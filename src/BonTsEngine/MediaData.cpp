// MediaData.cpp: CMediaData クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MediaData.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#pragma intrinsic(memcpy, memset)


//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

#define MINBUFSIZE	256UL		// 最小バッファサイズ
//#define MINADDSIZE	256UL		// 最小追加確保サイズ


CMediaData::CMediaData()
	: m_dwDataSize(0UL)
	, m_dwBuffSize(0UL)
	, m_pData(NULL)
{
	// 空のバッファを生成する
}

CMediaData::CMediaData(const CMediaData &Operand)
	: m_dwDataSize(0UL)
	, m_dwBuffSize(0UL)
	, m_pData(NULL)
{
	// コピーコンストラクタ
	*this = Operand;
}

CMediaData::CMediaData(const DWORD dwBuffSize)
	: m_dwDataSize(0UL)
	, m_dwBuffSize(0UL)
	, m_pData(NULL)
{
	// バッファサイズを指定してバッファを生成する
	GetBuffer(dwBuffSize);
}

CMediaData::CMediaData(const BYTE *pData, const DWORD dwDataSize)
	: m_dwDataSize(0UL)
	, m_dwBuffSize(0UL)
	, m_pData(NULL)
{
	// データ初期値を指定してバッファを生成する
	SetData(pData, dwDataSize);
}

CMediaData::CMediaData(const BYTE byFiller, const DWORD dwDataSize)
	: m_dwDataSize(0UL)
	, m_dwBuffSize(0UL)
	, m_pData(NULL)
{
	// フィルデータを指定してバッファを生成する
	SetSize(dwDataSize, byFiller);
}

CMediaData::~CMediaData()
{
	if (m_pData)
		Free(m_pData);
}

CMediaData & CMediaData::operator = (const CMediaData &Operand)
{
	if (&Operand != this) {
		// バッファサイズの情報まではコピーしない
		SetData(Operand.m_pData, Operand.m_dwDataSize);
	}
	return *this;
}

CMediaData & CMediaData::operator += (const CMediaData &Operand)
{
	AddData(&Operand);
	return *this;
}

BYTE *CMediaData::GetData()
{
	// バッファポインタを取得する
	return m_dwDataSize > 0 ? m_pData : NULL;
}

const BYTE *CMediaData::GetData() const
{
	// バッファポインタを取得する
	return m_dwDataSize > 0 ? m_pData : NULL;
}

void CMediaData::SetAt(const DWORD dwPos, const BYTE byData)
{
	// 1バイトセットする
	if (dwPos < m_dwDataSize)
		m_pData[dwPos] = byData;
}

const BYTE CMediaData::GetAt(const DWORD dwPos) const
{
	// 1バイト取得する
	return dwPos < m_dwDataSize ? m_pData[dwPos] : 0x00;
}

const DWORD CMediaData::SetData(const void *pData, const DWORD dwDataSize)
{
	if (dwDataSize > 0) {
		// バッファ確保
		if (GetBuffer(dwDataSize) < dwDataSize)
			return m_dwDataSize;

		// データセット
		::CopyMemory(m_pData, pData, dwDataSize);
	}

	// サイズセット
	m_dwDataSize = dwDataSize;

	return m_dwDataSize;
}

const DWORD CMediaData::AddData(const void *pData, const DWORD dwDataSize)
{
	if (dwDataSize > 0) {
		// バッファ確保
		DWORD NewSize = m_dwDataSize + dwDataSize;
		if (GetBuffer(NewSize) < NewSize)
			return m_dwDataSize;

		// データ追加
		::CopyMemory(&m_pData[m_dwDataSize], pData, dwDataSize);

		// サイズセット
		m_dwDataSize += dwDataSize;
	}
	return m_dwDataSize;
}

const DWORD CMediaData::AddData(const CMediaData *pData)
{
	//return AddData(pData->m_pData, pData->m_dwDataSize);
	if (pData->m_dwDataSize > 0) {
		// バッファ確保
		DWORD NewSize = m_dwDataSize + pData->m_dwDataSize;
		if (GetBuffer(NewSize) < NewSize)
			return m_dwDataSize;

		// データ追加
		::CopyMemory(&m_pData[m_dwDataSize], pData->m_pData, pData->m_dwDataSize);

		// サイズセット
		m_dwDataSize = NewSize;
	}
	return m_dwDataSize;
}

