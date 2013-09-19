// TsDescriptor.h: 記述子ラッパークラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Common.h"
#include "TsDescriptor.h"
#include "TsEncode.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


inline DWORD MSBFirst32(const BYTE *p)
{
	return ((DWORD)p[0] << 24) | ((DWORD)p[1] << 16) | ((DWORD)p[2] << 8) | (DWORD)p[3];
}


/////////////////////////////////////////////////////////////////////////////
// 記述子の基底クラス
/////////////////////////////////////////////////////////////////////////////

CBaseDesc::CBaseDesc()
{
	Reset();
}

CBaseDesc::CBaseDesc(const CBaseDesc &Operand)
{
	// コピーコンストラクタ
	CopyDesc(&Operand);
}

CBaseDesc::~CBaseDesc()
{

}

CBaseDesc & CBaseDesc::operator = (const CBaseDesc &Operand)
{
	// 代入演算子
	CopyDesc(&Operand);

	return *this;
}

void CBaseDesc::CopyDesc(const CBaseDesc *pOperand)
{
	// インスタンスのコピー
	m_byDescTag = pOperand->m_byDescTag;
	m_byDescLen = pOperand->m_byDescLen;
	m_bIsValid = pOperand->m_bIsValid;
}

const bool CBaseDesc::ParseDesc(const BYTE *pHexData, const WORD wDataLength)
{
	Reset();

	// 共通フォーマットをチェック
	if(!pHexData)return false;										// データが空
	else if(wDataLength < 2U)return false;							// データが最低記述子サイズ未満
	else if(wDataLength < (WORD)(pHexData[1] + 2U))return false;	// データが記述子のサイズよりも小さい

	m_byDescTag = pHexData[0];
	m_byDescLen = pHexData[1];

	// ペイロード解析
	if (m_byDescLen > 0 && StoreContents(&pHexData[2])) {
		m_bIsValid = true;
	}

	return m_bIsValid;
}

const bool CBaseDesc::IsValid(void) const
{
	// データが有効(解析済)かどうかを返す
	return m_bIsValid;
}

const BYTE CBaseDesc::GetTag(void) const
{
	// 記述子タグを返す
	return m_byDescTag;
}

const BYTE CBaseDesc::GetLength(void) const
{
	// 記述子長を返す
	return m_byDescLen;
}

void CBaseDesc::Reset(void)
{
	// 状態をクリアする
	m_byDescTag = 0x00U;
	m_byDescLen = 0U;
	m_bIsValid = false;
}

const bool CBaseDesc::StoreContents(const BYTE *pPayload)
{
	// デフォルトの実装では何もしない
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x09] Conditional Access 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CCaMethodDesc::CCaMethodDesc()
	: CBaseDesc()
{
	Reset();
}

CCaMethodDesc::CCaMethodDesc(const CCaMethodDesc &Operand)
{
	CopyDesc(&Operand);
}

CCaMethodDesc & CCaMethodDesc::operator = (const CCaMethodDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CCaMethodDesc::CopyDesc(const CBaseDesc *pOperand)
{
	// インスタンスのコピー
	CBaseDesc::CopyDesc(pOperand);

	const CCaMethodDesc *pSrcDesc = dynamic_cast<const CCaMethodDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_wCaMethodID = pSrcDesc->m_wCaMethodID;
		m_wCaPID = pSrcDesc->m_wCaPID;
		m_PrivateData = pSrcDesc->m_PrivateData;
	}
}

void CCaMethodDesc::Reset(void)
{
	CBaseDesc::Reset();

	m_wCaMethodID = 0x0000U;		// Conditional Access Method ID
	m_wCaPID = 0xFFFFU;				// Conditional Access PID
	m_PrivateData.ClearSize();		// Private Data
}

const WORD CCaMethodDesc::GetCaMethodID(void) const
{
	// Conditional Access Method ID を返す
	return m_wCaMethodID;
}

const WORD CCaMethodDesc::GetCaPID(void) const
{
	// Conditional Access PID
	return m_wCaPID;
}

const CMediaData * CCaMethodDesc::GetPrivateData(void) const
{
	// Private Data を返す
	return &m_PrivateData;
}

const bool CCaMethodDesc::StoreContents(const BYTE *pPayload)
{
	// フォーマットをチェック
	if(m_byDescTag != DESC_TAG)return false;							// タグが不正
	if(m_byDescLen < 4U)return false;								// CAメソッド記述子の最小サイズは4
	if((pPayload[2] & 0xE0U) != 0xE0U)return false;				// 固定ビットが不正

	// 記述子を解析
	m_wCaMethodID = ((WORD)pPayload[0] << 8) | (WORD)pPayload[1];			// +0,1	Conditional Access Method ID
	m_wCaPID = ((WORD)(pPayload[2] & 0x1FU) << 8) | (WORD)pPayload[3];	// +2,3	Conditional Access PID
	m_PrivateData.SetData(&pPayload[4], m_byDescLen - 4U);				// +4-	Private Data

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x48] Service 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CServiceDesc::CServiceDesc()
	: CBaseDesc()
{
	Reset();
}

CServiceDesc::CServiceDesc(const CServiceDesc &Operand)
{
	CopyDesc(&Operand);
}

