#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "StatusOptions.h"
#include "Settings.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


static const bool IS_HD=
#ifndef TVH264_FOR_1SEG
	true
#else
	false
#endif
	;

#define ITEM_MARGIN	2
#define CHECK_WIDTH	14

#define TIMER_ID_UP		1
#define TIMER_ID_DOWN	2




CStatusOptions::CStatusOptions(CStatusView *pStatusView)
	: COptions(TEXT("Status"))
	, m_pStatusView(pStatusView)
	, m_fShowTOTTime(false)
	, m_fEnablePopupProgramInfo(true)
	, m_fMultiRow(!IS_HD)
	, m_MaxRows(2)
{
	SetDefaultItemList();
	m_pStatusView->GetFont(&m_lfItemFont);
}


CStatusOptions::~CStatusOptions()
{
	Destroy();
}


bool CStatusOptions::ReadSettings(CSettings &Settings)
{
	int NumItems;
	if (Settings.Read(TEXT("NumItems"),&NumItems)
			&& NumItems>0 && NumItems<=NUM_STATUS_ITEMS) {
		StatusItemInfo ItemList[NUM_STATUS_ITEMS];
		int i,j,k;

		for (i=j=0;i<NumItems;i++) {
			TCHAR szKey[32];

			::wsprintf(szKey,TEXT("Item%d_ID"),i);
			if (Settings.Read(szKey,&ItemList[j].ID)
					&& ItemList[j].ID>=STATUS_ITEM_FIRST
					&& ItemList[j].ID<=STATUS_ITEM_LAST) {
				for (k=0;k<j;k++) {
					if (ItemList[k].ID==ItemList[j].ID)
						break;
				}
				if (k==j) {
					::wsprintf(szKey,TEXT("Item%d_Visible"),i);
					Settings.Read(szKey,&ItemList[j].fVisible);
					::wsprintf(szKey,TEXT("Item%d_Width"),i);
					if (!Settings.Read(szKey,&ItemList[j].Width) || ItemList[j].Width<1)
						ItemList[j].Width=-1;
					j++;
				}
			}
		}
		NumItems=j;
		if (NumItems<NUM_STATUS_ITEMS) {
			for (i=0;i<NUM_STATUS_ITEMS;i++) {
				for (k=0;k<NumItems;k++) {
					if (ItemList[k].ID==m_ItemList[i].ID)
						break;
				}
				if (k==NumItems)
					m_ItemList[j++]=m_ItemList[i];
			}
		}
		for (i=0;i<NumItems;i++)
			m_ItemList[i]=ItemList[i];
	}

	// Font
	TCHAR szFont[LF_FACESIZE];
	int Value;
	if (Settings.Read(TEXT("FontName"),szFont,LF_FACESIZE) && szFont[0]!='\0') {
		lstrcpy(m_lfItemFont.lfFaceName,szFont);
		m_lfItemFont.lfEscapement=0;
		m_lfItemFont.lfOrientation=0;
		m_lfItemFont.lfUnderline=0;
		m_lfItemFont.lfStrikeOut=0;
		m_lfItemFont.lfCharSet=DEFAULT_CHARSET;
		m_lfItemFont.lfOutPrecision=OUT_DEFAULT_PRECIS;
		m_lfItemFont.lfClipPrecision=CLIP_DEFAULT_PRECIS;
		m_lfItemFont.lfQuality=DRAFT_QUALITY;
		m_lfItemFont.lfPitchAndFamily=DEFAULT_PITCH | FF_DONTCARE;
	}
	if (Settings.Read(TEXT("FontSize"),&Value)) {
		m_lfItemFont.lfHeight=Value;
		m_lfItemFont.lfWidth=0;
	}
	if (Settings.Read(TEXT("FontWeight"),&Value))
		m_lfItemFont.lfWeight=Value;
	if (Settings.Read(TEXT("FontItalic"),&Value))
		m_lfItemFont.lfItalic=Value;

	Settings.Read(TEXT("MultiRow"),&m_fMultiRow);
	Settings.Read(TEXT("MaxRows"),&m_MaxRows);

	Settings.Read(TEXT("TOTTime"),&m_fShowTOTTime);
	Settings.Read(TEXT("PopupProgramInfo"),&m_fEnablePopupProgramInfo);

	return true;
}


