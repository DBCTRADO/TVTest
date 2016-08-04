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
			bool Draw(const SolidStyle &Style,const RECT &Rect);
			bool Draw(const GradientStyle &Style,const RECT &Rect);
			bool Draw(const FillStyle &Style,const RECT &Rect);
			bool Draw(const BackgroundStyle &Style,const RECT &Rect);
			bool Draw(const BackgroundStyle &Style,RECT *pRect);
			bool Draw(const ForegroundStyle &Style,const RECT &Rect,LPCTSTR pszText,UINT Flags);
			bool Draw(const BorderStyle &Style,const RECT &Rect);
			bool Draw(const BorderStyle &Style,RECT *pRect);
			const TVTest::Style::CStyleManager *GetStyleManager() const { return m_pStyleManager; }
			const TVTest::Style::CStyleScaling *GetStyleScaling() const { return m_pStyleScaling; }

		private:
			const TVTest::Style::CStyleManager *m_pStyleManager;
			const TVTest::Style::CStyleScaling *m_pStyleScaling;
			TVTest::Style::CStyleScaling m_StyleScaling;
			HDC m_hdc;
		};

	}	// namespace Theme

}	// namespace TVTest


#endif
