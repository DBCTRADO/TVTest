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
#include "GeneralOptions.h"
#include "DriverManager.h"
#include "DialogUtil.h"
#include "MessageDialog.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CGeneralOptions::~CGeneralOptions()
{
	Destroy();
}


bool CGeneralOptions::Apply(DWORD Flags)
{
	CAppMain &App = GetAppClass();

	if ((Flags & UPDATE_RESIDENT) != 0) {
		App.UICore.SetResident(m_fResident);
	}

	if ((Flags & UPDATE_1SEGFALLBACK) != 0) {
		LibISDB::TSPacketParserFilter *pPacketParser =
			App.CoreEngine.GetFilter<LibISDB::TSPacketParserFilter>();
		if (pPacketParser != nullptr)
			pPacketParser->SetGenerate1SegPAT(m_fEnable1SegFallback);
	}

	return true;
}


bool CGeneralOptions::ReadSettings(CSettings &Settings)
{
	int Value;

	TCHAR szDirectory[MAX_PATH];
	if (Settings.Read(TEXT("DriverDirectory"), szDirectory, lengthof(szDirectory))
			&& szDirectory[0] != '\0') {
		m_BonDriverDirectory = szDirectory;
		GetAppClass().CoreEngine.SetDriverDirectory(szDirectory);
	}

	if (Settings.Read(TEXT("DefaultDriverType"), &Value)
			&& CheckEnumRange(static_cast<DefaultDriverType>(Value)))
		m_DefaultDriverType = static_cast<DefaultDriverType>(Value);
	Settings.Read(TEXT("DefaultDriver"), &m_DefaultBonDriverName);
	Settings.Read(TEXT("Driver"), &m_LastBonDriverName);

	Settings.Read(TEXT("Resident"), &m_fResident);
	Settings.Read(TEXT("KeepSingleTask"), &m_fKeepSingleTask);
	Settings.Read(TEXT("StandaloneProgramGuide"), &m_fStandaloneProgramGuide);
	Settings.Read(TEXT("Enable1SegFallback"), &m_fEnable1SegFallback);

	return true;
}


bool CGeneralOptions::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("DriverDirectory"), m_BonDriverDirectory);
	Settings.Write(TEXT("DefaultDriverType"), static_cast<int>(m_DefaultDriverType));
	Settings.Write(TEXT("DefaultDriver"), m_DefaultBonDriverName);
	Settings.Write(TEXT("Driver"), GetAppClass().CoreEngine.GetDriverFileName());
	Settings.Write(TEXT("Resident"), m_fResident);
	Settings.Write(TEXT("KeepSingleTask"), m_fKeepSingleTask);
	Settings.Write(TEXT("StandaloneProgramGuide"), m_fStandaloneProgramGuide);
	Settings.Write(TEXT("Enable1SegFallback"), m_fEnable1SegFallback);

	return true;
}


bool CGeneralOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(
		hwndOwner,
		GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_OPTIONS_GENERAL));
}


CGeneralOptions::DefaultDriverType CGeneralOptions::GetDefaultDriverType() const
{
	return m_DefaultDriverType;
}


LPCTSTR CGeneralOptions::GetDefaultDriverName() const
{
	return m_DefaultBonDriverName.c_str();
}


bool CGeneralOptions::SetDefaultDriverName(LPCTSTR pszDriverName)
{
	if (pszDriverName == nullptr)
		m_DefaultBonDriverName.clear();
	else
		m_DefaultBonDriverName = pszDriverName;
	return true;
}


bool CGeneralOptions::GetFirstDriverName(String *pDriverName) const
{
	switch (m_DefaultDriverType) {
	case DefaultDriverType::None:
		pDriverName->clear();
		break;
	case DefaultDriverType::Last:
		*pDriverName = m_LastBonDriverName;
		break;
	case DefaultDriverType::Custom:
		*pDriverName = m_DefaultBonDriverName;
		break;
	default:
		return false;
	}
	return true;
}


bool CGeneralOptions::GetResident() const
{
	return m_fResident;
}


bool CGeneralOptions::GetKeepSingleTask() const
{
	return m_fKeepSingleTask;
}


