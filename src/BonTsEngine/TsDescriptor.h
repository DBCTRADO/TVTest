// TsDescriptor.h: �L�q�q���b�p�[�N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include <Vector>
#include "Common.h"
#include "MediaData.h"


/////////////////////////////////////////////////////////////////////////////
// �L�q�q�̊��N���X
/////////////////////////////////////////////////////////////////////////////

class CBaseDesc
{
public:
	CBaseDesc();
	virtual ~CBaseDesc();

	virtual void CopyDesc(const CBaseDesc *pOperand);
	bool ParseDesc(const BYTE *pHexData, const WORD wDataLength);

	bool IsValid() const;
	BYTE GetTag() const;
	BYTE GetLength() const;

	virtual void Reset();

protected:
	virtual bool StoreContents(const BYTE *pPayload);

	BYTE m_byDescTag;	// �L�q�q�^�O
	BYTE m_byDescLen;	// �L�q�q��
	bool m_bIsValid;	// ��͌���
};


/////////////////////////////////////////////////////////////////////////////
// �L�q�q�̃e���v���[�g�N���X
/////////////////////////////////////////////////////////////////////////////

template<typename T, BYTE Tag> class CDescTemplate : public CBaseDesc
{
public:
	enum {DESC_TAG = Tag};

	void CopyDesc(const CBaseDesc *pOperand) override
	{
		if (pOperand != this) {
			const T *pSrcDesc = dynamic_cast<const T *>(pOperand);

			if (pSrcDesc) {
				*static_cast<T*>(this) = *pSrcDesc;
			} else {
				CBaseDesc::CopyDesc(pOperand);
			}
		}
	}
};


/////////////////////////////////////////////////////////////////////////////
// [0x09] Conditional Access Method �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CCaMethodDesc : public CDescTemplate<CCaMethodDesc, 0x09>
{
public:
	CCaMethodDesc();

// CBaseDesc
	void Reset() override;

// CCaMethodDesc
	WORD GetCaMethodID() const;
	WORD GetCaPID() const;
	const CMediaData * GetPrivateData() const;

protected:
	bool StoreContents(const BYTE *pPayload) override;

	WORD m_wCaMethodID;			// Conditional Access Method ID
	WORD m_wCaPID;				// Conditional Access PID
	CMediaData m_PrivateData;	// Private Data
};


/////////////////////////////////////////////////////////////////////////////
// [0x48] Service �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CServiceDesc : public CDescTemplate<CServiceDesc, 0x48>
{
public:
	CServiceDesc();
	CServiceDesc(const CServiceDesc &Operand);
	CServiceDesc & operator = (const CServiceDesc &Operand);

// CBaseDesc
	void Reset() override;

// CServiceDesc
	BYTE GetServiceType() const;
	DWORD GetProviderName(LPTSTR lpszDst, int MaxLength) const;
	DWORD GetServiceName(LPTSTR lpszDst, int MaxLength) const;

protected:
	bool StoreContents(const BYTE *pPayload) override;

	BYTE m_byServiceType;			// Service Type
	TCHAR m_szProviderName[256];	// Service Provider Name
	TCHAR m_szServiceName[256];		// Service Name
};


/////////////////////////////////////////////////////////////////////////////
// [0x4D] Short Event �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CShortEventDesc : public CDescTemplate<CShortEventDesc, 0x4D>
{
public:
	CShortEventDesc();
	CShortEventDesc(const CShortEventDesc &Operand);
	CShortEventDesc & operator = (const CShortEventDesc &Operand);

// CBaseDesc
	void Reset() override;

// CShortEventDesc
	DWORD GetLanguageCode() const;
	DWORD GetEventName(LPTSTR lpszDst, int MaxLength) const;
	DWORD GetEventDesc(LPTSTR lpszDst, int MaxLength) const;

protected:
	bool StoreContents(const BYTE *pPayload) override;

	DWORD m_dwLanguageCode;			// ISO639  Language Code
	TCHAR m_szEventName[256];		// Event Name
	TCHAR m_szEventDesc[256];		// Event Description
};


