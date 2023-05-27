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
#include "AppMain.h"
#include "OptionDialog.h"
#include "DialogUtil.h"
#include "Graphics.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


bool COptionDialog::m_fInitialized = false;

COptionDialog::PageInfo COptionDialog::m_PageList[NUM_PAGES] = {
	{TEXT("一般"),               nullptr, HELP_ID_OPTIONS_GENERAL},
	{TEXT("表示"),               nullptr, HELP_ID_OPTIONS_VIEW},
	{TEXT("OSD"),                nullptr, HELP_ID_OPTIONS_OSD},
	{TEXT("通知バー"),           nullptr, HELP_ID_OPTIONS_NOTIFICATIONBAR},
	{TEXT("ステータスバー"),     nullptr, HELP_ID_OPTIONS_STATUSBAR},
	{TEXT("サイドバー"),         nullptr, HELP_ID_OPTIONS_SIDEBAR},
	{TEXT("メニュー"),           nullptr, HELP_ID_OPTIONS_MENU},
	{TEXT("パネル"),             nullptr, HELP_ID_OPTIONS_PANEL},
	{TEXT("テーマ/配色"),        nullptr, HELP_ID_OPTIONS_COLORSCHEME},
	{TEXT("操作"),               nullptr, HELP_ID_OPTIONS_OPERATION},
	{TEXT("キー割り当て"),       nullptr, HELP_ID_OPTIONS_ACCELERATOR},
	{TEXT("リモコン"),           nullptr, HELP_ID_OPTIONS_CONTROLLER},
	{TEXT("BonDriver設定"),      nullptr, HELP_ID_OPTIONS_DRIVER},
	{TEXT("映像"),               nullptr, HELP_ID_OPTIONS_VIDEO},
	{TEXT("音声"),               nullptr, HELP_ID_OPTIONS_AUDIO},
	{TEXT("再生"),               nullptr, HELP_ID_OPTIONS_PLAYBACK},
	{TEXT("録画"),               nullptr, HELP_ID_OPTIONS_RECORD},
	{TEXT("キャプチャ"),         nullptr, HELP_ID_OPTIONS_CAPTURE},
	{TEXT("チャンネルスキャン"), nullptr, HELP_ID_OPTIONS_CHANNELSCAN},
	{TEXT("EPG/番組情報"),       nullptr, HELP_ID_OPTIONS_EPG},
	{TEXT("EPG番組表"),          nullptr, HELP_ID_OPTIONS_PROGRAMGUIDE},
	{TEXT("プラグイン"),         nullptr, HELP_ID_OPTIONS_PLUGIN},
	{TEXT("TSプロセッサー"),     nullptr, HELP_ID_OPTIONS_TSPROCESSOR},
	{TEXT("ログ"),               nullptr, HELP_ID_OPTIONS_LOG},
};


COptionDialog::~COptionDialog()
{
	Destroy();
}


bool COptionDialog::Show(HWND hwndOwner, int StartPage)
{
	if (m_hDlg != nullptr)
		return false;

	if (!m_fInitialized) {
		Initialize();
		m_fInitialized = true;
	}

	COptions::SetFrame(this);
	for (int i = 0; i < NUM_PAGES; i++)
		m_PageList[i].pOptions->SetStyleScaling(m_pStyleScaling);
	m_StartPage = StartPage;
	if (ShowDialog(hwndOwner, GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_OPTIONS)) != IDOK) {
		return false;
	}
	if (hwndOwner != nullptr)
		::RedrawWindow(hwndOwner, nullptr, nullptr, RDW_UPDATENOW | RDW_ALLCHILDREN);
	for (int i = 0; i < NUM_PAGES; i++) {
		const DWORD Flags = m_PageList[i].pOptions->GetUpdateFlags();

		if (Flags != 0)
			m_PageList[i].pOptions->Apply(Flags);
	}
	return true;
}


bool COptionDialog::SetCurrentPage(int Page)
{
	if (Page < 0 || Page >= NUM_PAGES)
		return false;
	if (Page != m_CurrentPage) {
		if (m_hDlg != nullptr) {
			SetPage(Page);
		} else {
			m_CurrentPage = Page;
		}
	}
	return true;
}


