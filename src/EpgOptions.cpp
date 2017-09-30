#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "EpgOptions.h"
#include "DialogUtil.h"
#include "DrawUtil.h"
#include "StyleUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"




CEpgOptions::CEpgOptions()
	: m_fSaveEpgFile(true)
	, m_fUpdateWhenStandby(false)
	, m_fUpdateBSExtended(false)
	, m_fUpdateCSExtended(false)
	, m_fUseEDCBData(false)
	, m_EpgTimeMode(EPGTIME_JST)
	, m_fSaveLogoFile(true)

	, m_pEpgDataLoader(NULL)
{
	::lstrcpy(m_szEpgFileName,TEXT("EpgData"));

	m_szEDCBDataFolder[0]='\0';
#if 0
	static const TCHAR szEpgDataFolder[]=TEXT("EpgTimerBon\\EpgData");
#ifdef WIN_XP_SUPPORT
	TCHAR szPath[MAX_PATH];
	if (::SHGetSpecialFolderPath(NULL,szPath,CSIDL_PERSONAL,FALSE)
			&& ::lstrlen(szPath)+lengthof(szEpgDataFolder)<lengthof(m_szEDCBDataFolder)) {
		::PathAppend(m_szEDCBDataFolder,szEpgDataFolder);
	}
#else
	PWSTR pszPath;
	if (::SHGetKnownFolderPath(FOLDERID_Documents,0,NULL,&pszPath)==S_OK) {
		if (::lstrlen(pszPath)+lengthof(szEpgDataFolder)<lengthof(m_szEDCBDataFolder)) {
			::lstrcpy(m_szEDCBDataFolder,pszPath);
			::PathAppend(m_szEDCBDataFolder,szEpgDataFolder);
		}
		::CoTaskMemFree(pszPath);
	}
#endif
#endif

	::lstrcpy(m_szLogoFileName,TEXT("LogoData"));

	TVTest::StyleUtil::GetSystemFont(DrawUtil::FONT_MESSAGE,&m_EventInfoFont);
}


CEpgOptions::~CEpgOptions()
{
	Destroy();
	Finalize();
}


void CEpgOptions::Finalize()
{
	m_EpgDataStore.Close();

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

	int Value;
	if (Settings.Read(TEXT("EpgTimeMode"),&Value)
			&& Value>=EPGTIME_FIRST && Value<=EPGTIME_LAST)
		m_EpgTimeMode=(EpgTimeMode)Value;

	Settings.Read(TEXT("SaveLogoData"),&m_fSaveLogoFile);
	Settings.Read(TEXT("LogoDataFileName"),m_szLogoFileName,lengthof(m_szLogoFileName));

	CLogoManager &LogoManager=GetAppClass().LogoManager;
	bool fSaveLogo;
	if (Settings.Read(TEXT("SaveRawLogo"),&fSaveLogo))
		LogoManager.SetSaveLogo(fSaveLogo);
	if (Settings.Read(TEXT("SaveBmpLogo"),&fSaveLogo))
		LogoManager.SetSaveLogoBmp(fSaveLogo);
	TCHAR szLogoDir[MAX_PATH];
	if (Settings.Read(TEXT("LogoDirectory"),szLogoDir,MAX_PATH)) {
		LogoManager.SetLogoDirectory(szLogoDir);
	} else {
		// TVLogoMark のロゴがあれば利用する
		GetAppClass().GetAppDirectory(szLogoDir);
		::PathAppend(szLogoDir,TEXT("Plugins\\Logo"));
		if (::PathIsDirectory(szLogoDir))
			LogoManager.SetLogoDirectory(TEXT(".\\Plugins\\Logo"));
	}

	bool f;
	if (TVTest::StyleUtil::ReadFontSettings(Settings,TEXT("EventInfoFont"),&m_EventInfoFont,false,&f)) {
		if (!f)
			m_fChanged=true;
	}

	return true;
}


