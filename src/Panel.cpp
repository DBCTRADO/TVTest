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
#include "Panel.h"
#include "DrawUtil.h"
#include "DPIUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace
{

const LPCTSTR PANEL_WINDOW_CLASS       = APP_NAME TEXT(" Panel");
const LPCTSTR PANEL_FRAME_WINDOW_CLASS = APP_NAME TEXT(" Panel Frame");
const LPCTSTR DROP_HELPER_WINDOW_CLASS = APP_NAME TEXT(" Drop Helper");

}




HINSTANCE CPanel::m_hinst = nullptr;


bool CPanel::Initialize(HINSTANCE hinst)
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
		wc.lpszClassName = PANEL_WINDOW_CLASS;
		if (::RegisterClass(&wc) == 0)
			return false;
		m_hinst = hinst;
	}
	return true;
}


CPanel::CPanel()
{
	GetSystemFont(DrawUtil::FontType::Caption, &m_StyleFont);
}


CPanel::~CPanel()
{
	Destroy();
}


bool CPanel::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	return CreateBasicWindow(
		hwndParent, Style, ExStyle, ID,
		PANEL_WINDOW_CLASS, nullptr, m_hinst);
}


void CPanel::SetStyle(const Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);
}


void CPanel::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	m_Style.NormalizeStyle(pStyleManager, pStyleScaling);
}


void CPanel::SetTheme(const Theme::CThemeManager *pThemeManager)
{
	PanelTheme Theme;

	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_PANEL_TITLE,
		&Theme.TitleStyle);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_TITLEBAR_BUTTON,
		&Theme.TitleIconStyle);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_TITLEBAR_BUTTON_HOT,
		&Theme.TitleIconHighlightStyle);

	SetPanelTheme(Theme);
}


bool CPanel::SetWindow(CPanelContent *pContent, LPCTSTR pszTitle)
{
	RECT rc;

	if (m_pContent != nullptr)
		RemoveUIChild(m_pContent);
	m_pContent = pContent;
	if (m_pContent != nullptr) {
		if (pContent->GetParent() != m_hwnd)
			pContent->SetParent(m_hwnd);
		RegisterUIChild(pContent);
		pContent->SetStyleScaling(m_pStyleScaling);
		pContent->UpdateStyle();
		pContent->SetVisible(true);
		StringUtility::Assign(m_Title, pszTitle);
		GetPosition(&rc);
		rc.right = rc.left + pContent->GetWidth();
		SetPosition(&rc);
	} else {
		m_Title.clear();
	}
	return true;
}


void CPanel::ShowTitle(bool fShow)
{
	if (m_fShowTitle != fShow) {
		RECT rc;

		m_fShowTitle = fShow;
		GetClientRect(&rc);
		OnSize(rc.right, rc.bottom);
	}
}


void CPanel::EnableFloating(bool fEnable)
{
	m_fEnableFloating = fEnable;
}


void CPanel::SetEventHandler(CEventHandler *pHandler)
{
	m_pEventHandler = pHandler;
}


bool CPanel::SetPanelTheme(const PanelTheme &Theme)
{
	m_Theme = Theme;

	if (m_hwnd != nullptr && m_fShowTitle) {
		const int OldTitleHeight = m_TitleHeight;
		RECT rc;

		CalcDimensions();
		if (m_TitleHeight != OldTitleHeight) {
			GetClientRect(&rc);
			OnSize(rc.right, rc.bottom);
		}
		GetTitleRect(&rc);
		Invalidate(&rc);
	}
	return true;
}


bool CPanel::GetPanelTheme(PanelTheme *pTheme) const
{
	if (pTheme == nullptr)
		return false;
	*pTheme = m_Theme;
	return true;
}


bool CPanel::GetTitleRect(RECT *pRect) const
{
	if (m_hwnd == nullptr)
		return false;

	GetClientRect(pRect);
	pRect->bottom = m_TitleHeight;
	return true;
}


