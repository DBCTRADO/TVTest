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
#include "ProgramListPanel.h"
#include "AppMain.h"
#include "LogoManager.h"
#include "DrawUtil.h"
#include "TextDraw.h"
#include "DarkMode.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


class CProgramItemInfo
{
public:
	CProgramItemInfo(const LibISDB::EventInfo &EventInfo);

	const LibISDB::EventInfo &GetEventInfo() const { return m_EventInfo; }
	WORD GetEventID() const { return m_EventID; }
	int GetTitleLines() const { return m_NameLines; }
	int GetTextLines() const { return m_TextLines; }
	int GetLines() const { return m_NameLines + m_TextLines; }
	int CalcTitleLines(CTextDraw &DrawText, int Width, bool fUseARIBSymbol);
	int CalcTextLines(CTextDraw &DrawText, int Width, bool fUseARIBSymbol);
	void DrawTitle(CTextDraw &DrawText, const RECT &Rect, int LineHeight, bool fUseARIBSymbol);
	void DrawText(CTextDraw &DrawText, const RECT &Rect, int LineHeight, bool fUseARIBSymbol);
	SIZE GetTimeSize(HDC hdc) const;
	bool IsChanged(const CProgramItemInfo *pItem) const;

private:
	static constexpr size_t MAX_EVENT_TITLE = 256;

	LibISDB::EventInfo m_EventInfo;
	WORD m_EventID;
	int m_NameLines = 0;
	int m_TextLines = 0;

	String GetEventText(bool fUseARIBSymbol) const;
	void GetEventTitleText(LPTSTR pszText, int MaxLength, bool fUseARIBSymbol) const;
	int GetEventTimeText(LPTSTR pszText, int MaxLength) const;
};


CProgramItemInfo::CProgramItemInfo(const LibISDB::EventInfo &EventInfo)
	: m_EventInfo(EventInfo)
	, m_EventID(EventInfo.EventID)
{
}


int CProgramItemInfo::CalcTitleLines(CTextDraw &DrawText, int Width, bool fUseARIBSymbol)
{
	TCHAR szText[MAX_EVENT_TITLE];

	GetEventTitleText(szText, lengthof(szText), fUseARIBSymbol);
	m_NameLines = DrawText.CalcLineCount(szText, Width);
	return m_NameLines;
}


int CProgramItemInfo::CalcTextLines(CTextDraw &DrawText, int Width, bool fUseARIBSymbol)
{
	const String Text = GetEventText(fUseARIBSymbol);

	if (!Text.empty())
		m_TextLines = DrawText.CalcLineCount(Text.c_str(), Width);
	else
		m_TextLines = 0;
	return m_TextLines;
}


void CProgramItemInfo::DrawTitle(CTextDraw &DrawText, const RECT &Rect, int LineHeight, bool fUseARIBSymbol)
{
	TCHAR szText[MAX_EVENT_TITLE];

	GetEventTitleText(szText, lengthof(szText), fUseARIBSymbol);
	DrawText.Draw(szText, Rect, LineHeight);
}


void CProgramItemInfo::DrawText(CTextDraw &DrawText, const RECT &Rect, int LineHeight, bool fUseARIBSymbol)
{
	const String Text = GetEventText(fUseARIBSymbol);
	if (!Text.empty()) {
		DrawText.Draw(Text.c_str(), Rect, LineHeight);
	}
}


SIZE CProgramItemInfo::GetTimeSize(HDC hdc) const
{
	TCHAR szTime[EpgUtil::MAX_EVENT_TIME_LENGTH];
	SIZE sz;

	const int Length = GetEventTimeText(szTime, lengthof(szTime));
	::GetTextExtentPoint32(hdc, szTime, Length, &sz);
	return sz;
}


bool CProgramItemInfo::IsChanged(const CProgramItemInfo *pItem) const
{
	return m_EventID != pItem->m_EventID
		|| m_EventInfo.StartTime != pItem->m_EventInfo.StartTime
		|| m_EventInfo.Duration != pItem->m_EventInfo.Duration;
}


String CProgramItemInfo::GetEventText(bool fUseARIBSymbol) const
{
	return EpgUtil::GetEventDisplayText(m_EventInfo, fUseARIBSymbol);
}


void CProgramItemInfo::GetEventTitleText(LPTSTR pszText, int MaxLength, bool fUseARIBSymbol) const
{
	int Length = GetEventTimeText(pszText, MaxLength);

	if (Length + 2 < MaxLength) {
		pszText[Length++] = TEXT(' ');

		if (fUseARIBSymbol)
			EpgUtil::MapARIBSymbol(m_EventInfo.EventName.c_str(), pszText + Length, MaxLength - Length);
		else
			StringFormat(pszText + Length, MaxLength - Length, TEXT("{}"), m_EventInfo.EventName);
	}
}


int CProgramItemInfo::GetEventTimeText(LPTSTR pszText, int MaxLength) const
{
	return EpgUtil::FormatEventTime(
		m_EventInfo, pszText, MaxLength,
		EpgUtil::FormatEventTimeFlag::Hour2Digits);
}




CProgramItemInfo *CProgramItemList::GetItem(int Index)
{
	if (static_cast<unsigned int>(Index) >= m_ItemList.size())
		return nullptr;
	return m_ItemList[Index].get();
}


const CProgramItemInfo *CProgramItemList::GetItem(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_ItemList.size())
		return nullptr;
	return m_ItemList[Index].get();
}


bool CProgramItemList::Add(CProgramItemInfo *pItem)
{
	m_ItemList.emplace_back(pItem);
	return true;
}


void CProgramItemList::Clear()
{
	m_ItemList.clear();
}


void CProgramItemList::Attach(CProgramItemList *pList)
{
	m_ItemList.swap(pList->m_ItemList);
	pList->Clear();
}




const LPCTSTR CProgramListPanel::m_pszClassName = APP_NAME TEXT(" Program List Panel");
HINSTANCE CProgramListPanel::m_hinst = nullptr;


bool CProgramListPanel::Initialize(HINSTANCE hinst)
{
	if (m_hinst == nullptr) {
		WNDCLASS wc;

		wc.style = CS_HREDRAW;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hinst;
		wc.hIcon = nullptr;
		wc.hCursor = nullptr;
		wc.hbrBackground = nullptr;
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = m_pszClassName;
		if (::RegisterClass(&wc) == 0)
			return false;
		m_hinst = hinst;
	}
	return true;
}


CProgramListPanel::CProgramListPanel()
	: m_EventInfoPopupManager(&m_EventInfoPopup)
{
	GetDefaultFont(&m_StyleFont);
}