/////////////////////////////////////////////////////////////////////////////
// [0x4E] Extended Event �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CExtendedEventDesc : public CDescTemplate<CExtendedEventDesc, 0x4E>
{
public:
	CExtendedEventDesc();

// CBaseDesc
	void Reset() override;

// CExtendedEventDesc
	enum {
		MAX_DESCRIPTION = 32
	};
	struct ItemInfo {
		TCHAR szDescription[MAX_DESCRIPTION];
		BYTE ItemLength;
		BYTE ItemChar[220];
	};

	BYTE GetDescriptorNumber() const;
	BYTE GetLastDescriptorNumber() const;
	DWORD GetLanguageCode() const;
	int GetItemCount() const;
	const ItemInfo * GetItem(int Index) const;

protected:
	bool StoreContents(const BYTE *pPayload) override;

	BYTE m_DescriptorNumber;
	BYTE m_LastDescriptorNumber;
	DWORD m_LanguageCode;			// ISO639  Language Code
	std::vector<ItemInfo> m_ItemList;
};


/////////////////////////////////////////////////////////////////////////////
// [0x52] Stream Identifier �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CStreamIdDesc : public CDescTemplate<CStreamIdDesc, 0x52>
{
public:
	CStreamIdDesc();

// CBaseDesc
	void Reset() override;

// CStreamIdDesc
	BYTE GetComponentTag(void) const;

protected:
	bool StoreContents(const BYTE *pPayload) override;

	BYTE m_byComponentTag;		// Component Tag
};


/////////////////////////////////////////////////////////////////////////////
// [0xC0] Hierarchical Transmission �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CHierarchicalTransmissionDesc : public CDescTemplate<CHierarchicalTransmissionDesc, 0xC0>
{
public:
	CHierarchicalTransmissionDesc();

// CBaseDesc
	void Reset() override;

// CHierarchicalTransmissionDesc
	BYTE GetQualityLevel() const;
	WORD GetReferencePID() const;

protected:
	bool StoreContents(const BYTE *pPayload) override;

	BYTE m_QualityLevel;
	WORD m_ReferencePID;
};


/////////////////////////////////////////////////////////////////////////////
// [0x40] Network Name �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CNetworkNameDesc : public CDescTemplate<CNetworkNameDesc, 0x40>
{
public:
	CNetworkNameDesc();
	CNetworkNameDesc(const CNetworkNameDesc &Operand);
	CNetworkNameDesc & operator = (const CNetworkNameDesc &Operand);

// CBaseDesc
	void Reset() override;

// CNetworkNameDesc
	DWORD GetNetworkName(LPTSTR pszName, int MaxLength) const;

protected:
	bool StoreContents(const BYTE *pPayload) override;

	TCHAR m_szNetworkName[32];
};


/////////////////////////////////////////////////////////////////////////////
// [0x41] Service List �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CServiceListDesc : public CDescTemplate<CServiceListDesc, 0x41>
{
public:
	CServiceListDesc();

// CBaseDesc
	void Reset() override;

// CServiceListDesc
	struct ServiceInfo {
		WORD ServiceID;
		BYTE ServiceType;
	};

	int GetServiceNum() const;
	int GetServiceIndexByID(const WORD ServiceID) const;
	BYTE GetServiceTypeByID(const WORD ServiceID) const;
	bool GetServiceInfo(const int Index, ServiceInfo *pInfo) const;

protected:
	bool StoreContents(const BYTE *pPayload) override;

	std::vector<ServiceInfo> m_ServiceList;
};


/////////////////////////////////////////////////////////////////////////////
// [0x43] Satellite Delivery System �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CSatelliteDeliverySystemDesc : public CDescTemplate<CSatelliteDeliverySystemDesc, 0x43>
{
public:
	CSatelliteDeliverySystemDesc();

// CBaseDesc
	void Reset() override;

// CSatelliteDeliverySystemDesc
	DWORD GetFrequency() const;
	WORD GetOrbitalPosition() const;
	bool GetWestEastFlag() const;
	BYTE GetPolarization() const;
	BYTE GetModulation() const;
	DWORD GetSymbolRate() const;
	BYTE GetFECInner() const;

protected:
	bool StoreContents(const BYTE *pPayload) override;

	DWORD m_Frequency;
	WORD m_OrbitalPosition;
	bool m_bWestEastFlag;
	BYTE m_Polarization;
	BYTE m_Modulation;
	DWORD m_SymbolRate;
	BYTE m_FECInner;
};