const DWORD CMediaData::AddByte(const BYTE byData)
{
	// バッファ確保
	if (GetBuffer(m_dwDataSize + 1) <= m_dwDataSize)
		return m_dwDataSize;

	// データ追加
	m_pData[m_dwDataSize] = byData;

	// サイズ更新
	m_dwDataSize++;

	return m_dwDataSize;
}

const DWORD CMediaData::TrimHead(const DWORD dwTrimSize)
{
	// データ先頭を切り詰める
	if (m_dwDataSize == 0 || dwTrimSize == 0) {
		// 何もしない
	} else if (dwTrimSize >= m_dwDataSize) {
		// 全体を切り詰める
		m_dwDataSize = 0UL;
	} else {
		// データを移動する
		::MoveMemory(m_pData, m_pData + dwTrimSize, m_dwDataSize - dwTrimSize);
		m_dwDataSize -= dwTrimSize;
	}

	return m_dwDataSize;
}

const DWORD CMediaData::TrimTail(const DWORD dwTrimSize)
{
	// データ末尾を切り詰める
	if (dwTrimSize >= m_dwDataSize) {
		// 全体を切り詰める
		m_dwDataSize = 0UL;
	} else {
		// データ末尾を切り詰める
		m_dwDataSize -= dwTrimSize;
	}

	return m_dwDataSize;
}

const DWORD CMediaData::GetBuffer(const DWORD dwGetSize)
{
	if (dwGetSize <= m_dwBuffSize)
		return m_dwBuffSize;

	// 少なくとも指定サイズを格納できるバッファを確保する
	if (!m_pData) {
		// バッファ確保まだ
		DWORD dwBuffSize = max(dwGetSize, MINBUFSIZE);

		m_pData = static_cast<BYTE*>(Allocate(dwBuffSize));
		if (m_pData)
			m_dwBuffSize = dwBuffSize;
	} else if (dwGetSize > m_dwBuffSize) {
		// 要求サイズはバッファサイズを超える
		DWORD dwBuffSize = dwGetSize;

		if (dwBuffSize < 0x100000UL) {
			if (dwBuffSize < m_dwDataSize * 2)
				dwBuffSize = m_dwDataSize * 2;
		} else {
			dwBuffSize = (dwBuffSize / 0x100000UL + 1) * 0x100000UL;
		}

		BYTE *pNewBuffer = static_cast<BYTE*>(ReAllocate(m_pData, dwBuffSize));

		if (pNewBuffer != NULL) {
			m_dwBuffSize = dwBuffSize;
			m_pData = pNewBuffer;
		}
	}

	return m_dwBuffSize;
}

const DWORD CMediaData::SetSize(const DWORD dwSetSize)
{
	if (dwSetSize > 0) {
		// バッファ確保
		if (GetBuffer(dwSetSize) < dwSetSize)
			return m_dwDataSize;
	}

	// サイズセット
	m_dwDataSize = dwSetSize;

	return m_dwDataSize;
}

const DWORD CMediaData::SetSize(const DWORD dwSetSize, const BYTE byFiller)
{
	// サイズセット
	if (SetSize(dwSetSize) < dwSetSize)
		return m_dwDataSize;

	// データセット
	if (dwSetSize > 0)
		::FillMemory(m_pData, dwSetSize, byFiller);

	return m_dwDataSize;
}

void CMediaData::ClearSize(void)
{
	// データサイズをクリアする
	m_dwDataSize = 0UL;
}

void CMediaData::ClearBuffer(void)
{
	m_dwDataSize = 0UL;
	m_dwBuffSize = 0UL;
	if (m_pData) {
		Free(m_pData);
		m_pData = NULL;
	}
}

void *CMediaData::Allocate(size_t Size)
{
	return malloc(Size);
}

void CMediaData::Free(void *pBuffer)
{
	free(pBuffer);
}

void *CMediaData::ReAllocate(void *pBuffer, size_t Size)
{
	return realloc(pBuffer, Size);
}
