#include "stdafx.h"
#include "TVTest.h"
#include "ViewOptions.h"
#include "AppMain.h"
#include "MainWindow.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CViewOptions::CViewOptions()
	: m_fAdjustAspectResizing(false)
	, m_fSnapAtWindowEdge(false)
	, m_SnapAtWindowEdgeMargin(8)
	, m_fNearCornerResizeOrigin(false)
	, m_fZoomKeepAspectRatio(false)
	, m_PanScanAdjustWindowMode(ADJUSTWINDOW_WIDTH)
	, m_fMinimizeToTray(false)
	, m_fDisablePreviewWhenMinimized(false)
	, m_fUseLogoIcon(false)
	, m_fShowTitleEventTime(false)
	, m_fShowLogo(true)

	, m_fResetPanScanEventChange(true)
	, m_fNoMaskSideCut(true)
	, m_FullscreenStretchMode(CMediaViewer::STRETCH_KEEPASPECTRATIO)
	, m_MaximizeStretchMode(CMediaViewer::STRETCH_KEEPASPECTRATIO)
	, m_fIgnoreDisplayExtension(false)

	, m_fNoScreenSaver(false)
	, m_fNoMonitorLowPower(false)
	, m_fNoMonitorLowPowerActiveOnly(false)
{
	::lstrcpy(m_szLogoFileName,APP_NAME TEXT("_Logo.bmp"));
}


CViewOptions::~CViewOptions()
{
	Destroy();
}


bool CViewOptions::Apply(DWORD Flags)
{
	CAppMain &AppMain=GetAppClass();

	if ((Flags&UPDATE_MASKCUTAREA)!=0) {
		AppMain.GetCoreEngine()->m_DtvEngine.m_MediaViewer.SetNoMaskSideCut(m_fNoMaskSideCut);
	}

	if ((Flags&UPDATE_IGNOREDISPLAYEXTENSION)!=0) {
		AppMain.GetCoreEngine()->m_DtvEngine.m_MediaViewer.SetIgnoreDisplayExtension(m_fIgnoreDisplayExtension);
	}

	if ((Flags&UPDATE_LOGO)!=0) {
		CMainWindow *pMainWindow=dynamic_cast<CMainWindow*>(AppMain.GetUICore()->GetSkin());
		if (pMainWindow!=NULL)
			pMainWindow->SetLogo(m_fShowLogo?m_szLogoFileName:NULL);
	}

	return true;
}


bool CViewOptions::ReadSettings(CSettings &Settings)
{
	int Value;

	Settings.Read(TEXT("AdjustAspectResizing"),&m_fAdjustAspectResizing);
	Settings.Read(TEXT("SnapToWindowEdge"),&m_fSnapAtWindowEdge);
	Settings.Read(TEXT("NearCornerResizeOrigin"),&m_fNearCornerResizeOrigin);
	Settings.Read(TEXT("ZoomKeepAspectRatio"),&m_fZoomKeepAspectRatio);
	if (Settings.Read(TEXT("PanScanAdjustWindow"),&Value)
			&& Value>=ADJUSTWINDOW_FIRST && Value<=ADJUSTWINDOW_LAST) {
		m_PanScanAdjustWindowMode=(AdjustWindowMode)Value;
	} else {
		// 以前のバージョンとの互換用
		bool f;
		if (Settings.Read(TEXT("PanScanNoResizeWindow"),&f))
			m_PanScanAdjustWindowMode=f?ADJUSTWINDOW_WIDTH:ADJUSTWINDOW_FIT;
	}
	Settings.Read(TEXT("MinimizeToTray"),&m_fMinimizeToTray);
	Settings.Read(TEXT("DisablePreviewWhenMinimized"),&m_fDisablePreviewWhenMinimized);
	Settings.Read(TEXT("UseLogoIcon"),&m_fUseLogoIcon);
	Settings.Read(TEXT("TitleEventTime"),&m_fShowTitleEventTime);
	Settings.Read(TEXT("ShowLogo"),&m_fShowLogo);
	Settings.Read(TEXT("LogoFileName"),m_szLogoFileName,lengthof(m_szLogoFileName));
	Settings.Read(TEXT("ResetPanScanEventChange"),&m_fResetPanScanEventChange);
	Settings.Read(TEXT("NoMaskSideCut"),&m_fNoMaskSideCut);
	if (Settings.Read(TEXT("FullscreenStretchMode"),&Value))
		m_FullscreenStretchMode=Value==1?CMediaViewer::STRETCH_CUTFRAME:
										 CMediaViewer::STRETCH_KEEPASPECTRATIO;
	if (Settings.Read(TEXT("MaximizeStretchMode"),&Value))
		m_MaximizeStretchMode=Value==1?CMediaViewer::STRETCH_CUTFRAME:
									   CMediaViewer::STRETCH_KEEPASPECTRATIO;
	Settings.Read(TEXT("IgnoreDisplayExtension"),&m_fIgnoreDisplayExtension);
	Settings.Read(TEXT("NoScreenSaver"),&m_fNoScreenSaver);
	Settings.Read(TEXT("NoMonitorLowPower"),&m_fNoMonitorLowPower);
	Settings.Read(TEXT("NoMonitorLowPowerActiveOnly"),&m_fNoMonitorLowPowerActiveOnly);

	return true;
}


