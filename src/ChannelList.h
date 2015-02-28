#ifndef CHANNEL_LIST_H
#define CHANNEL_LIST_H


#include <vector>


#define FIRST_UHF_CHANNEL 13
#define MAX_CHANNEL_NAME 64


class CChannelInfo
{
	int m_Space;				// �`���[�j���O���
	int m_ChannelIndex;			// �`�����l���C���f�b�N�X(BonDriver�ł̔ԍ�)
	int m_ChannelNo;			// �����R���`�����l���ԍ�
	int m_PhysicalChannel;		// �����`�����l���ԍ�
	TVTest::String m_Name;		// �`�����l����
	WORD m_NetworkID;			// �l�b�g���[�NID
	WORD m_TransportStreamID;	// �g�����X�|�[�g�X�g���[��ID
	WORD m_ServiceID;			// �T�[�r�XID
	BYTE m_ServiceType;			// �T�[�r�X�̎��
	bool m_fEnabled;			// �L��

public:
	CChannelInfo();
	CChannelInfo(int Space,int ChannelIndex,int No,LPCTSTR pszName);
	virtual ~CChannelInfo() {}
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
	void Enable(bool fEnable) { m_fEnabled=fEnable; }
	bool IsEnabled() const { return m_fEnabled; }
};

class CTunerChannelInfo : public CChannelInfo
{
public:
	CTunerChannelInfo();
	CTunerChannelInfo(const CChannelInfo &ChannelInfo,LPCTSTR pszTunerName=NULL);
	void SetTunerName(LPCTSTR pszName);
	LPCTSTR GetTunerName() const { return m_TunerName.c_str(); }

protected:
	TVTest::String m_TunerName;
};

class CChannelList
{
public:
	CChannelList();
	CChannelList(const CChannelList &Src);
	~CChannelList();
	CChannelList &operator=(const CChannelList &Src);
	int NumChannels() const { return (int)m_ChannelList.size(); }
	int NumEnableChannels() const;
	bool AddChannel(const CChannelInfo &Info);
	bool AddChannel(CChannelInfo *pInfo);
	bool InsertChannel(int Index,const CChannelInfo &Info);
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
	int FindByIndex(int Space,int ChannelIndex,int ServiceID=-1,bool fEnabledOnly=true) const;
	int FindPhysicalChannel(int Channel) const;
	int FindChannelNo(int No,bool fEnabledOnly=true) const;
	int FindServiceID(WORD ServiceID) const;
	int FindByIDs(WORD NetworkID,WORD TransportStreamID,WORD ServiceID,bool fEnabledOnly=true) const;
	int FindByName(LPCTSTR pszName) const;
	int GetNextChannel(int Index,bool fWrap=false) const;
	int GetPrevChannel(int Index,bool fWrap=false) const;
	int GetMaxChannelNo() const;
	enum SortType {
		SORT_SPACE,
		SORT_CHANNELINDEX,
		SORT_CHANNELNO,
		SORT_PHYSICALCHANNEL,
		SORT_NAME,
		SORT_NETWORKID,
		SORT_SERVICEID,
		SORT_TRAILER
	};
	bool Sort(SortType Type,bool fDescending=false);
	bool HasRemoteControlKeyID() const;
	bool HasMultiService() const;

private:
	std::vector<CChannelInfo*> m_ChannelList;
};

class CTuningSpaceInfo
{
public:
	enum TuningSpaceType {
		SPACE_ERROR=-1,
		SPACE_UNKNOWN=0,
		SPACE_TERRESTRIAL,
		SPACE_BS,
		SPACE_110CS
	};

	CTuningSpaceInfo();
	CTuningSpaceInfo(const CTuningSpaceInfo &Info);
	~CTuningSpaceInfo();
	CTuningSpaceInfo &operator=(const CTuningSpaceInfo &Info);
	bool Create(const CChannelList *pList=NULL,LPCTSTR pszName=NULL);
	CChannelList *GetChannelList() { return m_pChannelList; }
	const CChannelList *GetChannelList() const { return m_pChannelList; }
	CChannelInfo *GetChannelInfo(int Index);
	const CChannelInfo *GetChannelInfo(int Index) const;
	LPCTSTR GetName() const { return m_Name.c_str(); }
	bool SetName(LPCTSTR pszName);
	TuningSpaceType GetType() const { return m_Space; }
	int NumChannels() const;

private:
	CChannelList *m_pChannelList;
	TVTest::String m_Name;
	TuningSpaceType m_Space;
};

class CTuningSpaceList
{
public:
	CTuningSpaceList();
	CTuningSpaceList(const CTuningSpaceList &List);
	~CTuningSpaceList();
	CTuningSpaceList &operator=(const CTuningSpaceList &List);
	int NumSpaces() const { return (int)m_TuningSpaceList.size(); }
	bool IsEmpty() const { return m_TuningSpaceList.empty(); }
	CTuningSpaceInfo *GetTuningSpaceInfo(int Space);
	const CTuningSpaceInfo *GetTuningSpaceInfo(int Space) const;
	CChannelList *GetChannelList(int Space);
	const CChannelList *GetChannelList(int Space) const;
	CChannelList *GetAllChannelList() { return &m_AllChannelList; }
	const CChannelList *GetAllChannelList() const { return &m_AllChannelList; }
	LPCTSTR GetTuningSpaceName(int Space) const;
	CTuningSpaceInfo::TuningSpaceType GetTuningSpaceType(int Space) const;
	bool Create(const CChannelList *pList,int Spaces=0);
	bool Reserve(int Spaces);
	bool MakeAllChannelList();
	void Clear();
	bool SaveToFile(LPCTSTR pszFileName) const;
	bool LoadFromFile(LPCTSTR pszFileName);

private:
	std::vector<CTuningSpaceInfo*> m_TuningSpaceList;
	CChannelList m_AllChannelList;
	bool MakeTuningSpaceList(const CChannelList *pList,int Spaces=0);
};


#endif