CProgramListPanel::~CProgramListPanel()
{
	Destroy();
}


bool CProgramListPanel::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	return CreateBasicWindow(
		hwndParent, Style, ExStyle, ID,
		m_pszClassName, TEXT("番組表"), m_hinst);
}


void CProgramListPanel::SetStyle(const Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);
}


void CProgramListPanel::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	m_Style.NormalizeStyle(pStyleManager, pStyleScaling);

	if (m_OldDPI == 0)
		m_OldDPI = pStyleScaling->GetDPI();
}


void CProgramListPanel::SetTheme(const Theme::CThemeManager *pThemeManager)
{
	ProgramListPanelTheme Theme;

	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_PROGRAMLISTPANEL_CHANNEL,
		&Theme.ChannelNameStyle);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_PROGRAMLISTPANEL_CURCHANNEL,
		&Theme.CurChannelNameStyle);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_PROGRAMLISTPANEL_CHANNELBUTTON,
		&Theme.ChannelButtonStyle);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_PROGRAMLISTPANEL_CHANNELBUTTON_HOT,
		&Theme.ChannelButtonHotStyle);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_PROGRAMLISTPANEL_EVENT,
		&Theme.EventTextStyle);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_PROGRAMLISTPANEL_CUREVENT,
		&Theme.CurEventTextStyle);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_PROGRAMLISTPANEL_TITLE,
		&Theme.EventNameStyle);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_PROGRAMLISTPANEL_CURTITLE,
		&Theme.CurEventNameStyle);
	Theme.MarginColor =
		pThemeManager->GetColor(CColorScheme::COLOR_PANELBACK);
	pThemeManager->GetBackgroundStyle(
		Theme::CThemeManager::STYLE_PROGRAMGUIDE_FEATUREDMARK,
		&Theme.FeaturedMarkStyle);

	SetProgramListPanelTheme(Theme);

	m_EpgTheme.SetTheme(pThemeManager);
	m_EventInfoPopup.SetTheme(pThemeManager);
}


bool CProgramListPanel::SetFont(const Style::Font &Font)
{
	m_StyleFont = Font;

	if (m_hwnd != nullptr) {
		ApplyStyle();
		RealizeStyle();
	}

	return true;
}


bool CProgramListPanel::ReadSettings(CSettings &Settings)
{
	Settings.Read(TEXT("ProgramListPanel.MouseOverEventInfo"), &m_fMouseOverEventInfo);
	Settings.Read(TEXT("ProgramListPanel.UseEpgColorScheme"), &m_fUseEpgColorScheme);
	Settings.Read(TEXT("ProgramListPanel.ShowFeaturedMark"), &m_fShowFeaturedMark);

	int PopupWidth, PopupHeight;
	if (Settings.Read(TEXT("ProgramListPanel.PopupEventInfoWidth"), &PopupWidth)
			&& Settings.Read(TEXT("ProgramListPanel.PopupEventInfoHeight"), &PopupHeight))
		m_EventInfoPopup.SetSize(PopupWidth, PopupHeight);

	return true;
}


bool CProgramListPanel::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("ProgramListPanel.MouseOverEventInfo"), m_fMouseOverEventInfo);
	Settings.Write(TEXT("ProgramListPanel.UseEpgColorScheme"), m_fUseEpgColorScheme);
	Settings.Write(TEXT("ProgramListPanel.ShowFeaturedMark"), m_fShowFeaturedMark);

	int PopupWidth, PopupHeight;
	m_EventInfoPopup.GetSize(&PopupWidth, &PopupHeight);
	Settings.Write(TEXT("ProgramListPanel.PopupEventInfoWidth"), PopupWidth);
	Settings.Write(TEXT("ProgramListPanel.PopupEventInfoHeight"), PopupHeight);

	return true;
}


bool CProgramListPanel::UpdateProgramList(const CChannelInfo *pChannelInfo)
{
	if (m_pEPGDatabase == nullptr || pChannelInfo == nullptr)
		return false;
	if (m_hwnd != nullptr
			&& m_SelectedChannel.GetNetworkID() == pChannelInfo->GetNetworkID()
			&& m_SelectedChannel.GetTransportStreamID() == pChannelInfo->GetTransportStreamID()
			&& m_SelectedChannel.GetServiceID() == pChannelInfo->GetServiceID()) {
		const bool fRetrieving = m_fShowRetrievingMessage;

		m_fShowRetrievingMessage = false;
		if (UpdateListInfo(pChannelInfo)) {
			CalcDimensions();
			SetScrollBar();
			//SetToolTip();
			RECT rc;
			GetProgramListRect(&rc);
			Invalidate(&rc);
		} else if (fRetrieving) {
			Invalidate();
		}
	}
	return true;
}


bool CProgramListPanel::UpdateListInfo(const CChannelInfo *pChannelInfo)
{
	if (m_pEPGDatabase == nullptr || pChannelInfo == nullptr)
		return false;

	LibISDB::DateTime Earliest;
	LibISDB::GetCurrentEPGTime(&Earliest);
	Earliest.TruncateToMinutes();
	LibISDB::DateTime Latest = Earliest;
	Latest.OffsetHours(24);

	CProgramItemList NewItemList;

	m_pEPGDatabase->EnumEventsSortedByTime(
		pChannelInfo->GetNetworkID(),
		pChannelInfo->GetTransportStreamID(),
		pChannelInfo->GetServiceID(),
		&Earliest, &Latest,
		[&](const LibISDB::EventInfo & Event) -> bool {
			NewItemList.Add(new CProgramItemInfo(Event));
			return true;
		});

	if (NewItemList.NumItems() == 0) {
		if (m_ItemList.NumItems() > 0) {
			m_ItemList.Clear();
			return true;
		}
		return false;
	}

	bool fChanged;
	if (NewItemList.NumItems() != m_ItemList.NumItems()) {
		fChanged = true;
	} else {
		fChanged = false;
		for (int i = 0; i < m_ItemList.NumItems(); i++) {
			if (m_ItemList.GetItem(i)->IsChanged(NewItemList.GetItem(i))) {
				fChanged = true;
				break;
			}
		}
	}
	if (!fChanged)
		return false;
	m_ItemList.Attach(&NewItemList);

	return true;
}


void CProgramListPanel::ClearProgramList()
{
	if (m_ItemList.NumItems() > 0) {
		m_ItemList.Clear();
		m_ScrollPos = 0;
		m_TotalLines = 0;
		if (m_hwnd != nullptr) {
			SetScrollBar();
			//SetToolTip();
			Invalidate();
		}
	}
}


