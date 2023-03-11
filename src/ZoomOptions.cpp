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
#include "ZoomOptions.h"
#include "DialogUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace
{

constexpr std::size_t MAX_ZOOM_TEXT = 64;


#ifdef TVTEST_FOR_1SEG
constexpr bool f1Seg = true;
constexpr int BASE_WIDTH  = 320;
constexpr int BASE_HEIGHT = 180;
#else
constexpr bool f1Seg = false;
constexpr int BASE_WIDTH  = 1920;
constexpr int BASE_HEIGHT = 1080;
#endif

}


const CZoomOptions::ZoomCommandInfo CZoomOptions::m_DefaultZoomList[NUM_ZOOM_COMMANDS] = {
	{CM_ZOOM_20,              {ZoomType::Rate, {  1,   5}, {BASE_WIDTH / 5,     BASE_HEIGHT / 5},     !f1Seg}},
	{CM_ZOOM_25,              {ZoomType::Rate, {  1,   4}, {BASE_WIDTH / 4,     BASE_HEIGHT / 4},     !f1Seg}},
	{CM_ZOOM_33,              {ZoomType::Rate, {  1,   3}, {BASE_WIDTH / 3,     BASE_HEIGHT / 3},     !f1Seg}},
	{CM_ZOOM_50,              {ZoomType::Rate, {  1,   2}, {BASE_WIDTH / 2,     BASE_HEIGHT / 2},     true}},
	{CM_ZOOM_66,              {ZoomType::Rate, {  2,   3}, {BASE_WIDTH * 2 / 3, BASE_HEIGHT * 2 / 3}, true}},
	{CM_ZOOM_75,              {ZoomType::Rate, {  3,   4}, {BASE_WIDTH * 3 / 4, BASE_HEIGHT * 3 / 4}, true}},
	{CM_ZOOM_100,             {ZoomType::Rate, {  1,   1}, {BASE_WIDTH,         BASE_HEIGHT},         true}},
	{CM_ZOOM_150,             {ZoomType::Rate, {  3,   2}, {BASE_WIDTH * 3 / 2, BASE_HEIGHT * 3 / 2}, true}},
	{CM_ZOOM_200,             {ZoomType::Rate, {  2,   1}, {BASE_WIDTH * 2,     BASE_HEIGHT * 2},     true}},
	{CM_ZOOM_250,             {ZoomType::Rate, {  5,   2}, {BASE_WIDTH * 5 / 2, BASE_HEIGHT * 5 / 2}, f1Seg}},
	{CM_ZOOM_300,             {ZoomType::Rate, {  3,   1}, {BASE_WIDTH * 3,     BASE_HEIGHT * 3},     f1Seg}},
	{CM_CUSTOMZOOM_FIRST + 0, {ZoomType::Rate, {100, 100}, {BASE_WIDTH,         BASE_HEIGHT},         false}},
	{CM_CUSTOMZOOM_FIRST + 1, {ZoomType::Rate, {100, 100}, {BASE_WIDTH,         BASE_HEIGHT},         false}},
	{CM_CUSTOMZOOM_FIRST + 2, {ZoomType::Rate, {100, 100}, {BASE_WIDTH,         BASE_HEIGHT},         false}},
	{CM_CUSTOMZOOM_FIRST + 3, {ZoomType::Rate, {100, 100}, {BASE_WIDTH,         BASE_HEIGHT},         false}},
	{CM_CUSTOMZOOM_FIRST + 4, {ZoomType::Rate, {100, 100}, {BASE_WIDTH,         BASE_HEIGHT},         false}},
	{CM_CUSTOMZOOM_FIRST + 5, {ZoomType::Rate, {100, 100}, {BASE_WIDTH,         BASE_HEIGHT},         false}},
	{CM_CUSTOMZOOM_FIRST + 6, {ZoomType::Rate, {100, 100}, {BASE_WIDTH,         BASE_HEIGHT},         false}},
	{CM_CUSTOMZOOM_FIRST + 7, {ZoomType::Rate, {100, 100}, {BASE_WIDTH,         BASE_HEIGHT},         false}},
	{CM_CUSTOMZOOM_FIRST + 8, {ZoomType::Rate, {100, 100}, {BASE_WIDTH,         BASE_HEIGHT},         false}},
	{CM_CUSTOMZOOM_FIRST + 9, {ZoomType::Rate, {100, 100}, {BASE_WIDTH,         BASE_HEIGHT},         false}},
};


