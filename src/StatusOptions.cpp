#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "StatusOptions.h"
#include "Settings.h"
#include "DialogUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


static const bool IS_HD=
#ifndef TVTEST_FOR_1SEG
	true
#else
	false
#endif
	;

#define TIMER_ID_UP		1
#define TIMER_ID_DOWN	2




CStatusOptions::CStatusOptions(CStatusView *pStatusView)
	: COptions(TEXT("Status"))
	, m_pStatusView(pStatusView)
	, m_ItemID(STATUS_ITEM_LAST+1)
	, m_fShowTOTTime(false)
	, m_fEnablePopupProgramInfo(true)
	, m_fShowEventProgress(true)
	, m_fMultiRow(!IS_HD)
	, m_MaxRows(2)
	, m_fShowPopup(true)
	, m_PopupOpacity(OPACITY_MAX)

	, m_ItemListSubclass(this)
	, m_ItemMargin(3)
	, m_CheckSize(14,14)
{
	static const struct {
		BYTE ID;
		bool fVisible;
	} DefaultItemList[] = {
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

	m_AvailItemList.reserve(lengthof(DefaultItemList));

	for (int i=0;i<lengthof(DefaultItemList);i++) {
		StatusItemInfo Info;

		Info.ID=DefaultItemList[i].ID;
		Info.fVisible=DefaultItemList[i].fVisible;
		Info.Width=-1;

		m_AvailItemList.push_back(Info);
	}

	m_ItemList=m_AvailItemList;

	m_pStatusView->GetFont(&m_lfItemFont);
}


CStatusOptions::~CStatusOptions()
{
	Destroy();
}


bool CStatusOptions::ReadSettings(CSettings &Settings)
{
	int NumItems;
	if (Settings.Read(TEXT("NumItems"),&NumItems) && NumItems>0) {
		StatusItemInfoList ItemList;
		TVTest::String ID;

		for (int i=0;i<NumItems;i++) {
			TCHAR szKey[32];

			StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Item%d_ID"),i);
			if (Settings.Read(szKey,&ID) && !ID.empty()) {
				StatusItemInfo Item;

				LPTSTR p;
				long IDNum=std::_tcstol(ID.c_str(),&p,10);
				if (*p==_T('\0')) {
					if (IDNum>=STATUS_ITEM_FIRST && IDNum<=STATUS_ITEM_LAST) {
						Item.ID=(int)IDNum;
					} else {
						continue;
					}
					size_t j;
					for (j=0;j<ItemList.size();j++) {
						if (ItemList[j].ID==Item.ID)
							break;
					}
					if (j<ItemList.size())
						continue;
					for (size_t j=0;j<m_AvailItemList.size();j++) {
						if (m_AvailItemList[j].ID==Item.ID) {
							Item.fVisible=m_AvailItemList[j].fVisible;
							break;
						}
					}
				} else {
					Item.ID=-1;
					Item.IDText=ID;
					size_t j=0;
					for (j=0;j<ItemList.size();j++) {
						if (TVTest::StringUtility::CompareNoCase(ItemList[j].IDText,Item.IDText)==0)
							break;
					}
					if (j<ItemList.size())
						continue;
					Item.fVisible=false;
				}

				StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Item%d_Visible"),i);
				Settings.Read(szKey,&Item.fVisible);
				StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Item%d_Width"),i);
				if (!Settings.Read(szKey,&Item.Width) || Item.Width<1)
					Item.Width=-1;

				ItemList.push_back(Item);
			}
		}

		for (size_t i=0;i<m_AvailItemList.size();i++) {
			const int ID=m_AvailItemList[i].ID;
			bool fFound=false;
			for (size_t j=0;j<ItemList.size();j++) {
				if (ItemList[j].ID==ID) {
					fFound=true;
					break;
				}
			}
			if (!fFound) {
				StatusItemInfo Item(m_AvailItemList[i]);
				Item.fVisible=false;
				ItemList.push_back(Item);
			}
		}

		m_ItemList=ItemList;
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
	Settings.Read(TEXT("ShowPopup"),&m_fShowPopup);
	if (Settings.Read(TEXT("PopupOpacity"),&Value))
		m_PopupOpacity=CLAMP(Value,OPACITY_MIN,OPACITY_MAX);

	Settings.Read(TEXT("TOTTime"),&m_fShowTOTTime);
	Settings.Read(TEXT("PopupProgramInfo"),&m_fEnablePopupProgramInfo);
	Settings.Read(TEXT("ShowEventProgress"),&m_fShowEventProgress);

	return true;
}


bool CStatusOptions::WriteSettings(CSettings &Settings)
{
	if (Settings.Write(TEXT("NumItems"),(int)m_ItemList.size())) {
		for (int i=0;i<(int)m_ItemList.size();i++) {
			const StatusItemInfo &Info=m_ItemList[i];
			TCHAR szKey[32];

			StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Item%d_ID"),i);
			if (!Info.IDText.empty())
				Settings.Write(szKey,Info.IDText);
			else
				Settings.Write(szKey,Info.ID);
			StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Item%d_Visible"),i);
			Settings.Write(szKey,Info.fVisible);
			StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Item%d_Width"),i);
			Settings.Write(szKey,Info.Width);
		}
	}

	// Font
	Settings.Write(TEXT("FontName"),m_lfItemFont.lfFaceName);
	Settings.Write(TEXT("FontSize"),(int)m_lfItemFont.lfHeight);
	Settings.Write(TEXT("FontWeight"),(int)m_lfItemFont.lfWeight);
	Settings.Write(TEXT("FontItalic"),(int)m_lfItemFont.lfItalic);

	Settings.Write(TEXT("MultiRow"),m_fMultiRow);
	Settings.Write(TEXT("MaxRows"),m_MaxRows);
	Settings.Write(TEXT("ShowPopup"),m_fShowPopup);
	Settings.Write(TEXT("PopupOpacity"),m_PopupOpacity);

	Settings.Write(TEXT("TOTTime"),m_fShowTOTTime);
	Settings.Write(TEXT("PopupProgramInfo"),m_fEnablePopupProgramInfo);
	Settings.Write(TEXT("ShowEventProgress"),m_fShowEventProgress);

	return true;
}


bool CStatusOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_STATUS));
}


