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
#include "StatusView.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CStatusItem::CStatusItem(int ID, const SizeValue &DefaultWidth)
	: m_ID(ID)
	, m_DefaultWidth(DefaultWidth)
{
}


int CStatusItem::GetIndex() const
{
	if (m_pStatus != nullptr) {
		for (int i = 0; i < m_pStatus->NumItems(); i++) {
			if (m_pStatus->GetItem(i) == this)
				return i;
		}
	}
	return -1;
}


bool CStatusItem::GetRect(RECT *pRect) const
{
	if (m_pStatus == nullptr)
		return false;
	return m_pStatus->GetItemRect(m_ID, pRect);
}


bool CStatusItem::GetClientRect(RECT *pRect) const
{
	if (m_pStatus == nullptr)
		return false;
	return m_pStatus->GetItemClientRect(m_ID, pRect);
}


bool CStatusItem::SetWidth(int Width)
{
	if (Width < 0)
		return false;
	if (Width < m_MinWidth)
		m_Width = m_MinWidth;
	else if (m_MaxWidth > 0 && Width < m_MaxWidth)
		m_Width = m_MaxWidth;
	else
		m_Width = Width;
	return true;
}


bool CStatusItem::SetActualWidth(int Width)
{
	if (Width < 0)
		return false;
	const int OldWidth = m_ActualWidth;
	if (Width < m_MinWidth)
		m_ActualWidth = m_MinWidth;
	else if (m_MaxWidth > 0 && Width < m_MaxWidth)
		m_ActualWidth = m_MaxWidth;
	else
		m_ActualWidth = Width;
	if (OldWidth >= 0 && OldWidth != m_ActualWidth)
		OnSizeChanged();
	return true;
}


void CStatusItem::SetVisible(bool fVisible)
{
	if (m_fVisible != fVisible) {
		m_fVisible = fVisible;
		OnVisibilityChanged();
	}
	OnPresentStatusChange(fVisible && m_pStatus != nullptr && m_pStatus->GetVisible());
}


void CStatusItem::SetItemStyle(StyleFlag Style)
{
	m_Style = Style;
}


void CStatusItem::SetItemStyle(StyleFlag Mask, StyleFlag Style)
{
	m_Style = (m_Style & ~Mask) | (Style & Mask);
}


bool CStatusItem::Update()
{
	if (m_pStatus == nullptr)
		return false;
	m_pStatus->UpdateItem(m_ID);
	return true;
}


void CStatusItem::Redraw()
{
	if (m_pStatus != nullptr)
		m_pStatus->RedrawItem(m_ID);
}


bool CStatusItem::GetMenuPos(POINT *pPos, UINT *pFlags, RECT *pExcludeRect)
{
	if (m_pStatus == nullptr)
		return false;

	RECT rc;

	if (!GetRect(&rc))
		return false;

	MapWindowRect(m_pStatus->GetHandle(), nullptr, &rc);

	if (pFlags != nullptr)
		*pFlags = 0;

	if (pPos != nullptr) {
		pPos->x = rc.left;
		pPos->y = rc.bottom;
	}

	if (pExcludeRect != nullptr) {
		*pExcludeRect = rc;
		*pFlags |= TPM_VERTICAL;
	}

	return true;
}


void CStatusItem::DrawText(HDC hdc, const RECT &Rect, LPCTSTR pszText, DrawTextFlag Flags) const
{
	DWORD DTFlags = DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX;
	if (!!(Flags & DrawTextFlag::HorizontalCenter))
		DTFlags |= DT_CENTER;
	if (!(Flags & DrawTextFlag::NoEndEllipsis))
		DTFlags |= DT_END_ELLIPSIS;
	RECT rc = Rect;
	::DrawText(hdc, pszText, -1, &rc, DTFlags);
}


void CStatusItem::DrawIcon(
	HDC hdc, const RECT &Rect, DrawUtil::CMonoColorIconList &IconList,
	int IconIndex, bool fEnabled) const
{
	if (hdc == nullptr)
		return;

	const Style::Size IconSize = m_pStatus->GetIconSize();
	const COLORREF cr = ::GetTextColor(hdc);
#if 0
	if (!fEnabled)
		cr = MixColor(cr, ::GetBkColor(hdc));
#endif
	IconList.Draw(
		hdc,
		Rect.left + ((Rect.right - Rect.left) - IconSize.Width) / 2,
		Rect.top + ((Rect.bottom - Rect.top) - IconSize.Height) / 2,
		IconSize.Width, IconSize.Height,
		IconIndex, cr, fEnabled ? 255 : 128);
}




CIconStatusItem::CIconStatusItem(int ID, int DefaultWidth)
	: CStatusItem(ID, SizeValue(DefaultWidth, SizeUnit::Pixel))
{
	m_MinWidth = DefaultWidth;
}


void CIconStatusItem::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	m_MinWidth = m_pStatus->GetIconSize().Width;
	if (m_Width < m_MinWidth)
		m_Width = m_MinWidth;
}




