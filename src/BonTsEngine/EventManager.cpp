#include "stdafx.h"
#include "Common.h"
#include "EventManager.h"
#include "TsEncode.h"
#include "../Common/DebugDef.h"


/////////////////////////////////////////////////////////////////////////////
// EITテーブル解析クラス
/////////////////////////////////////////////////////////////////////////////

class CEitParserTable : public CPsiStreamTable
{
public:
	CEitParserTable(ISectionHandler *pHandler = NULL);

// CPsiStreamTable
	void Reset() override;

// CEitParserTable
	struct EventInfo {
		WORD EventID;
		bool bValidStartTime;
		SYSTEMTIME StartTime;
		DWORD Duration;
		BYTE RunningStatus;
		bool bFreeCaMode;
		CDescBlock DescBlock;
	};

	WORD GetServiceID() const;
	WORD GetTransportStreamID() const;
	WORD GetOriginalNetworkID() const;
	BYTE GetSegmentLastSectionNumber() const;
	BYTE GetLastTableID() const;
	int GetEventNum() const;
	const EventInfo * GetEventInfo(const int Index) const;
	WORD GetEventID(const int Index) const;
	const SYSTEMTIME * GetStartTime(const int Index) const;
	DWORD GetDuration(const int Index) const;
	BYTE GetRunningStatus(const int Index) const;
	bool GetFreeCaMode(const int Index) const;
	const CDescBlock * GetItemDesc(const int Index) const;

protected:
	const bool OnTableUpdate(const CPsiSection *pCurSection) override;

	WORD m_ServiceID;
	WORD m_TransportStreamID;
	WORD m_OriginalNetworkID;
	BYTE m_SegmentLastSectionNumber;
	BYTE m_LastTableID;

	std::vector<EventInfo> m_EventList;
};

CEitParserTable::CEitParserTable(ISectionHandler *pHandler)
	: CPsiStreamTable(pHandler)
{
	Reset();
}

void CEitParserTable::Reset(void)
{
	// 状態をクリアする
	CPsiStreamTable::Reset();

	m_ServiceID = 0;
	m_TransportStreamID = 0;
	m_OriginalNetworkID = 0;
	m_SegmentLastSectionNumber = 0;
	m_LastTableID = 0;
	m_EventList.clear();
}

WORD CEitParserTable::GetServiceID() const
{
	return m_ServiceID;
}

WORD CEitParserTable::GetTransportStreamID() const
{
	return m_TransportStreamID;
}

WORD CEitParserTable::GetOriginalNetworkID() const
{
	return m_OriginalNetworkID;
}


BYTE CEitParserTable::GetSegmentLastSectionNumber() const
{
	return m_SegmentLastSectionNumber;
}


BYTE CEitParserTable::GetLastTableID() const
{
	return m_LastTableID;
}


int CEitParserTable::GetEventNum() const
{
	return (int)m_EventList.size();
}

const CEitParserTable::EventInfo * CEitParserTable::GetEventInfo(const int Index) const
{
	if (Index < 0 || (size_t)Index >= m_EventList.size())
		return NULL;
	return &m_EventList[Index];
}

WORD CEitParserTable::GetEventID(const int Index) const
{
	if (Index < 0 || (size_t)Index >= m_EventList.size())
		return 0;
	return m_EventList[Index].EventID;
}

const SYSTEMTIME * CEitParserTable::GetStartTime(const int Index) const
{
	if (Index < 0 || (size_t)Index >= m_EventList.size())
		return NULL;
	return &m_EventList[Index].StartTime;
}

DWORD CEitParserTable::GetDuration(const int Index) const
{
	if (Index < 0 || (size_t)Index >= m_EventList.size())
		return 0;
	return m_EventList[Index].Duration;
}

BYTE CEitParserTable::GetRunningStatus(const int Index) const
{
	if (Index < 0 || (size_t)Index >= m_EventList.size())
		return 0;
	return m_EventList[Index].RunningStatus;
}

bool CEitParserTable::GetFreeCaMode(const int Index) const
{
	// Free CA Modeを返す
	if (Index < 0 || (size_t)Index >= m_EventList.size())
		return 0;
	return m_EventList[Index].bFreeCaMode;
}

const CDescBlock * CEitParserTable::GetItemDesc(const int Index) const
{
	// アイテムの記述子ブロックを返す
	if (Index < 0 || (size_t)Index >= m_EventList.size())
		return NULL;
	return &m_EventList[Index].DescBlock;
}