bool CStatusOptions::WriteSettings(CSettings &Settings)
{
	if (Settings.Write(TEXT("NumItems"),NUM_STATUS_ITEMS)) {
		for (int i=0;i<NUM_STATUS_ITEMS;i++) {
			TCHAR szKey[32];

			::wsprintf(szKey,TEXT("Item%d_ID"),i);
			Settings.Write(szKey,m_ItemList[i].ID);
			::wsprintf(szKey,TEXT("Item%d_Visible"),i);
			Settings.Write(szKey,m_ItemList[i].fVisible);
			::wsprintf(szKey,TEXT("Item%d_Width"),i);
			Settings.Write(szKey,m_ItemList[i].Width);
		}
	}

	// Font
	Settings.Write(TEXT("FontName"),m_lfItemFont.lfFaceName);
	Settings.Write(TEXT("FontSize"),(int)m_lfItemFont.lfHeight);
	Settings.Write(TEXT("FontWeight"),(int)m_lfItemFont.lfWeight);
	Settings.Write(TEXT("FontItalic"),(int)m_lfItemFont.lfItalic);

	Settings.Write(TEXT("MultiRow"),m_fMultiRow);
	Settings.Write(TEXT("MaxRows"),m_MaxRows);

	Settings.Write(TEXT("TOTTime"),m_fShowTOTTime);
	Settings.Write(TEXT("PopupProgramInfo"),m_fEnablePopupProgramInfo);

	return true;
}


bool CStatusOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_STATUS));
}


void CStatusOptions::SetDefaultItemList()
{
	static const struct {
		BYTE ID;
		bool fVisible;
	} DefaultItemList[NUM_STATUS_ITEMS] = {
		{STATUS_ITEM_TUNER,			IS_HD},
		{STATUS_ITEM_CHANNEL,		true},
		{STATUS_ITEM_FAVORITES,		false},
		{STATUS_ITEM_VIDEOSIZE,		true},
		{STATUS_ITEM_VOLUME,		true},
		{STATUS_ITEM_AUDIOCHANNEL,	true},
		{STATUS_ITEM_RECORD,		true},
		{STATUS_ITEM_CAPTURE,		IS_HD},
		{STATUS_ITEM_ERROR,			IS_HD},
		{STATUS_ITEM_SIGNALLEVEL,	true},
		{STATUS_ITEM_CLOCK,			false},
		{STATUS_ITEM_PROGRAMINFO,	false},
		{STATUS_ITEM_BUFFERING,		false},
		{STATUS_ITEM_MEDIABITRATE,	false},
	};

	for (int i=0;i<NUM_STATUS_ITEMS;i++) {
		m_ItemList[i].ID=DefaultItemList[i].ID;
		m_ItemList[i].fVisible=DefaultItemList[i].fVisible;
		m_ItemList[i].Width=-1;
	}
}


void CStatusOptions::InitListBox(HWND hDlg)
{
	int i;

	for (i=0;i<NUM_STATUS_ITEMS;i++) {
		m_ItemListCur[i]=m_ItemList[i];
		DlgListBox_AddString(hDlg,IDC_STATUSOPTIONS_ITEMLIST,&m_ItemListCur[i]);
	}
}


static void SetFontInfo(HWND hDlg,const LOGFONT *plf)
{
	HDC hdc;
	TCHAR szText[LF_FACESIZE+16];

	hdc=GetDC(hDlg);
	wsprintf(szText,TEXT("%s, %d pt"),
			 plf->lfFaceName,CalcFontPointHeight(hdc,plf));
	ReleaseDC(hDlg,hdc);
	SetDlgItemText(hDlg,IDC_STATUSOPTIONS_FONTINFO,szText);
}


