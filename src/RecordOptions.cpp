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
#include "RecordOptions.h"
#include "Command.h"
#include "DialogUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


static constexpr UINT MEGA_BYTES = 1024 * 1024;

// 書き出しバッファサイズの制限(バイト単位)
static constexpr UINT WRITE_BUFFER_SIZE_MIN = 1024;
static constexpr UINT WRITE_BUFFER_SIZE_MAX = 32 * MEGA_BYTES;

// さかのぼり録画バッファサイズの制限(バイト単位)
static constexpr UINT TIMESHIFT_BUFFER_SIZE_MIN = 1 * MEGA_BYTES;
static constexpr UINT TIMESHIFT_BUFFER_SIZE_MAX = 1024 * MEGA_BYTES;

// 書き出し待ちバッファの制限(バイト単位)
static constexpr UINT MAX_PENDING_SIZE_MIN = 32 * MEGA_BYTES;
static constexpr UINT MAX_PENDING_SIZE_MAX = 1024 * MEGA_BYTES;

// ステータスバーからの録画のコマンド
static const int StatusBarCommandList[] = {
	CM_RECORD_START,
	CM_RECORDOPTION,
	CM_TIMESHIFTRECORDING,
	0,
};




CRecordOptions::CRecordOptions()
	: m_StatusBarRecordCommand(CM_RECORD_START)
{
}


CRecordOptions::~CRecordOptions()
{
	Destroy();
}


bool CRecordOptions::Apply(DWORD Flags)
{
	if ((Flags & UPDATE_RECORDINGSETTINGS) != 0)
		GetAppClass().RecordManager.SetRecordingSettings(m_Settings);

	return true;
}


bool CRecordOptions::ReadSettings(CSettings &Settings)
{
	TCHAR szPath[MAX_PATH];
	String Path;
	unsigned int Value;

	// 古いバージョンの互換用
	if (Settings.Read(TEXT("RecordFile"), szPath, lengthof(szPath))
			&& szPath[0] != '\0') {
		LPTSTR pszFileName = ::PathFindFileName(szPath);

		if (pszFileName != szPath) {
			*(pszFileName - 1) = '\0';
			m_SaveFolder = szPath;
			m_FileName = pszFileName;
		}
	}
	if (Settings.Read(TEXT("RecordFolder"), &Path)
			&& !Path.empty())
		m_SaveFolder = Path;
	if (Settings.Read(TEXT("RecordFileName"), &Path)
			&& !Path.empty())
		m_FileName = Path;
#if 0
	// 古いバージョンの互換用
	bool fAddTime;
	if (Settings.Read(TEXT("AddRecordTime"), &fAddTime) && fAddTime) {
		String Extension;
		m_FileName.GetExtension(&Extension);
		m_FileName.RemoveExtension();
		m_FileName += TEXT("%date%_%time%");
		m_FileName += Extension;
	}
#endif
	Settings.Read(TEXT("ConfirmRecChChange"), &m_fConfirmChannelChange);
	Settings.Read(TEXT("ConfrimRecordingExit"), &m_fConfirmExit);
	Settings.Read(TEXT("ConfrimRecordStop"), &m_fConfirmStop);
	Settings.Read(TEXT("ConfrimRecordStopStatusBarOnly"), &m_fConfirmStopStatusBarOnly);
	Settings.Read(TEXT("RecordCurServiceOnly"), &m_Settings.m_fCurServiceOnly);
	bool f;
	if (Settings.Read(TEXT("RecordSubtitle"), &f))
		m_Settings.SetSaveCaption(f);
	if (Settings.Read(TEXT("RecordDataCarrousel"), &f))
		m_Settings.SetSaveDataCarrousel(f);
	Settings.Read(TEXT("WritePlugin"), &m_Settings.m_WritePlugin);
	if (Settings.Read(TEXT("RecordBufferSize"), &Value))
		m_Settings.m_WriteCacheSize = std::clamp(Value, WRITE_BUFFER_SIZE_MIN, WRITE_BUFFER_SIZE_MAX);
	Settings.Read(TEXT("AlertLowFreeSpace"), &m_fAlertLowFreeSpace);
	Settings.Read(TEXT("LowFreeSpaceThreshold"), &m_LowFreeSpaceThreshold);
	if (Settings.Read(TEXT("TimeShiftRecBufferSize"), &Value))
		m_Settings.m_TimeShiftBufferSize = std::clamp(Value * MEGA_BYTES, TIMESHIFT_BUFFER_SIZE_MIN, TIMESHIFT_BUFFER_SIZE_MAX);
	Settings.Read(TEXT("TimeShiftRecording"), &m_Settings.m_fEnableTimeShift);
	if (Settings.Read(TEXT("RecMaxPendingSize"), &Value))
		m_Settings.m_MaxPendingSize = std::clamp(Value, MAX_PENDING_SIZE_MIN, MAX_PENDING_SIZE_MAX);
	Settings.Read(TEXT("ShowRecordRemainTime"), &m_fShowRemainTime);

	String CommandText;
	if (Settings.Read(TEXT("StatusBarRecordCommand"), &CommandText)) {
		if (!CommandText.empty()) {
			const int Command = GetAppClass().CommandManager.ParseIDText(CommandText);
			if (Command != 0)
				m_StatusBarRecordCommand = Command;
		} else {
			m_StatusBarRecordCommand = 0;
		}
	}

	return true;
}


