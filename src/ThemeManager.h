#ifndef TVTEST_THEME_MANAGER_H
#define TVTEST_THEME_MANAGER_H


#include "Theme.h"
#include "ColorScheme.h"


namespace TVTest
{

	namespace Theme
	{

		class CThemeManager
		{
		public:
			enum {
				STYLE_SCREEN,
				STYLE_STATUSBAR,
				STYLE_STATUSBAR_ITEM,
				STYLE_STATUSBAR_BOTTOMITEM,
				STYLE_STATUSBAR_ITEM_HOT,
				STYLE_STATUSBAR_EVENT_PROGRESS,
				STYLE_STATUSBAR_EVENT_PROGRESS_ELAPSED,
				STYLE_TITLEBAR,
				STYLE_TITLEBAR_CAPTION,
				STYLE_TITLEBAR_BUTTON,
				STYLE_TITLEBAR_BUTTON_HOT,
				STYLE_SIDEBAR,
				STYLE_SIDEBAR_ITEM,
				STYLE_SIDEBAR_ITEM_HOT,
				STYLE_SIDEBAR_ITEM_CHECKED,
				STYLE_PANEL_TAB,
				STYLE_PANEL_CURTAB,
				STYLE_PANEL_TABMARGIN,
				STYLE_PANEL_TITLE,
				STYLE_PROGRAMLISTPANEL_EVENT,
				STYLE_PROGRAMLISTPANEL_CUREVENT,
				STYLE_PROGRAMLISTPANEL_TITLE,
				STYLE_PROGRAMLISTPANEL_CURTITLE,
				STYLE_CHANNELPANEL_CHANNELNAME,
				STYLE_CHANNELPANEL_CURCHANNELNAME,
				STYLE_CHANNELPANEL_EVENTNAME1,
				STYLE_CHANNELPANEL_EVENTNAME2,
				STYLE_CHANNELPANEL_CURCHANNELEVENTNAME1,
				STYLE_CHANNELPANEL_CURCHANNELEVENTNAME2,
				STYLE_CONTROLPANEL_ITEM,
				STYLE_CONTROLPANEL_ITEM_HOT,
				STYLE_NOTIFICATIONBAR,
				STYLE_PROGRAMGUIDE_CHANNEL,
				STYLE_PROGRAMGUIDE_CURCHANNEL,
				STYLE_PROGRAMGUIDE_TIMEBAR,
				STYLE_PROGRAMGUIDE_TIMEBAR_0_2,
				STYLE_PROGRAMGUIDE_TIMEBAR_3_5,
				STYLE_PROGRAMGUIDE_TIMEBAR_6_8,
				STYLE_PROGRAMGUIDE_TIMEBAR_9_11,
				STYLE_PROGRAMGUIDE_TIMEBAR_12_14,
				STYLE_PROGRAMGUIDE_TIMEBAR_15_17,
				STYLE_PROGRAMGUIDE_TIMEBAR_18_20,
				STYLE_PROGRAMGUIDE_TIMEBAR_21_23,
				STYLE_PROGRAMGUIDE_STATUS,
				NUM_STYLES
			};

			CThemeManager(const CColorScheme *pColorScheme);
			ThemeColor GetColor(int Type) const;
			ThemeColor GetColor(LPCTSTR pszName) const;
			bool GetStyle(int Type,Style *pStyle) const;
			bool GetFillStyle(int Type,FillStyle *pStyle) const;
			bool GetBorderStyle(int Type,BorderStyle *pStyle) const;
			bool GetBackgroundStyle(int Type,BackgroundStyle *pStyle) const;
			bool GetForegroundStyle(int Type,ForegroundStyle *pStyle) const;

		private:
			struct StyleInfo
			{
				int Gradient;
				int Border;
				int ForeColor;
			};

			const CColorScheme *m_pColorScheme;

			static const StyleInfo m_StyleList[NUM_STYLES];
		};

	}	// namespace Theme

}	// namespace TVTest


#endif
