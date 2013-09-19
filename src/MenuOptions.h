#ifndef MENU_OPTIONS_H
#define MENU_OPTIONS_H


#include <vector>
#include "Options.h"


class CMenuOptions : public COptions
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
	bool GetMenuItemList(std::vector<int> *pItemList) const;
	int GetSubMenuPosByCommand(int Command) const;

private:
// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	int IDToCommand(int ID) const;
	int CommandToID(int Command) const;
	void SetDlgItemState(HWND hDlg);
	void GetItemText(int ID,LPTSTR pszText,int MaxLength) const;

	struct MenuInfo {
		int ID;
		int TextID;
		int Command;
	};

	struct MenuItemInfo {
		int ID;
		bool fVisible;
	};

	enum {
		MENU_ID_SEPARATOR = -1
	};

	static const MenuInfo m_DefaultMenuItemList[];

	int m_MaxChannelMenuRows;
	int m_MaxChannelMenuEventInfo;
	std::vector<MenuItemInfo> m_MenuItemList;

	bool m_fChanging;
};


#endif
