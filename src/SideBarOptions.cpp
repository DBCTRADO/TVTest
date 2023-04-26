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
#include "SideBarOptions.h"
#include "AppMain.h"
#include "DialogUtil.h"
#include "DPIUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace
{

constexpr int ZOOM_ICON_FIRST = 37;

}


static const CSideBar::SideBarItem ItemList[] = {
	{CM_ZOOM_20,                ZOOM_ICON_FIRST + 0},
	{CM_ZOOM_25,                ZOOM_ICON_FIRST + 1},
	{CM_ZOOM_33,                ZOOM_ICON_FIRST + 2},
	{CM_ZOOM_50,                ZOOM_ICON_FIRST + 3},
	{CM_ZOOM_66,                ZOOM_ICON_FIRST + 4},
	{CM_ZOOM_75,                ZOOM_ICON_FIRST + 5},
	{CM_ZOOM_100,               ZOOM_ICON_FIRST + 6},
	{CM_ZOOM_150,               ZOOM_ICON_FIRST + 7},
	{CM_ZOOM_200,               ZOOM_ICON_FIRST + 8},
	{CM_ZOOM_250,               ZOOM_ICON_FIRST + 9},
	{CM_ZOOM_300,               ZOOM_ICON_FIRST + 10},
	{CM_CUSTOMZOOM_FIRST + 0,   ZOOM_ICON_FIRST + 11},
	{CM_CUSTOMZOOM_FIRST + 1,   ZOOM_ICON_FIRST + 12},
	{CM_CUSTOMZOOM_FIRST + 2,   ZOOM_ICON_FIRST + 13},
	{CM_CUSTOMZOOM_FIRST + 3,   ZOOM_ICON_FIRST + 14},
	{CM_CUSTOMZOOM_FIRST + 4,   ZOOM_ICON_FIRST + 15},
	{CM_CUSTOMZOOM_FIRST + 5,   ZOOM_ICON_FIRST + 16},
	{CM_CUSTOMZOOM_FIRST + 6,   ZOOM_ICON_FIRST + 17},
	{CM_CUSTOMZOOM_FIRST + 7,   ZOOM_ICON_FIRST + 18},
	{CM_CUSTOMZOOM_FIRST + 8,   ZOOM_ICON_FIRST + 19},
	{CM_CUSTOMZOOM_FIRST + 9,   ZOOM_ICON_FIRST + 20},
	{CM_ASPECTRATIO_DEFAULT,    0},
	{CM_ASPECTRATIO_16x9,       1},
	{CM_ASPECTRATIO_LETTERBOX,  2},
	{CM_ASPECTRATIO_WINDOWBOX,  3},
	{CM_ASPECTRATIO_PILLARBOX,  4},
	{CM_ASPECTRATIO_4x3,        5},
	{CM_FULLSCREEN,             6},
	{CM_ALWAYSONTOP,            7},
	{CM_DISABLEVIEWER,          8},
	{CM_CAPTURE,                9},
	{CM_SAVEIMAGE,              10},
	{CM_COPYIMAGE,              11},
	{CM_CAPTUREPREVIEW,         12},
	{CM_RESET,                  13},
	{CM_RESETVIEWER,            14},
	{CM_PANEL,                  15},
	{CM_PROGRAMGUIDE,           16},
	{CM_STATUSBAR,              17},
	{CM_VIDEODECODERPROPERTY,   18},
	{CM_OPTIONS,                19},
	{CM_STREAMINFO,             20},
	{CM_SPDIF_TOGGLE,           21},
	{CM_HOMEDISPLAY,            22},
	{CM_CHANNELDISPLAY,         23},
	{CM_1SEGMODE,               24},
	{CM_CHANNELNO_1,            25},
	{CM_CHANNELNO_2,            26},
	{CM_CHANNELNO_3,            27},
	{CM_CHANNELNO_4,            28},
	{CM_CHANNELNO_5,            29},
	{CM_CHANNELNO_6,            30},
	{CM_CHANNELNO_7,            31},
	{CM_CHANNELNO_8,            32},
	{CM_CHANNELNO_9,            33},
	{CM_CHANNELNO_10,           34},
	{CM_CHANNELNO_11,           35},
	{CM_CHANNELNO_12,           36},
};

