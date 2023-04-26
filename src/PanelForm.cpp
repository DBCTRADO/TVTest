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
#include "PanelForm.h"
#include "Common/DebugDef.h"


namespace TVTest
{


const LPCTSTR CPanelForm::m_pszClassName = APP_NAME TEXT(" Panel Form");
HINSTANCE CPanelForm::m_hinst = nullptr;


bool CPanelForm::Initialize(HINSTANCE hinst)
{
	if (m_hinst == nullptr) {
		WNDCLASS wc;

		wc.style = 0;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hinst;
		wc.hIcon = nullptr;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = nullptr;
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = m_pszClassName;
		if (::RegisterClass(&wc) == 0)
			return false;
		m_hinst = hinst;
	}
	return true;
}


CPanelForm::CPanelForm()
{
	m_WindowPosition.Width = 200;
	m_WindowPosition.Height = 240;

	GetDefaultFont(&m_StyleFont);
}


CPanelForm::~CPanelForm()
{
	Destroy();

	for (const auto &e : m_WindowList) {
		e->m_pWindow->OnFormDelete();
	}
}


bool CPanelForm::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	return CreateBasicWindow(hwndParent, Style, ExStyle, ID, m_pszClassName, TEXT("パネル"), m_hinst);
}


void CPanelForm::SetVisible(bool fVisible)
{
	if (m_pEventHandler != nullptr)
		m_pEventHandler->OnVisibleChange(fVisible);
	CBasicWindow::SetVisible(fVisible);
}


void CPanelForm::SetStyle(const Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);
}


void CPanelForm::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	m_Style.NormalizeStyle(pStyleManager, pStyleScaling);
}


void CPanelForm::SetTheme(const Theme::CThemeManager *pThemeManager)
{
	PanelFormTheme Theme;

	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_PANEL_TAB,
		&Theme.TabStyle);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_PANEL_CURTAB,
		&Theme.CurTabStyle);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_PANEL_TABMARGIN,
		&Theme.TabMarginStyle);
	Theme.BackColor =
		pThemeManager->GetColor(CColorScheme::COLOR_PANELBACK);
	Theme.BorderColor =
		pThemeManager->GetColor(CColorScheme::COLOR_PANELTABLINE);

	SetPanelFormTheme(Theme);
}


bool CPanelForm::AddPage(const PageInfo &Info)
{
	m_WindowList.emplace_back(std::make_unique<CWindowInfo>(Info));
	m_TabOrder.push_back(static_cast<int>(m_WindowList.size()) - 1);
	RegisterUIChild(Info.pPage);
	if (m_hwnd != nullptr) {
		CalcTabSize();
		Invalidate();
		UpdateTooltip();
	}
	return true;
}


CPanelForm::CPage *CPanelForm::GetPageByIndex(int Index)
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_WindowList.size())
		return nullptr;
	return m_WindowList[Index]->m_pWindow;
}


CPanelForm::CPage *CPanelForm::GetPageByID(int ID)
{
	const int Index = IDToIndex(ID);

	if (Index < 0)
		return nullptr;
	return m_WindowList[Index]->m_pWindow;
}


bool CPanelForm::SetCurTab(int Index)
{
	if (Index < -1 || static_cast<size_t>(Index) >= m_WindowList.size())
		return false;

	if (!m_WindowList[Index]->m_fVisible)
		return false;

	if (m_CurTab != Index) {
		if (m_CurTab >= 0) {
			CWindowInfo *pWindow = m_WindowList[m_CurTab].get();
			m_PrevActivePageID = pWindow->m_ID;
			pWindow->m_pWindow->OnDeactivate();
			pWindow->m_pWindow->SetVisible(false);
		}

		if (Index >= 0) {
			CWindowInfo *pWindow = m_WindowList[Index].get();
			RECT rc;

			GetClientRect(&rc);
			rc.top = m_TabHeight;
			Style::Subtract(&rc, m_Style.ClientMargin);
			pWindow->m_pWindow->SetPosition(&rc);
			pWindow->m_pWindow->OnActivate();
			pWindow->m_pWindow->SetVisible(true);
		}

		m_CurTab = Index;

		Invalidate();
		//Update();

		if (m_pEventHandler != nullptr)
			m_pEventHandler->OnSelChange();
	}

	return true;
}


int CPanelForm::IDToIndex(int ID) const
{
	for (int i = 0; i < static_cast<int>(m_WindowList.size()); i++) {
		if (m_WindowList[i]->m_ID == ID)
			return i;
	}
	return -1;
}


int CPanelForm::GetCurPageID() const
{
	if (m_CurTab < 0)
		return -1;
	return m_WindowList[m_CurTab]->m_ID;
}