CServiceDesc & CServiceDesc::operator = (const CServiceDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CServiceDesc::CopyDesc(const CBaseDesc *pOperand)
{
	// インスタンスのコピー
	CBaseDesc::CopyDesc(pOperand);

	const CServiceDesc *pSrcDesc = dynamic_cast<const CServiceDesc *>(pOperand);
	
	if (pSrcDesc && pSrcDesc != this) {
		m_byServiceType = pSrcDesc->m_byServiceType;
		::lstrcpy(m_szProviderName, pSrcDesc->m_szProviderName);
		::lstrcpy(m_szServiceName, pSrcDesc->m_szServiceName);
	}
}

void CServiceDesc::Reset(void)
{
	CBaseDesc::Reset();

	m_byServiceType = 0x00U;			// Service Type
	m_szProviderName[0] = TEXT('\0');	// Service Provider Name
	m_szServiceName[0] = TEXT('\0');	// Service Name
}

const BYTE CServiceDesc::GetServiceType(void) const
{
	// Service Typeを返す
	return m_byServiceType;
}

const DWORD CServiceDesc::GetProviderName(LPTSTR lpszDst, int MaxLength) const
{
	// Service Provider Nameを返す
	if (lpszDst && MaxLength > 0)
		::lstrcpyn(lpszDst, m_szProviderName, MaxLength);

	return ::lstrlen(m_szProviderName);
}

const DWORD CServiceDesc::GetServiceName(LPTSTR lpszDst, int MaxLength) const
{
	// Service Provider Nameを返す
	if (lpszDst && MaxLength > 0)
		::lstrcpyn(lpszDst, m_szServiceName, MaxLength);

	return ::lstrlen(m_szServiceName);
}

const bool CServiceDesc::StoreContents(const BYTE *pPayload)
{
	// フォーマットをチェック
	if(m_byDescTag != DESC_TAG)return false;	// タグが不正
	else if(m_byDescLen < 3U)return false;		// サービス記述子のサイズは最低3

	// 記述子を解析
	m_byServiceType = pPayload[0];				// +0	Service Type

	int Pos = 1, Length;

	// Provider Name
	Length = pPayload[Pos++];
	m_szProviderName[0] = '\0';
	if (Length > 0) {
		if (Pos + Length >= m_byDescLen)
			return false;
		CAribString::AribToString(m_szProviderName, sizeof(m_szProviderName) / sizeof(TCHAR), &pPayload[Pos], Length);
		Pos += Length;
	}

	// Service Name
	Length = pPayload[Pos++];
	m_szServiceName[0] = '\0';
	if (Length > 0) {
		if (Pos + Length > m_byDescLen)
			return false;
		CAribString::AribToString(m_szServiceName, sizeof(m_szServiceName) / sizeof(TCHAR), &pPayload[Pos], Length);
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x4D] Short Event 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CShortEventDesc::CShortEventDesc()
	: CBaseDesc()
{
	Reset();
}

CShortEventDesc::CShortEventDesc(const CShortEventDesc &Operand)
{
	CopyDesc(&Operand);
}

CShortEventDesc & CShortEventDesc::operator = (const CShortEventDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CShortEventDesc::CopyDesc(const CBaseDesc *pOperand)
{
	// インスタンスのコピー
	CBaseDesc::CopyDesc(pOperand);

	const CShortEventDesc *pSrcDesc = dynamic_cast<const CShortEventDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_dwLanguageCode = pSrcDesc->m_dwLanguageCode;
		::lstrcpy(m_szEventName, pSrcDesc->m_szEventName);
		::lstrcpy(m_szEventDesc, pSrcDesc->m_szEventDesc);
	}
}

void CShortEventDesc::Reset(void)
{
	CBaseDesc::Reset();

	m_dwLanguageCode = 0UL;			// ISO639  Language Code
	m_szEventName[0] = TEXT('\0');	// Event Name
	m_szEventDesc[0] = TEXT('\0');	// Event Description
}

const DWORD CShortEventDesc::GetLanguageCode(void) const
{
	// Language Codeを返す
	return m_dwLanguageCode;
}

const DWORD CShortEventDesc::GetEventName(LPTSTR lpszDst, int MaxLength) const
{
	// Event Nameを返す
	if (lpszDst && MaxLength > 0)
		::lstrcpyn(lpszDst, m_szEventName, MaxLength);

	return ::lstrlen(m_szEventName);
}

const DWORD CShortEventDesc::GetEventDesc(LPTSTR lpszDst, int MaxLength) const
{
	// Event Descriptionを返す
	if (lpszDst && MaxLength > 0)
		::lstrcpyn(lpszDst, m_szEventDesc, MaxLength);

	return ::lstrlen(m_szEventDesc);
}

const bool CShortEventDesc::StoreContents(const BYTE *pPayload)
{
	// フォーマットをチェック
	if(m_byDescTag != DESC_TAG)return false;	// タグが不正
	else if(m_byDescLen < 5U)return false;		// Short Event記述子のサイズは最低5

	// 記述子を解析
	m_dwLanguageCode = ((DWORD)pPayload[0] << 16) | ((DWORD)pPayload[1] << 8) | (DWORD)pPayload[2];		// +0 - +2	ISO639  Language Code

	int Pos = 3, Length;

	// Event Name
	Length = pPayload[Pos++];
	m_szEventName[0] = '\0';
	if (Length > 0) {
		if (Pos + Length >= m_byDescLen)
			return false;
		CAribString::AribToString(m_szEventName, sizeof(m_szEventName) / sizeof(TCHAR), &pPayload[Pos], Length);
		Pos += Length;
	}

	// Event Description
	Length = pPayload[Pos++];
	m_szEventDesc[0] = '\0';
	if (Length > 0) {
		if (Pos + Length > m_byDescLen)
			return false;
		CAribString::AribToString(m_szEventDesc, sizeof(m_szEventDesc) / sizeof(TCHAR), &pPayload[Pos], Length);
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x4E] Extended Event 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CExtendedEventDesc::CExtendedEventDesc()
	: CBaseDesc()
{
	Reset();
}

CExtendedEventDesc::CExtendedEventDesc(const CExtendedEventDesc &Operand)
{
	CopyDesc(&Operand);
}

CExtendedEventDesc & CExtendedEventDesc::operator = (const CExtendedEventDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CExtendedEventDesc::CopyDesc(const CBaseDesc *pOperand)
{
	// インスタンスのコピー
	CBaseDesc::CopyDesc(pOperand);

	const CExtendedEventDesc *pSrcDesc = dynamic_cast<const CExtendedEventDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_DescriptorNumber = pSrcDesc->m_DescriptorNumber;
		m_LastDescriptorNumber = pSrcDesc->m_LastDescriptorNumber;
		m_LanguageCode = pSrcDesc->m_LanguageCode;
		m_ItemList = pSrcDesc->m_ItemList;
	}
}

void CExtendedEventDesc::Reset(void)
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

DWORD CExtendedEventDesc::GetLanguageCode(void) const
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

const bool CExtendedEventDesc::StoreContents(const BYTE *pPayload)
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
// [0x52] Stream Identifier 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CStreamIdDesc::CStreamIdDesc()
	: CBaseDesc()
{
	Reset();
}

CStreamIdDesc::CStreamIdDesc(const CStreamIdDesc &Operand)
{
	CopyDesc(&Operand);
}

CStreamIdDesc & CStreamIdDesc::operator = (const CStreamIdDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CStreamIdDesc::CopyDesc(const CBaseDesc *pOperand)
{
	// インスタンスのコピー
	CBaseDesc::CopyDesc(pOperand);

	const CStreamIdDesc *pSrcDesc = dynamic_cast<const CStreamIdDesc *>(pOperand);

	if (pSrcDesc) {
		m_byComponentTag = pSrcDesc->m_byComponentTag;
	}
}

void CStreamIdDesc::Reset(void)
{
	CBaseDesc::Reset();

	m_byComponentTag = 0x00U;	// Component Tag
}

const BYTE CStreamIdDesc::GetComponentTag(void) const
{
	// Component Tag を返す
	return m_byComponentTag;
}

const bool CStreamIdDesc::StoreContents(const BYTE *pPayload)
{
	// フォーマットをチェック
	if(m_byDescTag != DESC_TAG)return false;	// タグが不正
	else if(m_byDescLen != 1U)return false;		// ストリームID記述子のサイズは常に1

	// 記述子を解析
	m_byComponentTag = pPayload[0];				// +0	Component Tag

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x40] Network Name 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CNetworkNameDesc::CNetworkNameDesc()
	: CBaseDesc()
{
	Reset();
}

CNetworkNameDesc::CNetworkNameDesc(const CNetworkNameDesc &Operand)
{
	CopyDesc(&Operand);
}

CNetworkNameDesc & CNetworkNameDesc::operator = (const CNetworkNameDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CNetworkNameDesc::CopyDesc(const CBaseDesc *pOperand)
{
	CBaseDesc::CopyDesc(pOperand);

	const CNetworkNameDesc *pSrcDesc = dynamic_cast<const CNetworkNameDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		::lstrcpy(m_szNetworkName, pSrcDesc->m_szNetworkName);
	}
}

void CNetworkNameDesc::Reset(void)
{
	CBaseDesc::Reset();
	m_szNetworkName[0] = '\0';
}

const DWORD CNetworkNameDesc::GetNetworkName(LPTSTR pszName, int MaxLength) const
{
	if (pszName && MaxLength > 0)
		::lstrcpyn(pszName, m_szNetworkName, MaxLength);
	return ::lstrlen(m_szNetworkName);
}

const bool CNetworkNameDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG)
		return false;

	m_szNetworkName[0] = '\0';
	CAribString::AribToString(m_szNetworkName, sizeof(m_szNetworkName) / sizeof(TCHAR), &pPayload[0], m_byDescLen);

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x41] Service List 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CServiceListDesc::CServiceListDesc()
	: CBaseDesc()
{
	Reset();
}

CServiceListDesc::CServiceListDesc(const CServiceListDesc &Operand)
{
	CopyDesc(&Operand);
}

CServiceListDesc & CServiceListDesc::operator = (const CServiceListDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CServiceListDesc::CopyDesc(const CBaseDesc *pOperand)
{
	CBaseDesc::CopyDesc(pOperand);

	const CServiceListDesc *pSrcDesc = dynamic_cast<const CServiceListDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_ServiceList = pSrcDesc->m_ServiceList;
	}
}

void CServiceListDesc::Reset(void)
{
	CBaseDesc::Reset();
	m_ServiceList.clear();
}

const int CServiceListDesc::GetServiceNum() const
{
	return (int)m_ServiceList.size();
}

const int CServiceListDesc::GetServiceIndexByID(const WORD ServiceID) const
{
	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (m_ServiceList[i].ServiceID == ServiceID)
			return (int)i;
	}
	return -1;
}

const BYTE CServiceListDesc::GetServiceTypeByID(const WORD ServiceID) const
{
	int Index = GetServiceIndexByID(ServiceID);
	if (Index >= 0)
		return m_ServiceList[Index].ServiceType;
	return SERVICE_TYPE_INVALID;
}

const bool CServiceListDesc::GetServiceInfo(const int Index, ServiceInfo *pInfo) const
{
	if (!pInfo || Index < 0 || (size_t)Index >= m_ServiceList.size())
		return false;

	*pInfo = m_ServiceList[Index];

	return true;
}

const bool CServiceListDesc::StoreContents(const BYTE *pPayload)
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
// [0x43] Satellite Delivery System 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CSatelliteDeliverySystemDesc::CSatelliteDeliverySystemDesc()
	: CBaseDesc()
{
	Reset();
}

CSatelliteDeliverySystemDesc::CSatelliteDeliverySystemDesc(const CSatelliteDeliverySystemDesc &Operand)
{
	CopyDesc(&Operand);
}

CSatelliteDeliverySystemDesc & CSatelliteDeliverySystemDesc::operator = (const CSatelliteDeliverySystemDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CSatelliteDeliverySystemDesc::CopyDesc(const CBaseDesc *pOperand)
{
	CBaseDesc::CopyDesc(pOperand);

	const CSatelliteDeliverySystemDesc *pSrcDesc = dynamic_cast<const CSatelliteDeliverySystemDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_Frequency = pSrcDesc->m_Frequency;
		m_OrbitalPosition = pSrcDesc->m_OrbitalPosition;
		m_bWestEastFlag = pSrcDesc->m_bWestEastFlag;
		m_Polarization = pSrcDesc->m_Polarization;
		m_Modulation = pSrcDesc->m_Modulation;
		m_SymbolRate = pSrcDesc->m_SymbolRate;
		m_FECInner = pSrcDesc->m_FECInner;
	}
}

void CSatelliteDeliverySystemDesc::Reset(void)
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

const DWORD CSatelliteDeliverySystemDesc::GetFrequency() const
{
	return m_Frequency;
}

const WORD CSatelliteDeliverySystemDesc::GetOrbitalPosition() const
{
	return m_OrbitalPosition;
}

const bool CSatelliteDeliverySystemDesc::GetWestEastFlag() const
{
	return m_bWestEastFlag;
}

const BYTE CSatelliteDeliverySystemDesc::GetPolarization() const
{
	return m_Polarization;
}

const BYTE CSatelliteDeliverySystemDesc::GetModulation() const
{
	return m_Modulation;
}

const DWORD CSatelliteDeliverySystemDesc::GetSymbolRate() const
{
	return m_SymbolRate;
}

const BYTE CSatelliteDeliverySystemDesc::GetFECInner() const
{
	return m_FECInner;
}

static const DWORD GetBCD(const BYTE *pData, const int Length)
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

const bool CSatelliteDeliverySystemDesc::StoreContents(const BYTE *pPayload)
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
// [0xFA] Terrestrial Delivery System 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CTerrestrialDeliverySystemDesc::CTerrestrialDeliverySystemDesc()
	: CBaseDesc()
{
	Reset();
}

CTerrestrialDeliverySystemDesc::CTerrestrialDeliverySystemDesc(const CTerrestrialDeliverySystemDesc &Operand)
{
	CopyDesc(&Operand);
}

CTerrestrialDeliverySystemDesc & CTerrestrialDeliverySystemDesc::operator = (const CTerrestrialDeliverySystemDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CTerrestrialDeliverySystemDesc::CopyDesc(const CBaseDesc *pOperand)
{
	CBaseDesc::CopyDesc(pOperand);

	const CTerrestrialDeliverySystemDesc *pSrcDesc = dynamic_cast<const CTerrestrialDeliverySystemDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_AreaCode = pSrcDesc->m_AreaCode;
		m_GuardInterval = pSrcDesc->m_GuardInterval;
		m_TransmissionMode = pSrcDesc->m_TransmissionMode;
		m_Frequency = pSrcDesc->m_Frequency;
	}
}

void CTerrestrialDeliverySystemDesc::Reset(void)
{
	CBaseDesc::Reset();

	m_AreaCode = 0;
	m_GuardInterval = 0xFF;
	m_TransmissionMode = 0xFF;
	m_Frequency.clear();
}

const WORD CTerrestrialDeliverySystemDesc::GetAreaCode() const
{
	return m_AreaCode;
}

const BYTE CTerrestrialDeliverySystemDesc::GetGuardInterval() const
{
	return m_GuardInterval;
}

const BYTE CTerrestrialDeliverySystemDesc::GetTransmissionMode() const
{
	return m_TransmissionMode;
}

const int CTerrestrialDeliverySystemDesc::GetFrequencyNum() const
{
	return (int)m_Frequency.size();
}

const WORD CTerrestrialDeliverySystemDesc::GetFrequency(const int Index) const
{
	if (Index < 0 || (size_t)Index >= m_Frequency.size())
		return 0;
	return m_Frequency[Index];
}

const bool CTerrestrialDeliverySystemDesc::StoreContents(const BYTE *pPayload)
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
// [0xFE] System Management 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CSystemManageDesc::CSystemManageDesc()
	: CBaseDesc()
{
	Reset();
}

CSystemManageDesc::CSystemManageDesc(const CSystemManageDesc &Operand)
{
	CopyDesc(&Operand);
}

CSystemManageDesc & CSystemManageDesc::operator = (const CSystemManageDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CSystemManageDesc::CopyDesc(const CBaseDesc *pOperand)
{
	CBaseDesc::CopyDesc(pOperand);

	const CSystemManageDesc *pSrcDesc = dynamic_cast<const CSystemManageDesc *>(pOperand);

	if (pSrcDesc) {
		m_byBroadcastingFlag = pSrcDesc->m_byBroadcastingFlag;
		m_byBroadcastingID = pSrcDesc->m_byBroadcastingID;
		m_byAdditionalBroadcastingID = pSrcDesc->m_byAdditionalBroadcastingID;
	}
}

void CSystemManageDesc::Reset(void)
{
	CBaseDesc::Reset();
	m_byBroadcastingFlag = 0;
	m_byBroadcastingID = 0;
	m_byAdditionalBroadcastingID = 0;
}

const BYTE CSystemManageDesc::GetBroadcastingFlag(void) const
{
	return m_byBroadcastingFlag;
}

const BYTE CSystemManageDesc::GetBroadcastingID(void) const
{
	return m_byBroadcastingID;
}

const BYTE CSystemManageDesc::GetAdditionalBroadcastingID(void) const
{
	return m_byAdditionalBroadcastingID;
}

const bool CSystemManageDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen < 2)
		return false;

	m_byBroadcastingFlag = (pPayload[0] & 0xC0) >> 6;
	m_byBroadcastingID = (pPayload[0] & 0x3F);
	m_byAdditionalBroadcastingID = pPayload[1];

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0xCD] TS Information 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CTSInfoDesc::CTSInfoDesc()
	: CBaseDesc()
{
	Reset();
}

