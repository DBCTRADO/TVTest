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
#include "StatusOptions.h"
#include "Settings.h"
#include "DialogUtil.h"
#include "StyleUtil.h"
#include "DPIUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace
{

constexpr bool IS_HD =
#ifndef TVTEST_FOR_1SEG
	true
#else
	false
#endif
	;

constexpr UINT TIMER_ID_UP   = 1;
constexpr UINT TIMER_ID_DOWN = 2;

}




CStatusOptions::CStatusOptions(CStatusView *pStatusView)
	: COptions(TEXT("Status"))
	, m_pStatusView(pStatusView)
	, m_DPI(GetSystemDPI())
	, m_fMultiRow(!IS_HD)
{
	static const struct {
		BYTE ID;
		bool fVisible;
	} DefaultItemList[] = {
		{STATUS_ITEM_TUNER,        IS_HD},
		{STATUS_ITEM_CHANNEL,      true},
		{STATUS_ITEM_FAVORITES,    false},
		{STATUS_ITEM_VIDEOSIZE,    true},
		{STATUS_ITEM_VOLUME,       true},
		{STATUS_ITEM_AUDIOCHANNEL, true},
		{STATUS_ITEM_RECORD,       true},
		{STATUS_ITEM_CAPTURE,      IS_HD},
		{STATUS_ITEM_ERROR,        IS_HD},
		{STATUS_ITEM_SIGNALLEVEL,  true},
		{STATUS_ITEM_CLOCK,        false},
		{STATUS_ITEM_PROGRAMINFO,  false},
		{STATUS_ITEM_BUFFERING,    false},
		{STATUS_ITEM_MEDIABITRATE, false},
	};

	m_AvailItemList.reserve(lengthof(DefaultItemList));

	for (const auto &e : DefaultItemList) {
		StatusItemInfo Info;

		Info.ID = e.ID;
		Info.fVisible = e.fVisible;
		Info.Width = -1;

		m_AvailItemList.push_back(Info);
	}

	m_ItemList = m_AvailItemList;

	m_pStatusView->GetFont(&m_ItemFont);
}


CStatusOptions::~CStatusOptions()
{
	Destroy();
}


bool CStatusOptions::ReadSettings(CSettings &Settings)
{
	int NumItems;
	if (Settings.Read(TEXT("NumItems"), &NumItems) && NumItems > 0) {
		StatusItemInfoList ItemList;
		String ID;

		for (int i = 0; i < NumItems; i++) {
			TCHAR szKey[32];

			StringFormat(szKey, TEXT("Item{}_ID"), i);
			if (Settings.Read(szKey, &ID) && !ID.empty()) {
				StatusItemInfo Item;

				LPTSTR p;
				const long IDNum = std::_tcstol(ID.c_str(), &p, 10);
				if (*p == _T('\0')) {
					if (IDNum >= STATUS_ITEM_FIRST && IDNum <= STATUS_ITEM_LAST) {
						Item.ID = static_cast<int>(IDNum);
					} else {
						continue;
					}
					size_t j;
					for (j = 0; j < ItemList.size(); j++) {
						if (ItemList[j].ID == Item.ID)
							break;
					}
					if (j < ItemList.size())
						continue;
					for (const auto &e : m_AvailItemList) {
						if (e.ID == Item.ID) {
							Item.fVisible = e.fVisible;
							break;
						}
					}
				} else {
					Item.ID = -1;
					Item.IDText = ID;
					size_t j = 0;
					for (j = 0; j < ItemList.size(); j++) {
						if (StringUtility::IsEqualNoCase(ItemList[j].IDText, Item.IDText))
							break;
					}
					if (j < ItemList.size())
						continue;
					Item.fVisible = false;
				}

				StringFormat(szKey, TEXT("Item{}_Visible"), i);
				Settings.Read(szKey, &Item.fVisible);
				StringFormat(szKey, TEXT("Item{}_Width"), i);
				if (!Settings.Read(szKey, &Item.Width) || Item.Width < 1)
					Item.Width = -1;

				ItemList.push_back(Item);
			}
		}

		for (const auto &e : m_AvailItemList) {
			if (std::ranges::find(ItemList, e.ID, &StatusItemInfo::ID) == ItemList.end()) {
				StatusItemInfo Item(e);
				Item.fVisible = false;
				ItemList.push_back(Item);
			}
		}

		m_ItemList = ItemList;
	}

	if (!Settings.Read(TEXT("DPI"), &m_DPI))
		m_fChanged = true;

	bool f;
	if (StyleUtil::ReadFontSettings(Settings, TEXT("Font"), &m_ItemFont, true, &f)) {
		if (!f)
			m_fChanged = true;
	}

	Settings.Read(TEXT("MultiRow"), &m_fMultiRow);
	Settings.Read(TEXT("MaxRows"), &m_MaxRows);
	Settings.Read(TEXT("ShowPopup"), &m_fShowPopup);
	int Value;
	if (Settings.Read(TEXT("PopupOpacity"), &Value))
		m_PopupOpacity = std::clamp(Value, OPACITY_MIN, OPACITY_MAX);

	Settings.Read(TEXT("TOTTime"), &m_fShowTOTTime);
	Settings.Read(TEXT("InterpolateTOTTime"), &m_fInterpolateTOTTime);
	Settings.Read(TEXT("PopupProgramInfo"), &m_fEnablePopupProgramInfo);
	Settings.Read(TEXT("ShowEventProgress"), &m_fShowEventProgress);
	Settings.Read(TEXT("PopupEventInfoWidth"), &m_PopupEventInfoWidth);
	Settings.Read(TEXT("PopupEventInfoHeight"), &m_PopupEventInfoHeight);

	return true;
}