bool CEpgOptions::WriteSettings(CSettings &Settings)
{
	CLogoManager &LogoManager=GetAppClass().LogoManager;

	Settings.Write(TEXT("SaveEpgData"),m_fSaveEpgFile);
	Settings.Write(TEXT("EpgDataFileName"),m_szEpgFileName);
	Settings.Write(TEXT("EpgUpdateWhenStandby"),m_fUpdateWhenStandby);
	Settings.Write(TEXT("EpgUpdateBSExtended"),m_fUpdateBSExtended);
	Settings.Write(TEXT("EpgUpdateCSExtended"),m_fUpdateCSExtended);
	Settings.Write(TEXT("UseEpgData"),m_fUseEDCBData);
	Settings.Write(TEXT("EpgDataFolder"),m_szEDCBDataFolder);
	Settings.Write(TEXT("EpgTimeMode"),(int)m_EpgTimeMode);

	Settings.Write(TEXT("SaveLogoData"),m_fSaveLogoFile);
	Settings.Write(TEXT("LogoDataFileName"),m_szLogoFileName);
	Settings.Write(TEXT("SaveRawLogo"),LogoManager.GetSaveLogo());
	Settings.Write(TEXT("SaveBmpLogo"),LogoManager.GetSaveLogoBmp());
	Settings.Write(TEXT("LogoDirectory"),LogoManager.GetLogoDirectory());

	TVTest::StyleUtil::WriteFontSettings(Settings,TEXT("EventInfoFont"),m_EventInfoFont);

	return true;
}


bool CEpgOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_EPG));
}


bool CEpgOptions::LoadEpgFile(LibISDB::EPGDatabase *pEPGDatabase)
{
	if (m_fSaveEpgFile) {
		TCHAR szFileName[MAX_PATH];

		if (!GetAbsolutePath(m_szEpgFileName,szFileName,lengthof(szFileName)))
			return false;
		if (::PathFileExists(szFileName)) {
			GetAppClass().AddLog(TEXT("EPG データを \"%s\" から読み込みます..."),szFileName);
			if (!m_EpgDataStore.Open(pEPGDatabase,szFileName))
				return false;
			if (!m_EpgDataStore.Load())
				return false;
		}
	}
	return true;
}


bool CEpgOptions::AsyncLoadEpgFile(
	LibISDB::EPGDatabase *pEPGDatabase,TVTest::CEpgDataStore::CEventHandler *pEventHandler)
{
	if (m_fSaveEpgFile) {
		TCHAR szFileName[MAX_PATH];

		if (!GetAbsolutePath(m_szEpgFileName,szFileName,lengthof(szFileName)))
			return false;
		if (::PathFileExists(szFileName)) {
			GetAppClass().AddLog(TEXT("EPG データを \"%s\" から読み込みます..."),szFileName);
			m_EpgDataStore.SetEventHandler(pEventHandler);
			if (!m_EpgDataStore.Open(pEPGDatabase,szFileName,TVTest::CEpgDataStore::OpenFlag::LoadBackground))
				return false;
			if (!m_EpgDataStore.LoadAsync())
				return false;
		}
	}

	return true;
}


bool CEpgOptions::IsEpgFileLoading() const
{
	return m_EpgDataStore.IsLoading();
}


bool CEpgOptions::WaitEpgFileLoad(DWORD Timeout)
{
	return m_EpgDataStore.Wait(Timeout);
}


