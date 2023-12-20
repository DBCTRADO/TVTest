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
#include "TSProcessorOptions.h"
#include "DialogUtil.h"
#include "MessageDialog.h"
#include "Help/HelpID.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CTSProcessorOptions::CTSProcessorOptions(CTSProcessorManager &TSProcessorManager)
	: COptions(TEXT("TSProcessor"))
	, m_TSProcessorManager(TSProcessorManager)
{
}


bool CTSProcessorOptions::ReadSettings(CSettings &Settings)
{
	return m_TSProcessorManager.ReadSettings(Settings);
}


bool CTSProcessorOptions::WriteSettings(CSettings &Settings)
{
	return m_TSProcessorManager.WriteSettings(Settings);
}


bool CTSProcessorOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(
		hwndOwner,
		GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE(IDD_OPTIONS_TSPROCESSOR));
}


INT_PTR CTSProcessorOptions::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			m_pCurSettings = nullptr;

			m_TunerMapListView.Attach(::GetDlgItem(hDlg, IDC_TSPROCESSOR_TUNERMAP));
			m_TunerMapListView.SetExtendedStyle(
				LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
			static const struct {
				LPCTSTR pszText;
				int Format;
			} TunerMapColumns[] = {
				{TEXT("チューナー"),     LVCFMT_LEFT},
				{TEXT("ネットワークID"), LVCFMT_RIGHT},
				{TEXT("TSID"),           LVCFMT_RIGHT},
				{TEXT("サービスID"),     LVCFMT_RIGHT},
				{TEXT("処理"),           LVCFMT_LEFT},
				{TEXT("モジュール"),     LVCFMT_LEFT},
				{TEXT("デバイス"),       LVCFMT_LEFT},
				{TEXT("フィルター"),     LVCFMT_LEFT},
			};
			for (int i = 0; i < lengthof(TunerMapColumns); i++)
				m_TunerMapListView.InsertColumn(i, TunerMapColumns[i].pszText, TunerMapColumns[i].Format);

			std::vector<CTSProcessor*> TSProcessorList;

			if (m_TSProcessorManager.GetTSProcessorList(&TSProcessorList)
					&& !TSProcessorList.empty()) {

				for (const CTSProcessor *pTSProcessor : TSProcessorList) {
					GUID guid;
					String Name;

					if (pTSProcessor->GetGuid(&guid) && pTSProcessor->GetName(&Name)) {
						const CTSProcessorManager::CTSProcessorSettings *pCurSettings =
							m_TSProcessorManager.GetTSProcessorSettings(guid);
						CTSProcessorManager::CTSProcessorSettings *pSettings;

						if (pCurSettings != nullptr) {
							pSettings = new CTSProcessorManager::CTSProcessorSettings(*pCurSettings);
						} else {
							pSettings = new CTSProcessorManager::CTSProcessorSettings(guid);
						}
						const int i = static_cast<int>(DlgComboBox_AddString(hDlg, IDC_TSPROCESSOR_TSPROCESSORLIST, Name.c_str()));
						DlgComboBox_SetItemData(
							hDlg, IDC_TSPROCESSOR_TSPROCESSORLIST, i,
							reinterpret_cast<LPARAM>(pSettings));
						m_SettingsList.emplace_back(pSettings);
					}
				}

				DlgComboBox_SetCurSel(hDlg, IDC_TSPROCESSOR_TSPROCESSORLIST, 0);
			} else {
				DlgComboBox_SetCueBanner(
					hDlg, IDC_TSPROCESSOR_TSPROCESSORLIST,
					TEXT("TSプロセッサーはありません"));
			}

			UpdateCurSettings();

			m_TunerMapListView.AdjustColumnWidth();

			AddControl(IDC_TSPROCESSOR_TUNERMAP, AlignFlag::All);
			AddControls(IDC_TSPROCESSOR_TUNERMAP_ADD, IDC_TSPROCESSOR_TUNERMAP_DOWN, AlignFlag::Right);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_TSPROCESSOR_TSPROCESSORLIST:
			if (HIWORD(wParam) == CBN_SELCHANGE)
				UpdateCurSettings();
			return TRUE;

		case IDC_TSPROCESSOR_PROPERTIES:
			if (m_pCurSettings != nullptr) {
				CTSProcessor *pTSProcessor =
					m_TSProcessorManager.GetTSProcessor(m_pCurSettings->m_guid);
				if (pTSProcessor != nullptr) {
					if (pTSProcessor->ShowPropertyPage(hDlg, GetAppClass().GetResourceInstance()))
						m_TSProcessorManager.SaveTSProcessorProperties(pTSProcessor);
				}
			}
			return TRUE;

		case IDC_TSPROCESSOR_DEFAULTMODULE:
		case IDC_TSPROCESSOR_DEFAULTDEVICE:
			if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == CBN_EDITCHANGE)
				::PostMessage(hDlg, WM_APP_UPDATEDEVICEFILTERLIST, 0, 0);
			return TRUE;

		case IDC_TSPROCESSOR_TUNERMAP_ADD:
			if (m_pCurSettings != nullptr) {
				CTSProcessorManager::TunerFilterInfo Info;

				if (TunerMapDialog(&Info)) {
					m_pCurSettings->m_TunerFilterMap.push_back(Info);
					const int Index = m_TunerMapListView.InsertItem(
						-1, TEXT(""), m_pCurSettings->m_TunerFilterMap.size() - 1);
					const int ItemCount = m_TunerMapListView.GetItemCount();
					for (int i = Index; i < ItemCount; i++)
						UpdateTunerMapItem(i);
					m_TunerMapListView.SetItemState(
						Index,
						LVIS_FOCUSED | LVIS_SELECTED,
						LVIS_FOCUSED | LVIS_SELECTED);
				}
			}
			return TRUE;

		case IDC_TSPROCESSOR_TUNERMAP_EDIT:
			if (m_pCurSettings != nullptr) {
				const int Sel = m_TunerMapListView.GetSelectedItem();

				if (Sel >= 0 && static_cast<size_t>(Sel) < m_pCurSettings->m_TunerFilterMap.size()) {
					CTSProcessorManager::TunerFilterInfo &Info =
						m_pCurSettings->m_TunerFilterMap[Sel];
					if (TunerMapDialog(&Info)) {
						UpdateTunerMapItem(Sel);
					}
				}
			}
			return TRUE;

		case IDC_TSPROCESSOR_TUNERMAP_REMOVE:
			if (m_pCurSettings != nullptr) {
				const int Sel = m_TunerMapListView.GetSelectedItem();

				if (Sel >= 0) {
					m_TunerMapListView.DeleteItem(Sel);
					auto it = m_pCurSettings->m_TunerFilterMap.begin();
					std::advance(it, Sel);
					m_pCurSettings->m_TunerFilterMap.erase(it);
					const int ItemCount = m_TunerMapListView.GetItemCount();
					for (int i = Sel; i < ItemCount; i++)
						UpdateTunerMapItem(i);
				}
			}
			return TRUE;

		case IDC_TSPROCESSOR_TUNERMAP_UP:
		case IDC_TSPROCESSOR_TUNERMAP_DOWN:
			if (m_pCurSettings != nullptr) {
				const int Sel = m_TunerMapListView.GetSelectedItem();
				const int ItemCount = m_TunerMapListView.GetItemCount();
				int To;

				if (LOWORD(wParam) == IDC_TSPROCESSOR_TUNERMAP_UP) {
					if (Sel < 1)
						return TRUE;
					To = Sel - 1;
				} else {
					if (Sel < 0 || Sel + 1 >= ItemCount)
						return TRUE;
					To = Sel + 1;
				}

				std::swap(
					m_pCurSettings->m_TunerFilterMap[Sel],
					m_pCurSettings->m_TunerFilterMap[To]);
				UpdateTunerMapItem(Sel);
				UpdateTunerMapItem(To);
				m_TunerMapListView.SetItemState(
					To,
					LVIS_FOCUSED | LVIS_SELECTED,
					LVIS_FOCUSED | LVIS_SELECTED);
			}
			return TRUE;
		}
		return FALSE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case PSN_APPLY:
			{
				UpdateCurSettings();

				for (auto &e : m_SettingsList) {
					const GUID guid = e->m_guid;
					m_TSProcessorManager.SetTSProcessorSettings(e.release());
					m_TSProcessorManager.ApplyTSProcessorSettings(guid, false);
				}
				m_SettingsList.clear();

				m_fChanged = true;
			}
			return TRUE;

		case LVN_ITEMCHANGED:
			if (m_pCurSettings != nullptr) {
				const NMLISTVIEW *pnmlv = reinterpret_cast<const NMLISTVIEW*>(lParam);

				if ((pnmlv->uChanged & LVIF_STATE) == 0)
					return TRUE;

				const bool fCheckStateChanged =
					(pnmlv->uOldState & LVIS_STATEIMAGEMASK) != 0
					&& ((pnmlv->uOldState ^ pnmlv->uNewState) & LVIS_STATEIMAGEMASK) != 0;
				const bool fSelectStateChanged =
					((pnmlv->uOldState ^ pnmlv->uNewState) & LVIS_SELECTED) != 0;

				if (pnmlv->hdr.hwndFrom == ::GetDlgItem(hDlg, IDC_TSPROCESSOR_TUNERMAP)) {
					if (fCheckStateChanged) {
						if (pnmlv->iItem >= 0 && static_cast<size_t>(pnmlv->iItem) < m_pCurSettings->m_TunerFilterMap.size()) {
							m_pCurSettings->m_TunerFilterMap[pnmlv->iItem].fEnable =
								(pnmlv->uNewState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK(2);
						}
					}
					if (fSelectStateChanged) {
						const int Sel = m_TunerMapListView.GetSelectedItem();
						EnableDlgItems(
							hDlg,
							IDC_TSPROCESSOR_TUNERMAP_EDIT,
							IDC_TSPROCESSOR_TUNERMAP_REMOVE,
							Sel >= 0);
						EnableDlgItem(hDlg, IDC_TSPROCESSOR_TUNERMAP_UP, Sel > 0);
						EnableDlgItem(
							hDlg, IDC_TSPROCESSOR_TUNERMAP_DOWN,
							Sel >= 0 && Sel + 1 < m_TunerMapListView.GetItemCount());
					}
				}
			}
			return TRUE;

		case NM_DBLCLK:
			{
				const NMITEMACTIVATE *pnmia = reinterpret_cast<const NMITEMACTIVATE*>(lParam);

				if (pnmia->hdr.hwndFrom == ::GetDlgItem(hDlg, IDC_TSPROCESSOR_TUNERMAP)) {
					::SendMessage(hDlg, WM_COMMAND, IDC_TSPROCESSOR_TUNERMAP_EDIT, 0);
				}
			}
			return TRUE;

		case NM_RCLICK:
			{
				const NMITEMACTIVATE *pnmia = reinterpret_cast<const NMITEMACTIVATE*>(lParam);

				if (pnmia->hdr.hwndFrom == ::GetDlgItem(hDlg, IDC_TSPROCESSOR_TUNERMAP)) {
					if (m_TunerMapListView.GetSelectedItem() >= 0) {
						static const int MenuIDs[] = {
							IDC_TSPROCESSOR_TUNERMAP_EDIT,
							0,
							IDC_TSPROCESSOR_TUNERMAP_UP,
							IDC_TSPROCESSOR_TUNERMAP_DOWN,
							0,
							IDC_TSPROCESSOR_TUNERMAP_REMOVE,
						};
						PopupMenuFromControls(hDlg, MenuIDs, lengthof(MenuIDs), TPM_RIGHTBUTTON);
					}
				}
			}
			return TRUE;
		}
		break;

	case WM_DESTROY:
		m_SettingsList.clear();
		m_pCurSettings = nullptr;
		m_ModuleList.clear();
		m_ModuleInfoMap.clear();
		m_TunerMapListView.Detach();
		return TRUE;

	case WM_APP_UPDATEDEVICEFILTERLIST:
		UpdateDeviceFilterList(
			hDlg,
			IDC_TSPROCESSOR_DEFAULTMODULE,
			IDC_TSPROCESSOR_DEFAULTDEVICE,
			IDC_TSPROCESSOR_DEFAULTFILTER);
		return TRUE;
	}

	return FALSE;
}


