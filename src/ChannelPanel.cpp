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
#include "ChannelPanel.h"
#include "DrawUtil.h"
#include "DarkMode.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


const LPCTSTR CChannelPanel::m_pszClassName = APP_NAME TEXT(" Channel Panel");
HINSTANCE CChannelPanel::m_hinst = nullptr;


bool CChannelPanel::Initialize(HINSTANCE hinst)
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


CChannelPanel::CChannelPanel()
	: m_EventInfoPopupManager(&m_EventInfoPopup)
{
	GetDefaultFont(&m_StyleFont);
}


CChannelPanel::~CChannelPanel()
{
	Destroy();
	ClearChannels();
}


bool CChannelPanel::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	return CreateBasicWindow(
		hwndParent, Style, ExStyle, ID,
		m_pszClassName, TEXT("チャンネル"), m_hinst);
}


void CChannelPanel::SetStyle(const Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);
}


void CChannelPanel::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	m_Style.NormalizeStyle(pStyleManager, pStyleScaling);

	if (m_OldDPI == 0)
		m_OldDPI = pStyleScaling->GetDPI();
}


void CChannelPanel::SetTheme(const Theme::CThemeManager *pThemeManager)
{
	ChannelPanelTheme Theme;

	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_CHANNELPANEL_CHANNELNAME,
		&Theme.ChannelNameStyle);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_CHANNELPANEL_CURCHANNELNAME,
		&Theme.CurChannelNameStyle);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_CHANNELPANEL_EVENTNAME1,
		&Theme.EventStyle[0]);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_CHANNELPANEL_EVENTNAME2,
		&Theme.EventStyle[1]);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_CHANNELPANEL_CURCHANNELEVENTNAME1,
		&Theme.CurChannelEventStyle[0]);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_CHANNELPANEL_CURCHANNELEVENTNAME2,
		&Theme.CurChannelEventStyle[1]);
	Theme.MarginColor = pThemeManager->GetColor(CColorScheme::COLOR_PANELBACK);
	pThemeManager->GetBackgroundStyle(
		Theme::CThemeManager::STYLE_CHANNELPANEL_FEATUREDMARK,
		&Theme.FeaturedMarkStyle);
	pThemeManager->GetBackgroundStyle(
		Theme::CThemeManager::STYLE_CHANNELPANEL_PROGRESS,
		&Theme.ProgressStyle);
	pThemeManager->GetBackgroundStyle(
		Theme::CThemeManager::STYLE_CHANNELPANEL_CURPROGRESS,
		&Theme.CurProgressStyle);

	SetChannelPanelTheme(Theme);

	m_EpgTheme.SetTheme(pThemeManager);
	m_EventInfoPopup.SetTheme(pThemeManager);
}


bool CChannelPanel::SetFont(const Style::Font &Font)
{
	m_StyleFont = Font;

	if (m_hwnd != nullptr) {
		ApplyStyle();
		RealizeStyle();
	}

	return true;
}


bool CChannelPanel::ReadSettings(CSettings &Settings)
{
	bool f;
	if (Settings.Read(TEXT("ChannelPanelDetailToolTip"), &f))
		SetDetailToolTip(f);

	int EventsPerChannel = m_EventsPerChannel;
	int AdditionalEvents = m_ExpandAdditionalEvents;
	if (Settings.Read(TEXT("ChannelPanelEventsPerChannel"), &EventsPerChannel)
			|| Settings.Read(TEXT("ChannelPanelExpandEvents"), &AdditionalEvents))
		SetEventsPerChannel(EventsPerChannel, AdditionalEvents);

	Settings.Read(TEXT("ChannelPanelScrollToCurChannel"), &m_fScrollToCurChannel);
	Settings.Read(TEXT("ChannelPanelUseEpgColorScheme"), &m_fUseEpgColorScheme);
	Settings.Read(TEXT("ChannelPanelShowGenreColor"), &m_fShowGenreColor);
	Settings.Read(TEXT("ChannelPanelShowFeaturedMark"), &m_fShowFeaturedMark);
	Settings.Read(TEXT("ChannelPanelShowProgressBar"), &m_fShowProgressBar);
	int Style;
	if (Settings.Read(TEXT("ChannelPanelProgressBarStyle"), &Style))
		m_ProgressBarStyle = static_cast<ProgressBarStyle>(Style);

	int PopupWidth, PopupHeight;
	if (Settings.Read(TEXT("ChannelPanelPopupEventInfoWidth"), &PopupWidth)
			&& Settings.Read(TEXT("ChannelPanelPopupEventInfoHeight"), &PopupHeight))
		m_EventInfoPopup.SetSize(PopupWidth, PopupHeight);

	return true;
}


bool CChannelPanel::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("ChannelPanelDetailToolTip"), m_fDetailToolTip);
	Settings.Write(TEXT("ChannelPanelEventsPerChannel"), m_EventsPerChannel);
	Settings.Write(TEXT("ChannelPanelExpandEvents"), m_ExpandAdditionalEvents);
	Settings.Write(TEXT("ChannelPanelScrollToCurChannel"), m_fScrollToCurChannel);
	Settings.Write(TEXT("ChannelPanelUseEpgColorScheme"), m_fUseEpgColorScheme);
	Settings.Write(TEXT("ChannelPanelShowGenreColor"), m_fShowGenreColor);
	Settings.Write(TEXT("ChannelPanelShowFeaturedMark"), m_fShowFeaturedMark);
	Settings.Write(TEXT("ChannelPanelShowProgressBar"), m_fShowProgressBar);
	Settings.Write(TEXT("ChannelPanelProgressBarStyle"), static_cast<int>(m_ProgressBarStyle));

	int PopupWidth, PopupHeight;
	m_EventInfoPopup.GetSize(&PopupWidth, &PopupHeight);
	Settings.Write(TEXT("ChannelPanelPopupEventInfoWidth"), PopupWidth);
	Settings.Write(TEXT("ChannelPanelPopupEventInfoHeight"), PopupHeight);

	return true;
}


bool CChannelPanel::SetEPGDatabase(LibISDB::EPGDatabase *pEPGDatabase)
{
	m_pEPGDatabase = pEPGDatabase;
	return true;
}


void CChannelPanel::ClearChannels()
{
	m_ChannelList.clear();
}


