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


#ifndef TVTEST_TASKBAR_OPTIONS_H
#define TVTEST_TASKBAR_OPTIONS_H


#include <vector>
#include "Options.h"


namespace TVTest
{

	class CTaskbarOptions
		: public COptions
	{
	public:
		typedef std::vector<int> TaskList;

		CTaskbarOptions();

	// COptions
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	// CTaskbarOptions
		bool IsJumpListEnabled() const;
		void SetEnableJumpList(bool fEnable);
		bool GetEnableJumpList() const { return m_fEnableJumpList; }
		bool GetShowTasks() const { return m_fShowTasks; }
		const TaskList &GetTaskList() const { return m_TaskList; }
		bool GetShowRecentChannels() const { return m_fShowRecentChannels; }
		int GetMaxRecentChannels() const { return m_MaxRecentChannels; }
		bool GetShowChannelIcon() const { return m_fShowChannelIcon; }
		const String &GetIconDirectory() const { return m_IconDirectory; }
		void SetJumpListKeepSingleTask(bool fSingleTask) { m_fJumpListKeepSingleTask = fSingleTask; }
		bool GetJumpListKeepSingleTask() const { return m_fJumpListKeepSingleTask; }
		void SetUseUniqueAppID(bool fUnique) { m_fUseUniqueAppID = fUnique; }
		bool GetUseUniqueAppID() const { return m_fUseUniqueAppID; }
		const String &GetAppID() const { return m_AppID; }

	private:
		bool m_fEnableJumpList = true;
		bool m_fShowTasks = true;
		TaskList m_TaskList;
		bool m_fShowRecentChannels = true;
		int m_MaxRecentChannels = 10;
		bool m_fShowChannelIcon = true;
		String m_IconDirectory{TEXT(".\\JumpListIcons")};
		bool m_fJumpListKeepSingleTask = false;
		bool m_fUseUniqueAppID = false;
		String m_AppID{TEXT("DBCTRADO.") APP_NAME};

		static const int m_DefaultTaskList[];
	};

}	// namespace TVTest


#endif
