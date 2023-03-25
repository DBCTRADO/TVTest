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
#include "MessageDialog.h"
#include "DialogUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


void CMessageDialog::LogFontToCharFormat(const LOGFONT *plf, CHARFORMAT *pcf)
{
	const HDC hdc = ::GetDC(m_hDlg);
	CRichEditUtil::LogFontToCharFormat(hdc, plf, pcf);
	::ReleaseDC(m_hDlg, hdc);
}


INT_PTR CMessageDialog::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			if (!m_Caption.empty())
				::SetWindowText(hDlg, m_Caption.c_str());

			::SendDlgItemMessage(
				hDlg, IDC_ERROR_ICON, STM_SETICON,
				reinterpret_cast<WPARAM>(
					::LoadIcon(
						nullptr,
						m_MessageType == MessageType::Info ? IDI_INFORMATION :
						m_MessageType == MessageType::Warning ? IDI_WARNING : IDI_ERROR)),
				0);
			::SendDlgItemMessage(hDlg, IDC_ERROR_MESSAGE, EM_SETBKGNDCOLOR, 0, GetThemeColor(COLOR_WINDOW));

			const HWND hwndEdit = ::GetDlgItem(hDlg, IDC_ERROR_MESSAGE);
			CHARFORMAT cf, cfBold;

			CRichEditUtil::DisableAutoFont(hwndEdit);

			NONCLIENTMETRICS ncm;
#if WINVER<0x0600
			ncm.cbSize = sizeof(ncm);
#else
			ncm.cbSize = offsetof(NONCLIENTMETRICS, iPaddedBorderWidth);
#endif
			::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
			LogFontToCharFormat(&ncm.lfMessageFont, &cf);
			cf.crTextColor = GetThemeColor(COLOR_WINDOWTEXT);

			cfBold = cf;
			cfBold.dwMask |= CFM_BOLD;
			cfBold.dwEffects |= CFE_BOLD;
			if (!m_Title.empty()) {
				CRichEditUtil::AppendText(hwndEdit, m_Title.c_str(), &cfBold);
				CRichEditUtil::AppendText(hwndEdit, TEXT("\n"), &cf);
			}
			if (!m_Text.empty()) {
				CRichEditUtil::AppendText(hwndEdit, m_Text.c_str(), &cf);
			}
			if (!m_SystemMessage.empty()) {
				CRichEditUtil::AppendText(hwndEdit, TEXT("\n\nWindowsのエラーメッセージ :\n"), &cfBold);
				CRichEditUtil::AppendText(hwndEdit, m_SystemMessage.c_str(), &cf);
			}
			const int MaxWidth = CRichEditUtil::GetMaxLineWidth(hwndEdit) + 8;
			RECT rcEdit, rcIcon, rcDlg, rcClient, rcOK;
			::GetWindowRect(hwndEdit, &rcEdit);
			::OffsetRect(&rcEdit, -rcEdit.left, -rcEdit.top);
			::GetWindowRect(::GetDlgItem(hDlg, IDC_ERROR_ICON), &rcIcon);
			rcIcon.bottom -= rcIcon.top;
			if (rcEdit.bottom < rcIcon.bottom)
				rcEdit.bottom = rcIcon.bottom;
			::SetWindowPos(
				hwndEdit, nullptr, 0, 0, MaxWidth, rcEdit.bottom, SWP_NOMOVE | SWP_NOZORDER);
			::GetWindowRect(hDlg, &rcDlg);
			::GetClientRect(hDlg, &rcClient);
			GetDlgItemRect(hDlg, IDOK, &rcOK);
			const int Offset = MaxWidth - rcEdit.right;
			::SetWindowPos(
				::GetDlgItem(hDlg, IDOK), nullptr,
				rcOK.left + Offset, rcOK.top, 0, 0,
				SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
			::SetWindowPos(
				hDlg, nullptr, 0, 0, (rcDlg.right - rcDlg.left) + Offset, rcDlg.bottom - rcDlg.top,
				SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
			::SendMessage(hwndEdit, EM_SETEVENTMASK, 0, ENM_REQUESTRESIZE | ENM_MOUSEEVENTS);
			::SendDlgItemMessage(hDlg, IDC_ERROR_MESSAGE, EM_REQUESTRESIZE, 0, 0);

			AdjustDialogPos(::GetParent(hDlg), hDlg);
		}
		return TRUE;

	/*
	case WM_SIZE:
		::SendDlgItemMessage(hDlg, IDC_ERROR_MESSAGE, EM_REQUESTRESIZE, 0, 0);
		return TRUE;
	*/

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rcEdit, rc;

			::BeginPaint(hDlg, &ps);
			::GetWindowRect(::GetDlgItem(hDlg, IDC_ERROR_MESSAGE), &rcEdit);
			MapWindowRect(nullptr, hDlg, &rcEdit);
			::GetClientRect(hDlg, &rc);
			rc.bottom = rcEdit.bottom;
			DrawUtil::Fill(ps.hdc, &rc, GetThemeColor(COLOR_WINDOW));
			::EndPaint(hDlg, &ps);
		}
		return TRUE;

	case WM_CTLCOLORSTATIC:
		if (reinterpret_cast<HWND>(lParam) == ::GetDlgItem(hDlg, IDC_ERROR_ICON))
			return reinterpret_cast<INT_PTR>(m_BackBrush.GetHandle());
		break;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case EN_REQUESTRESIZE:
			{
				REQRESIZE *prr = reinterpret_cast<REQRESIZE*>(lParam);
				RECT rcEdit, rcDialog, rcClient, rcOK, rcIcon;

				::GetWindowRect(hDlg, &rcDialog);
				::GetClientRect(hDlg, &rcClient);
				::GetWindowRect(prr->nmhdr.hwndFrom, &rcEdit);
				GetDlgItemRect(hDlg, IDOK, &rcOK);
				const int MinWidth = (rcOK.right - rcOK.left) + (rcClient.right - rcOK.right) * 2;
				int Width = prr->rc.right - prr->rc.left;
				if (Width < MinWidth)
					Width = MinWidth;
				int Height = prr->rc.bottom - prr->rc.top;
				::GetWindowRect(::GetDlgItem(hDlg, IDC_ERROR_ICON), &rcIcon);
				rcIcon.bottom -= rcIcon.top;
				if (Height < rcIcon.bottom)
					Height = rcIcon.bottom;
				if (Width == rcEdit.right - rcEdit.left
						&& Height == rcEdit.bottom - rcEdit.top)
					break;
				const int XOffset = Width - (rcEdit.right - rcEdit.left);
				const int YOffset = Height - (rcEdit.bottom - rcEdit.top);
				::SetWindowPos(
					prr->nmhdr.hwndFrom, nullptr, 0, 0, Width, Height,
					SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
				::SetRect(&rcEdit, 0, 0, Width, Height);
				::SendDlgItemMessage(hDlg, IDC_ERROR_MESSAGE, EM_SETRECT, 0, reinterpret_cast<LPARAM>(&rcEdit));
				rcDialog.right += XOffset;
				rcDialog.bottom += YOffset;
				::MoveWindow(
					hDlg, rcDialog.left, rcDialog.top,
					rcDialog.right - rcDialog.left,
					rcDialog.bottom - rcDialog.top, TRUE);
				::MoveWindow(
					::GetDlgItem(hDlg, IDOK), rcOK.left + XOffset, rcOK.top + YOffset,
					rcOK.right - rcOK.left, rcOK.bottom - rcOK.top, TRUE);
			}
			return TRUE;

		case EN_MSGFILTER:
			if (reinterpret_cast<MSGFILTER*>(lParam)->msg == WM_RBUTTONUP) {
				const HMENU hmenu = ::CreatePopupMenu();
				POINT pt;

				::AppendMenu(hmenu, MF_STRING | MF_ENABLED, IDC_ERROR_COPY, TEXT("コピー(&C)"));
				::GetCursorPos(&pt);
				::TrackPopupMenu(hmenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hDlg, nullptr);
				::DestroyMenu(hmenu);
			}
			return TRUE;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ERROR_COPY:
			{
				const HWND hwndEdit = ::GetDlgItem(hDlg, IDC_ERROR_MESSAGE);

				if (::SendMessage(hwndEdit, EM_SELECTIONTYPE, 0, 0) == SEL_EMPTY) {
					CRichEditUtil::CopyAllText(hwndEdit);
				} else {
					::SendMessage(hwndEdit, WM_COPY, 0, 0);
				}
			}
			return TRUE;

		case IDOK:
		case IDCANCEL:
			::EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			m_Text.clear();
			m_Title.clear();
			m_SystemMessage.clear();
			m_Caption.clear();
		}
		return TRUE;
	}

	return FALSE;
}


