#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


using namespace TVTest;


static CAppMain g_App;


const COptionDialog::PageInfo COptionDialog::m_PageList[] = {
	{TEXT("一般"),					&g_App.GeneralOptions,			HELP_ID_OPTIONS_GENERAL},
	{TEXT("表示"),					&g_App.ViewOptions,				HELP_ID_OPTIONS_VIEW},
	{TEXT("OSD"),					&g_App.OSDOptions,				HELP_ID_OPTIONS_OSD},
	{TEXT("ステータスバー"),		&g_App.StatusOptions,			HELP_ID_OPTIONS_STATUSBAR},
	{TEXT("サイドバー"),			&g_App.SideBarOptions,			HELP_ID_OPTIONS_SIDEBAR},
	{TEXT("メニュー"),				&g_App.MenuOptions,				HELP_ID_OPTIONS_MENU},
	{TEXT("パネル"),				&g_App.PanelOptions,			HELP_ID_OPTIONS_PANEL},
	{TEXT("テーマ/配色"),			&g_App.ColorSchemeOptions,		HELP_ID_OPTIONS_COLORSCHEME},
	{TEXT("操作"),					&g_App.OperationOptions,		HELP_ID_OPTIONS_OPERATION},
	{TEXT("キー割り当て"),			&g_App.Accelerator,				HELP_ID_OPTIONS_ACCELERATOR},
	{TEXT("リモコン"),				&g_App.ControllerManager,		HELP_ID_OPTIONS_CONTROLLER},
	{TEXT("BonDriver設定"),			&g_App.DriverOptions,			HELP_ID_OPTIONS_DRIVER},
	{TEXT("再生"),					&g_App.PlaybackOptions,			HELP_ID_OPTIONS_PLAYBACK},
	{TEXT("録画"),					&g_App.RecordOptions,			HELP_ID_OPTIONS_RECORD},
	{TEXT("キャプチャ"),			&g_App.CaptureOptions,			HELP_ID_OPTIONS_CAPTURE},
	{TEXT("チャンネルスキャン"),	&g_App.ChannelScan,				HELP_ID_OPTIONS_CHANNELSCAN},
	{TEXT("EPG/番組情報"),			&g_App.EpgOptions,				HELP_ID_OPTIONS_EPG},
	{TEXT("EPG番組表"),				&g_App.ProgramGuideOptions,		HELP_ID_OPTIONS_PROGRAMGUIDE},
	{TEXT("プラグイン"),			&g_App.PluginOptions,			HELP_ID_OPTIONS_PLUGIN},
#ifdef NETWORK_REMOCON_SUPPORT
	{TEXT("ネットワークリモコン"),	&g_App.NetworkRemoconOptions,	HELP_ID_OPTIONS_NETWORKREMOCON},
#endif
	{TEXT("ログ"),					&g_App.Logger,					HELP_ID_OPTIONS_LOG},
};




CAppMain &GetAppClass()
{
	return g_App;
}




bool CTotTimeAdjuster::BeginAdjust(DWORD TimeOut)
{
	m_TimeOut=TimeOut;
	m_StartTime=::GetTickCount();
	m_PrevTime.wYear=0;
	m_fEnable=true;
	return true;
}


