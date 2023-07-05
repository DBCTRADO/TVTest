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
#include <algorithm>
#include "TVTest.h"
#include "AppMain.h"
#include "ColorSchemeOptions.h"
#include "Settings.h"
#include "DialogUtil.h"
#include "DrawUtil.h"
#include "ThemeManager.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


class CColorSchemeSaveDialog
	: public CBasicDialog
{
public:
	CColorSchemeSaveDialog(LPTSTR pszName)
		: m_pszName(pszName)
	{
	}

// CBasicDialog
	bool Show(HWND hwndOwner) override;

private:
	LPTSTR m_pszName;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
};


bool CColorSchemeSaveDialog::Show(HWND hwndOwner)
{
	return ShowDialog(
		hwndOwner,
		GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE(IDD_SAVECOLORSCHEME)) == IDOK;
}


INT_PTR CColorSchemeSaveDialog::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		DlgEdit_SetText(hDlg, IDC_SAVECOLORSCHEME_NAME, m_pszName);
		DlgEdit_LimitText(hDlg, IDC_SAVECOLORSCHEME_NAME, MAX_COLORSCHEME_NAME);
		EnableDlgItem(hDlg, IDOK, m_pszName[0] != _T('\0'));
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SAVECOLORSCHEME_NAME:
			if (HIWORD(wParam) == EN_CHANGE) {
				EnableDlgItem(hDlg, IDOK, GetDlgItemTextLength(hDlg, IDC_SAVECOLORSCHEME_NAME) > 0);
			}
			return TRUE;

		case IDOK:
			::GetDlgItemText(hDlg, IDC_SAVECOLORSCHEME_NAME, m_pszName, MAX_COLORSCHEME_NAME);
			if (m_pszName[0] == _T('\0'))
				return TRUE;
			[[fallthrough]];
		case IDCANCEL:
			::EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		return TRUE;
	}

	return FALSE;
}




const LPCTSTR CColorSchemeOptions::m_pszExtension = TEXT(".httheme");


CColorSchemeOptions::CColorSchemeOptions()
	: m_ColorScheme(std::make_unique<CColorScheme>())
{
}


CColorSchemeOptions::~CColorSchemeOptions()
{
	Destroy();
}


bool CColorSchemeOptions::LoadSettings(CSettings &Settings)
{
	if (!m_ColorScheme->Load(Settings))
		return false;

	// ver.0.9.0より前にあった「映像表示部の枠」の設定を反映させる
	if (m_ColorScheme->GetBorderType(CColorScheme::BORDER_SCREEN) == Theme::BorderType::Sunken
			&& Settings.SetSection(TEXT("Settings"))) {
		bool fClientEdge;

		if (Settings.Read(TEXT("ClientEdge"), &fClientEdge) && !fClientEdge)
			m_ColorScheme->SetBorderType(CColorScheme::BORDER_SCREEN, Theme::BorderType::None);
	}

	return true;
}


bool CColorSchemeOptions::SaveSettings(CSettings &Settings)
{
	if (!m_ColorScheme->Save(
				Settings,
				CColorScheme::SaveFlag::NoDefault |
				CColorScheme::SaveFlag::NoName))
		return false;

	if (Settings.SetSection(TEXT("Settings")))
		Settings.DeleteValue(TEXT("ClientEdge"));

	return true;
}


bool CColorSchemeOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(
		hwndOwner,
		GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_OPTIONS_COLORSCHEME));
}


bool CColorSchemeOptions::SetEventHandler(CEventHandler *pEventHandler)
{
	m_pEventHandler = pEventHandler;
	return true;
}


bool CColorSchemeOptions::ApplyColorScheme() const
{
	return Apply(m_ColorScheme.get());
}


bool CColorSchemeOptions::Apply(const CColorScheme *pColorScheme) const
{
	if (m_pEventHandler == nullptr)
		return false;
	return m_pEventHandler->ApplyColorScheme(pColorScheme);
}


COLORREF CColorSchemeOptions::GetColor(int Type) const
{
	if (m_PreviewColorScheme)
		return m_PreviewColorScheme->GetColor(Type);
	return m_ColorScheme->GetColor(Type);
}


COLORREF CColorSchemeOptions::GetColor(LPCTSTR pszText) const
{
	if (m_PreviewColorScheme)
		return m_PreviewColorScheme->GetColor(pszText);
	return m_ColorScheme->GetColor(pszText);
}


void CColorSchemeOptions::GetCurrentSettings(CColorScheme *pColorScheme)
{
	for (int i = 0; i < CColorScheme::NUM_COLORS; i++)
		pColorScheme->SetColor(i, static_cast<COLORREF>(DlgListBox_GetItemData(m_hDlg, IDC_COLORSCHEME_LIST, i)));
	for (int i = 0; i < CColorScheme::NUM_GRADIENTS; i++)
		pColorScheme->SetGradientStyle(i, m_GradientList[i]);
	for (int i = 0; i < CColorScheme::NUM_BORDERS; i++)
		pColorScheme->SetBorderType(i, m_BorderList[i]);
}