CTSInfoDesc::CTSInfoDesc(const CTSInfoDesc &Operand)
{
	CopyDesc(&Operand);
}

CTSInfoDesc & CTSInfoDesc::operator = (const CTSInfoDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CTSInfoDesc::CopyDesc(const CBaseDesc *pOperand)
{
	CBaseDesc::CopyDesc(pOperand);

	const CTSInfoDesc *pSrcDesc = dynamic_cast<const CTSInfoDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_byRemoteControlKeyID = pSrcDesc->m_byRemoteControlKeyID;
		::lstrcpy(m_szTSName, pSrcDesc->m_szTSName);
	}
}

void CTSInfoDesc::Reset(void)
{
	CBaseDesc::Reset();
	m_byRemoteControlKeyID = 0;
	m_szTSName[0] = '\0';
}

const BYTE CTSInfoDesc::GetRemoteControlKeyID(void) const
{
	return m_byRemoteControlKeyID;
}

const DWORD CTSInfoDesc::GetTSName(LPTSTR pszName, int MaxLength) const
{
	if (pszName && MaxLength > 0)
		::lstrcpyn(pszName, m_szTSName, MaxLength);
	return ::lstrlen(m_szTSName);
}

const bool CTSInfoDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen < 2)
		return false;

	m_byRemoteControlKeyID = pPayload[0];

	BYTE Length = pPayload[1] >> 2;
	if (2 + Length > m_byDescLen)
		return false;

	m_szTSName[0] = '\0';
	CAribString::AribToString(m_szTSName, sizeof(m_szTSName) / sizeof(TCHAR), &pPayload[2], Length);

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x50] Component 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CComponentDesc::CComponentDesc()
	: CBaseDesc()
{
	Reset();
}

