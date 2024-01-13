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
#include "ChannelHistory.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


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

	while (static_cast<int>(m_ChannelList.size()) - 1 > m_CurrentChannel) {
		m_ChannelList.pop_back();
	}

	m_ChannelList.emplace_back(std::make_unique<CTunerChannelInfo>(*pChannelInfo, pszDriverName));
	m_CurrentChannel++;

	if (static_cast<int>(m_ChannelList.size()) > m_MaxChannelHistory) {
		m_ChannelList.pop_front();
		m_CurrentChannel--;
	}

	return true;
}


const CTunerChannelInfo *CChannelHistory::Forward()
{
	if (m_CurrentChannel + 1 >= static_cast<int>(m_ChannelList.size()))
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
{
}


int CRecentChannelList::NumChannels() const
{
	return static_cast<int>(m_ChannelList.size());
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

	if (static_cast<int>(m_ChannelList.size()) > m_MaxChannelHistory) {
		m_ChannelList.pop_back();
	}

	return true;
}


bool CRecentChannelList::Add(const CTunerChannelInfo &ChannelInfo)
{
	return Add(ChannelInfo.GetTunerName(), &ChannelInfo);
}


bool CRecentChannelList::SetMenu(HWND hwnd, HMENU hmenu)
{
	m_ChannelMenu.Destroy();

	for (int i = ::GetMenuItemCount(hmenu) - 3; i >= 0; i--)
		::DeleteMenu(hmenu, i, MF_BYPOSITION);

	CChannelList ChannelList;

	for (int i = 0; i < m_MaxChannelHistoryMenu; i++) {
		const CTunerChannelInfo *pChannelInfo = GetChannelInfo(i);
		if (pChannelInfo == nullptr)
			break;
		ChannelList.AddChannel(*pChannelInfo);
	}

	if (ChannelList.NumChannels() > 0) {
		m_ChannelMenu.Create(
			&ChannelList, -1, CM_CHANNELHISTORY_FIRST, CM_CHANNELHISTORY_LAST, hmenu, hwnd,
			CChannelMenu::CreateFlag::ShowEventInfo |
			CChannelMenu::CreateFlag::ShowLogo |
			CChannelMenu::CreateFlag::NoClear |
			CChannelMenu::CreateFlag::IncludeDisabled);
		m_ChannelMenu.RegisterExtraItem(CM_CHANNELHISTORY_CLEAR);
	} else {
		::InsertMenu(hmenu, 0, MF_GRAYED, 0, TEXT("なし"));
	}

	::EnableMenuItem(hmenu, CM_CHANNELHISTORY_CLEAR, ChannelList.NumChannels() > 0 ? MF_ENABLED : MF_GRAYED);

	return true;
}


bool CRecentChannelList::OnMeasureItem(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	return m_ChannelMenu.OnMeasureItem(hwnd, wParam, lParam);
}


bool CRecentChannelList::OnDrawItem(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	return m_ChannelMenu.OnDrawItem(hwnd, wParam, lParam);
}


bool CRecentChannelList::OnMenuSelect(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	return m_ChannelMenu.OnMenuSelect(hwnd, wParam, lParam);
}


bool CRecentChannelList::OnUninitMenuPopup(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	return m_ChannelMenu.OnUninitMenuPopup(hwnd, wParam, lParam);
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

		StringFormat(szName, TEXT("History{}_Driver"), i);
		if (!Settings.Read(szName, szDriverName, lengthof(szDriverName))
				|| szDriverName[0] == '\0')
			break;
		StringFormat(szName, TEXT("History{}_Name"), i);
		if (!Settings.Read(szName, szChannelName, lengthof(szChannelName))
				|| szChannelName[0] == '\0')
			break;
		StringFormat(szName, TEXT("History{}_Space"), i);
		if (!Settings.Read(szName, &Space))
			break;
		StringFormat(szName, TEXT("History{}_Channel"), i);
		if (!Settings.Read(szName, &Channel))
			break;
		StringFormat(szName, TEXT("History{}_ServiceID"), i);
		if (!Settings.Read(szName, &ServiceID))
			break;
		StringFormat(szName, TEXT("History{}_NetworkID"), i);
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

		StringFormat(szName, TEXT("History{}_Driver"), i);
		Settings.Write(szName, pChannelInfo->GetTunerName());
		StringFormat(szName, TEXT("History{}_Name"), i);
		Settings.Write(szName, pChannelInfo->GetName());
		StringFormat(szName, TEXT("History{}_Space"), i);
		Settings.Write(szName, pChannelInfo->GetSpace());
		StringFormat(szName, TEXT("History{}_Channel"), i);
		Settings.Write(szName, pChannelInfo->GetChannelIndex());
		StringFormat(szName, TEXT("History{}_ServiceID"), i);
		Settings.Write(szName, pChannelInfo->GetServiceID());
		StringFormat(szName, TEXT("History{}_NetworkID"), i);
		Settings.Write(szName, pChannelInfo->GetNetworkID());
	}
	return true;
}


} // namespace TVTest