bool CRecordOptions::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("RecordFolder"), m_SaveFolder);
	Settings.Write(TEXT("RecordFileName"), m_FileName);
	Settings.Write(TEXT("ConfirmRecChChange"), m_fConfirmChannelChange);
	Settings.Write(TEXT("ConfrimRecordingExit"), m_fConfirmExit);
	Settings.Write(TEXT("ConfrimRecordStop"), m_fConfirmStop);
	Settings.Write(TEXT("ConfrimRecordStopStatusBarOnly"), m_fConfirmStopStatusBarOnly);
	Settings.Write(TEXT("RecordCurServiceOnly"), m_Settings.m_fCurServiceOnly);
	Settings.Write(TEXT("RecordSubtitle"), m_Settings.IsSaveCaption());
	Settings.Write(TEXT("RecordDataCarrousel"), m_Settings.IsSaveDataCarrousel());
	Settings.Write(TEXT("WritePlugin"), m_Settings.m_WritePlugin);
	Settings.Write(TEXT("RecordBufferSize"), static_cast<unsigned int>(m_Settings.m_WriteCacheSize));
	Settings.Write(TEXT("AlertLowFreeSpace"), m_fAlertLowFreeSpace);
	Settings.Write(TEXT("LowFreeSpaceThreshold"), m_LowFreeSpaceThreshold);
	Settings.Write(TEXT("TimeShiftRecBufferSize"), static_cast<unsigned int>(m_Settings.m_TimeShiftBufferSize / MEGA_BYTES));
	Settings.Write(TEXT("TimeShiftRecording"), m_Settings.m_fEnableTimeShift);
	Settings.Write(TEXT("RecMaxPendingSize"), static_cast<unsigned int>(m_Settings.m_MaxPendingSize));
	Settings.Write(TEXT("ShowRecordRemainTime"), m_fShowRemainTime);
	if (m_StatusBarRecordCommand != 0) {
		Settings.Write(TEXT("StatusBarRecordCommand"), GetAppClass().CommandManager.GetCommandIDText(m_StatusBarRecordCommand));
	} else {
		Settings.Write(TEXT("StatusBarRecordCommand"), TEXT(""));
	}
	return true;
}


bool CRecordOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(
		hwndOwner,
		GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_OPTIONS_RECORD));
}


bool CRecordOptions::SetSaveFolder(LPCTSTR pszFolder)
{
	m_SaveFolder = pszFolder;
	return true;
}


bool CRecordOptions::GetFilePath(LPTSTR pszFileName, int MaxLength) const
{
	if (m_SaveFolder.empty() || m_FileName.empty())
		return false;
	if (m_SaveFolder.length() + 1 + m_FileName.length() >= static_cast<size_t>(MaxLength))
		return false;
	::PathCombine(pszFileName, m_SaveFolder.c_str(), m_FileName.c_str());
	return true;
}


bool CRecordOptions::GenerateFilePath(CFilePath *pFilePath, LPCTSTR *ppszErrorMessage) const
{
	if (m_SaveFolder.empty()) {
		if (ppszErrorMessage)
			*ppszErrorMessage = TEXT("設定で保存先フォルダを指定してください。");
		return false;
	}
	if (m_FileName.empty()) {
		if (ppszErrorMessage)
			*ppszErrorMessage = TEXT("設定でファイル名を指定してください。");
		return false;
	}

	*pFilePath = m_SaveFolder;
	pFilePath->Append(m_FileName);

	return true;
}


