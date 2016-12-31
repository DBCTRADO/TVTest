// TsDescriptor.h: �L�q�q���b�p�[�N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Common.h"
#include "TsDescriptor.h"
#include "TsEncode.h"
#include "../HelperClass/StdUtil.h"
#include "../Common/DebugDef.h"


inline DWORD MSBFirst32(const BYTE *p)
{
	return ((DWORD)p[0] << 24) | ((DWORD)p[1] << 16) | ((DWORD)p[2] << 8) | (DWORD)p[3];
}


/////////////////////////////////////////////////////////////////////////////
// �L�q�q�̊��N���X
/////////////////////////////////////////////////////////////////////////////

CBaseDesc::CBaseDesc()
{
	Reset();
}

CBaseDesc::~CBaseDesc()
{
}

void CBaseDesc::CopyDesc(const CBaseDesc *pOperand)
{
	// �C���X�^���X�̃R�s�[
	*this = *pOperand;
}

bool CBaseDesc::ParseDesc(const BYTE *pHexData, const WORD wDataLength)
{
	Reset();

	// ���ʃt�H�[�}�b�g���`�F�b�N
	if(!pHexData)return false;										// �f�[�^����
	else if(wDataLength < 2U)return false;							// �f�[�^���Œ�L�q�q�T�C�Y����
	else if(wDataLength < (WORD)(pHexData[1] + 2U))return false;	// �f�[�^���L�q�q�̃T�C�Y����������

	m_byDescTag = pHexData[0];
	m_byDescLen = pHexData[1];

	// �y�C���[�h���
	if (m_byDescLen > 0 && StoreContents(&pHexData[2])) {
		m_bIsValid = true;
	}

	return m_bIsValid;
}

bool CBaseDesc::IsValid() const
{
	// �f�[�^���L��(��͍�)���ǂ�����Ԃ�
	return m_bIsValid;
}

BYTE CBaseDesc::GetTag() const
{
	// �L�q�q�^�O��Ԃ�
	return m_byDescTag;
}

BYTE CBaseDesc::GetLength() const
{
	// �L�q�q����Ԃ�
	return m_byDescLen;
}

void CBaseDesc::Reset()
{
	// ��Ԃ��N���A����
	m_byDescTag = 0x00U;
	m_byDescLen = 0U;
	m_bIsValid = false;
}

bool CBaseDesc::StoreContents(const BYTE *pPayload)
{
	// �f�t�H���g�̎����ł͉������Ȃ�
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x09] Conditional Access �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CCaMethodDesc::CCaMethodDesc()
{
	Reset();
}

void CCaMethodDesc::Reset()
{
	CBaseDesc::Reset();

	m_wCaMethodID = 0x0000U;		// Conditional Access Method ID
	m_wCaPID = 0xFFFFU;				// Conditional Access PID
	m_PrivateData.ClearSize();		// Private Data
}

WORD CCaMethodDesc::GetCaMethodID() const
{
	// Conditional Access Method ID ��Ԃ�
	return m_wCaMethodID;
}

WORD CCaMethodDesc::GetCaPID() const
{
	// Conditional Access PID
	return m_wCaPID;
}

const CMediaData * CCaMethodDesc::GetPrivateData() const
{
	// Private Data ��Ԃ�
	return &m_PrivateData;
}

bool CCaMethodDesc::StoreContents(const BYTE *pPayload)
{
	// �t�H�[�}�b�g���`�F�b�N
	if(m_byDescTag != DESC_TAG)return false;							// �^�O���s��
	if(m_byDescLen < 4U)return false;								// CA���\�b�h�L�q�q�̍ŏ��T�C�Y��4
	if((pPayload[2] & 0xE0U) != 0xE0U)return false;				// �Œ�r�b�g���s��

	// �L�q�q�����
	m_wCaMethodID = ((WORD)pPayload[0] << 8) | (WORD)pPayload[1];			// +0,1	Conditional Access Method ID
	m_wCaPID = ((WORD)(pPayload[2] & 0x1FU) << 8) | (WORD)pPayload[3];	// +2,3	Conditional Access PID
	m_PrivateData.SetData(&pPayload[4], m_byDescLen - 4U);				// +4-	Private Data

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x48] Service �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CServiceDesc::CServiceDesc()
{
	Reset();
}

CServiceDesc::CServiceDesc(const CServiceDesc &Operand)
{
	*this = Operand;
}

CServiceDesc & CServiceDesc::operator = (const CServiceDesc &Operand)
{
	if (&Operand != this) {
		CBaseDesc::CopyDesc(&Operand);

		m_byServiceType = Operand.m_byServiceType;
		StdUtil::strncpy(m_szProviderName, _countof(m_szProviderName), Operand.m_szProviderName);
		StdUtil::strncpy(m_szServiceName, _countof(m_szServiceName), Operand.m_szServiceName);
	}

	return *this;
}

void CServiceDesc::Reset()
{
	CBaseDesc::Reset();

	m_byServiceType = 0x00U;			// Service Type
	m_szProviderName[0] = TEXT('\0');	// Service Provider Name
	m_szServiceName[0] = TEXT('\0');	// Service Name
}

BYTE CServiceDesc::GetServiceType() const
{
	// Service Type��Ԃ�
	return m_byServiceType;
}

DWORD CServiceDesc::GetProviderName(LPTSTR lpszDst, int MaxLength) const
{
	// Service Provider Name��Ԃ�
	if (lpszDst && MaxLength > 0)
		::lstrcpyn(lpszDst, m_szProviderName, MaxLength);

	return ::lstrlen(m_szProviderName);
}

DWORD CServiceDesc::GetServiceName(LPTSTR lpszDst, int MaxLength) const
{
	// Service Provider Name��Ԃ�
	if (lpszDst && MaxLength > 0)
		::lstrcpyn(lpszDst, m_szServiceName, MaxLength);

	return ::lstrlen(m_szServiceName);
}

