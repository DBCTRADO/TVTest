// BasicFile.h: CBasicFile クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


class CBasicFile
{
public:
	enum {
		OPEN_READ			= 0x0001U,
		OPEN_WRITE			= 0x0002U,
		OPEN_NEW			= 0x0004U,
		OPEN_SHAREREAD		= 0x0008U,
		OPEN_SHAREWRITE		= 0x0010U,
		OPEN_SEQUENTIALREAD	= 0x0020U,
		OPEN_RANDOMACCESS	= 0x0040U,
		OPEN_PRIORITY_LOW	= 0x0100U,
		OPEN_PRIORITY_IDLE	= 0x0200U
	};

	CBasicFile();
	~CBasicFile();

	bool Open(LPCTSTR pszName, const UINT Flags);
	bool Close();
	bool IsOpen() const;

	DWORD Read(void *pBuff, const DWORD Size);
	DWORD Write(const void *pBuff, const DWORD Size);
	bool Flush();

	ULONGLONG GetSize();
	ULONGLONG GetPos();
	bool SetPos(const ULONGLONG Pos);

	bool GetTime(FILETIME *pCreationTime,
				 FILETIME *pLastAccessTime = nullptr,
				 FILETIME *pLastWriteTime = nullptr) const;
	LPCTSTR GetFileName() const;

	DWORD GetLastError() const;
	DWORD GetLastErrorMessage(LPTSTR pszMessage, const DWORD MaxLength) const;

	bool SetPreAllocationUnit(const ULONGLONG PreAllocationUnit);
	ULONGLONG GetPreAllocationUnit() const;
	ULONGLONG GetPreAllocatedSpace();

protected:
	HANDLE m_hFile;
	LPTSTR m_pszFileName;
	DWORD m_LastError;

	ULONGLONG m_PreAllocationUnit;
	ULONGLONG m_AllocatedSize;
	bool m_bAllocationFailed;
};