bool CMessageDialog::Show(HWND hwndOwner, MessageType Type, LPCTSTR pszText, LPCTSTR pszTitle, LPCTSTR pszSystemMessage, LPCTSTR pszCaption)
{
	if (pszText == nullptr && pszTitle == nullptr && pszSystemMessage == nullptr)
		return false;

	if (!m_RichEditUtil.LoadRichEditLib()) {
		TCHAR szMessage[1024];
		CStaticStringFormatter Formatter(szMessage, lengthof(szMessage));

		if (pszTitle != nullptr)
			Formatter.Append(pszTitle);
		if (pszText != nullptr) {
			if (!Formatter.IsEmpty())
				Formatter.Append(TEXT("\n"));
			Formatter.Append(pszText);
		}
		if (pszSystemMessage != nullptr) {
			if (!Formatter.IsEmpty())
				Formatter.Append(TEXT("\n\n"));
			Formatter.Append(TEXT("Windowsのエラーメッセージ:\n"));
			Formatter.Append(pszSystemMessage);
		}
		return ::MessageBox(
			hwndOwner, Formatter.GetString(), pszCaption,
			MB_OK |
				(Type == MessageType::Info ? MB_ICONINFORMATION :
				 Type == MessageType::Warning ? MB_ICONEXCLAMATION :
				 Type == MessageType::Error ? MB_ICONSTOP : 0)) == IDOK;
	}

	StringUtility::Assign(m_Text, pszText);
	StringUtility::Assign(m_Title, pszTitle);
	StringUtility::Assign(m_SystemMessage, pszSystemMessage);
	StringUtility::Assign(m_Caption, pszCaption);
	m_MessageType = Type;

	return Show(hwndOwner);
}


bool CMessageDialog::Show(HWND hwndOwner)
{
	return ShowDialog(hwndOwner, GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_ERROR)) == IDOK;
}


}	// namespace TVTest
