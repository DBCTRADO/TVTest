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

	// ���f�B�A�^�C�v�ݒ�
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

	// ���f�B�A�^�C�v���`�F�b�N����
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

	// �o�b�t�@��1����΂悢
	if (pRequest->cBuffers < 1)
		pRequest->cBuffers = 1;

	// �o�b�t�@�T�C�Y�w��
	if (pRequest->cbBuffer < SAMPLE_BUFFER_SIZE)
		pRequest->cbBuffer = SAMPLE_BUFFER_SIZE;

	// �A���P�[�^�v���p�e�B��ݒ肵�Ȃ���
	ALLOCATOR_PROPERTIES Actual;
	HRESULT hr = pAlloc->SetProperties(pRequest, &Actual);
	if (FAILED(hr))
		return hr;

	// �v�����󂯓����ꂽ������
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
		// �T���v�����o�͂����̂�҂�
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


bool CBonSrcPin::MapAudioPID(WORD AudioPID, WORD MapPID)
{
	m_SrcStream.MapAudioPID(AudioPID, MapPID);
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
						�o�b�t�@�g�p�����݂̂ł́A�����Z�O���r�b�g���[�g���Ⴂ�ꍇ�ɂȂ��Ȃ��Đ����J�n����Ȃ��̂ŁA
						�r�b�g���[�g�� 2MB/s �Ƃ��Čo�ߎ��Ԃł����肷��B
						������ݒ�ł���悤�ɂ��邩�A���邢�͎��Ԃł̔���ɓ��ꂷ��������������m��Ȃ��B
					*/
					&& pThis->m_SrcStream.GetPTSDuration() <
						(LONGLONG)(pThis->m_SrcStream.GetQueueSize() * TS_PACKETSIZE) * PoolPercentage / (2000000LL * 100LL / 90000LL)) {
				Wait = 10;
				continue;
			}
			pThis->m_bBuffering = false;
		}

		if (pThis->m_SrcStream.IsDataAvailable()) {
			// ��̃��f�B�A�T���v����v������
			IMediaSample *pSample = NULL;
			HRESULT hr = pThis->GetDeliveryBuffer(&pSample, NULL, NULL, 0);
			if (SUCCEEDED(hr)) {
				// �������ݐ�|�C���^���擾����
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
