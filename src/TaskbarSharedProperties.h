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


#ifndef TVTEST_TASKBAR_SHARED_PROPERTIES_H
#define TVTEST_TASKBAR_SHARED_PROPERTIES_H


#include "SharedMemory.h"
#include "ChannelHistory.h"


namespace TVTest
{

	class CTaskbarSharedProperties
	{
	public:
		~CTaskbarSharedProperties();

		bool Open(LPCTSTR pszName, const CRecentChannelList *pRecentChannels);
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

			static constexpr DWORD VERSION_CURRENT = 0;
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
		SharedInfoHeader *m_pHeader = nullptr;
		DWORD m_LockTimeout = 3000;

		static constexpr DWORD MAX_RECENT_CHANNELS = 20;

		bool ValidateHeader(const SharedInfoHeader *pHeader) const;
		void ReadRecentChannelList(
			const SharedInfoHeader *pHeader, CRecentChannelList *pList) const;
		void TunerChannelInfoToRecentChannelInfo(
			const CTunerChannelInfo *pTunerChInfo, RecentChannelInfo *pChannelInfo) const;
	};

}


#endif
