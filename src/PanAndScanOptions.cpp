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
#include "PanAndScanOptions.h"
#include "DialogUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


static constexpr int FACTOR_PERCENTAGE = 100;
static constexpr int HORZ_FACTOR = FACTOR_PERCENTAGE * 100;
static constexpr int VERT_FACTOR = FACTOR_PERCENTAGE * 100;




static void FormatValue(int Value, int Factor, LPTSTR pszText, size_t MaxLength)
{
	int Percentage;

	if (Factor != 0)
		Percentage = (Value * 100) / Factor;
	else
		Percentage = 0;
	if (Percentage % 100 == 0)
		StringFormat(pszText, MaxLength, TEXT("{}"), Percentage / 100);
	else
		StringFormat(pszText, MaxLength, TEXT("{}.{:02}"), Percentage / 100, std::abs(Percentage) % 100);
}


static int GetValue(LPCTSTR pszValue, int Factor)
{
	int Value = 0;
	LPCTSTR p;
	for (p = pszValue; *p >= _T('0') && *p <= _T('9'); p++) {
		Value = Value * 10 + (*p - _T('0'));
	}
	if (*p == _T('.')) {
		p++;
		int Decimal = 1;
		for (; Decimal < Factor && *p >= _T('0') && *p <= _T('9'); p++) {
			Value = Value * 10 + (*p - _T('0'));
			Decimal *= 10;
		}
		Value = Value * Factor / Decimal;
	} else {
		Value *= Factor;
	}

	return Value;
}


static int FormatPanAndScanInfo(const CCoreEngine::PanAndScanInfo &Info, LPTSTR pszText, int MaxLength)
{
	TCHAR szXPos[32], szYPos[32], szWidth[32], szHeight[32];

	FormatValue(Info.XPos, FACTOR_PERCENTAGE, szXPos, lengthof(szXPos));
	FormatValue(Info.YPos, FACTOR_PERCENTAGE, szYPos, lengthof(szYPos));
	FormatValue(Info.Width, FACTOR_PERCENTAGE, szWidth, lengthof(szWidth));
	FormatValue(Info.Height, FACTOR_PERCENTAGE, szHeight, lengthof(szHeight));
	return static_cast<int>(StringFormat(
		pszText, MaxLength, TEXT("{},{},{},{},{},{}"),
		szXPos, szYPos, szWidth, szHeight, Info.XAspect, Info.YAspect));
}


static bool ParsePanAndScanInfo(CCoreEngine::PanAndScanInfo *pInfo, LPTSTR pszText)
{
	LPTSTR p = pszText;
	int j;
	for (j = 0; j < 6 && *p != _T('\0'); j++) {
		while (*p == _T(' '))
			p++;
		const LPCTSTR pszValue = p;
		while (*p != _T('\0') && *p != _T(','))
			p++;
		if (*p != _T('\0'))
			*p++ = _T('\0');
		if (*pszValue != _T('\0')) {
			switch (j) {
			case 0: pInfo->XPos = GetValue(pszValue, FACTOR_PERCENTAGE);   break;
			case 1: pInfo->YPos = GetValue(pszValue, FACTOR_PERCENTAGE);   break;
			case 2: pInfo->Width = GetValue(pszValue, FACTOR_PERCENTAGE);  break;
			case 3: pInfo->Height = GetValue(pszValue, FACTOR_PERCENTAGE); break;
			case 4: pInfo->XAspect = GetValue(pszValue, 1);                break;
			case 5: pInfo->YAspect = GetValue(pszValue, 1);                break;
			}
		}
	}
	if (j < 6)
		return false;

	pInfo->XFactor = HORZ_FACTOR;
	pInfo->YFactor = VERT_FACTOR;

	if (pInfo->XPos < 0 || pInfo->YPos < 0
			|| pInfo->Width < 1 || pInfo->Height < 1
			|| pInfo->XPos + pInfo->Width > pInfo->XFactor
			|| pInfo->YPos + pInfo->Height > pInfo->YFactor
			|| pInfo->XAspect < 1 || pInfo->YAspect < 1) {
		return false;
	}

	return true;
}


