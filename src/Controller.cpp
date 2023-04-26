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
#include "Controller.h"
#include "Command.h"
#include "DialogUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


bool CController::TranslateMessage(HWND hwnd, MSG *pMessage)
{
	return false;
}


HBITMAP CController::GetImage(ImageType Type) const
{
	return nullptr;
}


bool CController::GetIniFileName(LPTSTR pszFileName, int MaxLength) const
{
	LPCTSTR pszIniFileName = GetAppClass().GetIniFileName();

	if (::lstrlen(pszIniFileName) >= MaxLength)
		return false;
	StringCopy(pszFileName, pszIniFileName);
	return true;
}


LPCTSTR CController::GetIniFileSection() const
{
	return GetName();
}


void CController::SetEventHandler(CEventHandler *pEventHandler)
{
	m_pEventHandler = pEventHandler;
}


bool CController::OnButtonDown(int Index)
{
	if (m_pEventHandler == nullptr || Index < 0 || Index >= NumButtons())
		return false;
	return m_pEventHandler->OnButtonDown(this, Index);
}




CControllerManager::~CControllerManager()
{
	Destroy();
	DeleteAllControllers();
}


bool CControllerManager::ReadSettings(CSettings &Settings)
{
	TCHAR szText[256];

	if (Settings.Read(TEXT("CurController"), szText, lengthof(szText)))
		m_CurController = szText;
	return true;
}


bool CControllerManager::WriteSettings(CSettings &Settings)
{
	if (!m_CurController.empty())
		Settings.Write(TEXT("CurController"), m_CurController);
	return true;
}


bool CControllerManager::Create(HWND hwndOwner)
{
	return CreateDialogWindow(
		hwndOwner,
		GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_OPTIONS_CONTROLLER));
}


bool CControllerManager::AddController(CController *pController)
{
	if (pController == nullptr || FindController(pController->GetName()) >= 0)
		return false;
	m_ControllerList.emplace_back(pController);
	pController->SetEventHandler(this);
	ControllerInfo &Info = m_ControllerList[m_ControllerList.size() - 1];
	const int NumButtons = pController->NumButtons();
	Info.Settings.AssignList.resize(NumButtons);
	Info.Settings.fActiveOnly = pController->IsActiveOnly();
	for (int i = 0; i < NumButtons; i++) {
		CController::ButtonInfo Button;

		if (pController->GetButtonInfo(i, &Button)) {
			Info.Settings.AssignList[i] = static_cast<WORD>(Button.DefaultCommand);
		}
	}
	return true;
}


bool CControllerManager::DeleteController(LPCTSTR pszName)
{
	if (pszName == nullptr)
		return false;
	for (std::vector<ControllerInfo>::iterator itr = m_ControllerList.begin();
			itr != m_ControllerList.end(); ++itr) {
		if (::lstrcmpi(itr->Controller->GetName(), pszName) == 0) {
			if (itr->fSettingsChanged)
				SaveControllerSettings(pszName);
			m_ControllerList.erase(itr);
			return true;
		}
	}
	return false;
}


void CControllerManager::DeleteAllControllers()
{
	for (auto &e : m_ControllerList) {
		if (e.fSettingsChanged)
			SaveControllerSettings(e.Controller->GetName());
	}
	m_ControllerList.clear();
}


bool CControllerManager::IsControllerEnabled(LPCTSTR pszName) const
{
	const int Index = FindController(pszName);
	if (Index < 0)
		return false;
	return m_ControllerList[Index].Controller->IsEnabled();
}


