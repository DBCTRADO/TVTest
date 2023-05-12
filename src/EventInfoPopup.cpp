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
#include "EventInfoPopup.h"
#include "Aero.h"
#include "DarkMode.h"
#include "Common/DebugDef.h"


namespace TVTest
{


const LPCTSTR CEventInfoPopup::m_pszWindowClass = APP_NAME TEXT(" Event Info");
HINSTANCE CEventInfoPopup::m_hinst = nullptr;


bool CEventInfoPopup::Initialize(HINSTANCE hinst)
{
	if (m_hinst == nullptr) {
		WNDCLASS wc;

		wc.style = CS_HREDRAW;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hinst;
		wc.hIcon = nullptr;
		wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = nullptr;
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = m_pszWindowClass;
		if (RegisterClass(&wc) == 0)
			return false;
		m_hinst = hinst;
	}
	return true;
}


CEventInfoPopup::CEventInfoPopup()
{
	m_WindowPosition.Width = 320;
	m_WindowPosition.Height = 320;

	GetDefaultFont(&m_StyleFont);

	SetStyleScaling(&m_StyleScaling);
}


CEventInfoPopup::~CEventInfoPopup()
{
	Destroy();
	if (m_pEventHandler != nullptr)
		m_pEventHandler->m_pPopup = nullptr;
}


bool CEventInfoPopup::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	return CreateBasicWindow(hwndParent, Style, ExStyle, ID, m_pszWindowClass, nullptr, m_hinst);
}


void CEventInfoPopup::SetEventInfo(const LibISDB::EventInfo *pEventInfo)
{
	if (m_EventInfo.IsEqual(*pEventInfo))
		return;

	m_EventInfo = *pEventInfo;

	UpdateEventInfo();
}


