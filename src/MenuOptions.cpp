#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "MenuOptions.h"
#include "Menu.h"
#include "Command.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define ITEM_STATE_VISIBLE 0x0001


const CMenuOptions::MenuInfo CMenuOptions::m_DefaultMenuItemList[] =
{
	{CMainMenu::SUBMENU_ZOOM,			IDS_MENU_ZOOM,				CM_ZOOMMENU},
	{CMainMenu::SUBMENU_ASPECTRATIO,	IDS_MENU_ASPECTRATIO,		CM_ASPECTRATIOMENU},
	{CM_FULLSCREEN,						CM_FULLSCREEN,				CM_FULLSCREEN},
	{CM_ALWAYSONTOP,					CM_ALWAYSONTOP,				CM_ALWAYSONTOP},
	{MENU_ID_SEPARATOR,					0,							0},
	{CMainMenu::SUBMENU_CHANNEL,		IDS_MENU_CHANNEL,			CM_CHANNELMENU},
	{CMainMenu::SUBMENU_SERVICE,		IDS_MENU_SERVICE,			CM_SERVICEMENU},
	{CMainMenu::SUBMENU_SPACE,			IDS_MENU_TUNER,				CM_TUNINGSPACEMENU},
	{CMainMenu::SUBMENU_FAVORITES,		IDS_MENU_FAVORITES,			CM_FAVORITESMENU},
	{CMainMenu::SUBMENU_CHANNELHISTORY,	IDS_MENU_CHANNELHISTORY,	CM_RECENTCHANNELMENU},
	{MENU_ID_SEPARATOR,					0,							0},
	{CMainMenu::SUBMENU_VOLUME,			IDS_MENU_VOLUME,			CM_VOLUMEMENU},
	{CMainMenu::SUBMENU_AUDIO,			IDS_MENU_AUDIO,				CM_AUDIOMENU},
	{MENU_ID_SEPARATOR,					0,							0},
	{CM_RECORD,							CM_RECORD,					CM_RECORD},
	{CM_RECORDOPTION,					CM_RECORDOPTION,			CM_RECORDOPTION},
	{MENU_ID_SEPARATOR,					0,							0},
	{CM_COPY,							CM_COPY,					CM_COPY},
	{CM_SAVEIMAGE,						CM_SAVEIMAGE,				CM_SAVEIMAGE},
	{CM_CAPTUREPREVIEW,					CM_CAPTUREPREVIEW,			CM_CAPTUREPREVIEW},
	{MENU_ID_SEPARATOR,					0,							0},
	{CM_DISABLEVIEWER,					CM_DISABLEVIEWER,			CM_DISABLEVIEWER},
	{CMainMenu::SUBMENU_RESET,			IDS_MENU_RESET,				CM_RESETMENU},
	{MENU_ID_SEPARATOR,					0,							0},
	{CM_PANEL,							CM_PANEL,					CM_PANEL},
	{CM_PROGRAMGUIDE,					CM_PROGRAMGUIDE,			CM_PROGRAMGUIDE},
	{CMainMenu::SUBMENU_BAR,			IDS_MENU_BAR,				CM_BARMENU},
	{CMainMenu::SUBMENU_PLUGIN,			IDS_MENU_PLUGIN,			CM_PLUGINMENU},
	{CM_OPTIONS,						CM_OPTIONS,					CM_OPTIONS},
	{CMainMenu::SUBMENU_FILTERPROPERTY,	IDS_MENU_FILTERPROPERTY,	CM_FILTERPROPERTYMENU},
	{CM_STREAMINFO,						CM_STREAMINFO,				CM_STREAMINFO},
	{MENU_ID_SEPARATOR,					0,							0},
	{CM_CLOSE,							CM_CLOSE,					CM_CLOSE},
};


CMenuOptions::CMenuOptions()
	: COptions(TEXT("Menu"))
	, m_MaxChannelMenuRows(24)
	, m_MaxChannelMenuEventInfo(30)
{
	m_MenuItemList.resize(lengthof(m_DefaultMenuItemList));
	for (int i=0;i<lengthof(m_DefaultMenuItemList);i++) {
		m_MenuItemList[i].ID=m_DefaultMenuItemList[i].ID;
		m_MenuItemList[i].fVisible=true;
	}
}


CMenuOptions::~CMenuOptions()
{
	Destroy();
}


