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
#include "ViewOptions.h"
#include "AppMain.h"
#include "DialogUtil.h"
#include "StyleUtil.h"
#include "StringUtility.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


static const struct {
	LPCTSTR pszDescript;
	LPCTSTR pszFormat;
} TitleTextFormatPresets[] =
{
	{
		TEXT("サービス名 / 番組時間 番組名 - ") APP_NAME,
		TEXT("%rec-circle% %service-name% %sep-slash% %event-time% %event-name% %sep-hyphen% ") APP_NAME
	},
	{
		TEXT("サービス名 / 番組名 - ") APP_NAME,
		TEXT("%rec-circle% %service-name% %sep-slash% %event-name% %sep-hyphen% ") APP_NAME
	},
};


// 旧仕様互換用
// ver.0.9.0 開発途中の変更なので長く残さなくていい
static void TitleFormatMakeCompatible(String &Str)
{
	if (Str.find(L"%event-sep%") != String::npos) {
		StringUtility::Replace(Str, L"%event-sep%", L"%sep-slash%");
		StringUtility::Replace(Str, L"- TVTest", L"%sep-hyphen% " APP_NAME);
	}
}


CViewOptions::CViewOptions()
	: m_TitleTextFormat(TitleTextFormatPresets[0].pszFormat)
{
	StyleUtil::GetSystemFont(DrawUtil::FontType::Caption, &m_TitleBarFont);
}


CViewOptions::~CViewOptions()
{
	Destroy();
}


bool CViewOptions::Apply(DWORD Flags)
{
	CAppMain &App = GetAppClass();

	if ((Flags & UPDATE_LOGO) != 0) {
		App.UICore.SetLogo(m_fShowLogo ? m_LogoFileName.c_str() : nullptr);
	}

	return true;
}


bool CViewOptions::ReadSettings(CSettings &Settings)
{
	int Value;

	Settings.Read(TEXT("AdjustAspectResizing"), &m_fAdjustAspectResizing);
	Settings.Read(TEXT("SnapToWindowEdge"), &m_fSnapAtWindowEdge);
	Settings.Read(TEXT("SupportAeroSnap"), &m_fSupportAeroSnap);
	Settings.Read(TEXT("NearCornerResizeOrigin"), &m_fNearCornerResizeOrigin);
	Settings.Read(TEXT("ZoomKeepAspectRatio"), &m_fZoomKeepAspectRatio);
	if (Settings.Read(TEXT("PanScanAdjustWindow"), &Value)
			&& CheckEnumRange(static_cast<AdjustWindowMode>(Value))) {
		m_PanScanAdjustWindowMode = static_cast<AdjustWindowMode>(Value);
	} else {
		// 以前のバージョンとの互換用
		bool f;
		if (Settings.Read(TEXT("PanScanNoResizeWindow"), &f))
			m_PanScanAdjustWindowMode = f ? AdjustWindowMode::Width : AdjustWindowMode::Fit;
	}
	Settings.Read(TEXT("Remember1SegWindowSize"), &m_fRemember1SegWindowSize);
	Settings.Read(TEXT("MinimizeToTray"), &m_fMinimizeToTray);
	Settings.Read(TEXT("DisablePreviewWhenMinimized"), &m_fDisablePreviewWhenMinimized);
	Settings.Read(TEXT("HideCursor"), &m_fHideCursor);
	Settings.Read(TEXT("UseLogoIcon"), &m_fUseLogoIcon);
	Settings.Read(TEXT("TitleTextFormat"), &m_TitleTextFormat);
	TitleFormatMakeCompatible(m_TitleTextFormat);
	Settings.Read(TEXT("MinimizedTitleTextFormat"), &m_MinimizedTitleTextFormat);
	TitleFormatMakeCompatible(m_MinimizedTitleTextFormat);
	Settings.Read(TEXT("MaximizedTitleTextFormat"), &m_MaximizedTitleTextFormat);
	TitleFormatMakeCompatible(m_MaximizedTitleTextFormat);
	Settings.Read(TEXT("TaskbarTitleTextFormat"), &m_TaskbarTitleTextFormat);
	TitleFormatMakeCompatible(m_TaskbarTitleTextFormat);
	Settings.Read(TEXT("EnableTitleBarFont"), &m_fEnableTitleBarFont);
	StyleUtil::ReadFontSettings(Settings, TEXT("TitleBarFont"), &m_TitleBarFont);
	Settings.Read(TEXT("ShowLogo"), &m_fShowLogo);
	Settings.Read(TEXT("LogoFileName"), &m_LogoFileName);

	return true;
}


