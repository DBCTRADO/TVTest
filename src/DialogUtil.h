#ifndef DIALOG_UTIL_H
#define DIALOG_UTIL_H


#define DlgEdit_SetText(hwndDlg,ID,pszText) \
	::SetDlgItemText(hwndDlg,ID,pszText)
#define DlgEdit_GetText(hwndDlg,ID,pszText,TextLength) \
	::GetDlgItemText(hwndDlg,ID,pszText,TextLength)
#define DlgEdit_SetInt(hwndDlg,ID,Value) \
	::SetDlgItemInt(hwndDlg,ID,Value,TRUE)
#define DlgEdit_SetUInt(hwndDlg,ID,Value) \
	::SetDlgItemInt(hwndDlg,ID,Value,FALSE)
#define DlgEdit_GetInt(hwndDlg,ID) \
	((int)::GetDlgItemInt(hwndDlg,ID,NULL,TRUE))
#define DlgEdit_GetUInt(hwndDlg,ID) \
	::GetDlgItemInt(hwndDlg,ID,NULL,FALSE)
#define DlgEdit_LimitText(hwndDlg,ID,Limit) \
	::SendDlgItemMessage(hwndDlg,ID,EM_LIMITTEXT,Limit,0)

#define DlgCheckBox_Check(hwndDlg,ID,fCheck) \
	::CheckDlgButton(hwndDlg,ID,(fCheck)?BST_CHECKED:BST_UNCHECKED)
#define DlgCheckBox_IsChecked(hwndDlg,ID) \
	(::IsDlgButtonChecked(hwndDlg,ID)==BST_CHECKED)

#define DlgRadioButton_IsChecked(hwndDlg,ID) DlgCheckBox_IsChecked(hwndDlg,ID)

#define DlgListBox_GetCount(hwndDlg,ID) \
	::SendDlgItemMessage(hwndDlg,ID,LB_GETCOUNT,0,0)
#define DlgListBox_AddString(hwndDlg,ID,pszString) \
	::SendDlgItemMessage(hwndDlg,ID,LB_ADDSTRING,0,(LPARAM)(pszString))
#define DlgListBox_AddItem(hwndDlg,ID,Data) \
	::SendDlgItemMessage(hwndDlg,ID,LB_ADDSTRING,0,Data)
#define DlgListBox_InsertString(hwndDlg,ID,Index,pszString) \
	::SendDlgItemMessage(hwndDlg,ID,LB_INSERTSTRING,Index,(LPARAM)(pszString))
#define DlgListBox_InsertItem(hwndDlg,ID,Index,Data) \
	::SendDlgItemMessage(hwndDlg,ID,LB_INSERTSTRING,Index,Data)
#define DlgListBox_DeleteString(hwndDlg,ID,Index) \
	::SendDlgItemMessage(hwndDlg,ID,LB_DELETESTRING,Index,0)
#define DlgListBox_DeleteItem(hwndDlg,ID,Index) \
	::SendDlgItemMessage(hwndDlg,ID,LB_DELETESTRING,Index,0)
#define DlgListBox_GetString(hwndDlg,ID,Index,pszString) \
	::SendDlgItemMessage(hwndDlg,ID,LB_GETTEXT,Index,(LPARAM)(pszString))
#define DlgListBox_GetStringLength(hwndDlg,ID,Index) \
	::SendDlgItemMessage(hwndDlg,ID,LB_GETTEXTLEN,Index,0)
#define DlgListBox_GetItemData(hwndDlg,ID,Index) \
	::SendDlgItemMessage(hwndDlg,ID,LB_GETITEMDATA,Index,0)
#define DlgListBox_SetItemData(hwndDlg,ID,Index,Data) \
	::SendDlgItemMessage(hwndDlg,ID,LB_SETITEMDATA,Index,Data)
#define DlgListBox_GetCurSel(hwndDlg,ID) \
	::SendDlgItemMessage(hwndDlg,ID,LB_GETCURSEL,0,0)
#define DlgListBox_SetCurSel(hwndDlg,ID,Index) \
	::SendDlgItemMessage(hwndDlg,ID,LB_SETCURSEL,Index,0)
#define DlgListBox_GetSel(hwndDlg,ID,Index) \
	(::SendDlgItemMessage(hwndDlg,ID,LB_GETSEL,Index,0)>0)
