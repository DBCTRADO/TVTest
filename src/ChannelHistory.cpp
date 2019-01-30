/*
  TVTest
  Copyright(c) 2008-2019 DBCTRADO

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
#include "ChannelHistory.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CChannelHistory::CChannelHistory()
	: m_MaxChannelHistory(20)
	, m_CurrentChannel(-1)
{
}


void CChannelHistory::Clear()
{
	m_ChannelList.clear();
	m_CurrentChannel = -1;
}


bool CChannelHistory::SetCurrentChannel(LPCTSTR pszDriverName, const CChannelInfo *pChannelInfo)
{
	if (pszDriverName == nullptr || pChannelInfo == nullptr)
		return false;

	if (m_CurrentChannel >= 0) {
		const CTunerChannelInfo *pCurChannel = m_ChannelList[m_CurrentChannel].get();

		if (IsEqualFileName(pCurChannel->GetTunerName(), pszDriverName)
				&& pCurChannel->GetChannelIndex() == pChannelInfo->GetChannelIndex()
				&& pCurChannel->GetNetworkID() == pChannelInfo->GetNetworkID()
				&& pCurChannel->GetTransportStreamID() == pChannelInfo->GetTransportStreamID()
				&& pCurChannel->GetServiceID() == pChannelInfo->GetServiceID())
			return true;
	}

	while ((int)m_ChannelList.size() - 1 > m_CurrentChannel) {
		m_ChannelList.pop_back();
	}

	m_ChannelList.emplace_back(std::make_unique<CTunerChannelInfo>(*pChannelInfo, pszDriverName));
	m_CurrentChannel++;

	if ((int)m_ChannelList.size() > m_MaxChannelHistory) {
		m_ChannelList.pop_front();
		m_CurrentChannel--;
	}

	return true;
}


const CTunerChannelInfo *CChannelHistory::Forward()
{
	if (m_CurrentChannel + 1 >= (int)m_ChannelList.size())
		return nullptr;
	return m_ChannelList[++m_CurrentChannel].get();
}


const CTunerChannelInfo *CChannelHistory::Backward()
{
	if (m_CurrentChannel < 1)
		return nullptr;
	return m_ChannelList[--m_CurrentChannel].get();
}




CRecentChannelList::CRecentChannelList()
	: CSettingsBase(TEXT("RecentChannel"))
	, m_MaxChannelHistory(20)
	, m_MaxChannelHistoryMenu(20)
{
}


int CRecentChannelList::NumChannels() const
{
	return (int)m_ChannelList.size();
}


void CRecentChannelList::Clear()
{
	m_ChannelList.clear();
}


const CTunerChannelInfo *CRecentChannelList::GetChannelInfo(int Index) const
{
	if (Index < 0 || Index >= NumChannels())
		return nullptr;
	return m_ChannelList[Index].get();
}


bool CRecentChannelList::Add(LPCTSTR pszDriverName, const CChannelInfo *pChannelInfo)
{
	if (pszDriverName == nullptr || pChannelInfo == nullptr)
		return false;

	for (auto itr = m_ChannelList.begin(); itr != m_ChannelList.end(); ++itr) {
		if (IsEqualFileName((*itr)->GetTunerName(), pszDriverName)
				&& (*itr)->GetSpace() == pChannelInfo->GetSpace()
				&& (*itr)->GetChannelIndex() == pChannelInfo->GetChannelIndex()
				&& (*itr)->GetServiceID() == pChannelInfo->GetServiceID()) {
			if (itr == m_ChannelList.begin()
					&& (*itr)->GetNetworkID() == pChannelInfo->GetNetworkID())
				return true;
			m_ChannelList.erase(itr);
			break;
		}
	}

	m_ChannelList.emplace_front(std::make_unique<CTunerChannelInfo>(*pChannelInfo, pszDriverName));

	if ((int)m_ChannelList.size() > m_MaxChannelHistory) {
		m_ChannelList.pop_back();
	}

	return true;
}


bool CRecentChannelList::Add(const CTunerChannelInfo &ChannelInfo)
{
	return Add(ChannelInfo.GetTunerName(), &ChannelInfo);
}


bool CRecentChannelList::SetMenu(HMENU hmenu, bool fClear) const
{
	ClearMenu(hmenu);

	for (int i = 0; i < m_MaxChannelHistoryMenu; i++) {
		const CTunerChannelInfo *pChannelInfo = GetChannelInfo(i);
		if (pChannelInfo == nullptr)
			break;

		TCHAR szText[64];
		int Length = 0;
		if (i < 36)
			Length = StringPrintf(szText, TEXT("&%c: "), i < 10 ? i + _T('0') : (i - 10) + _T('A'));
		CopyToMenuText(
			pChannelInfo->GetName(),
			szText + Length, lengthof(szText) - Length);
		::AppendMenu(hmenu, MFT_STRING | MFS_ENABLED, CM_CHANNELHISTORY_FIRST + i, szText);
	}

	if (fClear && NumChannels() > 0) {
		::AppendMenu(hmenu, MFT_SEPARATOR, 0, nullptr);
		::AppendMenu(hmenu, MFT_STRING | MFS_ENABLED, CM_CHANNELHISTORY_CLEAR, TEXT("履歴をクリア"));
	}

	return true;
}


bool CRecentChannelList::ReadSettings(CSettings &Settings)
{
	m_ChannelList.clear();

	int Count;

	if (!Settings.Read(TEXT("Count"), &Count))
		return false;

	if (Count > m_MaxChannelHistory)
		Count = m_MaxChannelHistory;

	for (int i = 0; i < Count; i++) {
		TCHAR szName[32], szDriverName[MAX_PATH], szChannelName[MAX_CHANNEL_NAME];
		int Space, Channel, ServiceID, NetworkID;

		StringPrintf(szName, TEXT("History%d_Driver"), i);
		if (!Settings.Read(szName, szDriverName, lengthof(szDriverName))
				|| szDriverName[0] == '\0')
			break;
		StringPrintf(szName, TEXT("History%d_Name"), i);
		if (!Settings.Read(szName, szChannelName, lengthof(szChannelName))
				|| szChannelName[0] == '\0')
			break;
		StringPrintf(szName, TEXT("History%d_Space"), i);
		if (!Settings.Read(szName, &Space))
			break;
		StringPrintf(szName, TEXT("History%d_Channel"), i);
		if (!Settings.Read(szName, &Channel))
			break;
		StringPrintf(szName, TEXT("History%d_ServiceID"), i);
		if (!Settings.Read(szName, &ServiceID))
			break;
		StringPrintf(szName, TEXT("History%d_NetworkID"), i);
		if (!Settings.Read(szName, &NetworkID))
			NetworkID = 0;
		CChannelInfo ChannelInfo(Space, Channel, 0, szChannelName);
		ChannelInfo.SetServiceID(ServiceID);
		ChannelInfo.SetNetworkID(NetworkID);
		m_ChannelList.emplace_back(std::make_unique<CTunerChannelInfo>(ChannelInfo, szDriverName));
	}

	return true;
}


bool CRecentChannelList::WriteSettings(CSettings &Settings)
{
	const int Channels = NumChannels();

	Settings.Clear();
	Settings.Write(TEXT("Count"), Channels);
	for (int i = 0; i < Channels; i++) {
		const CTunerChannelInfo *pChannelInfo = m_ChannelList[i].get();
		TCHAR szName[64];

		StringPrintf(szName, TEXT("History%d_Driver"), i);
		Settings.Write(szName, pChannelInfo->GetTunerName());
		StringPrintf(szName, TEXT("History%d_Name"), i);
		Settings.Write(szName, pChannelInfo->GetName());
		StringPrintf(szName, TEXT("History%d_Space"), i);
		Settings.Write(szName, pChannelInfo->GetSpace());
		StringPrintf(szName, TEXT("History%d_Channel"), i);
		Settings.Write(szName, pChannelInfo->GetChannelIndex());
		StringPrintf(szName, TEXT("History%d_ServiceID"), i);
		Settings.Write(szName, pChannelInfo->GetServiceID());
		StringPrintf(szName, TEXT("History%d_NetworkID"), i);
		Settings.Write(szName, pChannelInfo->GetNetworkID());
	}
	return true;
}


}	// namespace TVTest