// 設定サンプル
static const CPanAndScanOptions::PanAndScanInfo DefaultPresetList[] = {
	{   0, 1281, 10000,  7438, HORZ_FACTOR, VERT_FACTOR, 239, 100, TEXT("2.39:1 シネスコ"),   1},
	{   0, 1218, 10000,  7565, HORZ_FACTOR, VERT_FACTOR, 235, 100, TEXT("2.35:1 シネスコ"),   2},
	{   0,  195, 10000,  9610, HORZ_FACTOR, VERT_FACTOR, 185, 100, TEXT("1.85:1 ビスタ(米)"), 3},
	{ 331,    0,  9338, 10000, HORZ_FACTOR, VERT_FACTOR, 166, 100, TEXT("1.66:1 ビスタ(欧)"), 4},
};


CPanAndScanOptions::CPanAndScanOptions()
	: CSettingsBase(TEXT("PanAndScan"))
	, CCommandCustomizer(CM_PANANDSCAN_PRESET_FIRST, CM_PANANDSCAN_PRESET_LAST)
	, m_PresetID(lengthof(DefaultPresetList) + 1)
{
	for (const PanAndScanInfo &e : DefaultPresetList)
		m_PresetList.push_back(e);
}


bool CPanAndScanOptions::Show(HWND hwndOwner)
{
	return ShowDialog(
		hwndOwner,
		GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_PANANDSCANOPTIONS)) == IDOK;
}


bool CPanAndScanOptions::ReadSettings(CSettings &Settings)
{
	int Count;

	if (Settings.Read(TEXT("PresetCount"), &Count)) {
		m_PresetList.clear();

		for (int i = 0; i < Count; i++) {
			PanAndScanInfo Info;
			TCHAR szKey[32], szSettings[256];

			StringFormat(szKey, TEXT("Preset{}.Name"), i);
			if (!Settings.Read(szKey, Info.szName, MAX_NAME) || Info.szName[0] == _T('\0'))
				break;
			StringFormat(szKey, TEXT("Preset{}"), i);
			if (!Settings.Read(szKey, szSettings, lengthof(szSettings)) || szSettings[0] == _T('\0'))
				break;

			if (ParsePanAndScanInfo(&Info.Info, szSettings)) {
				Info.ID = m_PresetID++;
				m_PresetList.push_back(Info);
			}
		}
	}

	return true;
}


bool CPanAndScanOptions::WriteSettings(CSettings &Settings)
{
	Settings.Clear();
	Settings.Write(TEXT("PresetCount"), static_cast<unsigned int>(m_PresetList.size()));
	for (size_t i = 0; i < m_PresetList.size(); i++) {
		const PanAndScanInfo &Info = m_PresetList[i];
		TCHAR szKey[32], szSettings[256];

		StringFormat(szKey, TEXT("Preset{}.Name"), i);
		Settings.Write(szKey, Info.szName);
		StringFormat(szKey, TEXT("Preset{}"), i);
		FormatPanAndScanInfo(Info.Info, szSettings, lengthof(szSettings));
		Settings.Write(szKey, szSettings);
	}

	return true;
}


size_t CPanAndScanOptions::GetPresetCount() const
{
	return m_PresetList.size();
}


bool CPanAndScanOptions::GetPreset(size_t Index, PanAndScanInfo *pInfo) const
{
	if (Index >= m_PresetList.size())
		return false;

	*pInfo = m_PresetList[Index];

	return true;
}


bool CPanAndScanOptions::GetPresetByID(UINT ID, PanAndScanInfo *pInfo) const
{
	const int Index = FindPresetByID(ID);
	if (Index < 0)
		return false;

	*pInfo = m_PresetList[Index];

	return true;
}


