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
#include "DialogUtil.h"
#include "WindowUtil.h"
#include "Common/DebugDef.h"


namespace TVTest
{


void EnableDlgItem(HWND hDlg, int ID, bool fEnable)
{
	EnableWindow(GetDlgItem(hDlg, ID), fEnable);
}


void EnableDlgItems(HWND hDlg, int FirstID, int LastID, bool fEnable)
{
	if (FirstID > LastID)
		std::swap(FirstID, LastID);

	for (int i = FirstID; i <= LastID; i++)
		EnableDlgItem(hDlg, i, fEnable);
}


void InvalidateDlgItem(HWND hDlg, int ID, bool fErase)
{
	InvalidateDlgItem(hDlg, ID, nullptr, fErase);
}


void InvalidateDlgItem(HWND hDlg, int ID, const RECT *pRect, bool fErase)
{
	const HWND hwnd = GetDlgItem(hDlg, ID);
	if (hwnd != nullptr)
		InvalidateRect(hwnd, pRect, fErase);
}


void ShowDlgItem(HWND hDlg, int ID, bool fShow)
{
	ShowWindow(GetDlgItem(hDlg, ID), fShow ? SW_SHOW : SW_HIDE);
}


bool GetDlgItemRect(HWND hDlg, int ID, RECT *pRect)
{
	const HWND hwnd = ::GetDlgItem(hDlg, ID);
	if (hwnd == nullptr)
		return false;
	GetWindowRect(hwnd, pRect);
	MapWindowRect(nullptr, ::GetParent(hwnd), pRect);
	return true;
}


int GetDlgItemTextLength(HWND hDlg, int ID)
{
	return GetWindowTextLength(GetDlgItem(hDlg, ID));
}


void SetDlgItemFocus(HWND hDlg, int ID)
{
	SendMessage(hDlg, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(GetDlgItem(hDlg, ID)), TRUE);
}


int GetCheckedRadioButton(HWND hDlg, int FirstID, int LastID)
{
	if (LastID < FirstID)
		std::swap(FirstID, LastID);

	for (int i = FirstID; i <= LastID; i++) {
		if (IsDlgButtonChecked(hDlg, i) == BST_CHECKED)
			return i;
	}
	return -1;
}


bool AdjustDialogPos(HWND hwndOwner, HWND hDlg)
{
	RECT rcWork, rcWnd, rcDlg;

	if (hwndOwner) {
		const HMONITOR hMonitor = ::MonitorFromWindow(hwndOwner, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi;

		mi.cbSize = sizeof(mi);
		if (!::GetMonitorInfo(hMonitor, &mi))
			return false;
		rcWork = mi.rcWork;
		::GetWindowRect(hwndOwner, &rcWnd);
	} else {
		if (!::SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0))
			return false;
		rcWnd = rcWork;
	}

	::GetWindowRect(hDlg, &rcDlg);
	int x = ((rcWnd.right - rcWnd.left) - (rcDlg.right - rcDlg.left)) / 2 + rcWnd.left;
	if (x < rcWork.left)
		x = rcWork.left;
	else if (x + (rcDlg.right - rcDlg.left) > rcWork.right)
		x = rcWork.right - (rcDlg.right - rcDlg.left);
	int y = ((rcWnd.bottom - rcWnd.top) - (rcDlg.bottom - rcDlg.top)) / 2 + rcWnd.top;
	if (y < rcWork.top)
		y = rcWork.top;
	else if (y + (rcDlg.bottom - rcDlg.top) > rcWork.bottom)
		y = rcWork.bottom - (rcDlg.bottom - rcDlg.top);

	return ::SetWindowPos(hDlg, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE) != FALSE;
}


void SyncTrackBarWithEdit(HWND hDlg, int EditID, int TrackbarID)
{
	const int Min = static_cast<int>(SendDlgItemMessage(hDlg, TrackbarID, TBM_GETRANGEMIN, 0, 0));
	const int Max = static_cast<int>(SendDlgItemMessage(hDlg, TrackbarID, TBM_GETRANGEMAX, 0, 0));
	const int Val = GetDlgItemInt(hDlg, EditID, nullptr, TRUE);
	SendDlgItemMessage(hDlg, TrackbarID, TBM_SETPOS, TRUE, std::clamp(Val, Min, Max));
}


void SyncEditWithTrackBar(HWND hDlg, int TrackbarID, int EditID)
{
	const int Val = static_cast<int>(SendDlgItemMessage(hDlg, TrackbarID, TBM_GETPOS, 0, 0));
	SetDlgItemInt(hDlg, EditID, Val, TRUE);
}


void SetComboBoxList(HWND hDlg, int ID, LPCTSTR pszList)
{
	for (LPCTSTR p = pszList; *p != '\0'; p += lstrlen(p) + 1)
		SendDlgItemMessage(hDlg, ID, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(p));
}


void SetComboBoxList(HWND hDlg, int ID, const LPCTSTR *ppszList, int Length)
{
	for (int i = 0; i < Length; i++)
		SendDlgItemMessage(hDlg, ID, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(ppszList[i]));
}


bool SetDlgButtonBitmap(HWND hDlg, int ID, HINSTANCE hinst, LPCTSTR pszName)
{
	const HBITMAP hbm = static_cast<HBITMAP>(LoadImage(
		hinst, pszName, IMAGE_BITMAP, 0, 0,
		LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS));
	if (hbm == nullptr)
		return false;
	SendDlgItemMessage(hDlg, ID, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(hbm));
	return true;
}


bool SetListBoxHExtent(HWND hDlg, int ID)
{
	const HWND hwnd = GetDlgItem(hDlg, ID);
	int MaxWidth = 0;

	const int Count = static_cast<int>(SendMessage(hwnd, LB_GETCOUNT, 0, 0));
	if (Count >= 1) {
		const HFONT hfont = reinterpret_cast<HFONT>(SendMessage(hwnd, WM_GETFONT, 0, 0));
		if (hfont == nullptr)
			return false;
		const HDC hdc = GetDC(hwnd);
		if (hdc == nullptr)
			return false;
		const HFONT hfontOld = static_cast<HFONT>(SelectObject(hdc, hfont));
		for (int i = 0; i < Count; i++) {
			TCHAR szText[MAX_PATH];
			SendMessage(hwnd, LB_GETTEXT, i, reinterpret_cast<LPARAM>(szText));
			SIZE sz;
			GetTextExtentPoint32(hdc, szText, lstrlen(szText), &sz);
			if (sz.cx > MaxWidth)
				MaxWidth = sz.cx;
		}
		SelectObject(hdc, hfontOld);
		ReleaseDC(hwnd, hdc);
		MaxWidth += 2; /* 余白 */
	}
	SendDlgItemMessage(hDlg, ID, LB_SETHORIZONTALEXTENT, MaxWidth, 0);
	return true;
}


LPTSTR GetDlgItemString(HWND hDlg, int ID)
{
	const int Length = GetDlgItemTextLength(hDlg, ID);
	if (Length <= 0)
		return nullptr;
	const LPTSTR pszString = new TCHAR[Length + 1];
	if (pszString != nullptr)
		GetDlgItemText(hDlg, ID, pszString, Length + 1);
	return pszString;
}


bool GetDlgItemString(HWND hDlg, int ID, String *pString)
{
	if (pString == nullptr)
		return false;

	const int Length = GetDlgItemTextLength(hDlg, ID);
	if (Length > 0) {
		pString->resize(Length + 1);
		pString->resize(GetDlgItemText(hDlg, ID, pString->data(), Length + 1));
	} else {
		pString->clear();
	}

	return true;
}


bool GetDlgListBoxItemString(HWND hDlg, int ID, int Index, String *pString)
{
	if (hDlg == nullptr || pString == nullptr)
		return false;

	int Length = static_cast<int>(DlgListBox_GetStringLength(hDlg, ID, Index));
	if (Length > 0) {
		pString->resize(Length + 1);
		Length = static_cast<int>(DlgListBox_GetString(hDlg, ID, Index, pString->data()));
		pString->resize(std::max(Length, 0));
	} else {
		pString->clear();
	}

	return true;
}


bool GetDlgComboBoxItemString(HWND hDlg, int ID, int Index, String *pString)
{
	if (hDlg == nullptr || pString == nullptr)
		return false;

	int Length = static_cast<int>(DlgComboBox_GetLBStringLength(hDlg, ID, Index));
	if (Length > 0) {
		pString->resize(Length + 1);
		Length = static_cast<int>(DlgComboBox_GetLBString(hDlg, ID, Index, pString->data()));
		pString->resize(std::max(Length, 0));
	} else {
		pString->clear();
	}

	return true;
}


bool EnableDlgItemSyncCheckBox(HWND hDlg, int ID, int CheckBoxID)
{
	const bool fCheck = DlgCheckBox_IsChecked(hDlg, CheckBoxID);

	EnableDlgItem(hDlg, ID, fCheck);
	return fCheck;
}


bool EnableDlgItemsSyncCheckBox(HWND hDlg, int FirstID, int LastID, int CheckBoxID)
{
	const bool fCheck = DlgCheckBox_IsChecked(hDlg, CheckBoxID);

	EnableDlgItems(hDlg, FirstID, LastID, fCheck);
	return fCheck;
}


BOOL SetDlgItemInt64(HWND hDlg, int ID, ULONGLONG Value, BOOL fSigned)
{
	TCHAR szText[20];

	if (fSigned)
		UInt64ToString(Value, szText, lengthof(szText));
	else
		Int64ToString(static_cast<LONGLONG>(Value), szText, lengthof(szText));
	return SetDlgItemText(hDlg, ID, szText);
}


bool UpdateDlgItemInt(HWND hDlg, int ID, int Value)
{
	if (GetDlgItemInt(hDlg, ID, nullptr, TRUE) != Value) {
		SetDlgItemInt(hDlg, ID, Value, TRUE);
		return true;
	}
	return false;
}


ULONGLONG GetDlgItemInt64(HWND hDlg, int ID, BOOL *pfTranslated, BOOL fSigned)
{
	TCHAR szText[20];
	ULONGLONG Value;

	if (pfTranslated != nullptr)
		*pfTranslated = FALSE;
	if (GetDlgItemText(hDlg, ID, szText, lengthof(szText)) == 0)
		return 0;
	if (fSigned)
		Value = static_cast<ULONGLONG>(StringToInt64(szText));
	else
		Value = StringToUInt64(szText);
	if (pfTranslated != nullptr)
		*pfTranslated = TRUE;
	return Value;
}


HMENU CreatePopupMenuFromControls(HWND hDlg, const int *pIDList, int IDListLength)
{
	const HMENU hmenu = CreatePopupMenu();
	if (hmenu == nullptr)
		return nullptr;

	for (int i = 0; i < IDListLength; i++) {
		if (pIDList[i] != 0) {
			const HWND hwnd = GetDlgItem(hDlg, pIDList[i]);
			TCHAR szText[256];
			GetWindowText(hwnd, szText, lengthof(szText));
			unsigned int Flags = MF_STRING;
			if (!IsWindowEnabled(hwnd))
				Flags |= MF_GRAYED;
			AppendMenu(hmenu, Flags, pIDList[i], szText);
		} else {
			AppendMenu(hmenu, MF_SEPARATOR, 0, nullptr);
		}
	}

	return hmenu;
}


bool PopupMenuFromControls(
	HWND hDlg, const int *pIDList, int IDListLength,
	unsigned int Flags, const POINT *ppt)
{
	const HMENU hmenu = CreatePopupMenuFromControls(hDlg, pIDList, IDListLength);
	if (hmenu == nullptr)
		return false;

	POINT pt;
	if (ppt != nullptr)
		pt = *ppt;
	else
		GetCursorPos(&pt);

	TrackPopupMenu(hmenu, Flags, pt.x, pt.y, 0, hDlg, nullptr);
	DestroyMenu(hmenu);
	return true;
}


class CListBoxSubclass
	: public CWindowSubclass
{
	LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	void OnSubclassRemoved() override { delete this; }
};

LRESULT CListBoxSubclass::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_RBUTTONDOWN:
		{
			const int y = GET_Y_LPARAM(lParam);
			const int Index = ListBox_GetTopIndex(hwnd) + y / ListBox_GetItemHeight(hwnd, 0);

			if (Index >= 0 && Index < ListBox_GetCount(hwnd)) {
				if ((GetWindowStyle(hwnd) & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL)) != 0) {
					if (ListBox_GetSel(hwnd, Index) <= 0) {
						ListBox_SetSel(hwnd, TRUE, Index);
						SendMessage(
							GetParent(hwnd), WM_COMMAND,
							MAKEWPARAM(GetWindowID(hwnd), LBN_SELCHANGE),
							reinterpret_cast<LPARAM>(hwnd));
					}
				} else {
					if (Index != ListBox_GetCurSel(hwnd)) {
						ListBox_SetCurSel(hwnd, Index);
						SendMessage(
							GetParent(hwnd), WM_COMMAND,
							MAKEWPARAM(GetWindowID(hwnd), LBN_SELCHANGE),
							reinterpret_cast<LPARAM>(hwnd));
					}
				}
				SendMessage(
					GetParent(hwnd), WM_COMMAND,
					MAKEWPARAM(GetWindowID(hwnd), LBN_EX_RBUTTONDOWN),
					reinterpret_cast<LPARAM>(hwnd));
			}
		}
		return 0;

	case WM_RBUTTONUP:
		SendMessage(
			GetParent(hwnd), WM_COMMAND,
			MAKEWPARAM(GetWindowID(hwnd), LBN_EX_RBUTTONUP),
			reinterpret_cast<LPARAM>(hwnd));
		return 0;
	}

	return CWindowSubclass::OnMessage(hwnd, uMsg, wParam, lParam);
}