bool CStatusOptions::WriteSettings(CSettings &Settings)
{
	if (Settings.Write(TEXT("NumItems"), static_cast<int>(m_ItemList.size()))) {
		for (int i = 0; i < static_cast<int>(m_ItemList.size()); i++) {
			const StatusItemInfo &Info = m_ItemList[i];
			TCHAR szKey[32];

			StringFormat(szKey, TEXT("Item{}_ID"), i);
			if (!Info.IDText.empty())
				Settings.Write(szKey, Info.IDText);
			else
				Settings.Write(szKey, Info.ID);
			StringFormat(szKey, TEXT("Item{}_Visible"), i);
			Settings.Write(szKey, Info.fVisible);
			StringFormat(szKey, TEXT("Item{}_Width"), i);
			Settings.Write(szKey, Info.Width);
		}
	}

	Settings.Write(TEXT("DPI"), m_DPI);

	StyleUtil::WriteFontSettings(Settings, TEXT("Font"), m_ItemFont);

	Settings.Write(TEXT("MultiRow"), m_fMultiRow);
	Settings.Write(TEXT("MaxRows"), m_MaxRows);
	Settings.Write(TEXT("ShowPopup"), m_fShowPopup);
	Settings.Write(TEXT("PopupOpacity"), m_PopupOpacity);

	Settings.Write(TEXT("TOTTime"), m_fShowTOTTime);
	Settings.Write(TEXT("InterpolateTOTTime"), m_fInterpolateTOTTime);
	Settings.Write(TEXT("PopupProgramInfo"), m_fEnablePopupProgramInfo);
	Settings.Write(TEXT("ShowEventProgress"), m_fShowEventProgress);
	Settings.Write(TEXT("PopupEventInfoWidth"), m_PopupEventInfoWidth);
	Settings.Write(TEXT("PopupEventInfoHeight"), m_PopupEventInfoHeight);

	return true;
}


bool CStatusOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(
		hwndOwner,
		GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_OPTIONS_STATUS));
}


bool CStatusOptions::ApplyOptions()
{
	m_pStatusView->EnableSizeAdjustment(false);
	m_pStatusView->SetMultiRow(m_fMultiRow);
	m_pStatusView->SetMaxRows(m_MaxRows);
	m_pStatusView->SetFont(m_ItemFont);
	m_pStatusView->EnableSizeAdjustment(true);
	return true;
}


bool CStatusOptions::ApplyItemList()
{
	StatusItemInfoList ItemList;

	MakeItemList(&ItemList);

	std::vector<int> ItemOrder;
	ItemOrder.resize(ItemList.size());

	const int StatusBarDPI = m_pStatusView->GetStyleScaling()->GetDPI();

	for (size_t i = 0; i < ItemList.size(); i++) {
		ItemOrder[i] = ItemList[i].ID;
		CStatusItem *pItem = m_pStatusView->GetItemByID(ItemOrder[i]);
		pItem->SetVisible(ItemList[i].fVisible);
		if (ItemList[i].Width >= 0)
			pItem->SetWidth(::MulDiv(ItemList[i].Width, StatusBarDPI, m_DPI));
	}

	return m_pStatusView->SetItemOrder(ItemOrder.data());
}


void CStatusOptions::ApplyItemWidth()
{
	const int StatusBarDPI = m_pStatusView->GetStyleScaling()->GetDPI();

	for (auto &Item : m_ItemList) {
		if (Item.Width >= 0) {
			CStatusItem *pItem = m_pStatusView->GetItemByID(Item.ID);
			if (pItem != nullptr) {
				pItem->SetWidth(::MulDiv(Item.Width, StatusBarDPI, m_DPI));
				pItem->SetActualWidth(pItem->GetWidth());
			}
		}
	}
}


