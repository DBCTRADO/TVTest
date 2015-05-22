#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "GeneralOptions.h"
#include "DriverManager.h"
#include "DialogUtil.h"
#include "MessageDialog.h"
#include "resource.h"
#include "Common/DebugDef.h"




CGeneralOptions::CGeneralOptions()
	: m_DefaultDriverType(DEFAULT_DRIVER_LAST)
	, m_fResident(false)
	, m_fKeepSingleTask(false)
	, m_fStandaloneProgramGuide(false)
	, m_fEnable1SegFallback(true)
{
}


CGeneralOptions::~CGeneralOptions()
{
	Destroy();
}


bool CGeneralOptions::Apply(DWORD Flags)
{
	CAppMain &App=GetAppClass();

	if ((Flags & UPDATE_RESIDENT)!=0) {
		App.UICore.SetResident(m_fResident);
	}

	if ((Flags & UPDATE_1SEGFALLBACK)!=0) {
		App.CoreEngine.m_DtvEngine.m_TsPacketParser.EnablePATGeneration(m_fEnable1SegFallback);
	}

	return true;
}


bool CGeneralOptions::ReadSettings(CSettings &Settings)
{
	int Value;

	TCHAR szDirectory[MAX_PATH];
	if (Settings.Read(TEXT("DriverDirectory"),szDirectory,lengthof(szDirectory))
			&& szDirectory[0]!='\0') {
		m_BonDriverDirectory=szDirectory;
		GetAppClass().CoreEngine.SetDriverDirectory(szDirectory);
	}

	if (Settings.Read(TEXT("DefaultDriverType"),&Value)
			&& Value>=DEFAULT_DRIVER_NONE && Value<=DEFAULT_DRIVER_CUSTOM)
		m_DefaultDriverType=(DefaultDriverType)Value;
	Settings.Read(TEXT("DefaultDriver"),&m_DefaultBonDriverName);
	Settings.Read(TEXT("Driver"),&m_LastBonDriverName);

	Settings.Read(TEXT("Resident"),&m_fResident);
	Settings.Read(TEXT("KeepSingleTask"),&m_fKeepSingleTask);
	Settings.Read(TEXT("StandaloneProgramGuide"),&m_fStandaloneProgramGuide);
	Settings.Read(TEXT("Enable1SegFallback"),&m_fEnable1SegFallback);

	return true;
}


bool CGeneralOptions::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("DriverDirectory"),m_BonDriverDirectory);
	Settings.Write(TEXT("DefaultDriverType"),(int)m_DefaultDriverType);
	Settings.Write(TEXT("DefaultDriver"),m_DefaultBonDriverName);
	Settings.Write(TEXT("Driver"),GetAppClass().CoreEngine.GetDriverFileName());
	Settings.Write(TEXT("Resident"),m_fResident);
	Settings.Write(TEXT("KeepSingleTask"),m_fKeepSingleTask);
	Settings.Write(TEXT("StandaloneProgramGuide"),m_fStandaloneProgramGuide);
	Settings.Write(TEXT("Enable1SegFallback"),m_fEnable1SegFallback);

	return true;
}


bool CGeneralOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_GENERAL));
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
	if (pszDriverName==NULL)
		m_DefaultBonDriverName.clear();
	else
		m_DefaultBonDriverName=pszDriverName;
	return true;
}