bool CControllerManager::LoadControllerSettings(LPCTSTR pszName)
{
	const int Index = FindController(pszName);
	if (Index < 0)
		return false;

	ControllerInfo &Info = m_ControllerList[Index];

	if (Info.fSettingsLoaded)
		return true;

	CSettings Settings;
	TCHAR szFileName[MAX_PATH];

	if (!Info.Controller->GetIniFileName(szFileName, lengthof(szFileName)))
		return false;
	if (Settings.Open(szFileName, CSettings::OpenFlag::Read)
			&& Settings.SetSection(Info.Controller->GetIniFileSection())) {
		const int NumButtons = Info.Controller->NumButtons();
		const CCommandManager &CommandManager = GetAppClass().CommandManager;
		String Command;

		for (int i = 0; i < NumButtons; i++) {
			TCHAR szName[64];

			StringFormat(szName, TEXT("Button{}_Command"), i);
			if (Settings.Read(szName, &Command) && !Command.empty()) {
				Info.Settings.AssignList[i] = CommandManager.ParseIDText(Command);
			}
		}
		if (!Info.Controller->IsActiveOnly())
			Settings.Read(TEXT("ActiveOnly"), &Info.Settings.fActiveOnly);
		Info.fSettingsLoaded = true;
	}
	return true;
}


bool CControllerManager::SaveControllerSettings(LPCTSTR pszName) const
{
	const int Index = FindController(pszName);
	if (Index < 0)
		return false;

	const ControllerInfo &Info = m_ControllerList[Index];
	if (!Info.fSettingsChanged)
		return true;

	CSettings Settings;
	TCHAR szFileName[MAX_PATH];

	if (!Info.Controller->GetIniFileName(szFileName, lengthof(szFileName)))
		return false;
	if (Settings.Open(szFileName, CSettings::OpenFlag::Write)
			&& Settings.SetSection(Info.Controller->GetIniFileSection())) {
		const int NumButtons = Info.Controller->NumButtons();
		const CCommandManager &CommandManager = GetAppClass().CommandManager;
		String Text;

		for (int i = 0; i < NumButtons; i++) {
			TCHAR szName[64];

			StringFormat(szName, TEXT("Button{}_Command"), i);
			if (Info.Settings.AssignList[i] != 0)
				Text = CommandManager.GetCommandIDText(Info.Settings.AssignList[i]);
			else
				Text.clear();
			Settings.Write(szName, Text);
		}
		if (!Info.Controller->IsActiveOnly())
			Settings.Write(TEXT("ActiveOnly"), Info.Settings.fActiveOnly);
	}
	return true;
}


bool CControllerManager::TranslateMessage(HWND hwnd, MSG *pMessage)
{
	for (auto &e : m_ControllerList) {
		if (e.Controller->IsEnabled()) {
			if (e.Controller->TranslateMessage(hwnd, pMessage))
				return true;
		}
	}
	return false;
}


bool CControllerManager::OnActiveChange(HWND hwnd, bool fActive)
{
	m_fActive = fActive;
	if (fActive)
		OnFocusChange(hwnd, true);
	return true;
}


bool CControllerManager::OnFocusChange(HWND hwnd, bool fFocus)
{
	m_fFocus = fFocus;
	if (fFocus) {
		for (auto &e : m_ControllerList)
			e.Controller->SetTargetWindow(hwnd);
	}
	return true;
}


bool CControllerManager::OnButtonDown(LPCTSTR pszName, int Button) const
{
	if (pszName == nullptr || Button < 0)
		return false;

	const int Index = FindController(pszName);
	if (Index < 0)
		return false;
	const ControllerInfo &Info = m_ControllerList[Index];
	if (!m_fActive && Info.Settings.fActiveOnly)
		return false;
	if (Button >= static_cast<int>(Info.Settings.AssignList.size()))
		return false;
	const WORD Command = Info.Settings.AssignList[Button];
	if (Command != 0)
		::PostMessage(GetAppClass().UICore.GetMainWindow(), WM_COMMAND, Command, 0);
	return true;
}


bool CControllerManager::OnButtonDown(CController *pController, int Index)
{
	if (Index < 0)
		return false;

	for (auto &e : m_ControllerList) {
		if (e.Controller.get() == pController) {
			if (Index >= static_cast<int>(e.Settings.AssignList.size()))
				return false;
			const WORD Command = e.Settings.AssignList[Index];
			if (Command != 0)
				::PostMessage(GetAppClass().UICore.GetMainWindow(), WM_COMMAND, Command, 0);
			return true;
		}
	}
	return false;
}


