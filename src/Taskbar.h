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


#ifndef TVTEST_TASKBAR_H
#define TVTEST_TASKBAR_H


#include <vector>
#include <map>
#include "TaskbarSharedProperties.h"
#include "Settings.h"


namespace TVTest
{

	class CTaskbarManager
	{
	public:
		CTaskbarManager() = default;
		~CTaskbarManager();

		CTaskbarManager(const CTaskbarManager &) = delete;
		CTaskbarManager &operator=(const CTaskbarManager &) = delete;

		void SetAppID(LPCTSTR pszID);
		bool Initialize(HWND hwnd);
		void Finalize();
		bool HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
		bool SetRecordingStatus(bool fRecording);
		bool SetProgress(int Pos, int Max);
		bool EndProgress();
		bool ReinitializeJumpList();
		bool UpdateJumpList();
		bool ClearJumpList();
		bool AddRecentChannel(const CTunerChannelInfo &Info);
		bool ClearRecentChannelList();
		bool LoadSettings(CSettings &Settings);
		bool SaveSettings(CSettings &Settings);

	private:
		struct JumpListItem
		{
			String Title;
			String Args;
			String Description;
			String IconPath;
			int IconIndex;
		};
		typedef std::vector<JumpListItem> JumpListItemList;

		struct ChannelIconInfo
		{
			FILETIME UpdatedTime;

			ChannelIconInfo() = default;
			ChannelIconInfo(const FILETIME &Time) : UpdatedTime(Time) {}
		};

		static DWORD ChannelIconMapKey(WORD NetworkID, WORD ServiceID) {
			return (static_cast<DWORD>(NetworkID) << 16) | ServiceID;
		}

		struct CommandIconInfo
		{
			int Command;
			int Icon;
		};

		static const CommandIconInfo m_CommandIconList[];

		HWND m_hwnd = nullptr;
		UINT m_TaskbarButtonCreatedMessage = 0;
		interface ITaskbarList3 *m_pTaskbarList = nullptr;
		String m_AppID;
		bool m_fAppIDInvalid = false;
		bool m_fJumpListInitialized = false;
		CTaskbarSharedProperties m_SharedProperties;
		CRecentChannelList m_RecentChannelList;
		bool m_fSaveRecentChannelList = false;
		std::map<DWORD, ChannelIconInfo> m_ChannelIconMap;

		HRESULT InitializeJumpList();
		HRESULT AddTaskList(interface ICustomDestinationList *pcdl);
		HRESULT CreateAppShellLink(
			PCWSTR pszArgs, PCWSTR pszTitle, PCWSTR pszDescription,
			PCWSTR pszIconPath, int IconIndex,
			IShellLink **ppShellLink);
		HRESULT CreateSeparatorShellLink(IShellLink **ppShellLink);
		HRESULT AddJumpListCategory(
			interface ICustomDestinationList *pcdl,
			PCWSTR pszTitle, const JumpListItemList &ItemList);
		HRESULT AddRecentChannelsCategory(interface ICustomDestinationList *pcdl);
		int GetCommandIcon(int Command) const;
	};

}	// namespace TVTest


#endif
