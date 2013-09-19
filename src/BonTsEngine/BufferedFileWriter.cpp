#include "stdafx.h"
#include "BufferedFileWriter.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CBufferedFileWriter::CBufferedFileWriter(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1UL, 0UL)
	, m_pOutFile(NULL)
	, m_BufferSize(CNCachedFile::DEFBUFFSIZE)
	, m_llWriteSize(0U)
	, m_llWriteCount(0U)
	, m_bWriteError(false)
	, m_bPause(false)
	, m_bEnableQueueing(false)
	, m_QueueBlockSize(CNCachedFile::DEFBUFFSIZE / 188 * 188)
	, m_MaxQueueSize(16)
	, m_MaxPendingSize(0x20000000)
	, m_hThread(NULL)
{
}


CBufferedFileWriter::~CBufferedFileWriter()
{
	ClearQueue();
	CloseFile();
}


void CBufferedFileWriter::Reset(void)
{
	CBlockLock Lock(&m_DecoderLock);

	ClearQueue();
}


const bool CBufferedFileWriter::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	/*
	if (dwInputIndex>=GetInputNum())
		return false;
	*/

	if (m_bEnableQueueing || (m_pOutFile && !m_bPause))
		PushData(pMediaData->GetData(), pMediaData->GetSize());

	OutputMedia(pMediaData);

	return true;
}


CNCachedFile *CBufferedFileWriter::CreateNewFile(LPCTSTR pszFileName, UINT Flags)
{
	CNCachedFile *pFile = new CNCachedFile;

	if (!pFile->Open(pszFileName, CNCachedFile::CNF_WRITE | CNCachedFile::CNF_NEW | Flags, m_BufferSize)) {
		TCHAR szMessage[1024];
		pFile->GetLastErrorMessage(szMessage, 1024);
		SetError(pFile->GetLastError(), TEXT("ファイルが開けません。"), NULL, szMessage);
		delete pFile;
		return NULL;
	}

	return pFile;
}


const bool CBufferedFileWriter::OpenFile(LPCTSTR pszFileName, UINT Flags)
{
	if (!pszFileName) {
		SetError(TEXT("引数が不正です。"));
		return false;
	}

	CBlockLock Lock(&m_DecoderLock);

	// 一旦閉じる
	CloseFile();
	m_llWriteSize = 0U;
	m_llWriteCount = 0U;

	// ファイルを開く
	m_pOutFile = CreateNewFile(pszFileName, Flags);
	if (!m_pOutFile)
		return false;

	m_bWriteError = false;
	m_bPause = false;

	// 書き出しスレッド開始
	m_EndEvent.Create();
	m_hThread = (HANDLE)::_beginthreadex(NULL, 0, ThreadProc, this, 0, NULL);
	if (!m_hThread) {
		CloseFile();
		SetError(TEXT("スレッドが作成できません。"));
		return false;
	}

	ClearError();

	return true;
}


const bool CBufferedFileWriter::RelayFile(LPCTSTR pszFileName, UINT Flags)
{
	if (!pszFileName) {
		SetError(TEXT("引数が不正です。"));
		return false;
	}

	if (!m_pOutFile)
		return OpenFile(pszFileName, Flags);

	CNCachedFile *pNewFile = CreateNewFile(pszFileName, Flags);
	if (!pNewFile)
		return false;

	CBlockLock Lock1(&m_DecoderLock);
	CBlockLock Lock2(&m_FileLock);

	m_pOutFile->Close();
	delete m_pOutFile;
	m_pOutFile = pNewFile;

	m_bWriteError = false;

	return true;
}


void CBufferedFileWriter::CloseFile(void)
{
	CBlockLock Lock(&m_DecoderLock);

	// 書き出しスレッド終了
	if (m_hThread) {
		Trace(TEXT("ファイル書き出しスレッドを停止しています..."));
		m_EndEvent.Set();
		if (::WaitForSingleObject(m_hThread, 10000) != WAIT_OBJECT_0) {
			typedef BOOL (WINAPI *CancelSynchronousIoPtr)(HANDLE);
			CancelSynchronousIoPtr pCancelSynchronousIo=
				reinterpret_cast<CancelSynchronousIoPtr>(::GetProcAddress(
					::GetModuleHandle(TEXT("kernel32.dll")),"CancelSynchronousIo"));
			if (pCancelSynchronousIo!=NULL) {
				pCancelSynchronousIo(m_hThread);
				if (::WaitForSingleObject(m_hThread, 5000) != WAIT_OBJECT_0) {
					Trace(TEXT("ファイル書き出しスレッドが応答しないため強制終了します。"));
					::TerminateThread(m_hThread, -1);
				}
			} else {
				Trace(TEXT("ファイル書き出しスレッドが応答しないため強制終了します。"));
				::TerminateThread(m_hThread, -1);
			}
		}
		::CloseHandle(m_hThread);
		m_hThread = NULL;
	}

	m_EndEvent.Close();

	// ファイルを閉じる
	if (m_pOutFile) {
		m_pOutFile->Close();
		delete m_pOutFile;
		m_pOutFile = NULL;
	}

	/*
	m_llWriteSize = 0U;
	m_llWriteCount = 0U;
	*/
}


const bool CBufferedFileWriter::IsOpen() const
{
	return m_pOutFile != NULL;
}


const LONGLONG CBufferedFileWriter::GetWriteSize(void) const
{
	// 書き出し済みサイズを返す
	return m_llWriteSize;
}


const LONGLONG CBufferedFileWriter::GetWriteCount(void) const
{
	// 書き出し回数を返す
	return m_llWriteCount;
}


bool CBufferedFileWriter::SetBufferSize(DWORD Size)
{
	if (Size == 0)
		return false;
	m_BufferSize = Size;
	return true;
}