void CProgramListPanel::SelectChannel(const CChannelInfo *pChannelInfo, bool fUpdate)
{
	ClearProgramList();
	if (pChannelInfo != nullptr) {
		m_SelectedChannel = *pChannelInfo;
		if (fUpdate)
			UpdateProgramList(pChannelInfo);
	} else {
		m_SelectedChannel = CChannelInfo();
	}
}


void CProgramListPanel::SetCurrentChannel(const CChannelInfo *pChannelInfo)
{
	if (pChannelInfo != nullptr)
		m_CurChannel = *pChannelInfo;
	else
		m_CurChannel = CChannelInfo();
	m_CurEventID = -1;
	Invalidate();
}


void CProgramListPanel::SetCurrentEventID(int EventID)
{
	m_CurEventID = EventID;
	if (m_hwnd != nullptr)
		Invalidate();
}


void CProgramListPanel::GetHeaderRect(RECT *pRect) const
{
	GetClientRect(pRect);
	pRect->bottom = m_ChannelHeight;
}


void CProgramListPanel::GetChannelButtonRect(RECT *pRect) const
{
	GetHeaderRect(pRect);
	Style::Subtract(pRect, m_Style.ChannelPadding);
	const int Width = m_Style.ChannelButtonIconSize.Width + m_Style.ChannelButtonPadding.Horz();
	const int Height = m_Style.ChannelButtonIconSize.Height + m_Style.ChannelButtonPadding.Vert();
	pRect->left = pRect->right - Width;
	pRect->top = pRect->top + ((pRect->bottom - pRect->top) - Height) / 2;
	pRect->bottom = pRect->top + Height;
}


void CProgramListPanel::GetProgramListRect(RECT *pRect) const
{
	GetClientRect(pRect);
	pRect->top = m_ChannelHeight;
	if (pRect->bottom < pRect->top)
		pRect->bottom = pRect->top;
}


void CProgramListPanel::CalcChannelHeight()
{
	const int LabelHeight = m_FontHeight + m_Style.ChannelNameMargin.Vert();
	const int ButtonHeight = m_Style.ChannelButtonIconSize.Height + m_Style.ChannelButtonPadding.Vert();
	m_ChannelHeight = std::max(LabelHeight, ButtonHeight) + m_Style.ChannelPadding.Vert();
}


void CProgramListPanel::CalcDimensions()
{
	const HDC hdc = ::GetDC(m_hwnd);
	CTextDraw DrawText;
	RECT rc;
	GetClientRect(&rc);
	DrawText.Begin(hdc, rc, CTextDraw::Flag::JapaneseHyphnation);
	GetProgramListRect(&rc);
	const HFONT hfontOld = static_cast<HFONT>(::GetCurrentObject(hdc, OBJ_FONT));
	m_TotalLines = 0;
	for (int i = 0; i < m_ItemList.NumItems(); i++) {
		CProgramItemInfo *pItem = m_ItemList.GetItem(i);

		DrawUtil::SelectObject(hdc, m_TitleFont);
		m_TotalLines += pItem->CalcTitleLines(DrawText, rc.right, m_fUseARIBSymbol);
		DrawUtil::SelectObject(hdc, m_Font);
		m_TotalLines += pItem->CalcTextLines(DrawText, rc.right - GetTextLeftMargin(), m_fUseARIBSymbol);
	}
	::SelectObject(hdc, hfontOld);
	DrawText.End();
	::ReleaseDC(m_hwnd, hdc);
}


void CProgramListPanel::SetScrollPos(int Pos)
{
	RECT rc;

	GetProgramListRect(&rc);
	const int Page = rc.bottom - rc.top;
	if (Pos < 0) {
		Pos = 0;
	} else {
		int Max =
			m_TotalLines * (m_FontHeight + m_Style.LineSpacing) +
			m_ItemList.NumItems() * (m_Style.TitlePadding.Top + m_Style.TitlePadding.Bottom - m_Style.LineSpacing) - Page;
		if (Max < 0)
			Max = 0;
		if (Pos > Max)
			Pos = Max;
	}
	if (Pos != m_ScrollPos) {
		const int Offset = Pos - m_ScrollPos;
		SCROLLINFO si;

		m_ScrollPos = Pos;
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_POS;
		si.nPos = Pos;
		::SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
		::ScrollWindowEx(m_hwnd, 0, -Offset, &rc, &rc, nullptr, nullptr, SW_ERASE | SW_INVALIDATE);
		//SetToolTip();
	}
}


void CProgramListPanel::SetScrollBar()
{
	SCROLLINFO si;
	RECT rc;

	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS | SIF_DISABLENOSCROLL;
	si.nMin = 0;
	si.nMax =
		m_TotalLines < 1 ? 0 :
			m_TotalLines * (m_FontHeight + m_Style.LineSpacing) +
			m_ItemList.NumItems() * (m_Style.TitlePadding.Top + m_Style.TitlePadding.Bottom - m_Style.LineSpacing);
	GetProgramListRect(&rc);
	si.nPage = rc.bottom - rc.top;
	si.nPos = m_ScrollPos;
	::SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
}


bool CProgramListPanel::SetProgramListPanelTheme(const ProgramListPanelTheme &Theme)
{
	m_Theme = Theme;

	if (m_hwnd != nullptr) {
		if (IsDarkThemeSupported()) {
			SetWindowDarkTheme(m_hwnd, IsDarkThemeColor(m_Theme.MarginColor));
		}

		Invalidate();
	}

	return true;
}


bool CProgramListPanel::GetProgramListPanelTheme(ProgramListPanelTheme *pTheme) const
{
	if (pTheme == nullptr)
		return false;
	*pTheme = m_Theme;
	return true;
}


bool CProgramListPanel::SetEventInfoFont(const Style::Font &Font)
{
	return m_EventInfoPopup.SetFont(Font);
}


void CProgramListPanel::CalcFontHeight()
{
	const HDC hdc = ::GetDC(m_hwnd);
	if (hdc == nullptr)
		return;
	m_FontHeight = m_Font.GetHeight();
	::ReleaseDC(m_hwnd, hdc);
}


int CProgramListPanel::GetTextLeftMargin() const
{
	return m_Style.IconSize.Width + m_Style.IconMargin.Left + m_Style.IconMargin.Right;
}


void CProgramListPanel::ShowRetrievingMessage(bool fShow)
{
	if (m_fShowRetrievingMessage != fShow) {
		m_fShowRetrievingMessage = fShow;
		if (m_hwnd != nullptr)
			Invalidate();
	}
}


