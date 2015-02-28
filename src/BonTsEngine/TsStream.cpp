// TsStream.cpp: TS�X�g���[�����b�p�[�N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TsStream.h"
#include "TsUtilClass.h"

#include <mmsystem.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#pragma comment(lib, "winmm.lib")

#pragma intrinsic(memset, memcmp)


//////////////////////////////////////////////////////////////////////
// CTsPacket�N���X�̍\�z/����
//////////////////////////////////////////////////////////////////////

CTsPacket::CTsPacket()
{
#ifdef TSPACKET_NEED_ALIGNED_PAYLOAD
	GetBuffer(4 + 192);
#else
	GetBuffer(TS_PACKETSIZE);
#endif

	// ��̃p�P�b�g�𐶐�����
	::ZeroMemory(&m_Header, sizeof(m_Header));
	::ZeroMemory(&m_AdaptationField, sizeof(m_AdaptationField));
}

CTsPacket::CTsPacket(const CTsPacket &Operand)
{
	*this = Operand;
}

CTsPacket::~CTsPacket()
{
	ClearBuffer();
}

CTsPacket & CTsPacket::operator = (const CTsPacket &Operand)
{
	if (&Operand != this) {
		// �C���X�^���X�̃R�s�[
		CMediaData::operator = (Operand);

		m_Header = Operand.m_Header;
		m_AdaptationField = Operand.m_AdaptationField;
		if (Operand.m_AdaptationField.pOptionData)
			m_AdaptationField.pOptionData = &m_pData[6];
	}

	return *this;
}

DWORD CTsPacket::ParsePacket(BYTE *pContinuityCounter)
{
	// TS�p�P�b�g�w�b�_���
	m_Header.bySyncByte					= m_pData[0];							// +0
	m_Header.bTransportErrorIndicator	= (m_pData[1] & 0x80U) != 0;	// +1 bit7
	m_Header.bPayloadUnitStartIndicator	= (m_pData[1] & 0x40U) != 0;	// +1 bit6
	m_Header.TransportPriority			= (m_pData[1] & 0x20U) != 0;	// +1 bit5
	m_Header.wPID = ((WORD)(m_pData[1] & 0x1F) << 8) | (WORD)m_pData[2];		// +1 bit4-0, +2
	m_Header.byTransportScramblingCtrl	= (m_pData[3] & 0xC0U) >> 6;			// +3 bit7-6
	m_Header.byAdaptationFieldCtrl		= (m_pData[3] & 0x30U) >> 4;			// +3 bit5-4
	m_Header.byContinuityCounter		= m_pData[3] & 0x0FU;					// +3 bit3-0

	// �A�_�v�e�[�V�����t�B�[���h���
	::ZeroMemory(&m_AdaptationField, sizeof(m_AdaptationField));

	if (m_Header.byAdaptationFieldCtrl & 0x02) {
		// �A�_�v�e�[�V�����t�B�[���h����
		m_AdaptationField.byAdaptationFieldLength = m_pData[4];							// +4
		if (m_AdaptationField.byAdaptationFieldLength > 0) {
			// �t�B�[���h���ȍ~����
			m_AdaptationField.bDiscontinuityIndicator	= (m_pData[5] & 0x80U) != 0;	// +5 bit7
			m_AdaptationField.bRamdomAccessIndicator	= (m_pData[5] & 0x40U) != 0;	// +5 bit6
			m_AdaptationField.bEsPriorityIndicator		= (m_pData[5] & 0x20U) != 0;	// +5 bit5
			m_AdaptationField.bPcrFlag					= (m_pData[5] & 0x10U) != 0;	// +5 bit4
			m_AdaptationField.bOpcrFlag					= (m_pData[5] & 0x08U) != 0;	// +5 bit3
			m_AdaptationField.bSplicingPointFlag		= (m_pData[5] & 0x04U) != 0;	// +5 bit2
			m_AdaptationField.bTransportPrivateDataFlag	= (m_pData[5] & 0x02U) != 0;	// +5 bit1
			m_AdaptationField.bAdaptationFieldExtFlag	= (m_pData[5] & 0x01U) != 0;	// +5 bit0

			if (m_AdaptationField.byAdaptationFieldLength > 1U) {
				m_AdaptationField.pOptionData			= &m_pData[6];
				m_AdaptationField.byOptionSize			= m_AdaptationField.byAdaptationFieldLength - 1U;
			}
		}
	}

	// �p�P�b�g�̃t�H�[�}�b�g�K�������`�F�b�N����
	if(m_Header.bySyncByte != 0x47U)return EC_FORMAT;								// �����o�C�g�s��
	if(m_Header.bTransportErrorIndicator)return EC_TRANSPORT;						// �r�b�g��肠��
	if((m_Header.wPID >= 0x0002U) && (m_Header.wPID <= 0x000FU))return EC_FORMAT;	// ����`PID�͈�
	if(m_Header.byTransportScramblingCtrl == 0x01U)return EC_FORMAT;				// ����`�X�N�����u������l
	if(m_Header.byAdaptationFieldCtrl == 0x00U)return EC_FORMAT;					// ����`�A�_�v�e�[�V�����t�B�[���h����l
	if((m_Header.byAdaptationFieldCtrl == 0x02U) && (m_AdaptationField.byAdaptationFieldLength > 183U))return EC_FORMAT;	// �A�_�v�e�[�V�����t�B�[���h���ُ�
	if((m_Header.byAdaptationFieldCtrl == 0x03U) && (m_AdaptationField.byAdaptationFieldLength > 182U))return EC_FORMAT;	// �A�_�v�e�[�V�����t�B�[���h���ُ�

	if(!pContinuityCounter || m_Header.wPID == 0x1FFFU)return EC_VALID;

	// �A�����`�F�b�N
	const BYTE byOldCounter = pContinuityCounter[m_Header.wPID];
	const BYTE byNewCounter = (m_Header.byAdaptationFieldCtrl & 0x01U)? m_Header.byContinuityCounter : 0x10U;
	pContinuityCounter[m_Header.wPID] = byNewCounter;

	if(!m_AdaptationField.bDiscontinuityIndicator){
		if((byOldCounter < 0x10U) && (byNewCounter < 0x10U)){
			if(((byOldCounter + 1U) & 0x0FU) != byNewCounter){
				return EC_CONTINUITY;
				}
			}
		}

	return EC_VALID;
}