bool CServiceDesc::StoreContents(const BYTE *pPayload)
{
	// �t�H�[�}�b�g���`�F�b�N
	if(m_byDescTag != DESC_TAG)return false;	// �^�O���s��
	else if(m_byDescLen < 3U)return false;		// �T�[�r�X�L�q�q�̃T�C�Y�͍Œ�3

	// �L�q�q�����
	m_byServiceType = pPayload[0];				// +0	Service Type

	int Pos = 1, Length;

	// Provider Name
	Length = pPayload[Pos++];
	m_szProviderName[0] = '\0';
	if (Length > 0) {
		if (Pos + Length >= m_byDescLen)
			return false;
		CAribString::AribToString(m_szProviderName, _countof(m_szProviderName), &pPayload[Pos], Length);
		Pos += Length;
	}

	// Service Name
	Length = pPayload[Pos++];
	m_szServiceName[0] = '\0';
	if (Length > 0) {
		if (Pos + Length > m_byDescLen)
			return false;
		CAribString::AribToString(m_szServiceName, _countof(m_szServiceName), &pPayload[Pos], Length);
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x4D] Short Event �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CShortEventDesc::CShortEventDesc()
{
	Reset();
}

CShortEventDesc::CShortEventDesc(const CShortEventDesc &Operand)
{
	*this = Operand;
}

CShortEventDesc & CShortEventDesc::operator = (const CShortEventDesc &Operand)
{
	if (&Operand != this) {
		CBaseDesc::CopyDesc(&Operand);

		m_dwLanguageCode = Operand.m_dwLanguageCode;
		StdUtil::strncpy(m_szEventName, _countof(m_szEventName), Operand.m_szEventName);
		StdUtil::strncpy(m_szEventDesc, _countof(m_szEventDesc), Operand.m_szEventDesc);
	}

	return *this;
}

void CShortEventDesc::Reset()
{
	CBaseDesc::Reset();

	m_dwLanguageCode = 0UL;			// ISO639  Language Code
	m_szEventName[0] = TEXT('\0');	// Event Name
	m_szEventDesc[0] = TEXT('\0');	// Event Description
}

DWORD CShortEventDesc::GetLanguageCode() const
{
	// Language Code��Ԃ�
	return m_dwLanguageCode;
}

DWORD CShortEventDesc::GetEventName(LPTSTR lpszDst, int MaxLength) const
{
	// Event Name��Ԃ�
	if (lpszDst && MaxLength > 0)
		::lstrcpyn(lpszDst, m_szEventName, MaxLength);

	return ::lstrlen(m_szEventName);
}

DWORD CShortEventDesc::GetEventDesc(LPTSTR lpszDst, int MaxLength) const
{
	// Event Description��Ԃ�
	if (lpszDst && MaxLength > 0)
		::lstrcpyn(lpszDst, m_szEventDesc, MaxLength);

	return ::lstrlen(m_szEventDesc);
}

bool CShortEventDesc::StoreContents(const BYTE *pPayload)
{
	// �t�H�[�}�b�g���`�F�b�N
	if(m_byDescTag != DESC_TAG)return false;	// �^�O���s��
	else if(m_byDescLen < 5U)return false;		// Short Event�L�q�q�̃T�C�Y�͍Œ�5

	// �L�q�q�����
	m_dwLanguageCode = ((DWORD)pPayload[0] << 16) | ((DWORD)pPayload[1] << 8) | (DWORD)pPayload[2];		// +0 - +2	ISO639  Language Code

	int Pos = 3, Length;

	// Event Name
	Length = pPayload[Pos++];
	m_szEventName[0] = '\0';
	if (Length > 0) {
		if (Pos + Length >= m_byDescLen)
			return false;
		CAribString::AribToString(m_szEventName, _countof(m_szEventName), &pPayload[Pos], Length);
		Pos += Length;
	}

	// Event Description
	Length = pPayload[Pos++];
	m_szEventDesc[0] = '\0';
	if (Length > 0) {
		if (Pos + Length > m_byDescLen)
			return false;
		CAribString::AribToString(m_szEventDesc, _countof(m_szEventDesc), &pPayload[Pos], Length);
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x4E] Extended Event �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CExtendedEventDesc::CExtendedEventDesc()
{
	Reset();
}

void CExtendedEventDesc::Reset()
{
	CBaseDesc::Reset();

	m_DescriptorNumber = 0;
	m_LastDescriptorNumber = 0;
	m_LanguageCode = 0UL;
	m_ItemList.clear();
}

BYTE CExtendedEventDesc::GetDescriptorNumber() const
{
	return m_DescriptorNumber;
}

BYTE CExtendedEventDesc::GetLastDescriptorNumber() const
{
	return m_LastDescriptorNumber;
}

DWORD CExtendedEventDesc::GetLanguageCode() const
{
	return m_LanguageCode;
}

int CExtendedEventDesc::GetItemCount() const
{
	return (int)m_ItemList.size();
}

const CExtendedEventDesc::ItemInfo * CExtendedEventDesc::GetItem(int Index) const
{
	if (Index < 0 || Index >= (int)m_ItemList.size())
		return NULL;
	return &m_ItemList[Index];
}

bool CExtendedEventDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen < 5)
		return false;

	m_DescriptorNumber = pPayload[0] >> 4;
	m_LastDescriptorNumber = pPayload[0] & 0x0F;
	m_LanguageCode = ((DWORD)pPayload[1] << 16) | ((DWORD)pPayload[2] << 8) | (DWORD)pPayload[3];
	m_ItemList.clear();
	const int ItemLength = pPayload[4];
	if (5 + ItemLength > (int)m_byDescLen)
		return false;
	int Pos = 5;
	while (Pos < 5 + ItemLength) {
		ItemInfo Item;

		const int DescriptionLength = pPayload[Pos++];
		if (Pos + DescriptionLength >= (int)m_byDescLen)
			break;
		Item.szDescription[0] = '\0';
		if (DescriptionLength > 0)
			CAribString::AribToString(Item.szDescription, MAX_DESCRIPTION, &pPayload[Pos], DescriptionLength);
		Pos += DescriptionLength;

		const BYTE ItemLength = pPayload[Pos++];
		if (Pos + (int)ItemLength > (int)m_byDescLen)
			break;
		Item.ItemLength = min(ItemLength, 220);
		::CopyMemory(Item.ItemChar, &pPayload[Pos], Item.ItemLength);

		m_ItemList.push_back(Item);

		Pos += ItemLength;
	}
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x52] Stream Identifier �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CStreamIdDesc::CStreamIdDesc()
{
	Reset();
}

void CStreamIdDesc::Reset()
{
	CBaseDesc::Reset();

	m_byComponentTag = 0x00U;	// Component Tag
}

BYTE CStreamIdDesc::GetComponentTag() const
{
	// Component Tag ��Ԃ�
	return m_byComponentTag;
}

bool CStreamIdDesc::StoreContents(const BYTE *pPayload)
{
	// �t�H�[�}�b�g���`�F�b�N
	if(m_byDescTag != DESC_TAG)return false;	// �^�O���s��
	else if(m_byDescLen != 1U)return false;		// �X�g���[��ID�L�q�q�̃T�C�Y�͏��1

	// �L�q�q�����
	m_byComponentTag = pPayload[0];				// +0	Component Tag

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0xC0] Hierarchical Transmission �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CHierarchicalTransmissionDesc::CHierarchicalTransmissionDesc()
{
	Reset();
}

void CHierarchicalTransmissionDesc::Reset()
{
	CBaseDesc::Reset();

	m_QualityLevel = 0xFF;
	m_ReferencePID = 0xFFFF;
}

BYTE CHierarchicalTransmissionDesc::GetQualityLevel() const
{
	return m_QualityLevel;
}

WORD CHierarchicalTransmissionDesc::GetReferencePID() const
{
	return m_ReferencePID;
}

bool CHierarchicalTransmissionDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen != 3)
		return false;

	m_QualityLevel = pPayload[0] & 0x01;
	m_ReferencePID = ((WORD)(pPayload[1] & 0x1F) << 8) | (WORD)pPayload[2];

	/*
	TRACE(TEXT("hierarchical_transmission_descriptor\nquality_level = %d / reference_PID = %04x\n"),
		  m_QualityLevel, m_ReferencePID);
	*/

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x40] Network Name �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CNetworkNameDesc::CNetworkNameDesc()
{
	Reset();
}

CNetworkNameDesc::CNetworkNameDesc(const CNetworkNameDesc &Operand)
{
	*this = Operand;
}

