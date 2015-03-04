// TsTable.cpp: TSテーブルラッパークラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TsTable.h"
#include "TsEncode.h"
#include "TsUtilClass.h"
#include "../Common/DebugDef.h"


/////////////////////////////////////////////////////////////////////////////
// PSIテーブル基底クラス
/////////////////////////////////////////////////////////////////////////////

CPsiTableBase::CPsiTableBase(const bool bTargetSectionExt, const bool bIgnoreSectionNumber)
	: m_PsiSectionParser(this, bTargetSectionExt, bIgnoreSectionNumber)
	, m_bTableUpdated(false)
{
}

CPsiTableBase::~CPsiTableBase()
{
}

void CPsiTableBase::Reset()
{
	// 状態初期化
	m_PsiSectionParser.Reset();
	m_bTableUpdated = false;
}

bool CPsiTableBase::IsUpdated() const
{
	return m_bTableUpdated;
}

DWORD CPsiTableBase::GetCrcErrorCount() const
{
	return m_PsiSectionParser.GetCrcErrorCount();
}

const bool CPsiTableBase::StorePacket(const CTsPacket *pPacket)
{
	if (!pPacket)
		return false;

	m_bTableUpdated = false;

	// パケットストア
	m_PsiSectionParser.StorePacket(pPacket);

	return m_bTableUpdated;
}

void CPsiTableBase::OnPidUnmapped(const WORD wPID)
{
	// インスタンスを開放する
	delete this;
}


//////////////////////////////////////////////////////////////////////
// CPsiTableクラスの構築/消滅
//////////////////////////////////////////////////////////////////////

CPsiTable::CPsiTable()
{
	Reset();
}

CPsiTable::~CPsiTable()
{
	ClearTable();
}

WORD CPsiTable::GetExtensionNum() const
{
	// テーブルの数を返す
	return (WORD)m_TableArray.size();
}

bool CPsiTable::GetExtension(const WORD Index, WORD *pExtension) const
{
	if (Index >= GetExtensionNum())
		return false;

	// テーブルID拡張を返す
	*pExtension = m_TableArray[Index].TableIdExtension;

	return true;
}

int CPsiTable::GetExtensionIndexByTableID(const WORD TableID) const
{
	for (size_t Index = 0; Index < m_TableArray.size(); Index++) {
		if (m_TableArray[Index].TableIdExtension == TableID)
			return static_cast<int>(Index);
	}

	return -1;
}

WORD CPsiTable::GetSectionNum(const WORD Index) const
{
	if (Index >= GetExtensionNum())
		return 0;

	// セクション数を返す
	return m_TableArray[Index].SectionNum;
}

const CPsiTableBase * CPsiTable::GetSection(const WORD Index, const WORD SectionNo) const
{
	if (Index >= GetExtensionNum())
		return NULL;
	if (SectionNo >= m_TableArray[Index].SectionNum)
		return NULL;

	// セクションデータを返す
	return m_TableArray[Index].SectionArray[SectionNo].pTable;
}

bool CPsiTable::IsExtensionComplete(const WORD Index) const
{
	if (Index >= GetExtensionNum())
		return false;

	const TableItem &Table = m_TableArray[Index];

	for (size_t i = 0; i < Table.SectionArray.size(); i++) {
		if (!Table.SectionArray[i].pTable
				|| !Table.SectionArray[i].bUpdated)
			return false;
	}

	return true;
}

void CPsiTable::Reset()
{
	CPsiTableBase::Reset();

	ClearTable();
}

void CPsiTable::ClearTable()
{
	// 全てのテーブルを削除する
	for (size_t i = 0; i < m_TableArray.size(); i++)
		m_TableArray[i].ClearSection();

	m_TableArray.clear();
}

void CPsiTable::OnPsiSection(const CPsiSectionParser *pPsiSectionParser, const CPsiSection *pSection)
{
	m_bTableUpdated = false;

	// セクション番号チェック
	if (pSection->GetSectionNumber() > pSection->GetLastSectionNumber())
		return;

	// セクションのサイズをチェック
	if (!pSection->GetPayloadSize())
		return;

	// カレントネクストインジケータチェック(本来は別バンクに貯めるべき？)
	if (!pSection->IsCurrentNext())
		return;

	// テーブルID拡張を検索する
	size_t Index;

	for (Index = 0; Index < m_TableArray.size(); Index++) {
		if (m_TableArray[Index].TableIdExtension == pSection->GetTableIdExtension())
			break;
	}

	if (Index == m_TableArray.size()) {
		// テーブルID拡張が見つからない、テーブル追加
		m_TableArray.resize(Index + 1);
		m_TableArray[Index].TableIdExtension = pSection->GetTableIdExtension();
		m_TableArray[Index].SectionNum = (WORD)pSection->GetLastSectionNumber() + 1;
		m_TableArray[Index].VersionNo = pSection->GetVersionNo();
		m_TableArray[Index].SectionArray.resize(m_TableArray[Index].SectionNum);
		m_bTableUpdated = true;
	} else if (m_TableArray[Index].VersionNo != pSection->GetVersionNo()
			|| m_TableArray[Index].SectionNum != (WORD)pSection->GetLastSectionNumber() + 1) {
		// バージョンが不一致、テーブルが更新された
		m_TableArray[Index].SectionNum = (WORD)pSection->GetLastSectionNumber() + 1;
		m_TableArray[Index].VersionNo = pSection->GetVersionNo();
		m_TableArray[Index].ClearSection();
		m_TableArray[Index].SectionArray.resize(m_TableArray[Index].SectionNum);
		m_bTableUpdated = true;
	}

	// セクションデータを更新する
	const WORD SectionNumber = pSection->GetSectionNumber();
	TableItem::SectionItem &Section = m_TableArray[Index].SectionArray[SectionNumber];
	if (!Section.pTable) {
		Section.pTable = CreateSectionTable(pSection);
		if (!Section.pTable)
			return;
	}
	Section.pTable->OnPsiSection(pPsiSectionParser, pSection);
	if (Section.pTable->IsUpdated()) {
		Section.bUpdated = true;
		m_bTableUpdated = true;
	}
}


void CPsiTable::TableItem::ClearSection()
{
	for (size_t i = 0; i < SectionArray.size(); i++)
		delete SectionArray[i].pTable;

	SectionArray.clear();
}


/////////////////////////////////////////////////////////////////////////////
// PSIシングルテーブル抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CPsiSingleTable::CPsiSingleTable(const bool bTargetSectionExt)
	: CPsiTableBase(bTargetSectionExt)
{

}

CPsiSingleTable::~CPsiSingleTable()
{

}

