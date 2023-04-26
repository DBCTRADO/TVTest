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
#include "AppMain.h"
#include "ProgramSearch.h"
#include "DialogUtil.h"
#include "EpgUtil.h"
#include "EventInfoUtil.h"
#include "GUIUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


void CEventSearchServiceList::Clear()
{
	m_ServiceList.clear();
}


bool CEventSearchServiceList::IsEmpty() const
{
	return m_ServiceList.empty();
}


size_t CEventSearchServiceList::GetServiceCount() const
{
	return m_ServiceList.size();
}


void CEventSearchServiceList::Add(ServiceKey Key)
{
	m_ServiceList.insert(Key);
}


void CEventSearchServiceList::Add(WORD NetworkID, WORD TSID, WORD ServiceID)
{
	m_ServiceList.insert(GetServiceKey(NetworkID, TSID, ServiceID));
}


bool CEventSearchServiceList::IsExists(ServiceKey Key) const
{
	return m_ServiceList.find(Key) != m_ServiceList.end();
}


bool CEventSearchServiceList::IsExists(WORD NetworkID, WORD TSID, WORD ServiceID) const
{
	return m_ServiceList.find(GetServiceKey(NetworkID, TSID, ServiceID)) != m_ServiceList.end();
}


void CEventSearchServiceList::Combine(const CEventSearchServiceList &List)
{
	for (const auto &e : List.m_ServiceList) {
		m_ServiceList.insert(e);
	}
}


CEventSearchServiceList::Iterator CEventSearchServiceList::Begin() const
{
	return m_ServiceList.begin();
}


CEventSearchServiceList::Iterator CEventSearchServiceList::End() const
{
	return m_ServiceList.end();
}


bool CEventSearchServiceList::ToString(String *pString) const
{
	if (pString == nullptr)
		return false;

	pString->clear();

	ServiceKey PrevKey = 0;

	for (const ServiceKey e : m_ServiceList) {
		ServiceKey Key = e;
		TCHAR szKey[16];
		if (PrevKey != 0 && PrevKey >> 16 == Key >> 16)
			Key &= 0xFFFFULL;
		else if (ServiceKey_GetNetworkID(Key) == ServiceKey_GetTransportStreamID(Key))
			Key &= 0xFFFFFFFFULL;
		const int Length = EncodeServiceKey(Key, szKey);
		szKey[Length] = _T(':');
		szKey[Length + 1] = _T('\0');
		*pString += szKey;
		PrevKey = e;
	}

	return true;
}


bool CEventSearchServiceList::FromString(LPCTSTR pszString)
{
	if (pszString == nullptr)
		return false;

	m_ServiceList.clear();

	LPCTSTR p = pszString;
	ServiceKey PrevKey = 0;
	while (*p != _T('\0')) {
		size_t Length = 0;
		for (Length = 0; p[Length] != _T(':') && p[Length] != _T('\0'); Length++);
		if (Length > 0) {
			ServiceKey Key;
			if (!DecodeServiceKey(p, Length, &Key))
				break;
			if (Key <= 0xFFFFULL)
				Key |= PrevKey & 0xFFFFFFFF0000ULL;
			else if (ServiceKey_GetNetworkID(Key) == 0)
				Key |= static_cast<ULONGLONG>(ServiceKey_GetTransportStreamID(Key)) << 32;
			m_ServiceList.insert(Key);
			PrevKey = Key;
			p += Length;
		}
		if (*p == _T(':'))
			p++;
	}

	return true;
}


int CEventSearchServiceList::EncodeServiceKey(ServiceKey Key, LPTSTR pText)
{
	static const char EncodeChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	int Length = 0;
	for (int i = 0; i < 8; i++) {
		const unsigned int Char = static_cast<unsigned int>((Key >> (42 - i * 6)) & 0x3F);
		if (Char != 0 || Length > 0) {
			pText[Length++] = EncodeChars[Char];
		}
	}

	return Length;
}


bool CEventSearchServiceList::DecodeServiceKey(LPCTSTR pText, size_t Length, ServiceKey *pKey)
{
	ServiceKey Key = 0;

	for (size_t i = 0; i < Length; i++) {
		const TCHAR c = pText[i];
		unsigned int Value;

		if (c >= _T('A') && c <= _T('Z')) {
			Value = c - _T('A');
		} else if (c >= _T('a') && c <= _T('z')) {
			Value = c - _T('a') + 26;
		} else if (c >= _T('0') && c <= _T('9')) {
			Value = c - _T('0') + 52;
		} else if (c == _T('+')) {
			Value = 62;
		} else if (c == _T('/')) {
			Value = 63;
		} else {
			return false;
		}
		Key = (Key << 6) | Value;
	}

	*pKey = Key;

	return true;
}




void CEventSearchSettings::Clear()
{
	*this = CEventSearchSettings();
}


bool CEventSearchSettings::ToString(String *pString) const
{
	String Buffer;

	pString->clear();

	StringUtility::Encode(Name.c_str(), &Buffer);
	*pString += Buffer;
	*pString += TEXT(",");
	StringUtility::Encode(Keyword.c_str(), &Buffer);
	*pString += Buffer;

	ConditionFlag Flags = ConditionFlag::None;
	if (fDisabled)
		Flags |= ConditionFlag::Disabled;
	if (fRegExp)
		Flags |= ConditionFlag::RegExp;
	if (fIgnoreCase)
		Flags |= ConditionFlag::IgnoreCase;
	if (fIgnoreWidth)
		Flags |= ConditionFlag::IgnoreWidth;
	if (fEventName)
		Flags |= ConditionFlag::EventName;
	if (fEventText)
		Flags |= ConditionFlag::EventText;
	if (fGenre)
		Flags |= ConditionFlag::Genre;
	if (fDayOfWeek)
		Flags |= ConditionFlag::DayOfWeek;
	if (fTime)
		Flags |= ConditionFlag::Time;
	if (fDuration)
		Flags |= ConditionFlag::Duration;
	if (fCA)
		Flags |= ConditionFlag::CA;
	if (fVideo)
		Flags |= ConditionFlag::Video;
	if (fServiceList)
		Flags |= ConditionFlag::ServiceList;

	bool fGenre2 = false;
	for (int i = 0; i < 16; i++) {
		if (Genre2[i] != 0) {
			fGenre2 = true;
			break;
		}
	}
	TCHAR szGenre2[16 * 4 + 1];
	if (fGenre2) {
		for (int i = 0; i < 16; i++) {
			StringFormat(&szGenre2[i * 4], 5, TEXT("{:04x}"), Genre2[i]);
		}
	} else {
		szGenre2[0] = _T('\0');
	}

	StringFormat(
		&Buffer,
		TEXT(",{},{},{},{},{}:{:02},{}:{:02},{},{},{},{}"),
		static_cast<std::underlying_type_t<ConditionFlag>>(Flags),
		Genre1,
		szGenre2,
		DayOfWeekFlags,
		StartTime.Hour, StartTime.Minute,
		EndTime.Hour, EndTime.Minute,
		DurationShortest,
		DurationLongest,
		static_cast<int>(CA),
		static_cast<int>(Video));
	*pString += Buffer;

	if (!ServiceList.IsEmpty()) {
		*pString += TEXT(",");
		ServiceList.ToString(&Buffer);
		*pString += Buffer;
	}

	return true;
}


bool CEventSearchSettings::FromString(LPCTSTR pszString)
{
	std::vector<String> Value;

	StringUtility::Split(String(pszString), L",", &Value);

	for (size_t i = 0; i < Value.size(); i++) {
		switch (i) {
		case 0:
			StringUtility::Decode(Value[i].c_str(), &Name);
			break;

		case 1:
			StringUtility::Decode(Value[i].c_str(), &Keyword);
			break;

		case 2:
			{
				const ConditionFlag Flags = static_cast<ConditionFlag>(std::wcstoul(Value[i].c_str(), nullptr, 0));
				fDisabled = !!(Flags & ConditionFlag::Disabled);
				fRegExp = !!(Flags & ConditionFlag::RegExp);
				fIgnoreCase = !!(Flags & ConditionFlag::IgnoreCase);
				fIgnoreWidth = !!(Flags & ConditionFlag::IgnoreWidth);
				fEventName = !!(Flags & ConditionFlag::EventName);
				fEventText = !!(Flags & ConditionFlag::EventText);
				fGenre = !!(Flags & ConditionFlag::Genre);
				fDayOfWeek = !!(Flags & ConditionFlag::DayOfWeek);
				fTime = !!(Flags & ConditionFlag::Time);
				fDuration = !!(Flags & ConditionFlag::Duration);
				fCA = !!(Flags & ConditionFlag::CA);
				fVideo = !!(Flags & ConditionFlag::Video);
				fServiceList = !!(Flags & ConditionFlag::ServiceList);
			}
			break;

		case 3:
			Genre1 = static_cast<WORD>(std::wcstoul(Value[i].c_str(), nullptr, 0));
			break;

		case 4:
			if (Value[i].length() >= 16 * 4) {
				auto it = Value[i].begin();
				for (int j = 0; j < 16; j++) {
					TCHAR Hex[4];
					Hex[0] = *it++;
					Hex[1] = *it++;
					Hex[2] = *it++;
					Hex[3] = *it++;
					Genre2[j] = static_cast<WORD>(HexStringToUInt(Hex, 4));
				}
			} else {
				::ZeroMemory(Genre2, sizeof(Genre2));
			}
			break;

		case 5:
			DayOfWeekFlags = std::wcstoul(Value[i].c_str(), nullptr, 0);
			break;

		case 6:
			ParseTime(Value[i].c_str(), &StartTime);
			break;

		case 7:
			ParseTime(Value[i].c_str(), &EndTime);
			break;

		case 8:
			DurationShortest = std::wcstoul(Value[i].c_str(), nullptr, 0);
			break;

		case 9:
			DurationLongest = std::wcstoul(Value[i].c_str(), nullptr, 0);
			break;

		case 10:
			CA = static_cast<CAType>(std::wcstoul(Value[i].c_str(), nullptr, 0));
			break;

		case 11:
			Video = static_cast<VideoType>(std::wcstoul(Value[i].c_str(), nullptr, 0));
			break;

		case 12:
			ServiceList.FromString(Value[i].c_str());
			break;
		}
	}

	return true;
}


void CEventSearchSettings::ParseTime(LPCWSTR pszString, TimeInfo *pTime)
{
	wchar_t *p;

	pTime->Hour = std::wcstol(pszString, &p, 10);
	if (*p == L':')
		pTime->Minute = std::wcstol(p + 1, nullptr, 10);
	else
		pTime->Minute = 0;
}




CEventSearchSettingsList::CEventSearchSettingsList(const CEventSearchSettingsList &Src)
{
	*this = Src;
}


CEventSearchSettingsList &CEventSearchSettingsList::operator=(const CEventSearchSettingsList &Src)
{
	if (&Src != this) {
		Clear();

		if (!Src.m_List.empty()) {
			m_List.reserve(Src.m_List.size());

			for (const auto &e : Src.m_List) {
				m_List.emplace_back(std::make_unique<CEventSearchSettings>(*e));
			}
		}
	}

	return *this;
}


void CEventSearchSettingsList::Clear()
{
	m_List.clear();
}


size_t CEventSearchSettingsList::GetCount() const
{
	return m_List.size();
}


size_t CEventSearchSettingsList::GetEnabledCount() const
{
	size_t Count = 0;
	for (const auto &e : m_List) {
		if (!e->fDisabled)
			Count++;
	}
	return Count;
}


CEventSearchSettings *CEventSearchSettingsList::Get(size_t Index)
{
	if (Index >= m_List.size())
		return nullptr;
	return m_List[Index].get();
}


const CEventSearchSettings *CEventSearchSettingsList::Get(size_t Index) const
{
	if (Index >= m_List.size())
		return nullptr;
	return m_List[Index].get();
}


CEventSearchSettings *CEventSearchSettingsList::GetByName(LPCTSTR pszName)
{
	const int Index = FindByName(pszName);
	if (Index < 0)
		return nullptr;
	return m_List[Index].get();
}


