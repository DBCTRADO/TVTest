#include "stdafx.h"
#include "PATGenerator.h"
#include "TsTable.h"
#include "TsUtilClass.h"
#include "../Common/DebugDef.h"




CPATGenerator::CPATGenerator()
{
	Reset();
}


CPATGenerator::~CPATGenerator()
{
}


void CPATGenerator::Reset()
{
	m_TransportStreamID = 0;
	m_bHasPAT = false;
	m_bGeneratePAT = false;
	m_PidMapManager.UnmapAllTarget();
	m_ContinuityCounter = 0;
	::ZeroMemory(m_PmtCount, sizeof(m_PmtCount));

	// NITテーブルPIDマップ追加
	m_PidMapManager.MapTarget(PID_NIT, new CNitMultiTable, OnNitUpdated, this);

	// PMTテーブルPIDマップ追加
	for (WORD PID = ONESEG_PMT_PID_FIRST; PID <= ONESEG_PMT_PID_LAST; PID++)
		m_PidMapManager.MapTarget(PID, new CPmtTable);
}


bool CPATGenerator::StorePacket(const CTsPacket *pPacket)
{
	const WORD PID = pPacket->GetPID();

	if (PID == PID_PAT) {
		m_bHasPAT = true;
	} else if (PID == PID_NIT || Is1SegPmtPid(PID)) {
		if (m_PidMapManager.StorePacket(pPacket)) {
			if (PID != PID_NIT && !m_bHasPAT) {
				if (!m_bGeneratePAT) {
					// PMTがPAT_GEN_PMT_COUNT回来る間にPATが来なければPATが無いとみなす
					static const DWORD PAT_GEN_PMT_COUNT = 5;
					int Index = PID - ONESEG_PMT_PID_FIRST;
					if (m_PmtCount[Index] < PAT_GEN_PMT_COUNT) {
						m_PmtCount[Index]++;
						if (m_PmtCount[Index] == PAT_GEN_PMT_COUNT) {
							m_bGeneratePAT = true;
							TRACE(TEXT("CPATGenerator : Generate 1Seg PAT\n"));
						}
					}
				}
				return m_bGeneratePAT && m_TransportStreamID != 0;
			}
		}
	}

	return false;
}


bool CPATGenerator::GetPAT(CTsPacket *pPacket)
{
	if (m_TransportStreamID == 0)
		return false;

	const CPmtTable *PmtList[ONESEG_PMT_PID_NUM];
	int PmtCount = 0;
	for (int i = 0; i < ONESEG_PMT_PID_NUM; i++) {
		PmtList[i] = dynamic_cast<const CPmtTable*>(m_PidMapManager.GetMapTarget(ONESEG_PMT_PID_FIRST + i));
		if (PmtList[i] != NULL && PmtList[i]->GetProgramNumberID() != 0) {
			PmtCount++;
		} else {
			if (i == 0)
				return false;	// 先頭 PMT が無い
			PmtList[i] = NULL;
		}
	}

	const WORD SectionLength = 5 + (PmtCount + 1) * 4 + 4;

	if (pPacket->SetSize(188) < 188)
		return false;
	BYTE *pData = pPacket->GetData();

	// TS header
	pData[0] = 0x47;	// Sync
	pData[1] = 0x60;
	pData[2] = 0x00;
	pData[3] = 0x10 | (m_ContinuityCounter & 0x0F);
	pData[4] = 0x00;

	// PAT
	pData[5] = 0x00;	// table_id
	pData[6] = 0xF0 | (SectionLength >> 8);
	pData[7] = SectionLength & 0xFF;
	pData[8] = m_TransportStreamID >> 8;
	pData[9] = m_TransportStreamID & 0xFF;
	pData[10] = 0xC1;	// reserved(2) + version_number(5) + current_next_indicator(1)
	pData[11] = 0x00;	// section_number
	pData[12] = 0x00;	// last_section_number

	pData[13] = 0x00;
	pData[14] = 0x00;
	pData[15] = 0xE0;	// reserved(3) + NIT PID (high)
	pData[16] = 0x10;	// NIT PID (low)

	int Pos = 17;
	for (int i = 0; i <= ONESEG_PMT_PID_LAST - ONESEG_PMT_PID_FIRST; i++) {
		if (PmtList[i] != NULL) {
			WORD ServiceID = PmtList[i]->GetProgramNumberID();
			WORD PID = ONESEG_PMT_PID_FIRST + i;
			pData[Pos + 0] = ServiceID >> 8;
			pData[Pos + 1] = ServiceID & 0xFF;
			pData[Pos + 2] = 0xE0 | (PID >> 8);
			pData[Pos + 3] = PID & 0xFF;
			Pos += 4;
		}
	}

	DWORD Crc = CCrcCalculator::CalcCrc32(&pData[5], 8 + (PmtCount + 1) * 4);
	pData[Pos + 0] = (BYTE)(Crc >> 24);
	pData[Pos + 1] = (BYTE)((Crc >> 16) & 0xFF);
	pData[Pos + 2] = (BYTE)((Crc >> 8) & 0xFF);
	pData[Pos + 3] = (BYTE)(Crc & 0xFF);
	Pos += 4;

	::FillMemory(&pData[Pos], 188 - Pos, 0xFF);

#ifdef _DEBUG
	if (pPacket->ParsePacket() != CTsPacket::EC_VALID) {
		::DebugBreak();
		return false;
	}
#else
	pPacket->ParsePacket();
#endif

	m_ContinuityCounter++;

	return true;
}


bool CPATGenerator::SetTransportStreamID(WORD TransportStreamID)
{
	if (m_TransportStreamID != 0)
		return false;
	m_TransportStreamID = TransportStreamID;
	return true;
}


void CALLBACK CPATGenerator::OnNitUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	TRACE(TEXT("CPATGenerator::OnNitUpdated()\n"));

	CPATGenerator *pThis = static_cast<CPATGenerator*>(pParam);
	CNitMultiTable *pNitMultiTable = dynamic_cast<CNitMultiTable*>(pMapTarget);
	WORD TransportStreamID = 0;

	if (pNitMultiTable != NULL) {
		const CNitTable *pNitTable = pNitMultiTable->GetNitTable(0);

		if (pNitTable != NULL) {
			const CDescBlock *pDescBlock = pNitTable->GetItemDesc(0);

			if (pDescBlock != NULL) {
				const CPartialReceptionDesc *pPartialReceptionDesc = pDescBlock->GetDesc<CPartialReceptionDesc>();
				if (pPartialReceptionDesc != NULL
						&& pPartialReceptionDesc->GetServiceNum() > 0) {
					TransportStreamID = pNitTable->GetTransportStreamID(0);
				}
			}
		}
	}

	if (pThis->m_TransportStreamID != TransportStreamID) {
		TRACE(TEXT("Stream ID Changed %04x -> %04x\n"),
			  pThis->m_TransportStreamID, TransportStreamID);

		pThis->m_TransportStreamID = TransportStreamID;
		pThis->m_bHasPAT = false;
	}
}