bool CPanel::GetContentRect(RECT *pRect) const
{
	if (m_hwnd == nullptr)
		return false;

	GetClientRect(pRect);
	if (m_fShowTitle) {
		pRect->top = m_TitleHeight;
		if (pRect->bottom < pRect->top)
			pRect->bottom = pRect->top;
	}
	return true;
}


bool CPanel::SetTitleFont(const Style::Font &Font)
{
	m_StyleFont = Font;
	if (m_hwnd != nullptr) {
		ApplyStyle();
		RealizeStyle();
	}
	return true;
}


void CPanel::CalcDimensions()
{
	m_FontHeight = Style::GetFontHeight(
		m_hwnd, m_Font.GetHandle(), m_Style.TitleLabelExtraHeight);
	const int LabelHeight = m_FontHeight + m_Style.TitleLabelMargin.Vert();
	const int ButtonHeight =
		m_Style.TitleButtonIconSize.Height +
		m_Style.TitleButtonPadding.Vert();
	Theme::BorderStyle Border = m_Theme.TitleStyle.Back.Border;
	ConvertBorderWidthsInPixels(&Border);
	RECT rcBorder;
	Theme::GetBorderWidths(Border, &rcBorder);
	m_TitleHeight =
		std::max(LabelHeight, ButtonHeight) + m_Style.TitlePadding.Vert() +
		rcBorder.top + rcBorder.bottom;
}


void CPanel::Draw(HDC hdc, const RECT &PaintRect) const
{
	if (m_fShowTitle && PaintRect.top < m_TitleHeight) {
		Theme::CThemeDraw ThemeDraw(BeginThemeDraw(hdc));
		RECT rc;

		GetTitleRect(&rc);
		ThemeDraw.Draw(m_Theme.TitleStyle.Back, &rc);

		if (!m_Title.empty()) {
			Style::Subtract(&rc, m_Style.TitlePadding);
			rc.right -= m_Style.TitleButtonIconSize.Width + m_Style.TitleButtonPadding.Horz();
			Style::Subtract(&rc, m_Style.TitleLabelMargin);
			DrawUtil::DrawText(
				hdc, m_Title.c_str(), rc,
				DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX,
				&m_Font, m_Theme.TitleStyle.Fore.Fill.GetSolidColor());
		}

		GetCloseButtonRect(&rc);
		const Theme::Style &Style =
			m_HotItem == ItemType::Close ? m_Theme.TitleIconHighlightStyle : m_Theme.TitleIconStyle;
		if (Style.Back.Border.Type != Theme::BorderType::None
				|| Style.Back.Fill != m_Theme.TitleStyle.Back.Fill)
			ThemeDraw.Draw(Style.Back, rc);
		DrawUtil::DrawText(
			hdc, TEXT("r"), rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE,
			&m_IconFont, Style.Fore.Fill.GetSolidColor());
	}
}


void CPanel::OnSize(int Width, int Height)
{
	if (m_pContent != nullptr) {
		int y;

		if (m_fShowTitle)
			y = m_TitleHeight;
		else
			y = 0;
		m_pContent->SetPosition(0, y, Width, std::max(Height - y, 0));
	}
	if (m_pEventHandler != nullptr)
		m_pEventHandler->OnSizeChanged(Width, Height);
}


void CPanel::GetCloseButtonRect(RECT *pRect) const
{
	const int ButtonWidth = m_Style.TitleButtonIconSize.Width + m_Style.TitleButtonPadding.Horz();
	const int ButtonHeight = m_Style.TitleButtonIconSize.Height + m_Style.TitleButtonPadding.Vert();
	RECT rc;

	GetClientRect(&rc);
	Theme::BorderStyle Border = m_Theme.TitleStyle.Back.Border;
	ConvertBorderWidthsInPixels(&Border);
	Theme::SubtractBorderRect(Border, &rc);
	rc.right -= m_Style.TitlePadding.Right;
	rc.left = rc.right - ButtonWidth;
	rc.top =
		m_Style.TitlePadding.Top +
		(m_TitleHeight - m_Style.TitlePadding.Vert() - ButtonHeight) / 2;
	rc.bottom = rc.top + ButtonHeight;
	*pRect = rc;
}


