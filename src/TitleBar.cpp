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
#include "TitleBar.h"
#include "DrawUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace
{

constexpr int NUM_BUTTONS = 4;

enum {
	ICON_MINIMIZE,
	ICON_MAXIMIZE,
	ICON_FULLSCREEN,
	ICON_CLOSE,
	ICON_RESTORE,
	ICON_FULLSCREENCLOSE
};

}




const LPCTSTR CTitleBar::CLASS_NAME = APP_NAME TEXT(" Title Bar");
HINSTANCE CTitleBar::m_hinst = nullptr;


bool CTitleBar::Initialize(HINSTANCE hinst)
{
	if (m_hinst == nullptr) {
		WNDCLASS wc;

		wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hinst;
		wc.hIcon = nullptr;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = nullptr;
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = CLASS_NAME;
		if (RegisterClass(&wc) == 0)
			return false;
		m_hinst = hinst;
	}
	return true;
}


CTitleBar::CTitleBar()
	: m_fSnapLayoutsSupport(Util::OS::IsWindows11OrLater())
{
	GetSystemFont(DrawUtil::FontType::Caption, &m_StyleFont);
}


CTitleBar::~CTitleBar()
{
	Destroy();
	if (m_pEventHandler != nullptr)
		m_pEventHandler->m_pTitleBar = nullptr;
}


bool CTitleBar::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	return CreateBasicWindow(hwndParent, Style, ExStyle, ID, CLASS_NAME, nullptr, m_hinst);
}


void CTitleBar::SetVisible(bool fVisible)
{
	m_HotItem = -1;
	CBasicWindow::SetVisible(fVisible);
}


void CTitleBar::SetStyle(const Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);
}


void CTitleBar::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	m_Style.NormalizeStyle(pStyleManager, pStyleScaling);
}


void CTitleBar::SetTheme(const Theme::CThemeManager *pThemeManager)
{
	TitleBarTheme Theme;

	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_TITLEBAR_CAPTION,
		&Theme.CaptionStyle);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_TITLEBAR_BUTTON,
		&Theme.IconStyle);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_TITLEBAR_BUTTON_HOT,
		&Theme.HighlightIconStyle);
	pThemeManager->GetBorderStyle(
		Theme::CThemeManager::STYLE_TITLEBAR,
		&Theme.Border);

	SetTitleBarTheme(Theme);
}


int CTitleBar::CalcHeight() const
{
	const int LabelHeight = m_FontHeight + m_Style.LabelMargin.Vert();
	const int IconHeight = m_Style.IconSize.Height + m_Style.IconMargin.Vert();
	const int ButtonHeight = GetButtonHeight();
	const int Height = std::max({LabelHeight, IconHeight, ButtonHeight});
	RECT Border;
	GetBorderWidthsInPixels(m_Theme.Border, &Border);

	return Height + m_Style.Padding.Vert() + Border.top + Border.bottom;
}


int CTitleBar::GetButtonWidth() const
{
	return m_Style.ButtonIconSize.Width + m_Style.ButtonPadding.Horz();
}


int CTitleBar::GetButtonHeight() const
{
	return m_Style.ButtonIconSize.Height + m_Style.ButtonPadding.Vert();
}


bool CTitleBar::SetLabel(LPCTSTR pszLabel)
{
	if (StringUtility::Compare(m_Label, pszLabel) != 0) {
		StringUtility::Assign(m_Label, pszLabel);
		if (m_hwnd != nullptr)
			UpdateItem(ITEM_LABEL);
	}
	return true;
}


void CTitleBar::SetMaximizeMode(bool fMaximize)
{
	if (m_fMaximized != fMaximize) {
		m_fMaximized = fMaximize;
		if (m_hwnd != nullptr)
			UpdateItem(ITEM_MAXIMIZE);
	}
}


void CTitleBar::SetFullscreenMode(bool fFullscreen)
{
	if (m_fFullscreen != fFullscreen) {
		m_fFullscreen = fFullscreen;
		if (m_hwnd != nullptr)
			UpdateItem(ITEM_FULLSCREEN);
	}
}


bool CTitleBar::SetEventHandler(CEventHandler *pHandler)
{
	if (m_pEventHandler != nullptr)
		m_pEventHandler->m_pTitleBar = nullptr;
	if (pHandler != nullptr)
		pHandler->m_pTitleBar = this;
	m_pEventHandler = pHandler;
	return true;
}


