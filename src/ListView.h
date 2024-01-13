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


#ifndef TVTEST_LIST_VIEW_H
#define TVTEST_LIST_VIEW_H


namespace TVTest
{

	class CListView
	{
	public:
		CListView() = default;

		CListView(const CListView &) = delete;
		CListView &operator=(const CListView &) = delete;

		bool Attach(HWND hwnd);
		void Detach();
		void SetExtendedStyle(DWORD Style);
		bool InitCheckList();
		int InsertItem(int Index, LPCTSTR pszText, LPARAM Param = 0);
		bool DeleteItem(int Index);
		void DeleteAllItems();
		bool SetItemText(int Index, LPCTSTR pszText);
		bool SetItemText(int Index, int SubItem, LPCTSTR pszText);
		bool GetItemText(int Index, LPTSTR pszText, int MaxText) const;
		bool GetItemText(int Index, int SubItem, LPTSTR pszText, int MaxText) const;
		bool SetItemState(int Index, UINT State, UINT Mask);
		bool CheckItem(int Index, bool fCheck);
		bool IsItemChecked(int Index) const;
		bool SetItemParam(int Index, LPARAM Param);
		LPARAM GetItemParam(int Index) const;
		int GetItemCount() const;
		int GetSelectedItem() const;
		bool MoveItem(int From, int To);
		bool EnsureItemVisible(int Index, bool fPartialOK = false);
		int InsertColumn(int Index, LPCTSTR pszText, int Format = LVCFMT_LEFT);
		void AdjustColumnWidth(bool fUseHeader = true);
		void AdjustSingleColumnWidth();

	protected:
		HWND m_hwnd = nullptr;
	};

} // namespace TVTest


#endif