#define DlgListBox_SetSel(hwndDlg,ID,Index,fSel) \
	::SendDlgItemMessage(hwndDlg,ID,LB_SETSEL,fSel,Index)
#define DlgListBox_GetSelCount(hwndDlg,ID) \
	::SendDlgItemMessage(hwndDlg,ID,LB_GETSELCOUNT,0,0)
#define DlgListBox_GetSelItems(hwndDlg,ID,pItems,Size) \
	::SendDlgItemMessage(hwndDlg,ID,LB_GETSELITEMS,Size,(LPARAM)(pItems))
#define DlgListBox_Clear(hwndDlg,ID) \
	::SendDlgItemMessage(hwndDlg,ID,LB_RESETCONTENT,0,0)
#define DlgListBox_GetTopIndex(hwndDlg,ID) \
	::SendDlgItemMessage(hwndDlg,ID,LB_GETTOPINDEX,0,0)
#define DlgListBox_SetTopIndex(hwndDlg,ID,Index) \
	(::SendDlgItemMessage(hwndDlg,ID,LB_SETTOPINDEX,Index,0)!=LB_ERR)
#define DlgListBox_GetItemHeight(hwndDlg,ID,Index) \
	::SendDlgItemMessage(hwndDlg,ID,LB_GETITEMHEIGHT,Index,0)
#define DlgListBox_SetItemHeight(hwndDlg,ID,Index,Height) \
	(::SendDlgItemMessage(hwndDlg,ID,LB_SETITEMHEIGHT,Index,Height)!=LB_ERR)
#define DlgListBox_SetHorizontalExtent(hwndDlg,ID,Extent) \
	::SendDlgItemMessage(hwndDlg,ID,LB_SETHORIZONTALEXTENT,Extent,0)

#define DlgComboBox_LimitText(hwndDlg,ID,Limit) \
	::SendDlgItemMessage(hwndDlg,ID,CB_LIMITTEXT,Limit,0)
#define DlgComboBox_GetCount(hwndDlg,ID) \
	::SendDlgItemMessage(hwndDlg,ID,CB_GETCOUNT,0,0)
#define DlgComboBox_AddString(hwndDlg,ID,pszString) \
	::SendDlgItemMessage(hwndDlg,ID,CB_ADDSTRING,0,(LPARAM)(pszString))
#define DlgComboBox_AddItem(hwndDlg,ID,Data) \
	::SendDlgItemMessage(hwndDlg,ID,CB_ADDSTRING,0,Data)
#define DlgComboBox_InsertString(hwndDlg,ID,Index,pszString) \
	::SendDlgItemMessage(hwndDlg,ID,CB_INSERTSTRING,Index,(LPARAM)(pszString))
#define DlgComboBox_InsertItem(hwndDlg,ID,Index,Data) \
	::SendDlgItemMessage(hwndDlg,ID,CB_INSERTSTRING,Index,Data)
#define DlgComboBox_DeleteString(hwndDlg,ID,Index) \
	::SendDlgItemMessage(hwndDlg,ID,CB_DELETESTRING,Index,0)
#define DlgComboBox_DeleteItem(hwndDlg,ID,Index) \
	::SendDlgItemMessage(hwndDlg,ID,CB_DELETESTRING,Index,0)
#define DlgComboBox_GetItemData(hwndDlg,ID,Index) \
	::SendDlgItemMessage(hwndDlg,ID,CB_GETITEMDATA,Index,0)
#define DlgComboBox_SetItemData(hwndDlg,ID,Index,Data) \
	::SendDlgItemMessage(hwndDlg,ID,CB_SETITEMDATA,Index,Data)
#define DlgComboBox_GetCurSel(hwndDlg,ID) \
	::SendDlgItemMessage(hwndDlg,ID,CB_GETCURSEL,0,0)
#define DlgComboBox_SetCurSel(hwndDlg,ID,Index) \
	::SendDlgItemMessage(hwndDlg,ID,CB_SETCURSEL,Index,0)
#define DlgComboBox_GetLBString(hwndDlg,ID,Index,pszString) \
	::SendDlgItemMessage(hwndDlg,ID,CB_GETLBTEXT,Index,(LPARAM)(pszString))
