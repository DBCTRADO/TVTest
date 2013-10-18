#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H


#include "UISkin.h"
#include "View.h"
#include "ChannelManager.h"
#include "Layout.h"
#include "TitleBar.h"
#include "StatusView.h"
#include "Settings.h"
#include "NotificationBar.h"
#include "Panel.h"
#include "OSDManager.h"


#define MAIN_WINDOW_CLASS		APP_NAME TEXT(" Window")
#define FULLSCREEN_WINDOW_CLASS	APP_NAME TEXT(" Fullscreen")

// (*) が付いたものは、変えると異なるバージョン間での互換性が無くなるので注意
#define WM_APP_SERVICEUPDATE	(WM_APP+0)
#define WM_APP_CHANNELCHANGE	(WM_APP+1)
#define WM_APP_IMAGESAVE		(WM_APP+2)
#define WM_APP_TRAYICON			(WM_APP+3)
#define WM_APP_EXECUTE			(WM_APP+4)
#define WM_APP_QUERYPORT		(WM_APP+5)	// (*)
#define WM_APP_FILEWRITEERROR	(WM_APP+6)
#define WM_APP_VIDEOSIZECHANGED	(WM_APP+7)
#define WM_APP_EMMPROCESSED		(WM_APP+8)
#define WM_APP_ECMERROR			(WM_APP+9)
#define WM_APP_ECMREFUSED		(WM_APP+10)
#define WM_APP_CONTROLLERFOCUS	(WM_APP+11)	// (*)
#define WM_APP_EPGLOADED		(WM_APP+12)
#define WM_APP_PLUGINMESSAGE	(WM_APP+13)
#define WM_APP_CARDREADERHUNG	(WM_APP+14)
#define WM_APP_SERVICECHANGED	(WM_APP+15)

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


class CBasicViewer
{
protected:
	CDtvEngine *m_pDtvEngine;
	bool m_fEnabled;
	CViewWindow m_ViewWindow;
	CVideoContainerWindow m_VideoContainer;
	CDisplayBase m_DisplayBase;

public:
	CBasicViewer(CDtvEngine *pDtvEngine);
	bool Create(HWND hwndParent,int ViewID,int ContainerID,HWND hwndMessage);
	bool EnableViewer(bool fEnable);
	bool IsViewerEnabled() const { return m_fEnabled; }
	bool BuildViewer();
	bool CloseViewer();
	CViewWindow &GetViewWindow() { return m_ViewWindow; }
	const CViewWindow &GetViewWindow() const { return m_ViewWindow; }
	CVideoContainerWindow &GetVideoContainer() { return m_VideoContainer; }
	const CVideoContainerWindow &GetVideoContainer() const { return m_VideoContainer; }
	CDisplayBase &GetDisplayBase() { return m_DisplayBase; }
	const CDisplayBase &GetDisplayBase() const { return m_DisplayBase; }
};

class CFullscreen : public CCustomWindow
{
	Layout::CLayoutBase m_LayoutBase;
	CViewWindow m_ViewWindow;
	CBasicViewer *m_pViewer;
	CTitleBar m_TitleBar;
	CPanel m_Panel;
	class CPanelEventHandler : public CPanel::CEventHandler {
		bool OnClose();
	};
	CPanelEventHandler m_PanelEventHandler;
	bool m_fShowCursor;
	bool m_fMenu;
	bool m_fShowStatusView;
	bool m_fShowTitleBar;
	bool m_fShowSideBar;
	bool m_fShowPanel;
	int m_PanelWidth;
	POINT m_LastCursorMovePos;
	enum {
		TIMER_ID_HIDECURSOR=1
	};
	enum {
		HIDE_CURSOR_DELAY=1000UL
	};

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

public:
	CFullscreen();
	~CFullscreen();
	bool Create(HWND hwndOwner,CBasicViewer *pViewer);
	void ShowPanel(bool fShow);
	bool IsPanelVisible() const { return m_fShowPanel; }
	bool SetPanelWidth(int Width);
	int GetPanelWidth() const { return m_PanelWidth; }
	void OnRButtonDown();
	void OnMButtonDown();
	void OnMouseMove();

	static bool Initialize();
};

