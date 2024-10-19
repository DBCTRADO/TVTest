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


#include "stdafx.h"
#include <dwmapi.h>
#include "TVTest.h"
#include "AppMain.h"
#include "AboutDialog.h"
#include "DialogUtil.h"
#include "DrawUtil.h"
#include "WindowUtil.h"
#include "DPIUtil.h"
#include "PseudoOSD.h"
#include "EventInfoPopup.h"
#include "ToolTip.h"
#include "DarkMode.h"
#include "resource.h"
#include "Common/DebugDef.h"

#pragma comment(lib,"imm32.lib") // for ImmAssociateContext(Ex)


namespace TVTest
{

namespace
{

const LPCTSTR MAIN_TITLE_TEXT = APP_NAME;


static int CalcZoomSize(int Size, int Rate, int Factor)
{
	if (Factor == 0)
		return 0;
	return ::MulDiv(Size, Rate, Factor);
}

}




const LPCTSTR MAIN_WINDOW_CLASS       = APP_NAME TEXT(" Window");
const LPCTSTR FULLSCREEN_WINDOW_CLASS = APP_NAME TEXT(" Fullscreen");




const CMainWindow::DirectShowFilterPropertyInfo CMainWindow::m_DirectShowFilterPropertyList[] = {
	{LibISDB::ViewerFilter::PropertyFilterType::VideoDecoder,       CM_VIDEODECODERPROPERTY},
	{LibISDB::ViewerFilter::PropertyFilterType::VideoRenderer,      CM_VIDEORENDERERPROPERTY},
	{LibISDB::ViewerFilter::PropertyFilterType::AudioFilter,        CM_AUDIOFILTERPROPERTY},
	{LibISDB::ViewerFilter::PropertyFilterType::AudioRenderer,      CM_AUDIORENDERERPROPERTY},
	{LibISDB::ViewerFilter::PropertyFilterType::MPEG2Demultiplexer, CM_DEMULTIPLEXERPROPERTY},
};

CMainWindow *CMainWindow::m_pThis = nullptr;
HHOOK CMainWindow::m_hHook = nullptr;


bool CMainWindow::Initialize(HINSTANCE hinst)
{
	WNDCLASSEX wc;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hinst;
	wc.hIcon = CAppMain::GetAppIcon();
	wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = MAIN_WINDOW_CLASS;
	wc.hIconSm = CAppMain::GetAppIconSmall();
	return ::RegisterClassEx(&wc) != 0 && CFullscreen::Initialize(hinst);
}


CMainWindow::CMainWindow(CAppMain &App)
	: m_App(App)
	, m_Display(App)
	, m_ChannelInput(App.Accelerator.GetChannelInputOptions())
{
	POINT pt = {0, 0};
	int DPI = GetMonitorDPI(::MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY));
	if (DPI == 0)
		DPI = GetSystemDPI();

	// 適当にデフォルトサイズを設定
#ifndef TVTEST_FOR_1SEG
	static constexpr int DefaultWidth = 960, DefaultHeight = 540;
#else
	static constexpr int DefaultWidth = 640, DefaultHeight = 360;
#endif
	m_WindowPosition.Width = ::MulDiv(DefaultWidth, DPI, 96);
	m_WindowPosition.Height = ::MulDiv(DefaultHeight, DPI, 96);
	m_WindowPosition.Left =
		(::GetSystemMetrics(SM_CXSCREEN) - m_WindowPosition.Width) / 2;
	m_WindowPosition.Top =
		(::GetSystemMetrics(SM_CYSCREEN) - m_WindowPosition.Height) / 2;

	m_ThinFrameWidth = std::max(::MulDiv(1, DPI, 96), 1);

	if (Util::OS::IsWindows10OrLater()) {
		m_fCustomFrame = true;
		m_CustomFrameWidth = m_ThinFrameWidth;
	}
}


CMainWindow::~CMainWindow()
{
	Destroy();
}


bool CMainWindow::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	if (m_pCore->GetAlwaysOnTop())
		ExStyle |= WS_EX_TOPMOST;
	m_pThis = this;
	if (!CreateBasicWindow(nullptr, Style, ExStyle, ID, MAIN_WINDOW_CLASS, MAIN_TITLE_TEXT, m_App.GetInstance()))
		return false;
	return true;
}


bool CMainWindow::Show(int CmdShow, bool fForce)
{
	if (!m_fShowTitleBar || m_fCustomTitleBar)
		SetWindowStyle(GetWindowStyle() & ~WS_CAPTION, true);

	if (::ShowWindow(m_hwnd, !fForce && m_WindowPosition.fMaximized ? SW_SHOWMAXIMIZED : CmdShow))
		return false;

	Update();

	return true;
}


