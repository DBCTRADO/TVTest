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
#include "ControlPanel.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


const LPCTSTR CControlPanel::m_pszClassName = APP_NAME TEXT(" Control Panel");
HINSTANCE CControlPanel::m_hinst = nullptr;


bool CControlPanel::Initialize(HINSTANCE hinst)
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
		wc.lpszClassName = m_pszClassName;
		if (::RegisterClass(&wc) == 0)
			return false;
		m_hinst = hinst;
	}
	return true;
}


CControlPanel::~CControlPanel()
{
	Destroy();
}


bool CControlPanel::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	return CreateBasicWindow(
		hwndParent, Style, ExStyle, ID,
		m_pszClassName, TEXT("操作"), m_hinst);
}


void CControlPanel::SetStyle(const Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);

	for (auto &e : m_ItemList)
		e->SetStyle(pStyleManager);
}


void CControlPanel::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	m_Style.NormalizeStyle(pStyleManager, pStyleScaling);

	for (auto &e : m_ItemList)
		e->NormalizeStyle(pStyleManager, pStyleScaling);
}


void CControlPanel::SetTheme(const Theme::CThemeManager *pThemeManager)
{
	ControlPanelTheme Theme;

	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_CONTROLPANEL_ITEM,
		&Theme.ItemStyle);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_CONTROLPANEL_ITEM_HOT,
		&Theme.OverItemStyle);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_CONTROLPANEL_ITEM_CHECKED,
		&Theme.CheckedItemStyle);
	Theme.MarginColor =
		pThemeManager->GetColor(CColorScheme::COLOR_CONTROLPANELMARGIN);

	SetControlPanelTheme(Theme);
}


bool CControlPanel::SetFont(const Style::Font &Font)
{
	m_StyleFont = Font;
	if (m_hwnd != nullptr) {
		ApplyStyle();
		RealizeStyle();
	}
	return true;
}


bool CControlPanel::AddItem(CControlPanelItem *pItem)
{
	if (pItem == nullptr)
		return false;
	m_ItemList.emplace_back(pItem);
	pItem->m_pControlPanel = this;
	RegisterUIChild(pItem);
	return true;
}


CControlPanelItem *CControlPanel::GetItem(int Index) const
{
	if (Index < 0 || Index >= static_cast<int>(m_ItemList.size()))
		return nullptr;
	return m_ItemList[Index].get();
}


bool CControlPanel::UpdateItem(int Index)
{
	if (Index < 0 || Index >= static_cast<int>(m_ItemList.size()) || m_hwnd == nullptr)
		return false;
	RECT rc;
	m_ItemList[Index]->GetPosition(&rc);
	Invalidate(&rc);
	return true;
}


bool CControlPanel::GetItemPosition(int Index, RECT *pRect) const
{
	if (Index < 0 || Index >= static_cast<int>(m_ItemList.size()))
		return false;
	m_ItemList[Index]->GetPosition(pRect);
	return true;
}


void CControlPanel::UpdateLayout()
{
	RECT rc;
	GetClientRect(&rc);
	const int Width = (rc.right - rc.left) - (m_Style.Padding.Left + m_Style.Padding.Right);
	int x = m_Style.Padding.Left;
	int y = m_Style.Padding.Top;
	int MaxHeight = 0;

	for (size_t i = 0; i < m_ItemList.size(); i++) {
		CControlPanelItem *pItem = m_ItemList[i].get();

		if (!pItem->GetVisible())
			continue;
		if (pItem->GetBreak()) {
			x = m_Style.Padding.Left;
			if (i > 0)
				y += MaxHeight;
			MaxHeight = 0;
		}
		SIZE sz;
		pItem->CalcSize(Width, &sz);
		rc.left = x;
		rc.top = y;
		rc.right = x + sz.cx;
		rc.bottom = y + sz.cy;
		pItem->m_Position = rc;
		if (sz.cy > MaxHeight)
			MaxHeight = sz.cy;
		x += sz.cx;
	}
}


bool CControlPanel::SetControlPanelTheme(const ControlPanelTheme &Theme)
{
	m_Theme = Theme;
	if (m_hwnd != nullptr)
		Invalidate();
	return true;
}


bool CControlPanel::GetControlPanelTheme(ControlPanelTheme *pTheme) const
{
	if (pTheme == nullptr)
		return false;
	*pTheme = m_Theme;
	return true;
}


void CControlPanel::SetSendMessageWindow(HWND hwnd)
{
	m_hwndMessage = hwnd;
}