bool CColorSchemeOptions::GetThemesDirectory(CFilePath *pDirectory, bool fCreate)
{
	GetAppClass().GetAppDirectory(pDirectory);
	pDirectory->Append(TEXT("Themes"));
	if (fCreate && !::PathIsDirectory(pDirectory->c_str()))
		::CreateDirectory(pDirectory->c_str(), nullptr);
	return true;
}


INT_PTR CColorSchemeOptions::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CFilePath Directory;

			GetThemesDirectory(&Directory);
			m_PresetList.Load(Directory.c_str());
			//m_PresetList.SortByName();
			CColorScheme *pDefaultColorScheme = new CColorScheme;
			pDefaultColorScheme->SetName(TEXT("デフォルトのテーマ"));
			pDefaultColorScheme->SetLoaded();
			m_PresetList.Insert(0, pDefaultColorScheme);
			int CurPreset = -1;
			for (int i = 0; i < m_PresetList.NumColorSchemes(); i++) {
				if (m_ColorScheme->CompareScheme(*m_PresetList.GetColorScheme(i))) {
					CurPreset = i;
					break;
				}
			}
			if (CurPreset < 0) {
				CColorScheme *pColorScheme = new CColorScheme(*m_ColorScheme.get());
				pColorScheme->SetName(TEXT("現在のテーマ"));
				pColorScheme->SetLoaded();
				m_PresetList.Insert(0, pColorScheme);
			}
			for (int i = 0; i < m_PresetList.NumColorSchemes(); i++) {
				DlgComboBox_AddItem(hDlg, IDC_COLORSCHEME_PRESET, i);
			}
			DlgComboBox_SetCurSel(hDlg, IDC_COLORSCHEME_PRESET, CurPreset < 0 ? 0 : CurPreset);
			EnableDlgItem(hDlg, IDC_COLORSCHEME_DELETE, CurPreset > 0);

			for (int i = 0; i < CColorScheme::NUM_COLORS; i++) {
				DlgListBox_AddItem(hDlg, IDC_COLORSCHEME_LIST, m_ColorScheme->GetColor(i));
			}
			ExtendListBox(GetDlgItem(hDlg, IDC_COLORSCHEME_LIST));

			SetListItemSize();

			for (int i = 0; i < CColorScheme::NUM_GRADIENTS; i++)
				m_ColorScheme->GetGradientStyle(i, &m_GradientList[i]);
			for (int i = 0; i < CColorScheme::NUM_BORDERS; i++)
				m_BorderList[i] = m_ColorScheme->GetBorderType(i);

			RECT rc;
			static const RGBQUAD BaseColors[18] = {
				{0x00, 0x00, 0xFF},
				{0x00, 0x66, 0xFF},
				{0x00, 0xCC, 0xFF},
				{0x00, 0xFF, 0xFF},
				{0x00, 0xFF, 0xCC},
				{0x00, 0xFF, 0x66},
				{0x00, 0xFF, 0x00},
				{0x66, 0xFF, 0x00},
				{0xCC, 0xFF, 0x00},
				{0xFF, 0xFF, 0x00},
				{0xFF, 0xCC, 0x00},
				{0xFF, 0x66, 0x00},
				{0xFF, 0x00, 0x00},
				{0xFF, 0x00, 0x66},
				{0xFF, 0x00, 0xCC},
				{0xFF, 0x00, 0xFF},
				{0xCC, 0x00, 0xFF},
				{0x66, 0x00, 0xFF},
			};
			RGBQUAD Palette[256];

			CColorPalette::Initialize(GetWindowInstance(hDlg));
			m_ColorPalette.Create(hDlg, WS_CHILD | WS_VISIBLE, 0, IDC_COLORSCHEME_PALETTE);
			m_ColorPalette.SetTooltipFont(GetWindowFont(hDlg));
			if (m_fDarkMode)
				m_ColorPalette.SetBackColor(GetThemeColor(COLOR_3DFACE));
			GetWindowRect(GetDlgItem(hDlg, IDC_COLORSCHEME_PALETTEPLACE), &rc);
			MapWindowPoints(nullptr, hDlg, reinterpret_cast<LPPOINT>(&rc), 2);
			m_ColorPalette.SetPosition(&rc);
			for (int i = 0; i < lengthof(BaseColors); i++) {
				const RGBQUAD Color = BaseColors[i % 2 * (lengthof(BaseColors) / 2) + i / 2];
				int j;

				for (j = 0; j < 4; j++) {
					Palette[i * 8 + j].rgbBlue = (Color.rgbBlue * (j + 1)) / 4;
					Palette[i * 8 + j].rgbGreen = (Color.rgbGreen * (j + 1)) / 4;
					Palette[i * 8 + j].rgbRed = (Color.rgbRed * (j + 1)) / 4;
				}
				for (; j < 8; j++) {
					Palette[i * 8 + j].rgbBlue = Color.rgbBlue + (255 - Color.rgbBlue) * (j - 3) / 5;
					Palette[i * 8 + j].rgbGreen = Color.rgbGreen + (255 - Color.rgbGreen) * (j - 3) / 5;
					Palette[i * 8 + j].rgbRed = Color.rgbRed + (255 - Color.rgbRed) * (j - 3) / 5;
				}
			}
			int i = lengthof(BaseColors) * 8;
			for (int j = 0; j < 16; j++) {
				Palette[i].rgbBlue = (255 * j) / 15;
				Palette[i].rgbGreen = (255 * j) / 15;
				Palette[i].rgbRed = (255 * j) / 15;
				i++;
			}
			for (int j = 0; j < CColorScheme::NUM_COLORS; j++) {
				const COLORREF cr = m_ColorScheme->GetColor(j);
				int k;

				for (k = 0; k < i; k++) {
					if (cr == RGB(Palette[k].rgbRed, Palette[k].rgbGreen, Palette[k].rgbBlue))
						break;
				}
				if (k == i) {
					Palette[i].rgbBlue = GetBValue(cr);
					Palette[i].rgbGreen = GetGValue(cr);
					Palette[i].rgbRed = GetRValue(cr);
					i++;
				}
			}
			if (i < lengthof(Palette))
				ZeroMemory(&Palette[i], (lengthof(Palette) - i) * sizeof(RGBQUAD));
			m_ColorPalette.SetPalette(Palette, lengthof(Palette));

			AddControls({
				{IDC_COLORSCHEME_LIST,         AlignFlag::All},
				{IDC_COLORSCHEME_PALETTE,      AlignFlag::Right},
				{IDC_COLORSCHEME_PALETTEPLACE, AlignFlag::Right},
				{IDC_COLORSCHEME_PREVIEW,      AlignFlag::BottomRight},
			});
		}
		return TRUE;

	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT pdis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

			if (pdis->CtlID == IDC_COLORSCHEME_PRESET) {
				switch (pdis->itemAction) {
				case ODA_DRAWENTIRE:
				case ODA_SELECT:
					if (static_cast<int>(pdis->itemID) < 0) {
						DrawUtil::Fill(pdis->hDC, &pdis->rcItem, GetThemeColor(COLOR_WINDOW));
					} else {
						const CColorScheme *pColorScheme = m_PresetList.GetColorScheme(static_cast<int>(pdis->itemData));
						if (pColorScheme == nullptr)
							break;
						const bool fSelected =
							(pdis->itemState & ODS_SELECTED) != 0
							&& (pdis->itemState & ODS_COMBOBOXEDIT) == 0;
						const Theme::CThemeManager ThemeManager(pColorScheme);
						Theme::CThemeDraw ThemeDraw(BeginThemeDraw(pdis->hDC));
						Theme::Style Style;

						ThemeManager.GetStyle(
							fSelected ?
								Theme::CThemeManager::STYLE_STATUSBAR_ITEM_HOT :
								Theme::CThemeManager::STYLE_STATUSBAR_ITEM,
							&Style);
						ThemeDraw.Draw(Style.Back, pdis->rcItem);
						if (!IsStringEmpty(pColorScheme->GetName())) {
							RECT rc;
							HFONT hfont, hfontOld;

							if (fSelected) {
								LOGFONT lf;

								hfontOld = static_cast<HFONT>(::GetCurrentObject(pdis->hDC, OBJ_FONT));
								::GetObject(hfontOld, sizeof(LOGFONT), &lf);
								lf.lfWeight = FW_BOLD;
								hfont = ::CreateFontIndirect(&lf);
								SelectFont(pdis->hDC, hfont);
							} else {
								hfont = nullptr;
							}
							const int OldBkMode = ::SetBkMode(pdis->hDC, TRANSPARENT);
							rc = pdis->rcItem;
							rc.left += 4;
							ThemeDraw.Draw(
								Style.Fore, rc, pColorScheme->GetName(),
								DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
							::SetBkMode(pdis->hDC, OldBkMode);
							if (hfont != nullptr) {
								::SelectObject(pdis->hDC, hfontOld);
								::DeleteObject(hfont);
							}
						}
					}
					if ((pdis->itemState & ODS_FOCUS) == 0)
						break;
					[[fallthrough]];
				case ODA_FOCUS:
					if ((pdis->itemState & ODS_NOFOCUSRECT) == 0
							&& (pdis->itemState & ODS_COMBOBOXEDIT) != 0)
						::DrawFocusRect(pdis->hDC, &pdis->rcItem);
					break;
				}
			} else if (pdis->CtlID == IDC_COLORSCHEME_LIST) {
				switch (pdis->itemAction) {
				case ODA_DRAWENTIRE:
				case ODA_SELECT:
					{
						int BackSysColor;
						COLORREF TextColor;
						RECT rc;

						if ((pdis->itemState & ODS_SELECTED) == 0) {
							BackSysColor = COLOR_WINDOW;
							TextColor = GetThemeColor(COLOR_WINDOWTEXT);
						} else {
							BackSysColor = COLOR_HIGHLIGHT;
							TextColor = GetThemeColor(COLOR_HIGHLIGHTTEXT);
						}
						const COLORREF BackColor = GetThemeColor(BackSysColor);
						const int Border = CColorScheme::GetColorBorder(static_cast<int>(pdis->itemID));
						if (Border >= 0 && m_BorderList[Border] == Theme::BorderType::None)
							TextColor = MixColor(TextColor, BackColor);
						DrawUtil::Fill(pdis->hDC, &pdis->rcItem, GetThemeColor(BackSysColor));
						const HBRUSH hbr = ::CreateSolidBrush(static_cast<COLORREF>(pdis->itemData));
						const HBRUSH hbrOld = SelectBrush(pdis->hDC, hbr);
						const HPEN hpenOld = SelectPen(pdis->hDC, ::GetStockObject(BLACK_PEN));
						rc.left = pdis->rcItem.left + m_ColorListMargin;
						rc.top = pdis->rcItem.top + m_ColorListMargin;
						rc.bottom = pdis->rcItem.bottom - m_ColorListMargin;
						rc.right = rc.left + (rc.bottom - rc.top) * 2;
						::Rectangle(pdis->hDC, rc.left, rc.top, rc.right, rc.bottom);
						::SelectObject(pdis->hDC, hpenOld);
						::SelectObject(pdis->hDC, hbrOld);
						::DeleteObject(hbr);
						const int OldBkMode = ::SetBkMode(pdis->hDC, TRANSPARENT);
						const COLORREF OldTextColor = ::SetTextColor(pdis->hDC, TextColor);
						rc.left = rc.right + m_ColorListMargin;
						rc.top = pdis->rcItem.top;
						rc.right = pdis->rcItem.right;
						rc.bottom = pdis->rcItem.bottom;
						::DrawText(
							pdis->hDC, CColorScheme::GetColorName(pdis->itemID), -1, &rc,
							DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
						::SetTextColor(pdis->hDC, OldTextColor);
						::SetBkMode(pdis->hDC, OldBkMode);
					}
					if ((pdis->itemState & ODS_FOCUS) == 0)
						break;
					[[fallthrough]];
				case ODA_FOCUS:
					if ((pdis->itemState & ODS_NOFOCUSRECT) == 0)
						::DrawFocusRect(pdis->hDC, &pdis->rcItem);
					break;
				}
			}
		}
		return TRUE;

	case WM_COMPAREITEM:
		{
			const COMPAREITEMSTRUCT *pcis = reinterpret_cast<const COMPAREITEMSTRUCT*>(lParam);

			if (pcis->CtlID == IDC_COLORSCHEME_PRESET) {
				const CColorScheme *pColorScheme1 = m_PresetList.GetColorScheme(static_cast<int>(pcis->itemData1));
				const CColorScheme *pColorScheme2 = m_PresetList.GetColorScheme(static_cast<int>(pcis->itemData2));
				if (pColorScheme1 == nullptr || pColorScheme2 == nullptr)
					return 0;
				if (!pColorScheme1->IsLoadedFromFile() || !pColorScheme2->IsLoadedFromFile())
					return static_cast<int>(pcis->itemData1) - static_cast<int>(pcis->itemData2);
				int Cmp = ::CompareString(
					pcis->dwLocaleId, NORM_IGNORECASE,
					pColorScheme1->GetName(), -1,
					pColorScheme2->GetName(), -1);
				if (Cmp != 0)
					Cmp -= CSTR_EQUAL;
				return Cmp;
			}
		}
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_COLORSCHEME_PRESET:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				const int Sel = static_cast<int>(DlgComboBox_GetCurSel(hDlg, IDC_COLORSCHEME_PRESET));
				const CColorScheme *pColorScheme = nullptr;

				if (Sel >= 0) {
					const int Index = static_cast<int>(DlgComboBox_GetItemData(hDlg, IDC_COLORSCHEME_PRESET, Sel));

					pColorScheme = m_PresetList.GetColorScheme(Index);
					if (pColorScheme != nullptr) {
						for (int i = 0; i < CColorScheme::NUM_COLORS; i++) {
							if (pColorScheme->IsLoaded(i))
								SendDlgItemMessage(
									hDlg, IDC_COLORSCHEME_LIST,
									LB_SETITEMDATA, i, pColorScheme->GetColor(i));
						}
						SendDlgItemMessage(hDlg, IDC_COLORSCHEME_LIST, LB_SETSEL, FALSE, -1);
						InvalidateDlgItem(hDlg, IDC_COLORSCHEME_LIST);

						for (int i = 0; i < CColorScheme::NUM_GRADIENTS; i++)
							pColorScheme->GetGradientStyle(i, &m_GradientList[i]);
						for (int i = 0; i < CColorScheme::NUM_BORDERS; i++)
							m_BorderList[i] = pColorScheme->GetBorderType(i);

						m_ColorPalette.SetSel(-1);
						::SendMessage(hDlg, WM_COMMAND, IDC_COLORSCHEME_PREVIEW, 0);
					}
				}

				EnableDlgItem(
					hDlg, IDC_COLORSCHEME_DELETE,
					pColorScheme != nullptr && pColorScheme->IsLoadedFromFile());
			}
			return TRUE;

		case IDC_COLORSCHEME_SAVE:
			{
				CColorScheme *pColorScheme;
				TCHAR szName[MAX_COLORSCHEME_NAME];
				szName[0] = _T('\0');
				const LRESULT Sel = DlgComboBox_GetCurSel(hDlg, IDC_COLORSCHEME_PRESET);
				pColorScheme = m_PresetList.GetColorScheme(
					static_cast<int>(DlgComboBox_GetItemData(hDlg, IDC_COLORSCHEME_PRESET, Sel)));
				if (pColorScheme != nullptr && pColorScheme->IsLoadedFromFile())
					StringCopy(szName, pColorScheme->GetName());
				CColorSchemeSaveDialog SaveDlg(szName);
				if (!SaveDlg.Show(hDlg))
					return TRUE;

				pColorScheme = nullptr;
				int Index = m_PresetList.FindByName(szName);
				if (Index >= 0) {
					pColorScheme = m_PresetList.GetColorScheme(Index);
				}
				bool fNewColorScheme;
				if (pColorScheme != nullptr && pColorScheme->IsLoadedFromFile()) {
					fNewColorScheme = false;
				} else {
					CFilePath FilePath;
					GetThemesDirectory(&FilePath, true);
					FilePath.Append(szName);
					FilePath +=  m_pszExtension;
					pColorScheme = new CColorScheme;
					pColorScheme->SetFileName(FilePath.c_str());
					fNewColorScheme = true;
				}
				pColorScheme->SetName(szName);
				pColorScheme->SetLoaded();

				GetCurrentSettings(pColorScheme);
				if (!pColorScheme->Save(pColorScheme->GetFileName())) {
					if (fNewColorScheme)
						delete pColorScheme;
					::MessageBox(hDlg, TEXT("保存ができません。"), nullptr, MB_OK | MB_ICONEXCLAMATION);
					break;
				}

				if (fNewColorScheme) {
					m_PresetList.Add(pColorScheme);
					Index = static_cast<int>(DlgComboBox_AddItem(hDlg, IDC_COLORSCHEME_PRESET, m_PresetList.NumColorSchemes() - 1));
				} else {
					InvalidateDlgItem(hDlg, IDC_COLORSCHEME_PRESET);
					const int ItemCount = static_cast<int>(DlgComboBox_GetCount(hDlg, IDC_COLORSCHEME_PRESET));
					for (int i = 0; i < ItemCount; i++) {
						if (DlgComboBox_GetItemData(hDlg, IDC_COLORSCHEME_PRESET, i) == Index) {
							Index = i;
							break;
						}
					}
				}
				DlgComboBox_SetCurSel(hDlg, IDC_COLORSCHEME_PRESET, Index);

				::MessageBox(hDlg, TEXT("テーマを保存しました。"), TEXT("保存"), MB_OK | MB_ICONINFORMATION);
			}
			return TRUE;

		case IDC_COLORSCHEME_DELETE:
			{
				const LRESULT Sel = DlgComboBox_GetCurSel(hDlg, IDC_COLORSCHEME_PRESET);
				const CColorScheme *pColorScheme = m_PresetList.GetColorScheme(
					static_cast<int>(DlgComboBox_GetItemData(hDlg, IDC_COLORSCHEME_PRESET, Sel)));
				if (pColorScheme == nullptr || !pColorScheme->IsLoadedFromFile())
					break;
				if (::MessageBox(
							hDlg, TEXT("選択されたテーマを削除しますか?"), TEXT("削除の確認"),
							MB_OKCANCEL | MB_ICONEXCLAMATION) != IDOK)
					break;
				if (!::DeleteFile(pColorScheme->GetFileName())) {
					::MessageBox(hDlg, TEXT("ファイルを削除できません。"), nullptr, MB_OK | MB_ICONEXCLAMATION);
					break;
				}
				DlgComboBox_DeleteItem(hDlg, IDC_COLORSCHEME_PRESET, Sel);
			}
			return TRUE;

		case IDC_COLORSCHEME_LIST:
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE:
				{
					const int SelCount = static_cast<int>(DlgListBox_GetSelCount(hDlg, IDC_COLORSCHEME_LIST));
					COLORREF SelColor = CLR_INVALID;

					if (SelCount == 0) {
						m_ColorPalette.SetSel(-1);
						break;
					}
					if (SelCount == 1) {
						for (int i = 0; i < CColorScheme::NUM_COLORS; i++) {
							if (DlgListBox_GetSel(hDlg, IDC_COLORSCHEME_LIST, i)) {
								SelColor = static_cast<COLORREF>(DlgListBox_GetItemData(hDlg, IDC_COLORSCHEME_LIST, i));
								break;
							}
						}
					} else {
						int i;
						for (i = 0; i < CColorScheme::NUM_COLORS; i++) {
							if (DlgListBox_GetSel(hDlg, IDC_COLORSCHEME_LIST, i)) {
								const COLORREF Color = static_cast<COLORREF>(DlgListBox_GetItemData(hDlg, IDC_COLORSCHEME_LIST, i));
								if (SelColor == CLR_INVALID)
									SelColor = Color;
								else if (Color != SelColor)
									break;
							}
						}
						if (i < CColorScheme::NUM_COLORS) {
							m_ColorPalette.SetSel(-1);
							break;
						}
					}
					if (SelColor != CLR_INVALID)
						m_ColorPalette.SetSel(m_ColorPalette.FindColor(SelColor));
				}
				break;

			case LBN_EX_RBUTTONUP:
				{
					CPopupMenu Menu(GetAppClass().GetResourceInstance(), IDM_COLORSCHEME);

					Menu.EnableItem(IDC_COLORSCHEME_SELECTSAMECOLOR, m_ColorPalette.GetSel() >= 0);
					if (DlgListBox_GetSelCount(hDlg, IDC_COLORSCHEME_LIST) == 1) {
						int Sel;

						DlgListBox_GetSelItems(hDlg, IDC_COLORSCHEME_LIST, &Sel, 1);
						const int Gradient = CColorScheme::GetColorGradient(Sel);
						if (Gradient >= 0) {
							Menu.EnableSubMenu(2, true);
							Menu.CheckRadioItem(
								IDC_COLORSCHEME_GRADIENT_NORMAL, IDC_COLORSCHEME_GRADIENT_INTERLACED,
								IDC_COLORSCHEME_GRADIENT_NORMAL + static_cast<int>(m_GradientList[Gradient].Type));
							Menu.EnableSubMenu(3, true);
							Menu.CheckRadioItem(
								IDC_COLORSCHEME_DIRECTION_HORZ, IDC_COLORSCHEME_DIRECTION_VERTMIRROR,
								IDC_COLORSCHEME_DIRECTION_HORZ + static_cast<int>(m_GradientList[Gradient].Direction));
							if (!CColorScheme::IsGradientDirectionEnabled(Gradient)) {
								if (m_GradientList[Gradient].Direction == Theme::GradientDirection::Horz
										|| m_GradientList[Gradient].Direction == Theme::GradientDirection::HorzMirror) {
									Menu.EnableItem(IDC_COLORSCHEME_DIRECTION_VERT, false);
									Menu.EnableItem(IDC_COLORSCHEME_DIRECTION_VERTMIRROR, false);
								} else {
									Menu.EnableItem(IDC_COLORSCHEME_DIRECTION_HORZ, false);
									Menu.EnableItem(IDC_COLORSCHEME_DIRECTION_HORZMIRROR, false);
								}
							}
						}
						const int Border = CColorScheme::GetColorBorder(Sel);
						if (Border >= 0) {
							Menu.EnableSubMenu(4, true);
							Menu.CheckRadioItem(
								IDC_COLORSCHEME_BORDER_NONE, IDC_COLORSCHEME_BORDER_RAISED,
								IDC_COLORSCHEME_BORDER_NONE + static_cast<int>(m_BorderList[Border]));
						}
					}

					Menu.Show(hDlg);
				}
				break;
			}
			return TRUE;

		case IDC_COLORSCHEME_PALETTE:
			switch (HIWORD(wParam)) {
			case CColorPalette::NOTIFY_SELCHANGE:
				{
					const int Sel = m_ColorPalette.GetSel();
					const COLORREF Color = m_ColorPalette.GetColor(Sel);

					for (int i = 0; i < CColorScheme::NUM_COLORS; i++) {
						if (DlgListBox_GetSel(hDlg, IDC_COLORSCHEME_LIST, i))
							DlgListBox_SetItemData(hDlg, IDC_COLORSCHEME_LIST, i, Color);
					}
					InvalidateDlgItem(hDlg, IDC_COLORSCHEME_LIST);
				}
				break;

			case CColorPalette::NOTIFY_DOUBLECLICK:
				{
					const int Sel = m_ColorPalette.GetSel();
					COLORREF Color = m_ColorPalette.GetColor(Sel);

					if (ChooseColorDialog(hDlg, &Color)) {
						m_ColorPalette.SetColor(Sel, Color);

						for (int i = 0; i < CColorScheme::NUM_COLORS; i++) {
							if (DlgListBox_GetSel(hDlg, IDC_COLORSCHEME_LIST, i))
								DlgListBox_SetItemData(hDlg, IDC_COLORSCHEME_LIST, i, Color);
						}
						InvalidateDlgItem(hDlg, IDC_COLORSCHEME_LIST);
					}
				}
				break;
			}
			return TRUE;

		case IDC_COLORSCHEME_PREVIEW:
			if (!m_PreviewColorScheme)
				m_PreviewColorScheme = std::make_unique<CColorScheme>();
			GetCurrentSettings(m_PreviewColorScheme.get());
			Apply(m_PreviewColorScheme.get());
			return TRUE;

		case IDC_COLORSCHEME_SELECTSAMECOLOR:
			{
				const int Sel = m_ColorPalette.GetSel();

				if (Sel >= 0) {
					const COLORREF Color = m_ColorPalette.GetColor(Sel);
					const int TopIndex = static_cast<int>(DlgListBox_GetTopIndex(hDlg, IDC_COLORSCHEME_LIST));

					::SendDlgItemMessage(hDlg, IDC_COLORSCHEME_LIST, WM_SETREDRAW, FALSE, 0);
					for (int i = 0; i < CColorScheme::NUM_COLORS; i++) {
						DlgListBox_SetSel(
							hDlg, IDC_COLORSCHEME_LIST, i,
							static_cast<COLORREF>(DlgListBox_GetItemData(hDlg, IDC_COLORSCHEME_LIST, i)) == Color);
					}
					DlgListBox_SetTopIndex(hDlg, IDC_COLORSCHEME_LIST, TopIndex);
					::SendDlgItemMessage(hDlg, IDC_COLORSCHEME_LIST, WM_SETREDRAW, TRUE, 0);
					InvalidateDlgItem(hDlg, IDC_COLORSCHEME_LIST);
				}
			}
			return TRUE;

		case IDC_COLORSCHEME_GRADIENT_NORMAL:
		case IDC_COLORSCHEME_GRADIENT_GLOSSY:
		case IDC_COLORSCHEME_GRADIENT_INTERLACED:
			if (DlgListBox_GetSelCount(hDlg, IDC_COLORSCHEME_LIST) == 1) {
				int Sel;

				DlgListBox_GetSelItems(hDlg, IDC_COLORSCHEME_LIST, &Sel, 1);
				const int Gradient = CColorScheme::GetColorGradient(Sel);
				if (Gradient >= 0) {
					m_GradientList[Gradient].Type =
						static_cast<Theme::GradientType>(LOWORD(wParam) - IDC_COLORSCHEME_GRADIENT_NORMAL);
				}
			}
			return TRUE;

		case IDC_COLORSCHEME_DIRECTION_HORZ:
		case IDC_COLORSCHEME_DIRECTION_VERT:
		case IDC_COLORSCHEME_DIRECTION_HORZMIRROR:
		case IDC_COLORSCHEME_DIRECTION_VERTMIRROR:
			if (DlgListBox_GetSelCount(hDlg, IDC_COLORSCHEME_LIST) == 1) {
				int Sel;

				DlgListBox_GetSelItems(hDlg, IDC_COLORSCHEME_LIST, &Sel, 1);
				const int Gradient = CColorScheme::GetColorGradient(Sel);
				if (Gradient >= 0) {
					m_GradientList[Gradient].Direction =
						static_cast<Theme::GradientDirection>(LOWORD(wParam) - IDC_COLORSCHEME_DIRECTION_HORZ);
				}
			}
			return TRUE;

		case IDC_COLORSCHEME_BORDER_NONE:
		case IDC_COLORSCHEME_BORDER_SOLID:
		case IDC_COLORSCHEME_BORDER_SUNKEN:
		case IDC_COLORSCHEME_BORDER_RAISED:
			if (DlgListBox_GetSelCount(hDlg, IDC_COLORSCHEME_LIST) == 1) {
				int Sel;

				DlgListBox_GetSelItems(hDlg, IDC_COLORSCHEME_LIST, &Sel, 1);
				const int Border = CColorScheme::GetColorBorder(Sel);
				if (Border >= 0) {
					m_BorderList[Border] =
						static_cast<Theme::BorderType>(LOWORD(wParam) - IDC_COLORSCHEME_BORDER_NONE);
					RECT rc;
					::SendDlgItemMessage(
						hDlg, IDC_COLORSCHEME_LIST, LB_GETITEMRECT,
						Sel, reinterpret_cast<LPARAM>(&rc));
					InvalidateDlgItem(hDlg, IDC_COLORSCHEME_LIST, &rc);
				}
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case PSN_APPLY:
			GetCurrentSettings(m_ColorScheme.get());
			Apply(m_ColorScheme.get());
			m_fChanged = true;
			break;

		case PSN_RESET:
			if (m_PreviewColorScheme)
				Apply(m_ColorScheme.get());
			break;
		}
		break;