CStatusView::CEventHandler::~CEventHandler()
{
	if (m_pStatusView != nullptr)
		m_pStatusView->SetEventHandler(nullptr);
}




const LPCTSTR CStatusView::CLASS_NAME = APP_NAME TEXT(" Status");
HINSTANCE CStatusView::m_hinst = nullptr;


bool CStatusView::Initialize(HINSTANCE hinst)
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
		wc.hbrBackground = nullptr;
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = CLASS_NAME;
		if (::RegisterClass(&wc) == 0)
			return false;
		m_hinst = hinst;
	}
	return true;
}


CStatusView::CStatusView()
{
	GetDefaultFont(&m_Font);
}


CStatusView::~CStatusView()
{
	Destroy();

	if (m_pEventHandler != nullptr)
		m_pEventHandler->m_pStatusView = nullptr;
}


bool CStatusView::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	return CreateBasicWindow(hwndParent, Style, ExStyle, ID, CLASS_NAME, nullptr, m_hinst);
}


void CStatusView::SetStyle(const Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);

	for (auto &e : m_ItemList)
		e->SetStyle(pStyleManager);
}


void CStatusView::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	m_Style.NormalizeStyle(pStyleManager, pStyleScaling);

	for (auto &e : m_ItemList)
		e->NormalizeStyle(pStyleManager, pStyleScaling);
}


void CStatusView::SetTheme(const Theme::CThemeManager *pThemeManager)
{
	StatusViewTheme Theme;
	GetStatusViewThemeFromThemeManager(pThemeManager, &Theme);
	SetStatusViewTheme(Theme);

	SetItemTheme(pThemeManager);
}


const CStatusItem *CStatusView::GetItem(int Index) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_ItemList.size())
		return nullptr;
	return m_ItemList[Index].get();
}


CStatusItem *CStatusView::GetItem(int Index)
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_ItemList.size())
		return nullptr;
	return m_ItemList[Index].get();
}


const CStatusItem *CStatusView::GetItemByID(int ID) const
{
	const int Index = IDToIndex(ID);

	if (Index < 0)
		return nullptr;
	return m_ItemList[Index].get();
}


CStatusItem *CStatusView::GetItemByID(int ID)
{
	const int Index = IDToIndex(ID);

	if (Index < 0)
		return nullptr;
	return m_ItemList[Index].get();
}


bool CStatusView::AddItem(CStatusItem *pItem)
{
	if (pItem == nullptr)
		return false;

	m_ItemList.emplace_back(pItem);

	pItem->m_pStatus = this;

	RegisterUIChild(pItem);

	if (m_hwnd != nullptr) {
		if (pItem->GetWidth() < 0)
			pItem->SetWidth(CalcItemPixelSize(pItem->GetDefaultWidth()));
		pItem->SetActualWidth(pItem->GetWidth());
	}

	return true;
}


int CStatusView::IDToIndex(int ID) const
{
	for (size_t i = 0; i < m_ItemList.size(); i++) {
		if (m_ItemList[i]->GetID() == ID)
			return static_cast<int>(i);
	}
	return -1;
}


int CStatusView::IndexToID(int Index) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_ItemList.size())
		return -1;
	return m_ItemList[Index]->GetID();
}