void CMainWindow::CreatePanel()
{
	const SIZE IconDrawSize = m_App.Panel.Form.GetIconDrawSize();
	LPCTSTR pszIconImage;
	int IconSize;
	if (IconDrawSize.cx <= 16 && IconDrawSize.cy <= 16) {
		pszIconImage = MAKEINTRESOURCE(IDB_PANELTABICONS16);
		IconSize = 16;
	} else {
		pszIconImage = MAKEINTRESOURCE(IDB_PANELTABICONS32);
		IconSize = 32;
	}
	const HBITMAP hbm = static_cast<HBITMAP>(::LoadImage(
		m_App.GetResourceInstance(), pszIconImage,
		IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
	m_App.Panel.Form.SetIconImage(hbm, IconSize, IconSize);
	::DeleteObject(hbm);

	m_App.Panel.Form.SetTabFont(m_App.PanelOptions.GetFont());
	m_App.Panel.Form.SetTabStyle(m_App.PanelOptions.GetTabStyle());
	m_App.Panel.Form.Create(m_hwnd, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

	CPanelForm::PageInfo PageInfo;

	m_App.Panel.InfoPanel.SetProgramInfoRichEdit(m_App.PanelOptions.GetProgramInfoUseRichEdit());
	m_App.Panel.InfoPanel.Create(m_App.Panel.Form.GetHandle(), WS_CHILD | WS_CLIPCHILDREN);
	m_App.Panel.InfoPanel.GetItem<CInformationPanel::CSignalLevelItem>()->ShowSignalLevel(
		!m_App.DriverOptions.IsNoSignalLevel(m_App.CoreEngine.GetDriverFileName()));
	PageInfo.pPage = &m_App.Panel.InfoPanel;
	PageInfo.pszTitle = TEXT("情報");
	PageInfo.ID = PANEL_ID_INFORMATION;
	PageInfo.Icon = 0;
	PageInfo.fVisible = true;
	m_App.Panel.Form.AddPage(PageInfo);

	m_App.Panel.ProgramListPanel.SetEPGDatabase(&m_App.EPGDatabase);
	m_App.Panel.ProgramListPanel.SetVisibleEventIcons(m_App.ProgramGuideOptions.GetVisibleEventIcons());
	m_App.Panel.ProgramListPanel.SetUseARIBSymbol(m_App.ProgramGuideOptions.GetUseARIBSymbol());
	m_App.Panel.ProgramListPanel.Create(m_App.Panel.Form.GetHandle(), WS_CHILD | WS_VSCROLL);
	PageInfo.pPage = &m_App.Panel.ProgramListPanel;
	PageInfo.pszTitle = TEXT("番組表");
	PageInfo.ID = PANEL_ID_PROGRAMLIST;
	PageInfo.Icon = 1;
	m_App.Panel.Form.AddPage(PageInfo);

	m_App.Panel.ChannelPanel.SetEPGDatabase(&m_App.EPGDatabase);
	m_App.Panel.ChannelPanel.SetLogoManager(&m_App.LogoManager);
	m_App.Panel.ChannelPanel.SetUseARIBSymbol(m_App.ProgramGuideOptions.GetUseARIBSymbol());
	m_App.Panel.ChannelPanel.Create(m_App.Panel.Form.GetHandle(), WS_CHILD | WS_VSCROLL);
	PageInfo.pPage = &m_App.Panel.ChannelPanel;
	PageInfo.pszTitle = TEXT("チャンネル");
	PageInfo.ID = PANEL_ID_CHANNEL;
	PageInfo.Icon = 2;
	m_App.Panel.Form.AddPage(PageInfo);

	m_App.Panel.ControlPanel.SetSendMessageWindow(m_hwnd);
	m_App.Panel.InitControlPanel();
	m_App.Panel.ControlPanel.Create(m_App.Panel.Form.GetHandle(), WS_CHILD);
	PageInfo.pPage = &m_App.Panel.ControlPanel;
	PageInfo.pszTitle = TEXT("操作");
	PageInfo.ID = PANEL_ID_CONTROL;
	PageInfo.Icon = 3;
	m_App.Panel.Form.AddPage(PageInfo);

	m_App.Panel.CaptionPanel.Create(m_App.Panel.Form.GetHandle(), WS_CHILD | WS_CLIPCHILDREN);
	PageInfo.pPage = &m_App.Panel.CaptionPanel;
	PageInfo.pszTitle = TEXT("字幕");
	PageInfo.ID = PANEL_ID_CAPTION;
	PageInfo.Icon = 4;
	m_App.Panel.Form.AddPage(PageInfo);

	m_App.PluginManager.RegisterPanelItems();

	m_App.PanelOptions.InitializePanelForm(&m_App.Panel.Form);
	if (m_App.ViewOptions.GetTitleBarFontEnabled())
		m_App.Panel.Frame.GetPanel()->SetTitleFont(m_App.ViewOptions.GetTitleBarFont());
	m_App.Panel.Frame.Create(
		m_hwnd,
		dynamic_cast<Layout::CSplitter*>(m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER)),
		CONTAINER_ID_PANEL, &m_App.Panel.Form, TEXT("パネル"));

	if (m_fCustomFrame) {
		HookWindows(m_App.Panel.Frame.GetPanel()->GetHandle());
	}
}


bool CMainWindow::InitializeViewer(BYTE VideoStreamType)
{
	const bool fEnableViewer = IsViewerEnabled();

	m_pCore->SetStatusBarTrace(true);

	if (m_Display.BuildViewer(VideoStreamType)) {
		const LibISDB::ViewerFilter *pViewer =
			m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();

		for (const auto &e : m_DirectShowFilterPropertyList) {
			m_pCore->SetCommandEnabledState(e.Command, pViewer->FilterHasProperty(e.Filter));
		}

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

	m_fEnablePlayback = false;

	m_pCore->SetCommandCheckedState(CM_DISABLEVIEWER, true);

	m_App.Panel.InfoPanel.ResetItem(CInformationPanel::ITEM_VIDEODECODER);
	m_App.Panel.InfoPanel.ResetItem(CInformationPanel::ITEM_VIDEORENDERER);
	m_App.Panel.InfoPanel.ResetItem(CInformationPanel::ITEM_AUDIODEVICE);

	return true;
}


bool CMainWindow::SetFullscreen(bool fFullscreen)
{
	if (fFullscreen) {
		if (::IsIconic(m_hwnd))
			::ShowWindow(m_hwnd, SW_RESTORE);
		if (!m_Fullscreen.Create(m_hwnd, &m_Display))
			return false;
	} else {
		ForegroundWindow(m_hwnd);
		m_Fullscreen.SendMessage(WM_CLOSE, 0, 0);
		LibISDB::ViewerFilter *pViewer = m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();
		if (pViewer != nullptr) {
			pViewer->SetViewStretchMode(m_App.VideoOptions.GetStretchMode());
		}
	}
	m_App.StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
	m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VIDEO);
	m_pCore->SetCommandCheckedState(CM_FULLSCREEN, fFullscreen);
	m_pCore->SetCommandCheckedState(
		CM_PANEL, fFullscreen ? m_Fullscreen.IsPanelVisible() : m_App.Panel.fShowPanelWindow);
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


void CMainWindow::ShowNotificationBar(
	LPCTSTR pszText, CNotificationBar::MessageType Type, DWORD Duration, bool fSkippable)
{
	m_NotificationBar.SetFont(m_App.NotificationBarOptions.GetNotificationBarFont());
	m_NotificationBar.Show(
		pszText, Type,
		std::max(static_cast<DWORD>(m_App.NotificationBarOptions.GetNotificationBarDuration()), Duration),
		fSkippable);
}


void CMainWindow::AdjustWindowSize(int Width, int Height, bool fScreenSize)
{
	RECT rcOld, rc;

	GetPosition(&rcOld);

	if (fScreenSize) {
		::SetRect(&rc, 0, 0, Width, Height);
		m_Display.GetViewWindow().CalcWindowRect(&rc);
		Width = rc.right - rc.left;
		Height = rc.bottom - rc.top;
		m_LayoutBase.GetScreenPosition(&rc);
		rc.right = rc.left + Width;
		rc.bottom = rc.top + Height;
		if (m_fShowTitleBar && m_fCustomTitleBar)
			m_TitleBarManager.ReserveArea(&rc, true);
		if (m_fShowSideBar)
			m_SideBarManager.ReserveArea(&rc, true);
		if (m_App.Panel.fShowPanelWindow && !m_App.Panel.IsFloating()) {
			Layout::CSplitter *pSplitter = dynamic_cast<Layout::CSplitter*>(
				m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));
			if (m_App.Panel.Frame.IsDockingVertical())
				rc.bottom += pSplitter->GetBarWidth() + pSplitter->GetPaneSize(CONTAINER_ID_PANEL);
			else
				rc.right += pSplitter->GetBarWidth() + pSplitter->GetPaneSize(CONTAINER_ID_PANEL);
		}
		if (m_fShowStatusBar)
			rc.bottom += m_App.StatusView.CalcHeight(rc.right - rc.left);
		if (m_fCustomFrame)
			::InflateRect(&rc, m_CustomFrameWidth, m_CustomFrameWidth);
		else
			m_pStyleScaling->AdjustWindowRect(m_hwnd, &rc);
	} else {
		rc.left = rcOld.left;
		rc.top = rcOld.top;
		rc.right = rc.left + Width;
		rc.bottom = rc.top + Height;
	}

	if (rc == rcOld)
		return;

	const HMONITOR hMonitor = ::MonitorFromRect(&rcOld, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	::GetMonitorInfo(hMonitor, &mi);

	if (m_App.ViewOptions.GetNearCornerResizeOrigin()) {
		if (std::abs(rcOld.left - mi.rcWork.left) > std::abs(rcOld.right - mi.rcWork.right)) {
			rc.left = rcOld.right - (rc.right - rc.left);
			rc.right = rcOld.right;
		}
		if (std::abs(rcOld.top - mi.rcWork.top) > std::abs(rcOld.bottom - mi.rcWork.bottom)) {
			rc.top = rcOld.bottom - (rc.bottom - rc.top);
			rc.bottom = rcOld.bottom;
		}
	}

	// ウィンドウがモニタの外に出ないようにする
	if (rcOld.left >= mi.rcWork.left && rcOld.top >= mi.rcWork.top
			&& rcOld.right <= mi.rcWork.right && rcOld.bottom <= mi.rcWork.bottom) {
		if (rc.right > mi.rcWork.right && rc.left > mi.rcWork.left)
			::OffsetRect(&rc, std::max(mi.rcWork.right - rc.right, mi.rcWork.left - rc.left), 0);
		if (rc.bottom > mi.rcWork.bottom && rc.top > mi.rcWork.top)
			::OffsetRect(&rc, 0, std::max(mi.rcWork.bottom - rc.bottom, mi.rcWork.top - rc.top));
	}

	SetPosition(&rc);
}


bool CMainWindow::ReadSettings(CSettings &Settings)
{
	int Left, Top, Width, Height, Value;
	bool f;

	GetPosition(&Left, &Top, &Width, &Height);
	Settings.Read(TEXT("WindowLeft"), &Left);
	Settings.Read(TEXT("WindowTop"), &Top);
	Settings.Read(TEXT("WindowWidth"), &Width);
	Settings.Read(TEXT("WindowHeight"), &Height);
	SetPosition(Left, Top, Width, Height);
	MoveToMonitorInside();
	if (Settings.Read(TEXT("WindowMaximize"), &f))
		SetMaximize(f);

	Settings.Read(TEXT("WindowSize.HD.Width"), &m_HDWindowSize.Width);
	Settings.Read(TEXT("WindowSize.HD.Height"), &m_HDWindowSize.Height);
	Settings.Read(TEXT("WindowSize.1Seg.Width"), &m_1SegWindowSize.Width);
	Settings.Read(TEXT("WindowSize.1Seg.Height"), &m_1SegWindowSize.Height);
	if (m_HDWindowSize.Width == Width && m_HDWindowSize.Height == Height)
		m_WindowSizeMode = WindowSizeMode::HD;
	else if (m_1SegWindowSize.Width == Width && m_1SegWindowSize.Height == Height)
		m_WindowSizeMode = WindowSizeMode::OneSeg;

	if (Settings.Read(TEXT("AlwaysOnTop"), &f))
		m_pCore->SetAlwaysOnTop(f);
	if (Settings.Read(TEXT("ShowStatusBar"), &f))
		SetStatusBarVisible(f);
	if (Settings.Read(TEXT("ShowTitleBar"), &f))
		SetTitleBarVisible(f);
	Settings.Read(TEXT("PopupTitleBar"), &m_fPopupTitleBar);
	if (Settings.Read(TEXT("PanelDockingIndex"), &Value)
			&& (Value == 0 || Value == 1))
		m_PanelPaneIndex = Value;
	Settings.Read(TEXT("PanelVerticalAlign"), &m_fPanelVerticalAlign);
	if (Settings.Read(TEXT("FullscreenPanelWidth"), &Value))
		m_Fullscreen.SetPanelWidth(Value);
	if (Settings.Read(TEXT("FullscreenPanelHeight"), &Value))
		m_Fullscreen.SetPanelHeight(Value);
	if (Settings.Read(TEXT("FullscreenPanelPlace"), &Value))
		m_Fullscreen.SetPanelPlace(static_cast<CPanelFrame::DockingPlace>(Value));
	if (Settings.Read(TEXT("ThinFrameWidth"), &Value))
		m_ThinFrameWidth = std::max(Value, 1);
	Value = FRAME_NORMAL;
	if (!Settings.Read(TEXT("FrameType"), &Value)) {
		if (Settings.Read(TEXT("ThinFrame"), &f) && f) // 以前のバージョンとの互換用
			Value = FRAME_CUSTOM;
	}
	SetCustomFrame(Value != FRAME_NORMAL, Value == FRAME_CUSTOM ? m_ThinFrameWidth : 0);
	if (!m_fCustomFrame && Settings.Read(TEXT("CustomTitleBar"), &f))
		SetCustomTitleBar(f);
	Settings.Read(TEXT("SplitTitleBar"), &m_fSplitTitleBar);
	if (Settings.Read(TEXT("ShowSideBar"), &f))
		SetSideBarVisible(f);
	Settings.Read(TEXT("AccurateClock"), &m_fAccurateClock);

	return true;
}


bool CMainWindow::WriteSettings(CSettings &Settings)
{
	int Left, Top, Width, Height;

	GetPosition(&Left, &Top, &Width, &Height);
	Settings.Write(TEXT("WindowLeft"), Left);
	Settings.Write(TEXT("WindowTop"), Top);
	Settings.Write(TEXT("WindowWidth"), Width);
	Settings.Write(TEXT("WindowHeight"), Height);
	Settings.Write(TEXT("WindowMaximize"), GetMaximize());
	Settings.Write(TEXT("WindowSize.HD.Width"), m_HDWindowSize.Width);
	Settings.Write(TEXT("WindowSize.HD.Height"), m_HDWindowSize.Height);
	Settings.Write(TEXT("WindowSize.1Seg.Width"), m_1SegWindowSize.Width);
	Settings.Write(TEXT("WindowSize.1Seg.Height"), m_1SegWindowSize.Height);
	Settings.Write(TEXT("AlwaysOnTop"), m_pCore->GetAlwaysOnTop());
	Settings.Write(TEXT("ShowStatusBar"), m_fShowStatusBar);
	Settings.Write(TEXT("ShowTitleBar"), m_fShowTitleBar);
	//Settings.Write(TEXT("PopupTitleBar"), m_fPopupTitleBar);
	Settings.Write(TEXT("PanelDockingIndex"), m_PanelPaneIndex);
	Settings.Write(TEXT("PanelVerticalAlign"), m_fPanelVerticalAlign);
	Settings.Write(TEXT("FullscreenPanelWidth"), m_Fullscreen.GetPanelWidth());
	Settings.Write(TEXT("FullscreenPanelHeight"), m_Fullscreen.GetPanelHeight());
	Settings.Write(TEXT("FullscreenPanelPlace"), static_cast<int>(m_Fullscreen.GetPanelPlace()));
	Settings.Write(
		TEXT("FrameType"),
		!m_fCustomFrame ? FRAME_NORMAL : (m_CustomFrameWidth == 0 ? FRAME_NONE : FRAME_CUSTOM));
	//Settings.Write(TEXT("ThinFrameWidth"), m_ThinFrameWidth);
	Settings.Write(TEXT("CustomTitleBar"), m_fCustomTitleBar);
	Settings.Write(TEXT("SplitTitleBar"), m_fSplitTitleBar);
	Settings.Write(TEXT("ShowSideBar"), m_fShowSideBar);

	return true;
}


bool CMainWindow::SetAlwaysOnTop(bool fTop)
{
	if (m_hwnd != nullptr) {
		::SetWindowPos(
			m_hwnd, fTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE);
		m_pCore->SetCommandCheckedState(CM_ALWAYSONTOP, fTop);
	}
	return true;
}


void CMainWindow::ShowPanel(bool fShow)
{
	if (m_App.Panel.fShowPanelWindow == fShow)
		return;

	m_App.Panel.fShowPanelWindow = fShow;

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
		m_pCore->SetCommandCheckedState(CM_PANEL, fShow);
}


void CMainWindow::SetStatusBarVisible(bool fVisible)
{
	if (m_fShowStatusBar != fVisible) {
		if (!m_pCore->GetFullscreen()) {
			m_fShowStatusBar = fVisible;
			if (m_hwnd != nullptr) {
				LockLayout();

				RECT rc;

				if (fVisible) {
					// 一瞬変な位置に出ないように見えない位置に移動
					RECT rcClient;
					::GetClientRect(m_App.StatusView.GetParent(), &rcClient);
					m_App.StatusView.GetPosition(&rc);
					m_App.StatusView.SetPosition(0, rcClient.bottom, rc.right - rc.left, rc.bottom - rc.top);
				}
				m_LayoutBase.SetContainerVisible(CONTAINER_ID_STATUS, fVisible);

				GetPosition(&rc);
				if (fVisible)
					rc.bottom += m_App.StatusView.GetHeight();
				else
					rc.bottom -= m_App.StatusView.GetHeight();
				SetPosition(&rc);

				UpdateLayout();

				m_pCore->SetCommandCheckedState(CM_STATUSBAR, m_fShowStatusBar);
			}
		}
	}
}


bool CMainWindow::ShowStatusBarItem(int ID, bool fShow)
{
	CStatusItem *pItem = m_App.StatusView.GetItemByID(ID);
	if (pItem == nullptr)
		return false;
	if (pItem->GetVisible() == fShow)
		return true;

	pItem->SetVisible(fShow);

	if (!m_fStatusBarInitialized)
		return true;

	if (m_fShowStatusBar)
		LockLayout();

	const int OldHeight = m_App.StatusView.GetHeight();
	m_App.StatusView.AdjustSize();
	const int NewHeight = m_App.StatusView.GetHeight();

	if (OldHeight != NewHeight) {
		const int Offset = NewHeight - OldHeight;
		RECT rc;

		if (m_fShowStatusBar) {
			GetPosition(&rc);
			rc.bottom += Offset;
			SetPosition(&rc);
		}
		if ((!m_fShowStatusBar && m_App.StatusView.GetVisible())
				|| (m_pCore->GetFullscreen() && m_Fullscreen.IsStatusBarVisible())) {
			m_App.StatusView.GetPosition(&rc);
			rc.top -= Offset;
			rc.bottom = rc.top + NewHeight;
			m_App.StatusView.SetPosition(&rc);
		}

		// ポップアップ表示されたサイドバーの位置の調整
		if (((!m_fShowSideBar && m_App.SideBar.GetVisible())
					|| (m_pCore->GetFullscreen() && m_Fullscreen.IsSideBarVisible()))
				&& m_App.SideBarOptions.GetPlace() == CSideBarOptions::PlaceType::Bottom) {
			m_App.SideBar.GetPosition(&rc);
			::OffsetRect(&rc, 0, -Offset);
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
	m_fStatusBarInitialized = true;
}


void CMainWindow::OnStatusBarTraceEnd()
{
	// 起動時に一時的に表示していたステータスバーを非表示にする
	if (!m_fShowStatusBar)
		ShowPopupStatusBar(false);
}


void CMainWindow::SetTitleBarVisible(bool fVisible)
{
	if (m_fShowTitleBar != fVisible) {
		m_fShowTitleBar = fVisible;
		if (m_hwnd != nullptr) {
			const bool fMaximize = GetMaximize();
			RECT rc;

			LockLayout();

			if (!fMaximize)
				GetPosition(&rc);
			if (!m_fCustomTitleBar)
				SetWindowStyle(GetWindowStyle()^WS_CAPTION, fMaximize);
			else if (!fVisible)
				m_LayoutBase.SetContainerVisible(CONTAINER_ID_TITLEBAR, false);
			if (!fMaximize) {
				int CaptionHeight;

				if (!m_fCustomTitleBar)
					CaptionHeight = m_pStyleScaling->GetScaledSystemMetrics(SM_CYCAPTION, false);
				else
					CaptionHeight = m_TitleBar.GetHeight();
				if (fVisible)
					rc.bottom += CaptionHeight;
				else
					rc.bottom -= CaptionHeight;
				::SetWindowPos(
					m_hwnd, nullptr, rc.left, rc.top,
					rc.right - rc.left, rc.bottom - rc.top,
					SWP_NOZORDER | SWP_FRAMECHANGED | SWP_DRAWFRAME);
			}
			if (m_fCustomTitleBar && fVisible)
				m_LayoutBase.SetContainerVisible(CONTAINER_ID_TITLEBAR, true);

			UpdateLayout();

			m_pCore->SetCommandCheckedState(CM_TITLEBAR, m_fShowTitleBar);
		}
	}
}


// タイトルバーを独自のものにするか設定
void CMainWindow::SetCustomTitleBar(bool fCustom)
{
	if (m_fCustomTitleBar != fCustom) {
		if (!fCustom && m_fCustomFrame)
			SetCustomFrame(false);
		m_fCustomTitleBar = fCustom;
		if (m_hwnd != nullptr) {
			if (m_fShowTitleBar) {
				if (!fCustom)
					m_LayoutBase.SetContainerVisible(CONTAINER_ID_TITLEBAR, false);
				m_pCore->UpdateTitle();
				SetWindowStyle(GetWindowStyle()^WS_CAPTION, true);
				if (fCustom)
					m_LayoutBase.SetContainerVisible(CONTAINER_ID_TITLEBAR, true);
			}
		}
	}
}


// タイトルバーをパネルで分割するか設定
void CMainWindow::SetSplitTitleBar(bool fSplit)
{
	if (m_fSplitTitleBar != fSplit) {
		m_fSplitTitleBar = fSplit;
		if (m_hwnd != nullptr)
			UpdateLayoutStructure();
	}
}


// ウィンドウ枠を独自のものにするか設定
void CMainWindow::SetCustomFrame(bool fCustomFrame, int Width)
{
	if (m_fCustomFrame != fCustomFrame || (fCustomFrame && m_CustomFrameWidth != Width)) {
		if (fCustomFrame && Width < 0)
			return;
		if (fCustomFrame && !m_fCustomTitleBar)
			SetCustomTitleBar(true);
		m_fCustomFrame = fCustomFrame;
		if (fCustomFrame)
			m_CustomFrameWidth = Width;
		if (m_hwnd != nullptr) {
			// 最大化状態でウィンドウ枠を変えるとおかしくなるので、元に戻された時に変える
			if (::IsZoomed(m_hwnd))
				m_fWindowFrameChanged = true;
			else
				UpdateWindowFrame();
			if (fCustomFrame) {
				HookWindows(m_LayoutBase.GetHandle());
				HookWindows(m_App.Panel.Form.GetHandle());
			}
		}
	}
}


void CMainWindow::SetSideBarVisible(bool fVisible)
{
	if (m_fShowSideBar != fVisible) {
		m_fShowSideBar = fVisible;
		if (m_hwnd != nullptr) {
			LockLayout();

			RECT rc;

			if (fVisible) {
				// 一瞬変な位置に出ないように見えない位置に移動
				RECT rcClient;
				::GetClientRect(m_App.SideBar.GetParent(), &rcClient);
				m_App.SideBar.GetPosition(&rc);
				m_App.SideBar.SetPosition(0, rcClient.bottom, rc.right - rc.left, rc.bottom - rc.top);
			}
			m_LayoutBase.SetContainerVisible(CONTAINER_ID_SIDEBAR, fVisible);

			GetPosition(&rc);
			RECT rcArea = rc;
			if (fVisible)
				m_SideBarManager.ReserveArea(&rcArea, true);
			else
				m_SideBarManager.AdjustArea(&rcArea);
			rc.right = rc.left + (rcArea.right - rcArea.left);
			rc.bottom = rc.top + (rcArea.bottom - rcArea.top);
			SetPosition(&rc);

			UpdateLayout();

			m_pCore->SetCommandCheckedState(CM_SIDEBAR, m_fShowSideBar);
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
	::ScreenToClient(::GetParent(hwnd), &pt);
	if (hwnd == m_TitleBar.GetHandle()) {
		if (!m_fShowTitleBar) {
			if (!m_fShowSideBar && m_App.SideBar.GetVisible()
					&& m_App.SideBarOptions.GetPlace() == CSideBarOptions::PlaceType::Top) {
				if (::RealChildWindowFromPoint(m_App.SideBar.GetParent(), pt) == m_App.SideBar.GetHandle())
					return false;
			}
			ShowPopupTitleBar(false);
			if (!m_fShowSideBar && m_App.SideBar.GetVisible())
				ShowPopupSideBar(false);
			return true;
		}
	} else if (hwnd == m_App.StatusView.GetHandle()) {
		if (!m_fShowStatusBar) {
			if (!m_fShowSideBar && m_App.SideBar.GetVisible()
					&& m_App.SideBarOptions.GetPlace() == CSideBarOptions::PlaceType::Bottom) {
				if (::RealChildWindowFromPoint(m_App.SideBar.GetParent(), pt) == m_App.SideBar.GetHandle())
					return false;
			}
			ShowPopupStatusBar(false);
			if (!m_fShowSideBar && m_App.SideBar.GetVisible())
				ShowPopupSideBar(false);
			return true;
		}
	} else if (hwnd == m_App.SideBar.GetHandle()) {
		if (!m_fShowSideBar) {
			ShowPopupSideBar(false);
			if (!m_fShowTitleBar && m_TitleBar.GetVisible()
					&& ::RealChildWindowFromPoint(m_TitleBar.GetParent(), pt) != m_TitleBar.GetHandle())
				ShowPopupTitleBar(false);
			if (!m_fShowStatusBar && m_App.StatusView.GetVisible()
					&& ::RealChildWindowFromPoint(m_App.StatusView.GetParent(), pt) != m_App.StatusView.GetHandle())
				ShowPopupStatusBar(false);
			return true;
		}
	}

	return false;
}


int CMainWindow::GetPanelPaneIndex() const
{
	const Layout::CSplitter *pSplitter =
		dynamic_cast<Layout::CSplitter*>(m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));

	if (pSplitter == nullptr)
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
	return m_pCore->GetFullscreen() ? IsFullscreenPanelVisible() : m_App.Panel.fShowPanelWindow;
}


void CMainWindow::OnPanelFloating(bool fFloating)
{
	if (fFloating)
		m_LayoutBase.SetContainerVisible(CONTAINER_ID_PANEL, false);
	AdjustWindowSizeOnDockPanel(!fFloating);
}


void CMainWindow::OnPanelDocking(CPanelFrame::DockingPlace Place)
{
	const bool fMove = m_LayoutBase.GetContainerVisible(CONTAINER_ID_PANEL);
	RECT rcCur, rcAdjusted;

	LockLayout();

	if (fMove) {
		GetPosition(&rcCur);
		rcAdjusted = rcCur;
		GetPanelDockingAdjustedPos(false, &rcAdjusted);
		m_LayoutBase.SetContainerVisible(CONTAINER_ID_PANEL, false);
	}

	m_fPanelVerticalAlign =
		Place == CPanelFrame::DockingPlace::Top || Place == CPanelFrame::DockingPlace::Bottom;

	UpdateLayoutStructure();

	Layout::CSplitter *pPanelSplitter = dynamic_cast<Layout::CSplitter*>(
		m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));

	const int Index =
		(Place == CPanelFrame::DockingPlace::Left || Place == CPanelFrame::DockingPlace::Top) ? 0 : 1;
	if (pPanelSplitter->IDToIndex(CONTAINER_ID_PANEL) != Index)
		pPanelSplitter->SwapPane();

	if (fMove) {
		GetPanelDockingAdjustedPos(true, &rcAdjusted);
		if (rcCur.right - rcCur.left != rcAdjusted.right - rcAdjusted.left
				|| rcCur.bottom - rcCur.top != rcAdjusted.bottom - rcAdjusted.top)
			SetPosition(&rcAdjusted);
		pPanelSplitter->SetPaneSize(
			CONTAINER_ID_PANEL,
			m_fPanelVerticalAlign ?
				m_App.Panel.Frame.GetDockingHeight() :
				m_App.Panel.Frame.GetDockingWidth());
		m_LayoutBase.SetContainerVisible(CONTAINER_ID_PANEL, true);
	}

	UpdateLayout();
}


bool CMainWindow::IsDarkMenu() const
{
	return GetStyleManager()->IsUseDarkMenu() && IsDarkAppModeSupported() && TVTest::IsDarkMode();
}


LRESULT CMainWindow::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		HANDLE_MSG(hwnd, WM_TIMER, OnTimer);
		HANDLE_MSG(hwnd, WM_GETMINMAXINFO, OnGetMinMaxInfo);

	case WM_NCCREATE:
		if (!OnNCCreate(reinterpret_cast<CREATESTRUCT*>(lParam)))
			return FALSE;
		break;

	case WM_SIZE:
		OnSizeChanged(static_cast<UINT>(wParam), LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_SIZING:
		if (OnSizeChanging(static_cast<UINT>(wParam), reinterpret_cast<LPRECT>(lParam)))
			return TRUE;
		break;

	case WM_MOVE:
		m_App.OSDManager.OnParentMove();
		return 0;

	case WM_ENTERSIZEMOVE:
		OnEnterSizeMove();
		return 0;

	case WM_EXITSIZEMOVE:
		OnExitSizeMove();
		return 0;

	case WM_WINDOWPOSCHANGING:
		{
			::DefWindowProc(hwnd, uMsg, wParam, lParam);

			const WINDOWPOS *pPos = reinterpret_cast<const WINDOWPOS*>(lParam);
			RECT rcOld, rcNew;

			::GetWindowRect(hwnd, &rcOld);
			rcNew = rcOld;
			if (!(pPos->flags & SWP_NOMOVE)) {
				rcNew.left = pPos->x;
				rcNew.top = pPos->y;
			}
			if (!(pPos->flags & SWP_NOSIZE)) {
				rcNew.right = rcNew.left + pPos->cx;
				rcNew.bottom = rcNew.top + pPos->cy;
			} else {
				rcNew.right = rcNew.left + (rcOld.right - rcOld.left);
				rcNew.bottom = rcNew.top + (rcOld.bottom - rcOld.top);
			}
			m_App.Panel.OnOwnerWindowPosChanging(&rcOld, &rcNew);
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
			m_App.CommandManager.InvokeCommand(
				m_App.OperationOptions.GetRightClickCommand(),
				CCommandManager::InvokeFlag::Mouse);
		}
		return 0;

	case WM_MBUTTONUP:
		if (m_pCore->GetFullscreen()) {
			m_Fullscreen.OnMButtonUp();
		} else {
			m_App.CommandManager.InvokeCommand(
				m_App.OperationOptions.GetMiddleClickCommand(),
				CCommandManager::InvokeFlag::Mouse);
		}
		return 0;

	case WM_LBUTTONDOWN:
		if (m_App.OperationOptions.GetDisplayDragMove()) {
			// 画面ドラッグによるウィンドウの移動

			m_ptDragStartPos.x = GET_X_LPARAM(lParam);
			m_ptDragStartPos.y = GET_Y_LPARAM(lParam);
			::ClientToScreen(hwnd, &m_ptDragStartPos);

			m_fDragMoveTrigger = true;
			::SetCapture(hwnd);
			return 0;
		}
		break;

	case WM_LBUTTONUP:
		m_fDragMoveTrigger = false;
		if (::GetCapture() == hwnd)
			::ReleaseCapture();
		return 0;

	case WM_NCLBUTTONDOWN:
		if (wParam == HTCAPTION && !m_App.ViewOptions.GetSupportAeroSnap()) {
			// Aero Snap 非対応時は自前で移動処理を行う

			ForegroundWindow(hwnd);

			m_fCaptionLButtonDown = true;
			m_fDragging = true;
			::SetCapture(hwnd);
			OnEnterSizeMove();
			return 0;
		}
		break;

	case WM_CAPTURECHANGED:
		m_fDragMoveTrigger = false;
		if (m_fCaptionLButtonDown) {
			m_fCaptionLButtonDown = false;
			OnExitSizeMove();
		}
		return 0;

	case WM_MOUSEMOVE:
		OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_MOVING:
		return OnMoving(reinterpret_cast<RECT*>(lParam));

	case WM_LBUTTONDBLCLK:
		m_App.CommandManager.InvokeCommand(
			m_App.OperationOptions.GetLeftDoubleClickCommand(),
			CCommandManager::InvokeFlag::Mouse);
		return 0;

	case WM_SETCURSOR:
		if (OnSetCursor(reinterpret_cast<HWND>(wParam), LOWORD(lParam), HIWORD(lParam)))
			return TRUE;
		break;

	case WM_SYSKEYDOWN:
		if (wParam != VK_F10)
			break;
		[[fallthrough]];
	case WM_KEYDOWN:
		if (OnKeyDown(wParam))
			return 0;
		break;

	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		{
			const bool fHorz = uMsg == WM_MOUSEHWHEEL;

			OnMouseWheel(wParam, lParam, fHorz);
			// WM_MOUSEHWHEEL は 1を返さないと繰り返し送られて来ないらしい
			return fHorz;
		}

	case WM_MEASUREITEM:
		{
			const MEASUREITEMSTRUCT *pmis = reinterpret_cast<const MEASUREITEMSTRUCT*>(lParam);

			if (pmis->itemID >= CM_ASPECTRATIO_FIRST && pmis->itemID <= CM_ASPECTRATIO_3D_LAST) {
				if (m_App.AspectRatioIconMenu.OnMeasureItem(hwnd, wParam, lParam))
					return TRUE;
				break;
			}
			if (m_App.ChannelMenu.OnMeasureItem(hwnd, wParam, lParam))
				return TRUE;
			if (m_App.FavoritesMenu.OnMeasureItem(hwnd, wParam, lParam))
				return TRUE;
			if (m_App.RecentChannelList.OnMeasureItem(hwnd, wParam, lParam))
				return TRUE;
		}
		break;

	case WM_DRAWITEM:
		if (m_App.AspectRatioIconMenu.OnDrawItem(hwnd, wParam, lParam))
			return TRUE;
		if (m_App.ChannelMenu.OnDrawItem(hwnd, wParam, lParam))
			return TRUE;
		if (m_App.FavoritesMenu.OnDrawItem(hwnd, wParam, lParam))
			return TRUE;
		if (m_App.RecentChannelList.OnDrawItem(hwnd, wParam, lParam))
			return TRUE;
		break;

// ウィンドウ枠を独自のものにするためのコード
	case WM_NCACTIVATE:
		if (m_fCustomFrame) {
			DrawCustomFrame(wParam != FALSE);
			return TRUE;
		}
		break;

	case WM_NCCALCSIZE:
		if (m_fCustomFrame) {
			if (wParam != 0) {
				NCCALCSIZE_PARAMS *pnccsp = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);

				// 最大化状態で起動された際、最初にここに来る時 NCCALCSIZE_PARAMS::rgrc[0] が
				// デフォルトのウィンドウ枠の分大きくされたサイズになっている
				if (::IsZoomed(hwnd)) {
					const HMONITOR hMonitor = ::MonitorFromRect(&pnccsp->rgrc[0], MONITOR_DEFAULTTONEAREST);
					MONITORINFO mi;
					mi.cbSize = sizeof(MONITORINFO);
					::GetMonitorInfo(hMonitor, &mi);
					pnccsp->rgrc[0] = mi.rcWork;
				} else {
					::InflateRect(&pnccsp->rgrc[0], -m_CustomFrameWidth, -m_CustomFrameWidth);
				}
			}
			return 0;
		}
		break;

	case WM_NCPAINT:
		if (m_fCustomFrame) {
			DrawCustomFrame(::GetActiveWindow() == hwnd);
			return 0;
		}
		break;

	case WM_NCHITTEST:
		if (m_fCustomFrame && !::IsZoomed(hwnd)) {
			const int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
			const int BorderWidth = m_CustomFrameWidth;
			int Code = HTNOWHERE;
			RECT rc;

			::GetWindowRect(hwnd, &rc);
			if (x >= rc.left && x < rc.left + BorderWidth) {
				if (y >= rc.top) {
					if (y < rc.top + BorderWidth)
						Code = HTTOPLEFT;
					else if (y < rc.bottom - BorderWidth)
						Code = HTLEFT;
					else if (y < rc.bottom)
						Code = HTBOTTOMLEFT;
				}
			} else if (x >= rc.right - BorderWidth && x < rc.right) {
				if (y >= rc.top) {
					if (y < rc.top + BorderWidth)
						Code = HTTOPRIGHT;
					else if (y < rc.bottom - BorderWidth)
						Code = HTRIGHT;
					else if (y < rc.bottom)
						Code = HTBOTTOMRIGHT;
				}
			} else if (y >= rc.top && y < rc.top + BorderWidth) {
				Code = HTTOP;
			} else if (y >= rc.bottom - BorderWidth && y < rc.bottom) {
				Code = HTBOTTOM;
			} else {
				if (::PtInRect(&rc, POINT{x, y}))
					Code = HTCLIENT;
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
		if (m_App.ChannelMenu.OnUninitMenuPopup(hwnd, wParam, lParam))
			return 0;
		if (m_App.FavoritesMenu.OnUninitMenuPopup(hwnd, wParam, lParam))
			return 0;
		if (m_App.RecentChannelList.OnUninitMenuPopup(hwnd, wParam, lParam))
			return 0;
		break;

	case WM_MENUSELECT:
		if (m_App.ChannelMenu.OnMenuSelect(hwnd, wParam, lParam))
			return 0;
		if (m_App.FavoritesMenu.OnMenuSelect(hwnd, wParam, lParam))
			return 0;
		if (m_App.RecentChannelList.OnMenuSelect(hwnd, wParam, lParam))
			return 0;
		break;

	case WM_ENTERMENULOOP:
		m_fNoHideCursor = true;
		return 0;

	case WM_EXITMENULOOP:
		m_fNoHideCursor = false;
		m_CursorTracker.Reset();
		return 0;

	case WM_COMMAND:
		m_App.CommandManager.InvokeCommand(LOWORD(wParam), CCommandManager::InvokeFlag::None);
		return 0;

	case WM_SYSCOMMAND:
		if (OnSysCommand(static_cast<UINT>(wParam)))
			return 0;
		break;

	case WM_APPCOMMAND:
		{
			const int Command = m_App.Accelerator.TranslateAppCommand(wParam, lParam);

			if (Command != 0) {
				SendCommand(Command);
				return TRUE;
			}
		}
		break;

	case WM_INPUT:
		return m_App.Accelerator.OnInput(hwnd, wParam, lParam);

	case WM_HOTKEY:
		{
			const int Command = m_App.Accelerator.TranslateHotKey(wParam, lParam);

			if (Command > 0)
				PostMessage(WM_COMMAND, Command, 0);
		}
		return 0;

	case WM_SETFOCUS:
		m_Display.GetDisplayBase().SetFocus();
		return 0;

	case WM_SETICON:
		if (wParam == ICON_SMALL) {
			m_TitleBar.SetIcon(reinterpret_cast<HICON>(lParam));
			m_Fullscreen.SendMessage(uMsg, wParam, lParam);
		}
		break;

	case WM_POWERBROADCAST:
		if (wParam == PBT_APMSUSPEND) {
			m_App.AddLog(TEXT("サスペンドへの移行通知を受けました。"));
			if (m_App.EpgCaptureManager.IsCapturing()) {
				m_App.EpgCaptureManager.EndCapture(CEpgCaptureManager::EndFlag::None);
			} else if (!m_pCore->GetStandby()) {
				StoreTunerResumeInfo();
			}
			SuspendViewer(ResumeInfo::ViewerSuspendFlag::Suspend);
			m_App.Core.CloseTuner();
			FinalizeViewer();
		} else if (wParam == PBT_APMRESUMESUSPEND) {
			m_App.AddLog(TEXT("サスペンドからの復帰通知を受けました。"));
			if (!m_pCore->GetStandby()) {
				// 遅延させた方がいいかも?
				ResumeTuner();
			}
			ResumeViewer(ResumeInfo::ViewerSuspendFlag::Suspend);
		}
		break;

	case WM_DWMCOMPOSITIONCHANGED:
		m_App.OSDOptions.OnDwmCompositionChanged();
		return 0;

	case WM_SETTINGCHANGE:
		if (m_fAllowDarkMode) {
			if (IsDarkModeSettingChanged(hwnd, uMsg, wParam, lParam)) {
				const bool fDarkMode = TVTest::IsDarkMode();

				m_App.AppEventManager.OnDarkModeChanged(fDarkMode);

				if (m_fDarkMode != fDarkMode) {
					if (SetWindowFrameDarkMode(hwnd, fDarkMode)) {
						m_fDarkMode = fDarkMode;
						m_App.AppEventManager.OnMainWindowDarkModeChanged(fDarkMode);
					}
				}
			}
		}
		return 0;

	case CAppMain::WM_INTERPROCESS:
		return m_App.ReceiveInterprocessMessage(hwnd, wParam, lParam);

	case WM_APP_SERVICEUPDATE:
		// サービスが更新された
		TRACE(TEXT("WM_APP_SERVICEUPDATE\n"));
		{
			CServiceUpdateInfo *pInfo = reinterpret_cast<CServiceUpdateInfo*>(lParam);

			if (pInfo->m_fStreamChanged) {
				if (m_Timer.IsTimerEnabled(TIMER_ID_RESETERRORCOUNT))
					m_Timer.BeginTimer(TIMER_ID_RESETERRORCOUNT, 2000);
			}

			if (!m_App.Core.IsChannelScanning()
					&& !pInfo->m_ServiceList.empty() && pInfo->m_CurService >= 0) {
				const CChannelInfo *pChInfo = m_App.ChannelManager.GetCurrentChannelInfo();
				const WORD TransportStreamID = pInfo->m_TransportStreamID;
				const WORD ServiceID = pInfo->m_ServiceList[pInfo->m_CurService].ServiceID;

				if (/*pInfo->m_fStreamChanged
						&& */TransportStreamID != 0 && ServiceID != 0
						&& !m_App.CoreEngine.IsNetworkDriver()
						&& (pChInfo == nullptr
							|| ((pChInfo->GetTransportStreamID() != 0
								&& pChInfo->GetTransportStreamID() != TransportStreamID)
								|| (pChInfo->GetServiceID() != 0
									&& pChInfo->GetServiceID() != ServiceID)))) {
					// 外部からチャンネル変更されたか、
					// BonDriverが開かれたときのデフォルトチャンネル
					m_App.Core.FollowChannelChange(TransportStreamID, ServiceID);
				}

				// チャンネルスキャンがされていない場合、番組表パネルに現在のサービスの情報が表示されるようにする
				if (pChInfo == nullptr || pChInfo->GetServiceID() == 0) {
					CChannelInfo Channel;
					if (m_App.Core.GetCurrentStreamChannelInfo(&Channel, true) && Channel.GetServiceID() != 0) {
						m_App.Panel.ProgramListPanel.SetCurrentChannel(&Channel);
						m_App.Panel.ProgramListPanel.SelectChannel(&Channel, !m_App.EpgOptions.IsEpgDataLoading());
					}
				}
			} else if (pInfo->m_fServiceListEmpty && pInfo->m_fStreamChanged
					&& !m_App.Core.IsChannelScanning()
					&& !m_App.EpgCaptureManager.IsCapturing()) {
				ShowNotificationBar(
					TEXT("このチャンネルは放送休止中です"),
					CNotificationBar::MessageType::Info);
			}

			delete pInfo;

			m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_SERVICE);

			if (wParam != 0)
				m_App.AppEventManager.OnServiceListUpdated();
			else
				m_App.AppEventManager.OnServiceInfoUpdated();
		}
		return 0;

	case WM_APP_SERVICECHANGED:
		TRACE(TEXT("WM_APP_SERVICECHANGED\n"));
		m_App.AddLog(TEXT("サービスを変更しました。(SID {})"), static_cast<int>(wParam));
		m_pCore->UpdateTitle();
		m_App.StatusView.UpdateItem(STATUS_ITEM_CHANNEL);
		m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_SERVICE);
		if (m_App.Panel.Form.GetCurPageID() == PANEL_ID_INFORMATION)
			m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_PROGRAMINFO);

		if (!IsMessageInQueue(hwnd, WM_APP_SERVICECHANGED)) {
			m_App.Core.NotifyTSProcessorNetworkChanged(
				CTSProcessorManager::FilterOpenFlag::NotifyError |
				CTSProcessorManager::FilterOpenFlag::NoUI);
		}
		return 0;

	case WM_APP_SERVICEINFOUPDATED:
		TRACE(TEXT("WM_APP_SERVICEINFOUPDATED\n"));
		m_pCore->UpdateTitle();
		m_App.StatusView.UpdateItem(STATUS_ITEM_CHANNEL);

		if (!IsMessageInQueue(hwnd, WM_APP_SERVICEINFOUPDATED)) {
			m_App.Core.NotifyTSProcessorNetworkChanged(
				CTSProcessorManager::FilterOpenFlag::NotifyError |
				CTSProcessorManager::FilterOpenFlag::NoUI);
		}
		return 0;

	case WM_APP_TRAYICON:
		switch (lParam) {
		case WM_RBUTTONDOWN:
			{
				CPopupMenu Menu(m_App.GetResourceInstance(), IDM_TRAY);

				Menu.EnableItem(CM_SHOW, m_pCore->GetStandby() || IsMinimizeToTray());
				// お約束が必要な理由は以下を参照
				// http://support.microsoft.com/kb/135788/en-us
				ForegroundWindow(hwnd);             // お約束
				Menu.Show(hwnd);
				::PostMessage(hwnd, WM_NULL, 0, 0); // お約束
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
			const WORD Port =
				m_App.ChannelManager.GetCurrentChannel() +
				(m_App.CoreEngine.IsUDPDriver() ? 1234 : 2230);
			return MAKELRESULT(Port, 0);
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
				&& !m_Resume.ViewerSuspendFlags
				&& !IsMessageInQueue(hwnd, WM_APP_VIDEOSTREAMTYPECHANGED)) {
			const BYTE StreamType = static_cast<BYTE>(wParam);

			if (StreamType == m_App.CoreEngine.GetVideoStreamType())
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
		m_VideoSizeChangedTimerCount = 0;
		m_Timer.BeginTimer(TIMER_ID_VIDEOSIZECHANGED, 500);
		if (m_AspectRatioResetTime != 0
				&& !m_pCore->GetFullscreen()
				&& IsViewerEnabled()
				&& TickTimeSpan(m_AspectRatioResetTime, ::GetTickCount()) < 6000) {
			if (AutoFitWindowToVideo())
				m_AspectRatioResetTime = 0;
		}
		return 0;

	case WM_APP_EPGLOADED:
		// EPGファイルが読み込まれた
		TRACE(TEXT("WM_APP_EPGLOADED\n"));
		m_App.Panel.EnableProgramListUpdate(true);
		if (IsPanelVisible()
				&& (m_App.Panel.Form.GetCurPageID() == PANEL_ID_PROGRAMLIST
					|| m_App.Panel.Form.GetCurPageID() == PANEL_ID_CHANNEL)) {
			m_App.Panel.UpdateContent();
		}
		return 0;

	case WM_APP_CONTROLLERFOCUS:
		// コントローラの操作対象が変わった
		TRACE(TEXT("WM_APP_CONTROLLERFOCUS\n"));
		m_App.ControllerManager.OnFocusChange(hwnd, wParam != 0);
		return 0;

	case WM_APP_PLUGINMESSAGE:
		// プラグインのメッセージの処理
		return CPlugin::OnPluginMessage(wParam, lParam);

	case WM_APP_SHOWNOTIFICATIONBAR:
		// 通知バーの表示
		TRACE(TEXT("WM_APP_SHOWNOTIFICATIONBAR"));
		{
			BlockLock Lock(m_NotificationLock);

			for (const NotificationInfo &Info : m_PendingNotificationList) {
				if (m_App.NotificationBarOptions.IsNotifyEnabled(Info.NotifyType))
					ShowNotificationBar(Info.Text.c_str(), Info.MessageType, Info.Duration, Info.fSkippable);
			}

			m_PendingNotificationList.clear();
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
			//const HRESULT hr = static_cast<HRESULT>(wParam);
			LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions Options;

			m_App.CoreEngine.GetSPDIFOptions(&Options);
			Options.Mode = LibISDB::DirectShow::AudioDecoderFilter::SPDIFMode::Disabled;
			m_App.CoreEngine.SetSPDIFOptions(Options);
			m_App.CoreEngine.ResetViewer();

			ShowMessage(
				TEXT("S/PDIFパススルー出力ができません。\n")
				TEXT("デバイスがパススルー出力に対応しているか、\n")
				TEXT("またパススルー出力できるように設定されているか確認してください。"),
				TEXT("S/PDIFパススルー出力エラー"),
				MB_OK | MB_ICONEXCLAMATION);
		}
		return 0;

	case WM_APP_UPDATECLOCK:
		m_App.StatusView.RedrawItem(STATUS_ITEM_CLOCK);
		return 0;

	case WM_ACTIVATEAPP:
		{
			const bool fActive = wParam != FALSE;

			m_App.ControllerManager.OnActiveChange(hwnd, fActive);
			if (fActive)
				m_App.BroadcastControllerFocusMessage(hwnd, fActive || m_fClosing, !fActive);
		}
		return 0;

	case WM_DISPLAYCHANGE:
		{
			LibISDB::ViewerFilter *pViewer = m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();
			if (pViewer != nullptr)
				pViewer->DisplayModeChanged();
		}
		break;

	case WM_DPICHANGED:
		{
			const int OldDPI = m_pStyleScaling->GetDPI();
			RECT rcOld;

			m_Display.GetViewWindow().GetPosition(&rcOld);
			OnDPIChanged(hwnd, wParam, lParam);

			const int NewDPI = m_pStyleScaling->GetDPI();
			if (OldDPI != NewDPI) {
				Layout::CSplitter *pSplitter = static_cast<Layout::CSplitter*>(
					m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));
				RECT rcNew;
				int Offset;

				m_Display.GetViewWindow().GetPosition(&rcNew);
				::OffsetRect(&rcNew, -rcNew.left, -rcNew.top);
				::OffsetRect(&rcOld, -rcOld.left, -rcOld.top);
				if (m_fPanelVerticalAlign)
					Offset = ::MulDiv(rcNew.right, rcOld.bottom, rcOld.right) - rcNew.bottom;
				else
					Offset = ::MulDiv(rcNew.bottom, rcOld.right, rcOld.bottom) - rcNew.right;
				const int ID = pSplitter->GetPane(1 - GetPanelPaneIndex())->GetID();
				pSplitter->SetPaneSize(ID, pSplitter->GetPaneSize(ID) + Offset);
			}
		}
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
		[[fallthrough]];
	case WM_DESTROY:
		OnDestroy();
		return 0;

	default:
		/*
		if (m_App.ControllerManager.HandleMessage(hwnd, uMsg, wParam, lParam))
			return 0;
		*/
		if (m_App.TaskTrayManager.HandleMessage(uMsg, wParam, lParam))
			return 0;
		if (m_App.TaskbarManager.HandleMessage(uMsg, wParam, lParam))
			return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


bool CMainWindow::OnNCCreate(const CREATESTRUCT *pcs)
{
	RegisterUIChild(&m_App.OSDManager);
	RegisterUIChild(&m_App.StatusView);
	RegisterUIChild(&m_App.SideBar);
	RegisterUIChild(&m_LayoutBase);
	RegisterUIChild(&m_TitleBar);
	RegisterUIChild(&m_NotificationBar);
	RegisterUIChild(&m_Display.GetViewWindow());

	SetStyleScaling(&m_StyleScaling);
	InitStyleScaling(m_hwnd, true);

	return true;
}


bool CMainWindow::OnCreate(const CREATESTRUCT *pcs)
{
	InitializeUI();

	// ウィンドウの角を丸くするかの設定
	// Windows 11 で標準のウィンドウ枠を使う場合のみ有効
	if (!m_Style.WindowCornerStyle.empty() && Util::OS::IsWindows11OrLater()) {
		static const struct {
			LPCWSTR pszStyle;
			DWM_WINDOW_CORNER_PREFERENCE CornerPreference;
		} CornerStyleList[] = {
			{TEXT("square"),      DWMWCP_DONOTROUND},
			{TEXT("round"),       DWMWCP_ROUND},
			{TEXT("small-round"), DWMWCP_ROUNDSMALL},
		};

		for (auto &Style : CornerStyleList) {
			if (StringUtility::IsEqualNoCase(m_Style.WindowCornerStyle, Style.pszStyle)) {
				::DwmSetWindowAttribute(
					m_hwnd, DWMWA_WINDOW_CORNER_PREFERENCE,
					&Style.CornerPreference, sizeof(DWMWA_WINDOW_CORNER_PREFERENCE));
				break;
			}
		}
	}

	if (m_Style.fAllowDarkMode && SetWindowAllowDarkMode(m_hwnd, true)) {
		m_fAllowDarkMode = true;
		if (TVTest::IsDarkMode()) {
			if (SetWindowFrameDarkMode(m_hwnd, true))
				m_fDarkMode = true;
		}
	}

	if ((pcs->style & WS_MINIMIZE) != 0)
		m_WindowState = WindowState::Minimized;
	else if ((pcs->style & WS_MAXIMIZE) != 0)
		m_WindowState = WindowState::Maximized;
	else
		m_WindowState = WindowState::Normal;

	RECT rc;
	GetClientRect(&rc);
	m_LayoutBase.SetPosition(&rc);
	m_LayoutBase.Create(
		m_hwnd, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

	m_Display.GetViewWindow().SetMargin(m_Style.ScreenMargin);
	m_Display.Create(m_LayoutBase.GetHandle(), IDC_VIEW, IDC_VIDEOCONTAINER, m_hwnd);
	m_Display.GetViewWindow().SetEventHandler(&m_ViewWindowEventHandler);
	m_Display.GetVideoContainer().SetEventHandler(&m_VideoContainerEventHandler);
	m_Display.GetDisplayBase().SetEventHandler(&m_DisplayBaseEventHandler);

	if (m_App.ViewOptions.GetTitleBarFontEnabled())
		m_TitleBar.SetFont(m_App.ViewOptions.GetTitleBarFont());
	m_TitleBar.Create(
		m_LayoutBase.GetHandle(),
		WS_CHILD | WS_CLIPSIBLINGS | (m_fShowTitleBar && m_fCustomTitleBar ? WS_VISIBLE : 0),
		0, IDC_TITLEBAR);
	m_TitleBar.SetEventHandler(&m_TitleBarManager);
	m_TitleBar.SetLabel(pcs->lpszName);
	m_TitleBar.SetIcon(m_TitleBar.IsIconDrawSmall() ? m_App.GetAppIconSmall() : m_App.GetAppIcon());
	m_TitleBar.SetMaximizeMode((pcs->style & WS_MAXIMIZE) != 0);

	m_App.StatusView.AddItem(new CChannelStatusItem);
	m_App.StatusView.AddItem(new CVideoSizeStatusItem);
	m_App.StatusView.AddItem(new CVolumeStatusItem);
	m_App.StatusView.AddItem(new CAudioChannelStatusItem);
	CRecordStatusItem *pRecordStatusItem = new CRecordStatusItem;
	pRecordStatusItem->ShowRemainTime(m_App.RecordOptions.GetShowRemainTime());
	m_App.StatusView.AddItem(pRecordStatusItem);
	m_App.StatusView.AddItem(new CCaptureStatusItem);
	m_App.StatusView.AddItem(new CErrorStatusItem);
	m_App.StatusView.AddItem(new CSignalLevelStatusItem);
	CClockStatusItem *pClockStatusItem = new CClockStatusItem;
	pClockStatusItem->SetTOT(m_App.StatusOptions.GetShowTOTTime());
	m_App.StatusView.AddItem(pClockStatusItem);
	CProgramInfoStatusItem *pProgramInfoStatusItem = new CProgramInfoStatusItem;
	pProgramInfoStatusItem->EnablePopupInfo(m_App.StatusOptions.IsPopupProgramInfoEnabled());
	pProgramInfoStatusItem->SetShowProgress(m_App.StatusOptions.GetShowEventProgress());
	pProgramInfoStatusItem->SetPopupInfoSize(
		m_App.StatusOptions.GetPopupEventInfoWidth(),
		m_App.StatusOptions.GetPopupEventInfoHeight());
	m_App.StatusView.AddItem(pProgramInfoStatusItem);
	m_App.StatusView.AddItem(new CBufferingStatusItem);
	m_App.StatusView.AddItem(new CTunerStatusItem);
	m_App.StatusView.AddItem(new CMediaBitRateStatusItem);
	m_App.StatusView.AddItem(new CFavoritesStatusItem);
	const Theme::CThemeManager ThemeManager(m_pCore->GetCurrentColorScheme());
	m_App.StatusView.SetItemTheme(&ThemeManager);
	m_App.StatusView.Create(
		m_LayoutBase.GetHandle(),
		//WS_CHILD | (m_fShowStatusBar ? WS_VISIBLE : 0) | WS_CLIPSIBLINGS,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
		0, IDC_STATUS);
	m_App.StatusView.SetEventHandler(&m_StatusViewEventHandler);
	m_App.StatusOptions.ApplyOptions();

	m_NotificationBar.Create(
		m_Display.GetVideoContainer().GetHandle(),
		WS_CHILD | WS_CLIPSIBLINGS);

	m_App.SideBarOptions.ApplySideBarOptions();
	m_App.SideBar.SetEventHandler(&m_SideBarManager);
	m_App.SideBar.Create(
		m_LayoutBase.GetHandle(),
		WS_CHILD | WS_CLIPSIBLINGS/* | (m_fShowSideBar ? WS_VISIBLE : 0)*/,
		0, IDC_SIDEBAR);
	m_App.SideBarOptions.SetSideBarImage();

	Layout::CWindowContainer *pWindowContainer;
	Layout::CSplitter *pSideBarSplitter = new Layout::CSplitter(CONTAINER_ID_SIDEBARSPLITTER);
	const CSideBarOptions::PlaceType SideBarPlace = m_App.SideBarOptions.GetPlace();
	const bool fSideBarVertical =
		SideBarPlace == CSideBarOptions::PlaceType::Left ||
		SideBarPlace == CSideBarOptions::PlaceType::Right;
	const int SideBarWidth = m_App.SideBar.GetBarWidth();
	pSideBarSplitter->SetStyle(
		Layout::CSplitter::StyleFlag::Fixed |
			(fSideBarVertical ? Layout::CSplitter::StyleFlag::Horz : Layout::CSplitter::StyleFlag::Vert));
	pSideBarSplitter->SetVisible(true);
	pWindowContainer = new Layout::CWindowContainer(CONTAINER_ID_VIEW);
	pWindowContainer->SetWindow(&m_Display.GetViewWindow());
	pWindowContainer->SetMinSize(32, 32);
	pWindowContainer->SetVisible(true);
	pSideBarSplitter->SetPane(0, pWindowContainer);
	pSideBarSplitter->SetAdjustPane(CONTAINER_ID_VIEW);
	pWindowContainer = new Layout::CWindowContainer(CONTAINER_ID_SIDEBAR);
	pWindowContainer->SetWindow(&m_App.SideBar);
	pWindowContainer->SetMinSize(SideBarWidth, SideBarWidth);
	pWindowContainer->SetVisible(/*m_fShowSideBar*/false);
	pSideBarSplitter->SetPane(1, pWindowContainer);
	pSideBarSplitter->SetPaneSize(CONTAINER_ID_SIDEBAR, SideBarWidth);
	if (SideBarPlace == CSideBarOptions::PlaceType::Left
			|| SideBarPlace == CSideBarOptions::PlaceType::Top)
		pSideBarSplitter->SwapPane();

	Layout::CSplitter *pTitleBarSplitter = new Layout::CSplitter(CONTAINER_ID_TITLEBARSPLITTER);
	pTitleBarSplitter->SetStyle(Layout::CSplitter::StyleFlag::Vert | Layout::CSplitter::StyleFlag::Fixed);
	pTitleBarSplitter->SetVisible(true);
	Layout::CWindowContainer *pTitleBarContainer = new Layout::CWindowContainer(CONTAINER_ID_TITLEBAR);
	pTitleBarContainer->SetWindow(&m_TitleBar);
	pTitleBarContainer->SetMinSize(0, m_TitleBar.GetHeight());
	pTitleBarContainer->SetVisible(m_fShowTitleBar && m_fCustomTitleBar);
	pTitleBarSplitter->SetPane(0, pTitleBarContainer);
	pTitleBarSplitter->SetPaneSize(CONTAINER_ID_TITLEBAR, m_TitleBar.GetHeight());

	Layout::CSplitter *pPanelSplitter = new Layout::CSplitter(CONTAINER_ID_PANELSPLITTER);
	pPanelSplitter->SetStyle(
		m_fPanelVerticalAlign ? Layout::CSplitter::StyleFlag::Vert : Layout::CSplitter::StyleFlag::Horz);
	pPanelSplitter->SetVisible(true);
	Layout::CWindowContainer *pPanelContainer = new Layout::CWindowContainer(CONTAINER_ID_PANEL);
	pPanelContainer->SetMinSize(32, 32);
	pPanelSplitter->SetPane(m_PanelPaneIndex, pPanelContainer);
	pPanelSplitter->SetPane(1 - m_PanelPaneIndex, pSideBarSplitter);
	pPanelSplitter->SetAdjustPane(CONTAINER_ID_SIDEBARSPLITTER);
	pTitleBarSplitter->SetPane(1, pPanelSplitter);
	pTitleBarSplitter->SetAdjustPane(CONTAINER_ID_PANELSPLITTER);

	Layout::CSplitter *pStatusSplitter = new Layout::CSplitter(CONTAINER_ID_STATUSSPLITTER);
	pStatusSplitter->SetStyle(Layout::CSplitter::StyleFlag::Vert | Layout::CSplitter::StyleFlag::Fixed);
	pStatusSplitter->SetVisible(true);
	pStatusSplitter->SetPane(0, pTitleBarSplitter);
	pStatusSplitter->SetAdjustPane(CONTAINER_ID_TITLEBARSPLITTER);
	pWindowContainer = new Layout::CWindowContainer(CONTAINER_ID_STATUS);
	pWindowContainer->SetWindow(&m_App.StatusView);
	pWindowContainer->SetMinSize(0, m_App.StatusView.GetHeight());
	pWindowContainer->SetVisible(m_fShowStatusBar);
	pStatusSplitter->SetPane(1, pWindowContainer);
	pStatusSplitter->SetPaneSize(CONTAINER_ID_STATUS, m_App.StatusView.GetHeight());

	m_LayoutBase.LockLayout();
	m_LayoutBase.SetTopContainer(pStatusSplitter);
	m_LayoutBase.UnlockLayout();
	UpdateLayoutStructure();

	// 起動状況を表示するために、起動時は常にステータスバーを表示する
	if (!m_fShowStatusBar)
		ShowPopupStatusBar(true);
	m_App.StatusView.SetSingleText(TEXT("起動中..."));

	m_App.OSDManager.Initialize();
	m_App.OSDManager.SetEventHandler(this);

	if (m_fCustomFrame) {
		CAeroGlass Aero;
		Aero.EnableNcRendering(m_hwnd, false);
		HookWindows(m_LayoutBase.GetHandle());
		//HookWindows(m_App.Panel.Form.GetHandle());
	}

	// IME無効化
	::ImmAssociateContext(m_hwnd, nullptr);
	::ImmAssociateContextEx(m_hwnd, nullptr, IACE_CHILDREN);

	m_App.MainMenu.Create(m_App.GetResourceInstance());

	m_App.CommandManager.AddEventListener(&m_CommandEventListener);

	m_App.AppCommand.InitializeCommandState();

	m_pCore->SetCommandCheckedState(CM_DISABLEVIEWER, !m_fEnablePlayback);

	m_pCore->SetCommandCheckedState(CM_TITLEBAR, m_fShowTitleBar);
	m_pCore->SetCommandCheckedState(CM_STATUSBAR, m_fShowStatusBar);
	m_pCore->SetCommandCheckedState(CM_SIDEBAR, m_fShowSideBar);

	for (const auto &e : m_DirectShowFilterPropertyList) {
		m_pCore->SetCommandEnabledState(e.Command, false);
	}

	const HMENU hSysMenu = ::GetSystemMenu(m_hwnd, FALSE);
	::InsertMenu(
		hSysMenu, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED,
		SC_ABOUT, TEXT("バージョン情報(&A)"));
	::InsertMenu(hSysMenu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);

	static const CIconMenu::ItemInfo AspectRatioMenuItems[] = {
		{CM_ASPECTRATIO_DEFAULT,    MAKEINTRESOURCE(IDI_PANSCAN_AUTO)},
		{CM_ASPECTRATIO_16x9,       MAKEINTRESOURCE(IDI_PANSCAN_16x9)},
		{CM_ASPECTRATIO_LETTERBOX,  MAKEINTRESOURCE(IDI_PANSCAN_LETTERBOX)},
		{CM_ASPECTRATIO_WINDOWBOX,  MAKEINTRESOURCE(IDI_PANSCAN_WINDOWBOX)},
		{CM_ASPECTRATIO_PILLARBOX,  MAKEINTRESOURCE(IDI_PANSCAN_PILLARBOX)},
		{CM_ASPECTRATIO_4x3,        MAKEINTRESOURCE(IDI_PANSCAN_4x3)},
		{CM_ASPECTRATIO_32x9,       MAKEINTRESOURCE(IDI_PANSCAN_32x9)},
		{CM_ASPECTRATIO_16x9_LEFT,  MAKEINTRESOURCE(IDI_PANSCAN_16x9_LEFT)},
		{CM_ASPECTRATIO_16x9_RIGHT, MAKEINTRESOURCE(IDI_PANSCAN_16x9_RIGHT)},
		{CM_FRAMECUT,               MAKEINTRESOURCE(IDI_PANSCAN_TOUCHOUTSIDE)},
		{CM_PANANDSCANOPTIONS,      MAKEINTRESOURCE(IDI_PANSCAN_OPTIONS)},
	};
	const HMENU hmenuAspectRatio = m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_ASPECTRATIO);
	m_App.AspectRatioIconMenu.Initialize(
		hmenuAspectRatio, m_App.GetInstance(),
		AspectRatioMenuItems, lengthof(AspectRatioMenuItems));
	if (m_pCore->GetAspectRatioType() < CUICore::ASPECTRATIO_CUSTOM_FIRST) {
		m_pCore->SetCommandRadioCheckedState(
			CM_ASPECTRATIO_FIRST, CM_ASPECTRATIO_3D_LAST,
			CM_ASPECTRATIO_FIRST + m_pCore->GetAspectRatioType());
	}
	m_DefaultAspectRatioMenuItemCount = ::GetMenuItemCount(hmenuAspectRatio);

	if (!m_App.TaskbarOptions.GetUseUniqueAppID())
		m_App.TaskbarManager.SetAppID(m_App.TaskbarOptions.GetAppID().c_str());
	m_App.TaskbarManager.Initialize(m_hwnd);

	m_App.NotifyBalloonTip.Initialize(m_hwnd);

	LibISDB::ViewerFilter *pViewer = m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();
	if (pViewer != nullptr) {
		pViewer->SetViewStretchMode(
			(pcs->style & WS_MAXIMIZE) != 0 ?
				m_App.VideoOptions.GetMaximizeStretchMode() :
				m_App.VideoOptions.GetStretchMode());
	}

	m_App.EpgCaptureManager.SetEventHandler(&m_EpgCaptureEventHandler);

	if (GetStyleManager()->IsUseDarkMenu() && IsDarkAppModeSupported())
		m_hHook = ::SetWindowsHookEx(WH_CALLWNDPROC, HookProc, nullptr, ::GetCurrentThreadId());

	m_Timer.InitializeTimer(m_hwnd);
	m_Timer.BeginTimer(TIMER_ID_UPDATE, UPDATE_TIMER_INTERVAL);

	m_fShowCursor = true;
	m_CursorTracker.Reset();
	if (m_App.ViewOptions.GetHideCursor())
		m_Timer.BeginTimer(TIMER_ID_HIDECURSOR, HIDE_CURSOR_DELAY);

	m_fCreated = true;

	return true;
}


void CMainWindow::OnDestroy()
{
	m_ClockUpdateTimer.End();

	RECT rc;
	GetPosition(&rc);
	if (m_WindowSizeMode == WindowSizeMode::OneSeg)
		m_1SegWindowSize = rc;
	else
		m_HDWindowSize = rc;

	m_App.SetEnablePlaybackOnStart(m_fEnablePlayback);
	m_PanelPaneIndex = GetPanelPaneIndex();

	const CProgramInfoStatusItem *pProgramInfoStatusItem =
		dynamic_cast<CProgramInfoStatusItem*>(m_App.StatusView.GetItemByID(STATUS_ITEM_PROGRAMINFO));
	if (pProgramInfoStatusItem != nullptr) {
		int Width, Height;
		pProgramInfoStatusItem->GetPopupInfoSize(&Width, &Height);
		m_App.StatusOptions.SetPopupEventInfoSize(Width, Height);
	}

	m_App.HtmlHelpClass.Finalize();
	m_pCore->PreventDisplaySave(false);

	m_App.EpgCaptureManager.SetEventHandler(nullptr);

	if (m_hHook != nullptr) {
		::UnhookWindowsHookEx(m_hHook);
		m_hHook = nullptr;
	}

	m_App.Finalize();

	CBasicWindow::OnDestroy();
}


void CMainWindow::OnSizeChanged(UINT State, int Width, int Height)
{
	// WM_NCCREATE で EnableNonClientDpiScaling を呼んだ時に WM_SIZE が送られてくるため、
	// まだ WM_CREATE を処理してない場合は何もしないようにする
	if (!m_fCreated)
		return;

	const bool fMinimized = State == SIZE_MINIMIZED;
	const bool fMaximized = State == SIZE_MAXIMIZED;
	WindowState NewState;
	if (fMinimized)
		NewState = WindowState::Minimized;
	else if (fMaximized)
		NewState = WindowState::Maximized;
	else
		NewState = WindowState::Normal;

	if (fMinimized) {
		m_App.OSDManager.ClearOSD();
		m_App.OSDManager.Reset();
		m_App.TaskTrayManager.SetStatus(
			CTaskTrayManager::StatusFlag::Minimized,
			CTaskTrayManager::StatusFlag::Minimized);
		if (m_App.ViewOptions.GetDisablePreviewWhenMinimized()) {
			SuspendViewer(ResumeInfo::ViewerSuspendFlag::Minimize);
		}
	} else if (!!(m_App.TaskTrayManager.GetStatus() & CTaskTrayManager::StatusFlag::Minimized)) {
		SetWindowVisible();
	}

	if ((m_fCustomFrame && fMaximized) || m_fWindowRegionSet)
		SetMaximizedRegion(fMaximized);
	if (m_fWindowFrameChanged && !fMaximized && m_WindowState == WindowState::Maximized)
		UpdateWindowFrame();

	if (m_WindowState != NewState)
		m_pCore->UpdateTitle();

	m_WindowState = NewState;

	m_TitleBar.SetMaximizeMode(fMaximized);

	if (m_fLockLayout || fMinimized)
		return;

	m_LayoutBase.SetPosition(0, 0, Width, Height);

	if (!m_pCore->GetFullscreen()) {
		LibISDB::ViewerFilter *pViewer = m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();
		if (pViewer != nullptr) {
			if (State == SIZE_MAXIMIZED) {
				pViewer->SetViewStretchMode(m_App.VideoOptions.GetMaximizeStretchMode());
			} else if (State == SIZE_RESTORED) {
				pViewer->SetViewStretchMode(m_App.VideoOptions.GetStretchMode());
			}
		}
	}

	m_App.StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
	m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VIDEO);
}


bool CMainWindow::OnSizeChanging(UINT Edge, RECT *pRect)
{
	RECT rcOld;
	bool fResizePanel = false, fChanged = false;

	GetPosition(&rcOld);

	if (m_fEnterSizeMove) {
		if (::GetKeyState(VK_CONTROL) < 0)
			fResizePanel = true;
		if (m_fResizePanel != fResizePanel) {
			Layout::CSplitter *pSplitter = dynamic_cast<Layout::CSplitter*>(
				m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));

			if (fResizePanel)
				pSplitter->SetAdjustPane(CONTAINER_ID_PANEL);
			else
				pSplitter->SetAdjustPane(pSplitter->GetPane(!pSplitter->IDToIndex(CONTAINER_ID_PANEL))->GetID());
			m_fResizePanel = fResizePanel;
		}
	}

	bool fKeepRatio = false;
	if (!fResizePanel) {
		fKeepRatio = m_App.ViewOptions.GetAdjustAspectResizing();
		if (::GetKeyState(VK_SHIFT) < 0)
			fKeepRatio = !fKeepRatio;
	}
	if (fKeepRatio) {
		const LibISDB::ViewerFilter *pViewer = m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();
		int XAspect, YAspect;

		if (pViewer != nullptr && pViewer->GetEffectiveAspectRatio(&XAspect, &YAspect)) {
			RECT rcWindow, rcClient;

			GetPosition(&rcWindow);
			GetClientRect(&rcClient);
			m_Display.GetViewWindow().CalcClientRect(&rcClient);
			if (m_fShowStatusBar)
				rcClient.bottom -= m_App.StatusView.GetHeight();
			if (m_fShowTitleBar && m_fCustomTitleBar)
				m_TitleBarManager.AdjustArea(&rcClient);
			if (m_fShowSideBar)
				m_SideBarManager.AdjustArea(&rcClient);
			if (m_App.Panel.fShowPanelWindow && !m_App.Panel.IsFloating()) {
				Layout::CSplitter *pSplitter = dynamic_cast<Layout::CSplitter*>(
					m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));
				if (m_App.Panel.Frame.IsDockingVertical())
					rcClient.bottom -= pSplitter->GetPaneSize(CONTAINER_ID_PANEL) + pSplitter->GetBarWidth();
				else
					rcClient.right -= pSplitter->GetPaneSize(CONTAINER_ID_PANEL) + pSplitter->GetBarWidth();
			}
			::OffsetRect(&rcClient, -rcClient.left, -rcClient.top);
			if (rcClient.right <= 0 || rcClient.bottom <= 0)
				return false;
			const int XMargin = (rcWindow.right - rcWindow.left) - rcClient.right;
			const int YMargin = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
			int Width = (pRect->right - pRect->left) - XMargin;
			int Height = (pRect->bottom - pRect->top) - YMargin;
			if (Width <= 0 || Height <= 0)
				return false;
			if (Edge == WMSZ_LEFT || Edge == WMSZ_RIGHT)
				Height = Width * YAspect / XAspect;
			else if (Edge == WMSZ_TOP || Edge == WMSZ_BOTTOM)
				Width = Height * XAspect / YAspect;
			else if (Width * YAspect < Height * XAspect)
				Width = Height * XAspect / YAspect;
			else if (Width * YAspect > Height * XAspect)
				Height = Width * YAspect / XAspect;
			if (Edge == WMSZ_LEFT || Edge == WMSZ_TOPLEFT || Edge == WMSZ_BOTTOMLEFT)
				pRect->left = pRect->right - (Width + XMargin);
			else
				pRect->right = pRect->left + Width + XMargin;
			if (Edge == WMSZ_TOP || Edge == WMSZ_TOPLEFT || Edge == WMSZ_TOPRIGHT)
				pRect->top = pRect->bottom - (Height + YMargin);
			else
				pRect->bottom = pRect->top + Height + YMargin;
			fChanged = true;
		}
	}

	return fChanged;
}


void CMainWindow::OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO pmmi)
{
	SIZE sz;
	RECT rc;

	m_LayoutBase.GetMinSize(&sz);
	::SetRect(&rc, 0, 0, sz.cx, sz.cy);
	m_StyleScaling.AdjustWindowRect(m_hwnd, &rc);
	pmmi->ptMinTrackSize.x = rc.right - rc.left;
	pmmi->ptMinTrackSize.y = rc.bottom - rc.top;

	if (!m_fShowTitleBar || m_fCustomTitleBar) {
		WINDOWPLACEMENT wp;

		wp.length = sizeof(WINDOWPLACEMENT);
		::GetWindowPlacement(m_hwnd, &wp);

		const HMONITOR hMonitor = ::MonitorFromRect(&wp.rcNormalPosition, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi;

		mi.cbSize = sizeof(MONITORINFO);
		if (::GetMonitorInfo(hMonitor, &mi)) {
			RECT Border;

			if (m_fCustomFrame) {
				Border.left = Border.top = Border.right = Border.bottom = m_CustomFrameWidth;
			} else {
				RECT rcClient, rcWindow;

				::GetClientRect(hwnd, &rcClient);
				MapWindowRect(hwnd, nullptr, &rcClient);
				::GetWindowRect(hwnd, &rcWindow);
				Border.left = rcClient.left - rcWindow.left;
				Border.top = rcClient.top - rcWindow.top;
				Border.right = rcWindow.right - rcClient.right;
				Border.bottom = rcWindow.bottom - rcClient.bottom;
			}

			pmmi->ptMaxSize.x = (mi.rcWork.right - mi.rcWork.left) + Border.left + Border.right;
			pmmi->ptMaxSize.y = (mi.rcWork.bottom - mi.rcWork.top) + Border.top + Border.bottom;
			pmmi->ptMaxPosition.x = mi.rcWork.left - mi.rcMonitor.left - Border.left;
			pmmi->ptMaxPosition.y = mi.rcWork.top - mi.rcMonitor.top - Border.top;
			if (::IsZoomed(hwnd)) {
				// ウィンドウのあるモニタがプライマリモニタよりも大きい場合、
				// ptMaxTrackSize を設定しないとサイズがおかしくなる
				pmmi->ptMaxTrackSize = pmmi->ptMaxSize;
			} else {
				pmmi->ptMaxTrackSize.x = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
				pmmi->ptMaxTrackSize.y = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
			}
		}
	}
}


void CMainWindow::OnEnterSizeMove()
{
	m_fEnterSizeMove = true;
	m_fResizePanel = false;

	// リサイズ開始時のカーソル位置とウィンドウ位置を記憶
	::GetCursorPos(&m_ptDragStartPos);
	::GetWindowRect(m_hwnd, &m_rcDragStart);

	if (m_fDragging) {
		// ドラッグ中はカーソルが消えないようにする
		m_Timer.EndTimer(TIMER_ID_HIDECURSOR);
		//::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
		ShowCursor(true);
	}
}


void CMainWindow::OnExitSizeMove()
{
	m_fEnterSizeMove = false;

	if (m_fResizePanel) {
		m_fResizePanel = false;

		Layout::CSplitter *pSplitter = dynamic_cast<Layout::CSplitter*>(
			m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));
		pSplitter->SetAdjustPane(pSplitter->GetPane(!pSplitter->IDToIndex(CONTAINER_ID_PANEL))->GetID());
	}

	if (m_fDragging) {
		m_fDragging = false;
		if (m_App.ViewOptions.GetHideCursor())
			m_Timer.BeginTimer(TIMER_ID_HIDECURSOR, HIDE_CURSOR_DELAY);
	}

	m_fDragMoveTrigger = false;
	m_fCaptionLButtonDown = false;

	m_TitleBarManager.EndDrag();
}


bool CMainWindow::OnMoving(RECT *pPos)
{
	POINT pt;
	::GetCursorPos(&pt);

	// カーソル位置から移動後のウィンドウ位置を求める
	pPos->left = m_rcDragStart.left + (pt.x - m_ptDragStartPos.x);
	pPos->top = m_rcDragStart.top + (pt.y - m_ptDragStartPos.y);
	pPos->right = pPos->left + (m_rcDragStart.right - m_rcDragStart.left);
	pPos->bottom = pPos->top + (m_rcDragStart.bottom - m_rcDragStart.top);

	// ウィンドウと画面の端に吸着させる
	bool fSnap = m_App.ViewOptions.GetSnapAtWindowEdge();
	if (::GetKeyState(VK_SHIFT) < 0)
		fSnap = !fSnap;
	if (fSnap) {
		SnapWindow(
			m_hwnd, pPos,
			m_App.ViewOptions.GetSnapAtWindowEdgeMargin(),
			m_App.Panel.IsAttached() ? nullptr : m_App.Panel.Frame.GetHandle());
	}

	return true;
}


void CMainWindow::OnMouseMove(int x, int y)
{
	if (m_fDragMoveTrigger) {
		POINT pt = {x, y};
		::ClientToScreen(m_hwnd, &pt);
		if (pt.x != m_ptDragStartPos.x || pt.y != m_ptDragStartPos.y) {
			m_fDragMoveTrigger = false;
			m_fDragging = true;
			::ReleaseCapture();
			::SendMessage(m_hwnd, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(pt.x, pt.y));
		}
	} else if (m_fCaptionLButtonDown) {
		RECT rc;
		::GetWindowRect(m_hwnd, &rc);
		if (OnMoving(&rc))
			SetPosition(&rc);
	} else if (!m_pCore->GetFullscreen()) {
		const POINT pt = {x, y};

		if (m_CursorTracker.GetLastCursorPos() == pt) {
			if (m_App.ViewOptions.GetHideCursor())
				m_Timer.BeginTimer(TIMER_ID_HIDECURSOR, HIDE_CURSOR_DELAY);
			return;
		}

		RECT rcClient, rcTitle, rcStatus, rcSideBar, rc;
		bool fShowTitleBar = false, fShowStatusBar = false, fShowSideBar = false;

		m_Display.GetViewWindow().GetClientRect(&rcClient);
		MapWindowRect(m_Display.GetViewWindow().GetHandle(), m_LayoutBase.GetHandle(), &rcClient);
		if (!m_fShowTitleBar && m_fPopupTitleBar) {
			rc = rcClient;
			m_TitleBarManager.Layout(&rc, &rcTitle);
			if (::PtInRect(&rcTitle, pt))
				fShowTitleBar = true;
		}
		if (!m_fShowStatusBar && m_App.StatusOptions.GetShowPopup()) {
			rcStatus = rcClient;
			rcStatus.top = rcStatus.bottom - m_App.StatusView.CalcHeight(rcClient.right - rcClient.left);
			if (::PtInRect(&rcStatus, pt))
				fShowStatusBar = true;
		}
		if (!m_fShowSideBar && m_App.SideBarOptions.ShowPopup()) {
			switch (m_App.SideBarOptions.GetPlace()) {
			case CSideBarOptions::PlaceType::Left:
			case CSideBarOptions::PlaceType::Right:
				if (!fShowStatusBar && !fShowTitleBar) {
					m_SideBarManager.Layout(&rcClient, &rcSideBar);
					if (::PtInRect(&rcSideBar, pt))
						fShowSideBar = true;
				}
				break;
			case CSideBarOptions::PlaceType::Top:
				if (!m_fShowTitleBar && m_fPopupTitleBar)
					rcClient.top = rcTitle.bottom;
				m_SideBarManager.Layout(&rcClient, &rcSideBar);
				if (::PtInRect(&rcSideBar, pt)) {
					fShowSideBar = true;
					if (!m_fShowTitleBar && m_fPopupTitleBar)
						fShowTitleBar = true;
				}
				break;
			case CSideBarOptions::PlaceType::Bottom:
				if (!m_fShowStatusBar && m_App.StatusOptions.GetShowPopup())
					rcClient.bottom = rcStatus.top;
				m_SideBarManager.Layout(&rcClient, &rcSideBar);
				if (::PtInRect(&rcSideBar, pt)) {
					fShowSideBar = true;
					if (!m_fShowStatusBar && m_App.StatusOptions.GetShowPopup())
						fShowStatusBar = true;
				}
				break;
			}
		}

		if (fShowTitleBar) {
			if (!m_TitleBar.GetVisible())
				ShowPopupTitleBar(true);
		} else if (!m_fShowTitleBar && m_TitleBar.GetVisible()) {
			ShowPopupTitleBar(false);
		}
		if (fShowStatusBar) {
			if (!m_App.StatusView.GetVisible())
				ShowPopupStatusBar(true);
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

		if (m_CursorTracker.OnCursorMove(x, y)) {
			if (!m_fShowCursor)
				ShowCursor(true);
		}
		if (m_App.ViewOptions.GetHideCursor()) {
			if (fShowTitleBar || fShowStatusBar || fShowSideBar)
				m_Timer.EndTimer(TIMER_ID_HIDECURSOR);
			else
				m_Timer.BeginTimer(TIMER_ID_HIDECURSOR, HIDE_CURSOR_DELAY);
		}
	} else {
		m_Fullscreen.OnMouseMove();
	}
}


bool CMainWindow::OnSetCursor(HWND hwndCursor, int HitTestCode, int MouseMessage)
{
	if (HitTestCode == HTCLIENT && MouseMessage != 0) {
		if (hwndCursor == m_Display.GetVideoContainer().GetHandle()
				|| hwndCursor == m_Display.GetViewWindow().GetHandle()
				|| hwndCursor == m_NotificationBar.GetHandle()
				|| CPseudoOSD::IsPseudoOSD(hwndCursor)) {
			::SetCursor(m_fShowCursor ?::LoadCursor(nullptr, IDC_ARROW) : nullptr);
			return true;
		}
	}

	return false;
}


bool CMainWindow::OnKeyDown(WPARAM KeyCode)
{
	const CChannelInput::KeyDownResult Result = m_ChannelInput.OnKeyDown(KeyCode);

	if (Result != CChannelInput::KeyDownResult::NotProcessed) {
		switch (Result) {
		case CChannelInput::KeyDownResult::Completed:
			EndChannelNoInput(true);
			break;

		case CChannelInput::KeyDownResult::Cancelled:
			EndChannelNoInput(false);
			break;

		case CChannelInput::KeyDownResult::Begin:
		case CChannelInput::KeyDownResult::Continue:
			OnChannelNoInput();
			break;
		}

		return true;
	}

	if (KeyCode >= VK_F13 && KeyCode <= VK_F24
			&& (::GetKeyState(VK_SHIFT) < 0 || ::GetKeyState(VK_CONTROL) < 0)
			&& !m_App.ControllerManager.IsControllerEnabled(TEXT("HDUS Remocon"))) {
		ShowMessage(
			TEXT("リモコンを使用するためには、メニューの [プラグイン] -> [HDUSリモコン] でリモコンを有効にしてください。"),
			TEXT("お知らせ"), MB_OK | MB_ICONINFORMATION);
	}

	return false;
}


bool CMainWindow::HandleCommand(CCommandManager::InvokeParameters &Params)
{
	switch (Params.ID) {
	case CM_REBUILDVIEWER:
		InitializeViewer();
		return true;

	case CM_DISABLEVIEWER:
		m_pCore->EnableViewer(!m_fEnablePlayback);
		return true;

	case CM_PANEL:
		if (m_pCore->GetFullscreen()) {
			m_Fullscreen.ShowPanel(!m_Fullscreen.IsPanelVisible());
		} else {
			ShowPanel(!m_App.Panel.fShowPanelWindow);
		}
		return true;

	case CM_PROGRAMGUIDE:
		ShowProgramGuide(!m_App.Epg.fShowProgramGuide);
		return true;

	case CM_STATUSBAR:
		SetStatusBarVisible(!m_fShowStatusBar);
		return true;

	case CM_TITLEBAR:
		SetTitleBarVisible(!m_fShowTitleBar);
		return true;

	case CM_SIDEBAR:
		SetSideBarVisible(!m_fShowSideBar);
		return true;

	case CM_WINDOWFRAME_NORMAL:
		SetCustomFrame(false);
		return true;

	case CM_WINDOWFRAME_CUSTOM:
		SetCustomFrame(true, m_ThinFrameWidth);
		return true;

	case CM_WINDOWFRAME_NONE:
		SetCustomFrame(true, 0);
		return true;

	case CM_CUSTOMTITLEBAR:
		SetCustomTitleBar(!m_fCustomTitleBar);
		return true;

	case CM_SPLITTITLEBAR:
		SetSplitTitleBar(!m_fSplitTitleBar);
		return true;

	case CM_VIDEODECODERPROPERTY:
	case CM_VIDEORENDERERPROPERTY:
	case CM_AUDIOFILTERPROPERTY:
	case CM_AUDIORENDERERPROPERTY:
	case CM_DEMULTIPLEXERPROPERTY:
		{
			const HWND hwndOwner = GetVideoHostWindow();

			if (hwndOwner == nullptr || ::IsWindowEnabled(hwndOwner)) {
				for (const auto &e : m_DirectShowFilterPropertyList) {
					if (e.Command == Params.ID) {
						LibISDB::ViewerFilter *pViewer = m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();
						bool fOK = false;

						if (e.Filter == LibISDB::ViewerFilter::PropertyFilterType::VideoDecoder) {
							LibISDB::COMPointer<IBaseFilter> Decoder(pViewer->GetVideoDecoderFilter());
							if (Decoder) {
								const HRESULT hr = ShowPropertyPageFrame(Decoder.Get(), hwndOwner, m_App.GetResourceInstance());
								if (SUCCEEDED(hr)) {
									pViewer->SaveVideoDecoderSettings();
									CVideoDecoderOptions::VideoDecoderSettings Settings;
									if (pViewer->GetVideoDecoderSettings(&Settings))
										m_App.VideoDecoderOptions.SetVideoDecoderSettings(Settings);
									fOK = true;
								}
							}
						}

						if (!fOK) {
							pViewer->DisplayFilterProperty(hwndOwner, e.Filter);
						}
						break;
					}
				}
			}
		}
		return true;

	case CM_SHOW:
		if (m_pCore->GetStandby()) {
			m_pCore->SetStandby(false);
		} else {
			SetWindowVisible();
		}
		return true;

	case CM_MENU:
		{
			POINT pt;
			bool fDefault = false;

			if (!!(Params.Flags & CCommandManager::InvokeFlag::Mouse)) {
				::GetCursorPos(&pt);
				if (::GetKeyState(VK_SHIFT) < 0)
					fDefault = true;
			} else {
				pt.x = 0;
				pt.y = 0;
				::ClientToScreen(GetCurrentViewWindow().GetHandle(), &pt);
			}
			m_pCore->PopupMenu(&pt, fDefault ? CUICore::PopupMenuFlag::Default : CUICore::PopupMenuFlag::None);
		}
		return true;

	case CM_ACTIVATE:
		{
			const HWND hwndHost = GetVideoHostWindow();

			if (hwndHost != nullptr)
				ForegroundWindow(hwndHost);
		}
		return true;

	case CM_MINIMIZE:
		::ShowWindow(m_hwnd, ::IsIconic(m_hwnd) ? SW_RESTORE : SW_MINIMIZE);
		return true;

	case CM_MAXIMIZE:
		::ShowWindow(m_hwnd, ::IsZoomed(m_hwnd) ? SW_RESTORE : SW_MAXIMIZE);
		return true;

	case CM_HOMEDISPLAY:
		if (!m_App.HomeDisplay.GetVisible()) {
			Util::CWaitCursor WaitCursor;

			m_App.OSDManager.HideEventInfoOSD();

			m_App.HomeDisplay.SetFont(
				m_App.OSDOptions.GetDisplayFont(),
				m_App.OSDOptions.IsDisplayFontAutoSize());
			if (!m_App.HomeDisplay.IsCreated()) {
				m_App.HomeDisplay.SetEventHandler(&m_Display.HomeDisplayEventHandler);
				m_App.HomeDisplay.SetStyleScaling(&m_StyleScaling);
				m_App.HomeDisplay.Create(m_Display.GetDisplayViewParent(), WS_CHILD | WS_CLIPCHILDREN);
				if (m_fCustomFrame)
					HookWindows(m_App.HomeDisplay.GetHandle());
				RegisterUIChild(&m_App.HomeDisplay);
			}
			m_App.HomeDisplay.UpdateContents();
			m_Display.GetDisplayBase().SetDisplayView(&m_App.HomeDisplay);
			m_Display.GetDisplayBase().SetVisible(true);
			m_App.HomeDisplay.Update();
		} else {
			m_Display.GetDisplayBase().SetVisible(false);
		}
		return true;

	case CM_CHANNELDISPLAY:
		if (!m_App.ChannelDisplay.GetVisible()) {
			Util::CWaitCursor WaitCursor;

			m_App.OSDManager.HideEventInfoOSD();

			m_App.ChannelDisplay.SetFont(
				m_App.OSDOptions.GetDisplayFont(),
				m_App.OSDOptions.IsDisplayFontAutoSize());
			if (!m_App.ChannelDisplay.IsCreated()) {
				m_App.ChannelDisplay.SetEventHandler(&m_Display.ChannelDisplayEventHandler);
				m_App.ChannelDisplay.SetStyleScaling(&m_StyleScaling);
				m_App.ChannelDisplay.Create(
					m_Display.GetDisplayViewParent(),
					WS_CHILD | WS_CLIPCHILDREN);
				m_App.ChannelDisplay.SetDriverManager(&m_App.DriverManager);
				m_App.ChannelDisplay.SetLogoManager(&m_App.LogoManager);
				if (m_fCustomFrame)
					HookWindows(m_App.ChannelDisplay.GetHandle());
				RegisterUIChild(&m_App.ChannelDisplay);
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
		return true;

	case CM_POPUPTITLEBAR:
		if (!m_fShowTitleBar) {
			if (m_pCore->GetFullscreen())
				m_Fullscreen.ShowTitleBar(!m_Fullscreen.IsTitleBarVisible());
			else
				ShowPopupTitleBar(!m_TitleBar.GetVisible());
		}
		return true;

	case CM_POPUPSTATUSBAR:
		if (!m_fShowStatusBar) {
			if (m_pCore->GetFullscreen())
				m_Fullscreen.ShowStatusBar(!m_Fullscreen.IsStatusBarVisible());
			else
				ShowPopupStatusBar(!m_App.StatusView.GetVisible());
		}
		return true;

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
			const int SubMenu = m_App.MenuOptions.GetSubMenuPosByCommand(Params.ID);
			POINT pt;

			if (!!(Params.Flags & CCommandManager::InvokeFlag::Mouse)) {
				::GetCursorPos(&pt);
			} else {
				pt.x = 0;
				pt.y = 0;
				::ClientToScreen(GetCurrentViewWindow().GetHandle(), &pt);
			}
			m_App.MainMenu.PopupSubMenu(SubMenu, TPM_RIGHTBUTTON, m_hwnd, &pt);
		}
		return true;

	case CM_SIDEBAR_PLACE_LEFT:
	case CM_SIDEBAR_PLACE_RIGHT:
	case CM_SIDEBAR_PLACE_TOP:
	case CM_SIDEBAR_PLACE_BOTTOM:
		{
			const CSideBarOptions::PlaceType Place = static_cast<CSideBarOptions::PlaceType>(Params.ID - CM_SIDEBAR_PLACE_FIRST);

			if (Place != m_App.SideBarOptions.GetPlace()) {
				const bool fVertical =
					Place == CSideBarOptions::PlaceType::Left || Place == CSideBarOptions::PlaceType::Right;
				const int Pane =
					Place == CSideBarOptions::PlaceType::Left || Place == CSideBarOptions::PlaceType::Top ? 0 : 1;

				m_App.SideBarOptions.SetPlace(Place);
				m_App.SideBar.SetVertical(fVertical);
				Layout::CSplitter *pSplitter =
					dynamic_cast<Layout::CSplitter*>(m_LayoutBase.GetContainerByID(CONTAINER_ID_SIDEBARSPLITTER));
				const bool fSwap = pSplitter->IDToIndex(CONTAINER_ID_SIDEBAR) != Pane;
				pSplitter->SetStyle(
					(fVertical ? Layout::CSplitter::StyleFlag::Horz : Layout::CSplitter::StyleFlag::Vert) |
						Layout::CSplitter::StyleFlag::Fixed,
					!fSwap);
				if (fSwap)
					pSplitter->SwapPane();
			}
		}
		return true;

	case CM_CHANNELNO_2DIGIT:
	case CM_CHANNELNO_3DIGIT:
		{
			const int Digits = Params.ID == CM_CHANNELNO_2DIGIT ? 2 : 3;

			if (m_ChannelInput.IsInputting()) {
				const bool fCancel = Digits == m_ChannelInput.GetMaxDigits();
				EndChannelNoInput(false);
				if (fCancel)
					return true;
			}
			BeginChannelNoInput(Digits);
		}
		return true;

	default:
		if ((Params.ID >= CM_ZOOM_FIRST && Params.ID <= CM_ZOOM_LAST)
				|| (Params.ID >= CM_CUSTOMZOOM_FIRST && Params.ID <= CM_CUSTOMZOOM_LAST)) {
			CZoomOptions::ZoomInfo Info;

			if (m_pCore->GetFullscreen())
				m_pCore->SetFullscreen(false);
			if (::IsZoomed(m_hwnd))
				::ShowWindow(m_hwnd, SW_RESTORE);
			if (m_App.ZoomOptions.GetZoomInfoByCommand(Params.ID, &Info)) {
				if (Info.Type == CZoomOptions::ZoomType::Rate)
					SetZoomRate(Info.Rate.Rate, Info.Rate.Factor);
				else if (Info.Type == CZoomOptions::ZoomType::Size)
					AdjustWindowSize(Info.Size.Width, Info.Size.Height);
			}
			return true;
		}
	}

	return false;
}


bool CMainWindow::OnSysCommand(UINT Command)
{
	switch ((Command & 0xFFFFFFF0UL)) {
	case SC_MONITORPOWER:
		if (m_App.GeneralOptions.GetNoMonitorLowPower()
				&& m_pCore->IsViewerEnabled())
			return true;
		break;

	case SC_SCREENSAVE:
		if (m_App.GeneralOptions.GetNoScreenSaver()
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


void CMainWindow::OnTimer(HWND hwnd, UINT id)
{
	switch (id) {
	case TIMER_ID_UPDATE:
		// 情報更新
		{
			const CCoreEngine::StatusFlag UpdateStatus = m_App.CoreEngine.UpdateAsyncStatus();
			const CCoreEngine::StatisticsFlag UpdateStatistics = m_App.CoreEngine.UpdateStatistics();

			// 映像サイズの変化
			if (!!(UpdateStatus & CCoreEngine::StatusFlag::VideoSize)) {
				m_App.StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
				m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_VIDEOINFO);
				m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VIDEO);

				m_App.AppEventManager.OnVideoFormatChanged();
			}

			// 音声形式の変化
			if (!!(UpdateStatus &
					(CCoreEngine::StatusFlag::AudioChannels |
					 CCoreEngine::StatusFlag::AudioStreams |
					 CCoreEngine::StatusFlag::AudioComponentType |
					 CCoreEngine::StatusFlag::SPDIFPassthrough))) {
				TRACE(TEXT("Audio status changed.\n"));
				m_App.StatusView.UpdateItem(STATUS_ITEM_AUDIOCHANNEL);
				m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_AUDIO);
				m_pCore->SetCommandCheckedState(
					CM_SPDIF_TOGGLE, m_App.CoreEngine.IsSPDIFPassthroughEnabled());

				m_App.AppEventManager.OnAudioFormatChanged();
			}

			bool fUpdateEventInfo = false;

			if (!!(UpdateStatus & CCoreEngine::StatusFlag::EventID)) {
				// 番組の切り替わり
				OnEventChanged();

				m_App.AppEventManager.OnEventChanged();
			} else if (!!(UpdateStatus & CCoreEngine::StatusFlag::EventInfo)) {
				// 番組情報が更新された
				fUpdateEventInfo = true;

				m_pCore->UpdateTitle();
				m_App.StatusView.UpdateItem(STATUS_ITEM_AUDIOCHANNEL);

				m_App.AppEventManager.OnEventInfoChanged();
			}

			CProgramInfoStatusItem *pProgramInfoStatusItem =
				dynamic_cast<CProgramInfoStatusItem*>(m_App.StatusView.GetItemByID(STATUS_ITEM_PROGRAMINFO));
			if (pProgramInfoStatusItem != nullptr) {
				if (fUpdateEventInfo) {
					pProgramInfoStatusItem->Update();
				} else if (pProgramInfoStatusItem->GetShowProgress()
						&& !!(UpdateStatus & CCoreEngine::StatusFlag::TOT)) {
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

			if (!!(UpdateStatistics &
					(CCoreEngine::StatisticsFlag::ErrorPacketCount |
					 CCoreEngine::StatisticsFlag::ContinuityErrorPacketCount |
					 CCoreEngine::StatisticsFlag::ScrambledPacketCount))) {
				m_App.StatusView.UpdateItem(STATUS_ITEM_ERROR);
			}

			if (!!(UpdateStatistics &
					(CCoreEngine::StatisticsFlag::SignalLevel |
					 CCoreEngine::StatisticsFlag::BitRate)))
				m_App.StatusView.UpdateItem(STATUS_ITEM_SIGNALLEVEL);

			if (!!(UpdateStatistics &
					(CCoreEngine::StatisticsFlag::StreamRemain |
					 CCoreEngine::StatisticsFlag::PacketBufferRate)))
				m_App.StatusView.UpdateItem(STATUS_ITEM_BUFFERING);

			CClockStatusItem *pClockStatusItem =
				dynamic_cast<CClockStatusItem*>(m_App.StatusView.GetItemByID(STATUS_ITEM_CLOCK));
			if (pClockStatusItem != nullptr
					&& (!pClockStatusItem->IsTOT()
						|| (pClockStatusItem->IsInterpolateTOT() || !!(UpdateStatus & CCoreEngine::StatusFlag::TOT)))) {
				pClockStatusItem->Update();
			}

			if (!!(UpdateStatus & CCoreEngine::StatusFlag::TOT))
				m_App.TotTimeAdjuster.AdjustTime();

			m_App.StatusView.UpdateItem(STATUS_ITEM_MEDIABITRATE);

			if (IsPanelVisible()) {
				// パネルの更新
				switch (m_App.Panel.Form.GetCurPageID()) {
				case PANEL_ID_INFORMATION:
					// 情報タブ更新
					m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_VIDEOINFO);

					if (!!(UpdateStatistics &
							(CCoreEngine::StatisticsFlag::SignalLevel |
							 CCoreEngine::StatisticsFlag::BitRate))) {
						m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_SIGNALLEVEL);
					}

					m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_MEDIABITRATE);

					if (!!(UpdateStatistics &
							(CCoreEngine::StatisticsFlag::ErrorPacketCount |
							 CCoreEngine::StatisticsFlag::ContinuityErrorPacketCount |
							 CCoreEngine::StatisticsFlag::ScrambledPacketCount))) {
						m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_ERROR);
					}

					if (m_App.RecordManager.IsRecording())
						m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_RECORD);

					if (fUpdateEventInfo)
						m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_PROGRAMINFO);
					break;

				case PANEL_ID_CHANNEL:
					// チャンネルタブ更新
					if (!m_App.EpgOptions.IsEpgDataLoading()) {
						if (m_App.Panel.ChannelPanel.QueryUpdate()) {
							m_App.Panel.ChannelPanel.UpdateAllChannels();
						} else if (!!(UpdateStatus & CCoreEngine::StatusFlag::EventInfo)) {
							CAppCore::StreamIDInfo Info;
							if (m_App.Core.GetCurrentStreamIDInfo(&Info))
								m_App.Panel.ChannelPanel.UpdateChannels(Info.NetworkID, Info.TransportStreamID);
						}
					}
					if (m_App.Panel.ChannelPanel.QueryUpdateProgress())
						m_App.Panel.ChannelPanel.UpdateProgress();
					break;

				case PANEL_ID_PROGRAMLIST:
					if (fUpdateEventInfo)
						m_App.Panel.UpdateContent();
					break;

				case PANEL_ID_CONTROL:
					if (fUpdateEventInfo)
						m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_AUDIO);
					break;
				}
			}

			// 空き容量が少ない場合の注意表示
			if (m_App.RecordOptions.GetAlertLowFreeSpace()
					&& !m_fAlertedLowFreeSpace
					&& m_App.RecordManager.IsRecording()) {
				const LONGLONG FreeSpace = m_App.RecordManager.GetRecordTask()->GetFreeSpace();

				if (FreeSpace >= 0
						&& static_cast<ULONGLONG>(FreeSpace) <= m_App.RecordOptions.GetLowFreeSpaceThresholdBytes()) {
					m_App.NotifyBalloonTip.Show(
						APP_NAME TEXT("の録画ファイルの保存先の空き容量が少なくなっています。"),
						TEXT("空き容量が少なくなっています。"),
						nullptr, CBalloonTip::ICON_WARNING);
					m_Timer.BeginTimer(TIMER_ID_HIDETOOLTIP, 10000);
					ShowNotificationBar(
						TEXT("録画ファイルの保存先の空き容量が少なくなっています"),
						CNotificationBar::MessageType::Warning, 6000);
					m_fAlertedLowFreeSpace = true;
				}
			}

			m_App.PluginManager.SendStatusItemUpdateTimerEvent();
		}
		break;

	case TIMER_ID_OSD:
		// OSD を消す
		m_App.OSDManager.ClearCompositedOSD();
		m_Timer.EndTimer(TIMER_ID_OSD);
		break;

	case TIMER_ID_WHEELCHANNELSELECT:
		// ホイールでのチャンネル変更
		{
			const int Channel = m_App.ChannelManager.GetChangingChannel();

			SetWheelChannelChanging(false);
			m_App.ChannelManager.SetChangingChannel(-1);
			if (Channel >= 0)
				m_App.Core.SwitchChannel(Channel);
		}
		break;

	case TIMER_ID_PROGRAMLISTUPDATE:
		// EPG情報の同期
		if (!m_App.EpgOptions.IsEpgDataLoading()) {
			m_App.Panel.EnableProgramListUpdate(true);

			CChannelInfo ChInfo;

			if (IsPanelVisible()
					&& m_App.Core.GetCurrentStreamChannelInfo(&ChInfo)) {
				if (m_App.Panel.Form.GetCurPageID() == PANEL_ID_PROGRAMLIST) {
					if (ChInfo.GetServiceID() != 0) {
						m_App.Panel.ProgramListPanel.UpdateProgramList(&ChInfo);
					}
				} else if (m_App.Panel.Form.GetCurPageID() == PANEL_ID_CHANNEL) {
					m_App.Panel.ChannelPanel.UpdateChannels(
						ChInfo.GetNetworkID(),
						ChInfo.GetTransportStreamID());
				}
			}

			m_ProgramListUpdateTimerCount++;
			// 更新頻度を下げる
			if (m_ProgramListUpdateTimerCount >= 6 && m_ProgramListUpdateTimerCount <= 10)
				m_Timer.BeginTimer(TIMER_ID_PROGRAMLISTUPDATE, (m_ProgramListUpdateTimerCount - 5) * (60 * 1000));
		}
		break;

	case TIMER_ID_PROGRAMGUIDEUPDATE:
		// 番組表の取得
		if (!m_App.EpgCaptureManager.ProcessCapture())
			m_Timer.BeginTimer(TIMER_ID_PROGRAMGUIDEUPDATE, 3000);
		break;

	case TIMER_ID_VIDEOSIZECHANGED:
		// 映像サイズの変化に合わせる

		if (m_App.ViewOptions.GetRemember1SegWindowSize()) {
			int Width, Height;

			if (m_App.CoreEngine.GetVideoViewSize(&Width, &Height)
					&& Width > 0 && Height > 0) {
				const WindowSizeMode Mode =
					Height <= 240 ? WindowSizeMode::OneSeg : WindowSizeMode::HD;

				if (m_WindowSizeMode != Mode) {
					RECT rc;
					GetPosition(&rc);
					if (m_WindowSizeMode == WindowSizeMode::OneSeg)
						m_1SegWindowSize = rc;
					else
						m_HDWindowSize = rc;
					m_WindowSizeMode = Mode;
					const WindowSize *pSize;
					if (m_WindowSizeMode == WindowSizeMode::OneSeg)
						pSize = &m_1SegWindowSize;
					else
						pSize = &m_HDWindowSize;
					if (pSize->IsValid())
						AdjustWindowSize(pSize->Width, pSize->Height, false);
				}
			}
		}

		m_Display.GetVideoContainer().SendSizeMessage();
		m_App.StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
		m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VIDEO);
		if (m_VideoSizeChangedTimerCount == 3)
			m_Timer.EndTimer(TIMER_ID_VIDEOSIZECHANGED);
		else
			m_VideoSizeChangedTimerCount++;
		break;

	case TIMER_ID_RESETERRORCOUNT:
		// エラーカウントをリセットする
		// (既にサービスの情報が取得されている場合のみ)
		{
			const LibISDB::AnalyzerFilter *pAnalyzer =
				m_App.CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();

			if (pAnalyzer != nullptr && pAnalyzer->GetServiceCount() > 0) {
				SendCommand(CM_RESETERRORCOUNT);
				m_Timer.EndTimer(TIMER_ID_RESETERRORCOUNT);
			}
		}
		break;

	case TIMER_ID_HIDETOOLTIP:
		// ツールチップを非表示にする
		m_App.NotifyBalloonTip.Hide();
		m_Timer.EndTimer(TIMER_ID_HIDETOOLTIP);
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
		if (m_App.ViewOptions.GetHideCursor()) {
			if (!m_fNoHideCursor && !m_Display.GetDisplayBase().IsVisible()) {
				POINT pt;
				RECT rc;
				::GetCursorPos(&pt);
				m_Display.GetViewWindow().GetScreenPosition(&rc);
				if (::PtInRect(&rc, pt))
					ShowCursor(false);
			}
		}
		m_Timer.EndTimer(TIMER_ID_HIDECURSOR);
		break;
	}
}