int CStatusOptions::RegisterItem(LPCTSTR pszID)
{
	if (IsStringEmpty(pszID))
		return -1;

	StatusItemInfo Item;

	Item.ID = m_ItemID++;
	Item.IDText = pszID;
	Item.fVisible = false;
	Item.Width = -1;

	m_AvailItemList.push_back(Item);

	for (auto &e : m_ItemList) {
		if (e.ID < 0
				&& StringUtility::IsEqualNoCase(e.IDText, Item.IDText)) {
			e.ID = Item.ID;
			break;
		}
	}

	return Item.ID;
}


bool CStatusOptions::SetItemVisibility(int ID, bool fVisible)
{
	for (auto &e : m_ItemList) {
		if (e.ID == ID) {
			e.fVisible = fVisible;
			return true;
		}
	}

	return false;
}


void CStatusOptions::InitListBox()
{
	for (auto &e : m_ItemListCur)
		DlgListBox_AddItem(m_hDlg, IDC_STATUSOPTIONS_ITEMLIST, reinterpret_cast<LPARAM>(&e));
}


INT_PTR CStatusOptions::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			m_DropInsertPos = -1;
			m_DragTimerID = 0;

			MakeItemList(&m_ItemListCur);
			m_StatusBarDPI = m_pStatusView->GetStyleScaling()->GetDPI();
			for (auto &e : m_ItemListCur) {
				const CStatusItem *pItem = m_pStatusView->GetItemByID(e.ID);
				e.fVisible = pItem->GetVisible();
				if (e.Width > 0)
					e.Width = ::MulDiv(e.Width, m_StatusBarDPI, m_DPI);
			}

			DlgListBox_SetItemHeight(hDlg, IDC_STATUSOPTIONS_ITEMLIST, 0, m_ItemHeight);
			InitListBox();
			CalcTextWidth();
			SetListHExtent();

			m_ItemListSubclass.SetSubclass(::GetDlgItem(hDlg, IDC_STATUSOPTIONS_ITEMLIST));

			m_CurSettingFont = m_ItemFont;
			StyleUtil::SetFontInfoItem(hDlg, IDC_STATUSOPTIONS_FONTINFO, m_CurSettingFont);
			DlgCheckBox_Check(hDlg, IDC_STATUSOPTIONS_MULTIROW, m_fMultiRow);
			::SetDlgItemInt(hDlg, IDC_STATUSOPTIONS_MAXROWS, m_MaxRows, TRUE);
			DlgUpDown_SetRange(hDlg, IDC_STATUSOPTIONS_MAXROWS_UPDOWN, 1, UD_MAXVAL);
			EnableDlgItems(
				hDlg,
				IDC_STATUSOPTIONS_MAXROWS_LABEL,
				IDC_STATUSOPTIONS_MAXROWS_UPDOWN,
				m_fMultiRow);

			DlgCheckBox_Check(hDlg, IDC_STATUSOPTIONS_SHOWPOPUP, m_fShowPopup);

			::SendDlgItemMessage(
				hDlg, IDC_STATUSOPTIONS_OPACITY_SLIDER,
				TBM_SETRANGE, TRUE, MAKELPARAM(OPACITY_MIN, OPACITY_MAX));
			::SendDlgItemMessage(
				hDlg, IDC_STATUSOPTIONS_OPACITY_SLIDER,
				TBM_SETPOS, TRUE, m_PopupOpacity);
			::SendDlgItemMessage(
				hDlg, IDC_STATUSOPTIONS_OPACITY_SLIDER,
				TBM_SETPAGESIZE, 0, 10);
			::SendDlgItemMessage(
				hDlg, IDC_STATUSOPTIONS_OPACITY_SLIDER,
				TBM_SETTICFREQ, 10, 0);
			DlgEdit_SetInt(hDlg, IDC_STATUSOPTIONS_OPACITY_INPUT, m_PopupOpacity);
			DlgUpDown_SetRange(hDlg, IDC_STATUSOPTIONS_OPACITY_SPIN, OPACITY_MIN, OPACITY_MAX);
			EnableDlgItems(
				hDlg, IDC_STATUSOPTIONS_OPACITY_LABEL, IDC_STATUSOPTIONS_OPACITY_UNIT,
				m_fShowPopup && Util::OS::IsWindows8OrLater());

			AddControl(IDC_STATUSOPTIONS_ITEMLIST, AlignFlag::All);
			AddControl(IDC_STATUSOPTIONS_DEFAULT, AlignFlag::BottomRight);
			AddControls(
				IDC_STATUSOPTIONS_FONTINFO_LABEL,
				IDC_STATUSOPTIONS_OPACITY_UNIT,
				AlignFlag::Bottom);

			OpenTheme();
		}
		return TRUE;

	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT pdis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

			switch (pdis->itemAction) {
			case ODA_DRAWENTIRE:
			case ODA_SELECT:
				if (static_cast<INT>(pdis->itemID) >= 0) {
					const StatusItemInfo *pItemInfo = reinterpret_cast<const StatusItemInfo*>(pdis->itemData);
					CStatusItem *pItem = m_pStatusView->GetItemByID(pItemInfo->ID);
					int TextColor, BackColor;
					RECT rc;

					if ((pdis->itemState & ODS_SELECTED) == 0) {
						TextColor = COLOR_WINDOWTEXT;
						BackColor = COLOR_WINDOW;
					} else {
						TextColor = COLOR_HIGHLIGHTTEXT;
						BackColor = COLOR_HIGHLIGHT;
					}
					DrawUtil::Fill(pdis->hDC, &pdis->rcItem, GetThemeColor(BackColor));
					const int OldBkMode = ::SetBkMode(pdis->hDC, TRANSPARENT);
					COLORREF crTextColor = GetThemeColor(TextColor);
					if (!pItemInfo->fVisible)
						crTextColor = MixColor(crTextColor, GetThemeColor(BackColor));
					const COLORREF crOldTextColor = ::SetTextColor(pdis->hDC, crTextColor);
					rc.left = pdis->rcItem.left + m_ItemMargin.Left;
					rc.top = pdis->rcItem.top + m_ItemMargin.Top;
					rc.right = rc.left + m_CheckSize.Width;
					rc.bottom = pdis->rcItem.bottom - m_ItemMargin.Bottom;
					if (m_CheckTheme.IsOpen()) {
						m_CheckTheme.DrawBackground(
							pdis->hDC, BP_CHECKBOX,
							pItemInfo->fVisible ? CBS_CHECKEDNORMAL : CBS_UNCHECKEDNORMAL, &rc);
					} else {
						::DrawFrameControl(
							pdis->hDC, &rc, DFC_BUTTON,
							DFCS_BUTTONCHECK | (pItemInfo->fVisible ? DFCS_CHECKED : 0));
					}
					rc.left = pdis->rcItem.left + m_ItemMargin.Horz() + m_CheckSize.Width;
					rc.top = pdis->rcItem.top + m_ItemMargin.Top;
					rc.right = rc.left + m_TextWidth;
					rc.bottom = pdis->rcItem.bottom - m_ItemMargin.Bottom;
					::DrawText(
						pdis->hDC, pItem->GetName(), -1, &rc,
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
					rc.left = rc.right + m_ItemMargin.Left;
					rc.right =
						rc.left + (pItemInfo->Width >= 0 ? pItemInfo->Width : pItem->GetWidth()) +
						m_pStatusView->GetItemPadding().Horz();
					const int ItemHeight = m_ItemHeight - m_ItemMargin.Vert();
					rc.top += ((rc.bottom - rc.top) - ItemHeight) / 2;
					rc.bottom = rc.top + ItemHeight;
					const HPEN hpen = ::CreatePen(PS_SOLID, 1, crTextColor);
					const HPEN hpenOld = SelectPen(pdis->hDC, hpen);
					const HBRUSH hbrOld = SelectBrush(pdis->hDC, static_cast<HBRUSH>(GetStockObject(NULL_BRUSH)));
					::Rectangle(pdis->hDC, rc.left - 1, rc.top - 1, rc.right + 1, rc.bottom + 1);
					SelectBrush(pdis->hDC, hbrOld);
					SelectPen(pdis->hDC, hpenOld);
					::DeleteObject(hpen);

					{
						DrawUtil::CFont Font;
						m_pStatusView->CreateItemPreviewFont(m_CurSettingFont, &Font);
						m_pStatusView->DrawItemPreview(pItem, pdis->hDC, rc, false, Font.GetHandle());
					}

					::SetBkMode(pdis->hDC, OldBkMode);
					::SetTextColor(pdis->hDC, crOldTextColor);
					if (static_cast<int>(pdis->itemID) == m_DropInsertPos
							|| static_cast<int>(pdis->itemID) + 1 == m_DropInsertPos)
						::PatBlt(
							pdis->hDC, pdis->rcItem.left,
							static_cast<int>(pdis->itemID) == m_DropInsertPos ?
							pdis->rcItem.top : pdis->rcItem.bottom - 1,
							pdis->rcItem.right - pdis->rcItem.left, 1, DSTINVERT);
					if ((pdis->itemState & ODS_FOCUS) == 0)
						break;
				}
				[[fallthrough]];
			case ODA_FOCUS:
				if ((pdis->itemState & ODS_NOFOCUSRECT) == 0)
					::DrawFocusRect(pdis->hDC, &pdis->rcItem);
				break;
			}
		}
		return TRUE;

	case WM_HSCROLL:
		if (reinterpret_cast<HWND>(lParam) == ::GetDlgItem(hDlg, IDC_STATUSOPTIONS_OPACITY_SLIDER)) {
			SyncEditWithTrackBar(
				hDlg,
				IDC_STATUSOPTIONS_OPACITY_SLIDER,
				IDC_STATUSOPTIONS_OPACITY_INPUT);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_STATUSOPTIONS_DEFAULT:
			DlgListBox_Clear(hDlg, IDC_STATUSOPTIONS_ITEMLIST);
			m_ItemListCur = m_AvailItemList;
			InitListBox();
			SetListHExtent();
			return TRUE;

		case IDC_STATUSOPTIONS_CHOOSEFONT:
			{
				Style::Font Font = m_CurSettingFont;

				if (StyleUtil::ChooseStyleFont(hDlg, &Font) && Font != m_CurSettingFont) {
					m_CurSettingFont = Font;
					StyleUtil::SetFontInfoItem(hDlg, IDC_STATUSOPTIONS_FONTINFO, Font);
					m_pStyleScaling->RealizeFontSize(&Font);
					DrawUtil::CFont DrawFont(Font.LogFont);
					m_ItemHeight = m_pStatusView->CalcItemHeight(DrawFont) + m_ItemMargin.Vert();
					DlgListBox_SetItemHeight(hDlg, IDC_STATUSOPTIONS_ITEMLIST, 0, m_ItemHeight);
					InvalidateDlgItem(hDlg, IDC_STATUSOPTIONS_ITEMLIST);
				}
			}
			return TRUE;

		case IDC_STATUSOPTIONS_MULTIROW:
			EnableDlgItemsSyncCheckBox(
				hDlg,
				IDC_STATUSOPTIONS_MAXROWS_LABEL,
				IDC_STATUSOPTIONS_MAXROWS_UPDOWN,
				IDC_STATUSOPTIONS_MULTIROW);
			return TRUE;

		case IDC_STATUSOPTIONS_SHOWPOPUP:
			EnableDlgItems(
				hDlg, IDC_STATUSOPTIONS_OPACITY_LABEL, IDC_STATUSOPTIONS_OPACITY_UNIT,
				DlgCheckBox_IsChecked(hDlg, IDC_STATUSOPTIONS_SHOWPOPUP) && Util::OS::IsWindows8OrLater());
			return TRUE;

		case IDC_STATUSOPTIONS_OPACITY_INPUT:
			if (HIWORD(wParam) == EN_CHANGE) {
				SyncTrackBarWithEdit(
					hDlg,
					IDC_STATUSOPTIONS_OPACITY_INPUT,
					IDC_STATUSOPTIONS_OPACITY_SLIDER);
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case PSN_APPLY:
			{
				m_pStatusView->EnableSizeAdjustment(false);

				m_ItemList.resize(m_ItemListCur.size());
				for (size_t i = 0; i < m_ItemListCur.size(); i++) {
					m_ItemList[i] = *reinterpret_cast<StatusItemInfo*>(
						DlgListBox_GetItemData(hDlg, IDC_STATUSOPTIONS_ITEMLIST, i));
				}
				m_DPI = m_StatusBarDPI;
				ApplyItemList();

				if (m_ItemFont != m_CurSettingFont) {
					m_ItemFont = m_CurSettingFont;
					m_pStatusView->SetFont(m_ItemFont);
					for (StatusItemInfo &Item : m_ItemList) {
						if (Item.Width < 0)
							Item.Width = m_pStatusView->GetItemByID(Item.ID)->GetWidth();
					}
				}

				const bool fMultiRow = DlgCheckBox_IsChecked(hDlg, IDC_STATUSOPTIONS_MULTIROW);
				if (m_fMultiRow != fMultiRow) {
					m_fMultiRow = fMultiRow;
					m_pStatusView->SetMultiRow(fMultiRow);
				}
				int MaxRows = ::GetDlgItemInt(hDlg, IDC_STATUSOPTIONS_MAXROWS, nullptr, TRUE);
				if (MaxRows < 1)
					MaxRows = 1;
				if (m_MaxRows != MaxRows) {
					m_MaxRows = MaxRows;
					m_pStatusView->SetMaxRows(MaxRows);
				}

				m_fShowPopup = DlgCheckBox_IsChecked(hDlg, IDC_STATUSOPTIONS_SHOWPOPUP);
				if (Util::OS::IsWindows8OrLater()) {
					const int Opacity = DlgEdit_GetInt(hDlg, IDC_STATUSOPTIONS_OPACITY_INPUT);
					m_PopupOpacity = std::clamp(Opacity, OPACITY_MIN, OPACITY_MAX);
				}

				m_pStatusView->EnableSizeAdjustment(true);

				m_fChanged = true;
			}
			break;
		}
		break;

	case WM_THEMECHANGED:
		m_CheckTheme.Close();
		OpenTheme();
		return TRUE;

	case WM_DESTROY:
		m_CheckTheme.Close();
		return TRUE;
	}

	return FALSE;
}