CComponentDesc::CComponentDesc(const CComponentDesc &Operand)
{
	CopyDesc(&Operand);
}

CComponentDesc & CComponentDesc::operator = (const CComponentDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CComponentDesc::CopyDesc(const CBaseDesc *pOperand)
{
	CBaseDesc::CopyDesc(pOperand);

	const CComponentDesc *pSrcDesc = dynamic_cast<const CComponentDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_StreamContent = pSrcDesc->m_StreamContent;
		m_ComponentType = pSrcDesc->m_ComponentType;
		m_ComponentTag = pSrcDesc->m_ComponentTag;
		m_LanguageCode = pSrcDesc->m_LanguageCode;
		::lstrcpy(m_szText, pSrcDesc->m_szText);
	}
}

void CComponentDesc::Reset(void)
{
	CBaseDesc::Reset();

	m_StreamContent = 0;
	m_ComponentType = 0;
	m_ComponentTag = 0;
	m_LanguageCode = 0;
	m_szText[0] = '\0';
}

const BYTE CComponentDesc::GetStreamContent(void) const
{
	return m_StreamContent;
}

const BYTE CComponentDesc::GetComponentType(void) const
{
	return m_ComponentType;
}

const BYTE CComponentDesc::GetComponentTag(void) const
{
	return m_ComponentTag;
}