bool CPanelForm::SetCurPageByID(int ID)
{
	const int Index = IDToIndex(ID);

	if (Index < 0)
		return false;
	return SetCurTab(Index);
}


bool CPanelForm::SetTabVisible(int ID, bool fVisible)
{
	const int Index = IDToIndex(ID);

	if (Index < 0)
		return false;

	CWindowInfo *pWindow = m_WindowList[Index].get();
	if (pWindow->m_fVisible != fVisible) {
		pWindow->m_fVisible = fVisible;
		pWindow->m_pWindow->OnVisibilityChanged(fVisible);

		if (!fVisible && m_CurTab == Index) {
			int CurTab = -1;
			if (m_PrevActivePageID >= 0) {
				const int i = IDToIndex(m_PrevActivePageID);
				if (i >= 0 && m_WindowList[i]->m_fVisible)
					CurTab = i;
			}
			if (CurTab < 0) {
				for (int i = 0; i < static_cast<int>(m_WindowList.size()); i++) {
					if (m_WindowList[i]->m_fVisible) {
						CurTab = i;
						break;
					}
				}
			}
			SetCurTab(CurTab);
		}

		if (m_hwnd != nullptr) {
			CalcTabSize();
			Invalidate();
			UpdateTooltip();
		}
	}

	return true;
}


bool CPanelForm::GetTabVisible(int ID) const
{
	const int Index = IDToIndex(ID);

	if (Index < 0)
		return false;
	return m_WindowList[Index]->m_fVisible;
}


bool CPanelForm::SetTabOrder(const int *pOrder, int Count)
{
	if (pOrder == nullptr || Count < 0)
		return false;

	std::vector<int> TabOrder;

	for (int i = 0; i < Count; i++) {
		size_t j;
		for (j = 0; j < m_WindowList.size(); j++) {
			if (m_WindowList[j]->m_ID == pOrder[i])
				break;
		}
		if (j == m_WindowList.size())
			return false;
		TabOrder.push_back(pOrder[i]);
	}

	m_TabOrder = TabOrder;

	if (m_hwnd != nullptr) {
		Invalidate();
		UpdateTooltip();
	}

	return true;
}


bool CPanelForm::GetTabInfo(int Index, TabInfo *pInfo) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_TabOrder.size() || pInfo == nullptr)
		return false;
	const CWindowInfo *pWindowInfo = m_WindowList[m_TabOrder[Index]].get();
	pInfo->ID = pWindowInfo->m_ID;
	pInfo->fVisible = pWindowInfo->m_fVisible;
	return true;
}


int CPanelForm::GetTabID(int Index) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_TabOrder.size())
		return -1;
	return m_WindowList[m_TabOrder[Index]]->m_ID;
}


bool CPanelForm::GetTabTitle(int ID, String *pTitle) const
{
	if (pTitle == nullptr)
		return false;

	const int Index = IDToIndex(ID);
	if (Index < 0) {
		pTitle->clear();
		return false;
	}

	*pTitle = m_WindowList[Index]->m_Title;

	return true;
}


void CPanelForm::SetEventHandler(CEventHandler *pHandler)
{
	m_pEventHandler = pHandler;
}


bool CPanelForm::SetPanelFormTheme(const PanelFormTheme &Theme)
{
	m_Theme = Theme;
	if (m_hwnd != nullptr)
		Invalidate();
	return true;
}


bool CPanelForm::GetPanelFormTheme(PanelFormTheme *pTheme) const
{
	if (pTheme == nullptr)
		return false;
	*pTheme = m_Theme;
	return true;
}


bool CPanelForm::SetTabFont(const Style::Font &Font)
{
	m_StyleFont = Font;
	if (m_hwnd != nullptr) {
		ApplyStyle();
		RealizeStyle();
	}
	return true;
}


bool CPanelForm::SetPageFont(const Style::Font &Font)
{
	for (auto &e : m_WindowList)
		e->m_pWindow->SetFont(Font);
	return true;
}


bool CPanelForm::GetPageClientRect(RECT *pRect) const
{
	if (pRect == nullptr)
		return false;
	if (!GetClientRect(pRect))
		return false;
	pRect->top = m_TabHeight;
	Style::Subtract(pRect, m_Style.ClientMargin);
	return true;
}


bool CPanelForm::SetTabStyle(TabStyle Style)
{
	if (m_TabStyle != Style) {
		m_TabStyle = Style;
		if (m_hwnd != nullptr) {
			CalcTabSize();
			SendSizeMessage();
			Invalidate();
		}
	}
	return true;
}


