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
#include "TVTest.h"
#include "ListView.h"
#include "DialogUtil.h"
#include "DPIUtil.h"
#include "Common/DebugDef.h"


namespace TVTest
{


bool CListView::Attach(HWND hwnd)
{
	if (hwnd == nullptr)
		return false;

	m_hwnd = hwnd;

	return true;
}


void CListView::Detach()
{
	m_hwnd = nullptr;
}


void CListView::SetExtendedStyle(DWORD Style)
{
	if (m_hwnd != nullptr) {
		ListView_SetExtendedListViewStyle(m_hwnd, Style);

		if ((Style & (LVS_EX_INFOTIP | LVS_EX_LABELTIP)) != 0)
			SetListViewTooltipsTopMost(m_hwnd);
	}
}


bool CListView::InitCheckList()
{
	if (m_hwnd == nullptr)
		return false;

	ListView_SetExtendedListViewStyle(
		m_hwnd,
		LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER);

	RECT rc;
	::GetClientRect(m_hwnd, &rc);
	LVCOLUMN lvc;
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = rc.right - GetScrollBarWidth(m_hwnd);
	lvc.pszText = const_cast<LPTSTR>(TEXT(""));
	lvc.iSubItem = 0;
	ListView_InsertColumn(m_hwnd, 0, &lvc);

	SetListViewTooltipsTopMost(m_hwnd);

	return true;
}


int CListView::InsertItem(int Index, LPCTSTR pszText, LPARAM Param)
{
	if (m_hwnd == nullptr)
		return -1;

	if (Index < 0)
		Index = GetItemCount();

	LVITEM lvi;

	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iItem = Index;
	lvi.iSubItem = 0;
	lvi.pszText = const_cast<LPTSTR>(pszText);
	lvi.lParam = Param;

	return ListView_InsertItem(m_hwnd, &lvi);
}


bool CListView::DeleteItem(int Index)
{
	if (m_hwnd == nullptr)
		return false;
	return ListView_DeleteItem(m_hwnd, Index) != FALSE;
}


void CListView::DeleteAllItems()
{
	if (m_hwnd != nullptr)
		ListView_DeleteAllItems(m_hwnd);
}


bool CListView::SetItemText(int Index, LPCTSTR pszText)
{
	if (m_hwnd == nullptr)
		return false;

	ListView_SetItemText(m_hwnd, Index, 0, const_cast<LPTSTR>(pszText));

	return true;
}


bool CListView::SetItemText(int Index, int SubItem, LPCTSTR pszText)
{
	if (m_hwnd == nullptr)
		return false;

	LVITEM lvi;
	lvi.mask = LVIF_TEXT;
	lvi.iItem = Index;
	lvi.iSubItem = SubItem;
	lvi.pszText = const_cast<LPTSTR>(pszText);

	return ListView_SetItem(m_hwnd, &lvi) != FALSE;
}


bool CListView::GetItemText(int Index, LPTSTR pszText, int MaxText) const
{
	return GetItemText(Index, 0, pszText, MaxText);
}


bool CListView::GetItemText(int Index, int SubItem, LPTSTR pszText, int MaxText) const
{
	if (m_hwnd == nullptr)
		return false;
	if (pszText == nullptr || MaxText < 1)
		return false;

	LVITEM lvi;
	//lvi.mask = LVIF_TEXT;
	//lvi.iItem = Index;
	lvi.iSubItem = SubItem;
	lvi.pszText = pszText;
	lvi.cchTextMax = MaxText;

	if (::SendMessage(m_hwnd, LVM_GETITEMTEXT, Index, reinterpret_cast<LPARAM>(&lvi)) < 1) {
		pszText[0] = _T('\0');
		return false;
	}

	return true;
}


bool CListView::SetItemState(int Index, UINT State, UINT Mask)
{
	if (m_hwnd == nullptr)
		return false;

	ListView_SetItemState(m_hwnd, Index, State, Mask);

	return true;
}


bool CListView::CheckItem(int Index, bool fCheck)
{
	if (m_hwnd == nullptr)
		return false;

	ListView_SetCheckState(m_hwnd, Index, fCheck);

	return true;
}


bool CListView::IsItemChecked(int Index) const
{
	if (m_hwnd == nullptr)
		return false;

	return ListView_GetCheckState(m_hwnd, Index) != FALSE;
}


bool CListView::SetItemParam(int Index, LPARAM Param)
{
	if (m_hwnd == nullptr)
		return false;

	LVITEM lvi;

	lvi.mask = LVIF_PARAM;
	lvi.iItem = Index;
	lvi.iSubItem = 0;
	lvi.lParam = Param;

	return ListView_SetItem(m_hwnd, &lvi) != FALSE;
}


LPARAM CListView::GetItemParam(int Index) const
{
	if (m_hwnd == nullptr)
		return 0;

	LVITEM lvi;

	lvi.mask = LVIF_PARAM;
	lvi.iItem = Index;
	lvi.iSubItem = 0;

	if (!ListView_GetItem(m_hwnd, &lvi))
		return 0;

	return lvi.lParam;
}


int CListView::GetItemCount() const
{
	if (m_hwnd == nullptr)
		return 0;

	return ListView_GetItemCount(m_hwnd);
}


int CListView::GetSelectedItem() const
{
	if (m_hwnd == nullptr)
		return -1;

	return ListView_GetNextItem(m_hwnd, -1, LVNI_SELECTED);
}


bool CListView::MoveItem(int From, int To)
{
	if (m_hwnd == nullptr)
		return false;

	LVITEM lvi;
	TCHAR szText[1024];
	lvi.mask = LVIF_STATE | LVIF_TEXT | LVIF_PARAM;
	lvi.iItem = From;
	lvi.iSubItem = 0;
	lvi.stateMask = ~0U;
	lvi.pszText = szText;
	lvi.cchTextMax = lengthof(szText);
	if (!ListView_GetItem(m_hwnd, &lvi))
		return false;
	const BOOL fChecked = ListView_GetCheckState(m_hwnd, From);
	ListView_DeleteItem(m_hwnd, From);
	lvi.iItem = To;
	ListView_InsertItem(m_hwnd, &lvi);
	ListView_SetCheckState(m_hwnd, To, fChecked);

	return true;
}


bool CListView::EnsureItemVisible(int Index, bool fPartialOK)
{
	if (m_hwnd == nullptr)
		return false;

	return ListView_EnsureVisible(m_hwnd, Index, fPartialOK) != FALSE;
}


int CListView::InsertColumn(int Index, LPCTSTR pszText, int Format)
{
	if (m_hwnd == nullptr)
		return false;

	LVCOLUMN lvc;

	lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = Format;
	lvc.pszText = const_cast<LPTSTR>(pszText);
	lvc.iSubItem = Index;

	return ListView_InsertColumn(m_hwnd, Index, &lvc);
}


void CListView::AdjustColumnWidth(bool fUseHeader)
{
	if (m_hwnd != nullptr)
		AdjustListViewColumnWidth(m_hwnd, fUseHeader);
}


void CListView::AdjustSingleColumnWidth()
{
	if (m_hwnd == nullptr)
		return;

	RECT rcWindow;
	::GetWindowRect(m_hwnd, &rcWindow);
	RECT rcFrame = {};
	AdjustWindowRectWithDPI(&rcFrame, GetWindowStyle(m_hwnd), GetWindowExStyle(m_hwnd), false, GetWindowDPI(m_hwnd));
	ListView_SetColumnWidth(m_hwnd, 0, (rcWindow.right - rcWindow.left) - (-rcFrame.left + rcFrame.right + GetScrollBarWidth(m_hwnd)));
}


}	// namespace TVTest