bool CRecordOptions::ConfirmChannelChange(HWND hwndOwner) const
{
	if (m_fConfirmChannelChange) {
		if (::MessageBox(
					hwndOwner, TEXT("録画中です。チャンネル変更しますか?"),
					TEXT("チャンネル変更の確認"),
					MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONEXCLAMATION) != IDOK)
			return false;
	}
	return true;
}


bool CRecordOptions::ConfirmServiceChange(HWND hwndOwner, const CRecordManager *pRecordManager) const
{
	if (pRecordManager->GetCurServiceOnly()) {
		if (::MessageBox(
					hwndOwner,
					TEXT("現在のサービスのみ録画中です。\r\n")
					TEXT("サービスの変更をすると正常に再生できなくなるかも知れません。\r\n")
					TEXT("サービスを変更しますか?"),
					TEXT("変更の確認"),
					MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONEXCLAMATION) != IDOK)
			return false;
	}
	return true;
}


bool CRecordOptions::ConfirmStop(HWND hwndOwner) const
{
	if (m_fConfirmStop && !m_fConfirmStopStatusBarOnly) {
		if (::MessageBox(
					hwndOwner,
					TEXT("録画を停止しますか?"), TEXT("停止の確認"),
					MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONINFORMATION) != IDOK)
			return false;
	}
	return true;
}


bool CRecordOptions::ConfirmStatusBarStop(HWND hwndOwner) const
{
	if (m_fConfirmStop && m_fConfirmStopStatusBarOnly) {
		if (::MessageBox(
					hwndOwner,
					TEXT("録画を停止しますか?"), TEXT("停止の確認"),
					MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONINFORMATION) != IDOK)
			return false;
	}
	return true;
}


bool CRecordOptions::ConfirmExit(HWND hwndOwner, const CRecordManager *pRecordManager) const
{
	if (m_fConfirmExit && pRecordManager->IsRecording()) {
		if (::MessageBox(
					hwndOwner,
					TEXT("現在録画中です。\r\n終了してもいいですか?"),
					TEXT("終了の確認"),
					MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONEXCLAMATION) != IDOK)
			return false;
	}
	if (pRecordManager->IsReserved()) {
		if (::MessageBox(
					hwndOwner,
					TEXT("録画の設定がされています。\r\n")
					TEXT("終了すると録画は行われません。\r\n")
					TEXT("終了してもいいですか?"),
					TEXT("終了の確認"),
					MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONEXCLAMATION) != IDOK)
			return false;
	}
	return true;
}


bool CRecordOptions::EnableTimeShiftRecording(bool fEnable)
{
	if (m_Settings.m_fEnableTimeShift != fEnable) {
		m_Settings.m_fEnableTimeShift = fEnable;
		Apply(UPDATE_RECORDINGSETTINGS);
	}
	return true;
}


INT_PTR CRecordOptions::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
#if 0
			if (m_SaveFolder.empty()) {
				PWSTR pszPath;
				if (::SHGetKnownFolderPath(FOLDERID_Videos, 0, nullptr, &pszPath) == S_OK
						|| ::SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &pszPath) == S_OK) {
					m_SaveFolder = pszPath;
					::CoTaskMemFree(pszPath);
				}
			}
