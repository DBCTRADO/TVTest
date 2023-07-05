/*
  TVTest
  Copyright(c) 2008-2022 DBCTRADO

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
#include "AppMain.h"
#include "ProgramGuide.h"
#include "LogoManager.h"
#include "EpgChannelSettings.h"
#include "ProgramGuideToolbarOptions.h"
#include "DPIUtil.h"
#include "DarkMode.h"
#include "LibISDB/LibISDB/Utilities/Sort.hpp"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace
{

#define TITLE_TEXT TEXT("EPG番組表")

// 現在時刻を更新するタイマのID
constexpr UINT TIMER_ID_UPDATECURTIME = 1;

// メニューの位置
constexpr int MENU_DATE         = 0;
constexpr int MENU_CHANNELGROUP = 1;

constexpr int MAX_CHANNEL_GROUP_MENU_ITEMS =
	CM_PROGRAMGUIDE_CHANNELGROUP_LAST - CM_PROGRAMGUIDE_CHANNELGROUP_FIRST + 1;
constexpr int MAX_CHANNEL_PROVIDER_MENU_ITEMS =
	CM_PROGRAMGUIDE_CHANNELPROVIDER_LAST - CM_PROGRAMGUIDE_CHANNELPROVIDER_FIRST + 1;

}




namespace ProgramGuide
{


class CEventItem
{
	static constexpr size_t MAX_TITLE_LENGTH = 256;

	const LibISDB::EventInfo *m_pEventInfo = nullptr;
	const LibISDB::EventInfo *m_pCommonEventInfo = nullptr;
	LibISDB::DateTime m_StartTime;
	LibISDB::DateTime m_EndTime;
	int m_TitleLines = 0;
	int m_ItemPos = -1;
	int m_ItemLines = 0;
	int m_ItemColumns = 1;
	bool m_fSelected = false;

	int GetTitleText(LPTSTR pszText, int MaxLength, bool fUseARIBSymbol) const;
	int GetTimeText(LPTSTR pszText, int MaxLength) const;
	String GetEventText(bool fUseARIBSymbol) const;

public:
	CEventItem(const LibISDB::EventInfo *pInfo);
	CEventItem(const LibISDB::DateTime &StartTime, DWORD Duration);

	const LibISDB::EventInfo *GetEventInfo() const { return m_pEventInfo; }
	const LibISDB::EventInfo *GetCommonEventInfo() const { return m_pCommonEventInfo; }
	const LibISDB::DateTime &GetStartTime() const { return m_StartTime; }
	bool SetStartTime(const LibISDB::DateTime &Time);
	const LibISDB::DateTime &GetEndTime() const { return m_EndTime; }
	bool SetEndTime(const LibISDB::DateTime &Time);
	int GetGenre(int Level = 0) const;
	bool SetCommonEvent(const LibISDB::EventInfo *pEvent);
	int GetTitleLines() const { return m_TitleLines; }
	void CalcTitleLines(CTextDraw &TextDraw, int Width, bool fUseARIBSymbol);
	void ResetTitleLines() { m_TitleLines = 0; }
	void DrawTitle(CTextDraw &TextDraw, const RECT &Rect, int LineHeight, bool fUseARIBSymbol) const;
	void DrawText(CTextDraw &TextDraw, const RECT &Rect, int LineHeight, CTextDraw::DrawFlag TextDrawFlags, bool fUseARIBSymbol) const;
	void GetTimeSize(CTextDraw &TextDraw, SIZE *pSize) const;
	int GetItemPos() const { return m_ItemPos; }
	void SetItemPos(int Pos) { m_ItemPos = Pos; }
	int GetItemLines() const { return m_ItemLines; }
	void SetItemLines(int Lines) { m_ItemLines = Lines; }
	int GetItemColumns() const { return m_ItemColumns; }
	void SetItemColumns(int Columns) { m_ItemColumns = Columns; }
	bool IsNullItem() const { return m_pEventInfo == nullptr; }
	void SetSelected(bool fSelected) { m_fSelected = fSelected; }
	bool IsSelected() const { return m_fSelected; }
};


CEventItem::CEventItem(const LibISDB::EventInfo *pInfo)
	: m_pEventInfo(pInfo)
{
	m_pEventInfo->GetStartTime(&m_StartTime);
	m_StartTime.Second = 0;
	m_pEventInfo->GetEndTime(&m_EndTime);
	m_EndTime.Second = 0;
}


CEventItem::CEventItem(const LibISDB::DateTime &StartTime, DWORD Duration)
	: m_StartTime(StartTime)
{
	m_EndTime = StartTime;
	m_EndTime.OffsetSeconds(Duration);
}


bool CEventItem::SetStartTime(const LibISDB::DateTime &Time)
{
	if (Time >= m_EndTime)
		return false;
	m_StartTime = Time;
	return true;
}


bool CEventItem::SetEndTime(const LibISDB::DateTime &Time)
{
	if (m_StartTime >= Time)
		return false;
	m_EndTime = Time;
	return true;
}


int CEventItem::GetGenre(int Level) const
{
	if (m_pEventInfo != nullptr) {
		const LibISDB::EventInfo::ContentNibbleInfo *pContentNibble;

		if (m_pEventInfo->ContentNibble.NibbleCount > 0) {
			pContentNibble = &m_pEventInfo->ContentNibble;
		} else if (m_pCommonEventInfo != nullptr
				&& m_pCommonEventInfo->ContentNibble.NibbleCount > 0) {
			pContentNibble = &m_pCommonEventInfo->ContentNibble;
		} else {
			return -1;
		}
		for (int i = 0; i < pContentNibble->NibbleCount; i++) {
			if (pContentNibble->NibbleList[i].ContentNibbleLevel1 != 0xE) {
				const int Nibble =
					Level == 0 ?
						pContentNibble->NibbleList[i].ContentNibbleLevel1 :
						pContentNibble->NibbleList[i].ContentNibbleLevel2;
				if (Nibble <= CEpgGenre::GENRE_LAST)
					return Nibble;
				break;
			}
		}
	}
	return -1;
}


bool CEventItem::SetCommonEvent(const LibISDB::EventInfo *pEvent)
{
	if (m_pEventInfo == nullptr || !m_pEventInfo->IsCommonEvent || pEvent == nullptr)
		return false;
	m_pCommonEventInfo = pEvent;
	/*
	if (m_pEventInfo->EventName.empty())
		m_TitleLines=pItem->m_TitleLines;	// とりあえず
	*/
	return true;
}


int CEventItem::GetTitleText(LPTSTR pszText, int MaxLength, bool fUseARIBSymbol) const
{
	int Length = GetTimeText(pszText, MaxLength);

	if (m_pEventInfo != nullptr && Length + 2 < MaxLength) {
		const LibISDB::String *pEventName;
		if (m_pEventInfo->EventName.empty() && m_pCommonEventInfo != nullptr)
			pEventName = &m_pCommonEventInfo->EventName;
		else
			pEventName = &m_pEventInfo->EventName;
		if (!pEventName->empty()) {
			pszText[Length++] = TEXT(' ');
			if (fUseARIBSymbol)
				Length += static_cast<int>(EpgUtil::MapARIBSymbol(pEventName->c_str(), pszText + Length, MaxLength - Length));
			else
				Length += static_cast<int>(StringFormat(pszText + Length, MaxLength - Length, TEXT("{}"), *pEventName));
		}
	}
	return Length;
}


int CEventItem::GetTimeText(LPTSTR pszText, int MaxLength) const
{
	return EpgUtil::FormatEventTime(m_StartTime, 0, pszText, MaxLength, EpgUtil::FormatEventTimeFlag::StartOnly);
}


String CEventItem::GetEventText(bool fUseARIBSymbol) const
{
	if (m_pEventInfo == nullptr)
		return String();

#if 0
	if (m_pCommonEventInfo != nullptr
			&& ((m_pEventInfo->EventText.empty()
					&& !m_pCommonEventInfo->EventText.empty())
				|| (m_pEventInfo->ExtendedText.empty()
					&& !m_pCommonEventInfo->ExtendedText.empty())))
		return EpgUtil::GetEventDisplayText(*m_pCommonEventInfo, fUseARIBSymbol);
#endif

	return EpgUtil::GetEventDisplayText(*m_pEventInfo, fUseARIBSymbol);
}


void CEventItem::CalcTitleLines(CTextDraw &TextDraw, int Width, bool fUseARIBSymbol)
{
	if (m_TitleLines == 0) {
		TCHAR szText[MAX_TITLE_LENGTH];
		GetTitleText(szText, lengthof(szText), fUseARIBSymbol);
		m_TitleLines = TextDraw.CalcLineCount(szText, Width);
	}
}


void CEventItem::DrawTitle(CTextDraw &TextDraw, const RECT &Rect, int LineHeight, bool fUseARIBSymbol) const
{
	TCHAR szText[MAX_TITLE_LENGTH];

	GetTitleText(szText, lengthof(szText), fUseARIBSymbol);
	TextDraw.Draw(szText, Rect, LineHeight/*, CTextDraw::DRAW_FLAG_JUSTIFY_MULTI_LINE*/);
}


void CEventItem::DrawText(CTextDraw &TextDraw, const RECT &Rect, int LineHeight, CTextDraw::DrawFlag TextDrawFlags, bool fUseARIBSymbol) const
{
	const String Text = GetEventText(fUseARIBSymbol);
	if (!Text.empty())
		TextDraw.Draw(Text.c_str(), Rect, LineHeight, TextDrawFlags);
}


void CEventItem::GetTimeSize(CTextDraw &TextDraw, SIZE *pSize) const
{
	TCHAR szText[32];
	const int Length = GetTimeText(szText, lengthof(szText));
	CTextDraw::TextMetrics Metrics;

	if (TextDraw.GetTextMetrics(szText, Length, &Metrics)) {
		pSize->cx = Metrics.Width;
		pSize->cy = Metrics.Height;
	} else {
		pSize->cx = 0;
		pSize->cy = 0;
	}
}




class CEventLayout
{
	const CServiceInfo *m_pServiceInfo;
	std::vector<std::unique_ptr<CEventItem>> m_EventList;

public:
	CEventLayout(const CServiceInfo *pServiceInfo);
	const CServiceInfo *GetServiceInfo() const { return m_pServiceInfo; }
	void Clear();
	size_t NumItems() const { return m_EventList.size(); }
	void AddItem(CEventItem *pItem) { m_EventList.emplace_back(pItem); }
	bool InsertItem(size_t Index, CEventItem *pItem);
	CEventItem *GetItem(size_t Index);
	const CEventItem *GetItem(size_t Index) const;
	int FindItemByEventID(WORD EventID) const;
	void InsertNullItems(const LibISDB::DateTime &FirstTime, const LibISDB::DateTime &LastTime);
	int GetMaxItemColumnCount() const;
};


CEventLayout::CEventLayout(const CServiceInfo *pServiceInfo)
	: m_pServiceInfo(pServiceInfo)
{
}


void CEventLayout::Clear()
{
	m_EventList.clear();
}


bool CEventLayout::InsertItem(size_t Index, CEventItem *pItem)
{
	if (Index > m_EventList.size())
		return false;
	m_EventList.emplace(m_EventList.begin() + Index, pItem);
	return true;
}


CEventItem *CEventLayout::GetItem(size_t Index)
{
	if (Index >= m_EventList.size())
		return nullptr;
	return m_EventList[Index].get();
}


const CEventItem *CEventLayout::GetItem(size_t Index) const
{
	if (Index >= m_EventList.size())
		return nullptr;
	return m_EventList[Index].get();
}


int CEventLayout::FindItemByEventID(WORD EventID) const
{
	for (size_t i = 0; i < m_EventList.size(); i++) {
		const CEventItem *pItem = m_EventList[i].get();
		if (pItem->GetEventInfo() != nullptr && pItem->GetEventInfo()->EventID == EventID)
			return static_cast<int>(i);
	}
	return -1;
}


void CEventLayout::InsertNullItems(const LibISDB::DateTime &FirstTime, const LibISDB::DateTime &LastTime)
{
	int FirstItem = -1, LastItem = -1;
	LibISDB::DateTime PrevTime = FirstTime;
	int EmptyCount = 0;

	for (int i = 0; i < static_cast<int>(m_EventList.size()); i++) {
		const CEventItem *pItem = m_EventList[i].get();
		const LibISDB::DateTime StartTime = pItem->GetStartTime();
		const LibISDB::DateTime EndTime = pItem->GetEndTime();
		if (StartTime < LastTime && EndTime > FirstTime) {
			if (FirstItem < 0) {
				FirstItem = i;
				LastItem = i + 1;
			} else if (LastItem < i + 1) {
				LastItem = i + 1;
			}
			if (PrevTime < StartTime)
				EmptyCount++;
		}
		if (EndTime >= LastTime)
			break;
		PrevTime = EndTime;
	}

	if (EmptyCount > 0) {
		CEventItem *pPrevItem = nullptr;
		PrevTime = FirstTime;
		for (int i = FirstItem; i < LastItem; i++) {
			CEventItem *pItem = m_EventList[i].get();
			const LibISDB::DateTime StartTime = pItem->GetStartTime();
			const int Cmp = PrevTime.Compare(StartTime);
			if (Cmp > 0) {
				if (pPrevItem)
					pPrevItem->SetEndTime(StartTime);
			} else if (Cmp < 0) {
				const long long Diff = StartTime.DiffSeconds(PrevTime);

				if (Diff < 60) {
					if (pPrevItem)
						pPrevItem->SetEndTime(StartTime);
				} else {
					InsertItem(i, new CEventItem(PrevTime, static_cast<DWORD>(Diff)));
					i++;
					LastItem++;
				}
			}
			PrevTime = pItem->GetEndTime();
			pPrevItem = pItem;
		}
	}
}


int CEventLayout::GetMaxItemColumnCount() const
{
	int MaxColumns = 0;

	for (auto &pItem : m_EventList) {
		if (!pItem->IsNullItem() && pItem->GetItemLines() > 0
				&& MaxColumns < pItem->GetItemColumns())
			MaxColumns = pItem->GetItemColumns();
	}

	return MaxColumns;
}




void CEventLayoutList::Clear()
{
	m_LayoutList.clear();
}


void CEventLayoutList::Add(CEventLayout *pLayout)
{
	m_LayoutList.emplace_back(pLayout);
}


bool CEventLayoutList::Insert(size_t Index, CEventLayout *pLayout)
{
	if (Index > m_LayoutList.size())
		return false;
	m_LayoutList.emplace(m_LayoutList.begin() + Index, pLayout);
	return true;
}


CEventLayout *CEventLayoutList::operator[](size_t Index)
{
	if (Index >= m_LayoutList.size())
		return nullptr;
	return m_LayoutList[Index].get();
}


const CEventLayout *CEventLayoutList::operator[](size_t Index) const
{
	if (Index >= m_LayoutList.size())
		return nullptr;
	return m_LayoutList[Index].get();
}


void CEventLayoutList::EventLayoutDeleter::operator()(CEventLayout *p) const
{
	delete p;
}




CServiceInfo::CServiceInfo(const CChannelInfo &ChannelInfo, LPCTSTR pszBonDriver, size_t Index)
	: m_ChannelInfo(ChannelInfo, pszBonDriver)
	, m_ServiceInfo(
		ChannelInfo.GetNetworkID(),
		ChannelInfo.GetTransportStreamID(),
		ChannelInfo.GetServiceID())
	, m_Index(Index)
{
}


HBITMAP CServiceInfo::GetStretchedLogo(int Width, int Height)
{
	if (m_hbmLogo == nullptr)
		return nullptr;
	// AlphaBlendでリサイズすると汚いので、予めリサイズした画像を作成しておく
	if (m_StretchedLogo.IsCreated()) {
		if (m_StretchedLogo.GetWidth() != Width || m_StretchedLogo.GetHeight() != Height)
			m_StretchedLogo.Destroy();
	}
	if (!m_StretchedLogo.IsCreated()) {
		const HBITMAP hbm = DrawUtil::ResizeBitmap(m_hbmLogo, Width, Height);
		if (hbm != nullptr)
			m_StretchedLogo.Attach(hbm);
	}
	return m_StretchedLogo.GetHandle();
}


LibISDB::EventInfo *CServiceInfo::GetEvent(int Index)
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_EventList.size()) {
		TRACE(TEXT("CServiceInfo::GetEvent() : Out of range {}\n"), Index);
		return nullptr;
	}
	return m_EventList[Index].get();
}


const LibISDB::EventInfo *CServiceInfo::GetEvent(int Index) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_EventList.size()) {
		TRACE(TEXT("CServiceInfo::GetEvent() const : Out of range {}\n"), Index);
		return nullptr;
	}
	return m_EventList[Index].get();
}


LibISDB::EventInfo *CServiceInfo::GetEventByEventID(WORD EventID)
{
	EventIDMap::iterator itr = m_EventIDMap.find(EventID);
	if (itr == m_EventIDMap.end())
		return nullptr;
	return itr->second;
}


const LibISDB::EventInfo *CServiceInfo::GetEventByEventID(WORD EventID) const
{
	EventIDMap::const_iterator itr = m_EventIDMap.find(EventID);
	if (itr == m_EventIDMap.end())
		return nullptr;
	return itr->second;
}


bool CServiceInfo::AddEvent(LibISDB::EventInfo *pEvent)
{
	m_EventList.emplace_back(pEvent);
	m_EventIDMap[pEvent->EventID] = pEvent;
	return true;
}


void CServiceInfo::ClearEvents()
{
	m_EventList.clear();
	m_EventIDMap.clear();
}


void CServiceInfo::CalcLayout(
	CEventLayout *pEventList, const CServiceList *pServiceList,
	const LibISDB::DateTime &FirstTime, const LibISDB::DateTime &LastTime, int LinesPerHour)
{
	pEventList->Clear();

	int FirstItem = -1, LastItem = -1;
	for (int i = 0; i < static_cast<int>(m_EventList.size()); i++) {
		const LibISDB::EventInfo *pEvent = m_EventList[i].get();
		LibISDB::DateTime StartTime, EndTime;
		pEvent->GetStartTime(&StartTime);
		pEvent->GetEndTime(&EndTime);

		if (StartTime < LastTime && EndTime > FirstTime) {
			if (FirstItem < 0) {
				FirstItem = i;
				LastItem = i + 1;
			} else if (LastItem < i + 1) {
				LastItem = i + 1;
			}

			CEventItem *pItem = new CEventItem(pEvent);
			if (pEvent->IsCommonEvent) {
				const LibISDB::EventInfo *pCommonEvent =
					pServiceList->GetEventByIDs(
						m_ServiceInfo.NetworkID,
						m_ServiceInfo.TransportStreamID,
						pEvent->CommonEvent.ServiceID,
						pEvent->CommonEvent.EventID);
				if (pCommonEvent != nullptr)
					pItem->SetCommonEvent(pCommonEvent);
			}
			pEventList->AddItem(pItem);
		}
		if (EndTime >= LastTime)
			break;
	}
	if (FirstItem < 0)
		return;

	pEventList->InsertNullItems(FirstTime, LastTime);

	const size_t NumItems = pEventList->NumItems();
	LibISDB::DateTime First = FirstTime;
	int ItemPos = 0;

	for (size_t i = 0; i < NumItems;) {
		if (First >= LastTime)
			break;
		LibISDB::DateTime Last = First;
		Last.OffsetHours(1);
		do {
			if (pEventList->GetItem(i)->GetEndTime() > First)
				break;
			i++;
		} while (i < NumItems);
		if (i == NumItems)
			break;
		int ProgramsPerHour = 0;
		do {
			if (pEventList->GetItem(i + ProgramsPerHour)->GetStartTime() >= Last)
				break;
			ProgramsPerHour++;
		} while (i + ProgramsPerHour < NumItems);
		if (ProgramsPerHour > 0) {
			int Lines = LinesPerHour, Offset = 0;

			const LibISDB::DateTime &Start = pEventList->GetItem(i)->GetStartTime();
			if (Start > First) {
				Offset = static_cast<int>(Start.DiffSeconds(First) * LinesPerHour / (60 * 60));
				Lines -= Offset;
			}
			if (Lines > ProgramsPerHour) {
				const LibISDB::DateTime &End = pEventList->GetItem(i + ProgramsPerHour - 1)->GetEndTime();
				if (End < Last) {
					Lines -= static_cast<int>(Last.DiffSeconds(End) * LinesPerHour / (60 * 60));
					if (Lines < ProgramsPerHour)
						Lines = ProgramsPerHour;
				}
			}
			if (ProgramsPerHour == 1) {
				CEventItem *pItem = pEventList->GetItem(i);
				pItem->SetItemLines(pItem->GetItemLines() + Lines);
				if (pItem->GetItemPos() < 0)
					pItem->SetItemPos(ItemPos + Offset);
			} else {
				std::vector<int> ItemLines(ProgramsPerHour);

				for (int j = 0; j < ProgramsPerHour; j++)
					ItemLines[j] = j < Lines ? 1 : 0;
				if (Lines > ProgramsPerHour) {
					int LineCount = ProgramsPerHour;

					do {
						DWORD MaxTime = 0;
						int MaxItem;

						for (int j = 0; j < ProgramsPerHour; j++) {
							const CEventItem *pItem = pEventList->GetItem(i + j);
							LibISDB::DateTime Start = pItem->GetStartTime();
							if (Start < First)
								Start = First;
							LibISDB::DateTime End = pItem->GetEndTime();
							if (End > Last)
								End = Last;
							const DWORD Time = static_cast<DWORD>(End.DiffSeconds(Start) / ItemLines[j]);
							if (Time > MaxTime) {
								MaxTime = Time;
								MaxItem = j;
							}
						}
						if (MaxTime == 0)
							break;
						ItemLines[MaxItem]++;
						LineCount++;
					} while (LineCount < Lines);
				}
				int Pos = ItemPos + Offset;
				for (int j = 0; j < std::min(ProgramsPerHour, Lines); j++) {
					CEventItem *pItem = pEventList->GetItem(i + j);
					if (pItem->GetItemPos() < 0)
						pItem->SetItemPos(Pos);
					pItem->SetItemLines(pItem->GetItemLines() + ItemLines[j]);
					Pos += ItemLines[j];
				}
				i += ProgramsPerHour - 1;
			}
		}
		ItemPos += LinesPerHour;
		First = Last;
	}
}


bool CServiceInfo::SaveiEpgFile(const LibISDB::EventInfo *pEventInfo, LPCTSTR pszFileName, bool fVersion2) const
{
	if (pEventInfo == nullptr)
		return false;

	char szText[2048], szServiceName[64], szEventName[256];
	LibISDB::DateTime StartTime, EndTime;
	DWORD Length, Write;

	const HANDLE hFile = ::CreateFile(
		pszFileName, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;
	if (!IsStringEmpty(GetServiceName())) {
		::WideCharToMultiByte(
			932, 0, GetServiceName(), -1,
			szServiceName, sizeof(szServiceName), nullptr, nullptr);
	} else {
		szServiceName[0] = '\0';
	}
	pEventInfo->GetStartTime(&StartTime);
	pEventInfo->GetEndTime(&EndTime);
	if (!pEventInfo->EventName.empty()) {
		::WideCharToMultiByte(
			932, 0, pEventInfo->EventName.c_str(), -1,
			szEventName, sizeof(szEventName), nullptr, nullptr);
	} else {
		szEventName[0] = '\0';
	}

	if (fVersion2) {
		const char *pszStationFormat;
		switch (GetAppClass().NetworkDefinition.GetNetworkType(m_ServiceInfo.NetworkID)) {
		default:
		case CNetworkDefinition::NetworkType::Terrestrial:
			pszStationFormat = "DFS{:05x}";
			break;
		case CNetworkDefinition::NetworkType::BS:
			pszStationFormat = "BSDT{:03}";
			break;
		case CNetworkDefinition::NetworkType::CS:
			pszStationFormat = "CSDT{:03}";
			break;
		}
		char szStation[16];
		StringFormat(
			szStation,
			pszStationFormat, m_ServiceInfo.ServiceID);
		Length = static_cast<DWORD>(StringFormat(
			szText,
			"Content-type: application/x-tv-program-digital-info; charset=shift_jis\r\n"
			"version: 2\r\n"
			"station: {}\r\n"
			"station-name: {}\r\n"
			"year: {}\r\n"
			"month: {:02}\r\n"
			"date: {:02}\r\n"
			"start: {:02}:{:02}\r\n"
			"end: {:02}:{:02}\r\n"
			"program-title: {}\r\n"
			"program-id: {}\r\n",
			szStation, szServiceName,
			StartTime.Year, StartTime.Month, StartTime.Day,
			StartTime.Hour, StartTime.Minute,
			EndTime.Hour, EndTime.Minute,
			szEventName, pEventInfo->EventID));
	} else {
		Length = static_cast<DWORD>(StringFormat(
			szText,
			"Content-type: application/x-tv-program-info; charset=shift_jis\r\n"
			"version: 1\r\n"
			"station: {}\r\n"
			"year: {}\r\n"
			"month: {:02}\r\n"
			"date: {:02}\r\n"
			"start: {:02}:{:02}\r\n"
			"end: {:02}:{:02}\r\n"
			"program-title: {}\r\n",
			szServiceName,
			StartTime.Year, StartTime.Month, StartTime.Day,
			StartTime.Hour, StartTime.Minute,
			EndTime.Hour, EndTime.Minute,
			szEventName));
	}
	const bool fOK = ::WriteFile(hFile, szText, Length, &Write, nullptr) && Write == Length;
	::FlushFileBuffers(hFile);
	::CloseHandle(hFile);
	return fOK;
}




CServiceInfo *CServiceList::GetItem(size_t Index)
{
	if (Index >= m_ServiceList.size())
		return nullptr;
	return m_ServiceList[Index].get();
}


const CServiceInfo *CServiceList::GetItem(size_t Index) const
{
	if (Index >= m_ServiceList.size())
		return nullptr;
	return m_ServiceList[Index].get();
}


CServiceInfo *CServiceList::GetItemByIDs(WORD NetworkID, WORD TransportStreamID, WORD ServiceID)
{
	const int Index = FindItemByIDs(NetworkID, TransportStreamID, ServiceID);

	if (Index < 0)
		return nullptr;

	return m_ServiceList[Index].get();
}


const CServiceInfo *CServiceList::GetItemByIDs(WORD NetworkID, WORD TransportStreamID, WORD ServiceID) const
{
	const int Index = FindItemByIDs(NetworkID, TransportStreamID, ServiceID);

	if (Index < 0)
		return nullptr;

	return m_ServiceList[Index].get();
}


int CServiceList::FindItemByIDs(WORD NetworkID, WORD TransportStreamID, WORD ServiceID) const
{
	const LibISDB::EPGDatabase::ServiceInfo FindInfo(NetworkID, TransportStreamID, ServiceID);

	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (m_ServiceList[i]->GetServiceInfo() == FindInfo)
			return static_cast<int>(i);
	}

	return -1;
}


LibISDB::EventInfo *CServiceList::GetEventByIDs(WORD NetworkID, WORD TransportStreamID, WORD ServiceID, WORD EventID)
{
	CServiceInfo *pService = GetItemByIDs(NetworkID, TransportStreamID, ServiceID);
	if (pService == nullptr)
		return nullptr;
	return pService->GetEventByEventID(EventID);
}


const LibISDB::EventInfo *CServiceList::GetEventByIDs(WORD NetworkID, WORD TransportStreamID, WORD ServiceID, WORD EventID) const
{
	const CServiceInfo *pService = GetItemByIDs(NetworkID, TransportStreamID, ServiceID);
	if (pService == nullptr)
		return nullptr;
	return pService->GetEventByEventID(EventID);
}


void CServiceList::Add(CServiceInfo *pInfo)
{
	m_ServiceList.emplace_back(pInfo);
}


bool CServiceList::Insert(size_t Index, CServiceInfo *pInfo)
{
	if (Index > m_ServiceList.size())
		return false;
	m_ServiceList.emplace(m_ServiceList.begin() + Index, pInfo);
	return true;
}


void CServiceList::Clear()
{
	m_ServiceList.clear();
}


}	// namespace ProgramGuide




bool CProgramGuideChannelProvider::Update()
{
	return true;
}




CProgramGuideBaseChannelProvider::CProgramGuideBaseChannelProvider(const CTuningSpaceList *pSpaceList, LPCTSTR pszBonDriver)
{
	SetTuningSpaceList(pSpaceList);
	SetBonDriverFileName(pszBonDriver);
}


bool CProgramGuideBaseChannelProvider::GetName(LPTSTR pszName, int MaxName) const
{
	if (pszName == nullptr || MaxName < 1)
		return false;

	size_t Length = m_BonDriverFileName.length();
	if (Length >= static_cast<size_t>(MaxName))
		Length = MaxName - 1;
	if (Length > 0)
		m_BonDriverFileName.copy(pszName, Length);
	pszName[Length] = _T('\0');
	::PathRemoveExtension(pszName);

	return true;
}


size_t CProgramGuideBaseChannelProvider::GetGroupCount() const
{
	int GroupCount = m_TuningSpaceList.NumSpaces();
	if (HasAllChannelGroup())
		GroupCount++;
	return GroupCount;
}


bool CProgramGuideBaseChannelProvider::GetGroupName(size_t Group, LPTSTR pszName, int MaxName) const
{
	if (Group >= GetGroupCount() || pszName == nullptr || MaxName < 1)
		return false;

	if (HasAllChannelGroup()) {
		if (Group == 0) {
			StringCopy(pszName, TEXT("すべてのチャンネル"), MaxName);
			return true;
		}
		Group--;
	}

	const LPCTSTR pszTuningSpaceName = m_TuningSpaceList.GetTuningSpaceName(static_cast<int>(Group));
	if (!IsStringEmpty(pszTuningSpaceName))
		StringCopy(pszName, pszTuningSpaceName, MaxName);
	else
		StringFormat(pszName, MaxName, TEXT("チューニング空間 {}"), Group + 1);

	return true;
}


bool CProgramGuideBaseChannelProvider::GetGroupID(size_t Group, String *pID) const
{
	if (Group >= GetGroupCount() || pID == nullptr)
		return false;

	StringFormat(pID, TEXT("{}"), Group);

	return true;
}


int CProgramGuideBaseChannelProvider::ParseGroupID(LPCTSTR pszID) const
{
	if (IsStringEmpty(pszID))
		return -1;

	const int Group = ::StrToInt(pszID);
	if (Group < 0 || static_cast<size_t>(Group) >= GetGroupCount())
		return -1;
	return Group;
}


size_t CProgramGuideBaseChannelProvider::GetChannelCount(size_t Group) const
{
	if (Group >= GetGroupCount())
		return 0;

	if (HasAllChannelGroup()) {
		if (Group == 0)
			return m_TuningSpaceList.GetAllChannelList()->NumChannels();
		Group--;
	}

	const CTuningSpaceInfo *pSpaceInfo = m_TuningSpaceList.GetTuningSpaceInfo(static_cast<int>(Group));
	if (pSpaceInfo == nullptr)
		return 0;

	return pSpaceInfo->NumChannels();
}


const CChannelInfo *CProgramGuideBaseChannelProvider::GetChannelInfo(size_t Group, size_t Channel) const
{
	if (Group >= GetGroupCount())
		return nullptr;

	if (HasAllChannelGroup()) {
		if (Group == 0)
			return m_TuningSpaceList.GetAllChannelList()->GetChannelInfo(static_cast<int>(Channel));
		Group--;
	}

	const CTuningSpaceInfo *pSpaceInfo = m_TuningSpaceList.GetTuningSpaceInfo(static_cast<int>(Group));
	if (pSpaceInfo == nullptr)
		return nullptr;

	return pSpaceInfo->GetChannelInfo(static_cast<int>(Channel));
}


bool CProgramGuideBaseChannelProvider::GetBonDriver(LPTSTR pszFileName, int MaxLength) const
{
	if (pszFileName == nullptr || MaxLength < 1 || m_BonDriverFileName.empty())
		return false;

	const size_t Length = m_BonDriverFileName.length();
	if (static_cast<size_t>(MaxLength) <= Length)
		return false;
	m_BonDriverFileName.copy(pszFileName, Length);
	pszFileName[Length] = _T('\0');

	return true;
}


bool CProgramGuideBaseChannelProvider::GetBonDriverFileName(size_t Group, size_t Channel, LPTSTR pszFileName, int MaxLength) const
{
	return GetBonDriver(pszFileName, MaxLength);
}


bool CProgramGuideBaseChannelProvider::SetTuningSpaceList(const CTuningSpaceList *pList)
{
	if (pList != nullptr)
		m_TuningSpaceList = *pList;
	else
		m_TuningSpaceList.Clear();

	return true;
}


bool CProgramGuideBaseChannelProvider::SetBonDriverFileName(LPCTSTR pszFileName)
{
	if (!IsStringEmpty(pszFileName))
		m_BonDriverFileName = pszFileName;
	else
		m_BonDriverFileName.clear();

	return true;
}


bool CProgramGuideBaseChannelProvider::HasAllChannelGroup() const
{
	return m_TuningSpaceList.NumSpaces() > 1
		&& m_TuningSpaceList.GetAllChannelList()->NumChannels() > 0;
}




const LPCTSTR CProgramGuide::m_pszWindowClass = APP_NAME TEXT(" Program Guide");
HINSTANCE CProgramGuide::m_hinst = nullptr;


bool CProgramGuide::Initialize(HINSTANCE hinst)
{
	if (m_hinst == nullptr) {
		WNDCLASS wc;

		wc.style = CS_HREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hinst;
		wc.hIcon = nullptr;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = nullptr;
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = m_pszWindowClass;
		if (::RegisterClass(&wc) == 0)
			return false;
		m_hinst = hinst;
	}
	return true;
}


CProgramGuide::CProgramGuide(CEventSearchOptions &EventSearchOptions)
	: m_TextLeftMargin(
		m_Style.EventIconSize.Width +
		m_Style.EventIconMargin.Left + m_Style.EventIconMargin.Right)
	, m_EventInfoPopupManager(&m_EventInfoPopup)
	, m_ProgramSearch(EventSearchOptions)
{
	m_WindowPosition.Left = 0;
	m_WindowPosition.Top = 0;
	m_WindowPosition.Width = 640;
	m_WindowPosition.Height = 480;

	GetDefaultFont(&m_Font);

	m_EventInfoPopup.SetEventHandler(&m_EventInfoPopupHandler);

	static const LPCTSTR SearchTargetList[] = {
		TEXT("この番組表から"),
		TEXT("全ての番組から"),
	};
	m_ProgramSearch.SetSearchTargetList(SearchTargetList, lengthof(SearchTargetList));
}


CProgramGuide::~CProgramGuide()
{
	Destroy();

	if (m_pEventHandler != nullptr)
		m_pEventHandler->m_pProgramGuide = nullptr;
	if (m_pFrame != nullptr)
		m_pFrame->m_pProgramGuide = nullptr;
	if (m_pProgramCustomizer != nullptr)
		m_pProgramCustomizer->m_pProgramGuide = nullptr;
}


void CProgramGuide::SetStyle(const Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);
}


void CProgramGuide::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	m_Style.NormalizeStyle(pStyleManager, pStyleScaling);
}


