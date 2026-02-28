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
#include "TVTest.h"
#include "ChannelList.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CChannelInfo::CChannelInfo(int Space, int ChannelIndex, int No, LPCTSTR pszName)
	: m_Space(Space)
	, m_ChannelIndex(ChannelIndex)
	, m_ChannelNo(No)
{
	if (pszName != nullptr)
		m_Name = pszName;
}


bool CChannelInfo::SetSpace(int Space)
{
	m_Space = Space;
	return true;
}


bool CChannelInfo::SetChannelIndex(int Channel)
{
	m_ChannelIndex = Channel;
	return true;
}


bool CChannelInfo::SetChannelNo(int ChannelNo)
{
	if (ChannelNo < 0)
		return false;
	m_ChannelNo = ChannelNo;
	return true;
}


bool CChannelInfo::SetPhysicalChannel(int Channel)
{
	m_PhysicalChannel = Channel;
	return true;
}


bool CChannelInfo::SetName(LPCTSTR pszName)
{
	StringUtility::Assign(m_Name, pszName);
	return true;
}


void CChannelInfo::SetNetworkID(WORD NetworkID)
{
	m_NetworkID = NetworkID;
}


void CChannelInfo::SetTransportStreamID(WORD TransportStreamID)
{
	m_TransportStreamID = TransportStreamID;
}


void CChannelInfo::SetServiceID(WORD ServiceID)
{
	m_ServiceID = ServiceID;
}


void CChannelInfo::SetServiceType(BYTE ServiceType)
{
	m_ServiceType = ServiceType;
}




CTunerChannelInfo::CTunerChannelInfo(const CChannelInfo &ChannelInfo, LPCTSTR pszTunerName)
	: CChannelInfo(ChannelInfo)
{
	SetTunerName(pszTunerName);
}


CTunerChannelInfo &CTunerChannelInfo::operator=(const CChannelInfo &Src)
{
	CChannelInfo *p = this;
	if (p != &Src)
		*p = Src;
	return *this;
}


void CTunerChannelInfo::SetTunerName(LPCTSTR pszName)
{
	StringUtility::Assign(m_TunerName, pszName);
}




CChannelList::CChannelList(const CChannelList &Src)
{
	*this = Src;
}


CChannelList &CChannelList::operator=(const CChannelList &Src)
{
	if (&Src != this) {
		Clear();

		if (!Src.m_ChannelList.empty()) {
			m_ChannelList.reserve(Src.m_ChannelList.size());
			for (const auto &e : Src.m_ChannelList) {
				m_ChannelList.emplace_back(std::make_unique<CChannelInfo>(*e));
			}
		}
	}
	return *this;
}


int CChannelList::NumEnableChannels() const
{
	int Count = 0;

	for (const auto &e : m_ChannelList) {
		if (e->IsEnabled())
			Count++;
	}
	return Count;
}


bool CChannelList::AddChannel(const CChannelInfo &Info)
{
	return AddChannel(new CChannelInfo(Info));
}


bool CChannelList::AddChannel(CChannelInfo *pInfo)
{
	if (pInfo == nullptr)
		return false;
	m_ChannelList.emplace_back(pInfo);
	return true;
}


bool CChannelList::InsertChannel(int Index, const CChannelInfo &Info)
{
	if (Index < 0 || static_cast<size_t>(Index) > m_ChannelList.size()) {
		TRACE(TEXT("CChannelList::InsertChannel() : Out of range [{}]\n"), Index);
		return false;
	}

	auto itr = m_ChannelList.begin();
	if (Index > 0)
		std::advance(itr, Index);
	m_ChannelList.emplace(itr, std::make_unique<CChannelInfo>(Info));

	return true;
}


CChannelInfo *CChannelList::GetChannelInfo(int Index)
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_ChannelList.size()) {
		//TRACE(TEXT("CChannelList::GetChannelInfo Out of range {}\n"), Index);
		return nullptr;
	}
	return m_ChannelList[Index].get();
}


const CChannelInfo *CChannelList::GetChannelInfo(int Index) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_ChannelList.size()) {
		//TRACE(TEXT("CChannelList::GetChannelInfo Out of range {}\n"), Index);
		return nullptr;
	}
	return m_ChannelList[Index].get();
}


