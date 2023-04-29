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
#include "UIBase.h"
#include "AppMain.h"
#include "StyleUtil.h"
#include "DPIUtil.h"
#include <algorithm>
#include "Common/DebugDef.h"


namespace TVTest
{


Style::Font CUIBase::m_DefaultFont;
bool CUIBase::m_fValidDefaultFont = false;


CUIBase::~CUIBase() = default;


void CUIBase::SetStyle(const Style::CStyleManager *pStyleManager)
{
}


void CUIBase::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
}


void CUIBase::SetTheme(const Theme::CThemeManager *pThemeManager)
{
}


void CUIBase::SetStyleScaling(Style::CStyleScaling *pStyleScaling)
{
	m_pStyleScaling = pStyleScaling;

	for (CUIBase *e : m_UIChildList)
		e->SetStyleScaling(pStyleScaling);
}


void CUIBase::UpdateStyle()
{
	ResetStyle();
	ApplyStyle();
	UpdateChildrenStyle();
	RealizeStyle();
}


void CUIBase::InitializeUI()
{
	ResetStyle();
	ApplyStyle();
}


const Style::CStyleManager *CUIBase::GetStyleManager() const
{
	return &GetAppClass().StyleManager;
}


void CUIBase::ResetStyle()
{
	const Style::CStyleManager *pStyleManager = GetStyleManager();

	SetStyle(pStyleManager);

	Style::CStyleScaling DefaultStyleScaling;
	if (m_pStyleScaling == nullptr)
		pStyleManager->InitStyleScaling(&DefaultStyleScaling);
	NormalizeStyle(pStyleManager, m_pStyleScaling != nullptr ? m_pStyleScaling : &DefaultStyleScaling);
}


void CUIBase::UpdateChildrenStyle()
{
	for (CUIBase *e : m_UIChildList)
		e->UpdateStyle();
}


bool CUIBase::GetDefaultFont(Style::Font *pFont) const
{
	if (pFont == nullptr)
		return false;

	if (!m_fValidDefaultFont) {
		if (!StyleUtil::GetDefaultUIFont(&m_DefaultFont))
			return false;
		m_fValidDefaultFont = true;
	}

	*pFont = m_DefaultFont;

	return true;
}


bool CUIBase::GetSystemFont(DrawUtil::FontType Type, Style::Font *pFont) const
{
	return StyleUtil::GetSystemFont(Type, pFont);
}


bool CUIBase::CreateDrawFont(const Style::Font &Font, DrawUtil::CFont *pDrawFont) const
{
	if (pDrawFont == nullptr)
		return false;

	Style::Font f = Font;

	if (m_pStyleScaling != nullptr) {
		m_pStyleScaling->RealizeFontSize(&f);
	} else {
		Style::CStyleScaling StyleScaling;
		GetStyleManager()->InitStyleScaling(&StyleScaling);
		StyleScaling.RealizeFontSize(&f);
	}

	return pDrawFont->Create(&f.LogFont);
}


bool CUIBase::CreateDrawFontAndBoldFont(
	const Style::Font &Font, DrawUtil::CFont *pDrawFont, DrawUtil::CFont *pBoldFont) const
{
	if (!CreateDrawFont(Font, pDrawFont))
		return false;

	if (pBoldFont != nullptr) {
		LOGFONT lf;

		if (!pDrawFont->GetLogFont(&lf))
			return false;

		lf.lfWeight = FW_BOLD;

		if (!pBoldFont->Create(&lf))
			return false;
	}

	return true;
}


bool CUIBase::CreateDefaultFont(DrawUtil::CFont *pDefaultFont) const
{
	Style::Font Font;

	if (!GetDefaultFont(&Font))
		return false;

	return CreateDrawFont(Font, pDefaultFont);
}


bool CUIBase::CreateDefaultFontAndBoldFont(DrawUtil::CFont *pDefaultFont, DrawUtil::CFont *pBoldFont) const
{
	if (!CreateDefaultFont(pDefaultFont))
		return false;

	if (pBoldFont != nullptr) {
		LOGFONT lf;

		if (!pDefaultFont->GetLogFont(&lf))
			return false;

		lf.lfWeight = FW_BOLD;

		if (!pBoldFont->Create(&lf))
			return false;
	}

	return true;
}


