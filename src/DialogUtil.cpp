#include "stdafx.h"
#include "TVTest.h"
#include "DialogUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




void EnableDlgItem(HWND hDlg,int ID,bool fEnable)
{
	EnableWindow(GetDlgItem(hDlg,ID),fEnable);
}


void EnableDlgItems(HWND hDlg,int FirstID,int LastID,bool fEnable)
{
	int i;

	if (FirstID>LastID) {
		i=FirstID;
		FirstID=LastID;
		LastID=i;
	}
	for (i=FirstID;i<=LastID;i++)
		EnableDlgItem(hDlg,i,fEnable);
}


void InvalidateDlgItem(HWND hDlg,int ID,bool fErase)
{
	InvalidateDlgItem(hDlg,ID,NULL,fErase);
}


void InvalidateDlgItem(HWND hDlg,int ID,const RECT *pRect,bool fErase)
{
	HWND hwnd=GetDlgItem(hDlg,ID);
	if (hwnd!=NULL)
		InvalidateRect(hwnd,pRect,fErase);
}


void ShowDlgItem(HWND hDlg,int ID,bool fShow)
{
	ShowWindow(GetDlgItem(hDlg,ID),fShow?SW_SHOW:SW_HIDE);
}


bool GetDlgItemRect(HWND hDlg,int ID,RECT *pRect)
{
	HWND hwnd=::GetDlgItem(hDlg,ID);
	if (hwnd==NULL)
		return false;
	GetWindowRect(hwnd,pRect);
	MapWindowRect(NULL,::GetParent(hwnd),pRect);
	return true;
}


int GetDlgItemTextLength(HWND hDlg,int ID)
{
	return GetWindowTextLength(GetDlgItem(hDlg,ID));
}


void SetDlgItemFocus(HWND hDlg,int ID)
{
	SetFocus(GetDlgItem(hDlg,ID));
}


int GetCheckedRadioButton(HWND hDlg,int FirstID,int LastID)
{
	int i;

	if (LastID<FirstID) {
		i=FirstID;
		FirstID=LastID;
		LastID=i;
	}
	for (i=FirstID;i<=LastID;i++) {
		if (IsDlgButtonChecked(hDlg,i)==BST_CHECKED)
			return i;
	}
	return -1;
}


bool AdjustDialogPos(HWND hwndOwner,HWND hDlg)
{
	RECT rcWork,rcWnd,rcDlg;
	int x,y;

	if (!SystemParametersInfo(SPI_GETWORKAREA,0,&rcWork,0))
		return false;
	if (hwndOwner)
		GetWindowRect(hwndOwner,&rcWnd);
	else
		rcWnd=rcWork;
	GetWindowRect(hDlg,&rcDlg);
	x=((rcWnd.right-rcWnd.left)-(rcDlg.right-rcDlg.left))/2+rcWnd.left;
	if (x<rcWork.left)
		x=rcWork.left;
	else if (x+(rcDlg.right-rcDlg.left)>rcWork.right)
		x=rcWork.right-(rcDlg.right-rcDlg.left);
	y=((rcWnd.bottom-rcWnd.top)-(rcDlg.bottom-rcDlg.top))/2+rcWnd.top;
	if (y<rcWork.top)
		y=rcWork.top;
	else if (y+(rcDlg.bottom-rcDlg.top)>rcWork.bottom)
		y=rcWork.bottom-(rcDlg.bottom-rcDlg.top);
	return SetWindowPos(hDlg,NULL,x,y,0,0,SWP_NOSIZE | SWP_NOZORDER)!=FALSE;
}


void SyncTrackBarWithEdit(HWND hDlg,int EditID,int TrackbarID)
{
	int Min,Max,Val;

	Min=(int)SendDlgItemMessage(hDlg,TrackbarID,TBM_GETRANGEMIN,0,0);
	Max=(int)SendDlgItemMessage(hDlg,TrackbarID,TBM_GETRANGEMAX,0,0);
	Val=GetDlgItemInt(hDlg,EditID,NULL,TRUE);
	SendDlgItemMessage(hDlg,TrackbarID,TBM_SETPOS,TRUE,CLAMP(Val,Min,Max));
}


void SyncEditWithTrackBar(HWND hDlg,int TrackbarID,int EditID)
{
	int Val;

	Val=(int)SendDlgItemMessage(hDlg,TrackbarID,TBM_GETPOS,0,0);
	SetDlgItemInt(hDlg,EditID,Val,TRUE);
}


void SetComboBoxList(HWND hDlg,int ID,LPCTSTR pszList)
{
	LPCTSTR p;

	for (p=pszList;*p!='\0';p+=lstrlen(p)+1)
		SendDlgItemMessage(hDlg,ID,CB_ADDSTRING,0,(LPARAM)p);
}


