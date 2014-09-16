#include "StdAfx.h"
#include "BonSrcPin.h"
#include "BonSrcFilter.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#ifndef TS_PACKETSIZE
#define TS_PACKETSIZE	188
#endif

#define SAMPLE_PACKETS		1024
#define SAMPLE_BUFFER_SIZE	(TS_PACKETSIZE * SAMPLE_PACKETS)




CBonSrcPin::CBonSrcPin(HRESULT *phr, CBonSrcFilter *pFilter)
	: CBaseOutputPin(TEXT("CBonSrcPin"), pFilter, pFilter->m_pLock, phr, L"TS")
	, m_pFilter(pFilter)
	, m_hThread(NULL)
	, m_hEndEvent(NULL)
	, m_InitialPoolPercentage(0)
	, m_bBuffering(false)
	, m_bOutputWhenPaused(false)
	, m_InputWait(0)
	, m_bInputTimeout(false)
{
	TRACE(TEXT("CBonSrcPin::CBonSrcPin() %p\n"), this);

	*phr = S_OK;
}


CBonSrcPin::~CBonSrcPin()
{
	TRACE(TEXT("CBonSrcPin::~CBonSrcPin()\n"));

	EndStreamThread();
}


HRESULT CBonSrcPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	CheckPointer(pMediaType, E_POINTER);

	if (iPosition < 0)
		return E_INVALIDARG;
	if (iPosition > 0)
		return VFW_S_NO_MORE_ITEMS;

	// メディアタイプ設定
	pMediaType->InitMediaType();
	pMediaType->SetType(&MEDIATYPE_Stream);
	pMediaType->SetSubtype(&MEDIASUBTYPE_MPEG2_TRANSPORT);
	pMediaType->SetTemporalCompression(FALSE);
	pMediaType->SetSampleSize(TS_PACKETSIZE);

	return S_OK;
}


HRESULT CBonSrcPin::CheckMediaType(const CMediaType *pMediaType)
{
	CheckPointer(pMediaType, E_POINTER);
	CAutoLock AutoLock(&m_pFilter->m_cStateLock);

	// メディアタイプをチェックする
	CMediaType mt;
	GetMediaType(0, &mt);

	return (*pMediaType == mt) ? S_OK : E_FAIL;
}


HRESULT CBonSrcPin::Active()
{
	TRACE(TEXT("CBonSrcPin::Active()\n"));

	HRESULT hr = CBaseOutputPin::Active();
	if (FAILED(hr))
		return hr;

	if (m_hThread)
		return E_UNEXPECTED;

	if (!m_SrcStream.Initialize())
		return E_OUTOFMEMORY;

	m_bBuffering = m_InitialPoolPercentage > 0;
	m_bInputTimeout = false;

	if (m_hEndEvent == NULL) {
		m_hEndEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
		if (m_hEndEvent == NULL) {
			DWORD Error = ::GetLastError();
			return HRESULT_FROM_WIN32(Error);
		}
	} else {
		::ResetEvent(m_hEndEvent);
	}

	m_hThread = reinterpret_cast<HANDLE>(::_beginthreadex(NULL, 0, StreamThread, this, 0, NULL));
	if (m_hThread == NULL) {
		return E_FAIL;
	}

	return S_OK;
}


void CBonSrcPin::EndStreamThread()
{
	if (m_hThread) {
		::SetEvent(m_hEndEvent);
		if (::WaitForSingleObject(m_hThread, 5000) == WAIT_TIMEOUT) {
			::TerminateThread(m_hThread, -1);
		}
		::CloseHandle(m_hThread);
		m_hThread = NULL;
	}

	if (m_hEndEvent) {
		::CloseHandle(m_hEndEvent);
		m_hEndEvent = NULL;
	}
}


HRESULT CBonSrcPin::Inactive()
{
	TRACE(TEXT("CBonSrcPin::Inactive()\n"));

	HRESULT hr = CBaseOutputPin::Inactive();

	EndStreamThread();

	return hr;
}


HRESULT CBonSrcPin::Run(REFERENCE_TIME tStart)
{
	TRACE(TEXT("CBonSrcPin::Run()\n"));

	return CBaseOutputPin::Run(tStart);
}


HRESULT CBonSrcPin::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest)
{
	CheckPointer(pAlloc, E_POINTER);
	CheckPointer(pRequest, E_POINTER);

	// バッファは1個あればよい
	if (pRequest->cBuffers < 1)
		pRequest->cBuffers = 1;

	// バッファサイズ指定
	if (pRequest->cbBuffer < SAMPLE_BUFFER_SIZE)
		pRequest->cbBuffer = SAMPLE_BUFFER_SIZE;

	// アロケータプロパティを設定しなおす
	ALLOCATOR_PROPERTIES Actual;
	HRESULT hr = pAlloc->SetProperties(pRequest, &Actual);
	if (FAILED(hr))
		return hr;

	// 要求を受け入れられたか判定
	if (Actual.cBuffers < pRequest->cBuffers
			|| Actual.cbBuffer < pRequest->cbBuffer)
		return E_FAIL;

	return S_OK;
}