const CTSProcessorOptions::ModuleInfo *CTSProcessorOptions::GetModuleInfo(const String &Name)
{
	auto it = m_ModuleInfoMap.find(Name);

	if (it != m_ModuleInfoMap.end())
		return &it->second;

	if (m_pCurSettings != nullptr) {
		const CTSProcessor *pTSProcessor = m_TSProcessorManager.GetTSProcessor(m_pCurSettings->m_guid);

		if (pTSProcessor != nullptr) {
			ModuleInfo Module;

			if (!Name.empty()) {
				if (!pTSProcessor->GetModuleInfo(Name.c_str(), &Module))
					return nullptr;
			} else if (!pTSProcessor->IsFilterModuleSupported()) {
				const int DeviceCount = pTSProcessor->GetDeviceCount();
				for (int i = 0; i < DeviceCount; i++) {
					CTSProcessor::ModuleDeviceInfo Device;
					pTSProcessor->GetDeviceFilterList(i, &Device.FilterList);
					Module.DeviceList.push_back(Device);
				}
			} else {
				return nullptr;
			}

			auto Result = m_ModuleInfoMap.emplace(Name, Module);
			return &Result.first->second;
		}
	}

	return nullptr;
}


void CTSProcessorOptions::UpdateCurSettings()
{
	if (m_pCurSettings != nullptr) {
		m_pCurSettings->m_EnableProcessing =
			DlgCheckBox_IsChecked(m_hDlg, IDC_TSPROCESSOR_ENABLEPROCESSING);
		GetDlgItemString(m_hDlg, IDC_TSPROCESSOR_DEFAULTMODULE, &m_pCurSettings->m_DefaultFilter.Module);
		GetDlgItemString(m_hDlg, IDC_TSPROCESSOR_DEFAULTDEVICE, &m_pCurSettings->m_DefaultFilter.Device);
		GetDlgItemString(m_hDlg, IDC_TSPROCESSOR_DEFAULTFILTER, &m_pCurSettings->m_DefaultFilter.Filter);
	}

	const int Sel = static_cast<int>(DlgComboBox_GetCurSel(m_hDlg, IDC_TSPROCESSOR_TSPROCESSORLIST));

	if (Sel >= 0) {
		CTSProcessorManager::CTSProcessorSettings *pSettings =
			reinterpret_cast<CTSProcessorManager::CTSProcessorSettings*>(
				DlgComboBox_GetItemData(m_hDlg, IDC_TSPROCESSOR_TSPROCESSORLIST, Sel));
		if (m_pCurSettings == pSettings)
			return;
		m_pCurSettings = pSettings;
	} else {
		m_pCurSettings = nullptr;
	}

	m_ModuleList.clear();
	m_ModuleInfoMap.clear();

	UpdateItemsState();
}


