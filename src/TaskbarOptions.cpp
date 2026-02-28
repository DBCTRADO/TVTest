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
#include "TaskbarOptions.h"
#include "AppMain.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


const int CTaskbarOptions::m_DefaultTaskList[] = {
	CM_FULLSCREEN,
	CM_DISABLEVIEWER,
	CM_PROGRAMGUIDE,
};


CTaskbarOptions::CTaskbarOptions()
	: COptions(TEXT("Taskbar"))
{
	m_TaskList.resize(lengthof(m_DefaultTaskList));
	for (int i = 0; i < lengthof(m_DefaultTaskList); i++)
		m_TaskList[i] = m_DefaultTaskList[i];
}


bool CTaskbarOptions::ReadSettings(CSettings &Settings)
{
	Settings.Read(TEXT("EnableJumpList"), &m_fEnableJumpList);
	Settings.Read(TEXT("ShowTasks"), &m_fShowTasks);
	Settings.Read(TEXT("ShowRecentChannels"), &m_fShowRecentChannels);
	Settings.Read(TEXT("MaxRecentChannels"), &m_MaxRecentChannels);
	Settings.Read(TEXT("ShowChannelIcon"), &m_fShowChannelIcon);
	Settings.Read(TEXT("IconDirectory"), &m_IconDirectory);
	Settings.Read(TEXT("JumpListKeepSingleTask"), &m_fJumpListKeepSingleTask);
	Settings.Read(TEXT("UseUniqueAppID"), &m_fUseUniqueAppID);
	Settings.Read(TEXT("AppID"), &m_AppID);

	int TaskCount;
	if (Settings.Read(TEXT("TaskCount"), &TaskCount)) {
		m_TaskList.clear();

		const CCommandManager &CommandManager = GetAppClass().CommandManager;
		String Command;

		for (int i = 0; TaskCount; i++) {
			TCHAR szKey[32];
			StringFormat(szKey, TEXT("Task{}"), i);
			if (!Settings.Read(szKey, &Command))
				break;
			StringUtility::Trim(Command);
			if (Command.empty()) {
				m_TaskList.push_back(0); // Separator
			} else {
				const int ID = CommandManager.ParseIDText(Command);
				if (ID != 0)
					m_TaskList.push_back(ID);
			}
		}
	}

	return true;
}


bool CTaskbarOptions::WriteSettings(CSettings &Settings)
{
#if 0 // まだ設定インターフェースが無い
	Settings.Clear();

	Settings.Write(TEXT("ShowTasks"), m_fShowTasks);
	Settings.Write(TEXT("ShowRecentChannels"), m_fShowRecentChannels);
	Settings.Write(TEXT("MaxRecentChannels"), m_MaxRecentChannels);
	Settings.Write(TEXT("ShowChannelIcon"), m_fShowChannelIcon);
	Settings.Write(TEXT("IconDirectory"), m_IconDirectory);

	Settings.Write(TEXT("TaskCount"), static_cast<int>(m_TaskList.size()));
	const CCommandManager &CommandManager = GetAppClass().CommandManager;
	for (size_t i = 0; i < m_TaskList.size(); i++) {
		TCHAR szKey[32];
		StringFormat(szKey, TEXT("Task{}"), i);
		Settings.Write(szKey, CommandManager.GetCommandIDText(m_TaskList[i]));
	}
#endif

	Settings.Write(TEXT("EnableJumpList"), m_fEnableJumpList);
	Settings.Write(TEXT("JumpListKeepSingleTask"), m_fJumpListKeepSingleTask);
	Settings.Write(TEXT("UseUniqueAppID"), m_fUseUniqueAppID);

	return true;
}


bool CTaskbarOptions::IsJumpListEnabled() const
{
	return m_fEnableJumpList
		&& ((m_fShowTasks && !m_TaskList.empty())
			|| (m_fShowRecentChannels && m_MaxRecentChannels > 0));
}


void CTaskbarOptions::SetEnableJumpList(bool fEnable)
{
	if (m_fEnableJumpList != fEnable) {
		m_fEnableJumpList = fEnable;
		GetAppClass().TaskbarManager.ReinitializeJumpList();
	}
}


} // namespace TVTest
