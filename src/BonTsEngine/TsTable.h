// TsTable.h: TS�e�[�u�����b�p�[�N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include <vector>
#include <map>
#include "MediaData.h"
#include "TsStream.h"
#include "TsDescriptor.h"


/////////////////////////////////////////////////////////////////////////////
// PSI�e�[�u�����N���X
/////////////////////////////////////////////////////////////////////////////

class ABSTRACT_CLASS_DECL CPsiTableBase
	: public CTsPidMapTarget
	, public CPsiSectionParser::IPsiSectionHandler
{
public:
	CPsiTableBase(const bool bTargetSectionExt = true, const bool bIgnoreSectionNumber = false);
	virtual ~CPsiTableBase() = 0;

	virtual void Reset();

	bool IsUpdated() const;
	DWORD GetCrcErrorCount() const;

// CTsPidMapTarget
	virtual const bool StorePacket(const CTsPacket *pPacket) override;
	virtual void OnPidUnmapped(const WORD wPID) override;

protected:
	CPsiSectionParser m_PsiSectionParser;
	bool m_bTableUpdated;
};


/////////////////////////////////////////////////////////////////////////////
// PSI�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class ABSTRACT_CLASS_DECL CPsiTable : public CPsiTableBase
{
public:
	CPsiTable();
	virtual ~CPsiTable() = 0;

	WORD GetExtensionNum() const;
	bool GetExtension(const WORD Index, WORD *pExtension) const;
	int GetExtensionIndexByTableID(const WORD TableID) const;
	WORD GetSectionNum(const WORD Index) const;
	const CPsiTableBase * GetSection(const WORD Index = 0, const WORD SectionNo = 0) const;
	bool IsExtensionComplete(const WORD Index) const;

// CPsiTableBase
	virtual void Reset() override;

protected:
// CPsiSectionParser::IPsiSectionHandler
	virtual void OnPsiSection(const CPsiSectionParser *pPsiSectionParser, const CPsiSection *pSection) override;

	virtual CPsiTableBase * CreateSectionTable(const CPsiSection *pSection) = 0;

	void ClearTable();

	struct TableItem
	{
		struct SectionItem
		{
			CPsiTableBase * pTable;
			bool bUpdated;

			SectionItem() : pTable(NULL), bUpdated(false) {}
		};

		WORD TableIdExtension;					// �e�[�u��ID�g��
		WORD SectionNum;						// �Z�N�V������
		BYTE VersionNo;							// �o�[�W�����ԍ�
		std::vector<SectionItem> SectionArray;	// �Z�N�V�����f�[�^

		void ClearSection();
	};

	std::vector<TableItem> m_TableArray;		// �e�[�u��
};

/////////////////////////////////////////////////////////////////////////////
// PSI�V���O���e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class ABSTRACT_CLASS_DECL CPsiSingleTable : public CPsiTableBase
{
public:
	CPsiSingleTable(const bool bTargetSectionExt = true);
	virtual ~CPsiSingleTable() = 0;

// CPsiTableBase
	virtual void Reset() override;

	CPsiSection m_CurSection;

protected:
// CPsiSectionParser::IPsiSectionHandler
	virtual void OnPsiSection(const CPsiSectionParser *pPsiSectionParser, const CPsiSection *pSection) override;

	virtual const bool OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection);
};

/////////////////////////////////////////////////////////////////////////////
// �X�g���[���^�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class ABSTRACT_CLASS_DECL CPsiStreamTable : public CPsiTableBase
{
public:
	class ABSTRACT_CLASS_DECL ISectionHandler
	{
	public:
		virtual ~ISectionHandler() = 0;
		virtual void OnReset(CPsiStreamTable *pTable) {}
		virtual void OnSection(CPsiStreamTable *pTable, const CPsiSection *pSection) {}
	};

	CPsiStreamTable(ISectionHandler *pHandler = NULL, const bool bTargetSectionExt = true, const bool bIgnoreSectionNumber = false);
	virtual ~CPsiStreamTable() = 0;

// CPsiTableBase
	virtual void Reset() override;

protected:
// CPsiSectionParser::IPsiSectionHandler
	virtual void OnPsiSection(const CPsiSectionParser *pPsiSectionParser, const CPsiSection *pSection) override;

	virtual const bool OnTableUpdate(const CPsiSection *pCurSection);

	ISectionHandler *m_pSectionHandler;
};

