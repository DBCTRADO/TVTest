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
#include <algorithm>
#include <utility>
#include "TVTest.h"
#include "NetworkDefinition.h"
#include "Common/DebugDef.h"


namespace TVTest
{


WORD StrToWord(const String &Str)
{
	const unsigned long Value = std::_tcstoul(Str.c_str(), nullptr, 0);
	if (Value > 0xFFFF)
		return 0;
	return static_cast<WORD>(Value);
}




const CNetworkDefinition::RemoteControlKeyIDAssignInfo CNetworkDefinition::m_DefaultKeyIDAssignList[] = {
	{ 4,   101,   103,   100,  0},	// BS 1-3ch
	{ 4,   141,   229,   100, 10},	// BS 4-12ch
	{10, 32769, 33767, 32768,  0},
};


CNetworkDefinition::CNetworkDefinition()
{
	m_NetworkInfoList.emplace_back(4, TEXT("BS"), NetworkType::BS);
	m_NetworkInfoList.emplace_back(6, TEXT("CS.SP-Basic"), NetworkType::CS);
	m_NetworkInfoList.emplace_back(7, TEXT("CS.SP-Basic"), NetworkType::CS);
	m_NetworkInfoList.emplace_back(10, TEXT("CS.SP-Premium"), NetworkType::CS);

	m_KeyIDAssignList.reserve(lengthof(m_DefaultKeyIDAssignList));
	for (const auto &e : m_DefaultKeyIDAssignList)
		m_KeyIDAssignList.push_back(e);
}


bool CNetworkDefinition::LoadSettings(CSettings &Settings)
{
	if (Settings.SetSection(TEXT("NetworkInfoList"))) {
		CSettings::EntryList Entries;

		if (Settings.GetEntries(&Entries)) {
			for (const auto &e : Entries) {
				NetworkInfo Info;

				Info.NetworkID = StrToWord(e.Name);
				if (Info.NetworkID != 0) {
					Info.Name = e.Value;
					Info.Type = GetNetworkTypeFromName(Info.Name.c_str());

					auto itr = FindNetworkInfoByID(Info.NetworkID);
					if (itr != m_NetworkInfoList.end()) {
						*itr = std::move(Info);
					} else {
						m_NetworkInfoList.push_back(std::move(Info));
					}
				}
			}
		}
	}

	if (Settings.SetSection(TEXT("RemoteControlKeyIDAssignList"))) {
		std::vector<RemoteControlKeyIDAssignInfo> List;

		for (int i = 0;; i++) {
			TCHAR szKey[32];
			String Value;

			StringFormat(szKey, TEXT("Assign{}"), i);
			if (!Settings.Read(szKey, &Value))
				break;

			std::vector<String> Array;
			StringUtility::Split(Value, TEXT(","), &Array);

			if (Array.size() >= 3) {
				RemoteControlKeyIDAssignInfo Info;

				Info.NetworkID = StrToWord(Array[0]);
				Info.FirstServiceID = StrToWord(Array[1]);
				Info.LastServiceID = StrToWord(Array[2]);
				Info.Subtrahend = 0;
				Info.Divisor = 0;
				if (Array.size() >= 4) {
					Info.Subtrahend = StrToWord(Array[3]);
					if (Array.size() >= 5) {
						Info.Divisor = StrToWord(Array[4]);
					}
				}

				List.push_back(Info);
			}
		}

		for (const RemoteControlKeyIDAssignInfo &DefInfo : m_DefaultKeyIDAssignList) {
			auto itr = std::ranges::find_if(
				List,
				[&](const RemoteControlKeyIDAssignInfo &Info) -> bool {
					return Info.NetworkID == DefInfo.NetworkID
						&& Info.FirstServiceID <= DefInfo.LastServiceID
						&& Info.LastServiceID >= DefInfo.FirstServiceID;
				});
			if (itr == List.end())
				List.push_back(DefInfo);
		}

		m_KeyIDAssignList = List;
	}

	return true;
}


const CNetworkDefinition::NetworkInfo *CNetworkDefinition::GetNetworkInfoByID(WORD NetworkID) const
{
	auto itr = FindNetworkInfoByID(NetworkID);
	if (itr == m_NetworkInfoList.end())
		return nullptr;
	return &*itr;
}


CNetworkDefinition::NetworkType CNetworkDefinition::GetNetworkType(WORD NetworkID) const
{
	const NetworkInfo *pInfo = GetNetworkInfoByID(NetworkID);
	if (pInfo == nullptr)
		return NetworkType::Terrestrial;
	return pInfo->Type;
}


bool CNetworkDefinition::IsTerrestrialNetworkID(WORD NetworkID) const
{
	return GetNetworkType(NetworkID) == NetworkType::Terrestrial;
}


bool CNetworkDefinition::IsBSNetworkID(WORD NetworkID) const
{
	return GetNetworkType(NetworkID) == NetworkType::BS;
}


bool CNetworkDefinition::IsCSNetworkID(WORD NetworkID) const
{
	return GetNetworkType(NetworkID) == NetworkType::CS;
}


bool CNetworkDefinition::IsSatelliteNetworkID(WORD NetworkID) const
{
	const NetworkType Type = GetNetworkType(NetworkID);
	return Type == NetworkType::BS || Type == NetworkType::CS;
}


int CNetworkDefinition::GetNetworkTypeOrder(WORD NetworkID1, WORD NetworkID2) const
{
	if (NetworkID1 == NetworkID2)
		return 0;
	const NetworkType Network1 = GetNetworkType(NetworkID1);
	const NetworkType Network2 = GetNetworkType(NetworkID2);
	int Order;
	if (Network1 == Network2)
		Order = 0;
	else if (Network1 == NetworkType::Unknown)
		Order = 1;
	else if (Network2 == NetworkType::Unknown)
		Order = -1;
	else
		Order = static_cast<int>(Network1) - static_cast<int>(Network2);
	return Order;
}


int CNetworkDefinition::GetRemoteControlKeyID(WORD NetworkID, WORD ServiceID) const
{
	for (const auto &e : m_KeyIDAssignList) {
		if (NetworkID == e.NetworkID
				&& ServiceID >= e.FirstServiceID
				&& ServiceID <= e.LastServiceID) {
			int KeyID = ServiceID - e.Subtrahend;
			if (e.Divisor != 0)
				KeyID /= e.Divisor;
			return KeyID;
		}
	}

	if (ServiceID < 1000)
		return ServiceID;

	return 0;
}


CNetworkDefinition::NetworkInfoList::iterator CNetworkDefinition::FindNetworkInfoByID(WORD NetworkID)
{
	return std::ranges::find(m_NetworkInfoList, NetworkID, &NetworkInfo::NetworkID);
}


CNetworkDefinition::NetworkInfoList::const_iterator CNetworkDefinition::FindNetworkInfoByID(WORD NetworkID) const
{
	return std::ranges::find(m_NetworkInfoList, NetworkID, &NetworkInfo::NetworkID);
}


CNetworkDefinition::NetworkType CNetworkDefinition::GetNetworkTypeFromName(LPCTSTR pszName)
{
	static const struct {
		NetworkType Type;
		LPCTSTR pszName;
		int Length;
	} NetworkTypeList [] = {
		{NetworkType::Terrestrial, TEXT("T"),  1},
		{NetworkType::BS,          TEXT("BS"), 2},
		{NetworkType::CS,          TEXT("CS"), 2},
	};

	for (const auto &e : NetworkTypeList) {
		const int Length = e.Length;
		if (::StrCmpNI(pszName, e.pszName, Length) == 0
				&& (pszName[Length] == _T('\0') || pszName[Length] == _T('.'))) {
			return e.Type;
		}
	}

	return NetworkType::Unknown;
}




CNetworkDefinition::NetworkInfo::NetworkInfo()
	: NetworkID(0)
	, Type(NetworkType::Unknown)
{
}


CNetworkDefinition::NetworkInfo::NetworkInfo(WORD nid, LPCTSTR name, NetworkType type)
	: NetworkID(nid)
	, Name(name)
	, Type(type)
{
}


}	// namespace TVTest