void CProgramGuide::SetTheme(const Theme::CThemeManager *pThemeManager)
{
	static const struct {
		int From, To;
	} ProgramGuideColorMap[] = {
		{CColorScheme::COLOR_PROGRAMGUIDE_BACK,                 COLOR_BACK},
		{CColorScheme::COLOR_PROGRAMGUIDE_HIGHLIGHTTEXT,        COLOR_HIGHLIGHT_TEXT},
		{CColorScheme::COLOR_PROGRAMGUIDE_HIGHLIGHTTITLE,       COLOR_HIGHLIGHT_TITLE},
		{CColorScheme::COLOR_PROGRAMGUIDE_HIGHLIGHTBACK,        COLOR_HIGHLIGHT_BACK},
		{CColorScheme::COLOR_PROGRAMGUIDE_HIGHLIGHTBORDER,      COLOR_HIGHLIGHT_BORDER},
		{CColorScheme::COLOR_PROGRAMGUIDE_CHANNELTEXT,          COLOR_CHANNELNAMETEXT},
		{CColorScheme::COLOR_PROGRAMGUIDE_CHANNELHIGHLIGHTTEXT, COLOR_CHANNELNAMEHIGHLIGHTTEXT},
		{CColorScheme::COLOR_PROGRAMGUIDE_CURCHANNELTEXT,       COLOR_CURCHANNELNAMETEXT},
		{CColorScheme::COLOR_PROGRAMGUIDE_TIMETEXT,             COLOR_TIMETEXT},
		{CColorScheme::COLOR_PROGRAMGUIDE_TIMELINE,             COLOR_TIMELINE},
		{CColorScheme::COLOR_PROGRAMGUIDE_CURTIMELINE,          COLOR_CURTIMELINE},
	};
	for (const auto &e : ProgramGuideColorMap) {
		m_Theme.ColorList[e.To] = pThemeManager->GetColor(e.From);
	}

	pThemeManager->GetFillStyle(
		Theme::CThemeManager::STYLE_PROGRAMGUIDE_CHANNEL,
		&m_Theme.ChannelNameBackStyle);
	pThemeManager->GetFillStyle(
		Theme::CThemeManager::STYLE_PROGRAMGUIDE_CURCHANNEL,
		&m_Theme.CurChannelNameBackStyle);
	pThemeManager->GetFillStyle(
		Theme::CThemeManager::STYLE_PROGRAMGUIDE_TIMEBAR,
		&m_Theme.TimeBarMarginStyle);
	pThemeManager->GetBackgroundStyle(
		Theme::CThemeManager::STYLE_PROGRAMGUIDE_FEATUREDMARK,
		&m_Theme.FeaturedMarkStyle);
	pThemeManager->GetBackgroundStyle(
		Theme::CThemeManager::STYLE_PROGRAMGUIDE_FAVORITEBUTTON,
		&m_Theme.FavoriteButtonStyle);

	for (int i = 0; i < CProgramGuide::TIME_BAR_BACK_COLORS; i++) {
		pThemeManager->GetFillStyle(
			Theme::CThemeManager::STYLE_PROGRAMGUIDE_TIMEBAR_0_2 + i,
			&m_Theme.TimeBarBackStyle[i]);
	}

	m_EpgTheme.SetTheme(pThemeManager);
	m_EventInfoPopup.SetTheme(pThemeManager);

	if (m_hwnd != nullptr) {
		if (IsDarkThemeSupported()) {
			SetWindowDarkTheme(m_hwnd, IsDarkThemeColor(m_Theme.ColorList[COLOR_BACK]));
		}

		Invalidate();
	}
}


bool CProgramGuide::SetEPGDatabase(LibISDB::EPGDatabase *pEPGDatabase)
{
	TVTEST_ASSERT(m_hwnd == nullptr);
	m_pEPGDatabase = pEPGDatabase;
	return true;
}


void CProgramGuide::Clear()
{
	m_pChannelProvider = nullptr;
	m_ServiceList.Clear();
	m_CurrentChannelProvider = -1;
	m_CurrentChannelGroup = -1;
	m_EventLayoutList.Clear();
	m_CurEventItem.fSelected = false;
	m_ScrollPos.x = 0;
	m_ScrollPos.y = 0;
	m_OldScrollPos = m_ScrollPos;
	m_HitInfo = {};

	if (m_hwnd != nullptr) {
		SetCaption();
		Invalidate();
	}
}


bool CProgramGuide::Refresh()
{
	if (m_hwnd == nullptr)
		return false;

	if (m_pEventHandler != nullptr
			&& !m_pEventHandler->OnRefresh())
		return false;

	return UpdateProgramGuide();
}


bool CProgramGuide::UpdateProgramGuide()
{
	if (m_hwnd != nullptr && m_pChannelProvider != nullptr) {
		const HCURSOR hcurOld = ::SetCursor(::LoadCursor(nullptr, IDC_WAIT));

		ResetHitInfo(true);

		if (m_pFrame != nullptr)
			m_pFrame->SetCaption(TITLE_TEXT TEXT(" - 番組表を作成しています..."));

		SetMessage(TEXT("番組表を作成しています..."));

		if (UpdateList()) {
			CalcLayout();
			SetScrollBar();
			LibISDB::GetCurrentEPGTime(&m_CurTime);
		}

		SetMessage(nullptr, false);
		Invalidate();
		SetCaption();

		if (m_pFrame != nullptr) {
			m_pFrame->OnDateChanged();
			m_pFrame->OnSpaceChanged();
		}

		::SetCursor(hcurOld);
	}

	return true;
}


bool CProgramGuide::UpdateList()
{
	if (m_pEPGDatabase == nullptr
			|| m_pChannelProvider == nullptr)
		return false;

	LibISDB::EPGDatabase::ServiceInfo CurServiceInfo;
	if (m_ListMode == ListMode::Week) {
		const ProgramGuide::CServiceInfo *pCurService = m_ServiceList.GetItem(m_WeekListService);
		if (pCurService != nullptr)
			CurServiceInfo = pCurService->GetServiceInfo();
		m_WeekListService = -1;
	}

	m_ServiceList.Clear();
	for (size_t i = 0; i < m_pChannelProvider->GetChannelCount(m_CurrentChannelGroup); i++) {
		const CChannelInfo *pChannelInfo = m_pChannelProvider->GetChannelInfo(m_CurrentChannelGroup, i);

		if (pChannelInfo == nullptr
				|| !pChannelInfo->IsEnabled()
				|| IsExcludeService(
					pChannelInfo->GetNetworkID(),
					pChannelInfo->GetTransportStreamID(),
					pChannelInfo->GetServiceID()))
			continue;

		ProgramGuide::CServiceInfo *pService = MakeServiceInfo(m_CurrentChannelGroup, i);
		if (pService == nullptr)
			continue;

		if (m_ListMode == ListMode::Week && pService->GetServiceInfo() == CurServiceInfo)
			m_WeekListService = static_cast<int>(m_ServiceList.NumServices());

		m_ServiceList.Add(pService);
	}

	if (m_ListMode == ListMode::Week && m_WeekListService < 0) {
		m_ListMode = ListMode::Services;
		if (m_pFrame != nullptr)
			m_pFrame->OnListModeChanged();
	}

	return true;
}


bool CProgramGuide::UpdateService(ProgramGuide::CServiceInfo *pService)
{
	pService->ClearEvents();

	m_pEPGDatabase->EnumEventsSortedByTime(
		pService->GetNetworkID(),
		pService->GetTSID(),
		pService->GetServiceID(),
		[pService](const LibISDB::EventInfo & Event) -> bool {
			pService->AddEvent(new LibISDB::EventInfo(Event));
			return true;
		});

	return true;
}


void CProgramGuide::UpdateServiceList()
{
	if (m_ListMode != ListMode::Services) {
		m_ListMode = ListMode::Services;
		m_WeekListService = -1;
		if (m_pFrame != nullptr)
			m_pFrame->OnListModeChanged();
	}

	UpdateProgramGuide();

	RestoreTimePos();
}


bool CProgramGuide::RefreshService(uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID)
{
	if (m_pEPGDatabase == nullptr
			|| m_pChannelProvider == nullptr)
		return false;

	TRACE(
		TEXT("CProgramGuide::RefreshService() : NID {:x} / TSID {:x} / SID {:x}\n"),
		NetworkID, TransportStreamID, ServiceID);

	for (size_t i = 0; i < m_pChannelProvider->GetChannelCount(m_CurrentChannelGroup); i++) {
		const CChannelInfo *pChannelInfo = m_pChannelProvider->GetChannelInfo(m_CurrentChannelGroup, i);

		if (pChannelInfo == nullptr
				|| !pChannelInfo->IsEnabled()
				|| IsExcludeService(
					pChannelInfo->GetNetworkID(),
					pChannelInfo->GetTransportStreamID(),
					pChannelInfo->GetServiceID()))
			continue;

		if (pChannelInfo->GetNetworkID() == NetworkID
				&& pChannelInfo->GetTransportStreamID() == TransportStreamID
				&& pChannelInfo->GetServiceID() == ServiceID) {
			int ListIndex = m_ServiceList.FindItemByIDs(NetworkID, TransportStreamID, ServiceID);
			const bool fNewService = ListIndex < 0;
			ProgramGuide::CServiceInfo *pService = MakeServiceInfo(
				m_CurrentChannelGroup, i,
				fNewService ? nullptr : m_ServiceList.GetItem(ListIndex));
			if (pService == nullptr)
				return false;

			if (fNewService) {
				for (ListIndex = 0; static_cast<size_t>(ListIndex) < m_ServiceList.NumServices(); ListIndex++) {
					if (m_ServiceList.GetItem(ListIndex)->GetIndex() > i)
						break;
				}
				m_ServiceList.Insert(ListIndex, pService);

				if (m_ListMode == ListMode::Services) {
					if (m_CurEventItem.fSelected && m_CurEventItem.ListIndex >= ListIndex)
						m_CurEventItem.ListIndex++;
				} else if (m_ListMode == ListMode::Week) {
					if (m_WeekListService >= static_cast<int>(ListIndex))
						m_WeekListService++;
				}
			}

			if (m_ListMode != ListMode::Week || m_WeekListService == ListIndex) {
				ResetHitInfo(true);

				EventSelectInfo Select;
				if (m_CurEventItem.fSelected
						&& (m_ListMode == ListMode::Week || m_CurEventItem.ListIndex == ListIndex)) {
					Select = m_CurEventItem;
					m_CurEventItem.fSelected = false;
				}

				if (m_ListMode == ListMode::Services && !m_fCombineCommonEvents) {
					ProgramGuide::CEventLayout *pLayout;
					if (fNewService) {
						pLayout = new ProgramGuide::CEventLayout(pService);
						m_EventLayoutList.Insert(ListIndex, pLayout);
					} else {
						pLayout = m_EventLayoutList[ListIndex];
					}

					LibISDB::DateTime First, Last;
					GetCurrentTimeRange(&First, &Last);
					pService->CalcLayout(pLayout, &m_ServiceList, First, Last, m_LinesPerHour);
				} else {
					CalcLayout();
				}
				SetScrollBar();

				if (!fNewService && m_ListMode == ListMode::Services && !m_fCombineCommonEvents) {
					RECT rc;
					GetColumnRect(ListIndex, &rc);
					Invalidate(&rc);
				} else {
					Invalidate();
				}

				if (Select.fSelected) {
					const ProgramGuide::CEventLayout *pEventLayout = m_EventLayoutList[Select.ListIndex];
					if (pEventLayout != nullptr) {
						const int EventIndex = pEventLayout->FindItemByEventID(Select.EventID);
						if (EventIndex >= 0)
							SelectEvent(Select.ListIndex, EventIndex);
					}
					if (!m_CurEventItem.fSelected)
						SelectEventByIDs(NetworkID, TransportStreamID, ServiceID, Select.EventID);
				}
			}

			return true;
		}
	}

	return false;
}


ProgramGuide::CServiceInfo * CProgramGuide::MakeServiceInfo(
	size_t GroupIndex, size_t ChannelIndex, ProgramGuide::CServiceInfo *pService)
{
	const CChannelInfo *pChannelInfo = m_pChannelProvider->GetChannelInfo(GroupIndex, ChannelIndex);

	TCHAR szBonDriver[MAX_PATH];
	if (!m_pChannelProvider->GetBonDriverFileName(GroupIndex, ChannelIndex, szBonDriver, lengthof(szBonDriver)))
		szBonDriver[0] = _T('\0');

	if (pService != nullptr)
		pService->ClearEvents();

	m_pEPGDatabase->EnumEventsSortedByTime(
		pChannelInfo->GetNetworkID(),
		pChannelInfo->GetTransportStreamID(),
		pChannelInfo->GetServiceID(),
		[&](const LibISDB::EventInfo &Event) -> bool {
			if (pService == nullptr)
				pService = new ProgramGuide::CServiceInfo(*pChannelInfo, szBonDriver, ChannelIndex);
			pService->AddEvent(new LibISDB::EventInfo(Event));
			return true;
		});

	if (pService == nullptr) {
		if (m_fExcludeNoEventServices)
			return nullptr;
		pService = new ProgramGuide::CServiceInfo(*pChannelInfo, szBonDriver, ChannelIndex);
	} else {
		if (m_fExcludeCommonEventOnlyServices) {
			const int NumEvents = pService->NumEvents();
			int i;
			for (i = 0; i < NumEvents; i++) {
				const LibISDB::EventInfo *pEvent = pService->GetEvent(i);
				if (!pEvent->IsCommonEvent)
					break;
			}
			if (i == NumEvents) {
				delete pService;
				return nullptr;
			}
		}
	}

	const HBITMAP hbmLogo = GetAppClass().LogoManager.GetAssociatedLogoBitmap(
		pService->GetNetworkID(), pService->GetServiceID(), CLogoManager::LOGOTYPE_SMALL);
	if (hbmLogo != nullptr)
		pService->SetLogo(hbmLogo);

	return pService;
}


void CProgramGuide::CalcLayout()
{
	LibISDB::DateTime First, Last;
	GetCurrentTimeRange(&First, &Last);

	m_EventLayoutList.Clear();
	m_CurEventItem.fSelected = false;

	if (m_ListMode == ListMode::Services) {
		for (size_t i = 0; i < m_ServiceList.NumServices(); i++) {
			ProgramGuide::CServiceInfo *pService = m_ServiceList.GetItem(i);
			ProgramGuide::CEventLayout *pLayout = new ProgramGuide::CEventLayout(pService);

			pService->CalcLayout(
				pLayout, &m_ServiceList,
				First, Last, m_LinesPerHour);
			m_EventLayoutList.Add(pLayout);

			if (m_fCombineCommonEvents && i > 0) {
				for (size_t j = 0; j < pLayout->NumItems(); j++) {
					ProgramGuide::CEventItem *pItem = pLayout->GetItem(j);
					if (!pItem->IsNullItem() && pItem->GetItemLines() > 0) {
						const LibISDB::EventInfo *pCommonEventInfo = pItem->GetCommonEventInfo();
						if (pCommonEventInfo != nullptr) {
							for (int k = static_cast<int>(i) - 1; k >= 0; k--) {
								ProgramGuide::CEventLayout *pPrevLayout = m_EventLayoutList[k];
								const ProgramGuide::CServiceInfo *pPrevServiceInfo = pPrevLayout->GetServiceInfo();
								if (pPrevServiceInfo->GetNetworkID() != pCommonEventInfo->NetworkID)
									break;
								if (pPrevServiceInfo->GetServiceID() == pCommonEventInfo->ServiceID) {
									const int ItemIndex = pPrevLayout->FindItemByEventID(pCommonEventInfo->EventID);
									if (ItemIndex >= 0) {
										ProgramGuide::CEventItem *pEvent = pPrevLayout->GetItem(ItemIndex);
										if (pEvent->GetItemPos() == pItem->GetItemPos()
												&& pEvent->GetItemLines() == pItem->GetItemLines()) {
											pItem->SetItemColumns(0);
											pEvent->SetItemColumns(pEvent->GetItemColumns() + 1);
										}
										break;
									}
								} else {
									bool fContinue = false;
									for (size_t ItemIndex = 0; ItemIndex < pPrevLayout->NumItems(); ItemIndex++) {
										const ProgramGuide::CEventItem *pEvent = pPrevLayout->GetItem(ItemIndex);
										if (!pEvent->IsNullItem()) {
											if (pEvent->GetCommonEventInfo() == pCommonEventInfo) {
												if (pEvent->GetItemColumns() == 0)
													fContinue = true;
												break;
											}
										}
									}
									if (!fContinue)
										break;
								}
							}
						}
					}
				}
			}
		}
	} else if (m_ListMode == ListMode::Week) {
		ProgramGuide::CServiceInfo *pCurService = m_ServiceList.GetItem(m_WeekListService);

		if (pCurService != nullptr) {
			for (int i = 0; i < 8; i++) {
				ProgramGuide::CEventLayout *pLayout = new ProgramGuide::CEventLayout(pCurService);

				pCurService->CalcLayout(
					pLayout, &m_ServiceList,
					First, Last, m_LinesPerHour);
				m_EventLayoutList.Add(pLayout);
				First.OffsetDays(1);
				Last.OffsetDays(1);
			}
		}
	}

	SetTooltip();
}


void CProgramGuide::UpdateLayout()
{
	CalcLayout();
	SetScrollBar();
	ResetHitInfo();
	Invalidate();
}


CProgramGuide::EventItemStatus CProgramGuide::GetEventItemStatus(
	const ProgramGuide::CEventItem *pItem, EventItemStatus Mask) const
{
	const LibISDB::EventInfo *pEventInfo = pItem->GetEventInfo();
	const LibISDB::EventInfo *pOrigEventInfo = pEventInfo;
	const bool fCommonEvent = pEventInfo->IsCommonEvent;
	if (fCommonEvent && pItem->GetCommonEventInfo() != nullptr)
		pEventInfo = pItem->GetCommonEventInfo();
	EventItemStatus Status = EventItemStatus::None;

	if (!!(Mask & EventItemStatus::Highlighted)) {
		if (m_ProgramSearch.GetHighlightResult()
				&& m_ProgramSearch.IsHitEvent(pEventInfo))
			Status |= EventItemStatus::Highlighted;
	}

	if (!!(Mask & EventItemStatus::Current)) {
		if (m_CurrentEventID != 0
				&& m_CurrentChannel.ServiceID != LibISDB::SERVICE_ID_INVALID
				&& pOrigEventInfo->NetworkID == m_CurrentChannel.NetworkID
				&& pOrigEventInfo->TransportStreamID == m_CurrentChannel.TransportStreamID
				&& pOrigEventInfo->ServiceID == m_CurrentChannel.ServiceID
				&& pOrigEventInfo->EventID == m_CurrentEventID)
			Status |= EventItemStatus::Current;
	}

	if (!!(Mask & EventItemStatus::Filtered)) {
		const int Genre1 = pItem->GetGenre(0);
		const int Genre2 = pItem->GetGenre(1);
		bool fFilter = false;

		if (!!(m_Filter & FilterFlag::Free)
				&& pEventInfo->FreeCAMode
				&& GetAppClass().NetworkDefinition.IsSatelliteNetworkID(pEventInfo->NetworkID)) {
			fFilter = true;
		} else if (!!(m_Filter & FilterFlag::NewProgram)
				&& (pEventInfo->EventName.empty()
					|| pEventInfo->EventName.find(TEXT("[新]")) == LibISDB::String::npos)) {
			fFilter = true;
		} else if (!!(m_Filter & FilterFlag::Original)
				&& !pEventInfo->EventName.empty()
				&& pEventInfo->EventName.find(TEXT("[再]")) != LibISDB::String::npos) {
			fFilter = true;
		} else if (!!(m_Filter & FilterFlag::Rerun)
				&& (pEventInfo->EventName.empty()
					|| pEventInfo->EventName.find(TEXT("[再]")) == LibISDB::String::npos)) {
			fFilter = true;
		} else if (!!(m_Filter & FilterFlag::NotShopping)
				&& Genre1 == 2 && Genre2 == 4) {
			fFilter = true;
		} else if ((m_Filter & FilterFlag::GenreMask) != FilterFlag::None) {
			if (Genre1 < 0 || (static_cast<unsigned int>(m_Filter) & (static_cast<unsigned int>(FilterFlag::GenreFirst) << Genre1)) == 0)
				fFilter = true;
			// 映画ジャンルのアニメ
			if (!!(m_Filter & FilterFlag::Anime) && Genre1 == 6 && Genre2 == 2)
				fFilter = false;
		}

		if (fFilter)
			Status |= EventItemStatus::Filtered;
	}

	if (!!(Mask & EventItemStatus::Common)) {
		if (fCommonEvent)
			Status |= EventItemStatus::Common;
	}

	return Status;
}


void CProgramGuide::DrawEventBackground(
	ProgramGuide::CEventItem *pItem, HDC hdc, const RECT &Rect,
	Theme::CThemeDraw &ThemeDraw, CTextDraw &TextDraw, int LineHeight, int CurTimePos)
{
	const LibISDB::EventInfo *pEventInfo = pItem->GetEventInfo();
	const LibISDB::EventInfo *pOrigEventInfo = pEventInfo;
	const bool fCommonEvent = pEventInfo->IsCommonEvent;
	if (fCommonEvent && pItem->GetCommonEventInfo() != nullptr)
		pEventInfo = pItem->GetCommonEventInfo();
	const EventItemStatus ItemStatus =
		GetEventItemStatus(
			pItem,
			EventItemStatus::Current |
			EventItemStatus::Highlighted |
			EventItemStatus::Filtered);
	const bool fCurrent = !!(ItemStatus & EventItemStatus::Current);
	const bool fHighlighted = !!(ItemStatus & EventItemStatus::Highlighted);
	const bool fFiltered = !!(ItemStatus & EventItemStatus::Filtered);
	const int Genre1 = pItem->GetGenre(0);
	const int Genre2 = pItem->GetGenre(1);
	COLORREF BackColor = m_EpgTheme.GetGenreColor(Genre1);

	if (!fCurrent) {
		if (fFiltered) {
			BackColor = MixColor(BackColor, m_Theme.ColorList[COLOR_BACK], 96);
		} else if (fCommonEvent) {
			BackColor = MixColor(BackColor, m_Theme.ColorList[COLOR_BACK], 192);
		}
	}

	if (fHighlighted) {
		DrawUtil::FillGradient(
			hdc, &Rect,
			MixColor(m_Theme.ColorList[COLOR_HIGHLIGHT_BACK], BackColor, 128),
			BackColor,
			DrawUtil::FillDirection::Vert);
	} else if (fCurrent) {
		m_EpgTheme.DrawContentBackground(
			hdc, ThemeDraw, Rect, *pEventInfo,
			CEpgTheme::DrawContentBackgroundFlag::Current);
	} else {
		DrawUtil::Fill(hdc, &Rect, BackColor);
	}

	RECT rcLine = Rect;
	rcLine.bottom = rcLine.top + GetHairlineWidth();
	DrawUtil::Fill(
		hdc, &rcLine,
		MixColor(BackColor, RGB(0, 0, 0), pItem->GetStartTime().Minute == 0 ? 192 : 224));

	// 現在時刻の線
	if (((m_ListMode == ListMode::Services && m_Day == DAY_TODAY) || m_ListMode == ListMode::Week)
			&& CurTimePos >= Rect.top && CurTimePos < Rect.bottom) {
		RECT rcCurTime;

		rcCurTime.left = Rect.left;
		rcCurTime.right = Rect.right;
		rcCurTime.top = CurTimePos - m_Style.CurTimeLineWidth / 2;
		rcCurTime.bottom = rcCurTime.top + m_Style.CurTimeLineWidth;
		DrawUtil::Fill(
			hdc, &rcCurTime,
			MixColor(m_Theme.ColorList[COLOR_CURTIMELINE], BackColor, 64));
	}

	RECT rcTitle, rcText;
	TextDraw.SetFont(m_TitleFont.GetHandle());
	pItem->CalcTitleLines(TextDraw, Rect.right - Rect.left, m_fUseARIBSymbol);
	rcTitle = Rect;
	rcTitle.bottom = std::min(Rect.bottom, Rect.top + pItem->GetTitleLines() * LineHeight);
	rcText.left = Rect.left + m_TextLeftMargin;
	rcText.top = rcTitle.bottom;
	rcText.right = Rect.right - m_Style.EventPadding.Right;
	rcText.bottom = Rect.bottom;

	if (m_pProgramCustomizer != nullptr) {
		if (!fCommonEvent) {
			m_pProgramCustomizer->DrawBackground(*pEventInfo, hdc, Rect, rcTitle, rcText, BackColor);
		} else {
			LibISDB::EventInfo Info(*pEventInfo);
			Info.ServiceID = pOrigEventInfo->ServiceID;
			m_pProgramCustomizer->DrawBackground(Info, hdc, Rect, rcTitle, rcText, BackColor);
		}
	}

	if (fHighlighted) {
		RECT rc = Rect;
		Style::Subtract(&rc, m_Style.HighlightBorder);
		DrawUtil::FillBorder(
			hdc, &Rect, &rc, &Rect,
			MixColor(m_Theme.ColorList[COLOR_HIGHLIGHT_BORDER], BackColor, 80));
	}
	if (pItem->IsSelected()) {
		RECT rcOuter = Rect;
		rcOuter.left -= m_Style.SelectedBorder.Left;
		rcOuter.right += m_Style.SelectedBorder.Right;
		RECT rcInner = Rect;
		rcInner.top += m_Style.SelectedBorder.Top;
		rcInner.bottom -= m_Style.SelectedBorder.Bottom;
		DrawUtil::FillBorder(
			hdc, &rcOuter, &rcInner, &rcOuter,
			MixColor(m_Theme.ColorList[COLOR_HIGHLIGHT_BORDER], BackColor, 128));
	} else if (fCurrent) {
		RECT rcOuter = Rect;
		rcOuter.left -= m_Style.SelectedBorder.Left;
		rcOuter.right += m_Style.SelectedBorder.Right;
		DrawUtil::FillBorder(hdc, &rcOuter, &Rect, &rcOuter, m_Theme.ColorList[COLOR_CURTIMELINE]);
	}

	if (m_fShowFeaturedMark
			&& m_FeaturedEventsMatcher.IsMatch(*pEventInfo)) {
		SIZE sz;
		RECT rcMark;
		pItem->GetTimeSize(TextDraw, &sz);
		rcMark.left = rcTitle.left;
		rcMark.top = rcTitle.top + m_Style.EventLeading;
		rcMark.right = rcMark.left + sz.cx;
		rcMark.bottom = rcMark.top + sz.cy;
		Style::Subtract(&rcMark, m_Style.FeaturedMarkMargin);
		ThemeDraw.Draw(m_Theme.FeaturedMarkStyle, rcMark);
	}

	if (rcText.bottom > rcTitle.bottom) {
		const unsigned int ShowIcons =
			CEpgIcons::GetEventIcons(pEventInfo) & m_VisibleEventIcons;
		if (ShowIcons != 0) {
			m_EpgIcons.DrawIcons(
				ShowIcons, hdc,
				Rect.left + m_Style.EventIconMargin.Left,
				rcText.top + m_Style.EventIconMargin.Top,
				m_Style.EventIconSize.Width,
				m_Style.EventIconSize.Height,
				0, m_Style.EventIconSize.Height + m_Style.EventIconMargin.Bottom,
				(!fCurrent && (fCommonEvent || fFiltered)) ? 128 : 255,
				&Rect);
		}
	}
}


void CProgramGuide::DrawEventText(
	ProgramGuide::CEventItem *pItem, HDC hdc, const RECT &Rect,
	Theme::CThemeDraw &ThemeDraw, CTextDraw &TextDraw, int LineHeight)
{
	const EventItemStatus ItemStatus =
		GetEventItemStatus(
			pItem,
			EventItemStatus::Current |
			EventItemStatus::Highlighted |
			EventItemStatus::Filtered);
	COLORREF TitleColor = m_EpgTheme.GetColor(CEpgTheme::COLOR_EVENTNAME);
	COLORREF TextColor = m_EpgTheme.GetColor(CEpgTheme::COLOR_EVENTTEXT);

	if (!!(ItemStatus & EventItemStatus::Highlighted)) {
		TitleColor = m_Theme.ColorList[COLOR_HIGHLIGHT_TITLE];
		TextColor = m_Theme.ColorList[COLOR_HIGHLIGHT_TEXT];
	}

	if (!(ItemStatus & EventItemStatus::Current)) {
		if (!!(ItemStatus & EventItemStatus::Filtered)) {
			TitleColor = MixColor(TitleColor, m_Theme.ColorList[COLOR_BACK], 96);
			TextColor = MixColor(TextColor, m_Theme.ColorList[COLOR_BACK], 96);
		}
	}

	RECT rcTitle, rcText;
	rcTitle = Rect;
	rcTitle.bottom = std::min(Rect.bottom, Rect.top + pItem->GetTitleLines() * LineHeight);
	rcText.left = Rect.left + m_TextLeftMargin;
	rcText.top = rcTitle.bottom;
	rcText.right = Rect.right - m_Style.EventPadding.Right;
	rcText.bottom = Rect.bottom;

	rcTitle.top += m_Style.EventLeading;
	TextDraw.SetFont(m_TitleFont.GetHandle());
	TextDraw.SetTextColor(TitleColor);
	pItem->DrawTitle(TextDraw, rcTitle, LineHeight, m_fUseARIBSymbol);

	if (rcText.bottom > rcTitle.bottom) {
		RECT rc = rcText;
		rc.top += m_Style.EventLeading;
		TextDraw.SetFont(m_ContentFont.GetHandle());
		TextDraw.SetTextColor(TextColor);
		pItem->DrawText(
			TextDraw, rc, LineHeight,
			m_Style.fEventJustify ?
				CTextDraw::DrawFlag::JustifyMultiLine :
				CTextDraw::DrawFlag::None,
			m_fUseARIBSymbol);
	}
}


void CProgramGuide::DrawEventList(
	ProgramGuide::CEventLayout *pLayout,
	HDC hdc, const RECT &ColumnRect, const RECT &PaintRect,
	Theme::CThemeDraw &ThemeDraw, CTextDraw &TextDraw, bool fBackground)
{
	const int LineHeight = GetLineHeight();
	const int CurTimePos = ColumnRect.top + GetCurTimeLinePos();

	const HFONT hfontOld = static_cast<HFONT>(::GetCurrentObject(hdc, OBJ_FONT));
	const COLORREF OldTextColor = ::GetTextColor(hdc);

	if (fBackground)
		m_EpgIcons.BeginDraw(hdc, m_Style.EventIconSize.Width, m_Style.EventIconSize.Height);

	RECT rcItem;
	rcItem.left = ColumnRect.left + m_Style.ColumnMargin;

	for (size_t i = 0; i < pLayout->NumItems(); i++) {
		ProgramGuide::CEventItem *pItem = pLayout->GetItem(i);

		if (!pItem->IsNullItem() && pItem->GetItemLines() > 0 && pItem->GetItemColumns() > 0) {
			rcItem.top = ColumnRect.top + pItem->GetItemPos() * LineHeight;
			if (rcItem.top >= PaintRect.bottom)
				break;
			rcItem.bottom = rcItem.top + pItem->GetItemLines() * LineHeight;
			if (rcItem.bottom <= PaintRect.top)
				continue;

			rcItem.right = ColumnRect.right - m_Style.ColumnMargin;
			if (pItem->GetItemColumns() > 1)
				rcItem.right += (pItem->GetItemColumns() - 1) * m_ColumnWidth;

			if (fBackground)
				DrawEventBackground(pItem, hdc, rcItem, ThemeDraw, TextDraw, LineHeight, CurTimePos);
			else
				DrawEventText(pItem, hdc, rcItem, ThemeDraw, TextDraw, LineHeight);
		}
	}

	if (fBackground)
		m_EpgIcons.EndDraw();

	::SetTextColor(hdc, OldTextColor);
	::SelectObject(hdc, hfontOld);
}


void CProgramGuide::DrawHeaderBackground(Theme::CThemeDraw &ThemeDraw, const RECT &Rect, bool fCur) const
{
	const Theme::FillStyle &Style =
		fCur ? m_Theme.CurChannelNameBackStyle : m_Theme.ChannelNameBackStyle;
	const int LineWidth = GetHairlineWidth();
	RECT rc = Rect;
	rc.left += LineWidth;
	rc.right -= LineWidth;
	ThemeDraw.Draw(Style, rc);

	Theme::FillStyle Border;
	Border.Type = Theme::FillType::Gradient;
	Border.Gradient.Type =
		Style.Type == Theme::FillType::Gradient ?
			Style.Gradient.Type : Theme::GradientType::Normal;
	Border.Gradient.Direction = Theme::GradientDirection::Vert;
	Border.Gradient.Color1.Set(255, 255, 255);
	Border.Gradient.Color2.Set(255, 255, 255);
	rc = Rect;
	rc.right = rc.left + LineWidth;
	ThemeDraw.Draw(Theme::MixStyle(Style, Border, 192), rc);

	Border.Gradient.Color1.Set(0, 0, 0);
	Border.Gradient.Color2.Set(0, 0, 0);
	rc = Rect;
	rc.left = rc.right - LineWidth;
	ThemeDraw.Draw(Theme::MixStyle(Style, Border, 192), rc);
}


void CProgramGuide::DrawServiceHeader(
	ProgramGuide::CServiceInfo *pServiceInfo,
	HDC hdc, const RECT &Rect, Theme::CThemeDraw &ThemeDraw,
	int Chevron, bool fLeftAlign, HitTestPart HotPart)
{
	const bool fCur =
		m_CurrentChannel.ServiceID != LibISDB::SERVICE_ID_INVALID
		&& pServiceInfo->GetNetworkID() == m_CurrentChannel.NetworkID
		&& pServiceInfo->GetTSID() == m_CurrentChannel.TransportStreamID
		&& pServiceInfo->GetServiceID() == m_CurrentChannel.ServiceID;

	DrawHeaderBackground(ThemeDraw, Rect, fCur);

	const HFONT hfontOld = DrawUtil::SelectObject(hdc, m_TitleFont);
	const COLORREF TextColor = m_Theme.ColorList[
		fCur ?
			COLOR_CURCHANNELNAMETEXT :
		HotPart == HitTestPart::ServiceTitle ?
			COLOR_CHANNELNAMEHIGHLIGHTTEXT :
			COLOR_CHANNELNAMETEXT];
	const COLORREF OldTextColor = ::SetTextColor(hdc, TextColor);

	RECT rc = Rect;
	Style::Subtract(&rc, m_Style.HeaderPadding);

	const HBITMAP hbmLogo = pServiceInfo->GetLogo();
	if (hbmLogo != nullptr) {
		const int Height = (rc.bottom - rc.top) - m_Style.HeaderIconMargin.Vert();
		const int LogoHeight = std::min(Height, 24);
		const int LogoWidth = LogoHeight * 16 / 9;
		const HBITMAP hbmStretched = pServiceInfo->GetStretchedLogo(LogoWidth, LogoHeight);
		rc.left += m_Style.HeaderIconMargin.Left;
		DrawUtil::DrawBitmap(
			hdc,
			rc.left,
			rc.top + m_Style.HeaderIconMargin.Top + ((rc.bottom - rc.top) - Height) / 2,
			LogoWidth, LogoHeight,
			hbmStretched != nullptr ? hbmStretched : hbmLogo, nullptr,
			fCur || HotPart == HitTestPart::ServiceTitle ? 255 : 192);
		rc.left += LogoWidth + m_Style.HeaderIconMargin.Right;
	}

	rc.right -= m_Style.HeaderChevronSize.Width + m_Style.HeaderChevronMargin.Right;
	m_Chevron.Draw(
		hdc,
		rc.right,
		rc.top + m_Style.HeaderChevronMargin.Top +
		(((rc.bottom - rc.top) - m_Style.HeaderChevronMargin.Vert()) - m_Style.HeaderChevronSize.Height) / 2,
		m_Style.HeaderChevronSize.Width, m_Style.HeaderChevronSize.Height,
		Chevron, TextColor,
		HotPart == HitTestPart::ServiceChevron ? 255 : 192);
	rc.right -= m_Style.HeaderChevronMargin.Left;

	Style::Subtract(&rc, m_Style.HeaderChannelNameMargin);

	if (m_ListMode == ListMode::Week) {
		RECT rcText = {};
		::DrawText(
			hdc, pServiceInfo->GetServiceName(), -1, &rcText,
			DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);
		// DT_CALCRECT では太字や斜体での文字幅を正しく取得できないため、適当にマージンを取る
		rcText.right += m_FontHeight / 2;
		m_WeekListHeaderTitleWidth = (rc.left - Rect.left) + std::min(rcText.right, rc.right - rc.left);
		if (m_WeekListHeaderTitleWidth < 0)
			m_WeekListHeaderTitleWidth = 0;
	}

	::DrawText(
		hdc, pServiceInfo->GetServiceName(), -1, &rc,
		(fLeftAlign || hbmLogo != nullptr ? DT_LEFT : DT_CENTER) |
		DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);

	::SetTextColor(hdc, OldTextColor);
	::SelectObject(hdc, hfontOld);
}