bool CBufferedFileWriter::Pause()
{
	CBlockLock Lock(&m_DecoderLock);

	if (!m_pOutFile || m_bPause)
		return false;
	m_bPause = true;
	return true;
}


bool CBufferedFileWriter::Resume()
{
	CBlockLock Lock(&m_DecoderLock);

	if (!m_pOutFile || !m_bPause)
		return false;
	m_bPause = false;
	return true;
}


LPCTSTR CBufferedFileWriter::GetFileName() const
{
	if (!m_pOutFile)
		return NULL;
	return m_pOutFile->GetFileName();
}


bool CBufferedFileWriter::EnableQueueing(bool bEnable)
{
	CBlockLock Lock(&m_DecoderLock);

	if (m_bEnableQueueing != bEnable) {
		if (!bEnable && !m_pOutFile)
			ClearQueue();
		m_bEnableQueueing = bEnable;
	}

	return true;
}


bool CBufferedFileWriter::IsQueueingEnabled() const
{
	return m_bEnableQueueing;
}


bool CBufferedFileWriter::SetQueueSize(SIZE_T BlockSize, SIZE_T MaxBlocks)
{
	if (BlockSize < 188 || MaxBlocks == 0)
		return false;

	BlockSize = BlockSize / 188 * 188;

	CBlockLock Lock(&m_QueueLock);

	if (m_QueueBlockSize != BlockSize) {
		std::deque<BufferInfo> Queue;

		m_WriteQueue.swap(Queue);
		size_t AliveBlocks = Queue.size() * BlockSize / m_QueueBlockSize;
		m_QueueBlockSize = BlockSize;
		m_MaxQueueSize = MaxBlocks;
		while (!Queue.empty()) {
			BufferInfo Info = Queue.front();
			if (Queue.size() <= AliveBlocks) {
				SIZE_T Pos = 0;
				do {
					PushData(Info.pData + Pos, min(Info.Size - Pos, BlockSize));
					Pos += BlockSize;
				} while (Pos < Info.Size);
			}
			delete [] Info.pData;
			Queue.pop_front();
		}
	} else if (m_MaxQueueSize != MaxBlocks) {
		m_MaxQueueSize = MaxBlocks;
		if (!m_pOutFile || m_bPause) {
			while (m_WriteQueue.size() > MaxBlocks) {
				delete [] m_WriteQueue.front().pData;
				m_WriteQueue.pop_front();
			}
		}
	}

	return true;
}


void CBufferedFileWriter::ClearQueue()
{
	CBlockLock Lock(&m_QueueLock);

	while (!m_WriteQueue.empty()) {
		delete [] m_WriteQueue.front().pData;
		m_WriteQueue.pop_front();
	}
}


bool CBufferedFileWriter::SetMaxPendingSize(SIZE_T Size)
{
	if (Size == 0)
		return false;

	CBlockLock Lock(&m_QueueLock);

	m_MaxPendingSize = Size;

	return true;
}


bool CBufferedFileWriter::PushData(const BYTE *pData, SIZE_T DataSize)
{
	CBlockLock Lock(&m_QueueLock);

	if (!pData || DataSize == 0 || DataSize > m_QueueBlockSize)
		return false;

	if (!m_WriteQueue.empty()) {
		BufferInfo &Last = m_WriteQueue.back();
		if (Last.Size < m_QueueBlockSize) {
			SIZE_T Size = min(m_QueueBlockSize - Last.Size, DataSize);
			::CopyMemory(Last.pData + Last.Size, pData, Size);
			Last.Size += Size;
			if (Size == DataSize)
				return true;
			DataSize -= Size;
			pData += Size;
		}
	}

	BufferInfo Info;

	if (m_WriteQueue.size() < m_MaxQueueSize
			|| (m_pOutFile && !m_bPause
				&& m_WriteQueue.size() * m_QueueBlockSize < m_MaxPendingSize)) {
		try {
			Info.pData = new BYTE[m_QueueBlockSize];
		} catch (...) {
			return false;
		}
	} else {
		Info = m_WriteQueue.front();
		m_WriteQueue.pop_front();
	}
	Info.Size = DataSize;
	::CopyMemory(Info.pData, pData, DataSize);
	m_WriteQueue.push_back(Info);

	return true;
}


unsigned int __stdcall CBufferedFileWriter::ThreadProc(LPVOID lpParameter)
{
	CBufferedFileWriter *pThis = static_cast<CBufferedFileWriter*>(lpParameter);
	DWORD Wait = 0;

	while (pThis->m_EndEvent.Wait(Wait) == WAIT_TIMEOUT) {
		if (pThis->m_bPause) {
			Wait = 100;
			continue;
		}

		pThis->m_QueueLock.Lock();
		if (!pThis->m_WriteQueue.empty()) {
			BufferInfo Info = pThis->m_WriteQueue.front();
			pThis->m_WriteQueue.pop_front();
			pThis->m_QueueLock.Unlock();
			pThis->m_FileLock.Lock();
			bool bResult = pThis->m_pOutFile->Write(Info.pData, (DWORD)Info.Size);
			pThis->m_FileLock.Unlock();
			delete [] Info.pData;
			if (bResult) {
				pThis->m_llWriteSize += Info.Size;
				pThis->m_llWriteCount++;
				Wait = 0;
			} else {
				if (!pThis->m_bWriteError) {
					pThis->SendDecoderEvent(EID_WRITE_ERROR);
					pThis->m_bWriteError = true;
				}
				Wait = 10;
			}
		} else {
			pThis->m_QueueLock.Unlock();
			Wait = 50;
		}
	};

	return 0;
}
