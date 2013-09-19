#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "EpgOptions.h"
#include "DialogUtil.h"
#include "DrawUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CEpgOptions::CEpgOptions()
	: m_fSaveEpgFile(true)
	, m_fUpdateWhenStandby(false)
	, m_fUpdateBSExtended(false)
	, m_fUpdateCSExtended(false)
	, m_fUseEDCBData(false)
	, m_fSaveLogoFile(true)

	, m_hLoadThread(NULL)
	, m_pEpgDataLoader(NULL)
{
	::lstrcpy(m_szEpgFileName,TEXT("EpgData"));
#if 0
	if (::SHGetSpecialFolderPath(NULL,m_szEDCBDataFolder,CSIDL_PERSONAL,FALSE))
		::PathAppend(m_szEDCBDataFolder,TEXT("EpgTimerBon\\EpgData"));
	else
#endif
		m_szEDCBDataFolder[0]='\0';
	::lstrcpy(m_szLogoFileName,TEXT("LogoData"));

	DrawUtil::GetSystemFont(DrawUtil::FONT_MESSAGE,&m_EventInfoFont);
}


CEpgOptions::~CEpgOptions()
{
	Destroy();
	Finalize();
}


void CEpgOptions::Finalize()
{
	if (m_hLoadThread!=NULL) {
		if (::WaitForSingleObject(m_hLoadThread,0)==WAIT_TIMEOUT) {
			GetAppClass().AddLog(TEXT("EPGデータ読み込みスレッドの終了を待っています..."));
			if (::WaitForSingleObject(m_hLoadThread,30000)==WAIT_TIMEOUT) {
				GetAppClass().AddLog(TEXT("EPGデータ読み込みスレッドを強制終了します。"));
				::TerminateThread(m_hLoadThread,-1);
			}
		}
		::CloseHandle(m_hLoadThread);
		m_hLoadThread=NULL;
	}

	SAFE_DELETE(m_pEpgDataLoader);
}


bool CEpgOptions::ReadSettings(CSettings &Settings)
{
	Settings.Read(TEXT("SaveEpgData"),&m_fSaveEpgFile);
	Settings.Read(TEXT("EpgDataFileName"),m_szEpgFileName,lengthof(m_szEpgFileName));
	Settings.Read(TEXT("EpgUpdateWhenStandby"),&m_fUpdateWhenStandby);
	Settings.Read(TEXT("EpgUpdateBSExtended"),&m_fUpdateBSExtended);
	Settings.Read(TEXT("EpgUpdateCSExtended"),&m_fUpdateCSExtended);
	Settings.Read(TEXT("UseEpgData"),&m_fUseEDCBData);
	Settings.Read(TEXT("EpgDataFolder"),m_szEDCBDataFolder,lengthof(m_szEDCBDataFolder));

	Settings.Read(TEXT("SaveLogoData"),&m_fSaveLogoFile);
	Settings.Read(TEXT("LogoDataFileName"),m_szLogoFileName,lengthof(m_szLogoFileName));

	CLogoManager *pLogoManager=GetAppClass().GetLogoManager();
	bool fSaveLogo;
	if (Settings.Read(TEXT("SaveRawLogo"),&fSaveLogo))
		pLogoManager->SetSaveLogo(fSaveLogo);
	if (Settings.Read(TEXT("SaveBmpLogo"),&fSaveLogo))
		pLogoManager->SetSaveLogoBmp(fSaveLogo);
	TCHAR szLogoDir[MAX_PATH];
	if (Settings.Read(TEXT("LogoDirectory"),szLogoDir,MAX_PATH)) {
		pLogoManager->SetLogoDirectory(szLogoDir);
	} else {
		// TVLogoMark のロゴがあれば利用する
		GetAppClass().GetAppDirectory(szLogoDir);
		::PathAppend(szLogoDir,TEXT("Plugins\\Logo"));
		if (::PathIsDirectory(szLogoDir))
			pLogoManager->SetLogoDirectory(TEXT(".\\Plugins\\Logo"));
	}

	Settings.Read(TEXT("EventInfoFont"),&m_EventInfoFont);

	return true;
}


