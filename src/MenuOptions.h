#ifndef MENU_OPTIONS_H
#define MENU_OPTIONS_H


#include <vector>
#include "Options.h"
#include "ListView.h"


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
	int GetIDFromString(const TVTest::String &Str) const;
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
		TVTest::String Name;
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

	int m_MaxChannelMenuRows;
	int m_MaxChannelMenuEventInfo;
	std::vector<MenuItemInfo> m_MenuItemList;

	bool m_fChanging;
	TVTest::CListView m_ItemListView;
};


#endif