/////////////////////////////////////////////////////////////////////////////
// �����������Ȃ��e�[�u�����ۉ��N���X(���AdaptationField�����p)
// PSI�e�[�u���Ƃ��ď�������ׂ��ł͂Ȃ���������Ȃ����A����ケ���ɋL�q
/////////////////////////////////////////////////////////////////////////////
class CPsiNullTable :	public CTsPidMapTarget
{
public:
	CPsiNullTable();
	virtual ~CPsiNullTable();

// CTsPidMapTarget
	virtual const bool StorePacket(const CTsPacket *pPacket) = 0;
	virtual void OnPidUnmapped(const WORD wPID);

// CPsiNullTable

};

/////////////////////////////////////////////////////////////////////////////
// PSI�e�[�u���Z�b�g���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CPsiTableSet : public CPsiTableBase
{
public:
	CPsiTableSet(const bool bTargetSectionExt = true);
	~CPsiTableSet();

// CPsiTableBase
	virtual void Reset() override;

// CPsiTableSet
	bool MapTable(const BYTE TableID, CPsiTableBase *pTable);
	bool UnmapTable(const BYTE TableID);
	void UnmapAllTables();
	CPsiTableBase * GetTableByID(const BYTE TableID);
	const CPsiTableBase * GetTableByID(const BYTE TableID) const;
	BYTE GetLastUpdatedTableID() const;

protected:
// CPsiSectionParser::IPsiSectionHandler
	virtual void OnPsiSection(const CPsiSectionParser *pPsiSectionParser, const CPsiSection *pSection) override;

	typedef std::map<BYTE, CPsiTableBase *> SectionTableMap;
	SectionTableMap m_TableMap;

	BYTE m_LastUpdatedTableID;
};


/////////////////////////////////////////////////////////////////////////////
// PAT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CPatTable : public CPsiSingleTable
{
public:
	CPatTable(
#ifdef _DEBUG
		bool bTrace = false
#endif
	);

// CPsiSingleTable
	virtual void Reset() override;

// CPatTable
	const WORD GetTransportStreamID() const;

	const WORD GetNitPID(const WORD wIndex = 0U) const;
	const WORD GetNitNum() const;

	const WORD GetPmtPID(const WORD wIndex = 0U) const;
	const WORD GetProgramID(const WORD wIndex = 0U) const;
	const WORD GetProgramNum() const;

	const bool IsPmtTablePID(const WORD wPID) const;

protected:
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection);

	struct TAG_PATITEM
	{
		WORD wProgramID;	// �����ԑg�ԍ�ID
		WORD wPID;			// PMT��PID
	};

	std::vector<WORD> m_NitPIDArray;
	std::vector<TAG_PATITEM> m_PmtPIDArray;

#ifdef _DEBUG
	bool m_bDebugTrace;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CAT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CCatTable : public CPsiSingleTable
{
public:
	CCatTable();
	virtual ~CCatTable();

// CPsiSingleTable
	virtual void Reset() override;

// CCatTable
	const CCaMethodDesc * GetCaDescBySystemID(const WORD SystemID) const;
	WORD GetEmmPID() const;
	WORD GetEmmPID(const WORD CASystemID) const;
	const CDescBlock * GetCatDesc() const;

protected:
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection) override;

	CDescBlock m_DescBlock;
};


