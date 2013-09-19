#ifndef MEDIA_BUFFER_H
#define MEDIA_BUFFER_H


#include "MediaDecoder.h"
#include "TsStream.h"
#include "TsUtilClass.h"


class CMediaBuffer : public CMediaDecoder
{
	DWORD m_BufferLength;
	bool m_bEnableBuffering;
	bool m_bFileMode;
	DWORD m_PoolPercentage;
	DWORD m_TimingAdjustment;
	BYTE *m_pBuffer;
	CTsPacket m_OutputData;
	DWORD m_FirstBuffer;
	DWORD m_LastBuffer;
	DWORD m_UsedCount;
	bool m_bBuffering;
	CCriticalLock m_Lock;
	HANDLE m_hOutputThread;
	CLocalEvent m_BreakEvent;
	CLocalEvent m_CompleteEvent;
	volatile enum {
		SIGNAL_KILL,
		SIGNAL_RESET
	} m_SignalType;
#ifdef _DEBUG
	DWORD m_InputCount;
	DWORD m_OutputCount;
#endif
	static unsigned int __stdcall OutputThread(LPVOID lpParameter);

public:
	CMediaBuffer(IEventHandler *pEventHandler = NULL);
	virtual ~CMediaBuffer();
// CMediaDecoder
	virtual void Reset(void);
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);
// CMediaBuffer
	bool Play();
	bool Stop();
	bool EnableBuffering(bool bBuffering);
	void ResetBuffer();
	bool SetBufferLength(DWORD BufferLength);
	DWORD GetBufferLength() const { return m_BufferLength; }
	bool SetFileMode(bool bFileMode);
	bool GetFileMode() const { return m_bFileMode; }
	bool SetPoolPercentage(int Percentage);
	int GetPoolPercentage() const { return m_PoolPercentage; }
	DWORD GetUsedBufferCount();
};


#endif