void CStatusOptions::ApplyStyle()
{
	CResizableDialog::ApplyStyle();

	if (m_hDlg != nullptr) {
		m_ItemMargin = Style::Margins(3);
		m_pStyleScaling->ToPixels(&m_ItemMargin);
		m_CheckSize = Style::Size(14, 14);
		m_pStyleScaling->ToPixels(&m_CheckSize);
		m_ItemHeight = m_pStatusView->GetItemHeight() + m_ItemMargin.Vert();
	}
}


void CStatusOptions::RealizeStyle()
{
	CResizableDialog::RealizeStyle();

	if (m_hDlg != nullptr) {
		if (DlgListBox_GetCount(m_hDlg, IDC_STATUSOPTIONS_ITEMLIST) > 0) {
			DlgListBox_SetItemHeight(m_hDlg, IDC_STATUSOPTIONS_ITEMLIST, 0, m_ItemHeight);
			CalcTextWidth();
			SetListHExtent();
		}
	}
}


void CStatusOptions::CalcTextWidth()
{
	const HWND hwndList = GetDlgItem(m_hDlg, IDC_STATUSOPTIONS_ITEMLIST);
	const HDC hdc = GetDC(hwndList);
	if (hdc == nullptr)
		return;
	const HFONT hfontOld = SelectFont(hdc, GetWindowFont(hwndList));
	const int Count = ListBox_GetCount(hwndList);
	int MaxWidth = 0;
	for (int i = 0; i < Count; i++) {
		const CStatusItem *pItem = m_pStatusView->GetItemByID(m_ItemListCur[i].ID);
		SIZE sz;
		GetTextExtentPoint32(hdc, pItem->GetName(), lstrlen(pItem->GetName()), &sz);
		if (sz.cx > MaxWidth)
			MaxWidth = sz.cx;
	}
	SelectFont(hdc, hfontOld);
	ReleaseDC(hwndList, hdc);
	m_TextWidth = MaxWidth;
}


