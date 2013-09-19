#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "EpgProgramList.h"
#include "HelperClass/NFile.h"
#ifdef MOVE_SEMANTICS_SUPPORTED
#include <utility>
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


static bool CompareText(LPCTSTR pszText1,LPCTSTR pszText2)
{
	return (pszText1==NULL && pszText2==NULL)
			|| (pszText1!=NULL && pszText2!=NULL
				&& ::lstrcmp(pszText1,pszText2)==0);
}




CServiceInfoData::CServiceInfoData()
	: m_NetworkID(0)
	, m_TSID(0)
	, m_ServiceID(0)
{
}


CServiceInfoData::CServiceInfoData(WORD NetworkID,WORD TSID,WORD ServiceID)
	: m_NetworkID(NetworkID)
	, m_TSID(TSID)
	, m_ServiceID(ServiceID)
{
}


bool CServiceInfoData::operator==(const CServiceInfoData &Info) const
{
	return m_NetworkID==Info.m_NetworkID
		&& m_TSID==Info.m_TSID
		&& m_ServiceID==Info.m_ServiceID;
}


bool CServiceInfoData::IsBS() const
{
	return IsBSNetworkID(m_NetworkID);
}


bool CServiceInfoData::IsCS() const
{
	return IsCSNetworkID(m_NetworkID);
}




CEventInfoData::CEventInfoData()
	: m_fValidStartTime(false)
	, m_fCommonEvent(false)
	, m_UpdateTime(0)
	, m_Type(0)
	, m_fDatabase(false)
{
}


CEventInfoData::CEventInfoData(const CEventInfoData &Info)
{
	*this=Info;
}


#ifdef MOVE_SEMANTICS_SUPPORTED
CEventInfoData::CEventInfoData(CEventInfoData &&Info)
{
	*this=std::move(Info);
}
#endif


CEventInfoData::CEventInfoData(const CEventManager::CEventInfo &Info)
{
	*this=Info;
}


CEventInfoData::~CEventInfoData()
{
}


CEventInfoData &CEventInfoData::operator=(const CEventInfoData &Info)
{
	if (&Info!=this) {
		m_NetworkID=Info.m_NetworkID;
		m_TSID=Info.m_TSID;
		m_ServiceID=Info.m_ServiceID;
		m_EventID=Info.m_EventID;
		m_EventName=Info.m_EventName;
		m_EventText=Info.m_EventText;
		m_EventExtText=Info.m_EventExtText;
		m_fValidStartTime=Info.m_fValidStartTime;
		if (m_fValidStartTime)
			m_stStartTime=Info.m_stStartTime;
		m_DurationSec=Info.m_DurationSec;
		m_RunningStatus=Info.m_RunningStatus;
		m_CaType=Info.m_CaType;
		m_VideoInfo=Info.m_VideoInfo;
		m_AudioList=Info.m_AudioList;
		m_ContentNibble=Info.m_ContentNibble;
		m_EventGroupList=Info.m_EventGroupList;
		m_fCommonEvent=Info.m_fCommonEvent;
		m_CommonEventInfo=Info.m_CommonEventInfo;
		m_UpdateTime=Info.m_UpdateTime;
		m_fDatabase=Info.m_fDatabase;
	}
	return *this;
}


#ifdef MOVE_SEMANTICS_SUPPORTED
CEventInfoData &CEventInfoData::operator=(CEventInfoData &&Info)
{
	if (&Info!=this) {
		m_NetworkID=Info.m_NetworkID;
		m_TSID=Info.m_TSID;
		m_ServiceID=Info.m_ServiceID;
		m_EventID=Info.m_EventID;
		m_EventName=std::move(Info.m_EventName);
		m_EventText=std::move(Info.m_EventText);
		m_EventExtText=std::move(Info.m_EventExtText);
		m_fValidStartTime=Info.m_fValidStartTime;
		if (m_fValidStartTime)
			m_stStartTime=Info.m_stStartTime;
		m_DurationSec=Info.m_DurationSec;
		m_RunningStatus=Info.m_RunningStatus;
		m_CaType=Info.m_CaType;
		m_VideoInfo=Info.m_VideoInfo;
		m_AudioList=std::move(Info.m_AudioList);
		m_ContentNibble=Info.m_ContentNibble;
		m_EventGroupList=std::move(Info.m_EventGroupList);
		m_fCommonEvent=Info.m_fCommonEvent;
		m_CommonEventInfo=Info.m_CommonEventInfo;
		m_UpdateTime=Info.m_UpdateTime;
		m_fDatabase=Info.m_fDatabase;
	}
	return *this;
}
#endif


CEventInfoData &CEventInfoData::operator=(const CEventManager::CEventInfo &Info)
{
	m_EventID=Info.GetEventID();
	SetEventName(Info.GetEventName());
	SetEventText(Info.GetEventText());
	SetEventExtText(Info.GetEventExtendedText());
	m_fValidStartTime=true;
	m_stStartTime=Info.GetStartTime();
	m_stStartTime.wDayOfWeek=CalcDayOfWeek(m_stStartTime.wYear,
										   m_stStartTime.wMonth,
										   m_stStartTime.wDay);
	m_DurationSec=Info.GetDuration();
	m_RunningStatus=Info.GetRunningStatus();
	m_CaType=Info.GetFreeCaMode()?CA_TYPE_CHARGEABLE:CA_TYPE_FREE;
	m_VideoInfo=Info.GetVideoInfo();
	m_AudioList=Info.GetAudioList();
	m_ContentNibble=Info.GetContentNibble();
	m_EventGroupList=Info.GetEventGroupList();
	m_fCommonEvent=Info.IsCommonEvent();
	if (m_fCommonEvent)
		m_CommonEventInfo=Info.GetCommonEvent();
	m_UpdateTime=Info.GetUpdateTime();
	m_Type=Info.GetType();

	return *this;
}


bool CEventInfoData::operator==(const CEventInfoData &Info) const
{
	return m_NetworkID==Info.m_NetworkID
		&& m_TSID==Info.m_TSID
		&& m_ServiceID==Info.m_ServiceID
		&& m_EventID==Info.m_EventID
		&& m_EventName==Info.m_EventName
		&& m_EventText==Info.m_EventText
		&& m_EventExtText==Info.m_EventExtText
		&& m_fValidStartTime==Info.m_fValidStartTime
		&& (!m_fValidStartTime
			|| ::memcmp(&m_stStartTime,&Info.m_stStartTime,sizeof(SYSTEMTIME))==0)
		&& m_DurationSec==Info.m_DurationSec
		&& m_RunningStatus==Info.m_RunningStatus
		&& m_CaType==Info.m_CaType
		&& m_VideoInfo==Info.m_VideoInfo
		&& m_AudioList==Info.m_AudioList
		&& m_ContentNibble==Info.m_ContentNibble
		&& m_EventGroupList==Info.m_EventGroupList
		&& m_fCommonEvent==Info.m_fCommonEvent
		&& (!m_fCommonEvent
			|| m_CommonEventInfo==Info.m_CommonEventInfo);
}


bool CEventInfoData::SetEventName(LPCWSTR pszEventName)
{
	return m_EventName.Set(pszEventName);
}


bool CEventInfoData::SetEventText(LPCWSTR pszEventText)
{
	return m_EventText.Set(pszEventText);
}


bool CEventInfoData::SetEventExtText(LPCWSTR pszEventExtText)
{
	return m_EventExtText.Set(pszEventExtText);
}


bool CEventInfoData::GetStartTime(SYSTEMTIME *pTime) const
{
	if (m_fValidStartTime) {
		*pTime=m_stStartTime;
		return true;
	}
	::ZeroMemory(pTime,sizeof(SYSTEMTIME));
	return false;
}


bool CEventInfoData::GetEndTime(SYSTEMTIME *pTime) const
{
	if (m_fValidStartTime) {
		*pTime=m_stStartTime;
		if (OffsetSystemTime(pTime,(LONGLONG)m_DurationSec*1000))
			return true;
	}
	::ZeroMemory(pTime,sizeof(SYSTEMTIME));
	return false;
}


int CEventInfoData::GetMainAudioIndex() const
{
	for (size_t i=0;i<m_AudioList.size();i++) {
		if (m_AudioList[i].bMainComponentFlag)
			return (int)i;
	}
	return -1;
}