class CMainWindow
	: public CBasicWindow
	, public CUISkin
	, public COSDManager::CEventHandler
{
public:
	enum { COMMAND_FROM_MOUSE=8 };

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

	CMainWindow();
	~CMainWindow();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	bool Show(int CmdShow);
	void CreatePanel();
	void ShowNotificationBar(LPCTSTR pszText,
							 CNotificationBar::MessageType Type=CNotificationBar::MESSAGE_INFO,
							 DWORD Duration=0);
	void AdjustWindowSize(int Width,int Height);
	bool ReadSettings(CSettings &Settings);
	bool WriteSettings(CSettings &Settings);
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
	bool IsFullscreenPanelVisible() const { return m_Fullscreen.IsPanelVisible(); }
	int GetAspectRatioType() const { return m_AspectRatioType; }
	bool InitStandby();
	bool InitMinimize();
	ResumeInfo &GetResumeInfo() { return m_Resume; }
	bool IsMinimizeToTray() const;
	bool ConfirmExit();
	void OnMouseWheel(WPARAM wParam,LPARAM lParam,bool fHorz);
	void SendCommand(int Command) { SendMessage(WM_COMMAND,Command,0); }
	void PostCommand(int Command) { PostMessage(WM_COMMAND,Command,0); }
	bool CommandLineRecord(LPCTSTR pszFileName,const FILETIME *pStartTime,int Delay,int Duration);

	bool BeginChannelNoInput(int Digits);
	void EndChannelNoInput();
	bool OnChannelNoInput(int Number);

	bool BeginProgramGuideUpdate(LPCTSTR pszBonDriver=NULL,const CChannelList *pChannelList=NULL,bool fStandby=false);
	enum {
		EPG_UPDATE_END_CLOSE_TUNER = 0x0001U,
		EPG_UPDATE_END_RESUME      = 0x0002U,
		EPG_UPDATE_END_DEFAULT     = EPG_UPDATE_END_CLOSE_TUNER | EPG_UPDATE_END_RESUME
	};
	void OnProgramGuideUpdateEnd(unsigned int Flags=EPG_UPDATE_END_DEFAULT);
	void EndProgramGuideUpdate(unsigned int Flags=EPG_UPDATE_END_DEFAULT);

	void UpdatePanel();
	void ApplyColorScheme(const class CColorScheme *pColorScheme);
	bool SetLogo(LPCTSTR pszFileName);
	bool SetViewWindowEdge(bool fEdge);
	bool GetViewWindowEdge() const { return m_fViewWindowEdge; }
	bool GetExitOnRecordingStop() const { return m_fExitOnRecordingStop; }
	void SetExitOnRecordingStop(bool fExit) { m_fExitOnRecordingStop=fExit; }

	CStatusView *GetStatusView() const;
	Layout::CLayoutBase &GetLayoutBase() { return m_LayoutBase; }
	CDisplayBase &GetDisplayBase() { return m_Viewer.GetDisplayBase(); }
	CTitleBar &GetTitleBar() { return m_TitleBar; }

	bool UpdateProgramInfo();

	enum {
		PROGRAMGUIDE_SHOW_ONSCREEN = 0x0001U,
		PROGRAMGUIDE_SHOW_POPUP    = 0x0002U
	};
	struct ProgramGuideSpaceInfo {
		LPCTSTR pszTuner;
		int Space;
	};
	bool ShowProgramGuide(bool fShow,unsigned int Flags=0,const ProgramGuideSpaceInfo *pSpaceInfo=NULL);

	static bool Initialize();

// CUISkin
	HWND GetVideoHostWindow() const override;

private:
	enum { UPDATE_TIMER_INTERVAL=500 };

	Layout::CLayoutBase m_LayoutBase;
	CBasicViewer m_Viewer;
	CTitleBar m_TitleBar;
	CFullscreen m_Fullscreen;

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

	bool m_fStandbyInit;
	bool m_fMinimizeInit;
	ResumeInfo m_Resume;

	struct EpgChannelGroup {
		int Space;
		int Channel;
		DWORD Time;
		CChannelList ChannelList;
	};
	bool m_fProgramGuideUpdating;
	int m_EpgUpdateCurChannel;
	std::vector<EpgChannelGroup> m_EpgUpdateChannelList;
	Util::CClock m_EpgAccumulateClock;
	bool m_fEpgUpdateChannelChange;

	bool m_fExitOnRecordingStop;
	POINT m_ptDragStartPos;
	RECT m_rcDragStart;
	bool m_fClosing;

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

	static ATOM m_atomChildOldWndProcProp;

// CUISkin
	HWND GetMainWindow() const override { return m_hwnd; }
	bool InitializeViewer() override;
	bool FinalizeViewer() override;
	bool EnableViewer(bool fEnable) override;
	bool IsViewerEnabled() const override;
	HWND GetViewerWindow() const override;
	bool SetZoomRate(int Rate,int Factor) override;
	bool GetZoomRate(int *pRate,int *pFactor) override;
	bool SetPanAndScan(const PanAndScanInfo &Info) override;
	bool GetPanAndScan(PanAndScanInfo *pInfo) const override;
	void OnVolumeChanged(bool fOSD) override;
	void OnMuteChanged() override;
	void OnStereoModeChanged() override;
	void OnAudioStreamChanged() override;
	bool OnStandbyChange(bool fStandby) override;
	bool OnFullscreenChange(bool fFullscreen) override;
	bool SetAlwaysOnTop(bool fTop) override;
	void OnTunerChanged() override;
	void OnTunerOpened() override;
	void OnTunerClosed() override;
	void OnChannelListChanged() override;
	void OnChannelChanged(unsigned int Status) override;
	void OnServiceChanged() override;
	void OnRecordingStarted() override;
	void OnRecordingStopped() override;

// COSDManager::CEventHandler
	bool GetOSDWindow(HWND *phwndParent,RECT *pRect,bool *pfForcePseudoOSD) override;
	bool SetOSDHideTimer(DWORD Delay) override;

// CMainWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	bool OnCreate(const CREATESTRUCT *pcs);
	void OnSizeChanged(UINT State,int Width,int Height);
	bool OnSizeChanging(UINT Edge,RECT *pRect);
	void OnMouseMove(int x,int y);
	void OnCommand(HWND hwnd,int id,HWND hwndCtl,UINT codeNotify);
	void OnTimer(HWND hwnd,UINT id);
	bool OnInitMenuPopup(HMENU hmenu);
	void AutoSelectStereoMode();
	bool OnExecute(LPCTSTR pszCmdLine);
	int GetZoomPercentage();
	bool SetPanAndScan(int Command);
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
	void HookWindows(HWND hwnd);
	void HookChildWindow(HWND hwnd);

	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static DWORD WINAPI ExitWatchThread(LPVOID lpParameter);
	static LRESULT CALLBACK ChildHookProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