UINT CPanAndScanOptions::GetPresetID(size_t Index) const
{
	if (Index >= m_PresetList.size())
		return 0;
	return m_PresetList[Index].ID;
}


int CPanAndScanOptions::FindPresetByID(UINT ID) const
{
	for (size_t i = 0; i < m_PresetList.size(); i++) {
		if (m_PresetList[i].ID == ID)
			return static_cast<int>(i);
	}
	return -1;
}


static void FormatInfo(const CPanAndScanOptions::PanAndScanInfo *pInfo, LPTSTR pszText, size_t MaxLength)
{
	TCHAR szXPos[32], szYPos[32], szWidth[32], szHeight[32];

	FormatValue(pInfo->Info.XPos, FACTOR_PERCENTAGE, szXPos, lengthof(szXPos));
	FormatValue(pInfo->Info.YPos, FACTOR_PERCENTAGE, szYPos, lengthof(szYPos));
	FormatValue(pInfo->Info.Width, FACTOR_PERCENTAGE, szWidth, lengthof(szWidth));
	FormatValue(pInfo->Info.Height, FACTOR_PERCENTAGE, szHeight, lengthof(szHeight));
	StringFormat(
		pszText, MaxLength, TEXT("{} , {} / {} x {} / {} : {}"),
		szXPos, szYPos, szWidth, szHeight, pInfo->Info.XAspect, pInfo->Info.YAspect);
}

static void InsertItem(HWND hwndList, int Index, CPanAndScanOptions::PanAndScanInfo *pInfo, bool fSelect = false)
{
	LVITEM lvi;

	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	if (Index >= 0)
		lvi.iItem = Index;
	else
		lvi.iItem = ListView_GetItemCount(hwndList);
	lvi.iSubItem = 0;
	lvi.pszText = pInfo->szName;
	lvi.lParam = reinterpret_cast<LPARAM>(pInfo);
	ListView_InsertItem(hwndList, &lvi);

	TCHAR szText[80];
	FormatInfo(pInfo, szText, lengthof(szText));
	lvi.mask = LVIF_TEXT;
	lvi.iSubItem = 1;
	lvi.pszText = szText;
	ListView_SetItem(hwndList, &lvi);

	if (fSelect) {
		ListView_SetItemState(
			hwndList, lvi.iItem,
			LVIS_FOCUSED | LVIS_SELECTED,
			LVIS_FOCUSED | LVIS_SELECTED);
	}
}

static bool UpdateItem(HWND hwndList, int Index, const CPanAndScanOptions::PanAndScanInfo &Info)
{
	LVITEM lvi;

	lvi.mask = LVIF_PARAM;
	lvi.iItem = Index;
	lvi.iSubItem = 0;
	if (!ListView_GetItem(hwndList, &lvi))
		return false;

	CPanAndScanOptions::PanAndScanInfo *pInfo =
		reinterpret_cast<CPanAndScanOptions::PanAndScanInfo*>(lvi.lParam);
	*pInfo = Info;

	lvi.mask = LVIF_TEXT;
	lvi.pszText = pInfo->szName;
	ListView_SetItem(hwndList, &lvi);
	TCHAR szText[80];
	FormatInfo(pInfo, szText, lengthof(szText));
	lvi.iSubItem = 1;
	lvi.pszText = szText;
	ListView_SetItem(hwndList, &lvi);

	return true;
}

static CPanAndScanOptions::PanAndScanInfo *GetInfo(HWND hwndList, int Index)
{
	LVITEM lvi;

	lvi.mask = LVIF_PARAM;
	lvi.iItem = Index;
	lvi.iSubItem = 0;
	if (!ListView_GetItem(hwndList, &lvi))
		return nullptr;

	return reinterpret_cast<CPanAndScanOptions::PanAndScanInfo*>(lvi.lParam);
}