bool CEpgOptions::WriteSettings(CSettings &Settings)
{
	CLogoManager *pLogoManager=GetAppClass().GetLogoManager();

	Settings.Write(TEXT("SaveEpgData"),m_fSaveEpgFile);
	Settings.Write(TEXT("EpgDataFileName"),m_szEpgFileName);
	Settings.Write(TEXT("EpgUpdateWhenStandby"),m_fUpdateWhenStandby);
	Settings.Write(TEXT("EpgUpdateBSExtended"),m_fUpdateBSExtended);
	Settings.Write(TEXT("EpgUpdateCSExtended"),m_fUpdateCSExtended);
	Settings.Write(TEXT("UseEpgData"),m_fUseEDCBData);
	Settings.Write(TEXT("EpgDataFolder"),m_szEDCBDataFolder);

	Settings.Write(TEXT("SaveLogoData"),m_fSaveLogoFile);
	Settings.Write(TEXT("LogoDataFileName"),m_szLogoFileName);
	Settings.Write(TEXT("SaveRawLogo"),pLogoManager->GetSaveLogo());
	Settings.Write(TEXT("SaveBmpLogo"),pLogoManager->GetSaveLogoBmp());
	Settings.Write(TEXT("LogoDirectory"),pLogoManager->GetLogoDirectory());

	Settings.Write(TEXT("EventInfoFont"),&m_EventInfoFont);
	return true;
}


bool CEpgOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_EPG));
}


bool CEpgOptions::LoadEpgFile(CEpgProgramList *pEpgList)
{
	bool fOK=true;

	if (m_fSaveEpgFile) {
		TCHAR szFileName[MAX_PATH];

		if (!GetAbsolutePath(m_szEpgFileName,szFileName,lengthof(szFileName)))
			return false;
		if (::PathFileExists(szFileName)) {
			GetAppClass().AddLog(TEXT("EPG データを \"%s\" から読み込みます..."),szFileName);
			fOK=pEpgList->LoadFromFile(szFileName);
		}
	}
	return fOK;
}


struct EpgLoadInfo {
	CEpgProgramList *pList;
	CEpgOptions::CEpgFileLoadEventHandler *pEventHandler;
	TCHAR szFileName[MAX_PATH];
};

bool CEpgOptions::AsyncLoadEpgFile(CEpgProgramList *pEpgList,CEpgFileLoadEventHandler *pEventHandler)
{
	if (m_fSaveEpgFile) {
		TCHAR szFileName[MAX_PATH];

		if (!GetAbsolutePath(m_szEpgFileName,szFileName,lengthof(szFileName)))
			return false;
		if (::PathFileExists(szFileName)) {
			GetAppClass().AddLog(TEXT("EPG データを \"%s\" から読み込みます..."),szFileName);

			EpgLoadInfo *pInfo=new EpgLoadInfo;

			pInfo->pList=pEpgList;
			pInfo->pEventHandler=pEventHandler;
			::lstrcpy(pInfo->szFileName,szFileName);

			m_hLoadThread=(HANDLE)::_beginthreadex(NULL,0,EpgFileLoadThread,pInfo,0,NULL);
			if (m_hLoadThread==NULL) {
				delete pInfo;
				//return pEpgList->LoadFromFile(szFileName);
				return false;
			}
		}
	}

	return true;
}


unsigned int __stdcall CEpgOptions::EpgFileLoadThread(void *pParameter)
{
	EpgLoadInfo *pInfo=static_cast<EpgLoadInfo*>(pParameter);

	::SetThreadPriority(::GetCurrentThread(),THREAD_PRIORITY_LOWEST);
	if (pInfo->pEventHandler!=NULL)
		pInfo->pEventHandler->OnBeginLoad();
	bool fOK=pInfo->pList->LoadFromFile(pInfo->szFileName);
	if (pInfo->pEventHandler!=NULL)
		pInfo->pEventHandler->OnEndLoad(fOK);
	delete pInfo;
	return fOK;
}


bool CEpgOptions::IsEpgFileLoading() const
{
	return m_hLoadThread!=NULL
		&& ::WaitForSingleObject(m_hLoadThread,0)==WAIT_TIMEOUT;
}


bool CEpgOptions::WaitEpgFileLoad(DWORD Timeout)
{
	if (m_hLoadThread!=NULL) {
		if (::WaitForSingleObject(m_hLoadThread,Timeout)==WAIT_TIMEOUT)
			return false;
	}
	return true;
}


