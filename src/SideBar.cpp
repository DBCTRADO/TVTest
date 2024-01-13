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
#include "SideBar.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


const LPCTSTR CSideBar::CLASS_NAME = APP_NAME TEXT(" Side Bar");
HINSTANCE CSideBar::m_hinst = nullptr;


bool CSideBar::Initialize(HINSTANCE hinst)
{
	if (m_hinst == nullptr) {
		WNDCLASS wc;

		wc.style = CS_HREDRAW | CS_VREDRAW;
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


CSideBar::CSideBar(const CCommandManager *pCommandManager)
	: m_pCommandManager(pCommandManager)
{
}


CSideBar::~CSideBar()
{
	Destroy();
	if (m_pEventHandler != nullptr)
		m_pEventHandler->m_pSideBar = nullptr;
}


bool CSideBar::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	return CreateBasicWindow(hwndParent, Style, ExStyle, ID, CLASS_NAME, nullptr, m_hinst);
}


void CSideBar::SetStyle(const Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);
}


void CSideBar::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	m_Style.NormalizeStyle(pStyleManager, pStyleScaling);
}


void CSideBar::SetTheme(const Theme::CThemeManager *pThemeManager)
{
	SideBarTheme Theme;

	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_SIDEBAR_ITEM,
		&Theme.ItemStyle);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_SIDEBAR_ITEM_HOT,
		&Theme.HighlightItemStyle);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_SIDEBAR_ITEM_CHECKED,
		&Theme.CheckItemStyle);
	pThemeManager->GetBorderStyle(
		Theme::CThemeManager::STYLE_SIDEBAR,
		&Theme.Border);

	SetSideBarTheme(Theme);
}


int CSideBar::GetBarWidth() const
{
	Theme::BorderStyle Border = m_Theme.Border;
	ConvertBorderWidthsInPixels(&Border);
	RECT rcBorder;
	Theme::GetBorderWidths(Border, &rcBorder);
	int Width;
	if (m_fVertical) {
		Width = m_Style.IconSize.Width + m_Style.ItemPadding.Horz() + rcBorder.left + rcBorder.right;
	} else {
		Width = m_Style.IconSize.Height + m_Style.ItemPadding.Vert() + rcBorder.top + rcBorder.bottom;
	}
	return Width;
}


bool CSideBar::SetIconImage(HBITMAP hbm, int Width, int Height)
{
	if (hbm == nullptr)
		return false;
	if (!m_Icons.Create(hbm, Width, Height))
		return false;
	if (m_hwnd != nullptr)
		Invalidate();
	return true;
}


void CSideBar::DeleteAllItems()
{
	m_ItemList.clear();
	m_Tooltip.DeleteAllTools();
}


bool CSideBar::AddItem(const SideBarItem *pItem)
{
	return AddItems(pItem, 1);
}


bool CSideBar::AddItems(const SideBarItem *pItemList, int NumItems)
{
	if (pItemList == nullptr || NumItems <= 0)
		return false;

	const size_t OldSize = m_ItemList.size();
	m_ItemList.resize(OldSize + NumItems);
	std::memcpy(&m_ItemList[OldSize], pItemList, NumItems * sizeof(SideBarItem));

	if (m_Tooltip.IsCreated()) {
		for (int i = 0; i < NumItems; i++) {
			if (pItemList[i].Command != ITEM_SEPARATOR) {
				RECT rc;

				GetItemRect(static_cast<int>(OldSize) + i, &rc);
				m_Tooltip.AddTool(static_cast<UINT>(OldSize) + i, rc);
			}
		}
	}
	return true;
}


bool CSideBar::AddSeparator()
{
	SideBarItem Item;

	Item.Command = ITEM_SEPARATOR;
	Item.Icon = -1;
	Item.State = ItemState::None;

	return AddItem(&Item);
}


int CSideBar::GetItemCount() const
{
	return static_cast<int>(m_ItemList.size());
}


int CSideBar::GetItemCommand(int Index) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_ItemList.size())
		return -1;
	return m_ItemList[Index].Command;
}


int CSideBar::CommandToIndex(int Command) const
{
	for (size_t i = 0; i < m_ItemList.size(); i++) {
		if (m_ItemList[i].Command == Command)
			return static_cast<int>(i);
	}
	return -1;
}