const bool CEitParserTable::OnTableUpdate(const CPsiSection *pCurSection)
{
	const BYTE TableID = pCurSection->GetTableID();
	if (TableID < 0x4E || TableID > 0x6F
			/*|| (TableID <= 0x4F && pCurSection->GetSectionNumber() > 0x01)*/)
		return false;

	const WORD DataSize = pCurSection->GetPayloadSize();
	const BYTE *pData = pCurSection->GetPayloadData();

	if (DataSize < 6)
		return false;

	m_ServiceID = pCurSection->GetTableIdExtension();
	m_TransportStreamID = ((WORD)pData[0] << 8) | (WORD)pData[1];
	m_OriginalNetworkID = ((WORD)pData[2] << 8) | (WORD)pData[3];
	m_SegmentLastSectionNumber = pData[4];
	m_LastTableID = pData[5];

	m_EventList.clear();
	WORD Pos = 6;
	for (int i = 0; Pos + 12 <= DataSize; i++) {
		m_EventList.push_back(EventInfo());

		EventInfo &Info = m_EventList[i];

		Info.EventID = ((WORD)pData[Pos + 0] << 8) | (WORD)pData[Pos + 1];
		Info.bValidStartTime = CAribTime::AribToSystemTime(&pData[Pos + 2], &Info.StartTime);
		Info.Duration = CAribTime::AribBcdToSecond(&pData[Pos + 7]);
		Info.RunningStatus = pData[Pos + 10] >> 5;
		Info.bFreeCaMode = (pData[Pos + 10] & 0x10) != 0;
		WORD DescLength = (((WORD)pData[Pos + 10] & 0x0F) << 8) | (WORD)pData[Pos + 11];
		if (DescLength > 0 && Pos + 12 + DescLength <= DataSize)
			Info.DescBlock.ParseBlock(&pData[Pos + 12], DescLength);
		Pos += 12 + DescLength;
	}

	return true;
}




// SYSTEMTIME の差(ms単位)を取得する
static LONGLONG DiffSystemTime(const SYSTEMTIME *pTime1, const SYSTEMTIME *pTime2)
{
	FILETIME ft1, ft2;

	::SystemTimeToFileTime(pTime1, &ft1);
	::SystemTimeToFileTime(pTime2, &ft2);
	return ((((LONGLONG)ft1.dwHighDateTime << 32) | (LONGLONG)ft1.dwLowDateTime) -
			(((LONGLONG)ft2.dwHighDateTime << 32) | (LONGLONG)ft2.dwLowDateTime)) / 10000LL;
}

// EIT schedule の時刻を取得する
static ULONGLONG GetScheduleTime(ULONGLONG CurTime, WORD TableID, BYTE SectionNumber)
{
	static const ULONGLONG HOUR = 60 * 60;

	return (CurTime / (24 * HOUR) * (24 * HOUR)) +
			((TableID & 0x07) * (4 * 24 * HOUR)) +
			((SectionNumber >> 3) * (3 * HOUR));
}




CEventManager::CEventManager(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1, 1)
{
	Reset();
}