HCURSOR CUIBase::GetActionCursor() const
{
	return GetAppClass().UICore.GetActionCursor();
}


HCURSOR CUIBase::GetLinkCursor() const
{
	return GetAppClass().UICore.GetLinkCursor();
}


Theme::CThemeDraw CUIBase::BeginThemeDraw(HDC hdc) const
{
	Theme::CThemeDraw ThemeDraw(GetStyleManager(), m_pStyleScaling);
	ThemeDraw.Begin(hdc);
	return ThemeDraw;
}


bool CUIBase::ConvertBorderWidthsInPixels(Theme::BorderStyle *pStyle) const
{
	if (pStyle == nullptr)
		return false;

	const Style::CStyleScaling *pStyleScaling;
	Style::CStyleScaling StyleScaling;

	if (m_pStyleScaling != nullptr) {
		pStyleScaling = m_pStyleScaling;
	} else {
		GetStyleManager()->InitStyleScaling(&StyleScaling);
		pStyleScaling = &StyleScaling;
	}

	pStyleScaling->ToPixels(&pStyle->Width.Left);
	pStyleScaling->ToPixels(&pStyle->Width.Top);
	pStyleScaling->ToPixels(&pStyle->Width.Right);
	pStyleScaling->ToPixels(&pStyle->Width.Bottom);

	return true;
}


bool CUIBase::GetBorderWidthsInPixels(const Theme::BorderStyle &Style, RECT *pWidths) const
{
	Theme::BorderStyle Border = Style;

	ConvertBorderWidthsInPixels(&Border);
	return Theme::GetBorderWidths(Border, pWidths);
}


int CUIBase::GetHairlineWidth() const
{
	int Width;

	if (m_pStyleScaling != nullptr) {
		Width = m_pStyleScaling->ToPixels(1, Style::UnitType::LogicalPixel);
	} else {
		Style::CStyleScaling StyleScaling;
		GetStyleManager()->InitStyleScaling(&StyleScaling);
		Width = StyleScaling.ToPixels(1, Style::UnitType::LogicalPixel);
	}

	return std::max(Width, 1);
}


void CUIBase::RegisterUIChild(CUIBase *pChild)
{
	if (pChild == nullptr)
		return;
	if (std::ranges::find(m_UIChildList, pChild) == m_UIChildList.end())
		m_UIChildList.push_back(pChild);
}


void CUIBase::RemoveUIChild(CUIBase *pChild)
{
	auto it = std::ranges::find(m_UIChildList, pChild);
	if (it != m_UIChildList.end())
		m_UIChildList.erase(it);
}


void CUIBase::ClearUIChildList()
{
	m_UIChildList.clear();
}


void CUIBase::InitStyleScaling(HWND hwnd, bool fNonClientScaling)
{
	if (fNonClientScaling)
		EnableNonClientDPIScaling(hwnd);

	if (m_pStyleScaling != nullptr)
		GetStyleManager()->InitStyleScaling(m_pStyleScaling, hwnd);
}


void CUIBase::OnDPIChanged(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	TRACE(TEXT("DPI changed : {}x{}\n"), LOWORD(wParam), HIWORD(wParam));

	if (GetStyleManager()->IsHandleDPIChanged() && m_pStyleScaling != nullptr) {
		m_pStyleScaling->SetDPI(HIWORD(wParam));

		UpdateStyle();

		const RECT *prc = reinterpret_cast<RECT*>(lParam);
		::SetWindowPos(
			hwnd, nullptr,
			prc->left, prc->top,
			prc->right - prc->left, prc->bottom - prc->top,
			SWP_NOZORDER | SWP_NOACTIVATE);
	}
}


void CUIBase::ResetDefaultFont()
{
	m_fValidDefaultFont = false;
}


}	// namespace TVTest