bool CMainWindow::OnInitMenuPopup(HMENU hmenu)
{
	if (m_App.MainMenu.IsMainMenu(hmenu)) {
		const bool fView = IsViewerEnabled();

		m_App.MainMenu.EnableItem(CM_COPYIMAGE, fView);
		m_App.MainMenu.EnableItem(CM_SAVEIMAGE, fView);
	} else if (hmenu == m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_ZOOM)) {
		CZoomOptions::ZoomInfo Zoom;

		if (!GetZoomRate(&Zoom.Rate.Rate, &Zoom.Rate.Factor)) {
			Zoom.Rate.Rate = 0;
			Zoom.Rate.Factor = 0;
		}

		SIZE sz;
		if (m_Display.GetVideoContainer().GetClientSize(&sz)) {
			Zoom.Size.Width = sz.cx;
			Zoom.Size.Height = sz.cy;
		} else {
			Zoom.Size.Width = 0;
			Zoom.Size.Height = 0;
		}

		m_App.ZoomOptions.SetMenu(hmenu, &Zoom);
		m_App.Accelerator.SetMenuAccel(hmenu);
	} else if (hmenu == m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_SPACE)) {
		m_pCore->InitTunerMenu(hmenu);
	} else if (hmenu == m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_PLUGIN)) {
		m_App.PluginManager.SetMenu(hmenu);
		m_App.Accelerator.SetMenuAccel(hmenu);
	} else if (hmenu == m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_FAVORITES)) {
		//m_App.FavoritesManager.SetMenu(hmenu);
		m_App.FavoritesMenu.Create(
			&m_App.FavoritesManager.GetRootFolder(),
			CM_FAVORITECHANNEL_FIRST, hmenu, m_hwnd,
			CFavoritesMenu::CreateFlag::ShowEventInfo | CFavoritesMenu::CreateFlag::ShowLogo,
			m_pCore->GetPopupMenuDPI());
		::EnableMenuItem(
			hmenu, CM_ADDTOFAVORITES,
			MF_BYCOMMAND | (m_App.ChannelManager.GetCurrentChannelInfo() != nullptr ? MF_ENABLED : MF_GRAYED));
	} else if (hmenu == m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_CHANNELHISTORY)) {
		m_App.RecentChannelList.SetMenu(m_hwnd, hmenu);
	} else if (hmenu == m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_ASPECTRATIO)) {
		int ItemCount = ::GetMenuItemCount(hmenu);

		if (ItemCount > m_DefaultAspectRatioMenuItemCount) {
			for (; ItemCount > m_DefaultAspectRatioMenuItemCount; ItemCount--) {
				::DeleteMenu(hmenu, ItemCount - 3, MF_BYPOSITION);
			}
		}

		const size_t PresetCount = m_App.PanAndScanOptions.GetPresetCount();
		if (PresetCount > 0) {
			::InsertMenu(hmenu, ItemCount - 2, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
			const int AspectRatioType = m_pCore->GetAspectRatioType();
			for (size_t i = 0; i < PresetCount; i++) {
				CPanAndScanOptions::PanAndScanInfo Info;
				TCHAR szText[CPanAndScanOptions::MAX_NAME * 2];

				m_App.PanAndScanOptions.GetPreset(i, &Info);
				CopyToMenuText(Info.szName, szText, lengthof(szText));
				::InsertMenu(
					hmenu, ItemCount - 2 + static_cast<UINT>(i),
					MF_BYPOSITION | MF_STRING | MF_ENABLED
						| (AspectRatioType == CUICore::ASPECTRATIO_CUSTOM_FIRST + static_cast<int>(i) ? MF_CHECKED : MF_UNCHECKED),
					CM_PANANDSCAN_PRESET_FIRST + i, szText);
			}
		}

		const LibISDB::ViewerFilter *pViewer = m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();
		m_App.AspectRatioIconMenu.CheckItem(
			CM_FRAMECUT, pViewer != nullptr && pViewer->GetViewStretchMode() == LibISDB::ViewerFilter::ViewStretchMode::Crop);

		m_App.Accelerator.SetMenuAccel(hmenu);
		if (!m_App.AspectRatioIconMenu.OnInitMenuPopup(m_hwnd, hmenu))
			return false;
	} else if (hmenu == m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_CHANNEL)) {
		m_pCore->InitChannelMenu(hmenu);
	} else if (hmenu == m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_SERVICE)) {
		m_App.ChannelMenu.Destroy();
		ClearMenu(hmenu);

		CAppCore::StreamIDInfo StreamID;

		if (m_App.Core.GetCurrentStreamIDInfo(&StreamID)) {
			LibISDB::AnalyzerFilter::ServiceList ServiceList;
			CChannelList ChList;
			int CurService = -1;

			m_App.CoreEngine.GetSelectableServiceList(&ServiceList);

			for (int i = 0; i < static_cast<int>(ServiceList.size()); i++) {
				const LibISDB::AnalyzerFilter::ServiceInfo &ServiceInfo = ServiceList[i];
				CChannelInfo *pChInfo = new CChannelInfo;

				pChInfo->SetChannelNo(i + 1);
				if (!ServiceInfo.ServiceName.empty()) {
					pChInfo->SetName(ServiceInfo.ServiceName.c_str());
				} else {
					TCHAR szName[32];
					StringFormat(szName, TEXT("サービス{}"), i + 1);
					pChInfo->SetName(szName);
				}
				pChInfo->SetServiceID(ServiceInfo.ServiceID);
				pChInfo->SetNetworkID(StreamID.NetworkID);
				pChInfo->SetTransportStreamID(StreamID.TransportStreamID);
				ChList.AddChannel(pChInfo);
				if (ServiceInfo.ServiceID == StreamID.ServiceID)
					CurService = i;
			}

			m_App.ChannelMenu.Create(
				&ChList, CurService, CM_SERVICE_FIRST, CM_SERVICE_LAST, hmenu, m_hwnd,
				CChannelMenu::CreateFlag::ShowLogo |
				CChannelMenu::CreateFlag::ShowEventInfo |
				CChannelMenu::CreateFlag::CurrentServices,
				0,
				m_pCore->GetPopupMenuDPI());
		}
	} else if (hmenu == m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_AUDIO)) {
		CPopupMenu Menu(hmenu, false);
		Menu.Clear();

		const LibISDB::AnalyzerFilter *pAnalyzer = m_App.CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
		CAudioManager::AudioList AudioList;
		const LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode CurDualMonoMode = m_pCore->GetDualMonoMode();
		bool fDualMono = false;

		if (m_App.AudioManager.GetAudioList(&AudioList) && !AudioList.empty()) {
			const auto GetAudioInfoText = [](
					const CAudioManager::AudioInfo & Info, int StreamNumber,
					LPTSTR pszText, int MaxText)
				{
					size_t Length;
					if (!Info.Text.empty()) {
						Length = CopyToMenuText(Info.Text.c_str(), pszText, MaxText);
					} else if (Info.Language != 0 && (!Info.IsDualMono() || Info.fMultiLingual)) {
						LibISDB::GetLanguageText_ja(Info.Language, pszText, MaxText);
						Length = ::lstrlen(pszText);
						if (Info.DualMono == CAudioManager::DualMonoMode::Both) {
							TCHAR szLang2[LibISDB::MAX_LANGUAGE_TEXT_LENGTH];
							LibISDB::GetLanguageText_ja(Info.Language2, szLang2, lengthof(szLang2));
							Length += StringFormat(pszText + Length, MaxText - Length, TEXT("+{}"), szLang2);
						}
					} else if (Info.IsDualMono()) {
						Length = StringVFormat(
							pszText, MaxText,
							Info.DualMono == CAudioManager::DualMonoMode::Main ?
								TEXT("主音声") :
							Info.DualMono == CAudioManager::DualMonoMode::Sub ?
								TEXT("副音声") :
								TEXT("主+副音声"));
					} else {
						Length = StringFormat(pszText, MaxText, TEXT("音声{}"), StreamNumber + 1);
					}
					if (Info.ComponentType != LibISDB::COMPONENT_TYPE_INVALID) {
						LPCTSTR pszComponentType = LibISDB::GetAudioComponentTypeText_ja(Info.ComponentType);
						if (pszComponentType != nullptr) {
							StringFormat(
								pszText + Length, MaxText - Length,
								TEXT(" [{}]"), pszComponentType);
						}
					}
				};

			const int ServiceIndex = m_App.CoreEngine.GetServiceIndex();
			LibISDB::AnalyzerFilter::EventComponentGroupList GroupList;
			int Sel = -1;

			if (pAnalyzer->GetEventComponentGroupList(ServiceIndex, &GroupList)
					&& !GroupList.empty()) {
				// マルチビューTV
				const int NumGroup = static_cast<int>(GroupList.size());
				int SubGroupCount = 0;

				// グループ毎の音声
				for (int i = 0; i < NumGroup; i++) {
					const LibISDB::AnalyzerFilter::EventComponentGroupInfo &GroupInfo = GroupList[i];
					int StreamNumber = -1;

					for (int j = 0; j < GroupInfo.NumOfCAUnit; j++) {
						for (int k = 0; k < GroupInfo.CAUnitList[j].NumOfComponent; k++) {
							const BYTE ComponentTag = GroupInfo.CAUnitList[j].ComponentTag[k];

							for (int AudioIndex = 0; AudioIndex < static_cast<int>(AudioList.size()); AudioIndex++) {
								CAudioManager::AudioInfo &AudioInfo = AudioList[AudioIndex];

								if (AudioInfo.ComponentTag == ComponentTag) {
									TCHAR szText[80];
									size_t Length;

									if (!GroupInfo.Text.empty()) {
										Length = CopyToMenuText(GroupInfo.Text.c_str(), szText, lengthof(szText));
									} else {
										if (GroupInfo.ComponentGroupID == 0)
											Length = StringFormat(szText, TEXT("メイン"));
										else
											Length = StringFormat(szText, TEXT("サブ{}"), SubGroupCount + 1);
									}

									if (!AudioInfo.IsDualMono() || AudioInfo.DualMono == CAudioManager::DualMonoMode::Main)
										StreamNumber++;

									TCHAR szAudio[64];
									GetAudioInfoText(AudioInfo, StreamNumber, szAudio, lengthof(szAudio));
									StringFormat(
										szText + Length, lengthof(szText) - Length,
										TEXT(": {}"), szAudio);
									Menu.Append(CM_AUDIO_FIRST + AudioIndex, szText);
									AudioInfo.ID = CAudioManager::ID_INVALID;

									if (AudioInfo.IsDualMono()) {
										if (CurDualMonoMode == LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Main) {
											if (AudioInfo.DualMono == CAudioManager::DualMonoMode::Main)
												Sel = AudioIndex;
										} else if (CurDualMonoMode == LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Sub) {
											if (AudioInfo.DualMono == CAudioManager::DualMonoMode::Sub)
												Sel = AudioIndex;
										} else {
											if (AudioInfo.DualMono == CAudioManager::DualMonoMode::Both)
												Sel = AudioIndex;
										}
										fDualMono = true;
									} else {
										Sel = AudioIndex;
									}
								}
							}
						}
					}

					if (GroupInfo.ComponentGroupID != 0)
						SubGroupCount++;
				}
			}

			const CAudioManager::IDType CurAudioID =
				CAudioManager::MakeID(
					m_App.CoreEngine.GetAudioStream(),
					m_App.CoreEngine.GetAudioComponentTag());
			int StreamNumber = -1;

			for (int i = 0; i < static_cast<int>(AudioList.size()) && i <= CM_AUDIO_LAST - CM_AUDIO_FIRST; i++) {
				const CAudioManager::AudioInfo &AudioInfo = AudioList[i];

				if (AudioInfo.ID == CAudioManager::ID_INVALID)
					continue;

				if (!AudioInfo.IsDualMono() || AudioInfo.DualMono == CAudioManager::DualMonoMode::Main)
					StreamNumber++;

				TCHAR szText[80];
				const size_t Length = StringFormat(
					szText, TEXT("{}{}: "),
					i < 9 ? TEXT("&") : TEXT(""), i + 1);
				GetAudioInfoText(AudioInfo, StreamNumber, szText + Length, static_cast<int>(lengthof(szText) - Length));
				Menu.Append(CM_AUDIO_FIRST + i, szText);

				if (AudioInfo.ID == CurAudioID) {
					if (AudioInfo.IsDualMono()) {
						if (CurDualMonoMode == LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Main) {
							if (AudioInfo.DualMono == CAudioManager::DualMonoMode::Main)
								Sel = i;
						} else if (CurDualMonoMode == LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Sub) {
							if (AudioInfo.DualMono == CAudioManager::DualMonoMode::Sub)
								Sel = i;
						} else {
							if (AudioInfo.DualMono == CAudioManager::DualMonoMode::Both)
								Sel = i;
						}
						fDualMono = true;
					} else {
						Sel = i;
					}
				}
			}

			if (Sel >= 0) {
				Menu.CheckRadioItem(
					CM_AUDIO_FIRST,
					CM_AUDIO_FIRST + static_cast<int>(AudioList.size()) - 1,
					CM_AUDIO_FIRST + Sel);
			}
		}

		const HINSTANCE hinstRes = m_App.GetResourceInstance();

		if (!fDualMono) {
			const LibISDB::ViewerFilter *pViewer = m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();
			const int Channels = pViewer->GetAudioChannelCount();

			if (Channels == LibISDB::ViewerFilter::AudioChannelCount_DualMono) {
				if (Menu.GetItemCount() > 0)
					Menu.AppendSeparator();
				static const int DualMonoMenuList[] = {
					CM_DUALMONO_MAIN,
					CM_DUALMONO_SUB,
					CM_DUALMONO_BOTH
				};
				for (const int Command : DualMonoMenuList) {
					TCHAR szText[64];
					::LoadString(hinstRes, Command, szText, lengthof(szText));
					Menu.Append(Command, szText);
				}
				Menu.CheckRadioItem(
					CM_DUALMONO_MAIN, CM_DUALMONO_BOTH,
					CurDualMonoMode == LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Main ? CM_DUALMONO_MAIN :
					CurDualMonoMode == LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Sub ? CM_DUALMONO_SUB : CM_DUALMONO_BOTH);
			}
		}

		if (Menu.GetItemCount() > 0) {
			Menu.AppendSeparator();
		} else {
			Menu.Append(0U, TEXT("音声なし"), MF_GRAYED);
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
		for (const int Command : AdditionalMenuList) {
			if (Command != 0) {
				TCHAR szText[64];
				::LoadString(hinstRes, Command, szText, lengthof(szText));
				Menu.Append(Command, szText);
			} else {
				Menu.AppendSeparator();
			}
		}
		LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions SPDIFOptions;
		m_App.CoreEngine.GetSPDIFOptions(&SPDIFOptions);
		Menu.CheckRadioItem(
			CM_SPDIF_DISABLED, CM_SPDIF_AUTO,
			CM_SPDIF_DISABLED + static_cast<int>(SPDIFOptions.Mode));
		m_App.Accelerator.SetMenuAccel(hmenu);
	} else if (hmenu == m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_VIDEO)) {
		CPopupMenu Menu(hmenu, false);
		Menu.Clear();

		const LibISDB::AnalyzerFilter *pAnalyzer = m_App.CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
		const int ServiceIndex = m_App.CoreEngine.GetServiceIndex();
		LibISDB::AnalyzerFilter::EventComponentGroupList GroupList;

		if (pAnalyzer->GetEventComponentGroupList(ServiceIndex, &GroupList)
				&& !GroupList.empty()) {
			// マルチビューTV
			const int NumGroup = static_cast<int>(GroupList.size());
			const BYTE CurVideoComponentTag = m_App.CoreEngine.GetVideoComponentTag();
			int CurGroup = -1;
			int SubGroupCount = 0;

			for (int i = 0; i < NumGroup; i++) {
				LibISDB::AnalyzerFilter::EventComponentGroupInfo &GroupInfo = GroupList[i];
				TCHAR szText[80];

				if (!GroupInfo.Text.empty()) {
					CopyToMenuText(GroupInfo.Text.c_str(), szText, lengthof(szText));
				} else {
					if (GroupInfo.ComponentGroupID == 0)
						StringCopy(szText, TEXT("メイン"));
					else
						StringFormat(szText, TEXT("サブ{}"), SubGroupCount + 1);
					GroupInfo.Text = szText;
				}
				if (GroupInfo.ComponentGroupID != 0)
					SubGroupCount++;

				Menu.Append(CM_MULTIVIEW_FIRST + i, szText);

				if (CurGroup < 0) {
					for (int j = 0; j < GroupInfo.NumOfCAUnit; j++) {
						for (int k = 0; k < GroupInfo.CAUnitList[j].NumOfComponent; k++) {
							if (GroupInfo.CAUnitList[j].ComponentTag[k] == CurVideoComponentTag) {
								CurGroup = i;
								break;
							}
						}
					}
				}
			}

			if (CurGroup >= 0) {
				Menu.CheckRadioItem(
					CM_MULTIVIEW_FIRST,
					CM_MULTIVIEW_FIRST + NumGroup - 1,
					CM_MULTIVIEW_FIRST + CurGroup);
			}

			LibISDB::AnalyzerFilter::ESInfoList EsList;

			if (pAnalyzer->GetVideoESList(ServiceIndex, &EsList)
					&& !EsList.empty()) {
				Menu.AppendSeparator();

				int CurVideoIndex = -1;

				// グループ毎の映像
				for (int i = 0; i < NumGroup; i++) {
					const LibISDB::AnalyzerFilter::EventComponentGroupInfo &GroupInfo = GroupList[i];
					int VideoCount = 0;

					for (int j = 0; j < GroupInfo.NumOfCAUnit; j++) {
						for (int k = 0; k < GroupInfo.CAUnitList[j].NumOfComponent; k++) {
							const BYTE ComponentTag = GroupInfo.CAUnitList[j].ComponentTag[k];
							for (int EsIndex = 0; EsIndex < static_cast<int>(EsList.size()); EsIndex++) {
								if (EsList[EsIndex].ComponentTag == ComponentTag) {
									TCHAR szText[80];
									VideoCount++;
									const int Length = CopyToMenuText(GroupInfo.Text.c_str(), szText, lengthof(szText));
									StringFormat(szText + Length, lengthof(szText) - Length, TEXT(": 映像{}"), VideoCount);
									Menu.Append(CM_VIDEOSTREAM_FIRST + EsIndex, szText);
									if (ComponentTag == CurVideoComponentTag)
										CurVideoIndex = EsIndex;
									EsList[EsIndex].ComponentTag = LibISDB::COMPONENT_TAG_INVALID;
									break;
								}
							}
						}
					}
				}

				// グループに属さない映像
				int VideoCount = 0;
				for (int i = 0; i < static_cast<int>(EsList.size()); i++) {
					if (EsList[i].ComponentTag != LibISDB::COMPONENT_TAG_INVALID) {
						TCHAR szText[64];
						VideoCount++;
						const size_t Length = StringFormat(szText, TEXT("映像{}"), VideoCount);
						if (EsList[i].QualityLevel == 0)
							StringCopy(szText + Length, TEXT(" (降雨対応放送)"), lengthof(szText) - Length);
						Menu.Append(CM_VIDEOSTREAM_FIRST + i, szText);
						if (EsList[i].ComponentTag == CurVideoComponentTag)
							CurVideoIndex = i;
					}
				}

				if (CurVideoIndex >= 0) {
					Menu.CheckRadioItem(
						CM_VIDEOSTREAM_FIRST,
						CM_VIDEOSTREAM_FIRST + static_cast<int>(EsList.size()) - 1,
						CM_VIDEOSTREAM_FIRST + CurVideoIndex);
				}
			}
		} else {
			LibISDB::AnalyzerFilter::ESInfoList EsList;

			if (pAnalyzer->GetVideoESList(ServiceIndex, &EsList)
					&& !EsList.empty()) {
				for (int i = 0; i < static_cast<int>(EsList.size()) && CM_VIDEOSTREAM_FIRST + i <= CM_VIDEOSTREAM_LAST; i++) {
					TCHAR szText[64];
					const size_t Length = StringFormat(
						szText, TEXT("{}{}: 映像{}"),
						i < 9 ? TEXT("&") : TEXT(""), i + 1, i + 1);
					if (EsList[i].QualityLevel == 0)
						StringCopy(szText + Length, TEXT(" (降雨対応放送)"), lengthof(szText) - Length);
					Menu.Append(CM_VIDEOSTREAM_FIRST + i, szText);
				}

				Menu.CheckRadioItem(
					CM_VIDEOSTREAM_FIRST,
					CM_VIDEOSTREAM_FIRST + static_cast<int>(EsList.size()) - 1,
					CM_VIDEOSTREAM_FIRST + m_App.CoreEngine.GetVideoStream());
			} else {
				Menu.Append(0U, TEXT("映像なし"), MF_GRAYED);
			}
		}
	} else if (hmenu == m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_FILTERPROPERTY)) {
		const LibISDB::ViewerFilter *pViewer = m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();
		for (const auto &e : m_DirectShowFilterPropertyList) {
			m_App.MainMenu.EnableItem(
				e.Command,
				pViewer != nullptr && pViewer->FilterHasProperty(e.Filter));
		}
	} else if (hmenu == m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_BAR)) {
		m_App.MainMenu.CheckRadioItem(
			CM_WINDOWFRAME_NORMAL, CM_WINDOWFRAME_NONE,
			!m_fCustomFrame ?
				CM_WINDOWFRAME_NORMAL :
				(m_CustomFrameWidth == 0 ? CM_WINDOWFRAME_NONE : CM_WINDOWFRAME_CUSTOM));
		m_App.MainMenu.CheckItem(CM_CUSTOMTITLEBAR, m_fCustomTitleBar);
		m_App.MainMenu.EnableItem(CM_CUSTOMTITLEBAR, !m_fCustomFrame);
		m_App.MainMenu.CheckItem(CM_SPLITTITLEBAR, m_fSplitTitleBar);
		m_App.MainMenu.EnableItem(CM_SPLITTITLEBAR, m_fCustomFrame || m_fCustomTitleBar);
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

	m_fClosing = true;

	::SetCursor(::LoadCursor(nullptr, IDC_WAIT));

	m_App.AddLog(TEXT("ウィンドウを閉じています..."));

	m_Timer.EndTimer(TIMER_ID_UPDATE);

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
		m_App.EpgCaptureManager.EndCapture(CEpgCaptureManager::EndFlag::None);

	m_App.Panel.ProgramListPanel.ClearProgramList();
	m_App.Panel.ProgramListPanel.ShowRetrievingMessage(false);
	m_App.Panel.InfoPanel.ResetItem(CInformationPanel::ITEM_SERVICE);
	m_App.Panel.InfoPanel.ResetItem(CInformationPanel::ITEM_PROGRAMINFO);

	const bool fNoSignalLevel = m_App.DriverOptions.IsNoSignalLevel(m_App.CoreEngine.GetDriverFileName());
	m_App.Panel.InfoPanel.GetItem<CInformationPanel::CSignalLevelItem>()->ShowSignalLevel(!fNoSignalLevel);
	CSignalLevelStatusItem *pItem = dynamic_cast<CSignalLevelStatusItem*>(
		m_App.StatusView.GetItemByID(STATUS_ITEM_SIGNALLEVEL));
	if (pItem != nullptr)
		pItem->ShowSignalLevel(!fNoSignalLevel);

	/*
	if (IsPanelVisible() && m_App.Panel.Form.GetCurPageID() == PANEL_ID_CHANNEL) {
		m_App.Panel.ChannelPanel.SetChannelList(
			m_App.ChannelManager.GetCurrentChannelList(),
			!m_App.EpgOptions.IsEpgDataLoading());
	} else {
		m_App.Panel.ChannelPanel.ClearChannelList();
	}
	*/
	m_App.Panel.CaptionPanel.Reset();
	m_App.Epg.ProgramGuide.ClearCurrentService();
	ClearMenu(m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_SERVICE));
	m_Timer.EndTimer(TIMER_ID_RESETERRORCOUNT);
	m_App.StatusView.UpdateItem(STATUS_ITEM_CHANNEL);
	m_App.StatusView.UpdateItem(STATUS_ITEM_TUNER);
	m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_CHANNEL);
	m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_TUNER);
	m_pCore->SetCommandRadioCheckedState(CM_CHANNELNO_1, CM_CHANNELNO_12, 0);
	if (m_App.SideBarOptions.GetShowChannelLogo())
		m_App.SideBar.Invalidate();
	m_fForceResetPanAndScan = true;

	m_pCore->UpdateTitle();
	m_pCore->UpdateIcon();
}