int CChannelList::GetSpace(int Index) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_ChannelList.size())
		return -1;
	return m_ChannelList[Index]->GetSpace();
}


int CChannelList::GetChannelIndex(int Index) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_ChannelList.size())
		return -1;
	return m_ChannelList[Index]->GetChannelIndex();
}


int CChannelList::GetChannelNo(int Index) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_ChannelList.size())
		return -1;
	return m_ChannelList[Index]->GetChannelNo();
}


int CChannelList::GetPhysicalChannel(int Index) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_ChannelList.size())
		return -1;
	return m_ChannelList[Index]->GetPhysicalChannel();
}


LPCTSTR CChannelList::GetName(int Index) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_ChannelList.size())
		//return nullptr;
		return TEXT("");
	return m_ChannelList[Index]->GetName();
}


bool CChannelList::IsEnabled(int Index) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_ChannelList.size())
		return false;
	return m_ChannelList[Index]->IsEnabled();
}


bool CChannelList::DeleteChannel(int Index)
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_ChannelList.size())
		return false;

	auto itr = m_ChannelList.begin();
	if (Index > 0)
		std::advance(itr, Index);
	m_ChannelList.erase(itr);

	return true;
}


void CChannelList::Clear()
{
	m_ChannelList.clear();
}


int CChannelList::Find(const CChannelInfo *pInfo) const
{
	for (size_t i = 0; i < m_ChannelList.size(); i++) {
		if (m_ChannelList[i].get() == pInfo)
			return static_cast<int>(i);
	}
	return -1;
}


int CChannelList::Find(const CChannelInfo &Info, bool fEnabledOnly) const
{
	for (size_t i = 0; i < m_ChannelList.size(); i++) {
		const CChannelInfo *pChInfo = m_ChannelList[i].get();

		if ((!fEnabledOnly || pChInfo->IsEnabled())
				&& (Info.GetSpace() < 0
					|| pChInfo->GetSpace() == Info.GetSpace())
				&& (Info.GetChannelIndex() < 0
					|| pChInfo->GetChannelIndex() == Info.GetChannelIndex())
				&& (Info.GetChannelNo() <= 0
					|| pChInfo->GetChannelNo() == Info.GetChannelNo())
				&& (Info.GetPhysicalChannel() <= 0
					|| pChInfo->GetPhysicalChannel() == Info.GetPhysicalChannel())
				&& (Info.GetNetworkID() == 0
					|| pChInfo->GetNetworkID() == Info.GetNetworkID())
				&& (Info.GetTransportStreamID() == 0
					|| pChInfo->GetTransportStreamID() == Info.GetTransportStreamID())
				&& (Info.GetServiceID() == 0
					|| pChInfo->GetServiceID() == Info.GetServiceID()))
			return static_cast<int>(i);
	}
	return -1;
}


int CChannelList::FindByIndex(int Space, int ChannelIndex, int ServiceID, bool fEnabledOnly) const
{
	for (size_t i = 0; i < m_ChannelList.size(); i++) {
		const CChannelInfo *pChInfo = m_ChannelList[i].get();

		if ((!fEnabledOnly || pChInfo->IsEnabled())
				&& (Space < 0 || pChInfo->GetSpace() == Space)
				&& (ChannelIndex < 0 || pChInfo->GetChannelIndex() == ChannelIndex)
				&& (ServiceID <= 0 || pChInfo->GetServiceID() == ServiceID))
			return static_cast<int>(i);
	}
	return -1;
}


int CChannelList::FindPhysicalChannel(int Channel) const
{
	for (size_t i = 0; i < m_ChannelList.size(); i++) {
		if (m_ChannelList[i]->GetPhysicalChannel() == Channel)
			return static_cast<int>(i);
	}
	return -1;
}


int CChannelList::FindChannelNo(int No, bool fEnabledOnly) const
{
	for (size_t i = 0; i < m_ChannelList.size(); i++) {
		const CChannelInfo *pChInfo = m_ChannelList[i].get();
		if (pChInfo->GetChannelNo() == No
				&& (!fEnabledOnly || pChInfo->IsEnabled()))
			return static_cast<int>(i);
	}
	return -1;
}


int CChannelList::FindServiceID(WORD ServiceID) const
{
	for (size_t i = 0; i < m_ChannelList.size(); i++) {
		if (m_ChannelList[i]->GetServiceID() == ServiceID)
			return static_cast<int>(i);
	}
	return -1;
}