CZoomOptions::CZoomOptions()
	: CCommandCustomizer(CM_CUSTOMZOOM_FIRST, CM_CUSTOMZOOM_LAST)
{
	for (int i = 0; i < NUM_ZOOM_COMMANDS; i++) {
		m_ZoomList[i] = m_DefaultZoomList[i].Info;
		m_Order[i] = i;
	}
}


bool CZoomOptions::Show(HWND hwndOwner)
{
	return ShowDialog(
		hwndOwner,
		GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_ZOOMOPTIONS)) == IDOK;
}


bool CZoomOptions::ReadSettings(CSettings &Settings)
{
	TCHAR szText[256];

	int j = CM_ZOOM_LAST - CM_ZOOM_FIRST + 1;
	for (int i = 0; i <= CM_CUSTOMZOOM_LAST - CM_CUSTOMZOOM_FIRST; i++, j++) {
		int Type, Rate, Width, Height;

		StringFormat(szText, TEXT("CustomZoom{}"), i + 1);
		if (Settings.Read(szText, &Rate) && Rate > 0 && Rate <= MAX_RATE)
			m_ZoomList[j].Rate.Rate = Rate;
		StringFormat(szText, TEXT("CustomZoom{}_Type"), i + 1);
		if (Settings.Read(szText, &Type) && CheckEnumRange(static_cast<ZoomType>(Type)))
			m_ZoomList[j].Type = static_cast<ZoomType>(Type);
		StringFormat(szText, TEXT("CustomZoom{}_Width"), i + 1);
		if (Settings.Read(szText, &Width) && Width > 0)
			m_ZoomList[j].Size.Width = Width;
		StringFormat(szText, TEXT("CustomZoom{}_Height"), i + 1);
		if (Settings.Read(szText, &Height) && Height > 0)
			m_ZoomList[j].Size.Height = Height;
	}

	int ListCount;
	if (Settings.Read(TEXT("ZoomListCount"), &ListCount) && ListCount > 0) {
		const CCommandManager *pCommandManager = &GetAppClass().CommandManager;

		if (ListCount > NUM_ZOOM_COMMANDS)
			ListCount = NUM_ZOOM_COMMANDS;
		int Count = 0;
		for (int i = 0; i < ListCount; i++) {
			TCHAR szName[32];

			StringFormat(szName, TEXT("ZoomList{}"), i);
			if (Settings.Read(szName, szText, lengthof(szText))) {
				LPTSTR p = szText;
				while (*p != _T('\0') && *p != _T(','))
					p++;
				if (*p == _T(','))
					*p++ = _T('\0');
				const int Command = pCommandManager->ParseIDText(szText);
				if (Command != 0) {
					j = GetIndexByCommand(Command);
					if (j >= 0) {
						int k;
						for (k = 0; k < Count; k++) {
							if (m_Order[k] == j)
								break;
						}
						if (k == Count) {
							m_Order[Count] = j;
							m_ZoomList[j].fVisible = *p != _T('0');
							Count++;
						}
					}
				}
			}
		}
		if (Count < NUM_ZOOM_COMMANDS) {
			for (int i = 0; i < NUM_ZOOM_COMMANDS; i++) {
				for (j = 0; j < Count; j++) {
					if (m_Order[j] == i)
						break;
				}
				if (j == Count)
					m_Order[Count++] = i;
			}
		}
	}

	return true;
}


bool CZoomOptions::WriteSettings(CSettings &Settings)
{
	TCHAR szText[256];

	int j = CM_ZOOM_LAST - CM_ZOOM_FIRST + 1;
	for (int i = 0; i <= CM_CUSTOMZOOM_LAST - CM_CUSTOMZOOM_FIRST; i++, j++) {
		StringFormat(szText, TEXT("CustomZoom{}"), i + 1);
		Settings.Write(szText, m_ZoomList[j].Rate.Rate);
		StringFormat(szText, TEXT("CustomZoom{}_Type"), i + 1);
		Settings.Write(szText, static_cast<int>(m_ZoomList[j].Type));
		StringFormat(szText, TEXT("CustomZoom{}_Width"), i + 1);
		Settings.Write(szText, m_ZoomList[j].Size.Width);
		StringFormat(szText, TEXT("CustomZoom{}_Height"), i + 1);
		Settings.Write(szText, m_ZoomList[j].Size.Height);
	}

	Settings.Write(TEXT("ZoomListCount"), NUM_ZOOM_COMMANDS);
	const CCommandManager *pCommandManager = &GetAppClass().CommandManager;
	for (int i = 0; i < NUM_ZOOM_COMMANDS; i++) {
		const int Index = m_Order[i];
		const ZoomInfo &Info = m_ZoomList[Index];
		TCHAR szName[32];

		StringFormat(szName, TEXT("ZoomList{}"), i);
		StringFormat(
			szText, TEXT("{},{}"),
			pCommandManager->GetCommandIDText(m_DefaultZoomList[Index].Command),
			Info.fVisible ? 1 : 0);
		Settings.Write(szName, szText);
	}

	return true;
}


