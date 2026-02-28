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


#ifndef TVTEST_DIALOG_UTIL_H
#define TVTEST_DIALOG_UTIL_H


namespace TVTest
{

	inline bool DlgEdit_SetText(HWND hDlg, int ID, LPCTSTR pszText) {
		return ::SetDlgItemText(hDlg, ID, pszText) != FALSE;
	}
	inline UINT DlgEdit_GetText(HWND hDlg, int ID, LPTSTR pszText, int MaxText) {
		return ::GetDlgItemText(hDlg, ID, pszText, MaxText);
	}
	inline bool DlgEdit_SetInt(HWND hDlg, int ID, int Value) {
		return ::SetDlgItemInt(hDlg, ID, Value, TRUE) != FALSE;
	}
	inline bool DlgEdit_SetUInt(HWND hDlg, int ID, unsigned int Value) {
		return ::SetDlgItemInt(hDlg, ID, Value, FALSE) != FALSE;
	}
	inline int DlgEdit_GetInt(HWND hDlg, int ID, BOOL *pResult = nullptr) {
		return static_cast<INT>(::GetDlgItemInt(hDlg, ID, pResult, TRUE));
	}
	inline unsigned int DlgEdit_GetUInt(HWND hDlg, int ID, BOOL *pResult = nullptr) {
		return ::GetDlgItemInt(hDlg, ID, pResult, FALSE);
	}
	inline void DlgEdit_LimitText(HWND hDlg, int ID, WPARAM Limit) {
		::SendDlgItemMessage(hDlg, ID, EM_LIMITTEXT, Limit, 0);
	}

	inline bool DlgCheckBox_Check(HWND hDlg, int ID, bool fCheck) {
		return ::CheckDlgButton(hDlg, ID, fCheck ? BST_CHECKED : BST_UNCHECKED) != FALSE;
	}
	inline bool DlgCheckBox_IsChecked(HWND hDlg, int ID) {
		return ::IsDlgButtonChecked(hDlg, ID) == BST_CHECKED;
	}

	inline bool DlgRadioButton_IsChecked(HWND hDlg, int ID) {
		return DlgCheckBox_IsChecked(hDlg, ID);
	}

