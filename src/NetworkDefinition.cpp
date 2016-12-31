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
	unsigned long Value=std::_tcstoul(Str.c_str(),nullptr,0);
	if (Value>0xFFFF)
		return 0;
	return static_cast<WORD>(Value);
}




const CNetworkDefinition::RemoteControlKeyIDAssignInfo CNetworkDefinition::m_DefaultKeyIDAssignList[] = {
	{4,		101,	103,	100,	0},		// BS 1-3ch
	{4,		141,	229,	100,	10},	// BS 4-12ch
	{10,	32769,	33767,	32768,	0},
};


CNetworkDefinition::CNetworkDefinition()
{
	m_NetworkInfoList.push_back(NetworkInfo(4,TEXT("BS"),NETWORK_BS));
	m_NetworkInfoList.push_back(NetworkInfo(6,TEXT("CS.SP-Basic"),NETWORK_CS));
	m_NetworkInfoList.push_back(NetworkInfo(7,TEXT("CS.SP-Basic"),NETWORK_CS));
	m_NetworkInfoList.push_back(NetworkInfo(10,TEXT("CS.SP-Premium"),NETWORK_CS));

	m_KeyIDAssignList.reserve(lengthof(m_DefaultKeyIDAssignList));
	for (int i=0;i<lengthof(m_DefaultKeyIDAssignList);i++)
		m_KeyIDAssignList.push_back(m_DefaultKeyIDAssignList[i]);
}


bool CNetworkDefinition::LoadSettings(CSettings &Settings)
{
	if (Settings.SetSection(TEXT("NetworkInfoList"))) {
		CSettings::EntryList Entries;

		if (Settings.GetEntries(&Entries)) {
			for (auto itr=Entries.begin();itr!=Entries.end();++itr) {
				NetworkInfo Info;

				Info.NetworkID=StrToWord(itr->Name);
				if (Info.NetworkID!=0) {
					Info.Name=itr->Value;
					Info.Type=GetNetworkTypeFromName(Info.Name.c_str());

					auto itr=FindNetworkInfoByID(Info.NetworkID);
					if (itr!=m_NetworkInfoList.end()) {
						*itr=std::move(Info);
					} else {
						m_NetworkInfoList.push_back(std::move(Info));
					}
				}
			}
		}
	}

	if (Settings.SetSection(TEXT("RemoteControlKeyIDAssignList"))) {
		std::vector<RemoteControlKeyIDAssignInfo> List;

		for (int i=0;;i++) {
			TCHAR szKey[32];
			String Value;

			StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Assign%d"),i);
			if (!Settings.Read(szKey,&Value))
				break;

			std::vector<String> Array;
			StringUtility::Split(Value,TEXT(","),&Array);

			if (Array.size()>=3) {
				RemoteControlKeyIDAssignInfo Info;

				Info.NetworkID=StrToWord(Array[0]);
				Info.FirstServiceID=StrToWord(Array[1]);
				Info.LastServiceID=StrToWord(Array[2]);
				Info.Subtrahend=0;
				Info.Divisor=0;
				if (Array.size()>=4) {
					Info.Subtrahend=StrToWord(Array[3]);
					if (Array.size()>=5) {
						Info.Divisor=StrToWord(Array[4]);
					}
				}

				List.push_back(Info);
			}
		}

		for (int i=0;i<lengthof(m_DefaultKeyIDAssignList);i++) {
			const RemoteControlKeyIDAssignInfo &DefInfo=m_DefaultKeyIDAssignList[i];
			auto itr=std::find_if(List.begin(),List.end(),
				[&](const RemoteControlKeyIDAssignInfo &Info) -> bool {
					return Info.NetworkID==DefInfo.NetworkID
						&& Info.FirstServiceID<=DefInfo.LastServiceID
						&& Info.LastServiceID>=DefInfo.FirstServiceID;
				});
			if (itr==List.end())
				List.push_back(DefInfo);
		}

		m_KeyIDAssignList=List;
	}

	return true;
}


const CNetworkDefinition::NetworkInfo *CNetworkDefinition::GetNetworkInfoByID(WORD NetworkID) const
{
	auto itr=FindNetworkInfoByID(NetworkID);
	if (itr==m_NetworkInfoList.end())
		return nullptr;
	return &*itr;
}


