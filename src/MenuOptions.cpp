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
#include <algorithm>
#include "TVTest.h"
#include "AppMain.h"
#include "MenuOptions.h"
#include "Menu.h"
#include "Command.h"
#include "DialogUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace
{

constexpr int ITEM_STATE_VISIBLE = 0x0001;

}


const CMenuOptions::MenuInfo CMenuOptions::m_DefaultMenuItemList[] =
{
	{CMainMenu::SUBMENU_ZOOM,           IDS_MENU_ZOOM,           CM_ZOOMMENU},
	{CMainMenu::SUBMENU_ASPECTRATIO,    IDS_MENU_ASPECTRATIO,    CM_ASPECTRATIOMENU},
	{CM_FULLSCREEN,                     CM_FULLSCREEN,           CM_FULLSCREEN},
	{CM_ALWAYSONTOP,                    CM_ALWAYSONTOP,          CM_ALWAYSONTOP},
	{MENU_ID_SEPARATOR,                 0,                       0},
	{CMainMenu::SUBMENU_CHANNEL,        IDS_MENU_CHANNEL,        CM_CHANNELMENU},
	{CMainMenu::SUBMENU_SERVICE,        IDS_MENU_SERVICE,        CM_SERVICEMENU},
	{CMainMenu::SUBMENU_SPACE,          IDS_MENU_TUNER,          CM_TUNINGSPACEMENU},
	{CMainMenu::SUBMENU_FAVORITES,      IDS_MENU_FAVORITES,      CM_FAVORITESMENU},
	{CMainMenu::SUBMENU_CHANNELHISTORY, IDS_MENU_CHANNELHISTORY, CM_RECENTCHANNELMENU},
	{CM_1SEGMODE,                       CM_1SEGMODE,             CM_1SEGMODE},
	{MENU_ID_SEPARATOR,                 0,                       0},
	{CMainMenu::SUBMENU_VOLUME,         IDS_MENU_VOLUME,         CM_VOLUMEMENU},
	{CMainMenu::SUBMENU_AUDIO,          IDS_MENU_AUDIO,          CM_AUDIOMENU},
	{CMainMenu::SUBMENU_VIDEO,          IDS_MENU_VIDEO,          CM_VIDEOMENU},
	{MENU_ID_SEPARATOR,                 0,                       0},
	{CM_RECORD,                         CM_RECORD,               CM_RECORD},
	{CM_RECORDOPTION,                   CM_RECORDOPTION,         CM_RECORDOPTION},
	{MENU_ID_SEPARATOR,                 0,                       0},
	{CM_COPYIMAGE,                      CM_COPYIMAGE,            CM_COPYIMAGE},
	{CM_SAVEIMAGE,                      CM_SAVEIMAGE,            CM_SAVEIMAGE},
	{CM_CAPTUREPREVIEW,                 CM_CAPTUREPREVIEW,       CM_CAPTUREPREVIEW},
	{MENU_ID_SEPARATOR,                 0,                       0},
	{CM_DISABLEVIEWER,                  CM_DISABLEVIEWER,        CM_DISABLEVIEWER},
	{CMainMenu::SUBMENU_RESET,          IDS_MENU_RESET,          CM_RESETMENU},
	{MENU_ID_SEPARATOR,                 0,                       0},
	{CM_PANEL,                          CM_PANEL,                CM_PANEL},
	{CM_PROGRAMGUIDE,                   CM_PROGRAMGUIDE,         CM_PROGRAMGUIDE},
	{CMainMenu::SUBMENU_BAR,            IDS_MENU_BAR,            CM_BARMENU},
	{CMainMenu::SUBMENU_PLUGIN,         IDS_MENU_PLUGIN,         CM_PLUGINMENU},
	{CM_OPTIONS,                        CM_OPTIONS,              CM_OPTIONS},
	{CMainMenu::SUBMENU_FILTERPROPERTY, IDS_MENU_FILTERPROPERTY, CM_FILTERPROPERTYMENU},
	{CM_STREAMINFO,                     CM_STREAMINFO,           CM_STREAMINFO},
	{MENU_ID_SEPARATOR,                 0,                       0},
	{CM_CLOSE,                          CM_CLOSE,                CM_CLOSE},
};