const CEventSearchSettings *CEventSearchSettingsList::GetByName(LPCTSTR pszName) const
{
	const int Index = FindByName(pszName);
	if (Index < 0)
		return nullptr;
	return m_List[Index].get();
}


bool CEventSearchSettingsList::Add(const CEventSearchSettings &Settings)
{
	m_List.emplace_back(std::make_unique<CEventSearchSettings>(Settings));
	return true;
}


bool CEventSearchSettingsList::Erase(size_t Index)
{
	if (Index >= m_List.size())
		return false;

	auto it = m_List.begin();
	std::advance(it, Index);
	m_List.erase(it);

	return true;
}


int CEventSearchSettingsList::FindByName(LPCTSTR pszName) const
{
	if (pszName == nullptr)
		return -1;

	for (size_t i = 0; i < m_List.size(); i++) {
		if (::lstrcmpi(m_List[i]->Name.c_str(), pszName) == 0)
			return static_cast<int>(i);
	}

	return -1;
}


bool CEventSearchSettingsList::Load(CSettings &Settings, LPCTSTR pszPrefix)
{
	Clear();

	TCHAR szKey[256];
	String Value;

	for (int i = 0;; i++) {
		StringFormat(szKey, TEXT("{}{}"), pszPrefix, i);
		if (!Settings.Read(szKey, &Value))
			break;
		CEventSearchSettings SearchSettings;
		if (SearchSettings.FromString(Value.c_str())) {
			m_List.emplace_back(std::make_unique<CEventSearchSettings>(SearchSettings));
		}
	}

	return true;
}


bool CEventSearchSettingsList::Save(CSettings &Settings, LPCTSTR pszPrefix) const
{
	TCHAR szKey[256];
	String Value;

	for (size_t i = 0; i < m_List.size(); i++) {
		StringFormat(szKey, TEXT("{}{}"), pszPrefix, i);
		m_List[i]->ToString(&Value);
		Settings.Write(szKey, Value);
	}

	return true;
}




bool CEventSearcher::InitializeRegExp()
{
	return m_RegExp.Initialize();
}


void CEventSearcher::Finalize()
{
	m_RegExp.Finalize();
}


bool CEventSearcher::BeginSearch(const CEventSearchSettings &Settings)
{
	m_Settings = Settings;

	if (Settings.fRegExp && !Settings.Keyword.empty()) {
		if (!m_RegExp.Initialize())
			return false;
		CRegExp::PatternFlag Flags = CRegExp::PatternFlag::Optimize;
		if (Settings.fIgnoreCase)
			Flags |= CRegExp::PatternFlag::IgnoreCase;
		if (Settings.fIgnoreWidth)
			Flags |= CRegExp::PatternFlag::IgnoreWidth;
		if (!m_RegExp.SetPattern(Settings.Keyword.c_str(), Flags))
			return false;
	}

	return true;
}


bool CEventSearcher::Match(const LibISDB::EventInfo *pEventInfo)
{
	if (m_Settings.fServiceList) {
		if (!m_Settings.ServiceList.IsExists(
					pEventInfo->NetworkID,
					pEventInfo->TransportStreamID,
					pEventInfo->ServiceID))
			return false;
	}

	if (m_Settings.fGenre) {
		bool fMatch = false;
		for (int i = 0; i < pEventInfo->ContentNibble.NibbleCount; i++) {
			const int Level1 = pEventInfo->ContentNibble.NibbleList[i].ContentNibbleLevel1;
			if (Level1 != 0xE) {
				if (Level1 > 15)
					return false;
				if ((m_Settings.Genre1 & (1 << Level1)) != 0
						&& m_Settings.Genre2[Level1] == 0) {
					fMatch = true;
				} else {
					const int Level2 = pEventInfo->ContentNibble.NibbleList[i].ContentNibbleLevel2;
					if ((m_Settings.Genre2[Level1] & (1 << Level2)) != 0)
						fMatch = true;
				}
				break;
			}
		}
		if (!fMatch)
			return false;
	}

	if (m_Settings.fDayOfWeek) {
		if ((m_Settings.DayOfWeekFlags & (1 << pEventInfo->StartTime.DayOfWeek)) == 0)
			return false;
	}

	if (m_Settings.fTime) {
		const int RangeStart = (m_Settings.StartTime.Hour * 60 + m_Settings.StartTime.Minute) % (24 * 60);
		const int RangeEnd = (m_Settings.EndTime.Hour * 60 + m_Settings.EndTime.Minute) % (24 * 60);
		const int EventStart = pEventInfo->StartTime.Hour * 60 + pEventInfo->StartTime.Minute;
		const int EventEnd = EventStart + pEventInfo->Duration / 60;

		if (RangeStart <= RangeEnd) {
			if (EventEnd <= RangeStart || EventStart > RangeEnd)
				return false;
		} else {
			if (EventEnd <= RangeStart && EventStart > RangeEnd)
				return false;
		}
	}

	if (m_Settings.fDuration) {
		if (pEventInfo->Duration < m_Settings.DurationShortest)
			return false;
		if (m_Settings.DurationLongest > 0
				&& pEventInfo->Duration > m_Settings.DurationLongest)
			return false;
	}

	if (m_Settings.fCA) {
		switch (m_Settings.CA) {
		case CEventSearchSettings::CAType::Free:
			if (pEventInfo->FreeCAMode)
				return false;
			break;
		case CEventSearchSettings::CAType::Chargeable:
			if (!pEventInfo->FreeCAMode)
				return false;
			break;
		}
	}

	if (m_Settings.fVideo
			&& !pEventInfo->VideoList.empty()) {
		switch (m_Settings.Video) {
		case CEventSearchSettings::VideoType::HD:
			if (EpgUtil::GetVideoType(pEventInfo->VideoList[0].ComponentType) != EpgUtil::VideoType::HD)
				return false;
			break;
		case CEventSearchSettings::VideoType::SD:
			if (EpgUtil::GetVideoType(pEventInfo->VideoList[0].ComponentType) != EpgUtil::VideoType::SD)
				return false;
			break;
		}
	}

	if (m_Settings.Keyword.empty())
		return true;

	if (m_Settings.fRegExp)
		return MatchRegExp(pEventInfo);

	return MatchKeyword(pEventInfo, m_Settings.Keyword.c_str());
}


int CEventSearcher::FindKeyword(LPCTSTR pszText, LPCTSTR pKeyword, int KeywordLength, int *pFoundLength) const
{
	if (IsStringEmpty(pszText))
		return -1;

	UINT Flags = 0;
	if (m_Settings.fIgnoreCase)
		Flags |= NORM_IGNORECASE;
	if (m_Settings.fIgnoreWidth)
		Flags |= NORM_IGNOREWIDTH;

	return ::FindNLSString(
		LOCALE_USER_DEFAULT, FIND_FROMSTART | Flags,
		pszText, -1, pKeyword, KeywordLength, pFoundLength);
}


bool CEventSearcher::FindExtendedText(
	const LibISDB::EventInfo::ExtendedTextInfoList &ExtendedText, LPCTSTR pKeyword, int KeywordLength) const
{
	for (auto &e : ExtendedText) {
		if (FindKeyword(e.Description.c_str(), pKeyword, KeywordLength) >= 0)
			return true;
		if (FindKeyword(e.Text.c_str(), pKeyword, KeywordLength) >= 0)
			return true;
	}
	return false;
}


bool CEventSearcher::MatchKeyword(const LibISDB::EventInfo *pEventInfo, LPCTSTR pszKeyword) const
{
	bool fMatch = false, fMinusOnly = true;
	bool fOr = false, fPrevOr = false, fOrMatch;
	int WordCount = 0;
	LPCTSTR p = pszKeyword;

	while (*p != '\0') {
		TCHAR szWord[CEventSearchSettings::MAX_KEYWORD_LENGTH], Delimiter;
		bool fMinus = false;

		while (*p == ' ')
			p++;
		if (*p == '-') {
			fMinus = true;
			p++;
		}
		if (*p == '"') {
			p++;
			Delimiter = '"';
		} else {
			Delimiter = ' ';
		}
		int i;
		for (i = 0; *p != Delimiter && *p != '|' && *p != '\0'; i++) {
			szWord[i] = *p++;
		}
		if (*p == Delimiter)
			p++;
		while (*p == ' ')
			p++;
		if (*p == '|') {
			if (!fOr) {
				fOr = true;
				fOrMatch = false;
			}
			p++;
		} else {
			fOr = false;
		}
		if (i > 0) {
			if ((m_Settings.fEventName
					&& FindKeyword(pEventInfo->EventName.c_str(), szWord, i) >= 0)
					|| (m_Settings.fEventText
						&& FindKeyword(pEventInfo->EventText.c_str(), szWord, i) >= 0)
					|| (m_Settings.fEventText
						&& FindExtendedText(pEventInfo->ExtendedText, szWord, i))) {
				if (fMinus)
					return false;
				fMatch = true;
				if (fOr)
					fOrMatch = true;
			} else {
				if (!fMinus && !fOr && (!fPrevOr || !fOrMatch))
					return false;
			}
			if (!fMinus)
				fMinusOnly = false;
			WordCount++;
		}
		fPrevOr = fOr;
	}
	if (fMinusOnly && WordCount > 0)
		return true;
	return fMatch;
}


bool CEventSearcher::MatchRegExp(const LibISDB::EventInfo *pEventInfo)
{
	if (m_Settings.fEventName
			&& !pEventInfo->EventName.empty()
			&& m_RegExp.Match(pEventInfo->EventName.c_str()))
		return true;

	if (m_Settings.fEventText
			&& !pEventInfo->EventText.empty()
			&& m_RegExp.Match(pEventInfo->EventText.c_str()))
		return true;

	if (m_Settings.fEventText
			&& !pEventInfo->ExtendedText.empty()) {
		for (auto &e : pEventInfo->ExtendedText) {
			if (!e.Description.empty()
					&& m_RegExp.Match(e.Description.c_str()))
				return true;
			if (!e.Text.empty()
					&& m_RegExp.Match(e.Text.c_str()))
				return true;
		}
	}

	return false;
}




bool CEventSearchOptions::SetKeywordHistory(const LPTSTR *pKeywordList, int NumKeywords)
{
	if (pKeywordList == nullptr)
		return false;
	m_KeywordHistory.clear();
	for (int i = 0; i < NumKeywords; i++)
		m_KeywordHistory.emplace_back(pKeywordList[i]);
	return true;
}


bool CEventSearchOptions::SetKeywordHistory(const String *pKeywordList, size_t NumKeywords)
{
	if (pKeywordList == nullptr)
		return false;
	m_KeywordHistory.clear();
	for (size_t i = 0; i < NumKeywords; i++)
		m_KeywordHistory.emplace_back(pKeywordList[i]);
	return true;
}


int CEventSearchOptions::GetKeywordHistoryCount() const
{
	return static_cast<int>(m_KeywordHistory.size());
}


LPCTSTR CEventSearchOptions::GetKeywordHistory(int Index) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_KeywordHistory.size())
		return nullptr;
	return m_KeywordHistory[Index].c_str();
}


bool CEventSearchOptions::AddKeywordHistory(LPCTSTR pszKeyword)
{
	if (IsStringEmpty(pszKeyword))
		return false;

	for (auto it = m_KeywordHistory.begin(); it != m_KeywordHistory.end(); ++it) {
		if (StringUtility::CompareNoCase(*it, pszKeyword) == 0) {
			if (it == m_KeywordHistory.begin()
					&& it->compare(pszKeyword) == 0)
				return true;
			m_KeywordHistory.erase(it);
			break;
		}
	}

	m_KeywordHistory.emplace_front(pszKeyword);

	if (m_KeywordHistory.size() > static_cast<size_t>(m_MaxKeywordHistory)) {
		m_KeywordHistory.erase(m_KeywordHistory.begin() + m_MaxKeywordHistory, m_KeywordHistory.end());
	}

	return true;
}


