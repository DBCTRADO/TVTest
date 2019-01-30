/*
  TVTest
  Copyright(c) 2008-2019 DBCTRADO

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
#include <utility>
#include "TVTest.h"
#include "AppMain.h"
#include "ProgramGuideFavorites.h"
#include "DialogUtil.h"
#include "DrawUtil.h"
#include "DPIUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CProgramGuideFavorites::CProgramGuideFavorites()
	: m_fFixedWidth(true)
{
}


void CProgramGuideFavorites::Clear()
{
	m_List.clear();
}


size_t CProgramGuideFavorites::GetCount() const
{
	return m_List.size();
}


bool CProgramGuideFavorites::Add(const FavoriteInfo &Info)
{
	m_List.push_back(Info);

	return true;
}


bool CProgramGuideFavorites::Get(size_t Index, FavoriteInfo *pInfo) const
{
	if (Index >= m_List.size() || pInfo == nullptr)
		return false;

	*pInfo = m_List[Index];

	return true;
}


CProgramGuideFavorites::FavoriteInfo *CProgramGuideFavorites::Get(size_t Index)
{
	if (Index >= m_List.size())
		return nullptr;

	return &m_List[Index];
}


const CProgramGuideFavorites::FavoriteInfo *CProgramGuideFavorites::Get(size_t Index) const
{
	if (Index >= m_List.size())
		return nullptr;

	return &m_List[Index];
}


bool CProgramGuideFavorites::Set(size_t Index, const FavoriteInfo &Info)
{
	if (Index >= m_List.size())
		return false;

	m_List[Index] = Info;

	return true;
}




void CProgramGuideFavorites::FavoriteInfo::SetDefaultColors()
{
	enum {
		SPACE_TERRESTRIAL = 0x1U,
		SPACE_BS          = 0x2U,
		SPACE_CS          = 0x4U,
		SPACE_FAVORITES   = 0x8U
	};
	unsigned int Space = 0;
	LPCTSTR pszLabel = Label.c_str();

	if (::StrStr(pszLabel, TEXT("地")) != 0
			|| ::StrStrI(pszLabel, TEXT("UHF")) != 0
			|| ::StrStrI(pszLabel, TEXT("VHF")) != 0)
		Space |= SPACE_TERRESTRIAL;
	if (::StrStr(pszLabel, TEXT("BS")) != 0)
		Space |= SPACE_BS;
	if (::StrStr(pszLabel, TEXT("CS")) != 0)
		Space |= SPACE_CS;
	if (::StrStr(pszLabel, TEXT("お気に入り")) != 0)
		Space |= SPACE_FAVORITES;

	switch (Space) {
	case SPACE_TERRESTRIAL:
		BackColor = RGB(12, 200, 87);
		TextColor = RGB(255, 255, 255);
		break;

	case SPACE_BS:
		BackColor = RGB(52, 102, 240);
		TextColor = RGB(255, 255, 255);
		break;

	case SPACE_CS:
		BackColor = RGB(240, 82, 71);
		TextColor = RGB(255, 255, 255);
		break;

	case SPACE_FAVORITES:
		BackColor = RGB(255, 240, 195);
		TextColor = RGB(0, 0, 0);
		break;

	default:
		BackColor = RGB(255, 255, 255);
		TextColor = RGB(0, 0, 0);
		break;
	}
}




CProgramGuideFavoritesDialog::CProgramGuideFavoritesDialog(const CProgramGuideFavorites &Favorites)
	: m_Favorites(Favorites)
	, m_CurItem(-1)
{
}


CProgramGuideFavoritesDialog::~CProgramGuideFavoritesDialog()
{
	Destroy();
}


bool CProgramGuideFavoritesDialog::Show(HWND hwndOwner)
{
	return ShowDialog(
		hwndOwner,
		GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE(IDD_PROGRAMGUIDEFAVORITES)) == IDOK;
}


void CProgramGuideFavoritesDialog::AddNewItem(const CProgramGuideFavorites::FavoriteInfo &Info)
{
	m_Favorites.Add(Info);
	m_CurItem = (int)m_Favorites.GetCount() - 1;
}


INT_PTR CProgramGuideFavoritesDialog::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			HWND hwndList = ::GetDlgItem(hDlg, IDC_PROGRAMGUIDEFAVORITES_LIST);

			m_fChanging = true;

			ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT);
			LVCOLUMN lvc;
			lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM;
			lvc.fmt = LVCFMT_LEFT;
			lvc.pszText = const_cast<LPTSTR>(TEXT(""));
			lvc.iSubItem = 0;
			ListView_InsertColumn(hwndList, 0, &lvc);

			LVITEM lvi;
			lvi.mask = LVIF_TEXT | LVIF_PARAM;
			lvi.iSubItem = 0;
			for (size_t i = 0; i < m_Favorites.GetCount(); i++) {
				CProgramGuideFavorites::FavoriteInfo *pInfo = m_Favorites.Get(i);

				lvi.iItem = (int)i;
				lvi.lParam = reinterpret_cast<LPARAM>(pInfo);
				lvi.pszText = const_cast<LPTSTR>(pInfo->Label.c_str());
				ListView_InsertItem(hwndList, &lvi);
			}

			if (m_CurItem >= 0) {
				ListView_SetItemState(
					hwndList, m_CurItem,
					LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
			}

			ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE_USEHEADER);

			SetItemState(hDlg);
			m_fChanging = false;

			DlgCheckBox_Check(hDlg, IDC_PROGRAMGUIDEFAVORITES_FIXEDWIDTH, m_Favorites.GetFixedWidth());

			AdjustDialogPos(::GetParent(hDlg), hDlg);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_PROGRAMGUIDEFAVORITES_NAME:
			if (HIWORD(wParam) == EN_CHANGE) {
				HWND hwndList = ::GetDlgItem(hDlg, IDC_PROGRAMGUIDEFAVORITES_LIST);
				int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

				if (Sel >= 0) {
					LVITEM lvi;

					lvi.mask = LVIF_PARAM;
					lvi.iItem = Sel;
					lvi.iSubItem = 0;
					if (ListView_GetItem(hwndList, &lvi)) {
						CProgramGuideFavorites::FavoriteInfo *pInfo =
							reinterpret_cast<CProgramGuideFavorites::FavoriteInfo*>(lvi.lParam);
						TCHAR szText[256];

						m_fChanging = true;
						::GetDlgItemText(hDlg, IDC_PROGRAMGUIDEFAVORITES_NAME, szText, lengthof(szText));
						pInfo->Label = szText;
						lvi.mask = LVIF_TEXT;
						lvi.pszText = szText;
						ListView_SetItem(hwndList, &lvi);
						InvalidateDlgItem(hDlg, IDC_PROGRAMGUIDEFAVORITES_COLORS_PREVIEW);
						m_fChanging = false;
					}
				}
			}
			return TRUE;

		case IDC_PROGRAMGUIDEFAVORITES_BACKCOLOR:
			{
				CProgramGuideFavorites::FavoriteInfo *pInfo = GetCurItemInfo();

				if (pInfo != nullptr) {
					if (ChooseColorDialog(hDlg, &pInfo->BackColor)) {
						InvalidateDlgItem(hDlg, IDC_PROGRAMGUIDEFAVORITES_COLORS_PREVIEW);
					}
				}
			}
			return TRUE;

		case IDC_PROGRAMGUIDEFAVORITES_TEXTCOLOR:
			{
				CProgramGuideFavorites::FavoriteInfo *pInfo = GetCurItemInfo();

				if (pInfo != nullptr) {
					if (ChooseColorDialog(hDlg, &pInfo->TextColor)) {
						InvalidateDlgItem(hDlg, IDC_PROGRAMGUIDEFAVORITES_COLORS_PREVIEW);
					}
				}
			}
			return TRUE;

		case IDC_PROGRAMGUIDEFAVORITES_UP:
		case IDC_PROGRAMGUIDEFAVORITES_DOWN:
			{
				HWND hwndList = ::GetDlgItem(hDlg, IDC_PROGRAMGUIDEFAVORITES_LIST);
				int From = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED), To;

				if (From >= 0) {
					if (LOWORD(wParam) == IDC_PROGRAMGUIDEFAVORITES_UP) {
						if (From < 1)
							break;
						To = From - 1;
					} else {
						if (From + 1 >= ListView_GetItemCount(hwndList))
							break;
						To = From + 1;
					}
					m_fChanging = true;
					LVITEM lvi;
					TCHAR szText[256];
					lvi.mask = LVIF_STATE | LVIF_TEXT | LVIF_PARAM;
					lvi.iItem = From;
					lvi.iSubItem = 0;
					lvi.stateMask = ~0U;
					lvi.pszText = szText;
					lvi.cchTextMax = lengthof(szText);
					ListView_GetItem(hwndList, &lvi);
					ListView_DeleteItem(hwndList, From);
					lvi.iItem = To;
					ListView_InsertItem(hwndList, &lvi);
					SetItemState(hDlg);
					m_fChanging = false;
				}
			}
			return TRUE;

		case IDC_PROGRAMGUIDEFAVORITES_DELETE:
			{
				HWND hwndList = ::GetDlgItem(hDlg, IDC_PROGRAMGUIDEFAVORITES_LIST);
				int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

				if (Sel >= 0)
					ListView_DeleteItem(hwndList, Sel);
			}
			return TRUE;

		case IDOK:
			{
				HWND hwndList = ::GetDlgItem(hDlg, IDC_PROGRAMGUIDEFAVORITES_LIST);
				const int ItemCount = ListView_GetItemCount(hwndList);
				CProgramGuideFavorites Favorites;
				LVITEM lvi;

				lvi.mask = LVIF_PARAM;
				lvi.iSubItem = 0;

				for (int i = 0; i < ItemCount; i++) {
					lvi.iItem = i;
					ListView_GetItem(hwndList, &lvi);
					Favorites.Add(*reinterpret_cast<CProgramGuideFavorites::FavoriteInfo*>(lvi.lParam));
				}

				m_Favorites = std::move(Favorites);

				m_Favorites.SetFixedWidth(DlgCheckBox_IsChecked(hDlg, IDC_PROGRAMGUIDEFAVORITES_FIXEDWIDTH));
			}
		case IDCANCEL:
			::EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *pdis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);

			if (pdis->CtlID == IDC_PROGRAMGUIDEFAVORITES_COLORS_PREVIEW) {
				const CProgramGuideFavorites::FavoriteInfo *pInfo = GetCurItemInfo();

				if (pInfo != nullptr) {
					DrawUtil::FillGradient(
						pdis->hDC, &pdis->rcItem,
						pInfo->BackColor,
						MixColor(pInfo->BackColor, RGB(0, 0, 0), 220),
						DrawUtil::FillDirection::Vert);
					COLORREF OldTextColor = ::SetTextColor(pdis->hDC, pInfo->TextColor);
					int OldBkMode = ::SetBkMode(pdis->hDC, TRANSPARENT);
					::DrawText(
						pdis->hDC, pInfo->Label.c_str(), -1, &pdis->rcItem,
						DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
					::SetBkMode(pdis->hDC, OldBkMode);
					::SetTextColor(pdis->hDC, OldTextColor);
				} else {
					::FillRect(pdis->hDC, &pdis->rcItem, reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1));
				}
			}
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case LVN_ITEMCHANGED:
			if (!m_fChanging)
				SetItemState(hDlg);
			return TRUE;
		}
		break;
	}

	return FALSE;
}


void CProgramGuideFavoritesDialog::SetItemState(HWND hDlg)
{
	HWND hwndList = ::GetDlgItem(hDlg, IDC_PROGRAMGUIDEFAVORITES_LIST);
	int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

	if (Sel >= 0) {
		LVITEM lvi;
		lvi.mask = LVIF_PARAM;
		lvi.iItem = Sel;
		lvi.iSubItem = 0;
		ListView_GetItem(hwndList, &lvi);
		const CProgramGuideFavorites::FavoriteInfo *pInfo =
			reinterpret_cast<const CProgramGuideFavorites::FavoriteInfo*>(lvi.lParam);

		EnableDlgItems(
			hDlg,
			IDC_PROGRAMGUIDEFAVORITES_NAME_LABEL,
			IDC_PROGRAMGUIDEFAVORITES_TEXTCOLOR,
			true);
		::SetDlgItemText(
			hDlg, IDC_PROGRAMGUIDEFAVORITES_NAME,
			pInfo->Label.c_str());
		EnableDlgItem(hDlg, IDC_PROGRAMGUIDEFAVORITES_UP, Sel > 0);
		EnableDlgItem(hDlg, IDC_PROGRAMGUIDEFAVORITES_DOWN, Sel + 1 < ListView_GetItemCount(hwndList));
		EnableDlgItem(hDlg, IDC_PROGRAMGUIDEFAVORITES_DELETE, true);
	} else {
		EnableDlgItems(
			hDlg,
			IDC_PROGRAMGUIDEFAVORITES_NAME_LABEL,
			IDC_PROGRAMGUIDEFAVORITES_DELETE,
			false);
		::SetDlgItemText(hDlg, IDC_PROGRAMGUIDEFAVORITES_NAME, TEXT(""));
	}
	InvalidateDlgItem(hDlg, IDC_PROGRAMGUIDEFAVORITES_COLORS_PREVIEW);
}


CProgramGuideFavorites::FavoriteInfo *CProgramGuideFavoritesDialog::GetCurItemInfo()
{
	HWND hwndList = ::GetDlgItem(m_hDlg, IDC_PROGRAMGUIDEFAVORITES_LIST);
	int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

	if (Sel >= 0) {
		LVITEM lvi;

		lvi.mask = LVIF_PARAM;
		lvi.iItem = Sel;
		lvi.iSubItem = 0;
		if (ListView_GetItem(hwndList, &lvi)) {
			return reinterpret_cast<CProgramGuideFavorites::FavoriteInfo*>(lvi.lParam);
		}
	}

	return nullptr;
}


}	// namespace TVTest