/////////////////////////////////////////////////////////////////////////////
// PMT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CPmtTable : public CPsiSingleTable
{
public:
	CPmtTable(
#ifdef _DEBUG
		bool bTrace = false
#endif
	);

// CPsiSingleTable
	virtual void Reset() override;

// CPmtTable
	const WORD GetProgramNumberID() const;

	const WORD GetPcrPID() const;
	const CDescBlock * GetTableDesc() const;
	const WORD GetEcmPID() const;
	const WORD GetEcmPID(const WORD CASystemID) const;

	const WORD GetEsInfoNum() const;
	const BYTE GetStreamTypeID(const WORD wIndex) const;
	const WORD GetEsPID(const WORD wIndex) const;
	const CDescBlock * GetItemDesc(const WORD wIndex) const;

protected:
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection) override;

	struct TAG_PMTITEM
	{
		BYTE byStreamTypeID;			// Stream Type ID
		WORD wEsPID;					// Elementary Stream PID
		CDescBlock DescBlock;			// Stream ID Descriptor ��
	};

	std::vector<TAG_PMTITEM> m_EsInfoArray;

	WORD m_wPcrPID;						// PCR_PID
	CDescBlock m_TableDescBlock;		// Conditional Access Method Descriptor ��

#ifdef _DEBUG
	bool m_bDebugTrace;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// SDT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CSdtTable : public CPsiSingleTable
{
public:
	enum {
		TABLE_ID_ACTUAL = 0x42,
		TABLE_ID_OTHER  = 0x46
	};

	CSdtTable(const BYTE TableID = TABLE_ID_ACTUAL);

// CPsiSingleTable
	virtual void Reset() override;

// CSdtTable
	const BYTE GetTableID() const;
	const WORD GetTransportStreamID() const;
	const WORD GetNetworkID() const;
	const WORD GetServiceNum() const;
	const WORD GetServiceIndexByID(const WORD wServiceID);
	const WORD GetServiceID(const WORD wIndex) const;
	const bool GetHEITFlag(const WORD wIndex) const;
	const bool GetMEITFlag(const WORD wIndex) const;
	const bool GetLEITFlag(const WORD wIndex) const;
	const bool GetEITScheduleFlag(const WORD wIndex) const;
	const bool GetEITPresentFollowingFlag(const WORD wIndex) const;
	const BYTE GetRunningStatus(const WORD wIndex) const;
	const bool GetFreeCaMode(const WORD wIndex) const;
	const CDescBlock * GetItemDesc(const WORD wIndex) const;

protected:
	virtual void OnPsiSection(const CPsiSectionParser *pPsiSectionParser, const CPsiSection *pSection) override;
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection) override;

	struct TAG_SDTITEM
	{
		WORD wServiceID;				// Service ID
		bool bHEITFlag;					// H-EIT flag
		bool bMEITFlag;					// M-EIT flag
		bool bLEITFlag;					// L-EIT flag
		bool bEITScheduleFlag;			// EIT Schedule Flag
		bool bEITPresentFollowingFlag;	// EIT Present Following Flag
		BYTE byRunningStatus;			// Running Status
		bool bFreeCaMode;				// Free CA Mode(true: CA / false: Free)
		CDescBlock DescBlock;			// Service Descriptor ��
	};

	BYTE m_TableID;
	WORD m_wNetworkID;
	std::vector<TAG_SDTITEM> m_ServiceInfoArray;
};

class CSdtOtherTable : public CPsiTable
{
public:
	CSdtOtherTable();
	~CSdtOtherTable();

protected:
// CPsiTable
	virtual CPsiTableBase * CreateSectionTable(const CPsiSection *pSection) override;
};

class CSdtTableSet : public CPsiTableSet
{
public:
	CSdtTableSet();
	CSdtTable *GetActualSdtTable();
	CSdtOtherTable *GetOtherSdtTable();
};