bool CEpgOptions::SaveEpgFile(CEpgProgramList *pEpgList)
{
	bool fOK=true;

	if (m_fSaveEpgFile) {
		TCHAR szFileName[MAX_PATH];

		if (!pEpgList->IsUpdated())
			return true;
		if (!GetAbsolutePath(m_szEpgFileName,szFileName,lengthof(szFileName)))
			return false;
		if (pEpgList->NumServices()>0) {
			GetAppClass().AddLog(TEXT("EPG データを \"%s\" に保存します..."),szFileName);
			fOK=pEpgList->SaveToFile(szFileName);
			if (!fOK)
				::DeleteFile(szFileName);
		} else {
			/*
			fOK=::DeleteFile(szFileName)
				|| ::GetLastError()==ERROR_FILE_NOT_FOUND;
			*/
		}
	}

	return fOK;
}


bool CEpgOptions::LoadEDCBData()
{
	bool fOK=true;

	if (m_fUseEDCBData && m_szEDCBDataFolder[0]!='\0') {
		CEpgDataLoader Loader;

		fOK=Loader.Load(m_szEDCBDataFolder);
	}
	return fOK;
}


bool CEpgOptions::AsyncLoadEDCBData(CEDCBDataLoadEventHandler *pEventHandler)
{
	bool fOK=true;

	if (m_fUseEDCBData && m_szEDCBDataFolder[0]!='\0') {
		delete m_pEpgDataLoader;
		m_pEpgDataLoader=new CEpgDataLoader;

		fOK=m_pEpgDataLoader->LoadAsync(m_szEDCBDataFolder,pEventHandler);
	}
	return fOK;
}


bool CEpgOptions::IsEDCBDataLoading() const
{
	return m_pEpgDataLoader!=NULL
		&& m_pEpgDataLoader->IsLoading();
}


bool CEpgOptions::WaitEDCBDataLoad(DWORD Timeout)
{
	return m_pEpgDataLoader==NULL
		|| m_pEpgDataLoader->Wait(Timeout);
}


bool CEpgOptions::LoadLogoFile()
{
	if (m_fSaveLogoFile && m_szLogoFileName[0]!='\0') {
		CAppMain &App=GetAppClass();
		CLogoManager *pLogoManager=App.GetLogoManager();
		TCHAR szFileName[MAX_PATH];

		if (!GetAbsolutePath(m_szLogoFileName,szFileName,lengthof(szFileName)))
			return false;
		if (::PathFileExists(szFileName)) {
			App.AddLog(TEXT("ロゴデータを \"%s\" から読み込みます..."),szFileName);
			if (!pLogoManager->LoadLogoFile(szFileName)) {
				App.AddLog(TEXT("ロゴファイルの読み込みでエラーが発生しました。"));
				return false;
			}
		}
		if (::lstrlen(szFileName)+4<MAX_PATH) {
			::lstrcat(szFileName,TEXT(".ini"));
			if (!::PathFileExists(szFileName)) {
				// 以前のバージョンとの互換用
				::GetModuleFileName(NULL,szFileName,lengthof(szFileName));
				::PathRenameExtension(szFileName,TEXT(".logo.ini"));
				if (!::PathFileExists(szFileName))
					return false;
			}
			App.AddLog(TEXT("ロゴ設定を \"%s\" から読み込みます..."),szFileName);
			pLogoManager->LoadLogoIDMap(szFileName);
		}
	}
	return true;
}


bool CEpgOptions::SaveLogoFile()
{
	if (m_fSaveLogoFile && m_szLogoFileName[0]!='\0') {
		CAppMain &App=GetAppClass();
		CLogoManager *pLogoManager=App.GetLogoManager();
		TCHAR szFileName[MAX_PATH];

		if (!GetAbsolutePath(m_szLogoFileName,szFileName,lengthof(szFileName)))
			return false;
		if (!::PathFileExists(szFileName) || pLogoManager->IsLogoDataUpdated()) {
			App.AddLog(TEXT("ロゴデータを \"%s\" に保存します..."),szFileName);
			if (!pLogoManager->SaveLogoFile(szFileName)) {
				App.AddLog(TEXT("ロゴファイルの保存でエラーが発生しました。"));
				return false;
			}
		}
		if (::lstrlen(szFileName)+4<MAX_PATH) {
			::lstrcat(szFileName,TEXT(".ini"));
			if (!::PathFileExists(szFileName) || pLogoManager->IsLogoIDMapUpdated()) {
				App.AddLog(TEXT("ロゴ設定を \"%s\" に保存します..."),szFileName);
				pLogoManager->SaveLogoIDMap(szFileName);
			}
		}
	}
	return true;
}