bool ExtendListBox(HWND hwndList, unsigned int Flags)
{
	if (hwndList == nullptr)
		return false;

	CListBoxSubclass *pSubclass = new CListBoxSubclass;
	if (!pSubclass->SetSubclass(hwndList)) {
		delete pSubclass;
		return false;
	}

	return true;
}


bool SetListViewSortMark(HWND hwndList, int Column, bool fAscending)
{
	const HMODULE hLib = ::GetModuleHandle(TEXT("comctl32.dll"));
	if (hLib == nullptr)
		return false;

	const DLLGETVERSIONPROC pDllGetVersion = reinterpret_cast<DLLGETVERSIONPROC>(::GetProcAddress(hLib, "DllGetVersion"));

	if (pDllGetVersion == nullptr)
		return false;

	DLLVERSIONINFO dvi;
	dvi.cbSize = sizeof(dvi);
	if (FAILED((*pDllGetVersion)(&dvi)) || dvi.dwMajorVersion < 6)
		return false;

	const HWND hwndHeader = ListView_GetHeader(hwndList);
	HDITEM hdi;

	hdi.mask = HDI_FORMAT;
	const int Count = Header_GetItemCount(hwndHeader);
	for (int i = 0; i < Count; i++) {
		Header_GetItem(hwndHeader, i, &hdi);
		if (i == Column) {
			hdi.fmt =
				(hdi.fmt & ~(HDF_SORTUP | HDF_SORTDOWN)) |
				(fAscending ? HDF_SORTUP : HDF_SORTDOWN);
			Header_SetItem(hwndHeader, i, &hdi);
		} else if ((hdi.fmt & (HDF_SORTUP | HDF_SORTDOWN)) != 0) {
			hdi.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
			Header_SetItem(hwndHeader, i, &hdi);
		}
	}

	return true;
}