void CPsiSingleTable::Reset()
{
	// 状態初期化
	CPsiTableBase::Reset();
	m_CurSection.Reset();
}

void CPsiSingleTable::OnPsiSection(const CPsiSectionParser *pPsiSectionParser, const CPsiSection *pSection)
{
	// セクションのフィルタリングを行う場合は派生クラスでオーバーライドする
	// デフォルトの実装ではセクションペイロード更新時に仮想関数に通知する
	m_bTableUpdated = false;
	if (*pSection != m_CurSection) {
		// セクションが更新された
		if (OnTableUpdate(pSection, &m_CurSection)) {
			// セクションストア
			m_CurSection = *pSection;
			m_bTableUpdated = true;
		}
	}
}

const bool CPsiSingleTable::OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection)
{
	// デフォルトの実装では何もしない
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// ストリーム型テーブル抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CPsiStreamTable::CPsiStreamTable(ISectionHandler *pHandler, const bool bTargetSectionExt, const bool bIgnoreSectionNumber)
	: CPsiTableBase(bTargetSectionExt, bIgnoreSectionNumber)
	, m_pSectionHandler(pHandler)
{

}

CPsiStreamTable::~CPsiStreamTable()
{

}

void CPsiStreamTable::Reset()
{
	// 状態初期化
	CPsiTableBase::Reset();

	if (m_pSectionHandler)
		m_pSectionHandler->OnReset(this);
}

void CPsiStreamTable::OnPsiSection(const CPsiSectionParser *pPsiSectionParser, const CPsiSection *pSection)
{
	m_bTableUpdated = false;
	if (OnTableUpdate(pSection)) {
		if (m_pSectionHandler)
			m_pSectionHandler->OnSection(this, pSection);
		m_bTableUpdated = true;
	}
}

const bool CPsiStreamTable::OnTableUpdate(const CPsiSection *pCurSection)
{
	// デフォルトの実装では何もしない
	return true;
}

CPsiStreamTable::ISectionHandler::~ISectionHandler()
{
}


/////////////////////////////////////////////////////////////////////////////
// PSI NULLテーブル抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CPsiNullTable::CPsiNullTable()
	: CTsPidMapTarget()
{

}

CPsiNullTable::~CPsiNullTable()
{

}

/* 純粋仮想関数として実装
const bool CPsiNullTable::StorePacket(const CTsPacket *pPacket)
{
	if(!pPacket)return false;
	return true;
}
*/

void CPsiNullTable::OnPidUnmapped(const WORD wPID)
{
	// インスタンスを開放する
	delete this;
}


/////////////////////////////////////////////////////////////////////////////
// PSIテーブルセット抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CPsiTableSet::CPsiTableSet(const bool bTargetSectionExt)
	: CPsiTableBase(bTargetSectionExt)
	, m_LastUpdatedTableID(0xFF)
{

}

CPsiTableSet::~CPsiTableSet()
{
	UnmapAllTables();
}

void CPsiTableSet::Reset()
{
	// 状態初期化
	CPsiTableBase::Reset();

	m_LastUpdatedTableID = 0xFF;
}

bool CPsiTableSet::MapTable(const BYTE TableID, CPsiTableBase *pTable)
{
	if (pTable == NULL)
		return false;

	UnmapTable(TableID);

	m_TableMap.insert(std::pair<BYTE, CPsiTableBase *>(TableID, pTable));

	return true;
}

bool CPsiTableSet::UnmapTable(const BYTE TableID)
{
	SectionTableMap::iterator itr = m_TableMap.find(TableID);

	if (itr == m_TableMap.end())
		return false;

	delete itr->second;

	m_TableMap.erase(itr);

	return true;
}

void CPsiTableSet::UnmapAllTables()
{
	for (SectionTableMap::iterator itr = m_TableMap.begin();
			itr != m_TableMap.end(); ++itr)
		delete itr->second;

	m_TableMap.clear();
}

CPsiTableBase * CPsiTableSet::GetTableByID(const BYTE TableID)
{
	SectionTableMap::iterator itr = m_TableMap.find(TableID);

	if (itr == m_TableMap.end())
		return NULL;

	return itr->second;
}

const CPsiTableBase * CPsiTableSet::GetTableByID(const BYTE TableID) const
{
	SectionTableMap::const_iterator itr = m_TableMap.find(TableID);

	if (itr == m_TableMap.end())
		return NULL;

	return itr->second;
}

BYTE CPsiTableSet::GetLastUpdatedTableID() const
{
	return m_LastUpdatedTableID;
}

void CPsiTableSet::OnPsiSection(const CPsiSectionParser *pPsiSectionParser, const CPsiSection *pSection)
{
	CPsiTableBase *pTable = GetTableByID(pSection->GetTableID());

	m_bTableUpdated = false;

	if (pTable) {
		pTable->OnPsiSection(pPsiSectionParser, pSection);
		if (pTable->IsUpdated()) {
			m_bTableUpdated = true;
			m_LastUpdatedTableID = pSection->GetTableID();
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
// PATテーブル抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CPatTable::CPatTable(
#ifdef _DEBUG
	bool bTrace
#endif
)
	: CPsiSingleTable()
#ifdef _DEBUG
	, m_bDebugTrace(bTrace)
#endif
{
	Reset();
}

void CPatTable::Reset()
{
	// 状態をクリアする
	CPsiSingleTable::Reset();

	m_NitPIDArray.clear();
	m_PmtPIDArray.clear();
}

const WORD CPatTable::GetTransportStreamID() const
{
	return m_CurSection.GetTableIdExtension();
}

const WORD CPatTable::GetNitPID(const WORD wIndex) const
{
	// NITのPIDを返す
	return (wIndex < m_NitPIDArray.size())? m_NitPIDArray[wIndex] : 0xFFFFU;	// 0xFFFFは未定義のPID
}

const WORD CPatTable::GetNitNum() const
{
	// NITの数を返す
	return (WORD)m_NitPIDArray.size();
}

const WORD CPatTable::GetPmtPID(const WORD wIndex) const
{
	// PMTのPIDを返す
	return (wIndex < m_PmtPIDArray.size())? m_PmtPIDArray[wIndex].wPID : 0xFFFFU;	// 0xFFFFは未定義のPID
}

const WORD CPatTable::GetProgramID(const WORD wIndex) const
{
	// Program Number IDを返す
	return (wIndex < m_PmtPIDArray.size())? m_PmtPIDArray[wIndex].wProgramID : 0x0000U;	// 0xFFFFは未定義のPID
}

const WORD CPatTable::GetProgramNum() const
{
	// PMTの数を返す
	return (WORD)m_PmtPIDArray.size();
}

const bool CPatTable::IsPmtTablePID(const WORD wPID) const
{
	// PMTのPIDかどうかを返す
	for (WORD wIndex = 0U ; wIndex < m_PmtPIDArray.size() ; wIndex++) {
		if (wPID == m_PmtPIDArray[wIndex].wPID)
			return true;
	}

	return false;
}

const bool CPatTable::OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection)
{
	const WORD wDataSize = pCurSection->GetPayloadSize();
	const BYTE *pHexData = pCurSection->GetPayloadData();

	if(wDataSize % 4U)return false;						// テーブルのサイズが不正
	if(pCurSection->GetTableID() != 0x00U)return false;	// テーブルIDが不正

	// PIDをクリアする
	m_NitPIDArray.clear();
	m_PmtPIDArray.clear();

	// テーブルを解析する

#ifdef _DEBUG
	if (m_bDebugTrace)
		TRACE(TEXT("\n------- PAT Table -------\nTS ID = %04X\n"), pCurSection->GetTableIdExtension());
#endif

	for (WORD wPos = 0 ; wPos < wDataSize ; wPos += 4, pHexData += 4) {
		TAG_PATITEM PatItem;

		PatItem.wProgramID	= ((WORD)pHexData[0] << 8) | (WORD)pHexData[1];				// +1,2
		PatItem.wPID		= ((WORD)(pHexData[2] & 0x1FU) << 8) | (WORD)pHexData[3];	// +3,4

		if (PatItem.wProgramID == 0) {
			// NITのPID
#ifdef _DEBUG
			if (m_bDebugTrace)
				TRACE(TEXT("NIT #%u [ID:%04X][PID:%04X]\n"),
					  m_NitPIDArray.size(), PatItem.wProgramID, PatItem.wPID);
#endif
			m_NitPIDArray.push_back(PatItem.wPID);
		} else {
			// PMTのPID
#ifdef _DEBUG
			if (m_bDebugTrace)
				TRACE(TEXT("PMT #%u [ID:%04X][PID:%04X]\n"),
					  m_PmtPIDArray.size(), PatItem.wProgramID, PatItem.wPID);
#endif
			m_PmtPIDArray.push_back(PatItem);
		}
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// CATテーブル抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CCatTable::CCatTable()
	: CPsiSingleTable()
{
	Reset();
}

CCatTable::~CCatTable()
{
}

void CCatTable::Reset()
{
	// 状態をクリアする
	m_DescBlock.Reset();

	CPsiSingleTable::Reset();
}

const CCaMethodDesc * CCatTable::GetCaDescBySystemID(const WORD SystemID) const
{
	for (WORD i = 0 ; i < m_DescBlock.GetDescNum() ; i++) {
		const CBaseDesc *pDesc = m_DescBlock.GetDescByIndex(i);

		if (pDesc != NULL && pDesc->GetTag() == CCaMethodDesc::DESC_TAG) {
			const CCaMethodDesc *pCaDesc = dynamic_cast<const CCaMethodDesc*>(pDesc);

			if (pCaDesc != NULL && pCaDesc->GetCaMethodID() == SystemID)
				return pCaDesc;
		}
	}

	return NULL;
}

WORD CCatTable::GetEmmPID() const
{
	const CCaMethodDesc *pCaDesc = m_DescBlock.GetDesc<CCaMethodDesc>();

	if (pCaDesc != NULL)
		return pCaDesc->GetCaPID();

	return 0xFFFF;
}

WORD CCatTable::GetEmmPID(const WORD CASystemID) const
{
	const CCaMethodDesc *pCaDesc = GetCaDescBySystemID(CASystemID);

	if (pCaDesc != NULL)
		return pCaDesc->GetCaPID();

	return 0xFFFF;
}

const CDescBlock * CCatTable::GetCatDesc() const
{
	// 記述子領域を返す
	return &m_DescBlock;
}

const bool CCatTable::OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection)
{
	if (pCurSection->GetTableID() != 0x01
			|| pCurSection->GetSectionLength() > 1021
			|| pCurSection->GetSectionNumber() != 0x00
			|| pCurSection->GetLastSectionNumber() != 0x00)
		return false;

	TRACE(TEXT("\n------- CAT Table -------\n"));

	// テーブルを解析する
	m_DescBlock.ParseBlock(pCurSection->GetPayloadData(), pCurSection->GetPayloadSize());

#ifdef _DEBUG
	for (WORD i = 0 ; i < m_DescBlock.GetDescNum() ; i++) {
		const CBaseDesc *pBaseDesc = m_DescBlock.GetDescByIndex(i);
		TRACE(TEXT("[%lu] TAG = 0x%02X LEN = %lu\n"), i, pBaseDesc->GetTag(), pBaseDesc->GetLength());
	}
#endif

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// PMTテーブル抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CPmtTable::CPmtTable(
#ifdef _DEBUG
	bool bTrace
#endif
)
	: CPsiSingleTable()
#ifdef _DEBUG
	, m_bDebugTrace(bTrace)
#endif
{
	Reset();
}

void CPmtTable::Reset()
{
	// 状態をクリアする
	CPsiSingleTable::Reset();

	m_wPcrPID = 0xFFFFU;
	m_TableDescBlock.Reset();
	m_EsInfoArray.clear();
}

const WORD CPmtTable::GetProgramNumberID() const
{
	return m_CurSection.GetTableIdExtension();
}

const WORD CPmtTable::GetPcrPID() const
{
	// PCR_PID を返す
	return m_wPcrPID;
}

const CDescBlock * CPmtTable::GetTableDesc() const
{
	// テーブルの記述子ブロックを返す
	return &m_TableDescBlock;
}

const WORD CPmtTable::GetEcmPID() const
{
	// ECMのPIDを返す
	const CCaMethodDesc *pCaMethodDesc = m_TableDescBlock.GetDesc<CCaMethodDesc>();

	return (pCaMethodDesc)? pCaMethodDesc->GetCaPID() : 0xFFFFU;
}

const WORD CPmtTable::GetEcmPID(const WORD CASystemID) const
{
	// 指定されたCA_system_idに対応するECMのPIDを返す
	for (WORD i = 0 ; i < m_TableDescBlock.GetDescNum() ; i++) {
		const CBaseDesc *pDesc = m_TableDescBlock.GetDescByIndex(i);

		if (pDesc != NULL && pDesc->GetTag() == CCaMethodDesc::DESC_TAG) {
			const CCaMethodDesc *pCaDesc = dynamic_cast<const CCaMethodDesc*>(pDesc);

			if (pCaDesc != NULL && pCaDesc->GetCaMethodID() == CASystemID)
				return pCaDesc->GetCaPID();
		}
	}

	return 0xFFFF;
}

const WORD CPmtTable::GetEsInfoNum() const
{
	// ES情報の数を返す
	return (WORD)m_EsInfoArray.size();
}

const BYTE CPmtTable::GetStreamTypeID(const WORD wIndex) const
{
	// Stream Type ID を返す
	return (wIndex < m_EsInfoArray.size())? m_EsInfoArray[wIndex].byStreamTypeID : 0x00U;	// 0x00は未定義のID
}

const WORD CPmtTable::GetEsPID(const WORD wIndex) const
{
	// Elementary Stream PID を返す
	return (wIndex < m_EsInfoArray.size())? m_EsInfoArray[wIndex].wEsPID : 0xFFFFU;			// 0xFFFFは未定義のPID
}

const CDescBlock * CPmtTable::GetItemDesc(const WORD wIndex) const
{
	// アイテムの記述子ブロックを返す
	return (wIndex < m_EsInfoArray.size())? &m_EsInfoArray[wIndex].DescBlock : NULL;
}

const bool CPmtTable::OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection)
{
	const WORD wDataSize = pCurSection->GetPayloadSize();
	const BYTE *pHexData = pCurSection->GetPayloadData();

	if (wDataSize < 4)
		return false;

	if(pCurSection->GetTableID() != 0x02U)return false;	// テーブルIDが不正

	// 状態をクリアする
	m_wPcrPID = 0xFFFFU;
	m_EsInfoArray.clear();

	// テーブルを解析する
	m_wPcrPID = ((WORD)(pHexData[0] & 0x1FU) << 8) | (WORD)pHexData[1];				// +0,1
	WORD wDescLen = ((WORD)(pHexData[2] & 0x0FU) << 8) | (WORD)pHexData[3];
	if (4 + wDescLen > wDataSize)
		return false;

	// 記述子ブロック
	m_TableDescBlock.ParseBlock(&pHexData[4], wDescLen);

#ifdef _DEBUG
	if (m_bDebugTrace)
		TRACE(TEXT("\n------- PMT Table -------\nProgram Number ID = %04X(%d)\nPCR PID = %04X\nECM PID = %04X\n"),
			pCurSection->GetTableIdExtension(), pCurSection->GetTableIdExtension(), m_wPcrPID , m_TableDescBlock.GetDesc<CCaMethodDesc>() ? m_TableDescBlock.GetDesc<CCaMethodDesc>()->GetCaPID() : 0xFFFFU);
#endif

	// ストリーム情報を解析
	for (WORD wPos = wDescLen + 4 ; wPos + 5 <= wDataSize ; wPos += 5 + wDescLen) {
		TAG_PMTITEM PmtItem;

		PmtItem.byStreamTypeID = pHexData[wPos + 0];													// +0
		PmtItem.wEsPID = ((WORD)(pHexData[wPos + 1] & 0x1FU) << 8) | (WORD)pHexData[wPos + 2];			// +1,2
		wDescLen = ((WORD)(pHexData[wPos + 3] & 0x0FU) << 8) | (WORD)pHexData[wPos + 4];				// +3,4
		if (wPos + 5 + wDescLen > wDataSize)
			break;

		// 記述子ブロック
		PmtItem.DescBlock.ParseBlock(&pHexData[wPos + 5], wDescLen);

#ifdef _DEBUG
		if (m_bDebugTrace)
			TRACE(TEXT("[%u] Stream Type ID = %02X  PID = %04X\n"),
				  m_EsInfoArray.size(), PmtItem.byStreamTypeID, PmtItem.wEsPID);
#endif

		// テーブルに追加する
		m_EsInfoArray.push_back(PmtItem);
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// SDTテーブル抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CSdtTable::CSdtTable(const BYTE TableID)
	: CPsiSingleTable()
	, m_TableID(TableID)
{

}

void CSdtTable::Reset()
{
	// 状態をクリアする
	CPsiSingleTable::Reset();

	m_wNetworkID = 0xFFFF;
	m_ServiceInfoArray.clear();
}

const BYTE CSdtTable::GetTableID() const
{
	return m_TableID;
}

const WORD CSdtTable::GetTransportStreamID() const
{
	return m_CurSection.GetTableIdExtension();
}

const WORD CSdtTable::GetNetworkID() const
{
	return m_wNetworkID;
}

const WORD CSdtTable::GetServiceNum() const
{
	// サービス数を返す
	return (WORD)m_ServiceInfoArray.size();
}

const WORD CSdtTable::GetServiceIndexByID(const WORD wServiceID)
{
	// サービスIDからインデックスを返す
	for (WORD wIndex = 0U ; wIndex < GetServiceNum() ; wIndex++) {
		if (m_ServiceInfoArray[wIndex].wServiceID == wServiceID) {
			return wIndex;
		}
	}

	// サービスIDが見つからない
	return 0xFFFFU;
}

const WORD CSdtTable::GetServiceID(const WORD wIndex) const
{
	// 	サービスIDを返す
	return (wIndex < GetServiceNum())? m_ServiceInfoArray[wIndex].wServiceID : 0xFFFFU;
}

const bool CSdtTable::GetHEITFlag(const WORD wIndex) const
{
	return (wIndex < GetServiceNum()) ? m_ServiceInfoArray[wIndex].bHEITFlag : false;
}

const bool CSdtTable::GetMEITFlag(const WORD wIndex) const
{
	return (wIndex < GetServiceNum()) ? m_ServiceInfoArray[wIndex].bMEITFlag : false;
}

const bool CSdtTable::GetLEITFlag(const WORD wIndex) const
{
	return (wIndex < GetServiceNum()) ? m_ServiceInfoArray[wIndex].bLEITFlag : false;
}

const bool CSdtTable::GetEITScheduleFlag(const WORD wIndex) const
{
	// EIT Schedule Flagを返す
	return (wIndex < GetServiceNum())? m_ServiceInfoArray[wIndex].bEITScheduleFlag : false;
}

const bool CSdtTable::GetEITPresentFollowingFlag(const WORD wIndex) const
{
	// EIT Present Followingを返す
	return (wIndex < GetServiceNum())? m_ServiceInfoArray[wIndex].bEITPresentFollowingFlag : false;
}

const BYTE CSdtTable::GetRunningStatus(const WORD wIndex) const
{
	// Running Statusを返す
	return (wIndex < GetServiceNum())? m_ServiceInfoArray[wIndex].byRunningStatus : 0xFFU;
}

const bool CSdtTable::GetFreeCaMode(const WORD wIndex) const
{
	// Free CA Modeを返す
	return (wIndex < GetServiceNum())? m_ServiceInfoArray[wIndex].bFreeCaMode : false;
}

const CDescBlock * CSdtTable::GetItemDesc(const WORD wIndex) const
{
	// アイテムの記述子ブロックを返す
	return (wIndex < m_ServiceInfoArray.size())? &m_ServiceInfoArray[wIndex].DescBlock : NULL;
}

void CSdtTable::OnPsiSection(const CPsiSectionParser *pPsiSectionParser, const CPsiSection *pSection)
{
	if (pSection->GetTableID() == m_TableID)
		CPsiSingleTable::OnPsiSection(pPsiSectionParser, pSection);
}

const bool CSdtTable::OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection)
{
	const WORD wDataSize = pCurSection->GetPayloadSize();
	const BYTE *pHexData = pCurSection->GetPayloadData();

	if (wDataSize < 3)
		return false;
	if (pCurSection->GetTableID() != m_TableID)
		return false;

	// 状態をクリアする
	m_ServiceInfoArray.clear();

	m_wNetworkID = ((WORD)pHexData[0] << 8) | (WORD)pHexData[1];

	TRACE(TEXT("\n------- SDT Table (%s) -------\nTransport Stream ID = %04X\nOriginal Network ID = %04X\n"),
		  m_TableID == TABLE_ID_ACTUAL ? TEXT("actual") : TEXT("other"),
		  pCurSection->GetTableIdExtension(), m_wNetworkID);

	// テーブルを解析する
	WORD wDescLen;
	for (WORD wPos = 3 ; wPos + 5 <= wDataSize ; wPos += 5 + wDescLen) {
		TAG_SDTITEM SdtItem;

		SdtItem.wServiceID					= ((WORD)pHexData[wPos + 0] << 8) | (WORD)pHexData[wPos + 1];
		SdtItem.bHEITFlag					= (pHexData[wPos + 2] & 0x10) != 0;
		SdtItem.bMEITFlag					= (pHexData[wPos + 2] & 0x08) != 0;
		SdtItem.bLEITFlag					= (pHexData[wPos + 2] & 0x04) != 0;
		SdtItem.bEITScheduleFlag			= (pHexData[wPos + 2] & 0x02) != 0;
		SdtItem.bEITPresentFollowingFlag	= (pHexData[wPos + 2] & 0x01) != 0;
		SdtItem.byRunningStatus				= pHexData[wPos + 3] >> 5;
		SdtItem.bFreeCaMode					= (pHexData[wPos + 3] & 0x10U)? true : false;

		// Service Descriptor
		wDescLen = ((WORD)(pHexData[wPos + 3] & 0x0FU) << 8) | (WORD)pHexData[wPos + 4];

		if (wPos + 5 + wDescLen > wDataSize)
			break;

		// 記述子ブロック
		SdtItem.DescBlock.ParseBlock(&pHexData[wPos + 5], wDescLen);

		// テーブル追加
		m_ServiceInfoArray.push_back(SdtItem);

#ifdef _DEBUG
		// デバッグ用ここから
		const CServiceDesc *pServiceDesc = SdtItem.DescBlock.GetDesc<CServiceDesc>();
		if (pServiceDesc) {
			TCHAR szServiceName[1024] = {TEXT('\0')};
			pServiceDesc->GetServiceName(szServiceName,1024);
			TRACE(TEXT("[%u] Service ID = %04X  Running Status = %01X  Free CA Mode = %u  Service Type = %02X  Service Name = %s\n"), m_ServiceInfoArray.size(), SdtItem.wServiceID, SdtItem.byRunningStatus, SdtItem.bFreeCaMode, pServiceDesc->GetServiceType(), szServiceName);
		} else {
			TRACE(TEXT("[%u] Service ID = %04X  Running Status = %01X  Free CA Mode = %u  ※サービス記述子なし\n"), m_ServiceInfoArray.size(), SdtItem.wServiceID, SdtItem.byRunningStatus, SdtItem.bFreeCaMode);
		}
		// ここまで
#endif
	}

	return true;
}


CSdtOtherTable::CSdtOtherTable()
{
}

CSdtOtherTable::~CSdtOtherTable()
{
}

CPsiTableBase * CSdtOtherTable::CreateSectionTable(const CPsiSection *pSection)
{
	return new CSdtTable(CSdtTable::TABLE_ID_OTHER);
}


CSdtTableSet::CSdtTableSet()
{
	MapTable(CSdtTable::TABLE_ID_ACTUAL, new CSdtTable(CSdtTable::TABLE_ID_ACTUAL));
	MapTable(CSdtTable::TABLE_ID_OTHER, new CSdtOtherTable());
}

CSdtTable *CSdtTableSet::GetActualSdtTable()
{
	return dynamic_cast<CSdtTable*>(GetTableByID(CSdtTable::TABLE_ID_ACTUAL));
}

CSdtOtherTable *CSdtTableSet::GetOtherSdtTable()
{
	return dynamic_cast<CSdtOtherTable*>(GetTableByID(CSdtTable::TABLE_ID_OTHER));
}


/////////////////////////////////////////////////////////////////////////////
// NITテーブル抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CNitTable::CNitTable()
	: CPsiSingleTable()
{
	Reset();
}

void CNitTable::Reset()
{
	CPsiSingleTable::Reset();
	m_wNetworkID = 0xFFFF;
	m_NetworkDescBlock.Reset();
	m_TransportStreamArray.clear();
}

const WORD CNitTable::GetNetworkID() const
{
	return m_wNetworkID;
}

const CDescBlock * CNitTable::GetNetworkDesc() const
{
	return &m_NetworkDescBlock;
}

const WORD CNitTable::GetTransportStreamNum() const
{
	return (WORD)m_TransportStreamArray.size();
}

const WORD CNitTable::GetTransportStreamID(const WORD wIndex) const
{
	if (wIndex >= GetTransportStreamNum())
		return 0xFFFF;
	return m_TransportStreamArray[wIndex].wTransportStreamID;
}

const WORD CNitTable::GetOriginalNetworkID(const WORD wIndex) const
{
	if (wIndex >= GetTransportStreamNum())
		return 0xFFFF;
	return m_TransportStreamArray[wIndex].wOriginalNetworkID;
}

const CDescBlock * CNitTable::GetItemDesc(const WORD wIndex) const
{
	if (wIndex >= GetTransportStreamNum())
		return NULL;
	return &m_TransportStreamArray[wIndex].DescBlock;
}

const bool CNitTable::OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection)
{
	const WORD wDataSize = pCurSection->GetPayloadSize();
	const BYTE *pHexData = pCurSection->GetPayloadData();

	if (wDataSize < 2)
		return false;
	if (pCurSection->GetTableID() != 0x40)
		return false;

	m_TransportStreamArray.clear();

	m_wNetworkID = pCurSection->GetTableIdExtension();

	WORD DescLength = ((WORD)(pHexData[0] & 0x0F) << 8) | pHexData[1];
	WORD Pos = 2;
	if (Pos + DescLength > wDataSize)
		return false;
	m_NetworkDescBlock.ParseBlock(&pHexData[Pos], DescLength);
	Pos += DescLength;

	WORD StreamLoopLength = ((WORD)(pHexData[Pos] & 0x0F) << 8) | pHexData[Pos + 1];
	Pos += 2;
	if (Pos + StreamLoopLength > wDataSize)
		return false;
	for (WORD i = 0 ; i < StreamLoopLength ; i += 6 + DescLength) {
		TAG_NITITEM NitItem;

		if (Pos + 6 > wDataSize)
			return false;
		NitItem.wTransportStreamID = ((WORD)pHexData[Pos + 0] << 8) | pHexData[Pos + 1];
		NitItem.wOriginalNetworkID = ((WORD)pHexData[Pos + 2] << 8) | pHexData[Pos + 3];
		DescLength = ((WORD)(pHexData[Pos + 4] & 0x0F) << 8) | pHexData[Pos + 5];
		Pos += 6;
		if (Pos + DescLength > wDataSize)
			return false;
		NitItem.DescBlock.ParseBlock(&pHexData[Pos], DescLength);
		Pos += DescLength;
		m_TransportStreamArray.push_back(NitItem);
	}

	return true;
}


WORD CNitMultiTable::GetNitSectionNum() const
{
	return GetSectionNum(0);
}

const CNitTable * CNitMultiTable::GetNitTable(const WORD SectionNo) const
{
	return dynamic_cast<const CNitTable *>(GetSection(0, SectionNo));
}

bool CNitMultiTable::IsNitComplete() const
{
	return IsExtensionComplete(0);
}

CPsiTableBase * CNitMultiTable::CreateSectionTable(const CPsiSection *pSection)
{
	return new CNitTable;
}


/////////////////////////////////////////////////////////////////////////////
// EITテーブル抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CEitTable::CEitTable()
	: CPsiSingleTable()
{
	Reset();
}

void CEitTable::Reset()
{
	// 状態をクリアする
	CPsiSingleTable::Reset();

	m_ServiceID = 0;
	m_TransportStreamID = 0;
	m_OriginalNetworkID = 0;
	m_SegmentLastSectionNumber = 0;
	m_LastTableID = 0;
	m_EventList.clear();
}

WORD CEitTable::GetServiceID() const
{
	return m_ServiceID;
}

WORD CEitTable::GetTransportStreamID() const
{
	return m_TransportStreamID;
}

WORD CEitTable::GetOriginalNetworkID() const
{
	return m_OriginalNetworkID;
}

BYTE CEitTable::GetSegmentLastSectionNumber() const
{
	return m_SegmentLastSectionNumber;
}

BYTE CEitTable::GetLastTableID() const
{
	return m_LastTableID;
}

int CEitTable::GetEventNum() const
{
	return (int)m_EventList.size();
}

const CEitTable::EventInfo * CEitTable::GetEventInfo(const int Index) const
{
	if (Index < 0 || (size_t)Index >= m_EventList.size())
		return NULL;
	return &m_EventList[Index];
}

WORD CEitTable::GetEventID(const int Index) const
{
	if (Index < 0 || (size_t)Index >= m_EventList.size())
		return 0;
	return m_EventList[Index].EventID;
}

const SYSTEMTIME * CEitTable::GetStartTime(const int Index) const
{
	if (Index < 0 || (size_t)Index >= m_EventList.size()
			|| !m_EventList[Index].bValidStartTime)
		return NULL;
	return &m_EventList[Index].StartTime;
}

DWORD CEitTable::GetDuration(const int Index) const
{
	if (Index < 0 || (size_t)Index >= m_EventList.size())
		return 0;
	return m_EventList[Index].Duration;
}

BYTE CEitTable::GetRunningStatus(const int Index) const
{
	if (Index < 0 || (size_t)Index >= m_EventList.size())
		return 0;
	return m_EventList[Index].RunningStatus;
}

bool CEitTable::GetFreeCaMode(const int Index) const
{
	// Free CA Modeを返す
	if (Index < 0 || (size_t)Index >= m_EventList.size())
		return 0;
	return m_EventList[Index].bFreeCaMode;
}

const CDescBlock * CEitTable::GetItemDesc(const int Index) const
{
	// アイテムの記述子ブロックを返す
	if (Index < 0 || (size_t)Index >= m_EventList.size())
		return NULL;
	return &m_EventList[Index].DescBlock;
}

const bool CEitTable::OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection)
{
	const BYTE TableID = pCurSection->GetTableID();
	if (TableID < 0x4E || TableID > 0x6F)
		return false;

	const WORD DataSize = pCurSection->GetPayloadSize();
	const BYTE *pData = pCurSection->GetPayloadData();

	if (DataSize < 6)
		return false;

	m_ServiceID = pCurSection->GetTableIdExtension();
	m_TransportStreamID = ((WORD)pData[0] << 8) | (WORD)pData[1];
	m_OriginalNetworkID = ((WORD)pData[2] << 8) | (WORD)pData[3];
	m_SegmentLastSectionNumber = pData[4];
	m_LastTableID = pData[5];

	m_EventList.clear();
	WORD Pos = 6;
	for (int i = 0; Pos + 12 <= DataSize; i++) {
		m_EventList.push_back(EventInfo());

		EventInfo &Info = m_EventList.back();

		Info.EventID = ((WORD)pData[Pos + 0] << 8) | (WORD)pData[Pos + 1];
		Info.bValidStartTime = CAribTime::AribToSystemTime(&pData[Pos + 2], &Info.StartTime);
		Info.Duration = CAribTime::AribBcdToSecond(&pData[Pos + 7]);
		Info.RunningStatus = pData[Pos + 10] >> 5;
		Info.bFreeCaMode = (pData[Pos + 10] & 0x10) != 0;
		WORD DescLength = (((WORD)pData[Pos + 10] & 0x0F) << 8) | (WORD)pData[Pos + 11];
		if (DescLength > 0 && Pos + 12 + DescLength <= DataSize)
			Info.DescBlock.ParseBlock(&pData[Pos + 12], DescLength);
		Pos += 12 + DescLength;
	}

	return true;
}


const CEitTable * CEitMultiTable::GetEitTable(const WORD ServiceID, const WORD SectionNo) const
{
	const int Index = GetExtensionIndexByTableID(ServiceID);
	if (Index >= 0)
		return dynamic_cast<const CEitTable *>(GetSection(Index, SectionNo));
	return NULL;
}

CPsiTableBase * CEitMultiTable::CreateSectionTable(const CPsiSection *pSection)
{
	return new CEitTable;
}


CEitPfTable::CEitPfTable()
{
	MapTable(CEitTable::TABLE_ID_PF_ACTUAL, new CEitMultiTable);
	MapTable(CEitTable::TABLE_ID_PF_OTHER, new CEitMultiTable);
}

const CEitMultiTable * CEitPfTable::GetPfActualTable() const
{
	return dynamic_cast<const CEitMultiTable *>(GetTableByID(CEitTable::TABLE_ID_PF_ACTUAL));
}

const CEitTable * CEitPfTable::GetPfActualTable(const WORD ServiceID, const bool bFollowing) const
{
	const CEitMultiTable *pTable = GetPfActualTable();
	if (pTable)
		return pTable->GetEitTable(ServiceID, bFollowing ? 1 : 0);
	return NULL;
}

const CEitMultiTable * CEitPfTable::GetPfOtherTable() const
{
	return dynamic_cast<const CEitMultiTable *>(GetTableByID(CEitTable::TABLE_ID_PF_OTHER));
}

const CEitTable * CEitPfTable::GetPfOtherTable(const WORD ServiceID, const bool bFollowing) const
{
	const CEitMultiTable *pTable = GetPfOtherTable();
	if (pTable)
		return pTable->GetEitTable(ServiceID, bFollowing ? 1 : 0);
	return NULL;
}


CEitPfActualTable::CEitPfActualTable()
{
	MapTable(CEitTable::TABLE_ID_PF_ACTUAL, new CEitMultiTable);
}

const CEitMultiTable * CEitPfActualTable::GetPfActualTable() const
{
	return dynamic_cast<const CEitMultiTable *>(GetTableByID(CEitTable::TABLE_ID_PF_ACTUAL));
}

const CEitTable * CEitPfActualTable::GetPfActualTable(const WORD ServiceID, const bool bFollowing) const
{
	const CEitMultiTable *pTable = GetPfActualTable();
	if (pTable)
		return pTable->GetEitTable(ServiceID, bFollowing ? 1 : 0);
	return NULL;
}


/////////////////////////////////////////////////////////////////////////////
// TOTテーブル抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CTotTable::CTotTable()
	: CPsiSingleTable(false)
	, m_bValidDateTime(false)
{
}

CTotTable::~CTotTable()
{
}

void CTotTable::Reset()
{
	CPsiSingleTable::Reset();
	m_bValidDateTime = false;
	m_DescBlock.Reset();
}

const bool CTotTable::GetDateTime(SYSTEMTIME *pTime) const
{
	if (pTime == NULL || !m_bValidDateTime)
		return false;
#if 0
	const int Offset = GetLocalTimeOffset();
	if (Offset == 0) {
		*pTime = m_DateTime;
	} else {
		CDateTime Time(m_DateTime);

		Time.Offset(CDateTime::MINUTES(Offset));
		Time.Get(pTime);
	}
#else
	*pTime = m_DateTime;
#endif
	return true;
}

const int CTotTable::GetLocalTimeOffset() const
{
	const CLocalTimeOffsetDesc *pLocalTimeOffset = m_DescBlock.GetDesc<CLocalTimeOffsetDesc>();

	if (pLocalTimeOffset && pLocalTimeOffset->IsValid()) {
		return pLocalTimeOffset->GetLocalTimeOffset();
	}
	return 0;
}

const CDescBlock * CTotTable::GetTotDesc() const
{
	return &m_DescBlock;
}

const bool CTotTable::OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection)
{
	const WORD DataSize = pCurSection->GetPayloadSize();
	const BYTE *pHexData = pCurSection->GetPayloadData();

	if (DataSize < 7)
		return false;
	if (pCurSection->GetTableID() != TABLE_ID)
		return false;

	m_bValidDateTime = CAribTime::AribToSystemTime(pHexData, &m_DateTime);

	WORD DescLength = (((WORD)pHexData[5] & 0x0F) << 8) | (WORD)pHexData[6];
	if (DescLength > 0 && DescLength <= DataSize - 7)
		m_DescBlock.ParseBlock(&pHexData[7], DescLength);
	else
		m_DescBlock.Reset();

/*
#ifdef _DEBUG
	if (m_bValidDateTime) {
		TRACE(TEXT("TOT : %04d/%02d/%02d %02d:%02d:%02d\n"),
			  m_DateTime.wYear, m_DateTime.wMonth, m_DateTime.wDay,
			  m_DateTime.wHour, m_DateTime.wMinute, m_DateTime.wSecond);
	}
#endif
*/

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// CDTテーブル抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CCdtTable::CCdtTable(ISectionHandler *pHandler)
	: CPsiStreamTable(pHandler)
{
	Reset();
}

CCdtTable::~CCdtTable()
{
}

void CCdtTable::Reset()
{
	CPsiStreamTable::Reset();
	m_OriginalNetworkId = 0xFFFF;
	m_DataType = DATATYPE_INVALID;
	m_DescBlock.Reset();
	m_ModuleData.ClearSize();
}

const WORD CCdtTable::GetOriginalNetworkId() const
{
	return m_OriginalNetworkId;
}

const BYTE CCdtTable::GetDataType() const
{
	return m_DataType;
}

const CDescBlock * CCdtTable::GetDesc() const
{
	return &m_DescBlock;
}

const WORD CCdtTable::GetDataModuleSize() const
{
	return (WORD)m_ModuleData.GetSize();
}

const BYTE * CCdtTable::GetDataModuleByte() const
{
	if (m_ModuleData.GetSize() == 0)
		return NULL;

	return m_ModuleData.GetData();
}

const bool CCdtTable::OnTableUpdate(const CPsiSection *pCurSection)
{
	const WORD DataSize = pCurSection->GetPayloadSize();
	const BYTE *pHexData = pCurSection->GetPayloadData();

	if (DataSize < 5)
		return false;
	if (pCurSection->GetTableID() != TABLE_ID)
		return false;

	m_OriginalNetworkId = ((WORD)pHexData[0] << 8) | (WORD)pHexData[1];
	m_DataType = pHexData[2];

	WORD DescLength = (((WORD)pHexData[3] & 0x0F) << 8) | (WORD)pHexData[4];
	if (5 + DescLength <= DataSize) {
		if (DescLength > 0)
			m_DescBlock.ParseBlock(&pHexData[7], DescLength);
		m_ModuleData.SetData(&pHexData[5 + DescLength], DataSize - (5 + DescLength));
	} else {
		m_DescBlock.Reset();
		m_ModuleData.ClearSize();
	}

#ifdef _DEBUG
	/*
	TRACE(TEXT("CDT : Data ID %04X / Network ID %04X / Data type %02X / Size %lu\n"),
		  pCurSection->GetTableIdExtension(), m_OriginalNetworkId, m_DataType, (ULONG)m_ModuleData.GetSize());
	*/
#endif

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// SDTTテーブル抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CSdttTable::CSdttTable(ISectionHandler *pHandler)
	: CPsiStreamTable(pHandler)
{
	Reset();
}

CSdttTable::~CSdttTable()
{
}

void CSdttTable::Reset()
{
	CPsiStreamTable::Reset();
	m_MakerID = 0;
	m_ModelID = 0;
	m_TransportStreamID = 0xFFFF;
	m_OriginalNetworkID = 0xFFFF;
	m_ServiceID = 0xFFFF;
	m_ContentList.clear();
}

const BYTE CSdttTable::GetMakerID() const
{
	return m_MakerID;
}

const BYTE CSdttTable::GetModelID() const
{
	return m_ModelID;
}

const bool CSdttTable::IsCommon() const
{
	return m_MakerID == 0xFF && m_ModelID == 0xFE;
}

const WORD CSdttTable::GetTransportStreamID() const
{
	return m_TransportStreamID;
}

const WORD CSdttTable::GetOriginalNetworkID() const
{
	return m_OriginalNetworkID;
}

const WORD CSdttTable::GetServiceID() const
{
	return m_ServiceID;
}

const BYTE CSdttTable::GetNumOfContents() const
{
	return (BYTE)m_ContentList.size();
}

const CSdttTable::ContentInfo * CSdttTable::GetContentInfo(const BYTE Index) const
{
	if ((size_t)Index >= m_ContentList.size())
		return NULL;

	return &m_ContentList[Index];
}

const bool CSdttTable::IsSchedule(DWORD Index) const
{
	if (Index >= m_ContentList.size())
		return false;

	return !m_ContentList[Index].ScheduleList.empty();
}

const CDescBlock * CSdttTable::GetContentDesc(DWORD Index) const
{
	if (Index >= m_ContentList.size())
		return NULL;

	return &m_ContentList[Index].DescBlock;
}

const bool CSdttTable::OnTableUpdate(const CPsiSection *pCurSection)
{
	const WORD DataSize = pCurSection->GetPayloadSize();
	const BYTE *pHexData = pCurSection->GetPayloadData();

	if (DataSize < 7)
		return false;
	if (pCurSection->GetTableID() != TABLE_ID)
		return false;

	m_MakerID = (BYTE)(pCurSection->GetTableIdExtension() >> 8);
	m_ModelID = (BYTE)(pCurSection->GetTableIdExtension() & 0xFF);
	m_TransportStreamID = ((WORD)pHexData[0] << 8) | (WORD)pHexData[1];
	m_OriginalNetworkID = ((WORD)pHexData[2] << 8) | (WORD)pHexData[3];
	m_ServiceID = ((WORD)pHexData[4] << 8) | (WORD)pHexData[5];

	/*
	TRACE(TEXT("\n------- SDTT -------\nMaker 0x%02x / Model 0x%02x / TSID 0x%04x / NID %d / SID 0x%04x\n"),
		  m_MakerID, m_ModelID, m_TransportStreamID, m_OriginalNetworkID, m_ServiceID);
	*/

	m_ContentList.clear();
	const int NumOfContents = pHexData[6];
	DWORD Pos = 7;
	for (int i = 0; i < NumOfContents; i++) {
		if (Pos + 8 > DataSize)
			break;

		ContentInfo Content;

		Content.GroupID = pHexData[Pos] >> 4;
		Content.TargetVersion = ((WORD)(pHexData[Pos] & 0x0F) << 8) | (WORD)pHexData[Pos + 1];
		Content.NewVersion = ((WORD)pHexData[Pos + 2] << 4) | (WORD)(pHexData[Pos + 3] >> 4);
		Content.DownloadLevel = (pHexData[Pos + 3] >> 2) & 0x03;
		Content.VersionIndicator = pHexData[Pos + 3] & 0x03;

		const WORD ContentDescLength = ((WORD)pHexData[Pos + 4] << 4) | (WORD)(pHexData[Pos + 5] >> 4);
		const WORD ScheduleDescLength = ((WORD)pHexData[Pos + 6] << 4) | (WORD)(pHexData[Pos + 7] >> 4);
		if (ContentDescLength < ScheduleDescLength
				|| Pos + ContentDescLength > DataSize)
			break;

		Content.ScheduleTimeShiftInformation = pHexData[Pos + 7] & 0x0F;

		Pos += 8;

		if (ScheduleDescLength) {
			for (DWORD j = 0; j + 8 <= ScheduleDescLength; j += 8) {
				ScheduleDescription Schedule;

				CAribTime::AribToSystemTime(&pHexData[Pos + j], &Schedule.StartTime);
				Schedule.Duration = CAribTime::AribBcdToSecond(&pHexData[Pos + j + 5]);

				Content.ScheduleList.push_back(Schedule);
			}

			Pos += ScheduleDescLength;
		}

		const WORD DescLength = ContentDescLength - ScheduleDescLength;
		if (DescLength > 0) {
			Content.DescBlock.ParseBlock(&pHexData[Pos], DescLength);
			Pos += DescLength;
		}

		m_ContentList.push_back(Content);

		/*
		TRACE(TEXT("[%d/%d] : Group 0x%02x / Target ver. 0x%03x / New ver. 0x%03x / Download level %d / Version indicator %d\n"),
			  i + 1, NumOfContents,
			  Content.GroupID, Content.TargetVersion, Content.NewVersion, Content.DownloadLevel, Content.VersionIndicator);
		*/
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// PCR抽象化クラス
/////////////////////////////////////////////////////////////////////////////

CPcrTable::CPcrTable()
	: CPsiNullTable()
	, m_Pcr(0)
{
}

const bool CPcrTable::StorePacket(const CTsPacket *pPacket)
{
	if (!pPacket)
		return false;

	if (pPacket->m_AdaptationField.bPcrFlag) {
		if (pPacket->m_AdaptationField.byOptionSize < 5)
			return false;
		m_Pcr =
			((PcrType)pPacket->m_AdaptationField.pOptionData[0] << 25 ) |
			((PcrType)pPacket->m_AdaptationField.pOptionData[1] << 17 ) |
			((PcrType)pPacket->m_AdaptationField.pOptionData[2] << 9 ) |
			((PcrType)pPacket->m_AdaptationField.pOptionData[3] << 1 ) |
			((PcrType)pPacket->m_AdaptationField.pOptionData[4] >> 7 );
	}

	return true;
}

CPcrTable::PcrType CPcrTable::GetPcrTimeStamp() const
{
	return m_Pcr;
}
