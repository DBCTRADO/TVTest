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
#include "AboutDialog.h"
#include "DialogUtil.h"
#include "TVTestVersion.h"
#include "LibISDB/LibISDB/LibISDBVersion.hpp"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


static constexpr size_t MAX_INFO_TEXT = 256;


CAboutDialog::~CAboutDialog()
{
	Destroy();
}


bool CAboutDialog::Show(HWND hwndOwner)
{
	return ShowDialog(
		hwndOwner, GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE(IDD_ABOUT)) == IDOK;
}


INT_PTR CAboutDialog::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static const struct {
		COLORREF Header1;
		COLORREF Header2;
		COLORREF HeaderText;
		COLORREF HeaderShadow;
		COLORREF Info1;
		COLORREF Info2;
		COLORREF InfoText;
		COLORREF LinkText;
	} Colors = {
		RGB(192, 213, 245), RGB(160, 192, 240),
		RGB(  0,   0,   0),
		RGB(200, 214, 255),
		RGB(224, 232, 255), RGB(240, 244, 255),
		RGB( 64,  64,  64),
		RGB(  0,   0, 255)
	};

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			const HWND hwndHeader = ::GetDlgItem(hDlg, IDC_ABOUT_HEADER);
			const HWND hwndInfo = ::GetDlgItem(hDlg, IDC_ABOUT_INFO);
			const HWND hwndLogo = ::GetDlgItem(hDlg, IDC_ABOUT_LOGO);
			const HWND hwndLink = ::GetDlgItem(hDlg, IDC_ABOUT_LINK);

			const HDC hdc = ::GetDC(hDlg);
			const HFONT hfontOld = DrawUtil::SelectObject(hdc, m_Font);
			TCHAR szText[MAX_INFO_TEXT];

			StringCopy(
				szText,
				ABOUT_VERSION_TEXT
#ifdef VERSION_HASH
				TEXT(" ") VERSION_HASH
#endif
				TEXT(" (")
#ifdef _DEBUG
				TEXT("Debug")
#else
				TEXT("Release")
#endif
#ifdef VERSION_PLATFORM
				TEXT(" ") VERSION_PLATFORM
#endif
				TEXT(")"));
			::SetWindowText(hwndHeader, szText);
			RECT rcHeaderText = {0, 0, 0, 0};
			::DrawText(hdc, szText, -1, &rcHeaderText, DT_CALCRECT | DT_NOPREFIX);

			const size_t Length = StringFormat(
				szText,
				TEXT("Work with LibISDB ver.") LIBISDB_VERSION_STRING
#ifdef LIBISDB_VERSION_HASH
				TEXT(" ") LIBISDB_VERSION_HASH
#endif
				TEXT("\r\n")
#ifdef _MSC_FULL_VER
				TEXT("Compiled with MSVC {}.{}.{}.{}\r\n"),
				_MSC_FULL_VER / 10000000, (_MSC_FULL_VER / 100000) % 100, _MSC_FULL_VER % 100000, _MSC_BUILD
#endif
				);
			::GetWindowText(hwndInfo, szText + Length, static_cast<int>(lengthof(szText) - Length));
			::SetWindowText(hwndInfo, szText);
			RECT rcText = {0, 0, 0, 0};
			::DrawText(hdc, szText, -1, &rcText, DT_CALCRECT | DT_NOPREFIX);
			DrawUtil::SelectObject(hdc, m_LinkFont);

			::GetWindowText(hwndLink, szText, lengthof(szText));
			RECT rcLinkText = {0, 0, 0, 0};
			::DrawText(hdc, szText, -1, &rcLinkText, DT_CALCRECT | DT_NOPREFIX);

			::SelectObject(hdc, hfontOld);
			::ReleaseDC(hDlg, hdc);

			if (rcText.right < rcHeaderText.right)
				rcText.right = rcHeaderText.right;
			if (rcText.right < rcLinkText.right)
				rcText.right = rcLinkText.right;

			RECT rcInfo;
			::GetWindowRect(hwndInfo, &rcInfo);
			::OffsetRect(&rcInfo, -rcInfo.left, -rcInfo.top);
			RECT rcLink;
			GetDlgItemRect(hDlg, IDC_ABOUT_LINK, &rcLink);
			RECT rcLogo;
			::GetWindowRect(hwndLogo, &rcLogo);

			if (rcInfo.bottom < rcText.bottom || rcInfo.right < rcText.right
					|| rcLink.bottom - rcLink.top < rcLinkText.bottom) {
				const int Width = std::max(rcInfo.right, rcText.right);
				const int Height = std::max(rcInfo.bottom, rcText.bottom);
				const int XDiff = Width - rcInfo.right;
				int YDiff = Height - rcInfo.bottom;

				::SetWindowPos(
					hwndInfo, nullptr, 0, 0, Width, Height,
					SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
				::OffsetRect(&rcLink, 0, YDiff);
				rcLink.right += XDiff;
				YDiff += rcLinkText.bottom - (rcLink.bottom - rcLink.top);
				RECT rcHeader;
				::GetWindowRect(hwndHeader, &rcHeader);
				::SetWindowPos(
					hwndHeader, nullptr, 0, 0, Width, rcHeader.bottom - rcHeader.top,
					SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
				RECT rcDialog;
				::GetWindowRect(hDlg, &rcDialog);
				::SetWindowPos(
					hDlg, nullptr, 0, 0,
					(rcDialog.right - rcDialog.left) + XDiff,
					(rcDialog.bottom - rcDialog.top) + YDiff,
					SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
				RECT rcClient;
				::GetClientRect(hDlg, &rcClient);
				::SetWindowPos(
					hwndLogo, nullptr, 0, 0,
					rcLogo.right - rcLogo.left, rcClient.bottom,
					SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
			}

			::SetWindowPos(
				hwndLink, nullptr,
				rcLink.left + ((rcLink.right - rcLink.left) - rcLinkText.right) / 2,
				rcLink.top,
				rcLinkText.right,
				rcLinkText.bottom,
				SWP_NOZORDER | SWP_NOACTIVATE);

			::SetRect(&rcLogo, rcLogo.right - rcLogo.left, 0, 0, 0);
			if (!Util::OS::IsWindows8OrLater()
					&& m_AeroGlass.ApplyAeroGlass(hDlg, &rcLogo)) {
				m_fDrawLogo = true;
				m_LogoImage.LoadFromResource(
					GetAppClass().GetResourceInstance(),
					MAKEINTRESOURCE(IDB_LOGO32), TEXT("PNG"));
				::ShowWindow(hwndLogo, SW_HIDE);
			} else {
				const HBITMAP hbm = static_cast<HBITMAP>(
					::LoadImage(GetAppClass().GetResourceInstance(),
						MAKEINTRESOURCE(IDB_LOGO),
						IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR));
				::SendMessage(
					hwndLogo, STM_SETIMAGE,
					IMAGE_BITMAP, reinterpret_cast<LPARAM>(hbm));
			}

			AdjustDialogPos(GetParent(hDlg), hDlg);
		}
		return TRUE;

	case WM_CTLCOLORSTATIC:
		if (reinterpret_cast<HWND>(lParam) == ::GetDlgItem(hDlg, IDC_ABOUT_LOGO))
			return reinterpret_cast<INT_PTR>(::GetStockObject(WHITE_BRUSH));
		if (reinterpret_cast<HWND>(lParam) == ::GetDlgItem(hDlg, IDC_ABOUT_HEADER)
				|| reinterpret_cast<HWND>(lParam) == ::GetDlgItem(hDlg, IDC_ABOUT_INFO)
				|| reinterpret_cast<HWND>(lParam) == ::GetDlgItem(hDlg, IDC_ABOUT_LINK))
			return reinterpret_cast<INT_PTR>(::GetStockObject(NULL_BRUSH));
		break;

	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT pdis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
			const int OldBkMode = ::SetBkMode(pdis->hDC, TRANSPARENT);
			const COLORREF OldTextColor = ::GetTextColor(pdis->hDC);;
			const HFONT hfontOld = static_cast<HFONT>(::GetCurrentObject(pdis->hDC, OBJ_FONT));
			TCHAR szText[MAX_INFO_TEXT];

			if (pdis->CtlID == IDC_ABOUT_HEADER) {
				DrawUtil::SelectObject(pdis->hDC, m_Font);
				::SetTextColor(pdis->hDC, Colors.HeaderText);
				::GetDlgItemText(hDlg, IDC_ABOUT_HEADER, szText, lengthof(szText));
				::DrawText(
					pdis->hDC, szText, -1, &pdis->rcItem,
					DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
			} else if (pdis->CtlID == IDC_ABOUT_INFO) {
				DrawUtil::SelectObject(pdis->hDC, m_Font);
				::SetTextColor(pdis->hDC, Colors.InfoText);
				::GetDlgItemText(hDlg, IDC_ABOUT_INFO, szText, lengthof(szText));
				::DrawText(pdis->hDC, szText, -1, &pdis->rcItem, DT_CENTER | DT_NOPREFIX);
			} else if (pdis->CtlID == IDC_ABOUT_LINK) {
				DrawUtil::SelectObject(pdis->hDC, m_LinkFont);
				::SetTextColor(pdis->hDC, Colors.LinkText);
				::GetDlgItemText(hDlg, IDC_ABOUT_LINK, szText, lengthof(szText));
				::DrawText(pdis->hDC, szText, -1, &pdis->rcItem, DT_CENTER | DT_NOPREFIX);
			}

			::SelectObject(pdis->hDC, hfontOld);
			::SetTextColor(pdis->hDC, OldTextColor);
			::SetBkMode(pdis->hDC, OldBkMode);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ABOUT_LINK:
			if (HIWORD(wParam) == STN_CLICKED) {
				TCHAR szText[MAX_INFO_TEXT];

				if (::GetDlgItemText(hDlg, IDC_ABOUT_LINK, szText, lengthof(szText)) > 0)
					::ShellExecute(nullptr, TEXT("open"), szText, nullptr, nullptr, SW_SHOW);
			}
			return TRUE;

		case IDOK:
		case IDCANCEL:
			::EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rcClient, rcLogo, rcHeader;

			::BeginPaint(hDlg, &ps);
			GetDlgItemRect(hDlg, IDC_ABOUT_LOGO, &rcLogo);
			GetDlgItemRect(hDlg, IDC_ABOUT_HEADER, &rcHeader);
			::GetClientRect(hDlg, &rcClient);
			rcClient.left = rcLogo.right;

			if (m_fDrawLogo) {
				Graphics::CCanvas Canvas(ps.hdc);

				Canvas.Clear(0, 0, 0, 0);
				Canvas.DrawImage(
					&m_LogoImage,
					(rcLogo.right - m_LogoImage.GetWidth()) / 2,
					(rcLogo.bottom - m_LogoImage.GetHeight()) / 2);
				RECT rc = rcClient;
				rc.bottom = rcHeader.bottom;
				Canvas.FillGradient(
					Colors.Header1, Colors.Header2, rc,
					Graphics::GradientDirection::Vert);
				rc.top = rc.bottom;
				rc.bottom = rc.top + 8;
				Canvas.FillGradient(
					Colors.HeaderShadow, Colors.Info1, rc,
					Graphics::GradientDirection::Vert);
				rc.top = rc.bottom;
				rc.bottom = rcClient.bottom;
				Canvas.FillGradient(
					Colors.Info1, Colors.Info2, rc,
					Graphics::GradientDirection::Vert);
			} else {
				RECT rc = rcClient;
				rc.bottom = rcHeader.bottom;
				DrawUtil::FillGradient(
					ps.hdc, &rc, Colors.Header1, Colors.Header2,
					DrawUtil::FillDirection::Vert);
				rc.top = rc.bottom;
				rc.bottom = rc.top + 8;
				DrawUtil::FillGradient(
					ps.hdc, &rc, Colors.HeaderShadow, Colors.Info1,
					DrawUtil::FillDirection::Vert);
				rc.top = rc.bottom;
				rc.bottom = rcClient.bottom;
				DrawUtil::FillGradient(
					ps.hdc, &rc, Colors.Info1, Colors.Info2,
					DrawUtil::FillDirection::Vert);
			}

			::EndPaint(hDlg, &ps);
		}
		return TRUE;

	case WM_SETCURSOR:
		if (reinterpret_cast<HWND>(wParam) == ::GetDlgItem(hDlg, IDC_ABOUT_LINK)) {
			::SetCursor(GetAppClass().UICore.GetLinkCursor());
			::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			const HBITMAP hbm = reinterpret_cast<HBITMAP>(
				::SendDlgItemMessage(
					hDlg, IDC_ABOUT_LOGO,
					STM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(nullptr)));

			if (hbm != nullptr) {
				::DeleteObject(hbm);
			} else {
				m_LogoImage.Free();
			}

			m_Font.Destroy();
			m_LinkFont.Destroy();
		}
		return TRUE;
	}

	return FALSE;
}


void CAboutDialog::ApplyStyle()
{
	CBasicDialog::ApplyStyle();

	if (m_hDlg != nullptr) {
		Style::Font Font;
		GetSystemFont(DrawUtil::FontType::Message, &Font);
		CreateDrawFont(Font, &m_Font);
		LOGFONT lf;
		m_Font.GetLogFont(&lf);
		lf.lfUnderline = 1;
		m_LinkFont.Create(&lf);
	}
}


}	// namespace TVTest
