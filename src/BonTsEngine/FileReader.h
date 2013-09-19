// FileReader.h: CFileReader クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "MediaDecoder.h"
#include "NCachedFile.h"


/////////////////////////////////////////////////////////////////////////////
// 汎用ファイル入力(ファイルから読み込んだデータをデコーダグラフに入力する)
/////////////////////////////////////////////////////////////////////////////
// Output	#0	: CMediaData		読み込みデータ
/////////////////////////////////////////////////////////////////////////////


#define DEF_READSIZE	0x00200000UL		// 2MB


class CFileReader : public CMediaDecoder  
{
public:
	enum EVENTID
	{
		EID_READ_ASYNC_START,		// 非同期リード開始
		EID_READ_ASYNC_END,			// 非同期リード終了
		EID_READ_ASYNC_PREREAD,		// 非同期リード前
		EID_READ_ASYNC_POSTREAD		// 非同期リード後
	};

	CFileReader(IEventHandler *pEventHandler = NULL);
	virtual ~CFileReader();

// IMediaDecoder
	//virtual void Reset(void);
	const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex);

// CFileReader
	const bool OpenFile(LPCTSTR lpszFileName);
	void CloseFile(void);
	const bool IsOpen() const;

	const DWORD ReadSync(const DWORD dwReadSize = DEF_READSIZE);
	const DWORD ReadSync(const DWORD dwReadSize, const ULONGLONG llReadPos);

	const bool StartReadAnsync(const DWORD dwReadSize = DEF_READSIZE, const ULONGLONG llReadPos = 0ULL);
	void StopReadAnsync(void);
	const bool IsAnsyncReadBusy(void) const;

	const ULONGLONG GetReadPos(void);
	const ULONGLONG GetFileSize(void);
	const bool SetReadPos(const ULONGLONG llReadPos);

protected:
//	CNFile m_InFile;
	CNCachedFile m_InFile;
	CMediaData m_ReadBuffer;

	HANDLE m_hReadAnsyncThread;
	DWORD m_dwReadAnsyncThreadID;

	bool m_bKillSignal;
	bool m_bIsAnsyncRead;

private:
	static DWORD WINAPI ReadAnsyncThread(LPVOID pParam);
};