bool CSideBar::EnableItem(int Command, bool fEnable)
{
	const int Index = CommandToIndex(Command);

	if (Index < 0)
		return false;
	if (m_ItemList[Index].IsEnabled() != fEnable) {
		m_ItemList[Index].State ^= ItemState::Disabled;
		if (!fEnable && m_HotItem == Index)
			m_HotItem = -1;
		UpdateItem(Index);
	}
	return true;
}


bool CSideBar::EnableItemByIndex(int Index, bool fEnable)
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_ItemList.size())
		return false;
	if (m_ItemList[Index].IsEnabled() != fEnable) {
		m_ItemList[Index].State ^= ItemState::Disabled;
		if (!fEnable && m_HotItem == Index)
			m_HotItem = -1;
		UpdateItem(Index);
	}
	return true;
}


bool CSideBar::IsItemEnabled(int Command) const
{
	const int Index = CommandToIndex(Command);

	if (Index < 0)
		return false;
	return m_ItemList[Index].IsEnabled();
}


bool CSideBar::CheckItem(int Command, bool fCheck)
{
	const int Index = CommandToIndex(Command);

	if (Index < 0)
		return false;
	if (m_ItemList[Index].IsChecked() != fCheck) {
		m_ItemList[Index].State ^= ItemState::Checked;
		UpdateItem(Index);
	}
	return true;
}


bool CSideBar::CheckItemByIndex(int Index, bool fCheck)
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_ItemList.size())
		return false;
	if (m_ItemList[Index].IsChecked() != fCheck) {
		m_ItemList[Index].State ^= ItemState::Checked;
		UpdateItem(Index);
	}
	return true;
}


bool CSideBar::CheckRadioItem(int First, int Last, int Check)
{
	if (First > Last)
		return false;
	for (int i = First; i <= Last; i++)
		CheckItem(i, i == Check);
	return true;
}


bool CSideBar::IsItemChecked(int Command) const
{
	const int Index = CommandToIndex(Command);

	if (Index < 0)
		return false;
	return m_ItemList[Index].IsChecked();
}


bool CSideBar::RedrawItem(int Command)
{
	const int Index = CommandToIndex(Command);

	if (Index < 0)
		return false;

	UpdateItem(Index);

	return true;
}


bool CSideBar::SetSideBarTheme(const SideBarTheme &Theme)
{
	int OldBarWidth;
	if (m_hwnd != nullptr && m_pEventHandler != nullptr)
		OldBarWidth = GetBarWidth();

	m_Theme = Theme;

	if (m_hwnd != nullptr) {
		if (m_pEventHandler != nullptr) {
			const int NewBarWidth = GetBarWidth();
			if (NewBarWidth != OldBarWidth)
				m_pEventHandler->OnBarWidthChanged(NewBarWidth);
		}
		Invalidate();
	}

	return true;
}


bool CSideBar::GetSideBarTheme(SideBarTheme *pTheme) const
{
	if (pTheme == nullptr)
		return false;
	*pTheme = m_Theme;
	return true;
}


void CSideBar::ShowToolTips(bool fShow)
{
	if (m_fShowTooltips != fShow) {
		m_fShowTooltips = fShow;
		m_Tooltip.Enable(fShow);
	}
}


void CSideBar::SetVertical(bool fVertical)
{
	if (m_fVertical != fVertical) {
		m_fVertical = fVertical;
		if (m_hwnd != nullptr) {
			Invalidate();
			UpdateTooltipsRect();
		}
	}
}


void CSideBar::SetEventHandler(CEventHandler *pHandler)
{
	if (m_pEventHandler != nullptr)
		m_pEventHandler->m_pSideBar = nullptr;
	if (pHandler != nullptr)
		pHandler->m_pSideBar = this;
	m_pEventHandler = pHandler;
}


Style::Size CSideBar::GetIconDrawSize() const
{
	return m_Style.IconSize;
}


