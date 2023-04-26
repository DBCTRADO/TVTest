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
#include "FeaturedEvents.h"
#include "AppMain.h"
#include "DriverManager.h"
#include "DialogUtil.h"
#include "LogoManager.h"
#include "EpgUtil.h"
#include "DPIUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CFeaturedEventsSettings::CFeaturedEventsSettings()
	: CSettingsBase(TEXT("FeaturedEvents"))
{
}


bool CFeaturedEventsSettings::ReadSettings(CSettings &Settings)
{
	String Services;
	if (Settings.Read(TEXT("DefaultServices"), &Services))
		m_DefaultServiceList.FromString(Services.c_str());

	m_SearchSettingsList.Load(Settings, TEXT("Settings"));

	int Sort;
	if (Settings.Read(TEXT("Sort"), &Sort)
			&& CheckEnumRange(static_cast<SortType>(Sort)))
		m_SortType = static_cast<SortType>(Sort);

	Settings.Read(TEXT("Period"), &m_PeriodSeconds);
	Settings.Read(TEXT("ShowEventText"), &m_fShowEventText);

	int Lines;
	if (Settings.Read(TEXT("EventTextLines"), &Lines)
			&& Lines > 0 && Lines <= MAX_EVENT_TEXT_LINES)
		m_EventTextLines = Lines;

	return true;
}


bool CFeaturedEventsSettings::WriteSettings(CSettings &Settings)
{
	Settings.Clear();

	String Services;
	m_DefaultServiceList.ToString(&Services);
	Settings.Write(TEXT("DefaultServices"), Services);

	m_SearchSettingsList.Save(Settings, TEXT("Settings"));

	Settings.Write(TEXT("Sort"), static_cast<int>(m_SortType));
	Settings.Write(TEXT("Period"), m_PeriodSeconds);
	Settings.Write(TEXT("ShowEventText"), m_fShowEventText);
	Settings.Write(TEXT("EventTextLines"), m_EventTextLines);

	return true;
}




CFeaturedEventsSearcher::CFeaturedEventsSearcher(const CFeaturedEventsSettings &Settings)
	: m_Settings(Settings)
{
}


void CFeaturedEventsSearcher::Clear()
{
	m_EventList.clear();
}


bool CFeaturedEventsSearcher::Update()
{
	Clear();

	LibISDB::DateTime StartTime, PeriodTime;
	LibISDB::GetCurrentEPGTime(&StartTime);
	PeriodTime = StartTime;
	PeriodTime.OffsetSeconds(m_Settings.GetPeriodSeconds());

	const CEventSearchServiceList &DefaultServiceList = m_Settings.GetDefaultServiceList();
	CEventSearchServiceList ServiceIDList(DefaultServiceList);

	const CEventSearchSettingsList &SearchSettingsList = m_Settings.GetSearchSettingsList();
	std::vector<std::unique_ptr<CEventSearcher>> SearcherList;
	SearcherList.reserve(SearchSettingsList.GetEnabledCount());

	for (size_t i = 0; i < SearchSettingsList.GetCount(); i++) {
		const CEventSearchSettings *pSettings = SearchSettingsList.Get(i);

		if (!pSettings->fDisabled) {
			if (pSettings->fServiceList) {
				ServiceIDList.Combine(pSettings->ServiceList);
			}

			SearcherList.emplace_back(std::make_unique<CEventSearcher>())->BeginSearch(*pSettings);
		}
	}

	const LibISDB::EPGDatabase &EPGDatabase = GetAppClass().EPGDatabase;

	for (auto itService = ServiceIDList.Begin(); itService != ServiceIDList.End(); ++itService) {
		EPGDatabase.EnumEventsUnsorted(
			CEventSearchServiceList::ServiceKey_GetNetworkID(*itService),
			CEventSearchServiceList::ServiceKey_GetTransportStreamID(*itService),
			CEventSearchServiceList::ServiceKey_GetServiceID(*itService),
			[&](const LibISDB::EventInfo & Event) -> bool {
				if (Event.IsCommonEvent)
					return true;
				if (Event.StartTime >= PeriodTime)
					return true;

				LibISDB::DateTime EndTime;
				Event.GetEndTime(&EndTime);
				if (EndTime <= StartTime)
					return true;

				for (auto &Searcher : SearcherList) {
					if (!Searcher->GetSearchSettings().fServiceList) {
						if (!DefaultServiceList.IsExists(*itService))
							continue;
					}
					if (Searcher->Match(&Event)) {
						m_EventList.emplace_back(std::make_unique<LibISDB::EventInfo>(Event));
						break;
					}
				}

				return true;
			});
	}

	return true;
}


size_t CFeaturedEventsSearcher::GetEventCount() const
{
	return m_EventList.size();
}


const LibISDB::EventInfo *CFeaturedEventsSearcher::GetEventInfo(size_t Index) const
{
	if (Index >= m_EventList.size())
		return nullptr;

	return m_EventList[Index].get();
}