void CProgramGuide::DrawDayHeader(int Day, HDC hdc, const RECT &Rect, Theme::CThemeDraw &ThemeDraw) const
{
	LibISDB::DateTime Time;
	GetCurrentTimeRange(&Time, nullptr);
	if (Day > 0)
		Time.OffsetDays(Day);

	DrawHeaderBackground(ThemeDraw, Rect, false);

	const HFONT hfontOld = DrawUtil::SelectObject(hdc, m_TitleFont);
	const COLORREF OldTextColor = ::SetTextColor(hdc, m_Theme.ColorList[COLOR_CHANNELNAMETEXT]);
	TCHAR szText[64];
	StringFormat(
		szText, TEXT("{}/{}({})"),
		Time.Month, Time.Day, GetDayOfWeekText(Time.DayOfWeek));
	RECT rc = Rect;
	rc.left += m_Style.HeaderPadding.Left;
	rc.right -= m_Style.HeaderPadding.Right;
	::DrawText(
		hdc, szText, -1, &rc,
		DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
	::SetTextColor(hdc, OldTextColor);
	::SelectObject(hdc, hfontOld);
}


void CProgramGuide::DrawTimeBar(HDC hdc, const RECT &Rect, Theme::CThemeDraw &ThemeDraw, bool fRight)
{
	const int PixelsPerHour = GetLineHeight() * m_LinesPerHour;
	const int CurTimePos = Rect.top + GetCurTimeLinePos();
	const int LineWidth = GetHairlineWidth();
	const HFONT hfontOld = DrawUtil::SelectObject(hdc, m_TimeFont);
	const COLORREF crOldTextColor = ::SetTextColor(hdc, m_Theme.ColorList[COLOR_TIMETEXT]);
	RECT rc;

	rc.left = Rect.left;
	rc.top = Rect.top;
	rc.right = Rect.right;

	LibISDB::DateTime Time;
	GetCurrentTimeRange(&Time, nullptr);

	for (int i = 0; i < m_Hours; i++) {
		LibISDB::DateTime DispTime;

		if (i > 0)
			Time.OffsetHours(1);
		EpgUtil::EpgTimeToDisplayTime(Time, &DispTime);

		rc.bottom = rc.top + PixelsPerHour;
		ThemeDraw.Draw(m_Theme.TimeBarBackStyle[DispTime.Hour / 3], rc);

		RECT rcLine = rc;
		rcLine.bottom = rcLine.top + LineWidth;
		DrawUtil::Fill(hdc, &rcLine, m_Theme.ColorList[COLOR_TIMETEXT]);

		if (((m_ListMode == ListMode::Services && m_Day == DAY_TODAY) || m_ListMode == ListMode::Week)
				&& CurTimePos >= rc.top && CurTimePos < rc.bottom) {
			const HBRUSH hbr = ::CreateSolidBrush(m_Theme.ColorList[COLOR_CURTIMELINE]);
			const HBRUSH hbrOld = SelectBrush(hdc, hbr);
			const HPEN hpenOld = SelectPen(hdc, (HPEN)::GetStockObject(NULL_PEN));
			const int TriangleHeight = m_GDIFontHeight * 2 / 3;
			const int TriangleWidth = TriangleHeight * 8 / 10;
			POINT ptTriangle[3];

			if (fRight) {
				ptTriangle[0].x = rc.left;
				ptTriangle[0].y = CurTimePos;
				ptTriangle[1].x = ptTriangle[0].x + TriangleWidth;
				ptTriangle[1].y = ptTriangle[0].y - TriangleHeight / 2;
				ptTriangle[2].x = ptTriangle[0].x + TriangleWidth;
				ptTriangle[2].y = ptTriangle[1].y + TriangleHeight;
			} else {
				ptTriangle[0].x = rc.right - 1;
				ptTriangle[0].y = CurTimePos;
				ptTriangle[1].x = ptTriangle[0].x - TriangleWidth;
				ptTriangle[1].y = ptTriangle[0].y - TriangleHeight / 2;
				ptTriangle[2].x = ptTriangle[0].x - TriangleWidth;
				ptTriangle[2].y = ptTriangle[1].y + TriangleHeight;
			}
			::Polygon(hdc, ptTriangle, 3);
			::SelectObject(hdc, hbrOld);
			::SelectObject(hdc, hpenOld);
			::DeleteObject(hbr);
		}

		TCHAR szText[64];
		if (m_ListMode == ListMode::Services && (i == 0 || DispTime.Hour % 3 == 0)) {
			StringFormat(
				szText, TEXT("{}/{}({}) {}時"),
				DispTime.Month, DispTime.Day,
				GetDayOfWeekText(DispTime.DayOfWeek),
				DispTime.Hour);
		} else {
			StringFormat(szText, TEXT("{}"), DispTime.Hour);
		}
		::TextOut(
			hdc,
			rc.right - m_Style.TimeBarPadding.Right,
			rc.top + m_Style.TimeBarPadding.Top,
			szText, lstrlen(szText));
		rc.top = rc.bottom;
	}

	if (m_ListMode == ListMode::Services && m_Day < DAY_LAST) {
		// ▼
		RECT rcClient;

		GetClientRect(&rcClient);
		if (rc.top - m_TimeBarWidth < rcClient.bottom) {
			const HBRUSH hbr = ::CreateSolidBrush(m_Theme.ColorList[COLOR_TIMETEXT]);
			const HBRUSH hbrOld = SelectBrush(hdc, hbr);
			const HPEN hpenOld = SelectPen(hdc, (HPEN)::GetStockObject(NULL_PEN));
			const int TriangleWidth = m_GDIFontHeight * 2 / 3;
			const int TriangleHeight = TriangleWidth * 8 / 10;
			POINT ptTriangle[3];

			ptTriangle[0].x = m_TimeBarWidth / 2;
			ptTriangle[0].y = rc.top - (m_TimeBarWidth - TriangleHeight) / 2;
			ptTriangle[1].x = ptTriangle[0].x - TriangleWidth / 2;
			ptTriangle[1].y = ptTriangle[0].y - TriangleHeight;
			ptTriangle[2].x = ptTriangle[0].x + TriangleWidth / 2;
			ptTriangle[2].y = ptTriangle[1].y;
			::Polygon(hdc, ptTriangle, 3);
			for (int i = 0; i < 3; i++)
				ptTriangle[i].x += rcClient.right - m_TimeBarWidth;
			::Polygon(hdc, ptTriangle, 3);
			::SelectObject(hdc, hbrOld);
			::SelectObject(hdc, hpenOld);
			::DeleteObject(hbr);
		}
	}

	::SetTextColor(hdc, crOldTextColor);
	SelectFont(hdc, hfontOld);
}


void CProgramGuide::DrawMessage(HDC hdc, const RECT &ClientRect) const
{
	if (!m_Message.empty()) {
		RECT rc;
		const HFONT hfontOld = DrawUtil::SelectObject(hdc, m_ContentFont);
		::SetRectEmpty(&rc);
		::DrawText(hdc, m_Message.c_str(), -1, &rc, DT_NOPREFIX | DT_CALCRECT);
		::OffsetRect(
			&rc,
			(ClientRect.right - (rc.right - rc.left)) / 2,
			(ClientRect.bottom - (rc.bottom - rc.top)) / 2);
#if 0
		const HGDIOBJ hOldBrush = ::SelectObject(hdc, ::GetSysColorBrush(COLOR_WINDOW));
		const HPEN hpen = ::CreatePen(PS_INSIDEFRAME, 1, ::GetSysColor(COLOR_WINDOWBORDER));
		const HGDIOBJ hOldPen = ::SelectObject(hdc, hpen);
		::RoundRect(hdc, rc.left - 16, rc.top - 8, rc.right + 16, rc.bottom + 8, 16, 16);
		::SelectObject(hdc, hOldPen);
		::DeleteObject(hpen);
		::SelectObject(hdc, hOldBrush);
		const COLORREF OldTextColor = ::SetTextColor(hdc, ::GetSysColor(COLOR_WINDOWTEXT));
#else
		RECT rcBack;
		rcBack.left = rc.left - 24;
		rcBack.top = rc.top - 12;
		rcBack.right = rc.right + 24;
		rcBack.bottom = rc.bottom + 12;
		DrawUtil::FillGradient(
			hdc, &rcBack,
			DrawUtil::RGBA(255, 255, 255, 224),
			DrawUtil::RGBA(255, 255, 255, 255),
			DrawUtil::FillDirection::Vert);
		const HGDIOBJ hOldBrush = ::SelectObject(hdc, ::GetStockObject(NULL_BRUSH));
		const HPEN hpen = ::CreatePen(PS_INSIDEFRAME, 2, RGB(208, 208, 208));
		const HGDIOBJ hOldPen = ::SelectObject(hdc, hpen);
		::Rectangle(hdc, rcBack.left, rcBack.top, rcBack.right, rcBack.bottom);
		::SelectObject(hdc, hOldPen);
		::DeleteObject(hpen);
		::SelectObject(hdc, hOldBrush);
		rcBack.top = rcBack.bottom;
		rcBack.bottom = rcBack.top + 6;
		DrawUtil::FillGradient(
			hdc, &rcBack,
			DrawUtil::RGBA(0, 0, 0, 32),
			DrawUtil::RGBA(0, 0, 0, 0),
			DrawUtil::FillDirection::Vert);
		const COLORREF OldTextColor = ::SetTextColor(hdc, RGB(0, 0, 0));
#endif
		const int OldBkMode = ::SetBkMode(hdc, TRANSPARENT);
		::DrawText(hdc, m_Message.c_str(), -1, &rc, DT_NOPREFIX);
		::SetBkMode(hdc, OldBkMode);
		::SetTextColor(hdc, OldTextColor);
		::SelectObject(hdc, hfontOld);
	}
}


void CProgramGuide::Draw(HDC hdc, const RECT &PaintRect)
{
	RECT rcClient, rcGuide, rc;

	::GetClientRect(m_hwnd, &rcClient);
	GetProgramGuideRect(&rcGuide);

	Theme::CThemeDraw ThemeDraw(BeginThemeDraw(hdc));

	if (::IntersectRect(&rc, &rcGuide, &PaintRect))
		DrawUtil::Fill(hdc, &rc, m_Theme.ColorList[COLOR_BACK]);

	const int OldBkMode = ::SetBkMode(hdc, TRANSPARENT);

	int HeaderHeight = m_HeaderHeight;
	if (m_ListMode == ListMode::Week)
		HeaderHeight += m_HeaderHeight;

	if (m_EventLayoutList.Length() > 0) {
		if (PaintRect.top < HeaderHeight) {
			rc.left = rcClient.left + m_TimeBarWidth;
			rc.top = 0;
			rc.right = rcClient.right - m_TimeBarWidth;
			rc.bottom = HeaderHeight;
			HRGN hrgn = ::CreateRectRgnIndirect(&rc);
			::SelectClipRgn(hdc, hrgn);
			if (m_ListMode == ListMode::Services) {
				rc.left = m_TimeBarWidth - m_ScrollPos.x;
				for (size_t i = 0; i < m_ServiceList.NumServices(); i++) {
					rc.right = rc.left + m_ColumnWidth;
					if (rc.left < PaintRect.right && rc.right > PaintRect.left) {
						DrawServiceHeader(
							m_ServiceList.GetItem(i), hdc, rc, ThemeDraw, 2, false,
							m_HitInfo.ListIndex == static_cast<int>(i) ? m_HitInfo.Part : HitTestPart::None);
					}
					rc.left = rc.right;
				}
			} else if (m_ListMode == ListMode::Week) {
				rc.bottom = m_HeaderHeight;
				DrawServiceHeader(m_ServiceList.GetItem(m_WeekListService), hdc, rc, ThemeDraw, 3, true, m_HitInfo.Part);
				rc.left = m_TimeBarWidth - m_ScrollPos.x;
				rc.top = rc.bottom;
				rc.bottom += m_HeaderHeight;
				for (int i = 0; i < static_cast<int>(m_EventLayoutList.Length()); i++) {
					rc.right = rc.left + m_ColumnWidth;
					if (rc.left < PaintRect.right && rc.right > PaintRect.left)
						DrawDayHeader(i, hdc, rc, ThemeDraw);
					rc.left = rc.right;
				}
			}
			if (rc.left < PaintRect.right) {
				rc.right = PaintRect.right;
				ThemeDraw.Draw(m_Theme.ChannelNameBackStyle, rc);
			}
			::SelectClipRgn(hdc, nullptr);
			::DeleteObject(hrgn);
		}

		rc.top = HeaderHeight - m_ScrollPos.y * GetLineHeight();
		if (rc.top < PaintRect.bottom) {
			HRGN hrgn = ::CreateRectRgnIndirect(&rcGuide);
			::SelectClipRgn(hdc, hrgn);

			CTextDraw TextDraw;
			m_TextDrawClient.InitializeTextDraw(&TextDraw);
			TextDraw.BindDC(hdc, rcClient);

			const int PixelsPerHour = GetLineHeight() * m_LinesPerHour;
			const int CurTimePos = rc.top + GetCurTimeLinePos();
			const int TimeLineWidth = GetHairlineWidth();

			RECT rcLine;
			rcLine.left = std::max(rcGuide.left, PaintRect.left);
			rcLine.right = std::min(rcGuide.right, PaintRect.right);
			if (rcLine.left < rcLine.right) {
				// 1時間毎の線の描画
				for (int j = 0; j < m_Hours; j++) {
					const int y = rc.top + j * PixelsPerHour;
					if (y >= PaintRect.top && y < PaintRect.bottom) {
						rcLine.top = y;
						rcLine.bottom = y + TimeLineWidth;
						DrawUtil::Fill(hdc, &rcLine, m_Theme.ColorList[COLOR_TIMELINE]);
					}
				}

				// 現在時刻の線の描画
				if ((m_ListMode == ListMode::Services && m_Day == DAY_TODAY) || m_ListMode == ListMode::Week) {
					rcLine.top = CurTimePos - m_Style.CurTimeLineWidth / 2;
					rcLine.bottom = rcLine.top + m_Style.CurTimeLineWidth;
					DrawUtil::Fill(hdc, &rcLine, m_Theme.ColorList[COLOR_CURTIMELINE]);
				}
			}

			// 番組背景の描画
			const int XOrigin = m_TimeBarWidth - m_ScrollPos.x;
			rc.left = XOrigin;
			for (size_t i = 0; i < m_EventLayoutList.Length(); i++) {
				ProgramGuide::CEventLayout *pLayout = m_EventLayoutList[i];
				const int ColumnCount = pLayout->GetMaxItemColumnCount();
				rc.right = rc.left + m_ColumnWidth;
				if (ColumnCount > 0
						&& rc.left < PaintRect.right
						&& rc.left + ColumnCount * m_ColumnWidth > PaintRect.left)
					DrawEventList(pLayout, hdc, rc, PaintRect, ThemeDraw, TextDraw, true);
				rc.left = rc.right;
			}

			// 番組テキストの描画
			TextDraw.Begin(
				hdc, rcClient,
				CTextDraw::Flag::EndEllipsis |
				CTextDraw::Flag::JapaneseHyphnation);
			TextDraw.SetClippingRect(rcGuide);

			rc.left = XOrigin;
			for (size_t i = 0; i < m_EventLayoutList.Length(); i++) {
				ProgramGuide::CEventLayout *pLayout = m_EventLayoutList[i];
				const int ColumnCount = pLayout->GetMaxItemColumnCount();
				rc.right = rc.left + m_ColumnWidth;
				if (ColumnCount > 0
						&& rc.left < PaintRect.right
						&& rc.left + ColumnCount * m_ColumnWidth > PaintRect.left)
					DrawEventList(m_EventLayoutList[i], hdc, rc, PaintRect, ThemeDraw, TextDraw, false);
				rc.left = rc.right;
			}

			TextDraw.ResetClipping();
			TextDraw.End();

			::SelectClipRgn(hdc, nullptr);
			::DeleteObject(hrgn);
		}
	} else {
		if (PaintRect.top < m_HeaderHeight) {
			rc.left = std::max(PaintRect.left, static_cast<LONG>(m_TimeBarWidth));
			rc.right = std::min(PaintRect.right, rcClient.right - m_TimeBarWidth);
			if (rc.left < rc.right) {
				rc.top = 0;
				rc.bottom = m_HeaderHeight;
				ThemeDraw.Draw(m_Theme.ChannelNameBackStyle, rc);
			}
		}
	}

	rc.left = 0;
	rc.top = HeaderHeight;
	rc.right = rcClient.right;
	rc.bottom = rcClient.bottom;
	HRGN hrgn = ::CreateRectRgnIndirect(&rc);
	::SelectClipRgn(hdc, hrgn);
	rc.top = HeaderHeight - m_ScrollPos.y * GetLineHeight();
	rc.bottom = rc.top + GetLineHeight() * m_LinesPerHour * m_Hours;
	if (PaintRect.left < m_TimeBarWidth) {
		rc.left = 0;
		rc.right = m_TimeBarWidth;
		DrawTimeBar(hdc, rc, ThemeDraw, false);
	}
	rc.left = rcClient.right - m_TimeBarWidth;
	if (rc.left < PaintRect.right) {
		rc.right = rcClient.right;
		DrawTimeBar(hdc, rc, ThemeDraw, true);
	}
	::SelectClipRgn(hdc, nullptr);
	::DeleteObject(hrgn);

	if (rc.bottom < PaintRect.bottom) {
		::SetRect(&rc, 0, rc.bottom, m_TimeBarWidth, rcClient.bottom);
		ThemeDraw.Draw(m_Theme.TimeBarMarginStyle, rc);
		::OffsetRect(&rc, rcGuide.right, 0);
		ThemeDraw.Draw(m_Theme.TimeBarMarginStyle, rc);
	}
	if (PaintRect.top < HeaderHeight) {
		::SetRect(&rc, 0, 0, m_TimeBarWidth, HeaderHeight);
		ThemeDraw.Draw(m_Theme.TimeBarMarginStyle, rc);
		::OffsetRect(&rc, rcGuide.right, 0);
		ThemeDraw.Draw(m_Theme.TimeBarMarginStyle, rc);
	}

	if (m_ListMode == ListMode::Services && m_Day != DAY_TODAY
			&& PaintRect.top < m_HeaderHeight) {
		// ▲
		const HBRUSH hbr = ::CreateSolidBrush(m_Theme.ColorList[COLOR_TIMETEXT]);
		const HBRUSH hbrOld = SelectBrush(hdc, hbr);
		const HPEN hpenOld = SelectPen(hdc, (HPEN)::GetStockObject(NULL_PEN));
		const int TriangleWidth = m_GDIFontHeight * 2 / 3;
		const int TriangleHeight = TriangleWidth * 8 / 10;
		POINT ptTriangle[3];

		ptTriangle[0].x = m_TimeBarWidth / 2;
		ptTriangle[0].y = (m_HeaderHeight - TriangleHeight) / 2;
		ptTriangle[1].x = ptTriangle[0].x - TriangleWidth / 2;
		ptTriangle[1].y = ptTriangle[0].y + TriangleHeight;
		ptTriangle[2].x = ptTriangle[0].x + TriangleWidth / 2;
		ptTriangle[2].y = ptTriangle[1].y;
		::Polygon(hdc, ptTriangle, 3);
		for (int i = 0; i < 3; i++)
			ptTriangle[i].x += rcClient.right - m_TimeBarWidth;
		::Polygon(hdc, ptTriangle, 3);
		::SelectObject(hdc, hbrOld);
		::SelectObject(hdc, hpenOld);
		::DeleteObject(hbr);
	}

	if (m_fBarShadow) {
		if (m_Style.HeaderShadowHeight > 0) {
			rc.left = rcGuide.left;
			rc.top = rcGuide.top;
			rc.right = rcGuide.right;
			//rc.bottom = std::min(rc.top + m_Style.HeaderShadowHeight, rcGuide.bottom);
			rc.bottom = rc.top + m_Style.HeaderShadowHeight;
			DrawUtil::FillGradient(
				hdc, &rc, DrawUtil::RGBA(0, 0, 0, 80), DrawUtil::RGBA(0, 0, 0, 0),
				DrawUtil::FillDirection::Vert);
		}

		if (m_Style.TimeBarShadowWidth > 0) {
			rc.top = rcGuide.top;
			rc.bottom = rcGuide.bottom;
			rc.left = rcGuide.left;
			rc.right = std::min(rc.left + m_Style.TimeBarShadowWidth, rcGuide.right);
			DrawUtil::FillGradient(
				hdc, &rc, DrawUtil::RGBA(0, 0, 0, 64), DrawUtil::RGBA(0, 0, 0, 0),
				DrawUtil::FillDirection::Horz);
			rc.right = rcGuide.right;
			rc.left = std::max(rc.right - m_Style.TimeBarShadowWidth, rcGuide.left);
			DrawUtil::FillGradient(
				hdc, &rc, DrawUtil::RGBA(0, 0, 0, 0), DrawUtil::RGBA(0, 0, 0, 48),
				DrawUtil::FillDirection::Horz);
		}
	}

	DrawMessage(hdc, rcClient);

	::SetBkMode(hdc, OldBkMode);
}


bool CProgramGuide::CreateFonts()
{
	if (!CreateDrawFont(m_Font, &m_ContentFont))
		return false;

	LOGFONT lf;
	m_ContentFont.GetLogFont(&lf);
	lf.lfWeight = FW_BOLD;
	m_TitleFont.Create(&lf);
	lf.lfWeight = FW_NORMAL;
	lf.lfEscapement = lf.lfOrientation = 2700;
	m_TimeFont.Create(&lf);

	return true;
}


void CProgramGuide::CalcFontMetrics()
{
	if (m_hwnd != nullptr) {
		const HDC hdc = ::GetDC(m_hwnd);

		m_GDIFontHeight = m_ContentFont.GetHeight(hdc);

		{
			CTextDraw TextDraw;
			CTextDraw::FontMetrics FontMetrics;
			RECT rc;

			m_TextDrawClient.InitializeTextDraw(&TextDraw);
			GetClientRect(&rc);
			TextDraw.BindDC(hdc, rc);
			TextDraw.SetFont(m_ContentFont.GetHandle());
			if (TextDraw.GetFontMetrics(&FontMetrics))
				m_FontHeight = FontMetrics.Height;
			else
				m_FontHeight = m_GDIFontHeight;
		}

		::ReleaseDC(m_hwnd, hdc);

		m_HeaderHeight = CalcHeaderHeight();
		m_TimeBarWidth = m_GDIFontHeight + m_Style.TimeBarPadding.Horz();
	}
}


int CProgramGuide::GetLineHeight() const
{
	return m_FontHeight + m_Style.EventLeading + m_Style.EventLineSpacing;
}


int CProgramGuide::CalcHeaderHeight() const
{
	const int NameHeight = m_GDIFontHeight + m_Style.HeaderChannelNameMargin.Vert();
	const int ChevronHeight = m_Style.HeaderChevronSize.Height + m_Style.HeaderChevronMargin.Vert();

	return std::max(NameHeight, ChevronHeight) + m_Style.HeaderPadding.Vert();
}


int CProgramGuide::GetCurTimeLinePos() const
{
	LibISDB::DateTime First;

	GetCurrentTimeRange(&First, nullptr);
	LONGLONG Span = m_CurTime.DiffSeconds(First) % (24 * 60 * 60);
	if (Span < 0)
		Span += 24 * 60 * 60;
	return static_cast<int>(Span * static_cast<LONGLONG>(GetLineHeight() * m_LinesPerHour) / (60 * 60));
}


void CProgramGuide::GetProgramGuideRect(RECT *pRect) const
{
	GetClientRect(pRect);
	pRect->left += m_TimeBarWidth;
	pRect->right -= m_TimeBarWidth;
	pRect->top += m_HeaderHeight;
	if (m_ListMode == ListMode::Week)
		pRect->top += m_HeaderHeight;
}


void CProgramGuide::GetProgramGuideSize(SIZE *pSize) const
{
	pSize->cx = m_ColumnWidth * static_cast<int>(m_EventLayoutList.Length());
	pSize->cy = m_LinesPerHour * m_Hours;
}


void CProgramGuide::GetPageSize(SIZE *pSize) const
{
	RECT rc;

	GetProgramGuideRect(&rc);
	pSize->cx = std::max(rc.right - rc.left, 0L);
	pSize->cy = std::max(rc.bottom - rc.top, 0L) / GetLineHeight();
}


void CProgramGuide::GetColumnRect(int Index, RECT *pRect) const
{
	GetProgramGuideRect(pRect);

	pRect->left += Index * m_ColumnWidth + m_Style.ColumnMargin - m_ScrollPos.x;
	pRect->right = pRect->left + m_ItemWidth;
}


void CProgramGuide::Scroll(int XScroll, int YScroll)
{
	POINT Pos = m_ScrollPos;
	RECT rcGuide;
	SIZE GuideSize, PageSize;
	SCROLLINFO si;
	int XScrollSize = 0, YScrollSize = 0;

	GetProgramGuideRect(&rcGuide);
	GetProgramGuideSize(&GuideSize);
	GetPageSize(&PageSize);
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_POS;
	if (XScroll != 0) {
		Pos.x = m_ScrollPos.x + XScroll;
		if (Pos.x < 0)
			Pos.x = 0;
		else if (Pos.x > std::max(GuideSize.cx - PageSize.cx, 0L))
			Pos.x = std::max(GuideSize.cx - PageSize.cx, 0L);
		si.nPos = Pos.x;
		::SetScrollInfo(m_hwnd, SB_HORZ, &si, TRUE);
		XScrollSize = m_ScrollPos.x - Pos.x;
	}
	if (YScroll != 0) {
		Pos.y = m_ScrollPos.y + YScroll;
		if (Pos.y < 0)
			Pos.y = 0;
		else if (Pos.y > std::max(GuideSize.cy - PageSize.cy, 0L))
			Pos.y = std::max(GuideSize.cy - PageSize.cy, 0L);
		si.nPos = Pos.y;
		::SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
		YScrollSize = (m_ScrollPos.y - Pos.y) * GetLineHeight();
	}

	m_ScrollPos = Pos;

	if (!CBufferedPaint::IsSupported() && m_Message.empty()) {
		RECT rcClip = rcGuide;
		if (m_fBarShadow) {
			rcClip.top += m_Style.HeaderShadowHeight;
			rcClip.left += m_Style.TimeBarShadowWidth;
			rcClip.right -= m_Style.TimeBarShadowWidth;
		}
		if (rcClip.right > rcClip.left && rcClip.bottom > rcClip.top
				&& std::abs(YScrollSize) < rcClip.bottom - rcClip.top
				&& std::abs(XScrollSize) < rcClip.right - rcClip.left) {
			::ScrollWindowEx(
				m_hwnd, XScrollSize, YScrollSize, &rcGuide, &rcClip,
				nullptr, nullptr, SW_INVALIDATE);

			if (m_fBarShadow) {
				RECT rc;
				rc.left = rcGuide.left;
				rc.top = rcGuide.top;
				rc.right = rcGuide.right;
				rc.bottom = rcClip.top;
				::InvalidateRect(m_hwnd, &rc, FALSE);
				rc.top = rcGuide.top;
				rc.bottom = rcGuide.bottom;
				rc.left = rcGuide.left;
				rc.right = rcClip.left;
				::InvalidateRect(m_hwnd, &rc, FALSE);
				rc.left = rcClip.right;
				rc.right = rcGuide.right;
				::InvalidateRect(m_hwnd, &rc, FALSE);
			}

			if (XScrollSize != 0) {
				RECT rcHeader;

				::SetRect(&rcHeader, rcGuide.left, 0, rcGuide.right, rcGuide.top);
				if (m_ListMode == ListMode::Week)
					rcHeader.top += m_HeaderHeight;
				::ScrollWindowEx(m_hwnd, XScrollSize, 0, &rcHeader, &rcHeader, nullptr, nullptr, SW_INVALIDATE);
			}

			if (YScrollSize != 0) {
				RECT rcTime;

				::SetRect(&rcTime, 0, rcGuide.top, rcGuide.left, rcGuide.bottom);
				::ScrollWindowEx(m_hwnd, 0, YScrollSize, &rcTime, &rcTime, nullptr, nullptr, SW_INVALIDATE);
				rcTime.left = rcGuide.right;
				rcTime.right = rcTime.left + m_TimeBarWidth;
				::ScrollWindowEx(m_hwnd, 0, YScrollSize, &rcTime, &rcTime, nullptr, nullptr, SW_INVALIDATE);
			}
		} else {
			Invalidate();
		}
	} else {
		Invalidate();
	}

	SetTooltip();
}


void CProgramGuide::SetScrollPos(const POINT &Pos)
{
	if (Pos.x != m_ScrollPos.x || Pos.y != m_ScrollPos.y)
		Scroll(Pos.x - m_ScrollPos.x, Pos.y - m_ScrollPos.y);
}


void CProgramGuide::SetScrollBar()
{
	SCROLLINFO si;
	RECT rc;

	GetProgramGuideRect(&rc);
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS | SIF_DISABLENOSCROLL;
	si.nMin = 0;
	si.nMax = m_Hours * m_LinesPerHour - 1;
	si.nPage = (rc.bottom - rc.top) / GetLineHeight();
	si.nPos = m_ScrollPos.y;
	::SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
	si.nMax = static_cast<int>(m_EventLayoutList.Length()) * m_ColumnWidth - 1;
	si.nPage = rc.right - rc.left;
	si.nPos = m_ScrollPos.x;
	::SetScrollInfo(m_hwnd, SB_HORZ, &si, TRUE);
}


CProgramGuide::HitTestInfo CProgramGuide::HitTest(int x, int y) const
{
	HitTestInfo Info;

	RECT rc;
	::GetClientRect(m_hwnd, &rc);

	if (y >= 0 && y < (m_ListMode == ListMode::Week ? m_HeaderHeight * 2 : m_HeaderHeight)
			&& x >= m_TimeBarWidth && x < rc.right - m_TimeBarWidth) {
		const int ChevronArea =
			m_Style.HeaderChevronSize.Width +
			m_Style.HeaderChevronMargin.Right + m_Style.HeaderPadding.Right;

		if (m_ListMode == ListMode::Services) {
			x += m_ScrollPos.x - m_TimeBarWidth;
			if (x < static_cast<int>(m_EventLayoutList.Length()) * m_ColumnWidth) {
				const int Service = x / m_ColumnWidth;

				RECT HeaderRect;
				HeaderRect.left = m_TimeBarWidth + Service * m_ColumnWidth - m_ScrollPos.x;
				HeaderRect.right = HeaderRect.left + m_ColumnWidth;
				HeaderRect.top = 0;
				HeaderRect.bottom = m_HeaderHeight;

				if (x % m_ColumnWidth < m_ColumnWidth - (m_Style.HeaderChevronMargin.Left + ChevronArea)) {
					Info.Part = HitTestPart::ServiceTitle;
					Info.ListIndex = Service;
					Info.PartRect = HeaderRect;
					Info.PartRect.right -= m_Style.HeaderChevronMargin.Left + ChevronArea;
					Info.fHotTracking = true;
					Info.fClickable = true;
				} else if (x % m_ColumnWidth >= m_ColumnWidth - ChevronArea) {
					Info.Part = HitTestPart::ServiceChevron;
					Info.ListIndex = Service;
					Info.PartRect = HeaderRect;
					Info.PartRect.left = Info.PartRect.right - ChevronArea;
					Info.fHotTracking = true;
					Info.fClickable = true;
				} else {
					Info.Part = HitTestPart::ServiceHeader;
					Info.ListIndex = Service;
					Info.PartRect = HeaderRect;
				}
			}
		} else if (m_ListMode == ListMode::Week) {
			const RECT HeaderRect = {m_TimeBarWidth, 0, rc.right - m_TimeBarWidth, m_HeaderHeight};

			if (x >= HeaderRect.right - ChevronArea && y < m_HeaderHeight) {
				Info.Part = HitTestPart::ServiceChevron;
				Info.PartRect = HeaderRect;
				Info.PartRect.left = HeaderRect.right - ChevronArea;
				Info.fHotTracking = true;
				Info.fClickable = true;
			} else if (x < HeaderRect.left + m_WeekListHeaderTitleWidth && y < m_HeaderHeight) {
				Info.Part = HitTestPart::ServiceTitle;
				Info.PartRect = HeaderRect;
				Info.PartRect.right = HeaderRect.left + m_WeekListHeaderTitleWidth;
				Info.fHotTracking = true;
				Info.fClickable = true;
			} else {
				Info.Part = HitTestPart::ServiceHeader;
				Info.PartRect = HeaderRect;
				Info.PartRect.bottom += m_HeaderHeight;
			}
		}
	} else if ((x >= 0 && x < m_TimeBarWidth) || (x < rc.right && x >= rc.right - m_TimeBarWidth)) {
		RECT TimeBarRect;
		TimeBarRect.left = x < m_TimeBarWidth ? 0L : rc.right - m_TimeBarWidth;
		TimeBarRect.right = TimeBarRect.left + m_TimeBarWidth;
		TimeBarRect.top = 0;
		TimeBarRect.bottom = rc.bottom;

		if (m_ListMode == ListMode::Services) {
			if (m_Day > DAY_FIRST && y >= 0 && y < m_HeaderHeight) {
				Info.Part = HitTestPart::TimeBarPrev;
				Info.PartRect = TimeBarRect;
				Info.PartRect.bottom = m_HeaderHeight;
				Info.fClickable = true;
			} else if (m_Day < DAY_LAST) {
				const int Bottom = (m_Hours * m_LinesPerHour - m_ScrollPos.y) * GetLineHeight();
				if (y - m_HeaderHeight >= Bottom - m_TimeBarWidth && y - m_HeaderHeight < Bottom) {
					Info.Part = HitTestPart::TimeBarNext;
					Info.PartRect = TimeBarRect;
					Info.PartRect.top = Bottom - m_TimeBarWidth;
					Info.PartRect.bottom = Bottom;
					Info.fClickable = true;
				}
			}
		}

		if (Info.Part == HitTestPart::None) {
			Info.Part = HitTestPart::TimeBar;
			Info.PartRect = TimeBarRect;
		}
	}

	return Info;
}


bool CProgramGuide::SetHitInfo(const HitTestInfo &Info, bool fUpdate)
{
	if (m_HitInfo.Part == Info.Part && m_HitInfo.ListIndex == Info.ListIndex)
		return false;

	if (m_HitInfo.Part != HitTestPart::None && m_HitInfo.fHotTracking)
		Invalidate(&m_HitInfo.PartRect);

	m_HitInfo = Info;

	if (m_HitInfo.Part != HitTestPart::None && m_HitInfo.fHotTracking)
		Invalidate(&m_HitInfo.PartRect);

	if (fUpdate)
		Update();

	return true;
}


void CProgramGuide::ResetHitInfo(bool fUpdate)
{
	if (m_HitInfo.Part != HitTestPart::None)
		SetHitInfo({}, fUpdate);
}


int CProgramGuide::GetTimePos() const
{
	LibISDB::DateTime Begin;

	GetCurrentTimeRange(&Begin, nullptr);
	return Begin.Hour * m_LinesPerHour + m_ScrollPos.y;
}


bool CProgramGuide::SetTimePos(int Pos)
{
	LibISDB::DateTime Begin, End;
	GetCurrentTimeRange(&Begin, &End);

	LibISDB::DateTime Time = Begin;
	Time.OffsetMinutes(Pos * 60 / m_LinesPerHour - Begin.Hour * 60);
	if (Time < Begin)
		Time.OffsetDays(1);
	else if (Time >= End)
		Time.OffsetDays(-1);
	return ScrollToTime(Time);
}


void CProgramGuide::StoreTimePos()
{
	m_CurTimePos = GetTimePos();
}


void CProgramGuide::RestoreTimePos()
{
	if (m_fKeepTimePos)
		SetTimePos(m_CurTimePos);
}


void CProgramGuide::SetCaption()
{
	if (m_hwnd != nullptr && m_pFrame != nullptr) {
		if (m_pEPGDatabase != nullptr) {
			if (m_fEpgUpdating) {
				TCHAR szText[256];

				StringFormat(
					szText,
					TITLE_TEXT TEXT(" - 番組表の取得中... [{}/{}] 残り約{}分"),
					m_EpgUpdateProgress.Pos + 1, m_EpgUpdateProgress.End,
					(m_EpgUpdateProgress.RemainingTime + 59999) / 60000);
				m_pFrame->SetCaption(szText);
			} else {
				DateInfo Info;
				TCHAR szText[256];

				GetCurrentDateInfo(&Info);
				if (m_ListMode == ListMode::Services) {
					Info.EndTime.OffsetHours(-1);
					StringFormat(
						szText,
						TITLE_TEXT TEXT(" - {}{}{}/{}({}) {}時 ～ {}/{}({}) {}時"),
						Info.pszRelativeDayText != nullptr ? Info.pszRelativeDayText : TEXT(""),
						Info.pszRelativeDayText != nullptr ? TEXT(" ") : TEXT(""),
						Info.BeginningTime.Month,
						Info.BeginningTime.Day,
						GetDayOfWeekText(Info.BeginningTime.DayOfWeek),
						Info.BeginningTime.Hour,
						Info.EndTime.Month,
						Info.EndTime.Day,
						GetDayOfWeekText(Info.EndTime.DayOfWeek),
						Info.EndTime.Hour);
				} else {
					LibISDB::DateTime Last = Info.BeginningTime;
					Last.OffsetDays(6);
					StringFormat(
						szText,
						TITLE_TEXT TEXT(" - {} {}/{}({}) ～ {}/{}({})"),
						m_ServiceList.GetItem(m_WeekListService)->GetServiceName(),
						Info.BeginningTime.Month,
						Info.BeginningTime.Day,
						GetDayOfWeekText(Info.BeginningTime.DayOfWeek),
						Last.Month,
						Last.Day,
						GetDayOfWeekText(Last.DayOfWeek));
				}
				m_pFrame->SetCaption(szText);
			}
		} else {
			m_pFrame->SetCaption(TITLE_TEXT);
		}
	}
}


void CProgramGuide::SetTooltip()
{
	if (m_ListMode == ListMode::Services) {
		int NumTools = m_Tooltip.NumTools();
		const int NumServices = static_cast<int>(m_ServiceList.NumServices());

		RECT rcClient, rcHeader;
		GetClientRect(&rcClient);
		rcHeader.left = m_TimeBarWidth;
		rcHeader.right = rcClient.right - m_TimeBarWidth;
		rcHeader.top = 0;
		rcHeader.bottom = m_HeaderHeight;

		RECT rc;
		rc.left = m_TimeBarWidth - m_ScrollPos.x;
		rc.top = 0;
		rc.bottom = m_HeaderHeight;
		int ToolCount = 0;
		for (int i = 0; i < NumServices; i++) {
			rc.right = rc.left + m_ColumnWidth;
			rc.left = rc.right - (m_Style.HeaderPadding.Right + m_Style.HeaderChevronMargin.Right + m_Style.HeaderChevronSize.Width);
			if (rc.left >= rcHeader.right)
				break;
			if (rc.right > rcHeader.left) {
				RECT rcTool;

				::IntersectRect(&rcTool, &rc, &rcHeader);
				if (ToolCount < NumTools) {
					m_Tooltip.SetText(ToolCount, TEXT("1週間表示"));
					m_Tooltip.SetToolRect(ToolCount, rcTool);
				} else {
					m_Tooltip.AddTool(ToolCount, rcTool, TEXT("1週間表示"));
				}
				ToolCount++;
			}
			rc.left = rc.right;
		}

		while (NumTools > ToolCount) {
			m_Tooltip.DeleteTool(--NumTools);
		}

		if (m_Day > DAY_TODAY) {
			rc.top = 0;
			rc.bottom = m_HeaderHeight;
			rc.left = 0;
			rc.right = m_TimeBarWidth;
			m_Tooltip.AddTool(ToolCount++, rc, TEXT("一日前へ"));
			rc.left = rcClient.right - m_TimeBarWidth;
			rc.right = rcClient.right;
			m_Tooltip.AddTool(ToolCount++, rc, TEXT("一日前へ"));
		}
		if (m_Day < DAY_LAST) {
			const int y = m_HeaderHeight + (m_Hours * m_LinesPerHour - m_ScrollPos.y) * GetLineHeight();
			rc.top = y - m_TimeBarWidth;
			rc.bottom = y;
			rc.left = 0;
			rc.right = m_TimeBarWidth;
			m_Tooltip.AddTool(ToolCount++, rc, TEXT("一日後へ"));
			rc.left = rcClient.right - m_TimeBarWidth;
			rc.right = rcClient.right;
			m_Tooltip.AddTool(ToolCount, rc, TEXT("一日後へ"));
		}
	} else if (m_ListMode == ListMode::Week) {
		m_Tooltip.DeleteAllTools();
		RECT rc;
		GetClientRect(&rc);
		rc.right -= m_TimeBarWidth;
		rc.left = rc.right - (m_Style.HeaderPadding.Right + m_Style.HeaderChevronMargin.Right + m_Style.HeaderChevronSize.Width);
		if (rc.left < m_TimeBarWidth)
			rc.left = m_TimeBarWidth;
		if (rc.left < rc.right) {
			rc.bottom = m_HeaderHeight;
			m_Tooltip.AddTool(0, rc, TEXT("チャンネル一覧表示へ"));
		}
	}
}


void CProgramGuide::ResetEventFont()
{
	for (size_t i = 0; i < m_EventLayoutList.Length(); i++) {
		ProgramGuide::CEventLayout *pLayout = m_EventLayoutList[i];
		for (size_t j = 0; j < pLayout->NumItems(); j++) {
			ProgramGuide::CEventItem *pItem = pLayout->GetItem(j);
			pItem->ResetTitleLines();
		}
	}
}


void CProgramGuide::OnFontChanged()
{
	ResetEventFont();
	CalcFontMetrics();
	ResetHitInfo();
	SetScrollBar();
	SetTooltip();
	Invalidate();
}


bool CProgramGuide::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	return CreateBasicWindow(
		hwndParent, Style, ExStyle, ID,
		m_pszWindowClass, nullptr, m_hinst);
}