bool CTitleBar::SetTitleBarTheme(const TitleBarTheme &Theme)
{
	m_Theme = Theme;

	if (m_hwnd != nullptr)
		AdjustSize();

	return true;
}


bool CTitleBar::GetTitleBarTheme(TitleBarTheme *pTheme) const
{
	if (pTheme == nullptr)
		return false;
	*pTheme = m_Theme;
	return true;
}


bool CTitleBar::SetFont(const Style::Font &Font)
{
	m_StyleFont = Font;

	if (m_hwnd != nullptr)
		RealizeStyle();

	return true;
}


void CTitleBar::SetIcon(HICON hIcon)
{
	if (m_hIcon != hIcon) {
		m_hIcon = hIcon;
		if (m_hwnd != nullptr) {
			RECT rc;

			GetItemRect(ITEM_LABEL, &rc);
			Invalidate(&rc);
		}
	}
}


SIZE CTitleBar::GetIconDrawSize() const
{
	SIZE sz;

	sz.cx = m_Style.IconSize.Width;
	sz.cy = m_Style.IconSize.Height;
	return sz;
}


bool CTitleBar::IsIconDrawSmall() const
{
	return m_Style.IconSize.Width <= ::GetSystemMetrics(SM_CXSMICON)
		&& m_Style.IconSize.Height <= ::GetSystemMetrics(SM_CYSMICON);
}