bool CFeaturedEventsMatcher::BeginMatching(const CFeaturedEventsSettings &Settings)
{
	m_DefaultServiceList = Settings.GetDefaultServiceList();

	const CEventSearchSettingsList &SearchSettingsList = Settings.GetSearchSettingsList();
	m_SearcherList.clear();
	m_SearcherList.reserve(SearchSettingsList.GetEnabledCount());

	for (size_t i = 0; i < SearchSettingsList.GetCount(); i++) {
		const CEventSearchSettings *pSettings = SearchSettingsList.Get(i);

		if (!pSettings->fDisabled) {
			m_SearcherList.emplace_back(std::make_unique<CEventSearcher>())->BeginSearch(*pSettings);
		}
	}

	return true;
}


void CFeaturedEventsMatcher::EndMatching()
{
	m_DefaultServiceList.Clear();
	m_SearcherList.clear();
}


bool CFeaturedEventsMatcher::IsMatch(const LibISDB::EventInfo &EventInfo)
{
	for (auto &Searcher : m_SearcherList) {
		if (!Searcher->GetSearchSettings().fServiceList) {
			if (!m_DefaultServiceList.IsExists(
						EventInfo.NetworkID,
						EventInfo.TransportStreamID,
						EventInfo.ServiceID))
				continue;
		}
		if (Searcher->Match(&EventInfo)) {
			return true;
		}
	}

	return false;
}