bool CProgramGuide::SetChannelProviderManager(CProgramGuideChannelProviderManager *pManager)
{
	m_pChannelProviderManager = pManager;
	Clear();
	return true;
}


bool CProgramGuide::EnumChannelProvider(int Index, LPTSTR pszName, int MaxName) const
{
	if (m_pChannelProviderManager == nullptr
			|| Index < 0 || pszName == nullptr || MaxName < 1)
		return false;

	const CProgramGuideChannelProvider *pChannelProvider =
		m_pChannelProviderManager->GetChannelProvider(Index);
	if (pChannelProvider == nullptr)
		return false;

	pChannelProvider->GetName(pszName, MaxName);

	return true;
}


bool CProgramGuide::SetChannelProvider(int Provider, int Group)
{
	StoreTimePos();
	if (!SetCurrentChannelProvider(Provider, Group))
		return false;
	UpdateServiceList();
	return true;
}


bool CProgramGuide::SetChannelProvider(int Provider, LPCTSTR pszGroupID)
{
	StoreTimePos();
	if (!SetCurrentChannelProvider(Provider, pszGroupID))
		return false;
	UpdateServiceList();
	return true;
}


bool CProgramGuide::SetCurrentChannelProvider(int Provider, int Group)
{
	if (m_pChannelProviderManager == nullptr
			|| Provider < -1 || static_cast<size_t>(Provider) >= m_pChannelProviderManager->GetChannelProviderCount())
		return false;

	if (Provider >= 0) {
		m_pChannelProvider = m_pChannelProviderManager->GetChannelProvider(Provider);
		if (m_pChannelProvider == nullptr)
			return false;
		m_pChannelProvider->Update();
		m_CurrentChannelProvider = Provider;
		m_CurrentChannelGroup = -1;
		SetCurrentChannelGroup(Group);
	} else {
		Clear();
	}

	return true;
}


bool CProgramGuide::SetCurrentChannelProvider(int Provider, LPCTSTR pszGroupID)
{
	if (m_pChannelProviderManager == nullptr
			|| Provider < -1 || static_cast<size_t>(Provider) >= m_pChannelProviderManager->GetChannelProviderCount())
		return false;

	if (Provider >= 0) {
		m_pChannelProvider = m_pChannelProviderManager->GetChannelProvider(Provider);
		if (m_pChannelProvider == nullptr)
			return false;
		m_pChannelProvider->Update();
		m_CurrentChannelProvider = Provider;
		m_CurrentChannelGroup = -1;
		SetCurrentChannelGroup(m_pChannelProvider->ParseGroupID(pszGroupID));
	} else {
		Clear();
	}

	return true;
}


int CProgramGuide::GetChannelGroupCount() const
{
	if (m_pChannelProvider == nullptr)
		return 0;
	return static_cast<int>(m_pChannelProvider->GetGroupCount());
}


bool CProgramGuide::GetChannelGroupName(int Group, LPTSTR pszName, int MaxName) const
{
	if (m_pChannelProvider == nullptr)
		return false;
	return m_pChannelProvider->GetGroupName(Group, pszName, MaxName);
}


int CProgramGuide::ParseChannelGroupID(LPCTSTR pszGroupID) const
{
	if (m_pChannelProvider == nullptr)
		return false;
	return m_pChannelProvider->ParseGroupID(pszGroupID);
}


bool CProgramGuide::SetCurrentChannelGroup(int Group)
{
	if (m_pChannelProvider == nullptr
			|| Group < -1 || static_cast<size_t>(Group) >= m_pChannelProvider->GetGroupCount())
		return false;

	if (Group != m_CurrentChannelGroup) {
		m_ScrollPos.x = 0;
		m_ScrollPos.y = 0;
		m_OldScrollPos = m_ScrollPos;
	}
	m_CurrentChannelGroup = Group;

	return true;
}


bool CProgramGuide::GetChannelList(CChannelList *pList, bool fVisibleOnly) const
{
	if (pList == nullptr)
		return false;

	pList->Clear();

	if (fVisibleOnly) {
		for (size_t i = 0; i < m_ServiceList.NumServices(); i++)
			pList->AddChannel(m_ServiceList.GetItem(i)->GetChannelInfo());
	} else {
		if (m_pChannelProvider == nullptr)
			return false;
		for (size_t i = 0; i < m_pChannelProvider->GetChannelCount(m_CurrentChannelGroup); i++) {
			const CChannelInfo *pChannelInfo = m_pChannelProvider->GetChannelInfo(m_CurrentChannelGroup, i);

			if (pChannelInfo != nullptr
					&& pChannelInfo->IsEnabled())
				pList->AddChannel(*pChannelInfo);
		}
	}

	return true;
}


void CProgramGuide::SetCurrentService(WORD NetworkID, WORD TSID, WORD ServiceID)
{
	bool fRedrawEvent = false;
	int ListIndex, EventIndex;
	if (m_CurrentEventID != 0 && m_hwnd != nullptr) {
		fRedrawEvent = GetEventIndexByIDs(
			m_CurrentChannel.NetworkID,
			m_CurrentChannel.TransportStreamID,
			m_CurrentChannel.ServiceID,
			m_CurrentEventID,
			&ListIndex, &EventIndex);
	}

	m_CurrentChannel.NetworkID = NetworkID;
	m_CurrentChannel.TransportStreamID = TSID;
	m_CurrentChannel.ServiceID = ServiceID;
	m_CurrentEventID = 0;

	if (m_hwnd != nullptr) {
		RECT rc;

		GetClientRect(&rc);
		rc.bottom = m_HeaderHeight;
		::InvalidateRect(m_hwnd, &rc, TRUE);

		if (fRedrawEvent)
			RedrawEvent(ListIndex, EventIndex);
	}
}


void CProgramGuide::SetCurrentEvent(WORD EventID)
{
	if (m_CurrentEventID != EventID) {
		const WORD OldEventID = m_CurrentEventID;

		m_CurrentEventID = EventID;

		if (m_hwnd != nullptr) {
			if (OldEventID != 0) {
				RedrawEventByIDs(
					m_CurrentChannel.NetworkID,
					m_CurrentChannel.TransportStreamID,
					m_CurrentChannel.ServiceID,
					OldEventID);
			}
			if (m_CurrentEventID != 0) {
				RedrawEventByIDs(
					m_CurrentChannel.NetworkID,
					m_CurrentChannel.TransportStreamID,
					m_CurrentChannel.ServiceID,
					m_CurrentEventID);
			}
		}
	}
}


void CProgramGuide::SetExcludeNoEventServices(bool fExclude)
{
	m_fExcludeNoEventServices = fExclude;
}


void CProgramGuide::SetExcludeCommonEventOnlyServices(bool fExclude)
{
	m_fExcludeCommonEventOnlyServices = fExclude;
}


void CProgramGuide::SetCombineCommonEvents(bool fCombine)
{
	m_fCombineCommonEvents = fCombine;
}


bool CProgramGuide::SetExcludeServiceList(const ServiceInfoList &List)
{
	m_ExcludeServiceList = List;
	return true;
}


bool CProgramGuide::GetExcludeServiceList(ServiceInfoList *pList) const
{
	if (pList == nullptr)
		return false;
	*pList = m_ExcludeServiceList;
	return true;
}


bool CProgramGuide::IsExcludeService(WORD NetworkID, WORD TransportStreamID, WORD ServiceID) const
{
	for (const auto &e : m_ExcludeServiceList) {
		if ((NetworkID == 0 || e.NetworkID == NetworkID)
				&& (TransportStreamID == 0 || e.TransportStreamID == TransportStreamID)
				&& e.ServiceID == ServiceID)
			return true;
	}
	return false;
}


bool CProgramGuide::SetExcludeService(WORD NetworkID, WORD TransportStreamID, WORD ServiceID, bool fExclude)
{
	for (auto i = m_ExcludeServiceList.begin(); i != m_ExcludeServiceList.end(); i++) {
		if (i->NetworkID == NetworkID
				&& i->TransportStreamID == TransportStreamID
				&& i->ServiceID == ServiceID) {
			if (!fExclude)
				m_ExcludeServiceList.erase(i);
			return true;
		}
	}
	if (fExclude)
		m_ExcludeServiceList.emplace_back(NetworkID, TransportStreamID, ServiceID);
	return true;
}


bool CProgramGuide::SetServiceListMode()
{
	if (m_ListMode != ListMode::Services) {
		StoreTimePos();

		m_ListMode = ListMode::Services;
		m_WeekListService = -1;

		const HCURSOR hcurOld = ::SetCursor(::LoadCursor(nullptr, IDC_WAIT));

		m_ScrollPos = m_OldScrollPos;
		UpdateLayout();
		SetCaption();

		RestoreTimePos();

		if (m_pFrame != nullptr)
			m_pFrame->OnListModeChanged();

		::SetCursor(hcurOld);
	}

	return true;
}


bool CProgramGuide::SetWeekListMode(int Service)
{
	ProgramGuide::CServiceInfo *pServiceInfo = m_ServiceList.GetItem(Service);

	if (pServiceInfo == nullptr)
		return false;

	if (m_ListMode != ListMode::Week || m_WeekListService != Service) {
		StoreTimePos();

		m_ListMode = ListMode::Week;
		m_WeekListService = Service;
		m_WeekListHeaderTitleWidth = 0;

		const HCURSOR hcurOld = ::SetCursor(::LoadCursor(nullptr, IDC_WAIT));

		UpdateService(pServiceInfo);
		m_OldScrollPos = m_ScrollPos;
		m_ScrollPos.x = 0;
		m_ScrollPos.y = 0;
		UpdateLayout();
		SetCaption();

		RestoreTimePos();

		if (m_pFrame != nullptr)
			m_pFrame->OnListModeChanged();

		::SetCursor(hcurOld);
	}

	return true;
}


bool CProgramGuide::SetBeginHour(int Hour)
{
	if (Hour < -1 || Hour >= 24)
		return false;
	m_BeginHour = Hour;
	return true;
}


bool CProgramGuide::SetTimeRange(const LibISDB::DateTime &FirstTime, const LibISDB::DateTime &LastTime)
{
	LibISDB::DateTime First = FirstTime, Last = LastTime;
	First.TruncateToHours();
	Last.TruncateToHours();

	const int Hours = static_cast<int>(Last.DiffSeconds(First) / (60 * 60));
	if (Hours <= 0)
		return false;

	m_FirstTime = First;
	m_LastTime = Last;
	m_Hours = Hours;

	if (m_pFrame != nullptr)
		m_pFrame->OnTimeRangeChanged();

	return true;
}


bool CProgramGuide::GetTimeRange(LibISDB::DateTime *pFirstTime, LibISDB::DateTime *pLastTime) const
{
	if (pFirstTime != nullptr)
		*pFirstTime = m_FirstTime;
	if (pLastTime != nullptr)
		*pLastTime = m_LastTime;
	return true;
}


bool CProgramGuide::GetCurrentTimeRange(LibISDB::DateTime *pFirstTime, LibISDB::DateTime *pLastTime) const
{
	if (m_ListMode == ListMode::Week) {
		GetDayTimeRange(DAY_TOMORROW, pFirstTime, pLastTime);
		if (pFirstTime != nullptr)
			pFirstTime->OffsetDays(-1);
		if (pLastTime != nullptr)
			pLastTime->OffsetDays(-1);
		return true;
	}

	return GetDayTimeRange(m_Day, pFirstTime, pLastTime);
}


bool CProgramGuide::GetDayTimeRange(int Day, LibISDB::DateTime *pFirstTime, LibISDB::DateTime *pLastTime) const
{
	LibISDB::DateTime First = m_FirstTime, Last = m_LastTime;

	if (Day != DAY_TODAY) {
		long Offset;

		if (m_BeginHour < 0) {
			Offset = Day * 24;
		} else {
			int Begin = m_BeginHour * 60;
			const CEpgOptions::EpgTimeMode TimeMode = GetAppClass().EpgOptions.GetEpgTimeMode();
			bool fUTC = false;

			switch (TimeMode) {
			case CEpgOptions::EpgTimeMode::Local:
				{
					TIME_ZONE_INFORMATION tzi;
					const DWORD Result = ::GetTimeZoneInformation(&tzi);
					switch (Result) {
					case TIME_ZONE_ID_UNKNOWN:
						Begin += tzi.Bias;
						break;
					case TIME_ZONE_ID_STANDARD:
						Begin += tzi.Bias + tzi.StandardBias;
						break;
					case TIME_ZONE_ID_DAYLIGHT:
						Begin += tzi.Bias + tzi.DaylightBias;
						break;
					}
					if (Begin < 0)
						Begin += 24 * 60;
					fUTC = true;
				}
				break;

			case CEpgOptions::EpgTimeMode::JST:
				{
					TIME_ZONE_INFORMATION tzi;

					if (GetJSTTimeZoneInformation(&tzi)) {
						SYSTEMTIME stUTC, stJST;

						::GetSystemTime(&stUTC);
						if (::SystemTimeToTzSpecificLocalTime(&tzi, &stUTC, &stJST)) {
							Begin += static_cast<int>(DiffSystemTime(&stJST, &stUTC) / TimeConsts::SYSTEMTIME_MINUTE);
							fUTC = true;
						}
					}
				}
				break;

			case CEpgOptions::EpgTimeMode::UTC:
				fUTC = true;
				break;
			}

			if (fUTC)
				Begin += 9 * 60;
			Begin = Begin / 60 % 24;

			Offset = Day * 24 - First.Hour;
			if (First.Hour >= Begin)
				Offset += Begin;
			else
				Offset -= 24 - Begin;
		}
		First.OffsetHours(Offset);
		Last.OffsetHours(Offset);
	}
	if (pFirstTime != nullptr)
		*pFirstTime = First;
	if (pLastTime != nullptr)
		*pLastTime = Last;
	return true;
}


bool CProgramGuide::GetCurrentDateInfo(DateInfo *pInfo) const
{
	if (m_ListMode == ListMode::Week) {
		if (!GetCurrentTimeRange(&pInfo->BeginningTime, &pInfo->EndTime))
			return false;
		pInfo->pszRelativeDayText = nullptr;
	} else {
		if (!GetDateInfo(m_Day, pInfo))
			return false;
	}
	return true;
}


bool CProgramGuide::GetDateInfo(int Day, DateInfo *pInfo) const
{
	if (!GetDayTimeRange(Day, &pInfo->BeginningTime, &pInfo->EndTime))
		return false;
	if (m_ListMode == ListMode::Services) {
		if (Day == DAY_TODAY) {
			pInfo->pszRelativeDayText = GetRelativeDayText(0);
		} else {
			LibISDB::DateTime Time1, Time2;
			GetDayTimeRange(DAY_TODAY, &Time1, nullptr);
			Time1.TruncateToDays();
			Time2 = pInfo->BeginningTime;
			Time2.TruncateToDays();
			pInfo->pszRelativeDayText =
				GetRelativeDayText(static_cast<int>(Time2.DiffSeconds(Time1) / (24 * 60 * 60)));
		}
	} else {
		pInfo->pszRelativeDayText = nullptr;
	}
	return true;
}


bool CProgramGuide::ScrollToTime(const LibISDB::DateTime &Time, bool fHour)
{
	LibISDB::DateTime First, Last;
	if (!GetCurrentTimeRange(&First, &Last))
		return false;

	LibISDB::DateTime t = Time;
	if (m_ListMode == ListMode::Services) {
		if (t < First || t >= Last)
			return false;
	} else if (m_ListMode == ListMode::Week) {
		t.Year = First.Year;
		t.Month = First.Month;
		t.Day = First.Day;
		if (t < First)
			t.OffsetDays(1);
	}

	const long Diff = static_cast<long>(t.DiffSeconds(First));
	POINT Pos;
	Pos.x = m_ScrollPos.x;
	if (fHour)
		Pos.y = Diff / (60 * 60) * m_LinesPerHour;
	else
		Pos.y = Diff / 60 * m_LinesPerHour / 60;
	SetScrollPos(Pos);

	return true;
}


bool CProgramGuide::ScrollToCurrentTime()
{
	return ScrollToTime(m_CurTime, true);
}


bool CProgramGuide::SetViewDay(int Day)
{
	if (Day < DAY_FIRST || Day > DAY_LAST)
		return false;

	if (m_Day != Day || m_ListMode != ListMode::Services) {
		StoreTimePos();

		m_Day = Day;

		if (m_pEPGDatabase != nullptr) {
			const HCURSOR hcurOld = ::SetCursor(::LoadCursor(nullptr, IDC_WAIT));

			if (m_ListMode != ListMode::Services) {
				m_ListMode = ListMode::Services;
				m_WeekListService = -1;
				m_ScrollPos.x = m_OldScrollPos.x;
				if (m_pFrame != nullptr)
					m_pFrame->OnListModeChanged();
			}
			m_ScrollPos.y = 0;
			LibISDB::GetCurrentEPGTime(&m_CurTime);
			UpdateLayout();
			SetCaption();

			RestoreTimePos();

			::SetCursor(hcurOld);

			if (m_pFrame != nullptr)
				m_pFrame->OnDateChanged();
		}
	}

	return true;
}


// 指定された番組まで移動する
bool CProgramGuide::JumpEvent(WORD NetworkID, WORD TSID, WORD ServiceID, WORD EventID)
{
	const int ServiceIndex = m_ServiceList.FindItemByIDs(NetworkID, TSID, ServiceID);
	if (ServiceIndex < 0)
		return false;
	const ProgramGuide::CServiceInfo *pServiceInfo = m_ServiceList.GetItem(ServiceIndex);
	const LibISDB::EventInfo *pEventInfo = pServiceInfo->GetEventByEventID(EventID);
	if (pEventInfo == nullptr)
		return false;

	LibISDB::DateTime Start, End, First, Last;

	pEventInfo->GetStartTime(&Start);
	pEventInfo->GetEndTime(&End);

	bool fChangeDate = true;
	if (m_ListMode == ListMode::Services) {
		GetCurrentTimeRange(&First, &Last);
		if (First <= Start && Last >= End)
			fChangeDate = false;
	}
	if (fChangeDate) {
		int Day;
		for (Day = DAY_FIRST; Day <= DAY_LAST; Day++) {
			GetDayTimeRange(Day, &First, &Last);
			if (End <= Last)
				break;
		}
		if (End <= First)
			return false;
		if (!SetViewDay(Day))
			return false;
	}

	SIZE Size, Page;
	GetProgramGuideSize(&Size);
	GetPageSize(&Page);
	POINT Pos;
	Pos.x = m_ColumnWidth * ServiceIndex - (Page.cx - m_ColumnWidth) / 2;
	if (Pos.x < 0)
		Pos.x = 0;
	else if (Pos.x > std::max(Size.cx - Page.cx, 0L))
		Pos.x = std::max(Size.cx - Page.cx, 0L);
	Pos.y = static_cast<long>(pEventInfo->StartTime.DiffSeconds(First)) / 60 * m_LinesPerHour / 60;
	const int YOffset = (Page.cy - static_cast<int>(pEventInfo->Duration * m_LinesPerHour / (60 * 60))) / 2;
	if (YOffset > 0)
		Pos.y -= YOffset;
	if (Pos.y < 0)
		Pos.y = 0;
	else if (Pos.y > std::max(Size.cy - Page.cy, 0L))
		Pos.y = std::max(Size.cy - Page.cy, 0L);
	SetScrollPos(Pos);

	SelectEventByIDs(NetworkID, TSID, ServiceID, EventID);

	return true;
}


bool CProgramGuide::JumpEvent(const LibISDB::EventInfo &EventInfo)
{
	return JumpEvent(EventInfo.NetworkID, EventInfo.TransportStreamID, EventInfo.ServiceID, EventInfo.EventID);
}


bool CProgramGuide::ScrollToCurrentService()
{
	if (m_ListMode != ListMode::Services)
		return false;
	if (m_CurrentChannel.ServiceID == LibISDB::SERVICE_ID_INVALID)
		return false;

	const int ServiceIndex = m_ServiceList.FindItemByIDs(
		m_CurrentChannel.NetworkID, m_CurrentChannel.TransportStreamID, m_CurrentChannel.ServiceID);
	if (ServiceIndex < 0)
		return false;

	SIZE Size, Page;
	GetProgramGuideSize(&Size);
	GetPageSize(&Page);
	POINT Pos;
	Pos.x = m_ColumnWidth * ServiceIndex - (Page.cx - m_ColumnWidth) / 2;
	if (Pos.x < 0)
		Pos.x = 0;
	else if (Pos.x > std::max(Size.cx - Page.cx, 0L))
		Pos.x = std::max(Size.cx - Page.cx, 0L);
	Pos.y = m_ScrollPos.y;
	SetScrollPos(Pos);

	return true;
}


bool CProgramGuide::SetUIOptions(int LinesPerHour, int ItemWidth)
{
	if (LinesPerHour < MIN_LINES_PER_HOUR || LinesPerHour > MAX_LINES_PER_HOUR
			|| ItemWidth < MIN_ITEM_WIDTH || ItemWidth > MAX_ITEM_WIDTH)
		return false;
	if (m_LinesPerHour != LinesPerHour
			|| m_ItemWidth != ItemWidth) {
		m_LinesPerHour = LinesPerHour;
		m_ItemLogicalWidth = ItemWidth;
		if (m_hwnd != nullptr) {
			m_ItemWidth = m_pStyleScaling->LogicalPixelsToPhysicalPixels(m_ItemLogicalWidth);
			m_ColumnWidth = m_ItemWidth + m_Style.ColumnMargin * 2;
			m_ScrollPos.x = 0;
			m_ScrollPos.y = 0;
			m_OldScrollPos = m_ScrollPos;
			UpdateLayout();
		}
	}
	return true;
}


bool CProgramGuide::SetTextDrawEngine(CTextDrawClient::TextDrawEngine Engine)
{
	if (m_hwnd != nullptr) {
		if (!m_TextDrawClient.Initialize(Engine, m_hwnd))
			return false;

		OnFontChanged();
	}

	m_TextDrawEngine = Engine;

	return true;
}


bool CProgramGuide::SetDirectWriteRenderingParams(
	const CDirectWriteRenderer::RenderingParams &Params)
{
	if (!m_TextDrawClient.SetDirectWriteRenderingParams(Params))
		return false;

	if (m_hwnd != nullptr && m_TextDrawEngine == CTextDrawClient::TextDrawEngine::DirectWrite)
		Invalidate();

	return true;
}


bool CProgramGuide::SetFont(const Style::Font &Font)
{
	m_Font = Font;

	if (m_hwnd != nullptr) {
		CreateFonts();
		OnFontChanged();
		m_Tooltip.SetFont(m_ContentFont.GetHandle());
	}

	return true;
}


bool CProgramGuide::GetFont(Style::Font *pFont) const
{
	if (pFont == nullptr)
		return false;
	*pFont = m_Font;
	return true;
}


bool CProgramGuide::SetEventInfoFont(const Style::Font &Font)
{
	return m_EventInfoPopup.SetFont(Font);
}


bool CProgramGuide::SetShowToolTip(bool fShow)
{
	if (m_fShowToolTip != fShow) {
		m_fShowToolTip = fShow;
		m_EventInfoPopupManager.SetEnable(fShow);
	}
	return true;
}


void CProgramGuide::SetEventHandler(CEventHandler *pEventHandler)
{
	if (m_pEventHandler)
		m_pEventHandler->m_pProgramGuide = nullptr;
	if (pEventHandler)
		pEventHandler->m_pProgramGuide = this;
	m_pEventHandler = pEventHandler;
}


void CProgramGuide::SetFrame(CFrame *pFrame)
{
	if (m_pFrame)
		m_pFrame->m_pProgramGuide = nullptr;
	if (pFrame)
		pFrame->m_pProgramGuide = this;
	m_pFrame = pFrame;
}


void CProgramGuide::SetProgramCustomizer(CProgramCustomizer *pProgramCustomizer)
{
	if (m_pProgramCustomizer)
		m_pProgramCustomizer->m_pProgramGuide = nullptr;
	if (pProgramCustomizer) {
		pProgramCustomizer->m_pProgramGuide = this;
		if (m_hwnd != nullptr)
			pProgramCustomizer->Initialize();
	}
	m_pProgramCustomizer = pProgramCustomizer;
}


bool CProgramGuide::SetDragScroll(bool fDragScroll)
{
	if (m_fDragScroll != fDragScroll) {
		m_fDragScroll = fDragScroll;

		if (m_hwnd != nullptr) {
			RECT rc;
			POINT pt;

			::GetWindowRect(m_hwnd, &rc);
			::GetCursorPos(&pt);
			if (::PtInRect(&rc, pt)) {
				const WORD HitTestCode =
					static_cast<WORD>(SendMessage(WM_NCHITTEST, 0, MAKELPARAM(pt.x, pt.y)));
				if (HitTestCode == HTCLIENT) {
					::ScreenToClient(m_hwnd, &pt);
					SetHitInfo(HitTest(pt.x, pt.y));
				}
				SendMessage(
					WM_SETCURSOR, reinterpret_cast<WPARAM>(m_hwnd),
					MAKELPARAM(HitTestCode, WM_MOUSEMOVE));
			}
		}
	}
	return true;
}


bool CProgramGuide::SetFilter(FilterFlag Filter)
{
	if ((Filter & (FilterFlag::Original | FilterFlag::Rerun)) == (FilterFlag::Original | FilterFlag::Rerun))
		Filter &= ~(FilterFlag::Original | FilterFlag::Rerun);
	if (m_Filter != Filter) {
		m_Filter = Filter;
		if (m_hwnd != nullptr) {
			Invalidate();
		}
	}
	return true;
}


void CProgramGuide::SetUseARIBSymbol(bool fUseARIBSymbol)
{
	if (m_fUseARIBSymbol != fUseARIBSymbol) {
		m_fUseARIBSymbol = fUseARIBSymbol;
		if (m_hwnd != nullptr)
			OnFontChanged();
	}
}


void CProgramGuide::SetVisibleEventIcons(UINT VisibleIcons)
{
	if (m_VisibleEventIcons != VisibleIcons) {
		m_VisibleEventIcons = VisibleIcons;
		if (m_hwnd != nullptr)
			Invalidate();
	}
}


void CProgramGuide::SetKeepTimePos(bool fKeep)
{
	m_fKeepTimePos = fKeep;
}


void CProgramGuide::SetShowFeaturedMark(bool fShow)
{
	if (m_fShowFeaturedMark != fShow) {
		m_fShowFeaturedMark = fShow;
		if (m_hwnd != nullptr) {
			if (m_fShowFeaturedMark)
				m_FeaturedEventsMatcher.BeginMatching(GetAppClass().FeaturedEvents.GetSettings());
			Invalidate();
		}
	}
}


void CProgramGuide::SetAutoRefresh(bool fAuto)
{
	m_fAutoRefresh = fAuto;
}


const Style::Margins &CProgramGuide::GetToolbarItemPadding() const
{
	return m_Style.ToolbarItemPadding;
}


bool CProgramGuide::ShowProgramSearch(bool fShow)
{
	if (fShow) {
		if (!m_ProgramSearch.IsCreated()) {
			RECT rc;

			m_ProgramSearch.SetEventHandler(&m_ProgramSearchEventHandler);
			m_ProgramSearch.GetPosition(&rc);
			if (rc.left == rc.right || rc.top == rc.bottom) {
				POINT pt = {0, 0};
				::ClientToScreen(m_hwnd, &pt);
				m_ProgramSearch.SetPosition(pt.x, pt.y, 0, 0);
			}
			m_ProgramSearch.Create(m_hwnd);
			GetAppClass().UICore.RegisterModelessDialog(&m_ProgramSearch);
		}
		if (!m_ProgramSearch.IsVisible())
			m_ProgramSearch.SetVisible(true);
	} else {
		if (m_ProgramSearch.IsCreated()) {
			GetAppClass().UICore.UnregisterModelessDialog(&m_ProgramSearch);
			m_ProgramSearch.Destroy();
		}
	}

	return true;
}


void CProgramGuide::SetMessage(LPCTSTR pszMessage, bool fUpdate)
{
	bool fErase = false;

	if (!IsStringEmpty(pszMessage)) {
		fErase = !m_Message.empty();
		m_Message = pszMessage;
	} else {
		if (m_Message.empty())
			return;
		m_Message.clear();
	}

	if (fUpdate && m_hwnd != nullptr) {
		if (m_Message.empty() || fErase) {
			Redraw();
		} else {
			const HDC hdc = ::GetDC(m_hwnd);

			if (hdc != nullptr) {
				RECT rc;
				GetClientRect(&rc);
				DrawMessage(hdc, rc);
				::ReleaseDC(m_hwnd, hdc);
			}
		}
	}
}


void CProgramGuide::OnEpgCaptureBegin()
{
}


void CProgramGuide::OnEpgCaptureEnd()
{
	if (m_fEpgUpdating) {
		m_fEpgUpdating = false;
		SetCaption();
	}
}


void CProgramGuide::SetEpgCaptureProgress(int Pos, int End, DWORD RemainingTime)
{
	m_EpgUpdateProgress.Pos = Pos;
	m_EpgUpdateProgress.End = End;
	m_EpgUpdateProgress.RemainingTime = RemainingTime;
	if (m_fEpgUpdating)
		SetCaption();
}


bool CProgramGuide::OnCloseFrame()
{
	if (m_pEventHandler != nullptr && !m_pEventHandler->OnClose())
		return false;

	if (m_fEpgUpdating)
		GetAppClass().EpgCaptureManager.EndCapture();

	ShowProgramSearch(false);

	return true;
}


void CProgramGuide::OnShowFrame(bool fShow)
{
	if (m_ProgramSearch.IsCreated())
		m_ProgramSearch.SetVisible(fShow);
}


LPCTSTR CProgramGuide::GetRelativeDayText(int Day)
{
	static const LPCTSTR DayText[] = {
		TEXT("今日"),
		TEXT("明日"),
		TEXT("2日後"),
		TEXT("3日後"),
		TEXT("4日後"),
		TEXT("5日後"),
		TEXT("6日後"),
		TEXT("7日後"),
	};

	if (Day < 0 || Day >= lengthof(DayText))
		return nullptr;
	return DayText[Day];
}


ProgramGuide::CEventItem *CProgramGuide::GetEventItem(int ListIndex, int EventIndex)
{
	if (ListIndex < 0 || static_cast<size_t>(ListIndex) >= m_EventLayoutList.Length())
		return nullptr;
	return m_EventLayoutList[ListIndex]->GetItem(EventIndex);
}