void CMainWindow::OnTunerOpened()
{
	if (m_App.EpgCaptureManager.IsCapturing())
		m_App.EpgCaptureManager.EndCapture(CEpgCaptureManager::EndFlag::None);

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
	if (IsPanelVisible() && m_App.Panel.Form.GetCurPageID() == PANEL_ID_CHANNEL) {
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


void CMainWindow::OnChannelChanged(AppEvent::ChannelChangeStatus Status)
{
	const bool fSpaceChanged = !!(Status & AppEvent::ChannelChangeStatus::SpaceChanged);
	const int CurSpace = m_App.ChannelManager.GetCurrentSpace();
	const CChannelInfo *pCurChannel = m_App.ChannelManager.GetCurrentChannelInfo();

	SetWheelChannelChanging(false);

	if (m_App.EpgCaptureManager.IsCapturing()
			&& !m_App.EpgCaptureManager.IsChannelChanging()
			&& !(Status & AppEvent::ChannelChangeStatus::Detected))
		m_App.EpgCaptureManager.EndCapture(CEpgCaptureManager::EndFlag::None);

	if (CurSpace > CChannelManager::SPACE_INVALID)
		m_App.MainMenu.CheckRadioItem(
			CM_SPACE_ALL, CM_SPACE_ALL + m_App.ChannelManager.NumSpaces(),
			CM_SPACE_FIRST + CurSpace);
	ClearMenu(m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_SERVICE));
	m_App.StatusView.UpdateItem(STATUS_ITEM_CHANNEL);
	m_App.StatusView.UpdateItem(STATUS_ITEM_TUNER);
	m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_CHANNEL);
	m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_TUNER);

	if (pCurChannel != nullptr && m_App.OSDOptions.IsOSDEnabled(COSDOptions::OSDType::Channel))
		ShowChannelOSD();

	m_fNeedEventInfoOSD = false;
	if (!m_App.OSDManager.IsEventInfoOSDVisible()
			&& m_App.OSDOptions.GetEventInfoOSDAutoShowChannelChange()) {
		if (!m_App.UICore.ShowEventInfoOSD(COSDManager::EventInfoOSDFlag::Auto)) {
			// 番組情報が取得できるまで保留
			m_fNeedEventInfoOSD = true;
		}
	}

	const bool fEpgLoading = m_App.EpgOptions.IsEpgDataLoading();
	m_App.Panel.EnableProgramListUpdate(!fEpgLoading);
	m_Timer.BeginTimer(TIMER_ID_PROGRAMLISTUPDATE, 10000);
	m_ProgramListUpdateTimerCount = 0;
	m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_SERVICE);
	m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_PROGRAMINFO);

	const CChannelInfo *pProgramListChannel = nullptr;
	CChannelInfo Channel;
	if (pCurChannel != nullptr && pCurChannel->GetServiceID() != 0) {
		pProgramListChannel = pCurChannel;
	} else {
		if (m_App.Core.GetCurrentStreamChannelInfo(&Channel, true) && Channel.GetServiceID() != 0)
			pProgramListChannel = &Channel;
	}
	m_App.Panel.ProgramListPanel.ShowRetrievingMessage(pProgramListChannel != nullptr);
	m_App.Panel.ProgramListPanel.SetCurrentChannel(pProgramListChannel);
	m_App.Panel.ProgramListPanel.SelectChannel(pProgramListChannel, !fEpgLoading);

	if (fSpaceChanged) {
		if (IsPanelVisible() && m_App.Panel.Form.GetCurPageID() == PANEL_ID_CHANNEL) {
			m_App.Panel.ChannelPanel.SetChannelList(
				m_App.ChannelManager.GetCurrentChannelList(), !fEpgLoading);
			m_App.Panel.ChannelPanel.SetCurrentChannel(
				m_App.ChannelManager.GetCurrentChannel());
		} else {
			m_App.Panel.ChannelPanel.ClearChannelList();
		}
	} else {
		if (IsPanelVisible() && m_App.Panel.Form.GetCurPageID() == PANEL_ID_CHANNEL)
			m_App.Panel.ChannelPanel.SetCurrentChannel(m_App.ChannelManager.GetCurrentChannel());
	}
	if (pCurChannel != nullptr) {
		m_App.Epg.ProgramGuide.SetCurrentService(
			pCurChannel->GetNetworkID(),
			pCurChannel->GetTransportStreamID(),
			pCurChannel->GetServiceID());
	} else {
		m_App.Epg.ProgramGuide.ClearCurrentService();
	}
	int ChannelNo;
	if (pCurChannel != nullptr)
		ChannelNo = pCurChannel->GetChannelNo();
	m_pCore->SetCommandRadioCheckedState(
		CM_CHANNELNO_1, CM_CHANNELNO_12,
		pCurChannel != nullptr && ChannelNo >= 1 && ChannelNo <= 12 ?
		CM_CHANNELNO_1 + ChannelNo - 1 : 0);
	if (fSpaceChanged && m_App.SideBarOptions.GetShowChannelLogo())
		m_App.SideBar.Invalidate();
	m_App.Panel.CaptionPanel.Reset();
	m_App.Panel.UpdateControlPanel();

	const LPCTSTR pszDriverFileName = m_App.CoreEngine.GetDriverFileName();
	pCurChannel = m_App.ChannelManager.GetCurrentChannelInfo();
	if (pCurChannel != nullptr) {
		m_App.RecentChannelList.Add(pszDriverFileName, pCurChannel);
		m_App.ChannelHistory.SetCurrentChannel(pszDriverFileName, pCurChannel);
		m_App.TaskbarManager.AddRecentChannel(CTunerChannelInfo(*pCurChannel, pszDriverFileName));
		m_App.TaskbarManager.UpdateJumpList();
	}
	if (m_App.DriverOptions.IsResetChannelChangeErrorCount(pszDriverFileName))
		m_Timer.BeginTimer(TIMER_ID_RESETERRORCOUNT, 5000);
	else
		m_Timer.EndTimer(TIMER_ID_RESETERRORCOUNT);
	m_fForceResetPanAndScan = true;

	m_pCore->UpdateTitle();
	m_pCore->UpdateIcon();
}