static void InitServiceListView(
	HWND hwndList,
	const CChannelList &ServiceList,
	const CEventSearchServiceList &SearchServiceList,
	int DPI)
{
	::SetWindowTheme(hwndList, L"explorer", nullptr);
	ListView_SetExtendedListViewStyle(
		hwndList, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	const int IconWidth = GetSystemMetricsWithDPI(SM_CXSMICON, DPI);
	const int IconHeight = GetSystemMetricsWithDPI(SM_CYSMICON, DPI);
	const HIMAGELIST himl = ::ImageList_Create(
		IconWidth, IconHeight, ILC_COLOR24 | ILC_MASK,
		ServiceList.NumChannels() + 1, 100);
	const HICON hico = CreateEmptyIcon(IconWidth, IconHeight);
	ImageList_AddIcon(himl, hico);
	::DestroyIcon(hico);
	ListView_SetImageList(hwndList, himl, LVSIL_SMALL);
	CAppMain &App = GetAppClass();
	CLogoManager &LogoManager = App.LogoManager;

	enum {
		GROUP_ID_TERRESTRIAL = 1,
		GROUP_ID_BS,
		GROUP_ID_CS
	};
	LVGROUP lvg;
	lvg.cbSize = sizeof(LVGROUP);
	lvg.mask = LVGF_HEADER | LVGF_GROUPID;
	lvg.mask |= LVGF_STATE | LVGF_TASK;
	lvg.stateMask = ~0U;
	lvg.state = LVGS_COLLAPSIBLE;
	lvg.pszTask = const_cast<LPTSTR>(TEXT("選択"));
	lvg.pszHeader = const_cast<LPTSTR>(TEXT("地上"));
	lvg.iGroupId = GROUP_ID_TERRESTRIAL;
	ListView_InsertGroup(hwndList, -1, &lvg);
	lvg.pszHeader = const_cast<LPTSTR>(TEXT("BS"));
	lvg.iGroupId = GROUP_ID_BS;
	ListView_InsertGroup(hwndList, -1, &lvg);
	lvg.pszHeader = const_cast<LPTSTR>(TEXT("CS"));
	lvg.iGroupId = GROUP_ID_CS;
	ListView_InsertGroup(hwndList, -1, &lvg);

	LVCOLUMN lvc;
	RECT rc;
	::GetClientRect(hwndList, &rc);
	lvc.mask = LVCF_FMT | LVCF_WIDTH;
	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = rc.right - GetScrollBarWidth(hwndList);
	ListView_InsertColumn(hwndList, 0, &lvc);

	LVITEM lvi;
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_GROUPID;
	lvi.iSubItem = 0;
	for (int i = 0; i < ServiceList.NumChannels(); i++) {
		const CChannelInfo *pChannelInfo = ServiceList.GetChannelInfo(i);

		lvi.iItem = i;
		lvi.pszText = const_cast<LPTSTR>(pChannelInfo->GetName());

		const HICON hico = LogoManager.CreateLogoIcon(
			pChannelInfo->GetNetworkID(),
			pChannelInfo->GetServiceID(),
			IconWidth, IconHeight);
		if (hico != nullptr) {
			lvi.iImage = ImageList_AddIcon(himl, hico);
			::DestroyIcon(hico);
		} else {
			lvi.iImage = 0;
		}

		lvi.lParam = i;

		switch (App.NetworkDefinition.GetNetworkType(pChannelInfo->GetNetworkID())) {
		default:
		case CNetworkDefinition::NetworkType::Terrestrial:
			lvi.iGroupId = GROUP_ID_TERRESTRIAL;
			break;
		case CNetworkDefinition::NetworkType::BS:
			lvi.iGroupId = GROUP_ID_BS;
			break;
		case CNetworkDefinition::NetworkType::CS:
			lvi.iGroupId = GROUP_ID_CS;
			break;
		}

		const int Index = ListView_InsertItem(hwndList, &lvi);

		ListView_SetCheckState(
			hwndList, Index,
			SearchServiceList.IsExists(
				pChannelInfo->GetNetworkID(),
				pChannelInfo->GetTransportStreamID(),
				pChannelInfo->GetServiceID()));
	}

	ListView_EnableGroupView(hwndList, TRUE);
}


static BOOL ServiceListViewGetInfoTip(
	NMLVGETINFOTIP *pGetInfoTip,
	const CChannelList &ServiceList)
{
	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iItem = pGetInfoTip->iItem;
	lvi.iSubItem = 0;
	if (!ListView_GetItem(pGetInfoTip->hdr.hwndFrom, &lvi))
		return FALSE;

	const CChannelInfo *pChannelInfo = ServiceList.GetChannelInfo(static_cast<int>(lvi.lParam));
	if (pChannelInfo == nullptr)
		return FALSE;

	StringFormat(
		pGetInfoTip->pszText, pGetInfoTip->cchTextMax,
		TEXT("{0}\r\nサービス: {1} ({1:#04x})\r\nネットワークID: {2} ({2:#04x})\r\nTSID: {3} ({3:#04x})"),
		pChannelInfo->GetName(),
		pChannelInfo->GetServiceID(),
		pChannelInfo->GetNetworkID(),
		pChannelInfo->GetTransportStreamID());

	return TRUE;
}


static BOOL ServiceListViewOnLinkClick(HWND hDlg, NMLVLINK *pLink)
{
	const HWND hwndList = pLink->hdr.hwndFrom;
	RECT rc;
	if (!ListView_GetGroupRect(hwndList, pLink->iSubItem, LVGGR_HEADER, &rc))
		return FALSE;

	enum {
		COMMAND_CHECKALL = 1,
		COMMAND_UNCHECKALL,
		COMMAND_INVERTCHECK
	};
	static const struct {
		int ID;
		LPCTSTR pszText;
	} CommandList[] = {
		{COMMAND_CHECKALL,    TEXT("すべて選択")},
		{COMMAND_UNCHECKALL,  TEXT("すべて解除")},
		{COMMAND_INVERTCHECK, TEXT("選択の反転")},
	};

	const HMENU hmenu = ::CreatePopupMenu();
	for (const auto &e : CommandList) {
		::AppendMenu(hmenu, MF_STRING | MF_ENABLED, e.ID, e.pszText);
	}

	POINT pt = {rc.right, rc.bottom};
	::ClientToScreen(hwndList, &pt);
	const int Command = ::TrackPopupMenu(
		hmenu, TPM_RIGHTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
		pt.x, pt.y, 0, hDlg, nullptr);
	::DestroyMenu(hmenu);
	if (Command > 0) {
		const int ItemCount = ListView_GetItemCount(hwndList);
		bool fCheck = Command == COMMAND_CHECKALL;

		for (int i = 0; i < ItemCount; i++) {
			LVITEM lvi;

			lvi.mask = LVIF_GROUPID;
			lvi.iItem = i;
			lvi.iSubItem = 0;
			ListView_GetItem(hwndList, &lvi);

			if (lvi.iGroupId == pLink->iSubItem) {
				if (Command == COMMAND_INVERTCHECK) {
					fCheck = !ListView_GetCheckState(hwndList, i);
				}

				ListView_SetCheckState(hwndList, i, fCheck);
			}
		}
	}

	return TRUE;
}




class CFeaturedEventsSearchDialog
	: public CBasicDialog
{
public:
	CFeaturedEventsSearchDialog(
		CEventSearchSettings &Settings,
		CEventSearchOptions &Options,
		const CChannelList &ServiceList);

// CBasicDialog
	bool Show(HWND hwndOwner) override;

private:
// CBasicDialog
	INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	CEventSearchSettings &m_SearchSettings;
	CEventSearchSettingsDialog m_SearchSettingsDialog;
	const CChannelList &m_ServiceList;
};


CFeaturedEventsSearchDialog::CFeaturedEventsSearchDialog(
	CEventSearchSettings &Settings,
	CEventSearchOptions &Options,
	const CChannelList &ServiceList)
	: m_SearchSettings(Settings)
	, m_SearchSettingsDialog(Options)
	, m_ServiceList(ServiceList)
{
	RegisterUIChild(&m_SearchSettingsDialog);
	SetStyleScaling(&m_StyleScaling);
}


bool CFeaturedEventsSearchDialog::Show(HWND hwndOwner)
{
	return ShowDialog(
		hwndOwner,
		GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE(IDD_FEATUREDEVENTSSEARCH)) == IDOK;
}


INT_PTR CFeaturedEventsSearchDialog::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		m_SearchSettingsDialog.SetSettings(m_SearchSettings);
		m_SearchSettingsDialog.Create(hDlg);
		m_SearchSettingsDialog.ShowButton(IDC_EVENTSEARCH_HIGHLIGHT, false);
		m_SearchSettingsDialog.ShowButton(IDC_EVENTSEARCH_SEARCHTARGET, false);
		m_SearchSettingsDialog.ShowButton(IDC_EVENTSEARCH_SEARCH, false);
		{
			RECT rc;
			GetDlgItemRect(hDlg, IDC_FEATUREDEVENTSSEARCH_SETTINGSPLACE, &rc);
			m_SearchSettingsDialog.SetPosition(&rc);
		}

		DlgEdit_LimitText(hDlg, IDC_FEATUREDEVENTSSEARCH_NAME, CEventSearchSettings::MAX_NAME_LENGTH - 1);
		DlgEdit_SetText(hDlg, IDC_FEATUREDEVENTSSEARCH_NAME, m_SearchSettings.Name.c_str());

		InitServiceListView(
			::GetDlgItem(hDlg, IDC_FEATUREDEVENTSSEARCH_SERVICELIST),
			m_ServiceList, m_SearchSettings.ServiceList,
			m_CurrentDPI);

		DlgCheckBox_Check(hDlg, IDC_FEATUREDEVENTSSEARCH_USESERVICELIST, m_SearchSettings.fServiceList);
		EnableDlgItems(
			hDlg,
			IDC_FEATUREDEVENTSSEARCH_SERVICELIST,
			IDC_FEATUREDEVENTSSEARCH_SERVICELIST_UNCHECKALL,
			m_SearchSettings.fServiceList);
		return TRUE;

	case WM_SIZE:
		{
			RECT rc;
			GetDlgItemRect(hDlg, IDC_FEATUREDEVENTSSEARCH_SETTINGSPLACE, &rc);
			m_SearchSettingsDialog.SetPosition(&rc);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_FEATUREDEVENTSSEARCH_USESERVICELIST:
			EnableDlgItemsSyncCheckBox(
				hDlg,
				IDC_FEATUREDEVENTSSEARCH_SERVICELIST,
				IDC_FEATUREDEVENTSSEARCH_SERVICELIST_UNCHECKALL,
				IDC_FEATUREDEVENTSSEARCH_USESERVICELIST);
			return TRUE;

		case IDC_FEATUREDEVENTSSEARCH_SERVICELIST_CHECKALL:
		case IDC_FEATUREDEVENTSSEARCH_SERVICELIST_UNCHECKALL:
			{
				const HWND hwndList = ::GetDlgItem(hDlg, IDC_FEATUREDEVENTSSEARCH_SERVICELIST);
				const int ItemCount = ListView_GetItemCount(hwndList);
				const BOOL fCheck = LOWORD(wParam) == IDC_FEATUREDEVENTSSEARCH_SERVICELIST_CHECKALL;

				for (int i = 0; i < ItemCount; i++) {
					ListView_SetCheckState(hwndList, i, fCheck);
				}
			}
			return TRUE;

		case IDOK:
			{
				m_SearchSettingsDialog.GetSettings(&m_SearchSettings);

				TCHAR szName[CEventSearchSettings::MAX_NAME_LENGTH];
				DlgEdit_GetText(hDlg, IDC_FEATUREDEVENTSSEARCH_NAME, szName, lengthof(szName));
				m_SearchSettings.Name = szName;

				m_SearchSettings.fServiceList =
					DlgCheckBox_IsChecked(hDlg, IDC_FEATUREDEVENTSSEARCH_USESERVICELIST);

				if (m_SearchSettings.fServiceList) {
					const HWND hwndList = ::GetDlgItem(hDlg, IDC_FEATUREDEVENTSSEARCH_SERVICELIST);
					const int ItemCount = ListView_GetItemCount(hwndList);
					LVITEM lvi;
					lvi.mask = LVIF_PARAM;
					lvi.iSubItem = 0;
					for (int i = 0; i < ItemCount; i++) {
						if (ListView_GetCheckState(hwndList, i)) {
							lvi.iItem = i;
							ListView_GetItem(hwndList, &lvi);
							const CChannelInfo *pChannelInfo = m_ServiceList.GetChannelInfo(static_cast<int>(lvi.lParam));
							if (pChannelInfo != nullptr) {
								m_SearchSettings.ServiceList.Add(
									pChannelInfo->GetNetworkID(),
									pChannelInfo->GetTransportStreamID(),
									pChannelInfo->GetServiceID());
							}
						}
					}
				} else {
					m_SearchSettings.ServiceList.Clear();
				}
			}
			[[fallthrough]];
		case IDCANCEL:
			::EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		{
			const NMHDR *pnmh = reinterpret_cast<const NMHDR*>(lParam);

			switch (pnmh->code) {
			case LVN_GETINFOTIP:
				if (pnmh->idFrom == IDC_FEATUREDEVENTSSEARCH_SERVICELIST) {
					return ServiceListViewGetInfoTip(
						reinterpret_cast<NMLVGETINFOTIP*>(lParam), m_ServiceList);
				}
				break;

			case LVN_LINKCLICK:
				if (pnmh->idFrom == IDC_FEATUREDEVENTSSEARCH_SERVICELIST) {
					return ServiceListViewOnLinkClick(
						hDlg, reinterpret_cast<NMLVLINK*>(lParam));
				}
				break;
			}
		}
		break;
	}

	return FALSE;
}