void SetComboBoxList(HWND hDlg,int ID,const LPCTSTR *ppszList,int Length)
{
	for (int i=0;i<Length;i++)
		SendDlgItemMessage(hDlg,ID,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(ppszList[i]));
}


bool SetDlgButtonBitmap(HWND hDlg,int ID,HINSTANCE hinst,LPCTSTR pszName)
{
	HBITMAP hbm;

	hbm=(HBITMAP)LoadImage(hinst,pszName,IMAGE_BITMAP,0,0,
										LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS);
	if (hbm==NULL)
		return false;
	SendDlgItemMessage(hDlg,ID,BM_SETIMAGE,IMAGE_BITMAP,reinterpret_cast<LPARAM>(hbm));
	return true;
}


bool SetListBoxHExtent(HWND hDlg,int ID)
{
	HWND hwnd=GetDlgItem(hDlg,ID);
	int Count,MaxWidth=0;

	Count=(int)SendMessage(hwnd,LB_GETCOUNT,0,0);
	if (Count>=1) {
		HFONT hfont,hfontOld;
		HDC hdc;
		int i;
		TCHAR szText[MAX_PATH];
		SIZE sz;

		hfont=(HFONT)(UINT_PTR)SendMessage(hwnd,WM_GETFONT,0,0);
		if (hfont==NULL || (hdc=GetDC(hwnd))==NULL)
			return false;
		hfontOld=static_cast<HFONT>(SelectObject(hdc,hfont));
		for (i=0;i<Count;i++) {
			SendMessage(hwnd,LB_GETTEXT,i,(LPARAM)szText);
			GetTextExtentPoint32(hdc,szText,lstrlen(szText),&sz);
			if (sz.cx>MaxWidth)
				MaxWidth=sz.cx;
		}
		SelectObject(hdc,hfontOld);
		ReleaseDC(hwnd,hdc);
		MaxWidth+=2;	/* —]”’ */
	}
	SendDlgItemMessage(hDlg,ID,LB_SETHORIZONTALEXTENT,MaxWidth,0);
	return true;
}


LPTSTR GetDlgItemString(HWND hDlg,int ID)
{
	int Length;
	LPTSTR pszString;

	Length=GetDlgItemTextLength(hDlg,ID);
	if (Length<=0)
		return NULL;
	pszString=new TCHAR[Length+1];
	if (pszString!=NULL)
		GetDlgItemText(hDlg,ID,pszString,Length+1);
	return pszString;
}


bool EnableDlgItemSyncCheckBox(HWND hDlg,int ID,int CheckBoxID)
{
	bool fCheck=DlgCheckBox_IsChecked(hDlg,CheckBoxID);

	EnableDlgItem(hDlg,ID,fCheck);
	return fCheck;
}


bool EnableDlgItemsSyncCheckBox(HWND hDlg,int FirstID,int LastID,int CheckBoxID)
{
	bool fCheck=DlgCheckBox_IsChecked(hDlg,CheckBoxID);

	EnableDlgItems(hDlg,FirstID,LastID,fCheck);
	return fCheck;
}


BOOL SetDlgItemInt64(HWND hDlg,int ID,ULONGLONG Value,BOOL fSigned)
{
	TCHAR szText[20];

	if (fSigned)
		UInt64ToString(Value,szText,lengthof(szText));
	else
		Int64ToString((LONGLONG)Value,szText,lengthof(szText));
	return SetDlgItemText(hDlg,ID,szText);
}


bool UpdateDlgItemInt(HWND hDlg,int ID,int Value)
{
	if (GetDlgItemInt(hDlg,ID,NULL,TRUE)!=Value) {
		SetDlgItemInt(hDlg,ID,Value,TRUE);
		return true;
	}
	return false;
}


ULONGLONG GetDlgItemInt64(HWND hDlg,int ID,BOOL *pfTranslated,BOOL fSigned)
{
	TCHAR szText[20];
	ULONGLONG Value;

	if (pfTranslated!=NULL)
		*pfTranslated=FALSE;
	if (GetDlgItemText(hDlg,ID,szText,lengthof(szText))==0)
		return 0;
	if (fSigned)
		Value=(ULONGLONG)StringToInt64(szText);
	else
		Value=StringToUInt64(szText);
	if (pfTranslated!=NULL)
		*pfTranslated=TRUE;
	return Value;
}


HMENU CreatePopupMenuFromControls(HWND hDlg,const int *pIDList,int IDListLength)
{
	HMENU hmenu;
	int i;
	TCHAR szText[256];
	HWND hwnd;
	unsigned int Flags;

	hmenu=CreatePopupMenu();
	if (hmenu==NULL)
		return NULL;
	for (i=0;i<IDListLength;i++) {
		if (pIDList[i]!=0) {
			hwnd=GetDlgItem(hDlg,pIDList[i]);
			GetWindowText(hwnd,szText,lengthof(szText));
			Flags=MFT_STRING;
			if (!IsWindowEnabled(hwnd))
				Flags|=MFS_GRAYED;
			AppendMenu(hmenu,Flags,pIDList[i],szText);
		} else {
			AppendMenu(hmenu,MFT_SEPARATOR,0,NULL);
		}
	}
	return hmenu;
}