bool CBonSrcPin::InputMedia(CMediaData *pMediaData)
{
	const DWORD Wait = m_InputWait;
	if (Wait != 0 && m_SrcStream.IsBufferFull()) {
		if (m_bInputTimeout)
			return false;
		// サンプルが出力されるのを待つ
		const DWORD BeginTime = ::GetTickCount();
		for (;;) {
			::Sleep(10);
			if (!m_SrcStream.IsBufferFull())
				break;
			if ((DWORD)(::GetTickCount() - BeginTime) >= Wait) {
				TRACE(TEXT("CBonSrcPin::InputMedia() : Timeout %u ms\n"), Wait);
				m_bInputTimeout = true;
				return false;
			}
		}
	}

	m_SrcStream.InputMedia(pMediaData);

	m_bInputTimeout = false;

	return true;
}


void CBonSrcPin::Reset()
{
	m_SrcStream.Reset();
}


void CBonSrcPin::Flush()
{
	TRACE(TEXT("CBonSrcPin::Flush()\n"));
	Reset();
	DeliverBeginFlush();
	DeliverEndFlush();
}


bool CBonSrcPin::EnableSync(bool bEnable, bool b1Seg)
{
	return m_SrcStream.EnableSync(bEnable, b1Seg);
}


bool CBonSrcPin::IsSyncEnabled() const
{
	return m_SrcStream.IsSyncEnabled();
}


void CBonSrcPin::SetVideoPID(WORD PID)
{
	m_SrcStream.SetVideoPID(PID);
}


void CBonSrcPin::SetAudioPID(WORD PID)
{
	m_SrcStream.SetAudioPID(PID);
}


bool CBonSrcPin::SetBufferSize(size_t Size)
{
	return m_SrcStream.SetQueueSize(Size != 0 ? Size : CTsSrcStream::DEFAULT_QUEUE_SIZE);
}


bool CBonSrcPin::SetInitialPoolPercentage(int Percentage)
{
	if (Percentage < 0 || Percentage > 100)
		return false;

	m_InitialPoolPercentage = Percentage;

	return true;
}


int CBonSrcPin::GetBufferFillPercentage()
{
	return m_SrcStream.GetFillPercentage();
}


bool CBonSrcPin::SetInputWait(DWORD Wait)
{
	m_InputWait = Wait;
	return true;
}


unsigned int __stdcall CBonSrcPin::StreamThread(LPVOID lpParameter)
{
	CBonSrcPin *pThis = static_cast<CBonSrcPin*>(lpParameter);

	TRACE(TEXT("CBonSrcPin::StreamThread() Start\n"));

	::CoInitialize(NULL);

	DWORD Wait = 0;

	while (::WaitForSingleObject(pThis->m_hEndEvent, Wait) == WAIT_TIMEOUT) {
		if (pThis->m_bBuffering) {
			const int PoolPercentage = pThis->m_InitialPoolPercentage;
			if (pThis->m_SrcStream.GetFillPercentage() < PoolPercentage
					/*
						バッファ使用割合のみでは、ワンセグ等ビットレートが低い場合になかなか再生が開始されないので、
						ビットレートを 2MB/s として経過時間でも判定する。
						これも設定できるようにするか、あるいは時間での判定に統一する方がいいかも知れない。
					*/
					&& pThis->m_SrcStream.GetPTSDuration() <
						(LONGLONG)(pThis->m_SrcStream.GetQueueSize() * TS_PACKETSIZE) * PoolPercentage / (2000000LL * 100LL / 90000LL)) {
				Wait = 10;
				continue;
			}
			pThis->m_bBuffering = false;
		}

		if (pThis->m_SrcStream.IsDataAvailable()) {
			// 空のメディアサンプルを要求する
			IMediaSample *pSample = NULL;
			HRESULT hr = pThis->GetDeliveryBuffer(&pSample, NULL, NULL, 0);
			if (SUCCEEDED(hr)) {
				// 書き込み先ポインタを取得する
				BYTE *pSampleData = NULL;
				hr = pSample->GetPointer(&pSampleData);
				if (SUCCEEDED(hr)) {
					size_t Size = pThis->m_SrcStream.GetData(pSampleData, SAMPLE_PACKETS);
					if (Size != 0) {
						pSample->SetActualDataLength(static_cast<long>(Size * TS_PACKETSIZE));
						pThis->Deliver(pSample);
					}
					pSample->Release();
				}
			}
			Wait = 0;
		} else {
			Wait = 10;
		}
	}

	::CoUninitialize();

	TRACE(TEXT("CBonSrcPin::StreamThread() End\n"));

	return 0;
}
