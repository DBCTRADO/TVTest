// TsTable.h: TSテーブルラッパークラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include <vector>
#include <map>
#include "MediaData.h"
#include "TsStream.h"
#include "TsDescriptor.h"


using std::vector;


/////////////////////////////////////////////////////////////////////////////
// PSIテーブル基底クラス
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
// PSIテーブル抽象化クラス
/////////////////////////////////////////////////////////////////////////////

class ABSTRACT_CLASS_DECL CPsiTable : public CPsiTableBase
{
public:
	CPsiTable();
	virtual ~CPsiTable() = 0;

	WORD GetExtensionNum(void) const;
	bool GetExtension(const WORD Index, WORD *pExtension) const;
	WORD GetSectionNum(const WORD Index) const;
	const CPsiTableBase * GetSection(const WORD Index = 0, const BYTE SectionNo = 0) const;
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

		WORD TableIdExtension;					// テーブルID拡張
		WORD SectionNum;						// セクション数
		BYTE VersionNo;							// バージョン番号
		std::vector<SectionItem> SectionArray;	// セクションデータ

		void ClearSection();
	};

	std::vector<TableItem> m_TableArray;		// テーブル
};

/////////////////////////////////////////////////////////////////////////////
// PSIシングルテーブル抽象化クラス
/////////////////////////////////////////////////////////////////////////////

class ABSTRACT_CLASS_DECL CPsiSingleTable : public CPsiTableBase
{
public:
	CPsiSingleTable(const bool bTargetSectionExt = true);
	CPsiSingleTable(const CPsiSingleTable &Operand);
	virtual ~CPsiSingleTable() = 0;

	CPsiSingleTable & operator = (const CPsiSingleTable &Operand);

// CPsiTableBase
	virtual void Reset() override;

	CPsiSection m_CurSection;

protected:
// CPsiSectionParser::IPsiSectionHandler
	virtual void OnPsiSection(const CPsiSectionParser *pPsiSectionParser, const CPsiSection *pSection) override;

	virtual const bool OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection);
};

/////////////////////////////////////////////////////////////////////////////
// ストリーム型テーブル抽象化クラス
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
	CPsiStreamTable(const CPsiStreamTable &Operand);
	virtual ~CPsiStreamTable() = 0;

	CPsiStreamTable & operator = (const CPsiStreamTable &Operand);

// CPsiTableBase
	virtual void Reset() override;

protected:
// CPsiSectionParser::IPsiSectionHandler
	virtual void OnPsiSection(const CPsiSectionParser *pPsiSectionParser, const CPsiSection *pSection) override;

	virtual const bool OnTableUpdate(const CPsiSection *pCurSection);

	ISectionHandler *m_pSectionHandler;
};

/////////////////////////////////////////////////////////////////////////////
// 何も処理がないテーブル抽象化クラス(主にAdaptationField処理用)
// PSIテーブルとして処理するべきではないかもしれないが、流れ上ここに記述
/////////////////////////////////////////////////////////////////////////////
class CPsiNullTable :	public CTsPidMapTarget
{
public:
	CPsiNullTable();
	CPsiNullTable(const CPsiNullTable &Operand);
	virtual ~CPsiNullTable();

	CPsiNullTable & operator = (const CPsiNullTable &Operand);

// CTsPidMapTarget
	virtual const bool StorePacket(const CTsPacket *pPacket) = 0;
	virtual void OnPidUnmapped(const WORD wPID);

// CPsiNullTable

};