BYTE * CTsPacket::GetPayloadData(void)
{
	// �y�C���[�h�|�C���^��Ԃ�
	switch (m_Header.byAdaptationFieldCtrl) {
	case 1U :	// �y�C���[�h�̂�
		return &m_pData[4];

	case 3U :	// �A�_�v�e�[�V�����t�B�[���h�A�y�C���[�h����
		return &m_pData[m_AdaptationField.byAdaptationFieldLength + 5U];

	default :	// �A�_�v�e�[�V�����t�B�[���h�̂� or ��O
		return NULL;
	}
}

const BYTE * CTsPacket::GetPayloadData(void) const
{
	// �y�C���[�h�|�C���^��Ԃ�
	switch (m_Header.byAdaptationFieldCtrl) {
	case 1U :	// �y�C���[�h�̂�
		return &m_pData[4];

	case 3U :	// �A�_�v�e�[�V�����t�B�[���h�A�y�C���[�h����
		return &m_pData[m_AdaptationField.byAdaptationFieldLength + 5U];

	default :	// �A�_�v�e�[�V�����t�B�[���h�̂� or ��O
		return NULL;
	}
}

const BYTE CTsPacket::GetPayloadSize(void) const
{
	// �y�C���[�h�T�C�Y��Ԃ�
	switch(m_Header.byAdaptationFieldCtrl){
	case 1U :	// �y�C���[�h�̂�
		return (TS_PACKETSIZE - 4U);

	case 3U :	// �A�_�v�e�[�V�����t�B�[���h�A�y�C���[�h����
		return (TS_PACKETSIZE - m_AdaptationField.byAdaptationFieldLength - 5U);

	default :	// �A�_�v�e�[�V�����t�B�[���h�̂� or ��O
		return 0U;
	}
}

// �o�b�t�@�ɏ�������
void CTsPacket::StoreToBuffer(void *pBuffer)
{
	BYTE *p=static_cast<BYTE*>(pBuffer);

	::CopyMemory(p,m_pData,TS_PACKETSIZE);
	p+=TS_PACKETSIZE;
	::CopyMemory(p,&m_Header,sizeof(m_Header));
	p+=sizeof(m_Header);
	::CopyMemory(p,&m_AdaptationField,sizeof(m_AdaptationField));
}

// �o�b�t�@����ǂݍ���
void CTsPacket::RestoreFromBuffer(const void *pBuffer)
{
	const BYTE *p=static_cast<const BYTE*>(pBuffer);

	::CopyMemory(m_pData,p,TS_PACKETSIZE);
	p+=TS_PACKETSIZE;
	::CopyMemory(&m_Header,p,sizeof(m_Header));
	p+=sizeof(m_Header);
	::CopyMemory(&m_AdaptationField,p,sizeof(m_AdaptationField));
	if (m_AdaptationField.pOptionData)
		m_AdaptationField.pOptionData=&m_pData[6];
}

