#ifndef EPG_PROGRAM_LIST
#define EPG_PROGRAM_LIST


#if (_MSC_VER >= 1600) || (__cplusplus >= 201103)
#define EPG_EVENT_LIST_USE_UNORDERED_MAP
#endif

#include <vector>
#include <map>
#ifdef EPG_EVENT_LIST_USE_UNORDERED_MAP
#include <unordered_map>
#endif
#include "BonTsEngine/EventManager.h"


class CServiceInfoData
{
public:
	WORD m_NetworkID;
	WORD m_TSID;
	WORD m_ServiceID;

	CServiceInfoData();
	CServiceInfoData(WORD NetworkID,WORD TSID,WORD ServiceID);
	bool operator==(const CServiceInfoData &Info) const;
	bool operator!=(const CServiceInfoData &Info) const { return !(*this==Info); }
};

class CEventInfoData : public CEventInfo
{
public:
	typedef CContentDesc::Nibble NibbleData;

	enum {
		AUDIOCOMPONENT_MONO=1,
		AUDIOCOMPONENT_DUALMONO,
		AUDIOCOMPONENT_STEREO,
		AUDIOCOMPONENT_3_1,
		AUDIOCOMPONENT_3_2,
		AUDIOCOMPONENT_3_2_1
	};

	enum {
		CONTENT_NEWS,
		CONTENT_SPORTS,
		CONTENT_INFORMATION,
		CONTENT_DRAMA,
		CONTENT_MUSIC,
		CONTENT_VARIETY,
		CONTENT_MOVIE,
		CONTENT_ANIME,
		CONTENT_DOCUMENTARY,
		CONTENT_THEATER,
		CONTENT_EDUCATION,
		CONTENT_WELFARE,
		CONTENT_LAST=CONTENT_WELFARE
	};

	bool m_fDatabase;

	CEventInfoData();
	CEventInfoData(CEventInfoData &&Info);
	CEventInfoData(const CEventInfo &Info);
	~CEventInfoData();
	CEventInfoData &operator=(CEventInfoData &&Info);
	CEventInfoData &operator=(const CEventInfo &Info);
#if defined(_MSC_VER) && (_MSC_VER >= 1800)
	CEventInfoData(const CEventInfoData &) = default;
	CEventInfoData &operator=(const CEventInfoData &) = default;
#endif
	bool operator==(const CEventInfoData &Info) const;
	bool operator!=(const CEventInfoData &Info) const { return !(*this==Info); }
	int GetMainAudioIndex() const;
	const AudioInfo *GetMainAudioInfo() const;
};

class CEventInfoList
{
public:
#ifdef EPG_EVENT_LIST_USE_UNORDERED_MAP
	typedef std::unordered_map<WORD,CEventInfoData> EventMap;
#else
	typedef std::map<WORD,CEventInfoData> EventMap;
#endif
	typedef EventMap::iterator EventIterator;
	EventMap EventDataMap; //ÉLÅ[ EventID
	CEventManager::TimeEventMap EventTimeMap;

	CEventInfoList();
	CEventInfoList(const CEventInfoList &List);
	~CEventInfoList();
	CEventInfoList &operator=(const CEventInfoList &List);
	const CEventInfoData *GetEventInfo(WORD EventID);
	bool RemoveEvent(WORD EventID);
};

class CEpgServiceInfo
{
public:
	CServiceInfoData m_ServiceData;
	CEventInfoList m_EventList;
	bool m_fMergeOldEvents;

	CEpgServiceInfo();
	CEpgServiceInfo(const CServiceInfoData &ServiceData);
	~CEpgServiceInfo();
	const CEventInfoData *GetEventInfo(WORD EventID);
};

class CEpgProgramList
{
	CEventManager *m_pEventManager;
	typedef ULONGLONG ServiceMapKey;
	typedef std::map<ServiceMapKey,CEpgServiceInfo*> ServiceMap;
	ServiceMap m_ServiceMap;
	mutable CCriticalLock m_Lock;
	bool m_fUpdated;
	FILETIME m_LastWriteTime;

	static ServiceMapKey GetServiceMapKey(WORD NetworkID,WORD TSID,WORD ServiceID) {
		return ((ULONGLONG)NetworkID<<32) | ((ULONGLONG)TSID<<16) | (ULONGLONG)ServiceID;
	}
	static ServiceMapKey GetServiceMapKey(const CServiceInfoData &ServiceInfo) {
		return GetServiceMapKey(ServiceInfo.m_NetworkID,ServiceInfo.m_TSID,ServiceInfo.m_ServiceID);
	}

	const CEventInfoData *GetEventInfo(WORD NetworkID,WORD TSID,WORD ServiceID,WORD EventID);
	bool SetCommonEventInfo(CEventInfoData *pInfo);
	bool SetEventExtText(CEventInfoData *pInfo);
	bool CopyEventExtText(CEventInfoData *pDstInfo,const CEventInfoData *pSrcInfo);
	bool Merge(CEpgProgramList *pSrcList);

public:
	enum {
		SERVICE_UPDATE_DISCARD_OLD_EVENTS	= 0x0001U,
		SERVICE_UPDATE_DISCARD_ENDED_EVENTS	= 0x0002U,
		SERVICE_UPDATE_DATABASE				= 0x0004U
	};

	CEpgProgramList(CEventManager *pEventManager);
	~CEpgProgramList();
	bool UpdateService(const CEventManager::ServiceInfo *pService,UINT Flags=0);
	bool UpdateService(CEventManager *pEventManager,
					   const CEventManager::ServiceInfo *pService,UINT Flags=0);
	bool UpdateService(WORD NetworkID,WORD TSID,WORD ServiceID,UINT Flags=0);
	bool UpdateServices(WORD NetworkID,WORD TSID,UINT Flags=0);
	bool UpdateProgramList(UINT Flags=0);
	void Clear();
	int NumServices() const;
	CEpgServiceInfo *EnumService(int ServiceIndex);
	CEpgServiceInfo *GetServiceInfo(WORD NetworkID,WORD TSID,WORD ServiceID);
	bool GetEventInfo(WORD NetworkID,WORD TSID,WORD ServiceID,
					  WORD EventID,CEventInfoData *pInfo);
	bool GetEventInfo(WORD NetworkID,WORD TSID,WORD ServiceID,
					  const SYSTEMTIME *pTime,CEventInfoData *pInfo);
	bool GetNextEventInfo(WORD NetworkID,WORD TSID,WORD ServiceID,
						  const SYSTEMTIME *pTime,CEventInfoData *pInfo);
	bool IsUpdated() const { return m_fUpdated; }
	void SetUpdated(bool fUpdated) { m_fUpdated=fUpdated; }
	bool LoadFromFile(LPCTSTR pszFileName);
	bool SaveToFile(LPCTSTR pszFileName);
};


#endif
