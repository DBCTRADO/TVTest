#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "MiscDialog.h"
#include "DialogUtil.h"
#include "DrawUtil.h"
#include "WindowUtil.h"
#include "PseudoOSD.h"
#include "EventInfoPopup.h"
#include "ToolTip.h"
#include "HelperClass/StdUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"

#pragma comment(lib,"imm32.lib")	// for ImmAssociateContext(Ex)


#define MAIN_TITLE_TEXT APP_NAME


using namespace TVTest;




static int CalcZoomSize(int Size,int Rate,int Factor)
{
	if (Factor==0)
		return 0;
	return ::MulDiv(Size,Rate,Factor);
}




const BYTE CMainWindow::m_AudioGainList[] = {100, 125, 150, 200};

const CMainWindow::DirectShowFilterPropertyInfo CMainWindow::m_DirectShowFilterPropertyList[] = {
	{CMediaViewer::PROPERTY_FILTER_VIDEODECODER,		CM_VIDEODECODERPROPERTY},
	{CMediaViewer::PROPERTY_FILTER_VIDEORENDERER,		CM_VIDEORENDERERPROPERTY},
	{CMediaViewer::PROPERTY_FILTER_AUDIOFILTER,			CM_AUDIOFILTERPROPERTY},
	{CMediaViewer::PROPERTY_FILTER_AUDIORENDERER,		CM_AUDIORENDERERPROPERTY},
	{CMediaViewer::PROPERTY_FILTER_MPEG2DEMULTIPLEXER,	CM_DEMULTIPLEXERPROPERTY},
};

ATOM CMainWindow::m_atomChildOldWndProcProp=0;


bool CMainWindow::Initialize(HINSTANCE hinst)
{
	WNDCLASSEX wc;

	wc.cbSize=sizeof(WNDCLASSEX);
	wc.style=0;
	wc.lpfnWndProc=WndProc;
	wc.cbClsExtra=0;
	wc.cbWndExtra=0;
	wc.hInstance=hinst;
	wc.hIcon=CAppMain::GetAppIcon();
	wc.hCursor=::LoadCursor(nullptr,IDC_ARROW);
	wc.hbrBackground=(HBRUSH)(COLOR_3DFACE+1);
	wc.lpszMenuName=nullptr;
	wc.lpszClassName=MAIN_WINDOW_CLASS;
	wc.hIconSm=CAppMain::GetAppIconSmall();
	return ::RegisterClassEx(&wc)!=0 && CFullscreen::Initialize(hinst);
}


CMainWindow::CMainWindow(CAppMain &App)
	: m_App(App)
	, m_Display(App)
	, m_TitleBarManager(this,true)
	, m_SideBarManager(this)
	, m_StatusViewEventHandler(this)
	, m_VideoContainerEventHandler(this)
	, m_ViewWindowEventHandler(this)
	, m_Fullscreen(*this)
	, m_CommandEventHandler(this)

	, m_fShowStatusBar(true)
	, m_fShowTitleBar(true)
	, m_fCustomTitleBar(true)
	, m_fPopupTitleBar(true)
	, m_fSplitTitleBar(true)
	, m_fShowSideBar(false)
	, m_PanelPaneIndex(1)
	, m_fPanelVerticalAlign(false)
	, m_fCustomFrame(false)
	, m_CustomFrameWidth(0)
	, m_ThinFrameWidth(1)

	, m_fEnablePlayback(true)

	, m_fStandbyInit(false)
	, m_fMinimizeInit(false)

	, m_WindowSizeMode(WINDOW_SIZE_HD)

	, m_fLockLayout(false)
	, m_fStatusBarInitialized(false)

	, m_fShowCursor(true)
	, m_fNoHideCursor(false)

	, m_fDragging(false)
	, m_fEnterSizeMove(false)
	, m_fResizePanel(false)

	, m_fClosing(false)

	, m_WheelCount(0)
	, m_PrevWheelCommand(0)
	, m_PrevWheelTime(0)

	, m_AspectRatioType(ASPECTRATIO_DEFAULT)
	, m_AspectRatioResetTime(0)
	, m_fForceResetPanAndScan(false)
	, m_DefaultAspectRatioMenuItemCount(0)
	, m_fFrameCut(false)
	, m_ProgramListUpdateTimerCount(0)
	, m_fAlertedLowFreeSpace(false)
	, m_ResetErrorCountTimer(TIMER_ID_RESETERRORCOUNT)
	, m_ChannelInput(App.Accelerator.GetChannelInputOptions())
	, m_ChannelNoInputTimer(TIMER_ID_CHANNELNO)
	, m_DisplayBaseEventHandler(this)
	, m_EpgCaptureEventHandler(this)
{
	// 適当にデフォルトサイズを設定
#ifndef TVTEST_FOR_1SEG
	m_WindowPosition.Width=960;
	m_WindowPosition.Height=540;
#else
	m_WindowPosition.Width=400;
	m_WindowPosition.Height=320;
#endif
	m_WindowPosition.Left=
		(::GetSystemMetrics(SM_CXSCREEN)-m_WindowPosition.Width)/2;
	m_WindowPosition.Top=
		(::GetSystemMetrics(SM_CYSCREEN)-m_WindowPosition.Height)/2;
}


CMainWindow::~CMainWindow()
{
	Destroy();
	if (m_atomChildOldWndProcProp!=0) {
		::GlobalDeleteAtom(m_atomChildOldWndProcProp);
		m_atomChildOldWndProcProp=0;
	}
}


bool CMainWindow::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	if (m_pCore->GetAlwaysOnTop())
		ExStyle|=WS_EX_TOPMOST;
	if (!CreateBasicWindow(nullptr,Style,ExStyle,ID,MAIN_WINDOW_CLASS,MAIN_TITLE_TEXT,m_App.GetInstance()))
		return false;
	return true;
}


bool CMainWindow::Show(int CmdShow)
{
	return ::ShowWindow(m_hwnd,m_WindowPosition.fMaximized?SW_SHOWMAXIMIZED:CmdShow)!=FALSE;
}


void CMainWindow::CreatePanel()
{
	const SIZE IconDrawSize=m_App.Panel.Form.GetIconDrawSize();
	LPCTSTR pszIconImage;
	int IconSize;
	if (IconDrawSize.cx<=16 && IconDrawSize.cy<=16) {
		pszIconImage=MAKEINTRESOURCE(IDB_PANELTABICONS16);
		IconSize=16;
	} else {
		pszIconImage=MAKEINTRESOURCE(IDB_PANELTABICONS32);
		IconSize=32;
	}
	HBITMAP hbm=(HBITMAP)::LoadImage(m_App.GetResourceInstance(),pszIconImage,
									 IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
	m_App.Panel.Form.SetIconImage(hbm,IconSize,IconSize);
	::DeleteObject(hbm);

	m_App.Panel.Form.SetTabFont(m_App.PanelOptions.GetFont());
	m_App.Panel.Form.SetTabStyle(m_App.PanelOptions.GetTabStyle());
	m_App.Panel.Form.Create(m_hwnd,WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

	CPanelForm::PageInfo PageInfo;

	m_App.Panel.InfoPanel.SetProgramInfoRichEdit(m_App.PanelOptions.GetProgramInfoUseRichEdit());
	m_App.Panel.InfoPanel.Create(m_App.Panel.Form.GetHandle(),WS_CHILD | WS_CLIPCHILDREN);
	m_App.Panel.InfoPanel.GetItem<CInformationPanel::CSignalLevelItem>()->ShowSignalLevel(
		!m_App.DriverOptions.IsNoSignalLevel(m_App.CoreEngine.GetDriverFileName()));
	PageInfo.pPage=&m_App.Panel.InfoPanel;
	PageInfo.pszTitle=TEXT("情報");
	PageInfo.ID=PANEL_ID_INFORMATION;
	PageInfo.Icon=0;
	PageInfo.fVisible=true;
	m_App.Panel.Form.AddPage(PageInfo);

	m_App.Panel.ProgramListPanel.SetEpgProgramList(&m_App.EpgProgramList);
	m_App.Panel.ProgramListPanel.SetVisibleEventIcons(m_App.ProgramGuideOptions.GetVisibleEventIcons());
	m_App.Panel.ProgramListPanel.Create(m_App.Panel.Form.GetHandle(),WS_CHILD | WS_VSCROLL);
	PageInfo.pPage=&m_App.Panel.ProgramListPanel;
	PageInfo.pszTitle=TEXT("番組表");
	PageInfo.ID=PANEL_ID_PROGRAMLIST;
	PageInfo.Icon=1;
	m_App.Panel.Form.AddPage(PageInfo);

	m_App.Panel.ChannelPanel.SetEpgProgramList(&m_App.EpgProgramList);
	m_App.Panel.ChannelPanel.SetLogoManager(&m_App.LogoManager);
	m_App.Panel.ChannelPanel.Create(m_App.Panel.Form.GetHandle(),WS_CHILD | WS_VSCROLL);
	PageInfo.pPage=&m_App.Panel.ChannelPanel;
	PageInfo.pszTitle=TEXT("チャンネル");
	PageInfo.ID=PANEL_ID_CHANNEL;
	PageInfo.Icon=2;
	m_App.Panel.Form.AddPage(PageInfo);

	m_App.Panel.ControlPanel.SetSendMessageWindow(m_hwnd);
	m_App.Panel.InitControlPanel();
	m_App.Panel.ControlPanel.Create(m_App.Panel.Form.GetHandle(),WS_CHILD);
	PageInfo.pPage=&m_App.Panel.ControlPanel;
	PageInfo.pszTitle=TEXT("操作");
	PageInfo.ID=PANEL_ID_CONTROL;
	PageInfo.Icon=3;
	m_App.Panel.Form.AddPage(PageInfo);

	m_App.Panel.CaptionPanel.Create(m_App.Panel.Form.GetHandle(),WS_CHILD | WS_CLIPCHILDREN);
	PageInfo.pPage=&m_App.Panel.CaptionPanel;
	PageInfo.pszTitle=TEXT("字幕");
	PageInfo.ID=PANEL_ID_CAPTION;
	PageInfo.Icon=4;
	m_App.Panel.Form.AddPage(PageInfo);

	m_App.PluginManager.RegisterPanelItems();

	m_App.PanelOptions.InitializePanelForm(&m_App.Panel.Form);
	m_App.Panel.Frame.Create(m_hwnd,
		dynamic_cast<Layout::CSplitter*>(m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER)),
		CONTAINER_ID_PANEL,&m_App.Panel.Form,TEXT("パネル"));

	if (m_fCustomFrame) {
		HookWindows(m_App.Panel.Frame.GetPanel()->GetHandle());
	}
}


bool CMainWindow::InitializeViewer(BYTE VideoStreamType)
{
	const bool fEnableViewer=IsViewerEnabled();

	m_pCore->SetStatusBarTrace(true);

	if (m_Display.BuildViewer(VideoStreamType)) {
		CMediaViewer &MediaViewer=m_App.CoreEngine.m_DtvEngine.m_MediaViewer;

		m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_VIDEODECODER);
		m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_VIDEORENDERER);
		m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_AUDIODEVICE);

		if (fEnableViewer)
			m_pCore->EnableViewer(true);
		if (m_fCustomFrame)
			HookWindows(m_Display.GetVideoContainer().GetHandle());
	} else {
		FinalizeViewer();
	}

	m_pCore->SetStatusBarTrace(false);

	return true;
}


bool CMainWindow::FinalizeViewer()
{
	m_Display.CloseViewer();

	m_fEnablePlayback=false;

	m_pCore->SetCommandCheckedState(CM_DISABLEVIEWER,true);

	m_App.Panel.InfoPanel.ResetItem(CInformationPanel::ITEM_VIDEODECODER);
	m_App.Panel.InfoPanel.ResetItem(CInformationPanel::ITEM_VIDEORENDERER);
	m_App.Panel.InfoPanel.ResetItem(CInformationPanel::ITEM_AUDIODEVICE);

	return true;
}


bool CMainWindow::SetFullscreen(bool fFullscreen)
{
	if (fFullscreen) {
		if (::IsIconic(m_hwnd))
			::ShowWindow(m_hwnd,SW_RESTORE);
		if (!m_Fullscreen.Create(m_hwnd,&m_Display))
			return false;
	} else {
		ForegroundWindow(m_hwnd);
		m_Fullscreen.Destroy();
		m_App.CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(
			m_fFrameCut?CMediaViewer::STRETCH_CUTFRAME:
						CMediaViewer::STRETCH_KEEPASPECTRATIO);
	}
	m_App.StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
	m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VIDEO);
	m_pCore->SetCommandCheckedState(CM_FULLSCREEN,fFullscreen);
	m_pCore->SetCommandCheckedState(CM_PANEL,
		fFullscreen?m_Fullscreen.IsPanelVisible():m_App.Panel.fShowPanelWindow);
	return true;
}


HWND CMainWindow::GetVideoHostWindow() const
{
	if (m_pCore->GetStandby())
		return nullptr;
	if (m_pCore->GetFullscreen())
		return m_Fullscreen.GetHandle();
	return m_hwnd;
}


void CMainWindow::ShowNotificationBar(LPCTSTR pszText,
									  CNotificationBar::MessageType Type,
									  DWORD Duration,bool fSkippable)
{
	m_NotificationBar.SetFont(m_App.OSDOptions.GetNotificationBarFont());
	m_NotificationBar.Show(
		pszText,Type,
		max((DWORD)m_App.OSDOptions.GetNotificationBarDuration(),Duration),
		fSkippable);
}


void CMainWindow::AdjustWindowSize(int Width,int Height,bool fScreenSize)
{
	RECT rcOld,rc;

	GetPosition(&rcOld);

	if (fScreenSize) {
		::SetRect(&rc,0,0,Width,Height);
		m_Display.GetViewWindow().CalcWindowRect(&rc);
		Width=rc.right-rc.left;
		Height=rc.bottom-rc.top;
		m_LayoutBase.GetScreenPosition(&rc);
		rc.right=rc.left+Width;
		rc.bottom=rc.top+Height;
		if (m_fShowTitleBar && m_fCustomTitleBar)
			m_TitleBarManager.ReserveArea(&rc,true);
		if (m_fShowSideBar)
			m_SideBarManager.ReserveArea(&rc,true);
		if (m_App.Panel.fShowPanelWindow && !m_App.Panel.IsFloating()) {
			Layout::CSplitter *pSplitter=dynamic_cast<Layout::CSplitter*>(
				m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));
			if (m_App.Panel.Frame.IsDockingVertical())
				rc.bottom+=pSplitter->GetBarWidth()+pSplitter->GetPaneSize(CONTAINER_ID_PANEL);
			else
				rc.right+=pSplitter->GetBarWidth()+pSplitter->GetPaneSize(CONTAINER_ID_PANEL);
		}
		if (m_fShowStatusBar)
			rc.bottom+=m_App.StatusView.CalcHeight(rc.right-rc.left);
		if (m_fCustomFrame)
			::InflateRect(&rc,m_CustomFrameWidth,m_CustomFrameWidth);
		else
			CalcPositionFromClientRect(&rc);
	} else {
		rc.left=rcOld.left;
		rc.top=rcOld.top;
		rc.right=rc.left+Width;
		rc.bottom=rc.top+Height;
	}

	if (::EqualRect(&rc,&rcOld))
		return;

	const HMONITOR hMonitor=::MonitorFromRect(&rcOld,MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize=sizeof(mi);
	::GetMonitorInfo(hMonitor,&mi);

	if (m_App.ViewOptions.GetNearCornerResizeOrigin()) {
		if (abs(rcOld.left-mi.rcWork.left)>abs(rcOld.right-mi.rcWork.right)) {
			rc.left=rcOld.right-(rc.right-rc.left);
			rc.right=rcOld.right;
		}
		if (abs(rcOld.top-mi.rcWork.top)>abs(rcOld.bottom-mi.rcWork.bottom)) {
			rc.top=rcOld.bottom-(rc.bottom-rc.top);
			rc.bottom=rcOld.bottom;
		}
	}

	// ウィンドウがモニタの外に出ないようにする
	if (rcOld.left>=mi.rcWork.left && rcOld.top>=mi.rcWork.top
			&& rcOld.right<=mi.rcWork.right && rcOld.bottom<=mi.rcWork.bottom) {
		if (rc.right>mi.rcWork.right && rc.left>mi.rcWork.left)
			::OffsetRect(&rc,max(mi.rcWork.right-rc.right,mi.rcWork.left-rc.left),0);
		if (rc.bottom>mi.rcWork.bottom && rc.top>mi.rcWork.top)
			::OffsetRect(&rc,0,max(mi.rcWork.bottom-rc.bottom,mi.rcWork.top-rc.top));
	}

	SetPosition(&rc);
	m_App.Panel.OnOwnerMovingOrSizing(&rcOld,&rc);
}


bool CMainWindow::ReadSettings(CSettings &Settings)
{
	int Left,Top,Width,Height,Value;
	bool f;

	GetPosition(&Left,&Top,&Width,&Height);
	Settings.Read(TEXT("WindowLeft"),&Left);
	Settings.Read(TEXT("WindowTop"),&Top);
	Settings.Read(TEXT("WindowWidth"),&Width);
	Settings.Read(TEXT("WindowHeight"),&Height);
	SetPosition(Left,Top,Width,Height);
	MoveToMonitorInside();
	if (Settings.Read(TEXT("WindowMaximize"),&f))
		SetMaximize(f);

	Settings.Read(TEXT("WindowSize.HD.Width"),&m_HDWindowSize.Width);
	Settings.Read(TEXT("WindowSize.HD.Height"),&m_HDWindowSize.Height);
	Settings.Read(TEXT("WindowSize.1Seg.Width"),&m_1SegWindowSize.Width);
	Settings.Read(TEXT("WindowSize.1Seg.Height"),&m_1SegWindowSize.Height);
	if (m_HDWindowSize.Width==Width && m_HDWindowSize.Height==Height)
		m_WindowSizeMode=WINDOW_SIZE_HD;
	else if (m_1SegWindowSize.Width==Width && m_1SegWindowSize.Height==Height)
		m_WindowSizeMode=WINDOW_SIZE_1SEG;

	if (Settings.Read(TEXT("AlwaysOnTop"),&f))
		m_pCore->SetAlwaysOnTop(f);
	if (Settings.Read(TEXT("ShowStatusBar"),&f))
		SetStatusBarVisible(f);
	if (Settings.Read(TEXT("ShowTitleBar"),&f))
		SetTitleBarVisible(f);
	Settings.Read(TEXT("PopupTitleBar"),&m_fPopupTitleBar);
	if (Settings.Read(TEXT("PanelDockingIndex"),&Value)
			&& (Value==0 || Value==1))
		m_PanelPaneIndex=Value;
	Settings.Read(TEXT("PanelVerticalAlign"),&m_fPanelVerticalAlign);
	if (Settings.Read(TEXT("FullscreenPanelWidth"),&Value))
		m_Fullscreen.SetPanelWidth(Value);
	if (Settings.Read(TEXT("FullscreenPanelHeight"),&Value))
		m_Fullscreen.SetPanelHeight(Value);
	if (Settings.Read(TEXT("FullscreenPanelPlace"),&Value))
		m_Fullscreen.SetPanelPlace((CPanelFrame::DockingPlace)Value);
	if (Settings.Read(TEXT("ThinFrameWidth"),&Value))
		m_ThinFrameWidth=max(Value,1);
	Value=FRAME_NORMAL;
	if (!Settings.Read(TEXT("FrameType"),&Value)) {
		if (Settings.Read(TEXT("ThinFrame"),&f) && f)	// 以前のバージョンとの互換用
			Value=FRAME_CUSTOM;
	}
	SetCustomFrame(Value!=FRAME_NORMAL,Value==FRAME_CUSTOM?m_ThinFrameWidth:0);
	if (!m_fCustomFrame && Settings.Read(TEXT("CustomTitleBar"),&f))
		SetCustomTitleBar(f);
	Settings.Read(TEXT("SplitTitleBar"),&m_fSplitTitleBar);
	if (Settings.Read(TEXT("ShowSideBar"),&f))
		SetSideBarVisible(f);
	Settings.Read(TEXT("FrameCut"),&m_fFrameCut);

	return true;
}


bool CMainWindow::WriteSettings(CSettings &Settings)
{
	int Left,Top,Width,Height;

	GetPosition(&Left,&Top,&Width,&Height);
	Settings.Write(TEXT("WindowLeft"),Left);
	Settings.Write(TEXT("WindowTop"),Top);
	Settings.Write(TEXT("WindowWidth"),Width);
	Settings.Write(TEXT("WindowHeight"),Height);
	Settings.Write(TEXT("WindowMaximize"),GetMaximize());
	Settings.Write(TEXT("WindowSize.HD.Width"),m_HDWindowSize.Width);
	Settings.Write(TEXT("WindowSize.HD.Height"),m_HDWindowSize.Height);
	Settings.Write(TEXT("WindowSize.1Seg.Width"),m_1SegWindowSize.Width);
	Settings.Write(TEXT("WindowSize.1Seg.Height"),m_1SegWindowSize.Height);
	Settings.Write(TEXT("AlwaysOnTop"),m_pCore->GetAlwaysOnTop());
	Settings.Write(TEXT("ShowStatusBar"),m_fShowStatusBar);
	Settings.Write(TEXT("ShowTitleBar"),m_fShowTitleBar);
//	Settings.Write(TEXT("PopupTitleBar"),m_fPopupTitleBar);
	Settings.Write(TEXT("PanelDockingIndex"),m_PanelPaneIndex);
	Settings.Write(TEXT("PanelVerticalAlign"),m_fPanelVerticalAlign);
	Settings.Write(TEXT("FullscreenPanelWidth"),m_Fullscreen.GetPanelWidth());
	Settings.Write(TEXT("FullscreenPanelHeight"),m_Fullscreen.GetPanelHeight());
	Settings.Write(TEXT("FullscreenPanelPlace"),(int)m_Fullscreen.GetPanelPlace());
	Settings.Write(TEXT("FrameType"),
		!m_fCustomFrame?FRAME_NORMAL:(m_CustomFrameWidth==0?FRAME_NONE:FRAME_CUSTOM));
//	Settings.Write(TEXT("ThinFrameWidth"),m_ThinFrameWidth);
	Settings.Write(TEXT("CustomTitleBar"),m_fCustomTitleBar);
	Settings.Write(TEXT("SplitTitleBar"),m_fSplitTitleBar);
	Settings.Write(TEXT("ShowSideBar"),m_fShowSideBar);
	Settings.Write(TEXT("FrameCut"),m_fFrameCut);

	return true;
}


bool CMainWindow::SetAlwaysOnTop(bool fTop)
{
	if (m_hwnd!=nullptr) {
		::SetWindowPos(m_hwnd,fTop?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,
					   SWP_NOMOVE | SWP_NOSIZE);
		m_pCore->SetCommandCheckedState(CM_ALWAYSONTOP,fTop);
	}
	return true;
}


void CMainWindow::ShowPanel(bool fShow)
{
	if (m_App.Panel.fShowPanelWindow==fShow)
		return;

	m_App.Panel.fShowPanelWindow=fShow;

	LockLayout();

	m_App.Panel.Frame.SetPanelVisible(fShow);

	if (!fShow) {
		m_App.Panel.InfoPanel.ResetItem(CInformationPanel::ITEM_PROGRAMINFO);
		//m_App.Panel.ProgramListPanel.ClearProgramList();
		m_App.Panel.ChannelPanel.ClearChannelList();
	}

	if (!m_App.Panel.IsFloating()) {
		AdjustWindowSizeOnDockPanel(fShow);
	}

	UpdateLayout();

	if (fShow)
		m_App.Panel.UpdateContent();

	if (!m_pCore->GetFullscreen())
		m_pCore->SetCommandCheckedState(CM_PANEL,fShow);
}


void CMainWindow::SetStatusBarVisible(bool fVisible)
{
	if (m_fShowStatusBar!=fVisible) {
		if (!m_pCore->GetFullscreen()) {
			m_fShowStatusBar=fVisible;
			if (m_hwnd!=nullptr) {
				LockLayout();

				RECT rc;

				if (fVisible) {
					// 一瞬変な位置に出ないように見えない位置に移動
					RECT rcClient;
					::GetClientRect(m_App.StatusView.GetParent(),&rcClient);
					m_App.StatusView.GetPosition(&rc);
					m_App.StatusView.SetPosition(0,rcClient.bottom,rc.right-rc.left,rc.bottom-rc.top);
				}
				m_LayoutBase.SetContainerVisible(CONTAINER_ID_STATUS,fVisible);

				GetPosition(&rc);
				if (fVisible)
					rc.bottom+=m_App.StatusView.GetHeight();
				else
					rc.bottom-=m_App.StatusView.GetHeight();
				SetPosition(&rc);

				UpdateLayout();

				m_pCore->SetCommandCheckedState(CM_STATUSBAR,m_fShowStatusBar);
			}
		}
	}
}


bool CMainWindow::ShowStatusBarItem(int ID,bool fShow)
{
	CStatusItem *pItem=m_App.StatusView.GetItemByID(ID);
	if (pItem==nullptr)
		return false;
	if (pItem->GetVisible()==fShow)
		return true;

	pItem->SetVisible(fShow);

	if (!m_fStatusBarInitialized)
		return true;

	if (m_fShowStatusBar)
		LockLayout();

	int OldHeight=m_App.StatusView.GetHeight();
	m_App.StatusView.AdjustSize();
	int NewHeight=m_App.StatusView.GetHeight();

	if (OldHeight!=NewHeight) {
		const int Offset=NewHeight-OldHeight;
		RECT rc;

		if (m_fShowStatusBar) {
			GetPosition(&rc);
			rc.bottom+=Offset;
			SetPosition(&rc);
		}
		if ((!m_fShowStatusBar && m_App.StatusView.GetVisible())
				|| (m_pCore->GetFullscreen() && m_Fullscreen.IsStatusBarVisible())) {
			m_App.StatusView.GetPosition(&rc);
			rc.top-=Offset;
			rc.bottom=rc.top+NewHeight;
			m_App.StatusView.SetPosition(&rc);
		}

		// ポップアップ表示されたサイドバーの位置の調整
		if (((!m_fShowSideBar && m_App.SideBar.GetVisible())
				|| (m_pCore->GetFullscreen() && m_Fullscreen.IsSideBarVisible()))
				&& m_App.SideBarOptions.GetPlace()==CSideBarOptions::PLACE_BOTTOM) {
			m_App.SideBar.GetPosition(&rc);
			::OffsetRect(&rc,0,-Offset);
			m_App.SideBar.SetPosition(&rc);
		}
	}

	if (m_fShowStatusBar)
		UpdateLayout();

	return true;
}


void CMainWindow::OnStatusBarInitialized()
{
	m_App.StatusView.AdjustSize();
	m_fStatusBarInitialized=true;
}


void CMainWindow::OnStatusBarTraceEnd()
{
	// 起動時に一時的に表示していたステータスバーを非表示にする
	if (!m_fShowStatusBar)
		ShowPopupStatusBar(false);
}


void CMainWindow::SetTitleBarVisible(bool fVisible)
{
	if (m_fShowTitleBar!=fVisible) {
		m_fShowTitleBar=fVisible;
		if (m_hwnd!=nullptr) {
			bool fMaximize=GetMaximize();
			RECT rc;

			LockLayout();

			if (!fMaximize)
				GetPosition(&rc);
			if (!m_fCustomTitleBar)
				SetWindowStyle(GetWindowStyle()^WS_CAPTION,fMaximize);
			else if (!fVisible)
				m_LayoutBase.SetContainerVisible(CONTAINER_ID_TITLEBAR,false);
			if (!fMaximize) {
				int CaptionHeight;

				if (!m_fCustomTitleBar)
					CaptionHeight=::GetSystemMetrics(SM_CYCAPTION);
				else
					CaptionHeight=m_TitleBar.GetHeight();
				if (fVisible)
					rc.top-=CaptionHeight;
				else
					rc.top+=CaptionHeight;
				::SetWindowPos(m_hwnd,nullptr,rc.left,rc.top,
							   rc.right-rc.left,rc.bottom-rc.top,
							   SWP_NOZORDER | SWP_FRAMECHANGED | SWP_DRAWFRAME);
			}
			if (m_fCustomTitleBar && fVisible)
				m_LayoutBase.SetContainerVisible(CONTAINER_ID_TITLEBAR,true);

			UpdateLayout();

			m_pCore->SetCommandCheckedState(CM_TITLEBAR,m_fShowTitleBar);
		}
	}
}


// タイトルバーを独自のものにするか設定
void CMainWindow::SetCustomTitleBar(bool fCustom)
{
	if (m_fCustomTitleBar!=fCustom) {
		if (!fCustom && m_fCustomFrame)
			SetCustomFrame(false);
		m_fCustomTitleBar=fCustom;
		if (m_hwnd!=nullptr) {
			if (m_fShowTitleBar) {
				if (!fCustom)
					m_LayoutBase.SetContainerVisible(CONTAINER_ID_TITLEBAR,false);
				SetWindowStyle(GetWindowStyle()^WS_CAPTION,true);
				if (fCustom)
					m_LayoutBase.SetContainerVisible(CONTAINER_ID_TITLEBAR,true);
			}
		}
	}
}


// タイトルバーをパネルで分割するか設定
void CMainWindow::SetSplitTitleBar(bool fSplit)
{
	if (m_fSplitTitleBar!=fSplit) {
		m_fSplitTitleBar=fSplit;
		if (m_hwnd!=nullptr)
			UpdateLayoutStructure();
	}
}


// ウィンドウ枠を独自のものにするか設定
void CMainWindow::SetCustomFrame(bool fCustomFrame,int Width)
{
	if (m_fCustomFrame!=fCustomFrame || (fCustomFrame && m_CustomFrameWidth!=Width)) {
		if (fCustomFrame && Width<0)
			return;
		if (fCustomFrame && !m_fCustomTitleBar)
			SetCustomTitleBar(true);
		m_fCustomFrame=fCustomFrame;
		if (fCustomFrame)
			m_CustomFrameWidth=Width;
		if (m_hwnd!=nullptr) {
			::SetWindowPos(m_hwnd,nullptr,0,0,0,0,
						   SWP_FRAMECHANGED | SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
			CAeroGlass Aero;
			Aero.EnableNcRendering(m_hwnd,!fCustomFrame);
			if (fCustomFrame) {
				HookWindows(m_LayoutBase.GetHandle());
				HookWindows(m_App.Panel.Form.GetHandle());
			}
		}
	}
}


void CMainWindow::SetSideBarVisible(bool fVisible)
{
	if (m_fShowSideBar!=fVisible) {
		m_fShowSideBar=fVisible;
		if (m_hwnd!=nullptr) {
			LockLayout();

			RECT rc;

			if (fVisible) {
				// 一瞬変な位置に出ないように見えない位置に移動
				RECT rcClient;
				::GetClientRect(m_App.SideBar.GetParent(),&rcClient);
				m_App.SideBar.GetPosition(&rc);
				m_App.SideBar.SetPosition(0,rcClient.bottom,rc.right-rc.left,rc.bottom-rc.top);
			}
			m_LayoutBase.SetContainerVisible(CONTAINER_ID_SIDEBAR,fVisible);

			GetPosition(&rc);
			RECT rcArea=rc;
			if (fVisible)
				m_SideBarManager.ReserveArea(&rcArea,true);
			else
				m_SideBarManager.AdjustArea(&rcArea);
			rc.right=rc.left+(rcArea.right-rcArea.left);
			rc.bottom=rc.top+(rcArea.bottom-rcArea.top);
			SetPosition(&rc);

			UpdateLayout();

			m_pCore->SetCommandCheckedState(CM_SIDEBAR,m_fShowSideBar);
		}
	}
}


bool CMainWindow::OnBarMouseLeave(HWND hwnd)
{
	if (m_pCore->GetFullscreen()) {
		m_Fullscreen.OnMouseMove();
		return true;
	}

	POINT pt;

	::GetCursorPos(&pt);
	::ScreenToClient(::GetParent(hwnd),&pt);
	if (hwnd==m_TitleBar.GetHandle()) {
		if (!m_fShowTitleBar) {
			if (!m_fShowSideBar && m_App.SideBar.GetVisible()
					&& m_App.SideBarOptions.GetPlace()==CSideBarOptions::PLACE_TOP) {
				if (::RealChildWindowFromPoint(m_App.SideBar.GetParent(),pt)==m_App.SideBar.GetHandle())
					return false;
			}
			m_TitleBar.SetVisible(false);
			if (!m_fShowSideBar && m_App.SideBar.GetVisible())
				ShowPopupSideBar(false);
			return true;
		}
	} else if (hwnd==m_App.StatusView.GetHandle()) {
		if (!m_fShowStatusBar) {
			if (!m_fShowSideBar && m_App.SideBar.GetVisible()
					&& m_App.SideBarOptions.GetPlace()==CSideBarOptions::PLACE_BOTTOM) {
				if (::RealChildWindowFromPoint(m_App.SideBar.GetParent(),pt)==m_App.SideBar.GetHandle())
					return false;
			}
			ShowPopupStatusBar(false);
			if (!m_fShowSideBar && m_App.SideBar.GetVisible())
				ShowPopupSideBar(false);
			return true;
		}
	} else if (hwnd==m_App.SideBar.GetHandle()) {
		if (!m_fShowSideBar) {
			ShowPopupSideBar(false);
			if (!m_fShowTitleBar && m_TitleBar.GetVisible()
					&& ::RealChildWindowFromPoint(m_TitleBar.GetParent(),pt)!=m_TitleBar.GetHandle())
				m_TitleBar.SetVisible(false);
			if (!m_fShowStatusBar && m_App.StatusView.GetVisible()
					&& ::RealChildWindowFromPoint(m_App.StatusView.GetParent(),pt)!=m_App.StatusView.GetHandle())
				ShowPopupStatusBar(false);
			return true;
		}
	}

	return false;
}


int CMainWindow::GetPanelPaneIndex() const
{
	Layout::CSplitter *pSplitter=
		dynamic_cast<Layout::CSplitter*>(m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));

	if (pSplitter==nullptr)
		return m_PanelPaneIndex;
	return pSplitter->IDToIndex(CONTAINER_ID_PANEL);
}


bool CMainWindow::IsPanelVisible() const
{
	return m_App.Panel.fShowPanelWindow
		|| (m_pCore->GetFullscreen() && IsFullscreenPanelVisible());
}


bool CMainWindow::IsPanelPresent() const
{
	return m_pCore->GetFullscreen()?IsFullscreenPanelVisible():m_App.Panel.fShowPanelWindow;
}


void CMainWindow::OnPanelFloating(bool fFloating)
{
	if (fFloating)
		m_LayoutBase.SetContainerVisible(CONTAINER_ID_PANEL,false);
	AdjustWindowSizeOnDockPanel(!fFloating);
}


void CMainWindow::OnPanelDocking(CPanelFrame::DockingPlace Place)
{
	const bool fMove=m_LayoutBase.GetContainerVisible(CONTAINER_ID_PANEL);
	RECT rcCur,rcAdjusted;

	LockLayout();

	if (fMove) {
		GetPosition(&rcCur);
		rcAdjusted=rcCur;
		GetPanelDockingAdjustedPos(false,&rcAdjusted);
		m_LayoutBase.SetContainerVisible(CONTAINER_ID_PANEL,false);
	}

	m_fPanelVerticalAlign=
		Place==CPanelFrame::DOCKING_TOP || Place==CPanelFrame::DOCKING_BOTTOM;

	UpdateLayoutStructure();

	Layout::CSplitter *pPanelSplitter=dynamic_cast<Layout::CSplitter*>(
		m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));

	const int Index=
		(Place==CPanelFrame::DOCKING_LEFT || Place==CPanelFrame::DOCKING_TOP)?0:1;
	if (pPanelSplitter->IDToIndex(CONTAINER_ID_PANEL)!=Index)
		pPanelSplitter->SwapPane();

	if (fMove) {
		GetPanelDockingAdjustedPos(true,&rcAdjusted);
		if (rcCur.right-rcCur.left!=rcAdjusted.right-rcAdjusted.left
				|| rcCur.bottom-rcCur.top!=rcAdjusted.bottom-rcAdjusted.top)
			SetPosition(&rcAdjusted);
		pPanelSplitter->SetPaneSize(CONTAINER_ID_PANEL,
			m_fPanelVerticalAlign?
				m_App.Panel.Frame.GetDockingHeight():
				m_App.Panel.Frame.GetDockingWidth());
		m_LayoutBase.SetContainerVisible(CONTAINER_ID_PANEL,true);
	}

	UpdateLayout();
}