#ifdef TSPACKET_NEED_ALIGNED_PAYLOAD

void *CTsPacket::Allocate(size_t Size)
{
	return _aligned_offset_malloc(Size, 16, 4);
}

void CTsPacket::Free(void *pBuffer)
{
	_aligned_free(pBuffer);
}

void *CTsPacket::ReAllocate(void *pBuffer, size_t Size)
{
	return _aligned_offset_realloc(pBuffer, Size, 16, 4);
}

#endif	// TSPACKET_NEED_ALIGNED_PAYLOAD




//////////////////////////////////////////////////////////////////////
// CPsiSection�N���X�̍\�z/����
//////////////////////////////////////////////////////////////////////

CPsiSection::CPsiSection()
	: CMediaData()
{
	Reset();
}

CPsiSection::CPsiSection(const DWORD dwBuffSize)
	: CMediaData(dwBuffSize)
{
	Reset();
}

CPsiSection::CPsiSection(const CPsiSection &Operand)
{
	Reset();

	*this = Operand;
}

CPsiSection & CPsiSection::operator = (const CPsiSection &Operand)
{
	if (&Operand != this) {
		// �C���X�^���X�̃R�s�[
		CMediaData::operator = (Operand);
		m_Header = Operand.m_Header;
	}

	return *this;
}

const bool CPsiSection::operator == (const CPsiSection &Operand) const
{
	// �Z�N�V�����̓��e���r����
	if (GetPayloadSize() != Operand.GetPayloadSize()) {
		// �T�C�Y���قȂ�
		return false;
	}

	const BYTE *pSrcData = GetPayloadData();
	const BYTE *pDstData = Operand.GetPayloadData();

	if (!pSrcData && !pDstData) {
		// �������NULL
		return true;
	}

	if (!pSrcData || !pDstData) {
		// �������NULL
		return false;
	}

#if 0
	// �o�C�i����r
	for (DWORD dwPos = 0 ; dwPos < GetPayloadSize() ; dwPos++) {
		if (pSrcData[dwPos] != pDstData[dwPos])
			return false;
	}

	// ��v����
	return true;
#else
	return memcmp(pSrcData, pDstData, GetPayloadSize()) == 0;
#endif
}

const bool CPsiSection::ParseHeader(const bool bIsExtended, const bool bIgnoreSectionNumber)
{
	const DWORD dwHeaderSize = (bIsExtended)? 8UL : 3UL;

	// �w�b�_�T�C�Y�`�F�b�N
	if (m_dwDataSize < dwHeaderSize)
		return false;

	// �W���`���w�b�_���
	m_Header.byTableID					= m_pData[0];							// +0
	m_Header.bSectionSyntaxIndicator	= (m_pData[1] & 0x80U) != 0;			// +1 bit7
	m_Header.bPrivateIndicator			= (m_pData[1] & 0x40U) != 0;			// +1 bit6
	m_Header.wSectionLength = ((WORD)(m_pData[1] & 0x0FU) << 8) | (WORD)m_pData[2];		// +1 bit5-0, +2

	if(m_Header.bSectionSyntaxIndicator && bIsExtended){
		// �g���`���̃w�b�_���
		m_Header.wTableIdExtension		= (WORD)m_pData[3] << 8 | (WORD)m_pData[4];		// +3, +4
		m_Header.byVersionNo			= (m_pData[5] & 0x3EU) >> 1;			// +5 bit5-1
		m_Header.bCurrentNextIndicator	= (m_pData[5] & 0x01U) != 0;			// +5 bit0
		m_Header.bySectionNumber		= m_pData[6];							// +6
		m_Header.byLastSectionNumber	= m_pData[7];							// +7
		}

	// �w�b�_�̃t�H�[�}�b�g�K�������`�F�b�N����
	if (m_Header.byTableID == 0xFF)
		return false;
	if((m_pData[1] & 0x30U) != 0x30U)
		return false;									// �Œ�r�b�g�ُ�
	else if(m_Header.wSectionLength > 4093U)
		return false;									// �Z�N�V�������ُ�
	else if(m_Header.bSectionSyntaxIndicator){
		// �g���`���̃G���[�`�F�b�N
		if(!bIsExtended)
			return false;								// �ړI�̃w�b�_�ł͂Ȃ�
		else if((m_pData[5] & 0xC0U) != 0xC0U)
			return false;								// �Œ�r�b�g�ُ�
		else if(!bIgnoreSectionNumber
				&& m_Header.bySectionNumber > m_Header.byLastSectionNumber)
			return false;	// �Z�N�V�����ԍ��ُ�
		else if(m_Header.wSectionLength < 9U)
			return false;								// �Z�N�V�������ُ�
		}
	else{
		// �W���`���̃G���[�`�F�b�N	
		if(bIsExtended)
			return false;								// �ړI�̃w�b�_�ł͂Ȃ�
		else if(m_Header.wSectionLength < 4U)
			return false;								// �Z�N�V�������ُ�
		}

	return true;
}

