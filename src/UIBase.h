#ifndef TVTEST_UI_BASE_H
#define TVTEST_UI_BASE_H


#include "Style.h"
#include "ThemeManager.h"


namespace TVTest
{

	class CUIBase
	{
	public:
		virtual ~CUIBase() = 0;
		virtual void SetStyle(const Style::CStyleManager *pStyleManager);
		virtual void NormalizeStyle(const Style::CStyleManager *pStyleManager);
		virtual void SetTheme(const Theme::CThemeManager *pThemeManager);

	protected:
		void InitializeUI();
		const Style::CStyleManager *GetStyleManager() const;
		void UpdateStyle();
		HCURSOR GetActionCursor() const;
		HCURSOR GetLinkCursor() const;
	};

}	// namespace TVTest


#endif	// ndef TVTEST_UI_BASE_H