void CPanel::SetHotItem(ItemType Item)
{
	if (m_HotItem != Item) {
		m_HotItem = Item;

		RECT rc;
		GetCloseButtonRect(&rc);
		Invalidate(&rc);
	}
}


LRESULT CPanel::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			InitializeUI();

			m_HotItem = ItemType::None;
			m_fCloseButtonPushed = false;
		}
		return 0;

	case WM_SIZE:
		OnSize(LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd, &ps);
			Draw(ps.hdc, ps.rcPaint);
			::EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

			if (m_fShowTitle && pt.y < m_TitleHeight) {
				RECT rc;

				GetCloseButtonRect(&rc);
				if (::PtInRect(&rc, pt)) {
					SetHotItem(ItemType::Close);
					m_fCloseButtonPushed = true;
					::SetCapture(hwnd);
				} else {
					SetHotItem(ItemType::None);
					if (m_fEnableFloating) {
						m_fCloseButtonPushed = false;
						::ClientToScreen(hwnd, &pt);
						m_ptDragStartPos = pt;
						::SetCapture(hwnd);
					}
				}
			}
		}
		return 0;

	case WM_LBUTTONUP:
		if (::GetCapture() == hwnd) {
			const bool fCloseButtonPushed = m_fCloseButtonPushed;

			::ReleaseCapture();
			if (fCloseButtonPushed) {
				const POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
				RECT rc;

				GetCloseButtonRect(&rc);
				if (::PtInRect(&rc, pt)) {
					if (m_pEventHandler != nullptr)
						m_pEventHandler->OnClose();
				}
			}
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

			RECT rc;
			GetCloseButtonRect(&rc);
			if (::PtInRect(&rc, pt))
				SetHotItem(ItemType::Close);
			else
				SetHotItem(ItemType::None);

			if (::GetCapture() == hwnd) {
				if (!m_fCloseButtonPushed) {
					::ClientToScreen(hwnd, &pt);
					if (std::abs(pt.x - m_ptDragStartPos.x) >= 4
							|| std::abs(pt.y - m_ptDragStartPos.y) >= 4) {
						::ReleaseCapture();
						if (m_pEventHandler != nullptr
								&& m_pEventHandler->OnFloating()) {
							::SendMessage(GetParent(), WM_NCLBUTTONDOWN, HTCAPTION, MAKELONG(pt.x, pt.y));
						}
					}
				}
			} else {
				TRACKMOUSEEVENT tme;
				tme.cbSize = sizeof(TRACKMOUSEEVENT);
				tme.dwFlags = TME_LEAVE;
				tme.hwndTrack = hwnd;
				::TrackMouseEvent(&tme);
			}
		}
		return 0;

	case WM_MOUSELEAVE:
		SetHotItem(ItemType::None);
		return 0;

	case WM_SETCURSOR:
		if (reinterpret_cast<HWND>(wParam) == hwnd && LOWORD(lParam) == HTCLIENT && m_HotItem != ItemType::None) {
			::SetCursor(GetActionCursor());
			return TRUE;
		}
		break;

	case WM_CAPTURECHANGED:
		if (m_fCloseButtonPushed) {
			SetHotItem(ItemType::None);
			m_fCloseButtonPushed = false;
		}
		return 0;

	case WM_RBUTTONUP:
		{
			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

			if (m_fShowTitle && pt.y < m_TitleHeight
					&& m_pEventHandler != nullptr) {
				const HMENU hmenu = ::CreatePopupMenu();

				::AppendMenu(hmenu, MF_STRING | MF_ENABLED, 1, TEXT("閉じる(&C)"));
				if (m_fEnableFloating)
					::AppendMenu(hmenu, MF_STRING | MF_ENABLED, 2, TEXT("切り離す(&F)"));
				m_pEventHandler->OnMenuPopup(hmenu);
				::ClientToScreen(hwnd, &pt);
				const int Command = ::TrackPopupMenu(
					hmenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, nullptr);
				switch (Command) {
				case 0:
					break;
				case 1:
					m_pEventHandler->OnClose();
					break;
				case 2:
					m_pEventHandler->OnFloating();
					break;
				default:
					m_pEventHandler->OnMenuSelected(Command);
					break;
				}
			}
		}
		return 0;

	case WM_KEYDOWN:
		if (m_pEventHandler != nullptr
				&& m_pEventHandler->OnKeyDown(static_cast<UINT>(wParam), static_cast<UINT>(lParam)))
			return 0;
		break;
	}

	return CCustomWindow::OnMessage(hwnd, uMsg, wParam, lParam);
}