const CControllerManager::ControllerSettings *CControllerManager::GetControllerSettings(LPCTSTR pszName) const
{
	const int Index = FindController(pszName);

	if (Index < 0)
		return nullptr;
	return &m_ControllerList[Index].Settings;
}


int CControllerManager::FindController(LPCTSTR pszName) const
{
	if (pszName == nullptr)
		return -1;
	for (size_t i = 0; i < m_ControllerList.size(); i++) {
		if (::lstrcmpi(m_ControllerList[i].Controller->GetName(), pszName) == 0)
			return static_cast<int>(i);
	}
	return -1;
}


void CControllerManager::InitDlgItems()
{
	const HWND hwndList = ::GetDlgItem(m_hDlg, IDC_CONTROLLER_ASSIGN);
	ListView_DeleteAllItems(hwndList);

	if (m_hbmController != nullptr) {
		::DeleteObject(m_hbmController);
		m_hbmController = nullptr;
	}
	if (m_hbmSelButtons != nullptr) {
		::DeleteObject(m_hbmSelButtons);
		m_hbmSelButtons = nullptr;
	}
	m_Tooltip.DeleteAllTools();

	const int Sel = static_cast<int>(DlgComboBox_GetCurSel(m_hDlg, IDC_CONTROLLER_LIST));
	if (Sel >= 0) {
		const CCommandManager &CommandManager = GetAppClass().CommandManager;
		const ControllerInfo &Info = m_ControllerList[Sel];
		const CController *pController = Info.Controller.get();
		const int NumButtons = pController->NumButtons();

		if (!Info.fSettingsLoaded) {
			if (LoadControllerSettings(pController->GetName()))
				m_CurSettingsList[Sel] = Info.Settings;
		}

		const bool fActiveOnly = pController->IsActiveOnly();
		EnableDlgItem(m_hDlg, IDC_CONTROLLER_ACTIVEONLY, !fActiveOnly);
		DlgCheckBox_Check(
			m_hDlg, IDC_CONTROLLER_ACTIVEONLY,
			fActiveOnly || m_CurSettingsList[Sel].fActiveOnly);

		for (int i = 0; i < NumButtons; i++) {
			CController::ButtonInfo Button;
			const int Command = m_CurSettingsList[Sel].AssignList[i];
			LVITEM lvi;

			pController->GetButtonInfo(i, &Button);
			lvi.mask = LVIF_TEXT | LVIF_PARAM;
			lvi.iItem = i;
			lvi.iSubItem = 0;
			lvi.pszText = const_cast<LPTSTR>(Button.pszName);
			lvi.lParam = Command;
			lvi.iItem = ListView_InsertItem(hwndList, &lvi);
			if (Command > 0) {
				TCHAR szText[CCommandManager::MAX_COMMAND_TEXT];

				lvi.mask = LVIF_TEXT;
				lvi.iSubItem = 1;
				CommandManager.GetCommandText(Command, szText, lengthof(szText));
				lvi.pszText = szText;
				ListView_SetItem(hwndList, &lvi);
			}
		}
		for (int i = 0; i < 2; i++)
			ListView_SetColumnWidth(hwndList, i, LVSCW_AUTOSIZE_USEHEADER);

		m_hbmController = pController->GetImage(CController::ImageType::Controller);
		if (m_hbmController != nullptr) {
			BITMAP bm;
			::GetObject(m_hbmController, sizeof(BITMAP), &bm);
			RECT rc;
			::GetWindowRect(::GetDlgItem(m_hDlg, IDC_CONTROLLER_IMAGEPLACE), &rc);
			MapWindowRect(nullptr, m_hDlg, &rc);
			m_ImageRect.left = rc.left + ((rc.right - rc.left) - bm.bmWidth) / 2;
			m_ImageRect.top = rc.top + ((rc.bottom - rc.top) - bm.bmHeight) / 2;
			m_ImageRect.right = m_ImageRect.left + bm.bmWidth;
			m_ImageRect.bottom = m_ImageRect.top + bm.bmHeight;

			m_hbmSelButtons = pController->GetImage(CController::ImageType::SelButtons);

			if (m_hbmSelButtons != nullptr) {
				for (int i = 0; i < NumButtons; i++) {
					CController::ButtonInfo Button;

					pController->GetButtonInfo(i, &Button);
					rc.left = m_ImageRect.left + Button.ImageButtonRect.Left;
					rc.top = m_ImageRect.top + Button.ImageButtonRect.Top;
					rc.right = rc.left + Button.ImageButtonRect.Width;
					rc.bottom = rc.top + Button.ImageButtonRect.Height;
					m_Tooltip.AddTool(i, rc, const_cast<LPTSTR>(Button.pszName));
				}
			}
		}
	}
	EnableDlgItems(m_hDlg, IDC_CONTROLLER_ACTIVEONLY, IDC_CONTROLLER_DEFAULT, Sel >= 0);
	DlgComboBox_SetCurSel(m_hDlg, IDC_CONTROLLER_COMMAND, 0);
	EnableDlgItem(m_hDlg, IDC_CONTROLLER_COMMAND, false);
	::InvalidateRect(m_hDlg, nullptr, TRUE);
}


