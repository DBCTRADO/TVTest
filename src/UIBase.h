#ifndef TVTEST_UI_BASE_H
#define TVTEST_UI_BASE_H


#include "Style.h"
#include "ThemeManager.h"
#include "ThemeDraw.h"


namespace TVTest
{

	class CUIBase
	{
	public:
		virtual ~CUIBase() = 0;
		virtual void SetStyle(const Style::CStyleManager *pStyleManager);
		virtual void NormalizeStyle(const Style::CStyleManager *pStyleManager);
		virtual void SetTheme(const Theme::CThemeManager *pThemeManager);

		static void ResetDefaultFont();

	protected:
		void InitializeUI();
		const Style::CStyleManager *GetStyleManager() const;
		void UpdateStyle();
		virtual bool GetDefaultFont(Style::Font *pFont) const;
		bool GetSystemFont(DrawUtil::FontType Type,Style::Font *pFont) const;
		bool CreateDrawFont(const Style::Font &Font,DrawUtil::CFont *pDrawFont) const;
		bool CreateDefaultFont(DrawUtil::CFont *pDefaultFont) const;
		bool CreateDefaultFontAndBoldFont(DrawUtil::CFont *pDefaultFont,DrawUtil::CFont *pBoldFont) const;
		HCURSOR GetActionCursor() const;
		HCURSOR GetLinkCursor() const;
		Theme::CThemeDraw BeginThemeDraw(HDC hdc) const {
			Theme::CThemeDraw ThemeDraw(GetStyleManager());
			ThemeDraw.Begin(hdc);
			return ThemeDraw;
		}
		bool ConvertBorderWidthsInPixels(Theme::BorderStyle *pStyle) const;
		bool GetBorderWidthsInPixels(const Theme::BorderStyle &Style,RECT *pWidths) const;
		int GetHairlineWidth() const;

	private:
		static Style::Font m_DefaultFont;
		static bool m_fValidDefaultFont;
	};

}	// namespace TVTest


#endif	// ndef TVTEST_UI_BASE_H