bool CPanelForm::SetIconImage(HBITMAP hbm, int Width, int Height)
{
	if (hbm == nullptr)
		return false;
	if (!m_Icons.Create(hbm, Width, Height))
		return false;
	if (m_hwnd != nullptr)
		Invalidate();
	return true;
}


SIZE CPanelForm::GetIconDrawSize() const
{
	SIZE sz;
	sz.cx = m_Style.TabIconSize.Width;
	sz.cy = m_Style.TabIconSize.Height;
	return sz;
}


bool CPanelForm::EnableTooltip(bool fEnable)
{
	if (m_fEnableTooltip != fEnable) {
		m_fEnableTooltip = fEnable;
		if (m_hwnd != nullptr)
			UpdateTooltip();
	}
	return true;
}


LRESULT CPanelForm::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			InitializeUI();

			m_Tooltip.Create(hwnd);
			m_Tooltip.SetFont(m_Font.GetHandle());
			UpdateTooltip();
		}
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd, &ps);
			Draw(ps.hdc, ps.rcPaint);
			::EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_SIZE:
		if (m_fFitTabWidth) {
			RECT rc;
			::SetRect(&rc, 0, 0, LOWORD(lParam), m_TabHeight);
			::InvalidateRect(hwnd, &rc, FALSE);
			UpdateTooltip();
		}

		if (m_CurTab >= 0) {
			RECT rc;
			::SetRect(&rc, 0, m_TabHeight, LOWORD(lParam), HIWORD(lParam));
			Style::Subtract(&rc, m_Style.ClientMargin);
			m_WindowList[m_CurTab]->m_pWindow->SetPosition(&rc);
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			const int Index = HitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

			if (Index >= 0) {
				if (Index != m_CurTab)
					SetCurTab(Index);

				const CPage *pPage = m_WindowList[Index]->m_pWindow;
				if (pPage->NeedKeyboardFocus())
					::SetFocus(pPage->GetHandle());
			}
		}
		return 0;

	case WM_RBUTTONUP:
		if (m_pEventHandler != nullptr) {
			const POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			RECT rc;

			GetClientRect(&rc);
			if (::PtInRect(&rc, pt)) {
				if (pt.y < m_TabHeight)
					m_pEventHandler->OnTabRButtonUp(pt.x, pt.y);
				else
					m_pEventHandler->OnRButtonUp(pt.x, pt.y);
				return 0;
			}
		}
		break;

	case WM_SETCURSOR:
		{
			POINT pt;

			::GetCursorPos(&pt);
			::ScreenToClient(hwnd, &pt);
			const int Index = HitTest(pt.x, pt.y);
			if (Index >= 0) {
				::SetCursor(GetActionCursor());
				return TRUE;
			}
		}
		break;

	case WM_KEYDOWN:
		if (m_pEventHandler != nullptr
				&& m_pEventHandler->OnKeyDown(static_cast<UINT>(wParam), static_cast<UINT>(lParam)))
			return 0;
		break;

	case WM_DESTROY:
		m_Tooltip.Destroy();
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


void CPanelForm::ApplyStyle()
{
	if (m_hwnd != nullptr) {
		CreateDrawFont(m_StyleFont, &m_Font);
		CalcTabSize();
	}
}


void CPanelForm::RealizeStyle()
{
	if (m_hwnd != nullptr) {
		SendSizeMessage();
		UpdateTooltip();
		Invalidate();
		m_Tooltip.SetFont(m_Font.GetHandle());
	}
}


void CPanelForm::CalcTabSize()
{
	const HDC hdc = ::GetDC(m_hwnd);
	const int IconHeight = m_Style.TabIconSize.Height + m_Style.TabIconMargin.Vert();
	const int LabelHeight = m_Font.GetHeight(hdc) + m_Style.TabLabelMargin.Vert();
	m_TabLineWidth = GetHairlineWidth();
	m_TabHeight = std::max(IconHeight, LabelHeight) + m_Style.TabPadding.Vert();
	const HFONT hfontOld = DrawUtil::SelectObject(hdc, m_Font);

	m_TabWidth = m_Style.TabPadding.Horz();

	if (m_TabStyle != TabStyle::IconOnly) {
		int MaxWidth = 0;

		for (const auto &Window : m_WindowList) {
			if (Window->m_fVisible) {
				SIZE sz;
				::GetTextExtentPoint32(hdc, Window->m_Title.data(), static_cast<int>(Window->m_Title.length()), &sz);
				if (sz.cx > MaxWidth)
					MaxWidth = sz.cx;
			}
		}
		m_TabWidth += MaxWidth + m_Style.TabLabelMargin.Horz();
	}

	if (m_TabStyle != TabStyle::TextOnly) {
		m_TabWidth += m_Style.TabIconSize.Width + m_Style.TabIconMargin.Horz();
		if (m_TabStyle == TabStyle::IconAndText)
			m_TabWidth += m_Style.TabIconLabelMargin;
	}

	SelectFont(hdc, hfontOld);
	::ReleaseDC(m_hwnd, hdc);
}


