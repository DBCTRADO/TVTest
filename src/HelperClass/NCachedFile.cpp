// NCachedFile.cpp: CNCachedFile クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <new>
#include "NCachedFile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// 構築/消滅
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
		m_LastError = ERROR_BUSY;	// 「要求したリソースは使用中です。」
		return false;
	}

	if ((Flags & CNFile::CNF_WRITE) && !(Flags & CNFile::CNF_READ)) {
		// ライトキャッシュ有効
		m_bIsWritable = true;
	} else if (!(Flags & CNFile::CNF_WRITE) && (Flags & CNFile::CNF_READ) && !(Flags & CNFile::CNF_NEW)) {
		// リードキャッシュ有効
		m_bIsWritable = false;
	} else {
		// フラグの組み合わせが非対応
		m_LastError = ERROR_INVALID_PARAMETER;
		return false;
	}

	if (dwBuffSize==0) {
		m_LastError = ERROR_INVALID_PARAMETER;
		return false;
	}

	// ファイルオープン
	if (!CNFile::Open(lpszName, Flags))
		return false;

	// バッファ確保
	m_BuffSize = dwBuffSize;
	if (!m_bIsWritable && (ULONGLONG)dwBuffSize > GetSize()) {
		// 読み込みバッファ
		m_BuffSize = (DWORD)GetSize();
	}
	try {
		m_pBuff = new BYTE [m_BuffSize];
	} catch (std::bad_alloc&) {
		Close();
		m_LastError = ERROR_OUTOFMEMORY;
		return false;
	}

	// バッファ初期化
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
		// 未書き込みデータフラッシュ
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
	// エラー処理
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

	// ファイルシーク
	if (!SetPos(llPos))
		return 0;

	return Read(pBuff, dwLen);
}


const bool CNCachedFile::Write(const void *pBuff, const DWORD dwLen)
{
	// ファイル書き込み
	if (!m_bIsWritable) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return false;
	}

	if (pBuff == NULL || dwLen == 0) {
		m_LastError = ERROR_INVALID_PARAMETER;
		return false;
	}

	// バッファリング判定
	if ((m_BuffSize - m_DataSize) < dwLen) {
		// バッファ不足
		if (!Flush())
			return false;
	}

	// バッファリング
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

	// ファイルポジション更新
	m_CurPos += (ULONGLONG)dwLen;

	m_LastError = ERROR_SUCCESS;

	return true;
}


const bool CNCachedFile::Write(const void *pBuff, const DWORD dwLen, const ULONGLONG llPos)
{
	// ファイル書き込み
	if (!m_bIsWritable) {
		m_LastError = ERROR_INVALID_FUNCTION;
		return false;
	}

	// ファイルシーク
	if (!SetPos(llPos))
		return false;

	return Write(pBuff, dwLen);
}


const ULONGLONG CNCachedFile::GetPos(void)
{
	// 論理的なファイルポジションを返す
	return m_bIsWritable ? m_CurPos : CNFile::GetPos();
}


const bool CNCachedFile::SetPos(const ULONGLONG llPos)
{
	// シーク前にフラッシュ
	if (!Flush())
		return false;

	// シーク
	if (!CNFile::SetPos(llPos))
		return false;

	// ファイルポジション更新
	m_FilePos = llPos;
	m_CurPos = llPos;

	return true;
}


const bool CNCachedFile::Flush(void)
{
	m_LastError = ERROR_SUCCESS;

	if (!m_bIsWritable || m_DataSize == 0)
		return true;

	// バッファ先頭位置に書き込み
	if (!CNFile::SetPos(m_FilePos) || !CNFile::Write(m_pBuff, m_DataSize))
		return false;
	m_FilePos += m_DataSize;

	// バッファサイズクリア
	m_DataSize = 0;

	return true;
}


#if 0
// SetFileBandwidthReservation の使い方がよく分からない
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