void CControllerManager::SetButtonCommand(HWND hwndList, int Index, int Command)
{
	const int CurController = static_cast<int>(DlgComboBox_GetCurSel(m_hDlg, IDC_CONTROLLER_LIST));
	if (CurController < 0)
		return;

	LVITEM lvi;
	TCHAR szText[CCommandManager::MAX_COMMAND_TEXT];

	lvi.mask = LVIF_PARAM;
	lvi.iItem = Index;
	lvi.iSubItem = 0;
	lvi.lParam = Command;
	ListView_SetItem(hwndList, &lvi);
	lvi.mask = LVIF_TEXT;
	lvi.iSubItem = 1;
	lvi.pszText = szText;
	if (Command > 0) {
		GetAppClass().CommandManager.GetCommandText(Command, szText, lengthof(szText));
	} else {
		szText[0] = TEXT('\0');
	}
	ListView_SetItem(hwndList, &lvi);
	m_CurSettingsList[CurController].AssignList[Index] = static_cast<WORD>(Command);
}


void CControllerManager::SetDlgItemStatus()
{
	const HWND hwndList = ::GetDlgItem(m_hDlg, IDC_CONTROLLER_ASSIGN);
	const int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

	if (Sel >= 0) {
		LVITEM lvi;

		lvi.mask = LVIF_PARAM;
		lvi.iItem = Sel;
		lvi.iSubItem = 0;
		ListView_GetItem(hwndList, &lvi);
		if (lvi.lParam == 0) {
			DlgComboBox_SetCurSel(m_hDlg, IDC_CONTROLLER_COMMAND, 0);
		} else {
			const LRESULT Count = DlgComboBox_GetCount(m_hDlg, IDC_CONTROLLER_COMMAND);
			for (LRESULT i = 1; i < Count; i++) {
				if (DlgComboBox_GetItemData(m_hDlg, IDC_CONTROLLER_COMMAND, i) == lvi.lParam) {
					DlgComboBox_SetCurSel(m_hDlg, IDC_CONTROLLER_COMMAND, i);
					break;
				}
			}
		}
	} else {
		DlgComboBox_SetCurSel(m_hDlg, IDC_CONTROLLER_COMMAND, 0);
	}
	EnableDlgItem(m_hDlg, IDC_CONTROLLER_COMMAND, Sel >= 0);
}


CController *CControllerManager::GetCurController() const
{
	const int Sel = static_cast<int>(DlgComboBox_GetCurSel(m_hDlg, IDC_CONTROLLER_LIST));

	if (Sel < 0 || Sel >= static_cast<int>(m_ControllerList.size()))
		return nullptr;
	return m_ControllerList[Sel].Controller.get();
}


