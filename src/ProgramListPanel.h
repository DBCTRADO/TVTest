/*
  TVTest
  Copyright(c) 2008-2019 DBCTRADO

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


#ifndef TVTEST_PROGRAM_LIST_PANEL_H
#define TVTEST_PROGRAM_LIST_PANEL_H


#include "PanelForm.h"
#include "UIBase.h"
#include "EpgUtil.h"
#include "ChannelList.h"
#include "Theme.h"
#include "DrawUtil.h"
#include "Menu.h"
#include "EventInfoPopup.h"
#include "WindowUtil.h"
#include "FeaturedEvents.h"
#include "LibISDB/LibISDB/EPG/EPGDatabase.hpp"


namespace TVTest
{

	class CProgramItemInfo;

	class CProgramItemList
	{
		std::vector<std::unique_ptr<CProgramItemInfo>> m_ItemList;

	public:
		int NumItems() const { return (int)m_ItemList.size(); }
		CProgramItemInfo *GetItem(int Index);
		const CProgramItemInfo *GetItem(int Index) const;
		bool Add(CProgramItemInfo *pItem);
		void Clear();
		void Attach(CProgramItemList *pList);
	};

	class CProgramListPanel
		: public CPanelForm::CPage
		, public CSettingsBase
		, protected CFeaturedEvents::CEventHandler
	{
	public:
		struct ProgramListPanelTheme
		{
			Theme::Style ChannelNameStyle;
			Theme::Style CurChannelNameStyle;
			Theme::Style ChannelButtonStyle;
			Theme::Style ChannelButtonHotStyle;
			Theme::Style EventNameStyle;
			Theme::Style CurEventNameStyle;
			Theme::Style EventTextStyle;
			Theme::Style CurEventTextStyle;
			Theme::ThemeColor MarginColor;
			Theme::BackgroundStyle FeaturedMarkStyle;
		};

		static bool Initialize(HINSTANCE hinst);

		CProgramListPanel();
		~CProgramListPanel();

	// CBasicWindow
		bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;

	// CUIBase
		void SetStyle(const Style::CStyleManager *pStyleManager) override;
		void NormalizeStyle(
			const Style::CStyleManager *pStyleManager,
			const Style::CStyleScaling *pStyleScaling) override;
		void SetTheme(const Theme::CThemeManager *pThemeManager) override;

	// CPage
		bool SetFont(const Style::Font &Font) override;

	// CSettingsBase
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	// CProgramListPanel
		void SetEPGDatabase(LibISDB::EPGDatabase *pEPGDatabase) { m_pEPGDatabase = pEPGDatabase; }
		bool UpdateProgramList(const CChannelInfo *pChannelInfo);
		void ClearProgramList();
		void SelectChannel(const CChannelInfo *pChannelInfo, bool fUpdate = true);
		void SetCurrentChannel(const CChannelInfo *pChannelInfo);
		void SetCurrentEventID(int EventID);
		bool SetProgramListPanelTheme(const ProgramListPanelTheme &Theme);
		bool GetProgramListPanelTheme(ProgramListPanelTheme *pTheme) const;
		bool SetEventInfoFont(const Style::Font &Font);
		void ShowRetrievingMessage(bool fShow);
		void SetVisibleEventIcons(UINT VisibleIcons);
		void SetMouseOverEventInfo(bool fMouseOverEventInfo);
		bool GetMouseOverEventInfo() const { return m_fMouseOverEventInfo; }
		void SetUseEpgColorScheme(bool fUseEpgColorScheme);
		bool GetUseEpgColorScheme() const { return m_fUseEpgColorScheme; }
		void SetShowFeaturedMark(bool fShowFeaturedMark);
		bool GetShowFeaturedMark() const { return m_fShowFeaturedMark; }

	private:
		struct ProgramListPanelStyle
		{
			Style::Margins ChannelPadding;
			Style::Margins ChannelLogoMargin;
			Style::Margins ChannelNameMargin;
			Style::Size ChannelButtonIconSize;
			Style::Margins ChannelButtonPadding;
			Style::IntValue ChannelButtonMargin;
			Style::Margins TitlePadding;
			Style::Size IconSize;
			Style::Margins IconMargin;
			Style::IntValue LineSpacing;
			Style::Size FeaturedMarkSize;
			Style::Margins FeaturedMarkMargin;

			ProgramListPanelStyle();

			void SetStyle(const Style::CStyleManager *pStyleManager);
			void NormalizeStyle(
				const Style::CStyleManager *pStyleManager,
				const Style::CStyleScaling *pStyleScaling);
		};

		enum {
			ITEM_CHANNEL,
			ITEM_CHANNELLISTBUTTON
		};

		LibISDB::EPGDatabase *m_pEPGDatabase;
		Style::Font m_StyleFont;
		DrawUtil::CFont m_Font;
		DrawUtil::CFont m_TitleFont;
		DrawUtil::CFont m_IconFont;
		int m_FontHeight;
		ProgramListPanelStyle m_Style;
		ProgramListPanelTheme m_Theme;
		CEpgTheme m_EpgTheme;
		bool m_fMouseOverEventInfo;
		bool m_fUseEpgColorScheme;
		bool m_fShowFeaturedMark;
		CEpgIcons m_EpgIcons;
		UINT m_VisibleEventIcons;
		int m_ChannelHeight;
		int m_TotalLines;
		CProgramItemList m_ItemList;
		CChannelInfo m_SelectedChannel;
		CChannelInfo m_CurChannel;
		int m_CurEventID;
		int m_ScrollPos;
		int m_OldDPI;
		int m_HotItem;
		CChannelMenu m_ChannelMenu;
		CMouseWheelHandler m_MouseWheel;
		//HWND m_hwndToolTip;
		CEventInfoPopup m_EventInfoPopup;
		CEventInfoPopupManager m_EventInfoPopupManager;
		class CEventInfoPopupHandler
			: public CEventInfoPopupManager::CEventHandler
		{
			CProgramListPanel *m_pPanel;

		public:
			CEventInfoPopupHandler(CProgramListPanel *pPanel);

			bool HitTest(int x, int y, LPARAM *pParam);
			bool ShowPopup(LPARAM Param, CEventInfoPopup *pPopup);
		};
		CEventInfoPopupHandler m_EventInfoPopupHandler;
		CFeaturedEventsMatcher m_FeaturedEventsMatcher;
		bool m_fShowRetrievingMessage;

		static const LPCTSTR m_pszClassName;
		static HINSTANCE m_hinst;

		void Draw(HDC hdc, const RECT *prcPaint);
		bool UpdateListInfo(const CChannelInfo *pChannelInfo);
		void GetHeaderRect(RECT *pRect) const;
		void GetChannelButtonRect(RECT *pRect) const;
		void GetProgramListRect(RECT *pRect) const;
		void CalcChannelHeight();
		void CalcDimensions();
		void SetScrollPos(int Pos);
		void SetScrollBar();
		void CalcFontHeight();
		int GetTextLeftMargin() const;
		int ItemHitTest(int x, int y) const;
		int ProgramHitTest(int x, int y) const;
		bool GetItemRect(int Item, RECT *pRect) const;
		void SetHotItem(int Item);
		void ShowChannelListMenu();
		//void SetToolTip();

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void ApplyStyle() override;
		void RealizeStyle() override;

	// CPanelForm::CPage
		bool NeedKeyboardFocus() const override { return true; }

	// CFeaturedEvents::CEventHandler
		void OnFeaturedEventsSettingsChanged(CFeaturedEvents &FeaturedEvents) override;
	};

}	// namespace TVTest


#endif