LRESULT CMainWindow::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	HANDLE_MSG(hwnd,WM_COMMAND,OnCommand);
	HANDLE_MSG(hwnd,WM_TIMER,OnTimer);
	HANDLE_MSG(hwnd,WM_GETMINMAXINFO,OnGetMinMaxInfo);

	case WM_SIZE:
		OnSizeChanged((UINT)wParam,LOWORD(lParam),HIWORD(lParam));
		return 0;

	case WM_SIZING:
		if (OnSizeChanging((UINT)wParam,reinterpret_cast<LPRECT>(lParam)))
			return TRUE;
		break;

	case WM_MOVE:
		m_App.OSDManager.OnParentMove();
		return 0;

	case WM_ENTERSIZEMOVE:
		m_fEnterSizeMove=true;
		m_fResizePanel=false;
		return 0;

	case WM_EXITSIZEMOVE:
		if (m_fResizePanel) {
			m_fResizePanel=false;

			Layout::CSplitter *pSplitter=dynamic_cast<Layout::CSplitter*>(
				m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));
			pSplitter->SetAdjustPane(pSplitter->GetPane(!pSplitter->IDToIndex(CONTAINER_ID_PANEL))->GetID());
		}
		return 0;

	case WM_SHOWWINDOW:
		if (!wParam) {
			m_App.OSDManager.ClearOSD();
			m_App.OSDManager.Reset();
		}
		break;

	case WM_RBUTTONUP:
		if (m_pCore->GetFullscreen()) {
			m_Fullscreen.OnRButtonUp();
		} else {
			::SendMessage(hwnd,WM_COMMAND,
				MAKEWPARAM(m_App.OperationOptions.GetRightClickCommand(),COMMAND_FROM_MOUSE),0);
		}
		return 0;

	case WM_MBUTTONUP:
		if (m_pCore->GetFullscreen()) {
			m_Fullscreen.OnMButtonUp();
		} else {
			::SendMessage(hwnd,WM_COMMAND,
				MAKEWPARAM(m_App.OperationOptions.GetMiddleClickCommand(),COMMAND_FROM_MOUSE),0);
		}
		return 0;

	case WM_NCLBUTTONDOWN:
		if (wParam!=HTCAPTION)
			break;
		ForegroundWindow(hwnd);
	case WM_LBUTTONDOWN:
		if (!GetMaximize()
				&& (uMsg==WM_NCLBUTTONDOWN || m_App.OperationOptions.GetDisplayDragMove())) {
			m_fDragging=true;
			/*
			m_ptDragStartPos.x=GET_X_LPARAM(lParam);
			m_ptDragStartPos.y=GET_Y_LPARAM(lParam);
			::ClientToScreen(hwnd,&m_ptDragStartPos);
			*/
			::GetCursorPos(&m_ptDragStartPos);
			::GetWindowRect(hwnd,&m_rcDragStart);
			::SetCapture(hwnd);
			::KillTimer(hwnd,TIMER_ID_HIDECURSOR);
			::SetCursor(::LoadCursor(nullptr,IDC_ARROW));
			return 0;
		}
		break;

	case WM_NCLBUTTONUP:
	case WM_LBUTTONUP:
		if (::GetCapture()==hwnd)
			::ReleaseCapture();
		return 0;

	case WM_CAPTURECHANGED:
		if (m_fDragging) {
			m_fDragging=false;
			m_TitleBarManager.EndDrag();
			if (m_App.OperationOptions.GetHideCursor())
				::SetTimer(m_hwnd,TIMER_ID_HIDECURSOR,HIDE_CURSOR_DELAY,nullptr);
		}
		return 0;

	case WM_MOUSEMOVE:
		OnMouseMove(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		return 0;

	case WM_LBUTTONDBLCLK:
		::SendMessage(hwnd,WM_COMMAND,
			MAKEWPARAM(m_App.OperationOptions.GetLeftDoubleClickCommand(),COMMAND_FROM_MOUSE),0);
		return 0;

	case WM_SETCURSOR:
		if (OnSetCursor(reinterpret_cast<HWND>(wParam),LOWORD(lParam),HIWORD(lParam)))
			return TRUE;
		break;

	case WM_SYSKEYDOWN:
		if (wParam!=VK_F10)
			break;
	case WM_KEYDOWN:
		if (OnKeyDown(wParam))
			return 0;
		break;

	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		{
			bool fHorz=uMsg==WM_MOUSEHWHEEL;

			OnMouseWheel(wParam,lParam,fHorz);
			// WM_MOUSEHWHEEL は 1を返さないと繰り返し送られて来ないらしい
			return fHorz;
		}

	case WM_MEASUREITEM:
		{
			LPMEASUREITEMSTRUCT pmis=reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);

			if (pmis->itemID>=CM_ASPECTRATIO_FIRST && pmis->itemID<=CM_ASPECTRATIO_3D_LAST) {
				if (m_App.AspectRatioIconMenu.OnMeasureItem(hwnd,wParam,lParam))
					return TRUE;
				break;
			}
			if (m_App.ChannelMenu.OnMeasureItem(hwnd,wParam,lParam))
				return TRUE;
			if (m_App.FavoritesMenu.OnMeasureItem(hwnd,wParam,lParam))
				return TRUE;
		}
		break;

	case WM_DRAWITEM:
		if (m_App.AspectRatioIconMenu.OnDrawItem(hwnd,wParam,lParam))
			return TRUE;
		if (m_App.ChannelMenu.OnDrawItem(hwnd,wParam,lParam))
			return TRUE;
		if (m_App.FavoritesMenu.OnDrawItem(hwnd,wParam,lParam))
			return TRUE;
		break;

// ウィンドウ枠を独自のものにするためのコード
	case WM_NCACTIVATE:
		if (m_fCustomFrame) {
			DrawCustomFrame(wParam!=FALSE);
			return TRUE;
		}
		break;

	case WM_NCCALCSIZE:
		if (m_fCustomFrame) {
			if (wParam!=0) {
				NCCALCSIZE_PARAMS *pnccsp=reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);

				::InflateRect(&pnccsp->rgrc[0],-m_CustomFrameWidth,-m_CustomFrameWidth);
			}
			return 0;
		}
		break;

	case WM_NCPAINT:
		if (m_fCustomFrame) {
			DrawCustomFrame(::GetActiveWindow()==hwnd);
			return 0;
		}
		break;

	case WM_NCHITTEST:
		if (m_fCustomFrame && !::IsZoomed(hwnd)) {
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			int BorderWidth=m_CustomFrameWidth;
			RECT rc;
			int Code=HTNOWHERE;

			::GetWindowRect(hwnd,&rc);
			if (x>=rc.left && x<rc.left+BorderWidth) {
				if (y>=rc.top) {
					if (y<rc.top+BorderWidth)
						Code=HTTOPLEFT;
					else if (y<rc.bottom-BorderWidth)
						Code=HTLEFT;
					else if (y<rc.bottom)
						Code=HTBOTTOMLEFT;
				}
			} else if (x>=rc.right-BorderWidth && x<rc.right) {
				if (y>=rc.top) {
					if (y<rc.top+BorderWidth)
						Code=HTTOPRIGHT;
					else if (y<rc.bottom-BorderWidth)
						Code=HTRIGHT;
					else if (y<rc.bottom)
						Code=HTBOTTOMRIGHT;
				}
			} else if (y>=rc.top && y<rc.top+BorderWidth) {
				Code=HTTOP;
			} else if (y>=rc.bottom-BorderWidth && y<rc.bottom) {
				Code=HTBOTTOM;
			} else {
				POINT pt={x,y};
				if (::PtInRect(&rc,pt))
					Code=HTCLIENT;
			}
			return Code;
		}
		break;
// ウィンドウ枠を独自のものにするためのコード終わり

	case WM_INITMENUPOPUP:
		if (OnInitMenuPopup(reinterpret_cast<HMENU>(wParam)))
			return 0;
		break;

	case WM_UNINITMENUPOPUP:
		if (m_App.ChannelMenu.OnUninitMenuPopup(hwnd,wParam,lParam))
			return 0;
		if (m_App.FavoritesMenu.OnUninitMenuPopup(hwnd,wParam,lParam))
			return 0;
		break;

	case WM_MENUSELECT:
		if (m_App.ChannelMenu.OnMenuSelect(hwnd,wParam,lParam))
			return 0;
		if (m_App.FavoritesMenu.OnMenuSelect(hwnd,wParam,lParam))
			return 0;
		break;

	case WM_ENTERMENULOOP:
		m_fNoHideCursor=true;
		return 0;

	case WM_EXITMENULOOP:
		m_fNoHideCursor=false;
		return 0;

	case WM_SYSCOMMAND:
		if (OnSysCommand(static_cast<UINT>(wParam)))
			return 0;
		break;

	case WM_APPCOMMAND:
		{
			int Command=m_App.Accelerator.TranslateAppCommand(wParam,lParam);

			if (Command!=0) {
				SendCommand(Command);
				return TRUE;
			}
		}
		break;

	case WM_INPUT:
		return m_App.Accelerator.OnInput(hwnd,wParam,lParam);

	case WM_HOTKEY:
		{
			int Command=m_App.Accelerator.TranslateHotKey(wParam,lParam);

			if (Command>0)
				PostMessage(WM_COMMAND,Command,0);
		}
		return 0;

	case WM_SETFOCUS:
		m_Display.GetDisplayBase().SetFocus();
		return 0;

	case WM_SETTEXT:
		{
			LPCTSTR pszText=reinterpret_cast<LPCTSTR>(lParam);

			m_TitleBar.SetLabel(pszText);
			if (m_pCore->GetFullscreen())
				::SetWindowText(m_Fullscreen.GetHandle(),pszText);
		}
		break;

	case WM_SETICON:
		if (wParam==ICON_SMALL) {
			m_TitleBar.SetIcon(reinterpret_cast<HICON>(lParam));
			m_Fullscreen.SendMessage(uMsg,wParam,lParam);
		}
		break;

	case WM_POWERBROADCAST:
		if (wParam==PBT_APMSUSPEND) {
			m_App.AddLog(TEXT("サスペンドへの移行通知を受けました。"));
			if (m_App.EpgCaptureManager.IsCapturing()) {
				m_App.EpgCaptureManager.EndCapture(0);
			} else if (!m_pCore->GetStandby()) {
				StoreTunerResumeInfo();
			}
			SuspendViewer(ResumeInfo::VIEWERSUSPEND_SUSPEND);
			m_App.Core.CloseTuner();
			FinalizeViewer();
		} else if (wParam==PBT_APMRESUMESUSPEND) {
			m_App.AddLog(TEXT("サスペンドからの復帰通知を受けました。"));
			if (!m_pCore->GetStandby()) {
				// 遅延させた方がいいかも?
				ResumeTuner();
			}
			ResumeViewer(ResumeInfo::VIEWERSUSPEND_SUSPEND);
		}
		break;

	case WM_DWMCOMPOSITIONCHANGED:
		m_App.OSDOptions.OnDwmCompositionChanged();
		return 0;

	case CAppMain::WM_INTERPROCESS:
		return m_App.ReceiveInterprocessMessage(hwnd,wParam,lParam);

	case WM_APP_SERVICEUPDATE:
		// サービスが更新された
		TRACE(TEXT("WM_APP_SERVICEUPDATE\n"));
		{
			CServiceUpdateInfo *pInfo=reinterpret_cast<CServiceUpdateInfo*>(lParam);

			if (pInfo->m_fStreamChanged) {
				if (m_ResetErrorCountTimer.IsEnabled())
					m_ResetErrorCountTimer.Begin(hwnd,2000);

				m_Display.GetDisplayBase().SetVisible(false);
			}

			if (!m_App.Core.IsChannelScanning()
					&& pInfo->m_NumServices>0 && pInfo->m_CurService>=0) {
				const CChannelInfo *pChInfo=m_App.ChannelManager.GetCurrentChannelInfo();
				WORD ServiceID,TransportStreamID;

				TransportStreamID=pInfo->m_TransportStreamID;
				ServiceID=pInfo->m_pServiceList[pInfo->m_CurService].ServiceID;
				if (/*pInfo->m_fStreamChanged
						&& */TransportStreamID!=0 && ServiceID!=0
						&& !m_App.CoreEngine.IsNetworkDriver()
						&& (pChInfo==nullptr
						|| ((pChInfo->GetTransportStreamID()!=0
						&& pChInfo->GetTransportStreamID()!=TransportStreamID)
						|| (pChInfo->GetServiceID()!=0
						&& pChInfo->GetServiceID()!=ServiceID)))) {
					// 外部からチャンネル変更されたか、
					// BonDriverが開かれたときのデフォルトチャンネル
					m_App.Core.FollowChannelChange(TransportStreamID,ServiceID);
				}
			} else if (pInfo->m_fServiceListEmpty && pInfo->m_fStreamChanged
					&& !m_App.Core.IsChannelScanning()
					&& !m_App.EpgCaptureManager.IsCapturing()) {
				ShowNotificationBar(TEXT("このチャンネルは放送休止中です"),
									CNotificationBar::MESSAGE_INFO);
			}

			delete pInfo;

			m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_SERVICE);

			if (wParam!=0)
				m_App.AppEventManager.OnServiceListUpdated();
			else
				m_App.AppEventManager.OnServiceInfoUpdated();
		}
		return 0;

	case WM_APP_SERVICECHANGED:
		TRACE(TEXT("WM_APP_SERVICECHANGED\n"));
		m_App.AddLog(TEXT("サービスを変更しました。(SID %d)"),static_cast<int>(wParam));
		m_pCore->UpdateTitle();
		m_App.StatusView.UpdateItem(STATUS_ITEM_CHANNEL);
		m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_SERVICE);
		if (m_App.Panel.Form.GetCurPageID()==PANEL_ID_INFORMATION)
			m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_PROGRAMINFO);
		return 0;

	case WM_APP_SERVICEINFOUPDATED:
		TRACE(TEXT("WM_APP_SERVICEINFOUPDATED\n"));
		m_pCore->UpdateTitle();
		m_App.StatusView.UpdateItem(STATUS_ITEM_CHANNEL);
		if (!IsMessageInQueue(hwnd,WM_APP_SERVICEINFOUPDATED)) {
			m_App.TSProcessorManager.OnNetworkChanged(
				LOWORD(lParam),HIWORD(lParam),
				CTSProcessorManager::FILTER_OPEN_NOTIFY_ERROR |
				CTSProcessorManager::FILTER_OPEN_NO_UI);
		}
		return 0;

	/*
	case WM_APP_IMAGESAVE:
		{
			::MessageBox(nullptr,TEXT("画像の保存でエラーが発生しました。"),nullptr,
						 MB_OK | MB_ICONEXCLAMATION);
		}
		return 0;
	*/

	case WM_APP_TRAYICON:
		switch (lParam) {
		case WM_RBUTTONDOWN:
			{
				CPopupMenu Menu(m_App.GetResourceInstance(),IDM_TRAY);

				Menu.EnableItem(CM_SHOW,
								m_pCore->GetStandby() || IsMinimizeToTray());
				// お約束が必要な理由は以下を参照
				// http://support.microsoft.com/kb/135788/en-us
				ForegroundWindow(hwnd);				// お約束
				Menu.Show(hwnd);
				::PostMessage(hwnd,WM_NULL,0,0);	// お約束
			}
			break;

		case WM_LBUTTONUP:
			SendCommand(CM_SHOW);
			break;
		}
		return 0;

	case WM_APP_QUERYPORT:
		// 使っているポートを返す
		TRACE(TEXT("WM_APP_QUERYPORT\n"));
		if (!m_fClosing && m_App.CoreEngine.IsNetworkDriver()) {
			WORD Port=m_App.ChannelManager.GetCurrentChannel()+
										(m_App.CoreEngine.IsUDPDriver()?1234:2230);
			return MAKELRESULT(Port,0);
		}
		return 0;

	case WM_APP_FILEWRITEERROR:
		// ファイルの書き出しエラー
		TRACE(TEXT("WM_APP_FILEWRITEERROR\n"));
		ShowErrorMessage(TEXT("ファイルへの書き出しでエラーが発生しました。"));
		return 0;

	case WM_APP_VIDEOSTREAMTYPECHANGED:
		// 映像stream_typeが変わった
		TRACE(TEXT("WM_APP_VIDEOSTREAMTYPECHANGED\n"));
		if (m_fEnablePlayback
				&& !IsMessageInQueue(hwnd,WM_APP_VIDEOSTREAMTYPECHANGED)) {
			BYTE StreamType=static_cast<BYTE>(wParam);

			if (StreamType==m_App.CoreEngine.m_DtvEngine.GetVideoStreamType())
				m_pCore->EnableViewer(true);
		}
		return 0;

	case WM_APP_VIDEOSIZECHANGED:
		// 映像サイズが変わった
		TRACE(TEXT("WM_APP_VIDEOSIZECHANGED\n"));
		/*
			ストリームの映像サイズの変化を検知してから、それが実際に
			表示されるまでにはタイムラグがあるため、後で調整を行う
		*/
		m_VideoSizeChangedTimerCount=0;
		::SetTimer(hwnd,TIMER_ID_VIDEOSIZECHANGED,500,nullptr);
		if (m_AspectRatioResetTime!=0
				&& !m_pCore->GetFullscreen()
				&& IsViewerEnabled()
				&& TickTimeSpan(m_AspectRatioResetTime,::GetTickCount())<6000) {
			if (AutoFitWindowToVideo())
				m_AspectRatioResetTime=0;
		}
		return 0;

	case WM_APP_EPGLOADED:
		// EPGファイルが読み込まれた
		TRACE(TEXT("WM_APP_EPGLOADED\n"));
		m_App.Panel.EnableProgramListUpdate(true);
		if (IsPanelVisible()
				&& (m_App.Panel.Form.GetCurPageID()==PANEL_ID_PROGRAMLIST
				 || m_App.Panel.Form.GetCurPageID()==PANEL_ID_CHANNEL)) {
			m_App.Panel.UpdateContent();
		}
		return 0;

	case WM_APP_CONTROLLERFOCUS:
		// コントローラの操作対象が変わった
		TRACE(TEXT("WM_APP_CONTROLLERFOCUS\n"));
		m_App.ControllerManager.OnFocusChange(hwnd,wParam!=0);
		return 0;

	case WM_APP_PLUGINMESSAGE:
		// プラグインのメッセージの処理
		return CPlugin::OnPluginMessage(wParam,lParam);

	case WM_APP_SHOWNOTIFICATIONBAR:
		// 通知バーの表示
		TRACE(TEXT("WM_APP_SHOWNOTIFICATIONBAR"));
		{
			LPCTSTR pszMessage=reinterpret_cast<LPCTSTR>(lParam);

			if (pszMessage!=nullptr) {
				if (m_App.OSDOptions.IsNotifyEnabled(HIWORD(wParam))) {
					ShowNotificationBar(pszMessage,
						static_cast<CNotificationBar::MessageType>(LOWORD(wParam)),
						6000);
				}
				delete [] pszMessage;
			}
		}
		return 0;

	case WM_APP_AUDIOLISTCHANGED:
		TRACE(TEXT("WM_APP_AUDIOLISTCHANGED\n"));
		m_pCore->AutoSelectAudio();
		return 0;

	case WM_APP_SPDIFPASSTHROUGHERROR:
		// S/PDIFパススルー出力のエラー
		TRACE(TEXT("WM_APP_SPDIFPASSTHROUGHERROR\n"));
		{
			//HRESULT hr=static_cast<HRESULT>(wParam);
			CAudioDecFilter::SpdifOptions Options;

			m_App.CoreEngine.GetSpdifOptions(&Options);
			Options.Mode=CAudioDecFilter::SPDIF_MODE_DISABLED;
			m_App.CoreEngine.SetSpdifOptions(Options);
			m_App.CoreEngine.m_DtvEngine.ResetMediaViewer();

			ShowMessage(TEXT("S/PDIFパススルー出力ができません。\n")
						TEXT("デバイスがパススルー出力に対応しているか、\n")
						TEXT("またパススルー出力できるように設定されているか確認してください。"),
						TEXT("S/PDIFパススルー出力エラー"),
						MB_OK | MB_ICONEXCLAMATION);
		}
		return 0;

	case WM_ACTIVATEAPP:
		{
			bool fActive=wParam!=FALSE;

			m_App.ControllerManager.OnActiveChange(hwnd,fActive);
			if (fActive)
				m_App.BroadcastControllerFocusMessage(hwnd,fActive || m_fClosing,!fActive);
		}
		return 0;

	case WM_DISPLAYCHANGE:
		m_App.CoreEngine.m_DtvEngine.m_MediaViewer.DisplayModeChanged();
		break;

	case WM_THEMECHANGED:
		m_App.ChannelMenu.Destroy();
		m_App.FavoritesMenu.Destroy();
		return 0;

	case WM_CLOSE:
		if (!OnClose(hwnd))
			return 0;
		break;

	case WM_ENDSESSION:
		if (!wParam)
			break;
		m_App.Core.SetSilent(true);
	case WM_DESTROY:
		OnDestroy();
		return 0;

	default:
		/*
		if (m_App.ControllerManager.HandleMessage(hwnd,uMsg,wParam,lParam))
			return 0;
		*/
		if (m_App.TaskTrayManager.HandleMessage(uMsg,wParam,lParam))
			return 0;
		if (m_App.TaskbarManager.HandleMessage(uMsg,wParam,lParam))
			return 0;
	}

	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


bool CMainWindow::OnCreate(const CREATESTRUCT *pcs)
{
	InitializeUI();

	RECT rc;
	GetClientRect(&rc);
	m_LayoutBase.SetPosition(&rc);
	m_LayoutBase.Create(m_hwnd,
						WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

	m_Display.GetViewWindow().SetMargin(m_Style.ScreenMargin);
	m_Display.Create(m_LayoutBase.GetHandle(),IDC_VIEW,IDC_VIDEOCONTAINER,m_hwnd);
	m_Display.GetViewWindow().SetEventHandler(&m_ViewWindowEventHandler);
	m_Display.GetVideoContainer().SetEventHandler(&m_VideoContainerEventHandler);
	m_Display.GetDisplayBase().SetEventHandler(&m_DisplayBaseEventHandler);

	m_TitleBar.Create(m_LayoutBase.GetHandle(),
					  WS_CHILD | WS_CLIPSIBLINGS | (m_fShowTitleBar && m_fCustomTitleBar?WS_VISIBLE:0),
					  0,IDC_TITLEBAR);
	m_TitleBar.SetEventHandler(&m_TitleBarManager);
	m_TitleBar.SetLabel(pcs->lpszName);
	m_TitleBar.SetIcon(m_TitleBar.IsIconDrawSmall()?m_App.GetAppIconSmall():m_App.GetAppIcon());
	m_TitleBar.SetMaximizeMode((pcs->style&WS_MAXIMIZE)!=0);

	m_App.StatusView.AddItem(new CChannelStatusItem);
	m_App.StatusView.AddItem(new CVideoSizeStatusItem);
	m_App.StatusView.AddItem(new CVolumeStatusItem);
	m_App.StatusView.AddItem(new CAudioChannelStatusItem);
	CRecordStatusItem *pRecordStatusItem=new CRecordStatusItem;
	pRecordStatusItem->ShowRemainTime(m_App.RecordOptions.GetShowRemainTime());
	m_App.StatusView.AddItem(pRecordStatusItem);
	m_App.StatusView.AddItem(new CCaptureStatusItem);
	m_App.StatusView.AddItem(new CErrorStatusItem);
	m_App.StatusView.AddItem(new CSignalLevelStatusItem);
	CClockStatusItem *pClockStatusItem=new CClockStatusItem;
	pClockStatusItem->SetTOT(m_App.StatusOptions.GetShowTOTTime());
	m_App.StatusView.AddItem(pClockStatusItem);
	CProgramInfoStatusItem *pProgramInfoStatusItem=new CProgramInfoStatusItem;
	pProgramInfoStatusItem->EnablePopupInfo(m_App.StatusOptions.IsPopupProgramInfoEnabled());
	pProgramInfoStatusItem->SetShowProgress(m_App.StatusOptions.GetShowEventProgress());
	m_App.StatusView.AddItem(pProgramInfoStatusItem);
	m_App.StatusView.AddItem(new CBufferingStatusItem);
	m_App.StatusView.AddItem(new CTunerStatusItem);
	m_App.StatusView.AddItem(new CMediaBitRateStatusItem);
	m_App.StatusView.AddItem(new CFavoritesStatusItem);
	Theme::CThemeManager ThemeManager(m_pCore->GetCurrentColorScheme());
	m_App.StatusView.SetItemTheme(&ThemeManager);
	m_App.StatusView.Create(m_LayoutBase.GetHandle(),
		//WS_CHILD | (m_fShowStatusBar?WS_VISIBLE:0) | WS_CLIPSIBLINGS,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,0,IDC_STATUS);
	m_App.StatusView.SetEventHandler(&m_StatusViewEventHandler);
	m_App.StatusOptions.ApplyOptions();

	m_NotificationBar.Create(m_Display.GetVideoContainer().GetHandle(),
							 WS_CHILD | WS_CLIPSIBLINGS);

	m_App.SideBarOptions.ApplySideBarOptions();
	m_App.SideBar.SetEventHandler(&m_SideBarManager);
	m_App.SideBar.Create(m_LayoutBase.GetHandle(),
						 WS_CHILD | WS_CLIPSIBLINGS/* | (m_fShowSideBar?WS_VISIBLE:0)*/,
						 0,IDC_SIDEBAR);
	m_App.SideBarOptions.SetSideBarImage();

	Layout::CWindowContainer *pWindowContainer;
	Layout::CSplitter *pSideBarSplitter=new Layout::CSplitter(CONTAINER_ID_SIDEBARSPLITTER);
	CSideBarOptions::PlaceType SideBarPlace=m_App.SideBarOptions.GetPlace();
	bool fSideBarVertical=SideBarPlace==CSideBarOptions::PLACE_LEFT
						|| SideBarPlace==CSideBarOptions::PLACE_RIGHT;
	int SideBarWidth=m_App.SideBar.GetBarWidth();
	pSideBarSplitter->SetStyle(Layout::CSplitter::STYLE_FIXED |
		(fSideBarVertical?Layout::CSplitter::STYLE_HORZ:Layout::CSplitter::STYLE_VERT));
	pSideBarSplitter->SetVisible(true);
	pWindowContainer=new Layout::CWindowContainer(CONTAINER_ID_VIEW);
	pWindowContainer->SetWindow(&m_Display.GetViewWindow());
	pWindowContainer->SetMinSize(32,32);
	pWindowContainer->SetVisible(true);
	pSideBarSplitter->SetPane(0,pWindowContainer);
	pSideBarSplitter->SetAdjustPane(CONTAINER_ID_VIEW);
	pWindowContainer=new Layout::CWindowContainer(CONTAINER_ID_SIDEBAR);
	pWindowContainer->SetWindow(&m_App.SideBar);
	pWindowContainer->SetMinSize(SideBarWidth,SideBarWidth);
	pWindowContainer->SetVisible(/*m_fShowSideBar*/false);
	pSideBarSplitter->SetPane(1,pWindowContainer);
	pSideBarSplitter->SetPaneSize(CONTAINER_ID_SIDEBAR,SideBarWidth);
	if (SideBarPlace==CSideBarOptions::PLACE_LEFT
			|| SideBarPlace==CSideBarOptions::PLACE_TOP)
		pSideBarSplitter->SwapPane();

	Layout::CSplitter *pTitleBarSplitter=new Layout::CSplitter(CONTAINER_ID_TITLEBARSPLITTER);
	pTitleBarSplitter->SetStyle(Layout::CSplitter::STYLE_VERT | Layout::CSplitter::STYLE_FIXED);
	pTitleBarSplitter->SetVisible(true);
	Layout::CWindowContainer *pTitleBarContainer=new Layout::CWindowContainer(CONTAINER_ID_TITLEBAR);
	pTitleBarContainer->SetWindow(&m_TitleBar);
	pTitleBarContainer->SetMinSize(0,m_TitleBar.GetHeight());
	pTitleBarContainer->SetVisible(m_fShowTitleBar && m_fCustomTitleBar);
	pTitleBarSplitter->SetPane(0,pTitleBarContainer);
	pTitleBarSplitter->SetPaneSize(CONTAINER_ID_TITLEBAR,m_TitleBar.GetHeight());

	Layout::CSplitter *pPanelSplitter=new Layout::CSplitter(CONTAINER_ID_PANELSPLITTER);
	pPanelSplitter->SetStyle(
		m_fPanelVerticalAlign ? Layout::CSplitter::STYLE_VERT : Layout::CSplitter::STYLE_HORZ);
	pPanelSplitter->SetVisible(true);
	Layout::CWindowContainer *pPanelContainer=new Layout::CWindowContainer(CONTAINER_ID_PANEL);
	pPanelContainer->SetMinSize(32,32);
	pPanelSplitter->SetPane(m_PanelPaneIndex,pPanelContainer);
	pPanelSplitter->SetPane(1-m_PanelPaneIndex,pSideBarSplitter);
	pPanelSplitter->SetAdjustPane(CONTAINER_ID_SIDEBARSPLITTER);
	pTitleBarSplitter->SetPane(1,pPanelSplitter);
	pTitleBarSplitter->SetAdjustPane(CONTAINER_ID_PANELSPLITTER);

	Layout::CSplitter *pStatusSplitter=new Layout::CSplitter(CONTAINER_ID_STATUSSPLITTER);
	pStatusSplitter->SetStyle(Layout::CSplitter::STYLE_VERT | Layout::CSplitter::STYLE_FIXED);
	pStatusSplitter->SetVisible(true);
	pStatusSplitter->SetPane(0,pTitleBarSplitter);
	pStatusSplitter->SetAdjustPane(CONTAINER_ID_TITLEBARSPLITTER);
	pWindowContainer=new Layout::CWindowContainer(CONTAINER_ID_STATUS);
	pWindowContainer->SetWindow(&m_App.StatusView);
	pWindowContainer->SetMinSize(0,m_App.StatusView.GetHeight());
	pWindowContainer->SetVisible(m_fShowStatusBar);
	pStatusSplitter->SetPane(1,pWindowContainer);
	pStatusSplitter->SetPaneSize(CONTAINER_ID_STATUS,m_App.StatusView.GetHeight());

	m_LayoutBase.LockLayout();
	m_LayoutBase.SetTopContainer(pStatusSplitter);
	m_LayoutBase.UnlockLayout();
	UpdateLayoutStructure();

	// 起動状況を表示するために、起動時は常にステータスバーを表示する
	if (!m_fShowStatusBar) {
		RECT rc;

		GetClientRect(&rc);
		rc.top=rc.bottom-m_App.StatusView.GetHeight();
		m_App.StatusView.SetPosition(&rc);
		ShowPopupStatusBar(true);
	}
	m_App.StatusView.SetSingleText(TEXT("起動中..."));

	m_App.OSDManager.Initialize();
	m_App.OSDManager.SetEventHandler(this);

	if (m_fCustomFrame) {
		CAeroGlass Aero;
		Aero.EnableNcRendering(m_hwnd,false);
		HookWindows(m_LayoutBase.GetHandle());
		//HookWindows(m_App.Panel.Form.GetHandle());
	}

	// IME無効化
	::ImmAssociateContext(m_hwnd,nullptr);
	::ImmAssociateContextEx(m_hwnd,nullptr,IACE_CHILDREN);

	m_App.MainMenu.Create(m_App.GetResourceInstance());

	m_App.CommandList.SetEventHandler(&m_CommandEventHandler);

	m_pCore->SetCommandCheckedState(CM_ALWAYSONTOP,m_pCore->GetAlwaysOnTop());
	int Gain,SurroundGain;
	m_App.CoreEngine.GetAudioGainControl(&Gain,&SurroundGain);
	for (int i=0;i<lengthof(m_AudioGainList);i++) {
		if (Gain==m_AudioGainList[i]) {
			m_pCore->SetCommandRadioCheckedState(
				CM_AUDIOGAIN_FIRST,CM_AUDIOGAIN_LAST,CM_AUDIOGAIN_FIRST+i);
		}
		if (SurroundGain==m_AudioGainList[i]) {
			m_pCore->SetCommandRadioCheckedState(
				CM_SURROUNDAUDIOGAIN_FIRST,CM_SURROUNDAUDIOGAIN_LAST,
				CM_SURROUNDAUDIOGAIN_FIRST+i);
		}
	}
	m_pCore->SetCommandRadioCheckedState(CM_CAPTURESIZE_FIRST,CM_CAPTURESIZE_LAST,
		CM_CAPTURESIZE_FIRST+m_App.CaptureOptions.GetPresetCaptureSize());
	//m_pCore->SetCommandCheckedState(CM_CAPTUREPREVIEW,m_App.CaptureWindow.GetVisible());
	m_pCore->SetCommandCheckedState(CM_DISABLEVIEWER,!m_fEnablePlayback);
	m_pCore->SetCommandCheckedState(CM_PANEL,m_App.Panel.fShowPanelWindow);
	m_pCore->SetCommandCheckedState(CM_1SEGMODE,m_App.Core.Is1SegMode());

	m_pCore->SetCommandCheckedState(CM_SPDIF_TOGGLE,
		m_App.CoreEngine.IsSpdifPassthroughEnabled());

	m_pCore->SetCommandCheckedState(CM_TITLEBAR,m_fShowTitleBar);
	m_pCore->SetCommandCheckedState(CM_STATUSBAR,m_fShowStatusBar);
	m_pCore->SetCommandCheckedState(CM_SIDEBAR,m_fShowSideBar);

	HMENU hSysMenu=::GetSystemMenu(m_hwnd,FALSE);
	::InsertMenu(hSysMenu,0,MF_BYPOSITION | MF_STRING | MF_ENABLED,
				 SC_ABOUT,TEXT("バージョン情報(&A)"));
	::InsertMenu(hSysMenu,1,MF_BYPOSITION | MF_SEPARATOR,0,nullptr);

	static const CIconMenu::ItemInfo AspectRatioMenuItems[] = {
		{CM_ASPECTRATIO_DEFAULT,	0},
		{CM_ASPECTRATIO_16x9,		1},
		{CM_ASPECTRATIO_LETTERBOX,	2},
		{CM_ASPECTRATIO_SUPERFRAME,	3},
		{CM_ASPECTRATIO_SIDECUT,	4},
		{CM_ASPECTRATIO_4x3,		5},
		{CM_ASPECTRATIO_32x9,		6},
		{CM_ASPECTRATIO_16x9_LEFT,	7},
		{CM_ASPECTRATIO_16x9_RIGHT,	8},
		{CM_FRAMECUT,				9},
		{CM_PANANDSCANOPTIONS,		10},
	};
	HMENU hmenuAspectRatio=m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_ASPECTRATIO);
	m_App.AspectRatioIconMenu.Initialize(hmenuAspectRatio,
										 m_App.GetInstance(),MAKEINTRESOURCE(IDB_PANSCAN),16,
										 AspectRatioMenuItems,lengthof(AspectRatioMenuItems));
	if (m_AspectRatioType<ASPECTRATIO_CUSTOM) {
		m_pCore->SetCommandRadioCheckedState(
			CM_ASPECTRATIO_FIRST,CM_ASPECTRATIO_3D_LAST,
			CM_ASPECTRATIO_FIRST+m_AspectRatioType);
	}
	m_DefaultAspectRatioMenuItemCount=::GetMenuItemCount(hmenuAspectRatio);

	if (!m_App.TaskbarOptions.GetUseUniqueAppID())
		m_App.TaskbarManager.SetAppID(m_App.TaskbarOptions.GetAppID().c_str());
	m_App.TaskbarManager.Initialize(m_hwnd);

	m_App.NotifyBalloonTip.Initialize(m_hwnd);

	m_App.CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(
		(pcs->style&WS_MAXIMIZE)!=0?
			m_App.VideoOptions.GetMaximizeStretchMode():
			m_fFrameCut?CMediaViewer::STRETCH_CUTFRAME:
						CMediaViewer::STRETCH_KEEPASPECTRATIO);

	m_App.EpgCaptureManager.SetEventHandler(&m_EpgCaptureEventHandler);

	::SetTimer(m_hwnd,TIMER_ID_UPDATE,UPDATE_TIMER_INTERVAL,nullptr);

	m_fShowCursor=true;
	m_ShowCursorManager.Reset();
	if (m_App.OperationOptions.GetHideCursor())
		::SetTimer(m_hwnd,TIMER_ID_HIDECURSOR,HIDE_CURSOR_DELAY,nullptr);

	return true;
}


void CMainWindow::OnDestroy()
{
	RECT rc;
	GetPosition(&rc);
	if (m_WindowSizeMode==WINDOW_SIZE_1SEG)
		m_1SegWindowSize=rc;
	else
		m_HDWindowSize=rc;

	m_App.SetEnablePlaybackOnStart(m_fEnablePlayback);
	m_PanelPaneIndex=GetPanelPaneIndex();

	m_App.HtmlHelpClass.Finalize();
	m_pCore->PreventDisplaySave(false);

	m_App.EpgCaptureManager.SetEventHandler(nullptr);

	m_App.Finalize();

	CBasicWindow::OnDestroy();
}


void CMainWindow::OnSizeChanged(UINT State,int Width,int Height)
{
	const bool fMinimized=State==SIZE_MINIMIZED;
	const bool fMaximized=State==SIZE_MAXIMIZED;

	if (fMinimized) {
		m_App.OSDManager.ClearOSD();
		m_App.OSDManager.Reset();
		m_App.TaskTrayManager.SetStatus(CTaskTrayManager::STATUS_MINIMIZED,
										CTaskTrayManager::STATUS_MINIMIZED);
		if (m_App.ViewOptions.GetDisablePreviewWhenMinimized()) {
			SuspendViewer(ResumeInfo::VIEWERSUSPEND_MINIMIZE);
		}
	} else if ((m_App.TaskTrayManager.GetStatus() & CTaskTrayManager::STATUS_MINIMIZED)!=0) {
		SetWindowVisible();
	}

	m_TitleBar.SetMaximizeMode(fMaximized);

	if (m_fLockLayout || fMinimized)
		return;

	m_LayoutBase.SetPosition(0,0,Width,Height);

	if (!m_pCore->GetFullscreen()) {
		if (State==SIZE_MAXIMIZED) {
			m_App.CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(
				m_App.VideoOptions.GetMaximizeStretchMode());
		} else if (State==SIZE_RESTORED) {
			m_App.CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(
				m_fFrameCut?CMediaViewer::STRETCH_CUTFRAME:
							CMediaViewer::STRETCH_KEEPASPECTRATIO);
		}
	}

	m_App.StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
	m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VIDEO);
}