void CProgramListPanel::SetVisibleEventIcons(UINT VisibleIcons)
{
	if (m_VisibleEventIcons != VisibleIcons) {
		m_VisibleEventIcons = VisibleIcons;
		if (m_hwnd != nullptr)
			Invalidate();
	}
}


void CProgramListPanel::SetMouseOverEventInfo(bool fMouseOverEventInfo)
{
	if (m_fMouseOverEventInfo != fMouseOverEventInfo) {
		m_fMouseOverEventInfo = fMouseOverEventInfo;
		m_EventInfoPopupManager.SetEnable(fMouseOverEventInfo);
	}
}


void CProgramListPanel::SetUseEpgColorScheme(bool fUseEpgColorScheme)
{
	if (m_fUseEpgColorScheme != fUseEpgColorScheme) {
		m_fUseEpgColorScheme = fUseEpgColorScheme;
		if (m_hwnd != nullptr)
			Invalidate();
	}
}


void CProgramListPanel::SetShowFeaturedMark(bool fShowFeaturedMark)
{
	if (m_fShowFeaturedMark != fShowFeaturedMark) {
		m_fShowFeaturedMark = fShowFeaturedMark;
		if (m_hwnd != nullptr) {
			if (m_fShowFeaturedMark)
				m_FeaturedEventsMatcher.BeginMatching(GetAppClass().FeaturedEvents.GetSettings());
			Invalidate();
		}
	}
}


void CProgramListPanel::SetUseARIBSymbol(bool fUseARIBSymbol)
{
	if (m_fUseARIBSymbol != fUseARIBSymbol) {
		m_fUseARIBSymbol = fUseARIBSymbol;
		if (m_hwnd != nullptr) {
			CalcDimensions();
			SetScrollBar();
			Invalidate();
		}
	}
}


int CProgramListPanel::ItemHitTest(int x, int y) const
{
	const POINT pt = {x, y};
	RECT rcHeader;
	int HotItem = -1;

	GetHeaderRect(&rcHeader);
	if (::PtInRect(&rcHeader, pt)) {
		RECT rc;
		GetChannelButtonRect(&rc);
		if (::PtInRect(&rc, pt)) {
			HotItem = ITEM_CHANNELLISTBUTTON;
		} else if (pt.x < rc.left - m_Style.ChannelButtonMargin) {
			HotItem = ITEM_CHANNEL;
		}
	}

	return HotItem;
}


int CProgramListPanel::ProgramHitTest(int x, int y) const
{
	const POINT pt = {x, y};
	RECT rc;

	GetProgramListRect(&rc);
	if (!::PtInRect(&rc, pt))
		return -1;
	rc.top -= m_ScrollPos;
	for (int i = 0; i < m_ItemList.NumItems(); i++) {
		const CProgramItemInfo *pItem = m_ItemList.GetItem(i);

		rc.bottom =
			rc.top + (pItem->GetTitleLines() + pItem->GetTextLines()) * (m_FontHeight + m_Style.LineSpacing) +
			(m_Style.TitlePadding.Top + m_Style.TitlePadding.Bottom - m_Style.LineSpacing);
		if (::PtInRect(&rc, pt))
			return i;
		rc.top = rc.bottom;
	}
	return -1;
}


bool CProgramListPanel::GetItemRect(int Item, RECT *pRect) const
{
	if (Item < 0 || Item >= m_ItemList.NumItems())
		return false;

	RECT rc;

	GetProgramListRect(&rc);
	rc.top -= m_ScrollPos;
	for (int i = 0;; i++) {
		const CProgramItemInfo *pItem = m_ItemList.GetItem(i);

		rc.bottom =
			rc.top + (pItem->GetTitleLines() + pItem->GetTextLines()) * (m_FontHeight + m_Style.LineSpacing) +
			(m_Style.TitlePadding.Top + m_Style.TitlePadding.Bottom - m_Style.LineSpacing);
		if (i == Item)
			break;
		rc.top = rc.bottom;
	}

	*pRect = rc;

	return true;
}


void CProgramListPanel::SetHotItem(int Item)
{
	if (m_HotItem != Item) {
		m_HotItem = Item;
		RECT rc;
		GetHeaderRect(&rc);
		Invalidate(&rc);
	}
}


void CProgramListPanel::ShowChannelListMenu()
{
	const CAppMain &App = GetAppClass();
	const CChannelList *pChannelList = App.ChannelManager.GetCurrentChannelList();
	if (pChannelList == nullptr)
		return;

	CChannelList ChannelList;

	int ItemCount = 0, CurChannel = -1, SelectedChannel = -1;

	for (int i = 0; i < pChannelList->NumChannels(); i++) {
		const CChannelInfo *pChannelInfo = pChannelList->GetChannelInfo(i);

		if (pChannelInfo->IsEnabled()
				&& pChannelInfo->GetServiceID() > 0) {
			ChannelList.AddChannel(*pChannelInfo);

			if (CurChannel < 0
					&& m_CurChannel.GetServiceID() > 0
					&& m_CurChannel.GetNetworkID() == pChannelInfo->GetNetworkID()
					&& m_CurChannel.GetTransportStreamID() == pChannelInfo->GetTransportStreamID()
					&& m_CurChannel.GetServiceID() == pChannelInfo->GetServiceID())
				CurChannel = ItemCount;
			if (SelectedChannel < 0
					&& m_SelectedChannel.GetNetworkID() == pChannelInfo->GetNetworkID()
					&& m_SelectedChannel.GetTransportStreamID() == pChannelInfo->GetTransportStreamID()
					&& m_SelectedChannel.GetServiceID() == pChannelInfo->GetServiceID())
				SelectedChannel = ItemCount;
			ItemCount++;
		}
	}

	if (ItemCount == 0)
		return;

	const bool fDarkMenu = App.MainWindow.IsDarkMenu();
	if (fDarkMenu)
		::SetWindowTheme(m_hwnd, L"DarkMode", nullptr);

	m_ChannelMenu.Create(
		&ChannelList, CurChannel, 1, ItemCount, nullptr, m_hwnd,
		CChannelMenu::CreateFlag::ShowLogo | CChannelMenu::CreateFlag::SpaceBreak,
		GetAppClass().MenuOptions.GetMaxChannelMenuRows());
	if (SelectedChannel >= 0)
		m_ChannelMenu.SetHighlightedItem(SelectedChannel);

	RECT rc;

	GetHeaderRect(&rc);
	MapWindowRect(m_hwnd, nullptr, &rc);

	const int Result = m_ChannelMenu.Show(
		TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_VERTICAL,
		rc.left, rc.bottom, &rc);
	m_ChannelMenu.Destroy();

	if (fDarkMenu)
		::SetWindowTheme(m_hwnd, nullptr, nullptr);

	if (Result > 0) {
		const CChannelInfo *pChannelInfo = ChannelList.GetChannelInfo(Result - 1);

		if (pChannelInfo != nullptr) {
			SelectChannel(pChannelInfo);
		}
	}
}


