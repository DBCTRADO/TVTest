#include "stdafx.h"
#include "Common.h"
#include "TsSelector.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CTsSelector::CTsSelector(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1UL, 1UL)
	, m_InputPacketCount(0)
	, m_OutputPacketCount(0)
	, m_TargetServiceID(0)
	, m_TargetPmtPID(0)
	, m_TargetStream(STREAM_ALL)
	, m_LastTSID(0)
	, m_LastPmtPID(0)
	, m_LastVersion(0)
	, m_Version(0)
{
	m_PatPacket.SetSize(TS_PACKETSIZE);
	Reset();
}


CTsSelector::~CTsSelector()
{
}


void CTsSelector::Reset(void)
{
	CBlockLock Lock(&m_DecoderLock);

	// 内部状態を初期化する
	m_PidMapManager.UnmapAllTarget();

	// PATテーブルPIDマップ追加
	m_PidMapManager.MapTarget(PID_PAT, new CPatTable, OnPatUpdated, this);
	// CATテーブルPIDマップ追加
	m_PidMapManager.MapTarget(PID_CAT, new CCatTable, OnCatUpdated, this);

	/*
	// 統計データ初期化
	m_InputPacketCount = 0;
	m_OutputPacketCount = 0;
	*/

	// 対象サービス初期化
	m_TargetServiceID = 0;
	m_TargetPmtPID = 0;
	//m_TargetStream = STREAM_ALL;

	m_PmtPIDList.clear();
	m_EmmPIDList.clear();

	m_LastTSID = 0;
	m_LastPmtPID = 0;
	m_LastVersion = 0;
	m_Version = 0;
}


const bool CTsSelector::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	/*
	if (dwInputIndex >= GetInputNum())
		return false;

	CTsPacket *pTsPacket = dynamic_cast<CTsPacket *>(pMediaData);

	// 入力メディアデータは互換性がない
	if(!pTsPacket)return false;
	*/

	CTsPacket *pTsPacket = static_cast<CTsPacket *>(pMediaData);

	// 入力パケット数カウント
	m_InputPacketCount++;

	// PIDルーティング
	m_PidMapManager.StorePacket(pTsPacket);

	if (m_TargetServiceID == 0 && m_TargetStream == STREAM_ALL) {
		OutputMedia(pMediaData);
	} else {
		WORD PID = pTsPacket->GetPID();
		if (PID < 0x0030 || IsTargetPID(PID)) {
			m_OutputPacketCount++;
			// パケットを下流デコーダにデータを渡す
			if (PID == PID_PAT && m_TargetPmtPID != 0
					&& MakePat(pTsPacket, &m_PatPacket)) {
				OutputMedia(&m_PatPacket);
			} else {
				OutputMedia(pMediaData);
			}
		}
	}

	return true;
}


ULONGLONG CTsSelector::GetInputPacketCount() const
{
	// 入力パケット数を返す
	return m_InputPacketCount;
}


ULONGLONG CTsSelector::GetOutputPacketCount() const
{
	// 出力パケット数を返す
	return m_OutputPacketCount;
}


bool CTsSelector::SetTargetServiceID(WORD ServiceID, DWORD Stream)
{
	if (m_TargetServiceID != ServiceID || m_TargetStream != Stream) {
		CBlockLock Lock(&m_DecoderLock);

		m_TargetServiceID = ServiceID;
		m_TargetPmtPID = 0;
		m_TargetStream = Stream;
		m_PmtPIDList.clear();

		CPatTable *pPatTable = dynamic_cast<CPatTable *>(m_PidMapManager.GetMapTarget(PID_PAT));
		if (pPatTable != NULL) {
			for (WORD i = 0 ; i < pPatTable->GetProgramNum() ; i++) {
				WORD PmtPID = pPatTable->GetPmtPID(i);

				if (m_TargetServiceID == 0
						|| pPatTable->GetProgramID(i) == m_TargetServiceID) {

					if (m_TargetServiceID != 0)
						m_TargetPmtPID = PmtPID;

					TAG_PMTPIDINFO PIDInfo;
					PIDInfo.ServiceID = pPatTable->GetProgramID(i);
					PIDInfo.PmtPID = PmtPID;
					PIDInfo.PcrPID = 0xFFFF;
					m_PmtPIDList.push_back(PIDInfo);

					m_PidMapManager.MapTarget(PmtPID, new CPmtTable, OnPmtUpdated, this);
				} else {
					m_PidMapManager.UnmapTarget(PmtPID);
				}
			}
		}
	}
	return true;
}


bool CTsSelector::IsTargetPID(WORD PID) const
{
	if (m_PmtPIDList.size() == 0)
		return m_TargetServiceID == 0;

	for (size_t i = 0 ; i < m_PmtPIDList.size() ; i++) {
		if (m_PmtPIDList[i].PmtPID == PID
				|| m_PmtPIDList[i].PcrPID == PID)
			return true;
		for (size_t j = 0 ; j < m_PmtPIDList[i].EsPIDs.size() ; j++) {
			if (m_PmtPIDList[i].EsPIDs[j] == PID)
				return true;
		}
		for (size_t j = 0 ; j < m_PmtPIDList[i].EcmPIDs.size() ; j++) {
			if (m_PmtPIDList[i].EcmPIDs[j] == PID)
				return true;
		}
	}

	for (size_t i = 0 ; i < m_EmmPIDList.size() ; i++) {
		if (m_EmmPIDList[i] == PID)
			return true;
	}

	return false;
}