INT_PTR CPanAndScanOptions::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			m_fTested = false;

			const HWND hwndList = ::GetDlgItem(hDlg, IDC_PANANDSCAN_LIST);

			ListView_SetExtendedListViewStyle(
				hwndList,
				LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER);
			SetListViewTooltipsTopMost(hwndList);

			LVCOLUMN lvc;
			lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
			lvc.fmt = LVCFMT_LEFT;
			lvc.cx = 80;
			lvc.pszText = const_cast<LPTSTR>(TEXT("名前"));
			lvc.iSubItem = 0;
			ListView_InsertColumn(hwndList, 0, &lvc);
			lvc.pszText = const_cast<LPTSTR>(TEXT("設定"));
			lvc.iSubItem = 1;
			ListView_InsertColumn(hwndList, 1, &lvc);

			for (size_t i = 0; i < m_PresetList.size(); i++) {
				PanAndScanInfo *pInfo = new PanAndScanInfo(m_PresetList[i]);

				InsertItem(hwndList, -1, pInfo);
			}

			for (int i = 0; i < 2; i++)
				ListView_SetColumnWidth(hwndList, i, LVSCW_AUTOSIZE_USEHEADER);

			DlgEdit_LimitText(hDlg, IDC_PANANDSCAN_NAME, MAX_NAME - 1);

			SetItemStatus();

			AdjustDialogPos(::GetParent(hDlg), hDlg);
		}
		return TRUE;

	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT pdis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

			if (pdis->CtlID == IDC_PANANDSCAN_PREVIEW) {
				RECT rcScreen;
				CCoreEngine::PanAndScanInfo PanScan;

				const int ItemWidth = pdis->rcItem.right - pdis->rcItem.left;
				const int ItemHeight = pdis->rcItem.bottom - pdis->rcItem.top;
				const int ScreenWidth = std::min(ItemHeight * 16 / 9, ItemWidth);
				const int ScreenHeight = std::min(ItemWidth * 9 / 16, ItemHeight);
				rcScreen.left = pdis->rcItem.left + (ItemWidth - ScreenWidth) / 2;
				rcScreen.top = pdis->rcItem.top + (ItemHeight - ScreenHeight) / 2;
				rcScreen.right = rcScreen.left + ScreenWidth;
				rcScreen.bottom = rcScreen.top + ScreenHeight;
				::FillRect(
					pdis->hDC, &rcScreen,
					static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
				if (GetPanAndScanSettings(&PanScan)) {
					const HBRUSH hbr = ::CreateSolidBrush(RGB(128, 128, 128));
					const HPEN hpen = ::CreatePen(
						PS_INSIDEFRAME,
						m_pStyleScaling->ToPixels(1, Style::UnitType::LogicalPixel),
						RGB(160, 160, 160));
					const HGDIOBJ hOldBrush = ::SelectObject(pdis->hDC, hbr);
					const HGDIOBJ hOldPen = ::SelectObject(pdis->hDC, hpen);

					::Rectangle(
						pdis->hDC,
						rcScreen.left + ::MulDiv(PanScan.XPos, ScreenWidth, PanScan.XFactor),
						rcScreen.top + ::MulDiv(PanScan.YPos, ScreenHeight, PanScan.YFactor),
						rcScreen.left + ::MulDiv(PanScan.XPos + PanScan.Width, ScreenWidth, PanScan.XFactor),
						rcScreen.top + ::MulDiv(PanScan.YPos + PanScan.Height, ScreenHeight, PanScan.YFactor));
					::SelectObject(pdis->hDC, hOldBrush);
					::SelectObject(pdis->hDC, hOldPen);
					::DeleteObject(hbr);
					::DeleteObject(hpen);
				}
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_PANANDSCAN_UP:
		case IDC_PANANDSCAN_DOWN:
			{
				const HWND hwndList = ::GetDlgItem(hDlg, IDC_PANANDSCAN_LIST);
				const int From = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
				int To;

				if (From >= 0) {
					if (LOWORD(wParam) == IDC_PANANDSCAN_UP) {
						if (From < 1)
							break;
						To = From - 1;
					} else {
						if (From + 1 >= ListView_GetItemCount(hwndList))
							break;
						To = From + 1;
					}

					CPanAndScanOptions::PanAndScanInfo *pInfo = GetInfo(hwndList, From);
					if (pInfo != nullptr) {
						m_fStateChanging = true;
						ListView_DeleteItem(hwndList, From);
						InsertItem(hwndList, To, pInfo, true);
						SetItemStatus();
						m_fStateChanging = false;
					}
				}
			}
			return TRUE;

		case IDC_PANANDSCAN_REMOVE:
			{
				const HWND hwndList = GetDlgItem(hDlg, IDC_PANANDSCAN_LIST);
				const int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

				if (Sel >= 0) {
					CPanAndScanOptions::PanAndScanInfo *pInfo = GetInfo(hwndList, Sel);
					if (pInfo != nullptr) {
						ListView_DeleteItem(hwndList, Sel);
						delete pInfo;
						SetItemStatus();
					}
				}
			}
			return TRUE;

		case IDC_PANANDSCAN_CLEAR:
			{
				const HWND hwndList = ::GetDlgItem(hDlg, IDC_PANANDSCAN_LIST);
				const int ItemCount = ListView_GetItemCount(hwndList);

				for (int i = 0; i < ItemCount; i++)
					delete GetInfo(hwndList, i);
				ListView_DeleteAllItems(hwndList);
				SetItemStatus();
			}
			return TRUE;

		case IDC_PANANDSCAN_IMPORT:
			{
				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH];

				szFileName[0] = _T('\0');
				InitOpenFileName(&ofn);
				ofn.hwndOwner = hDlg;
				ofn.lpstrFilter =
					TEXT("設定ファイル(*.ini)\0*.ini\0")
					TEXT("すべてのファイル\0*.*\0");
				ofn.lpstrFile = szFileName;
				ofn.nMaxFile = lengthof(szFileName);
				ofn.lpstrTitle = TEXT("パン&スキャン設定の読み込み");
				ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_EXPLORER;
				if (FileOpenDialog(&ofn)) {
					Import(szFileName);
				}
			}
			return TRUE;

		case IDC_PANANDSCAN_EXPORT:
			{
				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH];

				szFileName[0] = _T('\0');
				InitOpenFileName(&ofn);
				ofn.hwndOwner = hDlg;
				ofn.lpstrFilter =
					TEXT("設定ファイル(*.ini)\0*.ini\0")
					TEXT("すべてのファイル\0*.*\0");
				ofn.lpstrFile = szFileName;
				ofn.nMaxFile = lengthof(szFileName);
				ofn.lpstrTitle = TEXT("パン&スキャン設定の保存");
				ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER;
				ofn.lpstrDefExt = TEXT("ini");
				if (FileSaveDialog(&ofn)) {
					Export(szFileName);
				}
			}
			return TRUE;

		case IDC_PANANDSCAN_ADD:
			{
				CPanAndScanOptions::PanAndScanInfo Info;

				if (GetSettings(&Info)) {
					Info.ID = m_PresetID++;
					InsertItem(
						::GetDlgItem(hDlg, IDC_PANANDSCAN_LIST),
						-1, new CPanAndScanOptions::PanAndScanInfo(Info), true);
					SetItemStatus();
				}
			}
			return TRUE;

		case IDC_PANANDSCAN_REPLACE:
			{
				const HWND hwndList = GetDlgItem(hDlg, IDC_PANANDSCAN_LIST);
				const int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

				if (Sel >= 0) {
					const PanAndScanInfo *pOldInfo = GetInfo(hwndList, Sel);

					if (pOldInfo != nullptr) {
						CPanAndScanOptions::PanAndScanInfo Info;

						if (GetSettings(&Info)) {
							Info.ID = pOldInfo->ID;
							UpdateItem(hwndList, Sel, Info);
						}
					}
				}
			}
			return TRUE;

		case IDC_PANANDSCAN_NAME:
			if (HIWORD(wParam) == EN_CHANGE && !m_fStateChanging) {
				const HWND hwndList = GetDlgItem(hDlg, IDC_PANANDSCAN_LIST);
				const int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
				const bool fOK = IsSettingsValid();

				EnableDlgItem(hDlg, IDC_PANANDSCAN_ADD, fOK);
				EnableDlgItem(hDlg, IDC_PANANDSCAN_REPLACE, fOK && Sel >= 0);
				EnableDlgItem(hDlg, IDC_PANANDSCAN_TEST, fOK);
			}
			return TRUE;

		case IDC_PANANDSCAN_XPOS:
		case IDC_PANANDSCAN_YPOS:
		case IDC_PANANDSCAN_WIDTH:
		case IDC_PANANDSCAN_HEIGHT:
			if (HIWORD(wParam) == EN_CHANGE) {
				InvalidateDlgItem(hDlg, IDC_PANANDSCAN_PREVIEW, nullptr, false);
			}
			return TRUE;

		case IDC_PANANDSCAN_TEST:
			{
				PanAndScanInfo Info;

				if (GetSettings(&Info)) {
					CUICore &UICore = GetAppClass().UICore;
					CCoreEngine::PanAndScanInfo OldInfo;

					if (!m_fTested)
						UICore.GetPanAndScan(&OldInfo);
					UICore.SetPanAndScan(Info.Info);
					if (!m_fTested) {
						m_fTested = true;
						m_OldPanAndScanInfo = OldInfo;
					}
				}
			}
			return TRUE;

		case IDOK:
			{
				const HWND hwndList = GetDlgItem(m_hDlg, IDC_PANANDSCAN_LIST);
				const int ItemCount = ListView_GetItemCount(hwndList);
				std::vector<PanAndScanInfo> List;

				for (int i = 0; i < ItemCount; i++) {
					const PanAndScanInfo *pInfo = GetInfo(hwndList, i);
					if (pInfo != nullptr)
						List.push_back(*pInfo);
				}

				m_PresetList = List;
			}
			[[fallthrough]];
		case IDCANCEL:
			if (m_fTested)
				GetAppClass().UICore.SetPanAndScan(m_OldPanAndScanInfo);

			::EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case LVN_ITEMCHANGED:
			{
				const NMLISTVIEW *pnmlv = reinterpret_cast<const NMLISTVIEW*>(lParam);

				if ((pnmlv->uOldState & LVIS_SELECTED) != (pnmlv->uNewState & LVIS_SELECTED)) {
					if ((pnmlv->uNewState & LVIS_SELECTED) != 0) {
						const CPanAndScanOptions::PanAndScanInfo *pInfo = GetInfo(pnmlv->hdr.hwndFrom, pnmlv->iItem);

						if (pInfo != nullptr) {
							TCHAR szText[32];

							m_fStateChanging = true;
							::SetDlgItemText(hDlg, IDC_PANANDSCAN_NAME, pInfo->szName);
							FormatValue(pInfo->Info.XPos, FACTOR_PERCENTAGE, szText, lengthof(szText));
							::SetDlgItemText(hDlg, IDC_PANANDSCAN_XPOS, szText);
							FormatValue(pInfo->Info.YPos, FACTOR_PERCENTAGE, szText, lengthof(szText));
							::SetDlgItemText(hDlg, IDC_PANANDSCAN_YPOS, szText);
							FormatValue(pInfo->Info.Width, FACTOR_PERCENTAGE, szText, lengthof(szText));
							::SetDlgItemText(hDlg, IDC_PANANDSCAN_WIDTH, szText);
							FormatValue(pInfo->Info.Height, FACTOR_PERCENTAGE, szText, lengthof(szText));
							::SetDlgItemText(hDlg, IDC_PANANDSCAN_HEIGHT, szText);
							::SetDlgItemInt(hDlg, IDC_PANANDSCAN_XASPECT, pInfo->Info.XAspect, TRUE);
							::SetDlgItemInt(hDlg, IDC_PANANDSCAN_YASPECT, pInfo->Info.YAspect, TRUE);
							m_fStateChanging = false;
							InvalidateDlgItem(hDlg, IDC_PANANDSCAN_PREVIEW, nullptr, false);
						}
					}

					SetItemStatus();
				}
			}
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			const HWND hwndList = ::GetDlgItem(hDlg, IDC_PANANDSCAN_LIST);
			const int ItemCount = ListView_GetItemCount(hwndList);

			for (int i = 0; i < ItemCount; i++)
				delete GetInfo(hwndList, i);
			ListView_DeleteAllItems(hwndList);
		}
		return TRUE;
	}

	return FALSE;
}


