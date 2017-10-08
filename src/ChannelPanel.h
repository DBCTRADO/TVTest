#ifndef CHANNEL_PANEL_H
#define CHANNEL_PANEL_H


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


class CChannelPanel
	: public CPanelForm::CPage
	, public CSettingsBase
	, protected CFeaturedEvents::CEventHandler
{
public:
	struct ChannelPanelTheme
	{
		TVTest::Theme::Style ChannelNameStyle;
		TVTest::Theme::Style CurChannelNameStyle;
		TVTest::Theme::Style EventStyle[2];
		TVTest::Theme::Style CurChannelEventStyle[2];
		TVTest::Theme::ThemeColor MarginColor;
		TVTest::Theme::BackgroundStyle FeaturedMarkStyle;
		TVTest::Theme::BackgroundStyle ProgressStyle;
		TVTest::Theme::BackgroundStyle CurProgressStyle;
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
	void SetStyle(const TVTest::Style::CStyleManager *pStyleManager) override;
	void NormalizeStyle(
		const TVTest::Style::CStyleManager *pStyleManager,
		const TVTest::Style::CStyleScaling *pStyleScaling) override;
	void SetTheme(const TVTest::Theme::CThemeManager *pThemeManager) override;

// CPage
	bool SetFont(const TVTest::Style::Font &Font) override;

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
	bool SetEventInfoFont(const TVTest::Style::Font &Font);
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
		HBITMAP m_hbmLogo;
		DrawUtil::CBitmap m_StretchedLogo;
		bool m_fExpanded;

	public:
		CChannelEventInfo(const CChannelInfo *pChannelInfo, int OriginalIndex);
		~CChannelEventInfo();
		bool SetEventInfo(int Index, const LibISDB::EventInfo *pInfo);
		const CChannelInfo &GetChannelInfo() const { return m_ChannelInfo; }
		const LibISDB::EventInfo &GetEventInfo(int Index) const { return m_EventList[Index]; }
		int NumEvents() const { return (int)m_EventList.size(); }
		void SetMaxEvents(int Events);
		bool IsEventEnabled(int Index) const;
		WORD GetNetworkID() const { return m_ChannelInfo.GetNetworkID(); }
		WORD GetTransportStreamID() const { return m_ChannelInfo.GetTransportStreamID(); }
		WORD GetServiceID() const { return m_ChannelInfo.GetServiceID(); }
		int FormatEventText(LPTSTR pszText, int MaxLength, int Index) const;
		void DrawChannelName(HDC hdc, const RECT *pRect, const TVTest::Style::Margins &LogoMargins);
		void DrawEventName(int Index, TVTest::CTextDraw &TextDraw, const RECT &Rect, int LineHeight);
		int GetOriginalChannelIndex() const { return m_OriginalChannelIndex; }
		HBITMAP GetLogo() const { return m_hbmLogo; }
		void SetLogo(HBITMAP hbm) { m_hbmLogo = hbm; }
		bool IsExpanded() const { return m_fExpanded; }
		void Expand(bool fExpand) { m_fExpanded = fExpand; }
	};

	struct ChannelPanelStyle
	{
		TVTest::Style::Margins ChannelNameMargin;
		TVTest::Style::Margins ChannelLogoMargin;
		TVTest::Style::Margins EventNameMargin;
		TVTest::Style::Size ChannelChevronSize;
		TVTest::Style::IntValue ChannelChevronMargin;
		TVTest::Style::Margins FeaturedMarkMargin;

		ChannelPanelStyle();
		void SetStyle(const TVTest::Style::CStyleManager *pStyleManager);
		void NormalizeStyle(
			const TVTest::Style::CStyleManager *pStyleManager,
			const TVTest::Style::CStyleScaling *pStyleScaling);
	};

	LibISDB::EPGDatabase *m_pEPGDatabase;
	ChannelPanelStyle m_Style;
	TVTest::Style::Font m_StyleFont;
	DrawUtil::CFont m_Font;
	DrawUtil::CFont m_ChannelFont;
	int m_FontHeight;
	int m_ChannelChevronMargin;
	int m_EventNameLines;
	int m_ChannelNameHeight;
	int m_EventNameHeight;
	int m_ItemHeight;
	int m_ExpandedItemHeight;
	ChannelPanelTheme m_Theme;
	CEpgTheme m_EpgTheme;
	bool m_fUseEpgColorScheme;
	bool m_fShowGenreColor;
	bool m_fShowFeaturedMark;
	bool m_fShowProgressBar;
	ProgressBarStyle m_ProgressBarStyle;
	DrawUtil::COffscreen m_Offscreen;
	TVTest::Theme::IconList m_Chevron;
	int m_EventsPerChannel;
	int m_ExpandAdditionalEvents;
	int m_ExpandEvents;
	int m_ScrollPos;
	int m_OldDPI;
	CMouseWheelHandler m_MouseWheel;
	bool m_fScrollToCurChannel;
	std::vector<std::unique_ptr<CChannelEventInfo>> m_ChannelList;
	int m_CurChannel;
	CEventHandler *m_pEventHandler;
	CTooltip m_Tooltip;
	bool m_fDetailToolTip;
	CEventInfoPopup m_EventInfoPopup;
	CEventInfoPopupManager m_EventInfoPopupManager;
	class CEventInfoPopupHandler
		: public CEventInfoPopupManager::CEventHandler
	{
		CChannelPanel *m_pChannelPanel;
	public:
		CEventInfoPopupHandler(CChannelPanel *pChannelPanel);
		bool HitTest(int x, int y, LPARAM *pParam);
		bool ShowPopup(LPARAM Param, CEventInfoPopup *pPopup);
	};
	CEventInfoPopupHandler m_EventInfoPopupHandler;
	CLogoManager *m_pLogoManager;
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


#endif