CEventManager::~CEventManager()
{
#ifdef _DEBUG

#if 0
	// デバッグ用ダンプ出力
	{
		TCHAR szFileName[MAX_PATH];
		::GetModuleFileName(NULL, szFileName, MAX_PATH);
		::lstrcpy(::PathFindFileName(szFileName), TEXT("EpgData.txt"));

		HANDLE hFile=::CreateFile(szFileName,GENERIC_WRITE,0,NULL,
								  CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		if (hFile==INVALID_HANDLE_VALUE)
			return;

		TCHAR szBuff[2048];
		DWORD Length,Write;
#ifdef UNICODE
		static const WORD BOM = 0xFEFF;
		::WriteFile(hFile, &BOM, 2, &Write, NULL);
#endif
		ServiceMap::iterator itrService = m_ServiceMap.begin();
		for (size_t i = 0; itrService != m_ServiceMap.end(); ++itrService, i++) {
			Length=::wsprintf(szBuff, TEXT("[Service %d : NID %04X / TSID %04X / SID %04X / %lu Events]\r\n"),
							  (int)i+1,
							  itrService->second.Info.OriginalNetworkID,
							  itrService->second.Info.TransportStreamID,
							  itrService->second.Info.ServiceID,
							  (ULONG)itrService->second.EventMap.size());
			::WriteFile(hFile, szBuff, Length * sizeof(TCHAR), &Write, NULL);

			EventMap::iterator itrEvent = itrService->second.EventMap.begin();
			for (size_t j = 0; itrEvent != itrService->second.EventMap.end(); ++itrEvent, j++) {
				const SYSTEMTIME &st = itrEvent->second.GetStartTime();
				Length=::wsprintf(szBuff, TEXT("Event %3d %04X : %d/%02d/%02d %02d:%02d:%02d %5lu %s\r\n"),
								  (int)j+1,
								  itrEvent->second.GetEventID(),
								  st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
								  itrEvent->second.GetDuration(),
								  itrEvent->second.GetEventName() != NULL ? itrEvent->second.GetEventName() : TEXT(""));
				::WriteFile(hFile, szBuff, Length * sizeof(TCHAR), &Write, NULL);
			}
			::WriteFile(hFile, TEXT("\r\n"), 2 * sizeof(TCHAR), &Write, NULL);
		}
		::CloseHandle(hFile);
	}
#endif

#ifdef EVENTMANAGER_USE_UNORDERED_MAP
	// unordered_map の統計情報出力
	{
		TRACE(TEXT("Event map statistics\n"));

		for (ServiceMap::iterator itrService = m_ServiceMap.begin(); itrService != m_ServiceMap.end(); ++itrService) {
			const EventMap::size_type BucketCount = itrService->second.EventMap.bucket_count();
			EventMap::size_type MaxBucketSize = 0;
			EventMap::size_type EmptyBucketCount = 0;
			for (EventMap::size_type i = 0; i < BucketCount; i++) {
				const EventMap::size_type BucketSize = itrService->second.EventMap.bucket_size(i);
				if (BucketSize > MaxBucketSize)
					MaxBucketSize = BucketSize;
				if (BucketSize == 0)
					EmptyBucketCount++;
			}

			TRACE(TEXT("Service [%04x : %04x] : size %lu / bucket_count %lu / load_factor %f / Max elements %u / Empty buckets %lu\n"),
				  itrService->second.Info.OriginalNetworkID, itrService->second.Info.ServiceID,
				  (unsigned long)itrService->second.EventMap.size(),
				  (unsigned long)itrService->second.EventMap.bucket_count(),
				  itrService->second.EventMap.load_factor(),
				  (unsigned long)MaxBucketSize,
				  (unsigned long)EmptyBucketCount);
		}
	}
#endif

#endif	// _DEBUG
}


const bool CEventManager::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	/*
	if (dwInputIndex >= GetInputNum())
		return false;

	CTsPacket *pTsPacket = dynamic_cast<CTsPacket *>(pMediaData);

	// 入力メディアデータは互換性がない
	if (!pTsPacket)
		return false;
	*/

	CTsPacket *pTsPacket = static_cast<CTsPacket *>(pMediaData);

	// PIDルーティング
	m_PidMapManager.StorePacket(pTsPacket);

	// 次のフィルタにデータを渡す
	OutputMedia(pMediaData);

	return true;
}


void CEventManager::Reset()
{
	CBlockLock Lock(&m_DecoderLock);

	// 全テーブルアンマップ
	m_PidMapManager.UnmapAllTarget();

	// H-EITテーブルPIDマップ追加
	m_PidMapManager.MapTarget(PID_HEIT, new CEitParserTable(this));

	// M-EITテーブルPIDマップ追加
	//m_PidMapManager.MapTarget(PID_MEIT, new CEitParserTable(this));

	// L-EITテーブルPIDマップ追加
	m_PidMapManager.MapTarget(PID_LEIT, new CEitParserTable(this));

	// TOTテーブルPIDマップ追加
	m_PidMapManager.MapTarget(PID_TOT, new CTotTable, OnTotUpdated, this);

	::ZeroMemory(&m_CurTotTime, sizeof(SYSTEMTIME));
	m_CurTotSeconds = 0;
}


void CEventManager::Clear()
{
	CBlockLock Lock(&m_DecoderLock);

	m_ServiceMap.clear();
}


int CEventManager::GetServiceNum()
{
	CBlockLock Lock(&m_DecoderLock);

	return (int)m_ServiceMap.size();
}


bool CEventManager::GetServiceList(ServiceList *pList)
{
	if (!pList)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	pList->resize(m_ServiceMap.size());
	ServiceMap::iterator itr = m_ServiceMap.begin();
	for (size_t i = 0; itr != m_ServiceMap.end(); i++, ++itr) {
		(*pList)[i] = itr->second.Info;
	}

	return true;
}


bool CEventManager::IsServiceUpdated(const WORD NetworkID, const WORD TransportStreamID, const WORD ServiceID)
{
	CBlockLock Lock(&m_DecoderLock);

	ServiceMapKey Key = GetServiceMapKey(NetworkID, TransportStreamID, ServiceID);
	ServiceMap::iterator itrService = m_ServiceMap.find(Key);
	if (itrService == m_ServiceMap.end())
		return false;
	return itrService->second.bUpdated;
}


