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
#include "WindowUtil.h"
#include "Common/DebugDef.h"


namespace TVTest
{


/*
	ウィンドウの端が実際に表示されているか判定する
	これだと不完全(常に最前面のウィンドウを考慮していない)
*/
static bool IsWindowEdgeVisible(HWND hwnd, HWND hwndTop, const RECT *pRect, HWND hwndTarget)
{
	if (hwndTop == hwnd || hwndTop == nullptr)
		return true;

	RECT rc;
	GetWindowRect(hwndTop, &rc);
	const HWND hwndNext = GetNextWindow(hwndTop, GW_HWNDNEXT);
	if (hwndTop == hwndTarget || !IsWindowVisible(hwndTop)
			|| rc.left == rc.right || rc.top == rc.bottom)
		return IsWindowEdgeVisible(hwnd, hwndNext, pRect, hwndTarget);

	RECT rcEdge = *pRect;

	if (pRect->top == pRect->bottom) {
		if (rc.top <= pRect->top && rc.bottom > pRect->top) {
			if (rc.left <= pRect->left && rc.right >= pRect->right)
				return false;
			if (rc.left <= pRect->left && rc.right > pRect->left) {
				rcEdge.right = std::min(rc.right, pRect->right);
				return IsWindowEdgeVisible(hwnd, hwndNext, &rcEdge, hwndTarget);
			} else if (rc.left > pRect->left && rc.right >= pRect->right) {
				rcEdge.left = rc.left;
				return IsWindowEdgeVisible(hwnd, hwndNext, &rcEdge, hwndTarget);
			} else if (rc.left > pRect->left && rc.right < pRect->right) {
				rcEdge.right = rc.left;
				if (IsWindowEdgeVisible(hwnd, hwndNext, &rcEdge, hwndTarget))
					return true;
				rcEdge.left = rc.right;
				rcEdge.right = pRect->right;
				return IsWindowEdgeVisible(hwnd, hwndNext, &rcEdge, hwndTarget);
			}
		}
	} else {
		if (rc.left <= pRect->left && rc.right > pRect->left) {
			if (rc.top <= pRect->top && rc.bottom >= pRect->bottom)
				return false;
			if (rc.top <= pRect->top && rc.bottom > pRect->top) {
				rcEdge.bottom = std::min(rc.bottom, pRect->bottom);
				return IsWindowEdgeVisible(hwnd, hwndNext, &rcEdge, hwndTarget);
			} else if (rc.top > pRect->top && rc.bottom >= pRect->bottom) {
				rcEdge.top = rc.top;
				return IsWindowEdgeVisible(hwnd, hwndNext, &rcEdge, hwndTarget);
			} else if (rc.top > pRect->top && rc.bottom < pRect->bottom) {
				rcEdge.bottom = rc.top;
				if (IsWindowEdgeVisible(hwnd, hwndNext, &rcEdge, hwndTarget))
					return true;
				rcEdge.top = rc.bottom;
				rcEdge.bottom = pRect->bottom;
				return IsWindowEdgeVisible(hwnd, hwndNext, &rcEdge, hwndTarget);
			}
		}
	}
	return IsWindowEdgeVisible(hwnd, hwndNext, pRect, hwndTarget);
}


struct SnapWindowInfo {
	HWND hwnd;
	RECT rcOriginal;
	RECT rcNearest;
	HWND hwndExclude;
};

static BOOL CALLBACK SnapWindowProc(HWND hwnd, LPARAM lParam)
{
	SnapWindowInfo *pInfo = reinterpret_cast<SnapWindowInfo*>(lParam);

	if (IsWindowVisible(hwnd) && hwnd != pInfo->hwnd && hwnd != pInfo->hwndExclude) {
		RECT rc, rcEdge;

		GetWindowRect(hwnd, &rc);
		if (rc.right > rc.left && rc.bottom > rc.top) {
			if (rc.top < pInfo->rcOriginal.bottom && rc.bottom > pInfo->rcOriginal.top) {
				if (std::abs(rc.left - pInfo->rcOriginal.right) < std::abs(pInfo->rcNearest.right)) {
					rcEdge.left = rc.left;
					rcEdge.right = rc.left;
					rcEdge.top = std::max(rc.top, pInfo->rcOriginal.top);
					rcEdge.bottom = std::min(rc.bottom, pInfo->rcOriginal.bottom);
					if (IsWindowEdgeVisible(hwnd, GetTopWindow(GetDesktopWindow()), &rcEdge, pInfo->hwnd))
						pInfo->rcNearest.right = rc.left - pInfo->rcOriginal.right;
				}
				if (std::abs(rc.right - pInfo->rcOriginal.left) < std::abs(pInfo->rcNearest.left)) {
					rcEdge.left = rc.right;
					rcEdge.right = rc.right;
					rcEdge.top = std::max(rc.top, pInfo->rcOriginal.top);
					rcEdge.bottom = std::min(rc.bottom, pInfo->rcOriginal.bottom);
					if (IsWindowEdgeVisible(hwnd, GetTopWindow(GetDesktopWindow()), &rcEdge, pInfo->hwnd))
						pInfo->rcNearest.left = rc.right - pInfo->rcOriginal.left;
				}
			}
			if (rc.left < pInfo->rcOriginal.right && rc.right > pInfo->rcOriginal.left) {
				if (std::abs(rc.top - pInfo->rcOriginal.bottom) < std::abs(pInfo->rcNearest.bottom)) {
					rcEdge.left = std::max(rc.left, pInfo->rcOriginal.left);
					rcEdge.right = std::min(rc.right, pInfo->rcOriginal.right);
					rcEdge.top = rc.top;
					rcEdge.bottom = rc.top;
					if (IsWindowEdgeVisible(hwnd, GetTopWindow(GetDesktopWindow()), &rcEdge, pInfo->hwnd))
						pInfo->rcNearest.bottom = rc.top - pInfo->rcOriginal.bottom;
				}
				if (std::abs(rc.bottom - pInfo->rcOriginal.top) < std::abs(pInfo->rcNearest.top)) {
					rcEdge.left = std::max(rc.left, pInfo->rcOriginal.left);
					rcEdge.right = std::min(rc.right, pInfo->rcOriginal.right);
					rcEdge.top = rc.bottom;
					rcEdge.bottom = rc.bottom;
					if (IsWindowEdgeVisible(hwnd, GetTopWindow(GetDesktopWindow()), &rcEdge, pInfo->hwnd))
						pInfo->rcNearest.top = rc.bottom - pInfo->rcOriginal.top;
				}
			}
		}
	}
	return TRUE;
}


void SnapWindow(HWND hwnd, RECT *prc, int Margin, HWND hwndExclude)
{
	RECT rc;

	const HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
	if (hMonitor != nullptr) {
		MONITORINFO mi;

		mi.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(hMonitor, &mi);
		rc = mi.rcMonitor;
	} else {
		rc.left = 0;
		rc.top = 0;
		rc.right = GetSystemMetrics(SM_CXSCREEN);
		rc.bottom = GetSystemMetrics(SM_CYSCREEN);
	}

	SnapWindowInfo Info;
	Info.hwnd = hwnd;
	Info.rcOriginal = *prc;
	Info.rcNearest.left = rc.left - prc->left;
	Info.rcNearest.top = rc.top - prc->top;
	Info.rcNearest.right = rc.right - prc->right;
	Info.rcNearest.bottom = rc.bottom - prc->bottom;
	Info.hwndExclude = hwndExclude;
	EnumWindows(SnapWindowProc, reinterpret_cast<LPARAM>(&Info));

	int XOffset, YOffset;
	if (std::abs(Info.rcNearest.left) < std::abs(Info.rcNearest.right)
			|| Info.rcNearest.left == Info.rcNearest.right)
		XOffset = Info.rcNearest.left;
	else if (std::abs(Info.rcNearest.left) > std::abs(Info.rcNearest.right))
		XOffset = Info.rcNearest.right;
	else
		XOffset = 0;
	if (std::abs(Info.rcNearest.top) < std::abs(Info.rcNearest.bottom)
			|| Info.rcNearest.top == Info.rcNearest.bottom)
		YOffset = Info.rcNearest.top;
	else if (std::abs(Info.rcNearest.top) > std::abs(Info.rcNearest.bottom))
		YOffset = Info.rcNearest.bottom;
	else
		YOffset = 0;
	if (std::abs(XOffset) <= Margin)
		prc->left += XOffset;
	if (std::abs(YOffset) <= Margin)
		prc->top += YOffset;
	prc->right = prc->left + (Info.rcOriginal.right - Info.rcOriginal.left);
	prc->bottom = prc->top + (Info.rcOriginal.bottom - Info.rcOriginal.top);
}


bool IsMessageInQueue(HWND hwnd, UINT Message)
{
	MSG msg;

	if (::PeekMessage(&msg, hwnd, Message, Message, PM_NOREMOVE)) {
		if (msg.message == WM_QUIT) {
			::PostQuitMessage(static_cast<int>(msg.wParam));
		} else {
			return true;
		}
	}

	return false;
}




void CMouseLeaveTrack::Initialize(HWND hwnd)
{
	m_hwnd = hwnd;
	m_fClientTrack = false;
	m_fNonClientTrack = false;
}

bool CMouseLeaveTrack::OnMouseMove()
{
	// WM_MOUSELEAVE が送られなくても無効にされる事があるようだ
	/*if (!m_fClientTrack)*/ {
		TRACKMOUSEEVENT tme;

		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = m_hwnd;
		if (!::TrackMouseEvent(&tme))
			return false;
		m_fClientTrack = true;
	}
	return true;
}

bool CMouseLeaveTrack::OnMouseLeave()
{
	m_fClientTrack = false;
	//return !IsCursorInWindow();
	return !m_fNonClientTrack;
}

bool CMouseLeaveTrack::OnNcMouseMove()
{
	/*if (!m_fNonClientTrack)*/ {
		TRACKMOUSEEVENT tme;

		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE | TME_NONCLIENT;
		tme.hwndTrack = m_hwnd;
		if (!::TrackMouseEvent(&tme))
			return false;
		m_fNonClientTrack = true;
	}
	return true;
}

bool CMouseLeaveTrack::OnNcMouseLeave()
{
	m_fNonClientTrack = false;
	//return !IsCursorInWindow();
	return !m_fClientTrack;
}

bool CMouseLeaveTrack::OnMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
	case WM_MOUSEMOVE:
		OnMouseMove();
		return true;

