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


#ifndef TVTEST_UI_BASE_H
#define TVTEST_UI_BASE_H


#include "Style.h"
#include "ThemeManager.h"
#include "ThemeDraw.h"
#include <vector>


namespace TVTest
{

	class CUIBase
	{
	public:
		CUIBase() = default;
		virtual ~CUIBase() = 0;

		CUIBase(const CUIBase &) = delete;
		CUIBase &operator=(const CUIBase &) = delete;

		virtual void SetStyle(const Style::CStyleManager *pStyleManager);
		virtual void NormalizeStyle(
			const Style::CStyleManager *pStyleManager,
			const Style::CStyleScaling *pStyleScaling);
		virtual void SetTheme(const Theme::CThemeManager *pThemeManager);
		void SetStyleScaling(Style::CStyleScaling *pStyleScaling);
		Style::CStyleScaling *GetStyleScaling() { return m_pStyleScaling; }
		const Style::CStyleScaling *GetStyleScaling() const { return m_pStyleScaling; }
		void UpdateStyle();
		void RegisterUIChild(CUIBase *pChild);
		void RemoveUIChild(CUIBase *pChild);
		void ClearUIChildList();

		static void ResetDefaultFont();

	protected:
		void InitializeUI();
		const Style::CStyleManager *GetStyleManager() const;
		void ResetStyle();
		void UpdateChildrenStyle();
		virtual void ApplyStyle() {}
		virtual void RealizeStyle() {}
		virtual bool GetDefaultFont(Style::Font *pFont) const;
		bool GetSystemFont(DrawUtil::FontType Type, Style::Font *pFont) const;
		bool CreateDrawFont(const Style::Font &Font, DrawUtil::CFont *pDrawFont) const;
		bool CreateDrawFontAndBoldFont(
			const Style::Font &Font, DrawUtil::CFont *pDrawFont, DrawUtil::CFont *pBoldFont) const;
		bool CreateDefaultFont(DrawUtil::CFont *pDefaultFont) const;
		bool CreateDefaultFontAndBoldFont(DrawUtil::CFont *pDefaultFont, DrawUtil::CFont *pBoldFont) const;
		HCURSOR GetActionCursor() const;
		HCURSOR GetLinkCursor() const;
		Theme::CThemeDraw BeginThemeDraw(HDC hdc) const;
		bool ConvertBorderWidthsInPixels(Theme::BorderStyle *pStyle) const;
		bool GetBorderWidthsInPixels(const Theme::BorderStyle &Style, RECT *pWidths) const;
		int GetHairlineWidth() const;
		void InitStyleScaling(HWND hwnd, bool fNonClientScaling);
		void OnDPIChanged(HWND hwnd, WPARAM wParam, LPARAM lParam);

		Style::CStyleScaling *m_pStyleScaling = nullptr;
		std::vector<CUIBase*> m_UIChildList;

	private:
		static Style::Font m_DefaultFont;
		static bool m_fValidDefaultFont;
	};

}	// namespace TVTest


#endif	// ndef TVTEST_UI_BASE_H