bool CMainWindow::OnSizeChanging(UINT Edge,RECT *pRect)
{
	RECT rcOld;
	bool fResizePanel=false,fChanged=false;

	GetPosition(&rcOld);

	if (m_fEnterSizeMove) {
		if (::GetKeyState(VK_CONTROL)<0)
			fResizePanel=true;
		if (m_fResizePanel!=fResizePanel) {
			Layout::CSplitter *pSplitter=dynamic_cast<Layout::CSplitter*>(
				m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));

			if (fResizePanel)
				pSplitter->SetAdjustPane(CONTAINER_ID_PANEL);
			else
				pSplitter->SetAdjustPane(pSplitter->GetPane(!pSplitter->IDToIndex(CONTAINER_ID_PANEL))->GetID());
			m_fResizePanel=fResizePanel;
		}
	}

	bool fKeepRatio=false;
	if (!fResizePanel) {
		fKeepRatio=m_App.ViewOptions.GetAdjustAspectResizing();
		if (::GetKeyState(VK_SHIFT)<0)
			fKeepRatio=!fKeepRatio;
	}
	if (fKeepRatio) {
		BYTE XAspect,YAspect;

		if (m_App.CoreEngine.m_DtvEngine.m_MediaViewer.GetEffectiveAspectRatio(&XAspect,&YAspect)) {
			RECT rcWindow,rcClient;
			int XMargin,YMargin,Width,Height;

			GetPosition(&rcWindow);
			GetClientRect(&rcClient);
			m_Display.GetViewWindow().CalcClientRect(&rcClient);
			if (m_fShowStatusBar)
				rcClient.bottom-=m_App.StatusView.GetHeight();
			if (m_fShowTitleBar && m_fCustomTitleBar)
				m_TitleBarManager.AdjustArea(&rcClient);
			if (m_fShowSideBar)
				m_SideBarManager.AdjustArea(&rcClient);
			if (m_App.Panel.fShowPanelWindow && !m_App.Panel.IsFloating()) {
				Layout::CSplitter *pSplitter=dynamic_cast<Layout::CSplitter*>(
					m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));
				if (m_App.Panel.Frame.IsDockingVertical())
					rcClient.bottom-=pSplitter->GetPaneSize(CONTAINER_ID_PANEL)+pSplitter->GetBarWidth();
				else
					rcClient.right-=pSplitter->GetPaneSize(CONTAINER_ID_PANEL)+pSplitter->GetBarWidth();
			}
			::OffsetRect(&rcClient,-rcClient.left,-rcClient.top);
			if (rcClient.right<=0 || rcClient.bottom<=0)
				goto SizingEnd;
			XMargin=(rcWindow.right-rcWindow.left)-rcClient.right;
			YMargin=(rcWindow.bottom-rcWindow.top)-rcClient.bottom;
			Width=(pRect->right-pRect->left)-XMargin;
			Height=(pRect->bottom-pRect->top)-YMargin;
			if (Width<=0 || Height<=0)
				goto SizingEnd;
			if (Edge==WMSZ_LEFT || Edge==WMSZ_RIGHT)
				Height=Width*YAspect/XAspect;
			else if (Edge==WMSZ_TOP || Edge==WMSZ_BOTTOM)
				Width=Height*XAspect/YAspect;
			else if (Width*YAspect<Height*XAspect)
				Width=Height*XAspect/YAspect;
			else if (Width*YAspect>Height*XAspect)
				Height=Width*YAspect/XAspect;
			if (Edge==WMSZ_LEFT || Edge==WMSZ_TOPLEFT || Edge==WMSZ_BOTTOMLEFT)
				pRect->left=pRect->right-(Width+XMargin);
			else
				pRect->right=pRect->left+Width+XMargin;
			if (Edge==WMSZ_TOP || Edge==WMSZ_TOPLEFT || Edge==WMSZ_TOPRIGHT)
				pRect->top=pRect->bottom-(Height+YMargin);
			else
				pRect->bottom=pRect->top+Height+YMargin;
			fChanged=true;
		}
	}
SizingEnd:
	m_App.Panel.OnOwnerMovingOrSizing(&rcOld,pRect);
	return fChanged;
}


void CMainWindow::OnGetMinMaxInfo(HWND hwnd,LPMINMAXINFO pmmi)
{
	SIZE sz;
	RECT rc;

	m_LayoutBase.GetMinSize(&sz);
	::SetRect(&rc,0,0,sz.cx,sz.cy);
	CalcPositionFromClientRect(&rc);
	pmmi->ptMinTrackSize.x=rc.right-rc.left;
	pmmi->ptMinTrackSize.y=rc.bottom-rc.top;

	if (!m_fShowTitleBar || m_fCustomTitleBar) {
		HMONITOR hMonitor=::MonitorFromWindow(hwnd,MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO mi;

		mi.cbSize=sizeof(MONITORINFO);
		if (::GetMonitorInfo(hMonitor,&mi)) {
			RECT Border;

			if (m_fCustomFrame) {
				Border.left=Border.top=Border.right=Border.bottom=m_CustomFrameWidth;
			} else {
				RECT rcClient,rcWindow;

				::GetClientRect(hwnd,&rcClient);
				MapWindowRect(hwnd,NULL,&rcClient);
				::GetWindowRect(hwnd,&rcWindow);
				Border.left=rcClient.left-rcWindow.left;
				Border.top=rcClient.top-rcWindow.top;
				Border.right=rcWindow.right-rcClient.right;
				Border.bottom=rcWindow.bottom-rcClient.bottom;
			}

			pmmi->ptMaxSize.x=(mi.rcWork.right-mi.rcWork.left)+Border.left+Border.right;
			pmmi->ptMaxSize.y=(mi.rcWork.bottom-mi.rcWork.top)+Border.top+Border.bottom;
			pmmi->ptMaxPosition.x=mi.rcWork.left-mi.rcMonitor.left-Border.left;
			pmmi->ptMaxPosition.y=mi.rcWork.top-mi.rcMonitor.top-Border.top;
			if (::IsZoomed(hwnd)) {
				// ウィンドウのあるモニタがプライマリモニタよりも大きい場合、
				// ptMaxTrackSize を設定しないとサイズがおかしくなる
				pmmi->ptMaxTrackSize=pmmi->ptMaxSize;
			} else {
				pmmi->ptMaxTrackSize.x=::GetSystemMetrics(SM_CXVIRTUALSCREEN);
				pmmi->ptMaxTrackSize.y=::GetSystemMetrics(SM_CYVIRTUALSCREEN);
			}
		}
	}
}


void CMainWindow::OnMouseMove(int x,int y)
{
	if (m_fDragging) {
		// ウィンドウ移動中
		POINT pt;
		RECT rcOld,rc;

		/*
		pt.x=x;
		pt.y=y;
		::ClientToScreen(hwnd,&pt);
		*/
		::GetCursorPos(&pt);
		::GetWindowRect(m_hwnd,&rcOld);
		rc.left=m_rcDragStart.left+(pt.x-m_ptDragStartPos.x);
		rc.top=m_rcDragStart.top+(pt.y-m_ptDragStartPos.y);
		rc.right=rc.left+(m_rcDragStart.right-m_rcDragStart.left);
		rc.bottom=rc.top+(m_rcDragStart.bottom-m_rcDragStart.top);
		bool fSnap=m_App.ViewOptions.GetSnapAtWindowEdge();
		if (::GetKeyState(VK_SHIFT)<0)
			fSnap=!fSnap;
		if (fSnap)
			SnapWindow(m_hwnd,&rc,
					   m_App.ViewOptions.GetSnapAtWindowEdgeMargin(),
					   m_App.Panel.IsAttached()?nullptr:m_App.Panel.Frame.GetHandle());
		SetPosition(&rc);
		m_App.Panel.OnOwnerMovingOrSizing(&rcOld,&rc);
	} else if (!m_pCore->GetFullscreen()) {
		POINT pt={x,y};
		RECT rcClient,rcTitle,rcStatus,rcSideBar,rc;
		bool fShowTitleBar=false,fShowStatusBar=false,fShowSideBar=false;

		m_Display.GetViewWindow().GetClientRect(&rcClient);
		MapWindowRect(m_Display.GetViewWindow().GetHandle(),m_LayoutBase.GetHandle(),&rcClient);
		if (!m_fShowTitleBar && m_fPopupTitleBar) {
			rc=rcClient;
			m_TitleBarManager.Layout(&rc,&rcTitle);
			if (::PtInRect(&rcTitle,pt))
				fShowTitleBar=true;
		}
		if (!m_fShowStatusBar && m_App.StatusOptions.GetShowPopup()) {
			rcStatus=rcClient;
			rcStatus.top=rcStatus.bottom-m_App.StatusView.CalcHeight(rcClient.right-rcClient.left);
			if (::PtInRect(&rcStatus,pt))
				fShowStatusBar=true;
		}
		if (!m_fShowSideBar && m_App.SideBarOptions.ShowPopup()) {
			switch (m_App.SideBarOptions.GetPlace()) {
			case CSideBarOptions::PLACE_LEFT:
			case CSideBarOptions::PLACE_RIGHT:
				if (!fShowStatusBar && !fShowTitleBar) {
					m_SideBarManager.Layout(&rcClient,&rcSideBar);
					if (::PtInRect(&rcSideBar,pt))
						fShowSideBar=true;
				}
				break;
			case CSideBarOptions::PLACE_TOP:
				if (!m_fShowTitleBar && m_fPopupTitleBar)
					rcClient.top=rcTitle.bottom;
				m_SideBarManager.Layout(&rcClient,&rcSideBar);
				if (::PtInRect(&rcSideBar,pt)) {
					fShowSideBar=true;
					if (!m_fShowTitleBar && m_fPopupTitleBar)
						fShowTitleBar=true;
				}
				break;
			case CSideBarOptions::PLACE_BOTTOM:
				if (!m_fShowStatusBar && m_App.StatusOptions.GetShowPopup())
					rcClient.bottom=rcStatus.top;
				m_SideBarManager.Layout(&rcClient,&rcSideBar);
				if (::PtInRect(&rcSideBar,pt)) {
					fShowSideBar=true;
					if (!m_fShowStatusBar && m_App.StatusOptions.GetShowPopup())
						fShowStatusBar=true;
				}
				break;
			}
		}

		if (fShowTitleBar) {
			if (!m_TitleBar.GetVisible()) {
				m_TitleBar.SetPosition(&rcTitle);
				m_TitleBar.SetVisible(true);
				::BringWindowToTop(m_TitleBar.GetHandle());
			}
		} else if (!m_fShowTitleBar && m_TitleBar.GetVisible()) {
			m_TitleBar.SetVisible(false);
		}
		if (fShowStatusBar) {
			if (!m_App.StatusView.GetVisible()) {
				m_App.StatusView.SetPosition(&rcStatus);
				ShowPopupStatusBar(true);
			}
		} else if (!m_fShowStatusBar && m_App.StatusView.GetVisible()) {
			ShowPopupStatusBar(false);
		}
		if (fShowSideBar) {
			if (!m_App.SideBar.GetVisible()) {
				m_App.SideBar.SetPosition(&rcSideBar);
				ShowPopupSideBar(true);
			}
		} else if (!m_fShowSideBar && m_App.SideBar.GetVisible()) {
			ShowPopupSideBar(false);
		}

		if (m_ShowCursorManager.OnCursorMove(x,y)) {
			if (!m_fShowCursor)
				ShowCursor(true);
		}
		if (m_App.OperationOptions.GetHideCursor())
			::SetTimer(m_hwnd,TIMER_ID_HIDECURSOR,HIDE_CURSOR_DELAY,nullptr);
	} else {
		m_Fullscreen.OnMouseMove();
	}
}


bool CMainWindow::OnSetCursor(HWND hwndCursor,int HitTestCode,int MouseMessage)
{
	if (HitTestCode==HTCLIENT) {
		if (hwndCursor==m_hwnd
				|| hwndCursor==m_Display.GetVideoContainer().GetHandle()
				|| hwndCursor==m_Display.GetViewWindow().GetHandle()
				|| hwndCursor==m_NotificationBar.GetHandle()
				|| CPseudoOSD::IsPseudoOSD(hwndCursor)) {
			::SetCursor(m_fShowCursor?::LoadCursor(nullptr,IDC_ARROW):nullptr);
			return true;
		}
	}

	return false;
}


bool CMainWindow::OnKeyDown(WPARAM KeyCode)
{
	CChannelInput::KeyDownResult Result=m_ChannelInput.OnKeyDown(KeyCode);

	if (Result!=CChannelInput::KEYDOWN_NOTPROCESSED) {
		switch (Result) {
		case CChannelInput::KEYDOWN_COMPLETED:
			EndChannelNoInput(true);
			break;

		case CChannelInput::KEYDOWN_CANCELLED:
			EndChannelNoInput(false);
			break;

		case CChannelInput::KEYDOWN_BEGIN:
		case CChannelInput::KEYDOWN_CONTINUE:
			OnChannelNoInput();
			break;
		}

		return true;
	}

	if (KeyCode>=VK_F13 && KeyCode<=VK_F24
			&& (::GetKeyState(VK_SHIFT)<0 || ::GetKeyState(VK_CONTROL)<0)
			&& !m_App.ControllerManager.IsControllerEnabled(TEXT("HDUS Remocon"))) {
		ShowMessage(TEXT("リモコンを使用するためには、メニューの [プラグイン] -> [HDUSリモコン] でリモコンを有効にしてください。"),
					TEXT("お知らせ"),MB_OK | MB_ICONINFORMATION);
	}

	return false;
}


void CMainWindow::OnCommand(HWND hwnd,int id,HWND hwndCtl,UINT codeNotify)
{
	switch (id) {
	case CM_ZOOMOPTIONS:
		if (m_App.ZoomOptions.Show(GetVideoHostWindow())) {
			m_App.SideBarOptions.SetSideBarImage();
			m_App.ZoomOptions.SaveSettings(m_App.GetIniFileName());
		}
		return;

	case CM_ASPECTRATIO:
		{
			int Command;

			if (m_AspectRatioType>=ASPECTRATIO_CUSTOM)
				Command=CM_ASPECTRATIO_DEFAULT;
			else if (m_AspectRatioType<ASPECTRATIO_32x9)
				Command=CM_ASPECTRATIO_FIRST+
					(m_AspectRatioType+1)%(CM_ASPECTRATIO_LAST-CM_ASPECTRATIO_FIRST+1);
			else
				Command=CM_ASPECTRATIO_3D_FIRST+
					(m_AspectRatioType-ASPECTRATIO_32x9+1)%(CM_ASPECTRATIO_3D_LAST-CM_ASPECTRATIO_3D_FIRST+1);
			SetPanAndScan(Command);
		}
		return;

	case CM_ASPECTRATIO_DEFAULT:
	case CM_ASPECTRATIO_16x9:
	case CM_ASPECTRATIO_LETTERBOX:
	case CM_ASPECTRATIO_SUPERFRAME:
	case CM_ASPECTRATIO_SIDECUT:
	case CM_ASPECTRATIO_4x3:
	case CM_ASPECTRATIO_32x9:
	case CM_ASPECTRATIO_16x9_LEFT:
	case CM_ASPECTRATIO_16x9_RIGHT:
		SetPanAndScan(id);
		return;

	case CM_PANANDSCANOPTIONS:
		{
			bool fSet=false;
			CPanAndScanOptions::PanAndScanInfo CurPanScan;

			if (m_AspectRatioType>=ASPECTRATIO_CUSTOM)
				fSet=m_App.PanAndScanOptions.GetPreset(m_AspectRatioType-ASPECTRATIO_CUSTOM,&CurPanScan);

			if (m_App.PanAndScanOptions.Show(GetVideoHostWindow())) {
				if (fSet) {
					CPanAndScanOptions::PanAndScanInfo NewPanScan;
					int Index=m_App.PanAndScanOptions.FindPresetByID(CurPanScan.ID);
					if (Index>=0 && m_App.PanAndScanOptions.GetPreset(Index,&NewPanScan)) {
						if (NewPanScan.Info!=CurPanScan.Info)
							SetPanAndScan(CM_PANANDSCAN_PRESET_FIRST+Index);
					} else {
						SetPanAndScan(CM_ASPECTRATIO_DEFAULT);
					}
				}
				m_App.PanAndScanOptions.SaveSettings(m_App.GetIniFileName());
			}
		}
		return;

	case CM_FRAMECUT:
		m_fFrameCut=!m_fFrameCut;
		m_App.CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(
			m_fFrameCut?CMediaViewer::STRETCH_CUTFRAME:
						CMediaViewer::STRETCH_KEEPASPECTRATIO);
		return;

	case CM_FULLSCREEN:
		m_pCore->ToggleFullscreen();
		return;

	case CM_ALWAYSONTOP:
		m_pCore->SetAlwaysOnTop(!m_pCore->GetAlwaysOnTop());
		return;

	case CM_VOLUME_UP:
	case CM_VOLUME_DOWN:
		{
			const int CurVolume=m_pCore->GetVolume();
			int Volume=CurVolume;

			if (id==CM_VOLUME_UP) {
				Volume+=m_App.OperationOptions.GetVolumeStep();
				if (Volume>CCoreEngine::MAX_VOLUME)
					Volume=CCoreEngine::MAX_VOLUME;
			} else {
				Volume-=m_App.OperationOptions.GetVolumeStep();
				if (Volume<0)
					Volume=0;
			}
			if (Volume!=CurVolume || m_pCore->GetMute())
				m_pCore->SetVolume(Volume);
		}
		return;

	case CM_VOLUME_MUTE:
		m_pCore->SetMute(!m_pCore->GetMute());
		return;

	case CM_AUDIOGAIN_NONE:
	case CM_AUDIOGAIN_125:
	case CM_AUDIOGAIN_150:
	case CM_AUDIOGAIN_200:
		{
			int SurroundGain;

			m_App.CoreEngine.GetAudioGainControl(nullptr,&SurroundGain);
			m_App.CoreEngine.SetAudioGainControl(
				m_AudioGainList[id-CM_AUDIOGAIN_FIRST],SurroundGain);
			m_pCore->SetCommandRadioCheckedState(CM_AUDIOGAIN_NONE,CM_AUDIOGAIN_LAST,id);
		}
		return;

	case CM_SURROUNDAUDIOGAIN_NONE:
	case CM_SURROUNDAUDIOGAIN_125:
	case CM_SURROUNDAUDIOGAIN_150:
	case CM_SURROUNDAUDIOGAIN_200:
		{
			int Gain;

			m_App.CoreEngine.GetAudioGainControl(&Gain,nullptr);
			m_App.CoreEngine.SetAudioGainControl(
				Gain,m_AudioGainList[id-CM_SURROUNDAUDIOGAIN_FIRST]);
			m_pCore->SetCommandRadioCheckedState(CM_SURROUNDAUDIOGAIN_NONE,CM_SURROUNDAUDIOGAIN_LAST,id);
		}
		return;

	case CM_AUDIODELAY_MINUS:
	case CM_AUDIODELAY_PLUS:
	case CM_AUDIODELAY_RESET:
		{
			CMediaViewer &MediaViewer=m_App.CoreEngine.m_DtvEngine.m_MediaViewer;
			static const LONGLONG MaxDelay=10000000LL;
			const LONGLONG Step=m_App.OperationOptions.GetAudioDelayStep()*10000LL;
			LONGLONG Delay;

			switch (id) {
			case CM_AUDIODELAY_MINUS:
				Delay=MediaViewer.GetAudioDelay()-Step;
				break;
			case CM_AUDIODELAY_PLUS:
				Delay=MediaViewer.GetAudioDelay()+Step;
				break;
			case CM_AUDIODELAY_RESET:
				Delay=0;
				break;
			}

			MediaViewer.SetAudioDelay(CLAMP(Delay,-MaxDelay,MaxDelay));
		}
		return;

	case CM_DUALMONO_MAIN:
	case CM_DUALMONO_SUB:
	case CM_DUALMONO_BOTH:
		m_pCore->SetDualMonoMode(
			id==CM_DUALMONO_MAIN?CAudioDecFilter::DUALMONO_MAIN:
			id==CM_DUALMONO_SUB ?CAudioDecFilter::DUALMONO_SUB:
			                     CAudioDecFilter::DUALMONO_BOTH);
		ShowAudioOSD();
		return;

	case CM_STEREOMODE_STEREO:
	case CM_STEREOMODE_LEFT:
	case CM_STEREOMODE_RIGHT:
		m_pCore->SetStereoMode(
			id==CM_STEREOMODE_STEREO?CAudioDecFilter::STEREOMODE_STEREO:
			id==CM_STEREOMODE_LEFT  ?CAudioDecFilter::STEREOMODE_LEFT:
			                         CAudioDecFilter::STEREOMODE_RIGHT);
		ShowAudioOSD();
		return;

	case CM_SWITCHAUDIO:
		m_pCore->SwitchAudio();
		ShowAudioOSD();
		return;

	case CM_SPDIF_DISABLED:
	case CM_SPDIF_PASSTHROUGH:
	case CM_SPDIF_AUTO:
		{
			CAudioDecFilter::SpdifOptions Options(m_App.AudioOptions.GetSpdifOptions());

			Options.Mode=(CAudioDecFilter::SpdifMode)(id-CM_SPDIF_DISABLED);
			m_App.CoreEngine.SetSpdifOptions(Options);
			m_pCore->SetCommandCheckedState(CM_SPDIF_TOGGLE,
				m_App.CoreEngine.IsSpdifPassthroughEnabled());
		}
		return;

	case CM_SPDIF_TOGGLE:
		{
			CAudioDecFilter::SpdifOptions Options;

			m_App.CoreEngine.GetSpdifOptions(&Options);
			if (m_App.CoreEngine.IsSpdifPassthroughEnabled())
				Options.Mode=CAudioDecFilter::SPDIF_MODE_DISABLED;
			else
				Options.Mode=CAudioDecFilter::SPDIF_MODE_PASSTHROUGH;
			m_App.CoreEngine.SetSpdifOptions(Options);
			m_pCore->SetCommandCheckedState(CM_SPDIF_TOGGLE,
				m_App.CoreEngine.IsSpdifPassthroughEnabled());
		}
		return;

	case CM_CAPTURE:
		SendCommand(m_App.CaptureOptions.TranslateCommand(CM_CAPTURE));
		return;

	case CM_COPY:
	case CM_SAVEIMAGE:
		if (IsViewerEnabled()) {
			HCURSOR hcurOld=::SetCursor(::LoadCursor(nullptr,IDC_WAIT));
			BYTE *pDib;

			pDib=static_cast<BYTE*>(m_App.CoreEngine.GetCurrentImage());
			if (pDib==nullptr) {
				::SetCursor(hcurOld);
				ShowMessage(TEXT("現在の画像を取得できません。\n")
							TEXT("レンダラやデコーダを変えてみてください。"),TEXT("ごめん"),
							MB_OK | MB_ICONEXCLAMATION);
				return;
			}

			CMediaViewer &MediaViewer=m_App.CoreEngine.m_DtvEngine.m_MediaViewer;
			BITMAPINFOHEADER *pbmih=(BITMAPINFOHEADER*)pDib;
			RECT rc;
			int Width,Height,OrigWidth,OrigHeight;
			HGLOBAL hGlobal=nullptr;

			OrigWidth=pbmih->biWidth;
			OrigHeight=abs(pbmih->biHeight);
			if (MediaViewer.GetSourceRect(&rc)) {
				WORD VideoWidth,VideoHeight;

				if (MediaViewer.GetOriginalVideoSize(&VideoWidth,&VideoHeight)
						&& (VideoWidth!=OrigWidth
							|| VideoHeight!=OrigHeight)) {
					rc.left=rc.left*OrigWidth/VideoWidth;
					rc.top=rc.top*OrigHeight/VideoHeight;
					rc.right=rc.right*OrigWidth/VideoWidth;
					rc.bottom=rc.bottom*OrigHeight/VideoHeight;
				}
				if (rc.right>OrigWidth)
					rc.right=OrigWidth;
				if (rc.bottom>OrigHeight)
					rc.bottom=OrigHeight;
			} else {
				rc.left=0;
				rc.top=0;
				rc.right=OrigWidth;
				rc.bottom=OrigHeight;
			}
			if (OrigHeight==1088) {
				rc.top=rc.top*1080/1088;
				rc.bottom=rc.bottom*1080/1088;
			}
			switch (m_App.CaptureOptions.GetCaptureSizeType()) {
			case CCaptureOptions::SIZE_TYPE_ORIGINAL:
				m_App.CoreEngine.GetVideoViewSize(&Width,&Height);
				break;
			case CCaptureOptions::SIZE_TYPE_VIEW:
				{
					WORD w,h;

					MediaViewer.GetDestSize(&w,&h);
					Width=w;
					Height=h;
				}
				break;
			/*
			case CCaptureOptions::SIZE_RAW:
				rc.left=rc.top=0;
				rc.right=OrigWidth;
				rc.bottom=OrigHeight;
				Width=OrigWidth;
				Height=OrigHeight;
				break;
			*/
			case CCaptureOptions::SIZE_TYPE_PERCENTAGE:
				{
					int Num,Denom;

					m_App.CoreEngine.GetVideoViewSize(&Width,&Height);
					m_App.CaptureOptions.GetSizePercentage(&Num,&Denom);
					Width=Width*Num/Denom;
					Height=Height*Num/Denom;
				}
				break;
			case CCaptureOptions::SIZE_TYPE_CUSTOM:
				m_App.CaptureOptions.GetCustomSize(&Width,&Height);
				break;
			}
			hGlobal=ResizeImage((BITMAPINFO*)pbmih,
								pDib+CalcDIBInfoSize(pbmih),&rc,Width,Height);
			::CoTaskMemFree(pDib);
			::SetCursor(hcurOld);
			if (hGlobal==nullptr) {
				return;
			}
			CCaptureImage *pImage=new CCaptureImage(hGlobal);
			if (m_App.CaptureOptions.GetWriteComment()) {
				String Comment;
				if (m_App.CaptureOptions.GetCommentText(&Comment,pImage))
					pImage->SetComment(Comment.c_str());
			}
			m_App.CaptureWindow.SetImage(pImage);
			if (id==CM_COPY) {
				if (!pImage->SetClipboard(hwnd)) {
					ShowErrorMessage(TEXT("クリップボードにデータを設定できません。"));
				}
			} else {
				if (!m_App.CaptureOptions.SaveImage(pImage)) {
					ShowErrorMessage(TEXT("画像の保存でエラーが発生しました。"));
				}
			}
			if (!m_App.CaptureWindow.HasImage())
				delete pImage;
		}
		return;

	case CM_CAPTUREPREVIEW:
		{
			if (!m_App.CaptureWindow.GetVisible()) {
				if (!m_App.CaptureWindow.IsCreated()) {
					m_App.CaptureWindow.Create(hwnd,
						WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |
							WS_VISIBLE | WS_CLIPCHILDREN,
						WS_EX_TOOLWINDOW);
				} else {
					m_App.CaptureWindow.SetVisible(true);
				}
			} else {
				m_App.CaptureWindow.Destroy();
				m_App.CaptureWindow.ClearImage();
			}

			m_pCore->SetCommandCheckedState(
				CM_CAPTUREPREVIEW,m_App.CaptureWindow.GetVisible());
		}
		return;

	case CM_CAPTUREOPTIONS:
		if (IsWindowEnabled(hwnd))
			m_App.ShowOptionDialog(hwnd,COptionDialog::PAGE_CAPTURE);
		return;

	case CM_OPENCAPTUREFOLDER:
		m_App.CaptureOptions.OpenSaveFolder();
		return;

	case CM_RESET:
		m_App.Core.ResetEngine();
		return;

	case CM_RESETVIEWER:
		m_App.CoreEngine.m_DtvEngine.ResetMediaViewer();
		return;

	case CM_REBUILDVIEWER:
		InitializeViewer();
		return;

	case CM_RECORD:
	case CM_RECORD_START:
	case CM_RECORD_STOP:
		if (id==CM_RECORD) {
			if (m_App.RecordManager.IsPaused()) {
				SendCommand(CM_RECORD_PAUSE);
				return;
			}
		} else if (id==CM_RECORD_START) {
			if (m_App.RecordManager.IsRecording()) {
				if (m_App.RecordManager.IsPaused())
					SendCommand(CM_RECORD_PAUSE);
				return;
			}
		} else if (id==CM_RECORD_STOP) {
			if (!m_App.RecordManager.IsRecording())
				return;
		}
		if (m_App.RecordManager.IsRecording()) {
			if (!m_App.RecordManager.IsPaused()
					&& !m_App.RecordOptions.ConfirmStop(GetVideoHostWindow()))
				return;
			m_App.Core.StopRecord();
		} else {
			if (m_App.RecordManager.IsReserved()) {
				if (ShowMessage(
						TEXT("既に設定されている録画があります。\n")
						TEXT("録画を開始すると既存の設定が破棄されます。\n")
						TEXT("録画を開始してもいいですか?"),
						TEXT("録画開始の確認"),
						MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2)!=IDOK) {
					return;
				}
			}
			m_App.Core.StartRecord();
		}
		return;

	case CM_RECORD_PAUSE:
		m_App.Core.PauseResumeRecording();
		return;

	case CM_RECORDOPTION:
		if (IsWindowEnabled(GetVideoHostWindow())) {
			if (m_App.RecordManager.IsRecording()) {
				if (m_App.RecordManager.RecordDialog(GetVideoHostWindow()))
					m_App.StatusView.UpdateItem(STATUS_ITEM_RECORD);
			} else {
				if (m_App.RecordManager.GetFileName()==nullptr) {
					TCHAR szFileName[MAX_PATH];

					if (m_App.RecordOptions.GetFilePath(szFileName,MAX_PATH))
						m_App.RecordManager.SetFileName(szFileName);
				}
				if (!m_App.RecordManager.IsReserved())
					m_App.RecordOptions.GetRecordingSettings(&m_App.RecordManager.GetRecordingSettings());
				if (m_App.RecordManager.RecordDialog(GetVideoHostWindow())) {
					m_App.RecordManager.SetClient(CRecordManager::CLIENT_USER);
					if (m_App.RecordManager.IsReserved()) {
						m_App.StatusView.UpdateItem(STATUS_ITEM_RECORD);
					} else {
						m_App.Core.StartReservedRecord();
					}
				} else {
					// 予約がキャンセルされた場合も表示を更新する
					m_App.StatusView.UpdateItem(STATUS_ITEM_RECORD);
				}
			}
		}
		return;

	case CM_RECORDEVENT:
		if (m_App.RecordManager.IsRecording()) {
			m_App.RecordManager.SetStopOnEventEnd(!m_App.RecordManager.GetStopOnEventEnd());
		} else {
			SendCommand(CM_RECORD_START);
			if (m_App.RecordManager.IsRecording())
				m_App.RecordManager.SetStopOnEventEnd(true);
		}
		return;

	case CM_EXITONRECORDINGSTOP:
		m_App.Core.SetExitOnRecordingStop(!m_App.Core.GetExitOnRecordingStop());
		return;

	case CM_OPTIONS_RECORD:
		if (IsWindowEnabled(hwnd))
			m_App.ShowOptionDialog(hwnd,COptionDialog::PAGE_RECORD);
		return;

	case CM_TIMESHIFTRECORDING:
		if (!m_App.RecordManager.IsRecording()) {
			if (m_App.RecordManager.IsReserved()) {
				if (ShowMessage(
						TEXT("既に設定されている録画があります。\n")
						TEXT("録画を開始すると既存の設定が破棄されます。\n")
						TEXT("録画を開始してもいいですか?"),
						TEXT("録画開始の確認"),
						MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2)!=IDOK) {
					return;
				}
			}
			m_App.Core.StartRecord(nullptr,nullptr,nullptr,CRecordManager::CLIENT_USER,true);
		}
		return;

	case CM_ENABLETIMESHIFTRECORDING:
		m_App.RecordOptions.EnableTimeShiftRecording(!m_App.RecordOptions.IsTimeShiftRecordingEnabled());
		return;

	case CM_STATUSBARRECORD:
		{
			int Command=m_App.RecordOptions.GetStatusBarRecordCommand();
			if (Command!=0)
				OnCommand(hwnd,Command,nullptr,0);
		}
		return;

	case CM_DISABLEVIEWER:
		m_pCore->EnableViewer(!m_fEnablePlayback);
		return;

	case CM_PANEL:
		if (m_pCore->GetFullscreen()) {
			m_Fullscreen.ShowPanel(!m_Fullscreen.IsPanelVisible());
		} else {
			ShowPanel(!m_App.Panel.fShowPanelWindow);
		}
		return;

	case CM_PROGRAMGUIDE:
		ShowProgramGuide(!m_App.Epg.fShowProgramGuide);
		return;

	case CM_STATUSBAR:
		SetStatusBarVisible(!m_fShowStatusBar);
		return;

	case CM_TITLEBAR:
		SetTitleBarVisible(!m_fShowTitleBar);
		return;

	case CM_SIDEBAR:
		SetSideBarVisible(!m_fShowSideBar);
		return;

	case CM_WINDOWFRAME_NORMAL:
		SetCustomFrame(false);
		return;

	case CM_WINDOWFRAME_CUSTOM:
		SetCustomFrame(true,m_ThinFrameWidth);
		return;

	case CM_WINDOWFRAME_NONE:
		SetCustomFrame(true,0);
		return;

	case CM_CUSTOMTITLEBAR:
		SetCustomTitleBar(!m_fCustomTitleBar);
		return;

	case CM_SPLITTITLEBAR:
		SetSplitTitleBar(!m_fSplitTitleBar);
		return;

	case CM_VIDEODECODERPROPERTY:
	case CM_VIDEORENDERERPROPERTY:
	case CM_AUDIOFILTERPROPERTY:
	case CM_AUDIORENDERERPROPERTY:
	case CM_DEMULTIPLEXERPROPERTY:
		{
			HWND hwndOwner=GetVideoHostWindow();

			if (hwndOwner==nullptr || ::IsWindowEnabled(hwndOwner)) {
				for (int i=0;i<lengthof(m_DirectShowFilterPropertyList);i++) {
					if (m_DirectShowFilterPropertyList[i].Command==id) {
						CMediaViewer &MediaViewer=m_App.CoreEngine.m_DtvEngine.m_MediaViewer;
						bool fOK=false;

						if (m_DirectShowFilterPropertyList[i].Filter==CMediaViewer::PROPERTY_FILTER_VIDEODECODER) {
							IBaseFilter *pDecoder = MediaViewer.GetVideoDecoderFilter();
							if (pDecoder!=nullptr) {
								HRESULT hr=ShowPropertyPageFrame(pDecoder,hwndOwner,m_App.GetResourceInstance());
								if (SUCCEEDED(hr)) {
									MediaViewer.SaveVideoDecoderSettings();
									fOK=true;
								}
								pDecoder->Release();
							}
						}

						if (!fOK) {
							MediaViewer.DisplayFilterProperty(m_DirectShowFilterPropertyList[i].Filter,hwndOwner);
						}
						break;
					}
				}
			}
		}
		return;

	case CM_OPTIONS:
		{
			HWND hwndOwner=GetVideoHostWindow();

			if (hwndOwner==nullptr || IsWindowEnabled(hwndOwner))
				m_App.ShowOptionDialog(hwndOwner);
		}
		return;

	case CM_STREAMINFO:
		{
			if (!m_App.StreamInfo.IsVisible()) {
				if (!m_App.StreamInfo.IsCreated())
					m_App.StreamInfo.Create(hwnd);
				else
					m_App.StreamInfo.SetVisible(true);
			} else {
				m_App.StreamInfo.Destroy();
			}

			m_pCore->SetCommandCheckedState(CM_STREAMINFO,m_App.StreamInfo.IsVisible());
		}
		return;

	case CM_CLOSE:
		if (m_pCore->GetStandby()) {
			m_pCore->SetStandby(false);
		} else if (m_App.TaskTrayManager.GetResident()) {
			m_pCore->SetStandby(true,false);
		} else if (m_App.GeneralOptions.GetStandaloneProgramGuide()
				&& m_App.Epg.ProgramGuideFrame.GetVisible()) {
			m_pCore->SetStandby(true,true);
		} else {
			PostMessage(WM_CLOSE,0,0);
		}
		return;

	case CM_EXIT:
		PostMessage(WM_CLOSE,0,0);
		return;

	case CM_SHOW:
		if (m_pCore->GetStandby()) {
			m_pCore->SetStandby(false);
		} else {
			SetWindowVisible();
		}
		return;

	case CM_CHANNEL_UP:
	case CM_CHANNEL_DOWN:
		{
			int Channel=m_App.ChannelManager.GetNextChannel(id==CM_CHANNEL_UP);

			if (Channel>=0)
				m_App.Core.SwitchChannel(Channel);
		}
		return;

	case CM_CHANNEL_BACKWARD:
	case CM_CHANNEL_FORWARD:
		{
			const CTunerChannelInfo *pChannel;

			if (id==CM_CHANNEL_BACKWARD)
				pChannel=m_App.ChannelHistory.Backward();
			else
				pChannel=m_App.ChannelHistory.Forward();
			if (pChannel!=nullptr) {
				m_App.Core.SelectChannel(pChannel->GetTunerName(),*pChannel);
			}
		}
		return;

	case CM_CHANNEL_PREVIOUS:
		if (m_App.RecentChannelList.NumChannels()>1) {
			const CTunerChannelInfo *pChannel=
				m_App.RecentChannelList.GetChannelInfo(1);
			if (pChannel!=nullptr) {
				m_App.Core.SelectChannel(pChannel->GetTunerName(),*pChannel);
			}
		}
		return;

#ifdef _DEBUG
	case CM_UPDATECHANNELLIST:
		// チャンネルリストの自動更新(現状役には立たない)
		//if (m_App.DriverOptions.IsChannelAutoUpdate(m_App.CoreEngine.GetDriverFileName()))
		{
			CTuningSpaceList TuningSpaceList(*m_App.ChannelManager.GetTuningSpaceList());
			std::vector<TVTest::String> MessageList;

			TRACE(TEXT("チャンネルリスト自動更新開始\n"));
			if (m_App.ChannelScan.AutoUpdateChannelList(&TuningSpaceList,&MessageList)) {
				m_App.AddLog(TEXT("チャンネルリストの自動更新を行いました。"));
				for (size_t i=0;i<MessageList.size();i++)
					m_App.AddLog(TEXT("%s"),MessageList[i].c_str());

				TuningSpaceList.MakeAllChannelList();
				m_App.Core.UpdateCurrentChannelList(&TuningSpaceList);

				TCHAR szFileName[MAX_PATH];
				if (!m_App.ChannelManager.GetChannelFileName(szFileName,lengthof(szFileName))
						|| ::lstrcmpi(::PathFindExtension(szFileName),CHANNEL_FILE_EXTENSION)!=0
						|| !::PathFileExists(szFileName)) {
					m_App.CoreEngine.GetDriverPath(szFileName,lengthof(szFileName));
					::PathRenameExtension(szFileName,CHANNEL_FILE_EXTENSION);
				}
				if (TuningSpaceList.SaveToFile(szFileName))
					m_App.AddLog(TEXT("チャンネルファイルを \"%s\" に保存しました。"),szFileName);
				else
					m_App.AddLog(CLogItem::TYPE_ERROR,TEXT("チャンネルファイル \"%s\" を保存できません。"),szFileName);
			}
		}
		return;
#endif

	case CM_MENU:
		{
			POINT pt;
			bool fDefault=false;

			if (codeNotify==COMMAND_FROM_MOUSE) {
				::GetCursorPos(&pt);
				if (::GetKeyState(VK_SHIFT)<0)
					fDefault=true;
			} else {
				pt.x=0;
				pt.y=0;
				::ClientToScreen(GetCurrentViewWindow().GetHandle(),&pt);
			}
			m_pCore->PopupMenu(&pt,fDefault?CUICore::POPUPMENU_DEFAULT:0);
		}
		return;

	case CM_ACTIVATE:
		{
			HWND hwndHost=GetVideoHostWindow();

			if (hwndHost!=nullptr)
				ForegroundWindow(hwndHost);
		}
		return;

	case CM_MINIMIZE:
		::ShowWindow(hwnd,::IsIconic(hwnd)?SW_RESTORE:SW_MINIMIZE);
		return;

	case CM_MAXIMIZE:
		::ShowWindow(hwnd,::IsZoomed(hwnd)?SW_RESTORE:SW_MAXIMIZE);
		return;

	case CM_1SEGMODE:
		m_App.Core.Set1SegMode(!m_App.Core.Is1SegMode(),true);
		return;

	case CM_CLOSETUNER:
		m_App.Core.ShutDownTuner();
		return;

	case CM_HOMEDISPLAY:
		if (!m_App.HomeDisplay.GetVisible()) {
			Util::CWaitCursor WaitCursor;

			m_App.HomeDisplay.SetFont(m_App.OSDOptions.GetDisplayFont(),
									  m_App.OSDOptions.IsDisplayFontAutoSize());
			if (!m_App.HomeDisplay.IsCreated()) {
				m_App.HomeDisplay.SetEventHandler(&m_Display.HomeDisplayEventHandler);
				m_App.HomeDisplay.Create(m_Display.GetDisplayViewParent(),
										 WS_CHILD | WS_CLIPCHILDREN);
				if (m_fCustomFrame)
					HookWindows(m_App.HomeDisplay.GetHandle());
			}
			m_App.HomeDisplay.UpdateContents();
			m_Display.GetDisplayBase().SetDisplayView(&m_App.HomeDisplay);
			m_Display.GetDisplayBase().SetVisible(true);
			m_App.HomeDisplay.Update();
		} else {
			m_Display.GetDisplayBase().SetVisible(false);
		}
		return;

	case CM_CHANNELDISPLAY:
		if (!m_App.ChannelDisplay.GetVisible()) {
			Util::CWaitCursor WaitCursor;

			m_App.ChannelDisplay.SetFont(m_App.OSDOptions.GetDisplayFont(),
										 m_App.OSDOptions.IsDisplayFontAutoSize());
			if (!m_App.ChannelDisplay.IsCreated()) {
				m_App.ChannelDisplay.SetEventHandler(&m_Display.ChannelDisplayEventHandler);
				m_App.ChannelDisplay.Create(
					m_Display.GetDisplayViewParent(),
					WS_CHILD | WS_CLIPCHILDREN);
				m_App.ChannelDisplay.SetDriverManager(&m_App.DriverManager);
				m_App.ChannelDisplay.SetLogoManager(&m_App.LogoManager);
				if (m_fCustomFrame)
					HookWindows(m_App.ChannelDisplay.GetHandle());
			}
			m_Display.GetDisplayBase().SetDisplayView(&m_App.ChannelDisplay);
			m_Display.GetDisplayBase().SetVisible(true);
			if (m_App.CoreEngine.IsDriverSpecified()) {
				m_App.ChannelDisplay.SetSelect(
					m_App.CoreEngine.GetDriverFileName(),
					m_App.ChannelManager.GetCurrentChannelInfo());
			}
			m_App.ChannelDisplay.Update();
		} else {
			m_Display.GetDisplayBase().SetVisible(false);
		}
		return;

	case CM_ENABLEBUFFERING:
		m_App.CoreEngine.SetPacketBufferPool(
			!m_App.CoreEngine.GetPacketBuffering(),
			m_App.PlaybackOptions.GetPacketBufferPoolPercentage());
		m_App.PlaybackOptions.SetPacketBuffering(m_App.CoreEngine.GetPacketBuffering());
		return;

	case CM_RESETBUFFER:
		m_App.CoreEngine.ResetPacketBuffer();
		return;

	case CM_RESETERRORCOUNT:
		m_App.CoreEngine.ResetErrorCount();
		m_App.StatusView.UpdateItem(STATUS_ITEM_ERROR);
		m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_ERROR);
		m_App.AppEventManager.OnStatisticsReset();
		return;

	case CM_SHOWRECORDREMAINTIME:
		{
			CRecordStatusItem *pItem=
				dynamic_cast<CRecordStatusItem*>(m_App.StatusView.GetItemByID(STATUS_ITEM_RECORD));

			if (pItem!=nullptr) {
				bool fRemain=!m_App.RecordOptions.GetShowRemainTime();
				m_App.RecordOptions.SetShowRemainTime(fRemain);
				pItem->ShowRemainTime(fRemain);
			}
		}
		return;

	case CM_SHOWTOTTIME:
		{
			const bool fTOT=!m_App.StatusOptions.GetShowTOTTime();
			m_App.StatusOptions.SetShowTOTTime(fTOT);

			CClockStatusItem *pItem=
				dynamic_cast<CClockStatusItem*>(m_App.StatusView.GetItemByID(STATUS_ITEM_CLOCK));
			if (pItem!=nullptr)
				pItem->SetTOT(fTOT);
		}
		return;

	case CM_PROGRAMINFOSTATUS_POPUPINFO:
		{
			const bool fEnable=!m_App.StatusOptions.IsPopupProgramInfoEnabled();
			m_App.StatusOptions.EnablePopupProgramInfo(fEnable);

			CProgramInfoStatusItem *pItem=
				dynamic_cast<CProgramInfoStatusItem*>(m_App.StatusView.GetItemByID(STATUS_ITEM_PROGRAMINFO));
			if (pItem!=nullptr)
				pItem->EnablePopupInfo(fEnable);
		}
		return;

	case CM_PROGRAMINFOSTATUS_SHOWPROGRESS:
		{
			CProgramInfoStatusItem *pItem=
				dynamic_cast<CProgramInfoStatusItem*>(m_App.StatusView.GetItemByID(STATUS_ITEM_PROGRAMINFO));
			if (pItem!=nullptr) {
				pItem->SetShowProgress(!pItem->GetShowProgress());
				m_App.StatusOptions.SetShowEventProgress(pItem->GetShowProgress());
			}
		}
		return;

	case CM_ADJUSTTOTTIME:
		m_App.TotTimeAdjuster.BeginAdjust();
		return;

	case CM_ZOOMMENU:
	case CM_ASPECTRATIOMENU:
	case CM_CHANNELMENU:
	case CM_SERVICEMENU:
	case CM_TUNINGSPACEMENU:
	case CM_FAVORITESMENU:
	case CM_RECENTCHANNELMENU:
	case CM_VOLUMEMENU:
	case CM_AUDIOMENU:
	case CM_VIDEOMENU:
	case CM_RESETMENU:
	case CM_BARMENU:
	case CM_PLUGINMENU:
	case CM_FILTERPROPERTYMENU:
		{
			int SubMenu=m_App.MenuOptions.GetSubMenuPosByCommand(id);
			POINT pt;

			if (codeNotify==COMMAND_FROM_MOUSE) {
				::GetCursorPos(&pt);
			} else {
				pt.x=0;
				pt.y=0;
				::ClientToScreen(GetCurrentViewWindow().GetHandle(),&pt);
			}
			m_App.MainMenu.PopupSubMenu(SubMenu,TPM_RIGHTBUTTON,hwnd,&pt);
		}
		return;

	case CM_SIDEBAR_PLACE_LEFT:
	case CM_SIDEBAR_PLACE_RIGHT:
	case CM_SIDEBAR_PLACE_TOP:
	case CM_SIDEBAR_PLACE_BOTTOM:
		{
			CSideBarOptions::PlaceType Place=(CSideBarOptions::PlaceType)(id-CM_SIDEBAR_PLACE_FIRST);

			if (Place!=m_App.SideBarOptions.GetPlace()) {
				bool fVertical=
					Place==CSideBarOptions::PLACE_LEFT || Place==CSideBarOptions::PLACE_RIGHT;
				int Pane=
					Place==CSideBarOptions::PLACE_LEFT || Place==CSideBarOptions::PLACE_TOP?0:1;

				m_App.SideBarOptions.SetPlace(Place);
				m_App.SideBar.SetVertical(fVertical);
				Layout::CSplitter *pSplitter=
					dynamic_cast<Layout::CSplitter*>(m_LayoutBase.GetContainerByID(CONTAINER_ID_SIDEBARSPLITTER));
				bool fSwap=pSplitter->IDToIndex(CONTAINER_ID_SIDEBAR)!=Pane;
				pSplitter->SetStyle(
					(fVertical?Layout::CSplitter::STYLE_HORZ:Layout::CSplitter::STYLE_VERT) |
					Layout::CSplitter::STYLE_FIXED,
					!fSwap);
				if (fSwap)
					pSplitter->SwapPane();
			}
		}
		return;

	case CM_SIDEBAROPTIONS:
		if (::IsWindowEnabled(hwnd))
			m_App.ShowOptionDialog(hwnd,COptionDialog::PAGE_SIDEBAR);
		return;

	case CM_DRIVER_BROWSE:
		{
			OPENFILENAME ofn;
			TCHAR szFileName[MAX_PATH],szInitDir[MAX_PATH];
			CFilePath FilePath;

			FilePath.SetPath(m_App.CoreEngine.GetDriverFileName());
			if (FilePath.GetDirectory(szInitDir)) {
				::lstrcpy(szFileName,FilePath.GetFileName());
			} else {
				m_App.GetAppDirectory(szInitDir);
				szFileName[0]='\0';
			}
			InitOpenFileName(&ofn);
			ofn.hwndOwner=GetVideoHostWindow();
			ofn.lpstrFilter=
				TEXT("BonDriver(BonDriver*.dll)\0BonDriver*.dll\0")
				TEXT("すべてのファイル\0*.*\0");
			ofn.lpstrFile=szFileName;
			ofn.nMaxFile=lengthof(szFileName);
			ofn.lpstrInitialDir=szInitDir;
			ofn.lpstrTitle=TEXT("BonDriverの選択");
			ofn.Flags=OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_EXPLORER;
			if (::GetOpenFileName(&ofn)) {
				m_App.Core.OpenTuner(szFileName);
			}
		}
		return;

	case CM_CHANNELHISTORY_CLEAR:
		m_App.RecentChannelList.Clear();
		m_App.TaskbarManager.ClearRecentChannelList();
		m_App.TaskbarManager.UpdateJumpList();
		return;

	case CM_CHANNELNO_2DIGIT:
	case CM_CHANNELNO_3DIGIT:
		{
			int Digits=id==CM_CHANNELNO_2DIGIT?2:3;

			if (m_ChannelInput.IsInputting()) {
				bool fCancel=Digits==m_ChannelInput.GetMaxDigits();
				EndChannelNoInput(false);
				if (fCancel)
					return;
			}
			BeginChannelNoInput(Digits);
		}
		return;

	case CM_ADDTOFAVORITES:
		{
			const CChannelInfo *pChannel=m_App.ChannelManager.GetCurrentChannelInfo();
			if (pChannel!=nullptr) {
				if (m_App.FavoritesManager.AddChannel(pChannel,m_App.CoreEngine.GetDriverFileName()))
					m_App.AppEventManager.OnFavoritesChanged();
			}
		}
		return;

	case CM_ORGANIZEFAVORITES:
		{
			COrganizeFavoritesDialog Dialog(&m_App.FavoritesManager);

			if (Dialog.Show(GetVideoHostWindow()))
				m_App.AppEventManager.OnFavoritesChanged();
		}
		return;

	case CM_SWITCHVIDEO:
		{
			CDtvEngine &DtvEngine=m_App.CoreEngine.m_DtvEngine;

			if (DtvEngine.m_TsAnalyzer.GetEventComponentGroupNum(DtvEngine.GetServiceIndex())>0)
				SendCommand(CM_MULTIVIEW_SWITCH);
			else
				SendCommand(CM_VIDEOSTREAM_SWITCH);
		}
		return;

	case CM_VIDEOSTREAM_SWITCH:
		{
			CDtvEngine &DtvEngine=m_App.CoreEngine.m_DtvEngine;
			const int StreamCount=DtvEngine.GetVideoStreamNum();

			if (StreamCount>1) {
				DtvEngine.SetVideoStream((DtvEngine.GetVideoStream()+1)%StreamCount);
			}
		}
		return;

	case CM_MULTIVIEW_SWITCH:
		{
			CDtvEngine &DtvEngine=m_App.CoreEngine.m_DtvEngine;
			const int GroupCount=DtvEngine.m_TsAnalyzer.GetEventComponentGroupNum(DtvEngine.GetServiceIndex());

			if (GroupCount>1) {
				SendCommand(CM_MULTIVIEW_FIRST+(DtvEngine.GetCurComponentGroup()+1)%GroupCount);
			}
		}
		return;

	default:
		if ((id>=CM_ZOOM_FIRST && id<=CM_ZOOM_LAST)
				|| (id>=CM_CUSTOMZOOM_FIRST && id<=CM_CUSTOMZOOM_LAST)) {
			CZoomOptions::ZoomInfo Info;

			if (m_pCore->GetFullscreen())
				m_pCore->SetFullscreen(false);
			if (::IsZoomed(hwnd))
				::ShowWindow(hwnd,SW_RESTORE);
			if (m_App.ZoomOptions.GetZoomInfoByCommand(id,&Info)) {
				if (Info.Type==CZoomOptions::ZOOM_RATE)
					SetZoomRate(Info.Rate.Rate,Info.Rate.Factor);
				else if (Info.Type==CZoomOptions::ZOOM_SIZE)
					AdjustWindowSize(Info.Size.Width,Info.Size.Height);
			}
			return;
		}

		if (id>=CM_CAPTURESIZE_FIRST && id<=CM_CAPTURESIZE_LAST) {
			int CaptureSize=id-CM_CAPTURESIZE_FIRST;

			m_App.CaptureOptions.SetPresetCaptureSize(CaptureSize);
			m_pCore->SetCommandRadioCheckedState(CM_CAPTURESIZE_FIRST,CM_CAPTURESIZE_LAST,id);
			return;
		}

		if (id>=CM_CHANNELNO_FIRST && id<=CM_CHANNELNO_LAST) {
			m_App.Core.SwitchChannelByNo((id-CM_CHANNELNO_FIRST)+1,true);
			return;
		}

		if (id>=CM_CHANNEL_FIRST && id<=CM_CHANNEL_LAST) {
			m_App.Core.SwitchChannel(id-CM_CHANNEL_FIRST);
			return;
		}

		if (id>=CM_SERVICE_FIRST && id<=CM_SERVICE_LAST) {
			if (m_App.RecordManager.IsRecording()) {
				if (!m_App.RecordOptions.ConfirmServiceChange(
						GetVideoHostWindow(),&m_App.RecordManager))
					return;
			}
			m_App.Core.SetServiceByIndex(id-CM_SERVICE_FIRST,CAppCore::SET_SERVICE_STRICT_ID);
			return;
		}

		if (id>=CM_SPACE_ALL && id<=CM_SPACE_LAST) {
			int Space=id-CM_SPACE_FIRST;

			if (Space!=m_App.ChannelManager.GetCurrentSpace()) {
				const CChannelList *pChannelList=m_App.ChannelManager.GetChannelList(Space);
				if (pChannelList!=nullptr) {
					for (int i=0;i<pChannelList->NumChannels();i++) {
						if (pChannelList->IsEnabled(i)) {
							m_App.Core.SetChannel(Space,i);
							return;
						}
					}
				}
			}
			return;
		}

		if (id>=CM_DRIVER_FIRST && id<=CM_DRIVER_LAST) {
			const CDriverInfo *pDriverInfo=m_App.DriverManager.GetDriverInfo(id-CM_DRIVER_FIRST);

			if (pDriverInfo!=nullptr) {
				if (!m_App.CoreEngine.IsTunerOpen()
						|| !IsEqualFileName(pDriverInfo->GetFileName(),m_App.CoreEngine.GetDriverFileName())) {
					if (m_App.Core.OpenTuner(pDriverInfo->GetFileName())) {
						m_App.Core.RestoreChannel();
					}
				}
			}
			return;
		}

		if (id>=CM_PLUGIN_FIRST && id<=CM_PLUGIN_LAST) {
			CPlugin *pPlugin=m_App.PluginManager.GetPlugin(m_App.PluginManager.FindPluginByCommand(id));

			if (pPlugin!=nullptr)
				pPlugin->Enable(!pPlugin->IsEnabled());
			return;
		}

		if (id>=CM_SPACE_CHANNEL_FIRST && id<=CM_SPACE_CHANNEL_LAST) {
			if (!m_pCore->ConfirmChannelChange())
				return;
			m_pCore->ProcessTunerMenu(id);
			return;
		}

		if (id>=CM_CHANNELHISTORY_FIRST && id<=CM_CHANNELHISTORY_LAST) {
			const CTunerChannelInfo *pChannel=
				m_App.RecentChannelList.GetChannelInfo(id-CM_CHANNELHISTORY_FIRST);

			if (pChannel!=nullptr)
				m_App.Core.SelectChannel(pChannel->GetTunerName(),*pChannel);
			return;
		}

		if (id>=CM_FAVORITECHANNEL_FIRST && id<=CM_FAVORITECHANNEL_LAST) {
			CFavoritesManager::ChannelInfo ChannelInfo;

			if (m_App.FavoritesManager.GetChannelByCommand(id,&ChannelInfo)) {
				m_App.Core.SelectChannel(
					ChannelInfo.BonDriverFileName.c_str(),
					ChannelInfo.Channel,
					ChannelInfo.fForceBonDriverChange?
						0 : CAppCore::SELECT_CHANNEL_USE_CUR_TUNER);
			}
			return;
		}

		if (id>=CM_PLUGINCOMMAND_FIRST && id<=CM_PLUGINCOMMAND_LAST) {
			m_App.PluginManager.OnPluginCommand(m_App.CommandList.GetCommandTextByID(id));
			return;
		}

		if (id>=CM_PANANDSCAN_PRESET_FIRST && id<=CM_PANANDSCAN_PRESET_LAST) {
			SetPanAndScan(id);
			return;
		}

		if (id>=CM_VIDEOSTREAM_FIRST && id<=CM_VIDEOSTREAM_LAST) {
			m_App.CoreEngine.m_DtvEngine.SetVideoStream(id-CM_VIDEOSTREAM_FIRST);
			return;
		}

		if (id>=CM_AUDIOSTREAM_FIRST && id<=CM_AUDIOSTREAM_LAST) {
			m_pCore->SetAudioStream(id-CM_AUDIOSTREAM_FIRST);
			ShowAudioOSD();
			return;
		}

		if (id>=CM_AUDIO_FIRST && id<=CM_AUDIO_LAST) {
			m_pCore->SelectAudio(id-CM_AUDIO_FIRST);
			ShowAudioOSD();
			return;
		}

		if (id>=CM_MULTIVIEW_FIRST && id<=CM_MULTIVIEW_LAST) {
			const WORD ServiceIndex=m_App.CoreEngine.m_DtvEngine.GetServiceIndex();
			CTsAnalyzer &TsAnalyzer=m_App.CoreEngine.m_DtvEngine.m_TsAnalyzer;
			CTsAnalyzer::EventComponentGroupInfo GroupInfo;

			if (ServiceIndex!=CDtvEngine::SERVICE_INVALID
					&& TsAnalyzer.GetEventComponentGroupInfo(ServiceIndex,id-CM_MULTIVIEW_FIRST,&GroupInfo)) {
				int VideoIndex=-1,AudioIndex=-1;

				for (int i=0;i<GroupInfo.CAUnitNum;i++) {
					for (int j=0;j<GroupInfo.CAUnit[i].ComponentNum;j++) {
						const BYTE ComponentTag=GroupInfo.CAUnit[i].ComponentTag[j];
						if (VideoIndex<0) {
							int Index=TsAnalyzer.GetVideoIndexByComponentTag(ServiceIndex,ComponentTag);
							if (Index>=0)
								VideoIndex=Index;
						}
						if (AudioIndex<0) {
							int Index=TsAnalyzer.GetAudioIndexByComponentTag(ServiceIndex,ComponentTag);
							if (Index>=0)
								AudioIndex=Index;
						}
						if (VideoIndex>=0 && AudioIndex>=0) {
							m_App.CoreEngine.m_DtvEngine.SetVideoStream(VideoIndex);
							m_pCore->SetAudioStream(AudioIndex);
							return;
						}
					}
				}
			}
			return;
		}

		if (id>=CM_PANEL_FIRST && id<=CM_PANEL_LAST) {
			m_App.Panel.Form.SetCurPageByID(id-CM_PANEL_FIRST);
			return;
		}
	}
}


