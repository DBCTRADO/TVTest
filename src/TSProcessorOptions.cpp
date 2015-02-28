#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "TSProcessorOptions.h"
#include "DialogUtil.h"
#include "MessageDialog.h"
#include "Help/HelpID.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


namespace TVTest
{


CTSProcessorOptions::CTSProcessorOptions(CTSProcessorManager &TSProcessorManager)
	: COptions(TEXT("TSProcessor"))
	, m_TSProcessorManager(TSProcessorManager)
	, m_pCurSettings(nullptr)
{
}


CTSProcessorOptions::~CTSProcessorOptions()
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
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),
							  MAKEINTRESOURCE(IDD_OPTIONS_TSPROCESSOR));
}


INT_PTR CTSProcessorOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			m_pCurSettings=nullptr;

			m_TunerMapListView.Attach(::GetDlgItem(hDlg,IDC_TSPROCESSOR_TUNERMAP));
			m_TunerMapListView.SetExtendedStyle(
				LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
			static const LPCTSTR TunerMapColumns[] = {
				TEXT("�`���[�i�["),
				TEXT("�L��/����"),
				TEXT("���W���[��"),
				TEXT("�f�o�C�X"),
				TEXT("�t�B���^�["),
			};
			for (int i=0;i<lengthof(TunerMapColumns);i++)
				m_TunerMapListView.InsertColumn(i,TunerMapColumns[i]);

			m_NetworkMapListView.Attach(::GetDlgItem(hDlg,IDC_TSPROCESSOR_NETWORKMAP));
			m_NetworkMapListView.SetExtendedStyle(
				LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
			static const LPCTSTR NetworkMapColumns[] = {
				TEXT("�l�b�g���[�NID"),
				TEXT("TSID"),
				TEXT("�L��/����"),
				TEXT("���W���[��"),
				TEXT("�f�o�C�X"),
				TEXT("�t�B���^�["),
			};
			for (int i=0;i<lengthof(NetworkMapColumns);i++)
				m_NetworkMapListView.InsertColumn(i,NetworkMapColumns[i]);

			std::vector<CTSProcessor*> TSProcessorList;

			if (m_TSProcessorManager.GetTSProcessorList(&TSProcessorList)
					&& !TSProcessorList.empty()) {

				for (auto it=TSProcessorList.begin();it!=TSProcessorList.end();++it) {
					CTSProcessor *pTSProcessor=*it;
					GUID guid;
					String Name;

					if (pTSProcessor->GetGuid(&guid) && pTSProcessor->GetName(&Name)) {
						const CTSProcessorManager::CTSProcessorSettings *pCurSettings=
							m_TSProcessorManager.GetTSProcessorSettings(guid);
						CTSProcessorManager::CTSProcessorSettings *pSettings;

						if (pCurSettings!=nullptr) {
							pSettings=new CTSProcessorManager::CTSProcessorSettings(*pCurSettings);
						} else {
							pSettings=new CTSProcessorManager::CTSProcessorSettings(guid);
						}
						int i=(int)DlgComboBox_AddString(hDlg,IDC_TSPROCESSOR_TSPROCESSORLIST,Name.c_str());
						DlgComboBox_SetItemData(hDlg,IDC_TSPROCESSOR_TSPROCESSORLIST,i,
												reinterpret_cast<LPARAM>(pSettings));
						m_SettingsList.push_back(pSettings);
					}
				}

				DlgComboBox_SetCurSel(hDlg,IDC_TSPROCESSOR_TSPROCESSORLIST,0);
			} else {
#ifdef WIN_XP_SUPPORT
				if (Util::OS::IsWindowsVistaOrLater())
#endif
					DlgComboBox_SetCueBanner(hDlg,IDC_TSPROCESSOR_TSPROCESSORLIST,
											 TEXT("TS�v���Z�b�T�[�͂���܂���"));
			}

			UpdateCurSettings();

			m_TunerMapListView.AdjustColumnWidth();
			m_NetworkMapListView.AdjustColumnWidth();
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_TSPROCESSOR_TSPROCESSORLIST:
			if (HIWORD(wParam)==CBN_SELCHANGE)
				UpdateCurSettings();
			return TRUE;

		case IDC_TSPROCESSOR_PROPERTIES:
			if (m_pCurSettings!=nullptr) {
				CTSProcessor *pTSProcessor=
					m_TSProcessorManager.GetTSProcessor(m_pCurSettings->m_guid);
				if (pTSProcessor!=nullptr) {
					if (pTSProcessor->ShowPropertyPage(hDlg, GetAppClass().GetResourceInstance()))
						m_TSProcessorManager.SaveTSProcessorProperties(pTSProcessor);
				}
			}
			return TRUE;

		case IDC_TSPROCESSOR_DEFAULTMODULE:
		case IDC_TSPROCESSOR_DEFAULTDEVICE:
			if (HIWORD(wParam)==CBN_SELCHANGE || HIWORD(wParam)==CBN_EDITCHANGE)
				::PostMessage(hDlg,WM_APP_UPDATEDEVICEFILTERLIST,0,0);
			return TRUE;

		case IDC_TSPROCESSOR_TUNERMAP_ADD:
			if (m_pCurSettings!=nullptr) {
				CTSProcessorManager::TunerFilterInfo Info;

				if (TunerMapDialog(&Info)) {
					m_pCurSettings->m_TunerFilterMap.push_back(Info);
					int Index=m_TunerMapListView.InsertItem(
						-1,TEXT(""),m_pCurSettings->m_TunerFilterMap.size()-1);
					int ItemCount=m_TunerMapListView.GetItemCount();
					for (int i=Index;i<ItemCount;i++)
						UpdateTunerMapItem(i);
					m_TunerMapListView.SetItemState(Index,
						LVIS_FOCUSED | LVIS_SELECTED,
						LVIS_FOCUSED | LVIS_SELECTED);
				}
			}
			return TRUE;

		case IDC_TSPROCESSOR_TUNERMAP_EDIT:
			if (m_pCurSettings!=nullptr) {
				int Sel=m_TunerMapListView.GetSelectedItem();

				if (Sel>=0 && (size_t)Sel<m_pCurSettings->m_TunerFilterMap.size()) {
					CTSProcessorManager::TunerFilterInfo &Info=
						m_pCurSettings->m_TunerFilterMap[Sel];
					if (TunerMapDialog(&Info)) {
						UpdateTunerMapItem(Sel);
					}
				}
			}
			return TRUE;

		case IDC_TSPROCESSOR_TUNERMAP_REMOVE:
			if (m_pCurSettings!=nullptr) {
				int Sel=m_TunerMapListView.GetSelectedItem();

				if (Sel>=0) {
					m_TunerMapListView.DeleteItem(Sel);
					auto it=m_pCurSettings->m_TunerFilterMap.begin();
					std::advance(it,Sel);
					m_pCurSettings->m_TunerFilterMap.erase(it);
					int ItemCount=m_TunerMapListView.GetItemCount();
					for (int i=Sel;i<ItemCount;i++)
						UpdateTunerMapItem(i);
				}
			}
			return TRUE;

		case IDC_TSPROCESSOR_TUNERMAP_UP:
		case IDC_TSPROCESSOR_TUNERMAP_DOWN:
			if (m_pCurSettings!=nullptr) {
				int Sel=m_TunerMapListView.GetSelectedItem();
				int ItemCount=m_TunerMapListView.GetItemCount();
				int To;

				if (LOWORD(wParam)==IDC_TSPROCESSOR_TUNERMAP_UP) {
					if (Sel<1)
						return TRUE;
					To=Sel-1;
				} else {
					if (Sel<0 || Sel+1>=ItemCount)
						return TRUE;
					To=Sel+1;
				}

				std::swap(m_pCurSettings->m_TunerFilterMap[Sel],
						  m_pCurSettings->m_TunerFilterMap[To]);
				UpdateTunerMapItem(Sel);
				UpdateTunerMapItem(To);
				m_TunerMapListView.SetItemState(To,
					LVIS_FOCUSED | LVIS_SELECTED,
					LVIS_FOCUSED | LVIS_SELECTED);
			}
			return TRUE;

		case IDC_TSPROCESSOR_NETWORKMAP_ADD:
			if (m_pCurSettings!=nullptr) {
				CTSProcessorManager::NetworkFilterInfo Info;

				if (NetworkMapDialog(&Info)) {
					m_pCurSettings->m_NetworkFilterMap.push_back(Info);
					int Index=m_NetworkMapListView.InsertItem(
						-1,TEXT(""),m_pCurSettings->m_NetworkFilterMap.size()-1);
					int ItemCount=m_NetworkMapListView.GetItemCount();
					for (int i=Index;i<ItemCount;i++)
						UpdateNetworkMapItem(i);
					m_NetworkMapListView.SetItemState(Index,
						LVIS_FOCUSED | LVIS_SELECTED,
						LVIS_FOCUSED | LVIS_SELECTED);
				}
			}
			return TRUE;

		case IDC_TSPROCESSOR_NETWORKMAP_EDIT:
			if (m_pCurSettings!=nullptr) {
				int Sel=m_NetworkMapListView.GetSelectedItem();

				if (Sel>=0 && (size_t)Sel<m_pCurSettings->m_NetworkFilterMap.size()) {
					CTSProcessorManager::NetworkFilterInfo &Info=
						m_pCurSettings->m_NetworkFilterMap[Sel];
					if (NetworkMapDialog(&Info)) {
						UpdateNetworkMapItem(Sel);
					}
				}
			}
			return TRUE;

		case IDC_TSPROCESSOR_NETWORKMAP_REMOVE:
			if (m_pCurSettings!=nullptr) {
				int Sel=m_NetworkMapListView.GetSelectedItem();

				if (Sel>=0) {
					m_NetworkMapListView.DeleteItem(Sel);
					auto it=m_pCurSettings->m_NetworkFilterMap.begin();
					std::advance(it,Sel);
					m_pCurSettings->m_NetworkFilterMap.erase(it);
					int ItemCount=m_NetworkMapListView.GetItemCount();
					for (int i=Sel;i<ItemCount;i++)
						UpdateNetworkMapItem(i);
				}
			}
			return TRUE;

		case IDC_TSPROCESSOR_NETWORKMAP_UP:
		case IDC_TSPROCESSOR_NETWORKMAP_DOWN:
			if (m_pCurSettings!=nullptr) {
				int Sel=m_NetworkMapListView.GetSelectedItem();
				int ItemCount=m_NetworkMapListView.GetItemCount();
				int To;

				if (LOWORD(wParam)==IDC_TSPROCESSOR_NETWORKMAP_UP) {
					if (Sel<1)
						return TRUE;
					To=Sel-1;
				} else {
					if (Sel<0 || Sel+1>=ItemCount)
						return TRUE;
					To=Sel+1;
				}

				std::swap(m_pCurSettings->m_NetworkFilterMap[Sel],
						  m_pCurSettings->m_NetworkFilterMap[To]);
				UpdateNetworkMapItem(Sel);
				UpdateNetworkMapItem(To);
				m_NetworkMapListView.SetItemState(To,
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

				for (auto it=m_SettingsList.begin();it!=m_SettingsList.end();++it) {
					GUID guid=(*it)->m_guid;
					m_TSProcessorManager.SetTSProcessorSettings(*it);
					m_TSProcessorManager.ApplyTSProcessorSettings(guid,false);
				}
				m_SettingsList.clear();

				m_fChanged=true;
			}
			return TRUE;

		case LVN_ITEMCHANGED:
			if (m_pCurSettings!=nullptr) {
				LPNMLISTVIEW pnmlv=reinterpret_cast<LPNMLISTVIEW>(lParam);

				if ((pnmlv->uChanged & LVIF_STATE)==0)
					return TRUE;

				const bool fCheckStateChanged=
					(pnmlv->uOldState & LVIS_STATEIMAGEMASK)!=0
						&& ((pnmlv->uOldState ^ pnmlv->uNewState) & LVIS_STATEIMAGEMASK)!=0;
				const bool fSelectStateChanged=
					((pnmlv->uOldState ^ pnmlv->uNewState) & LVIS_SELECTED)!=0;

				if (pnmlv->hdr.hwndFrom==::GetDlgItem(hDlg,IDC_TSPROCESSOR_TUNERMAP)) {
					if (fCheckStateChanged) {
						if (pnmlv->iItem>=0 && (size_t)pnmlv->iItem<m_pCurSettings->m_TunerFilterMap.size()) {
							m_pCurSettings->m_TunerFilterMap[pnmlv->iItem].fEnable=
								(pnmlv->uNewState & LVIS_STATEIMAGEMASK)==INDEXTOSTATEIMAGEMASK(2);
						}
					}
					if (fSelectStateChanged) {
						int Sel=m_TunerMapListView.GetSelectedItem();
						EnableDlgItems(hDlg,
							IDC_TSPROCESSOR_TUNERMAP_EDIT,
							IDC_TSPROCESSOR_TUNERMAP_REMOVE,
							Sel>=0);
						EnableDlgItem(hDlg,IDC_TSPROCESSOR_TUNERMAP_UP,Sel>0);
						EnableDlgItem(hDlg,IDC_TSPROCESSOR_TUNERMAP_DOWN,
									  Sel>=0 && Sel+1<m_TunerMapListView.GetItemCount());
					}
				} else if (pnmlv->hdr.hwndFrom==::GetDlgItem(hDlg,IDC_TSPROCESSOR_NETWORKMAP)) {
					if (fCheckStateChanged) {
						if (pnmlv->iItem>=0 && (size_t)pnmlv->iItem<m_pCurSettings->m_NetworkFilterMap.size()) {
							m_pCurSettings->m_NetworkFilterMap[pnmlv->iItem].fEnable=
								(pnmlv->uNewState & LVIS_STATEIMAGEMASK)==INDEXTOSTATEIMAGEMASK(2);
						}
					}
					if (fSelectStateChanged) {
						int Sel=m_NetworkMapListView.GetSelectedItem();
						EnableDlgItems(hDlg,
							IDC_TSPROCESSOR_NETWORKMAP_EDIT,
							IDC_TSPROCESSOR_NETWORKMAP_REMOVE,
							Sel>=0);
						EnableDlgItem(hDlg,IDC_TSPROCESSOR_NETWORKMAP_UP,Sel>0);
						EnableDlgItem(hDlg,IDC_TSPROCESSOR_NETWORKMAP_DOWN,
									  Sel>=0 && Sel+1<m_NetworkMapListView.GetItemCount());
					}
				}
			}
			return TRUE;

		case NM_DBLCLK:
			{
				LPNMITEMACTIVATE pnmia=reinterpret_cast<LPNMITEMACTIVATE>(lParam);

				if (pnmia->hdr.hwndFrom==::GetDlgItem(hDlg,IDC_TSPROCESSOR_TUNERMAP)) {
					::SendMessage(hDlg,WM_COMMAND,IDC_TSPROCESSOR_TUNERMAP_EDIT,0);
				} else if (pnmia->hdr.hwndFrom==::GetDlgItem(hDlg,IDC_TSPROCESSOR_NETWORKMAP)) {
					::SendMessage(hDlg,WM_COMMAND,IDC_TSPROCESSOR_NETWORKMAP_EDIT,0);
				}
			}
			return TRUE;

		case NM_RCLICK:
			{
				LPNMITEMACTIVATE pnmia=reinterpret_cast<LPNMITEMACTIVATE>(lParam);

				if (pnmia->hdr.hwndFrom==::GetDlgItem(hDlg,IDC_TSPROCESSOR_TUNERMAP)) {
					if (m_TunerMapListView.GetSelectedItem()>=0) {
						static const int MenuIDs[] = {
							IDC_TSPROCESSOR_TUNERMAP_EDIT,
							0,
							IDC_TSPROCESSOR_TUNERMAP_UP,
							IDC_TSPROCESSOR_TUNERMAP_DOWN,
							0,
							IDC_TSPROCESSOR_TUNERMAP_REMOVE,
						};
						PopupMenuFromControls(hDlg,MenuIDs,lengthof(MenuIDs),TPM_RIGHTBUTTON);
					}
				} else if (pnmia->hdr.hwndFrom==::GetDlgItem(hDlg,IDC_TSPROCESSOR_NETWORKMAP)) {
					if (m_NetworkMapListView.GetSelectedItem()>=0) {
						static const int MenuIDs[] = {
							IDC_TSPROCESSOR_NETWORKMAP_EDIT,
							0,
							IDC_TSPROCESSOR_NETWORKMAP_UP,
							IDC_TSPROCESSOR_NETWORKMAP_DOWN,
							0,
							IDC_TSPROCESSOR_NETWORKMAP_REMOVE,
						};
						PopupMenuFromControls(hDlg,MenuIDs,lengthof(MenuIDs),TPM_RIGHTBUTTON);
					}
				}
			}
			return TRUE;
		}
		break;

	case WM_DESTROY:
		for (auto it=m_SettingsList.begin();it!=m_SettingsList.end();++it)
			delete *it;
		m_SettingsList.clear();
		m_pCurSettings=nullptr;
		m_ModuleList.clear();
		m_ModuleInfoMap.clear();
		m_TunerMapListView.Detach();
		m_NetworkMapListView.Detach();
		return TRUE;

	case WM_APP_UPDATEDEVICEFILTERLIST:
		UpdateDeviceFilterList(hDlg,
								IDC_TSPROCESSOR_DEFAULTMODULE,
								IDC_TSPROCESSOR_DEFAULTDEVICE,
								IDC_TSPROCESSOR_DEFAULTFILTER);
		return TRUE;
	}

	return FALSE;
}


