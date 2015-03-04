#include "StdAfx.h"
#include <initguid.h>
#include "H265ParserFilter.h"
#include "DirectShowUtil.h"
#include "../Common/DebugDef.h"


CH265ParserFilter::CH265ParserFilter(LPUNKNOWN pUnk, HRESULT *phr)
	: CTransInPlaceFilter(H265PARSERFILTER_NAME, pUnk, CLSID_H265ParserFilter, phr, FALSE)
	, m_H265Parser(this)
{
	TRACE(TEXT("CH265ParserFilter::CH265ParserFilter() %p\n"), this);

	*phr = S_OK;
}


CH265ParserFilter::~CH265ParserFilter()
{
	TRACE(TEXT("CH265ParserFilter::~CH265ParserFilter()\n"));
}


IBaseFilter* WINAPI CH265ParserFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	// インスタンスを作成する
	CH265ParserFilter *pNewFilter = new CH265ParserFilter(pUnk, phr);
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


HRESULT CH265ParserFilter::CheckInputType(const CMediaType* mtIn)
{
	CheckPointer(mtIn, E_POINTER);

	if (*mtIn->Type() == MEDIATYPE_Video) {
		if (*mtIn->Subtype() == MEDIASUBTYPE_HEVC) {
			return S_OK;
		}
	}
	return VFW_E_TYPE_NOT_ACCEPTED;
}


HRESULT CH265ParserFilter::Transform(IMediaSample *pSample)
{
	BYTE *pData = NULL;
	HRESULT hr = pSample->GetPointer(&pData);
	if (FAILED(hr))
		return hr;
	LONG DataSize = pSample->GetActualDataLength();

	CAutoLock Lock(&m_ParserLock);

	// シーケンスを取得
	m_H265Parser.StoreEs(pData, DataSize);

	m_BitRateCalculator.Update(DataSize);

	return S_OK;
}


HRESULT CH265ParserFilter::Receive(IMediaSample *pSample)
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


HRESULT CH265ParserFilter::StartStreaming()
{
	CAutoLock Lock(&m_ParserLock);

	m_H265Parser.Reset();
	m_VideoInfo.Reset();

	m_BitRateCalculator.Initialize();

	return S_OK;
}


HRESULT CH265ParserFilter::StopStreaming()
{
	CAutoLock Lock(&m_ParserLock);

	m_BitRateCalculator.Reset();

	return S_OK;
}


HRESULT CH265ParserFilter::BeginFlush()
{
	HRESULT hr = CTransInPlaceFilter::BeginFlush();

	CAutoLock Lock(&m_ParserLock);

	m_H265Parser.Reset();
	m_VideoInfo.Reset();

	return hr;
}


void CH265ParserFilter::OnAccessUnit(const CH265Parser *pParser, const CH265AccessUnit *pAccessUnit)
{
	WORD OrigWidth, OrigHeight;
	OrigWidth = pAccessUnit->GetHorizontalSize();
	OrigHeight = pAccessUnit->GetVerticalSize();

	BYTE AspectX = 0, AspectY = 0;
	WORD SarX = 0, SarY = 0;
	if (pAccessUnit->GetSAR(&SarX, &SarY) && SarX != 0 && SarY != 0) {
		SARToDAR(SarX, SarY, OrigWidth, OrigHeight, &AspectX, &AspectY);
	} else {
		SARToDAR(1, 1, OrigWidth, OrigHeight, &AspectX, &AspectY);
	}

	CVideoParser::VideoInfo Info(OrigWidth, OrigHeight, OrigWidth, OrigHeight, AspectX, AspectY);

	CH265AccessUnit::TimingInfo TimingInfo;
	if (pAccessUnit->GetTimingInfo(&TimingInfo)) {
		Info.m_FrameRate.Num = TimingInfo.TimeScale;
		Info.m_FrameRate.Denom = TimingInfo.NumUnitsInTick;
	}

	if (Info != m_VideoInfo) {
		// 映像のサイズ及びフレームレートが変わった
		m_VideoInfo = Info;

		TRACE(TEXT("H.265 access unit %d x %d [SAR %d:%d (DAR %d:%d)] %lu/%lu\n"),
			  OrigWidth, OrigHeight, SarX, SarY, AspectX, AspectY,
			  Info.m_FrameRate.Num, Info.m_FrameRate.Denom);

		// 通知
		NotifyVideoInfo();
	}
}
