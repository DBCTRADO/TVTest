// NCachedFile.cpp: CNCachedFile �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <new>
#include "NCachedFile.h"
#include "../Common/DebugDef.h"

//////////////////////////////////////////////////////////////////////
// �\�z/����
//////////////////////////////////////////////////////////////////////

CNCachedFile::CNCachedFile()
	: CNFile()
	, m_bIsWritable(false)
	, m_pBuff(NULL)
	, m_BuffSize(0UL)
	, m_DataSize(0UL)
	, m_FilePos(0)
	, m_CurPos(0)
{
}


CNCachedFile::~CNCachedFile()
{
	Close();
}


const bool CNCachedFile::Open(LPCTSTR lpszName, const UINT Flags)
{
	return Open(lpszName, Flags, DEFBUFFSIZE);
}


const bool CNCachedFile::Open(LPCTSTR lpszName, const UINT Flags, const DWORD dwBuffSize)
{
	if (IsOpen()) {
		m_LastError = ERROR_BUSY;	// �u�v���������\�[�X�͎g�p���ł��B�v
		return false;
	}

	if ((Flags & CNFile::CNF_WRITE) && !(Flags & CNFile::CNF_READ)) {
		// ���C�g�L���b�V���L��
		m_bIsWritable = true;
	} else if (!(Flags & CNFile::CNF_WRITE) && (Flags & CNFile::CNF_READ) && !(Flags & CNFile::CNF_NEW)) {
		// ���[�h�L���b�V���L��
		m_bIsWritable = false;
	} else {
		// �t���O�̑g�ݍ��킹����Ή�
		m_LastError = ERROR_INVALID_PARAMETER;
		return false;
	}

	if (dwBuffSize==0) {
		m_LastError = ERROR_INVALID_PARAMETER;
		return false;
	}

	// �t�@�C���I�[�v��
	if (!CNFile::Open(lpszName, Flags))
		return false;

	// �o�b�t�@�m��
	m_BuffSize = dwBuffSize;
	if (!m_bIsWritable && (ULONGLONG)dwBuffSize > GetSize()) {
		// �ǂݍ��݃o�b�t�@
		m_BuffSize = (DWORD)GetSize();
	}
	try {
		m_pBuff = new BYTE [m_BuffSize];
	} catch (std::bad_alloc&) {
		Close();
		m_LastError = ERROR_OUTOFMEMORY;
		return false;
	}

	// �o�b�t�@������
	m_DataSize = 0;
	m_FilePos = 0;
	m_CurPos = 0;

	m_LastError = ERROR_SUCCESS;

	return true;
}


const bool CNCachedFile::Close(void)
{
	m_LastError = ERROR_SUCCESS;

	if (IsOpen()) {
		// ���������݃f�[�^�t���b�V��
		Flush();
		DWORD FlushError = m_LastError;

		CNFile::Close();
		if (FlushError != ERROR_SUCCESS)
			m_LastError = FlushError;

		if (m_pBuff) {
			delete [] m_pBuff;
			m_pBuff = NULL;
		}
	}

	return m_LastError == ERROR_SUCCESS;
}


const DWORD CNCachedFile::Read(void *pBuff, const DWORD dwLen)
{
	// �G���[����
	if (m_bIsWritable) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return 0;
	}

	return CNFile::Read(pBuff, dwLen);
}


const DWORD CNCachedFile::Read(void *pBuff, const DWORD dwLen, const ULONGLONG llPos)
{
	if (m_bIsWritable) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return 0;
	}

	// �t�@�C���V�[�N
	if (!SetPos(llPos))
		return 0;

	return Read(pBuff, dwLen);
}


const bool CNCachedFile::Write(const void *pBuff, const DWORD dwLen)
{
	// �t�@�C����������
	if (!m_bIsWritable) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return false;
	}

	if (pBuff == NULL || dwLen == 0) {
		m_LastError = ERROR_INVALID_PARAMETER;
		return false;
	}

	// �o�b�t�@�����O����
	if ((m_BuffSize - m_DataSize) < dwLen) {
		// �o�b�t�@�s��
		if (!Flush())
			return false;
	}

	// �o�b�t�@�����O
	if ((m_BuffSize - m_DataSize) >= dwLen) {
		::CopyMemory(m_pBuff + m_DataSize, pBuff, dwLen);
		m_DataSize += dwLen;
		if (m_DataSize == m_BuffSize) {
			if (!Flush())
				return false;
		}
	} else {
		if (!CNFile::SetPos(m_CurPos) || !CNFile::Write(pBuff, dwLen))
			return false;
		m_FilePos += dwLen;
	}

	// �t�@�C���|�W�V�����X�V
	m_CurPos += (ULONGLONG)dwLen;

	m_LastError = ERROR_SUCCESS;

	return true;
}


const bool CNCachedFile::Write(const void *pBuff, const DWORD dwLen, const ULONGLONG llPos)
{
	// �t�@�C����������
	if (!m_bIsWritable) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return false;
	}

	// �t�@�C���V�[�N
	if (!SetPos(llPos))
		return false;

	return Write(pBuff, dwLen);
}


const ULONGLONG CNCachedFile::GetPos(void)
{
	// �_���I�ȃt�@�C���|�W�V������Ԃ�
	return m_bIsWritable ? m_CurPos : CNFile::GetPos();
}


const bool CNCachedFile::SetPos(const ULONGLONG llPos)
{
	// �V�[�N�O�Ƀt���b�V��
	if (!Flush())
		return false;

	// �V�[�N
	if (!CNFile::SetPos(llPos))
		return false;

	// �t�@�C���|�W�V�����X�V
	m_FilePos = llPos;
	m_CurPos = llPos;

	return true;
}


const bool CNCachedFile::Flush(void)
{
	m_LastError = ERROR_SUCCESS;

	if (!m_bIsWritable || m_DataSize == 0)
		return true;

	// �o�b�t�@�擪�ʒu�ɏ�������
	if (!CNFile::SetPos(m_FilePos) || !CNFile::Write(m_pBuff, m_DataSize))
		return false;
	m_FilePos += m_DataSize;

	// �o�b�t�@�T�C�Y�N���A
	m_DataSize = 0;

	return true;
}


#if 0
// SetFileBandwidthReservation �̎g�������悭������Ȃ�
const bool CNCachedFile::ReserveBandwidth(DWORD BytesPerSecond)
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return false;
	}

	HMODULE hKernel=::GetModuleHandle(TEXT("kernel32.dll"));
	if (!hKernel) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return false;
	}

	typedef BOOL (WINAPI *SetFileBandwidthReservationFunc)(HANDLE hFile,
		DWORD nPeriodMilliseconds, DWORD nBytesPerPeriod, BOOL bDiscardable,
		LPDWORD lpTransferSize, LPDWORD lpNumOutstandingRequests);
	SetFileBandwidthReservationFunc pSetFileBandwidthReservation =
		(SetFileBandwidthReservationFunc)::GetProcAddress(hKernel, "SetFileBandwidthReservation");
	if (!pSetFileBandwidthReservation) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return false;
	}

	DWORD TransferSize, NumOutstandingRequests;
	if (!pSetFileBandwidthReservation(m_hFile, 1000, BytesPerSecond, FALSE,
									  &TransferSize, &NumOutstandingRequests)) {
		m_LastError = ::GetLastError();
		return false;
	}

	m_LastError = ERROR_SUCCESS;

	return true;
}
#endif