// 開発用機能
#ifdef _DEBUG
	case WM_RBUTTONUP:
		{
			CPopupMenu Menu;

			Menu.Create();
			Menu.Append(1, TEXT("配色コードをコピー(&C)"));
			Menu.Append(2, TEXT("ボーダー設定をコピー(&B)"));

			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			::ClientToScreen(hDlg, &pt);

			switch (Menu.Show(hDlg, &pt, TPM_RETURNCMD | TPM_RIGHTBUTTON)) {
			case 1:
				{
					String Buffer;

					for (int i = 0; i < CColorScheme::NUM_COLORS; i++) {
						const COLORREF cr = static_cast<COLORREF>(DlgListBox_GetItemData(hDlg, IDC_COLORSCHEME_LIST, i));
						TCHAR szColor[32];
						StringFormat(
							szColor,
							TEXT("HEXRGB(0x{:02X}{:02X}{:02X})\r\n"),
							GetRValue(cr), GetGValue(cr), GetBValue(cr));
						Buffer += szColor;
					}

					CopyTextToClipboard(hDlg, Buffer.c_str());
				}
				break;

			case 2:
				{
					String Buffer;

					for (int i = 0; i < CColorScheme::NUM_BORDERS; i++) {
						switch (m_BorderList[i]) {
						case Theme::BorderType::None:
							Buffer += TEXT("Theme::BorderType::None,\r\n");
							break;
						case Theme::BorderType::Solid:
							Buffer += TEXT("Theme::BorderType::Solid,\r\n");
							break;
						case Theme::BorderType::Sunken:
							Buffer += TEXT("Theme::BorderType::Sunken,\r\n");
							break;
						case Theme::BorderType::Raised:
							Buffer += TEXT("Theme::BorderType::Raised,\r\n");
							break;
						}
					}

					CopyTextToClipboard(hDlg, Buffer.c_str());
				}
				break;
			}
		}
		return TRUE;