void CMainWindow::LockLayout()
{
	if (!::IsIconic(m_hwnd) && !::IsZoomed(m_hwnd)) {
		m_fLockLayout = true;
		m_LayoutBase.LockLayout();
	}
}


void CMainWindow::UpdateLayout()
{
	if (m_fLockLayout) {
		m_fLockLayout = false;

		SIZE sz;
		GetClientSize(&sz);
		OnSizeChanged(SIZE_RESTORED, sz.cx, sz.cy);
		m_LayoutBase.UnlockLayout();
	}
}


void CMainWindow::UpdateLayoutStructure()
{
	Layout::CSplitter *pSideBarSplitter, *pTitleSplitter, *pPanelSplitter, *pStatusSplitter;

	pSideBarSplitter =
		dynamic_cast<Layout::CSplitter*>(
			m_LayoutBase.GetContainerByID(CONTAINER_ID_SIDEBARSPLITTER));
	pTitleSplitter =
		dynamic_cast<Layout::CSplitter*>(
			m_LayoutBase.GetContainerByID(CONTAINER_ID_TITLEBARSPLITTER));
	pPanelSplitter =
		dynamic_cast<Layout::CSplitter*>(
			m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));
	pStatusSplitter =
		dynamic_cast<Layout::CSplitter*>(
			m_LayoutBase.GetContainerByID(CONTAINER_ID_STATUSSPLITTER));
	if (pSideBarSplitter == nullptr || pTitleSplitter == nullptr
			|| pPanelSplitter == nullptr || pStatusSplitter == nullptr)
		return;

	const int PanelPane = GetPanelPaneIndex();
	Layout::CSplitter *pRoot;

	pPanelSplitter->SetStyle(
		m_fPanelVerticalAlign ? Layout::CSplitter::StyleFlag::Vert : Layout::CSplitter::StyleFlag::Horz,
		false);
	pPanelSplitter->ReplacePane(PanelPane, m_LayoutBase.GetContainerByID(CONTAINER_ID_PANEL));

	if (m_fPanelVerticalAlign) {
		pStatusSplitter->ReplacePane(0, pSideBarSplitter);
		pStatusSplitter->SetAdjustPane(CONTAINER_ID_SIDEBARSPLITTER);
		pPanelSplitter->ReplacePane(1 - PanelPane, pStatusSplitter);
		pPanelSplitter->SetAdjustPane(CONTAINER_ID_STATUSSPLITTER);
		pTitleSplitter->ReplacePane(1, pPanelSplitter);
		pTitleSplitter->SetAdjustPane(CONTAINER_ID_PANELSPLITTER);
		pRoot = pTitleSplitter;
	} else if (m_fSplitTitleBar) {
		pTitleSplitter->ReplacePane(1, pSideBarSplitter);
		pTitleSplitter->SetAdjustPane(CONTAINER_ID_SIDEBARSPLITTER);
		pPanelSplitter->ReplacePane(1 - PanelPane, pTitleSplitter);
		pPanelSplitter->SetAdjustPane(CONTAINER_ID_TITLEBARSPLITTER);
		pStatusSplitter->ReplacePane(0, pPanelSplitter);
		pStatusSplitter->SetAdjustPane(CONTAINER_ID_PANELSPLITTER);
		pRoot = pStatusSplitter;
	} else {
		pPanelSplitter->ReplacePane(1 - PanelPane, pSideBarSplitter);
		pPanelSplitter->SetAdjustPane(CONTAINER_ID_SIDEBARSPLITTER);
		pTitleSplitter->ReplacePane(1, pPanelSplitter);
		pTitleSplitter->SetAdjustPane(CONTAINER_ID_PANELSPLITTER);
		pStatusSplitter->ReplacePane(0, pTitleSplitter);
		pStatusSplitter->SetAdjustPane(CONTAINER_ID_PANELSPLITTER);
		pRoot = pStatusSplitter;
	}

	m_LayoutBase.SetTopContainer(pRoot);
}


void CMainWindow::AdjustWindowSizeOnDockPanel(bool fDock)
{
	// パネルの幅に合わせてウィンドウサイズを拡縮
	RECT rc;

	GetPosition(&rc);
	GetPanelDockingAdjustedPos(fDock, &rc);
	SetPosition(&rc);
	if (!fDock)
		::SetFocus(m_hwnd);
}


void CMainWindow::GetPanelDockingAdjustedPos(bool fDock, RECT *pPos) const
{
	const Layout::CSplitter *pSplitter =
		dynamic_cast<Layout::CSplitter*>(m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));
	int Width = m_App.Panel.Frame.GetDockingWidth() + pSplitter->GetBarWidth();
	int Height = m_App.Panel.Frame.GetDockingHeight() + pSplitter->GetBarWidth();

	if (!fDock) {
		Width = -Width;
		Height = -Height;
	}
	if (pSplitter->GetPane(0)->GetID() == CONTAINER_ID_PANEL) {
		if (m_App.Panel.Frame.IsDockingVertical())
			pPos->top -= Height;
		else
			pPos->left -= Width;
	} else {
		if (m_App.Panel.Frame.IsDockingVertical())
			pPos->bottom += Height;
		else
			pPos->right += Width;
	}

	if (fDock) {
		const HMONITOR hMonitor = ::MonitorFromRect(pPos, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi;

		mi.cbSize = sizeof(MONITORINFO);
		if (::GetMonitorInfo(hMonitor, &mi)) {
			int XOffset = 0, YOffset = 0;
			POINT pt = {pPos->left, pPos->top};

			if (::MonitorFromPoint(pt, MONITOR_DEFAULTTONULL) == nullptr) {
				if (pPos->left < mi.rcWork.left)
					XOffset = mi.rcWork.left - pPos->left;
				if (pPos->top < mi.rcWork.top)
					YOffset = mi.rcWork.top - pPos->top;
			}
			if (XOffset == 0 || YOffset == 0) {
				pt.x = pPos->right;
				pt.y = pPos->bottom;
				if (::MonitorFromPoint(pt, MONITOR_DEFAULTTONULL) == nullptr) {
					if (XOffset == 0 && pPos->right > mi.rcWork.right) {
						XOffset = mi.rcWork.right - pPos->right;
						if (pPos->left + XOffset < mi.rcWork.left)
							XOffset = mi.rcWork.left - pPos->left;
					}
					if (YOffset == 0 && pPos->bottom > mi.rcWork.bottom) {
						YOffset = mi.rcWork.bottom - pPos->bottom;
						if (pPos->top + YOffset < mi.rcWork.top)
							YOffset = mi.rcWork.top - pPos->top;
					}
				}
			}
			if (XOffset != 0 || YOffset != 0)
				::OffsetRect(pPos, XOffset, YOffset);
		}
	}
}


void CMainWindow::SetFixedPaneSize(int SplitterID, int ContainerID, int Width, int Height)
{
	Layout::CWindowContainer *pContainer =
		dynamic_cast<Layout::CWindowContainer*>(m_LayoutBase.GetContainerByID(ContainerID));

	if (pContainer != nullptr) {
		pContainer->SetMinSize(Width, Height);

		Layout::CSplitter *pSplitter =
			dynamic_cast<Layout::CSplitter*>(m_LayoutBase.GetContainerByID(SplitterID));
		if (pSplitter != nullptr) {
			pSplitter->SetPaneSize(
				ContainerID,
				!!(pSplitter->GetStyle() & Layout::CSplitter::StyleFlag::Vert) ? Height : Width);
		}
	}
}


void CMainWindow::ShowPopupTitleBar(bool fShow)
{
	if (fShow) {
		const CViewWindow &ViewWindow = GetCurrentViewWindow();
		RECT rcClient, rcTitle;

		ViewWindow.GetClientRect(&rcClient);
		m_TitleBarManager.Layout(&rcClient, &rcTitle);
		MapWindowRect(ViewWindow.GetHandle(), m_TitleBar.GetParent(), &rcTitle);
		m_TitleBar.SetPosition(&rcTitle);
		m_TitleBar.SetVisible(true);
		::BringWindowToTop(m_TitleBar.GetHandle());
	} else {
		m_TitleBar.SetVisible(false);
	}
}


void CMainWindow::ShowPopupStatusBar(bool fShow)
{
	if (fShow) {
		const CViewWindow &ViewWindow = GetCurrentViewWindow();
		RECT rc;

		ViewWindow.GetClientRect(&rc);
		MapWindowRect(ViewWindow.GetHandle(), m_App.StatusView.GetParent(), &rc);
		rc.top = rc.bottom - m_App.StatusView.CalcHeight(rc.right - rc.left);
		m_App.StatusView.SetPosition(&rc);
		m_App.StatusView.SetOpacity(m_App.StatusOptions.GetPopupOpacity() * 255 / CStatusOptions::OPACITY_MAX);
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
		m_App.SideBar.SetOpacity(m_App.SideBarOptions.GetPopupOpacity() * 255 / CSideBarOptions::OPACITY_MAX);
		m_App.SideBar.SetVisible(true);
		::BringWindowToTop(m_App.SideBar.GetHandle());
	} else {
		m_App.SideBar.SetVisible(false);
		m_App.SideBar.SetOpacity(255);
	}
}


void CMainWindow::ShowCursor(bool fShow)
{
	LibISDB::ViewerFilter *pViewer = m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();
	if (pViewer != nullptr)
		pViewer->HideCursor(!fShow);
	m_Display.GetViewWindow().SetShowCursor(fShow);
	m_fShowCursor = fShow;

	POINT pt;
	::GetCursorPos(&pt);
	for (HWND hwnd = ::WindowFromPoint(pt); hwnd != nullptr; hwnd = ::GetAncestor(hwnd, GA_PARENT)) {
		if (hwnd == m_Display.GetViewWindow().GetHandle()) {
			SendMessage(WM_SETCURSOR, reinterpret_cast<WPARAM>(hwnd), MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
			break;
		}
	}
}


void CMainWindow::ShowChannelOSD()
{
	if (GetVisible() && !::IsIconic(m_hwnd)) {
		const CChannelInfo *pInfo;

		if (m_fWheelChannelChanging)
			pInfo = m_App.ChannelManager.GetChangingChannelInfo();
		else
			pInfo = m_App.ChannelManager.GetCurrentChannelInfo();
		if (pInfo != nullptr) {
			String Text;

			if (m_App.OSDOptions.GetChannelChangeType() != COSDOptions::ChannelChangeType::LogoOnly) {
				CEventVariableStringMap::EventInfo Event;

				m_App.Core.GetVariableStringEventInfo(&Event);
				if (Event.Event.EventName.empty()) {
					LibISDB::DateTime Time;
					LibISDB::EventInfo EventData;

					LibISDB::GetCurrentEPGTime(&Time);
					if (m_App.EPGDatabase.GetEventInfo(
								pInfo->GetNetworkID(),
								pInfo->GetTransportStreamID(),
								pInfo->GetServiceID(),
								Time, &EventData))
						Event.Event = EventData;
				}
				CUICore::CTitleStringMap Map(m_App, &Event);
				FormatVariableString(&Map, m_App.OSDOptions.GetChannelChangeText(), &Text);
			}
			m_App.OSDManager.ShowChannelOSD(pInfo, Text.c_str(), m_fWheelChannelChanging);
		}
	}
}


void CMainWindow::OnServiceChanged()
{
	int CurService = 0;
	const WORD ServiceID = m_App.CoreEngine.GetServiceID();
	if (ServiceID != LibISDB::SERVICE_ID_INVALID)
		CurService = m_App.CoreEngine.GetSelectableServiceIndexByID(ServiceID);
	m_App.MainMenu.CheckRadioItem(
		CM_SERVICE_FIRST,
		CM_SERVICE_FIRST + m_App.CoreEngine.GetSelectableServiceCount() - 1,
		CM_SERVICE_FIRST + CurService);

	m_App.StatusView.UpdateItem(STATUS_ITEM_CHANNEL);

	m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_SERVICE);
	if (m_App.Panel.Form.GetCurPageID() == PANEL_ID_INFORMATION)
		m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_PROGRAMINFO);

	OnAudioStreamChanged(m_pCore->GetAudioStream());

	m_pCore->UpdateTitle();
}


void CMainWindow::OnEventChanged()
{
	const WORD EventID = m_App.CoreEngine.GetEventID();

	// 番組の最後まで録画
	if (m_App.RecordManager.GetStopOnEventEnd())
		m_App.Core.StopRecord();

	m_pCore->UpdateTitle();

	if (m_App.NotificationBarOptions.IsNotifyEnabled(CNotificationBarOptions::NOTIFY_EVENTNAME)
			&& !m_App.Core.IsChannelScanning()) {
		LibISDB::EventInfo EventInfo;

		if (m_App.CoreEngine.GetCurrentEventInfo(&EventInfo)) {
			String Text;

			if (EventInfo.StartTime.IsValid()) {
				TCHAR szTime[EpgUtil::MAX_EVENT_TIME_LENGTH];

				if (EpgUtil::FormatEventTime(EventInfo, szTime, lengthof(szTime)) > 0) {
					Text = szTime;
					Text += _T(' ');
				}
			}
			Text += EventInfo.EventName;
			ShowNotificationBar(Text.c_str(), CNotificationBar::MessageType::Info, 0, true);
		}
	}

	if (m_App.OSDManager.IsEventInfoOSDVisible()) {
		m_App.UICore.ShowEventInfoOSD();
	} else if (m_fNeedEventInfoOSD
			|| m_App.OSDOptions.GetEventInfoOSDAutoShowEventChange()) {
		m_App.UICore.ShowEventInfoOSD(COSDManager::EventInfoOSDFlag::Auto);
		m_fNeedEventInfoOSD = false;
	}

	if (IsPanelVisible()
			&& m_App.Panel.Form.GetCurPageID() == PANEL_ID_INFORMATION)
		m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_PROGRAMINFO);

	m_App.StatusView.UpdateItem(STATUS_ITEM_PROGRAMINFO);

	m_App.Panel.ProgramListPanel.SetCurrentEventID(EventID);
	m_App.Epg.ProgramGuide.SetCurrentEvent(EventID);

	const int AspectRatioType = m_pCore->GetAspectRatioType();
	if (AspectRatioType != CUICore::ASPECTRATIO_DEFAULT
			&& (m_fForceResetPanAndScan
				|| (m_App.VideoOptions.GetResetPanScanEventChange()
					&& AspectRatioType < CUICore::ASPECTRATIO_CUSTOM_FIRST))) {
		LibISDB::ViewerFilter *pViewer = m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();
		if (pViewer != nullptr)
			pViewer->SetPanAndScan(0, 0);
		if (!m_pCore->GetFullscreen()
				&& IsViewerEnabled()) {
			AutoFitWindowToVideo();
			// この時点でまだ新しい映像サイズが取得できない場合があるため、
			// WM_APP_VIDEOSIZECHANGED が来た時に調整するようにする
			m_AspectRatioResetTime = ::GetTickCount();
		}
		m_pCore->ResetAspectRatioType();
		m_fForceResetPanAndScan = false;
		m_App.StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
		m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VIDEO);
		m_pCore->SetCommandRadioCheckedState(
			CM_ASPECTRATIO_FIRST, CM_ASPECTRATIO_3D_LAST,
			CM_ASPECTRATIO_DEFAULT);
	}

	m_App.StatusView.UpdateItem(STATUS_ITEM_AUDIOCHANNEL);
	m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_AUDIO);


}


void CMainWindow::OnRecordingStarted()
{
	if (m_App.EpgCaptureManager.IsCapturing())
		m_App.EpgCaptureManager.EndCapture(CEpgCaptureManager::EndFlag::None);

	m_App.StatusView.UpdateItem(STATUS_ITEM_RECORD);
	m_App.StatusView.UpdateItem(STATUS_ITEM_ERROR);
	//m_App.MainMenu.EnableItem(CM_RECORDOPTION, false);
	//m_App.MainMenu.EnableItem(CM_RECORDSTOPTIME, true);
	m_App.TaskbarManager.SetRecordingStatus(true);

	m_Timer.EndTimer(TIMER_ID_RESETERRORCOUNT);
	m_fAlertedLowFreeSpace = false;
	if (m_App.OSDOptions.IsOSDEnabled(COSDOptions::OSDType::Recording))
		m_App.OSDManager.ShowOSD(TEXT("●録画"));

	m_pCore->UpdateTitle();
}


