#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "ZoomOptions.h"
#include "DialogUtil.h"
#include "HelperClass/StdUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define MAX_ZOOM_TEXT 64


#ifdef TVH264_FOR_1SEG
#define f1Seg true
#define BASE_WIDTH 320
#define BASE_HEIGHT 180
#else
#define f1Seg false
#define BASE_WIDTH 1920
#define BASE_HEIGHT 1080
#endif

const CZoomOptions::ZoomCommandInfo CZoomOptions::m_DefaultZoomList[NUM_ZOOM_COMMANDS] = {
	{CM_ZOOM_20,			{ZOOM_RATE,	{  1,   5},	{BASE_WIDTH/5,   BASE_HEIGHT/5},	!f1Seg}},
	{CM_ZOOM_25,			{ZOOM_RATE,	{  1,   4},	{BASE_WIDTH/4,   BASE_HEIGHT/4},	!f1Seg}},
	{CM_ZOOM_33,			{ZOOM_RATE,	{  1,   3},	{BASE_WIDTH/3,   BASE_HEIGHT/3},	!f1Seg}},
	{CM_ZOOM_50,			{ZOOM_RATE,	{  1,   2},	{BASE_WIDTH/2,   BASE_HEIGHT/2},	true}},
	{CM_ZOOM_66,			{ZOOM_RATE,	{  2,   3},	{BASE_WIDTH*2/3, BASE_HEIGHT*2/3},	true}},
	{CM_ZOOM_75,			{ZOOM_RATE,	{  3,   4},	{BASE_WIDTH*3/4, BASE_HEIGHT*3/4},	true}},
	{CM_ZOOM_100,			{ZOOM_RATE,	{  1,   1},	{BASE_WIDTH,     BASE_HEIGHT},		true}},
	{CM_ZOOM_150,			{ZOOM_RATE,	{  3,   2},	{BASE_WIDTH*3/2, BASE_HEIGHT*3/2},	true}},
	{CM_ZOOM_200,			{ZOOM_RATE,	{  2,   1},	{BASE_WIDTH*2,   BASE_HEIGHT*2},	true}},
	{CM_ZOOM_250,			{ZOOM_RATE,	{  5,   2},	{BASE_WIDTH*5/2, BASE_HEIGHT*5/2},	f1Seg}},
	{CM_ZOOM_300,			{ZOOM_RATE,	{  3,   1},	{BASE_WIDTH*3,   BASE_HEIGHT*3},	f1Seg}},
	{CM_CUSTOMZOOM_FIRST+0,	{ZOOM_RATE,	{100, 100},	{BASE_WIDTH,     BASE_HEIGHT},		false}},
	{CM_CUSTOMZOOM_FIRST+1,	{ZOOM_RATE,	{100, 100},	{BASE_WIDTH,     BASE_HEIGHT},		false}},
	{CM_CUSTOMZOOM_FIRST+2,	{ZOOM_RATE,	{100, 100},	{BASE_WIDTH,     BASE_HEIGHT},		false}},
	{CM_CUSTOMZOOM_FIRST+3,	{ZOOM_RATE,	{100, 100},	{BASE_WIDTH,     BASE_HEIGHT},		false}},
	{CM_CUSTOMZOOM_FIRST+4,	{ZOOM_RATE,	{100, 100},	{BASE_WIDTH,     BASE_HEIGHT},		false}},
	{CM_CUSTOMZOOM_FIRST+5,	{ZOOM_RATE,	{100, 100},	{BASE_WIDTH,     BASE_HEIGHT},		false}},
	{CM_CUSTOMZOOM_FIRST+6,	{ZOOM_RATE,	{100, 100},	{BASE_WIDTH,     BASE_HEIGHT},		false}},
	{CM_CUSTOMZOOM_FIRST+7,	{ZOOM_RATE,	{100, 100},	{BASE_WIDTH,     BASE_HEIGHT},		false}},
	{CM_CUSTOMZOOM_FIRST+8,	{ZOOM_RATE,	{100, 100},	{BASE_WIDTH,     BASE_HEIGHT},		false}},
	{CM_CUSTOMZOOM_FIRST+9,	{ZOOM_RATE,	{100, 100},	{BASE_WIDTH,     BASE_HEIGHT},		false}},
};