CNetworkNameDesc & CNetworkNameDesc::operator = (const CNetworkNameDesc &Operand)
{
	if (&Operand != this) {
		CBaseDesc::CopyDesc(&Operand);

		StdUtil::strncpy(m_szNetworkName, _countof(m_szNetworkName), Operand.m_szNetworkName);
	}

	return *this;
}

void CNetworkNameDesc::Reset()
{
	CBaseDesc::Reset();
	m_szNetworkName[0] = '\0';
}

DWORD CNetworkNameDesc::GetNetworkName(LPTSTR pszName, int MaxLength) const
{
	if (pszName && MaxLength > 0)
		::lstrcpyn(pszName, m_szNetworkName, MaxLength);
	return ::lstrlen(m_szNetworkName);
}

bool CNetworkNameDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG)
		return false;

	m_szNetworkName[0] = '\0';
	CAribString::AribToString(m_szNetworkName, _countof(m_szNetworkName), &pPayload[0], m_byDescLen);

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x41] Service List �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CServiceListDesc::CServiceListDesc()
{
	Reset();
}

void CServiceListDesc::Reset()
{
	CBaseDesc::Reset();

	m_ServiceList.clear();
}

int CServiceListDesc::GetServiceNum() const
{
	return (int)m_ServiceList.size();
}

int CServiceListDesc::GetServiceIndexByID(const WORD ServiceID) const
{
	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (m_ServiceList[i].ServiceID == ServiceID)
			return (int)i;
	}
	return -1;
}

BYTE CServiceListDesc::GetServiceTypeByID(const WORD ServiceID) const
{
	int Index = GetServiceIndexByID(ServiceID);
	if (Index >= 0)
		return m_ServiceList[Index].ServiceType;
	return SERVICE_TYPE_INVALID;
}

bool CServiceListDesc::GetServiceInfo(const int Index, ServiceInfo *pInfo) const
{
	if (!pInfo || Index < 0 || (size_t)Index >= m_ServiceList.size())
		return false;

	*pInfo = m_ServiceList[Index];

	return true;
}

bool CServiceListDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG)
		return false;

	const int NumServices = m_byDescLen / 3;

	m_ServiceList.resize(NumServices);

	int Pos = 0;
	for (int i = 0; i < NumServices; i++) {
		m_ServiceList[i].ServiceID = ((WORD)pPayload[Pos + 0] << 8) | (WORD)pPayload[Pos + 1];
		m_ServiceList[i].ServiceType = pPayload[Pos + 2];

		Pos += 3;
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x43] Satellite Delivery System �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CSatelliteDeliverySystemDesc::CSatelliteDeliverySystemDesc()
{
	Reset();
}

void CSatelliteDeliverySystemDesc::Reset()
{
	CBaseDesc::Reset();

	m_Frequency = 0;
	m_OrbitalPosition = 0;
	m_bWestEastFlag = false;
	m_Polarization = 0xFF;
	m_Modulation = 0;
	m_SymbolRate = 0;
	m_FECInner = 0;
}

DWORD CSatelliteDeliverySystemDesc::GetFrequency() const
{
	return m_Frequency;
}

WORD CSatelliteDeliverySystemDesc::GetOrbitalPosition() const
{
	return m_OrbitalPosition;
}

bool CSatelliteDeliverySystemDesc::GetWestEastFlag() const
{
	return m_bWestEastFlag;
}

BYTE CSatelliteDeliverySystemDesc::GetPolarization() const
{
	return m_Polarization;
}

BYTE CSatelliteDeliverySystemDesc::GetModulation() const
{
	return m_Modulation;
}

DWORD CSatelliteDeliverySystemDesc::GetSymbolRate() const
{
	return m_SymbolRate;
}

BYTE CSatelliteDeliverySystemDesc::GetFECInner() const
{
	return m_FECInner;
}

static DWORD GetBCD(const BYTE *pData, const int Length)
{
	DWORD Value = 0;
	for (int i = 0; i < Length; i++) {
		Value *= 10;
		if (i % 2 == 0)
			Value += pData[i / 2] >> 4;
		else
			Value += pData[i / 2] & 0x0F;
	}
	return Value;
}

bool CSatelliteDeliverySystemDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen != 11)
		return false;

	m_Frequency = GetBCD(&pPayload[0], 8);
	m_OrbitalPosition = (WORD)GetBCD(&pPayload[4], 4);
	m_bWestEastFlag = (pPayload[6] & 0x80) != 0;
	m_Polarization = (pPayload[6] >> 5) & 0x03;
	m_Modulation = pPayload[6] & 0x1F;
	m_SymbolRate = GetBCD(&pPayload[7], 7);
	m_FECInner = pPayload[10] & 0x0F;

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0xFA] Terrestrial Delivery System �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CTerrestrialDeliverySystemDesc::CTerrestrialDeliverySystemDesc()
{
	Reset();
}

void CTerrestrialDeliverySystemDesc::Reset()
{
	CBaseDesc::Reset();

	m_AreaCode = 0;
	m_GuardInterval = 0xFF;
	m_TransmissionMode = 0xFF;
	m_Frequency.clear();
}

WORD CTerrestrialDeliverySystemDesc::GetAreaCode() const
{
	return m_AreaCode;
}

BYTE CTerrestrialDeliverySystemDesc::GetGuardInterval() const
{
	return m_GuardInterval;
}

BYTE CTerrestrialDeliverySystemDesc::GetTransmissionMode() const
{
	return m_TransmissionMode;
}

int CTerrestrialDeliverySystemDesc::GetFrequencyNum() const
{
	return (int)m_Frequency.size();
}

WORD CTerrestrialDeliverySystemDesc::GetFrequency(const int Index) const
{
	if (Index < 0 || (size_t)Index >= m_Frequency.size())
		return 0;
	return m_Frequency[Index];
}

bool CTerrestrialDeliverySystemDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen < 4)
		return false;

	m_AreaCode = ((WORD)pPayload[0] << 4) | ((WORD)pPayload[1] >> 4);
	m_GuardInterval = (pPayload[1] & 0x0C) >> 2;
	m_TransmissionMode = pPayload[1] & 0x03;
	const int FrequencyNum = (m_byDescLen - 2) / 2;
	m_Frequency.resize(FrequencyNum);
	int Pos = 2;
	for (int i = 0; i < FrequencyNum; i++) {
		m_Frequency[i] = ((WORD)pPayload[Pos + 0] << 8) | (WORD)pPayload[Pos + 1];
		Pos += 2;
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0xFE] System Management �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CSystemManageDesc::CSystemManageDesc()
{
	Reset();
}

void CSystemManageDesc::Reset()
{
	CBaseDesc::Reset();

	m_byBroadcastingFlag = 0;
	m_byBroadcastingID = 0;
	m_byAdditionalBroadcastingID = 0;
}

BYTE CSystemManageDesc::GetBroadcastingFlag() const
{
	return m_byBroadcastingFlag;
}

BYTE CSystemManageDesc::GetBroadcastingID() const
{
	return m_byBroadcastingID;
}

BYTE CSystemManageDesc::GetAdditionalBroadcastingID() const
{
	return m_byAdditionalBroadcastingID;
}

bool CSystemManageDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen < 2)
		return false;

	m_byBroadcastingFlag = (pPayload[0] & 0xC0) >> 6;
	m_byBroadcastingID = (pPayload[0] & 0x3F);
	m_byAdditionalBroadcastingID = pPayload[1];

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0xCD] TS Information �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CTSInfoDesc::CTSInfoDesc()
{
	Reset();
}