CFeaturedEventsDialog::CFeaturedEventsDialog(
	CFeaturedEventsSettings &Settings,
	CEventSearchOptions &Options)
	: m_Settings(Settings)
	, m_Options(Options)
{
	m_SettingsColumnWidth[SETTINGS_COLUMN_NAME] = 120;
	m_SettingsColumnWidth[SETTINGS_COLUMN_KEYWORD] = 200;
	m_SettingsColumnWidth[SETTINGS_COLUMN_GENRE] = 200;
}


bool CFeaturedEventsDialog::Show(HWND hwndOwner)
{
	return ShowDialog(
		hwndOwner,
		GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE(IDD_FEATUREDEVENTS)) == IDOK;
}


INT_PTR CFeaturedEventsDialog::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static constexpr int PERIOD_UNIT = 60 * 60;

	switch (uMsg) {
	case WM_INITDIALOG:
		AddControl(IDC_FEATUREDEVENTS_SERVICELIST, AlignFlag::Vert);
		AddControls(
			IDC_FEATUREDEVENTS_SERVICELIST_CHECKALL,
			IDC_FEATUREDEVENTS_SERVICELIST_UNCHECKALL,
			AlignFlag::Bottom);
		AddControl(IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST, AlignFlag::All);
		AddControls(
			IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_EDIT,
			IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_DOWN,
			AlignFlag::Bottom);
		AddControls(
			IDC_FEATUREDEVENTS_PERIOD_LABEL,
			IDC_FEATUREDEVENTS_PERIOD_UNIT,
			AlignFlag::Bottom);
		AddControls(
			IDC_FEATUREDEVENTS_EVENTTEXTLINES_LABEL,
			IDC_FEATUREDEVENTS_EVENTTEXTLINES_UNIT,
			AlignFlag::Bottom);
		AddControl(IDOK, AlignFlag::BottomRight);
		AddControl(IDCANCEL, AlignFlag::BottomRight);

		GetAppClass().DriverManager.GetAllServiceList(&m_ServiceList);
		InitServiceListView(
			::GetDlgItem(hDlg, IDC_FEATUREDEVENTS_SERVICELIST),
			m_ServiceList, m_Settings.GetDefaultServiceList(),
			m_CurrentDPI);

		{
			const CEventSearchSettingsList &SettingsList = m_Settings.GetSearchSettingsList();
			const HWND hwndList = ::GetDlgItem(hDlg, IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST);

			ListView_SetExtendedListViewStyle(
				hwndList, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
			SetListViewTooltipsTopMost(hwndList);

			LVCOLUMN lvc;
			lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
			lvc.fmt = LVCFMT_LEFT;
			lvc.cx = m_SettingsColumnWidth[SETTINGS_COLUMN_NAME];
			lvc.pszText = const_cast<LPTSTR>(TEXT("名前"));
			lvc.iSubItem = 0;
			ListView_InsertColumn(hwndList, 0, &lvc);
			lvc.cx = m_SettingsColumnWidth[SETTINGS_COLUMN_KEYWORD];
			lvc.pszText = const_cast<LPTSTR>(TEXT("キーワード"));
			lvc.iSubItem = 1;
			ListView_InsertColumn(hwndList, 1, &lvc);
			lvc.cx = m_SettingsColumnWidth[SETTINGS_COLUMN_GENRE];
			lvc.pszText = const_cast<LPTSTR>(TEXT("ジャンル"));
			lvc.iSubItem = 2;
			ListView_InsertColumn(hwndList, 2, &lvc);

			for (int i = 0; i < static_cast<int>(SettingsList.GetCount()); i++) {
				AddSearchSettingsItem(hDlg, *SettingsList.Get(i));
			}

			SetSearchSettingsListItemStatus(hDlg);
		}

		DlgEdit_SetInt(hDlg, IDC_FEATUREDEVENTS_PERIOD, m_Settings.GetPeriodSeconds() / PERIOD_UNIT);
		DlgUpDown_SetRange(hDlg, IDC_FEATUREDEVENTS_PERIOD_SPIN, 1, 7 * 24);

		DlgEdit_SetInt(hDlg, IDC_FEATUREDEVENTS_EVENTTEXTLINES, m_Settings.GetEventTextLines());
		DlgUpDown_SetRange(
			hDlg, IDC_FEATUREDEVENTS_EVENTTEXTLINES_SPIN,
			1, CFeaturedEventsSettings::MAX_EVENT_TEXT_LINES);

		ApplyPosition();

		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_FEATUREDEVENTS_SERVICELIST_CHECKALL:
		case IDC_FEATUREDEVENTS_SERVICELIST_UNCHECKALL:
			{
				const HWND hwndList = ::GetDlgItem(hDlg, IDC_FEATUREDEVENTS_SERVICELIST);
				const int ItemCount = ListView_GetItemCount(hwndList);
				const BOOL fCheck = LOWORD(wParam) == IDC_FEATUREDEVENTS_SERVICELIST_CHECKALL;

				for (int i = 0; i < ItemCount; i++) {
					ListView_SetCheckState(hwndList, i, fCheck);
				}
			}
			return TRUE;

		case IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_EDIT:
			{
				const HWND hwndList = ::GetDlgItem(hDlg, IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST);
				const int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

				if (Sel >= 0) {
					LVITEM lvi;
					lvi.mask = LVIF_PARAM;
					lvi.iItem = Sel;
					lvi.iSubItem = 0;
					if (ListView_GetItem(hwndList, &lvi)) {
						CEventSearchSettings *pSettings =
							reinterpret_cast<CEventSearchSettings*>(lvi.lParam);
						CFeaturedEventsSearchDialog Dialog(*pSettings, m_Options, m_ServiceList);

						if (Dialog.Show(hDlg)) {
							UpdateSearchSettingsItem(hDlg, Sel);
						}
					}
				}
			}
			return TRUE;

		case IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_ADD:
			{
				CEventSearchSettings Settings;
				CFeaturedEventsSearchDialog Dialog(Settings, m_Options, m_ServiceList);

				if (Dialog.Show(hDlg)) {
					const HWND hwndList = ::GetDlgItem(hDlg, IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST);
					const int Item = AddSearchSettingsItem(hDlg, Settings);
					ListView_SetItemState(
						hwndList, Item,
						LVIS_FOCUSED | LVIS_SELECTED,
						LVIS_FOCUSED | LVIS_SELECTED);
					SetSearchSettingsListItemStatus(hDlg);
				}
			}
			return TRUE;

		case IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_DUPLICATE:
			{
				const HWND hwndList = ::GetDlgItem(hDlg, IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST);
				const int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

				if (Sel >= 0) {
					LVITEM lvi;
					lvi.mask = LVIF_STATE | LVIF_PARAM;
					lvi.iItem = Sel;
					lvi.iSubItem = 0;
					lvi.stateMask = ~0U;
					if (ListView_GetItem(hwndList, &lvi)) {
						CEventSearchSettings Settings(
							*reinterpret_cast<CEventSearchSettings*>(lvi.lParam));
						CFeaturedEventsSearchDialog Dialog(Settings, m_Options, m_ServiceList);

						if (Dialog.Show(hDlg)) {
							const int Item = AddSearchSettingsItem(hDlg, Settings);
							ListView_SetItemState(
								hwndList, Item,
								LVIS_FOCUSED | LVIS_SELECTED | (lvi.state & LVIS_STATEIMAGEMASK),
								LVIS_FOCUSED | LVIS_SELECTED | LVIS_STATEIMAGEMASK);
							SetSearchSettingsListItemStatus(hDlg);
						}
					}
				}
			}
			return TRUE;

		case IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_UP:
		case IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_DOWN:
			{
				const HWND hwndList = ::GetDlgItem(hDlg, IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST);
				const int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
				int To;

				if (LOWORD(wParam) == IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_UP) {
					if (Sel < 1)
						return TRUE;
					To = Sel - 1;
				} else {
					if (Sel < 0 || Sel >= ListView_GetItemCount(hwndList) - 1)
						return TRUE;
					To = Sel + 1;
				}

				LVITEM lvi;
				lvi.mask = LVIF_STATE | LVIF_PARAM;
				lvi.iItem = Sel;
				lvi.iSubItem = 0;
				lvi.stateMask = ~0U;
				ListView_GetItem(hwndList, &lvi);
				ListView_DeleteItem(hwndList, Sel);
				lvi.iItem = To;
				ListView_InsertItem(hwndList, &lvi);
				UpdateSearchSettingsItem(hDlg, To);
				ListView_SetItemState(
					hwndList, To,
					LVIS_FOCUSED | LVIS_SELECTED | (lvi.state & LVIS_STATEIMAGEMASK),
					LVIS_FOCUSED | LVIS_SELECTED | LVIS_STATEIMAGEMASK);
				SetSearchSettingsListItemStatus(hDlg);
			}
			return TRUE;

		case IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_DELETE:
			{
				const HWND hwndList = ::GetDlgItem(hDlg, IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST);
				const int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

				if (Sel >= 0) {
					LVITEM lvi;
					lvi.mask = LVIF_PARAM;
					lvi.iItem = Sel;
					lvi.iSubItem = 0;
					if (ListView_GetItem(hwndList, &lvi)) {
						ListView_DeleteItem(hwndList, Sel);
						delete reinterpret_cast<CEventSearchSettings*>(lvi.lParam);
						SetSearchSettingsListItemStatus(hDlg);
					}
				}
			}
			return TRUE;

		case IDOK:
			{
				CEventSearchServiceList &DefaultServiceList = m_Settings.GetDefaultServiceList();
				const HWND hwndList = ::GetDlgItem(hDlg, IDC_FEATUREDEVENTS_SERVICELIST);
				const int ItemCount = ListView_GetItemCount(hwndList);

				DefaultServiceList.Clear();

				LVITEM lvi;
				lvi.mask = LVIF_PARAM;
				lvi.iSubItem = 0;
				for (int i = 0; i < ItemCount; i++) {
					if (ListView_GetCheckState(hwndList, i)) {
						lvi.iItem = i;
						ListView_GetItem(hwndList, &lvi);
						const CChannelInfo *pChannelInfo = m_ServiceList.GetChannelInfo(static_cast<int>(lvi.lParam));
						if (pChannelInfo != nullptr) {
							DefaultServiceList.Add(
								pChannelInfo->GetNetworkID(),
								pChannelInfo->GetTransportStreamID(),
								pChannelInfo->GetServiceID());
						}
					}
				}
			}

			{
				CEventSearchSettingsList &SettingsList = m_Settings.GetSearchSettingsList();
				const HWND hwndList = ::GetDlgItem(hDlg, IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST);
				const int ItemCount = ListView_GetItemCount(hwndList);

				SettingsList.Clear();

				LVITEM lvi;
				lvi.mask = LVIF_PARAM;
				lvi.iSubItem = 0;
				for (int i = 0; i < ItemCount; i++) {
					lvi.iItem = i;
					ListView_GetItem(hwndList, &lvi);
					CEventSearchSettings *pSettings =
						reinterpret_cast<CEventSearchSettings*>(lvi.lParam);
					pSettings->fDisabled = !ListView_GetCheckState(hwndList, i);
					SettingsList.Add(*pSettings);
				}
			}

			m_Settings.SetPeriodSeconds(
				DlgEdit_GetInt(hDlg, IDC_FEATUREDEVENTS_PERIOD) * PERIOD_UNIT);

			{
				const int Lines = DlgEdit_GetInt(hDlg, IDC_FEATUREDEVENTS_EVENTTEXTLINES);

				if (Lines > 0 && Lines <= CFeaturedEventsSettings::MAX_EVENT_TEXT_LINES)
					m_Settings.SetEventTextLines(Lines);
			}

			[[fallthrough]];
		case IDCANCEL:
			::EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		{
			const NMHDR *pnmh = reinterpret_cast<const NMHDR*>(lParam);

			switch (pnmh->code) {
			case LVN_GETINFOTIP:
				if (pnmh->idFrom == IDC_FEATUREDEVENTS_SERVICELIST) {
					return ServiceListViewGetInfoTip(
						reinterpret_cast<NMLVGETINFOTIP*>(lParam), m_ServiceList);
				}
				break;

			case LVN_LINKCLICK:
				if (pnmh->idFrom == IDC_FEATUREDEVENTS_SERVICELIST) {
					return ServiceListViewOnLinkClick(
						hDlg, reinterpret_cast<NMLVLINK*>(lParam));
				}
				break;

			case LVN_ITEMCHANGED:
				if (pnmh->idFrom == IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST) {
					SetSearchSettingsListItemStatus(hDlg);
					return TRUE;
				}
				break;

			case NM_DBLCLK:
				if (pnmh->idFrom == IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST) {
					const NMITEMACTIVATE *pnmia = reinterpret_cast<const NMITEMACTIVATE*>(lParam);
					if (pnmia->iItem >= 0) {
						::SendMessage(hDlg, WM_COMMAND, IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_EDIT, 0);
					}
					return TRUE;
				}
				break;
			}
		}
		break;

	case WM_DESTROY:
		m_ServiceList.Clear();

		{
			const HWND hwndList = ::GetDlgItem(hDlg, IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST);

			for (int i = 0; i < lengthof(m_SettingsColumnWidth); i++)
				m_SettingsColumnWidth[i] = ListView_GetColumnWidth(hwndList, i);

			const int ItemCount = ListView_GetItemCount(hwndList);
			LVITEM lvi;
			lvi.mask = LVIF_PARAM;
			lvi.iSubItem = 0;
			for (int i = 0; i < ItemCount; i++) {
				lvi.iItem = i;
				ListView_GetItem(hwndList, &lvi);
				delete reinterpret_cast<CEventSearchSettings*>(lvi.lParam);
			}
			ListView_DeleteAllItems(hwndList);
		}
		return TRUE;
	}

	return FALSE;
}


