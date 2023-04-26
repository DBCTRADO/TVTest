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


#ifndef TVTEST_CHANNEL_PANEL_H
#define TVTEST_CHANNEL_PANEL_H


#include <vector>
#include <memory>
#include "PanelForm.h"
#include "UIBase.h"
#include "ChannelList.h"
#include "Theme.h"
#include "DrawUtil.h"
#include "EventInfoPopup.h"
#include "LogoManager.h"
#include "Tooltip.h"
#include "Settings.h"
#include "WindowUtil.h"
#include "EpgUtil.h"
#include "FeaturedEvents.h"
#include "TextDraw.h"
#include "LibISDB/LibISDB/EPG/EPGDatabase.hpp"


namespace TVTest
{

	class CChannelPanel
		: public CPanelForm::CPage
		, public CSettingsBase
		, protected CFeaturedEvents::CEventHandler
	{
	public:
		struct ChannelPanelTheme
		{
			Theme::Style ChannelNameStyle;
			Theme::Style CurChannelNameStyle;
			Theme::Style EventStyle[2];
			Theme::Style CurChannelEventStyle[2];
			Theme::ThemeColor MarginColor;
			Theme::BackgroundStyle FeaturedMarkStyle;
			Theme::BackgroundStyle ProgressStyle;
			Theme::BackgroundStyle CurProgressStyle;
		};

		class CEventHandler
		{
		public:
			virtual ~CEventHandler() = default;
			virtual void OnChannelClick(const CChannelInfo *pChannelInfo) {}
		};

		static constexpr int MAX_EVENTS_PER_CHANNEL = 4;
		static constexpr int MAX_EXPAND_EVENTS      = 10;

		enum class ProgressBarStyle {
			Elapsed,
			Remaining,
		};

		CChannelPanel();
		~CChannelPanel();

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

	// CChannelPanel
		bool SetEPGDatabase(LibISDB::EPGDatabase *pEPGDatabase);
		bool SetChannelList(const CChannelList *pChannelList, bool fSetEvent = true);
		void ClearChannelList() { SetChannelList(nullptr); }
		bool UpdateAllChannels();
		bool UpdateChannel(int ChannelIndex);
		bool UpdateChannels(WORD NetworkID, WORD TransportStreamID);
		bool IsChannelListEmpty() const;
		bool SetCurrentChannel(int CurChannel);
		bool ScrollToChannel(int Channel);
		bool ScrollToCurrentChannel();
		void SetEventHandler(CEventHandler *pEventHandler);
		bool SetChannelPanelTheme(const ChannelPanelTheme &Theme);
		bool GetChannelPanelTheme(ChannelPanelTheme *pTheme) const;
		bool SetEventInfoFont(const Style::Font &Font);
		void SetDetailToolTip(bool fDetail);
		bool GetDetailToolTip() const { return m_fDetailToolTip; }
		bool SetEventsPerChannel(int Events, int Expand = -1);
		int GetEventsPerChannel() const { return m_EventsPerChannel; }
		int GetExpandAdditionalEvents() const { return m_ExpandAdditionalEvents; }
		bool ExpandChannel(int Channel, bool fExpand);
		void SetScrollToCurChannel(bool fScroll);
		bool GetScrollToCurChannel() const { return m_fScrollToCurChannel; }
		void SetUseEpgColorScheme(bool fUseEpgColorScheme);
		bool GetUseEpgColorScheme() const { return m_fUseEpgColorScheme; }
		void SetShowGenreColor(bool fShowGenreColor);
		bool GetShowGenreColor() const { return m_fShowGenreColor; }
		void SetShowFeaturedMark(bool fShowFeaturedMark);
		bool GetShowFeaturedMark() const { return m_fShowFeaturedMark; }
		void SetShowProgressBar(bool fShowProgressBar);
		bool GetShowProgressBar() const { return m_fShowProgressBar; }
		void SetProgressBarStyle(ProgressBarStyle Style);
		ProgressBarStyle GetProgressBarStyle() const { return m_ProgressBarStyle; }
		void SetUseARIBSymbol(bool fUseARIBSymbol);
		bool GetUseARIBSymbol() const { return m_fUseARIBSymbol; }
		bool QueryUpdateProgress();
		void UpdateProgress();
		void SetLogoManager(CLogoManager *pLogoManager);
		bool QueryUpdate() const;

		static bool Initialize(HINSTANCE hinst);

	private:
		class CChannelEventInfo
		{
			CChannelInfo m_ChannelInfo;
			int m_OriginalChannelIndex;
			std::vector<LibISDB::EventInfo> m_EventList;
			HBITMAP m_hbmLogo = nullptr;
			DrawUtil::CBitmap m_StretchedLogo;
			bool m_fExpanded = false;

		public:
			CChannelEventInfo(const CChannelInfo *pChannelInfo, int OriginalIndex);

			bool SetEventInfo(int Index, const LibISDB::EventInfo *pInfo);
			const CChannelInfo &GetChannelInfo() const { return m_ChannelInfo; }
			const LibISDB::EventInfo &GetEventInfo(int Index) const { return m_EventList[Index]; }
			int NumEvents() const { return static_cast<int>(m_EventList.size()); }
			void SetMaxEvents(int Events);
			bool IsEventEnabled(int Index) const;
			WORD GetNetworkID() const { return m_ChannelInfo.GetNetworkID(); }
			WORD GetTransportStreamID() const { return m_ChannelInfo.GetTransportStreamID(); }
			WORD GetServiceID() const { return m_ChannelInfo.GetServiceID(); }
			bool FormatEventText(int Index, String *pText) const;
			void DrawChannelName(HDC hdc, const RECT *pRect, const Style::Margins &LogoMargins);
			void DrawEventName(int Index, CTextDraw &TextDraw, const RECT &Rect, int LineHeight, bool fUseARIBSymbol);
			int GetOriginalChannelIndex() const { return m_OriginalChannelIndex; }
			HBITMAP GetLogo() const { return m_hbmLogo; }
			void SetLogo(HBITMAP hbm) { m_hbmLogo = hbm; }
			bool IsExpanded() const { return m_fExpanded; }
			void Expand(bool fExpand) { m_fExpanded = fExpand; }
		};

		struct ChannelPanelStyle
		{
			Style::Margins ChannelNameMargin{2, 2, 2, 2};
			Style::Margins ChannelLogoMargin{0, 0, 3, 0};
			Style::Margins EventNameMargin{8, 1, 2, 1};
			Style::Size ChannelChevronSize{10, 10};
			Style::IntValue ChannelChevronMargin{12};
			Style::Margins FeaturedMarkMargin{1};

			void SetStyle(const Style::CStyleManager *pStyleManager);
			void NormalizeStyle(
				const Style::CStyleManager *pStyleManager,
				const Style::CStyleScaling *pStyleScaling);
		};

		LibISDB::EPGDatabase *m_pEPGDatabase = nullptr;
		ChannelPanelStyle m_Style;
		Style::Font m_StyleFont;
		DrawUtil::CFont m_Font;
		DrawUtil::CFont m_ChannelFont;
		int m_FontHeight = 0;
		int m_EventNameLines = 2;
		int m_ChannelNameHeight = 0;
		int m_EventNameHeight = 0;
		int m_ItemHeight = 0;
		int m_ExpandedItemHeight = 0;
		ChannelPanelTheme m_Theme;
		CEpgTheme m_EpgTheme;
		bool m_fUseEpgColorScheme = false;
		bool m_fShowGenreColor = false;
		bool m_fShowFeaturedMark = true;
		bool m_fShowProgressBar = true;
		bool m_fUseARIBSymbol = false;
		ProgressBarStyle m_ProgressBarStyle = ProgressBarStyle::Elapsed;
		DrawUtil::COffscreen m_Offscreen;
		Theme::IconList m_Chevron;
		int m_EventsPerChannel = 2;
		int m_ExpandAdditionalEvents = 4;
		int m_ExpandEvents = m_EventsPerChannel + m_ExpandAdditionalEvents;
		int m_ScrollPos = 0;
		int m_OldDPI = 0;
		CMouseWheelHandler m_MouseWheel;
		bool m_fScrollToCurChannel = false;
		std::vector<std::unique_ptr<CChannelEventInfo>> m_ChannelList;
		int m_CurChannel = -1;
		CEventHandler *m_pEventHandler = nullptr;
		CTooltip m_Tooltip;
		String m_TooltipText;
		bool m_fDetailToolTip = false;
		CEventInfoPopup m_EventInfoPopup;
		CEventInfoPopupManager m_EventInfoPopupManager;
		class CEventInfoPopupHandler
			: public CEventInfoPopupManager::CEventHandler
		{
			CChannelPanel *m_pChannelPanel;
		public:
			CEventInfoPopupHandler(CChannelPanel *pChannelPanel);
			bool HitTest(int x, int y, LPARAM *pParam) override;
			bool ShowPopup(LPARAM Param, CEventInfoPopup *pPopup) override;
		};
		CEventInfoPopupHandler m_EventInfoPopupHandler{this};
		CLogoManager *m_pLogoManager = nullptr;
		LibISDB::DateTime m_UpdatedTime;
		LibISDB::DateTime m_CurTime;
		CFeaturedEventsMatcher m_FeaturedEventsMatcher;

		static const LPCTSTR m_pszClassName;
		static HINSTANCE m_hinst;

		void ClearChannels();
		bool UpdateEvents(CChannelEventInfo *pInfo, const LibISDB::DateTime *pTime = nullptr);
		void Draw(HDC hdc, const RECT *prcPaint);
		void OnCommand(int ID);
		void SetScrollPos(int Pos);
		void SetScrollBar();
		void CalcItemHeight();
		int CalcHeight() const;
		void GetItemRect(int Index, RECT *pRect);
		enum HitType {
			HIT_CHANNELNAME,
			HIT_CHEVRON,
			HIT_MARGIN,
			HIT_EVENT1
		};
		int HitTest(int x, int y, HitType *pType = nullptr) const;
		bool CreateTooltip();
		void SetTooltipFont();
		void SetTooltips(bool fRectOnly = false);
		bool EventInfoPopupHitTest(int x, int y, LPARAM *pParam);
		bool ShowEventInfoPopup(LPARAM Param, CEventInfoPopup *pPopup);
		void ShowMenu(int x, int y);

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