const CMenuOptions::AdditionalItemInfo CMenuOptions::m_AdditionalItemList[] = {
	{CM_PLUGIN_FIRST,        CM_PLUGIN_LAST},
	{CM_PLUGINCOMMAND_FIRST, CM_PLUGINCOMMAND_LAST},
};


CMenuOptions::CMenuOptions()
	: COptions(TEXT("Menu"))
{
}


CMenuOptions::~CMenuOptions()
{
	Destroy();
}


bool CMenuOptions::ReadSettings(CSettings &Settings)
{
	int Value;

	if (Settings.Read(TEXT("MaxChannelMenuRows"), &Value) && Value > 0)
		m_MaxChannelMenuRows = Value;
	Settings.Read(TEXT("MaxChannelMenuEventInfo"), &m_MaxChannelMenuEventInfo);

	int ItemCount;
	if (Settings.Read(TEXT("ItemCount"), &ItemCount) && ItemCount > 0) {
		m_MenuItemList.clear();
		m_MenuItemList.reserve(ItemCount);

		String Text;

		for (int i = 0; i < ItemCount; i++) {
			TCHAR szName[32];

			StringFormat(szName, TEXT("Item{}_ID"), i);
			if (Settings.Read(szName, &Text)) {
				MenuItemInfo Item;

				Item.Name = Text;
				Item.ID = MENU_ID_INVALID;
				Item.fVisible = true;
				StringFormat(szName, TEXT("Item{}_State"), i);
				if (Settings.Read(szName, &Value))
					Item.fVisible = (Value & ITEM_STATE_VISIBLE) != 0;
				m_MenuItemList.push_back(Item);
			}
		}
	}

	return true;
}


bool CMenuOptions::WriteSettings(CSettings &Settings)
{
	Settings.Clear();

	Settings.Write(TEXT("MaxChannelMenuRows"), m_MaxChannelMenuRows);
	Settings.Write(TEXT("MaxChannelMenuEventInfo"), m_MaxChannelMenuEventInfo);

	if (!m_MenuItemList.empty()) {
		// デフォルトと同じである場合は保存しない
		// (新しく項目が追加された時にデフォルトとして反映されるようにするため)
		bool fDefault = true;
		if (m_MenuItemList.size() < lengthof(m_DefaultMenuItemList)) {
			fDefault = false;
		} else {
			for (size_t i = 0; i < lengthof(m_DefaultMenuItemList); i++) {
				if (m_MenuItemList[i].ID != m_DefaultMenuItemList[i].ID
						|| !m_MenuItemList[i].fVisible) {
					fDefault = false;
					break;
				}
			}
			if (fDefault) {
				for (size_t i = lengthof(m_DefaultMenuItemList); i < m_MenuItemList.size(); i++) {
					if (m_MenuItemList[i].fVisible) {
						fDefault = false;
						break;
					}
				}
			}
		}
		if (!fDefault) {
			Settings.Write(TEXT("ItemCount"), static_cast<int>(m_MenuItemList.size()));
			const CCommandManager &CommandManager = GetAppClass().CommandManager;
			for (size_t i = 0; i < m_MenuItemList.size(); i++) {
				const MenuItemInfo &Item = m_MenuItemList[i];
				TCHAR szName[32];

				StringFormat(szName, TEXT("Item{}_ID"), i);
				if (Item.ID == MENU_ID_INVALID) {
					Settings.Write(szName, Item.Name);
				} else if (Item.ID == MENU_ID_SEPARATOR) {
					Settings.Write(szName, TEXT(""));
				} else {
					Settings.Write(szName, CommandManager.GetCommandIDText(IDToCommand(Item.ID)));
				}
				StringFormat(szName, TEXT("Item{}_State"), i);
				Settings.Write(szName, Item.fVisible ? ITEM_STATE_VISIBLE : 0);
			}
		}
	}

	return true;
}


