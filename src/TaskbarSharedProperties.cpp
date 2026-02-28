/*
  TVTest
  Copyright(c) 2008-2020 DBCTRADO

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "stdafx.h"
#include "TVTest.h"
#include "TaskbarSharedProperties.h"
#include "AppMain.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CTaskbarSharedProperties::~CTaskbarSharedProperties()
{
	Close();
}


bool CTaskbarSharedProperties::Open(LPCTSTR pszName, const CRecentChannelList *pRecentChannels)
{
	bool fExists;

	if (!m_SharedMemory.Create(
				pszName,
				sizeof(SharedInfoHeader) + sizeof(RecentChannelInfo) * MAX_RECENT_CHANNELS,
				&fExists)) {
		GetAppClass().AddLog(
			CLogItem::LogType::Error,
			TEXT("共有メモリ({})を作成できません。"),
			pszName);
		return false;
	}

	m_pHeader = static_cast<SharedInfoHeader*>(m_SharedMemory.Map());
	if (m_pHeader == nullptr) {
		m_SharedMemory.Close();
		return false;
	}

	if (!fExists) {
		m_pHeader->Size = sizeof(SharedInfoHeader);
		m_pHeader->Version = SharedInfoHeader::VERSION_CURRENT;
		m_pHeader->MaxRecentChannels = MAX_RECENT_CHANNELS;

		if (pRecentChannels != nullptr) {
			DWORD ChannelCount = pRecentChannels->NumChannels();
			if (ChannelCount > MAX_RECENT_CHANNELS)
				ChannelCount = MAX_RECENT_CHANNELS;

			RecentChannelInfo *pChannelList = reinterpret_cast<RecentChannelInfo*>(m_pHeader + 1);

			for (DWORD i = 0; i < ChannelCount; i++) {
				TunerChannelInfoToRecentChannelInfo(
					pRecentChannels->GetChannelInfo(ChannelCount - 1 - i),
					pChannelList + i);
			}

			m_pHeader->RecentChannelCount = ChannelCount;
		} else {
			m_pHeader->RecentChannelCount = 0;
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
	m_pHeader = nullptr;
}


bool CTaskbarSharedProperties::IsOpened() const
{
	return m_SharedMemory.IsOpened();
}


bool CTaskbarSharedProperties::GetRecentChannelList(CRecentChannelList *pList)
{
	if (pList == nullptr)
		return false;

	if (m_pHeader == nullptr)
		return false;

	if (!m_SharedMemory.Lock(m_LockTimeout))
		return false;

	ReadRecentChannelList(m_pHeader, pList);

	m_SharedMemory.Unlock();

	return true;
}


bool CTaskbarSharedProperties::AddRecentChannel(const CTunerChannelInfo &Info)
{
	if (m_pHeader == nullptr)
		return false;

	if (!m_SharedMemory.Lock(m_LockTimeout))
		return false;

	CRecentChannelList ChannelList;

	ReadRecentChannelList(m_pHeader, &ChannelList);
	ChannelList.Add(Info);

	DWORD ChannelCount = ChannelList.NumChannels();
	if (ChannelCount > m_pHeader->MaxRecentChannels)
		ChannelCount = m_pHeader->MaxRecentChannels;
	RecentChannelInfo *pChannelList = reinterpret_cast<RecentChannelInfo*>(m_pHeader + 1);

	for (DWORD i = 0; i < ChannelCount; i++) {
		TunerChannelInfoToRecentChannelInfo(
			ChannelList.GetChannelInfo(ChannelCount - 1 - i),
			pChannelList + i);
	}

	m_pHeader->RecentChannelCount = ChannelCount;

	m_SharedMemory.Unlock();

	return true;
}


bool CTaskbarSharedProperties::ClearRecentChannelList()
{
	if (m_pHeader == nullptr)
		return false;

	if (!m_SharedMemory.Lock(m_LockTimeout))
		return false;

	m_pHeader->RecentChannelCount = 0;

	m_SharedMemory.Unlock();

	return true;
}


bool CTaskbarSharedProperties::ValidateHeader(const SharedInfoHeader *pHeader) const
{
	return pHeader != nullptr
		&& pHeader->Size == sizeof(SharedInfoHeader)
		&& pHeader->Version == SharedInfoHeader::VERSION_CURRENT;
}


void CTaskbarSharedProperties::ReadRecentChannelList(
	const SharedInfoHeader *pHeader, CRecentChannelList *pList) const
{
	const RecentChannelInfo *pChannelList = reinterpret_cast<const RecentChannelInfo*>(pHeader + 1);

	for (DWORD i = 0; i < pHeader->RecentChannelCount; i++) {
		const RecentChannelInfo *pChannelInfo = pChannelList + i;
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
	const CTunerChannelInfo *pTunerChInfo, RecentChannelInfo *pChannelInfo) const
{
	pChannelInfo->Space = pTunerChInfo->GetSpace();
	pChannelInfo->ChannelIndex = pTunerChInfo->GetChannelIndex();
	pChannelInfo->ChannelNo = pTunerChInfo->GetChannelNo();
	pChannelInfo->PhysicalChannel = pTunerChInfo->GetPhysicalChannel();
	pChannelInfo->NetworkID = pTunerChInfo->GetNetworkID();
	pChannelInfo->TransportStreamID = pTunerChInfo->GetTransportStreamID();
	pChannelInfo->ServiceID = pTunerChInfo->GetServiceID();
	pChannelInfo->ServiceType = pTunerChInfo->GetServiceType();
	StringCopy(pChannelInfo->szChannelName, pTunerChInfo->GetName());
	StringCopy(pChannelInfo->szTunerName, pTunerChInfo->GetTunerName());
}


} // namespace TVTest
