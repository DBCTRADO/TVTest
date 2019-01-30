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


#ifndef TVTEST_MAIN_WINDOW_H
#define TVTEST_MAIN_WINDOW_H


#include "UISkin.h"
#include "View.h"
#include "MainDisplay.h"
#include "ChannelManager.h"
#include "Command.h"
#include "Layout.h"
#include "TitleBar.h"
#include "StatusView.h"
#include "SideBar.h"
#include "Settings.h"
#include "Panel.h"
#include "OSDManager.h"
#include "WindowUtil.h"
#include "EpgCapture.h"
#include "AudioManager.h"
#include "ChannelInput.h"


namespace TVTest
{

	extern const LPCTSTR MAIN_WINDOW_CLASS;
	extern const LPCTSTR FULLSCREEN_WINDOW_CLASS;

// (*) が付いたものは、変えると異なるバージョン間での互換性が無くなるので注意
	constexpr UINT WM_APP_SERVICEUPDATE          = WM_APP + 0;
	constexpr UINT WM_APP_IMAGESAVE              = WM_APP + 2;
	constexpr UINT WM_APP_TRAYICON               = WM_APP + 3;
	constexpr UINT WM_APP_QUERYPORT              = WM_APP + 5;  // (*)
	constexpr UINT WM_APP_FILEWRITEERROR         = WM_APP + 6;
	constexpr UINT WM_APP_VIDEOSIZECHANGED       = WM_APP + 7;
	constexpr UINT WM_APP_CONTROLLERFOCUS        = WM_APP + 11; // (*)
	constexpr UINT WM_APP_EPGLOADED              = WM_APP + 12;
	constexpr UINT WM_APP_PLUGINMESSAGE          = WM_APP + 13;
	constexpr UINT WM_APP_SHOWNOTIFICATIONBAR    = WM_APP + 14;
	constexpr UINT WM_APP_SERVICECHANGED         = WM_APP + 15;
	constexpr UINT WM_APP_VIDEOSTREAMTYPECHANGED = WM_APP + 16;
	constexpr UINT WM_APP_SERVICEINFOUPDATED     = WM_APP + 17;
	constexpr UINT WM_APP_AUDIOLISTCHANGED       = WM_APP + 18;
	constexpr UINT WM_APP_SPDIFPASSTHROUGHERROR  = WM_APP + 19;
	constexpr UINT WM_APP_UPDATECLOCK            = WM_APP + 20;

	enum {
		CONTAINER_ID_VIEW = 1,
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

	class CMainWindow
		: public CBasicWindow
		, public CUISkin
		, public CUIBase
		, public COSDManager::CEventHandler
	{
	public:
		static const DWORD HIDE_CURSOR_DELAY = 1000UL;

		struct ResumeInfo
		{
			enum class ViewerSuspendFlag : unsigned int {
				None      = 0x0000U,
				Minimize  = 0x0001U,
				Standby   = 0x0002U,
				Suspend   = 0x0004U,
				EPGUpdate = 0x0008U,
			};

			CChannelSpec Channel;
			bool fOpenTuner = false;
			bool fSetChannel = false;
			bool fEnableViewer = false;
			ViewerSuspendFlag ViewerSuspendFlags = ViewerSuspendFlag::None;
			bool fFullscreen = false;
		};

		CMainWindow(CAppMain &App);
		~CMainWindow();

		bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0);
		bool Show(int CmdShow, bool fForce = false);
		void CreatePanel();
		void AdjustWindowSize(int Width, int Height, bool fScreenSize = true);
		bool ReadSettings(CSettings &Settings);
		bool WriteSettings(CSettings &Settings);
		void ShowPanel(bool fShow);
		void SetStatusBarVisible(bool fVisible);
		bool GetStatusBarVisible() const { return m_fShowStatusBar; }
		bool ShowStatusBarItem(int ID, bool fShow);
		void OnStatusBarInitialized();
		void OnStatusBarTraceEnd();
		void SetTitleBarVisible(bool fVisible);
		bool GetTitleBarVisible() const { return m_fShowTitleBar; }
		void SetCustomTitleBar(bool fCustom);
		bool GetCustomTitleBar() const { return m_fCustomTitleBar; }
		void SetSplitTitleBar(bool fSplit);
		bool GetSplitTitleBar() const { return m_fSplitTitleBar; }
		void SetCustomFrame(bool fCustomFrame, int Width = 0);
		bool GetCustomFrame() const { return m_fCustomFrame; }
		void SetSideBarVisible(bool fVisible);
		bool GetSideBarVisible() const { return m_fShowSideBar; }
		bool OnBarMouseLeave(HWND hwnd);
		int GetPanelPaneIndex() const;
		bool IsPanelVisible() const;
		bool IsPanelPresent() const;
		bool IsFullscreenPanelVisible() const { return m_Fullscreen.IsPanelVisible(); }
		void OnPanelFloating(bool fFloating);
		void OnPanelDocking(CPanelFrame::DockingPlace Place);

