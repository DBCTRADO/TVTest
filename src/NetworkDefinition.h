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


#ifndef TVTEST_NETWORK_DEFINITION_H
#define TVTEST_NETWORK_DEFINITION_H


#include <vector>
#include "Settings.h"


namespace TVTest
{

	class CNetworkDefinition
	{
	public:
		enum class NetworkType {
			Unknown,
			Terrestrial,
			BS,
			CS,
		};

		struct NetworkInfo
		{
			WORD NetworkID;
			String Name;
			NetworkType Type;

			NetworkInfo();
			NetworkInfo(WORD nid, LPCTSTR name, NetworkType type);
		};

		typedef std::vector<NetworkInfo> NetworkInfoList;

		struct RemoteControlKeyIDAssignInfo
		{
			WORD NetworkID;
			WORD FirstServiceID;
			WORD LastServiceID;
			WORD Subtrahend;
			WORD Divisor;
		};

		CNetworkDefinition();

		bool LoadSettings(CSettings &Settings);
		const NetworkInfo *GetNetworkInfoByID(WORD NetworkID) const;
		NetworkType GetNetworkType(WORD NetworkID) const;
		bool IsTerrestrialNetworkID(WORD NetworkID) const;
		bool IsBSNetworkID(WORD NetworkID) const;
		bool IsCSNetworkID(WORD NetworkID) const;
		bool IsSatelliteNetworkID(WORD NetworkID) const;
		int GetNetworkTypeOrder(WORD NetworkID1, WORD NetworkID2) const;
		int GetRemoteControlKeyID(WORD NetworkID, WORD ServiceID) const;

	private:
		NetworkInfoList::iterator FindNetworkInfoByID(WORD NetworkID);
		NetworkInfoList::const_iterator FindNetworkInfoByID(WORD NetworkID) const;
		static NetworkType GetNetworkTypeFromName(LPCTSTR pszName);

		NetworkInfoList m_NetworkInfoList;
		std::vector<RemoteControlKeyIDAssignInfo> m_KeyIDAssignList;

		static const RemoteControlKeyIDAssignInfo m_DefaultKeyIDAssignList[];
	};

} // namespace TVTest


#endif
