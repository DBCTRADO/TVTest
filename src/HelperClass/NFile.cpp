// NFile.cpp: CNFile �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NFile.h"
#include "StdUtil.h"
#include "../Common/DebugDef.h"

//////////////////////////////////////////////////////////////////////
// �\�z/����
//////////////////////////////////////////////////////////////////////

CNFile::CNFile()
	: m_hFile(INVALID_HANDLE_VALUE)
	, m_pszFileName(NULL)
	, m_LastError(ERROR_SUCCESS)
{
}


CNFile::~CNFile()
{
	Close();
}


const bool CNFile::Open(LPCTSTR lpszName, const UINT Flags)
{
	if (m_hFile != INVALID_HANDLE_VALUE) {
		m_LastError = ERROR_BUSY;	// �u�v���������\�[�X�͎g�p���ł��B�v
		return false;
	}

	if (lpszName == NULL) {
		m_LastError = ERROR_INVALID_PARAMETER;	// �u�p�����[�^������������܂���B�v
		return false;
	}

	// �t�@�C���A�N�Z�X�����\�z
	DWORD dwAccess = 0x00000000UL;

	if (Flags & CNF_READ)
		dwAccess |= GENERIC_READ;
	if (Flags & CNF_WRITE)
		dwAccess |= GENERIC_WRITE;

	if (!dwAccess) {
		m_LastError = ERROR_INVALID_PARAMETER;
		return false;
	}

	// �t�@�C�����L�����\�z
	DWORD dwShare = 0x00000000UL;

	if (Flags & CNF_SHAREREAD)
		dwShare |= FILE_SHARE_READ;
	if (Flags & CNF_SHAREWRITE)
		dwShare |= FILE_SHARE_WRITE;

	// �t�@�C���쐬�����\�z
	DWORD dwCreate;

	if (Flags & CNF_NEW)
		dwCreate = CREATE_ALWAYS;
	else
		dwCreate = OPEN_EXISTING;

	// �t�@�C������/�t���O�\�z
	DWORD dwAttributes = FILE_ATTRIBUTE_NORMAL;

	if (Flags & CNF_SEQUENTIALREAD)
		dwAttributes |= FILE_FLAG_SEQUENTIAL_SCAN;
	if (Flags & CNF_RANDOMACCESS)
		dwAttributes |= FILE_FLAG_RANDOM_ACCESS;

	// �����p�X�Ή�
	LPTSTR pszPath = NULL;
	const int PathLength = ::lstrlen(lpszName);
	if (PathLength >= MAX_PATH && ::StrCmpN(lpszName, TEXT("\\\\?"), 3) != 0) {
		if (lpszName[0] == _T('\\') && lpszName[1] == _T('\\')) {
			pszPath = new TCHAR[6 + PathLength + 1];
			::lstrcpy(pszPath, TEXT("\\\\?\\UNC"));
			::lstrcat(pszPath, &lpszName[1]);
		} else {
			pszPath = new TCHAR[4 + PathLength + 1];
			::lstrcpy(pszPath, TEXT("\\\\?\\"));
			::lstrcat(pszPath, lpszName);
		}
	}

	// �t�@�C���I�[�v��
	TRACE(TEXT("CNFile::Open() : Open file \"%s\"\n"), pszPath ? pszPath : lpszName);
	m_hFile = ::CreateFile(pszPath ? pszPath : lpszName,
						   dwAccess, dwShare, NULL, dwCreate,
						   dwAttributes, NULL);
	delete [] pszPath;
	if (m_hFile == INVALID_HANDLE_VALUE) {
		m_LastError = ::GetLastError();
		return false;
	}

#if _WIN32_WINNT >= 0x0600
	// I/O�D��x�̐ݒ�
	if (Flags & (CNF_PRIORITY_LOW | CNF_PRIORITY_IDLE)) {
		typedef BOOL (WINAPI *SetFileInformationByHandleFunc)(HANDLE hFile,
			FILE_INFO_BY_HANDLE_CLASS FileInformationClass, LPVOID lpFileInformation, DWORD dwBufferSize);
		HMODULE hKernel = ::GetModuleHandle(TEXT("kernel32.dll"));
		SetFileInformationByHandleFunc pSetFileInformationByHandle =
			(SetFileInformationByHandleFunc)::GetProcAddress(hKernel, "SetFileInformationByHandle");
		if (pSetFileInformationByHandle) {
			alignas(8) FILE_IO_PRIORITY_HINT_INFO PriorityHint;
			PriorityHint.PriorityHint = (Flags & CNF_PRIORITY_IDLE) ? IoPriorityHintVeryLow : IoPriorityHintLow;
			TRACE(TEXT("Set file I/O priority hint %d\n"), (int)PriorityHint.PriorityHint);
			if (!pSetFileInformationByHandle(m_hFile, FileIoPriorityHintInfo, &PriorityHint, sizeof(PriorityHint))) {
				TRACE(TEXT("�t�@�C��I/O�D��x�̐ݒ莸�s (Error 0x%x)\n"), ::GetLastError());
			}
		}
	}
#endif

	m_pszFileName = StdUtil::strdup(lpszName);

	m_LastError = ERROR_SUCCESS;

	return true;
}