static const int DefaultItemList[] = {
#ifndef TVTEST_FOR_1SEG
	CM_ZOOM_25,
	CM_ZOOM_33,
#endif
	CM_ZOOM_50,
	CM_ZOOM_100,
#ifdef TVTEST_FOR_1SEG
	CM_ZOOM_150,
	CM_ZOOM_200,
#endif
	0,
	CM_FULLSCREEN,
	CM_ALWAYSONTOP,
	CM_DISABLEVIEWER,
	0,
	CM_PANEL,
	CM_PROGRAMGUIDE,
	CM_CHANNELDISPLAY,
	CM_OPTIONS,
};


CSideBarOptions::CSideBarOptions(CSideBar *pSideBar, const CZoomOptions *pZoomOptions)
	: COptions(TEXT("SideBar"))
	, m_pSideBar(pSideBar)
	, m_pZoomOptions(pZoomOptions)
{
	m_AvailItemList.resize(lengthof(ItemList));
	for (int i = 0; i < lengthof(ItemList); i++)
		m_AvailItemList[i] = ItemList[i];

	m_ItemList.resize(lengthof(DefaultItemList));
	for (int i = 0; i < lengthof(DefaultItemList); i++)
		m_ItemList[i] = DefaultItemList[i];
}


CSideBarOptions::~CSideBarOptions()
{
	Destroy();
}


bool CSideBarOptions::ReadSettings(CSettings &Settings)
{
	int Value, NumItems;

	Settings.Read(TEXT("ShowPopup"), &m_fShowPopup);
	Settings.Read(TEXT("ShowToolTips"), &m_fShowToolTips);
	Settings.Read(TEXT("ShowChannelLogo"), &m_fShowChannelLogo);
	if (Settings.Read(TEXT("PopupOpacity"), &Value))
		m_PopupOpacity = std::clamp(Value, OPACITY_MIN, OPACITY_MAX);
	if (Settings.Read(TEXT("Place"), &Value)
			&& CheckEnumRange(static_cast<PlaceType>(Value)))
		m_Place = static_cast<PlaceType>(Value);

	if (Settings.Read(TEXT("ItemCount"), &NumItems) && NumItems > 0) {
		// はまるのを防ぐために、200を上限にしておく
		if (NumItems >= 200)
			NumItems = 200;

		m_ItemNameList.clear();

		String Command;

		for (int i = 0; i < NumItems; i++) {
			TCHAR szName[32];

			StringFormat(szName, TEXT("Item{}"), i);
			if (Settings.Read(szName, &Command)) {
				m_ItemNameList.push_back(Command);
				/*
				if (Command.empty)) {
					m_ItemNameList.emplace_back();
				} else {
					m_ItemNameList.emplace_back(Command);
					const int ID = m_pSideBar->GetCommandManager()->ParseIDText(Command);

					if (ID != 0) {
						for (const auto &Item : ItemList) {
							if (Item.Command == ID) {
								m_ItemList.push_back(ID);
								break;
							}
						}
					}
				}
				*/
			}
		}
	}

	return true;
}


bool CSideBarOptions::WriteSettings(CSettings &Settings)
{
	Settings.Clear();

	Settings.Write(TEXT("ShowPopup"), m_fShowPopup);
	Settings.Write(TEXT("ShowToolTips"), m_fShowToolTips);
	Settings.Write(TEXT("ShowChannelLogo"), m_fShowChannelLogo);
	Settings.Write(TEXT("PopupOpacity"), m_PopupOpacity);
	Settings.Write(TEXT("Place"), static_cast<int>(m_Place));

	Settings.Write(TEXT("ItemCount"), static_cast<int>(m_ItemNameList.size()));
	for (size_t i = 0; i < m_ItemNameList.size(); i++) {
		TCHAR szName[32];
		StringFormat(szName, TEXT("Item{}"), i);
		Settings.Write(szName, m_ItemNameList[i]);
	}
	/*
	const CCommandManager *pCommandManager = m_pSideBar->GetCommandManager();
	for (size_t i = 0; i < m_ItemList.size(); i++) {
		TCHAR szName[32];

		StringFormat(szName, TEXT("Item{}"), i);
		if (m_ItemList[i] == ITEM_SEPARATOR)
			Settings.Write(szName, TEXT(""));
		else
			Settings.Write(szName, pCommandManager->GetCommandIDText(m_ItemList[i]));
	}
	*/

	return true;
}


bool CSideBarOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(
		hwndOwner,
		GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_OPTIONS_SIDEBAR));
}