static void SetFontInfo(HWND hDlg,const LOGFONT *plf)
{
	HDC hdc;
	TCHAR szText[LF_FACESIZE+16];

	hdc=::GetDC(hDlg);
	if (hdc==NULL)
		return;
	::wsprintf(szText,TEXT("%s, %d pt"),plf->lfFaceName,CalcFontPointHeight(hdc,plf));
	::SetDlgItemText(hDlg,IDC_EVENTINFOOPTIONS_FONT_INFO,szText);
	::ReleaseDC(hDlg,hdc);
}

INT_PTR CEpgOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CLogoManager *pLogoManager=GetAppClass().GetLogoManager();

			DlgCheckBox_Check(hDlg,IDC_EPGOPTIONS_SAVEEPGFILE,m_fSaveEpgFile);
			::EnableDlgItems(hDlg,IDC_EPGOPTIONS_EPGFILENAME_LABEL,
					IDC_EPGOPTIONS_EPGFILENAME_BROWSE,m_fSaveEpgFile);
			::SendDlgItemMessage(hDlg,IDC_EPGOPTIONS_EPGFILENAME,
								 EM_LIMITTEXT,MAX_PATH-1,0);
			::SetDlgItemText(hDlg,IDC_EPGOPTIONS_EPGFILENAME,m_szEpgFileName);
			DlgCheckBox_Check(hDlg,IDC_EPGOPTIONS_UPDATEWHENSTANDBY,m_fUpdateWhenStandby);
			DlgCheckBox_Check(hDlg,IDC_EPGOPTIONS_UPDATEBSEXTENDED,m_fUpdateBSExtended);
			DlgCheckBox_Check(hDlg,IDC_EPGOPTIONS_UPDATECSEXTENDED,m_fUpdateCSExtended);
			DlgCheckBox_Check(hDlg,IDC_EPGOPTIONS_USEEPGDATA,m_fUseEDCBData);
			::SendDlgItemMessage(hDlg,IDC_EPGOPTIONS_EPGDATAFOLDER,EM_LIMITTEXT,MAX_PATH-1,0);
			::SetDlgItemText(hDlg,IDC_EPGOPTIONS_EPGDATAFOLDER,m_szEDCBDataFolder);
			EnableDlgItems(hDlg,IDC_EPGOPTIONS_EPGDATAFOLDER_LABEL,
								IDC_EPGOPTIONS_EPGDATAFOLDER_BROWSE,
						   m_fUseEDCBData);

			DlgCheckBox_Check(hDlg,IDC_LOGOOPTIONS_SAVEDATA,m_fSaveLogoFile);
			::SendDlgItemMessage(hDlg,IDC_LOGOOPTIONS_DATAFILENAME,EM_LIMITTEXT,MAX_PATH-1,0);
			::SetDlgItemText(hDlg,IDC_LOGOOPTIONS_DATAFILENAME,m_szLogoFileName);
			EnableDlgItems(hDlg,IDC_LOGOOPTIONS_DATAFILENAME_LABEL,
								IDC_LOGOOPTIONS_DATAFILENAME_BROWSE,
						   m_fSaveLogoFile);
			DlgCheckBox_Check(hDlg,IDC_LOGOOPTIONS_SAVERAWLOGO,pLogoManager->GetSaveLogo());
			DlgCheckBox_Check(hDlg,IDC_LOGOOPTIONS_SAVEBMPLOGO,pLogoManager->GetSaveLogoBmp());
			::SendDlgItemMessage(hDlg,IDC_LOGOOPTIONS_LOGOFOLDER,EM_LIMITTEXT,MAX_PATH-1,0);
			::SetDlgItemText(hDlg,IDC_LOGOOPTIONS_LOGOFOLDER,pLogoManager->GetLogoDirectory());

			m_CurEventInfoFont=m_EventInfoFont;
			SetFontInfo(hDlg,&m_CurEventInfoFont);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_EPGOPTIONS_SAVEEPGFILE:
			EnableDlgItemsSyncCheckBox(hDlg,
									   IDC_EPGOPTIONS_EPGFILENAME_LABEL,
									   IDC_EPGOPTIONS_EPGFILENAME_BROWSE,
									   IDC_EPGOPTIONS_SAVEEPGFILE);
			return TRUE;

		case IDC_EPGOPTIONS_EPGFILENAME_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH],szInitialDir[MAX_PATH];

				::GetDlgItemText(hDlg,IDC_EPGOPTIONS_EPGFILENAME,
								 szFileName,lengthof(szFileName));
				ofn.lStructSize=sizeof(OPENFILENAME);
				ofn.hwndOwner=hDlg;
				ofn.lpstrFilter=TEXT("すべてのファイル\0*.*\0");
				ofn.lpstrCustomFilter=NULL;
				ofn.nFilterIndex=1;
				ofn.lpstrFile=szFileName;
				ofn.nMaxFile=lengthof(szFileName);
				ofn.lpstrFileTitle=NULL;
				if (szFileName[0]=='\0' || ::PathIsFileSpec(szFileName)) {
					::GetModuleFileName(NULL,szInitialDir,lengthof(szInitialDir));
					::PathRemoveFileSpec(szInitialDir);
					ofn.lpstrInitialDir=szInitialDir;
				} else
					ofn.lpstrInitialDir=NULL;
				ofn.lpstrTitle=TEXT("EPGファイル名");
				ofn.Flags=OFN_EXPLORER | OFN_HIDEREADONLY;
				ofn.lpstrDefExt=NULL;
