#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H


#include "UISkin.h"
#include "View.h"
#include "ChannelManager.h"
#include "Layout.h"
#include "TitleBar.h"
#include "StatusView.h"
#include "SideBar.h"
#include "Settings.h"
#include "Panel.h"
#include "OSDManager.h"
#include "WindowUtil.h"
#include "EpgCapture.h"


#define MAIN_WINDOW_CLASS		APP_NAME TEXT(" Window")
#define FULLSCREEN_WINDOW_CLASS	APP_NAME TEXT(" Fullscreen")

// (*) が付いたものは、変えると異なるバージョン間での互換性が無くなるので注意
#define WM_APP_SERVICEUPDATE			(WM_APP+0)
#define WM_APP_CHANNELCHANGE			(WM_APP+1)
#define WM_APP_IMAGESAVE				(WM_APP+2)
#define WM_APP_TRAYICON					(WM_APP+3)
#define WM_APP_QUERYPORT				(WM_APP+5)	// (*)
#define WM_APP_FILEWRITEERROR			(WM_APP+6)
#define WM_APP_VIDEOSIZECHANGED			(WM_APP+7)
#define WM_APP_EMMPROCESSED				(WM_APP+8)
#define WM_APP_ECMERROR					(WM_APP+9)
#define WM_APP_ECMREFUSED				(WM_APP+10)
#define WM_APP_CONTROLLERFOCUS			(WM_APP+11)	// (*)
#define WM_APP_EPGLOADED				(WM_APP+12)
#define WM_APP_PLUGINMESSAGE			(WM_APP+13)
#define WM_APP_CARDREADERHUNG			(WM_APP+14)
#define WM_APP_SERVICECHANGED			(WM_APP+15)
#define WM_APP_VIDEOSTREAMTYPECHANGED	(WM_APP+16)
#define WM_APP_CHANGECASLIBRARY			(WM_APP+17)

enum {
	CONTAINER_ID_VIEW=1,
	CONTAINER_ID_PANELSPLITTER,
	CONTAINER_ID_PANEL,
	CONTAINER_ID_STATUSSPLITTER,
	CONTAINER_ID_STATUS,
	CONTAINER_ID_TITLEBARSPLITTER,
	CONTAINER_ID_TITLEBAR,
	CONTAINER_ID_SIDEBARSPLITTER,
	CONTAINER_ID_SIDEBAR
};


class CAppMain;

class CBasicViewer
{
public:
	CBasicViewer(CAppMain &App);
	bool Create(HWND hwndParent,int ViewID,int ContainerID,HWND hwndMessage);
	bool EnableViewer(bool fEnable);
	bool IsViewerEnabled() const { return m_fEnabled; }
	bool BuildViewer(BYTE VideoStreamType=0);
	bool CloseViewer();
	CViewWindow &GetViewWindow() { return m_ViewWindow; }
	const CViewWindow &GetViewWindow() const { return m_ViewWindow; }
	CVideoContainerWindow &GetVideoContainer() { return m_VideoContainer; }
	const CVideoContainerWindow &GetVideoContainer() const { return m_VideoContainer; }
	CDisplayBase &GetDisplayBase() { return m_DisplayBase; }
	const CDisplayBase &GetDisplayBase() const { return m_DisplayBase; }

protected:
	CAppMain &m_App;
	bool m_fEnabled;
	CViewWindow m_ViewWindow;
	CVideoContainerWindow m_VideoContainer;
	CDisplayBase m_DisplayBase;
};

