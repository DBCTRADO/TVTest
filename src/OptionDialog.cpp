#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "OptionDialog.h"
#include "DialogUtil.h"
#include "Graphics.h"
#include "resource.h"
#include "Common/DebugDef.h"




COptionDialog::COptionDialog()
	: m_CurrentPage(0)
	, m_himlIcons(NULL)
{
}


COptionDialog::~COptionDialog()
{
	Destroy();
}


bool COptionDialog::Show(HWND hwndOwner,int StartPage)
{
	if (m_hDlg!=NULL)
		return false;
	COptions::SetFrame(this);
	m_StartPage=StartPage;
	if (ShowDialog(hwndOwner,GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS))!=IDOK) {
		return false;
	}
	if (hwndOwner!=NULL)
		::RedrawWindow(hwndOwner,NULL,NULL,RDW_UPDATENOW | RDW_ALLCHILDREN);
	for (int i=0;i<NUM_PAGES;i++) {
		DWORD Flags=m_PageList[i].pOptions->GetUpdateFlags();

		if (Flags!=0)
			m_PageList[i].pOptions->Apply(Flags);
	}
	return true;
}


bool COptionDialog::SetCurrentPage(int Page)
{
	if (Page<0 || Page>=NUM_PAGES)
		return false;
	if (Page!=m_CurrentPage) {
		if (m_hDlg!=NULL) {
			SetPage(Page);
		} else {
			m_CurrentPage=Page;
		}
	}
	return true;
}


void COptionDialog::CreatePage(int Page)
{
	if (!m_PageList[Page].pOptions->IsCreated()) {
		RECT rcPage,rcOptions;

		m_PageList[Page].pOptions->Create(m_hDlg);
		GetDlgItemRect(m_hDlg,IDC_OPTIONS_PAGEPLACE,&rcPage);
		m_PageList[Page].pOptions->GetPosition(&rcOptions);
		m_PageList[Page].pOptions->SetPosition(rcPage.left,rcPage.top,
											   rcOptions.right-rcOptions.left,
											   rcOptions.bottom-rcOptions.top);
	}
}


void COptionDialog::SetPage(int Page)
{
	if (Page>=0 && Page<NUM_PAGES && m_CurrentPage!=Page) {
		if (!m_PageList[Page].pOptions->IsCreated()) {
			Util::CWaitCursor WaitCursor;
			CreatePage(Page);
		}
		m_PageList[m_CurrentPage].pOptions->SetVisible(false);
		m_PageList[Page].pOptions->SetVisible(true);
		m_CurrentPage=Page;
		DlgListBox_SetCurSel(m_hDlg,IDC_OPTIONS_LIST,Page);
		InvalidateDlgItem(m_hDlg,IDC_OPTIONS_TITLE);
	}
}


COLORREF COptionDialog::GetTitleColor(int Page) const
{
	return HSVToRGB((double)Page/(double)NUM_PAGES,0.4,0.9);
}