void CTSProcessorOptions::UpdateItemsState()
{
	m_TunerMapListView.DeleteAllItems();
	EnableDlgItems(
		m_hDlg,
		IDC_TSPROCESSOR_SETTINGSFIRST,
		IDC_TSPROCESSOR_SETTINGSLAST,
		m_pCurSettings != nullptr);

	if (m_pCurSettings != nullptr) {
		const CTSProcessor *pTSProcessor = m_TSProcessorManager.GetTSProcessor(m_pCurSettings->m_guid);
		bool f;

		EnableDlgItem(
			m_hDlg, IDC_TSPROCESSOR_PROPERTIES,
			pTSProcessor->IsPropertyPageSupported());

		if (pTSProcessor->GetEnableProcessing(&f)) {
			DlgCheckBox_Check(
				m_hDlg, IDC_TSPROCESSOR_ENABLEPROCESSING,
				m_pCurSettings->m_EnableProcessing ?
				m_pCurSettings->m_EnableProcessing.value() : f);
		} else {
			DlgCheckBox_Check(m_hDlg, IDC_TSPROCESSOR_ENABLEPROCESSING, false);
			EnableDlgItem(m_hDlg, IDC_TSPROCESSOR_ENABLEPROCESSING, false);
		}

		DlgComboBox_Clear(m_hDlg, IDC_TSPROCESSOR_DEFAULTMODULE);
		pTSProcessor->GetModuleList(&m_ModuleList);
		for (const String &e : m_ModuleList)
			DlgComboBox_AddString(m_hDlg, IDC_TSPROCESSOR_DEFAULTMODULE, e.c_str());
		if (pTSProcessor->IsFilterModuleSupported()) {
			::SetDlgItemText(
				m_hDlg, IDC_TSPROCESSOR_DEFAULTMODULE,
				m_pCurSettings->m_DefaultFilter.Module.c_str());
		} else {
			::SetDlgItemText(m_hDlg, IDC_TSPROCESSOR_DEFAULTMODULE, TEXT(""));
			EnableDlgItems(
				m_hDlg,
				IDC_TSPROCESSOR_DEFAULTMODULE_LABEL,
				IDC_TSPROCESSOR_DEFAULTMODULE,
				false);
		}
		::SetDlgItemText(
			m_hDlg, IDC_TSPROCESSOR_DEFAULTDEVICE,
			m_pCurSettings->m_DefaultFilter.Device.c_str());
		::SetDlgItemText(
			m_hDlg, IDC_TSPROCESSOR_DEFAULTFILTER,
			m_pCurSettings->m_DefaultFilter.Filter.c_str());
		UpdateDeviceFilterList(
			m_hDlg,
			IDC_TSPROCESSOR_DEFAULTMODULE,
			IDC_TSPROCESSOR_DEFAULTDEVICE,
			IDC_TSPROCESSOR_DEFAULTFILTER);

		for (int i = 0; i < static_cast<int>(m_pCurSettings->m_TunerFilterMap.size()); i++) {
			m_TunerMapListView.InsertItem(i, TEXT(""));
			UpdateTunerMapItem(i);
		}

		EnableDlgItems(
			m_hDlg,
			IDC_TSPROCESSOR_TUNERMAP_EDIT,
			IDC_TSPROCESSOR_TUNERMAP_DOWN,
			false);
	} else {
		m_ModuleList.clear();
		m_ModuleInfoMap.clear();
		DlgCheckBox_Check(m_hDlg, IDC_TSPROCESSOR_ENABLEPROCESSING, false);
		::SetDlgItemText(m_hDlg, IDC_TSPROCESSOR_DEFAULTMODULE, TEXT(""));
		::SetDlgItemText(m_hDlg, IDC_TSPROCESSOR_DEFAULTDEVICE, TEXT(""));
		::SetDlgItemText(m_hDlg, IDC_TSPROCESSOR_DEFAULTFILTER, TEXT(""));
	}
}


