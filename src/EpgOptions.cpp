/*
  TVTest
  Copyright(c) 2008-2017 DBCTRADO

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
#include "TVTest.h"
#include "AppMain.h"
#include "EpgOptions.h"
#include "DialogUtil.h"
#include "DrawUtil.h"
#include "StyleUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CEpgOptions::CEpgOptions()
	: m_fSaveEpgFile(true)
	, m_EpgFileName(TEXT("EpgData"))
	, m_fUpdateWhenStandby(false)
	, m_fUpdateBSExtended(false)
	, m_fUpdateCSExtended(false)
	, m_fUseEDCBData(false)
	, m_EpgTimeMode(EpgTimeMode::JST)
	, m_fSaveLogoFile(true)
	, m_LogoFileName(TEXT("LogoData"))
{
#if 0
	static const TCHAR szEpgDataFolder[] = TEXT("EpgTimerBon\\EpgData");
	PWSTR pszPath;
	if (::SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &pszPath) == S_OK) {
		m_EDCBDataFolder = pszPath;
		m_EDCBDataFolder.Append(szEpgDataFolder);
		::CoTaskMemFree(pszPath);
	}
#endif

	StyleUtil::GetSystemFont(DrawUtil::FontType::Message, &m_EventInfoFont);
}


CEpgOptions::~CEpgOptions()
{
	Destroy();
	Finalize();
}


void CEpgOptions::Finalize()
{
	m_EpgDataStore.Close();

	m_EpgDataLoader.reset();
}


bool CEpgOptions::ReadSettings(CSettings &Settings)
{
	Settings.Read(TEXT("SaveEpgData"), &m_fSaveEpgFile);
	Settings.Read(TEXT("EpgDataFileName"), &m_EpgFileName);
	Settings.Read(TEXT("EpgUpdateWhenStandby"), &m_fUpdateWhenStandby);
	Settings.Read(TEXT("EpgUpdateBSExtended"), &m_fUpdateBSExtended);
	Settings.Read(TEXT("EpgUpdateCSExtended"), &m_fUpdateCSExtended);
	Settings.Read(TEXT("UseEpgData"), &m_fUseEDCBData);
	Settings.Read(TEXT("EpgDataFolder"), &m_EDCBDataFolder);

	int Value;
	if (Settings.Read(TEXT("EpgTimeMode"), &Value)
			&& CheckEnumRange(static_cast<EpgTimeMode>(Value)))
		m_EpgTimeMode = static_cast<EpgTimeMode>(Value);

	Settings.Read(TEXT("SaveLogoData"), &m_fSaveLogoFile);
	Settings.Read(TEXT("LogoDataFileName"), &m_LogoFileName);

	CLogoManager &LogoManager = GetAppClass().LogoManager;
	bool fSaveLogo;
	if (Settings.Read(TEXT("SaveRawLogo"), &fSaveLogo))
		LogoManager.SetSaveLogo(fSaveLogo);
	if (Settings.Read(TEXT("SaveBmpLogo"), &fSaveLogo))
		LogoManager.SetSaveLogoBmp(fSaveLogo);
	TCHAR szLogoDir[MAX_PATH];
	if (Settings.Read(TEXT("LogoDirectory"), szLogoDir, MAX_PATH)) {
		LogoManager.SetLogoDirectory(szLogoDir);
	} else {
		// TVLogoMark のロゴがあれば利用する
		GetAppClass().GetAppDirectory(szLogoDir);
		::PathAppend(szLogoDir, TEXT("Plugins\\Logo"));
		if (::PathIsDirectory(szLogoDir))
			LogoManager.SetLogoDirectory(TEXT(".\\Plugins\\Logo"));
	}

	bool f;
	if (StyleUtil::ReadFontSettings(Settings, TEXT("EventInfoFont"), &m_EventInfoFont, false, &f)) {
		if (!f)
			m_fChanged = true;
	}

	return true;
}


bool CEpgOptions::WriteSettings(CSettings &Settings)
{
	CLogoManager &LogoManager = GetAppClass().LogoManager;

	Settings.Write(TEXT("SaveEpgData"), m_fSaveEpgFile);
	Settings.Write(TEXT("EpgDataFileName"), m_EpgFileName);
	Settings.Write(TEXT("EpgUpdateWhenStandby"), m_fUpdateWhenStandby);
	Settings.Write(TEXT("EpgUpdateBSExtended"), m_fUpdateBSExtended);
	Settings.Write(TEXT("EpgUpdateCSExtended"), m_fUpdateCSExtended);
	Settings.Write(TEXT("UseEpgData"), m_fUseEDCBData);
	Settings.Write(TEXT("EpgDataFolder"), m_EDCBDataFolder);
	Settings.Write(TEXT("EpgTimeMode"), (int)m_EpgTimeMode);

	Settings.Write(TEXT("SaveLogoData"), m_fSaveLogoFile);
	Settings.Write(TEXT("LogoDataFileName"), m_LogoFileName);
	Settings.Write(TEXT("SaveRawLogo"), LogoManager.GetSaveLogo());
	Settings.Write(TEXT("SaveBmpLogo"), LogoManager.GetSaveLogoBmp());
	Settings.Write(TEXT("LogoDirectory"), LogoManager.GetLogoDirectory());

	StyleUtil::WriteFontSettings(Settings, TEXT("EventInfoFont"), m_EventInfoFont);

	return true;
}


bool CEpgOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(
		hwndOwner,
		GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_OPTIONS_EPG));
}


bool CEpgOptions::LoadEpgFile(LibISDB::EPGDatabase *pEPGDatabase)
{
	if (m_fSaveEpgFile) {
		CFilePath FilePath;

		if (!GetAbsolutePath(m_EpgFileName, &FilePath))
			return false;
		if (FilePath.IsFileExists()) {
			GetAppClass().AddLog(TEXT("EPG データを \"%s\" から読み込みます..."), FilePath.c_str());
			if (!m_EpgDataStore.Open(pEPGDatabase, FilePath.c_str()))
				return false;
			if (!m_EpgDataStore.Load())
				return false;
		}
	}
	return true;
}


bool CEpgOptions::AsyncLoadEpgFile(
	LibISDB::EPGDatabase *pEPGDatabase, CEpgDataStore::CEventHandler *pEventHandler)
{
	if (m_fSaveEpgFile) {
		CFilePath FilePath;

		if (!GetAbsolutePath(m_EpgFileName, &FilePath))
			return false;
		if (FilePath.IsFileExists()) {
			GetAppClass().AddLog(TEXT("EPG データを \"%s\" から読み込みます..."), FilePath.c_str());
			m_EpgDataStore.SetEventHandler(pEventHandler);
			if (!m_EpgDataStore.Open(pEPGDatabase, FilePath.c_str(), CEpgDataStore::OpenFlag::LoadBackground))
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
	bool fOK = true;

	if (m_fSaveEpgFile) {
		String FilePath;

		if (!pEPGDatabase->IsUpdated())
			return true;
		if (!GetAbsolutePath(m_EpgFileName, &FilePath))
			return false;
		if (pEPGDatabase->GetServiceCount() > 0) {
			GetAppClass().AddLog(TEXT("EPG データを \"%s\" に保存します..."), FilePath.c_str());
			if (m_EpgDataStore.Open(pEPGDatabase, FilePath.c_str())) {
				if (!m_EpgDataStore.Save()) {
					::DeleteFile(FilePath.c_str());
					fOK = false;
				}
			} else {
				fOK = false;
			}
		} else {
			/*
			fOK = ::DeleteFile(FilePath.c_str())
				|| ::GetLastError() == ERROR_FILE_NOT_FOUND;
			*/
		}
	}

	return fOK;
}