void CPsiSection::Reset(void)
{
	// �f�[�^���N���A����
	ClearSize();
	::ZeroMemory(&m_Header, sizeof(m_Header));
}

BYTE * CPsiSection::GetPayloadData(void) const
{
	// �y�C���[�h�|�C���^��Ԃ�
	const DWORD dwHeaderSize = (m_Header.bSectionSyntaxIndicator)? 8UL : 3UL;

	return m_dwDataSize > dwHeaderSize ? &m_pData[dwHeaderSize] : NULL;
}

const WORD CPsiSection::GetPayloadSize(void) const
{
	// �y�C���[�h�T�C�Y��Ԃ�(���ۂɕێ����Ă�@���Z�N�V��������菭�Ȃ��Ȃ邱�Ƃ�����)
	const DWORD dwHeaderSize = (m_Header.bSectionSyntaxIndicator)? 8UL : 3UL;

	if(m_dwDataSize <= dwHeaderSize)return 0U;
	else if(m_Header.bSectionSyntaxIndicator){
		// �g���Z�N�V����
		return (m_dwDataSize >= (m_Header.wSectionLength + 3UL))? (m_Header.wSectionLength - 9U) : ((WORD)m_dwDataSize - 8U);
		}
	else{
		// �W���Z�N�V����
		return (m_dwDataSize >= (m_Header.wSectionLength + 3UL))? m_Header.wSectionLength : ((WORD)m_dwDataSize - 3U);
		}
}

const BYTE CPsiSection::GetTableID(void) const
{
	// �e�[�u��ID��Ԃ�
	return m_Header.byTableID;
}

const bool CPsiSection::IsExtendedSection(void) const
{
	// �Z�N�V�����V���^�b�N�X�C���W�P�[�^��Ԃ�
	return m_Header.bSectionSyntaxIndicator;
}

const bool CPsiSection::IsPrivate(void) const
{
	// �v���C�x�[�g�C���W�P�[�^��Ԃ�
	return m_Header.bPrivateIndicator;
}

const WORD CPsiSection::GetSectionLength(void) const
{
	// �Z�N�V��������Ԃ�
	return m_Header.wSectionLength;
}

const WORD CPsiSection::GetTableIdExtension(void) const
{
	// �e�[�u��ID�g����Ԃ�
	return m_Header.wTableIdExtension;
}

const BYTE CPsiSection::GetVersionNo(void) const
{
	// �o�[�W�����ԍ���Ԃ�
	return m_Header.byVersionNo;
}

const bool CPsiSection::IsCurrentNext(void) const
{
	// �J�����g�l�N�X�g�C���W�P�[�^��Ԃ�
	return m_Header.bCurrentNextIndicator;
}

const BYTE CPsiSection::GetSectionNumber(void) const
{
	// �Z�N�V�����ԍ���Ԃ�
	return m_Header.bySectionNumber;
}

const BYTE CPsiSection::GetLastSectionNumber(void) const
{
	// ���X�g�Z�N�V�����ԍ���Ԃ�
	return m_Header.byLastSectionNumber;
}


/////////////////////////////////////////////////////////////////////////////
// TS PID�}�b�v�ΏۃN���X
/////////////////////////////////////////////////////////////////////////////

void CTsPidMapTarget::OnPidMapped(const WORD wPID, const PVOID pParam)
{
	// �}�b�s���O���ꂽ
}

void CTsPidMapTarget::OnPidUnmapped(const WORD wPID)
{
	// �}�b�v�������ꂽ
}


/////////////////////////////////////////////////////////////////////////////
// TS PID�}�b�v�Ǘ��N���X
/////////////////////////////////////////////////////////////////////////////

CTsPidMapManager::CTsPidMapManager()
	: m_wMapCount(0U)
{
	::ZeroMemory(m_PidMap, sizeof(m_PidMap));
}

CTsPidMapManager::~CTsPidMapManager()
{
	UnmapAllTarget();
}

