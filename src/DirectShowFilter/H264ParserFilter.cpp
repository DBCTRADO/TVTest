#include "StdAfx.h"
#include <initguid.h>
#include "H264ParserFilter.h"
#include "DirectShowUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/*
	タイムスタンプ調整の処理内容はワンセグ仕様前提なので、
	それ以外の場合は問題が出ると思います。
*/

// REFERENCE_TIMEの一秒
#define REFERENCE_TIME_SECOND 10000000LL

#ifdef BONTSENGINE_1SEG_SUPPORT

// フレームレート
#define FRAME_RATE_NUM		15000
#define FRAME_RATE_FACTOR	1001

// バッファサイズ
#define SAMPLE_BUFFER_SIZE	0x100000L	// 1MiB

// 初期値はほとんど何でもいいみたい
// (ワンセグ用の設定でも BD の m2ts を再生できる)
#define INITIAL_BITRATE		320000
#define INITIAL_WIDTH		320
#define INITIAL_HEIGHT		240

#else

#define FRAME_RATE_NUM		30000
#define FRAME_RATE_FACTOR	1001
#define SAMPLE_BUFFER_SIZE	0x800000L	// 8MiB
#define INITIAL_BITRATE		32000000;
#define INITIAL_WIDTH		1920;
#define INITIAL_HEIGHT		1080;

#endif

// フレームの表示時間を算出
#define FRAME_TIME(time) \
	((LONGLONG)(time) * REFERENCE_TIME_SECOND * FRAME_RATE_FACTOR / FRAME_RATE_NUM)


CH264ParserFilter::CH264ParserFilter(LPUNKNOWN pUnk, HRESULT *phr)
	: CTransformFilter(H264PARSERFILTER_NAME, pUnk, CLSID_H264ParserFilter)
	, m_H264Parser(this)
	, m_bAdjustTime(
#ifdef BONTSENGINE_1SEG_SUPPORT
		true
#else
		false
#endif
	  )
	, m_bAdjustFrameRate(true)
	, m_bAttachMediaType(false)
{
	TRACE(TEXT("CH264ParserFilter::CH264ParserFilter() %p\n"),this);

	m_MediaType.InitMediaType();
	m_MediaType.SetType(&MEDIATYPE_Video);
	m_MediaType.SetSubtype(&MEDIASUBTYPE_H264);
	m_MediaType.SetTemporalCompression(TRUE);
	m_MediaType.SetSampleSize(0);
	m_MediaType.SetFormatType(&FORMAT_VideoInfo);
	VIDEOINFOHEADER *pvih = pointer_cast<VIDEOINFOHEADER*>(m_MediaType.AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
	if (!pvih) {
		*phr = E_OUTOFMEMORY;
		return;
	}
	::ZeroMemory(pvih, sizeof(VIDEOINFOHEADER));
	pvih->dwBitRate = INITIAL_BITRATE;
	pvih->AvgTimePerFrame = FRAME_TIME(1);
	pvih->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pvih->bmiHeader.biWidth = INITIAL_WIDTH;
	pvih->bmiHeader.biHeight = INITIAL_HEIGHT;
	pvih->bmiHeader.biCompression = MAKEFOURCC('h','2','6','4');

	*phr = S_OK;
}


CH264ParserFilter::~CH264ParserFilter(void)
{
	TRACE(TEXT("CH264ParserFilter::~CH264ParserFilter()\n"));

	ClearSampleDataQueue(&m_SampleQueue);
}


IBaseFilter* WINAPI CH264ParserFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	// インスタンスを作成する
	CH264ParserFilter *pNewFilter = new CH264ParserFilter(pUnk, phr);
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


HRESULT CH264ParserFilter::CheckInputType(const CMediaType* mtIn)
{
	CheckPointer(mtIn, E_POINTER);

	if (*mtIn->Type() == MEDIATYPE_Video)
		return S_OK;

	return VFW_E_TYPE_NOT_ACCEPTED;
}


HRESULT CH264ParserFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	CheckPointer(mtIn, E_POINTER);
	CheckPointer(mtOut, E_POINTER);

	if (*mtOut->Type() == MEDIATYPE_Video
			&& (*mtOut->Subtype() == MEDIASUBTYPE_H264
				|| *mtOut->Subtype() == MEDIASUBTYPE_h264
				|| *mtOut->Subtype() == MEDIASUBTYPE_H264_bis
				|| *mtOut->Subtype() == MEDIASUBTYPE_AVC1
				|| *mtOut->Subtype() == MEDIASUBTYPE_avc1)) {
		return S_OK;
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}


HRESULT CH264ParserFilter::DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop)
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