/////////////////////////////////////////////////////////////////////////////
// PSIテーブルセット抽象化クラス
/////////////////////////////////////////////////////////////////////////////
/*
class CPsiTableSuite	// 保留： 実際のユースケースを判断した上で仕様を決める必要あり
{
public:
	CPsiTableSuite();
	CPsiTableSuite(const CPsiTableSuite &Operand);

	CPsiTableSuite & operator = (const CPsiTableSuite &Operand);

	const bool StorePacket(const CTsPacket *pPacket);

	void SetTargetSectionExt(const bool bTargetExt);
	const bool AddTable(const BYTE byTableID);

	const WORD GetIndexByID(const BYTE byTableID);
	const CPsiTable * GetTable(const WORD wIndex = 0U) const;
	const CMediaData * GetSectionData(const WORD wIndex = 0U, const WORD wSubIndex = 0U, const BYTE bySectionNo = 0U) const;

	const DWORD GetCrcErrorCount(void) const;
	void Reset(void);

protected:
	static void CALLBACK StoreSection(const CPsiSection *pSection, const PVOID pParam);

	struct TAG_TABLESET
	{
		BYTE byTableID;						// テーブルID
		CPsiTable PsiTable;					// テーブル
	};

	vector<TAG_TABLESET> m_TableSet;		// テーブルセット

	bool m_bTargetSectionExt;
	bool m_bTableUpdated;

private:
	CPsiSectionParser m_PsiSectionParser;
};
*/

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
	BYTE GetLastUpdatedTableID() const;

protected:
// CPsiSectionParser::IPsiSectionHandler
	virtual void OnPsiSection(const CPsiSectionParser *pPsiSectionParser, const CPsiSection *pSection) override;

	typedef std::map<BYTE, CPsiTableBase *> SectionTableMap;
	SectionTableMap m_TableMap;

	BYTE m_LastUpdatedTableID;
};


/////////////////////////////////////////////////////////////////////////////
// PATテーブル抽象化クラス
/////////////////////////////////////////////////////////////////////////////

class CPatTable : public CPsiSingleTable
{
public:
	CPatTable(
#ifdef _DEBUG
		bool bTrace = false
#endif
	);
	CPatTable(const CPatTable &Operand);

	CPatTable & operator = (const CPatTable &Operand);

// CPsiSingleTable
	virtual void Reset(void);

// CPatTable
	const WORD GetTransportStreamID(void) const;

	const WORD GetNitPID(const WORD wIndex = 0U) const;
	const WORD GetNitNum(void) const;

	const WORD GetPmtPID(const WORD wIndex = 0U) const;
	const WORD GetProgramID(const WORD wIndex = 0U) const;
	const WORD GetProgramNum(void) const;

	const bool IsPmtTablePID(const WORD wPID) const;

protected:
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection);

	struct TAG_PATITEM
	{
		WORD wProgramID;	// 放送番組番号ID
		WORD wPID;			// PMTのPID
	};

	vector<WORD> m_NitPIDArray;
	vector<TAG_PATITEM> m_PmtPIDArray;

#ifdef _DEBUG
	bool m_bDebugTrace;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CATテーブル抽象化クラス
/////////////////////////////////////////////////////////////////////////////

class CCatTable : public CPsiSingleTable
{
public:
	CCatTable();
	virtual ~CCatTable(void);
	CCatTable(const CCatTable &Operand);

	CCatTable  & operator = (const CCatTable &Operand);

// CPsiSingleTable
	virtual void Reset(void);

// CCatTable
	const CCaMethodDesc * GetCaDescBySystemID(const WORD SystemID) const;
	WORD GetEmmPID() const;
	WORD GetEmmPID(const WORD CASystemID) const;
	const CDescBlock * GetCatDesc() const;

protected:
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection);

	CDescBlock m_DescBlock;
};


/////////////////////////////////////////////////////////////////////////////
// PMTテーブル抽象化クラス
/////////////////////////////////////////////////////////////////////////////

class CPmtTable : public CPsiSingleTable
{
public:
	CPmtTable(
#ifdef _DEBUG
		bool bTrace = false
#endif
	);
	CPmtTable(const CPmtTable &Operand);

	CPmtTable & operator = (const CPmtTable &Operand);

// CPsiSingleTable
	virtual void Reset(void);

// CPmtTable
	const WORD GetProgramNumberID(void) const;

	const WORD GetPcrPID(void) const;
	const CDescBlock * GetTableDesc(void) const;
	const WORD GetEcmPID(void) const;
	const WORD GetEcmPID(const WORD CASystemID) const;

