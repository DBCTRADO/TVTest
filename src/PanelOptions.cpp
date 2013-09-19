#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "PanelOptions.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define TAB_ITEM_MARGIN	2
#define TAB_CHECK_WIDTH	14




CPanelOptions::CPanelOptions(CPanelFrame *pPanelFrame)
	: m_pPanelFrame(pPanelFrame)
	, m_fSnapAtMainWindow(true)
	, m_SnapMargin(4)
	, m_fAttachToMainWindow(true)
	, m_Opacity(100)
	, m_fSpecCaptionFont(true)
	, m_FirstTab(-1)
	, m_LastTab(0)
	, m_fProgramInfoUseRichEdit(true)
{
	DrawUtil::GetDefaultUIFont(&m_Font);
	DrawUtil::GetSystemFont(DrawUtil::FONT_MESSAGE,&m_CaptionFont);

	for (int i=0;i<NUM_PANELS;i++) {
		m_TabList[i].ID=PANEL_ID_FIRST+i;
		m_TabList[i].fVisible=true;
	}
}


CPanelOptions::~CPanelOptions()
{
	Destroy();
}


bool CPanelOptions::InitializePanelForm(CPanelForm *pPanelForm)
{
	int TabOrder[NUM_PANELS];

	pPanelForm->SetTabFont(&m_Font);
	pPanelForm->SetPageFont(&m_Font);
	if (m_fSpecCaptionFont) {
		CPanelForm::CPage *pCaptionPanel=pPanelForm->GetPageByID(PANEL_ID_CAPTION);
		if (pCaptionPanel!=NULL)
			pCaptionPanel->SetFont(&m_CaptionFont);
	}
	pPanelForm->SetCurPageByID(GetFirstTab());
	for (int i=0;i<NUM_PANELS;i++) {
		TabOrder[i]=m_TabList[i].ID;
		pPanelForm->SetTabVisible(m_TabList[i].ID,m_TabList[i].fVisible);
	}
	pPanelForm->SetTabOrder(TabOrder);
	return true;
}


bool CPanelOptions::ReadSettings(CSettings &Settings)
{
	int Value;

	if (Settings.Read(TEXT("InfoCurTab"),&Value)
			&& Value>=PANEL_ID_FIRST && Value<=PANEL_ID_LAST)
		m_LastTab=Value;
	if (Settings.Read(TEXT("PanelFirstTab"),&Value)
			&& Value>=-1 && Value<=PANEL_ID_LAST)
		m_FirstTab=Value;
	Settings.Read(TEXT("PanelSnapAtMainWindow"),&m_fSnapAtMainWindow);
	Settings.Read(TEXT("PanelAttachToMainWindow"),&m_fAttachToMainWindow);
	if (Settings.Read(TEXT("PanelOpacity"),&m_Opacity))
		m_pPanelFrame->SetOpacity(m_Opacity*255/100);

	// Font
	TCHAR szFont[LF_FACESIZE];
	if (Settings.Read(TEXT("PanelFontName"),szFont,LF_FACESIZE) && szFont[0]!='\0') {
		::lstrcpy(m_Font.lfFaceName,szFont);
		m_Font.lfEscapement=0;
		m_Font.lfOrientation=0;
		m_Font.lfUnderline=0;
		m_Font.lfStrikeOut=0;
		m_Font.lfCharSet=DEFAULT_CHARSET;
		m_Font.lfOutPrecision=OUT_DEFAULT_PRECIS;
		m_Font.lfClipPrecision=CLIP_DEFAULT_PRECIS;
		m_Font.lfQuality=DRAFT_QUALITY;
		m_Font.lfPitchAndFamily=DEFAULT_PITCH | FF_DONTCARE;
	}
	if (Settings.Read(TEXT("PanelFontSize"),&Value)) {
		m_Font.lfHeight=Value;
		m_Font.lfWidth=0;
	}
	if (Settings.Read(TEXT("PanelFontWeight"),&Value))
		m_Font.lfWeight=Value;
	if (Settings.Read(TEXT("PanelFontItalic"),&Value))
		m_Font.lfItalic=Value;
	Settings.Read(TEXT("CaptionPanelFontSpec"),&m_fSpecCaptionFont);
	if (!Settings.Read(TEXT("CaptionPanelFont"),&m_CaptionFont))
		m_CaptionFont=m_Font;

	int TabCount;
	if (Settings.Read(TEXT("PanelTabCount"),&TabCount) && TabCount>0) {
		if (TabCount>NUM_PANELS)
			TabCount=NUM_PANELS;
		int j=0;
		for (int i=0;i<TabCount;i++) {
			TCHAR szName[32];
			int ID;

			::wsprintf(szName,TEXT("PanelTab%d_ID"),i);
			if (Settings.Read(szName,&ID)
					&& ID>=PANEL_ID_FIRST && ID<=PANEL_ID_LAST) {
				int k;
				for (k=0;k<j;k++) {
					if (m_TabList[k].ID==ID)
						break;
				}
				if (k==j) {
					bool fVisible=true;

					::wsprintf(szName,TEXT("PanelTab%d_Visible"),i);
					Settings.Read(szName,&fVisible);
					m_TabList[j].ID=ID;
					m_TabList[j].fVisible=fVisible;
					j++;
				}
			}
		}
		if (j<NUM_PANELS) {
			for (int i=PANEL_ID_FIRST;i<=PANEL_ID_LAST && j<NUM_PANELS;i++) {
				int k;
				for (k=0;k<j;k++) {
					if (m_TabList[k].ID==i)
						break;
				}
				if (k==j) {
					m_TabList[j].ID=i;
					m_TabList[j].fVisible=true;
					j++;
				}
			}
		}
	}

	Settings.Read(TEXT("InfoPanelUseRichEdit"),&m_fProgramInfoUseRichEdit);

	return true;
}