CZoomOptions::CZoomOptions()
	: CCommandCustomizer(CM_CUSTOMZOOM_FIRST,CM_CUSTOMZOOM_LAST)
{
	for (int i=0;i<NUM_ZOOM_COMMANDS;i++) {
		m_ZoomList[i]=m_DefaultZoomList[i].Info;
		m_Order[i]=i;
	}
}


CZoomOptions::~CZoomOptions()
{
}


bool CZoomOptions::Show(HWND hwndOwner)
{
	return ShowDialog(hwndOwner,
					  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_ZOOMOPTIONS))==IDOK;
}


bool CZoomOptions::ReadSettings(CSettings &Settings)
{
	TCHAR szText[256];

	int j=CM_ZOOM_LAST-CM_ZOOM_FIRST+1;
	for (int i=0;i<=CM_CUSTOMZOOM_LAST-CM_CUSTOMZOOM_FIRST;i++,j++) {
		int Type,Rate,Width,Height;

		::wsprintf(szText,TEXT("CustomZoom%d"),i+1);
		if (Settings.Read(szText,&Rate) && Rate>0 && Rate<=MAX_RATE)
			m_ZoomList[j].Rate.Rate=Rate;
		::wsprintf(szText,TEXT("CustomZoom%d_Type"),i+1);
		if (Settings.Read(szText,&Type) && Type>=ZOOM_RATE && Type<=ZOOM_SIZE)
			m_ZoomList[j].Type=(ZoomType)Type;
		::wsprintf(szText,TEXT("CustomZoom%d_Width"),i+1);
		if (Settings.Read(szText,&Width) && Width>0)
			m_ZoomList[j].Size.Width=Width;
		::wsprintf(szText,TEXT("CustomZoom%d_Height"),i+1);
		if (Settings.Read(szText,&Height) && Height>0)
			m_ZoomList[j].Size.Height=Height;
	}

	int ListCount;
	if (Settings.Read(TEXT("ZoomListCount"),&ListCount) && ListCount>0) {
		const CCommandList *pCommandList=GetAppClass().GetCommandList();

		if (ListCount>NUM_ZOOM_COMMANDS)
			ListCount=NUM_ZOOM_COMMANDS;
		int Count=0;
		for (int i=0;i<ListCount;i++) {
			TCHAR szName[32];

			::wsprintf(szName,TEXT("ZoomList%d"),i);
			if (Settings.Read(szName,szText,lengthof(szText))) {
				LPTSTR p=szText;
				while (*p!=_T('\0') && *p!=_T(','))
					p++;
				if (*p==_T(','))
					*p++=_T('\0');
				int Command=pCommandList->ParseText(szText);
				if (Command!=0) {
					j=GetIndexByCommand(Command);
					if (j>=0) {
						int k;
						for (k=0;k<Count;k++) {
							if (m_Order[k]==j)
								break;
						}
						if (k==Count) {
							m_Order[Count]=j;
							m_ZoomList[j].fVisible=*p!=_T('0');
							Count++;
						}
					}
				}
			}
		}
		if (Count<NUM_ZOOM_COMMANDS) {
			for (int i=0;i<NUM_ZOOM_COMMANDS;i++) {
				for (j=0;j<Count;j++) {
					if (m_Order[j]==i)
						break;
				}
				if (j==Count)
					m_Order[Count++]=i;
			}
		}
	}

	return true;
}