CNetworkDefinition::NetworkType CNetworkDefinition::GetNetworkType(WORD NetworkID) const
{
	const NetworkInfo *pInfo=GetNetworkInfoByID(NetworkID);
	if (pInfo==nullptr)
		return NETWORK_TERRESTRIAL;
	return pInfo->Type;
}


bool CNetworkDefinition::IsTerrestrialNetworkID(WORD NetworkID) const
{
	return GetNetworkType(NetworkID)==NETWORK_TERRESTRIAL;
}


bool CNetworkDefinition::IsBSNetworkID(WORD NetworkID) const
{
	return GetNetworkType(NetworkID)==NETWORK_BS;
}


bool CNetworkDefinition::IsCSNetworkID(WORD NetworkID) const
{
	return GetNetworkType(NetworkID)==NETWORK_CS;
}


bool CNetworkDefinition::IsSatelliteNetworkID(WORD NetworkID) const
{
	NetworkType Type=GetNetworkType(NetworkID);
	return Type==NETWORK_BS || Type==NETWORK_CS;
}


int CNetworkDefinition::GetNetworkTypeOrder(WORD NetworkID1,WORD NetworkID2) const
{
	if (NetworkID1==NetworkID2)
		return 0;
	NetworkType Network1=GetNetworkType(NetworkID1);
	NetworkType Network2=GetNetworkType(NetworkID2);
	int Order;
	if (Network1==Network2)
		Order=0;
	else if (Network1==NETWORK_UNKNOWN)
		Order=1;
	else if (Network2==NETWORK_UNKNOWN)
		Order=-1;
	else
		Order=static_cast<int>(Network1)-static_cast<int>(Network2);
	return Order;
}


int CNetworkDefinition::GetRemoteControlKeyID(WORD NetworkID,WORD ServiceID) const
{
	for (auto itr=m_KeyIDAssignList.begin();itr!=m_KeyIDAssignList.end();++itr) {
		if (NetworkID==itr->NetworkID
				&& ServiceID>=itr->FirstServiceID
				&& ServiceID<=itr->LastServiceID) {
			int KeyID=ServiceID-itr->Subtrahend;
			if (itr->Divisor!=0)
				KeyID/=itr->Divisor;
			return KeyID;
		}
	}

	if (ServiceID<1000)
		return ServiceID;

	return 0;
}


CNetworkDefinition::NetworkInfoList::iterator CNetworkDefinition::FindNetworkInfoByID(WORD NetworkID)
{
	return std::find_if(m_NetworkInfoList.begin(),m_NetworkInfoList.end(),
						[&](const NetworkInfo &Info) -> bool { return Info.NetworkID==NetworkID; });
}


CNetworkDefinition::NetworkInfoList::const_iterator CNetworkDefinition::FindNetworkInfoByID(WORD NetworkID) const
{
	return std::find_if(m_NetworkInfoList.begin(),m_NetworkInfoList.end(),
						[&](const NetworkInfo &Info) -> bool { return Info.NetworkID==NetworkID; });
}


CNetworkDefinition::NetworkType CNetworkDefinition::GetNetworkTypeFromName(LPCTSTR pszName)
{
	struct {
		NetworkType Type;
		LPCTSTR pszName;
		int Length;
	} NetworkTypeList [] = {
		{NETWORK_TERRESTRIAL,	TEXT("T"),	1},
		{NETWORK_BS,			TEXT("BS"),	2},
		{NETWORK_CS,			TEXT("CS"),	2},
	};

	for (int i=0;i<lengthof(NetworkTypeList);i++) {
		const int Length=NetworkTypeList[i].Length;
		if (::StrCmpNI(pszName,NetworkTypeList[i].pszName,Length)==0
				&& (pszName[Length]==_T('\0') || pszName[Length]==_T('.'))) {
			return NetworkTypeList[i].Type;
		}
	}

	return NETWORK_UNKNOWN;
}




CNetworkDefinition::NetworkInfo::NetworkInfo()
	: NetworkID(0)
	, Type(NETWORK_UNKNOWN)
{
}


CNetworkDefinition::NetworkInfo::NetworkInfo(WORD nid,LPCTSTR name,NetworkType type)
	: NetworkID(nid)
	, Name(name)
	, Type(type)
{
}


}	// namespace TVTest