/////////////////////////////////////////////////////////////////////////////
// [0xFA] Terrestrial Delivery System �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CTerrestrialDeliverySystemDesc : public CDescTemplate<CTerrestrialDeliverySystemDesc, 0xFA>
{
public:
	CTerrestrialDeliverySystemDesc();

// CBaseDesc
	void Reset() override;

// CTerrestrialDeliverySystemDesc
	WORD GetAreaCode() const;
	BYTE GetGuardInterval() const;
	BYTE GetTransmissionMode() const;
	int GetFrequencyNum() const;
	WORD GetFrequency(const int Index) const;

protected:
	bool StoreContents(const BYTE *pPayload) override;

	WORD m_AreaCode;
	BYTE m_GuardInterval;
	BYTE m_TransmissionMode;
	std::vector<WORD> m_Frequency;
};


/////////////////////////////////////////////////////////////////////////////
// [0xFE] System Management �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CSystemManageDesc : public CDescTemplate<CSystemManageDesc, 0xFE>
{
public:
	CSystemManageDesc();

// CBaseDesc
	void Reset() override;

// CServiceDesc
	BYTE GetBroadcastingFlag() const;
	BYTE GetBroadcastingID() const;
	BYTE GetAdditionalBroadcastingID() const;

protected:
	bool StoreContents(const BYTE *pPayload) override;

	BYTE m_byBroadcastingFlag;
	BYTE m_byBroadcastingID;
	BYTE m_byAdditionalBroadcastingID;
};


/////////////////////////////////////////////////////////////////////////////
// [0xCD] TS Information �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CTSInfoDesc : public CDescTemplate<CTSInfoDesc, 0xCD>
{
public:
	CTSInfoDesc();
	CTSInfoDesc(const CTSInfoDesc &Operand);
	CTSInfoDesc & operator = (const CTSInfoDesc &Operand);

// CBaseDesc
	void Reset() override;

// CTSInfoDesc
	BYTE GetRemoteControlKeyID() const;
	DWORD GetTSName(LPTSTR pszName, int MaxLength) const;

protected:
	bool StoreContents(const BYTE *pPayload) override;

	BYTE m_byRemoteControlKeyID;
	TCHAR m_szTSName[32];
};


/////////////////////////////////////////////////////////////////////////////
// [0x50] Component �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CComponentDesc : public CDescTemplate<CComponentDesc, 0x50>
{
public:
	CComponentDesc();
	CComponentDesc(const CComponentDesc &Operand);
	CComponentDesc & operator = (const CComponentDesc &Operand);

// CBaseDesc
	void Reset() override;

// CComponentDesc
	BYTE GetStreamContent() const;
	BYTE GetComponentType() const;
	BYTE GetComponentTag() const;
	DWORD GetLanguageCode() const;
	DWORD GetText(LPTSTR pszText, int MaxLength) const;

protected:
	bool StoreContents(const BYTE *pPayload) override;

	BYTE m_StreamContent;
	BYTE m_ComponentType;
	BYTE m_ComponentTag;
	DWORD m_LanguageCode;
	TCHAR m_szText[64];
};


/////////////////////////////////////////////////////////////////////////////
// [0xC4] Audio Component �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CAudioComponentDesc : public CDescTemplate<CAudioComponentDesc, 0xC4>
{
public:
	CAudioComponentDesc();
	CAudioComponentDesc(const CAudioComponentDesc &Operand);
	CAudioComponentDesc & operator = (const CAudioComponentDesc &Operand);

// CBaseDesc
	void Reset() override;

// CAudioComponentDesc
	BYTE GetStreamContent() const;
	BYTE GetComponentType() const;
	BYTE GetComponentTag() const;
	BYTE GetSimulcastGroupTag() const;
	bool GetESMultiLingualFlag() const;
	bool GetMainComponentFlag() const;
	BYTE GetQualityIndicator() const;
	BYTE GetSamplingRate() const;
	DWORD GetLanguageCode() const;
	DWORD GetLanguageCode2() const;
	DWORD GetText(LPTSTR pszText, int MaxLength) const;

protected:
	bool StoreContents(const BYTE *pPayload) override;

	BYTE m_StreamContent;
	BYTE m_ComponentType;
	BYTE m_ComponentTag;
	BYTE m_StreamType;
	BYTE m_SimulcastGroupTag;
	bool m_bESMultiLingualFlag;
	bool m_bMainComponentFlag;
	BYTE m_QualityIndicator;
	BYTE m_SamplingRate;
	DWORD m_LanguageCode;
	DWORD m_LanguageCode2;
	TCHAR m_szText[64];
};