bool CZoomOptions::WriteSettings(CSettings &Settings)
{
	TCHAR szText[256];

	int j=CM_ZOOM_LAST-CM_ZOOM_FIRST+1;
	for (int i=0;i<=CM_CUSTOMZOOM_LAST-CM_CUSTOMZOOM_FIRST;i++,j++) {
		::wsprintf(szText,TEXT("CustomZoom%d"),i+1);
		Settings.Write(szText,m_ZoomList[j].Rate.Rate);
		::wsprintf(szText,TEXT("CustomZoom%d_Type"),i+1);
		Settings.Write(szText,(int)m_ZoomList[j].Type);
		::wsprintf(szText,TEXT("CustomZoom%d_Width"),i+1);
		Settings.Write(szText,m_ZoomList[j].Size.Width);
		::wsprintf(szText,TEXT("CustomZoom%d_Height"),i+1);
		Settings.Write(szText,m_ZoomList[j].Size.Height);
	}

	Settings.Write(TEXT("ZoomListCount"),NUM_ZOOM_COMMANDS);
	const CCommandList *pCommandList=GetAppClass().GetCommandList();
	for (int i=0;i<NUM_ZOOM_COMMANDS;i++) {
		const int Index=m_Order[i];
		const ZoomInfo &Info=m_ZoomList[Index];
		TCHAR szName[32];

		::wsprintf(szName,TEXT("ZoomList%d"),i);
		StdUtil::snprintf(szText,lengthof(szText),TEXT("%s,%d"),
						  pCommandList->GetCommandTextByID(m_DefaultZoomList[Index].Command),
						  Info.fVisible?1:0);
		Settings.Write(szName,szText);
	}

	return true;
}


bool CZoomOptions::SetMenu(HMENU hmenu,const ZoomInfo *pCurZoom) const
{
	for (int i=::GetMenuItemCount(hmenu)-3;i>=0;i--)
		::DeleteMenu(hmenu,i,MF_BYPOSITION);

	int Pos=0;
	bool fRateCheck=false,fSizeCheck=false;
	for (int i=0;i<NUM_ZOOM_COMMANDS;i++) {
		const ZoomInfo &Info=m_ZoomList[m_Order[i]];

		if (Info.fVisible) {
			TCHAR szText[MAX_ZOOM_TEXT];
			UINT Flags=MF_BYPOSITION | MF_STRING | MF_ENABLED;

			if (Info.Type==ZOOM_RATE) {
				StdUtil::snprintf(szText,lengthof(szText),TEXT("%d%%"),
								  Info.Rate.GetPercentage());
				if (!fRateCheck
						&& pCurZoom!=NULL
						&& pCurZoom->Rate.GetPercentage()==Info.Rate.GetPercentage()) {
					Flags|=MF_CHECKED;
					fRateCheck=true;
				}
			} else {
				StdUtil::snprintf(szText,lengthof(szText),TEXT("%d x %d"),
								  Info.Size.Width,Info.Size.Height);
				if (!fSizeCheck
						&& pCurZoom!=NULL
						&& pCurZoom->Size.Width==Info.Size.Width
						&& pCurZoom->Size.Height==Info.Size.Height) {
					Flags|=MF_CHECKED;
					fSizeCheck=true;
				}
			}
			::InsertMenu(hmenu,Pos++,Flags,m_DefaultZoomList[m_Order[i]].Command,szText);
		}
	}

	return true;
}


bool CZoomOptions::GetZoomInfoByCommand(int Command,ZoomInfo *pInfo) const
{
	const int i=GetIndexByCommand(Command);

	if (i<0)
		return false;
	*pInfo=m_ZoomList[i];
	return true;
}


int CZoomOptions::GetIndexByCommand(int Command) const
{
	for (int i=0;i<NUM_ZOOM_COMMANDS;i++) {
		if (m_DefaultZoomList[i].Command==Command)
			return i;
	}
	return -1;
}


void CZoomOptions::FormatCommandText(int Command,const ZoomInfo &Info,LPTSTR pszText,int MaxLength) const
{
	int Length=::LoadString(GetAppClass().GetResourceInstance(),Command,pszText,MaxLength);
	if (Command>=CM_CUSTOMZOOM_FIRST && Command<=CM_CUSTOMZOOM_LAST) {
		if (Info.Type==ZOOM_RATE)
			StdUtil::snprintf(pszText+Length,MaxLength-Length,TEXT(" : %d%%"),Info.Rate.GetPercentage());
		else if (Info.Type==ZOOM_SIZE)
			StdUtil::snprintf(pszText+Length,MaxLength-Length,TEXT(" : %d x %d"),Info.Size.Width,Info.Size.Height);
	}
}