bool CStatusOptions::ApplyOptions()
{
	m_pStatusView->EnableSizeAdjustment(false);
	m_pStatusView->SetMultiRow(m_fMultiRow);
	m_pStatusView->SetMaxRows(m_MaxRows);
	m_pStatusView->SetFont(&m_lfItemFont);
	m_pStatusView->EnableSizeAdjustment(true);
	return true;
}


bool CStatusOptions::ApplyItemList()
{
	StatusItemInfoList ItemList;

	MakeItemList(&ItemList);

	std::vector<int> ItemOrder;
	ItemOrder.resize(ItemList.size());

	for (size_t i=0;i<ItemList.size();i++) {
		ItemOrder[i]=ItemList[i].ID;
		CStatusItem *pItem=m_pStatusView->GetItemByID(ItemOrder[i]);
		pItem->SetVisible(ItemList[i].fVisible);
		if (ItemList[i].Width>=0)
			pItem->SetWidth(ItemList[i].Width);
	}

	return m_pStatusView->SetItemOrder(ItemOrder.data());
}


int CStatusOptions::RegisterItem(LPCTSTR pszID)
{
	if (IsStringEmpty(pszID))
		return -1;

	StatusItemInfo Item;

	Item.ID=m_ItemID++;
	Item.IDText=pszID;
	Item.fVisible=false;
	Item.Width=-1;

	m_AvailItemList.push_back(Item);

	for (size_t i=0;i<m_ItemList.size();i++) {
		if (m_ItemList[i].ID<0
				&& TVTest::StringUtility::CompareNoCase(m_ItemList[i].IDText,Item.IDText)==0) {
			m_ItemList[i].ID=Item.ID;
			break;
		}
	}

	return Item.ID;
}


