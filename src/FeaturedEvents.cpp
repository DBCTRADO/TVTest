#include "stdafx.h"
#include "TVTest.h"
#include "FeaturedEvents.h"
#include "AppMain.h"
#include "DriverManager.h"
#include "DialogUtil.h"
#include "LogoManager.h"
#include "EpgUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CFeaturedEventsSettings::CFeaturedEventsSettings()
	: CSettingsBase(TEXT("FeaturedEvents"))
	, m_SortType(SORT_TIME)
	, m_PeriodSeconds(24*60*60)
	, m_fShowEventText(true)
	, m_EventTextLines(2)
{
}


CFeaturedEventsSettings::~CFeaturedEventsSettings()
{
}


bool CFeaturedEventsSettings::ReadSettings(CSettings &Settings)
{
	TVTest::String Services;
	if (Settings.Read(TEXT("DefaultServices"),&Services))
		m_DefaultServiceList.FromString(Services.c_str());

	m_SearchSettingsList.Load(Settings,TEXT("Settings"));

	int Sort;
	if (Settings.Read(TEXT("Sort"),&Sort)
			&& Sort>=SORT_FIRST && Sort<=SORT_LAST)
		m_SortType=(SortType)Sort;

	Settings.Read(TEXT("Period"),&m_PeriodSeconds);
	Settings.Read(TEXT("ShowEventText"),&m_fShowEventText);

	int Lines;
	if (Settings.Read(TEXT("EventTextLines"),&Lines)
			&& Lines>0 && Lines<=MAX_EVENT_TEXT_LINES)
		m_EventTextLines=Lines;

	return true;
}


bool CFeaturedEventsSettings::WriteSettings(CSettings &Settings)
{
	Settings.Clear();

	TVTest::String Services;
	m_DefaultServiceList.ToString(&Services);
	Settings.Write(TEXT("DefaultServices"),Services);

	m_SearchSettingsList.Save(Settings,TEXT("Settings"));

	Settings.Write(TEXT("Sort"),(int)m_SortType);
	Settings.Write(TEXT("Period"),m_PeriodSeconds);
	Settings.Write(TEXT("ShowEventText"),m_fShowEventText);
	Settings.Write(TEXT("EventTextLines"),m_EventTextLines);

	return true;
}




CFeaturedEvents::CFeaturedEvents(const CFeaturedEventsSettings &Settings)
	: m_Settings(Settings)
{
}


CFeaturedEvents::~CFeaturedEvents()
{
	Clear();
}


void CFeaturedEvents::Clear()
{
	for (auto it=m_EventList.begin();it!=m_EventList.end();++it)
		delete *it;
	m_EventList.clear();
}