void CPanel::ApplyStyle()
{
	if (m_hwnd != nullptr) {
		CreateDrawFont(m_StyleFont, &m_Font);

		LOGFONT lf = {};
		lf.lfHeight = -m_Style.TitleButtonIconSize.Height;
		lf.lfCharSet = SYMBOL_CHARSET;
		StringCopy(lf.lfFaceName, TEXT("Marlett"));
		m_IconFont.Create(&lf);

		CalcDimensions();
	}
}


void CPanel::RealizeStyle()
{
	if (m_hwnd != nullptr) {
		SendSizeMessage();
		Invalidate();
	}
}


void CPanel::PanelStyle::SetStyle(const Style::CStyleManager *pStyleManager)
{
	*this = PanelStyle();
	pStyleManager->Get(TEXT("panel.title.padding"), &TitlePadding);
	pStyleManager->Get(TEXT("panel.title.label.margin"), &TitleLabelMargin);
	pStyleManager->Get(TEXT("panel.title.label.extra-height"), &TitleLabelExtraHeight);
	pStyleManager->Get(TEXT("panel.title.button.icon"), &TitleButtonIconSize);
	pStyleManager->Get(TEXT("panel.title.button.padding"), &TitleButtonPadding);
}


void CPanel::PanelStyle::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	pStyleScaling->ToPixels(&TitlePadding);
	pStyleScaling->ToPixels(&TitleLabelMargin);
	pStyleScaling->ToPixels(&TitleLabelExtraHeight);
	pStyleScaling->ToPixels(&TitleButtonIconSize);
	pStyleScaling->ToPixels(&TitleButtonPadding);
}




HINSTANCE CPanelFrame::m_hinst = nullptr;


bool CPanelFrame::Initialize(HINSTANCE hinst)
{
	if (m_hinst == nullptr) {
		WNDCLASS wc;

		wc.style = 0;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hinst;
		wc.hIcon = nullptr;
		wc.hCursor = nullptr;
		wc.hbrBackground = nullptr;
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = PANEL_FRAME_WINDOW_CLASS;
		if (::RegisterClass(&wc) == 0)
			return false;
		m_hinst = hinst;
	}
	return CPanel::Initialize(hinst) && CDropHelper::Initialize(hinst);
}


CPanelFrame::CPanelFrame()
{
	m_WindowPosition.Left = 120;
	m_WindowPosition.Top = 120;
	m_WindowPosition.Width = 200;
	m_WindowPosition.Height = 240;

	SetStyleScaling(&m_StyleScaling);
}


CPanelFrame::~CPanelFrame()
{
	Destroy();
}


bool CPanelFrame::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	if (!CreateBasicWindow(
				hwndParent, Style, ExStyle, ID,
				PANEL_FRAME_WINDOW_CLASS, TEXT("パネル"), m_hinst))
		return false;
	if (m_Opacity < 255)
		SetOpacity(m_Opacity);
	return true;
}