bool CStatusOptions::SetItemVisibility(int ID,bool fVisible)
{
	for (auto itr=m_ItemList.begin();itr!=m_ItemList.end();++itr) {
		if (itr->ID==ID) {
			itr->fVisible=fVisible;
			return true;
		}
	}

	return false;
}


void CStatusOptions::InitListBox(HWND hDlg)
{
	for (size_t i=0;i<m_ItemListCur.size();i++)
		DlgListBox_AddString(hDlg,IDC_STATUSOPTIONS_ITEMLIST,&m_ItemListCur[i]);
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

			const TVTest::Style::CStyleManager &StyleManager=GetAppClass().StyleManager;
			StyleManager.ToPixels(&m_ItemMargin);
			StyleManager.ToPixels(&m_CheckSize);
			m_ItemHeight=m_pStatusView->GetItemHeight()+m_ItemMargin.Vert();
			DlgListBox_SetItemHeight(hDlg,IDC_STATUSOPTIONS_ITEMLIST,0,m_ItemHeight);

			MakeItemList(&m_ItemListCur);
			for (auto itr=m_ItemListCur.begin();itr!=m_ItemListCur.end();++itr) {
				const CStatusItem *pItem=m_pStatusView->GetItemByID(itr->ID);
				itr->fVisible=pItem->GetVisible();
			}
			InitListBox(hDlg);
			CalcTextWidth(hDlg);
			SetListHExtent(hDlg);

			m_ItemListSubclass.SetSubclass(::GetDlgItem(hDlg,IDC_STATUSOPTIONS_ITEMLIST));

			m_CurSettingFont=m_lfItemFont;
			SetFontInfo(hDlg,&m_CurSettingFont);
			DlgCheckBox_Check(hDlg,IDC_STATUSOPTIONS_MULTIROW,m_fMultiRow);
			::SetDlgItemInt(hDlg,IDC_STATUSOPTIONS_MAXROWS,m_MaxRows,TRUE);
			DlgUpDown_SetRange(hDlg,IDC_STATUSOPTIONS_MAXROWS_UPDOWN,1,UD_MAXVAL);
			EnableDlgItems(hDlg,
						   IDC_STATUSOPTIONS_MAXROWS_LABEL,
						   IDC_STATUSOPTIONS_MAXROWS_UPDOWN,
						   m_fMultiRow);

			DlgCheckBox_Check(hDlg,IDC_STATUSOPTIONS_SHOWPOPUP,m_fShowPopup);

			::SendDlgItemMessage(hDlg,IDC_STATUSOPTIONS_OPACITY_SLIDER,
								 TBM_SETRANGE,TRUE,MAKELPARAM(OPACITY_MIN,OPACITY_MAX));
			::SendDlgItemMessage(hDlg,IDC_STATUSOPTIONS_OPACITY_SLIDER,
								 TBM_SETPOS,TRUE,m_PopupOpacity);
			::SendDlgItemMessage(hDlg,IDC_STATUSOPTIONS_OPACITY_SLIDER,
								 TBM_SETPAGESIZE,0,10);
			::SendDlgItemMessage(hDlg,IDC_STATUSOPTIONS_OPACITY_SLIDER,
								 TBM_SETTICFREQ,10,0);
			DlgEdit_SetInt(hDlg,IDC_STATUSOPTIONS_OPACITY_INPUT,m_PopupOpacity);
			DlgUpDown_SetRange(hDlg,IDC_STATUSOPTIONS_OPACITY_SPIN,OPACITY_MIN,OPACITY_MAX);
			EnableDlgItems(hDlg,IDC_STATUSOPTIONS_OPACITY_LABEL,IDC_STATUSOPTIONS_OPACITY_UNIT,
						   m_fShowPopup && Util::OS::IsWindows8OrLater());
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

					if ((pdis->itemState & ODS_SELECTED)==0) {
						TextColor=COLOR_WINDOWTEXT;
						BackColor=COLOR_WINDOW;
					} else {
						TextColor=COLOR_HIGHLIGHTTEXT;
						BackColor=COLOR_HIGHLIGHT;
					}
					::FillRect(pdis->hDC,&pdis->rcItem,
							   reinterpret_cast<HBRUSH>(static_cast<INT_PTR>(BackColor+1)));
					OldBkMode=::SetBkMode(pdis->hDC,TRANSPARENT);
					crTextColor=::GetSysColor(TextColor);
					if (!pItemInfo->fVisible)
						crTextColor=MixColor(crTextColor,::GetSysColor(BackColor));
					crOldTextColor=::SetTextColor(pdis->hDC,crTextColor);
					rc.left=pdis->rcItem.left+m_ItemMargin.Left;
					rc.top=pdis->rcItem.top+m_ItemMargin.Top;
					rc.right=rc.left+m_CheckSize.Width;
					rc.bottom=pdis->rcItem.bottom-m_ItemMargin.Bottom;
					::DrawFrameControl(pdis->hDC,&rc,DFC_BUTTON,
						DFCS_BUTTONCHECK | (pItemInfo->fVisible?DFCS_CHECKED:0));
					rc.left=pdis->rcItem.left+m_ItemMargin.Horz()+m_CheckSize.Width;
					rc.top=pdis->rcItem.top+m_ItemMargin.Top;
					rc.right=rc.left+m_TextWidth;
					rc.bottom=pdis->rcItem.bottom-m_ItemMargin.Bottom;
					::DrawText(pdis->hDC,pItem->GetName(),-1,&rc,
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
					rc.left=rc.right+m_ItemMargin.Left;
					rc.right=rc.left+(pItemInfo->Width>=0?pItemInfo->Width:pItem->GetWidth())+
						m_pStatusView->GetItemPadding().Horz();
					const int ItemHeight=m_ItemHeight-m_ItemMargin.Vert();
					rc.top+=((rc.bottom-rc.top)-ItemHeight)/2;
					rc.bottom=rc.top+ItemHeight;
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
					m_pStatusView->DrawItemPreview(pItem,pdis->hDC,rc,false,hfont);
					::DeleteObject(hfont);
					::SetBkMode(pdis->hDC,OldBkMode);
					::SetTextColor(pdis->hDC,crOldTextColor);
					if ((int)pdis->itemID==m_DropInsertPos
								|| (int)pdis->itemID+1==m_DropInsertPos)
						::PatBlt(pdis->hDC,pdis->rcItem.left,
							(int)pdis->itemID==m_DropInsertPos?
										pdis->rcItem.top:pdis->rcItem.bottom-1,
							pdis->rcItem.right-pdis->rcItem.left,1,DSTINVERT);
					if ((pdis->itemState & ODS_FOCUS)==0)
						break;
				}
			case ODA_FOCUS:
				if ((pdis->itemState & ODS_NOFOCUSRECT)==0)
					::DrawFocusRect(pdis->hDC,&pdis->rcItem);
				break;
			}
		}
		return TRUE;

	case WM_HSCROLL:
		if (reinterpret_cast<HWND>(lParam)==::GetDlgItem(hDlg,IDC_STATUSOPTIONS_OPACITY_SLIDER)) {
			SyncEditWithTrackBar(hDlg,
								 IDC_STATUSOPTIONS_OPACITY_SLIDER,
								 IDC_STATUSOPTIONS_OPACITY_INPUT);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_STATUSOPTIONS_DEFAULT:
			DlgListBox_Clear(hDlg,IDC_STATUSOPTIONS_ITEMLIST);
			m_ItemListCur=m_AvailItemList;
			InitListBox(hDlg);
			SetListHExtent(hDlg);
			return TRUE;

		case IDC_STATUSOPTIONS_CHOOSEFONT:
			{
				LOGFONT lf=m_CurSettingFont;

				if (ChooseFontDialog(hDlg,&lf)
						&& !CompareLogFont(&m_CurSettingFont,&lf)) {
					DrawUtil::CFont Font(lf);

					m_CurSettingFont=lf;
					SetFontInfo(hDlg,&lf);
					m_ItemHeight=m_pStatusView->CalcItemHeight(Font)+m_ItemMargin.Vert();
					DlgListBox_SetItemHeight(hDlg,IDC_STATUSOPTIONS_ITEMLIST,0,m_ItemHeight);
					InvalidateDlgItem(hDlg,IDC_STATUSOPTIONS_ITEMLIST);
				}
			}
			return TRUE;

		case IDC_STATUSOPTIONS_MULTIROW:
			EnableDlgItemsSyncCheckBox(hDlg,
									   IDC_STATUSOPTIONS_MAXROWS_LABEL,
									   IDC_STATUSOPTIONS_MAXROWS_UPDOWN,
									   IDC_STATUSOPTIONS_MULTIROW);
			return TRUE;

		case IDC_STATUSOPTIONS_SHOWPOPUP:
			EnableDlgItems(hDlg,IDC_STATUSOPTIONS_OPACITY_LABEL,IDC_STATUSOPTIONS_OPACITY_UNIT,
				DlgCheckBox_IsChecked(hDlg,IDC_STATUSOPTIONS_SHOWPOPUP) && Util::OS::IsWindows8OrLater());
			return TRUE;

		case IDC_STATUSOPTIONS_OPACITY_INPUT:
			if (HIWORD(wParam)==EN_CHANGE) {
				SyncTrackBarWithEdit(hDlg,
									 IDC_STATUSOPTIONS_OPACITY_INPUT,
									 IDC_STATUSOPTIONS_OPACITY_SLIDER);
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				m_pStatusView->EnableSizeAdjustment(false);

				m_ItemList.resize(m_ItemListCur.size());
				for (size_t i=0;i<m_ItemListCur.size();i++) {
					m_ItemList[i]=*reinterpret_cast<StatusItemInfo*>(
						DlgListBox_GetItemData(hDlg,IDC_STATUSOPTIONS_ITEMLIST,i));
				}
				ApplyItemList();

				if (!CompareLogFont(&m_lfItemFont,&m_CurSettingFont)) {
					m_lfItemFont=m_CurSettingFont;
					m_pStatusView->SetFont(&m_lfItemFont);
					for (size_t i=0;i<m_ItemList.size();i++) {
						StatusItemInfo &Item=m_ItemList[i];
						if (Item.Width<0)
							Item.Width=m_pStatusView->GetItemByID(Item.ID)->GetWidth();
					}
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

				m_fShowPopup=DlgCheckBox_IsChecked(hDlg,IDC_STATUSOPTIONS_SHOWPOPUP);
				if (Util::OS::IsWindows8OrLater()) {
					int Opacity=DlgEdit_GetInt(hDlg,IDC_STATUSOPTIONS_OPACITY_INPUT);
					m_PopupOpacity=CLAMP(Opacity,OPACITY_MIN,OPACITY_MAX);
				}

				m_pStatusView->EnableSizeAdjustment(true);

				m_fChanged=true;
			}
			break;
		}
		break;
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
		pItem=m_pStatusView->GetItemByID(m_ItemListCur[i].ID);
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
	HWND hwndList=::GetDlgItem(hDlg,IDC_STATUSOPTIONS_ITEMLIST);
	int MaxWidth=0;

	for (size_t i=0;i<m_ItemListCur.size();i++) {
		const StatusItemInfo &Item=m_ItemListCur[i];
		int Width=Item.Width>=0?Item.Width:m_pStatusView->GetItemByID(Item.ID)->GetWidth();
		if (Width>MaxWidth)
			MaxWidth=Width;
	}

	ListBox_SetHorizontalExtent(hwndList,
		m_ItemMargin.Horz()+m_CheckSize.Width+m_TextWidth+
		m_ItemMargin.Horz()+MaxWidth+m_pStatusView->GetItemPadding().Horz());
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
	rc.left+=m_ItemMargin.Horz()+m_CheckSize.Width+m_TextWidth+m_ItemMargin.Left;
	rc.right=rc.left+
		(pItemInfo->Width>=0?pItemInfo->Width:
			m_pStatusView->GetItemByID(pItemInfo->ID)->GetWidth())+
		m_pStatusView->GetItemPadding().Horz();
	rc.top+=m_ItemMargin.Top;
	rc.bottom-=m_ItemMargin.Bottom;
	*pRect=rc;
	return true;
}