void CTSProcessorOptions::UpdateDeviceFilterList(HWND hDlg, int ModuleID, int DeviceID, int FilterID)
{
	String ModuleName, DeviceName, FilterName;
	DWORD DeviceEditSel, FilterEditSel;

	GetDlgItemString(hDlg, ModuleID, &ModuleName);
	GetDlgItemString(hDlg, DeviceID, &DeviceName);
	GetDlgItemString(hDlg, FilterID, &FilterName);
	DeviceEditSel = static_cast<DWORD>(::SendDlgItemMessage(hDlg, DeviceID, CB_GETEDITSEL, 0, 0));
	FilterEditSel = static_cast<DWORD>(::SendDlgItemMessage(hDlg, FilterID, CB_GETEDITSEL, 0, 0));
	DlgComboBox_Clear(hDlg, DeviceID);
	DlgComboBox_Clear(hDlg, FilterID);

	const ModuleInfo *pModuleInfo = GetModuleInfo(ModuleName);

	if (pModuleInfo != nullptr) {
		int DeviceSel = -1, FilterSel = -1;

		for (const auto &Dev : pModuleInfo->DeviceList) {
			DlgComboBox_AddString(hDlg, DeviceID, Dev.Name.c_str());
			if (StringUtility::IsEqualNoCase(DeviceName, Dev.Name)) {
				for (const String &Filter : Dev.FilterList) {
					DlgComboBox_AddString(hDlg, FilterID, Filter.c_str());
					if (StringUtility::IsEqualNoCase(FilterName, Filter))
						FilterSel = static_cast<int>(DlgComboBox_GetCount(hDlg, FilterID)) - 1;
				}
				DeviceSel = static_cast<int>(DlgComboBox_GetCount(hDlg, DeviceID)) - 1;
			}
		}

		if (DeviceSel >= 0) {
			DlgComboBox_SetCurSel(hDlg, DeviceID, DeviceSel);
			if (FilterSel >= 0) {
				DlgComboBox_SetCurSel(hDlg, FilterID, FilterSel);
			}
		}
	}

	::SetDlgItemText(hDlg, DeviceID, DeviceName.c_str());
	::SendDlgItemMessage(hDlg, DeviceID, CB_SETEDITSEL, 0, DeviceEditSel);
	::SetDlgItemText(hDlg, FilterID, FilterName.c_str());
	::SendDlgItemMessage(hDlg, FilterID, CB_SETEDITSEL, 0, FilterEditSel);
}