const bool CTsPidMapManager::StorePacket(const CTsPacket *pPacket)
{
	const WORD wPID = pPacket->GetPID();

	if(wPID > 0x1FFFU)return false;					// PID�͈͊O

	if(!m_PidMap[wPID].pMapTarget)return false;		// PID�}�b�v�^�[�Q�b�g�Ȃ�

	if(m_PidMap[wPID].pMapTarget->StorePacket(pPacket)){
		// �^�[�Q�b�g�̍X�V���������Ƃ��̓R�[���o�b�N���Ăяo��

		if(m_PidMap[wPID].pMapCallback){
			m_PidMap[wPID].pMapCallback(wPID, m_PidMap[wPID].pMapTarget, this, m_PidMap[wPID].pMapParam);
			}
		}

	return true;
}

const bool CTsPidMapManager::MapTarget(const WORD wPID, CTsPidMapTarget *pMapTarget, const PIDMAPHANDLERFUNC pMapCallback, const PVOID pMapParam)
{
	if((wPID > 0x1FFFU) || (!pMapTarget))return false;

	// ���݂̃^�[�Q�b�g���A���}�b�v
	UnmapTarget(wPID);

	// �V�����^�[�Q�b�g���}�b�v
	m_PidMap[wPID].pMapTarget = pMapTarget;
	m_PidMap[wPID].pMapCallback = pMapCallback;
	m_PidMap[wPID].pMapParam = pMapParam;
	m_wMapCount++;

	pMapTarget->OnPidMapped(wPID, pMapParam);

	return true;
}

const bool CTsPidMapManager::UnmapTarget(const WORD wPID)
{
	if(wPID > 0x1FFFU)return false;

	if(!m_PidMap[wPID].pMapTarget)return false;

	// ���݂̃^�[�Q�b�g���A���}�b�v
	CTsPidMapTarget *pTarget = m_PidMap[wPID].pMapTarget;
	::ZeroMemory(&m_PidMap[wPID], sizeof(m_PidMap[wPID]));
	m_wMapCount--;

	pTarget->OnPidUnmapped(wPID);

	return true;
}

void CTsPidMapManager::UnmapAllTarget(void)
{
	// �S�^�[�Q�b�g���A���}�b�v
	for (WORD wPID = 0x0000U ; wPID <= 0x1FFFU ; wPID++) {
		UnmapTarget(wPID);
	}
}

CTsPidMapTarget * CTsPidMapManager::GetMapTarget(const WORD wPID) const
{
	// �}�b�v����Ă���^�[�Q�b�g��Ԃ�
	return (wPID <= 0x1FFFU)? m_PidMap[wPID].pMapTarget : NULL;
}

const WORD CTsPidMapManager::GetMapCount(void) const
{
	// �}�b�v����Ă���PID�̑�����Ԃ�
	return m_wMapCount;
}


/////////////////////////////////////////////////////////////////////////////
// PSI�Z�N�V�������o�N���X
/////////////////////////////////////////////////////////////////////////////

CPsiSectionParser::CPsiSectionParser(IPsiSectionHandler *pPsiSectionHandler,
									 const bool bTargetExt, const bool bIgnoreSectionNumber)
	: m_pPsiSectionHandler(pPsiSectionHandler)
	, m_PsiSection(0x10002UL)		// PSI�Z�N�V�����ő�T�C�Y�̃o�b�t�@�m��
	, m_bTargetExt(bTargetExt)
	, m_bIgnoreSectionNumber(bIgnoreSectionNumber)
	, m_bIsStoring(false)
	, m_dwCrcErrorCount(0UL)
{

}

CPsiSectionParser::CPsiSectionParser(const CPsiSectionParser &Operand)
{
	*this = Operand;
}

CPsiSectionParser & CPsiSectionParser::operator = (const CPsiSectionParser &Operand)
{
	if (&Operand != this) {
		// �C���X�^���X�̃R�s�[
		m_pPsiSectionHandler = Operand.m_pPsiSectionHandler;
		m_bTargetExt = Operand.m_bTargetExt;
		m_PsiSection = Operand.m_PsiSection;
		m_bIsStoring = Operand.m_bIsStoring;
		m_dwStoreCrc = Operand.m_dwStoreCrc;
		m_wStoreSize = Operand.m_wStoreSize;
		m_dwCrcErrorCount = Operand.m_dwCrcErrorCount;
	}

	return *this;
}

