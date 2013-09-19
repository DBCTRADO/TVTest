#ifndef SIDE_BAR_OPTIONS_H
#define SIDE_BAR_OPTIONS_H


#include <vector>
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
	bool SetSideBarImage();
	bool ShowPopup() const { return m_fShowPopup; }
	PlaceType GetPlace() const { return m_Place; }
	bool SetPlace(PlaceType Place);
	bool GetShowChannelLogo() const { return m_fShowChannelLogo; }
	void SetEventHandler(CEventHandler *pHandler) { m_pEventHandler=pHandler; }

protected:
	enum { ITEM_SEPARATOR=0 };
	CSideBar *m_pSideBar;
	const CZoomOptions *m_pZoomOptions;
	std::vector<int> m_ItemList;
	bool m_fShowPopup;
	bool m_fShowToolTips;
	bool m_fShowChannelLogo;
	PlaceType m_Place;
	HIMAGELIST m_himlIcons;
	CEventHandler *m_pEventHandler;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	HBITMAP CreateImage();
	void ApplyItemList() const;
	void SetItemList(HWND hwndList,const int *pList,int NumItems);
};


#endif
