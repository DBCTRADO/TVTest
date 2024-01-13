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
#include <utility>
#include <algorithm>
#include "TVTest.h"
#include "HomeDisplay.h"
#include "AppMain.h"
#include "Favorites.h"
#include "EpgUtil.h"
#include "LogoManager.h"
#include "ChannelHistory.h"
#include "FeaturedEvents.h"
#include "DriverManager.h"
#include "TextDraw.h"
#include "DarkMode.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


static constexpr int CATEGORY_ICON_WIDTH = 32, CATEGORY_ICON_HEIGHT = 32;

enum {
	CATEGORY_ICON_FAVORITES,
	CATEGORY_ICON_RECENT_CHANNELS,
	CATEGORY_ICON_FEATURED_EVENTS
};

enum {
	CATEGORY_ID_FAVORITES,
	CATEGORY_ID_RECENT_CHANNELS,
	CATEGORY_ID_FEATURED_EVENTS
};




class ABSTRACT_CLASS(CChannelListCategoryBase)
	: public CHomeDisplay::CCategory
{
public:
	CChannelListCategoryBase(CHomeDisplay * pHomeDisplay);

// CHomeDisplay::CCategory
	int GetHeight() const override { return m_Height; }
	void LayOut(const CHomeDisplay::StyleInfo &Style, HDC hdc, const RECT &ContentRect) override;
	void Draw(
		HDC hdc, const CHomeDisplay::StyleInfo &Style, const RECT &ContentRect, const RECT &PaintRect,
		Theme::CThemeDraw &ThemeDraw) const override;
	bool GetCurItemRect(RECT *pRect) const override;
	bool SetFocus(bool fFocus) override;
	bool IsFocused() const override { return m_HotItem >= 0; }
	void OnCursorMove(int x, int y) override;
	void OnCursorLeave() override;
	void OnLButtonDown(int x, int y) override;
	bool OnLButtonUp(int x, int y) override;
	bool OnSetCursor() override;
	bool OnCursorKey(WPARAM KeyCode) override;

protected:
	class ABSTRACT_CLASS(CChannelItemBase)
		: public CChannelInfo
	{
	public:
		CChannelItemBase(const CChannelInfo & ChannelInfo);
		virtual ~CChannelItemBase() = default;
		void SetSmallLogo(HBITMAP hbm) { m_hbmSmallLogo = hbm; }
		void SetBigLogo(HBITMAP hbm) { m_hbmBigLogo = hbm; }
		HBITMAP GetStretchedLogo(int Width, int Height) const;
		bool SetEvent(int Index, const LibISDB::EventInfo *pEvent);
		const LibISDB::EventInfo *GetEvent(int Index) const;

	protected:
		HBITMAP m_hbmSmallLogo = nullptr;
		HBITMAP m_hbmBigLogo = nullptr;
		mutable DrawUtil::CBitmap m_StretchedLogo;
		LibISDB::EventInfo m_Event[2];
	};

	typedef std::vector<std::unique_ptr<CChannelItemBase>> ItemList;

	RECT m_Rect{};
	int m_Height = 0;
	int m_ItemHeight = 0;
	int m_ChannelNameWidth = 0;
	int m_LogoWidth = 0;
	int m_LogoHeight = 0;
	int m_HotItem = -1;
	int m_ClickingItem = -1;
	ItemList m_ItemList;

	void Clear();
	void UpdateChannelInfo();
	bool GetItemRect(size_t Item, RECT *pRect) const;
	int GetItemByPosition(int x, int y) const;
	void RedrawItem(int Item);
	bool SetHotItem(int Item);
};


CChannelListCategoryBase::CChannelListCategoryBase(CHomeDisplay *pHomeDisplay)
	: CCategory(pHomeDisplay)
{
}


void CChannelListCategoryBase::LayOut(const CHomeDisplay::StyleInfo &Style, HDC hdc, const RECT &ContentRect)
{
	m_ItemHeight = Style.FontHeight * 2 + Style.ItemMargins.Vert();
	m_Height = static_cast<int>(m_ItemList.size()) * m_ItemHeight;
	m_Rect = ContentRect;

	m_LogoHeight = std::min(Style.FontHeight, 36);
	m_LogoWidth = (m_LogoHeight * 16 + 4) / 9;

	m_ChannelNameWidth = Style.FontHeight * 8;
	for (const auto &e : m_ItemList) {
		const LPCTSTR pszName = e->GetName();
		SIZE sz;

		if (::GetTextExtentPoint32(hdc, pszName, ::lstrlen(pszName), &sz)
				&& sz.cx > m_ChannelNameWidth)
			m_ChannelNameWidth = sz.cx;
	}
}


