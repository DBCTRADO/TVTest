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
	m_EpgFileLoader.reset();
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
	const CLogoManager &LogoManager = GetAppClass().LogoManager;

	Settings.Write(TEXT("SaveEpgData"), m_fSaveEpgFile);
	Settings.Write(TEXT("EpgDataFileName"), m_EpgFileName);
	Settings.Write(TEXT("EpgUpdateWhenStandby"), m_fUpdateWhenStandby);
	Settings.Write(TEXT("EpgUpdateBSExtended"), m_fUpdateBSExtended);
	Settings.Write(TEXT("EpgUpdateCSExtended"), m_fUpdateCSExtended);
	Settings.Write(TEXT("UseEpgData"), m_fUseEDCBData);
	Settings.Write(TEXT("EpgDataFolder"), m_EDCBDataFolder);
	Settings.Write(TEXT("EpgTimeMode"), static_cast<int>(m_EpgTimeMode));

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


bool CEpgOptions::LoadEpgFile(
	LibISDB::EPGDatabase *pEPGDatabase,
	CEpgFileLoadEventHandler *pEventHandler,
	EpgFileLoadFlag Flags)
{
	CEpgDataStore *pEpgDataStore = nullptr;
	CFilePath EpgDataPath;
	CEpgDataLoader *pEdcbDataLoader = nullptr;
	String EdcbDataFolder;

	if (!!(Flags & EpgFileLoadFlag::EpgData) && m_fSaveEpgFile) {
		if (GetAbsolutePath(m_EpgFileName, &EpgDataPath)) {
			if (EpgDataPath.IsFileExists()) {
				pEpgDataStore = &m_EpgDataStore;
			}
		}
	}

	if (!!(Flags & EpgFileLoadFlag::EdcbData) && m_fUseEDCBData && !m_EDCBDataFolder.empty()) {
		if (GetAbsolutePath(m_EDCBDataFolder, &EdcbDataFolder)) {
			if (::PathIsDirectory(EdcbDataFolder.c_str())) {
				m_EpgDataLoader = std::make_unique<CEpgDataLoader>();
				pEdcbDataLoader = m_EpgDataLoader.get();
			}
		}
	}

	if ((pEpgDataStore != nullptr) || (pEdcbDataLoader != nullptr)) {
		m_EpgFileLoader = std::make_unique<CEpgFileLoader>();
		if (!m_EpgFileLoader->StartLoading(
					pEPGDatabase,
					pEpgDataStore, EpgDataPath,
					pEdcbDataLoader, EdcbDataFolder,
					pEventHandler)) {
			m_EpgFileLoader.reset();
			return false;
		}
	}

	return true;
}


bool CEpgOptions::IsEpgFileLoading() const
{
	return m_EpgFileLoader
		&& m_EpgFileLoader->IsLoading();
}


bool CEpgOptions::IsEpgDataLoading() const
{
	return m_EpgFileLoader
		&& m_EpgFileLoader->IsEpgDataLoading();
}


