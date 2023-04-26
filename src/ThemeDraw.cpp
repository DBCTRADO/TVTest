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
#include "ThemeDraw.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace Theme
{


CThemeDraw::CThemeDraw(
	const TVTest::Style::CStyleManager *pStyleManager,
	const TVTest::Style::CStyleScaling *pStyleScaling)
	: m_pStyleManager(pStyleManager)
	, m_pStyleScaling(pStyleScaling)
{
	if (m_pStyleScaling == nullptr) {
		m_pStyleManager->InitStyleScaling(&m_StyleScaling);
		m_pStyleScaling = &m_StyleScaling;
	}
}


bool CThemeDraw::Begin(HDC hdc)
{
	m_hdc = hdc;
	return hdc != nullptr;
}


void CThemeDraw::End()
{
	m_hdc = nullptr;
}


bool CThemeDraw::Draw(const SolidStyle &Style, const RECT &Rect)
{
	if (m_hdc == nullptr)
		return false;
	return Theme::Draw(m_hdc, Rect, Style);
}


bool CThemeDraw::Draw(const GradientStyle &Style, const RECT &Rect)
{
	if (m_hdc == nullptr)
		return false;
	return Theme::Draw(m_hdc, Rect, Style);
}


bool CThemeDraw::Draw(const FillStyle &Style, const RECT &Rect)
{
	if (m_hdc == nullptr)
		return false;
	return Theme::Draw(m_hdc, Rect, Style);
}


bool CThemeDraw::Draw(const BackgroundStyle &Style, const RECT &Rect)
{
	RECT rc = Rect;
	return Draw(Style, &rc);
}


bool CThemeDraw::Draw(const BackgroundStyle &Style, RECT *pRect)
{
	if (m_hdc == nullptr || pRect == nullptr)
		return false;
	if (Style.Border.Type != BorderType::None)
		Draw(Style.Border, pRect);
	return Theme::Draw(m_hdc, *pRect, Style.Fill);
}


bool CThemeDraw::Draw(const ForegroundStyle &Style, const RECT &Rect, LPCTSTR pszText, UINT Flags)
{
	if (m_hdc == nullptr)
		return false;
	return Theme::Draw(m_hdc, Rect, Style, pszText, Flags);
}


bool CThemeDraw::Draw(const BorderStyle &Style, const RECT &Rect)
{
	RECT rc = Rect;
	return Draw(Style, &rc);
}


bool CThemeDraw::Draw(const BorderStyle &Style, RECT *pRect)
{
	if (m_hdc == nullptr)
		return false;

	BorderStyle DrawStyle = Style;

	m_pStyleScaling->ToPixels(&DrawStyle.Width.Left);
	m_pStyleScaling->ToPixels(&DrawStyle.Width.Top);
	m_pStyleScaling->ToPixels(&DrawStyle.Width.Right);
	m_pStyleScaling->ToPixels(&DrawStyle.Width.Bottom);

	return Theme::Draw(m_hdc, pRect, DrawStyle);
}


}	// namespace Theme

}	// namespace TVTest