void CPsiSectionParser::StorePacket(const CTsPacket *pPacket)
{
	const BYTE *pData = pPacket->GetPayloadData();
	const BYTE byPayloadSize = pPacket->GetPayloadSize();
	if (!byPayloadSize || !pData)
		return;

	BYTE byPos, bySize;

	if (pPacket->m_Header.bPayloadUnitStartIndicator) {
		// [�w�b�_�f�� | �y�C���[�h�f��] + [�X�^�b�t�B���O�o�C�g] + �w�b�_�擪 + [�w�b�_�f��] + [�y�C���[�h�f��] + [�X�^�b�t�B���O�o�C�g]
		const BYTE byUnitStartPos = pData[0] + 1U;
		if (byUnitStartPos >= byPayloadSize)
			return;

		if (byUnitStartPos > 1U) {
			// ���j�b�g�J�n�ʒu���擪�ł͂Ȃ��ꍇ(�f�Ђ�����ꍇ)
			byPos = 1U;
			bySize = byUnitStartPos - byPos;
			if (m_bIsStoring) {
				StorePayload(&pData[byPos], &bySize);
			} else if (m_PsiSection.GetSize() > 0
					&& StoreHeader(&pData[byPos], &bySize)) {
				byPos += bySize;
				bySize = byUnitStartPos - byPos;
				StorePayload(&pData[byPos], &bySize);
			}
		}

		// ���j�b�g�J�n�ʒu����V�K�Z�N�V�����̃X�g�A���J�n����
		m_PsiSection.Reset();
		m_bIsStoring = false;

		byPos = byUnitStartPos;
		while (byPos < byPayloadSize) {
			bySize = byPayloadSize - byPos;
			if (!m_bIsStoring) {
				if (!StoreHeader(&pData[byPos], &bySize))
					break;
				byPos += bySize;
				bySize = byPayloadSize - byPos;
			}
			if (!StorePayload(&pData[byPos], &bySize))
				break;
			byPos += bySize;
			if (byPos >= byPayloadSize || pData[byPos] == 0xFF)
				break;
		}
	} else {
		// [�w�b�_�f��] + �y�C���[�h + [�X�^�b�t�B���O�o�C�g]
		byPos = 0U;
		bySize = byPayloadSize;
		if (!m_bIsStoring) {
			if (m_PsiSection.GetSize() == 0
					|| !StoreHeader(&pData[byPos], &bySize))
				return;
			byPos += bySize;
			bySize = byPayloadSize - byPos;
		}
		StorePayload(&pData[byPos], &bySize);
	}
}

void CPsiSectionParser::Reset(void)
{
	// ��Ԃ�����������
	m_bIsStoring = false;
	m_wStoreSize = 0U;
	m_dwCrcErrorCount = 0UL;
	m_PsiSection.Reset();
}

const DWORD CPsiSectionParser::GetCrcErrorCount(void) const
{
	// CRC�G���[�񐔂�Ԃ�
	return m_dwCrcErrorCount;
}

const bool CPsiSectionParser::StoreHeader(const BYTE *pPayload, BYTE *pbyRemain)
{
	// �w�b�_����͂��ăZ�N�V�����̃X�g�A���J�n����
	if (m_bIsStoring) {
		*pbyRemain = 0;
		return false;
	}

	const BYTE byHeaderSize = (m_bTargetExt)? 8U : 3U;
	const BYTE byHeaderRemain = byHeaderSize - (BYTE)m_PsiSection.GetSize();

	if (*pbyRemain >= byHeaderRemain) {
		// �w�b�_�X�g�A�����A�w�b�_����͂��ăy�C���[�h�̃X�g�A���J�n����
		m_PsiSection.AddData(pPayload, byHeaderRemain);
		if (m_PsiSection.ParseHeader(m_bTargetExt, m_bIgnoreSectionNumber)) {
			// �w�b�_�t�H�[�}�b�gOK�A�w�b�_�݂̂�CRC���v�Z����
			m_wStoreSize = m_PsiSection.GetSectionLength() + 3U;
			m_dwStoreCrc = CCrcCalculator::CalcCrc32(m_PsiSection.GetData(), byHeaderSize);
			m_bIsStoring = true;
			*pbyRemain = byHeaderRemain;
			return true;
		} else {
			// �w�b�_�G���[
			m_PsiSection.Reset();
			*pbyRemain = byHeaderRemain;
			TRACE(TEXT("PSI header format error\n"));
			return false;
		}
	} else {
		// �w�b�_�X�g�A�������A���̃f�[�^��҂�
		m_PsiSection.AddData(pPayload, *pbyRemain);
		return false;
	}
}