LRESULT CTitleBar::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			InitializeUI();

			const CREATESTRUCT *pcs = reinterpret_cast<const CREATESTRUCT*>(lParam);
			RECT rc;

			rc.left = 0;
			rc.top = 0;
			rc.right = 0;
			rc.bottom = CalcHeight();
			::AdjustWindowRectEx(&rc, pcs->style, FALSE, pcs->dwExStyle);
			::MoveWindow(hwnd, 0, 0, 0, rc.bottom - rc.top, FALSE);

			m_Tooltip.Create(hwnd);
			m_Tooltip.SetFont(m_Font.GetHandle());
			for (int i = ITEM_BUTTON_FIRST; i <= ITEM_LAST; i++) {
				RECT rc;
				GetItemRect(i, &rc);
				m_Tooltip.AddTool(i, rc);
			}

			m_HotItem = -1;
			m_ClickItem = -1;

			m_MouseLeaveTrack.Initialize(hwnd);
		}
		return 0;

	case WM_SIZE:
		SetHotItem(-1);
		UpdateTooltipsRect();
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd, &ps);
			Draw(ps.hdc, ps.rcPaint);
			::EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			const int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
			int HotItem = HitTest(x, y);

			if (GetCapture() == hwnd) {
				if (HotItem != m_ClickItem)
					HotItem = -1;
				SetHotItem(HotItem);
			} else {
				SetHotItem(HotItem);
				m_MouseLeaveTrack.OnMouseMove();
			}
		}
		return 0;

	case WM_MOUSELEAVE:
		SetHotItem(-1);

		if (m_MouseLeaveTrack.OnMouseLeave()) {
			if (m_pEventHandler)
				m_pEventHandler->OnMouseLeave();
		}
		return 0;

	case WM_NCMOUSEMOVE:
		if (wParam == HTMAXBUTTON) {
			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			::ScreenToClient(hwnd, &pt);
			const int HotItem = HitTest(pt.x, pt.y);

			if (HotItem != m_HotItem) {
				SetHotItem(HotItem);
				if (m_HotItem > 0)
					::SetCursor(GetActionCursor());
			}
		}

		m_MouseLeaveTrack.OnNcMouseMove();
		return 0;

	case WM_NCMOUSELEAVE:
		SetHotItem(-1);

		if (m_MouseLeaveTrack.OnNcMouseLeave()) {
			if (m_pEventHandler)
				m_pEventHandler->OnMouseLeave();
		}
		return 0;

	case WM_LBUTTONDOWN:
		m_ClickItem = m_HotItem;
		if (m_ClickItem == ITEM_LABEL) {
			if (m_pEventHandler != nullptr) {
				const int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

				if (PtInIcon(x, y))
					m_pEventHandler->OnIconLButtonDown(x, y);
				else
					m_pEventHandler->OnLabelLButtonDown(x, y);
			}
		} else {
			::SetCapture(hwnd);
		}
		return 0;

	case WM_NCLBUTTONDOWN:
		if (wParam == HTMAXBUTTON && m_HotItem == ITEM_MAXIMIZE) {
			m_ClickItem = ITEM_MAXIMIZE;
			::SetCapture(hwnd);
			return 0;
		}
		break;

	case WM_RBUTTONUP:
		if (m_HotItem == ITEM_LABEL) {
			if (m_pEventHandler != nullptr)
				m_pEventHandler->OnLabelRButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		return 0;

	case WM_LBUTTONUP:
		if (GetCapture() == hwnd) {
			::ReleaseCapture();
			if (m_HotItem >= 0) {
				if (m_pEventHandler != nullptr) {
					switch (m_HotItem) {
					case ITEM_MINIMIZE:
						m_pEventHandler->OnMinimize();
						break;
					case ITEM_MAXIMIZE:
						m_pEventHandler->OnMaximize();
						break;
					case ITEM_FULLSCREEN:
						m_pEventHandler->OnFullscreen();
						break;
					case ITEM_CLOSE:
						m_pEventHandler->OnClose();
						break;
					}
				}
			}
		}
		return 0;

	case WM_LBUTTONDBLCLK:
		{
			const int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

			if (m_HotItem < 0 && HitTest(x, y) == ITEM_LABEL)
				m_HotItem = ITEM_LABEL;
			if (m_HotItem == ITEM_LABEL) {
				if (m_pEventHandler != nullptr) {
					if (PtInIcon(x, y))
						m_pEventHandler->OnIconLButtonDoubleClick(x, y);
					else
						m_pEventHandler->OnLabelLButtonDoubleClick(x, y);
				}
			}
		}
		return 0;

	case WM_SETCURSOR:
		{
			const int HitTestCode = LOWORD(lParam);

			if (HitTestCode == HTCLIENT || HitTestCode == HTMAXBUTTON) {
				if (m_HotItem > 0) {
					::SetCursor(GetActionCursor());
					return TRUE;
				}
			}
		}
		break;

	case WM_NCHITTEST:
		/*
			Windows 11 のスナップレイアウトを表示させるには、最大化ボタンの位置で HTMAXBUTTON を返す必要がある。
			https://docs.microsoft.com/ja-jp/windows/apps/desktop/modernize/apply-snap-layout-menu
		*/
		if (m_fSnapLayoutsSupport && !m_fFullscreen && ::GetCapture() != hwnd) {
			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			RECT rc;

			::ScreenToClient(hwnd, &pt);
			::GetClientRect(hwnd, &rc);
			if (::PtInRect(&rc, pt)) {
				const int Item = HitTest(pt.x, pt.y);

#if 0
				switch (Item) {
				case ITEM_LABEL:
					if (PtInIcon(pt.x, pt.y))
						return HTSYSMENU;
					return HTCAPTION;

				case ITEM_MINIMIZE:
					return HTMINBUTTON;

				case ITEM_MAXIMIZE:
					return HTMAXBUTTON;

				case ITEM_CLOSE:
					return HTCLOSE;
				}
#else
				if (Item == ITEM_MAXIMIZE)
					return HTMAXBUTTON;
#endif

				break;
			}
		}
		break;

	case WM_NOTIFY:
		switch (reinterpret_cast<NMHDR*>(lParam)->code) {
		case TTN_NEEDTEXT:
			{
				static const LPCTSTR pszToolTip[] = {
					TEXT("最小化"),
					TEXT("最大化"),
					TEXT("全画面表示"),
					TEXT("閉じる"),
				};
				LPNMTTDISPINFO pnmttdi = reinterpret_cast<LPNMTTDISPINFO>(lParam);

				if (m_fMaximized && pnmttdi->hdr.idFrom == ITEM_MAXIMIZE)
					StringCopy(pnmttdi->szText, TEXT("元のサイズに戻す"));
				else if (m_fFullscreen && pnmttdi->hdr.idFrom == ITEM_FULLSCREEN)
					StringCopy(pnmttdi->szText, TEXT("全画面表示解除"));
				else
					StringCopy(pnmttdi->szText, pszToolTip[pnmttdi->hdr.idFrom - ITEM_BUTTON_FIRST]);
				pnmttdi->lpszText = pnmttdi->szText;
				pnmttdi->hinst = nullptr;
			}
			return 0;
		}
		break;

	case WM_DESTROY:
		m_Tooltip.Destroy();
		return 0;
	}

	return CCustomWindow::OnMessage(hwnd, uMsg, wParam, lParam);
}


void CTitleBar::AdjustSize()
{
	RECT rc;
	GetPosition(&rc);
	const int NewHeight = CalcHeight();
	if (NewHeight != rc.bottom - rc.top) {
		rc.bottom = rc.top + NewHeight;
		SetPosition(&rc);
		if (m_pEventHandler != nullptr)
			m_pEventHandler->OnHeightChanged(NewHeight);
	} else {
		SendSizeMessage();
	}
	Invalidate();
}