bool CMenuOptions::ReadSettings(CSettings &Settings)
{
	int Value;

	if (Settings.Read(TEXT("MaxChannelMenuRows"),&Value) && Value>0)
		m_MaxChannelMenuRows=Value;
	Settings.Read(TEXT("MaxChannelMenuEventInfo"),&m_MaxChannelMenuEventInfo);

	int ItemCount;
	if (Settings.Read(TEXT("ItemCount"),&ItemCount) && ItemCount>0) {
		const CCommandList *pCommandList=GetAppClass().GetCommandList();
		std::vector<MenuItemInfo> ItemList;

		for (int i=0;i<ItemCount;i++) {
			TCHAR szName[32],szText[CCommandList::MAX_COMMAND_TEXT];

			::wsprintf(szName,TEXT("Item%d_ID"),i);
			if (Settings.Read(szName,szText,lengthof(szText))) {
				MenuItemInfo Item;

				Item.fVisible=true;
				::wsprintf(szName,TEXT("Item%d_State"),i);
				if (Settings.Read(szName,&Value))
					Item.fVisible=(Value&ITEM_STATE_VISIBLE)!=0;

				if (szText[0]==_T('\0')) {
					Item.ID=MENU_ID_SEPARATOR;
					ItemList.push_back(Item);
				} else {
					int Command=pCommandList->ParseText(szText);
					if (Command>0) {
						int ID=CommandToID(Command);
						if (ID>=0) {
							Item.ID=ID;
							ItemList.push_back(Item);
						}
					}
#ifdef _DEBUG
					else {
						TRACE(TEXT("CMenuOptions::ReadSettings() : Unknown command \"%s\"\n"),szText);
					}
#endif
				}
			}
		}

		for (int i=0;i<lengthof(m_DefaultMenuItemList);i++) {
			bool fFound=false;
			for (size_t j=0;j<ItemList.size();j++) {
				if (ItemList[j].ID==m_DefaultMenuItemList[i].ID) {
					fFound=true;
					break;
				}
			}
			if (!fFound) {
				MenuItemInfo Item;
				Item.ID=m_DefaultMenuItemList[i].ID;
				Item.fVisible=false;
				ItemList.push_back(Item);
			}
		}

		m_MenuItemList=ItemList;
	}

	return true;
}


bool CMenuOptions::WriteSettings(CSettings &Settings)
{
	Settings.Clear();

	Settings.Write(TEXT("MaxChannelMenuRows"),m_MaxChannelMenuRows);
	Settings.Write(TEXT("MaxChannelMenuEventInfo"),m_MaxChannelMenuEventInfo);

	bool fDefault=true;
	if (m_MenuItemList.size()!=lengthof(m_DefaultMenuItemList)) {
		fDefault=false;
	} else {
		for (int i=0;i<lengthof(m_DefaultMenuItemList);i++) {
			if (m_MenuItemList[i].ID!=m_DefaultMenuItemList[i].ID
					|| !m_MenuItemList[i].fVisible) {
				fDefault=false;
				break;
			}
		}
	}
	if (!fDefault) {
		Settings.Write(TEXT("ItemCount"),(int)m_MenuItemList.size());
		const CCommandList *pCommandList=GetAppClass().GetCommandList();
		for (size_t i=0;i<m_MenuItemList.size();i++) {
			TCHAR szName[32];

			::wsprintf(szName,TEXT("Item%d_ID"),i);
			if (m_MenuItemList[i].ID==MENU_ID_SEPARATOR) {
				Settings.Write(szName,TEXT(""));
			} else {
				Settings.Write(szName,pCommandList->GetCommandTextByID(IDToCommand(m_MenuItemList[i].ID)));
			}
			::wsprintf(szName,TEXT("Item%d_State"),i);
			Settings.Write(szName,m_MenuItemList[i].fVisible?ITEM_STATE_VISIBLE:0);
		}
	}

	return true;
}


bool CMenuOptions::GetMenuItemList(std::vector<int> *pItemList) const
{
	if (pItemList==NULL)
		return false;

	pItemList->clear();

	for (auto itr=m_MenuItemList.begin();itr!=m_MenuItemList.end();++itr) {
		if (itr->fVisible)
			pItemList->push_back(itr->ID);
	}

	return true;
}


int CMenuOptions::GetSubMenuPosByCommand(int Command) const
{
	for (int i=0;i<lengthof(m_DefaultMenuItemList);i++) {
		if (m_DefaultMenuItemList[i].Command==Command) {
			if (m_DefaultMenuItemList[i].ID<CM_COMMAND_FIRST)
				return m_DefaultMenuItemList[i].ID;
			break;
		}
	}
	return -1;
}


int CMenuOptions::IDToCommand(int ID) const
{
	for (int i=0;i<lengthof(m_DefaultMenuItemList);i++) {
		if (m_DefaultMenuItemList[i].ID==ID)
			return m_DefaultMenuItemList[i].Command;
	}
	return 0;
}


