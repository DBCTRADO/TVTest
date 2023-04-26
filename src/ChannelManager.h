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


#ifndef TVTEST_CHANNEL_MANAGER_H
#define TVTEST_CHANNEL_MANAGER_H


#include "ChannelList.h"
#include "LibISDB/LibISDB/Windows/Filters/BonDriverSourceFilter.hpp"


namespace TVTest
{

	class CChannelManager
	{
		int m_CurrentSpace;
		int m_CurrentChannel;
		int m_CurrentServiceID;
		int m_ChangingChannel;

		CTuningSpaceList m_TuningSpaceList;
		CTuningSpaceList m_DriverTuningSpaceList;
		bool m_fUseDriverChannelList;
		bool m_fChannelFileHasStreamIDs;
		String m_ChannelFileName;

	public:
		static constexpr int SPACE_INVALID = -2;
		static constexpr int SPACE_ALL     = -1;

		enum class UpDownOrder {
			Index,
			ID,
		};

		CChannelManager();

		void Reset();
		bool LoadChannelList(LPCTSTR pszFileName);
		bool SetTuningSpaceList(const CTuningSpaceList *pList);
		bool MakeDriverTuningSpaceList(const LibISDB::BonDriverSourceFilter *pSrcDecoder);
		bool SetUseDriverChannelList(bool fUse);
		bool GetUseDriverChannelList() const { return m_fUseDriverChannelList; }
		bool SetCurrentChannel(int Space, int Channel);
		int GetCurrentSpace() const { return m_CurrentSpace; }
		int GetCurrentChannel() const { return m_CurrentChannel; }
		int GetCurrentServiceID() const { return m_CurrentServiceID; }
		bool SetCurrentServiceID(int ServiceID);
		bool SetChangingChannel(int Channel);
		int GetChangingChannel() const { return m_ChangingChannel; }
		const CChannelInfo *GetCurrentChannelInfo() const;
		const CChannelInfo *GetChangingChannelInfo() const;
		int GetNextChannel(int CurChannel, UpDownOrder Order, bool fNext) const;
		int GetNextChannel(UpDownOrder Order, bool fNext) const;
		const CChannelList *GetCurrentChannelList() const;
		const CChannelList *GetChannelList(int Space) const;
		const CChannelList *GetFileChannelList(int Space) const;
		const CChannelList *GetDriverChannelList(int Space) const;
		const CChannelList *GetAllChannelList() const;
		const CChannelList *GetFileAllChannelList() const;
		const CChannelList *GetDriverAllChannelList() const;
		const CTuningSpaceList *GetTuningSpaceList() const { return &m_TuningSpaceList; }
		const CTuningSpaceList *GetDriverTuningSpaceList() const { return &m_DriverTuningSpaceList; }
		LPCTSTR GetTuningSpaceName(int Space) const;
		int FindChannelInfo(const CChannelInfo *pInfo) const;
		int FindChannelByIDs(
			int Space, WORD NetworkID, WORD TransportStreamID, WORD ServiceID,
			bool fEnabledOnly = true) const;
		int NumSpaces() const;
		bool GetChannelFileName(LPTSTR pszFileName, int MaxLength) const;

		bool ChannelFileHasStreamIDs() const { return m_fChannelFileHasStreamIDs; }
	};

	class CChannelSpec
	{
		int m_Space = CChannelManager::SPACE_INVALID;
		int m_Channel = -1;
		int m_ServiceID = -1;

	public:
		bool Store(const CChannelManager *pChannelManager);
		bool SetSpace(int Space);
		int GetSpace() const { return m_Space; }
		bool SetChannel(int Channel);
		int GetChannel() const { return m_Channel; }
		bool SetServiceID(int ServiceID);
		int GetServiceID() const { return m_ServiceID; }
		bool IsValid() const { return m_Space > CChannelManager::SPACE_INVALID && m_Channel >= 0; }
	};

}	// namespace TVTest


#endif