static bool IsEventValid(const CEventInfo &Event)
{
	return !Event.m_EventName.empty() || Event.m_bCommonEvent;
}

bool CEventManager::GetEventList(const WORD NetworkID, const WORD TransportStreamID, const WORD ServiceID,
								 EventList *pList, TimeEventMap *pTimeEventMap)
{
	if (!pList)
		return false;

	pList->clear();

	CBlockLock Lock(&m_DecoderLock);

	ServiceMapKey Key = GetServiceMapKey(NetworkID, TransportStreamID, ServiceID);
	ServiceMap::iterator itrService = m_ServiceMap.find(Key);
	if (itrService == m_ServiceMap.end())
		return false;

	const ServiceEventMap &Service = itrService->second;

	pList->reserve(Service.EventMap.size());

	if (pTimeEventMap) {
		pTimeEventMap->clear();
		for (auto itrTime = Service.TimeMap.begin(); itrTime != Service.TimeMap.end(); ++itrTime) {
			auto itrEvent = Service.EventMap.find(itrTime->EventID);
			if (itrEvent != Service.EventMap.end()
					&& IsEventValid(itrEvent->second)) {
				pList->push_back(itrEvent->second);
				pTimeEventMap->insert(*itrTime);
			}
		}
	} else {
		for (auto itrEvent = Service.EventMap.begin(); itrEvent != Service.EventMap.end(); ++itrEvent) {
			if (IsEventValid(itrEvent->second))
				pList->push_back(itrEvent->second);
		}
	}

	itrService->second.bUpdated = false;

	return true;
}


bool CEventManager::GetEventInfo(const WORD NetworkID, const WORD TransportStreamID, const WORD ServiceID,
								 const WORD EventID, CEventInfo *pInfo)
{
	if (!pInfo)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	ServiceMap::iterator itrService =
		m_ServiceMap.find(GetServiceMapKey(NetworkID, TransportStreamID, ServiceID));
	if (itrService != m_ServiceMap.end()) {
		EventMap::iterator itrEvent = itrService->second.EventMap.find(EventID);
		if (itrEvent != itrService->second.EventMap.end()
				&& IsEventValid(itrEvent->second)) {
			*pInfo = itrEvent->second;
			return true;
		}
	}

	return false;
}


bool CEventManager::GetEventInfo(const WORD NetworkID, const WORD TransportStreamID, const WORD ServiceID,
								 const SYSTEMTIME *pTime, CEventInfo *pInfo)
{
	if (!pTime || !pInfo)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	bool bFound = false;
	ServiceMap::iterator itrService =
		m_ServiceMap.find(GetServiceMapKey(NetworkID, TransportStreamID, ServiceID));
	if (itrService != m_ServiceMap.end()) {
		TimeEventInfo Key(*pTime);
		TimeEventMap::iterator itrTime = itrService->second.TimeMap.upper_bound(Key);
		if (itrTime != itrService->second.TimeMap.begin()) {
			--itrTime;
			if (itrTime->StartTime + itrTime->Duration > Key.StartTime) {
				EventMap::iterator itrEvent = itrService->second.EventMap.find(itrTime->EventID);
				if (itrEvent != itrService->second.EventMap.end()
						&& IsEventValid(itrEvent->second)) {
					*pInfo = itrEvent->second;
					bFound = true;
				}
			}
		}
	}

	return bFound;
}


bool CEventManager::IsScheduleComplete(const WORD NetworkID, const WORD TransportStreamID, const WORD ServiceID, const bool bExtended)
{
	CBlockLock Lock(&m_DecoderLock);

	ServiceMap::iterator itrService =
		m_ServiceMap.find(GetServiceMapKey(NetworkID, TransportStreamID, ServiceID));
	if (itrService == m_ServiceMap.end())
		return false;

	return itrService->second.Schedule.IsComplete(m_CurTotTime.wHour, bExtended);
}


bool CEventManager::HasSchedule(const WORD NetworkID, const WORD TransportStreamID, const WORD ServiceID, const bool bExtended)
{
	CBlockLock Lock(&m_DecoderLock);

	ServiceMap::iterator itrService =
		m_ServiceMap.find(GetServiceMapKey(NetworkID, TransportStreamID, ServiceID));
	if (itrService == m_ServiceMap.end())
		return false;

	return itrService->second.Schedule.HasSchedule(bExtended);
}


bool CEventManager::RemoveEvent(EventMap *pMap, const WORD EventID)
{
	EventMap::iterator itr = pMap->find(EventID);
	if (itr == pMap->end())
		return false;
	pMap->erase(itr);
	TRACE(TEXT("CEventManager::RemoveEvent() : Event removed [%04x]\n"), EventID);
	return true;
}


