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
#include "TaskTray.h"
#include "AppMain.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CTaskTrayManager::~CTaskTrayManager()
{
	Finalize();
}


bool CTaskTrayManager::Initialize(HWND hwnd, UINT Message)
{
	m_hwnd = hwnd;
	m_TrayIconMessage = Message;
	m_TaskbarCreatedMessage = ::RegisterWindowMessage(TEXT("TaskbarCreated"));
	if (NeedTrayIcon()) {
		if (!AddTrayIcon())
			return false;
	}
	return true;
}


void CTaskTrayManager::Finalize()
{
	if (NeedTrayIcon())
		RemoveTrayIcon();
}


bool CTaskTrayManager::SetResident(bool fResident)
{
	if (m_fResident != fResident) {
		if (m_hwnd != nullptr && !NeedTrayIcon() && fResident) {
			if (!AddTrayIcon())
				return false;
		}
		m_fResident = fResident;
		if (m_hwnd != nullptr && !NeedTrayIcon())
			RemoveTrayIcon();
	}
	return true;
}


bool CTaskTrayManager::SetMinimizeToTray(bool fMinimizeToTray)
{
	if (m_fMinimizeToTray != fMinimizeToTray) {
		if (m_hwnd != nullptr) {
			if (!!(m_Status & StatusFlag::Minimized)) {
				if (fMinimizeToTray && !NeedTrayIcon()) {
					if (!AddTrayIcon())
						return false;
				}
				::ShowWindow(m_hwnd, fMinimizeToTray ? SW_HIDE : SW_SHOWMINNOACTIVE);
			}
		}
		m_fMinimizeToTray = fMinimizeToTray;
		if (m_hwnd != nullptr && !NeedTrayIcon())
			RemoveTrayIcon();
	}
	return true;
}


bool CTaskTrayManager::AddTrayIcon()
{
	NOTIFYICONDATA nid;

	nid.cbSize = NOTIFYICONDATA_V2_SIZE;
	nid.hWnd = m_hwnd;
	nid.uID = 1;
	nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	nid.uCallbackMessage = m_TrayIconMessage;
	nid.hIcon = LoadTrayIcon();
	StringCopy(nid.szTip, !m_TipText.empty() ? m_TipText.c_str() : APP_NAME);
	const bool fResult = ::Shell_NotifyIcon(NIM_ADD, &nid) != FALSE;
	::DestroyIcon(nid.hIcon);
	if (!fResult)
		return false;
	/*
	nid.uVersion = NOTIFYICON_VERSION;
	::Shell_NotifyIcon(NIM_SETVERSION, &nid);
	*/
	return true;
}


bool CTaskTrayManager::RemoveTrayIcon()
{
	NOTIFYICONDATA nid;

	nid.cbSize = NOTIFYICONDATA_V2_SIZE;
	nid.hWnd = m_hwnd;
	nid.uID = 1;
	nid.uFlags = 0;
	return Shell_NotifyIcon(NIM_DELETE, &nid) != FALSE;
}


bool CTaskTrayManager::ChangeTrayIcon()
{
	NOTIFYICONDATA nid;

	nid.cbSize = NOTIFYICONDATA_V2_SIZE;
	nid.hWnd = m_hwnd;
	nid.uID = 1;
	nid.uFlags = NIF_ICON;
	nid.hIcon = LoadTrayIcon();
	const bool fResult = Shell_NotifyIcon(NIM_MODIFY, &nid) != FALSE;
	::DestroyIcon(nid.hIcon);
	if (!fResult) {
		if (!AddTrayIcon())
			return false;
	}

	return true;
}


HICON CTaskTrayManager::LoadTrayIcon() const
{
	return LoadIconStandardSize(
		GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE(!!(m_Status & StatusFlag::Recording) ? IDI_TRAY_RECORDING : IDI_TRAY),
		IconSizeType::Small);
}


bool CTaskTrayManager::UpdateTipText()
{
	NOTIFYICONDATA nid;

	nid.cbSize = NOTIFYICONDATA_V2_SIZE;
	nid.hWnd = m_hwnd;
	nid.uID = 1;
	nid.uFlags = NIF_TIP;
	StringCopy(nid.szTip, !m_TipText.empty() ? m_TipText.c_str() : APP_NAME);
	return ::Shell_NotifyIcon(NIM_MODIFY, &nid) != FALSE;
}


bool CTaskTrayManager::NeedTrayIcon() const
{
	return m_hwnd != nullptr
		&& (m_fResident
			|| !!(m_Status & StatusFlag::Standby)
			|| (m_fMinimizeToTray && !!(m_Status & StatusFlag::Minimized)));
}


bool CTaskTrayManager::SetStatus(StatusFlag Status, StatusFlag Mask)
{
	const StatusFlag NewStatus = (m_Status & ~Mask) | (Status & Mask);

	if (m_Status != NewStatus) {
		const StatusFlag StatusDiff = m_Status ^ NewStatus;
		const bool fNeedTrayIconOld = NeedTrayIcon();
		bool fChangeIcon = false;

		if (!!(StatusDiff & StatusFlag::Recording))
			fChangeIcon = true;
		if (!!(StatusDiff & StatusFlag::Minimized)) {
			if (m_hwnd != nullptr && m_fMinimizeToTray)
				::ShowWindow(m_hwnd, !!(NewStatus & StatusFlag::Minimized) ? SW_HIDE : SW_SHOW);
		}

		m_Status = NewStatus;

		if (NeedTrayIcon()) {
			if (!fNeedTrayIconOld)
				AddTrayIcon();
			else if (fChangeIcon)
				ChangeTrayIcon();
		} else {
			if (fNeedTrayIconOld)
				RemoveTrayIcon();
		}
	}

	return true;
}


bool CTaskTrayManager::SetTipText(LPCTSTR pszText)
{
	if (StringUtility::Compare(m_TipText, pszText) != 0) {
		StringUtility::Assign(m_TipText, pszText);
		if (NeedTrayIcon())
			UpdateTipText();
	}
	return true;
}


bool CTaskTrayManager::ShowMessage(LPCTSTR pszText, LPCTSTR pszTitle, int Icon, DWORD TimeOut)
{
	if (!NeedTrayIcon())
		return false;

	NOTIFYICONDATA nid = {};

	nid.cbSize = NOTIFYICONDATA_V2_SIZE;
	nid.hWnd = m_hwnd;
	nid.uID = 1;
	nid.uFlags = NIF_INFO;
	StringCopy(nid.szInfo, pszText);
	if (pszTitle)
		StringCopy(nid.szInfoTitle, pszTitle);
	nid.dwInfoFlags =
		Icon == MESSAGE_ICON_INFO ? NIIF_INFO :
		Icon == MESSAGE_ICON_WARNING ? NIIF_WARNING :
		Icon == MESSAGE_ICON_ERROR ? NIIF_ERROR : NIIF_NONE;
	nid.uTimeout = TimeOut;
	return Shell_NotifyIcon(NIM_MODIFY, &nid) != FALSE;
}


bool CTaskTrayManager::HandleMessage(UINT Message, WPARAM wParam, LPARAM lParam)
{
	// シェルが再起動した時の対策
	if (m_TaskbarCreatedMessage != 0
			&& Message == m_TaskbarCreatedMessage) {
		if (NeedTrayIcon())
			AddTrayIcon();
		return true;
	}
	return false;
}


} // namespace TVTest
