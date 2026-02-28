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
#include "Tooltip.h"
#include "Common/DebugDef.h"

#ifndef TTTOOLINFO_V2_SIZE
#ifdef UNICODE
#define TTTOOLINFO_V2_SIZE TTTOOLINFOW_V2_SIZE
#else
#define TTTOOLINFO_V2_SIZE TTTOOLINFOA_V2_SIZE
#endif
#endif



namespace TVTest
{


CTooltip::~CTooltip()
{
	Destroy();
}


bool CTooltip::Create(HWND hwnd)
{
	if (m_hwndTooltip != nullptr)
		return false;

	m_hwndTooltip = ::CreateWindowEx(
		WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr,
		WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
		0, 0, 0, 0,
		hwnd, nullptr, GetWindowInstance(hwnd), nullptr);
	if (m_hwndTooltip == nullptr)
		return false;

	m_hwndParent = hwnd;

	return true;
}


void CTooltip::Destroy()
{
	if (m_hwndTooltip != nullptr) {
		::DestroyWindow(m_hwndTooltip);
		m_hwndTooltip = nullptr;
	}
}


void CTooltip::DeleteAllTools()
{
	if (m_hwndTooltip != nullptr) {
		const int Count = static_cast<int>(::SendMessage(m_hwndTooltip, TTM_GETTOOLCOUNT, 0, 0));
		TOOLINFO ti;

		ti.cbSize = TTTOOLINFO_V1_SIZE;
		ti.lpszText = nullptr;
		for (int i = Count - 1; i >= 0; i--) {
			if (::SendMessage(m_hwndTooltip, TTM_ENUMTOOLS, i, reinterpret_cast<LPARAM>(&ti)))
				::SendMessage(m_hwndTooltip, TTM_DELTOOL, 0, reinterpret_cast<LPARAM>(&ti));
		}
	}
}


bool CTooltip::Enable(bool fEnable)
{
	if (m_hwndTooltip == nullptr)
		return false;
	::SendMessage(m_hwndTooltip, TTM_ACTIVATE, fEnable, 0);
	return true;
}


bool CTooltip::IsVisible() const
{
	return m_hwndTooltip != nullptr && ::IsWindowVisible(m_hwndTooltip) != FALSE;
}


bool CTooltip::SetMaxWidth(int Width)
{
	if (m_hwndTooltip == nullptr)
		return false;
	::SendMessage(m_hwndTooltip, TTM_SETMAXTIPWIDTH, 0, Width);
	return true;
}


bool CTooltip::SetPopDelay(int Delay)
{
	if (m_hwndTooltip == nullptr)
		return false;
	::SendMessage(
		m_hwndTooltip, TTM_SETDELAYTIME, TTDT_AUTOPOP,
		MAKELONG(std::min(Delay, 32767), 0));
	return true;
}


int CTooltip::NumTools() const
{
	if (m_hwndTooltip == nullptr)
		return 0;
	return static_cast<int>(::SendMessage(m_hwndTooltip, TTM_GETTOOLCOUNT, 0, 0));
}


bool CTooltip::AddTool(UINT ID, const RECT &Rect, LPCTSTR pszText, LPARAM lParam)
{
	if (m_hwndTooltip == nullptr)
		return false;

	TOOLINFO ti;

	ti.cbSize = TTTOOLINFO_V2_SIZE;
	ti.uFlags = TTF_SUBCLASS | TTF_TRANSPARENT;
	ti.hwnd = m_hwndParent;
	ti.uId = ID;
	ti.rect = Rect;
	ti.hinst = nullptr;
	ti.lpszText = const_cast<LPTSTR>(pszText);
	ti.lParam = lParam;
	return ::SendMessage(m_hwndTooltip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti)) != FALSE;
}


bool CTooltip::AddTool(HWND hwnd, LPCTSTR pszText, LPARAM lParam)
{
	if (m_hwndTooltip == nullptr || hwnd == nullptr)
		return false;

	TOOLINFO ti;

	ti.cbSize = TTTOOLINFO_V2_SIZE;
	ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS | TTF_TRANSPARENT;
	ti.hwnd = m_hwndParent;
	ti.uId = reinterpret_cast<UINT_PTR>(hwnd);
	ti.hinst = nullptr;
	ti.lpszText = const_cast<LPTSTR>(pszText);
	ti.lParam = lParam;
	return ::SendMessage(m_hwndTooltip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti)) != FALSE;
}


bool CTooltip::DeleteTool(UINT ID)
{
	if (m_hwndTooltip == nullptr)
		return false;

	TOOLINFO ti;

	ti.cbSize = TTTOOLINFO_V2_SIZE;
	ti.hwnd = m_hwndParent;
	ti.uId = ID;
	::SendMessage(m_hwndTooltip, TTM_DELTOOL, 0, reinterpret_cast<LPARAM>(&ti));
	return true;
}


bool CTooltip::SetToolRect(UINT ID, const RECT &Rect)
{
	if (m_hwndTooltip == nullptr)
		return false;

	TOOLINFO ti;

	ti.cbSize = TTTOOLINFO_V2_SIZE;
	ti.hwnd = m_hwndParent;
	ti.uId = ID;
	ti.rect = Rect;
	::SendMessage(m_hwndTooltip, TTM_NEWTOOLRECT, 0, reinterpret_cast<LPARAM>(&ti));
	return true;
}


