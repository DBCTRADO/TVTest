#include "stdafx.h"
#include "PATGenerator.h"
#include "TsTable.h"
#include "TsUtilClass.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define PMT_PID_FIRST	0x1FC8
#define PMT_PID_LAST	0x1FCF




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
	m_PidMapManager.UnmapAllTarget();
	m_ContinuityCounter = 0;

	// NITテーブルPIDマップ追加
	m_PidMapManager.MapTarget(0x0010, new CNitTable, OnNitUpdated, this);

	// PMTテーブルPIDマップ追加
	for (WORD PID = PMT_PID_FIRST; PID <= PMT_PID_LAST; PID++)
		m_PidMapManager.MapTarget(PID, new CPmtTable, OnPmtUpdated, this);
}


bool CPATGenerator::StorePacket(const CTsPacket *pPacket)
{
	const WORD PID = pPacket->GetPID();

	if (PID == 0x0000) {
		m_bHasPAT = true;
	} else if (PID == 0x0010 || (PID >= PMT_PID_FIRST && PID <= PMT_PID_LAST)) {
		m_bUpdated = false;
		if (m_PidMapManager.StorePacket(pPacket))
			//return !m_bHasPAT && m_bUpdated;
			return !m_bHasPAT && m_TransportStreamID != 0 && PID != 0x0010;
	}
	return false;
}


bool CPATGenerator::GetPAT(CTsPacket *pPacket)
{
	if (m_TransportStreamID == 0)
		return false;

	const CPmtTable *PmtList[PMT_PID_LAST - PMT_PID_FIRST + 1];
	int PmtCount = 0;
	for (int i = 0; i <= PMT_PID_LAST - PMT_PID_FIRST; i++) {
		PmtList[i] = dynamic_cast<const CPmtTable*>(m_PidMapManager.GetMapTarget(PMT_PID_FIRST + i));
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
	for (int i = 0; i <= PMT_PID_LAST - PMT_PID_FIRST; i++) {
		if (PmtList[i] != NULL) {
			WORD ServiceID = PmtList[i]->GetProgramNumberID();
			WORD PID = PMT_PID_FIRST + i;
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
	CPATGenerator *pThis = static_cast<CPATGenerator*>(pParam);
	CNitTable *pNitTable = dynamic_cast<CNitTable*>(pMapTarget);
	const WORD TransportStreamID = pNitTable->GetTransportStreamID(0);

	if (pThis->m_TransportStreamID != TransportStreamID) {
		TRACE(TEXT("CPATGenerator::OnNitUpdated() Stream ID Changed %04x -> %04x\n"),
			  pThis->m_TransportStreamID, TransportStreamID);

		pThis->m_TransportStreamID = TransportStreamID;
		pThis->m_bHasPAT = false;
	}
	pThis->m_bUpdated = true;

	TRACE(TEXT("CPATGenerator::OnNitUpdated()\n"));
}


void CALLBACK CPATGenerator::OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	CPATGenerator *pThis = static_cast<CPATGenerator*>(pParam);

	pThis->m_bUpdated = true;

	TRACE(TEXT("CPATGenerator::OnPmtUpdated()\n"));
}
