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
	bool IsBS() const;
	bool IsCS() const;
};

class CEventInfoData
{
public:
	typedef CEventManager::CEventInfo::VideoInfo VideoInfo;
	typedef CEventManager::CEventInfo::AudioInfo AudioInfo;
	typedef CEventManager::CEventInfo::AudioList AudioList;
	typedef CContentDesc::Nibble NibbleData;
	typedef CEventManager::CEventInfo::ContentNibble ContentNibble;
	typedef CEventManager::CEventInfo::EventGroupInfo EventGroupInfo;
	typedef CEventManager::CEventInfo::EventGroupList EventGroupList;
	typedef CEventManager::CEventInfo::CommonEventInfo CommonEventInfo;

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

	enum {
		CA_TYPE_UNKNOWN,
		CA_TYPE_FREE,
		CA_TYPE_CHARGEABLE
	};

	enum {
		TYPE_BASIC     = 0x0001U,
		TYPE_EXTENDED  = 0x0002U,
		TYPE_PRESENT   = 0x0004U,
		TYPE_FOLLOWING = 0x0008U
	};

	WORD m_NetworkID;
	WORD m_TSID;
	WORD m_ServiceID;
	WORD m_EventID;
	bool m_fValidStartTime;
	SYSTEMTIME m_stStartTime;
	DWORD m_DurationSec;
	BYTE m_RunningStatus;
	//bool m_fFreeCaMode;
	BYTE m_CaType;
	VideoInfo m_VideoInfo;
	AudioList m_AudioList;
	ContentNibble m_ContentNibble;
	EventGroupList m_EventGroupList;
	bool m_fCommonEvent;
	CommonEventInfo m_CommonEventInfo;
	ULONGLONG m_UpdateTime;
	unsigned int m_Type;
	bool m_fDatabase;

	CEventInfoData();
	CEventInfoData(const CEventInfoData &Info);
#ifdef MOVE_SEMANTICS_SUPPORTED
	CEventInfoData(CEventInfoData &&Info);
#endif
	CEventInfoData(const CEventManager::CEventInfo &Info);
	~CEventInfoData();
	CEventInfoData &operator=(const CEventInfoData &Info);
#ifdef MOVE_SEMANTICS_SUPPORTED
	CEventInfoData &operator=(CEventInfoData &&Info);
#endif
	CEventInfoData &operator=(const CEventManager::CEventInfo &Info);
	bool operator==(const CEventInfoData &Info) const;
	bool operator!=(const CEventInfoData &Info) const { return !(*this==Info); }
	LPCWSTR GetEventName() const { return m_EventName.Get(); }
	bool SetEventName(LPCWSTR pszEventName);
	LPCWSTR GetEventText() const { return m_EventText.Get(); }
	bool SetEventText(LPCWSTR pszEventText);
	LPCWSTR GetEventExtText() const { return m_EventExtText.Get(); }
	bool SetEventExtText(LPCWSTR pszEventExtText);
	bool GetStartTime(SYSTEMTIME *pTime) const;
	bool GetEndTime(SYSTEMTIME *pTime) const;
	int GetMainAudioIndex() const;
	const AudioInfo *GetMainAudioInfo() const;
	bool HasExtended() const { return (m_Type&TYPE_EXTENDED)!=0; }
	bool IsPresent() const { return (m_Type&TYPE_PRESENT)!=0; }
	bool IsFollowing() const { return (m_Type&TYPE_FOLLOWING)!=0; }
	bool IsPresentFollowing() const { return (m_Type&(TYPE_PRESENT | TYPE_FOLLOWING))!=0; }

private:
	CDynamicString m_EventName;
	CDynamicString m_EventText;
	CDynamicString m_EventExtText;

	friend class CEpgProgramList;
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