bool CMenuOptions::GetMenuItemList(std::vector<int> *pItemList)
{
	if (pItemList == nullptr)
		return false;

	if (m_MenuItemList.empty()) {
		pItemList->resize(lengthof(m_DefaultMenuItemList));
		for (int i = 0; i < lengthof(m_DefaultMenuItemList); i++)
			(*pItemList)[i] = m_DefaultMenuItemList[i].ID;
	} else {
		pItemList->clear();
		pItemList->reserve(m_MenuItemList.size());

		for (MenuItemInfo &Item : m_MenuItemList) {
			if (Item.fVisible) {
				int &ID = Item.ID;
				if (ID == MENU_ID_INVALID)
					ID = GetIDFromString(Item.Name);
				if (ID != MENU_ID_INVALID)
					pItemList->push_back(ID);
			}
		}
	}

	return true;
}


int CMenuOptions::GetSubMenuPosByCommand(int Command) const
{
	for (const MenuInfo &Item : m_DefaultMenuItemList) {
		if (Item.Command == Command) {
			if (Item.ID < CM_COMMAND_FIRST)
				return Item.ID;
			break;
		}
	}
	return -1;
}


int CMenuOptions::IDToCommand(int ID) const
{
	for (const MenuInfo &Item : m_DefaultMenuItemList) {
		if (Item.ID == ID)
			return Item.Command;
	}
	return ID;
}


int CMenuOptions::CommandToID(int Command) const
{
	for (const MenuInfo &Item : m_DefaultMenuItemList) {
		if (Item.Command == Command)
			return Item.ID;
	}
	return Command;
}


int CMenuOptions::GetIDFromString(const String &Str) const
{
	if (Str.empty())
		return MENU_ID_SEPARATOR;

	const int Command = GetAppClass().CommandManager.ParseIDText(Str);
	if (Command > 0)
		return CommandToID(Command);

	return MENU_ID_INVALID;
}


bool CMenuOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(
		hwndOwner,
		GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_OPTIONS_MENU));
}


