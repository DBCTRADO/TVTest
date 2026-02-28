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
#include "BasicWindow.h"
#include "DarkMode.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CBasicWindow::~CBasicWindow()
{
	Destroy();
}


void CBasicWindow::Destroy()
{
	if (m_hwnd != nullptr) {
		::DestroyWindow(m_hwnd);
		m_hwnd = nullptr;
	}
}


bool CBasicWindow::SetPosition(int Left, int Top, int Width, int Height)
{
	if (Width < 0 || Height < 0)
		return false;
	if (m_hwnd != nullptr) {
		if ((GetWindowStyle() & WS_CHILD) != 0
				|| (!::IsZoomed(m_hwnd) && !::IsIconic(m_hwnd))) {
			::MoveWindow(m_hwnd, Left, Top, Width, Height, TRUE);
		} else {
			WINDOWPLACEMENT wp;

			wp.length = sizeof(WINDOWPLACEMENT);
			::GetWindowPlacement(m_hwnd, &wp);
			wp.rcNormalPosition.left = Left;
			wp.rcNormalPosition.top = Top;
			wp.rcNormalPosition.right = Left + Width;
			wp.rcNormalPosition.bottom = Top + Height;
			if ((GetWindowExStyle() & WS_EX_TOOLWINDOW) == 0) {
				const HMONITOR hMonitor = ::MonitorFromRect(&wp.rcNormalPosition, MONITOR_DEFAULTTONEAREST);
				MONITORINFO mi;

				mi.cbSize = sizeof(MONITORINFO);
				::GetMonitorInfo(hMonitor, &mi);
				::OffsetRect(
					&wp.rcNormalPosition,
					mi.rcMonitor.left - mi.rcWork.left,
					mi.rcMonitor.top - mi.rcWork.top);
			}
			::SetWindowPlacement(m_hwnd, &wp);
		}
	} else {
		m_WindowPosition.Left = Left;
		m_WindowPosition.Top = Top;
		m_WindowPosition.Width = Width;
		m_WindowPosition.Height = Height;
	}
	return true;
}


bool CBasicWindow::SetPosition(const RECT *pPosition)
{
	return SetPosition(
		pPosition->left, pPosition->top,
		pPosition->right - pPosition->left,
		pPosition->bottom - pPosition->top);
}


void CBasicWindow::GetPosition(int *pLeft, int *pTop, int *pWidth, int *pHeight) const
{
	if (m_hwnd != nullptr) {
		RECT rc;

		if ((GetWindowStyle() & WS_CHILD) != 0) {
			::GetWindowRect(m_hwnd, &rc);
			::MapWindowPoints(nullptr, ::GetParent(m_hwnd), reinterpret_cast<POINT*>(&rc), 2);
			if (pLeft)
				*pLeft = rc.left;
			if (pTop)
				*pTop = rc.top;
			if (pWidth)
				*pWidth = rc.right - rc.left;
			if (pHeight)
				*pHeight = rc.bottom - rc.top;
		} else {
			WINDOWPLACEMENT wp;

			wp.length = sizeof(WINDOWPLACEMENT);
			::GetWindowPlacement(m_hwnd, &wp);
			if (wp.showCmd == SW_SHOWNORMAL) {
				// 通常表示時はGetWindowRectの方が座標変換の問題がないので確実
				::GetWindowRect(m_hwnd, &rc);
			} else {
				/*
					WS_EX_TOOLWINDOWスタイルが付いていない場合は、
					rcNormalPositionはワークスペース座標になる(仕様が意味不明...)
				*/
				if ((GetWindowExStyle() & WS_EX_TOOLWINDOW) == 0) {
					const HMONITOR hMonitor = ::MonitorFromRect(&wp.rcNormalPosition, MONITOR_DEFAULTTONEAREST);
					MONITORINFO mi;

					mi.cbSize = sizeof(MONITORINFO);
					::GetMonitorInfo(hMonitor, &mi);
					::OffsetRect(
						&wp.rcNormalPosition,
						mi.rcWork.left - mi.rcMonitor.left,
						mi.rcWork.top - mi.rcMonitor.top);
				}
				rc = wp.rcNormalPosition;
			}
			if (pLeft)
				*pLeft = rc.left;
			if (pTop)
				*pTop = rc.top;
			if (pWidth)
				*pWidth = rc.right - rc.left;
			if (pHeight)
				*pHeight = rc.bottom - rc.top;
		}
	} else {
		if (pLeft)
			*pLeft = m_WindowPosition.Left;
		if (pTop)
			*pTop = m_WindowPosition.Top;
		if (pWidth)
			*pWidth = m_WindowPosition.Width;
		if (pHeight)
			*pHeight = m_WindowPosition.Height;
	}
}


