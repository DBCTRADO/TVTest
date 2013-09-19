#pragma once


#include <deque>
#include "MediaDecoder.h"
#include "NCachedFile.h"


// キューバッファ付きファイル出力クラス
class CBufferedFileWriter : public CMediaDecoder
{
public:
	enum EVENTID
	{
		EID_WRITE_ERROR	// 書き出しエラー
	};

	CBufferedFileWriter(IEventHandler *pEventHandler = NULL);
	virtual ~CBufferedFileWriter();

// IMediaDecoder
	virtual void Reset(void);
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CBufferedFileWriter
	const bool OpenFile(LPCTSTR pszFileName, UINT Flags = 0);
	const bool RelayFile(LPCTSTR pszFileName, UINT Flags = 0);
	void CloseFile(void);
	const bool IsOpen() const;

	const LONGLONG GetWriteSize(void) const;
	const LONGLONG GetWriteCount(void) const;

	bool SetBufferSize(DWORD Size);
	DWORD GetBufferSize() const { return m_BufferSize; }
	bool Pause();
	bool Resume();
	LPCTSTR GetFileName() const;

	bool EnableQueueing(bool bEnable);
	bool IsQueueingEnabled() const;
	bool SetQueueSize(SIZE_T BlockSize, SIZE_T MaxBlocks);
	void ClearQueue();
	bool SetMaxPendingSize(SIZE_T Size);

protected:
	CNCachedFile *CreateNewFile(LPCTSTR pszFileName, UINT Flags);
	bool PushData(const BYTE *pData, SIZE_T DataSize);
	static unsigned int __stdcall ThreadProc(LPVOID lpParameter);

	CNCachedFile *m_pOutFile;
	CCriticalLock m_FileLock;
	DWORD m_BufferSize;

	LONGLONG m_llWriteSize;
	LONGLONG m_llWriteCount;

	bool m_bWriteError;
	volatile bool m_bPause;

	bool m_bEnableQueueing;
	SIZE_T m_QueueBlockSize;
	SIZE_T m_MaxQueueSize;
	SIZE_T m_MaxPendingSize;
	struct BufferInfo {
		BYTE *pData;
		SIZE_T Size;
	};
	std::deque<BufferInfo> m_WriteQueue;
	CCriticalLock m_QueueLock;
	HANDLE m_hThread;
	CLocalEvent m_EndEvent;
};