		bool EnablePlayback(bool fEnable);
		bool IsPlaybackEnabled() const { return m_fEnablePlayback; }

		bool InitStandby();
		bool InitMinimize();
		ResumeInfo &GetResumeInfo() { return m_Resume; }
		bool IsMinimizeToTray() const;
		bool ConfirmExit();
		void OnMouseWheel(WPARAM wParam, LPARAM lParam, bool fHorz);

		bool IsNoAcceleratorMessage(const MSG *pMsg);
		bool BeginChannelNoInput(int Digits);
		void EndChannelNoInput(bool fDetermine = false);

		bool HandleCommand(CCommandManager::InvokeParameters &Params);

		Layout::CLayoutBase &GetLayoutBase() { return m_LayoutBase; }
		CDisplayBase &GetDisplayBase() { return m_Display.GetDisplayBase(); }
		CTitleBar &GetTitleBar() { return m_TitleBar; }
		CStatusView &GetStatusView();
		CSideBar &GetSideBar();

		enum class ShowProgramGuideFlag : unsigned int {
			None     = 0x0000U,
			OnScreen = 0x0001U,
			Popup    = 0x0002U,
		};
		struct ProgramGuideSpaceInfo
		{
			LPCTSTR pszTuner;
			LPCTSTR pszSpace;
		};
		bool ShowProgramGuide(
			bool fShow, ShowProgramGuideFlag Flags = ShowProgramGuideFlag::None,
			const ProgramGuideSpaceInfo *pSpaceInfo = nullptr);

		static bool Initialize(HINSTANCE hinst);

	// CUISkin
		HWND GetVideoHostWindow() const override;

	// CUIBase
		void SetStyle(const Style::CStyleManager *pStyleManager) override;
		void NormalizeStyle(
			const Style::CStyleManager *pStyleManager,
			const Style::CStyleScaling *pStyleScaling) override;
		void SetTheme(const Theme::CThemeManager *pThemeManager) override;

	private:
		enum {
			TIMER_ID_UPDATE              = 0x0001U,
			TIMER_ID_OSD                 = 0x0002U,
			TIMER_ID_DISPLAY             = 0x0004U,
			TIMER_ID_WHEELCHANNELSELECT  = 0x0008U,
			TIMER_ID_PROGRAMLISTUPDATE   = 0x0010U,
			TIMER_ID_PROGRAMGUIDEUPDATE  = 0x0020U,
			TIMER_ID_VIDEOSIZECHANGED    = 0x0040U,
			TIMER_ID_RESETERRORCOUNT     = 0x0080U,
			TIMER_ID_HIDETOOLTIP         = 0x0100U,
			TIMER_ID_CHANNELNO           = 0x0200U,
			TIMER_ID_HIDECURSOR          = 0x0400U,
		};

		struct MainWindowStyle
		{
			Style::Margins ScreenMargin;
			Style::Margins FullscreenMargin;
			Style::Margins ResizingMargin;

			MainWindowStyle();

			void SetStyle(const Style::CStyleManager *pStyleManager);
			void NormalizeStyle(
				const Style::CStyleManager *pStyleManager,
				const Style::CStyleScaling *pStyleScaling);
		};

		struct MainWindowTheme
		{
			Theme::BackgroundStyle FrameStyle;
			Theme::BackgroundStyle ActiveFrameStyle;
		};

		class CBarLayout
		{
		public:
			virtual ~CBarLayout() = default;
			virtual void Layout(RECT *pArea, RECT *pBarRect) = 0;

			bool IsSpot(const RECT *pArea, const POINT *pPos);
			void AdjustArea(RECT *pArea);
			void ReserveArea(RECT *pArea, bool fNoMove);
		};