bool CEventSearchOptions::DeleteKeywordHistory(int Index)
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_KeywordHistory.size())
		return false;

	m_KeywordHistory.erase(m_KeywordHistory.begin() + Index);

	return true;
}


void CEventSearchOptions::ClearKeywordHistory()
{
	m_KeywordHistory.clear();
}


bool CEventSearchOptions::SetMaxKeywordHistory(int Max)
{
	if (Max < 0)
		return false;

	m_MaxKeywordHistory = Max;

	if (m_KeywordHistory.size() > static_cast<size_t>(Max)) {
		m_KeywordHistory.erase(m_KeywordHistory.begin() + Max, m_KeywordHistory.end());
	}

	return true;
}


size_t CEventSearchOptions::GetSearchSettingsCount() const
{
	return m_SettingsList.GetCount();
}


CEventSearchSettings *CEventSearchOptions::GetSearchSettings(size_t Index)
{
	return m_SettingsList.Get(Index);
}


const CEventSearchSettings *CEventSearchOptions::GetSearchSettings(size_t Index) const
{
	return m_SettingsList.Get(Index);
}


CEventSearchSettings *CEventSearchOptions::GetSearchSettingsByName(LPCTSTR pszName)
{
	return m_SettingsList.GetByName(pszName);
}


const CEventSearchSettings *CEventSearchOptions::GetSearchSettingsByName(LPCTSTR pszName) const
{
	return m_SettingsList.GetByName(pszName);
}


void CEventSearchOptions::ClearSearchSettings()
{
	m_SettingsList.Clear();
}


bool CEventSearchOptions::AddSearchSettings(const CEventSearchSettings &Settings)
{
	return m_SettingsList.Add(Settings);
}


bool CEventSearchOptions::DeleteSearchSettings(size_t Index)
{
	return m_SettingsList.Erase(Index);
}


int CEventSearchOptions::FindSearchSettings(LPCTSTR pszName) const
{
	return m_SettingsList.FindByName(pszName);
}


bool CEventSearchOptions::LoadSearchSettings(CSettings &Settings, LPCTSTR pszPrefix)
{
	return m_SettingsList.Load(Settings, pszPrefix);
}


bool CEventSearchOptions::SaveSearchSettings(CSettings &Settings, LPCTSTR pszPrefix) const
{
	return m_SettingsList.Save(Settings, pszPrefix);
}




namespace
{

constexpr UINT WM_PROGRAM_SEARCH_GENRE_CHANGED = WM_APP;

constexpr LPARAM GENRE_LPARAM_PACK(int Level1, int Level2) { return (Level1 << 16) | static_cast<WORD>(static_cast<SHORT>(Level2)); }
constexpr int GENRE_LPARAM_LEVEL1(LPARAM lParam) { return static_cast<int>(lParam >> 16); }
constexpr int GENRE_LPARAM_LEVEL2(LPARAM lParam) { return static_cast<SHORT>(static_cast<WORD>(lParam & 0xFFFF)); }


static ULONGLONG GetResultMapKey(const LibISDB::EventInfo *pEventInfo)
{
	return (static_cast<ULONGLONG>(pEventInfo->NetworkID) << 48)
		| (static_cast<ULONGLONG>(pEventInfo->TransportStreamID) << 32)
		| (static_cast<DWORD>(pEventInfo->ServiceID) << 16)
		| pEventInfo->EventID;
}

}




CEventSearchSettingsDialog::CEventSearchSettingsDialog(CEventSearchOptions &Options)
	: m_Options(Options)
{
}


bool CEventSearchSettingsDialog::Create(HWND hwndOwner)
{
	return CreateDialogWindow(
		hwndOwner,
		GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE(IDD_EVENTSEARCH));
}


bool CEventSearchSettingsDialog::GetSettings(CEventSearchSettings *pSettings) const
{
	if (m_hDlg != nullptr) {
		TCHAR szKeyword[CEventSearchSettings::MAX_KEYWORD_LENGTH];
		::GetDlgItemText(m_hDlg, IDC_EVENTSEARCH_KEYWORD, szKeyword, lengthof(szKeyword));
		RemoveTrailingWhitespace(szKeyword);
		pSettings->Keyword = szKeyword;
		pSettings->fIgnoreCase =
			!DlgCheckBox_IsChecked(m_hDlg, IDC_EVENTSEARCH_CASESENSITIVE);
		pSettings->fRegExp =
			DlgCheckBox_IsChecked(m_hDlg, IDC_EVENTSEARCH_REGEXP);

		switch (DlgComboBox_GetCurSel(m_hDlg, IDC_EVENTSEARCH_KEYWORDTARGET)) {
		case KEYWORDTARGET_EVENTNAME_AND_EVENTTEXT:
			pSettings->fEventName = true;
			pSettings->fEventText = true;
			break;
		case KEYWORDTARGET_EVENTNAME:
			pSettings->fEventName = true;
			pSettings->fEventText = false;
			break;
		case KEYWORDTARGET_EVENTTEXT:
			pSettings->fEventName = false;
			pSettings->fEventText = true;
			break;
		}

		GetGenreSettings(pSettings);

		pSettings->fDayOfWeek =
			DlgCheckBox_IsChecked(m_hDlg, IDC_EVENTSEARCH_DAYOFWEEK);
		unsigned int DayOfWeekFlags = 0;
		for (int i = 0; i < 7; i++) {
			if (DlgCheckBox_IsChecked(m_hDlg, IDC_EVENTSEARCH_DAYOFWEEK_SUNDAY + i))
				DayOfWeekFlags |= 1 << i;
		}
		pSettings->DayOfWeekFlags = DayOfWeekFlags;

		pSettings->fTime =
			DlgCheckBox_IsChecked(m_hDlg, IDC_EVENTSEARCH_TIME);
		pSettings->StartTime.Hour =
			DlgEdit_GetInt(m_hDlg, IDC_EVENTSEARCH_TIME_START_HOUR);
		pSettings->StartTime.Minute =
			DlgEdit_GetInt(m_hDlg, IDC_EVENTSEARCH_TIME_START_MINUTE);
		pSettings->EndTime.Hour =
			DlgEdit_GetInt(m_hDlg, IDC_EVENTSEARCH_TIME_END_HOUR);
		pSettings->EndTime.Minute =
			DlgEdit_GetInt(m_hDlg, IDC_EVENTSEARCH_TIME_END_MINUTE);

		pSettings->fDuration =
			DlgCheckBox_IsChecked(m_hDlg, IDC_EVENTSEARCH_DURATION);
		pSettings->DurationShortest =
			DlgEdit_GetInt(m_hDlg, IDC_EVENTSEARCH_DURATION_SHORT_INPUT) * 60;

		pSettings->fCA =
			DlgCheckBox_IsChecked(m_hDlg, IDC_EVENTSEARCH_CA);
		pSettings->CA =
			static_cast<CEventSearchSettings::CAType>(DlgComboBox_GetCurSel(m_hDlg, IDC_EVENTSEARCH_CA_LIST));

		pSettings->fVideo =
			DlgCheckBox_IsChecked(m_hDlg, IDC_EVENTSEARCH_VIDEO);
		pSettings->Video =
			static_cast<CEventSearchSettings::VideoType>(DlgComboBox_GetCurSel(m_hDlg, IDC_EVENTSEARCH_VIDEO_LIST));
	} else {
		*pSettings = m_SearchSettings;
	}

	return true;
}


void CEventSearchSettingsDialog::SetSettings(const CEventSearchSettings &Settings)
{
	if (m_hDlg != nullptr) {
		// キーワード
		::SetDlgItemText(m_hDlg, IDC_EVENTSEARCH_KEYWORD, Settings.Keyword.c_str());
		DlgCheckBox_Check(m_hDlg, IDC_EVENTSEARCH_CASESENSITIVE, !Settings.fIgnoreCase);
		DlgCheckBox_Check(m_hDlg, IDC_EVENTSEARCH_REGEXP, Settings.fRegExp);

		// キーワード検索対象
		int KeywordTarget = KEYWORDTARGET_EVENTNAME_AND_EVENTTEXT;
		if (Settings.fEventName != Settings.fEventText) {
			if (Settings.fEventName)
				KeywordTarget = KEYWORDTARGET_EVENTNAME;
			else
				KeywordTarget = KEYWORDTARGET_EVENTTEXT;
		}
		DlgComboBox_SetCurSel(m_hDlg, IDC_EVENTSEARCH_KEYWORDTARGET, KeywordTarget);

		// ジャンル
		const HWND hwndGenre = ::GetDlgItem(m_hDlg, IDC_EVENTSEARCH_GENRE);
		HTREEITEM hItem = TreeView_GetChild(hwndGenre, TVI_ROOT);
		TVITEM tvi;
		tvi.mask = TVIF_PARAM;
		while (hItem != nullptr) {
			tvi.hItem = hItem;
			TreeView_GetItem(hwndGenre, &tvi);
			const int Level1 = GENRE_LPARAM_LEVEL1(tvi.lParam);
			TreeView_SetCheckState(hwndGenre, hItem, (Settings.Genre1 & (1 << Level1)) != 0);
			HTREEITEM hChild = TreeView_GetChild(hwndGenre, hItem);
			while (hChild != nullptr) {
				tvi.hItem = hChild;
				TreeView_GetItem(hwndGenre, &tvi);
				const int Level2 = GENRE_LPARAM_LEVEL2(tvi.lParam);
				TreeView_SetCheckState(hwndGenre, hChild, (Settings.Genre2[Level1] & (1 << Level2)) != 0);
				hChild = TreeView_GetNextSibling(hwndGenre, hChild);
			}
			hItem = TreeView_GetNextSibling(hwndGenre, hItem);
		}
		SetGenreStatus();

		// 曜日
		DlgCheckBox_Check(m_hDlg, IDC_EVENTSEARCH_DAYOFWEEK, Settings.fDayOfWeek);
		for (int i = 0; i < 7; i++) {
			::CheckDlgButton(
				m_hDlg, IDC_EVENTSEARCH_DAYOFWEEK_SUNDAY + i,
				(Settings.DayOfWeekFlags & (1 << i)) != 0);
		}
		EnableDlgItems(
			m_hDlg,
			IDC_EVENTSEARCH_DAYOFWEEK_SUNDAY,
			IDC_EVENTSEARCH_DAYOFWEEK_SATURDAY,
			Settings.fDayOfWeek);

		// 時間
		TCHAR szText[16];
		DlgCheckBox_Check(m_hDlg, IDC_EVENTSEARCH_TIME, Settings.fTime);
		DlgEdit_SetInt(m_hDlg, IDC_EVENTSEARCH_TIME_START_HOUR, Settings.StartTime.Hour);
		StringFormat(szText, TEXT("{:02}"), Settings.StartTime.Minute);
		DlgEdit_SetText(m_hDlg, IDC_EVENTSEARCH_TIME_START_MINUTE, szText);
		DlgEdit_SetInt(m_hDlg, IDC_EVENTSEARCH_TIME_END_HOUR, Settings.EndTime.Hour);
		StringFormat(szText, TEXT("{:02}"), Settings.EndTime.Minute);
		DlgEdit_SetText(m_hDlg, IDC_EVENTSEARCH_TIME_END_MINUTE, szText);
		EnableDlgItems(
			m_hDlg, IDC_EVENTSEARCH_TIME_START_HOUR, IDC_EVENTSEARCH_TIME_END_MINUTE,
			Settings.fTime);

		// 長さ
		DlgCheckBox_Check(m_hDlg, IDC_EVENTSEARCH_DURATION, Settings.fDuration);
		DlgEdit_SetInt(m_hDlg, IDC_EVENTSEARCH_DURATION_SHORT_INPUT, Settings.DurationShortest / 60);
		EnableDlgItems(
			m_hDlg,
			IDC_EVENTSEARCH_DURATION_SHORT_INPUT,
			IDC_EVENTSEARCH_DURATION_SHORT_UNIT,
			Settings.fDuration);

		// CA
		DlgCheckBox_Check(m_hDlg, IDC_EVENTSEARCH_CA, Settings.fCA);
		DlgComboBox_SetCurSel(m_hDlg, IDC_EVENTSEARCH_CA_LIST, static_cast<int>(Settings.CA));
		EnableDlgItem(m_hDlg, IDC_EVENTSEARCH_CA_LIST, Settings.fCA);

		// 映像
		DlgCheckBox_Check(m_hDlg, IDC_EVENTSEARCH_VIDEO, Settings.fVideo);
		DlgComboBox_SetCurSel(m_hDlg, IDC_EVENTSEARCH_VIDEO_LIST, static_cast<int>(Settings.Video));
		EnableDlgItem(m_hDlg, IDC_EVENTSEARCH_VIDEO_LIST, Settings.fVideo);
	} else {
		m_SearchSettings = Settings;
	}
}