INT_PTR CStatusOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			m_DropInsertPos=-1;
			m_DragTimerID=0;
			InitListBox(hDlg);
			m_pOldListProc=SubclassWindow(GetDlgItem(hDlg,IDC_STATUSOPTIONS_ITEMLIST),ItemListProc);
			m_ItemHeight=m_pStatusView->GetItemHeight()+(1+ITEM_MARGIN)*2;
			DlgListBox_SetItemHeight(hDlg,IDC_STATUSOPTIONS_ITEMLIST,0,m_ItemHeight);
			CalcTextWidth(hDlg);
			SetListHExtent(hDlg);
			m_CurSettingFont=m_lfItemFont;
			SetFontInfo(hDlg,&m_CurSettingFont);
			DlgCheckBox_Check(hDlg,IDC_STATUSOPTIONS_MULTIROW,m_fMultiRow);
			::SetDlgItemInt(hDlg,IDC_STATUSOPTIONS_MAXROWS,m_MaxRows,TRUE);
			DlgUpDown_SetRange(hDlg,IDC_STATUSOPTIONS_MAXROWS_UPDOWN,1,UD_MAXVAL);
			EnableDlgItems(hDlg,
						   IDC_STATUSOPTIONS_MAXROWS_LABEL,
						   IDC_STATUSOPTIONS_MAXROWS_UPDOWN,
						   m_fMultiRow);
		}
		return TRUE;

	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT pdis=reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

			switch (pdis->itemAction) {
			case ODA_DRAWENTIRE:
			case ODA_SELECT:
				if ((INT)pdis->itemID>=0) {
					StatusItemInfo *pItemInfo=reinterpret_cast<StatusItemInfo*>(pdis->itemData);
					CStatusItem *pItem=m_pStatusView->GetItemByID(pItemInfo->ID);
					int TextColor,BackColor;
					int OldBkMode;
					COLORREF crTextColor,crOldTextColor;
					RECT rc;

					if ((pdis->itemState&ODS_SELECTED)==0) {
						TextColor=COLOR_WINDOWTEXT;
						BackColor=COLOR_WINDOW;
					} else {
						TextColor=COLOR_HIGHLIGHTTEXT;
						BackColor=COLOR_HIGHLIGHT;
					}
					::FillRect(pdis->hDC,&pdis->rcItem,
							   reinterpret_cast<HBRUSH>(BackColor+1));
					OldBkMode=::SetBkMode(pdis->hDC,TRANSPARENT);
					crTextColor=::GetSysColor(TextColor);
					if (!pItemInfo->fVisible)
						crTextColor=MixColor(crTextColor,::GetSysColor(BackColor));
					crOldTextColor=::SetTextColor(pdis->hDC,crTextColor);
					rc.left=pdis->rcItem.left+ITEM_MARGIN;
					rc.top=pdis->rcItem.top+ITEM_MARGIN;
					rc.right=rc.left+CHECK_WIDTH;
					rc.bottom=pdis->rcItem.bottom-ITEM_MARGIN;
					::DrawFrameControl(pdis->hDC,&rc,DFC_BUTTON,
						DFCS_BUTTONCHECK | (pItemInfo->fVisible?DFCS_CHECKED:0));
					rc.left=pdis->rcItem.left+ITEM_MARGIN+CHECK_WIDTH+ITEM_MARGIN;
					rc.top=pdis->rcItem.top+ITEM_MARGIN;
					rc.right=rc.left+m_TextWidth;
					rc.bottom=pdis->rcItem.bottom-ITEM_MARGIN;
					::DrawText(pdis->hDC,pItem->GetName(),-1,&rc,
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
					rc.left=rc.right+ITEM_MARGIN+1;
					rc.right=rc.left+(pItemInfo->Width>=0?
									pItemInfo->Width:pItem->GetDefaultWidth());
					rc.top=pdis->rcItem.top+((pdis->rcItem.bottom-pdis->rcItem.top)-m_pStatusView->GetItemHeight())/2;
					rc.bottom=rc.top+m_pStatusView->GetItemHeight();
					HPEN hpen,hpenOld;
					HBRUSH hbrOld;
					hpen=::CreatePen(PS_SOLID,1,crTextColor);
					hpenOld=SelectPen(pdis->hDC,hpen);
					hbrOld=SelectBrush(pdis->hDC,static_cast<HBRUSH>(GetStockObject(NULL_BRUSH)));
					::Rectangle(pdis->hDC,rc.left-1,rc.top-1,rc.right+1,rc.bottom+1);
					SelectBrush(pdis->hDC,hbrOld);
					SelectPen(pdis->hDC,hpenOld);
					::DeleteObject(hpen);
					HFONT hfont=::CreateFontIndirect(&m_CurSettingFont);
					m_pStatusView->DrawItemPreview(pItem,pdis->hDC,&rc,false,hfont);
					::DeleteObject(hfont);
					::SetBkMode(pdis->hDC,OldBkMode);
					::SetTextColor(pdis->hDC,crOldTextColor);
					if ((int)pdis->itemID==m_DropInsertPos
								|| (int)pdis->itemID+1==m_DropInsertPos)
						::PatBlt(pdis->hDC,pdis->rcItem.left,
							(int)pdis->itemID==m_DropInsertPos?
										pdis->rcItem.top:pdis->rcItem.bottom-1,
							pdis->rcItem.right-pdis->rcItem.left,1,DSTINVERT);
					if ((pdis->itemState&ODS_FOCUS)==0)
						break;
				}
			case ODA_FOCUS:
				if ((pdis->itemState & ODS_NOFOCUSRECT)==0)
					::DrawFocusRect(pdis->hDC,&pdis->rcItem);
				break;
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_STATUSOPTIONS_DEFAULT:
			SetDefaultItemList();
			DlgListBox_Clear(hDlg,IDC_STATUSOPTIONS_ITEMLIST);
			InitListBox(hDlg);
			SetListHExtent(hDlg);
			return TRUE;

		case IDC_STATUSOPTIONS_CHOOSEFONT:
			{
				LOGFONT lf=m_CurSettingFont;

				if (ChooseFontDialog(hDlg,&lf)
						&& !CompareLogFont(&m_CurSettingFont,&lf)) {
					DrawUtil::CFont Font(lf);
					RECT rc;

					m_CurSettingFont=lf;
					SetFontInfo(hDlg,&lf);
					m_pStatusView->GetItemMargin(&rc);
					HWND hwndList=::GetDlgItem(hDlg,IDC_STATUSOPTIONS_ITEMLIST);
					HDC hdc=::GetDC(hwndList);
					m_ItemHeight=(Font.GetHeight(hdc,false)+rc.top+rc.bottom)+(1+ITEM_MARGIN)*2;
					::ReleaseDC(hwndList,hdc);
					DlgListBox_SetItemHeight(hDlg,IDC_STATUSOPTIONS_ITEMLIST,0,m_ItemHeight);
					::InvalidateRect(hwndList,NULL,TRUE);
				}
			}
			return TRUE;

		case IDC_STATUSOPTIONS_MULTIROW:
			EnableDlgItemsSyncCheckBox(hDlg,
									   IDC_STATUSOPTIONS_MAXROWS_LABEL,
									   IDC_STATUSOPTIONS_MAXROWS_UPDOWN,
									   IDC_STATUSOPTIONS_MULTIROW);
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				m_pStatusView->EnableSizeAdjustment(false);

				for (int i=0;i<NUM_STATUS_ITEMS;i++)
					m_ItemList[i]=*reinterpret_cast<StatusItemInfo*>(
						DlgListBox_GetItemData(hDlg,IDC_STATUSOPTIONS_ITEMLIST,i));
				ApplyItemList();

				if (!CompareLogFont(&m_lfItemFont,&m_CurSettingFont)) {
					m_lfItemFont=m_CurSettingFont;
					m_pStatusView->SetFont(&m_lfItemFont);
				}

				bool fMultiRow=DlgCheckBox_IsChecked(hDlg,IDC_STATUSOPTIONS_MULTIROW);
				if (m_fMultiRow!=fMultiRow) {
					m_fMultiRow=fMultiRow;
					m_pStatusView->SetMultiRow(fMultiRow);
				}
				int MaxRows=::GetDlgItemInt(hDlg,IDC_STATUSOPTIONS_MAXROWS,NULL,TRUE);
				if (MaxRows<1)
					MaxRows=1;
				if (m_MaxRows!=MaxRows) {
					m_MaxRows=MaxRows;
					m_pStatusView->SetMaxRows(MaxRows);
				}

				m_pStatusView->EnableSizeAdjustment(true);

				m_fChanged=true;
			}
			break;
		}
		break;

	case WM_DESTROY:
		SubclassWindow(GetDlgItem(hDlg,IDC_STATUSOPTIONS_ITEMLIST),m_pOldListProc);
		return TRUE;
	}

	return FALSE;
}