void CPanAndScanOptions::SetItemStatus() const
{
	const HWND hwndList = GetDlgItem(m_hDlg, IDC_PANANDSCAN_LIST);
	const int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
	const int ItemCount = ListView_GetItemCount(hwndList);
	const bool fValid = IsSettingsValid();

	EnableDlgItem(m_hDlg, IDC_PANANDSCAN_UP, Sel > 0);
	EnableDlgItem(m_hDlg, IDC_PANANDSCAN_DOWN, Sel >= 0 && Sel + 1 < ItemCount);
	EnableDlgItem(m_hDlg, IDC_PANANDSCAN_REMOVE, Sel >= 0);
	EnableDlgItem(m_hDlg, IDC_PANANDSCAN_CLEAR, ItemCount > 0);
	EnableDlgItem(m_hDlg, IDC_PANANDSCAN_EXPORT, ItemCount > 0);
	EnableDlgItem(m_hDlg, IDC_PANANDSCAN_ADD, fValid);
	EnableDlgItem(m_hDlg, IDC_PANANDSCAN_REPLACE, Sel >= 0 && fValid);
	EnableDlgItem(m_hDlg, IDC_PANANDSCAN_TEST, fValid);
}


static int GetItemValue(HWND hDlg, int ID, int Factor)
{
	TCHAR szValue[32];

	::GetDlgItemText(hDlg, ID, szValue, lengthof(szValue));

	return GetValue(szValue, Factor);
}