const CTSProcessorOptions::ModuleInfo *CTSProcessorOptions::GetModuleInfo(const String &Name)
{
	auto it=m_ModuleInfoMap.find(Name);

	if (it!=m_ModuleInfoMap.end())
		return &it->second;

	if (m_pCurSettings!=nullptr) {
		CTSProcessor *pTSProcessor=m_TSProcessorManager.GetTSProcessor(m_pCurSettings->m_guid);

		if (pTSProcessor!=nullptr) {
			ModuleInfo Module;

			if (!Name.empty()) {
				if (!pTSProcessor->GetModuleInfo(Name.c_str(),&Module))
					return nullptr;
			} else if (!pTSProcessor->IsFilterModuleSupported()) {
				const int DeviceCount=pTSProcessor->GetDeviceCount();
				for (int i=0;i<DeviceCount;i++) {
					CTSProcessor::ModuleDeviceInfo Device;
					pTSProcessor->GetDeviceFilterList(i,&Device.FilterList);
					Module.DeviceList.push_back(Device);
				}
			} else {
				return nullptr;
			}

			auto Result=m_ModuleInfoMap.insert(std::pair<String,ModuleInfo>(Name,Module));
			return &Result.first->second;
		}
	}

	return nullptr;
}


void CTSProcessorOptions::UpdateCurSettings()
{
	if (m_pCurSettings!=nullptr) {
		m_pCurSettings->m_EnableProcessing=
			DlgCheckBox_IsChecked(m_hDlg,IDC_TSPROCESSOR_ENABLEPROCESSING);
		GetDlgItemString(m_hDlg,IDC_TSPROCESSOR_DEFAULTMODULE,&m_pCurSettings->m_DefaultFilter.Module);
		GetDlgItemString(m_hDlg,IDC_TSPROCESSOR_DEFAULTDEVICE,&m_pCurSettings->m_DefaultFilter.Device);
		GetDlgItemString(m_hDlg,IDC_TSPROCESSOR_DEFAULTFILTER,&m_pCurSettings->m_DefaultFilter.Filter);
	}

	int Sel=(int)DlgComboBox_GetCurSel(m_hDlg,IDC_TSPROCESSOR_TSPROCESSORLIST);

	if (Sel>=0) {
		CTSProcessorManager::CTSProcessorSettings *pSettings=
			reinterpret_cast<CTSProcessorManager::CTSProcessorSettings*>(
				DlgComboBox_GetItemData(m_hDlg,IDC_TSPROCESSOR_TSPROCESSORLIST,Sel));
		if (m_pCurSettings==pSettings)
			return;
		m_pCurSettings=pSettings;
	} else {
		m_pCurSettings=nullptr;
	}

	m_ModuleList.clear();
	m_ModuleInfoMap.clear();

	UpdateItemsState();
}