HBITMAP CSideBarOptions::CreateImage(IconSizeType SizeType, SIZE *pIconSize)
{
	LPCTSTR pszImageName, pszZoomImageName;
	int IconWidth, IconHeight;
	int NumWidth1, NumWidth2, NumWidth3, NumHeight3;
	int NumMargin1, NumMargin2, NumMargin3;
	int PercentWidth1, PercentWidth2;

	if (SizeType == IconSizeType::Small) {
		pszImageName = MAKEINTRESOURCE(IDB_SIDEBAR16);
		pszZoomImageName = MAKEINTRESOURCE(IDB_SIDEBARZOOM16);
		IconWidth = 16;
		IconHeight = 16;
		NumWidth1 = 4;
		NumWidth2 = 3;
		NumWidth3 = 3;
		NumHeight3 = 8;
		NumMargin1 = 1;
		NumMargin2 = 1;
		NumMargin3 = 1;
		PercentWidth1 = 6;
		PercentWidth2 = 4;
	} else {
		pszImageName = MAKEINTRESOURCE(IDB_SIDEBAR32);
		pszZoomImageName = MAKEINTRESOURCE(IDB_SIDEBARZOOM32);
		IconWidth = 32;
		IconHeight = 32;
		NumWidth1 = 8;
		NumWidth2 = 6;
		NumWidth3 = 6;
		NumHeight3 = 16;
		NumMargin1 = 2;
		NumMargin2 = 2;
		NumMargin3 = 2;
		PercentWidth1 = 12;
		PercentWidth2 = 8;
	}

	if (pIconSize != nullptr) {
		pIconSize->cx = IconWidth;
		pIconSize->cy = IconHeight;
	}

	const HINSTANCE hinst = GetAppClass().GetResourceInstance();
	const HBITMAP hbm = static_cast<HBITMAP>(::LoadImage(
		hinst, pszImageName, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));

	if (hbm != nullptr) {
		// 表示倍率のアイコンを描画する
		const HBITMAP hbmZoom = static_cast<HBITMAP>(::LoadImage(
			hinst, pszZoomImageName, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));

		if (hbmZoom != nullptr) {
			const HDC hdcDst = ::CreateCompatibleDC(nullptr);
			const HBITMAP hbmDstOld = static_cast<HBITMAP>(::SelectObject(hdcDst, hbm));
			const HDC hdcSrc = ::CreateCompatibleDC(nullptr);
			const HBITMAP hbmSrcOld = static_cast<HBITMAP>(::SelectObject(hdcSrc, hbmZoom));

			for (int i = 0; i < CZoomOptions::NUM_ZOOM_COMMANDS; i++) {
				CZoomOptions::ZoomInfo Zoom;
				if (m_pZoomOptions->GetZoomInfoByCommand(ItemList[i].Command, &Zoom)) {
					RECT rc;
					rc.left = (ZOOM_ICON_FIRST + i) * IconWidth;
					rc.top = 0;
					rc.right = rc.left + IconWidth;
					rc.bottom = IconHeight;
					if (Zoom.Type == CZoomOptions::ZoomType::Rate) {
						int Percentage = Zoom.Rate.GetPercentage();
						if (Percentage < 1000) {
							::FillRect(hdcDst, &rc, static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
							if (Percentage < 100) {
								if (Percentage >= 10) {
									::BitBlt(
										hdcDst, rc.left, 0, NumWidth1, IconHeight,
										hdcSrc, (Percentage / 10) * NumWidth1, 0, SRCCOPY);
								}
								::BitBlt(
									hdcDst, rc.left + NumWidth1 + NumMargin1, 0, NumWidth1, IconHeight,
									hdcSrc, (Percentage % 10) * NumWidth1, 0, SRCCOPY);
								// %
								::BitBlt(
									hdcDst,
									(ZOOM_ICON_FIRST + i) * IconWidth + (NumWidth1 + NumMargin1) * 2, 0,
									PercentWidth1, IconHeight,
									hdcSrc, NumWidth1 * 10, 0, SRCCOPY);
							} else {
								for (int j = 0; j < 3; j++) {
									::BitBlt(
										hdcDst,
										rc.left + (2 - j) * (NumWidth2 + NumMargin2), 0,
										NumWidth2, IconHeight,
										hdcSrc, (Percentage % 10) * NumWidth2, IconHeight, SRCCOPY);
									Percentage /= 10;
								}
								// %
								::BitBlt(
									hdcDst,
									rc.left + (NumWidth2 + NumMargin2) * 3, 0,
									PercentWidth2, IconHeight,
									hdcSrc, NumWidth2 * 10, IconHeight, SRCCOPY);
							}
						}
					} else if (Zoom.Type == CZoomOptions::ZoomType::Size) {
						if (Zoom.Size.Width < 10000 && Zoom.Size.Height < 10000) {
							::FillRect(hdcDst, &rc, static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
							for (int j = 0; j < 2; j++) {
								int Size = j == 0 ? Zoom.Size.Width : Zoom.Size.Height;
								for (int k = 0; k < 4 && Size != 0; k++) {
									::BitBlt(
										hdcDst,
										rc.left + (3 - k) * (NumWidth3 + NumMargin3), j * IconHeight / 2,
										NumWidth3, NumHeight3,
										hdcSrc, (Size % 10) * NumWidth3, IconHeight * 2, SRCCOPY);
									Size /= 10;
								}
							}
						}
					}
				}
			}

			::SelectObject(hdcSrc, hbmSrcOld);
			::DeleteDC(hdcSrc);
			::SelectObject(hdcDst, hbmDstOld);
			::DeleteDC(hdcDst);
			::DeleteObject(hbmZoom);
		}
	}

	return hbm;
}


bool CSideBarOptions::ApplySideBarOptions()
{
	m_pSideBar->ShowToolTips(m_fShowToolTips);
	m_pSideBar->SetVertical(m_Place == PlaceType::Left || m_Place == PlaceType::Right);
	return true;
}


void CSideBarOptions::ApplyItemList()
{
	const CCommandManager *pCommandManager = m_pSideBar->GetCommandManager();

	if (!m_ItemNameList.empty()) {
		m_ItemList.clear();

		for (const String &Name : m_ItemNameList) {
			if (Name.empty()) {
				m_ItemList.push_back(ITEM_SEPARATOR);
			} else {
				const int ID = pCommandManager->ParseIDText(Name);
				if (ID != 0)
					m_ItemList.push_back(ID);
			}
		}
	}

	m_pSideBar->DeleteAllItems();

	for (const int Command : m_ItemList) {
		if (Command == ITEM_SEPARATOR) {
			m_pSideBar->AddSeparator();
		} else {
			for (const auto &e : m_AvailItemList) {
				if (e.Command == Command) {
					CSideBar::SideBarItem Item;

					Item.Command = Command;
					Item.Icon = e.Icon;
					Item.State = CSideBar::ItemState::None;
					const CCommandManager::CommandState State = pCommandManager->GetCommandState(Command);
					if (!!(State & CCommandManager::CommandState::Disabled))
						Item.State |= CSideBar::ItemState::Disabled;
					if (!!(State & CCommandManager::CommandState::Checked))
						Item.State |= CSideBar::ItemState::Checked;

					m_pSideBar->AddItem(&Item);
					break;
				}
			}
		}
	}

	m_pSideBar->Invalidate();
}


bool CSideBarOptions::SetSideBarImage()
{
	const Style::Size IconSize = m_pSideBar->GetIconDrawSize();
	SIZE sz;
	const HBITMAP hbm = CreateImage(
		IconSize.Width <= 16 && IconSize.Height <= 16 ? IconSizeType::Small : IconSizeType::Big,
		&sz);
	if (hbm == nullptr)
		return false;
	const bool fResult = m_pSideBar->SetIconImage(hbm, sz.cx, sz.cy);
	::DeleteObject(hbm);
	return fResult;
}


bool CSideBarOptions::RegisterCommand(int ID)
{
	if (ID <= 0 || IsAvailableItem(ID))
		return false;

	CSideBar::SideBarItem Item;

	Item.Command = ID;
	Item.Icon = -1;
	Item.State = CSideBar::ItemState::None;

	m_AvailItemList.push_back(Item);

	return true;
}


bool CSideBarOptions::SetPlace(PlaceType Place)
{
	if (!CheckEnumRange(Place))
		return false;
	m_Place = Place;
	return true;
}


INT_PTR CSideBarOptions::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			DlgCheckBox_Check(hDlg, IDC_SIDEBAR_SHOWPOPUP, m_fShowPopup);

			::SendDlgItemMessage(
				hDlg, IDC_SIDEBAR_OPACITY_SLIDER,
				TBM_SETRANGE, TRUE, MAKELPARAM(OPACITY_MIN, OPACITY_MAX));
			::SendDlgItemMessage(
				hDlg, IDC_SIDEBAR_OPACITY_SLIDER,
				TBM_SETPOS, TRUE, m_PopupOpacity);
			::SendDlgItemMessage(
				hDlg, IDC_SIDEBAR_OPACITY_SLIDER,
				TBM_SETPAGESIZE, 0, 10);
			::SendDlgItemMessage(
				hDlg, IDC_SIDEBAR_OPACITY_SLIDER,
				TBM_SETTICFREQ, 10, 0);
			DlgEdit_SetInt(hDlg, IDC_SIDEBAR_OPACITY_INPUT, m_PopupOpacity);
			DlgUpDown_SetRange(hDlg, IDC_SIDEBAR_OPACITY_SPIN, OPACITY_MIN, OPACITY_MAX);
			EnableDlgItems(
				hDlg, IDC_SIDEBAR_OPACITY_LABEL, IDC_SIDEBAR_OPACITY_UNIT,
				m_fShowPopup && Util::OS::IsWindows8OrLater());

			DlgCheckBox_Check(hDlg, IDC_SIDEBAR_SHOWTOOLTIPS, m_fShowToolTips);
			DlgCheckBox_Check(hDlg, IDC_SIDEBAR_SHOWCHANNELLOGO, m_fShowChannelLogo);

			HWND hwndList = ::GetDlgItem(hDlg, IDC_SIDEBAR_ITEMLIST);
			ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
			SetListViewTooltipsTopMost(hwndList);
			m_himlIcons = CreateIconImageList();
			ListView_SetImageList(hwndList, m_himlIcons, LVSIL_SMALL);
			RECT rc;
			::GetClientRect(hwndList, &rc);
			rc.right -= GetScrollBarWidth(hwndList);
			LVCOLUMN lvc;
			lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM;
			lvc.fmt = LVCFMT_LEFT;
			lvc.cx = rc.right;
			lvc.iSubItem = 0;
			ListView_InsertColumn(hwndList, 0, &lvc);
			SetItemList(hwndList, &m_ItemList[0], static_cast<int>(m_ItemList.size()));

			hwndList = ::GetDlgItem(hDlg, IDC_SIDEBAR_COMMANDLIST);
			ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
			SetListViewTooltipsTopMost(hwndList);
			ListView_SetImageList(hwndList, m_himlIcons, LVSIL_SMALL);
			::GetClientRect(hwndList, &rc);
			rc.right -= GetScrollBarWidth(hwndList);
			lvc.cx = rc.right;
			ListView_InsertColumn(hwndList, 0, &lvc);
			std::vector<int> List;
			List.resize(m_AvailItemList.size());
			for (size_t i = 0; i < m_AvailItemList.size(); i++)
				List[i] = m_AvailItemList[i].Command;
			SetItemList(hwndList, List.data(), static_cast<int>(List.size()));

			AddControl(IDC_SIDEBAR_ITEMLIST, AlignFlag::VertLeft | AlignFlag::RightHalf);
			AddControl(IDC_SIDEBAR_COMMANDLIST_LABEL, AlignFlag::LeftHalf);
			AddControl(IDC_SIDEBAR_COMMANDLIST, AlignFlag::LeftHalf | AlignFlag::VertRight);
			AddControls(IDC_SIDEBAR_UP, IDC_SIDEBAR_DEFAULT, AlignFlag::Bottom);
			AddControls(IDC_SIDEBAR_ADD, IDC_SIDEBAR_SEPARATOR, AlignFlag::LeftHalf | AlignFlag::Bottom);
		}
		return TRUE;

	case WM_HSCROLL:
		if (reinterpret_cast<HWND>(lParam) == ::GetDlgItem(hDlg, IDC_SIDEBAR_OPACITY_SLIDER)) {
			SyncEditWithTrackBar(
				hDlg,
				IDC_SIDEBAR_OPACITY_SLIDER,
				IDC_SIDEBAR_OPACITY_INPUT);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SIDEBAR_SHOWPOPUP:
			EnableDlgItems(
				hDlg, IDC_SIDEBAR_OPACITY_LABEL, IDC_SIDEBAR_OPACITY_UNIT,
				DlgCheckBox_IsChecked(hDlg, IDC_SIDEBAR_SHOWPOPUP) && Util::OS::IsWindows8OrLater());
			return TRUE;

		case IDC_SIDEBAR_OPACITY_INPUT:
			if (HIWORD(wParam) == EN_CHANGE) {
				SyncTrackBarWithEdit(
					hDlg,
					IDC_SIDEBAR_OPACITY_INPUT,
					IDC_SIDEBAR_OPACITY_SLIDER);
			}
			return TRUE;

		case IDC_SIDEBAR_UP:
		case IDC_SIDEBAR_DOWN:
			{
				const bool fUp = LOWORD(wParam) == IDC_SIDEBAR_UP;
				const HWND hwndList = ::GetDlgItem(hDlg, IDC_SIDEBAR_ITEMLIST);
				const int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

				if ((fUp && Sel > 0) || (!fUp && Sel < ListView_GetItemCount(hwndList) - 1)) {
					LVITEM lvi;
					TCHAR szText[CCommandManager::MAX_COMMAND_TEXT];

					lvi.mask = LVIF_STATE | LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
					lvi.iItem = Sel;
					lvi.iSubItem = 0;
					lvi.stateMask = ~0U;
					lvi.pszText = szText;
					lvi.cchTextMax = lengthof(szText);
					ListView_GetItem(hwndList, &lvi);
					ListView_DeleteItem(hwndList, Sel);
					if (fUp)
						lvi.iItem--;
					else
						lvi.iItem++;
					ListView_InsertItem(hwndList, &lvi);
				}
			}
			return TRUE;

		case IDC_SIDEBAR_REMOVE:
			{
				const HWND hwndList = ::GetDlgItem(hDlg, IDC_SIDEBAR_ITEMLIST);
				const int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

				if (Sel >= 0)
					ListView_DeleteItem(hwndList, Sel);
			}
			return TRUE;

		case IDC_SIDEBAR_DEFAULT:
			SetItemList(
				::GetDlgItem(hDlg, IDC_SIDEBAR_ITEMLIST),
				DefaultItemList, lengthof(DefaultItemList));
			return TRUE;

		case IDC_SIDEBAR_ADD:
			{
				HWND hwndList = ::GetDlgItem(hDlg, IDC_SIDEBAR_COMMANDLIST);
				int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

				if (Sel >= 0) {
					LVITEM lvi;
					TCHAR szText[CCommandManager::MAX_COMMAND_TEXT];

					lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
					lvi.iItem = Sel;
					lvi.iSubItem = 0;
					lvi.pszText = szText;
					lvi.cchTextMax = lengthof(szText);
					ListView_GetItem(hwndList, &lvi);
					hwndList = ::GetDlgItem(hDlg, IDC_SIDEBAR_ITEMLIST);
					Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
					lvi.iItem = Sel >= 0 ? Sel : ListView_GetItemCount(hwndList);
					lvi.state = LVIS_SELECTED | LVIS_FOCUSED;
					lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
					ListView_InsertItem(hwndList, &lvi);
				}
			}
			return TRUE;

		case IDC_SIDEBAR_SEPARATOR:
			{
				const HWND hwndList = ::GetDlgItem(hDlg, IDC_SIDEBAR_ITEMLIST);
				const int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
				LVITEM lvi;

				lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
				lvi.iItem = Sel >= 0 ? Sel : ListView_GetItemCount(hwndList);
				lvi.iSubItem = 0;
				lvi.pszText = const_cast<LPTSTR>(TEXT("(区切り)"));
				lvi.iImage = -1;
				lvi.lParam = ITEM_SEPARATOR;
				ListView_InsertItem(hwndList, &lvi);
			}
			return TRUE;
		}
		return TRUE;

	case WM_SIZE:
		{
			static const int IDList[] = {IDC_SIDEBAR_ITEMLIST, IDC_SIDEBAR_COMMANDLIST};

			for (const int ID : IDList) {
				const HWND hwndList = ::GetDlgItem(hDlg, ID);
				RECT rcWindow;
				::GetWindowRect(hwndList, &rcWindow);
				RECT rcFrame = {};
				AdjustWindowRectWithDPI(&rcFrame, GetWindowStyle(hwndList), GetWindowExStyle(hwndList), false, GetWindowDPI(hwndList));
				ListView_SetColumnWidth(hwndList, 0, (rcWindow.right - rcWindow.left) - (-rcFrame.left + rcFrame.right + GetScrollBarWidth(hwndList)));
			}
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case LVN_ITEMCHANGED:
			{
				const NMLISTVIEW *pnmlv = reinterpret_cast<const NMLISTVIEW*>(lParam);
				const int Sel = ListView_GetNextItem(pnmlv->hdr.hwndFrom, -1, LVNI_SELECTED);

				if (pnmlv->hdr.idFrom == IDC_SIDEBAR_ITEMLIST) {
					EnableDlgItem(hDlg, IDC_SIDEBAR_UP, Sel > 0);
					EnableDlgItem(hDlg, IDC_SIDEBAR_DOWN, Sel >= 0 && Sel < ListView_GetItemCount(pnmlv->hdr.hwndFrom) - 1);
					EnableDlgItem(hDlg, IDC_SIDEBAR_REMOVE, Sel >= 0);
				} else {
					EnableDlgItem(hDlg, IDC_SIDEBAR_ADD, Sel >= 0);
				}
			}
			break;

		case NM_RCLICK:
			{
				const NMITEMACTIVATE *pnmia = reinterpret_cast<const NMITEMACTIVATE*>(lParam);

				if (pnmia->hdr.idFrom == IDC_SIDEBAR_ITEMLIST) {
					if (pnmia->iItem >= 0) {
						static const int MenuIDs[] = {
							IDC_SIDEBAR_UP,
							IDC_SIDEBAR_DOWN,
							0,
							IDC_SIDEBAR_REMOVE,
						};
						PopupMenuFromControls(hDlg, MenuIDs, lengthof(MenuIDs), TPM_RIGHTBUTTON);
					}
				} else {
					if (pnmia->iItem >= 0) {
						static const int MenuIDs[] = {
							IDC_SIDEBAR_ADD,
						};
						PopupMenuFromControls(hDlg, MenuIDs, lengthof(MenuIDs), TPM_RIGHTBUTTON);
					}
				}
			}
			break;

		case PSN_APPLY:
			{
				m_fShowPopup = DlgCheckBox_IsChecked(hDlg, IDC_SIDEBAR_SHOWPOPUP);
				if (Util::OS::IsWindows8OrLater()) {
					const int Opacity = DlgEdit_GetInt(hDlg, IDC_SIDEBAR_OPACITY_INPUT);
					m_PopupOpacity = std::clamp(Opacity, OPACITY_MIN, OPACITY_MAX);
				}
				m_fShowToolTips = DlgCheckBox_IsChecked(hDlg, IDC_SIDEBAR_SHOWTOOLTIPS);
				m_pSideBar->ShowToolTips(m_fShowToolTips);
				const bool fShowChannelLogo = DlgCheckBox_IsChecked(hDlg, IDC_SIDEBAR_SHOWCHANNELLOGO);
				if (m_fShowChannelLogo != fShowChannelLogo) {
					m_fShowChannelLogo = fShowChannelLogo;
					m_pSideBar->Invalidate();
				}

				const HWND hwndList = ::GetDlgItem(hDlg, IDC_SIDEBAR_ITEMLIST);
				const int Count = ListView_GetItemCount(hwndList);
				std::vector<int> ItemList;
				if (Count > 0) {
					ItemList.resize(Count);
					LVITEM lvi;
					lvi.mask = LVIF_PARAM;
					lvi.iSubItem = 0;
					for (int i = 0; i < Count; i++) {
						lvi.iItem = i;
						ListView_GetItem(hwndList, &lvi);
						ItemList[i] = static_cast<int>(lvi.lParam);
					}
				}
				if (ItemList != m_ItemList) {
					m_ItemList = ItemList;

					const CCommandManager *pCommandManager = m_pSideBar->GetCommandManager();
					m_ItemNameList.clear();
					for (int i = 0; i < Count; i++) {
						const int ID = m_ItemList[i];
						if (ID == ITEM_SEPARATOR) {
							m_ItemNameList.emplace_back();
						} else {
							m_ItemNameList.emplace_back(pCommandManager->GetCommandIDText(ID));
						}
					}

					ApplyItemList();
				}

				m_fChanged = true;
			}
			break;
		}
		break;

	case WM_DESTROY:
		if (m_himlIcons != nullptr) {
			::ImageList_Destroy(m_himlIcons);
			m_himlIcons = nullptr;
		}
		m_IconIDMap.clear();
		return TRUE;
	}

	return FALSE;
}