const CEventInfoData::AudioInfo *CEventInfoData::GetMainAudioInfo() const
{
	if (m_AudioList.size()==0)
		return NULL;

	const int MainAudioIndex=GetMainAudioIndex();
	if (MainAudioIndex>=0)
		return &m_AudioList[MainAudioIndex];

	return &m_AudioList[0];
}




CEventInfoList::CEventInfoList()
{
}


CEventInfoList::CEventInfoList(const CEventInfoList &List)
	: EventDataMap(List.EventDataMap)
{
}


CEventInfoList::~CEventInfoList()
{
}


CEventInfoList &CEventInfoList::operator=(const CEventInfoList &List)
{
	EventDataMap=List.EventDataMap;
	return *this;
}


const CEventInfoData *CEventInfoList::GetEventInfo(WORD EventID)
{
	EventMap::iterator itrEvent=EventDataMap.find(EventID);

	if (itrEvent==EventDataMap.end())
		return NULL;
	return &itrEvent->second;
}


bool CEventInfoList::RemoveEvent(WORD EventID)
{
	EventMap::iterator itrEvent=EventDataMap.find(EventID);

	if (itrEvent==EventDataMap.end())
		return false;
	EventDataMap.erase(itrEvent);
	return true;
}




CEpgServiceInfo::CEpgServiceInfo()
	: m_fMergeOldEvents(true)
{
}


CEpgServiceInfo::CEpgServiceInfo(const CServiceInfoData &ServiceData)
	: m_ServiceData(ServiceData)
	, m_fMergeOldEvents(true)
{
}


CEpgServiceInfo::~CEpgServiceInfo()
{
}


const CEventInfoData *CEpgServiceInfo::GetEventInfo(WORD EventID)
{
	return m_EventList.GetEventInfo(EventID);
}




CEpgProgramList::CEpgProgramList(CEventManager *pEventManager)
	: m_pEventManager(pEventManager)
	, m_fUpdated(false)
{
	::ZeroMemory(&m_LastWriteTime,sizeof(FILETIME));
}


CEpgProgramList::~CEpgProgramList()
{
	Clear();
}


bool CEpgProgramList::UpdateService(const CEventManager::ServiceInfo *pService,UINT Flags)
{
	return UpdateService(m_pEventManager,pService,Flags);
}


bool CEpgProgramList::UpdateService(CEventManager *pEventManager,
									const CEventManager::ServiceInfo *pService,
									UINT Flags)
{
	CBlockLock Lock(&m_Lock);

	if (!pEventManager->IsServiceUpdated(pService->OriginalNetworkID,
										 pService->TransportStreamID,
										 pService->ServiceID))
		return false;

	CEventManager::EventList EventList;
	if (!pEventManager->GetEventList(pService->OriginalNetworkID,
									 pService->TransportStreamID,
									 pService->ServiceID,
									 &EventList)
			|| EventList.empty())
		return false;

	CServiceInfoData ServiceData(pService->OriginalNetworkID,
								 pService->TransportStreamID,
								 pService->ServiceID);
	CEpgServiceInfo *pServiceInfo=new CEpgServiceInfo(ServiceData);
#ifdef EVENTMANAGER_USE_UNORDERED_MAP
	pServiceInfo->m_EventList.EventDataMap.rehash(max(EventList.size(),300));
#endif

	struct EventTime {
		ULONGLONG StartTime;
		DWORD Duration;
		WORD EventID;
		ULONGLONG UpdateTime;
		EventTime(const CEventInfoData &Info)
			: StartTime(CEventManager::SystemTimeToSeconds(Info.m_stStartTime))
			, Duration(Info.m_DurationSec)
			, EventID(Info.m_EventID)
			, UpdateTime(Info.m_UpdateTime)
		{
		}
		bool operator<(const EventTime &Obj) const {
			return StartTime < Obj.StartTime;
		}
	};
	std::set<EventTime> EventTimeTable;

#ifdef _DEBUG
	SYSTEMTIME stOldestTime,stNewestTime;
	::FillMemory(&stOldestTime,sizeof(SYSTEMTIME),0xFF);
	::FillMemory(&stNewestTime,sizeof(SYSTEMTIME),0x00);
#endif

	const bool fDatabase=(Flags & SERVICE_UPDATE_DATABASE)!=0;

	for (CEventManager::EventList::const_iterator itrEvent=EventList.begin();itrEvent!=EventList.end();++itrEvent) {
		CEventInfoData &EventData=
			pServiceInfo->m_EventList.EventDataMap.insert(
				std::pair<WORD,CEventInfoData>(itrEvent->GetEventID(),CEventInfoData())).first->second;

		EventData=*itrEvent;
		EventData.m_NetworkID=ServiceData.m_NetworkID;
		EventData.m_TSID=ServiceData.m_TSID;
		EventData.m_ServiceID=ServiceData.m_ServiceID;
		EventData.m_fDatabase=fDatabase;

#ifdef _DEBUG
		if (CompareSystemTime(&EventData.m_stStartTime,&stOldestTime)<0)
			stOldestTime=EventData.m_stStartTime;
		SYSTEMTIME stEnd;
		EventData.GetEndTime(&stEnd);
		if (CompareSystemTime(&stEnd,&stNewestTime)>0)
			stNewestTime=EventData.m_stStartTime;
#endif

		EventTimeTable.insert(EventTime(EventData));
	}

#ifdef _DEBUG
	TRACE(TEXT("CEpgProgramList::UpdateService() [%d] %d/%d %d:%02d - %d/%d %d:%02d %u Events\n"),
		  pService->ServiceID,
		  stOldestTime.wMonth,stOldestTime.wDay,stOldestTime.wHour,stOldestTime.wMinute,
		  stNewestTime.wMonth,stNewestTime.wDay,stNewestTime.wHour,stNewestTime.wMinute,
		  (unsigned int)pServiceInfo->m_EventList.EventDataMap.size());
#endif

	const ServiceMapKey Key=GetServiceMapKey(ServiceData);
	std::pair<ServiceMap::iterator,bool> ServiceInsertResult=
		m_ServiceMap.insert(std::pair<ServiceMapKey,CEpgServiceInfo*>(Key,pServiceInfo));

	if (!ServiceInsertResult.second) {
		CEpgServiceInfo *pOldServiceInfo=ServiceInsertResult.first->second;

		if (pOldServiceInfo->m_fMergeOldEvents
				&& (Flags&SERVICE_UPDATE_DISCARD_OLD_EVENTS)==0) {
			// 既存のイベントで新しいリストに無いものを追加する

			const bool fDiscardEndedEvents=(Flags&SERVICE_UPDATE_DISCARD_ENDED_EVENTS)!=0;
			ULONGLONG CurTime;
			if (fDiscardEndedEvents) {
				SYSTEMTIME st;
				GetCurrentJST(&st);
				CurTime=CEventManager::SystemTimeToSeconds(st);
			}

			bool fMergeOldEvents=false;
#ifdef _DEBUG
			unsigned int MergeEventCount=0;
			unsigned int MergeExtTextCount=0;
#endif

			for (CEventInfoList::EventMap::iterator itrEvent=pOldServiceInfo->m_EventList.EventDataMap.begin();
					itrEvent!=pOldServiceInfo->m_EventList.EventDataMap.end();
					++itrEvent) {
				if (!itrEvent->second.m_fDatabase)
					continue;

				CEventInfoList::EventMap::iterator itrEventNew=
					pServiceInfo->m_EventList.EventDataMap.find(itrEvent->second.m_EventID);
				if (itrEventNew!=pServiceInfo->m_EventList.EventDataMap.end()) {
					if (!itrEventNew->second.HasExtended()
							&& CompareSystemTime(&itrEventNew->second.m_stStartTime,
												 &itrEvent->second.m_stStartTime)==0
							&& CopyEventExtText(&itrEventNew->second,&itrEvent->second)) {
						itrEventNew->second.m_fDatabase=true;
						fMergeOldEvents=true;
#ifdef _DEBUG
						MergeExtTextCount++;
#endif
					}
					continue;
				}

				EventTime Time(itrEvent->second);

				if (fDiscardEndedEvents
						&& Time.StartTime+Time.Duration<=CurTime)
					continue;

				std::set<EventTime>::iterator itrUpper=EventTimeTable.upper_bound(Time);
				bool fSkip=false;
				if (itrUpper!=EventTimeTable.begin()) {
					std::set<EventTime>::iterator itr=itrUpper;
					--itr;
					if (itr->StartTime==Time.StartTime) {
						fSkip=true;
					} else {
						while (itr->StartTime+itr->Duration>Time.StartTime) {
							if (itr->UpdateTime>=Time.UpdateTime) {
								fSkip=true;
								break;
							}
							pServiceInfo->m_EventList.RemoveEvent(itr->EventID);
							if (itr==EventTimeTable.begin()) {
								EventTimeTable.erase(itr);
								break;
							}
							EventTimeTable.erase(itr--);
						}
					}
				}
				if (!fSkip) {
					for (std::set<EventTime>::iterator itr=itrUpper;itr!=EventTimeTable.end();) {
						if (itr->StartTime>=Time.StartTime+Time.Duration)
							break;
						if (itr->UpdateTime>=Time.UpdateTime) {
							fSkip=true;
							break;
						}
						pServiceInfo->m_EventList.RemoveEvent(itr->EventID);
						EventTimeTable.erase(itr++);
					}
				}
				if (!fSkip) {
					pServiceInfo->m_EventList.EventDataMap.insert(
						std::pair<WORD,CEventInfoData>(itrEvent->second.m_EventID,itrEvent->second));
					fMergeOldEvents=true;
#ifdef _DEBUG
					MergeEventCount++;
#endif
				}
			}

			if (!fMergeOldEvents && !fDatabase)
				pServiceInfo->m_fMergeOldEvents=false;

#ifdef _DEBUG
			TRACE(TEXT("古いイベント %u / 拡張テキスト %u\n"),
				  MergeEventCount,MergeExtTextCount);
#endif
		}

		delete pOldServiceInfo;
		m_ServiceMap[Key]=pServiceInfo;
	} else {
		pServiceInfo->m_fMergeOldEvents=fDatabase;
	}

	m_fUpdated=true;

	return true;
}