/////////////////////////////////////////////////////////////////////////////
// [0x54] Content �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CContentDesc : public CDescTemplate<CContentDesc, 0x54>
{
public:
	CContentDesc();

// CBaseDesc
	void Reset() override;

// CContentDesc
	struct Nibble {
		BYTE ContentNibbleLevel1;
		BYTE ContentNibbleLevel2;
		BYTE UserNibble1;
		BYTE UserNibble2;

		bool operator==(const Nibble &Operand) const {
			return ContentNibbleLevel1 == Operand.ContentNibbleLevel1
				&& ContentNibbleLevel2 == Operand.ContentNibbleLevel2
				&& UserNibble1 == Operand.UserNibble1
				&& UserNibble2 == Operand.UserNibble2;
		}
		bool operator!=(const Nibble &Operand) const { return !(*this==Operand); }
	};

	int GetNibbleCount() const;
	bool GetNibble(int Index, Nibble *pNibble) const;

protected:
	bool StoreContents(const BYTE *pPayload) override;

	int m_NibbleCount;
	Nibble m_NibbleList[7];
};


/////////////////////////////////////////////////////////////////////////////
// [0xCF] Logo Transmission �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CLogoTransmissionDesc : public CDescTemplate<CLogoTransmissionDesc, 0xCF>
{
public:
	CLogoTransmissionDesc();

// CBaseDesc
	void Reset() override;

// CLogoTransmissionDesc
	// logo_transmission_type
	enum {
		TRANSMISSION_UNDEFINED,
		TRANSMISSION_CDT1,		// CDT�`������1
		TRANSMISSION_CDT2,		// CDT�`������2
		TRANSMISSION_CHAR		// �ȈՃ��S����
	};
	enum {
		MAX_LOGO_CHAR		= 12,		// �ő�ȈՃ��S��
		LOGOID_INVALID		= 0xFFFF,	// ������ logo_id
		LOGOVERSION_INVALID	= 0xFFFF,	// ������ logo_version
		DATAID_INVALID		= 0xFFFF	// ������ download_data_id
	};

	BYTE GetLogoTransmissionType() const;
	WORD GetLogoID() const;
	WORD GetLogoVersion() const;
	WORD GetDownloadDataID() const;
	int GetLogoChar(char *pChar, int MaxLength) const;

protected:
	bool StoreContents(const BYTE *pPayload) override;

	BYTE m_LogoTransmissionType;	// logo_transmission_type
	WORD m_LogoID;					// logo_id
	WORD m_LogoVersion;				// logo_version
	WORD m_DownloadDataID;			// download_data_id
	char m_LogoChar[MAX_LOGO_CHAR];	// logo_char
};


/////////////////////////////////////////////////////////////////////////////
// [0xD5] Series �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CSeriesDesc : public CDescTemplate<CSeriesDesc, 0xD5>
{
public:
	CSeriesDesc();
	CSeriesDesc(const CSeriesDesc &Operand);
	CSeriesDesc & operator = (const CSeriesDesc &Operand);

// CBaseDesc
	void Reset() override;

// CSeriesDesc
	enum {
		SERIESID_INVALID	= 0xFFFF,
		MAX_SERIES_NAME		= 64
	};
	enum {
		PROGRAMPATTERN_IRREGULAR,					// �s���
		PROGRAMPATTERN_ACROSS_THE_BOARD,			// �єԑg
		PROGRAMPATTERN_WEEKLY,						// �T���
		PROGRAMPATTERN_MONTHLY,						// �����
		PROGRAMPATTERN_MULTIPLE_EPISODES_IN_DAY,	// �������ɕ����b�Ґ�
		PROGRAMPATTERN_DIVISION_LONG_PROGRAM,		// �����Ԕԑg�̕���
		PROGRAMPATTERN_INVALID = 0xFF
	};

	WORD GetSeriesID() const;
	BYTE GetRepeatLabel() const;
	BYTE GetProgramPattern() const;
	bool IsExpireDateValid() const;
	bool GetExpireDate(SYSTEMTIME *pDate) const;
	WORD GetEpisodeNumber() const;
	WORD GetLastEpisodeNumber() const;
	int GetSeriesName(LPTSTR pszName, int MaxName) const;

protected:
	bool StoreContents(const BYTE *pPayload) override;

	WORD m_SeriesID;
	BYTE m_RepeatLabel;
	BYTE m_ProgramPattern;
	bool m_bExpireDateValidFlag;
	SYSTEMTIME m_ExpireDate;
	WORD m_EpisodeNumber;
	WORD m_LastEpisodeNumber;
	TCHAR m_szSeriesName[MAX_SERIES_NAME];
};


