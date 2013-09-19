#ifndef CHANNEL_PANEL_H
#define CHANNEL_PANEL_H


#include <vector>
#include "PanelForm.h"
#include "EpgProgramList.h"
#include "ChannelList.h"
#include "Theme.h"
#include "DrawUtil.h"
#include "EventInfoPopup.h"
#include "LogoManager.h"
#include "Tooltip.h"
#include "Settings.h"


class CChannelPanel
	: public CPanelForm::CPage
	, public CSettingsBase
{
public:
	struct ThemeInfo {
		Theme::Style ChannelNameStyle;
		Theme::Style CurChannelNameStyle;
		Theme::Style EventStyle[2];
		Theme::Style CurChannelEventStyle[2];
		COLORREF MarginColor;
	};

	class CEventHandler {
	public:
		virtual ~CEventHandler() {}
		virtual void OnChannelClick(const CChannelInfo *pChannelInfo) {}
		virtual void OnRButtonDown() {}
	};

	enum {
		MAX_EVENTS_PER_CHANNEL = 4,
		MAX_EXPAND_EVENTS      = 10
	};

	CChannelPanel();
	~CChannelPanel();

// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;

// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;

// CChannelPanel
	bool SetEpgProgramList(CEpgProgramList *pList);
	bool SetChannelList(const CChannelList *pChannelList,bool fSetEvent=true);
	void ClearChannelList() { SetChannelList(NULL); }
	bool UpdateAllChannels(bool fUpdateProgramList);
	bool UpdateChannel(int ChannelIndex);
	bool UpdateChannels(WORD NetworkID,WORD TransportStreamID);
	bool IsChannelListEmpty() const;
	bool SetCurrentChannel(int CurChannel);
	bool ScrollToChannel(int Channel);
	bool ScrollToCurrentChannel();
	void SetEventHandler(CEventHandler *pEventHandler);
	bool SetTheme(const ThemeInfo *pTheme);
	bool GetTheme(ThemeInfo *pTheme) const;
	bool SetFont(const LOGFONT *pFont);
	bool SetEventInfoFont(const LOGFONT *pFont);
	void SetDetailToolTip(bool fDetail);
	bool GetDetailToolTip() const { return m_fDetailToolTip; }
	bool SetEventsPerChannel(int Events,int Expand=-1);
	int GetEventsPerChannel() const { return m_EventsPerChannel; }
	int GetExpandAdditionalEvents() const { return m_ExpandAdditionalEvents; }
	bool ExpandChannel(int Channel,bool fExpand);
	void SetScrollToCurChannel(bool fScroll);
	bool GetScrollToCurChannel() const { return m_fScrollToCurChannel; }
	void SetLogoManager(CLogoManager *pLogoManager);
	bool QueryUpdate() const;

	static bool Initialize(HINSTANCE hinst);

private:
	class CChannelEventInfo
	{
		CChannelInfo m_ChannelInfo;
		int m_OriginalChannelIndex;
		std::vector<CEventInfoData> m_EventList;
		HBITMAP m_hbmLogo;
		DrawUtil::CBitmap m_StretchedLogo;
		bool m_fExpanded;

	public:
		CChannelEventInfo(const CChannelInfo *pChannelInfo,int OriginalIndex);
		~CChannelEventInfo();
		bool SetEventInfo(int Index,const CEventInfoData *pInfo);
		const CChannelInfo &GetChannelInfo() const { return m_ChannelInfo; }
		const CEventInfoData &GetEventInfo(int Index) const { return m_EventList[Index]; }
		int NumEvents() const { return (int)m_EventList.size(); }
		void SetMaxEvents(int Events);
		bool IsEventEnabled(int Index) const;
		WORD GetNetworkID() const { return m_ChannelInfo.GetNetworkID(); }
		WORD GetTransportStreamID() const { return m_ChannelInfo.GetTransportStreamID(); }
		WORD GetServiceID() const { return m_ChannelInfo.GetServiceID(); }
		int FormatEventText(LPTSTR pszText,int MaxLength,int Index) const;
		void DrawChannelName(HDC hdc,const RECT *pRect);
		void DrawEventName(HDC hdc,const RECT *pRect,int Index);
		int GetOriginalChannelIndex() const { return m_OriginalChannelIndex; }
		HBITMAP GetLogo() const { return m_hbmLogo; }
		void SetLogo(HBITMAP hbm) { m_hbmLogo=hbm; }
		bool IsExpanded() const { return m_fExpanded; }
		void Expand(bool fExpand) { m_fExpanded=fExpand; }
	};

	CEpgProgramList *m_pProgramList;
	DrawUtil::CFont m_Font;
	DrawUtil::CFont m_ChannelFont;
	int m_FontHeight;
	Util::CRect m_ChannelNameMargins;
	Util::CRect m_EventNameMargins;
	int m_ChannelChevronMargin;
	int m_EventNameLines;
	int m_ChannelNameHeight;
	int m_EventNameHeight;
	int m_ItemHeight;
	int m_ExpandedItemHeight;
	ThemeInfo m_Theme;
	DrawUtil::CMonoColorBitmap m_Chevron;
	int m_EventsPerChannel;
	int m_ExpandAdditionalEvents;
	int m_ExpandEvents;
	int m_ScrollPos;
	bool m_fScrollToCurChannel;
	std::vector<CChannelEventInfo*> m_ChannelList;
	int m_CurChannel;
	CEventHandler *m_pEventHandler;
	CTooltip m_Tooltip;
	bool m_fDetailToolTip;
	CEventInfoPopup m_EventInfoPopup;
	CEventInfoPopupManager m_EventInfoPopupManager;
	class CEventInfoPopupHandler : public CEventInfoPopupManager::CEventHandler
	{
		CChannelPanel *m_pChannelPanel;
	public:
		CEventInfoPopupHandler(CChannelPanel *pChannelPanel);
		bool HitTest(int x,int y,LPARAM *pParam);
		bool GetEventInfo(LPARAM Param,const CEventInfoData **ppInfo);
	};
	CEventInfoPopupHandler m_EventInfoPopupHandler;
	CLogoManager *m_pLogoManager;
	SYSTEMTIME m_UpdatedTime;

	static const LPCTSTR m_pszClassName;
	static HINSTANCE m_hinst;

	void ClearChannels();
	bool UpdateEvents(CChannelEventInfo *pInfo,const SYSTEMTIME *pTime=NULL);
	void Draw(HDC hdc,const RECT *prcPaint);
	void SetScrollPos(int Pos);
	void SetScrollBar();
	void CalcItemHeight();
	int CalcHeight() const;
	void GetItemRect(int Index,RECT *pRect);
	enum HitType {
		HIT_CHANNELNAME,
		HIT_CHEVRON,
		HIT_MARGIN,
		HIT_EVENT1
	};
	int HitTest(int x,int y,HitType *pType=NULL) const;
	bool CreateTooltip();
	void SetTooltips(bool fRectOnly=false);
	bool EventInfoPopupHitTest(int x,int y,LPARAM *pParam);
	bool GetEventInfoPopupEventInfo(LPARAM Param,const CEventInfoData **ppInfo);

// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};


#endif
