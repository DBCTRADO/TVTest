// BasicFile.cpp: CBasicFile クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "BasicFile.h"
#include "StdUtil.h"
#include "../Common/DebugDef.h"


static const LARGE_INTEGER liZero = {0, 0};


//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CBasicFile::CBasicFile()
	: m_hFile(INVALID_HANDLE_VALUE)
	, m_pszFileName(nullptr)
	, m_LastError(ERROR_SUCCESS)

	, m_PreAllocationUnit(0)
	, m_AllocatedSize(0)
	, m_bAllocationFailed(false)
{
}


CBasicFile::~CBasicFile()
{
	Close();
}


bool CBasicFile::Open(LPCTSTR pszName, const UINT Flags)
{
	if (m_hFile != INVALID_HANDLE_VALUE) {
		m_LastError = ERROR_BUSY;	// 「要求したリソースは使用中です。」
		return false;
	}

	if (pszName == nullptr) {
		m_LastError = ERROR_INVALID_PARAMETER;	// 「パラメータが正しくありません。」
		return false;
	}

	DWORD Access = 0;
	if (Flags & OPEN_READ)
		Access |= GENERIC_READ;
	if (Flags & OPEN_WRITE)
		Access |= GENERIC_WRITE;
	if (!Access) {
		m_LastError = ERROR_INVALID_PARAMETER;
		return false;
	}

	DWORD Share = 0;
	if (Flags & OPEN_SHAREREAD)
		Share |= FILE_SHARE_READ;
	if (Flags & OPEN_SHAREWRITE)
		Share |= FILE_SHARE_WRITE;

	DWORD Create;
	if (Flags & OPEN_NEW)
		Create = CREATE_ALWAYS;
	else
		Create = OPEN_EXISTING;

	DWORD Attributes = FILE_ATTRIBUTE_NORMAL;
	if (Flags & OPEN_SEQUENTIALREAD)
		Attributes |= FILE_FLAG_SEQUENTIAL_SCAN;
	if (Flags & OPEN_RANDOMACCESS)
		Attributes |= FILE_FLAG_RANDOM_ACCESS;

	// 長いパス対応
	LPTSTR pszPath = nullptr;
	const int PathLength = ::lstrlen(pszName);
	if (PathLength >= MAX_PATH && ::StrCmpN(pszName, TEXT("\\\\?"), 3) != 0) {
		if (pszName[0] == _T('\\') && pszName[1] == _T('\\')) {
			pszPath = new TCHAR[6 + PathLength + 1];
			::lstrcpy(pszPath, TEXT("\\\\?\\UNC"));
			::lstrcat(pszPath, &pszName[1]);
		} else {
			pszPath = new TCHAR[4 + PathLength + 1];
			::lstrcpy(pszPath, TEXT("\\\\?\\"));
			::lstrcat(pszPath, pszName);
		}
	}

	// ファイルオープン
	TRACE(TEXT("CBasicFile::Open() : Open file \"%s\"\n"), pszPath ? pszPath : pszName);
	m_hFile = ::CreateFile(pszPath ? pszPath : pszName,
						   Access, Share, nullptr, Create, Attributes, nullptr);
	delete [] pszPath;
	if (m_hFile == INVALID_HANDLE_VALUE) {
		m_LastError = ::GetLastError();
		return false;
	}

#if _WIN32_WINNT >= 0x0600
	// I/O優先度の設定
	if (Flags & (OPEN_PRIORITY_LOW | OPEN_PRIORITY_IDLE)) {
		alignas(8) FILE_IO_PRIORITY_HINT_INFO PriorityHint;
		PriorityHint.PriorityHint = (Flags & OPEN_PRIORITY_IDLE) ? IoPriorityHintVeryLow : IoPriorityHintLow;
		TRACE(TEXT("Set file I/O priority hint %d\n"), (int)PriorityHint.PriorityHint);
#ifdef WIN_XP_SUPPORT
		auto pSetFileInformationByHandle = reinterpret_cast<decltype(SetFileInformationByHandle)*>(
			::GetProcAddress(::GetModuleHandle(TEXT("kernel32.dll")), "SetFileInformationByHandle"));
		if (pSetFileInformationByHandle) {
			if (!pSetFileInformationByHandle(m_hFile, FileIoPriorityHintInfo, &PriorityHint, sizeof(PriorityHint))) {
				TRACE(TEXT("ファイルI/O優先度の設定失敗 (Error %#x)\n"), ::GetLastError());
			}
		}
#else
		if (!::SetFileInformationByHandle(m_hFile, FileIoPriorityHintInfo, &PriorityHint, sizeof(PriorityHint))) {
			TRACE(TEXT("ファイルI/O優先度の設定失敗 (Error %#x)\n"), ::GetLastError());
		}
#endif
	}
#endif

	m_pszFileName = StdUtil::strdup(pszName);

	m_AllocatedSize = 0;
	m_bAllocationFailed = false;

	m_LastError = ERROR_SUCCESS;

	return true;
}


bool CBasicFile::Close()
{
	// ファイルクローズ

	m_LastError = ERROR_SUCCESS;

	if (m_hFile != INVALID_HANDLE_VALUE) {
		if (m_AllocatedSize)
			::SetEndOfFile(m_hFile);

		if (!::CloseHandle(m_hFile))
			m_LastError = ::GetLastError();
		m_hFile = INVALID_HANDLE_VALUE;
	}

	if (m_pszFileName) {
		delete [] m_pszFileName;
		m_pszFileName = nullptr;
	}

	return m_LastError == ERROR_SUCCESS;
}


bool CBasicFile::IsOpen() const
{
	return m_hFile != INVALID_HANDLE_VALUE;
}