		class CTitleBarManager
			: public CTitleBar::CEventHandler
			, public CBarLayout
		{
		public:
			CTitleBarManager(CMainWindow *pMainWindow, bool fMainWindow);

		// CBarLayout
			void Layout(RECT *pArea, RECT *pBarRect) override;

		// CTitleBarManager
			void EndDrag();

		private:
		// CTitleBar::CEventHandler
			bool OnClose() override;
			bool OnMinimize() override;
			bool OnMaximize() override;
			bool OnFullscreen() override;
			void OnMouseLeave() override;
			void OnLabelLButtonDown(int x, int y) override;
			void OnLabelLButtonDoubleClick(int x, int y) override;
			void OnLabelRButtonUp(int x, int y) override;
			void OnIconLButtonDown(int x, int y) override;
			void OnIconLButtonDoubleClick(int x, int y) override;
			void OnHeightChanged(int Height) override;

		// CTitleBarManager
			void ShowSystemMenu(int x, int y);

			CMainWindow *m_pMainWindow;
			bool m_fMainWindow;
			bool m_fFixed;
		};

		class CSideBarManager
			: public CSideBar::CEventHandler
			, public CBarLayout
		{
		public:
			CSideBarManager(CMainWindow *pMainWindow);

		// CBarLayout
			void Layout(RECT *pArea, RECT *pBarRect) override;

		private:
			CMainWindow *m_pMainWindow;
			bool m_fFixed;

			const CChannelInfo *GetChannelInfoByCommand(int Command);

		// CSideBar::CEventHandler
			void OnCommand(int Command) override;
			void OnRButtonUp(int x, int y) override;
			void OnMouseLeave() override;
			bool GetTooltipText(int Command, LPTSTR pszText, int MaxText) override;
			bool DrawIcon(const CSideBar::DrawIconInfo *pInfo) override;
			void OnBarWidthChanged(int BarWidth) override;
			void OnStyleChanged() override;
		};

		class CCursorTracker
		{
		public:
			CCursorTracker();

			void Reset();
			bool OnCursorMove(int x, int y);
			POINT GetLastCursorPos() const { return m_LastCursorPos; }
			void SetMoveDelta(int Delta) { m_MoveDelta = Delta; }

		private:
			int m_MoveDelta;
			POINT m_LastMovePos;
			POINT m_LastCursorPos;
		};

		class CFullscreen
			: public CCustomWindow
			, public CUIBase
		{
		public:
			CFullscreen(CMainWindow &MainWindow);
			~CFullscreen();

			bool Create(HWND hwndOwner, CMainDisplay *pDisplay);
			void ShowStatusBar(bool fShow);
			bool IsStatusBarVisible() const { return m_fShowStatusView; }
			void ShowTitleBar(bool fShow);
			bool IsTitleBarVisible() const { return m_fShowTitleBar; }
			bool IsSideBarVisible() const { return m_fShowSideBar; }
			void ShowPanel(bool fShow);
			bool IsPanelVisible() const { return m_fShowPanel; }
			bool SetPanelWidth(int Width);
			int GetPanelWidth() const { return m_PanelWidth; }
			bool SetPanelHeight(int Height);
			int GetPanelHeight() const { return m_PanelHeight; }
			bool SetPanelPlace(CPanelFrame::DockingPlace Place);
			CPanelFrame::DockingPlace GetPanelPlace() const { return m_PanelPlace; }
			void HideAllBars();
			void OnRButtonUp();
			void OnMButtonUp();
			void OnMouseMove();
			CViewWindow &GetViewWindow() { return m_ViewWindow; }
			void SetTitleFont(const Style::Font &Font);

		// CUIBase
			void SetTheme(const Theme::CThemeManager *pThemeManager) override;

			static bool Initialize(HINSTANCE hinst);

		private:
			class CPanelEventHandler
				: public CPanel::CEventHandler
			{
			public:
				CPanelEventHandler(CFullscreen &Fullscreen);

				bool OnClose() override;
				bool OnMenuPopup(HMENU hmenu) override;
				bool OnMenuSelected(int Command) override;

			private:
				CFullscreen &m_Fullscreen;
			};

			CMainWindow &m_MainWindow;
			CAppMain &m_App;
			Style::CStyleScaling m_StyleScaling;
			Layout::CLayoutBase m_LayoutBase;
			CViewWindow m_ViewWindow;
			CMainDisplay *m_pDisplay;
			CTitleBar m_TitleBar;
			CTitleBarManager m_TitleBarManager;
			CPanel m_Panel;
			CPanelEventHandler m_PanelEventHandler;
			Style::Margins m_ScreenMargin;
			bool m_fShowCursor;
			bool m_fMenu;
			bool m_fShowStatusView;
			bool m_fShowTitleBar;
			bool m_fShowSideBar;
			bool m_fShowPanel;
			CPanelFrame::DockingPlace m_PanelPlace;
			int m_PanelWidth;
			int m_PanelHeight;
			CCursorTracker m_CursorTracker;

			bool OnCreate();
			void OnMouseCommand(int Command);
			void OnLButtonDoubleClick();
			void ShowCursor(bool fShow);
			void ShowSideBar(bool fShow);
			void OnBarHide(CBasicWindow &Window);
			void OnSharedBarVisibilityChange(CBasicWindow &Window, CUIBase &UIBase, bool fVisible);
			void RestorePanel(bool fPreventForeground);
			// CBasicWindow
			bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;
			// CCustomWindow
			LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		};

