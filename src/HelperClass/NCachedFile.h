// NCachedFile.h: CNCachedFile クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "NFile.h"


class CNCachedFile : public CNFile
{
public:
	CNCachedFile();
	virtual ~CNCachedFile();

	enum { DEFBUFFSIZE = 0x00200000UL };
	const bool Open(LPCTSTR lpszName, const UINT Flags);
	const bool Open(LPCTSTR lpszName, const UINT Flags, const DWORD dwBuffSize);
	const bool Close(void);

	const DWORD Read(void *pBuff, const DWORD dwLen);
	const DWORD Read(void *pBuff, const DWORD dwLen, const ULONGLONG llPos);

	const bool Write(const void *pBuff, const DWORD dwLen);
	const bool Write(const void *pBuff, const DWORD dwLen, const ULONGLONG llPos);
	const bool Flush(void);

	const ULONGLONG GetPos(void);
	const bool SetPos(const ULONGLONG llPos);

	//const bool ReserveBandwidth(DWORD BytesPerSecond);

protected:
	BOOL m_bIsWritable;

	BYTE *m_pBuff;
	DWORD m_BuffSize;
	DWORD m_DataSize;

	ULONGLONG m_FilePos;
	ULONGLONG m_CurPos;
};