bool CEpgProgramList::UpdateService(WORD NetworkID,WORD TSID,WORD ServiceID,UINT Flags)
{
	CBlockLock Lock(&m_Lock);

	CEventManager::ServiceList ServiceList;
	if (!m_pEventManager->GetServiceList(&ServiceList))
		return false;
	for (size_t i=0;i<ServiceList.size();i++) {
		if ((TSID==0 || ServiceList[i].TransportStreamID==TSID)
				&& ServiceList[i].ServiceID==ServiceID)
			return UpdateService(&ServiceList[i],Flags);
	}
	return false;
}


bool CEpgProgramList::UpdateServices(WORD NetworkID,WORD TSID,UINT Flags)
{
	if (NetworkID==0 && TSID==0)
		return false;

	CBlockLock Lock(&m_Lock);

	CEventManager::ServiceList ServiceList;
	if (!m_pEventManager->GetServiceList(&ServiceList))
		return false;
	bool fUpdated=false;
	for (size_t i=0;i<ServiceList.size();i++) {
		if ((NetworkID==0 || ServiceList[i].OriginalNetworkID==NetworkID)
				&& (TSID==0 || ServiceList[i].TransportStreamID==TSID)) {
			if (UpdateService(&ServiceList[i],Flags))
				fUpdated=true;
		}
	}
	return fUpdated;
}


bool CEpgProgramList::UpdateProgramList(UINT Flags)
{
	CBlockLock Lock(&m_Lock);

	CEventManager::ServiceList ServiceList;
	if (!m_pEventManager->GetServiceList(&ServiceList))
		return false;
	bool fUpdated=false;
	for (size_t i=0;i<ServiceList.size();i++) {
		if (UpdateService(&ServiceList[i],Flags))
			fUpdated=true;
	}
	return fUpdated;
}


void CEpgProgramList::Clear()
{
	CBlockLock Lock(&m_Lock);

	for (ServiceMap::iterator itr=m_ServiceMap.begin();itr!=m_ServiceMap.end();++itr)
		delete itr->second;
	m_ServiceMap.clear();
}


int CEpgProgramList::NumServices() const
{
	CBlockLock Lock(&m_Lock);

	return (int)m_ServiceMap.size();
}


CEpgServiceInfo *CEpgProgramList::EnumService(int ServiceIndex)
{
	CBlockLock Lock(&m_Lock);

	if (ServiceIndex<0)
		return NULL;
	ServiceMap::iterator itrService=m_ServiceMap.begin();
	for (int i=0;i<ServiceIndex && itrService!=m_ServiceMap.end();i++)
		++itrService;
	if (itrService==m_ServiceMap.end())
		return NULL;
	return itrService->second;
}


CEpgServiceInfo *CEpgProgramList::GetServiceInfo(WORD NetworkID,WORD TSID,WORD ServiceID)
{
	CBlockLock Lock(&m_Lock);
	ServiceMap::iterator itrService;

	if (NetworkID!=0 && TSID!=0) {
		itrService=m_ServiceMap.find(GetServiceMapKey(NetworkID,TSID,ServiceID));
		if (itrService!=m_ServiceMap.end())
			return itrService->second;
	} else {
		for (itrService=m_ServiceMap.begin();itrService!=m_ServiceMap.end();++itrService) {
			if ((NetworkID==0 || itrService->second->m_ServiceData.m_NetworkID==NetworkID)
					&& (TSID==0 || itrService->second->m_ServiceData.m_TSID==TSID)
					&& itrService->second->m_ServiceData.m_ServiceID==ServiceID)
				return itrService->second;
		}
	}
	return NULL;
}


bool CEpgProgramList::GetEventInfo(WORD NetworkID,WORD TSID,WORD ServiceID,
								   WORD EventID,CEventInfoData *pInfo)
{
	CBlockLock Lock(&m_Lock);

	CEventManager::CEventInfo EventInfo;
	if (m_pEventManager->GetEventInfo(NetworkID,TSID,ServiceID,
									  EventID,&EventInfo)) {
		*pInfo=EventInfo;
		pInfo->m_NetworkID=NetworkID;
		pInfo->m_TSID=TSID;
		pInfo->m_ServiceID=ServiceID;
		SetCommonEventInfo(pInfo);
		SetEventExtText(pInfo);
	} else {
		const CEventInfoData *pEventInfo=GetEventInfo(NetworkID,TSID,ServiceID,EventID);
		if (pEventInfo==NULL)
			return false;
		*pInfo=*pEventInfo;
		SetCommonEventInfo(pInfo);
	}

	return true;
}


bool CEpgProgramList::GetEventInfo(WORD NetworkID,WORD TSID,WORD ServiceID,
								   const SYSTEMTIME *pTime,CEventInfoData *pInfo)
{
	CBlockLock Lock(&m_Lock);

	CEventManager::CEventInfo EventInfo;
	if (m_pEventManager->GetEventInfo(NetworkID,TSID,ServiceID,
									  pTime,&EventInfo)) {
		*pInfo=EventInfo;
		pInfo->m_NetworkID=NetworkID;
		pInfo->m_TSID=TSID;
		pInfo->m_ServiceID=ServiceID;
		SetCommonEventInfo(pInfo);
		SetEventExtText(pInfo);
		return true;
	}

	CEpgServiceInfo *pServiceInfo=GetServiceInfo(NetworkID,TSID,ServiceID);
	if (pServiceInfo==NULL)
		return NULL;

	CEventInfoList::EventMap::iterator itrEvent;
	for (itrEvent=pServiceInfo->m_EventList.EventDataMap.begin();
			itrEvent!=pServiceInfo->m_EventList.EventDataMap.end();++itrEvent) {
		SYSTEMTIME stStart,stEnd;

		itrEvent->second.GetStartTime(&stStart);
		itrEvent->second.GetEndTime(&stEnd);
		if (CompareSystemTime(&stStart,pTime)<=0
				&& CompareSystemTime(&stEnd,pTime)>0) {
			*pInfo=itrEvent->second;
			SetCommonEventInfo(pInfo);
			return true;
		}
	}
	return false;
}