bool CViewOptions::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("AdjustAspectResizing"), m_fAdjustAspectResizing);
	Settings.Write(TEXT("SnapToWindowEdge"), m_fSnapAtWindowEdge);
	Settings.Write(TEXT("SupportAeroSnap"), m_fSupportAeroSnap);
	Settings.Write(TEXT("NearCornerResizeOrigin"), m_fNearCornerResizeOrigin);
	Settings.Write(TEXT("ZoomKeepAspectRatio"), m_fZoomKeepAspectRatio);
	Settings.Write(TEXT("PanScanAdjustWindow"), static_cast<int>(m_PanScanAdjustWindowMode));
	Settings.Write(TEXT("Remember1SegWindowSize"), m_fRemember1SegWindowSize);
	Settings.Write(TEXT("MinimizeToTray"), m_fMinimizeToTray);
	Settings.Write(TEXT("DisablePreviewWhenMinimized"), m_fDisablePreviewWhenMinimized);
	Settings.Write(TEXT("HideCursor"), m_fHideCursor);
	Settings.Write(TEXT("UseLogoIcon"), m_fUseLogoIcon);
	Settings.Write(TEXT("TitleTextFormat"), m_TitleTextFormat);
	// 設定UI未実装
	/*
	Settings.Write(TEXT("MinimizedTitleTextFormat"), m_MinimizedTitleTextFormat);
	Settings.Write(TEXT("MaximizedTitleTextFormat"), m_MaximizedTitleTextFormat);
	Settings.Write(TEXT("TaskbarTitleTextFormat"), m_TaskbarTitleTextFormat);
	*/
	Settings.DeleteValue(TEXT("TitleEventTime"));
	Settings.Write(TEXT("EnableTitleBarFont"), m_fEnableTitleBarFont);
	StyleUtil::WriteFontSettings(Settings, TEXT("TitleBarFont"), m_TitleBarFont);
	Settings.Write(TEXT("ShowLogo"), m_fShowLogo);
	Settings.Write(TEXT("LogoFileName"), m_LogoFileName);

	return true;
}


bool CViewOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(
		hwndOwner,
		GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_OPTIONS_VIEW));
}