LRESULT CSideBar::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			InitializeUI();

			const CREATESTRUCT *pcs = reinterpret_cast<const CREATESTRUCT*>(lParam);

			m_Tooltip.Create(hwnd);
			m_Tooltip.Enable(m_fShowTooltips);
			SetTooltipFont();

			for (int i = 0; i < static_cast<int>(m_ItemList.size()); i++) {
				if (m_ItemList[i].Command != ITEM_SEPARATOR) {
					RECT rc;
					GetItemRect(i, &rc);
					m_Tooltip.AddTool(i, rc);
				}
			}

			m_HotItem = -1;
			m_MouseLeaveTrack.Initialize(hwnd);
		}
		return 0;

	case WM_SIZE:
		if (m_HotItem >= 0) {
			UpdateItem(m_HotItem);
			m_HotItem = -1;
		}
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

			if (HotItem >= 0
					&& (m_ItemList[HotItem].Command == ITEM_SEPARATOR
						|| m_ItemList[HotItem].IsDisabled()))
				HotItem = -1;
			if (GetCapture() == hwnd) {
				if (HotItem != m_ClickItem)
					HotItem = -1;
				if (HotItem != m_HotItem) {
					const int OldHotItem = m_HotItem;
					m_HotItem = HotItem;
					if (OldHotItem >= 0)
						UpdateItem(OldHotItem);
					if (m_HotItem >= 0)
						UpdateItem(m_HotItem);
				}
			} else {
				if (HotItem != m_HotItem) {
					const int OldHotItem = m_HotItem;
					m_HotItem = HotItem;
					if (OldHotItem >= 0)
						UpdateItem(OldHotItem);
					if (m_HotItem >= 0)
						UpdateItem(m_HotItem);
				}
				m_MouseLeaveTrack.OnMouseMove();
			}
		}
		return 0;

	case WM_MOUSELEAVE:
		if (m_HotItem >= 0) {
			UpdateItem(m_HotItem);
			m_HotItem = -1;
		}
		if (m_MouseLeaveTrack.OnMouseLeave()) {
			if (m_pEventHandler)
				m_pEventHandler->OnMouseLeave();
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
		if (m_HotItem >= 0) {
			m_ClickItem = m_HotItem;
			::SetCapture(hwnd);
		}
		return 0;

	case WM_LBUTTONUP:
		if (::GetCapture() == hwnd) {
			::ReleaseCapture();
			if (m_HotItem >= 0) {
				if (m_pEventHandler != nullptr)
					m_pEventHandler->OnCommand(m_ItemList[m_HotItem].Command);
			}
		}
		return 0;

	case WM_RBUTTONUP:
		if (m_pEventHandler != nullptr)
			m_pEventHandler->OnRButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT) {
			if (m_HotItem >= 0) {
				::SetCursor(GetActionCursor());
				return TRUE;
			}
		}
		break;

	case WM_NOTIFY:
		switch (reinterpret_cast<NMHDR*>(lParam)->code) {
		case TTN_NEEDTEXT:
			{
				LPNMTTDISPINFO pnmttdi = reinterpret_cast<LPNMTTDISPINFO>(lParam);

				if (m_pEventHandler == nullptr
						|| !m_pEventHandler->GetTooltipText(
							m_ItemList[pnmttdi->hdr.idFrom].Command,
							pnmttdi->szText, lengthof(pnmttdi->szText))) {
					m_pCommandManager->GetCommandShortText(
						m_ItemList[pnmttdi->hdr.idFrom].Command,
						pnmttdi->szText, lengthof(pnmttdi->szText));
				}
				pnmttdi->lpszText = pnmttdi->szText;
				pnmttdi->hinst = nullptr;
			}
			return 0;

		case TTN_SHOW:
			if (m_fVertical) {
				const NMHDR *pnmh = reinterpret_cast<const NMHDR*>(lParam);
				RECT rcBar, rcTip;

				::GetWindowRect(hwnd, &rcBar);
				Theme::BorderStyle Border = m_Theme.Border;
				ConvertBorderWidthsInPixels(&Border);
				Theme::SubtractBorderRect(Border, &rcBar);
				::GetWindowRect(pnmh->hwndFrom, &rcTip);
				int x = rcBar.right;
				const HMONITOR hMonitor = ::MonitorFromRect(&rcTip, MONITOR_DEFAULTTONULL);
				if (hMonitor != nullptr) {
					MONITORINFO mi;

					mi.cbSize = sizeof(mi);
					if (::GetMonitorInfo(hMonitor, &mi)) {
						if (x >= mi.rcMonitor.right - 16)
							x = rcBar.left - (rcTip.right - rcTip.left);
					}
				}
				::SetWindowPos(
					pnmh->hwndFrom, HWND_TOPMOST,
					x, rcTip.top, rcTip.right - rcTip.left, rcTip.bottom - rcTip.top,
					SWP_NOACTIVATE);
				return TRUE;
			}
			break;
		}
		break;

	case WM_DESTROY:
		m_Tooltip.Destroy();
		m_TooltipFont.Destroy();
		return 0;
	}

	return CCustomWindow::OnMessage(hwnd, uMsg, wParam, lParam);
}