bool CPanAndScanOptions::GetSettings(CPanAndScanOptions::PanAndScanInfo *pInfo) const
{
	::GetDlgItemText(m_hDlg, IDC_PANANDSCAN_NAME, pInfo->szName, MAX_NAME);

	if (!GetPanAndScanSettings(&pInfo->Info)) {
		::MessageBox(m_hDlg, TEXT("入力された値が正しくありません。"), nullptr, MB_OK | MB_ICONINFORMATION);
		return false;
	}

	return true;
}


bool CPanAndScanOptions::GetPanAndScanSettings(CCoreEngine::PanAndScanInfo *pInfo) const
{
	pInfo->XFactor = HORZ_FACTOR;
	pInfo->YFactor = VERT_FACTOR;
	pInfo->XPos = GetItemValue(m_hDlg, IDC_PANANDSCAN_XPOS, FACTOR_PERCENTAGE);
	pInfo->YPos = GetItemValue(m_hDlg, IDC_PANANDSCAN_YPOS, FACTOR_PERCENTAGE);
	pInfo->Width = GetItemValue(m_hDlg, IDC_PANANDSCAN_WIDTH, FACTOR_PERCENTAGE);
	pInfo->Height = GetItemValue(m_hDlg, IDC_PANANDSCAN_HEIGHT, FACTOR_PERCENTAGE);
	pInfo->XAspect = ::GetDlgItemInt(m_hDlg, IDC_PANANDSCAN_XASPECT, nullptr, TRUE);
	pInfo->YAspect = ::GetDlgItemInt(m_hDlg, IDC_PANANDSCAN_YASPECT, nullptr, TRUE);

	if (pInfo->XPos < 0 || pInfo->YPos < 0
			|| pInfo->Width <= 0 || pInfo->Height <= 0
			|| pInfo->XPos + pInfo->Width > pInfo->XFactor
			|| pInfo->YPos + pInfo->Height > pInfo->YFactor
			|| pInfo->XAspect <= 0 || pInfo->YAspect <= 0) {
		return false;
	}

	return true;
}