bool CEpgOptions::LoadEDCBData()
{
	bool fOK = true;

	if (m_fUseEDCBData && !m_EDCBDataFolder.empty()) {
		String Path;

		if (!GetAbsolutePath(m_EDCBDataFolder, &Path))
			return false;

		CEpgDataLoader Loader;

		fOK = Loader.Load(Path.c_str());
	}
	return fOK;
}


bool CEpgOptions::AsyncLoadEDCBData(CEDCBDataLoadEventHandler *pEventHandler)
{
	bool fOK = true;

	if (m_fUseEDCBData && !m_EDCBDataFolder.empty()) {
		String Path;

		if (!GetAbsolutePath(m_EDCBDataFolder, &Path))
			return false;

		m_EpgDataLoader = std::make_unique<CEpgDataLoader>();

		fOK = m_EpgDataLoader->LoadAsync(Path.c_str(), pEventHandler);
	}
	return fOK;
}


bool CEpgOptions::IsEDCBDataLoading() const
{
	return m_EpgDataLoader
		&& m_EpgDataLoader->IsLoading();
}


bool CEpgOptions::WaitEDCBDataLoad(DWORD Timeout)
{
	return !m_EpgDataLoader
		|| m_EpgDataLoader->Wait(Timeout);
}


bool CEpgOptions::LoadLogoFile()
{
	if (m_fSaveLogoFile && !m_LogoFileName.empty()) {
		CAppMain &App = GetAppClass();
		CLogoManager &LogoManager = App.LogoManager;
		CFilePath FilePath;

		if (!GetAbsolutePath(m_LogoFileName, &FilePath))
			return false;
		if (FilePath.IsFileExists()) {
			App.AddLog(TEXT("ロゴデータを \"%s\" から読み込みます..."), FilePath.c_str());
			if (!LogoManager.LoadLogoFile(FilePath.c_str())) {
				App.AddLog(CLogItem::LogType::Error, TEXT("ロゴファイルの読み込みでエラーが発生しました。"));
				return false;
			}
		}

		FilePath += TEXT(".ini");
		if (!FilePath.IsFileExists()) {
			// 以前のバージョンとの互換用
			App.GetAppFilePath(&FilePath);
			FilePath.RenameExtension(TEXT(".logo.ini"));
			if (!FilePath.IsFileExists())
				return false;
		}
		App.AddLog(TEXT("ロゴ設定を \"%s\" から読み込みます..."), FilePath.c_str());
		LogoManager.LoadLogoIDMap(FilePath.c_str());
	}

	return true;
}