	case WM_MOUSELEAVE:
		OnMouseLeave();
		return true;

	case WM_NCMOUSEMOVE:
		OnNcMouseMove();
		return true;

	case WM_NCMOUSELEAVE:
		OnNcMouseLeave();
		return true;
	}

	return false;
}

bool CMouseLeaveTrack::IsCursorInWindow() const
{
	const DWORD Pos = ::GetMessagePos();
	const POINT pt = {GET_X_LPARAM(Pos), GET_Y_LPARAM(Pos)};
	RECT rc;

	::GetWindowRect(m_hwnd, &rc);
	return ::PtInRect(&rc, pt) != FALSE;
}


void CMouseWheelHandler::Reset()
{
	m_DeltaSum = 0;
	m_LastDelta = 0;
	m_LastTime = 0;
}

void CMouseWheelHandler::ResetDelta()
{
	m_DeltaSum = 0;
	m_LastDelta = 0;
}

int CMouseWheelHandler::OnWheel(int Delta)
{
	const DWORD CurTime = ::GetTickCount();

	if (static_cast<DWORD>(CurTime - m_LastTime) > 500
			|| (Delta > 0) != (m_LastDelta > 0)) {
		m_DeltaSum = 0;
	}

	m_DeltaSum += Delta;
	m_LastDelta = Delta;
	m_LastTime = CurTime;

	return m_DeltaSum;
}