/*
void CProgramListPanel::SetToolTip()
{
	if (m_hwndToolTip != nullptr) {
		int NumTools = ::SendMessage(m_hwndToolTip, TTM_GETTOOLCOUNT, 0, 0);
		int NumItems = m_ItemList.NumItems();
		TOOLINFO ti;

		ti.cbSize = TTTOOLINFOA_V2_SIZE;
		ti.hwnd = m_hwnd;
		if (NumTools < NumItems) {
			ti.uFlags = TTF_SUBCLASS;
			ti.hinst = nullptr;
			ti.lpszText = LPSTR_TEXTCALLBACK;
			::SetRect(&ti.rect, 0, 0, 0, 0);
			for (int i = NumTools; i < NumItems; i++) {
				ti.uId = i;
				ti.lParam = i;
				::SendMessage(m_hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti);
			}
		} else if (NumTools > NumItems) {
			for (int i = NumItems; i < NumTools; i++) {
				ti.uId = i;
				::SendMessage(m_hwndToolTip, TTM_DELTOOL, 0, (LPARAM)&ti);
			}
		}
		GetClientRect(&ti.rect);
		ti.rect.top = -m_ScrollPos;
		ti.uId = 0;
		for (int i = 0; i < NumItems; i++) {
			const CProgramItemInfo *pItem = m_ItemList.GetItem(i);

			ti.rect.bottom = ti.rect.top + (pItem->GetTitleLines() + pItem->GetTextLines()) * (m_FontHeight+m_Style.LineSpacing) +
				(m_Style.TitlePadding.Top + m_Style.TitlePadding.Bottom - m_Style.LineSpacing);
			::SendMessage(m_hwndToolTip, TTM_NEWTOOLRECT, 0, (LPARAM)&ti);
			ti.uId++;
			ti.rect.top = ti.rect.bottom;
		}
	}
}
*/


LRESULT CProgramListPanel::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			m_ScrollPos = 0;
			m_OldDPI = 0;

			InitializeUI();

			if (IsDarkThemeSupported()) {
				SetWindowDarkTheme(hwnd, IsDarkThemeColor(m_Theme.MarginColor));
			}

			m_EpgIcons.Load();
			/*
			m_hwndToolTip = ::CreateWindowEx(
				WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr,
				WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, 0, 0, 0, 0,
				hwnd, nullptr, m_hinst, nullptr);
			::SendMessage(m_hwndToolTip, TTM_SETMAXTIPWIDTH, 0, 320);
			::SendMessage(m_hwndToolTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 30000);
			*/
			m_EventInfoPopupManager.Initialize(hwnd, &m_EventInfoPopupHandler);
			m_EventInfoPopupManager.SetEnable(m_fMouseOverEventInfo);

			CFeaturedEvents &FeaturedEvents = GetAppClass().FeaturedEvents;
			FeaturedEvents.AddEventHandler(this);
			if (m_fShowFeaturedMark)
				m_FeaturedEventsMatcher.BeginMatching(FeaturedEvents.GetSettings());

			m_HotItem = -1;
		}
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			BeginPaint(hwnd, &ps);
			Draw(ps.hdc, &ps.rcPaint);
			EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_SIZE:
		CalcDimensions();
		SetScrollBar();
		//SetToolTip();
		return 0;

	case WM_MOUSEWHEEL:
		{
			const int Delta = m_MouseWheel.OnMouseWheel(
				wParam, (m_FontHeight + m_Style.LineSpacing) * m_MouseWheel.GetDefaultScrollLines());

			if (Delta != 0)
				SetScrollPos(m_ScrollPos - Delta);
		}
		return 0;

	case WM_VSCROLL:
		{
			RECT rc;
			GetProgramListRect(&rc);
			const int Page = rc.bottom - rc.top;
			const int LineHeight = m_FontHeight + m_Style.LineSpacing;
			int Max =
				m_TotalLines * LineHeight +
				m_ItemList.NumItems() * (m_Style.TitlePadding.Top + m_Style.TitlePadding.Bottom - m_Style.LineSpacing) - Page;
			if (Max < 0)
				Max = 0;
			int Pos = m_ScrollPos;

			switch (LOWORD(wParam)) {
			case SB_LINEUP:        Pos -= LineHeight;    break;
			case SB_LINEDOWN:      Pos += LineHeight;    break;
			case SB_PAGEUP:        Pos -= Page;          break;
			case SB_PAGEDOWN:      Pos += Page;          break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:    Pos = HIWORD(wParam); break;
			case SB_TOP:           Pos = 0;              break;
			case SB_BOTTOM:        Pos = Max;            break;
			default:               return 0;
			}
			SetScrollPos(Pos);
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			const int HotItem = ItemHitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

			if (HotItem != m_HotItem) {
				SetHotItem(HotItem);

				if (m_HotItem >= 0) {
					TRACKMOUSEEVENT tme;

					tme.cbSize = sizeof(tme);
					tme.dwFlags = TME_LEAVE;
					tme.hwndTrack = hwnd;
					::TrackMouseEvent(&tme);
				}
			}
		}
		return 0;

	case WM_MOUSELEAVE:
		if (m_HotItem >= 0)
			SetHotItem(-1);
		return 0;

	case WM_LBUTTONDOWN:
		{
			::SetFocus(hwnd);

			const int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
			const int HotItem = ItemHitTest(x, y);
			if (HotItem == m_HotItem) {
				switch (HotItem) {
				case ITEM_CHANNEL:
					if (!IsStringEmpty(m_SelectedChannel.GetName())) {
						GetAppClass().Core.SelectChannel(
							nullptr, m_SelectedChannel, CAppCore::SelectChannelFlag::UseCurrentTuner);
					}
					break;

				case ITEM_CHANNELLISTBUTTON:
					ShowChannelListMenu();
					SetHotItem(-1);
					break;
				}
			} else {
				SetHotItem(HotItem);
			}

			if (HotItem < 0 && !m_fMouseOverEventInfo) {
				m_EventInfoPopupManager.Popup(x, y);
			}
		}
		return 0;

	case WM_RBUTTONUP:
		{
			::SetFocus(hwnd);

			CPopupMenu Menu(GetAppClass().GetResourceInstance(), IDM_PROGRAMLISTPANEL);

			Menu.CheckItem(CM_PROGRAMLISTPANEL_MOUSEOVEREVENTINFO, m_fMouseOverEventInfo);
			Menu.CheckItem(CM_PROGRAMLISTPANEL_USEEPGCOLORSCHEME, m_fUseEpgColorScheme);
			Menu.CheckItem(CM_PROGRAMLISTPANEL_SHOWFEATUREDMARK, m_fShowFeaturedMark);
			Menu.Show(hwnd);
		}
		return 0;

	case WM_SETCURSOR:
		if (reinterpret_cast<HWND>(wParam) == hwnd) {
			if (LOWORD(lParam) == HTCLIENT && m_HotItem >= 0)
				::SetCursor(GetActionCursor());
			else
				::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
			return TRUE;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case CM_PROGRAMLISTPANEL_MOUSEOVEREVENTINFO:
			SetMouseOverEventInfo(!m_fMouseOverEventInfo);
			return 0;

		case CM_PROGRAMLISTPANEL_USEEPGCOLORSCHEME:
			SetUseEpgColorScheme(!m_fUseEpgColorScheme);
			return 0;

		case CM_PROGRAMLISTPANEL_SHOWFEATUREDMARK:
			SetShowFeaturedMark(!m_fShowFeaturedMark);
			return 0;
		}
		return 0;

