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


#ifndef TVTEST_TASK_TRAY_H
#define TVTEST_TASK_TRAY_H


namespace TVTest
{

	class CTaskTrayManager
	{
	public:
		enum class StatusFlag : unsigned int {
			None      = 0x0000U,
			Recording = 0x0001U,
			Minimized = 0x0002U,
			Standby   = 0x0004U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		enum {
			MESSAGE_ICON_NONE,
			MESSAGE_ICON_INFO,
			MESSAGE_ICON_WARNING,
			MESSAGE_ICON_ERROR
		};

		CTaskTrayManager() = default;
		~CTaskTrayManager();

		CTaskTrayManager(const CTaskTrayManager &) = delete;
		CTaskTrayManager &operator=(const CTaskTrayManager &) = delete;

		bool Initialize(HWND hwnd, UINT Message);
		void Finalize();
		bool SetResident(bool fResident);
		bool GetResident() const { return m_fResident; }
		bool SetMinimizeToTray(bool fMinimizeToTray);
		bool GetMinimizeToTray() const { return m_fMinimizeToTray; }
		bool SetStatus(StatusFlag Status, StatusFlag Mask = EnumNot(StatusFlag::None));
		StatusFlag GetStatus() const { return m_Status; }
		bool SetTipText(LPCTSTR pszText);
		bool ShowMessage(LPCTSTR pszText, LPCTSTR pszTitle, int Icon = 0, DWORD TimeOut = 5000);
		bool HandleMessage(UINT Message, WPARAM wParam, LPARAM lParam);

	private:
		HWND m_hwnd = nullptr;
		UINT m_TrayIconMessage = 0;
		bool m_fResident = false;
		bool m_fMinimizeToTray = true;
		StatusFlag m_Status = StatusFlag::None;
		UINT m_TaskbarCreatedMessage = 0;
		String m_TipText;

		bool AddTrayIcon();
		bool RemoveTrayIcon();
		bool ChangeTrayIcon();
		HICON LoadTrayIcon() const;
		bool UpdateTipText();
		bool NeedTrayIcon() const;
	};

} // namespace TVTest


#endif
