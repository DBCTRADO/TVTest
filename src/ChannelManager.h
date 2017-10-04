#ifndef CHANNEL_MANAGER_H
#define CHANNEL_MANAGER_H


#include "ChannelList.h"
#include "LibISDB/LibISDB/Windows/Filters/BonDriverSourceFilter.hpp"


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

public:
	enum {
		SPACE_INVALID = -2,
		SPACE_ALL = -1
	};

	enum UpDownOrder {
		UP_DOWN_ORDER_INDEX,
		UP_DOWN_ORDER_ID
	};

	CChannelManager();
	~CChannelManager();
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
	bool IsValid() const { return m_Space > CChannelManager::SPACE_INVALID && m_Channel >= 0; }
};


#endif