bool CMainWindow::OnSysCommand(UINT Command)
{
	switch ((Command & 0xFFFFFFF0UL)) {
	case SC_MONITORPOWER:
		if (m_App.ViewOptions.GetNoMonitorLowPower()
				&& m_pCore->IsViewerEnabled())
			return true;
		break;

	case SC_SCREENSAVE:
		if (m_App.ViewOptions.GetNoScreenSaver()
				&& m_pCore->IsViewerEnabled())
			return true;
		break;

	case SC_ABOUT:
		{
			CAboutDialog AboutDialog;

			AboutDialog.Show(GetVideoHostWindow());
		}
		return true;

	case SC_MINIMIZE:
	case SC_MAXIMIZE:
	case SC_RESTORE:
		if (m_pCore->GetFullscreen())
			m_pCore->SetFullscreen(false);
		break;

	case SC_CLOSE:
		SendCommand(CM_CLOSE);
		return true;
	}

	return false;
}


void CMainWindow::OnTimer(HWND hwnd,UINT id)
{
	switch (id) {
	case TIMER_ID_UPDATE:
		// 情報更新
		{
			DWORD UpdateStatus=m_App.CoreEngine.UpdateAsyncStatus();
			DWORD UpdateStatistics=m_App.CoreEngine.UpdateStatistics();

			// 映像サイズの変化
			if ((UpdateStatus&CCoreEngine::STATUS_VIDEOSIZE)!=0) {
				m_App.StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
				m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_VIDEOINFO);
				m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VIDEO);
			}

			// 音声形式の変化
			if ((UpdateStatus&(CCoreEngine::STATUS_AUDIOCHANNELS
							 | CCoreEngine::STATUS_AUDIOSTREAMS
							 | CCoreEngine::STATUS_AUDIOCOMPONENTTYPE
							 | CCoreEngine::STATUS_SPDIFPASSTHROUGH))!=0) {
				TRACE(TEXT("Audio status changed.\n"));
				/*
				if ((UpdateStatus&CCoreEngine::STATUS_SPDIFPASSTHROUGH)==0)
					AutoSelectStereoMode();
				*/
				m_App.StatusView.UpdateItem(STATUS_ITEM_AUDIOCHANNEL);
				m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_AUDIO);
				m_pCore->SetCommandCheckedState(CM_SPDIF_TOGGLE,
					m_App.CoreEngine.IsSpdifPassthroughEnabled());
			}

			bool fUpdateEventInfo=false;

			if ((UpdateStatus & CCoreEngine::STATUS_EVENTID)!=0) {
				// 番組の切り替わり
				OnEventChanged();
			} else if ((UpdateStatus & CCoreEngine::STATUS_EVENTINFO)!=0) {
				// 番組情報が更新された
				fUpdateEventInfo=true;

				m_pCore->UpdateTitle();
				m_App.StatusView.UpdateItem(STATUS_ITEM_AUDIOCHANNEL);
			}

			CProgramInfoStatusItem *pProgramInfoStatusItem=
				dynamic_cast<CProgramInfoStatusItem*>(m_App.StatusView.GetItemByID(STATUS_ITEM_PROGRAMINFO));
			if (pProgramInfoStatusItem!=nullptr) {
				if (fUpdateEventInfo) {
					pProgramInfoStatusItem->Update();
				} else if (pProgramInfoStatusItem->GetShowProgress()
						&& (UpdateStatus & CCoreEngine::STATUS_TOT)!=0) {
					if (pProgramInfoStatusItem->UpdateProgress())
						pProgramInfoStatusItem->Redraw();
				}
			}

			if (m_App.RecordManager.IsRecording()) {
				if (m_App.RecordManager.QueryStop()) {
					m_App.Core.StopRecord();
				} else if (!m_App.RecordManager.IsPaused()) {
					m_App.StatusView.UpdateItem(STATUS_ITEM_RECORD);
				}
			} else {
				if (m_App.RecordManager.QueryStart())
					m_App.Core.StartReservedRecord();
			}

			if ((UpdateStatistics&(CCoreEngine::STATISTIC_ERRORPACKETCOUNT
								 | CCoreEngine::STATISTIC_CONTINUITYERRORPACKETCOUNT
								 | CCoreEngine::STATISTIC_SCRAMBLEPACKETCOUNT))!=0) {
				m_App.StatusView.UpdateItem(STATUS_ITEM_ERROR);
			}

			if ((UpdateStatistics&(CCoreEngine::STATISTIC_SIGNALLEVEL
								 | CCoreEngine::STATISTIC_BITRATE))!=0)
				m_App.StatusView.UpdateItem(STATUS_ITEM_SIGNALLEVEL);

			if ((UpdateStatistics&(CCoreEngine::STATISTIC_STREAMREMAIN
								 | CCoreEngine::STATISTIC_PACKETBUFFERRATE))!=0)
				m_App.StatusView.UpdateItem(STATUS_ITEM_BUFFERING);

			CClockStatusItem *pClockStatusItem=
				dynamic_cast<CClockStatusItem*>(m_App.StatusView.GetItemByID(STATUS_ITEM_CLOCK));
			if (pClockStatusItem!=nullptr
					&& (!pClockStatusItem->IsTOT() || (UpdateStatus & CCoreEngine::STATUS_TOT)!=0)) {
				pClockStatusItem->Update();
			}

			if ((UpdateStatus & CCoreEngine::STATUS_TOT)!=0)
				m_App.TotTimeAdjuster.AdjustTime();

			m_App.StatusView.UpdateItem(STATUS_ITEM_MEDIABITRATE);

			if (IsPanelVisible()) {
				// パネルの更新
				switch (m_App.Panel.Form.GetCurPageID()) {
				case PANEL_ID_INFORMATION:
					// 情報タブ更新
					m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_VIDEOINFO);

					if ((UpdateStatistics&(CCoreEngine::STATISTIC_SIGNALLEVEL
										 | CCoreEngine::STATISTIC_BITRATE))!=0) {
						m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_SIGNALLEVEL);
					}

					m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_MEDIABITRATE);

					if ((UpdateStatistics&(CCoreEngine::STATISTIC_ERRORPACKETCOUNT
										 | CCoreEngine::STATISTIC_CONTINUITYERRORPACKETCOUNT
										 | CCoreEngine::STATISTIC_SCRAMBLEPACKETCOUNT))!=0) {
						m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_ERROR);
					}

					if (m_App.RecordManager.IsRecording())
						m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_RECORD);

					if (fUpdateEventInfo) {
						m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_PROGRAMINFO);
						m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_AUDIO);
					}
					break;

				case PANEL_ID_CHANNEL:
					// チャンネルタブ更新
					if (!m_App.EpgOptions.IsEpgFileLoading()) {
						if (m_App.Panel.ChannelPanel.QueryUpdate()) {
							m_App.Panel.ChannelPanel.UpdateAllChannels(false);
						} else if (fUpdateEventInfo) {
							CAppCore::StreamIDInfo Info;
							if (m_App.Core.GetCurrentStreamIDInfo(&Info))
								m_App.Panel.ChannelPanel.UpdateChannels(Info.NetworkID,Info.TransportStreamID);
						}
					}
					if (m_App.Panel.ChannelPanel.QueryUpdateProgress())
						m_App.Panel.ChannelPanel.UpdateProgress();
					break;

				case PANEL_ID_PROGRAMLIST:
					if (fUpdateEventInfo)
						m_App.Panel.UpdateContent();
					break;
				}
			}

			// 空き容量が少ない場合の注意表示
			if (m_App.RecordOptions.GetAlertLowFreeSpace()
					&& !m_fAlertedLowFreeSpace
					&& m_App.RecordManager.IsRecording()) {
				LONGLONG FreeSpace=m_App.RecordManager.GetRecordTask()->GetFreeSpace();

				if (FreeSpace>=0
						&& (ULONGLONG)FreeSpace<=m_App.RecordOptions.GetLowFreeSpaceThresholdBytes()) {
					m_App.NotifyBalloonTip.Show(
						APP_NAME TEXT("の録画ファイルの保存先の空き容量が少なくなっています。"),
						TEXT("空き容量が少なくなっています。"),
						nullptr,CBalloonTip::ICON_WARNING);
					::SetTimer(m_hwnd,TIMER_ID_HIDETOOLTIP,10000,nullptr);
					ShowNotificationBar(
						TEXT("録画ファイルの保存先の空き容量が少なくなっています"),
						CNotificationBar::MESSAGE_WARNING,6000);
					m_fAlertedLowFreeSpace=true;
				}
			}

			m_App.PluginManager.SendStatusItemUpdateTimerEvent();
		}
		break;

	case TIMER_ID_OSD:
		// OSD を消す
		m_App.OSDManager.ClearOSD();
		::KillTimer(hwnd,TIMER_ID_OSD);
		break;

	case TIMER_ID_DISPLAY:
		// モニタがオフにならないようにする
		::SetThreadExecutionState(ES_DISPLAY_REQUIRED);
		break;

	case TIMER_ID_WHEELCHANNELCHANGE:
		// ホイールでのチャンネル変更
		{
			const int Channel=m_App.ChannelManager.GetChangingChannel();

			SetWheelChannelChanging(false);
			m_App.ChannelManager.SetChangingChannel(-1);
			if (Channel>=0)
				m_App.Core.SwitchChannel(Channel);
		}
		break;

	case TIMER_ID_PROGRAMLISTUPDATE:
		if (m_ProgramListUpdateTimerCount==0) {
			// サービスとロゴを関連付ける
			CTsAnalyzer *pAnalyzer=&m_App.CoreEngine.m_DtvEngine.m_TsAnalyzer;
			const WORD NetworkID=pAnalyzer->GetNetworkID();
			if (NetworkID!=0) {
				CTsAnalyzer::ServiceList ServiceList;
				if (pAnalyzer->GetServiceList(&ServiceList)) {
					for (size_t i=0;i<ServiceList.size();i++) {
						const CTsAnalyzer::ServiceInfo *pServiceInfo=&ServiceList[i];
						const WORD LogoID=pServiceInfo->LogoID;
						if (LogoID!=0xFFFF)
							m_App.LogoManager.AssociateLogoID(NetworkID,pServiceInfo->ServiceID,LogoID);
					}
				}
			}
		}

		// EPG情報の同期
		if (!m_App.EpgOptions.IsEpgFileLoading()
				&& !m_App.EpgOptions.IsEDCBDataLoading()) {
			m_App.Panel.EnableProgramListUpdate(true);

			CChannelInfo ChInfo;

			if (IsPanelVisible()
					&& m_App.Core.GetCurrentStreamChannelInfo(&ChInfo)) {
				if (m_App.Panel.Form.GetCurPageID()==PANEL_ID_PROGRAMLIST) {
					if (ChInfo.GetServiceID()!=0) {
						const HANDLE hThread=::GetCurrentThread();
						const int OldPriority=::GetThreadPriority(hThread);
						::SetThreadPriority(hThread,THREAD_PRIORITY_BELOW_NORMAL);

						m_App.EpgProgramList.UpdateService(
							ChInfo.GetNetworkID(),
							ChInfo.GetTransportStreamID(),
							ChInfo.GetServiceID());
						m_App.Panel.ProgramListPanel.UpdateProgramList(&ChInfo);

						::SetThreadPriority(hThread,OldPriority);
					}
				} else if (m_App.Panel.Form.GetCurPageID()==PANEL_ID_CHANNEL) {
					m_App.Panel.ChannelPanel.UpdateChannels(
						ChInfo.GetNetworkID(),
						ChInfo.GetTransportStreamID());
				}
			}

			m_ProgramListUpdateTimerCount++;
			// 更新頻度を下げる
			if (m_ProgramListUpdateTimerCount>=6 && m_ProgramListUpdateTimerCount<=10)
				::SetTimer(hwnd,TIMER_ID_PROGRAMLISTUPDATE,(m_ProgramListUpdateTimerCount-5)*(60*1000),nullptr);
		}
		break;

	case TIMER_ID_PROGRAMGUIDEUPDATE:
		// 番組表の取得
		if (!m_App.EpgCaptureManager.ProcessCapture())
			::SetTimer(m_hwnd,TIMER_ID_PROGRAMGUIDEUPDATE,3000,nullptr);
		break;

	case TIMER_ID_VIDEOSIZECHANGED:
		// 映像サイズの変化に合わせる

		if (m_App.ViewOptions.GetRemember1SegWindowSize()) {
			int Width,Height;

			if (m_App.CoreEngine.GetVideoViewSize(&Width,&Height)
					&& Width>0 && Height>0) {
				WindowSizeMode Mode=
					Height<=240 ? WINDOW_SIZE_1SEG : WINDOW_SIZE_HD;

				if (m_WindowSizeMode!=Mode) {
					RECT rc;
					GetPosition(&rc);
					if (m_WindowSizeMode==WINDOW_SIZE_1SEG)
						m_1SegWindowSize=rc;
					else
						m_HDWindowSize=rc;
					m_WindowSizeMode=Mode;
					const WindowSize *pSize;
					if (m_WindowSizeMode==WINDOW_SIZE_1SEG)
						pSize=&m_1SegWindowSize;
					else
						pSize=&m_HDWindowSize;
					if (pSize->IsValid())
						AdjustWindowSize(pSize->Width,pSize->Height,false);
				}
			}
		}

		m_Display.GetVideoContainer().SendSizeMessage();
		m_App.StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
		m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VIDEO);
		if (m_VideoSizeChangedTimerCount==3)
			::KillTimer(hwnd,TIMER_ID_VIDEOSIZECHANGED);
		else
			m_VideoSizeChangedTimerCount++;
		break;

	case TIMER_ID_RESETERRORCOUNT:
		// エラーカウントをリセットする
		// (既にサービスの情報が取得されている場合のみ)
		if (m_App.CoreEngine.m_DtvEngine.m_TsAnalyzer.GetServiceNum()>0) {
			SendCommand(CM_RESETERRORCOUNT);
			m_ResetErrorCountTimer.End();
		}
		break;

	case TIMER_ID_HIDETOOLTIP:
		// ツールチップを非表示にする
		m_App.NotifyBalloonTip.Hide();
		::KillTimer(hwnd,TIMER_ID_HIDETOOLTIP);
		break;

	case TIMER_ID_CHANNELNO:
		// チャンネル番号入力の時間切れ
		if (m_ChannelInput.IsInputting()
				&& !m_App.Accelerator.GetChannelInputOptions().fKeyTimeoutCancel) {
			EndChannelNoInput(true);
		} else {
			EndChannelNoInput(false);
		}
		return;

	case TIMER_ID_HIDECURSOR:
		if (m_App.OperationOptions.GetHideCursor()) {
			if (!m_fNoHideCursor && !m_Display.GetDisplayBase().IsVisible()) {
				POINT pt;
				RECT rc;
				::GetCursorPos(&pt);
				m_Display.GetViewWindow().GetScreenPosition(&rc);
				if (::PtInRect(&rc,pt)) {
					ShowCursor(false);
					::SetCursor(nullptr);
				}
			}
		}
		::KillTimer(hwnd,TIMER_ID_HIDECURSOR);
		break;
	}
}