LRESULT CStatusView::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			InitializeUI();

			const CREATESTRUCT *pcs = reinterpret_cast<const CREATESTRUCT*>(lParam);
			RECT rc;

			::SetRectEmpty(&rc);
			rc.bottom = m_ItemHeight;
			Theme::BorderStyle Border = m_Theme.Border;
			ConvertBorderWidthsInPixels(&Border);
			Theme::AddBorderRect(Border, &rc);
			::AdjustWindowRectEx(&rc, pcs->style, FALSE, pcs->dwExStyle);
			::SetWindowPos(
				hwnd, nullptr, 0, 0, pcs->cx, rc.bottom - rc.top,
				SWP_NOZORDER | SWP_NOMOVE);

			m_HotItem = -1;
			m_MouseLeaveTrack.Initialize(hwnd);

			m_CapturedItem = -1;
		}
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd, &ps);
			if (m_fBufferedPaint) {
				const HDC hdc = m_BufferedPaint.Begin(ps.hdc, &ps.rcPaint, true);

				if (hdc != nullptr) {
					Draw(hdc, &ps.rcPaint);
					m_BufferedPaint.SetOpaque();
					m_BufferedPaint.End();
				} else {
					Draw(ps.hdc, &ps.rcPaint);
				}
			} else {
				Draw(ps.hdc, &ps.rcPaint);
			}
			::EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_SIZE:
		AdjustSize();
		return 0;

	case WM_MOUSEMOVE:
		{
			int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

			if (::GetCapture() != hwnd) {
				if (m_fSingleMode)
					break;

				const POINT pt = {x, y};
				int i;
				for (i = 0; i < static_cast<int>(m_ItemList.size()); i++) {
					if (!m_ItemList[i]->GetVisible())
						continue;
					RECT rc;
					GetItemRectByIndex(i, &rc);
					if (::PtInRect(&rc, pt))
						break;
				}
				if (i == static_cast<int>(m_ItemList.size()))
					i = -1;
				if (i != m_HotItem)
					SetHotItem(i);
				m_MouseLeaveTrack.OnMouseMove();
			}

			if (m_HotItem >= 0) {
				RECT rc;
				GetItemRectByIndex(m_HotItem, &rc);
				x -= rc.left;
				y -= rc.top;
				m_ItemList[m_HotItem]->OnMouseMove(x, y);
			}
		}
		return 0;

	case WM_MOUSELEAVE:
		{
			const bool fLeave = m_MouseLeaveTrack.OnMouseLeave();
			if (!m_fOnButtonDown) {
				if (m_HotItem >= 0)
					SetHotItem(-1);
				if (fLeave && m_pEventHandler)
					m_pEventHandler->OnMouseLeave();
			}
		}
		return 0;

	case WM_NCMOUSEMOVE:
		m_MouseLeaveTrack.OnNcMouseMove();
		return 0;

	case WM_NCMOUSELEAVE:
		if (m_MouseLeaveTrack.OnNcMouseLeave()) {
			if (m_pEventHandler)
				m_pEventHandler->OnMouseLeave();
		}
		return 0;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONDBLCLK:
		if (m_HotItem >= 0) {
			CStatusItem *pItem = m_ItemList[m_HotItem].get();
			int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
			RECT rc;

			GetItemRectByIndex(m_HotItem, &rc);
			x -= rc.left;
			y -= rc.top;

			const bool fCaptured = ::GetCapture() == hwnd;

			m_fOnButtonDown = true;

			switch (uMsg) {
			case WM_LBUTTONDOWN:
				pItem->OnLButtonDown(x, y);
				break;
			case WM_LBUTTONDBLCLK:
				pItem->OnLButtonDoubleClick(x, y);
				break;
			case WM_RBUTTONDOWN:
				pItem->OnRButtonDown(x, y);
				break;
			case WM_RBUTTONDBLCLK:
				pItem->OnRButtonDoubleClick(x, y);
				break;
			case WM_MBUTTONDOWN:
				pItem->OnMButtonDown(x, y);
				break;
			case WM_MBUTTONDBLCLK:
				pItem->OnMButtonDoubleClick(x, y);
				break;
			}

			m_fOnButtonDown = false;

			if (!fCaptured && ::GetCapture() == hwnd)
				m_CapturedItem = m_HotItem;

			if (!m_MouseLeaveTrack.IsClientTrack()) {
				POINT pt;

				::GetCursorPos(&pt);
				::ScreenToClient(hwnd, &pt);
				::GetClientRect(hwnd, &rc);
				if (::PtInRect(&rc, pt)) {
					::SendMessage(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(static_cast<SHORT>(pt.x), static_cast<SHORT>(pt.y)));
				} else {
					SetHotItem(-1);
					if (m_pEventHandler)
						m_pEventHandler->OnMouseLeave();
				}
			}
		}
		return 0;

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
		if (m_HotItem >= 0) {
			CStatusItem *pItem = m_ItemList[m_HotItem].get();
			int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
			RECT rc;

			GetItemRectByIndex(m_HotItem, &rc);
			x -= rc.left;
			y -= rc.top;
			switch (uMsg) {
			case WM_LBUTTONUP:
				pItem->OnLButtonUp(x, y);
				break;
			case WM_RBUTTONUP:
				pItem->OnRButtonUp(x, y);
				break;
			case WM_MBUTTONUP:
				pItem->OnMButtonUp(x, y);
				break;
			}
		}

		if (::GetCapture() == hwnd) {
			::ReleaseCapture();
		}
		return 0;

	case WM_MOUSEHOVER:
		if (m_HotItem >= 0) {
			int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
			RECT rc;

			GetItemRectByIndex(m_HotItem, &rc);
			x -= rc.left;
			y -= rc.top;
			if (m_ItemList[m_HotItem]->OnMouseHover(x, y)) {
				TRACKMOUSEEVENT tme;
				tme.cbSize = sizeof(TRACKMOUSEEVENT);
				tme.dwFlags = TME_HOVER;
				tme.hwndTrack = hwnd;
				tme.dwHoverTime = HOVER_DEFAULT;
				::TrackMouseEvent(&tme);
			}
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

	case WM_CAPTURECHANGED:
		if (m_CapturedItem >= 0) {
			CStatusItem *pItem = GetItem(m_CapturedItem);

			m_CapturedItem = -1;
			if (pItem != nullptr)
				pItem->OnCaptureReleased();
		}
		return 0;

	case WM_NOTIFY:
		if (m_HotItem >= 0)
			return m_ItemList[m_HotItem]->OnNotifyMessage(reinterpret_cast<LPNMHDR>(lParam));
		break;

	case WM_DISPLAYCHANGE:
		m_Offscreen.Destroy();
		return 0;

	case WM_DESTROY:
		m_Offscreen.Destroy();
		return 0;
	}

	return CCustomWindow::OnMessage(hwnd, uMsg, wParam, lParam);
}