void CTSProcessorOptions::UpdateItemsState()
{
	m_TunerMapListView.DeleteAllItems();
	m_NetworkMapListView.DeleteAllItems();
	EnableDlgItems(m_hDlg,
				   IDC_TSPROCESSOR_SETTINGSFIRST,
				   IDC_TSPROCESSOR_SETTINGSLAST,
				   m_pCurSettings!=nullptr);

	if (m_pCurSettings!=nullptr) {
		CTSProcessor *pTSProcessor=m_TSProcessorManager.GetTSProcessor(m_pCurSettings->m_guid);
		bool f;

		EnableDlgItem(m_hDlg,IDC_TSPROCESSOR_PROPERTIES,
					  pTSProcessor->IsPropertyPageSupported());

		if (pTSProcessor->GetEnableProcessing(&f)) {
			DlgCheckBox_Check(m_hDlg,IDC_TSPROCESSOR_ENABLEPROCESSING,
				m_pCurSettings->m_EnableProcessing ?
					m_pCurSettings->m_EnableProcessing.value() : f);
		} else {
			DlgCheckBox_Check(m_hDlg,IDC_TSPROCESSOR_ENABLEPROCESSING,false);
			EnableDlgItem(m_hDlg,IDC_TSPROCESSOR_ENABLEPROCESSING,false);
		}

		DlgComboBox_Clear(m_hDlg,IDC_TSPROCESSOR_DEFAULTMODULE);
		pTSProcessor->GetModuleList(&m_ModuleList);
		for (auto it=m_ModuleList.begin();it!=m_ModuleList.end();++it)
			DlgComboBox_AddString(m_hDlg,IDC_TSPROCESSOR_DEFAULTMODULE,it->c_str());
		if (pTSProcessor->IsFilterModuleSupported()) {
			::SetDlgItemText(m_hDlg,IDC_TSPROCESSOR_DEFAULTMODULE,
							 m_pCurSettings->m_DefaultFilter.Module.c_str());
		} else {
			::SetDlgItemText(m_hDlg,IDC_TSPROCESSOR_DEFAULTMODULE,TEXT(""));
			EnableDlgItems(m_hDlg,
						   IDC_TSPROCESSOR_DEFAULTMODULE_LABEL,
						   IDC_TSPROCESSOR_DEFAULTMODULE,
						   false);
		}
		::SetDlgItemText(m_hDlg,IDC_TSPROCESSOR_DEFAULTDEVICE,
						 m_pCurSettings->m_DefaultFilter.Device.c_str());
		::SetDlgItemText(m_hDlg,IDC_TSPROCESSOR_DEFAULTFILTER,
						 m_pCurSettings->m_DefaultFilter.Filter.c_str());
		UpdateDeviceFilterList(m_hDlg,
								IDC_TSPROCESSOR_DEFAULTMODULE,
								IDC_TSPROCESSOR_DEFAULTDEVICE,
								IDC_TSPROCESSOR_DEFAULTFILTER);

		for (int i=0;i<(int)m_pCurSettings->m_TunerFilterMap.size();i++) {
			m_TunerMapListView.InsertItem(i,TEXT(""));
			UpdateTunerMapItem(i);
		}

		for (int i=0;i<(int)m_pCurSettings->m_NetworkFilterMap.size();i++) {
			m_NetworkMapListView.InsertItem(i,TEXT(""));
			UpdateNetworkMapItem(i);
		}

		EnableDlgItems(m_hDlg,
					   IDC_TSPROCESSOR_TUNERMAP_EDIT,
					   IDC_TSPROCESSOR_TUNERMAP_DOWN,
					   false);
		EnableDlgItems(m_hDlg,
					   IDC_TSPROCESSOR_NETWORKMAP_EDIT,
					   IDC_TSPROCESSOR_NETWORKMAP_DOWN,
					   false);
	} else {
		m_ModuleList.clear();
		m_ModuleInfoMap.clear();
		DlgCheckBox_Check(m_hDlg,IDC_TSPROCESSOR_ENABLEPROCESSING,false);
		::SetDlgItemText(m_hDlg,IDC_TSPROCESSOR_DEFAULTMODULE,TEXT(""));
		::SetDlgItemText(m_hDlg,IDC_TSPROCESSOR_DEFAULTDEVICE,TEXT(""));
		::SetDlgItemText(m_hDlg,IDC_TSPROCESSOR_DEFAULTFILTER,TEXT(""));
	}
}