void CEventManager::OnSection(CPsiStreamTable *pTable, const CPsiSection *pSection)
{
	const CEitParserTable *pEitTable = dynamic_cast<const CEitParserTable *>(pTable);

	ServiceMapKey Key = GetServiceMapKey(pEitTable->GetOriginalNetworkID(),
										 pEitTable->GetTransportStreamID(),
										 pEitTable->GetServiceID());
	ServiceMap::iterator itrService = m_ServiceMap.find(Key);
	if (itrService == m_ServiceMap.end()) {
		ServiceEventMap EventMap;
		EventMap.Info.OriginalNetworkID = pEitTable->GetOriginalNetworkID();
		EventMap.Info.TransportStreamID = pEitTable->GetTransportStreamID();
		EventMap.Info.ServiceID = pEitTable->GetServiceID();
		EventMap.bUpdated = false;
		EventMap.ScheduleUpdatedTime = m_CurTotTime;

		itrService = m_ServiceMap.insert(std::pair<ServiceMapKey, ServiceEventMap>(Key, EventMap)).first;
#ifdef EVENTMANAGER_USE_UNORDERED_MAP
		itrService->second.EventMap.rehash(300);
#endif
	}

	ServiceEventMap &Service = itrService->second;

	SYSTEMTIME CurSysTime;
	GetCurrentEpgTime(&CurSysTime);

	const ULONGLONG CurTotTime = m_CurTotSeconds;
	bool bUpdated = false;

	const WORD TableID = pSection->GetTableID();
	const int NumEvents = pEitTable->GetEventNum();

	if (NumEvents > 0) {
		const WORD NetworkID = pEitTable->GetOriginalNetworkID();
		const WORD TransportStreamID = pEitTable->GetTransportStreamID();
		const WORD ServiceID = pEitTable->GetServiceID();

		for (int i = 0; i < NumEvents; i++) {
			const CEitParserTable::EventInfo *pEventInfo = pEitTable->GetEventInfo(i);

			// 開始/終了時刻が未定義のものは除外する
			if (!pEventInfo->bValidStartTime || pEventInfo->Duration == 0)
				continue;

			// 既に終了しているものは除外する
			// (時計のずれを考えて3分マージンをとっている)
			SYSTEMTIME EndTime;
			{
				CDateTime Time(pEventInfo->StartTime);
				if (!Time.Offset(CDateTime::SECONDS(pEventInfo->Duration)))
					continue;
				Time.Get(&EndTime);
			}
			if (DiffSystemTime(&EndTime, &CurSysTime) <= -3*60*1000)
				continue;

			TimeEventInfo TimeEvent(pEventInfo->StartTime);
			TimeEvent.Duration = pEventInfo->Duration;
			TimeEvent.EventID = pEventInfo->EventID;
			TimeEvent.UpdateTime = CurTotTime;
			std::pair<TimeEventMap::iterator, bool> TimeResult = Service.TimeMap.insert(TimeEvent);
			TimeEventMap::iterator itrCur = TimeResult.first;
			if (TimeResult.second
					|| itrCur->Duration != TimeEvent.Duration
					|| itrCur->EventID != TimeEvent.EventID) {
				if (!TimeResult.second) {
					// 既存のデータの方が新しい場合は除外する
					if (itrCur->UpdateTime > TimeEvent.UpdateTime)
						continue;
				}

				// 時間が被っていないか調べる
				bool bSkip = false;
				TimeEventMap::iterator itr = itrCur;
				for (++itr; itr != Service.TimeMap.end();) {
					if (itr->StartTime >= TimeEvent.StartTime + TimeEvent.Duration)
						break;
					if (itr->UpdateTime > TimeEvent.UpdateTime) {
						bSkip = true;
						break;
					}
					RemoveEvent(&Service.EventMap, itr->EventID);
					Service.TimeMap.erase(itr++);
					bUpdated = true;
				}
				if (!bSkip && itrCur != Service.TimeMap.begin()) {
					itr = itrCur;
					--itr;
					while (true) {
						if (itr->StartTime + itr->Duration <= TimeEvent.StartTime)
							break;
						if (itr->UpdateTime > TimeEvent.UpdateTime) {
							bSkip = true;
							break;
						}
						RemoveEvent(&Service.EventMap, itr->EventID);
						bUpdated = true;
						if (itr == Service.TimeMap.begin()) {
							Service.TimeMap.erase(itr);
							break;
						}
						Service.TimeMap.erase(itr--);
					}
				}
				if (bSkip) {
					if (TimeResult.second)
						Service.TimeMap.erase(itrCur);
					TRACE(TEXT("CEventManager::OnEitUpdated() : Invalid time range\n"));
					continue;
				}
				if (!TimeResult.second) {
					if (itrCur->EventID != TimeEvent.EventID)
						RemoveEvent(&Service.EventMap, itrCur->EventID);
				}
			}
			if (!TimeResult.second) {
				Service.TimeMap.erase(itrCur);
				Service.TimeMap.insert(TimeEvent);
			}

			// イベントを追加 or 既存のイベントを取得
			std::pair<EventMap::iterator, bool> EventResult =
				Service.EventMap.insert(std::pair<WORD, CEventInfo>(pEventInfo->EventID, CEventInfo()));
			CEventInfo *pEvent = &EventResult.first->second;
			if (!EventResult.second) {
				if (pEvent->m_UpdatedTime > CurTotTime)
					continue;

				if (DiffSystemTime(&pEvent->m_StartTime, &pEventInfo->StartTime) != 0) {
					// 開始時刻が変わった
					TimeEventInfo Key(pEvent->m_StartTime);
					TimeEventMap::iterator itr = Service.TimeMap.find(Key);
					if (itr != Service.TimeMap.end()
							&& itr->EventID == pEventInfo->EventID)
						Service.TimeMap.erase(itr);
				}
			}

			pEvent->m_UpdatedTime = CurTotTime;
			pEvent->m_NetworkID = NetworkID;
			pEvent->m_TransportStreamID = TransportStreamID;
			pEvent->m_ServiceID = ServiceID;
			pEvent->m_EventID = pEventInfo->EventID;
			pEvent->m_bValidStartTime = true;
			pEvent->m_StartTime = pEventInfo->StartTime;
			pEvent->m_Duration = pEventInfo->Duration;
			pEvent->m_RunningStatus = pEventInfo->RunningStatus;
			pEvent->m_bFreeCaMode = pEventInfo->bFreeCaMode;

			if (TableID == 0x4E || TableID == 0x4F) {
				pEvent->m_Type = CEventInfo::TYPE_BASIC | CEventInfo::TYPE_EXTENDED
					| (TableID == 0x4E ? CEventInfo::TYPE_PRESENT : CEventInfo::TYPE_FOLLOWING);
			} else {
				if ((TableID & 0x0F) <= 0x07) {
					pEvent->m_Type |= CEventInfo::TYPE_BASIC;
				} else {
					pEvent->m_Type |= CEventInfo::TYPE_EXTENDED;
				}
				pEvent->m_Type &= ~(CEventInfo::TYPE_PRESENT | CEventInfo::TYPE_FOLLOWING);
			}

			const CDescBlock *pDescBlock = &pEventInfo->DescBlock;
			int Length;
			TCHAR szText[2048];

			const CShortEventDesc *pShortEvent = pDescBlock->GetDesc<CShortEventDesc>();
			if (pShortEvent) {
				Length = (int)pShortEvent->GetEventName(szText, _countof(szText));
				if (Length > 0)
					pEvent->m_EventName = szText;
				Length = (int)pShortEvent->GetEventDesc(szText, _countof(szText));
				if (Length > 0)
					pEvent->m_EventText = szText;
			}

			Length = GetEventExtendedText(pDescBlock, szText, _countof(szText));
			if (Length > 0)
				pEvent->m_EventExtendedText = szText;

			if (pDescBlock->GetDescByTag(CComponentDesc::DESC_TAG) != NULL) {
				pEvent->m_VideoList.clear();
				for (WORD j = 0; j < pDescBlock->GetDescNum(); j++) {
					const CBaseDesc *pDesc = pDescBlock->GetDescByIndex(j);
					if (pDesc->GetTag() == CComponentDesc::DESC_TAG) {
						const CComponentDesc *pComponentDesc =
							dynamic_cast<const CComponentDesc*>(pDesc);
						if (pComponentDesc) {
							CEventInfo::VideoInfo Info;

							Info.StreamContent = pComponentDesc->GetStreamContent();
							Info.ComponentType = pComponentDesc->GetComponentType();
							Info.ComponentTag = pComponentDesc->GetComponentTag();
							Info.LanguageCode = pComponentDesc->GetLanguageCode();
							Info.szText[0] = _T('\0');
							pComponentDesc->GetText(Info.szText, CEventInfo::VideoInfo::MAX_TEXT);
							pEvent->m_VideoList.push_back(Info);
						}
					}
				}
			}

			if (pDescBlock->GetDescByTag(CAudioComponentDesc::DESC_TAG) != NULL) {
				pEvent->m_AudioList.clear();
				for (WORD j = 0; j < pDescBlock->GetDescNum(); j++) {
					const CBaseDesc *pDesc = pDescBlock->GetDescByIndex(j);
					if (pDesc->GetTag() == CAudioComponentDesc::DESC_TAG) {
						const CAudioComponentDesc *pAudioDesc =
							dynamic_cast<const CAudioComponentDesc*>(pDesc);
						if (pAudioDesc) {
							CEventInfo::AudioInfo Info;

							Info.StreamContent = pAudioDesc->GetStreamContent();
							Info.ComponentType = pAudioDesc->GetComponentType();
							Info.ComponentTag = pAudioDesc->GetComponentTag();
							Info.SimulcastGroupTag = pAudioDesc->GetSimulcastGroupTag();
							Info.bESMultiLingualFlag = pAudioDesc->GetESMultiLingualFlag();
							Info.bMainComponentFlag = pAudioDesc->GetMainComponentFlag();
							Info.QualityIndicator = pAudioDesc->GetQualityIndicator();
							Info.SamplingRate = pAudioDesc->GetSamplingRate();
							Info.LanguageCode = pAudioDesc->GetLanguageCode();
							Info.LanguageCode2 = pAudioDesc->GetLanguageCode2();
							Info.szText[0] = _T('\0');
							pAudioDesc->GetText(Info.szText, CEventInfo::AudioInfo::MAX_TEXT);
							pEvent->m_AudioList.push_back(Info);
						}
					}
				}
			}

			const CContentDesc *pContentDesc = pDescBlock->GetDesc<CContentDesc>();
			if (pContentDesc) {
				int NibbleCount = pContentDesc->GetNibbleCount();
				if (NibbleCount > 7)
					NibbleCount = 7;
				pEvent->m_ContentNibble.NibbleCount = NibbleCount;
				for (int j = 0; j < NibbleCount; j++)
					pContentDesc->GetNibble(j, &pEvent->m_ContentNibble.NibbleList[j]);
			}

			const CEventGroupDesc *pGroupDesc = pDescBlock->GetDesc<CEventGroupDesc>();
			if (pGroupDesc) {
				pEvent->m_EventGroupList.resize(1);
				CEventInfo::EventGroupInfo &GroupInfo = pEvent->m_EventGroupList[0];

				GroupInfo.GroupType = pGroupDesc->GetGroupType();
				const int NumEvents = pGroupDesc->GetEventNum();
				GroupInfo.EventList.resize(NumEvents);
				for (int j = 0; j < NumEvents; j++)
					pGroupDesc->GetEventInfo(j, &GroupInfo.EventList[j]);

				if (GroupInfo.GroupType == CEventGroupDesc::GROUPTYPE_COMMON
						&& NumEvents == 1) {
					const CEventGroupDesc::EventInfo &Info = GroupInfo.EventList[0];
					if (Info.ServiceID != pEitTable->GetServiceID()) {
						pEvent->m_bCommonEvent = true;
						pEvent->m_CommonEventInfo.ServiceID = Info.ServiceID;
						pEvent->m_CommonEventInfo.EventID = Info.EventID;
					}
				}
			}

			bUpdated = true;
		}
	} else {
		// イベントが消滅している場合は削除する
		if (m_CurTotTime.wHour > 0 || m_CurTotTime.wMinute > 0 || m_CurTotTime.wSecond >= 30) {
			if ((TableID >= 0x50 && TableID <= 0x57) || (TableID >= 0x60 && TableID <= 0x67)) {
				// Schedule basic
				const ULONGLONG Time = GetScheduleTime(CurTotTime, TableID, pSection->GetSectionNumber());
				TimeEventMap::iterator itr = Service.TimeMap.lower_bound(TimeEventInfo(Time));
				while (itr != Service.TimeMap.end()) {
					if (itr->StartTime < Time || itr->StartTime >= Time + (3 * 60 * 60)
							|| itr->UpdateTime >= CurTotTime)
						break;
					RemoveEvent(&Service.EventMap, itr->EventID);
					Service.TimeMap.erase(itr++);
					bUpdated = true;
				}
			}
		}
	}

	if (bUpdated)
		Service.bUpdated = true;

	if (m_CurTotTime.wHour > 0 || m_CurTotTime.wMinute > 0 || m_CurTotTime.wSecond >= 30) {
		if (Service.ScheduleUpdatedTime.wYear != m_CurTotTime.wYear
				|| Service.ScheduleUpdatedTime.wMonth != m_CurTotTime.wMonth
				|| Service.ScheduleUpdatedTime.wDay != m_CurTotTime.wDay)
			Service.Schedule.Clear();

		if (Service.Schedule.OnSection(pEitTable, pSection))
			Service.ScheduleUpdatedTime = m_CurTotTime;
	}
}