int CChannelList::FindByIDs(WORD NetworkID, WORD TransportStreamID, WORD ServiceID, bool fEnabledOnly) const
{
	for (size_t i = 0; i < m_ChannelList.size(); i++) {
		const CChannelInfo *pChannelInfo = m_ChannelList[i].get();
		if ((!fEnabledOnly || pChannelInfo->IsEnabled())
				&& (NetworkID == 0 || pChannelInfo->GetNetworkID() == NetworkID)
				&& (TransportStreamID == 0 || pChannelInfo->GetTransportStreamID() == TransportStreamID)
				&& (ServiceID == 0 || pChannelInfo->GetServiceID() == ServiceID))
			return static_cast<int>(i);
	}
	return -1;
}


int CChannelList::FindByName(LPCTSTR pszName) const
{
	if (pszName == nullptr)
		return -1;
	for (size_t i = 0; i < m_ChannelList.size(); i++) {
		if (::lstrcmp(m_ChannelList[i]->GetName(), pszName) == 0)
			return static_cast<int>(i);
	}
	return -1;
}


int CChannelList::GetNextChannel(int Index, bool fWrap) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_ChannelList.size())
		return -1;

	const int ChannelNo = GetChannelNo(Index);

	for (int i = Index + 1; i < static_cast<int>(m_ChannelList.size()); i++) {
		const CChannelInfo *pChInfo = m_ChannelList[i].get();

		if (pChInfo->IsEnabled() && pChInfo->GetChannelNo() == ChannelNo)
			return i;
	}

	int Channel = INT_MAX, Min = INT_MAX;

	for (const auto &ChInfo : m_ChannelList) {
		if (ChInfo->IsEnabled()) {
			const int No = ChInfo->GetChannelNo();
			if (No != 0) {
				if (No > ChannelNo && No < Channel)
					Channel = No;
				if (No < Min)
					Min = No;
			}
		}
	}
	if (Channel == INT_MAX) {
		if (Min == INT_MAX || !fWrap)
			return -1;
		Channel = Min;
	}
	return FindChannelNo(Channel);
}


int CChannelList::GetPrevChannel(int Index, bool fWrap) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_ChannelList.size())
		return -1;

	const int ChannelNo = GetChannelNo(Index);

	for (int i = Index - 1; i >= 0; i--) {
		const CChannelInfo *pChInfo = m_ChannelList[i].get();

		if (pChInfo->IsEnabled() && pChInfo->GetChannelNo() == ChannelNo)
			return i;
	}

	int Channel = 0, Max = 0;

	for (const auto &ChInfo : m_ChannelList) {
		if (ChInfo->IsEnabled()) {
			const int No = ChInfo->GetChannelNo();
			if (No != 0) {
				if (No < ChannelNo && No > Channel)
					Channel = No;
				if (No > Max)
					Max = No;
			}
		}
	}
	if (Channel == 0) {
		if (!fWrap)
			return -1;
		Channel = Max;
	}

	for (int i = static_cast<int>(m_ChannelList.size()) - 1; i >= 0; i--) {
		const CChannelInfo *pChInfo = m_ChannelList[i].get();

		if (pChInfo->IsEnabled() && pChInfo->GetChannelNo() == Channel)
			return i;
	}

	return -1;
}


int CChannelList::GetMaxChannelNo() const
{
	int Max = 0;

	for (const auto &ChInfo : m_ChannelList) {
		const int No = ChInfo->GetChannelNo();
		if (No > Max)
			Max = No;
	}
	return Max;
}