bool CPanelOptions::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("PanelFirstTab"),m_FirstTab);
	Settings.Write(TEXT("PanelSnapAtMainWindow"),m_fSnapAtMainWindow);
	Settings.Write(TEXT("PanelAttachToMainWindow"),m_fAttachToMainWindow);
	Settings.Write(TEXT("PanelOpacity"),m_Opacity);

	// Font
	Settings.Write(TEXT("PanelFontName"),m_Font.lfFaceName);
	Settings.Write(TEXT("PanelFontSize"),(int)m_Font.lfHeight);
	Settings.Write(TEXT("PanelFontWeight"),(int)m_Font.lfWeight);
	Settings.Write(TEXT("PanelFontItalic"),(int)m_Font.lfItalic);
	Settings.Write(TEXT("CaptionPanelFontSpec"),m_fSpecCaptionFont);
	Settings.Write(TEXT("CaptionPanelFont"),&m_CaptionFont);

	// Tab order
	Settings.Write(TEXT("PanelTabCount"),NUM_PANELS);
	for (int i=0;i<NUM_PANELS;i++) {
		TCHAR szName[32];

		::wsprintf(szName,TEXT("PanelTab%d_ID"),i);
		Settings.Write(szName,m_TabList[i].ID);
		::wsprintf(szName,TEXT("PanelTab%d_Visible"),i);
		Settings.Write(szName,m_TabList[i].fVisible);
	}

	// Information panel
	// UI未実装
	//Settings.Write(TEXT("InfoPanelUseRichEdit"),m_fProgramInfoUseRichEdit);

	return true;
}


bool CPanelOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_PANEL));
}


int CPanelOptions::GetFirstTab() const
{
	return m_FirstTab>=0?m_FirstTab:m_LastTab;
}


static void SetFontInfo(HWND hDlg,int ID,const LOGFONT *plf)
{
	HDC hdc;
	TCHAR szText[LF_FACESIZE+16];

	hdc=GetDC(hDlg);
	if (hdc==NULL)
		return;
	StdUtil::snprintf(szText,lengthof(szText),TEXT("%s, %d pt"),
					  plf->lfFaceName,CalcFontPointHeight(hdc,plf));
	SetDlgItemText(hDlg,ID,szText);
	ReleaseDC(hDlg,hdc);
}