int CTsSelector::GetServiceIndexByID(WORD ServiceID) const
{
	int Index;

	// プログラムIDからサービスインデックスを検索する
	for (Index = (int)m_PmtPIDList.size() - 1 ; Index >= 0  ; Index--) {
		if (m_PmtPIDList[Index].ServiceID == ServiceID)
			break;
	}

	return Index;
}


void CALLBACK CTsSelector::OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// PATが更新された
	CTsSelector *pThis = static_cast<CTsSelector *>(pParam);
	CPatTable *pPatTable = static_cast<CPatTable *>(pMapTarget);

	for (size_t i = 0 ; i < pThis->m_PmtPIDList.size() ; i++)
		pMapManager->UnmapTarget(pThis->m_PmtPIDList[i].PmtPID);
	pThis->m_PmtPIDList.clear();
	pThis->m_TargetPmtPID = 0;

	// PMTテーブルPIDマップ追加
	for (WORD i = 0 ; i < pPatTable->GetProgramNum() ; i++) {
		if (pThis->m_TargetServiceID == 0
				|| pPatTable->GetProgramID(i) == pThis->m_TargetServiceID) {
			WORD PmtPID = pPatTable->GetPmtPID(i);

			if (pThis->m_TargetServiceID != 0)
				pThis->m_TargetPmtPID = PmtPID;

			TAG_PMTPIDINFO PIDInfo;
			PIDInfo.ServiceID = pPatTable->GetProgramID(i);
			PIDInfo.PmtPID = PmtPID;
			PIDInfo.PcrPID = 0xFFFF;
			pThis->m_PmtPIDList.push_back(PIDInfo);

			pMapManager->MapTarget(PmtPID, new CPmtTable, OnPmtUpdated, pParam);
		}
	}
}


void CALLBACK CTsSelector::OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// PMTが更新された
	CTsSelector *pThis = static_cast<CTsSelector *>(pParam);
	CPmtTable *pPmtTable = static_cast<CPmtTable *>(pMapTarget);

	const int ServiceIndex = pThis->GetServiceIndexByID(pPmtTable->m_CurSection.GetTableIdExtension());
	if (ServiceIndex < 0)
		return;
	TAG_PMTPIDINFO &PIDInfo = pThis->m_PmtPIDList[ServiceIndex];

	// PCRのPID追加
	WORD PcrPID = pPmtTable->GetPcrPID();
	if (PcrPID < 0x1FFF) {
		PIDInfo.PcrPID = PcrPID;
	} else {
		PIDInfo.PcrPID = 0xFFFF;
	}

	// ECMのPID追加
	PIDInfo.EcmPIDs.clear();
	const CDescBlock *pDescBlock = pPmtTable->GetTableDesc();
	for (WORD i = 0 ; i < pDescBlock->GetDescNum() ; i++) {
		const CBaseDesc *pDesc = pDescBlock->GetDescByIndex(i);

		if (pDesc != NULL && pDesc->GetTag() == CCaMethodDesc::DESC_TAG) {
			const CCaMethodDesc *pCaDesc = dynamic_cast<const CCaMethodDesc*>(pDesc);

			if (pCaDesc != NULL && pCaDesc->GetCaPID() < 0x1FFF)
				PIDInfo.EcmPIDs.push_back(pCaDesc->GetCaPID());
		}
	}

	// ESのPID追加
	PIDInfo.EsPIDs.clear();
	static const BYTE StreamTypeList [] = {
		STREAM_TYPE_MPEG1,
		STREAM_TYPE_MPEG2,
		STREAM_TYPE_CAPTION,
		STREAM_TYPE_DATACARROUSEL,
		STREAM_TYPE_AAC,
		STREAM_TYPE_H264,
	};
	for (WORD i = 0 ; i < pPmtTable->GetEsInfoNum() ; i++) {
		bool bTarget;

		if (pThis->m_TargetStream == STREAM_ALL) {
			bTarget = true;
		} else {
			bTarget = false;
			const BYTE StreamType = pPmtTable->GetStreamTypeID(i);
			for (int j = 0 ; j < sizeof(StreamTypeList) ; j++) {
				if (StreamTypeList[j] == StreamType) {
					bTarget = (pThis->m_TargetStream & (1 << j)) != 0;
					break;
				}
			}
		}
		if (bTarget)
			PIDInfo.EsPIDs.push_back(pPmtTable->GetEsPID(i));
	}
}