void CTSProcessorOptions::UpdateTunerMapItem(int Index)
{
	const CTSProcessorManager::TunerFilterInfo &Settings =
		m_pCurSettings->m_TunerFilterMap[Index];
	TCHAR szText[32];

	m_TunerMapListView.CheckItem(Index, Settings.fEnable);
	m_TunerMapListView.SetItemText(
		Index, 0, !Settings.Tuner.empty() ? Settings.Tuner.c_str() : TEXT("(指定なし)"));
	if (Settings.IsNetworkIDEnabled())
		StringFormat(szText, TEXT("{}"), Settings.NetworkID);
	else
		StringCopy(szText, TEXT("(指定なし)"));
	m_TunerMapListView.SetItemText(Index, 1, szText);
	if (Settings.IsTransportStreamIDEnabled())
		StringFormat(szText, TEXT("{}"), Settings.TransportStreamID);
	else
		StringCopy(szText, TEXT("(指定なし)"));
	m_TunerMapListView.SetItemText(Index, 2, szText);
	if (Settings.IsServiceIDEnabled())
		StringFormat(szText, TEXT("{}"), Settings.ServiceID);
	else
		StringCopy(szText, TEXT("(指定なし)"));
	m_TunerMapListView.SetItemText(Index, 3, szText);
	m_TunerMapListView.SetItemText(Index, 4, Settings.fEnableProcessing ? TEXT("有効") : TEXT("無効"));
	m_TunerMapListView.SetItemText(Index, 5, Settings.Module.c_str());
	m_TunerMapListView.SetItemText(Index, 6, Settings.Device.c_str());
	m_TunerMapListView.SetItemText(Index, 7, Settings.Filter.c_str());
}