bool AdjustListViewColumnWidth(HWND hwndList, bool fUseHeader)
{
	if (hwndList == nullptr)
		return false;

	const int Count = Header_GetItemCount(ListView_GetHeader(hwndList));
	for (int i = 0; i < Count; i++)
		ListView_SetColumnWidth(hwndList, i, fUseHeader ? LVSCW_AUTOSIZE_USEHEADER : LVSCW_AUTOSIZE);

	return true;
}


static LRESULT CALLBACK TooltipsTopMostSubclassProc(
	HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (uMsg) {
	case WM_NOTIFY:
		{
			const NMHDR *pnmh = reinterpret_cast<const NMHDR *>(lParam);
			if (pnmh->code == TTN_SHOW) {
				::SetWindowPos(pnmh->hwndFrom, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
			}
		}
		break;

	case WM_NCDESTROY:
		::RemoveWindowSubclass(hWnd, TooltipsTopMostSubclassProc, uIdSubclass);
		break;
	}

	return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// リストビューのツールチップを最前面に表示する
// 親ウィンドウ(または親ウィンドウのオーナー)が最前面表示されていると背後に隠れてしまうため
bool SetListViewTooltipsTopMost(HWND hwndList)
{
	return ::SetWindowSubclass(hwndList, TooltipsTopMostSubclassProc, 1, 0);
}


// ボタンをドロップダウンメニュー表示用に初期化する
bool InitDropDownButton(HWND hDlg, int ID)
{
	const HWND hwnd = ::GetDlgItem(hDlg, ID);
	if (hwnd == nullptr)
		return false;

	::SetWindowLong(hwnd, GWL_STYLE, ::GetWindowLong(hwnd, GWL_STYLE) | BS_SPLITBUTTON);
	::SetWindowText(hwnd, TEXT(""));
	RECT rc;
	::GetClientRect(hwnd, &rc);
	BUTTON_SPLITINFO bsi;
	bsi.mask = BCSIF_STYLE | BCSIF_SIZE;
	bsi.uSplitStyle = BCSS_NOSPLIT | BCSS_STRETCH | BCSS_ALIGNLEFT;
	bsi.size.cx = rc.right;
	bsi.size.cy = rc.bottom;
	::SendMessage(hwnd, BCM_SETSPLITINFO, 0, reinterpret_cast<LPARAM>(&bsi));

	return true;
}


bool InitDropDownButtonWithText(HWND hDlg, int ID)
{
	const HWND hwnd = ::GetDlgItem(hDlg, ID);
	if (hwnd == nullptr)
		return false;

	::SetWindowLong(hwnd, GWL_STYLE, ::GetWindowLong(hwnd, GWL_STYLE) | BS_SPLITBUTTON);
	BUTTON_SPLITINFO bsi;
	bsi.mask = BCSIF_STYLE;
	bsi.uSplitStyle = BCSS_NOSPLIT;
	Button_SetSplitInfo(hwnd, &bsi);

	return true;
}


} // namespace TVTest