int CTitleBar::CalcFontHeight() const
{
	return Style::GetFontHeight(m_hwnd, m_Font.GetHandle(), m_Style.LabelExtraHeight);
}


bool CTitleBar::GetItemRect(int Item, RECT *pRect) const
{
	if (m_hwnd == nullptr || Item < 0 || Item > ITEM_LAST)
		return false;

	RECT rc;

	GetClientRect(&rc);
	Theme::BorderStyle Border = m_Theme.Border;
	ConvertBorderWidthsInPixels(&Border);
	Theme::SubtractBorderRect(Border, &rc);
	Style::Subtract(&rc, m_Style.Padding);
	const int ButtonWidth = GetButtonWidth();
	int ButtonPos = rc.right - NUM_BUTTONS * ButtonWidth;
	if (ButtonPos < 0)
		ButtonPos = 0;
	if (Item == ITEM_LABEL) {
		rc.right = ButtonPos;
		if (rc.right < rc.left)
			rc.right = rc.left;
	} else {
		const int ButtonHeight = GetButtonHeight();
		rc.left = ButtonPos + (Item - 1) * ButtonWidth;
		rc.right = rc.left + ButtonWidth;
		rc.top += ((rc.bottom - rc.top) - ButtonHeight) / 2;
		rc.bottom = rc.top + ButtonHeight;
	}
	*pRect = rc;
	return true;
}


bool CTitleBar::UpdateItem(int Item)
{
	RECT rc;

	if (m_hwnd == nullptr)
		return false;
	if (!GetItemRect(Item, &rc))
		return false;
	Invalidate(&rc, false);
	return true;
}


int CTitleBar::HitTest(int x, int y) const
{
	const POINT pt = {x, y};
	int i;

	for (i = ITEM_LAST; i >= 0; i--) {
		RECT rc;
		GetItemRect(i, &rc);
		if (::PtInRect(&rc, pt))
			break;
	}
	return i;
}


bool CTitleBar::PtInIcon(int x, int y) const
{
	RECT Border;
	GetBorderWidthsInPixels(m_Theme.Border, &Border);
	const int IconLeft = Border.left + m_Style.Padding.Left + m_Style.IconMargin.Left;
	if (x >= IconLeft && x < IconLeft + m_Style.IconSize.Width)
		return true;
	return false;
}


void CTitleBar::UpdateTooltipsRect()
{
	for (int i = ITEM_BUTTON_FIRST; i <= ITEM_LAST; i++) {
		RECT rc;
		GetItemRect(i, &rc);
		m_Tooltip.SetToolRect(i, rc);
	}
}