void CEventInfoPopup::UpdateEventInfo()
{
	LOGFONT lf;
	CHARFORMAT cf;
	const HDC hdc = ::GetDC(m_hwndEdit);
	m_Font.GetLogFont(&lf);
	CRichEditUtil::LogFontToCharFormat(hdc, &lf, &cf);
	cf.dwMask |= CFM_COLOR;
	cf.crTextColor = m_TextColor;
	::ReleaseDC(m_hwndEdit, hdc);
	::SendMessage(m_hwndEdit, WM_SETREDRAW, FALSE, 0);
	::SetWindowText(m_hwndEdit, nullptr);

	TCHAR szText[4096];
	CStaticStringFormatter Formatter(szText, lengthof(szText));
	int TitleLines = 0;

	{
		TCHAR szBuf[EpgUtil::MAX_EVENT_TIME_LENGTH];
		if (EpgUtil::FormatEventTime(
					m_EventInfo, szBuf, lengthof(szBuf),
					EpgUtil::FormatEventTimeFlag::Date | EpgUtil::FormatEventTimeFlag::Year) > 0) {
			Formatter.Append(szBuf);
			Formatter.Append(TEXT("\r\n"));
			TitleLines++;
		}
	}

	if (!m_EventInfo.EventName.empty()) {
		Formatter.Append(m_EventInfo.EventName.c_str());
		Formatter.Append(TEXT("\r\n"));
		TitleLines++;
	}

	if (!Formatter.IsEmpty()) {
		Formatter.Append(TEXT("\r\n"));
		CHARFORMAT cfBold = cf;
		cfBold.dwMask |= CFM_BOLD;
		cfBold.dwEffects |= CFE_BOLD;
		cfBold.crTextColor = m_EventTitleColor;
		CRichEditUtil::AppendText(m_hwndEdit, Formatter.GetString(), &cfBold);
		Formatter.Clear();
	}

	if (!m_EventInfo.EventText.empty()) {
		Formatter.Append(m_EventInfo.EventText.c_str());
		Formatter.RemoveTrailingWhitespace();
	}
	if (!m_EventInfo.ExtendedText.empty()) {
		if (!m_EventInfo.EventText.empty())
			Formatter.Append(TEXT("\r\n\r\n"));
		String Text;
		m_EventInfo.GetConcatenatedExtendedText(&Text);
		Formatter.Append(Text.c_str());
		Formatter.RemoveTrailingWhitespace();
	}

	Formatter.Append(TEXT("\r\n"));

	if (!m_EventInfo.VideoList.empty()) {
		// TODO: 複数映像対応
		LPCTSTR pszVideo = LibISDB::GetComponentTypeText_ja(
			m_EventInfo.VideoList[0].StreamContent,
			m_EventInfo.VideoList[0].ComponentType);
		if (pszVideo != nullptr) {
			Formatter.AppendFormat(TEXT("\r\n■ 映像： {}"), pszVideo);
		}
	}

	if (!m_EventInfo.AudioList.empty()) {
		const LibISDB::EventInfo::AudioInfo *pMainAudioInfo = m_EventInfo.GetMainAudioInfo();
		TCHAR szBuff[64];

		Formatter.Append(TEXT("\r\n■ 音声： "));
		if (m_EventInfo.AudioList.size() == 1) {
			FormatAudioInfo(pMainAudioInfo, szBuff, lengthof(szBuff));
			Formatter.Append(szBuff);
		} else {
			Formatter.Append(TEXT("主: "));
			FormatAudioInfo(pMainAudioInfo, szBuff, lengthof(szBuff));
			Formatter.Append(szBuff);
			for (size_t i = 0; i < m_EventInfo.AudioList.size(); i++) {
				const LibISDB::EventInfo::AudioInfo *pAudioInfo = &m_EventInfo.AudioList[i];
				if (pAudioInfo != pMainAudioInfo) {
					Formatter.Append(TEXT(" / 副: "));
					FormatAudioInfo(pAudioInfo, szBuff, lengthof(szBuff));
					Formatter.Append(szBuff);
				}
			}
		}
	}

	int Genre1, Genre2;
	if (EpgUtil::GetEventGenre(m_EventInfo, &Genre1, &Genre2)) {
		CEpgGenre EpgGenre;
		LPCTSTR pszGenre = EpgGenre.GetText(Genre1, -1);
		if (pszGenre != nullptr) {
			Formatter.AppendFormat(TEXT("\r\n■ ジャンル： {}"), pszGenre);
			pszGenre = EpgGenre.GetText(Genre1, Genre2);
			if (pszGenre != nullptr)
				Formatter.AppendFormat(TEXT(" - {}"), pszGenre);
		}
	}

	if (m_fDetailInfo) {
		Formatter.AppendFormat(TEXT("\r\n■ イベントID： {:#04x}"), m_EventInfo.EventID);
		if (m_EventInfo.IsCommonEvent)
			Formatter.AppendFormat(
				TEXT(" (イベント共有 サービスID {:#04x} / イベントID {:#04x})"),
				m_EventInfo.CommonEvent.ServiceID,
				m_EventInfo.CommonEvent.EventID);
	}

	CRichEditUtil::AppendText(
		m_hwndEdit, SkipLeadingWhitespace(Formatter.GetString()), &cf);
	m_RichEditLink.DetectURL(m_hwndEdit, &cf, TitleLines + 1);

	POINT pt = {0, 0};
	::SendMessage(m_hwndEdit, EM_SETSCROLLPOS, 0, reinterpret_cast<LPARAM>(&pt));
	::SendMessage(m_hwndEdit, WM_SETREDRAW, TRUE, 0);
	::InvalidateRect(m_hwndEdit, nullptr, TRUE);

	m_TitleBackColor = m_EpgTheme.GetGenreColor(m_EventInfo),
	m_TitleTextColor = m_EpgTheme.GetColor(CEpgTheme::COLOR_EVENTNAME);

	CalcTitleHeight();
	RECT rc;
	GetClientRect(&rc);
	::MoveWindow(m_hwndEdit, 0, m_TitleHeight, rc.right, std::max(rc.bottom - m_TitleHeight, 0L), TRUE);
	Invalidate();
	::RedrawWindow(m_hwnd, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE);
}


