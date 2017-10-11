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
		CTaskbarManager();
		~CTaskbarManager();

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

			ChannelIconInfo() {}
			ChannelIconInfo(const FILETIME &Time) : UpdatedTime(Time) {}
		};

		static DWORD ChannelIconMapKey(WORD NetworkID, WORD ServiceID) {
			return ((DWORD)NetworkID << 16) | ServiceID;
		}

		struct CommandIconInfo
		{
			int Command;
			int Icon;
		};

		static const CommandIconInfo m_CommandIconList[];

		HWND m_hwnd;
		UINT m_TaskbarButtonCreatedMessage;
		interface ITaskbarList3 *m_pTaskbarList;
		String m_AppID;
		bool m_fAppIDInvalid;
		bool m_fJumpListInitialized;
		CTaskbarSharedProperties m_SharedProperties;
		CRecentChannelList m_RecentChannelList;
		bool m_fSaveRecentChannelList;
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