const ProgramGuide::CEventItem *CProgramGuide::GetEventItem(int ListIndex, int EventIndex) const
{
	if (ListIndex < 0 || static_cast<size_t>(ListIndex) >= m_EventLayoutList.Length())
		return nullptr;
	return m_EventLayoutList[ListIndex]->GetItem(EventIndex);
}


bool CProgramGuide::GetEventRect(int ListIndex, int EventIndex, RECT *pRect) const
{
	const ProgramGuide::CEventItem *pItem = GetEventItem(ListIndex, EventIndex);
	if (pItem == nullptr || pItem->GetItemColumns() == 0)
		return false;

	GetColumnRect(ListIndex, pRect);
	if (pItem->GetItemColumns() > 1)
		pRect->right += (pItem->GetItemColumns() - 1) * m_ColumnWidth;

	const int LineHeight = GetLineHeight();
	pRect->top += pItem->GetItemPos() * LineHeight - m_ScrollPos.y * LineHeight;
	pRect->bottom = pRect->top + pItem->GetItemLines() * LineHeight;

	return true;
}


bool CProgramGuide::RedrawEvent(int ListIndex, int EventIndex)
{
	RECT rc;

	if (!GetEventRect(ListIndex, EventIndex, &rc))
		return false;
	rc.left -= m_Style.SelectedBorder.Left;
	rc.right += m_Style.SelectedBorder.Right;
	Invalidate(&rc);
	return true;
}


bool CProgramGuide::RedrawEventByIDs(WORD NetworkID, WORD TSID, WORD ServiceID, WORD EventID)
{
	int ListIndex, EventIndex;

	if (!GetEventIndexByIDs(NetworkID, TSID, ServiceID, EventID, &ListIndex, &EventIndex))
		return false;

	return RedrawEvent(ListIndex, EventIndex);
}


bool CProgramGuide::EventHitTest(int x, int y, int *pListIndex, int *pEventIndex) const
{
	RECT rc;
	GetProgramGuideRect(&rc);

	if (!::PtInRect(&rc, POINT{x, y}))
		return false;

	const int XPos = x - rc.left + m_ScrollPos.x;
	int List = XPos / m_ColumnWidth;
	if (List < 0 || List >= static_cast<int>(m_EventLayoutList.Length()))
		return false;

	const ProgramGuide::CEventLayout *pLayout = m_EventLayoutList[List];
	const int LineHeight = GetLineHeight();
	const int YOrigin = rc.top - m_ScrollPos.y * LineHeight;

	for (size_t i = 0; i < pLayout->NumItems(); i++) {
		const ProgramGuide::CEventItem *pItem = pLayout->GetItem(i);

		if (!pItem->IsNullItem() && pItem->GetItemLines() > 0) {
			rc.top = pItem->GetItemPos() * LineHeight + YOrigin;
			rc.bottom = rc.top + pItem->GetItemLines() * LineHeight;
			if (y >= rc.top && y < rc.bottom) {
				int ColumnCount = pItem->GetItemColumns();

				if (ColumnCount == 0) {
					const LibISDB::EventInfo *pEventInfo = pItem->GetCommonEventInfo();
					if (pEventInfo == nullptr)
						return false;

					for (int j = List - 1; j >= 0; j--) {
						const ProgramGuide::CEventLayout *pPrevLayout = m_EventLayoutList[j];
						if (pPrevLayout->GetServiceInfo()->GetServiceID() == pEventInfo->ServiceID) {
							const int Index = pPrevLayout->FindItemByEventID(pEventInfo->EventID);
							if (Index >= 0) {
								pItem = pPrevLayout->GetItem(Index);
								if (j + pItem->GetItemColumns() < List)
									return false;
								ColumnCount = pItem->GetItemColumns();
								List = j;
								i = Index;
								break;
							}
						}
					}
					if (ColumnCount == 0)
						return false;
				}

				if (XPos < List * m_ColumnWidth + m_Style.ColumnMargin
						|| XPos >= (List + ColumnCount) * m_ColumnWidth - m_Style.ColumnMargin)
					return false;

				if (pListIndex != nullptr)
					*pListIndex = List;
				if (pEventIndex != nullptr)
					*pEventIndex = static_cast<int>(i);
				return true;
			}
		}
	}

	return false;
}


bool CProgramGuide::GetEventIndexByIDs(
	WORD NetworkID, WORD TSID, WORD ServiceID, WORD EventID,
	int *pListIndex, int *pEventIndex) const
{
	int ListIndex, FirstListIndex;

	if (m_ListMode == ListMode::Services) {
		ListIndex = m_ServiceList.FindItemByIDs(NetworkID, TSID, ServiceID);
		if (ListIndex < 0)
			return false;
		FirstListIndex = ListIndex;
	} else if (m_ListMode == ListMode::Week) {
		const ProgramGuide::CServiceInfo *pService = m_ServiceList.GetItem(m_WeekListService);
		if (pService == nullptr
				|| pService->GetNetworkID() != NetworkID
				|| pService->GetTSID() != TSID
				|| pService->GetServiceID() != ServiceID)
			return false;
		ListIndex = static_cast<int>(m_EventLayoutList.Length()) - 1;
		FirstListIndex = 0;
	} else {
		return false;
	}

	for (; ListIndex >= FirstListIndex; ListIndex--) {
		const ProgramGuide::CEventLayout *pEventLayout = m_EventLayoutList[ListIndex];
		if (pEventLayout != nullptr) {
			const int EventIndex = pEventLayout->FindItemByEventID(EventID);
			if (EventIndex >= 0) {
				if (pListIndex != nullptr)
					*pListIndex = ListIndex;
				if (pEventIndex != nullptr)
					*pEventIndex = EventIndex;
				return true;
			}
		}
	}

	return false;
}


bool CProgramGuide::SelectEvent(int ListIndex, int EventIndex)
{
	bool fSelected;
	if (ListIndex >= 0 && static_cast<size_t>(ListIndex) < m_EventLayoutList.Length()
			&& EventIndex >= 0 && static_cast<size_t>(EventIndex) < m_EventLayoutList[ListIndex]->NumItems()) {
		fSelected = true;
	} else {
		fSelected = false;
	}

	ProgramGuide::CEventItem *pItem;

	if (m_CurEventItem.fSelected) {
		if (fSelected && m_CurEventItem.ListIndex == ListIndex && m_CurEventItem.EventIndex == EventIndex)
			return true;
		pItem = GetEventItem(m_CurEventItem.ListIndex, m_CurEventItem.EventIndex);
		if (pItem != nullptr) {
			pItem->SetSelected(false);
			RedrawEvent(m_CurEventItem.ListIndex, m_CurEventItem.EventIndex);
		}
	}

	m_CurEventItem.fSelected = fSelected;
	if (fSelected) {
		pItem = GetEventItem(ListIndex, EventIndex);
		pItem->SetSelected(true);
		m_CurEventItem.ListIndex = ListIndex;
		m_CurEventItem.EventIndex = EventIndex;
		m_CurEventItem.EventID = pItem->GetEventInfo()->EventID;
		RedrawEvent(ListIndex, EventIndex);
	}

	return true;
}


bool CProgramGuide::SelectEventByPosition(int x, int y)
{
	int ListIndex, EventIndex;
	const bool fSel = EventHitTest(x, y, &ListIndex, &EventIndex);

	if (fSel)
		SelectEvent(ListIndex, EventIndex);
	else
		SelectEvent(-1, -1);
	return fSel;
}


bool CProgramGuide::SelectEventByIDs(WORD NetworkID, WORD TSID, WORD ServiceID, WORD EventID)
{
	int ListIndex, EventIndex;

	if (!GetEventIndexByIDs(NetworkID, TSID, ServiceID, EventID, &ListIndex, &EventIndex))
		return false;

	return SelectEvent(ListIndex, EventIndex);
}


LRESULT CProgramGuide::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			if (!m_TextDrawClient.Initialize(m_TextDrawEngine, hwnd)) {
				if (m_TextDrawEngine != CTextDrawClient::TextDrawEngine::GDI) {
					m_TextDrawEngine = CTextDrawClient::TextDrawEngine::GDI;
					m_TextDrawClient.Initialize(m_TextDrawEngine, hwnd);
				}
			}
			m_TextDrawClient.SetMaxFontCache(2);

			InitializeUI();

			if (IsDarkThemeSupported()) {
				SetWindowDarkTheme(hwnd, IsDarkThemeColor(m_Theme.ColorList[COLOR_BACK]));
			}

			if (m_hDragCursor1 == nullptr)
				m_hDragCursor1 = ::LoadCursor(m_hinst, MAKEINTRESOURCE(IDC_GRAB1));
			if (m_hDragCursor2 == nullptr)
				m_hDragCursor2 = ::LoadCursor(m_hinst, MAKEINTRESOURCE(IDC_GRAB2));

			m_HitInfo = {};
			m_MouseLeaveTrack.Initialize(hwnd);

			m_EpgIcons.Load();
			m_EventInfoPopupManager.Initialize(hwnd, &m_EventInfoPopupHandler);
			m_Tooltip.Create(hwnd);
			m_Tooltip.SetFont(m_ContentFont.GetHandle());
			if (m_pProgramCustomizer != nullptr)
				m_pProgramCustomizer->Initialize();

			m_fBarShadow = CBufferedPaint::IsSupported();

			CFeaturedEvents &FeaturedEvents = GetAppClass().FeaturedEvents;
			FeaturedEvents.AddEventHandler(this);
			if (m_fShowFeaturedMark)
				m_FeaturedEventsMatcher.BeginMatching(FeaturedEvents.GetSettings());

			if (m_pEPGDatabase != nullptr)
				m_pEPGDatabase->AddEventListener(&m_EPGDatabaseEventListener);

			LibISDB::GetCurrentEPGTime(&m_CurTime);
			::SetTimer(hwnd, TIMER_ID_UPDATECURTIME, 1000, nullptr);
		}
		return 0;

	case WM_PAINT:
		OnPaint(hwnd);
		return 0;

	case WM_SIZE:
		{
			SIZE Size, Page;

			GetProgramGuideSize(&Size);
			GetPageSize(&Page);
			POINT Pos = m_ScrollPos;
			if (Pos.x > std::max(Size.cx - Page.cx, 0L))
				Pos.x = std::max(Size.cx - Page.cx, 0L);
			if (Pos.y > std::max(Size.cy - Page.cy, 0L))
				Pos.y = std::max(Size.cy - Page.cy, 0L);
			ResetHitInfo(true);
			SetScrollBar();
			SetScrollPos(Pos);
		}
		return 0;

	case WM_VSCROLL:
	case WM_MOUSEWHEEL:
		{
			SIZE Size, Page;

			GetProgramGuideSize(&Size);
			GetPageSize(&Page);
			int Pos = m_ScrollPos.y;
			if (uMsg == WM_VSCROLL) {
				switch (LOWORD(wParam)) {
				case SB_LINEUP:        Pos--;                                 break;
				case SB_LINEDOWN:      Pos++;                                 break;
				case SB_PAGEUP:        Pos -= Page.cy;                        break;
				case SB_PAGEDOWN:      Pos += Page.cy;                        break;
				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:    Pos = HIWORD(wParam);                  break;
				case SB_TOP:           Pos = 0;                               break;
				case SB_BOTTOM:        Pos = std::max(Size.cy - Page.cy, 0L); break;
				default:               return 0;
				}
			} else {
				Pos -= m_VertWheel.OnMouseWheel(wParam, m_WheelScrollLines);
			}
			if (Pos < 0)
				Pos = 0;
			else if (Pos > std::max(Size.cy - Page.cy, 0L))
				Pos = std::max(Size.cy - Page.cy, 0L);
			if (Pos != m_ScrollPos.y)
				Scroll(0, Pos - m_ScrollPos.y);
		}
		return 0;

	case WM_HSCROLL:
	case WM_MOUSEHWHEEL:
		{
			SIZE Size, Page;

			GetProgramGuideSize(&Size);
			GetPageSize(&Page);
			int Pos = m_ScrollPos.x;
			if (uMsg == WM_HSCROLL) {
				switch (LOWORD(wParam)) {
				case SB_LINELEFT:      Pos -= m_FontHeight;                   break;
				case SB_LINERIGHT:     Pos += m_FontHeight;                   break;
				case SB_PAGELEFT:      Pos -= Page.cx;                        break;
				case SB_PAGERIGHT:     Pos += Page.cx;                        break;
				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:    Pos = HIWORD(wParam);                  break;
				case SB_LEFT:          Pos = 0;                               break;
				case SB_RIGHT:         Pos = std::max(Size.cx - Page.cx, 0L); break;
				default:               return 0;
				}
			} else {
				Pos += m_HorzWheel.OnMouseHWheel(wParam, m_FontHeight);
			}
			if (Pos < 0)
				Pos = 0;
			else if (Pos > std::max(Size.cx - Page.cx, 0L))
				Pos = std::max(Size.cx - Page.cx, 0L);
			if (Pos != m_ScrollPos.x)
				Scroll(Pos - m_ScrollPos.x, 0);
		}
		return uMsg == WM_MOUSEHWHEEL;

	case WM_LBUTTONDOWN:
		{
			const POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

			SetHitInfo(HitTest(pt.x, pt.y), true);

			::SetFocus(hwnd);

			if (m_HitInfo.Part != HitTestPart::None) {
				switch (m_HitInfo.Part) {
				case HitTestPart::ServiceTitle:
					{
						int ServiceIndex;

						if (m_ListMode == ListMode::Services)
							ServiceIndex = m_HitInfo.ListIndex;
						else if (m_ListMode == ListMode::Week)
							ServiceIndex = m_WeekListService;
						else
							break;
						if (m_pEventHandler) {
							const ProgramGuide::CServiceInfo *pServiceInfo = m_ServiceList.GetItem(ServiceIndex);

							if (pServiceInfo != nullptr) {
								String BonDriver(pServiceInfo->GetChannelInfo().GetTunerName());
								const LibISDB::EPGDatabase::ServiceInfo ServiceInfo(pServiceInfo->GetServiceInfo());

								m_pEventHandler->OnServiceTitleLButtonDown(BonDriver.c_str(), &ServiceInfo);
							}
						}
					}
					break;

				case HitTestPart::ServiceChevron:
					if (m_ListMode == ListMode::Services)
						SetWeekListMode(m_HitInfo.ListIndex);
					else if (m_ListMode == ListMode::Week)
						SetServiceListMode();
					break;

				case HitTestPart::TimeBarPrev:
					::SendMessage(hwnd, WM_COMMAND, CM_PROGRAMGUIDE_DAY_FIRST + (m_Day - 1), 0);
					break;

				case HitTestPart::TimeBarNext:
					::SendMessage(hwnd, WM_COMMAND, CM_PROGRAMGUIDE_DAY_FIRST + (m_Day + 1), 0);
					break;
				}
			} else if (m_fDragScroll) {
				m_fScrolling = true;
				m_DragInfo.StartCursorPos = pt;
				m_DragInfo.StartScrollPos = m_ScrollPos;
				m_DragInfo.fCursorMoved = false;
				::SetCursor(m_hDragCursor2);
				::SetCapture(hwnd);
				m_EventInfoPopupManager.SetEnable(false);
			} else {
				SelectEventByPosition(pt.x, pt.y);
				m_EventInfoPopupManager.Popup(pt.x, pt.y);
			}
		}
		return 0;

	case WM_LBUTTONUP:
		if (m_fScrolling) {
			::ReleaseCapture();

			if (!m_DragInfo.fCursorMoved) {
				const int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

				if (SelectEventByPosition(x, y))
					m_EventInfoPopupManager.Popup(x, y);
			}
		}
		return 0;

	case WM_LBUTTONDBLCLK:
		{
			const POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

			if (SelectEventByPosition(pt.x, pt.y)) {
				if (m_pProgramCustomizer != nullptr) {
					const ProgramGuide::CEventItem *pItem =
						GetEventItem(m_CurEventItem.ListIndex, m_CurEventItem.EventIndex);

					if (pItem != nullptr) {
						RECT ItemRect;

						GetEventRect(m_CurEventItem.ListIndex, m_CurEventItem.EventIndex, &ItemRect);
						m_pProgramCustomizer->OnLButtonDoubleClick(
							*pItem->GetEventInfo(), pt, ItemRect);
					}
				}
			}
		}
		return 0;

	case WM_RBUTTONDOWN:
		{
			const int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

			::SetFocus(hwnd);
			SelectEventByPosition(x, y);
		}
		return 0;

	case WM_RBUTTONUP:
		{
			const int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

			ShowPopupMenu(x, y);
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			const int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

			if (m_fScrolling) {
				if (!m_DragInfo.fCursorMoved
						&& (m_DragInfo.StartCursorPos.x != x || m_DragInfo.StartCursorPos.y != y))
					m_DragInfo.fCursorMoved = true;

				const int XScroll = (m_DragInfo.StartScrollPos.x + (m_DragInfo.StartCursorPos.x - x)) - m_ScrollPos.x;
				const int YScroll = (m_DragInfo.StartScrollPos.y + (m_DragInfo.StartCursorPos.y - y) / GetLineHeight()) - m_ScrollPos.y;
				if (XScroll != 0 || YScroll != 0)
					Scroll(XScroll, YScroll);
			} else {
				SetHitInfo(HitTest(x, y));

				m_MouseLeaveTrack.OnMouseMove();
			}
		}
		return 0;

	case WM_NCMOUSEMOVE:
		m_MouseLeaveTrack.OnNcMouseMove();
		return 0;

	case WM_MOUSELEAVE:
		m_MouseLeaveTrack.OnMouseLeave();
		ResetHitInfo();
		return 0;

	case WM_NCMOUSELEAVE:
		m_MouseLeaveTrack.OnNcMouseLeave();
		ResetHitInfo();
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT) {
			if (m_HitInfo.Part != HitTestPart::None) {
				if (m_HitInfo.fClickable) {
					::SetCursor(GetActionCursor());
					return TRUE;
				}
			} else if (m_fDragScroll) {
				::SetCursor(m_hDragCursor1);
				return TRUE;
			}
		}
		break;

	case WM_CAPTURECHANGED:
		if (m_fScrolling) {
			m_fScrolling = false;
			::SetCursor(m_hDragCursor1);
			m_EventInfoPopupManager.SetEnable(m_fShowToolTip);
		}
		return 0;

	case WM_KEYDOWN:
		{
			static const struct {
				WORD KeyCode;
				WORD Message;
				WORD Request;
			} KeyMap[] = {
				{VK_PRIOR, WM_VSCROLL, SB_PAGEUP},
				{VK_NEXT,  WM_VSCROLL, SB_PAGEDOWN},
				{VK_UP,    WM_VSCROLL, SB_LINEUP},
				{VK_DOWN,  WM_VSCROLL, SB_LINEDOWN},
				{VK_LEFT,  WM_HSCROLL, SB_LINEUP},
				{VK_RIGHT, WM_HSCROLL, SB_LINEDOWN},
				{VK_HOME,  WM_VSCROLL, SB_TOP},
				{VK_END,   WM_VSCROLL, SB_BOTTOM},
			};

			for (const auto &e : KeyMap) {
				if (wParam == static_cast<WPARAM>(e.KeyCode)) {
					::SendMessage(hwnd, e.Message, e.Request, 0);
					return 0;
				}
			}

			if (m_pEventHandler != nullptr
					&& m_pEventHandler->OnKeyDown(static_cast<UINT>(wParam), static_cast<UINT>(lParam)))
				return 0;
		}
		break;

	case WM_TIMER:
		if (wParam == TIMER_ID_UPDATECURTIME) {
			if (m_Day == DAY_TODAY) {
				LibISDB::DateTime Time;

				LibISDB::GetCurrentEPGTime(&Time);
				if (m_CurTime.Minute != Time.Minute
						|| m_CurTime.Hour != Time.Hour
						|| m_CurTime.Day != Time.Day
						|| m_CurTime.Month != Time.Month
						|| m_CurTime.Year != Time.Year) {
					const int OldTimeLinePos = GetCurTimeLinePos();
					m_CurTime = Time;
					const int NewTimeLinePos = GetCurTimeLinePos();
					if (NewTimeLinePos != OldTimeLinePos) {
						RECT rc, rcGuide;

						::GetClientRect(hwnd, &rc);
						GetProgramGuideRect(&rcGuide);
						const int Offset = rcGuide.top - m_ScrollPos.y * GetLineHeight();
						rc.top = Offset + OldTimeLinePos - m_FontHeight / 2;
						rc.bottom = Offset + NewTimeLinePos + m_FontHeight / 2;
						::InvalidateRect(hwnd, &rc, FALSE);
					}
				}
			}
		}
		return 0;

	case WM_COMMAND:
		OnCommand(LOWORD(wParam));
		return 0;

	case WM_APPCOMMAND:
		if (GET_APPCOMMAND_LPARAM(lParam) == APPCOMMAND_BROWSER_BACKWARD) {
			if (m_ListMode == ListMode::Week)
				SetServiceListMode();
			return TRUE;
		}
		break;

	case WM_DESTROY:
		ShowProgramSearch(false);
		if (m_pProgramCustomizer != nullptr)
			m_pProgramCustomizer->Finalize();
		m_Tooltip.Destroy();
		if (m_pEventHandler != nullptr)
			m_pEventHandler->OnDestroy();
		m_Chevron.Destroy();
		m_EpgIcons.Destroy();
		m_TextDrawClient.Finalize();
		if (m_pEPGDatabase != nullptr)
			m_pEPGDatabase->RemoveEventListener(&m_EPGDatabaseEventListener);
		GetAppClass().FeaturedEvents.RemoveEventHandler(this);
		return 0;

	case MESSAGE_REFRESHSERVICE:
		if (!m_fAutoRefresh)
			return FALSE;
		return RefreshService(LOWORD(wParam), HIWORD(wParam), LOWORD(lParam));
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


void CProgramGuide::OnCommand(int id)
{
	switch (id) {
	case CM_PROGRAMGUIDE_UPDATE:
		if (!m_fEpgUpdating) {
			CAppMain &App = GetAppClass();

			if (!App.EpgCaptureManager.IsCapturing()) {
				TCHAR szBonDriver[MAX_PATH];

				if (m_pEventHandler != nullptr
						&& m_pChannelProvider != nullptr
						&& m_pChannelProvider->GetBonDriver(szBonDriver, lengthof(szBonDriver))) {
					CChannelList ChannelList;

					GetChannelList(&ChannelList, false);
					if (ChannelList.NumChannels() > 0) {
						m_EpgUpdateProgress.Clear();
						if (App.EpgCaptureManager.BeginCapture(szBonDriver, &ChannelList)) {
							m_fEpgUpdating = true;
							SetCaption();
						}
					}
				}
			}
		}
		return;

	case CM_PROGRAMGUIDE_ENDUPDATE:
		if (m_fEpgUpdating) {
			GetAppClass().EpgCaptureManager.EndCapture();
		}
		return;

	case CM_PROGRAMGUIDE_REFRESH:
		Refresh();
		return;

	case CM_PROGRAMGUIDE_AUTOREFRESH:
		SetAutoRefresh(!m_fAutoRefresh);
		return;

	case CM_PROGRAMGUIDE_IEPGASSOCIATE:
		if (m_CurEventItem.fSelected) {
			const ProgramGuide::CEventLayout *pLayout = m_EventLayoutList[m_CurEventItem.ListIndex];

			if (pLayout != nullptr) {
				const ProgramGuide::CEventItem *pEvent = pLayout->GetItem(m_CurEventItem.EventIndex);

				if (pEvent != nullptr) {
					ExecuteiEpgAssociate(pLayout->GetServiceInfo(), pEvent->GetEventInfo());
				}
			}
		}
		return;

	case CM_PROGRAMGUIDE_SEARCH:
		ShowProgramSearch(!m_ProgramSearch.IsVisible());
		return;

	case CM_PROGRAMGUIDE_CHANNELSETTINGS:
		{
			CEpgChannelSettings Settings(this);

			POINT pt = {0, 0};
			::ClientToScreen(m_hwnd, &pt);
			Settings.SetPosition(pt.x, pt.y, 0, 0);

			if (Settings.Show(m_hwnd))
				UpdateProgramGuide();
		}
		return;

	case CM_PROGRAMGUIDE_ALWAYSONTOP:
		if (m_pFrame != nullptr)
			m_pFrame->SetAlwaysOnTop(!m_pFrame->GetAlwaysOnTop());
		return;

	case CM_PROGRAMGUIDE_DRAGSCROLL:
		SetDragScroll(!m_fDragScroll);
		return;

	case CM_PROGRAMGUIDE_POPUPEVENTINFO:
		SetShowToolTip(!m_fShowToolTip);
		return;

	case CM_PROGRAMGUIDE_KEEPTIMEPOS:
		SetKeepTimePos(!m_fKeepTimePos);
		return;

	case CM_PROGRAMGUIDE_ADDTOFAVORITES:
		if (m_pChannelProvider != nullptr
				&& m_CurrentChannelGroup >= 0) {
			CProgramGuideFavorites::FavoriteInfo Info;
			TCHAR szText[256];

			m_pChannelProvider->GetName(szText, lengthof(szText));
			Info.Name = szText;
			m_pChannelProvider->GetGroupID(m_CurrentChannelGroup, &Info.GroupID);
			m_pChannelProvider->GetGroupName(m_CurrentChannelGroup, szText, lengthof(szText));
			Info.Label = szText;
			Info.SetDefaultColors();

			CProgramGuideFavoritesDialog Dialog(m_Favorites, m_Theme.FavoriteButtonStyle);
			Dialog.AddNewItem(Info);
			if (Dialog.Show(m_hwnd)) {
				m_Favorites = Dialog.GetFavorites();
				if (m_pFrame != nullptr)
					m_pFrame->OnFavoritesChanged();
			}
		}
		return;

	case CM_PROGRAMGUIDE_ORGANIZEFAVORITES:
		{
			CProgramGuideFavoritesDialog Dialog(m_Favorites, m_Theme.FavoriteButtonStyle);

			if (Dialog.Show(m_hwnd)) {
				m_Favorites = Dialog.GetFavorites();
				if (m_pFrame != nullptr)
					m_pFrame->OnFavoritesChanged();
			}
		}
		return;

	case CM_PROGRAMGUIDE_SHOWFEATUREDMARK:
		SetShowFeaturedMark(!m_fShowFeaturedMark);
		return;

	default:
		if (id >= CM_PROGRAMGUIDE_DAY_FIRST
				&& id <= CM_PROGRAMGUIDE_DAY_LAST) {
			SetViewDay(id - CM_PROGRAMGUIDE_DAY_FIRST);
			return;
		}

		if (id >= CM_PROGRAMGUIDE_FILTER_FIRST
				&& id <= CM_PROGRAMGUIDE_FILTER_LAST) {
			FilterFlag Filter = m_Filter ^ static_cast<FilterFlag>(1U << (id - CM_PROGRAMGUIDE_FILTER_FIRST));

			if (id == CM_PROGRAMGUIDE_FILTER_ORIGINAL) {
				if (!!(Filter & FilterFlag::Original))
					Filter &= ~FilterFlag::Rerun;
			} else if (id == CM_PROGRAMGUIDE_FILTER_RERUN) {
				if (!!(Filter & FilterFlag::Rerun))
					Filter &= ~FilterFlag::Original;
			}
			SetFilter(Filter);
			return;
		}

		if (id >= CM_PROGRAMGUIDE_CHANNELGROUP_FIRST
				&& id <= CM_PROGRAMGUIDE_CHANNELGROUP_LAST) {
			if (m_fEpgUpdating)
				OnCommand(CM_PROGRAMGUIDE_ENDUPDATE);
			StoreTimePos();
			if (SetCurrentChannelGroup(id - CM_PROGRAMGUIDE_CHANNELGROUP_FIRST))
				UpdateServiceList();
			return;
		}

		if (id >= CM_PROGRAMGUIDE_CHANNELPROVIDER_FIRST
				&& id <= CM_PROGRAMGUIDE_CHANNELPROVIDER_LAST) {
			if (m_fEpgUpdating)
				OnCommand(CM_PROGRAMGUIDE_ENDUPDATE);
			SetChannelProvider(id - CM_PROGRAMGUIDE_CHANNELPROVIDER_FIRST, 0);
			return;
		}

		if (id >= CM_PROGRAMGUIDE_CUSTOM_FIRST
				&& id <= CM_PROGRAMGUIDE_CUSTOM_LAST) {
			if (m_CurEventItem.fSelected) {
				if (m_pProgramCustomizer != nullptr) {
					const ProgramGuide::CEventItem *pEvent =
						GetEventItem(m_CurEventItem.ListIndex, m_CurEventItem.EventIndex);

					if (pEvent != nullptr)
						m_pProgramCustomizer->ProcessMenu(*pEvent->GetEventInfo(), id);
				}
			} else {
				if (m_pEventHandler != nullptr)
					m_pEventHandler->OnMenuSelected(id);
			}
			return;
		}

		if (id >= CM_PROGRAMGUIDETOOL_FIRST
				&& id <= CM_PROGRAMGUIDETOOL_LAST) {
			if (m_CurEventItem.fSelected) {
				const ProgramGuide::CEventLayout *pLayout = m_EventLayoutList[m_CurEventItem.ListIndex];

				if (pLayout != nullptr) {
					const ProgramGuide::CEventItem *pEvent = pLayout->GetItem(m_CurEventItem.EventIndex);

					if (pEvent != nullptr) {
						ExecuteTool(
							id - CM_PROGRAMGUIDETOOL_FIRST,
							pLayout->GetServiceInfo(), pEvent->GetEventInfo());
					}
				}
			}
			return;
		}

		if (id >= CM_PROGRAMGUIDE_FAVORITE_FIRST
				&& id <= CM_PROGRAMGUIDE_FAVORITE_LAST) {
			const CProgramGuideFavorites::FavoriteInfo *pInfo =
				m_Favorites.Get(id - CM_PROGRAMGUIDE_FAVORITE_FIRST);

			if (pInfo != nullptr) {
				TCHAR szName[256];

				for (int i = 0; EnumChannelProvider(i, szName, lengthof(szName)); i++) {
					if (::lstrcmpi(szName, pInfo->Name.c_str()) == 0) {
						SetChannelProvider(i, pInfo->GroupID.c_str());
						return;
					}
				}
			}
			return;
		}

		if (id >= CM_CHANNEL_FIRST && id <= CM_CHANNEL_LAST) {
			SetWeekListMode(id - CM_CHANNEL_FIRST);
			return;
		}
	}

	if (m_pFrame != nullptr)
		m_pFrame->OnCommand(id);
}


void CProgramGuide::ShowPopupMenu(int x, int y)
{
	POINT pt = {x, y};
	TCHAR szText[256], szMenu[64];

	CPopupMenu Menu(GetAppClass().GetResourceInstance(), IDM_PROGRAMGUIDE);
	const HMENU hmenuPopup = Menu.GetPopupHandle();

	Menu.CheckRadioItem(
		CM_PROGRAMGUIDE_DAY_FIRST,
		CM_PROGRAMGUIDE_DAY_LAST,
		CM_PROGRAMGUIDE_DAY_FIRST + m_Day);
	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STRING;
	mii.dwTypeData = szText;
	for (int i = CM_PROGRAMGUIDE_DAY_FIRST; i <= CM_PROGRAMGUIDE_DAY_LAST; i++) {
		LibISDB::DateTime Time;

		GetDayTimeRange(i - CM_PROGRAMGUIDE_DAY_FIRST, &Time, nullptr);
		mii.cch = lengthof(szText);
		::GetMenuItemInfo(hmenuPopup, i, FALSE, &mii);
		const int Length = ::lstrlen(szText);
		StringFormat(
			szText + Length, lengthof(szText) - Length, TEXT(" {}/{}({}) {}時～"),
			Time.Month, Time.Day, GetDayOfWeekText(Time.DayOfWeek), Time.Hour);
		::SetMenuItemInfo(hmenuPopup, i, FALSE, &mii);
	}

	const HMENU hmenuChannelGroup = ::GetSubMenu(hmenuPopup, MENU_CHANNELGROUP);
	ClearMenu(hmenuChannelGroup);
	int ChannelGroupCount = GetChannelGroupCount();
	if (ChannelGroupCount > MAX_CHANNEL_GROUP_MENU_ITEMS)
		ChannelGroupCount = MAX_CHANNEL_GROUP_MENU_ITEMS;
	for (int i = 0; i < ChannelGroupCount; i++) {
		if (GetChannelGroupName(i, szText, lengthof(szText))) {
			CopyToMenuText(szText, szMenu, lengthof(szMenu));
			::AppendMenu(
				hmenuChannelGroup, MF_STRING | MF_ENABLED,
				CM_PROGRAMGUIDE_CHANNELGROUP_FIRST + i, szMenu);
		}
	}
	if (::GetMenuItemCount(hmenuChannelGroup) > 0
			&& m_CurrentChannelGroup >= 0
			&& m_CurrentChannelGroup < ChannelGroupCount) {
		::CheckMenuRadioItem(
			hmenuChannelGroup,
			CM_PROGRAMGUIDE_CHANNELGROUP_FIRST,
			CM_PROGRAMGUIDE_CHANNELGROUP_FIRST + ChannelGroupCount - 1,
			CM_PROGRAMGUIDE_CHANNELGROUP_FIRST + m_CurrentChannelGroup,
			MF_BYCOMMAND);
	}
	if (m_pChannelProviderManager != nullptr) {
		int ProviderCount = static_cast<int>(m_pChannelProviderManager->GetChannelProviderCount());
		if (ProviderCount > 0) {
			if (ProviderCount > MAX_CHANNEL_PROVIDER_MENU_ITEMS)
				ProviderCount = MAX_CHANNEL_PROVIDER_MENU_ITEMS;
			::AppendMenu(hmenuChannelGroup, MF_SEPARATOR, 0, nullptr);
			for (int i = 0; i < ProviderCount; i++) {
				if (EnumChannelProvider(i, szText, lengthof(szText))) {
					CopyToMenuText(szText, szMenu, lengthof(szMenu));
					::AppendMenu(
						hmenuChannelGroup, MF_STRING | MF_ENABLED,
						CM_PROGRAMGUIDE_CHANNELPROVIDER_FIRST + i, szMenu);
				}
			}
			if (m_CurrentChannelProvider >= 0
					&& m_CurrentChannelProvider < ProviderCount) {
				::CheckMenuRadioItem(
					hmenuChannelGroup,
					CM_PROGRAMGUIDE_CHANNELPROVIDER_FIRST,
					CM_PROGRAMGUIDE_CHANNELPROVIDER_FIRST + ProviderCount - 1,
					CM_PROGRAMGUIDE_CHANNELPROVIDER_FIRST + m_CurrentChannelProvider,
					MF_BYCOMMAND);
			}
		}
	}
	Menu.EnableSubMenu(MENU_CHANNELGROUP, ::GetMenuItemCount(hmenuChannelGroup) > 0);

	for (int i = 0; (static_cast<unsigned int>(m_Filter) >> i) != 0; i++) {
		if (((static_cast<unsigned int>(m_Filter) >> i) & 1) != 0)
			Menu.CheckItem(CM_PROGRAMGUIDE_FILTER_FIRST + i, true);
	}

	Menu.CheckItem(CM_PROGRAMGUIDE_SEARCH, m_ProgramSearch.IsCreated());
	Menu.EnableItem(
		CM_PROGRAMGUIDE_CHANNELSETTINGS,
		m_pChannelProvider != nullptr &&
			m_pChannelProvider->GetChannelCount(m_CurrentChannelGroup) > 0);
	Menu.EnableItem(
		CM_PROGRAMGUIDE_UPDATE,
		!m_fEpgUpdating
			&& m_pChannelProvider != nullptr
			&& m_pChannelProvider->GetBonDriver(szText, lengthof(szText)));
	Menu.EnableItem(CM_PROGRAMGUIDE_ENDUPDATE, m_fEpgUpdating);
	Menu.CheckItem(CM_PROGRAMGUIDE_AUTOREFRESH, m_fAutoRefresh);
	Menu.CheckItem(CM_PROGRAMGUIDE_ALWAYSONTOP, m_pFrame->GetAlwaysOnTop());
	Menu.CheckItem(CM_PROGRAMGUIDE_DRAGSCROLL, m_fDragScroll);
	Menu.CheckItem(CM_PROGRAMGUIDE_POPUPEVENTINFO, m_fShowToolTip);
	Menu.CheckItem(CM_PROGRAMGUIDE_SHOWFEATUREDMARK, m_fShowFeaturedMark);
	Menu.CheckItem(CM_PROGRAMGUIDE_KEEPTIMEPOS, m_fKeepTimePos);
	Menu.EnableItem(CM_PROGRAMGUIDE_IEPGASSOCIATE, m_CurEventItem.fSelected);
	Menu.EnableItem(CM_PROGRAMGUIDE_ADDTOFAVORITES, m_pChannelProvider != nullptr);

	if (m_CurEventItem.fSelected) {
		if (m_pProgramCustomizer != nullptr) {
			const ProgramGuide::CEventItem *pItem =
				GetEventItem(m_CurEventItem.ListIndex, m_CurEventItem.EventIndex);

			if (pItem != nullptr) {
				RECT ItemRect;

				GetEventRect(m_CurEventItem.ListIndex, m_CurEventItem.EventIndex, &ItemRect);
				m_pProgramCustomizer->InitializeMenu(
					*pItem->GetEventInfo(),
					hmenuPopup, CM_PROGRAMGUIDE_CUSTOM_FIRST,
					pt, ItemRect);
			}
		}

		AppendToolMenu(hmenuPopup);
	} else {
		if (m_pEventHandler != nullptr) {
			m_pEventHandler->OnMenuInitialize(hmenuPopup, CM_PROGRAMGUIDE_CUSTOM_FIRST);
		}
	}

	if (m_pFrame != nullptr)
		m_pFrame->OnMenuInitialize(hmenuPopup);

	::ClientToScreen(m_hwnd, &pt);
	Menu.Show(m_hwnd, &pt);
}