void CTitleBar::Draw(HDC hdc, const RECT &PaintRect)
{
	const HFONT hfontOld = DrawUtil::SelectObject(hdc, m_Font);
	const int OldBkMode = ::SetBkMode(hdc, TRANSPARENT);
	const COLORREF crOldTextColor = ::GetTextColor(hdc);
	const COLORREF crOldBkColor = ::GetBkColor(hdc);

	Theme::CThemeDraw ThemeDraw(BeginThemeDraw(hdc));

	RECT rc;
	GetClientRect(&rc);
	Theme::BorderStyle Border = m_Theme.Border;
	ConvertBorderWidthsInPixels(&Border);
	Theme::SubtractBorderRect(Border, &rc);
	if (rc.left < PaintRect.left)
		rc.left = PaintRect.left;
	if (rc.right > PaintRect.right)
		rc.right = PaintRect.right;
	ThemeDraw.Draw(m_Theme.CaptionStyle.Back, rc);

	for (int i = 0; i <= ITEM_LAST; i++) {
		GetItemRect(i, &rc);
		if (rc.right > rc.left
				&& rc.left < PaintRect.right && rc.right > PaintRect.left
				&& rc.top < PaintRect.bottom && rc.bottom > PaintRect.top) {
			const bool fHighlight = i == m_HotItem && i != ITEM_LABEL;

			if (i == ITEM_LABEL) {
				if (m_hIcon != nullptr) {
					const int Height = m_Style.IconSize.Height + m_Style.IconMargin.Vert();
					rc.left += m_Style.IconMargin.Left;
					::DrawIconEx(
						hdc,
						rc.left,
						rc.top + m_Style.IconMargin.Top + ((rc.bottom - rc.top) - Height) / 2,
						m_hIcon,
						m_Style.IconSize.Width, m_Style.IconSize.Height,
						0, nullptr, DI_NORMAL);
					rc.left += m_Style.IconSize.Width;
				}
				if (!m_Label.empty()) {
					Style::Subtract(&rc, m_Style.LabelMargin);
					ThemeDraw.Draw(
						m_Theme.CaptionStyle.Fore, rc, m_Label.c_str(),
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
				}
			} else {
				const Theme::Style &Style =
					fHighlight ? m_Theme.HighlightIconStyle : m_Theme.IconStyle;

				// とりあえず変にならないようにする。
				// 背景を透過指定できるようにした方が良い。
				if (Style.Back.Border.Type != Theme::BorderType::None
						|| Style.Back.Fill != m_Theme.CaptionStyle.Back.Fill)
					ThemeDraw.Draw(Style.Back, rc);
				m_ButtonIcons.Draw(
					hdc,
					rc.left + m_Style.ButtonPadding.Left,
					rc.top + m_Style.ButtonPadding.Top,
					m_Style.ButtonIconSize.Width,
					m_Style.ButtonIconSize.Height,
					(i == ITEM_MAXIMIZE && m_fMaximized) ? ICON_RESTORE :
					((i == ITEM_FULLSCREEN && m_fFullscreen) ? ICON_FULLSCREENCLOSE : i - 1),
					Style.Fore.Fill.GetSolidColor());
			}
		}
	}

	GetClientRect(&rc);
	ThemeDraw.Draw(m_Theme.Border, rc);

	::SetBkColor(hdc, crOldBkColor);
	::SetTextColor(hdc, crOldTextColor);
	::SetBkMode(hdc, OldBkMode);
	::SelectObject(hdc, hfontOld);
}


void CTitleBar::SetHotItem(int Item)
{
	if (Item != m_HotItem) {
		const int OldHotItem = m_HotItem;
		m_HotItem = Item;
		if (OldHotItem >= 0)
			UpdateItem(OldHotItem);
		if (m_HotItem >= 0)
			UpdateItem(m_HotItem);
	}
}


void CTitleBar::ApplyStyle()
{
	if (m_hwnd == nullptr)
		return;

	CreateDrawFont(m_StyleFont, &m_Font);
	m_FontHeight = CalcFontHeight();

	if (m_Tooltip.IsCreated())
		m_Tooltip.SetFont(m_Font.GetHandle());

	static const Theme::IconList::ResourceInfo ResourceList[] = {
		{MAKEINTRESOURCE(IDB_TITLEBAR12), 12, 12},
		{MAKEINTRESOURCE(IDB_TITLEBAR24), 24, 24},
	};
	m_ButtonIcons.Load(
		GetAppClass().GetResourceInstance(),
		m_Style.ButtonIconSize.Width, m_Style.ButtonIconSize.Height,
		ResourceList, lengthof(ResourceList));
}


void CTitleBar::RealizeStyle()
{
	if (m_hwnd != nullptr) {
		AdjustSize();
	}
}




CTitleBar::CEventHandler::~CEventHandler()
{
	if (m_pTitleBar != nullptr)
		m_pTitleBar->SetEventHandler(nullptr);
}




void CTitleBar::TitleBarStyle::SetStyle(const Style::CStyleManager *pStyleManager)
{
	*this = TitleBarStyle();
	pStyleManager->Get(TEXT("title-bar.padding"), &Padding);
	pStyleManager->Get(TEXT("title-bar.label.margin"), &LabelMargin);
	pStyleManager->Get(TEXT("title-bar.label.extra-height"), &LabelExtraHeight);
	pStyleManager->Get(TEXT("title-bar.icon"), &IconSize);
	pStyleManager->Get(TEXT("title-bar.icon.margin"), &IconMargin);
	pStyleManager->Get(TEXT("title-bar.button.icon"), &ButtonIconSize);
	pStyleManager->Get(TEXT("title-bar.button.padding"), &ButtonPadding);
}


void CTitleBar::TitleBarStyle::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	pStyleScaling->ToPixels(&Padding);
	pStyleScaling->ToPixels(&LabelMargin);
	pStyleScaling->ToPixels(&LabelExtraHeight);
	pStyleScaling->ToPixels(&IconSize);
	pStyleScaling->ToPixels(&IconMargin);
	pStyleScaling->ToPixels(&ButtonIconSize);
	pStyleScaling->ToPixels(&ButtonPadding);
}


}	// namespace TVTest
