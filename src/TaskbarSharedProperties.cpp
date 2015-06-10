#include "stdafx.h"
#include "TVTest.h"
#include "TaskbarSharedProperties.h"
#include "AppMain.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CTaskbarSharedProperties::CTaskbarSharedProperties()
	: m_pHeader(nullptr)
	, m_LockTimeout(3000)
{
}


CTaskbarSharedProperties::~CTaskbarSharedProperties()
{
	Close();
}


bool CTaskbarSharedProperties::Open(LPCTSTR pszName,const CRecentChannelList *pRecentChannels)
{
	bool fExists;

	if (!m_SharedMemory.Create(pszName,
			sizeof(SharedInfoHeader)+sizeof(RecentChannelInfo)*MAX_RECENT_CHANNELS,
			&fExists)) {
		GetAppClass().AddLog(CLogItem::TYPE_ERROR,
							 TEXT("ã§óLÉÅÉÇÉä(%s)ÇçÏê¨Ç≈Ç´Ç‹ÇπÇÒÅB"),
							 pszName);
		return false;
	}

	m_pHeader=static_cast<SharedInfoHeader*>(m_SharedMemory.Map());
	if (m_pHeader==nullptr) {
		m_SharedMemory.Close();
		return false;
	}

	if (!fExists) {
		m_pHeader->Size=sizeof(SharedInfoHeader);
		m_pHeader->Version=SharedInfoHeader::VERSION_CURRENT;
		m_pHeader->MaxRecentChannels=MAX_RECENT_CHANNELS;

		if (pRecentChannels!=nullptr) {
			DWORD ChannelCount=pRecentChannels->NumChannels();
			if (ChannelCount>MAX_RECENT_CHANNELS)
				ChannelCount=MAX_RECENT_CHANNELS;

			RecentChannelInfo *pChannelList=pointer_cast<RecentChannelInfo*>(m_pHeader+1);

			for (DWORD i=0;i<ChannelCount;i++) {
				TunerChannelInfoToRecentChannelInfo(
					pRecentChannels->GetChannelInfo(ChannelCount-1-i),
					pChannelList+i);
			}

			m_pHeader->RecentChannelCount=ChannelCount;
		} else {
			m_pHeader->RecentChannelCount=0;
		}
	} else {
		if (!ValidateHeader(m_pHeader)) {
			Close();
			return false;
		}
	}

	m_SharedMemory.Unlock();

	return true;
}


void CTaskbarSharedProperties::Close()
{
	m_SharedMemory.Close();
	m_pHeader=nullptr;
}


bool CTaskbarSharedProperties::IsOpened() const
{
	return m_SharedMemory.IsOpened();
}


bool CTaskbarSharedProperties::GetRecentChannelList(CRecentChannelList *pList)
{
	if (pList==nullptr)
		return false;

	if (m_pHeader==nullptr)
		return false;

	if (!m_SharedMemory.Lock(m_LockTimeout))
		return false;

	ReadRecentChannelList(m_pHeader,pList);

	m_SharedMemory.Unlock();

	return true;
}


bool CTaskbarSharedProperties::AddRecentChannel(const CTunerChannelInfo &Info)
{
	if (m_pHeader==nullptr)
		return false;

	if (!m_SharedMemory.Lock(m_LockTimeout))
		return false;

	CRecentChannelList ChannelList;

	ReadRecentChannelList(m_pHeader,&ChannelList);
	ChannelList.Add(Info);

	DWORD ChannelCount=ChannelList.NumChannels();
	if (ChannelCount>m_pHeader->MaxRecentChannels)
		ChannelCount=m_pHeader->MaxRecentChannels;
	RecentChannelInfo *pChannelList=pointer_cast<RecentChannelInfo*>(m_pHeader+1);

	for (DWORD i=0;i<ChannelCount;i++) {
		TunerChannelInfoToRecentChannelInfo(
			ChannelList.GetChannelInfo(ChannelCount-1-i),
			pChannelList+i);
	}

	m_pHeader->RecentChannelCount=ChannelCount;

	m_SharedMemory.Unlock();

	return true;
}


bool CTaskbarSharedProperties::ClearRecentChannelList()
{
	if (m_pHeader==nullptr)
		return false;

	if (!m_SharedMemory.Lock(m_LockTimeout))
		return false;

	m_pHeader->RecentChannelCount=0;

	m_SharedMemory.Unlock();

	return true;
}


bool CTaskbarSharedProperties::ValidateHeader(const SharedInfoHeader *pHeader) const
{
	return pHeader!=nullptr
		&& pHeader->Size==sizeof(SharedInfoHeader)
		&& pHeader->Version==SharedInfoHeader::VERSION_CURRENT;
}


void CTaskbarSharedProperties::ReadRecentChannelList(
	const SharedInfoHeader *pHeader,CRecentChannelList *pList) const
{
	const RecentChannelInfo *pChannelList=pointer_cast<const RecentChannelInfo*>(pHeader+1);

	for (DWORD i=0;i<pHeader->RecentChannelCount;i++) {
		const RecentChannelInfo *pChannelInfo=pChannelList+i;
		CTunerChannelInfo ChannelInfo;

		ChannelInfo.SetSpace(pChannelInfo->Space);
		ChannelInfo.SetChannelIndex(pChannelInfo->ChannelIndex);
		ChannelInfo.SetChannelNo(pChannelInfo->ChannelNo);
		ChannelInfo.SetPhysicalChannel(pChannelInfo->PhysicalChannel);
		ChannelInfo.SetNetworkID(pChannelInfo->NetworkID);
		ChannelInfo.SetTransportStreamID(pChannelInfo->TransportStreamID);
		ChannelInfo.SetServiceID(pChannelInfo->ServiceID);
		ChannelInfo.SetServiceType(pChannelInfo->ServiceType);
		ChannelInfo.SetName(pChannelInfo->szChannelName);
		ChannelInfo.SetTunerName(pChannelInfo->szTunerName);
		pList->Add(ChannelInfo);
	}
}


void CTaskbarSharedProperties::TunerChannelInfoToRecentChannelInfo(
	const CTunerChannelInfo *pTunerChInfo,RecentChannelInfo *pChannelInfo) const
{
	pChannelInfo->Space=pTunerChInfo->GetSpace();
	pChannelInfo->ChannelIndex=pTunerChInfo->GetChannelIndex();
	pChannelInfo->ChannelNo=pTunerChInfo->GetChannelNo();
	pChannelInfo->PhysicalChannel=pTunerChInfo->GetPhysicalChannel();
	pChannelInfo->NetworkID=pTunerChInfo->GetNetworkID();
	pChannelInfo->TransportStreamID=pTunerChInfo->GetTransportStreamID();
	pChannelInfo->ServiceID=pTunerChInfo->GetServiceID();
	pChannelInfo->ServiceType=pTunerChInfo->GetServiceType();
	::lstrcpynW(pChannelInfo->szChannelName,pTunerChInfo->GetName(),MAX_CHANNEL_NAME);
	::lstrcpynW(pChannelInfo->szTunerName,pTunerChInfo->GetTunerName(),MAX_PATH);
}


}	// namespace TVTest