bool CStatusView::UpdateItem(int ID)
{
	CStatusItem *pItem = GetItemByID(ID);

	if (pItem == nullptr || !pItem->UpdateContent())
		return false;

	RedrawItem(ID);

	return true;
}


void CStatusView::RedrawItem(int ID)
{
	if (m_hwnd != nullptr) {
		RECT rc;

		GetItemRect(ID, &rc);
		if (rc.right > rc.left)
			Invalidate(&rc);
	}
}


bool CStatusView::GetItemRect(int ID, RECT *pRect) const
{
	const int Index = IDToIndex(ID);
	if (Index < 0)
		return false;
	return GetItemRectByIndex(Index, pRect);
}


bool CStatusView::GetItemRectByIndex(int Index, RECT *pRect) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_ItemList.size())
		return false;

	RECT rc;
	GetClientRect(&rc);
	Theme::BorderStyle Border = m_Theme.Border;
	ConvertBorderWidthsInPixels(&Border);
	Theme::SubtractBorderRect(Border, &rc);
	if (m_Rows > 1)
		rc.bottom = rc.top + m_ItemHeight;
	const int HorzMargin = m_Style.ItemPadding.Horz();
	const int Left = rc.left;
	for (int i = 0; i < Index; i++) {
		const CStatusItem *pItem = m_ItemList[i].get();
		if (pItem->m_fBreak) {
			rc.left = Left;
			rc.top = rc.bottom;
			rc.bottom += m_ItemHeight;
		} else if (pItem->GetVisible()) {
			rc.left += pItem->GetActualWidth() + HorzMargin;
		}
	}
	rc.right = rc.left;
	const CStatusItem *pItem = m_ItemList[Index].get();
	if (pItem->GetVisible())
		rc.right += pItem->GetActualWidth() + HorzMargin;
	*pRect = rc;
	return true;
}


bool CStatusView::GetItemClientRect(int ID, RECT *pRect) const
{
	RECT rc;

	if (!GetItemRect(ID, &rc))
		return false;
	Style::Subtract(&rc, m_Style.ItemPadding);
	if (rc.right < rc.left)
		rc.right = rc.left;
	if (rc.bottom < rc.top)
		rc.bottom = rc.top;
	*pRect = rc;
	return true;
}


int CStatusView::GetItemHeight() const
{
	RECT rc;

	if (m_Rows > 1)
		return m_ItemHeight;
	GetClientRect(&rc);
	Theme::BorderStyle Border = m_Theme.Border;
	ConvertBorderWidthsInPixels(&Border);
	Theme::SubtractBorderRect(Border, &rc);
	return rc.bottom - rc.top;
}


int CStatusView::CalcItemHeight(const DrawUtil::CFont &Font) const
{
	return CalcItemHeight(CalcTextHeight(Font));
}


const Style::Margins &CStatusView::GetItemPadding() const
{
	return m_Style.ItemPadding;
}


const Style::Size &CStatusView::GetIconSize() const
{
	return m_Style.IconSize;
}


int CStatusView::GetIntegralWidth() const
{
	int Width = 0;

	for (const auto &e : m_ItemList) {
		if (e->GetVisible())
			Width += e->GetWidth() + m_Style.ItemPadding.Horz();
	}
	RECT rc;
	GetBorderWidthsInPixels(m_Theme.Border, &rc);
	return Width + rc.left + rc.right;
}


void CStatusView::SetVisible(bool fVisible)
{
	if (m_HotItem >= 0)
		SetHotItem(-1);
	CBasicWindow::SetVisible(fVisible);
	for (const auto &e : m_ItemList)
		e->OnPresentStatusChange(fVisible && e->GetVisible());
}


bool CStatusView::AdjustSize()
{
	if (m_hwnd == nullptr || !m_fAdjustSize)
		return false;

	const int OldRows = m_Rows;
	RECT rcWindow, rc;

	CalcLayout();
	GetPosition(&rcWindow);
	::SetRectEmpty(&rc);
	rc.bottom = m_ItemHeight * m_Rows;
	Theme::BorderStyle Border = m_Theme.Border;
	ConvertBorderWidthsInPixels(&Border);
	Theme::AddBorderRect(Border, &rc);
	::AdjustWindowRectEx(&rc, GetWindowStyle(), FALSE, GetWindowExStyle());
	const int Height = rc.bottom - rc.top;
	if (Height != rcWindow.bottom - rcWindow.top) {
		::SetWindowPos(
			m_hwnd, nullptr, 0, 0, rcWindow.right - rcWindow.left, Height,
			SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
		Invalidate();
		if (m_pEventHandler != nullptr)
			m_pEventHandler->OnHeightChanged(Height);
	} else {
		Invalidate();
	}

	return true;
}


void CStatusView::SetSingleText(LPCTSTR pszText)
{
	if (pszText != nullptr) {
		m_SingleText = pszText;
		m_fSingleMode = true;
		SetHotItem(-1);
	} else {
		if (!m_fSingleMode)
			return;
		m_SingleText.clear();
		m_fSingleMode = false;
	}
	if (m_hwnd != nullptr)
		Redraw(nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
}


bool CStatusView::GetStatusViewThemeFromThemeManager(
	const Theme::CThemeManager *pThemeManager, StatusViewTheme *pTheme)
{
	if (pThemeManager == nullptr || pTheme == nullptr)
		return false;

	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_STATUSBAR_ITEM,
		&pTheme->ItemStyle);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_STATUSBAR_BOTTOMITEM,
		&pTheme->BottomItemStyle);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_STATUSBAR_ITEM_HOT,
		&pTheme->HighlightItemStyle);
	pThemeManager->GetBorderStyle(
		Theme::CThemeManager::STYLE_STATUSBAR,
		&pTheme->Border);

	return true;
}