void CControlPanel::SendCommand(int Command)
{
	if (m_hwndMessage != nullptr)
		::SendMessage(m_hwndMessage, WM_COMMAND, Command, reinterpret_cast<LPARAM>(GetHandle()));
}


bool CControlPanel::CheckRadioItem(int FirstID, int LastID, int CheckID)
{
	for (auto &Item : m_ItemList) {
		if (Item->m_Command >= FirstID && Item->m_Command <= LastID)
			Item->m_fCheck = Item->m_Command == CheckID;
	}
	if (m_hwnd != nullptr)
		Invalidate();
	return true;
}


const Style::Margins &CControlPanel::GetItemPadding() const
{
	return m_Style.ItemPadding;
}


const Style::Size &CControlPanel::GetIconSize() const
{
	return m_Style.IconSize;
}


bool CControlPanel::CalcTextSize(LPCTSTR pszText, SIZE *pSize)
{
	pSize->cx = 0;
	pSize->cy = 0;

	if (m_hwnd == nullptr || pszText == nullptr)
		return false;

	const HDC hdc = ::GetDC(m_hwnd);
	if (hdc == nullptr)
		return false;
	const HFONT hfontOld = DrawUtil::SelectObject(hdc, m_Font);
	RECT rc = {0, 0, 0, 0};
	::DrawText(hdc, pszText, -1, &rc, DT_NOPREFIX | DT_CALCRECT);
	pSize->cx = rc.right - rc.left;
	pSize->cy = rc.bottom - rc.top;
	SelectFont(hdc, hfontOld);
	::ReleaseDC(m_hwnd, hdc);
	return true;
}


int CControlPanel::CalcFontHeight() const
{
	return Style::GetFontHeight(
		m_hwnd, m_Font.GetHandle(), m_Style.TextExtraHeight);
}


int CControlPanel::GetTextItemHeight() const
{
	return m_FontHeight + m_Style.ItemPadding.Vert();
}


void CControlPanel::Draw(HDC hdc, const RECT &PaintRect)
{
	RECT rcClient;
	GetClientRect(&rcClient);
	const int Width = (rcClient.right - rcClient.left) - (m_Style.Padding.Left + m_Style.Padding.Right);
	int MaxHeight = 0;

	for (const auto &Item : m_ItemList) {
		if (Item->GetVisible()) {
			const int Height = Item->m_Position.bottom - Item->m_Position.top;
			if (Height > MaxHeight)
				MaxHeight = Height;
		}
	}
	if (MaxHeight == 0)
		return;
	if (!m_Offscreen.IsCreated()
			|| m_Offscreen.GetWidth() < Width
			|| m_Offscreen.GetHeight() < MaxHeight) {
		if (!m_Offscreen.Create(Width + 32, MaxHeight, hdc))
			return;
	}

	const HBRUSH hbrMargin = ::CreateSolidBrush(m_Theme.MarginColor);
	const HDC hdcOffscreen = m_Offscreen.GetDC();
	RECT rc, rcOffscreen, rcDest;

	rc = rcClient;
	rc.left += m_Style.Padding.Left;
	rc.top += m_Style.Padding.Top;
	rc.right -= m_Style.Padding.Right;
	rc.bottom -= m_Style.Padding.Bottom;
	DrawUtil::FillBorder(hdc, &rcClient, &rc, &PaintRect, hbrMargin);
	const HFONT hfontOld = DrawUtil::SelectObject(hdcOffscreen, m_Font);
	const COLORREF crOldTextColor = ::GetTextColor(hdcOffscreen);
	const int OldBkMode = ::SetBkMode(hdcOffscreen, TRANSPARENT);
	::SetRect(&rcOffscreen, 0, 0, Width, MaxHeight);
	::SetRect(&rcDest, m_Style.Padding.Left, -1, m_Style.Padding.Left + Width, 0);

	Theme::CThemeDraw ThemeDraw(BeginThemeDraw(hdcOffscreen));

	for (int i = 0; i < static_cast<int>(m_ItemList.size()); i++) {
		CControlPanelItem *pItem = m_ItemList[i].get();

		if (!pItem->GetVisible())
			continue;
		pItem->GetPosition(&rc);
		if (rc.left < PaintRect.right && rc.right > PaintRect.left
				&& rc.top < PaintRect.bottom && rc.bottom > PaintRect.top) {
			COLORREF crText, crBack;

			if (rc.top != rcDest.top) {
				if (rcDest.top >= 0)
					m_Offscreen.CopyTo(hdc, &rcDest);
				::FillRect(hdcOffscreen, &rcOffscreen, hbrMargin);
				rcDest.top = rc.top;
			}
			if (rcDest.bottom < rc.bottom)
				rcDest.bottom = rc.bottom;
			::OffsetRect(&rc, -m_Style.Padding.Left, -rc.top);
			if (i == m_HotItem) {
				crText = m_Theme.OverItemStyle.Fore.Fill.GetSolidColor();
				crBack = m_Theme.OverItemStyle.Back.Fill.GetSolidColor();
				ThemeDraw.Draw(m_Theme.OverItemStyle.Back, rc);
			} else {
				const Theme::Style Style =
					pItem->GetCheck() ? m_Theme.CheckedItemStyle : m_Theme.ItemStyle;

				crText = Style.Fore.Fill.GetSolidColor();
				crBack = Style.Back.Fill.GetSolidColor();
				if (!pItem->GetEnable())
					crText = MixColor(crText, crBack);
				ThemeDraw.Draw(Style.Back, rc);
			}
			::SetTextColor(hdcOffscreen, crText);
			::SetBkColor(hdcOffscreen, crBack);
			pItem->Draw(hdcOffscreen, rc);
		}
	}
	if (rcDest.top >= 0) {
		m_Offscreen.CopyTo(hdc, &rcDest);
		rc.top = rcDest.bottom;
	} else {
		rc.top = m_Style.Padding.Top;
	}
	if (rc.top < PaintRect.bottom) {
		rc.left = PaintRect.left;
		rc.right = PaintRect.right;
		rc.bottom = PaintRect.bottom;
		::FillRect(hdc, &rc, hbrMargin);
	}
	::SetBkMode(hdc, OldBkMode);
	::SetTextColor(hdc, crOldTextColor);
	SelectFont(hdc, hfontOld);
	::DeleteObject(hbrMargin);
}