bool CChannelList::Sort(SortType Type, bool fDescending)
{
	if (!CheckEnumRange(Type))
		return false;

	if (m_ChannelList.size() > 1) {
		class CPredicator
		{
			SortType m_Type;
			bool m_fDescending;

		public:
			CPredicator(SortType Type, bool fDescending)
				: m_Type(Type)
				, m_fDescending(fDescending)
			{
			}

			bool operator()(
				const std::unique_ptr<CChannelInfo> &Channel1,
				const std::unique_ptr<CChannelInfo> &Channel2)
			{
				int Cmp;

				switch (m_Type) {
				case SortType::Space:
					Cmp = Channel1->GetSpace() - Channel2->GetSpace();
					break;
				case SortType::ChannelIndex:
					Cmp = Channel1->GetChannelIndex() - Channel2->GetChannelIndex();
					break;
				case SortType::ChannelNo:
					Cmp = Channel1->GetChannelNo() - Channel2->GetChannelNo();
					break;
				case SortType::PhysicalChannel:
					Cmp = Channel1->GetPhysicalChannel() - Channel2->GetPhysicalChannel();
					break;
				case SortType::Name:
					Cmp = ::lstrcmpi(Channel1->GetName(), Channel2->GetName());
					if (Cmp == 0)
						Cmp = ::lstrcmp(Channel1->GetName(), Channel2->GetName());
					break;
				case SortType::NetworkID:
					Cmp = Channel1->GetNetworkID() - Channel2->GetNetworkID();
					break;
				case SortType::ServiceID:
					Cmp = Channel1->GetServiceID() - Channel2->GetServiceID();
					break;
				}

				return m_fDescending ? Cmp > 0 : Cmp < 0;
			}
		};

		std::ranges::stable_sort(m_ChannelList, CPredicator(Type, fDescending));
	}

	return true;
}


bool CChannelList::HasRemoteControlKeyID() const
{
	for (const auto &ChInfo : m_ChannelList) {
		if (ChInfo->GetChannelNo() != 0)
			return true;
	}
	return false;
}


bool CChannelList::HasMultiService() const
{
	for (size_t i = 0; i + 1 < m_ChannelList.size(); i++) {
		const CChannelInfo *pChannelInfo1 = m_ChannelList[i].get();

		for (size_t j = i + 1; j < m_ChannelList.size(); j++) {
			const CChannelInfo *pChannelInfo2 = m_ChannelList[j].get();

			if (pChannelInfo1->GetNetworkID() == pChannelInfo2->GetNetworkID()
					&& pChannelInfo1->GetTransportStreamID() == pChannelInfo2->GetTransportStreamID()
					&& pChannelInfo1->GetServiceID() != pChannelInfo2->GetServiceID())
				return true;
		}
	}
	return false;
}




CTuningSpaceInfo::CTuningSpaceInfo(const CTuningSpaceInfo &Info)
{
	Create(Info.m_ChannelList.get(), Info.m_Name.c_str());
}


CTuningSpaceInfo &CTuningSpaceInfo::operator=(const CTuningSpaceInfo &Info)
{
	if (&Info != this)
		Create(Info.m_ChannelList.get(), Info.m_Name.c_str());
	return *this;
}


bool CTuningSpaceInfo::Create(const CChannelList *pList, LPCTSTR pszName)
{
	if (pList != nullptr)
		m_ChannelList = std::make_unique<CChannelList>(*pList);
	else
		m_ChannelList = std::make_unique<CChannelList>();
	SetName(pszName);
	return true;
}


const CChannelInfo *CTuningSpaceInfo::GetChannelInfo(int Index) const
{
	if (!m_ChannelList)
		return nullptr;
	return m_ChannelList->GetChannelInfo(Index);
}


CChannelInfo *CTuningSpaceInfo::GetChannelInfo(int Index)
{
	if (!m_ChannelList)
		return nullptr;
	return m_ChannelList->GetChannelInfo(Index);
}


bool CTuningSpaceInfo::SetName(LPCTSTR pszName)
{
	// チューニング空間の種類を判定する
	// BonDriverから取得できないので苦肉の策
	m_Space = TuningSpaceType::Unknown;
	if (!IsStringEmpty(pszName)) {
		m_Name = pszName;
		if (::StrStr(pszName, TEXT("地")) != nullptr
				|| ::StrStrI(pszName, TEXT("VHF")) != nullptr
				|| ::StrStrI(pszName, TEXT("UHF")) != nullptr
				|| ::StrStrI(pszName, TEXT("CATV")) != nullptr) {
			m_Space = TuningSpaceType::Terrestrial;
		} else if (::StrStrI(pszName, TEXT("BS")) != nullptr) {
			m_Space = TuningSpaceType::BS;
		} else if (::StrStrI(pszName, TEXT("CS")) != nullptr) {
			m_Space = TuningSpaceType::CS110;
		}
	} else {
		m_Name.clear();
	}
	return true;
}