const bool CNFile::Close(void)
{
	// �t�@�C���N���[�Y

	m_LastError = ERROR_SUCCESS;

	if (m_hFile != INVALID_HANDLE_VALUE) {
		if (!::CloseHandle(m_hFile))
			m_LastError = ::GetLastError();
		m_hFile = INVALID_HANDLE_VALUE;
		delete [] m_pszFileName;
		m_pszFileName = NULL;
	}

	return m_LastError == ERROR_SUCCESS;
}


const bool CNFile::IsOpen() const
{
	return m_hFile != INVALID_HANDLE_VALUE;
}


const DWORD CNFile::Read(void *pBuff, const DWORD dwLen)
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return 0;
	}

	if (pBuff == NULL || dwLen == 0) {
		m_LastError = ERROR_INVALID_PARAMETER;
		return 0;
	}

	// �t�@�C�����[�h
	DWORD dwRead = 0;

	if (!::ReadFile(m_hFile, pBuff, dwLen, &dwRead, NULL)) {
		m_LastError = ::GetLastError();
		return 0;
	}

	m_LastError = ERROR_SUCCESS;

	return dwRead;
}


const DWORD CNFile::Read(void *pBuff, const DWORD dwLen, const ULONGLONG llPos)
{
	// �t�@�C�����[�h
	if (!SetPos(llPos))
		return 0;

	return Read(pBuff, dwLen);
}


const bool CNFile::Write(const void *pBuff, const DWORD dwLen)
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return false;
	}

	if (pBuff == NULL || dwLen == 0) {
		m_LastError = ERROR_INVALID_PARAMETER;
		return false;
	}

	// �t�@�C�����C�g
	DWORD dwWritten = 0UL;

	if (!::WriteFile(m_hFile, pBuff, dwLen, &dwWritten, NULL)) {
		m_LastError = ::GetLastError();
		return false;
	}
	if (dwWritten != dwLen) {
		m_LastError = ERROR_WRITE_FAULT;
		return false;
	}

	m_LastError = ERROR_SUCCESS;

	return true;
}


const bool CNFile::Write(const void *pBuff, const DWORD dwLen, const ULONGLONG llPos)
{
	// �t�@�C���V�[�N
	if (!SetPos(llPos))
		return false;
	return Write(pBuff, dwLen);
}


const bool CNFile::Flush(void)
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


const ULONGLONG CNFile::GetSize(void)
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return 0;
	}

	// �t�@�C���T�C�Y�擾
	LARGE_INTEGER FileSize;

	if (!::GetFileSizeEx(m_hFile, &FileSize)) {
		m_LastError = ::GetLastError();
		return 0;
	}

	m_LastError = ERROR_SUCCESS;

	return FileSize.QuadPart;
}


const ULONGLONG CNFile::GetPos(void)
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return 0;
	}

	// �|�W�V�����擾
	static const LARGE_INTEGER DistanceToMove = {0, 0};
	LARGE_INTEGER NewPos;

	if (!::SetFilePointerEx(m_hFile, DistanceToMove, &NewPos, FILE_CURRENT)) {
		m_LastError = ::GetLastError();
		return 0;
	}

	m_LastError = ERROR_SUCCESS;

	return NewPos.QuadPart;
}


const bool CNFile::SetPos(const ULONGLONG llPos)
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return false;
	}

	// �t�@�C���V�[�N
	LARGE_INTEGER DistanceToMove;
	DistanceToMove.QuadPart = llPos;
	if (!::SetFilePointerEx(m_hFile, DistanceToMove, NULL, FILE_BEGIN)) {
		m_LastError = ::GetLastError();
		return false;
	}

	m_LastError = ERROR_SUCCESS;

	return true;
}


bool CNFile::GetTime(FILETIME *pCreationTime, FILETIME *pLastAccessTime, FILETIME *pLastWriteTime) const
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return false;
	return ::GetFileTime(m_hFile, pCreationTime, pLastAccessTime, pLastWriteTime) != FALSE;
}


LPCTSTR CNFile::GetFileName() const
{
	return m_pszFileName;
}


DWORD CNFile::GetLastError(void) const
{
	return m_LastError;
}


DWORD CNFile::GetLastErrorMessage(LPTSTR pszMessage, const DWORD MaxLength) const
{
	if (pszMessage == NULL || MaxLength == 0)
		return 0;

	DWORD Length = ::FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
		m_LastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		pszMessage, MaxLength, NULL);
	if (Length == 0)
		pszMessage[0] = _T('\0');
	return Length;
}