	inline LRESULT DlgListBox_GetCount(HWND hDlg, int ID) {
		return ::SendDlgItemMessage(hDlg, ID, LB_GETCOUNT, 0, 0);
	}
	inline LRESULT DlgListBox_AddString(HWND hDlg, int ID, LPCTSTR pszString) {
		return ::SendDlgItemMessage(hDlg, ID, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(pszString));
	}
	inline LRESULT DlgListBox_AddItem(HWND hDlg, int ID, LPARAM Data) {
		return ::SendDlgItemMessage(hDlg, ID, LB_ADDSTRING, 0, Data);
	}
	inline LRESULT DlgListBox_InsertString(HWND hDlg, int ID, WPARAM Index, LPCTSTR pszString) {
		return ::SendDlgItemMessage(hDlg, ID, LB_INSERTSTRING, Index, reinterpret_cast<LPARAM>(pszString));
	}
	inline LRESULT DlgListBox_InsertItem(HWND hDlg, int ID, WPARAM Index, LPARAM Data) {
		return ::SendDlgItemMessage(hDlg, ID, LB_INSERTSTRING, Index, Data);
	}
	inline LRESULT DlgListBox_DeleteString(HWND hDlg, int ID, WPARAM Index) {
		return ::SendDlgItemMessage(hDlg, ID, LB_DELETESTRING, Index, 0);
	}
	inline LRESULT DlgListBox_DeleteItem(HWND hDlg, int ID, WPARAM Index) {
		return ::SendDlgItemMessage(hDlg, ID, LB_DELETESTRING, Index, 0);
	}
	inline LRESULT DlgListBox_GetString(HWND hDlg, int ID, WPARAM Index, LPTSTR pszString) {
		return ::SendDlgItemMessage(hDlg, ID, LB_GETTEXT, Index, reinterpret_cast<LPARAM>(pszString));
	}
	inline LRESULT DlgListBox_GetStringLength(HWND hDlg, int ID, WPARAM Index) {
		return ::SendDlgItemMessage(hDlg, ID, LB_GETTEXTLEN, Index, 0);
	}
	inline LRESULT DlgListBox_GetString(HWND hDlg, int ID, WPARAM Index, String *pString) {
		LRESULT Length = DlgListBox_GetStringLength(hDlg, ID, Index);
		if (Length > 0) {
			pString->resize(Length + 1);
			Length = DlgListBox_GetString(hDlg, ID, Index, pString->data());
			pString->resize(std::max(Length, (LRESULT)0));
		} else {
			pString->clear();
		}
		return Length;
	}
	inline LPARAM DlgListBox_GetItemData(HWND hDlg, int ID, WPARAM Index) {
		return ::SendDlgItemMessage(hDlg, ID, LB_GETITEMDATA, Index, 0);
	}
	inline bool DlgListBox_SetItemData(HWND hDlg, int ID, WPARAM Index, LPARAM Data) {
		return ::SendDlgItemMessage(hDlg, ID, LB_SETITEMDATA, Index, Data) != LB_ERR;
	}
	inline LRESULT DlgListBox_GetCurSel(HWND hDlg, int ID) {
		return ::SendDlgItemMessage(hDlg, ID, LB_GETCURSEL, 0, 0);
	}
	inline bool DlgListBox_SetCurSel(HWND hDlg, int ID, WPARAM Index) {
		return ::SendDlgItemMessage(hDlg, ID, LB_SETCURSEL, Index, 0) != LB_ERR;
	}
	inline bool DlgListBox_GetSel(HWND hDlg, int ID, WPARAM Index) {
		return (::SendDlgItemMessage(hDlg, ID, LB_GETSEL, Index, 0) > 0);
	}
	inline bool DlgListBox_SetSel(HWND hDlg, int ID, WPARAM Index, bool fSel) {
		return ::SendDlgItemMessage(hDlg, ID, LB_SETSEL, fSel, Index) != LB_ERR;
	}
	inline LRESULT DlgListBox_GetSelCount(HWND hDlg, int ID) {
		return ::SendDlgItemMessage(hDlg, ID, LB_GETSELCOUNT, 0, 0);
	}
	inline LRESULT DlgListBox_GetSelItems(HWND hDlg, int ID, int *pItems, WPARAM Size) {
		return ::SendDlgItemMessage(hDlg, ID, LB_GETSELITEMS, Size, reinterpret_cast<LPARAM>(pItems));
	}
	inline void DlgListBox_Clear(HWND hDlg, int ID) {
		::SendDlgItemMessage(hDlg, ID, LB_RESETCONTENT, 0, 0);
	}
	inline LRESULT DlgListBox_GetTopIndex(HWND hDlg, int ID) {
		return ::SendDlgItemMessage(hDlg, ID, LB_GETTOPINDEX, 0, 0);
	}
	inline bool DlgListBox_SetTopIndex(HWND hDlg, int ID, WPARAM Index) {
		return ::SendDlgItemMessage(hDlg, ID, LB_SETTOPINDEX, Index, 0) != LB_ERR;
	}
	inline LRESULT DlgListBox_GetItemHeight(HWND hDlg, int ID, WPARAM Index) {
		return ::SendDlgItemMessage(hDlg, ID, LB_GETITEMHEIGHT, Index, 0);
	}
	inline bool DlgListBox_SetItemHeight(HWND hDlg, int ID, WPARAM Index, LPARAM Height) {
		return ::SendDlgItemMessage(hDlg, ID, LB_SETITEMHEIGHT, Index, Height) != LB_ERR;
	}
	inline void DlgListBox_SetHorizontalExtent(HWND hDlg, int ID, WPARAM Extent) {
		::SendDlgItemMessage(hDlg, ID, LB_SETHORIZONTALEXTENT, Extent, 0);
	}
	inline LRESULT DlgListBox_FindString(HWND hDlg, int ID, WPARAM First, LPCTSTR pszString) {
		return ::SendDlgItemMessage(hDlg, ID, LB_FINDSTRING, First, reinterpret_cast<LPARAM>(pszString));
	}
	inline LRESULT DlgListBox_FindStringExact(HWND hDlg, int ID, WPARAM First, LPCTSTR pszString) {
		return ::SendDlgItemMessage(hDlg, ID, LB_FINDSTRINGEXACT, First, reinterpret_cast<LPARAM>(pszString));
	}