void CTSProcessorOptions::UpdateDeviceFilterList(HWND hDlg,int ModuleID,int DeviceID,int FilterID)
{
	String ModuleName,DeviceName,FilterName;
	DWORD DeviceEditSel,FilterEditSel;

	GetDlgItemString(hDlg,ModuleID,&ModuleName);
	GetDlgItemString(hDlg,DeviceID,&DeviceName);
	GetDlgItemString(hDlg,FilterID,&FilterName);
	DeviceEditSel=(DWORD)::SendDlgItemMessage(hDlg,DeviceID,CB_GETEDITSEL,0,0);
	FilterEditSel=(DWORD)::SendDlgItemMessage(hDlg,FilterID,CB_GETEDITSEL,0,0);
	DlgComboBox_Clear(hDlg,DeviceID);
	DlgComboBox_Clear(hDlg,FilterID);

	const ModuleInfo *pModuleInfo=GetModuleInfo(ModuleName);

	if (pModuleInfo!=nullptr) {
		int DeviceSel=-1,FilterSel=-1;

		for (auto itDev=pModuleInfo->DeviceList.begin();itDev!=pModuleInfo->DeviceList.end();++itDev) {
			DlgComboBox_AddString(hDlg,DeviceID,itDev->Name.c_str());
			if (StringUtility::CompareNoCase(DeviceName,itDev->Name)==0) {
				for (auto itDec=itDev->FilterList.begin();itDec!=itDev->FilterList.end();++itDec) {
					DlgComboBox_AddString(hDlg,FilterID,itDec->c_str());
					if (StringUtility::CompareNoCase(FilterName,*itDec)==0)
						FilterSel=(int)DlgComboBox_GetCount(hDlg,FilterID)-1;
				}
				DeviceSel=(int)DlgComboBox_GetCount(hDlg,DeviceID)-1;
			}
		}

		if (DeviceSel>=0) {
			DlgComboBox_SetCurSel(hDlg,DeviceID,DeviceSel);
			if (FilterSel>=0) {
				DlgComboBox_SetCurSel(hDlg,FilterID,FilterSel);
			}
		}
	}

	::SetDlgItemText(hDlg,DeviceID,DeviceName.c_str());
	::SendDlgItemMessage(hDlg,DeviceID,CB_SETEDITSEL,0,DeviceEditSel);
	::SetDlgItemText(hDlg,FilterID,FilterName.c_str());
	::SendDlgItemMessage(hDlg,FilterID,CB_SETEDITSEL,0,FilterEditSel);
}