void CSideBarOptions::OnDarkModeChanged(bool fDarkMode)
{
	const HIMAGELIST himlIcons = CreateIconImageList();
	if (himlIcons != nullptr) {
		UpdateListViewIcons(::GetDlgItem(m_hDlg, IDC_SIDEBAR_ITEMLIST), himlIcons);
		UpdateListViewIcons(::GetDlgItem(m_hDlg, IDC_SIDEBAR_COMMANDLIST), himlIcons);

		if (m_himlIcons != nullptr)
			::ImageList_Destroy(m_himlIcons);
		m_himlIcons = himlIcons;
	}
}


HIMAGELIST CSideBarOptions::CreateIconImageList()
{
	const COLORREF IconColor = GetThemeColor(COLOR_WINDOWTEXT);
	const int IconWidth = GetSystemMetricsWithDPI(SM_CXSMICON, m_CurrentDPI);
	const int IconHeight = GetSystemMetricsWithDPI(SM_CYSMICON, m_CurrentDPI);
	SIZE sz;
	const HBITMAP hbmIcons = CreateImage(IconWidth <= 16 && IconHeight <= 16 ? IconSizeType::Small : IconSizeType::Big, &sz);
	Theme::IconList Bitmap;
	Bitmap.Create(hbmIcons, sz.cx, sz.cy, IconWidth, IconHeight);
	::DeleteObject(hbmIcons);
	const HIMAGELIST himlIcons = Bitmap.CreateImageList(IconColor);
	Bitmap.Destroy();

	m_IconIDMap.clear();
	for (const auto &e : ItemList)
		m_IconIDMap.emplace(e.Command, e.Icon);
	const CCommandManager *pCommandManager = m_pSideBar->GetCommandManager();
	for (size_t i = lengthof(ItemList); i < m_AvailItemList.size(); i++) {
		const int ID = m_AvailItemList[i].Command;
		if (ID >= CM_PLUGIN_FIRST && ID <= CM_PLUGIN_LAST) {
			CPlugin *pPlugin = GetAppClass().PluginManager.GetPluginByCommand(ID);
			if (pPlugin != nullptr && pPlugin->GetIcon().IsCreated()) {
				const HICON hIcon = pPlugin->GetIcon().ExtractIcon(IconColor);
				if (hIcon != nullptr) {
					const int Icon = ImageList_AddIcon(himlIcons, hIcon);
					::DestroyIcon(hIcon);
					m_IconIDMap.emplace(ID, Icon);
				}
			}
		} else if (ID >= CM_PLUGINCOMMAND_FIRST && ID <= CM_PLUGINCOMMAND_LAST) {
			String CommandID = pCommandManager->GetCommandIDText(ID);
			LPCTSTR pszCommand;
			CPlugin *pPlugin = GetAppClass().PluginManager.GetPluginByPluginCommand(CommandID.c_str(), &pszCommand);
			if (pPlugin != nullptr) {
				CPlugin::CPluginCommandInfo *pCommandInfo =
					pPlugin->GetPluginCommandInfo(pszCommand);
				if (pCommandInfo != nullptr && pCommandInfo->GetIcon().IsCreated()) {
					const HICON hIcon = pCommandInfo->GetIcon().ExtractIcon(IconColor);
					if (hIcon != nullptr) {
						const int Icon = ImageList_AddIcon(himlIcons, hIcon);
						::DestroyIcon(hIcon);
						m_IconIDMap.emplace(ID, Icon);
					}
				}
			}
		}
	}

	return himlIcons;
}