bool CViewOptions::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("AdjustAspectResizing"),m_fAdjustAspectResizing);
	Settings.Write(TEXT("SnapToWindowEdge"),m_fSnapAtWindowEdge);
	Settings.Write(TEXT("NearCornerResizeOrigin"),m_fNearCornerResizeOrigin);
	Settings.Write(TEXT("ZoomKeepAspectRatio"),m_fZoomKeepAspectRatio);
	Settings.Write(TEXT("PanScanAdjustWindow"),(int)m_PanScanAdjustWindowMode);
	Settings.Write(TEXT("MinimizeToTray"),m_fMinimizeToTray);
	Settings.Write(TEXT("DisablePreviewWhenMinimized"),m_fDisablePreviewWhenMinimized);
	Settings.Write(TEXT("UseLogoIcon"),m_fUseLogoIcon);
	Settings.Write(TEXT("TitleEventTime"),m_fShowTitleEventTime);
	Settings.Write(TEXT("ShowLogo"),m_fShowLogo);
	Settings.Write(TEXT("LogoFileName"),m_szLogoFileName);
	Settings.Write(TEXT("ResetPanScanEventChange"),m_fResetPanScanEventChange);
	Settings.Write(TEXT("NoMaskSideCut"),m_fNoMaskSideCut);
	Settings.Write(TEXT("FullscreenStretchMode"),(int)m_FullscreenStretchMode);
	Settings.Write(TEXT("MaximizeStretchMode"),(int)m_MaximizeStretchMode);
	Settings.Write(TEXT("IgnoreDisplayExtension"),m_fIgnoreDisplayExtension);
	Settings.Write(TEXT("NoScreenSaver"),m_fNoScreenSaver);
	Settings.Write(TEXT("NoMonitorLowPower"),m_fNoMonitorLowPower);
	Settings.Write(TEXT("NoMonitorLowPowerActiveOnly"),m_fNoMonitorLowPowerActiveOnly);

	return true;
}


bool CViewOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_VIEW));
}