HRESULT CH264ParserFilter::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	CheckPointer(pMediaType, E_POINTER);
	CAutoLock AutoLock(m_pLock);

	if (iPosition < 0)
		return E_INVALIDARG;
	if (iPosition > 0)
		return VFW_S_NO_MORE_ITEMS;

	*pMediaType = m_MediaType;

	return S_OK;
}


HRESULT CH264ParserFilter::StartStreaming()
{
	CAutoLock Lock(&m_ParserLock);

	Reset();
	m_VideoInfo.Reset();

	m_BitRateCalculator.Initialize();

	return S_OK;
}


HRESULT CH264ParserFilter::StopStreaming()
{
	CAutoLock Lock(&m_ParserLock);

	m_BitRateCalculator.Reset();

	return S_OK;
}


HRESULT CH264ParserFilter::BeginFlush()
{
	HRESULT hr = __super::BeginFlush();

	CAutoLock Lock(&m_ParserLock);

	Reset();
	m_VideoInfo.Reset();

	return hr;
}


HRESULT CH264ParserFilter::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	// 入力データポインタを取得する
	BYTE *pInData = NULL;
	HRESULT hr = pIn->GetPointer(&pInData);
	if (FAILED(hr))
		return hr;
	LONG InDataSize = pIn->GetActualDataLength();

	// 出力データポインタを取得する
	BYTE *pOutData = NULL;
	hr = pOut->GetPointer(&pOutData);
	if (FAILED(hr))
		return hr;
	pOut->SetActualDataLength(0);

	m_bChangeSize = false;

	{
		CAutoLock Lock(&m_ParserLock);

		if (m_bAdjustTime) {
			// タイムスタンプ取得
			REFERENCE_TIME StartTime, EndTime;
			hr = pIn->GetTime(&StartTime, &EndTime);
			if (hr == S_OK || hr == VFW_S_NO_STOP_TIME) {
				if (m_bAdjustFrameRate) {
					if (m_PrevTime >= 0
							&& (m_PrevTime >= StartTime
								|| m_PrevTime + REFERENCE_TIME_SECOND * 3 < StartTime)) {
						TRACE(TEXT("Reset H.264 media queue\n"));
						ClearSampleDataQueue(&m_SampleQueue);
					} else if (!m_SampleQueue.empty()) {
						const REFERENCE_TIME Duration = StartTime - m_PrevTime;
						const size_t Frames = m_SampleQueue.size();
						REFERENCE_TIME Start, End;

						Start = m_PrevTime;
						for (size_t i = 1; i <= Frames; i++) {
							CSampleData *pSampleData = m_SampleQueue.front();
							m_SampleQueue.pop_front();
							End = m_PrevTime + Duration * (REFERENCE_TIME)i / (REFERENCE_TIME)Frames;
							pSampleData->SetTime(Start, End);
							Start = End;
							m_OutSampleQueue.push_back(pSampleData);
						}
					}
					m_PrevTime = StartTime;
				} else {
					if (m_PrevTime < 0
							|| _abs64((m_PrevTime + FRAME_TIME(m_SampleCount)) - StartTime) > REFERENCE_TIME_SECOND / 5LL) {
#ifdef _DEBUG
						if (m_PrevTime >= 0)
							TRACE(TEXT("Reset H.264 sample time\n"));
#endif
						m_PrevTime = StartTime;
						m_SampleCount = 0;
					}
				}
			}

			hr = S_OK;
		} else {
			hr = pOut->SetActualDataLength(InDataSize);
			if (SUCCEEDED(hr))
				::CopyMemory(pOutData, pInData, InDataSize);
		}

		m_H264Parser.StoreEs(pInData, InDataSize);

		m_BitRateCalculator.Update(InDataSize);
	}

	if (!m_OutSampleQueue.empty()) {
		do {
			CSampleData *pSampleData = m_OutSampleQueue.front();

			hr = pOut->SetActualDataLength(pSampleData->GetSize());
			if (SUCCEEDED(hr)) {
				::CopyMemory(pOutData, pSampleData->GetData(), pSampleData->GetSize());

				if (pSampleData->HasTimestamp())
					pOut->SetTime(&pSampleData->m_StartTime, &pSampleData->m_EndTime);
				else
					pOut->SetTime(NULL, NULL);

				if (pSampleData->m_bChangeSize)
					AttachMediaType(pOut, pSampleData->m_Width, pSampleData->m_Height);
				else
					pOut->SetMediaType(NULL);

				hr = m_pOutput->Deliver(pOut);
			}
			m_OutSampleQueue.pop_front();
			m_SampleDataPool.Restore(pSampleData);
			if (FAILED(hr))
				break;
		} while (!m_OutSampleQueue.empty());

		pOut->SetActualDataLength(0);

		ClearSampleDataQueue(&m_OutSampleQueue);
	}

	if (SUCCEEDED(hr)) {
		if (pOut->GetActualDataLength() > 0) {
			if (m_bChangeSize)
				AttachMediaType(pOut, m_VideoInfo.m_OrigWidth, m_VideoInfo.m_OrigHeight);
			hr = S_OK;
		} else {
			hr = S_FALSE;
		}
	}

	return hr;
}