void CSideBarOptions::UpdateListViewIcons(HWND hwndList, HIMAGELIST himl)
{
	ListView_SetImageList(hwndList, himl, LVSIL_SMALL);

	const int ItemCount = ListView_GetItemCount(hwndList);

	for (int i = 0; i < ItemCount; i++) {
		LVITEM lvi;

		lvi.mask = LVIF_PARAM;
		lvi.iItem = i;
		lvi.iSubItem = 0;
		ListView_GetItem(hwndList, &lvi);
		if (lvi.lParam != ITEM_SEPARATOR) {
			lvi.mask = LVIF_IMAGE;
			auto itr = m_IconIDMap.find(static_cast<int>(lvi.lParam));
			if (itr != m_IconIDMap.end())
				lvi.iImage = itr->second;
			else
				lvi.iImage = -1;
			ListView_SetItem(hwndList, &lvi);
		}
	}
}


void CSideBarOptions::SetItemList(HWND hwndList, const int *pList, int NumItems)
{
	const CCommandManager *pCommandManager = m_pSideBar->GetCommandManager();
	LVITEM lvi;
	TCHAR szText[CCommandManager::MAX_COMMAND_TEXT];

	ListView_DeleteAllItems(hwndList);
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvi.iSubItem = 0;
	lvi.pszText = szText;

	for (int i = 0; i < NumItems; i++) {
		const int ID = pList[i];

		lvi.iImage = -1;
		if (ID == ITEM_SEPARATOR) {
			StringCopy(szText, TEXT("(区切り)"));
		} else {
			pCommandManager->GetCommandText(ID, szText, lengthof(szText));
			auto itr = m_IconIDMap.find(ID);
			if (itr != m_IconIDMap.end())
				lvi.iImage = itr->second;
		}
		lvi.iItem = i;
		lvi.lParam = pList[i];
		ListView_InsertItem(hwndList, &lvi);
	}
}


bool CSideBarOptions::IsAvailableItem(int ID) const
{
	for (const auto &e : m_AvailItemList) {
		if (e.Command == ID)
			return true;
	}

	return false;
}


}	// namespace TVTest