void CEventInfoPopup::FormatAudioInfo(
	const LibISDB::EventInfo::AudioInfo *pAudioInfo, LPTSTR pszText, int MaxLength) const
{
	LPCTSTR pszAudio;
	bool fBilingual = false;

	if (pAudioInfo->ComponentType == 0x02
			&& pAudioInfo->ESMultiLingualFlag
			&& pAudioInfo->LanguageCode != pAudioInfo->LanguageCode2) {
		pszAudio = TEXT("Mono 二カ国語");
		fBilingual = true;
	} else {
		pszAudio = LibISDB::GetComponentTypeText_ja(
			pAudioInfo->StreamContent, pAudioInfo->ComponentType);
	}

	LPCTSTR p = pAudioInfo->Text.c_str();
	TCHAR szAudioComponent[64];
	szAudioComponent[0] = _T('\0');
	if (*p != _T('\0')) {
		szAudioComponent[0] = _T(' ');
		szAudioComponent[1] = _T('[');
		size_t i;
		for (i = 2; *p != _T('\0') && i < lengthof(szAudioComponent) - 2; i++) {
			if (*p == _T('\r') || *p == _T('\n')) {
				szAudioComponent[i] = _T('/');
				p++;
				if (*p == _T('\n'))
					p++;
			} else {
				szAudioComponent[i] = *p++;
			}
		}
		szAudioComponent[i + 0] = _T(']');
		szAudioComponent[i + 1] = _T('\0');
	} else if (fBilingual) {
		TCHAR szLang1[LibISDB::MAX_LANGUAGE_TEXT_LENGTH];
		TCHAR szLang2[LibISDB::MAX_LANGUAGE_TEXT_LENGTH];
		LibISDB::GetLanguageText_ja(pAudioInfo->LanguageCode, szLang1, lengthof(szLang1));
		LibISDB::GetLanguageText_ja(pAudioInfo->LanguageCode2, szLang2, lengthof(szLang2));
		StringFormat(
			szAudioComponent,
			TEXT(" [{}/{}]"), szLang1, szLang2);
	} else {
		TCHAR szLang[LibISDB::MAX_LANGUAGE_TEXT_LENGTH];
		LibISDB::GetLanguageText_ja(pAudioInfo->LanguageCode, szLang, lengthof(szLang));
		StringFormat(
			szAudioComponent,
			TEXT(" [{}]"), szLang);
	}

	StringFormat(
		pszText, MaxLength, TEXT("{}{}"),
		pszAudio != nullptr ? pszAudio : TEXT("?"),
		szAudioComponent);
}


void CEventInfoPopup::CalcTitleHeight()
{
	int FontHeight = 0;
	const HDC hdc = ::GetDC(m_hwnd);
	if (hdc != nullptr) {
		const HFONT hfontOld = DrawUtil::SelectObject(hdc, m_TitleFont);
		TEXTMETRIC tm;
		::GetTextMetrics(hdc, &tm);
		FontHeight = tm.tmHeight;
		::SelectObject(hdc, hfontOld);
		::ReleaseDC(m_hwnd, hdc);
	}

	const int IconHeight = m_pStyleScaling->GetScaledSystemMetrics(SM_CYSMICON);
	const int ButtonHeight = m_ButtonSize + m_ButtonMargin * 2;
	const int Margin = m_pStyleScaling->LogicalPixelsToPhysicalPixels(2);

	m_TitleHeight = std::max(IconHeight, ButtonHeight);
	m_TitleHeight = std::max(m_TitleHeight, FontHeight + Margin);
}


bool CEventInfoPopup::Show(
	const LibISDB::EventInfo *pEventInfo, const RECT *pPos,
	HICON hIcon, LPCTSTR pszTitle)
{
	if (pEventInfo == nullptr)
		return false;

	if (m_hwnd == nullptr) {
		if (!Create(nullptr, WS_POPUP | WS_CLIPCHILDREN | WS_THICKFRAME, WS_EX_TOPMOST | WS_EX_NOACTIVATE, 0))
			return false;
	}

	if (!GetVisible() || m_EventInfo != *pEventInfo) {
		if (pPos != nullptr) {
			SetPosition(pPos);
		} else {
			RECT rc;

			GetDefaultPopupPosition(&rc);
			::SetWindowPos(
				m_hwnd, HWND_TOPMOST,
				rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
				SWP_NOACTIVATE);
		}
	}

	SetEventInfo(pEventInfo);

	if (pszTitle != nullptr)
		m_TitleText = pszTitle;
	else
		m_TitleText.clear();

	m_TitleIcon.Attach(hIcon);

	::ShowWindow(m_hwnd, SW_SHOWNA);

	return true;
}


bool CEventInfoPopup::Hide()
{
	if (m_hwnd != nullptr)
		::ShowWindow(m_hwnd, SW_HIDE);
	return true;
}


bool CEventInfoPopup::IsVisible()
{
	return m_hwnd != nullptr && GetVisible();
}


