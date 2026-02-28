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
				STYLE_WINDOW_FRAME,
				STYLE_WINDOW_ACTIVEFRAME,
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
				STYLE_PANEL_CONTENT,
				STYLE_INFORMATIONPANEL_EVENTINFO,
				STYLE_INFORMATIONPANEL_BUTTON,
				STYLE_INFORMATIONPANEL_BUTTON_HOT,
				STYLE_PROGRAMLISTPANEL_CHANNEL,
				STYLE_PROGRAMLISTPANEL_CURCHANNEL,
				STYLE_PROGRAMLISTPANEL_CHANNELBUTTON,
				STYLE_PROGRAMLISTPANEL_CHANNELBUTTON_HOT,
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
				STYLE_CHANNELPANEL_FEATUREDMARK,
				STYLE_CHANNELPANEL_PROGRESS,
				STYLE_CHANNELPANEL_CURPROGRESS,
				STYLE_CONTROLPANEL_ITEM,
				STYLE_CONTROLPANEL_ITEM_HOT,
				STYLE_CONTROLPANEL_ITEM_CHECKED,
				STYLE_NOTIFICATIONBAR,
				STYLE_PROGRAMGUIDE_FEATUREDMARK,
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
				STYLE_PROGRAMGUIDE_DATEBUTTON,
				STYLE_PROGRAMGUIDE_DATEBUTTON_CUR,
				STYLE_PROGRAMGUIDE_DATEBUTTON_HOT,
				STYLE_PROGRAMGUIDE_TIMEBUTTON,
				STYLE_PROGRAMGUIDE_TIMEBUTTON_CUR,
				STYLE_PROGRAMGUIDE_TIMEBUTTON_HOT,
				STYLE_PROGRAMGUIDE_FAVORITEBUTTON,
				STYLE_PROGRAMGUIDE_FAVORITEBUTTON_CUR,
				STYLE_PROGRAMGUIDE_FAVORITEBUTTON_HOT,
				NUM_STYLES
			};

			CThemeManager(const CColorScheme *pColorScheme);

			ThemeColor GetColor(int Type) const;
			ThemeColor GetColor(LPCTSTR pszName) const;
			bool GetStyle(int Type, Style *pStyle) const;
			bool GetFillStyle(int Type, FillStyle *pStyle) const;
			bool GetBorderStyle(int Type, BorderStyle *pStyle) const;
			bool GetBackgroundStyle(int Type, BackgroundStyle *pStyle) const;
			bool GetForegroundStyle(int Type, ForegroundStyle *pStyle) const;
			LPCTSTR GetStyleName(int Type) const;
			int ParseStyleName(LPCTSTR pszName) const;

		private:
			struct StyleInfo
			{
				LPCTSTR pszName;
				int Gradient;
				int Border;
				int ForeColor;
			};

			const CColorScheme *m_pColorScheme;

			static const StyleInfo m_StyleList[NUM_STYLES];
		};

	} // namespace Theme

} // namespace TVTest


#endif
