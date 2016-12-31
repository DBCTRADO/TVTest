#pragma once


#include <deque>
#include "MediaDecoder.h"


class CTsRecorder : public CMediaDecoder
{
public:
	enum
	{
		EVENT_WRITE_ERROR	// èëÇ´èoÇµÉGÉâÅ[
	};

	enum
	{
		OPEN_OVERWRITE	= 0x0001U	// è„èëÇ´
	};

	class ABSTRACT_CLASS_DECL CWriter : public CBonErrorHandler
	{
	public:
		virtual ~CWriter() {}
		virtual bool Open(LPCTSTR pszFileName, UINT Flags, ULONGLONG PreAllocationUnit) = 0;
		virtual bool ReOpen(LPCTSTR pszFileName, UINT Flags, ULONGLONG PreAllocationUnit) = 0;
		virtual void Close() = 0;
		virtual bool IsOpen() const = 0;
		virtual DWORD Write(const void *pBuffer, DWORD Size) = 0;
		virtual int GetFileName(LPTSTR pszFileName, int MaxFileName) const = 0;
		virtual ULONGLONG GetWriteSize() const = 0;
		virtual bool IsWriteSizeAvailable() const = 0;
		virtual bool SetPreAllocationUnit(ULONGLONG PreAllocationUnit) { return true; }
	};

	struct WriteStatistics
	{
		ULONGLONG InputSize;
		ULONGLONG OutputSize;
		ULONGLONG OutputCount;
		DWORD WriteErrorCount;
	};

	CTsRecorder(IEventHandler *pEventHandler = nullptr);
	virtual ~CTsRecorder();

// CMediaDecoder
	virtual void Reset(void) override;
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL) override;

// CTsRecorder
	bool OpenFile(LPCTSTR pszFileName, UINT Flags = 0);
	bool OpenFile(LPCTSTR pszPluginName, LPCTSTR pszFileName, UINT Flags = 0);
	bool RelayFile(LPCTSTR pszFileName, UINT Flags = 0);
	void CloseFile();
	bool IsOpen() const;
	int GetFileName(LPTSTR pszFileName, int MaxFileName) const;

	bool GetWriteStatistics(WriteStatistics *pStatistics) const;
	ULONGLONG GetInputSize() const;
	ULONGLONG GetOutputSize() const;
	ULONGLONG GetOutputCount() const;
	bool IsWriteSizeAvailable() const;
	ULONGLONG GetWriteSize() const;
	DWORD GetWriteErrorCount() const;

	bool Pause();
	bool Resume();

	bool SetBufferSize(DWORD Size);
	DWORD GetBufferSize() const { return m_BufferSize; }

	bool SetPreAllocationUnit(ULONGLONG PreAllocationUnit);
	ULONGLONG GetPreAllocationUnit() const { return m_PreAllocationUnit; }

	bool EnableQueueing(bool bEnable);
	bool IsQueueingEnabled() const;
	bool SetQueueSize(SIZE_T BlockSize, SIZE_T MaxBlocks);
	void ClearQueue();
	bool SetMaxPendingSize(SIZE_T Size);

protected:
	bool PushData(const BYTE *pData, SIZE_T DataSize);
	DWORD WriteData(const BYTE *pData, DWORD DataSize, DWORD *pWrittenSize);
	bool FlushBuffer();

	static unsigned int __stdcall WriteThread(LPVOID lpParameter);
	void WriteMain();

	CWriter *m_pWriter;
	CCriticalLock m_WriterLock;

	DWORD m_BufferSize;
	DWORD m_BufferUsed;
	BYTE *m_pWriteBuffer;

	ULONGLONG m_PreAllocationUnit;

	ULONGLONG m_InputSize;
	ULONGLONG m_OutputSize;
	ULONGLONG m_OutputCount;
	DWORD m_WriteErrorCount;

	bool m_bWriteError;
	volatile bool m_bPause;

	bool m_bEnableQueueing;
	SIZE_T m_QueueBlockSize;
	SIZE_T m_MaxQueueSize;
	SIZE_T m_MaxPendingSize;
	struct QueueBlock {
		BYTE *pData;
		SIZE_T Size;
		SIZE_T Offset;
	};
	std::deque<QueueBlock> m_WriteQueue;
	CCriticalLock m_QueueLock;
	HANDLE m_hThread;
	CLocalEvent m_EndEvent;
};