bool CStatusView::SetStatusViewTheme(const StatusViewTheme &Theme)
{
	m_Theme = Theme;
	if (m_hwnd != nullptr) {
		AdjustSize();
		Invalidate();
	}
	return true;
}


bool CStatusView::GetStatusViewTheme(StatusViewTheme *pTheme) const
{
	if (pTheme == nullptr)
		return false;
	*pTheme = m_Theme;
	return true;
}


void CStatusView::SetItemTheme(const Theme::CThemeManager *pThemeManager)
{
	for (auto &e : m_ItemList)
		e->SetTheme(pThemeManager);
}


bool CStatusView::SetFont(const Style::Font &Font)
{
	m_Font = Font;
	m_DrawFont.Destroy();

	if (m_hwnd != nullptr) {
		CreateDrawFont(m_Font, &m_DrawFont);
		m_TextHeight = CalcTextHeight(&m_FontHeight);
		m_ItemHeight = CalcItemHeight();
		AdjustSize();
		for (auto &e : m_ItemList)
			e->OnFontChanged();
		Invalidate();
	}

	return true;
}


bool CStatusView::GetFont(Style::Font *pFont) const
{
	if (pFont == nullptr)
		return false;
	*pFont = m_Font;
	return true;
}


HFONT CStatusView::GetFont() const
{
	return m_DrawFont.GetHandle();
}


int CStatusView::GetCurItem() const
{
	if (m_HotItem < 0)
		return -1;
	return m_ItemList[m_HotItem]->GetID();
}


bool CStatusView::SetMultiRow(bool fMultiRow)
{
	if (m_fMultiRow != fMultiRow) {
		m_fMultiRow = fMultiRow;
		if (m_hwnd != nullptr)
			AdjustSize();
	}
	return true;
}


bool CStatusView::SetMaxRows(int MaxRows)
{
	if (MaxRows < 1)
		return false;
	if (m_MaxRows != MaxRows) {
		m_MaxRows = MaxRows;
		if (m_hwnd != nullptr)
			AdjustSize();
	}
	return true;
}


int CStatusView::CalcHeight(int Width) const
{
	std::vector<const CStatusItem*> ItemList;
	ItemList.reserve(m_ItemList.size());
	for (auto &e : m_ItemList) {
		if (e->GetVisible())
			ItemList.push_back(e.get());
	}

	RECT rcBorder;
	GetBorderWidthsInPixels(m_Theme.Border, &rcBorder);

	const int Rows = CalcRows(ItemList, Width - (rcBorder.left + rcBorder.right));

	return m_ItemHeight * Rows + rcBorder.top + rcBorder.bottom;
}


bool CStatusView::SetEventHandler(CEventHandler *pEventHandler)
{
	if (m_pEventHandler != nullptr)
		m_pEventHandler->m_pStatusView = nullptr;
	if (pEventHandler != nullptr)
		pEventHandler->m_pStatusView = this;
	m_pEventHandler = pEventHandler;
	return true;
}


bool CStatusView::SetItemOrder(const int *pOrderList)
{
	std::vector<CStatusItem*> NewList(m_ItemList.size());

	for (size_t i = 0; i < m_ItemList.size(); i++) {
		CStatusItem *pItem = GetItem(IDToIndex(pOrderList[i]));

		if (pItem == nullptr)
			return false;
		for (size_t j = 0; j < i; j++) {
			if (NewList[j] == pItem)
				return false;
		}
		NewList[i] = pItem;
	}

	for (size_t i = 0; i < m_ItemList.size(); i++) {
		m_ItemList[i].release();
		m_ItemList[i].reset(NewList[i]);
	}

	if (m_hwnd != nullptr && !m_fSingleMode) {
		AdjustSize();
		Invalidate();
	}
	return true;
}


bool CStatusView::CreateItemPreviewFont(
	const Style::Font &Font, DrawUtil::CFont *pDrawFont) const
{
	return CreateDrawFont(Font, pDrawFont);
}