void CChannelListCategoryBase::Draw(
	HDC hdc, const CHomeDisplay::StyleInfo &Style, const RECT &ContentRect, const RECT &PaintRect,
	Theme::CThemeDraw &ThemeDraw) const
{
	for (size_t i = 0; i < m_ItemList.size(); i++) {
		RECT rcItem, rc;

		GetItemRect(i, &rcItem);
		if (!IsRectIntersect(&rcItem, &PaintRect))
			continue;
		const CChannelItemBase *pItem = m_ItemList[i].get();
		const Theme::Style *pItemStyle;
		if (i == m_HotItem) {
			pItemStyle = &Style.ItemHotStyle;
		} else {
			pItemStyle = &Style.ItemStyle[i % 2];
		}
		ThemeDraw.Draw(pItemStyle->Back, rcItem);
		rc = rcItem;
		rc.left += Style.ItemMargins.Left;
		rc.top += Style.ItemMargins.Top;
		rc.right = rc.left + m_ChannelNameWidth;
		rc.bottom -= Style.ItemMargins.Bottom;
		const HBITMAP hbmLogo = pItem->GetStretchedLogo(m_LogoWidth, m_LogoHeight);
		if (hbmLogo != nullptr) {
			DrawUtil::DrawBitmap(
				hdc, rc.left, rc.top + (Style.FontHeight - m_LogoHeight) / 2,
				m_LogoWidth, m_LogoHeight, hbmLogo, nullptr,
				i == m_HotItem ? 255 : 224);
			rc.top += Style.FontHeight;
		}
		ThemeDraw.Draw(
			pItemStyle->Fore, rc, pItem->GetName(),
			DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);

		rc.left = rc.right + 8;
		rc.top = rcItem.top + Style.ItemMargins.Top;
		rc.right = rcItem.right - Style.ItemMargins.Right;
		rc.bottom = rc.top + Style.FontHeight;

		for (int j = 0; j < 2; j++) {
			const LibISDB::EventInfo *pEventInfo = pItem->GetEvent(j);

			if (pEventInfo != nullptr) {
				TCHAR szText[1024];
				size_t Length;

				Length = EpgUtil::FormatEventTime(
					*pEventInfo, szText, lengthof(szText),
					EpgUtil::FormatEventTimeFlag::Hour2Digits | EpgUtil::FormatEventTimeFlag::StartOnly);
				if (!pEventInfo->EventName.empty()) {
					Length += StringFormat(
						szText + Length, lengthof(szText) - Length,
						TEXT("{}{}"),
						Length > 0 ? TEXT(" ") : TEXT(""),
						pEventInfo->EventName);
				}
				if (Length > 0) {
					ThemeDraw.Draw(
						pItemStyle->Fore, rc, szText,
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
				}
			}

			rc.top = rc.bottom;
			rc.bottom = rc.top + Style.FontHeight;
		}
	}
}


bool CChannelListCategoryBase::GetCurItemRect(RECT *pRect) const
{
	if (m_HotItem < 0)
		return false;
	return GetItemRect(m_HotItem, pRect);
}


bool CChannelListCategoryBase::SetFocus(bool fFocus)
{
	if (fFocus) {
		if (m_ItemList.empty())
			return false;
		SetHotItem(0);
	} else {
		SetHotItem(-1);
	}
	return true;
}


void CChannelListCategoryBase::OnCursorMove(int x, int y)
{
	SetHotItem(GetItemByPosition(x, y));
}


void CChannelListCategoryBase::OnCursorLeave()
{
	SetHotItem(-1);
}


void CChannelListCategoryBase::OnLButtonDown(int x, int y)
{
	SetHotItem(GetItemByPosition(x, y));
	m_ClickingItem = m_HotItem;
}


bool CChannelListCategoryBase::OnLButtonUp(int x, int y)
{
	if (m_ClickingItem < 0 || m_ClickingItem != GetItemByPosition(x, y))
		return false;
	return OnDecide();
}


bool CChannelListCategoryBase::OnSetCursor()
{
	if (m_HotItem >= 0) {
		::SetCursor(GetAppClass().UICore.GetActionCursor());
		return true;
	}
	return false;
}


bool CChannelListCategoryBase::OnCursorKey(WPARAM KeyCode)
{
	int HotItem;

	if (KeyCode == VK_UP) {
		if (m_HotItem == 0 || m_ItemList.empty())
			return false;
		if (m_HotItem < 0)
			HotItem = 0;
		else
			HotItem = m_HotItem - 1;
	} else if (KeyCode == VK_DOWN) {
		if (m_ItemList.empty() || m_HotItem + 1 == static_cast<int>(m_ItemList.size()))
			return false;
		if (m_HotItem < 0)
			HotItem = 0;
		else
			HotItem = m_HotItem + 1;
	} else {
		return false;
	}

	SetHotItem(HotItem);

	return true;
}


void CChannelListCategoryBase::Clear()
{
	m_ItemList.clear();

	m_Height = 0;
	m_HotItem = -1;
	m_ClickingItem = -1;
}


void CChannelListCategoryBase::UpdateChannelInfo()
{
	const LibISDB::EPGDatabase &EPGDatabase = GetAppClass().EPGDatabase;
	CLogoManager &LogoManager = GetAppClass().LogoManager;
	LibISDB::DateTime CurTime;

	LibISDB::GetCurrentEPGTime(&CurTime);

	for (const auto &Item : m_ItemList) {
		LibISDB::EventInfo EventInfo;

		if (EPGDatabase.GetEventInfo(
					Item->GetNetworkID(),
					Item->GetTransportStreamID(),
					Item->GetServiceID(),
					CurTime, &EventInfo)) {
			Item->SetEvent(0, &EventInfo);
			LibISDB::DateTime EndTime;
			if (EventInfo.StartTime.IsValid()
					&& EventInfo.Duration > 0
					&& EventInfo.GetEndTime(&EndTime)
					&& EPGDatabase.GetEventInfo(
						Item->GetNetworkID(),
						Item->GetTransportStreamID(),
						Item->GetServiceID(),
						EndTime, &EventInfo)) {
				Item->SetEvent(1, &EventInfo);
			} else {
				Item->SetEvent(1, nullptr);
			}
		} else {
			Item->SetEvent(0, nullptr);
			if (EPGDatabase.GetNextEventInfo(
						Item->GetNetworkID(),
						Item->GetTransportStreamID(),
						Item->GetServiceID(),
						CurTime, &EventInfo)
					&& EventInfo.StartTime.DiffSeconds(CurTime) < 8 * 60 * 60) {
				Item->SetEvent(1, &EventInfo);
			} else {
				Item->SetEvent(1, nullptr);
			}
		}

		HBITMAP hbmLogo = LogoManager.GetAssociatedLogoBitmap(
			Item->GetNetworkID(), Item->GetServiceID(), CLogoManager::LOGOTYPE_SMALL);
		if (hbmLogo != nullptr)
			Item->SetSmallLogo(hbmLogo);
		hbmLogo = LogoManager.GetAssociatedLogoBitmap(
			Item->GetNetworkID(), Item->GetServiceID(), CLogoManager::LOGOTYPE_BIG);
		if (hbmLogo != nullptr)
			Item->SetBigLogo(hbmLogo);
	}
}


bool CChannelListCategoryBase::GetItemRect(size_t Item, RECT *pRect) const
{
	if (Item >= m_ItemList.size())
		return false;

	pRect->left = m_Rect.left;
	pRect->top = m_Rect.top + static_cast<int>(Item) * m_ItemHeight - m_pHomeDisplay->GetScrollPos();
	pRect->right = m_Rect.right;
	pRect->bottom = pRect->top + m_ItemHeight;

	return true;
}


int CChannelListCategoryBase::GetItemByPosition(int x, int y) const
{
	const POINT pt = {x, y};

	if (!::PtInRect(&m_Rect, pt))
		return -1;

	const int Item = (y - m_Rect.top + m_pHomeDisplay->GetScrollPos()) / m_ItemHeight;
	if (static_cast<size_t>(Item) >= m_ItemList.size())
		return -1;
	return Item;
}


void CChannelListCategoryBase::RedrawItem(int Item)
{
	RECT rc;

	if (GetItemRect(Item, &rc))
		m_pHomeDisplay->Invalidate(&rc);
}


bool CChannelListCategoryBase::SetHotItem(int Item)
{
	int HotItem;

	if (Item >= 0) {
		if (static_cast<size_t>(Item) >= m_ItemList.size())
			return false;
		HotItem = Item;
	} else {
		HotItem = -1;
	}

	if (m_HotItem != HotItem) {
		if (m_HotItem >= 0)
			RedrawItem(m_HotItem);
		m_HotItem = HotItem;
		if (HotItem >= 0)
			RedrawItem(HotItem);
		m_ClickingItem = -1;
	}

	return true;
}


CChannelListCategoryBase::CChannelItemBase::CChannelItemBase(const CChannelInfo &ChannelInfo)
	: CChannelInfo(ChannelInfo)
{
}


HBITMAP CChannelListCategoryBase::CChannelItemBase::GetStretchedLogo(int Width, int Height) const
{
	const HBITMAP hbmLogo =
		(Height <= 14 || m_hbmBigLogo == nullptr) ? m_hbmSmallLogo : m_hbmBigLogo;
	if (hbmLogo == nullptr)
		return nullptr;

	// AlphaBlendでリサイズすると汚いので、予めリサイズした画像を作成しておく
	if (m_StretchedLogo.IsCreated()) {
		if (m_StretchedLogo.GetWidth() != Width || m_StretchedLogo.GetHeight() != Height)
			m_StretchedLogo.Destroy();
	}
	if (!m_StretchedLogo.IsCreated()) {
		const HBITMAP hbm = DrawUtil::ResizeBitmap(hbmLogo, Width, Height);
		if (hbm != nullptr)
			m_StretchedLogo.Attach(hbm);
	}
	return m_StretchedLogo.GetHandle();
}


bool CChannelListCategoryBase::CChannelItemBase::SetEvent(int Index, const LibISDB::EventInfo *pEvent)
{
	if (Index < 0 || Index > 1)
		return false;
	if (pEvent != nullptr)
		m_Event[Index] = *pEvent;
	else
		m_Event[Index].EventName.clear();
	return true;
}


const LibISDB::EventInfo *CChannelListCategoryBase::CChannelItemBase::GetEvent(int Index) const
{
	if (Index < 0 || Index > 1)
		return nullptr;
	if (m_Event[Index].EventName.empty())
		return nullptr;
	return &m_Event[Index];
}




class CFavoritesCategory
	: public CChannelListCategoryBase
{
public:
	CFavoritesCategory(CHomeDisplay *pHomeDisplay);

// CHomeDisplay::CCategory
	int GetID() const override { return CATEGORY_ID_FAVORITES; }
	LPCTSTR GetTitle() const override { return TEXT("お気に入り"); }
	int GetIconIndex() const override { return CATEGORY_ICON_FAVORITES; }
	bool Create() override;
	bool OnDecide() override;

private:
	class CChannelItem
		: public CChannelItemBase
	{
		String m_BonDriverFileName;
		bool m_fForceBonDriverChange;

	public:
		CChannelItem(const CFavoriteChannel &Channel)
			: CChannelItemBase(Channel.GetChannelInfo())
			, m_BonDriverFileName(Channel.GetBonDriverFileName())
			, m_fForceBonDriverChange(Channel.GetForceBonDriverChange())
		{
		}

		LPCTSTR GetBonDriverFileName() const { return m_BonDriverFileName.c_str(); }
		bool GetForceBonDriverChange() const { return m_fForceBonDriverChange; }
	};
};


CFavoritesCategory::CFavoritesCategory(CHomeDisplay *pHomeDisplay)
	: CChannelListCategoryBase(pHomeDisplay)
{
}


bool CFavoritesCategory::Create()
{
	class CItemEnumerator
		: public CFavoriteItemEnumerator
	{
		ItemList &m_ItemList;

		bool ChannelItem(CFavoriteFolder &Folder, CFavoriteChannel &Channel) override
		{
			m_ItemList.emplace_back(std::make_unique<CChannelItem>(Channel));
			return true;
		}

	public:
		CItemEnumerator(ItemList &ItemList)
			: m_ItemList(ItemList)
		{
		}
	};

	CFavoritesManager &FavoritesManager = GetAppClass().FavoritesManager;
	CItemEnumerator ItemEnumerator(m_ItemList);

	Clear();
	ItemEnumerator.EnumItems(FavoritesManager.GetRootFolder());

	UpdateChannelInfo();

	return true;
}


bool CFavoritesCategory::OnDecide()
{
	if (m_HotItem >= 0) {
		const CChannelItem *pItem = static_cast<CChannelItem*>(m_ItemList[m_HotItem].get());

		return GetAppClass().Core.SelectChannel(
			pItem->GetBonDriverFileName(),
			*static_cast<const CChannelInfo*>(pItem),
			pItem->GetForceBonDriverChange() ?
				CAppCore::SelectChannelFlag::None :
				CAppCore::SelectChannelFlag::UseCurrentTuner);
	}

	return false;
}




class CRecentChannelsCategory
	: public CChannelListCategoryBase
{
public:
	CRecentChannelsCategory(CHomeDisplay *pHomeDisplay);

// CHomeDisplay::CCategory
	int GetID() const override { return CATEGORY_ID_RECENT_CHANNELS; }
	LPCTSTR GetTitle() const override { return TEXT("最近見たチャンネル"); }
	int GetIconIndex() const override { return CATEGORY_ICON_RECENT_CHANNELS; }
	bool Create() override;
	void ReadSettings(CSettings &Settings) override;
	bool OnDecide() override;

private:
	class CChannelItem
		: public CChannelItemBase
	{
		String m_BonDriverFileName;

	public:
		CChannelItem(const CTunerChannelInfo *pChannel)
			: CChannelItemBase(*pChannel)
			, m_BonDriverFileName(pChannel->GetTunerName())
		{
		}

		LPCTSTR GetBonDriverFileName() const { return m_BonDriverFileName.c_str(); }
	};

	int m_MaxChannels = 20;
};


CRecentChannelsCategory::CRecentChannelsCategory(CHomeDisplay *pHomeDisplay)
	: CChannelListCategoryBase(pHomeDisplay)
{
}


bool CRecentChannelsCategory::Create()
{
	Clear();

	const CRecentChannelList &RecentChannelList = GetAppClass().RecentChannelList;
	int NumChannels = RecentChannelList.NumChannels();
	if (NumChannels > m_MaxChannels)
		NumChannels = m_MaxChannels;
	for (int i = 0; i < NumChannels; i++) {
		m_ItemList.emplace_back(std::make_unique<CChannelItem>(RecentChannelList.GetChannelInfo(i)));
	}

	UpdateChannelInfo();

	return true;
}


void CRecentChannelsCategory::ReadSettings(CSettings &Settings)
{
	if (Settings.SetSection(TEXT("HomeDisplay"))) {
		int MaxChannels;
		if (Settings.Read(TEXT("RecentChannels_MaxChannels"), &MaxChannels) && MaxChannels > 0)
			m_MaxChannels = MaxChannels;
	}
}


bool CRecentChannelsCategory::OnDecide()
{
	if (m_HotItem >= 0) {
		const CChannelItem *pItem = static_cast<CChannelItem*>(m_ItemList[m_HotItem].get());

		return GetAppClass().Core.SelectChannel(
			pItem->GetBonDriverFileName(),
			*static_cast<const CChannelInfo*>(pItem));
	}

	return false;
}




class CFeaturedEventsCategory
	: public CHomeDisplay::CCategory
	, public CFeaturedEvents::CEventHandler
{
public:
	CFeaturedEventsCategory(CHomeDisplay *pHomeDisplay);

// CHomeDisplay::CCategory
	int GetID() const override { return CATEGORY_ID_FEATURED_EVENTS; }
	LPCTSTR GetTitle() const override { return TEXT("注目の番組"); }
	int GetIconIndex() const override { return CATEGORY_ICON_FEATURED_EVENTS; }
	int GetHeight() const override { return m_Height; }
	bool Create() override;
	void LayOut(const CHomeDisplay::StyleInfo &Style, HDC hdc, const RECT &ContentRect) override;
	void Draw(
		HDC hdc, const CHomeDisplay::StyleInfo &Style, const RECT &ContentRect, const RECT &PaintRect,
		Theme::CThemeDraw &ThemeDraw) const override;
	bool GetCurItemRect(RECT *pRect) const override;
	bool SetFocus(bool fFocus) override;
	bool IsFocused() const override { return m_HotItem >= 0; }
	bool OnDecide() override;
	void OnWindowCreate() override;
	void OnWindowDestroy() override;
	void OnCursorMove(int x, int y) override;
	void OnCursorLeave() override;
	void OnLButtonDown(int x, int y) override;
	bool OnLButtonUp(int x, int y) override;
	bool OnRButtonUp(int x, int y) override;
	bool OnSetCursor() override { return false; }
	bool OnCursorKey(WPARAM KeyCode) override;

private:
	class CEventItem
	{
	public:
		CEventItem(const CChannelInfo &ChannelInfo, const LibISDB::EventInfo &EventInfo);
		const CChannelInfo &GetChannelInfo() const { return m_ChannelInfo; }
		const LibISDB::EventInfo &GetEventInfo() const { return m_EventInfo; }
		int GetExpandedHeight() const { return m_ExpandedHeight; }
		void SetExpandedHeight(int Height) { m_ExpandedHeight = Height; }
		bool IsExpanded() const { return m_fExpanded; }
		void SetExpanded(bool fExpanded) { m_fExpanded = fExpanded; }
		void SetLogo(HBITMAP hbm);
		HBITMAP GetStretchedLogo(int Width, int Height) const;
		bool GetEventText(String *pText) const;

	private:
		CChannelInfo m_ChannelInfo;
		LibISDB::EventInfo m_EventInfo;
		int m_ExpandedHeight = 0;
		bool m_fExpanded = false;
		HBITMAP m_hbmLogo = nullptr;
		mutable DrawUtil::CBitmap m_StretchedLogo;

		static void AppendEventText(String *pString, LPCWSTR pszText);
	};

	RECT m_Rect{};
	int m_Height = 0;
	int m_ItemHeight = 0;
	int m_ChannelNameWidth = 0;
	int m_LogoWidth = 0;
	int m_LogoHeight = 0;
	int m_HotItem = -1;
	int m_ClickingItem = -1;
	std::vector<std::unique_ptr<CEventItem>> m_ItemList;

	void Clear();
	void SortItems(CFeaturedEventsSettings::SortType SortType);
	void ExpandItem(int Item);
	bool GetItemRect(size_t Item, RECT *pRect) const;
	int GetItemByPosition(int x, int y) const;
	void RedrawItem(int Item);
	bool SetHotItem(int Item);

// CFeaturedEvents::CEventHandler
	void OnFeaturedEventsSettingsChanged(CFeaturedEvents &FeaturedEvents) override;
};


CFeaturedEventsCategory::CFeaturedEventsCategory(CHomeDisplay *pHomeDisplay)
	: CCategory(pHomeDisplay)
{
}


bool CFeaturedEventsCategory::Create()
{
	Clear();

	CAppMain &App = GetAppClass();

	CChannelList ServiceList;
	App.DriverManager.GetAllServiceList(&ServiceList);

	const CFeaturedEventsSettings &Settings = App.FeaturedEvents.GetSettings();
	CFeaturedEventsSearcher Searcher(Settings);
	Searcher.Update();

	const size_t EventCount = Searcher.GetEventCount();
	for (size_t i = 0; i < EventCount; i++) {
		const LibISDB::EventInfo *pEventInfo = Searcher.GetEventInfo(i);
		const int Index = ServiceList.FindByIDs(
			pEventInfo->NetworkID, pEventInfo->TransportStreamID, pEventInfo->ServiceID);

		if (Index >= 0)
			m_ItemList.emplace_back(std::make_unique<CEventItem>(*ServiceList.GetChannelInfo(Index), *pEventInfo));
	}

	SortItems(Settings.GetSortType());

	return true;
}


void CFeaturedEventsCategory::LayOut(const CHomeDisplay::StyleInfo &Style, HDC hdc, const RECT &ContentRect)
{
	CTextDraw DrawText;
	RECT rc;
	m_pHomeDisplay->GetClientRect(&rc);
	DrawText.Begin(hdc, rc, CTextDraw::Flag::JapaneseHyphnation);

	const CFeaturedEventsSettings &Settings = GetAppClass().FeaturedEvents.GetSettings();
	const int ItemBaseHeight = 2 * Style.FontHeight + Style.ItemMargins.Vert();
	m_ItemHeight = ItemBaseHeight;
	if (Settings.GetShowEventText())
		m_ItemHeight += Settings.GetEventTextLines() * Style.FontHeight;
	m_Height = static_cast<int>(m_ItemList.size()) * m_ItemHeight;
	m_Rect = ContentRect;

	m_LogoHeight = std::min(Style.FontHeight, 36);
	m_LogoWidth = (m_LogoHeight * 16 + 4) / 9;
	CLogoManager &LogoManager = GetAppClass().LogoManager;

	const int EventTextWidth =
		(ContentRect.right - ContentRect.left) - Style.ItemMargins.Horz() - Style.FontHeight;
	String Text;

	m_ChannelNameWidth = 0;

	for (const auto &Item : m_ItemList) {
		const LPCTSTR pszName = Item->GetChannelInfo().GetName();
		SIZE sz;
		if (::GetTextExtentPoint32(hdc, pszName, ::lstrlen(pszName), &sz)
				&& sz.cx > m_ChannelNameWidth)
			m_ChannelNameWidth = sz.cx;

		Item->SetLogo(
			LogoManager.GetAssociatedLogoBitmap(
				Item->GetChannelInfo().GetNetworkID(),
				Item->GetChannelInfo().GetServiceID(),
				m_LogoHeight <= 14 ? CLogoManager::LOGOTYPE_SMALL : CLogoManager::LOGOTYPE_BIG));

		int Height = m_ItemHeight;
		if (Item->GetEventText(&Text)) {
			const int Lines = DrawText.CalcLineCount(Text.c_str(), EventTextWidth);
			if (!Settings.GetShowEventText()
					|| Lines > Settings.GetEventTextLines()) {
				Height = ItemBaseHeight + Lines * Style.FontHeight;
				if (Item->IsExpanded()) {
					m_Height += Height - m_ItemHeight;
				}
			}
		}
		Item->SetExpandedHeight(Height);
	}

	DrawText.End();
}


void CFeaturedEventsCategory::Draw(
	HDC hdc, const CHomeDisplay::StyleInfo &Style, const RECT &ContentRect, const RECT &PaintRect,
	Theme::CThemeDraw &ThemeDraw) const
{
	const CFeaturedEventsSettings &Settings = GetAppClass().FeaturedEvents.GetSettings();

	if (m_ItemList.empty()) {
		LPCTSTR pszText;
		if (Settings.GetSearchSettingsList().GetCount() == 0) {
			pszText = TEXT("注目の番組を表示するには、右クリックメニューの [注目の番組の設定] から検索条件を登録します。");
		} else {
			pszText = TEXT("検索された番組はありません。");
		}
		RECT rc = ContentRect;
		Style::Subtract(&rc, Style.ItemMargins);
		::SetTextColor(hdc, Style.BannerTextColor);
		::DrawText(hdc, pszText, -1, &rc, DT_LEFT | DT_NOPREFIX | DT_WORDBREAK);
		return;
	}

	CTextDraw DrawText;
	RECT rcClient;
	m_pHomeDisplay->GetClientRect(&rcClient);
	DrawText.Begin(
		hdc, rcClient,
		CTextDraw::Flag::EndEllipsis |
		CTextDraw::Flag::JapaneseHyphnation);

	const HFONT hfontText = static_cast<HFONT>(::GetCurrentObject(hdc, OBJ_FONT));
	LOGFONT lf;
	::GetObject(hfontText, sizeof(lf), &lf);
	lf.lfWeight = FW_BOLD;
	const HFONT hfontTitle = ::CreateFontIndirect(&lf);

	for (size_t i = 0; i < m_ItemList.size(); i++) {
		RECT rcItem, rc;

		GetItemRect(i, &rcItem);
		if (!IsRectIntersect(&rcItem, &PaintRect))
			continue;
		const CEventItem *pItem = m_ItemList[i].get();
		const CChannelInfo &ChannelInfo = pItem->GetChannelInfo();
		const Theme::Style *pItemStyle;
		if (i == m_HotItem) {
			pItemStyle = &Style.ItemHotStyle;
		} else {
			pItemStyle = &Style.ItemStyle[i % 2];
		}
		ThemeDraw.Draw(pItemStyle->Back, rcItem);
		::SetTextColor(hdc, pItemStyle->Fore.Fill.GetSolidColor());
		rc = rcItem;
		rc.left += Style.ItemMargins.Left;
		rc.top += Style.ItemMargins.Top;
		rc.bottom = rc.top + Style.FontHeight;
		const HBITMAP hbmLogo = pItem->GetStretchedLogo(m_LogoWidth, m_LogoHeight);
		if (hbmLogo != nullptr) {
			DrawUtil::DrawBitmap(
				hdc, rc.left, rc.top + (Style.FontHeight - m_LogoHeight) / 2,
				m_LogoWidth, m_LogoHeight, hbmLogo, nullptr,
				i == m_HotItem ? 255 : 224);
		}
		rc.left += m_LogoWidth + Style.ItemMargins.Left;
		rc.right = rc.left + m_ChannelNameWidth;
		::DrawText(
			hdc, ChannelInfo.GetName(), -1, &rc,
			DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);

		rc.left = rc.right + Style.FontHeight;
		rc.right = rcItem.right - Style.ItemMargins.Right;

		const LibISDB::EventInfo &EventInfo = pItem->GetEventInfo();
		TCHAR szText[1024];
		const int Length = EpgUtil::FormatEventTime(
			EventInfo, szText, lengthof(szText), EpgUtil::FormatEventTimeFlag::Date);
		if (Length > 0) {
			::DrawText(
				hdc, szText, Length, &rc,
				DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
		}
		rc.left = rcItem.left + Style.ItemMargins.Left;
		rc.top = rc.bottom;
		rc.bottom = rc.top + Style.FontHeight;
		if (!EventInfo.EventName.empty()) {
			::SelectObject(hdc, hfontTitle);
			::DrawText(
				hdc,
				EventInfo.EventName.data(), static_cast<int>(EventInfo.EventName.length()), &rc,
				DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
			::SelectObject(hdc, hfontText);
		}

		if (Settings.GetShowEventText() || pItem->IsExpanded()) {
			String Text;

			if (pItem->GetEventText(&Text)) {
				rc.left = rcItem.left + Style.ItemMargins.Left + Style.FontHeight;
				rc.top = rc.bottom;
				rc.bottom = rcItem.bottom - Style.ItemMargins.Bottom;
				DrawText.Draw(Text.c_str(), rc, Style.FontHeight);
			}
		}
	}

	::DeleteObject(hfontTitle);
	DrawText.End();
}


bool CFeaturedEventsCategory::GetCurItemRect(RECT *pRect) const
{
	if (m_HotItem < 0)
		return false;
	return GetItemRect(m_HotItem, pRect);
}


bool CFeaturedEventsCategory::SetFocus(bool fFocus)
{
	if (fFocus) {
		if (m_ItemList.empty())
			return false;
		SetHotItem(0);
	} else {
		SetHotItem(-1);
	}
	return true;
}


bool CFeaturedEventsCategory::OnDecide()
{
	if (m_HotItem >= 0) {
		ExpandItem(m_HotItem);
	}
	return false;
}


void CFeaturedEventsCategory::OnWindowCreate()
{
	GetAppClass().FeaturedEvents.AddEventHandler(this);
}


void CFeaturedEventsCategory::OnWindowDestroy()
{
	GetAppClass().FeaturedEvents.RemoveEventHandler(this);
}


void CFeaturedEventsCategory::OnCursorMove(int x, int y)
{
	SetHotItem(GetItemByPosition(x, y));
}


void CFeaturedEventsCategory::OnCursorLeave()
{
	SetHotItem(-1);
}


void CFeaturedEventsCategory::OnLButtonDown(int x, int y)
{
	SetHotItem(GetItemByPosition(x, y));
	m_ClickingItem = m_HotItem;
}


bool CFeaturedEventsCategory::OnLButtonUp(int x, int y)
{
	if (m_ClickingItem < 0 || m_ClickingItem != GetItemByPosition(x, y))
		return false;
	return OnDecide();
}


bool CFeaturedEventsCategory::OnRButtonUp(int x, int y)
{
	CFeaturedEvents &FeaturedEvents = GetAppClass().FeaturedEvents;
	CFeaturedEventsSettings &Settings = FeaturedEvents.GetSettings();
	CPopupMenu Menu(GetAppClass().GetResourceInstance(), IDM_FEATUREDEVENTS);

	Menu.CheckItem(CM_FEATUREDEVENTS_SHOWEVENTTEXT, Settings.GetShowEventText());
	Menu.CheckRadioItem(
		CM_FEATUREDEVENTS_SORT_FIRST,
		CM_FEATUREDEVENTS_SORT_LAST,
		CM_FEATUREDEVENTS_SORT_FIRST + static_cast<int>(Settings.GetSortType()));

	const int Result = Menu.Show(
		m_pHomeDisplay->GetHandle(), nullptr, TPM_RIGHTBUTTON | TPM_RETURNCMD);

	switch (Result) {
	case CM_FEATUREDEVENTS_SETTINGS:
		FeaturedEvents.ShowDialog(m_pHomeDisplay->GetHandle());
		break;

	case CM_FEATUREDEVENTS_SHOWEVENTTEXT:
		Settings.SetShowEventText(!Settings.GetShowEventText());
		m_pHomeDisplay->UpdateCurContent();
		break;

	default:
		if (Result >= CM_FEATUREDEVENTS_SORT_FIRST
				&& Result <= CM_FEATUREDEVENTS_SORT_LAST) {
			const CFeaturedEventsSettings::SortType SortType =
				static_cast<CFeaturedEventsSettings::SortType>(Result - CM_FEATUREDEVENTS_SORT_FIRST);
			if (SortType != Settings.GetSortType()) {
				Settings.SetSortType(SortType);
				SortItems(SortType);
				m_pHomeDisplay->SetScrollPos(0, false);
				m_pHomeDisplay->Invalidate();
			}
			break;
		}
	}

	return true;
}


bool CFeaturedEventsCategory::OnCursorKey(WPARAM KeyCode)
{
	int HotItem;

	if (KeyCode == VK_UP) {
		if (m_HotItem == 0 || m_ItemList.empty())
			return false;
		if (m_HotItem < 0)
			HotItem = 0;
		else
			HotItem = m_HotItem - 1;
	} else if (KeyCode == VK_DOWN) {
		if (m_ItemList.empty() || m_HotItem + 1 == static_cast<int>(m_ItemList.size()))
			return false;
		if (m_HotItem < 0)
			HotItem = 0;
		else
			HotItem = m_HotItem + 1;
	} else {
		return false;
	}

	SetHotItem(HotItem);

	return true;
}


void CFeaturedEventsCategory::Clear()
{
	m_ItemList.clear();

	m_Height = 0;
	m_HotItem = -1;
	m_ClickingItem = -1;
}


void CFeaturedEventsCategory::SortItems(CFeaturedEventsSettings::SortType SortType)
{
	class CItemCompare
	{
	public:
		CItemCompare(CFeaturedEventsSettings::SortType SortType) : m_SortType(SortType) {}

		bool operator()(
			const std::unique_ptr<CEventItem> &Item1,
			const std::unique_ptr<CEventItem> &Item2) const
		{
			int Cmp = 0;

			switch (m_SortType) {
			case CFeaturedEventsSettings::SortType::Time:
				Cmp = CompareTime(*Item1, *Item2);
				if (Cmp == 0)
					Cmp = CompareService(*Item1, *Item2);
				break;

			case CFeaturedEventsSettings::SortType::Service:
				Cmp = CompareService(*Item1, *Item2);
				if (Cmp == 0)
					Cmp = CompareTime(*Item1, *Item2);
				break;
			}

			return Cmp < 0;
		}

	private:
		CFeaturedEventsSettings::SortType m_SortType;

		int CompareTime(const CEventItem &Item1, const CEventItem &Item2) const
		{
			int Cmp;

			Cmp = Item1.GetEventInfo().StartTime.Compare(Item2.GetEventInfo().StartTime);
			if (Cmp == 0) {
				Cmp =
					static_cast<int>(Item1.GetEventInfo().Duration) -
					static_cast<int>(Item2.GetEventInfo().Duration);
			}
			return Cmp;
		}

		int CompareService(const CEventItem &Item1, const CEventItem &Item2) const
		{
			const CChannelInfo &ChannelInfo1 = Item1.GetChannelInfo();
			const CChannelInfo &ChannelInfo2 = Item2.GetChannelInfo();
			int Cmp;
			Cmp = GetAppClass().NetworkDefinition.GetNetworkTypeOrder(
				ChannelInfo1.GetNetworkID(), ChannelInfo2.GetNetworkID());
			if (Cmp == 0) {
				int Channel1 = ChannelInfo1.GetChannelNo();
				if (Channel1 <= 0)
					Channel1 = ChannelInfo1.GetServiceID();
				int Channel2 = ChannelInfo2.GetChannelNo();
				if (Channel2 <= 0)
					Channel2 = ChannelInfo1.GetServiceID();
				Cmp = Channel1 - Channel2;
			}
			return Cmp;
		}
	};

	std::ranges::sort(m_ItemList, CItemCompare(SortType));
}


void CFeaturedEventsCategory::ExpandItem(int Item)
{
	CEventItem *pCurItem = nullptr;

	if (Item >= 0 && static_cast<size_t>(Item) < m_ItemList.size())
		pCurItem = m_ItemList[Item].get();

	int ScrollPos = m_pHomeDisplay->GetScrollPos();

	if (pCurItem != nullptr && pCurItem->IsExpanded()) {
		pCurItem->SetExpanded(false);
		m_Height -= pCurItem->GetExpandedHeight() - m_ItemHeight;
	} else {
		for (size_t i = 0; i < m_ItemList.size(); i++) {
			CEventItem *pItem = m_ItemList[i].get();
			if (pItem->IsExpanded()) {
				pItem->SetExpanded(false);
				const int ExtendedHeight = pItem->GetExpandedHeight() - m_ItemHeight;
				if (pCurItem != nullptr && i < static_cast<size_t>(Item)) {
					ScrollPos -= ExtendedHeight;
				}
				m_Height -= ExtendedHeight;
			}
		}
		if (pCurItem != nullptr) {
			pCurItem->SetExpanded(true);
			m_Height += pCurItem->GetExpandedHeight() - m_ItemHeight;
		}
	}
	m_pHomeDisplay->OnContentChanged();
	m_pHomeDisplay->SetScrollPos(ScrollPos, false);
}


bool CFeaturedEventsCategory::GetItemRect(size_t Item, RECT *pRect) const
{
	if (Item >= m_ItemList.size())
		return false;

	RECT rc;
	rc.left = m_Rect.left;
	rc.bottom = m_Rect.top - m_pHomeDisplay->GetScrollPos();
	rc.right = m_Rect.right;
	for (size_t i = 0; i <= Item; i++) {
		const CEventItem *pItem = m_ItemList[i].get();
		rc.top = rc.bottom;
		if (pItem->IsExpanded()) {
			rc.bottom += pItem->GetExpandedHeight();
		} else {
			rc.bottom += m_ItemHeight;
		}
	}

	*pRect = rc;

	return true;
}


int CFeaturedEventsCategory::GetItemByPosition(int x, int y) const
{
	const POINT pt = {x, y};

	if (!::PtInRect(&m_Rect, pt))
		return -1;

	int Bottom = m_Rect.top - m_pHomeDisplay->GetScrollPos();

	for (size_t i = 0; i < m_ItemList.size(); i++) {
		const CEventItem *pItem = m_ItemList[i].get();
		const int Top = Bottom;
		if (pItem->IsExpanded()) {
			Bottom = Top + pItem->GetExpandedHeight();
		} else {
			Bottom = Top + m_ItemHeight;
		}
		if (y >= Top && y < Bottom)
			return static_cast<int>(i);
	}

	return -1;
}


void CFeaturedEventsCategory::RedrawItem(int Item)
{
	RECT rc;

	if (GetItemRect(Item, &rc))
		m_pHomeDisplay->Invalidate(&rc);
}


bool CFeaturedEventsCategory::SetHotItem(int Item)
{
	int HotItem;

	if (Item >= 0) {
		if (static_cast<size_t>(Item) >= m_ItemList.size())
			return false;
		HotItem = Item;
	} else {
		HotItem = -1;
	}

	if (m_HotItem != HotItem) {
		if (m_HotItem >= 0)
			RedrawItem(m_HotItem);
		m_HotItem = HotItem;
		if (HotItem >= 0)
			RedrawItem(HotItem);
		m_ClickingItem = -1;
	}

	return true;
}


void CFeaturedEventsCategory::OnFeaturedEventsSettingsChanged(CFeaturedEvents &FeaturedEvents)
{
	if (m_pHomeDisplay->GetCurCategoryID() == CATEGORY_ID_FEATURED_EVENTS)
		m_pHomeDisplay->UpdateCurContent();
}


CFeaturedEventsCategory::CEventItem::CEventItem(const CChannelInfo &ChannelInfo, const LibISDB::EventInfo &EventInfo)
	: m_ChannelInfo(ChannelInfo)
	, m_EventInfo(EventInfo)
{
}


void CFeaturedEventsCategory::CEventItem::SetLogo(HBITMAP hbm)
{
	if (m_hbmLogo != hbm) {
		m_hbmLogo = hbm;
		m_StretchedLogo.Destroy();
	}
}


HBITMAP CFeaturedEventsCategory::CEventItem::GetStretchedLogo(int Width, int Height) const
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


bool CFeaturedEventsCategory::CEventItem::GetEventText(String *pText) const
{
	pText->clear();
	if (!m_EventInfo.EventText.empty())
		AppendEventText(pText, m_EventInfo.EventText.c_str());
	if (!m_EventInfo.ExtendedText.empty()) {
		LibISDB::String ExtendedText;
		m_EventInfo.GetConcatenatedExtendedText(&ExtendedText);
		AppendEventText(pText, ExtendedText.c_str());
	}
	return !pText->empty();
}


void CFeaturedEventsCategory::CEventItem::AppendEventText(String *pString, LPCWSTR pszText)
{
	bool fFirst = true;
	LPCWSTR p = pszText;

	while (*p != L'\0') {
		if (*p == L'\r' || *p == L'\n') {
			const WCHAR c = *p++;
			int i = 0;
			while (*p == L'\r' || *p == L'\n') {
				if (*p == c)
					i++;
				p++;
			}
			if (*p == L'\0')
				break;
			if (!fFirst) {
				if (i > 0)
					pString->append(L"\r\n");
				else
					pString->push_back(L'　');
			}
		} else {
			if (fFirst) {
				if (!pString->empty())
					pString->append(L"\r\n");
				fFirst = false;
			}
			pString->push_back(*p);
			p++;
		}
	}
}




const LPCTSTR CHomeDisplay::m_pszWindowClass = APP_NAME TEXT(" Home Display");
HINSTANCE CHomeDisplay::m_hinst = nullptr;


bool CHomeDisplay::Initialize(HINSTANCE hinst)
{
	if (m_hinst == nullptr) {
		WNDCLASS wc;

		wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hinst;
		wc.hIcon = nullptr;
		wc.hCursor = nullptr;
		wc.hbrBackground = nullptr;
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = m_pszWindowClass;
		if (::RegisterClass(&wc) == 0)
			return false;
		m_hinst = hinst;
	}
	return true;
}


CHomeDisplay::CHomeDisplay()
{
	GetDefaultFont(&m_StyleFont);

	GetBackgroundStyle(BackgroundType::Categories, &m_HomeDisplayStyle.CategoriesBackStyle);
	GetBackgroundStyle(BackgroundType::Content, &m_HomeDisplayStyle.ContentBackStyle);
	GetItemStyle(ItemType::Normal, &m_HomeDisplayStyle.CategoryItemStyle);
	GetItemStyle(ItemType::Selected, &m_HomeDisplayStyle.CategoryItemSelStyle);
	GetItemStyle(ItemType::Current, &m_HomeDisplayStyle.CategoryItemCurStyle);
	GetItemStyle(ItemType::Normal1, &m_HomeDisplayStyle.ItemStyle[0]);
	GetItemStyle(ItemType::Normal2, &m_HomeDisplayStyle.ItemStyle[1]);
	GetItemStyle(ItemType::Hot, &m_HomeDisplayStyle.ItemHotStyle);
	m_HomeDisplayStyle.BannerTextColor = RGB(255, 255, 255);

	m_HomeDisplayStyle.ItemMargins.Left = 4;
	m_HomeDisplayStyle.ItemMargins.Top = 2;
	m_HomeDisplayStyle.ItemMargins.Right = 4;
	m_HomeDisplayStyle.ItemMargins.Bottom = 2;
	m_HomeDisplayStyle.CategoryItemMargins.Left = 4;
	m_HomeDisplayStyle.CategoryItemMargins.Top = 4;
	m_HomeDisplayStyle.CategoryItemMargins.Right = 6;
	m_HomeDisplayStyle.CategoryItemMargins.Bottom = 4;
	m_HomeDisplayStyle.CategoryIconMargin = 6;

	m_CategoryList.reserve(3);
	m_CategoryList.emplace_back(std::make_unique<CFavoritesCategory>(this));
	m_CategoryList.emplace_back(std::make_unique<CRecentChannelsCategory>(this));
	m_CategoryList.emplace_back(std::make_unique<CFeaturedEventsCategory>(this));
}


CHomeDisplay::~CHomeDisplay()
{
	Destroy();
}


bool CHomeDisplay::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	return CreateBasicWindow(hwndParent, Style, ExStyle, ID, m_pszWindowClass, nullptr, m_hinst);
}


bool CHomeDisplay::Close()
{
	if (m_pHomeDisplayEventHandler != nullptr) {
		m_pHomeDisplayEventHandler->OnClose();
		return true;
	}
	return false;
}


bool CHomeDisplay::IsMessageNeed(const MSG *pMsg) const
{
	if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_KEYUP) {
		switch (pMsg->wParam) {
		case VK_LEFT:
		case VK_RIGHT:
		case VK_UP:
		case VK_DOWN:
		case VK_RETURN:
		case VK_SPACE:
		case VK_PRIOR:
		case VK_NEXT:
		case VK_HOME:
		case VK_END:
		case VK_TAB:
		case VK_ESCAPE:
			return true;
		}
	}
	return false;
}


bool CHomeDisplay::OnMouseWheel(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (Msg == WM_MOUSEWHEEL && m_hwnd != nullptr) {
		const int Delta = m_MouseWheel.OnMouseWheel(
			wParam, m_HomeDisplayStyle.FontHeight * m_MouseWheel.GetDefaultScrollLines());
		if (Delta != 0)
			SetScrollPos(m_ScrollPos - Delta, true);
		return true;
	}

	return false;
}


bool CHomeDisplay::LoadSettings(CSettings &Settings)
{
	if (Settings.SetSection(TEXT("HomeDisplay"))) {
		int CategoryCount;
		if (Settings.Read(TEXT("CategoryCount"), &CategoryCount)
				&& CategoryCount > 0 && static_cast<size_t>(CategoryCount) <= m_CategoryList.size()) {
			std::vector<CCategory*> CategoryOrder;

			CategoryOrder.reserve(m_CategoryList.size());

			for (size_t i = 0; i < static_cast<size_t>(CategoryCount); i++) {
				TCHAR szKey[32];
				int ID;

				StringFormat(szKey, TEXT("Category{}_ID"), i);
				if (Settings.Read(szKey, &ID)) {
					size_t j;
					for (j = 0; j < i; j++) {
						if (CategoryOrder[j]->GetID() == ID)
							break;
					}
					if (j == i) {
						for (; j < m_CategoryList.size(); j++) {
							CCategory *pCategory = m_CategoryList[j].get();
							if (pCategory->GetID() == ID) {
								CategoryOrder.push_back(pCategory);
								break;
							}
						}
					}
				}
			}

			if (CategoryOrder.size() < m_CategoryList.size()) {
				for (size_t i = 0; i < m_CategoryList.size(); i++) {
					CCategory *pCategory = m_CategoryList[i].get();
					size_t j;
					for (j = 0; j < CategoryOrder.size(); j++) {
						if (CategoryOrder[j]->GetID() == pCategory->GetID())
							break;
					}
					if (j == CategoryOrder.size())
						CategoryOrder.push_back(pCategory);
				}
			}

			TVTEST_ASSERT(CategoryOrder.size() == m_CategoryList.size());

			for (size_t i = 0; i < m_CategoryList.size(); i++) {
				m_CategoryList[i].release();
				m_CategoryList[i].reset(CategoryOrder[i]);
			}
		}

		int CurCategory;
		if (Settings.Read(TEXT("CurCategory"), &CurCategory)
				&& CurCategory >= 0 && static_cast<size_t>(CurCategory) < m_CategoryList.size())
			m_CurCategory = CurCategory;
	}

	for (auto &e : m_CategoryList)
		e->ReadSettings(Settings);

	return true;
}


bool CHomeDisplay::SaveSettings(CSettings &Settings)
{
	if (Settings.SetSection(TEXT("HomeDisplay"))) {
		if (Settings.Write(TEXT("CategoryCount"), static_cast<int>(m_CategoryList.size()))) {
			for (int i = 0; i < static_cast<int>(m_CategoryList.size()); i++) {
				const CCategory *pCategory = m_CategoryList[i].get();
				TCHAR szKey[32];

				StringFormat(szKey, TEXT("Category{}_ID"), i);
				Settings.Write(szKey, pCategory->GetID());
			}
		}

		Settings.Write(TEXT("CurCategory"), m_CurCategory);
	}

	for (auto &e : m_CategoryList)
		e->WriteSettings(Settings);

	return true;
}


void CHomeDisplay::Clear()
{
	m_CategoryList.clear();
}


bool CHomeDisplay::UpdateContents()
{
	for (auto &e : m_CategoryList)
		e->Create();

	if (m_hwnd != nullptr) {
		LayOut();
		Invalidate();
	}

	return true;
}


void CHomeDisplay::SetEventHandler(CHomeDisplayEventHandler *pEventHandler)
{
	if (m_pHomeDisplayEventHandler != nullptr)
		m_pHomeDisplayEventHandler->m_pHomeDisplay = nullptr;
	if (pEventHandler != nullptr)
		pEventHandler->m_pHomeDisplay = this;
	m_pHomeDisplayEventHandler = pEventHandler;
	CDisplayView::SetEventHandler(pEventHandler);
}


bool CHomeDisplay::SetFont(const Style::Font &Font, bool fAutoSize)
{
	m_StyleFont = Font;
	m_fAutoFontSize = fAutoSize;
	if (m_hwnd != nullptr) {
		ApplyStyle();
		RealizeStyle();
	}
	return true;
}


bool CHomeDisplay::SetScrollPos(int Pos, bool fScroll)
{
	if (m_hwnd == nullptr)
		return false;

	RECT rcContent;
	GetContentAreaRect(&rcContent);

	if (Pos < 0 || m_ContentHeight <= rcContent.bottom - rcContent.top) {
		Pos = 0;
	} else if (Pos > m_ContentHeight - (rcContent.bottom - rcContent.top)) {
		Pos = m_ContentHeight - (rcContent.bottom - rcContent.top);
	}

	if (Pos != m_ScrollPos) {
		SCROLLINFO si;

		si.cbSize = sizeof(si);
		si.fMask = SIF_POS;
		si.nPos = Pos;
		::SetScrollInfo(m_hwndScroll, SB_CTL, &si, TRUE);
		if (fScroll) {
			Update();
			::ScrollWindowEx(
				m_hwnd, 0, m_ScrollPos - Pos,
				&rcContent, &rcContent, nullptr, nullptr, SW_INVALIDATE);
		}
		m_ScrollPos = Pos;
	}

	return true;
}


bool CHomeDisplay::SetCurCategory(int Category)
{
	if (Category != m_CurCategory) {
		if (Category < 0 || static_cast<size_t>(Category) >= m_CategoryList.size())
			return false;

		if (m_CurCategory >= 0)
			m_CategoryList[m_CurCategory]->SetFocus(false);

		m_CurCategory = Category;

		m_ScrollPos = 0;
		if (m_hwnd != nullptr) {
			LayOut();
			Invalidate();
		}
	}

	return true;
}


int CHomeDisplay::GetCurCategoryID() const
{
	if (m_CurCategory < 0 || static_cast<size_t>(m_CurCategory) >= m_CategoryList.size())
		return -1;
	return m_CategoryList[m_CurCategory]->GetID();
}


bool CHomeDisplay::UpdateCurContent()
{
	if (m_CurCategory < 0)
		return false;

	m_CategoryList[m_CurCategory]->Create();

	if (m_hwnd != nullptr) {
		m_ScrollPos = 0;
		LayOut();
		RECT rc;
		GetContentAreaRect(&rc);
		Invalidate(&rc);
	}

	return true;
}


bool CHomeDisplay::OnContentChanged()
{
	if (m_CurCategory < 0)
		return false;

	m_ContentHeight = m_CategoryList[m_CurCategory]->GetHeight();

	if (m_hwnd != nullptr) {
		SetScrollBar();
		RECT rc;
		GetContentAreaRect(&rc);
		Invalidate(&rc);
	}

	return true;
}


LRESULT CHomeDisplay::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		InitializeUI();

		m_hwndScroll = ::CreateWindowEx(
			0, TEXT("SCROLLBAR"), TEXT(""),
			WS_CHILD | SBS_VERT, 0, 0, 0, 0, hwnd, nullptr, m_hinst, nullptr);

		if (IsDarkThemeSupported())
			SetWindowDarkTheme(m_hwndScroll, IsDarkThemeStyle(m_HomeDisplayStyle.ContentBackStyle));

		m_ScrollPos = 0;
		m_fHitCloseButton = false;
		m_CursorPart = PartType::Margin;
		m_himlIcons = ::ImageList_LoadImage(
			m_hinst, MAKEINTRESOURCE(IDB_HOME),
			CATEGORY_ICON_WIDTH, 1, 0, IMAGE_BITMAP, LR_CREATEDIBSECTION);

		for (auto &e : m_CategoryList)
			e->OnWindowCreate();
		return 0;

	case WM_SIZE:
		LayOut();
		return 0;

	case WM_PAINT:
#if 0
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd, &ps);
			Draw(ps.hdc, ps.rcPaint);
			::EndPaint(hwnd, &ps);
		}