		class CStatusViewEventHandler
			: public CStatusView::CEventHandler
		{
		public:
			CStatusViewEventHandler(CMainWindow *pMainWindow);

		private:
			CMainWindow *m_pMainWindow;

			void OnMouseLeave() override;
			void OnHeightChanged(int Height) override;
			void OnStyleChanged() override;
		};

		class CVideoContainerEventHandler
			: public CVideoContainerWindow::CEventHandler
		{
		public:
			CVideoContainerEventHandler(CMainWindow *pMainWindow);

		private:
			CMainWindow *m_pMainWindow;

			void OnSizeChanged(int Width, int Height) override;
		};

		class CViewWindowEventHandler
			: public CViewWindow::CEventHandler
		{
		public:
			CViewWindowEventHandler(CMainWindow *pMainWindow);

		private:
			CMainWindow *m_pMainWindow;

			void OnSizeChanged(int Width, int Height) override;
		};

		class CEpgCaptureEventHandler
			: public CEpgCaptureManager::CEventHandler
		{
		public:
			CEpgCaptureEventHandler(CMainWindow *pMainWindow);

		private:
			CMainWindow *m_pMainWindow;

			void OnBeginCapture(CEpgCaptureManager::BeginFlag Flags, CEpgCaptureManager::BeginStatus Status) override;
			void OnEndCapture(CEpgCaptureManager::EndFlag Flags) override;
			void OnChannelChanged() override;
			void OnChannelEnd(bool fComplete) override;
		};

		class CCommandEventListener
			: public CCommandManager::CEventListener
		{
		public:
			CCommandEventListener(CMainWindow *pMainWindow);

		private:
			CMainWindow *m_pMainWindow;

			void OnCommandStateChanged(int ID, CCommandManager::CommandState OldState, CCommandManager::CommandState NewState) override;
			void OnCommandRadioCheckedStateChanged(int FirstID, int LastID, int CheckedID) override;
		};

		static constexpr DWORD UPDATE_TIMER_INTERVAL = 500;

		CAppMain &m_App;
		Style::CStyleScaling m_StyleScaling;
		Layout::CLayoutBase m_LayoutBase;
		CMainDisplay m_Display;
		CTitleBar m_TitleBar;
		CTitleBarManager m_TitleBarManager;
		CSideBarManager m_SideBarManager;
		CStatusViewEventHandler m_StatusViewEventHandler;
		CVideoContainerEventHandler m_VideoContainerEventHandler;
		CViewWindowEventHandler m_ViewWindowEventHandler;
		CFullscreen m_Fullscreen;
		CNotificationBar m_NotificationBar;
		CCommandEventListener m_CommandEventListener;
		CWindowTimerManager m_Timer;

		MainWindowStyle m_Style;
		MainWindowTheme m_Theme;
		bool m_fShowStatusBar;
		bool m_fShowTitleBar;
		bool m_fCustomTitleBar;
		bool m_fPopupTitleBar;
		bool m_fSplitTitleBar;
		bool m_fShowSideBar;
		int m_PanelPaneIndex;
		bool m_fPanelVerticalAlign;
		bool m_fCustomFrame;
		int m_CustomFrameWidth;
		int m_ThinFrameWidth;
		enum {
			FRAME_NORMAL,
			FRAME_CUSTOM,
			FRAME_NONE
		};

