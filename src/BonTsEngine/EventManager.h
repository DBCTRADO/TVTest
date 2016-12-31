#pragma once

#if (_MSC_VER >= 1600) || (__cplusplus >= 201103)
#define EVENTMANAGER_USE_UNORDERED_MAP
#endif

#include <map>
#include <set>
#include <vector>
#ifdef EVENTMANAGER_USE_UNORDERED_MAP
#include <unordered_map>
#endif
#include "MediaDecoder.h"
#include "TsTable.h"
#include "EventInfo.h"


/*
	î‘ëgèÓïÒä«óùÉNÉâÉX
*/
class CEventManager
	: public CMediaDecoder
	, public CPsiStreamTable::ISectionHandler
{
public:
	typedef std::vector<CEventInfo> EventList;

	struct ServiceInfo {
		WORD OriginalNetworkID;
		WORD TransportStreamID;
		WORD ServiceID;
	};
	typedef std::vector<ServiceInfo> ServiceList;

	struct TimeEventInfo {
		ULONGLONG StartTime;
		DWORD Duration;
		WORD EventID;
		ULONGLONG UpdateTime;

		TimeEventInfo(ULONGLONG Time);
		TimeEventInfo(const SYSTEMTIME &StartTime);
		TimeEventInfo(const CEventInfo &Info);
		bool operator<(const TimeEventInfo &Obj) const {
			return StartTime < Obj.StartTime;
		}
	};
	typedef std::set<TimeEventInfo> TimeEventMap;

	CEventManager(IEventHandler *pEventHandler = NULL);
	virtual ~CEventManager();

// CMediaDecoder
	virtual void Reset();
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CEventManager
	void Clear();
	int GetServiceNum();
	bool GetServiceList(ServiceList *pList);
	bool IsServiceUpdated(const WORD NetworkID, const WORD TransportStreamID, const WORD ServiceID);
	bool GetEventList(const WORD NetworkID, const WORD TransportStreamID, const WORD ServiceID,
					  EventList *pList, TimeEventMap *pTimeEventMap = NULL);
	bool GetEventInfo(const WORD NetworkID, const WORD TransportStreamID, const WORD ServiceID,
					  const WORD EventID, CEventInfo *pInfo);
	bool GetEventInfo(const WORD NetworkID, const WORD TransportStreamID, const WORD ServiceID,
					  const SYSTEMTIME *pTime, CEventInfo *pInfo);
	bool IsScheduleComplete(const WORD NetworkID, const WORD TransportStreamID, const WORD ServiceID, const bool bExtended = false);
	bool HasSchedule(const WORD NetworkID, const WORD TransportStreamID, const WORD ServiceID, const bool bExtended = false);
	void SetScheduleOnly(const bool bScheduleOnly);
	bool IsScheduleOnly() const { return m_bScheduleOnly; }

	static ULONGLONG SystemTimeToSeconds(const SYSTEMTIME &Time)
	{
		FILETIME ft;
		if (!::SystemTimeToFileTime(&Time, &ft))
			return 0;
		return (((ULONGLONG)ft.dwHighDateTime << 32) | (ULONGLONG)ft.dwLowDateTime) / 10000000ULL;
	}

protected:
	class CScheduleInfo {
	public:
		CScheduleInfo();
		void Clear();
		bool IsComplete(int Hour, bool bExtended) const;
		bool IsTableComplete(int Table, int Hour, bool bExtended) const;
		bool HasSchedule(const bool bExtended) const;
		bool OnSection(const class CEitParserTable *pTable, const CPsiSection *pSection);

	private:
		struct SegmentInfo {
			BYTE SectionNum;
			BYTE Flags;
		};

		struct TableInfo {
			BYTE Version;
			SegmentInfo SegmentList[32];
		};

		struct TableList {
			BYTE TableNum;
			TableInfo Table[8];
		};

		TableList m_Basic;
		TableList m_Extended;
	};

#ifdef EVENTMANAGER_USE_UNORDERED_MAP
	typedef std::unordered_map<WORD, CEventInfo> EventMap;
#else
	typedef std::map<WORD, CEventInfo> EventMap;
#endif

	struct ServiceEventMap {
		ServiceInfo Info;
		EventMap EventMap;
		TimeEventMap TimeMap;
		bool bUpdated;
		CScheduleInfo Schedule;
		SYSTEMTIME ScheduleUpdatedTime;
	};

	typedef ULONGLONG ServiceMapKey;
	typedef std::map<ServiceMapKey, ServiceEventMap> ServiceMap;
	static ServiceMapKey GetServiceMapKey(WORD OriginalNetworkID, WORD TransportStreamID, WORD ServiceID) {
		return ((ServiceMapKey)OriginalNetworkID << 32)
			| ((ServiceMapKey)TransportStreamID << 16)
			| (ServiceMapKey)ServiceID;
	}

	CTsPidMapManager m_PidMapManager;
	ServiceMap m_ServiceMap;
	SYSTEMTIME m_CurTotTime;
	ULONGLONG m_CurTotSeconds;
	bool m_bScheduleOnly;

	static bool RemoveEvent(EventMap *pMap, const WORD EventID);

private:
// CPsiStreamTable::ISectionHandler
	void OnSection(CPsiStreamTable *pTable, const CPsiSection *pSection);

	static void CALLBACK OnTotUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
};