INT_PTR CViewOptions::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			// ウィンドウ設定
			DlgCheckBox_Check(
				hDlg, IDC_OPTIONS_SNAPATWINDOWEDGE,
				m_fSnapAtWindowEdge);
			DlgCheckBox_Check(
				hDlg, IDC_OPTIONS_SUPPORTAEROSNAP,
				m_fSupportAeroSnap);
			DlgCheckBox_Check(
				hDlg, IDC_OPTIONS_ADJUSTASPECTRESIZING,
				m_fAdjustAspectResizing);
			DlgCheckBox_Check(
				hDlg, IDC_OPTIONS_NEARCORNERRESIZEORIGIN,
				m_fNearCornerResizeOrigin);
			DlgCheckBox_Check(hDlg, IDC_OPTIONS_ZOOMKEEPASPECTRATIO, m_fZoomKeepAspectRatio);
			{
				static const LPCTSTR AdjustWindowModeList[] = {
					TEXT("サイズを変えない"),
					TEXT("幅のみ変える"),
					TEXT("幅と高さを変える"),
				};
				for (const LPCTSTR e : AdjustWindowModeList) {
					DlgComboBox_AddString(hDlg, IDC_OPTIONS_PANSCANADJUSTWINDOW, e);
				}
				DlgComboBox_SetCurSel(
					hDlg, IDC_OPTIONS_PANSCANADJUSTWINDOW,
					static_cast<int>(m_PanScanAdjustWindowMode));
			}
			DlgCheckBox_Check(hDlg, IDC_OPTIONS_REMEMBER1SEGWINDOWSIZE, m_fRemember1SegWindowSize);
			DlgCheckBox_Check(hDlg, IDC_OPTIONS_MINIMIZETOTRAY, m_fMinimizeToTray);
			DlgCheckBox_Check(hDlg, IDC_OPTIONS_MINIMIZEDISABLEPREVIEW, m_fDisablePreviewWhenMinimized);
			DlgCheckBox_Check(hDlg, IDC_OPTIONS_HIDECURSOR, m_fHideCursor);
			DlgCheckBox_Check(hDlg, IDC_OPTIONS_USELOGOICON, m_fUseLogoIcon);
			::SetDlgItemText(hDlg, IDC_OPTIONS_TITLETEXTFORMAT, m_TitleTextFormat.c_str());
			InitDropDownButton(hDlg, IDC_OPTIONS_TITLETEXTFORMAT_PARAMETERS);
			InitDropDownButtonWithText(hDlg, IDC_OPTIONS_TITLETEXTFORMAT_PRESETS);

			m_CurTitleBarFont = m_TitleBarFont;
			DlgCheckBox_Check(hDlg, IDC_OPTIONS_TITLEBARFONT_ENABLE, m_fEnableTitleBarFont);
			StyleUtil::SetFontInfoItem(hDlg, IDC_OPTIONS_TITLEBARFONT_INFO, m_CurTitleBarFont);
			EnableDlgItems(
				hDlg,
				IDC_OPTIONS_TITLEBARFONT_INFO,
				IDC_OPTIONS_TITLEBARFONT_CHOOSE,
				m_fEnableTitleBarFont);

			DlgCheckBox_Check(hDlg, IDC_OPTIONS_SHOWLOGO, m_fShowLogo);
			::SetDlgItemText(hDlg, IDC_OPTIONS_LOGOFILENAME, m_LogoFileName.c_str());
			::SendDlgItemMessage(hDlg, IDC_OPTIONS_LOGOFILENAME, EM_LIMITTEXT, MAX_PATH - 1, 0);
			EnableDlgItems(
				hDlg, IDC_OPTIONS_LOGOFILENAME, IDC_OPTIONS_LOGOFILENAME_BROWSE,
				m_fShowLogo);

			AddControls({
				{IDC_OPTIONS_TITLETEXTFORMAT,            AlignFlag::Horz},
				{IDC_OPTIONS_TITLETEXTFORMAT_PARAMETERS, AlignFlag::Right},
			});
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OPTIONS_TITLETEXTFORMAT_PARAMETERS:
			{
				RECT rc;
				::GetWindowRect(::GetDlgItem(hDlg, IDC_OPTIONS_TITLETEXTFORMAT_PARAMETERS), &rc);
				CUICore::CTitleStringMap StrMap(GetAppClass());
				StrMap.InputParameter(hDlg, IDC_OPTIONS_TITLETEXTFORMAT, rc);
			}
			return TRUE;

		case IDC_OPTIONS_TITLETEXTFORMAT_PRESETS:
			{
				RECT rc;
				::GetWindowRect(::GetDlgItem(hDlg, IDC_OPTIONS_TITLETEXTFORMAT_PRESETS), &rc);
				CPopupMenu Menu;
				Menu.Create();
				for (int i = 0; i < lengthof(TitleTextFormatPresets); i++)
					Menu.Append(i + 1, TitleTextFormatPresets[i].pszDescript);
				const POINT pt = {rc.left, rc.bottom};
				const int Result = Menu.Show(hDlg, &pt, TPM_RETURNCMD, &rc);
				if (Result > 0 && Result <= lengthof(TitleTextFormatPresets)) {
					::SetDlgItemText(
						hDlg, IDC_OPTIONS_TITLETEXTFORMAT,
						TitleTextFormatPresets[Result - 1].pszFormat);
				}
			}
			return TRUE;

		case IDC_OPTIONS_TITLEBARFONT_ENABLE:
			EnableDlgItemsSyncCheckBox(
				hDlg,
				IDC_OPTIONS_TITLEBARFONT_INFO,
				IDC_OPTIONS_TITLEBARFONT_CHOOSE,
				IDC_OPTIONS_TITLEBARFONT_ENABLE);
			return TRUE;

		case IDC_OPTIONS_TITLEBARFONT_CHOOSE:
			if (StyleUtil::ChooseStyleFont(hDlg, &m_CurTitleBarFont))
				StyleUtil::SetFontInfoItem(hDlg, IDC_OPTIONS_TITLEBARFONT_INFO, m_CurTitleBarFont);
			return TRUE;

		case IDC_OPTIONS_SHOWLOGO:
			EnableDlgItemsSyncCheckBox(
				hDlg, IDC_OPTIONS_LOGOFILENAME, IDC_OPTIONS_LOGOFILENAME_BROWSE,
				IDC_OPTIONS_SHOWLOGO);
			return TRUE;

		case IDC_OPTIONS_LOGOFILENAME_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH];
				String FileName, InitDir;

				::GetDlgItemText(hDlg, IDC_OPTIONS_LOGOFILENAME, szFileName, lengthof(szFileName));
				if (PathUtil::Split(szFileName, &InitDir, &FileName)) {
					StringCopy(szFileName, FileName.c_str());
				} else {
					GetAppClass().GetAppDirectory(&InitDir);
				}
				InitOpenFileName(&ofn);
				ofn.hwndOwner = hDlg;
				ofn.lpstrFilter =
					TEXT("BMPファイル(*.bmp)\0*.bmp\0")
					TEXT("すべてのファイル\0*.*\0");
				ofn.lpstrFile = szFileName;
				ofn.nMaxFile = lengthof(szFileName);
				ofn.lpstrInitialDir = InitDir.c_str();
				ofn.lpstrTitle = TEXT("ロゴ画像の選択");
				ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_EXPLORER;
				if (FileOpenDialog(&ofn)) {
					::SetDlgItemText(hDlg, IDC_OPTIONS_LOGOFILENAME, szFileName);
				}
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case PSN_APPLY:
			{
				CAppMain &App = GetAppClass();
				bool f;

				m_fSnapAtWindowEdge =
					DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_SNAPATWINDOWEDGE);
				m_fSupportAeroSnap =
					DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_SUPPORTAEROSNAP);
				m_fAdjustAspectResizing =
					DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_ADJUSTASPECTRESIZING);
				m_fNearCornerResizeOrigin =
					DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_NEARCORNERRESIZEORIGIN);
				m_fZoomKeepAspectRatio =
					DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_ZOOMKEEPASPECTRATIO);
				m_PanScanAdjustWindowMode =
					static_cast<AdjustWindowMode>(DlgComboBox_GetCurSel(hDlg, IDC_OPTIONS_PANSCANADJUSTWINDOW));
				m_fRemember1SegWindowSize =
					DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_REMEMBER1SEGWINDOWSIZE);
				m_fMinimizeToTray =
					DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_MINIMIZETOTRAY);
				m_fDisablePreviewWhenMinimized =
					DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_MINIMIZEDISABLEPREVIEW);
				m_fHideCursor =
					DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_HIDECURSOR);
				f = DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_USELOGOICON);
				if (m_fUseLogoIcon != f) {
					m_fUseLogoIcon = f;
					App.UICore.UpdateIcon();
				}
				{
					String Text;
					GetDlgItemString(hDlg, IDC_OPTIONS_TITLETEXTFORMAT, &Text);
					if (m_TitleTextFormat != Text) {
						m_TitleTextFormat = std::move(Text);
						App.UICore.UpdateTitle();
					}
				}
				{
					const bool fLogo = DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_SHOWLOGO);
					String FileName;

					GetDlgItemString(hDlg, IDC_OPTIONS_LOGOFILENAME, &FileName);
					if ((fLogo != m_fShowLogo) || (m_LogoFileName != FileName)) {
						m_fShowLogo = fLogo;
						m_LogoFileName = FileName;
						SetUpdateFlag(UPDATE_LOGO);
					}
				}

				bool fTitleBarFontChanged = false;
				const bool fEnableTitleBarFont =
					DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_TITLEBARFONT_ENABLE);
				if (m_fEnableTitleBarFont != fEnableTitleBarFont) {
					m_fEnableTitleBarFont = fEnableTitleBarFont;
					fTitleBarFontChanged = true;
				}
				if (m_TitleBarFont != m_CurTitleBarFont) {
					m_TitleBarFont = m_CurTitleBarFont;
					if (m_fEnableTitleBarFont)
						fTitleBarFontChanged = true;
				}
				if (fTitleBarFontChanged) {
					Style::Font Font;
					if (m_fEnableTitleBarFont)
						Font = m_TitleBarFont;
					else
						StyleUtil::GetSystemFont(DrawUtil::FontType::Caption, &Font);
					App.UICore.SetTitleFont(Font);
					App.Panel.Frame.GetPanel()->SetTitleFont(Font);
				}

				m_fChanged = true;
			}
			break;
		}
		break;
	}

	return FALSE;
}


} // namespace TVTest