void CEventSearchSettingsDialog::SetEventHandler(CEventHandler *pEventHandler)
{
	m_pEventHandler = pEventHandler;
}


bool CEventSearchSettingsDialog::BeginSearch()
{
	if (m_hDlg == nullptr)
		return false;
	::SendMessage(m_hDlg, WM_COMMAND, IDC_EVENTSEARCH_SEARCH, 0);
	return true;
}


bool CEventSearchSettingsDialog::SetKeyword(LPCTSTR pszKeyword)
{
	if (m_hDlg == nullptr)
		return false;

	const HWND hwndComboBox = ::GetDlgItem(m_hDlg, IDC_EVENTSEARCH_KEYWORD);
	if (IsStringEmpty(pszKeyword)) {
		ComboBox_SetText(hwndComboBox, TEXT(""));
	} else {
		TCHAR szCurKeyword[CEventSearchSettings::MAX_KEYWORD_LENGTH];
		const int Length = ComboBox_GetText(hwndComboBox, szCurKeyword, lengthof(szCurKeyword));
		if (Length < 1 || ::lstrcmp(szCurKeyword, pszKeyword) != 0)
			ComboBox_SetText(hwndComboBox, pszKeyword);
	}
	::UpdateWindow(hwndComboBox);

	return true;
}


bool CEventSearchSettingsDialog::AddToKeywordHistory(LPCTSTR pszKeyword)
{
	if (m_hDlg == nullptr || IsStringEmpty(pszKeyword))
		return false;

	const HWND hwndComboBox = ::GetDlgItem(m_hDlg, IDC_EVENTSEARCH_KEYWORD);
	const int i = ComboBox_FindStringExact(hwndComboBox, -1, pszKeyword);
	if (i == CB_ERR) {
		ComboBox_InsertString(hwndComboBox, 0, pszKeyword);
		const int MaxHistory = m_Options.GetMaxKeywordHistory();
		if (ComboBox_GetCount(hwndComboBox) > MaxHistory)
			ComboBox_DeleteString(hwndComboBox, MaxHistory);
	} else if (i != 0) {
		ComboBox_DeleteString(hwndComboBox, i);
		ComboBox_InsertString(hwndComboBox, 0, pszKeyword);
	}

	SetKeyword(pszKeyword);

	m_Options.AddKeywordHistory(pszKeyword);

	return true;
}


void CEventSearchSettingsDialog::ShowButton(int ID, bool fShow)
{
	if (m_hDlg != nullptr)
		ShowDlgItem(m_hDlg, ID, fShow);
}


void CEventSearchSettingsDialog::CheckButton(int ID, bool fCheck)
{
	if (m_hDlg != nullptr)
		::CheckDlgButton(m_hDlg, ID, fCheck);
}


void CEventSearchSettingsDialog::SetFocus(int ID)
{
	if (m_hDlg != nullptr)
		SetDlgItemFocus(m_hDlg, ID);
}


void CEventSearchSettingsDialog::SetSearchTargetList(const LPCTSTR *ppszList, int Count)
{
	if (ppszList != nullptr && Count > 0) {
		m_SearchTargetList.resize(Count);
		for (int i = 0; i < Count; i++)
			m_SearchTargetList[i] = ppszList[i];
	} else {
		m_SearchTargetList.clear();
	}
}


bool CEventSearchSettingsDialog::SetSearchTarget(int Target)
{
	if (Target < 0 || static_cast<size_t>(Target) >= m_SearchTargetList.size())
		return false;
	m_SearchTarget = Target;
	return true;
}


INT_PTR CEventSearchSettingsDialog::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		AddControl(IDC_EVENTSEARCH_KEYWORD, AlignFlag::Horz);
		AddControl(IDC_EVENTSEARCH_KEYWORDMENU, AlignFlag::Right);
		AddControl(IDC_EVENTSEARCH_SETTINGSLIST, AlignFlag::Horz);
		AddControl(IDC_EVENTSEARCH_SETTINGSLIST_SAVE, AlignFlag::Right);
		AddControl(IDC_EVENTSEARCH_SETTINGSLIST_DELETE, AlignFlag::Right);
		AddControl(IDC_EVENTSEARCH_SEARCHTARGET, AlignFlag::Right);
		AddControl(IDC_EVENTSEARCH_SEARCH, AlignFlag::Right);

		// キーワード
		{
			DlgComboBox_LimitText(hDlg, IDC_EVENTSEARCH_KEYWORD, CEventSearchSettings::MAX_KEYWORD_LENGTH - 1);

			LPCTSTR pszKeyword;
			for (int i = 0; (pszKeyword = m_Options.GetKeywordHistory(i)) != nullptr; i++)
				DlgComboBox_AddString(hDlg, IDC_EVENTSEARCH_KEYWORD, pszKeyword);

			COMBOBOXINFO cbi;
			cbi.cbSize = sizeof(cbi);
			::GetComboBoxInfo(::GetDlgItem(hDlg, IDC_EVENTSEARCH_KEYWORD), &cbi);
			m_KeywordEditSubclass.SetSubclass(cbi.hwndItem);

			InitDropDownButton(hDlg, IDC_EVENTSEARCH_KEYWORDMENU);
		}

		// キーワード検索対象
		{
			static const LPCTSTR KeywordTargetList[] = {
				TEXT("番組名と番組情報"),
				TEXT("番組名"),
				TEXT("番組情報"),
			};

			for (const LPCTSTR pszText : KeywordTargetList)
				DlgComboBox_AddString(hDlg, IDC_EVENTSEARCH_KEYWORDTARGET, pszText);
		}

		// ジャンル
		{
			const HWND hwndGenre = GetDlgItem(hDlg, IDC_EVENTSEARCH_GENRE);

			// 最初からTVS_CHECKBOXESを付けるとデフォルトでチェックできない
			::SetWindowLong(
				hwndGenre, GWL_STYLE,
				::GetWindowLong(hwndGenre, GWL_STYLE) | TVS_CHECKBOXES);

			TVINSERTSTRUCT tvis;
			CEpgGenre EpgGenre;
			TCHAR szText[256];
			tvis.hInsertAfter = TVI_LAST;
			tvis.item.mask = TVIF_STATE | TVIF_TEXT | TVIF_PARAM;
			tvis.item.stateMask = ~0U;
			for (int i = 0; i < CEpgGenre::NUM_GENRE; i++) {
				LPCTSTR pszText = EpgGenre.GetText(i, -1);
				if (pszText != nullptr) {
					StringCopy(szText, pszText);
					tvis.hParent = TVI_ROOT;
					tvis.item.state = INDEXTOSTATEIMAGEMASK(1);
					if (m_fGenreExpanded[i])
						tvis.item.state |= TVIS_EXPANDED;
					tvis.item.pszText = szText;
					tvis.item.lParam = GENRE_LPARAM_PACK(i, -1);
					tvis.hParent = TreeView_InsertItem(hwndGenre, &tvis);
					for (int j = 0; j < CEpgGenre::NUM_SUB_GENRE; j++) {
						pszText = EpgGenre.GetText(i, j);
						if (pszText != nullptr) {
							StringCopy(szText, pszText);
							tvis.item.state = INDEXTOSTATEIMAGEMASK(1);
							tvis.item.lParam = GENRE_LPARAM_PACK(i, j);
							TreeView_InsertItem(hwndGenre, &tvis);
						}
					}
				}
			}
		}

		// 長さ
		DlgUpDown_SetRange(hDlg, IDC_EVENTSEARCH_DURATION_SHORT_SPIN, 1, 999);

		// CA
		{
			static const LPCTSTR pszCAList[] = {
				TEXT("無料"),
				TEXT("有料"),
			};

			for (const LPCTSTR pszText : pszCAList)
				DlgComboBox_AddString(hDlg, IDC_EVENTSEARCH_CA_LIST, pszText);
		}

		// 映像
		{
			static const LPCTSTR pszVideoList[] = {
				TEXT("HD"),
				TEXT("SD"),
			};

			for (const LPCTSTR pszText : pszVideoList)
				DlgComboBox_AddString(hDlg, IDC_EVENTSEARCH_VIDEO_LIST, pszText);
		}

		// 設定保存
		{
			DlgComboBox_LimitText(hDlg, IDC_EVENTSEARCH_SETTINGSLIST, CEventSearchSettings::MAX_NAME_LENGTH - 1);

			const size_t Count = m_Options.GetSearchSettingsCount();
			for (size_t i = 0; i < Count; i++) {
				const CEventSearchSettings *pSettings = m_Options.GetSearchSettings(i);
				DlgComboBox_AddString(
					hDlg, IDC_EVENTSEARCH_SETTINGSLIST,
					pSettings->Name.c_str());
			}

			DlgComboBox_SetCueBanner(hDlg, IDC_EVENTSEARCH_SETTINGSLIST, TEXT("設定名"));
		}

		// 検索対象
		if (!m_SearchTargetList.empty()) {
			for (const String &e : m_SearchTargetList)
				DlgComboBox_AddString(hDlg, IDC_EVENTSEARCH_SEARCHTARGET, e.c_str());
			DlgComboBox_SetCurSel(hDlg, IDC_EVENTSEARCH_SEARCHTARGET, m_SearchTarget);
		} else {
			ShowDlgItem(hDlg, IDC_EVENTSEARCH_SEARCHTARGET, false);
		}

		SetSettings(m_SearchSettings);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_EVENTSEARCH_SEARCH:
			if (m_pEventHandler != nullptr) {
				m_pEventHandler->OnSearch();
			}
			return TRUE;

		case IDC_EVENTSEARCH_GENRE_CHECKALL:
		case IDC_EVENTSEARCH_GENRE_UNCHECKALL:
			{
				const BOOL fCheck = LOWORD(wParam) == IDC_EVENTSEARCH_GENRE_CHECKALL;
				const HWND hwndGenre = ::GetDlgItem(hDlg, IDC_EVENTSEARCH_GENRE);
				HTREEITEM hItem = TreeView_GetChild(hwndGenre, TVI_ROOT);
				while (hItem != nullptr) {
					TreeView_SetCheckState(hwndGenre, hItem, fCheck);
					HTREEITEM hChild = TreeView_GetChild(hwndGenre, hItem);
					while (hChild != nullptr) {
						TreeView_SetCheckState(hwndGenre, hChild, fCheck);
						hChild = TreeView_GetNextSibling(hwndGenre, hChild);
					}
					hItem = TreeView_GetNextSibling(hwndGenre, hItem);
				}
				SetGenreStatus();
			}
			return TRUE;

		case IDC_EVENTSEARCH_DAYOFWEEK:
			EnableDlgItemsSyncCheckBox(
				hDlg,
				IDC_EVENTSEARCH_DAYOFWEEK_SUNDAY,
				IDC_EVENTSEARCH_DAYOFWEEK_SATURDAY,
				IDC_EVENTSEARCH_DAYOFWEEK);
			return TRUE;

		case IDC_EVENTSEARCH_TIME:
			EnableDlgItemsSyncCheckBox(
				hDlg,
				IDC_EVENTSEARCH_TIME_START_HOUR,
				IDC_EVENTSEARCH_TIME_END_MINUTE,
				IDC_EVENTSEARCH_TIME);
			return TRUE;

		case IDC_EVENTSEARCH_DURATION:
			EnableDlgItemsSyncCheckBox(
				hDlg,
				IDC_EVENTSEARCH_DURATION_SHORT_INPUT,
				IDC_EVENTSEARCH_DURATION_SHORT_UNIT,
				IDC_EVENTSEARCH_DURATION);
			return TRUE;

		case IDC_EVENTSEARCH_CA:
			EnableDlgItemSyncCheckBox(
				hDlg,
				IDC_EVENTSEARCH_CA_LIST,
				IDC_EVENTSEARCH_CA);
			return TRUE;

		case IDC_EVENTSEARCH_VIDEO:
			EnableDlgItemSyncCheckBox(
				hDlg,
				IDC_EVENTSEARCH_VIDEO_LIST,
				IDC_EVENTSEARCH_VIDEO);
			return TRUE;

		case IDC_EVENTSEARCH_HIGHLIGHT:
			if (m_pEventHandler != nullptr) {
				m_pEventHandler->OnHighlightResult(
					DlgCheckBox_IsChecked(hDlg, IDC_EVENTSEARCH_HIGHLIGHT));
			}
			return TRUE;

		case IDC_EVENTSEARCH_SETTINGSLIST:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				const LRESULT Sel = DlgComboBox_GetCurSel(hDlg, IDC_EVENTSEARCH_SETTINGSLIST);

				if (Sel >= 0) {
					TCHAR szName[CEventSearchSettings::MAX_NAME_LENGTH];

					if (DlgComboBox_GetLBString(hDlg, IDC_EVENTSEARCH_SETTINGSLIST, Sel, szName) > 0) {
						const CEventSearchSettings *pSettings = m_Options.GetSearchSettingsByName(szName);

						if (pSettings != nullptr) {
							SetSettings(*pSettings);
						}
					}
				}
			}
			return TRUE;

		case IDC_EVENTSEARCH_SETTINGSLIST_SAVE:
			{
				const HWND hwndComboBox = ::GetDlgItem(hDlg, IDC_EVENTSEARCH_SETTINGSLIST);
				TCHAR szName[CEventSearchSettings::MAX_NAME_LENGTH];

				if (::GetWindowText(hwndComboBox, szName, lengthof(szName)) < 1) {
					::MessageBox(hDlg, TEXT("名前を入力してください。"), TEXT("設定の保存"), MB_OK | MB_ICONEXCLAMATION);
					SetDlgItemFocus(hDlg, IDC_EVENTSEARCH_SETTINGSLIST);
					return TRUE;
				}

				const int Index = ComboBox_FindStringExact(hwndComboBox, -1, szName);
				CEventSearchSettings *pSettings = m_Options.GetSearchSettingsByName(szName);
				if (pSettings != nullptr) {
					GetSettings(pSettings);
				} else {
					CEventSearchSettings Settings;

					GetSettings(&Settings);
					Settings.Name = szName;
					m_Options.AddSearchSettings(Settings);
				}
				if (Index == CB_ERR) {
					ComboBox_AddString(hwndComboBox, szName);
				}

				TCHAR szText[lengthof(szName) + 64];
				StringFormat(
					szText, TEXT("設定 \"{}\" を{}しました。"),
					szName, pSettings == nullptr ? TEXT("保存") : TEXT("上書き"));
				::MessageBox(hDlg, szText, TEXT("設定の保存"), MB_OK | MB_ICONINFORMATION);
			}
			return TRUE;

		case IDC_EVENTSEARCH_SETTINGSLIST_DELETE:
			{
				const HWND hwndComboBox = ::GetDlgItem(hDlg, IDC_EVENTSEARCH_SETTINGSLIST);
				TCHAR szName[CEventSearchSettings::MAX_NAME_LENGTH];

				if (::GetWindowText(hwndComboBox, szName, lengthof(szName)) > 0) {
					const int Index = ComboBox_FindStringExact(hwndComboBox, -1, szName);

					if (Index != CB_ERR) {
						const int i = m_Options.FindSearchSettings(szName);
						if (i >= 0)
							m_Options.DeleteSearchSettings(i);

						ComboBox_DeleteString(hwndComboBox, Index);

						TCHAR szText[lengthof(szName) + 64];
						StringFormat(szText, TEXT("設定 \"{}\" を削除しました。"), szName);
						::MessageBox(hDlg, szText, TEXT("設定の削除"), MB_OK | MB_ICONINFORMATION);
					}
				}
			}
			return TRUE;

		case IDC_EVENTSEARCH_SEARCHTARGET:
			if (HIWORD(wParam) == CBN_SELCHANGE)
				m_SearchTarget = static_cast<int>(DlgComboBox_GetCurSel(hDlg, IDC_EVENTSEARCH_SEARCHTARGET));
			return TRUE;

		case IDC_EVENTSEARCH_KEYWORDMENU:
			{
				CPopupMenu Menu(
					GetAppClass().GetResourceInstance(),
					MAKEINTRESOURCE(IDM_EVENTSEARCHKEYWORD));
				TCHAR szKeyword[CEventSearchSettings::MAX_KEYWORD_LENGTH];
				RECT rc;

				::GetDlgItemText(hDlg, IDC_EVENTSEARCH_KEYWORD, szKeyword, lengthof(szKeyword));
				Menu.EnableItem(
					IDC_EVENTSEARCH_DELETEKEYWORD,
					szKeyword[0] != _T('\0') &&
						DlgComboBox_FindStringExact(hDlg, IDC_EVENTSEARCH_KEYWORD, -1, szKeyword) >= 0);
				::GetWindowRect(::GetDlgItem(hDlg, IDC_EVENTSEARCH_KEYWORDMENU), &rc);
				const POINT pt = {rc.right, rc.bottom};
				Menu.Show(hDlg, &pt, TPM_RIGHTALIGN);
			}
			return TRUE;

		case IDC_EVENTSEARCH_DELETEKEYWORD:
			{
				int Index = -1;

				if (DlgComboBox_GetDroppedState(hDlg, IDC_EVENTSEARCH_KEYWORD)) {
					Index = static_cast<int>(DlgComboBox_GetCurSel(hDlg, IDC_EVENTSEARCH_KEYWORD));
				} else {
					TCHAR szKeyword[CEventSearchSettings::MAX_KEYWORD_LENGTH];

					if (::GetDlgItemText(hDlg, IDC_EVENTSEARCH_KEYWORD, szKeyword, lengthof(szKeyword)) > 0) {
						Index = static_cast<int>(DlgComboBox_FindStringExact(hDlg, IDC_EVENTSEARCH_KEYWORD, -1, szKeyword));
						if (Index >= 0)
							::SetDlgItemText(hDlg, IDC_EVENTSEARCH_KEYWORD, TEXT(""));
					}
				}
				if (Index >= 0) {
					DlgComboBox_DeleteItem(hDlg, IDC_EVENTSEARCH_KEYWORD, Index);
					m_Options.DeleteKeywordHistory(Index);
				}
			}
			return TRUE;

		case IDC_EVENTSEARCH_CLEARKEYWORDHISTORY:
			DlgComboBox_Clear(hDlg, IDC_EVENTSEARCH_KEYWORD);
			m_Options.ClearKeywordHistory();
			return TRUE;
		}
		return TRUE;

