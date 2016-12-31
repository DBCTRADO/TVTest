// MediaData.cpp: CMediaData �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MediaData.h"
#include "../Common/DebugDef.h"


#pragma intrinsic(memcpy, memset)


//////////////////////////////////////////////////////////////////////
// �\�z/����
//////////////////////////////////////////////////////////////////////

#define MINBUFSIZE	256UL		// �ŏ��o�b�t�@�T�C�Y
//#define MINADDSIZE	256UL		// �ŏ��ǉ��m�ۃT�C�Y


CMediaData::CMediaData()
	: m_dwDataSize(0UL)
	, m_dwBuffSize(0UL)
	, m_pData(NULL)
{
	// ��̃o�b�t�@�𐶐�����
}

CMediaData::CMediaData(const CMediaData &Operand)
	: m_dwDataSize(0UL)
	, m_dwBuffSize(0UL)
	, m_pData(NULL)
{
	// �R�s�[�R���X�g���N�^
	*this = Operand;
}

CMediaData::CMediaData(const DWORD dwBuffSize)
	: m_dwDataSize(0UL)
	, m_dwBuffSize(0UL)
	, m_pData(NULL)
{
	// �o�b�t�@�T�C�Y���w�肵�ăo�b�t�@�𐶐�����
	GetBuffer(dwBuffSize);
}

CMediaData::CMediaData(const BYTE *pData, const DWORD dwDataSize)
	: m_dwDataSize(0UL)
	, m_dwBuffSize(0UL)
	, m_pData(NULL)
{
	// �f�[�^�����l���w�肵�ăo�b�t�@�𐶐�����
	SetData(pData, dwDataSize);
}

CMediaData::CMediaData(const BYTE byFiller, const DWORD dwDataSize)
	: m_dwDataSize(0UL)
	, m_dwBuffSize(0UL)
	, m_pData(NULL)
{
	// �t�B���f�[�^���w�肵�ăo�b�t�@�𐶐�����
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
		// �o�b�t�@�T�C�Y�̏��܂ł̓R�s�[���Ȃ�
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
	// �o�b�t�@�|�C���^���擾����
	return m_dwDataSize > 0 ? m_pData : NULL;
}

const BYTE *CMediaData::GetData() const
{
	// �o�b�t�@�|�C���^���擾����
	return m_dwDataSize > 0 ? m_pData : NULL;
}

void CMediaData::SetAt(const DWORD dwPos, const BYTE byData)
{
	// 1�o�C�g�Z�b�g����
	if (dwPos < m_dwDataSize)
		m_pData[dwPos] = byData;
}

const BYTE CMediaData::GetAt(const DWORD dwPos) const
{
	// 1�o�C�g�擾����
	return dwPos < m_dwDataSize ? m_pData[dwPos] : 0x00;
}

const DWORD CMediaData::SetData(const void *pData, const DWORD dwDataSize)
{
	if (dwDataSize > 0) {
		// �o�b�t�@�m��
		if (GetBuffer(dwDataSize) < dwDataSize)
			return m_dwDataSize;

		// �f�[�^�Z�b�g
		::CopyMemory(m_pData, pData, dwDataSize);
	}

	// �T�C�Y�Z�b�g
	m_dwDataSize = dwDataSize;

	return m_dwDataSize;
}

const DWORD CMediaData::AddData(const void *pData, const DWORD dwDataSize)
{
	if (dwDataSize > 0) {
		// �o�b�t�@�m��
		DWORD NewSize = m_dwDataSize + dwDataSize;
		if (GetBuffer(NewSize) < NewSize)
			return m_dwDataSize;

		// �f�[�^�ǉ�
		::CopyMemory(&m_pData[m_dwDataSize], pData, dwDataSize);

		// �T�C�Y�Z�b�g
		m_dwDataSize += dwDataSize;
	}
	return m_dwDataSize;
}

const DWORD CMediaData::AddData(const CMediaData *pData)
{
	//return AddData(pData->m_pData, pData->m_dwDataSize);
	if (pData->m_dwDataSize > 0) {
		// �o�b�t�@�m��
		DWORD NewSize = m_dwDataSize + pData->m_dwDataSize;
		if (GetBuffer(NewSize) < NewSize)
			return m_dwDataSize;

		// �f�[�^�ǉ�
		::CopyMemory(&m_pData[m_dwDataSize], pData->m_pData, pData->m_dwDataSize);

		// �T�C�Y�Z�b�g
		m_dwDataSize = NewSize;
	}
	return m_dwDataSize;
}

const DWORD CMediaData::AddByte(const BYTE byData)
{
	// �o�b�t�@�m��
	if (GetBuffer(m_dwDataSize + 1) <= m_dwDataSize)
		return m_dwDataSize;

	// �f�[�^�ǉ�
	m_pData[m_dwDataSize] = byData;

	// �T�C�Y�X�V
	m_dwDataSize++;

	return m_dwDataSize;
}

const DWORD CMediaData::TrimHead(const DWORD dwTrimSize)
{
	// �f�[�^�擪��؂�l�߂�
	if (m_dwDataSize == 0 || dwTrimSize == 0) {
		// �������Ȃ�
	} else if (dwTrimSize >= m_dwDataSize) {
		// �S�̂�؂�l�߂�
		m_dwDataSize = 0UL;
	} else {
		// �f�[�^���ړ�����
		::MoveMemory(m_pData, m_pData + dwTrimSize, m_dwDataSize - dwTrimSize);
		m_dwDataSize -= dwTrimSize;
	}

	return m_dwDataSize;
}

const DWORD CMediaData::TrimTail(const DWORD dwTrimSize)
{
	// �f�[�^������؂�l�߂�
	if (dwTrimSize >= m_dwDataSize) {
		// �S�̂�؂�l�߂�
		m_dwDataSize = 0UL;
	} else {
		// �f�[�^������؂�l�߂�
		m_dwDataSize -= dwTrimSize;
	}

	return m_dwDataSize;
}

const DWORD CMediaData::GetBuffer(const DWORD dwGetSize)
{
	if (dwGetSize <= m_dwBuffSize)
		return m_dwBuffSize;

	// ���Ȃ��Ƃ��w��T�C�Y���i�[�ł���o�b�t�@���m�ۂ���
	if (!m_pData) {
		// �o�b�t�@�m�ۂ܂�
		DWORD dwBuffSize = max(dwGetSize, MINBUFSIZE);

		m_pData = static_cast<BYTE*>(Allocate(dwBuffSize));
		if (m_pData)
			m_dwBuffSize = dwBuffSize;
	} else if (dwGetSize > m_dwBuffSize) {
		// �v���T�C�Y�̓o�b�t�@�T�C�Y�𒴂���
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
		// �o�b�t�@�m��
		if (GetBuffer(dwSetSize) < dwSetSize)
			return m_dwDataSize;
	}

	// �T�C�Y�Z�b�g
	m_dwDataSize = dwSetSize;

	return m_dwDataSize;
}

const DWORD CMediaData::SetSize(const DWORD dwSetSize, const BYTE byFiller)
{
	// �T�C�Y�Z�b�g
	if (SetSize(dwSetSize) < dwSetSize)
		return m_dwDataSize;

	// �f�[�^�Z�b�g
	if (dwSetSize > 0)
		::FillMemory(m_pData, dwSetSize, byFiller);

	return m_dwDataSize;
}

void CMediaData::ClearSize(void)
{
	// �f�[�^�T�C�Y���N���A����
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
