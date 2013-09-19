#include "StdAfx.h"
#include <dvdmedia.h>
#include <initguid.h>
#include "Mpeg2ParserFilter.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


static const long SAMPLE_BUFFER_SIZE = 0x800000L;	// 8MiB


CMpeg2ParserFilter::CMpeg2ParserFilter(LPUNKNOWN pUnk, HRESULT *phr)
#ifndef MPEG2PARSERFILTER_INPLACE
	: CTransformFilter(MPEG2PARSERFILTER_NAME, pUnk, CLSID_Mpeg2ParserFilter)
#else
	: CTransInPlaceFilter(MPEG2PARSERFILTER_NAME, pUnk, CLSID_Mpeg2ParserFilter, phr, FALSE)
#endif
	, m_Mpeg2Parser(this)
	, m_bAttachMediaType(false)
	, m_pOutSample(NULL)
{
	TRACE(TEXT("CMpeg2ParserFilter::CMpeg2ParserFilter() %p\n"), this);

	*phr = S_OK;
}


CMpeg2ParserFilter::~CMpeg2ParserFilter()
{
	TRACE(TEXT("CMpeg2ParserFilter::~CMpeg2ParserFilter()\n"));
}


IBaseFilter* WINAPI CMpeg2ParserFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	// インスタンスを作成する
	CMpeg2ParserFilter *pNewFilter = new CMpeg2ParserFilter(pUnk, phr);
	if (FAILED(*phr)) {
		delete pNewFilter;
		return NULL;
	}

	IBaseFilter *pFilter;
	*phr = pNewFilter->QueryInterface(IID_IBaseFilter,
									  pointer_cast<void**>(&pFilter));
	if (FAILED(*phr)) {
		delete pNewFilter;
		return NULL;
	}

	return pFilter;
}


HRESULT CMpeg2ParserFilter::CheckInputType(const CMediaType* mtIn)
{
	CheckPointer(mtIn, E_POINTER);

	if (*mtIn->Type() == MEDIATYPE_Video) {
		if (*mtIn->Subtype() == MEDIASUBTYPE_MPEG2_VIDEO) {
			return S_OK;
		}
	}
	return VFW_E_TYPE_NOT_ACCEPTED;
}


#ifndef MPEG2PARSERFILTER_INPLACE


HRESULT CMpeg2ParserFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	CheckPointer(mtIn, E_POINTER);
	CheckPointer(mtOut, E_POINTER);

	if (*mtOut->Type() == MEDIATYPE_Video) {
		if (*mtOut->Subtype() == MEDIASUBTYPE_MPEG2_VIDEO) {
			return S_OK;
		}
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}


HRESULT CMpeg2ParserFilter::DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop)
{
	CheckPointer(pAllocator, E_POINTER);
	CheckPointer(pprop, E_POINTER);

	// バッファは1個あればよい
	if (pprop->cBuffers < 1)
		pprop->cBuffers = 1;

	if (pprop->cbBuffer < SAMPLE_BUFFER_SIZE)
		pprop->cbBuffer = SAMPLE_BUFFER_SIZE;

	// アロケータプロパティを設定しなおす
	ALLOCATOR_PROPERTIES Actual;
	HRESULT hr = pAllocator->SetProperties(pprop, &Actual);
	if (FAILED(hr))
		return hr;

	// 要求を受け入れられたか判定
	if (Actual.cBuffers < pprop->cBuffers
			|| Actual.cbBuffer < pprop->cbBuffer)
		return E_FAIL;

	return S_OK;
}


HRESULT CMpeg2ParserFilter::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	CheckPointer(pMediaType, E_POINTER);
	CAutoLock AutoLock(m_pLock);

	if (iPosition < 0)
		return E_INVALIDARG;
	if (iPosition > 0)
		return VFW_S_NO_MORE_ITEMS;

	*pMediaType = m_pInput->CurrentMediaType();

	return S_OK;
}


HRESULT CMpeg2ParserFilter::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	BYTE *pInData = NULL;
	HRESULT hr = pIn->GetPointer(&pInData);
	if (FAILED(hr))
		return hr;
	LONG InDataSize = pIn->GetActualDataLength();

	pOut->SetActualDataLength(0);
	m_pOutSample = pOut;

	CAutoLock Lock(&m_ParserLock);

	// シーケンスを取得
	m_Mpeg2Parser.StoreEs(pInData, InDataSize);

	m_BitRateCalculator.Update(InDataSize);

	return pOut->GetActualDataLength() > 0 ? S_OK : S_FALSE;
}


void CMpeg2ParserFilter::SetFixSquareDisplay(bool bFix)
{
	m_Mpeg2Parser.SetFixSquareDisplay(bFix);
}


#else	// ndef MPEG2PARSERFILTER_INPLACE