void CStatusOptions::CalcTextWidth(HWND hDlg)
{
	HWND hwndList;
	HDC hdc;
	HFONT hfontOld;
	int Count,i;
	int MaxWidth;
	CStatusItem *pItem;
	SIZE sz;

	hwndList=GetDlgItem(hDlg,IDC_STATUSOPTIONS_ITEMLIST);
	hdc=GetDC(hwndList);
	if (hdc==NULL)
		return;
	hfontOld=SelectFont(hdc,GetWindowFont(hwndList));
	Count=ListBox_GetCount(hwndList);
	MaxWidth=0;
	for (i=0;i<Count;i++) {
		pItem=m_pStatusView->GetItemByID(m_ItemList[i].ID);
		GetTextExtentPoint32(hdc,pItem->GetName(),lstrlen(pItem->GetName()),&sz);
		if (sz.cx>MaxWidth)
			MaxWidth=sz.cx;
	}
	SelectFont(hdc,hfontOld);
	ReleaseDC(hwndList,hdc);
	m_TextWidth=MaxWidth;
}


void CStatusOptions::SetListHExtent(HWND hDlg)
{
	HWND hwndList;
	int MaxWidth,Width;
	int i;

	hwndList=GetDlgItem(hDlg,IDC_STATUSOPTIONS_ITEMLIST);
	MaxWidth=0;
	for (i=0;i<NUM_STATUS_ITEMS;i++) {
		Width=m_ItemListCur[i].Width>=0?m_ItemListCur[i].Width:
			m_pStatusView->GetItem(m_pStatusView->IDToIndex(m_ItemList[i].ID))->GetWidth();
		if (Width>MaxWidth)
			MaxWidth=Width;
	}
	ListBox_SetHorizontalExtent(hwndList,
		ITEM_MARGIN+CHECK_WIDTH+ITEM_MARGIN+m_TextWidth+ITEM_MARGIN+MaxWidth+2+ITEM_MARGIN);
}