bool CGeneralOptions::GetFirstDriverName(LPTSTR pszDriverName) const
{
	switch (m_DefaultDriverType) {
	case DEFAULT_DRIVER_NONE:
		pszDriverName[0]='\0';
		break;
	case DEFAULT_DRIVER_LAST:
		if (m_LastBonDriverName.length()>=MAX_PATH)
			return false;
		::lstrcpy(pszDriverName,m_LastBonDriverName.c_str());
		break;
	case DEFAULT_DRIVER_CUSTOM:
		if (m_DefaultBonDriverName.length()>=MAX_PATH)
			return false;
		::lstrcpy(pszDriverName,m_DefaultBonDriverName.c_str());
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


INT_PTR CGeneralOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CAppMain &App=GetAppClass();

			::SendDlgItemMessage(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,EM_LIMITTEXT,MAX_PATH-1,0);
			::SetDlgItemText(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,m_BonDriverDirectory.c_str());

			// BonDriver
			::CheckRadioButton(hDlg,IDC_OPTIONS_DEFAULTDRIVER_NONE,
									IDC_OPTIONS_DEFAULTDRIVER_CUSTOM,
							   (int)m_DefaultDriverType+IDC_OPTIONS_DEFAULTDRIVER_NONE);
			EnableDlgItems(hDlg,IDC_OPTIONS_DEFAULTDRIVER,
								IDC_OPTIONS_DEFAULTDRIVER_BROWSE,
						   m_DefaultDriverType==DEFAULT_DRIVER_CUSTOM);

			const CDriverManager &DriverManager=App.DriverManager;
			DlgComboBox_LimitText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,MAX_PATH-1);
			for (int i=0;i<DriverManager.NumDrivers();i++) {
				DlgComboBox_AddString(hDlg,IDC_OPTIONS_DEFAULTDRIVER,
									  DriverManager.GetDriverInfo(i)->GetFileName());
			}
			::SetDlgItemText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,m_DefaultBonDriverName.c_str());

			DlgCheckBox_Check(hDlg,IDC_OPTIONS_RESIDENT,m_fResident);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_KEEPSINGLETASK,m_fKeepSingleTask);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_STANDALONEPROGRAMGUIDE,m_fStandaloneProgramGuide);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_ENABLE1SEGFALLBACK,m_fEnable1SegFallback);

			if (Util::OS::IsWindows7OrLater()) {
				DlgCheckBox_Check(hDlg,IDC_OPTIONS_USEUNIQUEAPPID,
								  App.TaskbarOptions.GetUseUniqueAppID());
				DlgCheckBox_Check(hDlg,IDC_OPTIONS_ENABLEJUMPLIST,
								  App.TaskbarOptions.GetEnableJumpList());
				DlgCheckBox_Check(hDlg,IDC_OPTIONS_JUMPLISTKEEPSINGLETASK,
								  App.TaskbarOptions.GetJumpListKeepSingleTask());
				EnableDlgItem(hDlg,IDC_OPTIONS_JUMPLISTKEEPSINGLETASK,
							  App.TaskbarOptions.GetEnableJumpList());
			} else {
				EnableDlgItems(hDlg,
							   IDC_OPTIONS_ENABLEJUMPLIST,
							   IDC_OPTIONS_USEUNIQUEAPPID,
							   false);
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OPTIONS_DRIVERDIRECTORY_BROWSE:
			{
				TCHAR szDirectory[MAX_PATH];

				if (::GetDlgItemText(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,szDirectory,lengthof(szDirectory))>0) {
					if (::PathIsRelative(szDirectory)) {
						TCHAR szTemp[MAX_PATH];

						GetAppClass().GetAppDirectory(szTemp);
						::PathAppend(szTemp,szDirectory);
						::PathCanonicalize(szDirectory,szTemp);
					}
				} else {
					GetAppClass().GetAppDirectory(szDirectory);
				}
				if (BrowseFolderDialog(hDlg,szDirectory,TEXT("BonDriver の検索フォルダを選択してください。")))
					::SetDlgItemText(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,szDirectory);
			}
			return TRUE;

		case IDC_OPTIONS_DEFAULTDRIVER_NONE:
		case IDC_OPTIONS_DEFAULTDRIVER_LAST:
		case IDC_OPTIONS_DEFAULTDRIVER_CUSTOM:
			EnableDlgItemsSyncCheckBox(hDlg,IDC_OPTIONS_DEFAULTDRIVER,
									   IDC_OPTIONS_DEFAULTDRIVER_BROWSE,
									   IDC_OPTIONS_DEFAULTDRIVER_CUSTOM);
			return TRUE;

		case IDC_OPTIONS_DEFAULTDRIVER_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH],szInitDir[MAX_PATH];
				CFilePath FilePath;

				::GetDlgItemText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,szFileName,lengthof(szFileName));
				FilePath.SetPath(szFileName);
				if (FilePath.GetDirectory(szInitDir)) {
					::lstrcpy(szFileName,FilePath.GetFileName());
				} else {
					GetAppClass().GetAppDirectory(szInitDir);
				}
				InitOpenFileName(&ofn);
				ofn.hwndOwner=hDlg;
				ofn.lpstrFilter=
					TEXT("BonDriver(BonDriver*.dll)\0BonDriver*.dll\0")
					TEXT("すべてのファイル\0*.*\0");
				ofn.lpstrFile=szFileName;
				ofn.nMaxFile=lengthof(szFileName);
				ofn.lpstrInitialDir=szInitDir;
				ofn.lpstrTitle=TEXT("BonDriverの選択");
				ofn.Flags=OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_EXPLORER;
				if (::GetOpenFileName(&ofn))
					::SetDlgItemText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,szFileName);
			}
			return TRUE;

		case IDC_OPTIONS_ENABLEJUMPLIST:
			EnableDlgItemSyncCheckBox(hDlg,
									  IDC_OPTIONS_JUMPLISTKEEPSINGLETASK,
									  IDC_OPTIONS_ENABLEJUMPLIST);
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				TCHAR szDirectory[MAX_PATH];
				::GetDlgItemText(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,
								 szDirectory,lengthof(szDirectory));
				m_BonDriverDirectory=szDirectory;

				m_DefaultDriverType=(DefaultDriverType)
					(GetCheckedRadioButton(hDlg,IDC_OPTIONS_DEFAULTDRIVER_NONE,
										   IDC_OPTIONS_DEFAULTDRIVER_CUSTOM)-
					IDC_OPTIONS_DEFAULTDRIVER_NONE);

				TCHAR szDefaultBonDriver[MAX_PATH];
				::GetDlgItemText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,
								 szDefaultBonDriver,lengthof(szDefaultBonDriver));
				m_DefaultBonDriverName=szDefaultBonDriver;

				bool fResident=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_RESIDENT);
				if (fResident!=m_fResident) {
					m_fResident=fResident;
					SetUpdateFlag(UPDATE_RESIDENT);
				}

				m_fKeepSingleTask=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_KEEPSINGLETASK);

				m_fStandaloneProgramGuide=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_STANDALONEPROGRAMGUIDE);

				bool fEnable1SegFallback=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_ENABLE1SEGFALLBACK);
				if (fEnable1SegFallback!=m_fEnable1SegFallback) {
					m_fEnable1SegFallback=fEnable1SegFallback;
					SetUpdateFlag(UPDATE_1SEGFALLBACK);
				}

				if (Util::OS::IsWindows7OrLater()) {
					CTaskbarOptions &TaskbarOptions=GetAppClass().TaskbarOptions;

					TaskbarOptions.SetUseUniqueAppID(
						DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_USEUNIQUEAPPID));
					TaskbarOptions.SetEnableJumpList(
						DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_ENABLEJUMPLIST));
					TaskbarOptions.SetJumpListKeepSingleTask(
						DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_JUMPLISTKEEPSINGLETASK));
					TaskbarOptions.SetChanged();
				}

				m_fChanged=true;
			}
			return TRUE;
		}
		break;
	}

	return FALSE;
}