		bool m_fEnablePlayback;

		bool m_fStandbyInit;
		bool m_fMinimizeInit;
		ResumeInfo m_Resume;

		enum class WindowState {
			Normal,
			Minimized,
			Maximized,
		};
		WindowState m_WindowState;
		bool m_fWindowRegionSet;
		bool m_fWindowFrameChanged;

		enum class WindowSizeMode {
			HD,
			OneSeg,
		};
		struct WindowSize
		{
			int Width = 0;
			int Height = 0;

			WindowSize &operator=(const RECT &rc) {
				Width = rc.right - rc.left;
				Height = rc.bottom - rc.top;
				return *this;
			}
			bool IsValid() const { return Width > 0 && Height > 0; }
		};
		WindowSizeMode m_WindowSizeMode;
		WindowSize m_HDWindowSize;
		WindowSize m_1SegWindowSize;

		bool m_fLockLayout;
		bool m_fStatusBarInitialized;

		bool m_fShowCursor;
		bool m_fNoHideCursor;
		CCursorTracker m_CursorTracker;

		bool m_fDragging;
		POINT m_ptDragStartPos;
		RECT m_rcDragStart;
		bool m_fEnterSizeMove;
		bool m_fResizePanel;

		bool m_fCreated;
		bool m_fClosing;

		CMouseWheelHandler m_WheelHandler;
		int m_WheelCount;
		int m_PrevWheelCommand;
		DWORD m_PrevWheelTime;

		DWORD m_AspectRatioResetTime;
		bool m_fForceResetPanAndScan;
		int m_DefaultAspectRatioMenuItemCount;
		int m_VideoSizeChangedTimerCount;
		unsigned int m_ProgramListUpdateTimerCount;
		bool m_fAlertedLowFreeSpace;

		class CDisplayBaseEventHandler
			: public CDisplayBase::CEventHandler
		{
			CMainWindow *m_pMainWindow;
			bool OnVisibleChange(bool fVisible) override;
		public:
			CDisplayBaseEventHandler(CMainWindow *pMainWindow);
		};
		CDisplayBaseEventHandler m_DisplayBaseEventHandler;

		CChannelInput m_ChannelInput;

		CEpgCaptureEventHandler m_EpgCaptureEventHandler;

		class CClockUpdateTimer
			: public Util::CTimer
		{
		public:
			CClockUpdateTimer(CMainWindow *pMainWindow);
		private:
			void OnTimer() override;
			CMainWindow *m_pMainWindow;
		};
		CClockUpdateTimer m_ClockUpdateTimer;
		bool m_fAccurateClock;

		struct DirectShowFilterPropertyInfo
		{
			LibISDB::ViewerFilter::PropertyFilterType Filter;
			int Command;
		};

		static const DirectShowFilterPropertyInfo m_DirectShowFilterPropertyList[];
		static ATOM m_atomChildOldWndProcProp;
		static CMainWindow *m_pThis;

	// CUISkin
		HWND GetMainWindow() const override { return m_hwnd; }
		HWND GetFullscreenWindow() const override { return m_Fullscreen.GetHandle(); }
		const CUIBase *GetUIBase() const override { return this; }
		const CUIBase *GetFullscreenUIBase() const override { return &m_Fullscreen; }
		void ShowNotificationBar(
			LPCTSTR pszText,
			CNotificationBar::MessageType Type = CNotificationBar::MessageType::Info,
			DWORD Duration = 0, bool fSkippable = false) override;
		bool InitializeViewer(BYTE VideoStreamType = 0) override;
		bool FinalizeViewer() override;
		bool EnableViewer(bool fEnable) override;
		bool IsViewerEnabled() const override;
		HWND GetViewerWindow() const override;
		bool SetZoomRate(int Rate, int Factor) override;
		bool GetZoomRate(int *pRate, int *pFactor) override;
		void SetTitleText(LPCTSTR pszTitleText, LPCTSTR pszWindowText) override;
		bool SetTitleFont(const Style::Font &Font) override;
		bool SetLogo(HBITMAP hbm) override;
		bool SetAlwaysOnTop(bool fTop) override;
		bool SetFullscreen(bool fFullscreen) override;
		bool SetStandby(bool fStandby) override;
		bool ShowVolumeOSD() override;
		void PreventDisplaySleep(bool fPrevent) override;
		void BeginWheelChannelSelect(DWORD Delay) override;
		void EndWheelChannelSelect() override;