void CMainWindow::OnRecordingStopped()
{
	m_App.StatusView.UpdateItem(STATUS_ITEM_RECORD);
	m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_RECORD);
	//m_App.MainMenu.EnableItem(CM_RECORDOPTION, true);
	//m_App.MainMenu.EnableItem(CM_RECORDSTOPTIME, false);
	m_App.TaskbarManager.SetRecordingStatus(false);
	m_App.RecordManager.SetStopOnEventEnd(false);
	if (m_App.OSDOptions.IsOSDEnabled(COSDOptions::OSDType::Recording))
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
	m_pCore->SetCommandCheckedState(CM_1SEGMODE, f1SegMode);
}


void CMainWindow::OnMouseWheel(WPARAM wParam, LPARAM lParam, bool fHorz)
{
	const POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

	if (m_Display.GetDisplayBase().IsVisible()) {
		CDisplayView *pDisplayView = m_Display.GetDisplayBase().GetDisplayView();

		if (pDisplayView != nullptr) {
			RECT rc;

			m_Display.GetDisplayBase().GetParent()->GetScreenPosition(&rc);
			if (::PtInRect(&rc, pt)) {
				if (pDisplayView->OnMouseWheel(fHorz ? WM_MOUSEHWHEEL : WM_MOUSEWHEEL, wParam, lParam))
					return;
			}
		}
	}

	int Command;

	if (fHorz) {
		Command = m_App.OperationOptions.GetWheelTiltCommand();
	} else {
		if ((wParam & MK_SHIFT) != 0)
			Command = m_App.OperationOptions.GetWheelShiftCommand();
		else if ((wParam & MK_CONTROL) != 0)
			Command = m_App.OperationOptions.GetWheelCtrlCommand();
		else
			Command = m_App.OperationOptions.GetWheelCommand();
	}

	if (m_App.OperationOptions.IsStatusBarWheelEnabled() && m_App.StatusView.GetVisible()) {
		RECT rc;

		m_App.StatusView.GetScreenPosition(&rc);
		if (::PtInRect(&rc, pt)) {
			const int ItemID = m_App.StatusView.GetCurItem();

			if (ItemID >= 0) {
				CStatusItem *pItem = m_App.StatusView.GetItemByID(ItemID);

				if (pItem != nullptr) {
					POINT ptItem = pt;
					RECT rcItem;

					::ScreenToClient(m_App.StatusView.GetHandle(), &ptItem);
					pItem->GetRect(&rcItem);
					ptItem.x -= rcItem.left;
					ptItem.y -= rcItem.top;

					int NewCommand = 0;
					if (pItem->OnMouseWheel(
								ptItem.x, ptItem.y,
								fHorz, GET_WHEEL_DELTA_WPARAM(wParam),
								&NewCommand))
						Command = NewCommand;
				}
			}
		}
	}

	if (Command == 0) {
		m_PrevWheelCommand = 0;
		return;
	}

	int Delta = m_WheelHandler.OnMouseWheel(wParam, 1);
	if (Delta == 0)
		return;
	if (m_App.OperationOptions.IsWheelCommandReverse(Command))
		Delta = -Delta;
	const DWORD CurTime = ::GetTickCount();
	bool fProcessed = false;

	if (Command != m_PrevWheelCommand)
		m_WheelCount = 0;
	else
		m_WheelCount++;

	switch (Command) {
	case CM_WHEEL_VOLUME:
		SendCommand(Delta > 0 ? CM_VOLUME_UP : CM_VOLUME_DOWN);
		fProcessed = true;
		break;

	case CM_WHEEL_CHANNEL:
		{
			bool fUp;

			if (fHorz)
				fUp = Delta > 0;
			else
				fUp = Delta < 0;
			const int Channel = m_pCore->GetNextChannel(fUp);
			if (Channel >= 0) {
				if (m_fWheelChannelChanging
						&& m_WheelCount < 5
						&& TickTimeSpan(m_PrevWheelTime, CurTime) < (5UL - m_WheelCount) * 100UL) {
					break;
				}
				SetWheelChannelChanging(true, m_App.OperationOptions.GetWheelChannelDelay());
				m_App.ChannelManager.SetChangingChannel(Channel);
				m_App.StatusView.UpdateItem(STATUS_ITEM_CHANNEL);
				if (m_App.OSDOptions.IsOSDEnabled(COSDOptions::OSDType::Channel))
					ShowChannelOSD();
			}
			fProcessed = true;
		}
		break;

	case CM_WHEEL_AUDIO:
		if (Command != m_PrevWheelCommand || TickTimeSpan(m_PrevWheelTime, CurTime) >= 300) {
			SendCommand(CM_SWITCHAUDIO);
			fProcessed = true;
		}
		break;

	case CM_WHEEL_ZOOM:
		if (Command != m_PrevWheelCommand || TickTimeSpan(m_PrevWheelTime, CurTime) >= 500) {
			if (!IsZoomed(m_hwnd) && !m_pCore->GetFullscreen()) {
				int ZoomNum, ZoomDenom;
				if (m_pCore->GetZoomRate(&ZoomNum, &ZoomDenom)) {
					int Step = m_App.OperationOptions.GetWheelZoomStep();
					if (Delta < 0)
						Step = -Step;
					if (ZoomDenom >= 100) {
						ZoomNum += ::MulDiv(Step, ZoomDenom, 100);
					} else {
						ZoomNum = ZoomNum * 100 + Step * ZoomDenom;
						ZoomDenom *= 100;
					}
					SetZoomRate(ZoomNum, ZoomDenom);
				}
			}
			fProcessed = true;
		}
		break;

	case CM_WHEEL_ASPECTRATIO:
		if (Command != m_PrevWheelCommand || TickTimeSpan(m_PrevWheelTime, CurTime) >= 300) {
			SendCommand(CM_ASPECTRATIO_TOGGLE);
			fProcessed = true;
		}
		break;

	case CM_WHEEL_AUDIODELAY:
		SendCommand(Delta > 0 ? CM_AUDIODELAY_MINUS : CM_AUDIODELAY_PLUS);
		fProcessed = true;
		break;
	}

	m_PrevWheelCommand = Command;
	if (fProcessed)
		m_PrevWheelTime = CurTime;
}


bool CMainWindow::EnableViewer(bool fEnable)
{
	const LibISDB::ViewerFilter *pViewer = m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();

	if (fEnable) {
		bool fInit;

		if (!pViewer->IsOpen()) {
			fInit = true;
		} else {
			const BYTE VideoStreamType = m_App.CoreEngine.GetVideoStreamType();
			fInit =
				VideoStreamType != LibISDB::STREAM_TYPE_UNINITIALIZED &&
				VideoStreamType != pViewer->GetVideoStreamType();
		}

		if (fInit) {
			if (!InitializeViewer())
				return false;
		}
	}

	if (pViewer->IsOpen()) {
		if (!m_Display.EnableViewer(fEnable))
			return false;

		m_pCore->PreventDisplaySave(fEnable);
	}

	m_fEnablePlayback = fEnable;

	m_pCore->SetCommandCheckedState(CM_DISABLEVIEWER, !fEnable);

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
	if (m_App.OSDOptions.IsOSDEnabled(COSDOptions::OSDType::Volume)
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
	m_pCore->SetCommandCheckedState(CM_VOLUME_MUTE, false);
}


void CMainWindow::OnMuteChanged(bool fMute)
{
	m_App.StatusView.UpdateItem(STATUS_ITEM_VOLUME);
	m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VOLUME);
	m_pCore->SetCommandCheckedState(CM_VOLUME_MUTE, fMute);
}


void CMainWindow::OnDualMonoModeChanged(LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode Mode)
{
	m_App.StatusView.UpdateItem(STATUS_ITEM_AUDIOCHANNEL);
	m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_AUDIO);
}


void CMainWindow::OnAudioStreamChanged(int Stream)
{
	m_App.StatusView.UpdateItem(STATUS_ITEM_AUDIOCHANNEL);
	m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_AUDIO);
}


void CMainWindow::OnStartupDone()
{
	// TODO: ステータスバーに時計が表示されている場合のみタイマーを有効にする
	if (m_fAccurateClock)
		m_ClockUpdateTimer.Begin(1000, 200);
}


void CMainWindow::OnVariableChanged()
{
	m_pCore->UpdateTitle();
}


bool CMainWindow::SetZoomRate(int Rate, int Factor)
{
	if (Rate < 1 || Factor < 1)
		return false;

	int Width, Height;

	if (m_App.CoreEngine.GetVideoViewSize(&Width, &Height) && Width > 0 && Height > 0) {
		int ZoomWidth = CalcZoomSize(Width, Rate, Factor);
		int ZoomHeight = CalcZoomSize(Height, Rate, Factor);

		if (m_App.ViewOptions.GetZoomKeepAspectRatio()) {
			SIZE ScreenSize;

			m_Display.GetVideoContainer().GetClientSize(&ScreenSize);
			if (ScreenSize.cx > 0 && ScreenSize.cy > 0) {
				if (static_cast<double>(ZoomWidth) / static_cast<double>(ScreenSize.cx) <=
						static_cast<double>(ZoomHeight) / static_cast<double>(ScreenSize.cy)) {
					ZoomWidth = CalcZoomSize(ScreenSize.cx, ZoomHeight, ScreenSize.cy);
				} else {
					ZoomHeight = CalcZoomSize(ScreenSize.cy, ZoomWidth, ScreenSize.cx);
				}
			}
		}

		AdjustWindowSize(ZoomWidth, ZoomHeight);
	}

	return true;
}


bool CMainWindow::GetZoomRate(int *pRate, int *pFactor)
{
	bool fOK = false;
	int Width, Height;
	int Rate = 0, Factor = 1;

	if (m_App.CoreEngine.GetVideoViewSize(&Width, &Height) && Width > 0 && Height > 0) {
		/*
		SIZE sz;

		m_Display.GetVideoContainer().GetClientSize(&sz);
		Rate = sz.cy;
		Factor = Height;
		*/
		const LibISDB::ViewerFilter *pViewer = m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();
		int DstWidth, DstHeight;
		if (pViewer != nullptr && pViewer->GetDestSize(&DstWidth, &DstHeight)) {
			Rate = DstHeight;
			Factor = Height;
		}
		fOK = true;
	}
	if (pRate)
		*pRate = Rate;
	if (pFactor)
		*pFactor = Factor;
	return fOK;
}


bool CMainWindow::AutoFitWindowToVideo()
{
	int Width, Height;
	SIZE sz;

	if (!m_App.CoreEngine.GetVideoViewSize(&Width, &Height)
			|| Width <= 0 || Height <= 0)
		return false;
	m_Display.GetVideoContainer().GetClientSize(&sz);
	Width = CalcZoomSize(Width, sz.cy, Height);
	if (sz.cx < Width)
		AdjustWindowSize(Width, sz.cy);

	return true;
}


void CMainWindow::OnPanAndScanChanged()
{
	if (!m_pCore->GetFullscreen()) {
		switch (m_App.ViewOptions.GetPanScanAdjustWindowMode()) {
		case CViewOptions::AdjustWindowMode::Fit:
			{
				int ZoomRate, ZoomFactor;
				int Width, Height;

				if (GetZoomRate(&ZoomRate, &ZoomFactor)
						&& m_App.CoreEngine.GetVideoViewSize(&Width, &Height)) {
					AdjustWindowSize(
						CalcZoomSize(Width, ZoomRate, ZoomFactor),
						CalcZoomSize(Height, ZoomRate, ZoomFactor));
				} else {
					const LibISDB::ViewerFilter *pViewer = m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();
					int DstWidth, DstHeight;

					if (pViewer != nullptr && pViewer->GetDestSize(&DstWidth, &DstHeight))
						AdjustWindowSize(DstWidth, DstHeight);
				}
			}
			break;

		case CViewOptions::AdjustWindowMode::Width:
			{
				SIZE sz;
				int Width, Height;

				m_Display.GetVideoContainer().GetClientSize(&sz);
				if (m_App.CoreEngine.GetVideoViewSize(&Width, &Height))
					AdjustWindowSize(CalcZoomSize(Width, sz.cy, Height), sz.cy);
			}
			break;
		}
	}

	m_App.StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
	m_App.Panel.ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VIDEO);
}


void CMainWindow::OnAspectRatioTypeChanged(int Type)
{
	m_AspectRatioResetTime = 0;
}


void CMainWindow::SetTitleText(LPCTSTR pszTitleText, LPCTSTR pszWindowText)
{
	const LPCTSTR pszWindow = m_fCustomTitleBar ? pszWindowText : pszTitleText;
	const int Length = ::GetWindowTextLength(m_hwnd) + 1;
	Util::CTempBuffer<TCHAR, 256> OldText(Length);
	::GetWindowText(m_hwnd, OldText.GetBuffer(), Length);
	if (::lstrcmp(pszWindow, OldText.GetBuffer()) != 0)
		::SetWindowText(m_hwnd, pszWindow);

	m_TitleBar.SetLabel(pszTitleText);
	if (m_pCore->GetFullscreen())
		::SetWindowText(m_Fullscreen.GetHandle(), pszTitleText);
	m_App.TaskTrayManager.SetTipText(pszTitleText);
}


bool CMainWindow::SetTitleFont(const Style::Font &Font)
{
	m_TitleBar.SetFont(Font);
	const int Height = m_TitleBar.GetHeight();
	Layout::CWindowContainer *pContainer = static_cast<Layout::CWindowContainer*>(
			m_LayoutBase.GetContainerByID(CONTAINER_ID_TITLEBAR));
	pContainer->SetMinSize(0, Height);
	Layout::CSplitter *pSplitter = static_cast<Layout::CSplitter*>(
		m_LayoutBase.GetContainerByID(CONTAINER_ID_TITLEBARSPLITTER));
	pSplitter->SetPaneSize(CONTAINER_ID_TITLEBAR, Height);

	m_Fullscreen.SetTitleFont(Font);

	return true;
}


bool CMainWindow::SetLogo(HBITMAP hbm)
{
	return m_Display.GetViewWindow().SetLogo(hbm);
}


void CMainWindow::BeginWheelChannelSelect(DWORD Delay)
{
	if (Delay > 0)
		m_Timer.BeginTimer(TIMER_ID_WHEELCHANNELSELECT, Delay);
}


void CMainWindow::EndWheelChannelSelect()
{
	m_Timer.EndTimer(TIMER_ID_WHEELCHANNELSELECT);
}


bool CMainWindow::ShowProgramGuide(bool fShow, ShowProgramGuideFlag Flags, const ProgramGuideSpaceInfo *pSpaceInfo)
{
	if (m_App.Epg.fShowProgramGuide == fShow)
		return true;

	if (fShow) {
		const bool fOnScreen =
			!(Flags & ShowProgramGuideFlag::Popup)
			&& (!!(Flags & ShowProgramGuideFlag::OnScreen)
				|| m_App.ProgramGuideOptions.GetOnScreen()
				|| (m_pCore->GetFullscreen() && ::GetSystemMetrics(SM_CMONITORS) == 1));

		Util::CWaitCursor WaitCursor;

		if (fOnScreen) {
			m_App.Epg.ProgramGuideDisplay.SetStyleScaling(&m_StyleScaling);
			m_App.Epg.ProgramGuideDisplay.Create(
				m_Display.GetDisplayViewParent(), WS_CHILD | WS_CLIPCHILDREN);
			m_Display.GetDisplayBase().SetDisplayView(&m_App.Epg.ProgramGuideDisplay);
			if (m_fCustomFrame)
				HookWindows(m_App.Epg.ProgramGuideDisplay.GetHandle());
			RegisterUIChild(&m_App.Epg.ProgramGuideDisplay);
		} else {
			m_App.Epg.ProgramGuideFrame.Create(
				nullptr,
				WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX |
				WS_THICKFRAME | WS_CLIPCHILDREN);
		}

		LibISDB::DateTime First, Last;
		m_App.ProgramGuideOptions.GetTimeRange(&First, &Last);
		m_App.Epg.ProgramGuide.SetTimeRange(First, Last);
		m_App.Epg.ProgramGuide.SetViewDay(CProgramGuide::DAY_TODAY);

		if (fOnScreen) {
			m_Display.GetDisplayBase().SetVisible(true);
		} else {
			m_App.Epg.ProgramGuideFrame.Show();
			m_App.Epg.ProgramGuideFrame.Update();
		}

		if (m_App.EpgOptions.IsEpgFileLoading()) {
			m_App.Epg.ProgramGuide.SetMessage(TEXT("EPGファイルの読み込み中..."));
			m_App.EpgOptions.WaitEpgFileLoad();
			m_App.Epg.ProgramGuide.SetMessage(nullptr);
		}

		const CEpg::CChannelProviderManager *pChannelProviderManager =
			m_App.Epg.CreateChannelProviderManager(pSpaceInfo != nullptr ? pSpaceInfo->pszTuner : nullptr);
		const int Provider = pChannelProviderManager->GetCurChannelProvider();
		int Space;
		if (Provider >= 0) {
			const CProgramGuideChannelProvider *pChannelProvider =
				pChannelProviderManager->GetChannelProvider(Provider);
			bool fGroupID = false;
			if (pSpaceInfo != nullptr && pSpaceInfo->pszSpace != nullptr) {
				if (StringIsDigit(pSpaceInfo->pszSpace)) {
					Space = ::StrToInt(pSpaceInfo->pszSpace);
				} else {
					Space = pChannelProvider->ParseGroupID(pSpaceInfo->pszSpace);
					fGroupID = true;
				}
			} else {
				Space = m_App.ChannelManager.GetCurrentSpace();
			}
			if (Space < 0) {
				Space = 0;
			} else if (!fGroupID) {
				const CProgramGuideBaseChannelProvider *pBaseChannelProvider =
					dynamic_cast<const CProgramGuideBaseChannelProvider*>(pChannelProvider);
				if (pBaseChannelProvider != nullptr) {
					if (pBaseChannelProvider->HasAllChannelGroup())
						Space++;
					if (static_cast<size_t>(Space) >= pBaseChannelProvider->GetGroupCount())
						Space = 0;
				}
			}
		} else {
			Space = -1;
		}
		m_App.Epg.ProgramGuide.SetCurrentChannelProvider(Provider, Space);
		m_App.Epg.ProgramGuide.UpdateProgramGuide();
		if (m_App.ProgramGuideOptions.ScrollToCurChannel())
			m_App.Epg.ProgramGuide.ScrollToCurrentService();
	} else {
		if (m_App.Epg.ProgramGuideFrame.IsCreated()) {
			m_App.Epg.ProgramGuideFrame.Destroy();
		} else {
			m_Display.GetDisplayBase().SetVisible(false);
		}
	}

	m_App.Epg.fShowProgramGuide = fShow;

	m_pCore->SetCommandCheckedState(CM_PROGRAMGUIDE, m_App.Epg.fShowProgramGuide);

	return true;
}


bool CMainWindow::IsProgramGuideOnScreenDisplaying() const
{
	return m_App.Epg.fShowProgramGuide
		&& !m_App.Epg.ProgramGuideFrame.IsCreated();
}


bool CMainWindow::PostNotification(
	LPCTSTR pszText,
	unsigned int NotifyType,
	CNotificationBar::MessageType MessageType,
	bool fSkippable)
{
	if (IsStringEmpty(pszText))
		return false;

	BlockLock Lock(m_NotificationLock);

	NotificationInfo &Info = m_PendingNotificationList.emplace_back();

	Info.Text = pszText;
	Info.NotifyType = NotifyType;
	Info.MessageType = MessageType;
	Info.Duration = 6000;
	Info.fSkippable = fSkippable;

	if (!PostMessage(WM_APP_SHOWNOTIFICATIONBAR, 0, 0)) {
		m_PendingNotificationList.clear();
		return false;
	}

	return true;
}


LRESULT CALLBACK CMainWindow::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CMainWindow *pThis;

	if (uMsg == WM_NCCREATE) {
		pThis = static_cast<CMainWindow*>(CBasicWindow::OnCreate(hwnd, lParam));
	} else {
		pThis = static_cast<CMainWindow*>(GetBasicWindow(hwnd));
	}

	if (pThis == nullptr) {
		if (uMsg == WM_GETMINMAXINFO || uMsg == WM_NCCALCSIZE) {
			pThis = m_pThis;
			pThis->m_hwnd = hwnd;
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
		} else {
			return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
	}

	if (uMsg == WM_CREATE) {
		if (!pThis->OnCreate(reinterpret_cast<LPCREATESTRUCT>(lParam)))
			return -1;
		return 0;
	}

	LRESULT Result = 0;
	if (pThis->m_App.PluginManager.OnMessage(hwnd, uMsg, wParam, lParam, &Result))
		return Result;

	if (uMsg == WM_DESTROY) {
		pThis->OnMessage(hwnd, uMsg, wParam, lParam);
		::PostQuitMessage(0);
		return 0;
	}

	return pThis->OnMessage(hwnd, uMsg, wParam, lParam);
}


void CMainWindow::HookWindows(HWND hwnd)
{
	if (hwnd != nullptr) {
		HookChildWindow(hwnd);
		for (hwnd = ::GetWindow(hwnd, GW_CHILD); hwnd != nullptr; hwnd = ::GetWindow(hwnd, GW_HWNDNEXT))
			HookWindows(hwnd);
	}
}


void CMainWindow::HookChildWindow(HWND hwnd)
{
	if (hwnd == nullptr)
		return;

	// this のアドレスを ID として使う
	const UINT_PTR SubclassID = reinterpret_cast<UINT_PTR>(this);

	// 既にサブクラス化されているか確認
	DWORD_PTR RefData;
	if (::GetWindowSubclass(hwnd, ChildSubclassProc, SubclassID, &RefData) && RefData == SubclassID)
		return;

#ifdef _DEBUG
	{
		TCHAR szClass[256];
		::GetClassName(hwnd, szClass, lengthof(szClass));
		TRACE(TEXT("Hook window {} \"{}\"\n"), static_cast<void *>(hwnd), szClass);
	}
#endif
	::SetWindowSubclass(hwnd, ChildSubclassProc, SubclassID, reinterpret_cast<DWORD_PTR>(this));
}


LRESULT CALLBACK CMainWindow::ChildSubclassProc(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	CMainWindow *pThis = reinterpret_cast<CMainWindow*>(dwRefData);

	switch (uMsg) {
	case WM_NCHITTEST:
		if (pThis->m_fCustomFrame && !::IsZoomed(pThis->m_hwnd)
				&& ::GetAncestor(hwnd, GA_ROOT) == pThis->m_hwnd) {
			const POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			RECT rc;

			pThis->GetScreenPosition(&rc);
			if (::PtInRect(&rc, pt)) {
				Style::Margins FrameWidth = pThis->m_Style.ResizingMargin;
				if (FrameWidth.Left < pThis->m_CustomFrameWidth)
					FrameWidth.Left = pThis->m_CustomFrameWidth;
				if (FrameWidth.Top < pThis->m_CustomFrameWidth)
					FrameWidth.Top = pThis->m_CustomFrameWidth;
				if (FrameWidth.Right < pThis->m_CustomFrameWidth)
					FrameWidth.Right = pThis->m_CustomFrameWidth;
				if (FrameWidth.Bottom < pThis->m_CustomFrameWidth)
					FrameWidth.Bottom = pThis->m_CustomFrameWidth;
				const int CornerMarginLeft = FrameWidth.Left * 2;
				const int CornerMarginRight = FrameWidth.Right * 2;
				RECT rcFrame = rc;
				Style::Subtract(&rcFrame, FrameWidth);
				int Code = HTNOWHERE;

				if (pt.y < rcFrame.top) {
					if (pt.x < rcFrame.left + CornerMarginLeft)
						Code = HTTOPLEFT;
					else if (pt.x >= rcFrame.right - CornerMarginRight)
						Code = HTTOPRIGHT;
					else
						Code = HTTOP;
				} else if (pt.y >= rcFrame.bottom) {
					if (pt.x < rcFrame.left + CornerMarginLeft)
						Code = HTBOTTOMLEFT;
					else if (pt.x >= rcFrame.right - CornerMarginRight)
						Code = HTBOTTOMRIGHT;
					else
						Code = HTBOTTOM;
				} else if (pt.x < rcFrame.left) {
					Code = HTLEFT;
				} else if (pt.x >= rcFrame.right) {
					Code = HTRIGHT;
				}
				if (Code != HTNOWHERE) {
					return Code;
				}
			}
		}
		break;

	case WM_NCLBUTTONDOWN:
		if (pThis->m_fCustomFrame && ::GetAncestor(hwnd, GA_ROOT) == pThis->m_hwnd) {
			BYTE Flag = 0;

			switch (wParam) {
			case HTTOP:         Flag = WMSZ_TOP;         break;
			case HTTOPLEFT:     Flag = WMSZ_TOPLEFT;     break;
			case HTTOPRIGHT:    Flag = WMSZ_TOPRIGHT;    break;
			case HTLEFT:        Flag = WMSZ_LEFT;        break;
			case HTRIGHT:       Flag = WMSZ_RIGHT;       break;
			case HTBOTTOM:      Flag = WMSZ_BOTTOM;      break;
			case HTBOTTOMLEFT:  Flag = WMSZ_BOTTOMLEFT;  break;
			case HTBOTTOMRIGHT: Flag = WMSZ_BOTTOMRIGHT; break;
			}

			if (Flag != 0) {
				::SendMessage(pThis->m_hwnd, WM_SYSCOMMAND, SC_SIZE | Flag, lParam);
				return 0;
			}
		}
		break;

	case WM_DESTROY:
#ifdef _DEBUG
		{
			TCHAR szClass[256];
			::GetClassName(hwnd, szClass, lengthof(szClass));
			TRACE(TEXT("Unhook window {} \"{}\"\n"), static_cast<void *>(hwnd), szClass);
		}
#endif
		::RemoveWindowSubclass(hwnd, ChildSubclassProc, reinterpret_cast<UINT_PTR>(pThis));
		break;
	}

	return ::DefSubclassProc(hwnd, uMsg, wParam, lParam);
}


void CMainWindow::SetMaximizedRegion(bool fSet)
{
	// ウィンドウ枠を独自にしている場合、最大化時にクリッピングしないと
	// 枠が隣接するモニタの端に表示されてしまうことがある
	if (fSet) {
		RECT rcWindow, rcClient;

		::GetWindowRect(m_hwnd, &rcWindow);
		::GetClientRect(m_hwnd, &rcClient);
		MapWindowRect(m_hwnd, nullptr, &rcClient);
		if (rcWindow != rcClient) {
			::OffsetRect(&rcClient, -rcWindow.left, -rcWindow.top);
			const HRGN hrgn = ::CreateRectRgnIndirect(&rcClient);
			if (::SetWindowRgn(m_hwnd, hrgn, TRUE))
				m_fWindowRegionSet = true;
			else
				::DeleteObject(hrgn);
		}
	} else if (m_fWindowRegionSet) {
		::SetWindowRgn(m_hwnd, nullptr, TRUE);
		m_fWindowRegionSet = false;
	}
}


void CMainWindow::UpdateWindowFrame()
{
	m_fWindowFrameChanged = false;

	CAeroGlass Aero;
	Aero.EnableNcRendering(m_hwnd, !m_fCustomFrame);

	::SetWindowPos(
		m_hwnd, nullptr, 0, 0, 0, 0,
		SWP_FRAMECHANGED | SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
}


void CMainWindow::SetWindowVisible()
{
	bool fRestore = false, fShow = false;

	if (!!(m_App.TaskTrayManager.GetStatus() & CTaskTrayManager::StatusFlag::Minimized)) {
		m_App.TaskTrayManager.SetStatus(
			CTaskTrayManager::StatusFlag::None,
			CTaskTrayManager::StatusFlag::Minimized);
		m_App.TaskTrayManager.SetMinimizeToTray(m_App.ViewOptions.GetMinimizeToTray());
		fRestore = true;
	}
	if (!GetVisible()) {
		if (m_fMinimizeInit || m_fStandbyInit) {
			Show(SW_SHOWNORMAL);
			if (m_fMinimizeInit)
				fRestore = true;
		} else {
			SetVisible(true);
		}
		ForegroundWindow(m_hwnd);
		Update();
		fShow = true;
	}
	if (::IsIconic(m_hwnd)) {
		::ShowWindow(m_hwnd, SW_RESTORE);
		Update();
		fRestore = true;
	} else if (!fShow) {
		ForegroundWindow(m_hwnd);
	}
	if (m_fMinimizeInit) {
		// 最小化状態での起動後最初の表示
		ShowFloatingWindows(true);
		m_fMinimizeInit = false;
	}
	if (fRestore) {
		ResumeViewer(ResumeInfo::ViewerSuspendFlag::Minimize);
	}
}


void CMainWindow::ShowFloatingWindows(bool fShow, bool fNoProgramGuide)
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
	const HDC hdc = ::GetWindowDC(m_hwnd);
	{
		Theme::CThemeDraw ThemeDraw(BeginThemeDraw(hdc));
		const Theme::BackgroundStyle &Style =
			fActive ? m_Theme.ActiveFrameStyle : m_Theme.FrameStyle;
		RECT rc, rcEmpty;

		::GetWindowRect(m_hwnd, &rc);
		::OffsetRect(&rc, -rc.left, -rc.top);
		rcEmpty = rc;
		::InflateRect(&rcEmpty, -m_CustomFrameWidth, -m_CustomFrameWidth);
		ThemeDraw.Draw(Style.Border, &rc);
		DrawUtil::FillBorder(hdc, &rc, &rcEmpty, &rc, Style.Fill.GetSolidColor());
	}
	::ReleaseDC(m_hwnd, hdc);
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
		SuspendViewer(ResumeInfo::ViewerSuspendFlag::Standby);
		//FinalizeViewer();
		m_Resume.fFullscreen = m_pCore->GetFullscreen();
		if (m_Resume.fFullscreen)
			m_pCore->SetFullscreen(false);
		ShowFloatingWindows(false, m_App.GeneralOptions.GetStandaloneProgramGuide());
		SetVisible(false);

		if (!m_App.EpgCaptureManager.IsCapturing()) {
			StoreTunerResumeInfo();

			if (m_App.EpgOptions.GetUpdateWhenStandby()
					&& m_App.CoreEngine.IsTunerOpen()
					&& !m_App.RecordManager.IsRecording()
					&& !m_App.CoreEngine.IsNetworkDriver()
					&& !m_App.CmdLineOptions.m_fNoEpg) {
				m_App.EpgCaptureManager.BeginCapture(
					nullptr, nullptr,
					CEpgCaptureManager::BeginFlag::Standby | CEpgCaptureManager::BeginFlag::NoUI);
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
			const bool fSetChannel = m_Resume.fSetChannel;
			m_Resume.fSetChannel = false;
			ResumeTuner();
			m_Resume.fSetChannel = fSetChannel;
			m_App.Core.InitializeChannel();
			InitializeViewer();
			m_fStandbyInit = false;
		}
		if (m_Resume.fFullscreen)
			m_pCore->SetFullscreen(true);
		ShowFloatingWindows(true);
		ForegroundWindow(m_hwnd);
		ResumeTuner();
		ResumeViewer(ResumeInfo::ViewerSuspendFlag::Standby);
	}

	return true;
}


bool CMainWindow::EnablePlayback(bool fEnable)
{
	m_fEnablePlayback = fEnable;

	m_pCore->SetCommandCheckedState(CM_DISABLEVIEWER, !m_fEnablePlayback);

	return true;
}


bool CMainWindow::InitStandby()
{
	if (!m_App.CmdLineOptions.m_fNoDirectShow && !m_App.CmdLineOptions.m_fNoView
			&& (!m_App.PlaybackOptions.GetRestorePlayStatus() || m_App.GetEnablePlaybackOnStart())) {
		m_Resume.fEnableViewer = true;
		m_Resume.ViewerSuspendFlags = ResumeInfo::ViewerSuspendFlag::Standby;
	}
	m_Resume.fFullscreen = m_App.CmdLineOptions.m_fFullscreen;

	if (m_App.CoreEngine.IsDriverSpecified())
		m_Resume.fOpenTuner = true;

	if (m_App.RestoreChannelInfo.Space >= 0 && m_App.RestoreChannelInfo.Channel >= 0) {
		const int Space = m_App.RestoreChannelInfo.fAllChannels ? CChannelManager::SPACE_ALL : m_App.RestoreChannelInfo.Space;
		const CChannelList *pList = m_App.ChannelManager.GetChannelList(Space);
		if (pList != nullptr) {
			const int Index = pList->FindByIndex(
				m_App.RestoreChannelInfo.Space,
				m_App.RestoreChannelInfo.Channel,
				m_App.RestoreChannelInfo.ServiceID);
			if (Index >= 0) {
				m_Resume.Channel.SetSpace(Space);
				m_Resume.Channel.SetChannel(Index);
				m_Resume.Channel.SetServiceID(m_App.RestoreChannelInfo.ServiceID);
				m_Resume.fSetChannel = true;
			}
		}
	}

	m_fStandbyInit = true;
	m_pCore->SetStandby(true, true);

	return true;
}