void COptionDialog::Initialize()
{
	CAppMain &App = GetAppClass();

	m_PageList[PAGE_GENERAL        ].pOptions = &App.GeneralOptions;
	m_PageList[PAGE_VIEW           ].pOptions = &App.ViewOptions;
	m_PageList[PAGE_OSD            ].pOptions = &App.OSDOptions;
	m_PageList[PAGE_NOTIFICATIONBAR].pOptions = &App.NotificationBarOptions;
	m_PageList[PAGE_STATUS         ].pOptions = &App.StatusOptions;
	m_PageList[PAGE_SIDEBAR        ].pOptions = &App.SideBarOptions;
	m_PageList[PAGE_MENU           ].pOptions = &App.MenuOptions;
	m_PageList[PAGE_PANEL          ].pOptions = &App.PanelOptions;
	m_PageList[PAGE_COLORSCHEME    ].pOptions = &App.ColorSchemeOptions;
	m_PageList[PAGE_OPERATION      ].pOptions = &App.OperationOptions;
	m_PageList[PAGE_ACCELERATOR    ].pOptions = &App.Accelerator;
	m_PageList[PAGE_CONTROLLER     ].pOptions = &App.ControllerManager;
	m_PageList[PAGE_DRIVER         ].pOptions = &App.DriverOptions;
	m_PageList[PAGE_VIDEO          ].pOptions = &App.VideoOptions;
	m_PageList[PAGE_AUDIO          ].pOptions = &App.AudioOptions;
	m_PageList[PAGE_PLAYBACK       ].pOptions = &App.PlaybackOptions;
	m_PageList[PAGE_RECORD         ].pOptions = &App.RecordOptions;
	m_PageList[PAGE_CAPTURE        ].pOptions = &App.CaptureOptions;
	m_PageList[PAGE_CHANNELSCAN    ].pOptions = &App.ChannelScan;
	m_PageList[PAGE_EPG            ].pOptions = &App.EpgOptions;
	m_PageList[PAGE_PROGRAMGUIDE   ].pOptions = &App.ProgramGuideOptions;
	m_PageList[PAGE_PLUGIN         ].pOptions = &App.PluginOptions;
	m_PageList[PAGE_TSPROCESSOR    ].pOptions = &App.TSProcessorOptions;
	m_PageList[PAGE_LOG            ].pOptions = &App.Logger;

	for (int i = 0; i < NUM_PAGES; i++) {
		COptions *pOptions = m_PageList[i].pOptions;

		pOptions->SetStyleScaling(m_pStyleScaling);
		RegisterUIChild(pOptions);
	}
}


void COptionDialog::CreatePage(int Page)
{
	if (!m_PageList[Page].pOptions->IsCreated()) {
		m_PageList[Page].pOptions->Create(m_hDlg);
		SetPagePos(Page);
	}
}


void COptionDialog::SetPage(int Page)
{
	if (Page >= 0 && Page < NUM_PAGES && m_CurrentPage != Page) {
		if (!m_PageList[Page].pOptions->IsCreated()) {
			Util::CWaitCursor WaitCursor;
			CreatePage(Page);
		}
		m_PageList[m_CurrentPage].pOptions->SetVisible(false);
		SetPagePos(Page);
		m_PageList[Page].pOptions->SetVisible(true);
		m_CurrentPage = Page;
		DlgListBox_SetCurSel(m_hDlg, IDC_OPTIONS_LIST, Page);
		InvalidateDlgItem(m_hDlg, IDC_OPTIONS_TITLE);
	}
}


void COptionDialog::SetPagePos(int Page)
{
	if (m_PageList[Page].pOptions->IsCreated()) {
		RECT rcPage;

		GetDlgItemRect(m_hDlg, IDC_OPTIONS_PAGEPLACE, &rcPage);
		m_PageList[Page].pOptions->SetPosition(&rcPage);
	}
}


COLORREF COptionDialog::GetTitleColor(int Page) const
{
	return HSVToRGB(static_cast<double>(Page) / static_cast<double>(NUM_PAGES), 0.4, 0.9);
}


