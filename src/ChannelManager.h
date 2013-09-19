#ifndef CHANNEL_MANAGER_H
#define CHANNEL_MANAGER_H


#include "ChannelList.h"
#include "BonSrcDecoder.h"


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
	TVTest::String m_ChannelFileName;

#ifdef NETWORK_REMOCON_SUPPORT
	bool m_fNetworkRemocon;
	CChannelList *m_pNetworkRemoconChannelList;
	int m_NetworkRemoconCurrentChannel;
#endif

public:
	enum {
		SPACE_INVALID=-2,
		SPACE_ALL=-1
	};

	CChannelManager();
	~CChannelManager();
	void Reset();
	bool LoadChannelList(LPCTSTR pszFileName);
	bool SetTuningSpaceList(const CTuningSpaceList *pList);
	bool MakeDriverTuningSpaceList(const CBonSrcDecoder *pSrcDecoder);
	bool SetUseDriverChannelList(bool fUse);
	bool GetUseDriverChannelList() const { return m_fUseDriverChannelList; }
	bool SetCurrentChannel(int Space,int Channel);
	int GetCurrentSpace() const { return m_CurrentSpace; }
	int GetCurrentChannel() const { return m_CurrentChannel; }
	int GetCurrentServiceID() const { return m_CurrentServiceID; }
	bool SetCurrentServiceID(int ServiceID);
	bool SetChangingChannel(int Channel);
	const CChannelInfo *GetCurrentChannelInfo() const;
	const CChannelInfo *GetCurrentRealChannelInfo() const;
	const CChannelInfo *GetChangingChannelInfo() const;
	const CChannelInfo *GetNextChannelInfo(bool fNext) const;
	const CChannelList *GetCurrentChannelList() const;
	const CChannelList *GetCurrentRealChannelList() const;
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
	int FindChannelByIDs(int Space,WORD NetworkID,WORD TransportStreamID,WORD ServiceID) const;
	int NumSpaces() const;
	bool GetChannelFileName(LPTSTR pszFileName,int MaxLength) const;

#ifdef NETWORK_REMOCON_SUPPORT
	bool SetNetworkRemoconMode(bool fNetworkRemocon,CChannelList *pList=NULL);
	bool IsNetworkRemoconMode() const { return m_fNetworkRemocon; }
	int GetNetworkRemoconCurrentChannel() const { return m_NetworkRemoconCurrentChannel; }
	bool SetNetworkRemoconCurrentChannel(int Channel);
#endif

	bool ChannelFileHasStreamIDs() const { return m_fChannelFileHasStreamIDs; }
	bool UpdateStreamInfo(int Space,int ChannelIndex,
						  WORD NetworkID,WORD TransportStreamID,WORD ServiceID);
	bool LoadChannelSettings(LPCTSTR pszFileName,LPCTSTR pszDriverName);
	bool SaveChannelSettings(LPCTSTR pszFileName,LPCTSTR pszDriverName);
};

class CChannelSpec
{
	int m_Space;
	int m_Channel;
	int m_ServiceID;

public:
	CChannelSpec();
	~CChannelSpec();
	bool Store(const CChannelManager *pChannelManager);
	bool SetSpace(int Space);
	int GetSpace() const { return m_Space; }
	bool SetChannel(int Channel);
	int GetChannel() const { return m_Channel; }
	bool SetServiceID(int ServiceID);
	int GetServiceID() const { return m_ServiceID; }
	bool IsValid() const { return m_Space>=0 && m_Channel>=0; }
};


#endif