bool CStatusView::DrawItemPreview(
	CStatusItem *pItem, HDC hdc, const RECT &ItemRect,
	bool fHighlight, HFONT hfont) const
{
	if (pItem == nullptr || hdc == nullptr)
		return false;

	Theme::CThemeDraw ThemeDraw(BeginThemeDraw(hdc));
	HFONT hfontOld;
	const Theme::Style &Style =
		fHighlight ? m_Theme.HighlightItemStyle : m_Theme.ItemStyle;

	if (hfont != nullptr)
		hfontOld = SelectFont(hdc, hfont);
	else
		hfontOld = DrawUtil::SelectObject(hdc, m_DrawFont);
	const int OldBkMode = ::SetBkMode(hdc, TRANSPARENT);
	const COLORREF crOldTextColor = ::SetTextColor(hdc, Style.Fore.Fill.GetSolidColor());
	const COLORREF crOldBkColor = ::SetBkColor(hdc, Style.Back.Fill.GetSolidColor());
	ThemeDraw.Draw(Style.Back, ItemRect);
	RECT rcDraw = ItemRect;
	Style::Subtract(&rcDraw, m_Style.ItemPadding);
	CStatusItem::DrawFlag Flags = CStatusItem::DrawFlag::Preview;
	if (fHighlight)
		Flags |= CStatusItem::DrawFlag::Highlight;
	pItem->Draw(hdc, ItemRect, rcDraw, Flags);
	::SetBkColor(hdc, crOldBkColor);
	::SetTextColor(hdc, crOldTextColor);
	::SetBkMode(hdc, OldBkMode);
	SelectFont(hdc, hfontOld);
	return true;
}


bool CStatusView::EnableBufferedPaint(bool fEnable)
{
	m_fBufferedPaint = fEnable;
	return true;
}


void CStatusView::EnableSizeAdjustment(bool fEnable)
{
	if (m_fAdjustSize != fEnable) {
		m_fAdjustSize = fEnable;
		if (fEnable && m_hwnd != nullptr)
			AdjustSize();
	}
}


void CStatusView::SetHotItem(int Item)
{
	if (Item < 0 || static_cast<size_t>(Item) >= m_ItemList.size())
		Item = -1;
	if (m_HotItem != Item) {
		const int OldHotItem = m_HotItem;

		m_HotItem = Item;
		if (OldHotItem >= 0) {
			m_ItemList[OldHotItem]->OnFocus(false);
			RedrawItem(IndexToID(OldHotItem));
		}
		if (m_HotItem >= 0) {
			m_ItemList[m_HotItem]->OnFocus(true);
			RedrawItem(IndexToID(m_HotItem));
		}

		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(TRACKMOUSEEVENT);
		tme.dwFlags = TME_HOVER;
		if (m_HotItem < 0)
			tme.dwFlags |= TME_CANCEL;
		tme.hwndTrack = m_hwnd;
		tme.dwHoverTime = HOVER_DEFAULT;
		::TrackMouseEvent(&tme);
	}
}