void CZoomOptions::SetItemState(HWND hDlg)
{
	HWND hwndList=::GetDlgItem(hDlg,IDC_ZOOMOPTIONS_LIST);
	int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

	if (Sel>=0) {
		const int Index=GetItemIndex(hwndList,Sel);
		const ZoomInfo &Info=m_ZoomSettingList[Index];
		const bool fCustom=m_DefaultZoomList[Index].Command>=CM_CUSTOMZOOM_FIRST
						&& m_DefaultZoomList[Index].Command<=CM_CUSTOMZOOM_LAST;
		::CheckRadioButton(hDlg,IDC_ZOOMOPTIONS_TYPE_RATE,IDC_ZOOMOPTIONS_TYPE_SIZE,
						   IDC_ZOOMOPTIONS_TYPE_RATE+Info.Type);
		::EnableDlgItems(hDlg,IDC_ZOOMOPTIONS_TYPE_RATE,IDC_ZOOMOPTIONS_TYPE_SIZE,fCustom);
		::SetDlgItemInt(hDlg,IDC_ZOOMOPTIONS_RATE,Info.Rate.GetPercentage(),TRUE);
		::SetDlgItemInt(hDlg,IDC_ZOOMOPTIONS_WIDTH,Info.Size.Width,TRUE);
		::SetDlgItemInt(hDlg,IDC_ZOOMOPTIONS_HEIGHT,Info.Size.Height,TRUE);
		::EnableDlgItems(hDlg,IDC_ZOOMOPTIONS_RATE_LABEL,IDC_ZOOMOPTIONS_RATE_UNIT,Info.Type==ZOOM_RATE);
		::EnableDlgItem(hDlg,IDC_ZOOMOPTIONS_RATE,Info.Type==ZOOM_RATE && fCustom);
		::EnableDlgItems(hDlg,IDC_ZOOMOPTIONS_WIDTH_LABEL,IDC_ZOOMOPTIONS_GETCURSIZE,Info.Type==ZOOM_SIZE);
		::EnableDlgItem(hDlg,IDC_ZOOMOPTIONS_WIDTH,Info.Type==ZOOM_SIZE && fCustom);
		::EnableDlgItem(hDlg,IDC_ZOOMOPTIONS_HEIGHT,Info.Type==ZOOM_SIZE && fCustom);
		::EnableDlgItem(hDlg,IDC_ZOOMOPTIONS_GETCURSIZE,Info.Type==ZOOM_SIZE && fCustom);
		::EnableDlgItem(hDlg,IDC_ZOOMOPTIONS_UP,Sel>0);
		::EnableDlgItem(hDlg,IDC_ZOOMOPTIONS_DOWN,Sel+1<NUM_ZOOM_COMMANDS);
	} else {
		::SetDlgItemText(hDlg,IDC_ZOOMOPTIONS_RATE,TEXT(""));
		::SetDlgItemText(hDlg,IDC_ZOOMOPTIONS_WIDTH,TEXT(""));
		::SetDlgItemText(hDlg,IDC_ZOOMOPTIONS_HEIGHT,TEXT(""));
		::EnableDlgItems(hDlg,IDC_ZOOMOPTIONS_TYPE_RATE,IDC_ZOOMOPTIONS_DOWN,FALSE);
	}
}


int CZoomOptions::GetItemIndex(HWND hwndList,int Item)
{
	LVITEM lvi;

	lvi.mask=LVIF_PARAM;
	lvi.iItem=Item;
	lvi.iSubItem=0;
	ListView_GetItem(hwndList,&lvi);
	return (int)lvi.lParam;
}


void CZoomOptions::UpdateItemText(HWND hDlg,int Item)
{
	HWND hwndList=::GetDlgItem(hDlg,IDC_ZOOMOPTIONS_LIST);
	const int Index=GetItemIndex(hwndList,Item);
	LVITEM lvi;
	TCHAR szText[MAX_ZOOM_TEXT];

	FormatCommandText(m_DefaultZoomList[Index].Command,m_ZoomSettingList[Index],
					  szText,lengthof(szText));
	lvi.mask=LVIF_TEXT;
	lvi.iItem=Item;
	lvi.iSubItem=0;
	lvi.pszText=szText;
	ListView_SetItem(hwndList,&lvi);
}