const bool CPsiSectionParser::StorePayload(const BYTE *pPayload, BYTE *pbyRemain)
{
	// �Z�N�V�����̃X�g�A����������
	if (!m_bIsStoring) {
		*pbyRemain = 0;
		return false;
	}

	const BYTE byRemain = *pbyRemain;
	const WORD wStoreRemain = m_wStoreSize - (WORD)m_PsiSection.GetSize();

	if (wStoreRemain <= (WORD)byRemain) {
		// �X�g�A����
		m_PsiSection.AddData(pPayload, wStoreRemain);

		if (!CCrcCalculator::CalcCrc32(pPayload, wStoreRemain, m_dwStoreCrc)) {
			// CRC����A�n���h���ɃZ�N�V������n��
			if (m_pPsiSectionHandler)
				m_pPsiSectionHandler->OnPsiSection(this, &m_PsiSection);
			//TRACE(TEXT("[%02X] PSI Stored: %lu / %lu\n"), m_PsiSection.GetTableID(), m_PsiSection.GetSize(), (DWORD)m_wStoreSize);
		} else {
			// CRC�ُ�
			//if (m_dwCrcErrorCount < 0xFFFFFFFFUL)
				m_dwCrcErrorCount++;
			//TRACE(TEXT("[%02X] PSI CRC Error: %lu / %lu\n"), m_PsiSection.GetTableID(), m_PsiSection.GetSize(), (DWORD)m_wStoreSize);
		}

		// ��Ԃ����������A���̃Z�N�V������M�ɔ�����
		m_PsiSection.Reset();
		m_bIsStoring = false;

		*pbyRemain = (BYTE)wStoreRemain;
		return true;
	} else {
		// �X�g�A�������A���̃y�C���[�h��҂�
		m_PsiSection.AddData(pPayload, byRemain);
		m_dwStoreCrc = CCrcCalculator::CalcCrc32(pPayload, byRemain, m_dwStoreCrc);
		return false;
	}
}


/////////////////////////////////////////////////////////////////////////////
// PCR���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CTsClockRef::CTsClockRef()
{
	InitPcrPll(0LL);
}

CTsClockRef::CTsClockRef(const CTsClockRef &Operand)
{
	*this = Operand;
}

CTsClockRef & CTsClockRef::operator = (const CTsClockRef &Operand)
{
	if (&Operand != this) {
		// �C���X�^���X�̃R�s�[
		m_llHrcUnitFreq = Operand.m_llHrcUnitFreq;
		m_llHrcLastTime = Operand.m_llHrcLastTime;
		m_llCurPcrCount = Operand.m_llCurPcrCount;
		m_lfPllFeedBack = Operand.m_lfPllFeedBack;
	}

	return *this;
}

const bool CTsClockRef::StorePacket(const CTsPacket *pPacket, const WORD wPcrPID)
{
	if(pPacket->GetPID() != wPcrPID)return false;
	if(!pPacket->m_AdaptationField.bPcrFlag)return false;

	// 33bit 90KHz PCR���v�Z
	const LONGLONG llCurPcrCount = GetPcrFromHex(pPacket->m_AdaptationField.pOptionData);

	if(llCurPcrCount < 0LL){
		// PCR�Ȃ�(�G���[)
		TRACE(TEXT("PCR�Ȃ�(�G���[)\n"));
		return true;
		}
	else if(!m_llCurPcrCount){
		// PCR PLL������
		InitPcrPll(llCurPcrCount);
		TRACE(TEXT("PLL������\n"));
		}
	else if(pPacket->m_AdaptationField.bDiscontinuityIndicator){
		// PCR PLL�ē���
		SyncPcrPll(llCurPcrCount);
		TRACE(TEXT("PLL�ē���\n"));
		}
	else{
		// PCR PLL����
		ProcPcrPll(llCurPcrCount);
		}

	return true;
}

void CTsClockRef::Reset(void)
{
	InitPcrPll(0LL);
}

const LONGLONG CTsClockRef::GetGlobalPcr(void) const
{
	// ���ݎ�������O���[�o��PCR���擾����
	LONGLONG llHrcCurTime;
	::QueryPerformanceCounter((LARGE_INTEGER *)&llHrcCurTime);

	// �Ō�ɍX�V���ꂽPCR + �X�V����̌o�ߎ��� = ���݂�PCR
	return (m_llGlobalPcrCount + (LONGLONG)(((double)(llHrcCurTime - m_llHrcLastTime) * 90000.0) / (double)m_llHrcUnitFreq));
}