void CALLBACK CTsSelector::OnCatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// CATが更新された
	CTsSelector *pThis = static_cast<CTsSelector *>(pParam);
	CCatTable *pCatTable = static_cast<CCatTable *>(pMapTarget);

	// EMMのPID追加
	pThis->m_EmmPIDList.clear();
	const CDescBlock *pDescBlock = pCatTable->GetCatDesc();
	for (WORD i = 0 ; i < pDescBlock->GetDescNum() ; i++) {
		const CBaseDesc *pDesc = pDescBlock->GetDescByIndex(i);

		if (pDesc != NULL && pDesc->GetTag() == CCaMethodDesc::DESC_TAG) {
			const CCaMethodDesc *pCaDesc = dynamic_cast<const CCaMethodDesc*>(pDesc);

			if (pCaDesc != NULL && pCaDesc->GetCaPID() < 0x1FFF)
				pThis->m_EmmPIDList.push_back(pCaDesc->GetCaPID());
		}
	}
}


bool CTsSelector::MakePat(const CTsPacket *pSrcPacket, CTsPacket *pDstPacket)
{
	const BYTE *pPayloadData = pSrcPacket->GetPayloadData();
	if (pPayloadData == NULL)
		return false;
	const BYTE *pSrcData = pSrcPacket->GetData();
	BYTE *pDstData = pDstPacket->GetData();
	SIZE_T HeaderSize = pPayloadData-pSrcData;

	if (!pSrcPacket->m_Header.bPayloadUnitStartIndicator)
		return false;
	SIZE_T UnitStartPos = pPayloadData[0] + 1;
	pPayloadData += UnitStartPos;
	HeaderSize += UnitStartPos;
	if (HeaderSize >= TS_PACKETSIZE)
		return false;

	::FillMemory(pDstData, TS_PACKETSIZE, 0xFF);
	::CopyMemory(pDstData, pSrcData, HeaderSize);
	pDstData += HeaderSize;

	if (pPayloadData[0] != 0)	// table_id 不正
		return false;

	WORD SectionLength = ((WORD)(pPayloadData[1]&0x0F)<<8) | pPayloadData[2];
	if (SectionLength > TS_PACKETSIZE-HeaderSize-3-4 )
		return false;

	DWORD CRC = ((DWORD)pPayloadData[3+SectionLength-4+0]<<24) |
				((DWORD)pPayloadData[3+SectionLength-4+1]<<16) |
				((DWORD)pPayloadData[3+SectionLength-4+2]<<8) |
				((DWORD)pPayloadData[3+SectionLength-4+3]);
	if (CCrcCalculator::CalcCrc32(pPayloadData, 3+SectionLength-4) != CRC)
		return false;

	WORD TSID = ((WORD)pPayloadData[3]<<8) | pPayloadData[4];
	BYTE Version = (pPayloadData[5]&0x3E)>>1;
	if (TSID != m_LastTSID) {
		m_Version = 0;
	} else if (m_TargetPmtPID != m_LastPmtPID || Version != m_LastVersion) {
		m_Version = (m_Version+1)&0x1F;
	}
	m_LastTSID = TSID;
	m_LastPmtPID = m_TargetPmtPID;
	m_LastVersion = Version;

	const BYTE *pProgramData = pPayloadData+8;
	SIZE_T Pos = 0;
	DWORD NewProgramListSize = 0;
	bool bHasPmtPID = false;
	while (Pos < (SIZE_T)SectionLength-(5+4)) {
		//WORD ProgramNumber = ((WORD)pProgramData[Pos]<<8) | pProgramData[Pos+1];
		WORD PID = ((WORD)(pProgramData[Pos+2]&0x1F)<<8) | pProgramData[Pos+3];

		if (PID == 0x0010 || PID == m_TargetPmtPID) {
			::CopyMemory(pDstData+8+NewProgramListSize, pProgramData+Pos, 4);
			NewProgramListSize += 4;
			if (PID == m_TargetPmtPID)
				bHasPmtPID = true;
		}
		Pos += 4;
	}
	if (!bHasPmtPID)
		return false;

	pDstData[0] = 0;
	pDstData[1] = (pPayloadData[1]&0xF0) | (BYTE)((NewProgramListSize+(5+4))>>8);
	pDstData[2] = (BYTE)((NewProgramListSize+(5+4))&0xFF);
	pDstData[3] = (BYTE)(TSID>>8);
	pDstData[4] = (BYTE)(TSID&0xFF);
	pDstData[5] = (pPayloadData[5]&0xC1) | (m_Version<<1);
	pDstData[6] = pPayloadData[6];
	pDstData[7] = pPayloadData[7];
	CRC = CCrcCalculator::CalcCrc32(pDstData, 8+NewProgramListSize);
	pDstData[8+NewProgramListSize+0] = (BYTE)(CRC>>24);
	pDstData[8+NewProgramListSize+1] = (BYTE)((CRC>>16)&0xFF);
	pDstData[8+NewProgramListSize+2] = (BYTE)((CRC>>8)&0xFF);
	pDstData[8+NewProgramListSize+3] = (BYTE)(CRC&0xFF);

	pDstPacket->ParsePacket();

	return true;
}