bool CEpgProgramList::GetNextEventInfo(WORD NetworkID,WORD TSID,WORD ServiceID,
									   const SYSTEMTIME *pTime,CEventInfoData *pInfo)
{
	CBlockLock Lock(&m_Lock);

	CEpgServiceInfo *pServiceInfo=GetServiceInfo(NetworkID,TSID,ServiceID);
	if (pServiceInfo==NULL)
		return NULL;

	CEventInfoList::EventMap::iterator itrEvent;
	const CEventInfoData *pNextEvent=NULL;
	SYSTEMTIME stNearest;
	for (itrEvent=pServiceInfo->m_EventList.EventDataMap.begin();
			itrEvent!=pServiceInfo->m_EventList.EventDataMap.end();++itrEvent) {
		SYSTEMTIME stStart;

		itrEvent->second.GetStartTime(&stStart);
		if (CompareSystemTime(&stStart,pTime)>0
				&& (pNextEvent==NULL || CompareSystemTime(&stStart,&stNearest)<0)) {
			pNextEvent=&itrEvent->second;
			stNearest=stStart;
		}
	}
	if (pNextEvent!=NULL) {
		*pInfo=*pNextEvent;
		SetCommonEventInfo(pInfo);
		return true;
	}
	return false;
}


const CEventInfoData *CEpgProgramList::GetEventInfo(WORD NetworkID,WORD TSID,WORD ServiceID,WORD EventID)
{
	CEpgServiceInfo *pServiceInfo=GetServiceInfo(NetworkID,TSID,ServiceID);

	if (pServiceInfo==NULL)
		return NULL;
	return pServiceInfo->GetEventInfo(EventID);
}


bool CEpgProgramList::SetCommonEventInfo(CEventInfoData *pInfo)
{
	// イベント共有の参照先から情報を取得する
	if (pInfo->m_fCommonEvent) {
		CEventManager::CEventInfo EventInfo;
		if (m_pEventManager->GetEventInfo(pInfo->m_NetworkID,
										  pInfo->m_TSID,
										  pInfo->m_CommonEventInfo.ServiceID,
										  pInfo->m_CommonEventInfo.EventID,
										  &EventInfo)) {
			pInfo->SetEventName(EventInfo.GetEventName());
			pInfo->SetEventText(EventInfo.GetEventText());
			pInfo->SetEventExtText(EventInfo.GetEventExtendedText());
			pInfo->m_CaType=EventInfo.GetFreeCaMode()?
								CEventInfoData::CA_TYPE_CHARGEABLE:
								CEventInfoData::CA_TYPE_FREE;
			pInfo->m_VideoInfo=EventInfo.GetVideoInfo();
			pInfo->m_AudioList=EventInfo.GetAudioList();
			pInfo->m_ContentNibble=EventInfo.GetContentNibble();
		} else {
			const CEventInfoData *pCommonEvent=GetEventInfo(pInfo->m_NetworkID,
															pInfo->m_TSID,
															pInfo->m_CommonEventInfo.ServiceID,
															pInfo->m_CommonEventInfo.EventID);
			if (pCommonEvent==NULL)
				return false;
			pInfo->SetEventName(pCommonEvent->GetEventName());
			pInfo->SetEventText(pCommonEvent->GetEventText());
			pInfo->SetEventExtText(pCommonEvent->GetEventExtText());
			pInfo->m_CaType=pCommonEvent->m_CaType;
			pInfo->m_VideoInfo=pCommonEvent->m_VideoInfo;
			pInfo->m_AudioList=pCommonEvent->m_AudioList;
			pInfo->m_ContentNibble=pCommonEvent->m_ContentNibble;
		}
	}
	return true;
}


bool CEpgProgramList::SetEventExtText(CEventInfoData *pInfo)
{
	if (IsStringEmpty(pInfo->GetEventExtText())) {
		const CEventInfoData *pEvent=GetEventInfo(pInfo->m_NetworkID,
												  pInfo->m_TSID,
												  pInfo->m_CommonEventInfo.ServiceID,
												  pInfo->m_CommonEventInfo.EventID);
		if (pEvent==NULL)
			return false;
		return CopyEventExtText(pInfo,pEvent);
	}
	return true;
}


bool CEpgProgramList::CopyEventExtText(CEventInfoData *pDstInfo,const CEventInfoData *pSrcInfo)
{
	if (IsStringEmpty(pDstInfo->GetEventExtText())
			&& !IsStringEmpty(pSrcInfo->GetEventExtText())
			&& CompareText(pDstInfo->GetEventName(),pSrcInfo->GetEventName())) {
		pDstInfo->SetEventExtText(pSrcInfo->GetEventExtText());
		return true;
	}
	return false;
}


#include <pshpack1.h>

/*
	EPGファイルのフォーマット
	┌─────────────────────┐
	│EpgListFileHeader                         │
	├─────────────────────┤
	│┌───────────────────┐│
	││ServiceInfoHeader2                    ││
	│├───────────────────┤│
	││┌─────────────────┐││
	│││EventInfoHeader2                  │││
	││├─────────────────┤││
	│││┌───────────────┐│││
	││││EventAudioHeader              ││││
	│││├───────────────┤│││
	││││音声コンポーネントテキスト    ││││
	│││└───────────────┘│││
	│││ ...                              │││
	││├─────────────────┤││
	│││┌───────────────┐│││
	││││NibbleData                    ││││
	│││└───────────────┘│││
	│││ ...                              │││
	││├─────────────────┤││
	│││(イベント名)                      │││
	││├─────────────────┤││
	│││(イベントテキスト)                │││
	││├─────────────────┤││
	│││(イベント拡張テキスト)            │││
	││├─────────────────┤││
	│││┌───────────────┐│││
	││││EventAudioExInfo              ││││
	│││└───────────────┘│││
	│││ ...                              │││
	││├─────────────────┤││
	│││┌───────────────┐│││
	││││EventVideoInfo                ││││
	│││├───────────────┤│││
	││││映像コンポーネントテキスト    ││││
	│││└───────────────┘│││
	│││ ...                              │││
	││├─────────────────┤││
	│││┌───────────────┐│││
	││││EventGroupHeader              ││││
	│││├───────────────┤│││
	││││┌─────────────┐││││
	│││││CEventGroupDesc::EventInfo│││││
	││││└─────────────┘││││
	││││ ...                          ││││
	│││└───────────────┘│││
	│││ ...                              │││
	││├─────────────────┤││
	│││CRC                               │││
	││└─────────────────┘││
	││ ...                                  ││
	│└───────────────────┘│
	│ ...                                      │
	└─────────────────────┘
*/

// ver.0.7.0より前の古いEPGファイルに対応
//#define EPG_FILE_V0_SUPPORT

struct EpgListFileHeader {
	char Type[8];
	DWORD Version;
	DWORD NumServices;
};

#define EPGLISTFILEHEADER_TYPE		"EPG-LIST"
#define EPGLISTFILEHEADER_VERSION	1

#ifdef EPG_FILE_V0_SUPPORT

struct ServiceInfoHeader {
	DWORD NumEvents;
	WORD NetworkID;
	WORD TSID;
	WORD ServiceID;
	WORD ServiceType;
	ServiceInfoHeader() {}
	ServiceInfoHeader(const CServiceInfoData &Data)
		: NumEvents(0)
		, NetworkID(Data.m_NetworkID)
		, TSID(Data.m_TSID)
		, ServiceID(Data.m_ServiceID)
		, ServiceType(0)
	{
	}
};

struct EventInfoHeader {
	WORD ServiceID;
	WORD EventID;
	SYSTEMTIME StartTime;
	DWORD DurationSec;
	BYTE ComponentType;
	BYTE AudioComponentType;
	BYTE ESMultiLangFlag;
	BYTE MainComponentFlag;
	BYTE SamplingRate;
	BYTE Reserved[3];
	DWORD ContentNibbleListCount;
};

#endif	// EPG_FILE_V0_SUPPORT

struct ServiceInfoHeader2 {
	WORD NetworkID;
	WORD TransportStreamID;
	WORD ServiceID;
	WORD NumEvents;
	DWORD CRC;
	ServiceInfoHeader2() {}
	ServiceInfoHeader2(const CServiceInfoData &Data)
		: NetworkID(Data.m_NetworkID)
		, TransportStreamID(Data.m_TSID)
		, ServiceID(Data.m_ServiceID)
		, NumEvents(0)
	{
	}
};

#define MAX_EPG_TEXT_LENGTH 4096
#define MAX_CONTENT_NIBBLE_COUNT 7

#define EVENTINFO_FLAG_RUNNINGSTATUSMASK	0x07
#define EVENTINFO_FLAG_CATYPE_MASK			0x18
#define EVENTINFO_FLAG_CATYPE_SHIFT			3