LRESULT CControlPanel::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		InitializeUI();

		m_HotItem = -1;
		m_fTrackMouseEvent = false;
		m_fOnButtonDown = false;
		return 0;

	case WM_SIZE:
		UpdateLayout();
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
			int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

			if (::GetCapture() == hwnd) {
				RECT rc;
				m_ItemList[m_HotItem]->GetPosition(&rc);
				x -= rc.left;
				y -= rc.top;
				m_ItemList[m_HotItem]->OnMouseMove(x, y);
			} else {
				const POINT pt = {x, y};
				int i;

				for (i = static_cast<int>(m_ItemList.size()) - 1; i >= 0; i--) {
					const CControlPanelItem *pItem = m_ItemList[i].get();

					if (pItem->GetVisible() && pItem->GetEnable()) {
						RECT rc;
						pItem->GetPosition(&rc);
						if (::PtInRect(&rc, pt))
							break;
					}
				}
				if (i != m_HotItem) {
					const int OldHotItem = m_HotItem;
					m_HotItem = i;
					if (OldHotItem >= 0)
						UpdateItem(OldHotItem);
					if (m_HotItem >= 0)
						UpdateItem(m_HotItem);
				}
				if (!m_fTrackMouseEvent) {
					TRACKMOUSEEVENT tme;

					tme.cbSize = sizeof(TRACKMOUSEEVENT);
					tme.dwFlags = TME_LEAVE;
					tme.hwndTrack = hwnd;
					if (::TrackMouseEvent(&tme))
						m_fTrackMouseEvent = true;
				}
			}
		}
		return 0;

	case WM_MOUSELEAVE:
		if (!m_fOnButtonDown) {
			if (m_HotItem >= 0) {
				const int i = m_HotItem;

				m_HotItem = -1;
				UpdateItem(i);
			}
		}
		m_fTrackMouseEvent = false;
		return 0;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		{
			int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

			if (m_HotItem >= 0) {
				RECT rc;

				m_ItemList[m_HotItem]->GetPosition(&rc);
				x -= rc.left;
				y -= rc.top;
				m_fOnButtonDown = true;
				if (uMsg == WM_LBUTTONDOWN)
					m_ItemList[m_HotItem]->OnLButtonDown(x, y);
				else
					m_ItemList[m_HotItem]->OnRButtonDown(x, y);
				m_fOnButtonDown = false;
				if (!m_fTrackMouseEvent) {
					POINT pt;

					::GetCursorPos(&pt);
					::ScreenToClient(hwnd, &pt);
					::GetClientRect(hwnd, &rc);
					if (::PtInRect(&rc, pt)) {
						::SendMessage(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(pt.x, pt.y));
					} else {
						::SendMessage(hwnd, WM_MOUSELEAVE, 0, 0);
					}
				}
			}
		}
		return 0;

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		{
			int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

			if (m_HotItem >= 0) {
				RECT rc;

				m_ItemList[m_HotItem]->GetPosition(&rc);
				x -= rc.left;
				y -= rc.top;
				if (uMsg == WM_LBUTTONUP)
					m_ItemList[m_HotItem]->OnLButtonUp(x, y);
				else
					m_ItemList[m_HotItem]->OnRButtonUp(x, y);
			} else if (uMsg == WM_RBUTTONUP) {
				POINT pt = {x, y};
				::MapWindowPoints(hwnd, GetParent(), &pt, 1);
				::SendMessage(GetParent(), uMsg, wParam, MAKELPARAM(pt.x, pt.y));
			}

			if (GetCapture() == hwnd)
				::ReleaseCapture();
		}
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT) {
			if (m_HotItem >= 0) {
				::SetCursor(GetActionCursor());
				return TRUE;
			}
		}
		break;

	case WM_DISPLAYCHANGE:
		m_Offscreen.Destroy();
		return 0;
	}

	return CCustomWindow::OnMessage(hwnd, uMsg, wParam, lParam);
}


