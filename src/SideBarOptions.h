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


#ifndef TVTEST_SIDE_BAR_OPTIONS_H
#define TVTEST_SIDE_BAR_OPTIONS_H


#include <vector>
#include <map>
#include "Options.h"
#include "SideBar.h"
#include "ZoomOptions.h"


namespace TVTest
{

	class CSideBarOptions
		: public COptions
	{
	public:
		static constexpr int OPACITY_MIN = 20;
		static constexpr int OPACITY_MAX = 100;

		enum class PlaceType {
			Left,
			Right,
			Top,
			Bottom,
			TVTEST_ENUM_CLASS_TRAILER
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

		enum class IconSizeType {
			Small,
			Big,
		};

		CSideBar *m_pSideBar;
		const CZoomOptions *m_pZoomOptions;
		std::vector<CSideBar::SideBarItem> m_AvailItemList;
		std::vector<int> m_ItemList;
		std::vector<String> m_ItemNameList;
		bool m_fShowPopup = true;
		bool m_fShowToolTips = true;
		bool m_fShowChannelLogo = true;
		int m_PopupOpacity = OPACITY_MAX;
		PlaceType m_Place = PlaceType::Left;
		HIMAGELIST m_himlIcons = nullptr;
		std::map<int, int> m_IconIDMap;

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		void OnDarkModeChanged(bool fDarkMode) override;

		HBITMAP CreateImage(IconSizeType SizeType, SIZE *pIconSize);
		HIMAGELIST CreateIconImageList();
		void UpdateListViewIcons(HWND hwndList, HIMAGELIST himl);
		void SetItemList(HWND hwndList, const int *pList, int NumItems);
		bool IsAvailableItem(int ID) const;
	};

} // namespace TVTest


#endif
