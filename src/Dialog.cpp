/*
  TVTest
  Copyright(c) 2008-2019 DBCTRADO

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
#include "Dialog.h"
#include "DialogUtil.h"
#include "DPIUtil.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CBasicDialog::CBasicDialog()
	: m_hDlg(nullptr)
	, m_fModeless(false)
	, m_fSetPosition(false)
	, m_OriginalDPI(0)
	, m_CurrentDPI(0)
	, m_hOriginalFont(nullptr)
	, m_fInitializing(false)
	, m_fOwnDPIScaling(false)
{
	SetStyleScaling(&m_StyleScaling);
}


CBasicDialog::~CBasicDialog()
{
	Destroy();
}


bool CBasicDialog::IsCreated() const
{
	return m_hDlg != nullptr;
}


bool CBasicDialog::Destroy()
{
	if (m_hDlg != nullptr) {
		return ::DestroyWindow(m_hDlg) != FALSE;
	}
	return true;
}


bool CBasicDialog::ProcessMessage(LPMSG pMsg)
{
	if (m_hDlg == nullptr)
		return false;
	return ::IsDialogMessage(m_hDlg, pMsg) != FALSE;
}


bool CBasicDialog::IsVisible() const
{
	return m_hDlg != nullptr && ::IsWindowVisible(m_hDlg);
}


bool CBasicDialog::SetVisible(bool fVisible)
{
	if (m_hDlg == nullptr)
		return false;
	return ::ShowWindow(m_hDlg, fVisible ? SW_SHOW : SW_HIDE) != FALSE;
}


bool CBasicDialog::GetPosition(Position *pPosition) const
{
	if (pPosition == nullptr)
		return false;
	if (m_hDlg == nullptr) {
		*pPosition = m_Position;
	} else {
		RECT rc;
		::GetWindowRect(m_hDlg, &rc);
		pPosition->Set(&rc);
	}
	return true;
}


bool CBasicDialog::GetPosition(RECT *pPosition) const
{
	if (pPosition == nullptr)
		return false;
	if (m_hDlg == nullptr)
		m_Position.Get(pPosition);
	else
		::GetWindowRect(m_hDlg, pPosition);
	return true;
}


bool CBasicDialog::GetPosition(int *pLeft, int *pTop, int *pWidth, int *pHeight) const
{
	RECT rc;

	GetPosition(&rc);
	if (pLeft != nullptr)
		*pLeft = rc.left;
	if (pTop != nullptr)
		*pTop = rc.top;
	if (pWidth != nullptr)
		*pWidth = rc.right - rc.left;
	if (pHeight != nullptr)
		*pHeight = rc.bottom - rc.top;
	return true;
}


bool CBasicDialog::SetPosition(const Position &Pos)
{
	return SetPosition(Pos.x, Pos.y, Pos.Width, Pos.Height);
}


bool CBasicDialog::SetPosition(const RECT *pPosition)
{
	if (pPosition == nullptr)
		return false;
	return SetPosition(
		pPosition->left, pPosition->top,
		pPosition->right - pPosition->left,
		pPosition->bottom - pPosition->top);
}


bool CBasicDialog::SetPosition(int Left, int Top, int Width, int Height)
{
	if (Width < 0 || Height < 0)
		return false;
	if (m_hDlg == nullptr) {
		m_Position.x = Left;
		m_Position.y = Top;
		m_Position.Width = Width;
		m_Position.Height = Height;
		m_fSetPosition = true;
	} else {
		::MoveWindow(m_hDlg, Left, Top, Width, Height, TRUE);
	}
	return true;
}


bool CBasicDialog::SetPosition(int Left, int Top)
{
	if (m_hDlg == nullptr) {
		m_Position.x = Left;
		m_Position.y = Top;
		m_fSetPosition = true;
	} else {
		::SetWindowPos(m_hDlg, nullptr, Left, Top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
	return true;
}


LRESULT CBasicDialog::SendMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_hDlg == nullptr)
		return 0;
	return ::SendMessage(m_hDlg, uMsg, wParam, lParam);
}


int CBasicDialog::ShowDialog(HWND hwndOwner, HINSTANCE hinst, LPCTSTR pszTemplate)
{
	if (m_hDlg != nullptr)
		return -1;

	return (int)::DialogBoxParam(
		hinst, pszTemplate, hwndOwner, DialogProc,
		reinterpret_cast<LPARAM>(this));
}


bool CBasicDialog::CreateDialogWindow(HWND hwndOwner, HINSTANCE hinst, LPCTSTR pszTemplate)
{
	if (m_hDlg != nullptr)
		return false;

	if (::CreateDialogParam(
				hinst, pszTemplate, hwndOwner, DialogProc,
				reinterpret_cast<LPARAM>(this)) == nullptr)
		return false;
	m_fModeless = true;
	return true;
}


CBasicDialog *CBasicDialog::GetThis(HWND hDlg)
{
	return static_cast<CBasicDialog*>(::GetProp(hDlg, TEXT("This")));
}


INT_PTR CALLBACK CBasicDialog::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CBasicDialog *pThis;

	if (uMsg == WM_INITDIALOG) {
		pThis = reinterpret_cast<CBasicDialog*>(lParam);
		pThis->m_hDlg = hDlg;
		pThis->m_fOwnDPIScaling = IsWindowPerMonitorDPIV1(hDlg);
		::SetProp(hDlg, TEXT("This"), pThis);
	} else {
		pThis = GetThis(hDlg);
		if (uMsg == WM_NCDESTROY) {
			if (pThis != nullptr) {
				pThis->HandleMessage(hDlg, uMsg, wParam, lParam);
				pThis->m_hDlg = nullptr;
			}
			::RemoveProp(hDlg, TEXT("This"));
			pThis->OnDestroyed();
			// ここで既に pThis が delete されている可能性がある
			return TRUE;
		}
	}
	if (pThis != nullptr)
		return pThis->HandleMessage(hDlg, uMsg, wParam, lParam);
	return FALSE;
}


INT_PTR CBasicDialog::HandleMessage(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		m_fInitializing = true;
		if (m_fSetPosition) {
			m_Position.Width = 0;
			m_Position.Height = 0;
			ApplyPosition();
		}
		InitDialog();
		break;

	case WM_DPICHANGED:
		if (!m_fInitializing)
			OnDPIChanged(hDlg, wParam, lParam);
		break;

	case WM_DESTROY:
		StorePosition();

		if (m_fOwnDPIScaling) {
			if (!m_ItemList.empty()) {
				for (const auto &e : m_ItemList)
					SetWindowFont(e.hwnd, m_hOriginalFont, FALSE);
				m_ItemList.clear();
			}
			if (m_hOriginalFont != nullptr)
				SetWindowFont(hDlg, m_hOriginalFont, FALSE);
		}
		break;
	}

	return DlgProc(hDlg, uMsg, wParam, lParam);
}


INT_PTR CBasicDialog::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		return TRUE;
	}

	return FALSE;
}


bool CBasicDialog::ApplyPosition()
{
	if (m_hDlg == nullptr || !m_fSetPosition)
		return false;

	RECT rc;
	m_Position.Get(&rc);
	HMONITOR hMonitor = ::MonitorFromRect(&rc, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	if (::GetMonitorInfo(hMonitor, &mi)) {
		if (rc.left < mi.rcMonitor.left)
			m_Position.x = mi.rcMonitor.left;
		else if (rc.right > mi.rcMonitor.right)
			m_Position.x = mi.rcMonitor.right - m_Position.Width;
		if (rc.top < mi.rcMonitor.top)
			m_Position.y = mi.rcMonitor.top;
		else if (rc.bottom > mi.rcMonitor.bottom)
			m_Position.y = mi.rcMonitor.bottom - m_Position.Height;
	}

	UINT Flags = SWP_NOZORDER | SWP_NOACTIVATE;
	if (m_Position.Width <= 0 || m_Position.Height <= 0)
		Flags |= SWP_NOSIZE;

	return ::SetWindowPos(
		m_hDlg, nullptr,
		m_Position.x, m_Position.y,
		m_Position.Width, m_Position.Height,
		Flags) != FALSE;
}


void CBasicDialog::StorePosition()
{
	if (m_hDlg != nullptr) {
		RECT rc;

		if (::GetWindowRect(m_hDlg, &rc)) {
			m_Position.Set(&rc);
			m_fSetPosition = true;
		}
	}
}


void CBasicDialog::InitDialog()
{
	m_fInitializing = true;

	if (m_pStyleScaling == &m_StyleScaling) {
		InitStyleScaling(m_hDlg, false);
	}

	if (IsWindowPerMonitorDPIV2(m_hDlg))
		m_OriginalDPI = GetWindowDPI(m_hDlg);
	else
		m_OriginalDPI = m_pStyleScaling->GetSystemDPI();
	m_CurrentDPI = m_OriginalDPI;
	m_hOriginalFont = GetWindowFont(m_hDlg);
	::GetObject(m_hOriginalFont, sizeof(LOGFONT), &m_lfOriginalFont);

	InitializeUI();

	if (m_fOwnDPIScaling && m_pStyleScaling->GetDPI() != m_OriginalDPI) {
		RealizeStyle();

		RECT rc;
		::GetClientRect(m_hDlg, &rc);
		rc.right = ::MulDiv(rc.right, m_CurrentDPI, m_OriginalDPI);
		rc.bottom = ::MulDiv(rc.bottom, m_CurrentDPI, m_OriginalDPI);
		::AdjustWindowRectEx(&rc, GetWindowStyle(m_hDlg), FALSE, GetWindowExStyle(m_hDlg));
		::SetWindowPos(
			m_hDlg, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
			SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
	}

	m_fInitializing = false;
}


void CBasicDialog::ApplyStyle()
{
	if (m_hDlg != nullptr) {
		const int DPI = m_pStyleScaling->GetDPI();

		LOGFONT lf = m_lfOriginalFont;
		LONG Height = ::MulDiv(abs(lf.lfHeight), DPI, m_OriginalDPI);
		lf.lfHeight = lf.lfHeight < 0 ? -Height : Height;
		lf.lfWidth = 0;
		m_Font.Create(&lf);
	}
}


void CBasicDialog::RealizeStyle()
{
	if (m_hDlg != nullptr && m_fOwnDPIScaling) {
		const int DPI = m_pStyleScaling->GetDPI();

		if (m_CurrentDPI != DPI) {
			m_CurrentDPI = DPI;

			if (m_ItemList.empty()) {
				HWND hwnd = nullptr;

				while ((hwnd = ::FindWindowEx(m_hDlg, hwnd, nullptr, nullptr)) != nullptr) {
					ItemInfo Item;

					Item.hwnd = hwnd;
					::GetWindowRect(hwnd, &Item.rcOriginal);
					MapWindowRect(nullptr, m_hDlg, &Item.rcOriginal);
					m_ItemList.push_back(Item);
				}
			}

			HDWP hdwp = ::BeginDeferWindowPos(static_cast<int>(m_ItemList.size()));

			for (const auto &e : m_ItemList) {
				HWND hwnd = e.hwnd;
				RECT rc = e.rcOriginal;

				rc.left = ::MulDiv(rc.left, DPI, m_OriginalDPI);
				rc.top = ::MulDiv(rc.top, DPI, m_OriginalDPI);
				rc.right = ::MulDiv(rc.right, DPI, m_OriginalDPI);
				rc.bottom = ::MulDiv(rc.bottom, DPI, m_OriginalDPI);
				if (hdwp != nullptr) {
					::DeferWindowPos(
						hdwp, hwnd, nullptr,
						rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
						SWP_NOZORDER | SWP_NOACTIVATE);
				} else {
					::SetWindowPos(
						hwnd, nullptr,
						rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
						SWP_NOZORDER | SWP_NOACTIVATE);
				}

				TCHAR szClass[32];
				::GetClassName(hwnd, szClass, lengthof(szClass));
				if (::lstrcmpi(szClass, TEXT("BUTTON")) == 0
						|| ::lstrcmpi(szClass, WC_LISTVIEW) == 0
						|| ::lstrcmpi(szClass, WC_TREEVIEW) == 0)
					::SendMessage(hwnd, CCM_DPISCALE, TRUE, 0);

				SetWindowFont(hwnd, m_Font.GetHandle(), FALSE);
				::InvalidateRect(hwnd, nullptr, TRUE);
			}

			if (hdwp != nullptr)
				::EndDeferWindowPos(hdwp);

			SetWindowFont(m_hDlg, m_Font.GetHandle(), FALSE);
			::InvalidateRect(m_hDlg, nullptr, TRUE);
		}
	}
}




CResizableDialog::CResizableDialog()
	: m_hwndSizeGrip(nullptr)
{
	m_MinSize.cx = 0;
	m_MinSize.cy = 0;
}


CResizableDialog::~CResizableDialog()
{
}


INT_PTR CResizableDialog::HandleMessage(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			RECT rc;

			if (IsWindowPerMonitorDPIV2(hDlg))
				m_BaseDPI = GetWindowDPI(hDlg);
			else
				m_BaseDPI = m_pStyleScaling->GetSystemDPI();

			::GetClientRect(hDlg, &rc);
			m_OriginalClientSize.cx = rc.right;
			m_OriginalClientSize.cy = rc.bottom;

			InitDialog();

			if ((::GetWindowLong(hDlg, GWL_STYLE)&WS_CHILD) == 0) {
				::GetClientRect(hDlg, &rc);
				m_hwndSizeGrip = ::CreateWindowEx(
					0, TEXT("SCROLLBAR"), nullptr,
					WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SBS_SIZEGRIP |
					SBS_SIZEBOXBOTTOMRIGHTALIGN,
					0, 0, rc.right, rc.bottom, m_hDlg, (HMENU)0,
					reinterpret_cast<HINSTANCE>(::GetWindowLongPtr(m_hDlg, GWLP_HINSTANCE)), nullptr);
				::SetWindowPos(m_hwndSizeGrip, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			} else {
				m_hwndSizeGrip = nullptr;
			}
		}
		break;

	case WM_GETMINMAXINFO:
		{
			MINMAXINFO *pmmi = reinterpret_cast<MINMAXINFO*>(lParam);

			pmmi->ptMinTrackSize.x = m_MinSize.cx;
			pmmi->ptMinTrackSize.y = m_MinSize.cy;
		}
		return TRUE;

	case WM_SIZE:
		DoLayout();
		break;

	case WM_DPICHANGED:
		if (!m_fInitializing)
			OnDPIChanged(hDlg, wParam, lParam);
		break;

	case WM_DESTROY:
		return CBasicDialog::HandleMessage(hDlg, uMsg, wParam, lParam);
	}

	return DlgProc(hDlg, uMsg, wParam, lParam);
}


bool CResizableDialog::ApplyPosition()
{
	if (m_Position.Width < m_MinSize.cx)
		m_Position.Width = m_MinSize.cx;
	if (m_Position.Height < m_MinSize.cy)
		m_Position.Height = m_MinSize.cy;

	return CBasicDialog::ApplyPosition();
}


void CResizableDialog::DoLayout()
{
	RECT rc;
	int Width, Height;

	::GetClientRect(m_hDlg, &rc);
	Width = rc.right;
	Height = rc.bottom;

	HDWP hdwp = ::BeginDeferWindowPos(static_cast<int>(m_ControlList.size()));

	for (const auto &e : m_ControlList) {
		rc = e.rcOriginal;

		const int DPI = e.DPI;
		if (DPI != m_CurrentDPI) {
			rc.left = ::MulDiv(rc.left, m_CurrentDPI, DPI);
			rc.top = ::MulDiv(rc.top, m_CurrentDPI, DPI);
			rc.right = ::MulDiv(rc.right, m_CurrentDPI, DPI);
			rc.bottom = ::MulDiv(rc.bottom, m_CurrentDPI, DPI);
		}

		if (!!(e.Align & AlignFlag::Right)) {
			rc.right += Width - m_ScaledClientSize.cx;
			if (!(e.Align & AlignFlag::Left))
				rc.left += Width - m_ScaledClientSize.cx;
			if (rc.right < rc.left)
				rc.right = rc.left;
		}
		if (!!(e.Align & AlignFlag::Bottom)) {
			rc.bottom += Height - m_ScaledClientSize.cy;
			if (!(e.Align & AlignFlag::Top))
				rc.top += Height - m_ScaledClientSize.cy;
			if (rc.bottom < rc.top)
				rc.bottom = rc.top;
		}

		if (hdwp != nullptr) {
			::DeferWindowPos(
				hdwp, ::GetDlgItem(m_hDlg, e.ID), nullptr,
				rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
				SWP_NOZORDER | SWP_NOACTIVATE);
		} else {
			::SetWindowPos(
				::GetDlgItem(m_hDlg, e.ID), nullptr,
				rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
				SWP_NOZORDER | SWP_NOACTIVATE);
		}
	}

	if (hdwp != nullptr)
		::EndDeferWindowPos(hdwp);

	if (m_hwndSizeGrip != nullptr) {
		::GetWindowRect(m_hwndSizeGrip, &rc);
		::OffsetRect(&rc, -rc.left, -rc.top);
		::MoveWindow(
			m_hwndSizeGrip, Width - rc.right, Height - rc.bottom,
			rc.right, rc.bottom, TRUE);
	}
}


bool CResizableDialog::AddControl(int ID, AlignFlag Align)
{
	HWND hwnd = ::GetDlgItem(m_hDlg, ID);
	if (hwnd == nullptr)
		return false;

	LayoutItem Item;

	Item.ID = ID;
	::GetWindowRect(hwnd, &Item.rcOriginal);
	::MapWindowPoints(nullptr, m_hDlg, reinterpret_cast<LPPOINT>(&Item.rcOriginal), 2);
	Item.DPI = m_CurrentDPI;
	Item.Align = Align;
	m_ControlList.push_back(Item);
	return true;
}


bool CResizableDialog::AddControls(int FirstID, int LastID, AlignFlag Align)
{
	if (FirstID > LastID)
		return false;
	for (int i = FirstID; i <= LastID; i++) {
		if (!AddControl(i, Align))
			return false;
	}
	return true;
}


bool CResizableDialog::UpdateControlPosition(int ID)
{
	for (auto &e : m_ControlList) {
		if (e.ID == ID) {
			GetDlgItemRect(m_hDlg, ID, &e.rcOriginal);
			e.DPI = m_CurrentDPI;
			return true;
		}
	}

	return false;
}


void CResizableDialog::ApplyStyle()
{
	CBasicDialog::ApplyStyle();

	if (m_hDlg != nullptr) {
		const int DPI = m_pStyleScaling->GetDPI();
		RECT rc;

		m_ScaledClientSize.cx = ::MulDiv(m_OriginalClientSize.cx, DPI, m_BaseDPI);
		m_ScaledClientSize.cy = ::MulDiv(m_OriginalClientSize.cy, DPI, m_BaseDPI);

		rc.left = 0;
		rc.top = 0;
		rc.right = m_ScaledClientSize.cx;
		rc.bottom = m_ScaledClientSize.cy;
		::AdjustWindowRectEx(&rc, GetWindowStyle(m_hDlg), FALSE, GetWindowExStyle(m_hDlg));
		m_MinSize.cx = rc.right - rc.left;
		m_MinSize.cy = rc.bottom - rc.top;
	}
}


void CResizableDialog::RealizeStyle()
{
	CBasicDialog::RealizeStyle();
}


}	// namespace TVTest