bool CPanelFrame::Create(
	HWND hwndOwner, Layout::CSplitter *pSplitter, int PanelID,
	CPanelContent *pContent, LPCTSTR pszTitle)
{
	RECT rc;

	m_pSplitter = pSplitter;
	m_PanelID = PanelID;
	if (!Create(
				hwndOwner,
				WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_CLIPCHILDREN,
				WS_EX_TOOLWINDOW))
		return false;

	m_Panel.SetStyleScaling(
		m_fFloating ? m_pStyleScaling : m_pSplitter->GetStyleScaling());
	m_Panel.Create(m_hwnd, WS_CHILD | WS_CLIPCHILDREN);
	m_Panel.SetWindow(pContent, pszTitle);
	m_Panel.SetEventHandler(this);
	m_Panel.GetPosition(&rc);
	if (IsDockingVertical()) {
		if (m_DockingHeight < 0)
			m_DockingHeight = rc.bottom;
		if (m_DockingWidth < 0)
			m_DockingWidth = m_DockingHeight;
	} else {
		if (m_DockingWidth < 0)
			m_DockingWidth = rc.right;
		if (m_DockingHeight < 0)
			m_DockingHeight = m_DockingWidth;
	}

	if (m_fFloating) {
		RegisterUIChild(&m_Panel);
		m_Panel.SetParent(this);
		GetClientRect(&rc);
		m_Panel.SetPosition(&rc);
		m_Panel.ShowTitle(false);
		m_Panel.SetVisible(true);
	} else {
		Layout::CWindowContainer *pContainer =
			dynamic_cast<Layout::CWindowContainer*>(m_pSplitter->GetPaneByID(PanelID));

		pContainer->SetWindow(&m_Panel, &m_Panel);
		if (IsDockingVertical()) {
			m_pSplitter->SetPaneSize(PanelID, m_DockingHeight);
			rc.bottom = rc.top + m_DockingHeight;
		} else {
			m_pSplitter->SetPaneSize(PanelID, m_DockingWidth);
			rc.right = rc.left + m_DockingWidth;
		}
		m_Panel.SetPosition(&rc);
		m_Panel.ShowTitle(true);
	}

	return true;
}


bool CPanelFrame::SetFloating(bool fFloating)
{
	if (m_fFloating != fFloating) {
		if (m_hwnd != nullptr) {
			Layout::CWindowContainer *pContainer =
				dynamic_cast<Layout::CWindowContainer*>(m_pSplitter->GetPaneByID(m_PanelID));
			RECT rc;

			m_fFloatingTransition = true;
			if (fFloating) {
				pContainer->SetVisible(false);
				pContainer->SetWindow(nullptr);
				RegisterUIChild(&m_Panel);
				m_Panel.SetParent(this);
				m_Panel.SetStyleScaling(m_pStyleScaling);
				m_Panel.UpdateStyle();
				m_Panel.SetVisible(true);
				GetClientRect(&rc);
				m_Panel.SetPosition(&rc);
				m_Panel.ShowTitle(false);
				SetVisible(true);
			} else {
				/*
				m_Panel.GetContentRect(&rc);
				if (IsDockingVertical())
					m_DockingHeight = (rc.bottom - rc.top) + m_Panel.GetTitleHeight();
				else
					m_DockingWidth = rc.right - rc.left;
				*/
				RemoveUIChild(&m_Panel);
				SetVisible(false);
				m_Panel.SetVisible(false);
				m_Panel.ShowTitle(true);
				pContainer->SetWindow(&m_Panel, &m_Panel);
				m_Panel.SetStyleScaling(m_pSplitter->GetStyleScaling());
				m_Panel.UpdateStyle();
				m_pSplitter->SetPaneSize(
					m_PanelID, IsDockingVertical() ? m_DockingHeight : m_DockingWidth);
				pContainer->SetVisible(true);
			}
			m_fFloatingTransition = false;
		}

		m_fFloating = fFloating;
	}

	return true;
}


bool CPanelFrame::SetDockingWidth(int Width)
{
	m_DockingWidth = Width;
	return true;
}


bool CPanelFrame::SetDockingHeight(int Height)
{
	m_DockingHeight = Height;
	return true;
}