class CMainWindow
	: public CBasicWindow
	, public CUISkin
	, public TVTest::CUIBase
	, public COSDManager::CEventHandler
{
public:
	enum { COMMAND_FROM_MOUSE=8 };
	static const DWORD HIDE_CURSOR_DELAY=1000UL;

	struct ResumeInfo {
		enum {
			VIEWERSUSPEND_MINIMIZE	=0x0001U,
			VIEWERSUSPEND_STANDBY	=0x0002U,
			VIEWERSUSPEND_SUSPEND	=0x0004U,
			VIEWERSUSPEND_EPGUPDATE	=0x0008U
		};

		CChannelSpec Channel;
		bool fOpenTuner;
		bool fSetChannel;
		bool fEnableViewer;
		unsigned int ViewerSuspendFlags;
		bool fFullscreen;

		ResumeInfo()
			: fOpenTuner(false)
			, fSetChannel(false)
			, fEnableViewer(false)
			, ViewerSuspendFlags(0)
			, fFullscreen(false)
		{
		}
	};

	CMainWindow(CAppMain &App);
	~CMainWindow();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	bool Show(int CmdShow);
	void CreatePanel();
	void AdjustWindowSize(int Width,int Height,bool fScreenSize=true);
	bool ReadSettings(CSettings &Settings);
	bool WriteSettings(CSettings &Settings);
	void ShowPanel(bool fShow);
	void SetStatusBarVisible(bool fVisible);
	bool GetStatusBarVisible() const { return m_fShowStatusBar; }
	void SetTitleBarVisible(bool fVisible);
	bool GetTitleBarVisible() const { return m_fShowTitleBar; }
	void SetCustomTitleBar(bool fCustom);
	bool GetCustomTitleBar() const { return m_fCustomTitleBar; }
	void SetSplitTitleBar(bool fSplit);
	bool GetSplitTitleBar() const { return m_fSplitTitleBar; }
	void SetCustomFrame(bool fCustomFrame,int Width=0);
	bool GetCustomFrame() const { return m_fCustomFrame; }
	void SetSideBarVisible(bool fVisible);
	bool GetSideBarVisible() const { return m_fShowSideBar; }
	bool OnBarMouseLeave(HWND hwnd);
	int GetPanelPaneIndex() const;
	bool IsPanelVisible() const;
	bool IsPanelPresent() const;
	bool IsFullscreenPanelVisible() const { return m_Fullscreen.IsPanelVisible(); }
	int GetAspectRatioType() const { return m_AspectRatioType; }

	bool EnablePlayback(bool fEnable);
	bool IsPlaybackEnabled() const { return m_fEnablePlayback; }

	bool InitStandby();
	bool InitMinimize();
	ResumeInfo &GetResumeInfo() { return m_Resume; }
	bool IsMinimizeToTray() const;
	bool ConfirmExit();
	void OnMouseWheel(WPARAM wParam,LPARAM lParam,bool fHorz);

	bool BeginChannelNoInput(int Digits);
	void EndChannelNoInput();
	bool OnChannelNoInput(int Number);

	void UpdatePanel();
	bool SetViewWindowEdge(bool fEdge);
	bool GetViewWindowEdge() const { return m_fViewWindowEdge; }

	Layout::CLayoutBase &GetLayoutBase() { return m_LayoutBase; }
	CDisplayBase &GetDisplayBase() { return m_Viewer.GetDisplayBase(); }
	CTitleBar &GetTitleBar() { return m_TitleBar; }
	CStatusView &GetStatusView();
	CSideBar &GetSideBar();

	enum {
		PROGRAMGUIDE_SHOW_ONSCREEN = 0x0001U,
		PROGRAMGUIDE_SHOW_POPUP    = 0x0002U
	};
	struct ProgramGuideSpaceInfo {
		LPCTSTR pszTuner;
		LPCTSTR pszSpace;
	};
	bool ShowProgramGuide(bool fShow,unsigned int Flags=0,const ProgramGuideSpaceInfo *pSpaceInfo=NULL);

	static bool Initialize(HINSTANCE hinst);

// CUISkin
	HWND GetVideoHostWindow() const override;

// CUIBase
	void SetTheme(const TVTest::Theme::CThemeManager *pThemeManager) override;

private:
	class CBarLayout
	{
	public:
		CBarLayout() {}
		virtual ~CBarLayout() {}
		virtual void Layout(RECT *pArea,RECT *pBarRect) = 0;

		bool IsSpot(const RECT *pArea,const POINT *pPos);
		void AdjustArea(RECT *pArea);
		void ReserveArea(RECT *pArea,bool fNoMove);
	};

	class CTitleBarManager
		: public CTitleBar::CEventHandler
		, public CBarLayout
	{
	public:
		CTitleBarManager(CMainWindow *pMainWindow,bool fMainWindow);
	// CBarLayout
		void Layout(RECT *pArea,RECT *pBarRect) override;
	// CTitleBarManager
		void EndDrag();

	private:
	// CTitleBar::CEventHandler
		bool OnClose() override;
		bool OnMinimize() override;
		bool OnMaximize() override;
		bool OnFullscreen() override;
		void OnMouseLeave() override;
		void OnLabelLButtonDown(int x,int y) override;
		void OnLabelLButtonDoubleClick(int x,int y) override;
		void OnLabelRButtonDown(int x,int y) override;
		void OnIconLButtonDown(int x,int y) override;
		void OnIconLButtonDoubleClick(int x,int y) override;
	// CTitleBarManager
		void ShowSystemMenu(int x,int y);

		CMainWindow *m_pMainWindow;
		bool m_fMainWindow;
		bool m_fFixed;
	};

	class CSideBarManager : public CSideBar::CEventHandler, public CBarLayout
	{
	public:
		CSideBarManager(CMainWindow *pMainWindow);
	// CBarLayout
		void Layout(RECT *pArea,RECT *pBarRect) override;

	private:
		CMainWindow *m_pMainWindow;
		bool m_fFixed;

		const CChannelInfo *GetChannelInfoByCommand(int Command);

	// CSideBar::CEventHandler
		void OnCommand(int Command) override;
		void OnRButtonDown(int x,int y) override;
		void OnMouseLeave() override;
		bool GetTooltipText(int Command,LPTSTR pszText,int MaxText) override;
		bool DrawIcon(int Command,HDC hdc,const RECT &ItemRect,COLORREF ForeColor,HDC hdcBuffer) override;
	};

	class CShowCursorManager
	{
	public:
		CShowCursorManager();
		void Reset(int Delta=1);
		bool OnCursorMove(int x,int y);

	private:
		int m_MoveDelta;
		POINT m_LastMovePos;
	};

	class CFullscreen
		: public CCustomWindow
		, public TVTest::CUIBase
	{
	public:
		CFullscreen(CMainWindow &MainWindow);
		~CFullscreen();
		bool Create(HWND hwndOwner,CBasicViewer *pViewer);
		void ShowPanel(bool fShow);
		bool IsPanelVisible() const { return m_fShowPanel; }
		bool SetPanelWidth(int Width);
		int GetPanelWidth() const { return m_PanelWidth; }
		void HideAllBars();
		void OnRButtonDown();
		void OnMButtonDown();
		void OnMouseMove();

	// CUIBase
		void SetTheme(const TVTest::Theme::CThemeManager *pThemeManager) override;

		static bool Initialize(HINSTANCE hinst);

	private:
		class CPanelEventHandler : public CPanel::CEventHandler {
		public:
			CPanelEventHandler(CMainWindow *pMainWindow);
			bool OnClose() override;

		private:
			CMainWindow *m_pMainWindow;
		};

		CMainWindow &m_MainWindow;
		CAppMain &m_App;
		Layout::CLayoutBase m_LayoutBase;
		CViewWindow m_ViewWindow;
		CBasicViewer *m_pViewer;
		CTitleBar m_TitleBar;
		CTitleBarManager m_TitleBarManager;
		CPanel m_Panel;
		CPanelEventHandler m_PanelEventHandler;
		bool m_fShowCursor;
		bool m_fMenu;
		bool m_fShowStatusView;
		bool m_fShowTitleBar;
		bool m_fShowSideBar;
		bool m_fShowPanel;
		int m_PanelWidth;
		CShowCursorManager m_ShowCursorManager;

		bool OnCreate();
		void OnMouseCommand(int Command);
		void OnLButtonDoubleClick();
		void ShowCursor(bool fShow);
		void ShowStatusView(bool fShow);
		void ShowTitleBar(bool fShow);
		void ShowSideBar(bool fShow);
	// CBasicWindow
		bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;
	// CCustomWindow
		LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
	};

	class CStatusViewEventHandler : public CStatusView::CEventHandler
	{
	public:
		CStatusViewEventHandler(CMainWindow *pMainWindow);

	private:
		CMainWindow *m_pMainWindow;

		void OnMouseLeave() override;
		void OnHeightChanged(int Height) override;
	};

	class CVideoContainerEventHandler : public CVideoContainerWindow::CEventHandler
	{
	public:
		CVideoContainerEventHandler(CMainWindow *pMainWindow);

	private:
		CMainWindow *m_pMainWindow;

		void OnSizeChanged(int Width,int Height) override;
	};

	class CViewWindowEventHandler : public CViewWindow::CEventHandler
	{
	public:
		CViewWindowEventHandler(CMainWindow *pMainWindow);

	private:
		CMainWindow *m_pMainWindow;

		void OnSizeChanged(int Width,int Height) override;
	};

	class CEpgCaptureEventHandler : public TVTest::CEpgCaptureManager::CEventHandler
	{
	public:
		CEpgCaptureEventHandler(CMainWindow *pMainWindow);

	private:
		CMainWindow *m_pMainWindow;

		void OnBeginCapture(unsigned int Flags,unsigned int Status) override;
		void OnEndCapture(unsigned int Flags) override;
		void OnChannelChanged() override;
		void OnChannelEnd(bool fComplete) override;
	};

	enum { UPDATE_TIMER_INTERVAL=500 };

	CAppMain &m_App;
	Layout::CLayoutBase m_LayoutBase;
	CBasicViewer m_Viewer;
	CTitleBar m_TitleBar;
	CTitleBarManager m_TitleBarManager;
	CSideBarManager m_SideBarManager;
	CStatusViewEventHandler m_StatusViewEventHandler;
	CVideoContainerEventHandler m_VideoContainerEventHandler;
	CViewWindowEventHandler m_ViewWindowEventHandler;
	CFullscreen m_Fullscreen;
	CNotificationBar m_NotificationBar;

	bool m_fShowStatusBar;
	bool m_fPopupStatusBar;
	bool m_fShowTitleBar;
	bool m_fCustomTitleBar;
	bool m_fPopupTitleBar;
	bool m_fSplitTitleBar;
	bool m_fShowSideBar;
	int m_PanelPaneIndex;
	bool m_fCustomFrame;
	int m_CustomFrameWidth;
	int m_ThinFrameWidth;
	enum {
		FRAME_NORMAL,
		FRAME_CUSTOM,
		FRAME_NONE
	};
	bool m_fViewWindowEdge;

	bool m_fEnablePlayback;

	bool m_fStandbyInit;
	bool m_fMinimizeInit;
	ResumeInfo m_Resume;

	enum WindowSizeMode {
		WINDOW_SIZE_HD,
		WINDOW_SIZE_1SEG
	};
	struct WindowSize {
		int Width;
		int Height;
		WindowSize() : Width(0), Height(0) {}
		WindowSize &operator=(const RECT &rc) {
			Width=rc.right-rc.left;
			Height=rc.bottom-rc.top;
			return *this;
		}
		bool IsValid() const { return Width>0 && Height>0; }
	};
	WindowSizeMode m_WindowSizeMode;
	WindowSize m_HDWindowSize;
	WindowSize m_1SegWindowSize;

	bool m_fLockLayout;

	bool m_fShowCursor;
	bool m_fNoHideCursor;
	CShowCursorManager m_ShowCursorManager;

	bool m_fDragging;
	POINT m_ptDragStartPos;
	RECT m_rcDragStart;
	bool m_fClosing;

	CMouseWheelHandler m_WheelHandler;
	int m_WheelCount;
	int m_PrevWheelMode;
	DWORD m_PrevWheelTime;

	enum {
		ASPECTRATIO_DEFAULT,
		ASPECTRATIO_16x9,
		ASPECTRATIO_LETTERBOX,
		ASPECTRATIO_SUPERFRAME,
		ASPECTRATIO_SIDECUT,
		ASPECTRATIO_4x3,
		ASPECTRATIO_32x9,
		ASPECTRATIO_16x9_LEFT,
		ASPECTRATIO_16x9_RIGHT,
		ASPECTRATIO_CUSTOM
	};
	int m_AspectRatioType;
	DWORD m_AspectRatioResetTime;
	bool m_fForceResetPanAndScan;
	int m_DefaultAspectRatioMenuItemCount;
	bool m_fFrameCut;
	int m_VideoSizeChangedTimerCount;
	unsigned int m_ProgramListUpdateTimerCount;
	int m_CurEventStereoMode;
	bool m_fAlertedLowFreeSpace;

	class CTimer {
		HWND m_hwnd;
		UINT m_ID;
	public:
		CTimer(UINT ID) : m_hwnd(NULL), m_ID(ID) {}
		bool Begin(HWND hwnd,DWORD Interval) {
			if (::SetTimer(hwnd,m_ID,Interval,NULL)==0) {
				m_hwnd=NULL;
				return false;
			}
			m_hwnd=hwnd;
			return true;
		}
		void End() {
			if (m_hwnd!=NULL) {
				::KillTimer(m_hwnd,m_ID);
				m_hwnd=NULL;
			}
		}
		bool IsEnabled() const { return m_hwnd!=NULL; }
	};
	CTimer m_ResetErrorCountTimer;

	class CDisplayBaseEventHandler : public CDisplayBase::CEventHandler {
		CMainWindow *m_pMainWindow;
		bool OnVisibleChange(bool fVisible) override;
	public:
		CDisplayBaseEventHandler(CMainWindow *pMainWindow);
	};
	CDisplayBaseEventHandler m_DisplayBaseEventHandler;

	struct ChannelNoInputInfo {
		bool fInputting;
		int Digits;
		int CurDigit;
		int Number;
		ChannelNoInputInfo() : fInputting(false) {}
	};
	ChannelNoInputInfo m_ChannelNoInput;
	DWORD m_ChannelNoInputTimeout;
	CTimer m_ChannelNoInputTimer;

	CEpgCaptureEventHandler m_EpgCaptureEventHandler;

	struct DirectShowFilterPropertyInfo {
		CMediaViewer::PropertyFilter Filter;
		int Command;
	};

	static const BYTE m_AudioGainList[];
	static const DirectShowFilterPropertyInfo m_DirectShowFilterPropertyList[];
	static ATOM m_atomChildOldWndProcProp;

// CUISkin
	HWND GetMainWindow() const override { return m_hwnd; }
	void ShowNotificationBar(LPCTSTR pszText,
							 CNotificationBar::MessageType Type=CNotificationBar::MESSAGE_INFO,
							 DWORD Duration=0,bool fSkippable=false) override;
	bool InitializeViewer(BYTE VideoStreamType=0) override;
	bool FinalizeViewer() override;
	bool EnableViewer(bool fEnable) override;
	bool IsViewerEnabled() const override;
	HWND GetViewerWindow() const override;
	bool SetZoomRate(int Rate,int Factor) override;
	bool GetZoomRate(int *pRate,int *pFactor) override;
	bool SetPanAndScan(const PanAndScanInfo &Info) override;
	bool GetPanAndScan(PanAndScanInfo *pInfo) const override;
	bool SetLogo(HBITMAP hbm) override;
	bool SetAlwaysOnTop(bool fTop) override;
	bool SetFullscreen(bool fFullscreen) override;
	bool SetStandby(bool fStandby) override;
	bool ShowVolumeOSD() override;

// CAppEventHandler
	void OnTunerChanged() override;
	void OnTunerOpened() override;
	void OnTunerClosed() override;
	void OnChannelChanged(unsigned int Status) override;
	void OnServiceChanged() override;
	void OnChannelListChanged() override;
	void OnRecordingStarted() override;
	void OnRecordingStopped() override;
	void OnRecordingPaused() override;
	void OnRecordingResumed() override;
	void On1SegModeChanged(bool f1SegMode) override;
	void OnVolumeChanged(int Volume) override;
	void OnMuteChanged(bool fMute) override;
	void OnStereoModeChanged(int StereoMode) override;
	void OnAudioStreamChanged(int Stream) override;

// COSDManager::CEventHandler
	bool GetOSDWindow(HWND *phwndParent,RECT *pRect,bool *pfForcePseudoOSD) override;
	bool SetOSDHideTimer(DWORD Delay) override;

// CMainWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	bool OnCreate(const CREATESTRUCT *pcs);
	void OnDestroy();
	void OnSizeChanged(UINT State,int Width,int Height);
	bool OnSizeChanging(UINT Edge,RECT *pRect);
	void OnMouseMove(int x,int y);
	bool OnSetCursor(HWND hwndCursor,int HitTestCode,int MouseMessage);
	void OnCommand(HWND hwnd,int id,HWND hwndCtl,UINT codeNotify);
	void OnTimer(HWND hwnd,UINT id);
	bool OnInitMenuPopup(HMENU hmenu);
	void OnRecordingStateChanged();
	void OnEventChanged();
	void AutoSelectStereoMode();
	int GetZoomPercentage();
	bool AutoFitWindowToVideo();
	bool SetPanAndScan(int Command);
	void LockLayout();
	void UpdateLayout();
	void ShowCursor(bool fShow);
	void ShowChannelOSD();
	void ShowAudioOSD();
	void SetWindowVisible();
	void ShowFloatingWindows(bool fShow);
	void StoreTunerResumeInfo();
	bool ResumeTuner();
	void ResumeChannel();
	void SuspendViewer(unsigned int Flags);
	void ResumeViewer(unsigned int Flags);
	bool SetEpgUpdateNextChannel();
	void RefreshChannelPanel();
	void InitControlPanel();
	void UpdateControlPanel();
	void HookWindows(HWND hwnd);
	void HookChildWindow(HWND hwnd);
	void SendCommand(int Command) { SendMessage(WM_COMMAND,Command,0); }
	void PostCommand(int Command) { PostMessage(WM_COMMAND,Command,0); }

	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static LRESULT CALLBACK ChildHookProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