/////////////////////////////////////////////////////////////////////////////
// NIT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CNitTable : public CPsiSingleTable
{
public:
	CNitTable();

// CPsiSingleTable
	virtual void Reset() override;

// CNitTable
	const WORD GetNetworkID() const;
	const CDescBlock * GetNetworkDesc() const;
	const WORD GetTransportStreamNum() const;
	const WORD GetTransportStreamID(const WORD wIndex) const;
	const WORD GetOriginalNetworkID(const WORD wIndex) const;
	const CDescBlock * GetItemDesc(const WORD wIndex) const;

protected:
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection) override;

	struct TAG_NITITEM {
		WORD wTransportStreamID;
		WORD wOriginalNetworkID;
		CDescBlock DescBlock;
	};

	WORD m_wNetworkID;				// Network ID
	CDescBlock m_NetworkDescBlock;	// Network descriptor
	std::vector<TAG_NITITEM> m_TransportStreamArray;
};

class CNitMultiTable : public CPsiTable
{
public:
	WORD GetNitSectionNum() const;
	const CNitTable * GetNitTable(const WORD SectionNo) const;
	bool IsNitComplete() const;

protected:
// CPsiTable
	virtual CPsiTableBase * CreateSectionTable(const CPsiSection *pSection) override;
};


/////////////////////////////////////////////////////////////////////////////
// EIT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CEitTable : public CPsiSingleTable
{
public:
	enum {
		TABLE_ID_PF_ACTUAL = 0x4E,	// p/f actual
		TABLE_ID_PF_OTHER  = 0x4F	// p/f other
	};

	CEitTable();

// CPsiSingleTable
	void Reset() override;

// CEitTable
	struct EventInfo {
		WORD EventID;
		bool bValidStartTime;
		SYSTEMTIME StartTime;
		DWORD Duration;
		BYTE RunningStatus;
		bool bFreeCaMode;
		CDescBlock DescBlock;
	};

	WORD GetServiceID() const;
	WORD GetTransportStreamID() const;
	WORD GetOriginalNetworkID() const;
	BYTE GetSegmentLastSectionNumber() const;
	BYTE GetLastTableID() const;
	int GetEventNum() const;
	const EventInfo * GetEventInfo(const int Index = 0) const;
	WORD GetEventID(const int Index = 0) const;
	const SYSTEMTIME * GetStartTime(const int Index = 0) const;
	DWORD GetDuration(const int Index = 0) const;
	BYTE GetRunningStatus(const int Index = 0) const;
	bool GetFreeCaMode(const int Index = 0) const;
	const CDescBlock * GetItemDesc(const int Index = 0) const;

protected:
	const bool OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection) override;

	WORD m_ServiceID;
	WORD m_TransportStreamID;
	WORD m_OriginalNetworkID;
	BYTE m_SegmentLastSectionNumber;
	BYTE m_LastTableID;

	std::vector<EventInfo> m_EventList;
};

class CEitMultiTable : public CPsiTable
{
public:
	const CEitTable * GetEitTable(const WORD ServiceID, const WORD SectionNo) const;

protected:
// CPsiTable
	CPsiTableBase * CreateSectionTable(const CPsiSection *pSection) override;
};

class CEitPfTable : public CPsiTableSet
{
public:
	CEitPfTable();
	const CEitMultiTable * GetPfActualTable() const;
	const CEitTable * GetPfActualTable(const WORD ServiceID, const bool bFollowing = false) const;
	const CEitMultiTable * GetPfOtherTable() const;
	const CEitTable * GetPfOtherTable(const WORD ServiceID, const bool bFollowing = false) const;
};

class CEitPfActualTable : public CPsiTableSet
{
public:
	CEitPfActualTable();
	const CEitMultiTable * GetPfActualTable() const;
	const CEitTable * GetPfActualTable(const WORD ServiceID, const bool bFollowing = false) const;
};