static int ListBox_GetHitItem(HWND hwndList,int x,int y)
{
	int Index;

	Index=ListBox_GetTopIndex(hwndList)+y/ListBox_GetItemHeight(hwndList,0);
	if (Index<0 || Index>=ListBox_GetCount(hwndList))
		return -1;
	return Index;
}


static void ListBox_MoveItem(HWND hwndList,int From,int To)
{
	int Top=ListBox_GetTopIndex(hwndList);
	LPARAM lData;

	lData=ListBox_GetItemData(hwndList,From);
	ListBox_DeleteString(hwndList,From);
	ListBox_InsertItemData(hwndList,To,lData);
	ListBox_SetCurSel(hwndList,To);
	ListBox_SetTopIndex(hwndList,Top);
}


static void ListBox_EnsureVisible(HWND hwndList,int Index)
{
	int Top;

	Top=ListBox_GetTopIndex(hwndList);
	if (Index<Top) {
		ListBox_SetTopIndex(hwndList,Index);
	} else if (Index>Top) {
		RECT rc;
		int Rows;

		GetClientRect(hwndList,&rc);
		Rows=rc.bottom/ListBox_GetItemHeight(hwndList,0);
		if (Rows==0)
			Rows=1;
		if (Index>=Top+Rows)
			ListBox_SetTopIndex(hwndList,Index-Rows+1);
	}
}


void CStatusOptions::DrawInsertMark(HWND hwndList,int Pos)
{
	HDC hdc;
	RECT rc;

	hdc=GetDC(hwndList);
	GetClientRect(hwndList,&rc);
	rc.top=(Pos-ListBox_GetTopIndex(hwndList))*m_ItemHeight-1;
	PatBlt(hdc,0,rc.top,rc.right-rc.left,2,DSTINVERT);
	ReleaseDC(hwndList,hdc);
}