void CSideBar::RealizeStyle()
{
	if (m_hwnd != nullptr) {
		RECT rc;
		GetPosition(&rc);
		const int OldBarWidth = m_fVertical ? rc.right - rc.left : rc.bottom - rc.top;
		SendSizeMessage();
		Invalidate();

		if (m_Tooltip.IsCreated())
			SetTooltipFont();

		if (m_pEventHandler != nullptr) {
			m_pEventHandler->OnStyleChanged();
			const int NewBarWidth = GetBarWidth();
			if (OldBarWidth != NewBarWidth)
				m_pEventHandler->OnBarWidthChanged(NewBarWidth);
		}
	}
}


void CSideBar::GetItemRect(int Item, RECT *pRect) const
{
	const int ItemWidth = m_Style.IconSize.Width + m_Style.ItemPadding.Horz();
	const int ItemHeight = m_Style.IconSize.Height + m_Style.ItemPadding.Vert();
	int Offset = 0;

	for (int i = 0; i < Item; i++) {
		if (m_ItemList[i].Command == ITEM_SEPARATOR) {
			Offset += m_Style.SeparatorWidth;
		} else {
			if (m_fVertical)
				Offset += ItemHeight;
			else
				Offset += ItemWidth;
		}
	}
	Theme::BorderStyle Border = m_Theme.Border;
	ConvertBorderWidthsInPixels(&Border);
	RECT rcBorder;
	Theme::GetBorderWidths(Border, &rcBorder);
	if (m_fVertical) {
		pRect->left = rcBorder.left;
		pRect->right = rcBorder.left + ItemWidth;
		pRect->top = rcBorder.top + Offset;
		pRect->bottom = pRect->top + (m_ItemList[Item].Command == ITEM_SEPARATOR ? static_cast<int>(m_Style.SeparatorWidth) : ItemHeight);
	} else {
		pRect->top = rcBorder.top;
		pRect->bottom = rcBorder.top + ItemHeight;
		pRect->left = rcBorder.left + Offset;
		pRect->right = pRect->left + (m_ItemList[Item].Command == ITEM_SEPARATOR ? static_cast<int>(m_Style.SeparatorWidth) : ItemWidth);
	}
}


void CSideBar::UpdateItem(int Item)
{
	if (m_hwnd != nullptr) {
		RECT rc;

		GetItemRect(Item, &rc);
		Invalidate(&rc);
	}
}


int CSideBar::HitTest(int x, int y) const
{
	const POINT pt = {x, y};

	for (int i = 0; i < static_cast<int>(m_ItemList.size()); i++) {
		RECT rc;
		GetItemRect(i, &rc);
		if (::PtInRect(&rc, pt))
			return i;
	}
	return -1;
}


void CSideBar::UpdateTooltipsRect()
{
	for (int i = 0; i < static_cast<int>(m_ItemList.size()); i++) {
		RECT rc;

		GetItemRect(i, &rc);
		m_Tooltip.SetToolRect(i, rc);
	}
}


void CSideBar::SetTooltipFont()
{
	Style::Font Font;

	GetSystemFont(DrawUtil::FontType::Status, &Font);
	CreateDrawFont(Font, &m_TooltipFont);
	m_Tooltip.SetFont(m_TooltipFont.GetHandle());
}