#if 0	// テキストが長過ぎてツールチップを使うと問題がある
	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case TTN_NEEDTEXT:
			{
				LPNMTTDISPINFO pnmtdi = reinterpret_cast<LPNMTTDISPINFO>(lParam);
				const CProgramItemInfo *pItem = m_ItemList.GetItem(static_cast<int>(pnmtdi->lParam));

				if (pItem != nullptr) {
					static TCHAR szText[1024];
					const LibISDB::EventInfo &EventInfo = pItem->GetEventInfo();
					TCHAR szEndTime[16];
					SYSTEMTIME stEnd;
					if (EventInfo.m_Duration > 0 && EventInfo.GetEndTime(&stEnd)) {
						StringFormat(
							szEndTime,
							TEXT("～{}:{:02}"), stEnd.wHour, stEnd.wMinute);
					} else {
						szEndTime[0] = '\0';
					}
					StringFormat(
						szText,
						TEXT("{}/{}({}) {}:{:02}{}\n{}\n\n{}{}{}{}"),
						EventInfo.m_StartTime.wMonth,
						EventInfo.m_StartTime.wDay,
						GetDayOfWeekText(EventInfo.m_StartTime.wDayOfWeek),
						EventInfo.m_StartTime.wHour,
						EventInfo.m_StartTime.wMinute,
						szEndTime,
						EventInfo.m_EventName,
						EventInfo.m_EventText,
						!EventInfo.m_EventText.empty() ? TEXT("\n\n") : TEXT(""),
						EventInfo.m_EventExtendedText,
						!EventInfo.m_EventExtendedText.empty() ? TEXT("\n\n") : TEXT(""));
					pnmtdi->lpszText = szText;
				} else {
					pnmtdi->lpszText = TEXT("");
				}
				pnmtdi->szText[0] = '\0';
				pnmtdi->hinst = nullptr;
			}
			return 0;

		case TTN_SHOW:
			{
				// ツールチップの位置がカーソルと重なっていると
				// 出たり消えたりを繰り返しておかしくなるのでずらす
				LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);
				RECT rcTip;
				POINT pt;

				::GetWindowRect(pnmh->hwndFrom, &rcTip);
				::GetCursorPos(&pt);
				if (::PtInRect(&rcTip, pt)) {
					const HMONITOR hMonitor = ::MonitorFromRect(&rcTip, MONITOR_DEFAULTTONEAREST);
					if (hMonitor != nullptr) {
						MONITORINFO mi;

						mi.cbSize = sizeof(mi);
						if (::GetMonitorInfo(hMonitor, &mi)) {
							if (rcTip.left <= mi.rcMonitor.left + 16)
								rcTip.left = pt.x + 16;
							else if (rcTip.right >= mi.rcMonitor.right - 16)
								rcTip.left = pt.x - (rcTip.right - rcTip.left) - 8;
							else
								break;
							::SetWindowPos(
								pnmh->hwndFrom, HWND_TOPMOST,
								rcTip.left, rcTip.top, 0, 0,
								SWP_NOSIZE | SWP_NOACTIVATE);
							return TRUE;
						}
					}
				}
			}
			break;
		}
		break;
#endif

	case WM_DESTROY:
		m_EpgIcons.Destroy();
		//m_hwndToolTip = nullptr;
		return 0;

	default:
		{
			LRESULT Result;

			if (m_ChannelMenu.HandleMessage(hwnd, uMsg, wParam, lParam, &Result))
				return Result;
		}
		break;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


void CProgramListPanel::ApplyStyle()
{
	if (m_hwnd != nullptr) {
		CreateDrawFontAndBoldFont(m_StyleFont, &m_Font, &m_TitleFont);

		LOGFONT lf = {};
		lf.lfHeight = -m_Style.ChannelButtonIconSize.Height;
		lf.lfCharSet = SYMBOL_CHARSET;
		StringCopy(lf.lfFaceName, TEXT("Marlett"));
		m_IconFont.Create(&lf);

		CalcFontHeight();
		CalcChannelHeight();
	}
}


void CProgramListPanel::RealizeStyle()
{
	if (m_hwnd != nullptr) {
		if (m_pStyleScaling != nullptr) {
			const int NewDPI = m_pStyleScaling->GetDPI();
			if (m_OldDPI != 0)
				m_ScrollPos = ::MulDiv(m_ScrollPos, NewDPI, m_OldDPI);
			m_OldDPI = NewDPI;
		}

		SendSizeMessage();
		Invalidate();
	}
}