void CTSProcessorOptions::UpdateTunerMapItem(int Index)
{
	const CTSProcessorManager::TunerFilterInfo &Settings=
		m_pCurSettings->m_TunerFilterMap[Index];

	m_TunerMapListView.CheckItem(Index,Settings.fEnable);
	m_TunerMapListView.SetItemText(Index,0,Settings.Tuner.c_str());
	m_TunerMapListView.SetItemText(Index,1,Settings.fEnableProcessing?TEXT("�L��"):TEXT("����"));
	m_TunerMapListView.SetItemText(Index,2,Settings.Module.c_str());
	m_TunerMapListView.SetItemText(Index,3,Settings.Device.c_str());
	m_TunerMapListView.SetItemText(Index,4,Settings.Filter.c_str());
}


void CTSProcessorOptions::UpdateNetworkMapItem(int Index)
{
	const CTSProcessorManager::NetworkFilterInfo &Settings=
		m_pCurSettings->m_NetworkFilterMap[Index];
	TCHAR szText[32];

	m_NetworkMapListView.CheckItem(Index,Settings.fEnable);
	if (Settings.NetworkID!=0xFFFF)
		StdUtil::snprintf(szText,lengthof(szText),TEXT("%d"),Settings.NetworkID);
	else
		StdUtil::strncpy(szText,lengthof(szText),TEXT("(�w��Ȃ�)"));
	m_NetworkMapListView.SetItemText(Index,0,szText);
	if (Settings.TransportStreamID!=0xFFFF)
		StdUtil::snprintf(szText,lengthof(szText),TEXT("%d"),Settings.TransportStreamID);
	else
		StdUtil::strncpy(szText,lengthof(szText),TEXT("(�w��Ȃ�)"));
	m_NetworkMapListView.SetItemText(Index,1,szText);
	m_NetworkMapListView.SetItemText(Index,2,Settings.fEnableProcessing?TEXT("�L��"):TEXT("����"));
	m_NetworkMapListView.SetItemText(Index,3,Settings.Module.c_str());
	m_NetworkMapListView.SetItemText(Index,4,Settings.Device.c_str());
	m_NetworkMapListView.SetItemText(Index,5,Settings.Filter.c_str());
}