bool CEpgOptions::SaveEpgFile(LibISDB::EPGDatabase *pEPGDatabase)
{
	bool fOK=true;

	if (m_fSaveEpgFile) {
		TCHAR szFileName[MAX_PATH];

		if (!pEPGDatabase->IsUpdated())
			return true;
		if (!GetAbsolutePath(m_szEpgFileName,szFileName,lengthof(szFileName)))
			return false;
		if (pEPGDatabase->GetServiceCount()>0) {
			GetAppClass().AddLog(TEXT("EPG データを \"%s\" に保存します..."),szFileName);
			if (m_EpgDataStore.Open(pEPGDatabase,szFileName)) {
				if (!m_EpgDataStore.Save()) {
					::DeleteFile(szFileName);
					fOK=false;
				}
			} else {
				fOK=false;
			}
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
		TCHAR szPath[MAX_PATH];

		if (!GetAbsolutePath(m_szEDCBDataFolder,szPath,lengthof(szPath)))
			return false;

		CEpgDataLoader Loader;

		fOK=Loader.Load(szPath);
	}
	return fOK;
}


bool CEpgOptions::AsyncLoadEDCBData(CEDCBDataLoadEventHandler *pEventHandler)
{
	bool fOK=true;

	if (m_fUseEDCBData && m_szEDCBDataFolder[0]!='\0') {
		TCHAR szPath[MAX_PATH];

		if (!GetAbsolutePath(m_szEDCBDataFolder,szPath,lengthof(szPath)))
			return false;

		delete m_pEpgDataLoader;
		m_pEpgDataLoader=new CEpgDataLoader;

		fOK=m_pEpgDataLoader->LoadAsync(szPath,pEventHandler);
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
		CLogoManager &LogoManager=App.LogoManager;
		TCHAR szFileName[MAX_PATH];

		if (!GetAbsolutePath(m_szLogoFileName,szFileName,lengthof(szFileName)))
			return false;
		if (::PathFileExists(szFileName)) {
			App.AddLog(TEXT("ロゴデータを \"%s\" から読み込みます..."),szFileName);
			if (!LogoManager.LoadLogoFile(szFileName)) {
				App.AddLog(CLogItem::TYPE_ERROR,TEXT("ロゴファイルの読み込みでエラーが発生しました。"));
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
			LogoManager.LoadLogoIDMap(szFileName);
		}
	}
	return true;
}


bool CEpgOptions::SaveLogoFile()
{
	if (m_fSaveLogoFile && m_szLogoFileName[0]!='\0') {
		CAppMain &App=GetAppClass();
		CLogoManager &LogoManager=App.LogoManager;
		TCHAR szFileName[MAX_PATH];

		if (!GetAbsolutePath(m_szLogoFileName,szFileName,lengthof(szFileName)))
			return false;
		if (!::PathFileExists(szFileName) || LogoManager.IsLogoDataUpdated()) {
			App.AddLog(TEXT("ロゴデータを \"%s\" に保存します..."),szFileName);
			if (!LogoManager.SaveLogoFile(szFileName)) {
				App.AddLog(CLogItem::TYPE_ERROR,TEXT("ロゴファイルの保存でエラーが発生しました。"));
				return false;
			}
		}
		if (::lstrlen(szFileName)+4<MAX_PATH) {
			::lstrcat(szFileName,TEXT(".ini"));
			if (!::PathFileExists(szFileName) || LogoManager.IsLogoIDMapUpdated()) {
				App.AddLog(TEXT("ロゴ設定を \"%s\" に保存します..."),szFileName);
				LogoManager.SaveLogoIDMap(szFileName);
			}
		}
	}
	return true;
}


INT_PTR CEpgOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CLogoManager &LogoManager=GetAppClass().LogoManager;

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
			DlgCheckBox_Check(hDlg,IDC_LOGOOPTIONS_SAVERAWLOGO,LogoManager.GetSaveLogo());
			DlgCheckBox_Check(hDlg,IDC_LOGOOPTIONS_SAVEBMPLOGO,LogoManager.GetSaveLogoBmp());
			::SendDlgItemMessage(hDlg,IDC_LOGOOPTIONS_LOGOFOLDER,EM_LIMITTEXT,MAX_PATH-1,0);
			::SetDlgItemText(hDlg,IDC_LOGOOPTIONS_LOGOFOLDER,LogoManager.GetLogoDirectory());

			m_CurEventInfoFont=m_EventInfoFont;
			TVTest::StyleUtil::SetFontInfoItem(hDlg,IDC_EVENTINFOOPTIONS_FONT_INFO,m_CurEventInfoFont);
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
				if (FileOpenDialog(&ofn))
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
				if (FileOpenDialog(&ofn))
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
			if (TVTest::StyleUtil::ChooseStyleFont(hDlg,&m_CurEventInfoFont))
				TVTest::StyleUtil::SetFontInfoItem(hDlg,IDC_EVENTINFOOPTIONS_FONT_INFO,m_CurEventInfoFont);
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CLogoManager &LogoManager=GetAppClass().LogoManager;
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
					AsyncLoadEDCBData(&GetAppClass().EpgLoadEventHandler);
				}
				m_fUseEDCBData=fUseEpgData;

				m_fSaveLogoFile=DlgCheckBox_IsChecked(hDlg,IDC_LOGOOPTIONS_SAVEDATA);
				::GetDlgItemText(hDlg,IDC_LOGOOPTIONS_DATAFILENAME,
								 m_szLogoFileName,lengthof(m_szLogoFileName));
				LogoManager.SetSaveLogo(DlgCheckBox_IsChecked(hDlg,IDC_LOGOOPTIONS_SAVERAWLOGO));
				LogoManager.SetSaveLogoBmp(DlgCheckBox_IsChecked(hDlg,IDC_LOGOOPTIONS_SAVEBMPLOGO));
				::GetDlgItemText(hDlg,IDC_LOGOOPTIONS_LOGOFOLDER,szPath,lengthof(szPath));
				LogoManager.SetLogoDirectory(szPath);

				if (m_EventInfoFont!=m_CurEventInfoFont) {
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