#else
		OnPaint(hwnd);
#endif
		return 0;

	case WM_LBUTTONDOWN:
		{
			const POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

			::SetFocus(hwnd);
			m_fHitCloseButton = CloseButtonHitTest(pt.x, pt.y);
		}
		[[fallthrough]];
	case WM_MOUSEMOVE:
		{
			const int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
			int CategoryIndex = -1;
			const PartType Part = HitTest(x, y, &CategoryIndex);

			m_CursorPart = Part;
			if (Part == PartType::Content) {
				if (m_CurCategory >= 0) {
					CCategory *pCategory = m_CategoryList[m_CurCategory].get();
					const bool fFocused = pCategory->IsFocused();
					if (uMsg == WM_LBUTTONDOWN)
						pCategory->OnLButtonDown(x, y);
					else
						pCategory->OnCursorMove(x, y);
					if (pCategory->IsFocused() != fFocused)
						RedrawCategoryItem(m_CurCategory);
				}
			} else {
				/*
				if (m_CurCategory >= 0) {
					CCategory *pCategory = m_CategoryList[m_CurCategory].get();
					const bool fFocused = pCategory->IsFocused();
					pCategory->OnCursorLeave();
					if (pCategory->IsFocused() != fFocused)
						RedrawCategoryItem(m_CurCategory);
				}
				*/
				if (Part == PartType::Category) {
					if (m_CurCategory != CategoryIndex) {
						SetCurCategory(CategoryIndex);
					}
				}
			}
		}
		return 0;

	case WM_LBUTTONUP:
		{
			const int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

			if (m_fHitCloseButton) {
				if (CloseButtonHitTest(x, y)) {
					Close();
				}
			} else {
				int Category;
				const PartType Part = HitTest(x, y, &Category);

				switch (Part) {
				case PartType::Category:
					SetCurCategory(Category);
					break;

				case PartType::Content:
					if (m_CategoryList[m_CurCategory]->OnLButtonUp(x, y)) {
						Close();
					}
					break;
				}
			}
		}
		return 0;

	case WM_RBUTTONUP:
		{
			const POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			RECT rc;

			GetContentAreaRect(&rc);
			if (m_CurCategory >= 0
					&& ::PtInRect(&rc, pt)
					&& m_CategoryList[m_CurCategory]->OnRButtonUp(pt.x, pt.y)) {
				return 0;
			}
		}
		break;

	case WM_SETCURSOR:
		if (reinterpret_cast<HWND>(wParam) == hwnd
				&& LOWORD(lParam) == HTCLIENT) {
			if (m_CursorPart == PartType::Content && m_CurCategory >= 0) {
				if (m_CategoryList[m_CurCategory]->OnSetCursor())
					return TRUE;
			}
			::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
			return TRUE;
		}
		break;

	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		if (OnMouseWheel(uMsg, wParam, lParam))
			return 0;
		break;

	case WM_VSCROLL:
		{
			int Pos = m_ScrollPos;
			RECT rcContent;
			GetContentAreaRect(&rcContent);
			const int PageSize = rcContent.bottom - rcContent.top;

			switch (LOWORD(wParam)) {
			case SB_LINEUP:        Pos--;                                         break;
			case SB_LINEDOWN:      Pos++;                                         break;
			case SB_PAGEUP:        Pos -= PageSize;                               break;
			case SB_PAGEDOWN:      Pos += PageSize;                               break;
			case SB_TOP:           Pos = 0;                                       break;
			case SB_BOTTOM:        Pos = std::max(m_ContentHeight - PageSize, 0); break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:    Pos = HIWORD(wParam);                          break;
			default:               return 0;
			}
			SetScrollPos(Pos, true);
		}
		return 0;

	case WM_KEYDOWN:
		switch (wParam) {
		case VK_LEFT:
			if (m_CurCategory >= 0 && m_CategoryList[m_CurCategory]->IsFocused()) {
				m_CategoryList[m_CurCategory]->SetFocus(false);
				RedrawCategoryItem(m_CurCategory);
			}
			break;

		case VK_RIGHT:
			if (m_CurCategory >= 0 && !m_CategoryList[m_CurCategory]->IsFocused()) {
				if (m_CategoryList[m_CurCategory]->SetFocus(true))
					RedrawCategoryItem(m_CurCategory);
			}
			break;

		case VK_UP:
		case VK_DOWN:
			{
				CCategory *pCategory = m_CategoryList[m_CurCategory].get();

				if (pCategory->IsFocused()) {
					if (pCategory->OnCursorKey(wParam)) {
						ScrollToCurItem();
					}
				} else {
					int Category = m_CurCategory;

					if (wParam == VK_UP) {
						if (Category < 1)
							break;
						Category--;
					} else {
						Category++;
					}
					SetCurCategory(Category);
				}
			}
			break;

		case VK_RETURN:
		case VK_SPACE:
			{
				CCategory *pCategory = m_CategoryList[m_CurCategory].get();

				if (pCategory->IsFocused()) {
					if (pCategory->OnDecide()) {
						Close();
					}
				}
			}
			break;

		case VK_PRIOR:
		case VK_NEXT:
		case VK_HOME:
		case VK_END:
			{
				static const struct {
					WORD KeyCode;
					WORD Scroll;
				} KeyMap[] = {
					{VK_PRIOR, SB_PAGEUP},
					{VK_NEXT,  SB_PAGEDOWN},
					{VK_HOME,  SB_TOP},
					{VK_END,   SB_BOTTOM},
				};
				int i;
				for (i = 0; KeyMap[i].KeyCode != wParam; i++);
				::SendMessage(hwnd, WM_VSCROLL, KeyMap[i].Scroll, reinterpret_cast<LPARAM>(m_hwndScroll));
			}
			break;

		case VK_TAB:
			{
				int Category = m_CurCategory;

				if (::GetKeyState(VK_SHIFT) < 0) {
					if (Category < 1)
						break;
					Category--;
				} else {
					Category++;
				}
				SetCurCategory(Category);
			}
			break;

		case VK_ESCAPE:
			Close();
			break;
		}
		return 0;

	case WM_DESTROY:
		for (auto &e : m_CategoryList)
			e->OnWindowDestroy();

		if (m_himlIcons != nullptr) {
			::ImageList_Destroy(m_himlIcons);
			m_himlIcons = nullptr;
		}
		return 0;
	}

	return CDisplayView::OnMessage(hwnd, uMsg, wParam, lParam);
}