#if 0
	case WM_CTLCOLORSTATIC:
		if (reinterpret_cast<HWND>(lParam) == ::GetDlgItem(hDlg, IDC_EVENTSEARCH_GENRE_STATUS)) {
			const HDC hdc = reinterpret_cast<HDC>(wParam);

			::SetTextColor(hdc, ::GetSysColor(COLOR_GRAYTEXT));
			::SetBkColor(hdc, ::GetSysColor(COLOR_3DFACE));
			return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_3DFACE));
		}
		break;
#endif

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case NM_CLICK:
			{
				const NMHDR *pnmh = reinterpret_cast<const NMHDR*>(lParam);

				if (pnmh->idFrom == IDC_EVENTSEARCH_GENRE) {
					const HWND hwndTree = pnmh->hwndFrom;
					TVHITTESTINFO tvhti;
					const DWORD Pos = ::GetMessagePos();
					tvhti.pt.x = GET_X_LPARAM(Pos);
					tvhti.pt.y = GET_Y_LPARAM(Pos);
					::ScreenToClient(hwndTree, &tvhti.pt);
					if (TreeView_HitTest(hwndTree, &tvhti) != nullptr
							&& (tvhti.flags & TVHT_ONITEMSTATEICON) != 0) {
						TreeView_SelectItem(hwndTree, tvhti.hItem);
						::PostMessage(hDlg, WM_PROGRAM_SEARCH_GENRE_CHANGED, 0, 0);
					}
					return TRUE;
				}
			}
			break;

		case TVN_KEYDOWN:
			{
				const NMTVKEYDOWN *pnmtvkd = reinterpret_cast<const NMTVKEYDOWN*>(lParam);

				if (pnmtvkd->wVKey == VK_SPACE)
					::PostMessage(hDlg, WM_PROGRAM_SEARCH_GENRE_CHANGED, 0, 0);
			}
			return TRUE;
		}
		break;

	case WM_PROGRAM_SEARCH_GENRE_CHANGED:
		SetGenreStatus();
		return TRUE;

	case WM_DESTROY:
		// ジャンルのツリー展開状態を保存
		{
			const HWND hwndGenre = ::GetDlgItem(hDlg, IDC_EVENTSEARCH_GENRE);
			HTREEITEM hItem = TreeView_GetChild(hwndGenre, TVI_ROOT);
			TVITEM tvi;
			tvi.mask = TVIF_STATE | TVIF_PARAM;
			tvi.stateMask = ~0U;
			for (int i = 0; hItem != nullptr; i++) {
				tvi.hItem = hItem;
				TreeView_GetItem(hwndGenre, &tvi);
				m_fGenreExpanded[GENRE_LPARAM_LEVEL1(tvi.lParam)] = (tvi.state & TVIS_EXPANDED) != 0;
				hItem = TreeView_GetNextSibling(hwndGenre, hItem);
			}
		}

		return TRUE;
	}

	return FALSE;
}


void CEventSearchSettingsDialog::GetGenreSettings(CEventSearchSettings *pSettings) const
{
	const HWND hwndGenre = ::GetDlgItem(m_hDlg, IDC_EVENTSEARCH_GENRE);
	HTREEITEM hItem = TreeView_GetChild(hwndGenre, TVI_ROOT);
	TVITEM tvi;
	tvi.mask = TVIF_PARAM | TVIF_STATE;
	tvi.stateMask = TVIS_STATEIMAGEMASK;
	while (hItem != nullptr) {
		tvi.hItem = hItem;
		TreeView_GetItem(hwndGenre, &tvi);
		const int Level1 = GENRE_LPARAM_LEVEL1(tvi.lParam);
		if ((tvi.state & TVIS_STATEIMAGEMASK) >> 12 > 1) {
			pSettings->Genre1 |= 1 << Level1;
			pSettings->fGenre = true;
		}
		HTREEITEM hChild = TreeView_GetChild(hwndGenre, hItem);
		while (hChild != nullptr) {
			tvi.hItem = hChild;
			TreeView_GetItem(hwndGenre, &tvi);
			if ((tvi.state & TVIS_STATEIMAGEMASK) >> 12 > 1) {
				const int Level2 = GENRE_LPARAM_LEVEL2(tvi.lParam);
				pSettings->Genre2[Level1] |= 1 << Level2;
				pSettings->fGenre = true;
			}
			hChild = TreeView_GetNextSibling(hwndGenre, hChild);
		}
		hItem = TreeView_GetNextSibling(hwndGenre, hItem);
	}
}