void CProgramListPanel::Draw(HDC hdc, const RECT *prcPaint)
{
	RECT rc, rcMargin;
	GetClientRect(&rc);

	Theme::CThemeDraw ThemeDraw(BeginThemeDraw(hdc));

	CTextDraw DrawText;
	DrawText.Begin(hdc, rc, CTextDraw::Flag::JapaneseHyphnation);

	const int LineHeight = m_FontHeight + m_Style.LineSpacing;

	const HFONT hfontOld = static_cast<HFONT>(::GetCurrentObject(hdc, OBJ_FONT));
	const COLORREF crOldTextColor = ::GetTextColor(hdc);
	const int OldBkMode = ::SetBkMode(hdc, TRANSPARENT);

	const bool fCurChannel =
		m_CurChannel.GetServiceID() > 0
		&& m_SelectedChannel.GetNetworkID() == m_CurChannel.GetNetworkID()
		&& m_SelectedChannel.GetTransportStreamID() == m_CurChannel.GetTransportStreamID()
		&& m_SelectedChannel.GetServiceID() == m_CurChannel.GetServiceID();

	GetHeaderRect(&rc);
	if (IsRectIntersect(&rc, prcPaint)) {
		const Theme::Style &ChannelStyle =
			fCurChannel ? m_Theme.CurChannelNameStyle : m_Theme.ChannelNameStyle;

		ThemeDraw.Draw(ChannelStyle.Back, rc);

		if (!IsStringEmpty(m_SelectedChannel.GetName())) {
			Style::Subtract(&rc, m_Style.ChannelPadding);

			const HBITMAP hbmLogo = GetAppClass().LogoManager.GetAssociatedLogoBitmap(
				m_SelectedChannel.GetNetworkID(), m_SelectedChannel.GetServiceID(),
				CLogoManager::LOGOTYPE_SMALL);
			if (hbmLogo != nullptr) {
				const int LogoHeight = (rc.bottom - rc.top) - m_Style.ChannelLogoMargin.Vert();
				const int LogoWidth = LogoHeight * 16 / 9;
				rc.left += m_Style.ChannelLogoMargin.Left;
				DrawUtil::DrawBitmap(
					hdc,
					rc.left, rc.top + m_Style.ChannelLogoMargin.Top,
					LogoWidth, LogoHeight,
					hbmLogo);
				rc.left += LogoWidth + m_Style.ChannelLogoMargin.Right;
			}

			rc.right -=
				m_Style.ChannelButtonMargin +
				m_Style.ChannelButtonIconSize.Width +
				m_Style.ChannelButtonPadding.Horz();
			Style::Subtract(&rc, m_Style.ChannelNameMargin);
			DrawUtil::SelectObject(hdc, m_TitleFont);
			ThemeDraw.Draw(
				ChannelStyle.Fore, rc, m_SelectedChannel.GetName(),
				DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
		}

		GetChannelButtonRect(&rc);
		const Theme::Style &ButtonStyle =
			m_HotItem == ITEM_CHANNELLISTBUTTON ?
			m_Theme.ChannelButtonHotStyle : m_Theme.ChannelButtonStyle;
		if (ButtonStyle.Back.Border.Type != Theme::BorderType::None
				|| ButtonStyle.Back.Fill != ChannelStyle.Back.Fill)
			ThemeDraw.Draw(ButtonStyle.Back, rc);
		Style::Subtract(&rc, m_Style.ChannelButtonPadding);
		DrawUtil::SelectObject(hdc, m_IconFont);
		ThemeDraw.Draw(ButtonStyle.Fore, rc, TEXT("6"), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}

	const HBRUSH hbr = ::CreateSolidBrush(m_Theme.MarginColor);

	GetProgramListRect(&rc);

	if (m_fShowRetrievingMessage && m_ItemList.NumItems() == 0) {
		::FillRect(hdc, &rc, hbr);
		DrawUtil::SelectObject(hdc, m_Font);
		::SetTextColor(hdc, m_Theme.EventTextStyle.Fore.Fill.GetSolidColor());
		Style::Subtract(&rc, m_Style.TitlePadding);
		DrawText.Draw(TEXT("番組表の取得中です..."), rc, LineHeight);
	} else {
		const HRGN hrgn = ::CreateRectRgnIndirect(&rc);
		::SelectClipRgn(hdc, hrgn);

		m_EpgIcons.BeginDraw(hdc, m_Style.IconSize.Width, m_Style.IconSize.Height);

		rc.top -= m_ScrollPos;
		for (int i = 0; i < m_ItemList.NumItems(); i++) {
			CProgramItemInfo *pItem = m_ItemList.GetItem(i);
			const bool fCur = fCurChannel && pItem->GetEventID() == m_CurEventID;
			const int EventTextHeight = pItem->GetTextLines() * LineHeight;

			rc.bottom =
				rc.top + pItem->GetTitleLines() * LineHeight +
				(m_Style.TitlePadding.Top + m_Style.TitlePadding.Bottom - m_Style.LineSpacing);
			if (m_fUseEpgColorScheme) {
				RECT rcContent;
				rcContent.left = 0;
				rcContent.top = rc.top;
				rcContent.right = rc.right;
				rcContent.bottom = rc.bottom + EventTextHeight;
				if (rcContent.bottom > prcPaint->top) {
					CEpgTheme::DrawContentBackgroundFlag Flags = CEpgTheme::DrawContentBackgroundFlag::Separator;
					if (fCur)
						Flags |= CEpgTheme::DrawContentBackgroundFlag::Current;
					m_EpgTheme.DrawContentBackground(hdc, ThemeDraw, rcContent, pItem->GetEventInfo(), Flags);
				}
			}
			if (rc.bottom > prcPaint->top) {
				rc.left = 0;
				if (m_fUseEpgColorScheme) {
					::SetTextColor(hdc, m_EpgTheme.GetColor(CEpgTheme::COLOR_EVENTNAME));
				} else {
					const Theme::Style &Style =
						fCur ? m_Theme.CurEventNameStyle : m_Theme.EventNameStyle;
					::SetTextColor(hdc, Style.Fore.Fill.GetSolidColor());
					ThemeDraw.Draw(Style.Back, rc);
				}

				RECT rcTitle = rc;
				Style::Subtract(&rcTitle, m_Style.TitlePadding);
				DrawUtil::SelectObject(hdc, m_TitleFont);

				if (m_fShowFeaturedMark
						&& m_FeaturedEventsMatcher.IsMatch(pItem->GetEventInfo())) {
					RECT rcMark;
					const SIZE sz = pItem->GetTimeSize(hdc);
					if (m_fUseEpgColorScheme) {
						rcMark.left = rcTitle.left;
						rcMark.top = rcTitle.top;
						rcMark.right = rcMark.left + sz.cx;
						rcMark.bottom = rcMark.top + sz.cy;
						Style::Subtract(&rcMark, m_Style.FeaturedMarkMargin);
					} else {
						rcMark.left = rc.left + 1;
						rcMark.top = rc.top + 1;
						rcMark.right = rcMark.left + m_Style.FeaturedMarkSize.Width;
						rcMark.bottom = rcMark.top + m_Style.FeaturedMarkSize.Height;
					}
					ThemeDraw.Draw(m_Theme.FeaturedMarkStyle, rcMark);
				}

				pItem->DrawTitle(DrawText, rcTitle, LineHeight, m_fUseARIBSymbol);
			}

			rc.top = rc.bottom;
			rc.bottom = rc.top + EventTextHeight;
			if (rc.bottom > prcPaint->top) {
				rc.left = 0;
				if (m_fUseEpgColorScheme) {
					::SetTextColor(hdc, m_EpgTheme.GetColor(CEpgTheme::COLOR_EVENTTEXT));
				} else {
					const Theme::Style &Style =
						fCur ? m_Theme.CurEventTextStyle : m_Theme.EventTextStyle;
					::SetTextColor(hdc, Style.Fore.Fill.GetSolidColor());
					ThemeDraw.Draw(Style.Back, rc);
				}
				DrawUtil::SelectObject(hdc, m_Font);
				rc.left = GetTextLeftMargin();
				pItem->DrawText(DrawText, rc, LineHeight, m_fUseARIBSymbol);

				const unsigned int ShowIcons =
					CEpgIcons::GetEventIcons(&pItem->GetEventInfo()) & m_VisibleEventIcons;
				if (ShowIcons != 0) {
					rc.left = 0;
					m_EpgIcons.DrawIcons(
						ShowIcons, hdc,
						m_Style.IconMargin.Left, rc.top + m_Style.IconMargin.Top,
						m_Style.IconSize.Width, m_Style.IconSize.Height,
						0, m_Style.IconSize.Height + m_Style.IconMargin.Bottom,
						m_fUseEpgColorScheme ? 255 : 192, &rc);
				}
			}

			rc.top = rc.bottom;
			if (rc.top >= prcPaint->bottom)
				break;
		}

		if (rc.top < prcPaint->bottom) {
			rcMargin.left = prcPaint->left;
			rcMargin.top = std::max(rc.top, prcPaint->top);
			rcMargin.right = prcPaint->right;
			rcMargin.bottom = prcPaint->bottom;
			::FillRect(hdc, &rcMargin, hbr);
		}

		m_EpgIcons.EndDraw();

		::SelectClipRgn(hdc, nullptr);
		::DeleteObject(hrgn);
	}

	::SetTextColor(hdc, crOldTextColor);
	::SetBkMode(hdc, OldBkMode);
	::SelectObject(hdc, hfontOld);
	::DeleteObject(hbr);
	DrawText.End();
}


void CProgramListPanel::OnFeaturedEventsSettingsChanged(CFeaturedEvents &FeaturedEvents)
{
	if (m_fShowFeaturedMark) {
		m_FeaturedEventsMatcher.BeginMatching(FeaturedEvents.GetSettings());
		Invalidate();
	}
}


CProgramListPanel::CEventInfoPopupHandler::CEventInfoPopupHandler(CProgramListPanel *pPanel)
	: m_pPanel(pPanel)
{
}


bool CProgramListPanel::CEventInfoPopupHandler::HitTest(int x, int y, LPARAM *pParam)
{
	const int Program = m_pPanel->ProgramHitTest(x, y);

	if (Program >= 0) {
		*pParam = Program;
		return true;
	}
	return false;
}


bool CProgramListPanel::CEventInfoPopupHandler::ShowPopup(LPARAM Param, CEventInfoPopup *pPopup)
{
	const int ItemIndex = static_cast<int>(Param);
	const CProgramItemInfo *pItem = m_pPanel->m_ItemList.GetItem(ItemIndex);
	if (pItem == nullptr)
		return false;

	int IconWidth, IconHeight;
	pPopup->GetPreferredIconSize(&IconWidth, &IconHeight);
	const HICON hIcon = GetAppClass().LogoManager.CreateLogoIcon(
		m_pPanel->m_SelectedChannel.GetNetworkID(),
		m_pPanel->m_SelectedChannel.GetServiceID(),
		IconWidth, IconHeight);

	RECT rc;
	m_pPanel->GetItemRect(ItemIndex, &rc);
	POINT pt = {rc.left, rc.bottom};
	::ClientToScreen(m_pPanel->m_hwnd, &pt);
	pPopup->GetDefaultPopupPosition(&rc);
	if (rc.top > pt.y) {
		rc.bottom = pt.y + (rc.bottom - rc.top);
		rc.top = pt.y;
	}

	if (!pPopup->Show(
				&pItem->GetEventInfo(), &rc,
				hIcon, m_pPanel->m_SelectedChannel.GetName())) {
		if (hIcon != nullptr)
			::DestroyIcon(hIcon);
		return false;
	}

	return true;
}




void CProgramListPanel::ProgramListPanelStyle::SetStyle(const Style::CStyleManager *pStyleManager)
{
	*this = ProgramListPanelStyle();
	pStyleManager->Get(TEXT("program-list-panel.channel.padding"), &ChannelPadding);
	pStyleManager->Get(TEXT("program-list-panel.channel.logo.margin"), &ChannelLogoMargin);
	pStyleManager->Get(TEXT("program-list-panel.channel.channel-name.margin"), &ChannelNameMargin);
	pStyleManager->Get(TEXT("program-list-panel.channel.button.icon"), &ChannelButtonIconSize);
	pStyleManager->Get(TEXT("program-list-panel.channel.button.padding"), &ChannelButtonPadding);
	pStyleManager->Get(TEXT("program-list-panel.channel.button.margin"), &ChannelButtonMargin);
	pStyleManager->Get(TEXT("program-list-panel.title.padding"), &TitlePadding);
	pStyleManager->Get(TEXT("program-list-panel.icon"), &IconSize);
	pStyleManager->Get(TEXT("program-list-panel.icon.margin"), &IconMargin);
	pStyleManager->Get(TEXT("program-list-panel.line-spacing"), &LineSpacing);
	pStyleManager->Get(TEXT("program-list-panel.featured-mark"), &FeaturedMarkSize);
	pStyleManager->Get(TEXT("program-guide.event.featured-mark.margin"), &FeaturedMarkMargin);
}


void CProgramListPanel::ProgramListPanelStyle::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	pStyleScaling->ToPixels(&ChannelPadding);
	pStyleScaling->ToPixels(&ChannelLogoMargin);
	pStyleScaling->ToPixels(&ChannelNameMargin);
	pStyleScaling->ToPixels(&ChannelButtonIconSize);
	pStyleScaling->ToPixels(&ChannelButtonPadding);
	pStyleScaling->ToPixels(&ChannelButtonMargin);
	pStyleScaling->ToPixels(&TitlePadding);
	pStyleScaling->ToPixels(&IconSize);
	pStyleScaling->ToPixels(&IconMargin);
	pStyleScaling->ToPixels(&LineSpacing);
	pStyleScaling->ToPixels(&FeaturedMarkSize);
	pStyleScaling->ToPixels(&FeaturedMarkMargin);
}


}	// namespace TVTest