bool CMainWindow::InitMinimize()
{
	if (!m_App.CmdLineOptions.m_fNoDirectShow && !m_App.CmdLineOptions.m_fNoView
			&& (!m_App.PlaybackOptions.GetRestorePlayStatus() || m_App.GetEnablePlaybackOnStart())
			&& m_App.ViewOptions.GetDisablePreviewWhenMinimized()) {
		m_Resume.fEnableViewer = true;
		m_Resume.ViewerSuspendFlags = ResumeInfo::ViewerSuspendFlag::Minimize;
	}

	m_App.TaskTrayManager.SetStatus(
		CTaskTrayManager::StatusFlag::Minimized,
		CTaskTrayManager::StatusFlag::Minimized);

	if (!m_App.TaskTrayManager.GetMinimizeToTray())
		Show(SW_SHOWMINNOACTIVE, true);

	m_fMinimizeInit = true;

	return true;
}


bool CMainWindow::IsMinimizeToTray() const
{
	return m_App.TaskTrayManager.GetMinimizeToTray()
		&& !!(m_App.TaskTrayManager.GetStatus() & CTaskTrayManager::StatusFlag::Minimized);
}


void CMainWindow::StoreTunerResumeInfo()
{
	m_Resume.Channel.Store(&m_App.ChannelManager);
	m_Resume.fSetChannel = m_Resume.Channel.IsValid();
	m_Resume.fOpenTuner = m_App.CoreEngine.IsTunerOpen();
}


bool CMainWindow::ResumeTuner()
{
	if (m_App.EpgCaptureManager.IsCapturing())
		m_App.EpgCaptureManager.EndCapture(CEpgCaptureManager::EndFlag::None);

	if (m_Resume.fOpenTuner) {
		m_Resume.fOpenTuner = false;
		if (!m_App.Core.OpenTuner()) {
			m_Resume.fSetChannel = false;
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
			m_App.Core.SetChannel(
				m_Resume.Channel.GetSpace(),
				m_Resume.Channel.GetChannel(),
				m_Resume.Channel.GetServiceID());
		}
		m_Resume.fSetChannel = false;
	}
}


void CMainWindow::SuspendViewer(ResumeInfo::ViewerSuspendFlag Flags)
{
	if (IsViewerEnabled()) {
		TRACE(TEXT("Suspend viewer\n"));
		m_pCore->EnableViewer(false);
		m_Resume.fEnableViewer = true;
	}
	m_Resume.ViewerSuspendFlags |= Flags;
}


void CMainWindow::ResumeViewer(ResumeInfo::ViewerSuspendFlag Flags)
{
	if (!!(m_Resume.ViewerSuspendFlags & Flags)) {
		m_Resume.ViewerSuspendFlags &= ~Flags;
		if (!m_Resume.ViewerSuspendFlags) {
			if (m_Resume.fEnableViewer) {
				TRACE(TEXT("Resume viewer\n"));
				m_pCore->EnableViewer(true);
				m_Resume.fEnableViewer = false;
			}
		}
	}
}


bool CMainWindow::ConfirmExit()
{
	return m_App.RecordOptions.ConfirmExit(GetVideoHostWindow(), &m_App.RecordManager);
}


bool CMainWindow::IsNoAcceleratorMessage(const MSG *pMsg)
{
	if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_KEYUP) {
		if (m_ChannelInput.IsKeyNeeded(pMsg->wParam))
			return true;
	}

	return false;
}


bool CMainWindow::BeginChannelNoInput(int Digits)
{
	if (m_ChannelInput.IsInputting())
		EndChannelNoInput(false);

	if (Digits < 0 || Digits > 5)
		return false;

	TRACE(TEXT("チャンネル番号{}桁入力開始\n"), Digits);

	m_ChannelInput.BeginInput(Digits);

	OnChannelNoInput();

	return true;
}


void CMainWindow::EndChannelNoInput(bool fDetermine)
{
	if (m_ChannelInput.IsInputting()) {
		TRACE(TEXT("チャンネル番号入力終了\n"));

		int Number = 0;
		if (fDetermine)
			Number = m_ChannelInput.GetNumber();

		m_ChannelInput.EndInput();
		m_Timer.EndTimer(TIMER_ID_CHANNELNO);
		m_App.OSDManager.ClearOSD();

		if (Number > 0)
			m_App.Core.SwitchChannelByNo(Number, true);
	}
}


void CMainWindow::OnChannelNoInput()
{
	if (m_App.OSDOptions.IsOSDEnabled(COSDOptions::OSDType::ChannelNoInput)) {
		TCHAR szText[16];
		const int MaxDigits = m_ChannelInput.GetMaxDigits();
		int Number = m_ChannelInput.GetNumber();
		if (MaxDigits > 0) {
			for (int i = MaxDigits - 1; i >= 0; i--) {
				if (i < m_ChannelInput.GetCurDigits()) {
					szText[i] = Number % 10 + _T('0');
					Number /= 10;
				} else {
					szText[i] = _T('-');
				}
			}
			szText[MaxDigits] = _T('\0');
		} else {
			StringFormat(szText, TEXT("{}"), Number);
		}
		m_App.OSDManager.ShowOSD(szText, COSDManager::ShowFlag::NoFade);
	}

	const DWORD Timeout = m_App.Accelerator.GetChannelInputOptions().KeyTimeout;
	if (Timeout > 0)
		m_Timer.BeginTimer(TIMER_ID_CHANNELNO, Timeout);
}


void CMainWindow::SetStyle(const Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);
}


void CMainWindow::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	m_Style.NormalizeStyle(pStyleManager, pStyleScaling);
}


void CMainWindow::SetTheme(const Theme::CThemeManager *pThemeManager)
{
	pThemeManager->GetBackgroundStyle(Theme::CThemeManager::STYLE_WINDOW_FRAME, &m_Theme.FrameStyle);
	pThemeManager->GetBackgroundStyle(Theme::CThemeManager::STYLE_WINDOW_ACTIVEFRAME, &m_Theme.ActiveFrameStyle);
	if (m_fCustomFrame)
		Redraw(nullptr, RDW_FRAME | RDW_INVALIDATE);

	m_LayoutBase.SetBackColor(pThemeManager->GetColor(CColorScheme::COLOR_SPLITTER));

	Theme::BorderStyle Border;
	pThemeManager->GetBorderStyle(Theme::CThemeManager::STYLE_SCREEN, &Border);
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
		pInfo->hwndParent = m_Display.GetVideoContainer().GetHandle();
	} else {
		pInfo->hwndParent = m_Display.GetVideoContainer().GetParent();
		pInfo->fForcePseudoOSD = true;
	}

	// まだ再生が開始されていない場合、アニメーションを無効にする
	// アニメーションの途中で DirectShow の初期化処理が入り、中途半端な表示になるため
	if (m_fEnablePlayback && !IsViewerEnabled())
		pInfo->fAnimation = false;

	RECT rc;
	::GetClientRect(pInfo->hwndParent, &rc);
	rc.top += m_NotificationBar.GetBarHeight();
	if (!m_fShowStatusBar && m_App.StatusOptions.GetShowPopup())
		rc.bottom -= m_App.StatusView.GetHeight();
	pInfo->ClientRect = rc;

	return true;
}


bool CMainWindow::SetOSDHideTimer(DWORD Delay)
{
	return m_Timer.BeginTimer(TIMER_ID_OSD, Delay);
}


CStatusView &CMainWindow::GetStatusView()
{
	return m_App.StatusView;
}


CSideBar &CMainWindow::GetSideBar()
{
	return m_App.SideBar;
}


/*
	メニューがオーナー描画か複数列の場合、テーマが無効で描画されてしまうため、
	サブクラス化して枠と背景を独自に描画する。
	現在のところ複数列のメニューは同時にオーナー描画であるため問題ないが、
	複数列でオーナー描画でないメニューが存在するならば追加の対応が必要になる。
*/
LRESULT CALLBACK CMainWindow::HookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION) {
		const CWPSTRUCT *pcwp = reinterpret_cast<const CWPSTRUCT *>(lParam);
		if (pcwp->message == WM_CREATE) {
			if (TVTest::IsDarkMode()) {
				const HWND hwnd = pcwp->hwnd;
				TCHAR szClass[32];
				if (::GetClassName(hwnd, szClass, lengthof(szClass)) == 6
						&& ::lstrcmp(szClass, TEXT("#32768")) == 0) {
					::SetWindowTheme(hwnd, L"DarkMode", nullptr);
					m_pThis->m_MenuPainter.emplace(
						std::piecewise_construct,
						std::forward_as_tuple(hwnd),
						std::forward_as_tuple()).first->second.Initialize(hwnd, GetWindowDPI(hwnd));
					::SetWindowSubclass(hwnd, MenuSubclassProc, 1, reinterpret_cast<DWORD_PTR>(m_pThis));
				}
			}
		}
	}

	return ::CallNextHookEx(m_hHook, nCode, wParam, lParam);
}


static bool IsMenuNeedCustomErase(HMENU hmenu)
{
	const int ItemCount = ::GetMenuItemCount(hmenu);

	MENUITEMINFO mii;
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_FTYPE;

	for (int i = 0; i < ItemCount; i++) {
		if (::GetMenuItemInfo(hmenu, i, TRUE, &mii)
				&& (mii.fType & (MFT_OWNERDRAW | MFT_MENUBREAK)) != 0) {
			return true;
		}
	}

	return false;
}

LRESULT CALLBACK CMainWindow::MenuSubclassProc(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	CMainWindow *pThis = reinterpret_cast<CMainWindow*>(dwRefData);

	switch (uMsg) {
	case WM_PRINT:
		// メニューの描画は WM_PAINT / WM_NCPAINT ではなく WM_PRINT で行われる模様。
		{
			const HMENU hmenu = reinterpret_cast<HMENU>(::SendMessage(hwnd, MN_GETHMENU, 0, 0));
			if (hmenu == nullptr || !IsMenuNeedCustomErase(hmenu))
				break;

			::DefSubclassProc(hwnd, uMsg, wParam, lParam);

			const HDC hdc = reinterpret_cast<HDC>(wParam);

			RECT rcClient, rcWindow;
			::GetClientRect(hwnd, &rcClient);
			MapWindowRect(hwnd, nullptr, &rcClient);
			::GetWindowRect(hwnd, &rcWindow);
			::OffsetRect(&rcClient, -rcWindow.left, -rcWindow.top);
			::OffsetRect(&rcWindow, -rcWindow.left, -rcWindow.top);
			::ExcludeClipRect(hdc, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);

			auto it = pThis->m_MenuPainter.find(hwnd);
			if (it != pThis->m_MenuPainter.end() && it->second.IsThemed()) {
				it->second.DrawBorder(hdc, rcWindow);
			} else {
				const HPEN hpen = ::CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
				const HBRUSH hbr = ::CreateSolidBrush(RGB(43, 43, 43));
				const HGDIOBJ hOldPen = ::SelectObject(hdc, hpen);
				const HGDIOBJ hOldBrush = ::SelectObject(hdc, hbr);

				::Rectangle(hdc, rcWindow.left, rcWindow.top, rcWindow.right, rcWindow.bottom);

				::SelectObject(hdc, hOldPen);
				::SelectObject(hdc, hOldBrush);
				::DeleteObject(hpen);
				::DeleteObject(hbr);
			}
		}
		return 0;

	case WM_ERASEBKGND:
		// セパレータの背景や、複数列のメニューの余白部分は WM_ERASEBKGND で塗り潰される。
		{
			const HMENU hmenu = reinterpret_cast<HMENU>(::SendMessage(hwnd, MN_GETHMENU, 0, 0));
			if (hmenu == nullptr || !IsMenuNeedCustomErase(hmenu))
				break;

			const HDC hdc = reinterpret_cast<HDC>(wParam);
			RECT rc;
			::GetClientRect(hwnd, &rc);

			auto it = pThis->m_MenuPainter.find(hwnd);
			if (it != pThis->m_MenuPainter.end() && it->second.IsThemed()) {
				it->second.DrawBackground(hdc, rc);
			} else {
				DrawUtil::Fill(hdc, &rc, RGB(43, 43, 43));
			}
		}
		return 1;

	case WM_DESTROY:
		pThis->m_MenuPainter.erase(hwnd);
		::RemoveWindowSubclass(hwnd, MenuSubclassProc, 1);
		break;
	}

	return ::DefSubclassProc(hwnd, uMsg, wParam, lParam);
}




bool CMainWindow::CFullscreen::Initialize(HINSTANCE hinst)
{
	WNDCLASS wc;

	wc.style = CS_DBLCLKS;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hinst;
	wc.hIcon = nullptr;
	wc.hCursor = nullptr;
	wc.hbrBackground = ::CreateSolidBrush(RGB(0, 0, 0));
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = FULLSCREEN_WINDOW_CLASS;
	return ::RegisterClass(&wc) != 0;
}


CMainWindow::CFullscreen::CFullscreen(CMainWindow &MainWindow)
	: m_MainWindow(MainWindow)
	, m_App(MainWindow.m_App)
	, m_TitleBarManager(&MainWindow, false)
{
	RegisterUIChild(&m_LayoutBase);
	RegisterUIChild(&m_TitleBar);
	RegisterUIChild(&m_ViewWindow);
	RegisterUIChild(&m_Panel);

	SetStyleScaling(&m_StyleScaling);
}


CMainWindow::CFullscreen::~CFullscreen()
{
	Destroy();
}


LRESULT CMainWindow::CFullscreen::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		return OnCreate() ? 0 : -1;

	case WM_NCCREATE:
		InitStyleScaling(hwnd, true);
		break;

	case WM_SIZE:
		m_LayoutBase.SetPosition(0, 0, LOWORD(lParam), HIWORD(lParam));
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
		if (wParam == TIMER_ID_HIDECURSOR) {
			if (!m_fMenu && !m_pDisplay->GetDisplayBase().IsVisible()) {
				POINT pt;
				RECT rc;
				::GetCursorPos(&pt);
				m_ViewWindow.GetScreenPosition(&rc);
				if (::PtInRect(&rc, pt))
					ShowCursor(false);
			}
			::KillTimer(hwnd, TIMER_ID_HIDECURSOR);
		}
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT && HIWORD(lParam) != 0) {
			const HWND hwndCursor = reinterpret_cast<HWND>(wParam);

			if (hwndCursor == hwnd
					|| hwndCursor == m_pDisplay->GetVideoContainer().GetHandle()
					|| hwndCursor == m_ViewWindow.GetHandle()
					|| CPseudoOSD::IsPseudoOSD(hwndCursor)) {
				::SetCursor(m_fShowCursor ?::LoadCursor(nullptr, IDC_ARROW) : nullptr);
				return TRUE;
			}
		}
		break;

	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		{
			const bool fHorz = uMsg == WM_MOUSEHWHEEL;

			m_MainWindow.OnMouseWheel(wParam, lParam, fHorz);
			return fHorz;
		}

#if 0
	case WM_WINDOWPOSCHANGING:
		{
			WINDOWPOS *pwp = reinterpret_cast<WINDOWPOS*>(lParam);

			pwp->hwndInsertAfter = HWND_TOPMOST;
		}
		return 0;