#define DATA_FLAG_EVENTNAME		0x80
#define DATA_FLAG_EVENTTEXT		0x40
#define DATA_FLAG_EVENTEXTTEXT	0x20
#define DATA_FLAG_AUDIOINFO		0x10
#define DATA_FLAG_VIDEOINFO		0x08
#define DATA_FLAG_GROUPINFO		0x04

struct EventInfoHeader2 {
	WORD EventID;
	WORD CommonServiceID;
	WORD CommonEventID;
	BYTE Flags;
	BYTE VideoListCount;
	ULONGLONG UpdateTime;
	SYSTEMTIME StartTime;
	DWORD Duration;
	BYTE ComponentType;
	BYTE AudioListCount;
	BYTE ContentNibbleListCount;
	BYTE DataFlags;
	EventInfoHeader2() {}
	EventInfoHeader2(const CEventInfoData &Data)
		: EventID(Data.m_EventID)
		, CommonServiceID(Data.m_fCommonEvent?Data.m_CommonEventInfo.ServiceID:0)
		, CommonEventID(Data.m_fCommonEvent?Data.m_CommonEventInfo.EventID:0)
		, Flags(Data.m_RunningStatus | (Data.m_CaType<<EVENTINFO_FLAG_CATYPE_SHIFT))
		, VideoListCount(1)
		, UpdateTime(Data.m_UpdateTime)
		, StartTime(Data.m_stStartTime)
		, Duration(Data.m_DurationSec)
		, ComponentType(Data.m_VideoInfo.ComponentType)
		, AudioListCount((BYTE)Data.m_AudioList.size())
		, ContentNibbleListCount((BYTE)min(Data.m_ContentNibble.NibbleCount,MAX_CONTENT_NIBBLE_COUNT))
		, DataFlags((Data.GetEventName()!=NULL?DATA_FLAG_EVENTNAME:0) |
					(Data.GetEventText()!=NULL?DATA_FLAG_EVENTTEXT:0) |
					(Data.GetEventExtText()!=NULL?DATA_FLAG_EVENTEXTTEXT:0) |
					(Data.m_AudioList.size()>0?DATA_FLAG_AUDIOINFO:0) |
					DATA_FLAG_VIDEOINFO |
					(Data.m_EventGroupList.size()>0?DATA_FLAG_GROUPINFO:0))
	{
	}
};

struct EventAudioHeader {
	BYTE Flags;
	BYTE ComponentType;
	BYTE SamplingRate;
	BYTE Reserved;
	DWORD LanguageCode;
	DWORD LanguageCode2;
};

#define AUDIO_FLAG_MULTILINGUAL		0x01
#define AUDIO_FLAG_MAINCOMPONENT	0x02

struct EventVideoInfo {
	BYTE StreamContent;
	BYTE ComponentType;
	BYTE ComponentTag;
	BYTE Reserved;
	DWORD LanguageCode;
};

struct EventAudioExInfo {
	BYTE StreamContent;
	BYTE ComponentTag;
	BYTE SimulcastGroupTag;
	BYTE QualityIndicator;
};

struct EventGroupHeader {
	BYTE GroupType;
	BYTE EventCount;
};

#include <poppack.h>


static bool ReadData(CNFile *pFile,void *pData,DWORD DataSize,CCrc32 *pCrc)
{
	if (pFile->Read(pData,DataSize)!=DataSize)
		return false;
	pCrc->Calc(pData,DataSize);
	return true;
}

static bool ReadString(CNFile *pFile,LPWSTR *ppszString,CCrc32 *pCrc)
{
	WORD Length;

	*ppszString=NULL;
	if (!ReadData(pFile,&Length,sizeof(WORD),pCrc)
			|| Length>MAX_EPG_TEXT_LENGTH)
		return false;
	if (Length>0) {
		*ppszString=new WCHAR[Length+1];
		if (!ReadData(pFile,*ppszString,Length*sizeof(WCHAR),pCrc)) {
			delete [] *ppszString;
			*ppszString=NULL;
			return false;
		}
		(*ppszString)[Length]=L'\0';
	}
	return true;
}

#ifdef EPG_FILE_V0_SUPPORT
// 旧形式用
static bool ReadString(CNFile *pFile,LPWSTR *ppszString)
{
	DWORD Length;

	*ppszString=NULL;
	if (pFile->Read(&Length,sizeof(DWORD))!=sizeof(DWORD)
			|| Length>MAX_EPG_TEXT_LENGTH)
		return false;
	if (Length>0) {
		*ppszString=new WCHAR[Length+1];
		if (pFile->Read(*ppszString,Length*sizeof(WCHAR))!=Length*sizeof(WCHAR)) {
			delete [] *ppszString;
			*ppszString=NULL;
			return false;
		}
		(*ppszString)[Length]=L'\0';
	}
	return true;
}
#endif

static bool WriteData(CNFile *pFile,const void *pData,DWORD DataSize,CCrc32 *pCrc)
{
	pCrc->Calc(pData,DataSize);
	return pFile->Write(pData,DataSize);
}

static bool WriteString(CNFile *pFile,LPCWSTR pszString,CCrc32 *pCrc)
{
	WORD Length;

	if (pszString!=NULL) {
		Length=(WORD)::lstrlenW(pszString);
		if (Length>MAX_EPG_TEXT_LENGTH)
			Length=MAX_EPG_TEXT_LENGTH;
	} else
		Length=0;
	if (!WriteData(pFile,&Length,sizeof(WORD),pCrc))
		return false;
	if (Length>0) {
		if (!WriteData(pFile,pszString,Length*sizeof(WCHAR),pCrc))
			return false;
	}
	return true;
}

static bool WriteCRC(CNFile *pFile,const CCrc32 *pCrc)
{
	DWORD CRC32=pCrc->GetCrc();
	return pFile->Write(&CRC32,sizeof(CRC32));
}