const DWORD CComponentDesc::GetLanguageCode(void) const
{
	return m_LanguageCode;
}

const DWORD CComponentDesc::GetText(LPTSTR pszText, int MaxLength) const
{
	if (pszText && MaxLength > 0)
		::lstrcpyn(pszText, m_szText, MaxLength);
	return ::lstrlen(m_szText);
}

const bool CComponentDesc::StoreContents(const BYTE *pPayload)
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
		CAribString::AribToString(m_szText, sizeof(m_szText) / sizeof(TCHAR), &pPayload[6], min(m_byDescLen - 6, 16));
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0xC4] Audio Component 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CAudioComponentDesc::CAudioComponentDesc()
	: CBaseDesc()
{
	Reset();
}

CAudioComponentDesc::CAudioComponentDesc(const CAudioComponentDesc &Operand)
{
	CopyDesc(&Operand);
}

CAudioComponentDesc & CAudioComponentDesc::operator = (const CAudioComponentDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CAudioComponentDesc::CopyDesc(const CBaseDesc *pOperand)
{
	CBaseDesc::CopyDesc(pOperand);

	const CAudioComponentDesc *pSrcDesc = dynamic_cast<const CAudioComponentDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_StreamContent = pSrcDesc->m_StreamContent;
		m_ComponentType = pSrcDesc->m_ComponentType;
		m_ComponentTag = pSrcDesc->m_ComponentTag;
		m_StreamType = pSrcDesc->m_StreamType;
		m_SimulcastGroupTag = pSrcDesc->m_SimulcastGroupTag;
		m_bESMultiLingualFlag = pSrcDesc->m_bESMultiLingualFlag;
		m_bMainComponentFlag = pSrcDesc->m_bMainComponentFlag;
		m_QualityIndicator = pSrcDesc->m_QualityIndicator;
		m_SamplingRate = pSrcDesc->m_SamplingRate;
		m_LanguageCode = pSrcDesc->m_LanguageCode;
		m_LanguageCode2 = pSrcDesc->m_LanguageCode2;
		::lstrcpy(m_szText, pSrcDesc->m_szText);
	}
}