	const WORD GetEsInfoNum(void) const;
	const BYTE GetStreamTypeID(const WORD wIndex) const;
	const WORD GetEsPID(const WORD wIndex) const;
	const CDescBlock * GetItemDesc(const WORD wIndex) const;

protected:
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection);

	struct TAG_PMTITEM
	{
		BYTE byStreamTypeID;			// Stream Type ID
		WORD wEsPID;					// Elementary Stream PID
		CDescBlock DescBlock;			// Stream ID Descriptor 他
	};

	vector<TAG_PMTITEM> m_EsInfoArray;

	WORD m_wPcrPID;						// PCR_PID
	CDescBlock m_TableDescBlock;		// Conditional Access Method Descriptor 他

#ifdef _DEBUG
	bool m_bDebugTrace;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// SDTテーブル抽象化クラス
/////////////////////////////////////////////////////////////////////////////

class CSdtTable : public CPsiSingleTable
{
public:
	enum {
		TABLE_ID_ACTUAL = 0x42,
		TABLE_ID_OTHER  = 0x46
	};

	CSdtTable(const BYTE TableID = TABLE_ID_ACTUAL);
	CSdtTable(const CSdtTable &Operand);

	CSdtTable & operator = (const CSdtTable &Operand);

// CPsiSingleTable
	virtual void Reset(void);

// CSdtTable
	const BYTE GetTableID(void) const;
	const WORD GetTransportStreamID(void) const;
	const WORD GetNetworkID(void) const;
	const WORD GetServiceNum(void) const;
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
	virtual void OnPsiSection(const CPsiSectionParser *pPsiSectionParser, const CPsiSection *pSection);
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection);

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
		CDescBlock DescBlock;			// Service Descriptor 他
	};

	BYTE m_TableID;
	WORD m_wNetworkID;
	vector<TAG_SDTITEM> m_ServiceInfoArray;
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


/////////////////////////////////////////////////////////////////////////////
// NITテーブル抽象化クラス
/////////////////////////////////////////////////////////////////////////////

class CNitTable : public CPsiSingleTable
{
public:
	CNitTable();
	CNitTable(const CNitTable &Operand);

	CNitTable & operator = (const CNitTable &Operand);

// CPsiSingleTable
	virtual void Reset(void);

// CNitTable
	const WORD GetNetworkID(void) const;
	const CDescBlock * GetNetworkDesc(void) const;
	const WORD GetTransportStreamNum(void) const;
	const WORD GetTransportStreamID(const WORD wIndex) const;
	const WORD GetOriginalNetworkID(const WORD wIndex) const;
	const CDescBlock * GetItemDesc(const WORD wIndex) const;

protected:
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection);

	struct TAG_NITITEM {
		WORD wTransportStreamID;
		WORD wOriginalNetworkID;
		CDescBlock DescBlock;
	};

	WORD m_wNetworkID;				// Network ID
	CDescBlock m_NetworkDescBlock;	// Network descriptor
	vector<TAG_NITITEM> m_TransportStreamArray;
};


/////////////////////////////////////////////////////////////////////////////
// EIT[p/f]テーブル抽象化クラス
/////////////////////////////////////////////////////////////////////////////

class CEitPfTable : public CPsiStreamTable
{
public:
	CEitPfTable();
	CEitPfTable(const CEitPfTable &Operand);

	CEitPfTable & operator = (const CEitPfTable &Operand);

// CPsiSingleTable
	virtual void Reset(void);

// CEitPfTable
	const DWORD GetServiceNum(void) const;
	const int GetServiceIndexByID(WORD ServiceID) const;
	const WORD GetServiceID(DWORD Index) const;
	const WORD GetTransportStreamID(DWORD Index) const;
	const WORD GetOriginalNetworkID(DWORD Index) const;
	const WORD GetEventID(DWORD Index, DWORD EventIndex) const;
	const SYSTEMTIME *GetStartTime(DWORD Index, DWORD EventIndex) const;
	const DWORD GetDuration(DWORD Index, DWORD EventIndex) const;
	const BYTE GetRunningStatus(DWORD Index, DWORD EventIndex) const;
	const bool GetFreeCaMode(DWORD Index, DWORD EventIndex) const;
	const CDescBlock * GetItemDesc(DWORD Index, DWORD EventIndex) const;

protected:
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection);

	struct EventInfo {
		bool bEnable;
		WORD EventID;
		bool bValidStartTime;
		SYSTEMTIME StartTime;
		DWORD Duration;
		BYTE RunningStatus;
		bool FreeCaMode;
		CDescBlock DescBlock;
		EventInfo() : bEnable(false) {}
	};

	struct ServiceInfo {
		WORD ServiceID;
		WORD TransportStreamID;
		WORD OriginalNetworkID;
		EventInfo EventList[2];
	};

	vector<ServiceInfo> m_ServiceList;
};