	inline void DlgComboBox_LimitText(HWND hDlg, int ID, WPARAM Limit) {
		::SendDlgItemMessage(hDlg, ID, CB_LIMITTEXT, Limit, 0);
	}
	inline LRESULT DlgComboBox_GetCount(HWND hDlg, int ID) {
		return ::SendDlgItemMessage(hDlg, ID, CB_GETCOUNT, 0, 0);
	}
	inline LRESULT DlgComboBox_AddString(HWND hDlg, int ID, LPCTSTR pszString) {
		return ::SendDlgItemMessage(hDlg, ID, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(pszString));
	}
	inline LRESULT DlgComboBox_AddItem(HWND hDlg, int ID, LPARAM Data) {
		return ::SendDlgItemMessage(hDlg, ID, CB_ADDSTRING, 0, Data);
	}
	inline LRESULT DlgComboBox_InsertString(HWND hDlg, int ID, WPARAM Index, LPCTSTR pszString) {
		return ::SendDlgItemMessage(hDlg, ID, CB_INSERTSTRING, Index, reinterpret_cast<LPARAM>(pszString));
	}
	inline LRESULT DlgComboBox_InsertItem(HWND hDlg, int ID, WPARAM Index, LPARAM Data) {
		return ::SendDlgItemMessage(hDlg, ID, CB_INSERTSTRING, Index, Data);
	}
	inline LRESULT DlgComboBox_DeleteString(HWND hDlg, int ID, WPARAM Index) {
		return ::SendDlgItemMessage(hDlg, ID, CB_DELETESTRING, Index, 0);
	}
	inline LRESULT DlgComboBox_DeleteItem(HWND hDlg, int ID, WPARAM Index) {
		return ::SendDlgItemMessage(hDlg, ID, CB_DELETESTRING, Index, 0);
	}
	inline LPARAM DlgComboBox_GetItemData(HWND hDlg, int ID, WPARAM Index) {
		return ::SendDlgItemMessage(hDlg, ID, CB_GETITEMDATA, Index, 0);
	}
	inline bool DlgComboBox_SetItemData(HWND hDlg, int ID, WPARAM Index, LPARAM Data) {
		return ::SendDlgItemMessage(hDlg, ID, CB_SETITEMDATA, Index, Data) != CB_ERR;
	}
	inline LRESULT DlgComboBox_GetCurSel(HWND hDlg, int ID) {
		return ::SendDlgItemMessage(hDlg, ID, CB_GETCURSEL, 0, 0);
	}
	inline LRESULT DlgComboBox_SetCurSel(HWND hDlg, int ID, WPARAM Index) {
		return ::SendDlgItemMessage(hDlg, ID, CB_SETCURSEL, Index, 0);
	}
	inline LRESULT DlgComboBox_GetLBString(HWND hDlg, int ID, WPARAM Index, LPTSTR pszString) {
		return ::SendDlgItemMessage(hDlg, ID, CB_GETLBTEXT, Index, reinterpret_cast<LPARAM>(pszString));
	}
	inline LRESULT DlgComboBox_GetLBStringLength(HWND hDlg, int ID, WPARAM Index) {
		return ::SendDlgItemMessage(hDlg, ID, CB_GETLBTEXTLEN, Index, 0);
	}
	inline LRESULT DlgComboBox_GetLBString(HWND hDlg, int ID, WPARAM Index, String *pString) {
		LRESULT Length = DlgComboBox_GetLBStringLength(hDlg, ID, Index);
		if (Length > 0) {
			pString->resize(Length + 1);
			Length = DlgComboBox_GetLBString(hDlg, ID, Index, pString->data());
			pString->resize(std::max(Length, (LRESULT)0));
		} else {
			pString->clear();
		}
		return Length;
	}
	inline void DlgComboBox_Clear(HWND hDlg, int ID) {
		::SendDlgItemMessage(hDlg, ID, CB_RESETCONTENT, 0, 0);
	}
	inline LRESULT DlgComboBox_FindString(HWND hDlg, int ID, WPARAM First, LPCTSTR pszString) {
		return ::SendDlgItemMessage(hDlg, ID, CB_FINDSTRING, First, reinterpret_cast<LPARAM>(pszString));
	}
	inline LRESULT DlgComboBox_FindStringExact(HWND hDlg, int ID, WPARAM First, LPCTSTR pszString) {
		return ::SendDlgItemMessage(hDlg, ID, CB_FINDSTRINGEXACT, First, reinterpret_cast<LPARAM>(pszString));
	}
	inline LRESULT DlgComboBox_GetItemHeight(HWND hDlg, int ID, WPARAM Index) {
		return ::SendDlgItemMessage(hDlg, ID, CB_GETITEMHEIGHT, Index, 0);
	}
	inline bool DlgComboBox_SetItemHeight(HWND hDlg, int ID, WPARAM Index, LPARAM Height) {
		return ::SendDlgItemMessage(hDlg, ID, CB_SETITEMHEIGHT, Index, Height) != CB_ERR;
	}
	inline bool DlgComboBox_SetCueBanner(HWND hDlg, int ID, LPCTSTR pszString) {
		return ::SendDlgItemMessage(hDlg, ID, CB_SETCUEBANNER, 0, reinterpret_cast<LPARAM>(pszString)) == 1;
	}
	inline bool DlgComboBox_GetDroppedState(HWND hDlg, int ID) {
		return ::SendDlgItemMessage(hDlg, ID, CB_GETDROPPEDSTATE, 0, 0) != FALSE;
	}

