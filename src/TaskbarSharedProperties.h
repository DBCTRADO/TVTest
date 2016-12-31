#ifndef TVTEST_TASKBAR_SHARED_PROPERTIES_H
#define TVTEST_TASKBAR_SHARED_PROPERTIES_H


#include "SharedMemory.h"
#include "ChannelHistory.h"


namespace TVTest
{

	class CTaskbarSharedProperties
	{
	public:
		CTaskbarSharedProperties();
		~CTaskbarSharedProperties();
		bool Open(LPCTSTR pszName,const CRecentChannelList *pRecentChannels);
		void Close();
		bool IsOpened() const;
		bool GetRecentChannelList(CRecentChannelList *pList);
		bool AddRecentChannel(const CTunerChannelInfo &Info);
		bool ClearRecentChannelList();

	private:
		struct SharedInfoHeader
		{
			DWORD Size;
			DWORD Version;
			DWORD MaxRecentChannels;
			DWORD RecentChannelCount;

			static const DWORD VERSION_CURRENT=0;
		};

		struct RecentChannelInfo
		{
			int Space;
			int ChannelIndex;
			int ChannelNo;
			int PhysicalChannel;
			WORD NetworkID;
			WORD TransportStreamID;
			WORD ServiceID;
			BYTE ServiceType;
			BYTE Reserved;
			WCHAR szChannelName[MAX_CHANNEL_NAME];
			WCHAR szTunerName[MAX_PATH];
		};

		CSharedMemory m_SharedMemory;
		SharedInfoHeader *m_pHeader;
		DWORD m_LockTimeout;

		static const DWORD MAX_RECENT_CHANNELS=20;

		bool ValidateHeader(const SharedInfoHeader *pHeader) const;
		void ReadRecentChannelList(
			const SharedInfoHeader *pHeader,CRecentChannelList *pList) const;
		void TunerChannelInfoToRecentChannelInfo(
			const CTunerChannelInfo *pTunerChInfo,RecentChannelInfo *pChannelInfo) const;
	};

}


#endif