INT_PTR CZoomOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			m_fChanging=true;
			HWND hwndList=::GetDlgItem(hDlg,IDC_ZOOMOPTIONS_LIST);
			ListView_SetExtendedListViewStyle(hwndList,
											  LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
			RECT rc;
			GetClientRect(hwndList,&rc);
			rc.right-=GetSystemMetrics(SM_CXHSCROLL);
			LVCOLUMN lvc;
			lvc.mask=LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
			lvc.fmt=LVCFMT_LEFT;
			lvc.cx=rc.right;
			lvc.pszText=TEXT("");
			lvc.iSubItem=0;
			ListView_InsertColumn(hwndList,0,&lvc);
			LVITEM lvi;
			TCHAR szText[MAX_ZOOM_TEXT];
			lvi.mask=LVIF_TEXT | LVIF_PARAM;
			lvi.iSubItem=0;
			lvi.pszText=szText;
			for (int i=0;i<NUM_ZOOM_COMMANDS;i++) {
				const int Index=m_Order[i];
				const ZoomInfo &Info=m_ZoomList[Index];
				m_ZoomSettingList[Index]=Info;
				lvi.iItem=i;
				lvi.lParam=Index;
				FormatCommandText(m_DefaultZoomList[Index].Command,Info,szText,lengthof(szText));
				ListView_InsertItem(hwndList,&lvi);
				ListView_SetCheckState(hwndList,i,Info.fVisible);
			}
			SetItemState(hDlg);
			m_fChanging=false;
			AdjustDialogPos(::GetParent(hDlg),hDlg);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ZOOMOPTIONS_TYPE_RATE:
		case IDC_ZOOMOPTIONS_TYPE_SIZE:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_ZOOMOPTIONS_LIST);
				const int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

				if (Sel>=0) {
					const int Index=GetItemIndex(hwndList,Sel);
					const int Command=m_DefaultZoomList[Index].Command;
					if (Command>=CM_CUSTOMZOOM_FIRST && Command<=CM_CUSTOMZOOM_LAST) {
						ZoomType Type=::IsDlgButtonChecked(hDlg,IDC_ZOOMOPTIONS_TYPE_RATE)?ZOOM_RATE:ZOOM_SIZE;
						ZoomInfo &Info=m_ZoomSettingList[Index];
						if (Type!=Info.Type) {
							Info.Type=Type;
							m_fChanging=true;
							UpdateItemText(hDlg,Sel);
							SetItemState(hDlg);
							m_fChanging=false;
						}
					}
				}
			}
			return TRUE;

		case IDC_ZOOMOPTIONS_RATE:
			if (HIWORD(wParam)==EN_CHANGE && !m_fChanging) {
				int Rate=::GetDlgItemInt(hDlg,IDC_ZOOMOPTIONS_RATE,NULL,TRUE);

				if (Rate>0 && Rate<=MAX_RATE) {
					HWND hwndList=::GetDlgItem(hDlg,IDC_ZOOMOPTIONS_LIST);
					const int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

					if (Sel>=0) {
						const int Index=GetItemIndex(hwndList,Sel);
						const int Command=m_DefaultZoomList[Index].Command;
						if (Command>=CM_CUSTOMZOOM_FIRST && Command<=CM_CUSTOMZOOM_LAST) {
							ZoomInfo &Info=m_ZoomSettingList[Index];
							Info.Rate.Rate=Rate;
							Info.Rate.Factor=100;
							m_fChanging=true;
							UpdateItemText(hDlg,Sel);
							m_fChanging=false;
						}
					}
				}
			}
			return TRUE;

		case IDC_ZOOMOPTIONS_WIDTH:
		case IDC_ZOOMOPTIONS_HEIGHT:
			if (HIWORD(wParam)==EN_CHANGE && !m_fChanging) {
				int Width=::GetDlgItemInt(hDlg,IDC_ZOOMOPTIONS_WIDTH,NULL,TRUE);
				int Height=::GetDlgItemInt(hDlg,IDC_ZOOMOPTIONS_HEIGHT,NULL,TRUE);

				if (Width>0 && Height>0) {
					HWND hwndList=::GetDlgItem(hDlg,IDC_ZOOMOPTIONS_LIST);
					const int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

					if (Sel>=0) {
						const int Index=GetItemIndex(hwndList,Sel);
						const int Command=m_DefaultZoomList[Index].Command;
						if (Command>=CM_CUSTOMZOOM_FIRST && Command<=CM_CUSTOMZOOM_LAST) {
							ZoomInfo &Info=m_ZoomSettingList[Index];
							Info.Size.Width=Width;
							Info.Size.Height=Height;
							m_fChanging=true;
							UpdateItemText(hDlg,Sel);
							m_fChanging=false;
						}
					}
				}
			}
			return TRUE;

		case IDC_ZOOMOPTIONS_GETCURSIZE:
			{
				HWND hwndViewer=GetAppClass().GetUICore()->GetViewerWindow();

				if (hwndViewer!=NULL) {
					RECT rc;

					::GetClientRect(hwndViewer,&rc);
					::SetDlgItemInt(hDlg,IDC_ZOOMOPTIONS_WIDTH,rc.right-rc.left,TRUE);
					::SetDlgItemInt(hDlg,IDC_ZOOMOPTIONS_HEIGHT,rc.bottom-rc.top,TRUE);
				}
			}
			return TRUE;

		case IDC_ZOOMOPTIONS_UP:
		case IDC_ZOOMOPTIONS_DOWN:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_ZOOMOPTIONS_LIST);
				int From=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED),To;

				if (From>=0) {
					if (LOWORD(wParam)==IDC_ZOOMOPTIONS_UP) {
						if (From<1)
							break;
						To=From-1;
					} else {
						if (From+1>=NUM_ZOOM_COMMANDS)
							break;
						To=From+1;
					}
					m_fChanging=true;
					LVITEM lvi;
					TCHAR szText[MAX_ZOOM_TEXT];
					lvi.mask=LVIF_STATE | LVIF_TEXT | LVIF_PARAM;
					lvi.iItem=From;
					lvi.iSubItem=0;
					lvi.stateMask=~0U;
					lvi.pszText=szText;
					lvi.cchTextMax=lengthof(szText);
					ListView_GetItem(hwndList,&lvi);
					BOOL fChecked=ListView_GetCheckState(hwndList,From);
					ListView_DeleteItem(hwndList,From);
					lvi.iItem=To;
					ListView_InsertItem(hwndList,&lvi);
					ListView_SetCheckState(hwndList,To,fChecked);
					SetItemState(hDlg);
					m_fChanging=false;
				}
			}
			return TRUE;

		case IDOK:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_ZOOMOPTIONS_LIST);

				for (int i=0;i<NUM_ZOOM_COMMANDS;i++) {
					int Index=GetItemIndex(hwndList,i);
					m_Order[i]=Index;
					if (m_DefaultZoomList[Index].Command>=CM_CUSTOMZOOM_FIRST
							&& m_DefaultZoomList[Index].Command<=CM_CUSTOMZOOM_LAST) {
						m_ZoomList[Index]=m_ZoomSettingList[Index];
					}
					m_ZoomList[Index].fVisible=ListView_GetCheckState(hwndList,i)!=FALSE;
				}
			}
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case LVN_ITEMCHANGED:
			if (!m_fChanging)
				SetItemState(hDlg);
			return TRUE;
		}
		break;
	}

	return FALSE;
}


bool CZoomOptions::GetCommandName(int Command,LPTSTR pszName,int MaxLength)
{
	ZoomInfo Info;
	if (!GetZoomInfoByCommand(Command,&Info))
		return false;
	FormatCommandText(Command,Info,pszName,MaxLength);
	return true;
}