void CFeaturedEventsDialog::SetSearchSettingsListItemStatus(HWND hDlg)
{
	const HWND hwndList = ::GetDlgItem(hDlg, IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST);
	const int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

	EnableDlgItem(hDlg, IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_EDIT, Sel >= 0);
	EnableDlgItem(hDlg, IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_DUPLICATE, Sel >= 0);
	EnableDlgItem(hDlg, IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_UP, Sel > 0);
	EnableDlgItem(hDlg, IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_DOWN, Sel >= 0 && Sel < ListView_GetItemCount(hwndList) - 1);
	EnableDlgItem(hDlg, IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_DELETE, Sel >= 0);
}


int CFeaturedEventsDialog::AddSearchSettingsItem(HWND hDlg, const CEventSearchSettings &Settings)
{
	const HWND hwndList = ::GetDlgItem(hDlg, IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST);
	const CEventSearchSettings *pSettings = new CEventSearchSettings(Settings);

	LVITEM lvi;
	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iItem = ListView_GetItemCount(hwndList);
	lvi.iSubItem = 0;
	lvi.pszText = const_cast<LPTSTR>(TEXT(""));
	lvi.lParam = reinterpret_cast<LPARAM>(pSettings);
	const int Item = ListView_InsertItem(hwndList, &lvi);
	UpdateSearchSettingsItem(hDlg, Item);
	return Item;
}