void CAudioComponentDesc::Reset(void)
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

const BYTE CAudioComponentDesc::GetStreamContent(void) const
{
	return m_StreamContent;
}

const BYTE CAudioComponentDesc::GetComponentType(void) const
{
	return m_ComponentType;
}

const BYTE CAudioComponentDesc::GetComponentTag(void) const
{
	return m_ComponentTag;
}

const BYTE CAudioComponentDesc::GetSimulcastGroupTag(void) const
{
	return m_SimulcastGroupTag;
}

const bool CAudioComponentDesc::GetESMultiLingualFlag(void) const
{
	return m_bESMultiLingualFlag;
}

const bool CAudioComponentDesc::GetMainComponentFlag(void) const
{
	return m_bMainComponentFlag;
}

const BYTE CAudioComponentDesc::GetQualityIndicator(void) const
{
	return m_QualityIndicator;
}

const BYTE CAudioComponentDesc::GetSamplingRate(void) const
{
	return m_SamplingRate;
}

const DWORD CAudioComponentDesc::GetLanguageCode(void) const
{
	return m_LanguageCode;
}

const DWORD CAudioComponentDesc::GetLanguageCode2(void) const
{
	return m_LanguageCode2;
}

const DWORD CAudioComponentDesc::GetText(LPTSTR pszText, int MaxLength) const
{
	if (pszText && MaxLength > 0)
		::lstrcpyn(pszText, m_szText, MaxLength);
	return ::lstrlen(m_szText);
}

const bool CAudioComponentDesc::StoreContents(const BYTE *pPayload)
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
		CAribString::AribToString(m_szText, sizeof(m_szText) / sizeof(TCHAR), &pPayload[Pos], min(m_byDescLen - Pos, 33));
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x54] Content 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CContentDesc::CContentDesc()
	: CBaseDesc()
{
	Reset();
}

CContentDesc::CContentDesc(const CContentDesc &Operand)
{
	CopyDesc(&Operand);
}

CContentDesc & CContentDesc::operator = (const CContentDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CContentDesc::CopyDesc(const CBaseDesc *pOperand)
{
	CBaseDesc::CopyDesc(pOperand);

	const CContentDesc *pSrcDesc = dynamic_cast<const CContentDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_NibbleCount = pSrcDesc->m_NibbleCount;
		::CopyMemory(m_NibbleList, pSrcDesc->m_NibbleList, pSrcDesc->m_NibbleCount * sizeof(Nibble));
	}
}

void CContentDesc::Reset(void)
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

const bool CContentDesc::StoreContents(const BYTE *pPayload)
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
// [0xCF] Logo Transmission 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CLogoTransmissionDesc::CLogoTransmissionDesc()
	: CBaseDesc()
{
	Reset();
}

CLogoTransmissionDesc::CLogoTransmissionDesc(const CLogoTransmissionDesc &Operand)
{
	CopyDesc(&Operand);
}

CLogoTransmissionDesc & CLogoTransmissionDesc::operator = (const CLogoTransmissionDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CLogoTransmissionDesc::CopyDesc(const CBaseDesc *pOperand)
{
	CBaseDesc::CopyDesc(pOperand);

	const CLogoTransmissionDesc *pSrcDesc = dynamic_cast<const CLogoTransmissionDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_LogoTransmissionType = pSrcDesc->m_LogoTransmissionType;
		m_LogoID = pSrcDesc->m_LogoID;
		m_LogoVersion = pSrcDesc->m_LogoVersion;
		m_DownloadDataID = pSrcDesc->m_DownloadDataID;
		::lstrcpyA(m_LogoChar, pSrcDesc->m_LogoChar);
	}
}

void CLogoTransmissionDesc::Reset(void)
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