/////////////////////////////////////////////////////////////////////////////
// [0xD6] Event Group �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CEventGroupDesc : public CDescTemplate<CEventGroupDesc, 0xD6>
{
public:
	CEventGroupDesc();

// CBaseDesc
	void Reset() override;

// CEventGroupDesc
	enum {
		GROUPTYPE_UNDEFINED,
		GROUPTYPE_COMMON,
		GROUPTYPE_RELAY,
		GROUPTYPE_MOVEMENT,
		GROUPTYPE_RELAY_TO_OTHER_NETWORK,
		GROUPTYPE_MOVEMENT_FROM_OTHER_NETWORK
	};

	struct EventInfo {
		WORD ServiceID;
		WORD EventID;
		WORD OriginalNetworkID;
		WORD TransportStreamID;

		bool operator==(const EventInfo &Operand) const
		{
			return ServiceID == Operand.ServiceID
				&& EventID == Operand.EventID
				&& OriginalNetworkID == Operand.OriginalNetworkID
				&& TransportStreamID == Operand.TransportStreamID;
		}

		bool operator!=(const EventInfo &Operand) const
		{
			return !(*this==Operand);
		}
	};

	BYTE GetGroupType() const;
	int GetEventNum() const;
	bool GetEventInfo(int Index, EventInfo *pInfo) const;

protected:
	bool StoreContents(const BYTE *pPayload) override;

	BYTE m_GroupType;
	std::vector<EventInfo> m_EventList;
};


/////////////////////////////////////////////////////////////////////////////
// [0xD9] Component Group �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CComponentGroupDesc : public CDescTemplate<CComponentGroupDesc, 0xD9>
{
public:
	CComponentGroupDesc();

// CBaseDesc
	void Reset() override;

// CComponentGroupDesc
	static const int MAX_TEXT = 64;

	struct CAUnitInfo
	{
		BYTE CAUnitID;
		BYTE ComponentNum;
		BYTE ComponentTag[16];
	};

	struct GroupInfo
	{
		BYTE ComponentGroupID;
		BYTE CAUnitNum;
		CAUnitInfo CAUnit[16];
		BYTE TotalBitRate;
		TCHAR szText[MAX_TEXT];
	};

	BYTE GetComponentGroupType() const;
	bool GetTotalBitRateFlag() const;
	BYTE GetGroupNum() const;
	const GroupInfo *GetGroupInfo(const int Index) const;

protected:
	bool StoreContents(const BYTE *pPayload) override;

	BYTE m_ComponentGroupType;
	bool m_bTotalBitRateFlag;
	std::vector<GroupInfo> m_GroupList;
};


/////////////////////////////////////////////////////////////////////////////
// [0x58] Local Time Offset �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CLocalTimeOffsetDesc : public CDescTemplate<CLocalTimeOffsetDesc, 0x58>
{
public:
	CLocalTimeOffsetDesc();

// CBaseDesc
	void Reset() override;

// CLocalTimeOffsetDesc
	enum {
		COUNTRYCODE_JPN		= 0x4A504EUL,
		COUNTRYREGION_ALL	= 0x00
	};

	bool IsValid() const;
	DWORD GetCountryCode() const;
	BYTE GetCountryRegionID() const;
	int GetLocalTimeOffset() const;
	bool GetTimeOfChange(SYSTEMTIME *pTime) const;
	int GetNextTimeOffset() const;

protected:
	bool StoreContents(const BYTE *pPayload) override;

	struct TimeOffsetInfo {
		bool bValid;
		DWORD CountryCode;
		BYTE CountryRegionID;
		BYTE LocalTimeOffsetPolarity;
		WORD LocalTimeOffset;
		SYSTEMTIME TimeOfChange;
		WORD NextTimeOffset;
	};
	TimeOffsetInfo m_Info;
};