HRESULT CMpeg2ParserFilter::Transform(IMediaSample *pSample)
{
	BYTE *pData = NULL;
	HRESULT hr = pSample->GetPointer(&pData);
	if (FAILED(hr))
		return hr;
	LONG DataSize = pSample->GetActualDataLength();
	m_pOutSample = pSample;

	CAutoLock Lock(&m_ParserLock);

	// シーケンスを取得
	m_Mpeg2Parser.StoreEs(pData, DataSize);

	m_BitRateCalculator.Update(DataSize);

	return S_OK;
}


HRESULT CMpeg2ParserFilter::Receive(IMediaSample *pSample)
{
	const AM_SAMPLE2_PROPERTIES *pProps = m_pInput->SampleProps();
	if (pProps->dwStreamId != AM_STREAM_MEDIA)
		return m_pOutput->Deliver(pSample);

	if (UsingDifferentAllocators()) {
		pSample = Copy(pSample);
		if (pSample == NULL)
			return E_UNEXPECTED;
	}

	HRESULT hr = Transform(pSample);
	if (SUCCEEDED(hr)) {
		if (hr == S_OK)
			hr = m_pOutput->Deliver(pSample);
		else if (hr == S_FALSE)
			hr = S_OK;
	}

	if (UsingDifferentAllocators())
		pSample->Release();

	return hr;
}


#endif	// MPEG2PARSERFILTER_INPLACE


HRESULT CMpeg2ParserFilter::StartStreaming()
{
	CAutoLock Lock(&m_ParserLock);

	m_Mpeg2Parser.Reset();
	m_VideoInfo.Reset();

	m_BitRateCalculator.Initialize();

	return S_OK;
}


HRESULT CMpeg2ParserFilter::StopStreaming()
{
	CAutoLock Lock(&m_ParserLock);

	m_BitRateCalculator.Reset();

	return S_OK;
}


HRESULT CMpeg2ParserFilter::BeginFlush()
{
	HRESULT hr = __super::BeginFlush();

	CAutoLock Lock(&m_ParserLock);

	m_Mpeg2Parser.Reset();
	m_VideoInfo.Reset();

	return hr;
}


void CMpeg2ParserFilter::SetAttachMediaType(bool bAttach)
{
	CAutoLock Lock(&m_ParserLock);

	m_bAttachMediaType = bAttach;
}


void CMpeg2ParserFilter::OnMpeg2Sequence(const CMpeg2Parser *pMpeg2Parser, const CMpeg2Sequence *pSequence)
{
#ifndef MPEG2PARSERFILTER_INPLACE
	LONG Offset = m_pOutSample->GetActualDataLength();
	BYTE *pOutData = NULL;
	if (SUCCEEDED(m_pOutSample->GetPointer(&pOutData))
			&& SUCCEEDED(m_pOutSample->SetActualDataLength(Offset + pSequence->GetSize()))) {
		::CopyMemory(&pOutData[Offset], pSequence->GetData(), pSequence->GetSize());
	}
#endif

	WORD OrigWidth, OrigHeight;
	WORD DisplayWidth, DisplayHeight;
	BYTE AspectX, AspectY;

	OrigWidth = pSequence->GetHorizontalSize();
	OrigHeight = pSequence->GetVerticalSize();

	if (pSequence->GetExtendDisplayInfo()) {
		DisplayWidth = pSequence->GetExtendDisplayHorizontalSize();
		DisplayHeight = pSequence->GetExtendDisplayVerticalSize();
	} else {
		DisplayWidth = OrigWidth;
		DisplayHeight = OrigHeight;
	}

	if (!pSequence->GetAspectRatio(&AspectX, &AspectY))
		AspectX = AspectY = 0;

	CVideoParser::VideoInfo Info(OrigWidth, OrigHeight, DisplayWidth, DisplayHeight, AspectX, AspectY);

	pSequence->GetFrameRate(&Info.m_FrameRate.Num, &Info.m_FrameRate.Denom);

	if (Info != m_VideoInfo) {
		// 映像のサイズ及びフレームレートが変わった
		if (m_bAttachMediaType
				&& (m_VideoInfo.m_OrigWidth != OrigWidth ||
					m_VideoInfo.m_OrigHeight != OrigHeight)) {
			CMediaType MediaType(m_pOutput->CurrentMediaType());
			MPEG2VIDEOINFO *pVideoInfo = pointer_cast<MPEG2VIDEOINFO*>(MediaType.Format());

			if (pVideoInfo
					&& (pVideoInfo->hdr.bmiHeader.biWidth != OrigWidth ||
						pVideoInfo->hdr.bmiHeader.biHeight != OrigHeight)) {
				pVideoInfo->hdr.bmiHeader.biWidth = OrigWidth;
				pVideoInfo->hdr.bmiHeader.biHeight = OrigHeight;
				m_pOutSample->SetMediaType(&MediaType);
			}
		}

		m_VideoInfo = Info;

		TRACE(TEXT("Mpeg2 sequence %d x %d [%d x %d (%d=%d:%d)]\n"),
			  OrigWidth, OrigHeight, DisplayWidth, DisplayHeight,
			  pSequence->GetAspectRatioInfo(), AspectX, AspectY);

		// 通知
		NotifyVideoInfo();
	}
}