void CALLBACK CEventManager::OnTotUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	CTotTable *pTotTable = dynamic_cast<CTotTable*>(pMapTarget);

	if (pTotTable) {
		SYSTEMTIME st;

		if (pTotTable->GetDateTime(&st)) {
			CEventManager *pThis = static_cast<CEventManager*>(pParam);

			pThis->m_CurTotTime = st;
			pThis->m_CurTotSeconds = SystemTimeToSeconds(st);
		}
	}
}




CEventManager::TimeEventInfo::TimeEventInfo(ULONGLONG Time)
	: StartTime(Time)
{
}


CEventManager::TimeEventInfo::TimeEventInfo(const SYSTEMTIME &StartTime)
	: StartTime(SystemTimeToSeconds(StartTime))
{
}


CEventManager::TimeEventInfo::TimeEventInfo(const CEventInfo &Info)
	: StartTime(SystemTimeToSeconds(Info.m_StartTime))
	, Duration(Info.m_Duration)
	, EventID(Info.m_EventID)
	, UpdateTime(Info.m_UpdatedTime)
{
}




CEventManager::CScheduleInfo::CScheduleInfo()
{
	Clear();
}


void CEventManager::CScheduleInfo::Clear()
{
	m_Basic.TableNum = 0;
	m_Extended.TableNum = 0;
}