void CStatusOptions::SetListHExtent()
{
	const HWND hwndList = ::GetDlgItem(m_hDlg, IDC_STATUSOPTIONS_ITEMLIST);
	int MaxWidth = 0;

	for (const StatusItemInfo &Item : m_ItemListCur) {
		const int Width = Item.Width >= 0 ? Item.Width : m_pStatusView->GetItemByID(Item.ID)->GetWidth();
		if (Width > MaxWidth)
			MaxWidth = Width;
	}

	ListBox_SetHorizontalExtent(
		hwndList,
		m_ItemMargin.Horz() + m_CheckSize.Width + m_TextWidth +
		m_ItemMargin.Horz() + MaxWidth + m_pStatusView->GetItemPadding().Horz());
}


static int ListBox_GetHitItem(HWND hwndList, int x, int y)
{
	const int Index = ListBox_GetTopIndex(hwndList) + y / ListBox_GetItemHeight(hwndList, 0);
	if (Index < 0 || Index >= ListBox_GetCount(hwndList))
		return -1;
	return Index;
}


static void ListBox_MoveItem(HWND hwndList, int From, int To)
{
	const int Top = ListBox_GetTopIndex(hwndList);
	const LPARAM lData = ListBox_GetItemData(hwndList, From);

	ListBox_DeleteString(hwndList, From);
	ListBox_InsertItemData(hwndList, To, lData);
	ListBox_SetCurSel(hwndList, To);
	ListBox_SetTopIndex(hwndList, Top);
}


