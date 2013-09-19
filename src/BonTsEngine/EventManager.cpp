#include "stdafx.h"
#include "Common.h"
#include "EventManager.h"
#include "TsEncode.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


/////////////////////////////////////////////////////////////////////////////
// EITテーブル解析クラス
/////////////////////////////////////////////////////////////////////////////

class CEitTable : public CPsiStreamTable
{
public:
	CEitTable(ISectionHandler *pHandler = NULL);
	CEitTable(const CEitTable &Operand);

	CEitTable & operator = (const CEitTable &Operand);

// CPsiStreamTable
	virtual void Reset(void);

// CEitTable
	struct EventInfo {
		WORD EventID;
		bool bValidStartTime;
		SYSTEMTIME StartTime;
		DWORD Duration;
		BYTE RunningStatus;
		bool bFreeCaMode;
		CDescBlock DescBlock;
	};

	const WORD GetServiceID() const;
	const WORD GetTransportStreamID() const;
	const WORD GetOriginalNetworkID() const;
	const BYTE GetSegmentLastSectionNumber() const;
	const BYTE GetLastTableID() const;
	const int GetEventNum() const;
	const EventInfo * GetEventInfo(const int Index) const;
	const WORD GetEventID(const int Index) const;
	const SYSTEMTIME * GetStartTime(const int Index) const;
	const DWORD GetDuration(const int Index) const;
	const BYTE GetRunningStatus(const int Index) const;
	const bool GetFreeCaMode(const int Index) const;
	const CDescBlock * GetItemDesc(const int Index) const;

protected:
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection);

	WORD m_ServiceID;
	WORD m_TransportStreamID;
	WORD m_OriginalNetworkID;
	BYTE m_SegmentLastSectionNumber;
	BYTE m_LastTableID;

	std::vector<EventInfo> m_EventList;
};

CEitTable::CEitTable(ISectionHandler *pHandler)
	: CPsiStreamTable(pHandler)
{
	Reset();
}

CEitTable::CEitTable(const CEitTable &Operand)
{
	*this = Operand;
}

CEitTable & CEitTable::operator = (const CEitTable &Operand)
{
	if (this != &Operand) {
		CPsiStreamTable::operator = (Operand);
		m_ServiceID = Operand.m_ServiceID;
		m_TransportStreamID = Operand.m_TransportStreamID;
		m_OriginalNetworkID = Operand.m_OriginalNetworkID;
		m_EventList = Operand.m_EventList;
	}

	return *this;
}

void CEitTable::Reset(void)
{
	// 状態をクリアする
	CPsiStreamTable::Reset();

	m_ServiceID = 0;
	m_TransportStreamID = 0;
	m_OriginalNetworkID = 0;
	m_EventList.clear();
}

const WORD CEitTable::GetServiceID() const
{
	return m_ServiceID;
}

const WORD CEitTable::GetTransportStreamID() const
{
	return m_TransportStreamID;
}

const WORD CEitTable::GetOriginalNetworkID() const
{
	return m_OriginalNetworkID;
}


const BYTE CEitTable::GetSegmentLastSectionNumber() const
{
	return m_SegmentLastSectionNumber;
}


const BYTE CEitTable::GetLastTableID() const
{
	return m_LastTableID;
}


const int CEitTable::GetEventNum() const
{
	return (int)m_EventList.size();
}

const CEitTable::EventInfo * CEitTable::GetEventInfo(const int Index) const
{
	if (Index < 0 || (size_t)Index >= m_EventList.size())
		return NULL;
	return &m_EventList[Index];
}

const WORD CEitTable::GetEventID(const int Index) const
{
	if (Index < 0 || (size_t)Index >= m_EventList.size())
		return 0;
	return m_EventList[Index].EventID;
}

const SYSTEMTIME * CEitTable::GetStartTime(const int Index) const
{
	if (Index < 0 || (size_t)Index >= m_EventList.size())
		return NULL;
	return &m_EventList[Index].StartTime;
}

const DWORD CEitTable::GetDuration(const int Index) const
{
	if (Index < 0 || (size_t)Index >= m_EventList.size())
		return 0;
	return m_EventList[Index].Duration;
}

const BYTE CEitTable::GetRunningStatus(const int Index) const
{
	if (Index < 0 || (size_t)Index >= m_EventList.size())
		return 0;
	return m_EventList[Index].RunningStatus;
}