bool CEventInfoPopup::IsOwnWindow(HWND hwnd) const
{
	if (hwnd == nullptr)
		return false;
	return hwnd == m_hwnd || hwnd == m_hwndEdit;
}


void CEventInfoPopup::GetSize(int *pWidth, int *pHeight) const
{
	RECT rc;

	GetPosition(&rc);
	if (pWidth != nullptr)
		*pWidth = rc.right - rc.left;
	if (pHeight != nullptr)
		*pHeight = rc.bottom - rc.top;
}


bool CEventInfoPopup::SetSize(int Width, int Height)
{
	if (Width < 0 || Height < 0)
		return false;
	RECT rc;
	GetPosition(&rc);
	rc.right = rc.left + Width;
	rc.bottom = rc.top + Height;
	return SetPosition(&rc);
}


void CEventInfoPopup::SetTheme(const Theme::CThemeManager *pThemeManager)
{
	m_EpgTheme.SetTheme(pThemeManager);

	m_BackColor = pThemeManager->GetColor(CColorScheme::COLOR_EVENTINFOPOPUP_BACK);
	m_TextColor = pThemeManager->GetColor(CColorScheme::COLOR_EVENTINFOPOPUP_TEXT);
	m_EventTitleColor = pThemeManager->GetColor(CColorScheme::COLOR_EVENTINFOPOPUP_EVENTTITLE);

	if (m_hwnd != nullptr) {
		::SendMessage(m_hwndEdit, EM_SETBKGNDCOLOR, 0, m_BackColor);
		if (::GetWindowTextLength(m_hwndEdit) > 0)
			UpdateEventInfo();
		//::InvalidateRect(m_hwndEdit, nullptr, TRUE);

		if (IsDarkThemeSupported())
			SetWindowDarkTheme(m_hwndEdit, IsDarkThemeColor(m_BackColor));
	}
}


bool CEventInfoPopup::SetFont(const Style::Font &Font)
{
	m_StyleFont = Font;

	if (m_hwnd != nullptr) {
		ApplyStyle();
		RealizeStyle();
	}

	return true;
}


void CEventInfoPopup::SetEventHandler(CEventHandler *pEventHandler)
{
	if (m_pEventHandler != nullptr)
		m_pEventHandler->m_pPopup = nullptr;
	if (pEventHandler != nullptr)
		pEventHandler->m_pPopup = this;
	m_pEventHandler = pEventHandler;
}


bool CEventInfoPopup::IsSelected() const
{
	return CRichEditUtil::IsSelected(m_hwndEdit);
}


String CEventInfoPopup::GetSelectedText() const
{
	return CRichEditUtil::GetSelectedText(m_hwndEdit);
}


void CEventInfoPopup::GetPreferredIconSize(int *pWidth, int *pHeight) const
{
	if (pWidth != nullptr)
		*pWidth = m_pStyleScaling->GetScaledSystemMetrics(SM_CXSMICON);
	if (pHeight != nullptr)
		*pHeight = m_pStyleScaling->GetScaledSystemMetrics(SM_CYSMICON);
}


bool CEventInfoPopup::GetPopupPosition(int x, int y, RECT *pPos) const
{
	if (pPos == nullptr)
		return false;

	RECT rc;
	GetPosition(&rc);
	const int Width = rc.right - rc.left;
	const int Height = rc.bottom - rc.top;

	POINT pt = {x, y};
	const HMONITOR hMonitor = ::MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
	if (hMonitor != nullptr) {
		MONITORINFO mi;

		mi.cbSize = sizeof(mi);
		if (::GetMonitorInfo(hMonitor, &mi)) {
			if (x + Width > mi.rcMonitor.right)
				x = mi.rcMonitor.right - Width;
			if (y + Height > mi.rcMonitor.bottom) {
				y = mi.rcMonitor.bottom - Height;
				if (x + Width < mi.rcMonitor.right)
					x += std::min(16L, mi.rcMonitor.right - (x + Width));
			}
		}
	}

	pPos->left = x;
	pPos->right = x + Width;
	pPos->top = y;
	pPos->bottom = y + Height;

	return true;
}


bool CEventInfoPopup::AdjustPopupPosition(POINT *pPos) const
{
	if (pPos == nullptr)
		return false;

	RECT rc;
	if (!GetPopupPosition(pPos->x, pPos->y, &rc))
		return false;

	pPos->x = rc.left;
	pPos->y = rc.top;

	return true;
}