void CFeaturedEventsDialog::UpdateSearchSettingsItem(HWND hDlg, int Item)
{
	const HWND hwndList = ::GetDlgItem(hDlg, IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST);

	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iItem = Item;
	lvi.iSubItem = 0;
	if (!ListView_GetItem(hwndList, &lvi))
		return;

	const CEventSearchSettings *pSettings =
		reinterpret_cast<const CEventSearchSettings *>(lvi.lParam);

	ListView_SetCheckState(hwndList, Item, !pSettings->fDisabled);
	ListView_SetItemText(hwndList, Item, 0, const_cast<LPTSTR>(pSettings->Name.c_str()));
	ListView_SetItemText(hwndList, Item, 1, const_cast<LPTSTR>(pSettings->Keyword.c_str()));

	CEpgGenre EpgGenre;
	String Genre;

	for (int i = 0; i < CEpgGenre::NUM_GENRE; i++) {
		if ((pSettings->Genre1 & (1 << i)) != 0 || pSettings->Genre2[i] != 0) {
			LPCTSTR pszText = EpgGenre.GetText(i, -1);
			if (pszText != nullptr) {
				Genre += pszText;
				bool fFirst = true;
				for (int j = 0; j < CEpgGenre::NUM_SUB_GENRE; j++) {
					if ((pSettings->Genre2[i] & (1 << j)) != 0) {
						pszText = EpgGenre.GetText(i, j);
						if (pszText != nullptr) {
							if (fFirst) {
								Genre += TEXT("（");
								fFirst = false;
							} else {
								Genre += TEXT("、");
							}
							Genre += pszText;
						}
					}
				}
				if (!fFirst)
					Genre += TEXT("）");
			}
		}
	}

	ListView_SetItemText(hwndList, Item, 2, const_cast<LPTSTR>(Genre.c_str()));
}




