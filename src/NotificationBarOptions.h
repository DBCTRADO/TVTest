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


#ifndef TVTEST_NOTIFICATION_BAR_OPTIONS_H
#define TVTEST_NOTIFICATION_BAR_OPTIONS_H


#include "Options.h"
#include "Style.h"


namespace TVTest
{

	class CNotificationBarOptions
		: public COptions
	{
	public:
		enum {
			NOTIFY_EVENTNAME        = 0x00000001,
			NOTIFY_TSPROCESSORERROR = 0x00000002,
		};

		CNotificationBarOptions();
		~CNotificationBarOptions();

	// CSettingsBase
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	// CBasicDialog
		bool Create(HWND hwndOwner) override;

	// CNotificationBarOptions
		bool IsNotificationBarEnabled() const { return m_fEnableNotificationBar; }
		int GetNotificationBarDuration() const { return m_NotificationBarDuration; }
		const Style::Font &GetNotificationBarFont() const { return m_NotificationBarFont; }
		bool IsNotifyEnabled(unsigned int Type) const;

	private:
	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		void EnableNotify(unsigned int Type, bool fEnabled);

		bool m_fEnableNotificationBar = true;
		int m_NotificationBarDuration = 3000;
		unsigned int m_NotificationBarFlags = NOTIFY_EVENTNAME | NOTIFY_TSPROCESSORERROR;
		Style::Font m_NotificationBarFont;
		Style::Font m_CurNotificationBarFont;
	};

} // namespace TVTest


#endif
