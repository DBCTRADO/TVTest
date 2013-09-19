// NFile.h: CNFile クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


class CNFile
{
public:
	enum {
		CNF_READ			= 0x0001U,
		CNF_WRITE			= 0x0002U,
		CNF_NEW				= 0x0004U,
		CNF_SHAREREAD		= 0x0008U,
		CNF_SHAREWRITE		= 0x0010U,
		CNF_SEQUENTIALREAD	= 0x0020U,
		CNF_RANDOMACCESS	= 0x0040U,
		CNF_PRIORITY_LOW	= 0x0100U,
		CNF_PRIORITY_IDLE	= 0x0200U
	};

	CNFile();
	virtual ~CNFile();

	virtual const bool Open(LPCTSTR lpszName, const UINT Flags);
	virtual const bool Close(void);
	virtual const bool IsOpen() const;

	virtual const DWORD Read(void *pBuff, const DWORD dwLen);
	virtual const DWORD Read(void *pBuff, const DWORD dwLen, const ULONGLONG llPos);

	virtual const bool Write(const void *pBuff, const DWORD dwLen);
	virtual const bool Write(const void *pBuff, const DWORD dwLen, const ULONGLONG llPos);
	virtual const bool Flush(void);

	virtual const ULONGLONG GetSize(void);
	virtual const ULONGLONG GetPos(void);
	virtual const bool SetPos(const ULONGLONG llPos);

	bool GetTime(FILETIME *pCreationTime, FILETIME *pLastAccessTime = NULL, FILETIME *pLastWriteTime = NULL) const;
	LPCTSTR GetFileName(void) const;

	DWORD GetLastError(void) const;
	DWORD GetLastErrorMessage(LPTSTR pszMessage, const DWORD MaxLength) const;

protected:
	HANDLE m_hFile;
	LPTSTR m_pszFileName;
	DWORD m_LastError;
};