CTSInfoDesc::CTSInfoDesc(const CTSInfoDesc &Operand)
{
	*this = Operand;
}

CTSInfoDesc & CTSInfoDesc::operator = (const CTSInfoDesc &Operand)
{
	if (&Operand != this) {
		CBaseDesc::CopyDesc(&Operand);

		m_byRemoteControlKeyID = Operand.m_byRemoteControlKeyID;
		StdUtil::strncpy(m_szTSName, _countof(m_szTSName), Operand.m_szTSName);
	}

	return *this;
}

void CTSInfoDesc::Reset()
{
	CBaseDesc::Reset();

	m_byRemoteControlKeyID = 0;
	m_szTSName[0] = '\0';
}

BYTE CTSInfoDesc::GetRemoteControlKeyID() const
{
	return m_byRemoteControlKeyID;
}

DWORD CTSInfoDesc::GetTSName(LPTSTR pszName, int MaxLength) const
{
	if (pszName && MaxLength > 0)
		::lstrcpyn(pszName, m_szTSName, MaxLength);
	return ::lstrlen(m_szTSName);
}

bool CTSInfoDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen < 2)
		return false;

	m_byRemoteControlKeyID = pPayload[0];

	BYTE Length = pPayload[1] >> 2;
	if (2 + Length > m_byDescLen)
		return false;

	m_szTSName[0] = '\0';
	CAribString::AribToString(m_szTSName, _countof(m_szTSName), &pPayload[2], Length);

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x50] Component �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CComponentDesc::CComponentDesc()
{
	Reset();
}

CComponentDesc::CComponentDesc(const CComponentDesc &Operand)
{
	*this = Operand;
}

CComponentDesc & CComponentDesc::operator = (const CComponentDesc &Operand)
{
	if (&Operand != this) {
		CBaseDesc::CopyDesc(&Operand);

		m_StreamContent = Operand.m_StreamContent;
		m_ComponentType = Operand.m_ComponentType;
		m_ComponentTag = Operand.m_ComponentTag;
		m_LanguageCode = Operand.m_LanguageCode;
		StdUtil::strncpy(m_szText, _countof(m_szText), Operand.m_szText);
	}

	return *this;
}

void CComponentDesc::Reset()
{
	CBaseDesc::Reset();

	m_StreamContent = 0;
	m_ComponentType = 0;
	m_ComponentTag = 0;
	m_LanguageCode = 0;
	m_szText[0] = '\0';
}

BYTE CComponentDesc::GetStreamContent() const
{
	return m_StreamContent;
}

BYTE CComponentDesc::GetComponentType() const
{
	return m_ComponentType;
}

BYTE CComponentDesc::GetComponentTag() const
{
	return m_ComponentTag;
}

DWORD CComponentDesc::GetLanguageCode() const
{
	return m_LanguageCode;
}

DWORD CComponentDesc::GetText(LPTSTR pszText, int MaxLength) const
{
	if (pszText && MaxLength > 0)
		::lstrcpyn(pszText, m_szText, MaxLength);
	return ::lstrlen(m_szText);
}

bool CComponentDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen < 6)
		return false;

	m_StreamContent = pPayload[0] & 0x0F;
	if (m_StreamContent != 0x01)
		return false;
	m_ComponentType = pPayload[1];
	m_ComponentTag = pPayload[2];
	m_LanguageCode = (pPayload[3] << 16) | (pPayload[4] << 8) | pPayload[5];
	m_szText[0]='\0';
	if (m_byDescLen > 6)
		CAribString::AribToString(m_szText, _countof(m_szText), &pPayload[6], min(m_byDescLen - 6, 16));
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0xC4] Audio Component �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CAudioComponentDesc::CAudioComponentDesc()
{
	Reset();
}

CAudioComponentDesc::CAudioComponentDesc(const CAudioComponentDesc &Operand)
{
	*this = Operand;
}

CAudioComponentDesc & CAudioComponentDesc::operator = (const CAudioComponentDesc &Operand)
{
	if (&Operand != this) {
		CBaseDesc::CopyDesc(&Operand);

		m_StreamContent = Operand.m_StreamContent;
		m_ComponentType = Operand.m_ComponentType;
		m_ComponentTag = Operand.m_ComponentTag;
		m_StreamType = Operand.m_StreamType;
		m_SimulcastGroupTag = Operand.m_SimulcastGroupTag;
		m_bESMultiLingualFlag = Operand.m_bESMultiLingualFlag;
		m_bMainComponentFlag = Operand.m_bMainComponentFlag;
		m_QualityIndicator = Operand.m_QualityIndicator;
		m_SamplingRate = Operand.m_SamplingRate;
		m_LanguageCode = Operand.m_LanguageCode;
		m_LanguageCode2 = Operand.m_LanguageCode2;
		StdUtil::strncpy(m_szText, _countof(m_szText), Operand.m_szText);
	}

	return *this;
}

void CAudioComponentDesc::Reset()
{
	CBaseDesc::Reset();

	m_StreamContent = 0;
	m_ComponentType = 0;
	m_ComponentTag = 0;
	m_StreamType = 0;
	m_SimulcastGroupTag = 0;
	m_bESMultiLingualFlag = false;
	m_bMainComponentFlag = false;
	m_QualityIndicator = 0;
	m_SamplingRate = 0;
	m_LanguageCode = 0;
	m_LanguageCode2 = 0;
	m_szText[0] = '\0';
}

BYTE CAudioComponentDesc::GetStreamContent() const
{
	return m_StreamContent;
}

BYTE CAudioComponentDesc::GetComponentType() const
{
	return m_ComponentType;
}

BYTE CAudioComponentDesc::GetComponentTag() const
{
	return m_ComponentTag;
}

BYTE CAudioComponentDesc::GetSimulcastGroupTag() const
{
	return m_SimulcastGroupTag;
}

bool CAudioComponentDesc::GetESMultiLingualFlag() const
{
	return m_bESMultiLingualFlag;
}

bool CAudioComponentDesc::GetMainComponentFlag() const
{
	return m_bMainComponentFlag;
}

BYTE CAudioComponentDesc::GetQualityIndicator() const
{
	return m_QualityIndicator;
}

BYTE CAudioComponentDesc::GetSamplingRate() const
{
	return m_SamplingRate;
}

DWORD CAudioComponentDesc::GetLanguageCode() const
{
	return m_LanguageCode;
}

DWORD CAudioComponentDesc::GetLanguageCode2() const
{
	return m_LanguageCode2;
}

DWORD CAudioComponentDesc::GetText(LPTSTR pszText, int MaxLength) const
{
	if (pszText && MaxLength > 0)
		::lstrcpyn(pszText, m_szText, MaxLength);
	return ::lstrlen(m_szText);
}

