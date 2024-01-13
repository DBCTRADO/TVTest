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


#ifndef TVTEST_CHANNEL_LIST_H
#define TVTEST_CHANNEL_LIST_H


#include <vector>
#include <memory>


namespace TVTest
{

	constexpr int FIRST_UHF_CHANNEL = 13;
	constexpr std::size_t MAX_CHANNEL_NAME = 64;


	class CChannelInfo
	{
		int m_Space = -1;             // チューニング空間
		int m_ChannelIndex = -1;      // チャンネルインデックス(BonDriverでの番号)
		int m_ChannelNo = 0;          // リモコンチャンネル番号
		int m_PhysicalChannel = 0;    // 物理チャンネル番号
		String m_Name;                // チャンネル名
		WORD m_NetworkID = 0;         // ネットワークID
		WORD m_TransportStreamID = 0; // トランスポートストリームID
		WORD m_ServiceID = 0;         // サービスID
		BYTE m_ServiceType = 0;       // サービスの種類
		bool m_fEnabled = true;       // 有効

	public:
		CChannelInfo() = default;
		CChannelInfo(int Space, int ChannelIndex, int No, LPCTSTR pszName);
		virtual ~CChannelInfo() = default;

		int GetSpace() const { return m_Space; }
		bool SetSpace(int Space);
		int GetChannelIndex() const { return m_ChannelIndex; }
		bool SetChannelIndex(int Channel);
		bool SetChannelNo(int ChannelNo);
		int GetChannelNo() const { return m_ChannelNo; }
		int GetPhysicalChannel() const { return m_PhysicalChannel; }
		bool SetPhysicalChannel(int Channel);
		LPCTSTR GetName() const { return m_Name.c_str(); }
		bool SetName(LPCTSTR pszName);
		void SetNetworkID(WORD NetworkID);
		WORD GetNetworkID() const { return m_NetworkID; }
		void SetTransportStreamID(WORD TransportStreamID);
		WORD GetTransportStreamID() const { return m_TransportStreamID; }
		void SetServiceID(WORD ServiceID);
		WORD GetServiceID() const { return m_ServiceID; }
		void SetServiceType(BYTE ServiceType);
		BYTE GetServiceType() const { return m_ServiceType; }
		void Enable(bool fEnable) { m_fEnabled = fEnable; }
		bool IsEnabled() const { return m_fEnabled; }
	};

	class CTunerChannelInfo
		: public CChannelInfo
	{
	public:
		CTunerChannelInfo() = default;
		CTunerChannelInfo(const CChannelInfo &ChannelInfo, LPCTSTR pszTunerName = nullptr);

		CTunerChannelInfo &operator=(const CChannelInfo &Src);

		void SetTunerName(LPCTSTR pszName);
		LPCTSTR GetTunerName() const { return m_TunerName.c_str(); }

	protected:
		String m_TunerName;
	};

	class CChannelList
	{
	public:
		enum class SortType {
			Space,
			ChannelIndex,
			ChannelNo,
			PhysicalChannel,
			Name,
			NetworkID,
			ServiceID,
			TVTEST_ENUM_CLASS_TRAILER
		};

		CChannelList() = default;
		CChannelList(const CChannelList &Src);

		CChannelList &operator=(const CChannelList &Src);

		int NumChannels() const { return static_cast<int>(m_ChannelList.size()); }
		int NumEnableChannels() const;
		bool AddChannel(const CChannelInfo &Info);
		bool AddChannel(CChannelInfo *pInfo);
		bool InsertChannel(int Index, const CChannelInfo &Info);
		CChannelInfo *GetChannelInfo(int Index);
		const CChannelInfo *GetChannelInfo(int Index) const;
		int GetSpace(int Index) const;
		int GetChannelIndex(int Index) const;
		int GetChannelNo(int Index) const;
		int GetPhysicalChannel(int Index) const;
		LPCTSTR GetName(int Index) const;
		bool IsEnabled(int Index) const;
		bool DeleteChannel(int Index);
		void Clear();
		int Find(const CChannelInfo *pInfo) const;
		int Find(const CChannelInfo &Info, bool fEnabledOnly = true) const;
		int FindByIndex(int Space, int ChannelIndex, int ServiceID = -1, bool fEnabledOnly = true) const;
		int FindPhysicalChannel(int Channel) const;
		int FindChannelNo(int No, bool fEnabledOnly = true) const;
		int FindServiceID(WORD ServiceID) const;
		int FindByIDs(WORD NetworkID, WORD TransportStreamID, WORD ServiceID, bool fEnabledOnly = true) const;
		int FindByName(LPCTSTR pszName) const;
		int GetNextChannel(int Index, bool fWrap = false) const;
		int GetPrevChannel(int Index, bool fWrap = false) const;
		int GetMaxChannelNo() const;
		bool Sort(SortType Type, bool fDescending = false);
		bool HasRemoteControlKeyID() const;
		bool HasMultiService() const;

	private:
		std::vector<std::unique_ptr<CChannelInfo>> m_ChannelList;
	};

	class CTuningSpaceInfo
	{
	public:
		enum class TuningSpaceType {
			Error = -1,
			Unknown = 0,
			Terrestrial,
			BS,
			CS110,
		};

		CTuningSpaceInfo() = default;
		CTuningSpaceInfo(const CTuningSpaceInfo &Info);

		CTuningSpaceInfo &operator=(const CTuningSpaceInfo &Info);

		bool Create(const CChannelList *pList = nullptr, LPCTSTR pszName = nullptr);
		CChannelList *GetChannelList() { return m_ChannelList.get(); }
		const CChannelList *GetChannelList() const { return m_ChannelList.get(); }
		CChannelInfo *GetChannelInfo(int Index);
		const CChannelInfo *GetChannelInfo(int Index) const;
		LPCTSTR GetName() const { return m_Name.c_str(); }
		bool SetName(LPCTSTR pszName);
		TuningSpaceType GetType() const { return m_Space; }
		int NumChannels() const;

	private:
		std::unique_ptr<CChannelList> m_ChannelList;
		String m_Name;
		TuningSpaceType m_Space = TuningSpaceType::Unknown;
	};

	class CTuningSpaceList
	{
	public:
		CTuningSpaceList() = default;
		CTuningSpaceList(const CTuningSpaceList &List);

		CTuningSpaceList &operator=(const CTuningSpaceList &List);

		int NumSpaces() const { return static_cast<int>(m_TuningSpaceList.size()); }
		bool IsEmpty() const { return m_TuningSpaceList.empty(); }
		CTuningSpaceInfo *GetTuningSpaceInfo(int Space);
		const CTuningSpaceInfo *GetTuningSpaceInfo(int Space) const;
		CChannelList *GetChannelList(int Space);
		const CChannelList *GetChannelList(int Space) const;
		CChannelList *GetAllChannelList() { return &m_AllChannelList; }
		const CChannelList *GetAllChannelList() const { return &m_AllChannelList; }
		LPCTSTR GetTuningSpaceName(int Space) const;
		CTuningSpaceInfo::TuningSpaceType GetTuningSpaceType(int Space) const;
		bool Create(const CChannelList *pList, int Spaces = 0);
		bool Reserve(int Spaces);
		bool MakeAllChannelList();
		void Clear();
		bool SaveToFile(LPCTSTR pszFileName) const;
		bool LoadFromFile(LPCTSTR pszFileName);

	private:
		std::vector<std::unique_ptr<CTuningSpaceInfo>> m_TuningSpaceList;
		CChannelList m_AllChannelList;
		bool MakeTuningSpaceList(const CChannelList *pList, int Spaces = 0);
	};

} // namespace TVTest


#endif
