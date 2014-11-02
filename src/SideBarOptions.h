#ifndef SIDE_BAR_OPTIONS_H
#define SIDE_BAR_OPTIONS_H


#include <vector>
#include <map>
#include "Options.h"
#include "SideBar.h"
#include "ZoomOptions.h"


class CSideBarOptions : public COptions
{
public:
	enum PlaceType {
		PLACE_LEFT,
		PLACE_RIGHT,
		PLACE_TOP,
		PLACE_BOTTOM,
		PLACE_FIRST=PLACE_LEFT,
		PLACE_LAST=PLACE_BOTTOM
	};

	class ABSTRACT_CLASS(CEventHandler) {
	public:
		virtual void OnItemChanged() {}
	};

	CSideBarOptions(CSideBar *pSideBar,const CZoomOptions *pZoomOptions);
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
	PlaceType GetPlace() const { return m_Place; }
	bool SetPlace(PlaceType Place);
	bool GetShowChannelLogo() const { return m_fShowChannelLogo; }
	void SetEventHandler(CEventHandler *pHandler) { m_pEventHandler=pHandler; }

protected:
	enum { ITEM_SEPARATOR=0 };

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
	PlaceType m_Place;
	HIMAGELIST m_himlIcons;
	std::map<int,int> m_IconIDMap;
	CEventHandler *m_pEventHandler;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	HBITMAP CreateImage(IconSizeType SizeType,SIZE *pIconSize);
	void SetItemList(HWND hwndList,const int *pList,int NumItems);
	bool IsAvailableItem(int ID) const;
};


#endif