bool CMainWindow::OnInitMenuPopup(HMENU hmenu)
{
	if (m_App.MainMenu.IsMainMenu(hmenu)) {
		bool fView=IsViewerEnabled();

		m_App.MainMenu.EnableItem(CM_COPY,fView);
		m_App.MainMenu.EnableItem(CM_SAVEIMAGE,fView);
	} else if (hmenu==m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_ZOOM)) {
		CZoomOptions::ZoomInfo Zoom;

		if (!GetZoomRate(&Zoom.Rate.Rate,&Zoom.Rate.Factor)) {
			Zoom.Rate.Rate=0;
			Zoom.Rate.Factor=0;
		}

		SIZE sz;
		if (m_Display.GetVideoContainer().GetClientSize(&sz)) {
			Zoom.Size.Width=sz.cx;
			Zoom.Size.Height=sz.cy;
		} else {
			Zoom.Size.Width=0;
			Zoom.Size.Height=0;
		}

		m_App.ZoomOptions.SetMenu(hmenu,&Zoom);
		m_App.Accelerator.SetMenuAccel(hmenu);
	} else if (hmenu==m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_SPACE)) {
		m_pCore->InitTunerMenu(hmenu);
	} else if (hmenu==m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_PLUGIN)) {
		m_App.PluginManager.SetMenu(hmenu);
		m_App.Accelerator.SetMenuAccel(hmenu);
	} else if (hmenu==m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_FAVORITES)) {
		//m_App.FavoritesManager.SetMenu(hmenu);
		m_App.FavoritesMenu.Create(&m_App.FavoritesManager.GetRootFolder(),
			CM_FAVORITECHANNEL_FIRST,hmenu,m_hwnd,
			CFavoritesMenu::FLAG_SHOWEVENTINFO | CFavoritesMenu::FLAG_SHOWLOGO);
		::EnableMenuItem(hmenu,CM_ADDTOFAVORITES,
			MF_BYCOMMAND | (m_App.ChannelManager.GetCurrentChannelInfo()!=nullptr?MF_ENABLED:MF_GRAYED));
	} else if (hmenu==m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_CHANNELHISTORY)) {
		m_App.RecentChannelList.SetMenu(hmenu);
	} else if (hmenu==m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_ASPECTRATIO)) {
		int ItemCount=::GetMenuItemCount(hmenu);

		if (ItemCount>m_DefaultAspectRatioMenuItemCount) {
			for (;ItemCount>m_DefaultAspectRatioMenuItemCount;ItemCount--) {
				::DeleteMenu(hmenu,ItemCount-3,MF_BYPOSITION);
			}
		}

		size_t PresetCount=m_App.PanAndScanOptions.GetPresetCount();
		if (PresetCount>0) {
			::InsertMenu(hmenu,ItemCount-2,MF_BYPOSITION | MF_SEPARATOR,0,nullptr);
			for (size_t i=0;i<PresetCount;i++) {
				CPanAndScanOptions::PanAndScanInfo Info;
				TCHAR szText[CPanAndScanOptions::MAX_NAME*2];

				m_App.PanAndScanOptions.GetPreset(i,&Info);
				CopyToMenuText(Info.szName,szText,lengthof(szText));
				::InsertMenu(hmenu,ItemCount-2+(UINT)i,
							 MF_BYPOSITION | MF_STRING | MF_ENABLED
							 | (m_AspectRatioType==ASPECTRATIO_CUSTOM+(int)i?MF_CHECKED:MF_UNCHECKED),
							 CM_PANANDSCAN_PRESET_FIRST+i,szText);
			}
		}

		m_App.AspectRatioIconMenu.CheckItem(CM_FRAMECUT,
			m_App.CoreEngine.m_DtvEngine.m_MediaViewer.GetViewStretchMode()==CMediaViewer::STRETCH_CUTFRAME);

		m_App.Accelerator.SetMenuAccel(hmenu);
		if (!m_App.AspectRatioIconMenu.OnInitMenuPopup(m_hwnd,hmenu))
			return false;
	} else if (hmenu==m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_CHANNEL)) {
		m_pCore->InitChannelMenu(hmenu);
	} else if (hmenu==m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_SERVICE)) {
		m_App.ChannelMenu.Destroy();
		ClearMenu(hmenu);

		CAppCore::StreamIDInfo StreamID;

		if (m_App.Core.GetCurrentStreamIDInfo(&StreamID)) {
			CTsAnalyzer::ServiceList ServiceList;
			CChannelList ChList;
			int CurService=-1;

			m_App.CoreEngine.m_DtvEngine.m_TsAnalyzer.GetViewableServiceList(&ServiceList);

			for (int i=0;i<static_cast<int>(ServiceList.size());i++) {
				const CTsAnalyzer::ServiceInfo &ServiceInfo=ServiceList[i];
				CChannelInfo *pChInfo=new CChannelInfo;

				pChInfo->SetChannelNo(i+1);
				if (ServiceInfo.szServiceName[0]!='\0') {
					pChInfo->SetName(ServiceInfo.szServiceName);
				} else {
					TCHAR szName[32];
					StdUtil::snprintf(szName,lengthof(szName),TEXT("サービス%d"),i+1);
					pChInfo->SetName(szName);
				}
				pChInfo->SetServiceID(ServiceInfo.ServiceID);
				pChInfo->SetNetworkID(StreamID.NetworkID);
				pChInfo->SetTransportStreamID(StreamID.TransportStreamID);
				ChList.AddChannel(pChInfo);
				if (ServiceInfo.ServiceID==StreamID.ServiceID)
					CurService=i;
			}

			m_App.ChannelMenu.Create(&ChList,CurService,CM_SERVICE_FIRST,hmenu,m_hwnd,
									 CChannelMenu::FLAG_SHOWLOGO |
									 CChannelMenu::FLAG_SHOWEVENTINFO |
									 CChannelMenu::FLAG_CURSERVICES);
		}
	} else if (hmenu==m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_AUDIO)) {
		CPopupMenu Menu(hmenu);
		Menu.Clear();

		CDtvEngine &DtvEngine=m_App.CoreEngine.m_DtvEngine;
		CAudioManager::AudioList AudioList;
		const CAudioDecFilter::DualMonoMode CurDualMonoMode=m_pCore->GetDualMonoMode();
		bool fDualMono=false;

		if (m_App.AudioManager.GetAudioList(&AudioList) && !AudioList.empty()) {
			auto GetAudioInfoText=[](
				const CAudioManager::AudioInfo &Info,int StreamNumber,
				LPTSTR pszText,int MaxText)
			{
				int Length;
				if (!Info.Text.empty()) {
					Length=CopyToMenuText(Info.Text.c_str(),pszText,MaxText);
				} else if (Info.Language!=0 && (!Info.IsDualMono() || Info.fMultiLingual)) {
					EpgUtil::GetLanguageText(Info.Language,pszText,MaxText);
					Length=::lstrlen(pszText);
					if (Info.DualMono==CAudioManager::DUALMONO_BOTH) {
						TCHAR szLang2[EpgUtil::MAX_LANGUAGE_TEXT_LENGTH];
						EpgUtil::GetLanguageText(Info.Language2,szLang2,lengthof(szLang2));
						Length+=StdUtil::snprintf(pszText+Length,MaxText-Length,TEXT("+%s"),szLang2);
					}
				} else if (Info.IsDualMono()) {
					Length=StdUtil::snprintf(pszText,MaxText,
						Info.DualMono==CAudioManager::DUALMONO_MAIN?
							TEXT("主音声"):
						Info.DualMono==CAudioManager::DUALMONO_SUB?
							TEXT("副音声"):
							TEXT("主+副音声"));
				} else {
					Length=StdUtil::snprintf(pszText,MaxText,TEXT("音声%d"),StreamNumber+1);
				}
				if (Info.ComponentType!=CAudioManager::COMPONENT_TYPE_INVALID) {
					LPCTSTR pszComponentType=EpgUtil::GetAudioComponentTypeText(Info.ComponentType);
					if (pszComponentType!=nullptr) {
						StdUtil::snprintf(pszText+Length,MaxText-Length,
										  TEXT(" [%s]"),pszComponentType);
					}
				}
			};

			const WORD ServiceIndex=DtvEngine.GetServiceIndex();
			const BYTE CurComponentTag=DtvEngine.GetAudioComponentTag();
			CTsAnalyzer::EventComponentGroupList GroupList;
			int Sel=-1;

			if (DtvEngine.m_TsAnalyzer.GetEventComponentGroupList(ServiceIndex,&GroupList)
					&& !GroupList.empty()) {
				// マルチビューTV
				const int NumGroup=static_cast<int>(GroupList.size());
				int SubGroupCount=0;

				// グループ毎の音声
				for (int i=0;i<NumGroup;i++) {
					const CTsAnalyzer::EventComponentGroupInfo &GroupInfo=GroupList[i];
					int StreamNumber=-1;

					for (int j=0;j<GroupInfo.CAUnitNum;j++) {
						for (int k=0;k<GroupInfo.CAUnit[j].ComponentNum;k++) {
							const BYTE ComponentTag=GroupInfo.CAUnit[j].ComponentTag[k];

							for (int AudioIndex=0;AudioIndex=static_cast<int>(AudioList.size());AudioIndex++) {
								CAudioManager::AudioInfo &AudioInfo=AudioList[AudioIndex];

								if (AudioInfo.ComponentTag==ComponentTag) {
									TCHAR szText[80];
									int Length;

									if (GroupInfo.szText[0]!=_T('\0')) {
										Length=CopyToMenuText(GroupInfo.szText,szText,lengthof(szText));
									} else {
										if (GroupInfo.ComponentGroupID==0)
											Length=StdUtil::snprintf(szText,lengthof(szText),TEXT("メイン"));
										else
											Length=StdUtil::snprintf(szText,lengthof(szText),TEXT("サブ%d"),SubGroupCount+1);
									}

									if (!AudioInfo.IsDualMono() || AudioInfo.DualMono==CAudioManager::DUALMONO_MAIN)
										StreamNumber++;

									TCHAR szAudio[64];
									GetAudioInfoText(AudioInfo,StreamNumber,szAudio,lengthof(szAudio));
									StdUtil::snprintf(szText+Length,lengthof(szText)-Length,
													  TEXT(": %s"),szAudio);
									Menu.Append(CM_AUDIO_FIRST+AudioIndex,szText);
									AudioInfo.ComponentTag=CAudioManager::COMPONENT_TAG_INVALID;

									if (AudioInfo.IsDualMono()) {
										if (CurDualMonoMode==CAudioDecFilter::DUALMONO_MAIN) {
											if (AudioInfo.DualMono==CAudioManager::DUALMONO_MAIN)
												Sel=AudioIndex;
										} else if (CurDualMonoMode==CAudioDecFilter::DUALMONO_SUB) {
											if (AudioInfo.DualMono==CAudioManager::DUALMONO_SUB)
												Sel=AudioIndex;
										} else {
											if (AudioInfo.DualMono==CAudioManager::DUALMONO_BOTH)
												Sel=AudioIndex;
										}
										fDualMono=true;
									} else {
										Sel=AudioIndex;
									}
								}
							}
						}
					}

					if (GroupInfo.ComponentGroupID!=0)
						SubGroupCount++;
				}
			}

			int StreamNumber=-1;

			for (int i=0;i<static_cast<int>(AudioList.size()) && i<=CM_AUDIO_LAST-CM_AUDIO_FIRST;i++) {
				const CAudioManager::AudioInfo &AudioInfo=AudioList[i];

				if (AudioInfo.ComponentTag==CAudioManager::COMPONENT_TAG_INVALID)
					continue;

				if (!AudioInfo.IsDualMono() || AudioInfo.DualMono==CAudioManager::DUALMONO_MAIN)
					StreamNumber++;

				TCHAR szText[80];
				int Length;

				Length=StdUtil::snprintf(szText,lengthof(szText),TEXT("%s%d: "),
										 i<9?TEXT("&"):TEXT(""),i+1);
				GetAudioInfoText(AudioInfo,StreamNumber,szText+Length,lengthof(szText)-Length);
				Menu.Append(CM_AUDIO_FIRST+i,szText);

				if (AudioInfo.ComponentTag==CurComponentTag) {
					if (AudioInfo.IsDualMono()) {
						if (CurDualMonoMode==CAudioDecFilter::DUALMONO_MAIN) {
							if (AudioInfo.DualMono==CAudioManager::DUALMONO_MAIN)
								Sel=i;
						} else if (CurDualMonoMode==CAudioDecFilter::DUALMONO_SUB) {
							if (AudioInfo.DualMono==CAudioManager::DUALMONO_SUB)
								Sel=i;
						} else {
							if (AudioInfo.DualMono==CAudioManager::DUALMONO_BOTH)
								Sel=i;
						}
						fDualMono=true;
					} else {
						Sel=i;
					}
				}
			}

			if (Sel>=0) {
				Menu.CheckRadioItem(CM_AUDIO_FIRST,
									CM_AUDIO_FIRST+static_cast<int>(AudioList.size())-1,
									CM_AUDIO_FIRST+Sel);
			}
		}

		HINSTANCE hinstRes=m_App.GetResourceInstance();

		if (!fDualMono) {
			const BYTE Channels=DtvEngine.GetAudioChannelNum();

			if (Channels==CMediaViewer::AUDIO_CHANNEL_DUALMONO) {
				if (Menu.GetItemCount()>0)
					Menu.AppendSeparator();
				static const int DualMonoMenuList[] = {
					CM_DUALMONO_MAIN,
					CM_DUALMONO_SUB,
					CM_DUALMONO_BOTH
				};
				for (int i=0;i<lengthof(DualMonoMenuList);i++) {
					TCHAR szText[64];
					::LoadString(hinstRes,DualMonoMenuList[i],szText,lengthof(szText));
					Menu.Append(DualMonoMenuList[i],szText);
				}
				Menu.CheckRadioItem(CM_DUALMONO_MAIN,CM_DUALMONO_BOTH,
					CurDualMonoMode==CAudioDecFilter::DUALMONO_MAIN?CM_DUALMONO_MAIN:
					CurDualMonoMode==CAudioDecFilter::DUALMONO_SUB?CM_DUALMONO_SUB:CM_DUALMONO_BOTH);
			} else if (Channels==2 && DtvEngine.GetAudioComponentType()==0) {
				if (Menu.GetItemCount()>0)
					Menu.AppendSeparator();
				static const int StereoModeMenuList[] = {
					CM_STEREOMODE_STEREO,
					CM_STEREOMODE_LEFT,
					CM_STEREOMODE_RIGHT
				};
				for (int i=0;i<lengthof(StereoModeMenuList);i++) {
					TCHAR szText[64];
					::LoadString(hinstRes,StereoModeMenuList[i],szText,lengthof(szText));
					Menu.Append(StereoModeMenuList[i],szText);
				}
				const CAudioDecFilter::StereoMode CurStereoMode=m_pCore->GetStereoMode();
				Menu.CheckRadioItem(CM_STEREOMODE_STEREO,CM_STEREOMODE_RIGHT,
					CurStereoMode==CAudioDecFilter::STEREOMODE_STEREO?
						CM_STEREOMODE_STEREO:
					CurStereoMode==CAudioDecFilter::STEREOMODE_LEFT?
						CM_STEREOMODE_LEFT:
						CM_STEREOMODE_RIGHT);
			}
		}

		if (Menu.GetItemCount()>0) {
			Menu.AppendSeparator();
		} else {
			Menu.Append(0U,TEXT("音声なし"),MF_GRAYED);
			Menu.AppendSeparator();
		}
		static const int AdditionalMenuList[] = {
			CM_SPDIF_DISABLED,
			CM_SPDIF_PASSTHROUGH,
			CM_SPDIF_AUTO,
			0,
			CM_AUDIODELAY_MINUS,
			CM_AUDIODELAY_PLUS,
			CM_AUDIODELAY_RESET,
		};
		for (int i=0;i<lengthof(AdditionalMenuList);i++) {
			if (AdditionalMenuList[i]!=0) {
				TCHAR szText[64];
				::LoadString(hinstRes,AdditionalMenuList[i],szText,lengthof(szText));
				Menu.Append(AdditionalMenuList[i],szText);
			} else {
				Menu.AppendSeparator();
			}
		}
		CAudioDecFilter::SpdifOptions SpdifOptions;
		m_App.CoreEngine.GetSpdifOptions(&SpdifOptions);
		Menu.CheckRadioItem(CM_SPDIF_DISABLED,CM_SPDIF_AUTO,
							CM_SPDIF_DISABLED+(int)SpdifOptions.Mode);
		m_App.Accelerator.SetMenuAccel(hmenu);
	} else if (hmenu==m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_VIDEO)) {
		CPopupMenu Menu(hmenu);
		Menu.Clear();

		CDtvEngine &DtvEngine=m_App.CoreEngine.m_DtvEngine;
		const WORD ServiceIndex=DtvEngine.GetServiceIndex();
		CTsAnalyzer::EventComponentGroupList GroupList;

		if (DtvEngine.m_TsAnalyzer.GetEventComponentGroupList(ServiceIndex,&GroupList)
				&& !GroupList.empty()) {
			// マルチビューTV
			const int NumGroup=static_cast<int>(GroupList.size());
			const BYTE CurVideoComponentTag=DtvEngine.GetVideoComponentTag();
			int CurGroup=-1;
			int SubGroupCount=0;

			for (int i=0;i<NumGroup;i++) {
				CTsAnalyzer::EventComponentGroupInfo &GroupInfo=GroupList[i];
				TCHAR szText[80];

				if (GroupInfo.szText[0]!=_T('\0')) {
					CopyToMenuText(GroupInfo.szText,szText,lengthof(szText));
				} else {
					if (GroupInfo.ComponentGroupID==0)
						StdUtil::strncpy(szText,lengthof(szText),TEXT("メイン"));
					else
						StdUtil::snprintf(szText,lengthof(szText),TEXT("サブ%d"),SubGroupCount+1);
					StdUtil::strncpy(GroupInfo.szText,lengthof(GroupInfo.szText),szText);
				}
				if (GroupInfo.ComponentGroupID!=0)
					SubGroupCount++;

				Menu.Append(CM_MULTIVIEW_FIRST+i,szText);

				if (CurGroup<0) {
					for (int j=0;j<GroupInfo.CAUnitNum;j++) {
						for (int k=0;k<GroupInfo.CAUnit[j].ComponentNum;k++) {
							if (GroupInfo.CAUnit[j].ComponentTag[k]==CurVideoComponentTag) {
								CurGroup=i;
								break;
							}
						}
					}
				}
			}

			if (CurGroup>=0) {
				Menu.CheckRadioItem(CM_MULTIVIEW_FIRST,
									CM_MULTIVIEW_FIRST+NumGroup-1,
									CM_MULTIVIEW_FIRST+CurGroup);
			}

			CTsAnalyzer::EsInfoList EsList;

			if (DtvEngine.m_TsAnalyzer.GetVideoEsList(ServiceIndex,&EsList)
					&& !EsList.empty()) {
				Menu.AppendSeparator();

				int CurVideoIndex=-1;

				// グループ毎の映像
				for (int i=0;i<NumGroup;i++) {
					const CTsAnalyzer::EventComponentGroupInfo &GroupInfo=GroupList[i];
					int VideoCount=0;

					for (int j=0;j<GroupInfo.CAUnitNum;j++) {
						for (int k=0;k<GroupInfo.CAUnit[j].ComponentNum;k++) {
							const BYTE ComponentTag=GroupInfo.CAUnit[j].ComponentTag[k];
							for (int EsIndex=0;EsIndex=static_cast<int>(EsList.size());EsIndex++) {
								if (EsList[EsIndex].ComponentTag==ComponentTag) {
									TCHAR szText[80];
									VideoCount++;
									int Length=CopyToMenuText(GroupInfo.szText,szText,lengthof(szText));
									StdUtil::snprintf(szText+Length,lengthof(szText)-Length,TEXT(": 映像%d"),VideoCount);
									Menu.Append(CM_VIDEOSTREAM_FIRST+EsIndex,szText);
									if (ComponentTag==CurVideoComponentTag)
										CurVideoIndex=EsIndex;
									EsList[EsIndex].ComponentTag=CTsAnalyzer::COMPONENTTAG_INVALID;
									break;
								}
							}
						}
					}
				}

				// グループに属さない映像
				int VideoCount=0;
				for (int i=0;i<static_cast<int>(EsList.size());i++) {
					if (EsList[i].ComponentTag!=CTsAnalyzer::COMPONENTTAG_INVALID) {
						TCHAR szText[64];
						VideoCount++;
						int Length=StdUtil::snprintf(szText,lengthof(szText),TEXT("映像%d"),VideoCount);
						if (EsList[i].QualityLevel==0)
							StdUtil::strncpy(szText+Length,lengthof(szText)-Length,TEXT(" (降雨対応放送)"));
						Menu.Append(CM_VIDEOSTREAM_FIRST+i,szText);
						if (EsList[i].ComponentTag==CurVideoComponentTag)
							CurVideoIndex=i;
					}
				}

				if (CurVideoIndex>=0) {
					Menu.CheckRadioItem(CM_VIDEOSTREAM_FIRST,
										CM_VIDEOSTREAM_FIRST+static_cast<int>(EsList.size())-1,
										CM_VIDEOSTREAM_FIRST+CurVideoIndex);
				}
			}
		} else {
			CTsAnalyzer::EsInfoList EsList;

			if (DtvEngine.m_TsAnalyzer.GetVideoEsList(ServiceIndex,&EsList)
					&& !EsList.empty()) {
				for (int i=0;i<static_cast<int>(EsList.size()) && CM_VIDEOSTREAM_FIRST+i<=CM_VIDEOSTREAM_LAST;i++) {
					TCHAR szText[64];
					int Length=StdUtil::snprintf(szText,lengthof(szText),TEXT("%s%d: 映像%d"),
												 i<9?TEXT("&"):TEXT(""),i+1,i+1);
					if (EsList[i].QualityLevel==0)
						StdUtil::strncpy(szText+Length,lengthof(szText)-Length,TEXT(" (降雨対応放送)"));
					Menu.Append(CM_VIDEOSTREAM_FIRST+i,szText);
				}

				Menu.CheckRadioItem(CM_VIDEOSTREAM_FIRST,
									CM_VIDEOSTREAM_FIRST+static_cast<int>(EsList.size())-1,
									CM_VIDEOSTREAM_FIRST+DtvEngine.GetVideoStream());
			} else {
				Menu.Append(0U,TEXT("映像なし"),MF_GRAYED);
			}
		}
	} else if (hmenu==m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_FILTERPROPERTY)) {
		for (int i=0;i<lengthof(m_DirectShowFilterPropertyList);i++) {
			m_App.MainMenu.EnableItem(m_DirectShowFilterPropertyList[i].Command,
				m_App.CoreEngine.m_DtvEngine.m_MediaViewer.FilterHasProperty(
					m_DirectShowFilterPropertyList[i].Filter));
		}
	} else if (hmenu==m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_BAR)) {
		m_App.MainMenu.CheckRadioItem(CM_WINDOWFRAME_NORMAL,CM_WINDOWFRAME_NONE,
			!m_fCustomFrame?CM_WINDOWFRAME_NORMAL:
			(m_CustomFrameWidth==0?CM_WINDOWFRAME_NONE:CM_WINDOWFRAME_CUSTOM));
		m_App.MainMenu.CheckItem(CM_CUSTOMTITLEBAR,m_fCustomTitleBar);
		m_App.MainMenu.EnableItem(CM_CUSTOMTITLEBAR,!m_fCustomFrame);
		m_App.MainMenu.CheckItem(CM_SPLITTITLEBAR,m_fSplitTitleBar);
		m_App.MainMenu.EnableItem(CM_SPLITTITLEBAR,m_fCustomFrame || m_fCustomTitleBar);
	} else {
		if (m_pCore->HandleInitMenuPopup(hmenu))
			return true;

		return false;
	}

	return true;
}


bool CMainWindow::OnClose(HWND hwnd)
{
	if (!ConfirmExit())
		return false;

	m_fClosing=true;

	::SetCursor(::LoadCursor(nullptr,IDC_WAIT));

	m_App.AddLog(TEXT("ウィンドウを閉じています..."));

	::KillTimer(hwnd,TIMER_ID_UPDATE);

	//m_App.CoreEngine.EnableMediaViewer(false);

	m_App.AppEventManager.OnClose();

	m_Fullscreen.Destroy();

	ShowFloatingWindows(false);

	return true;
}


void CMainWindow::OnTunerChanged()
{
	SetWheelChannelChanging(false);

	if (m_App.EpgCaptureManager.IsCapturing())
		m_App.EpgCaptureManager.EndCapture(0);

	m_App.Panel.ProgramListPanel.ClearProgramList();
	m_App.Panel.ProgramListPanel.ShowRetrievingMessage(false);
	m_App.Panel.InfoPanel.ResetItem(CInformationPanel::ITEM_SERVICE);
	m_App.Panel.InfoPanel.ResetItem(CInformationPanel::ITEM_PROGRAMINFO);

	bool fNoSignalLevel=m_App.DriverOptions.IsNoSignalLevel(m_App.CoreEngine.GetDriverFileName());
	m_App.Panel.InfoPanel.GetItem<CInformationPanel::CSignalLevelItem>()->ShowSignalLevel(!fNoSignalLevel);
	CSignalLevelStatusItem *pItem=dynamic_cast<CSignalLevelStatusItem*>(
		m_App.StatusView.GetItemByID(STATUS_ITEM_SIGNALLEVEL));
	if (pItem!=nullptr)
		pItem->ShowSignalLevel(!fNoSignalLevel);

	/*
	if (IsPanelVisible() && m_App.Panel.Form.GetCurPageID()==PANEL_ID_CHANNEL) {
		m_App.Panel.ChannelPanel.SetChannelList(
			m_App.ChannelManager.GetCurrentChannelList(),
			!m_App.EpgOptions.IsEpgFileLoading());
	} else {
		m_App.Panel.ChannelPanel.ClearChannelList();
	}
	*/
	m_App.Panel.CaptionPanel.Clear();
	m_App.Epg.ProgramGuide.ClearCurrentService();
	ClearMenu(m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_SERVICE));
	m_ResetErrorCountTimer.End();
	m_App.StatusView.UpdateItem(STATUS_ITEM_CHANNEL);
	m_App.StatusView.UpdateItem(STATUS_ITEM_TUNER);
	m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_CHANNEL);
	m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_TUNER);
	m_pCore->SetCommandRadioCheckedState(CM_CHANNELNO_1,CM_CHANNELNO_12,0);
	if (m_App.SideBarOptions.GetShowChannelLogo())
		m_App.SideBar.Invalidate();
	m_fForceResetPanAndScan=true;

	m_pCore->UpdateTitle();
	m_pCore->UpdateIcon();
}


void CMainWindow::OnTunerOpened()
{
	if (m_App.EpgCaptureManager.IsCapturing())
		m_App.EpgCaptureManager.EndCapture(0);

	m_pCore->UpdateTitle();
	m_pCore->UpdateIcon();
}


void CMainWindow::OnTunerClosed()
{
	m_pCore->UpdateTitle();
	m_pCore->UpdateIcon();
}


void CMainWindow::OnTunerShutDown()
{
	OnTunerChanged();
	m_App.Panel.ChannelPanel.ClearChannelList();
}


void CMainWindow::OnChannelListChanged()
{
	if (IsPanelVisible() && m_App.Panel.Form.GetCurPageID()==PANEL_ID_CHANNEL) {
		m_App.Panel.ChannelPanel.SetChannelList(m_App.ChannelManager.GetCurrentChannelList());
		m_App.Panel.ChannelPanel.SetCurrentChannel(m_App.ChannelManager.GetCurrentChannel());
	} else {
		m_App.Panel.ChannelPanel.ClearChannelList();
	}
	if (m_App.SideBarOptions.GetShowChannelLogo())
		m_App.SideBar.Invalidate();

	m_pCore->UpdateTitle();
	m_pCore->UpdateIcon();
}


void CMainWindow::OnChannelChanged(unsigned int Status)
{
	const bool fSpaceChanged=(Status & AppEvent::CHANNEL_CHANGED_STATUS_SPACE_CHANGED)!=0;
	const int CurSpace=m_App.ChannelManager.GetCurrentSpace();
	const CChannelInfo *pCurChannel=m_App.ChannelManager.GetCurrentChannelInfo();

	SetWheelChannelChanging(false);

	if (m_App.EpgCaptureManager.IsCapturing()
			&& !m_App.EpgCaptureManager.IsChannelChanging()
			&& (Status & AppEvent::CHANNEL_CHANGED_STATUS_DETECTED)==0)
		m_App.EpgCaptureManager.EndCapture(0);

	if (CurSpace>CChannelManager::SPACE_INVALID)
		m_App.MainMenu.CheckRadioItem(CM_SPACE_ALL,CM_SPACE_ALL+m_App.ChannelManager.NumSpaces(),
									  CM_SPACE_FIRST+CurSpace);
	ClearMenu(m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_SERVICE));
	m_App.StatusView.UpdateItem(STATUS_ITEM_CHANNEL);
	m_App.StatusView.UpdateItem(STATUS_ITEM_TUNER);
	m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_CHANNEL);
	m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_TUNER);
	if (pCurChannel!=nullptr && m_App.OSDOptions.IsOSDEnabled(COSDOptions::OSD_CHANNEL))
		ShowChannelOSD();
	const bool fEpgLoading=
		m_App.EpgOptions.IsEpgFileLoading() || m_App.EpgOptions.IsEDCBDataLoading();
	m_App.Panel.EnableProgramListUpdate(!fEpgLoading);
	::SetTimer(m_hwnd,TIMER_ID_PROGRAMLISTUPDATE,10000,nullptr);
	m_ProgramListUpdateTimerCount=0;
	m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_SERVICE);
	m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_PROGRAMINFO);
	m_App.Panel.ProgramListPanel.ShowRetrievingMessage(true);
	m_App.Panel.ProgramListPanel.SetCurrentChannel(pCurChannel);
	m_App.Panel.ProgramListPanel.SelectChannel(pCurChannel,!fEpgLoading);
	if (fSpaceChanged) {
		if (IsPanelVisible() && m_App.Panel.Form.GetCurPageID()==PANEL_ID_CHANNEL) {
			m_App.Panel.ChannelPanel.SetChannelList(
				m_App.ChannelManager.GetCurrentChannelList(),!fEpgLoading);
			m_App.Panel.ChannelPanel.SetCurrentChannel(
				m_App.ChannelManager.GetCurrentChannel());
		} else {
			m_App.Panel.ChannelPanel.ClearChannelList();
		}
	} else {
		if (IsPanelVisible() && m_App.Panel.Form.GetCurPageID()==PANEL_ID_CHANNEL)
			m_App.Panel.ChannelPanel.SetCurrentChannel(m_App.ChannelManager.GetCurrentChannel());
	}
	if (pCurChannel!=nullptr) {
		m_App.Epg.ProgramGuide.SetCurrentService(
			pCurChannel->GetNetworkID(),
			pCurChannel->GetTransportStreamID(),
			pCurChannel->GetServiceID());
	} else {
		m_App.Epg.ProgramGuide.ClearCurrentService();
	}
	int ChannelNo;
	if (pCurChannel!=nullptr)
		ChannelNo=pCurChannel->GetChannelNo();
	m_pCore->SetCommandRadioCheckedState(
		CM_CHANNELNO_1,CM_CHANNELNO_12,
		pCurChannel!=nullptr && ChannelNo>=1 && ChannelNo<=12?
			CM_CHANNELNO_1+ChannelNo-1:0);
	if (fSpaceChanged && m_App.SideBarOptions.GetShowChannelLogo())
		m_App.SideBar.Invalidate();
	m_App.Panel.CaptionPanel.Clear();
	m_App.Panel.UpdateControlPanel();

	LPCTSTR pszDriverFileName=m_App.CoreEngine.GetDriverFileName();
	pCurChannel=m_App.ChannelManager.GetCurrentChannelInfo();
	if (pCurChannel!=nullptr) {
		m_App.RecentChannelList.Add(pszDriverFileName,pCurChannel);
		m_App.ChannelHistory.SetCurrentChannel(pszDriverFileName,pCurChannel);
		m_App.TaskbarManager.AddRecentChannel(CTunerChannelInfo(*pCurChannel,pszDriverFileName));
		m_App.TaskbarManager.UpdateJumpList();
	}
	if (m_App.DriverOptions.IsResetChannelChangeErrorCount(pszDriverFileName))
		m_ResetErrorCountTimer.Begin(m_hwnd,5000);
	else
		m_ResetErrorCountTimer.End();
	m_fForceResetPanAndScan=true;

	m_pCore->UpdateTitle();
	m_pCore->UpdateIcon();
}