INT_PTR CPanelOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static const LPCTSTR pszTabList[] = {
		TEXT("情報"),
		TEXT("番組表"),
		TEXT("チャンネル"),
		TEXT("操作"),
		TEXT("字幕"),
	};

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			DlgCheckBox_Check(hDlg,IDC_PANELOPTIONS_SNAPATMAINWINDOW,
												m_fSnapAtMainWindow);
			DlgCheckBox_Check(hDlg,IDC_PANELOPTIONS_ATTACHTOMAINWINDOW,
												m_fAttachToMainWindow);

			// Opacity
			::SendDlgItemMessage(hDlg,IDC_PANELOPTIONS_OPACITY_TB,
										TBM_SETRANGE,TRUE,MAKELPARAM(20,100));
			::SendDlgItemMessage(hDlg,IDC_PANELOPTIONS_OPACITY_TB,
										TBM_SETPOS,TRUE,m_Opacity);
			::SendDlgItemMessage(hDlg,IDC_PANELOPTIONS_OPACITY_TB,
										TBM_SETPAGESIZE,0,10);
			::SendDlgItemMessage(hDlg,IDC_PANELOPTIONS_OPACITY_TB,
										TBM_SETTICFREQ,10,0);
			::SetDlgItemInt(hDlg,IDC_PANELOPTIONS_OPACITY_EDIT,m_Opacity,TRUE);
			::SendDlgItemMessage(hDlg,IDC_PANELOPTIONS_OPACITY_UD,
										UDM_SETRANGE,0,MAKELPARAM(100,20));

			HWND hwndTabList=::GetDlgItem(hDlg,IDC_PANELOPTIONS_TABLIST);
			RECT rc;
			::GetClientRect(::GetDlgItem(hDlg,IDC_PANELOPTIONS_TABLIST),&rc);
			::SendMessage(hwndTabList,LB_SETITEMHEIGHT,0,rc.bottom);
			::SendMessage(hwndTabList,LB_SETCOLUMNWIDTH,rc.right/NUM_PANELS,0);
			for (int i=0;i<NUM_PANELS;i++)
				::SendMessage(hwndTabList,LB_ADDSTRING,0,
				MAKELPARAM(m_TabList[i].ID,m_TabList[i].fVisible));
			::SetProp(hwndTabList,TEXT("TabList"),SubclassWindow(hwndTabList,TabListProc));

			DlgComboBox_AddString(hDlg,IDC_PANELOPTIONS_FIRSTTAB,TEXT("最後に表示したタブ"));
			for (int i=0;i<lengthof(pszTabList);i++)
				DlgComboBox_AddString(hDlg,IDC_PANELOPTIONS_FIRSTTAB,pszTabList[i]);
			DlgComboBox_SetCurSel(hDlg,IDC_PANELOPTIONS_FIRSTTAB,m_FirstTab+1);

			m_CurSettingFont=m_Font;
			SetFontInfo(hDlg,IDC_PANELOPTIONS_FONTINFO,&m_Font);
			DlgCheckBox_Check(hDlg,IDC_PANELOPTIONS_SPECCAPTIONFONT,m_fSpecCaptionFont);
			EnableDlgItems(hDlg,IDC_PANELOPTIONS_CAPTIONFONT_INFO,
						   IDC_PANELOPTIONS_CAPTIONFONT_CHOOSE,
						   m_fSpecCaptionFont);
			m_CurSettingCaptionFont=m_CaptionFont;
			SetFontInfo(hDlg,IDC_PANELOPTIONS_CAPTIONFONT_INFO,&m_CaptionFont);
		}
		return TRUE;

	case WM_HSCROLL:
		if (reinterpret_cast<HWND>(lParam)==
				::GetDlgItem(hDlg,IDC_PANELOPTIONS_OPACITY_TB)) {
			SyncEditWithTrackBar(hDlg,IDC_PANELOPTIONS_OPACITY_TB,
									  IDC_PANELOPTIONS_OPACITY_EDIT);
		}
		return TRUE;

	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT pdis=reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

			if ((INT)pdis->itemID>=0) {
				int ID=LOWORD(pdis->itemData);
				if (ID>PANEL_ID_LAST)
					break;
				bool fVisible=HIWORD(pdis->itemData)!=0;
				bool fSelected=(pdis->itemState&ODS_SELECTED)!=0;
				COLORREF OldTextColor;
				int OldBkMode;
				RECT rc;

				::FillRect(pdis->hDC,&pdis->rcItem,reinterpret_cast<HBRUSH>((fSelected?COLOR_HIGHLIGHT:COLOR_WINDOW)+1));
				OldTextColor=::SetTextColor(pdis->hDC,::GetSysColor(fSelected?COLOR_HIGHLIGHTTEXT:COLOR_WINDOWTEXT));
				OldBkMode=::SetBkMode(pdis->hDC,TRANSPARENT);
				rc=pdis->rcItem;
				::InflateRect(&rc,-TAB_ITEM_MARGIN,-TAB_ITEM_MARGIN);
				rc.right=rc.left+TAB_CHECK_WIDTH;
				::DrawFrameControl(pdis->hDC,&rc,DFC_BUTTON,
								DFCS_BUTTONCHECK | (fVisible?DFCS_CHECKED:0));
				rc.left=rc.right+TAB_ITEM_MARGIN;
				rc.right=pdis->rcItem.right-TAB_ITEM_MARGIN;
				::DrawText(pdis->hDC,pszTabList[ID],-1,&rc,DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
				::SetTextColor(pdis->hDC,OldTextColor);
				::SetBkMode(pdis->hDC,OldBkMode);
			}
			if ((pdis->itemState & (ODS_FOCUS | ODS_NOFOCUSRECT))==ODS_FOCUS)
				::DrawFocusRect(pdis->hDC,&pdis->rcItem);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_PANELOPTIONS_OPACITY_EDIT:
			if (HIWORD(wParam)==EN_CHANGE)
				SyncTrackBarWithEdit(hDlg,IDC_PANELOPTIONS_OPACITY_EDIT,
										  IDC_PANELOPTIONS_OPACITY_TB);
			return TRUE;

		case IDC_PANELOPTIONS_TABLIST:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				int Sel=(int)DlgListBox_GetCurSel(hDlg,IDC_PANELOPTIONS_TABLIST);

				::EnableDlgItem(hDlg,IDC_PANELOPTIONS_TAB_LEFT,Sel>0);
				::EnableDlgItem(hDlg,IDC_PANELOPTIONS_TAB_RIGHT,Sel+1<NUM_PANELS);
			}
			return TRUE;

		case IDC_PANELOPTIONS_TAB_LEFT:
		case IDC_PANELOPTIONS_TAB_RIGHT:
			{
				int From=(int)DlgListBox_GetCurSel(hDlg,IDC_PANELOPTIONS_TABLIST),To;

				if (LOWORD(wParam)==IDC_PANELOPTIONS_TAB_LEFT)
					To=From-1;
				else
					To=From+1;
				if (To<0 || To>=NUM_PANELS)
					return TRUE;
				LPARAM Data1=DlgListBox_GetItemData(hDlg,IDC_PANELOPTIONS_TABLIST,From);
				LPARAM Data2=DlgListBox_GetItemData(hDlg,IDC_PANELOPTIONS_TABLIST,To);
				DlgListBox_SetItemData(hDlg,IDC_PANELOPTIONS_TABLIST,To,Data1);
				DlgListBox_SetItemData(hDlg,IDC_PANELOPTIONS_TABLIST,From,Data2);
				DlgListBox_SetCurSel(hDlg,IDC_PANELOPTIONS_TABLIST,To);
				InvalidateDlgItem(hDlg,IDC_PANELOPTIONS_TABLIST);
				::EnableDlgItem(hDlg,IDC_PANELOPTIONS_TAB_LEFT,To>0);
				::EnableDlgItem(hDlg,IDC_PANELOPTIONS_TAB_RIGHT,To+1<NUM_PANELS);
			}
			return TRUE;

		case IDC_PANELOPTIONS_CHOOSEFONT:
			if (ChooseFontDialog(hDlg,&m_CurSettingFont))
				SetFontInfo(hDlg,IDC_PANELOPTIONS_FONTINFO,&m_CurSettingFont);
			return TRUE;

		case IDC_PANELOPTIONS_SPECCAPTIONFONT:
			EnableDlgItemsSyncCheckBox(hDlg,IDC_PANELOPTIONS_CAPTIONFONT_INFO,
									   IDC_PANELOPTIONS_CAPTIONFONT_CHOOSE,
									   IDC_PANELOPTIONS_SPECCAPTIONFONT);
			return TRUE;

		case IDC_PANELOPTIONS_CAPTIONFONT_CHOOSE:
			if (ChooseFontDialog(hDlg,&m_CurSettingCaptionFont))
				SetFontInfo(hDlg,IDC_PANELOPTIONS_CAPTIONFONT_INFO,&m_CurSettingCaptionFont);
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CPanelForm *pPanel=dynamic_cast<CPanelForm*>(m_pPanelFrame->GetWindow());

				m_fSnapAtMainWindow=
					DlgCheckBox_IsChecked(hDlg,IDC_PANELOPTIONS_SNAPATMAINWINDOW);
				m_fAttachToMainWindow=
					DlgCheckBox_IsChecked(hDlg,IDC_PANELOPTIONS_ATTACHTOMAINWINDOW);
				m_Opacity=::GetDlgItemInt(hDlg,IDC_PANELOPTIONS_OPACITY_EDIT,NULL,TRUE);
				m_pPanelFrame->SetOpacity(m_Opacity*255/100);
				m_FirstTab=(int)DlgComboBox_GetCurSel(hDlg,IDC_PANELOPTIONS_FIRSTTAB)-1;

				for (int i=0;i<NUM_PANELS;i++) {
					LPARAM Data=DlgListBox_GetItemData(hDlg,IDC_PANELOPTIONS_TABLIST,i);
					m_TabList[i].ID=LOWORD(Data);
					m_TabList[i].fVisible=HIWORD(Data)!=0;
				}
				if (pPanel!=NULL) {
					int TabOrder[NUM_PANELS];
					for (int i=0;i<NUM_PANELS;i++) {
						TabOrder[i]=m_TabList[i].ID;
						pPanel->SetTabVisible(m_TabList[i].ID,m_TabList[i].fVisible);
					}
					pPanel->SetTabOrder(TabOrder);
				}

				bool fFontChanged=!CompareLogFont(&m_Font,&m_CurSettingFont);
				if (fFontChanged) {
					m_Font=m_CurSettingFont;
					if (pPanel!=NULL) {
						pPanel->SetTabFont(&m_Font);
						pPanel->SetPageFont(&m_Font);
					}
				}

				bool fChangeCaptionFont=false;
				bool fSpecCaptionFont=DlgCheckBox_IsChecked(hDlg,IDC_PANELOPTIONS_SPECCAPTIONFONT);
				if (m_fSpecCaptionFont!=fSpecCaptionFont) {
					m_fSpecCaptionFont=fSpecCaptionFont;
					fChangeCaptionFont=true;
				}
				if (!CompareLogFont(&m_CaptionFont,&m_CurSettingCaptionFont)) {
					m_CaptionFont=m_CurSettingCaptionFont;
					if (m_fSpecCaptionFont)
						fChangeCaptionFont=true;
				} else if (m_fSpecCaptionFont && fFontChanged) {
					fChangeCaptionFont=true;
				}
				if (fChangeCaptionFont) {
					CPanelForm::CPage *pCaptionPanel=pPanel->GetPageByID(PANEL_ID_CAPTION);
					if (pCaptionPanel!=NULL)
						pCaptionPanel->SetFont(m_fSpecCaptionFont?
											   &m_CaptionFont:&m_Font);
				}

				m_fChanged=true;
			}
			break;
		}
		break;
	}

	return FALSE;
}