bool CTooltip::SetText(UINT ID, LPCTSTR pszText)
{
	if (m_hwndTooltip == nullptr)
		return false;

	TOOLINFO ti;

	ti.cbSize = TTTOOLINFO_V2_SIZE;
	ti.hwnd = m_hwndParent;
	ti.uId = ID;
	ti.hinst = nullptr;
	ti.lpszText = const_cast<LPTSTR>(pszText);
	::SendMessage(m_hwndTooltip, TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&ti));
	return true;
}


bool CTooltip::AddTrackingTip(UINT ID, LPCTSTR pszText, LPARAM lParam)
{
	if (m_hwndTooltip == nullptr)
		return false;

	TOOLINFO ti;

	ti.cbSize = TTTOOLINFO_V2_SIZE;
	ti.uFlags = TTF_SUBCLASS | TTF_TRANSPARENT | TTF_TRACK | TTF_ABSOLUTE;
	ti.hwnd = m_hwndParent;
	ti.uId = ID;
	::SetRectEmpty(&ti.rect);
	ti.hinst = nullptr;
	ti.lpszText = const_cast<LPTSTR>(pszText);
	ti.lParam = lParam;
	return ::SendMessage(m_hwndTooltip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti)) != FALSE;
}


bool CTooltip::TrackActivate(UINT ID, bool fActivate)
{
	if (m_hwndTooltip == nullptr)
		return false;

	TOOLINFO ti;

	ti.cbSize = TTTOOLINFO_V2_SIZE;
	ti.hwnd = m_hwndParent;
	ti.uId = ID;
	::SendMessage(m_hwndTooltip, TTM_TRACKACTIVATE, fActivate, reinterpret_cast<LPARAM>(&ti));
	return true;
}


bool CTooltip::TrackPosition(int x, int y)
{
	if (m_hwndTooltip == nullptr)
		return false;

	::SendMessage(m_hwndTooltip, TTM_TRACKPOSITION, 0, MAKELONG(x, y));
	return true;
}


bool CTooltip::SetFont(HFONT hfont)
{
	if (m_hwndTooltip == nullptr || hfont == nullptr)
		return false;

	SetWindowFont(m_hwndTooltip, hfont, FALSE);
	return true;
}




CBalloonTip::~CBalloonTip()
{
	Finalize();
}


bool CBalloonTip::Initialize(HWND hwnd)
{
	if (m_hwndToolTips != nullptr)
		return false;

	m_hwndToolTips = ::CreateWindowEx(
		WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr,
		WS_POPUP | TTS_NOPREFIX | TTS_BALLOON/* | TTS_CLOSE*/,
		0, 0, 0, 0,
		nullptr, nullptr, GetAppClass().GetInstance(), nullptr);
	if (m_hwndToolTips == nullptr)
		return false;

	::SendMessage(m_hwndToolTips, TTM_SETMAXTIPWIDTH, 0, 320);

	TOOLINFO ti = {};

	ti.cbSize = TTTOOLINFO_V1_SIZE;
	ti.uFlags = TTF_SUBCLASS | TTF_TRACK;
	ti.hwnd = hwnd;
	ti.uId = 0;
	ti.hinst = nullptr;
	ti.lpszText = const_cast<LPTSTR>(TEXT(""));
	::SendMessage(m_hwndToolTips, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));

	m_hwndOwner = hwnd;

	return true;
}


void CBalloonTip::Finalize()
{
	if (m_hwndToolTips != nullptr) {
		::DestroyWindow(m_hwndToolTips);
		m_hwndToolTips = nullptr;
	}
}


bool CBalloonTip::Show(LPCTSTR pszText, LPCTSTR pszTitle, const POINT *pPos, int Icon)
{
	if (m_hwndToolTips == nullptr || pszText == nullptr)
		return false;
	TOOLINFO ti;
	ti.cbSize = TTTOOLINFO_V1_SIZE;
	ti.hwnd = m_hwndOwner;
	ti.uId = 0;
	ti.lpszText = const_cast<LPTSTR>(pszText);
	::SendMessage(m_hwndToolTips, TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&ti));
	::SendMessage(m_hwndToolTips, TTM_SETTITLE, Icon, reinterpret_cast<LPARAM>(pszTitle != nullptr ? pszTitle : TEXT("")));
	POINT pt;
	if (pPos != nullptr) {
		pt = *pPos;
	} else {
		RECT rc;
		::SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
		pt.x = rc.right - 32;
		pt.y = rc.bottom;
	}
	::SendMessage(m_hwndToolTips, TTM_TRACKPOSITION, 0, MAKELPARAM(pt.x, pt.y));
	::SendMessage(m_hwndToolTips, TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(&ti));
	return true;
}


bool CBalloonTip::Hide()
{
	TOOLINFO ti;

	ti.cbSize = TTTOOLINFO_V1_SIZE;
	ti.hwnd = m_hwndOwner;
	ti.uId = 0;
	::SendMessage(m_hwndToolTips, TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&ti));
	return true;
}


} // namespace TVTest
