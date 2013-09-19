// FileWriter.h: CFileWriter クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "MediaDecoder.h"
#include "NCachedFile.h"


/////////////////////////////////////////////////////////////////////////////
// 汎用ファイル出力(CMediaDataをそのままファイルに書き出す)
/////////////////////////////////////////////////////////////////////////////
// Input	#0	: CMediaData		書き込みデータ
/////////////////////////////////////////////////////////////////////////////

class CFileWriter : public CMediaDecoder
{
public:
	enum EVENTID
	{
		EID_WRITE_ERROR
	};

	CFileWriter(IEventHandler *pEventHandler = NULL);
	virtual ~CFileWriter();

// IMediaDecoder
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CFileWriter
	// Modified by HDUSTestの中の人
	// フラグを指定できるようにした
	// const bool OpenFile(LPCTSTR lpszFileName);
	const bool OpenFile(LPCTSTR lpszFileName, BYTE bFlags = 0);
	void CloseFile(void);
	const bool IsOpen() const;

	const LONGLONG GetWriteSize(void) const;
	const LONGLONG GetWriteCount(void) const;

	// Append by HDUSTestの中の人
	bool SetBufferSize(DWORD Size);
	DWORD GetBufferSize() const { return m_BufferSize; }
	bool Pause();
	bool Resume();
	LPCTSTR GetFileName() const;
protected:
	CNCachedFile m_OutFile;
	DWORD m_BufferSize;

	LONGLONG m_llWriteSize;
	LONGLONG m_llWriteCount;

	bool m_bWriteError;
	bool m_bPause;
};