DWORD CBasicFile::Read(void *pBuff, const DWORD Size)
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return 0;
	}

	if (pBuff == nullptr || Size == 0) {
		m_LastError = ERROR_INVALID_PARAMETER;
		return 0;
	}

	// ファイル読み込み
	DWORD Read = 0;

	if (!::ReadFile(m_hFile, pBuff, Size, &Read, nullptr)) {
		m_LastError = ::GetLastError();
		return 0;
	}

	m_LastError = ERROR_SUCCESS;

	return Read;
}


DWORD CBasicFile::Write(const void *pBuff, const DWORD Size)
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return 0;
	}

	if (pBuff == nullptr || Size == 0) {
		m_LastError = ERROR_INVALID_PARAMETER;
		return 0;
	}

	if (m_PreAllocationUnit && !m_bAllocationFailed) {
		LARGE_INTEGER CurPos, FileSize;

		if (::SetFilePointerEx(m_hFile, liZero, &CurPos, FILE_CURRENT)
				&& ::GetFileSizeEx(m_hFile, &FileSize)
				&& CurPos.QuadPart + (LONGLONG)Size > FileSize.QuadPart) {
			LONGLONG ExtendSize = ((ULONGLONG)Size + m_PreAllocationUnit - 1) / m_PreAllocationUnit * m_PreAllocationUnit;
			TRACE(TEXT("ファイルサイズを伸長します: %lld + %lld bytes (%s)\n"),
				  FileSize.QuadPart, ExtendSize, m_pszFileName);
			FileSize.QuadPart += ExtendSize;
			if (::SetFilePointerEx(m_hFile, FileSize, nullptr, FILE_BEGIN)) {
				if (::SetEndOfFile(m_hFile)) {
					m_AllocatedSize = FileSize.QuadPart;
				} else {
					TRACE(TEXT("ファイルサイズを伸長できません。(Error %#x)\n"),::GetLastError());
					m_bAllocationFailed = true;
				}
				::SetFilePointerEx(m_hFile, CurPos, nullptr, FILE_BEGIN);
			}
		}
	}

	// ファイル書き出し
	DWORD Written = 0;

	if (!::WriteFile(m_hFile, pBuff, Size, &Written, nullptr)) {
		m_LastError = ::GetLastError();
		return Written;
	}

	m_LastError = ERROR_SUCCESS;

	return Written;
}


bool CBasicFile::Flush()
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return false;
	}

	if (!::FlushFileBuffers(m_hFile)) {
		m_LastError = ::GetLastError();
		return false;
	}

	m_LastError = ERROR_SUCCESS;

	return true;
}


ULONGLONG CBasicFile::GetSize()
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return 0;
	}

	// ファイルサイズ取得
	LARGE_INTEGER FileSize;

	if (!::GetFileSizeEx(m_hFile, &FileSize)) {
		m_LastError = ::GetLastError();
		return 0;
	}

	m_LastError = ERROR_SUCCESS;

	return FileSize.QuadPart;
}


ULONGLONG CBasicFile::GetPos()
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return 0;
	}

	// ポジション取得
	LARGE_INTEGER Pos;

	if (!::SetFilePointerEx(m_hFile, liZero, &Pos, FILE_CURRENT)) {
		m_LastError = ::GetLastError();
		return 0;
	}

	m_LastError = ERROR_SUCCESS;

	return Pos.QuadPart;
}


bool CBasicFile::SetPos(const ULONGLONG Pos)
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return false;
	}

	// ファイルシーク
	LARGE_INTEGER DistanceToMove;
	DistanceToMove.QuadPart = Pos;
	if (!::SetFilePointerEx(m_hFile, DistanceToMove, nullptr, FILE_BEGIN)) {
		m_LastError = ::GetLastError();
		return false;
	}

	m_LastError = ERROR_SUCCESS;

	return true;
}


bool CBasicFile::GetTime(FILETIME *pCreationTime,
						 FILETIME *pLastAccessTime,
						 FILETIME *pLastWriteTime) const
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return false;
	return ::GetFileTime(m_hFile, pCreationTime, pLastAccessTime, pLastWriteTime) != FALSE;
}


LPCTSTR CBasicFile::GetFileName() const
{
	return m_pszFileName;
}


DWORD CBasicFile::GetLastError() const
{
	return m_LastError;
}


DWORD CBasicFile::GetLastErrorMessage(LPTSTR pszMessage, const DWORD MaxLength) const
{
	if (pszMessage == nullptr || MaxLength == 0)
		return 0;

	DWORD Length = ::FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
		m_LastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		pszMessage, MaxLength, nullptr);
	if (Length == 0)
		pszMessage[0] = _T('\0');
	return Length;
}


bool CBasicFile::SetPreAllocationUnit(const ULONGLONG PreAllocationUnit)
{
	m_PreAllocationUnit = PreAllocationUnit;

	return true;
}


ULONGLONG CBasicFile::GetPreAllocationUnit() const
{
	return m_PreAllocationUnit;
}


ULONGLONG CBasicFile::GetPreAllocatedSpace()
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return 0;
	}

	if (!m_AllocatedSize) {
		m_LastError = ERROR_SUCCESS;
		return 0;
	}

	LARGE_INTEGER Pos;
	if (!::SetFilePointerEx(m_hFile, liZero, &Pos, FILE_CURRENT)) {
		m_LastError = ::GetLastError();
		return 0;
	}

	m_LastError = ERROR_SUCCESS;

	if (Pos.QuadPart >= (LONGLONG)m_AllocatedSize)
		return 0;

	return m_AllocatedSize - Pos.QuadPart;
}