bool CPanAndScanOptions::IsSettingsValid() const
{
	return ::GetWindowTextLength(::GetDlgItem(m_hDlg, IDC_PANANDSCAN_NAME)) > 0;
}


bool CPanAndScanOptions::Import(LPCTSTR pszFileName)
{
	CSettings Settings;

	if (!Settings.Open(pszFileName, CSettings::OpenFlag::Read)
			|| !Settings.SetSection(TEXT("PanAndScan")))
		return false;

	for (int i = 0;; i++) {
		PanAndScanInfo Info;
		TCHAR szKey[32], szSettings[256];

		StringFormat(szKey, TEXT("Preset{}.Name"), i);
		if (!Settings.Read(szKey, Info.szName, MAX_NAME) || Info.szName[0] == _T('\0'))
			break;
		StringFormat(szKey, TEXT("Preset{}"), i);
		if (!Settings.Read(szKey, szSettings, lengthof(szSettings)) || szSettings[0] == _T('\0'))
			break;

		if (ParsePanAndScanInfo(&Info.Info, szSettings)) {
			Info.ID = m_PresetID++;
			InsertItem(
				::GetDlgItem(m_hDlg, IDC_PANANDSCAN_LIST),
				-1, new CPanAndScanOptions::PanAndScanInfo(Info));
		}
	}

	SetItemStatus();

	return true;
}