bool CChannelPanel::UpdateEvents(CChannelEventInfo *pInfo, const LibISDB::DateTime *pTime)
{
	const WORD NetworkID = pInfo->GetNetworkID();
	const WORD TransportStreamID = pInfo->GetTransportStreamID();
	const WORD ServiceID = pInfo->GetServiceID();
	LibISDB::DateTime Time;
	LibISDB::EventInfo EventInfo;
	bool fChanged = false;

	if (pTime != nullptr)
		Time = *pTime;
	else
		LibISDB::GetCurrentEPGTime(&Time);
	const int NumEvents = pInfo->IsExpanded() ? m_ExpandEvents : m_EventsPerChannel;
	for (int i = 0; i < NumEvents; i++) {
		if (m_pEPGDatabase->GetEventInfo(NetworkID, TransportStreamID, ServiceID, Time, &EventInfo)) {
			if (pInfo->SetEventInfo(i, &EventInfo))
				fChanged = true;
		} else {
			if (i == 0) {
				if (pInfo->SetEventInfo(0, nullptr))
					fChanged = true;
				if (NumEvents == 1)
					break;
				i++;
			}
			if (m_pEPGDatabase->GetNextEventInfo(NetworkID, TransportStreamID, ServiceID, Time, &EventInfo)
					&& EventInfo.StartTime.DiffSeconds(Time) < 8 * 60 * 60) {
				if (pInfo->SetEventInfo(i, &EventInfo))
					fChanged = true;
			} else {
				for (; i < m_EventsPerChannel; i++) {
					if (pInfo->SetEventInfo(i, nullptr))
						fChanged = true;
				}
				break;
			}
		}
		EventInfo.GetEndTime(&Time);
	}
	return fChanged;
}


bool CChannelPanel::SetChannelList(const CChannelList *pChannelList, bool fSetEvent)
{
	ClearChannels();

	m_ScrollPos = 0;
	m_CurChannel = -1;

	if (pChannelList != nullptr) {
		LibISDB::DateTime Current;

		LibISDB::GetCurrentEPGTime(&Current);
		for (int i = 0; i < pChannelList->NumChannels(); i++) {
			const CChannelInfo *pChInfo = pChannelList->GetChannelInfo(i);

			if (!pChInfo->IsEnabled())
				continue;

			CChannelEventInfo *pEventInfo = new CChannelEventInfo(pChInfo, i);

			if (fSetEvent && m_pEPGDatabase != nullptr)
				UpdateEvents(pEventInfo, &Current);
			if (m_pLogoManager != nullptr) {
				const HBITMAP hbmLogo = m_pLogoManager->GetAssociatedLogoBitmap(
					pEventInfo->GetNetworkID(), pEventInfo->GetServiceID(),
					CLogoManager::LOGOTYPE_SMALL);
				if (hbmLogo != nullptr)
					pEventInfo->SetLogo(hbmLogo);
			}
			m_ChannelList.emplace_back(pEventInfo);
		}
	}

	if (m_hwnd != nullptr) {
		SetScrollBar();
		SetTooltips();
		Invalidate();
		Update();
	}

	return true;
}


bool CChannelPanel::UpdateAllChannels()
{
	if (m_pEPGDatabase != nullptr && !m_ChannelList.empty()) {
		bool fChanged = false;

		LibISDB::GetCurrentEPGTime(&m_UpdatedTime);
		for (auto &e : m_ChannelList) {
			if (UpdateEvents(e.get(), &m_UpdatedTime))
				fChanged = true;
		}
		if (m_hwnd != nullptr && fChanged) {
			Redraw();
		}
	}
	return true;
}


bool CChannelPanel::UpdateChannel(int ChannelIndex)
{
	if (m_pEPGDatabase != nullptr) {
		for (int i = 0; i < static_cast<int>(m_ChannelList.size()); i++) {
			CChannelEventInfo *pEventInfo = m_ChannelList[i].get();

			if (pEventInfo->GetOriginalChannelIndex() == ChannelIndex) {
				if (UpdateEvents(pEventInfo) && m_hwnd != nullptr) {
					RECT rc;

					GetItemRect(i, &rc);
					Invalidate(&rc);
					Update();
				}
				return true;
			}
		}
	}
	return false;
}


bool CChannelPanel::UpdateChannels(WORD NetworkID, WORD TransportStreamID)
{
	if (NetworkID == 0 && TransportStreamID == 0)
		return false;
	if (m_pEPGDatabase == nullptr)
		return false;

	LibISDB::DateTime Time;
	bool fChanged = false;

	LibISDB::GetCurrentEPGTime(&Time);
	for (auto &e : m_ChannelList) {
		if ((NetworkID == 0 || e->GetNetworkID() == NetworkID)
				&& (TransportStreamID == 0 || e->GetTransportStreamID() == TransportStreamID)) {
			if (UpdateEvents(e.get(), &Time))
				fChanged = true;
		}
	}
	if (m_hwnd != nullptr && fChanged) {
		Redraw();
	}
	return true;
}


bool CChannelPanel::IsChannelListEmpty() const
{
	return m_ChannelList.empty();
}


bool CChannelPanel::SetCurrentChannel(int CurChannel)
{
	if (CurChannel < -1)
		return false;

	m_CurChannel = CurChannel;

	if (m_hwnd != nullptr) {
		if (m_fScrollToCurChannel && m_CurChannel >= 0)
			ScrollToCurrentChannel();
		Invalidate();
	}

	return true;
}


bool CChannelPanel::ScrollToChannel(int Channel)
{
	if (Channel < 0 || static_cast<size_t>(Channel) >= m_ChannelList.size())
		return false;

	RECT rcClient, rcItem;
	GetClientRect(&rcClient);
	GetItemRect(Channel, &rcItem);
	if (rcItem.top >= rcClient.top && rcItem.bottom <= rcClient.bottom)
		return true;
	const bool fBottom = rcItem.bottom > rcClient.bottom;
	if (m_ScrollPos != 0)
		::OffsetRect(&rcItem, 0, m_ScrollPos);
	int Pos = rcItem.top;
	if (fBottom) {
		Pos -= rcClient.bottom - (rcItem.bottom - rcItem.top);
		if (Pos > rcItem.top)
			Pos = rcItem.top;
	}
	SetScrollPos(Pos);

	return true;
}


bool CChannelPanel::ScrollToCurrentChannel()
{
	for (int i = 0; i < static_cast<int>(m_ChannelList.size()); i++) {
		if (m_ChannelList[i]->GetOriginalChannelIndex() == m_CurChannel) {
			return ScrollToChannel(i);
		}
	}

	return false;
}


void CChannelPanel::SetEventHandler(CEventHandler *pEventHandler)
{
	m_pEventHandler = pEventHandler;
}


