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


#ifndef TVTEST_MENU_OPTIONS_H
#define TVTEST_MENU_OPTIONS_H


#include <vector>
#include "Options.h"
#include "ListView.h"


namespace TVTest
{

	class CMenuOptions
		: public COptions
	{
	public:
		CMenuOptions();
		~CMenuOptions();

	// CSettingsBase
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	// CBasicDialog
		bool Create(HWND hwndOwner) override;

	// CMenuOptions
		int GetMaxChannelMenuRows() const { return m_MaxChannelMenuRows; }
		int GetMaxChannelMenuEventInfo() const { return m_MaxChannelMenuEventInfo; }
		bool GetMenuItemList(std::vector<int> *pItemList);
		int GetSubMenuPosByCommand(int Command) const;

	private:
	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		int IDToCommand(int ID) const;
		int CommandToID(int Command) const;
		int GetIDFromString(const String &Str) const;
		void SetDlgItemState(HWND hDlg);
		void GetItemText(int ID, LPTSTR pszText, int MaxLength) const;

		struct MenuInfo
		{
			int ID;
			int TextID;
			int Command;
		};

		struct MenuItemInfo
		{
			String Name;
			int ID;
			bool fVisible;
		};

		struct AdditionalItemInfo
		{
			int First;
			int Last;
		};

		static constexpr int MENU_ID_SEPARATOR = -1;
		static constexpr int MENU_ID_INVALID   = -2;

		static const MenuInfo m_DefaultMenuItemList[];
		static const AdditionalItemInfo m_AdditionalItemList[];

		int m_MaxChannelMenuRows = 24;
		int m_MaxChannelMenuEventInfo = 30;
		std::vector<MenuItemInfo> m_MenuItemList;

		bool m_fChanging = false;
		CListView m_ItemListView;
	};

} // namespace TVTest


#endif
