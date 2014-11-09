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
	{CM_1SEGMODE,						CM_1SEGMODE,				CM_1SEGMODE},
	{MENU_ID_SEPARATOR,					0,							0},
	{CMainMenu::SUBMENU_VOLUME,			IDS_MENU_VOLUME,			CM_VOLUMEMENU},
	{CMainMenu::SUBMENU_AUDIO,			IDS_MENU_AUDIO,				CM_AUDIOMENU},
	{CMainMenu::SUBMENU_VIDEO,			IDS_MENU_VIDEO,				CM_VIDEOMENU},
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
		const CCommandList &CommandList=GetAppClass().CommandList;
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
					int Command=CommandList.ParseText(szText);
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
		const CCommandList &CommandList=GetAppClass().CommandList;
		for (size_t i=0;i<m_MenuItemList.size();i++) {
			TCHAR szName[32];

			::wsprintf(szName,TEXT("Item%d_ID"),i);
			if (m_MenuItemList[i].ID==MENU_ID_SEPARATOR) {
				Settings.Write(szName,TEXT(""));
			} else {
				Settings.Write(szName,CommandList.GetCommandTextByID(IDToCommand(m_MenuItemList[i].ID)));
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


INT_PTR CMenuOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		::SetDlgItemInt(hDlg,IDC_MENUOPTIONS_MAXCHANNELMENUROWS,m_MaxChannelMenuRows,TRUE);
		DlgUpDown_SetRange(hDlg,IDC_MENUOPTIONS_MAXCHANNELMENUROWS_SPIN,1,100);

		::SetDlgItemInt(hDlg,IDC_MENUOPTIONS_MAXCHANNELMENUEVENTINFO,m_MaxChannelMenuEventInfo,TRUE);
		DlgUpDown_SetRange(hDlg,IDC_MENUOPTIONS_MAXCHANNELMENUEVENTINFO_SPIN,0,100);

		{
			m_ItemListView.Attach(::GetDlgItem(hDlg,IDC_MENUOPTIONS_ITEMLIST));
			m_ItemListView.InitCheckList();

			m_fChanging=true;

			for (int i=0;i<(int)m_MenuItemList.size();i++) {
				int ID=m_MenuItemList[i].ID;
				TCHAR szText[CCommandList::MAX_COMMAND_NAME];

				GetItemText(ID,szText,lengthof(szText));
				m_ItemListView.InsertItem(i,szText,ID);
				m_ItemListView.CheckItem(i,m_MenuItemList[i].fVisible);
			}

			m_fChanging=false;
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_MENUOPTIONS_ITEMLIST_UP:
		case IDC_MENUOPTIONS_ITEMLIST_DOWN:
			{
				int From=m_ItemListView.GetSelectedItem(),To;

				if (From>=0) {
					if (LOWORD(wParam)==IDC_MENUOPTIONS_ITEMLIST_UP) {
						if (From<1)
							break;
						To=From-1;
					} else {
						if (From+1>=m_ItemListView.GetItemCount())
							break;
						To=From+1;
					}
					m_fChanging=true;
					m_ItemListView.MoveItem(From,To);
					m_ItemListView.EnsureItemVisible(To);
					SetDlgItemState(hDlg);
					m_fChanging=false;
				}
			}
			return TRUE;

		case IDC_MENUOPTIONS_ITEMLIST_INSERTSEPARATOR:
			{
				int Sel=m_ItemListView.GetSelectedItem();

				if (Sel>=0) {
					TCHAR szText[CCommandList::MAX_COMMAND_NAME];

					m_fChanging=true;
					GetItemText(MENU_ID_SEPARATOR,szText,lengthof(szText));
					m_ItemListView.InsertItem(Sel,szText,MENU_ID_SEPARATOR);
					m_ItemListView.SetItemState(Sel,LVIS_SELECTED | LVIS_FOCUSED,LVIS_SELECTED | LVIS_FOCUSED);
					m_ItemListView.CheckItem(Sel,true);
					m_ItemListView.EnsureItemVisible(Sel);
					SetDlgItemState(hDlg);
					m_fChanging=false;
				}
			}
			return TRUE;

		case IDC_MENUOPTIONS_ITEMLIST_REMOVESEPARATOR:
			{
				int Sel=m_ItemListView.GetSelectedItem();

				if (Sel>=0) {
					if (m_ItemListView.GetItemParam(Sel)==MENU_ID_SEPARATOR) {
						m_fChanging=true;
						m_ItemListView.DeleteItem(Sel);
						SetDlgItemState(hDlg);
						m_fChanging=false;
					}
				}
			}
			return TRUE;

		case IDC_MENUOPTIONS_ITEMLIST_DEFAULT:
			{
				m_fChanging=true;

				m_ItemListView.DeleteAllItems();

				for (int i=0;i<lengthof(m_DefaultMenuItemList);i++) {
					int ID=m_DefaultMenuItemList[i].ID;
					TCHAR szText[CCommandList::MAX_COMMAND_NAME];

					GetItemText(ID,szText,lengthof(szText));
					m_ItemListView.InsertItem(i,szText,ID);
					m_ItemListView.CheckItem(i,true);
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
				const int ItemCount=m_ItemListView.GetItemCount();

				m_MenuItemList.resize(ItemCount);

				for (int i=0;i<ItemCount;i++) {
					m_MenuItemList[i].ID=(int)m_ItemListView.GetItemParam(i);
					m_MenuItemList[i].fVisible=m_ItemListView.IsItemChecked(i);
				}
			}

			m_fChanged=true;
			return TRUE;
		}
		break;

	case WM_DESTROY:
		m_ItemListView.Detach();
		return TRUE;
	}

	return FALSE;
}


void CMenuOptions::SetDlgItemState(HWND hDlg)
{
	int Sel=m_ItemListView.GetSelectedItem();

	EnableDlgItem(hDlg,IDC_MENUOPTIONS_ITEMLIST_UP,Sel>0);
	EnableDlgItem(hDlg,IDC_MENUOPTIONS_ITEMLIST_DOWN,Sel>=0 && Sel+1<m_ItemListView.GetItemCount());
	EnableDlgItem(hDlg,IDC_MENUOPTIONS_ITEMLIST_INSERTSEPARATOR,Sel>=0);
	EnableDlgItem(hDlg,IDC_MENUOPTIONS_ITEMLIST_REMOVESEPARATOR,
				  Sel>=0 && m_ItemListView.GetItemParam(Sel)==MENU_ID_SEPARATOR);
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