bool CChannelPanel::SetChannelPanelTheme(const ChannelPanelTheme &Theme)
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


bool CChannelPanel::GetChannelPanelTheme(ChannelPanelTheme *pTheme) const
{
	if (pTheme == nullptr)
		return false;
	*pTheme = m_Theme;
	return true;
}


bool CChannelPanel::SetEventInfoFont(const Style::Font &Font)
{
	return m_EventInfoPopup.SetFont(Font);
}


void CChannelPanel::SetDetailToolTip(bool fDetail)
{
	if (m_fDetailToolTip != fDetail) {
		m_fDetailToolTip = fDetail;
		if (m_hwnd != nullptr) {
			if (fDetail) {
				m_Tooltip.Destroy();
				m_EventInfoPopupManager.Initialize(m_hwnd, &m_EventInfoPopupHandler);
			} else {
				m_EventInfoPopupManager.Finalize();
				CreateTooltip();
			}
		}
	}
}


bool CChannelPanel::SetEventsPerChannel(int Events, int Expand)
{
	if (Events > 0 && Events > MAX_EVENTS_PER_CHANNEL)
		return false;
	if (Expand > 0 && Expand > MAX_EXPAND_EVENTS)
		return false;
	if ((Events > 0 && m_EventsPerChannel != Events)
			|| (Expand > 0 && m_ExpandAdditionalEvents != Expand)) {
		if (Events > 0)
			m_EventsPerChannel = Events;
		if (Expand > 0)
			m_ExpandAdditionalEvents = Expand;
		m_ExpandEvents = m_EventsPerChannel + m_ExpandAdditionalEvents;
		for (auto &e : m_ChannelList) {
			e->SetMaxEvents(e->IsExpanded() ? m_ExpandEvents : m_EventsPerChannel);
		}
		if (m_hwnd != nullptr)
			CalcItemHeight();
		UpdateAllChannels();
		if (m_hwnd != nullptr) {
			m_ScrollPos = 0;
			SetScrollBar();
			Invalidate();
			SetTooltips();
		}
	}
	return true;
}


bool CChannelPanel::ExpandChannel(int Channel, bool fExpand)
{
	if (Channel < 0 || static_cast<size_t>(Channel) >= m_ChannelList.size())
		return false;
	CChannelEventInfo *pInfo = m_ChannelList[Channel].get();
	if (pInfo->IsExpanded() != fExpand) {
		pInfo->Expand(fExpand);
		pInfo->SetMaxEvents(fExpand ? m_ExpandEvents : m_EventsPerChannel);
		UpdateEvents(pInfo);
		if (m_hwnd != nullptr) {
			RECT rcClient, rc;

			GetClientRect(&rcClient);
			GetItemRect(Channel, &rc);
			if (fExpand) {
				rc.bottom = rcClient.bottom;
				Invalidate(&rc);
			} else {
				const int Height = CalcHeight();

				if (m_ScrollPos > Height - rcClient.bottom) {
					m_ScrollPos = std::max(Height - rcClient.bottom, 0L);
					Invalidate();
				} else {
					rc.bottom = rcClient.bottom;
					Invalidate(&rc);
				}
			}
			SetScrollBar();
			SetTooltips();
		}
	}
	return true;
}


void CChannelPanel::SetScrollToCurChannel(bool fScroll)
{
	m_fScrollToCurChannel = fScroll;
}


void CChannelPanel::SetUseEpgColorScheme(bool fUseEpgColorScheme)
{
	if (m_fUseEpgColorScheme != fUseEpgColorScheme) {
		m_fUseEpgColorScheme = fUseEpgColorScheme;
		if (m_hwnd != nullptr)
			Invalidate();
	}
}


void CChannelPanel::SetShowGenreColor(bool fShowGenreColor)
{
	if (m_fShowGenreColor != fShowGenreColor) {
		m_fShowGenreColor = fShowGenreColor;
		if (!m_fUseEpgColorScheme && m_hwnd != nullptr)
			Invalidate();
	}
}


void CChannelPanel::SetShowFeaturedMark(bool fShowFeaturedMark)
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


void CChannelPanel::SetShowProgressBar(bool fShowProgressBar)
{
	if (m_fShowProgressBar != fShowProgressBar) {
		m_fShowProgressBar = fShowProgressBar;
		if (m_hwnd != nullptr) {
			if (m_fShowProgressBar)
				LibISDB::GetCurrentEPGTime(&m_CurTime);
			Invalidate();
		}
	}
}


void CChannelPanel::SetProgressBarStyle(ProgressBarStyle Style)
{
	if (m_ProgressBarStyle != Style) {
		m_ProgressBarStyle = Style;
		if (m_fShowProgressBar && m_hwnd != nullptr)
			Invalidate();
	}
}


void CChannelPanel::SetUseARIBSymbol(bool fUseARIBSymbol)
{
	if (m_fUseARIBSymbol != fUseARIBSymbol) {
		m_fUseARIBSymbol = fUseARIBSymbol;
		if (m_hwnd != nullptr)
			Invalidate();
	}
}


bool CChannelPanel::QueryUpdateProgress()
{
	if (m_fShowProgressBar && m_hwnd != nullptr) {
		LibISDB::DateTime Time;
		LibISDB::GetCurrentEPGTime(&Time);
		return m_CurTime.Second / 10 != Time.Second / 10
			|| m_CurTime.Minute != Time.Minute
			|| m_CurTime.Hour != Time.Hour
			|| m_CurTime.Day != Time.Day
			|| m_CurTime.Month != Time.Month
			|| m_CurTime.Year != Time.Year;
	}
	return false;
}


void CChannelPanel::UpdateProgress()
{
	if (m_fShowProgressBar && m_hwnd != nullptr) {
		LibISDB::GetCurrentEPGTime(&m_CurTime);
		Invalidate();
	}
}


void CChannelPanel::SetLogoManager(CLogoManager *pLogoManager)
{
	m_pLogoManager = pLogoManager;
}


bool CChannelPanel::QueryUpdate() const
{
	LibISDB::DateTime Time;

	LibISDB::GetCurrentEPGTime(&Time);
	return m_UpdatedTime.Minute != Time.Minute
		|| m_UpdatedTime.Hour != Time.Hour
		|| m_UpdatedTime.Day != Time.Day
		|| m_UpdatedTime.Month != Time.Month
		|| m_UpdatedTime.Year != Time.Year;
}