const bool CEitTable::GetFreeCaMode(const int Index) const
{
	// Free CA Modeを返す
	if (Index < 0 || (size_t)Index >= m_EventList.size())
		return 0;
	return m_EventList[Index].bFreeCaMode;
}

const CDescBlock * CEitTable::GetItemDesc(const int Index) const
{
	// アイテムの記述子ブロックを返す
	if (Index < 0 || (size_t)Index >= m_EventList.size())
		return NULL;
	return &m_EventList[Index].DescBlock;
}

const bool CEitTable::OnTableUpdate(const CPsiSection *pCurSection)
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

// 拡張テキストを取得する
static int GetExtendedEventText(const CDescBlock *pDescBlock, LPTSTR pszText, int MaxLength)
{
	pszText[0] = '\0';

	std::vector<const CExtendedEventDesc *> DescList;
	for (int i = 0; i < pDescBlock->GetDescNum(); i++) {
		const CBaseDesc *pDesc = pDescBlock->GetDescByIndex(i);
		if (pDesc != NULL && pDesc->GetTag() == CExtendedEventDesc::DESC_TAG) {
			const CExtendedEventDesc *pExtendedEvent = dynamic_cast<const CExtendedEventDesc *>(pDesc);
			if (pExtendedEvent != NULL) {
				DescList.push_back(pExtendedEvent);
			}
		}
	}
	if (DescList.size() == 0)
		return 0;

	// descriptor_number 順にソートする
	for (int i = (int)DescList.size() - 2; i >= 0; i--) {
		const CExtendedEventDesc *pKey = DescList[i];
		int j;
		for (j = i + 1; j < (int)DescList.size() && DescList[j]->GetDescriptorNumber() < pKey->GetDescriptorNumber(); j++)
			DescList[j - 1] = DescList[j];
		DescList[j - 1] = pKey;
	}

	struct ItemInfo {
		BYTE DescriptorNumber;
		LPCTSTR pszDescription;
		int Data1Length;
		const BYTE *pData1;
		int Data2Length;
		const BYTE *pData2;
	};
	std::vector<ItemInfo> ItemList;
	for (int i = 0; i < (int)DescList.size(); i++) {
		const CExtendedEventDesc *pExtendedEvent = DescList[i];
		for (int j = 0; j < pExtendedEvent->GetItemCount(); j++) {
			const CExtendedEventDesc::ItemInfo *pItem = pExtendedEvent->GetItem(j);
			if (pItem == NULL)
				continue;
			if (pItem->szDescription[0] != '\0') {
				// 新規項目
				ItemInfo Item;
				Item.DescriptorNumber = pExtendedEvent->GetDescriptorNumber();
				Item.pszDescription = pItem->szDescription;
				Item.Data1Length = pItem->ItemLength;
				Item.pData1 = pItem->ItemChar;
				Item.Data2Length = 0;
				Item.pData2 = NULL;
				ItemList.push_back(Item);
			} else if (ItemList.size() > 0) {
				// 前の項目の続き
				ItemInfo &Item = ItemList[ItemList.size() - 1];
				if (Item.DescriptorNumber == pExtendedEvent->GetDescriptorNumber() - 1
						&& Item.pData2 == NULL) {
					Item.Data2Length = pItem->ItemLength;
					Item.pData2 = pItem->ItemChar;
				}
			}
		}
	}

	TCHAR szText[1024];
	int Length;
	int Pos = 0;
	for (int i = 0; i < (int)ItemList.size(); i++) {
		ItemInfo &Item = ItemList[i];
		Length = ::lstrlen(Item.pszDescription);
		if (Length + 2 >= MaxLength - Pos)
			break;
		::lstrcpy(&pszText[Pos], Item.pszDescription);
		Pos += Length;
		pszText[Pos++] = '\r';
		pszText[Pos++] = '\n';
		if (Item.pData2 == NULL) {
			CAribString::AribToString(szText, 1024, Item.pData1, Item.Data1Length);
		} else {
			BYTE Buffer[220 * 2];
			::CopyMemory(Buffer, Item.pData1, Item.Data1Length);
			::CopyMemory(Buffer + Item.Data1Length, Item.pData2, Item.Data2Length);
			CAribString::AribToString(szText, 1024, Buffer, Item.Data1Length + Item.Data2Length);
		}
		LPTSTR p = szText;
		while (*p != '\0') {
			if (Pos >= MaxLength - 1)
				break;
			pszText[Pos++] = *p;
			if (*p == '\r') {
				if (*(p + 1) != '\n') {
					if (Pos == MaxLength - 1)
						break;
					pszText[Pos++] = '\n';
				}
			}
			p++;
		}
		if (Pos + 2 >= MaxLength)
			break;
		pszText[Pos++] = '\r';
		pszText[Pos++] = '\n';
	}
	pszText[Pos] = '\0';

	return Pos;
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
	m_PidMapManager.MapTarget(PID_HEIT, new CEitTable(this));

	// M-EITテーブルPIDマップ追加
	//m_PidMapManager.MapTarget(PID_MEIT, new CEitTable(this));

	// L-EITテーブルPIDマップ追加
	m_PidMapManager.MapTarget(PID_LEIT, new CEitTable(this));

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


static bool IsEventValid(const CEventManager::CEventInfo &Event)
{
	return Event.GetEventName() != NULL || Event.IsCommonEvent();
}

bool CEventManager::GetEventList(const WORD NetworkID, const WORD TransportStreamID, const WORD ServiceID, EventList *pList)
{
	if (!pList)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	ServiceMapKey Key = GetServiceMapKey(NetworkID, TransportStreamID, ServiceID);
	ServiceMap::iterator itrService = m_ServiceMap.find(Key);
	if (itrService == m_ServiceMap.end()) {
		pList->clear();
		return false;
	}

	pList->resize(itrService->second.EventMap.size());
	EventMap::iterator itrEvent = itrService->second.EventMap.begin();
	size_t i;
	for (i = 0; itrEvent != itrService->second.EventMap.end(); ++itrEvent) {
		if (IsEventValid(itrEvent->second))
			(*pList)[i++] = itrEvent->second;
	}
	if (i < pList->size())
		pList->resize(i);

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
	const CEitTable *pEitTable = dynamic_cast<const CEitTable *>(pTable);

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
#if 0
	::GetLocalTime(&CurSysTime);
#else
	// UTC+9の現在日時を取得
	FILETIME ft;
	ULARGE_INTEGER uli;
	::GetSystemTimeAsFileTime(&ft);
	uli.LowPart  = ft.dwLowDateTime;
	uli.HighPart = ft.dwHighDateTime;
	uli.QuadPart += 9LL * 60LL * 60LL * 10000000LL;
	ft.dwLowDateTime  = uli.LowPart;
	ft.dwHighDateTime = uli.HighPart;
	::FileTimeToSystemTime(&ft, &CurSysTime);
#endif

	const ULONGLONG CurTotTime = m_CurTotSeconds;
	bool bUpdated = false;

	const WORD TableID = pSection->GetTableID();
	const int NumEvents = pEitTable->GetEventNum();

	if (NumEvents > 0) {
		for (int i = 0; i < NumEvents; i++) {
			const CEitTable::EventInfo *pEventInfo = pEitTable->GetEventInfo(i);

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
				if (pEvent->m_UpdateTime > CurTotTime)
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

			pEvent->m_UpdateTime = CurTotTime;
			pEvent->m_EventID = pEventInfo->EventID;
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

			const CShortEventDesc *pShortEvent =
				dynamic_cast<const CShortEventDesc*>(pDescBlock->GetDescByTag(CShortEventDesc::DESC_TAG));
			if (pShortEvent) {
				Length = (int)pShortEvent->GetEventName(szText, _countof(szText));
				if (Length > 0)
					pEvent->SetEventName(szText);
				Length = (int)pShortEvent->GetEventDesc(szText, _countof(szText));
				if (Length > 0)
					pEvent->SetEventText(szText);
			}

			Length = GetExtendedEventText(pDescBlock, szText, _countof(szText));
			if (Length > 0)
				pEvent->SetEventExtendedText(szText);

			const CComponentDesc *pComponentDesc =
				dynamic_cast<const CComponentDesc*>(pDescBlock->GetDescByTag(CComponentDesc::DESC_TAG));
			if (pComponentDesc) {
				pEvent->m_VideoInfo.StreamContent = pComponentDesc->GetStreamContent();
				pEvent->m_VideoInfo.ComponentType = pComponentDesc->GetComponentType();
				pEvent->m_VideoInfo.ComponentTag = pComponentDesc->GetComponentTag();
				pEvent->m_VideoInfo.LanguageCode = pComponentDesc->GetLanguageCode();
				pEvent->m_VideoInfo.szText[0] = _T('\0');
				pComponentDesc->GetText(pEvent->m_VideoInfo.szText, CEventInfo::VideoInfo::MAX_TEXT);
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

			const CContentDesc *pContentDesc =
				dynamic_cast<const CContentDesc *>(pDescBlock->GetDescByTag(CContentDesc::DESC_TAG));
			if (pContentDesc) {
				int NibbleCount = pContentDesc->GetNibbleCount();
				if (NibbleCount > 7)
					NibbleCount = 7;
				pEvent->m_ContentNibble.NibbleCount = NibbleCount;
				for (int i = 0; i < NibbleCount; i++)
					pContentDesc->GetNibble(i, &pEvent->m_ContentNibble.NibbleList[i]);
			}

			const CEventGroupDesc *pGroupDesc =
				dynamic_cast<const CEventGroupDesc*>(pDescBlock->GetDescByTag(CEventGroupDesc::DESC_TAG));
			if (pGroupDesc) {
				pEvent->m_EventGroupList.resize(1);
				CEventInfo::EventGroupInfo &GroupInfo = pEvent->m_EventGroupList[0];

				GroupInfo.GroupType = pGroupDesc->GetGroupType();
				const int NumEvents = pGroupDesc->GetEventNum();
				GroupInfo.EventList.resize(NumEvents);
				for (int i = 0; i < NumEvents; i++)
					pGroupDesc->GetEventInfo(i, &GroupInfo.EventList[i]);

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




CEventManager::CEventInfo::CEventInfo()
	: m_UpdateTime(0)
	, m_Type(0)
	, m_pszEventName(NULL)
	, m_pszEventText(NULL)
	, m_pszEventExtendedText(NULL)
	, m_bCommonEvent(false)
{
	m_ContentNibble.NibbleCount = 0;
}


CEventManager::CEventInfo::CEventInfo(const CEventInfo &Src)
	: m_pszEventName(NULL)
	, m_pszEventText(NULL)
	, m_pszEventExtendedText(NULL)
{
	*this = Src;
}


CEventManager::CEventInfo::~CEventInfo()
{
	delete [] m_pszEventName;
	delete [] m_pszEventText;
	delete [] m_pszEventExtendedText;
}


CEventManager::CEventInfo &CEventManager::CEventInfo::operator=(const CEventInfo &Src)
{
	if (&Src != this) {
		m_UpdateTime = Src.m_UpdateTime;
		m_Type = Src.m_Type;
		m_EventID = Src.m_EventID;
		m_StartTime = Src.m_StartTime;
		m_Duration = Src.m_Duration;
		m_RunningStatus = Src.m_RunningStatus;
		m_bFreeCaMode = Src.m_bFreeCaMode;
		SetEventName(Src.m_pszEventName);
		SetEventText(Src.m_pszEventText);
		SetEventExtendedText(Src.m_pszEventExtendedText);
		m_VideoInfo = Src.m_VideoInfo;
		m_AudioList = Src.m_AudioList;
		m_ContentNibble = Src.m_ContentNibble;
		m_EventGroupList = Src.m_EventGroupList;
		m_bCommonEvent = Src.m_bCommonEvent;
		m_CommonEventInfo = Src.m_CommonEventInfo;
	}
	return *this;
}


bool CEventManager::CEventInfo::GetEndTime(SYSTEMTIME *pTime) const
{
	CDateTime Time(m_StartTime);
	if (!Time.Offset(CDateTime::SECONDS(m_Duration)))
		return false;
	Time.Get(pTime);
	return true;
}


void CEventManager::CEventInfo::SetEventName(LPCTSTR pszEventName)
{
	SetText(&m_pszEventName, pszEventName);
}


void CEventManager::CEventInfo::SetEventText(LPCTSTR pszEventText)
{
	SetText(&m_pszEventText, pszEventText);
}


void CEventManager::CEventInfo::SetEventExtendedText(LPCTSTR pszEventExtendedText)
{
	SetText(&m_pszEventExtendedText, pszEventExtendedText);
}


void CEventManager::CEventInfo::SetText(LPTSTR *ppszText, LPCTSTR pszNewText)
{
	if (*ppszText != NULL) {
		if (pszNewText != NULL && ::lstrcmp(*ppszText, pszNewText) == 0)
			return;
		delete [] *ppszText;
		*ppszText = NULL;
	}
	if (pszNewText != NULL) {
		*ppszText = new TCHAR[::lstrlen(pszNewText) + 1];
		::lstrcpy(*ppszText, pszNewText);
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


bool CEventManager::CScheduleInfo::OnSection(const CEitTable *pTable, const CPsiSection *pSection)
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