bool CStatusOptions::IsCursorResize(HWND hwndList,int x,int y)
{
	RECT rc;

	if (!GetItemPreviewRect(hwndList,ListBox_GetHitItem(hwndList,x,y),&rc))
		return false;
	int Margin=(::GetSystemMetrics(SM_CXSIZEFRAME)+1)/2;
	return x>=rc.right-Margin && x<=rc.right+Margin;
}


void CStatusOptions::MakeItemList(StatusItemInfoList *pList) const
{
	pList->clear();
	pList->reserve(m_AvailItemList.size());

	for (auto itr=m_ItemList.begin();itr!=m_ItemList.end();++itr) {
		if (itr->ID>=0)
			pList->push_back(*itr);
	}

	if (pList->size()<m_AvailItemList.size()) {
		for (size_t i=0;i<m_AvailItemList.size();i++) {
			const int ID=m_AvailItemList[i].ID;
			bool fFound=false;
			for (size_t j=0;j<pList->size();j++) {
				if ((*pList)[j].ID==ID) {
					fFound=true;
					break;
				}
			}
			if (!fFound) {
				pList->push_back(m_AvailItemList[i]);
				const CStatusItem *pItem=m_pStatusView->GetItemByID(ID);
				if (pItem!=NULL)
					pList->back().fVisible=pItem->GetVisible();
			}
		}
	}
}