bool CFeaturedEvents::Update()
{
	Clear();

	SYSTEMTIME StartTime,PeriodTime;
	GetCurrentJST(&StartTime);
	PeriodTime=StartTime;
	OffsetSystemTime(&PeriodTime,(LONGLONG)m_Settings.GetPeriodSeconds()*1000);

	const CEventSearchServiceList &DefaultServiceList=m_Settings.GetDefaultServiceList();
	CEventSearchServiceList ServiceIDList(DefaultServiceList);

	const CEventSearchSettingsList &SearchSettingsList=m_Settings.GetSearchSettingsList();
	std::vector<CEventSearcher> SearcherList;
	SearcherList.resize(SearchSettingsList.GetEnabledCount());

	for (size_t i=0;i<SearchSettingsList.GetCount();i++) {
		const CEventSearchSettings *pSettings=SearchSettingsList.Get(i);

		if (!pSettings->fDisabled) {
			if (pSettings->fServiceList) {
				ServiceIDList.Combine(pSettings->ServiceList);
			}

			SearcherList[i].BeginSearch(*pSettings);
		}
	}

	CEpgProgramList *pEpgProgramList=GetAppClass().GetEpgProgramList();

	for (auto itService=ServiceIDList.Begin();itService!=ServiceIDList.End();++itService) {
		const CEpgServiceInfo *pServiceInfo=pEpgProgramList->GetServiceInfo(
			CEventSearchServiceList::ServiceKey_GetNetworkID(*itService),
			CEventSearchServiceList::ServiceKey_GetTransportStreamID(*itService),
			CEventSearchServiceList::ServiceKey_GetServiceID(*itService));
		if (pServiceInfo!=NULL) {
			const CEventInfoList &EventList=pServiceInfo->m_EventList;
			for (auto itEvent=EventList.EventDataMap.begin();itEvent!=EventList.EventDataMap.end();++itEvent) {
				if (itEvent->second.m_fCommonEvent)
					continue;
				if (CompareSystemTime(&itEvent->second.m_stStartTime,&PeriodTime)>=0)
					continue;
				SYSTEMTIME stEnd;
				itEvent->second.GetEndTime(&stEnd);
				if (CompareSystemTime(&stEnd,&StartTime)<=0)
					continue;
				for (auto itSearcher=SearcherList.begin();itSearcher!=SearcherList.end();++itSearcher) {
					if (!itSearcher->GetSearchSettings().fServiceList) {
						if (!DefaultServiceList.IsExists(*itService))
							continue;
					}
					if (itSearcher->Match(&itEvent->second)) {
						m_EventList.push_back(new CEventInfoData(itEvent->second));
						break;
					}
				}
			}
		}
	}

	return true;
}


size_t CFeaturedEvents::GetEventCount() const
{
	return m_EventList.size();
}


const CEventInfoData *CFeaturedEvents::GetEventInfo(size_t Index) const
{
	if (Index>=m_EventList.size())
		return NULL;

	return m_EventList[Index];
}