bool CAudioComponentDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen < 9)
		return false;

	m_StreamContent = pPayload[0] & 0x0F;
	if (m_StreamContent != 0x02)
		return false;
	m_ComponentType = pPayload[1];
	m_ComponentTag = pPayload[2];
	m_StreamType = pPayload[3];
	m_SimulcastGroupTag = pPayload[4];
	m_bESMultiLingualFlag = (pPayload[5] & 0x80) != 0;
	m_bMainComponentFlag = (pPayload[5] & 0x40) != 0;
	m_QualityIndicator = (pPayload[5] & 0x30) >> 4;
	m_SamplingRate = (pPayload[5] & 0x0E) >> 1;
	m_LanguageCode = (pPayload[6] << 16) | (pPayload[7] << 8) | pPayload[8];
	int Pos = 9;
	if (m_bESMultiLingualFlag) {
		if (Pos + 3 > m_byDescLen)
			return false;
		m_LanguageCode2 = (pPayload[Pos] << 16) | (pPayload[Pos + 1] << 8) | pPayload[Pos + 2];
		Pos += 3;
	}
	m_szText[0]='\0';
	if (Pos < m_byDescLen)
		CAribString::AribToString(m_szText, _countof(m_szText), &pPayload[Pos], min(m_byDescLen - Pos, 33));
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x54] Content �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CContentDesc::CContentDesc()
{
	Reset();
}

void CContentDesc::Reset()
{
	CBaseDesc::Reset();

	m_NibbleCount = 0;
}

int CContentDesc::GetNibbleCount() const
{
	return m_NibbleCount;
}

bool CContentDesc::GetNibble(int Index, Nibble *pNibble) const
{
	if (Index < 0 || Index >= m_NibbleCount || pNibble == NULL)
		return false;
	*pNibble = m_NibbleList[Index];
	return true;
}

bool CContentDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen > 14)
		return false;

	m_NibbleCount = m_byDescLen / 2;
	for (int i = 0; i < m_NibbleCount; i++) {
		m_NibbleList[i].ContentNibbleLevel1 = pPayload[i * 2 + 0] >> 4;
		m_NibbleList[i].ContentNibbleLevel2 = pPayload[i * 2 + 0] & 0x0F;
		m_NibbleList[i].UserNibble1 = pPayload[i * 2 + 1] >> 4;
		m_NibbleList[i].UserNibble2 = pPayload[i * 2 + 1] & 0x0F;
	}
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0xCF] Logo Transmission �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CLogoTransmissionDesc::CLogoTransmissionDesc()
{
	Reset();
}

void CLogoTransmissionDesc::Reset()
{
	CBaseDesc::Reset();

	m_LogoTransmissionType = TRANSMISSION_UNDEFINED;
	m_LogoID = LOGOID_INVALID;
	m_LogoVersion = LOGOVERSION_INVALID;
	m_DownloadDataID = DATAID_INVALID;
	m_LogoChar[0] = '\0';
}

BYTE CLogoTransmissionDesc::GetLogoTransmissionType() const
{
	return m_LogoTransmissionType;
}

WORD CLogoTransmissionDesc::GetLogoID() const
{
	return m_LogoID;
}

WORD CLogoTransmissionDesc::GetLogoVersion() const
{
	return m_LogoVersion;
}

WORD CLogoTransmissionDesc::GetDownloadDataID() const
{
	return m_DownloadDataID;
}

int CLogoTransmissionDesc::GetLogoChar(char *pChar, int MaxLength) const
{
	if (pChar == 0 || MaxLength <= 0)
		return 0;
	::lstrcpynA(pChar, m_LogoChar, MaxLength);
	return ::lstrlenA(m_LogoChar);
}

bool CLogoTransmissionDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen < 1)
		return false;

	m_LogoTransmissionType = pPayload[0];
	m_LogoID = LOGOID_INVALID;
	m_LogoVersion = LOGOVERSION_INVALID;
	m_DownloadDataID = DATAID_INVALID;
	m_LogoChar[0] = '\0';
	if (m_LogoTransmissionType == 0x01) {
		// CDT�`������1
		if (m_byDescLen < 7)
			return false;
		m_LogoID = ((WORD)(pPayload[1] & 0x01) << 8) | (WORD)pPayload[2];
		m_LogoVersion = ((WORD)(pPayload[3] & 0x0F) << 8) | (WORD) pPayload[4];
		m_DownloadDataID = ((WORD)pPayload[5] << 8) | (WORD)pPayload[6];
	} else if (m_LogoTransmissionType == 0x02) {
		// CDT�`������2
		if (m_byDescLen < 3)
			return false;
		m_LogoID = ((WORD)(pPayload[1] & 0x01) << 8) | (WORD)pPayload[2];
	} else if (m_LogoTransmissionType == 0x03) {
		// �ȈՃ��S����
		int i;
		for (i = 0; i < (int)m_byDescLen - 1 && i < MAX_LOGO_CHAR - 1; i++) {
			m_LogoChar[i] = pPayload[1 + i];
		}
		m_LogoChar[i] = '\0';
	}
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0xD5] Series �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CSeriesDesc::CSeriesDesc()
{
	Reset();
}

CSeriesDesc::CSeriesDesc(const CSeriesDesc &Operand)
{
	*this = Operand;
}

CSeriesDesc & CSeriesDesc::operator = (const CSeriesDesc &Operand)
{
	if (&Operand != this) {
		CBaseDesc::CopyDesc(&Operand);

		m_SeriesID = Operand.m_SeriesID;
		m_RepeatLabel = Operand.m_RepeatLabel;
		m_ProgramPattern = Operand.m_ProgramPattern;
		m_bExpireDateValidFlag = Operand.m_bExpireDateValidFlag;
		m_ExpireDate = Operand.m_ExpireDate;
		m_EpisodeNumber = Operand.m_EpisodeNumber;
		m_LastEpisodeNumber = Operand.m_LastEpisodeNumber;
		StdUtil::strncpy(m_szSeriesName, _countof(m_szSeriesName), Operand.m_szSeriesName);
	}

	return *this;
}

void CSeriesDesc::Reset()
{
	CBaseDesc::Reset();

	m_SeriesID = SERIESID_INVALID;
	m_RepeatLabel = 0x00;
	m_ProgramPattern = PROGRAMPATTERN_INVALID;
	m_bExpireDateValidFlag = false;
	m_EpisodeNumber = 0;
	m_LastEpisodeNumber = 0;
	m_szSeriesName[0] = '\0';
}

WORD CSeriesDesc::GetSeriesID() const
{
	return m_SeriesID;
}

BYTE CSeriesDesc::GetRepeatLabel() const
{
	return m_RepeatLabel;
}

BYTE CSeriesDesc::GetProgramPattern() const
{
	return m_ProgramPattern;
}

bool CSeriesDesc::IsExpireDateValid() const
{
	return m_bExpireDateValidFlag;
}

bool CSeriesDesc::GetExpireDate(SYSTEMTIME *pDate) const
{
	if (pDate == NULL || !m_bExpireDateValidFlag)
		return false;
	*pDate = m_ExpireDate;
	return true;
}

WORD CSeriesDesc::GetEpisodeNumber() const
{
	return m_EpisodeNumber;
}

WORD CSeriesDesc::GetLastEpisodeNumber() const
{
	return m_LastEpisodeNumber;
}

int CSeriesDesc::GetSeriesName(LPTSTR pszName, int MaxName) const
{
	if (pszName == NULL || MaxName <= 0)
		return 0;
	::lstrcpyn(pszName, m_szSeriesName, MaxName);
	return ::lstrlen(m_szSeriesName);
}