void CSideBar::Draw(HDC hdc, const RECT &PaintRect)
{
	RECT rcClient, rc;

	GetClientRect(&rcClient);
	rc = rcClient;
	if (m_fVertical) {
		rc.top = PaintRect.top;
		rc.bottom = PaintRect.bottom;
	} else {
		rc.left = PaintRect.left;
		rc.right = PaintRect.right;
	}

	Theme::CThemeDraw ThemeDraw(BeginThemeDraw(hdc));

	Theme::BackgroundStyle BackStyle;
	BackStyle = m_Theme.ItemStyle.Back;
	if (!m_fVertical && BackStyle.Fill.Type == Theme::FillType::Gradient)
		BackStyle.Fill.Gradient.Rotate(Theme::GradientStyle::RotateType::Right);
	ThemeDraw.Draw(BackStyle, rc);

	const HDC hdcMemory = ::CreateCompatibleDC(hdc);
	const HBITMAP hbmOld = static_cast<HBITMAP>(::GetCurrentObject(hdcMemory, OBJ_BITMAP));

	for (int i = 0; i < static_cast<int>(m_ItemList.size()); i++) {
		GetItemRect(i, &rc);
		if (m_ItemList[i].Command != ITEM_SEPARATOR
				&& rc.left < PaintRect.right && rc.right > PaintRect.left
				&& rc.top < PaintRect.bottom && rc.bottom > PaintRect.top) {
			const bool fHot = m_HotItem == i;
			COLORREF ForeColor;
			BYTE Opacity = 255;

			if (fHot) {
				Theme::Style Style = m_Theme.HighlightItemStyle;
				if (!m_fVertical && Style.Back.Fill.Type == Theme::FillType::Gradient)
					Style.Back.Fill.Gradient.Rotate(Theme::GradientStyle::RotateType::Right);
				if (m_ItemList[i].IsChecked())
					Style.Back.Border = m_Theme.CheckItemStyle.Back.Border;
				ThemeDraw.Draw(Style.Back, rc);
				ForeColor = m_Theme.HighlightItemStyle.Fore.Fill.GetSolidColor();
			} else {
				if (m_ItemList[i].IsChecked()) {
					Theme::Style Style = m_Theme.CheckItemStyle;
					if (!m_fVertical && Style.Back.Fill.Type == Theme::FillType::Gradient)
						Style.Back.Fill.Gradient.Rotate(Theme::GradientStyle::RotateType::Right);
					ThemeDraw.Draw(Style.Back, rc);
					ForeColor = m_Theme.CheckItemStyle.Fore.Fill.GetSolidColor();
				} else {
					ForeColor = m_Theme.ItemStyle.Fore.Fill.GetSolidColor();
				}
				if (m_ItemList[i].IsDisabled()) {
#if 0
					ForeColor = MixColor(ForeColor, m_Theme.ItemStyle.Fore.Fill.GetSolidColor());
#else
					Opacity = 128;
#endif
				}
			}

			RECT rcItem;
			rcItem.left = rc.left + m_Style.ItemPadding.Left;
			rcItem.top = rc.top + m_Style.ItemPadding.Top;
			rcItem.right = rcItem.left + m_Style.IconSize.Width;
			rcItem.bottom = rcItem.top + m_Style.IconSize.Height;

			bool fIconDrew = false;
			if (m_pEventHandler != nullptr) {
				DrawIconInfo Info;
				Info.Command = m_ItemList[i].Command;
				Info.State = m_ItemList[i].State;
				if (fHot)
					Info.State |= ItemState::Hot;
				Info.hdc = hdc;
				Info.IconRect = rcItem;
				Info.Color = ForeColor;
				Info.Opacity = Opacity;
				Info.hdcBuffer = hdcMemory;
				if (m_pEventHandler->DrawIcon(&Info))
					fIconDrew = true;
			}
			if (!fIconDrew && m_ItemList[i].Icon >= 0) {
				m_Icons.Draw(
					hdc, rcItem.left, rcItem.top,
					m_Style.IconSize.Width, m_Style.IconSize.Height,
					m_ItemList[i].Icon, ForeColor, Opacity);
			}
		}
	}

	::SelectObject(hdcMemory, hbmOld);
	::DeleteDC(hdcMemory);

	ThemeDraw.Draw(m_Theme.Border, rcClient);
}




CSideBar::CEventHandler::~CEventHandler()
{
	if (m_pSideBar != nullptr)
		m_pSideBar->SetEventHandler(nullptr);
}




void CSideBar::SideBarStyle::SetStyle(const Style::CStyleManager *pStyleManager)
{
	*this = SideBarStyle();
	pStyleManager->Get(TEXT("side-bar.item.icon"), &IconSize);
	pStyleManager->Get(TEXT("side-bar.item.padding"), &ItemPadding);
	pStyleManager->Get(TEXT("side-bar.separator.width"), &SeparatorWidth);
}


void CSideBar::SideBarStyle::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	pStyleScaling->ToPixels(&IconSize);
	pStyleScaling->ToPixels(&ItemPadding);
	pStyleScaling->ToPixels(&SeparatorWidth);
}


} // namespace TVTest