int CMenuOptions::CommandToID(int Command) const
{
	for (int i=0;i<lengthof(m_DefaultMenuItemList);i++) {
		if (m_DefaultMenuItemList[i].Command==Command)
			return m_DefaultMenuItemList[i].ID;
	}
	return -1;
}


bool CMenuOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_MENU));
}


static LPARAM GetListViewItemParam(HWND hwndList,int Item)
{
	LVITEM lvi;

	lvi.mask=LVIF_PARAM;
	lvi.iItem=Item;
	lvi.iSubItem=0;
	if (!ListView_GetItem(hwndList,&lvi))
		return 0;
	return lvi.lParam;
}

INT_PTR CMenuOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		::SetDlgItemInt(hDlg,IDC_MENUOPTIONS_MAXCHANNELMENUROWS,m_MaxChannelMenuRows,TRUE);
		DlgUpDown_SetRange(hDlg,IDC_MENUOPTIONS_MAXCHANNELMENUROWS_SPIN,1,100);

		::SetDlgItemInt(hDlg,IDC_MENUOPTIONS_MAXCHANNELMENUEVENTINFO,m_MaxChannelMenuEventInfo,TRUE);
		DlgUpDown_SetRange(hDlg,IDC_MENUOPTIONS_MAXCHANNELMENUEVENTINFO_SPIN,0,100);

		{
			HWND hwndList=::GetDlgItem(hDlg,IDC_MENUOPTIONS_ITEMLIST);
			ListView_SetExtendedListViewStyle(hwndList,
				LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_LABELTIP);

			RECT rc;
			::GetClientRect(hwndList,&rc);
			LVCOLUMN lvc;
			lvc.mask=LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
			lvc.fmt=LVCFMT_LEFT;
			lvc.cx=rc.right-::GetSystemMetrics(SM_CXVSCROLL);
			lvc.pszText=TEXT("");
			lvc.iSubItem=0;
			ListView_InsertColumn(hwndList,0,&lvc);

			m_fChanging=true;

			LVITEM lvi;
			TCHAR szText[CCommandList::MAX_COMMAND_NAME];
			lvi.mask=LVIF_TEXT | LVIF_PARAM;
			lvi.iSubItem=0;
			lvi.pszText=szText;

			for (int i=0;i<(int)m_MenuItemList.size();i++) {
				int ID=m_MenuItemList[i].ID;
				GetItemText(ID,szText,lengthof(szText));
				lvi.iItem=i;
				lvi.lParam=ID;
				lvi.iItem=ListView_InsertItem(hwndList,&lvi);
				ListView_SetCheckState(hwndList,lvi.iItem,m_MenuItemList[i].fVisible);
			}

			m_fChanging=false;
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_MENUOPTIONS_ITEMLIST_UP:
		case IDC_MENUOPTIONS_ITEMLIST_DOWN:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_MENUOPTIONS_ITEMLIST);
				int From=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED),To;

				if (From>=0) {
					if (LOWORD(wParam)==IDC_MENUOPTIONS_ITEMLIST_UP) {
						if (From<1)
							break;
						To=From-1;
					} else {
						if (From+1>=ListView_GetItemCount(hwndList))
							break;
						To=From+1;
					}
					m_fChanging=true;
					LVITEM lvi;
					TCHAR szText[CCommandList::MAX_COMMAND_NAME];
					lvi.mask=LVIF_STATE | LVIF_TEXT | LVIF_PARAM;
					lvi.iItem=From;
					lvi.iSubItem=0;
					lvi.stateMask=~0U;
					lvi.pszText=szText;
					lvi.cchTextMax=lengthof(szText);
					ListView_GetItem(hwndList,&lvi);
					BOOL fChecked=ListView_GetCheckState(hwndList,From);
					ListView_DeleteItem(hwndList,From);
					lvi.iItem=To;
					ListView_InsertItem(hwndList,&lvi);
					ListView_SetCheckState(hwndList,To,fChecked);
					SetDlgItemState(hDlg);
					m_fChanging=false;
				}
			}
			return TRUE;

		case IDC_MENUOPTIONS_ITEMLIST_INSERTSEPARATOR:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_MENUOPTIONS_ITEMLIST);
				int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

				if (Sel>=0) {
					LVITEM lvi;
					TCHAR szText[CCommandList::MAX_COMMAND_NAME];

					m_fChanging=true;
					GetItemText(MENU_ID_SEPARATOR,szText,lengthof(szText));
					lvi.mask=LVIF_TEXT | LVIF_PARAM;
					lvi.iItem=Sel;
					lvi.iSubItem=0;
					lvi.pszText=szText;
					lvi.lParam=MENU_ID_SEPARATOR;
					ListView_InsertItem(hwndList,&lvi);
					ListView_SetItemState(hwndList,Sel,LVIS_SELECTED | LVIS_FOCUSED,LVIS_SELECTED | LVIS_FOCUSED);
					ListView_SetCheckState(hwndList,Sel,TRUE);
					SetDlgItemState(hDlg);
					m_fChanging=false;
				}
			}
			return TRUE;

		case IDC_MENUOPTIONS_ITEMLIST_REMOVESEPARATOR:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_MENUOPTIONS_ITEMLIST);
				int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

				if (Sel>=0) {
					if (GetListViewItemParam(hwndList,Sel)==MENU_ID_SEPARATOR) {
						m_fChanging=true;
						ListView_DeleteItem(hwndList,Sel);
						SetDlgItemState(hDlg);
						m_fChanging=false;
					}
				}
			}
			return TRUE;

		case IDC_MENUOPTIONS_ITEMLIST_DEFAULT:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_MENUOPTIONS_ITEMLIST);

				m_fChanging=true;

				ListView_DeleteAllItems(hwndList);

				LVITEM lvi;
				TCHAR szText[CCommandList::MAX_COMMAND_NAME];
				lvi.mask=LVIF_TEXT | LVIF_PARAM;
				lvi.iSubItem=0;
				lvi.pszText=szText;

				for (int i=0;i<lengthof(m_DefaultMenuItemList);i++) {
					int ID=m_DefaultMenuItemList[i].ID;
					GetItemText(ID,szText,lengthof(szText));
					lvi.iItem=i;
					lvi.lParam=ID;
					lvi.iItem=ListView_InsertItem(hwndList,&lvi);
					ListView_SetCheckState(hwndList,lvi.iItem,TRUE);
				}

				SetDlgItemState(hDlg);

				m_fChanging=false;
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case LVN_ITEMCHANGED:
			if (!m_fChanging)
				SetDlgItemState(hDlg);
			return TRUE;

		case PSN_APPLY:
			{
				int Value;

				Value=::GetDlgItemInt(hDlg,IDC_MENUOPTIONS_MAXCHANNELMENUROWS,NULL,TRUE);
				if (Value>0)
					m_MaxChannelMenuRows=Value;

				Value=::GetDlgItemInt(hDlg,IDC_MENUOPTIONS_MAXCHANNELMENUEVENTINFO,NULL,TRUE);
				m_MaxChannelMenuEventInfo=Value;
			}

			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_MENUOPTIONS_ITEMLIST);
				const int ItemCount=ListView_GetItemCount(hwndList);

				m_MenuItemList.resize(ItemCount);

				LVITEM lvi;
				lvi.mask=LVIF_STATE | LVIF_PARAM;
				lvi.iSubItem=0;
				lvi.stateMask=~0U;

				for (int i=0;i<ItemCount;i++) {
					lvi.iItem=i;
					ListView_GetItem(hwndList,&lvi);
					m_MenuItemList[i].ID=(int)lvi.lParam;
					m_MenuItemList[i].fVisible=(lvi.state & LVIS_STATEIMAGEMASK)==INDEXTOSTATEIMAGEMASK(2);
				}
			}

			m_fChanged=true;
			return TRUE;
		}
		break;
	}

	return FALSE;
}