	// CAppEventHandler
		void OnTunerChanged() override;
		void OnTunerOpened() override;
		void OnTunerClosed() override;
		void OnTunerShutDown() override;
		void OnChannelChanged(AppEvent::ChannelChangeStatus Status) override;
		void OnServiceChanged() override;
		void OnChannelListChanged() override;
		void OnRecordingStarted() override;
		void OnRecordingStopped() override;
		void OnRecordingPaused() override;
		void OnRecordingResumed() override;
		void On1SegModeChanged(bool f1SegMode) override;
		void OnPanAndScanChanged() override;
		void OnAspectRatioTypeChanged(int Type) override;
		void OnVolumeChanged(int Volume) override;
		void OnMuteChanged(bool fMute) override;
		void OnDualMonoModeChanged(LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode Mode) override;
		void OnStereoModeChanged(LibISDB::DirectShow::AudioDecoderFilter::StereoMode Mode) override;
		void OnAudioStreamChanged(int Stream) override;
		void OnStartupDone() override;
		void OnVariableChanged() override;

	// COSDManager::CEventHandler
		bool GetOSDClientInfo(COSDManager::OSDClientInfo *pInfo) override;
		bool SetOSDHideTimer(DWORD Delay) override;

	// CMainWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		bool OnNCCreate(const CREATESTRUCT *pcs);
		bool OnCreate(const CREATESTRUCT *pcs);
		void OnDestroy();
		void OnSizeChanged(UINT State, int Width, int Height);
		bool OnSizeChanging(UINT Edge, RECT *pRect);
		void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO pmmi);
		void OnMouseMove(int x, int y);
		bool OnSetCursor(HWND hwndCursor, int HitTestCode, int MouseMessage);
		bool OnKeyDown(WPARAM KeyCode);
		bool OnSysCommand(UINT Command);
		void OnTimer(HWND hwnd, UINT id);
		bool OnInitMenuPopup(HMENU hmenu);
		bool OnClose(HWND hwnd);
		void OnRecordingStateChanged();
		void OnEventChanged();
		bool AutoFitWindowToVideo();
		bool SetPanAndScan(int Command);
		void LockLayout();
		void UpdateLayout();
		void UpdateLayoutStructure();
		void AdjustWindowSizeOnDockPanel(bool fDock);
		void GetPanelDockingAdjustedPos(bool fDock, RECT *pPos) const;
		void SetFixedPaneSize(int SplitterID, int ContainerID, int Width, int Height);
		void ShowPopupTitleBar(bool fShow);
		void ShowPopupStatusBar(bool fShow);
		void ShowPopupSideBar(bool fShow);
		void ShowCursor(bool fShow);
		void ShowChannelOSD();
		void SetWindowVisible();
		void ShowFloatingWindows(bool fShow, bool fNoProgramGuide = false);
		void DrawCustomFrame(bool fActive);
		CViewWindow &GetCurrentViewWindow();
		void StoreTunerResumeInfo();
		bool ResumeTuner();
		void ResumeChannel();
		void SuspendViewer(ResumeInfo::ViewerSuspendFlag Flags);
		void ResumeViewer(ResumeInfo::ViewerSuspendFlag Flags);
		void OnChannelNoInput();
		void HookWindows(HWND hwnd);
		void HookChildWindow(HWND hwnd);
		void SetMaximizedRegion(bool fSet);
		void UpdateWindowFrame();
		void SendCommand(int Command) { SendMessage(WM_COMMAND, Command, 0); }
		void PostCommand(int Command) { PostMessage(WM_COMMAND, Command, 0); }

		static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK ChildHookProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	};

	TVTEST_ENUM_FLAGS(CMainWindow::ResumeInfo::ViewerSuspendFlag)
	TVTEST_ENUM_FLAGS(CMainWindow::ShowProgramGuideFlag)

}	// namespace TVTest


#endif