void CHomeDisplay::ApplyStyle()
{
	if (m_hwnd != nullptr) {
		if (!m_fAutoFontSize)
			CreateDrawFont(m_StyleFont, &m_Font);
	}
}


void CHomeDisplay::RealizeStyle()
{
	if (m_hwnd != nullptr) {
		LayOut();
		Invalidate();
	}
}


void CHomeDisplay::Draw(HDC hdc, const RECT &PaintRect)
{
	RECT rcClient;
	GetClientRect(&rcClient);

	const int OldBkMode = ::SetBkMode(hdc, TRANSPARENT);
	const COLORREF OldTextColor = ::GetTextColor(hdc);
	const HFONT hfontOld = DrawUtil::SelectObject(hdc, m_Font);

	const CCategory *pCategory = nullptr;
	if (m_CurCategory >= 0)
		pCategory = m_CategoryList[m_CurCategory].get();

	Theme::CThemeDraw ThemeDraw(BeginThemeDraw(hdc));

	if (PaintRect.left < m_CategoriesAreaWidth) {
		RECT rcCategories = rcClient;
		rcCategories.right = m_CategoriesAreaWidth;
		ThemeDraw.Draw(m_HomeDisplayStyle.CategoriesBackStyle, rcCategories);

		RECT rcItem;
		rcItem.left = rcCategories.left + m_Style.CategoriesMargin.Left;
		rcItem.right = rcItem.left + m_CategoryItemWidth;
		rcItem.top = rcCategories.top + m_Style.CategoriesMargin.Top;

		for (size_t i = 0; i < m_CategoryList.size(); i++) {
			const CCategory *pCategory = m_CategoryList[i].get();
			const Theme::Style *pStyle =
				(static_cast<int>(i) == m_CurCategory) ?
					(pCategory->IsFocused() ? &m_HomeDisplayStyle.CategoryItemSelStyle : &m_HomeDisplayStyle.CategoryItemCurStyle) :
					(&m_HomeDisplayStyle.CategoryItemStyle);

			rcItem.bottom = rcItem.top + m_CategoryItemHeight;
			ThemeDraw.Draw(pStyle->Back, rcItem);
			RECT rc = rcItem;
			Style::Subtract(&rc, m_HomeDisplayStyle.CategoryItemMargins);
			::ImageList_Draw(
				m_himlIcons, pCategory->GetIconIndex(),
				hdc, rc.left, rc.top + ((rc.bottom - rc.top) - CATEGORY_ICON_HEIGHT) / 2, ILD_TRANSPARENT);
			rc.left += CATEGORY_ICON_WIDTH + m_HomeDisplayStyle.CategoryIconMargin;
			ThemeDraw.Draw(
				pStyle->Fore, rc, pCategory->GetTitle(),
				DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
			rcItem.top = rcItem.bottom;
		}
	}

	if (PaintRect.right > m_CategoriesAreaWidth) {
		RECT rcContent = rcClient;
		rcContent.left = m_CategoriesAreaWidth;
		ThemeDraw.Draw(m_HomeDisplayStyle.ContentBackStyle, rcContent);

		if (pCategory != nullptr) {
			Style::Subtract(&rcContent, m_Style.ContentMargin);
			if (rcContent.left < rcContent.right) {
				const HRGN hrgn = ::CreateRectRgnIndirect(&rcContent);
				::SelectClipRgn(hdc, hrgn);

				::OffsetRect(&rcContent, 0, -m_ScrollPos);
				pCategory->Draw(hdc, m_HomeDisplayStyle, rcContent, PaintRect, ThemeDraw);

				::SelectClipRgn(hdc, nullptr);
				::DeleteObject(hrgn);
			}
		}
	}

	::SelectObject(hdc, hfontOld);
	::SetTextColor(hdc, OldTextColor);
	::SetBkMode(hdc, OldBkMode);

	DrawCloseButton(hdc);
}


void CHomeDisplay::LayOut()
{
	RECT rcClient;
	GetClientRect(&rcClient);

	if (m_fAutoFontSize) {
		LOGFONT lf = m_StyleFont.LogFont;
		lf.lfHeight = -GetDefaultFontSize(rcClient.right, rcClient.bottom);
		lf.lfWidth = 0;
		m_Font.Create(&lf);
	}

	const HDC hdc = ::GetDC(m_hwnd);
	const HFONT hfontOld = DrawUtil::SelectObject(hdc, m_Font);

	m_HomeDisplayStyle.FontHeight = m_Font.GetHeight(hdc);

	int CategoryTextWidth = 0;
	for (const auto &e : m_CategoryList) {
		RECT rc = {0, 0, 0, 0};
		::DrawText(hdc, e->GetTitle(), -1, &rc, DT_CALCRECT);
		if (rc.right > CategoryTextWidth)
			CategoryTextWidth = rc.right;
	}
	m_CategoryItemWidth =
		CategoryTextWidth + CATEGORY_ICON_WIDTH + m_HomeDisplayStyle.CategoryIconMargin +
		m_HomeDisplayStyle.CategoryItemMargins.Horz();
	m_CategoryItemHeight =
		std::max(m_HomeDisplayStyle.FontHeight, CATEGORY_ICON_HEIGHT) +
		m_HomeDisplayStyle.CategoryItemMargins.Vert();
	m_CategoriesAreaWidth = m_CategoryItemWidth + m_Style.CategoriesMargin.Horz();

	CCategory *pCategory = m_CategoryList[m_CurCategory].get();
	RECT rcContent;
	GetContentAreaRect(&rcContent);
	pCategory->LayOut(m_HomeDisplayStyle, hdc, rcContent);
	m_ContentHeight = pCategory->GetHeight();

	::SelectObject(hdc, hfontOld);
	::ReleaseDC(m_hwnd, hdc);

	SetScrollBar();
}


void CHomeDisplay::SetScrollBar()
{
	RECT rcContent;
	GetContentAreaRect(&rcContent);
	const int ContentAreaHeight = rcContent.bottom - rcContent.top;

	if (ContentAreaHeight < m_ContentHeight) {
		SCROLLINFO si;

		if (m_ScrollPos > m_ContentHeight - ContentAreaHeight)
			m_ScrollPos = m_ContentHeight - ContentAreaHeight;
		si.cbSize = sizeof(si);
		si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
		si.nMin = 0;
		si.nMax = m_ContentHeight - 1;
		si.nPage = ContentAreaHeight;
		si.nPos = m_ScrollPos;
		::SetScrollInfo(m_hwndScroll, SB_CTL, &si, TRUE);
		const int ScrollWidth = m_pStyleScaling->GetScaledSystemMetrics(SM_CXVSCROLL);
		::MoveWindow(
			m_hwndScroll,
			rcContent.right, rcContent.top,
			ScrollWidth, rcContent.bottom - rcContent.top, TRUE);
		::ShowWindow(m_hwndScroll, SW_SHOW);
	} else {
		m_ScrollPos = 0;
		::ShowWindow(m_hwndScroll, SW_HIDE);
	}
}


void CHomeDisplay::GetContentAreaRect(RECT *pRect) const
{
	GetClientRect(pRect);
	Style::Subtract(pRect, m_Style.ContentMargin);
	pRect->left += m_CategoriesAreaWidth;
	pRect->right -= m_pStyleScaling->GetScaledSystemMetrics(SM_CXVSCROLL);
	if (pRect->right < pRect->left)
		pRect->right = pRect->left;
	if (pRect->bottom < pRect->top)
		pRect->bottom = pRect->top;
}


CHomeDisplay::PartType CHomeDisplay::HitTest(int x, int y, int *pCategoryIndex) const
{
	const POINT pt = {x, y};

	RECT rcCategories;
	rcCategories.left = m_Style.CategoriesMargin.Left;
	rcCategories.top = m_Style.CategoriesMargin.Top;
	rcCategories.right = rcCategories.left + m_CategoriesAreaWidth;
	rcCategories.bottom = rcCategories.top + m_CategoryItemHeight * static_cast<int>(m_CategoryList.size());
	if (::PtInRect(&rcCategories, pt)) {
		if (pCategoryIndex != nullptr)
			*pCategoryIndex = (pt.y - rcCategories.top) / m_CategoryItemHeight;
		return PartType::Category;
	}

	RECT rcContent;
	GetContentAreaRect(&rcContent);
	if (rcContent.bottom - rcContent.top > m_ContentHeight)
		rcContent.bottom = rcContent.top + m_ContentHeight;
	if (::PtInRect(&rcContent, pt)) {
		return PartType::Content;
	}

	return PartType::Margin;
}


void CHomeDisplay::ScrollToCurItem()
{
	if (m_CurCategory >= 0) {
		RECT rcItem;

		if (m_CategoryList[m_CurCategory]->GetCurItemRect(&rcItem)) {
			RECT rcContent;
			int Pos;

			GetContentAreaRect(&rcContent);
			if (rcItem.top < rcContent.top) {
				Pos = rcItem.top - rcContent.top + m_ScrollPos;
			} else if (rcItem.bottom > rcContent.bottom) {
				Pos =
					(rcItem.bottom - rcContent.top + m_ScrollPos) -
					(rcContent.bottom - rcContent.top);
			} else {
				return;
			}
			SetScrollPos(Pos, true);
		}
	}
}


bool CHomeDisplay::GetCategoryItemRect(int Category, RECT *pRect) const
{
	if (Category < 0 || static_cast<size_t>(Category) >= m_CategoryList.size())
		return false;

	pRect->left = m_Style.CategoriesMargin.Left;
	pRect->top = m_Style.CategoriesMargin.Top + (Category * m_CategoryItemHeight);
	pRect->right = pRect->left + m_CategoryItemWidth;
	pRect->bottom = pRect->top + m_CategoryItemHeight;

	return true;
}


bool CHomeDisplay::RedrawCategoryItem(int Category)
{
	RECT rc;

	if (!GetCategoryItemRect(Category, &rc))
		return false;
	Invalidate(&rc);
	return true;
}




CHomeDisplay::CCategory::CCategory(CHomeDisplay *pHomeDisplay)
	: m_pHomeDisplay(pHomeDisplay)
{
}


} // namespace TVTest