CFeaturedEvents::CFeaturedEvents(CEventSearchOptions &EventSearchOptions)
	: m_Dialog(m_Settings, EventSearchOptions)
{
}


bool CFeaturedEvents::LoadSettings(CSettings &Settings)
{
	m_Settings.LoadSettings(Settings);

	if (Settings.SetSection(TEXT("FeaturedEvents"))) {
		CBasicDialog::Position Pos;
		if (Settings.Read(TEXT("DialogLeft"), &Pos.x)
				&& Settings.Read(TEXT("DialogTop"), &Pos.y)) {
			Settings.Read(TEXT("DialogWidth"), &Pos.Width);
			Settings.Read(TEXT("DialogHeight"), &Pos.Height);
			m_Dialog.SetPosition(Pos);
		}
	}

	return true;
}


bool CFeaturedEvents::SaveSettings(CSettings &Settings)
{
	m_Settings.SaveSettings(Settings);

	if (Settings.SetSection(TEXT("FeaturedEvents"))) {
		if (m_Dialog.IsPositionSet()) {
			CBasicDialog::Position Pos;
			m_Dialog.GetPosition(&Pos);
			Settings.Write(TEXT("DialogLeft"), Pos.x);
			Settings.Write(TEXT("DialogTop"), Pos.y);
			Settings.Write(TEXT("DialogWidth"), Pos.Width);
			Settings.Write(TEXT("DialogHeight"), Pos.Height);
		}
	}

	return true;
}


bool CFeaturedEvents::AddEventHandler(CEventHandler *pEventHandler)
{
	if (pEventHandler == nullptr)
		return false;

	m_EventHandlerList.push_back(pEventHandler);

	return true;
}


bool CFeaturedEvents::RemoveEventHandler(CEventHandler *pEventHandler)
{
	for (auto itr = m_EventHandlerList.begin(); itr != m_EventHandlerList.end(); ++itr) {
		if (*itr == pEventHandler) {
			m_EventHandlerList.erase(itr);
			return true;
		}
	}

	return false;
}


bool CFeaturedEvents::ShowDialog(HWND hwndOwner)
{
	if (!m_Dialog.Show(hwndOwner))
		return false;

	for (CEventHandler *pHandler : m_EventHandlerList)
		pHandler->OnFeaturedEventsSettingsChanged(*this);

	return true;
}


}	// namespace TVTest