const LONGLONG CTsClockRef::GetCurrentPcr(void) const
{
	// ���ݎ�������PCR���擾����
	LONGLONG llHrcCurTime;
	::QueryPerformanceCounter((LARGE_INTEGER *)&llHrcCurTime);

	// �Ō�ɍX�V���ꂽPCR + �X�V����̌o�ߎ��� = ���݂�PCR
	return (m_llCurPcrCount + (LONGLONG)(((double)(llHrcCurTime - m_llHrcLastTime) * 90000.0) / (double)m_llHrcUnitFreq));
}

const LONGLONG CTsClockRef::PtsToGlobalPcr(const LONGLONG llPts) const
{
	// PTS��PCR�ɕϊ�����
	if(llPts > m_llCurPcrCount){
		// �O���[�o��PCR��PCR�̃I�t�Z�b�g�����Z�����l��Ԃ�
		return m_llGlobalPcrCount + (llPts - m_llCurPcrCount);
		}
	else{
		// ���Ɏ������߂��Ă���(�G���[ or �啝�ȏ����x��)
		LONGLONG llHrcCurTime;
		::QueryPerformanceCounter((LARGE_INTEGER *)&llHrcCurTime);

		// �Ō�ɍX�V���ꂽPCR + �X�V����̌o�ߎ��� = ���݂�PCR
		return (m_llGlobalPcrCount + (LONGLONG)(((double)(llHrcCurTime - m_llHrcLastTime) * 90000.0) / (double)m_llHrcUnitFreq));		
		}
}

void CTsClockRef::InitPcrPll(const LONGLONG llCurPcr)
{
	// PLL������������
	::QueryPerformanceFrequency((LARGE_INTEGER *)&m_llHrcUnitFreq);
	::QueryPerformanceCounter((LARGE_INTEGER *)&m_llHrcLastTime);

	m_llCurPcrCount = llCurPcr;
	m_lfPllFeedBack = 0.0;

	m_llGlobalPcrCount = 0LL;
	m_llBasePcrCount = llCurPcr;
}

void CTsClockRef::ProcPcrPll(const LONGLONG llCurPcr)
{
	// PLL����������
	ULONGLONG llHrcCurTime;
	::QueryPerformanceCounter((LARGE_INTEGER *)&llHrcCurTime);

	// ���[�J��PCR���v�Z����(������\�^�C�}�ɂ��������̐ϕ��l)
	m_llCurPcrCount += (LONGLONG)(((double)(llHrcCurTime - m_llHrcLastTime) * 90000.0) / (double)m_llHrcUnitFreq);	// 50ms
	m_llHrcLastTime = llHrcCurTime;

	// ���[�J��PCR�ƃX�g���[��PCR�̈ʑ�����LPF���{��(�t�B�[�h�o�b�N�Q�C�� = -40dB�A���萔 = ��5.5s)
	m_lfPllFeedBack = m_lfPllFeedBack * 0.99 + (double)(m_llCurPcrCount - llCurPcr) * 0.01;

	// ���[�J��PCR�Ɉʑ������t�B�[�h�o�b�N����
	m_llCurPcrCount -= (LONGLONG)m_lfPllFeedBack;

	// �O���[�o��PCR���X�V����
	m_llGlobalPcrCount = m_llCurPcrCount - m_llBasePcrCount;
}

void CTsClockRef::SyncPcrPll(const LONGLONG llCurPcr)
{
	// PCR�s�A����������PLL���ē�������
	::QueryPerformanceCounter((LARGE_INTEGER *)&m_llHrcLastTime);

	// PLL�����l�ݒ�
	m_llCurPcrCount = llCurPcr;
	m_lfPllFeedBack = 0.0;

	// �O���[�o��PCR�̊�_�Đݒ�
	m_llBasePcrCount = llCurPcr;
}

inline const LONGLONG CTsClockRef::GetPcrFromHex(const BYTE *pPcrData)
{
	// PCR����͂���(42bit 27MHz)
	LONGLONG llCurPcrCount = 0LL;
	llCurPcrCount |= (LONGLONG)pPcrData[0] << 34;
	llCurPcrCount |= (LONGLONG)pPcrData[1] << 26;
	llCurPcrCount |= (LONGLONG)pPcrData[2] << 18;
	llCurPcrCount |= (LONGLONG)pPcrData[3] << 10;
	llCurPcrCount |= (LONGLONG)(pPcrData[4] & 0x80U) << 2;
	llCurPcrCount |= (LONGLONG)(pPcrData[4] & 0x01U) << 8;
	llCurPcrCount |= (LONGLONG)pPcrData[5];

	// 33bit 90KHz�ɃV�t�g����(42bit�ł͉��Z�덷���~�ς��Ďg�����ɂȂ�Ȃ�)
	return llCurPcrCount >> 9;
}