INT_PTR CGeneralOptions::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			const CAppMain &App = GetAppClass();

			::SendDlgItemMessage(hDlg, IDC_OPTIONS_DRIVERDIRECTORY, EM_LIMITTEXT, MAX_PATH - 1, 0);
			::SetDlgItemText(hDlg, IDC_OPTIONS_DRIVERDIRECTORY, m_BonDriverDirectory.c_str());

			// BonDriver
			::CheckRadioButton(
				hDlg, IDC_OPTIONS_DEFAULTDRIVER_NONE,
				IDC_OPTIONS_DEFAULTDRIVER_CUSTOM,
				static_cast<int>(m_DefaultDriverType) + IDC_OPTIONS_DEFAULTDRIVER_NONE);
			EnableDlgItems(
				hDlg, IDC_OPTIONS_DEFAULTDRIVER,
				IDC_OPTIONS_DEFAULTDRIVER_BROWSE,
				m_DefaultDriverType == DefaultDriverType::Custom);

			const CDriverManager &DriverManager = App.DriverManager;
			DlgComboBox_LimitText(hDlg, IDC_OPTIONS_DEFAULTDRIVER, MAX_PATH - 1);
			for (int i = 0; i < DriverManager.NumDrivers(); i++) {
				DlgComboBox_AddString(
					hDlg, IDC_OPTIONS_DEFAULTDRIVER,
					DriverManager.GetDriverInfo(i)->GetFileName());
			}
			::SetDlgItemText(hDlg, IDC_OPTIONS_DEFAULTDRIVER, m_DefaultBonDriverName.c_str());

			DlgCheckBox_Check(hDlg, IDC_OPTIONS_RESIDENT, m_fResident);
			DlgCheckBox_Check(hDlg, IDC_OPTIONS_KEEPSINGLETASK, m_fKeepSingleTask);
			DlgCheckBox_Check(hDlg, IDC_OPTIONS_STANDALONEPROGRAMGUIDE, m_fStandaloneProgramGuide);
			DlgCheckBox_Check(hDlg, IDC_OPTIONS_ENABLE1SEGFALLBACK, m_fEnable1SegFallback);

			DlgCheckBox_Check(
				hDlg, IDC_OPTIONS_USEUNIQUEAPPID,
				App.TaskbarOptions.GetUseUniqueAppID());
			DlgCheckBox_Check(
				hDlg, IDC_OPTIONS_ENABLEJUMPLIST,
				App.TaskbarOptions.GetEnableJumpList());
			DlgCheckBox_Check(
				hDlg, IDC_OPTIONS_JUMPLISTKEEPSINGLETASK,
				App.TaskbarOptions.GetJumpListKeepSingleTask());
			EnableDlgItem(
				hDlg, IDC_OPTIONS_JUMPLISTKEEPSINGLETASK,
				App.TaskbarOptions.GetEnableJumpList());

			AddControls({
				{IDC_OPTIONS_DRIVERDIRECTORY,        AlignFlag::Horz},
				{IDC_OPTIONS_DRIVERDIRECTORY_BROWSE, AlignFlag::Right},
			});
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OPTIONS_DRIVERDIRECTORY_BROWSE:
			{
				TCHAR szDirectory[MAX_PATH];

				if (::GetDlgItemText(hDlg, IDC_OPTIONS_DRIVERDIRECTORY, szDirectory, lengthof(szDirectory)) > 0) {
					if (::PathIsRelative(szDirectory)) {
						TCHAR szTemp[MAX_PATH];

						GetAppClass().GetAppDirectory(szTemp);
						::PathAppend(szTemp, szDirectory);
						::PathCanonicalize(szDirectory, szTemp);
					}
				} else {
					GetAppClass().GetAppDirectory(szDirectory);
				}
				if (BrowseFolderDialog(hDlg, szDirectory, TEXT("BonDriver の検索フォルダを選択してください。")))
					::SetDlgItemText(hDlg, IDC_OPTIONS_DRIVERDIRECTORY, szDirectory);
			}
			return TRUE;

		case IDC_OPTIONS_DEFAULTDRIVER_NONE:
		case IDC_OPTIONS_DEFAULTDRIVER_LAST:
		case IDC_OPTIONS_DEFAULTDRIVER_CUSTOM:
			EnableDlgItemsSyncCheckBox(
				hDlg, IDC_OPTIONS_DEFAULTDRIVER,
				IDC_OPTIONS_DEFAULTDRIVER_BROWSE,
				IDC_OPTIONS_DEFAULTDRIVER_CUSTOM);
			return TRUE;

		case IDC_OPTIONS_DEFAULTDRIVER_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH];
				String FileName, InitDir;

				::GetDlgItemText(hDlg, IDC_OPTIONS_DEFAULTDRIVER, szFileName, lengthof(szFileName));
				if (PathUtil::Split(szFileName, &InitDir, &FileName)) {
					StringCopy(szFileName, FileName.c_str());
				} else {
					GetAppClass().GetAppDirectory(&InitDir);
				}
				InitOpenFileName(&ofn);
				ofn.hwndOwner = hDlg;
				ofn.lpstrFilter =
					TEXT("BonDriver(BonDriver*.dll)\0BonDriver*.dll\0")
					TEXT("すべてのファイル\0*.*\0");
				ofn.lpstrFile = szFileName;
				ofn.nMaxFile = lengthof(szFileName);
				ofn.lpstrInitialDir = InitDir.c_str();
				ofn.lpstrTitle = TEXT("BonDriverの選択");
				ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_EXPLORER;
				if (FileOpenDialog(&ofn))
					::SetDlgItemText(hDlg, IDC_OPTIONS_DEFAULTDRIVER, szFileName);
			}
			return TRUE;

		case IDC_OPTIONS_ENABLEJUMPLIST:
			EnableDlgItemSyncCheckBox(
				hDlg,
				IDC_OPTIONS_JUMPLISTKEEPSINGLETASK,
				IDC_OPTIONS_ENABLEJUMPLIST);
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case PSN_APPLY:
			{
				TCHAR szDirectory[MAX_PATH];
				::GetDlgItemText(
					hDlg, IDC_OPTIONS_DRIVERDIRECTORY,
					szDirectory, lengthof(szDirectory));
				m_BonDriverDirectory = szDirectory;

				m_DefaultDriverType = static_cast<DefaultDriverType>(
					GetCheckedRadioButton(
						hDlg,
						IDC_OPTIONS_DEFAULTDRIVER_NONE,
						IDC_OPTIONS_DEFAULTDRIVER_CUSTOM) -
						IDC_OPTIONS_DEFAULTDRIVER_NONE);

				TCHAR szDefaultBonDriver[MAX_PATH];
				::GetDlgItemText(
					hDlg, IDC_OPTIONS_DEFAULTDRIVER,
					szDefaultBonDriver, lengthof(szDefaultBonDriver));
				m_DefaultBonDriverName = szDefaultBonDriver;

				const bool fResident = DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_RESIDENT);
				if (fResident != m_fResident) {
					m_fResident = fResident;
					SetUpdateFlag(UPDATE_RESIDENT);
				}

				m_fKeepSingleTask =
					DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_KEEPSINGLETASK);

				m_fStandaloneProgramGuide =
					DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_STANDALONEPROGRAMGUIDE);

				const bool fEnable1SegFallback =
					DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_ENABLE1SEGFALLBACK);
				if (fEnable1SegFallback != m_fEnable1SegFallback) {
					m_fEnable1SegFallback = fEnable1SegFallback;
					SetUpdateFlag(UPDATE_1SEGFALLBACK);
				}

				CTaskbarOptions &TaskbarOptions = GetAppClass().TaskbarOptions;

				TaskbarOptions.SetUseUniqueAppID(
					DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_USEUNIQUEAPPID));
				TaskbarOptions.SetEnableJumpList(
					DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_ENABLEJUMPLIST));
				TaskbarOptions.SetJumpListKeepSingleTask(
					DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_JUMPLISTKEEPSINGLETASK));
				TaskbarOptions.SetChanged();

				m_fChanged = true;
			}
			return TRUE;
		}
		break;
	}

	return FALSE;
}


}	// namespace TVTest
