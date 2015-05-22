#include "stdafx.h"
#include "TVTest.h"
#include "TaskbarOptions.h"
#include "AppMain.h"
#include "resource.h"
#include "Common/DebugDef.h"




const int CTaskbarOptions::m_DefaultTaskList[] = {
	CM_FULLSCREEN,
	CM_DISABLEVIEWER,
	CM_PROGRAMGUIDE,
};


CTaskbarOptions::CTaskbarOptions()
	: COptions(TEXT("Taskbar"))
	, m_fEnableJumpList(true)
	, m_fShowTasks(true)
	, m_fShowRecentChannels(true)
	, m_MaxRecentChannels(10)
	, m_fShowChannelIcon(true)
	, m_IconDirectory(TEXT(".\\JumpListIcons"))
	, m_fJumpListKeepSingleTask(false)
	, m_fUseUniqueAppID(false)
	, m_AppID(TEXT("DBCTRADO.") APP_NAME)
{
	m_TaskList.resize(lengthof(m_DefaultTaskList));
	for (int i=0;i<lengthof(m_DefaultTaskList);i++)
		m_TaskList[i]=m_DefaultTaskList[i];
}


CTaskbarOptions::~CTaskbarOptions()
{
}


bool CTaskbarOptions::ReadSettings(CSettings &Settings)
{
	Settings.Read(TEXT("EnableJumpList"),&m_fEnableJumpList);
	Settings.Read(TEXT("ShowTasks"),&m_fShowTasks);
	Settings.Read(TEXT("ShowRecentChannels"),&m_fShowRecentChannels);
	Settings.Read(TEXT("MaxRecentChannels"),&m_MaxRecentChannels);
	Settings.Read(TEXT("ShowChannelIcon"),&m_fShowChannelIcon);
	Settings.Read(TEXT("IconDirectory"),&m_IconDirectory);
	Settings.Read(TEXT("JumpListKeepSingleTask"),&m_fJumpListKeepSingleTask);
	Settings.Read(TEXT("UseUniqueAppID"),&m_fUseUniqueAppID);
	Settings.Read(TEXT("AppID"),&m_AppID);

	int TaskCount;
	if (Settings.Read(TEXT("TaskCount"),&TaskCount)) {
		m_TaskList.clear();
		const CCommandList &CommandList=GetAppClass().CommandList;
		for (int i=0;TaskCount;i++) {
			TCHAR szKey[32];
			TVTest::String Command;
			StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Task%d"),i);
			if (!Settings.Read(szKey,&Command))
				break;
			TVTest::StringUtility::Trim(Command);
			if (Command.empty()) {
				m_TaskList.push_back(0);	// Separator
			} else {
				int ID=CommandList.ParseText(Command.c_str());
				if (ID!=0)
					m_TaskList.push_back(ID);
			}
		}
	}

	return true;
}


bool CTaskbarOptions::WriteSettings(CSettings &Settings)
{
#if 0	// まだ設定インターフェースが無い
	Settings.Clear();

	Settings.Write(TEXT("ShowTasks"),m_fShowTasks);
	Settings.Write(TEXT("ShowRecentChannels"),m_fShowRecentChannels);
	Settings.Write(TEXT("MaxRecentChannels"),m_MaxRecentChannels);
	Settings.Write(TEXT("ShowChannelIcon"),m_fShowChannelIcon);
	Settings.Write(TEXT("IconDirectory"),m_IconDirectory);

	Settings.Write(TEXT("TaskCount"),(int)m_TaskList.size());
	const CCommandList &CommandList=GetAppClass().CommandList;
	for (int i=0;i<(int)m_TaskList.size();i++) {
		TCHAR szKey[32];
		StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Task%d"),i);
		Settings.Write(szKey,CommandList.GetCommandText(m_TaskList[i]));
	}
#endif

	Settings.Write(TEXT("EnableJumpList"),m_fEnableJumpList);
	Settings.Write(TEXT("JumpListKeepSingleTask"),m_fJumpListKeepSingleTask);
	Settings.Write(TEXT("UseUniqueAppID"),m_fUseUniqueAppID);

	return true;
}


bool CTaskbarOptions::IsJumpListEnabled() const
{
	return m_fEnableJumpList
		&& ((m_fShowTasks && !m_TaskList.empty()) ||
			(m_fShowRecentChannels && m_MaxRecentChannels>0));
}


void CTaskbarOptions::SetEnableJumpList(bool fEnable)
{
	if (m_fEnableJumpList!=fEnable) {
		m_fEnableJumpList=fEnable;
		GetAppClass().TaskbarManager.ReinitializeJumpList();
	}
}