bool CEpgOptions::SaveLogoFile()
{
	if (m_fSaveLogoFile && !m_LogoFileName.empty()) {
		CAppMain &App = GetAppClass();
		CLogoManager &LogoManager = App.LogoManager;
		CFilePath FilePath;

		if (!GetAbsolutePath(m_LogoFileName, &FilePath))
			return false;
		if (!FilePath.IsFileExists() || LogoManager.IsLogoDataUpdated()) {
			App.AddLog(TEXT("ロゴデータを \"%s\" に保存します..."), FilePath.c_str());
			if (!LogoManager.SaveLogoFile(FilePath.c_str())) {
				App.AddLog(CLogItem::LogType::Error, TEXT("ロゴファイルの保存でエラーが発生しました。"));
				return false;
			}
		}

		FilePath += TEXT(".ini");
		if (!FilePath.IsFileExists() || LogoManager.IsLogoIDMapUpdated()) {
			App.AddLog(TEXT("ロゴ設定を \"%s\" に保存します..."), FilePath.c_str());
			LogoManager.SaveLogoIDMap(FilePath.c_str());
		}
	}

	return true;
}


INT_PTR CEpgOptions::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CLogoManager &LogoManager = GetAppClass().LogoManager;

			DlgCheckBox_Check(hDlg, IDC_EPGOPTIONS_SAVEEPGFILE, m_fSaveEpgFile);
			EnableDlgItems(
				hDlg, IDC_EPGOPTIONS_EPGFILENAME_LABEL,
				IDC_EPGOPTIONS_EPGFILENAME_BROWSE, m_fSaveEpgFile);
			::SendDlgItemMessage(
				hDlg, IDC_EPGOPTIONS_EPGFILENAME,
				EM_LIMITTEXT, MAX_PATH - 1, 0);
			::SetDlgItemText(hDlg, IDC_EPGOPTIONS_EPGFILENAME, m_EpgFileName.c_str());
			DlgCheckBox_Check(hDlg, IDC_EPGOPTIONS_UPDATEWHENSTANDBY, m_fUpdateWhenStandby);
			DlgCheckBox_Check(hDlg, IDC_EPGOPTIONS_UPDATEBSEXTENDED, m_fUpdateBSExtended);
			DlgCheckBox_Check(hDlg, IDC_EPGOPTIONS_UPDATECSEXTENDED, m_fUpdateCSExtended);
			DlgCheckBox_Check(hDlg, IDC_EPGOPTIONS_USEEPGDATA, m_fUseEDCBData);
			::SendDlgItemMessage(hDlg, IDC_EPGOPTIONS_EPGDATAFOLDER, EM_LIMITTEXT, MAX_PATH - 1, 0);
			::SetDlgItemText(hDlg, IDC_EPGOPTIONS_EPGDATAFOLDER, m_EDCBDataFolder.c_str());
			EnableDlgItems(
				hDlg,
				IDC_EPGOPTIONS_EPGDATAFOLDER_LABEL,
				IDC_EPGOPTIONS_EPGDATAFOLDER_BROWSE,
				m_fUseEDCBData);

			DlgCheckBox_Check(hDlg, IDC_LOGOOPTIONS_SAVEDATA, m_fSaveLogoFile);
			::SendDlgItemMessage(hDlg, IDC_LOGOOPTIONS_DATAFILENAME, EM_LIMITTEXT, MAX_PATH - 1, 0);
			::SetDlgItemText(hDlg, IDC_LOGOOPTIONS_DATAFILENAME, m_LogoFileName.c_str());
			EnableDlgItems(
				hDlg,
				IDC_LOGOOPTIONS_DATAFILENAME_LABEL,
				IDC_LOGOOPTIONS_DATAFILENAME_BROWSE,
				m_fSaveLogoFile);
			DlgCheckBox_Check(hDlg, IDC_LOGOOPTIONS_SAVERAWLOGO, LogoManager.GetSaveLogo());
			DlgCheckBox_Check(hDlg, IDC_LOGOOPTIONS_SAVEBMPLOGO, LogoManager.GetSaveLogoBmp());
			::SendDlgItemMessage(hDlg, IDC_LOGOOPTIONS_LOGOFOLDER, EM_LIMITTEXT, MAX_PATH - 1, 0);
			::SetDlgItemText(hDlg, IDC_LOGOOPTIONS_LOGOFOLDER, LogoManager.GetLogoDirectory());

			m_CurEventInfoFont = m_EventInfoFont;
			StyleUtil::SetFontInfoItem(hDlg, IDC_EVENTINFOOPTIONS_FONT_INFO, m_CurEventInfoFont);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_EPGOPTIONS_SAVEEPGFILE:
			EnableDlgItemsSyncCheckBox(
				hDlg,
				IDC_EPGOPTIONS_EPGFILENAME_LABEL,
				IDC_EPGOPTIONS_EPGFILENAME_BROWSE,
				IDC_EPGOPTIONS_SAVEEPGFILE);
			return TRUE;

		case IDC_EPGOPTIONS_EPGFILENAME_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH], szInitialDir[MAX_PATH];

				::GetDlgItemText(
					hDlg, IDC_EPGOPTIONS_EPGFILENAME,
					szFileName, lengthof(szFileName));
				InitOpenFileName(&ofn);
				ofn.hwndOwner = hDlg;
				ofn.lpstrFilter = TEXT("すべてのファイル\0*.*\0");
				ofn.lpstrFile = szFileName;
				ofn.nMaxFile = lengthof(szFileName);
				if (szFileName[0] == '\0' || ::PathIsFileSpec(szFileName)) {
					::GetModuleFileName(nullptr, szInitialDir, lengthof(szInitialDir));
					::PathRemoveFileSpec(szInitialDir);
					ofn.lpstrInitialDir = szInitialDir;
				}
				ofn.lpstrTitle = TEXT("EPGファイル名");
				ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY;

				if (FileOpenDialog(&ofn))
					::SetDlgItemText(hDlg, IDC_EPGOPTIONS_EPGFILENAME, szFileName);
			}
			return TRUE;

		case IDC_EPGOPTIONS_USEEPGDATA:
			EnableDlgItemsSyncCheckBox(
				hDlg,
				IDC_EPGOPTIONS_EPGDATAFOLDER_LABEL,
				IDC_EPGOPTIONS_EPGDATAFOLDER_BROWSE,
				IDC_EPGOPTIONS_USEEPGDATA);
			return TRUE;

		case IDC_EPGOPTIONS_EPGDATAFOLDER_BROWSE:
			{
				TCHAR szFolder[MAX_PATH];

				::GetDlgItemText(hDlg, IDC_EPGOPTIONS_EPGDATAFOLDER, szFolder, lengthof(szFolder));
				if (BrowseFolderDialog(hDlg, szFolder, TEXT("EPGデータのフォルダ")))
					::SetDlgItemText(hDlg, IDC_EPGOPTIONS_EPGDATAFOLDER, szFolder);
			}
			return TRUE;

		case IDC_LOGOOPTIONS_SAVEDATA:
			EnableDlgItemsSyncCheckBox(
				hDlg,
				IDC_LOGOOPTIONS_DATAFILENAME_LABEL,
				IDC_LOGOOPTIONS_DATAFILENAME_BROWSE,
				IDC_LOGOOPTIONS_SAVEDATA);
			return TRUE;

		case IDC_LOGOOPTIONS_DATAFILENAME_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH], szInitialDir[MAX_PATH];

				::GetDlgItemText(hDlg, IDC_LOGOOPTIONS_DATAFILENAME, szFileName, lengthof(szFileName));
				InitOpenFileName(&ofn);
				ofn.hwndOwner = hDlg;
				ofn.lpstrFilter = TEXT("すべてのファイル\0*.*\0");
				ofn.lpstrFile = szFileName;
				ofn.nMaxFile = lengthof(szFileName);
				if (szFileName[0] == '\0' || ::PathIsFileSpec(szFileName)) {
					::GetModuleFileName(nullptr, szInitialDir, lengthof(szInitialDir));
					::PathRemoveFileSpec(szInitialDir);
					ofn.lpstrInitialDir = szInitialDir;
				}
				ofn.lpstrTitle = TEXT("ロゴファイル名");
				ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY;

				if (FileOpenDialog(&ofn))
					::SetDlgItemText(hDlg, IDC_LOGOOPTIONS_DATAFILENAME, szFileName);
			}
			return TRUE;

		case IDC_LOGOOPTIONS_LOGOFOLDER_BROWSE:
			{
				TCHAR szFolder[MAX_PATH];

				::GetDlgItemText(hDlg, IDC_LOGOOPTIONS_LOGOFOLDER, szFolder, lengthof(szFolder));
				if (BrowseFolderDialog(hDlg, szFolder, TEXT("ロゴのフォルダ")))
					::SetDlgItemText(hDlg, IDC_LOGOOPTIONS_LOGOFOLDER, szFolder);
			}
			return TRUE;

		case IDC_EVENTINFOOPTIONS_FONT_CHOOSE:
			if (StyleUtil::ChooseStyleFont(hDlg, &m_CurEventInfoFont))
				StyleUtil::SetFontInfoItem(hDlg, IDC_EVENTINFOOPTIONS_FONT_INFO, m_CurEventInfoFont);
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CLogoManager &LogoManager = GetAppClass().LogoManager;

				m_fSaveEpgFile =
					DlgCheckBox_IsChecked(hDlg, IDC_EPGOPTIONS_SAVEEPGFILE);
				GetDlgItemString(hDlg, IDC_EPGOPTIONS_EPGFILENAME, &m_EpgFileName);
				m_fUpdateWhenStandby =
					DlgCheckBox_IsChecked(hDlg, IDC_EPGOPTIONS_UPDATEWHENSTANDBY);
				m_fUpdateBSExtended =
					DlgCheckBox_IsChecked(hDlg, IDC_EPGOPTIONS_UPDATEBSEXTENDED);
				m_fUpdateCSExtended =
					DlgCheckBox_IsChecked(hDlg, IDC_EPGOPTIONS_UPDATECSEXTENDED);
				bool fUseEpgData =
					DlgCheckBox_IsChecked(hDlg, IDC_EPGOPTIONS_USEEPGDATA);
				GetDlgItemString(hDlg, IDC_EPGOPTIONS_EPGDATAFOLDER, &m_EDCBDataFolder);
				if (!m_fUseEDCBData && fUseEpgData) {
					m_fUseEDCBData = fUseEpgData;
					AsyncLoadEDCBData(&GetAppClass().EpgLoadEventHandler);
				}
				m_fUseEDCBData = fUseEpgData;

				m_fSaveLogoFile = DlgCheckBox_IsChecked(hDlg, IDC_LOGOOPTIONS_SAVEDATA);
				GetDlgItemString(hDlg, IDC_LOGOOPTIONS_DATAFILENAME, &m_LogoFileName);
				LogoManager.SetSaveLogo(DlgCheckBox_IsChecked(hDlg, IDC_LOGOOPTIONS_SAVERAWLOGO));
				LogoManager.SetSaveLogoBmp(DlgCheckBox_IsChecked(hDlg, IDC_LOGOOPTIONS_SAVEBMPLOGO));
				String Path;
				GetDlgItemString(hDlg, IDC_LOGOOPTIONS_LOGOFOLDER, &Path);
				LogoManager.SetLogoDirectory(Path.c_str());

				if (m_EventInfoFont != m_CurEventInfoFont) {
					m_EventInfoFont = m_CurEventInfoFont;
					SetGeneralUpdateFlag(UPDATE_GENERAL_EVENTINFOFONT);
				}

				m_fChanged = true;
			}
			break;
		}
		break;
	}

	return FALSE;
}


}	// namespace TVTest