int CTuningSpaceInfo::NumChannels() const
{
	if (!m_ChannelList)
		return 0;
	return m_ChannelList->NumChannels();
}




CTuningSpaceList::CTuningSpaceList(const CTuningSpaceList &List)
{
	*this = List;
}


CTuningSpaceList &CTuningSpaceList::operator=(const CTuningSpaceList &List)
{
	if (&List != this) {
		Clear();
		if (List.NumSpaces() > 0) {
			m_TuningSpaceList.resize(List.m_TuningSpaceList.size());
			for (size_t i = 0; i < List.m_TuningSpaceList.size(); i++)
				m_TuningSpaceList[i] = std::make_unique<CTuningSpaceInfo>(*List.m_TuningSpaceList[i]);
		}
		m_AllChannelList = List.m_AllChannelList;
	}
	return *this;
}


CTuningSpaceInfo *CTuningSpaceList::GetTuningSpaceInfo(int Space)
{
	if (Space < 0 || Space >= NumSpaces())
		return nullptr;
	return m_TuningSpaceList[Space].get();
}


const CTuningSpaceInfo *CTuningSpaceList::GetTuningSpaceInfo(int Space) const
{
	if (Space < 0 || Space >= NumSpaces())
		return nullptr;
	return m_TuningSpaceList[Space].get();
}


CChannelList *CTuningSpaceList::GetChannelList(int Space)
{
	if (Space < 0 || Space >= NumSpaces())
		return nullptr;
	return m_TuningSpaceList[Space]->GetChannelList();
}


const CChannelList *CTuningSpaceList::GetChannelList(int Space) const
{
	if (Space < 0 || Space >= NumSpaces())
		return nullptr;
	return m_TuningSpaceList[Space]->GetChannelList();
}


LPCTSTR CTuningSpaceList::GetTuningSpaceName(int Space) const
{
	if (Space < 0 || Space >= NumSpaces())
		return nullptr;
	return m_TuningSpaceList[Space]->GetName();
}


CTuningSpaceInfo::TuningSpaceType CTuningSpaceList::GetTuningSpaceType(int Space) const
{
	if (Space < 0 || Space >= NumSpaces()) {
		return CTuningSpaceInfo::TuningSpaceType::Error;
	}
	return m_TuningSpaceList[Space]->GetType();
}


bool CTuningSpaceList::MakeTuningSpaceList(const CChannelList *pList, int Spaces)
{
	for (int i = 0; i < pList->NumChannels(); i++) {
		const int Space = pList->GetSpace(i);
		if (Space + 1 > Spaces)
			Spaces = Space + 1;
	}
	if (Spaces < 1)
		return false;
	if (!Reserve(Spaces))
		return false;
	for (int i = 0; i < pList->NumChannels(); i++) {
		const CChannelInfo *pChInfo = pList->GetChannelInfo(i);

		m_TuningSpaceList[pChInfo->GetSpace()]->GetChannelList()->AddChannel(*pChInfo);
	}
	return true;
}


bool CTuningSpaceList::Create(const CChannelList *pList, int Spaces)
{
	Clear();
	if (!MakeTuningSpaceList(pList, Spaces))
		return false;
	m_AllChannelList = *pList;
	return true;
}


bool CTuningSpaceList::Reserve(int Spaces)
{
	if (Spaces < 0)
		return false;
	if (Spaces == NumSpaces())
		return true;
	if (Spaces == 0) {
		Clear();
		return true;
	}
	if (Spaces < NumSpaces()) {
		m_TuningSpaceList.resize(Spaces);
	} else {
		for (int i = NumSpaces(); i < Spaces; i++) {
			std::unique_ptr<CTuningSpaceInfo> Info = std::make_unique<CTuningSpaceInfo>();

			Info->Create();
			m_TuningSpaceList.emplace_back(std::move(Info));
		}
	}
	return true;
}


void CTuningSpaceList::Clear()
{
	m_TuningSpaceList.clear();
	m_AllChannelList.Clear();
}


bool CTuningSpaceList::MakeAllChannelList()
{
	m_AllChannelList.Clear();
	for (int i = 0; i < NumSpaces(); i++) {
		CChannelList *pList = m_TuningSpaceList[i]->GetChannelList();

		for (int j = 0; j < pList->NumChannels(); j++) {
			m_AllChannelList.AddChannel(*pList->GetChannelInfo(j));
		}
	}
	return true;
}