#define DlgComboBox_GetLBStringLength(hwndDlg,ID,Index) \
	::SendDlgItemMessage(hwndDlg,ID,CB_GETLBTEXTLEN,Index,0)
#define DlgComboBox_Clear(hwndDlg,ID) \
	::SendDlgItemMessage(hwndDlg,ID,CB_RESETCONTENT,0,0)
#define DlgComboBox_FindString(hwndDlg,ID,First,pszString) \
	::SendDlgItemMessage(hwndDlg,ID,CB_FINDSTRING,First,(LPARAM)(pszString))
#define DlgComboBox_FindStringExact(hwndDlg,ID,First,pszString) \
	::SendDlgItemMessage(hwndDlg,ID,CB_FINDSTRINGEXACT,First,(LPARAM)(pszString))
#define DlgComboBox_GetItemHeight(hwndDlg,ID,Index) \
	::SendDlgItemMessage(hwndDlg,ID,CB_GETITEMHEIGHT,Index,0)
#define DlgComboBox_SetItemHeight(hwndDlg,ID,Index,Height) \
	(::SendDlgItemMessage(hwndDlg,ID,CB_SETITEMHEIGHT,Index,Height)!=CB_ERR)
#define DlgComboBox_SetCueBanner(hwndDlg,ID,pszString) \
	::SendDlgItemMessage(hwndDlg,ID,CB_SETCUEBANNER,0,(LPARAM)(pszString))

#define DlgUpDown_SetRange(hwndDlg,ID,Low,High) \
	::SendDlgItemMessage(hwndDlg,ID,UDM_SETRANGE32,Low,High)
#define DlgUpDown_SetPos(hwndDlg,ID,Pos) \
	::SendDlgItemMessage(hwndDlg,ID,UDM_SETPOS32,0,Pos)

void EnableDlgItem(HWND hDlg,int ID,bool fEnable);
void EnableDlgItems(HWND hDlg,int FirstID,int LastID,bool fEnable);
void InvalidateDlgItem(HWND hDlg,int ID,bool fErase=true);
void InvalidateDlgItem(HWND hDlg,int ID,const RECT *pRect,bool fErase=true);
void ShowDlgItem(HWND hDlg,int ID,bool fShow);
bool GetDlgItemRect(HWND hDlg,int ID,RECT *pRect);
int GetDlgItemTextLength(HWND hDlg,int ID);
void SetDlgItemFocus(HWND hDlg,int ID);
int GetCheckedRadioButton(HWND hDlg,int FirstID,int LastID);
bool AdjustDialogPos(HWND hwndOwner,HWND hDlg);
void SyncTrackBarWithEdit(HWND hDlg,int EditID,int TrackbarID);
void SyncEditWithTrackBar(HWND hDlg,int TrackbarID,int EditID);
void SetComboBoxList(HWND hDlg,int ID,LPCTSTR pszList);
void SetComboBoxList(HWND hDlg,int ID,const LPCTSTR *ppszList,int Length);
bool SetDlgButtonBitmap(HWND hDlg,int ID,HINSTANCE hinst,LPCTSTR pszName);
bool SetListBoxHExtent(HWND hDlg,int ID);
LPTSTR GetDlgItemString(HWND hDlg,int ID);
bool EnableDlgItemSyncCheckBox(HWND hDlg,int ID,int CheckBoxID);
bool EnableDlgItemsSyncCheckBox(HWND hDlg,int FirstID,int LastID,int CheckBoxID);
BOOL SetDlgItemInt64(HWND hDlg,int ID,ULONGLONG Value,BOOL fSigned);
ULONGLONG GetDlgItemInt64(HWND hDlg,int ID,BOOL *pfTranslated,BOOL fSigned);
bool UpdateDlgItemInt(HWND hDlg,int ID,int Value);
HMENU CreatePopupMenuFromControls(HWND hDlg,const int *pIDList,int IDListLength);
bool PopupMenuFromControls(HWND hDlg,const int *pIDList,int IDListLength,
						   unsigned int Flags=0,const POINT *ppt=NULL);
#define LBN_EX_RBUTTONDOWN	0x0100
bool ExtendListBox(HWND hwndList,unsigned int Flags=0);
bool SetListViewSortMark(HWND hwndList,int Column,bool fAscending=true);


#endif