bool CSeriesDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen < 8)
		return false;

	m_SeriesID = ((WORD)pPayload[0] << 8) | (WORD)pPayload[1];
	m_RepeatLabel = pPayload[2] >> 4;
	m_ProgramPattern = (pPayload[2] & 0x0E) >> 1;
	m_bExpireDateValidFlag = (pPayload[2] & 0x01) != 0;
	if (m_bExpireDateValidFlag)
		CAribTime::MjdToSystemTime(((WORD)pPayload[3] << 8) | (WORD)pPayload[4], &m_ExpireDate);
	m_EpisodeNumber = ((WORD)pPayload[5] << 4) | (WORD)(pPayload[6] >> 4);
	m_LastEpisodeNumber = ((WORD)(pPayload[6] & 0x0F) << 8) | (WORD)pPayload[7];
	m_szSeriesName[0] = '\0';
	if (m_byDescLen > 8)
		CAribString::AribToString(m_szSeriesName, _countof(m_szSeriesName), &pPayload[8], m_byDescLen - 8);
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0xD6] Event Group �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CEventGroupDesc::CEventGroupDesc()
{
	Reset();
}

void CEventGroupDesc::Reset()
{
	CBaseDesc::Reset();

	m_GroupType = GROUPTYPE_UNDEFINED;
	m_EventList.clear();
}

BYTE CEventGroupDesc::GetGroupType() const
{
	return m_GroupType;
}

int CEventGroupDesc::GetEventNum() const
{
	return (int)m_EventList.size();
}

bool CEventGroupDesc::GetEventInfo(int Index, EventInfo *pInfo) const
{
	if (Index < 0 || Index >= (int)m_EventList.size() || pInfo == NULL)
		return false;
	*pInfo = m_EventList[Index];
	return true;
}

bool CEventGroupDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen < 1)
		return false;

	m_GroupType = pPayload[0] >> 4;
	const int EventCount = pPayload[0] & 0x0F;
	m_EventList.clear();
	if (m_GroupType != 0x04 && m_GroupType != 0x05) {
		int Pos = 1;
		if (Pos + EventCount * 4 > m_byDescLen)
			return false;
		for (int i = 0; i < EventCount; i++) {
			EventInfo Info;
			Info.ServiceID = ((WORD)pPayload[Pos + 0] << 8) | (WORD)pPayload[Pos + 1];
			Info.EventID   = ((WORD)pPayload[Pos + 2] << 8) | (WORD)pPayload[Pos + 3];
			Info.OriginalNetworkID = 0;
			Info.TransportStreamID = 0;
			m_EventList.push_back(Info);
			Pos += 4;
		}
	} else {
		if (EventCount != 0)
			return false;
		int Pos = 1;
		while (Pos + 8 <= m_byDescLen) {
			EventInfo Info;
			Info.OriginalNetworkID = ((WORD)pPayload[Pos + 0] << 8) | (WORD)pPayload[Pos + 1];
			Info.TransportStreamID = ((WORD)pPayload[Pos + 2] << 8) | (WORD)pPayload[Pos + 3];
			Info.ServiceID = ((WORD)pPayload[Pos + 4] << 8) | (WORD)pPayload[Pos + 5];
			Info.EventID   = ((WORD)pPayload[Pos + 6] << 8) | (WORD)pPayload[Pos + 7];
			m_EventList.push_back(Info);
			Pos += 8;
		}
	}
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0xD9] Component Group �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CComponentGroupDesc::CComponentGroupDesc()
{
	Reset();
}

void CComponentGroupDesc::Reset()
{
	m_ComponentGroupType = 0;
	m_bTotalBitRateFlag = false;
	m_GroupList.clear();
}

BYTE CComponentGroupDesc::GetComponentGroupType() const
{
	return m_ComponentGroupType;
}

bool CComponentGroupDesc::GetTotalBitRateFlag() const
{
	return m_bTotalBitRateFlag;
}

BYTE CComponentGroupDesc::GetGroupNum() const
{
	return static_cast<BYTE>(m_GroupList.size());
}

const CComponentGroupDesc::GroupInfo *CComponentGroupDesc::GetGroupInfo(const int Index) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_GroupList.size())
		return NULL;

	return &m_GroupList[Index];
}

bool CComponentGroupDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen < 1)
		return false;

	m_ComponentGroupType = pPayload[0] >> 5;
	m_bTotalBitRateFlag = (pPayload[0] & 0x10) != 0;

	const int GroupNum = pPayload[0] & 0x0F;

	m_GroupList.clear();
	m_GroupList.reserve(GroupNum);

	int Pos = 1;

	for (int i = 0; (i < GroupNum) && (Pos + 2 <= m_byDescLen); i++) {
		GroupInfo Group;

		Group.ComponentGroupID = pPayload[Pos] >> 4;
		Group.CAUnitNum = pPayload[Pos] & 0x0F;
		Pos++;

		for (int j = 0; j < Group.CAUnitNum; j++) {
			CAUnitInfo &CAUnit = Group.CAUnit[j];

			if (Pos >= m_byDescLen)
				return false;
			CAUnit.CAUnitID = pPayload[Pos] >> 4;
			CAUnit.ComponentNum = pPayload[Pos] & 0x0F;
			Pos++;
			if (Pos + CAUnit.ComponentNum > m_byDescLen)
				return false;
			::CopyMemory(CAUnit.ComponentTag, &pPayload[Pos], CAUnit.ComponentNum);
			Pos += CAUnit.ComponentNum;
		}

		if (m_bTotalBitRateFlag) {
			if (Pos >= m_byDescLen)
				return false;
			Group.TotalBitRate = pPayload[Pos++];
		} else {
			Group.TotalBitRate = 0;
		}

		if (Pos >= m_byDescLen)
			return false;
		const BYTE TextLength = pPayload[Pos++];
		Group.szText[0] = '\0';
		if (TextLength > 0) {
			if (Pos + TextLength > m_byDescLen)
				return false;
			CAribString::AribToString(Group.szText, _countof(Group.szText), &pPayload[Pos], TextLength);
			Pos += TextLength;
		}

		m_GroupList.push_back(Group);
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x58] Local Time Offset �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CLocalTimeOffsetDesc::CLocalTimeOffsetDesc()
{
	Reset();
}

void CLocalTimeOffsetDesc::Reset()
{
	CBaseDesc::Reset();

	::ZeroMemory(&m_Info, sizeof(TimeOffsetInfo));
}

bool CLocalTimeOffsetDesc::IsValid() const
{
	return m_Info.bValid
		&& m_Info.CountryCode == COUNTRYCODE_JPN
		&& m_Info.CountryRegionID == COUNTRYREGION_ALL;
}

DWORD CLocalTimeOffsetDesc::GetCountryCode() const
{
	return m_Info.CountryCode;
}

BYTE CLocalTimeOffsetDesc::GetCountryRegionID() const
{
	return m_Info.CountryRegionID;
}

int CLocalTimeOffsetDesc::GetLocalTimeOffset() const
{
	return m_Info.LocalTimeOffsetPolarity == 0 ? m_Info.LocalTimeOffset : -(int)m_Info.LocalTimeOffset;
}

bool CLocalTimeOffsetDesc::GetTimeOfChange(SYSTEMTIME *pTime) const
{
	if (pTime == NULL || !m_Info.bValid)
		return false;
	*pTime = m_Info.TimeOfChange;
	return true;
}