void CMainWindow::LockLayout()
{
	if (!::IsIconic(m_hwnd) && !::IsZoomed(m_hwnd)) {
		m_fLockLayout=true;
		m_LayoutBase.LockLayout();
	}
}


void CMainWindow::UpdateLayout()
{
	if (m_fLockLayout) {
		m_fLockLayout=false;

		SIZE sz;
		GetClientSize(&sz);
		OnSizeChanged(SIZE_RESTORED,sz.cx,sz.cy);
		m_LayoutBase.UnlockLayout();
	}
}


void CMainWindow::UpdateLayoutStructure()
{
	Layout::CSplitter *pSideBarSplitter,*pTitleSplitter,*pPanelSplitter,*pStatusSplitter;

	pSideBarSplitter=dynamic_cast<Layout::CSplitter*>(
		m_LayoutBase.GetContainerByID(CONTAINER_ID_SIDEBARSPLITTER));
	pTitleSplitter=dynamic_cast<Layout::CSplitter*>(
		m_LayoutBase.GetContainerByID(CONTAINER_ID_TITLEBARSPLITTER));
	pPanelSplitter=dynamic_cast<Layout::CSplitter*>(
		m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));
	pStatusSplitter=dynamic_cast<Layout::CSplitter*>(
		m_LayoutBase.GetContainerByID(CONTAINER_ID_STATUSSPLITTER));
	if (pSideBarSplitter==nullptr || pTitleSplitter==nullptr
			|| pPanelSplitter==nullptr || pStatusSplitter==nullptr)
		return;

	const int PanelPane=GetPanelPaneIndex();
	Layout::CSplitter *pRoot;

	pPanelSplitter->SetStyle(
		m_fPanelVerticalAlign ? Layout::CSplitter::STYLE_VERT : Layout::CSplitter::STYLE_HORZ,
		false);
	pPanelSplitter->ReplacePane(PanelPane,m_LayoutBase.GetContainerByID(CONTAINER_ID_PANEL));

	if (m_fPanelVerticalAlign) {
		pStatusSplitter->ReplacePane(0,pSideBarSplitter);
		pStatusSplitter->SetAdjustPane(CONTAINER_ID_SIDEBARSPLITTER);
		pPanelSplitter->ReplacePane(1-PanelPane,pStatusSplitter);
		pPanelSplitter->SetAdjustPane(CONTAINER_ID_STATUSSPLITTER);
		pTitleSplitter->ReplacePane(1,pPanelSplitter);
		pTitleSplitter->SetAdjustPane(CONTAINER_ID_PANELSPLITTER);
		pRoot=pTitleSplitter;
	} else if (m_fSplitTitleBar) {
		pTitleSplitter->ReplacePane(1,pSideBarSplitter);
		pTitleSplitter->SetAdjustPane(CONTAINER_ID_SIDEBARSPLITTER);
		pPanelSplitter->ReplacePane(1-PanelPane,pTitleSplitter);
		pPanelSplitter->SetAdjustPane(CONTAINER_ID_TITLEBARSPLITTER);
		pStatusSplitter->ReplacePane(0,pPanelSplitter);
		pStatusSplitter->SetAdjustPane(CONTAINER_ID_PANELSPLITTER);
		pRoot=pStatusSplitter;
	} else {
		pPanelSplitter->ReplacePane(1-PanelPane,pSideBarSplitter);
		pPanelSplitter->SetAdjustPane(CONTAINER_ID_SIDEBARSPLITTER);
		pTitleSplitter->ReplacePane(1,pPanelSplitter);
		pTitleSplitter->SetAdjustPane(CONTAINER_ID_PANELSPLITTER);
		pStatusSplitter->ReplacePane(0,pTitleSplitter);
		pStatusSplitter->SetAdjustPane(CONTAINER_ID_PANELSPLITTER);
		pRoot=pStatusSplitter;
	}

	m_LayoutBase.SetTopContainer(pRoot);
}


void CMainWindow::AdjustWindowSizeOnDockPanel(bool fDock)
{
	// パネルの幅に合わせてウィンドウサイズを拡縮
	RECT rc;

	GetPosition(&rc);
	GetPanelDockingAdjustedPos(fDock,&rc);
	SetPosition(&rc);
	if (!fDock)
		::SetFocus(m_hwnd);
}


void CMainWindow::GetPanelDockingAdjustedPos(bool fDock,RECT *pPos) const
{
	Layout::CSplitter *pSplitter=
		dynamic_cast<Layout::CSplitter*>(m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));
	int Width=m_App.Panel.Frame.GetDockingWidth()+pSplitter->GetBarWidth();
	int Height=m_App.Panel.Frame.GetDockingHeight()+pSplitter->GetBarWidth();

	if (!fDock) {
		Width=-Width;
		Height=-Height;
	}
	if (pSplitter->GetPane(0)->GetID()==CONTAINER_ID_PANEL) {
		if (m_App.Panel.Frame.IsDockingVertical())
			pPos->top-=Height;
		else
			pPos->left-=Width;
	} else {
		if (m_App.Panel.Frame.IsDockingVertical())
			pPos->bottom+=Height;
		else
			pPos->right+=Width;
	}

	if (fDock) {
		HMONITOR hMonitor=::MonitorFromRect(pPos,MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi;

		mi.cbSize=sizeof(MONITORINFO);
		if (::GetMonitorInfo(hMonitor,&mi)) {
			int XOffset=0,YOffset=0;
			POINT pt;

			pt.x=pPos->left;
			pt.y=pPos->top;
			if (::MonitorFromPoint(pt,MONITOR_DEFAULTTONULL)==nullptr) {
				if (pPos->left<mi.rcWork.left)
					XOffset=mi.rcWork.left-pPos->left;
				if (pPos->top<mi.rcWork.top)
					YOffset=mi.rcWork.top-pPos->top;
			}
			if (XOffset==0 || YOffset==0) {
				pt.x=pPos->right;
				pt.y=pPos->bottom;
				if (::MonitorFromPoint(pt,MONITOR_DEFAULTTONULL)==nullptr) {
					if (XOffset==0 && pPos->right>mi.rcWork.right) {
						XOffset=mi.rcWork.right-pPos->right;
						if (pPos->left+XOffset<mi.rcWork.left)
							XOffset=mi.rcWork.left-pPos->left;
					}
					if (YOffset==0 && pPos->bottom>mi.rcWork.bottom) {
						YOffset=mi.rcWork.bottom-pPos->bottom;
						if (pPos->top+YOffset<mi.rcWork.top)
							YOffset=mi.rcWork.top-pPos->top;
					}
				}
			}
			if (XOffset!=0 || YOffset!=0)
				::OffsetRect(pPos,XOffset,YOffset);
		}
	}
}


void CMainWindow::SetFixedPaneSize(int SplitterID,int ContainerID,int Width,int Height)
{
	Layout::CWindowContainer *pContainer=
		dynamic_cast<Layout::CWindowContainer*>(m_LayoutBase.GetContainerByID(ContainerID));

	if (pContainer!=nullptr) {
		pContainer->SetMinSize(Width,Height);

		Layout::CSplitter *pSplitter=
			dynamic_cast<Layout::CSplitter*>(m_LayoutBase.GetContainerByID(SplitterID));
		if (pSplitter!=nullptr) {
			pSplitter->SetPaneSize(ContainerID,
				(pSplitter->GetStyle() & Layout::CSplitter::STYLE_VERT)!=0 ? Height : Width);
		}
	}
}


void CMainWindow::ShowPopupStatusBar(bool fShow)
{
	if (fShow) {
		m_App.StatusView.SetOpacity(m_App.StatusOptions.GetPopupOpacity()*255/CStatusOptions::OPACITY_MAX);
		m_App.StatusView.SetVisible(true);
		::BringWindowToTop(m_App.StatusView.GetHandle());
	} else {
		m_App.StatusView.SetVisible(false);
		m_App.StatusView.SetOpacity(255);
	}
}


void CMainWindow::ShowPopupSideBar(bool fShow)
{
	if (fShow) {
		m_App.SideBar.SetOpacity(m_App.SideBarOptions.GetPopupOpacity()*255/CSideBarOptions::OPACITY_MAX);
		m_App.SideBar.SetVisible(true);
		::BringWindowToTop(m_App.SideBar.GetHandle());
	} else {
		m_App.SideBar.SetVisible(false);
		m_App.SideBar.SetOpacity(255);
	}
}


void CMainWindow::ShowCursor(bool fShow)
{
	m_App.CoreEngine.m_DtvEngine.m_MediaViewer.HideCursor(!fShow);
	m_Display.GetViewWindow().ShowCursor(fShow);
	m_fShowCursor=fShow;
}


void CMainWindow::ShowChannelOSD()
{
	if (GetVisible() && !::IsIconic(m_hwnd)) {
		const CChannelInfo *pInfo;

		if (m_fWheelChannelChanging)
			pInfo=m_App.ChannelManager.GetChangingChannelInfo();
		else
			pInfo=m_App.ChannelManager.GetCurrentChannelInfo();
		if (pInfo!=nullptr)
			m_App.OSDManager.ShowChannelOSD(pInfo,m_fWheelChannelChanging);
	}
}


void CMainWindow::OnServiceChanged()
{
	int CurService=0;
	WORD ServiceID;
	if (m_App.CoreEngine.m_DtvEngine.GetServiceID(&ServiceID))
		CurService=m_App.CoreEngine.m_DtvEngine.m_TsAnalyzer.GetViewableServiceIndexByID(ServiceID);
	m_App.MainMenu.CheckRadioItem(CM_SERVICE_FIRST,
		CM_SERVICE_FIRST+m_App.CoreEngine.m_DtvEngine.m_TsAnalyzer.GetViewableServiceNum()-1,
		CM_SERVICE_FIRST+CurService);

	m_App.StatusView.UpdateItem(STATUS_ITEM_CHANNEL);

	m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_SERVICE);
	if (m_App.Panel.Form.GetCurPageID()==PANEL_ID_INFORMATION)
		m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_PROGRAMINFO);

	OnAudioStreamChanged(m_pCore->GetAudioStream());

	m_pCore->UpdateTitle();
}


void CMainWindow::OnEventChanged()
{
	const WORD EventID=m_App.CoreEngine.m_DtvEngine.GetEventID();

	// 番組の最後まで録画
	if (m_App.RecordManager.GetStopOnEventEnd())
		m_App.Core.StopRecord();

	m_pCore->UpdateTitle();

	if (m_App.OSDOptions.IsNotifyEnabled(COSDOptions::NOTIFY_EVENTNAME)
			&& !m_App.Core.IsChannelScanning()) {
		TCHAR szEventName[256];

		if (m_App.CoreEngine.m_DtvEngine.GetEventName(szEventName,lengthof(szEventName))>0) {
			TCHAR szBarText[EpgUtil::MAX_EVENT_TIME_LENGTH+lengthof(szEventName)];
			int Length=0;
			SYSTEMTIME StartTime;
			DWORD Duration;

			if (m_App.CoreEngine.m_DtvEngine.GetEventTime(&StartTime,&Duration)) {
				Length=EpgUtil::FormatEventTime(StartTime,Duration,
												szBarText,EpgUtil::MAX_EVENT_TIME_LENGTH);
				if (Length>0)
					szBarText[Length++]=_T(' ');
			}
			::lstrcpy(szBarText+Length,szEventName);
			ShowNotificationBar(szBarText,CNotificationBar::MESSAGE_INFO,0,true);
		}
	}

	if (IsPanelVisible()
			&& m_App.Panel.Form.GetCurPageID()==PANEL_ID_INFORMATION)
		m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_PROGRAMINFO);

	m_App.StatusView.UpdateItem(STATUS_ITEM_PROGRAMINFO);

	m_App.Panel.ProgramListPanel.SetCurrentEventID(EventID);
	m_App.Epg.ProgramGuide.SetCurrentEvent(EventID);

	if (m_AspectRatioType!=ASPECTRATIO_DEFAULT
			&& (m_fForceResetPanAndScan
			|| (m_App.VideoOptions.GetResetPanScanEventChange()
				&& m_AspectRatioType<ASPECTRATIO_CUSTOM))) {
		m_App.CoreEngine.m_DtvEngine.m_MediaViewer.SetPanAndScan(0,0);
		if (!m_pCore->GetFullscreen()
				&& IsViewerEnabled()) {
			AutoFitWindowToVideo();
			// この時点でまだ新しい映像サイズが取得できない場合があるため、
			// WM_APP_VIDEOSIZECHANGED が来た時に調整するようにする
			m_AspectRatioResetTime=::GetTickCount();
		}
		m_AspectRatioType=ASPECTRATIO_DEFAULT;
		m_fForceResetPanAndScan=false;
		m_App.StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
		m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VIDEO);
		m_pCore->SetCommandRadioCheckedState(
			CM_ASPECTRATIO_FIRST,CM_ASPECTRATIO_3D_LAST,
			CM_ASPECTRATIO_DEFAULT);
	}

	m_App.StatusView.UpdateItem(STATUS_ITEM_AUDIOCHANNEL);
	m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_AUDIO);
}


void CMainWindow::OnRecordingStarted()
{
	if (m_App.EpgCaptureManager.IsCapturing())
		m_App.EpgCaptureManager.EndCapture(0);

	m_App.StatusView.UpdateItem(STATUS_ITEM_RECORD);
	m_App.StatusView.UpdateItem(STATUS_ITEM_ERROR);
	//m_App.MainMenu.EnableItem(CM_RECORDOPTION,false);
	//m_App.MainMenu.EnableItem(CM_RECORDSTOPTIME,true);
	m_App.TaskbarManager.SetRecordingStatus(true);

	m_ResetErrorCountTimer.End();
	m_fAlertedLowFreeSpace=false;
	if (m_App.OSDOptions.IsOSDEnabled(COSDOptions::OSD_RECORDING))
		m_App.OSDManager.ShowOSD(TEXT("●録画"));

	m_pCore->UpdateTitle();
}


void CMainWindow::OnRecordingStopped()
{
	m_App.StatusView.UpdateItem(STATUS_ITEM_RECORD);
	m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_RECORD);
	//m_App.MainMenu.EnableItem(CM_RECORDOPTION,true);
	//m_App.MainMenu.EnableItem(CM_RECORDSTOPTIME,false);
	m_App.TaskbarManager.SetRecordingStatus(false);
	m_App.RecordManager.SetStopOnEventEnd(false);
	if (m_App.OSDOptions.IsOSDEnabled(COSDOptions::OSD_RECORDING))
		m_App.OSDManager.ShowOSD(TEXT("■録画停止"));
	if (m_pCore->GetStandby())
		m_App.Core.CloseTuner();

	m_pCore->UpdateTitle();
}


void CMainWindow::OnRecordingPaused()
{
	OnRecordingStateChanged();
}


void CMainWindow::OnRecordingResumed()
{
	OnRecordingStateChanged();
}


void CMainWindow::OnRecordingStateChanged()
{
	m_App.StatusView.UpdateItem(STATUS_ITEM_RECORD);
	m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_RECORD);
}


void CMainWindow::On1SegModeChanged(bool f1SegMode)
{
	m_pCore->SetCommandCheckedState(CM_1SEGMODE,f1SegMode);
}


void CMainWindow::OnMouseWheel(WPARAM wParam,LPARAM lParam,bool fHorz)
{
	POINT pt;
	pt.x=GET_X_LPARAM(lParam);
	pt.y=GET_Y_LPARAM(lParam);

	if (m_Display.GetDisplayBase().IsVisible()) {
		CDisplayView *pDisplayView=m_Display.GetDisplayBase().GetDisplayView();

		if (pDisplayView!=nullptr) {
			RECT rc;

			m_Display.GetDisplayBase().GetParent()->GetScreenPosition(&rc);
			if (::PtInRect(&rc,pt)) {
				if (pDisplayView->OnMouseWheel(fHorz?WM_MOUSEHWHEEL:WM_MOUSEWHEEL,wParam,lParam))
					return;
			}
		}
	}

	int Command;

	if (fHorz) {
		Command=m_App.OperationOptions.GetWheelTiltCommand();
	} else {
		if ((wParam&MK_SHIFT)!=0)
			Command=m_App.OperationOptions.GetWheelShiftCommand();
		else if ((wParam&MK_CONTROL)!=0)
			Command=m_App.OperationOptions.GetWheelCtrlCommand();
		else
			Command=m_App.OperationOptions.GetWheelCommand();
	}

	if (m_App.OperationOptions.IsStatusBarWheelEnabled() && m_App.StatusView.GetVisible()) {
		RECT rc;

		m_App.StatusView.GetScreenPosition(&rc);
		if (::PtInRect(&rc,pt)) {
			int ItemID=m_App.StatusView.GetCurItem();

			if (ItemID>=0) {
				CStatusItem *pItem=m_App.StatusView.GetItemByID(ItemID);

				if (pItem!=NULL) {
					POINT ptItem=pt;
					RECT rcItem;

					::ScreenToClient(m_App.StatusView.GetHandle(),&ptItem);
					pItem->GetRect(&rcItem);
					ptItem.x-=rcItem.left;
					ptItem.y-=rcItem.top;

					int NewCommand=0;
					if (pItem->OnMouseWheel(ptItem.x,ptItem.y,
											fHorz,GET_WHEEL_DELTA_WPARAM(wParam),
											&NewCommand))
						Command=NewCommand;
				}
			}
		}
	}

	if (Command==0) {
		m_PrevWheelCommand=0;
		return;
	}

	int Delta=m_WheelHandler.OnMouseWheel(wParam,1);
	if (Delta==0)
		return;
	if (m_App.OperationOptions.IsWheelCommandReverse(Command))
		Delta=-Delta;
	const DWORD CurTime=::GetTickCount();
	bool fProcessed=false;

	if (Command!=m_PrevWheelCommand)
		m_WheelCount=0;
	else
		m_WheelCount++;

	switch (Command) {
	case CM_WHEEL_VOLUME:
		SendCommand(Delta>0?CM_VOLUME_UP:CM_VOLUME_DOWN);
		fProcessed=true;
		break;

	case CM_WHEEL_CHANNEL:
		{
			bool fUp;

			if (fHorz)
				fUp=Delta>0;
			else
				fUp=Delta<0;
			int Channel=m_App.ChannelManager.GetNextChannel(fUp);
			if (Channel>=0) {
				if (m_fWheelChannelChanging
						&& m_WheelCount<5
						&& TickTimeSpan(m_PrevWheelTime,CurTime)<(5UL-m_WheelCount)*100UL) {
					break;
				}
				SetWheelChannelChanging(true,m_App.OperationOptions.GetWheelChannelDelay());
				m_App.ChannelManager.SetChangingChannel(Channel);
				m_App.StatusView.UpdateItem(STATUS_ITEM_CHANNEL);
				if (m_App.OSDOptions.IsOSDEnabled(COSDOptions::OSD_CHANNEL))
					ShowChannelOSD();
			}
			fProcessed=true;
		}
		break;

	case CM_WHEEL_AUDIO:
		if (Command!=m_PrevWheelCommand || TickTimeSpan(m_PrevWheelTime,CurTime)>=300) {
			SendCommand(CM_SWITCHAUDIO);
			fProcessed=true;
		}
		break;

	case CM_WHEEL_ZOOM:
		if (Command!=m_PrevWheelCommand || TickTimeSpan(m_PrevWheelTime,CurTime)>=500) {
			if (!IsZoomed(m_hwnd) && !m_pCore->GetFullscreen()) {
				int ZoomNum,ZoomDenom;
				if (m_pCore->GetZoomRate(&ZoomNum,&ZoomDenom)) {
					int Step=m_App.OperationOptions.GetWheelZoomStep();
					if (Delta<0)
						Step=-Step;
					if (ZoomDenom>=100) {
						ZoomNum+=::MulDiv(Step,ZoomDenom,100);
					} else {
						ZoomNum=ZoomNum*100+Step*ZoomDenom;
						ZoomDenom*=100;
					}
					SetZoomRate(ZoomNum,ZoomDenom);
				}
			}
			fProcessed=true;
		}
		break;

	case CM_WHEEL_ASPECTRATIO:
		if (Command!=m_PrevWheelCommand || TickTimeSpan(m_PrevWheelTime,CurTime)>=300) {
			SendCommand(CM_ASPECTRATIO);
			fProcessed=true;
		}
		break;

	case CM_WHEEL_AUDIODELAY:
		SendCommand(Delta>0?CM_AUDIODELAY_MINUS:CM_AUDIODELAY_PLUS);
		fProcessed=true;
		break;
	}

	m_PrevWheelCommand=Command;
	if (fProcessed)
		m_PrevWheelTime=CurTime;
}


bool CMainWindow::EnableViewer(bool fEnable)
{
	CMediaViewer &MediaViewer=m_App.CoreEngine.m_DtvEngine.m_MediaViewer;

	if (fEnable) {
		bool fInit;

		if (!MediaViewer.IsOpen()) {
			fInit=true;
		} else {
			BYTE VideoStreamType=m_App.CoreEngine.m_DtvEngine.GetVideoStreamType();
			fInit=
				VideoStreamType!=STREAM_TYPE_UNINITIALIZED &&
				VideoStreamType!=MediaViewer.GetVideoStreamType();
		}

		if (fInit) {
			if (!InitializeViewer())
				return false;
		}
	}

	if (MediaViewer.IsOpen()) {
		if (!m_Display.EnableViewer(fEnable))
			return false;

		m_pCore->PreventDisplaySave(fEnable);
	}

	m_fEnablePlayback=fEnable;

	m_pCore->SetCommandCheckedState(CM_DISABLEVIEWER,!fEnable);

	return true;
}


bool CMainWindow::IsViewerEnabled() const
{
	return m_Display.IsViewerEnabled();
}


HWND CMainWindow::GetViewerWindow() const
{
	return m_Display.GetVideoContainer().GetHandle();
}


bool CMainWindow::ShowVolumeOSD()
{
	if (m_App.OSDOptions.IsOSDEnabled(COSDOptions::OSD_VOLUME)
			&& GetVisible() && !::IsIconic(m_hwnd)) {
		m_App.OSDManager.ShowVolumeOSD(m_pCore->GetVolume());
		return true;
	}
	return false;
}


void CMainWindow::OnVolumeChanged(int Volume)
{
	m_App.StatusView.UpdateItem(STATUS_ITEM_VOLUME);
	m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VOLUME);
	m_pCore->SetCommandCheckedState(CM_VOLUME_MUTE,false);
}


void CMainWindow::OnMuteChanged(bool fMute)
{
	m_App.StatusView.UpdateItem(STATUS_ITEM_VOLUME);
	m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VOLUME);
	m_pCore->SetCommandCheckedState(CM_VOLUME_MUTE,fMute);
}


void CMainWindow::OnDualMonoModeChanged(CAudioDecFilter::DualMonoMode Mode)
{
	m_App.StatusView.UpdateItem(STATUS_ITEM_AUDIOCHANNEL);
	m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_AUDIO);
}


void CMainWindow::OnStereoModeChanged(CAudioDecFilter::StereoMode Mode)
{
	m_App.StatusView.UpdateItem(STATUS_ITEM_AUDIOCHANNEL);
	m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_AUDIO);
}


void CMainWindow::OnAudioStreamChanged(int Stream)
{
	m_App.StatusView.UpdateItem(STATUS_ITEM_AUDIOCHANNEL);
	m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_AUDIO);
}


void CMainWindow::ShowAudioOSD()
{
	if (m_App.OSDOptions.IsOSDEnabled(COSDOptions::OSD_AUDIO)) {
		TCHAR szText[128];

		if (m_pCore->GetSelectedAudioText(szText,lengthof(szText)))
			m_App.OSDManager.ShowOSD(szText);
	}
}


bool CMainWindow::SetZoomRate(int Rate,int Factor)
{
	if (Rate<1 || Factor<1)
		return false;

	int Width,Height;

	if (m_App.CoreEngine.GetVideoViewSize(&Width,&Height) && Width>0 && Height>0) {
		int ZoomWidth,ZoomHeight;

		ZoomWidth=CalcZoomSize(Width,Rate,Factor);
		ZoomHeight=CalcZoomSize(Height,Rate,Factor);

		if (m_App.ViewOptions.GetZoomKeepAspectRatio()) {
			SIZE ScreenSize;

			m_Display.GetVideoContainer().GetClientSize(&ScreenSize);
			if (ScreenSize.cx>0 && ScreenSize.cy>0) {
				if ((double)ZoomWidth/(double)ScreenSize.cx<=(double)ZoomHeight/(double)ScreenSize.cy) {
					ZoomWidth=CalcZoomSize(ScreenSize.cx,ZoomHeight,ScreenSize.cy);
				} else {
					ZoomHeight=CalcZoomSize(ScreenSize.cy,ZoomWidth,ScreenSize.cx);
				}
			}
		}

		AdjustWindowSize(ZoomWidth,ZoomHeight);
	}

	return true;
}


bool CMainWindow::GetZoomRate(int *pRate,int *pFactor)
{
	bool fOK=false;
	int Width,Height;
	int Rate=0,Factor=1;

	if (m_App.CoreEngine.GetVideoViewSize(&Width,&Height) && Width>0 && Height>0) {
		/*
		SIZE sz;

		m_Display.GetVideoContainer().GetClientSize(&sz);
		Rate=sz.cy;
		Factor=Height;
		*/
		WORD DstWidth,DstHeight;
		if (m_App.CoreEngine.m_DtvEngine.m_MediaViewer.GetDestSize(&DstWidth,&DstHeight)) {
			Rate=DstHeight;
			Factor=Height;
		}
		fOK=true;
	}
	if (pRate)
		*pRate=Rate;
	if (pFactor)
		*pFactor=Factor;
	return fOK;
}


bool CMainWindow::AutoFitWindowToVideo()
{
	int Width,Height;
	SIZE sz;

	if (!m_App.CoreEngine.GetVideoViewSize(&Width,&Height)
			|| Width<=0 || Height<=0)
		return false;
	m_Display.GetVideoContainer().GetClientSize(&sz);
	Width=CalcZoomSize(Width,sz.cy,Height);
	if (sz.cx<Width)
		AdjustWindowSize(Width,sz.cy);

	return true;
}


void CMainWindow::OnPanAndScanChanged()
{
	if (!m_pCore->GetFullscreen()) {
		switch (m_App.ViewOptions.GetPanScanAdjustWindowMode()) {
		case CViewOptions::ADJUSTWINDOW_FIT:
			{
				int ZoomRate,ZoomFactor;
				int Width,Height;

				if (GetZoomRate(&ZoomRate,&ZoomFactor)
						&& m_App.CoreEngine.GetVideoViewSize(&Width,&Height)) {
					AdjustWindowSize(CalcZoomSize(Width,ZoomRate,ZoomFactor),
									 CalcZoomSize(Height,ZoomRate,ZoomFactor));
				} else {
					WORD DstWidth,DstHeight;

					if (m_App.CoreEngine.m_DtvEngine.m_MediaViewer.GetDestSize(&DstWidth,&DstHeight))
						AdjustWindowSize(DstWidth,DstHeight);
				}
			}
			break;

		case CViewOptions::ADJUSTWINDOW_WIDTH:
			{
				SIZE sz;
				int Width,Height;

				m_Display.GetVideoContainer().GetClientSize(&sz);
				if (m_App.CoreEngine.GetVideoViewSize(&Width,&Height))
					AdjustWindowSize(CalcZoomSize(Width,sz.cy,Height),sz.cy);
			}
			break;
		}
	}

	m_App.StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
	m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VIDEO);
}


bool CMainWindow::SetPanAndScan(int Command)
{
	CCoreEngine::PanAndScanInfo Info;
	int Type;

	if (Command>=CM_ASPECTRATIO_FIRST && Command<=CM_ASPECTRATIO_3D_LAST) {
		static const CCoreEngine::PanAndScanInfo PanAndScanList[] = {
			{0, 0,  0,  0,  0,  0,  0,  0},	// デフォルト
			{0, 0,  1,  1,  1,  1, 16,  9},	// 16:9
			{0, 3,  1, 18,  1, 24, 16,  9},	// 16:9 レターボックス
			{2, 3, 12, 18, 16, 24, 16,  9},	// 16:9 超額縁
			{2, 0, 12,  1, 16,  1,  4,  3},	// 4:3 サイドカット
			{0, 0,  1,  1,  1,  1,  4,  3},	// 4:3
			{0, 0,  1,  1,  1,  1, 32,  9},	// 32:9
			{0, 0,  1,  1,  2,  1, 16,  9},	// 16:9 左
			{1, 0,  1,  1,  2,  1, 16,  9},	// 16:9 右
		};

		Type=Command-CM_ASPECTRATIO_FIRST;
		Info=PanAndScanList[Type];
	} else if (Command>=CM_PANANDSCAN_PRESET_FIRST && Command<=CM_PANANDSCAN_PRESET_LAST) {
		CPanAndScanOptions::PanAndScanInfo PanScan;
		if (!m_App.PanAndScanOptions.GetPreset(Command-CM_PANANDSCAN_PRESET_FIRST,&PanScan))
			return false;
		Info=PanScan.Info;
		Type=ASPECTRATIO_CUSTOM+(Command-CM_PANANDSCAN_PRESET_FIRST);
	} else {
		return false;
	}

	m_pCore->SetPanAndScan(Info);

	m_AspectRatioType=Type;
	m_AspectRatioResetTime=0;

	m_pCore->SetCommandRadioCheckedState(
		CM_ASPECTRATIO_FIRST,CM_ASPECTRATIO_3D_LAST,
		m_AspectRatioType<ASPECTRATIO_CUSTOM?CM_ASPECTRATIO_FIRST+m_AspectRatioType:0);

	return true;
}


bool CMainWindow::SetLogo(HBITMAP hbm)
{
	return m_Display.GetViewWindow().SetLogo(hbm);
}


bool CMainWindow::ShowProgramGuide(bool fShow,unsigned int Flags,const ProgramGuideSpaceInfo *pSpaceInfo)
{
	if (m_App.Epg.fShowProgramGuide==fShow)
		return true;

	if (fShow) {
		const bool fOnScreen=
			(Flags & PROGRAMGUIDE_SHOW_POPUP)==0
			&& ((Flags & PROGRAMGUIDE_SHOW_ONSCREEN)!=0
				|| m_App.ProgramGuideOptions.GetOnScreen()
				|| (m_pCore->GetFullscreen() && ::GetSystemMetrics(SM_CMONITORS)==1));

		Util::CWaitCursor WaitCursor;

		if (fOnScreen) {
			m_App.Epg.ProgramGuideDisplay.Create(m_Display.GetDisplayViewParent(),
				WS_CHILD | WS_CLIPCHILDREN);
			m_Display.GetDisplayBase().SetDisplayView(&m_App.Epg.ProgramGuideDisplay);
			if (m_fCustomFrame)
				HookWindows(m_App.Epg.ProgramGuideDisplay.GetHandle());
		} else {
			m_App.Epg.ProgramGuideFrame.Create(nullptr,
				WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX |
					WS_THICKFRAME | WS_CLIPCHILDREN);
		}

		SYSTEMTIME stFirst,stLast;
		m_App.ProgramGuideOptions.GetTimeRange(&stFirst,&stLast);
		m_App.Epg.ProgramGuide.SetTimeRange(&stFirst,&stLast);
		m_App.Epg.ProgramGuide.SetViewDay(CProgramGuide::DAY_TODAY);

		if (fOnScreen) {
			m_Display.GetDisplayBase().SetVisible(true);
		} else {
			m_App.Epg.ProgramGuideFrame.Show();
			m_App.Epg.ProgramGuideFrame.Update();
		}

		if (m_App.EpgOptions.IsEpgFileLoading()
				|| m_App.EpgOptions.IsEDCBDataLoading()) {
			m_App.Epg.ProgramGuide.SetMessage(TEXT("EPGファイルの読み込み中..."));
			m_App.EpgOptions.WaitEpgFileLoad();
			m_App.EpgOptions.WaitEDCBDataLoad();
			m_App.Epg.ProgramGuide.SetMessage(nullptr);
		}

		CEpg::CChannelProviderManager *pChannelProviderManager=
			m_App.Epg.CreateChannelProviderManager(pSpaceInfo!=nullptr?pSpaceInfo->pszTuner:nullptr);
		int Provider=pChannelProviderManager->GetCurChannelProvider();
		int Space;
		if (Provider>=0) {
			CProgramGuideChannelProvider *pChannelProvider=
				pChannelProviderManager->GetChannelProvider(Provider);
			bool fGroupID=false;
			if (pSpaceInfo!=nullptr && pSpaceInfo->pszSpace!=nullptr) {
				if (StringIsDigit(pSpaceInfo->pszSpace)) {
					Space=::StrToInt(pSpaceInfo->pszSpace);
				} else {
					Space=pChannelProvider->ParseGroupID(pSpaceInfo->pszSpace);
					fGroupID=true;
				}
			} else {
				Space=m_App.ChannelManager.GetCurrentSpace();
			}
			if (Space<0) {
				Space=0;
			} else if (!fGroupID) {
				CProgramGuideBaseChannelProvider *pBaseChannelProvider=
					dynamic_cast<CProgramGuideBaseChannelProvider*>(pChannelProvider);
				if (pBaseChannelProvider!=nullptr) {
					if (pBaseChannelProvider->HasAllChannelGroup())
						Space++;
					if ((size_t)Space>=pBaseChannelProvider->GetGroupCount())
						Space=0;
				}
			}
		} else {
			Space=-1;
		}
		m_App.Epg.ProgramGuide.SetCurrentChannelProvider(Provider,Space);
		m_App.Epg.ProgramGuide.UpdateProgramGuide(true);
		if (m_App.ProgramGuideOptions.ScrollToCurChannel())
			m_App.Epg.ProgramGuide.ScrollToCurrentService();
	} else {
		if (m_App.Epg.ProgramGuideFrame.IsCreated()) {
			m_App.Epg.ProgramGuideFrame.Destroy();
		} else {
			m_Display.GetDisplayBase().SetVisible(false);
		}
	}

	m_App.Epg.fShowProgramGuide=fShow;

	m_pCore->SetCommandCheckedState(CM_PROGRAMGUIDE,m_App.Epg.fShowProgramGuide);

	return true;
}


LRESULT CALLBACK CMainWindow::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	CMainWindow *pThis;

	if (uMsg==WM_NCCREATE) {
		pThis=static_cast<CMainWindow*>(CBasicWindow::OnCreate(hwnd,lParam));
	} else {
		pThis=static_cast<CMainWindow*>(GetBasicWindow(hwnd));
	}

	if (pThis==nullptr) {
		return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
	}

	if (uMsg==WM_CREATE) {
		if (!pThis->OnCreate(reinterpret_cast<LPCREATESTRUCT>(lParam)))
			return -1;
		return 0;
	}

	LRESULT Result=0;
	if (pThis->m_App.PluginManager.OnMessage(hwnd,uMsg,wParam,lParam,&Result))
		return Result;

	if (uMsg==WM_DESTROY) {
		pThis->OnMessage(hwnd,uMsg,wParam,lParam);
		::PostQuitMessage(0);
		return 0;
	}

	return pThis->OnMessage(hwnd,uMsg,wParam,lParam);
}


