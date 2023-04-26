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


#ifndef TVTEST_THEME_DRAW_H
#define TVTEST_THEME_DRAW_H


#include "Theme.h"


namespace TVTest
{

	namespace Theme
	{

		class CThemeDraw
		{
		public:
			CThemeDraw(
				const TVTest::Style::CStyleManager *pStyleManager,
				const TVTest::Style::CStyleScaling *pStyleScaling);

			bool Begin(HDC hdc);
			void End();
			bool Draw(const SolidStyle &Style, const RECT &Rect);
			bool Draw(const GradientStyle &Style, const RECT &Rect);
			bool Draw(const FillStyle &Style, const RECT &Rect);
			bool Draw(const BackgroundStyle &Style, const RECT &Rect);
			bool Draw(const BackgroundStyle &Style, RECT *pRect);
			bool Draw(const ForegroundStyle &Style, const RECT &Rect, LPCTSTR pszText, UINT Flags);
			bool Draw(const BorderStyle &Style, const RECT &Rect);
			bool Draw(const BorderStyle &Style, RECT *pRect);
			const TVTest::Style::CStyleManager *GetStyleManager() const { return m_pStyleManager; }
			const TVTest::Style::CStyleScaling *GetStyleScaling() const { return m_pStyleScaling; }

		private:
			const TVTest::Style::CStyleManager *m_pStyleManager;
			const TVTest::Style::CStyleScaling *m_pStyleScaling;
			TVTest::Style::CStyleScaling m_StyleScaling;
			HDC m_hdc = nullptr;
		};

	}	// namespace Theme

}	// namespace TVTest


#endif