bool CTotTimeAdjuster::AdjustTime()
{
	if (!m_fEnable)
		return false;
	if (TickTimeSpan(m_StartTime,::GetTickCount())>=m_TimeOut) {
		m_fEnable=false;
		return false;
	}

	SYSTEMTIME st;
	if (!g_App.CoreEngine.m_DtvEngine.m_TsAnalyzer.GetTotTime(&st))
		return false;
	if (m_PrevTime.wYear==0) {
		m_PrevTime=st;
		return false;
	} else if (CompareSystemTime(&m_PrevTime,&st)==0) {
		return false;
	}

	bool fOK=false;
	HANDLE hToken;
	if (::OpenProcessToken(::GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&hToken)) {
		LUID luid;
		if (::LookupPrivilegeValue(NULL,SE_SYSTEMTIME_NAME,&luid)) {
			TOKEN_PRIVILEGES tkp;
			tkp.PrivilegeCount=1;
			tkp.Privileges[0].Luid=luid;
			tkp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
			if (::AdjustTokenPrivileges(hToken,FALSE, &tkp,sizeof(TOKEN_PRIVILEGES),NULL,0)
					&& ::GetLastError()==ERROR_SUCCESS) {
				// バッファがあるので少し時刻を戻す
				OffsetSystemTime(&st,-2000);
				if (::SetLocalTime(&st)) {
					g_App.Logger.AddLog(TEXT("TOTで時刻合わせを行いました。(%d/%d/%d %d:%02d:%02d)"),
										st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
					fOK=true;
				}
			}
		}
		::CloseHandle(hToken);
	}
	m_fEnable=false;
	return fOK;
}




bool CChannelMenuManager::InitPopup(HMENU hmenuParent,HMENU hmenu)
{
	bool fChannelMenu=false;
	int Count=::GetMenuItemCount(hmenuParent);
	int i;
	for (i=0;i<Count;i++) {
		if (::GetSubMenu(hmenuParent,i)==hmenu) {
			fChannelMenu=true;
			break;
		}
		if ((::GetMenuState(hmenuParent,i,MF_BYPOSITION)&MF_POPUP)==0)
			break;
	}

	if (fChannelMenu) {
		const CChannelList *pChannelList;
		int Command=CM_SPACE_CHANNEL_FIRST;

		pChannelList=g_App.ChannelManager.GetAllChannelList();
		if (g_App.ChannelManager.NumSpaces()>1) {
			if (i==0) {
				CreateChannelMenu(
					pChannelList,
					g_App.ChannelManager.GetCurrentSpace()==CChannelManager::SPACE_ALL?
						g_App.ChannelManager.GetCurrentChannel():-1,
					Command,hmenu,g_App.UICore.GetMainWindow(),
					CChannelMenu::FLAG_SPACEBREAK);
				return true;
			}
			i--;
		}
		if (i>=g_App.ChannelManager.NumSpaces()) {
			TRACE(TEXT("CChannelMenuManager::InitPopup() : Invalid space %d\n"),i);
			ClearMenu(hmenu);
			return true;
		}
		Command+=pChannelList->NumChannels();
		for (int j=0;j<i;j++) {
			pChannelList=g_App.ChannelManager.GetChannelList(j);
			Command+=pChannelList->NumChannels();
		}
		CreateChannelMenu(
			g_App.ChannelManager.GetChannelList(i),
			g_App.ChannelManager.GetCurrentSpace()==i?
				g_App.ChannelManager.GetCurrentChannel():-1,
			Command,hmenu,g_App.UICore.GetMainWindow());
		return true;
	}

	return false;
}


bool CChannelMenuManager::CreateChannelMenu(
	const CChannelList *pChannelList,int CurChannel,
	UINT Command,HMENU hmenu,HWND hwnd,unsigned int Flags)
{
	if (pChannelList==NULL)
		return false;
	const bool fEventInfo=
		(Flags&CChannelMenu::FLAG_SHOWEVENTINFO)!=0
		|| pChannelList->NumEnableChannels()<=g_App.MenuOptions.GetMaxChannelMenuEventInfo();
	unsigned int MenuFlags=CChannelMenu::FLAG_SHOWLOGO | Flags;
	if (fEventInfo)
		MenuFlags|=CChannelMenu::FLAG_SHOWEVENTINFO;
	else
		MenuFlags|=CChannelMenu::FLAG_SHOWTOOLTIP;
	return g_App.ChannelMenu.Create(pChannelList,CurChannel,
									Command,hmenu,hwnd,MenuFlags,
									fEventInfo?0:g_App.MenuOptions.GetMaxChannelMenuRows());
}




CTunerSelectMenu::CTunerSelectMenu()
{
}


CTunerSelectMenu::~CTunerSelectMenu()
{
	Destroy();
}


bool CTunerSelectMenu::Create(HWND hwnd)
{
	Destroy();

	m_Menu.Create();
	m_hwnd=hwnd;

	HMENU hmenuSpace;
	const CChannelList *pChannelList;
	int Command;
	int i,j;
	LPCTSTR pszName;
	TCHAR szText[MAX_PATH*2];
	int Length;

	Command=CM_SPACE_CHANNEL_FIRST;
	pChannelList=g_App.ChannelManager.GetAllChannelList();
	if (g_App.ChannelManager.NumSpaces()>1) {
		hmenuSpace=::CreatePopupMenu();
		m_Menu.Append(hmenuSpace,TEXT("&A: すべて"));
	}
	Command+=pChannelList->NumChannels();
	for (i=0;i<g_App.ChannelManager.NumSpaces();i++) {
		pChannelList=g_App.ChannelManager.GetChannelList(i);
		hmenuSpace=::CreatePopupMenu();
		Length=StdUtil::snprintf(szText,lengthof(szText),TEXT("&%d: "),i);
		pszName=g_App.ChannelManager.GetTuningSpaceName(i);
		if (!IsStringEmpty(pszName))
			CopyToMenuText(pszName,szText+Length,lengthof(szText)-Length);
		else
			StdUtil::snprintf(szText+Length,lengthof(szText)-Length,TEXT("チューニング空間%d"),i);
		m_Menu.Append(hmenuSpace,szText,
					  pChannelList->NumEnableChannels()>0?MF_ENABLED:MF_GRAYED);
		Command+=pChannelList->NumChannels();
	}

	if (Command>CM_SPACE_CHANNEL_FIRST)
		m_Menu.AppendSeparator();

	for (i=0;i<g_App.DriverManager.NumDrivers();i++) {
		CDriverInfo *pDriverInfo=g_App.DriverManager.GetDriverInfo(i);

		if (IsEqualFileName(pDriverInfo->GetFileName(),g_App.CoreEngine.GetDriverFileName())) {
			continue;
		}
		TCHAR szFileName[MAX_PATH];
		::lstrcpyn(szFileName,pDriverInfo->GetFileName(),lengthof(szFileName));
		::PathRemoveExtension(szFileName);

		const CTuningSpaceList *pTuningSpaceList;
		if (pDriverInfo->LoadTuningSpaceList(CDriverInfo::LOADTUNINGSPACE_NOLOADDRIVER)
				&& (pTuningSpaceList=pDriverInfo->GetAvailableTuningSpaceList())!=NULL) {
			HMENU hmenuDriver=::CreatePopupMenu();

			for (j=0;j<pTuningSpaceList->NumSpaces();j++) {
				pChannelList=pTuningSpaceList->GetChannelList(j);
				if (pChannelList->NumEnableChannels()==0) {
					Command+=pChannelList->NumChannels();
					continue;
				}
				if (pTuningSpaceList->NumSpaces()>1)
					hmenuSpace=::CreatePopupMenu();
				else
					hmenuSpace=hmenuDriver;
				m_PopupList.push_back(PopupInfo(pChannelList,Command));
				MENUINFO mi;
				mi.cbSize=sizeof(mi);
				mi.fMask=MIM_MENUDATA;
				mi.dwMenuData=m_PopupList.size()-1;
				::SetMenuInfo(hmenuSpace,&mi);
				Command+=pChannelList->NumChannels();
				if (hmenuSpace!=hmenuDriver) {
					pszName=pTuningSpaceList->GetTuningSpaceName(j);
					Length=StdUtil::snprintf(szText,lengthof(szText),TEXT("&%d: "),j);
					if (!IsStringEmpty(pszName))
						CopyToMenuText(pszName,szText+Length,lengthof(szText)-Length);
					else
						StdUtil::snprintf(szText+Length,lengthof(szText)-Length,
										  TEXT("チューニング空間%d"),j);
					::AppendMenu(hmenuDriver,MF_POPUP | MF_ENABLED,
								 reinterpret_cast<UINT_PTR>(hmenuSpace),szText);
				}
			}
			if (!IsStringEmpty(pDriverInfo->GetTunerName())) {
				TCHAR szTemp[lengthof(szText)];

				StdUtil::snprintf(szTemp,lengthof(szTemp),TEXT("%s [%s]"),
								  pDriverInfo->GetTunerName(),
								  szFileName);
				CopyToMenuText(szTemp,szText,lengthof(szText));
			} else {
				CopyToMenuText(szFileName,szText,lengthof(szText));
			}
			m_Menu.Append(hmenuDriver,szText);
		} else {
			m_Menu.AppendUnformatted(CM_DRIVER_FIRST+i,szFileName);
		}
	}
	return true;
}


void CTunerSelectMenu::Destroy()
{
	m_Menu.Destroy();
	m_hwnd=NULL;
	m_PopupList.clear();
}


bool CTunerSelectMenu::Show(UINT Flags,int x,int y)
{
	POINT pt={x,y};
	return m_Menu.Show(m_hwnd,&pt,Flags);
}


bool CTunerSelectMenu::OnInitMenuPopup(HMENU hmenu)
{
	if (!m_Menu.IsCreated())
		return false;

	if (g_App.ChannelMenuManager.InitPopup(m_Menu.GetPopupHandle(),hmenu))
		return true;

	bool fChannelMenu=false;
	int Count=m_Menu.GetItemCount();
	int i,j;
	i=g_App.ChannelManager.NumSpaces();
	if (i>1)
		i++;
	for (i++;i<Count;i++) {
		HMENU hmenuChannel=m_Menu.GetSubMenu(i);
		int Items=::GetMenuItemCount(hmenuChannel);

		if (hmenuChannel==hmenu) {
			if (Items>0)
				return true;
			fChannelMenu=true;
			break;
		}
		if (Items>0) {
			for (j=0;j<Items;j++) {
				if (::GetSubMenu(hmenuChannel,j)==hmenu)
					break;
			}
			if (j<Items) {
				fChannelMenu=true;
				break;
			}
		}
	}

	if (fChannelMenu) {
		MENUINFO mi;

		mi.cbSize=sizeof(mi);
		mi.fMask=MIM_MENUDATA;
		if (!::GetMenuInfo(hmenu,&mi) || mi.dwMenuData>=m_PopupList.size())
			return false;
		const PopupInfo &Info=m_PopupList[mi.dwMenuData];
		g_App.ChannelMenuManager.CreateChannelMenu(Info.pChannelList,-1,Info.Command,hmenu,m_hwnd);
		return true;
	}

	return false;
}




CMainPanel::CMainPanel()
	: m_FrameEventHandler(&Frame)
	, m_FormEventHandler(&Form)
	, fShowPanelWindow(false)
{
	Frame.SetFloating(false);
	Frame.SetEventHandler(&m_FrameEventHandler);
	Form.SetEventHandler(&m_FormEventHandler);
	ChannelPanel.SetEventHandler(&m_ChannelPanelEventHandler);
}


bool CMainPanel::IsFloating() const
{
	return Frame.GetFloating();
}


bool CMainPanel::OnOwnerMovingOrSizing(const RECT *pOldRect,const RECT *pNewRect)
{
	if (fShowPanelWindow && g_App.PanelOptions.GetAttachToMainWindow()
			&& IsFloating()) {
		RECT rc;
		int XOffset,YOffset;
		bool fAttached=false;

		Frame.GetPosition(&rc);
		XOffset=YOffset=0;
		if (rc.top<pOldRect->bottom && rc.bottom>pOldRect->top) {
			if (rc.right==pOldRect->left) {
				XOffset=pNewRect->left-rc.right;
				fAttached=true;
			} else if (rc.left==pOldRect->right) {
				XOffset=pNewRect->right-rc.left;
				fAttached=true;
			}
			if (fAttached)
				YOffset=pNewRect->top-pOldRect->top;
		}
		if (!fAttached && rc.left<pOldRect->right && rc.right>pOldRect->left) {
			if (rc.bottom==pOldRect->top) {
				YOffset=pNewRect->top-rc.bottom;
				fAttached=true;
			} else if (rc.top==pOldRect->bottom) {
				YOffset=pNewRect->bottom-rc.top;
				fAttached=true;
			}
			if (fAttached)
				XOffset=pNewRect->left-pOldRect->left;
		}
		if (XOffset!=0 || YOffset!=0) {
			::OffsetRect(&rc,XOffset,YOffset);
			Frame.SetPosition(&rc);
			Frame.MoveToMonitorInside();
		}
		return true;
	}
	return false;
}


bool CMainPanel::IsAttached()
{
	bool fAttached=false;

	if (fShowPanelWindow && g_App.PanelOptions.GetAttachToMainWindow()
			&& IsFloating()) {
		RECT rcPanel,rcMain;

		Frame.GetPosition(&rcPanel);
		g_App.MainWindow.GetPosition(&rcMain);
		if (rcPanel.top<rcMain.bottom && rcPanel.bottom>rcMain.top) {
			if (rcPanel.right==rcMain.left || rcPanel.left==rcMain.right)
				fAttached=true;
		}
		if (!fAttached && rcPanel.left<rcMain.right && rcPanel.right>rcMain.left) {
			if (rcPanel.bottom==rcMain.top || rcPanel.top==rcMain.bottom)
				fAttached=true;
		}
	}
	return fAttached;
}


void CMainPanel::SetTheme(const TVTest::Theme::CThemeManager *pThemeManager)
{
	Frame.SetTheme(pThemeManager);
	Form.SetTheme(pThemeManager);
	InfoPanel.SetTheme(pThemeManager);
	ProgramListPanel.SetTheme(pThemeManager);
	ChannelPanel.SetTheme(pThemeManager);
	ControlPanel.SetTheme(pThemeManager);
	CaptionPanel.SetTheme(pThemeManager);
}


CMainPanel::CFrameEventHandler::CFrameEventHandler(CPanelFrame *pFrame)
	: m_pFrame(pFrame)
	, m_SnapEdge(EDGE_NONE)
{
}


bool CMainPanel::CFrameEventHandler::OnClose()
{
	g_App.UICore.DoCommand(CM_PANEL);
	return false;
}


bool CMainPanel::CFrameEventHandler::OnMoving(RECT *pRect)
{
	if (!m_pFrame->GetFloating())
		return false;

	POINT pt;
	RECT rc;

	::GetCursorPos(&pt);
	pt.x=m_ptStartPos.x+(pt.x-m_ptDragStartCursorPos.x);
	pt.y=m_ptStartPos.y+(pt.y-m_ptDragStartCursorPos.y);
	::OffsetRect(pRect,pt.x-pRect->left,pt.y-pRect->top);
	if (g_App.PanelOptions.GetSnapAtMainWindow()) {
		// メインウィンドウにスナップさせる
		int SnapMargin=g_App.PanelOptions.GetSnapMargin();
		int XOffset,YOffset;
		bool fSnap;

		g_App.MainWindow.GetPosition(&rc);
		XOffset=YOffset=0;
		fSnap=false;
		if (pRect->top<rc.bottom && pRect->bottom>rc.top) {
			if (pRect->right>=rc.left-SnapMargin && pRect->right<=rc.left+SnapMargin) {
				XOffset=rc.left-pRect->right;
				fSnap=true;
			} else if (pRect->left>=rc.right-SnapMargin && pRect->left<=rc.right+SnapMargin) {
				XOffset=rc.right-pRect->left;
				fSnap=true;
			}
			if (fSnap) {
				if (pRect->top>=rc.top-SnapMargin && pRect->top<=rc.top+SnapMargin) {
					YOffset=rc.top-pRect->top;
				} else if (pRect->bottom>=rc.bottom-SnapMargin && pRect->bottom<=rc.bottom+SnapMargin) {
					YOffset=rc.bottom-pRect->bottom;
				}
			}
		}
		if (!fSnap && pRect->left<rc.right && pRect->right>rc.left) {
			if (pRect->bottom>=rc.top-SnapMargin && pRect->bottom<=rc.top+SnapMargin) {
				YOffset=rc.top-pRect->bottom;
				fSnap=true;
			} else if (pRect->top>=rc.bottom-SnapMargin && pRect->top<=rc.bottom+SnapMargin) {
				YOffset=rc.bottom-pRect->top;
				fSnap=true;
			}
			if (fSnap) {
				if (pRect->left>=rc.left-SnapMargin && pRect->left<=rc.left+SnapMargin) {
					XOffset=rc.left-pRect->left;
				} else if (pRect->right>=rc.right-SnapMargin && pRect->right<=rc.right+SnapMargin) {
					XOffset=rc.right-pRect->right;
				}
			}
		}
		::OffsetRect(pRect,XOffset,YOffset);
	}
	return true;
}


bool CMainPanel::CFrameEventHandler::OnEnterSizeMove()
{
	if (!m_pFrame->GetFloating())
		return false;

	::GetCursorPos(&m_ptDragStartCursorPos);
	int x,y;
	m_pFrame->GetPosition(&x,&y,NULL,NULL);
	m_ptStartPos.x=x;
	m_ptStartPos.y=y;
	return true;
}


bool CMainPanel::CFrameEventHandler::OnKeyDown(UINT KeyCode,UINT Flags)
{
	g_App.MainWindow.SendMessage(WM_KEYDOWN,KeyCode,Flags);
	return true;
}


bool CMainPanel::CFrameEventHandler::OnMouseWheel(WPARAM wParam,LPARAM lParam)
{
	SendMessage(g_App.MainWindow.GetVideoHostWindow(),WM_MOUSEWHEEL,wParam,lParam);
	return true;
}


void CMainPanel::CFrameEventHandler::OnVisibleChange(bool fVisible)
{
	if (!m_pFrame->GetFloating())
		return;
	if (!fVisible) {
		m_SnapEdge=EDGE_NONE;
		if (g_App.PanelOptions.GetAttachToMainWindow()) {
			RECT rcPanel,rcMain;

			m_pFrame->GetPosition(&rcPanel);
			g_App.MainWindow.GetPosition(&rcMain);
			if (rcPanel.top<rcMain.bottom && rcPanel.bottom>rcMain.top) {
				if (rcPanel.right==rcMain.left)
					m_SnapEdge=EDGE_LEFT;
				else if (rcPanel.left==rcMain.right)
					m_SnapEdge=EDGE_RIGHT;
				if (m_SnapEdge!=EDGE_NONE)
					m_AttachOffset=rcPanel.top-rcMain.top;
			}
			if (rcPanel.left<rcMain.right && rcPanel.right>rcMain.left) {
				if (rcPanel.bottom==rcMain.top)
					m_SnapEdge=EDGE_TOP;
				else if (rcPanel.top==rcMain.bottom)
					m_SnapEdge=EDGE_BOTTOM;
				if (m_SnapEdge!=EDGE_NONE)
					m_AttachOffset=rcPanel.left-rcMain.left;
			}
		}
	} else {
		if (m_SnapEdge!=EDGE_NONE) {
			RECT rcPanel,rcMain;
			int x,y;

			m_pFrame->GetPosition(&rcPanel);
			OffsetRect(&rcPanel,-rcPanel.left,-rcPanel.top);
			g_App.MainWindow.GetPosition(&rcMain);
			switch (m_SnapEdge) {
			case EDGE_LEFT:
				x=rcMain.left-rcPanel.right;
				y=rcMain.top+m_AttachOffset;
				break;
			case EDGE_RIGHT:
				x=rcMain.right;
				y=rcMain.top+m_AttachOffset;
				break;
			case EDGE_TOP:
				y=rcMain.top-rcPanel.bottom;
				x=rcMain.left+m_AttachOffset;
				break;
			case EDGE_BOTTOM:
				y=rcMain.bottom;
				x=rcMain.left+m_AttachOffset;
				break;
			}
			m_pFrame->SetPosition(x,y,rcPanel.right,rcPanel.bottom);
			m_pFrame->MoveToMonitorInside();
		}
	}
}


bool CMainPanel::CFrameEventHandler::OnFloatingChange(bool fFloating)
{
	Layout::CSplitter *pSplitter=
		dynamic_cast<Layout::CSplitter*>(g_App.MainWindow.GetLayoutBase().GetContainerByID(CONTAINER_ID_PANELSPLITTER));
	int Size;
	RECT rc;

	int PanelPaneIndex=pSplitter->IDToIndex(CONTAINER_ID_PANEL);
	if (fFloating)
		pSplitter->GetPane(PanelPaneIndex)->SetVisible(false);
	g_App.MainWindow.GetPosition(&rc);
	Size=m_pFrame->GetDockingWidth()+pSplitter->GetBarWidth();
	if (!fFloating || rc.right-rc.left>Size) {
		if (PanelPaneIndex==0) {
			if (fFloating)
				rc.left+=Size;
			else
				rc.left-=Size;
		} else {
			if (fFloating)
				rc.right-=Size;
			else
				rc.right+=Size;
		}
		g_App.MainWindow.SetPosition(&rc);
	}
	return true;
}


CMainPanel::CFormEventHandler::CFormEventHandler(CPanelForm *pForm)
	: m_pForm(pForm)
{
}


void CMainPanel::CFormEventHandler::OnSelChange()
{
	g_App.MainWindow.UpdatePanel();
}


void CMainPanel::CFormEventHandler::OnRButtonDown()
{
	g_App.UICore.PopupMenu();
}


void CMainPanel::CFormEventHandler::OnTabRButtonDown(int x,int y)
{
	CPopupMenu Menu;
	Menu.Create();

	int Cur=-1;
	int VisibleCount=0;
	for (int i=0;i<m_pForm->NumPages();i++) {
		CPanelForm::TabInfo TabInfo;

		m_pForm->GetTabInfo(i,&TabInfo);
		if (TabInfo.fVisible) {
			TCHAR szText[64];
			::LoadString(g_App.GetResourceInstance(),
						 CM_PANEL_FIRST+TabInfo.ID,szText,lengthof(szText));
			Menu.Append(CM_PANEL_FIRST+TabInfo.ID,szText);
			if (TabInfo.ID==m_pForm->GetCurPageID())
				Cur=VisibleCount;
			VisibleCount++;
		}
	}
	if (VisibleCount>0) {
		if (Cur>=0)
			Menu.CheckRadioItem(0,VisibleCount-1,Cur,MF_BYPOSITION);
		POINT pt={x,y};
		::ClientToScreen(m_pForm->GetHandle(),&pt);
		Menu.Show(g_App.UICore.GetMainWindow(),&pt,TPM_RIGHTBUTTON);
	}
}


bool CMainPanel::CFormEventHandler::OnKeyDown(UINT KeyCode,UINT Flags)
{
	g_App.MainWindow.SendMessage(WM_KEYDOWN,KeyCode,Flags);
	return true;
}


void CMainPanel::CChannelPanelEventHandler::OnChannelClick(const CChannelInfo *pChannelInfo)
{
	const CChannelList *pList=g_App.ChannelManager.GetCurrentChannelList();

	if (pList!=NULL) {
#ifdef NETWORK_REMOCON_SUPPORT
		if (g_App.pNetworkRemocon!=NULL) {
			g_App.UICore.DoCommandAsync(CM_CHANNELNO_FIRST+pChannelInfo->GetChannelNo()-1);
		} else
#endif
		{
			int Index=pList->FindByIndex(pChannelInfo->GetSpace(),
										 pChannelInfo->GetChannelIndex(),
										 pChannelInfo->GetServiceID());
			if (Index<0 && pChannelInfo->GetServiceID()>0)
				Index=pList->FindByIndex(pChannelInfo->GetSpace(),
										 pChannelInfo->GetChannelIndex());
			if (Index>=0)
				g_App.UICore.DoCommandAsync(CM_CHANNEL_FIRST+Index);
		}
	}
}


void CMainPanel::CChannelPanelEventHandler::OnRButtonDown()
{
	CPopupMenu Menu(g_App.GetResourceInstance(),IDM_CHANNELPANEL);

	Menu.CheckItem(CM_CHANNELPANEL_DETAILPOPUP,g_App.Panel.ChannelPanel.GetDetailToolTip());
	Menu.CheckItem(CM_CHANNELPANEL_SCROLLTOCURCHANNEL,g_App.Panel.ChannelPanel.GetScrollToCurChannel());
	Menu.CheckRadioItem(CM_CHANNELPANEL_EVENTS_1,CM_CHANNELPANEL_EVENTS_4,
						CM_CHANNELPANEL_EVENTS_1+g_App.Panel.ChannelPanel.GetEventsPerChannel()-1);
	Menu.CheckRadioItem(CM_CHANNELPANEL_EXPANDEVENTS_2,CM_CHANNELPANEL_EXPANDEVENTS_8,
						CM_CHANNELPANEL_EXPANDEVENTS_2+g_App.Panel.ChannelPanel.GetExpandAdditionalEvents()-2);
	Menu.CheckItem(CM_CHANNELPANEL_USEEPGCOLORSCHEME,g_App.Panel.ChannelPanel.GetUseEpgColorScheme());
	Menu.CheckItem(CM_CHANNELPANEL_SHOWGENRECOLOR,g_App.Panel.ChannelPanel.GetShowGenreColor());
	Menu.EnableItem(CM_CHANNELPANEL_SHOWGENRECOLOR,!g_App.Panel.ChannelPanel.GetUseEpgColorScheme());
	Menu.Show(g_App.UICore.GetMainWindow());
}




void CDisplayEventHandlerBase::RelayMouseMessage(CDisplayView *pView,UINT Message,int x,int y)
{
	if (pView==nullptr)
		return;
	HWND hwndParent=pView->GetParent();
	POINT pt={x,y};
	::MapWindowPoints(pView->GetHandle(),hwndParent,&pt,1);
	::SendMessage(hwndParent,Message,0,MAKELPARAM(pt.x,pt.y));
}




CEpg::CEpg(CEpgProgramList &EpgProgramList,CEventSearchOptions &EventSearchOptions)
	: ProgramGuide(EventSearchOptions)
	, ProgramGuideFrame(&ProgramGuide,&ProgramGuideFrameSettings)
	, ProgramGuideDisplay(&ProgramGuide,&ProgramGuideFrameSettings)
	, fShowProgramGuide(false)
{
	ProgramGuide.SetEpgProgramList(&EpgProgramList);
	ProgramGuide.SetEventHandler(&m_ProgramGuideEventHandler);
	ProgramGuide.SetProgramCustomizer(&m_ProgramCustomizer);
	ProgramGuide.SetChannelProviderManager(&m_ChannelProviderManager);

	ProgramGuideDisplay.SetEventHandler(&m_ProgramGuideDisplayEventHandler);
}


CEpg::CChannelProviderManager *CEpg::CreateChannelProviderManager(LPCTSTR pszDefaultTuner)
{
	m_ChannelProviderManager.Create(pszDefaultTuner);
	return &m_ChannelProviderManager;
}


CEpg::CChannelProviderManager::CChannelProviderManager()
	: m_CurChannelProvider(-1)
{
}


CEpg::CChannelProviderManager::~CChannelProviderManager()
{
	Clear();
}


size_t CEpg::CChannelProviderManager::GetChannelProviderCount() const
{
	return m_ChannelProviderList.size();
}


CProgramGuideChannelProvider *CEpg::CChannelProviderManager::GetChannelProvider(size_t Index) const
{
	if (Index>=m_ChannelProviderList.size())
		return NULL;
	return m_ChannelProviderList[Index];
}


bool CEpg::CChannelProviderManager::Create(LPCTSTR pszDefaultTuner)
{
	Clear();

	m_ChannelProviderList.push_back(new CFavoritesChannelProvider);
	if (pszDefaultTuner!=NULL && ::lstrcmpi(pszDefaultTuner,TEXT("favorites"))==0)
		m_CurChannelProvider=0;

	CProgramGuideBaseChannelProvider *pCurChannelProvider=NULL;
	String DefaultTuner;
	if (m_CurChannelProvider<0) {
		if (!IsStringEmpty(pszDefaultTuner)) {
			pCurChannelProvider=new CBonDriverChannelProvider(pszDefaultTuner);
			DefaultTuner=pszDefaultTuner;
		} else if (pszDefaultTuner==NULL
				&& g_App.CoreEngine.IsDriverSpecified()
				&& !g_App.CoreEngine.IsNetworkDriver()) {
			DefaultTuner=g_App.CoreEngine.GetDriverFileName();
			pCurChannelProvider=new CProgramGuideBaseChannelProvider(
				g_App.ChannelManager.GetTuningSpaceList(),
				DefaultTuner.c_str());
		}
	}

	for (int i=0;i<g_App.DriverManager.NumDrivers();i++) {
		const CDriverInfo *pDriverInfo=g_App.DriverManager.GetDriverInfo(i);

		if (pDriverInfo!=NULL) {
			if (pCurChannelProvider!=NULL
					&& IsEqualFileName(DefaultTuner.c_str(),pDriverInfo->GetFileName())) {
				m_CurChannelProvider=(int)m_ChannelProviderList.size();
				m_ChannelProviderList.push_back(pCurChannelProvider);
				pCurChannelProvider=NULL;
			} else if (!CCoreEngine::IsNetworkDriverFileName(pDriverInfo->GetFileName())) {
				CBonDriverChannelProvider *pDriverChannelProvider=
					new CBonDriverChannelProvider(pDriverInfo->GetFileName());

				m_ChannelProviderList.push_back(pDriverChannelProvider);
			}
		}
	}

	if (pCurChannelProvider!=NULL) {
		auto itr=m_ChannelProviderList.begin();
		++itr;
		m_ChannelProviderList.insert(itr,pCurChannelProvider);
		m_CurChannelProvider=1;
	}

	return true;
}


void CEpg::CChannelProviderManager::Clear()
{
	for (size_t i=0;i<m_ChannelProviderList.size();i++)
		delete m_ChannelProviderList[i];
	m_ChannelProviderList.clear();
	m_CurChannelProvider=-1;
}


CEpg::CChannelProviderManager::CBonDriverChannelProvider::CBonDriverChannelProvider(LPCTSTR pszFileName)
	: CProgramGuideBaseChannelProvider(NULL,pszFileName)
{
}


bool CEpg::CChannelProviderManager::CBonDriverChannelProvider::Update()
{
	CDriverInfo DriverInfo(m_BonDriverFileName.c_str());

	if (!DriverInfo.LoadTuningSpaceList())
		return false;

	m_TuningSpaceList=*DriverInfo.GetTuningSpaceList();

	return true;
}


CEpg::CChannelProviderManager::CFavoritesChannelProvider::~CFavoritesChannelProvider()
{
	ClearGroupList();
}


bool CEpg::CChannelProviderManager::CFavoritesChannelProvider::Update()
{
	ClearGroupList();
	AddFavoritesChannels(g_App.FavoritesManager.GetRootFolder(),TVTest::String());
	m_GroupList.front()->Name=TEXT("お気に入り");

	const int NumSpaces=static_cast<int>(m_GroupList.size());
	m_TuningSpaceList.Clear();
	m_TuningSpaceList.Reserve(NumSpaces);
	for (int i=0;i<NumSpaces;i++) {
		const GroupInfo *pGroup=m_GroupList[i];
		CTuningSpaceInfo *pTuningSpace=m_TuningSpaceList.GetTuningSpaceInfo(i);
		pTuningSpace->SetName(pGroup->Name.c_str());
		CChannelList *pChannelList=pTuningSpace->GetChannelList();
		for (auto itr=pGroup->ChannelList.begin();itr!=pGroup->ChannelList.end();++itr) {
			pChannelList->AddChannel(itr->GetChannelInfo());
		}
	}

	return true;
}


bool CEpg::CChannelProviderManager::CFavoritesChannelProvider::GetName(
	LPTSTR pszName,int MaxName) const
{
	if (pszName==NULL || MaxName<1)
		return false;

	::lstrcpyn(pszName,TEXT("お気に入りチャンネル"),MaxName);

	return true;
}


bool CEpg::CChannelProviderManager::CFavoritesChannelProvider::GetGroupID(
	size_t Group,TVTest::String *pID) const
{
	if (Group>=m_GroupList.size() || pID==NULL)
		return false;
	*pID=m_GroupList[Group]->ID;
	return true;
}


int CEpg::CChannelProviderManager::CFavoritesChannelProvider::ParseGroupID(
	LPCTSTR pszID) const
{
	if (IsStringEmpty(pszID))
		return -1;

	for (size_t i=0;i<m_GroupList.size();i++) {
		if (m_GroupList[i]->ID.compare(pszID)==0)
			return static_cast<int>(i);
	}

	// 以前のバージョンとの互換用
	if (::lstrcmp(pszID,TEXT("0"))==0)
		return 0;

	return -1;
}


bool CEpg::CChannelProviderManager::CFavoritesChannelProvider::GetBonDriver(
	LPTSTR pszFileName,int MaxLength) const
{
	if (pszFileName==NULL || MaxLength<1
			|| m_GroupList.empty()
			|| m_GroupList.front()->ChannelList.empty())
		return false;

	LPCTSTR pszBonDriver=m_GroupList.front()->ChannelList.front().GetBonDriverFileName();
	if (IsStringEmpty(pszBonDriver))
		return false;

	for (size_t i=0;i<m_GroupList.size();i++) {
		const GroupInfo *pGroup=m_GroupList[i];
		for (auto itr=pGroup->ChannelList.begin();
				itr!=pGroup->ChannelList.end();++itr) {
			if (!IsEqualFileName(itr->GetBonDriverFileName(),pszBonDriver))
				return false;
		}
	}

	::lstrcpyn(pszFileName,pszBonDriver,MaxLength);

	return false;
}


bool CEpg::CChannelProviderManager::CFavoritesChannelProvider::GetBonDriverFileName(
	size_t Group,size_t Channel,LPTSTR pszFileName,int MaxLength) const
{
	if (Group>=m_GroupList.size()
			|| Channel>=m_GroupList[Group]->ChannelList.size()
			|| pszFileName==NULL || MaxLength<1)
		return false;

	const CFavoriteChannel &FavoriteChannel=m_GroupList[Group]->ChannelList[Channel];
	const CChannelInfo &ChannelInfo=FavoriteChannel.GetChannelInfo();

	if (!FavoriteChannel.GetForceBonDriverChange()
			&& g_App.CoreEngine.IsTunerOpen()) {
		int Space=g_App.ChannelManager.GetCurrentSpace();
		if (Space!=CChannelManager::SPACE_INVALID) {
			int Index=g_App.ChannelManager.FindChannelByIDs(Space,
				ChannelInfo.GetNetworkID(),
				ChannelInfo.GetTransportStreamID(),
				ChannelInfo.GetServiceID());
			if (Index<0 && Space!=CChannelManager::SPACE_ALL) {
				for (Space=0;Space<g_App.ChannelManager.NumSpaces();Space++) {
					Index=g_App.ChannelManager.FindChannelByIDs(Space,
						ChannelInfo.GetNetworkID(),
						ChannelInfo.GetTransportStreamID(),
						ChannelInfo.GetServiceID());
					if (Index>=0)
						break;
				}
			}
			if (Index>=0) {
				pszFileName[0]=_T('\0');
				return true;
			}
		}
	}

	::lstrcpyn(pszFileName,FavoriteChannel.GetBonDriverFileName(),MaxLength);

	return true;
}


void CEpg::CChannelProviderManager::CFavoritesChannelProvider::ClearGroupList()
{
	if (!m_GroupList.empty()) {
		for (auto itr=m_GroupList.begin();itr!=m_GroupList.end();++itr)
			delete *itr;
		m_GroupList.clear();
	}
}


void CEpg::CChannelProviderManager::CFavoritesChannelProvider::AddFavoritesChannels(
	const CFavoriteFolder &Folder,const TVTest::String &Path)
{
	GroupInfo *pGroup=new GroupInfo;
	pGroup->Name=Folder.GetName();
	if (Path.empty())
		pGroup->ID=TEXT("\\");
	else
		pGroup->ID=Path;
	m_GroupList.push_back(pGroup);

	for (size_t i=0;i<Folder.GetItemCount();i++) {
		const CFavoriteItem *pItem=Folder.GetItem(i);

		if (pItem->GetType()==CFavoriteItem::ITEM_FOLDER) {
			const CFavoriteFolder *pFolder=static_cast<const CFavoriteFolder*>(pItem);
			TVTest::String FolderPath,Name;
			TVTest::StringUtility::Encode(pItem->GetName(),&Name);
			FolderPath=Path;
			FolderPath+=_T('\\');
			FolderPath+=Name;
			AddSubItems(pGroup,*pFolder);
			AddFavoritesChannels(*pFolder,FolderPath);
		} else if (pItem->GetType()==CFavoriteItem::ITEM_CHANNEL) {
			const CFavoriteChannel *pChannel=static_cast<const CFavoriteChannel*>(pItem);
			pGroup->ChannelList.push_back(*pChannel);
		}
	}
}


void CEpg::CChannelProviderManager::CFavoritesChannelProvider::AddSubItems(
	GroupInfo *pGroup,const CFavoriteFolder &Folder)
{
	for (size_t i=0;i<Folder.GetItemCount();i++) {
		const CFavoriteItem *pItem=Folder.GetItem(i);

		if (pItem->GetType()==CFavoriteItem::ITEM_FOLDER) {
			AddSubItems(pGroup,*static_cast<const CFavoriteFolder*>(pItem));
		} else if (pItem->GetType()==CFavoriteItem::ITEM_CHANNEL) {
			pGroup->ChannelList.push_back(*static_cast<const CFavoriteChannel*>(pItem));
		}
	}
}


bool CEpg::CProgramGuideEventHandler::OnClose()
{
	g_App.Epg.fShowProgramGuide=false;
	g_App.MainMenu.CheckItem(CM_PROGRAMGUIDE,false);
	g_App.SideBar.CheckItem(CM_PROGRAMGUIDE,false);
	return true;
}


void CEpg::CProgramGuideEventHandler::OnDestroy()
{
	m_pProgramGuide->Clear();

	if (g_App.CmdLineOptions.m_fProgramGuideOnly
			&& g_App.UICore.GetStandby()
			&& !g_App.RecordManager.IsRecording()
			&& !g_App.RecordManager.IsReserved())
		g_App.UICore.DoCommandAsync(CM_EXIT);
}


int CEpg::CProgramGuideEventHandler::FindChannel(const CChannelList *pChannelList,const CServiceInfoData *pServiceInfo)
{
	for (int i=0;i<pChannelList->NumChannels();i++) {
		const CChannelInfo *pChannelInfo=pChannelList->GetChannelInfo(i);

		if (pChannelInfo->GetTransportStreamID()==pServiceInfo->m_TSID
				&& pChannelInfo->GetServiceID()==pServiceInfo->m_ServiceID
				&& pChannelInfo->IsEnabled())
			return i;
	}
	return -1;
}


void CEpg::CProgramGuideEventHandler::OnServiceTitleLButtonDown(LPCTSTR pszDriverFileName,const CServiceInfoData *pServiceInfo)
{
	if (!g_App.UICore.ConfirmChannelChange())
		return;

	const bool fSetBonDriver=!IsStringEmpty(pszDriverFileName);
	CMainWindow::ResumeInfo &ResumeInfo=g_App.MainWindow.GetResumeInfo();
	ResumeInfo.fSetChannel=false;
	ResumeInfo.fOpenTuner=!fSetBonDriver;

	g_App.UICore.DoCommand(CM_SHOW);

	if (fSetBonDriver) {
		if (!g_App.Core.OpenTuner(pszDriverFileName))
			return;
	}

	const CChannelList *pChannelList=g_App.ChannelManager.GetCurrentChannelList();
	if (pChannelList!=NULL) {
		int Index=FindChannel(pChannelList,pServiceInfo);
		if (Index>=0) {
#ifdef NETWORK_REMOCON_SUPPORT
			if (g_App.pNetworkRemocon!=NULL) {
				g_App.pNetworkRemocon->SetChannel(pChannelList->GetChannelInfo(Index)->GetChannelNo()-1);
				g_App.ChannelManager.SetNetworkRemoconCurrentChannel(Index);
				g_App.AppEventManager.OnChannelChanged(0);
			} else
#endif
			{
				g_App.Core.SetChannel(g_App.ChannelManager.GetCurrentSpace(),Index);
			}
			return;
		}
	}
	for (int i=0;i<g_App.ChannelManager.NumSpaces();i++) {
		pChannelList=g_App.ChannelManager.GetChannelList(i);
		if (pChannelList!=NULL) {
			int Index=FindChannel(pChannelList,pServiceInfo);
			if (Index>=0) {
				g_App.Core.SetChannel(i,Index);
				return;
			}
		}
	}
}


bool CEpg::CProgramGuideEventHandler::OnBeginUpdate(LPCTSTR pszBonDriver,const CChannelList *pChannelList)
{
	if (g_App.CmdLineOptions.m_fNoEpg) {
		g_App.MainWindow.ShowMessage(
			TEXT("コマンドラインオプションでEPG情報を取得しないように指定されているため、\n番組表の取得ができません。"),
			TEXT("お知らせ"),MB_OK | MB_ICONINFORMATION);
		return false;
	}
	if (g_App.RecordManager.IsRecording()) {
		g_App.MainWindow.ShowMessage(
			TEXT("録画を停止させてから番組表を取得してください。"),
			TEXT("お知らせ"),MB_OK | MB_ICONINFORMATION);
		return false;
	}
	if (g_App.CoreEngine.IsNetworkDriverFileName(pszBonDriver)) {
		g_App.MainWindow.ShowMessage(
			TEXT("UDP/TCPでは番組表の取得はできません。"),
			TEXT("お知らせ"),MB_OK | MB_ICONINFORMATION);
		return false;
	}

	return g_App.MainWindow.BeginProgramGuideUpdate(pszBonDriver,pChannelList);
}


void CEpg::CProgramGuideEventHandler::OnEndUpdate()
{
	g_App.MainWindow.OnProgramGuideUpdateEnd();
}


bool CEpg::CProgramGuideEventHandler::OnKeyDown(UINT KeyCode,UINT Flags)
{
	g_App.MainWindow.SendMessage(WM_KEYDOWN,KeyCode,Flags);
	return true;
}


bool CEpg::CProgramGuideEventHandler::OnMenuInitialize(HMENU hmenu,UINT CommandBase)
{
	return g_App.PluginManager.SendProgramGuideInitializeMenuEvent(hmenu,&CommandBase);
}


bool CEpg::CProgramGuideEventHandler::OnMenuSelected(UINT Command)
{
	return g_App.PluginManager.SendProgramGuideMenuSelectedEvent(Command);
}


bool CEpg::CProgramGuideDisplayEventHandler::OnHide()
{
	m_pProgramGuideDisplay->Destroy();
	g_App.Epg.fShowProgramGuide=false;
	g_App.MainMenu.CheckItem(CM_PROGRAMGUIDE,g_App.Epg.fShowProgramGuide);
	g_App.SideBar.CheckItem(CM_PROGRAMGUIDE,g_App.Epg.fShowProgramGuide);
	return true;
}


bool CEpg::CProgramGuideDisplayEventHandler::SetAlwaysOnTop(bool fTop)
{
	return g_App.UICore.SetAlwaysOnTop(fTop);
}


bool CEpg::CProgramGuideDisplayEventHandler::GetAlwaysOnTop() const
{
	return g_App.UICore.GetAlwaysOnTop();
}


void CEpg::CProgramGuideDisplayEventHandler::OnMouseMessage(UINT Msg,int x,int y)
{
	RelayMouseMessage(m_pProgramGuideDisplay,Msg,x,y);
}


bool CEpg::CProgramGuideProgramCustomizer::Initialize()
{
	return g_App.PluginManager.SendProgramGuideInitializeEvent(m_pProgramGuide->GetHandle());
}


void CEpg::CProgramGuideProgramCustomizer::Finalize()
{
	g_App.PluginManager.SendProgramGuideFinalizeEvent(m_pProgramGuide->GetHandle());
}


bool CEpg::CProgramGuideProgramCustomizer::DrawBackground(
	const CEventInfoData &Event,HDC hdc,
	const RECT &ItemRect,const RECT &TitleRect,const RECT &ContentRect,
	COLORREF BackgroundColor)
{
	return g_App.PluginManager.SendProgramGuideProgramDrawBackgroundEvent(
		Event,hdc,ItemRect,TitleRect,ContentRect,BackgroundColor);
}


bool CEpg::CProgramGuideProgramCustomizer::InitializeMenu(
	const CEventInfoData &Event,HMENU hmenu,UINT CommandBase,
	const POINT &CursorPos,const RECT &ItemRect)
{
	return g_App.PluginManager.SendProgramGuideProgramInitializeMenuEvent(
		Event,hmenu,&CommandBase,CursorPos,ItemRect);
}


bool CEpg::CProgramGuideProgramCustomizer::ProcessMenu(
	const CEventInfoData &Event,UINT Command)
{
	return g_App.PluginManager.SendProgramGuideProgramMenuSelectedEvent(Event,Command);
}


bool CEpg::CProgramGuideProgramCustomizer::OnLButtonDoubleClick(
	const CEventInfoData &Event,const POINT &CursorPos,const RECT &ItemRect)
{
	LPCTSTR pszCommand=g_App.ProgramGuideOptions.GetProgramLDoubleClickCommand();
	if (IsStringEmpty(pszCommand))
		return false;
	int Command=g_App.ProgramGuideOptions.ParseCommand(pszCommand);
	if (Command>0) {
		m_pProgramGuide->SendMessage(WM_COMMAND,Command,0);
		return true;
	}
	return g_App.PluginManager.OnProgramGuideCommand(pszCommand,
		TVTest::PROGRAMGUIDE_COMMAND_ACTION_MOUSE,&Event,&CursorPos,&ItemRect);
}




void CEpgLoadEventHandler::OnBeginLoad()
{
	TRACE(TEXT("Start EPG file loading ...\n"));
}


void CEpgLoadEventHandler::OnEndLoad(bool fSuccess)
{
	TRACE(TEXT("End EPG file loading : %s\n"),fSuccess?TEXT("Succeeded"):TEXT("Failed"));
	if (fSuccess)
		g_App.MainWindow.PostMessage(WM_APP_EPGLOADED,0,0);
}


void CEpgLoadEventHandler::OnStart()
{
	TRACE(TEXT("Start EDCB data loading ...\n"));
}


void CEpgLoadEventHandler::OnEnd(bool fSuccess,CEventManager *pEventManager)
{
	TRACE(TEXT("End EDCB data loading : %s\n"),fSuccess?TEXT("Succeeded"):TEXT("Failed"));
	if (fSuccess) {
		CEventManager::ServiceList ServiceList;

		if (pEventManager->GetServiceList(&ServiceList)) {
			for (size_t i=0;i<ServiceList.size();i++) {
				g_App.EpgProgramList.UpdateService(pEventManager,&ServiceList[i],
					CEpgProgramList::SERVICE_UPDATE_DATABASE);
			}
			g_App.MainWindow.PostMessage(WM_APP_EPGLOADED,0,0);
		}
	}
}




void CSideBarOptionsEventHandler::OnItemChanged()
{
	const CUICore &UICore=g_App.UICore;

	g_App.SideBar.CheckRadioItem(CM_ASPECTRATIO_FIRST,CM_ASPECTRATIO_LAST,
								 CM_ASPECTRATIO_FIRST+g_App.MainWindow.GetAspectRatioType());
	g_App.SideBar.CheckItem(CM_FULLSCREEN,UICore.GetFullscreen());
	g_App.SideBar.CheckItem(CM_ALWAYSONTOP,UICore.GetAlwaysOnTop());
	g_App.SideBar.CheckItem(CM_DISABLEVIEWER,!g_App.MainWindow.IsPlaybackEnabled());
	g_App.SideBar.CheckItem(CM_CAPTUREPREVIEW,g_App.CaptureWindow.GetVisible());
	g_App.SideBar.CheckItem(CM_PANEL,g_App.MainWindow.IsPanelPresent());
	g_App.SideBar.CheckItem(CM_PROGRAMGUIDE,g_App.Epg.fShowProgramGuide);
	g_App.SideBar.CheckItem(CM_STATUSBAR,g_App.MainWindow.GetStatusBarVisible());
	g_App.SideBar.CheckItem(CM_STREAMINFO,g_App.StreamInfo.IsVisible());
	g_App.SideBar.CheckItem(CM_1SEGMODE,g_App.Core.Is1SegMode());
	//g_App.SideBar.CheckItem(CM_HOMEDISPLAY,g_App.HomeDisplay.GetVisible());
	//g_App.SideBar.CheckItem(CM_CHANNELDISPLAY,g_App.ChannelDisplay.GetVisible());
	const CChannelInfo *pCurChannel=g_App.ChannelManager.GetCurrentChannelInfo();
	int ChannelNo;
	if (pCurChannel!=NULL)
		ChannelNo=pCurChannel->GetChannelNo();
	g_App.SideBar.CheckRadioItem(CM_CHANNELNO_1,CM_CHANNELNO_12,
								 pCurChannel!=NULL && ChannelNo>=1 && ChannelNo<=12?
								 CM_CHANNELNO_1+ChannelNo-1:0);
}




void CHomeDisplayEventHandler::OnClose()
{
	g_App.HomeDisplay.SetVisible(false);
}


void CHomeDisplayEventHandler::OnMouseMessage(UINT Msg,int x,int y)
{
	RelayMouseMessage(m_pHomeDisplay,Msg,x,y);
}




void CChannelDisplayEventHandler::OnTunerSelect(LPCTSTR pszDriverFileName,int TuningSpace)
{
	if (g_App.CoreEngine.IsTunerOpen()
			&& IsEqualFileName(g_App.CoreEngine.GetDriverFileName(),pszDriverFileName)) {
		g_App.UICore.DoCommand(CM_CHANNELDISPLAY);
	} else {
		if (!g_App.UICore.ConfirmChannelChange())
			return;

		if (g_App.Core.OpenTuner(pszDriverFileName)) {
			if (TuningSpace!=SPACE_NOTSPECIFIED) {
				g_App.UICore.DoCommand(CM_SPACE_FIRST+TuningSpace);
				if (TuningSpace==SPACE_ALL
						|| TuningSpace==g_App.RestoreChannelInfo.Space)
					g_App.Core.RestoreChannel();
			} else {
				g_App.Core.RestoreChannel();
			}
		}
		g_App.UICore.DoCommand(CM_CHANNELDISPLAY);
	}
}


void CChannelDisplayEventHandler::OnChannelSelect(LPCTSTR pszDriverFileName,const CChannelInfo *pChannelInfo)
{
	if (!g_App.UICore.ConfirmChannelChange())
		return;

	if (g_App.Core.OpenTuner(pszDriverFileName)) {
		int Space;
		if (g_App.RestoreChannelInfo.fAllChannels)
			Space=CChannelManager::SPACE_ALL;
		else
			Space=pChannelInfo->GetSpace();
		const CChannelList *pList=g_App.ChannelManager.GetChannelList(Space);
		if (pList!=NULL) {
			int Index=pList->FindByIndex(pChannelInfo->GetSpace(),
										 pChannelInfo->GetChannelIndex(),
										 pChannelInfo->GetServiceID());

			if (Index<0 && Space==CChannelManager::SPACE_ALL) {
				Space=pChannelInfo->GetSpace();
				pList=g_App.ChannelManager.GetChannelList(Space);
				if (pList!=NULL)
					Index=pList->FindByIndex(-1,
											 pChannelInfo->GetChannelIndex(),
											 pChannelInfo->GetServiceID());
			}
			if (Index>=0)
				g_App.Core.SetChannel(Space,Index);
		}
		g_App.UICore.DoCommand(CM_CHANNELDISPLAY);
	}
}


void CChannelDisplayEventHandler::OnClose()
{
	g_App.ChannelDisplay.SetVisible(false);
}


void CChannelDisplayEventHandler::OnMouseMessage(UINT Msg,int x,int y)
{
	RelayMouseMessage(m_pChannelDisplay,Msg,x,y);
}




CServiceUpdateInfo::CServiceUpdateInfo(CDtvEngine *pEngine,CTsAnalyzer *pTsAnalyzer)
{
	CTsAnalyzer::ServiceList ServiceList;

	pTsAnalyzer->GetViewableServiceList(&ServiceList);
	m_NumServices=(int)ServiceList.size();
	m_CurService=-1;
	if (m_NumServices>0) {
		m_pServiceList=new ServiceInfo[m_NumServices];
		for (int i=0;i<m_NumServices;i++) {
			const CTsAnalyzer::ServiceInfo *pServiceInfo=&ServiceList[i];
			m_pServiceList[i].ServiceID=pServiceInfo->ServiceID;
			::lstrcpy(m_pServiceList[i].szServiceName,pServiceInfo->szServiceName);
			m_pServiceList[i].LogoID=pServiceInfo->LogoID;
		}
		WORD ServiceID;
		if (pEngine->GetServiceID(&ServiceID)) {
			for (int i=0;i<m_NumServices;i++) {
				if (m_pServiceList[i].ServiceID==ServiceID) {
					m_CurService=i;
					break;
				}
			}
		}
	} else {
		m_pServiceList=NULL;
	}
	m_NetworkID=pTsAnalyzer->GetNetworkID();
	m_TransportStreamID=pTsAnalyzer->GetTransportStreamID();
	m_fServiceListEmpty=pTsAnalyzer->GetServiceNum()==0;
}


CServiceUpdateInfo::~CServiceUpdateInfo()
{
	delete [] m_pServiceList;
}




// エントリポイント
int APIENTRY _tWinMain(HINSTANCE hInstance,HINSTANCE /*hPrevInstance*/,
					   LPTSTR pszCmdLine,int nCmdShow)
{
	// DLLハイジャック対策
	SetDllDirectory(TEXT(""));

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF/* | _CRTDBG_CHECK_ALWAYS_DF*/);
#else
	CDebugHelper::Initialize();
	CDebugHelper::SetExceptionFilterMode(CDebugHelper::EXCEPTION_FILTER_DIALOG);
#endif

	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

	g_App.Logger.AddLog(TEXT("******** ") ABOUT_VERSION_TEXT
#ifdef VERSION_PLATFORM
						TEXT(" (") VERSION_PLATFORM TEXT(")")
#endif
						TEXT(" 起動 ********"));

	CoInitializeEx(NULL,COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE | COINIT_SPEED_OVER_MEMORY);

	const int Result=g_App.Main(hInstance,pszCmdLine,nCmdShow);

	CoUninitialize();

	g_App.Logger.AddLog(TEXT("******** 終了 ********"));
	if (g_App.CmdLineOptions.m_fSaveLog && !g_App.Logger.GetOutputToFile()) {
		TCHAR szFileName[MAX_PATH];

		g_App.Logger.GetDefaultLogFileName(szFileName);
		g_App.Logger.SaveToFile(szFileName,true);
	}

	return Result;
}