void CEventSearchSettingsDialog::SetGenreStatus()
{
	const HWND hwndGenre = ::GetDlgItem(m_hDlg, IDC_EVENTSEARCH_GENRE);
	HTREEITEM hItem = TreeView_GetChild(hwndGenre, TVI_ROOT);
	int CheckCount = 0;
	TVITEM tvi;
	tvi.mask = TVIF_PARAM | TVIF_STATE;
	tvi.stateMask = TVIS_STATEIMAGEMASK;
	while (hItem != nullptr) {
		tvi.hItem = hItem;
		TreeView_GetItem(hwndGenre, &tvi);
		if ((tvi.state & TVIS_STATEIMAGEMASK) >> 12 > 1)
			CheckCount++;
		HTREEITEM hChild = TreeView_GetChild(hwndGenre, hItem);
		while (hChild != nullptr) {
			tvi.hItem = hChild;
			TreeView_GetItem(hwndGenre, &tvi);
			if ((tvi.state & TVIS_STATEIMAGEMASK) >> 12 > 1)
				CheckCount++;
			hChild = TreeView_GetNextSibling(hwndGenre, hChild);
		}
		hItem = TreeView_GetNextSibling(hwndGenre, hItem);
	}

	TCHAR szText[256];
	if (CheckCount > 0)
		StringFormat(szText, TEXT("{} 個選択"), CheckCount);
	else
		StringCopy(szText, TEXT("指定なし"));
	::SetDlgItemText(m_hDlg, IDC_EVENTSEARCH_GENRE_STATUS, szText);
}


LRESULT CEventSearchSettingsDialog::CKeywordEditSubclass::OnMessage(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_GETDLGCODE:
		if (wParam == VK_RETURN)
			return DLGC_WANTALLKEYS;
		break;

	case WM_KEYDOWN:
		if (wParam == VK_RETURN) {
			const HWND hwndComboBox = ::GetParent(hwnd);
			if (!ComboBox_GetDroppedState(hwndComboBox)) {
				::PostMessage(::GetParent(hwndComboBox), WM_COMMAND, IDC_EVENTSEARCH_SEARCH, 0);
				return 0;
			}
		} else if (wParam == VK_DELETE) {
			const int Length = ::GetWindowTextLength(hwnd);

			if (Length > 0) {
				const HWND hwndComboBox = ::GetParent(hwnd);

				if (ComboBox_GetDroppedState(hwndComboBox)) {
					const int Sel = ComboBox_GetCurSel(hwndComboBox);

					if (Sel >= 0) {
						DWORD Start, End;

						::SendMessage(
							hwnd, EM_GETSEL,
							reinterpret_cast<WPARAM>(&Start),
							reinterpret_cast<LPARAM>(&End));
						if (Start == 0 && End == static_cast<DWORD>(Length)) {
							TCHAR szKeyword[CEventSearchSettings::MAX_KEYWORD_LENGTH];

							if (::GetWindowText(hwnd, szKeyword, lengthof(szKeyword)) == Length
									&& ComboBox_FindStringExact(hwndComboBox, Sel - 1, szKeyword) == Sel) {
								::SendMessage(
									::GetParent(hwndComboBox), WM_COMMAND,
									IDC_EVENTSEARCH_DELETEKEYWORD, 0);
								return 0;
							}
						}
					}
				}
			}
			break;
		}
		break;

	case WM_CHAR:
		if ((wParam == '\r' || wParam == '\n')
				&& !ComboBox_GetDroppedState(::GetParent(hwnd)))
			return 0;
		break;
	}

	return CWindowSubclass::OnMessage(hwnd, uMsg, wParam, lParam);
}




CSearchEventInfo::CSearchEventInfo(
	const LibISDB::EventInfo &EventInfo, const CTunerChannelInfo &ChannelInfo)
	: LibISDB::EventInfo(EventInfo)
	, m_ChannelInfo(ChannelInfo)
{
}




CProgramSearchDialog::CProgramSearchDialog(CEventSearchOptions &Options)
	: m_Options(Options)
	, m_SearchSettingsDialog(Options)
{
	for (int i = 0; i < NUM_COLUMNS; i++)
		m_ColumnWidth[i] = -1;

	RegisterUIChild(&m_SearchSettingsDialog);
	SetStyleScaling(&m_StyleScaling);
}


CProgramSearchDialog::~CProgramSearchDialog()
{
	if (m_pEventHandler != nullptr)
		m_pEventHandler->m_pSearchDialog = nullptr;
}


bool CProgramSearchDialog::Create(HWND hwndOwner)
{
	m_RichEditUtil.LoadRichEditLib();
	return CreateDialogWindow(hwndOwner, GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_PROGRAMSEARCH));
}


bool CProgramSearchDialog::SetEventHandler(CEventHandler *pHandler)
{
	if (m_pEventHandler != nullptr)
		m_pEventHandler->m_pSearchDialog = nullptr;
	if (pHandler != nullptr)
		pHandler->m_pSearchDialog = this;
	m_pEventHandler = pHandler;
	return true;
}


int CProgramSearchDialog::GetColumnWidth(int Index) const
{
	if (Index < 0 || Index >= NUM_COLUMNS)
		return 0;
	return m_ColumnWidth[Index];
}


bool CProgramSearchDialog::SetColumnWidth(int Index, int Width)
{
	if (Index < 0 || Index >= NUM_COLUMNS)
		return false;
	m_ColumnWidth[Index] = Width;
	return true;
}


bool CProgramSearchDialog::Search(LPCTSTR pszKeyword)
{
	if (m_hDlg == nullptr || pszKeyword == nullptr)
		return false;
	m_SearchSettingsDialog.SetKeyword(pszKeyword);
	m_SearchSettingsDialog.BeginSearch();
	return true;
}


bool CProgramSearchDialog::SetHighlightResult(bool fHighlight)
{
	if (m_fHighlightResult != fHighlight) {
		m_fHighlightResult = fHighlight;
		if (m_pEventHandler != nullptr && !m_ResultMap.empty())
			m_pEventHandler->OnHighlightChange(fHighlight);
	}
	return true;
}


bool CProgramSearchDialog::IsHitEvent(const LibISDB::EventInfo *pEventInfo) const
{
	if (pEventInfo == nullptr)
		return false;
	return m_ResultMap.find(GetResultMapKey(pEventInfo)) != m_ResultMap.end();
}


void CProgramSearchDialog::SetResultListHeight(int Height)
{
	m_ResultListHeight = Height;
}


void CProgramSearchDialog::SetSearchTargetList(const LPCTSTR *ppszList, int Count)
{
	m_SearchSettingsDialog.SetSearchTargetList(ppszList, Count);
}


bool CProgramSearchDialog::SetSearchTarget(int Target)
{
	return m_SearchSettingsDialog.SetSearchTarget(Target);
}


int CProgramSearchDialog::GetSearchTarget() const
{
	return m_SearchSettingsDialog.GetSearchTarget();
}


INT_PTR CProgramSearchDialog::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		AddControl(IDC_PROGRAMSEARCH_SETTINGSPLACE, AlignFlag::Horz);
		AddControl(IDC_PROGRAMSEARCH_STATUS, AlignFlag::Horz);
		AddControl(IDC_PROGRAMSEARCH_RESULTPANE, AlignFlag::All);

		m_SearchSettingsDialog.SetEventHandler(this);
		m_SearchSettings.Keyword.clear();
		m_SearchSettingsDialog.SetSettings(m_SearchSettings);
		m_SearchSettingsDialog.Create(hDlg);
		m_SearchSettingsDialog.CheckButton(IDC_EVENTSEARCH_HIGHLIGHT, m_fHighlightResult);

		// 検索結果一覧
		{
			const HWND hwndList = ::GetDlgItem(hDlg, IDC_PROGRAMSEARCH_RESULT);
			ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
			SetListViewTooltipsTopMost(hwndList);

			bool fNeedFontSize = false;
			bool fAllZero = true;
			for (int i = 0; i < NUM_COLUMNS; i++) {
				if (m_ColumnWidth[i] < 0)
					fNeedFontSize = true;
				if (m_ColumnWidth[i] != 0)
					fAllZero = false;
			}

			// 以前のバージョンでカラムのデフォルト幅が全て0になることがあったのを対策
			if (fAllZero) {
				for (int i = 0; i < NUM_COLUMNS; i++)
					m_ColumnWidth[i] = -1;
				fNeedFontSize = true;
			}

			int FontSize;
			if (fNeedFontSize) {
				const HDC hdc = ::GetDC(hwndList);
				const HFONT hfont = GetWindowFont(hwndList);
				const HFONT hfontOld = SelectFont(hdc, hfont);
				TEXTMETRIC tm;
				::GetTextMetrics(hdc, &tm);
				FontSize = tm.tmHeight - tm.tmInternalLeading;
				SelectFont(hdc, hfontOld);
				::ReleaseDC(hwndList, hdc);
			}

			LVCOLUMN lvc;
			lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			lvc.fmt = LVCFMT_LEFT;
			lvc.cx = m_ColumnWidth[COLUMN_CHANNEL] >= 0 ? m_ColumnWidth[COLUMN_CHANNEL] : 8 * FontSize;
			lvc.pszText = const_cast<LPTSTR>(TEXT("チャンネル"));
			ListView_InsertColumn(hwndList, COLUMN_CHANNEL, &lvc);
			lvc.cx = m_ColumnWidth[COLUMN_TIME] >= 0 ? m_ColumnWidth[COLUMN_TIME] : 12 * FontSize;
			lvc.pszText = const_cast<LPTSTR>(TEXT("日時"));
			ListView_InsertColumn(hwndList, COLUMN_TIME, &lvc);
			lvc.cx = m_ColumnWidth[COLUMN_EVENTNAME] >= 0 ? m_ColumnWidth[COLUMN_EVENTNAME] : 20 * FontSize;
			lvc.pszText = const_cast<LPTSTR>(TEXT("番組名"));
			ListView_InsertColumn(hwndList, COLUMN_EVENTNAME, &lvc);

			m_SortColumn = -1;
			m_fSortDescending = false;
		}

		// 番組情報
		{
			LOGFONT lf;

			::GetObject(GetWindowFont(hDlg), sizeof(LOGFONT), &lf);
			const HDC hdc = ::GetDC(hDlg);
			CRichEditUtil::LogFontToCharFormat(hdc, &lf, &m_InfoTextFormat);
			::ReleaseDC(hDlg, hdc);
			CRichEditUtil::DisableAutoFont(::GetDlgItem(hDlg, IDC_PROGRAMSEARCH_INFO));
			::SendDlgItemMessage(hDlg, IDC_PROGRAMSEARCH_INFO, EM_SETEVENTMASK, 0, ENM_MOUSEEVENTS | ENM_LINK);
			::SendDlgItemMessage(hDlg, IDC_PROGRAMSEARCH_INFO, EM_SETBKGNDCOLOR, 0, GetThemeColor(COLOR_WINDOW));
			m_InfoTextFormat.crTextColor = GetThemeColor(COLOR_WINDOWTEXT);
		}

		SetWindowIcon(hDlg, GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDI_SEARCH));

		ApplyPosition();

		if (m_ResultListHeight >= 0)
			AdjustResultListHeight(m_ResultListHeight);

		m_fSplitterCursor = false;

		m_SearchSettingsDialog.SetFocus(IDC_EVENTSEARCH_KEYWORD);

		return FALSE;

	case WM_SIZE:
		{
			RECT rc;
			GetDlgItemRect(hDlg, IDC_PROGRAMSEARCH_SETTINGSPLACE, &rc);
			m_SearchSettingsDialog.SetPosition(&rc);

			InvalidateDlgItem(hDlg, IDC_PROGRAMSEARCH_STATUS);

			AdjustResultListHeight(-1);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			OnSearch();
			return TRUE;

		case IDCANCEL:
			if (m_pEventHandler == nullptr || m_pEventHandler->OnClose())
				::DestroyWindow(hDlg);
			return TRUE;
		}
		return TRUE;

	case WM_CTLCOLORSTATIC:
		if (reinterpret_cast<HWND>(lParam) == ::GetDlgItem(hDlg, IDC_PROGRAMSEARCH_STATUS)) {
			const HDC hdc = reinterpret_cast<HDC>(wParam);

			::SetTextColor(hdc, GetThemeColor(COLOR_WINDOWTEXT));
			::SetBkColor(hdc, GetThemeColor(COLOR_WINDOW));
			return reinterpret_cast<INT_PTR>(m_BackBrush.GetHandle());
		}
		break;

	case WM_SETCURSOR:
		if (reinterpret_cast<HWND>(wParam) == hDlg && LOWORD(lParam) == HTCLIENT && m_fSplitterCursor) {
			::SetCursor(::LoadCursor(nullptr, IDC_SIZENS));
			::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
			return TRUE;
		} else if (m_RichEditLink.OnSetCursor(hDlg, wParam, lParam)) {
			return TRUE;
		}
		break;

	case WM_LBUTTONDOWN:
		{
			const int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

			if (IsSplitterPos(x, y)) {
				RECT rc;

				::GetWindowRect(::GetDlgItem(hDlg, IDC_PROGRAMSEARCH_RESULT), &rc);
				m_ResultListHeight = rc.bottom - rc.top;
				m_fSplitterCursor = true;
				m_SplitterDragPos = y;
				::SetCursor(::LoadCursor(nullptr, IDC_SIZENS));
				::SetCapture(hDlg);
			}
		}
		return TRUE;

	case WM_LBUTTONUP:
		if (::GetCapture() == hDlg)
			::ReleaseCapture();
		return TRUE;

	case WM_MOUSEMOVE:
		{
			const int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

			if (::GetCapture() == hDlg)
				AdjustResultListHeight(m_ResultListHeight + (y - m_SplitterDragPos));
			else
				m_fSplitterCursor = IsSplitterPos(x, y);
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case NM_RCLICK:
		case NM_DBLCLK:
			{
				const NMITEMACTIVATE *pnmia = reinterpret_cast<const NMITEMACTIVATE*>(lParam);

				if (pnmia->hdr.idFrom == IDC_PROGRAMSEARCH_RESULT
						&& m_pEventHandler != nullptr
						&& pnmia->iItem >= 0) {
					LVITEM lvi;

					lvi.mask = LVIF_PARAM;
					lvi.iItem = pnmia->iItem;
					lvi.iSubItem = 0;
					if (ListView_GetItem(pnmia->hdr.hwndFrom, &lvi)) {
						const CSearchEventInfo *pEventInfo =
							reinterpret_cast<const CSearchEventInfo*>(lvi.lParam);
						if (pnmia->hdr.code == NM_DBLCLK)
							m_pEventHandler->OnLDoubleClick(pEventInfo);
						else
							m_pEventHandler->OnRButtonClick(pEventInfo);
						return TRUE;
					}
				}
			}
			break;

		case LVN_COLUMNCLICK:
			{
				const NMLISTVIEW *pnmlv = reinterpret_cast<const NMLISTVIEW*>(lParam);

				if (pnmlv->iSubItem == m_SortColumn) {
					m_fSortDescending = !m_fSortDescending;
				} else {
					m_SortColumn = pnmlv->iSubItem;
					m_fSortDescending = false;
				}
				SortSearchResult();
			}
			return TRUE;

		case LVN_ITEMCHANGED:
			{
				const NMLISTVIEW *pnmlv = reinterpret_cast<const NMLISTVIEW*>(lParam);

				if (!!(pnmlv->uChanged & LVIF_STATE)
						&& !!((pnmlv->uNewState ^ pnmlv->uOldState) & LVIS_SELECTED)) {
					UpdateEventInfoText();
				}
			}
			return TRUE;

		case LVN_GETEMPTYMARKUP:
			{
				NMLVEMPTYMARKUP *pnmMarkup = reinterpret_cast<NMLVEMPTYMARKUP*>(lParam);

				pnmMarkup->dwFlags = EMF_CENTERED;
				StringCopy(pnmMarkup->szMarkup, L"検索された番組はありません");
				::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
			}
			return TRUE;

		case EN_MSGFILTER:
			{
				MSGFILTER *pMsgFilter = reinterpret_cast<MSGFILTER*>(lParam);

				switch (pMsgFilter->msg) {
				case WM_RBUTTONUP:
					EventInfoUtil::EventInfoContextMenu(hDlg, ::GetDlgItem(hDlg, IDC_PROGRAMSEARCH_INFO));
					break;

				default:
					m_RichEditLink.OnMsgFilter(pMsgFilter);
					break;
				}
			}
			return TRUE;

#if 0
		case EN_LINK:
			{
				ENLINK *penl = reinterpret_cast<ENLINK*>(lParam);

				if (penl->msg == WM_LBUTTONUP) {
					CRichEditUtil::HandleLinkClick(penl);
				}
			}
			return TRUE;
#endif
		}
		break;

	case WM_DESTROY:
		// 検索結果のカラムの幅を保存
		{
			const HWND hwndList = ::GetDlgItem(hDlg, IDC_PROGRAMSEARCH_RESULT);
			for (int i = 0; i < lengthof(m_ColumnWidth); i++)
				m_ColumnWidth[i] = ListView_GetColumnWidth(hwndList, i);
		}

		{
			RECT rc;
			::GetWindowRect(::GetDlgItem(hDlg, IDC_PROGRAMSEARCH_RESULT), &rc);
			m_ResultListHeight = rc.bottom - rc.top;
		}

		ClearSearchResult();

		m_Searcher.Finalize();

		m_RichEditLink.Reset();

		return TRUE;
	}

	return FALSE;
}