bool CEpgOptions::WaitEpgFileLoad(DWORD Timeout)
{
	return !m_EpgFileLoader
		|| m_EpgFileLoader->WaitLoading(Timeout);
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
			WaitEpgFileLoad();

			GetAppClass().AddLog(TEXT("EPG データを \"{}\" に保存します..."), FilePath);

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


bool CEpgOptions::LoadLogoFile()
{
	if (m_fSaveLogoFile && !m_LogoFileName.empty()) {
		CAppMain &App = GetAppClass();
		CLogoManager &LogoManager = App.LogoManager;
		CFilePath FilePath;

		if (!GetAbsolutePath(m_LogoFileName, &FilePath))
			return false;
		if (FilePath.IsFileExists()) {
			App.AddLog(TEXT("ロゴデータを \"{}\" から読み込みます..."), FilePath);
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
		App.AddLog(TEXT("ロゴ設定を \"{}\" から読み込みます..."), FilePath);
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
			App.AddLog(TEXT("ロゴデータを \"{}\" に保存します..."), FilePath);
			if (!LogoManager.SaveLogoFile(FilePath.c_str())) {
				App.AddLog(CLogItem::LogType::Error, TEXT("ロゴファイルの保存でエラーが発生しました。"));
				return false;
			}
		}

		FilePath += TEXT(".ini");
		if (!FilePath.IsFileExists() || LogoManager.IsLogoIDMapUpdated()) {
			App.AddLog(TEXT("ロゴ設定を \"{}\" に保存します..."), FilePath);
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
			const CLogoManager &LogoManager = GetAppClass().LogoManager;

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

			AddControls({
				{IDC_EPGOPTIONS_GROUP,                AlignFlag::Horz},
				{IDC_EPGOPTIONS_EPGFILENAME,          AlignFlag::Horz},
				{IDC_EPGOPTIONS_EPGFILENAME_BROWSE,   AlignFlag::Right},
				{IDC_EPGOPTIONS_EPGDATAFOLDER,        AlignFlag::Horz},
				{IDC_EPGOPTIONS_EPGDATAFOLDER_BROWSE, AlignFlag::Right},
				{IDC_LOGOOPTIONS_GROUP,               AlignFlag::Horz},
				{IDC_LOGOOPTIONS_DATAFILENAME,        AlignFlag::Horz},
				{IDC_LOGOOPTIONS_DATAFILENAME_BROWSE, AlignFlag::Right},
				{IDC_LOGOOPTIONS_LOGOFOLDER,          AlignFlag::Horz},
				{IDC_LOGOOPTIONS_LOGOFOLDER_BROWSE,   AlignFlag::Right},
			});
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
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
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
				const bool fUseEpgData =
					DlgCheckBox_IsChecked(hDlg, IDC_EPGOPTIONS_USEEPGDATA);
				GetDlgItemString(hDlg, IDC_EPGOPTIONS_EPGDATAFOLDER, &m_EDCBDataFolder);
				if (!m_fUseEDCBData && fUseEpgData) {
					m_fUseEDCBData = fUseEpgData;
					CAppMain &App = GetAppClass();
					LoadEpgFile(
						&App.EPGDatabase,
						&App.EpgLoadEventHandler,
						EpgFileLoadFlag::EdcbData);
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




CEpgOptions::CEpgFileLoader::~CEpgFileLoader()
{
	if (m_hThread != nullptr) {
		if (::WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT) {
			if (m_hAbortEvent != nullptr)
				::SetEvent(m_hAbortEvent);
			::CancelSynchronousIo(m_hThread);
			if (::WaitForSingleObject(m_hThread, 10000) == WAIT_TIMEOUT) {
				TRACE(TEXT("Terminate CEpgFileLoader thread\n"));
				::TerminateThread(m_hThread, -1);
			}
		}
		::CloseHandle(m_hThread);
	}

	if (m_hAbortEvent != nullptr) {
		::CloseHandle(m_hAbortEvent);
		m_hAbortEvent = nullptr;
	}
}


bool CEpgOptions::CEpgFileLoader::StartLoading(
	LibISDB::EPGDatabase *pEPGDatabase,
	CEpgDataStore *pEpgDataStore, const String &EpgDataPath,
	CEpgDataLoader *pEdcbDataLoader, const String &EdcbDataFolder,
	CEpgFileLoadEventHandler *pEventHandler)
{
	if (m_hThread != nullptr) {
		if (::WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT)
			return false;
		::CloseHandle(m_hThread);
		m_hThread = nullptr;
	}
	if (m_hAbortEvent != nullptr) {
		::CloseHandle(m_hAbortEvent);
		m_hAbortEvent = nullptr;
	}

	m_State = STATE_READY;

	m_pEPGDatabase = pEPGDatabase;
	m_EpgDataPath = EpgDataPath;
	m_pEpgDataStore = pEpgDataStore;
	m_EdcbDataFolder = EdcbDataFolder;
	m_pEdcbDataLoader = pEdcbDataLoader;
	m_pEventHandler = pEventHandler;

	m_hAbortEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);

	m_hThread = reinterpret_cast<HANDLE>(::_beginthreadex(nullptr, 0, LoadThread, this, 0, nullptr));
	if (m_hThread == nullptr) {
		m_State = STATE_ERROR;
		return false;
	}
	m_State = STATE_THREAD_START;

	return true;
}


bool CEpgOptions::CEpgFileLoader::IsLoading()
{
	return m_hThread != nullptr
		&& ::WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
}


bool CEpgOptions::CEpgFileLoader::IsEpgDataLoading()
{
	if (!IsLoading())
		return false;

	const int State = m_State;

	return (State == STATE_EPG_DATA_LOADING)
		|| (State == STATE_THREAD_START && m_pEpgDataStore != nullptr);
}


bool CEpgOptions::CEpgFileLoader::WaitLoading(DWORD Timeout)
{
	if (m_hThread != nullptr) {
		if (::WaitForSingleObject(m_hThread, Timeout) == WAIT_TIMEOUT)
			return false;
	}
	return true;
}


void CEpgOptions::CEpgFileLoader::OnBeginLoading()
{
	m_State = STATE_EPG_DATA_LOADING;
	if (m_pEventHandler != nullptr)
		m_pEventHandler->OnBeginEpgDataLoading();
}


void CEpgOptions::CEpgFileLoader::OnEndLoading(bool fSuccess)
{
	m_State = STATE_EPG_DATA_LOADED;
	if (m_pEventHandler != nullptr)
		m_pEventHandler->OnEndEpgDataLoading(fSuccess);
}


void CEpgOptions::CEpgFileLoader::OnStart()
{
	m_State = STATE_EDCB_DATA_LOADING;
	if (m_pEventHandler != nullptr)
		m_pEventHandler->OnBeginEdcbDataLoading();
}


void CEpgOptions::CEpgFileLoader::OnEnd(bool fSuccess, LibISDB::EPGDatabase *pEPGDatabase)
{
	m_State = STATE_EDCB_DATA_LOADED;
	if (m_pEventHandler != nullptr)
		m_pEventHandler->OnEndEdcbDataLoading(fSuccess, pEPGDatabase);
}


void CEpgOptions::CEpgFileLoader::LoadMain()
{
	if (m_pEpgDataStore != nullptr) {
		GetAppClass().AddLog(TEXT("EPG データを \"{}\" から読み込みます..."), m_EpgDataPath);

		if (m_pEpgDataStore->Open(m_pEPGDatabase, m_EpgDataPath.c_str(), CEpgDataStore::OpenFlag::LoadBackground)) {
			m_pEpgDataStore->SetEventHandler(this);
			m_pEpgDataStore->Load();
		}
	}

	if (::WaitForSingleObject(m_hAbortEvent, 0) == WAIT_OBJECT_0)
		return;

	if (m_pEdcbDataLoader != nullptr) {
		GetAppClass().AddLog(TEXT("EDCB の EPG データを \"{}\" から読み込みます..."), m_EdcbDataFolder);

		m_pEdcbDataLoader->Load(m_EdcbDataFolder.c_str(), m_hAbortEvent, this);
	}
}


unsigned int __stdcall CEpgOptions::CEpgFileLoader::LoadThread(void *pParameter)
{
	CEpgFileLoader *pLoader = static_cast<CEpgFileLoader*>(pParameter);

	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_LOWEST);

	pLoader->LoadMain();

	pLoader->m_State = STATE_THREAD_END;

	return 0;
}


}	// namespace TVTest