LRESULT CChannelPanel::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

			if (m_fDetailToolTip)
				m_EventInfoPopupManager.Initialize(hwnd, &m_EventInfoPopupHandler);
			else
				CreateTooltip();

			if (m_fShowProgressBar)
				LibISDB::GetCurrentEPGTime(&m_CurTime);

			CFeaturedEvents &FeaturedEvents = GetAppClass().FeaturedEvents;
			FeaturedEvents.AddEventHandler(this);
			if (m_fShowFeaturedMark)
				m_FeaturedEventsMatcher.BeginMatching(FeaturedEvents.GetSettings());
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
		{
			const int Height = HIWORD(lParam);
			const int TotalHeight = CalcHeight();
			const int Max = std::max(TotalHeight - Height, 0);

			if (m_ScrollPos > Max) {
				m_ScrollPos = Max;
				Invalidate();
				SetTooltips(true);
			}
			SetScrollBar();
		}
		return 0;

	case WM_MOUSEWHEEL:
		{
			const int Delta = m_MouseWheel.OnMouseWheel(wParam, m_FontHeight * m_MouseWheel.GetDefaultScrollLines());

			if (Delta != 0)
				SetScrollPos(m_ScrollPos - Delta);
		}
		return 0;

	case WM_VSCROLL:
		{
			const int Height = CalcHeight();
			RECT rc;
			GetClientRect(&rc);
			const int Page = rc.bottom;
			int Pos = m_ScrollPos;

			switch (LOWORD(wParam)) {
			case SB_LINEUP:        Pos -= m_FontHeight;              break;
			case SB_LINEDOWN:      Pos += m_FontHeight;              break;
			case SB_PAGEUP:        Pos -= Page;                      break;
			case SB_PAGEDOWN:      Pos += Page;                      break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:    Pos = HIWORD(wParam);             break;
			case SB_TOP:           Pos = 0;                          break;
			case SB_BOTTOM:        Pos = std::max(Height - Page, 0); break;
			default:               return 0;
			}
			SetScrollPos(Pos);
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			SetFocus(hwnd);
			HitType Type;
			const int Channel = HitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), &Type);
			if (Channel >= 0) {
				if (Type == HIT_CHEVRON)
					ExpandChannel(Channel, !m_ChannelList[Channel]->IsExpanded());
				else if (Type != HIT_MARGIN
						&& m_pEventHandler != nullptr)
					m_pEventHandler->OnChannelClick(&m_ChannelList[Channel]->GetChannelInfo());
			}
		}
		return 0;

	case WM_RBUTTONUP:
		{
			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			::ClientToScreen(hwnd, &pt);
			ShowMenu(pt.x, pt.y);
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			HitType Type;
			const int Channel = HitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), &Type);
			if (Channel >= 0 && Type != HIT_MARGIN)
				::SetCursor(GetActionCursor());
			else
				::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
		}
		return 0;

	case WM_COMMAND:
		OnCommand(LOWORD(wParam));
		return 0;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case TTN_NEEDTEXT:
			{
				LPNMTTDISPINFO pnmtdi = reinterpret_cast<LPNMTTDISPINFO>(lParam);
				const int Channel = LOWORD(pnmtdi->lParam), Event = HIWORD(pnmtdi->lParam);

				if (Channel >= 0 && static_cast<size_t>(Channel) < m_ChannelList.size()) {
					m_ChannelList[Channel]->FormatEventText(Event, &m_TooltipText);
					pnmtdi->lpszText = m_TooltipText.data();
				} else {
					pnmtdi->lpszText = pnmtdi->szText;
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

	case WM_DISPLAYCHANGE:
		m_Offscreen.Destroy();
		return 0;

	case WM_DESTROY:
		m_EventInfoPopupManager.Finalize();
		m_Tooltip.Destroy();
		m_Offscreen.Destroy();
		m_Chevron.Destroy();
		GetAppClass().FeaturedEvents.RemoveEventHandler(this);
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


void CChannelPanel::ApplyStyle()
{
	if (m_hwnd != nullptr) {
		CreateDrawFontAndBoldFont(m_StyleFont, &m_Font, &m_ChannelFont);

		CalcItemHeight();

		if (m_Tooltip.IsCreated())
			SetTooltipFont();

		static const Theme::IconList::ResourceInfo ResourceList[] = {
			{MAKEINTRESOURCE(IDB_CHEVRON10), 10, 10},
			{MAKEINTRESOURCE(IDB_CHEVRON20), 20, 20},
		};
		m_Chevron.Load(
			m_hinst,
			m_Style.ChannelChevronSize.Width,
			m_Style.ChannelChevronSize.Height,
			ResourceList, lengthof(ResourceList));
	}
}


void CChannelPanel::RealizeStyle()
{
	if (m_hwnd != nullptr) {
		if (m_pStyleScaling != nullptr) {
			const int NewDPI = m_pStyleScaling->GetDPI();
			if (m_OldDPI != 0)
				m_ScrollPos = ::MulDiv(m_ScrollPos, NewDPI, m_OldDPI);
			m_OldDPI = NewDPI;
		}

		SetScrollBar();
		Invalidate();
	}
}


void CChannelPanel::Draw(HDC hdc, const RECT *prcPaint)
{
	RECT rcClient;
	GetClientRect(&rcClient);

	if (!m_Offscreen.IsCreated()
			|| m_Offscreen.GetWidth() < rcClient.right
			|| m_Offscreen.GetHeight() < m_ChannelNameHeight + m_EventNameHeight * m_ExpandEvents)
		m_Offscreen.Create(rcClient.right, m_ChannelNameHeight + m_EventNameHeight * m_ExpandEvents, hdc);
	HDC hdcDst = m_Offscreen.GetDC();
	if (hdcDst == nullptr)
		hdcDst = hdc;

	const HFONT hfontOld = static_cast<HFONT>(::GetCurrentObject(hdcDst, OBJ_FONT));
	const COLORREF crOldTextColor = ::GetTextColor(hdcDst);
	const int OldBkMode = ::SetBkMode(hdcDst, TRANSPARENT);

	Theme::CThemeDraw ThemeDraw(BeginThemeDraw(hdcDst));

	CTextDraw TextDraw;
	TextDraw.Begin(
		hdcDst, rcClient,
		CTextDraw::Flag::JapaneseHyphnation |
		CTextDraw::Flag::EndEllipsis);

	RECT rcItem;
	rcItem.left = 0;
	rcItem.right = rcClient.right;
	rcItem.top = -m_ScrollPos;

	for (int i = 0; i < static_cast<int>(m_ChannelList.size()) && rcItem.top < prcPaint->bottom; i++) {
		CChannelEventInfo *pChannelInfo = m_ChannelList[i].get();
		const int NumEvents = pChannelInfo->IsExpanded() ? m_ExpandEvents : m_EventsPerChannel;
		const int ItemHeight = m_ChannelNameHeight + m_EventNameHeight * NumEvents;

		rcItem.bottom = rcItem.top + ItemHeight;
		if (rcItem.bottom > prcPaint->top) {
			const bool fCurrent = pChannelInfo->GetOriginalChannelIndex() == m_CurChannel;

			RECT rc = rcItem;
			if (hdcDst != hdc)
				::OffsetRect(&rc, 0, -rcItem.top);

			rc.bottom = rc.top + m_ChannelNameHeight;

			const Theme::Style &Style =
				fCurrent ? m_Theme.CurChannelNameStyle : m_Theme.ChannelNameStyle;
			DrawUtil::SelectObject(hdcDst, m_ChannelFont);
			::SetTextColor(hdcDst, Style.Fore.Fill.GetSolidColor());
			ThemeDraw.Draw(Style.Back, rc);
			RECT rcName = rc;
			Style::Subtract(&rcName, m_Style.ChannelNameMargin);
			rcName.right -= m_Style.ChannelChevronMargin + m_Style.ChannelChevronSize.Width;
			pChannelInfo->DrawChannelName(hdcDst, &rcName, m_Style.ChannelLogoMargin);
			m_Chevron.Draw(
				hdcDst,
				rcName.right + m_Style.ChannelChevronMargin,
				rcName.top + ((rcName.bottom - rcName.top) - m_Style.ChannelChevronSize.Height) / 2,
				m_Style.ChannelChevronSize.Width,
				m_Style.ChannelChevronSize.Height,
				pChannelInfo->IsExpanded() ? 1 : 0,
				Style.Fore.Fill.GetSolidColor());

			for (int j = 0; j < NumEvents; j++) {
				rc.top = rc.bottom;
				rc.bottom = rc.top + m_EventNameHeight;

				RECT rcContent = rc;

				if (m_fUseEpgColorScheme && pChannelInfo->IsEventEnabled(j)) {
					m_EpgTheme.DrawContentBackground(
						hdcDst, ThemeDraw, rc, pChannelInfo->GetEventInfo(j),
						CEpgTheme::DrawContentBackgroundFlag::Separator);
					::SetTextColor(hdcDst, m_EpgTheme.GetColor(CEpgTheme::COLOR_EVENTNAME));
				} else {
					const Theme::Style &Style =
						(fCurrent ? m_Theme.CurChannelEventStyle : m_Theme.EventStyle)[j % 2];
					ThemeDraw.Draw(Style.Back, &rcContent);
					if (m_fShowGenreColor && pChannelInfo->IsEventEnabled(j)) {
						RECT rcBar = rcContent;
						rcBar.right = m_Style.EventNameMargin.Left;
						CEpgTheme::DrawContentBackgroundFlag Flags = CEpgTheme::DrawContentBackgroundFlag::NoBorder;
						if (rcBar.top == rc.top && rcBar.bottom == rc.bottom)
							Flags |= CEpgTheme::DrawContentBackgroundFlag::Separator;
						m_EpgTheme.DrawContentBackground(
							hdcDst, ThemeDraw, rcBar, pChannelInfo->GetEventInfo(j), Flags);
					}
					::SetTextColor(hdcDst, Style.Fore.Fill.GetSolidColor());
				}

				if (m_fShowFeaturedMark
						&& pChannelInfo->IsEventEnabled(j)
						&& m_FeaturedEventsMatcher.IsMatch(pChannelInfo->GetEventInfo(j))) {
					RECT rcMark = rcContent;
					rcMark.right = m_Style.EventNameMargin.Left;
					Style::Subtract(&rcMark, m_Style.FeaturedMarkMargin);
					rcMark.bottom = rcMark.top + (rcMark.right - rcMark.left);
					ThemeDraw.Draw(m_Theme.FeaturedMarkStyle, rcMark);
				}

				RECT rcText = rc;
				Style::Subtract(&rcText, m_Style.EventNameMargin);

				if (m_fShowProgressBar && j == 0 && pChannelInfo->IsEventEnabled(0)) {
					const LibISDB::EventInfo &EventInfo = pChannelInfo->GetEventInfo(0);
					LONGLONG Elapsed = m_CurTime.DiffSeconds(EventInfo.StartTime);

					if (Elapsed >= 0) {
						RECT rcProgress = rcText;
						if (EventInfo.Duration > 0) {
							if (Elapsed > static_cast<LONGLONG>(EventInfo.Duration))
								Elapsed = EventInfo.Duration;
							const int Width =
								::MulDiv(
									rcProgress.right - rcProgress.left,
									static_cast<int>(Elapsed), EventInfo.Duration);
							if (m_ProgressBarStyle == ProgressBarStyle::Elapsed)
								rcProgress.right = rcProgress.left + Width;
							else
								rcProgress.left += Width;
						}
						if (rcProgress.left < rcProgress.right) {
							rcProgress.top = rcProgress.bottom - m_FontHeight / 3;
							ThemeDraw.Draw(
								fCurrent ? m_Theme.CurProgressStyle : m_Theme.ProgressStyle, rcProgress);
						}
					}
				}

				DrawUtil::SelectObject(hdcDst, m_Font);
				pChannelInfo->DrawEventName(j, TextDraw, rcText, m_FontHeight, m_fUseARIBSymbol);
			}

			if (hdcDst != hdc)
				m_Offscreen.CopyTo(hdc, &rcItem);
		}

		rcItem.top = rcItem.bottom;
	}

	::SetTextColor(hdcDst, crOldTextColor);
	::SetBkMode(hdcDst, OldBkMode);
	SelectFont(hdcDst, hfontOld);
	TextDraw.End();

	if (rcItem.top < prcPaint->bottom) {
		rcItem.left = prcPaint->left;
		if (rcItem.top < prcPaint->top)
			rcItem.top = prcPaint->top;
		rcItem.right = prcPaint->right;
		rcItem.bottom = prcPaint->bottom;
		DrawUtil::Fill(hdc, &rcItem, m_Theme.MarginColor);
	}
}


void CChannelPanel::OnCommand(int ID)
{
	switch (ID) {
	case CM_CHANNELPANEL_UPDATE:
		UpdateAllChannels();
		return;

	case CM_CHANNELPANEL_CURCHANNEL:
		ScrollToCurrentChannel();
		return;

	case CM_CHANNELPANEL_DETAILPOPUP:
		SetDetailToolTip(!m_fDetailToolTip);
		return;

	case CM_CHANNELPANEL_SCROLLTOCURCHANNEL:
		SetScrollToCurChannel(!m_fScrollToCurChannel);
		return;

	case CM_CHANNELPANEL_EVENTS_1:
	case CM_CHANNELPANEL_EVENTS_2:
	case CM_CHANNELPANEL_EVENTS_3:
	case CM_CHANNELPANEL_EVENTS_4:
		SetEventsPerChannel(ID - CM_CHANNELPANEL_EVENTS_1 + 1);
		return;

	case CM_CHANNELPANEL_EXPANDEVENTS_2:
	case CM_CHANNELPANEL_EXPANDEVENTS_3:
	case CM_CHANNELPANEL_EXPANDEVENTS_4:
	case CM_CHANNELPANEL_EXPANDEVENTS_5:
	case CM_CHANNELPANEL_EXPANDEVENTS_6:
	case CM_CHANNELPANEL_EXPANDEVENTS_7:
	case CM_CHANNELPANEL_EXPANDEVENTS_8:
		SetEventsPerChannel(-1, ID - CM_CHANNELPANEL_EXPANDEVENTS_2 + 2);
		return;

	case CM_CHANNELPANEL_USEEPGCOLORSCHEME:
		SetUseEpgColorScheme(!m_fUseEpgColorScheme);
		return;

	case CM_CHANNELPANEL_SHOWGENRECOLOR:
		SetShowGenreColor(!m_fShowGenreColor);
		return;

	case CM_CHANNELPANEL_SHOWFEATUREDMARK:
		SetShowFeaturedMark(!m_fShowFeaturedMark);
		return;

	case CM_CHANNELPANEL_PROGRESSBAR_NONE:
		SetShowProgressBar(false);
		return;

	case CM_CHANNELPANEL_PROGRESSBAR_ELAPSED:
	case CM_CHANNELPANEL_PROGRESSBAR_REMAINING:
		SetProgressBarStyle(
			ID == CM_CHANNELPANEL_PROGRESSBAR_ELAPSED ?
				ProgressBarStyle::Elapsed :
				ProgressBarStyle::Remaining);
		SetShowProgressBar(true);
		return;
	}
}


void CChannelPanel::SetScrollPos(int Pos)
{
	RECT rc;

	GetClientRect(&rc);
	if (Pos < 0) {
		Pos = 0;
	} else {
		const int Height = CalcHeight();
		const int Max = std::max(Height - rc.bottom, 0L);
		if (Pos > Max)
			Pos = Max;
	}
	if (Pos != m_ScrollPos) {
		const int Offset = Pos - m_ScrollPos;

		m_ScrollPos = Pos;
		if (std::abs(Offset) < rc.bottom) {
			::ScrollWindowEx(
				m_hwnd, 0, -Offset,
				nullptr, nullptr, nullptr, nullptr, SW_ERASE | SW_INVALIDATE);
		} else {
			Invalidate();
		}
		SetScrollBar();
		SetTooltips(true);
	}
}


void CChannelPanel::SetScrollBar()
{
	SCROLLINFO si;
	RECT rc;

	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS | SIF_DISABLENOSCROLL;
	si.nMin = 0;
	si.nMax = CalcHeight();
	GetClientRect(&rc);
	si.nPage = rc.bottom;
	si.nPos = m_ScrollPos;
	::SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
}


void CChannelPanel::CalcItemHeight()
{
	const HDC hdc = ::GetDC(m_hwnd);
	if (hdc == nullptr)
		return;
	m_FontHeight = m_Font.GetHeight(hdc);
	m_ChannelNameHeight =
		std::max(m_FontHeight, m_Style.ChannelChevronSize.Height.Value) +
			m_Style.ChannelNameMargin.Top + m_Style.ChannelNameMargin.Bottom;
	m_EventNameHeight =
		m_FontHeight * m_EventNameLines +
			m_Style.EventNameMargin.Top + m_Style.EventNameMargin.Bottom;
	m_ItemHeight = m_ChannelNameHeight + m_EventNameHeight * m_EventsPerChannel;
	m_ExpandedItemHeight = m_ChannelNameHeight + m_EventNameHeight * m_ExpandEvents;
	::ReleaseDC(m_hwnd, hdc);
}


int CChannelPanel::CalcHeight() const
{
	int Height;

	Height = 0;
	for (const auto &e : m_ChannelList) {
		if (e->IsExpanded())
			Height += m_ExpandedItemHeight;
		else
			Height += m_ItemHeight;
	}
	return Height;
}


void CChannelPanel::GetItemRect(int Index, RECT *pRect)
{
	int y;

	y = -m_ScrollPos;
	for (int i = 0; i < Index; i++)
		y += m_ChannelList[i]->IsExpanded() ? m_ExpandedItemHeight : m_ItemHeight;
	GetClientRect(pRect);
	pRect->top = y;
	pRect->bottom = y + (m_ChannelList[Index]->IsExpanded() ? m_ExpandedItemHeight : m_ItemHeight);
}


int CChannelPanel::HitTest(int x, int y, HitType *pType) const
{
	const POINT pt = {x, y};
	RECT rc;

	GetClientRect(&rc);
	rc.top = -m_ScrollPos;

	for (int i = 0; i < static_cast<int>(m_ChannelList.size()); i++) {
		rc.bottom = rc.top + m_ChannelNameHeight;
		if (::PtInRect(&rc, pt)) {
			if (pType != nullptr) {
				if (x < rc.right - (m_Style.ChannelChevronMargin + m_Style.ChannelChevronSize.Width + m_Style.ChannelNameMargin.Right))
					*pType = HIT_CHANNELNAME;
				else if (x >= rc.right - (m_Style.ChannelChevronSize.Width + m_Style.ChannelNameMargin.Right))
					*pType = HIT_CHEVRON;
				else
					*pType = HIT_MARGIN;
			}
			return i;
		}
		const int NumEvents =
			m_ChannelList[i]->IsExpanded() ? m_ExpandEvents : m_EventsPerChannel;
		for (int j = 0; j < NumEvents; j++) {
			rc.top = rc.bottom;
			rc.bottom = rc.top + m_EventNameHeight;
			if (::PtInRect(&rc, pt)) {
				if (pType != nullptr)
					*pType = static_cast<HitType>(HIT_EVENT1 + j);
				return i;
			}
		}
		rc.top = rc.bottom;
	}
	return -1;
}


bool CChannelPanel::CreateTooltip()
{
	if (!m_Tooltip.Create(m_hwnd))
		return false;
	SetTooltipFont();
	m_Tooltip.SetPopDelay(30 * 1000);
	SetTooltips();
	return true;
}


void CChannelPanel::SetTooltipFont()
{
	m_Tooltip.SetFont(m_Font.GetHandle());
	m_Tooltip.SetMaxWidth(m_FontHeight * 20);
}


void CChannelPanel::SetTooltips(bool fRectOnly)
{
	if (m_Tooltip.IsCreated()) {
		int NumTools;
		if (fRectOnly) {
			NumTools = m_Tooltip.NumTools();
		} else {
			m_Tooltip.DeleteAllTools();
			NumTools = 0;
		}

		RECT rc;
		GetClientRect(&rc);
		rc.top = -m_ScrollPos;

		int ToolCount = 0;

		for (int i = 0; i < static_cast<int>(m_ChannelList.size()); i++) {
			rc.top += m_ChannelNameHeight;
			const int NumEvents = m_ChannelList[i]->IsExpanded() ? m_ExpandEvents : m_EventsPerChannel;
			for (int j = 0; j < NumEvents; j++) {
				rc.bottom = rc.top + m_EventNameHeight;
				if (ToolCount < NumTools)
					m_Tooltip.SetToolRect(ToolCount, rc);
				else
					m_Tooltip.AddTool(ToolCount, rc, LPSTR_TEXTCALLBACK, MAKELPARAM(i, j));
				ToolCount++;
				rc.top = rc.bottom;
			}
		}
		if (NumTools > ToolCount) {
			for (int i = NumTools - 1; i >= ToolCount; i--)
				m_Tooltip.DeleteTool(i);
		}
	}
}


bool CChannelPanel::EventInfoPopupHitTest(int x, int y, LPARAM *pParam)
{
	if (m_fDetailToolTip) {
		HitType Type;
		const int Channel = HitTest(x, y, &Type);

		if (Channel >= 0 && Type >= HIT_EVENT1) {
			const int Event = Type - HIT_EVENT1;
			if (m_ChannelList[Channel]->IsEventEnabled(Event)) {
				*pParam = MAKELONG(Channel, Event);
				return true;
			}
		}
	}
	return false;
}


bool CChannelPanel::ShowEventInfoPopup(LPARAM Param, CEventInfoPopup *pPopup)
{
	const int Channel = LOWORD(Param), Event = HIWORD(Param);

	if (Channel < 0 || static_cast<size_t>(Channel) >= m_ChannelList.size())
		return false;
	const CChannelEventInfo *pChEventInfo = m_ChannelList[Channel].get();
	if (!pChEventInfo->IsEventEnabled(Event))
		return false;

	HICON hIcon = nullptr;
	if (m_pLogoManager != nullptr) {
		int IconWidth, IconHeight;
		pPopup->GetPreferredIconSize(&IconWidth, &IconHeight);
		hIcon = m_pLogoManager->CreateLogoIcon(
			pChEventInfo->GetNetworkID(),
			pChEventInfo->GetServiceID(),
			IconWidth, IconHeight);
	}

	RECT rc;
	GetItemRect(Channel, &rc);
	POINT pt = {rc.left, rc.top};
	::ClientToScreen(m_hwnd, &pt);
	const int y = pt.y + m_ChannelNameHeight + m_EventNameHeight * (Event + 1);
	pPopup->GetDefaultPopupPosition(&rc);
	if (rc.top > y) {
		rc.bottom = y + (rc.bottom - rc.top);
		rc.top = y;
	}

	if (!pPopup->Show(
				&pChEventInfo->GetEventInfo(Event), &rc,
				hIcon, pChEventInfo->GetChannelInfo().GetName())) {
		if (hIcon != nullptr)
			::DestroyIcon(hIcon);
		return false;
	}

	return true;
}


void CChannelPanel::ShowMenu(int x, int y)
{
	CPopupMenu Menu(GetAppClass().GetResourceInstance(), IDM_CHANNELPANEL);

	Menu.CheckItem(CM_CHANNELPANEL_DETAILPOPUP, m_fDetailToolTip);
	Menu.CheckItem(CM_CHANNELPANEL_SCROLLTOCURCHANNEL, m_fScrollToCurChannel);
	Menu.CheckRadioItem(
		CM_CHANNELPANEL_EVENTS_1, CM_CHANNELPANEL_EVENTS_4,
		CM_CHANNELPANEL_EVENTS_1 + m_EventsPerChannel - 1);
	Menu.CheckRadioItem(
		CM_CHANNELPANEL_EXPANDEVENTS_2, CM_CHANNELPANEL_EXPANDEVENTS_8,
		CM_CHANNELPANEL_EXPANDEVENTS_2 + m_ExpandAdditionalEvents - 2);
	Menu.CheckItem(CM_CHANNELPANEL_USEEPGCOLORSCHEME, m_fUseEpgColorScheme);
	Menu.CheckItem(CM_CHANNELPANEL_SHOWGENRECOLOR, m_fShowGenreColor);
	Menu.EnableItem(CM_CHANNELPANEL_SHOWGENRECOLOR, !m_fUseEpgColorScheme);
	Menu.CheckItem(CM_CHANNELPANEL_SHOWFEATUREDMARK, m_fShowFeaturedMark);
	Menu.CheckRadioItem(
		CM_CHANNELPANEL_PROGRESSBAR_NONE, CM_CHANNELPANEL_PROGRESSBAR_REMAINING,
		!m_fShowProgressBar ?
			CM_CHANNELPANEL_PROGRESSBAR_NONE :
		m_ProgressBarStyle == ProgressBarStyle::Elapsed ?
			CM_CHANNELPANEL_PROGRESSBAR_ELAPSED :
			CM_CHANNELPANEL_PROGRESSBAR_REMAINING);

	const POINT pt = {x, y};
	Menu.Show(m_hwnd, &pt);
}


void CChannelPanel::OnFeaturedEventsSettingsChanged(CFeaturedEvents &FeaturedEvents)
{
	if (m_fShowFeaturedMark) {
		m_FeaturedEventsMatcher.BeginMatching(FeaturedEvents.GetSettings());
		Invalidate();
	}
}


CChannelPanel::CEventInfoPopupHandler::CEventInfoPopupHandler(CChannelPanel *pChannelPanel)
	: m_pChannelPanel(pChannelPanel)
{
}


bool CChannelPanel::CEventInfoPopupHandler::HitTest(int x, int y, LPARAM *pParam)
{
	return m_pChannelPanel->EventInfoPopupHitTest(x, y, pParam);
}


bool CChannelPanel::CEventInfoPopupHandler::ShowPopup(LPARAM Param, CEventInfoPopup *pPopup)
{
	return m_pChannelPanel->ShowEventInfoPopup(Param, pPopup);
}




CChannelPanel::CChannelEventInfo::CChannelEventInfo(const CChannelInfo *pInfo, int OriginalIndex)
	: m_ChannelInfo(*pInfo)
	, m_OriginalChannelIndex(OriginalIndex)
{
}


bool CChannelPanel::CChannelEventInfo::SetEventInfo(int Index, const LibISDB::EventInfo *pInfo)
{
	if (Index < 0)
		return false;
	bool fChanged = false;
	if (pInfo != nullptr) {
		if (static_cast<int>(m_EventList.size()) <= Index)
			m_EventList.resize(Index + 1);
		if (!m_EventList[Index].IsEqual(*pInfo)) {
			m_EventList[Index] = *pInfo;
			fChanged = true;
		}
	} else {
		if (Index < static_cast<int>(m_EventList.size())
				&& m_EventList[Index].StartTime.IsValid()) {
			m_EventList[Index] = LibISDB::EventInfo();
			fChanged = true;
		}
	}
	return fChanged;
}


void CChannelPanel::CChannelEventInfo::SetMaxEvents(int Events)
{
	if (Events > static_cast<int>(m_EventList.size()))
		m_EventList.resize(Events);
}


bool CChannelPanel::CChannelEventInfo::IsEventEnabled(int Index) const
{
	if (Index < 0 || Index >= static_cast<int>(m_EventList.size()))
		return false;
	return m_EventList[Index].StartTime.IsValid();
}


bool CChannelPanel::CChannelEventInfo::FormatEventText(int Index, String *pText) const
{
	if (!IsEventEnabled(Index)) {
		pText->clear();
		return false;
	}

	const LibISDB::EventInfo &Info = m_EventList[Index];
	TCHAR szTime[EpgUtil::MAX_EVENT_TIME_LENGTH];
	EpgUtil::FormatEventTime(Info, szTime, lengthof(szTime));
	StringFormat(
		pText, TEXT("{} {}{}{}"),
		szTime,
		Info.EventName,
		!Info.EventText.empty() ? TEXT("\n\n") : TEXT(""),
		Info.EventText);
	StringUtility::Trim(*pText, TEXT(" \r\n"));
	return true;
}


void CChannelPanel::CChannelEventInfo::DrawChannelName(
	HDC hdc, const RECT *pRect, const Style::Margins &LogoMargins)
{
	RECT rc = *pRect;

	if (m_hbmLogo != nullptr) {
		const int LogoHeight = (rc.bottom - rc.top) - (LogoMargins.Top + LogoMargins.Bottom);
		if (LogoHeight > 0) {
			const int LogoWidth = LogoHeight * 16 / 9;
			// AlphaBlendでリサイズすると汚いので、予めリサイズした画像を作成しておく
			if (m_StretchedLogo.IsCreated()) {
				if (m_StretchedLogo.GetWidth() != LogoWidth || m_StretchedLogo.GetHeight() != LogoHeight)
					m_StretchedLogo.Destroy();
			}
			if (!m_StretchedLogo.IsCreated()) {
				const HBITMAP hbm = DrawUtil::ResizeBitmap(m_hbmLogo, LogoWidth, LogoHeight);
				if (hbm != nullptr)
					m_StretchedLogo.Attach(hbm);
			}
			rc.left += LogoMargins.Left;
			DrawUtil::DrawBitmap(
				hdc, rc.left, rc.top + LogoMargins.Top,
				LogoWidth, LogoHeight,
				m_StretchedLogo.IsCreated() ? m_StretchedLogo.GetHandle() : m_hbmLogo, nullptr, 192);
			rc.left += LogoWidth + LogoMargins.Right;
		}
	}

	TCHAR szText[MAX_CHANNEL_NAME + 16];
	if (m_ChannelInfo.GetChannelNo() != 0)
		StringFormat(
			szText, TEXT("{}: {}"),
			m_ChannelInfo.GetChannelNo(), m_ChannelInfo.GetName());
	else
		StringCopy(szText, m_ChannelInfo.GetName());
	::DrawText(
		hdc, szText, -1, &rc,
		DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
}


void CChannelPanel::CChannelEventInfo::DrawEventName(
	int Index, CTextDraw &TextDraw, const RECT &Rect, int LineHeight, bool fUseARIBSymbol)
{
	if (IsEventEnabled(Index)) {
		const LibISDB::EventInfo &Info = m_EventList[Index];
		TCHAR szText[256];

		int Length = EpgUtil::FormatEventTime(Info, szText, lengthof(szText), EpgUtil::FormatEventTimeFlag::Hour2Digits);
		szText[Length++] = TEXT(' ');
		if (fUseARIBSymbol)
			EpgUtil::MapARIBSymbol(Info.EventName.c_str(), szText + Length, lengthof(szText) - Length);
		else
			StringFormat(szText + Length, lengthof(szText) - Length, TEXT("{}"), Info.EventName);
		TextDraw.Draw(szText, Rect, LineHeight);
	}
}




void CChannelPanel::ChannelPanelStyle::SetStyle(const Style::CStyleManager *pStyleManager)
{
	*this = ChannelPanelStyle();
	pStyleManager->Get(TEXT("channel-list-panel.channel-name.margin"), &ChannelNameMargin);
	pStyleManager->Get(TEXT("channel-list-panel.channel-name.logo.margin"), &ChannelLogoMargin);
	pStyleManager->Get(TEXT("channel-list-panel.channel-name.chevron"), &ChannelChevronSize);
	pStyleManager->Get(TEXT("channel-list-panel.channel-name.chevron.margin"), &ChannelChevronMargin);
	pStyleManager->Get(TEXT("channel-list-panel.event-name.margin"), &EventNameMargin);
	pStyleManager->Get(TEXT("channel-list-panel.featured-mark.margin"), &FeaturedMarkMargin);
}


void CChannelPanel::ChannelPanelStyle::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	pStyleScaling->ToPixels(&ChannelNameMargin);
	pStyleScaling->ToPixels(&ChannelLogoMargin);
	pStyleScaling->ToPixels(&ChannelChevronSize);
	pStyleScaling->ToPixels(&ChannelChevronMargin);
	pStyleScaling->ToPixels(&EventNameMargin);
	pStyleScaling->ToPixels(&FeaturedMarkMargin);
}


} // namespace TVTest