INT_PTR CViewOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			// ウィンドウ設定
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_SNAPATWINDOWEDGE,
							  m_fSnapAtWindowEdge);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_ADJUSTASPECTRESIZING,
							  m_fAdjustAspectResizing);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_NEARCORNERRESIZEORIGIN,
							  m_fNearCornerResizeOrigin);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_ZOOMKEEPASPECTRATIO,m_fZoomKeepAspectRatio);
			{
				static const LPCTSTR AdjustWindowModeList[] = {
					TEXT("サイズを変えない"),
					TEXT("幅のみ変える"),
					TEXT("幅と高さを変える"),
				};
				for (int i=0;i<lengthof(AdjustWindowModeList);i++) {
					DlgComboBox_AddString(hDlg,IDC_OPTIONS_PANSCANADJUSTWINDOW,
										  AdjustWindowModeList[i]);
				}
				DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_PANSCANADJUSTWINDOW,
									  m_PanScanAdjustWindowMode);
			}
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_MINIMIZETOTRAY,m_fMinimizeToTray);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_MINIMIZEDISABLEPREVIEW,
							  m_fDisablePreviewWhenMinimized);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_USELOGOICON,
							  m_fUseLogoIcon);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_SHOWTITLEEVENTTIME,
							  m_fShowTitleEventTime);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_SHOWLOGO,m_fShowLogo);
			::SetDlgItemText(hDlg,IDC_OPTIONS_LOGOFILENAME,m_szLogoFileName);
			::SendDlgItemMessage(hDlg,IDC_OPTIONS_LOGOFILENAME,EM_LIMITTEXT,MAX_PATH-1,0);
			::EnableDlgItems(hDlg,IDC_OPTIONS_LOGOFILENAME,IDC_OPTIONS_LOGOFILENAME_BROWSE,
							 m_fShowLogo);

			// 映像表示設定
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_RESETPANSCANEVENTCHANGE,
							  m_fResetPanScanEventChange);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_NOMASKSIDECUT,
							  m_fNoMaskSideCut);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_FULLSCREENCUTFRAME,
				m_FullscreenStretchMode==CMediaViewer::STRETCH_CUTFRAME);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_MAXIMIZECUTFRAME,
				m_MaximizeStretchMode==CMediaViewer::STRETCH_CUTFRAME);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_IGNOREDISPLAYSIZE,
							  m_fIgnoreDisplayExtension);

			// 抑止設定
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_NOSCREENSAVER,m_fNoScreenSaver);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_NOMONITORLOWPOWER,m_fNoMonitorLowPower);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_NOMONITORLOWPOWERACTIVEONLY,
							  m_fNoMonitorLowPowerActiveOnly);
			EnableDlgItem(hDlg,IDC_OPTIONS_NOMONITORLOWPOWERACTIVEONLY,m_fNoMonitorLowPower);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OPTIONS_SHOWLOGO:
			EnableDlgItemsSyncCheckBox(hDlg,IDC_OPTIONS_LOGOFILENAME,IDC_OPTIONS_LOGOFILENAME_BROWSE,
									   IDC_OPTIONS_SHOWLOGO);
			return TRUE;

		case IDC_OPTIONS_LOGOFILENAME_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH],szInitDir[MAX_PATH];
				CFilePath FilePath;

				::GetDlgItemText(hDlg,IDC_OPTIONS_LOGOFILENAME,szFileName,lengthof(szFileName));
				FilePath.SetPath(szFileName);
				if (FilePath.GetDirectory(szInitDir)) {
					::lstrcpy(szFileName,FilePath.GetFileName());
				} else {
					GetAppClass().GetAppDirectory(szInitDir);
				}
				InitOpenFileName(&ofn);
				ofn.hwndOwner=hDlg;
				ofn.lpstrFilter=
					TEXT("BMPファイル(*.bmp)\0*.bmp\0")
					TEXT("すべてのファイル\0*.*\0");
				ofn.lpstrFile=szFileName;
				ofn.nMaxFile=lengthof(szFileName);
				ofn.lpstrInitialDir=szInitDir;
				ofn.lpstrTitle=TEXT("ロゴ画像の選択");
				ofn.Flags=OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_EXPLORER;
				if (::GetOpenFileName(&ofn)) {
					::SetDlgItemText(hDlg,IDC_OPTIONS_LOGOFILENAME,szFileName);
				}
			}
			return TRUE;

		case IDC_OPTIONS_NOMONITORLOWPOWER:
			EnableDlgItemSyncCheckBox(hDlg,IDC_OPTIONS_NOMONITORLOWPOWERACTIVEONLY,
									  IDC_OPTIONS_NOMONITORLOWPOWER);
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CAppMain &AppMain=GetAppClass();
				bool f;

				m_fSnapAtWindowEdge=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_SNAPATWINDOWEDGE);
				m_fAdjustAspectResizing=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_ADJUSTASPECTRESIZING);
				m_fNearCornerResizeOrigin=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_NEARCORNERRESIZEORIGIN);
				m_fZoomKeepAspectRatio=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_ZOOMKEEPASPECTRATIO);
				m_PanScanAdjustWindowMode=(AdjustWindowMode)
					DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_PANSCANADJUSTWINDOW);
				m_fMinimizeToTray=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_MINIMIZETOTRAY);
				m_fDisablePreviewWhenMinimized=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_MINIMIZEDISABLEPREVIEW);
				f=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_USELOGOICON);
				if (m_fUseLogoIcon!=f) {
					m_fUseLogoIcon=f;
					GetAppClass().GetUICore()->UpdateIcon();
				}
				f=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_SHOWTITLEEVENTTIME);
				if (m_fShowTitleEventTime!=f) {
					m_fShowTitleEventTime=f;
					GetAppClass().GetUICore()->UpdateTitle();
				}
				{
					bool fLogo=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_SHOWLOGO);
					TCHAR szFileName[MAX_PATH];

					::GetDlgItemText(hDlg,IDC_OPTIONS_LOGOFILENAME,szFileName,MAX_PATH);
					if (fLogo!=m_fShowLogo
							|| ::lstrcmp(szFileName,m_szLogoFileName)!=0) {
						m_fShowLogo=fLogo;
						::lstrcpy(m_szLogoFileName,szFileName);
						SetUpdateFlag(UPDATE_LOGO);
					}
				}

				m_fResetPanScanEventChange=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_RESETPANSCANEVENTCHANGE);
				f=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_NOMASKSIDECUT);
				if (m_fNoMaskSideCut!=f) {
					m_fNoMaskSideCut=f;
					SetUpdateFlag(UPDATE_MASKCUTAREA);
				}
				m_FullscreenStretchMode=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_FULLSCREENCUTFRAME)?
					CMediaViewer::STRETCH_CUTFRAME:CMediaViewer::STRETCH_KEEPASPECTRATIO;
				if (AppMain.GetUICore()->GetFullscreen())
					AppMain.GetCoreEngine()->m_DtvEngine.m_MediaViewer.SetViewStretchMode(m_FullscreenStretchMode);
				m_MaximizeStretchMode=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_MAXIMIZECUTFRAME)?
					CMediaViewer::STRETCH_CUTFRAME:CMediaViewer::STRETCH_KEEPASPECTRATIO;
				if (::IsZoomed(AppMain.GetUICore()->GetMainWindow()))
					AppMain.GetCoreEngine()->m_DtvEngine.m_MediaViewer.SetViewStretchMode(m_MaximizeStretchMode);
				f=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_IGNOREDISPLAYSIZE);
				if (m_fIgnoreDisplayExtension!=f) {
					m_fIgnoreDisplayExtension=f;
					SetUpdateFlag(UPDATE_IGNOREDISPLAYEXTENSION);
				}

				m_fNoScreenSaver=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_NOSCREENSAVER);
				m_fNoMonitorLowPower=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_NOMONITORLOWPOWER);
				m_fNoMonitorLowPowerActiveOnly=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_NOMONITORLOWPOWERACTIVEONLY);
				AppMain.GetUICore()->PreventDisplaySave(true);

				m_fChanged=true;
			}
			break;
		}
		break;
	}

	return FALSE;
}