bool CStatusOptions::GetItemPreviewRect(HWND hwndList,int Index,RECT *pRect)
{
	StatusItemInfo *pItemInfo;
	RECT rc;

	if (Index<0)
		return false;
	pItemInfo=reinterpret_cast<StatusItemInfo*>(ListBox_GetItemData(hwndList,Index));
	ListBox_GetItemRect(hwndList,Index,&rc);
	OffsetRect(&rc,-GetScrollPos(hwndList,SB_HORZ),0);
	rc.left+=ITEM_MARGIN+CHECK_WIDTH+ITEM_MARGIN+m_TextWidth+ITEM_MARGIN+1;
	rc.right=rc.left+(pItemInfo->Width>=0?pItemInfo->Width:
				m_pStatusView->GetItemByID(pItemInfo->ID)->GetDefaultWidth());
	rc.top+=ITEM_MARGIN+1;
	rc.bottom-=ITEM_MARGIN+1;
	*pRect=rc;
	return true;
}


bool CStatusOptions::IsCursorResize(HWND hwndList,int x,int y)
{
	RECT rc;

	if (!GetItemPreviewRect(hwndList,ListBox_GetHitItem(hwndList,x,y),&rc))
		return false;
	return x>=rc.right-2 && x<=rc.right+2;
}