int CPanelForm::GetRealTabWidth() const
{
	if (m_fFitTabWidth) {
		int NumVisibleTabs = 0;
		for (const auto &Window : m_WindowList) {
			if (Window->m_fVisible)
				NumVisibleTabs++;
		}
		RECT rc;
		GetClientRect(&rc);
		if (NumVisibleTabs * m_TabWidth > rc.right) {
			const int Width = rc.right / NumVisibleTabs;
			int MinWidth = m_Style.TabPadding.Horz();
			if (m_TabStyle != TabStyle::TextOnly)
				MinWidth += m_Style.TabIconSize.Width;
			else
				MinWidth += 16;
			return std::max(Width, MinWidth);
		}
	}
	return m_TabWidth;
}


int CPanelForm::HitTest(int x, int y) const
{
	if (y < 0 || y >= m_TabHeight)
		return -1;

	const int TabWidth = GetRealTabWidth();
	const POINT pt = {x, y};
	RECT rc;

	::SetRect(&rc, 0, 0, TabWidth, m_TabHeight);
	for (const int Index : m_TabOrder) {
		if (m_WindowList[Index]->m_fVisible) {
			if (::PtInRect(&rc, pt))
				return Index;
			::OffsetRect(&rc, TabWidth, 0);
		}
	}
	return -1;
}