static void InitServiceListView(
	HWND hwndList,
	const CChannelList &ServiceList,
	const CEventSearchServiceList &SearchServiceList)
{
	if (Util::OS::IsWindowsVistaOrLater())
		::SetWindowTheme(hwndList,L"explorer",NULL);
	ListView_SetExtendedListViewStyle(hwndList,
		LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	const int IconWidth=::GetSystemMetrics(SM_CXSMICON);
	const int IconHeight=::GetSystemMetrics(SM_CYSMICON);
	HIMAGELIST himl=::ImageList_Create(IconWidth,IconHeight,ILC_COLOR24 | ILC_MASK,
									   ServiceList.NumChannels()+1,100);
	ImageList_AddIcon(himl,CreateEmptyIcon(IconWidth,IconHeight));
	ListView_SetImageList(hwndList,himl,LVSIL_SMALL);
	CLogoManager *pLogoManager=GetAppClass().GetLogoManager();

	enum {
		GROUP_ID_TERRESTRIAL=1,
		GROUP_ID_BS,
		GROUP_ID_CS
	};
	LVGROUP lvg;
	lvg.cbSize=LVGROUP_V5_SIZE;
	lvg.mask=LVGF_HEADER | LVGF_GROUPID;
	if (Util::OS::IsWindowsVistaOrLater()) {
		lvg.mask|=LVGF_STATE;
		lvg.stateMask=~0U;
		lvg.state=LVGS_COLLAPSIBLE;
	}
	lvg.pszHeader=TEXT("地上");
	lvg.iGroupId=GROUP_ID_TERRESTRIAL;
	ListView_InsertGroup(hwndList,-1,&lvg);
	lvg.pszHeader=TEXT("BS");
	lvg.iGroupId=GROUP_ID_BS;
	ListView_InsertGroup(hwndList,-1,&lvg);
	lvg.pszHeader=TEXT("CS");
	lvg.iGroupId=GROUP_ID_CS;
	ListView_InsertGroup(hwndList,-1,&lvg);

	LVCOLUMN lvc;
	RECT rc;
	::GetClientRect(hwndList,&rc);
	lvc.mask=LVCF_FMT | LVCF_WIDTH;
	lvc.fmt=LVCFMT_LEFT;
	lvc.cx=rc.right-::GetSystemMetrics(SM_CXVSCROLL);
	ListView_InsertColumn(hwndList,0,&lvc);

	LVITEM lvi;
	lvi.mask=LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_GROUPID;
	lvi.iSubItem=0;
	for (int i=0;i<ServiceList.NumChannels();i++) {
		const CChannelInfo *pChannelInfo=ServiceList.GetChannelInfo(i);

		lvi.iItem=i;
		lvi.pszText=const_cast<LPTSTR>(pChannelInfo->GetName());

		HICON hico=pLogoManager->CreateLogoIcon(
			pChannelInfo->GetNetworkID(),
			pChannelInfo->GetServiceID(),
			IconWidth,IconHeight);
		if (hico!=NULL) {
			lvi.iImage=ImageList_AddIcon(himl,hico);
			::DestroyIcon(hico);
		} else {
			lvi.iImage=0;
		}

		lvi.lParam=i;

		switch (GetNetworkType(pChannelInfo->GetNetworkID())) {
		case NETWORK_TERRESTRIAL:	lvi.iGroupId=GROUP_ID_TERRESTRIAL;	break;
		case NETWORK_BS:			lvi.iGroupId=GROUP_ID_BS;			break;
		case NETWORK_CS:			lvi.iGroupId=GROUP_ID_CS;			break;
		}

		int Index=ListView_InsertItem(hwndList,&lvi);

		ListView_SetCheckState(hwndList,Index,
			SearchServiceList.IsExists(
				pChannelInfo->GetNetworkID(),
				pChannelInfo->GetTransportStreamID(),
				pChannelInfo->GetServiceID()));
	}

	ListView_EnableGroupView(hwndList,TRUE);
}


static BOOL ServiceListViewGetInfoTip(
	NMLVGETINFOTIP *pGetInfoTip,
	const CChannelList &ServiceList)
{
	LVITEM lvi;
	lvi.mask=LVIF_PARAM;
	lvi.iItem=pGetInfoTip->iItem;
	lvi.iSubItem=0;
	if (!ListView_GetItem(pGetInfoTip->hdr.hwndFrom,&lvi))
		return FALSE;

	const CChannelInfo *pChannelInfo=ServiceList.GetChannelInfo((int)lvi.lParam);
	if (pChannelInfo==NULL)
		return FALSE;

	StdUtil::snprintf(pGetInfoTip->pszText,pGetInfoTip->cchTextMax,
		TEXT("%s\r\nサービス: %d (0x%04x)\r\nネットワークID: %d (0x%04x)\r\nTSID: %d (0x%04x)"),
		pChannelInfo->GetName(),
		pChannelInfo->GetServiceID(),pChannelInfo->GetServiceID(),
		pChannelInfo->GetNetworkID(),pChannelInfo->GetNetworkID(),
		pChannelInfo->GetTransportStreamID(),pChannelInfo->GetTransportStreamID());

	return TRUE;
}




class CFeaturedEventsSearchDialog : public CBasicDialog
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
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

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
}


bool CFeaturedEventsSearchDialog::Show(HWND hwndOwner)
{
	return ShowDialog(hwndOwner,
					  GetAppClass().GetResourceInstance(),
					  MAKEINTRESOURCE(IDD_FEATUREDEVENTSSEARCH))==IDOK;
}


INT_PTR CFeaturedEventsSearchDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		m_SearchSettingsDialog.SetSettings(m_SearchSettings);
		m_SearchSettingsDialog.Create(hDlg);
		m_SearchSettingsDialog.ShowButton(IDC_EVENTSEARCH_HIGHLIGHT,false);
		m_SearchSettingsDialog.ShowButton(IDC_EVENTSEARCH_SEARCH,false);
		{
			RECT rc;
			GetDlgItemRect(hDlg,IDC_FEATUREDEVENTSSEARCH_SETTINGSPLACE,&rc);
			m_SearchSettingsDialog.SetPosition(&rc);
		}

		DlgEdit_LimitText(hDlg,IDC_FEATUREDEVENTSSEARCH_NAME,CEventSearchSettings::MAX_NAME_LENGTH-1);
		DlgEdit_SetText(hDlg,IDC_FEATUREDEVENTSSEARCH_NAME,m_SearchSettings.Name.c_str());

		InitServiceListView(::GetDlgItem(hDlg,IDC_FEATUREDEVENTSSEARCH_SERVICELIST),
							m_ServiceList,m_SearchSettings.ServiceList);

		DlgCheckBox_Check(hDlg,IDC_FEATUREDEVENTSSEARCH_USESERVICELIST,m_SearchSettings.fServiceList);
		EnableDlgItems(hDlg,
					   IDC_FEATUREDEVENTSSEARCH_SERVICELIST,
					   IDC_FEATUREDEVENTSSEARCH_SERVICELIST_UNCHECKALL,
					   m_SearchSettings.fServiceList);
		return TRUE;

	case WM_SIZE:
		{
			RECT rc;
			GetDlgItemRect(hDlg,IDC_FEATUREDEVENTSSEARCH_SETTINGSPLACE,&rc);
			m_SearchSettingsDialog.SetPosition(&rc);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_FEATUREDEVENTSSEARCH_USESERVICELIST:
			EnableDlgItemsSyncCheckBox(hDlg,
									   IDC_FEATUREDEVENTSSEARCH_SERVICELIST,
									   IDC_FEATUREDEVENTSSEARCH_SERVICELIST_UNCHECKALL,
									   IDC_FEATUREDEVENTSSEARCH_USESERVICELIST);
			return TRUE;

		case IDC_FEATUREDEVENTSSEARCH_SERVICELIST_CHECKALL:
		case IDC_FEATUREDEVENTSSEARCH_SERVICELIST_UNCHECKALL:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_FEATUREDEVENTSSEARCH_SERVICELIST);
				const int ItemCount=ListView_GetItemCount(hwndList);
				const BOOL fCheck=LOWORD(wParam)==IDC_FEATUREDEVENTSSEARCH_SERVICELIST_CHECKALL;

				for (int i=0;i<ItemCount;i++) {
					ListView_SetCheckState(hwndList,i,fCheck);
				}
			}
			return TRUE;

		case IDOK:
			{
				m_SearchSettingsDialog.GetSettings(&m_SearchSettings);

				TCHAR szName[CEventSearchSettings::MAX_NAME_LENGTH];
				DlgEdit_GetText(hDlg,IDC_FEATUREDEVENTSSEARCH_NAME,szName,lengthof(szName));
				m_SearchSettings.Name=szName;

				m_SearchSettings.fServiceList=
					DlgCheckBox_IsChecked(hDlg,IDC_FEATUREDEVENTSSEARCH_USESERVICELIST);

				if (m_SearchSettings.fServiceList) {
					HWND hwndList=::GetDlgItem(hDlg,IDC_FEATUREDEVENTSSEARCH_SERVICELIST);
					const int ItemCount=ListView_GetItemCount(hwndList);
					LVITEM lvi;
					lvi.mask=LVIF_PARAM;
					lvi.iSubItem=0;
					for (int i=0;i<ItemCount;i++) {
						if (ListView_GetCheckState(hwndList,i)) {
							lvi.iItem=i;
							ListView_GetItem(hwndList,&lvi);
							const CChannelInfo *pChannelInfo=m_ServiceList.GetChannelInfo((int)lvi.lParam);
							if (pChannelInfo!=NULL) {
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
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		{
			LPNMHDR pnmh=reinterpret_cast<LPNMHDR>(lParam);

			switch (pnmh->code) {
			case LVN_GETINFOTIP:
				if (pnmh->idFrom==IDC_FEATUREDEVENTSSEARCH_SERVICELIST) {
					return ServiceListViewGetInfoTip(
						reinterpret_cast<NMLVGETINFOTIP*>(lParam),m_ServiceList);
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
	m_SettingsColumnWidth[SETTINGS_COLUMN_NAME]=120;
	m_SettingsColumnWidth[SETTINGS_COLUMN_KEYWORD]=200;
	m_SettingsColumnWidth[SETTINGS_COLUMN_GENRE]=200;
}


CFeaturedEventsDialog::~CFeaturedEventsDialog()
{
}


bool CFeaturedEventsDialog::Show(HWND hwndOwner)
{
	return ShowDialog(hwndOwner,
					  GetAppClass().GetResourceInstance(),
					  MAKEINTRESOURCE(IDD_FEATUREDEVENTS))==IDOK;
}


INT_PTR CFeaturedEventsDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static const int PERIOD_UNIT=60*60;

	INT_PTR Result=CResizableDialog::DlgProc(hDlg,uMsg,wParam,lParam);

	switch (uMsg) {
	case WM_INITDIALOG:
		AddControl(IDC_FEATUREDEVENTS_SERVICELIST,ALIGN_VERT);
		AddControls(IDC_FEATUREDEVENTS_SERVICELIST_CHECKALL,
					IDC_FEATUREDEVENTS_SERVICELIST_UNCHECKALL,
					ALIGN_BOTTOM);
		AddControl(IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST,ALIGN_ALL);
		AddControls(IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_EDIT,
					IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_DOWN,
					ALIGN_BOTTOM);
		AddControls(IDC_FEATUREDEVENTS_PERIOD_LABEL,
					IDC_FEATUREDEVENTS_PERIOD_UNIT,
					ALIGN_BOTTOM);
		AddControls(IDC_FEATUREDEVENTS_EVENTTEXTLINES_LABEL,
					IDC_FEATUREDEVENTS_EVENTTEXTLINES_UNIT,
					ALIGN_BOTTOM);
		AddControl(IDOK,ALIGN_BOTTOM_RIGHT);
		AddControl(IDCANCEL,ALIGN_BOTTOM_RIGHT);

		GetAppClass().GetDriverManager()->GetAllServiceList(&m_ServiceList);
		InitServiceListView(::GetDlgItem(hDlg,IDC_FEATUREDEVENTS_SERVICELIST),
							m_ServiceList,m_Settings.GetDefaultServiceList());

		{
			const CEventSearchSettingsList &SettingsList=m_Settings.GetSearchSettingsList();
			HWND hwndList=::GetDlgItem(hDlg,IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST);

			ListView_SetExtendedListViewStyle(hwndList,
				LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

			LVCOLUMN lvc;
			lvc.mask=LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
			lvc.fmt=LVCFMT_LEFT;
			lvc.cx=m_SettingsColumnWidth[SETTINGS_COLUMN_NAME];
			lvc.pszText=TEXT("名前");
			lvc.iSubItem=0;
			ListView_InsertColumn(hwndList,0,&lvc);
			lvc.cx=m_SettingsColumnWidth[SETTINGS_COLUMN_KEYWORD];
			lvc.pszText=TEXT("キーワード");
			lvc.iSubItem=1;
			ListView_InsertColumn(hwndList,1,&lvc);
			lvc.cx=m_SettingsColumnWidth[SETTINGS_COLUMN_GENRE];
			lvc.pszText=TEXT("ジャンル");
			lvc.iSubItem=2;
			ListView_InsertColumn(hwndList,2,&lvc);

			for (int i=0;i<(int)SettingsList.GetCount();i++) {
				AddSearchSettingsItem(hDlg,*SettingsList.Get(i));
			}

			SetSearchSettingsListItemStatus(hDlg);
		}

		DlgEdit_SetInt(hDlg,IDC_FEATUREDEVENTS_PERIOD,m_Settings.GetPeriodSeconds()/PERIOD_UNIT);
		DlgUpDown_SetRange(hDlg,IDC_FEATUREDEVENTS_PERIOD_SPIN,1,7*24);

		DlgEdit_SetInt(hDlg,IDC_FEATUREDEVENTS_EVENTTEXTLINES,m_Settings.GetEventTextLines());
		DlgUpDown_SetRange(hDlg,IDC_FEATUREDEVENTS_EVENTTEXTLINES_SPIN,
						   1,CFeaturedEventsSettings::MAX_EVENT_TEXT_LINES);

		ApplyPosition();

		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_FEATUREDEVENTS_SERVICELIST_CHECKALL:
		case IDC_FEATUREDEVENTS_SERVICELIST_UNCHECKALL:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_FEATUREDEVENTS_SERVICELIST);
				const int ItemCount=ListView_GetItemCount(hwndList);
				const BOOL fCheck=LOWORD(wParam)==IDC_FEATUREDEVENTS_SERVICELIST_CHECKALL;

				for (int i=0;i<ItemCount;i++) {
					ListView_SetCheckState(hwndList,i,fCheck);
				}
			}
			return TRUE;

		case IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_EDIT:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST);
				int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

				if (Sel>=0) {
					LVITEM lvi;
					lvi.mask=LVIF_PARAM;
					lvi.iItem=Sel;
					lvi.iSubItem=0;
					if (ListView_GetItem(hwndList,&lvi)) {
						CEventSearchSettings *pSettings=
							reinterpret_cast<CEventSearchSettings*>(lvi.lParam);
						CFeaturedEventsSearchDialog Dialog(*pSettings,m_Options,m_ServiceList);

						if (Dialog.Show(hDlg)) {
							UpdateSearchSettingsItem(hDlg,Sel);
						}
					}
				}
			}
			return TRUE;

		case IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_ADD:
			{
				CEventSearchSettings Settings;
				CFeaturedEventsSearchDialog Dialog(Settings,m_Options,m_ServiceList);

				if (Dialog.Show(hDlg)) {
					HWND hwndList=::GetDlgItem(hDlg,IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST);
					int Item=AddSearchSettingsItem(hDlg,Settings);
					ListView_SetItemState(hwndList,Item,
										  LVIS_FOCUSED | LVIS_SELECTED,
										  LVIS_FOCUSED | LVIS_SELECTED);
					SetSearchSettingsListItemStatus(hDlg);
				}
			}
			return TRUE;

		case IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_DUPLICATE:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST);
				int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

				if (Sel>=0) {
					LVITEM lvi;
					lvi.mask=LVIF_STATE | LVIF_PARAM;
					lvi.iItem=Sel;
					lvi.iSubItem=0;
					lvi.stateMask=~0U;
					if (ListView_GetItem(hwndList,&lvi)) {
						CEventSearchSettings Settings(
							*reinterpret_cast<CEventSearchSettings*>(lvi.lParam));
						CFeaturedEventsSearchDialog Dialog(Settings,m_Options,m_ServiceList);

						if (Dialog.Show(hDlg)) {
							int Item=AddSearchSettingsItem(hDlg,Settings);
							ListView_SetItemState(hwndList,Item,
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
				HWND hwndList=::GetDlgItem(hDlg,IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST);
				int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
				int To;

				if (LOWORD(wParam)==IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_UP) {
					if (Sel<1)
						return TRUE;
					To=Sel-1;
				} else {
					if (Sel<0 || Sel>=ListView_GetItemCount(hwndList)-1)
						return TRUE;
					To=Sel+1;
				}

				LVITEM lvi;
				lvi.mask=LVIF_STATE | LVIF_PARAM;
				lvi.iItem=Sel;
				lvi.iSubItem=0;
				lvi.stateMask=~0U;
				ListView_GetItem(hwndList,&lvi);
				ListView_DeleteItem(hwndList,Sel);
				lvi.iItem=To;
				ListView_InsertItem(hwndList,&lvi);
				UpdateSearchSettingsItem(hDlg,To);
				ListView_SetItemState(hwndList,To,
									  LVIS_FOCUSED | LVIS_SELECTED | (lvi.state & LVIS_STATEIMAGEMASK),
									  LVIS_FOCUSED | LVIS_SELECTED | LVIS_STATEIMAGEMASK);
				SetSearchSettingsListItemStatus(hDlg);
			}
			return TRUE;

		case IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_DELETE:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST);
				int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

				if (Sel>=0) {
					LVITEM lvi;
					lvi.mask=LVIF_PARAM;
					lvi.iItem=Sel;
					lvi.iSubItem=0;
					if (ListView_GetItem(hwndList,&lvi)) {
						ListView_DeleteItem(hwndList,Sel);
						delete reinterpret_cast<CEventSearchSettings*>(lvi.lParam);
						SetSearchSettingsListItemStatus(hDlg);
					}
				}
			}
			return TRUE;

		case IDOK:
			{
				CEventSearchServiceList &DefaultServiceList=m_Settings.GetDefaultServiceList();
				HWND hwndList=::GetDlgItem(hDlg,IDC_FEATUREDEVENTS_SERVICELIST);
				const int ItemCount=ListView_GetItemCount(hwndList);

				DefaultServiceList.Clear();

				LVITEM lvi;
				lvi.mask=LVIF_PARAM;
				lvi.iSubItem=0;
				for (int i=0;i<ItemCount;i++) {
					if (ListView_GetCheckState(hwndList,i)) {
						lvi.iItem=i;
						ListView_GetItem(hwndList,&lvi);
						const CChannelInfo *pChannelInfo=m_ServiceList.GetChannelInfo((int)lvi.lParam);
						if (pChannelInfo!=NULL) {
							DefaultServiceList.Add(
								pChannelInfo->GetNetworkID(),
								pChannelInfo->GetTransportStreamID(),
								pChannelInfo->GetServiceID());
						}
					}
				}
			}

			{
				CEventSearchSettingsList &SettingsList=m_Settings.GetSearchSettingsList();
				HWND hwndList=::GetDlgItem(hDlg,IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST);
				const int ItemCount=ListView_GetItemCount(hwndList);

				SettingsList.Clear();

				LVITEM lvi;
				lvi.mask=LVIF_PARAM;
				lvi.iSubItem=0;
				for (int i=0;i<ItemCount;i++) {
					lvi.iItem=i;
					ListView_GetItem(hwndList,&lvi);
					CEventSearchSettings *pSettings=
						reinterpret_cast<CEventSearchSettings*>(lvi.lParam);
					pSettings->fDisabled=!ListView_GetCheckState(hwndList,i);
					SettingsList.Add(*pSettings);
				}
			}

			m_Settings.SetPeriodSeconds(
				DlgEdit_GetInt(hDlg,IDC_FEATUREDEVENTS_PERIOD)*PERIOD_UNIT);

			{
				int Lines=DlgEdit_GetInt(hDlg,IDC_FEATUREDEVENTS_EVENTTEXTLINES);

				if (Lines>0 && Lines<=CFeaturedEventsSettings::MAX_EVENT_TEXT_LINES)
					m_Settings.SetEventTextLines(Lines);
			}

		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		{
			LPNMHDR pnmh=reinterpret_cast<LPNMHDR>(lParam);

			switch (pnmh->code) {
			case LVN_GETINFOTIP:
				if (pnmh->idFrom==IDC_FEATUREDEVENTS_SERVICELIST) {
					return ServiceListViewGetInfoTip(
						reinterpret_cast<NMLVGETINFOTIP*>(lParam),m_ServiceList);
				}
				break;

			case LVN_ITEMCHANGED:
				if (pnmh->idFrom==IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST) {
					SetSearchSettingsListItemStatus(hDlg);
					return TRUE;
				}
				break;

			case NM_DBLCLK:
				if (pnmh->idFrom==IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST) {
					LPNMITEMACTIVATE pnmia=reinterpret_cast<LPNMITEMACTIVATE>(lParam);
					if (pnmia->iItem>=0) {
						::SendMessage(hDlg,WM_COMMAND,IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_EDIT,0);
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
			HWND hwndList=::GetDlgItem(hDlg,IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST);

			for (int i=0;i<lengthof(m_SettingsColumnWidth);i++)
				m_SettingsColumnWidth[i]=ListView_GetColumnWidth(hwndList,i);

			const int ItemCount=ListView_GetItemCount(hwndList);
			LVITEM lvi;
			lvi.mask=LVIF_PARAM;
			lvi.iSubItem=0;
			for (int i=0;i<ItemCount;i++) {
				lvi.iItem=i;
				ListView_GetItem(hwndList,&lvi);
				delete reinterpret_cast<CEventSearchSettings*>(lvi.lParam);
			}
			ListView_DeleteAllItems(hwndList);
		}
		break;
	}

	return Result;
}


void CFeaturedEventsDialog::SetSearchSettingsListItemStatus(HWND hDlg)
{
	HWND hwndList=::GetDlgItem(hDlg,IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST);
	int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

	EnableDlgItem(hDlg,IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_EDIT,Sel>=0);
	EnableDlgItem(hDlg,IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_DUPLICATE,Sel>=0);
	EnableDlgItem(hDlg,IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_UP,Sel>0);
	EnableDlgItem(hDlg,IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_DOWN,Sel>=0 && Sel<ListView_GetItemCount(hwndList)-1);
	EnableDlgItem(hDlg,IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_DELETE,Sel>=0);
}


int CFeaturedEventsDialog::AddSearchSettingsItem(HWND hDlg,const CEventSearchSettings &Settings)
{
	HWND hwndList=::GetDlgItem(hDlg,IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST);
	CEventSearchSettings *pSettings=new CEventSearchSettings(Settings);

	LVITEM lvi;
	lvi.mask=LVIF_TEXT | LVIF_PARAM;
	lvi.iItem=ListView_GetItemCount(hwndList);
	lvi.iSubItem=0;
	lvi.pszText=TEXT("");
	lvi.lParam=reinterpret_cast<LPARAM>(pSettings);
	int Item=ListView_InsertItem(hwndList,&lvi);
	UpdateSearchSettingsItem(hDlg,Item);
	return Item;
}


void CFeaturedEventsDialog::UpdateSearchSettingsItem(HWND hDlg,int Item)
{
	HWND hwndList=::GetDlgItem(hDlg,IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST);

	LVITEM lvi;
	lvi.mask=LVIF_PARAM;
	lvi.iItem=Item;
	lvi.iSubItem=0;
	if (!ListView_GetItem(hwndList,&lvi))
		return;

	const CEventSearchSettings *pSettings=
		reinterpret_cast<const CEventSearchSettings *>(lvi.lParam);

	ListView_SetCheckState(hwndList,Item,!pSettings->fDisabled);
	ListView_SetItemText(hwndList,Item,0,const_cast<LPTSTR>(pSettings->Name.c_str()));
	ListView_SetItemText(hwndList,Item,1,const_cast<LPTSTR>(pSettings->Keyword.c_str()));

	CEpgGenre EpgGenre;
	TVTest::String Genre;

	for (int i=0;i<CEpgGenre::NUM_GENRE;i++) {
		if ((pSettings->Genre1&(1<<i))!=0 || pSettings->Genre2[i]!=0) {
			LPCTSTR pszText=EpgGenre.GetText(i,-1);
			if (pszText!=NULL) {
				Genre+=pszText;
				bool fFirst=true;
				for (int j=0;j<CEpgGenre::NUM_SUB_GENRE;j++) {
					if ((pSettings->Genre2[i]&(1<<j))!=0) {
						pszText=EpgGenre.GetText(i,j);
						if (pszText!=NULL) {
							if (fFirst) {
								Genre+=TEXT("（");
								fFirst=false;
							} else {
								Genre+=TEXT("、");
							}
							Genre+=pszText;
						}
					}
				}
				if (!fFirst)
					Genre+=TEXT("）");
			}
		}
	}

	ListView_SetItemText(hwndList,Item,2,const_cast<LPTSTR>(Genre.c_str()));
}