bool CEventInfoPopup::GetDefaultPopupPosition(RECT *pPos) const
{
	if (pPos == nullptr)
		return false;

	POINT pt;
	::GetCursorPos(&pt);

	return GetPopupPosition(pt.x, pt.y + 16, pPos);
}


bool CEventInfoPopup::GetDefaultPopupPosition(POINT *pPos) const
{
	if (pPos == nullptr)
		return false;

	RECT rc;
	if (!GetDefaultPopupPosition(&rc))
		return false;

	pPos->x = rc.left;
	pPos->y = rc.top;

	return true;
}


LRESULT CEventInfoPopup::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		InitializeUI();

		m_RichEditUtil.LoadRichEditLib();
		m_hwndEdit = ::CreateWindowEx(
			0, m_RichEditUtil.GetWindowClassName(), TEXT(""),
			WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_NOHIDESEL,
			0, 0, 0, 0, hwnd, (HMENU)1, m_hinst, nullptr);
		CRichEditUtil::DisableAutoFont(m_hwndEdit);
		SetWindowFont(m_hwndEdit, m_Font.GetHandle(), FALSE);
		::SendMessage(m_hwndEdit, EM_SETEVENTMASK, 0, ENM_MOUSEEVENTS | ENM_LINK);
		::SendMessage(m_hwndEdit, EM_SETBKGNDCOLOR, 0, m_BackColor);

		if (IsDarkThemeSupported())
			SetWindowDarkTheme(m_hwndEdit, IsDarkThemeColor(m_BackColor));

		SetNcRendering();
		return 0;

	case WM_NCCREATE:
		InitStyleScaling(hwnd, true);
		break;

	case WM_SIZE:
		CalcTitleHeight();
		::MoveWindow(
			m_hwndEdit, 0, m_TitleHeight,
			LOWORD(lParam), std::max(HIWORD(lParam) - m_TitleHeight, 0), TRUE);
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rc;

			::BeginPaint(hwnd, &ps);

			::GetClientRect(hwnd, &rc);
			rc.bottom = m_TitleHeight;
			DrawUtil::Fill(ps.hdc, &rc, m_TitleBackColor);

			rc.left += m_TitleLeftMargin;

			if (m_TitleIcon) {
				int IconWidth, IconHeight;

				GetPreferredIconSize(&IconWidth, &IconHeight);
				::DrawIconEx(
					ps.hdc,
					rc.left, rc.top + (m_TitleHeight - IconHeight) / 2,
					m_TitleIcon, IconWidth, IconHeight, 0, nullptr, DI_NORMAL);
				rc.left += IconWidth + m_TitleIconTextMargin;
			}

			if (!m_TitleText.empty()) {
				rc.right -= m_ButtonSize + m_ButtonMargin * 2;
				if (rc.left < rc.right) {
					const HFONT hfontOld = DrawUtil::SelectObject(ps.hdc, m_TitleFont);
					const int OldBkMode = ::SetBkMode(ps.hdc, TRANSPARENT);
					const COLORREF OldTextColor = ::SetTextColor(ps.hdc, m_TitleTextColor);
					::DrawText(
						ps.hdc, m_TitleText.data(), static_cast<int>(m_TitleText.length()), &rc,
						DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
					::SelectObject(ps.hdc, hfontOld);
					::SetBkMode(ps.hdc, OldBkMode);
					::SetTextColor(ps.hdc, OldTextColor);
				}
			}

			GetCloseButtonRect(&rc);
			::DrawFrameControl(ps.hdc, &rc, DFC_CAPTION, DFCS_CAPTIONCLOSE | DFCS_MONO);

			::EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE) {
			Hide();
		}
		return 0;

	case WM_ACTIVATEAPP:
		if (wParam == 0) {
			Hide();
		}
		return 0;

	case WM_NCHITTEST:
		{
			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			RECT rc;

			::ScreenToClient(hwnd, &pt);
			GetCloseButtonRect(&rc);
			if (::PtInRect(&rc, pt))
				return HTCLOSE;
			::GetClientRect(hwnd, &rc);
			rc.bottom = m_TitleHeight;
			if (::PtInRect(&rc, pt))
				return HTCAPTION;
		}
		break;

	case WM_NCLBUTTONDOWN:
		if (wParam == HTCLOSE) {
			::SendMessage(hwnd, WM_CLOSE, 0, 0);
			return 0;
		}
		break;

	case WM_NCRBUTTONDOWN:
		if (wParam == HTCAPTION) {
			const POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			const HMENU hmenu = ::CreatePopupMenu();

			::AppendMenu(hmenu, MF_STRING | MF_ENABLED, 1, TEXT("番組名をコピー(&C)"));
			const int Command = ::TrackPopupMenu(hmenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, nullptr);
			::DestroyMenu(hmenu);
			switch (Command) {
			case 1:
				CopyTextToClipboard(hwnd, m_EventInfo.EventName.c_str());
				break;
			}
			return 0;
		}
		break;

	case WM_MOUSEWHEEL:
		return ::SendMessage(m_hwndEdit, uMsg, wParam, lParam);

	case WM_NCMOUSEMOVE:
		m_fCursorInWindow = true;
		return 0;

	case WM_SHOWWINDOW:
		if (wParam != 0) {
			::SetTimer(hwnd, TIMER_ID_HIDE, 200, nullptr);
			m_fCursorInWindow = false;
		} else {
			::KillTimer(hwnd, TIMER_ID_HIDE);
		}
		return 0;

	case WM_TIMER:
		if (wParam == TIMER_ID_HIDE) {
			if (!m_fMenuShowing) {
				POINT pt;

				::GetCursorPos(&pt);
				if (!m_fCursorInWindow) {
					if (IsOwnWindow(::WindowFromPoint(pt)))
						m_fCursorInWindow = true;
				} else {
					RECT rc;

					::GetWindowRect(hwnd, &rc);
					::InflateRect(
						&rc,
						m_pStyleScaling->GetScaledSystemMetrics(SM_CXSIZEFRAME) * 2,
						m_pStyleScaling->GetScaledSystemMetrics(SM_CYSIZEFRAME) * 2);
					if (!::PtInRect(&rc, pt))
						Hide();
				}
			}
		}
		return 0;

	case WM_ENTERMENULOOP:
		m_fMenuShowing = true;
		return 0;

	case WM_EXITMENULOOP:
		m_fMenuShowing = false;
		m_fCursorInWindow = false;
		return 0;

	case WM_NCACTIVATE:
		return TRUE;

	case WM_NCPAINT:
		{
			const HDC hdc = ::GetWindowDC(hwnd);
			RECT rcWindow, rcClient;
			const int LineWidth = GetHairlineWidth();

			::GetWindowRect(hwnd, &rcWindow);
			::GetClientRect(hwnd, &rcClient);
			MapWindowRect(hwnd, nullptr, &rcClient);
			::OffsetRect(&rcClient, -rcWindow.left, -rcWindow.top);
			::OffsetRect(&rcWindow, -rcWindow.left, -rcWindow.top);
			DrawUtil::FillBorder(hdc, &rcWindow, &rcClient, &rcWindow, m_TitleBackColor);
			const HPEN hpen = ::CreatePen(PS_INSIDEFRAME, LineWidth, MixColor(m_TitleBackColor, RGB(0, 0, 0), 192));
			const HGDIOBJ hOldPen = ::SelectObject(hdc, hpen);
			const HGDIOBJ hOldBrush = ::SelectObject(hdc, ::GetStockObject(NULL_BRUSH));
			::Rectangle(hdc, rcWindow.left, rcWindow.top, rcWindow.right, rcWindow.bottom);
			::Rectangle(hdc, rcClient.left - LineWidth, rcClient.top - LineWidth, rcClient.right + LineWidth, rcClient.bottom + LineWidth);
			::SelectObject(hdc, hOldBrush);
			::SelectObject(hdc, hOldPen);
			::DeleteObject(hpen);
			::ReleaseDC(hwnd, hdc);
		}
		return 0;

	case WM_DWMCOMPOSITIONCHANGED:
		SetNcRendering();
		return 0;

	case WM_DPICHANGED:
		OnDPIChanged(hwnd, wParam, lParam);
		break;

	case WM_SETCURSOR:
		if (m_RichEditLink.OnSetCursor(hwnd, wParam, lParam))
			return TRUE;
		break;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case EN_MSGFILTER:
			{
				MSGFILTER *pMsgFilter = reinterpret_cast<MSGFILTER*>(lParam);

				switch (pMsgFilter->msg) {
				case WM_RBUTTONDOWN:
					ShowContextMenu();
					break;

				default:
					m_RichEditLink.OnMsgFilter(pMsgFilter);
					break;
				}
			}
			return 0;

