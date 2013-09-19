#include "stdafx.h"
#include <new>
#include "MediaBuffer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CMediaBuffer::CMediaBuffer(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler,1,1)
	, m_BufferLength(40000)
	, m_bEnableBuffering(true)
	, m_bFileMode(false)
	, m_PoolPercentage(50)
	, m_pBuffer(NULL)
	, m_hOutputThread(NULL)
#ifdef _DEBUG
	, m_InputCount(0)
	, m_OutputCount(0)
#endif
{
	m_OutputData.SetSize(TS_PACKETSIZE);
	m_BreakEvent.Create();
	m_CompleteEvent.Create();
}


CMediaBuffer::~CMediaBuffer()
{
	Stop();
	delete [] m_pBuffer;
#ifdef _DEBUG
	TRACE(TEXT("CMediaBuffer::~CMediaBuffer input %lu / output %lu\n"),m_InputCount,m_OutputCount);
#endif
}


void CMediaBuffer::Reset(void)
{
	TRACE(TEXT("CMediaBuffer::Reset()\n"));
	ResetBuffer();
}


const bool CMediaBuffer::InputMedia(CMediaData *pMediaData,const DWORD dwInputIndex)
{
	/*
	if (dwInputIndex>=GetInputNum())
		return false;
	*/

	CTsPacket *pPacket=static_cast<CTsPacket*>(pMediaData);

	// 視聴に不要なパケットを破棄する
	/*
	WORD PID=pPacket->GetPID();
	if (PID==0x0010 || PID==0x0011 || PID==0x0012 || PID==0x0014 || PID==0x0024)
		return true;
	*/

	if (!m_bEnableBuffering || m_hOutputThread==NULL)
		return OutputMedia(pMediaData);

	m_Lock.Lock();

#ifdef _DEBUG
	m_InputCount++;
#endif

	if (m_bBuffering) {
		// バッファにパケットが溜まったら出力を開始する
		if (m_UsedCount>=m_BufferLength*m_PoolPercentage/100)
			m_bBuffering=false;
	} else if (m_UsedCount==m_BufferLength) {
		if (m_bFileMode) {
			// 出力スレッドがパケットを出力するのを待つ
			// ファイルからの再生の場合はこうしないとまずい
			// チューナーからの入力でこれをすると録画に影響を与える
#if 0
			// この方法ではフィルタグラフが固まった時に一緒に固まる
			do {
				m_Lock.Unlock();
				::Sleep(1);
				m_Lock.Lock();
			} while (m_UsedCount==m_BufferLength);
#else
			m_Lock.Unlock();
			::Sleep(100);
			m_Lock.Lock();
			if (m_UsedCount==m_BufferLength) {
				//TRACE(TEXT("CMediaBuffer::InputMedia() buffer over flow\n"));
				m_FirstBuffer++;
				if (m_FirstBuffer==m_BufferLength)
					m_FirstBuffer=0;
				m_UsedCount--;
			}
#endif
		} else {
			//TRACE(TEXT("CMediaBuffer::InputMedia() buffer over flow\n"));
			// BonDriverのバッファオーバーフローを防ぐために、ブロッキングはしない
			m_FirstBuffer++;
			if (m_FirstBuffer==m_BufferLength)
				m_FirstBuffer=0;
			m_UsedCount--;
		}
	}

	pPacket->StoreToBuffer(m_pBuffer+m_LastBuffer*CTsPacket::BUFFER_SIZE);
	m_LastBuffer++;
	if (m_LastBuffer==m_BufferLength)
		m_LastBuffer=0;
	m_UsedCount++;

	m_Lock.Unlock();

	return true;
}


bool CMediaBuffer::Play()
{
	TRACE(TEXT("CMediaBuffer::Play()\n"));
	if (m_hOutputThread)
		return false;
	if (!m_bEnableBuffering)
		return true;
	m_Lock.Lock();
	if (m_pBuffer==NULL) {
		try {
			m_pBuffer=new BYTE[m_BufferLength*CTsPacket::BUFFER_SIZE];
		} catch (std::bad_alloc) {
			m_Lock.Unlock();
			return false;
		}
	}
	m_FirstBuffer=0;
	m_LastBuffer=0;
	m_UsedCount=0;
	m_bBuffering=true;
	m_BreakEvent.Reset();
	m_hOutputThread=(HANDLE)::_beginthreadex(NULL,0,OutputThread,this,0,NULL);
	m_Lock.Unlock();
	if (m_hOutputThread==NULL)
		return false;
	return true;
}


bool CMediaBuffer::Stop()
{
	TRACE(TEXT("CMediaBuffer::Stop()\n"));
	if (m_hOutputThread) {
		m_SignalType=SIGNAL_KILL;
		m_BreakEvent.Set();
		if (::WaitForSingleObject(m_hOutputThread,2000)!=WAIT_OBJECT_0) {
			TRACE(TEXT("Terminate CMediaBuffer::OutputThread\n"));
			::TerminateThread(m_hOutputThread,1);
		}
		::CloseHandle(m_hOutputThread);
		m_hOutputThread=NULL;
	}
	return true;
}