/////////////////////////////////////////////////////////////////////////////
// TOTテーブル抽象化クラス
/////////////////////////////////////////////////////////////////////////////

class CTotTable : public CPsiSingleTable
{
public:
	enum { TABLE_ID = 0x73 };

	CTotTable();
	virtual ~CTotTable();

// CPsiSingleTable
	virtual void Reset(void);

// CTotTable
	const bool GetDateTime(SYSTEMTIME *pTime) const;
	const int GetLocalTimeOffset() const;
	const CDescBlock * GetTotDesc(void) const;

protected:
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection);

	bool m_bValidDateTime;
	SYSTEMTIME m_DateTime;	// 現在日付/時刻
	CDescBlock m_DescBlock;	// 記述子領域
};


/////////////////////////////////////////////////////////////////////////////
// CDTテーブル抽象化クラス
/////////////////////////////////////////////////////////////////////////////

class CCdtTable : public CPsiStreamTable
{
public:
	enum { TABLE_ID = 0xC8 };

	CCdtTable(ISectionHandler *pHandler = NULL);
	virtual ~CCdtTable();

// CPsiStreamTable
	virtual void Reset(void);

// CCdtTable
	// データの種類
	enum {
		DATATYPE_LOGO		= 0x01,	// ロゴ
		DATATYPE_INVALID	= 0xFF	// 無効
	};

	const WORD GetOriginalNetworkId() const;
	const BYTE GetDataType() const;
	const CDescBlock * GetDesc() const;
	const WORD GetDataModuleSize() const;
	const BYTE * GetDataModuleByte() const;

protected:
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection);

	WORD m_OriginalNetworkId;	// original_network_id
	BYTE m_DataType;			// data_type
	CDescBlock m_DescBlock;		// 記述子領域
	CMediaData m_ModuleData;
};


/////////////////////////////////////////////////////////////////////////////
// SDTTテーブル抽象化クラス
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
		CDescBlock DescBlock;				// 記述子領域
	};

	CSdttTable(ISectionHandler *pHandler = NULL);
	virtual ~CSdttTable();

// CPsiStreamTable
	virtual void Reset(void);

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
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection);

	BYTE m_MakerID;				// maker_id
	BYTE m_ModelID;				// model_id
	WORD m_TransportStreamID;	// transport_stream_id
	WORD m_OriginalNetworkID;	// original_network_id
	WORD m_ServiceID;			// service_id
	std::vector<ContentInfo> m_ContentList;
};


/////////////////////////////////////////////////////////////////////////////
// PCR抽象化クラス
// 元々Demuxの箇所にあったものだが使ってないようだったので、Table側に移動
// 現時点で使えるものとは言い難い
/////////////////////////////////////////////////////////////////////////////

class CPcrTable : public CPsiNullTable
{
public:
	CPcrTable(WORD wServiceIndex);
	CPcrTable(const CPcrTable &Operand);

	CPcrTable & operator = (const CPcrTable &Operand);

// CPsiNullTable
	virtual const bool StorePacket(const CTsPacket *pPacket);

// CPcrTable
	void AddServiceIndex(WORD wServiceIndex);
	const WORD GetServiceIndex(WORD *pwServiceIndex, WORD wIndex=0);
	const unsigned __int64 GetPcrTimeStamp();

protected:
	vector<WORD> m_ServiceIndex;
	unsigned __int64 m_ui64_Pcr;
};