int CMouseWheelHandler::OnMouseWheel(WPARAM wParam, int ScrollLines)
{
	const int Delta = OnWheel(GET_WHEEL_DELTA_WPARAM(wParam));
	if (std::abs(Delta) < WHEEL_DELTA)
		return 0;

	if (ScrollLines == 0)
		ScrollLines = GetDefaultScrollLines();

	ResetDelta();

	return ::MulDiv(Delta, ScrollLines, WHEEL_DELTA);
}

int CMouseWheelHandler::OnMouseHWheel(WPARAM wParam, int ScrollChars)
{
	const int Delta = OnWheel(GET_WHEEL_DELTA_WPARAM(wParam));
	if (std::abs(Delta) < WHEEL_DELTA)
		return 0;

	if (ScrollChars == 0)
		ScrollChars = GetDefaultScrollChars();

	ResetDelta();

	return ::MulDiv(Delta, ScrollChars, WHEEL_DELTA);
}

int CMouseWheelHandler::GetDefaultScrollLines() const
{
	UINT Lines;

	if (::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &Lines, 0))
		return Lines;
	return 2;
}

int CMouseWheelHandler::GetDefaultScrollChars() const
{
	UINT Chars;

	if (::SystemParametersInfo(SPI_GETWHEELSCROLLCHARS, 0, &Chars, 0))
		return Chars;
	return 3;
}