#endif

	case WM_SYSKEYDOWN:
		if (wParam != VK_F10)
			break;
		[[fallthrough]];
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) {
			m_App.UICore.SetFullscreen(false);
			return 0;
		}
		[[fallthrough]];
	case WM_COMMAND:
		return m_MainWindow.SendMessage(uMsg, wParam, lParam);

	case WM_SYSCOMMAND:
		switch (wParam & 0xFFFFFFF0) {
		case SC_MONITORPOWER:
			if (m_App.GeneralOptions.GetNoMonitorLowPower()
					&& m_App.UICore.IsViewerEnabled())
				return 0;
			break;

		case SC_SCREENSAVE:
			if (m_App.GeneralOptions.GetNoScreenSaver()
					&& m_App.UICore.IsViewerEnabled())
				return 0;
			break;
		}
		break;

	case WM_APPCOMMAND:
		{
			const int Command = m_App.Accelerator.TranslateAppCommand(wParam, lParam);

			if (Command != 0) {
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
		if (wParam == ICON_SMALL)
			m_TitleBar.SetIcon(reinterpret_cast<HICON>(lParam));
		break;

	case WM_SHOWWINDOW:
		if (wParam != 0) {
			if (m_fShowEventInfoOSD) {
				::PostMessage(hwnd, MESSAGE_SHOWEVENTINFOOSD, 0, 0);
				m_fShowEventInfoOSD = false;
			}
		}
		break;

	case WM_CLOSE:
		m_fShowEventInfoOSD = m_App.OSDManager.IsEventInfoOSDVisible();
		break;

	case WM_DESTROY:
		m_pDisplay->GetVideoContainer().SetParent(&m_pDisplay->GetViewWindow());
		m_pDisplay->GetViewWindow().SendSizeMessage();
		delete m_LayoutBase.GetTopContainer();
		m_LayoutBase.SetTopContainer(nullptr);
		ShowCursor(true);
		m_pDisplay->GetDisplayBase().AdjustPosition();

		m_App.OSDManager.Reset();
		if (m_fShowEventInfoOSD)
			m_App.UICore.ShowEventInfoOSD();

		m_TitleBar.Destroy();
		ShowStatusBar(false);
		ShowSideBar(false);
		ShowPanel(false);
		RestorePanel(false);
		return 0;

	case MESSAGE_SHOWEVENTINFOOSD:
		m_App.UICore.ShowEventInfoOSD();
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


bool CMainWindow::CFullscreen::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	return CreateBasicWindow(
		hwndParent, Style, ExStyle, ID,
		FULLSCREEN_WINDOW_CLASS, nullptr, m_App.GetInstance());
}


bool CMainWindow::CFullscreen::Create(HWND hwndOwner, CMainDisplay *pDisplay)
{
	m_ScreenMargin = m_MainWindow.m_Style.FullscreenMargin;

	int x, y, Width, Height;

	const HMONITOR hMonitor = ::MonitorFromWindow(m_MainWindow.GetHandle(), MONITOR_DEFAULTTONEAREST);
	if (hMonitor != nullptr) {
		MONITORINFOEX mi;

		mi.cbSize = sizeof(mi);
		::GetMonitorInfo(hMonitor, &mi);
		x = mi.rcMonitor.left;
		y = mi.rcMonitor.top;
		Width = mi.rcMonitor.right - mi.rcMonitor.left;
		Height = mi.rcMonitor.bottom - mi.rcMonitor.top;

		DISPLAY_DEVICE dd;
		dd.cb = sizeof(dd);
		int MonitorNo = 1;
		for (DWORD i = 0; ::EnumDisplayDevices(0, i, &dd, 0); i++) {
			if ((dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) == 0) {
				if (::lstrcmpi(dd.DeviceName, mi.szDevice) == 0) {
					const Style::CStyleManager *pStyleManager = GetStyleManager();
					TCHAR szKey[64];
					StringFormat(szKey, TEXT("fullscreen.monitor{}.margin"), MonitorNo);
					Style::Margins Margin;
					if (pStyleManager->Get(szKey, &Margin)) {
						m_pStyleScaling->ToPixels(&Margin);
						m_ScreenMargin = Margin;
					}
					break;
				}
				MonitorNo++;
			}
		}
	} else {
		x = y = 0;
		Width = ::GetSystemMetrics(SM_CXSCREEN);
		Height = ::GetSystemMetrics(SM_CYSCREEN);
	}
#ifdef _DEBUG
	// デバッグし易いように小さく表示
	if (::GetKeyState(VK_SHIFT) < 0) {
		Width /= 2;
		Height /= 2;
	}
#endif
	SetPosition(x, y, Width, Height);

	m_pDisplay = pDisplay;

	return Create(hwndOwner, WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN, WS_EX_TOPMOST);
}


bool CMainWindow::CFullscreen::OnCreate()
{
	m_LayoutBase.Create(m_hwnd, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

	m_ViewWindow.SetMargin(m_ScreenMargin);
	m_ViewWindow.Create(
		m_LayoutBase.GetHandle(),
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN  | WS_CLIPSIBLINGS, 0, IDC_VIEW);
	m_ViewWindow.SetMessageWindow(m_hwnd);
	m_pDisplay->GetVideoContainer().SetParent(m_ViewWindow.GetHandle());
	m_ViewWindow.SetVideoContainer(&m_pDisplay->GetVideoContainer());

	m_Panel.Create(
		m_LayoutBase.GetHandle(),
		WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	m_Panel.ShowTitle(true);
	m_Panel.EnableFloating(false);
	m_Panel.SetEventHandler(&m_PanelEventHandler);
	CPanel::PanelTheme PanelTheme;
	m_App.Panel.Frame.GetPanelTheme(&PanelTheme);
	m_Panel.SetPanelTheme(PanelTheme);

	Layout::CSplitter *pSplitter = new Layout::CSplitter(CONTAINER_ID_PANELSPLITTER);
	pSplitter->SetVisible(true);
	if (m_PanelPlace == CPanelFrame::DockingPlace::None) {
		if (!m_App.Panel.Frame.IsDockingVertical()) {
			m_PanelPlace =
				m_MainWindow.GetPanelPaneIndex() == 0 ?
					CPanelFrame::DockingPlace::Left : CPanelFrame::DockingPlace::Right;
		} else {
			m_PanelPlace = CPanelFrame::DockingPlace::Right;
		}
	}
	const int PanelPaneIndex =
		m_PanelPlace == CPanelFrame::DockingPlace::Left || m_PanelPlace == CPanelFrame::DockingPlace::Top ? 0 : 1;
	Layout::CWindowContainer *pViewContainer = new Layout::CWindowContainer(CONTAINER_ID_VIEW);
	pViewContainer->SetWindow(&m_ViewWindow);
	pViewContainer->SetMinSize(32, 32);
	pViewContainer->SetVisible(true);
	pSplitter->SetPane(1 - PanelPaneIndex, pViewContainer);
	Layout::CWindowContainer *pPanelContainer = new Layout::CWindowContainer(CONTAINER_ID_PANEL);
	pPanelContainer->SetWindow(&m_Panel);
	pPanelContainer->SetMinSize(32, 32);
	pSplitter->SetPane(PanelPaneIndex, pPanelContainer);
	pSplitter->SetAdjustPane(CONTAINER_ID_VIEW);
	m_LayoutBase.SetTopContainer(pSplitter);

	RECT rc;
	m_pDisplay->GetDisplayBase().GetParent()->GetClientRect(&rc);
	m_pDisplay->GetDisplayBase().SetPosition(&rc);

	if (m_App.ViewOptions.GetTitleBarFontEnabled())
		m_TitleBar.SetFont(m_App.ViewOptions.GetTitleBarFont());
	m_TitleBar.Create(
		m_ViewWindow.GetHandle(),
		WS_CHILD | WS_CLIPSIBLINGS, 0, IDC_TITLEBAR);
	m_TitleBar.SetEventHandler(&m_TitleBarManager);
	m_TitleBar.SetIcon(
		reinterpret_cast<HICON>(m_MainWindow.SendMessage(
			WM_GETICON, m_TitleBar.IsIconDrawSmall() ? ICON_SMALL : ICON_BIG, 0)));
	m_TitleBar.SetMaximizeMode(m_MainWindow.GetMaximize());
	m_TitleBar.SetFullscreenMode(true);

	m_fShowEventInfoOSD = m_App.OSDManager.IsEventInfoOSDVisible();
	m_App.OSDManager.Reset();

	LibISDB::ViewerFilter *pViewer = m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();
	if (pViewer != nullptr)
		pViewer->SetViewStretchMode(m_App.VideoOptions.GetFullscreenStretchMode());

	m_fShowCursor = false;
	m_fMenu = false;
	m_fShowStatusView = false;
	m_fShowTitleBar = false;
	m_fShowSideBar = false;
	m_fShowPanel = false;

	m_CursorTracker.SetMoveDelta(4);
	m_CursorTracker.Reset();
	::SetTimer(m_hwnd, TIMER_ID_HIDECURSOR, HIDE_CURSOR_DELAY, nullptr);

	return true;
}


void CMainWindow::CFullscreen::ShowCursor(bool fShow)
{
	LibISDB::ViewerFilter *pViewer = m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();
	if (pViewer != nullptr)
		pViewer->HideCursor(!fShow);
	m_ViewWindow.SetShowCursor(fShow);
	m_fShowCursor = fShow;

	POINT pt;
	::GetCursorPos(&pt);
	for (HWND hwnd = ::WindowFromPoint(pt); hwnd != nullptr; hwnd = ::GetAncestor(hwnd, GA_PARENT)) {
		if (hwnd == m_ViewWindow.GetHandle()) {
			SendMessage(WM_SETCURSOR, reinterpret_cast<WPARAM>(hwnd), MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
			break;
		}
	}
}


void CMainWindow::CFullscreen::ShowPanel(bool fShow)
{
	if (m_fShowPanel != fShow) {
		Layout::CSplitter *pSplitter =
			dynamic_cast<Layout::CSplitter*>(m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));
		const bool fVertical =
			m_PanelPlace == CPanelFrame::DockingPlace::Top ||
			m_PanelPlace == CPanelFrame::DockingPlace::Bottom;

		if (fShow) {
			if (m_Panel.GetWindow() == nullptr) {
				if (m_PanelWidth < 0)
					m_PanelWidth = m_App.Panel.Frame.GetDockingWidth();
				if (m_PanelHeight < 0)
					m_PanelHeight = m_App.Panel.Frame.GetDockingHeight();
				m_App.Panel.Frame.SetPanelVisible(false);
				m_App.Panel.Frame.GetPanel()->SetWindow(nullptr, nullptr);
				m_Panel.SetWindow(&m_App.Panel.Form, TEXT("パネル"));
				pSplitter->SetStyle(
					fVertical ? Layout::CSplitter::StyleFlag::Vert : Layout::CSplitter::StyleFlag::Horz);
				pSplitter->SetPaneSize(
					CONTAINER_ID_PANEL,
					fVertical ? m_PanelHeight : m_PanelWidth);
			}
			m_Panel.SendSizeMessage();
			m_LayoutBase.SetContainerVisible(CONTAINER_ID_PANEL, true);
			m_App.Panel.UpdateContent();
		} else {
			if (fVertical)
				m_PanelHeight = m_Panel.GetHeight();
			else
				m_PanelWidth = m_Panel.GetWidth();
			m_LayoutBase.SetContainerVisible(CONTAINER_ID_PANEL, false);
			RestorePanel(true);
		}

		m_fShowPanel = fShow;

		m_App.UICore.SetCommandCheckedState(CM_PANEL, fShow);
	}
}


void CMainWindow::CFullscreen::RestorePanel(bool fPreventForeground)
{
	if (m_Panel.GetWindow() != nullptr) {
		m_Panel.SetWindow(nullptr, nullptr);
		CPanel *pPanel = m_App.Panel.Frame.GetPanel();
		pPanel->SetWindow(&m_App.Panel.Form, TEXT("パネル"));
		pPanel->SendSizeMessage();

		if (m_App.Panel.fShowPanelWindow) {
			// フローティングのパネルを再表示した際に、なぜか全画面ウィンドウの前に出るため
			// 見えない位置に移動した後でZオーダーを調整する
			RECT rc;
			if (fPreventForeground && m_App.Panel.IsFloating()) {
				m_App.Panel.Frame.GetPosition(&rc);
				const int Width = rc.right - rc.left, Height = rc.bottom - rc.top;
				m_App.Panel.Frame.SetPosition(
					::GetSystemMetrics(SM_XVIRTUALSCREEN) - Width - 64, 0, Width, Height);
			}

			m_App.Panel.Frame.SetPanelVisible(true, true);

			if (fPreventForeground && m_App.Panel.IsFloating()) {
				::SetWindowPos(
					m_App.Panel.Frame.GetHandle(), m_hwnd, 0, 0, 0, 0,
					SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
				::SetWindowPos(
					m_App.Panel.Frame.GetHandle(), HWND_NOTOPMOST,
					rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
					SWP_NOACTIVATE);
				if (m_MainWindow.m_pCore->GetAlwaysOnTop()) {
					::SetWindowPos(
						m_MainWindow.GetHandle(), HWND_TOPMOST, 0, 0, 0, 0,
						SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
				}
			}
		}
	}
}


bool CMainWindow::CFullscreen::SetPanelWidth(int Width)
{
	m_PanelWidth = Width;
	return true;
}


bool CMainWindow::CFullscreen::SetPanelHeight(int Height)
{
	m_PanelHeight = Height;
	return true;
}


bool CMainWindow::CFullscreen::SetPanelPlace(CPanelFrame::DockingPlace Place)
{
	if (m_PanelPlace == Place)
		return true;

	if (m_Panel.GetWindow() != nullptr) {
		Layout::CSplitter *pSplitter = dynamic_cast<Layout::CSplitter*>(
			m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));
		const bool fVerticalOld =
			m_PanelPlace == CPanelFrame::DockingPlace::Top ||
			m_PanelPlace == CPanelFrame::DockingPlace::Bottom;
		const bool fVerticalNew =
			Place == CPanelFrame::DockingPlace::Top ||
			Place == CPanelFrame::DockingPlace::Bottom;
		const int Index =
			Place == CPanelFrame::DockingPlace::Left ||
			Place == CPanelFrame::DockingPlace::Top ? 0 : 1;

		if (fVerticalOld)
			m_PanelHeight = pSplitter->GetPaneSize(CONTAINER_ID_PANEL);
		else
			m_PanelWidth = pSplitter->GetPaneSize(CONTAINER_ID_PANEL);
		m_LayoutBase.LockLayout();
		if (fVerticalNew != fVerticalOld) {
			pSplitter->SetStyle(
				fVerticalNew ? Layout::CSplitter::StyleFlag::Vert : Layout::CSplitter::StyleFlag::Horz);
		}
		pSplitter->SetPaneSize(
			CONTAINER_ID_PANEL, fVerticalNew ? m_PanelHeight : m_PanelWidth);
		if (pSplitter->IDToIndex(CONTAINER_ID_PANEL) != Index)
			pSplitter->SwapPane();
		m_LayoutBase.UnlockLayout();
	}

	m_PanelPlace = Place;

	return true;
}


void CMainWindow::CFullscreen::HideAllBars()
{
	ShowTitleBar(false);
	ShowSideBar(false);
	ShowStatusBar(false);
}


void CMainWindow::CFullscreen::SetTheme(const Theme::CThemeManager *pThemeManager)
{
	m_LayoutBase.SetBackColor(pThemeManager->GetColor(CColorScheme::COLOR_SPLITTER));
}


void CMainWindow::CFullscreen::OnMouseCommand(int Command)
{
	if (Command == 0)
		return;
	// メニュー表示中はカーソルを消さない
	::KillTimer(m_hwnd, TIMER_ID_HIDECURSOR);
	ShowCursor(true);
	m_fMenu = true;
	m_App.CommandManager.InvokeCommand(Command, CCommandManager::InvokeFlag::Mouse);
	m_fMenu = false;
	if (m_hwnd != nullptr) {
		m_CursorTracker.Reset();
		::SetTimer(m_hwnd, TIMER_ID_HIDECURSOR, HIDE_CURSOR_DELAY, nullptr);
	}
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
	::GetCursorPos(&pt);
	::ScreenToClient(m_hwnd, &pt);

	if (m_CursorTracker.GetLastCursorPos() == pt) {
		::SetTimer(m_hwnd, TIMER_ID_HIDECURSOR, HIDE_CURSOR_DELAY, nullptr);
		return;
	}

	bool fShowStatusView = false, fShowTitleBar = false, fShowSideBar = false;

	RECT rcClient;
	m_ViewWindow.GetClientRect(&rcClient);
	MapWindowRect(m_ViewWindow.GetHandle(), m_hwnd, &rcClient);

	RECT rcStatus = rcClient;
	rcStatus.top = rcStatus.bottom - m_App.StatusView.CalcHeight(rcClient.right - rcClient.left);
	if (::PtInRect(&rcStatus, pt))
		fShowStatusView = true;

	RECT rc = rcClient, rcTitle;
	m_TitleBarManager.Layout(&rc, &rcTitle);
	if (::PtInRect(&rcTitle, pt))
		fShowTitleBar = true;

	if (m_App.SideBarOptions.ShowPopup()) {
		RECT rcSideBar;
		switch (m_App.SideBarOptions.GetPlace()) {
		case CSideBarOptions::PlaceType::Left:
		case CSideBarOptions::PlaceType::Right:
			if (!fShowStatusView && !fShowTitleBar) {
				m_MainWindow.m_SideBarManager.Layout(&rcClient, &rcSideBar);
				if (::PtInRect(&rcSideBar, pt))
					fShowSideBar = true;
			}
			break;
		case CSideBarOptions::PlaceType::Top:
			rcClient.top = rcTitle.bottom;
			m_MainWindow.m_SideBarManager.Layout(&rcClient, &rcSideBar);
			if (::PtInRect(&rcSideBar, pt)) {
				fShowSideBar = true;
				fShowTitleBar = true;
			}
			break;
		case CSideBarOptions::PlaceType::Bottom:
			rcClient.bottom = rcStatus.top;
			m_MainWindow.m_SideBarManager.Layout(&rcClient, &rcSideBar);
			if (::PtInRect(&rcSideBar, pt)) {
				fShowSideBar = true;
				fShowStatusView = true;
			}
			break;
		}
	}

	ShowStatusBar(fShowStatusView);
	ShowTitleBar(fShowTitleBar);
	ShowSideBar(fShowSideBar);

	if (fShowStatusView || fShowTitleBar || fShowSideBar) {
		m_CursorTracker.Reset();
		::KillTimer(m_hwnd, TIMER_ID_HIDECURSOR);
		return;
	}

	if (m_CursorTracker.OnCursorMove(pt.x, pt.y)) {
		if (!m_fShowCursor)
			ShowCursor(true);
	}

	::SetTimer(m_hwnd, TIMER_ID_HIDECURSOR, HIDE_CURSOR_DELAY, nullptr);
}


void CMainWindow::CFullscreen::SetTitleFont(const Style::Font &Font)
{
	m_TitleBar.SetFont(Font);
}


void CMainWindow::CFullscreen::ShowStatusBar(bool fShow)
{
	if (fShow == m_fShowStatusView)
		return;

	Layout::CLayoutBase &LayoutBase = m_MainWindow.GetLayoutBase();

	if (fShow) {
		ShowSideBar(false);
		m_App.StatusView.SetVisible(false);
		LayoutBase.SetContainerVisible(CONTAINER_ID_STATUS, false);
		OnSharedBarVisibilityChange(m_App.StatusView, m_App.StatusView, true);
		m_MainWindow.ShowPopupStatusBar(true);
	} else {
		OnBarHide(m_App.StatusView);
		m_MainWindow.ShowPopupStatusBar(false);
		OnSharedBarVisibilityChange(m_App.StatusView, m_App.StatusView, false);
		if (m_MainWindow.GetStatusBarVisible())
			LayoutBase.SetContainerVisible(CONTAINER_ID_STATUS, true);
	}

	m_fShowStatusView = fShow;
}


void CMainWindow::CFullscreen::ShowTitleBar(bool fShow)
{
	if (fShow == m_fShowTitleBar)
		return;

	if (fShow) {
		RECT rc, rcBar;

		ShowSideBar(false);
		m_ViewWindow.GetClientRect(&rc);
		m_TitleBarManager.Layout(&rc, &rcBar);
		m_TitleBar.SetPosition(&rcBar);
		m_TitleBar.SetLabel(m_MainWindow.GetTitleBar().GetLabel());
		m_TitleBar.SetMaximizeMode(m_MainWindow.GetMaximize());
		CTitleBar::TitleBarTheme TitleBarTheme;
		m_MainWindow.GetTitleBar().GetTitleBarTheme(&TitleBarTheme);
		m_TitleBar.SetTitleBarTheme(TitleBarTheme);
		m_TitleBar.SetVisible(true);
		::BringWindowToTop(m_TitleBar.GetHandle());
	} else {
		OnBarHide(m_TitleBar);
		m_TitleBar.SetVisible(false);
	}

	m_fShowTitleBar = fShow;
}


void CMainWindow::CFullscreen::ShowSideBar(bool fShow)
{
	if (fShow == m_fShowSideBar)
		return;

	Layout::CLayoutBase &LayoutBase = m_MainWindow.GetLayoutBase();

	if (fShow) {
		RECT rcClient, rcBar;

		m_App.SideBar.SetVisible(false);
		LayoutBase.SetContainerVisible(CONTAINER_ID_SIDEBAR, false);
		OnSharedBarVisibilityChange(m_App.SideBar, m_App.SideBar, true);
		m_ViewWindow.GetClientRect(&rcClient);
		if (m_fShowStatusView)
			rcClient.bottom -= m_App.StatusView.GetHeight();
		if (m_fShowTitleBar)
			rcClient.top += m_TitleBar.GetHeight();
		m_MainWindow.m_SideBarManager.Layout(&rcClient, &rcBar);
		m_App.SideBar.SetPosition(&rcBar);
		m_MainWindow.ShowPopupSideBar(true);
	} else {
		OnBarHide(m_App.SideBar);
		m_MainWindow.ShowPopupSideBar(false);
		OnSharedBarVisibilityChange(m_App.SideBar, m_App.SideBar, false);
		if (m_MainWindow.GetSideBarVisible())
			LayoutBase.SetContainerVisible(CONTAINER_ID_SIDEBAR, true);
	}

	m_fShowSideBar = fShow;
}


void CMainWindow::CFullscreen::OnBarHide(CBasicWindow &Window)
{
	if (!GetVisible())
		return;

	POINT pt;
	RECT rc;

	::GetCursorPos(&pt);
	Window.GetScreenPosition(&rc);
	if (::PtInRect(&rc, pt)) {
		::ScreenToClient(m_hwnd, &pt);
		m_CursorTracker.OnCursorMove(pt.x, pt.y);
		ShowCursor(false);
	}
}


void CMainWindow::CFullscreen::OnSharedBarVisibilityChange(
	CBasicWindow &Window, CUIBase &UIBase, bool fVisible)
{
	if (fVisible) {
		Window.SetParent(&m_ViewWindow);
		m_MainWindow.RemoveUIChild(&UIBase);
		UIBase.SetStyleScaling(m_pStyleScaling);
	} else {
		Window.SetParent(&m_MainWindow.GetLayoutBase());
		m_MainWindow.RegisterUIChild(&UIBase);
		UIBase.SetStyleScaling(m_MainWindow.GetStyleScaling());
	}
	UIBase.UpdateStyle();
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
	PANEL_MENU_LEFT = CPanel::MENU_USER,
	PANEL_MENU_RIGHT,
	PANEL_MENU_TOP,
	PANEL_MENU_BOTTOM
};

bool CMainWindow::CFullscreen::CPanelEventHandler::OnMenuPopup(HMENU hmenu)
{
	::AppendMenu(hmenu, MF_SEPARATOR, 0, nullptr);
	::AppendMenu(hmenu, MF_STRING | MF_ENABLED, PANEL_MENU_LEFT, TEXT("左へ(&L)"));
	::AppendMenu(hmenu, MF_STRING | MF_ENABLED, PANEL_MENU_RIGHT, TEXT("右へ(&R)"));
	::AppendMenu(hmenu, MF_STRING | MF_ENABLED, PANEL_MENU_TOP, TEXT("上へ(&T)"));
	::AppendMenu(hmenu, MF_STRING | MF_ENABLED, PANEL_MENU_BOTTOM, TEXT("下へ(&B)"));
	::EnableMenuItem(
		hmenu,
		m_Fullscreen.m_PanelPlace == CPanelFrame::DockingPlace::Left ? PANEL_MENU_LEFT :
		m_Fullscreen.m_PanelPlace == CPanelFrame::DockingPlace::Right ? PANEL_MENU_RIGHT :
		m_Fullscreen.m_PanelPlace == CPanelFrame::DockingPlace::Top ? PANEL_MENU_TOP : PANEL_MENU_BOTTOM,
		MF_BYCOMMAND | MF_GRAYED);
	return true;
}


bool CMainWindow::CFullscreen::CPanelEventHandler::OnMenuSelected(int Command)
{
	switch (Command) {
	case PANEL_MENU_LEFT:
		m_Fullscreen.SetPanelPlace(CPanelFrame::DockingPlace::Left);
		return true;
	case PANEL_MENU_RIGHT:
		m_Fullscreen.SetPanelPlace(CPanelFrame::DockingPlace::Right);
		return true;
	case PANEL_MENU_TOP:
		m_Fullscreen.SetPanelPlace(CPanelFrame::DockingPlace::Top);
		return true;
	case PANEL_MENU_BOTTOM:
		m_Fullscreen.SetPanelPlace(CPanelFrame::DockingPlace::Bottom);
		return true;
	}

	return false;
}




CMainWindow::MainWindowStyle::MainWindowStyle()
{
	int SizingBorderX = 0, SizingBorderY;

	NONCLIENTMETRICS ncm;
	ncm.cbSize = sizeof(NONCLIENTMETRICS);
	if (::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0))
		SizingBorderX = SizingBorderY = ncm.iBorderWidth + ncm.iPaddedBorderWidth;

	if (SizingBorderX == 0) {
		SizingBorderX = ::GetSystemMetrics(SM_CXSIZEFRAME);
		SizingBorderY = ::GetSystemMetrics(SM_CYSIZEFRAME);
	}
	ResizingMargin = Style::Margins(SizingBorderX, SizingBorderY, SizingBorderX, SizingBorderY);
}

void CMainWindow::MainWindowStyle::SetStyle(const Style::CStyleManager *pStyleManager)
{
	*this = MainWindowStyle();
	pStyleManager->Get(TEXT("screen.margin"), &ScreenMargin);
	pStyleManager->Get(TEXT("fullscreen.margin"), &FullscreenMargin);
	pStyleManager->Get(TEXT("main-window.resizing-margin"), &ResizingMargin);
	pStyleManager->Get(TEXT("main-window.corner.style"), &WindowCornerStyle);
	pStyleManager->Get(TEXT("main-window.allow-dark-mode"), &fAllowDarkMode);
}

void CMainWindow::MainWindowStyle::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	pStyleScaling->ToPixels(&ScreenMargin);
	pStyleScaling->ToPixels(&FullscreenMargin);
	pStyleScaling->ToPixels(&ResizingMargin);
}




bool CMainWindow::CBarLayout::IsSpot(const RECT *pArea, const POINT *pPos)
{
	RECT rcArea = *pArea, rcBar;

	Layout(&rcArea, &rcBar);
	return ::PtInRect(&rcBar, *pPos) != FALSE;
}

void CMainWindow::CBarLayout::AdjustArea(RECT *pArea)
{
	RECT rcBar;
	Layout(pArea, &rcBar);
}

void CMainWindow::CBarLayout::ReserveArea(RECT *pArea, bool fNoMove)
{
	RECT rc = *pArea;

	AdjustArea(&rc);
	if (fNoMove) {
		pArea->right += (pArea->right - pArea->left) - (rc.right - rc.left);
		pArea->bottom += (pArea->bottom - pArea->top) - (rc.bottom - rc.top);
	} else {
		pArea->left -= rc.left - pArea->left;
		pArea->top -= rc.top - pArea->top;
		pArea->right += pArea->right - rc.right;
		pArea->bottom += pArea->bottom - rc.bottom;
	}
}


CMainWindow::CTitleBarManager::CTitleBarManager(CMainWindow *pMainWindow, bool fMainWindow)
	: m_pMainWindow(pMainWindow)
	, m_fMainWindow(fMainWindow)
{
}

bool CMainWindow::CTitleBarManager::OnClose()
{
	m_pMainWindow->PostCommand(CM_CLOSE);
	return true;
}

bool CMainWindow::CTitleBarManager::OnMinimize()
{
	m_pMainWindow->SendMessage(WM_SYSCOMMAND, SC_MINIMIZE, 0);
	return true;
}

bool CMainWindow::CTitleBarManager::OnMaximize()
{
	m_pMainWindow->SendMessage(
		WM_SYSCOMMAND,
		m_pMainWindow->GetMaximize() ? SC_RESTORE : SC_MAXIMIZE, 0);
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

void CMainWindow::CTitleBarManager::OnLabelLButtonDown(int x, int y)
{
	if (m_fMainWindow) {
		POINT pt = {x, y};

		::ClientToScreen(m_pTitleBar->GetHandle(), &pt);
		m_pMainWindow->SendMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(pt.x, pt.y));
		m_fFixed = true;
	}
}

void CMainWindow::CTitleBarManager::OnLabelLButtonDoubleClick(int x, int y)
{
	m_fFixed = false;
	if (m_fMainWindow)
		OnMaximize();
	else
		OnFullscreen();
}

void CMainWindow::CTitleBarManager::OnLabelRButtonUp(int x, int y)
{
	POINT pt = {x, y};

	::ClientToScreen(m_pTitleBar->GetHandle(), &pt);
	ShowSystemMenu(pt.x, pt.y);
}

void CMainWindow::CTitleBarManager::OnIconLButtonDown(int x, int y)
{
	RECT rc;

	m_pTitleBar->GetScreenPosition(&rc);
	ShowSystemMenu(rc.left, rc.bottom);
}

void CMainWindow::CTitleBarManager::OnIconLButtonDoubleClick(int x, int y)
{
	m_pMainWindow->PostCommand(CM_CLOSE);
}

void CMainWindow::CTitleBarManager::OnHeightChanged(int Height)
{
	m_pMainWindow->SetFixedPaneSize(CONTAINER_ID_TITLEBARSPLITTER, CONTAINER_ID_TITLEBAR, 0, Height);
}

void CMainWindow::CTitleBarManager::Layout(RECT *pArea, RECT *pBarRect)
{
	pBarRect->left = pArea->left;
	pBarRect->top = pArea->top;
	pBarRect->right = pArea->right;
	pBarRect->bottom = pArea->top + m_pTitleBar->GetHeight();
	pArea->top += m_pTitleBar->GetHeight();
}

void CMainWindow::CTitleBarManager::EndDrag()
{
	m_fFixed = false;
}

void CMainWindow::CTitleBarManager::ShowSystemMenu(int x, int y)
{
	m_fFixed = true;
	m_pMainWindow->SendMessage(0x0313, 0, MAKELPARAM(x, y));
	m_fFixed = false;

	RECT rc;
	POINT pt;
	m_pTitleBar->GetScreenPosition(&rc);
	::GetCursorPos(&pt);
	if (!::PtInRect(&rc, pt))
		OnMouseLeave();
}


CMainWindow::CSideBarManager::CSideBarManager(CMainWindow *pMainWindow)
	: m_pMainWindow(pMainWindow)
{
}

void CMainWindow::CSideBarManager::Layout(RECT *pArea, RECT *pBarRect)
{
	const int BarWidth = m_pSideBar->GetBarWidth();

	switch (m_pMainWindow->m_App.SideBarOptions.GetPlace()) {
	case CSideBarOptions::PlaceType::Left:
		pBarRect->left = pArea->left;
		pBarRect->top = pArea->top;
		pBarRect->right = pBarRect->left + BarWidth;
		pBarRect->bottom = pArea->bottom;
		pArea->left += BarWidth;
		break;

	case CSideBarOptions::PlaceType::Right:
		pBarRect->left = pArea->right - BarWidth;
		pBarRect->top = pArea->top;
		pBarRect->right = pArea->right;
		pBarRect->bottom = pArea->bottom;
		pArea->right -= BarWidth;
		break;

	case CSideBarOptions::PlaceType::Top:
		pBarRect->left = pArea->left;
		pBarRect->top = pArea->top;
		pBarRect->right = pArea->right;
		pBarRect->bottom = pArea->top + BarWidth;
		pArea->top += BarWidth;
		break;

	case CSideBarOptions::PlaceType::Bottom:
		pBarRect->left = pArea->left;
		pBarRect->top = pArea->bottom - BarWidth;
		pBarRect->right = pArea->right;
		pBarRect->bottom = pArea->bottom;
		pArea->bottom -= BarWidth;
		break;
	}
}

const CChannelInfo *CMainWindow::CSideBarManager::GetChannelInfoByCommand(int Command)
{
	const CChannelList *pList = m_pMainWindow->m_App.ChannelManager.GetCurrentChannelList();
	if (pList != nullptr) {
		const int No = Command - CM_CHANNELNO_FIRST;
		int Index;

		if (pList->HasRemoteControlKeyID()) {
			Index = pList->FindChannelNo(No + 1);
			if (Index < 0)
				return nullptr;
		} else {
			Index = No;
		}
		return pList->GetChannelInfo(Index);
	}
	return nullptr;
}

void CMainWindow::CSideBarManager::OnCommand(int Command)
{
	m_pMainWindow->SendCommand(Command);
}

void CMainWindow::CSideBarManager::OnRButtonUp(int x, int y)
{
	CPopupMenu Menu(m_pMainWindow->m_App.GetResourceInstance(), IDM_SIDEBAR);

	Menu.CheckItem(CM_SIDEBAR, m_pMainWindow->GetSideBarVisible());
	Menu.EnableItem(CM_SIDEBAR, !m_pMainWindow->m_pCore->GetFullscreen());
	Menu.CheckRadioItem(
		CM_SIDEBAR_PLACE_FIRST, CM_SIDEBAR_PLACE_LAST,
		CM_SIDEBAR_PLACE_FIRST + static_cast<int>(m_pMainWindow->m_App.SideBarOptions.GetPlace()));

	POINT pt = {x, y};
	::ClientToScreen(m_pSideBar->GetHandle(), &pt);

	m_fFixed = true;
	Menu.Show(m_pMainWindow->GetHandle(), &pt);
	m_fFixed = false;

	RECT rc;
	m_pSideBar->GetScreenPosition(&rc);
	::GetCursorPos(&pt);
	if (!::PtInRect(&rc, pt))
		OnMouseLeave();
}

void CMainWindow::CSideBarManager::OnMouseLeave()
{
	if (!m_fFixed)
		m_pMainWindow->OnBarMouseLeave(m_pSideBar->GetHandle());
}

bool CMainWindow::CSideBarManager::GetTooltipText(int Command, LPTSTR pszText, int MaxText)
{
	if (Command >= CM_CHANNELNO_FIRST && Command <= CM_CHANNELNO_LAST) {
		const CChannelInfo *pChInfo = GetChannelInfoByCommand(Command);
		if (pChInfo != nullptr) {
			StringFormat(
				pszText, MaxText, TEXT("{}: {}"),
				(Command - CM_CHANNELNO_FIRST) + 1, pChInfo->GetName());
			return true;
		}
	}
	return false;
}

bool CMainWindow::CSideBarManager::DrawIcon(const CSideBar::DrawIconInfo *pInfo)
{
	if (pInfo->Command >= CM_CHANNELNO_FIRST && pInfo->Command <= CM_CHANNELNO_LAST) {
		if (m_pMainWindow->m_App.SideBarOptions.GetShowChannelLogo()) {
			// アイコンに局ロゴを表示
			// TODO: 新しくロゴが取得された時に再描画する
			const CChannelInfo *pChannel = GetChannelInfoByCommand(pInfo->Command);
			if (pChannel != nullptr) {
				const HBITMAP hbmLogo = m_pMainWindow->m_App.LogoManager.GetAssociatedLogoBitmap(
					pChannel->GetNetworkID(), pChannel->GetServiceID(),
					CLogoManager::LOGOTYPE_SMALL);
				if (hbmLogo != nullptr) {
					const int Width = pInfo->IconRect.right - pInfo->IconRect.left;
					const int Height = pInfo->IconRect.bottom - pInfo->IconRect.top;
					const int IconHeight = Height * 10 / 16; // 本来の比率より縦長にしている(見栄えのため)
					const HBITMAP hbmOld = SelectBitmap(pInfo->hdcBuffer, hbmLogo);
					const int OldStretchMode = ::SetStretchBltMode(pInfo->hdc, STRETCH_HALFTONE);
					BITMAP bm;
					::GetObject(hbmLogo, sizeof(bm), &bm);
					::StretchBlt(
						pInfo->hdc,
						pInfo->IconRect.left, pInfo->IconRect.top + (Height - IconHeight) / 2, Width, IconHeight,
						pInfo->hdcBuffer, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
					::SetStretchBltMode(pInfo->hdc, OldStretchMode);
					::SelectObject(pInfo->hdcBuffer, hbmOld);
					return true;
				}
			}
		}
	} else if (pInfo->Command >= CM_PLUGIN_FIRST
			&& pInfo->Command <= CM_PLUGIN_LAST) {
		CPlugin *pPlugin = m_pMainWindow->m_App.PluginManager.GetPluginByCommand(pInfo->Command);

		if (pPlugin != nullptr && pPlugin->GetIcon().IsCreated()) {
			pPlugin->GetIcon().Draw(
				pInfo->hdc,
				pInfo->IconRect.left, pInfo->IconRect.top,
				pInfo->IconRect.right - pInfo->IconRect.left,
				pInfo->IconRect.bottom - pInfo->IconRect.top,
				0, 0, 0, 0,
				pInfo->Color, pInfo->Opacity);
			return true;
		}
	} else if (pInfo->Command >= CM_PLUGINCOMMAND_FIRST
			&& pInfo->Command <= CM_PLUGINCOMMAND_LAST) {
		String CommandID = m_pMainWindow->m_App.CommandManager.GetCommandIDText(pInfo->Command);
		LPCTSTR pszCommand;
		CPlugin *pPlugin = m_pMainWindow->m_App.PluginManager.GetPluginByPluginCommand(CommandID.c_str(), &pszCommand);

		if (pPlugin != nullptr) {
			CPlugin::CPluginCommandInfo *pCommandInfo =
				pPlugin->GetPluginCommandInfo(pszCommand);

			if (pCommandInfo != nullptr) {
				if ((pCommandInfo->GetFlags() & PLUGIN_COMMAND_FLAG_NOTIFYDRAWICON) != 0) {
					DrawCommandIconInfo Info;

					Info.ID = pCommandInfo->GetID();
					Info.Flags = COMMAND_ICON_FLAG_NONE;
					Info.State = COMMAND_ICON_STATE_NONE;
					if (!!(pInfo->State & CSideBar::ItemState::Disabled))
						Info.State |= COMMAND_ICON_STATE_DISABLED;
					if (!!(pInfo->State & CSideBar::ItemState::Checked))
						Info.State |= COMMAND_ICON_STATE_CHECKED;
					if (!!(pInfo->State & CSideBar::ItemState::Hot))
						Info.State |= COMMAND_ICON_STATE_HOT;
					if ((Info.State & COMMAND_ICON_STATE_HOT) != 0)
						Info.pszStyle = L"side-bar.item.hot";
					else if ((Info.State & COMMAND_ICON_STATE_CHECKED) != 0)
						Info.pszStyle = L"side-bar.item.checked";
					else
						Info.pszStyle = L"side-bar.item";
					Info.hdc = pInfo->hdc;
					Info.DrawRect = pInfo->IconRect;
					Info.Color = pInfo->Color;
					Info.Opacity = pInfo->Opacity;

					if (pPlugin->DrawPluginCommandIcon(&Info))
						return true;
				}

				pCommandInfo->GetIcon().Draw(
					pInfo->hdc,
					pInfo->IconRect.left, pInfo->IconRect.top,
					pInfo->IconRect.right - pInfo->IconRect.left,
					pInfo->IconRect.bottom - pInfo->IconRect.top,
					0, 0, 0, 0,
					pInfo->Color, pInfo->Opacity);
			}
		}

		return true;
	}

	return false;
}

void CMainWindow::CSideBarManager::OnBarWidthChanged(int BarWidth)
{
	int Width, Height;

	if (m_pMainWindow->m_App.SideBar.GetVertical()) {
		Width = BarWidth;
		Height = 0;
	} else {
		Width = 0;
		Height = BarWidth;
	}
	m_pMainWindow->SetFixedPaneSize(CONTAINER_ID_SIDEBARSPLITTER, CONTAINER_ID_SIDEBAR, Width, Height);
}

void CMainWindow::CSideBarManager::OnStyleChanged()
{
	m_pMainWindow->m_App.SideBarOptions.SetSideBarImage();
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
	m_pMainWindow->SetFixedPaneSize(CONTAINER_ID_STATUSSPLITTER, CONTAINER_ID_STATUS, 0, Height);
}

void CMainWindow::CStatusViewEventHandler::OnStyleChanged()
{
	m_pMainWindow->m_App.StatusOptions.ApplyItemWidth();
}


CMainWindow::CVideoContainerEventHandler::CVideoContainerEventHandler(CMainWindow *pMainWindow)
	: m_pMainWindow(pMainWindow)
{
}

void CMainWindow::CVideoContainerEventHandler::OnSizeChanged(int Width, int Height)
{
	CNotificationBar &NotificationBar = m_pMainWindow->m_NotificationBar;
	if (NotificationBar.GetVisible()) {
		RECT rc, rcView;

		NotificationBar.GetPosition(&rc);
		::GetClientRect(NotificationBar.GetParent(), &rcView);
		rc.left = rcView.left;
		rc.right = rcView.right;
		NotificationBar.SetPosition(&rc);
	}

	m_pMainWindow->m_App.OSDManager.HideVolumeOSD();
	m_pMainWindow->m_App.OSDManager.AdjustPosition();
}


CMainWindow::CViewWindowEventHandler::CViewWindowEventHandler(CMainWindow *pMainWindow)
	: m_pMainWindow(pMainWindow)
{
}

void CMainWindow::CViewWindowEventHandler::OnSizeChanged(int Width, int Height)
{
	// 一時的に表示されているバーのサイズを合わせる
	RECT rcView, rc;

	m_pView->GetPosition(&rcView);
	if (!m_pMainWindow->GetTitleBarVisible()
			&& m_pMainWindow->m_TitleBar.GetVisible()) {
		m_pMainWindow->m_TitleBarManager.Layout(&rcView, &rc);
		m_pMainWindow->m_TitleBar.SetPosition(&rc);
	}
	CStatusView &StatusView = m_pMainWindow->GetStatusView();
	if (!m_pMainWindow->GetStatusBarVisible()
			&& StatusView.GetVisible()
			&& StatusView.GetParent() == m_pView->GetParent()) {
		rc = rcView;
		rc.top = rc.bottom - StatusView.GetHeight();
		rcView.bottom -= StatusView.GetHeight();
		StatusView.SetPosition(&rc);
	}
	CSideBar &SideBar = m_pMainWindow->GetSideBar();
	if (!m_pMainWindow->GetSideBarVisible()
			&& SideBar.GetVisible()) {
		m_pMainWindow->m_SideBarManager.Layout(&rcView, &rc);
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


CMainWindow::CCursorTracker::CCursorTracker()
{
	Reset();
}

void CMainWindow::CCursorTracker::Reset()
{
	m_LastMovePos.x = LONG_MAX / 2;
	m_LastMovePos.y = LONG_MAX / 2;
	m_LastCursorPos = m_LastMovePos;
}

bool CMainWindow::CCursorTracker::OnCursorMove(int x, int y)
{
	m_LastCursorPos.x = x;
	m_LastCursorPos.y = y;

	if (std::abs(m_LastMovePos.x - x) >= m_MoveDelta || std::abs(m_LastMovePos.y - y) >= m_MoveDelta) {
		m_LastMovePos.x = x;
		m_LastMovePos.y = y;
		return true;
	}

	return false;
}


CMainWindow::CEpgCaptureEventHandler::CEpgCaptureEventHandler(CMainWindow *pMainWindow)
	: m_pMainWindow(pMainWindow)
{
}

void CMainWindow::CEpgCaptureEventHandler::OnBeginCapture(
	CEpgCaptureManager::BeginFlag Flags, CEpgCaptureManager::BeginStatus Status)
{
	if (!(Status & CEpgCaptureManager::BeginStatus::Standby)) {
		m_pMainWindow->StoreTunerResumeInfo();
		m_pMainWindow->m_Resume.fOpenTuner =
			!!(Status & CEpgCaptureManager::BeginStatus::TunerAlreadyOpened);
	}

	m_pMainWindow->SuspendViewer(ResumeInfo::ViewerSuspendFlag::EPGUpdate);

	m_pMainWindow->m_App.Epg.ProgramGuide.OnEpgCaptureBegin();
}

void CMainWindow::CEpgCaptureEventHandler::OnEndCapture(CEpgCaptureManager::EndFlag Flags)
{
	CAppMain &App = m_pMainWindow->m_App;
	HANDLE hThread;
	int OldPriority;

	m_pMainWindow->m_Timer.EndTimer(TIMER_ID_PROGRAMGUIDEUPDATE);

	if (m_pMainWindow->m_pCore->GetStandby()) {
		hThread = ::GetCurrentThread();
		OldPriority = ::GetThreadPriority(hThread);
		::SetThreadPriority(hThread, THREAD_PRIORITY_LOWEST);
	} else {
		::SetCursor(::LoadCursor(nullptr, IDC_WAIT));
	}
	App.EpgOptions.SaveEpgFile(&App.EPGDatabase);
	App.Epg.ProgramGuide.OnEpgCaptureEnd();
	if (m_pMainWindow->m_pCore->GetStandby()) {
		App.Epg.ProgramGuide.Refresh();
		::SetThreadPriority(hThread, OldPriority);
		if (!!(Flags & CEpgCaptureManager::EndFlag::CloseTuner))
			App.Core.CloseTuner();
	} else {
		::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
		if (!!(Flags & CEpgCaptureManager::EndFlag::Resume))
			m_pMainWindow->ResumeChannel();
		if (m_pMainWindow->IsPanelVisible()
				&& App.Panel.Form.GetCurPageID() == PANEL_ID_CHANNEL)
			App.Panel.ChannelPanel.UpdateAllChannels();
	}

	m_pMainWindow->ResumeViewer(ResumeInfo::ViewerSuspendFlag::EPGUpdate);
}

void CMainWindow::CEpgCaptureEventHandler::OnChannelChanged()
{
	CAppMain &App = m_pMainWindow->m_App;

	m_pMainWindow->m_Timer.BeginTimer(TIMER_ID_PROGRAMGUIDEUPDATE, 15000);

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


CMainWindow::CCommandEventListener::CCommandEventListener(CMainWindow *pMainWindow)
	: m_pMainWindow(pMainWindow)
{
}

void CMainWindow::CCommandEventListener::OnCommandStateChanged(
	int ID, CCommandManager::CommandState OldState, CCommandManager::CommandState NewState)
{
	if (!!((OldState ^ NewState) & CCommandManager::CommandState::Checked)) {
		const bool fChecked = !!(NewState & CCommandManager::CommandState::Checked);
		m_pMainWindow->m_App.MainMenu.CheckItem(ID, fChecked);
		m_pMainWindow->m_App.SideBar.CheckItem(ID, fChecked);
	}

	if (!!((OldState ^ NewState) & CCommandManager::CommandState::Disabled)) {
		const bool fEnabled = !(NewState & CCommandManager::CommandState::Disabled);
		m_pMainWindow->m_App.MainMenu.EnableItem(ID, fEnabled);
		m_pMainWindow->m_App.SideBar.EnableItem(ID, fEnabled);
	}
}

void CMainWindow::CCommandEventListener::OnCommandRadioCheckedStateChanged(
	int FirstID, int LastID, int CheckedID)
{
	if (FirstID >= CM_ASPECTRATIO_FIRST && FirstID <= CM_ASPECTRATIO_LAST)
		m_pMainWindow->m_App.AspectRatioIconMenu.CheckRadioItem(FirstID, LastID, CheckedID);
	else
		m_pMainWindow->m_App.MainMenu.CheckRadioItem(FirstID, LastID, CheckedID);
	m_pMainWindow->m_App.SideBar.CheckRadioItem(FirstID, LastID, CheckedID);
}


CMainWindow::CClockUpdateTimer::CClockUpdateTimer(CMainWindow *pMainWindow)
	: m_pMainWindow(pMainWindow)
{
}

void CMainWindow::CClockUpdateTimer::OnTimer()
{
	CClockStatusItem *pClockStatusItem =
		dynamic_cast<CClockStatusItem*>(m_pMainWindow->m_App.StatusView.GetItemByID(STATUS_ITEM_CLOCK));
	if (pClockStatusItem != nullptr && pClockStatusItem->UpdateContent())
		m_pMainWindow->PostMessage(WM_APP_UPDATECLOCK, 0, 0);
}


} // namespace TVTest