void CMainWindow::HookWindows(HWND hwnd)
{
	if (hwnd!=nullptr) {
		HookChildWindow(hwnd);
		for (hwnd=::GetWindow(hwnd,GW_CHILD);hwnd!=nullptr;hwnd=::GetWindow(hwnd,GW_HWNDNEXT))
			HookWindows(hwnd);
	}
}


#define CHILD_PROP_THIS	APP_NAME TEXT("ChildThis")

void CMainWindow::HookChildWindow(HWND hwnd)
{
	if (hwnd==nullptr)
		return;

	if (m_atomChildOldWndProcProp==0) {
		m_atomChildOldWndProcProp=::GlobalAddAtom(APP_NAME TEXT("ChildOldWndProc"));
		if (m_atomChildOldWndProcProp==0)
			return;
	}

	if (::GetProp(hwnd,MAKEINTATOM(m_atomChildOldWndProcProp))==nullptr) {
#ifdef _DEBUG
		TCHAR szClass[256];
		::GetClassName(hwnd,szClass,lengthof(szClass));
		TRACE(TEXT("Hook window %p \"%s\"\n"),hwnd,szClass);
#endif
		WNDPROC pOldWndProc=SubclassWindow(hwnd,ChildHookProc);
		::SetProp(hwnd,MAKEINTATOM(m_atomChildOldWndProcProp),pOldWndProc);
		::SetProp(hwnd,CHILD_PROP_THIS,this);
	}
}


LRESULT CALLBACK CMainWindow::ChildHookProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	WNDPROC pOldWndProc=static_cast<WNDPROC>(::GetProp(hwnd,MAKEINTATOM(m_atomChildOldWndProcProp)));

	if (pOldWndProc==nullptr)
		return ::DefWindowProc(hwnd,uMsg,wParam,lParam);

	switch (uMsg) {
	case WM_NCHITTEST:
		{
			CMainWindow *pThis=static_cast<CMainWindow*>(::GetProp(hwnd,CHILD_PROP_THIS));

			if (pThis!=nullptr && pThis->m_fCustomFrame && !::IsZoomed(pThis->m_hwnd)
					&& ::GetAncestor(hwnd,GA_ROOT)==pThis->m_hwnd) {
				POINT pt={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
				RECT rc;

				pThis->GetScreenPosition(&rc);
				if (::PtInRect(&rc,pt)) {
					Style::Margins FrameWidth=pThis->m_Style.ResizingMargin;
					if (FrameWidth.Left<pThis->m_CustomFrameWidth)
						FrameWidth.Left=pThis->m_CustomFrameWidth;
					if (FrameWidth.Top<pThis->m_CustomFrameWidth)
						FrameWidth.Top=pThis->m_CustomFrameWidth;
					if (FrameWidth.Right<pThis->m_CustomFrameWidth)
						FrameWidth.Right=pThis->m_CustomFrameWidth;
					if (FrameWidth.Bottom<pThis->m_CustomFrameWidth)
						FrameWidth.Bottom=pThis->m_CustomFrameWidth;
					int CornerMarginLeft=FrameWidth.Left*2;
					int CornerMarginRight=FrameWidth.Right*2;
					RECT rcFrame=rc;
					Style::Subtract(&rcFrame,FrameWidth);
					int Code=HTNOWHERE;

					if (pt.y<rcFrame.top) {
						if (pt.x<rcFrame.left+CornerMarginLeft)
							Code=HTTOPLEFT;
						else if (pt.x>=rcFrame.right-CornerMarginRight)
							Code=HTTOPRIGHT;
						else
							Code=HTTOP;
					} else if (pt.y>=rcFrame.bottom) {
						if (pt.x<rcFrame.left+CornerMarginLeft)
							Code=HTBOTTOMLEFT;
						else if (pt.x>=rcFrame.right-CornerMarginRight)
							Code=HTBOTTOMRIGHT;
						else
							Code=HTBOTTOM;
					} else if (pt.x<rcFrame.left) {
						Code=HTLEFT;
					} else if (pt.x>=rcFrame.right) {
						Code=HTRIGHT;
					}
					if (Code!=HTNOWHERE) {
						return Code;
					}
				}
			}
		}
		break;

	case WM_NCLBUTTONDOWN:
		{
			CMainWindow *pThis=static_cast<CMainWindow*>(::GetProp(hwnd,CHILD_PROP_THIS));

			if (pThis!=nullptr && pThis->m_fCustomFrame && ::GetAncestor(hwnd,GA_ROOT)==pThis->m_hwnd) {
				BYTE Flag=0;

				switch (wParam) {
				case HTTOP:			Flag=WMSZ_TOP;			break;
				case HTTOPLEFT:		Flag=WMSZ_TOPLEFT;		break;
				case HTTOPRIGHT:	Flag=WMSZ_TOPRIGHT;		break;
				case HTLEFT:		Flag=WMSZ_LEFT;			break;
				case HTRIGHT:		Flag=WMSZ_RIGHT;		break;
				case HTBOTTOM:		Flag=WMSZ_BOTTOM;		break;
				case HTBOTTOMLEFT:	Flag=WMSZ_BOTTOMLEFT;	break;
				case HTBOTTOMRIGHT:	Flag=WMSZ_BOTTOMRIGHT;	break;
				}

				if (Flag!=0) {
					::SendMessage(pThis->m_hwnd,WM_SYSCOMMAND,SC_SIZE | Flag,lParam);
					return 0;
				}
			}
		}
		break;

	case WM_DESTROY:
#ifdef _DEBUG
		{
			TCHAR szClass[256];
			::GetClassName(hwnd,szClass,lengthof(szClass));
			TRACE(TEXT("Unhook window %p \"%s\"\n"),hwnd,szClass);
		}
#endif
		::SetWindowLongPtr(hwnd,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(pOldWndProc));
		::RemoveProp(hwnd,MAKEINTATOM(m_atomChildOldWndProcProp));
		::RemoveProp(hwnd,CHILD_PROP_THIS);
		break;
	}

	return ::CallWindowProc(pOldWndProc,hwnd,uMsg,wParam,lParam);
}


void CMainWindow::SetWindowVisible()
{
	bool fRestore=false,fShow=false;

	if ((m_App.TaskTrayManager.GetStatus() & CTaskTrayManager::STATUS_MINIMIZED)!=0) {
		m_App.TaskTrayManager.SetStatus(0,CTaskTrayManager::STATUS_MINIMIZED);
		m_App.TaskTrayManager.SetMinimizeToTray(m_App.ViewOptions.GetMinimizeToTray());
		fRestore=true;
	}
	if (!GetVisible()) {
		SetVisible(true);
		ForegroundWindow(m_hwnd);
		Update();
		fShow=true;
	}
	if (::IsIconic(m_hwnd)) {
		::ShowWindow(m_hwnd,SW_RESTORE);
		Update();
		fRestore=true;
	} else if (!fShow) {
		ForegroundWindow(m_hwnd);
	}
	if (m_fMinimizeInit) {
		// 最小化状態での起動後最初の表示
		ShowFloatingWindows(true);
		m_fMinimizeInit=false;
	}
	if (fRestore) {
		ResumeViewer(ResumeInfo::VIEWERSUSPEND_MINIMIZE);
	}
}


void CMainWindow::ShowFloatingWindows(bool fShow,bool fNoProgramGuide)
{
	if (m_App.Panel.fShowPanelWindow && m_App.Panel.IsFloating()) {
		m_App.Panel.Frame.SetPanelVisible(fShow);
		if (fShow)
			m_App.Panel.Frame.Update();
	}
	if (m_App.Epg.fShowProgramGuide && !fNoProgramGuide)
		m_App.Epg.ProgramGuideFrame.SetVisible(fShow);
	if (m_App.CaptureWindow.IsCreated())
		m_App.CaptureWindow.SetVisible(fShow);
	if (m_App.StreamInfo.IsCreated())
		m_App.StreamInfo.SetVisible(fShow);
}


void CMainWindow::DrawCustomFrame(bool fActive)
{
	const TVTest::Theme::BackgroundStyle &Style=
		fActive?m_Theme.ActiveFrameStyle:m_Theme.FrameStyle;
	HDC hdc=::GetWindowDC(m_hwnd);
	RECT rc,rcEmpty;

	::GetWindowRect(m_hwnd,&rc);
	::OffsetRect(&rc,-rc.left,-rc.top);
	rcEmpty=rc;
	::InflateRect(&rcEmpty,-m_CustomFrameWidth,-m_CustomFrameWidth);
	TVTest::Theme::Draw(hdc,&rc,Style.Border);
	DrawUtil::FillBorder(hdc,&rc,&rcEmpty,&rc,Style.Fill.GetSolidColor());
	::ReleaseDC(m_hwnd,hdc);
}


CViewWindow &CMainWindow::GetCurrentViewWindow()
{
	if (m_pCore->GetFullscreen())
		return m_Fullscreen.GetViewWindow();
	return m_Display.GetViewWindow();
}


bool CMainWindow::SetStandby(bool fStandby)
{
	if (fStandby) {
		if (m_fStandbyInit)
			return true;
		m_App.AddLog(TEXT("待機状態に移行します。"));
		SuspendViewer(ResumeInfo::VIEWERSUSPEND_STANDBY);
		//FinalizeViewer();
		m_Resume.fFullscreen=m_pCore->GetFullscreen();
		if (m_Resume.fFullscreen)
			m_pCore->SetFullscreen(false);
		ShowFloatingWindows(false,m_App.GeneralOptions.GetStandaloneProgramGuide());
		SetVisible(false);

		if (!m_App.EpgCaptureManager.IsCapturing()) {
			StoreTunerResumeInfo();

			if (m_App.EpgOptions.GetUpdateWhenStandby()
					&& m_App.CoreEngine.IsTunerOpen()
					&& !m_App.RecordManager.IsRecording()
					&& !m_App.CoreEngine.IsNetworkDriver()
					&& !m_App.CmdLineOptions.m_fNoEpg) {
				m_App.EpgCaptureManager.BeginCapture(
					nullptr,nullptr,
					CEpgCaptureManager::BEGIN_STANDBY | CEpgCaptureManager::BEGIN_NO_UI);
			}

			if (!m_App.RecordManager.IsRecording()
					&& !m_App.EpgCaptureManager.IsCapturing())
				m_App.Core.CloseTuner();
		}
	} else {
		m_App.AddLog(TEXT("待機状態から復帰します。"));
		SetWindowVisible();
		Util::CWaitCursor WaitCursor;
		if (m_fStandbyInit) {
			bool fSetChannel=m_Resume.fSetChannel;
			m_Resume.fSetChannel=false;
			ResumeTuner();
			m_Resume.fSetChannel=fSetChannel;
			m_App.Core.InitializeChannel();
			InitializeViewer();
			m_fStandbyInit=false;
		}
		if (m_Resume.fFullscreen)
			m_pCore->SetFullscreen(true);
		ShowFloatingWindows(true);
		ForegroundWindow(m_hwnd);
		ResumeTuner();
		ResumeViewer(ResumeInfo::VIEWERSUSPEND_STANDBY);
	}

	return true;
}


bool CMainWindow::EnablePlayback(bool fEnable)
{
	m_fEnablePlayback=fEnable;

	m_pCore->SetCommandCheckedState(CM_DISABLEVIEWER,!m_fEnablePlayback);

	return true;
}


bool CMainWindow::InitStandby()
{
	if (!m_App.CmdLineOptions.m_fNoDirectShow && !m_App.CmdLineOptions.m_fNoView
			&& (!m_App.PlaybackOptions.GetRestorePlayStatus() || m_App.GetEnablePlaybackOnStart())) {
		m_Resume.fEnableViewer=true;
		m_Resume.ViewerSuspendFlags=ResumeInfo::VIEWERSUSPEND_STANDBY;
	}
	m_Resume.fFullscreen=m_App.CmdLineOptions.m_fFullscreen;

	if (m_App.CoreEngine.IsDriverSpecified())
		m_Resume.fOpenTuner=true;

	if (m_App.RestoreChannelInfo.Space>=0 && m_App.RestoreChannelInfo.Channel>=0) {
		int Space=m_App.RestoreChannelInfo.fAllChannels?CChannelManager::SPACE_ALL:m_App.RestoreChannelInfo.Space;
		const CChannelList *pList=m_App.ChannelManager.GetChannelList(Space);
		if (pList!=nullptr) {
			int Index=pList->FindByIndex(m_App.RestoreChannelInfo.Space,
										 m_App.RestoreChannelInfo.Channel,
										 m_App.RestoreChannelInfo.ServiceID);
			if (Index>=0) {
				m_Resume.Channel.SetSpace(Space);
				m_Resume.Channel.SetChannel(Index);
				m_Resume.Channel.SetServiceID(m_App.RestoreChannelInfo.ServiceID);
				m_Resume.fSetChannel=true;
			}
		}
	}

	m_fStandbyInit=true;
	m_pCore->SetStandby(true,true);

	return true;
}


bool CMainWindow::InitMinimize()
{
	if (!m_App.CmdLineOptions.m_fNoDirectShow && !m_App.CmdLineOptions.m_fNoView
			&& (!m_App.PlaybackOptions.GetRestorePlayStatus() || m_App.GetEnablePlaybackOnStart())) {
		m_Resume.fEnableViewer=true;
		m_Resume.ViewerSuspendFlags=ResumeInfo::VIEWERSUSPEND_MINIMIZE;
	}

	m_App.TaskTrayManager.SetStatus(CTaskTrayManager::STATUS_MINIMIZED,
									CTaskTrayManager::STATUS_MINIMIZED);
	if (!m_App.TaskTrayManager.GetMinimizeToTray())
		::ShowWindow(m_hwnd,SW_SHOWMINNOACTIVE);

	m_fMinimizeInit=true;

	return true;
}


bool CMainWindow::IsMinimizeToTray() const
{
	return m_App.TaskTrayManager.GetMinimizeToTray()
		&& (m_App.TaskTrayManager.GetStatus() & CTaskTrayManager::STATUS_MINIMIZED)!=0;
}


void CMainWindow::StoreTunerResumeInfo()
{
	m_Resume.Channel.Store(&m_App.ChannelManager);
	m_Resume.fSetChannel=m_Resume.Channel.IsValid();
	m_Resume.fOpenTuner=m_App.CoreEngine.IsTunerOpen();
}


bool CMainWindow::ResumeTuner()
{
	if (m_App.EpgCaptureManager.IsCapturing())
		m_App.EpgCaptureManager.EndCapture(0);

	if (m_Resume.fOpenTuner) {
		m_Resume.fOpenTuner=false;
		if (!m_App.Core.OpenTuner()) {
			m_Resume.fSetChannel=false;
			return false;
		}
	}

	ResumeChannel();

	return true;
}


void CMainWindow::ResumeChannel()
{
	if (m_Resume.fSetChannel) {
		if (m_App.CoreEngine.IsTunerOpen()
				&& !m_App.RecordManager.IsRecording()) {
			m_App.Core.SetChannel(m_Resume.Channel.GetSpace(),
								  m_Resume.Channel.GetChannel(),
								  m_Resume.Channel.GetServiceID());
		}
		m_Resume.fSetChannel=false;
	}
}


void CMainWindow::SuspendViewer(unsigned int Flags)
{
	if (IsViewerEnabled()) {
		TRACE(TEXT("Suspend viewer\n"));
		m_pCore->EnableViewer(false);
		m_Resume.fEnableViewer=true;
	}
	m_Resume.ViewerSuspendFlags|=Flags;
}


void CMainWindow::ResumeViewer(unsigned int Flags)
{
	if ((m_Resume.ViewerSuspendFlags & Flags)!=0) {
		m_Resume.ViewerSuspendFlags&=~Flags;
		if (m_Resume.ViewerSuspendFlags==0) {
			if (m_Resume.fEnableViewer) {
				TRACE(TEXT("Resume viewer\n"));
				m_pCore->EnableViewer(true);
				m_Resume.fEnableViewer=false;
			}
		}
	}
}


bool CMainWindow::ConfirmExit()
{
	return m_App.RecordOptions.ConfirmExit(GetVideoHostWindow(),&m_App.RecordManager);
}


bool CMainWindow::IsNoAcceleratorMessage(const MSG *pMsg)
{
	if (pMsg->message==WM_KEYDOWN || pMsg->message==WM_KEYUP) {
		if (m_ChannelInput.IsKeyNeeded(pMsg->wParam))
			return true;
	}

	return false;
}


bool CMainWindow::BeginChannelNoInput(int Digits)
{
	if (m_ChannelInput.IsInputting())
		EndChannelNoInput(false);

	if (Digits<0 || Digits>5)
		return false;

	TRACE(TEXT("チャンネル番号%d桁入力開始\n"),Digits);

	m_ChannelInput.BeginInput(Digits);

	OnChannelNoInput();

	return true;
}


void CMainWindow::EndChannelNoInput(bool fDetermine)
{
	if (m_ChannelInput.IsInputting()) {
		TRACE(TEXT("チャンネル番号入力終了\n"));

		int Number=0;
		if (fDetermine)
			Number=m_ChannelInput.GetNumber();

		m_ChannelInput.EndInput();
		m_ChannelNoInputTimer.End();
		m_App.OSDManager.ClearOSD();

		if (Number>0) {
#if 0
			const CChannelList *pChannelList=m_App.ChannelManager.GetCurrentChannelList();
			if (pChannelList!=nullptr) {
				int Index=pChannelList->FindChannelNo(Number);
				if (Index<0 && Number<=0xFFFF)
					Index=pChannelList->FindServiceID((WORD)Number);
				if (Index>=0)
					SendCommand(CM_CHANNEL_FIRST+Index);
			}
#else
			m_App.Core.SwitchChannelByNo(Number,true);
#endif
		}
	}
}


void CMainWindow::OnChannelNoInput()
{
	if (m_App.OSDOptions.IsOSDEnabled(COSDOptions::OSD_CHANNELNOINPUT)) {
		TCHAR szText[16];
		int MaxDigits=m_ChannelInput.GetMaxDigits();
		int Number=m_ChannelInput.GetNumber();
		if (MaxDigits>0) {
			for (int i=MaxDigits-1;i>=0;i--) {
				if (i<m_ChannelInput.GetCurDigits()) {
					szText[i]=Number%10+_T('0');
					Number/=10;
				} else {
					szText[i]=_T('-');
				}
			}
			szText[MaxDigits]=_T('\0');
		} else {
			StdUtil::snprintf(szText,lengthof(szText),TEXT("%d"),Number);
		}
		m_App.OSDManager.ShowOSD(szText,COSDManager::SHOW_NO_FADE);
	}

	const DWORD Timeout=m_App.Accelerator.GetChannelInputOptions().KeyTimeout;
	if (Timeout>0)
		m_ChannelNoInputTimer.Begin(m_hwnd,Timeout);
}


void CMainWindow::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);
}


void CMainWindow::NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	m_Style.NormalizeStyle(pStyleManager);
}


void CMainWindow::SetTheme(const TVTest::Theme::CThemeManager *pThemeManager)
{
	pThemeManager->GetBackgroundStyle(TVTest::Theme::CThemeManager::STYLE_WINDOW_FRAME,&m_Theme.FrameStyle);
	pThemeManager->GetBackgroundStyle(TVTest::Theme::CThemeManager::STYLE_WINDOW_ACTIVEFRAME,&m_Theme.ActiveFrameStyle);
	if (m_fCustomFrame)
		Redraw(NULL,RDW_FRAME | RDW_INVALIDATE);

	m_LayoutBase.SetBackColor(pThemeManager->GetColor(CColorScheme::COLOR_SPLITTER));

	Theme::BorderStyle Border;
	pThemeManager->GetBorderStyle(TVTest::Theme::CThemeManager::STYLE_SCREEN,&Border);
	m_Display.GetViewWindow().SetBorder(Border);

	m_TitleBar.SetTheme(pThemeManager);
	m_NotificationBar.SetTheme(pThemeManager);
	m_Fullscreen.SetTheme(pThemeManager);
}


bool CMainWindow::GetOSDClientInfo(COSDManager::OSDClientInfo *pInfo)
{
	if (!GetVisible() || ::IsIconic(m_hwnd))
		return false;

	if (m_Display.GetVideoContainer().GetVisible()) {
		pInfo->hwndParent=m_Display.GetVideoContainer().GetHandle();
	} else {
		pInfo->hwndParent=m_Display.GetVideoContainer().GetParent();
		pInfo->fForcePseudoOSD=true;
	}

	// まだ再生が開始されていない場合、アニメーションを無効にする
	// アニメーションの途中で DirectShow の初期化処理が入り、中途半端な表示になるため
	if (m_fEnablePlayback && !IsViewerEnabled())
		pInfo->fAnimation=false;

	RECT rc;
	::GetClientRect(pInfo->hwndParent,&rc);
	rc.top+=m_NotificationBar.GetBarHeight();
	if (!m_fShowStatusBar && m_App.StatusOptions.GetShowPopup())
		rc.bottom-=m_App.StatusView.GetHeight();
	pInfo->ClientRect=rc;

	return true;
}


bool CMainWindow::SetOSDHideTimer(DWORD Delay)
{
	return ::SetTimer(m_hwnd,TIMER_ID_OSD,Delay,nullptr)!=0;
}


CStatusView &CMainWindow::GetStatusView()
{
	return m_App.StatusView;
}


CSideBar &CMainWindow::GetSideBar()
{
	return m_App.SideBar;
}




bool CMainWindow::CFullscreen::Initialize(HINSTANCE hinst)
{
	WNDCLASS wc;

	wc.style=CS_DBLCLKS;
	wc.lpfnWndProc=WndProc;
	wc.cbClsExtra=0;
	wc.cbWndExtra=0;
	wc.hInstance=hinst;
	wc.hIcon=nullptr;
	wc.hCursor=nullptr;
	wc.hbrBackground=::CreateSolidBrush(RGB(0,0,0));
	wc.lpszMenuName=nullptr;
	wc.lpszClassName=FULLSCREEN_WINDOW_CLASS;
	return ::RegisterClass(&wc)!=0;
}


CMainWindow::CFullscreen::CFullscreen(CMainWindow &MainWindow)
	: m_MainWindow(MainWindow)
	, m_App(MainWindow.m_App)
	, m_pDisplay(nullptr)
	, m_TitleBarManager(&MainWindow,false)
	, m_PanelEventHandler(*this)
	, m_PanelWidth(-1)
	, m_PanelHeight(-1)
	, m_PanelPlace(CPanelFrame::DOCKING_NONE)
{
}


CMainWindow::CFullscreen::~CFullscreen()
{
	Destroy();
}


LRESULT CMainWindow::CFullscreen::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		return OnCreate()?0:-1;

	case WM_SIZE:
		m_LayoutBase.SetPosition(0,0,LOWORD(lParam),HIWORD(lParam));
		return 0;

	case WM_RBUTTONUP:
		OnRButtonUp();
		return 0;

	case WM_MBUTTONUP:
		OnMButtonUp();
		return 0;

	case WM_LBUTTONDBLCLK:
		OnLButtonDoubleClick();
		return 0;

	case WM_MOUSEMOVE:
		OnMouseMove();
		return 0;

	case WM_TIMER:
		if (wParam==TIMER_ID_HIDECURSOR) {
			if (!m_fMenu && !m_pDisplay->GetDisplayBase().IsVisible()) {
				POINT pt;
				RECT rc;
				::GetCursorPos(&pt);
				m_ViewWindow.GetScreenPosition(&rc);
				if (::PtInRect(&rc,pt)) {
					ShowCursor(false);
					::SetCursor(nullptr);
				}
			}
			::KillTimer(hwnd,TIMER_ID_HIDECURSOR);
		}
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT) {
			HWND hwndCursor=reinterpret_cast<HWND>(wParam);

			if (hwndCursor==hwnd
					|| hwndCursor==m_pDisplay->GetVideoContainer().GetHandle()
					|| hwndCursor==m_ViewWindow.GetHandle()
					|| CPseudoOSD::IsPseudoOSD(hwndCursor)) {
				::SetCursor(m_fShowCursor?::LoadCursor(nullptr,IDC_ARROW):nullptr);
				return TRUE;
			}
		}
		break;

	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		{
			bool fHorz=uMsg==WM_MOUSEHWHEEL;

			m_MainWindow.OnMouseWheel(wParam,lParam,fHorz);
			return fHorz;
		}

#if 0
	case WM_WINDOWPOSCHANGING:
		{
			WINDOWPOS *pwp=reinterpret_cast<WINDOWPOS*>(lParam);

			pwp->hwndInsertAfter=HWND_TOPMOST;
		}
		return 0;
#endif

	case WM_SYSKEYDOWN:
		if (wParam!=VK_F10)
			break;
	case WM_KEYDOWN:
		if (wParam==VK_ESCAPE) {
			m_App.UICore.SetFullscreen(false);
			return 0;
		}
	case WM_COMMAND:
		return m_MainWindow.SendMessage(uMsg,wParam,lParam);

	case WM_SYSCOMMAND:
		switch (wParam&0xFFFFFFF0) {
		case SC_MONITORPOWER:
			if (m_App.ViewOptions.GetNoMonitorLowPower()
					&& m_App.UICore.IsViewerEnabled())
				return 0;
			break;

		case SC_SCREENSAVE:
			if (m_App.ViewOptions.GetNoScreenSaver()
					&& m_App.UICore.IsViewerEnabled())
				return 0;
			break;
		}
		break;

	case WM_APPCOMMAND:
		{
			int Command=m_App.Accelerator.TranslateAppCommand(wParam,lParam);

			if (Command!=0) {
				m_MainWindow.SendCommand(Command);
				return TRUE;
			}
		}
		break;

	case WM_SETFOCUS:
		if (m_pDisplay->GetDisplayBase().IsVisible())
			m_pDisplay->GetDisplayBase().SetFocus();
		return 0;

	case WM_SETTEXT:
		m_TitleBar.SetLabel(reinterpret_cast<LPCTSTR>(lParam));
		break;

	case WM_SETICON:
		if (wParam==ICON_SMALL)
			m_TitleBar.SetIcon(reinterpret_cast<HICON>(lParam));
		break;

	case WM_DESTROY:
		m_pDisplay->GetVideoContainer().SetParent(&m_pDisplay->GetViewWindow());
		m_pDisplay->GetViewWindow().SendSizeMessage();
		delete m_LayoutBase.GetTopContainer();
		m_LayoutBase.SetTopContainer(nullptr);
		ShowCursor(true);
		m_pDisplay->GetDisplayBase().AdjustPosition();
		m_TitleBar.Destroy();
		m_App.OSDManager.Reset();
		ShowStatusView(false);
		ShowSideBar(false);
		ShowPanel(false);
		RestorePanel();
		return 0;
	}

	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


bool CMainWindow::CFullscreen::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 FULLSCREEN_WINDOW_CLASS,nullptr,m_App.GetInstance());
}


bool CMainWindow::CFullscreen::Create(HWND hwndOwner,CMainDisplay *pDisplay)
{
	m_ScreenMargin=m_MainWindow.m_Style.FullscreenMargin;

	HMONITOR hMonitor;
	int x,y,Width,Height;

	hMonitor=::MonitorFromWindow(m_MainWindow.GetHandle(),MONITOR_DEFAULTTONEAREST);
	if (hMonitor!=nullptr) {
		MONITORINFOEX mi;

		mi.cbSize=sizeof(mi);
		::GetMonitorInfo(hMonitor,&mi);
		x=mi.rcMonitor.left;
		y=mi.rcMonitor.top;
		Width=mi.rcMonitor.right-mi.rcMonitor.left;
		Height=mi.rcMonitor.bottom-mi.rcMonitor.top;

		DISPLAY_DEVICE dd;
		dd.cb=sizeof(dd);
		int MonitorNo=1;
		for (DWORD i=0;::EnumDisplayDevices(0,i,&dd,0);i++) {
			if ((dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)==0) {
				if (::lstrcmpi(dd.DeviceName,mi.szDevice)==0) {
					const TVTest::Style::CStyleManager *pStyleManager=GetStyleManager();
					TCHAR szKey[64];
					StdUtil::snprintf(szKey,lengthof(szKey),TEXT("fullscreen.monitor%d.margin"),MonitorNo);
					TVTest::Style::Margins Margin;
					if (pStyleManager->Get(szKey,&Margin)) {
						pStyleManager->ToPixels(&Margin);
						m_ScreenMargin=Margin;
					}
					break;
				}
				MonitorNo++;
			}
		}
	} else {
		x=y=0;
		Width=::GetSystemMetrics(SM_CXSCREEN);
		Height=::GetSystemMetrics(SM_CYSCREEN);
	}
#ifdef _DEBUG
	// デバッグし易いように小さく表示
	if (::GetKeyState(VK_SHIFT)<0) {
		Width/=2;
		Height/=2;
	}
#endif
	SetPosition(x,y,Width,Height);

	m_pDisplay=pDisplay;

	return Create(hwndOwner,WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN,WS_EX_TOPMOST);
}


bool CMainWindow::CFullscreen::OnCreate()
{
	m_LayoutBase.Create(m_hwnd,WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

	m_ViewWindow.SetMargin(m_ScreenMargin);
	m_ViewWindow.Create(m_LayoutBase.GetHandle(),
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN  | WS_CLIPSIBLINGS,0,IDC_VIEW);
	m_ViewWindow.SetMessageWindow(m_hwnd);
	m_pDisplay->GetVideoContainer().SetParent(m_ViewWindow.GetHandle());
	m_ViewWindow.SetVideoContainer(&m_pDisplay->GetVideoContainer());

	m_Panel.Create(m_LayoutBase.GetHandle(),
				   WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	m_Panel.ShowTitle(true);
	m_Panel.EnableFloating(false);
	m_Panel.SetEventHandler(&m_PanelEventHandler);
	CPanel::PanelTheme PanelTheme;
	m_App.Panel.Frame.GetPanelTheme(&PanelTheme);
	m_Panel.SetPanelTheme(PanelTheme);

	Layout::CSplitter *pSplitter=new Layout::CSplitter(CONTAINER_ID_PANELSPLITTER);
	pSplitter->SetVisible(true);
	if (m_PanelPlace==CPanelFrame::DOCKING_NONE) {
		if (!m_App.Panel.Frame.IsDockingVertical()) {
			m_PanelPlace=
				m_MainWindow.GetPanelPaneIndex()==0 ?
					CPanelFrame::DOCKING_LEFT : CPanelFrame::DOCKING_RIGHT;
		} else {
			m_PanelPlace=CPanelFrame::DOCKING_RIGHT;
		}
	}
	int PanelPaneIndex=
		m_PanelPlace==CPanelFrame::DOCKING_LEFT || m_PanelPlace==CPanelFrame::DOCKING_TOP?0:1;
	Layout::CWindowContainer *pViewContainer=new Layout::CWindowContainer(CONTAINER_ID_VIEW);
	pViewContainer->SetWindow(&m_ViewWindow);
	pViewContainer->SetMinSize(32,32);
	pViewContainer->SetVisible(true);
	pSplitter->SetPane(1-PanelPaneIndex,pViewContainer);
	Layout::CWindowContainer *pPanelContainer=new Layout::CWindowContainer(CONTAINER_ID_PANEL);
	pPanelContainer->SetWindow(&m_Panel);
	pPanelContainer->SetMinSize(32,32);
	pSplitter->SetPane(PanelPaneIndex,pPanelContainer);
	pSplitter->SetAdjustPane(CONTAINER_ID_VIEW);
	m_LayoutBase.SetTopContainer(pSplitter);

	RECT rc;
	m_pDisplay->GetDisplayBase().GetParent()->GetClientRect(&rc);
	m_pDisplay->GetDisplayBase().SetPosition(&rc);

	m_TitleBar.Create(m_ViewWindow.GetHandle(),
					  WS_CHILD | WS_CLIPSIBLINGS,0,IDC_TITLEBAR);
	m_TitleBar.SetEventHandler(&m_TitleBarManager);
	m_TitleBar.SetIcon(
		reinterpret_cast<HICON>(m_MainWindow.SendMessage(
			WM_GETICON,m_TitleBar.IsIconDrawSmall()?ICON_SMALL:ICON_BIG,0)));
	m_TitleBar.SetMaximizeMode(m_MainWindow.GetMaximize());
	m_TitleBar.SetFullscreenMode(true);

	m_App.OSDManager.Reset();

	m_App.CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(
		m_App.VideoOptions.GetFullscreenStretchMode());

	m_fShowCursor=true;
	m_fMenu=false;
	m_fShowStatusView=false;
	m_fShowTitleBar=false;
	m_fShowSideBar=false;
	m_fShowPanel=false;

	m_ShowCursorManager.Reset(4);
	::SetTimer(m_hwnd,TIMER_ID_HIDECURSOR,HIDE_CURSOR_DELAY,nullptr);

	return true;
}


void CMainWindow::CFullscreen::ShowCursor(bool fShow)
{
	m_App.CoreEngine.m_DtvEngine.m_MediaViewer.HideCursor(!fShow);
	m_ViewWindow.ShowCursor(fShow);
	m_fShowCursor=fShow;
}


void CMainWindow::CFullscreen::ShowPanel(bool fShow)
{
	if (m_fShowPanel!=fShow) {
		Layout::CSplitter *pSplitter=
			dynamic_cast<Layout::CSplitter*>(m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));
		const bool fVertical=
			m_PanelPlace==CPanelFrame::DOCKING_TOP ||
			m_PanelPlace==CPanelFrame::DOCKING_BOTTOM;

		if (fShow) {
			if (m_Panel.GetWindow()==nullptr) {
				if (m_PanelWidth<0)
					m_PanelWidth=m_App.Panel.Frame.GetDockingWidth();
				if (m_PanelHeight<0)
					m_PanelHeight=m_App.Panel.Frame.GetDockingHeight();
				m_App.Panel.Frame.SetPanelVisible(false);
				m_App.Panel.Frame.GetPanel()->SetWindow(nullptr,nullptr);
				m_Panel.SetWindow(&m_App.Panel.Form,TEXT("パネル"));
				pSplitter->SetStyle(
					fVertical ? Layout::CSplitter::STYLE_VERT : Layout::CSplitter::STYLE_HORZ);
				pSplitter->SetPaneSize(CONTAINER_ID_PANEL,
									   fVertical?m_PanelHeight:m_PanelWidth);
			}
			m_Panel.SendSizeMessage();
			m_LayoutBase.SetContainerVisible(CONTAINER_ID_PANEL,true);
			m_App.Panel.UpdateContent();
		} else {
			if (fVertical)
				m_PanelHeight=m_Panel.GetHeight();
			else
				m_PanelWidth=m_Panel.GetWidth();
			m_LayoutBase.SetContainerVisible(CONTAINER_ID_PANEL,false);
			if (!m_App.Panel.IsFloating())
				RestorePanel();
		}

		m_fShowPanel=fShow;

		m_App.UICore.SetCommandCheckedState(CM_PANEL,fShow);
	}
}


void CMainWindow::CFullscreen::RestorePanel()
{
	if (m_Panel.GetWindow()!=nullptr) {
		m_Panel.SetWindow(nullptr,nullptr);
		CPanel *pPanel=m_App.Panel.Frame.GetPanel();
		pPanel->SetWindow(&m_App.Panel.Form,TEXT("パネル"));
		pPanel->SendSizeMessage();
		if (m_App.Panel.fShowPanelWindow) {
			m_App.Panel.Frame.SetPanelVisible(true,true);
		}
	}
}


bool CMainWindow::CFullscreen::SetPanelWidth(int Width)
{
	m_PanelWidth=Width;
	return true;
}


bool CMainWindow::CFullscreen::SetPanelHeight(int Height)
{
	m_PanelHeight=Height;
	return true;
}


bool CMainWindow::CFullscreen::SetPanelPlace(CPanelFrame::DockingPlace Place)
{
	if (m_PanelPlace==Place)
		return true;

	if (m_Panel.GetWindow()!=nullptr) {
		Layout::CSplitter *pSplitter=dynamic_cast<Layout::CSplitter*>(
			m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));
		const bool fVerticalOld=
			m_PanelPlace==CPanelFrame::DOCKING_TOP ||
			m_PanelPlace==CPanelFrame::DOCKING_BOTTOM;
		const bool fVerticalNew=
			Place==CPanelFrame::DOCKING_TOP ||
			Place==CPanelFrame::DOCKING_BOTTOM;
		const int Index=
			Place==CPanelFrame::DOCKING_LEFT ||
			Place==CPanelFrame::DOCKING_TOP?0:1;

		if (fVerticalOld)
			m_PanelHeight=pSplitter->GetPaneSize(CONTAINER_ID_PANEL);
		else
			m_PanelWidth=pSplitter->GetPaneSize(CONTAINER_ID_PANEL);
		m_LayoutBase.LockLayout();
		if (fVerticalNew!=fVerticalOld) {
			pSplitter->SetStyle(
				fVerticalNew ? Layout::CSplitter::STYLE_VERT : Layout::CSplitter::STYLE_HORZ);
		}
		pSplitter->SetPaneSize(CONTAINER_ID_PANEL,
							   fVerticalNew?m_PanelHeight:m_PanelWidth);
		if (pSplitter->IDToIndex(CONTAINER_ID_PANEL)!=Index)
			pSplitter->SwapPane();
		m_LayoutBase.UnlockLayout();
	}

	m_PanelPlace=Place;

	return true;
}