bool CPanelFrame::SetDockingPlace(DockingPlace Place)
{
	if (m_pEventHandler != nullptr)
		m_pEventHandler->OnDocking(Place);

	return true;
}


void CPanelFrame::SetEventHandler(CEventHandler *pHandler)
{
	m_pEventHandler = pHandler;
}


bool CPanelFrame::SetPanelVisible(bool fVisible, bool fNoActivate)
{
	if (m_hwnd == nullptr)
		return false;
	if (m_pEventHandler != nullptr)
		m_pEventHandler->OnVisibleChange(fVisible);
	if (m_fFloating) {
		if (fVisible) {
			SendSizeMessage();
			m_Panel.SetVisible(true);
			m_Panel.Invalidate();
		}
		if (fVisible && fNoActivate)
			::ShowWindow(m_hwnd, SW_SHOWNA);
		else
			SetVisible(fVisible);
	} else {
		Layout::CWindowContainer *pContainer =
			dynamic_cast<Layout::CWindowContainer*>(m_pSplitter->GetPaneByID(m_PanelID));

		pContainer->SetVisible(fVisible);
	}
	return true;
}


bool CPanelFrame::IsDockingVertical() const
{
	return !!(m_pSplitter->GetStyle() & Layout::CSplitter::StyleFlag::Vert);
}


bool CPanelFrame::SetPanelTheme(const CPanel::PanelTheme &Theme)
{
	return m_Panel.SetPanelTheme(Theme);
}


bool CPanelFrame::GetPanelTheme(CPanel::PanelTheme *pTheme) const
{
	return m_Panel.GetPanelTheme(pTheme);
}


bool CPanelFrame::SetPanelOpacity(int Opacity)
{
	if (Opacity < 0 || Opacity > 255)
		return false;
	if (Opacity != m_Opacity) {
		if (m_hwnd != nullptr) {
			if (!SetOpacity(Opacity))
				return false;
		}
		m_Opacity = Opacity;
	}
	return true;
}


void CPanelFrame::SetTheme(const Theme::CThemeManager *pThemeManager)
{
	m_Panel.SetTheme(pThemeManager);
}