int CLocalTimeOffsetDesc::GetNextTimeOffset() const
{
	return m_Info.LocalTimeOffsetPolarity == 0 ? m_Info.NextTimeOffset : -(int)m_Info.NextTimeOffset;
}

bool CLocalTimeOffsetDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen < 13)
		return false;

	m_Info.bValid = true;
	m_Info.CountryCode = ((DWORD)pPayload[0] << 16) | ((DWORD)pPayload[1] << 8) | (DWORD)pPayload[2];
	m_Info.CountryRegionID = (pPayload[3] & 0xFC) >> 2;
	m_Info.LocalTimeOffsetPolarity = pPayload[3] & 0x01;
	m_Info.LocalTimeOffset = CAribTime::BcdHMToMinute(((WORD)pPayload[4] << 8) | (WORD)pPayload[5]);
	CAribTime::AribToSystemTime(&pPayload[6], &m_Info.TimeOfChange);
	m_Info.NextTimeOffset = CAribTime::BcdHMToMinute(((WORD)pPayload[11] << 8) | (WORD)pPayload[12]);
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0xC9] Download Content �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CDownloadContentDesc::CDownloadContentDesc()
{
	Reset();
}

void CDownloadContentDesc::Reset()
{
	CBaseDesc::Reset();

	::ZeroMemory(&m_Info, sizeof(m_Info));
}

bool CDownloadContentDesc::GetReboot() const
{
	return m_Info.bReboot;
}

bool CDownloadContentDesc::GetAddOn() const
{
	return m_Info.bAddOn;
}

DWORD CDownloadContentDesc::GetComponentSize() const
{
	return m_Info.ComponentSize;
}

DWORD CDownloadContentDesc::GetDownloadID() const
{
	return m_Info.DownloadID;
}

DWORD CDownloadContentDesc::GetTimeOutValueDII() const
{
	return m_Info.TimeOutValueDII;
}

DWORD CDownloadContentDesc::GetLeakRate() const
{
	return m_Info.LeakRate;
}

BYTE CDownloadContentDesc::GetComponentTag() const
{
	return m_Info.ComponentTag;
}

bool CDownloadContentDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen < 18)
		return false;

	m_Info.bReboot = (pPayload[0] & 0x80) != 0;
	m_Info.bAddOn = (pPayload[0] & 0x40) != 0;
	m_Info.bCompatibilityFlag = (pPayload[0] & 0x20) != 0;
	m_Info.bModuleInfoFlag = (pPayload[0] & 0x10) != 0;
	m_Info.bTextInfoFlag = (pPayload[0] & 0x80) != 0;
	m_Info.ComponentSize = MSBFirst32(&pPayload[1]);
	m_Info.DownloadID = MSBFirst32(&pPayload[5]);
	m_Info.TimeOutValueDII = MSBFirst32(&pPayload[9]);
	m_Info.LeakRate = ((DWORD)pPayload[13] << 14) | ((DWORD)pPayload[14] << 6) | (DWORD)(pPayload[15] >> 2);
	m_Info.ComponentTag = pPayload[16];

	// ���g�p�Ȃ̂łƂ肠�������
	/*
	if (m_Info.bCompatibilityFlag) {
	}

	if (m_Info.bModuleInfoFlag) {
	}

	//private_data_length
	//private_data_byte

	if (m_Info.bTextInfoFlag) {
	}
	*/

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0xCB] CA Contract Info �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CCaContractInfoDesc::CCaContractInfoDesc()
{
	Reset();
}

CCaContractInfoDesc::CCaContractInfoDesc(const CCaContractInfoDesc &Operand)
{
	*this = Operand;
}

CCaContractInfoDesc & CCaContractInfoDesc::operator = (const CCaContractInfoDesc &Operand)
{
	if (&Operand != this) {
		CBaseDesc::CopyDesc(&Operand);

		m_CaUnitID = Operand.m_CaUnitID;
		m_NumOfComponent = Operand.m_NumOfComponent;
		::CopyMemory(m_ComponentTag, Operand.m_ComponentTag, Operand.m_NumOfComponent);
		m_ContractVerificationInfoLength = Operand.m_ContractVerificationInfoLength;
		::CopyMemory(m_ContractVerificationInfo, Operand.m_ContractVerificationInfo,
					 Operand.m_ContractVerificationInfoLength);
		StdUtil::strncpy(m_szFeeName, _countof(m_szFeeName), Operand.m_szFeeName);
	}

	return *this;
}

void CCaContractInfoDesc::Reset()
{
	CBaseDesc::Reset();

	m_CaSystemID = 0x0000;
	m_CaUnitID = 0x0;
	m_NumOfComponent = 0;
	m_ContractVerificationInfoLength = 0;
	m_szFeeName[0] = _T('\0');
}

WORD CCaContractInfoDesc::GetCaSystemID() const
{
	return m_CaSystemID;
}

BYTE CCaContractInfoDesc::GetCaUnitID() const
{
	return m_CaUnitID;
}

BYTE CCaContractInfoDesc::GetNumOfComponent() const
{
	return m_NumOfComponent;
}

BYTE CCaContractInfoDesc::GetComponentTag(BYTE Index) const
{
	if (Index >= m_NumOfComponent)
		return 0x00;
	return m_ComponentTag[Index];
}

BYTE CCaContractInfoDesc::GetContractVerificationInfoLength() const
{
	return m_ContractVerificationInfoLength;
}

BYTE CCaContractInfoDesc::GetContractVerificationInfo(BYTE *pInfo, BYTE MaxLength) const
{
	if (!pInfo || MaxLength < m_ContractVerificationInfoLength)
		return 0;

	::CopyMemory(pInfo, m_ContractVerificationInfo, m_ContractVerificationInfoLength);

	return m_ContractVerificationInfoLength;
}

int CCaContractInfoDesc::GetFeeName(LPTSTR pszName, int MaxName) const
{
	if (pszName && MaxName > 0)
		::lstrcpyn(pszName, m_szFeeName, MaxName);

	return ::lstrlen(m_szFeeName);
}

bool CCaContractInfoDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen < 7)
		return false;

	m_CaSystemID = (pPayload[0] << 8) | pPayload[1];
	m_CaUnitID = pPayload[2] >> 4;
	if (m_CaUnitID == 0x0) {
		Reset();
		return false;
	}

	// Component Tag
	m_NumOfComponent = pPayload[2] & 0x0F;
	if (m_NumOfComponent == 0
			|| m_NumOfComponent > MAX_NUM_OF_COMPONENT
			|| m_byDescLen < 7 + m_NumOfComponent) {
		Reset();
		return false;
	}
	int Pos = 3;
	::CopyMemory(m_ComponentTag, &pPayload[Pos], m_NumOfComponent);
	Pos += m_NumOfComponent;

	// Contract Verification Info
	m_ContractVerificationInfoLength = pPayload[Pos++];
	if (m_ContractVerificationInfoLength > MAX_VERIFICATION_INFO_LENGTH
			|| m_byDescLen < Pos + m_ContractVerificationInfoLength + 1) {
		Reset();
		return false;
	}
	::CopyMemory(m_ContractVerificationInfo, &pPayload[Pos], m_ContractVerificationInfoLength);
	Pos += m_ContractVerificationInfoLength;

	// Fee Name
	const BYTE FeeNameLength = pPayload[Pos++];
	m_szFeeName[0] = _T('\0');
	if (FeeNameLength > 0) {
		if (m_byDescLen < Pos + FeeNameLength) {
			Reset();
			return false;
		}
		CAribString::AribToString(m_szFeeName, _countof(m_szFeeName), &pPayload[Pos], FeeNameLength);
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0xFB] ������M�L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CPartialReceptionDesc::CPartialReceptionDesc()
{
	Reset();
}