static constexpr UINT CP_SHIFT_JIS = 932;

bool CTuningSpaceList::SaveToFile(LPCTSTR pszFileName) const
{
	TRACE(TEXT("CTuningSpaceList::SaveToFile() : \"{}\"\n"), pszFileName);

	String Buffer;

	Buffer =
		TEXT("; ") APP_NAME TEXT(" チャンネル設定ファイル\r\n")
		TEXT("; 名称,チューニング空間,チャンネル,リモコン番号,サービスタイプ,サービスID,ネットワークID,TSID,状態\r\n");

	for (int i = 0; i < NumSpaces(); i++) {
		const CChannelList *pChannelList = m_TuningSpaceList[i]->GetChannelList();
		TCHAR szText[MAX_CHANNEL_NAME + 256];

		if (pChannelList->NumChannels() == 0)
			continue;

		if (GetTuningSpaceName(i) != nullptr) {
			StringFormat(szText, TEXT(";#SPACE({},{})\r\n"), i, GetTuningSpaceName(i));
			Buffer += szText;
		}

		for (int j = 0; j < pChannelList->NumChannels(); j++) {
			const CChannelInfo *pChInfo = pChannelList->GetChannelInfo(j);
			LPCTSTR pszName = pChInfo->GetName();
			String Name;

			// 必要に応じて " で囲む
			if (pszName[0] == _T('#') || pszName[0] == _T(';')
					|| ::StrChr(pszName, _T(',')) != nullptr
					|| ::StrChr(pszName, _T('"')) != nullptr) {
				LPCTSTR p = pszName;
				Name = _T('"');
				while (*p != _T('\0')) {
					if (*p == _T('"')) {
						Name += TEXT("\"\"");
						p++;
					} else {
#ifdef UNICODE
						int SrcLength;
						for (SrcLength = 1; p[SrcLength] != L'"' && p[SrcLength] != L'\0'; SrcLength++);
						Name.append(p, SrcLength);
						p += SrcLength;
#else
						if (::IsDBCSLeadByteEx(CP_ACP, *p)) {
							if (*(p + 1) == _T('\0'))
								break;
							Name += *p++;
						}
						Name += *p++;
#endif
					}
				}
				Name += _T('"');
			}

			StringFormat(
				szText,
				TEXT("{},{},{},{},"),
				Name.empty() ? pszName : Name.c_str(),
				pChInfo->GetSpace(),
				pChInfo->GetChannelIndex(),
				pChInfo->GetChannelNo());
			Buffer += szText;
			if (pChInfo->GetServiceType() != 0) {
				StringFormat(szText, TEXT("{}"), pChInfo->GetServiceType());
				Buffer += szText;
			}
			Buffer += _T(',');
			if (pChInfo->GetServiceID() != 0) {
				StringFormat(szText, TEXT("{}"), pChInfo->GetServiceID());
				Buffer += szText;
			}
			Buffer += _T(',');
			if (pChInfo->GetNetworkID() != 0) {
				StringFormat(szText, TEXT("{}"), pChInfo->GetNetworkID());
				Buffer += szText;
			}
			Buffer += _T(',');
			if (pChInfo->GetTransportStreamID() != 0) {
				StringFormat(szText, TEXT("{}"), pChInfo->GetTransportStreamID());
				Buffer += szText;
			}
			Buffer += _T(',');
			Buffer += pChInfo->IsEnabled() ? _T('1') : _T('0');
			Buffer += TEXT("\r\n");
		}
	}

	const HANDLE hFile = ::CreateFile(
		pszFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	DWORD Write;

#ifdef UNICODE
	bool fUnicode = true;

	if (::GetACP() == CP_SHIFT_JIS) {
		BOOL fUsedDefaultChar = FALSE;
		int Length = ::WideCharToMultiByte(
			CP_SHIFT_JIS, 0,
			Buffer.data(), static_cast<int>(Buffer.length()),
			nullptr, 0, nullptr, &fUsedDefaultChar);
		if (Length > 0 && !fUsedDefaultChar) {
			std::string MBCSBuffer(Length, '\0');
			Length = ::WideCharToMultiByte(
				CP_SHIFT_JIS, 0,
				Buffer.data(), static_cast<int>(Buffer.length()),
				MBCSBuffer.data(), Length, nullptr, nullptr);
			if (Length < 1
					|| !::WriteFile(hFile, MBCSBuffer.data(), Length, &Write, nullptr)
					|| Write != static_cast<DWORD>(Length)) {
				::CloseHandle(hFile);
				return false;
			}
			fUnicode = false;
		}
	}

	if (fUnicode) {
		static const WCHAR BOM = 0xFEFF;
		const DWORD Size = static_cast<DWORD>(Buffer.length()) * sizeof(WCHAR);
		if (!::WriteFile(hFile, &BOM, sizeof(BOM), &Write, nullptr)
				|| Write != sizeof(BOM)
				|| !::WriteFile(hFile, Buffer.data(), Size, &Write, nullptr)
				|| Write != Size) {
			::CloseHandle(hFile);
			return false;
		}
	}
#else
	if (!::WriteFile(hFile, Buffer.data(), static_cast<DWORD>(Buffer.length()), &Write, nullptr)
			|| Write != static_cast<DWORD>(Buffer.length())) {
		::CloseHandle(hFile);
		return false;
	}
#endif

	::CloseHandle(hFile);

	return true;
}


static void SkipSpaces(LPCTSTR *ppText)
{
	LPCTSTR p = *ppText;
	p += ::StrSpn(p, TEXT(" \t"));
	*ppText = p;
}

static bool NextToken(LPCTSTR *ppText)
{
	LPCTSTR p = *ppText;

	SkipSpaces(&p);
	if (*p != _T(','))
		return false;
	p++;
	SkipSpaces(&p);
	*ppText = p;
	return true;
}

bool inline IsDigit(TCHAR c)
{
	return c >= _T('0') && c <= _T('9');
}

static int ParseDigits(LPCTSTR *ppText)
{
	LPTSTR pEnd;
	const int Value = std::_tcstol(*ppText, &pEnd, 10);
	*ppText = pEnd;
	return Value;
}

bool CTuningSpaceList::LoadFromFile(LPCTSTR pszFileName)
{
	TRACE(TEXT("CTuningSpaceList::LoadFromFile() : \"{}\"\n"), pszFileName);

	static constexpr LONGLONG MAX_FILE_SIZE = 8LL * 1024 * 1024;

	LARGE_INTEGER FileSize;
	DWORD Read;

	const HANDLE hFile = ::CreateFile(
		pszFileName, GENERIC_READ, FILE_SHARE_READ, nullptr,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;
	if (!::GetFileSizeEx(hFile, &FileSize)
			|| FileSize.QuadPart < 1
			|| FileSize.QuadPart > MAX_FILE_SIZE) {
		::CloseHandle(hFile);
		return false;
	}
	std::unique_ptr<BYTE[]> FileBuffer(new BYTE[FileSize.LowPart + sizeof(TCHAR)]);
	if (!::ReadFile(hFile, FileBuffer.get(), FileSize.LowPart, &Read, nullptr) || Read != FileSize.LowPart) {
		::CloseHandle(hFile);
		return false;
	}
	::CloseHandle(hFile);

	String Buffer;
	LPCTSTR p;
	if (FileSize.LowPart >= 2 && *reinterpret_cast<LPWSTR>(FileBuffer.get()) == 0xFEFF) {
#ifdef UNICODE
		LPWSTR pszBuffer = reinterpret_cast<LPWSTR>(FileBuffer.get()) + 1;
		pszBuffer[FileSize.LowPart / 2 - 1] = L'\0';
		p = pszBuffer;
#else
		const int Length = ::WideCharToMultiByte(
			CP_ACP, 0,
			reinterpret_cast<LPCSTR>(FileBuffer.get()), FileSize.LowPart,
			nullptr, 0, nullptr, nullptr);
		if (Length < 1) {
			return false;
		}
		Buffer.resize(Length);
		::WideCharToMultiByte(
			CP_ACP, 0,
			reinterpret_cast<LPCWTR>(FileBuffer.get()) + 1, FileSize.LowPart / 2 - 1,
			Buffer.data(), Length, nullptr, nullptr);
		p = Buffer.c_str();
#endif
	} else {
#ifdef UNICODE
		int Length = ::MultiByteToWideChar(
			CP_SHIFT_JIS, 0,
			reinterpret_cast<LPCSTR>(FileBuffer.get()), FileSize.LowPart,
			nullptr, 0);
		if (Length < 1) {
			return false;
		}
		Buffer.resize(Length);
		Length = ::MultiByteToWideChar(
			CP_SHIFT_JIS, 0,
			reinterpret_cast<LPCSTR>(FileBuffer.get()), FileSize.LowPart,
			Buffer.data(), Length);
		p = Buffer.c_str();
#else
		LPSTR pszBuffer = reinterpret_cast<LPSTR>(FileBuffer.get());
		pszBuffer[FileSize.LowPart] = '\0';
		p = pszBuffer;
#endif
	}

	m_AllChannelList.Clear();

	do {
		TCHAR szName[MAX_CHANNEL_NAME];

		p += ::StrSpn(p, TEXT("\r\n \t"));

		if (*p == _T('#') || *p == _T(';')) { // コメント
			p++;
			if (*p == _T('#')) {
				p++;
				if (::StrCmpNI(p, TEXT("SPACE("), 6) == 0) {
					// チューニング空間名 #space(インデックス,名前)
					p += 6;
					SkipSpaces(&p);
					if (IsDigit(*p)) {
						const int Space = ParseDigits(&p);
						if (Space >= 0 && Space < 100 && NextToken(&p)) {
							int Length = ::StrCSpn(p, TEXT(")\r\n"));
							if (p[Length] == _T(')') && p[Length + 1] == _T(')'))
								Length++;
							if (Length > 0) {
								StringCopy(szName, p, std::min<size_t>(Length + 1, lengthof(szName)));
								if (static_cast<int>(m_TuningSpaceList.size()) <= Space) {
									Reserve(Space + 1);
									m_TuningSpaceList[Space]->SetName(szName);
								}
								p += Length;
								if (*p == _T('\0'))
									break;
								p++;
							}
						}
					}
				}
			}
			goto Next;
		}
		if (*p == _T('\0'))
			break;

		{
			CChannelInfo ChInfo;

			// チャンネル名
			int NameLength = 0;
			bool fQuote = false;
			if (*p == _T('"')) {
				fQuote = true;
				p++;
			}
			while (*p != _T('\0')) {
				if (fQuote) {
					if (*p == _T('"')) {
						p++;
						if (*p != _T('"')) {
							SkipSpaces(&p);
							break;
						}
					}
				} else {
					if (*p == _T(','))
						break;
				}
				if (NameLength < lengthof(szName) - 1)
					szName[NameLength++] = *p;
				p++;
			}
			szName[NameLength] = _T('\0');
			ChInfo.SetName(szName);
			if (!NextToken(&p))
				goto Next;

			// チューニング空間
			if (!IsDigit(*p))
				goto Next;
			ChInfo.SetSpace(ParseDigits(&p));
			if (!NextToken(&p))
				goto Next;

			// チャンネル
			if (!IsDigit(*p))
				goto Next;
			ChInfo.SetChannelIndex(ParseDigits(&p));

			if (NextToken(&p)) {
				// リモコン番号(オプション)
				ChInfo.SetChannelNo(ParseDigits(&p));
				if (NextToken(&p)) {
					// サービスタイプ(オプション)
					ChInfo.SetServiceType(static_cast<BYTE>(ParseDigits(&p)));
					if (NextToken(&p)) {
						// サービスID(オプション)
						ChInfo.SetServiceID(static_cast<WORD>(ParseDigits(&p)));
						if (NextToken(&p)) {
							// ネットワークID(オプション)
							ChInfo.SetNetworkID(static_cast<WORD>(ParseDigits(&p)));
							if (NextToken(&p)) {
								// トランスポートストリームID(オプション)
								ChInfo.SetTransportStreamID(static_cast<WORD>(ParseDigits(&p)));
								if (NextToken(&p)) {
									// 状態(オプション)
									if (IsDigit(*p)) {
										const int Flags = ParseDigits(&p);
										ChInfo.Enable((Flags & 1) != 0);
									}
								}
							}
						}
					}
				}
			}

			m_AllChannelList.AddChannel(ChInfo);
		}

Next:
		p += ::StrCSpn(p, TEXT("\r\n"));
	} while (*p != _T('\0'));

	return MakeTuningSpaceList(&m_AllChannelList);
}


} // namespace TVTest