bool CTSProcessorOptions::TunerMapDialog(CTSProcessorManager::TunerFilterInfo *pInfo)
{
	CTunerMapDialog Dialog(this,pInfo);

	return Dialog.Show(m_hDlg);
}


bool CTSProcessorOptions::NetworkMapDialog(CTSProcessorManager::NetworkFilterInfo *pInfo)
{
	CNetworkMapDialog Dialog(this,pInfo);

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
	return ShowDialog(hwndOwner,
					  GetAppClass().GetResourceInstance(),
					  MAKEINTRESOURCE(IDD_TSPROCESSOR_TUNERMAP))==IDOK;
}


INT_PTR CTSProcessorOptions::CTunerMapDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			const CDriverManager &DriverManager=GetAppClass().DriverManager;

			int NumDrivers=DriverManager.NumDrivers();
			for (int i=0;i<NumDrivers;i++) {
				DlgComboBox_AddString(hDlg,IDC_TSPROCESSORTUNERMAP_TUNER,
									  DriverManager.GetDriverInfo(i)->GetFileName());
			}
			::SetDlgItemText(hDlg,IDC_TSPROCESSORTUNERMAP_TUNER,m_pInfo->Tuner.c_str());
			::CheckRadioButton(hDlg,
				IDC_TSPROCESSORTUNERMAP_ENABLEPROCESSING,
				IDC_TSPROCESSORTUNERMAP_DISABLEPROCESSING,
				m_pInfo->fEnableProcessing?
					IDC_TSPROCESSORTUNERMAP_ENABLEPROCESSING:
					IDC_TSPROCESSORTUNERMAP_DISABLEPROCESSING);
			for (auto it=m_pOptions->m_ModuleList.begin();it!=m_pOptions->m_ModuleList.end();++it)
				DlgComboBox_AddString(hDlg,IDC_TSPROCESSORTUNERMAP_MODULE,it->c_str());
			::SetDlgItemText(hDlg,IDC_TSPROCESSORTUNERMAP_MODULE,m_pInfo->Module.c_str());
			::SetDlgItemText(hDlg,IDC_TSPROCESSORTUNERMAP_DEVICE,m_pInfo->Device.c_str());
			::SetDlgItemText(hDlg,IDC_TSPROCESSORTUNERMAP_FILTER,m_pInfo->Filter.c_str());
			EnableDlgItems(hDlg,
						   IDC_TSPROCESSORTUNERMAP_MODULE_LABEL,
						   IDC_TSPROCESSORTUNERMAP_FILTER,
						   m_pInfo->fEnableProcessing);
			m_pOptions->UpdateDeviceFilterList(hDlg,
				IDC_TSPROCESSORTUNERMAP_MODULE,
				IDC_TSPROCESSORTUNERMAP_DEVICE,
				IDC_TSPROCESSORTUNERMAP_FILTER);
			AdjustDialogPos(::GetParent(hDlg),hDlg);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_TSPROCESSORTUNERMAP_ENABLEPROCESSING:
		case IDC_TSPROCESSORTUNERMAP_DISABLEPROCESSING:
			EnableDlgItemsSyncCheckBox(hDlg,
									   IDC_TSPROCESSORTUNERMAP_MODULE_LABEL,
									   IDC_TSPROCESSORTUNERMAP_FILTER,
									   IDC_TSPROCESSORTUNERMAP_ENABLEPROCESSING);
			return TRUE;

		case IDC_TSPROCESSORTUNERMAP_MODULE:
		case IDC_TSPROCESSORTUNERMAP_DEVICE:
			if (HIWORD(wParam)==CBN_SELCHANGE || HIWORD(wParam)==CBN_EDITCHANGE)
				::PostMessage(hDlg,WM_APP_UPDATEDEVICEFILTERLIST,0,0);
			return TRUE;

		case IDC_TSPROCESSORTUNERMAP_HELP:
			GetAppClass().UICore.ShowHelpContent(HELP_ID_TSPROCESSOR_TUNERMAP);
			return TRUE;

		case IDOK:
			GetDlgItemString(hDlg,IDC_TSPROCESSORTUNERMAP_TUNER,&m_pInfo->Tuner);
			m_pInfo->fEnableProcessing=
				DlgRadioButton_IsChecked(hDlg,IDC_TSPROCESSORTUNERMAP_ENABLEPROCESSING);
			GetDlgItemString(hDlg,IDC_TSPROCESSORTUNERMAP_MODULE,&m_pInfo->Module);
			GetDlgItemString(hDlg,IDC_TSPROCESSORTUNERMAP_DEVICE,&m_pInfo->Device);
			GetDlgItemString(hDlg,IDC_TSPROCESSORTUNERMAP_FILTER,&m_pInfo->Filter);
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_APP_UPDATEDEVICEFILTERLIST:
		m_pOptions->UpdateDeviceFilterList(hDlg,
			IDC_TSPROCESSORTUNERMAP_MODULE,
			IDC_TSPROCESSORTUNERMAP_DEVICE,
			IDC_TSPROCESSORTUNERMAP_FILTER);
		return TRUE;
	}

	return FALSE;
}