bool CPanAndScanOptions::Export(LPCTSTR pszFileName) const
{
	const HANDLE hFile = ::CreateFile(
		pszFileName, GENERIC_WRITE, 0, nullptr,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE) {
		::MessageBox(m_hDlg, TEXT("ファイルを作成できません。"), nullptr, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	DWORD Write;

#ifdef UNICODE
	const WORD BOM = 0xFEFF;
	::WriteFile(hFile, &BOM, 2, &Write, nullptr);
#endif

	static const TCHAR szHeader[] = TEXT("; ") APP_NAME TEXT(" Pan&Scan presets\r\n[PanAndScan]\r\n");
	::WriteFile(hFile, szHeader, sizeof(szHeader) - sizeof(TCHAR), &Write, nullptr);

	const HWND hwndList = ::GetDlgItem(m_hDlg, IDC_PANANDSCAN_LIST);
	const int ItemCount = ListView_GetItemCount(hwndList);
	for (int i = 0; i < ItemCount; i++) {
		const PanAndScanInfo *pInfo = GetInfo(hwndList, i);
		if (pInfo == nullptr)
			break;

		TCHAR szBuffer[256], szSettings[256];

		FormatPanAndScanInfo(pInfo->Info, szSettings, lengthof(szSettings));
		const size_t Length = StringFormat(
			szBuffer,
			TEXT("Preset{}.Name={}\r\nPreset{}={}\r\n"),
			i, pInfo->szName, i, szSettings);
		::WriteFile(hFile, szBuffer, static_cast<DWORD>(Length * sizeof(TCHAR)), &Write, nullptr);
	}

	::CloseHandle(hFile);

	return true;
}


bool CPanAndScanOptions::GetCommandText(int Command, LPTSTR pszText, size_t MaxLength)
{
	if (Command < m_FirstID || Command > m_LastID)
		return false;
	const int Index = Command - m_FirstID;
	const size_t Length = StringFormat(pszText, MaxLength, TEXT("パン&スキャン{}"), Index + 1);
	if (static_cast<size_t>(Index) < m_PresetList.size()) {
		StringFormat(
			pszText + Length, MaxLength - Length,
			TEXT(" : {}"), m_PresetList[Index].szName);
	}
	return true;
}


} // namespace TVTest