static void ListBox_EnsureVisible(HWND hwndList, int Index)
{
	const int Top = ListBox_GetTopIndex(hwndList);

	if (Index < Top) {
		ListBox_SetTopIndex(hwndList, Index);
	} else if (Index > Top) {
		RECT rc;
		int Rows;

		GetClientRect(hwndList, &rc);
		Rows = rc.bottom / ListBox_GetItemHeight(hwndList, 0);
		if (Rows == 0)
			Rows = 1;
		if (Index >= Top + Rows)
			ListBox_SetTopIndex(hwndList, Index - Rows + 1);
	}
}


void CStatusOptions::DrawInsertMark(HWND hwndList, int Pos)
{
	const HDC hdc = GetDC(hwndList);
	RECT rc;
	GetClientRect(hwndList, &rc);
	rc.top = (Pos - ListBox_GetTopIndex(hwndList)) * m_ItemHeight - 1;
	PatBlt(hdc, 0, rc.top, rc.right - rc.left, 2, DSTINVERT);
	ReleaseDC(hwndList, hdc);
}


bool CStatusOptions::GetItemPreviewRect(HWND hwndList, int Index, RECT *pRect)
{
	if (Index < 0)
		return false;
	const StatusItemInfo *pItemInfo = reinterpret_cast<const StatusItemInfo*>(ListBox_GetItemData(hwndList, Index));
	RECT rc;
	ListBox_GetItemRect(hwndList, Index, &rc);
	OffsetRect(&rc, -GetScrollPos(hwndList, SB_HORZ), 0);
	rc.left += m_ItemMargin.Horz() + m_CheckSize.Width + m_TextWidth + m_ItemMargin.Left;
	rc.right =
		rc.left +
		(pItemInfo->Width >= 0 ?
			pItemInfo->Width :
			m_pStatusView->GetItemByID(pItemInfo->ID)->GetWidth()) +
		m_pStatusView->GetItemPadding().Horz();
	rc.top += m_ItemMargin.Top;
	rc.bottom -= m_ItemMargin.Bottom;
	*pRect = rc;
	return true;
}