/////////////////////////////////////////////////////////////////////////////
// [0xC9] Download Content �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CDownloadContentDesc : public CDescTemplate<CDownloadContentDesc, 0xC9>
{
public:
	enum {DESC_TAG = 0xC9U};

	CDownloadContentDesc();

// CBaseDesc
	void Reset() override;

// CDownloadContentDesc
	bool GetReboot() const;
	bool GetAddOn() const;
	DWORD GetComponentSize() const;
	DWORD GetDownloadID() const;
	DWORD GetTimeOutValueDII() const;
	DWORD GetLeakRate() const;
	BYTE GetComponentTag() const;

protected:
	bool StoreContents(const BYTE *pPayload) override;

	struct DownloadContentInfo
	{
		bool bReboot;
		bool bAddOn;
		bool bCompatibilityFlag;
		bool bModuleInfoFlag;
		bool bTextInfoFlag;
		DWORD ComponentSize;
		DWORD DownloadID;
		DWORD TimeOutValueDII;
		DWORD LeakRate;
		BYTE ComponentTag;
	};

	DownloadContentInfo m_Info;
};


/////////////////////////////////////////////////////////////////////////////
// [0xCB] CA Contract Info �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CCaContractInfoDesc : public CDescTemplate<CCaContractInfoDesc, 0xCB>
{
public:
	CCaContractInfoDesc();
	CCaContractInfoDesc(const CCaContractInfoDesc &Operand);
	CCaContractInfoDesc & operator = (const CCaContractInfoDesc &Operand);

// CBaseDesc
	void Reset() override;

// CCAContractInfoDesc
	enum {
		MAX_NUM_OF_COMPONENT = 12,
		MAX_VERIFICATION_INFO_LENGTH = 172,
		MAX_FEE_NAME = 256
	};

	WORD GetCaSystemID() const;
	BYTE GetCaUnitID() const;
	BYTE GetNumOfComponent() const;
	BYTE GetComponentTag(BYTE Index) const;
	BYTE GetContractVerificationInfoLength() const;
	BYTE GetContractVerificationInfo(BYTE *pInfo, BYTE MaxLength) const;
	int GetFeeName(LPTSTR pszName, int MaxName) const;

protected:
	bool StoreContents(const BYTE *pPayload) override;

	WORD m_CaSystemID;
	BYTE m_CaUnitID;
	BYTE m_NumOfComponent;
	BYTE m_ComponentTag[MAX_NUM_OF_COMPONENT];
	BYTE m_ContractVerificationInfoLength;
	BYTE m_ContractVerificationInfo[MAX_VERIFICATION_INFO_LENGTH];
	TCHAR m_szFeeName[MAX_FEE_NAME];
};


/////////////////////////////////////////////////////////////////////////////
// [0xFB] ������M�L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CPartialReceptionDesc : public CDescTemplate<CPartialReceptionDesc, 0xFB>
{
public:
	CPartialReceptionDesc();

// CBaseDesc
	void Reset() override;

// CPartialReceptionDesc
	BYTE GetServiceNum() const;
	WORD GetServiceID(const BYTE Index) const;

protected:
	bool StoreContents(const BYTE *pPayload) override;

	BYTE m_ServiceNum;
	WORD m_ServiceList[3];
};


/////////////////////////////////////////////////////////////////////////////
// �L�q�q�u���b�N���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CDescBlock
{
public:
	CDescBlock();
	CDescBlock(const CDescBlock &Operand);
	~CDescBlock();
	CDescBlock & operator = (const CDescBlock &Operand);

	WORD ParseBlock(const BYTE *pHexData, const WORD wDataLength);
	const CBaseDesc * ParseBlock(const BYTE *pHexData, const WORD wDataLength, const BYTE byTag);

	virtual void Reset();

	WORD GetDescNum() const;
	const CBaseDesc * GetDescByIndex(const WORD wIndex = 0U) const;
	const CBaseDesc * GetDescByTag(const BYTE byTag) const;
	template<typename T> const T * GetDesc() const {
		return dynamic_cast<const T *>(GetDescByTag(T::DESC_TAG));
	}
	template<typename TDesc, typename TPred> void EnumDesc(TPred Pred) const {
		const WORD DescNum = GetDescNum();
		for (WORD i = 0; i < DescNum; i++) {
			const CBaseDesc *pBaseDesc = GetDescByIndex(i);
			if (pBaseDesc->GetTag() == TDesc::DESC_TAG) {
				const TDesc *pDesc = dynamic_cast<const TDesc *>(pBaseDesc);
				if (pDesc)
					Pred(pDesc);
			}
		}
	}

protected:
	CBaseDesc * ParseDesc(const BYTE *pHexData, const WORD wDataLength);
	static CBaseDesc * CreateDescInstance(const BYTE byTag);

	std::vector<CBaseDesc *> m_DescArray;
};