LRESULT CALLBACK CPanelOptions::TabListProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	WNDPROC pOldWndProc=static_cast<WNDPROC>(::GetProp(hwnd,TEXT("TabList")));

	if (pOldWndProc==NULL)
		return ::DefWindowProc(hwnd,uMsg,wParam,lParam);

	switch (uMsg) {
	case WM_LBUTTONDOWN:
		{
			int x=GET_X_LPARAM(lParam);
			int Count=(int)::SendMessage(hwnd,LB_GETCOUNT,0,0);

			for (int i=0;i<Count;i++) {
				RECT rc;
				::SendMessage(hwnd,LB_GETITEMRECT,i,reinterpret_cast<LPARAM>(&rc));
				if (x>=rc.left+TAB_ITEM_MARGIN && x<rc.left+TAB_ITEM_MARGIN+TAB_CHECK_WIDTH) {
					LPARAM Data=::SendMessage(hwnd,LB_GETITEMDATA,i,0);
					Data=MAKELPARAM(LOWORD(Data),!HIWORD(Data));
					::SendMessage(hwnd,LB_SETITEMDATA,i,Data);
					::InvalidateRect(hwnd,&rc,TRUE);
					break;
				}
			}
		}
		break;

	case WM_NCDESTROY:
		::RemoveProp(hwnd,TEXT("TabList"));
		break;
	}

	return ::CallWindowProc(pOldWndProc,hwnd,uMsg,wParam,lParam);
}