void CProgramSearchDialog::OnDarkModeChanged(bool fDarkMode)
{
	::SendDlgItemMessage(m_hDlg, IDC_PROGRAMSEARCH_INFO, EM_SETBKGNDCOLOR, 0, GetThemeColor(COLOR_WINDOW));
	m_InfoTextFormat.crTextColor = GetThemeColor(COLOR_WINDOWTEXT);
	UpdateEventInfoText();
}


void CProgramSearchDialog::ApplyStyle()
{
	CResizableDialog::ApplyStyle();

	if (m_hDlg != nullptr) {
		const HWND hwnd = ::GetDlgItem(m_hDlg, IDC_PROGRAMSEARCH_INFO);
		const HDC hdc = ::GetDC(hwnd);
		LOGFONT lf;
		m_Font.GetLogFont(&lf);
		CRichEditUtil::LogFontToCharFormat(hdc, &lf, &m_InfoTextFormat);
		::ReleaseDC(hwnd, hdc);
	}
}


int CALLBACK CProgramSearchDialog::ResultCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	const CSearchEventInfo *pInfo1 = reinterpret_cast<const CSearchEventInfo*>(lParam1);
	const CSearchEventInfo *pInfo2 = reinterpret_cast<const CSearchEventInfo*>(lParam2);
	int Cmp;

	switch (LOWORD(lParamSort)) {
	case COLUMN_CHANNEL:   // チャンネル
		Cmp = ::lstrcmpi(pInfo1->GetChannelInfo().GetName(), pInfo2->GetChannelInfo().GetName());
		break;
	case COLUMN_TIME:      // 日時
		Cmp = pInfo1->StartTime.Compare(pInfo2->StartTime);
		break;
	case COLUMN_EVENTNAME: // 番組名
		if (pInfo1->EventName.empty()) {
			if (pInfo2->EventName.empty())
				Cmp = 0;
			else
				Cmp = -1;
		} else {
			if (pInfo2->EventName.empty())
				Cmp = 1;
			else
				Cmp = ::lstrcmpi(pInfo1->EventName.c_str(), pInfo2->EventName.c_str());
		}
		break;
	}
	return HIWORD(lParamSort) == 0 ? Cmp : -Cmp;
}


bool CProgramSearchDialog::AddSearchResult(CSearchEventInfo *pEventInfo)
{
	if (m_hDlg == nullptr || pEventInfo == nullptr)
		return false;

	const HWND hwndList = ::GetDlgItem(m_hDlg, IDC_PROGRAMSEARCH_RESULT);
	LVITEM lvi;
	TCHAR szText[256];

	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iItem = ListView_GetItemCount(hwndList);
	lvi.iSubItem = COLUMN_CHANNEL;
	StringCopy(szText, pEventInfo->GetChannelInfo().GetName());
	lvi.pszText = szText;
	lvi.lParam = reinterpret_cast<LPARAM>(pEventInfo);
	ListView_InsertItem(hwndList, &lvi);
	LibISDB::DateTime Start, End;
	EpgUtil::EpgTimeToDisplayTime(pEventInfo->StartTime, &Start);
	pEventInfo->GetEndTime(&End);
	EpgUtil::EpgTimeToDisplayTime(&End);
	StringFormat(
		szText,
		TEXT("{:02}/{:02}({}) {:02}:{:02}～{:02}:{:02}"),
		Start.Month, Start.Day,
		GetDayOfWeekText(Start.DayOfWeek),
		Start.Hour, Start.Minute,
		End.Hour, End.Minute);
	lvi.mask = LVIF_TEXT;
	lvi.iSubItem = COLUMN_TIME;
	//lvi.pszText = szText;
	ListView_SetItem(hwndList, &lvi);
	//lvi.mask = LVIF_TEXT;
	lvi.iSubItem = COLUMN_EVENTNAME;
	StringCopy(szText, pEventInfo->EventName.c_str());
	//lvi.pszText = szText;
	ListView_SetItem(hwndList, &lvi);

	m_ResultMap.emplace(GetResultMapKey(pEventInfo), pEventInfo);

	return true;
}


void CProgramSearchDialog::ClearSearchResult()
{
	m_ResultMap.clear();

	const HWND hwndList = ::GetDlgItem(m_hDlg, IDC_PROGRAMSEARCH_RESULT);
	const int Items = ListView_GetItemCount(hwndList);
	LVITEM lvi;

	lvi.mask = LVIF_PARAM;
	lvi.iSubItem = 0;
	for (int i = 0; i < Items; i++) {
		lvi.iItem = i;
		ListView_GetItem(hwndList, &lvi);
		delete reinterpret_cast<CSearchEventInfo*>(lvi.lParam);
	}
	ListView_DeleteAllItems(hwndList);

	::SetDlgItemText(m_hDlg, IDC_PROGRAMSEARCH_STATUS, TEXT(""));
	::SetDlgItemText(m_hDlg, IDC_PROGRAMSEARCH_INFO, TEXT(""));
}


void CProgramSearchDialog::SortSearchResult()
{
	const HWND hwndList = ::GetDlgItem(m_hDlg, IDC_PROGRAMSEARCH_RESULT);

	ListView_SortItems(
		hwndList, ResultCompareFunc,
		MAKELPARAM(m_SortColumn, m_fSortDescending));
	SetListViewSortMark(hwndList, m_SortColumn, !m_fSortDescending);
}


void CProgramSearchDialog::UpdateEventInfoText()
{
	const HWND hwndList = ::GetDlgItem(m_hDlg, IDC_PROGRAMSEARCH_RESULT);
	const int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

	::SetDlgItemText(m_hDlg, IDC_PROGRAMSEARCH_INFO, TEXT(""));
	if (Sel >= 0) {
		LVITEM lvi;

		lvi.mask = LVIF_PARAM;
		lvi.iItem = Sel;
		lvi.iSubItem = 0;
		ListView_GetItem(hwndList, &lvi);
		const CSearchEventInfo *pEventInfo = reinterpret_cast<const CSearchEventInfo*>(lvi.lParam);
		SetEventInfoText(pEventInfo);
	}
}


void CProgramSearchDialog::SetEventInfoText(const LibISDB::EventInfo *pEventInfo)
{
	const HWND hwndInfo = ::GetDlgItem(m_hDlg, IDC_PROGRAMSEARCH_INFO);
	TCHAR szText[256];
	String Text;

	::SendMessage(hwndInfo, WM_SETREDRAW, FALSE, 0);
	FormatEventTimeText(pEventInfo, szText, lengthof(szText));
	CRichEditUtil::AppendText(hwndInfo, szText, &m_InfoTextFormat);
	FormatEventInfoText(pEventInfo, &Text);
	CRichEditUtil::AppendText(hwndInfo, Text.c_str(), &m_InfoTextFormat);
	HighlightKeyword();
	m_RichEditLink.DetectURL(hwndInfo, &m_InfoTextFormat, 1);
	POINT pt = {0, 0};
	::SendMessage(hwndInfo, EM_SETSCROLLPOS, 0, reinterpret_cast<LPARAM>(&pt));
	::SendMessage(hwndInfo, WM_SETREDRAW, TRUE, 0);
	::InvalidateRect(hwndInfo, nullptr, TRUE);
}