void CMenuOptions::SetDlgItemState(HWND hDlg)
{
	HWND hwndList=::GetDlgItem(hDlg,IDC_MENUOPTIONS_ITEMLIST);
	int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

	EnableDlgItem(hDlg,IDC_MENUOPTIONS_ITEMLIST_UP,Sel>0);
	EnableDlgItem(hDlg,IDC_MENUOPTIONS_ITEMLIST_DOWN,Sel>=0 && Sel+1<ListView_GetItemCount(hwndList));
	EnableDlgItem(hDlg,IDC_MENUOPTIONS_ITEMLIST_INSERTSEPARATOR,Sel>=0);
	EnableDlgItem(hDlg,IDC_MENUOPTIONS_ITEMLIST_REMOVESEPARATOR,
				  Sel>=0 && GetListViewItemParam(hwndList,Sel)==MENU_ID_SEPARATOR);
}


void CMenuOptions::GetItemText(int ID,LPTSTR pszText,int MaxLength) const
{
	if (ID==MENU_ID_SEPARATOR) {
		::lstrcpyn(pszText,TEXT("Å@<ãÊêÿÇË>"),MaxLength);
	} else {
		for (int i=0;i<lengthof(m_DefaultMenuItemList);i++) {
			if (m_DefaultMenuItemList[i].ID==ID) {
				::LoadString(GetAppClass().GetResourceInstance(),
							 m_DefaultMenuItemList[i].TextID,
							 pszText,MaxLength);
				break;
			}
		}
	}
}