void CWindowTimerManager::InitializeTimer(HWND hwnd)
{
	m_hwndTimer = hwnd;
	m_TimerIDs = 0;
}


bool CWindowTimerManager::BeginTimer(unsigned int ID, DWORD Interval)
{
	if (m_hwndTimer == nullptr
			|| ::SetTimer(m_hwndTimer, ID, Interval, nullptr) == 0)
		return false;
	m_TimerIDs |= ID;
	return true;
}


void CWindowTimerManager::EndTimer(unsigned int ID)
{
	if ((m_TimerIDs & ID) != 0) {
		::KillTimer(m_hwndTimer, ID);
		m_TimerIDs &= ~ID;
	}
}


void CWindowTimerManager::EndAllTimers()
{
	unsigned int Flags = m_TimerIDs;
	for (int i = 0; Flags != 0; i++, Flags >>= 1) {
		const unsigned int ID = m_TimerIDs & (1U << i);
		if (ID != 0)
			EndTimer(ID);
	}
}


bool CWindowTimerManager::IsTimerEnabled(unsigned int ID) const
{
	return (m_TimerIDs & ID) == ID;
}




CWindowSubclass::~CWindowSubclass()
{
	RemoveSubclass();
}


bool CWindowSubclass::SetSubclass(HWND hwnd)
{
	RemoveSubclass();

	if (hwnd == nullptr)
		return false;

	if (!::SetWindowSubclass(
				hwnd, SubclassProc,
				reinterpret_cast<UINT_PTR>(this),
				reinterpret_cast<DWORD_PTR>(this)))
		return false;

	m_hwnd = hwnd;

	return true;
}


void CWindowSubclass::RemoveSubclass()
{
	if (m_hwnd != nullptr) {
		::RemoveWindowSubclass(m_hwnd, SubclassProc, reinterpret_cast<UINT_PTR>(this));
		m_hwnd = nullptr;
	}
}


LRESULT CALLBACK CWindowSubclass::SubclassProc(
	HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	CWindowSubclass *pThis = reinterpret_cast<CWindowSubclass*>(dwRefData);

	if (uMsg == WM_NCDESTROY) {
		pThis->RemoveSubclass();
		const LRESULT Result = pThis->OnMessage(hWnd, uMsg, wParam, lParam);
		pThis->OnSubclassRemoved();
		return Result;
	}

	return pThis->OnMessage(hWnd, uMsg, wParam, lParam);
}


LRESULT CWindowSubclass::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return ::DefSubclassProc(hwnd, uMsg, wParam, lParam);
}


}	// namespace TVTest