void CStatusView::Draw(HDC hdc, const RECT *pPaintRect)
{
	const int HorzMargin = m_Style.ItemPadding.Horz();
	RECT rcClient, rc;
	HDC hdcDst;

	GetClientRect(&rcClient);
	rc = rcClient;
	Theme::BorderStyle Border = m_Theme.Border;
	ConvertBorderWidthsInPixels(&Border);
	Theme::SubtractBorderRect(Border, &rc);
	const int ItemHeight = m_Rows > 1 ? m_ItemHeight : rc.bottom - rc.top;

	if (!m_fSingleMode) {
		int MaxWidth = 0;
		for (auto &e : m_ItemList) {
			if (e->GetVisible() && e->GetActualWidth() > MaxWidth)
				MaxWidth = e->GetActualWidth();
		}
		if (MaxWidth + HorzMargin > m_Offscreen.GetWidth()
				|| ItemHeight > m_Offscreen.GetHeight())
			m_Offscreen.Create(MaxWidth + HorzMargin, ItemHeight);
		hdcDst = m_Offscreen.GetDC();
		if (hdcDst == nullptr)
			hdcDst = hdc;
	} else {
		hdcDst = hdc;
	}

	const HFONT hfontOld = DrawUtil::SelectObject(hdcDst, m_DrawFont);
	const int OldBkMode = ::SetBkMode(hdcDst, TRANSPARENT);
	const COLORREF crOldTextColor = ::GetTextColor(hdcDst);
	const COLORREF crOldBkColor = ::GetBkColor(hdcDst);

	if (m_Rows > 1)
		rc.bottom = rc.top + ItemHeight;

	Theme::CThemeDraw ThemeDraw(BeginThemeDraw(hdc));

	if (m_fSingleMode) {
		RECT rcRow = rc;

		for (int i = 0; i < m_Rows; i++) {
			const Theme::Style &Style = i == 0 ? m_Theme.ItemStyle : m_Theme.BottomItemStyle;

			ThemeDraw.Draw(Style.Back.Fill, rcRow);
			rcRow.top = rcRow.bottom;
			rcRow.bottom += ItemHeight;
		}
		rc.left += m_Style.ItemPadding.Left;
		rc.right -= m_Style.ItemPadding.Right;
		ThemeDraw.Draw(
			m_Theme.ItemStyle.Fore, rc, m_SingleText.c_str(),
			DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
	} else {
		Theme::CThemeDraw ItemThemeDraw(BeginThemeDraw(hdcDst));
		const int Left = rc.left;
		int Row = 0;

		rc.right = Left;
		for (int i = 0; i < static_cast<int>(m_ItemList.size()); i++) {
			CStatusItem *pItem = m_ItemList[i].get();

			if (pItem->GetVisible()) {
				rc.left = rc.right;
				rc.right = rc.left + pItem->GetActualWidth() + HorzMargin;
				if (rc.right > pPaintRect->left && rc.left < pPaintRect->right) {
					const bool fHighlight = i == m_HotItem;
					const Theme::Style &Style =
						fHighlight ? m_Theme.HighlightItemStyle :
						Row == 0 ? m_Theme.ItemStyle : m_Theme.BottomItemStyle;
					RECT rcItem = rc;
					if (hdcDst != hdc)
						::OffsetRect(&rcItem, -rcItem.left, -rcItem.top);
					ItemThemeDraw.Draw(Style.Back, rcItem);
					::SetTextColor(hdcDst, Style.Fore.Fill.GetSolidColor());
					::SetBkColor(hdcDst, Style.Back.Fill.GetSolidColor());
					RECT rcDraw = rcItem;
					Style::Subtract(&rcDraw, m_Style.ItemPadding);
					CStatusItem::DrawFlag Flags = CStatusItem::DrawFlag::None;
					if (fHighlight)
						Flags |= CStatusItem::DrawFlag::Highlight;
					if (Row > 0)
						Flags |= CStatusItem::DrawFlag::Bottom;
					pItem->Draw(hdcDst, rcItem, rcDraw, Flags);
					if (hdcDst != hdc)
						m_Offscreen.CopyTo(hdc, &rc);
				}
			}
			if (m_Rows > 1 && pItem->m_fBreak) {
				if (rc.right < pPaintRect->right) {
					rc.left = std::max(rc.right, pPaintRect->left);
					rc.right = pPaintRect->right;
					ThemeDraw.Draw(Row == 0 ? m_Theme.ItemStyle.Back.Fill : m_Theme.BottomItemStyle.Back.Fill, rc);
				}
				rc.right = Left;
				rc.top = rc.bottom;
				rc.bottom += ItemHeight;
				Row++;
			}
		}
		if (rc.right < pPaintRect->right) {
			rc.left = std::max(rc.right, pPaintRect->left);
			rc.right = pPaintRect->right;
			ThemeDraw.Draw(Row == 0 ? m_Theme.ItemStyle.Back.Fill : m_Theme.BottomItemStyle.Back.Fill, rc);
		}
	}

	ThemeDraw.Draw(m_Theme.Border, rcClient);

	::SetBkColor(hdcDst, crOldBkColor);
	::SetTextColor(hdcDst, crOldTextColor);
	::SetBkMode(hdcDst, OldBkMode);
	::SelectObject(hdcDst, hfontOld);
}


void CStatusView::CalcLayout()
{
	std::vector<CStatusItem*> ItemList;
	ItemList.reserve(m_ItemList.size());
	for (auto &e : m_ItemList) {
		e->m_fBreak = false;
		e->SetActualWidth(e->GetWidth());

		if (e->GetVisible())
			ItemList.push_back(e.get());
	}

	RECT rc;
	GetClientRect(&rc);
	Theme::BorderStyle Border = m_Theme.Border;
	ConvertBorderWidthsInPixels(&Border);
	Theme::SubtractBorderRect(m_Theme.Border, &rc);
	const int MaxRowWidth = rc.right - rc.left;

	m_Rows = CalcRows(ItemList, MaxRowWidth);

	CStatusItem *pVariableItem = nullptr;
	int RowWidth = 0;
	for (size_t i = 0; i < ItemList.size(); i++) {
		CStatusItem *pItem = ItemList[i];

		if (pItem->IsVariableWidth())
			pVariableItem = pItem;
		RowWidth += pItem->GetActualWidth() + m_Style.ItemPadding.Horz();
		if (pItem->m_fBreak || i + 1 == ItemList.size()) {
			if (pVariableItem != nullptr) {
				const int Add = MaxRowWidth - RowWidth;
				if (Add > 0)
					pVariableItem->SetActualWidth(pVariableItem->GetActualWidth() + Add);
			}
			pVariableItem = nullptr;
			RowWidth = 0;
		}
	}
}


int CStatusView::CalcRows(const std::vector<const CStatusItem*> &ItemList, int MaxRowWidth) const
{
	const int MaxRegularRows = m_fMultiRow ? m_MaxRows : 1;
	int Rows = 1;
	int RowWidth = 0;

	for (size_t i = 0; i < ItemList.size(); i++) {
		const CStatusItem *pItem = ItemList[i];

		if (pItem->IsFullRow()) {
			if (pItem->IsForceFullRow()
					|| (Rows < MaxRegularRows
						&& (i == 0 || i + 1 == ItemList.size() || Rows + 1 < MaxRegularRows))) {
				Rows++;
				if (i + 1 < ItemList.size() && Rows < MaxRegularRows)
					Rows++;
				RowWidth = 0;
				continue;
			}
		}

		const int ItemWidth = pItem->GetWidth() + m_Style.ItemPadding.Horz();
		if (Rows < MaxRegularRows && RowWidth > 0 && RowWidth + ItemWidth > MaxRowWidth) {
			Rows++;
			RowWidth = ItemWidth;
		} else {
			RowWidth += ItemWidth;
		}
	}

	return Rows;
}


int CStatusView::CalcRows(const std::vector<CStatusItem*> &ItemList, int MaxRowWidth)
{
	const int MaxRegularRows = m_fMultiRow ? m_MaxRows : 1;
	int Rows = 1;
	int RowWidth = 0;

	for (size_t i = 0; i < ItemList.size(); i++) {
		CStatusItem *pItem = ItemList[i];

		if (pItem->IsFullRow()) {
			if (pItem->IsForceFullRow()
					|| (Rows < MaxRegularRows
						&& (i == 0 || i + 1 == ItemList.size() || Rows + 1 < MaxRegularRows))) {
				if (i > 0)
					ItemList[i - 1]->m_fBreak = true;
				pItem->SetActualWidth(MaxRowWidth - m_Style.ItemPadding.Horz());
				pItem->m_fBreak = true;
				Rows++;
				if (i + 1 < ItemList.size() && Rows < MaxRegularRows)
					Rows++;
				RowWidth = 0;
				continue;
			}
		}

		const int ItemWidth = pItem->GetWidth() + m_Style.ItemPadding.Horz();
		if (Rows < MaxRegularRows && RowWidth > 0 && RowWidth + ItemWidth > MaxRowWidth) {
			if (i > 0)
				ItemList[i - 1]->m_fBreak = true;
			Rows++;
			RowWidth = ItemWidth;
		} else {
			RowWidth += ItemWidth;
		}
	}

	return Rows;
}


int CStatusView::CalcTextHeight(const DrawUtil::CFont &Font, int *pFontHeight) const
{
	TEXTMETRIC tm;
	const int TextHeight = Style::GetFontHeight(
		m_hwnd, Font.GetHandle(), m_Style.TextExtraHeight, &tm);
	if (pFontHeight != nullptr)
		*pFontHeight = tm.tmHeight - tm.tmInternalLeading;
	return TextHeight;
}


int CStatusView::CalcTextHeight(int *pFontHeight) const
{
	return CalcTextHeight(m_DrawFont, pFontHeight);
}


int CStatusView::CalcItemHeight(int TextHeight) const
{
	return std::max(TextHeight, static_cast<int>(m_Style.IconSize.Height)) + m_Style.ItemPadding.Vert();
}


int CStatusView::CalcItemHeight() const
{
	return CalcItemHeight(m_TextHeight);
}


int CStatusView::CalcItemPixelSize(const CStatusItem::SizeValue &Size) const
{
	switch (Size.Unit) {
	case CStatusItem::SizeUnit::Pixel:
		return ::MulDiv(Size.Value, m_pStyleScaling->GetDPI(), 96);

	case CStatusItem::SizeUnit::EM:
		return ::MulDiv(Size.Value, m_FontHeight, CStatusItem::EM_FACTOR);
	}

	return 0;
}


void CStatusView::ApplyStyle()
{
	if (m_hwnd == nullptr)
		return;

	CreateDrawFont(m_Font, &m_DrawFont);

	m_TextHeight = CalcTextHeight(&m_FontHeight);
	m_ItemHeight = CalcItemHeight();

	for (auto &Item : m_ItemList) {
		if (Item->GetWidth() < 0)
			Item->SetWidth(CalcItemPixelSize(Item->GetDefaultWidth()));
		Item->SetActualWidth(Item->GetWidth());

		Item->OnFontChanged();
	}
}


void CStatusView::RealizeStyle()
{
	if (m_hwnd != nullptr) {
		if (m_pEventHandler != nullptr)
			m_pEventHandler->OnStyleChanged();
		AdjustSize();
	}
}




void CStatusView::StatusViewStyle::SetStyle(const Style::CStyleManager *pStyleManager)
{
	*this = StatusViewStyle();
	pStyleManager->Get(TEXT("status-bar.item.padding"), &ItemPadding);
	pStyleManager->Get(TEXT("status-bar.item.text.extra-height"), &TextExtraHeight);
	pStyleManager->Get(TEXT("status-bar.item.icon"), &IconSize);
}


void CStatusView::StatusViewStyle::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	pStyleScaling->ToPixels(&ItemPadding);
	pStyleScaling->ToPixels(&TextExtraHeight);
	pStyleScaling->ToPixels(&IconSize);
}


} // namespace TVTest