void CProgramGuide::AppendToolMenu(HMENU hmenu) const
{
	if (m_ToolList.NumTools() > 0) {
		::AppendMenu(hmenu, MF_SEPARATOR | MF_ENABLED, 0, nullptr);
		for (size_t i = 0; i < m_ToolList.NumTools(); i++) {
			const CProgramGuideTool *pTool = m_ToolList.GetTool(i);
			TCHAR szText[256];

			CopyToMenuText(pTool->GetName(), szText, lengthof(szText));
			::AppendMenu(
				hmenu, MF_STRING | MF_ENABLED,
				CM_PROGRAMGUIDETOOL_FIRST + i, szText);
		}
	}
}


bool CProgramGuide::ExecuteiEpgAssociate(const ProgramGuide::CServiceInfo *pServiceInfo,
		const LibISDB::EventInfo *pEventInfo)
{
	if (pServiceInfo == nullptr || pEventInfo == nullptr)
		return false;

	TCHAR szFileName[MAX_PATH];
	GetAppClass().GetAppDirectory(szFileName);
	::PathAppend(szFileName, TEXT("iepg.tvpid"));
	if (!pServiceInfo->SaveiEpgFile(pEventInfo, szFileName, true))
		return false;

	return reinterpret_cast<INT_PTR>(::ShellExecute(nullptr, nullptr, szFileName, nullptr, nullptr, SW_SHOWNORMAL)) > 32;
}


bool CProgramGuide::ExecuteTool(
	int Tool,
	const ProgramGuide::CServiceInfo *pServiceInfo,
	const LibISDB::EventInfo *pEventInfo)
{
	if (pServiceInfo == nullptr || pEventInfo == nullptr)
		return false;

	CProgramGuideTool *pTool = m_ToolList.GetTool(Tool);
	if (pTool == nullptr)
		return false;

	return pTool->Execute(pServiceInfo, pEventInfo, ::GetAncestor(m_hwnd, GA_ROOT));
}


void CProgramGuide::ApplyStyle()
{
	if (m_hwnd != nullptr) {
		CreateFonts();
		CalcFontMetrics();

		m_ItemWidth = m_pStyleScaling->LogicalPixelsToPhysicalPixels(m_ItemLogicalWidth);
		m_ColumnWidth = m_ItemWidth + m_Style.ColumnMargin * 2;
		m_TextLeftMargin =
			m_Style.EventIconSize.Width +
			m_Style.EventIconMargin.Left + m_Style.EventIconMargin.Right;

		static const Theme::IconList::ResourceInfo ResourceList[] = {
			{MAKEINTRESOURCE(IDB_CHEVRON10), 10, 10},
			{MAKEINTRESOURCE(IDB_CHEVRON20), 20, 20},
		};
		m_Chevron.Load(
			m_hinst,
			m_Style.HeaderChevronSize.Width,
			m_Style.HeaderChevronSize.Height,
			ResourceList, lengthof(ResourceList));

		if (m_Tooltip.IsCreated())
			m_Tooltip.SetFont(m_ContentFont.GetHandle());
	}
}


void CProgramGuide::RealizeStyle()
{
	if (m_hwnd != nullptr) {
		UpdateLayout();
	}
}


void CProgramGuide::OnFeaturedEventsSettingsChanged(CFeaturedEvents &FeaturedEvents)
{
	if (m_fShowFeaturedMark) {
		m_FeaturedEventsMatcher.BeginMatching(FeaturedEvents.GetSettings());
		Invalidate();
	}
}




CProgramGuide::CEventHandler::~CEventHandler()
{
	if (m_pProgramGuide != nullptr)
		m_pProgramGuide->SetEventHandler(nullptr);
}


CProgramGuide::CFrame::~CFrame()
{
	if (m_pProgramGuide != nullptr)
		m_pProgramGuide->SetFrame(nullptr);
}


CProgramGuide::CProgramCustomizer::~CProgramCustomizer()
{
	if (m_pProgramGuide != nullptr)
		m_pProgramGuide->SetEventHandler(nullptr);
}


CProgramGuide::CEPGDatabaseEventListener::CEPGDatabaseEventListener(CProgramGuide *pProgramGuide)
	: m_pProgramGuide(pProgramGuide)
{
}


void CProgramGuide::CEPGDatabaseEventListener::OnServiceCompleted(
	LibISDB::EPGDatabase *pEPGDatabase,
	uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID,
	bool IsExtended)
{
	m_pProgramGuide->PostMessage(
		CProgramGuide::MESSAGE_REFRESHSERVICE,
		MAKEWPARAM(NetworkID, TransportStreamID),
		MAKELPARAM(ServiceID, IsExtended ? 1 : 0));
}


CProgramGuide::CEventInfoPopupHandler::CEventInfoPopupHandler(CProgramGuide *pProgramGuide)
	: m_pProgramGuide(pProgramGuide)
{
}


bool CProgramGuide::CEventInfoPopupHandler::HitTest(int x, int y, LPARAM *pParam)
{
	/*if (m_pProgramGuide->m_fShowToolTip)*/ {
		int List, Event;

		if (m_pProgramGuide->EventHitTest(x, y, &List, &Event)) {
			*pParam = MAKELONG(List, Event);
			return true;
		}
	}
	return false;
}


bool CProgramGuide::CEventInfoPopupHandler::ShowPopup(LPARAM Param, CEventInfoPopup *pPopup)
{
	const int List = LOWORD(Param), Event = HIWORD(Param);
	const ProgramGuide::CEventLayout *pLayout = m_pProgramGuide->m_EventLayoutList[List];
	if (pLayout != nullptr) {
		const ProgramGuide::CEventItem *pItem = pLayout->GetItem(Event);
		if (pItem != nullptr) {
			const LibISDB::EventInfo *pEventInfo;
			if (pItem->GetCommonEventInfo() != nullptr)
				pEventInfo = pItem->GetCommonEventInfo();
			else
				pEventInfo = pItem->GetEventInfo();
			/*
			if (pEventInfo->m_EventName.empty() && pEventInfo->m_bCommonEvent) {
				const CProgramGuideItem *pCommonItem = m_pProgramGuide->m_ServiceList.GetEventByIDs(
					pServiceInfo->GetTSID(),
					pEventInfo->m_CommonEventInfo.ServiceID,
					pEventInfo->m_CommonEventInfo.EventID);
				if (pCommonItem != nullptr)
					pEventInfo=&pCommonItem->GetEventInfo();
			}
			*/

			const ProgramGuide::CServiceInfo *pServiceInfo = pLayout->GetServiceInfo();
			int IconWidth, IconHeight;
			pPopup->GetPreferredIconSize(&IconWidth, &IconHeight);
			const HICON hIcon = GetAppClass().LogoManager.CreateLogoIcon(
				pServiceInfo->GetNetworkID(), pServiceInfo->GetServiceID(),
				IconWidth, IconHeight);

			RECT rc;
			m_pProgramGuide->GetEventRect(List, Event, &rc);
			POINT pt = {rc.left, rc.bottom};
			::ClientToScreen(m_pProgramGuide->m_hwnd, &pt);
			pPopup->GetDefaultPopupPosition(&rc);
			if (rc.top > pt.y) {
				rc.bottom = pt.y + (rc.bottom - rc.top);
				rc.top = pt.y;
			}

			if (!pPopup->Show(pEventInfo, &rc, hIcon, pServiceInfo->GetServiceName())) {
				if (hIcon != nullptr)
					::DestroyIcon(hIcon);
				return false;
			}

			return true;
		}
	}

	return false;
}


bool CProgramGuide::CEventInfoPopupHandler::OnMenuPopup(HMENU hmenu)
{
	::AppendMenu(hmenu, MF_SEPARATOR, 0, nullptr);
	::AppendMenu(
		hmenu, MF_STRING | (CEventInfoPopup::CEventHandler::m_pPopup->IsSelected() ? MF_ENABLED : MF_GRAYED),
		COMMAND_FIRST, TEXT("選択文字列を検索(&S)"));
	return true;
}


void CProgramGuide::CEventInfoPopupHandler::OnMenuSelected(int Command)
{
	String Text(CEventInfoPopup::CEventHandler::m_pPopup->GetSelectedText());
	if (!Text.empty()) {
		if (!m_pProgramGuide->m_ProgramSearch.IsCreated())
			m_pProgramGuide->SendMessage(WM_COMMAND, CM_PROGRAMGUIDE_SEARCH, 0);
		m_pProgramGuide->m_ProgramSearch.Search(Text.c_str());
	}
}




CProgramGuide::CProgramSearchEventHandler::CProgramSearchEventHandler(CProgramGuide *pProgramGuide)
	: m_pProgramGuide(pProgramGuide)
{
}


bool CProgramGuide::CProgramSearchEventHandler::OnSearch()
{
	// 番組の検索

	LibISDB::DateTime First;
	m_pProgramGuide->GetDayTimeRange(DAY_TODAY, &First, nullptr);

	for (size_t i = 0; i < m_pProgramGuide->m_ServiceList.NumServices(); i++) {
		const ProgramGuide::CServiceInfo *pServiceInfo = m_pProgramGuide->m_ServiceList.GetItem(i);
		int j;

		for (j = 0; j < pServiceInfo->NumEvents(); j++) {
			const LibISDB::EventInfo *pEventInfo = pServiceInfo->GetEvent(j);
			LibISDB::DateTime End;
			pEventInfo->GetEndTime(&End);
			if (End > First)
				break;
		}

		for (; j < pServiceInfo->NumEvents(); j++) {
			const LibISDB::EventInfo *pEventInfo = pServiceInfo->GetEvent(j);
			if (!pEventInfo->IsCommonEvent && Match(pEventInfo)) {
				AddSearchResult(
					new CSearchEventInfo(
						*pEventInfo,
						pServiceInfo->GetChannelInfo()));
			}
		}
	}

	if (m_pSearchDialog->GetSearchTarget() == SEARCH_TARGET_ALL) {
		// 全ての番組から検索
		const CAppMain &App = GetAppClass();
		CChannelList ServiceList;

		App.DriverManager.GetAllServiceList(&ServiceList);

		for (int i = 0; i < ServiceList.NumChannels(); i++) {
			const CTunerChannelInfo *pChInfo =
				static_cast<const CTunerChannelInfo*>(ServiceList.GetChannelInfo(i));

			if (m_pProgramGuide->m_ServiceList.GetItemByIDs(
						pChInfo->GetNetworkID(),
						pChInfo->GetTransportStreamID(),
						pChInfo->GetServiceID()) == nullptr) {
				m_pProgramGuide->m_pEPGDatabase->EnumEventsSortedByTime(
					pChInfo->GetNetworkID(),
					pChInfo->GetTransportStreamID(),
					pChInfo->GetServiceID(),
					&First, nullptr,
					[&](const LibISDB::EventInfo & Event) -> bool {
						if (!Event.IsCommonEvent && Match(&Event))
							AddSearchResult(new CSearchEventInfo(Event, *pChInfo));
						return true;
					});
			}
		}
	}

	return true;
}


void CProgramGuide::CProgramSearchEventHandler::OnEndSearch()
{
	if (m_pSearchDialog->GetHighlightResult())
		m_pProgramGuide->Invalidate();
}


bool CProgramGuide::CProgramSearchEventHandler::OnClose()
{
	if (m_pSearchDialog->GetHighlightResult())
		m_pProgramGuide->Invalidate();
	return true;
}


bool CProgramGuide::CProgramSearchEventHandler::OnLDoubleClick(
	const CSearchEventInfo *pEventInfo)
{
	// 検索結果の一覧のダブルクリック
	// TODO: 動作をカスタマイズできるようにする
	if (!m_pProgramGuide->IsExcludeService(
				pEventInfo->NetworkID,
				pEventInfo->TransportStreamID,
				pEventInfo->ServiceID))
		DoCommand(CM_PROGRAMGUIDE_JUMPEVENT, pEventInfo);
	return true;
}


bool CProgramGuide::CProgramSearchEventHandler::OnRButtonClick(
	const CSearchEventInfo *pEventInfo)
{
	// 検索結果の一覧の右クリックメニューを表示
	CPopupMenu Menu(GetAppClass().GetResourceInstance(), IDM_PROGRAMSEARCH);
	const HMENU hmenuPopup = Menu.GetPopupHandle();

	if (m_pProgramGuide->IsExcludeService(
				pEventInfo->NetworkID,
				pEventInfo->TransportStreamID,
				pEventInfo->ServiceID))
		Menu.EnableItem(CM_PROGRAMGUIDE_JUMPEVENT, false);

	if (m_pProgramGuide->m_pProgramCustomizer != nullptr) {
		const POINT pt = {50, 50};
		const RECT rc = {0, 0, 100, 100};
		m_pProgramGuide->m_pProgramCustomizer->InitializeMenu(
			*pEventInfo, hmenuPopup, CM_PROGRAMGUIDE_CUSTOM_FIRST, pt, rc);
	}

	m_pProgramGuide->AppendToolMenu(hmenuPopup);

	const int Command = Menu.Show(
		m_pProgramGuide->GetHandle(), nullptr, TPM_RETURNCMD | TPM_RIGHTBUTTON);
	if (Command > 0) {
		DoCommand(Command, pEventInfo);
	}

	return true;
}


void CProgramGuide::CProgramSearchEventHandler::OnHighlightChange(bool fHighlight)
{
	m_pProgramGuide->Invalidate();
}


static int FindChannelFromChannelProvider(
	const CProgramGuideChannelProvider *pChannelProvider,
	const CChannelInfo &ChannelInfo)
{
	const size_t GroupCount = pChannelProvider->GetGroupCount();

	for (size_t i = 0; i < GroupCount; i++) {
		const size_t ChannelCount = pChannelProvider->GetChannelCount(i);

		for (size_t j = 0; j < ChannelCount; j++) {
			const CChannelInfo *pChInfo = pChannelProvider->GetChannelInfo(i, j);

			if (pChInfo->GetNetworkID() == ChannelInfo.GetNetworkID()
					&& pChInfo->GetTransportStreamID() == ChannelInfo.GetTransportStreamID()
					&& pChInfo->GetServiceID() == ChannelInfo.GetServiceID()) {
				return static_cast<int>(i);
			}
		}
	}

	return -1;
}

void CProgramGuide::CProgramSearchEventHandler::DoCommand(
	int Command, const CSearchEventInfo *pEventInfo)
{
	const ProgramGuide::CServiceInfo *pServiceInfo =
		m_pProgramGuide->m_ServiceList.GetItemByIDs(
			pEventInfo->NetworkID, pEventInfo->TransportStreamID, pEventInfo->ServiceID);

	if (Command == CM_PROGRAMGUIDE_JUMPEVENT) {
		if (pServiceInfo != nullptr) {
			m_pProgramGuide->JumpEvent(*pEventInfo);
		} else if (m_pProgramGuide->m_pChannelProviderManager != nullptr) {
			if (m_pProgramGuide->m_pChannelProvider != nullptr) {
				const int Group = FindChannelFromChannelProvider(
					m_pProgramGuide->m_pChannelProvider, pEventInfo->GetChannelInfo());
				if (Group >= 0) {
					m_pProgramGuide->SetChannelProvider(
						m_pProgramGuide->GetCurrentChannelProvider(), Group);
					m_pProgramGuide->JumpEvent(*pEventInfo);
					return;
				}
			}

			const size_t ProviderCount = m_pProgramGuide->m_pChannelProviderManager->GetChannelProviderCount();
			for (size_t i = 0; i < ProviderCount; i++) {
				if (static_cast<int>(i) != m_pProgramGuide->GetCurrentChannelProvider()) {
					CProgramGuideChannelProvider *pChannelProvider =
						m_pProgramGuide->m_pChannelProviderManager->GetChannelProvider(i);
					pChannelProvider->Update();
					const int Group = FindChannelFromChannelProvider(
						pChannelProvider, pEventInfo->GetChannelInfo());
					if (Group >= 0) {
						m_pProgramGuide->SetChannelProvider(static_cast<int>(i), Group);
						m_pProgramGuide->JumpEvent(*pEventInfo);
						return;
					}
				}
			}
		}
	} else if (Command == CM_PROGRAMGUIDE_IEPGASSOCIATE) {
		if (pServiceInfo != nullptr) {
			m_pProgramGuide->ExecuteiEpgAssociate(pServiceInfo, pEventInfo);
		} else {
			ProgramGuide::CServiceInfo ServiceInfo(
				pEventInfo->GetChannelInfo(),
				pEventInfo->GetChannelInfo().GetTunerName());
			m_pProgramGuide->ExecuteiEpgAssociate(&ServiceInfo, pEventInfo);
		}
	} else if (Command >= CM_PROGRAMGUIDE_CUSTOM_FIRST
			&& Command <= CM_PROGRAMGUIDE_CUSTOM_LAST) {
		if (m_pProgramGuide->m_pProgramCustomizer != nullptr)
			m_pProgramGuide->m_pProgramCustomizer->ProcessMenu(*pEventInfo, Command);
	} else if (Command >= CM_PROGRAMGUIDETOOL_FIRST
			&& Command <= CM_PROGRAMGUIDETOOL_LAST) {
		const int Tool = Command - CM_PROGRAMGUIDETOOL_FIRST;
		if (pServiceInfo != nullptr) {
			m_pProgramGuide->ExecuteTool(Tool, pServiceInfo, pEventInfo);
		} else {
			ProgramGuide::CServiceInfo ServiceInfo(
				pEventInfo->GetChannelInfo(),
				pEventInfo->GetChannelInfo().GetTunerName());
			m_pProgramGuide->ExecuteTool(Tool, &ServiceInfo, pEventInfo);
		}
	}
}




void CProgramGuide::ProgramGuideStyle::SetStyle(const Style::CStyleManager *pStyleManager)
{
	*this = ProgramGuideStyle();
	pStyleManager->Get(TEXT("program-guide.column.margin"), &ColumnMargin);
	pStyleManager->Get(TEXT("program-guide.header.padding"), &HeaderPadding);
	pStyleManager->Get(TEXT("program-guide.header.channel-name.margin"), &HeaderChannelNameMargin);
	pStyleManager->Get(TEXT("program-guide.header.icon.margin"), &HeaderIconMargin);
	pStyleManager->Get(TEXT("program-guide.header.chevron"), &HeaderChevronSize);
	pStyleManager->Get(TEXT("program-guide.header.chevron.margin"), &HeaderChevronMargin);
	pStyleManager->Get(TEXT("program-guide.header.shadow.height"), &HeaderShadowHeight);
	pStyleManager->Get(TEXT("program-guide.event.leading"), &EventLeading);
	pStyleManager->Get(TEXT("program-guide.event.line-spacing"), &EventLineSpacing);
	pStyleManager->Get(TEXT("program-guide.event.justify"), &fEventJustify);
	pStyleManager->Get(TEXT("program-guide.event.padding"), &EventPadding);
	pStyleManager->Get(TEXT("program-guide.event.icon"), &EventIconSize);
	pStyleManager->Get(TEXT("program-guide.event.icon.margin"), &EventIconMargin);
	pStyleManager->Get(TEXT("program-guide.event.featured-mark.margin"), &FeaturedMarkMargin);
	pStyleManager->Get(TEXT("program-guide.event.highlight-border"), &HighlightBorder);
	pStyleManager->Get(TEXT("program-guide.event.selected-border"), &SelectedBorder);
	pStyleManager->Get(TEXT("program-guide.time-bar.padding"), &TimeBarPadding);
	pStyleManager->Get(TEXT("program-guide.time-bar.shadow.width"), &TimeBarShadowWidth);
	pStyleManager->Get(TEXT("program-guide.cur-time-line.width"), &CurTimeLineWidth);
	pStyleManager->Get(TEXT("program-guide.tool-bar.item.padding"), &ToolbarItemPadding);
}


void CProgramGuide::ProgramGuideStyle::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	pStyleScaling->ToPixels(&ColumnMargin);
	pStyleScaling->ToPixels(&HeaderPadding);
	pStyleScaling->ToPixels(&HeaderChannelNameMargin);
	pStyleScaling->ToPixels(&HeaderIconMargin);
	pStyleScaling->ToPixels(&HeaderChevronSize);
	pStyleScaling->ToPixels(&HeaderChevronMargin);
	pStyleScaling->ToPixels(&HeaderShadowHeight);
	pStyleScaling->ToPixels(&EventLeading);
	pStyleScaling->ToPixels(&EventLineSpacing);
	pStyleScaling->ToPixels(&EventPadding);
	pStyleScaling->ToPixels(&EventIconSize);
	pStyleScaling->ToPixels(&EventIconMargin);
	pStyleScaling->ToPixels(&FeaturedMarkMargin);
	pStyleScaling->ToPixels(&HighlightBorder);
	pStyleScaling->ToPixels(&SelectedBorder);
	pStyleScaling->ToPixels(&TimeBarPadding);
	pStyleScaling->ToPixels(&TimeBarShadowWidth);
	pStyleScaling->ToPixels(&CurTimeLineWidth);
	pStyleScaling->ToPixels(&ToolbarItemPadding);
}




namespace ProgramGuideBar
{

enum {
	STATUS_ITEM_TUNER,
	STATUS_ITEM_DATE,
	STATUS_ITEM_DATEPREV,
	STATUS_ITEM_DATENEXT
};


class CStatusItemBase
	: public CStatusItem
{
public:
	CStatusItemBase(int ID, const SizeValue &DefaultWidth)
		: CStatusItem(ID, DefaultWidth)
	{
	}

	void ApplyStyle() override
	{
		m_Width = m_pStatus->CalcItemPixelSize(m_DefaultWidth);
		m_ActualWidth = m_Width;
	}

protected:
	bool IsDarkMenu() const
	{
		return GetAppClass().StyleManager.IsUseDarkMenu() && IsDarkAppModeSupported() && IsDarkMode();
	}
};

class CProgramGuideTunerStatusItem
	: public CStatusItemBase
{
	CProgramGuide *m_pProgramGuide;
	CDropDownMenu m_Menu;

public:
	CProgramGuideTunerStatusItem(CProgramGuide *pProgramGuide)
		: CStatusItemBase(STATUS_ITEM_TUNER, SizeValue(14 * EM_FACTOR, SizeUnit::EM))
		, m_pProgramGuide(pProgramGuide)
	{
	}

	LPCTSTR GetIDText() const override { return TEXT("Tuner"); }
	LPCTSTR GetName() const override { return TEXT("チューナー"); }

	void Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags) override
	{
		TCHAR szText[256];

		if (m_pProgramGuide->GetChannelGroupName(m_pProgramGuide->GetCurrentChannelGroup(), szText, lengthof(szText)))
			DrawText(hdc, DrawRect, szText);
	}

	void OnFocus(bool fFocus) override
	{
		if (fFocus) {
			TCHAR szText[256];

			m_Menu.Clear();
			for (int i = 0; i < MAX_CHANNEL_GROUP_MENU_ITEMS
					&& m_pProgramGuide->GetChannelGroupName(i, szText, lengthof(szText)); i++)
				m_Menu.AppendItem(new CDropDownMenu::CItem(CM_PROGRAMGUIDE_CHANNELGROUP_FIRST + i, szText));
			m_Menu.AppendSeparator();
			for (int i = 0; i < MAX_CHANNEL_PROVIDER_MENU_ITEMS
					&& m_pProgramGuide->EnumChannelProvider(i, szText, lengthof(szText)); i++)
				m_Menu.AppendItem(new CDropDownMenu::CItem(CM_PROGRAMGUIDE_CHANNELPROVIDER_FIRST + i, szText));
			RECT rc;
			GetRect(&rc);
			POINT pt = {rc.left, rc.bottom};
			::ClientToScreen(m_pStatus->GetHandle(), &pt);
			m_Menu.SetDarkMode(IsDarkMenu());
			m_Menu.Show(
				GetParent(m_pStatus->GetHandle()), m_pProgramGuide->GetHandle(), &pt,
				CM_PROGRAMGUIDE_CHANNELGROUP_FIRST + m_pProgramGuide->GetCurrentChannelGroup(),
				0, m_pStyleScaling->GetDPI());
		} else {
			POINT pt;
			RECT rc;

			::GetCursorPos(&pt);
			if (!m_Menu.GetPosition(&rc) || !::PtInRect(&rc, pt))
				m_Menu.Hide();
		}
	}
};

class CListSelectStatusItem
	: public CStatusItemBase
{
	CProgramGuide *m_pProgramGuide;
	CDropDownMenu m_Menu;

	class CServiceMenuItem
		: public CDropDownMenu::CItem
	{
		static constexpr int m_LogoWidth = 16;
		static constexpr int m_LogoHeight = 9;
		static constexpr int m_LogoMargin = 2;

		HBITMAP m_hbmLogo;

		int GetWidth(HDC hdc) override
		{
			if (m_Width == 0) {
				m_Width = CItem::GetWidth(hdc) + (m_LogoWidth + m_LogoMargin);
			}
			return m_Width;
		}

		void Draw(HDC hdc, const RECT *pRect) override
		{
			RECT rc = *pRect;

			if (m_hbmLogo != nullptr) {
				DrawUtil::DrawBitmap(
					hdc,
					rc.left, rc.top + (rc.bottom - rc.top - m_LogoHeight) / 2,
					m_LogoWidth, m_LogoHeight, m_hbmLogo);
			}
			rc.left += m_LogoWidth + m_LogoMargin;
			CItem::Draw(hdc, &rc);
		}

	public:
		CServiceMenuItem(int Command, LPCTSTR pszText, HBITMAP hbmLogo)
			: CItem(Command, pszText)
			, m_hbmLogo(hbmLogo)
		{
		}
	};

public:
	CListSelectStatusItem(CProgramGuide *pProgramGuide)
		: CStatusItemBase(STATUS_ITEM_DATE, SizeValue(14 * EM_FACTOR, SizeUnit::EM))
		, m_pProgramGuide(pProgramGuide)
	{
	}

	LPCTSTR GetIDText() const override { return TEXT("Date"); }
	LPCTSTR GetName() const override { return TEXT("日時"); }

	void Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags) override
	{
		if (m_pProgramGuide->GetListMode() == CProgramGuide::ListMode::Services) {
			CProgramGuide::DateInfo Info;
			TCHAR szText[256];

			m_pProgramGuide->GetCurrentDateInfo(&Info);
			EpgUtil::EpgTimeToDisplayTime(&Info.BeginningTime);
			StringFormat(
				szText, TEXT("{}{}{}/{}({}) {}時～"),
				Info.pszRelativeDayText != nullptr ? Info.pszRelativeDayText : TEXT(""),
				Info.pszRelativeDayText != nullptr ? TEXT(" ") : TEXT(""),
				Info.BeginningTime.Month, Info.BeginningTime.Day,
				GetDayOfWeekText(Info.BeginningTime.DayOfWeek),
				Info.BeginningTime.Hour);
			DrawText(hdc, DrawRect, szText);
		} else {
			const ProgramGuide::CServiceInfo *pService =
				m_pProgramGuide->GetServiceList().GetItem(m_pProgramGuide->GetWeekListService());
			if (pService != nullptr)
				DrawText(hdc, DrawRect, pService->GetServiceName());
		}
	}

	void OnFocus(bool fFocus) override
	{
		if (fFocus) {
			int CurItem;

			m_Menu.Clear();
			if (m_pProgramGuide->GetListMode() == CProgramGuide::ListMode::Services) {
				for (int i = CProgramGuide::DAY_FIRST; i <= CProgramGuide::DAY_LAST; i++) {
					CProgramGuide::DateInfo Info;
					TCHAR szText[256];

					m_pProgramGuide->GetDateInfo(i, &Info);
					EpgUtil::EpgTimeToDisplayTime(&Info.BeginningTime);
					StringFormat(
						szText, TEXT("{}{}{}/{}({}) {}時～"),
						Info.pszRelativeDayText != nullptr ? Info.pszRelativeDayText : TEXT(""),
						Info.pszRelativeDayText != nullptr ? TEXT(" ") : TEXT(""),
						Info.BeginningTime.Month,
						Info.BeginningTime.Day,
						GetDayOfWeekText(Info.BeginningTime.DayOfWeek),
						Info.BeginningTime.Hour);
					m_Menu.AppendItem(new CDropDownMenu::CItem(CM_PROGRAMGUIDE_DAY_FIRST + i, szText));
				}
				CurItem = CM_PROGRAMGUIDE_DAY_FIRST + m_pProgramGuide->GetViewDay();
			} else {
				const ProgramGuide::CServiceList &ServiceList = m_pProgramGuide->GetServiceList();

				for (size_t i = 0; i < ServiceList.NumServices(); i++) {
					const ProgramGuide::CServiceInfo *pService = ServiceList.GetItem(i);

					m_Menu.AppendItem(
						new CServiceMenuItem(
							CM_CHANNEL_FIRST + static_cast<int>(i),
							pService->GetServiceName(),
							pService->GetLogo()));
				}
				CurItem = CM_CHANNEL_FIRST + m_pProgramGuide->GetWeekListService();
			}
			RECT rc;
			GetRect(&rc);
			POINT pt = {rc.left, rc.bottom};
			::ClientToScreen(m_pStatus->GetHandle(), &pt);
			m_Menu.SetDarkMode(IsDarkMenu());
			m_Menu.Show(
				GetParent(m_pStatus->GetHandle()), m_pProgramGuide->GetHandle(), &pt,
				CurItem, 0, m_pStyleScaling->GetDPI());
		} else {
			POINT pt;
			RECT rc;

			::GetCursorPos(&pt);
			if (!m_Menu.GetPosition(&rc) || !::PtInRect(&rc, pt))
				m_Menu.Hide();
		}
	}
};

class CListPrevStatusItem
	: public CStatusItemBase
{
	CProgramGuide *m_pProgramGuide;

public:
	CListPrevStatusItem(CProgramGuide *pProgramGuide)
		: CStatusItemBase(STATUS_ITEM_DATEPREV, SizeValue(1 * EM_FACTOR, SizeUnit::EM))
		, m_pProgramGuide(pProgramGuide)
	{
	}

	LPCTSTR GetIDText() const override { return TEXT("Prev"); }
	LPCTSTR GetName() const override { return TEXT("前へ"); }

	void Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags) override
	{
		bool fEnabled;

		if (m_pProgramGuide->GetListMode() == CProgramGuide::ListMode::Services)
			fEnabled = m_pProgramGuide->GetViewDay() > CProgramGuide::DAY_FIRST;
		else
			fEnabled = m_pProgramGuide->GetWeekListService() > 0;

		COLORREF OldTextColor;
		if (!fEnabled)
			OldTextColor = ::SetTextColor(hdc, MixColor(::GetTextColor(hdc), GetBkColor(hdc)));
		DrawText(hdc, DrawRect, TEXT("▲"), DrawTextFlag::HorizontalCenter | DrawTextFlag::NoEndEllipsis);
		if (!fEnabled)
			::SetTextColor(hdc, OldTextColor);
	}

	void OnLButtonDown(int x, int y) override
	{
		if (m_pProgramGuide->GetListMode() == CProgramGuide::ListMode::Services) {
			const int Day = m_pProgramGuide->GetViewDay();
			if (Day > CProgramGuide::DAY_FIRST)
				m_pProgramGuide->SendMessage(WM_COMMAND, CM_PROGRAMGUIDE_DAY_FIRST + Day - 1, 0);
		} else {
			m_pProgramGuide->SetWeekListMode(m_pProgramGuide->GetWeekListService() - 1);
		}
	}
};

class CListNextStatusItem
	: public CStatusItemBase
{
	CProgramGuide *m_pProgramGuide;

public:
	CListNextStatusItem(CProgramGuide *pProgramGuide)
		: CStatusItemBase(STATUS_ITEM_DATENEXT, SizeValue(1 * EM_FACTOR, SizeUnit::EM))
		, m_pProgramGuide(pProgramGuide)
	{
	}

	LPCTSTR GetIDText() const override { return TEXT("Next"); }
	LPCTSTR GetName() const override { return TEXT("次へ"); }

	void Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags) override
	{
		bool fEnabled;

		if (m_pProgramGuide->GetListMode() == CProgramGuide::ListMode::Services)
			fEnabled = m_pProgramGuide->GetViewDay() < CProgramGuide::DAY_LAST;
		else
			fEnabled = m_pProgramGuide->GetWeekListService() + 1 < static_cast<int>(m_pProgramGuide->GetServiceList().NumServices());

		COLORREF OldTextColor;
		if (!fEnabled)
			OldTextColor = ::SetTextColor(hdc, MixColor(::GetTextColor(hdc), GetBkColor(hdc)));
		DrawText(hdc, DrawRect, TEXT("▼"), DrawTextFlag::HorizontalCenter | DrawTextFlag::NoEndEllipsis);
		if (!fEnabled)
			::SetTextColor(hdc, OldTextColor);
	}

	void OnLButtonDown(int x, int y) override
	{
		if (m_pProgramGuide->GetListMode() == CProgramGuide::ListMode::Services) {
			const int Day = m_pProgramGuide->GetViewDay();
			if (Day < CProgramGuide::DAY_LAST)
				m_pProgramGuide->SendMessage(WM_COMMAND, CM_PROGRAMGUIDE_DAY_FIRST + Day + 1, 0);
		} else {
			m_pProgramGuide->SetWeekListMode(m_pProgramGuide->GetWeekListService() + 1);
		}
	}
};




CProgramGuideBar::CProgramGuideBar(CProgramGuide *pProgramGuide)
	: m_pProgramGuide(pProgramGuide)
{
}




class ABSTRACT_CLASS(CProgramGuideToolbar)
	: public CProgramGuideBar
	, public CBasicWindow
	, public CUIBase
{
public:
	CProgramGuideToolbar(CProgramGuide * pProgramGuide);
	virtual ~CProgramGuideToolbar();

// CBasicWindow
	bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;

// CProgramGuideBar
	bool CreateBar(HWND hwndParent, DWORD Style) override;
	bool IsBarCreated() const override;
	void DestroyBar() override;
	bool SetBarVisible(bool fVisible) override;
	void GetBarSize(SIZE * pSize) override;
	void SetBarPosition(int x, int y, int Width, int Height) override;
	CUIBase * GetUIBase() override;
	bool OnSetCursor(HWND hwnd, int HitTestCode) override;
	bool OnNotify(LPARAM lParam, LRESULT * pResult) override;

// CProgramGuideToolbar
	void SelectButton(int Button);
	void UnselectButton();

protected:
	CBufferedPaint m_BufferedPaint;
	Style::Margins m_Padding;
	DrawUtil::CFont m_Font;
	int m_FontHeight = 0;

	void AdjustSize();
	void DeleteAllButtons();
	void SetToolbarFont();

// CUIBase
	void ApplyStyle() override;
	void RealizeStyle() override;

	virtual void OnCreate() {}
	virtual void OnCustomDraw(NMTBCUSTOMDRAW *pnmtb, HDC hdc) = 0;
};