void CPanelForm::Draw(HDC hdc, const RECT &PaintRect)
{
	if (PaintRect.top < m_TabHeight) {
		Theme::CThemeDraw ThemeDraw(BeginThemeDraw(hdc));
		const int TabWidth = GetRealTabWidth();
		const COLORREF crOldTextColor = ::GetTextColor(hdc);
		const int OldBkMode = ::SetBkMode(hdc, TRANSPARENT);
		const HFONT hfontOld = DrawUtil::SelectObject(hdc, m_Font);
		const HBRUSH hbrOld = SelectBrush(hdc, ::GetStockObject(NULL_BRUSH));
		RECT rc = {0, 0, TabWidth, m_TabHeight};

		for (const int Index : m_TabOrder) {
			const CWindowInfo *pWindow = m_WindowList[Index].get();

			if (!pWindow->m_fVisible)
				continue;

			const bool fCur = Index == m_CurTab;
			const Theme::Style &Style = fCur ? m_Theme.CurTabStyle : m_Theme.TabStyle;
			RECT rcTab, rcContent, rcText;

			rcTab = rc;
			if (fCur) {
				RECT rcBorder;
				GetBorderWidthsInPixels(Style.Back.Border, &rcBorder);
				rcTab.bottom += rcBorder.bottom;
			}
			ThemeDraw.Draw(Style.Back, rcTab);
			if (!fCur) {
				RECT rcLine = rc;
				rcLine.top = rcLine.bottom - m_TabLineWidth;
				DrawUtil::Fill(hdc, &rcLine, m_Theme.BorderColor);
			}

			rcContent = rc;
			Style::Subtract(&rcContent, m_Style.TabPadding);
			rcText = rcContent;

			if (m_TabStyle != TabStyle::TextOnly) {
				RECT rcIcon = rcContent;
				Style::Subtract(&rcIcon, m_Style.TabIconMargin);
				int x = rcIcon.left;
				const int y = rcIcon.top + ((rcIcon.bottom - rcIcon.top) - m_Style.TabIconSize.Height) / 2;
				if (m_TabStyle == TabStyle::IconOnly)
					x += ((rcIcon.right - rcIcon.left) - m_Style.TabIconSize.Width) / 2;
				bool fIcon;
				if (pWindow->m_Icon >= 0) {
					m_Icons.Draw(
						hdc, x, y,
						m_Style.TabIconSize.Width,
						m_Style.TabIconSize.Height,
						pWindow->m_Icon,
						Style.Fore.Fill.GetSolidColor());
					fIcon = true;
				} else {
					fIcon = pWindow->m_pWindow->DrawIcon(
						hdc, x, y,
						m_Style.TabIconSize.Width,
						m_Style.TabIconSize.Height,
						Style.Fore.Fill.GetSolidColor());
				}
				if (fIcon) {
					rcText.left =
						rcIcon.left + m_Style.TabIconSize.Width +
						m_Style.TabIconMargin.Right + m_Style.TabIconLabelMargin;
				}
			}

			if (m_TabStyle != TabStyle::IconOnly) {
				Style::Subtract(&rcText, m_Style.TabLabelMargin);
				ThemeDraw.Draw(
					Style.Fore, rcText, pWindow->m_Title.c_str(),
					(m_TabStyle != TabStyle::TextOnly ? DT_LEFT : DT_CENTER)
						| DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
			}

			rc.left = rc.right;
			rc.right = rc.left + TabWidth;
		}

		SelectBrush(hdc, hbrOld);
		SelectFont(hdc, hfontOld);
		::SetBkMode(hdc, OldBkMode);
		::SetTextColor(hdc, crOldTextColor);

		if (PaintRect.right > rc.left) {
			if (PaintRect.left > rc.left)
				rc.left = PaintRect.left;
			rc.right = PaintRect.right;
			ThemeDraw.Draw(m_Theme.TabMarginStyle.Back, rc);
			rc.top = rc.bottom - m_TabLineWidth;
			DrawUtil::Fill(hdc, &rc, m_Theme.BorderColor);
		}
	}

	if (PaintRect.bottom > m_TabHeight) {
		const RECT rc = {
			PaintRect.left,
			std::max(PaintRect.top, static_cast<LONG>(m_TabHeight)),
			PaintRect.right,
			PaintRect.bottom
		};
		DrawUtil::Fill(hdc, &rc, m_Theme.BackColor);
	}
}


void CPanelForm::UpdateTooltip()
{
	const int TabWidth = GetRealTabWidth();

	if (!m_fEnableTooltip || !m_fFitTabWidth
			|| (m_TabStyle != TabStyle::IconOnly && TabWidth >= m_TabWidth)) {
		m_Tooltip.Enable(false);
		return;
	}

	const int ToolCount = m_Tooltip.NumTools();
	int TabCount = 0;
	RECT rc;

	rc.left = 0;
	rc.top = 0;
	rc.bottom = m_TabHeight;

	for (const int Index : m_TabOrder) {
		const CWindowInfo *pInfo = m_WindowList[Index].get();

		if (pInfo->m_fVisible) {
			rc.right = rc.left + TabWidth;
			if (TabCount < ToolCount) {
				m_Tooltip.SetToolRect(TabCount, rc);
				m_Tooltip.SetText(TabCount, pInfo->m_Title.c_str());
			} else {
				m_Tooltip.AddTool(TabCount, rc, pInfo->m_Title.c_str());
			}
			TabCount++;
			rc.left = rc.right;
		}
	}

	if (ToolCount > TabCount) {
		for (int i = ToolCount - 1; i >= TabCount; i--)
			m_Tooltip.DeleteTool(i);
	}

	m_Tooltip.Enable(true);
}




CPanelForm::CWindowInfo::CWindowInfo(const PageInfo &Info)
	: m_pWindow(Info.pPage)
	, m_Title(Info.pszTitle)
	, m_ID(Info.ID)
	, m_Icon(Info.Icon)
	, m_fVisible(Info.fVisible)
{
}




CPanelForm::CPage::~CPage() = default;


bool CPanelForm::CPage::CreateDefaultFont(DrawUtil::CFont *pFont)
{
	Style::Font Font;

	if (!GetDefaultFont(&Font))
		return false;
	return pFont->Create(&Font.LogFont);
}




void CPanelForm::PanelFormStyle::SetStyle(const Style::CStyleManager *pStyleManager)
{
	*this = PanelFormStyle();
	pStyleManager->Get(TEXT("panel.tab.padding"), &TabPadding);
	pStyleManager->Get(TEXT("panel.tab.icon"), &TabIconSize);
	pStyleManager->Get(TEXT("panel.tab.icon.margin"), &TabIconMargin);
	pStyleManager->Get(TEXT("panel.tab.label.margin"), &TabLabelMargin);
	pStyleManager->Get(TEXT("panel.tab.icon-label-margin"), &TabIconLabelMargin);
	pStyleManager->Get(TEXT("panel.client.margin"), &ClientMargin);
}


void CPanelForm::PanelFormStyle::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	pStyleScaling->ToPixels(&TabPadding);
	pStyleScaling->ToPixels(&TabIconSize);
	pStyleScaling->ToPixels(&TabIconMargin);
	pStyleScaling->ToPixels(&TabLabelMargin);
	pStyleScaling->ToPixels(&TabIconLabelMargin);
	pStyleScaling->ToPixels(&ClientMargin);
}


}	// namespace TVTest