#endif
			::SendDlgItemMessage(hDlg, IDC_RECORDOPTIONS_SAVEFOLDER, EM_LIMITTEXT, MAX_PATH - 1, 0);
			::SetDlgItemText(hDlg, IDC_RECORDOPTIONS_SAVEFOLDER, m_SaveFolder.c_str());
			::SendDlgItemMessage(hDlg, IDC_RECORDOPTIONS_FILENAME, EM_LIMITTEXT, MAX_PATH - 1, 0);
			::SetDlgItemText(hDlg, IDC_RECORDOPTIONS_FILENAME, m_FileName.c_str());
			InitDropDownButton(hDlg, IDC_RECORDOPTIONS_FILENAMEFORMAT);
			DlgCheckBox_Check(
				hDlg, IDC_RECORDOPTIONS_CONFIRMCHANNELCHANGE,
				m_fConfirmChannelChange);
			DlgCheckBox_Check(
				hDlg, IDC_RECORDOPTIONS_CONFIRMEXIT,
				m_fConfirmExit);
			DlgCheckBox_Check(
				hDlg, IDC_RECORDOPTIONS_CONFIRMSTOP,
				m_fConfirmStop);
			DlgCheckBox_Check(
				hDlg, IDC_RECORDOPTIONS_CONFIRMSTATUSBARSTOP,
				m_fConfirmStopStatusBarOnly);
			EnableDlgItem(hDlg, IDC_RECORDOPTIONS_CONFIRMSTATUSBARSTOP, m_fConfirmStop);
			DlgCheckBox_Check(
				hDlg, IDC_RECORDOPTIONS_CURSERVICEONLY,
				m_Settings.m_fCurServiceOnly);
			DlgCheckBox_Check(
				hDlg, IDC_RECORDOPTIONS_SAVESUBTITLE,
				m_Settings.IsSaveCaption());
			DlgCheckBox_Check(
				hDlg, IDC_RECORDOPTIONS_SAVEDATACARROUSEL,
				m_Settings.IsSaveDataCarrousel());

			// 保存プラグイン
			DlgComboBox_AddString(hDlg, IDC_RECORDOPTIONS_WRITEPLUGIN, TEXT("使用しない (TS出力)"));
			CRecordManager::GetWritePluginList(&m_WritePluginList);
			int Sel = -1;
			for (size_t i = 0; i < m_WritePluginList.size(); i++) {
				LPCTSTR pszFileName = m_WritePluginList[i].c_str();
				DlgComboBox_AddString(hDlg, IDC_RECORDOPTIONS_WRITEPLUGIN, pszFileName);
				if (!m_Settings.m_WritePlugin.empty()
						&& IsEqualFileName(pszFileName, m_Settings.m_WritePlugin.c_str()))
					Sel = static_cast<int>(i);
			}
			DlgComboBox_SetCurSel(hDlg, IDC_RECORDOPTIONS_WRITEPLUGIN, Sel + 1);
			EnableDlgItem(hDlg, IDC_RECORDOPTIONS_WRITEPLUGINSETTING, Sel >= 0);

			DlgCheckBox_Check(hDlg, IDC_RECORDOPTIONS_ALERTLOWFREESPACE, m_fAlertLowFreeSpace);
			::SetDlgItemInt(hDlg, IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD, m_LowFreeSpaceThreshold, FALSE);
			DlgUpDown_SetRange(
				hDlg, IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD_SPIN,
				1, 0x7FFFFFFF);
			EnableDlgItems(
				hDlg, IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD_LABEL,
				IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD_UNIT,
				m_fAlertLowFreeSpace);

			const CCommandManager &CommandManager = GetAppClass().CommandManager;
			for (int i = 0; i < lengthof(StatusBarCommandList); i++) {
				const int Command = StatusBarCommandList[i];
				TCHAR szText[CCommandManager::MAX_COMMAND_TEXT];

				if (Command != 0)
					CommandManager.GetCommandText(Command, szText, lengthof(szText));
				else
					StringCopy(szText, TEXT("何もしない"));
				DlgComboBox_AddString(hDlg, IDC_RECORDOPTIONS_STATUSBARCOMMAND, szText);
				if (Command == m_StatusBarRecordCommand)
					DlgComboBox_SetCurSel(hDlg, IDC_RECORDOPTIONS_STATUSBARCOMMAND, i);
			}

			::SetDlgItemInt(
				hDlg, IDC_RECORDOPTIONS_BUFFERSIZE,
				m_Settings.m_WriteCacheSize / 1024, FALSE);
			DlgUpDown_SetRange(
				hDlg, IDC_RECORDOPTIONS_BUFFERSIZE_UD,
				WRITE_BUFFER_SIZE_MIN / 1024, WRITE_BUFFER_SIZE_MAX / 1024);

			::SetDlgItemInt(
				hDlg, IDC_RECORDOPTIONS_TIMESHIFTBUFFERSIZE,
				m_Settings.m_TimeShiftBufferSize / MEGA_BYTES, FALSE);
			DlgUpDown_SetRange(
				hDlg, IDC_RECORDOPTIONS_TIMESHIFTBUFFERSIZE_SPIN,
				TIMESHIFT_BUFFER_SIZE_MIN / MEGA_BYTES, TIMESHIFT_BUFFER_SIZE_MAX / MEGA_BYTES);

			::SetDlgItemInt(
				hDlg, IDC_RECORDOPTIONS_MAXPENDINGSIZE,
				m_Settings.m_MaxPendingSize / MEGA_BYTES, FALSE);
			DlgUpDown_SetRange(
				hDlg, IDC_RECORDOPTIONS_MAXPENDINGSIZE_SPIN,
				MAX_PENDING_SIZE_MIN / MEGA_BYTES, MAX_PENDING_SIZE_MAX / MEGA_BYTES);

			AddControls({
				{IDC_RECORDOPTIONS_SAVEFOLDER,        AlignFlag::Horz},
				{IDC_RECORDOPTIONS_SAVEFOLDER_BROWSE, AlignFlag::Right},
				{IDC_RECORDOPTIONS_FILENAME,          AlignFlag::Horz},
				{IDC_RECORDOPTIONS_FILENAMEFORMAT,    AlignFlag::Right},
				{IDC_RECORDOPTIONS_FILENAMEPREVIEW,   AlignFlag::Horz},
			});
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_RECORDOPTIONS_SAVEFOLDER_BROWSE:
			{
				TCHAR szFolder[MAX_PATH];

				::GetDlgItemText(hDlg, IDC_RECORDOPTIONS_SAVEFOLDER, szFolder, MAX_PATH);
				if (BrowseFolderDialog(hDlg, szFolder, TEXT("録画ファイルの保存先フォルダ:")))
					::SetDlgItemText(hDlg, IDC_RECORDOPTIONS_SAVEFOLDER, szFolder);
			}
			return TRUE;

		case IDC_RECORDOPTIONS_FILENAME:
			if (HIWORD(wParam) == EN_CHANGE) {
				TCHAR szFormat[MAX_PATH];
				String FileName;

				::GetDlgItemText(hDlg, IDC_RECORDOPTIONS_FILENAME, szFormat, lengthof(szFormat));
				if (szFormat[0] != '\0') {
					CEventVariableStringMap EventVarStrMap;
					EventVarStrMap.SetSampleEventInfo();
					FormatVariableString(&EventVarStrMap, szFormat, &FileName);
				}
				::SetDlgItemText(hDlg, IDC_RECORDOPTIONS_FILENAMEPREVIEW, FileName.c_str());
			}
			return TRUE;

		case IDC_RECORDOPTIONS_FILENAMEFORMAT:
			{
				RECT rc;

				::GetWindowRect(::GetDlgItem(hDlg, IDC_RECORDOPTIONS_FILENAMEFORMAT), &rc);
				const POINT pt = {rc.left, rc.bottom};
				CEventVariableStringMap EventVarStrMap;
				EventVarStrMap.InputParameter(hDlg, IDC_RECORDOPTIONS_FILENAME, pt);
			}
			return TRUE;

		case IDC_RECORDOPTIONS_CONFIRMSTOP:
			EnableDlgItem(
				hDlg, IDC_RECORDOPTIONS_CONFIRMSTATUSBARSTOP,
				DlgCheckBox_IsChecked(hDlg, IDC_RECORDOPTIONS_CONFIRMSTOP));
			return TRUE;

		case IDC_RECORDOPTIONS_ALERTLOWFREESPACE:
			EnableDlgItems(
				hDlg, IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD_LABEL,
				IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD_UNIT,
				DlgCheckBox_IsChecked(hDlg, IDC_RECORDOPTIONS_ALERTLOWFREESPACE));
			return TRUE;

		case IDC_RECORDOPTIONS_WRITEPLUGIN:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				EnableDlgItem(
					hDlg, IDC_RECORDOPTIONS_WRITEPLUGINSETTING,
					DlgComboBox_GetCurSel(hDlg, IDC_RECORDOPTIONS_WRITEPLUGIN) > 0);
			}
			return TRUE;

		case IDC_RECORDOPTIONS_WRITEPLUGINSETTING:
			{
				const LRESULT Sel = DlgComboBox_GetCurSel(hDlg, IDC_RECORDOPTIONS_WRITEPLUGIN) - 1;

				if (Sel >= 0 && static_cast<size_t>(Sel) < m_WritePluginList.size()) {
					CRecordManager::ShowWritePluginSetting(hDlg, m_WritePluginList[Sel].c_str());
				}
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case PSN_APPLY:
			{
				String SaveFolder, FileName;

				GetDlgItemString(hDlg, IDC_RECORDOPTIONS_SAVEFOLDER, &SaveFolder);
				const CAppMain::CreateDirectoryResult CreateDirResult =
					GetAppClass().CreateDirectory(
						hDlg, SaveFolder.c_str(),
						TEXT("録画ファイルの保存先フォルダ \"{}\" がありません。\n")
						TEXT("作成しますか?"));
				if (CreateDirResult == CAppMain::CreateDirectoryResult::Error) {
					SettingError();
					SetDlgItemFocus(hDlg, IDC_RECORDOPTIONS_SAVEFOLDER);
					return TRUE;
				}

				GetDlgItemString(hDlg, IDC_RECORDOPTIONS_FILENAME, &FileName);
				if (!FileName.empty()) {
					String Message;
					if (!IsValidFileName(FileName.c_str(), FileNameValidateFlag::AllowDelimiter, &Message)) {
						SettingError();
						::SendDlgItemMessage(hDlg, IDC_RECORDOPTIONS_FILENAME, EM_SETSEL, 0, -1);
						::MessageBox(hDlg, Message.c_str(), nullptr, MB_OK | MB_ICONEXCLAMATION);
						SetDlgItemFocus(hDlg, IDC_RECORDOPTIONS_FILENAME);
						return TRUE;
					}
				}

				m_SaveFolder = SaveFolder;
				m_FileName = FileName;

				m_fConfirmChannelChange =
					DlgCheckBox_IsChecked(hDlg, IDC_RECORDOPTIONS_CONFIRMCHANNELCHANGE);
				m_fConfirmExit =
					DlgCheckBox_IsChecked(hDlg, IDC_RECORDOPTIONS_CONFIRMEXIT);
				m_fConfirmStop =
					DlgCheckBox_IsChecked(hDlg, IDC_RECORDOPTIONS_CONFIRMSTOP);
				m_fConfirmStopStatusBarOnly =
					DlgCheckBox_IsChecked(hDlg, IDC_RECORDOPTIONS_CONFIRMSTATUSBARSTOP);

				m_Settings.m_fCurServiceOnly =
					DlgCheckBox_IsChecked(hDlg, IDC_RECORDOPTIONS_CURSERVICEONLY);
				m_Settings.SetSaveCaption(
					DlgCheckBox_IsChecked(hDlg, IDC_RECORDOPTIONS_SAVESUBTITLE));
				m_Settings.SetSaveDataCarrousel(
					DlgCheckBox_IsChecked(hDlg, IDC_RECORDOPTIONS_SAVEDATACARROUSEL));

				m_fAlertLowFreeSpace =
					DlgCheckBox_IsChecked(hDlg, IDC_RECORDOPTIONS_ALERTLOWFREESPACE);
				m_LowFreeSpaceThreshold =
					::GetDlgItemInt(hDlg, IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD, nullptr, FALSE);

				int Sel = static_cast<int>(DlgComboBox_GetCurSel(hDlg, IDC_RECORDOPTIONS_STATUSBARCOMMAND));
				if (Sel >= 0 && Sel < lengthof(StatusBarCommandList))
					m_StatusBarRecordCommand = StatusBarCommandList[Sel];

				Sel = static_cast<int>(DlgComboBox_GetCurSel(hDlg, IDC_RECORDOPTIONS_WRITEPLUGIN)) - 1;
				if (Sel >= 0 && static_cast<size_t>(Sel) < m_WritePluginList.size())
					m_Settings.m_WritePlugin = m_WritePluginList[Sel];
				else
					m_Settings.m_WritePlugin.clear();

				unsigned int BufferSize =
					::GetDlgItemInt(hDlg, IDC_RECORDOPTIONS_BUFFERSIZE, nullptr, FALSE);
				if (BufferSize != 0)
					m_Settings.m_WriteCacheSize = std::clamp(BufferSize * 1024, WRITE_BUFFER_SIZE_MIN, WRITE_BUFFER_SIZE_MAX);

				BufferSize = ::GetDlgItemInt(hDlg, IDC_RECORDOPTIONS_TIMESHIFTBUFFERSIZE, nullptr, FALSE);
				if (BufferSize != 0) {
					BufferSize = std::clamp(BufferSize * MEGA_BYTES, TIMESHIFT_BUFFER_SIZE_MIN, TIMESHIFT_BUFFER_SIZE_MAX);
					if (BufferSize != m_Settings.m_TimeShiftBufferSize) {
						m_Settings.m_TimeShiftBufferSize = BufferSize;
					}
				}

				BufferSize = ::GetDlgItemInt(hDlg, IDC_RECORDOPTIONS_MAXPENDINGSIZE, nullptr, FALSE);
				if (BufferSize != 0)
					m_Settings.m_MaxPendingSize = std::clamp(BufferSize * MEGA_BYTES, MAX_PENDING_SIZE_MIN, MAX_PENDING_SIZE_MAX);

				SetUpdateFlag(UPDATE_RECORDINGSETTINGS);

				m_fChanged = true;
			}
			break;
		}
		break;

	case WM_DESTROY:
		m_WritePluginList.clear();
		break;
	}

	return FALSE;
}


}	// namespace TVTest