bool CZoomOptions::SetMenu(HMENU hmenu, const ZoomInfo *pCurZoom) const
{
	for (int i = ::GetMenuItemCount(hmenu) - 3; i >= 0; i--)
		::DeleteMenu(hmenu, i, MF_BYPOSITION);

	int Pos = 0;
	bool fRateCheck = false, fSizeCheck = false;
	for (int i = 0; i < NUM_ZOOM_COMMANDS; i++) {
		const ZoomInfo &Info = m_ZoomList[m_Order[i]];

		if (Info.fVisible) {
			TCHAR szText[MAX_ZOOM_TEXT];
			UINT Flags = MF_BYPOSITION | MF_STRING | MF_ENABLED;

			if (Info.Type == ZoomType::Rate) {
				const size_t Length = StringFormat(
					szText, TEXT("{}%"),
					Info.Rate.GetPercentage());
				if (Info.Rate.Rate * 100 % Info.Rate.Factor != 0) {
					StringFormat(
						szText + Length, lengthof(szText) - Length, TEXT(" ({}/{})"),
						Info.Rate.Rate, Info.Rate.Factor);
				}
				if (!fRateCheck
						&& pCurZoom != nullptr
						&& pCurZoom->Rate.GetPercentage() == Info.Rate.GetPercentage()) {
					Flags |= MF_CHECKED;
					fRateCheck = true;
				}
			} else {
				StringFormat(
					szText, TEXT("{} x {}"),
					Info.Size.Width, Info.Size.Height);
				if (!fSizeCheck
						&& pCurZoom != nullptr
						&& pCurZoom->Size.Width == Info.Size.Width
						&& pCurZoom->Size.Height == Info.Size.Height) {
					Flags |= MF_CHECKED;
					fSizeCheck = true;
				}
			}
			::InsertMenu(hmenu, Pos++, Flags, m_DefaultZoomList[m_Order[i]].Command, szText);
		}
	}

	return true;
}


bool CZoomOptions::GetZoomInfoByCommand(int Command, ZoomInfo *pInfo) const
{
	const int i = GetIndexByCommand(Command);

	if (i < 0)
		return false;
	*pInfo = m_ZoomList[i];
	return true;
}


int CZoomOptions::GetIndexByCommand(int Command) const
{
	for (int i = 0; i < NUM_ZOOM_COMMANDS; i++) {
		if (m_DefaultZoomList[i].Command == Command)
			return i;
	}
	return -1;
}


void CZoomOptions::FormatCommandText(int Command, const ZoomInfo &Info, LPTSTR pszText, size_t MaxLength) const
{
	const int Length = ::LoadString(GetAppClass().GetResourceInstance(), Command, pszText, static_cast<int>(MaxLength));
	if (Command >= CM_CUSTOMZOOM_FIRST && Command <= CM_CUSTOMZOOM_LAST) {
		if (Info.Type == ZoomType::Rate)
			StringFormat(pszText + Length, MaxLength - Length, TEXT(" : {}%"), Info.Rate.GetPercentage());
		else if (Info.Type == ZoomType::Size)
			StringFormat(pszText + Length, MaxLength - Length, TEXT(" : {} x {}"), Info.Size.Width, Info.Size.Height);
	}
}


