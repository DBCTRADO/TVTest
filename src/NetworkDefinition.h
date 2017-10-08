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

}	// namespace TVTest


#endif