CTSProcessorOptions::CNetworkMapDialog::CNetworkMapDialog(
		CTSProcessorOptions *pOptions,
		CTSProcessorManager::NetworkFilterInfo *pInfo)
	: m_pOptions(pOptions)
	, m_pInfo(pInfo)
{
}


bool CTSProcessorOptions::CNetworkMapDialog::Show(HWND hwndOwner)
{
	return ShowDialog(hwndOwner,
					  GetAppClass().GetResourceInstance(),
					  MAKEINTRESOURCE(IDD_TSPROCESSOR_NETWORKMAP))==IDOK;
}


INT_PTR CTSProcessorOptions::CNetworkMapDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			if (m_pInfo->NetworkID!=0xFFFF)
				::SetDlgItemInt(hDlg,IDC_TSPROCESSORNETWORKMAP_NETWORKID,m_pInfo->NetworkID,FALSE);
			if (m_pInfo->TransportStreamID!=0xFFFF)
				::SetDlgItemInt(hDlg,IDC_TSPROCESSORNETWORKMAP_TSID,m_pInfo->TransportStreamID,FALSE);
			::CheckRadioButton(hDlg,
				IDC_TSPROCESSORNETWORKMAP_ENABLEPROCESSING,
				IDC_TSPROCESSORNETWORKMAP_DISABLEPROCESSING,
				m_pInfo->fEnableProcessing?
					IDC_TSPROCESSORNETWORKMAP_ENABLEPROCESSING:
					IDC_TSPROCESSORNETWORKMAP_DISABLEPROCESSING);
			for (auto it=m_pOptions->m_ModuleList.begin();it!=m_pOptions->m_ModuleList.end();++it)
				DlgComboBox_AddString(hDlg,IDC_TSPROCESSORNETWORKMAP_MODULE,it->c_str());
			::SetDlgItemText(hDlg,IDC_TSPROCESSORNETWORKMAP_MODULE,m_pInfo->Module.c_str());
			::SetDlgItemText(hDlg,IDC_TSPROCESSORNETWORKMAP_DEVICE,m_pInfo->Device.c_str());
			::SetDlgItemText(hDlg,IDC_TSPROCESSORNETWORKMAP_FILTER,m_pInfo->Filter.c_str());
			EnableDlgItems(hDlg,
						   IDC_TSPROCESSORNETWORKMAP_MODULE_LABEL,
						   IDC_TSPROCESSORNETWORKMAP_FILTER,
						   m_pInfo->fEnableProcessing);
			m_pOptions->UpdateDeviceFilterList(hDlg,
				IDC_TSPROCESSORNETWORKMAP_MODULE,
				IDC_TSPROCESSORNETWORKMAP_DEVICE,
				IDC_TSPROCESSORNETWORKMAP_FILTER);
			AdjustDialogPos(::GetParent(hDlg),hDlg);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_TSPROCESSORNETWORKMAP_ENABLEPROCESSING:
		case IDC_TSPROCESSORNETWORKMAP_DISABLEPROCESSING:
			EnableDlgItemsSyncCheckBox(hDlg,
									   IDC_TSPROCESSORNETWORKMAP_MODULE_LABEL,
									   IDC_TSPROCESSORNETWORKMAP_FILTER,
									   IDC_TSPROCESSORNETWORKMAP_ENABLEPROCESSING);
			return TRUE;

		case IDC_TSPROCESSORNETWORKMAP_MODULE:
		case IDC_TSPROCESSORNETWORKMAP_DEVICE:
			if (HIWORD(wParam)==CBN_SELCHANGE || HIWORD(wParam)==CBN_EDITCHANGE)
				::PostMessage(hDlg,WM_APP_UPDATEDEVICEFILTERLIST,0,0);
			return TRUE;

		case IDC_TSPROCESSORNETWORKMAP_HELP:
			GetAppClass().UICore.ShowHelpContent(HELP_ID_TSPROCESSOR_NETWORKMAP);
			return TRUE;

		case IDOK:
			{
				String Buffer;

				if (GetDlgItemString(hDlg,IDC_TSPROCESSORNETWORKMAP_NETWORKID,&Buffer)) {
					try {
						m_pInfo->NetworkID=static_cast<WORD>(std::stoi(Buffer,nullptr,0));
					} catch (...) {
						m_pInfo->NetworkID=0xFFFF;
					}
				}
				if (GetDlgItemString(hDlg,IDC_TSPROCESSORNETWORKMAP_TSID,&Buffer)) {
					try {
						m_pInfo->TransportStreamID=static_cast<WORD>(std::stoi(Buffer,nullptr,0));
					} catch (...) {
						m_pInfo->TransportStreamID=0xFFFF;
					}
				}
				m_pInfo->fEnableProcessing=
					DlgRadioButton_IsChecked(hDlg,IDC_TSPROCESSORNETWORKMAP_ENABLEPROCESSING);
				GetDlgItemString(hDlg,IDC_TSPROCESSORNETWORKMAP_MODULE,&m_pInfo->Module);
				GetDlgItemString(hDlg,IDC_TSPROCESSORNETWORKMAP_DEVICE,&m_pInfo->Device);
				GetDlgItemString(hDlg,IDC_TSPROCESSORNETWORKMAP_FILTER,&m_pInfo->Filter);
			}
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_APP_UPDATEDEVICEFILTERLIST:
		m_pOptions->UpdateDeviceFilterList(hDlg,
			IDC_TSPROCESSORNETWORKMAP_MODULE,
			IDC_TSPROCESSORNETWORKMAP_DEVICE,
			IDC_TSPROCESSORNETWORKMAP_FILTER);
		return TRUE;
	}

	return FALSE;
}


}	// namespace TVTest