#if _WIN32_WINNT>=0x500
				ofn.pvReserved=NULL;
				ofn.dwReserved=0;
				ofn.FlagsEx=0;
#endif
				if (::GetOpenFileName(&ofn))
					::SetDlgItemText(hDlg,IDC_EPGOPTIONS_EPGFILENAME,szFileName);
			}
			return TRUE;

		case IDC_EPGOPTIONS_USEEPGDATA:
			EnableDlgItemsSyncCheckBox(hDlg,
									   IDC_EPGOPTIONS_EPGDATAFOLDER_LABEL,
									   IDC_EPGOPTIONS_EPGDATAFOLDER_BROWSE,
									   IDC_EPGOPTIONS_USEEPGDATA);
			return TRUE;

		case IDC_EPGOPTIONS_EPGDATAFOLDER_BROWSE:
			{
				TCHAR szFolder[MAX_PATH];

				::GetDlgItemText(hDlg,IDC_EPGOPTIONS_EPGDATAFOLDER,szFolder,lengthof(szFolder));
				if (BrowseFolderDialog(hDlg,szFolder,TEXT("EPGデータのフォルダ")))
					::SetDlgItemText(hDlg,IDC_EPGOPTIONS_EPGDATAFOLDER,szFolder);
			}
			return TRUE;

		case IDC_LOGOOPTIONS_SAVEDATA:
			EnableDlgItemsSyncCheckBox(hDlg,
									   IDC_LOGOOPTIONS_DATAFILENAME_LABEL,
									   IDC_LOGOOPTIONS_DATAFILENAME_BROWSE,
									   IDC_LOGOOPTIONS_SAVEDATA);
			return TRUE;

		case IDC_LOGOOPTIONS_DATAFILENAME_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH],szInitialDir[MAX_PATH];

				::GetDlgItemText(hDlg,IDC_LOGOOPTIONS_DATAFILENAME,szFileName,lengthof(szFileName));
				ofn.lStructSize=sizeof(OPENFILENAME);
				ofn.hwndOwner=hDlg;
				ofn.lpstrFilter=TEXT("すべてのファイル\0*.*\0");
				ofn.lpstrCustomFilter=NULL;
				ofn.nFilterIndex=1;
				ofn.lpstrFile=szFileName;
				ofn.nMaxFile=lengthof(szFileName);
				ofn.lpstrFileTitle=NULL;
				if (szFileName[0]=='\0' || ::PathIsFileSpec(szFileName)) {
					::GetModuleFileName(NULL,szInitialDir,lengthof(szInitialDir));
					::PathRemoveFileSpec(szInitialDir);
					ofn.lpstrInitialDir=szInitialDir;
				} else
					ofn.lpstrInitialDir=NULL;
				ofn.lpstrTitle=TEXT("ロゴファイル名");
				ofn.Flags=OFN_EXPLORER | OFN_HIDEREADONLY;
				ofn.lpstrDefExt=NULL;