bool CEpgProgramList::LoadFromFile(LPCTSTR pszFileName)
{
	CBlockLock Lock(&m_Lock);
	CAppMain &AppMain=GetAppClass();
	CNFile File;

	if (!File.Open(pszFileName,
				   CNFile::CNF_READ | CNFile::CNF_SHAREREAD |
				   CNFile::CNF_SEQUENTIALREAD | CNFile::CNF_PRIORITY_LOW)) {
		AppMain.AddLog(TEXT("EPGファイルを開けません。(エラーコード 0x%lu)"),
					   File.GetLastError());
		return false;
	}

	EpgListFileHeader FileHeader;

	if (File.Read(&FileHeader,sizeof(EpgListFileHeader))!=sizeof(EpgListFileHeader))
		return false;
	if (memcmp(FileHeader.Type,EPGLISTFILEHEADER_TYPE,sizeof(FileHeader.Type))!=0)
		return false;
#ifndef EPG_FILE_V0_SUPPORT
	if (FileHeader.Version==0) {
		AppMain.AddLog(TEXT("EPGファイルが古い形式のため読み込めません。"));
		return false;
	}
#endif
	if (FileHeader.Version>EPGLISTFILEHEADER_VERSION) {
		AppMain.AddLog(TEXT("EPGファイルが未知の形式のため読み込めません。"));
		return false;
	}

	Clear();

	for (DWORD i=0;i<FileHeader.NumServices;i++) {
		LPWSTR pszText;

		ServiceInfoHeader2 ServiceHeader2;
#ifdef EPG_FILE_V0_SUPPORT
		if (FileHeader.Version==0) {
			ServiceInfoHeader ServiceHeader;

			if (File.Read(&ServiceHeader,sizeof(ServiceInfoHeader))!=sizeof(ServiceInfoHeader)
					|| !ReadString(&File,&pszText))
				goto OnError;
			delete [] pszText;
			if (ServiceHeader.NumEvents>0xFFFF)
				goto OnError;
			ServiceHeader2.NetworkID=ServiceHeader.NetworkID;
			ServiceHeader2.TransportStreamID=ServiceHeader.TSID;
			ServiceHeader2.ServiceID=ServiceHeader.ServiceID;
			ServiceHeader2.NumEvents=(WORD)ServiceHeader.NumEvents;
		} else
#endif
		{
			if (File.Read(&ServiceHeader2,sizeof(ServiceInfoHeader2))!=sizeof(ServiceInfoHeader2))
				goto OnError;
			if (ServiceHeader2.CRC!=CCrcCalculator::CalcCrc32((const BYTE*)&ServiceHeader2,
															  sizeof(ServiceInfoHeader2)-sizeof(DWORD))) {
				AppMain.AddLog(TEXT("EPGファイルの破損が検出されました。"));
				goto OnError;
			}
		}

		CServiceInfoData ServiceData(ServiceHeader2.NetworkID,
									 ServiceHeader2.TransportStreamID,
									 ServiceHeader2.ServiceID);

		CEpgServiceInfo *pServiceInfo=new CEpgServiceInfo(ServiceData);
		if (!m_ServiceMap.insert(
				std::pair<ServiceMapKey,CEpgServiceInfo*>(
					GetServiceMapKey(ServiceData),
					pServiceInfo)).second) {
			delete pServiceInfo;
			break;
		}
#ifdef EVENTMANAGER_USE_UNORDERED_MAP
		pServiceInfo->m_EventList.EventDataMap.rehash(ServiceHeader2.NumEvents);
#endif

		bool fCRCError=false;

		for (WORD j=0;j<ServiceHeader2.NumEvents;j++) {
			CEventInfoData *pEventData;

#ifdef EPG_FILE_V0_SUPPORT
			if (FileHeader.Version==0) {
				EventInfoHeader EventHeader;
				if (File.Read(&EventHeader,sizeof(EventInfoHeader))!=sizeof(EventInfoHeader))
					goto OnError;

				pEventData=
					&pServiceInfo->m_EventList.EventDataMap.insert(
						std::pair<WORD,CEventInfoData>(EventHeader.EventID,CEventInfoData())).first->second;

				pEventData->m_NetworkID=ServiceHeader2.NetworkID;
				pEventData->m_TSID=ServiceHeader2.TransportStreamID;
				pEventData->m_ServiceID=EventHeader.ServiceID;
				pEventData->m_EventID=EventHeader.EventID;
				pEventData->m_fValidStartTime=true;
				pEventData->m_stStartTime=EventHeader.StartTime;
				pEventData->m_DurationSec=EventHeader.DurationSec;
				pEventData->m_RunningStatus=0;
				pEventData->m_CaType=CEventInfoData::CA_TYPE_UNKNOWN;
				::ZeroMemory(&pEventData->m_VideoInfo,sizeof(CEventInfoData::VideoInfo));
				pEventData->m_VideoInfo.ComponentType=EventHeader.ComponentType;
				pEventData->m_AudioList.resize(1);
				pEventData->m_AudioList[0].ComponentType=EventHeader.AudioComponentType;
				pEventData->m_AudioList[0].bESMultiLingualFlag=EventHeader.ESMultiLangFlag!=0;
				//pEventData->m_AudioList[0].bMainComponentFlag=EventHeader.MainComponentFlag!=0;
				pEventData->m_AudioList[0].bMainComponentFlag=true;
				pEventData->m_AudioList[0].SamplingRate=EventHeader.SamplingRate;
				pEventData->m_AudioList[0].LanguageCode=0;
				pEventData->m_AudioList[0].LanguageCode2=0;
				pEventData->m_AudioList[0].szText[0]='\0';
				if (EventHeader.ContentNibbleListCount>0) {
					if (EventHeader.ContentNibbleListCount>MAX_CONTENT_NIBBLE_COUNT)
						goto OnError;
					pEventData->m_ContentNibble.NibbleCount=EventHeader.ContentNibbleListCount;
					for (DWORD k=0;k<EventHeader.ContentNibbleListCount;k++) {
						CEventInfoData::NibbleData Nibble;
						if (File.Read(&Nibble,sizeof(Nibble))!=sizeof(Nibble))
							goto OnError;
						pEventData->m_ContentNibble.NibbleList[k]=Nibble;
					}
				}
				pEventData->m_fCommonEvent=false;
				pEventData->m_UpdateTime=0;
				LPWSTR pszEventName=NULL,pszEventText=NULL,pszEventExtText=NULL;
				if (!ReadString(&File,&pszEventName)
						|| !ReadString(&File,&pszEventText)
						|| !ReadString(&File,&pszEventExtText))
					goto OnError;
				pEventData->m_EventName.Attach(pszEventName);
				pEventData->m_EventText.Attach(pszEventText);
				pEventData->m_EventExtText.Attach(pszEventExtText);
				if (!ReadString(&File,&pszText))	// Component type text
					goto OnError;
				delete [] pszText;
				if (!ReadString(&File,&pszText))
					goto OnError;
				if (pszText!=NULL) {
					::lstrcpyn(pEventData->m_AudioList[0].szText,pszText,CEventInfoData::AudioInfo::MAX_TEXT);
					delete [] pszText;
				}
				pEventData->m_fDatabase=true;
			} else
#endif
			{
				EventInfoHeader2 EventHeader2;
				CCrc32 CRC;
				if (!ReadData(&File,&EventHeader2,sizeof(EventInfoHeader2),&CRC))
					goto OnError;

				CEventInfoList::EventMap::iterator itrEvent=
					pServiceInfo->m_EventList.EventDataMap.insert(
						std::pair<WORD,CEventInfoData>(EventHeader2.EventID,CEventInfoData())).first;
				pEventData=&itrEvent->second;

				pEventData->m_NetworkID=ServiceHeader2.NetworkID;
				pEventData->m_TSID=ServiceHeader2.TransportStreamID;
				pEventData->m_ServiceID=ServiceHeader2.ServiceID;
				pEventData->m_EventID=EventHeader2.EventID;
				pEventData->m_fValidStartTime=true;
				pEventData->m_stStartTime=EventHeader2.StartTime;
				pEventData->m_DurationSec=EventHeader2.Duration;
				pEventData->m_RunningStatus=EventHeader2.Flags&EVENTINFO_FLAG_RUNNINGSTATUSMASK;
				pEventData->m_CaType=
					(EventHeader2.Flags&EVENTINFO_FLAG_CATYPE_MASK)>>EVENTINFO_FLAG_CATYPE_SHIFT;
				::ZeroMemory(&pEventData->m_VideoInfo,sizeof(CEventInfoData::VideoInfo));
				pEventData->m_VideoInfo.ComponentType=EventHeader2.ComponentType;

				pEventData->m_AudioList.resize(EventHeader2.AudioListCount);
				if (EventHeader2.AudioListCount>0) {
					for (int k=0;k<EventHeader2.AudioListCount;k++) {
						EventAudioHeader AudioHeader;

						if (!ReadData(&File,&AudioHeader,sizeof(AudioHeader),&CRC))
							goto OnError;
						CEventInfoData::AudioInfo &AudioInfo=pEventData->m_AudioList[k];
						::ZeroMemory(&AudioInfo,sizeof(AudioInfo));
						AudioInfo.ComponentType=AudioHeader.ComponentType;
						AudioInfo.bESMultiLingualFlag=(AudioHeader.Flags&AUDIO_FLAG_MULTILINGUAL)!=0;
						AudioInfo.bMainComponentFlag=(AudioHeader.Flags&AUDIO_FLAG_MAINCOMPONENT)!=0;
						AudioInfo.SamplingRate=AudioHeader.SamplingRate;
						AudioInfo.LanguageCode=AudioHeader.LanguageCode;
						AudioInfo.LanguageCode2=AudioHeader.LanguageCode2;
						if (!ReadString(&File,&pszText,&CRC))
							goto OnError;
						if (pszText!=NULL) {
							::lstrcpyn(AudioInfo.szText,pszText,CEventInfoData::AudioInfo::MAX_TEXT);
							delete [] pszText;
						}
					}
				}

				if (EventHeader2.ContentNibbleListCount>0) {
					if (EventHeader2.ContentNibbleListCount>MAX_CONTENT_NIBBLE_COUNT)
						goto OnError;
					pEventData->m_ContentNibble.NibbleCount=EventHeader2.ContentNibbleListCount;
					for (int k=0;k<EventHeader2.ContentNibbleListCount;k++) {
						CEventInfoData::NibbleData Nibble;
						if (!ReadData(&File,&Nibble,sizeof(Nibble),&CRC))
							goto OnError;
						pEventData->m_ContentNibble.NibbleList[k]=Nibble;
					}
				} else {
					pEventData->m_ContentNibble.NibbleCount=0;
				}

				pEventData->m_fCommonEvent=EventHeader2.CommonServiceID!=0
											&& EventHeader2.CommonEventID!=0;
				if (pEventData->m_fCommonEvent) {
					pEventData->m_CommonEventInfo.ServiceID=EventHeader2.CommonServiceID;
					pEventData->m_CommonEventInfo.EventID=EventHeader2.CommonEventID;
				}

				pEventData->m_UpdateTime=EventHeader2.UpdateTime;

				for (BYTE Flag=0x80;Flag!=0;Flag>>=1) {
					if ((EventHeader2.DataFlags&Flag)!=0) {
						if (Flag==DATA_FLAG_AUDIOINFO) {
							WORD Size;
							if (!ReadData(&File,&Size,sizeof(WORD),&CRC)
									|| Size!=EventHeader2.AudioListCount*sizeof(EventAudioExInfo))
								goto OnError;
							for (int k=0;k<(int)EventHeader2.AudioListCount;k++) {
								EventAudioExInfo AudioInfo;

								if (!ReadData(&File,&AudioInfo,sizeof(AudioInfo),&CRC))
									goto OnError;
								CEventInfoData::AudioInfo &Audio=pEventData->m_AudioList[k];
								Audio.StreamContent=AudioInfo.StreamContent;
								Audio.ComponentTag=AudioInfo.ComponentTag;
								Audio.SimulcastGroupTag=AudioInfo.SimulcastGroupTag;
								Audio.QualityIndicator=AudioInfo.QualityIndicator;
							}
						} else if (Flag==DATA_FLAG_VIDEOINFO) {
							WORD Size;
							if (!ReadData(&File,&Size,sizeof(WORD),&CRC)
									|| Size<EventHeader2.VideoListCount*(sizeof(EventVideoInfo)+sizeof(WORD)))
								goto OnError;
							for (int k=0;k<(int)EventHeader2.VideoListCount;k++) {
								EventVideoInfo VideoInfo;
								if (!ReadData(&File,&VideoInfo,sizeof(VideoInfo),&CRC)
										|| !ReadString(&File,&pszText,&CRC))
									goto OnError;
								if (k==0) {
									pEventData->m_VideoInfo.StreamContent=VideoInfo.StreamContent;
									pEventData->m_VideoInfo.ComponentType=VideoInfo.ComponentType;
									pEventData->m_VideoInfo.ComponentTag=VideoInfo.ComponentTag;
									pEventData->m_VideoInfo.LanguageCode=VideoInfo.LanguageCode;
									if (pszText!=NULL)
										::lstrcpyn(pEventData->m_VideoInfo.szText,pszText,
												   CEventInfoData::VideoInfo::MAX_TEXT);
								}
								delete [] pszText;
							}
						} else if (Flag==DATA_FLAG_GROUPINFO) {
							WORD Size,GroupCount;

							if (!ReadData(&File,&Size,sizeof(WORD),&CRC)
									|| !ReadData(&File,&GroupCount,sizeof(WORD),&CRC)
									|| Size<GroupCount*sizeof(EventGroupHeader))
								goto OnError;
							pEventData->m_EventGroupList.resize(GroupCount);
							for (int k=0;k<(int)GroupCount;k++) {
								EventGroupHeader GroupHeader;

								if (!ReadData(&File,&GroupHeader,sizeof(GroupHeader),&CRC))
									goto OnError;
								CEventInfoData::EventGroupInfo &Group=pEventData->m_EventGroupList[k];
								Group.GroupType=GroupHeader.GroupType;
								Group.EventList.resize(GroupHeader.EventCount);
								if (!ReadData(&File,&Group.EventList[0],
											  sizeof(CEventGroupDesc::EventInfo)*GroupHeader.EventCount,
											  &CRC))
									goto OnError;
							}
						} else {
							if (!ReadString(&File,&pszText,&CRC))
								goto OnError;
							if (pszText!=NULL) {
								switch (Flag) {
								case DATA_FLAG_EVENTNAME:
									pEventData->m_EventName.Attach(pszText);
									break;
								case DATA_FLAG_EVENTTEXT:
									pEventData->m_EventText.Attach(pszText);
									break;
								case DATA_FLAG_EVENTEXTTEXT:
									pEventData->m_EventExtText.Attach(pszText);
									break;
								default:
									delete [] pszText;
								}
							}
						}
					}
				}

				pEventData->m_fDatabase=true;

				DWORD CRC32;
				if (File.Read(&CRC32,sizeof(DWORD))!=sizeof(DWORD))
					goto OnError;
				if (CRC32!=CRC.GetCrc()) {
					// 2回続けてCRCエラーの場合は読み込み中止
					if (fCRCError) {
						AppMain.AddLog(TEXT("EPGファイルの破損が検出されました。"));
						goto OnError;
					}
					fCRCError=true;
					pServiceInfo->m_EventList.EventDataMap.erase(itrEvent);
				} else {
					fCRCError=false;
				}
			}
		}
	}

	File.GetTime(NULL,NULL,&m_LastWriteTime);
	return true;

OnError:
	AppMain.AddLog(TEXT("EPGファイルの読み込みエラーが発生しました。"));
	Clear();
	return false;
}