#endif

	case WM_DESTROY:
		m_PreviewColorScheme.reset();
		m_PresetList.Clear();
		break;
	}

	return FALSE;
}


void CColorSchemeOptions::OnDarkModeChanged(bool fDarkMode)
{
	m_ColorPalette.SetBackColor(fDarkMode ? GetThemeColor(COLOR_3DFACE) : CLR_INVALID);
}


void CColorSchemeOptions::ApplyStyle()
{
	CResizableDialog::ApplyStyle();

	if (m_hDlg != nullptr) {
		m_ColorListMargin = m_pStyleScaling->LogicalPixelsToPhysicalPixels(2);
	}
}


void CColorSchemeOptions::RealizeStyle()
{
	CResizableDialog::RealizeStyle();

	if (m_hDlg != nullptr) {
		SetListItemSize();

		if (m_ColorPalette.IsCreated())
			m_ColorPalette.SetTooltipFont(m_Font.GetHandle());
	}
}


void CColorSchemeOptions::SetListItemSize()
{
	HWND hwnd = ::GetDlgItem(m_hDlg, IDC_COLORSCHEME_PRESET);
	HDC hdc = ::GetDC(hwnd);
	HFONT hfontOld = SelectFont(hdc, GetWindowFont(hwnd));
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	SelectFont(hdc, hfontOld);
	::ReleaseDC(hwnd, hdc);
	const int Height = tm.tmHeight + m_pStyleScaling->LogicalPixelsToPhysicalPixels(4 * 2);
	DlgComboBox_SetItemHeight(m_hDlg, IDC_COLORSCHEME_PRESET, 0, Height);
	DlgComboBox_SetItemHeight(m_hDlg, IDC_COLORSCHEME_PRESET, -1, Height);

	hwnd = ::GetDlgItem(m_hDlg, IDC_COLORSCHEME_LIST);
	hdc = ::GetDC(hwnd);
	hfontOld = SelectFont(hdc, GetWindowFont(hwnd));
	::GetTextMetrics(hdc, &tm);
	long MaxWidth = 0;
	for (int i = 0; i < CColorScheme::NUM_COLORS; i++) {
		LPCTSTR pszName = CColorScheme::GetColorName(i);
		RECT rc = {0, 0, 0, 0};
		::DrawText(hdc, pszName, -1, &rc, DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);
		if (rc.right > MaxWidth)
			MaxWidth = rc.right;
	}
	SelectFont(hdc, hfontOld);
	::ReleaseDC(hwnd, hdc);
	DlgListBox_SetItemHeight(m_hDlg, IDC_COLORSCHEME_LIST, 0, tm.tmHeight + m_ColorListMargin * 2);
	DlgListBox_SetHorizontalExtent(
		m_hDlg, IDC_COLORSCHEME_LIST, tm.tmHeight * 2 + MaxWidth + m_ColorListMargin * 3);
}


}	// namespace TVTest
