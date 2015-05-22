#ifndef TVTEST_TASKBAR_OPTIONS_H
#define TVTEST_TASKBAR_OPTIONS_H


#include <vector>
#include "Options.h"


class CTaskbarOptions : public COptions
{
public:
	typedef std::vector<int> TaskList;

	CTaskbarOptions();
	~CTaskbarOptions();

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
	const TVTest::String &GetIconDirectory() const { return m_IconDirectory; }
	void SetJumpListKeepSingleTask(bool fSingleTask) { m_fJumpListKeepSingleTask=fSingleTask; }
	bool GetJumpListKeepSingleTask() const { return m_fJumpListKeepSingleTask; }
	void SetUseUniqueAppID(bool fUnique) { m_fUseUniqueAppID=fUnique; }
	bool GetUseUniqueAppID() const { return m_fUseUniqueAppID; }
	const TVTest::String &GetAppID() const { return m_AppID; }

private:
	bool m_fEnableJumpList;
	bool m_fShowTasks;
	TaskList m_TaskList;
	bool m_fShowRecentChannels;
	int m_MaxRecentChannels;
	bool m_fShowChannelIcon;
	TVTest::String m_IconDirectory;
	bool m_fJumpListKeepSingleTask;
	bool m_fUseUniqueAppID;
	TVTest::String m_AppID;

	static const int m_DefaultTaskList[];
};


#endif