void CZoomOptions::SetItemState(HWND hDlg)
{
	const int Sel = m_ItemListView.GetSelectedItem();

	if (Sel >= 0) {
		const int Index = GetItemIndex(Sel);
		const ZoomInfo &Info = m_ZoomSettingList[Index];
		const bool fCustom =
			m_DefaultZoomList[Index].Command >= CM_CUSTOMZOOM_FIRST &&
			m_DefaultZoomList[Index].Command <= CM_CUSTOMZOOM_LAST;
		::CheckRadioButton(
			hDlg, IDC_ZOOMOPTIONS_TYPE_RATE, IDC_ZOOMOPTIONS_TYPE_SIZE,
			IDC_ZOOMOPTIONS_TYPE_RATE + static_cast<int>(Info.Type));
		EnableDlgItems(hDlg, IDC_ZOOMOPTIONS_TYPE_RATE, IDC_ZOOMOPTIONS_TYPE_SIZE, fCustom);
		::SetDlgItemInt(hDlg, IDC_ZOOMOPTIONS_RATE, Info.Rate.GetPercentage(), TRUE);
		::SetDlgItemInt(hDlg, IDC_ZOOMOPTIONS_WIDTH, Info.Size.Width, TRUE);
		::SetDlgItemInt(hDlg, IDC_ZOOMOPTIONS_HEIGHT, Info.Size.Height, TRUE);
		EnableDlgItems(hDlg, IDC_ZOOMOPTIONS_RATE_LABEL, IDC_ZOOMOPTIONS_RATE_UNIT, Info.Type == ZoomType::Rate);
		EnableDlgItem(hDlg, IDC_ZOOMOPTIONS_RATE, Info.Type == ZoomType::Rate && fCustom);
		EnableDlgItems(hDlg, IDC_ZOOMOPTIONS_WIDTH_LABEL, IDC_ZOOMOPTIONS_GETCURSIZE, Info.Type == ZoomType::Size);
		EnableDlgItem(hDlg, IDC_ZOOMOPTIONS_WIDTH, Info.Type == ZoomType::Size && fCustom);
		EnableDlgItem(hDlg, IDC_ZOOMOPTIONS_HEIGHT, Info.Type == ZoomType::Size && fCustom);
		EnableDlgItem(hDlg, IDC_ZOOMOPTIONS_GETCURSIZE, Info.Type == ZoomType::Size && fCustom);
		EnableDlgItem(hDlg, IDC_ZOOMOPTIONS_UP, Sel > 0);
		EnableDlgItem(hDlg, IDC_ZOOMOPTIONS_DOWN, Sel + 1 < NUM_ZOOM_COMMANDS);
	} else {
		::SetDlgItemText(hDlg, IDC_ZOOMOPTIONS_RATE, TEXT(""));
		::SetDlgItemText(hDlg, IDC_ZOOMOPTIONS_WIDTH, TEXT(""));
		::SetDlgItemText(hDlg, IDC_ZOOMOPTIONS_HEIGHT, TEXT(""));
		EnableDlgItems(hDlg, IDC_ZOOMOPTIONS_TYPE_RATE, IDC_ZOOMOPTIONS_DOWN, FALSE);
	}
}


int CZoomOptions::GetItemIndex(int Item)
{
	return static_cast<int>(m_ItemListView.GetItemParam(Item));
}


void CZoomOptions::UpdateItemText(int Item)
{
	const int Index = GetItemIndex(Item);
	TCHAR szText[MAX_ZOOM_TEXT];

	FormatCommandText(
		m_DefaultZoomList[Index].Command, m_ZoomSettingList[Index],
		szText, lengthof(szText));
	m_ItemListView.SetItemText(Item, szText);
}