#if 0
		case EN_LINK:
			{
				ENLINK *penl = reinterpret_cast<ENLINK*>(lParam);

				if (penl->msg == WM_LBUTTONUP)
					CRichEditUtil::HandleLinkClick(penl);
			}
			return 0;
#endif
		}
		break;

	case WM_CLOSE:
		Hide();
		return 0;

	case WM_DESTROY:
		m_TitleIcon.Destroy();
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


void CEventInfoPopup::ApplyStyle()
{
	if (m_hwnd != nullptr) {
		CreateDrawFontAndBoldFont(m_StyleFont, &m_Font, &m_TitleFont);
	}
}


void CEventInfoPopup::RealizeStyle()
{
	if (m_hwnd != nullptr) {
		SendSizeMessage();
		Invalidate();
		SetWindowFont(m_hwndEdit, m_Font.GetHandle(), TRUE);
	}
}


void CEventInfoPopup::GetCloseButtonRect(RECT *pRect) const
{
	RECT rc;

	GetClientRect(&rc);
	rc.right -= m_ButtonMargin;
	rc.left = rc.right - m_ButtonSize;
	rc.top = m_ButtonMargin;
	rc.bottom = rc.top + m_ButtonSize;
	*pRect = rc;
}


void CEventInfoPopup::SetNcRendering()
{
	CAeroGlass Aero;

	if (Aero.IsEnabled())
		Aero.EnableNcRendering(m_hwnd, false);
}


