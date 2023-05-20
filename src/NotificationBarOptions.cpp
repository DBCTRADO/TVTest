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
#include "NotificationBarOptions.h"
#include "DialogUtil.h"
#include "Util.h"
#include "StyleUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CNotificationBarOptions::CNotificationBarOptions()
{
	LOGFONT lf;
	DrawUtil::GetSystemFont(DrawUtil::FontType::Message, &lf);

	m_NotificationBarFont.LogFont = lf;
#ifndef TVTEST_FOR_1SEG
	m_NotificationBarFont.LogFont.lfHeight = lf.lfHeight * 12 / 10;
#endif
	Style::CStyleManager::AssignFontSizeFromLogFont(&m_NotificationBarFont);
}


CNotificationBarOptions::~CNotificationBarOptions()
{
	Destroy();
}


bool CNotificationBarOptions::ReadSettings(CSettings &Settings)
{
	Settings.Read(TEXT("EnableNotificationBar"), &m_fEnableNotificationBar);
	Settings.Read(TEXT("NotificationBarDuration"), &m_NotificationBarDuration);
	bool f;
	if (Settings.Read(TEXT("NotifyEventName"), &f))
		EnableNotify(NOTIFY_EVENTNAME, f);
	if (Settings.Read(TEXT("NotifyTSProcessorError"), &f))
		EnableNotify(NOTIFY_TSPROCESSORERROR, f);

	if (StyleUtil::ReadFontSettings(
				Settings, TEXT("NotificationBarFont"), &m_NotificationBarFont, true, &f)) {
		if (!f)
			m_fChanged = true;
	}

	return true;
}


bool CNotificationBarOptions::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("EnableNotificationBar"), m_fEnableNotificationBar);
	Settings.Write(TEXT("NotificationBarDuration"), m_NotificationBarDuration);
	Settings.Write(TEXT("NotifyEventName"), (m_NotificationBarFlags & NOTIFY_EVENTNAME) != 0);
	Settings.Write(TEXT("NotifyTSProcessorError"), (m_NotificationBarFlags & NOTIFY_TSPROCESSORERROR) != 0);

	StyleUtil::WriteFontSettings(Settings, TEXT("NotificationBarFont"), m_NotificationBarFont);

	return true;
}


bool CNotificationBarOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(
		hwndOwner,
		GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_OPTIONS_NOTIFICATIONBAR));
}


bool CNotificationBarOptions::IsNotifyEnabled(unsigned int Type) const
{
	return m_fEnableNotificationBar && (m_NotificationBarFlags & Type) != 0;
}


void CNotificationBarOptions::EnableNotify(unsigned int Type, bool fEnabled)
{
	if (fEnabled)
		m_NotificationBarFlags |= Type;
	else
		m_NotificationBarFlags &= ~Type;
}


INT_PTR CNotificationBarOptions::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			DlgCheckBox_Check(hDlg, IDC_NOTIFICATIONBAR_ENABLE, m_fEnableNotificationBar);
			DlgCheckBox_Check(hDlg, IDC_NOTIFICATIONBAR_NOTIFYEVENTNAME, (m_NotificationBarFlags & NOTIFY_EVENTNAME) != 0);
			DlgCheckBox_Check(hDlg, IDC_NOTIFICATIONBAR_NOTIFYTSPROCESSORERROR, (m_NotificationBarFlags & NOTIFY_TSPROCESSORERROR) != 0);
			::SetDlgItemInt(hDlg, IDC_NOTIFICATIONBAR_DURATION, m_NotificationBarDuration / 1000, FALSE);
			DlgUpDown_SetRange(hDlg, IDC_NOTIFICATIONBAR_DURATION_UPDOWN, 1, 60);
			m_CurNotificationBarFont = m_NotificationBarFont;
			StyleUtil::SetFontInfoItem(hDlg, IDC_NOTIFICATIONBAR_FONT_INFO, m_CurNotificationBarFont);
			EnableDlgItems(hDlg, IDC_NOTIFICATIONBAR_FIRST, IDC_NOTIFICATIONBAR_LAST, m_fEnableNotificationBar);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_NOTIFICATIONBAR_ENABLE:
			EnableDlgItemsSyncCheckBox(
				hDlg, IDC_NOTIFICATIONBAR_FIRST, IDC_NOTIFICATIONBAR_LAST, IDC_NOTIFICATIONBAR_ENABLE);
			return TRUE;

		case IDC_NOTIFICATIONBAR_FONT_CHOOSE:
			if (StyleUtil::ChooseStyleFont(hDlg, &m_CurNotificationBarFont))
				StyleUtil::SetFontInfoItem(hDlg, IDC_NOTIFICATIONBAR_FONT_INFO, m_CurNotificationBarFont);
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case PSN_APPLY:
			{
				m_fEnableNotificationBar =
					DlgCheckBox_IsChecked(hDlg, IDC_NOTIFICATIONBAR_ENABLE);
				EnableNotify(
					NOTIFY_EVENTNAME,
					DlgCheckBox_IsChecked(hDlg, IDC_NOTIFICATIONBAR_NOTIFYEVENTNAME));
				EnableNotify(
					NOTIFY_TSPROCESSORERROR,
					DlgCheckBox_IsChecked(hDlg, IDC_NOTIFICATIONBAR_NOTIFYTSPROCESSORERROR));
				m_NotificationBarDuration =
					::GetDlgItemInt(hDlg, IDC_NOTIFICATIONBAR_DURATION, nullptr, FALSE) * 1000;
				m_NotificationBarFont = m_CurNotificationBarFont;

				m_fChanged = true;
			}
			return TRUE;
		}
		break;
	}

	return FALSE;
}


}	// namespace TVTest