bool CEventManager::CScheduleInfo::IsComplete(int Hour, bool bExtended) const
{
	const TableList &TableList = bExtended ? m_Extended : m_Basic;

	if (TableList.TableNum == 0)
		return false;

	for (int i = 0; i < TableList.TableNum; i++) {
		if (!IsTableComplete(i, Hour, bExtended))
			return false;
	}

	return true;
}


bool CEventManager::CScheduleInfo::IsTableComplete(int Table, int Hour, bool bExtended) const
{
	const TableList &TableList = bExtended ? m_Extended : m_Basic;

	if (Table < 0 || Table >= TableList.TableNum
			|| (Table == 0 && Hour < 0))
		return false;

	const TableInfo &TableInfo = TableList.Table[Table];

	for (int i = ((Table == 0) ? (Hour / 3) : 0); i < 32; i++) {
		const SegmentInfo &Segment = TableInfo.SegmentList[i];

		if (Segment.SectionNum == 0)
			return false;

		if (Segment.Flags != (1 << Segment.SectionNum) - 1)
			return false;
	}

	return true;
}


bool CEventManager::CScheduleInfo::HasSchedule(const bool bExtended) const
{
	return (bExtended ? m_Extended : m_Basic).TableNum > 0;
}


bool CEventManager::CScheduleInfo::OnSection(const CEitParserTable *pTable, const CPsiSection *pSection)
{
	const BYTE TableID = pSection->GetTableID();
	const BYTE LastTableID = pTable->GetLastTableID();
	const BYTE SectionNumber = pSection->GetSectionNumber();
	const BYTE LastSectionNumber = pTable->GetSegmentLastSectionNumber();

	if (TableID > LastTableID || SectionNumber > LastSectionNumber
			|| TableID < 0x50 || TableID > 0x6F) {
		return false;
	}

/*
	TRACE(TEXT("H-EIT : SID %04x / table_id %02x / last_table_id %02x / section_number %02x / last_section_number %02x\n"),
		  pTable->GetServiceID(), TableID, LastTableID, SectionNumber, LastSectionNumber);
*/

	TableList *pTableList;
	if ((TableID & 0x0F) <= 0x07) {
		pTableList = &m_Basic;
	} else {
		pTableList = &m_Extended;
	}

	const BYTE FirstTableID = TableID & 0xF8;
	const BYTE NewTableNum = (LastTableID - FirstTableID) + 1;
	TableInfo &TableInfo = pTableList->Table[TableID & 0x07];

	if (pTableList->TableNum != NewTableNum) {
		pTableList->TableNum = NewTableNum;
		::ZeroMemory(pTableList->Table, sizeof(pTableList->Table));
		TableInfo.Version = pSection->GetVersionNo();
	} else if (pSection->GetVersionNo() != TableInfo.Version) {
		TableInfo.Version = pSection->GetVersionNo();
		::ZeroMemory(TableInfo.SegmentList, sizeof(TableInfo.SegmentList));
	}

	SegmentInfo &Segment = TableInfo.SegmentList[SectionNumber >> 3];

	Segment.SectionNum = (LastSectionNumber - (LastSectionNumber & 0xF8)) + 1;
	Segment.Flags |= 1 << (SectionNumber & 0x07);

	return true;
}