/////////////////////////////////////////////////////////////////////////////
// TOT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CTotTable : public CPsiSingleTable
{
public:
	enum { TABLE_ID = 0x73 };

	CTotTable();
	virtual ~CTotTable();

// CPsiSingleTable
	virtual void Reset() override;

// CTotTable
	const bool GetDateTime(SYSTEMTIME *pTime) const;
	const int GetLocalTimeOffset() const;
	const CDescBlock * GetTotDesc() const;

protected:
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection) override;

	bool m_bValidDateTime;
	SYSTEMTIME m_DateTime;	// ���ݓ��t/����
	CDescBlock m_DescBlock;	// �L�q�q�̈�
};


/////////////////////////////////////////////////////////////////////////////
// CDT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CCdtTable : public CPsiStreamTable
{
public:
	enum { TABLE_ID = 0xC8 };

	CCdtTable(ISectionHandler *pHandler = NULL);
	virtual ~CCdtTable();

// CPsiStreamTable
	virtual void Reset() override;

// CCdtTable
	// �f�[�^�̎��
	enum {
		DATATYPE_LOGO		= 0x01,	// ���S
		DATATYPE_INVALID	= 0xFF	// ����
	};

	const WORD GetOriginalNetworkId() const;
	const BYTE GetDataType() const;
	const CDescBlock * GetDesc() const;
	const WORD GetDataModuleSize() const;
	const BYTE * GetDataModuleByte() const;

protected:
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection) override;

	WORD m_OriginalNetworkId;	// original_network_id
	BYTE m_DataType;			// data_type
	CDescBlock m_DescBlock;		// �L�q�q�̈�
	CMediaData m_ModuleData;
};


/////////////////////////////////////////////////////////////////////////////
// SDTT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CSdttTable : public CPsiStreamTable
{
public:
	enum { TABLE_ID = 0xC3 };

	struct ScheduleDescription
	{
		SYSTEMTIME StartTime;				// start_time
		DWORD Duration;						// duration
	};

	struct ContentInfo
	{
		BYTE GroupID;						// group_id
		WORD TargetVersion;					// target_version
		WORD NewVersion;					// new_version
		BYTE DownloadLevel;					// donwload_level
		BYTE VersionIndicator;				// version_indicator
		BYTE ScheduleTimeShiftInformation;	// schedule_time-shift_information
		std::vector<ScheduleDescription> ScheduleList;
		CDescBlock DescBlock;				// �L�q�q�̈�
	};

	CSdttTable(ISectionHandler *pHandler = NULL);
	virtual ~CSdttTable();

// CPsiStreamTable
	virtual void Reset() override;

// CSdttTable
	const BYTE GetMakerID() const;
	const BYTE GetModelID() const;
	const bool IsCommon() const;
	const WORD GetTransportStreamID() const;
	const WORD GetOriginalNetworkID() const;
	const WORD GetServiceID() const;
	const BYTE GetNumOfContents() const;
	const ContentInfo * GetContentInfo(const BYTE Index) const;
	const bool IsSchedule(DWORD Index) const;
	const CDescBlock * GetContentDesc(DWORD Index) const;

protected:
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection) override;

	BYTE m_MakerID;				// maker_id
	BYTE m_ModelID;				// model_id
	WORD m_TransportStreamID;	// transport_stream_id
	WORD m_OriginalNetworkID;	// original_network_id
	WORD m_ServiceID;			// service_id
	std::vector<ContentInfo> m_ContentList;
};


/////////////////////////////////////////////////////////////////////////////
// PCR���ۉ��N���X
// ���XDemux�̉ӏ��ɂ��������̂����g���ĂȂ��悤�������̂ŁATable���Ɉړ�
// �����_�Ŏg������̂Ƃ͌����
/////////////////////////////////////////////////////////////////////////////

class CPcrTable : public CPsiNullTable
{
public:
	typedef ULONGLONG PcrType;

	CPcrTable();

// CPsiNullTable
	const bool StorePacket(const CTsPacket *pPacket) override;

// CPcrTable
	PcrType GetPcrTimeStamp() const;

protected:
	PcrType m_Pcr;
};