void CBasicWindow::GetPosition(RECT *pPosition) const
{
	int Left, Top, Width, Height;

	GetPosition(&Left, &Top, &Width, &Height);
	::SetRect(pPosition, Left, Top, Left + Width, Top + Height);
}


int CBasicWindow::GetWidth() const
{
	int Width;

	GetPosition(nullptr, nullptr, &Width, nullptr);
	return Width;
}


int CBasicWindow::GetHeight() const
{
	int Height;

	GetPosition(nullptr, nullptr, nullptr, &Height);
	return Height;
}


bool CBasicWindow::GetScreenPosition(RECT *pPosition) const
{
	if (m_hwnd == nullptr) {
		GetPosition(pPosition);
		return true;
	}
	return ::GetWindowRect(m_hwnd, pPosition) != FALSE;
}


void CBasicWindow::SetVisible(bool fVisible)
{
	if (m_hwnd != nullptr)
		::ShowWindow(m_hwnd, fVisible ? SW_SHOW : SW_HIDE);
}


bool CBasicWindow::GetVisible() const
{
	return m_hwnd != nullptr && ::IsWindowVisible(m_hwnd);
}


bool CBasicWindow::SetMaximize(bool fMaximize)
{
	if (m_hwnd != nullptr) {
		::ShowWindow(m_hwnd, fMaximize ? SW_MAXIMIZE : SW_RESTORE);
	} else {
		m_WindowPosition.fMaximized = fMaximize;
	}
	return true;
}


bool CBasicWindow::GetMaximize() const
{
	if (m_hwnd != nullptr)
		return ::IsZoomed(m_hwnd) != FALSE;
	return m_WindowPosition.fMaximized;
}


bool CBasicWindow::Invalidate(bool fErase)
{
	return m_hwnd != nullptr && ::InvalidateRect(m_hwnd, nullptr, fErase);
}


bool CBasicWindow::Invalidate(const RECT *pRect, bool fErase)
{
	return m_hwnd != nullptr && ::InvalidateRect(m_hwnd, pRect, fErase);
}


bool CBasicWindow::Update()
{
	return m_hwnd != nullptr && ::UpdateWindow(m_hwnd);
}


bool CBasicWindow::Redraw(const RECT *pRect, UINT Flags)
{
	return m_hwnd != nullptr && ::RedrawWindow(m_hwnd, pRect, nullptr, Flags);
}


bool CBasicWindow::GetClientRect(RECT *pRect) const
{
	return m_hwnd != nullptr && ::GetClientRect(m_hwnd, pRect);
}


bool CBasicWindow::GetClientSize(SIZE *pSize) const
{
	RECT rc;

	if (m_hwnd == nullptr || !::GetClientRect(m_hwnd, &rc))
		return false;
	pSize->cx = rc.right;
	pSize->cy = rc.bottom;
	return true;
}


bool CBasicWindow::SetParent(HWND hwnd)
{
	return m_hwnd != nullptr && ::SetParent(m_hwnd, hwnd);
}


bool CBasicWindow::SetParent(CBasicWindow *pWindow)
{
	return m_hwnd != nullptr && ::SetParent(m_hwnd, pWindow->m_hwnd);
}


HWND CBasicWindow::GetParent() const
{
	if (m_hwnd == nullptr)
		return nullptr;
	return ::GetParent(m_hwnd);
}