const bool CLogoTransmissionDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen < 1)
		return false;

	m_LogoTransmissionType = pPayload[0];
	m_LogoID = LOGOID_INVALID;
	m_LogoVersion = LOGOVERSION_INVALID;
	m_DownloadDataID = DATAID_INVALID;
	m_LogoChar[0] = '\0';
	if (m_LogoTransmissionType == 0x01) {
		// CDT伝送方式1
		if (m_byDescLen < 7)
			return false;
		m_LogoID = ((WORD)(pPayload[1] & 0x01) << 8) | (WORD)pPayload[2];
		m_LogoVersion = ((WORD)(pPayload[3] & 0x0F) << 8) | (WORD) pPayload[4];
		m_DownloadDataID = ((WORD)pPayload[5] << 8) | (WORD)pPayload[6];
	} else if (m_LogoTransmissionType == 0x02) {
		// CDT伝送方式2
		if (m_byDescLen < 3)
			return false;
		m_LogoID = ((WORD)(pPayload[1] & 0x01) << 8) | (WORD)pPayload[2];
	} else if (m_LogoTransmissionType == 0x03) {
		// 簡易ロゴ方式
		int i;
		for (i = 0; i < (int)m_byDescLen - 1 && i < MAX_LOGO_CHAR - 1; i++) {
			m_LogoChar[i] = pPayload[1 + i];
		}
		m_LogoChar[i] = '\0';
	}
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0xD5] Series 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CSeriesDesc::CSeriesDesc()
	: CBaseDesc()
{
	Reset();
}

CSeriesDesc::CSeriesDesc(const CSeriesDesc &Operand)
{
	CopyDesc(&Operand);
}

CSeriesDesc & CSeriesDesc::operator = (const CSeriesDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CSeriesDesc::CopyDesc(const CBaseDesc *pOperand)
{
	CBaseDesc::CopyDesc(pOperand);

	const CSeriesDesc *pSrcDesc = dynamic_cast<const CSeriesDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_SeriesID = pSrcDesc->m_SeriesID;
		m_RepeatLabel = pSrcDesc->m_RepeatLabel;
		m_ProgramPattern = pSrcDesc->m_ProgramPattern;
		m_bExpireDateValidFlag = pSrcDesc->m_bExpireDateValidFlag;
		m_ExpireDate = pSrcDesc->m_ExpireDate;
		m_EpisodeNumber = pSrcDesc->m_EpisodeNumber;
		m_LastEpisodeNumber = pSrcDesc->m_LastEpisodeNumber;
		::lstrcpy(m_szSeriesName, pSrcDesc->m_szSeriesName);
	}
}

void CSeriesDesc::Reset(void)
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

const bool CSeriesDesc::StoreContents(const BYTE *pPayload)
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
		CAribString::AribToString(m_szSeriesName, MAX_SERIES_NAME, &pPayload[8], m_byDescLen - 8);
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0xD6] Event Group 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CEventGroupDesc::CEventGroupDesc()
	: CBaseDesc()
{
	Reset();
}

CEventGroupDesc::CEventGroupDesc(const CEventGroupDesc &Operand)
{
	CopyDesc(&Operand);
}

CEventGroupDesc & CEventGroupDesc::operator = (const CEventGroupDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CEventGroupDesc::CopyDesc(const CBaseDesc *pOperand)
{
	CBaseDesc::CopyDesc(pOperand);

	const CEventGroupDesc *pSrcDesc = dynamic_cast<const CEventGroupDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_GroupType = pSrcDesc->m_GroupType;
		m_EventList = pSrcDesc->m_EventList;
	}
}

void CEventGroupDesc::Reset(void)
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

const bool CEventGroupDesc::StoreContents(const BYTE *pPayload)
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
// [0x58] Local Time Offset 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CLocalTimeOffsetDesc::CLocalTimeOffsetDesc()
{
	Reset();
}

CLocalTimeOffsetDesc::CLocalTimeOffsetDesc(const CLocalTimeOffsetDesc &Operand)
{
	CopyDesc(&Operand);
}

CLocalTimeOffsetDesc & CLocalTimeOffsetDesc::operator = (const CLocalTimeOffsetDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CLocalTimeOffsetDesc::CopyDesc(const CBaseDesc *pOperand)
{
	CBaseDesc::CopyDesc(pOperand);

	const CLocalTimeOffsetDesc *pSrcDesc = dynamic_cast<const CLocalTimeOffsetDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_Info = pSrcDesc->m_Info;
	}
}

void CLocalTimeOffsetDesc::Reset(void)
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

const bool CLocalTimeOffsetDesc::StoreContents(const BYTE *pPayload)
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
// [0xC9] Download Content 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CDownloadContentDesc::CDownloadContentDesc()
{
	Reset();
}

CDownloadContentDesc::CDownloadContentDesc(const CDownloadContentDesc &Operand)
{
	CopyDesc(&Operand);
}

CDownloadContentDesc & CDownloadContentDesc::operator = (const CDownloadContentDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CDownloadContentDesc::CopyDesc(const CBaseDesc *pOperand)
{
	CBaseDesc::CopyDesc(pOperand);

	const CDownloadContentDesc *pSrcDesc = dynamic_cast<const CDownloadContentDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_Info = pSrcDesc->m_Info;
	}
}

void CDownloadContentDesc::Reset(void)
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