bool CEpgProgramList::SaveToFile(LPCTSTR pszFileName)
{
	CBlockLock Lock(&m_Lock);
	CAppMain &AppMain=GetAppClass();

	TCHAR szName[MAX_PATH+8];
	::lstrcpy(szName,pszFileName);
	::CharUpperBuff(szName,::lstrlen(szName));
	for (int i=0;szName[i]!=_T('\0');i++) {
		if (szName[i]==_T('\\'))
			szName[i]=_T(':');
	}
	::lstrcat(szName,TEXT(":Lock"));
	CGlobalLock GlobalLock;
	if (GlobalLock.Create(szName)) {
		if (!GlobalLock.Wait(10000)) {
			AppMain.AddLog(TEXT("EPGファイルがロックされているため保存できません。"));
			return false;
		}
	}

	// ファイルが読み込んだ時から更新されている場合読み込み直す
	// (複数起動して他のプロセスが更新した可能性があるため)
	WIN32_FIND_DATA fd;
	HANDLE hFind=::FindFirstFile(pszFileName,&fd);
	if (hFind!=INVALID_HANDLE_VALUE) {
		::FindClose(hFind);
		if ((m_LastWriteTime.dwLowDateTime==0 && m_LastWriteTime.dwHighDateTime==0)
				|| ::CompareFileTime(&fd.ftLastWriteTime,&m_LastWriteTime)>0) {
			TRACE(TEXT("CEpgProgramList::SaveToFile() : Reload\n"));
			CEpgProgramList List(m_pEventManager);
			if (List.LoadFromFile(pszFileName))
				Merge(&List);
		}
	}

	CNFile File;

	if (!File.Open(pszFileName,CNFile::CNF_WRITE | CNFile::CNF_NEW)) {
		GlobalLock.Release();
		AppMain.AddLog(TEXT("EPGファイルが開けません。(エラーコード 0x%lx)"),
					   File.GetLastError());
		return false;
	}

	SYSTEMTIME stCurrent,st;
	GetCurrentJST(&stCurrent);

	WORD *pNumEvents=new WORD[m_ServiceMap.size()];
	DWORD NumServices=0;
	size_t ServiceIndex=0;
	for (ServiceMap::iterator itrService=m_ServiceMap.begin();
			itrService!=m_ServiceMap.end();
			++itrService) {
		const CEpgServiceInfo *pServiceInfo=itrService->second;
		CEventInfoList::EventMap::const_iterator itrEvent;
		WORD NumEvents=0;

		for (itrEvent=pServiceInfo->m_EventList.EventDataMap.begin();
				itrEvent!=pServiceInfo->m_EventList.EventDataMap.end();
				++itrEvent) {
			if (itrEvent->second.GetEndTime(&st)
					&& CompareSystemTime(&st,&stCurrent)>0)
				NumEvents++;
		}
		pNumEvents[ServiceIndex++]=NumEvents;
		if (NumEvents>0)
			NumServices++;
	}

	EpgListFileHeader FileHeader;

	::CopyMemory(FileHeader.Type,EPGLISTFILEHEADER_TYPE,sizeof(FileHeader.Type));
	FileHeader.Version=EPGLISTFILEHEADER_VERSION;
	FileHeader.NumServices=NumServices;

	if (!File.Write(&FileHeader,sizeof(EpgListFileHeader)))
		goto OnError;

	ServiceIndex=0;
	for (ServiceMap::iterator itrService=m_ServiceMap.begin();
		 	itrService!=m_ServiceMap.end();++itrService,ServiceIndex++) {
		if (pNumEvents[ServiceIndex]==0)
			continue;

		const CEpgServiceInfo *pServiceInfo=itrService->second;
		ServiceInfoHeader2 ServiceHeader2(pServiceInfo->m_ServiceData);
		ServiceHeader2.NumEvents=pNumEvents[ServiceIndex];
		ServiceHeader2.CRC=CCrcCalculator::CalcCrc32((const BYTE*)&ServiceHeader2,
													 sizeof(ServiceInfoHeader2)-sizeof(DWORD));
		if (!File.Write(&ServiceHeader2,sizeof(ServiceInfoHeader2)))
			goto OnError;

		CEventInfoList::EventMap::const_iterator itrEvent;
		for (itrEvent=pServiceInfo->m_EventList.EventDataMap.begin();
				itrEvent!=pServiceInfo->m_EventList.EventDataMap.end();
				++itrEvent) {
			const CEventInfoData &EventInfo=itrEvent->second;

			if (EventInfo.GetEndTime(&st)
					&& CompareSystemTime(&st,&stCurrent)>0) {
				CCrc32 CRC;

				EventInfoHeader2 EventHeader2(EventInfo);
				if (!WriteData(&File,&EventHeader2,sizeof(EventInfoHeader2),&CRC))
					goto OnError;

				if (EventHeader2.AudioListCount>0) {
					for (size_t i=0;i<EventInfo.m_AudioList.size();i++) {
						const CEventInfoData::AudioInfo &Audio=EventInfo.m_AudioList[i];
						EventAudioHeader AudioHeader;

						AudioHeader.Flags=0;
						if (Audio.bESMultiLingualFlag)
							AudioHeader.Flags|=AUDIO_FLAG_MULTILINGUAL;
						if (Audio.bMainComponentFlag)
							AudioHeader.Flags|=AUDIO_FLAG_MAINCOMPONENT;
						AudioHeader.ComponentType=Audio.ComponentType;
						AudioHeader.SamplingRate=Audio.SamplingRate;
						AudioHeader.LanguageCode=Audio.LanguageCode;
						AudioHeader.LanguageCode2=Audio.LanguageCode2;
						AudioHeader.Reserved=0;
						if (!WriteData(&File,&AudioHeader,sizeof(EventAudioHeader),&CRC)
								|| !WriteString(&File,Audio.szText,&CRC))
							goto OnError;
					}
				}

				if (EventHeader2.ContentNibbleListCount>0) {
					for (int i=0;i<EventHeader2.ContentNibbleListCount;i++) {
						if (!WriteData(&File,&EventInfo.m_ContentNibble.NibbleList[i],
									   sizeof(CEventInfoData::NibbleData),&CRC))
							goto OnError;
					}
				}

				if (((EventHeader2.DataFlags&DATA_FLAG_EVENTNAME)!=0
							&& !WriteString(&File,EventInfo.GetEventName(),&CRC))
						|| ((EventHeader2.DataFlags&DATA_FLAG_EVENTTEXT)!=0
							&& !WriteString(&File,EventInfo.GetEventText(),&CRC))
						|| ((EventHeader2.DataFlags&DATA_FLAG_EVENTEXTTEXT)!=0
							&& !WriteString(&File,EventInfo.GetEventExtText(),&CRC)))
					goto OnError;

				if (EventHeader2.AudioListCount>0) {
					WORD Size=(WORD)(EventHeader2.AudioListCount*sizeof(EventAudioExInfo));
					if (!WriteData(&File,&Size,sizeof(WORD),&CRC))
						goto OnError;
					for (size_t i=0;i<EventInfo.m_AudioList.size();i++) {
						const CEventInfoData::AudioInfo &Audio=EventInfo.m_AudioList[i];
						EventAudioExInfo AudioInfo;

						AudioInfo.StreamContent=Audio.StreamContent;
						AudioInfo.ComponentTag=Audio.ComponentTag;
						AudioInfo.SimulcastGroupTag=Audio.SimulcastGroupTag;
						AudioInfo.QualityIndicator=Audio.QualityIndicator;
						if (!WriteData(&File,&AudioInfo,sizeof(AudioInfo),&CRC))
							goto OnError;
					}
				}

				WORD Size=(WORD)(sizeof(EventVideoInfo)+
								 sizeof(WORD)+::lstrlenW(EventInfo.m_VideoInfo.szText)*sizeof(WCHAR));
				if (!WriteData(&File,&Size,sizeof(WORD),&CRC))
					goto OnError;
				EventVideoInfo VideoInfo;
				VideoInfo.StreamContent=EventInfo.m_VideoInfo.StreamContent;
				VideoInfo.ComponentType=EventInfo.m_VideoInfo.ComponentType;
				VideoInfo.ComponentTag=EventInfo.m_VideoInfo.ComponentTag;
				VideoInfo.Reserved=0;
				VideoInfo.LanguageCode=EventInfo.m_VideoInfo.LanguageCode;
				if (!WriteData(&File,&VideoInfo,sizeof(VideoInfo),&CRC)
						|| !WriteString(&File,EventInfo.m_VideoInfo.szText,&CRC))
					goto OnError;

				if (EventInfo.m_EventGroupList.size()>0) {
					WORD Size=sizeof(WORD);
					for (size_t i=0;i<EventInfo.m_EventGroupList.size();i++)
						Size+=(WORD)(sizeof(EventGroupHeader)+
							EventInfo.m_EventGroupList[i].EventList.size()*sizeof(CEventGroupDesc::EventInfo));
					if (!WriteData(&File,&Size,sizeof(Size),&CRC))
						goto OnError;
					WORD GroupCount=(WORD)EventInfo.m_EventGroupList.size();
					if (!WriteData(&File,&GroupCount,sizeof(GroupCount),&CRC))
						goto OnError;
					for (size_t i=0;i<EventInfo.m_EventGroupList.size();i++) {
						const CEventInfoData::EventGroupInfo &Group=EventInfo.m_EventGroupList[i];
						EventGroupHeader GroupHeader;

						GroupHeader.GroupType=Group.GroupType;
						GroupHeader.EventCount=(BYTE)Group.EventList.size();
						if (!WriteData(&File,&GroupHeader,sizeof(GroupHeader),&CRC)
								|| !WriteData(&File,&Group.EventList[0],
										(DWORD)(sizeof(CEventGroupDesc::EventInfo)*Group.EventList.size()),&CRC))
							goto OnError;
					}
				}

				if (!WriteCRC(&File,&CRC))
					goto OnError;
			}
		}
	}

	delete [] pNumEvents;

	File.Close();

	HANDLE hFile=::CreateFile(pszFileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
	if (hFile!=INVALID_HANDLE_VALUE) {
		::GetFileTime(hFile,NULL,NULL,&m_LastWriteTime);
		::CloseHandle(hFile);
	}

	GlobalLock.Release();
	return true;

OnError:
	AppMain.AddLog(TEXT("EPGファイルの書き出しエラーが発生しました。(エラーコード 0x%lx)"),
				   File.GetLastError());
	delete [] pNumEvents;
	File.Close();
	::DeleteFile(pszFileName);
	GlobalLock.Release();
	return false;
}


bool CEpgProgramList::Merge(CEpgProgramList *pSrcList)
{
	ServiceMap::iterator itrSrcService;

	for (itrSrcService=pSrcList->m_ServiceMap.begin();
			itrSrcService!=pSrcList->m_ServiceMap.end();) {
		CEpgServiceInfo *pSrcServiceInfo=itrSrcService->second;
		ServiceMapKey Key=GetServiceMapKey(pSrcServiceInfo->m_ServiceData);
		ServiceMap::iterator itrDstService=m_ServiceMap.find(Key);
		if (itrDstService==m_ServiceMap.end()) {
			m_ServiceMap.insert(std::pair<ServiceMapKey,CEpgServiceInfo*>(Key,pSrcServiceInfo));
			pSrcList->m_ServiceMap.erase(itrSrcService++);
		} else {
			CEventInfoList::EventMap::iterator itrEvent;

			for (itrEvent=pSrcServiceInfo->m_EventList.EventDataMap.begin();
					itrEvent!=pSrcServiceInfo->m_EventList.EventDataMap.end();
					++itrEvent) {
				itrDstService->second->m_EventList.EventDataMap.insert(
					std::pair<WORD,CEventInfoData>(itrEvent->first,itrEvent->second));
			}
			++itrSrcService;
		}
	}
	return true;
}