void CEventInfoPopup::ShowContextMenu()
{
	enum {
		COMMAND_COPY = 1,
		COMMAND_SELECTALL,
		COMMAND_COPYEVENTNAME,
		COMMAND_SEARCH
	};
	const HMENU hmenu = ::CreatePopupMenu();

	::AppendMenu(hmenu, MF_STRING | MF_ENABLED, COMMAND_COPY, TEXT("コピー(&C)"));
	::AppendMenu(hmenu, MF_STRING | MF_ENABLED, COMMAND_SELECTALL, TEXT("すべて選択(&A)"));
	::AppendMenu(hmenu, MF_STRING | MF_ENABLED, COMMAND_COPYEVENTNAME, TEXT("番組名をコピー(&E)"));
	if (m_pEventHandler != nullptr)
		m_pEventHandler->OnMenuPopup(hmenu);
	if (CRichEditUtil::IsSelected(m_hwndEdit)) {
		const CKeywordSearch &KeywordSearch = GetAppClass().KeywordSearch;
		if (KeywordSearch.GetSearchEngineCount() > 0) {
			::AppendMenu(hmenu, MF_SEPARATOR, 0, nullptr);
			KeywordSearch.InitializeMenu(hmenu, COMMAND_SEARCH, CEventHandler::COMMAND_FIRST - COMMAND_SEARCH);
		}
	}

	POINT pt;
	::GetCursorPos(&pt);
	const int Command = ::TrackPopupMenu(hmenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, m_hwnd, nullptr);
	::DestroyMenu(hmenu);

	switch (Command) {
	case COMMAND_COPY:
		if (::SendMessage(m_hwndEdit, EM_SELECTIONTYPE, 0, 0) == SEL_EMPTY) {
			CRichEditUtil::CopyAllText(m_hwndEdit);
		} else {
			::SendMessage(m_hwndEdit, WM_COPY, 0, 0);
		}
		break;

	case COMMAND_SELECTALL:
		CRichEditUtil::SelectAll(m_hwndEdit);
		break;

	case COMMAND_COPYEVENTNAME:
		CopyTextToClipboard(m_hwnd, m_EventInfo.EventName.c_str());
		break;

	default:
		if (Command >= CEventHandler::COMMAND_FIRST) {
			m_pEventHandler->OnMenuSelected(Command);
		} else if (Command >= COMMAND_SEARCH) {
			String Keyword(CRichEditUtil::GetSelectedText(m_hwndEdit));
			if (!Keyword.empty()) {
				GetAppClass().KeywordSearch.Search(Command - COMMAND_SEARCH, Keyword.c_str());
			}
		}
		break;
	}
}