void CControlPanel::ApplyStyle()
{
	if (m_hwnd != nullptr) {
		CreateDrawFont(m_StyleFont, &m_Font);
		m_FontHeight = CalcFontHeight();
	}
}


void CControlPanel::RealizeStyle()
{
	if (m_hwnd != nullptr) {
		UpdateLayout();
		Invalidate();
	}
}




void CControlPanel::ControlPanelStyle::SetStyle(const Style::CStyleManager *pStyleManager)
{
	*this = ControlPanelStyle();
	pStyleManager->Get(TEXT("control-panel.padding"), &Padding);
	pStyleManager->Get(TEXT("control-panel.item.padding"), &ItemPadding);
	pStyleManager->Get(TEXT("control-panel.item.text.extra-height"), &TextExtraHeight);
	pStyleManager->Get(TEXT("control-panel.item.icon"), &IconSize);
}


void CControlPanel::ControlPanelStyle::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	pStyleScaling->ToPixels(&Padding);
	pStyleScaling->ToPixels(&ItemPadding);
	pStyleScaling->ToPixels(&TextExtraHeight);
	pStyleScaling->ToPixels(&IconSize);
}




void CControlPanelItem::GetPosition(int *pLeft, int *pTop, int *pWidth, int *pHeight) const
{
	if (pLeft)
		*pLeft = m_Position.left;
	if (pTop)
		*pTop = m_Position.top;
	if (pWidth)
		*pWidth = m_Position.right - m_Position.left;
	if (pHeight)
		*pHeight = m_Position.bottom - m_Position.top;
}


bool CControlPanelItem::SetPosition(int Left, int Top, int Width, int Height)
{
	if (Width < 0 || Height < 0)
		return false;
	m_Position.left = Left;
	m_Position.top = Top;
	m_Position.right = Left + Width;
	m_Position.bottom = Top + Height;
	return true;
}


void CControlPanelItem::GetPosition(RECT *pRect) const
{
	*pRect = m_Position;
}


void CControlPanelItem::SetVisible(bool fVisible)
{
	m_fVisible = fVisible;
}


void CControlPanelItem::SetEnable(bool fEnable)
{
	m_fEnable = fEnable;
}


void CControlPanelItem::SetCheck(bool fCheck)
{
	m_fCheck = fCheck;
}


void CControlPanelItem::SetBreak(bool fBreak)
{
	m_fBreak = fBreak;
}


void CControlPanelItem::CalcSize(int Width, SIZE *pSize)
{
	pSize->cx = Width;
	pSize->cy = m_Position.bottom - m_Position.top;
}


void CControlPanelItem::OnLButtonDown(int x, int y)
{
	m_pControlPanel->SendCommand(m_Command);
}


bool CControlPanelItem::CalcTextSize(LPCTSTR pszText, SIZE *pSize) const
{
	if (m_pControlPanel == nullptr || pszText == nullptr) {
		pSize->cx = 0;
		pSize->cy = 0;
		return false;
	}
	return m_pControlPanel->CalcTextSize(pszText, pSize);
}


int CControlPanelItem::GetTextItemHeight() const
{
	if (m_pControlPanel == nullptr)
		return 0;
	return m_pControlPanel->GetTextItemHeight();
}


void CControlPanelItem::GetMenuPos(POINT *pPos) const
{
	if (m_pControlPanel == nullptr) {
		::GetCursorPos(pPos);
		return;
	}
	pPos->x = m_Position.left;
	pPos->y = m_Position.bottom;
	::ClientToScreen(m_pControlPanel->GetHandle(), pPos);
}


} // namespace TVTest