bool CStatusOptions::IsCursorResize(HWND hwndList, int x, int y)
{
	RECT rc;

	if (!GetItemPreviewRect(hwndList, ListBox_GetHitItem(hwndList, x, y), &rc))
		return false;
	const int Margin = (GetSystemMetricsWithDPI(SM_CXSIZEFRAME, m_CurrentDPI) + 1) / 2;
	return x >= rc.right - Margin && x <= rc.right + Margin;
}


void CStatusOptions::MakeItemList(StatusItemInfoList *pList) const
{
	pList->clear();
	pList->reserve(m_AvailItemList.size());

	for (const auto &e : m_ItemList) {
		if (e.ID >= 0)
			pList->push_back(e);
	}

	if (pList->size() < m_AvailItemList.size()) {
		for (const auto &e : m_AvailItemList) {
			const int ID = e.ID;
			bool fFound = false;
			for (size_t j = 0; j < pList->size(); j++) {
				if ((*pList)[j].ID == ID) {
					fFound = true;
					break;
				}
			}
			if (!fFound) {
				pList->push_back(e);
				const CStatusItem *pItem = m_pStatusView->GetItemByID(ID);
				if (pItem != nullptr)
					pList->back().fVisible = pItem->GetVisible();
			}
		}
	}
}


void CStatusOptions::OpenTheme()
{
	if (m_CheckTheme.IsActive())
		m_CheckTheme.Open(::GetDlgItem(m_hDlg, IDC_STATUSOPTIONS_ITEMLIST), VSCLASS_BUTTON, m_CurrentDPI);
}


CStatusOptions::CItemListSubclass::CItemListSubclass(CStatusOptions *pStatusOptions)
	: m_pStatusOptions(pStatusOptions)
{
}