const bool CDownloadContentDesc::StoreContents(const BYTE *pPayload)
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

	// 未使用なのでとりあえず後回し
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
// [0xCB] CA Contract Info 記述子抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CCaContractInfoDesc::CCaContractInfoDesc()
{
	Reset();
}

CCaContractInfoDesc::CCaContractInfoDesc(const CCaContractInfoDesc &Operand)
{
	CopyDesc(&Operand);
}

CCaContractInfoDesc & CCaContractInfoDesc::operator = (const CCaContractInfoDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CCaContractInfoDesc::CopyDesc(const CBaseDesc *pOperand)
{
	CBaseDesc::CopyDesc(pOperand);

	const CCaContractInfoDesc *pSrcDesc = dynamic_cast<const CCaContractInfoDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_CaUnitID = pSrcDesc->m_CaUnitID;
		m_NumOfComponent = pSrcDesc->m_NumOfComponent;
		::CopyMemory(m_ComponentTag, pSrcDesc->m_ComponentTag, pSrcDesc->m_NumOfComponent);
		m_ContractVerificationInfoLength = pSrcDesc->m_ContractVerificationInfoLength;
		::CopyMemory(m_ContractVerificationInfo, pSrcDesc->m_ContractVerificationInfo,
					 pSrcDesc->m_ContractVerificationInfoLength);
		::lstrcpyn(m_szFeeName, pSrcDesc->m_szFeeName, MAX_FEE_NAME);
	}
}

void CCaContractInfoDesc::Reset(void)
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

const bool CCaContractInfoDesc::StoreContents(const BYTE *pPayload)
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
		CAribString::AribToString(m_szFeeName, MAX_FEE_NAME, &pPayload[Pos], FeeNameLength);
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// 記述子ブロック抽象化クラス
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

	// インスタンスのコピー
	Reset();
	m_DescArray.resize(Operand.m_DescArray.size());

	for (size_t Index = 0 ; Index < m_DescArray.size() ; Index++) {
		m_DescArray[Index] = CreateDescInstance(Operand.m_DescArray[Index]->GetTag());
		m_DescArray[Index]->CopyDesc(Operand.m_DescArray[Index]);
	}

	return *this;
}

const WORD CDescBlock::ParseBlock(const BYTE *pHexData, const WORD wDataLength)
{
	if (!pHexData || wDataLength < 2U)
		return 0U;

	// 状態をクリア
	Reset();

	// 指定されたブロックに含まれる記述子を解析する
	WORD wPos = 0UL;

	do {
		CBaseDesc *pNewDesc;

		// ブロックを解析する
		if (!(pNewDesc = ParseDesc(&pHexData[wPos], wDataLength - wPos)))
			break;

		// リストに追加する
		m_DescArray.push_back(pNewDesc);

		// 位置更新
		wPos += (pNewDesc->GetLength() + 2U);
	} while (wPos + 2 <= wDataLength);

	return (WORD)m_DescArray.size();
}

const CBaseDesc * CDescBlock::ParseBlock(const BYTE *pHexData, const WORD wDataLength, const BYTE byTag)
{
	// 指定されたブロックに含まれる記述子を解析して指定されたタグの記述子を返す
	return (ParseBlock(pHexData, wDataLength))? GetDescByTag(byTag) : NULL;
}

void CDescBlock::Reset(void)
{
	// 全てのインスタンスを開放する
	for (size_t Index = 0 ; Index < m_DescArray.size() ; Index++) {
		delete m_DescArray[Index];
	}

	m_DescArray.clear();
}

const WORD CDescBlock::GetDescNum(void) const
{
	// 記述子の数を返す
	return (WORD)m_DescArray.size();
}

const CBaseDesc * CDescBlock::GetDescByIndex(const WORD wIndex) const
{
	// インデックスで指定した記述子を返す
	return (wIndex < m_DescArray.size())? m_DescArray[wIndex] : NULL;
}

const CBaseDesc * CDescBlock::GetDescByTag(const BYTE byTag) const
{
	// 指定したタグに一致する記述子を返す
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

	// タグに対応したインスタンスを生成する
	CBaseDesc *pNewDesc = CreateDescInstance(pHexData[0]);

	/*
	// メモリ不足
	if(!pNewDesc)return NULL;
	*/

	// 記述子を解析する
	if (!pNewDesc->ParseDesc(pHexData, wDataLength)) {
		// エラーあり
		delete pNewDesc;
		return NULL;
	}

	return pNewDesc;
}

CBaseDesc * CDescBlock::CreateDescInstance(const BYTE byTag)
{
	// タグに対応したインスタンスを生成する
	switch (byTag) {
	case CCaMethodDesc::DESC_TAG					: return new CCaMethodDesc;
	case CServiceDesc::DESC_TAG						: return new CServiceDesc;
	case CShortEventDesc::DESC_TAG					: return new CShortEventDesc;
	case CExtendedEventDesc::DESC_TAG				: return new CExtendedEventDesc;
	case CStreamIdDesc::DESC_TAG					: return new CStreamIdDesc;
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
	case CLocalTimeOffsetDesc::DESC_TAG				: return new CLocalTimeOffsetDesc;
	case CDownloadContentDesc::DESC_TAG				: return new CDownloadContentDesc;
	case CCaContractInfoDesc::DESC_TAG				: return new CCaContractInfoDesc;
	default											: return new CBaseDesc;
	}
}