	inline void DlgUpDown_SetRange(HWND hDlg, int ID, INT32 Low, INT32 High) {
		::SendDlgItemMessage(hDlg, ID, UDM_SETRANGE32, Low, High);
	}
	inline INT32 DlgUpDown_SetPos(HWND hDlg, int ID, INT32 Pos) {
		return static_cast<INT32>(::SendDlgItemMessage(hDlg, ID, UDM_SETPOS32, 0, Pos));
	}

	void EnableDlgItem(HWND hDlg, int ID, bool fEnable);
	void EnableDlgItems(HWND hDlg, int FirstID, int LastID, bool fEnable);
	void InvalidateDlgItem(HWND hDlg, int ID, bool fErase = true);
	void InvalidateDlgItem(HWND hDlg, int ID, const RECT *pRect, bool fErase = true);
	void ShowDlgItem(HWND hDlg, int ID, bool fShow);
	bool GetDlgItemRect(HWND hDlg, int ID, RECT *pRect);
	int GetDlgItemTextLength(HWND hDlg, int ID);
	void SetDlgItemFocus(HWND hDlg, int ID);
	int GetCheckedRadioButton(HWND hDlg, int FirstID, int LastID);
	bool AdjustDialogPos(HWND hwndOwner, HWND hDlg);
	void SyncTrackBarWithEdit(HWND hDlg, int EditID, int TrackbarID);
	void SyncEditWithTrackBar(HWND hDlg, int TrackbarID, int EditID);
	void SetComboBoxList(HWND hDlg, int ID, LPCTSTR pszList);
	void SetComboBoxList(HWND hDlg, int ID, const LPCTSTR *ppszList, int Length);
	bool SetDlgButtonBitmap(HWND hDlg, int ID, HINSTANCE hinst, LPCTSTR pszName);
	bool SetListBoxHExtent(HWND hDlg, int ID);
	LPTSTR GetDlgItemString(HWND hDlg, int ID);
	bool GetDlgItemString(HWND hDlg, int ID, String *pString);
	bool GetDlgListBoxItemString(HWND hDlg, int ID, int Index, String *pString);
	bool GetDlgComboBoxItemString(HWND hDlg, int ID, int Index, String *pString);
	bool EnableDlgItemSyncCheckBox(HWND hDlg, int ID, int CheckBoxID);
	bool EnableDlgItemsSyncCheckBox(HWND hDlg, int FirstID, int LastID, int CheckBoxID);
	BOOL SetDlgItemInt64(HWND hDlg, int ID, ULONGLONG Value, BOOL fSigned);
	ULONGLONG GetDlgItemInt64(HWND hDlg, int ID, BOOL *pfTranslated, BOOL fSigned);
	bool UpdateDlgItemInt(HWND hDlg, int ID, int Value);
	HMENU CreatePopupMenuFromControls(HWND hDlg, const int *pIDList, int IDListLength);
	bool PopupMenuFromControls(
		HWND hDlg, const int *pIDList, int IDListLength, 
		unsigned int Flags = 0, const POINT *ppt = nullptr);
	constexpr unsigned int LBN_EX_RBUTTONDOWN = 0x0100;
	constexpr unsigned int LBN_EX_RBUTTONUP   = 0x0101;
	bool ExtendListBox(HWND hwndList, unsigned int Flags = 0);
	bool SetListViewSortMark(HWND hwndList, int Column, bool fAscending = true);
	bool AdjustListViewColumnWidth(HWND hwndList, bool fUseHeader = true);
	bool SetListViewTooltipsTopMost(HWND hwndList);
	bool InitDropDownButton(HWND hDlg, int ID);
	bool InitDropDownButtonWithText(HWND hDlg, int ID);

} // namespace TVTest


#endif