bool CBasicWindow::MoveToMonitorInside()
{
	RECT rc;
	MONITORINFO mi;

	GetPosition(&rc);
	const HMONITOR hMonitor = ::MonitorFromRect(&rc, MONITOR_DEFAULTTONEAREST);
	mi.cbSize = sizeof(MONITORINFO);
	::GetMonitorInfo(hMonitor, &mi);
	if (rc.left >= mi.rcMonitor.right || rc.top >= mi.rcMonitor.bottom
			|| rc.right <= mi.rcMonitor.left || rc.bottom <= mi.rcMonitor.top) {
		int XOffset = 0, YOffset = 0;

		if (rc.left >= mi.rcMonitor.right)
			XOffset = mi.rcMonitor.right - rc.right;
		else if (rc.right <= mi.rcMonitor.left)
			XOffset = mi.rcMonitor.left - rc.left;
		if (rc.top >= mi.rcMonitor.bottom)
			YOffset = mi.rcMonitor.bottom - rc.bottom;
		else if (rc.bottom <= mi.rcMonitor.top)
			YOffset = mi.rcMonitor.top - rc.top;
		::OffsetRect(&rc, XOffset, YOffset);
		SetPosition(&rc);
		return true;
	}
	return false;
}


DWORD CBasicWindow::GetWindowStyle() const
{
	if (m_hwnd == nullptr)
		return 0;
	return ::GetWindowLong(m_hwnd, GWL_STYLE);
}


bool CBasicWindow::SetWindowStyle(DWORD Style, bool fFrameChange)
{
	if (m_hwnd == nullptr)
		return false;
	::SetWindowLong(m_hwnd, GWL_STYLE, Style);
	if (fFrameChange)
		::SetWindowPos(
			m_hwnd, nullptr, 0, 0, 0, 0,
			SWP_FRAMECHANGED | SWP_DRAWFRAME | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	return true;
}


DWORD CBasicWindow::GetWindowExStyle() const
{
	if (m_hwnd == nullptr)
		return 0;
	return ::GetWindowLong(m_hwnd, GWL_EXSTYLE);
}


bool CBasicWindow::SetWindowExStyle(DWORD ExStyle, bool fFrameChange)
{
	if (m_hwnd == nullptr)
		return false;
	::SetWindowLong(m_hwnd, GWL_EXSTYLE, ExStyle);
	if (fFrameChange)
		::SetWindowPos(
			m_hwnd, nullptr, 0, 0, 0, 0,
			SWP_FRAMECHANGED | SWP_DRAWFRAME | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	return true;
}


bool CBasicWindow::CreateBasicWindow(
	HWND hwndParent, DWORD Style, DWORD ExStyle,
	int ID, LPCTSTR pszClassName, LPCTSTR pszText, HINSTANCE hinst)
{
	if (m_hwnd != nullptr)
		return false;
	m_hwnd = ::CreateWindowEx(
		ExStyle, pszClassName, pszText, Style,
		m_WindowPosition.Left, m_WindowPosition.Top,
		m_WindowPosition.Width, m_WindowPosition.Height,
		hwndParent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID)), hinst, this);
	return m_hwnd != nullptr;
}


CBasicWindow *CBasicWindow::OnCreate(HWND hwnd, LPARAM lParam)
{
	LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
	CBasicWindow *pWindow = static_cast<CBasicWindow*>(pcs->lpCreateParams);

	pWindow->m_hwnd = hwnd;
	::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
	return pWindow;
}


void CBasicWindow::OnDestroy()
{
	GetPosition(
		&m_WindowPosition.Left, &m_WindowPosition.Top,
		&m_WindowPosition.Width, &m_WindowPosition.Height);
	m_WindowPosition.fMaximized = ::IsZoomed(m_hwnd) != FALSE;
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(nullptr));
	m_hwnd = nullptr;
}


CBasicWindow *CBasicWindow::GetBasicWindow(HWND hwnd)
{
	return reinterpret_cast<CBasicWindow*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
}