INT_PTR CControllerManager::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			const size_t NumControllers = m_ControllerList.size();
			if (NumControllers > 0) {
				m_CurSettingsList.resize(NumControllers);
				int Sel = 0;
				for (size_t i = 0; i < NumControllers; i++) {
					const ControllerInfo &Info = m_ControllerList[i];

					DlgComboBox_AddString(hDlg, IDC_CONTROLLER_LIST, Info.Controller->GetText());
					if (!m_CurController.empty()
							&& ::lstrcmpi(m_CurController.c_str(), Info.Controller->GetName()) == 0)
						Sel = static_cast<int>(i);
					m_CurSettingsList[i] = Info.Settings;
				}
				DlgComboBox_SetCurSel(hDlg, IDC_CONTROLLER_LIST, Sel);
			}
			EnableDlgItem(hDlg, IDC_CONTROLLER_LIST, NumControllers > 0);

			const HWND hwndList = ::GetDlgItem(hDlg, IDC_CONTROLLER_ASSIGN);
			ListView_SetExtendedListViewStyle(
				hwndList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP);
			SetListViewTooltipsTopMost(hwndList);

			LVCOLUMN lvc;
			lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			lvc.fmt = LVCFMT_LEFT;
			lvc.cx = 120;
			lvc.pszText = const_cast<LPTSTR>(TEXT("ボタン"));
			ListView_InsertColumn(hwndList, 0, &lvc);
			lvc.pszText = const_cast<LPTSTR>(TEXT("コマンド"));
			ListView_InsertColumn(hwndList, 1, &lvc);

			const CCommandManager &CommandManager = GetAppClass().CommandManager;
			CCommandManager::CCommandLister CommandLister(CommandManager);
			TCHAR szText[CCommandManager::MAX_COMMAND_TEXT];
			int Command;
			DlgComboBox_AddString(hDlg, IDC_CONTROLLER_COMMAND, TEXT("なし"));
			while ((Command = CommandLister.Next()) != 0) {
				CommandManager.GetCommandText(Command, szText, lengthof(szText));
				const LRESULT Index = DlgComboBox_AddString(hDlg, IDC_CONTROLLER_COMMAND, szText);
				DlgComboBox_SetItemData(hDlg, IDC_CONTROLLER_COMMAND, Index, Command);
			}

			m_Tooltip.Create(hDlg);
			m_Tooltip.SetFont(GetWindowFont(hDlg));

			InitDlgItems();
			SetDlgItemStatus();

			AddControls({
				{IDC_CONTROLLER_ASSIGN,  AlignFlag::All},
				{IDC_CONTROLLER_COMMAND, AlignFlag::HorzBottom},
				{IDC_CONTROLLER_DEFAULT, AlignFlag::Right},
			});
		}
		return TRUE;

	case WM_PAINT:
		{
			if (m_hbmController == nullptr)
				break;

			const CController *pController = GetCurController();
			if (pController == nullptr)
				break;

			const int CurButton = ListView_GetNextItem(::GetDlgItem(hDlg, IDC_CONTROLLER_ASSIGN), -1, LVNI_SELECTED);

			PAINTSTRUCT ps;
			BITMAP bm;
			RECT rc;

			::BeginPaint(hDlg, &ps);
			::GetObject(m_hbmController, sizeof(BITMAP), &bm);
			::GetWindowRect(::GetDlgItem(hDlg, IDC_CONTROLLER_IMAGEPLACE), &rc);
			MapWindowRect(nullptr, hDlg, &rc);
			if (m_fDarkMode)
				DrawUtil::Fill(ps.hdc, &rc, GetThemeColor(COLOR_3DFACE));
			else
				::FillRect(ps.hdc, &rc, static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)));
			const HDC hdcMem = ::CreateCompatibleDC(ps.hdc);
			const HBITMAP hbmOld = static_cast<HBITMAP>(::SelectObject(hdcMem, m_hbmController));
			::BitBlt(
				ps.hdc, m_ImageRect.left, m_ImageRect.top,
				bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);
			if (CurButton >= 0 && m_hbmSelButtons != nullptr) {
				CController::ButtonInfo Button;

				pController->GetButtonInfo(CurButton, &Button);
				::SelectObject(hdcMem, m_hbmSelButtons);
				::GdiTransparentBlt(
					ps.hdc,
					m_ImageRect.left + Button.ImageButtonRect.Left,
					m_ImageRect.top + Button.ImageButtonRect.Top,
					Button.ImageButtonRect.Width,
					Button.ImageButtonRect.Height,
					hdcMem,
					Button.ImageSelButtonPos.Left,
					Button.ImageSelButtonPos.Top,
					Button.ImageButtonRect.Width,
					Button.ImageButtonRect.Height,
					RGB(255, 0, 255));
			}
			::SelectObject(hdcMem, hbmOld);
			::DeleteDC(hdcMem);
			::EndPaint(hDlg, &ps);
		}
		return TRUE;

	case WM_LBUTTONDOWN:
		{
			const POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

			if (m_hbmSelButtons != nullptr
					&& ::PtInRect(&m_ImageRect, pt)) {
				const CController *pController = GetCurController();
				if (pController == nullptr)
					return TRUE;

				const int NumButtons = pController->NumButtons();
				for (int i = 0; i < NumButtons; i++) {
					CController::ButtonInfo Button;
					RECT rc;

					pController->GetButtonInfo(i, &Button);
					rc.left = m_ImageRect.left + Button.ImageButtonRect.Left;
					rc.top = m_ImageRect.top + Button.ImageButtonRect.Top;
					rc.right = rc.left + Button.ImageButtonRect.Width;
					rc.bottom = rc.top + Button.ImageButtonRect.Height;
					if (::PtInRect(&rc, pt)) {
						const HWND hwndList = ::GetDlgItem(hDlg, IDC_CONTROLLER_ASSIGN);

						ListView_SetItemState(
							hwndList, i,
							LVIS_FOCUSED | LVIS_SELECTED,
							LVIS_FOCUSED | LVIS_SELECTED);
						ListView_EnsureVisible(hwndList, i, FALSE);
						SetDlgItemFocus(hDlg, IDC_CONTROLLER_ASSIGN);
						break;
					}
				}
			}
		}
		return TRUE;

	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT) {
			if (m_hbmSelButtons != nullptr) {
				POINT pt;

				::GetCursorPos(&pt);
				::ScreenToClient(hDlg, &pt);
				if (::PtInRect(&m_ImageRect, pt)) {
					const CController *pController = GetCurController();
					if (pController == nullptr)
						break;

					const int NumButtons = pController->NumButtons();
					for (int i = 0; i < NumButtons; i++) {
						CController::ButtonInfo Button;
						RECT rc;

						pController->GetButtonInfo(i, &Button);
						rc.left = m_ImageRect.left + Button.ImageButtonRect.Left;
						rc.top = m_ImageRect.top + Button.ImageButtonRect.Top;
						rc.right = rc.left + Button.ImageButtonRect.Width;
						rc.bottom = rc.top + Button.ImageButtonRect.Height;
						if (::PtInRect(&rc, pt)) {
							::SetCursor(::LoadCursor(nullptr, IDC_HAND));
							::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
							return TRUE;
						}
					}
				}
			}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CONTROLLER_LIST:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				const CController *pCurController = GetCurController();

				InitDlgItems();
				if (pCurController != nullptr) {
					m_CurController = pCurController->GetName();
					m_fChanged = true;
				}
			}
			return TRUE;

		case IDC_CONTROLLER_ACTIVEONLY:
			{
				const int CurController = static_cast<int>(DlgComboBox_GetCurSel(hDlg, IDC_CONTROLLER_LIST));

				if (CurController >= 0) {
					m_CurSettingsList[CurController].fActiveOnly =
						DlgCheckBox_IsChecked(hDlg, IDC_CONTROLLER_ACTIVEONLY);
				}
			}
			return TRUE;

		case IDC_CONTROLLER_COMMAND:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				const HWND hwndList = ::GetDlgItem(hDlg, IDC_CONTROLLER_ASSIGN);
				const int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

				if (Sel >= 0) {
					const int Command = static_cast<int>(DlgComboBox_GetCurSel(hDlg, IDC_CONTROLLER_COMMAND));

					SetButtonCommand(
						hwndList, Sel,
						Command <= 0 ? 0 : static_cast<int>(DlgComboBox_GetItemData(hDlg, IDC_CONTROLLER_COMMAND, Command)));
				}
			}
			return TRUE;

		case IDC_CONTROLLER_DEFAULT:
			{
				const CController *pController = GetCurController();
				if (pController == nullptr)
					return TRUE;

				const HWND hwndList = ::GetDlgItem(hDlg, IDC_CONTROLLER_ASSIGN);
				const int NumButtons = pController->NumButtons();
				for (int i = 0; i < NumButtons; i++) {
					CController::ButtonInfo Button;

					pController->GetButtonInfo(i, &Button);
					SetButtonCommand(hwndList, i, Button.DefaultCommand);
				}
				SetDlgItemStatus();
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case LVN_ITEMCHANGED:
			SetDlgItemStatus();
			::InvalidateRect(hDlg, &m_ImageRect, FALSE);
			break;

		case LVN_KEYDOWN:
			{
				const NMLVKEYDOWN *pnmlvk = reinterpret_cast<const NMLVKEYDOWN*>(lParam);

				if (pnmlvk->wVKey == VK_BACK || pnmlvk->wVKey == VK_DELETE) {
					const HWND hwndList = ::GetDlgItem(hDlg, IDC_CONTROLLER_ASSIGN);
					const int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

					if (Sel >= 0)
						SetButtonCommand(hwndList, Sel, 0);
				}
			}
			break;

		case PSN_APPLY:
			{
				for (size_t i = 0; i < m_ControllerList.size(); i++) {
					ControllerInfo &Info = m_ControllerList[i];
					const ControllerSettings &CurSettings = m_CurSettingsList[i];

					if (Info.Settings != CurSettings) {
						if (Info.Controller->IsEnabled()) {
							if (CurSettings.fActiveOnly != Info.Settings.fActiveOnly) {
								Info.Controller->Enable(false);
								Info.Settings.fActiveOnly = CurSettings.fActiveOnly;
								Info.Controller->Enable(true);
							}
						}
						Info.Settings = CurSettings;
						Info.fSettingsChanged = true;
					}
				}
			}
			break;
		}
		break;

	case WM_DESTROY:
		{
			if (m_hbmController != nullptr) {
				::DeleteObject(m_hbmController);
				m_hbmController = nullptr;
			}
			if (m_hbmSelButtons != nullptr) {
				::DeleteObject(m_hbmSelButtons);
				m_hbmSelButtons = nullptr;
			}
			m_CurSettingsList.clear();
			m_Tooltip.Destroy();
		}
		return TRUE;
	}

	return FALSE;
}


void CControllerManager::RealizeStyle()
{
	CBasicDialog::RealizeStyle();

	if (m_hDlg != nullptr) {
		const HWND hwndList = ::GetDlgItem(m_hDlg, IDC_CONTROLLER_ASSIGN);

		for (int i = 0; i < 2; i++)
			ListView_SetColumnWidth(hwndList, i, LVSCW_AUTOSIZE_USEHEADER);

		m_Tooltip.SetFont(m_Font.GetHandle());
	}
}




bool CControllerManager::ControllerSettings::operator==(const ControllerSettings &Operand) const
{
	if (AssignList.size() != Operand.AssignList.size()
			|| fActiveOnly != Operand.fActiveOnly)
		return false;

	for (size_t i = 0; i < AssignList.size(); i++) {
		if (AssignList[i] != Operand.AssignList[i])
			return false;
	}

	return true;
}


}	// namespace TVTest