bool PopupMenuFromControls(HWND hDlg,const int *pIDList,int IDListLength,
										unsigned int Flags,const POINT *ppt)
{
	HMENU hmenu;
	POINT pt;

	hmenu=CreatePopupMenuFromControls(hDlg,pIDList,IDListLength);
	if (hmenu==NULL)
		return false;
	if (ppt!=NULL)
		pt=*ppt;
	else
		GetCursorPos(&pt);
	TrackPopupMenu(hmenu,Flags,pt.x,pt.y,0,hDlg,NULL);
	DestroyMenu(hmenu);
	return true;
}


static LRESULT CALLBACK ListBoxHookProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	WNDPROC pOldWndProc=static_cast<WNDPROC>(GetProp(hwnd,TEXT("ExListBox")));

	if (pOldWndProc==NULL)
		return DefWindowProc(hwnd,uMsg,wParam,lParam);
	switch (uMsg) {
	case WM_RBUTTONDOWN:
		{
			int y=GET_Y_LPARAM(lParam);
			int Index;

			Index=ListBox_GetTopIndex(hwnd)+y/ListBox_GetItemHeight(hwnd,0);
			if (Index>=0 && Index<ListBox_GetCount(hwnd)) {
				if ((GetWindowStyle(hwnd)&
									(LBS_MULTIPLESEL | LBS_EXTENDEDSEL))!=0) {
					if (ListBox_GetSel(hwnd,Index)<=0) {
						(void)ListBox_SetSel(hwnd,TRUE,Index);
						SendMessage(GetParent(hwnd),WM_COMMAND,
									MAKEWPARAM(GetWindowID(hwnd),LBN_SELCHANGE),
									reinterpret_cast<LPARAM>(hwnd));
					}
				} else {
					if (Index!=ListBox_GetCurSel(hwnd)) {
						(void)ListBox_SetCurSel(hwnd,Index);
						SendMessage(GetParent(hwnd),WM_COMMAND,
									MAKEWPARAM(GetWindowID(hwnd),LBN_SELCHANGE),
									reinterpret_cast<LPARAM>(hwnd));
					}
				}
				SendMessage(GetParent(hwnd),WM_COMMAND,
							MAKEWPARAM(GetWindowID(hwnd),LBN_EX_RBUTTONDOWN),
							reinterpret_cast<LPARAM>(hwnd));
			}
		}
		return 0;

	case WM_RBUTTONUP:
		return 0;

	case WM_NCDESTROY:
		CallWindowProc(pOldWndProc,hwnd,uMsg,wParam,lParam);
		RemoveProp(hwnd,TEXT("ExListBox"));
		return 0;
	}
	return CallWindowProc(pOldWndProc,hwnd,uMsg,wParam,lParam);
}

bool ExtendListBox(HWND hwndList,unsigned int Flags)
{
	WNDPROC pOldProc;

	pOldProc=SubclassWindow(hwndList,ListBoxHookProc);
	SetProp(hwndList,TEXT("ExListBox"),pOldProc);
	return true;
}


bool SetListViewSortMark(HWND hwndList,int Column,bool fAscending)
{
	HMODULE hLib=::GetModuleHandle(TEXT("comctl32.dll"));
	if (hLib==NULL)
		return false;

	DLLGETVERSIONPROC pDllGetVersion=(DLLGETVERSIONPROC)::GetProcAddress(hLib,"DllGetVersion");

	if (pDllGetVersion==NULL)
		return false;

	DLLVERSIONINFO dvi;
	dvi.cbSize=sizeof(dvi);
	if (FAILED((*pDllGetVersion)(&dvi)) || dvi.dwMajorVersion<6)
		return false;

	HWND hwndHeader=ListView_GetHeader(hwndList);
	HDITEM hdi;
#ifndef HDF_SORTUP
#define HDF_SORTUP		0x0400
#define HDF_SORTDOWN	0x0200
#endif

	hdi.mask=HDI_FORMAT;
	int Count=Header_GetItemCount(hwndHeader);
	for (int i=0;i<Count;i++) {
		Header_GetItem(hwndHeader,i,&hdi);
		if (i==Column) {
			hdi.fmt=(hdi.fmt&~(HDF_SORTUP | HDF_SORTDOWN)) |
					(fAscending?HDF_SORTUP:HDF_SORTDOWN);
			Header_SetItem(hwndHeader,i,&hdi);
		} else if ((hdi.fmt&(HDF_SORTUP | HDF_SORTDOWN))!=0) {
			hdi.fmt&=~(HDF_SORTUP | HDF_SORTDOWN);
			Header_SetItem(hwndHeader,i,&hdi);
		}
	}

	return true;
}