LRESULT CPanelFrame::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			m_fDragMoving = false;
			const HMENU hmenu = GetSystemMenu(hwnd, FALSE);
			InsertMenu(hmenu, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, SC_DOCKING, TEXT("ドッキング(&D)"));
			InsertMenu(hmenu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
		}
		return 0;

	case WM_NCCREATE:
		InitStyleScaling(hwnd, true);
		break;

	case WM_SIZE:
		if (m_fFloating)
			m_Panel.SetPosition(0, 0, LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_KEYDOWN:
		if (m_pEventHandler != nullptr
				&& m_pEventHandler->OnKeyDown(static_cast<UINT>(wParam), static_cast<UINT>(lParam)))
			return 0;
		break;

	case WM_MOUSEWHEEL:
		if (m_pEventHandler != nullptr
				&& m_pEventHandler->OnMouseWheel(wParam, lParam))
			return 0;
		break;

	case WM_ACTIVATE:
		if (m_pEventHandler != nullptr
				&& m_pEventHandler->OnActivate(LOWORD(wParam) != WA_INACTIVE))
			return 0;
		break;

	case WM_MOVING:
		if (m_pEventHandler != nullptr
				&& m_pEventHandler->OnMoving(reinterpret_cast<LPRECT>(lParam)))
			return TRUE;
		break;

	case WM_MOVE:
		if (m_fDragMoving) {
			const int Margin = m_pStyleScaling->GetScaledSystemMetrics(SM_CYCAPTION);
			POINT pt;
			RECT rcTarget, rc;
			DockingPlace Target = DockingPlace::None;

			::GetCursorPos(&pt);
			m_pSplitter->GetLayoutBase()->GetScreenPosition(&rcTarget);
			rc = rcTarget;
			rc.right = rc.left + Margin;
			if (::PtInRect(&rc, pt)) {
				Target = DockingPlace::Left;
			} else {
				rc.right = rcTarget.right;
				rc.left = rc.right - Margin;
				if (::PtInRect(&rc, pt)) {
					Target = DockingPlace::Right;
				} else {
					rc.left = rcTarget.left;
					rc.right = rcTarget.right;
					rc.top = rcTarget.top;
					rc.bottom = rc.top + Margin;
					if (::PtInRect(&rc, pt)) {
						Target = DockingPlace::Top;
					} else {
						rc.bottom = rcTarget.bottom;
						rc.top = rc.bottom - Margin;
						if (::PtInRect(&rc, pt))
							Target = DockingPlace::Bottom;
					}
				}
			}
			if (Target != m_DragDockingTarget) {
				if (Target == DockingPlace::None) {
					m_DropHelper.Hide();
				} else {
					switch (Target) {
					case DockingPlace::Left:
						rc.right = rcTarget.left;
						rc.left = rc.right - m_DockingWidth;
						break;
					case DockingPlace::Right:
						rc.left = rcTarget.right;
						rc.right = rc.left + m_DockingWidth;
						break;
					case DockingPlace::Top:
						rc.bottom = rcTarget.top;
						rc.top = rc.bottom - m_DockingHeight;
						break;
					case DockingPlace::Bottom:
						rc.top = rcTarget.bottom;
						rc.bottom = rc.top + m_DockingHeight;
						break;
					}
					m_DropHelper.Show(&rc);
				}
				m_DragDockingTarget = Target;
			}
		}
		break;

	case WM_EXITSIZEMOVE:
		if (m_DragDockingTarget != DockingPlace::None) {
			m_DropHelper.Hide();
			if (m_pEventHandler != nullptr)
				m_pEventHandler->OnDocking(m_DragDockingTarget);
			::SendMessage(hwnd, WM_SYSCOMMAND, SC_DOCKING, 0);
		}
		m_fDragMoving = false;
		return 0;

	case WM_ENTERSIZEMOVE:
		m_DragDockingTarget = DockingPlace::None;
		m_fDragMoving = true;
		if (m_pEventHandler != nullptr
				&& m_pEventHandler->OnEnterSizeMove())
			return 0;
		break;

	case WM_SYSCOMMAND:
		switch (wParam) {
		case SC_DOCKING:
			if (m_pEventHandler != nullptr
					&& !m_pEventHandler->OnFloatingChange(false))
				return 0;
			SetFloating(false);
			return 0;
		}
		break;

	case WM_DPICHANGED:
		OnDPIChanged(hwnd, wParam, lParam);
		break;

	case WM_CLOSE:
		if (m_pEventHandler != nullptr
				&& !m_pEventHandler->OnClose())
			return 0;
		break;
	}

	return CCustomWindow::OnMessage(hwnd, uMsg, wParam, lParam);
}


bool CPanelFrame::OnFloating()
{
	if (m_fFloating)
		return false;

	RECT rc;
	m_Panel.GetContentRect(&rc);
	MapWindowRect(m_Panel.GetHandle(), nullptr, &rc);
	if (m_pEventHandler != nullptr && !m_pEventHandler->OnFloatingChange(true))
		return false;
	m_pStyleScaling->AdjustWindowRect(m_hwnd, &rc);
	SetPosition(&rc);
	SetFloating(true);
	return true;
}


bool CPanelFrame::OnClose()
{
	return m_pEventHandler == nullptr || m_pEventHandler->OnClose();
}


bool CPanelFrame::OnMoving(RECT *pRect)
{
	return m_pEventHandler != nullptr && m_pEventHandler->OnMoving(pRect);
}


bool CPanelFrame::OnEnterSizeMove()
{
	return m_pEventHandler != nullptr && m_pEventHandler->OnEnterSizeMove();
}


bool CPanelFrame::OnKeyDown(UINT KeyCode, UINT Flags)
{
	return m_pEventHandler != nullptr && m_pEventHandler->OnKeyDown(KeyCode, Flags);
}