INT_PTR CMenuOptions::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		::SetDlgItemInt(hDlg, IDC_MENUOPTIONS_MAXCHANNELMENUROWS, m_MaxChannelMenuRows, TRUE);
		DlgUpDown_SetRange(hDlg, IDC_MENUOPTIONS_MAXCHANNELMENUROWS_SPIN, 1, 100);

		::SetDlgItemInt(hDlg, IDC_MENUOPTIONS_MAXCHANNELMENUEVENTINFO, m_MaxChannelMenuEventInfo, TRUE);
		DlgUpDown_SetRange(hDlg, IDC_MENUOPTIONS_MAXCHANNELMENUEVENTINFO_SPIN, 0, 100);

		{
			m_ItemListView.Attach(::GetDlgItem(hDlg, IDC_MENUOPTIONS_ITEMLIST));
			m_ItemListView.InitCheckList();

			m_fChanging = true;

			if (m_MenuItemList.empty()) {
				m_MenuItemList.resize(lengthof(m_DefaultMenuItemList));
				for (int i = 0; i < lengthof(m_DefaultMenuItemList); i++) {
					m_MenuItemList[i].ID = m_DefaultMenuItemList[i].ID;
					m_MenuItemList[i].fVisible = true;
				}
			} else {
				for (MenuItemInfo &Item : m_MenuItemList) {
					if (Item.ID == MENU_ID_INVALID)
						Item.ID = GetIDFromString(Item.Name);
				}

				for (const MenuInfo &e : m_DefaultMenuItemList) {
					const int ID = e.ID;
					auto it = std::ranges::find(m_MenuItemList, ID, &MenuItemInfo::ID);
					if (it == m_MenuItemList.end()) {
						MenuItemInfo Item;
						Item.ID = ID;
						Item.fVisible = false;
						m_MenuItemList.push_back(Item);
					}
				}
			}

			const CCommandManager &CommandManager = GetAppClass().CommandManager;
			for (const AdditionalItemInfo &e : m_AdditionalItemList) {
				for (int ID = e.First; ID <= e.Last; ID++) {
					if (!CommandManager.IsCommandValid(ID))
						break;
					auto it = std::ranges::find(m_MenuItemList, ID, &MenuItemInfo::ID);
					if (it == m_MenuItemList.end()) {
						MenuItemInfo Item;
						Item.ID = ID;
						Item.fVisible = false;
						m_MenuItemList.push_back(Item);
					}
				}
			}

			int i = 0;
			for (MenuItemInfo &Item : m_MenuItemList) {
				TCHAR szText[CCommandManager::MAX_COMMAND_TEXT];

				if (Item.ID == MENU_ID_INVALID)
					Item.ID = GetIDFromString(Item.Name);
				if (Item.ID != MENU_ID_INVALID) {
					GetItemText(Item.ID, szText, lengthof(szText));
					m_ItemListView.InsertItem(i, szText, Item.ID);
					m_ItemListView.CheckItem(i, Item.fVisible);
					i++;
				}
			}

			AddControl(IDC_MENUOPTIONS_ITEMLIST, AlignFlag::All);
			AddControls(
				IDC_MENUOPTIONS_ITEMLIST_UP,
				IDC_MENUOPTIONS_ITEMLIST_REMOVESEPARATOR,
				AlignFlag::Right);
			AddControl(IDC_MENUOPTIONS_ITEMLIST_DEFAULT, AlignFlag::BottomRight);

			m_fChanging = false;
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_MENUOPTIONS_ITEMLIST_UP:
		case IDC_MENUOPTIONS_ITEMLIST_DOWN:
			{
				const int From = m_ItemListView.GetSelectedItem();
				int To;

				if (From >= 0) {
					if (LOWORD(wParam) == IDC_MENUOPTIONS_ITEMLIST_UP) {
						if (From < 1)
							break;
						To = From - 1;
					} else {
						if (From + 1 >= m_ItemListView.GetItemCount())
							break;
						To = From + 1;
					}
					m_fChanging = true;
					m_ItemListView.MoveItem(From, To);
					m_ItemListView.EnsureItemVisible(To);
					SetDlgItemState(hDlg);
					m_fChanging = false;
				}
			}
			return TRUE;

		case IDC_MENUOPTIONS_ITEMLIST_INSERTSEPARATOR:
			{
				const int Sel = m_ItemListView.GetSelectedItem();

				if (Sel >= 0) {
					TCHAR szText[CCommandManager::MAX_COMMAND_TEXT];

					m_fChanging = true;
					GetItemText(MENU_ID_SEPARATOR, szText, lengthof(szText));
					m_ItemListView.InsertItem(Sel, szText, MENU_ID_SEPARATOR);
					m_ItemListView.SetItemState(Sel, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
					m_ItemListView.CheckItem(Sel, true);
					m_ItemListView.EnsureItemVisible(Sel);
					SetDlgItemState(hDlg);
					m_fChanging = false;
				}
			}
			return TRUE;

		case IDC_MENUOPTIONS_ITEMLIST_REMOVESEPARATOR:
			{
				const int Sel = m_ItemListView.GetSelectedItem();

				if (Sel >= 0) {
					if (m_ItemListView.GetItemParam(Sel) == MENU_ID_SEPARATOR) {
						m_fChanging = true;
						m_ItemListView.DeleteItem(Sel);
						SetDlgItemState(hDlg);
						m_fChanging = false;
					}
				}
			}
			return TRUE;

		case IDC_MENUOPTIONS_ITEMLIST_DEFAULT:
			{
				m_fChanging = true;

				m_ItemListView.DeleteAllItems();

				for (int i = 0; i < lengthof(m_DefaultMenuItemList); i++) {
					const int ID = m_DefaultMenuItemList[i].ID;
					TCHAR szText[CCommandManager::MAX_COMMAND_TEXT];

					GetItemText(ID, szText, lengthof(szText));
					m_ItemListView.InsertItem(i, szText, ID);
					m_ItemListView.CheckItem(i, true);
				}

				const CCommandManager &CommandManager = GetAppClass().CommandManager;
				for (const AdditionalItemInfo &Item : m_AdditionalItemList) {
					for (int ID = Item.First; ID <= Item.Last; ID++) {
						if (!CommandManager.IsCommandValid(ID))
							break;

						TCHAR szText[CCommandManager::MAX_COMMAND_TEXT];

						GetItemText(ID, szText, lengthof(szText));
						const int Index = m_ItemListView.InsertItem(-1, szText, ID);
						m_ItemListView.CheckItem(Index, false);
					}
				}

				SetDlgItemState(hDlg);

				m_fChanging = false;
			}
			return TRUE;
		}
		return TRUE;

	case WM_SIZE:
		m_ItemListView.AdjustSingleColumnWidth();
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case LVN_ITEMCHANGED:
			if (!m_fChanging)
				SetDlgItemState(hDlg);
			return TRUE;

		case PSN_APPLY:
			{
				int Value;

				Value = ::GetDlgItemInt(hDlg, IDC_MENUOPTIONS_MAXCHANNELMENUROWS, nullptr, TRUE);
				if (Value > 0)
					m_MaxChannelMenuRows = Value;

				Value = ::GetDlgItemInt(hDlg, IDC_MENUOPTIONS_MAXCHANNELMENUEVENTINFO, nullptr, TRUE);
				m_MaxChannelMenuEventInfo = Value;
			}

			{
				const int ItemCount = m_ItemListView.GetItemCount();

				m_MenuItemList.clear();
				m_MenuItemList.resize(ItemCount);

				for (int i = 0; i < ItemCount; i++) {
					m_MenuItemList[i].ID = static_cast<int>(m_ItemListView.GetItemParam(i));
					m_MenuItemList[i].fVisible = m_ItemListView.IsItemChecked(i);
				}
			}

			m_fChanged = true;
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
	const int Sel = m_ItemListView.GetSelectedItem();

	EnableDlgItem(hDlg, IDC_MENUOPTIONS_ITEMLIST_UP, Sel > 0);
	EnableDlgItem(hDlg, IDC_MENUOPTIONS_ITEMLIST_DOWN, Sel >= 0 && Sel + 1 < m_ItemListView.GetItemCount());
	EnableDlgItem(hDlg, IDC_MENUOPTIONS_ITEMLIST_INSERTSEPARATOR, Sel >= 0);
	EnableDlgItem(
		hDlg, IDC_MENUOPTIONS_ITEMLIST_REMOVESEPARATOR,
		Sel >= 0 && m_ItemListView.GetItemParam(Sel) == MENU_ID_SEPARATOR);
}


void CMenuOptions::GetItemText(int ID, LPTSTR pszText, int MaxLength) const
{
	if (ID == MENU_ID_SEPARATOR) {
		StringCopy(pszText, TEXT("　<区切り>"), MaxLength);
	} else {
		for (const MenuInfo &Item : m_DefaultMenuItemList) {
			if (Item.ID == ID) {
				::LoadString(
					GetAppClass().GetResourceInstance(),
					Item.TextID,
					pszText, MaxLength);
				return;
			}
		}

		GetAppClass().CommandManager.GetCommandText(ID, pszText, MaxLength);
	}
}


} // namespace TVTest