#if _WIN32_WINNT>=0x500
				ofn.pvReserved=NULL;
				ofn.dwReserved=0;
				ofn.FlagsEx=0;
#endif
				if (::GetOpenFileName(&ofn))
					::SetDlgItemText(hDlg,IDC_LOGOOPTIONS_DATAFILENAME,szFileName);
			}
			return TRUE;

		case IDC_LOGOOPTIONS_LOGOFOLDER_BROWSE:
			{
				TCHAR szFolder[MAX_PATH];

				::GetDlgItemText(hDlg,IDC_LOGOOPTIONS_LOGOFOLDER,szFolder,lengthof(szFolder));
				if (BrowseFolderDialog(hDlg,szFolder,TEXT("ロゴのフォルダ")))
					::SetDlgItemText(hDlg,IDC_LOGOOPTIONS_LOGOFOLDER,szFolder);
			}
			return TRUE;

		case IDC_EVENTINFOOPTIONS_FONT_CHOOSE:
			if (ChooseFontDialog(hDlg,&m_CurEventInfoFont))
				SetFontInfo(hDlg,&m_CurEventInfoFont);
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CLogoManager *pLogoManager=GetAppClass().GetLogoManager();
				TCHAR szPath[MAX_PATH];

				m_fSaveEpgFile=
					DlgCheckBox_IsChecked(hDlg,IDC_EPGOPTIONS_SAVEEPGFILE);
				::GetDlgItemText(hDlg,IDC_EPGOPTIONS_EPGFILENAME,
								 m_szEpgFileName,lengthof(m_szEpgFileName));
				m_fUpdateWhenStandby=
					DlgCheckBox_IsChecked(hDlg,IDC_EPGOPTIONS_UPDATEWHENSTANDBY);
				m_fUpdateBSExtended=
					DlgCheckBox_IsChecked(hDlg,IDC_EPGOPTIONS_UPDATEBSEXTENDED);
				m_fUpdateCSExtended=
					DlgCheckBox_IsChecked(hDlg,IDC_EPGOPTIONS_UPDATECSEXTENDED);
				bool fUseEpgData=
					DlgCheckBox_IsChecked(hDlg,IDC_EPGOPTIONS_USEEPGDATA);
				::GetDlgItemText(hDlg,IDC_EPGOPTIONS_EPGDATAFOLDER,
					m_szEDCBDataFolder,lengthof(m_szEDCBDataFolder));
				if (!m_fUseEDCBData && fUseEpgData) {
					m_fUseEDCBData=fUseEpgData;
					AsyncLoadEDCBData();
				}
				m_fUseEDCBData=fUseEpgData;

				m_fSaveLogoFile=DlgCheckBox_IsChecked(hDlg,IDC_LOGOOPTIONS_SAVEDATA);
				::GetDlgItemText(hDlg,IDC_LOGOOPTIONS_DATAFILENAME,
								 m_szLogoFileName,lengthof(m_szLogoFileName));
				pLogoManager->SetSaveLogo(DlgCheckBox_IsChecked(hDlg,IDC_LOGOOPTIONS_SAVERAWLOGO));
				pLogoManager->SetSaveLogoBmp(DlgCheckBox_IsChecked(hDlg,IDC_LOGOOPTIONS_SAVEBMPLOGO));
				::GetDlgItemText(hDlg,IDC_LOGOOPTIONS_LOGOFOLDER,szPath,lengthof(szPath));
				pLogoManager->SetLogoDirectory(szPath);

				if (!CompareLogFont(&m_EventInfoFont,&m_CurEventInfoFont)) {
					m_EventInfoFont=m_CurEventInfoFont;
					SetGeneralUpdateFlag(UPDATE_GENERAL_EVENTINFOFONT);
				}

				m_fChanged=true;
			}
			break;
		}
		break;
	}

	return FALSE;
}