LRESULT CStatusOptions::CItemListSubclass::OnMessage(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_LBUTTONDOWN:
		{
			const int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
			const int Sel = ListBox_GetHitItem(hwnd, x, y);

			SetFocus(hwnd);

			if (Sel >= 0) {
				RECT rcItem;
				ListBox_GetItemRect(hwnd, Sel, &rcItem);
				RECT rc = rcItem;
				OffsetRect(&rc, -GetScrollPos(hwnd, SB_HORZ), 0);
				if (x >= rc.left + m_pStatusOptions->m_ItemMargin.Left
						&& x < rc.left + m_pStatusOptions->m_ItemMargin.Left + m_pStatusOptions->m_CheckSize.Width) {
					StatusItemInfo *pItemInfo = reinterpret_cast<StatusItemInfo*>(ListBox_GetItemData(hwnd, Sel));
					pItemInfo->fVisible = !pItemInfo->fVisible;
					InvalidateRect(hwnd, &rcItem, TRUE);
				} else {
					if (ListBox_GetCurSel(hwnd) != Sel)
						ListBox_SetCurSel(hwnd, Sel);
					SetCapture(hwnd);
					m_pStatusOptions->m_fDragResize = m_pStatusOptions->IsCursorResize(hwnd, x, y);
				}
			}
		}
		return 0;

	case WM_LBUTTONUP:
		if (GetCapture() == hwnd)
			ReleaseCapture();
		return 0;

	case WM_CAPTURECHANGED:
		if (m_pStatusOptions->m_DragTimerID != 0) {
			KillTimer(hwnd, m_pStatusOptions->m_DragTimerID);
			m_pStatusOptions->m_DragTimerID = 0;
		}
		if (m_pStatusOptions->m_DropInsertPos >= 0) {
			m_pStatusOptions->DrawInsertMark(hwnd, m_pStatusOptions->m_DropInsertPos);
			const int From = ListBox_GetCurSel(hwnd);
			int To = m_pStatusOptions->m_DropInsertPos;
			if (To > From)
				To--;
			SetWindowRedraw(hwnd, FALSE);
			ListBox_MoveItem(hwnd, From, To);
			SetWindowRedraw(hwnd, TRUE);
			m_pStatusOptions->m_DropInsertPos = -1;
		}
		return 0;

	case WM_MOUSEMOVE:
		if (GetCapture() == hwnd) {
			const int y = GET_Y_LPARAM(lParam);
			RECT rc;

			GetClientRect(hwnd, &rc);
			if (m_pStatusOptions->m_fDragResize) {
				const int x = GET_X_LPARAM(lParam);
				const int Sel = ListBox_GetCurSel(hwnd);
				RECT rc;

				if (m_pStatusOptions->GetItemPreviewRect(hwnd, Sel, &rc)) {
					StatusItemInfo *pItemInfo = reinterpret_cast<StatusItemInfo*>(ListBox_GetItemData(hwnd, Sel));
					int Width = (x - rc.left) - m_pStatusOptions->m_pStatusView->GetItemPadding().Horz();
					const CStatusItem *pItem = m_pStatusOptions->m_pStatusView->GetItemByID(pItemInfo->ID);
					const int MinWidth = pItem->GetMinWidth();
					const int MaxWidth = pItem->GetMaxWidth();

					if (Width < MinWidth)
						Width = MinWidth;
					else if (MaxWidth > 0 && Width > MaxWidth)
						Width = MaxWidth;
					pItemInfo->Width = Width;
					ListBox_GetItemRect(hwnd, Sel, &rc);
					InvalidateRect(hwnd, &rc, TRUE);
					m_pStatusOptions->SetListHExtent();
				}
			} else if (y >= 0 && y < rc.bottom) {
				if (m_pStatusOptions->m_DragTimerID != 0) {
					KillTimer(hwnd, m_pStatusOptions->m_DragTimerID);
					m_pStatusOptions->m_DragTimerID = 0;
				}
				int Insert =
					ListBox_GetTopIndex(hwnd) +
					(y + m_pStatusOptions->m_ItemHeight / 2) / m_pStatusOptions->m_ItemHeight;
				const int Count = ListBox_GetCount(hwnd);
				if (Insert > Count) {
					Insert = Count;
				} else {
					const int Sel = ListBox_GetCurSel(hwnd);
					if (Insert == Sel || Insert == Sel + 1)
						Insert = -1;
				}
				if (m_pStatusOptions->m_DropInsertPos >= 0)
					m_pStatusOptions->DrawInsertMark(hwnd, m_pStatusOptions->m_DropInsertPos);
				m_pStatusOptions->m_DropInsertPos = Insert;
				if (m_pStatusOptions->m_DropInsertPos >= 0)
					m_pStatusOptions->DrawInsertMark(hwnd, m_pStatusOptions->m_DropInsertPos);
				SetCursor(LoadCursor(nullptr, IDC_ARROW));
			} else {
				UINT TimerID;

				if (m_pStatusOptions->m_DropInsertPos >= 0) {
					m_pStatusOptions->DrawInsertMark(hwnd, m_pStatusOptions->m_DropInsertPos);
					m_pStatusOptions->m_DropInsertPos = -1;
				}
				if (y < 0)
					TimerID = TIMER_ID_UP;
				else
					TimerID = TIMER_ID_DOWN;
				if (TimerID != m_pStatusOptions->m_DragTimerID) {
					if (m_pStatusOptions->m_DragTimerID != 0)
						KillTimer(hwnd, m_pStatusOptions->m_DragTimerID);
					m_pStatusOptions->m_DragTimerID = static_cast<UINT>(SetTimer(hwnd, TimerID, 100, nullptr));
				}
				SetCursor(LoadCursor(nullptr, IDC_NO));
			}
		} else {
			const int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

			SetCursor(LoadCursor(nullptr, m_pStatusOptions->IsCursorResize(hwnd, x, y) ? IDC_SIZEWE : IDC_ARROW));
		}
		return 0;

	case WM_RBUTTONDOWN:
		if (GetCapture() == hwnd) {
			ReleaseCapture();
			if (m_pStatusOptions->m_DragTimerID != 0) {
				KillTimer(hwnd, m_pStatusOptions->m_DragTimerID);
				m_pStatusOptions->m_DragTimerID = 0;
			}
			if (m_pStatusOptions->m_DropInsertPos >= 0) {
				m_pStatusOptions->DrawInsertMark(hwnd, m_pStatusOptions->m_DropInsertPos);
				m_pStatusOptions->m_DropInsertPos = -1;
			}
		}
		return 0;

	case WM_TIMER:
		{
			int Pos = ListBox_GetTopIndex(hwnd);
			if (wParam == TIMER_ID_UP) {
				if (Pos > 0)
					Pos--;
			} else
				Pos++;
			ListBox_SetTopIndex(hwnd, Pos);
		}
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT)
			return TRUE;
		break;
	}

	return CWindowSubclass::OnMessage(hwnd, uMsg, wParam, lParam);
}


} // namespace TVTest