CProgramGuideToolbar::CProgramGuideToolbar(CProgramGuide *pProgramGuide)
	: CProgramGuideBar(pProgramGuide)
	, m_Padding(pProgramGuide->GetToolbarItemPadding())
{
}


CProgramGuideToolbar::~CProgramGuideToolbar()
{
	Destroy();
}


bool CProgramGuideToolbar::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	if (!CreateBasicWindow(
				hwndParent,
				Style | TBSTYLE_CUSTOMERASE | TBSTYLE_LIST | TBSTYLE_FLAT
					| CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN,
				ExStyle, ID,
				TOOLBARCLASSNAME, nullptr, GetAppClass().GetInstance()))
		return false;

	InitializeUI();

	::SendMessage(m_hwnd, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
	//::SendMessage(m_hwnd, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);
	// ボタンをアイコン無しにしてもなぜかアイコン分の幅がとられる
	::SendMessage(m_hwnd, TB_SETBITMAPSIZE, 0, MAKELONG(1, 15));

	SetToolbarFont();

	return true;
}


bool CProgramGuideToolbar::CreateBar(HWND hwndParent, DWORD Style)
{
	if (!Create(hwndParent, Style))
		return false;

	OnCreate();

	return true;
}


bool CProgramGuideToolbar::IsBarCreated() const
{
	return IsCreated();
}


void CProgramGuideToolbar::DestroyBar()
{
	Destroy();
}


bool CProgramGuideToolbar::SetBarVisible(bool fVisible)
{
	if (m_fVisible != fVisible) {
		if (IsCreated()) {
			SetVisible(fVisible);
		}
		m_fVisible = fVisible;
	}

	return true;
}


void CProgramGuideToolbar::GetBarSize(SIZE *pSize)
{
	if (m_hwnd != nullptr && static_cast<int>(::SendMessage(m_hwnd, TB_BUTTONCOUNT, 0, 0)) > 0) {
		::SendMessage(m_hwnd, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(pSize));
	} else {
		pSize->cx = 0;
		pSize->cy = 0;
	}
}


void CProgramGuideToolbar::SetBarPosition(int x, int y, int Width, int Height)
{
	SetPosition(x, y, Width, Height);
}


CUIBase *CProgramGuideToolbar::GetUIBase()
{
	return this;
}


bool CProgramGuideToolbar::OnSetCursor(HWND hwnd, int HitTestCode)
{
	if (hwnd == m_hwnd && HitTestCode == HTCLIENT) {
		::SetCursor(GetAppClass().UICore.GetActionCursor());
		return true;
	}

	return false;
}


bool CProgramGuideToolbar::OnNotify(LPARAM lParam, LRESULT *pResult)
{
	NMTBCUSTOMDRAW *pnmtb = reinterpret_cast<NMTBCUSTOMDRAW*>(lParam);

	if (pnmtb->nmcd.hdr.hwndFrom != m_hwnd
			|| pnmtb->nmcd.hdr.code != NM_CUSTOMDRAW)
		return false;

	switch (pnmtb->nmcd.dwDrawStage) {
	case CDDS_PREERASE:
		*pResult = CDRF_SKIPDEFAULT;
		break;

	case CDDS_PREPAINT:
		*pResult = CDRF_NOTIFYITEMDRAW;
		break;

	case CDDS_ITEMPREPAINT:
		{
			HDC hdc = pnmtb->nmcd.hdc;
			HDC hdcBuffer = nullptr;

			if (m_fUseBufferedPaint) {
				hdcBuffer = m_BufferedPaint.Begin(pnmtb->nmcd.hdc, &pnmtb->nmcd.rc);
				if (hdcBuffer != nullptr)
					hdc = hdcBuffer;
			}

			OnCustomDraw(pnmtb, hdc);

			if (hdcBuffer != nullptr) {
				//m_BufferedPaint.SetAlpha(224);
				m_BufferedPaint.SetOpaque();
				m_BufferedPaint.End();
			}
		}
		*pResult = CDRF_SKIPDEFAULT;
		break;

	default:
		*pResult = CDRF_DODEFAULT;
		break;
	}

	return true;
}


void CProgramGuideToolbar::SelectButton(int Button)
{
	if (m_hwnd != nullptr)
		::SendMessage(m_hwnd, TB_CHECKBUTTON, Button, TRUE);
}


void CProgramGuideToolbar::UnselectButton()
{
	if (m_hwnd != nullptr) {
		const int ButtonCount = static_cast<int>(::SendMessage(m_hwnd, TB_BUTTONCOUNT, 0, 0));

		for (int i = 0; i < ButtonCount; i++) {
			TBBUTTON tbb;

			::SendMessage(m_hwnd, TB_GETBUTTON, i, reinterpret_cast<LPARAM>(&tbb));
			::SendMessage(m_hwnd, TB_CHECKBUTTON, tbb.idCommand, FALSE);
		}
	}
}


void CProgramGuideToolbar::AdjustSize()
{
	RECT rcToolbar;
	GetPosition(&rcToolbar);
	SIZE sz;
	::SendMessage(m_hwnd, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&sz));
	SetPosition(rcToolbar.left, rcToolbar.top, sz.cx, sz.cy);
}


void CProgramGuideToolbar::DeleteAllButtons()
{
	if (m_hwnd != nullptr) {
		const int ButtonCount = static_cast<int>(::SendMessage(m_hwnd, TB_BUTTONCOUNT, 0, 0));

		for (int i = ButtonCount - 1; i >= 0; i--)
			::SendMessage(m_hwnd, TB_DELETEBUTTON, i, 0);
	}
}


void CProgramGuideToolbar::SetToolbarFont()
{
	SetWindowFont(m_hwnd, m_Font.GetHandle(), TRUE);
	::SendMessage(
		m_hwnd, TB_SETPADDING, 0,
		MAKELONG((m_Padding.Horz() + 1) / 2, (m_Padding.Vert() + 1) / 2));
}


void CProgramGuideToolbar::ApplyStyle()
{
	Style::Font Font;

	GetSystemFont(DrawUtil::FontType::Menu, &Font);
	CreateDrawFont(Font, &m_Font);
	const HDC hdc = ::GetDC(m_hwnd);
	m_FontHeight = m_Font.GetHeight(hdc, false);
	::ReleaseDC(m_hwnd, hdc);
}


void CProgramGuideToolbar::RealizeStyle()
{
	SetToolbarFont();
}




class ABSTRACT_CLASS(CStatusBar)
	: public CProgramGuideBar
	, public CUIBase
{
public:
	CStatusBar(CProgramGuide * pProgramGuide);
	virtual ~CStatusBar() = default;

// CProgramGuideBar
	bool CreateBar(HWND hwndParent, DWORD Style) override;
	bool IsBarCreated() const override;
	void DestroyBar() override;
	bool SetBarVisible(bool fVisible) override;
	void GetBarSize(SIZE * pSize) override;
	void SetBarPosition(int x, int y, int Width, int Height) override;
	void SetTheme(const ThemeInfo & Theme) override;
	CUIBase * GetUIBase() override;

protected:
	CStatusView m_StatusView;
};


CStatusBar::CStatusBar(CProgramGuide * pProgramGuide)
	: CProgramGuideBar(pProgramGuide)
{
	Style::Font Font;
	if (GetSystemFont(DrawUtil::FontType::Menu, &Font))
		m_StatusView.SetFont(Font);

	RegisterUIChild(&m_StatusView);
}


bool CStatusBar::CreateBar(HWND hwndParent, DWORD Style)
{
	m_StatusView.EnableBufferedPaint(m_fUseBufferedPaint);
	return m_StatusView.Create(hwndParent, Style);
}


bool CStatusBar::IsBarCreated() const
{
	return m_StatusView.IsCreated();
}


void CStatusBar::DestroyBar()
{
	m_StatusView.Destroy();
}


bool CStatusBar::SetBarVisible(bool fVisible)
{
	if (m_fVisible != fVisible) {
		if (m_StatusView.IsCreated())
			m_StatusView.SetVisible(fVisible);
		m_fVisible = fVisible;
	}

	return true;
}


void CStatusBar::GetBarSize(SIZE *pSize)
{
	pSize->cx = m_StatusView.GetIntegralWidth();
	pSize->cy = m_StatusView.GetHeight();
}


void CStatusBar::SetBarPosition(int x, int y, int Width, int Height)
{
	m_StatusView.SetPosition(x, y, Width, Height);
}


void CStatusBar::SetTheme(const ThemeInfo &Theme)
{
	m_StatusView.SetStatusViewTheme(Theme.StatusTheme);
}


CUIBase *CStatusBar::GetUIBase()
{
	return this;
}




class CTunerMenuBar
	: public CStatusBar
{
public:
	CTunerMenuBar(CProgramGuide *pProgramGuide);

	void OnSpaceChanged() override;
};


CTunerMenuBar::CTunerMenuBar(CProgramGuide *pProgramGuide)
	: CStatusBar(pProgramGuide)
{
	m_StatusView.AddItem(new CProgramGuideTunerStatusItem(pProgramGuide));
}


void CTunerMenuBar::OnSpaceChanged()
{
	m_StatusView.UpdateItem(STATUS_ITEM_TUNER);
}




class CDateMenuBar
	: public CStatusBar
{
public:
	CDateMenuBar(CProgramGuide *pProgramGuide);

	void OnDateChanged() override;
};


CDateMenuBar::CDateMenuBar(CProgramGuide *pProgramGuide)
	: CStatusBar(pProgramGuide)
{
	m_StatusView.AddItem(new CListSelectStatusItem(pProgramGuide));
	m_StatusView.AddItem(new CListPrevStatusItem(pProgramGuide));
	m_StatusView.AddItem(new CListNextStatusItem(pProgramGuide));
}


void CDateMenuBar::OnDateChanged()
{
	m_StatusView.UpdateItem(STATUS_ITEM_DATE);
	m_StatusView.UpdateItem(STATUS_ITEM_DATEPREV);
	m_StatusView.UpdateItem(STATUS_ITEM_DATENEXT);
}




class CFavoritesToolbar
	: public CProgramGuideToolbar
{
public:
	CFavoritesToolbar(CProgramGuide *pProgramGuide);
	~CFavoritesToolbar();
	void SetTheme(const ThemeInfo &Theme) override;
	void OnSpaceChanged() override;
	void OnFavoritesChanged() override;

private:
	void SetButtons();
	void OnCreate() override;
	void OnCustomDraw(NMTBCUSTOMDRAW *pnmtb, HDC hdc) override;
	void RealizeStyle() override;

	FavoriteButtonTheme m_ButtonTheme;
};


CFavoritesToolbar::CFavoritesToolbar(CProgramGuide *pProgramGuide)
	: CProgramGuideToolbar(pProgramGuide)
{
}


CFavoritesToolbar::~CFavoritesToolbar()
{
	Destroy();
}


void CFavoritesToolbar::SetTheme(const ThemeInfo &Theme)
{
	m_ButtonTheme = Theme.FavoriteButton;

	if (m_hwnd != nullptr)
		::InvalidateRect(m_hwnd, nullptr, TRUE);
}


void CFavoritesToolbar::OnSpaceChanged()
{
	const int CurChannelProvider = m_pProgramGuide->GetCurrentChannelProvider();
	TCHAR szName[256];
	int SelButton = 0;

	if (CurChannelProvider >= 0
			&& m_pProgramGuide->EnumChannelProvider(CurChannelProvider, szName, lengthof(szName))) {
		const int CurChannelGroup = m_pProgramGuide->GetCurrentChannelGroup();

		if (CurChannelGroup >= 0) {
			const CProgramGuideFavorites *pFavorites = m_pProgramGuide->GetFavorites();
			const int ButtonCount = static_cast<int>(::SendMessage(m_hwnd, TB_BUTTONCOUNT, 0, 0));

			for (int i = 0; i < ButtonCount; i++) {
				TBBUTTON tbb;

				if (::SendMessage(m_hwnd, TB_GETBUTTON, i, reinterpret_cast<LPARAM>(&tbb))) {
					const CProgramGuideFavorites::FavoriteInfo *pInfo = pFavorites->Get(tbb.dwData);

					if (pInfo != nullptr
							&& ::lstrcmpi(pInfo->Name.c_str(), szName) == 0
							&& m_pProgramGuide->ParseChannelGroupID(pInfo->GroupID.c_str()) == CurChannelGroup) {
						SelButton = tbb.idCommand;
						break;
					}
				}
			}
		}
	}

	if (SelButton > 0)
		SelectButton(SelButton);
	else
		UnselectButton();
}


void CFavoritesToolbar::OnFavoritesChanged()
{
	DeleteAllButtons();
	SetButtons();
	OnSpaceChanged();
}


void CFavoritesToolbar::SetButtons()
{
	const CProgramGuideFavorites *pFavorites = m_pProgramGuide->GetFavorites();
	const size_t Count = pFavorites->GetCount();

	TBBUTTON tbb;
	tbb.iBitmap = I_IMAGENONE;
	tbb.fsState = TBSTATE_ENABLED;
	tbb.fsStyle = BTNS_CHECKGROUP | BTNS_SHOWTEXT | BTNS_NOPREFIX;
	if (!pFavorites->GetFixedWidth())
		tbb.fsStyle |= BTNS_AUTOSIZE;
	tbb.idCommand = CM_PROGRAMGUIDE_FAVORITE_FIRST;

	for (size_t i = 0; i < Count && tbb.idCommand <= CM_PROGRAMGUIDE_FAVORITE_LAST; i++) {
		const CProgramGuideFavorites::FavoriteInfo *pInfo = pFavorites->Get(i);

		tbb.iString = reinterpret_cast<INT_PTR>(pInfo->Label.c_str());
		tbb.dwData = i;
		::SendMessage(m_hwnd, TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&tbb));
		tbb.idCommand++;
	}

	SIZE ButtonSize;
	ButtonSize.cx = 0;
	ButtonSize.cy = m_FontHeight + m_Padding.Horz();

	if (pFavorites->GetFixedWidth()) {
		const int ButtonCount = static_cast<int>(::SendMessage(m_hwnd, TB_BUTTONCOUNT, 0, 0));
		for (int i = 0; i < ButtonCount; i++) {
			RECT rc;
			::SendMessage(m_hwnd, TB_GETITEMRECT, i, reinterpret_cast<LPARAM>(&rc));
			if (ButtonSize.cx < rc.right - rc.left)
				ButtonSize.cx = rc.right - rc.left;
		}
	}

	::SendMessage(m_hwnd, TB_SETBUTTONSIZE, 0, MAKELONG(ButtonSize.cx, ButtonSize.cy));

	AdjustSize();
}


void CFavoritesToolbar::OnCreate()
{
	SetButtons();
}


void CFavoritesToolbar::OnCustomDraw(NMTBCUSTOMDRAW *pnmtb, HDC hdc)
{
	const CProgramGuideFavorites::FavoriteInfo *pInfo =
		m_pProgramGuide->GetFavorites()->Get(pnmtb->nmcd.lItemlParam);

	if (pInfo != nullptr) {
		Theme::CThemeDraw ThemeDraw(BeginThemeDraw(hdc));
		Theme::BackgroundStyle Style;
		const COLORREF BackColor = pInfo->BackColor;
		const COLORREF ShadowColor = MixColor(BackColor, RGB(0, 0, 0), 220);

		if ((pnmtb->nmcd.uItemState & (CDIS_CHECKED | CDIS_SELECTED)) != 0) {
			Style = m_ButtonTheme.CurStyle;
		} else if ((pnmtb->nmcd.uItemState & CDIS_HOT) != 0) {
			Style = m_ButtonTheme.HotStyle;
		} else {
			Style = m_ButtonTheme.Style;
		}

		if (Style.Fill.Type == Theme::FillType::Gradient) {
			if ((pnmtb->nmcd.uItemState & (CDIS_CHECKED | CDIS_HOT)) != 0) {
				Style.Fill.Gradient.Color1 = ShadowColor;
				Style.Fill.Gradient.Color2 = BackColor;
			} else {
				Style.Fill.Gradient.Color1 = BackColor;
				Style.Fill.Gradient.Color2 = ShadowColor;
			}
		} else {
			Style.Fill.Solid.Color = BackColor;
		}

		if (Style.Border.Type == Theme::BorderType::Sunken) {
			Style.Border.Color = ShadowColor;
		} else {
			Style.Border.Color = BackColor;
		}

		ThemeDraw.Draw(Style, pnmtb->nmcd.rc);

		const HFONT hfont = reinterpret_cast<HFONT>(::SendMessage(m_hwnd, WM_GETFONT, 0, 0));
		const HGDIOBJ hOldFont = ::SelectObject(hdc, hfont);
		const int OldBkMode = ::SetBkMode(hdc, TRANSPARENT);
		const COLORREF OldTextColor = ::SetTextColor(hdc, pInfo->TextColor);

		RECT rc = pnmtb->nmcd.rc;
		Style::Subtract(&rc, m_Padding);
		::DrawText(
			hdc, pInfo->Label.c_str(), -1, &rc,
			DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

		::SetTextColor(hdc, OldTextColor);
		::SetBkMode(hdc, OldBkMode);
		::SelectObject(hdc, hOldFont);
	}
}


void CFavoritesToolbar::RealizeStyle()
{
	CProgramGuideToolbar::RealizeStyle();
	OnFavoritesChanged();
}




class CDateToolbar
	: public CProgramGuideToolbar
{
public:
	static constexpr int MAX_BUTTON_COUNT = CProgramGuideFrameSettings::DATEBAR_MAXBUTTONCOUNT;

	CDateToolbar(CProgramGuide *pProgramGuide);
	~CDateToolbar();
	void SetTheme(const ThemeInfo &Theme) override;
	void OnDateChanged() override;
	void OnTimeRangeChanged() override;
	bool SetButtonCount(int Count);
	int GetButtonCount() const { return m_ButtonCount; }

private:
	static constexpr DWORD ITEM_FLAG_NOW = 0x80000000;

	int m_ButtonCount = CProgramGuideFrameSettings::DATEBAR_DEFAULTBUTTONCOUNT;
	DateButtonTheme m_Theme;

	bool SetButtons(const LibISDB::DateTime *pDateList, int Days, int FirstCommand);
	void OnCustomDraw(NMTBCUSTOMDRAW *pnmtb, HDC hdc) override;
	void RealizeStyle() override;
};


CDateToolbar::CDateToolbar(CProgramGuide *pProgramGuide)
	: CProgramGuideToolbar(pProgramGuide)
{
}


CDateToolbar::~CDateToolbar()
{
	Destroy();
}


void CDateToolbar::SetTheme(const ThemeInfo &Theme)
{
	m_Theme = Theme.DateButton;

	if (m_hwnd != nullptr)
		::InvalidateRect(m_hwnd, nullptr, TRUE);
}


void CDateToolbar::OnDateChanged()
{
	UnselectButton();
	if (m_pProgramGuide->GetListMode() == CProgramGuide::ListMode::Services)
		SelectButton(CM_PROGRAMGUIDE_DAY_FIRST + m_pProgramGuide->GetViewDay());
}


void CDateToolbar::OnTimeRangeChanged()
{
	LibISDB::DateTime DateList[MAX_BUTTON_COUNT];

	for (int i = 0; i < m_ButtonCount; i++) {
		LibISDB::DateTime Time;
		m_pProgramGuide->GetDayTimeRange(i, &Time, nullptr);
		EpgUtil::EpgTimeToDisplayTime(Time, &DateList[i]);
	}
	SetButtons(DateList, m_ButtonCount, CM_PROGRAMGUIDE_DAY_FIRST);
	SelectButton(CM_PROGRAMGUIDE_DAY_FIRST + m_pProgramGuide->GetViewDay());
}


bool CDateToolbar::SetButtonCount(int Count)
{
	if (Count < 1 || Count > MAX_BUTTON_COUNT)
		return false;

	m_ButtonCount = Count;

	return true;
}


bool CDateToolbar::SetButtons(const LibISDB::DateTime *pDateList, int Days, int FirstCommand)
{
	if (m_hwnd == nullptr)
		return false;

	DeleteAllButtons();

	TBBUTTON tbb;
	tbb.iBitmap = I_IMAGENONE;
	tbb.fsState = TBSTATE_ENABLED;
	tbb.fsStyle = BTNS_CHECKGROUP | BTNS_SHOWTEXT | BTNS_NOPREFIX;
	for (int i = 0; i < Days; i++) {
		const LibISDB::DateTime &Date = pDateList[i];
		TCHAR szText[32];

		tbb.idCommand = FirstCommand + i;
		StringFormat(
			szText,
			TEXT("{}/{}({})"),
			Date.Month, Date.Day, GetDayOfWeekText(Date.DayOfWeek));
		tbb.iString = reinterpret_cast<INT_PTR>(szText);
		tbb.dwData = (static_cast<DWORD>(Date.Month) << 16) | (static_cast<DWORD>(Date.Day) << 8) | Date.DayOfWeek;
		if (i == 0 && Days > 1 && Date.Day == pDateList[i + 1].Day)
			tbb.dwData |= ITEM_FLAG_NOW;
		::SendMessage(m_hwnd, TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&tbb));
	}

	SIZE ButtonSize = {0, m_FontHeight};
	const HDC hdc = ::GetDC(m_hwnd);
	const HFONT hfontOld = SelectFont(hdc, GetWindowFont(m_hwnd));
	for (int i = 0; i < Days; i++) {
		const LibISDB::DateTime &Date = pDateList[i];
		TCHAR szText[32];
		StringFormat(
			szText,
			TEXT("{:02}/{:02}({})"),	// {:02} にしているのは幅を揃えるため
			Date.Month, Date.Day, GetDayOfWeekText(Date.DayOfWeek));
		RECT rc = {0, 0, 0, 0};
		::DrawText(hdc, szText, -1, &rc, DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);
		if (rc.right > ButtonSize.cx)
			ButtonSize.cx = rc.right;
	}
	SelectFont(hdc, hfontOld);
	::ReleaseDC(m_hwnd, hdc);

	ButtonSize.cx += m_Padding.Horz();
	ButtonSize.cy += m_Padding.Vert();
	::SendMessage(m_hwnd, TB_SETBUTTONSIZE, 0, MAKELONG(ButtonSize.cx, ButtonSize.cy));

	AdjustSize();

	return true;
}


void CDateToolbar::OnCustomDraw(NMTBCUSTOMDRAW *pnmtb, HDC hdc)
{
	Theme::CThemeDraw ThemeDraw(BeginThemeDraw(hdc));

	Theme::Style Style;
	if ((pnmtb->nmcd.uItemState & CDIS_CHECKED) != 0) {
		Style = m_Theme.CurStyle;
	} else if ((pnmtb->nmcd.uItemState & CDIS_HOT) != 0) {
		Style = m_Theme.HotStyle;
	} else {
		Style = m_Theme.Style;
	}
	ThemeDraw.Draw(Style.Back, pnmtb->nmcd.rc);

	const int DayOfWeek = static_cast<int>(pnmtb->nmcd.lItemlParam & 0xFF);

	const HFONT hfont = reinterpret_cast<HFONT>(::SendMessage(m_hwnd, WM_GETFONT, 0, 0));
	const HGDIOBJ hOldFont = ::SelectObject(hdc, hfont);
	const int OldBkMode = ::SetBkMode(hdc, TRANSPARENT);
	const COLORREF OldTextColor =
		::SetTextColor(
			hdc,
			DayOfWeek == 0 ? m_Theme.SundayTextColor :
			DayOfWeek == 6 ? m_Theme.SaturdayTextColor :
			Style.Fore.Fill.GetSolidColor());

	TCHAR szText[32];
	if ((pnmtb->nmcd.lItemlParam & ITEM_FLAG_NOW) != 0) {
		StringCopy(szText, TEXT("今日"));
	} else {
		StringFormat(
			szText, TEXT("{}/{}({})"),
			static_cast<int>(pnmtb->nmcd.lItemlParam >> 16),
			static_cast<int>((pnmtb->nmcd.lItemlParam >> 8) & 0xFF),
			GetDayOfWeekText(DayOfWeek));
	}
	RECT rc = pnmtb->nmcd.rc;
	Style::Subtract(&rc, m_Padding);
	::DrawText(
		hdc, szText, -1, &rc,
		DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	::SetTextColor(hdc, OldTextColor);
	::SetBkMode(hdc, OldBkMode);
	::SelectObject(hdc, hOldFont);
}


void CDateToolbar::RealizeStyle()
{
	CProgramGuideToolbar::RealizeStyle();
	OnTimeRangeChanged();
}




class CTimeToolbar
	: public CProgramGuideToolbar
{
public:
	typedef CProgramGuideFrameSettings::TimeBarSettings TimeBarSettings;

	CTimeToolbar(CProgramGuide *pProgramGuide);
	~CTimeToolbar();
	void SetTheme(const ThemeInfo &Theme) override;
	void OnDateChanged() override { ChangeTime(); }
	void OnTimeRangeChanged() override { ChangeTime(); }
	void SetSettings(const TimeBarSettings &Settings);
	bool GetTimeByCommand(int Command, LibISDB::DateTime *pTime) const;

private:
	struct TimeInfo
	{
		WORD Hour;
		WORD Offset;
		int Command;
	};

	CProgramGuideFrameSettings::TimeBarSettings m_Settings;
	TimeButtonTheme m_TimeButtonTheme;
	DateButtonTheme m_DateButtonTheme;

	void ChangeTime();
	bool SetButtons(const TimeInfo *pTimeList, int TimeListLength);
	void OnCustomDraw(NMTBCUSTOMDRAW *pnmtb, HDC hdc) override;
	void RealizeStyle() override;
};


CTimeToolbar::CTimeToolbar(CProgramGuide *pProgramGuide)
	: CProgramGuideToolbar(pProgramGuide)
{
}


CTimeToolbar::~CTimeToolbar()
{
	Destroy();
}


void CTimeToolbar::SetTheme(const ThemeInfo &Theme)
{
	m_TimeButtonTheme = Theme.TimeButton;
	m_DateButtonTheme = Theme.DateButton;

	if (m_hwnd != nullptr)
		::InvalidateRect(m_hwnd, nullptr, TRUE);
}


void CTimeToolbar::SetSettings(const TimeBarSettings &Settings)
{
	m_Settings = Settings;
}


bool CTimeToolbar::GetTimeByCommand(int Command, LibISDB::DateTime *pTime) const
{
	TBBUTTONINFO tbbi;

	tbbi.cbSize = sizeof(tbbi);
	tbbi.dwMask = TBIF_LPARAM;
	if (::SendMessage(
				m_hwnd, TB_GETBUTTONINFO,
				Command, reinterpret_cast<LPARAM>(&tbbi)) < 0)
		return false;

	if (!m_pProgramGuide->GetCurrentTimeRange(pTime, nullptr))
		return false;

	pTime->OffsetHours(HIWORD(tbbi.lParam));

	return true;
}


void CTimeToolbar::ChangeTime()
{
	LibISDB::DateTime First, Last;

	if (m_pProgramGuide->GetCurrentTimeRange(&First, &Last)) {
		TimeInfo TimeList[(CM_PROGRAMGUIDE_TIME_LAST - CM_PROGRAMGUIDE_TIME_FIRST) + 2];
		TimeList[0].Hour = 0;
		TimeList[0].Offset = 0;
		TimeList[0].Command = CM_PROGRAMGUIDE_TIME_CURRENT;

		int i = 1;

		if (m_Settings.Time == TimeBarSettings::TimeType::Interval) {
			LibISDB::DateTime Time = First;

			for (; i < lengthof(TimeList) && i - 1 < m_Settings.MaxButtonCount && Time < Last; i++) {
				LibISDB::DateTime DispTime;

				EpgUtil::EpgTimeToDisplayTime(Time, &DispTime);
				TimeList[i].Hour = DispTime.Hour;
				TimeList[i].Offset = (i - 1) * m_Settings.Interval;
				TimeList[i].Command = CM_PROGRAMGUIDE_TIME_FIRST + (i - 1);
				Time.OffsetHours(m_Settings.Interval);
			}
		} else if (m_Settings.Time == TimeBarSettings::TimeType::Custom) {
			std::vector<String> Times;
			StringUtility::Split(m_Settings.CustomTime, _T(","), &Times);
			if (!Times.empty()) {
				std::vector<int> Hours;
				Hours.reserve(Times.size());
				for (const String &e : Times) {
					try {
						const int Hour = std::stoi(e, nullptr, 10);
						if (Hour >= 0)
							Hours.push_back(Hour);
					} catch (...) {
					}
				}
				if (!Hours.empty()) {
					LibISDB::InsertionSort(Hours);
					LibISDB::DateTime DispFirst;
					EpgUtil::EpgTimeToDisplayTime(First, &DispFirst);
					const int FirstHour = DispFirst.Hour;
					const int LastHour = FirstHour + static_cast<int>(Last.DiffSeconds(First)) / (60 * 60);
					size_t j = 0;
					for (; j < Hours.size(); j++) {
						if (Hours[j] >= FirstHour)
							break;
					}
					for (size_t k = 0; i < lengthof(TimeList) && i - 1 < m_Settings.MaxButtonCount && k < Hours.size(); k++) {
						if (j == Hours.size())
							j = 0;
						const int Hour = Hours[j];
						j++;
						if (Hour >= LastHour)
							break;
						int HourOffset = Hour;
						if (HourOffset < FirstHour) {
							HourOffset += 24;
							if (HourOffset < FirstHour || HourOffset >= LastHour)
								break;
						}
						HourOffset -= FirstHour;
						LibISDB::DateTime Time = DispFirst;
						Time.OffsetHours(HourOffset);
						if (EpgUtil::DisplayTimeToEpgTime(&Time)) {
							const int Diff = static_cast<int>(Time.DiffSeconds(First)) / (60 * 60);
							if (Diff >= 0) {
								TimeList[i].Hour = static_cast<WORD>(Hour);
								TimeList[i].Offset = static_cast<WORD>(Diff);
								TimeList[i].Command = CM_PROGRAMGUIDE_TIME_FIRST + (i - 1);
								i++;
							}
						}
					}
				}
			}
		}

		SetButtons(TimeList, i);
	}
}


bool CTimeToolbar::SetButtons(const TimeInfo *pTimeList, int TimeListLength)
{
	if (m_hwnd == nullptr)
		return false;

	DeleteAllButtons();

	TBBUTTON tbb;
	tbb.iBitmap = I_IMAGENONE;
	tbb.fsState = TBSTATE_ENABLED;
	tbb.fsStyle = BTNS_BUTTON | BTNS_SHOWTEXT | BTNS_NOPREFIX;
	for (int i = 0; i < TimeListLength; i++) {
		const TimeInfo &TimeInfo = pTimeList[i];
		TCHAR szText[32];

		tbb.idCommand = TimeInfo.Command;
		if (TimeInfo.Command == CM_PROGRAMGUIDE_TIME_CURRENT) {
			StringCopy(szText, TEXT("現在"));
		} else {
			StringFormat(szText, TEXT("{}時～"), TimeInfo.Hour);
		}
		tbb.iString = reinterpret_cast<INT_PTR>(szText);
		tbb.dwData = MAKELONG(TimeInfo.Hour, TimeInfo.Offset);
		::SendMessage(m_hwnd, TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&tbb));
	}

	SIZE ButtonSize = {0, m_FontHeight};
	const HDC hdc = ::GetDC(m_hwnd);
	const HFONT hfontOld = SelectFont(hdc, GetWindowFont(m_hwnd));
	for (int i = 0; i < TimeListLength; i++) {
		const TimeInfo &TimeInfo = pTimeList[i];
		TCHAR szText[32];
		StringFormat(
			szText,
			TEXT("{:02}時～"),	// {:02} にしているのは幅を揃えるため
			TimeInfo.Hour);
		RECT rc = {0, 0, 0, 0};
		::DrawText(hdc, szText, -1, &rc, DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);
		if (rc.right > ButtonSize.cx)
			ButtonSize.cx = rc.right;
	}
	SelectFont(hdc, hfontOld);
	::ReleaseDC(m_hwnd, hdc);

	ButtonSize.cx += m_Padding.Horz();
	ButtonSize.cy += m_Padding.Vert();
	::SendMessage(m_hwnd, TB_SETBUTTONSIZE, 0, MAKELONG(ButtonSize.cx, ButtonSize.cy));

	AdjustSize();

	return true;
}


void CTimeToolbar::OnCustomDraw(NMTBCUSTOMDRAW *pnmtb, HDC hdc)
{
	const bool fCurrent = pnmtb->nmcd.dwItemSpec == CM_PROGRAMGUIDE_TIME_CURRENT;
	Theme::CThemeDraw ThemeDraw(BeginThemeDraw(hdc));
	Theme::Style Style;
	int Hour;

	if (fCurrent) {
		if ((pnmtb->nmcd.uItemState & (CDIS_CHECKED | CDIS_SELECTED)) != 0) {
			Style = m_DateButtonTheme.CurStyle;
		} else if ((pnmtb->nmcd.uItemState & CDIS_HOT) != 0) {
			Style = m_DateButtonTheme.HotStyle;
		} else {
			Style = m_DateButtonTheme.Style;
		}
	} else {
		Hour = LOWORD(pnmtb->nmcd.lItemlParam);
		Style = m_TimeButtonTheme.TimeStyle[Hour / 3];
		if (Style.Back.Fill.Type == Theme::FillType::Gradient)
			Style.Back.Fill.Gradient.Rotate(Theme::GradientStyle::RotateType::Right);
		Style.Back.Border.Type =
			(pnmtb->nmcd.uItemState & (CDIS_CHECKED | CDIS_SELECTED)) != 0 ?
				m_TimeButtonTheme.CurBorderStyle.Type :
			(pnmtb->nmcd.uItemState & CDIS_HOT) != 0 ?
				m_TimeButtonTheme.HotBorderStyle.Type :
				m_TimeButtonTheme.BorderStyle.Type;
		Style.Back.Border.Color = Style.Back.Fill.GetSolidColor();
		if ((pnmtb->nmcd.uItemState & (CDIS_CHECKED | CDIS_HOT)) != 0
				&& Style.Back.Fill.Type == Theme::FillType::Gradient) {
			std::swap(
				Style.Back.Fill.Gradient.Color1,
				Style.Back.Fill.Gradient.Color2);
		}
	}

	ThemeDraw.Draw(Style.Back, pnmtb->nmcd.rc);

	const HFONT hfont = reinterpret_cast<HFONT>(::SendMessage(m_hwnd, WM_GETFONT, 0, 0));
	const HGDIOBJ hOldFont = ::SelectObject(hdc, hfont);
	const int OldBkMode = ::SetBkMode(hdc, TRANSPARENT);

	TCHAR szText[32];
	if (fCurrent) {
		StringCopy(szText, TEXT("現在"));
	} else {
		StringFormat(szText, TEXT("{}時～"), Hour);
	}
	RECT rc = pnmtb->nmcd.rc;
	Style::Subtract(&rc, m_Padding);
	ThemeDraw.Draw(
		Style.Fore, rc, szText,
		DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	::SetBkMode(hdc, OldBkMode);
	::SelectObject(hdc, hOldFont);
}


void CTimeToolbar::RealizeStyle()
{
	CProgramGuideToolbar::RealizeStyle();
	ChangeTime();
}


}	// namespace ProgramGuideBar