INT_PTR COptionDialog::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			COptions::ClearGeneralUpdateFlags();

			DlgListBox_SetItemHeight(m_hDlg, IDC_OPTIONS_LIST, 0, m_ListItemHeight);

			for (int i = 0; i < NUM_PAGES; i++) {
				m_PageList[i].pOptions->ClearUpdateFlags();
				DlgListBox_AddItem(hDlg, IDC_OPTIONS_LIST, i);
			}
			if (m_StartPage >= 0 && m_StartPage < NUM_PAGES)
				m_CurrentPage = m_StartPage;
			CreatePage(m_CurrentPage);
			m_PageList[m_CurrentPage].pOptions->SetVisible(true);
			DlgListBox_SetCurSel(hDlg, IDC_OPTIONS_LIST, m_CurrentPage);

			AddControls({
				{IDC_OPTIONS_LIST,      AlignFlag::Vert},
				{IDC_OPTIONS_TITLE,     AlignFlag::Horz},
				{IDC_OPTIONS_PAGEPLACE, AlignFlag::All},
				{IDC_OPTIONS_SEPARATOR, AlignFlag::HorzBottom},
				{IDC_OPTIONS_HELP,      AlignFlag::Bottom},
				{IDOK,                  AlignFlag::BottomRight},
				{IDCANCEL,              AlignFlag::BottomRight},
			});

			ApplyPosition();

			m_fApplied = false;
		}
		return TRUE;

	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT pdis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

			if (wParam == IDC_OPTIONS_LIST) {
				const bool fSelected = (pdis->itemState & ODS_SELECTED) != 0;
				COLORREF crText;
				RECT rc;

				if (fSelected) {
					rc = pdis->rcItem;
					rc.right = rc.left + m_ListMargin + m_IconWidth + m_IconTextMargin / 2;
					DrawUtil::Fill(pdis->hDC, &rc, GetThemeColor(COLOR_WINDOW));
					rc.left = rc.right;
					rc.right = pdis->rcItem.right;
					DrawUtil::FillGradient(
						pdis->hDC, &rc, RGB(0, 0, 0), GetTitleColor(static_cast<int>(pdis->itemData)));
					crText = RGB(255, 255, 255);
				} else {
					DrawUtil::Fill(pdis->hDC, &pdis->rcItem, GetThemeColor(COLOR_WINDOW));
					crText = GetThemeColor(COLOR_WINDOWTEXT);
				}

				rc = pdis->rcItem;
				rc.left += m_ListMargin;
				const int y = rc.top + ((rc.bottom - rc.top) - m_IconHeight) / 2;

				int IconWidth, IconHeight;
				::ImageList_GetIconSize(m_himlIcons, &IconWidth, &IconHeight);
				if (IconWidth == m_IconWidth && IconHeight == m_IconHeight) {
					::ImageList_Draw(m_himlIcons, static_cast<int>(pdis->itemData), pdis->hDC, rc.left, y, ILD_TRANSPARENT);
					if (fSelected)
						::ImageList_Draw(m_himlIcons, static_cast<int>(pdis->itemData), pdis->hDC, rc.left, y, ILD_TRANSPARENT);
				} else {
					const HICON hicon = ::ImageList_ExtractIcon(nullptr, m_himlIcons, static_cast<int>(pdis->itemData));
#if 0				// DrawIconEx で描画すると汚い
					::DrawIconEx(pdis->hDC, rc.left, y, hicon, m_IconWidth, m_IconHeight, 0, nullptr, DI_NORMAL);
					if (fSelected)
						::DrawIconEx(pdis->hDC, rc.left, y, hicon, m_IconWidth, m_IconHeight, 0, nullptr, DI_NORMAL);
#else
					ICONINFO ii;
					if (::GetIconInfo(hicon, &ii)) {
						const HBITMAP hbm = static_cast<HBITMAP>(::CopyImage(ii.hbmColor, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
						::DeleteObject(ii.hbmColor);
						::DeleteObject(ii.hbmMask);
						{
							Graphics::CImage Image;
							Image.CreateFromBitmap(hbm);
							Graphics::CCanvas Canvas(pdis->hDC);
							Canvas.SetComposition(true);
							Canvas.DrawImage(rc.left, y, m_IconWidth, m_IconHeight, &Image, 0, 0, IconWidth, IconHeight);
							if (fSelected)
								Canvas.DrawImage(rc.left, y, m_IconWidth, m_IconHeight, &Image, 0, 0, IconWidth, IconHeight);
						}
						::DeleteObject(hbm);
					}
#endif
					::DestroyIcon(hicon);
				}

				const COLORREF crOldText = ::SetTextColor(pdis->hDC, crText);
				const int OldBkMode = ::SetBkMode(pdis->hDC, TRANSPARENT);
				rc.left += m_IconWidth + m_IconTextMargin;
				::DrawText(
					pdis->hDC, m_PageList[pdis->itemData].pszTitle, -1,
					&rc, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
				::SetTextColor(pdis->hDC, crOldText);
				::SetBkMode(pdis->hDC, OldBkMode);
				if ((pdis->itemState & (ODS_FOCUS | ODS_NOFOCUSRECT)) == ODS_FOCUS) {
					rc = pdis->rcItem;
					rc.left += m_ListMargin + m_IconWidth + m_IconTextMargin / 2;
					::DrawFocusRect(pdis->hDC, &rc);
				}
			} else if (wParam == IDC_OPTIONS_TITLE) {
				DrawUtil::FillGradient(
					pdis->hDC, &pdis->rcItem, RGB(0, 0, 0), GetTitleColor(m_CurrentPage));
				const HFONT hfontOld = SelectFont(pdis->hDC, m_TitleFont.GetHandle());
				const COLORREF crOldText = ::SetTextColor(pdis->hDC, RGB(255, 255, 255));
				const int OldBkMode = ::SetBkMode(pdis->hDC, TRANSPARENT);
				RECT rc = {
					pdis->rcItem.left + 2,
					pdis->rcItem.top,
					pdis->rcItem.right - 2,
					pdis->rcItem.bottom
				};
				::DrawText(
					pdis->hDC, m_PageList[m_CurrentPage].pszTitle, -1,
					&rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
				SelectFont(pdis->hDC, hfontOld);
				::SetTextColor(pdis->hDC, crOldText);
				::SetBkMode(pdis->hDC, OldBkMode);
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OPTIONS_LIST:
			if (HIWORD(wParam) == LBN_SELCHANGE) {
				const int NewPage = static_cast<int>(DlgListBox_GetCurSel(hDlg, IDC_OPTIONS_LIST));

				if (NewPage >= 0)
					SetPage(NewPage);
			}
			return TRUE;

		case IDC_OPTIONS_HELP:
			GetAppClass().UICore.ShowHelpContent(m_PageList[m_CurrentPage].HelpID);
			return TRUE;

		case IDOK:
		case IDCANCEL:
			{
				const HCURSOR hcurOld = ::SetCursor(::LoadCursor(nullptr, IDC_WAIT));
				NMHDR nmh;

				nmh.code = LOWORD(wParam) == IDOK ? PSN_APPLY : PSN_RESET;
				m_fSettingError = false;
				for (int i = 0; i < NUM_PAGES; i++) {
					if (m_PageList[i].pOptions->IsCreated()) {
						m_PageList[i].pOptions->SendMessage(WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&nmh));
						if (m_fSettingError) {
							::SetCursor(hcurOld);
							return TRUE;
						}
						if (LOWORD(wParam) == IDOK)
							m_fApplied = true;
					}
				}
				::SetCursor(hcurOld);
				::EndDialog(hDlg, m_fApplied ? IDOK : IDCANCEL);
			}
			return TRUE;
		}
		return TRUE;

	case WM_SIZE:
		SetPagePos(m_CurrentPage);
		InvalidateDlgItem(hDlg, IDC_OPTIONS_TITLE, false);
		return TRUE;

	case WM_DESTROY:
		if (m_himlIcons != nullptr) {
			::ImageList_Destroy(m_himlIcons);
			m_himlIcons = nullptr;
		}
		m_TitleFont.Destroy();
		return TRUE;
	}

	return FALSE;
}


void COptionDialog::ApplyStyle()
{
	CResizableDialog::ApplyStyle();

	if (m_hDlg != nullptr) {
		m_IconWidth = m_pStyleScaling->GetScaledSystemMetrics(SM_CXSMICON);
		m_IconHeight = m_pStyleScaling->GetScaledSystemMetrics(SM_CYSMICON);
		m_ListMargin = m_pStyleScaling->LogicalPixelsToPhysicalPixels(1);
		m_IconTextMargin = m_pStyleScaling->LogicalPixelsToPhysicalPixels(4);

		LOGFONT lf;
		m_Font.GetLogFont(&lf);
		lf.lfWeight = FW_BOLD;
		m_TitleFont.Create(&lf);

		const HDC hdc = ::GetDC(m_hDlg);
		const int FontHeight = m_Font.GetHeight(hdc, false);
		::ReleaseDC(m_hDlg, hdc);

		m_ListItemHeight = std::max(FontHeight, m_IconHeight) + m_ListMargin * 2;

		if (m_himlIcons != nullptr)
			::ImageList_Destroy(m_himlIcons);
		m_himlIcons = ::ImageList_LoadImage(
			GetAppClass().GetResourceInstance(),
			m_IconWidth <= 16 ? MAKEINTRESOURCE(IDB_OPTIONS16) : MAKEINTRESOURCE(IDB_OPTIONS32),
			m_IconWidth <= 16 ? 16 : 32,
			1, CLR_NONE, IMAGE_BITMAP, LR_CREATEDIBSECTION);
	}
}


void COptionDialog::RealizeStyle()
{
	CResizableDialog::RealizeStyle();

	if (m_hDlg != nullptr) {
		if (DlgListBox_GetCount(m_hDlg, IDC_OPTIONS_LIST) > 0) {
			DlgListBox_SetItemHeight(m_hDlg, IDC_OPTIONS_LIST, 0, m_ListItemHeight);

			for (int i = 0; i < NUM_PAGES; i++)
				SetPagePos(i);
		}
	}
}


void COptionDialog::ActivatePage(COptions *pOptions)
{
	for (int i = 0; i < NUM_PAGES; i++) {
		if (m_PageList[i].pOptions == pOptions) {
			SetPage(i);
			break;
		}
	}
}


void COptionDialog::OnSettingError(COptions *pOptions)
{
	for (int i = 0; i < NUM_PAGES; i++) {
		if (m_PageList[i].pOptions == pOptions) {
			SetPage(i);
			m_fSettingError = true;
			break;
		}
	}
}


}	// namespace TVTest