bool CMediaBuffer::EnableBuffering(bool bBuffering)
{
	if (bBuffering!=m_bEnableBuffering) {
		m_bEnableBuffering=bBuffering;
		if (!bBuffering) {
			if (m_hOutputThread)
				Stop();
		} else {
			Play();
		}
	}
	return true;
}


void CMediaBuffer::ResetBuffer()
{
	if (m_hOutputThread) {
		m_CompleteEvent.Reset();
		m_SignalType=SIGNAL_RESET;
		m_BreakEvent.Set();
		m_CompleteEvent.Wait(2000/*INFINITE*/);
	}
}


bool CMediaBuffer::SetBufferLength(DWORD BufferLength)
{
	if (BufferLength<1)
		return false;
	if (BufferLength==m_BufferLength)
		return true;
	m_Lock.Lock();
	if (m_pBuffer) {
		BYTE *pNewBuffer;

		try {
			pNewBuffer=new BYTE[BufferLength*CTsPacket::BUFFER_SIZE];
		} catch (std::bad_alloc) {
			m_Lock.Unlock();
			return false;
		}

		if (m_UsedCount>0) {
			if (m_UsedCount>BufferLength) {
				long First;

				m_UsedCount=BufferLength;
				First=(long)m_LastBuffer-m_UsedCount;
				if (First<0)
					First=m_BufferLength+First;
				m_FirstBuffer=First;
			}
			if (m_LastBuffer>m_FirstBuffer) {
				::CopyMemory(pNewBuffer,
							 m_pBuffer+m_FirstBuffer*CTsPacket::BUFFER_SIZE,
							 m_UsedCount*CTsPacket::BUFFER_SIZE);
			} else {
				SIZE_T Size=(m_BufferLength-m_FirstBuffer)*CTsPacket::BUFFER_SIZE;

				::CopyMemory(pNewBuffer,
						m_pBuffer+m_FirstBuffer*CTsPacket::BUFFER_SIZE,Size);
				::CopyMemory(pNewBuffer+Size,m_pBuffer,
										m_LastBuffer*CTsPacket::BUFFER_SIZE);
			}
		}
		m_FirstBuffer=0;
		m_LastBuffer=m_UsedCount;
		delete [] m_pBuffer;
		m_pBuffer=pNewBuffer;
	}
	m_BufferLength=BufferLength;
	m_Lock.Unlock();
	return true;
}


bool CMediaBuffer::SetFileMode(bool bFileMode)
{
	m_bFileMode=bFileMode;
	return true;
}


bool CMediaBuffer::SetPoolPercentage(int Percentage)
{
	if (Percentage<0 || Percentage>100)
		return false;
	m_PoolPercentage=Percentage;
	return true;
}


DWORD CMediaBuffer::GetUsedBufferCount()
{
	if (m_hOutputThread==NULL)
		return 0;
	return m_UsedCount;
}


unsigned int __stdcall CMediaBuffer::OutputThread(LPVOID lpParameter)
{
	CMediaBuffer *pThis=static_cast<CMediaBuffer*>(lpParameter);

	TRACE(TEXT("CMediaBuffer::OutputThread() Begin\n"));

	::CoInitialize(NULL);

	while (true) {
		while (pThis->m_BreakEvent.Wait(10)==WAIT_TIMEOUT) {
			pThis->m_Lock.Lock();
			while (!pThis->m_bBuffering && pThis->m_UsedCount>0) {
				if (pThis->m_BreakEvent.IsSignaled()) {
					pThis->m_Lock.Unlock();
					goto Break;
				}
				pThis->m_OutputData.RestoreFromBuffer(
					pThis->m_pBuffer+pThis->m_FirstBuffer*CTsPacket::BUFFER_SIZE);
				pThis->m_FirstBuffer++;
				if (pThis->m_FirstBuffer==pThis->m_BufferLength)
					pThis->m_FirstBuffer=0;
				pThis->m_UsedCount--;
#ifdef _DEBUG
				pThis->m_OutputCount++;
#endif
				pThis->m_Lock.Unlock();
				pThis->OutputMedia(&pThis->m_OutputData);
				pThis->m_Lock.Lock();
			}
			pThis->m_Lock.Unlock();
		}
	Break:
		if (pThis->m_SignalType==SIGNAL_KILL)
			break;
		pThis->m_Lock.Lock();
		pThis->m_FirstBuffer=0;
		pThis->m_LastBuffer=0;
		pThis->m_UsedCount=0;
		pThis->m_bBuffering=true;
		pThis->m_Lock.Unlock();
		pThis->m_CompleteEvent.Set();
	}

	::CoUninitialize();

	TRACE(TEXT("CMediaBuffer::OutputThread() End\n"));

	return 0;
}