void CPartialReceptionDesc::Reset()
{
	CBaseDesc::Reset();

	m_ServiceNum = 0;
}

BYTE CPartialReceptionDesc::GetServiceNum() const
{
	return m_ServiceNum;
}

WORD CPartialReceptionDesc::GetServiceID(const BYTE Index) const
{
	if (Index >= m_ServiceNum)
		return 0;
	return m_ServiceList[Index];
}

bool CPartialReceptionDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG)
		return false;

	BYTE ServiceNum = m_byDescLen / 2;
	if (ServiceNum > 3)
		ServiceNum = 3;
	m_ServiceNum = ServiceNum;
	for (BYTE i = 0; i < ServiceNum; i++)
		m_ServiceList[i] = (pPayload[i * 2] << 8) | (pPayload[i * 2 + 1]);

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// �L�q�q�u���b�N���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CDescBlock::CDescBlock()
{

}

CDescBlock::CDescBlock(const CDescBlock &Operand)
{
	*this = Operand;
}

CDescBlock::~CDescBlock()
{
	Reset();
}

CDescBlock & CDescBlock::operator = (const CDescBlock &Operand)
{
	if (&Operand == this)
		return *this;

	// �C���X�^���X�̃R�s�[
	Reset();
	m_DescArray.resize(Operand.m_DescArray.size());

	for (size_t Index = 0 ; Index < m_DescArray.size() ; Index++) {
		m_DescArray[Index] = CreateDescInstance(Operand.m_DescArray[Index]->GetTag());
		m_DescArray[Index]->CopyDesc(Operand.m_DescArray[Index]);
	}

	return *this;
}

WORD CDescBlock::ParseBlock(const BYTE *pHexData, const WORD wDataLength)
{
	if (!pHexData || wDataLength < 2U)
		return 0U;

	// ��Ԃ��N���A
	Reset();

	// �w�肳�ꂽ�u���b�N�Ɋ܂܂��L�q�q����͂���
	WORD wPos = 0UL;

	do {
		CBaseDesc *pNewDesc;

		// �u���b�N����͂���
		if (!(pNewDesc = ParseDesc(&pHexData[wPos], wDataLength - wPos)))
			break;

		// ���X�g�ɒǉ�����
		m_DescArray.push_back(pNewDesc);

		// �ʒu�X�V
		wPos += (pNewDesc->GetLength() + 2U);
	} while (wPos + 2 <= wDataLength);

	return (WORD)m_DescArray.size();
}

const CBaseDesc * CDescBlock::ParseBlock(const BYTE *pHexData, const WORD wDataLength, const BYTE byTag)
{
	// �w�肳�ꂽ�u���b�N�Ɋ܂܂��L�q�q����͂��Ďw�肳�ꂽ�^�O�̋L�q�q��Ԃ�
	return (ParseBlock(pHexData, wDataLength))? GetDescByTag(byTag) : NULL;
}

void CDescBlock::Reset()
{
	// �S�ẴC���X�^���X���J������
	for (size_t Index = 0 ; Index < m_DescArray.size() ; Index++) {
		delete m_DescArray[Index];
	}

	m_DescArray.clear();
}

WORD CDescBlock::GetDescNum() const
{
	// �L�q�q�̐���Ԃ�
	return (WORD)m_DescArray.size();
}

const CBaseDesc * CDescBlock::GetDescByIndex(const WORD wIndex) const
{
	// �C���f�b�N�X�Ŏw�肵���L�q�q��Ԃ�
	return (wIndex < m_DescArray.size())? m_DescArray[wIndex] : NULL;
}

const CBaseDesc * CDescBlock::GetDescByTag(const BYTE byTag) const
{
	// �w�肵���^�O�Ɉ�v����L�q�q��Ԃ�
	for (size_t Index = 0 ; Index < m_DescArray.size() ; Index++){
		if (m_DescArray[Index]->GetTag() == byTag)
			return m_DescArray[Index];
	}

	return NULL;
}

CBaseDesc * CDescBlock::ParseDesc(const BYTE *pHexData, const WORD wDataLength)
{
	if (!pHexData || wDataLength < 2U)
		return NULL;

	// �^�O�ɑΉ������C���X�^���X�𐶐�����
	CBaseDesc *pNewDesc = CreateDescInstance(pHexData[0]);

	/*
	// �������s��
	if(!pNewDesc)return NULL;
	*/

	// �L�q�q����͂���
	if (!pNewDesc->ParseDesc(pHexData, wDataLength)) {
		// �G���[����
		delete pNewDesc;
		return NULL;
	}

	return pNewDesc;
}

CBaseDesc * CDescBlock::CreateDescInstance(const BYTE byTag)
{
	// �^�O�ɑΉ������C���X�^���X�𐶐�����
	switch (byTag) {
	case CCaMethodDesc::DESC_TAG					: return new CCaMethodDesc;
	case CServiceDesc::DESC_TAG						: return new CServiceDesc;
	case CShortEventDesc::DESC_TAG					: return new CShortEventDesc;
	case CExtendedEventDesc::DESC_TAG				: return new CExtendedEventDesc;
	case CStreamIdDesc::DESC_TAG					: return new CStreamIdDesc;
	case CHierarchicalTransmissionDesc::DESC_TAG	: return new CHierarchicalTransmissionDesc;
	case CNetworkNameDesc::DESC_TAG					: return new CNetworkNameDesc;
	case CServiceListDesc::DESC_TAG					: return new CServiceListDesc;
	case CSatelliteDeliverySystemDesc::DESC_TAG		: return new CSatelliteDeliverySystemDesc;
	case CTerrestrialDeliverySystemDesc::DESC_TAG	: return new CTerrestrialDeliverySystemDesc;
	case CSystemManageDesc::DESC_TAG				: return new CSystemManageDesc;
	case CTSInfoDesc::DESC_TAG						: return new CTSInfoDesc;
	case CComponentDesc::DESC_TAG					: return new CComponentDesc;
	case CAudioComponentDesc::DESC_TAG				: return new CAudioComponentDesc;
	case CContentDesc::DESC_TAG						: return new CContentDesc;
	case CLogoTransmissionDesc::DESC_TAG			: return new CLogoTransmissionDesc;
	case CSeriesDesc::DESC_TAG						: return new CSeriesDesc;
	case CEventGroupDesc::DESC_TAG					: return new CEventGroupDesc;
	case CComponentGroupDesc::DESC_TAG				: return new CComponentGroupDesc;
	case CLocalTimeOffsetDesc::DESC_TAG				: return new CLocalTimeOffsetDesc;
	case CDownloadContentDesc::DESC_TAG				: return new CDownloadContentDesc;
	case CCaContractInfoDesc::DESC_TAG				: return new CCaContractInfoDesc;
	case CPartialReceptionDesc::DESC_TAG			: return new CPartialReceptionDesc;
	default											: return new CBaseDesc;
	}
}