size_t CProgramSearchDialog::FormatEventTimeText(const LibISDB::EventInfo *pEventInfo, LPTSTR pszText, size_t MaxLength) const
{
	if (pEventInfo == nullptr) {
		pszText[0] = '\0';
		return 0;
	}

	TCHAR szEndTime[16];
	LibISDB::DateTime End;
	if (pEventInfo->Duration > 0 && pEventInfo->GetEndTime(&End)) {
		StringFormat(
			szEndTime,
			TEXT("～{}:{:02}"), End.Hour, End.Minute);
	} else {
		szEndTime[0] = '\0';
	}
	return StringFormat(
		pszText, MaxLength, TEXT("{}/{}/{}({}) {}:{:02}{}\r\n"),
		pEventInfo->StartTime.Year,
		pEventInfo->StartTime.Month,
		pEventInfo->StartTime.Day,
		GetDayOfWeekText(pEventInfo->StartTime.DayOfWeek),
		pEventInfo->StartTime.Hour,
		pEventInfo->StartTime.Minute,
		szEndTime);
}


void CProgramSearchDialog::FormatEventInfoText(const LibISDB::EventInfo *pEventInfo, String *pText) const
{
	pText->clear();

	if (pEventInfo == nullptr)
		return;

	*pText = pEventInfo->EventName;
	*pText += TEXT("\r\n\r\n");
	if (!pEventInfo->EventText.empty()) {
		*pText += pEventInfo->EventText;
		*pText += TEXT("\r\n");
	}
	if (!pEventInfo->ExtendedText.empty()) {
		if (!pEventInfo->EventText.empty())
			*pText += TEXT("\r\n");
		LibISDB::String ExtendedText;
		pEventInfo->GetConcatenatedExtendedText(&ExtendedText);
		*pText += ExtendedText;
	}

	const String::size_type Pos = pText->find_last_not_of(TEXT("\r\n"));
	if (Pos != String::npos && pText->length() > Pos + 2)
		pText->resize(Pos + 2);
}


void CProgramSearchDialog::HighlightKeyword()
{
	if (m_SearchSettings.Keyword.empty()
			|| (!m_SearchSettings.fEventName && !m_SearchSettings.fEventText))
		return;

	const HWND hwndInfo = ::GetDlgItem(m_hDlg, IDC_PROGRAMSEARCH_INFO);

	int FirstLine, LastLine;
	if (m_SearchSettings.fEventName)
		FirstLine = 1;
	else
		FirstLine = 3;
	if (m_SearchSettings.fEventText)
		LastLine = static_cast<int>(::SendMessage(hwndInfo, EM_GETLINECOUNT, 0, 0)) - 1;
	else
		LastLine = 1;

	CHARFORMAT2 cfHighlight;
	CRichEditUtil::CharFormatToCharFormat2(&m_InfoTextFormat, &cfHighlight);
	cfHighlight.dwMask |= CFM_BOLD | CFM_BACKCOLOR;
	cfHighlight.dwEffects |= CFE_BOLD;
	cfHighlight.crBackColor = m_fDarkMode ? RGB(128, 128, 0) : RGB(255, 255, 0);
	CHARRANGE cr, crOld;

	::SendMessage(hwndInfo, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&crOld));

	for (int i = FirstLine; i <= LastLine;) {
		const int LineIndex = static_cast<int>(::SendMessage(hwndInfo, EM_LINEINDEX, i, 0));
		TCHAR szText[2048];
		LPTSTR q = szText;
		int TotalLength = 0;

		while (i <= LastLine) {
#ifdef UNICODE
			q[0] = static_cast<WORD>(lengthof(szText) - 2 - TotalLength);
#else
			*reinterpret_cast<WORD*>(q) = static_cast<WORD>(sizeof(szText) - sizeof(WORD) - 1 - TotalLength);
#endif
			const int Length = static_cast<int>(::SendMessage(hwndInfo, EM_GETLINE, i, reinterpret_cast<LPARAM>(q)));
			i++;
			if (Length < 1)
				break;
			q += Length;
			TotalLength += Length;
			if (*(q - 1) == _T('\r') || *(q - 1) == _T('\n'))
				break;
		}
		if (TotalLength > 0) {
			szText[TotalLength] = _T('\0');

			if (m_SearchSettings.fRegExp) {
				LPCTSTR q = szText;
				CRegExp::TextRange Range;

				while (*q != _T('\0') && m_Searcher.GetRegExp().Match(q, &Range)) {
					q += Range.Start;
					cr.cpMin = LineIndex + static_cast<LONG>(q - szText);
					cr.cpMax = cr.cpMin + static_cast<LONG>(Range.Length);
					::SendMessage(hwndInfo, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&cr));
					::SendMessage(hwndInfo, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cfHighlight));
					q += Range.Length;
				}
			} else {
				LPCTSTR p = m_SearchSettings.Keyword.c_str();
				while (*p != _T('\0')) {
					TCHAR szWord[CEventSearchSettings::MAX_KEYWORD_LENGTH], Delimiter;
					bool fMinus = false;

					while (*p == _T(' '))
						p++;
					if (*p == _T('-')) {
						fMinus = true;
						p++;
					}
					if (*p == _T('"')) {
						p++;
						Delimiter = _T('"');
					} else {
						Delimiter = _T(' ');
					}
					int KeywordLength;
					for (KeywordLength = 0; *p != Delimiter && *p != _T('|') && *p != _T('\0'); KeywordLength++)
						szWord[KeywordLength] = *p++;
					if (*p == Delimiter)
						p++;
					if (!fMinus && KeywordLength > 0) {
						LPCTSTR q = szText;
						int Length;
						while (SearchNextKeyword(&q, szWord, KeywordLength, &Length)) {
							cr.cpMin = LineIndex + static_cast<LONG>(q - szText);
							cr.cpMax = cr.cpMin + Length;
							::SendMessage(hwndInfo, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&cr));
							::SendMessage(hwndInfo, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cfHighlight));
							q += Length;
						}
					}
					while (*p == _T(' '))
						p++;
					if (*p == _T('|'))
						p++;
				}
			}
		}
	}

	::SendMessage(hwndInfo, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&crOld));
}


bool CProgramSearchDialog::SearchNextKeyword(LPCTSTR *ppszText, LPCTSTR pKeyword, int KeywordLength, int *pLength) const
{
	const int Pos = m_Searcher.FindKeyword(*ppszText, pKeyword, KeywordLength, pLength);
	if (Pos < 0)
		return false;
	*ppszText += Pos;
	return true;
}


bool CProgramSearchDialog::IsSplitterPos(int x, int y) const
{
	RECT rcList, rcInfo;

	GetDlgItemRect(m_hDlg, IDC_PROGRAMSEARCH_RESULT, &rcList);
	GetDlgItemRect(m_hDlg, IDC_PROGRAMSEARCH_INFO, &rcInfo);
	return x >= rcInfo.left && x < rcInfo.right && y >= rcList.bottom && y < rcInfo.top;
}


void CProgramSearchDialog::AdjustResultListHeight(int Height)
{
	RECT rcPane, rcList, rcInfo;

	GetDlgItemRect(m_hDlg, IDC_PROGRAMSEARCH_RESULTPANE, &rcPane);
	GetDlgItemRect(m_hDlg, IDC_PROGRAMSEARCH_RESULT, &rcList);
	GetDlgItemRect(m_hDlg, IDC_PROGRAMSEARCH_INFO, &rcInfo);
	const int SplitterHeight = rcInfo.top - rcList.bottom;
	if (Height < 0)
		Height = (rcPane.bottom - rcPane.top) - (rcInfo.bottom - rcInfo.top) - SplitterHeight;
	if (rcPane.top + Height > rcPane.bottom - MIN_PANE_HEIGHT - SplitterHeight)
		Height = (rcPane.bottom - rcPane.top) - MIN_PANE_HEIGHT - SplitterHeight;
	if (Height < MIN_PANE_HEIGHT)
		Height = MIN_PANE_HEIGHT;
	::MoveWindow(
		::GetDlgItem(m_hDlg, IDC_PROGRAMSEARCH_RESULT),
		rcPane.left, rcPane.top, rcPane.right - rcPane.left, Height, TRUE);
	rcInfo.top = rcPane.top + Height + SplitterHeight;
	::MoveWindow(
		::GetDlgItem(m_hDlg, IDC_PROGRAMSEARCH_INFO),
		rcPane.left, rcInfo.top, rcPane.right - rcPane.left, std::max(rcPane.bottom - rcInfo.top, 0L), TRUE);
}


void CProgramSearchDialog::OnSearch()
{
	CEventSearchSettings Settings;

	if (!m_SearchSettingsDialog.GetSettings(&Settings))
		return;

	ClearSearchResult();

	if (!Settings.Keyword.empty()) {
		m_SearchSettingsDialog.AddToKeywordHistory(Settings.Keyword.c_str());

		if (Settings.fRegExp) {
			if (!m_Searcher.InitializeRegExp()) {
				::MessageBox(m_hDlg, TEXT("正規表現が利用できません。"), nullptr, MB_OK | MB_ICONEXCLAMATION);
				return;
			}
		}
	}

	m_SearchSettings = Settings;
	if (!m_Searcher.BeginSearch(m_SearchSettings))
		return;

	const HCURSOR hcurOld = ::SetCursor(::LoadCursor(nullptr, IDC_WAIT));
	const DWORD StartTime = ::GetTickCount();
	m_pEventHandler->Search(&m_Searcher);
	const DWORD SearchTime = TickTimeSpan(StartTime, ::GetTickCount());
	if (m_SortColumn >= 0)
		SortSearchResult();
	::SetCursor(hcurOld);

	TCHAR szStatus[CEventSearchSettings::MAX_KEYWORD_LENGTH + 64];
	CStaticStringFormatter StatusFormat(szStatus, lengthof(szStatus));
	if (!m_SearchSettings.Keyword.empty()) {
		if (m_SearchSettings.fGenre)
			StatusFormat.Append(TEXT("指定ジャンルから "));
		StatusFormat.AppendFormat(TEXT("{} に一致する番組"), m_SearchSettings.Keyword);
	} else {
		if (m_SearchSettings.fGenre)
			StatusFormat.Append(TEXT("指定ジャンルの番組"));
		else
			StatusFormat.Append(TEXT("すべての番組"));
	}
	StatusFormat.AppendFormat(
		TEXT(" {} 件 ({}.{:02} 秒)"),
		ListView_GetItemCount(::GetDlgItem(m_hDlg, IDC_PROGRAMSEARCH_RESULT)),
		SearchTime / 1000, SearchTime / 10 % 100);
	::SetDlgItemText(m_hDlg, IDC_PROGRAMSEARCH_STATUS, StatusFormat.GetString());

	m_pEventHandler->OnEndSearch();
}


void CProgramSearchDialog::OnHighlightResult(bool fHighlight)
{
	SetHighlightResult(fHighlight);
}




CProgramSearchDialog::CEventHandler::~CEventHandler()
{
	if (m_pSearchDialog != nullptr)
		m_pSearchDialog->m_pEventHandler = nullptr;
}


bool CProgramSearchDialog::CEventHandler::Search(CEventSearcher *pSearcher)
{
	m_pSearcher = pSearcher;

	return OnSearch();
}


bool CProgramSearchDialog::CEventHandler::AddSearchResult(CSearchEventInfo *pEventInfo)
{
	if (pEventInfo == nullptr || m_pSearchDialog == nullptr)
		return false;
	return m_pSearchDialog->AddSearchResult(pEventInfo);
}


bool CProgramSearchDialog::CEventHandler::Match(const LibISDB::EventInfo *pEventInfo) const
{
	return m_pSearcher->Match(pEventInfo);
}


}	// namespace TVTest