void CPanelFrame::OnSizeChanged(int Width, int Height)
{
	if (!m_fFloating && !m_fFloatingTransition) {
		if (IsDockingVertical())
			m_DockingHeight = Height;
		else
			m_DockingWidth = Width;
	}
}


enum {
	PANEL_MENU_LEFT = CPanel::MENU_USER,
	PANEL_MENU_RIGHT,
	PANEL_MENU_TOP,
	PANEL_MENU_BOTTOM
};

bool CPanelFrame::OnMenuPopup(HMENU hmenu)
{
	::AppendMenu(hmenu, MF_SEPARATOR, 0, nullptr);
	::AppendMenu(hmenu, MF_STRING | MF_ENABLED, PANEL_MENU_LEFT, TEXT("左へ(&L)"));
	::AppendMenu(hmenu, MF_STRING | MF_ENABLED, PANEL_MENU_RIGHT, TEXT("右へ(&R)"));
	::AppendMenu(hmenu, MF_STRING | MF_ENABLED, PANEL_MENU_TOP, TEXT("上へ(&T)"));
	::AppendMenu(hmenu, MF_STRING | MF_ENABLED, PANEL_MENU_BOTTOM, TEXT("下へ(&B)"));
	const int Index = m_pSplitter->IDToIndex(m_PanelID);
	::EnableMenuItem(
		hmenu,
		IsDockingVertical() ?
			(Index == 0 ? PANEL_MENU_TOP : PANEL_MENU_BOTTOM) :
			(Index == 0 ? PANEL_MENU_LEFT : PANEL_MENU_RIGHT),
		MF_BYCOMMAND | MF_GRAYED);
	return true;
}


bool CPanelFrame::OnMenuSelected(int Command)
{
	switch (Command) {
	case PANEL_MENU_LEFT:
		SetDockingPlace(DockingPlace::Left);
		return true;
	case PANEL_MENU_RIGHT:
		SetDockingPlace(DockingPlace::Right);
		return true;
	case PANEL_MENU_TOP:
		SetDockingPlace(DockingPlace::Top);
		return true;
	case PANEL_MENU_BOTTOM:
		SetDockingPlace(DockingPlace::Bottom);
		return true;
	}

	return false;
}




HINSTANCE CDropHelper::m_hinst = nullptr;


bool CDropHelper::Initialize(HINSTANCE hinst)
{
	if (m_hinst == nullptr) {
		WNDCLASS wc;

		wc.style = 0;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hinst;
		wc.hIcon = nullptr;
		wc.hCursor = nullptr;
		wc.hbrBackground = nullptr;
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = DROP_HELPER_WINDOW_CLASS;
		if (::RegisterClass(&wc) == 0)
			return false;
		m_hinst = hinst;
	}
	return true;
}


CDropHelper::~CDropHelper()
{
	Destroy();
}


bool CDropHelper::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	return CreateBasicWindow(
		hwndParent, Style, ExStyle, ID,
		DROP_HELPER_WINDOW_CLASS, TEXT(""), m_hinst);
}


bool CDropHelper::Show(const RECT *pRect)
{
	if (m_hwnd == nullptr) {
		if (!Create(
					nullptr, WS_POPUP,
					WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE))
			return false;
		::SetLayeredWindowAttributes(m_hwnd, CLR_INVALID, m_Opacity, LWA_ALPHA);
	}
	SetPosition(pRect);
	::ShowWindow(m_hwnd, SW_SHOWNA);
	return true;
}


bool CDropHelper::Hide()
{
	if (m_hwnd != nullptr)
		SetVisible(false);
	return true;
}


LRESULT CDropHelper::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd, &ps);
			::FillRect(ps.hdc, &ps.rcPaint, static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
			::EndPaint(hwnd, &ps);
		}
		return 0;
	}

	return CCustomWindow::OnMessage(hwnd, uMsg, wParam, lParam);
}


}	// namespace TVTest
