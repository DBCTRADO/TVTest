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


#ifndef TVTEST_PROGRAM_GUIDE_FAVORITES_H
#define TVTEST_PROGRAM_GUIDE_FAVORITES_H


#include <vector>
#include "Dialog.h"


namespace TVTest
{

	class CProgramGuideFavorites
	{
	public:
		struct FavoriteInfo
		{
			String Name;
			String GroupID;
			String Label;
			COLORREF BackColor;
			COLORREF TextColor;

			void SetDefaultColors();
		};

		void Clear();
		size_t GetCount() const;
		bool Add(const FavoriteInfo &Info);
		bool Get(size_t Index, FavoriteInfo *pInfo) const;
		FavoriteInfo *Get(size_t Index);
		const FavoriteInfo *Get(size_t Index) const;
		bool Set(size_t Index, const FavoriteInfo &Info);
		bool GetFixedWidth() const { return m_fFixedWidth; }
		void SetFixedWidth(bool fFixed) { m_fFixedWidth = fFixed; }

	private:
		std::vector<FavoriteInfo> m_List;
		bool m_fFixedWidth = true;
	};

	class CProgramGuideFavoritesDialog
		: public CBasicDialog
	{
	public:
		CProgramGuideFavoritesDialog(
			const CProgramGuideFavorites &Favorites,
			const Theme::BackgroundStyle &ButtonTheme);
		~CProgramGuideFavoritesDialog();

	// CBasicDialog
		bool Show(HWND hwndOwner) override;

	// CProgramGuideFavoritesDialog
		void AddNewItem(const CProgramGuideFavorites::FavoriteInfo &Info);
		const CProgramGuideFavorites &GetFavorites() const { return m_Favorites; }

	private:
		CProgramGuideFavorites m_Favorites;
		int m_CurItem = -1;
		bool m_fChanging = false;
		Theme::BackgroundStyle m_ButtonTheme;

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		void SetItemState(HWND hDlg);
		CProgramGuideFavorites::FavoriteInfo *GetCurItemInfo();
	};

}	// namespace TVTest


#endif