CEventInfoPopup::CEventHandler::~CEventHandler()
{
	if (m_pPopup != nullptr)
		m_pPopup->m_pEventHandler = nullptr;
}




CEventInfoPopupManager::CEventInfoPopupManager(CEventInfoPopup *pPopup)
	: m_pPopup(pPopup)
{
}


CEventInfoPopupManager::~CEventInfoPopupManager()
{
	Finalize();
}


bool CEventInfoPopupManager::Initialize(HWND hwnd, CEventHandler *pEventHandler)
{
	if (hwnd == nullptr)
		return false;
	if (!SetSubclass(hwnd))
		return false;
	m_pEventHandler = pEventHandler;
	if (m_pEventHandler != nullptr)
		m_pEventHandler->m_pPopup = m_pPopup;
	m_fTrackMouseEvent = false;
	return true;
}


void CEventInfoPopupManager::Finalize()
{
	if (m_hwnd != nullptr) {
		m_pPopup->Hide();
		RemoveSubclass();
	}
}


bool CEventInfoPopupManager::SetEnable(bool fEnable)
{
	m_fEnable = fEnable;
	if (!fEnable)
		m_pPopup->Hide();
	return true;
}


bool CEventInfoPopupManager::Popup(int x, int y)
{
	if (m_pEventHandler == nullptr)
		return false;
	m_HitTestParam = -1;
	if (m_pEventHandler->HitTest(x, y, &m_HitTestParam)) {
		if (m_pEventHandler->ShowPopup(m_HitTestParam, m_pPopup)) {
			return true;
		}
	}
	return false;
}


LRESULT CEventInfoPopupManager::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_MOUSEMOVE:
		if (!m_fTrackMouseEvent) {
			TRACKMOUSEEVENT tme;

			tme.cbSize = sizeof(tme);
			tme.dwFlags = TME_HOVER | TME_LEAVE;
			tme.hwndTrack = hwnd;
			tme.dwHoverTime = 1000;
			if (::TrackMouseEvent(&tme))
				m_fTrackMouseEvent = true;
		}
		if (m_pPopup->IsVisible() && m_pEventHandler != nullptr) {
			LPARAM Param;
			if (m_pEventHandler->HitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), &Param)) {
				if (Param != m_HitTestParam) {
					m_HitTestParam = Param;
					m_pEventHandler->ShowPopup(m_HitTestParam, m_pPopup);
				}
			} else {
				m_pPopup->Hide();
			}
		}
		break;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
	case WM_NCLBUTTONDOWN:
	case WM_NCRBUTTONDOWN:
	case WM_NCMBUTTONDOWN:
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
	case WM_VSCROLL:
	case WM_HSCROLL:
		m_pPopup->Hide();
		break;

	case WM_MOUSELEAVE:
		if (m_pPopup->IsVisible()) {
			POINT pt;
			::GetCursorPos(&pt);
			const HWND hwndCur = ::WindowFromPoint(pt);
			if (!m_pPopup->IsOwnWindow(hwndCur))
				m_pPopup->Hide();
		}
		m_fTrackMouseEvent = false;
		break;

	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE) {
			const HWND hwndActive = reinterpret_cast<HWND>(lParam);
			if (!m_pPopup->IsOwnWindow(hwndActive))
				m_pPopup->Hide();
		}
		break;

	case WM_MOUSEHOVER:
		if (m_pEventHandler != nullptr && m_fEnable
				&& ::GetActiveWindow() == ::GetForegroundWindow()) {
			bool fHit = false;
			m_HitTestParam = -1;
			if (m_pEventHandler->HitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), &m_HitTestParam)) {
				if (m_pEventHandler->ShowPopup(m_HitTestParam, m_pPopup)) {
					fHit = true;
				}
			}
			if (!fHit)
				m_pPopup->Hide();
		}
		m_fTrackMouseEvent = false;
		return 0;

	case WM_SHOWWINDOW:
		if (!wParam)
			m_pPopup->Hide();
		break;

	case WM_DESTROY:
		m_pPopup->Hide();
		break;
	}

	return CWindowSubclass::OnMessage(hwnd, uMsg, wParam, lParam);
}


}	// namespace TVTest