INT_PTR CZoomOptions::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			m_fChanging = true;

			m_ItemListView.Attach(::GetDlgItem(hDlg, IDC_ZOOMOPTIONS_LIST));
			m_ItemListView.InitCheckList();

			for (int i = 0; i < NUM_ZOOM_COMMANDS; i++) {
				const int Index = m_Order[i];
				const ZoomInfo &Info = m_ZoomList[Index];
				TCHAR szText[MAX_ZOOM_TEXT];

				m_ZoomSettingList[Index] = Info;
				FormatCommandText(m_DefaultZoomList[Index].Command, Info, szText, lengthof(szText));
				m_ItemListView.InsertItem(i, szText, Index);
				m_ItemListView.CheckItem(i, Info.fVisible);
			}

			SetItemState(hDlg);

			m_fChanging = false;

			AdjustDialogPos(::GetParent(hDlg), hDlg);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ZOOMOPTIONS_TYPE_RATE:
		case IDC_ZOOMOPTIONS_TYPE_SIZE:
			{
				const int Sel = m_ItemListView.GetSelectedItem();

				if (Sel >= 0) {
					const int Index = GetItemIndex(Sel);
					const int Command = m_DefaultZoomList[Index].Command;

					if (Command >= CM_CUSTOMZOOM_FIRST && Command <= CM_CUSTOMZOOM_LAST) {
						const ZoomType Type = ::IsDlgButtonChecked(hDlg, IDC_ZOOMOPTIONS_TYPE_RATE) ? ZoomType::Rate : ZoomType::Size;
						ZoomInfo &Info = m_ZoomSettingList[Index];

						if (Type != Info.Type) {
							Info.Type = Type;
							m_fChanging = true;
							UpdateItemText(Sel);
							SetItemState(hDlg);
							m_fChanging = false;
						}
					}
				}
			}
			return TRUE;

		case IDC_ZOOMOPTIONS_RATE:
			if (HIWORD(wParam) == EN_CHANGE && !m_fChanging) {
				const int Rate = ::GetDlgItemInt(hDlg, IDC_ZOOMOPTIONS_RATE, nullptr, TRUE);

				if (Rate > 0 && Rate <= MAX_RATE) {
					const int Sel = m_ItemListView.GetSelectedItem();

					if (Sel >= 0) {
						const int Index = GetItemIndex(Sel);
						const int Command = m_DefaultZoomList[Index].Command;

						if (Command >= CM_CUSTOMZOOM_FIRST && Command <= CM_CUSTOMZOOM_LAST) {
							ZoomInfo &Info = m_ZoomSettingList[Index];
							Info.Rate.Rate = Rate;
							Info.Rate.Factor = 100;
							m_fChanging = true;
							UpdateItemText(Sel);
							m_fChanging = false;
						}
					}
				}
			}
			return TRUE;

		case IDC_ZOOMOPTIONS_WIDTH:
		case IDC_ZOOMOPTIONS_HEIGHT:
			if (HIWORD(wParam) == EN_CHANGE && !m_fChanging) {
				const int Width = ::GetDlgItemInt(hDlg, IDC_ZOOMOPTIONS_WIDTH, nullptr, TRUE);
				const int Height = ::GetDlgItemInt(hDlg, IDC_ZOOMOPTIONS_HEIGHT, nullptr, TRUE);

				if (Width > 0 && Height > 0) {
					const int Sel = m_ItemListView.GetSelectedItem();

					if (Sel >= 0) {
						const int Index = GetItemIndex(Sel);
						const int Command = m_DefaultZoomList[Index].Command;

						if (Command >= CM_CUSTOMZOOM_FIRST && Command <= CM_CUSTOMZOOM_LAST) {
							ZoomInfo &Info = m_ZoomSettingList[Index];
							Info.Size.Width = Width;
							Info.Size.Height = Height;
							m_fChanging = true;
							UpdateItemText(Sel);
							m_fChanging = false;
						}
					}
				}
			}
			return TRUE;

		case IDC_ZOOMOPTIONS_GETCURSIZE:
			{
				const HWND hwndViewer = GetAppClass().UICore.GetViewerWindow();

				if (hwndViewer != nullptr) {
					RECT rc;

					::GetClientRect(hwndViewer, &rc);
					::SetDlgItemInt(hDlg, IDC_ZOOMOPTIONS_WIDTH, rc.right - rc.left, TRUE);
					::SetDlgItemInt(hDlg, IDC_ZOOMOPTIONS_HEIGHT, rc.bottom - rc.top, TRUE);
				}
			}
			return TRUE;

		case IDC_ZOOMOPTIONS_UP:
		case IDC_ZOOMOPTIONS_DOWN:
			{
				const int From = m_ItemListView.GetSelectedItem();
				int To;

				if (From >= 0) {
					if (LOWORD(wParam) == IDC_ZOOMOPTIONS_UP) {
						if (From < 1)
							break;
						To = From - 1;
					} else {
						if (From + 1 >= NUM_ZOOM_COMMANDS)
							break;
						To = From + 1;
					}

					m_fChanging = true;
					m_ItemListView.MoveItem(From, To);
					m_ItemListView.EnsureItemVisible(To);
					SetItemState(hDlg);
					m_fChanging = false;
				}
			}
			return TRUE;

		case IDOK:
			{
				for (int i = 0; i < NUM_ZOOM_COMMANDS; i++) {
					const int Index = GetItemIndex(i);
					m_Order[i] = Index;
					if (m_DefaultZoomList[Index].Command >= CM_CUSTOMZOOM_FIRST
							&& m_DefaultZoomList[Index].Command <= CM_CUSTOMZOOM_LAST) {
						m_ZoomList[Index] = m_ZoomSettingList[Index];
					}
					m_ZoomList[Index].fVisible = m_ItemListView.IsItemChecked(i);
				}
			}
			[[fallthrough]];
		case IDCANCEL:
			::EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case LVN_ITEMCHANGED:
			if (!m_fChanging)
				SetItemState(hDlg);
			return TRUE;
		}
		break;

	case WM_DESTROY:
		m_ItemListView.Detach();
		return TRUE;
	}

	return FALSE;
}


bool CZoomOptions::GetCommandText(int Command, LPTSTR pszText, size_t MaxLength)
{
	ZoomInfo Info;
	if (!GetZoomInfoByCommand(Command, &Info))
		return false;
	FormatCommandText(Command, Info, pszText, MaxLength);
	return true;
}


}	// namespace TVTest
