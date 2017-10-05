#ifndef SIDE_BAR_OPTIONS_H
#define SIDE_BAR_OPTIONS_H


#include <vector>
#include <map>
#include "Options.h"
#include "SideBar.h"
#include "ZoomOptions.h"


class CSideBarOptions
	: public COptions
{
public:
	static const int OPACITY_MIN = 20;
	static const int OPACITY_MAX = 100;

	enum PlaceType {
		PLACE_LEFT,
		PLACE_RIGHT,
		PLACE_TOP,
		PLACE_BOTTOM,
		PLACE_FIRST = PLACE_LEFT,
		PLACE_LAST = PLACE_BOTTOM
	};

	CSideBarOptions(CSideBar *pSideBar, const CZoomOptions *pZoomOptions);
	~CSideBarOptions();

// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;

// CBasicDialog
	bool Create(HWND hwndOwner) override;

// CSideBarOptions
	bool ApplySideBarOptions();
	void ApplyItemList();
	bool SetSideBarImage();
	bool RegisterCommand(int ID);
	bool ShowPopup() const { return m_fShowPopup; }
	int GetPopupOpacity() const { return m_PopupOpacity; }
	PlaceType GetPlace() const { return m_Place; }
	bool SetPlace(PlaceType Place);
	bool GetShowChannelLogo() const { return m_fShowChannelLogo; }

protected:
	static constexpr int ITEM_SEPARATOR = 0;

	enum IconSizeType {
		ICON_SIZE_SMALL,
		ICON_SIZE_BIG
	};

	CSideBar *m_pSideBar;
	const CZoomOptions *m_pZoomOptions;
	std::vector<CSideBar::SideBarItem> m_AvailItemList;
	std::vector<int> m_ItemList;
	std::vector<TVTest::String> m_ItemNameList;
	bool m_fShowPopup;
	bool m_fShowToolTips;
	bool m_fShowChannelLogo;
	int m_PopupOpacity;
	PlaceType m_Place;
	HIMAGELIST m_himlIcons;
	std::map<int, int> m_IconIDMap;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	HBITMAP CreateImage(IconSizeType SizeType, SIZE *pIconSize);
	void SetItemList(HWND hwndList, const int *pList, int NumItems);
	bool IsAvailableItem(int ID) const;
};


#endif