HRESULT CH264ParserFilter::Receive(IMediaSample *pSample)
{
	const AM_SAMPLE2_PROPERTIES *pProps = m_pInput->SampleProps();
	if (pProps->dwStreamId != AM_STREAM_MEDIA)
		return m_pOutput->Deliver(pSample);

	IMediaSample *pOutSample;
	HRESULT hr;

	hr = InitializeOutputSample(pSample, &pOutSample);
	if (FAILED(hr))
		return hr;
	hr = Transform(pSample, pOutSample);
	if (SUCCEEDED(hr)) {
		if (hr == S_OK)
			hr = m_pOutput->Deliver(pOutSample);
		else if (hr == S_FALSE)
			hr = S_OK;
		m_bSampleSkipped = FALSE;
	}

	pOutSample->Release();

	return hr;
}


bool CH264ParserFilter::SetAdjustTime(bool bAdjust)
{
	CAutoLock Lock(&m_ParserLock);

	if (m_bAdjustTime != bAdjust) {
		m_bAdjustTime = bAdjust;
		Reset();
	}
	return true;
}


bool CH264ParserFilter::SetAdjustFrameRate(bool bAdjust)
{
	CAutoLock Lock(&m_ParserLock);

	if (m_bAdjustFrameRate != bAdjust) {
		m_bAdjustFrameRate = bAdjust;
		if (m_bAdjustTime)
			Reset();
	}
	return true;
}


void CH264ParserFilter::SetAttachMediaType(bool bAttach)
{
	CAutoLock Lock(&m_ParserLock);

	m_bAttachMediaType = bAttach;
}


void CH264ParserFilter::Reset()
{
	m_H264Parser.Reset();
	m_PrevTime = -1;
	m_SampleCount = 0;
	ClearSampleDataQueue(&m_SampleQueue);
}


void CH264ParserFilter::ClearSampleDataQueue(CSampleDataQueue *pQueue)
{
	while (!pQueue->empty()) {
		m_SampleDataPool.Restore(pQueue->front());
		pQueue->pop_front();
	}
}


HRESULT CH264ParserFilter::AttachMediaType(IMediaSample *pSample, int Width, int Height)
{
	CMediaType MediaType(m_pOutput->CurrentMediaType());
	VIDEOINFOHEADER *pVideoInfo = pointer_cast<VIDEOINFOHEADER*>(MediaType.Format());
	HRESULT hr;

	if (pVideoInfo
			&& (pVideoInfo->bmiHeader.biWidth != Width ||
				pVideoInfo->bmiHeader.biHeight != Height)) {
		pVideoInfo->bmiHeader.biWidth = Width;
		pVideoInfo->bmiHeader.biHeight = Height;
		hr = pSample->SetMediaType(&MediaType);
	} else {
		hr = S_FALSE;
	}

	return hr;
}


