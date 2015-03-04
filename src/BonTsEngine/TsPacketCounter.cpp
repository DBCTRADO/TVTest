#include "stdafx.h"
#include "Common.h"
#include "TsPacketCounter.h"
#include "TsTable.h"
#include <utility>
#include "../Common/DebugDef.h"




CTsPacketCounter::CTsPacketCounter(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1UL, 1UL)
	, m_EsPidMapTarget(this)
	, m_TargetServiceID(0)
{
	Reset();
}


CTsPacketCounter::~CTsPacketCounter()
{
}


void CTsPacketCounter::Reset()
{
	CBlockLock Lock(&m_DecoderLock);

	m_PidMapManager.UnmapAllTarget();

	m_PidMapManager.MapTarget(PID_PAT, new CPatTable, OnPatUpdated, this);

	m_ServiceList.clear();
	m_TargetServiceID = 0;

	m_InputPacketCount.Reset();
	m_ScrambledPacketCount.Reset();
}


const bool CTsPacketCounter::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
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

	m_InputPacketCount.Increment();

	m_PidMapManager.StorePacket(pTsPacket);

	if (m_TargetServiceID == 0 && pTsPacket->IsScrambled())
		m_ScrambledPacketCount.Increment();

	OutputMedia(pTsPacket);

	return true;
}


bool CTsPacketCounter::SetActiveServiceID(WORD ServiceID)
{
	CBlockLock Lock(&m_DecoderLock);

	if (m_TargetServiceID != ServiceID) {
		m_TargetServiceID = ServiceID;

		for (size_t i = 0; i < m_ServiceList.size(); i++) {
			if (m_ServiceList[i].ServiceID != ServiceID)
				UnmapServiceESs(i);
		}
		for (size_t i = 0; i < m_ServiceList.size(); i++) {
			if (m_ServiceList[i].ServiceID == ServiceID)
				MapServiceESs(i);
		}
	}

	return true;
}


WORD CTsPacketCounter::GetActiveServiceID() const
{
	return m_TargetServiceID;
}


UINT64 CTsPacketCounter::GetInputPacketCount() const
{
	return m_InputPacketCount.Get();
}


void CTsPacketCounter::ResetInputPacketCount()
{
	m_InputPacketCount.Reset();
}


UINT64 CTsPacketCounter::GetScrambledPacketCount() const
{
	return m_ScrambledPacketCount.Get();
}


void CTsPacketCounter::ResetScrambledPacketCount()
{
	m_ScrambledPacketCount.Reset();
}


int CTsPacketCounter::GetServiceIndexByID(WORD ServiceID) const
{
	int Index;

	for (Index = (int)m_ServiceList.size() - 1; Index >= 0; Index--) {
		if (m_ServiceList[Index].ServiceID == ServiceID)
			break;
	}

	return Index;
}


void CTsPacketCounter::MapServiceESs(size_t Index)
{
	const ServiceInfo &Info = m_ServiceList[Index];

	for (size_t i = 0; i < Info.EsPidList.size(); i++)
		m_PidMapManager.MapTarget(Info.EsPidList[i], &m_EsPidMapTarget);
}


void CTsPacketCounter::UnmapServiceESs(size_t Index)
{
	const ServiceInfo &Info = m_ServiceList[Index];

	for (size_t i = 0; i < Info.EsPidList.size(); i++)
		m_PidMapManager.UnmapTarget(Info.EsPidList[i]);
}


void CALLBACK CTsPacketCounter::OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// PATが更新された
	CTsPacketCounter *pThis = static_cast<CTsPacketCounter *>(pParam);
	CPatTable *pPatTable = dynamic_cast<CPatTable *>(pMapTarget);
	if (pPatTable == nullptr)
		return;

	for (size_t i = 0; i < pThis->m_ServiceList.size(); i++) {
		pThis->UnmapServiceESs(i);
		pMapManager->UnmapTarget(pThis->m_ServiceList[i].PmtPID);
	}

	const WORD ProgramNum = pPatTable->GetProgramNum();
	pThis->m_ServiceList.clear();
	pThis->m_ServiceList.reserve(ProgramNum);

	for (WORD i = 0; i < ProgramNum; i++) {
		ServiceInfo Info;

		Info.ServiceID = pPatTable->GetProgramID(i);
		Info.PmtPID = pPatTable->GetPmtPID(i);

		pThis->m_ServiceList.push_back(Info);

		pMapManager->MapTarget(Info.PmtPID, new CPmtTable, OnPmtUpdated, pParam);
	}
}


void CALLBACK CTsPacketCounter::OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// PMTが更新された
	CTsPacketCounter *pThis = static_cast<CTsPacketCounter *>(pParam);
	CPmtTable *pPmtTable = dynamic_cast<CPmtTable *>(pMapTarget);
	if (pPmtTable == nullptr)
		return;

	const WORD ServiceID = pPmtTable->m_CurSection.GetTableIdExtension();
	const int ServiceIndex = pThis->GetServiceIndexByID(ServiceID);
	if (ServiceIndex < 0)
		return;

	ServiceInfo &Info = pThis->m_ServiceList[ServiceIndex];

	const WORD EsNum = pPmtTable->GetEsInfoNum();
	std::vector<WORD> EsPidList;
	EsPidList.resize(EsNum);
	for (WORD i = 0; i < EsNum; i++)
		EsPidList[i] = pPmtTable->GetEsPID(i);
	if (EsPidList != Info.EsPidList) {
		const bool bIsTarget = ServiceID == pThis->m_TargetServiceID;
		if (bIsTarget)
			pThis->UnmapServiceESs(ServiceIndex);
		Info.EsPidList = std::move(EsPidList);
		if (bIsTarget)
			pThis->MapServiceESs(ServiceIndex);
	}
}




CTsPacketCounter::CEsPidMapTarget::CEsPidMapTarget(CTsPacketCounter *pTsPacketCounter)
	: m_pTsPacketCounter(pTsPacketCounter)
{
}


const bool CTsPacketCounter::CEsPidMapTarget::StorePacket(const CTsPacket *pPacket)
{
	if (pPacket->IsScrambled())
		m_pTsPacketCounter->m_ScrambledPacketCount.Increment();
	return true;
}