bool CTSProcessorOptions::TunerMapDialog(CTSProcessorManager::TunerFilterInfo *pInfo)
{
	CTunerMapDialog Dialog(this, pInfo);

	return Dialog.Show(m_hDlg);
}




CTSProcessorOptions::CTunerMapDialog::CTunerMapDialog(
	CTSProcessorOptions *pOptions,
	CTSProcessorManager::TunerFilterInfo *pInfo)
	: m_pOptions(pOptions)
	, m_pInfo(pInfo)
{
}


bool CTSProcessorOptions::CTunerMapDialog::Show(HWND hwndOwner)
{
	return ShowDialog(
		hwndOwner,
		GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE(IDD_TSPROCESSOR_TUNERMAP)) == IDOK;
}


INT_PTR CTSProcessorOptions::CTunerMapDialog::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			const CDriverManager &DriverManager = GetAppClass().DriverManager;

			const int NumDrivers = DriverManager.NumDrivers();
			for (int i = 0; i < NumDrivers; i++) {
				DlgComboBox_AddString(
					hDlg, IDC_TSPROCESSORTUNERMAP_TUNER,
					DriverManager.GetDriverInfo(i)->GetFileName());
			}
			::SetDlgItemText(hDlg, IDC_TSPROCESSORTUNERMAP_TUNER, m_pInfo->Tuner.c_str());
			if (m_pInfo->IsNetworkIDEnabled())
				::SetDlgItemInt(hDlg, IDC_TSPROCESSORTUNERMAP_NETWORKID, m_pInfo->NetworkID, FALSE);
			if (m_pInfo->IsTransportStreamIDEnabled())
				::SetDlgItemInt(hDlg, IDC_TSPROCESSORTUNERMAP_TSID, m_pInfo->TransportStreamID, FALSE);
			if (m_pInfo->IsServiceIDEnabled())
				::SetDlgItemInt(hDlg, IDC_TSPROCESSORTUNERMAP_SERVICEID, m_pInfo->ServiceID, FALSE);
			::CheckRadioButton(
				hDlg,
				IDC_TSPROCESSORTUNERMAP_ENABLEPROCESSING,
				IDC_TSPROCESSORTUNERMAP_DISABLEPROCESSING,
				m_pInfo->fEnableProcessing ?
				IDC_TSPROCESSORTUNERMAP_ENABLEPROCESSING :
				IDC_TSPROCESSORTUNERMAP_DISABLEPROCESSING);
			for (const String &e : m_pOptions->m_ModuleList)
				DlgComboBox_AddString(hDlg, IDC_TSPROCESSORTUNERMAP_MODULE, e.c_str());
			::SetDlgItemText(hDlg, IDC_TSPROCESSORTUNERMAP_MODULE, m_pInfo->Module.c_str());
			::SetDlgItemText(hDlg, IDC_TSPROCESSORTUNERMAP_DEVICE, m_pInfo->Device.c_str());
			::SetDlgItemText(hDlg, IDC_TSPROCESSORTUNERMAP_FILTER, m_pInfo->Filter.c_str());
			EnableDlgItems(
				hDlg,
				IDC_TSPROCESSORTUNERMAP_MODULE_LABEL,
				IDC_TSPROCESSORTUNERMAP_FILTER,
				m_pInfo->fEnableProcessing);
			m_pOptions->UpdateDeviceFilterList(
				hDlg,
				IDC_TSPROCESSORTUNERMAP_MODULE,
				IDC_TSPROCESSORTUNERMAP_DEVICE,
				IDC_TSPROCESSORTUNERMAP_FILTER);
			AdjustDialogPos(::GetParent(hDlg), hDlg);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_TSPROCESSORTUNERMAP_ENABLEPROCESSING:
		case IDC_TSPROCESSORTUNERMAP_DISABLEPROCESSING:
			EnableDlgItemsSyncCheckBox(
				hDlg,
				IDC_TSPROCESSORTUNERMAP_MODULE_LABEL,
				IDC_TSPROCESSORTUNERMAP_FILTER,
				IDC_TSPROCESSORTUNERMAP_ENABLEPROCESSING);
			return TRUE;

		case IDC_TSPROCESSORTUNERMAP_MODULE:
		case IDC_TSPROCESSORTUNERMAP_DEVICE:
			if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == CBN_EDITCHANGE)
				::PostMessage(hDlg, WM_APP_UPDATEDEVICEFILTERLIST, 0, 0);
			return TRUE;

		case IDC_TSPROCESSORTUNERMAP_HELP:
			GetAppClass().UICore.ShowHelpContent(HELP_ID_TSPROCESSOR_TUNERMAP);
			return TRUE;

		case IDOK:
			{
				String Buffer;

				GetDlgItemString(hDlg, IDC_TSPROCESSORTUNERMAP_TUNER, &m_pInfo->Tuner);
				if (GetDlgItemString(hDlg, IDC_TSPROCESSORTUNERMAP_NETWORKID, &Buffer)) {
					try {
						m_pInfo->NetworkID = static_cast<WORD>(std::stoi(Buffer, nullptr, 0));
					} catch (...) {
						m_pInfo->NetworkID = CTSProcessorManager::TunerFilterInfo::NID_INVALID;
					}
				}
				if (GetDlgItemString(hDlg, IDC_TSPROCESSORTUNERMAP_TSID, &Buffer)) {
					try {
						m_pInfo->TransportStreamID = static_cast<WORD>(std::stoi(Buffer, nullptr, 0));
					} catch (...) {
						m_pInfo->TransportStreamID = CTSProcessorManager::TunerFilterInfo::TSID_INVALID;
					}
				}
				if (GetDlgItemString(hDlg, IDC_TSPROCESSORTUNERMAP_SERVICEID, &Buffer)) {
					try {
						m_pInfo->ServiceID = static_cast<WORD>(std::stoi(Buffer, nullptr, 0));
					} catch (...) {
						m_pInfo->ServiceID = CTSProcessorManager::TunerFilterInfo::SID_INVALID;
					}
				}
				m_pInfo->fEnableProcessing =
					DlgRadioButton_IsChecked(hDlg, IDC_TSPROCESSORTUNERMAP_ENABLEPROCESSING);
				GetDlgItemString(hDlg, IDC_TSPROCESSORTUNERMAP_MODULE, &m_pInfo->Module);
				GetDlgItemString(hDlg, IDC_TSPROCESSORTUNERMAP_DEVICE, &m_pInfo->Device);
				GetDlgItemString(hDlg, IDC_TSPROCESSORTUNERMAP_FILTER, &m_pInfo->Filter);
			}
			[[fallthrough]];
		case IDCANCEL:
			::EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_APP_UPDATEDEVICEFILTERLIST:
		m_pOptions->UpdateDeviceFilterList(
			hDlg,
			IDC_TSPROCESSORTUNERMAP_MODULE,
			IDC_TSPROCESSORTUNERMAP_DEVICE,
			IDC_TSPROCESSORTUNERMAP_FILTER);
		return TRUE;
	}

	return FALSE;
}


}	// namespace TVTest