void CH264ParserFilter::OnAccessUnit(const CH264Parser *pParser, const CH264AccessUnit *pAccessUnit)
{
	WORD OrigWidth, OrigHeight;
	OrigWidth = pAccessUnit->GetHorizontalSize();
	OrigHeight = pAccessUnit->GetVerticalSize();
	bool bChangeSize = false;

	if (m_bAttachMediaType
			&& (m_VideoInfo.m_OrigWidth != OrigWidth ||
				m_VideoInfo.m_OrigHeight != OrigHeight)) {
		bChangeSize = true;
		m_bChangeSize = true;
	}

	if (m_bAdjustTime) {
		// ワンセグは1フレーム単位でタイムスタンプを設定しないとかくつく
		CSampleData *pSampleData = m_SampleDataPool.Get(*pAccessUnit);

		if (pSampleData != NULL) {
			if (bChangeSize)
				pSampleData->ChangeSize(OrigWidth, OrigHeight);

			if (m_bAdjustFrameRate && m_PrevTime >= 0) {
				m_SampleQueue.push_back(pSampleData);
			} else {
				if (m_PrevTime >= 0) {
					REFERENCE_TIME StartTime = m_PrevTime + FRAME_TIME(m_SampleCount);
					REFERENCE_TIME EndTime = m_PrevTime + FRAME_TIME(m_SampleCount + 1) - 1;
					pSampleData->SetTime(StartTime, EndTime);
				}
				m_OutSampleQueue.push_back(pSampleData);
			}
		}
		m_SampleCount++;
	}

	WORD DisplayWidth, DisplayHeight;
	BYTE AspectX = 0, AspectY = 0;

	DisplayWidth = OrigWidth;
	DisplayHeight = OrigHeight;

	WORD SarX = 0, SarY = 0;
	if (pAccessUnit->GetSAR(&SarX, &SarY) && SarX != 0 && SarY != 0) {
		DWORD Width = OrigWidth * SarX, Height = OrigHeight * SarY;

		// とりあえず 16:9 と 4:3 だけ
		if (Width * 9 == Height * 16) {
			AspectX = 16;
			AspectY = 9;
		} else if (Width * 3 == Height * 4) {
			AspectX = 4;
			AspectY = 3;
		}
	} else {
		if (OrigWidth * 9 == OrigHeight * 16) {
			AspectX = 16;
			AspectY = 9;
		} else if (OrigWidth * 3 == OrigHeight * 4) {
			AspectX = 4;
			AspectY = 3;
		}
	}

	CVideoParser::VideoInfo Info(OrigWidth, OrigHeight, DisplayWidth, DisplayHeight, AspectX, AspectY);

	CH264AccessUnit::TimingInfo TimingInfo;
	if (pAccessUnit->GetTimingInfo(&TimingInfo)) {
		// 実際のフレームレートではない
		Info.m_FrameRate.Num = TimingInfo.TimeScale;
		Info.m_FrameRate.Denom = TimingInfo.NumUnitsInTick;
	}

	if (Info != m_VideoInfo) {
		// 映像のサイズ及びフレームレートが変わった
		m_VideoInfo = Info;

		TRACE(TEXT("H.264 access unit %d x %d [SAR %d:%d (DAR %d:%d)] %lu/%lu\n"),
			  OrigWidth, OrigHeight, SarX, SarY, AspectX, AspectY,
			  Info.m_FrameRate.Num, Info.m_FrameRate.Denom);

		// 通知
		NotifyVideoInfo();
	}
}




CH264ParserFilter::CSampleData::CSampleData(const CMediaData &Data)
	: CMediaData(Data)
{
	ResetProperties();
}


void CH264ParserFilter::CSampleData::ResetProperties()
{
	m_StartTime = -1;
	m_EndTime = -1;
	m_bChangeSize = false;
}


void CH264ParserFilter::CSampleData::SetTime(const REFERENCE_TIME &StartTime, const REFERENCE_TIME &EndTime)
{
	m_StartTime = StartTime;
	m_EndTime = EndTime;
}


void CH264ParserFilter::CSampleData::ChangeSize(int Width, int Height)
{
	m_bChangeSize = true;
	m_Width = Width;
	m_Height = Height;
}




CH264ParserFilter::CSampleDataPool::CSampleDataPool()
	: m_MaxData(256)
	, m_DataCount(0)
{
}


CH264ParserFilter::CSampleDataPool::~CSampleDataPool()
{
	TRACE(TEXT("CH264ParserFilter::CSampleDataPool::~CSampleDataPool() Data count %lu / %lu\n"),
		  (unsigned long)m_DataCount, (unsigned long)m_MaxData);

	Clear();
}


void CH264ParserFilter::CSampleDataPool::Clear()
{
	CAutoLock Lock(&m_Lock);

	while (!m_Queue.empty()) {
		delete m_Queue.back();
		m_Queue.pop_back();
	}

	m_DataCount = 0;
}


CH264ParserFilter::CSampleData *CH264ParserFilter::CSampleDataPool::Get(const CMediaData &Data)
{
	CAutoLock Lock(&m_Lock);

	if (m_Queue.empty()) {
		if (m_DataCount >= m_MaxData)
			return NULL;
		m_DataCount++;
		return new CSampleData(Data);
	}

	CSampleData *pData = m_Queue.back();
	pData->ResetProperties();
	const DWORD Size = Data.GetSize();
	if (pData->SetData(Data.GetData(), Size) < Size)
		return NULL;
	m_Queue.pop_back();
	return pData;
}


void CH264ParserFilter::CSampleDataPool::Restore(CSampleData *pData)
{
	CAutoLock Lock(&m_Lock);

	m_Queue.push_back(pData);
}