INT_PTR COptionDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			const int DPI=GetAppClass().StyleManager.GetSystemDPI();
			m_IconWidth=::GetSystemMetrics(SM_CXSMICON);
			m_IconHeight=::GetSystemMetrics(SM_CYSMICON);
			m_ListMargin=::MulDiv(1,DPI,96);
			m_IconTextMargin=::MulDiv(4,DPI,96);

			RECT rc={0,0,0,10};
			::MapDialogRect(hDlg,&rc);
			DlgListBox_SetItemHeight(hDlg,IDC_OPTIONS_LIST,0,
									 max(rc.bottom,m_IconHeight+m_ListMargin*2));

			COptions::ClearGeneralUpdateFlags();
			for (int i=0;i<NUM_PAGES;i++) {
				m_PageList[i].pOptions->ClearUpdateFlags();
				DlgListBox_AddString(hDlg,IDC_OPTIONS_LIST,i);
			}
			if (m_StartPage>=0 && m_StartPage<NUM_PAGES)
				m_CurrentPage=m_StartPage;
			CreatePage(m_CurrentPage);
			m_PageList[m_CurrentPage].pOptions->SetVisible(true);
			DlgListBox_SetCurSel(hDlg,IDC_OPTIONS_LIST,m_CurrentPage);

			m_himlIcons=::ImageList_LoadImage(
				GetAppClass().GetResourceInstance(),
				m_IconWidth<=16?MAKEINTRESOURCE(IDB_OPTIONS16):MAKEINTRESOURCE(IDB_OPTIONS32),
				m_IconWidth<=16?16:32,
				1,CLR_NONE,IMAGE_BITMAP,LR_CREATEDIBSECTION);

			HFONT hfont=reinterpret_cast<HFONT>(::SendMessage(hDlg,WM_GETFONT,0,0));
			LOGFONT lf;
			::GetObject(hfont,sizeof(LOGFONT),&lf);
			lf.lfWeight=FW_BOLD;
			m_TitleFont.Create(&lf);

			m_fApplied=false;
		}
		return TRUE;

	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT pdis=reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

			if (wParam==IDC_OPTIONS_LIST) {
				const bool fSelected=(pdis->itemState&ODS_SELECTED)!=0;
				COLORREF crText,crOldText;
				int OldBkMode;
				RECT rc;

				if (fSelected) {
					rc=pdis->rcItem;
					rc.right=rc.left+m_ListMargin+m_IconWidth+m_IconTextMargin/2;
					::FillRect(pdis->hDC,&rc,reinterpret_cast<HBRUSH>(COLOR_WINDOW+1));
					rc.left=rc.right;
					rc.right=pdis->rcItem.right;
					DrawUtil::FillGradient(pdis->hDC,&rc,
										   RGB(0,0,0),GetTitleColor((int)pdis->itemData));
					crText=RGB(255,255,255);
				} else {
					::FillRect(pdis->hDC,&pdis->rcItem,
							   reinterpret_cast<HBRUSH>(COLOR_WINDOW+1));
					crText=::GetSysColor(COLOR_WINDOWTEXT);
				}

				rc=pdis->rcItem;
				rc.left+=m_ListMargin;
				int y=rc.top+((rc.bottom-rc.top)-m_IconHeight)/2;

				int IconWidth,IconHeight;
				::ImageList_GetIconSize(m_himlIcons,&IconWidth,&IconHeight);
				if (IconWidth==m_IconWidth && IconHeight==m_IconHeight) {
					::ImageList_Draw(m_himlIcons,(int)pdis->itemData,pdis->hDC,rc.left,y,ILD_TRANSPARENT);
					if (fSelected)
						::ImageList_Draw(m_himlIcons,(int)pdis->itemData,pdis->hDC,rc.left,y,ILD_TRANSPARENT);
				} else {
					HICON hicon=::ImageList_ExtractIcon(NULL,m_himlIcons,(int)pdis->itemData);
#if 0				// DrawIconEx ‚Å•`‰æ‚·‚é‚Æ‰˜‚¢
					::DrawIconEx(pdis->hDC,rc.left,y,hicon,m_IconWidth,m_IconHeight,0,NULL,DI_NORMAL);
					if (fSelected)
						::DrawIconEx(pdis->hDC,rc.left,y,hicon,m_IconWidth,m_IconHeight,0,NULL,DI_NORMAL);
#else
					ICONINFO ii;
					if (::GetIconInfo(hicon,&ii)) {
						HBITMAP hbm=(HBITMAP)::CopyImage(ii.hbmColor,IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
						::DeleteObject(ii.hbmColor);
						::DeleteObject(ii.hbmMask);
						TVTest::Graphics::CImage Image;
						Image.CreateFromBitmap(hbm);
						::DeleteObject(hbm);
						TVTest::Graphics::CCanvas Canvas(pdis->hDC);
						Canvas.SetComposition(true);
						Canvas.DrawImage(rc.left,y,m_IconWidth,m_IconHeight,&Image,0,0,IconWidth,IconHeight);
						if (fSelected)
							Canvas.DrawImage(rc.left,y,m_IconWidth,m_IconHeight,&Image,0,0,IconWidth,IconHeight);
					}
#endif
					::DestroyIcon(hicon);
				}

				crOldText=::SetTextColor(pdis->hDC,crText);
				OldBkMode=::SetBkMode(pdis->hDC,TRANSPARENT);
				rc.left+=m_IconWidth+m_IconTextMargin;
				::DrawText(pdis->hDC,m_PageList[pdis->itemData].pszTitle,-1,
						   &rc,DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
				::SetTextColor(pdis->hDC,crOldText);
				::SetBkMode(pdis->hDC,OldBkMode);
				if ((pdis->itemState & (ODS_FOCUS | ODS_NOFOCUSRECT))==ODS_FOCUS) {
					rc=pdis->rcItem;
					rc.left+=m_ListMargin+m_IconWidth+m_IconTextMargin/2;
					::DrawFocusRect(pdis->hDC,&rc);
				}
			} else if (wParam==IDC_OPTIONS_TITLE) {
				HFONT hfontOld;
				COLORREF crOldText;
				int OldBkMode;
				RECT rc;

				DrawUtil::FillGradient(pdis->hDC,&pdis->rcItem,
									   RGB(0,0,0),GetTitleColor(m_CurrentPage));
				hfontOld=SelectFont(pdis->hDC,m_TitleFont.GetHandle());
				crOldText=::SetTextColor(pdis->hDC,RGB(255,255,255));
				OldBkMode=::SetBkMode(pdis->hDC,TRANSPARENT);
				rc.left=pdis->rcItem.left+2;
				rc.top=pdis->rcItem.top;
				rc.right=pdis->rcItem.right-2;
				rc.bottom=pdis->rcItem.bottom;
				::DrawText(pdis->hDC,m_PageList[m_CurrentPage].pszTitle,-1,
						   &rc,DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
				SelectFont(pdis->hDC,hfontOld);
				::SetTextColor(pdis->hDC,crOldText);
				::SetBkMode(pdis->hDC,OldBkMode);
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OPTIONS_LIST:
			if (HIWORD(wParam)==LBN_SELCHANGE) {
				int NewPage=(int)DlgListBox_GetCurSel(hDlg,IDC_OPTIONS_LIST);

				if (NewPage>=0)
					SetPage(NewPage);
			}
			return TRUE;

		case IDC_OPTIONS_HELP:
			GetAppClass().UICore.ShowHelpContent(m_PageList[m_CurrentPage].HelpID);
			return TRUE;

		case IDOK:
		case IDCANCEL:
			{
				HCURSOR hcurOld;
				NMHDR nmh;
				int i;

				hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));
				nmh.code=LOWORD(wParam)==IDOK?PSN_APPLY:PSN_RESET;
				m_fSettingError=false;
				for (i=0;i<NUM_PAGES;i++) {
					if (m_PageList[i].pOptions->IsCreated()) {
						m_PageList[i].pOptions->SendMessage(WM_NOTIFY,0,reinterpret_cast<LPARAM>(&nmh));
						if (m_fSettingError) {
							::SetCursor(hcurOld);
							return TRUE;
						}
						if (LOWORD(wParam)==IDOK)
							m_fApplied=true;
					}
				}
				::SetCursor(hcurOld);
				::EndDialog(hDlg,m_fApplied?IDOK:IDCANCEL);
			}
			return TRUE;
		}
		return TRUE;

	case WM_DESTROY:
		if (m_himlIcons!=NULL) {
			::ImageList_Destroy(m_himlIcons);
			m_himlIcons=NULL;
		}
		m_TitleFont.Destroy();
		return TRUE;
	}

	return FALSE;
}


void COptionDialog::ActivatePage(COptions *pOptions)
{
	for (int i=0;i<NUM_PAGES;i++) {
		if (m_PageList[i].pOptions==pOptions) {
			SetPage(i);
			break;
		}
	}
}


void COptionDialog::OnSettingError(COptions *pOptions)
{
	for (int i=0;i<NUM_PAGES;i++) {
		if (m_PageList[i].pOptions==pOptions) {
			SetPage(i);
			m_fSettingError=true;
			break;
		}
	}
}