CStatusOptions::CItemListSubclass::CItemListSubclass(CStatusOptions *pStatusOptions)
	: m_pStatusOptions(pStatusOptions)
{
}


LRESULT CStatusOptions::CItemListSubclass::OnMessage(
	HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
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
				if (x>=rc.left+m_pStatusOptions->m_ItemMargin.Left
						&& x<rc.left+m_pStatusOptions->m_ItemMargin.Left+m_pStatusOptions->m_CheckSize.Width) {
					StatusItemInfo *pItemInfo=reinterpret_cast<StatusItemInfo*>(ListBox_GetItemData(hwnd,Sel));
					RECT rc;

					pItemInfo->fVisible=!pItemInfo->fVisible;
					ListBox_GetItemRect(hwnd,Sel,&rc);
					InvalidateRect(hwnd,&rc,TRUE);
				} else {
					if (ListBox_GetCurSel(hwnd)!=Sel)
						ListBox_SetCurSel(hwnd,Sel);
					SetCapture(hwnd);
					m_pStatusOptions->m_fDragResize=m_pStatusOptions->IsCursorResize(hwnd,x,y);
				}
			}
		}
		return 0;

	case WM_LBUTTONUP:
		if (GetCapture()==hwnd)
			ReleaseCapture();
		return 0;

	case WM_CAPTURECHANGED:
		if (m_pStatusOptions->m_DragTimerID!=0) {
			KillTimer(hwnd,m_pStatusOptions->m_DragTimerID);
			m_pStatusOptions->m_DragTimerID=0;
		}
		if (m_pStatusOptions->m_DropInsertPos>=0) {
			int From=ListBox_GetCurSel(hwnd),To;

			m_pStatusOptions->DrawInsertMark(hwnd,m_pStatusOptions->m_DropInsertPos);
			To=m_pStatusOptions->m_DropInsertPos;
			if (To>From)
				To--;
			SetWindowRedraw(hwnd,FALSE);
			ListBox_MoveItem(hwnd,From,To);
			SetWindowRedraw(hwnd,TRUE);
			m_pStatusOptions->m_DropInsertPos=-1;
		}
		return 0;

	case WM_MOUSEMOVE:
		if (GetCapture()==hwnd) {
			int y=GET_Y_LPARAM(lParam);
			RECT rc;

			GetClientRect(hwnd,&rc);
			if (m_pStatusOptions->m_fDragResize) {
				int x=GET_X_LPARAM(lParam);
				int Sel=ListBox_GetCurSel(hwnd);
				RECT rc;

				if (m_pStatusOptions->GetItemPreviewRect(hwnd,Sel,&rc)) {
					StatusItemInfo *pItemInfo=reinterpret_cast<StatusItemInfo*>(ListBox_GetItemData(hwnd,Sel));
					int Width=(x-rc.left)-m_pStatusOptions->m_pStatusView->GetItemPadding().Horz();
					const CStatusItem *pItem=m_pStatusOptions->m_pStatusView->GetItemByID(pItemInfo->ID);
					int MinWidth=pItem->GetMinWidth();
					int MaxWidth=pItem->GetMaxWidth();

					if (Width<MinWidth)
						Width=MinWidth;
					else if (MaxWidth>0 && Width>MaxWidth)
						Width=MaxWidth;
					pItemInfo->Width=Width;
					ListBox_GetItemRect(hwnd,Sel,&rc);
					InvalidateRect(hwnd,&rc,TRUE);
					m_pStatusOptions->SetListHExtent(GetParent(hwnd));
				}
			} else if (y>=0 && y<rc.bottom) {
				int Insert,Count,Sel;

				if (m_pStatusOptions->m_DragTimerID!=0) {
					KillTimer(hwnd,m_pStatusOptions->m_DragTimerID);
					m_pStatusOptions->m_DragTimerID=0;
				}
				Insert=ListBox_GetTopIndex(hwnd)+
								(y+m_pStatusOptions->m_ItemHeight/2)/m_pStatusOptions->m_ItemHeight;
				Count=ListBox_GetCount(hwnd);
				if (Insert>Count) {
					Insert=Count;
				} else {
					Sel=ListBox_GetCurSel(hwnd);
					if (Insert==Sel || Insert==Sel+1)
						Insert=-1;
				}
				if (m_pStatusOptions->m_DropInsertPos>=0)
					m_pStatusOptions->DrawInsertMark(hwnd,m_pStatusOptions->m_DropInsertPos);
				m_pStatusOptions->m_DropInsertPos=Insert;
				if (m_pStatusOptions->m_DropInsertPos>=0)
					m_pStatusOptions->DrawInsertMark(hwnd,m_pStatusOptions->m_DropInsertPos);
				SetCursor(LoadCursor(NULL,IDC_ARROW));
			} else {
				UINT TimerID;

				if (m_pStatusOptions->m_DropInsertPos>=0) {
					m_pStatusOptions->DrawInsertMark(hwnd,m_pStatusOptions->m_DropInsertPos);
					m_pStatusOptions->m_DropInsertPos=-1;
				}
				if (y<0)
					TimerID=TIMER_ID_UP;
				else
					TimerID=TIMER_ID_DOWN;
				if (TimerID!=m_pStatusOptions->m_DragTimerID) {
					if (m_pStatusOptions->m_DragTimerID!=0)
						KillTimer(hwnd,m_pStatusOptions->m_DragTimerID);
					m_pStatusOptions->m_DragTimerID=(UINT)SetTimer(hwnd,TimerID,100,NULL);
				}
				SetCursor(LoadCursor(NULL,IDC_NO));
			}
		} else {
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);

			SetCursor(LoadCursor(NULL,m_pStatusOptions->IsCursorResize(hwnd,x,y)?IDC_SIZEWE:IDC_ARROW));
		}
		return 0;

	case WM_RBUTTONDOWN:
		if (GetCapture()==hwnd) {
			ReleaseCapture();
			if (m_pStatusOptions->m_DragTimerID!=0) {
				KillTimer(hwnd,m_pStatusOptions->m_DragTimerID);
				m_pStatusOptions->m_DragTimerID=0;
			}
			if (m_pStatusOptions->m_DropInsertPos>=0) {
				m_pStatusOptions->DrawInsertMark(hwnd,m_pStatusOptions->m_DropInsertPos);
				m_pStatusOptions->m_DropInsertPos=-1;
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

	return CWindowSubclass::OnMessage(hwnd,uMsg,wParam,lParam);
}