void CMainWindow::CFullscreen::HideAllBars()
{
	ShowTitleBar(false);
	ShowSideBar(false);
	ShowStatusView(false);
}


void CMainWindow::CFullscreen::SetTheme(const TVTest::Theme::CThemeManager *pThemeManager)
{
	m_LayoutBase.SetBackColor(pThemeManager->GetColor(CColorScheme::COLOR_SPLITTER));
}


void CMainWindow::CFullscreen::OnMouseCommand(int Command)
{
	if (Command==0)
		return;
	// メニュー表示中はカーソルを消さない
	::KillTimer(m_hwnd,TIMER_ID_HIDECURSOR);
	ShowCursor(true);
	::SetCursor(LoadCursor(nullptr,IDC_ARROW));
	m_fMenu=true;
	m_MainWindow.SendMessage(WM_COMMAND,MAKEWPARAM(Command,CMainWindow::COMMAND_FROM_MOUSE),0);
	m_fMenu=false;
	if (m_hwnd!=nullptr)
		::SetTimer(m_hwnd,TIMER_ID_HIDECURSOR,HIDE_CURSOR_DELAY,nullptr);
}


void CMainWindow::CFullscreen::OnRButtonUp()
{
	OnMouseCommand(m_App.OperationOptions.GetRightClickCommand());
}


void CMainWindow::CFullscreen::OnMButtonUp()
{
	OnMouseCommand(m_App.OperationOptions.GetMiddleClickCommand());
}


void CMainWindow::CFullscreen::OnLButtonDoubleClick()
{
	OnMouseCommand(m_App.OperationOptions.GetLeftDoubleClickCommand());
}


void CMainWindow::CFullscreen::OnMouseMove()
{
	if (m_fMenu)
		return;

	POINT pt;
	RECT rcClient,rcStatus,rcTitle,rc;
	bool fShowStatusView=false,fShowTitleBar=false,fShowSideBar=false;

	::GetCursorPos(&pt);
	::ScreenToClient(m_hwnd,&pt);
	m_ViewWindow.GetClientRect(&rcClient);

	rcStatus=rcClient;
	rcStatus.top=rcStatus.bottom-m_App.StatusView.CalcHeight(rcClient.right-rcClient.left);
	if (::PtInRect(&rcStatus,pt))
		fShowStatusView=true;
	rc=rcClient;
	m_TitleBarManager.Layout(&rc,&rcTitle);
	if (::PtInRect(&rcTitle,pt))
		fShowTitleBar=true;

	if (m_App.SideBarOptions.ShowPopup()) {
		RECT rcSideBar;
		switch (m_App.SideBarOptions.GetPlace()) {
		case CSideBarOptions::PLACE_LEFT:
		case CSideBarOptions::PLACE_RIGHT:
			if (!fShowStatusView && !fShowTitleBar) {
				m_MainWindow.m_SideBarManager.Layout(&rcClient,&rcSideBar);
				if (::PtInRect(&rcSideBar,pt))
					fShowSideBar=true;
			}
			break;
		case CSideBarOptions::PLACE_TOP:
			rcClient.top=rcTitle.bottom;
			m_MainWindow.m_SideBarManager.Layout(&rcClient,&rcSideBar);
			if (::PtInRect(&rcSideBar,pt)) {
				fShowSideBar=true;
				fShowTitleBar=true;
			}
			break;
		case CSideBarOptions::PLACE_BOTTOM:
			rcClient.bottom=rcStatus.top;
			m_MainWindow.m_SideBarManager.Layout(&rcClient,&rcSideBar);
			if (::PtInRect(&rcSideBar,pt)) {
				fShowSideBar=true;
				fShowStatusView=true;
			}
			break;
		}
	}

	ShowStatusView(fShowStatusView);
	ShowTitleBar(fShowTitleBar);
	ShowSideBar(fShowSideBar);

	if (fShowStatusView || fShowTitleBar || fShowSideBar) {
		::KillTimer(m_hwnd,TIMER_ID_HIDECURSOR);
		return;
	}

	if (m_ShowCursorManager.OnCursorMove(pt.x,pt.y)) {
		if (!m_fShowCursor) {
			::SetCursor(::LoadCursor(nullptr,IDC_ARROW));
			ShowCursor(true);
		}
	}

	::SetTimer(m_hwnd,TIMER_ID_HIDECURSOR,HIDE_CURSOR_DELAY,nullptr);
}


void CMainWindow::CFullscreen::ShowStatusView(bool fShow)
{
	if (fShow==m_fShowStatusView)
		return;

	Layout::CLayoutBase &LayoutBase=m_MainWindow.GetLayoutBase();

	if (fShow) {
		RECT rc;

		ShowSideBar(false);
		m_ViewWindow.GetClientRect(&rc);
		rc.top=rc.bottom-m_App.StatusView.CalcHeight(rc.right-rc.left);
		m_App.StatusView.SetVisible(false);
		LayoutBase.SetContainerVisible(CONTAINER_ID_STATUS,false);
		m_App.StatusView.SetParent(&m_ViewWindow);
		m_App.StatusView.SetPosition(&rc);
		m_MainWindow.ShowPopupStatusBar(true);
	} else {
		m_MainWindow.ShowPopupStatusBar(false);
		m_App.StatusView.SetParent(&LayoutBase);
		if (m_MainWindow.GetStatusBarVisible())
			LayoutBase.SetContainerVisible(CONTAINER_ID_STATUS,true);
	}

	m_fShowStatusView=fShow;
}


void CMainWindow::CFullscreen::ShowTitleBar(bool fShow)
{
	if (fShow==m_fShowTitleBar)
		return;

	if (fShow) {
		RECT rc,rcBar;

		ShowSideBar(false);
		m_ViewWindow.GetClientRect(&rc);
		m_TitleBarManager.Layout(&rc,&rcBar);
		m_TitleBar.SetPosition(&rcBar);
		m_TitleBar.SetLabel(m_MainWindow.GetTitleBar().GetLabel());
		m_TitleBar.SetMaximizeMode(m_MainWindow.GetMaximize());
		CTitleBar::TitleBarTheme TitleBarTheme;
		m_MainWindow.GetTitleBar().GetTitleBarTheme(&TitleBarTheme);
		m_TitleBar.SetTitleBarTheme(TitleBarTheme);
		m_TitleBar.SetVisible(true);
		::BringWindowToTop(m_TitleBar.GetHandle());
	} else {
		m_TitleBar.SetVisible(false);
	}

	m_fShowTitleBar=fShow;
}


void CMainWindow::CFullscreen::ShowSideBar(bool fShow)
{
	if (fShow==m_fShowSideBar)
		return;

	Layout::CLayoutBase &LayoutBase=m_MainWindow.GetLayoutBase();

	if (fShow) {
		RECT rcClient,rcBar;

		m_ViewWindow.GetClientRect(&rcClient);
		if (m_fShowStatusView)
			rcClient.bottom-=m_App.StatusView.GetHeight();
		if (m_fShowTitleBar)
			rcClient.top+=m_TitleBar.GetHeight();
		m_MainWindow.m_SideBarManager.Layout(&rcClient,&rcBar);
		m_App.SideBar.SetVisible(false);
		LayoutBase.SetContainerVisible(CONTAINER_ID_SIDEBAR,false);
		m_App.SideBar.SetParent(&m_ViewWindow);
		m_App.SideBar.SetPosition(&rcBar);
		m_MainWindow.ShowPopupSideBar(true);
	} else {
		m_MainWindow.ShowPopupSideBar(false);
		m_App.SideBar.SetParent(&LayoutBase);
		if (m_MainWindow.GetSideBarVisible())
			LayoutBase.SetContainerVisible(CONTAINER_ID_SIDEBAR,true);
	}

	m_fShowSideBar=fShow;
}


CMainWindow::CFullscreen::CPanelEventHandler::CPanelEventHandler(CFullscreen &Fullscreen)
	: m_Fullscreen(Fullscreen)
{
}

bool CMainWindow::CFullscreen::CPanelEventHandler::OnClose()
{
	m_Fullscreen.m_MainWindow.SendCommand(CM_PANEL);
	return true;
}


enum {
	PANEL_MENU_LEFT=CPanel::MENU_USER,
	PANEL_MENU_RIGHT,
	PANEL_MENU_TOP,
	PANEL_MENU_BOTTOM
};

bool CMainWindow::CFullscreen::CPanelEventHandler::OnMenuPopup(HMENU hmenu)
{
	::AppendMenu(hmenu,MF_SEPARATOR,0,NULL);
	::AppendMenu(hmenu,MF_STRING | MF_ENABLED,PANEL_MENU_LEFT,TEXT("左へ(&L)"));
	::AppendMenu(hmenu,MF_STRING | MF_ENABLED,PANEL_MENU_RIGHT,TEXT("右へ(&R)"));
	::AppendMenu(hmenu,MF_STRING | MF_ENABLED,PANEL_MENU_TOP,TEXT("上へ(&T)"));
	::AppendMenu(hmenu,MF_STRING | MF_ENABLED,PANEL_MENU_BOTTOM,TEXT("下へ(&B)"));
	::EnableMenuItem(hmenu,
		m_Fullscreen.m_PanelPlace==CPanelFrame::DOCKING_LEFT?PANEL_MENU_LEFT:
		m_Fullscreen.m_PanelPlace==CPanelFrame::DOCKING_RIGHT?PANEL_MENU_RIGHT:
		m_Fullscreen.m_PanelPlace==CPanelFrame::DOCKING_TOP?PANEL_MENU_TOP:PANEL_MENU_BOTTOM,
		MF_BYCOMMAND | MF_GRAYED);
	return true;
}


bool CMainWindow::CFullscreen::CPanelEventHandler::OnMenuSelected(int Command)
{
	switch (Command) {
	case PANEL_MENU_LEFT:
		m_Fullscreen.SetPanelPlace(CPanelFrame::DOCKING_LEFT);
		return true;
	case PANEL_MENU_RIGHT:
		m_Fullscreen.SetPanelPlace(CPanelFrame::DOCKING_RIGHT);
		return true;
	case PANEL_MENU_TOP:
		m_Fullscreen.SetPanelPlace(CPanelFrame::DOCKING_TOP);
		return true;
	case PANEL_MENU_BOTTOM:
		m_Fullscreen.SetPanelPlace(CPanelFrame::DOCKING_BOTTOM);
		return true;
	}

	return false;
}




CMainWindow::MainWindowStyle::MainWindowStyle()
	: ScreenMargin(0,0,0,0)
	, FullscreenMargin(0,0,0,0)
{
	int SizingBorderX=0,SizingBorderY;

#ifdef WIN_XP_SUPPORT
	if (Util::OS::IsWindowsVistaOrLater())
#endif
	{
		NONCLIENTMETRICS ncm;

		ncm.cbSize=sizeof(NONCLIENTMETRICS);
		if (::SystemParametersInfo(SPI_GETNONCLIENTMETRICS,ncm.cbSize,&ncm,0))
			SizingBorderX=SizingBorderY=ncm.iBorderWidth+ncm.iPaddedBorderWidth;
	}
	if (SizingBorderX==0) {
		SizingBorderX=::GetSystemMetrics(SM_CXSIZEFRAME);
		SizingBorderY=::GetSystemMetrics(SM_CYSIZEFRAME);
	}
	ResizingMargin=Style::Margins(SizingBorderX,SizingBorderY,SizingBorderX,SizingBorderY);
}

void CMainWindow::MainWindowStyle::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	pStyleManager->Get(TEXT("screen.margin"),&ScreenMargin);
	pStyleManager->Get(TEXT("fullscreen.margin"),&FullscreenMargin);
	pStyleManager->Get(TEXT("main-window.resizing-margin"),&ResizingMargin);
}

void CMainWindow::MainWindowStyle::NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	pStyleManager->ToPixels(&ScreenMargin);
	pStyleManager->ToPixels(&FullscreenMargin);
	pStyleManager->ToPixels(&ResizingMargin);
}




bool CMainWindow::CBarLayout::IsSpot(const RECT *pArea,const POINT *pPos)
{
	RECT rcArea=*pArea,rcBar;

	Layout(&rcArea,&rcBar);
	return ::PtInRect(&rcBar,*pPos)!=FALSE;
}

void CMainWindow::CBarLayout::AdjustArea(RECT *pArea)
{
	RECT rcBar;
	Layout(pArea,&rcBar);
}

void CMainWindow::CBarLayout::ReserveArea(RECT *pArea,bool fNoMove)
{
	RECT rc;

	rc=*pArea;
	AdjustArea(&rc);
	if (fNoMove) {
		pArea->right+=(pArea->right-pArea->left)-(rc.right-rc.left);
		pArea->bottom+=(pArea->bottom-pArea->top)-(rc.bottom-rc.top);
	} else {
		pArea->left-=rc.left-pArea->left;
		pArea->top-=rc.top-pArea->top;
		pArea->right+=pArea->right-rc.right;
		pArea->bottom+=pArea->bottom-rc.bottom;
	}
}


CMainWindow::CTitleBarManager::CTitleBarManager(CMainWindow *pMainWindow,bool fMainWindow)
	: m_pMainWindow(pMainWindow)
	, m_fMainWindow(fMainWindow)
	, m_fFixed(false)
{
}

bool CMainWindow::CTitleBarManager::OnClose()
{
	m_pMainWindow->PostCommand(CM_CLOSE);
	return true;
}

bool CMainWindow::CTitleBarManager::OnMinimize()
{
	m_pMainWindow->SendMessage(WM_SYSCOMMAND,SC_MINIMIZE,0);
	return true;
}

bool CMainWindow::CTitleBarManager::OnMaximize()
{
	m_pMainWindow->SendMessage(WM_SYSCOMMAND,
		m_pMainWindow->GetMaximize()?SC_RESTORE:SC_MAXIMIZE,0);
	return true;
}

bool CMainWindow::CTitleBarManager::OnFullscreen()
{
	m_pMainWindow->m_pCore->ToggleFullscreen();
	return true;
}

void CMainWindow::CTitleBarManager::OnMouseLeave()
{
	if (!m_fFixed)
		m_pMainWindow->OnBarMouseLeave(m_pTitleBar->GetHandle());
}

void CMainWindow::CTitleBarManager::OnLabelLButtonDown(int x,int y)
{
	if (m_fMainWindow) {
		POINT pt;

		pt.x=x;
		pt.y=y;
		::ClientToScreen(m_pTitleBar->GetHandle(),&pt);
		m_pMainWindow->SendMessage(WM_NCLBUTTONDOWN,HTCAPTION,MAKELPARAM(pt.x,pt.y));
		m_fFixed=true;
	}
}

void CMainWindow::CTitleBarManager::OnLabelLButtonDoubleClick(int x,int y)
{
	if (m_fMainWindow)
		OnMaximize();
	else
		OnFullscreen();
}

void CMainWindow::CTitleBarManager::OnLabelRButtonUp(int x,int y)
{
	POINT pt;

	pt.x=x;
	pt.y=y;
	::ClientToScreen(m_pTitleBar->GetHandle(),&pt);
	ShowSystemMenu(pt.x,pt.y);
}

void CMainWindow::CTitleBarManager::OnIconLButtonDown(int x,int y)
{
	RECT rc;

	m_pTitleBar->GetScreenPosition(&rc);
	ShowSystemMenu(rc.left,rc.bottom);
}

void CMainWindow::CTitleBarManager::OnIconLButtonDoubleClick(int x,int y)
{
	m_pMainWindow->PostCommand(CM_CLOSE);
}

void CMainWindow::CTitleBarManager::OnHeightChanged(int Height)
{
	m_pMainWindow->SetFixedPaneSize(CONTAINER_ID_TITLEBARSPLITTER,CONTAINER_ID_TITLEBAR,0,Height);
}

void CMainWindow::CTitleBarManager::Layout(RECT *pArea,RECT *pBarRect)
{
	pBarRect->left=pArea->left;
	pBarRect->top=pArea->top;
	pBarRect->right=pArea->right;
	pBarRect->bottom=pArea->top+m_pTitleBar->GetHeight();
	pArea->top+=m_pTitleBar->GetHeight();
}

void CMainWindow::CTitleBarManager::EndDrag()
{
	m_fFixed=false;
}

void CMainWindow::CTitleBarManager::ShowSystemMenu(int x,int y)
{
	m_fFixed=true;
	m_pMainWindow->SendMessage(0x0313,0,MAKELPARAM(x,y));
	m_fFixed=false;

	RECT rc;
	POINT pt;
	m_pTitleBar->GetScreenPosition(&rc);
	::GetCursorPos(&pt);
	if (!::PtInRect(&rc,pt))
		OnMouseLeave();
}


CMainWindow::CSideBarManager::CSideBarManager(CMainWindow *pMainWindow)
	: m_pMainWindow(pMainWindow)
	, m_fFixed(false)
{
}

void CMainWindow::CSideBarManager::Layout(RECT *pArea,RECT *pBarRect)
{
	const int BarWidth=m_pSideBar->GetBarWidth();

	switch (m_pMainWindow->m_App.SideBarOptions.GetPlace()) {
	case CSideBarOptions::PLACE_LEFT:
		pBarRect->left=pArea->left;
		pBarRect->top=pArea->top;
		pBarRect->right=pBarRect->left+BarWidth;
		pBarRect->bottom=pArea->bottom;
		pArea->left+=BarWidth;
		break;

	case CSideBarOptions::PLACE_RIGHT:
		pBarRect->left=pArea->right-BarWidth;
		pBarRect->top=pArea->top;
		pBarRect->right=pArea->right;
		pBarRect->bottom=pArea->bottom;
		pArea->right-=BarWidth;
		break;

	case CSideBarOptions::PLACE_TOP:
		pBarRect->left=pArea->left;
		pBarRect->top=pArea->top;
		pBarRect->right=pArea->right;
		pBarRect->bottom=pArea->top+BarWidth;
		pArea->top+=BarWidth;
		break;

	case CSideBarOptions::PLACE_BOTTOM:
		pBarRect->left=pArea->left;
		pBarRect->top=pArea->bottom-BarWidth;
		pBarRect->right=pArea->right;
		pBarRect->bottom=pArea->bottom;
		pArea->bottom-=BarWidth;
		break;
	}
}

const CChannelInfo *CMainWindow::CSideBarManager::GetChannelInfoByCommand(int Command)
{
	const CChannelList *pList=m_pMainWindow->m_App.ChannelManager.GetCurrentChannelList();
	if (pList!=nullptr) {
		int No=Command-CM_CHANNELNO_FIRST;
		int Index;

		if (pList->HasRemoteControlKeyID()) {
			Index=pList->FindChannelNo(No+1);
			if (Index<0)
				return nullptr;
		} else {
			Index=No;
		}
		return pList->GetChannelInfo(Index);
	}
	return nullptr;
}

void CMainWindow::CSideBarManager::OnCommand(int Command)
{
	m_pMainWindow->SendCommand(Command);
}

void CMainWindow::CSideBarManager::OnRButtonUp(int x,int y)
{
	CPopupMenu Menu(m_pMainWindow->m_App.GetResourceInstance(),IDM_SIDEBAR);
	POINT pt;

	Menu.CheckItem(CM_SIDEBAR,m_pMainWindow->GetSideBarVisible());
	Menu.EnableItem(CM_SIDEBAR,!m_pMainWindow->m_pCore->GetFullscreen());
	Menu.CheckRadioItem(CM_SIDEBAR_PLACE_FIRST,CM_SIDEBAR_PLACE_LAST,
						CM_SIDEBAR_PLACE_FIRST+(int)m_pMainWindow->m_App.SideBarOptions.GetPlace());
	pt.x=x;
	pt.y=y;
	::ClientToScreen(m_pSideBar->GetHandle(),&pt);
	m_fFixed=true;
	Menu.Show(m_pMainWindow->GetHandle(),&pt);
	m_fFixed=false;

	RECT rc;
	m_pSideBar->GetScreenPosition(&rc);
	::GetCursorPos(&pt);
	if (!::PtInRect(&rc,pt))
		OnMouseLeave();
}

void CMainWindow::CSideBarManager::OnMouseLeave()
{
	if (!m_fFixed)
		m_pMainWindow->OnBarMouseLeave(m_pSideBar->GetHandle());
}

bool CMainWindow::CSideBarManager::GetTooltipText(int Command,LPTSTR pszText,int MaxText)
{
	if (Command>=CM_CHANNELNO_FIRST && Command<=CM_CHANNELNO_LAST) {
		const CChannelInfo *pChInfo=GetChannelInfoByCommand(Command);
		if (pChInfo!=nullptr) {
			StdUtil::snprintf(pszText,MaxText,TEXT("%d: %s"),
							  (Command-CM_CHANNELNO_FIRST)+1,pChInfo->GetName());
			return true;
		}
	}
	return false;
}

bool CMainWindow::CSideBarManager::DrawIcon(const CSideBar::DrawIconInfo *pInfo)
{
	if (pInfo->Command>=CM_CHANNELNO_FIRST && pInfo->Command<=CM_CHANNELNO_LAST) {
		if (m_pMainWindow->m_App.SideBarOptions.GetShowChannelLogo()) {
			// アイコンに局ロゴを表示
			// TODO: 新しくロゴが取得された時に再描画する
			const CChannelInfo *pChannel=GetChannelInfoByCommand(pInfo->Command);
			if (pChannel!=nullptr) {
				HBITMAP hbmLogo=m_pMainWindow->m_App.LogoManager.GetAssociatedLogoBitmap(
					pChannel->GetNetworkID(),pChannel->GetServiceID(),
					CLogoManager::LOGOTYPE_SMALL);
				if (hbmLogo!=nullptr) {
					const int Width=pInfo->IconRect.right-pInfo->IconRect.left;
					const int Height=pInfo->IconRect.bottom-pInfo->IconRect.top;
					const int IconHeight=Height*10/16;	// 本来の比率より縦長にしている(見栄えのため)
					HBITMAP hbmOld=SelectBitmap(pInfo->hdcBuffer,hbmLogo);
					int OldStretchMode=::SetStretchBltMode(pInfo->hdc,STRETCH_HALFTONE);
					BITMAP bm;
					::GetObject(hbmLogo,sizeof(bm),&bm);
					::StretchBlt(pInfo->hdc,
								 pInfo->IconRect.left,pInfo->IconRect.top+(Height-IconHeight)/2,Width,IconHeight,
								 pInfo->hdcBuffer,0,0,bm.bmWidth,bm.bmHeight,SRCCOPY);
					::SetStretchBltMode(pInfo->hdc,OldStretchMode);
					::SelectObject(pInfo->hdcBuffer,hbmOld);
					return true;
				}
			}
		}
	} else if (pInfo->Command>=CM_PLUGIN_FIRST
			&& pInfo->Command<=CM_PLUGIN_LAST) {
		CPlugin *pPlugin=m_pMainWindow->m_App.PluginManager.GetPluginByCommand(pInfo->Command);

		if (pPlugin!=nullptr && pPlugin->GetIcon().IsCreated()) {
			pPlugin->GetIcon().Draw(
				pInfo->hdc,
				pInfo->IconRect.left,pInfo->IconRect.top,
				pInfo->IconRect.right-pInfo->IconRect.left,
				pInfo->IconRect.bottom-pInfo->IconRect.top,
				0,0,0,0,
				pInfo->Color,pInfo->Opacity);
			return true;
		}
	} else if (pInfo->Command>=CM_PLUGINCOMMAND_FIRST
			&& pInfo->Command<=CM_PLUGINCOMMAND_LAST) {
		LPCTSTR pszCommand;
		CPlugin *pPlugin=m_pMainWindow->m_App.PluginManager.GetPluginByPluginCommand(
			m_pMainWindow->m_App.CommandList.GetCommandTextByID(pInfo->Command),&pszCommand);

		if (pPlugin!=nullptr) {
			CPlugin::CPluginCommandInfo *pCommandInfo=
				pPlugin->GetPluginCommandInfo(pszCommand);

			if (pCommandInfo!=nullptr) {
				if ((pCommandInfo->GetFlags() & TVTest::PLUGIN_COMMAND_FLAG_NOTIFYDRAWICON)!=0) {
					TVTest::DrawCommandIconInfo Info;

					Info.ID=pCommandInfo->GetID();
					Info.Flags=0;
					Info.State=0;
					if ((pInfo->State & CSideBar::ITEM_STATE_DISABLED)!=0)
						Info.State|=TVTest::COMMAND_ICON_STATE_DISABLED;
					if ((pInfo->State & CSideBar::ITEM_STATE_CHECKED)!=0)
						Info.State|=TVTest::COMMAND_ICON_STATE_CHECKED;
					if ((pInfo->State & CSideBar::ITEM_STATE_HOT)!=0)
						Info.State|=TVTest::COMMAND_ICON_STATE_HOT;
					if ((Info.State & TVTest::COMMAND_ICON_STATE_HOT)!=0)
						Info.pszStyle=L"side-bar.item.hot";
					else if ((Info.State & TVTest::COMMAND_ICON_STATE_CHECKED)!=0)
						Info.pszStyle=L"side-bar.item.checked";
					else
						Info.pszStyle=L"side-bar.item";
					Info.hdc=pInfo->hdc;
					Info.DrawRect=pInfo->IconRect;
					Info.Color=pInfo->Color;
					Info.Opacity=pInfo->Opacity;

					if (pPlugin->DrawPluginCommandIcon(&Info))
						return true;
				}

				pCommandInfo->GetIcon().Draw(
					pInfo->hdc,
					pInfo->IconRect.left,pInfo->IconRect.top,
					pInfo->IconRect.right-pInfo->IconRect.left,
					pInfo->IconRect.bottom-pInfo->IconRect.top,
					0,0,0,0,
					pInfo->Color,pInfo->Opacity);
			}
		}

		return true;
	}

	return false;
}

void CMainWindow::CSideBarManager::OnBarWidthChanged(int BarWidth)
{
	int Width,Height;

	if (m_pMainWindow->m_App.SideBar.GetVertical()) {
		Width=BarWidth;
		Height=0;
	} else {
		Width=0;
		Height=BarWidth;
	}
	m_pMainWindow->SetFixedPaneSize(CONTAINER_ID_SIDEBARSPLITTER,CONTAINER_ID_SIDEBAR,Width,Height);
}


CMainWindow::CStatusViewEventHandler::CStatusViewEventHandler(CMainWindow *pMainWindow)
	: m_pMainWindow(pMainWindow)
{
}

void CMainWindow::CStatusViewEventHandler::OnMouseLeave()
{
	m_pMainWindow->OnBarMouseLeave(m_pStatusView->GetHandle());
}

void CMainWindow::CStatusViewEventHandler::OnHeightChanged(int Height)
{
	m_pMainWindow->SetFixedPaneSize(CONTAINER_ID_STATUSSPLITTER,CONTAINER_ID_STATUS,0,Height);
}


CMainWindow::CVideoContainerEventHandler::CVideoContainerEventHandler(CMainWindow *pMainWindow)
	: m_pMainWindow(pMainWindow)
{
}

void CMainWindow::CVideoContainerEventHandler::OnSizeChanged(int Width,int Height)
{
	CNotificationBar &NotificationBar=m_pMainWindow->m_NotificationBar;
	if (NotificationBar.GetVisible()) {
		RECT rc,rcView;

		NotificationBar.GetPosition(&rc);
		::GetClientRect(NotificationBar.GetParent(),&rcView);
		rc.left=rcView.left;
		rc.right=rcView.right;
		NotificationBar.SetPosition(&rc);
	}

	m_pMainWindow->m_App.OSDManager.HideVolumeOSD();
}


CMainWindow::CViewWindowEventHandler::CViewWindowEventHandler(CMainWindow *pMainWindow)
	: m_pMainWindow(pMainWindow)
{
}

void CMainWindow::CViewWindowEventHandler::OnSizeChanged(int Width,int Height)
{
	// 一時的に表示されているバーのサイズを合わせる
	RECT rcView,rc;

	m_pView->GetPosition(&rcView);
	if (!m_pMainWindow->GetTitleBarVisible()
			&& m_pMainWindow->m_TitleBar.GetVisible()) {
		m_pMainWindow->m_TitleBarManager.Layout(&rcView,&rc);
		m_pMainWindow->m_TitleBar.SetPosition(&rc);
	}
	CStatusView &StatusView=m_pMainWindow->GetStatusView();
	if (!m_pMainWindow->GetStatusBarVisible()
			&& StatusView.GetVisible()
			&& StatusView.GetParent()==m_pView->GetParent()) {
		rc=rcView;
		rc.top=rc.bottom-StatusView.GetHeight();
		rcView.bottom-=StatusView.GetHeight();
		StatusView.SetPosition(&rc);
	}
	CSideBar &SideBar=m_pMainWindow->GetSideBar();
	if (!m_pMainWindow->GetSideBarVisible()
			&& SideBar.GetVisible()) {
		m_pMainWindow->m_SideBarManager.Layout(&rcView,&rc);
		SideBar.SetPosition(&rc);
	}
}


CMainWindow::CDisplayBaseEventHandler::CDisplayBaseEventHandler(CMainWindow *pMainWindow)
	: m_pMainWindow(pMainWindow)
{
}

bool CMainWindow::CDisplayBaseEventHandler::OnVisibleChange(bool fVisible)
{
	if (!m_pMainWindow->IsViewerEnabled()) {
		m_pMainWindow->m_Display.GetVideoContainer().SetVisible(fVisible);
	}
	if (fVisible && m_pMainWindow->m_pCore->GetFullscreen()) {
		m_pMainWindow->m_Fullscreen.HideAllBars();
	}
	return true;
}


CMainWindow::CShowCursorManager::CShowCursorManager()
{
	Reset();
}

void CMainWindow::CShowCursorManager::Reset(int Delta)
{
	m_MoveDelta=Delta;
	m_LastMovePos.x=LONG_MAX/2;
	m_LastMovePos.y=LONG_MAX/2;
}

bool CMainWindow::CShowCursorManager::OnCursorMove(int x,int y)
{
	if (abs(m_LastMovePos.x-x)>=m_MoveDelta || abs(m_LastMovePos.y-y)>=m_MoveDelta) {
		m_LastMovePos.x=x;
		m_LastMovePos.y=y;
		return true;
	}

	return false;
}


CMainWindow::CEpgCaptureEventHandler::CEpgCaptureEventHandler(CMainWindow *pMainWindow)
	 : m_pMainWindow(pMainWindow)
{
}

void CMainWindow::CEpgCaptureEventHandler::OnBeginCapture(unsigned int Flags,unsigned int Status)
{
	if ((Status & CEpgCaptureManager::BEGIN_STATUS_STANDBY)==0) {
		m_pMainWindow->StoreTunerResumeInfo();
		m_pMainWindow->m_Resume.fOpenTuner=
			(Status & CEpgCaptureManager::BEGIN_STATUS_TUNER_ALREADY_OPENED)!=0;
	}

	m_pMainWindow->SuspendViewer(ResumeInfo::VIEWERSUSPEND_EPGUPDATE);

	m_pMainWindow->m_App.Epg.ProgramGuide.OnEpgCaptureBegin();
}

void CMainWindow::CEpgCaptureEventHandler::OnEndCapture(unsigned int Flags)
{
	CAppMain &App=m_pMainWindow->m_App;
	HANDLE hThread;
	int OldPriority;

	::KillTimer(m_pMainWindow->m_hwnd,TIMER_ID_PROGRAMGUIDEUPDATE);

	if (m_pMainWindow->m_pCore->GetStandby()) {
		hThread=::GetCurrentThread();
		OldPriority=::GetThreadPriority(hThread);
		::SetThreadPriority(hThread,THREAD_PRIORITY_LOWEST);
	} else {
		::SetCursor(::LoadCursor(nullptr,IDC_WAIT));
	}
	App.EpgProgramList.UpdateProgramList();
	App.EpgOptions.SaveEpgFile(&App.EpgProgramList);
	App.Epg.ProgramGuide.OnEpgCaptureEnd();
	if (m_pMainWindow->m_pCore->GetStandby()) {
		App.Epg.ProgramGuide.Refresh();
		::SetThreadPriority(hThread,OldPriority);
		if ((Flags & CEpgCaptureManager::END_CLOSE_TUNER)!=0)
			App.Core.CloseTuner();
	} else {
		::SetCursor(::LoadCursor(nullptr,IDC_ARROW));
		if ((Flags & CEpgCaptureManager::END_RESUME)!=0)
			m_pMainWindow->ResumeChannel();
		if (m_pMainWindow->IsPanelVisible()
				&& App.Panel.Form.GetCurPageID()==PANEL_ID_CHANNEL)
			App.Panel.ChannelPanel.UpdateAllChannels(false);
	}

	m_pMainWindow->ResumeViewer(ResumeInfo::VIEWERSUSPEND_EPGUPDATE);
}

void CMainWindow::CEpgCaptureEventHandler::OnChannelChanged()
{
	CAppMain &App=m_pMainWindow->m_App;

	::SetTimer(m_pMainWindow->m_hwnd,TIMER_ID_PROGRAMGUIDEUPDATE,15000,nullptr);

	App.Epg.ProgramGuide.SetEpgCaptureProgress(
		App.EpgCaptureManager.GetCurChannel(),
		App.EpgCaptureManager.GetChannelCount(),
		App.EpgCaptureManager.GetRemainingTime());
}

void CMainWindow::CEpgCaptureEventHandler::OnChannelEnd(bool fComplete)
{
	if (!m_pMainWindow->m_pCore->GetStandby())
		m_pMainWindow->m_App.Epg.ProgramGuide.Refresh();
}


CMainWindow::CCommandEventHandler::CCommandEventHandler(CMainWindow *pMainWindow)
	: m_pMainWindow(pMainWindow)
{
}

void CMainWindow::CCommandEventHandler::OnCommandStateChanged(
	int ID,unsigned int OldState,unsigned int NewState)
{
	if (((OldState ^ NewState) & CCommandList::COMMAND_STATE_CHECKED)!=0) {
		const bool fChecked=(NewState & CCommandList::COMMAND_STATE_CHECKED)!=0;
		m_pMainWindow->m_App.MainMenu.CheckItem(ID,fChecked);
		m_pMainWindow->m_App.SideBar.CheckItem(ID,fChecked);
	}

	if (((OldState ^ NewState) & CCommandList::COMMAND_STATE_DISABLED)!=0) {
		const bool fEnabled=(NewState & CCommandList::COMMAND_STATE_DISABLED)==0;
		m_pMainWindow->m_App.MainMenu.EnableItem(ID,fEnabled);
		m_pMainWindow->m_App.SideBar.EnableItem(ID,fEnabled);
	}
}

void CMainWindow::CCommandEventHandler::OnCommandRadioCheckedStateChanged(
	int FirstID,int LastID,int CheckedID)
{
	if (FirstID>=CM_ASPECTRATIO_FIRST && FirstID<=CM_ASPECTRATIO_LAST)
		m_pMainWindow->m_App.AspectRatioIconMenu.CheckRadioItem(FirstID,LastID,CheckedID);
	else
		m_pMainWindow->m_App.MainMenu.CheckRadioItem(FirstID,LastID,CheckedID);
	m_pMainWindow->m_App.SideBar.CheckRadioItem(FirstID,LastID,CheckedID);
}