LRESULT CALLBACK CStatusOptions::ItemListProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	CStatusOptions *pThis=static_cast<CStatusOptions*>(GetThis(GetParent(hwnd)));

	switch (uMsg) {
	case WM_LBUTTONDOWN:
		{
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			int Sel;

			SetFocus(hwnd);
			Sel=ListBox_GetHitItem(hwnd,x,y);
			if (Sel>=0) {
				RECT rc;

				ListBox_GetItemRect(hwnd,Sel,&rc);
				OffsetRect(&rc,-GetScrollPos(hwnd,SB_HORZ),0);
				if (x>=rc.left+ITEM_MARGIN && x<rc.left+ITEM_MARGIN+CHECK_WIDTH) {
					StatusItemInfo *pItemInfo=reinterpret_cast<StatusItemInfo*>(ListBox_GetItemData(hwnd,Sel));
					RECT rc;

					pItemInfo->fVisible=!pItemInfo->fVisible;
					ListBox_GetItemRect(hwnd,Sel,&rc);
					InvalidateRect(hwnd,&rc,TRUE);
				} else {
					if (ListBox_GetCurSel(hwnd)!=Sel)
						ListBox_SetCurSel(hwnd,Sel);
					SetCapture(hwnd);
					pThis->m_fDragResize=pThis->IsCursorResize(hwnd,x,y);
				}
			}
		}
		return 0;

	case WM_LBUTTONUP:
		if (GetCapture()==hwnd)
			ReleaseCapture();
		return 0;

	case WM_CAPTURECHANGED:
		if (pThis->m_DragTimerID!=0) {
			KillTimer(hwnd,pThis->m_DragTimerID);
			pThis->m_DragTimerID=0;
		}
		if (pThis->m_DropInsertPos>=0) {
			int From=ListBox_GetCurSel(hwnd),To;

			pThis->DrawInsertMark(hwnd,pThis->m_DropInsertPos);
			To=pThis->m_DropInsertPos;
			if (To>From)
				To--;
			SetWindowRedraw(hwnd,FALSE);
			ListBox_MoveItem(hwnd,From,To);
			SetWindowRedraw(hwnd,TRUE);
			pThis->m_DropInsertPos=-1;
		}
		return 0;

	case WM_MOUSEMOVE:
		if (GetCapture()==hwnd) {
			int y=GET_Y_LPARAM(lParam);
			RECT rc;

			GetClientRect(hwnd,&rc);
			if (pThis->m_fDragResize) {
				int x=GET_X_LPARAM(lParam);
				int Sel=ListBox_GetCurSel(hwnd);
				RECT rc;

				if (pThis->GetItemPreviewRect(hwnd,Sel,&rc)) {
					StatusItemInfo *pItemInfo=reinterpret_cast<StatusItemInfo*>(ListBox_GetItemData(hwnd,Sel));

					pItemInfo->Width=max(x-(int)rc.left,
						pThis->m_pStatusView->GetItemByID(pItemInfo->ID)->GetMinWidth());
					ListBox_GetItemRect(hwnd,Sel,&rc);
					InvalidateRect(hwnd,&rc,TRUE);
					pThis->SetListHExtent(GetParent(hwnd));
				}
			} else if (y>=0 && y<rc.bottom) {
				int Insert,Count,Sel;

				if (pThis->m_DragTimerID!=0) {
					KillTimer(hwnd,pThis->m_DragTimerID);
					pThis->m_DragTimerID=0;
				}
				Insert=ListBox_GetTopIndex(hwnd)+
								(y+pThis->m_ItemHeight/2)/pThis->m_ItemHeight;
				Count=ListBox_GetCount(hwnd);
				if (Insert>Count) {
					Insert=Count;
				} else {
					Sel=ListBox_GetCurSel(hwnd);
					if (Insert==Sel || Insert==Sel+1)
						Insert=-1;
				}
				if (pThis->m_DropInsertPos>=0)
					pThis->DrawInsertMark(hwnd,pThis->m_DropInsertPos);
				pThis->m_DropInsertPos=Insert;
				if (pThis->m_DropInsertPos>=0)
					pThis->DrawInsertMark(hwnd,pThis->m_DropInsertPos);
				SetCursor(LoadCursor(NULL,IDC_ARROW));
			} else {
				UINT TimerID;

				if (pThis->m_DropInsertPos>=0) {
					pThis->DrawInsertMark(hwnd,pThis->m_DropInsertPos);
					pThis->m_DropInsertPos=-1;
				}
				if (y<0)
					TimerID=TIMER_ID_UP;
				else
					TimerID=TIMER_ID_DOWN;
				if (TimerID!=pThis->m_DragTimerID) {
					if (pThis->m_DragTimerID!=0)
						KillTimer(hwnd,pThis->m_DragTimerID);
					pThis->m_DragTimerID=(UINT)SetTimer(hwnd,TimerID,100,NULL);
				}
				SetCursor(LoadCursor(NULL,IDC_NO));
			}
		} else {
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);

			SetCursor(LoadCursor(NULL,pThis->IsCursorResize(hwnd,x,y)?IDC_SIZEWE:IDC_ARROW));
		}
		return 0;

	case WM_RBUTTONDOWN:
		if (GetCapture()==hwnd) {
			ReleaseCapture();
			if (pThis->m_DragTimerID!=0) {
				KillTimer(hwnd,pThis->m_DragTimerID);
				pThis->m_DragTimerID=0;
			}
			if (pThis->m_DropInsertPos>=0) {
				pThis->DrawInsertMark(hwnd,pThis->m_DropInsertPos);
				pThis->m_DropInsertPos=-1;
			}
		}
		return 0;

	case WM_TIMER:
		{
			int Pos;

			Pos=ListBox_GetTopIndex(hwnd);
			if (wParam==TIMER_ID_UP) {
				if (Pos>0)
					Pos--;
			} else
				Pos++;
			ListBox_SetTopIndex(hwnd,Pos);
		}
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT)
			return TRUE;
		break;
	}
	return CallWindowProc(pThis->m_pOldListProc,hwnd,uMsg,wParam,lParam);
}


bool CStatusOptions::ApplyItemList()
{
	int i;
	int ItemOrder[NUM_STATUS_ITEMS];
	CStatusItem *pItem;

	for (i=0;i<NUM_STATUS_ITEMS;i++) {
		ItemOrder[i]=m_ItemList[i].ID;
		pItem=m_pStatusView->GetItemByID(ItemOrder[i]);
		pItem->SetVisible(m_ItemList[i].fVisible);
		if (m_ItemList[i].Width>=0)
			pItem->SetWidth(m_ItemList[i].Width);
		else
			pItem->SetWidth(pItem->GetDefaultWidth());
	}
	return m_pStatusView->SetItemOrder(ItemOrder);
}


bool CStatusOptions::ApplyOptions()
{
	m_pStatusView->EnableSizeAdjustment(false);
	m_pStatusView->SetMultiRow(m_fMultiRow);
	m_pStatusView->SetMaxRows(m_MaxRows);
	m_pStatusView->SetFont(&m_lfItemFont);
	ApplyItemList();
	m_pStatusView->EnableSizeAdjustment(true);
	return true;
}