CProgramGuideFrameBase::CProgramGuideFrameBase(CProgramGuide *pProgramGuide, CProgramGuideFrameSettings *pSettings)
	: m_pProgramGuide(pProgramGuide)
	, m_pSettings(pSettings)
{
	m_ToolbarList[TOOLBAR_TUNER_MENU] = std::make_unique<ProgramGuideBar::CTunerMenuBar>(pProgramGuide);
	m_ToolbarList[TOOLBAR_DATE_MENU ] = std::make_unique<ProgramGuideBar::CDateMenuBar>(pProgramGuide);
	m_ToolbarList[TOOLBAR_FAVORITES ] = std::make_unique<ProgramGuideBar::CFavoritesToolbar>(pProgramGuide);
	m_ToolbarList[TOOLBAR_DATE      ] = std::make_unique<ProgramGuideBar::CDateToolbar>(pProgramGuide);
	m_ToolbarList[TOOLBAR_TIME      ] = std::make_unique<ProgramGuideBar::CTimeToolbar>(pProgramGuide);
}


void CProgramGuideFrameBase::SetTheme(const Theme::CThemeManager *pThemeManager)
{
	ProgramGuideBar::CProgramGuideBar::ThemeInfo Theme;

	CStatusView::GetStatusViewThemeFromThemeManager(pThemeManager, &Theme.StatusTheme);
	pThemeManager->GetBorderStyle(
		Theme::CThemeManager::STYLE_PROGRAMGUIDE_STATUS,
		&Theme.StatusTheme.Border);

	const Theme::ThemeColor TimeTextColor(pThemeManager->GetColor(CColorScheme::COLOR_PROGRAMGUIDE_TIMETEXT));
	for (int i = 0; i < CProgramGuide::TIME_BAR_BACK_COLORS; i++) {
		pThemeManager->GetFillStyle(
			Theme::CThemeManager::STYLE_PROGRAMGUIDE_TIMEBAR_0_2 + i,
			&Theme.TimeButton.TimeStyle[i].Back.Fill);
		Theme.TimeButton.TimeStyle[i].Fore.Fill.Type = Theme::FillType::Solid;
		Theme.TimeButton.TimeStyle[i].Fore.Fill.Solid.Color = TimeTextColor;
	}
	pThemeManager->GetBorderStyle(
		Theme::CThemeManager::STYLE_PROGRAMGUIDE_TIMEBUTTON,
		&Theme.TimeButton.BorderStyle);
	pThemeManager->GetBorderStyle(
		Theme::CThemeManager::STYLE_PROGRAMGUIDE_TIMEBUTTON_CUR,
		&Theme.TimeButton.CurBorderStyle);
	pThemeManager->GetBorderStyle(
		Theme::CThemeManager::STYLE_PROGRAMGUIDE_TIMEBUTTON_HOT,
		&Theme.TimeButton.HotBorderStyle);

	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_PROGRAMGUIDE_DATEBUTTON,
		&Theme.DateButton.Style);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_PROGRAMGUIDE_DATEBUTTON_CUR,
		&Theme.DateButton.CurStyle);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_PROGRAMGUIDE_DATEBUTTON_HOT,
		&Theme.DateButton.HotStyle);
	Theme.DateButton.SundayTextColor =
		pThemeManager->GetColor(CColorScheme::COLOR_PROGRAMGUIDE_DATEBUTTON_SUNDAYTEXT);
	Theme.DateButton.SaturdayTextColor =
		pThemeManager->GetColor(CColorScheme::COLOR_PROGRAMGUIDE_DATEBUTTON_SATURDAYTEXT);

	pThemeManager->GetBackgroundStyle(
		Theme::CThemeManager::STYLE_PROGRAMGUIDE_FAVORITEBUTTON,
		&Theme.FavoriteButton.Style);
	pThemeManager->GetBackgroundStyle(
		Theme::CThemeManager::STYLE_PROGRAMGUIDE_FAVORITEBUTTON_CUR,
		&Theme.FavoriteButton.CurStyle);
	pThemeManager->GetBackgroundStyle(
		Theme::CThemeManager::STYLE_PROGRAMGUIDE_FAVORITEBUTTON_HOT,
		&Theme.FavoriteButton.HotStyle);

	for (const auto &e : m_ToolbarList)
		e->SetTheme(Theme);
}


bool CProgramGuideFrameBase::SetToolbarVisible(int Toolbar, bool fVisible)
{
	if (Toolbar < 0 || Toolbar >= TOOLBAR_NUM)
		return false;

	ProgramGuideBar::CProgramGuideBar *pBar = m_ToolbarList[Toolbar].get();

	if (pBar->IsBarVisible() != fVisible) {
		if (pBar->IsBarCreated()) {
			if (fVisible) {
				// 一瞬変な位置に表示されるのを防ぐために見えない位置に移動
				SIZE sz;
				pBar->GetBarSize(&sz);
				pBar->SetBarPosition(-sz.cx, -sz.cy, sz.cx, sz.cy);
			}
			pBar->SetBarVisible(fVisible);
			if (!m_fNoUpdateLayout)
				OnLayoutChange();
		} else {
			pBar->SetBarVisible(fVisible);
		}
	}

	m_pSettings->SetToolbarVisible(Toolbar, fVisible);

	return true;
}


bool CProgramGuideFrameBase::GetToolbarVisible(int Toolbar) const
{
	if (Toolbar < 0 || Toolbar >= TOOLBAR_NUM)
		return false;

	return m_ToolbarList[Toolbar]->IsBarVisible();
}


void CProgramGuideFrameBase::OnDateChanged()
{
	for (const auto &e : m_ToolbarList)
		e->OnDateChanged();
}


void CProgramGuideFrameBase::OnSpaceChanged()
{
	for (const auto &e : m_ToolbarList)
		e->OnSpaceChanged();
}


void CProgramGuideFrameBase::OnListModeChanged()
{
	OnDateChanged();
}


void CProgramGuideFrameBase::OnTimeRangeChanged()
{
	for (const auto &e : m_ToolbarList)
		e->OnTimeRangeChanged();

	OnLayoutChange();
}


void CProgramGuideFrameBase::OnFavoritesChanged()
{
	for (const auto &e : m_ToolbarList)
		e->OnFavoritesChanged();

	OnLayoutChange();
}


bool CProgramGuideFrameBase::OnCommand(int Command)
{
	switch (Command) {
	case CM_PROGRAMGUIDE_TOOLBAR_TUNERMENU:
		SetToolbarVisible(TOOLBAR_TUNER_MENU, !m_ToolbarList[TOOLBAR_TUNER_MENU]->IsBarVisible());
		return true;

	case CM_PROGRAMGUIDE_TOOLBAR_DATEMENU:
		SetToolbarVisible(TOOLBAR_DATE_MENU, !m_ToolbarList[TOOLBAR_DATE_MENU]->IsBarVisible());
		return true;

	case CM_PROGRAMGUIDE_TOOLBAR_FAVORITES:
		SetToolbarVisible(TOOLBAR_FAVORITES, !m_ToolbarList[TOOLBAR_FAVORITES]->IsBarVisible());
		return true;

	case CM_PROGRAMGUIDE_TOOLBAR_DATE:
		SetToolbarVisible(TOOLBAR_DATE, !m_ToolbarList[TOOLBAR_DATE]->IsBarVisible());
		return true;

	case CM_PROGRAMGUIDE_TOOLBAR_TIME:
		SetToolbarVisible(TOOLBAR_TIME, !m_ToolbarList[TOOLBAR_TIME]->IsBarVisible());
		return true;

	case CM_PROGRAMGUIDE_TIME_CURRENT:
		if (m_pProgramGuide->GetListMode() == CProgramGuide::ListMode::Services)
			m_pProgramGuide->SetViewDay(CProgramGuide::DAY_TODAY);
		m_pProgramGuide->ScrollToCurrentTime();
		return true;

	case CM_PROGRAMGUIDE_TOOLBAROPTIONS:
		{
			CProgramGuideToolbarOptions Options(*m_pSettings);

			if (Options.Show(m_pProgramGuide->GetHandle())) {
				m_fNoUpdateLayout = true;

				for (int i = 0; i < TOOLBAR_NUM; i++)
					SetToolbarVisible(i, m_pSettings->GetToolbarVisible(i));

				ProgramGuideBar::CDateToolbar *pDateToolbar =
					static_cast<ProgramGuideBar::CDateToolbar*>(m_ToolbarList[TOOLBAR_DATE].get());
				if (pDateToolbar->GetButtonCount() != m_pSettings->GetDateBarButtonCount()) {
					pDateToolbar->SetButtonCount(m_pSettings->GetDateBarButtonCount());
					pDateToolbar->OnTimeRangeChanged();
				}

				ProgramGuideBar::CTimeToolbar *pTimeToolbar =
					static_cast<ProgramGuideBar::CTimeToolbar*>(m_ToolbarList[TOOLBAR_TIME].get());
				pTimeToolbar->SetSettings(m_pSettings->GetTimeBarSettings());
				pTimeToolbar->OnTimeRangeChanged();

				m_fNoUpdateLayout = false;
				OnLayoutChange();
			}
		}
		return true;
	}

	if (Command >= CM_PROGRAMGUIDE_TIME_FIRST
			&& Command <= CM_PROGRAMGUIDE_TIME_LAST) {
		const ProgramGuideBar::CTimeToolbar *pTimeToolbar =
			static_cast<const ProgramGuideBar::CTimeToolbar*>(m_ToolbarList[TOOLBAR_TIME].get());
		LibISDB::DateTime Time;

		if (pTimeToolbar->GetTimeByCommand(Command, &Time))
			m_pProgramGuide->ScrollToTime(Time);
		return true;
	}

	return false;
}


void CProgramGuideFrameBase::OnMenuInitialize(HMENU hmenu)
{
	::CheckMenuItem(
		hmenu, CM_PROGRAMGUIDE_TOOLBAR_TUNERMENU,
		MF_BYCOMMAND | (m_ToolbarList[TOOLBAR_TUNER_MENU]->IsBarVisible() ? MF_CHECKED : MF_UNCHECKED));
	::CheckMenuItem(
		hmenu, CM_PROGRAMGUIDE_TOOLBAR_DATEMENU,
		MF_BYCOMMAND | (m_ToolbarList[TOOLBAR_DATE_MENU]->IsBarVisible() ? MF_CHECKED : MF_UNCHECKED));
	::CheckMenuItem(
		hmenu, CM_PROGRAMGUIDE_TOOLBAR_FAVORITES,
		MF_BYCOMMAND | (m_ToolbarList[TOOLBAR_FAVORITES]->IsBarVisible() ? MF_CHECKED : MF_UNCHECKED));
	::CheckMenuItem(
		hmenu, CM_PROGRAMGUIDE_TOOLBAR_DATE,
		MF_BYCOMMAND | (m_ToolbarList[TOOLBAR_DATE]->IsBarVisible() ? MF_CHECKED : MF_UNCHECKED));
	::CheckMenuItem(
		hmenu, CM_PROGRAMGUIDE_TOOLBAR_TIME,
		MF_BYCOMMAND | (m_ToolbarList[TOOLBAR_TIME]->IsBarVisible() ? MF_CHECKED : MF_UNCHECKED));
}


void CProgramGuideFrameBase::OnWindowCreate(
	HWND hwnd, Style::CStyleScaling *pStyleScaling, bool fBufferedPaint)
{
	CUIBase *pUIBase = GetUIBase();

	for (const auto &e : m_ToolbarList)
		pUIBase->RegisterUIChild(e->GetUIBase());
	pUIBase->SetStyleScaling(pStyleScaling);

	m_pProgramGuide->SetFrame(this);
	m_pProgramGuide->SetStyleScaling(pStyleScaling);
	m_pProgramGuide->Create(hwnd, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL);

	pUIBase->RegisterUIChild(m_pProgramGuide);

	for (int i = 0; i < lengthof(m_ToolbarList); i++) {
		ProgramGuideBar::CProgramGuideBar *pBar = m_ToolbarList[i].get();

		pBar->SetBarVisible(m_pSettings->GetToolbarVisible(i));
		pBar->EnableBufferedPaint(fBufferedPaint);
		pBar->CreateBar(hwnd, WS_CHILD | (pBar->IsBarVisible() ? WS_VISIBLE : 0));
	}

	ProgramGuideBar::CDateToolbar *pDateToolbar =
		static_cast<ProgramGuideBar::CDateToolbar*>(m_ToolbarList[TOOLBAR_DATE].get());
	pDateToolbar->SetButtonCount(m_pSettings->GetDateBarButtonCount());

	ProgramGuideBar::CTimeToolbar *pTimeToolbar =
		static_cast<ProgramGuideBar::CTimeToolbar*>(m_ToolbarList[TOOLBAR_TIME].get());
	pTimeToolbar->SetSettings(m_pSettings->GetTimeBarSettings());
}


void CProgramGuideFrameBase::OnWindowDestroy()
{
	GetUIBase()->ClearUIChildList();

	m_pProgramGuide->SetFrame(nullptr);

	for (const auto &e : m_ToolbarList)
		e->DestroyBar();
}


void CProgramGuideFrameBase::OnSizeChanged(int Width, int Height)
{
	int OrderList[TOOLBAR_NUM];
	m_pSettings->GetToolbarOrderList(OrderList);

	const int ToolbarAreaWidth = Width - m_FrameStyle.ToolbarMargin.Right - m_ToolbarRightMargin;
	int x = m_FrameStyle.ToolbarMargin.Left;
	int y = m_FrameStyle.ToolbarMargin.Top;
	int BarHeight = 0;

	for (int i = 0; i < lengthof(m_ToolbarList); i++) {
		ProgramGuideBar::CProgramGuideBar *pBar = m_ToolbarList[OrderList[i]].get();

		if (pBar->IsBarVisible()) {
			SIZE sz;

			pBar->GetBarSize(&sz);
			if (x + sz.cx > ToolbarAreaWidth) {
				if (i > 0) {
					x = m_FrameStyle.ToolbarMargin.Left;
					y += BarHeight + m_FrameStyle.ToolbarVertGap;
				}
				if (x + sz.cx > ToolbarAreaWidth)
					sz.cx = std::max(ToolbarAreaWidth - x, 0);
				BarHeight = sz.cy;
			} else {
				if (sz.cy > BarHeight)
					BarHeight = sz.cy;
			}
			pBar->SetBarPosition(x, y, sz.cx, sz.cy);
			x += sz.cx + m_FrameStyle.ToolbarHorzGap;
		}
	}

	if (BarHeight > 0)
		y += BarHeight + m_FrameStyle.ToolbarMargin.Bottom;
	else
		y = 0;
	m_pProgramGuide->SetPosition(0, y, Width, std::max(Height - y, 0));
}


LRESULT CProgramGuideFrameBase::DefaultMessageHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_SIZE:
		OnSizeChanged(LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) {
			::SendMessage(hwnd, WM_CLOSE, 0, 0);
			return 0;
		}
		[[fallthrough]];
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		return m_pProgramGuide->SendMessage(uMsg, wParam, lParam);

	case WM_RBUTTONUP:
		{
			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			::ClientToScreen(hwnd, &pt);

			CPopupMenu Menu(GetAppClass().GetResourceInstance(), IDM_PROGRAMGUIDETOOLBAR);

			OnMenuInitialize(Menu.GetPopupHandle());

			Menu.Show(m_pProgramGuide->GetHandle(), &pt);
		}
		return 0;

	case WM_SETCURSOR:
		{
			const HWND hwndCursor = reinterpret_cast<HWND>(wParam);
			const int HitTestCode = LOWORD(lParam);

			for (const auto &e : m_ToolbarList) {
				if (e->OnSetCursor(hwndCursor, HitTestCode))
					return TRUE;
			}
		}
		break;

	case WM_NOTIFY:
		for (const auto &e : m_ToolbarList) {
			LRESULT Result;
			if (e->OnNotify(lParam, &Result))
				return Result;
		}
		break;

	case WM_COMMAND:
		return m_pProgramGuide->SendMessage(WM_COMMAND, wParam, lParam);

	case WM_SHOWWINDOW:
		m_pProgramGuide->OnShowFrame(wParam != FALSE);
		break;

	case WM_DESTROY:
		OnWindowDestroy();
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}




void CProgramGuideFrameBase::FrameStyle::SetStyle(const Style::CStyleManager *pStyleManager)
{
	*this = FrameStyle();
	pStyleManager->Get(TEXT("program-guide.tool-bar.margin"), &ToolbarMargin);
	pStyleManager->Get(TEXT("program-guide.tool-bar.horz-gap"), &ToolbarHorzGap);
	pStyleManager->Get(TEXT("program-guide.tool-bar.vert-gap"), &ToolbarVertGap);
	pStyleManager->Get(TEXT("program-guide.extend-frame"), &fExtendFrame);
	pStyleManager->Get(TEXT("program-guide.allow-dark-mode"), &fAllowDarkMode);
}


void CProgramGuideFrameBase::FrameStyle::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	pStyleScaling->ToPixels(&ToolbarMargin);
	pStyleScaling->ToPixels(&ToolbarHorzGap);
	pStyleScaling->ToPixels(&ToolbarVertGap);
}




const CProgramGuideFrameSettings::ToolbarInfo
CProgramGuideFrameSettings::m_ToolbarInfoList[TOOLBAR_NUM] =
{
	{TEXT("TunerMenu"), TEXT("チューナーメニュー")},
	{TEXT("DateMenu"),  TEXT("日付メニュー")},
	{TEXT("Favorites"), TEXT("番組表選択ボタン")},
	{TEXT("DateBar"),   TEXT("日付バー")},
	{TEXT("TimeBar"),   TEXT("時刻バー")},
};


CProgramGuideFrameSettings::CProgramGuideFrameSettings()
	: CSettingsBase(TEXT("ProgramGuide"))
	, m_DateBarButtonCount(DATEBAR_DEFAULTBUTTONCOUNT)
{
	for (int i = 0; i < lengthof(m_ToolbarSettingsList); i++) {
		m_ToolbarSettingsList[i].fVisible = true;
		m_ToolbarSettingsList[i].Order = i;
	}
}


bool CProgramGuideFrameSettings::ReadSettings(CSettings &Settings)
{
	int OrderList[TOOLBAR_NUM];
	int Count = 0;

	for (int i = 0; i < TOOLBAR_NUM; i++) {
		TCHAR szText[32], szName[32];
		int ID;

		StringFormat(szText, TEXT("Toolbar{}_Name"), i);
		if (Settings.Read(szText, szName, lengthof(szName))) {
			ID = ParseIDText(szName);
			if (ID < 0)
				continue;
		} else {
			// ver.0.9.0 より前との互換用
			ID = i;
		}

		int j;
		for (j = 0; j < Count; j++) {
			if (OrderList[j] == ID)
				break;
		}
		if (j < Count)
			continue;

		StringFormat(szText, TEXT("Toolbar{}_Status"), i);
		unsigned int Status;
		if (Settings.Read(szText, &Status)) {
			m_ToolbarSettingsList[ID].fVisible = (Status & TOOLBAR_STATUS_VISIBLE) != 0;
		}

		OrderList[Count] = ID;
		Count++;
	}

	if (Count < TOOLBAR_NUM) {
		for (int i = 0; i < TOOLBAR_NUM; i++) {
			int j;
			for (j = 0; j < Count; j++) {
				if (OrderList[j] == i)
					break;
			}
			if (j == Count) {
				OrderList[Count] = i;
				Count++;
			}
		}
	}

	SetToolbarOrderList(OrderList);

	int Value;

	if (Settings.Read(TEXT("DateBar.ButtonCount"), &Value))
		m_DateBarButtonCount = std::clamp(Value, 1, DATEBAR_MAXBUTTONCOUNT);

	if (Settings.Read(TEXT("TimeBar.TimeType"), &Value)
			&& CheckEnumRange(static_cast<TimeBarSettings::TimeType>(Value)))
		m_TimeBarSettings.Time = static_cast<TimeBarSettings::TimeType>(Value);
	if (Settings.Read(TEXT("TimeBar.Interval"), &Value)
			&& Value >= TimeBarSettings::INTERVAL_MIN
			&& Value <= TimeBarSettings::INTERVAL_MAX)
		m_TimeBarSettings.Interval = Value;
	Settings.Read(TEXT("TimeBar.Custom"), &m_TimeBarSettings.CustomTime);
	if (Settings.Read(TEXT("TimeBar.MaxButtonCount"), &Value)
			&& Value >= TimeBarSettings::BUTTONCOUNT_MIN
			&& Value <= TimeBarSettings::BUTTONCOUNT_MAX)
		m_TimeBarSettings.MaxButtonCount = Value;

	return true;
}


bool CProgramGuideFrameSettings::WriteSettings(CSettings &Settings)
{
	int OrderList[TOOLBAR_NUM];

	GetToolbarOrderList(OrderList);

	for (int i = 0; i < TOOLBAR_NUM; i++) {
		const int ID = OrderList[i];
		TCHAR szText[32];

		StringFormat(szText, TEXT("Toolbar{}_Name"), i);
		Settings.Write(szText, m_ToolbarInfoList[ID].pszIDText);

		StringFormat(szText, TEXT("Toolbar{}_Status"), i);
		unsigned int Status = 0;
		if (m_ToolbarSettingsList[ID].fVisible)
			Status |= TOOLBAR_STATUS_VISIBLE;
		Settings.Write(szText, Status);
	}

	Settings.Write(TEXT("DateBar.ButtonCount"), m_DateBarButtonCount);

	Settings.Write(TEXT("TimeBar.TimeType"), static_cast<int>(m_TimeBarSettings.Time));
	Settings.Write(TEXT("TimeBar.Interval"), m_TimeBarSettings.Interval);
	Settings.Write(TEXT("TimeBar.Custom"), m_TimeBarSettings.CustomTime);
	Settings.Write(TEXT("TimeBar.MaxButtonCount"), m_TimeBarSettings.MaxButtonCount);

	return true;
}


LPCTSTR CProgramGuideFrameSettings::GetToolbarIDText(int Toolbar) const
{
	if (Toolbar < 0 || Toolbar >= lengthof(m_ToolbarInfoList))
		return nullptr;
	return m_ToolbarInfoList[Toolbar].pszIDText;
}


LPCTSTR CProgramGuideFrameSettings::GetToolbarName(int Toolbar) const
{
	if (Toolbar < 0 || Toolbar >= lengthof(m_ToolbarInfoList))
		return nullptr;
	return m_ToolbarInfoList[Toolbar].pszName;
}


bool CProgramGuideFrameSettings::SetToolbarVisible(int Toolbar, bool fVisible)
{
	if (Toolbar < 0 || Toolbar >= lengthof(m_ToolbarSettingsList))
		return false;
	m_ToolbarSettingsList[Toolbar].fVisible = fVisible;
	return true;
}


bool CProgramGuideFrameSettings::GetToolbarVisible(int Toolbar) const
{
	if (Toolbar < 0 || Toolbar >= lengthof(m_ToolbarSettingsList))
		return false;
	return m_ToolbarSettingsList[Toolbar].fVisible;
}


bool CProgramGuideFrameSettings::SetToolbarOrderList(const int *pOrder)
{
	if (pOrder == nullptr)
		return false;

	for (int i = 0; i < lengthof(m_ToolbarSettingsList); i++) {
		const int ID = pOrder[i];

		if (ID < 0 || ID >= lengthof(m_ToolbarSettingsList))
			return false;

		for (int j = i + 1; j < lengthof(m_ToolbarSettingsList); j++) {
			if (pOrder[j] == ID)
				return false;
		}
	}

	for (int i = 0; i < lengthof(m_ToolbarSettingsList); i++)
		m_ToolbarSettingsList[pOrder[i]].Order = i;

	return true;
}


bool CProgramGuideFrameSettings::GetToolbarOrderList(int *pOrder) const
{
	if (pOrder == nullptr)
		return false;

	for (int i = 0; i < lengthof(m_ToolbarSettingsList); i++)
		pOrder[m_ToolbarSettingsList[i].Order] = i;

	return true;
}


bool CProgramGuideFrameSettings::SetDateBarButtonCount(int ButtonCount)
{
	if (ButtonCount < 1 || ButtonCount > DATEBAR_MAXBUTTONCOUNT)
		return false;

	m_DateBarButtonCount = ButtonCount;

	return true;
}


bool CProgramGuideFrameSettings::SetTimeBarSettings(const TimeBarSettings &Settings)
{
	m_TimeBarSettings = Settings;
	return true;
}


int CProgramGuideFrameSettings::ParseIDText(LPCTSTR pszID) const
{
	if (IsStringEmpty(pszID))
		return -1;

	for (int i = 0; i < lengthof(m_ToolbarInfoList); i++) {
		if (::lstrcmpi(m_ToolbarInfoList[i].pszIDText, pszID) == 0)
			return i;
	}

	return -1;
}




const LPCTSTR CProgramGuideFrame::m_pszWindowClass = APP_NAME TEXT(" Program Guide Frame");
HINSTANCE CProgramGuideFrame::m_hinst = nullptr;


bool CProgramGuideFrame::Initialize(HINSTANCE hinst)
{
	if (m_hinst == nullptr) {
		WNDCLASS wc;

		wc.style = 0;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hinst;
		wc.hIcon = ::LoadIcon(hinst, MAKEINTRESOURCE(IDI_PROGRAMGUIDE));
		wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = ::CreateSolidBrush(0xFF000000);
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = m_pszWindowClass;
		if (::RegisterClass(&wc) == 0)
			return false;
		m_hinst = hinst;
	}
	return true;
}


CProgramGuideFrame::CProgramGuideFrame(CProgramGuide *pProgramGuide, CProgramGuideFrameSettings *pSettings)
	: CProgramGuideFrameBase(pProgramGuide, pSettings)
{
	m_WindowPosition.Width = 640;
	m_WindowPosition.Height = 480;

	SetStyleScaling(&m_StyleScaling);
}


CProgramGuideFrame::~CProgramGuideFrame()
{
	Destroy();
}


bool CProgramGuideFrame::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	if (m_WindowPosition.fMaximized)
		Style |= WS_MAXIMIZE;
	if (m_fAlwaysOnTop)
		ExStyle |= WS_EX_TOPMOST;
	return CreateBasicWindow(hwndParent, Style, ExStyle, ID, m_pszWindowClass, TITLE_TEXT, m_hinst);
}


bool CProgramGuideFrame::SetAlwaysOnTop(bool fTop)
{
	if (m_fAlwaysOnTop != fTop) {
		m_fAlwaysOnTop = fTop;
		if (m_hwnd != nullptr)
			::SetWindowPos(m_hwnd, fTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
	return true;
}


void CProgramGuideFrame::SetTheme(const Theme::CThemeManager *pThemeManager)
{
	CProgramGuideFrameBase::SetTheme(pThemeManager);
}


void CProgramGuideFrame::SetStyle(const Style::CStyleManager *pStyleManager)
{
	m_FrameStyle.SetStyle(pStyleManager);
}


void CProgramGuideFrame::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	m_FrameStyle.NormalizeStyle(pStyleManager, pStyleScaling);
}


bool CProgramGuideFrame::Show()
{
	if (m_hwnd == nullptr)
		return false;
	::ShowWindow(m_hwnd, m_WindowPosition.fMaximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);
	return true;
}


void CProgramGuideFrame::OnLayoutChange()
{
	SendSizeMessage();
}


void CProgramGuideFrame::SetCaption(LPCTSTR pszFileName)
{
	::SetWindowText(m_hwnd, pszFileName);
}


LRESULT CProgramGuideFrame::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		InitializeUI();
		OnWindowCreate(hwnd, m_pStyleScaling, true);

		if (m_FrameStyle.fAllowDarkMode && SetWindowAllowDarkMode(hwnd, true)) {
			m_fAllowDarkMode = true;
			if (TVTest::IsDarkMode()) {
				if (SetWindowFrameDarkMode(hwnd, true))
					m_fDarkMode = true;
			}
		}
		if (!m_fDarkMode) {
			SetAeroGlass();
		}

		m_fCreated = true;
		return 0;

	case WM_NCCREATE:
		// EnableNonClientDpiScaling 内から WM_SIZE が送られる
		InitStyleScaling(hwnd, true);
		break;

	case WM_SIZE:
		if (m_fCreated) {
			RECT rcOld, rcNew;

			m_pProgramGuide->GetPosition(&rcOld);
			DefaultMessageHandler(hwnd, uMsg, wParam, lParam);
			m_pProgramGuide->GetPosition(&rcNew);
			if (rcNew.top != rcOld.top) {
				SetAeroGlass();
			}
		}
		return 0;

	case WM_SHOWWINDOW:
		if (!wParam)
			break;
		[[fallthrough]];
	case WM_DWMCOMPOSITIONCHANGED:
		SetAeroGlass();
		m_UxTheme.Close();
		break;

	case WM_PAINT:
		if (m_fAero && !Util::OS::IsWindows8OrLater()) {
			::PAINTSTRUCT ps;

			::BeginPaint(hwnd, &ps);
			{
				CBufferedPaint BufferedPaint;
				RECT rc;
				::GetClientRect(hwnd, &rc);
				const HDC hdc = BufferedPaint.Begin(ps.hdc, &rc);
				if (hdc != nullptr) {
					::FillRect(hdc, &ps.rcPaint, static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
					BufferedPaint.SetAlpha(32);
					BufferedPaint.End();
				}
			}
			::EndPaint(hwnd, &ps);
			return 0;
		}
		break;

	case WM_ERASEBKGND:
		if (!m_fAero) {
			DrawBackground(reinterpret_cast<HDC>(wParam), ::GetForegroundWindow() == hwnd);
			return 1;
		}
		break;

	case WM_ACTIVATE:
		if (!m_fAero) {
			const HDC hdc = ::GetDC(hwnd);
			DrawBackground(hdc, wParam != WA_INACTIVE);
			::ReleaseDC(hwnd, hdc);
		}
		break;

	case WM_CLOSE:
		if (!m_pProgramGuide->OnCloseFrame())
			return 0;
		break;

	case WM_DPICHANGED:
		m_UxTheme.Close();
		OnDPIChanged(hwnd, wParam, lParam);
		break;

	case WM_THEMECHANGED:
		m_UxTheme.Close();
		break;

	case WM_SETTINGCHANGE:
		if (m_fAllowDarkMode) {
			if (IsDarkModeSettingChanged(hwnd, uMsg, wParam, lParam)) {
				const bool fDarkMode = TVTest::IsDarkMode();
				if (m_fDarkMode != fDarkMode) {
					if (SetWindowFrameDarkMode(hwnd, fDarkMode)) {
						m_fDarkMode = fDarkMode;
						SetAeroGlass();
						GetAppClass().AppEventManager.OnProgramGuideDarkModeChanged(fDarkMode);
					}
				}
			}
		}
		break;

	case WM_DESTROY:
		m_UxTheme.Close();
		break;
	}

	return DefaultMessageHandler(hwnd, uMsg, wParam, lParam);
}


void CProgramGuideFrame::RealizeStyle()
{
	SendSizeMessage();
}


void CProgramGuideFrame::SetAeroGlass()
{
	const bool fAeroOld = m_fAero;
	m_fAero = false;
	if (m_hwnd != nullptr && !m_fDarkMode && m_FrameStyle.fExtendFrame) {
		RECT rc = {0, 0, 0, 0}, rcPos;

		m_pProgramGuide->GetPosition(&rcPos);
		rc.top = rcPos.top;
		if (m_AeroGlass.ApplyAeroGlass(m_hwnd, &rc))
			m_fAero = true;
	} else if (m_hwnd != nullptr && fAeroOld) {
		RECT rc = {0, 0, 0, 0};
		m_AeroGlass.ApplyAeroGlass(m_hwnd, &rc);
	}
}


void CProgramGuideFrame::DrawBackground(HDC hdc, bool fActive)
{
	RECT rc, rcGuide;

	GetClientRect(&rc);
	m_pProgramGuide->GetPosition(&rcGuide);
	rc.bottom = rcGuide.top;

	if (m_fDarkMode) {
		DrawUtil::Fill(hdc, &rc, RGB(43, 43, 43));
	} else if (m_UxTheme.IsActive()
			&& (m_UxTheme.IsOpen()
				|| m_UxTheme.Open(m_hwnd, L"Window", m_pStyleScaling->GetDPI()))) {
		COLORREF Color;
		m_UxTheme.GetColor(
			WP_CAPTION,
			fActive ? CS_ACTIVE : CS_INACTIVE,
			fActive ? TMT_FILLCOLORHINT : TMT_BORDERCOLORHINT,
			&Color);
		DrawUtil::Fill(hdc, &rc, Color);
	} else {
		::FillRect(hdc, &rc, reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1));
	}
}




const LPCTSTR CProgramGuideDisplay::m_pszWindowClass = APP_NAME TEXT(" Program Guide Display");
HINSTANCE CProgramGuideDisplay::m_hinst = nullptr;


bool CProgramGuideDisplay::Initialize(HINSTANCE hinst)
{
	if (m_hinst == nullptr) {
		WNDCLASS wc;

		wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hinst;
		wc.hIcon = nullptr;
		wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = ::CreateSolidBrush(RGB(0, 0, 0));
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = m_pszWindowClass;
		if (::RegisterClass(&wc) == 0)
			return false;
		m_hinst = hinst;
	}
	return true;
}


CProgramGuideDisplay::CProgramGuideDisplay(CProgramGuide *pProgramGuide, CProgramGuideFrameSettings *pSettings)
	: CProgramGuideFrameBase(pProgramGuide, pSettings)
{
}


CProgramGuideDisplay::~CProgramGuideDisplay()
{
	Destroy();
}


bool CProgramGuideDisplay::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	return CreateBasicWindow(hwndParent, Style, ExStyle, ID, m_pszWindowClass, nullptr, m_hinst);
}


void CProgramGuideDisplay::SetTheme(const Theme::CThemeManager *pThemeManager)
{
	CProgramGuideFrameBase::SetTheme(pThemeManager);
	CDisplayView::SetTheme(pThemeManager);
}


void CProgramGuideDisplay::SetStyle(const Style::CStyleManager *pStyleManager)
{
	m_FrameStyle.SetStyle(pStyleManager);
}


void CProgramGuideDisplay::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	m_FrameStyle.NormalizeStyle(pStyleManager, pStyleScaling);
}


void CProgramGuideDisplay::SetEventHandler(CProgramGuideDisplayEventHandler *pHandler)
{
	if (m_pProgramGuideDisplayEventHandler != nullptr)
		m_pProgramGuideDisplayEventHandler->m_pProgramGuideDisplay = nullptr;
	if (pHandler != nullptr)
		pHandler->m_pProgramGuideDisplay = this;
	m_pProgramGuideDisplayEventHandler = pHandler;
	CDisplayView::SetEventHandler(pHandler);
}


bool CProgramGuideDisplay::Close()
{
	SetVisible(false);
	return true;
}


bool CProgramGuideDisplay::OnVisibleChange(bool fVisible)
{
	if (!fVisible && m_pProgramGuideDisplayEventHandler != nullptr)
		return m_pProgramGuideDisplayEventHandler->OnHide();
	return true;
}


bool CProgramGuideDisplay::SetAlwaysOnTop(bool fTop)
{
	if (m_pProgramGuideDisplayEventHandler == nullptr)
		return false;
	return m_pProgramGuideDisplayEventHandler->SetAlwaysOnTop(fTop);
}


bool CProgramGuideDisplay::GetAlwaysOnTop() const
{
	if (m_pProgramGuideDisplayEventHandler == nullptr)
		return false;
	return m_pProgramGuideDisplayEventHandler->GetAlwaysOnTop();
}


void CProgramGuideDisplay::OnLayoutChange()
{
	SendSizeMessage();
}


LRESULT CProgramGuideDisplay::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		InitializeUI();
		OnWindowCreate(hwnd, m_pStyleScaling, false);
		return 0;

	case WM_SIZE:
		{
			RECT rc;

			GetCloseButtonRect(&rc);
			m_ToolbarRightMargin = LOWORD(lParam) - rc.left;
		}
		break;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd, &ps);
			DrawCloseButton(ps.hdc);
			::EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			const int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

			if (CloseButtonHitTest(x, y))
				Close();
			else
				::SetFocus(hwnd);
		}
		return 0;

	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) {
			Close();
			return 0;
		}
		break;

	default:
		{
			LRESULT Result;
			if (HandleMessage(hwnd, uMsg, wParam, lParam, &Result))
				return Result;
		}
		break;
	}

	return DefaultMessageHandler(hwnd, uMsg, wParam, lParam);
}


}	// namespace TVTest