LRESULT CBasicWindow::SendMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (m_hwnd == nullptr)
		return 0;
	return ::SendMessage(m_hwnd, Msg, wParam, lParam);
}


bool CBasicWindow::PostMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (m_hwnd == nullptr)
		return false;
	return ::PostMessage(m_hwnd, Msg, wParam, lParam) != FALSE;
}


bool CBasicWindow::SendSizeMessage()
{
	if (m_hwnd == nullptr)
		return false;

	RECT rc;
	if (!::GetClientRect(m_hwnd, &rc))
		return false;
	::SendMessage(m_hwnd, WM_SIZE, 0, MAKELONG(rc.right, rc.bottom));
	return true;
}


bool CBasicWindow::SetOpacity(int Opacity, bool fClearLayered)
{
	if (Opacity < 0 || Opacity > 255 || m_hwnd == nullptr)
		return false;

	// 子ウィンドウをレイヤードウィンドウにできるのは Windows 8 以降
	if ((GetWindowStyle() & WS_CHILD) != 0 && !Util::OS::IsWindows8OrLater())
		return false;

	const DWORD ExStyle = GetWindowExStyle();

	if (Opacity < 255) {
		if ((ExStyle & WS_EX_LAYERED) == 0)
			SetWindowExStyle(ExStyle | WS_EX_LAYERED);
		if (!::SetLayeredWindowAttributes(m_hwnd, 0, static_cast<BYTE>(Opacity), LWA_ALPHA))
			return false;
	} else {
		if ((ExStyle & WS_EX_LAYERED) != 0) {
			if (fClearLayered) {
				SetWindowExStyle(ExStyle ^ WS_EX_LAYERED);
				Redraw(nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
			} else {
				::SetLayeredWindowAttributes(m_hwnd, 0, 255, LWA_ALPHA);
			}
		}
	}

	return true;
}




CCustomWindow::~CCustomWindow()
{
	Destroy();
}


LRESULT CALLBACK CCustomWindow::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CCustomWindow *pThis;

	if (uMsg == WM_NCCREATE) {
		pThis = static_cast<CCustomWindow*>(OnCreate(hwnd, lParam));
		if (!pThis->HandleMessage(hwnd, uMsg, wParam, lParam)) {
			pThis->m_hwnd = nullptr;
			return FALSE;
		}
		return TRUE;
	} else {
		pThis = static_cast<CCustomWindow*>(GetBasicWindow(hwnd));
		if (pThis == nullptr)
			return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
		if (uMsg == WM_CREATE) {
			if (pThis->HandleMessage(hwnd, uMsg, wParam, lParam) < 0) {
				pThis->m_hwnd = nullptr;
				return -1;
			}
			return 0;
		}
		if (uMsg == WM_DESTROY) {
			pThis->HandleMessage(hwnd, uMsg, wParam, lParam);
			pThis->OnDestroy();
			return 0;
		}
	}

	return pThis->OnMessage(hwnd, uMsg, wParam, lParam);
}


LRESULT CCustomWindow::HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return OnMessage(hwnd, uMsg, wParam, lParam);
}


LRESULT CCustomWindow::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}




LRESULT CPopupWindow::HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		if (SetWindowAllowDarkMode(m_hwnd, true)) {
			m_fAllowDarkMode = true;
			if (TVTest::IsDarkMode()) {
				if (SetWindowFrameDarkMode(m_hwnd, true))
					m_fDarkMode = true;
			}
		}
		break;

	case WM_SETTINGCHANGE:
		if (m_fAllowDarkMode) {
			if (IsDarkModeSettingChanged(hwnd, uMsg, wParam, lParam)) {
				const bool fDarkMode = TVTest::IsDarkMode();

				if (m_fDarkMode != fDarkMode) {
					if (SetWindowFrameDarkMode(hwnd, fDarkMode)) {
						m_fDarkMode = fDarkMode;
						OnDarkModeChanged(fDarkMode);
					}
				}
			}
		}
		break;
	}

	return OnMessage(hwnd, uMsg, wParam, lParam);
}


} // namespace TVTest
