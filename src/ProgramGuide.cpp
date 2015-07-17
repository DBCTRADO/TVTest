#include "stdafx.h"
#include <algorithm>
#include "TVTest.h"
#include "AppMain.h"
#include "ProgramGuide.h"
#include "DialogUtil.h"
#include "LogoManager.h"
#include "EpgChannelSettings.h"
#include "ProgramGuideToolbarOptions.h"
#include "VariableString.h"
#include "Help/HelpID.h"
#include "BonTsEngine/TsUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


#define TITLE_TEXT TEXT("EPG番組表")

// 現在時刻を更新するタイマのID
#define TIMER_ID_UPDATECURTIME	1

#define CHEVRON_WIDTH	10
#define CHEVRON_HEIGHT	10

// メニューの位置
#define MENU_DATE			0
#define MENU_CHANNELGROUP	1

#define MAX_CHANNEL_GROUP_MENU_ITEMS \
	(CM_PROGRAMGUIDE_CHANNELGROUP_LAST-CM_PROGRAMGUIDE_CHANNELGROUP_FIRST+1)
#define MAX_CHANNEL_PROVIDER_MENU_ITEMS \
	(CM_PROGRAMGUIDE_CHANNELPROVIDER_LAST-CM_PROGRAMGUIDE_CHANNELPROVIDER_FIRST+1)




namespace ProgramGuide
{


class CEventItem
{
	static const int MAX_TITLE_LENGTH=256;

	const CEventInfoData *m_pEventInfo;
	const CEventInfoData *m_pCommonEventInfo;
	SYSTEMTIME m_StartTime;
	SYSTEMTIME m_EndTime;
	DWORD m_Duration;
	int m_TitleLines;
	int m_ItemPos;
	int m_ItemLines;
	bool m_fSelected;

	int GetTitleText(LPTSTR pszText,int MaxLength) const;
	int GetTimeText(LPTSTR pszText,int MaxLength) const;
	LPCTSTR GetEventText() const;

public:
	CEventItem(const CEventInfoData *pInfo);
	CEventItem(const SYSTEMTIME &StartTime,DWORD Duration);
	~CEventItem();
	const CEventInfoData *GetEventInfo() const { return m_pEventInfo; }
	const CEventInfoData *GetCommonEventInfo() const { return m_pCommonEventInfo; }
	const SYSTEMTIME &GetStartTime() const { return m_StartTime; }
	bool SetStartTime(const SYSTEMTIME &Time);
	const SYSTEMTIME &GetEndTime() const { return m_EndTime; }
	bool SetEndTime(const SYSTEMTIME &Time);
	int GetGenre(int Level=0) const;
	bool SetCommonEvent(const CEventInfoData *pEvent);
	int GetTitleLines() const { return m_TitleLines; }
	void CalcTitleLines(TVTest::CTextDraw &TextDraw,int Width);
	void ResetTitleLines() { m_TitleLines=0; }
	void DrawTitle(TVTest::CTextDraw &TextDraw,const RECT &Rect,int LineHeight) const;
	void DrawText(TVTest::CTextDraw &TextDraw,const RECT &Rect,int LineHeight) const;
	void GetTimeSize(TVTest::CTextDraw &TextDraw,SIZE *pSize) const;
	int GetItemPos() const { return m_ItemPos; }
	void SetItemPos(int Pos) { m_ItemPos=Pos; }
	int GetItemLines() const { return m_ItemLines; }
	void SetItemLines(int Lines) { m_ItemLines=Lines; }
	bool IsNullItem() const { return m_pEventInfo==NULL; }
	void SetSelected(bool fSelected) { m_fSelected=fSelected; }
	bool IsSelected() const { return m_fSelected; }
};


CEventItem::CEventItem(const CEventInfoData *pInfo)
	: m_pEventInfo(pInfo)
	, m_pCommonEventInfo(NULL)
	, m_TitleLines(0)
	, m_ItemPos(-1)
	, m_ItemLines(0)
	, m_fSelected(false)
{
	m_pEventInfo->GetStartTime(&m_StartTime);
	m_StartTime.wSecond=0;
	m_pEventInfo->GetEndTime(&m_EndTime);
	m_EndTime.wSecond=0;
}


CEventItem::CEventItem(const SYSTEMTIME &StartTime,DWORD Duration)
	: m_pEventInfo(NULL)
	, m_pCommonEventInfo(NULL)
	, m_StartTime(StartTime)
	, m_Duration(Duration)
	, m_TitleLines(0)
	, m_ItemPos(-1)
	, m_ItemLines(0)
	, m_fSelected(false)
{
	m_EndTime=StartTime;
	OffsetSystemTime(&m_EndTime,Duration*TimeConsts::SYSTEMTIME_SECOND);
}


CEventItem::~CEventItem()
{
}


bool CEventItem::SetStartTime(const SYSTEMTIME &Time)
{
	if (CompareSystemTime(&Time,&m_EndTime)>=0)
		return false;
	m_StartTime=Time;
	return true;
}


bool CEventItem::SetEndTime(const SYSTEMTIME &Time)
{
	if (CompareSystemTime(&m_StartTime,&Time)>=0)
		return false;
	m_EndTime=Time;
	return true;
}


int CEventItem::GetGenre(int Level) const
{
	if (m_pEventInfo!=NULL) {
		const CEventInfoData::ContentNibble *pContentNibble;

		if (m_pEventInfo->m_ContentNibble.NibbleCount>0) {
			pContentNibble=&m_pEventInfo->m_ContentNibble;
		} else if (m_pCommonEventInfo!=NULL
				&& m_pCommonEventInfo->m_ContentNibble.NibbleCount>0) {
			pContentNibble=&m_pCommonEventInfo->m_ContentNibble;
		} else {
			return -1;
		}
		for (int i=0;i<pContentNibble->NibbleCount;i++) {
			if (pContentNibble->NibbleList[i].ContentNibbleLevel1!=0xE) {
				int Nibble=Level==0?pContentNibble->NibbleList[i].ContentNibbleLevel1:
									pContentNibble->NibbleList[i].ContentNibbleLevel2;
				if (Nibble<=CEventInfoData::CONTENT_LAST)
					return Nibble;
				break;
			}
		}
	}
	return -1;
}


bool CEventItem::SetCommonEvent(const CEventInfoData *pEvent)
{
	if (m_pEventInfo==NULL || !m_pEventInfo->m_bCommonEvent || pEvent==NULL)
		return false;
	m_pCommonEventInfo=pEvent;
	/*
	if (m_pEventInfo->m_EventName.empty())
		m_TitleLines=pItem->m_TitleLines;	// とりあえず
	*/
	return true;
}


int CEventItem::GetTitleText(LPTSTR pszText,int MaxLength) const
{
	int Length;

	Length=GetTimeText(pszText,MaxLength);
	if (m_pEventInfo!=NULL) {
		const CEventInfoData::String *pEventName;
		if (m_pEventInfo->m_EventName.empty() && m_pCommonEventInfo!=NULL)
			pEventName=&m_pCommonEventInfo->m_EventName;
		else
			pEventName=&m_pEventInfo->m_EventName;
		if (!pEventName->empty())
			Length+=StdUtil::snprintf(pszText+Length,MaxLength-Length,TEXT(" %s"),pEventName->c_str());
	}
	return Length;
}


int CEventItem::GetTimeText(LPTSTR pszText,int MaxLength) const
{
	return EpgUtil::FormatEventTime(m_StartTime,0,pszText,MaxLength,EpgUtil::EVENT_TIME_START_ONLY);
}


LPCTSTR CEventItem::GetEventText() const
{
	if (m_pEventInfo==NULL)
		return NULL;

#if 0
	if (m_pCommonEventInfo!=NULL
			&& ((m_pEventInfo->m_EventText.empty()
					&& !m_pCommonEventInfo->m_EventText.empty())
				|| (m_pEventInfo->m_EventExtendedText.empty()
					&& !m_pCommonEventInfo->m_EventExtendedText.empty())))
		return EpgUtil::GetEventDisplayText(*m_pCommonEventInfo);
#endif

	return EpgUtil::GetEventDisplayText(*m_pEventInfo);
}


void CEventItem::CalcTitleLines(TVTest::CTextDraw &TextDraw,int Width)
{
	if (m_TitleLines==0) {
		TCHAR szText[MAX_TITLE_LENGTH];
		GetTitleText(szText,lengthof(szText));
		m_TitleLines=TextDraw.CalcLineCount(szText,Width);
	}
}


void CEventItem::DrawTitle(TVTest::CTextDraw &TextDraw,const RECT &Rect,int LineHeight) const
{
	TCHAR szText[MAX_TITLE_LENGTH];

	GetTitleText(szText,lengthof(szText));
	TextDraw.Draw(szText,Rect,LineHeight/*,TVTest::CTextDraw::DRAW_FLAG_JUSTIFY_MULTI_LINE*/);
}


void CEventItem::DrawText(TVTest::CTextDraw &TextDraw,const RECT &Rect,int LineHeight) const
{
	LPCTSTR pszEventText=GetEventText();
	if (!IsStringEmpty(pszEventText))
		TextDraw.Draw(pszEventText,Rect,LineHeight,TVTest::CTextDraw::DRAW_FLAG_JUSTIFY_MULTI_LINE);
}


void CEventItem::GetTimeSize(TVTest::CTextDraw &TextDraw,SIZE *pSize) const
{
	TCHAR szText[32];
	int Length;
	TVTest::CTextDraw::TextMetrics Metrics;

	Length=GetTimeText(szText,lengthof(szText));
	if (TextDraw.GetTextMetrics(szText,Length,&Metrics)) {
		pSize->cx=Metrics.Width;
		pSize->cy=Metrics.Height;
	} else {
		pSize->cx=0;
		pSize->cy=0;
	}
}




class CEventLayout
{
	const CServiceInfo *m_pServiceInfo;
	std::vector<CEventItem*> m_EventList;

public:
	CEventLayout(const CServiceInfo *pServiceInfo);
	~CEventLayout();
	const CServiceInfo *GetServiceInfo() const { return m_pServiceInfo; }
	void Clear();
	size_t NumItems() const { return m_EventList.size(); }
	void AddItem(CEventItem *pItem) { m_EventList.push_back(pItem); }
	bool InsertItem(size_t Index,CEventItem *pItem);
	CEventItem *GetItem(size_t Index);
	const CEventItem *GetItem(size_t Index) const;
	void InsertNullItems(const SYSTEMTIME &FirstTime,const SYSTEMTIME &LastTime);
};


CEventLayout::CEventLayout(const CServiceInfo *pServiceInfo)
	: m_pServiceInfo(pServiceInfo)
{
}


CEventLayout::~CEventLayout()
{
	Clear();
}


void CEventLayout::Clear()
{
	for (size_t i=0;i<m_EventList.size();i++)
		delete m_EventList[i];
	m_EventList.clear();
}


bool CEventLayout::InsertItem(size_t Index,CEventItem *pItem)
{
	if (Index>m_EventList.size())
		return false;
	m_EventList.insert(m_EventList.begin()+Index,pItem);
	return true;
}


CEventItem *CEventLayout::GetItem(size_t Index)
{
	if (Index>=m_EventList.size())
		return NULL;
	return m_EventList[Index];
}


const CEventItem *CEventLayout::GetItem(size_t Index) const
{
	if (Index>=m_EventList.size())
		return NULL;
	return m_EventList[Index];
}


void CEventLayout::InsertNullItems(const SYSTEMTIME &FirstTime,const SYSTEMTIME &LastTime)
{
	int FirstItem,LastItem;
	int i;
	CEventItem *pItem,*pPrevItem;
	SYSTEMTIME stPrev,stStart,stEnd;
	int EmptyCount;

	FirstItem=-1;
	LastItem=-1;
	EmptyCount=0;
	stPrev=FirstTime;
	for (i=0;i<(int)m_EventList.size();i++) {
		pItem=m_EventList[i];
		stStart=pItem->GetStartTime();
		stEnd=pItem->GetEndTime();
		if (CompareSystemTime(&stStart,&LastTime)<0
				&& CompareSystemTime(&stEnd,&FirstTime)>0) {
			if (FirstItem<0) {
				FirstItem=i;
				LastItem=i+1;
			} else if (LastItem<i+1) {
				LastItem=i+1;
			}
			if (CompareSystemTime(&stPrev,&stStart)<0)
				EmptyCount++;
		}
		if (CompareSystemTime(&stEnd,&LastTime)>=0)
			break;
		stPrev=stEnd;
	}
	if (EmptyCount>0) {
		pPrevItem=NULL;
		stPrev=FirstTime;
		for (i=FirstItem;i<LastItem;i++) {
			pItem=m_EventList[i];
			stStart=pItem->GetStartTime();
			int Cmp=CompareSystemTime(&stPrev,&stStart);
			if (Cmp>0) {
				if (pPrevItem)
					pPrevItem->SetEndTime(stStart);
			} else if (Cmp<0) {
				LONGLONG Diff=DiffSystemTime(&stPrev,&stStart);

				if (Diff<TimeConsts::SYSTEMTIME_MINUTE) {
					if (pPrevItem)
						pPrevItem->SetEndTime(stStart);
				} else {
					InsertItem(i,new CEventItem(stPrev,(DWORD)(Diff/1000)));
					i++;
					LastItem++;
				}
			}
			stPrev=pItem->GetEndTime();
			pPrevItem=pItem;
		}
	}
}




CEventLayoutList::~CEventLayoutList()
{
	Clear();
}


void CEventLayoutList::Clear()
{
	for (size_t i=0;i<m_LayoutList.size();i++)
		delete m_LayoutList[i];
	m_LayoutList.clear();
}


void CEventLayoutList::Add(CEventLayout *pLayout)
{
	m_LayoutList.push_back(pLayout);
}


CEventLayout *CEventLayoutList::operator[](size_t Index)
{
	if (Index>=m_LayoutList.size())
		return NULL;
	return m_LayoutList[Index];
}


const CEventLayout *CEventLayoutList::operator[](size_t Index) const
{
	if (Index>=m_LayoutList.size())
		return NULL;
	return m_LayoutList[Index];
}




class CServiceInfo
{
	CChannelInfo m_ChannelInfo;
	CServiceInfoData m_ServiceData;
	LPTSTR m_pszBonDriverFileName;
	HBITMAP m_hbmLogo;
	DrawUtil::CBitmap m_StretchedLogo;
	std::vector<CEventInfoData*> m_EventList;
	typedef std::map<WORD,CEventInfoData*> EventIDMap;
	EventIDMap m_EventIDMap;

public:
	CServiceInfo(const CChannelInfo &ChannelInfo,LPCTSTR pszBonDriver);
	CServiceInfo(const CChannelInfo &ChannelInfo,const CEpgServiceInfo &Info,LPCTSTR pszBonDriver);
	~CServiceInfo();
	const CChannelInfo &GetChannelInfo() const { return m_ChannelInfo; }
	const CServiceInfoData &GetServiceInfoData() const { return m_ServiceData; }
	WORD GetNetworkID() const { return m_ServiceData.m_NetworkID; }
	WORD GetTSID() const { return m_ServiceData.m_TSID; }
	WORD GetServiceID() const { return m_ServiceData.m_ServiceID; }
	LPCTSTR GetServiceName() const { return m_ChannelInfo.GetName(); }
	LPCTSTR GetBonDriverFileName() const { return m_pszBonDriverFileName; }
	void SetLogo(HBITMAP hbm) { m_hbmLogo=hbm; }
	HBITMAP GetLogo() const { return m_hbmLogo; }
	HBITMAP GetStretchedLogo(int Width,int Height);
	int NumEvents() const { return (int)m_EventList.size(); }
	CEventInfoData *GetEvent(int Index);
	const CEventInfoData *GetEvent(int Index) const;
	CEventInfoData *GetEventByEventID(WORD EventID);
	const CEventInfoData *GetEventByEventID(WORD EventID) const;
	bool AddEvent(CEventInfoData *pEvent);
	void ClearEvents();
	void CalcLayout(CEventLayout *pEventList,const CServiceList *pServiceList,
		const SYSTEMTIME &FirstTime,const SYSTEMTIME &LastTime,int LinesPerHour);
	bool SaveiEpgFile(const CEventInfoData *pEventInfo,LPCTSTR pszFileName,bool fVersion2) const;
};


CServiceInfo::CServiceInfo(const CChannelInfo &ChannelInfo,LPCTSTR pszBonDriver)
	: m_ChannelInfo(ChannelInfo)
	, m_ServiceData(ChannelInfo.GetNetworkID(),
					ChannelInfo.GetTransportStreamID(),
					ChannelInfo.GetServiceID())
	, m_pszBonDriverFileName(DuplicateString(pszBonDriver))
	, m_hbmLogo(NULL)
{
}


CServiceInfo::CServiceInfo(const CChannelInfo &ChannelInfo,const CEpgServiceInfo &Info,LPCTSTR pszBonDriver)
	: m_ChannelInfo(ChannelInfo)
	, m_ServiceData(Info.m_ServiceData)
	, m_pszBonDriverFileName(DuplicateString(pszBonDriver))
	, m_hbmLogo(NULL)
{
}


CServiceInfo::~CServiceInfo()
{
	delete [] m_pszBonDriverFileName;
	ClearEvents();
}


HBITMAP CServiceInfo::GetStretchedLogo(int Width,int Height)
{
	if (m_hbmLogo==NULL)
		return NULL;
	// AlphaBlendでリサイズすると汚いので、予めリサイズした画像を作成しておく
	if (m_StretchedLogo.IsCreated()) {
		if (m_StretchedLogo.GetWidth()!=Width || m_StretchedLogo.GetHeight()!=Height)
			m_StretchedLogo.Destroy();
	}
	if (!m_StretchedLogo.IsCreated()) {
		HBITMAP hbm=DrawUtil::ResizeBitmap(m_hbmLogo,Width,Height);
		if (hbm!=NULL)
			m_StretchedLogo.Attach(hbm);
	}
	return m_StretchedLogo.GetHandle();
}


CEventInfoData *CServiceInfo::GetEvent(int Index)
{
	if (Index<0 || (size_t)Index>=m_EventList.size()) {
		TRACE(TEXT("CServiceInfo::GetEvent() : Out of range %d\n"),Index);
		return NULL;
	}
	return m_EventList[Index];
}


const CEventInfoData *CServiceInfo::GetEvent(int Index) const
{
	if (Index<0 || (size_t)Index>=m_EventList.size()) {
		TRACE(TEXT("CServiceInfo::GetEvent() const : Out of range %d\n"),Index);
		return NULL;
	}
	return m_EventList[Index];
}


CEventInfoData *CServiceInfo::GetEventByEventID(WORD EventID)
{
	EventIDMap::iterator itr=m_EventIDMap.find(EventID);
	if (itr==m_EventIDMap.end())
		return NULL;
	return itr->second;
}


const CEventInfoData *CServiceInfo::GetEventByEventID(WORD EventID) const
{
	EventIDMap::const_iterator itr=m_EventIDMap.find(EventID);
	if (itr==m_EventIDMap.end())
		return NULL;
	return itr->second;
}


bool CServiceInfo::AddEvent(CEventInfoData *pEvent)
{
	m_EventList.push_back(pEvent);
	m_EventIDMap[pEvent->m_EventID]=pEvent;
	return true;
}


void CServiceInfo::ClearEvents()
{
	for (size_t i=0;i<m_EventList.size();i++)
		delete m_EventList[i];
	m_EventList.clear();
	m_EventIDMap.clear();
}


void CServiceInfo::CalcLayout(CEventLayout *pEventList,const CServiceList *pServiceList,
	const SYSTEMTIME &FirstTime,const SYSTEMTIME &LastTime,int LinesPerHour)
{
	pEventList->Clear();

	int FirstItem=-1,LastItem=-1;
	for (int i=0;i<(int)m_EventList.size();i++) {
		CEventInfoData *pEvent=m_EventList[i];
		SYSTEMTIME StartTime,EndTime;
		pEvent->GetStartTime(&StartTime);
		pEvent->GetEndTime(&EndTime);

		if (CompareSystemTime(&StartTime,&LastTime)<0
				&& CompareSystemTime(&EndTime,&FirstTime)>0) {
			if (FirstItem<0) {
				FirstItem=i;
				LastItem=i+1;
			} else if (LastItem<i+1) {
				LastItem=i+1;
			}

			CEventItem *pItem=new CEventItem(pEvent);
			if (pEvent->m_bCommonEvent) {
				const CEventInfoData *pCommonEvent=
					pServiceList->GetEventByIDs(m_ServiceData.m_TSID,
												pEvent->m_CommonEventInfo.ServiceID,
												pEvent->m_CommonEventInfo.EventID);
				if (pCommonEvent!=NULL)
					pItem->SetCommonEvent(pCommonEvent);
			}
			pEventList->AddItem(pItem);
		}
		if (CompareSystemTime(&EndTime,&LastTime)>=0)
			break;
	}
	if (FirstItem<0)
		return;

	pEventList->InsertNullItems(FirstTime,LastTime);

	const size_t NumItems=pEventList->NumItems();
	SYSTEMTIME stFirst,stLast;
	CEventItem *pItem;
	int ItemPos=0;

	stFirst=FirstTime;
	for (size_t i=0;i<NumItems;) {
		if (CompareSystemTime(&stFirst,&LastTime)>=0)
			break;
		stLast=stFirst;
		OffsetSystemTime(&stLast,TimeConsts::SYSTEMTIME_HOUR);
		do {
			if (CompareSystemTime(&pEventList->GetItem(i)->GetEndTime(),&stFirst)>0)
				break;
			i++;
		} while (i<NumItems);
		if (i==NumItems)
			break;
		int ProgramsPerHour=0;
		do {
			if (CompareSystemTime(&pEventList->GetItem(i+ProgramsPerHour)->GetStartTime(),&stLast)>=0)
				break;
			ProgramsPerHour++;
		} while (i+ProgramsPerHour<NumItems);
		if (ProgramsPerHour>0) {
			int Lines=LinesPerHour,Offset=0;

			const SYSTEMTIME &stStart=pEventList->GetItem(i)->GetStartTime();
			if (CompareSystemTime(&stStart,&stFirst)>0) {
				Offset=(int)(DiffSystemTime(&stFirst,&stStart)*LinesPerHour/TimeConsts::SYSTEMTIME_HOUR);
				Lines-=Offset;
			}
			if (Lines>ProgramsPerHour) {
				const SYSTEMTIME &stEnd=pEventList->GetItem(i+ProgramsPerHour-1)->GetEndTime();
				if (CompareSystemTime(&stEnd,&stLast)<0) {
					Lines-=(int)(DiffSystemTime(&stEnd,&stLast)*LinesPerHour/TimeConsts::SYSTEMTIME_HOUR);
					if (Lines<ProgramsPerHour)
						Lines=ProgramsPerHour;
				}
			}
			if (ProgramsPerHour==1) {
				pItem=pEventList->GetItem(i);
				pItem->SetItemLines(pItem->GetItemLines()+Lines);
				if (pItem->GetItemPos()<0)
					pItem->SetItemPos(ItemPos+Offset);
			} else {
				int *pItemLines=new int[ProgramsPerHour];

				for (int j=0;j<ProgramsPerHour;j++)
					pItemLines[j]=j<Lines?1:0;
				if (Lines>ProgramsPerHour) {
					int LineCount=ProgramsPerHour;

					do {
						DWORD Time,MaxTime;
						int MaxItem;

						MaxTime=0;
						for (int j=0;j<ProgramsPerHour;j++) {
							pItem=pEventList->GetItem(i+j);
							SYSTEMTIME stStart=pItem->GetStartTime();
							if (CompareSystemTime(&stStart,&stFirst)<0)
								stStart=stFirst;
							SYSTEMTIME stEnd=pItem->GetEndTime();
							if (CompareSystemTime(&stEnd,&stLast)>0)
								stEnd=stLast;
							Time=(DWORD)(DiffSystemTime(&stStart,&stEnd)/pItemLines[j]);
							if (Time>MaxTime) {
								MaxTime=Time;
								MaxItem=j;
							}
						}
						if (MaxTime==0)
							break;
						pItemLines[MaxItem]++;
						LineCount++;
					} while (LineCount<Lines);
				}
				int Pos=ItemPos+Offset;
				for (int j=0;j<min(ProgramsPerHour,Lines);j++) {
					pItem=pEventList->GetItem(i+j);
					if (pItem->GetItemPos()<0)
						pItem->SetItemPos(Pos);
					pItem->SetItemLines(pItem->GetItemLines()+pItemLines[j]);
					Pos+=pItemLines[j];
				}
				delete [] pItemLines;
				i+=ProgramsPerHour-1;
			}
		}
		ItemPos+=LinesPerHour;
		stFirst=stLast;
	}
}


bool CServiceInfo::SaveiEpgFile(const CEventInfoData *pEventInfo,LPCTSTR pszFileName,bool fVersion2) const
{
	if (pEventInfo==NULL)
		return false;

	HANDLE hFile;
	char szText[2048],szServiceName[64],szEventName[256];
	SYSTEMTIME stStart,stEnd;
	DWORD Length,Write;

	hFile=::CreateFile(pszFileName,GENERIC_WRITE,FILE_SHARE_READ,NULL,
					   CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return false;
	if (!IsStringEmpty(GetServiceName()))
		::WideCharToMultiByte(932,0,GetServiceName(),-1,
							  szServiceName,sizeof(szServiceName),NULL,NULL);
	else
		szServiceName[0]='\0';
	pEventInfo->GetStartTime(&stStart);
	pEventInfo->GetEndTime(&stEnd);
	if (!pEventInfo->m_EventName.empty())
		::WideCharToMultiByte(932,0,pEventInfo->m_EventName.c_str(),-1,
							  szEventName,sizeof(szEventName),NULL,NULL);
	else
		szEventName[0]='\0';
	if (fVersion2) {
		const char *pszStationFormat;
		switch (GetAppClass().NetworkDefinition.GetNetworkType(m_ServiceData.m_NetworkID)) {
		default:
		case TVTest::CNetworkDefinition::NETWORK_TERRESTRIAL:
			pszStationFormat="DFS%05x";
			break;
		case TVTest::CNetworkDefinition::NETWORK_BS:
			pszStationFormat="BSDT%03d";
			break;
		case TVTest::CNetworkDefinition::NETWORK_CS:
			pszStationFormat="CSDT%03d";
			break;
		}
		char szStation[16];
		StdUtil::snprintf(szStation,lengthof(szStation),
						  pszStationFormat,m_ServiceData.m_ServiceID);
		Length=StdUtil::snprintf(szText,lengthof(szText),
			"Content-type: application/x-tv-program-digital-info; charset=shift_jis\r\n"
			"version: 2\r\n"
			"station: %s\r\n"
			"station-name: %s\r\n"
			"year: %d\r\n"
			"month: %02d\r\n"
			"date: %02d\r\n"
			"start: %02d:%02d\r\n"
			"end: %02d:%02d\r\n"
			"program-title: %s\r\n"
			"program-id: %d\r\n",
			szStation,szServiceName,
			stStart.wYear,stStart.wMonth,stStart.wDay,
			stStart.wHour,stStart.wMinute,
			stEnd.wHour,stEnd.wMinute,
			szEventName,pEventInfo->m_EventID);
	} else {
		Length=StdUtil::snprintf(szText,lengthof(szText),
			"Content-type: application/x-tv-program-info; charset=shift_jis\r\n"
			"version: 1\r\n"
			"station: %s\r\n"
			"year: %d\r\n"
			"month: %02d\r\n"
			"date: %02d\r\n"
			"start: %02d:%02d\r\n"
			"end: %02d:%02d\r\n"
			"program-title: %s\r\n",
			szServiceName,
			stStart.wYear,stStart.wMonth,stStart.wDay,
			stStart.wHour,stStart.wMinute,
			stEnd.wHour,stEnd.wMinute,
			szEventName);
	}
	bool fOK=::WriteFile(hFile,szText,Length,&Write,NULL) && Write==Length;
	::FlushFileBuffers(hFile);
	::CloseHandle(hFile);
	return fOK;
}




CServiceList::~CServiceList()
{
	Clear();
}


CServiceInfo *CServiceList::GetItem(size_t Index)
{
	if (Index>=m_ServiceList.size())
		return NULL;
	return m_ServiceList[Index];
}


const CServiceInfo *CServiceList::GetItem(size_t Index) const
{
	if (Index>=m_ServiceList.size())
		return NULL;
	return m_ServiceList[Index];
}


CServiceInfo *CServiceList::GetItemByIDs(WORD TransportStreamID,WORD ServiceID)
{
	int Index=FindItemByIDs(TransportStreamID,ServiceID);

	if (Index<0)
		return NULL;

	return m_ServiceList[Index];
}


const CServiceInfo *CServiceList::GetItemByIDs(WORD TransportStreamID,WORD ServiceID) const
{
	int Index=FindItemByIDs(TransportStreamID,ServiceID);

	if (Index<0)
		return NULL;

	return m_ServiceList[Index];
}


int CServiceList::FindItemByIDs(WORD TransportStreamID,WORD ServiceID) const
{
	for (size_t i=0;i<m_ServiceList.size();i++) {
		const CServiceInfo *pInfo=m_ServiceList[i];

		if (pInfo->GetTSID()==TransportStreamID
				&& pInfo->GetServiceID()==ServiceID)
			return (int)i;
	}
	return -1;
}


CEventInfoData *CServiceList::GetEventByIDs(WORD TransportStreamID,WORD ServiceID,WORD EventID)
{
	CServiceInfo *pService=GetItemByIDs(TransportStreamID,ServiceID);
	if (pService==NULL)
		return NULL;
	return pService->GetEventByEventID(EventID);
}


const CEventInfoData *CServiceList::GetEventByIDs(WORD TransportStreamID,WORD ServiceID,WORD EventID) const
{
	const CServiceInfo *pService=GetItemByIDs(TransportStreamID,ServiceID);
	if (pService==NULL)
		return NULL;
	return pService->GetEventByEventID(EventID);
}


void CServiceList::Add(CServiceInfo *pInfo)
{
	m_ServiceList.push_back(pInfo);
}


void CServiceList::Clear()
{
	for (size_t i=0;i<m_ServiceList.size();i++)
		delete m_ServiceList[i];
	m_ServiceList.clear();
}


}	// namespace ProgramGuide




CProgramGuideChannelProvider::~CProgramGuideChannelProvider()
{
}


bool CProgramGuideChannelProvider::Update()
{
	return true;
}




CProgramGuideBaseChannelProvider::CProgramGuideBaseChannelProvider()
{
}


CProgramGuideBaseChannelProvider::CProgramGuideBaseChannelProvider(const CTuningSpaceList *pSpaceList,LPCTSTR pszBonDriver)
{
	SetTuningSpaceList(pSpaceList);
	SetBonDriverFileName(pszBonDriver);
}


CProgramGuideBaseChannelProvider::~CProgramGuideBaseChannelProvider()
{
}


bool CProgramGuideBaseChannelProvider::GetName(LPTSTR pszName,int MaxName) const
{
	if (pszName==NULL || MaxName<1)
		return false;

	size_t Length=m_BonDriverFileName.length();
	if (Length>=(size_t)MaxName)
		Length=MaxName-1;
	if (Length>0)
		m_BonDriverFileName.copy(pszName,Length);
	pszName[Length]=_T('\0');
	::PathRemoveExtension(pszName);

	return true;
}


size_t CProgramGuideBaseChannelProvider::GetGroupCount() const
{
	int GroupCount=m_TuningSpaceList.NumSpaces();
	if (HasAllChannelGroup())
		GroupCount++;
	return GroupCount;
}


bool CProgramGuideBaseChannelProvider::GetGroupName(size_t Group,LPTSTR pszName,int MaxName) const
{
	if (Group>=GetGroupCount() || pszName==NULL || MaxName<1)
		return false;

	if (HasAllChannelGroup()) {
		if (Group==0) {
			::lstrcpyn(pszName,TEXT("すべてのチャンネル"),MaxName);
			return true;
		}
		Group--;
	}

	LPCTSTR pszTuningSpaceName=m_TuningSpaceList.GetTuningSpaceName((int)Group);
	if (!IsStringEmpty(pszTuningSpaceName))
		::lstrcpyn(pszName,pszTuningSpaceName,MaxName);
	else
		StdUtil::snprintf(pszName,MaxName,TEXT("チューニング空間 %d"),(int)Group+1);

	return true;
}


bool CProgramGuideBaseChannelProvider::GetGroupID(size_t Group,TVTest::String *pID) const
{
	if (Group>=GetGroupCount() || pID==NULL)
		return false;

	TVTest::StringUtility::Format(*pID,TEXT("%u"),static_cast<unsigned int>(Group));

	return true;
}


int CProgramGuideBaseChannelProvider::ParseGroupID(LPCTSTR pszID) const
{
	if (IsStringEmpty(pszID))
		return -1;

	int Group=::StrToInt(pszID);
	if (Group<0 || static_cast<size_t>(Group)>=GetGroupCount())
		return -1;
	return Group;
}


size_t CProgramGuideBaseChannelProvider::GetChannelCount(size_t Group) const
{
	if (Group>=GetGroupCount())
		return 0;

	if (HasAllChannelGroup()) {
		if (Group==0)
			return m_TuningSpaceList.GetAllChannelList()->NumChannels();
		Group--;
	}

	const CTuningSpaceInfo *pSpaceInfo=m_TuningSpaceList.GetTuningSpaceInfo((int)Group);
	if (pSpaceInfo==NULL)
		return 0;

	return pSpaceInfo->NumChannels();
}


const CChannelInfo *CProgramGuideBaseChannelProvider::GetChannelInfo(size_t Group,size_t Channel) const
{
	if (Group>=GetGroupCount())
		return NULL;

	if (HasAllChannelGroup()) {
		if (Group==0)
			return m_TuningSpaceList.GetAllChannelList()->GetChannelInfo((int)Channel);
		Group--;
	}

	const CTuningSpaceInfo *pSpaceInfo=m_TuningSpaceList.GetTuningSpaceInfo((int)Group);
	if (pSpaceInfo==NULL)
		return false;

	return pSpaceInfo->GetChannelInfo((int)Channel);
}


bool CProgramGuideBaseChannelProvider::GetBonDriver(LPTSTR pszFileName,int MaxLength) const
{
	if (pszFileName==NULL || MaxLength<1 || m_BonDriverFileName.empty())
		return false;

	const size_t Length=m_BonDriverFileName.length();
	if ((size_t)MaxLength<=Length)
		return false;
	m_BonDriverFileName.copy(pszFileName,Length);
	pszFileName[Length]=_T('\0');

	return true;
}


bool CProgramGuideBaseChannelProvider::GetBonDriverFileName(size_t Group,size_t Channel,LPTSTR pszFileName,int MaxLength) const
{
	return GetBonDriver(pszFileName,MaxLength);
}


bool CProgramGuideBaseChannelProvider::SetTuningSpaceList(const CTuningSpaceList *pList)
{
	if (pList!=NULL)
		m_TuningSpaceList=*pList;
	else
		m_TuningSpaceList.Clear();

	return true;
}


bool CProgramGuideBaseChannelProvider::SetBonDriverFileName(LPCTSTR pszFileName)
{
	if (!IsStringEmpty(pszFileName))
		m_BonDriverFileName=pszFileName;
	else
		m_BonDriverFileName.clear();

	return true;
}


bool CProgramGuideBaseChannelProvider::HasAllChannelGroup() const
{
	return m_TuningSpaceList.NumSpaces()>1
		&& m_TuningSpaceList.GetAllChannelList()->NumChannels()>0;
}




CProgramGuideChannelProviderManager::~CProgramGuideChannelProviderManager()
{
}




const LPCTSTR CProgramGuide::m_pszWindowClass=APP_NAME TEXT(" Program Guide");
HINSTANCE CProgramGuide::m_hinst=NULL;


bool CProgramGuide::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=m_pszWindowClass;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CProgramGuide::CProgramGuide(CEventSearchOptions &EventSearchOptions)
	: m_pProgramList(NULL)
	, m_ListMode(LIST_SERVICES)
	, m_WeekListService(-1)
	, m_LinesPerHour(12)
	, m_TextDrawEngine(TVTest::CTextDrawClient::ENGINE_GDI)
	, m_ItemWidth(140)
	, m_TextLeftMargin(m_Style.EventIconSize.Width+
					   m_Style.EventIconMargin.Left+m_Style.EventIconMargin.Right)
	, m_fDragScroll(false)
	, m_fScrolling(false)
	, m_hDragCursor1(NULL)
	, m_hDragCursor2(NULL)
	, m_VisibleEventIcons(((1<<(CEpgIcons::ICON_LAST+1))-1)^CEpgIcons::IconFlag(CEpgIcons::ICON_PAY))
	, m_fBarShadow(false)
	, m_EventInfoPopupManager(&m_EventInfoPopup)
	, m_EventInfoPopupHandler(this)
	, m_fShowToolTip(true)
	, m_fKeepTimePos(false)
	, m_pChannelProviderManager(NULL)
	, m_pChannelProvider(NULL)
	, m_CurrentChannelProvider(-1)
	, m_CurrentChannelGroup(-1)
	, m_fExcludeNoEventServices(true)
	, m_CurrentEventID(0)
	, m_BeginHour(-1)
	, m_pEventHandler(NULL)
	, m_pFrame(NULL)
	, m_pProgramCustomizer(NULL)
	, m_WheelScrollLines(0)
	, m_ProgramSearchEventHandler(this)
	, m_Filter(0)
	, m_fEpgUpdating(false)
	, m_ProgramSearch(EventSearchOptions)
	, m_fShowFeaturedMark(true)
{
	m_WindowPosition.Left=0;
	m_WindowPosition.Top=0;
	m_WindowPosition.Width=640;
	m_WindowPosition.Height=480;

	LOGFONT lf;
	DrawUtil::GetDefaultUIFont(&lf);
	SetFont(&lf);

	m_ScrollPos.x=0;
	m_ScrollPos.y=0;
	m_OldScrollPos=m_ScrollPos;

	m_CurEventItem.fSelected=false;

	m_EventInfoPopup.SetEventHandler(&m_EventInfoPopupHandler);

	static const LPCTSTR SearchTargetList[] = {
		TEXT("この番組表から"),
		TEXT("全ての番組から"),
	};
	m_ProgramSearch.SetSearchTargetList(SearchTargetList,lengthof(SearchTargetList));
}


CProgramGuide::~CProgramGuide()
{
	Destroy();

	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pProgramGuide=NULL;
	if (m_pFrame!=NULL)
		m_pFrame->m_pProgramGuide=NULL;
	if (m_pProgramCustomizer!=NULL)
		m_pProgramCustomizer->m_pProgramGuide=NULL;
}


void CProgramGuide::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);
}


void CProgramGuide::NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	m_Style.NormalizeStyle(pStyleManager);
}


void CProgramGuide::SetTheme(const TVTest::Theme::CThemeManager *pThemeManager)
{
	static const struct {
		int From,To;
	} ProgramGuideColorMap[] = {
		{CColorScheme::COLOR_PROGRAMGUIDE_BACK,				COLOR_BACK},
		{CColorScheme::COLOR_PROGRAMGUIDE_HIGHLIGHTTEXT,	COLOR_HIGHLIGHT_TEXT},
		{CColorScheme::COLOR_PROGRAMGUIDE_HIGHLIGHTTITLE,	COLOR_HIGHLIGHT_TITLE},
		{CColorScheme::COLOR_PROGRAMGUIDE_HIGHLIGHTBACK,	COLOR_HIGHLIGHT_BACK},
		{CColorScheme::COLOR_PROGRAMGUIDE_HIGHLIGHTBORDER,	COLOR_HIGHLIGHT_BORDER},
		{CColorScheme::COLOR_PROGRAMGUIDE_CHANNELTEXT,		COLOR_CHANNELNAMETEXT},
		{CColorScheme::COLOR_PROGRAMGUIDE_CURCHANNELTEXT,	COLOR_CURCHANNELNAMETEXT},
		{CColorScheme::COLOR_PROGRAMGUIDE_TIMETEXT,			COLOR_TIMETEXT},
		{CColorScheme::COLOR_PROGRAMGUIDE_TIMELINE,			COLOR_TIMELINE},
		{CColorScheme::COLOR_PROGRAMGUIDE_CURTIMELINE,		COLOR_CURTIMELINE},
	};
	for (int i=0;i<lengthof(ProgramGuideColorMap);i++) {
		m_Theme.ColorList[ProgramGuideColorMap[i].To]=
			pThemeManager->GetColor(ProgramGuideColorMap[i].From);
	}

	pThemeManager->GetFillStyle(TVTest::Theme::CThemeManager::STYLE_PROGRAMGUIDE_CHANNEL,
								&m_Theme.ChannelNameBackStyle);
	pThemeManager->GetFillStyle(TVTest::Theme::CThemeManager::STYLE_PROGRAMGUIDE_CURCHANNEL,
								&m_Theme.CurChannelNameBackStyle);
	pThemeManager->GetFillStyle(TVTest::Theme::CThemeManager::STYLE_PROGRAMGUIDE_TIMEBAR,
								&m_Theme.TimeBarMarginStyle);
	pThemeManager->GetBackgroundStyle(TVTest::Theme::CThemeManager::STYLE_PROGRAMGUIDE_FEATUREDMARK,
									  &m_Theme.FeaturedMarkStyle);

	for (int i=0;i<CProgramGuide::TIME_BAR_BACK_COLORS;i++) {
		pThemeManager->GetFillStyle(
			TVTest::Theme::CThemeManager::STYLE_PROGRAMGUIDE_TIMEBAR_0_2+i,
			&m_Theme.TimeBarBackStyle[i]);
	}

	m_EpgTheme.SetTheme(pThemeManager);

	if (m_hwnd!=NULL)
		Invalidate();
}


bool CProgramGuide::SetEpgProgramList(CEpgProgramList *pList)
{
	m_pProgramList=pList;
	return true;
}


void CProgramGuide::Clear()
{
	m_pChannelProvider=NULL;
	m_ServiceList.Clear();
	m_CurrentChannelProvider=-1;
	m_CurrentChannelGroup=-1;
	//m_CurrentChannel.Clear();
	m_EventLayoutList.Clear();
	m_CurEventItem.fSelected=false;
	m_ScrollPos.x=0;
	m_ScrollPos.y=0;
	m_OldScrollPos=m_ScrollPos;
	if (m_hwnd!=NULL) {
		SetCaption();
		Invalidate();
	}
}


bool CProgramGuide::Refresh()
{
	if (m_hwnd==NULL)
		return false;

	if (m_pEventHandler!=NULL
			&& !m_pEventHandler->OnRefresh())
		return false;

	return UpdateProgramGuide(true);
}


bool CProgramGuide::UpdateProgramGuide(bool fUpdateList)
{
	if (m_hwnd!=NULL && m_pChannelProvider!=NULL) {
		HCURSOR hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));

		if (m_pFrame!=NULL)
			m_pFrame->SetCaption(TITLE_TEXT TEXT(" - 番組表を作成しています..."));

		SetMessage(TEXT("番組表を作成しています..."));

		if (UpdateList(fUpdateList)) {
			CalcLayout();
			SetScrollBar();
			GetCurrentEpgTime(&m_stCurTime);
		}

		SetMessage(NULL,false);
		Invalidate();
		SetCaption();

		if (m_pFrame!=NULL) {
			m_pFrame->OnDateChanged();
			m_pFrame->OnSpaceChanged();
		}

		::SetCursor(hcurOld);
	}

	return true;
}


bool CProgramGuide::UpdateList(bool fUpdateList)
{
	if (m_pProgramList==NULL
			|| m_pChannelProvider==NULL)
		return false;

	CServiceInfoData CurServiceInfo;
	if (m_ListMode==LIST_WEEK) {
		ProgramGuide::CServiceInfo *pCurService=m_ServiceList.GetItem(m_WeekListService);
		if (pCurService!=NULL)
			CurServiceInfo=pCurService->GetServiceInfoData();
		m_WeekListService=-1;
	}

	m_ServiceList.Clear();
	for (size_t i=0;i<m_pChannelProvider->GetChannelCount(m_CurrentChannelGroup);i++) {
		const CChannelInfo *pChannelInfo=m_pChannelProvider->GetChannelInfo(m_CurrentChannelGroup,i);

		if (pChannelInfo==NULL
				|| !pChannelInfo->IsEnabled()
				|| IsExcludeService(pChannelInfo->GetNetworkID(),
									pChannelInfo->GetTransportStreamID(),
									pChannelInfo->GetServiceID()))
			continue;

		if (fUpdateList)
			m_pProgramList->UpdateService(
				pChannelInfo->GetNetworkID(),
				pChannelInfo->GetTransportStreamID(),
				pChannelInfo->GetServiceID());
		CEpgServiceInfo *pServiceInfo=m_pProgramList->GetServiceInfo(
			pChannelInfo->GetNetworkID(),
			pChannelInfo->GetTransportStreamID(),
			pChannelInfo->GetServiceID());

		TCHAR szBonDriver[MAX_PATH];
		if (!m_pChannelProvider->GetBonDriverFileName(m_CurrentChannelGroup,i,szBonDriver,lengthof(szBonDriver)))
			szBonDriver[0]=_T('\0');

		ProgramGuide::CServiceInfo *pService;

		if (pServiceInfo!=NULL) {
			pService=new ProgramGuide::CServiceInfo(*pChannelInfo,*pServiceInfo,szBonDriver);
			const CEventInfoList &EventList=pServiceInfo->m_EventList;
			for (auto itrTime=EventList.EventTimeMap.begin();
					itrTime!=EventList.EventTimeMap.end();
					++itrTime) {
				auto itrEvent=EventList.EventDataMap.find(itrTime->EventID);
				if (itrEvent!=EventList.EventDataMap.end())
					pService->AddEvent(new CEventInfoData(itrEvent->second));
			}
		} else {
			if (m_fExcludeNoEventServices)
				continue;
			pService=new ProgramGuide::CServiceInfo(*pChannelInfo,szBonDriver);
		}

		HBITMAP hbmLogo=GetAppClass().LogoManager.GetAssociatedLogoBitmap(
			pService->GetNetworkID(),pService->GetServiceID(),CLogoManager::LOGOTYPE_SMALL);
		if (hbmLogo!=NULL)
			pService->SetLogo(hbmLogo);

		if (m_ListMode==LIST_WEEK && pService->GetServiceInfoData()==CurServiceInfo)
			m_WeekListService=(int)m_ServiceList.NumServices();

		m_ServiceList.Add(pService);
	}

	if (m_ListMode==LIST_WEEK && m_WeekListService<0) {
		m_ListMode=LIST_SERVICES;
		if (m_pFrame!=NULL)
			m_pFrame->OnListModeChanged();
	}

	return true;
}


bool CProgramGuide::UpdateService(ProgramGuide::CServiceInfo *pService,bool fUpdateEpg)
{
	if (fUpdateEpg)
		m_pProgramList->UpdateService(pService->GetNetworkID(),pService->GetTSID(),pService->GetServiceID());

	CEpgServiceInfo *pServiceInfo=
		m_pProgramList->GetServiceInfo(pService->GetNetworkID(),pService->GetTSID(),pService->GetServiceID());
	if (pServiceInfo==NULL)
		return false;

	pService->ClearEvents();

	const CEventInfoList &EventList=pServiceInfo->m_EventList;

	for (auto itrTime=EventList.EventTimeMap.begin();
			itrTime!=EventList.EventTimeMap.end();
			++itrTime) {
		auto itrEvent=EventList.EventDataMap.find(itrTime->EventID);
		if (itrEvent!=EventList.EventDataMap.end())
			pService->AddEvent(new CEventInfoData(itrEvent->second));
	}

	return true;
}


void CProgramGuide::UpdateServiceList()
{
	if (m_ListMode!=LIST_SERVICES) {
		m_ListMode=LIST_SERVICES;
		m_WeekListService=-1;
		if (m_pFrame!=NULL)
			m_pFrame->OnListModeChanged();
	}

	UpdateProgramGuide();

	RestoreTimePos();
}


void CProgramGuide::CalcLayout()
{
	SYSTEMTIME stFirst,stLast;
	GetCurrentTimeRange(&stFirst,&stLast);

	m_EventLayoutList.Clear();
	m_CurEventItem.fSelected=false;

	if (m_ListMode==LIST_SERVICES) {
		for (size_t i=0;i<m_ServiceList.NumServices();i++) {
			ProgramGuide::CServiceInfo *pService=m_ServiceList.GetItem(i);
			ProgramGuide::CEventLayout *pLayout=new ProgramGuide::CEventLayout(pService);

			pService->CalcLayout(
				pLayout,&m_ServiceList,
				stFirst,stLast,m_LinesPerHour);
			m_EventLayoutList.Add(pLayout);
		}
	} else if (m_ListMode==LIST_WEEK) {
		ProgramGuide::CServiceInfo *pCurService=m_ServiceList.GetItem(m_WeekListService);

		if (pCurService!=NULL) {
			for (int i=0;i<8;i++) {
				ProgramGuide::CEventLayout *pLayout=new ProgramGuide::CEventLayout(pCurService);

				pCurService->CalcLayout(
					pLayout,&m_ServiceList,
					stFirst,stLast,m_LinesPerHour);
				m_EventLayoutList.Add(pLayout);
				OffsetSystemTime(&stFirst,TimeConsts::SYSTEMTIME_DAY);
				OffsetSystemTime(&stLast,TimeConsts::SYSTEMTIME_DAY);
			}
		}
	}

	SetTooltip();
}


unsigned int CProgramGuide::GetEventItemStatus(
	const ProgramGuide::CEventItem *pItem,unsigned int Mask) const
{
	const CEventInfoData *pEventInfo=pItem->GetEventInfo();
	const CEventInfoData *pOrigEventInfo=pEventInfo;
	const bool fCommonEvent=pEventInfo->m_bCommonEvent;
	if (fCommonEvent && pItem->GetCommonEventInfo()!=NULL)
		pEventInfo=pItem->GetCommonEventInfo();
	unsigned int Status=0;

	if ((Mask & EVENT_ITEM_STATUS_HIGHLIGHTED)!=0) {
		if (m_ProgramSearch.GetHighlightResult()
				&& m_ProgramSearch.IsHitEvent(pEventInfo))
			Status|=EVENT_ITEM_STATUS_HIGHLIGHTED;
	}

	if ((Mask & EVENT_ITEM_STATUS_CURRENT)!=0) {
		if (m_CurrentEventID!=0
				&& m_CurrentChannel.ServiceID!=0
				&& pOrigEventInfo->m_NetworkID==m_CurrentChannel.NetworkID
				&& pOrigEventInfo->m_TransportStreamID==m_CurrentChannel.TransportStreamID
				&& pOrigEventInfo->m_ServiceID==m_CurrentChannel.ServiceID
				&& pOrigEventInfo->m_EventID==m_CurrentEventID)
			Status|=EVENT_ITEM_STATUS_CURRENT;
	}

	if ((Mask & EVENT_ITEM_STATUS_FILTERED)!=0) {
		const int Genre1=pItem->GetGenre(0);
		const int Genre2=pItem->GetGenre(1);
		bool fFilter=false;

		if ((m_Filter&FILTER_FREE)!=0
				&& pEventInfo->m_bFreeCaMode
				&& GetAppClass().NetworkDefinition.IsSatelliteNetworkID(pEventInfo->m_NetworkID)) {
			fFilter=true;
		} else if ((m_Filter&FILTER_NEWPROGRAM)!=0
				&& (pEventInfo->m_EventName.empty()
					|| pEventInfo->m_EventName.find(TEXT("[新]"))==CEventInfo::String::npos)) {
			fFilter=true;
		} else if ((m_Filter&FILTER_ORIGINAL)!=0
				&& !pEventInfo->m_EventName.empty()
				&& pEventInfo->m_EventName.find(TEXT("[再]"))!=CEventInfo::String::npos) {
			fFilter=true;
		} else if ((m_Filter&FILTER_RERUN)!=0
				&& (pEventInfo->m_EventName.empty()
					|| pEventInfo->m_EventName.find(TEXT("[再]"))==CEventInfo::String::npos)) {
			fFilter=true;
		} else if ((m_Filter&FILTER_NOT_SHOPPING)!=0
				&& Genre1==2 && Genre2==4) {
			fFilter=true;
		} else if ((m_Filter&FILTER_GENRE_MASK)!=0) {
			if (Genre1<0 || (m_Filter&(FILTER_GENRE_FIRST<<Genre1))==0)
				fFilter=true;
			// 映画ジャンルのアニメ
			if ((m_Filter&FILTER_ANIME)!=0 && Genre1==6 && Genre2==2)
				fFilter=false;
		}

		if (fFilter)
			Status|=EVENT_ITEM_STATUS_FILTERED;
	}

	if ((Mask & EVENT_ITEM_STATUS_COMMON)!=0) {
		if (fCommonEvent)
			Status|=EVENT_ITEM_STATUS_COMMON;
	}

	return Status;
}


void CProgramGuide::DrawEventBackground(
	ProgramGuide::CEventItem *pItem,
	HDC hdc,const RECT &Rect,TVTest::CTextDraw &TextDraw,int LineHeight,int CurTimePos)
{
	const CEventInfoData *pEventInfo=pItem->GetEventInfo();
	const CEventInfoData *pOrigEventInfo=pEventInfo;
	const bool fCommonEvent=pEventInfo->m_bCommonEvent;
	if (fCommonEvent && pItem->GetCommonEventInfo()!=NULL)
		pEventInfo=pItem->GetCommonEventInfo();
	const unsigned int ItemStatus=
		GetEventItemStatus(pItem,
						   EVENT_ITEM_STATUS_CURRENT |
						   EVENT_ITEM_STATUS_HIGHLIGHTED |
						   EVENT_ITEM_STATUS_FILTERED);
	const bool fCurrent=(ItemStatus & EVENT_ITEM_STATUS_CURRENT)!=0;
	const bool fHighlighted=(ItemStatus & EVENT_ITEM_STATUS_HIGHLIGHTED)!=0;
	const bool fFiltered=(ItemStatus & EVENT_ITEM_STATUS_FILTERED)!=0;
	const int Genre1=pItem->GetGenre(0);
	const int Genre2=pItem->GetGenre(1);
	COLORREF BackColor=m_EpgTheme.GetGenreColor(Genre1);

	if (!fCurrent) {
		if (fFiltered) {
			BackColor=MixColor(BackColor,m_Theme.ColorList[COLOR_BACK],96);
		} else if (fCommonEvent) {
			BackColor=MixColor(BackColor,m_Theme.ColorList[COLOR_BACK],192);
		}
	}

	if (fHighlighted) {
		DrawUtil::FillGradient(hdc,&Rect,
			MixColor(m_Theme.ColorList[COLOR_HIGHLIGHT_BACK],BackColor,128),
			BackColor,
			DrawUtil::DIRECTION_VERT);
	} else if (fCurrent) {
		m_EpgTheme.DrawContentBackground(hdc,Rect,*pEventInfo,
										 CEpgTheme::DRAW_CONTENT_BACKGROUND_CURRENT);
	} else {
		DrawUtil::Fill(hdc,&Rect,BackColor);
	}

	HPEN hpen=::CreatePen(PS_SOLID,1,
		MixColor(BackColor,RGB(0,0,0),pItem->GetStartTime().wMinute==0?192:224));
	HPEN hpenOld=static_cast<HPEN>(::SelectObject(hdc,hpen));
	::MoveToEx(hdc,Rect.left,Rect.top,NULL);
	::LineTo(hdc,Rect.right,Rect.top);
	::SelectObject(hdc,hpenOld);
	::DeleteObject(hpen);

	// 現在時刻の線
	if (((m_ListMode==LIST_SERVICES && m_Day==DAY_TODAY) || m_ListMode==LIST_WEEK)
			&& CurTimePos>=Rect.top && CurTimePos<Rect.bottom) {
		LOGBRUSH lb;

		lb.lbStyle=BS_SOLID;
		lb.lbColor=MixColor(m_Theme.ColorList[COLOR_CURTIMELINE],BackColor,64);
		hpen=::ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT,
							m_Style.CurTimeLineWidth,&lb,0,NULL);
		::SelectObject(hdc,hpen);
		::MoveToEx(hdc,Rect.left,CurTimePos,NULL);
		::LineTo(hdc,Rect.right,CurTimePos);
		::SelectObject(hdc,hpenOld);
		::DeleteObject(hpen);
	}

	RECT rcTitle,rcText;
	TextDraw.SetFont(m_TitleFont.GetHandle());
	pItem->CalcTitleLines(TextDraw,Rect.right-Rect.left);
	rcTitle=Rect;
	rcTitle.bottom=min(Rect.bottom,Rect.top+pItem->GetTitleLines()*LineHeight);
	rcText.left=Rect.left+m_TextLeftMargin;
	rcText.top=rcTitle.bottom;
	rcText.right=Rect.right-m_Style.EventPadding.Right;
	rcText.bottom=Rect.bottom;

	if (m_pProgramCustomizer!=NULL) {
		if (!fCommonEvent) {
			m_pProgramCustomizer->DrawBackground(*pEventInfo,hdc,Rect,rcTitle,rcText,BackColor);
		} else {
			CEventInfoData Info(*pEventInfo);
			Info.m_ServiceID=pOrigEventInfo->m_ServiceID;
			m_pProgramCustomizer->DrawBackground(Info,hdc,Rect,rcTitle,rcText,BackColor);
		}
	}

	if (fHighlighted) {
		HBRUSH hbr=::CreateSolidBrush(MixColor(m_Theme.ColorList[COLOR_HIGHLIGHT_BORDER],BackColor,80));
		RECT rc=Rect;
		TVTest::Style::Subtract(&rc,m_Style.HighlightBorder);
		DrawUtil::FillBorder(hdc,&Rect,&rc,&Rect,hbr);
		::DeleteObject(hbr);
	}
	if (pItem->IsSelected()) {
		HBRUSH hbr=::CreateSolidBrush(MixColor(m_Theme.ColorList[COLOR_HIGHLIGHT_BORDER],BackColor,128));
		RECT rcOuter=Rect;
		rcOuter.left-=m_Style.SelectedBorder.Left;
		rcOuter.right+=m_Style.SelectedBorder.Right;
		RECT rcInner=Rect;
		rcInner.top+=m_Style.SelectedBorder.Top;
		rcInner.bottom-=m_Style.SelectedBorder.Bottom;
		DrawUtil::FillBorder(hdc,&rcOuter,&rcInner,&rcOuter,hbr);
		::DeleteObject(hbr);
	} else if (fCurrent) {
		HBRUSH hbr=::CreateSolidBrush(m_Theme.ColorList[COLOR_CURTIMELINE]);
		RECT rcOuter=Rect;
		rcOuter.left-=m_Style.SelectedBorder.Left;
		rcOuter.right+=m_Style.SelectedBorder.Right;
		DrawUtil::FillBorder(hdc,&rcOuter,&Rect,&rcOuter,hbr);
		::DeleteObject(hbr);
	}

	if (m_fShowFeaturedMark
			&& m_FeaturedEventsMatcher.IsMatch(*pEventInfo)) {
		SIZE sz;
		RECT rcMark;
		pItem->GetTimeSize(TextDraw,&sz);
		rcMark.left=rcTitle.left;
		rcMark.top=rcTitle.top+m_Style.EventLeading;
		rcMark.right=rcMark.left+sz.cx;
		rcMark.bottom=rcMark.top+sz.cy;
		TVTest::Style::Subtract(&rcMark,m_Style.FeaturedMarkMargin);
		TVTest::Theme::Draw(hdc,rcMark,m_Theme.FeaturedMarkStyle);
	}

	if (rcText.bottom>rcTitle.bottom) {
		const unsigned int ShowIcons=
			CEpgIcons::GetEventIcons(pEventInfo) & m_VisibleEventIcons;
		if (ShowIcons!=0) {
			m_EpgIcons.DrawIcons(
				ShowIcons,hdc,
				Rect.left+m_Style.EventIconMargin.Left,
				rcText.top+m_Style.EventIconMargin.Top,
				m_Style.EventIconSize.Width,
				m_Style.EventIconSize.Height,
				0,m_Style.EventIconSize.Height+m_Style.EventIconMargin.Bottom,
				(!fCurrent && (fCommonEvent || fFiltered))?128:255,
				&Rect);
		}
	}
}


void CProgramGuide::DrawEventText(
	ProgramGuide::CEventItem *pItem,
	HDC hdc,const RECT &Rect,TVTest::CTextDraw &TextDraw,int LineHeight)
{
	const unsigned int ItemStatus=
		GetEventItemStatus(pItem,
						   EVENT_ITEM_STATUS_CURRENT |
						   EVENT_ITEM_STATUS_HIGHLIGHTED |
						   EVENT_ITEM_STATUS_FILTERED);
	COLORREF TitleColor=m_EpgTheme.GetColor(CEpgTheme::COLOR_EVENTNAME);
	COLORREF TextColor=m_EpgTheme.GetColor(CEpgTheme::COLOR_EVENTTEXT);

	if ((ItemStatus & EVENT_ITEM_STATUS_HIGHLIGHTED)!=0) {
		TitleColor=m_Theme.ColorList[COLOR_HIGHLIGHT_TITLE];
		TextColor=m_Theme.ColorList[COLOR_HIGHLIGHT_TEXT];
	}

	if ((ItemStatus & EVENT_ITEM_STATUS_CURRENT)==0) {
		if ((ItemStatus & EVENT_ITEM_STATUS_FILTERED)!=0) {
			TitleColor=MixColor(TitleColor,m_Theme.ColorList[COLOR_BACK],96);
			TextColor=MixColor(TextColor,m_Theme.ColorList[COLOR_BACK],96);
		}
	}

	RECT rcTitle,rcText;
	rcTitle=Rect;
	rcTitle.bottom=min(Rect.bottom,Rect.top+pItem->GetTitleLines()*LineHeight);
	rcText.left=Rect.left+m_TextLeftMargin;
	rcText.top=rcTitle.bottom;
	rcText.right=Rect.right-m_Style.EventPadding.Right;
	rcText.bottom=Rect.bottom;

	rcTitle.top+=m_Style.EventLeading;
	TextDraw.SetFont(m_TitleFont.GetHandle());
	TextDraw.SetTextColor(TitleColor);
	pItem->DrawTitle(TextDraw,rcTitle,LineHeight);

	if (rcText.bottom>rcTitle.bottom) {
		RECT rc=rcText;
		rc.top+=m_Style.EventLeading;
		TextDraw.SetFont(m_Font.GetHandle());
		TextDraw.SetTextColor(TextColor);
		pItem->DrawText(TextDraw,rc,LineHeight);
	}
}


void CProgramGuide::DrawEventList(
	ProgramGuide::CEventLayout *pLayout,
	HDC hdc,const RECT &Rect,const RECT &PaintRect,
	TVTest::CTextDraw &TextDraw,bool fBackground)
{
	const int LineHeight=GetLineHeight();
	const int CurTimePos=Rect.top+GetCurTimeLinePos();

	HFONT hfontOld=static_cast<HFONT>(::GetCurrentObject(hdc,OBJ_FONT));
	COLORREF OldTextColor=::GetTextColor(hdc);

	if (fBackground)
		m_EpgIcons.BeginDraw(hdc,m_Style.EventIconSize.Width,m_Style.EventIconSize.Height);

	RECT rcItem;
	rcItem.left=Rect.left;
	rcItem.right=Rect.right;

	for (size_t i=0;i<pLayout->NumItems();i++) {
		ProgramGuide::CEventItem *pItem=pLayout->GetItem(i);

		if (!pItem->IsNullItem() && pItem->GetItemLines()>0) {
			rcItem.top=Rect.top+pItem->GetItemPos()*LineHeight;
			if (rcItem.top>=PaintRect.bottom)
				break;
			rcItem.bottom=rcItem.top+pItem->GetItemLines()*LineHeight;
			if (rcItem.bottom<=PaintRect.top)
				continue;

			if (fBackground)
				DrawEventBackground(pItem,hdc,rcItem,TextDraw,LineHeight,CurTimePos);
			else
				DrawEventText(pItem,hdc,rcItem,TextDraw,LineHeight);
		}
	}

	if (fBackground)
		m_EpgIcons.EndDraw();

	::SetTextColor(hdc,OldTextColor);
	::SelectObject(hdc,hfontOld);
}


void CProgramGuide::DrawHeaderBackground(HDC hdc,const RECT &Rect,bool fCur) const
{
	const TVTest::Theme::FillStyle &Style=
		fCur?m_Theme.CurChannelNameBackStyle:m_Theme.ChannelNameBackStyle;
	RECT rc;

	rc=Rect;
	rc.left++;
	rc.right--;
	TVTest::Theme::Draw(hdc,rc,Style);

	TVTest::Theme::FillStyle Border;
	Border.Type=TVTest::Theme::FILL_GRADIENT;
	Border.Gradient.Type=
		Style.Type==TVTest::Theme::FILL_GRADIENT?
			Style.Gradient.Type : TVTest::Theme::GRADIENT_NORMAL;
	Border.Gradient.Direction=TVTest::Theme::DIRECTION_VERT;
	Border.Gradient.Color1.Set(255,255,255);
	Border.Gradient.Color2.Set(255,255,255);
	rc=Rect;
	rc.right=rc.left+1;
	TVTest::Theme::Draw(hdc,rc,TVTest::Theme::MixStyle(Style,Border,192));

	Border.Gradient.Color1.Set(0,0,0);
	Border.Gradient.Color2.Set(0,0,0);
	rc=Rect;
	rc.left=rc.right-1;
	TVTest::Theme::Draw(hdc,rc,TVTest::Theme::MixStyle(Style,Border,192));
}


void CProgramGuide::DrawServiceHeader(ProgramGuide::CServiceInfo *pServiceInfo,
									  HDC hdc,const RECT &Rect,int Chevron,
									  bool fLeftAlign)
{
	bool fCur=
		m_CurrentChannel.ServiceID>0
		&& pServiceInfo->GetNetworkID()==m_CurrentChannel.NetworkID
		&& pServiceInfo->GetTSID()==m_CurrentChannel.TransportStreamID
		&& pServiceInfo->GetServiceID()==m_CurrentChannel.ServiceID;

	DrawHeaderBackground(hdc,Rect,fCur);

	HFONT hfontOld=DrawUtil::SelectObject(hdc,m_TitleFont);
	COLORREF TextColor=m_Theme.ColorList[fCur?COLOR_CURCHANNELNAMETEXT:COLOR_CHANNELNAMETEXT];
	COLORREF OldTextColor=::SetTextColor(hdc,TextColor);

	RECT rc=Rect;
	TVTest::Style::Subtract(&rc,m_Style.HeaderPadding);

	HBITMAP hbmLogo=pServiceInfo->GetLogo();
	if (hbmLogo!=NULL) {
		int Height,LogoWidth,LogoHeight;
		Height=(rc.bottom-rc.top)-m_Style.HeaderIconMargin.Vert();
		LogoHeight=min(Height,24);
		LogoWidth=LogoHeight*16/9;
		HBITMAP hbmStretched=pServiceInfo->GetStretchedLogo(LogoWidth,LogoHeight);
		rc.left+=m_Style.HeaderIconMargin.Left;
		DrawUtil::DrawBitmap(
			hdc,
			rc.left,
			rc.top+m_Style.HeaderIconMargin.Top+((rc.bottom-rc.top)-Height)/2,
			LogoWidth,LogoHeight,
			hbmStretched!=NULL?hbmStretched:hbmLogo,NULL,192);
		rc.left+=LogoWidth+m_Style.HeaderIconMargin.Right;
	}

	rc.right-=m_Style.HeaderChevronSize.Width+m_Style.HeaderChevronMargin.Right;
	m_Chevron.Draw(
		hdc,
		rc.right,
		rc.top+m_Style.HeaderChevronMargin.Top+
			(((rc.bottom-rc.top)-m_Style.HeaderChevronMargin.Vert())-m_Style.HeaderChevronSize.Height)/2,
		m_Style.HeaderChevronSize.Width,m_Style.HeaderChevronSize.Height,
		Chevron,TextColor);
	rc.right-=m_Style.HeaderChevronMargin.Left;

	TVTest::Style::Subtract(&rc,m_Style.HeaderChannelNameMargin);
	::DrawText(hdc,pServiceInfo->GetServiceName(),-1,&rc,
			   (fLeftAlign || hbmLogo!=NULL?DT_LEFT:DT_CENTER) |
			   DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
	::SetTextColor(hdc,OldTextColor);
	::SelectObject(hdc,hfontOld);
}


void CProgramGuide::DrawDayHeader(int Day,HDC hdc,const RECT &Rect) const
{
	SYSTEMTIME st;
	GetCurrentTimeRange(&st,NULL);
	if (Day>0)
		OffsetSystemTime(&st,Day*TimeConsts::SYSTEMTIME_DAY);

	DrawHeaderBackground(hdc,Rect,false);

	HFONT hfontOld=DrawUtil::SelectObject(hdc,m_TitleFont);
	COLORREF OldTextColor=::SetTextColor(hdc,m_Theme.ColorList[COLOR_CHANNELNAMETEXT]);
	TCHAR szText[64];
	StdUtil::snprintf(szText,lengthof(szText),TEXT("%d/%d(%s)"),
					  st.wMonth,st.wDay,GetDayOfWeekText(st.wDayOfWeek));
	RECT rc=Rect;
	rc.left+=m_Style.HeaderPadding.Left;
	rc.right-=m_Style.HeaderPadding.Right;
	::DrawText(hdc,szText,-1,&rc,
			   DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
	::SetTextColor(hdc,OldTextColor);
	::SelectObject(hdc,hfontOld);
}


void CProgramGuide::DrawTimeBar(HDC hdc,const RECT &Rect,bool fRight)
{
	const int PixelsPerHour=GetLineHeight()*m_LinesPerHour;
	const int CurTimePos=Rect.top+GetCurTimeLinePos();
	HFONT hfontOld;
	COLORREF crOldTextColor;
	HPEN hpen,hpenOld;
	RECT rc;

	hfontOld=DrawUtil::SelectObject(hdc,m_TimeFont);
	crOldTextColor=::SetTextColor(hdc,m_Theme.ColorList[COLOR_TIMETEXT]);
	hpen=::CreatePen(PS_SOLID,0,m_Theme.ColorList[COLOR_TIMETEXT]);
	hpenOld=SelectPen(hdc,hpen);
	rc.left=Rect.left;
	rc.top=Rect.top;
	rc.right=Rect.right;

	SYSTEMTIME Time;
	GetCurrentTimeRange(&Time,NULL);

	for (int i=0;i<m_Hours;i++) {
		SYSTEMTIME DispTime;

		if (i>0)
			OffsetSystemTime(&Time,TimeConsts::SYSTEMTIME_HOUR);
		EpgUtil::EpgTimeToDisplayTime(Time,&DispTime);

		rc.bottom=rc.top+PixelsPerHour;
		TVTest::Theme::Draw(hdc,rc,m_Theme.TimeBarBackStyle[DispTime.wHour/3]);
		::MoveToEx(hdc,rc.left,rc.top,NULL);
		::LineTo(hdc,rc.right,rc.top);

		if (((m_ListMode==LIST_SERVICES && m_Day==DAY_TODAY) || m_ListMode==LIST_WEEK)
				&& CurTimePos>=rc.top && CurTimePos<rc.bottom) {
			const int TriangleHeight=m_GDIFontHeight*2/3;
			const int TriangleWidth=TriangleHeight*8/10;
			POINT ptTriangle[3];
			HBRUSH hbr,hbrOld;

			hbr=::CreateSolidBrush(m_Theme.ColorList[COLOR_CURTIMELINE]);
			hbrOld=SelectBrush(hdc,hbr);
			SelectObject(hdc,::CreatePen(PS_SOLID,1,m_Theme.ColorList[COLOR_CURTIMELINE]));
			if (fRight) {
				ptTriangle[0].x=rc.left;
				ptTriangle[0].y=CurTimePos;
				ptTriangle[1].x=ptTriangle[0].x+TriangleWidth;
				ptTriangle[1].y=ptTriangle[0].y-TriangleHeight/2;
				ptTriangle[2].x=ptTriangle[0].x+TriangleWidth;
				ptTriangle[2].y=ptTriangle[1].y+TriangleHeight;
			} else {
				ptTriangle[0].x=rc.right-1;
				ptTriangle[0].y=CurTimePos;
				ptTriangle[1].x=ptTriangle[0].x-TriangleWidth;
				ptTriangle[1].y=ptTriangle[0].y-TriangleHeight/2;
				ptTriangle[2].x=ptTriangle[0].x-TriangleWidth;
				ptTriangle[2].y=ptTriangle[1].y+TriangleHeight;
			}
			::Polygon(hdc,ptTriangle,3);
			::SelectObject(hdc,hbrOld);
			::DeleteObject(hbr);
			::DeleteObject(::SelectObject(hdc,hpen));
		}

		TCHAR szText[64];
		if (m_ListMode==LIST_SERVICES && (i==0 || DispTime.wHour%3==0)) {
			StdUtil::snprintf(szText,lengthof(szText),TEXT("%d/%d(%s) %d時"),
							  DispTime.wMonth,DispTime.wDay,
							  GetDayOfWeekText(DispTime.wDayOfWeek),
							  DispTime.wHour);
		} else {
			StdUtil::snprintf(szText,lengthof(szText),TEXT("%d"),DispTime.wHour);
		}
		::TextOut(hdc,
				  rc.right-m_Style.TimeBarPadding.Right,
				  rc.top+m_Style.TimeBarPadding.Top,
				  szText,lstrlen(szText));
		rc.top=rc.bottom;
	}

	if (m_ListMode==LIST_SERVICES && m_Day<DAY_LAST) {
		// ▼
		RECT rcClient;

		GetClientRect(&rcClient);
		if (rc.top-m_TimeBarWidth<rcClient.bottom) {
			const int TriangleWidth=m_GDIFontHeight*2/3;
			const int TriangleHeight=TriangleWidth*8/10;
			POINT ptTriangle[3];
			HBRUSH hbr,hbrOld;

			hbr=::CreateSolidBrush(m_Theme.ColorList[COLOR_TIMETEXT]);
			hbrOld=SelectBrush(hdc,hbr);
			ptTriangle[0].x=m_TimeBarWidth/2;
			ptTriangle[0].y=rc.top-(m_TimeBarWidth-TriangleHeight)/2;
			ptTriangle[1].x=ptTriangle[0].x-TriangleWidth/2;
			ptTriangle[1].y=ptTriangle[0].y-TriangleHeight;
			ptTriangle[2].x=ptTriangle[0].x+TriangleWidth/2;
			ptTriangle[2].y=ptTriangle[1].y;
			::Polygon(hdc,ptTriangle,3);
			for (int i=0;i<3;i++)
				ptTriangle[i].x+=rcClient.right-m_TimeBarWidth;
			::Polygon(hdc,ptTriangle,3);
			::SelectObject(hdc,hbrOld);
			::DeleteObject(hbr);
		}
	}

	::SelectObject(hdc,hpenOld);
	::DeleteObject(hpen);
	::SetTextColor(hdc,crOldTextColor);
	SelectFont(hdc,hfontOld);
}


void CProgramGuide::DrawMessage(HDC hdc,const RECT &ClientRect) const
{
	if (!m_Message.empty()) {
		RECT rc;
		HFONT hfontOld=DrawUtil::SelectObject(hdc,m_Font);
		::SetRectEmpty(&rc);
		::DrawText(hdc,m_Message.c_str(),-1,&rc,DT_NOPREFIX | DT_CALCRECT);
		::OffsetRect(&rc,
					 (ClientRect.right-(rc.right-rc.left))/2,
					 (ClientRect.bottom-(rc.bottom-rc.top))/2);
#if 0
		HGDIOBJ hOldBrush=::SelectObject(hdc,::GetSysColorBrush(COLOR_WINDOW));
		HPEN hpen=::CreatePen(PS_INSIDEFRAME,1,::GetSysColor(COLOR_WINDOWBORDER));
		HGDIOBJ hOldPen=::SelectObject(hdc,hpen);
		::RoundRect(hdc,rc.left-16,rc.top-8,rc.right+16,rc.bottom+8,16,16);
		::SelectObject(hdc,hOldPen);
		::DeleteObject(hpen);
		::SelectObject(hdc,hOldBrush);
		COLORREF OldTextColor=::SetTextColor(hdc,::GetSysColor(COLOR_WINDOWTEXT));
#else
		RECT rcBack;
		rcBack.left=rc.left-24;
		rcBack.top=rc.top-12;
		rcBack.right=rc.right+24;
		rcBack.bottom=rc.bottom+12;
		DrawUtil::FillGradient(hdc,&rcBack,
							   DrawUtil::RGBA(255,255,255,224),
							   DrawUtil::RGBA(255,255,255,255),
							   DrawUtil::DIRECTION_VERT);
		HGDIOBJ hOldBrush=::SelectObject(hdc,::GetStockObject(NULL_BRUSH));
		HPEN hpen=::CreatePen(PS_INSIDEFRAME,2,RGB(208,208,208));
		HGDIOBJ hOldPen=::SelectObject(hdc,hpen);
		::Rectangle(hdc,rcBack.left,rcBack.top,rcBack.right,rcBack.bottom);
		::SelectObject(hdc,hOldPen);
		::DeleteObject(hpen);
		::SelectObject(hdc,hOldBrush);
		rcBack.top=rcBack.bottom;
		rcBack.bottom=rcBack.top+6;
		DrawUtil::FillGradient(hdc,&rcBack,
							   DrawUtil::RGBA(0,0,0,32),
							   DrawUtil::RGBA(0,0,0,0),
							   DrawUtil::DIRECTION_VERT);
		COLORREF OldTextColor=::SetTextColor(hdc,RGB(0,0,0));
#endif
		int OldBkMode=::SetBkMode(hdc,TRANSPARENT);
		::DrawText(hdc,m_Message.c_str(),-1,&rc,DT_NOPREFIX);
		::SetBkMode(hdc,OldBkMode);
		::SetTextColor(hdc,OldTextColor);
		::SelectObject(hdc,hfontOld);
	}
}


void CProgramGuide::Draw(HDC hdc,const RECT &PaintRect)
{
	RECT rcClient,rcGuide,rc;
	HRGN hrgn;

	::GetClientRect(m_hwnd,&rcClient);
	GetProgramGuideRect(&rcGuide);

	if (::IntersectRect(&rc,&rcGuide,&PaintRect)) {
		HBRUSH hbr=::CreateSolidBrush(m_Theme.ColorList[COLOR_BACK]);
		::FillRect(hdc,&rc,hbr);
		::DeleteObject(hbr);
	}

	int OldBkMode=::SetBkMode(hdc,TRANSPARENT);

	int HeaderHeight=m_HeaderHeight;
	if (m_ListMode==LIST_WEEK)
		HeaderHeight+=m_HeaderHeight;

	if (m_EventLayoutList.Length()>0) {
		if (PaintRect.top<HeaderHeight) {
			rc.left=rcClient.left+m_TimeBarWidth;
			rc.top=0;
			rc.right=rcClient.right-m_TimeBarWidth;
			rc.bottom=HeaderHeight;
			hrgn=::CreateRectRgnIndirect(&rc);
			::SelectClipRgn(hdc,hrgn);
			if (m_ListMode==LIST_SERVICES) {
				rc.left=m_TimeBarWidth-m_ScrollPos.x;
				for (size_t i=0;i<m_ServiceList.NumServices();i++) {
					rc.right=rc.left+(m_ItemWidth+m_Style.ColumnMargin*2);
					if (rc.left<PaintRect.right && rc.right>PaintRect.left)
						DrawServiceHeader(m_ServiceList.GetItem(i),hdc,rc,2);
					rc.left=rc.right;
				}
			} else if (m_ListMode==LIST_WEEK) {
				rc.bottom=m_HeaderHeight;
				DrawServiceHeader(m_ServiceList.GetItem(m_WeekListService),hdc,rc,3,true);
				rc.left=m_TimeBarWidth-m_ScrollPos.x;
				rc.top=rc.bottom;
				rc.bottom+=m_HeaderHeight;
				for (int i=0;i<(int)m_EventLayoutList.Length();i++) {
					rc.right=rc.left+(m_ItemWidth+m_Style.ColumnMargin*2);
					if (rc.left<PaintRect.right && rc.right>PaintRect.left)
						DrawDayHeader(i,hdc,rc);
					rc.left=rc.right;
				}
			}
			if (rc.left<PaintRect.right) {
				rc.right=PaintRect.right;
				TVTest::Theme::Draw(hdc,rc,m_Theme.ChannelNameBackStyle);
			}
			::SelectClipRgn(hdc,NULL);
			::DeleteObject(hrgn);
		}

		rc.top=HeaderHeight-m_ScrollPos.y*GetLineHeight();
		if (rc.top<PaintRect.bottom) {
			hrgn=::CreateRectRgnIndirect(&rcGuide);
			::SelectClipRgn(hdc,hrgn);

			TVTest::CTextDraw TextDraw;
			m_TextDrawClient.InitializeTextDraw(&TextDraw);
			TextDraw.BindDC(hdc,rcClient);

			HPEN hpen,hpenOld;
			hpen=::CreatePen(PS_SOLID,0,m_Theme.ColorList[COLOR_TIMELINE]);
			hpenOld=SelectPen(hdc,hpen);

			const int PixelsPerHour=GetLineHeight()*m_LinesPerHour;
			const int CurTimePos=rc.top+GetCurTimeLinePos();
			const int XOrigin=m_TimeBarWidth+m_Style.ColumnMargin-m_ScrollPos.x;

			// 番組背景の描画
			rc.left=XOrigin;

			for (size_t i=0;i<m_EventLayoutList.Length();i++) {
				rc.right=rc.left+m_ItemWidth;
				for (int j=0;j<m_Hours;j++) {
					int y=rc.top+j*PixelsPerHour;
					if (y>=PaintRect.top && y<PaintRect.bottom) {
						/*
						::MoveToEx(hdc,rc.left-m_Style.ColumnMargin,y,NULL);
						::LineTo(hdc,rc.left,y);
						::MoveToEx(hdc,rc.right,y,NULL);
						::LineTo(hdc,rc.right+m_Style.ColumnMargin,y);
						*/
						::MoveToEx(hdc,rc.left-m_Style.ColumnMargin,y,NULL);
						::LineTo(hdc,rc.right+m_Style.ColumnMargin,y);
					}
					if (((m_ListMode==LIST_SERVICES && m_Day==DAY_TODAY) || m_ListMode==LIST_WEEK)
							&& CurTimePos>=y && CurTimePos<y+PixelsPerHour) {
						HPEN hpenCurTime;
						LOGBRUSH lb;

						lb.lbStyle=BS_SOLID;
						lb.lbColor=m_Theme.ColorList[COLOR_CURTIMELINE];
						hpenCurTime=::ExtCreatePen(
							PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT,
							m_Style.CurTimeLineWidth,&lb,0,NULL);
						::SelectObject(hdc,hpenCurTime);
						::MoveToEx(hdc,rc.left-m_Style.ColumnMargin,CurTimePos,NULL);
						::LineTo(hdc,rc.right+m_Style.ColumnMargin,CurTimePos);
						::SelectObject(hdc,hpen);
						::DeleteObject(hpenCurTime);
					}
				}
				if (rc.left<PaintRect.right && rc.right>PaintRect.left)
					DrawEventList(m_EventLayoutList[i],hdc,rc,PaintRect,TextDraw,true);
				rc.left=rc.right+m_Style.ColumnMargin*2;
			}
			::SelectObject(hdc,hpenOld);
			::DeleteObject(hpen);

			// 番組テキストの描画
			TextDraw.Begin(hdc,rcClient,
						   TVTest::CTextDraw::FLAG_END_ELLIPSIS |
						   TVTest::CTextDraw::FLAG_JAPANESE_HYPHNATION);
			TextDraw.SetClippingRect(rcGuide);

			rc.left=XOrigin;
			for (size_t i=0;i<m_EventLayoutList.Length();i++) {
				rc.right=rc.left+m_ItemWidth;
				if (rc.left<PaintRect.right && rc.right>PaintRect.left)
					DrawEventList(m_EventLayoutList[i],hdc,rc,PaintRect,TextDraw,false);
				rc.left=rc.right+m_Style.ColumnMargin*2;
			}

			TextDraw.ResetClipping();
			TextDraw.End();

			::SelectClipRgn(hdc,NULL);
			::DeleteObject(hrgn);
		}
	} else {
		if (PaintRect.top<m_HeaderHeight) {
			rc.left=max(PaintRect.left,m_TimeBarWidth);
			rc.right=min(PaintRect.right,rcClient.right-m_TimeBarWidth);
			if (rc.left<rc.right) {
				rc.top=0;
				rc.bottom=m_HeaderHeight;
				TVTest::Theme::Draw(hdc,rc,m_Theme.ChannelNameBackStyle);
			}
		}
	}

	rc.left=0;
	rc.top=HeaderHeight;
	rc.right=rcClient.right;
	rc.bottom=rcClient.bottom;
	hrgn=::CreateRectRgnIndirect(&rc);
	::SelectClipRgn(hdc,hrgn);
	rc.top=HeaderHeight-m_ScrollPos.y*GetLineHeight();
		rc.bottom=rc.top+GetLineHeight()*m_LinesPerHour*m_Hours;
	if (PaintRect.left<m_TimeBarWidth) {
		rc.left=0;
		rc.right=m_TimeBarWidth;
		DrawTimeBar(hdc,rc,false);
	}
	rc.left=rcClient.right-m_TimeBarWidth;
	if (rc.left<PaintRect.right) {
		rc.right=rcClient.right;
		DrawTimeBar(hdc,rc,true);
	}
	::SelectClipRgn(hdc,NULL);
	::DeleteObject(hrgn);

	if (rc.bottom<PaintRect.bottom) {
		::SetRect(&rc,0,rc.bottom,m_TimeBarWidth,rcClient.bottom);
		TVTest::Theme::Draw(hdc,rc,m_Theme.TimeBarMarginStyle);
		::OffsetRect(&rc,rcGuide.right,0);
		TVTest::Theme::Draw(hdc,rc,m_Theme.TimeBarMarginStyle);
	}
	if (PaintRect.top<HeaderHeight) {
		::SetRect(&rc,0,0,m_TimeBarWidth,HeaderHeight);
		TVTest::Theme::Draw(hdc,rc,m_Theme.TimeBarMarginStyle);
		::OffsetRect(&rc,rcGuide.right,0);
		TVTest::Theme::Draw(hdc,rc,m_Theme.TimeBarMarginStyle);
	}

	if (m_ListMode==LIST_SERVICES && m_Day!=DAY_TODAY
			&& PaintRect.top<m_HeaderHeight) {
		// ▲
		const int TriangleWidth=m_GDIFontHeight*2/3;
		const int TriangleHeight=TriangleWidth*8/10;
		POINT ptTriangle[3];
		HPEN hpen,hpenOld;
		HBRUSH hbr,hbrOld;

		hbr=::CreateSolidBrush(m_Theme.ColorList[COLOR_TIMETEXT]);
		hpen=::CreatePen(PS_SOLID,0,m_Theme.ColorList[COLOR_TIMETEXT]);
		hbrOld=SelectBrush(hdc,hbr);
		hpenOld=SelectPen(hdc,hpen);
		ptTriangle[0].x=m_TimeBarWidth/2;
		ptTriangle[0].y=(m_HeaderHeight-TriangleHeight)/2;
		ptTriangle[1].x=ptTriangle[0].x-TriangleWidth/2;
		ptTriangle[1].y=ptTriangle[0].y+TriangleHeight;
		ptTriangle[2].x=ptTriangle[0].x+TriangleWidth/2;
		ptTriangle[2].y=ptTriangle[1].y;
		::Polygon(hdc,ptTriangle,3);
		for (int i=0;i<3;i++)
			ptTriangle[i].x+=rcClient.right-m_TimeBarWidth;
		::Polygon(hdc,ptTriangle,3);
		::SelectObject(hdc,hbrOld);
		::SelectObject(hdc,hpenOld);
		::DeleteObject(hbr);
		::DeleteObject(hpen);
	}

	if (m_fBarShadow) {
		if (m_Style.HeaderShadowHeight>0) {
			rc.left=rcGuide.left;
			rc.top=rcGuide.top;
			rc.right=rcGuide.right;
			//rc.bottom=min(rc.top+m_Style.HeaderShadowHeight,rcGuide.bottom);
			rc.bottom=rc.top+m_Style.HeaderShadowHeight;
			DrawUtil::FillGradient(hdc,&rc,DrawUtil::RGBA(0,0,0,80),DrawUtil::RGBA(0,0,0,0),
								   DrawUtil::DIRECTION_VERT);
		}

		if (m_Style.TimeBarShadowWidth>0) {
			rc.top=rcGuide.top;
			rc.bottom=rcGuide.bottom;
			rc.left=rcGuide.left;
			rc.right=min(rc.left+m_Style.TimeBarShadowWidth,rcGuide.right);
			DrawUtil::FillGradient(hdc,&rc,DrawUtil::RGBA(0,0,0,64),DrawUtil::RGBA(0,0,0,0),
								   DrawUtil::DIRECTION_HORZ);
			rc.right=rcGuide.right;
			rc.left=max(rc.right-m_Style.TimeBarShadowWidth,rcGuide.left);
			DrawUtil::FillGradient(hdc,&rc,DrawUtil::RGBA(0,0,0,0),DrawUtil::RGBA(0,0,0,48),
								   DrawUtil::DIRECTION_HORZ);
		}
	}

	DrawMessage(hdc,rcClient);

	::SetBkMode(hdc,OldBkMode);
}


void CProgramGuide::CalcFontMetrics()
{
	if (m_hwnd!=NULL) {
		HDC hdc=::GetDC(m_hwnd);

		m_GDIFontHeight=m_Font.GetHeight(hdc);

		{
			TVTest::CTextDraw TextDraw;
			TVTest::CTextDraw::FontMetrics FontMetrics;
			RECT rc;

			m_TextDrawClient.InitializeTextDraw(&TextDraw);
			GetClientRect(&rc);
			TextDraw.BindDC(hdc,rc);
			TextDraw.SetFont(m_Font.GetHandle());
			if (TextDraw.GetFontMetrics(&FontMetrics))
				m_FontHeight=FontMetrics.Height;
			else
				m_FontHeight=m_GDIFontHeight;
		}

		::ReleaseDC(m_hwnd,hdc);

		m_HeaderHeight=CalcHeaderHeight();
		m_TimeBarWidth=m_GDIFontHeight+m_Style.TimeBarPadding.Horz();
	}
}


int CProgramGuide::GetLineHeight() const
{
	return m_FontHeight+m_Style.EventLeading+m_Style.EventLineSpacing;
}


int CProgramGuide::CalcHeaderHeight() const
{
	int NameHeight=m_GDIFontHeight+m_Style.HeaderChannelNameMargin.Vert();
	int ChevronHeight=m_Style.HeaderChevronSize.Height+m_Style.HeaderChevronMargin.Vert();

	return max(NameHeight,ChevronHeight)+m_Style.HeaderPadding.Vert();
}


int CProgramGuide::GetCurTimeLinePos() const
{
	SYSTEMTIME stFirst;
	LONGLONG Span;

	GetCurrentTimeRange(&stFirst,NULL);
	Span=DiffSystemTime(&stFirst,&m_stCurTime)%(24LL*TimeConsts::SYSTEMTIME_HOUR);
	if (Span<0)
		Span+=24*TimeConsts::SYSTEMTIME_HOUR;
	return (int)(Span*(LONGLONG)(GetLineHeight()*m_LinesPerHour)/TimeConsts::SYSTEMTIME_HOUR);
}


void CProgramGuide::GetProgramGuideRect(RECT *pRect) const
{
	GetClientRect(pRect);
	pRect->left+=m_TimeBarWidth;
	pRect->right-=m_TimeBarWidth;
	pRect->top+=m_HeaderHeight;
	if (m_ListMode==LIST_WEEK)
		pRect->top+=m_HeaderHeight;
}


void CProgramGuide::GetProgramGuideSize(SIZE *pSize) const
{
	pSize->cx=(m_ItemWidth+m_Style.ColumnMargin*2)*(int)m_EventLayoutList.Length();
	pSize->cy=m_LinesPerHour*m_Hours;
}


void CProgramGuide::GetPageSize(SIZE *pSize) const
{
	RECT rc;

	GetProgramGuideRect(&rc);
	pSize->cx=max(rc.right-rc.left,0);
	pSize->cy=max(rc.bottom-rc.top,0)/GetLineHeight();
}


void CProgramGuide::Scroll(int XScroll,int YScroll)
{
	POINT Pos=m_ScrollPos;
	RECT rcGuide;
	SIZE GuideSize,PageSize;
	SCROLLINFO si;
	int XScrollSize=0,YScrollSize=0;

	GetProgramGuideRect(&rcGuide);
	GetProgramGuideSize(&GuideSize);
	GetPageSize(&PageSize);
	si.cbSize=sizeof(SCROLLINFO);
	si.fMask=SIF_POS;
	if (XScroll!=0) {
		Pos.x=m_ScrollPos.x+XScroll;
		if (Pos.x<0)
			Pos.x=0;
		else if (Pos.x>max(GuideSize.cx-PageSize.cx,0))
			Pos.x=max(GuideSize.cx-PageSize.cx,0);
		si.nPos=Pos.x;
		::SetScrollInfo(m_hwnd,SB_HORZ,&si,TRUE);
		XScrollSize=m_ScrollPos.x-Pos.x;
	}
	if (YScroll!=0) {
		Pos.y=m_ScrollPos.y+YScroll;
		if (Pos.y<0)
			Pos.y=0;
		else if (Pos.y>max(GuideSize.cy-PageSize.cy,0))
			Pos.y=max(GuideSize.cy-PageSize.cy,0);
		si.nPos=Pos.y;
		::SetScrollInfo(m_hwnd,SB_VERT,&si,TRUE);
		YScrollSize=(m_ScrollPos.y-Pos.y)*GetLineHeight();
	}

	m_ScrollPos=Pos;

	if (!CBufferedPaint::IsSupported() && m_Message.empty()) {
		RECT rcClip=rcGuide;
		if (m_fBarShadow) {
			rcClip.top+=m_Style.HeaderShadowHeight;
			rcClip.left+=m_Style.TimeBarShadowWidth;
			rcClip.right-=m_Style.TimeBarShadowWidth;
		}
		if (rcClip.right>rcClip.left && rcClip.bottom>rcClip.top
				&& abs(YScrollSize)<rcClip.bottom-rcClip.top
				&& abs(XScrollSize)<rcClip.right-rcClip.left) {
			::ScrollWindowEx(m_hwnd,XScrollSize,YScrollSize,&rcGuide,&rcClip,
							 NULL,NULL,SW_INVALIDATE);

			if (m_fBarShadow) {
				RECT rc;
				rc.left=rcGuide.left;
				rc.top=rcGuide.top;
				rc.right=rcGuide.right;
				rc.bottom=rcClip.top;
				::InvalidateRect(m_hwnd,&rc,FALSE);
				rc.top=rcGuide.top;
				rc.bottom=rcGuide.bottom;
				rc.left=rcGuide.left;
				rc.right=rcClip.left;
				::InvalidateRect(m_hwnd,&rc,FALSE);
				rc.left=rcClip.right;
				rc.right=rcGuide.right;
				::InvalidateRect(m_hwnd,&rc,FALSE);
			}

			if (XScrollSize!=0) {
				RECT rcHeader;

				::SetRect(&rcHeader,rcGuide.left,0,rcGuide.right,rcGuide.top);
				if (m_ListMode==LIST_WEEK)
					rcHeader.top+=m_HeaderHeight;
				::ScrollWindowEx(m_hwnd,XScrollSize,0,&rcHeader,&rcHeader,NULL,NULL,SW_INVALIDATE);
			}

			if (YScrollSize!=0) {
				RECT rcTime;

				::SetRect(&rcTime,0,rcGuide.top,rcGuide.left,rcGuide.bottom);
				::ScrollWindowEx(m_hwnd,0,YScrollSize,&rcTime,&rcTime,NULL,NULL,SW_INVALIDATE);
				rcTime.left=rcGuide.right;
				rcTime.right=rcTime.left+m_TimeBarWidth;
				::ScrollWindowEx(m_hwnd,0,YScrollSize,&rcTime,&rcTime,NULL,NULL,SW_INVALIDATE);
			}
		} else {
			Invalidate();
		}
	} else {
		Invalidate();
	}

	SetTooltip();
}


void CProgramGuide::SetScrollPos(const POINT &Pos)
{
	if (Pos.x!=m_ScrollPos.x || Pos.y!=m_ScrollPos.y)
		Scroll(Pos.x-m_ScrollPos.x,Pos.y-m_ScrollPos.y);
}


void CProgramGuide::SetScrollBar()
{
	SCROLLINFO si;
	RECT rc;

	GetProgramGuideRect(&rc);
	si.cbSize=sizeof(SCROLLINFO);
	si.fMask=SIF_PAGE | SIF_RANGE | SIF_POS | SIF_DISABLENOSCROLL;
	si.nMin=0;
	si.nMax=m_Hours*m_LinesPerHour-1;
	si.nPage=(rc.bottom-rc.top)/GetLineHeight();
	si.nPos=m_ScrollPos.y;
	::SetScrollInfo(m_hwnd,SB_VERT,&si,TRUE);
	si.nMax=(int)m_EventLayoutList.Length()*(m_ItemWidth+m_Style.ColumnMargin*2)-1;
	si.nPage=rc.right-rc.left;
	si.nPos=m_ScrollPos.x;
	::SetScrollInfo(m_hwnd,SB_HORZ,&si,TRUE);
}


int CProgramGuide::GetTimePos() const
{
	SYSTEMTIME stBegin;

	GetCurrentTimeRange(&stBegin,NULL);
	return stBegin.wHour*m_LinesPerHour+m_ScrollPos.y;
}


bool CProgramGuide::SetTimePos(int Pos)
{
	SYSTEMTIME stBegin,stEnd,st;

	GetCurrentTimeRange(&stBegin,&stEnd);
	st=stBegin;
	OffsetSystemTime(&st,(LONGLONG)(Pos*60/m_LinesPerHour-stBegin.wHour*60)*TimeConsts::SYSTEMTIME_MINUTE);
	if (CompareSystemTime(&st,&stBegin)<0)
		OffsetSystemTime(&st,TimeConsts::SYSTEMTIME_DAY);
	else if (CompareSystemTime(&st,&stEnd)>=0)
		OffsetSystemTime(&st,-TimeConsts::SYSTEMTIME_DAY);
	return ScrollToTime(st);
}


void CProgramGuide::StoreTimePos()
{
	m_CurTimePos=GetTimePos();
}


void CProgramGuide::RestoreTimePos()
{
	if (m_fKeepTimePos)
		SetTimePos(m_CurTimePos);
}


void CProgramGuide::SetCaption()
{
	if (m_hwnd!=NULL && m_pFrame!=NULL) {
		if (m_pProgramList!=NULL) {
			if (m_fEpgUpdating) {
				TCHAR szText[256];

				StdUtil::snprintf(szText,lengthof(szText),
					TITLE_TEXT TEXT(" - 番組表の取得中... [%d/%d] 残り約%d分"),
					m_EpgUpdateProgress.Pos+1,m_EpgUpdateProgress.End,
					(m_EpgUpdateProgress.RemainingTime+59999)/60000);
				m_pFrame->SetCaption(szText);
			} else {
				DateInfo Info;
				TCHAR szText[256];

				GetCurrentDateInfo(&Info);
				if (m_ListMode==LIST_SERVICES) {
					OffsetSystemTime(&Info.EndTime,-TimeConsts::SYSTEMTIME_HOUR);
					StdUtil::snprintf(szText,lengthof(szText),
						TITLE_TEXT TEXT(" - %s%s%d/%d(%s) %d時 ～ %d/%d(%s) %d時"),
						Info.pszRelativeDayText!=NULL?Info.pszRelativeDayText:TEXT(""),
						Info.pszRelativeDayText!=NULL?TEXT(" "):TEXT(""),
						Info.BeginningTime.wMonth,
						Info.BeginningTime.wDay,
						GetDayOfWeekText(Info.BeginningTime.wDayOfWeek),
						Info.BeginningTime.wHour,
						Info.EndTime.wMonth,
						Info.EndTime.wDay,
						GetDayOfWeekText(Info.EndTime.wDayOfWeek),
						Info.EndTime.wHour);
				} else {
					SYSTEMTIME stLast=Info.BeginningTime;
					OffsetSystemTime(&stLast,6LL*TimeConsts::SYSTEMTIME_DAY);
					StdUtil::snprintf(szText,lengthof(szText),
						TITLE_TEXT TEXT(" - %s %d/%d(%s) ～ %d/%d(%s)"),
						m_ServiceList.GetItem(m_WeekListService)->GetServiceName(),
						Info.BeginningTime.wMonth,
						Info.BeginningTime.wDay,
						GetDayOfWeekText(Info.BeginningTime.wDayOfWeek),
						stLast.wMonth,
						stLast.wDay,
						GetDayOfWeekText(stLast.wDayOfWeek));
				}
				m_pFrame->SetCaption(szText);
			}
		} else {
			m_pFrame->SetCaption(TITLE_TEXT);
		}
	}
}


void CProgramGuide::SetTooltip()
{
	RECT rc;

	if (m_ListMode==LIST_SERVICES) {
		int NumTools=m_Tooltip.NumTools();
		int NumServices=(int)m_ServiceList.NumServices();

		RECT rcClient,rcHeader;
		GetClientRect(&rcClient);
		rcHeader.left=m_TimeBarWidth;
		rcHeader.right=rcClient.right-m_TimeBarWidth;
		rcHeader.top=0;
		rcHeader.bottom=m_HeaderHeight;

		rc.left=m_TimeBarWidth-m_ScrollPos.x;
		rc.top=0;
		rc.bottom=m_HeaderHeight;
		int ToolCount=0;
		for (int i=0;i<NumServices;i++) {
			rc.right=rc.left+(m_ItemWidth+m_Style.ColumnMargin*2);
			rc.left=rc.right-(m_Style.HeaderPadding.Right+m_Style.HeaderChevronMargin.Right+m_Style.HeaderChevronSize.Width);
			if (rc.left>=rcHeader.right)
				break;
			if (rc.right>rcHeader.left) {
				RECT rcTool;

				::IntersectRect(&rcTool,&rc,&rcHeader);
				if (ToolCount<NumTools) {
					m_Tooltip.SetText(ToolCount,TEXT("1週間表示"));
					m_Tooltip.SetToolRect(ToolCount,rcTool);
				} else {
					m_Tooltip.AddTool(ToolCount,rcTool,TEXT("1週間表示"));
				}
				ToolCount++;
			}
			rc.left=rc.right;
		}

		while (NumTools>ToolCount) {
			m_Tooltip.DeleteTool(--NumTools);
		}

		if (m_Day>DAY_TODAY) {
			rc.top=0;
			rc.bottom=m_HeaderHeight;
			rc.left=0;
			rc.right=m_TimeBarWidth;
			m_Tooltip.AddTool(ToolCount++,rc,TEXT("一日前へ"));
			rc.left=rcClient.right-m_TimeBarWidth;
			rc.right=rcClient.right;
			m_Tooltip.AddTool(ToolCount++,rc,TEXT("一日前へ"));
		}
		if (m_Day<DAY_LAST) {
			int y=m_HeaderHeight+
				(m_Hours*m_LinesPerHour-m_ScrollPos.y)*GetLineHeight();
			rc.top=y-m_TimeBarWidth;
			rc.bottom=y;
			rc.left=0;
			rc.right=m_TimeBarWidth;
			m_Tooltip.AddTool(ToolCount++,rc,TEXT("一日後へ"));
			rc.left=rcClient.right-m_TimeBarWidth;
			rc.right=rcClient.right;
			m_Tooltip.AddTool(ToolCount,rc,TEXT("一日後へ"));
		}
	} else if (m_ListMode==LIST_WEEK) {
		m_Tooltip.DeleteAllTools();
		GetClientRect(&rc);
		rc.left+=m_TimeBarWidth;
		rc.right-=m_TimeBarWidth;
		rc.bottom=m_HeaderHeight;
		m_Tooltip.AddTool(0,rc,TEXT("チャンネル一覧表示へ"));
	}
}


void CProgramGuide::OnFontChanged()
{
	for (size_t i=0;i<m_EventLayoutList.Length();i++) {
		ProgramGuide::CEventLayout *pLayout=m_EventLayoutList[i];
		for (size_t i=0;i<pLayout->NumItems();i++) {
			ProgramGuide::CEventItem *pItem=pLayout->GetItem(i);
			pItem->ResetTitleLines();
		}
	}

	CalcFontMetrics();
	SetScrollBar();
	SetTooltip();
	Invalidate();
}


bool CProgramGuide::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 m_pszWindowClass,NULL,m_hinst);
}


bool CProgramGuide::SetChannelProviderManager(CProgramGuideChannelProviderManager *pManager)
{
	m_pChannelProviderManager=pManager;
	Clear();
	return true;
}


bool CProgramGuide::EnumChannelProvider(int Index,LPTSTR pszName,int MaxName) const
{
	if (m_pChannelProviderManager==NULL
			|| Index<0 || pszName==NULL || MaxName<1)
		return false;

	const CProgramGuideChannelProvider *pChannelProvider=
		m_pChannelProviderManager->GetChannelProvider(Index);
	if (pChannelProvider==NULL)
		return false;

	pChannelProvider->GetName(pszName,MaxName);

	return true;
}


bool CProgramGuide::SetChannelProvider(int Provider,int Group)
{
	StoreTimePos();
	if (!SetCurrentChannelProvider(Provider,Group))
		return false;
	UpdateServiceList();
	return true;
}


bool CProgramGuide::SetChannelProvider(int Provider,LPCTSTR pszGroupID)
{
	StoreTimePos();
	if (!SetCurrentChannelProvider(Provider,pszGroupID))
		return false;
	UpdateServiceList();
	return true;
}


bool CProgramGuide::SetCurrentChannelProvider(int Provider,int Group)
{
	if (m_pChannelProviderManager==NULL
			|| Provider<-1 || (size_t)Provider>=m_pChannelProviderManager->GetChannelProviderCount())
		return false;

	if (Provider>=0) {
		m_pChannelProvider=m_pChannelProviderManager->GetChannelProvider(Provider);
		if (m_pChannelProvider==NULL)
			return false;
		m_pChannelProvider->Update();
		m_CurrentChannelProvider=Provider;
		m_CurrentChannelGroup=-1;
		SetCurrentChannelGroup(Group);
	} else {
		Clear();
	}

	return true;
}


bool CProgramGuide::SetCurrentChannelProvider(int Provider,LPCTSTR pszGroupID)
{
	if (m_pChannelProviderManager==NULL
			|| Provider<-1 || (size_t)Provider>=m_pChannelProviderManager->GetChannelProviderCount())
		return false;

	if (Provider>=0) {
		m_pChannelProvider=m_pChannelProviderManager->GetChannelProvider(Provider);
		if (m_pChannelProvider==NULL)
			return false;
		m_pChannelProvider->Update();
		m_CurrentChannelProvider=Provider;
		m_CurrentChannelGroup=-1;
		SetCurrentChannelGroup(m_pChannelProvider->ParseGroupID(pszGroupID));
	} else {
		Clear();
	}

	return true;
}


int CProgramGuide::GetChannelGroupCount() const
{
	if (m_pChannelProvider==NULL)
		return 0;
	return (int)m_pChannelProvider->GetGroupCount();
}


bool CProgramGuide::GetChannelGroupName(int Group,LPTSTR pszName,int MaxName) const
{
	if (m_pChannelProvider==NULL)
		return false;
	return m_pChannelProvider->GetGroupName(Group,pszName,MaxName);
}


int CProgramGuide::ParseChannelGroupID(LPCTSTR pszGroupID) const
{
	if (m_pChannelProvider==NULL)
		return false;
	return m_pChannelProvider->ParseGroupID(pszGroupID);
}


bool CProgramGuide::SetCurrentChannelGroup(int Group)
{
	if (m_pChannelProvider==NULL
			|| Group<-1 || (size_t)Group>=m_pChannelProvider->GetGroupCount())
		return false;

	if (Group!=m_CurrentChannelGroup) {
		m_ScrollPos.x=0;
		m_ScrollPos.y=0;
		m_OldScrollPos=m_ScrollPos;
	}
	m_CurrentChannelGroup=Group;

	return true;
}


bool CProgramGuide::GetChannelList(CChannelList *pList,bool fVisibleOnly) const
{
	if (pList==NULL)
		return false;

	pList->Clear();

	if (fVisibleOnly) {
		for (size_t i=0;i<m_ServiceList.NumServices();i++)
			pList->AddChannel(m_ServiceList.GetItem(i)->GetChannelInfo());
	} else {
		if (m_pChannelProvider==NULL)
			return false;
		for (size_t i=0;i<m_pChannelProvider->GetChannelCount(m_CurrentChannelGroup);i++) {
			const CChannelInfo *pChannelInfo=m_pChannelProvider->GetChannelInfo(m_CurrentChannelGroup,i);

			if (pChannelInfo!=NULL
					&& pChannelInfo->IsEnabled())
				pList->AddChannel(*pChannelInfo);
		}
	}

	return true;
}


void CProgramGuide::SetCurrentService(WORD NetworkID,WORD TSID,WORD ServiceID)
{
	bool fRedrawEvent=false;
	int ListIndex,EventIndex;
	if (m_CurrentEventID!=0 && m_hwnd!=NULL) {
		fRedrawEvent=GetEventIndexByIDs(
			m_CurrentChannel.NetworkID,
			m_CurrentChannel.TransportStreamID,
			m_CurrentChannel.ServiceID,
			m_CurrentEventID,
			&ListIndex,&EventIndex);
	}

	m_CurrentChannel.NetworkID=NetworkID;
	m_CurrentChannel.TransportStreamID=TSID;
	m_CurrentChannel.ServiceID=ServiceID;
	m_CurrentEventID=0;

	if (m_hwnd!=NULL) {
		RECT rc;

		GetClientRect(&rc);
		rc.bottom=m_HeaderHeight;
		::InvalidateRect(m_hwnd,&rc,TRUE);

		if (fRedrawEvent)
			RedrawEvent(ListIndex,EventIndex);
	}
}


void CProgramGuide::SetCurrentEvent(WORD EventID)
{
	if (m_CurrentEventID!=EventID) {
		const WORD OldEventID=m_CurrentEventID;

		m_CurrentEventID=EventID;

		if (m_hwnd!=NULL) {
			if (OldEventID!=0) {
				RedrawEventByIDs(m_CurrentChannel.NetworkID,
								 m_CurrentChannel.TransportStreamID,
								 m_CurrentChannel.ServiceID,
								 OldEventID);
			}
			if (m_CurrentEventID!=0) {
				RedrawEventByIDs(m_CurrentChannel.NetworkID,
								 m_CurrentChannel.TransportStreamID,
								 m_CurrentChannel.ServiceID,
								 m_CurrentEventID);
			}
		}
	}
}


bool CProgramGuide::SetExcludeNoEventServices(bool fExclude)
{
	m_fExcludeNoEventServices=fExclude;
	return true;
}


bool CProgramGuide::SetExcludeServiceList(const ServiceInfoList &List)
{
	m_ExcludeServiceList=List;
	return true;
}


bool CProgramGuide::GetExcludeServiceList(ServiceInfoList *pList) const
{
	if (pList==NULL)
		return false;
	*pList=m_ExcludeServiceList;
	return true;
}


bool CProgramGuide::IsExcludeService(WORD NetworkID,WORD TransportStreamID,WORD ServiceID) const
{
	for (auto i=m_ExcludeServiceList.begin();i!=m_ExcludeServiceList.end();i++) {
		if ((NetworkID==0 || i->NetworkID==NetworkID)
				&& (TransportStreamID==0 || i->TransportStreamID==TransportStreamID)
				&& i->ServiceID==ServiceID)
			return true;
	}
	return false;
}


bool CProgramGuide::SetExcludeService(WORD NetworkID,WORD TransportStreamID,WORD ServiceID,bool fExclude)
{
	for (auto i=m_ExcludeServiceList.begin();i!=m_ExcludeServiceList.end();i++) {
		if (i->NetworkID==NetworkID
				&& i->TransportStreamID==TransportStreamID
				&& i->ServiceID==ServiceID) {
			if (!fExclude)
				m_ExcludeServiceList.erase(i);
			return true;
		}
	}
	if (fExclude)
		m_ExcludeServiceList.push_back(ServiceInfo(NetworkID,TransportStreamID,ServiceID));
	return true;
}


bool CProgramGuide::SetServiceListMode()
{
	if (m_ListMode!=LIST_SERVICES) {
		StoreTimePos();

		m_ListMode=LIST_SERVICES;
		m_WeekListService=-1;

		HCURSOR hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));

		m_ScrollPos=m_OldScrollPos;
		CalcLayout();
		SetScrollBar();
		SetCaption();
		Invalidate();

		RestoreTimePos();

		if (m_pFrame!=NULL)
			m_pFrame->OnListModeChanged();

		::SetCursor(hcurOld);
	}

	return true;
}


bool CProgramGuide::SetWeekListMode(int Service)
{
	ProgramGuide::CServiceInfo *pServiceInfo=m_ServiceList.GetItem(Service);

	if (pServiceInfo==NULL)
		return false;

	if (m_ListMode!=LIST_WEEK || m_WeekListService!=Service) {
		StoreTimePos();

		m_ListMode=LIST_WEEK;
		m_WeekListService=Service;

		HCURSOR hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));

		UpdateService(pServiceInfo,true);
		m_OldScrollPos=m_ScrollPos;
		m_ScrollPos.x=0;
		m_ScrollPos.y=0;
		CalcLayout();
		SetScrollBar();
		SetCaption();
		Invalidate();

		RestoreTimePos();

		if (m_pFrame!=NULL)
			m_pFrame->OnListModeChanged();

		::SetCursor(hcurOld);
	}

	return true;
}


bool CProgramGuide::SetBeginHour(int Hour)
{
	if (Hour<-1 || Hour>=24)
		return false;
	m_BeginHour=Hour;
	return true;
}


bool CProgramGuide::SetTimeRange(const SYSTEMTIME *pFirstTime,const SYSTEMTIME *pLastTime)
{
	SYSTEMTIME stFirst,stLast;

	stFirst=*pFirstTime;
	SystemTimeTruncateHour(&stFirst);
	stLast=*pLastTime;
	SystemTimeTruncateHour(&stLast);

	int Hours=(int)(DiffSystemTime(&stFirst,&stLast)/TimeConsts::SYSTEMTIME_HOUR);
	if (Hours<=0)
		return false;

	m_stFirstTime=stFirst;
	m_stLastTime=stLast;
	m_Hours=Hours;

	if (m_pFrame!=NULL)
		m_pFrame->OnTimeRangeChanged();

	return true;
}


bool CProgramGuide::GetTimeRange(SYSTEMTIME *pFirstTime,SYSTEMTIME *pLastTime) const
{
	if (pFirstTime!=NULL)
		*pFirstTime=m_stFirstTime;
	if (pLastTime!=NULL)
		*pLastTime=m_stLastTime;
	return true;
}


bool CProgramGuide::GetCurrentTimeRange(SYSTEMTIME *pFirstTime,SYSTEMTIME *pLastTime) const
{
	if (m_ListMode==LIST_WEEK) {
		GetDayTimeRange(DAY_TOMORROW,pFirstTime,pLastTime);
		if (pFirstTime!=NULL)
			OffsetSystemTime(pFirstTime,-TimeConsts::SYSTEMTIME_DAY);
		if (pLastTime!=NULL)
			OffsetSystemTime(pLastTime,-TimeConsts::SYSTEMTIME_DAY);
		return true;
	}

	return GetDayTimeRange(m_Day,pFirstTime,pLastTime);
}


bool CProgramGuide::GetDayTimeRange(int Day,SYSTEMTIME *pFirstTime,SYSTEMTIME *pLastTime) const
{
	SYSTEMTIME stFirst=m_stFirstTime,stLast=m_stLastTime;

	if (Day!=DAY_TODAY) {
		LONGLONG Offset;

		if (m_BeginHour<0) {
			Offset=Day*24;
		} else {
			int Begin=m_BeginHour*60;
			CEpgOptions::EpgTimeMode TimeMode=GetAppClass().EpgOptions.GetEpgTimeMode();
			bool fUTC=false;

			switch (TimeMode) {
			case CEpgOptions::EPGTIME_LOCAL:
				{
					TIME_ZONE_INFORMATION tzi;
					DWORD Result=::GetTimeZoneInformation(&tzi);
					switch (Result) {
					case TIME_ZONE_ID_UNKNOWN:
						Begin+=tzi.Bias;
						break;
					case TIME_ZONE_ID_STANDARD:
						Begin+=tzi.Bias+tzi.StandardBias;
						break;
					case TIME_ZONE_ID_DAYLIGHT:
						Begin+=tzi.Bias+tzi.DaylightBias;
						break;
					}
					if (Begin<0)
						Begin+=24*60;
					fUTC=true;
				}
				break;

			case CEpgOptions::EPGTIME_JST:
				{
					TIME_ZONE_INFORMATION tzi;

					if (GetJSTTimeZoneInformation(&tzi)) {
						SYSTEMTIME stUTC,stJST;

						::GetSystemTime(&stUTC);
						if (::SystemTimeToTzSpecificLocalTime(&tzi,&stUTC,&stJST)) {
							Begin+=(int)(DiffSystemTime(&stJST,&stUTC)/TimeConsts::SYSTEMTIME_MINUTE);
							fUTC=true;
						}
					}
				}
				break;

			case CEpgOptions::EPGTIME_UTC:
				fUTC=true;
				break;
			}

			if (fUTC)
				Begin+=9*60;
			Begin=Begin/60%24;

			Offset=Day*24-stFirst.wHour;
			if (stFirst.wHour>=Begin)
				Offset+=Begin;
			else
				Offset-=24-Begin;
		}
		Offset*=TimeConsts::SYSTEMTIME_HOUR;
		OffsetSystemTime(&stFirst,Offset);
		OffsetSystemTime(&stLast,Offset);
	}
	if (pFirstTime!=NULL)
		*pFirstTime=stFirst;
	if (pLastTime!=NULL)
		*pLastTime=stLast;
	return true;
}


bool CProgramGuide::GetCurrentDateInfo(DateInfo *pInfo) const
{
	if (m_ListMode==LIST_WEEK) {
		if (!GetCurrentTimeRange(&pInfo->BeginningTime,&pInfo->EndTime))
			return false;
		pInfo->pszRelativeDayText=NULL;
	} else {
		if (!GetDateInfo(m_Day,pInfo))
			return false;
	}
	return true;
}


bool CProgramGuide::GetDateInfo(int Day,DateInfo *pInfo) const
{
	if (!GetDayTimeRange(Day,&pInfo->BeginningTime,&pInfo->EndTime))
		return false;
	if (m_ListMode==LIST_SERVICES) {
		if (Day==DAY_TODAY) {
			pInfo->pszRelativeDayText=GetRelativeDayText(0);
		} else {
			SYSTEMTIME st1,st2;
			GetDayTimeRange(DAY_TODAY,&st1,NULL);
			SystemTimeTruncateDay(&st1);
			st2=pInfo->BeginningTime;
			SystemTimeTruncateDay(&st2);
			pInfo->pszRelativeDayText=
				GetRelativeDayText((int)(DiffSystemTime(&st1,&st2)/TimeConsts::SYSTEMTIME_DAY));
		}
	} else {
		pInfo->pszRelativeDayText=NULL;
	}
	return true;
}


bool CProgramGuide::ScrollToTime(const SYSTEMTIME &Time,bool fHour)
{
	SYSTEMTIME stFirst,stLast;
	if (!GetCurrentTimeRange(&stFirst,&stLast))
		return false;

	SYSTEMTIME st=Time;
	if (m_ListMode==LIST_SERVICES) {
		if (CompareSystemTime(&st,&stFirst)<0
				|| CompareSystemTime(&st,&stLast)>=0)
			return false;
	} else if (m_ListMode==LIST_WEEK) {
		st.wYear=stFirst.wYear;
		st.wMonth=stFirst.wMonth;
		st.wDay=stFirst.wDay;
		if (CompareSystemTime(&st,&stFirst)<0)
			OffsetSystemTime(&st,TimeConsts::SYSTEMTIME_DAY);
	}

	LONGLONG Diff=DiffSystemTime(&stFirst,&st);
	POINT Pos;
	Pos.x=m_ScrollPos.x;
	if (fHour)
		Pos.y=(int)(Diff/TimeConsts::SYSTEMTIME_HOUR)*m_LinesPerHour;
	else
		Pos.y=(int)(Diff/TimeConsts::SYSTEMTIME_MINUTE*m_LinesPerHour/60);
	SetScrollPos(Pos);

	return true;
}


bool CProgramGuide::ScrollToCurrentTime()
{
	return ScrollToTime(m_stCurTime,true);
}


bool CProgramGuide::SetViewDay(int Day)
{
	if (Day<DAY_FIRST || Day>DAY_LAST)
		return false;

	if (m_Day!=Day || m_ListMode!=LIST_SERVICES) {
		StoreTimePos();

		m_Day=Day;

		if (m_pProgramList!=NULL) {
			HCURSOR hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));

			if (m_ListMode!=LIST_SERVICES) {
				m_ListMode=LIST_SERVICES;
				m_WeekListService=-1;
				m_ScrollPos.x=m_OldScrollPos.x;
				if (m_pFrame!=NULL)
					m_pFrame->OnListModeChanged();
			}
			m_ScrollPos.y=0;
			CalcLayout();
			SetScrollBar();
			SetCaption();
			GetCurrentEpgTime(&m_stCurTime);
			Invalidate();

			RestoreTimePos();

			::SetCursor(hcurOld);

			if (m_pFrame!=NULL)
				m_pFrame->OnDateChanged();
		}
	}

	return true;
}


// 指定された番組まで移動する
bool CProgramGuide::JumpEvent(WORD NetworkID,WORD TSID,WORD ServiceID,WORD EventID)
{
	const int ServiceIndex=m_ServiceList.FindItemByIDs(TSID,ServiceID);
	if (ServiceIndex<0)
		return false;
	const ProgramGuide::CServiceInfo *pServiceInfo=m_ServiceList.GetItem(ServiceIndex);
	const CEventInfoData *pEventInfo=pServiceInfo->GetEventByEventID(EventID);
	if (pEventInfo==NULL)
		return false;

	SYSTEMTIME stStart,stEnd,stFirst,stLast;

	pEventInfo->GetStartTime(&stStart);
	pEventInfo->GetEndTime(&stEnd);

	bool fChangeDate=true;
	if (m_ListMode==LIST_SERVICES) {
		GetCurrentTimeRange(&stFirst,&stLast);
		if (CompareSystemTime(&stFirst,&stStart)<=0
				&& CompareSystemTime(&stLast,&stEnd)>=0)
			fChangeDate=false;
	}
	if (fChangeDate) {
		int Day;
		for (Day=DAY_FIRST;Day<=DAY_LAST;Day++) {
			GetDayTimeRange(Day,&stFirst,&stLast);
			if (CompareSystemTime(&stEnd,&stLast)<=0)
				break;
		}
		if (CompareSystemTime(&stEnd,&stFirst)<=0)
			return false;
		if (!SetViewDay(Day))
			return false;
	}

	SIZE Size,Page;
	GetProgramGuideSize(&Size);
	GetPageSize(&Page);
	const int ItemWidth=m_ItemWidth+m_Style.ColumnMargin*2;
	POINT Pos;
	Pos.x=ItemWidth*ServiceIndex-(Page.cx-ItemWidth)/2;
	if (Pos.x<0)
		Pos.x=0;
	else if (Pos.x>max(Size.cx-Page.cx,0))
		Pos.x=max(Size.cx-Page.cx,0);
	Pos.y=(int)(DiffSystemTime(&stFirst,&pEventInfo->m_StartTime)/TimeConsts::SYSTEMTIME_MINUTE)*m_LinesPerHour/60;
	const int YOffset=(Page.cy-(int)(pEventInfo->m_Duration*m_LinesPerHour/(60*60)))/2;
	if (YOffset>0)
		Pos.y-=YOffset;
	if (Pos.y<0)
		Pos.y=0;
	else if (Pos.y>max(Size.cy-Page.cy,0))
		Pos.y=max(Size.cy-Page.cy,0);
	SetScrollPos(Pos);

	SelectEventByIDs(NetworkID,TSID,ServiceID,EventID);

	return true;
}


bool CProgramGuide::JumpEvent(const CEventInfoData &EventInfo)
{
	return JumpEvent(EventInfo.m_NetworkID,EventInfo.m_TransportStreamID,EventInfo.m_ServiceID,EventInfo.m_EventID);
}


bool CProgramGuide::ScrollToCurrentService()
{
	if (m_ListMode!=LIST_SERVICES)
		return false;
	if (m_CurrentChannel.ServiceID==0)
		return false;

	const int ServiceIndex=m_ServiceList.FindItemByIDs(
		m_CurrentChannel.TransportStreamID,m_CurrentChannel.ServiceID);
	if (ServiceIndex<0)
		return false;

	SIZE Size,Page;
	GetProgramGuideSize(&Size);
	GetPageSize(&Page);
	const int ItemWidth=m_ItemWidth+m_Style.ColumnMargin*2;
	POINT Pos;
	Pos.x=ItemWidth*ServiceIndex-(Page.cx-ItemWidth)/2;
	if (Pos.x<0)
		Pos.x=0;
	else if (Pos.x>max(Size.cx-Page.cx,0))
		Pos.x=max(Size.cx-Page.cx,0);
	Pos.y=m_ScrollPos.y;
	SetScrollPos(Pos);

	return true;
}


bool CProgramGuide::SetUIOptions(int LinesPerHour,int ItemWidth)
{
	if (LinesPerHour<MIN_LINES_PER_HOUR || LinesPerHour>MAX_LINES_PER_HOUR
			|| ItemWidth<MIN_ITEM_WIDTH || ItemWidth>MAX_ITEM_WIDTH)
		return false;
	if (m_LinesPerHour!=LinesPerHour
			|| m_ItemWidth!=ItemWidth) {
		m_LinesPerHour=LinesPerHour;
		m_ItemWidth=ItemWidth;
		if (m_hwnd!=NULL) {
			m_ScrollPos.x=0;
			m_ScrollPos.y=0;
			m_OldScrollPos=m_ScrollPos;
			CalcLayout();
			SetScrollBar();
			Invalidate();
		}
	}
	return true;
}


bool CProgramGuide::SetTextDrawEngine(TVTest::CTextDrawClient::TextDrawEngine Engine)
{
	if (m_hwnd!=NULL) {
		if (!m_TextDrawClient.Initialize(Engine,m_hwnd))
			return false;

		OnFontChanged();
	}

	m_TextDrawEngine=Engine;

	return true;
}


bool CProgramGuide::SetDirectWriteRenderingParams(
	const TVTest::CDirectWriteRenderer::RenderingParams &Params)
{
	if (!m_TextDrawClient.SetDirectWriteRenderingParams(Params))
		return false;

	if (m_hwnd!=NULL && m_TextDrawEngine==TVTest::CTextDrawClient::ENGINE_DIRECTWRITE)
		Invalidate();

	return true;
}


bool CProgramGuide::SetFont(const LOGFONT *pFont)
{
	LOGFONT lf;

	if (!m_Font.Create(pFont))
		return false;
	lf=*pFont;
	lf.lfWeight=FW_BOLD;
	m_TitleFont.Create(&lf);
	lf.lfWeight=FW_NORMAL;
	lf.lfEscapement=lf.lfOrientation=2700;
	m_TimeFont.Create(&lf);
	if (m_hwnd!=NULL)
		OnFontChanged();

	return true;
}


bool CProgramGuide::GetFont(LOGFONT *pFont) const
{
	return m_Font.GetLogFont(pFont);
}


bool CProgramGuide::SetEventInfoFont(const LOGFONT *pFont)
{
	return m_EventInfoPopup.SetFont(pFont);
}


bool CProgramGuide::SetShowToolTip(bool fShow)
{
	if (m_fShowToolTip!=fShow) {
		m_fShowToolTip=fShow;
		m_EventInfoPopupManager.SetEnable(fShow);
	}
	return true;
}


void CProgramGuide::SetEventHandler(CEventHandler *pEventHandler)
{
	if (m_pEventHandler)
		m_pEventHandler->m_pProgramGuide=NULL;
	if (pEventHandler)
		pEventHandler->m_pProgramGuide=this;
	m_pEventHandler=pEventHandler;
}


void CProgramGuide::SetFrame(CFrame *pFrame)
{
	if (m_pFrame)
		m_pFrame->m_pProgramGuide=NULL;
	if (pFrame)
		pFrame->m_pProgramGuide=this;
	m_pFrame=pFrame;
}


void CProgramGuide::SetProgramCustomizer(CProgramCustomizer *pProgramCustomizer)
{
	if (m_pProgramCustomizer)
		m_pProgramCustomizer->m_pProgramGuide=NULL;
	if (pProgramCustomizer) {
		pProgramCustomizer->m_pProgramGuide=this;
		if (m_hwnd!=NULL)
			pProgramCustomizer->Initialize();
	}
	m_pProgramCustomizer=pProgramCustomizer;
}


bool CProgramGuide::SetDragScroll(bool fDragScroll)
{
	if (m_fDragScroll!=fDragScroll) {
		m_fDragScroll=fDragScroll;
		if (m_hwnd!=NULL) {
			POINT pt;

			::GetCursorPos(&pt);
			SendMessage(WM_SETCURSOR,(WPARAM)m_hwnd,
						MAKELPARAM(SendMessage(WM_NCHITTEST,0,MAKELPARAM(pt.x,pt.y)),WM_MOUSEMOVE));
		}
	}
	return true;
}


bool CProgramGuide::SetFilter(unsigned int Filter)
{
	if ((Filter&(FILTER_ORIGINAL | FILTER_RERUN))==(FILTER_ORIGINAL | FILTER_RERUN))
		Filter&=~(FILTER_ORIGINAL | FILTER_RERUN);
	if (m_Filter!=Filter) {
		m_Filter=Filter;
		if (m_hwnd!=NULL) {
			Invalidate();
		}
	}
	return true;
}


void CProgramGuide::SetVisibleEventIcons(UINT VisibleIcons)
{
	if (m_VisibleEventIcons!=VisibleIcons) {
		m_VisibleEventIcons=VisibleIcons;
		if (m_hwnd!=NULL)
			Invalidate();
	}
}


void CProgramGuide::SetKeepTimePos(bool fKeep)
{
	m_fKeepTimePos=fKeep;
}


void CProgramGuide::SetShowFeaturedMark(bool fShow)
{
	if (m_fShowFeaturedMark!=fShow) {
		m_fShowFeaturedMark=fShow;
		if (m_hwnd!=NULL) {
			if (m_fShowFeaturedMark)
				m_FeaturedEventsMatcher.BeginMatching(GetAppClass().FeaturedEvents.GetSettings());
			Invalidate();
		}
	}
}


const TVTest::Style::Margins &CProgramGuide::GetToolbarItemPadding() const
{
	return m_Style.ToolbarItemPadding;
}


bool CProgramGuide::ShowProgramSearch(bool fShow)
{
	if (fShow) {
		if (!m_ProgramSearch.IsCreated()) {
			RECT rc;

			m_ProgramSearch.SetEventHandler(&m_ProgramSearchEventHandler);
			m_ProgramSearch.GetPosition(&rc);
			if (rc.left==rc.right || rc.top==rc.bottom) {
				POINT pt={0,0};
				::ClientToScreen(m_hwnd,&pt);
				m_ProgramSearch.SetPosition(pt.x,pt.y,0,0);
			}
			m_ProgramSearch.Create(m_hwnd);
			GetAppClass().UICore.RegisterModelessDialog(&m_ProgramSearch);
		}
		if (!m_ProgramSearch.IsVisible())
			m_ProgramSearch.SetVisible(true);
	} else {
		if (m_ProgramSearch.IsCreated()) {
			GetAppClass().UICore.UnregisterModelessDialog(&m_ProgramSearch);
			m_ProgramSearch.Destroy();
		}
	}

	return true;
}


void CProgramGuide::SetMessage(LPCTSTR pszMessage,bool fUpdate)
{
	bool fErase=false;

	if (!IsStringEmpty(pszMessage)) {
		fErase=!m_Message.empty();
		m_Message=pszMessage;
	} else {
		if (m_Message.empty())
			return;
		m_Message.clear();
	}

	if (fUpdate && m_hwnd!=NULL) {
		if (m_Message.empty() || fErase) {
			Redraw();
		} else {
			HDC hdc=::GetDC(m_hwnd);

			if (hdc!=NULL) {
				RECT rc;
				GetClientRect(&rc);
				DrawMessage(hdc,rc);
				::ReleaseDC(m_hwnd,hdc);
			}
		}
	}
}


void CProgramGuide::OnEpgCaptureBegin()
{
}


void CProgramGuide::OnEpgCaptureEnd()
{
	if (m_fEpgUpdating) {
		m_fEpgUpdating=false;
		SetCaption();
	}
}


void CProgramGuide::SetEpgCaptureProgress(int Pos,int End,DWORD RemainingTime)
{
	m_EpgUpdateProgress.Pos=Pos;
	m_EpgUpdateProgress.End=End;
	m_EpgUpdateProgress.RemainingTime=RemainingTime;
	if (m_fEpgUpdating)
		SetCaption();
}


bool CProgramGuide::OnCloseFrame()
{
	if (m_pEventHandler!=NULL && !m_pEventHandler->OnClose())
		return false;

	if (m_fEpgUpdating)
		GetAppClass().EpgCaptureManager.EndCapture();

	ShowProgramSearch(false);

	return true;
}


void CProgramGuide::OnShowFrame(bool fShow)
{
	if (m_ProgramSearch.IsCreated())
		m_ProgramSearch.SetVisible(fShow);
}


LPCTSTR CProgramGuide::GetRelativeDayText(int Day)
{
	static const LPCTSTR DayText[] = {
		TEXT("今日"),
		TEXT("明日"),
		TEXT("2日後"),
		TEXT("3日後"),
		TEXT("4日後"),
		TEXT("5日後"),
		TEXT("6日後"),
		TEXT("7日後"),
	};

	if (Day<0 || Day>=lengthof(DayText))
		return NULL;
	return DayText[Day];
}


ProgramGuide::CEventItem *CProgramGuide::GetEventItem(int ListIndex,int EventIndex)
{
	if (ListIndex<0 || (size_t)ListIndex>=m_EventLayoutList.Length())
		return NULL;
	return m_EventLayoutList[ListIndex]->GetItem(EventIndex);
}


const ProgramGuide::CEventItem *CProgramGuide::GetEventItem(int ListIndex,int EventIndex) const
{
	if (ListIndex<0 || (size_t)ListIndex>=m_EventLayoutList.Length())
		return NULL;
	return m_EventLayoutList[ListIndex]->GetItem(EventIndex);
}


bool CProgramGuide::GetEventRect(int ListIndex,int EventIndex,RECT *pRect) const
{
	const ProgramGuide::CEventItem *pItem=GetEventItem(ListIndex,EventIndex);
	if (pItem==NULL)
		return false;

	int LineHeight=GetLineHeight();
	RECT rc;
	GetProgramGuideRect(&rc);

	pRect->top=pItem->GetItemPos()*LineHeight+(rc.top-m_ScrollPos.y*LineHeight);
	pRect->bottom=pRect->top+pItem->GetItemLines()*LineHeight;
	pRect->left=ListIndex*(m_ItemWidth+m_Style.ColumnMargin*2)+m_Style.ColumnMargin+(rc.left-m_ScrollPos.x);
	pRect->right=pRect->left+m_ItemWidth;

	return true;
}


bool CProgramGuide::RedrawEvent(int ListIndex,int EventIndex)
{
	RECT rc;

	if (!GetEventRect(ListIndex,EventIndex,&rc))
		return false;
	rc.left-=m_Style.SelectedBorder.Left;
	rc.right+=m_Style.SelectedBorder.Right;
	Invalidate(&rc);
	return true;
}


bool CProgramGuide::RedrawEventByIDs(WORD NetworkID,WORD TSID,WORD ServiceID,WORD EventID)
{
	int ListIndex,EventIndex;

	if (!GetEventIndexByIDs(NetworkID,TSID,ServiceID,EventID,&ListIndex,&EventIndex))
		return false;

	return RedrawEvent(ListIndex,EventIndex);
}


bool CProgramGuide::EventHitTest(int x,int y,int *pListIndex,int *pEventIndex,RECT *pItemRect) const
{
	POINT pt;
	RECT rc;

	pt.x=x;
	pt.y=y;
	GetProgramGuideRect(&rc);
	if (::PtInRect(&rc,pt)) {
		const int XPos=x-rc.left+m_ScrollPos.x;
		const int ServiceWidth=m_ItemWidth+m_Style.ColumnMargin*2;

		if (XPos%ServiceWidth<m_Style.ColumnMargin
				|| XPos%ServiceWidth>=m_Style.ColumnMargin+m_ItemWidth)
			return false;
		int List=XPos/ServiceWidth;
		if (List<(int)m_EventLayoutList.Length()) {
			const ProgramGuide::CEventLayout *pLayout=m_EventLayoutList[List];
			int LineHeight=GetLineHeight();
			int YOrigin=rc.top-m_ScrollPos.y*LineHeight;

			y-=YOrigin;
			for (size_t i=0;i<pLayout->NumItems();i++) {
				const ProgramGuide::CEventItem *pItem=pLayout->GetItem(i);

				if (!pItem->IsNullItem() && pItem->GetItemLines()>0) {
					rc.top=pItem->GetItemPos()*LineHeight;
					rc.bottom=rc.top+pItem->GetItemLines()*LineHeight;
					if (y>=rc.top && y<rc.bottom) {
						if (pListIndex!=NULL)
							*pListIndex=List;
						if (pEventIndex!=NULL)
							*pEventIndex=(int)i;
						if (pItemRect!=NULL) {
							pItemRect->top=rc.top+YOrigin;
							pItemRect->bottom=pItemRect->top+(rc.bottom-rc.top);
							pItemRect->left=List*ServiceWidth-m_ScrollPos.x+m_Style.ColumnMargin;
							pItemRect->right=pItemRect->left+m_ItemWidth;
						}
						return true;
					}
				}
			}
		}
	}
	return false;
}


bool CProgramGuide::GetEventIndexByIDs(
	WORD NetworkID,WORD TSID,WORD ServiceID,WORD EventID,
	int *pListIndex,int *pEventIndex) const
{
	int ListIndex;

	if (m_ListMode==LIST_SERVICES) {
		ListIndex=m_ServiceList.FindItemByIDs(TSID,ServiceID);
		if (ListIndex<0)
			return false;
	} else {
		return false;
	}

	const ProgramGuide::CEventLayout *pEventLayout=m_EventLayoutList[ListIndex];
	if (pEventLayout==NULL)
		return false;
	const size_t NumItems=pEventLayout->NumItems();
	for (size_t i=0;i<NumItems;i++) {
		const ProgramGuide::CEventItem *pItem=pEventLayout->GetItem(i);
		if (pItem->GetEventInfo()!=NULL
				&& pItem->GetEventInfo()->m_EventID==EventID) {
			if (pListIndex!=NULL)
				*pListIndex=ListIndex;
			if (pEventIndex!=NULL)
				*pEventIndex=static_cast<int>(i);
			return true;
		}
	}

	return false;
}


bool CProgramGuide::SelectEvent(int ListIndex,int EventIndex)
{
	bool fSelected;
	if (ListIndex>=0 && (size_t)ListIndex<m_EventLayoutList.Length()
			&& EventIndex>=0 && (size_t)EventIndex<m_EventLayoutList[ListIndex]->NumItems()) {
		fSelected=true;
	} else {
		fSelected=false;
	}

	ProgramGuide::CEventItem *pItem;

	if (m_CurEventItem.fSelected) {
		if (fSelected && m_CurEventItem.ListIndex==ListIndex && m_CurEventItem.EventIndex==EventIndex)
			return true;
		pItem=GetEventItem(m_CurEventItem.ListIndex,m_CurEventItem.EventIndex);
		if (pItem!=NULL) {
			pItem->SetSelected(false);
			RedrawEvent(m_CurEventItem.ListIndex,m_CurEventItem.EventIndex);
		}
	}

	m_CurEventItem.fSelected=fSelected;
	if (fSelected) {
		pItem=GetEventItem(ListIndex,EventIndex);
		pItem->SetSelected(true);
		m_CurEventItem.ListIndex=ListIndex;
		m_CurEventItem.EventIndex=EventIndex;
		RedrawEvent(ListIndex,EventIndex);
	}

	return true;
}


bool CProgramGuide::SelectEventByPosition(int x,int y)
{
	int ListIndex,EventIndex;
	bool fSel=EventHitTest(x,y,&ListIndex,&EventIndex);

	if (fSel)
		SelectEvent(ListIndex,EventIndex);
	else
		SelectEvent(-1,-1);
	return fSel;
}


bool CProgramGuide::SelectEventByIDs(WORD NetworkID,WORD TSID,WORD ServiceID,WORD EventID)
{
	int ListIndex,EventIndex;

	if (!GetEventIndexByIDs(NetworkID,TSID,ServiceID,EventID,&ListIndex,&EventIndex))
		return false;

	return SelectEvent(ListIndex,EventIndex);
}


LRESULT CProgramGuide::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			InitializeUI();

			m_TextLeftMargin=
				m_Style.EventIconSize.Width+
				m_Style.EventIconMargin.Left+m_Style.EventIconMargin.Right;

			if (m_hDragCursor1==NULL)
				m_hDragCursor1=::LoadCursor(m_hinst,MAKEINTRESOURCE(IDC_GRAB1));
			if (m_hDragCursor2==NULL)
				m_hDragCursor2=::LoadCursor(m_hinst,MAKEINTRESOURCE(IDC_GRAB2));
			m_Chevron.Load(m_hinst,IDB_CHEVRON,CHEVRON_WIDTH,CHEVRON_HEIGHT);
			m_EpgIcons.Load();
			m_EventInfoPopupManager.Initialize(hwnd,&m_EventInfoPopupHandler);
			m_Tooltip.Create(hwnd);
			if (m_pProgramCustomizer!=NULL)
				m_pProgramCustomizer->Initialize();

			m_fBarShadow=CBufferedPaint::IsSupported();

			if (!m_TextDrawClient.Initialize(m_TextDrawEngine,hwnd)) {
				if (m_TextDrawEngine!=TVTest::CTextDrawClient::ENGINE_GDI) {
					m_TextDrawEngine=TVTest::CTextDrawClient::ENGINE_GDI;
					m_TextDrawClient.Initialize(m_TextDrawEngine,hwnd);
				}
			}
			m_TextDrawClient.SetMaxFontCache(2);
			CalcFontMetrics();

			CFeaturedEvents &FeaturedEvents=GetAppClass().FeaturedEvents;
			FeaturedEvents.AddEventHandler(this);
			if (m_fShowFeaturedMark)
				m_FeaturedEventsMatcher.BeginMatching(FeaturedEvents.GetSettings());

			GetCurrentEpgTime(&m_stCurTime);
			::SetTimer(hwnd,TIMER_ID_UPDATECURTIME,1000,NULL);
		}
		return 0;

	case WM_PAINT:
		OnPaint(hwnd);
		return 0;

	case WM_SIZE:
		{
			SIZE Size,Page;
			POINT Pos;

			GetProgramGuideSize(&Size);
			GetPageSize(&Page);
			Pos=m_ScrollPos;
			if (Pos.x>max(Size.cx-Page.cx,0))
				Pos.x=max(Size.cx-Page.cx,0);
			if (Pos.y>max(Size.cy-Page.cy,0))
				Pos.y=max(Size.cy-Page.cy,0);
			SetScrollBar();
			SetScrollPos(Pos);
		}
		return 0;

	case WM_VSCROLL:
	case WM_MOUSEWHEEL:
		{
			SIZE Size,Page;
			int Pos;

			GetProgramGuideSize(&Size);
			GetPageSize(&Page);
			Pos=m_ScrollPos.y;
			if (uMsg==WM_VSCROLL) {
				switch (LOWORD(wParam)) {
				case SB_LINEUP:		Pos--;						break;
				case SB_LINEDOWN:	Pos++;						break;
				case SB_PAGEUP:		Pos-=Page.cy;				break;
				case SB_PAGEDOWN:	Pos+=Page.cy;				break;
				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:	Pos=HIWORD(wParam);			break;
				case SB_TOP:		Pos=0;						break;
				case SB_BOTTOM:		Pos=max(Size.cy-Page.cy,0);	break;
				default:	return 0;
				}
			} else {
				Pos-=m_VertWheel.OnMouseWheel(wParam,m_WheelScrollLines);
			}
			if (Pos<0)
				Pos=0;
			else if (Pos>max(Size.cy-Page.cy,0))
				Pos=max(Size.cy-Page.cy,0);
			if (Pos!=m_ScrollPos.y)
				Scroll(0,Pos-m_ScrollPos.y);
		}
		return 0;

	case WM_HSCROLL:
	case WM_MOUSEHWHEEL:
		{
			SIZE Size,Page;
			int Pos;

			GetProgramGuideSize(&Size);
			GetPageSize(&Page);
			Pos=m_ScrollPos.x;
			if (uMsg==WM_HSCROLL) {
				switch (LOWORD(wParam)) {
				case SB_LINELEFT:	Pos-=m_FontHeight;			break;
				case SB_LINERIGHT:	Pos+=m_FontHeight;			break;
				case SB_PAGELEFT:	Pos-=Page.cx;				break;
				case SB_PAGERIGHT:	Pos+=Page.cx;				break;
				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:	Pos=HIWORD(wParam);			break;
				case SB_LEFT:		Pos=0;						break;
				case SB_RIGHT:		Pos=max(Size.cx-Page.cx,0);	break;
				default:	return 0;
				}
			} else {
				Pos+=m_HorzWheel.OnMouseHWheel(wParam,m_FontHeight);
			}
			if (Pos<0)
				Pos=0;
			else if (Pos>max(Size.cx-Page.cx,0))
				Pos=max(Size.cx-Page.cx,0);
			if (Pos!=m_ScrollPos.x)
				Scroll(Pos-m_ScrollPos.x,0);
		}
		return uMsg==WM_MOUSEHWHEEL;

	case WM_LBUTTONDOWN:
		{
			POINT pt;
			RECT rc;

			::SetFocus(hwnd);
			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			::GetClientRect(hwnd,&rc);
			if (pt.y<m_HeaderHeight
					&& pt.x>=m_TimeBarWidth && pt.x<rc.right-m_TimeBarWidth) {
				if (m_ListMode==LIST_SERVICES) {
					const int HeaderWidth=m_ItemWidth+m_Style.ColumnMargin*2;
					int x=pt.x+m_ScrollPos.x-m_TimeBarWidth;

					if (x<(int)m_EventLayoutList.Length()*HeaderWidth) {
						int Service=x/HeaderWidth;
						int ChevronArea=m_Style.HeaderChevronSize.Width+
							m_Style.HeaderChevronMargin.Right+m_Style.HeaderPadding.Right;

						if (x%HeaderWidth<HeaderWidth-(m_Style.HeaderChevronMargin.Left+ChevronArea)) {
							if (m_pEventHandler) {
								ProgramGuide::CServiceInfo *pServiceInfo=m_ServiceList.GetItem(Service);

								if (pServiceInfo!=NULL) {
									TVTest::String BonDriver(pServiceInfo->GetBonDriverFileName());
									CServiceInfoData ServiceInfo(pServiceInfo->GetServiceInfoData());

									m_pEventHandler->OnServiceTitleLButtonDown(
										BonDriver.c_str(),&ServiceInfo);
								}
							}
						} else if (x%HeaderWidth>=HeaderWidth-ChevronArea) {
							SetWeekListMode(Service);
						}
					}
				} else if (m_ListMode==LIST_WEEK) {
					SetServiceListMode();
				}
			} else if (pt.x<m_TimeBarWidth || pt.x>=rc.right-m_TimeBarWidth) {
				if (m_ListMode==LIST_SERVICES) {
					if (m_Day>DAY_FIRST && pt.y<m_HeaderHeight) {
						::SendMessage(hwnd,WM_COMMAND,CM_PROGRAMGUIDE_DAY_FIRST+(m_Day-1),0);
					} else if (m_Day<DAY_LAST) {
						int y=(m_Hours*m_LinesPerHour-m_ScrollPos.y)*GetLineHeight();
						if (pt.y-m_HeaderHeight>=y-m_TimeBarWidth
								&& pt.y-m_HeaderHeight<y) {
							::SendMessage(hwnd,WM_COMMAND,CM_PROGRAMGUIDE_DAY_FIRST+(m_Day+1),0);
						}
					}
				}
			} else if (m_fDragScroll) {
				m_fScrolling=true;
				m_DragInfo.StartCursorPos=pt;
				m_DragInfo.StartScrollPos=m_ScrollPos;
				::SetCursor(m_hDragCursor2);
				::SetCapture(hwnd);
				m_EventInfoPopupManager.SetEnable(false);
			} else {
				SelectEventByPosition(pt.x,pt.y);
				m_EventInfoPopupManager.Popup(pt.x,pt.y);
			}
		}
		return 0;

	case WM_LBUTTONUP:
		if (m_fScrolling) {
			::ReleaseCapture();
		}
		return 0;

	case WM_LBUTTONDBLCLK:
		{
			POINT pt={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};

			if (SelectEventByPosition(pt.x,pt.y)) {
				if (m_pProgramCustomizer!=NULL) {
					const ProgramGuide::CEventItem *pItem=
						GetEventItem(m_CurEventItem.ListIndex,m_CurEventItem.EventIndex);

					if (pItem!=NULL) {
						RECT ItemRect;

						GetEventRect(m_CurEventItem.ListIndex,m_CurEventItem.EventIndex,&ItemRect);
						m_pProgramCustomizer->OnLButtonDoubleClick(
							*pItem->GetEventInfo(),pt,ItemRect);
					}
				}
			}
		}
		return 0;

	case WM_RBUTTONDOWN:
		{
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);

			::SetFocus(hwnd);
			SelectEventByPosition(x,y);
		}
		return 0;

	case WM_RBUTTONUP:
		{
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);

			ShowPopupMenu(x,y);
		}
		return 0;

	case WM_MOUSEMOVE:
		if (m_fScrolling) {
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			int XScroll,YScroll;

			XScroll=(m_DragInfo.StartScrollPos.x+(m_DragInfo.StartCursorPos.x-x))-m_ScrollPos.x;
			YScroll=(m_DragInfo.StartScrollPos.y+(m_DragInfo.StartCursorPos.y-y)/GetLineHeight())-m_ScrollPos.y;
			if (XScroll!=0 || YScroll!=0)
				Scroll(XScroll,YScroll);
		}
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT) {
			POINT pt;
			RECT rc;

			::GetCursorPos(&pt);
			::ScreenToClient(hwnd,&pt);
			::GetClientRect(hwnd,&rc);
			if (pt.y<m_HeaderHeight
					&& pt.x>=m_TimeBarWidth
					&& pt.x<rc.right-m_TimeBarWidth) {
				if (m_ListMode==LIST_SERVICES) {
					const int HeaderWidth=m_ItemWidth+m_Style.ColumnMargin*2;
					const int ChevronArea=m_Style.HeaderChevronSize.Width+
						m_Style.HeaderChevronMargin.Right+m_Style.HeaderPadding.Right;
					int x=pt.x+m_ScrollPos.x-m_TimeBarWidth;
					if (x<(int)m_EventLayoutList.Length()*HeaderWidth
							&& (x%HeaderWidth<HeaderWidth-(m_Style.HeaderChevronMargin.Left+ChevronArea)
								|| x%HeaderWidth>=HeaderWidth-ChevronArea)) {
						::SetCursor(GetActionCursor());
						return TRUE;
					}
				} else if (m_ListMode==LIST_WEEK) {
					::SetCursor(GetActionCursor());
					return TRUE;
				}
			} else if (pt.x<m_TimeBarWidth
					|| pt.x>=rc.right-m_TimeBarWidth) {
				if (m_ListMode==LIST_SERVICES) {
					if (m_Day>DAY_FIRST
							&& pt.y<m_HeaderHeight) {
						::SetCursor(GetActionCursor());
						return TRUE;
					}
					int y=(m_Hours*m_LinesPerHour-m_ScrollPos.y)*GetLineHeight();
					if (m_Day<DAY_LAST
							&& pt.y-m_HeaderHeight>=y-m_TimeBarWidth
							&& pt.y-m_HeaderHeight<y) {
						::SetCursor(GetActionCursor());
						return TRUE;
					}
				}
			} else if (m_fDragScroll) {
				::SetCursor(m_hDragCursor1);
				return TRUE;
			}
		}
		break;

	case WM_CAPTURECHANGED:
		if (m_fScrolling) {
			m_fScrolling=false;
			::SetCursor(m_hDragCursor1);
			m_EventInfoPopupManager.SetEnable(m_fShowToolTip);
		}
		return 0;

	case WM_KEYDOWN:
		{
			static const struct {
				WORD KeyCode;
				WORD Message;
				WORD Request;
			} KeyMap[] = {
				{VK_PRIOR,	WM_VSCROLL,	SB_PAGEUP},
				{VK_NEXT,	WM_VSCROLL,	SB_PAGEDOWN},
				{VK_UP,		WM_VSCROLL,	SB_LINEUP},
				{VK_DOWN,	WM_VSCROLL,	SB_LINEDOWN},
				{VK_LEFT,	WM_HSCROLL,	SB_LINEUP},
				{VK_RIGHT,	WM_HSCROLL,	SB_LINEDOWN},
				{VK_HOME,	WM_VSCROLL,	SB_TOP},
				{VK_END,	WM_VSCROLL,	SB_BOTTOM},
			};

			for (int i=0;i<lengthof(KeyMap);i++) {
				if (wParam==(WPARAM)KeyMap[i].KeyCode) {
					::SendMessage(hwnd,KeyMap[i].Message,KeyMap[i].Request,0);
					return 0;
				}
			}

			if (m_pEventHandler!=NULL
					&& m_pEventHandler->OnKeyDown((UINT)wParam,(UINT)lParam))
				return 0;
		}
		break;

	case WM_TIMER:
		if (wParam==TIMER_ID_UPDATECURTIME) {
			if (m_Day==DAY_TODAY) {
				SYSTEMTIME st;

				GetCurrentEpgTime(&st);
				if (m_stCurTime.wMinute!=st.wMinute
						|| m_stCurTime.wHour!=st.wHour
						|| m_stCurTime.wDay!=st.wDay
						|| m_stCurTime.wMonth!=st.wMonth
						|| m_stCurTime.wYear!=st.wYear) {
					int OldTimeLinePos=GetCurTimeLinePos(),NewTimeLinePos;

					m_stCurTime=st;
					NewTimeLinePos=GetCurTimeLinePos();
					if (NewTimeLinePos!=OldTimeLinePos) {
						RECT rc,rcGuide;
						int Offset;

						::GetClientRect(hwnd,&rc);
						GetProgramGuideRect(&rcGuide);
						Offset=rcGuide.top-m_ScrollPos.y*GetLineHeight();
						rc.top=Offset+OldTimeLinePos-m_FontHeight/2;
						rc.bottom=Offset+NewTimeLinePos+m_FontHeight/2;
						::InvalidateRect(hwnd,&rc,FALSE);
					}
				}
			}
		}
		return 0;

	case WM_COMMAND:
		OnCommand(LOWORD(wParam));
		return 0;

	case WM_DESTROY:
		ShowProgramSearch(false);
		if (m_pProgramCustomizer!=NULL)
			m_pProgramCustomizer->Finalize();
		m_Tooltip.Destroy();
		if (m_pEventHandler!=NULL)
			m_pEventHandler->OnDestroy();
		m_Chevron.Destroy();
		m_EpgIcons.Destroy();
		m_TextDrawClient.Finalize();
		GetAppClass().FeaturedEvents.RemoveEventHandler(this);
		return 0;
	}

	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


void CProgramGuide::OnCommand(int id)
{
	switch (id) {
	case CM_PROGRAMGUIDE_UPDATE:
		if (!m_fEpgUpdating) {
			CAppMain &App=GetAppClass();

			if (!App.EpgCaptureManager.IsCapturing()) {
				TCHAR szBonDriver[MAX_PATH];

				if (m_pEventHandler!=NULL
						&& m_pChannelProvider!=NULL
						&& m_pChannelProvider->GetBonDriver(szBonDriver,lengthof(szBonDriver))) {
					CChannelList ChannelList;

					GetChannelList(&ChannelList,false);
					if (ChannelList.NumChannels()>0) {
						m_EpgUpdateProgress.Clear();
						if (App.EpgCaptureManager.BeginCapture(szBonDriver,&ChannelList)) {
							m_fEpgUpdating=true;
							SetCaption();
						}
					}
				}
			}
		}
		return;

	case CM_PROGRAMGUIDE_ENDUPDATE:
		if (m_fEpgUpdating) {
			GetAppClass().EpgCaptureManager.EndCapture();
		}
		return;

	case CM_PROGRAMGUIDE_REFRESH:
		Refresh();
		return;

	case CM_PROGRAMGUIDE_IEPGASSOCIATE:
		if (m_CurEventItem.fSelected) {
			const ProgramGuide::CEventLayout *pLayout=m_EventLayoutList[m_CurEventItem.ListIndex];

			if (pLayout!=NULL) {
				const ProgramGuide::CEventItem *pEvent=pLayout->GetItem(m_CurEventItem.EventIndex);

				if (pEvent!=NULL) {
					ExecuteiEpgAssociate(pLayout->GetServiceInfo(),pEvent->GetEventInfo());
				}
			}
		}
		return;

	case CM_PROGRAMGUIDE_SEARCH:
		ShowProgramSearch(!m_ProgramSearch.IsVisible());
		return;

	case CM_PROGRAMGUIDE_CHANNELSETTINGS:
		{
			CEpgChannelSettings Settings(this);

			POINT pt={0,0};
			::ClientToScreen(m_hwnd,&pt);
			Settings.SetPosition(pt.x,pt.y,0,0);

			if (Settings.Show(m_hwnd))
				UpdateProgramGuide(false);
		}
		return;

	case CM_PROGRAMGUIDE_ALWAYSONTOP:
		if (m_pFrame!=NULL)
			m_pFrame->SetAlwaysOnTop(!m_pFrame->GetAlwaysOnTop());
		return;

	case CM_PROGRAMGUIDE_DRAGSCROLL:
		SetDragScroll(!m_fDragScroll);
		return;

	case CM_PROGRAMGUIDE_POPUPEVENTINFO:
		SetShowToolTip(!m_fShowToolTip);
		return;

	case CM_PROGRAMGUIDE_KEEPTIMEPOS:
		SetKeepTimePos(!m_fKeepTimePos);
		return;

	case CM_PROGRAMGUIDE_ADDTOFAVORITES:
		if (m_pChannelProvider!=NULL
				&& m_CurrentChannelGroup>=0) {
			CProgramGuideFavorites::FavoriteInfo Info;
			TCHAR szText[256];

			m_pChannelProvider->GetName(szText,lengthof(szText));
			Info.Name=szText;
			m_pChannelProvider->GetGroupID(m_CurrentChannelGroup,&Info.GroupID);
			m_pChannelProvider->GetGroupName(m_CurrentChannelGroup,szText,lengthof(szText));
			Info.Label=szText;
			Info.SetDefaultColors();

			CProgramGuideFavoritesDialog Dialog(m_Favorites);
			Dialog.AddNewItem(Info);
			if (Dialog.Show(m_hwnd)) {
				m_Favorites=Dialog.GetFavorites();
				if (m_pFrame!=NULL)
					m_pFrame->OnFavoritesChanged();
			}
		}
		return;

	case CM_PROGRAMGUIDE_ORGANIZEFAVORITES:
		{
			CProgramGuideFavoritesDialog Dialog(m_Favorites);

			if (Dialog.Show(m_hwnd)) {
				m_Favorites=Dialog.GetFavorites();
				if (m_pFrame!=NULL)
					m_pFrame->OnFavoritesChanged();
			}
		}
		return;

	case CM_PROGRAMGUIDE_SHOWFEATUREDMARK:
		SetShowFeaturedMark(!m_fShowFeaturedMark);
		return;

	default:
		if (id>=CM_PROGRAMGUIDE_DAY_FIRST
				&& id<=CM_PROGRAMGUIDE_DAY_LAST) {
			SetViewDay(id-CM_PROGRAMGUIDE_DAY_FIRST);
			return;
		}

		if (id>=CM_PROGRAMGUIDE_FILTER_FIRST
				&& id<=CM_PROGRAMGUIDE_FILTER_LAST) {
			unsigned int Filter=m_Filter^(1<<(id-CM_PROGRAMGUIDE_FILTER_FIRST));

			if (id==CM_PROGRAMGUIDE_FILTER_ORIGINAL) {
				if ((Filter&FILTER_ORIGINAL)!=0)
					Filter&=~FILTER_RERUN;
			} else if (id==CM_PROGRAMGUIDE_FILTER_RERUN) {
				if ((Filter&FILTER_RERUN)!=0)
					Filter&=~FILTER_ORIGINAL;
			}
			SetFilter(Filter);
			return;
		}

		if (id>=CM_PROGRAMGUIDE_CHANNELGROUP_FIRST
				&& id<=CM_PROGRAMGUIDE_CHANNELGROUP_LAST) {
			if (m_fEpgUpdating)
				OnCommand(CM_PROGRAMGUIDE_ENDUPDATE);
			StoreTimePos();
			if (SetCurrentChannelGroup(id-CM_PROGRAMGUIDE_CHANNELGROUP_FIRST))
				UpdateServiceList();
			return;
		}

		if (id>=CM_PROGRAMGUIDE_CHANNELPROVIDER_FIRST
				&& id<=CM_PROGRAMGUIDE_CHANNELPROVIDER_LAST) {
			if (m_fEpgUpdating)
				OnCommand(CM_PROGRAMGUIDE_ENDUPDATE);
			SetChannelProvider(id-CM_PROGRAMGUIDE_CHANNELPROVIDER_FIRST,0);
			return;
		}

		if (id>=CM_PROGRAMGUIDE_CUSTOM_FIRST
				&& id<=CM_PROGRAMGUIDE_CUSTOM_LAST) {
			if (m_CurEventItem.fSelected) {
				if (m_pProgramCustomizer!=NULL) {
					const ProgramGuide::CEventItem *pEvent=
						GetEventItem(m_CurEventItem.ListIndex,m_CurEventItem.EventIndex);

					if (pEvent!=NULL)
						m_pProgramCustomizer->ProcessMenu(*pEvent->GetEventInfo(),id);
				}
			} else {
				if (m_pEventHandler!=NULL)
					m_pEventHandler->OnMenuSelected(id);
			}
			return;
		}

		if (id>=CM_PROGRAMGUIDETOOL_FIRST
				&& id<=CM_PROGRAMGUIDETOOL_LAST) {
			if (m_CurEventItem.fSelected) {
				const ProgramGuide::CEventLayout *pLayout=m_EventLayoutList[m_CurEventItem.ListIndex];

				if (pLayout!=NULL) {
					const ProgramGuide::CEventItem *pEvent=pLayout->GetItem(m_CurEventItem.EventIndex);

					if (pEvent!=NULL) {
						ExecuteTool(id-CM_PROGRAMGUIDETOOL_FIRST,
									pLayout->GetServiceInfo(),pEvent->GetEventInfo());
					}
				}
			}
			return;
		}

		if (id>=CM_PROGRAMGUIDE_FAVORITE_FIRST
				&& id<=CM_PROGRAMGUIDE_FAVORITE_LAST) {
			const CProgramGuideFavorites::FavoriteInfo *pInfo=
				m_Favorites.Get(id-CM_PROGRAMGUIDE_FAVORITE_FIRST);

			if (pInfo!=NULL) {
				TCHAR szName[256];

				for (int i=0;EnumChannelProvider(i,szName,lengthof(szName));i++) {
					if (::lstrcmpi(szName,pInfo->Name.c_str())==0) {
						SetChannelProvider(i,pInfo->GroupID.c_str());
						return;
					}
				}
			}
			return;
		}

		if (id>=CM_CHANNEL_FIRST && id<=CM_CHANNEL_LAST) {
			SetWeekListMode(id-CM_CHANNEL_FIRST);
			return;
		}
	}

	if (m_pFrame!=NULL)
		m_pFrame->OnCommand(id);
}


void CProgramGuide::ShowPopupMenu(int x,int y)
{
	POINT pt={x,y};
	HMENU hmenu,hmenuPopup;
	TCHAR szText[256],szMenu[64];

	hmenu=::LoadMenu(GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDM_PROGRAMGUIDE));
	hmenuPopup=::GetSubMenu(hmenu,0);

	::CheckMenuRadioItem(hmenu,CM_PROGRAMGUIDE_DAY_FIRST,
						 CM_PROGRAMGUIDE_DAY_LAST,
						 CM_PROGRAMGUIDE_DAY_FIRST+m_Day,MF_BYCOMMAND);
	MENUITEMINFO mii;
	mii.cbSize=sizeof(mii);
	mii.fMask=MIIM_STRING;
	mii.dwTypeData=szText;
	for (int i=CM_PROGRAMGUIDE_DAY_FIRST;i<=CM_PROGRAMGUIDE_DAY_LAST;i++) {
		SYSTEMTIME st;

		GetDayTimeRange(i-CM_PROGRAMGUIDE_DAY_FIRST,&st,NULL);
		mii.cch=lengthof(szText);
		::GetMenuItemInfo(hmenu,i,FALSE,&mii);
		int Length=::lstrlen(szText);
		StdUtil::snprintf(szText+Length,lengthof(szText)-Length,TEXT(" %d/%d(%s) %d時～"),
						  st.wMonth,st.wDay,GetDayOfWeekText(st.wDayOfWeek),st.wHour);
		::SetMenuItemInfo(hmenu,i,FALSE,&mii);
	}

	HMENU hmenuChannelGroup=::GetSubMenu(hmenuPopup,MENU_CHANNELGROUP);
	ClearMenu(hmenuChannelGroup);
	int ChannelGroupCount=GetChannelGroupCount();
	if (ChannelGroupCount>MAX_CHANNEL_GROUP_MENU_ITEMS)
		ChannelGroupCount=MAX_CHANNEL_GROUP_MENU_ITEMS;
	for (int i=0;i<ChannelGroupCount;i++) {
		if (GetChannelGroupName(i,szText,lengthof(szText))) {
			CopyToMenuText(szText,szMenu,lengthof(szMenu));
			::AppendMenu(hmenuChannelGroup,MF_STRING | MF_ENABLED,
						 CM_PROGRAMGUIDE_CHANNELGROUP_FIRST+i,szMenu);
		}
	}
	if (::GetMenuItemCount(hmenuChannelGroup)>0
			&& m_CurrentChannelGroup>=0
			&& m_CurrentChannelGroup<ChannelGroupCount) {
		::CheckMenuRadioItem(hmenuChannelGroup,
							 CM_PROGRAMGUIDE_CHANNELGROUP_FIRST,
							 CM_PROGRAMGUIDE_CHANNELGROUP_FIRST+ChannelGroupCount-1,
							 CM_PROGRAMGUIDE_CHANNELGROUP_FIRST+m_CurrentChannelGroup,
							 MF_BYCOMMAND);
	}
	if (m_pChannelProviderManager!=NULL) {
		int ProviderCount=(int)m_pChannelProviderManager->GetChannelProviderCount();
		if (ProviderCount>0) {
			if (ProviderCount>MAX_CHANNEL_PROVIDER_MENU_ITEMS)
				ProviderCount=MAX_CHANNEL_PROVIDER_MENU_ITEMS;
			::AppendMenu(hmenuChannelGroup,MFT_SEPARATOR,0,NULL);
			for (int i=0;i<ProviderCount;i++) {
				if (EnumChannelProvider(i,szText,lengthof(szText))) {
					CopyToMenuText(szText,szMenu,lengthof(szMenu));
					::AppendMenu(hmenuChannelGroup,MFT_STRING | MFS_ENABLED,
								 CM_PROGRAMGUIDE_CHANNELPROVIDER_FIRST+i,szMenu);
				}
			}
			if (m_CurrentChannelProvider>=0
					&& m_CurrentChannelProvider<ProviderCount) {
				::CheckMenuRadioItem(hmenuChannelGroup,
									 CM_PROGRAMGUIDE_CHANNELPROVIDER_FIRST,
									 CM_PROGRAMGUIDE_CHANNELPROVIDER_FIRST+ProviderCount-1,
									 CM_PROGRAMGUIDE_CHANNELPROVIDER_FIRST+m_CurrentChannelProvider,
									 MF_BYCOMMAND);
			}
		}
	}
	::EnableMenuItem(hmenuPopup,MENU_CHANNELGROUP,
					 MF_BYPOSITION | (::GetMenuItemCount(hmenuChannelGroup)>0?MF_ENABLED:MF_GRAYED));

	for (int i=0;(m_Filter>>i)!=0;i++) {
		if (((m_Filter>>i)&1)!=0)
			::CheckMenuItem(hmenu,CM_PROGRAMGUIDE_FILTER_FIRST+i,MF_BYCOMMAND | MF_CHECKED);
	}

	::CheckMenuItem(hmenu,CM_PROGRAMGUIDE_SEARCH,
		MF_BYCOMMAND | (m_ProgramSearch.IsCreated()?MF_CHECKED:MF_UNCHECKED));
	::EnableMenuItem(hmenu,CM_PROGRAMGUIDE_CHANNELSETTINGS,
		MF_BYCOMMAND |
			(m_pChannelProvider!=NULL &&
			m_pChannelProvider->GetChannelCount(m_CurrentChannelGroup)>0?MF_ENABLED:MF_GRAYED));
	::EnableMenuItem(hmenu,CM_PROGRAMGUIDE_UPDATE,
		MF_BYCOMMAND |
			(!m_fEpgUpdating
			&& m_pChannelProvider!=NULL
			&& m_pChannelProvider->GetBonDriver(szText,lengthof(szText))?MF_ENABLED:MF_GRAYED));
	::EnableMenuItem(hmenu,CM_PROGRAMGUIDE_ENDUPDATE,
		MF_BYCOMMAND | (m_fEpgUpdating?MF_ENABLED:MF_GRAYED));
	::CheckMenuItem(hmenu,CM_PROGRAMGUIDE_ALWAYSONTOP,
		MF_BYCOMMAND | (m_pFrame->GetAlwaysOnTop()?MF_CHECKED:MF_UNCHECKED));
	::CheckMenuItem(hmenu,CM_PROGRAMGUIDE_DRAGSCROLL,
		MF_BYCOMMAND | (m_fDragScroll?MF_CHECKED:MF_UNCHECKED));
	::CheckMenuItem(hmenu,CM_PROGRAMGUIDE_POPUPEVENTINFO,
		MF_BYCOMMAND | (m_fShowToolTip?MF_CHECKED:MF_UNCHECKED));
	::CheckMenuItem(hmenu,CM_PROGRAMGUIDE_SHOWFEATUREDMARK,
		MF_BYCOMMAND | (m_fShowFeaturedMark?MF_CHECKED:MF_UNCHECKED));
	::CheckMenuItem(hmenu,CM_PROGRAMGUIDE_KEEPTIMEPOS,
		MF_BYCOMMAND | (m_fKeepTimePos?MF_CHECKED:MF_UNCHECKED));
	::EnableMenuItem(hmenu,CM_PROGRAMGUIDE_IEPGASSOCIATE,
		MF_BYCOMMAND | (m_CurEventItem.fSelected?MF_ENABLED:MF_GRAYED));
	::EnableMenuItem(hmenu,CM_PROGRAMGUIDE_ADDTOFAVORITES,
		MF_BYCOMMAND | (m_pChannelProvider!=NULL?MF_ENABLED:MF_GRAYED));

	if (m_CurEventItem.fSelected) {
		if (m_pProgramCustomizer!=NULL) {
			const ProgramGuide::CEventItem *pItem=
				GetEventItem(m_CurEventItem.ListIndex,m_CurEventItem.EventIndex);

			if (pItem!=NULL) {
				RECT ItemRect;

				GetEventRect(m_CurEventItem.ListIndex,m_CurEventItem.EventIndex,&ItemRect);
				m_pProgramCustomizer->InitializeMenu(
					*pItem->GetEventInfo(),
					hmenuPopup,CM_PROGRAMGUIDE_CUSTOM_FIRST,
					pt,ItemRect);
			}
		}

		AppendToolMenu(hmenuPopup);
	} else {
		if (m_pEventHandler!=NULL) {
			m_pEventHandler->OnMenuInitialize(hmenuPopup,CM_PROGRAMGUIDE_CUSTOM_FIRST);
		}
	}

	if (m_pFrame!=NULL)
		m_pFrame->OnMenuInitialize(hmenuPopup);

	::ClientToScreen(m_hwnd,&pt);
	::TrackPopupMenu(hmenuPopup,TPM_RIGHTBUTTON,pt.x,pt.y,
					 0,m_hwnd,NULL);
	::DestroyMenu(hmenu);
}


void CProgramGuide::AppendToolMenu(HMENU hmenu) const
{
	if (m_ToolList.NumTools()>0) {
		::AppendMenu(hmenu,MF_SEPARATOR | MF_ENABLED,0,NULL);
		for (size_t i=0;i<m_ToolList.NumTools();i++) {
			const CProgramGuideTool *pTool=m_ToolList.GetTool(i);
			TCHAR szText[256];

			CopyToMenuText(pTool->GetName(),szText,lengthof(szText));
			::AppendMenu(hmenu,MF_STRING | MF_ENABLED,
						 CM_PROGRAMGUIDETOOL_FIRST+i,szText);
		}
	}
}


bool CProgramGuide::ExecuteiEpgAssociate(const ProgramGuide::CServiceInfo *pServiceInfo,
										 const CEventInfoData *pEventInfo)
{
	if (pServiceInfo==NULL || pEventInfo==NULL)
		return false;

	TCHAR szFileName[MAX_PATH];
	GetAppClass().GetAppDirectory(szFileName);
	::PathAppend(szFileName,TEXT("iepg.tvpid"));
	if (!pServiceInfo->SaveiEpgFile(pEventInfo,szFileName,true))
		return false;

	return (int)::ShellExecute(NULL,NULL,szFileName,NULL,NULL,SW_SHOWNORMAL)>32;
}


bool CProgramGuide::ExecuteTool(int Tool,
								const ProgramGuide::CServiceInfo *pServiceInfo,
								const CEventInfoData *pEventInfo)
{
	if (pServiceInfo==NULL || pEventInfo==NULL)
		return false;

	CProgramGuideTool *pTool=m_ToolList.GetTool(Tool);
	if (pTool==NULL)
		return false;

	return pTool->Execute(pServiceInfo,pEventInfo,
						  ::GetAncestor(m_hwnd,GA_ROOT));
}


void CProgramGuide::OnFeaturedEventsSettingsChanged(CFeaturedEvents &FeaturedEvents)
{
	if (m_fShowFeaturedMark) {
		m_FeaturedEventsMatcher.BeginMatching(FeaturedEvents.GetSettings());
		Invalidate();
	}
}




CProgramGuide::CEventHandler::CEventHandler()
	: m_pProgramGuide(NULL)
{
}


CProgramGuide::CEventHandler::~CEventHandler()
{
	if (m_pProgramGuide!=NULL)
		m_pProgramGuide->SetEventHandler(NULL);
}


CProgramGuide::CFrame::CFrame()
	: m_pProgramGuide(NULL)
{
}


CProgramGuide::CFrame::~CFrame()
{
	if (m_pProgramGuide!=NULL)
		m_pProgramGuide->SetFrame(NULL);
}


CProgramGuide::CProgramCustomizer::CProgramCustomizer()
	: m_pProgramGuide(NULL)
{
}


CProgramGuide::CProgramCustomizer::~CProgramCustomizer()
{
	if (m_pProgramGuide!=NULL)
		m_pProgramGuide->SetEventHandler(NULL);
}


CProgramGuide::CEventInfoPopupHandler::CEventInfoPopupHandler(CProgramGuide *pProgramGuide)
	: m_pProgramGuide(pProgramGuide)
{
}


bool CProgramGuide::CEventInfoPopupHandler::HitTest(int x,int y,LPARAM *pParam)
{
	/*if (m_pProgramGuide->m_fShowToolTip)*/ {
		int List,Event;

		if (m_pProgramGuide->EventHitTest(x,y,&List,&Event)) {
			*pParam=MAKELONG(List,Event);
			return true;
		}
	}
	return false;
}


bool CProgramGuide::CEventInfoPopupHandler::ShowPopup(LPARAM Param,CEventInfoPopup *pPopup)
{
	int List=LOWORD(Param),Event=HIWORD(Param);
	const ProgramGuide::CEventLayout *pLayout=m_pProgramGuide->m_EventLayoutList[List];
	if (pLayout!=NULL) {
		const ProgramGuide::CEventItem *pItem=pLayout->GetItem(Event);
		if (pItem!=NULL) {
			const CEventInfoData *pEventInfo;
			if (pItem->GetCommonEventInfo()!=NULL)
				pEventInfo=pItem->GetCommonEventInfo();
			else
				pEventInfo=pItem->GetEventInfo();
			/*
			if (pEventInfo->m_EventName.empty() && pEventInfo->m_bCommonEvent) {
				const CProgramGuideItem *pCommonItem=m_pProgramGuide->m_ServiceList.GetEventByIDs(
					pServiceInfo->GetTSID(),
					pEventInfo->m_CommonEventInfo.ServiceID,
					pEventInfo->m_CommonEventInfo.EventID);
				if (pCommonItem!=NULL)
					pEventInfo=&pCommonItem->GetEventInfo();
			}
			*/

			pPopup->SetTitleColor(m_pProgramGuide->m_EpgTheme.GetGenreColor(*pEventInfo),
								  m_pProgramGuide->m_EpgTheme.GetColor(CEpgTheme::COLOR_EVENTNAME));

			const ProgramGuide::CServiceInfo *pServiceInfo=pLayout->GetServiceInfo();
			int IconWidth,IconHeight;
			pPopup->GetPreferredIconSize(&IconWidth,&IconHeight);
			HICON hIcon=GetAppClass().LogoManager.CreateLogoIcon(
				pServiceInfo->GetNetworkID(),pServiceInfo->GetServiceID(),
				IconWidth,IconHeight);

			RECT rc;
			POINT pt;
			m_pProgramGuide->GetEventRect(List,Event,&rc);
			pt.x=rc.left;
			pt.y=rc.bottom;
			::ClientToScreen(m_pProgramGuide->m_hwnd,&pt);
			pPopup->GetDefaultPopupPosition(&rc);
			if (rc.top>pt.y) {
				rc.bottom=pt.y+(rc.bottom-rc.top);
				rc.top=pt.y;
			}

			if (!pPopup->Show(pEventInfo,&rc,hIcon,pServiceInfo->GetServiceName())) {
				if (hIcon!=NULL)
					::DestroyIcon(hIcon);
				return false;
			}

			return true;
		}
	}

	return false;
}


bool CProgramGuide::CEventInfoPopupHandler::OnMenuPopup(HMENU hmenu)
{
	::AppendMenu(hmenu,MFT_SEPARATOR,0,NULL);
	::AppendMenu(hmenu,MFT_STRING | (CEventInfoPopup::CEventHandler::m_pPopup->IsSelected()?MFS_ENABLED:MFS_GRAYED),
				 COMMAND_FIRST,TEXT("選択文字列を検索(&S)"));
	return true;
}


void CProgramGuide::CEventInfoPopupHandler::OnMenuSelected(int Command)
{
	LPTSTR pszText=CEventInfoPopup::CEventHandler::m_pPopup->GetSelectedText();
	if (pszText!=NULL) {
		if (!m_pProgramGuide->m_ProgramSearch.IsCreated())
			m_pProgramGuide->SendMessage(WM_COMMAND,CM_PROGRAMGUIDE_SEARCH,0);
		m_pProgramGuide->m_ProgramSearch.Search(pszText);
		delete [] pszText;
	}
}




CProgramGuide::CProgramSearchEventHandler::CProgramSearchEventHandler(CProgramGuide *pProgramGuide)
	: m_pProgramGuide(pProgramGuide)
{
}


bool CProgramGuide::CProgramSearchEventHandler::OnSearch()
{
	// 番組の検索

	SYSTEMTIME stFirst;
	m_pProgramGuide->GetDayTimeRange(DAY_TODAY,&stFirst,NULL);

	for (size_t i=0;i<m_pProgramGuide->m_ServiceList.NumServices();i++) {
		const ProgramGuide::CServiceInfo *pServiceInfo=m_pProgramGuide->m_ServiceList.GetItem(i);
		const CEventInfoData *pEventInfo;
		int j;

		for (j=0;j<pServiceInfo->NumEvents();j++) {
			pEventInfo=pServiceInfo->GetEvent(j);
			SYSTEMTIME stEnd;
			pEventInfo->GetEndTime(&stEnd);
			if (CompareSystemTime(&stEnd,&stFirst)>0)
				break;
		}

		for (;j<pServiceInfo->NumEvents();j++) {
			pEventInfo=pServiceInfo->GetEvent(j);
			if (!pEventInfo->m_bCommonEvent && Match(pEventInfo)) {
				AddSearchResult(
					new CSearchEventInfo(
						*pEventInfo,
						CTunerChannelInfo(pServiceInfo->GetChannelInfo(),
										  pServiceInfo->GetBonDriverFileName())));
			}
		}
	}

	if (m_pSearchDialog->GetSearchTarget()==SEARCH_TARGET_ALL) {
		// 全ての番組から検索
		CAppMain &App=GetAppClass();
		CChannelList ServiceList;

		App.DriverManager.GetAllServiceList(&ServiceList);

		for (int i=0;i<ServiceList.NumChannels();i++) {
			const CTunerChannelInfo *pChInfo=
				static_cast<const CTunerChannelInfo*>(ServiceList.GetChannelInfo(i));

			if (m_pProgramGuide->m_ServiceList.GetItemByIDs(
					pChInfo->GetTransportStreamID(),pChInfo->GetServiceID())==NULL) {
				m_pProgramGuide->m_pProgramList->UpdateService(
					pChInfo->GetNetworkID(),pChInfo->GetTransportStreamID(),pChInfo->GetServiceID());

				const CEpgServiceInfo *pServiceInfo=
					m_pProgramGuide->m_pProgramList->GetServiceInfo(
						pChInfo->GetNetworkID(),pChInfo->GetTransportStreamID(),pChInfo->GetServiceID());
				if (pServiceInfo!=NULL && !pServiceInfo->m_EventList.EventTimeMap.empty()) {
					const CEventInfoList &EventList=pServiceInfo->m_EventList;
					CEventManager::TimeEventInfo Key(stFirst);
					auto itrTime=EventList.EventTimeMap.upper_bound(Key);

					if (itrTime!=EventList.EventTimeMap.begin()) {
						auto itrPrev=itrTime;
						--itrPrev;
						if (itrPrev->StartTime+itrPrev->Duration>Key.StartTime)
							itrTime=itrPrev;
					}

					for (;itrTime!=EventList.EventTimeMap.end();++itrTime) {
						auto itrEvent=EventList.EventDataMap.find(itrTime->EventID);
						if (itrEvent!=EventList.EventDataMap.end()) {
							const CEventInfoData &EventInfo=itrEvent->second;
							if (!EventInfo.m_bCommonEvent && Match(&EventInfo)) {
								AddSearchResult(new CSearchEventInfo(EventInfo,*pChInfo));
							}
						}
					}
				}
			}
		}
	}

	return true;
}


void CProgramGuide::CProgramSearchEventHandler::OnEndSearch()
{
	if (m_pSearchDialog->GetHighlightResult())
		m_pProgramGuide->Invalidate();
}


bool CProgramGuide::CProgramSearchEventHandler::OnClose()
{
	if (m_pSearchDialog->GetHighlightResult())
		m_pProgramGuide->Invalidate();
	return true;
}


bool CProgramGuide::CProgramSearchEventHandler::OnLDoubleClick(
	const CSearchEventInfo *pEventInfo)
{
	// 検索結果の一覧のダブルクリック
	// TODO: 動作をカスタマイズできるようにする
	if (!m_pProgramGuide->IsExcludeService(
			pEventInfo->m_NetworkID,
			pEventInfo->m_TransportStreamID,
			pEventInfo->m_ServiceID))
		DoCommand(CM_PROGRAMGUIDE_JUMPEVENT,pEventInfo);
	return true;
}


bool CProgramGuide::CProgramSearchEventHandler::OnRButtonClick(
	const CSearchEventInfo *pEventInfo)
{
	// 検索結果の一覧の右クリックメニューを表示
	HMENU hmenu=::LoadMenu(GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDM_PROGRAMSEARCH));
	HMENU hmenuPopup=::GetSubMenu(hmenu,0);

	if (m_pProgramGuide->IsExcludeService(
			pEventInfo->m_NetworkID,
			pEventInfo->m_TransportStreamID,
			pEventInfo->m_ServiceID))
		::EnableMenuItem(hmenu,CM_PROGRAMGUIDE_JUMPEVENT,MF_BYCOMMAND | MF_GRAYED);

	if (m_pProgramGuide->m_pProgramCustomizer!=NULL) {
		POINT pt={50,50};
		RECT rc={0,0,100,100};
		m_pProgramGuide->m_pProgramCustomizer->InitializeMenu(
			*pEventInfo,hmenuPopup,CM_PROGRAMGUIDE_CUSTOM_FIRST,pt,rc);
	}

	m_pProgramGuide->AppendToolMenu(hmenuPopup);

	POINT pt;
	::GetCursorPos(&pt);
	int Command=::TrackPopupMenu(hmenuPopup,TPM_RETURNCMD | TPM_RIGHTBUTTON,
								 pt.x,pt.y,0,m_pProgramGuide->GetHandle(),NULL);
	::DestroyMenu(hmenu);

	if (Command>0) {
		DoCommand(Command,pEventInfo);
	}

	return true;
}


void CProgramGuide::CProgramSearchEventHandler::OnHighlightChange(bool fHighlight)
{
	m_pProgramGuide->Invalidate();
}


static int FindChannelFromChannelProvider(
	const CProgramGuideChannelProvider *pChannelProvider,
	const CChannelInfo &ChannelInfo)
{
	const size_t GroupCount=pChannelProvider->GetGroupCount();

	for (size_t i=0;i<GroupCount;i++) {
		const size_t ChannelCount=pChannelProvider->GetChannelCount(i);

		for (size_t j=0;j<ChannelCount;j++) {
			const CChannelInfo *pChInfo=pChannelProvider->GetChannelInfo(i,j);

			if (pChInfo->GetNetworkID()==ChannelInfo.GetNetworkID()
					&& pChInfo->GetTransportStreamID()==ChannelInfo.GetTransportStreamID()
					&& pChInfo->GetServiceID()==ChannelInfo.GetServiceID()) {
				return (int)i;
			}
		}
	}

	return -1;
}

void CProgramGuide::CProgramSearchEventHandler::DoCommand(
	int Command,const CSearchEventInfo *pEventInfo)
{
	ProgramGuide::CServiceInfo *pServiceInfo=
		m_pProgramGuide->m_ServiceList.GetItemByIDs(pEventInfo->m_TransportStreamID,pEventInfo->m_ServiceID);

	if (Command==CM_PROGRAMGUIDE_JUMPEVENT) {
		if (pServiceInfo!=NULL) {
			m_pProgramGuide->JumpEvent(*pEventInfo);
		} else if (m_pProgramGuide->m_pChannelProviderManager!=NULL) {
			if (m_pProgramGuide->m_pChannelProvider!=NULL) {
				const int Group=FindChannelFromChannelProvider(
					m_pProgramGuide->m_pChannelProvider,pEventInfo->GetChannelInfo());
				if (Group>=0) {
					m_pProgramGuide->SetChannelProvider(
						m_pProgramGuide->GetCurrentChannelProvider(),Group);
					m_pProgramGuide->JumpEvent(*pEventInfo);
					return;
				}
			}

			const size_t ProviderCount=m_pProgramGuide->m_pChannelProviderManager->GetChannelProviderCount();
			for (size_t i=0;i<ProviderCount;i++) {
				if ((int)i!=m_pProgramGuide->GetCurrentChannelProvider()) {
					CProgramGuideChannelProvider *pChannelProvider=
						m_pProgramGuide->m_pChannelProviderManager->GetChannelProvider(i);
					pChannelProvider->Update();
					const int Group=FindChannelFromChannelProvider(
						pChannelProvider,pEventInfo->GetChannelInfo());
					if (Group>=0) {
						m_pProgramGuide->SetChannelProvider((int)i,Group);
						m_pProgramGuide->JumpEvent(*pEventInfo);
						return;
					}
				}
			}
		}
	} else if (Command==CM_PROGRAMGUIDE_IEPGASSOCIATE) {
		if (pServiceInfo!=NULL) {
			m_pProgramGuide->ExecuteiEpgAssociate(pServiceInfo,pEventInfo);
		} else {
			ProgramGuide::CServiceInfo ServiceInfo(pEventInfo->GetChannelInfo(),
												   pEventInfo->GetChannelInfo().GetTunerName());
			m_pProgramGuide->ExecuteiEpgAssociate(&ServiceInfo,pEventInfo);
		}
	} else if (Command>=CM_PROGRAMGUIDE_CUSTOM_FIRST
			&& Command<=CM_PROGRAMGUIDE_CUSTOM_LAST) {
		if (m_pProgramGuide->m_pProgramCustomizer!=NULL)
			m_pProgramGuide->m_pProgramCustomizer->ProcessMenu(*pEventInfo,Command);
	} else if (Command>=CM_PROGRAMGUIDETOOL_FIRST
			&& Command<=CM_PROGRAMGUIDETOOL_LAST) {
		const int Tool=Command-CM_PROGRAMGUIDETOOL_FIRST;
		if (pServiceInfo!=NULL) {
			m_pProgramGuide->ExecuteTool(Tool,pServiceInfo,pEventInfo);
		} else {
			ProgramGuide::CServiceInfo ServiceInfo(pEventInfo->GetChannelInfo(),
												   pEventInfo->GetChannelInfo().GetTunerName());
			m_pProgramGuide->ExecuteTool(Tool,&ServiceInfo,pEventInfo);
		}
	}
}




CProgramGuide::ProgramGuideStyle::ProgramGuideStyle()
	: ColumnMargin(4)
	, HeaderPadding(4)
	, HeaderChannelNameMargin(0)
	, HeaderIconMargin(0,0,4,0)
	, HeaderChevronSize(CHEVRON_WIDTH,CHEVRON_HEIGHT)
	, HeaderChevronMargin(8,0,0,0)
	, HeaderShadowHeight(8)
	, EventLeading(1)
	, EventLineSpacing(0)
	, EventPadding(0,0,2,0)
	, EventIconSize(CEpgIcons::DEFAULT_ICON_WIDTH,CEpgIcons::DEFAULT_ICON_HEIGHT)
	, EventIconMargin(1)
	, FeaturedMarkMargin(0)
	, HighlightBorder(3)
	, SelectedBorder(2)
	, TimeBarPadding(4)
	, TimeBarShadowWidth(6)
	, CurTimeLineWidth(2)
	, ToolbarItemPadding(4)
{
}


void CProgramGuide::ProgramGuideStyle::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	pStyleManager->Get(TEXT("program-guide.column.margin"),&ColumnMargin);
	pStyleManager->Get(TEXT("program-guide.header.padding"),&HeaderPadding);
	pStyleManager->Get(TEXT("program-guide.header.channel-name.margin"),&HeaderChannelNameMargin);
	pStyleManager->Get(TEXT("program-guide.header.icon.margin"),&HeaderIconMargin);
	pStyleManager->Get(TEXT("program-guide.header.chevron"),&HeaderChevronSize);
	pStyleManager->Get(TEXT("program-guide.header.chevron.margin"),&HeaderChevronMargin);
	pStyleManager->Get(TEXT("program-guide.header.shadow.height"),&HeaderShadowHeight);
	pStyleManager->Get(TEXT("program-guide.event.leading"),&EventLeading);
	pStyleManager->Get(TEXT("program-guide.event.line-spacing"),&EventLineSpacing);
	pStyleManager->Get(TEXT("program-guide.event.padding"),&EventPadding);
	pStyleManager->Get(TEXT("program-guide.event.icon"),&EventIconSize);
	pStyleManager->Get(TEXT("program-guide.event.icon.margin"),&EventIconMargin);
	pStyleManager->Get(TEXT("program-guide.event.featured-mark.margin"),&FeaturedMarkMargin);
	pStyleManager->Get(TEXT("program-guide.event.highlight-border"),&HighlightBorder);
	pStyleManager->Get(TEXT("program-guide.event.selected-border"),&SelectedBorder);
	pStyleManager->Get(TEXT("program-guide.time-bar.padding"),&TimeBarPadding);
	pStyleManager->Get(TEXT("program-guide.time-bar.shadow.width"),&TimeBarShadowWidth);
	pStyleManager->Get(TEXT("program-guide.cur-time-line.width"),&CurTimeLineWidth);
	pStyleManager->Get(TEXT("program-guide.tool-bar.item.padding"),&ToolbarItemPadding);
}


void CProgramGuide::ProgramGuideStyle::NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	pStyleManager->ToPixels(&ColumnMargin);
	pStyleManager->ToPixels(&HeaderPadding);
	pStyleManager->ToPixels(&HeaderChannelNameMargin);
	pStyleManager->ToPixels(&HeaderIconMargin);
	pStyleManager->ToPixels(&HeaderChevronSize);
	pStyleManager->ToPixels(&HeaderChevronMargin);
	pStyleManager->ToPixels(&HeaderShadowHeight);
	pStyleManager->ToPixels(&EventLeading);
	pStyleManager->ToPixels(&EventLineSpacing);
	pStyleManager->ToPixels(&EventPadding);
	pStyleManager->ToPixels(&EventIconSize);
	pStyleManager->ToPixels(&EventIconMargin);
	pStyleManager->ToPixels(&FeaturedMarkMargin);
	pStyleManager->ToPixels(&HighlightBorder);
	pStyleManager->ToPixels(&SelectedBorder);
	pStyleManager->ToPixels(&TimeBarPadding);
	pStyleManager->ToPixels(&TimeBarShadowWidth);
	pStyleManager->ToPixels(&CurTimeLineWidth);
	pStyleManager->ToPixels(&ToolbarItemPadding);
}




#include "Menu.h"

namespace ProgramGuideBar
{

enum {
	STATUS_ITEM_TUNER,
	STATUS_ITEM_DATE,
	STATUS_ITEM_DATEPREV,
	STATUS_ITEM_DATENEXT
};


class CProgramGuideTunerStatusItem : public CStatusItem
{
	CProgramGuide *m_pProgramGuide;
	CDropDownMenu m_Menu;

public:
	CProgramGuideTunerStatusItem::CProgramGuideTunerStatusItem(CProgramGuide *pProgramGuide)
		: CStatusItem(STATUS_ITEM_TUNER,SizeValue(14*EM_FACTOR,SIZE_EM))
		, m_pProgramGuide(pProgramGuide)
	{
	}

	LPCTSTR GetIDText() const override { return TEXT("Tuner"); }
	LPCTSTR GetName() const override { return TEXT("チューナー"); }

	void Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags) override
	{
		TCHAR szText[256];

		if (m_pProgramGuide->GetChannelGroupName(m_pProgramGuide->GetCurrentChannelGroup(),szText,lengthof(szText)))
			DrawText(hdc,DrawRect,szText);
	}

	void OnFocus(bool fFocus) override
	{
		if (fFocus) {
			TCHAR szText[256];

			m_Menu.Clear();
			for (int i=0;i<MAX_CHANNEL_GROUP_MENU_ITEMS
				 	&& m_pProgramGuide->GetChannelGroupName(i,szText,lengthof(szText));i++)
				m_Menu.AppendItem(new CDropDownMenu::CItem(CM_PROGRAMGUIDE_CHANNELGROUP_FIRST+i,szText));
			m_Menu.AppendSeparator();
			for (int i=0;i<MAX_CHANNEL_PROVIDER_MENU_ITEMS
				 	&& m_pProgramGuide->EnumChannelProvider(i,szText,lengthof(szText));i++)
				m_Menu.AppendItem(new CDropDownMenu::CItem(CM_PROGRAMGUIDE_CHANNELPROVIDER_FIRST+i,szText));
			RECT rc;
			POINT pt;
			GetRect(&rc);
			pt.x=rc.left;
			pt.y=rc.bottom;
			::ClientToScreen(m_pStatus->GetHandle(),&pt);
			m_Menu.Show(GetParent(m_pStatus->GetHandle()),m_pProgramGuide->GetHandle(),&pt,
						CM_PROGRAMGUIDE_CHANNELGROUP_FIRST+m_pProgramGuide->GetCurrentChannelGroup());
		} else {
			POINT pt;
			RECT rc;

			::GetCursorPos(&pt);
			if (!m_Menu.GetPosition(&rc) || !::PtInRect(&rc,pt))
				m_Menu.Hide();
		}
	}
};

class CListSelectStatusItem : public CStatusItem
{
	CProgramGuide *m_pProgramGuide;
	CDropDownMenu m_Menu;

	class CServiceMenuItem : public CDropDownMenu::CItem
	{
		static const int m_LogoWidth=16;
		static const int m_LogoHeight=9;
		static const int m_LogoMargin=2;

		HBITMAP m_hbmLogo;

		int GetWidth(HDC hdc) override
		{
			if (m_Width==0) {
				m_Width=CItem::GetWidth(hdc)+(m_LogoWidth+m_LogoMargin);
			}
			return m_Width;
		}

		void Draw(HDC hdc,const RECT *pRect) override
		{
			RECT rc=*pRect;

			if (m_hbmLogo!=NULL) {
				DrawUtil::DrawBitmap(hdc,
									 rc.left,rc.top+(rc.bottom-rc.top-m_LogoHeight)/2,
									 m_LogoWidth,m_LogoHeight,m_hbmLogo);
			}
			rc.left+=m_LogoWidth+m_LogoMargin;
			CItem::Draw(hdc,&rc);
		}

	public:
		CServiceMenuItem(int Command,LPCTSTR pszText,HBITMAP hbmLogo)
			: CItem(Command,pszText)
			, m_hbmLogo(hbmLogo)
		{
		}
	};

public:
	CListSelectStatusItem::CListSelectStatusItem(CProgramGuide *pProgramGuide)
		: CStatusItem(STATUS_ITEM_DATE,SizeValue(14*EM_FACTOR,SIZE_EM))
		, m_pProgramGuide(pProgramGuide)
	{
	}

	LPCTSTR GetIDText() const override { return TEXT("Date"); }
	LPCTSTR GetName() const override { return TEXT("日時"); }

	void Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags) override
	{
		if (m_pProgramGuide->GetListMode()==CProgramGuide::LIST_SERVICES) {
			CProgramGuide::DateInfo Info;
			TCHAR szText[256];

			m_pProgramGuide->GetCurrentDateInfo(&Info);
			EpgUtil::EpgTimeToDisplayTime(&Info.BeginningTime);
			StdUtil::snprintf(szText,lengthof(szText),TEXT("%s%s%d/%d(%s) %d時～"),
							  Info.pszRelativeDayText!=NULL?Info.pszRelativeDayText:TEXT(""),
							  Info.pszRelativeDayText!=NULL?TEXT(" "):TEXT(""),
							  Info.BeginningTime.wMonth,Info.BeginningTime.wDay,
							  GetDayOfWeekText(Info.BeginningTime.wDayOfWeek),
							  Info.BeginningTime.wHour);
			DrawText(hdc,DrawRect,szText);
		} else {
			const ProgramGuide::CServiceInfo *pService=
				m_pProgramGuide->GetServiceList().GetItem(m_pProgramGuide->GetWeekListService());
			if (pService!=NULL)
				DrawText(hdc,DrawRect,pService->GetServiceName());
		}
	}

	void OnFocus(bool fFocus) override
	{
		if (fFocus) {
			int CurItem;

			m_Menu.Clear();
			if (m_pProgramGuide->GetListMode()==CProgramGuide::LIST_SERVICES) {
				for (int i=CProgramGuide::DAY_FIRST;i<=CProgramGuide::DAY_LAST;i++) {
					CProgramGuide::DateInfo Info;
					TCHAR szText[256];

					m_pProgramGuide->GetDateInfo(i,&Info);
					EpgUtil::EpgTimeToDisplayTime(&Info.BeginningTime);
					StdUtil::snprintf(szText,lengthof(szText),TEXT("%s%s%d/%d(%s) %d時～"),
									  Info.pszRelativeDayText!=NULL?Info.pszRelativeDayText:TEXT(""),
									  Info.pszRelativeDayText!=NULL?TEXT(" "):TEXT(""),
									  Info.BeginningTime.wMonth,
									  Info.BeginningTime.wDay,
									  GetDayOfWeekText(Info.BeginningTime.wDayOfWeek),
									  Info.BeginningTime.wHour);
					m_Menu.AppendItem(new CDropDownMenu::CItem(CM_PROGRAMGUIDE_DAY_FIRST+i,szText));
				}
				CurItem=CM_PROGRAMGUIDE_DAY_FIRST+m_pProgramGuide->GetViewDay();
			} else {
				const ProgramGuide::CServiceList &ServiceList=m_pProgramGuide->GetServiceList();

				for (size_t i=0;i<ServiceList.NumServices();i++) {
					const ProgramGuide::CServiceInfo *pService=ServiceList.GetItem(i);

					m_Menu.AppendItem(
						new CServiceMenuItem(CM_CHANNEL_FIRST+(int)i,
											 pService->GetServiceName(),
											 pService->GetLogo()));
				}
				CurItem=CM_CHANNEL_FIRST+m_pProgramGuide->GetWeekListService();
			}
			RECT rc;
			POINT pt;
			GetRect(&rc);
			pt.x=rc.left;
			pt.y=rc.bottom;
			::ClientToScreen(m_pStatus->GetHandle(),&pt);
			m_Menu.Show(GetParent(m_pStatus->GetHandle()),m_pProgramGuide->GetHandle(),&pt,
						CurItem);
		} else {
			POINT pt;
			RECT rc;

			::GetCursorPos(&pt);
			if (!m_Menu.GetPosition(&rc) || !::PtInRect(&rc,pt))
				m_Menu.Hide();
		}
	}
};

class CListPrevStatusItem : public CStatusItem
{
	CProgramGuide *m_pProgramGuide;

public:
	CListPrevStatusItem::CListPrevStatusItem(CProgramGuide *pProgramGuide)
		: CStatusItem(STATUS_ITEM_DATEPREV,SizeValue(16,SIZE_PIXEL))
		, m_pProgramGuide(pProgramGuide)
	{
	}

	LPCTSTR GetIDText() const override { return TEXT("Prev"); }
	LPCTSTR GetName() const override { return TEXT("前へ"); }

	void Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags) override
	{
		bool fEnabled;

		if (m_pProgramGuide->GetListMode()==CProgramGuide::LIST_SERVICES)
			fEnabled=m_pProgramGuide->GetViewDay()>CProgramGuide::DAY_FIRST;
		else
			fEnabled=m_pProgramGuide->GetWeekListService()>0;

		COLORREF OldTextColor;
		if (!fEnabled)
			OldTextColor=::SetTextColor(hdc,MixColor(::GetTextColor(hdc),GetBkColor(hdc)));
		DrawText(hdc,DrawRect,TEXT("▲"),DRAWTEXT_HCENTER);
		if (!fEnabled)
			::SetTextColor(hdc,OldTextColor);
	}

	void OnLButtonDown(int x,int y) override
	{
		if (m_pProgramGuide->GetListMode()==CProgramGuide::LIST_SERVICES) {
			int Day=m_pProgramGuide->GetViewDay();
			if (Day>CProgramGuide::DAY_FIRST)
				m_pProgramGuide->SendMessage(WM_COMMAND,CM_PROGRAMGUIDE_DAY_FIRST+Day-1,0);
		} else {
			m_pProgramGuide->SetWeekListMode(m_pProgramGuide->GetWeekListService()-1);
		}
	}
};

class CListNextStatusItem : public CStatusItem
{
	CProgramGuide *m_pProgramGuide;

public:
	CListNextStatusItem::CListNextStatusItem(CProgramGuide *pProgramGuide)
		: CStatusItem(STATUS_ITEM_DATENEXT,SizeValue(16,SIZE_PIXEL))
		, m_pProgramGuide(pProgramGuide)
	{
	}

	LPCTSTR GetIDText() const override { return TEXT("Next"); }
	LPCTSTR GetName() const override { return TEXT("次へ"); }

	void Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags) override
	{
		bool fEnabled;

		if (m_pProgramGuide->GetListMode()==CProgramGuide::LIST_SERVICES)
			fEnabled=m_pProgramGuide->GetViewDay()<CProgramGuide::DAY_LAST;
		else
			fEnabled=m_pProgramGuide->GetWeekListService()+1<
				(int)m_pProgramGuide->GetServiceList().NumServices();

		COLORREF OldTextColor;
		if (!fEnabled)
			OldTextColor=::SetTextColor(hdc,MixColor(::GetTextColor(hdc),GetBkColor(hdc)));
		DrawText(hdc,DrawRect,TEXT("▼"),DRAWTEXT_HCENTER);
		if (!fEnabled)
			::SetTextColor(hdc,OldTextColor);
	}

	void OnLButtonDown(int x,int y) override
	{
		if (m_pProgramGuide->GetListMode()==CProgramGuide::LIST_SERVICES) {
			int Day=m_pProgramGuide->GetViewDay();
			if (Day<CProgramGuide::DAY_LAST)
				m_pProgramGuide->SendMessage(WM_COMMAND,CM_PROGRAMGUIDE_DAY_FIRST+Day+1,0);
		} else {
			m_pProgramGuide->SetWeekListMode(m_pProgramGuide->GetWeekListService()+1);
		}
	}
};




CProgramGuideBar::CProgramGuideBar(CProgramGuide *pProgramGuide)
	: m_pProgramGuide(pProgramGuide)
	, m_fVisible(true)
	, m_fUseBufferedPaint(false)
{
}


CProgramGuideBar::~CProgramGuideBar()
{
}




class ABSTRACT_CLASS(CProgramGuideToolbar)
	: public CProgramGuideBar
	, public CBasicWindow
{
public:
	CProgramGuideToolbar(CProgramGuide *pProgramGuide);
	virtual ~CProgramGuideToolbar();

// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;

// CProgramGuideBar
	bool CreateBar(HWND hwndParent,DWORD Style) override;
	bool IsBarCreated() const override;
	void DestroyBar() override;
	bool SetBarVisible(bool fVisible) override;
	void GetBarSize(SIZE *pSize) override;
	void SetBarPosition(int x,int y,int Width,int Height) override;
	bool OnSetCursor(HWND hwnd,int HitTestCode) override;
	bool OnNotify(LPARAM lParam,LRESULT *pResult) override;

// CProgramGuideToolbar
	void SelectButton(int Button);
	void UnselectButton();

protected:
	CBufferedPaint m_BufferedPaint;
	TVTest::Style::Margins m_Padding;

	void AdjustSize();
	void DeleteAllButtons();

	virtual void OnCreate() {}
	virtual void OnCustomDraw(NMTBCUSTOMDRAW *pnmtb,HDC hdc) = 0;
};


CProgramGuideToolbar::CProgramGuideToolbar(CProgramGuide *pProgramGuide)
	: CProgramGuideBar(pProgramGuide)
	, m_Padding(pProgramGuide->GetToolbarItemPadding())
{
}


CProgramGuideToolbar::~CProgramGuideToolbar()
{
	Destroy();
}


bool CProgramGuideToolbar::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	if (!CreateBasicWindow(hwndParent,
						   Style | TBSTYLE_CUSTOMERASE | TBSTYLE_LIST | TBSTYLE_FLAT
						   | CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN,
						   ExStyle,ID,
						   TOOLBARCLASSNAME,NULL,GetAppClass().GetInstance()))
		return false;

	::SendMessage(m_hwnd,TB_BUTTONSTRUCTSIZE,sizeof(TBBUTTON),0);
	//::SendMessage(m_hwnd,TB_SETEXTENDEDSTYLE,0,TBSTYLE_EX_MIXEDBUTTONS);
	::SendMessage(m_hwnd,TB_SETPADDING,0,
				  MAKELONG((m_Padding.Horz()+1)/2,(m_Padding.Vert()+1)/2));
	// ボタンをアイコン無しにしてもなぜかアイコン分の幅がとられる
	::SendMessage(m_hwnd,TB_SETBITMAPSIZE,0,MAKELONG(1,15));

	return true;
}


bool CProgramGuideToolbar::CreateBar(HWND hwndParent,DWORD Style)
{
	if (!Create(hwndParent,Style))
		return false;

	OnCreate();

	return true;
}


bool CProgramGuideToolbar::IsBarCreated() const
{
	return IsCreated();
}


void CProgramGuideToolbar::DestroyBar()
{
	Destroy();
}


bool CProgramGuideToolbar::SetBarVisible(bool fVisible)
{
	if (m_fVisible!=fVisible) {
		if (IsCreated()) {
			SetVisible(fVisible);
		}
		m_fVisible=fVisible;
	}

	return true;
}


void CProgramGuideToolbar::GetBarSize(SIZE *pSize)
{
	int ButtonCount;

	if (m_hwnd!=NULL && (ButtonCount=(int)::SendMessage(m_hwnd,TB_BUTTONCOUNT,0,0))>0) {
#if 0
		TBBUTTON tbb;
		RECT rc;

		::SendMessage(m_hwnd,TB_GETBUTTON,ButtonCount-1,reinterpret_cast<LPARAM>(&tbb));
		::SendMessage(m_hwnd,TB_GETRECT,tbb.idCommand,reinterpret_cast<LPARAM>(&rc));
		pSize->cx=rc.right;
		pSize->cy=rc.bottom;
#else
		::SendMessage(m_hwnd,TB_GETMAXSIZE,0,reinterpret_cast<LPARAM>(pSize));
#endif
	} else {
		pSize->cx=0;
		pSize->cy=0;
	}
}


void CProgramGuideToolbar::SetBarPosition(int x,int y,int Width,int Height)
{
	SetPosition(x,y,Width,Height);
}


bool CProgramGuideToolbar::OnSetCursor(HWND hwnd,int HitTestCode)
{
	if (hwnd==m_hwnd && HitTestCode==HTCLIENT) {
		::SetCursor(GetAppClass().UICore.GetActionCursor());
		return true;
	}

	return false;
}


bool CProgramGuideToolbar::OnNotify(LPARAM lParam,LRESULT *pResult)
{
	NMTBCUSTOMDRAW *pnmtb=reinterpret_cast<NMTBCUSTOMDRAW*>(lParam);

	if (pnmtb->nmcd.hdr.hwndFrom!=m_hwnd
			|| pnmtb->nmcd.hdr.code!=NM_CUSTOMDRAW)
		return false;

	switch (pnmtb->nmcd.dwDrawStage) {
	case CDDS_PREERASE:
		*pResult=CDRF_SKIPDEFAULT;
		break;

	case CDDS_PREPAINT:
		*pResult=CDRF_NOTIFYITEMDRAW;
		break;

	case CDDS_ITEMPREPAINT:
		{
			HDC hdcBuffer=NULL,hdc;

			hdc=pnmtb->nmcd.hdc;
			if (m_fUseBufferedPaint) {
				hdcBuffer=m_BufferedPaint.Begin(pnmtb->nmcd.hdc,&pnmtb->nmcd.rc);
				if (hdcBuffer!=NULL)
					hdc=hdcBuffer;
			}

			OnCustomDraw(pnmtb,hdc);

			if (hdcBuffer!=NULL) {
				//m_BufferedPaint.SetAlpha(224);
				m_BufferedPaint.SetOpaque();
				m_BufferedPaint.End();
			}
		}
		*pResult=CDRF_SKIPDEFAULT;
		break;

	default:
		*pResult=CDRF_DODEFAULT;
		break;
	}

	return true;
}


void CProgramGuideToolbar::SelectButton(int Button)
{
	if (m_hwnd!=NULL)
		::SendMessage(m_hwnd,TB_CHECKBUTTON,Button,TRUE);
}


void CProgramGuideToolbar::UnselectButton()
{
	if (m_hwnd!=NULL) {
		const int ButtonCount=(int)::SendMessage(m_hwnd,TB_BUTTONCOUNT,0,0);

		for (int i=0;i<ButtonCount;i++) {
			TBBUTTON tbb;

			::SendMessage(m_hwnd,TB_GETBUTTON,i,reinterpret_cast<LPARAM>(&tbb));
			::SendMessage(m_hwnd,TB_CHECKBUTTON,tbb.idCommand,FALSE);
		}
	}
}


void CProgramGuideToolbar::AdjustSize()
{
	RECT rcToolbar;
	GetPosition(&rcToolbar);
	SIZE sz;
	::SendMessage(m_hwnd,TB_GETMAXSIZE,0,reinterpret_cast<LPARAM>(&sz));
	SetPosition(rcToolbar.left,rcToolbar.top,sz.cx,sz.cy);
}


void CProgramGuideToolbar::DeleteAllButtons()
{
	if (m_hwnd!=NULL) {
		int ButtonCount=(int)::SendMessage(m_hwnd,TB_BUTTONCOUNT,0,0);

		for (int i=ButtonCount-1;i>=0;i--)
			::SendMessage(m_hwnd,TB_DELETEBUTTON,i,0);
	}
}




class ABSTRACT_CLASS(CStatusBar) : public CProgramGuideBar
{
public:
	CStatusBar(CProgramGuide *pProgramGuide);
	virtual ~CStatusBar();

// CProgramGuideBar
	bool CreateBar(HWND hwndParent,DWORD Style) override;
	bool IsBarCreated() const override;
	void DestroyBar() override;
	bool SetBarVisible(bool fVisible) override;
	void GetBarSize(SIZE *pSize) override;
	void SetBarPosition(int x,int y,int Width,int Height) override;
	void SetTheme(const ThemeInfo &Theme) override;

protected:
	CStatusView m_StatusView;
};


CStatusBar::CStatusBar(CProgramGuide *pProgramGuide)
	: CProgramGuideBar(pProgramGuide)
{
	LOGFONT lf;
	if (DrawUtil::GetSystemFont(DrawUtil::FONT_MENU,&lf))
		m_StatusView.SetFont(&lf);
}


CStatusBar::~CStatusBar()
{
}


bool CStatusBar::CreateBar(HWND hwndParent,DWORD Style)
{
	m_StatusView.EnableBufferedPaint(m_fUseBufferedPaint);
	return m_StatusView.Create(hwndParent,Style);
}


bool CStatusBar::IsBarCreated() const
{
	return m_StatusView.IsCreated();
}


void CStatusBar::DestroyBar()
{
	m_StatusView.Destroy();
}


bool CStatusBar::SetBarVisible(bool fVisible)
{
	if (m_fVisible!=fVisible) {
		if (m_StatusView.IsCreated())
			m_StatusView.SetVisible(fVisible);
		m_fVisible=fVisible;
	}

	return true;
}


void CStatusBar::GetBarSize(SIZE *pSize)
{
	pSize->cx=m_StatusView.GetIntegralWidth();
	pSize->cy=m_StatusView.GetHeight();
}


void CStatusBar::SetBarPosition(int x,int y,int Width,int Height)
{
	m_StatusView.SetPosition(x,y,Width,Height);
}


void CStatusBar::SetTheme(const ThemeInfo &Theme)
{
	m_StatusView.SetStatusViewTheme(Theme.StatusTheme);
}




class CTunerMenuBar : public CStatusBar
{
public:
	CTunerMenuBar(CProgramGuide *pProgramGuide);
	~CTunerMenuBar();
	void OnSpaceChanged() override;
};


CTunerMenuBar::CTunerMenuBar(CProgramGuide *pProgramGuide)
	: CStatusBar(pProgramGuide)
{
	m_StatusView.AddItem(new CProgramGuideTunerStatusItem(pProgramGuide));
}


CTunerMenuBar::~CTunerMenuBar()
{
}


void CTunerMenuBar::OnSpaceChanged()
{
	m_StatusView.UpdateItem(STATUS_ITEM_TUNER);
}




class CDateMenuBar : public CStatusBar
{
public:
	CDateMenuBar(CProgramGuide *pProgramGuide);
	~CDateMenuBar();
	void OnDateChanged() override;
};


CDateMenuBar::CDateMenuBar(CProgramGuide *pProgramGuide)
	: CStatusBar(pProgramGuide)
{
	m_StatusView.AddItem(new CListSelectStatusItem(pProgramGuide));
	m_StatusView.AddItem(new CListPrevStatusItem(pProgramGuide));
	m_StatusView.AddItem(new CListNextStatusItem(pProgramGuide));
}


CDateMenuBar::~CDateMenuBar()
{
}


void CDateMenuBar::OnDateChanged()
{
	m_StatusView.UpdateItem(STATUS_ITEM_DATE);
	m_StatusView.UpdateItem(STATUS_ITEM_DATEPREV);
	m_StatusView.UpdateItem(STATUS_ITEM_DATENEXT);
}




class CFavoritesToolbar : public CProgramGuideToolbar
{
public:
	CFavoritesToolbar(CProgramGuide *pProgramGuide);
	~CFavoritesToolbar();
	void OnSpaceChanged() override;
	void OnFavoritesChanged() override;

private:
	void SetButtons();
	void OnCreate() override;
	void OnCustomDraw(NMTBCUSTOMDRAW *pnmtb,HDC hdc) override;
};


CFavoritesToolbar::CFavoritesToolbar(CProgramGuide *pProgramGuide)
	: CProgramGuideToolbar(pProgramGuide)
{
}


CFavoritesToolbar::~CFavoritesToolbar()
{
	Destroy();
}


void CFavoritesToolbar::OnSpaceChanged()
{
	const int CurChannelProvider=m_pProgramGuide->GetCurrentChannelProvider();
	TCHAR szName[256];
	int SelButton=0;

	if (CurChannelProvider>=0
			&& m_pProgramGuide->EnumChannelProvider(CurChannelProvider,szName,lengthof(szName))) {
		const int CurChannelGroup=m_pProgramGuide->GetCurrentChannelGroup();

		if (CurChannelGroup>=0) {
			const CProgramGuideFavorites *pFavorites=m_pProgramGuide->GetFavorites();
			const int ButtonCount=(int)::SendMessage(m_hwnd,TB_BUTTONCOUNT,0,0);

			for (int i=0;i<ButtonCount;i++) {
				TBBUTTON tbb;

				if (::SendMessage(m_hwnd,TB_GETBUTTON,i,reinterpret_cast<LPARAM>(&tbb))) {
					const CProgramGuideFavorites::FavoriteInfo *pInfo=pFavorites->Get(tbb.dwData);

					if (pInfo!=NULL
							&& ::lstrcmpi(pInfo->Name.c_str(),szName)==0
							&& m_pProgramGuide->ParseChannelGroupID(pInfo->GroupID.c_str())==CurChannelGroup) {
						SelButton=tbb.idCommand;
						break;
					}
				}
			}
		}
	}

	if (SelButton>0)
		SelectButton(SelButton);
	else
		UnselectButton();
}


void CFavoritesToolbar::OnFavoritesChanged()
{
	DeleteAllButtons();
	SetButtons();
	OnSpaceChanged();
}


void CFavoritesToolbar::SetButtons()
{
	const CProgramGuideFavorites *pFavorites=m_pProgramGuide->GetFavorites();
	const size_t Count=pFavorites->GetCount();

	TBBUTTON tbb;
	tbb.iBitmap=I_IMAGENONE;
	tbb.fsState=TBSTATE_ENABLED;
	tbb.fsStyle=BTNS_CHECKGROUP | BTNS_SHOWTEXT | BTNS_NOPREFIX;
	if (!pFavorites->GetFixedWidth())
		tbb.fsStyle|=BTNS_AUTOSIZE;
	tbb.idCommand=CM_PROGRAMGUIDE_FAVORITE_FIRST;

	for (size_t i=0;i<Count && tbb.idCommand<=CM_PROGRAMGUIDE_FAVORITE_LAST;i++) {
		const CProgramGuideFavorites::FavoriteInfo *pInfo=pFavorites->Get(i);

		tbb.iString=reinterpret_cast<INT_PTR>(pInfo->Label.c_str());
		tbb.dwData=i;
		::SendMessage(m_hwnd,TB_ADDBUTTONS,1,reinterpret_cast<LPARAM>(&tbb));
		tbb.idCommand++;
	}

	AdjustSize();
}


void CFavoritesToolbar::OnCreate()
{
	SetButtons();
}


void CFavoritesToolbar::OnCustomDraw(NMTBCUSTOMDRAW *pnmtb,HDC hdc)
{
	const CProgramGuideFavorites::FavoriteInfo *pInfo=
		m_pProgramGuide->GetFavorites()->Get(pnmtb->nmcd.lItemlParam);

	if (pInfo!=NULL) {
		TVTest::Theme::BackgroundStyle Style;
		COLORREF LightColor=pInfo->BackColor;
		COLORREF DarkColor=MixColor(LightColor,RGB(0,0,0),220);

		Style.Fill.Type=TVTest::Theme::FILL_GRADIENT;
		Style.Fill.Gradient.Type=TVTest::Theme::GRADIENT_NORMAL;
		Style.Fill.Gradient.Direction=TVTest::Theme::DIRECTION_VERT;
		if ((pnmtb->nmcd.uItemState&(CDIS_CHECKED | CDIS_HOT))!=0) {
			Style.Fill.Gradient.Color1=DarkColor;
			Style.Fill.Gradient.Color2=LightColor;
		} else {
			Style.Fill.Gradient.Color1=LightColor;
			Style.Fill.Gradient.Color2=DarkColor;
		}
		if ((pnmtb->nmcd.uItemState&(CDIS_CHECKED | CDIS_SELECTED))!=0) {
			Style.Border.Type=TVTest::Theme::BORDER_SUNKEN;
			Style.Border.Color=DarkColor;
		} else {
			Style.Border.Type=TVTest::Theme::BORDER_RAISED;
			Style.Border.Color=LightColor;
		}
		TVTest::Theme::Draw(hdc,pnmtb->nmcd.rc,Style);

		HFONT hfont=reinterpret_cast<HFONT>(::SendMessage(m_hwnd,WM_GETFONT,0,0));
		HGDIOBJ hOldFont=::SelectObject(hdc,hfont);
		int OldBkMode=::SetBkMode(hdc,TRANSPARENT);
		COLORREF OldTextColor=::SetTextColor(hdc,pInfo->TextColor);

		RECT rc=pnmtb->nmcd.rc;
		TVTest::Style::Subtract(&rc,m_Padding);
		::DrawText(hdc,pInfo->Label.c_str(),-1,&rc,
				   DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

		::SetTextColor(hdc,OldTextColor);
		::SetBkMode(hdc,OldBkMode);
		::SelectObject(hdc,hOldFont);
	}
}




class CDateToolbar : public CProgramGuideToolbar
{
public:
	enum {
		MAX_BUTTON_COUNT=CProgramGuideFrameSettings::DATEBAR_MAXBUTTONCOUNT
	};

	CDateToolbar(CProgramGuide *pProgramGuide);
	~CDateToolbar();
	void OnDateChanged() override;
	void OnTimeRangeChanged() override;
	bool SetButtonCount(int Count);
	int GetButtonCount() const { return m_ButtonCount; }

private:
	static const DWORD ITEM_FLAG_NOW=0x80000000;

	int m_ButtonCount;

	bool SetButtons(const SYSTEMTIME *pDateList,int Days,int FirstCommand);
	void OnCustomDraw(NMTBCUSTOMDRAW *pnmtb,HDC hdc) override;
};


CDateToolbar::CDateToolbar(CProgramGuide *pProgramGuide)
	: CProgramGuideToolbar(pProgramGuide)
	, m_ButtonCount(CProgramGuideFrameSettings::DATEBAR_DEFAULTBUTTONCOUNT)
{
}


CDateToolbar::~CDateToolbar()
{
	Destroy();
}


void CDateToolbar::OnDateChanged()
{
	UnselectButton();
	if (m_pProgramGuide->GetListMode()==CProgramGuide::LIST_SERVICES)
		SelectButton(CM_PROGRAMGUIDE_DAY_FIRST+m_pProgramGuide->GetViewDay());
}


void CDateToolbar::OnTimeRangeChanged()
{
	SYSTEMTIME DateList[MAX_BUTTON_COUNT];

	for (int i=0;i<m_ButtonCount;i++) {
		SYSTEMTIME st;
		m_pProgramGuide->GetDayTimeRange(i,&st,NULL);
		EpgUtil::EpgTimeToDisplayTime(st,&DateList[i]);
	}
	SetButtons(DateList,m_ButtonCount,CM_PROGRAMGUIDE_DAY_FIRST);
	SelectButton(CM_PROGRAMGUIDE_DAY_FIRST+m_pProgramGuide->GetViewDay());
}


bool CDateToolbar::SetButtonCount(int Count)
{
	if (Count<1 || Count>MAX_BUTTON_COUNT)
		return false;

	m_ButtonCount=Count;

	return true;
}


bool CDateToolbar::SetButtons(const SYSTEMTIME *pDateList,int Days,int FirstCommand)
{
	if (m_hwnd==NULL)
		return false;

	DeleteAllButtons();

	TBBUTTON tbb;
	tbb.iBitmap=I_IMAGENONE;
	tbb.fsState=TBSTATE_ENABLED;
	tbb.fsStyle=BTNS_CHECKGROUP | BTNS_SHOWTEXT | BTNS_NOPREFIX;
	for (int i=0;i<Days;i++) {
		const SYSTEMTIME &Date=pDateList[i];
		TCHAR szText[32];

		tbb.idCommand=FirstCommand+i;
		StdUtil::snprintf(szText,lengthof(szText),
						  TEXT("%02d/%02d(%s)"),	// %02d にしているのは幅を揃えるため
						  Date.wMonth,Date.wDay,GetDayOfWeekText(Date.wDayOfWeek));
		tbb.iString=reinterpret_cast<INT_PTR>(szText);
		tbb.dwData=((DWORD)Date.wMonth<<16) | ((DWORD)Date.wDay<<8) | Date.wDayOfWeek;
		if (i==0 && Days>1 && Date.wDay==pDateList[i+1].wDay)
			tbb.dwData|=ITEM_FLAG_NOW;
		::SendMessage(m_hwnd,TB_ADDBUTTONS,1,reinterpret_cast<LPARAM>(&tbb));
	}

	AdjustSize();

	return true;
}


void CDateToolbar::OnCustomDraw(NMTBCUSTOMDRAW *pnmtb,HDC hdc)
{
	TVTest::Theme::BackgroundStyle Style;
	Style.Fill.Type=TVTest::Theme::FILL_GRADIENT;
	Style.Fill.Gradient.Type=TVTest::Theme::GRADIENT_NORMAL;
	Style.Fill.Gradient.Direction=TVTest::Theme::DIRECTION_VERT;
	if ((pnmtb->nmcd.uItemState&(CDIS_CHECKED | CDIS_HOT))!=0) {
		Style.Fill.Gradient.Color1.Set(220,220,220);
		Style.Fill.Gradient.Color2.Set(255,255,255);
	} else {
		Style.Fill.Gradient.Color1.Set(255,255,255);
		Style.Fill.Gradient.Color2.Set(220,220,220);
	}
	if ((pnmtb->nmcd.uItemState&CDIS_CHECKED)!=0) {
		Style.Border.Type=TVTest::Theme::BORDER_SUNKEN;
		Style.Border.Color.Set(220,220,220);
	} else {
		Style.Border.Type=TVTest::Theme::BORDER_RAISED;
		Style.Border.Color.Set(255,255,255);
	}
	TVTest::Theme::Draw(hdc,pnmtb->nmcd.rc,Style);

	int DayOfWeek=(int)(pnmtb->nmcd.lItemlParam&0xFF);

	HFONT hfont=reinterpret_cast<HFONT>(::SendMessage(m_hwnd,WM_GETFONT,0,0));
	HGDIOBJ hOldFont=::SelectObject(hdc,hfont);
	int OldBkMode=::SetBkMode(hdc,TRANSPARENT);
	COLORREF OldTextColor=
		::SetTextColor(hdc,DayOfWeek==0?RGB(255,32,0):DayOfWeek==6?RGB(0,32,255):RGB(0,0,0));

	TCHAR szText[32];
	if ((pnmtb->nmcd.lItemlParam&ITEM_FLAG_NOW)!=0) {
		::lstrcpy(szText,TEXT("今日"));
	} else {
		StdUtil::snprintf(szText,lengthof(szText),TEXT("%d/%d(%s)"),
						  (int)(pnmtb->nmcd.lItemlParam>>16),
						  (int)((pnmtb->nmcd.lItemlParam>>8)&0xFF),
						  GetDayOfWeekText(DayOfWeek));
	}
	RECT rc=pnmtb->nmcd.rc;
	TVTest::Style::Subtract(&rc,m_Padding);
	::DrawText(hdc,szText,-1,&rc,
			   DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	::SetTextColor(hdc,OldTextColor);
	::SetBkMode(hdc,OldBkMode);
	::SelectObject(hdc,hOldFont);
}




class CTimeToolbar : public CProgramGuideToolbar
{
public:
	typedef CProgramGuideFrameSettings::TimeBarSettings TimeBarSettings;

	CTimeToolbar(CProgramGuide *pProgramGuide);
	~CTimeToolbar();
	void SetTheme(const ThemeInfo &Theme) override;
	void OnDateChanged() override { ChangeTime(); }
	void OnTimeRangeChanged() override { ChangeTime(); }
	void SetSettings(const TimeBarSettings &Settings);
	bool GetTimeByCommand(int Command,SYSTEMTIME *pTime) const;

private:
	struct TimeInfo {
		WORD Hour;
		WORD Offset;
		int Command;
	};

	CProgramGuideFrameSettings::TimeBarSettings m_Settings;
	TVTest::Theme::Style m_ButtonStyle[CProgramGuide::TIME_BAR_BACK_COLORS];

	void ChangeTime();
	bool SetButtons(const TimeInfo *pTimeList,int TimeListLength);
	void OnCustomDraw(NMTBCUSTOMDRAW *pnmtb,HDC hdc) override;
};


CTimeToolbar::CTimeToolbar(CProgramGuide *pProgramGuide)
	: CProgramGuideToolbar(pProgramGuide)
{
}


CTimeToolbar::~CTimeToolbar()
{
	Destroy();
}


void CTimeToolbar::SetTheme(const ThemeInfo &Theme)
{
	for (int i=0;i<CProgramGuide::TIME_BAR_BACK_COLORS;i++)
		m_ButtonStyle[i]=Theme.TimeStyle[i];

	if (m_hwnd!=NULL)
		::InvalidateRect(m_hwnd,NULL,TRUE);
}


void CTimeToolbar::SetSettings(const TimeBarSettings &Settings)
{
	m_Settings=Settings;
}


bool CTimeToolbar::GetTimeByCommand(int Command,SYSTEMTIME *pTime) const
{
	TBBUTTONINFO tbbi;

	tbbi.cbSize=sizeof(tbbi);
	tbbi.dwMask=TBIF_LPARAM;
	if (::SendMessage(m_hwnd,TB_GETBUTTONINFO,
					  Command,reinterpret_cast<LPARAM>(&tbbi))<0)
		return false;

	if (!m_pProgramGuide->GetCurrentTimeRange(pTime,NULL))
		return false;

	OffsetSystemTime(pTime,HIWORD(tbbi.lParam)*TimeConsts::SYSTEMTIME_HOUR);

	return true;
}


void CTimeToolbar::ChangeTime()
{
	SYSTEMTIME stFirst,stLast;

	if (m_pProgramGuide->GetCurrentTimeRange(&stFirst,&stLast)) {
		TimeInfo TimeList[(CM_PROGRAMGUIDE_TIME_LAST-CM_PROGRAMGUIDE_TIME_FIRST)+2];
		TimeList[0].Hour=0;
		TimeList[0].Offset=0;
		TimeList[0].Command=CM_PROGRAMGUIDE_TIME_CURRENT;

		int i=1;

		if (m_Settings.Time==TimeBarSettings::TIME_INTERVAL) {
			SYSTEMTIME st=stFirst;

			for (;i<lengthof(TimeList) && i-1<m_Settings.MaxButtonCount
					&& CompareSystemTime(&st,&stLast)<0;i++) {
				SYSTEMTIME stDisp;

				EpgUtil::EpgTimeToDisplayTime(st,&stDisp);
				TimeList[i].Hour=stDisp.wHour;
				TimeList[i].Offset=(i-1)*m_Settings.Interval;
				TimeList[i].Command=CM_PROGRAMGUIDE_TIME_FIRST+(i-1);
				OffsetSystemTime(&st,m_Settings.Interval*TimeConsts::SYSTEMTIME_HOUR);
			}
		} else if (m_Settings.Time==TimeBarSettings::TIME_CUSTOM) {
			std::vector<TVTest::String> Times;
			TVTest::StringUtility::Split(m_Settings.CustomTime,_T(","),&Times);
			if (!Times.empty()) {
				std::vector<int> Hours;
				Hours.reserve(Times.size());
				for (auto itr=Times.begin();itr!=Times.end();++itr) {
					try {
						int Hour=std::stoi(*itr,nullptr,10);
						if (Hour>=0)
							Hours.push_back(Hour);
					} catch (...) {
					}
				}
				if (!Hours.empty()) {
					TsEngine::InsertionSort(Hours);
					SYSTEMTIME stDispFirst;
					EpgUtil::EpgTimeToDisplayTime(stFirst,&stDispFirst);
					const int FirstHour=stDispFirst.wHour;
					const int LastHour=FirstHour+(int)(DiffSystemTime(&stFirst,&stLast)/TimeConsts::SYSTEMTIME_HOUR);
					size_t j=0;
					for (;j<Hours.size();j++) {
						if (Hours[j]>=FirstHour)
							break;
					}
					for (size_t k=0;i<lengthof(TimeList) && i-1<m_Settings.MaxButtonCount && k<Hours.size();k++) {
						if (j==Hours.size())
							j=0;
						int Hour=Hours[j];
						j++;
						if (Hour>=LastHour)
							break;
						int HourOffset=Hour;
						if (HourOffset<FirstHour) {
							HourOffset+=24;
							if (HourOffset<FirstHour || HourOffset>=LastHour)
								break;
						}
						HourOffset-=FirstHour;
						SYSTEMTIME st=stDispFirst;
						OffsetSystemTime(&st,HourOffset*TimeConsts::SYSTEMTIME_HOUR);
						if (EpgUtil::DisplayTimeToEpgTime(&st)) {
							int Diff=(int)(DiffSystemTime(&stFirst,&st)/TimeConsts::SYSTEMTIME_HOUR);
							if (Diff>=0) {
								TimeList[i].Hour=(WORD)Hour;
								TimeList[i].Offset=(WORD)Diff;
								TimeList[i].Command=CM_PROGRAMGUIDE_TIME_FIRST+(i-1);
								i++;
							}
						}
					}
				}
			}
		}

		SetButtons(TimeList,i);
	}
}


bool CTimeToolbar::SetButtons(const TimeInfo *pTimeList,int TimeListLength)
{
	if (m_hwnd==NULL)
		return false;

	DeleteAllButtons();

	TBBUTTON tbb;
	tbb.iBitmap=I_IMAGENONE;
	tbb.fsState=TBSTATE_ENABLED;
	tbb.fsStyle=BTNS_BUTTON | BTNS_SHOWTEXT | BTNS_NOPREFIX;
	for (int i=0;i<TimeListLength;i++) {
		const TimeInfo &TimeInfo=pTimeList[i];
		TCHAR szText[32];

		tbb.idCommand=TimeInfo.Command;
		if (TimeInfo.Command==CM_PROGRAMGUIDE_TIME_CURRENT) {
			::lstrcpy(szText,TEXT(" 現在 "));
		} else {
			StdUtil::snprintf(szText,lengthof(szText),
							  TEXT("%02d時～"),TimeInfo.Hour);
		}
		tbb.iString=reinterpret_cast<INT_PTR>(szText);
		tbb.dwData=MAKELONG(TimeInfo.Hour,TimeInfo.Offset);
		::SendMessage(m_hwnd,TB_ADDBUTTONS,1,reinterpret_cast<LPARAM>(&tbb));
	}

	AdjustSize();

	return true;
}


void CTimeToolbar::OnCustomDraw(NMTBCUSTOMDRAW *pnmtb,HDC hdc)
{
	const bool fCurrent=pnmtb->nmcd.dwItemSpec==CM_PROGRAMGUIDE_TIME_CURRENT;
	TVTest::Theme::Style Style;
	int Hour;

	if (fCurrent) {
		Style.Back.Fill.Type=TVTest::Theme::FILL_GRADIENT;
		Style.Back.Fill.Gradient=TVTest::Theme::GradientStyle(
			TVTest::Theme::GRADIENT_NORMAL,
			TVTest::Theme::DIRECTION_VERT,
			TVTest::Theme::ThemeColor(255,255,255),
			TVTest::Theme::ThemeColor(220,220,220));
		Style.Fore.Fill.Type=TVTest::Theme::FILL_SOLID;
		Style.Fore.Fill.Solid.Color.Set(0,0,0);
	} else {
		Hour=LOWORD(pnmtb->nmcd.lItemlParam);
		Style=m_ButtonStyle[Hour/3];
		if (Style.Back.Fill.Type==TVTest::Theme::FILL_GRADIENT)
			Style.Back.Fill.Gradient.Rotate(TVTest::Theme::GradientStyle::ROTATE_RIGHT);
	}
	Style.Back.Border.Type=
		(pnmtb->nmcd.uItemState & (CDIS_CHECKED | CDIS_SELECTED))!=0?
			TVTest::Theme::BORDER_SUNKEN:
			TVTest::Theme::BORDER_RAISED;
	Style.Back.Border.Color=Style.Back.Fill.GetSolidColor();
	if ((pnmtb->nmcd.uItemState & (CDIS_CHECKED | CDIS_HOT))!=0
			&& Style.Back.Fill.Type==TVTest::Theme::FILL_GRADIENT) {
		std::swap(Style.Back.Fill.Gradient.Color1,
				  Style.Back.Fill.Gradient.Color2);
	}
	TVTest::Theme::Draw(hdc,pnmtb->nmcd.rc,Style.Back);

	HFONT hfont=reinterpret_cast<HFONT>(::SendMessage(m_hwnd,WM_GETFONT,0,0));
	HGDIOBJ hOldFont=::SelectObject(hdc,hfont);
	int OldBkMode=::SetBkMode(hdc,TRANSPARENT);

	TCHAR szText[32];
	if (fCurrent) {
		::lstrcpy(szText,TEXT("現在"));
	} else {
		StdUtil::snprintf(szText,lengthof(szText),TEXT("%d時～"),Hour);
	}
	RECT rc=pnmtb->nmcd.rc;
	TVTest::Style::Subtract(&rc,m_Padding);
	TVTest::Theme::Draw(hdc,rc,Style.Fore,szText,
						DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	::SetBkMode(hdc,OldBkMode);
	::SelectObject(hdc,hOldFont);
}


}	// namespace ProgramGuideBar




CProgramGuideFrameBase::CProgramGuideFrameBase(CProgramGuide *pProgramGuide,CProgramGuideFrameSettings *pSettings)
	: m_pProgramGuide(pProgramGuide)
	, m_pSettings(pSettings)
	, m_fNoUpdateLayout(false)
{
	m_ToolbarMargin.left=0;
	m_ToolbarMargin.top=0;
	m_ToolbarMargin.right=0;
	m_ToolbarMargin.bottom=5;
	m_ToolbarGap.x=6;
	m_ToolbarGap.y=5;

	m_ToolbarList[TOOLBAR_TUNER_MENU]=new ProgramGuideBar::CTunerMenuBar(pProgramGuide);
	m_ToolbarList[TOOLBAR_DATE_MENU ]=new ProgramGuideBar::CDateMenuBar(pProgramGuide);
	m_ToolbarList[TOOLBAR_FAVORITES ]=new ProgramGuideBar::CFavoritesToolbar(pProgramGuide);
	m_ToolbarList[TOOLBAR_DATE      ]=new ProgramGuideBar::CDateToolbar(pProgramGuide);
	m_ToolbarList[TOOLBAR_TIME      ]=new ProgramGuideBar::CTimeToolbar(pProgramGuide);
}


CProgramGuideFrameBase::~CProgramGuideFrameBase()
{
	for (int i=0;i<lengthof(m_ToolbarList);i++)
		delete m_ToolbarList[i];
}


void CProgramGuideFrameBase::SetTheme(const TVTest::Theme::CThemeManager *pThemeManager)
{
	ProgramGuideBar::CProgramGuideBar::ThemeInfo Theme;

	CStatusView::GetStatusViewThemeFromThemeManager(pThemeManager,&Theme.StatusTheme);
	pThemeManager->GetBorderStyle(TVTest::Theme::CThemeManager::STYLE_PROGRAMGUIDE_STATUS,
								  &Theme.StatusTheme.Border);

	TVTest::Theme::ThemeColor TimeTextColor(pThemeManager->GetColor(CColorScheme::COLOR_PROGRAMGUIDE_TIMETEXT));
	for (int i=0;i<CProgramGuide::TIME_BAR_BACK_COLORS;i++) {
		pThemeManager->GetFillStyle(
			TVTest::Theme::CThemeManager::STYLE_PROGRAMGUIDE_TIMEBAR_0_2+i,
			&Theme.TimeStyle[i].Back.Fill);
		Theme.TimeStyle[i].Fore.Fill.Type=TVTest::Theme::FILL_SOLID;
		Theme.TimeStyle[i].Fore.Fill.Solid.Color=TimeTextColor;
	}

	for (int i=0;i<lengthof(m_ToolbarList);i++)
		m_ToolbarList[i]->SetTheme(Theme);
}


bool CProgramGuideFrameBase::SetToolbarVisible(int Toolbar,bool fVisible)
{
	if (Toolbar<0 || Toolbar>=TOOLBAR_NUM)
		return false;

	ProgramGuideBar::CProgramGuideBar *pBar=m_ToolbarList[Toolbar];

	if (pBar->IsBarVisible()!=fVisible) {
		if (pBar->IsBarCreated()) {
			if (fVisible) {
				// 一瞬変な位置に表示されるのを防ぐために見えない位置に移動
				SIZE sz;
				pBar->GetBarSize(&sz);
				pBar->SetBarPosition(-sz.cx,-sz.cy,sz.cx,sz.cy);
			}
			pBar->SetBarVisible(fVisible);
			if (!m_fNoUpdateLayout)
				OnLayoutChange();
		} else {
			pBar->SetBarVisible(fVisible);
		}
	}

	m_pSettings->SetToolbarVisible(Toolbar,fVisible);

	return true;
}


bool CProgramGuideFrameBase::GetToolbarVisible(int Toolbar) const
{
	if (Toolbar<0 || Toolbar>=TOOLBAR_NUM)
		return false;

	return m_ToolbarList[Toolbar]->IsBarVisible();
}


void CProgramGuideFrameBase::OnDateChanged()
{
	for (int i=0;i<lengthof(m_ToolbarList);i++)
		m_ToolbarList[i]->OnDateChanged();
}


void CProgramGuideFrameBase::OnSpaceChanged()
{
	for (int i=0;i<lengthof(m_ToolbarList);i++)
		m_ToolbarList[i]->OnSpaceChanged();
}


void CProgramGuideFrameBase::OnListModeChanged()
{
	OnDateChanged();
}


void CProgramGuideFrameBase::OnTimeRangeChanged()
{
	for (int i=0;i<lengthof(m_ToolbarList);i++)
		m_ToolbarList[i]->OnTimeRangeChanged();

	OnLayoutChange();
}


void CProgramGuideFrameBase::OnFavoritesChanged()
{
	for (int i=0;i<lengthof(m_ToolbarList);i++)
		m_ToolbarList[i]->OnFavoritesChanged();

	OnLayoutChange();
}


bool CProgramGuideFrameBase::OnCommand(int Command)
{
	switch (Command) {
	case CM_PROGRAMGUIDE_TOOLBAR_TUNERMENU:
		SetToolbarVisible(TOOLBAR_TUNER_MENU,!m_ToolbarList[TOOLBAR_TUNER_MENU]->IsBarVisible());
		return true;

	case CM_PROGRAMGUIDE_TOOLBAR_DATEMENU:
		SetToolbarVisible(TOOLBAR_DATE_MENU,!m_ToolbarList[TOOLBAR_DATE_MENU]->IsBarVisible());
		return true;

	case CM_PROGRAMGUIDE_TOOLBAR_FAVORITES:
		SetToolbarVisible(TOOLBAR_FAVORITES,!m_ToolbarList[TOOLBAR_FAVORITES]->IsBarVisible());
		return true;

	case CM_PROGRAMGUIDE_TOOLBAR_DATE:
		SetToolbarVisible(TOOLBAR_DATE,!m_ToolbarList[TOOLBAR_DATE]->IsBarVisible());
		return true;

	case CM_PROGRAMGUIDE_TOOLBAR_TIME:
		SetToolbarVisible(TOOLBAR_TIME,!m_ToolbarList[TOOLBAR_TIME]->IsBarVisible());
		return true;

	case CM_PROGRAMGUIDE_TIME_CURRENT:
		if (m_pProgramGuide->GetListMode()==CProgramGuide::LIST_SERVICES)
			m_pProgramGuide->SetViewDay(CProgramGuide::DAY_TODAY);
		m_pProgramGuide->ScrollToCurrentTime();
		return true;

	case CM_PROGRAMGUIDE_TOOLBAROPTIONS:
		{
			CProgramGuideToolbarOptions Options(*m_pSettings);

			if (Options.Show(m_pProgramGuide->GetHandle())) {
				m_fNoUpdateLayout=true;

				for (int i=0;i<TOOLBAR_NUM;i++)
					SetToolbarVisible(i,m_pSettings->GetToolbarVisible(i));

				ProgramGuideBar::CDateToolbar *pDateToolbar=
					static_cast<ProgramGuideBar::CDateToolbar*>(m_ToolbarList[TOOLBAR_DATE]);
				if (pDateToolbar->GetButtonCount()!=m_pSettings->GetDateBarButtonCount()) {
					pDateToolbar->SetButtonCount(m_pSettings->GetDateBarButtonCount());
					pDateToolbar->OnTimeRangeChanged();
				}

				ProgramGuideBar::CTimeToolbar *pTimeToolbar=
					static_cast<ProgramGuideBar::CTimeToolbar*>(m_ToolbarList[TOOLBAR_TIME]);
				pTimeToolbar->SetSettings(m_pSettings->GetTimeBarSettings());
				pTimeToolbar->OnTimeRangeChanged();

				m_fNoUpdateLayout=false;
				OnLayoutChange();
			}
		}
		return true;
	}

	if (Command>=CM_PROGRAMGUIDE_TIME_FIRST
			&& Command<=CM_PROGRAMGUIDE_TIME_LAST) {
		const ProgramGuideBar::CTimeToolbar *pTimeToolbar=
			static_cast<const ProgramGuideBar::CTimeToolbar*>(m_ToolbarList[TOOLBAR_TIME]);
		SYSTEMTIME st;

		if (pTimeToolbar->GetTimeByCommand(Command,&st))
			m_pProgramGuide->ScrollToTime(st);
		return true;
	}

	return false;
}


void CProgramGuideFrameBase::OnMenuInitialize(HMENU hmenu)
{
	::CheckMenuItem(hmenu,CM_PROGRAMGUIDE_TOOLBAR_TUNERMENU,
					MF_BYCOMMAND | (m_ToolbarList[TOOLBAR_TUNER_MENU]->IsBarVisible()?MF_CHECKED:MF_UNCHECKED));
	::CheckMenuItem(hmenu,CM_PROGRAMGUIDE_TOOLBAR_DATEMENU,
					MF_BYCOMMAND | (m_ToolbarList[TOOLBAR_DATE_MENU]->IsBarVisible()?MF_CHECKED:MF_UNCHECKED));
	::CheckMenuItem(hmenu,CM_PROGRAMGUIDE_TOOLBAR_FAVORITES,
					MF_BYCOMMAND | (m_ToolbarList[TOOLBAR_FAVORITES]->IsBarVisible()?MF_CHECKED:MF_UNCHECKED));
	::CheckMenuItem(hmenu,CM_PROGRAMGUIDE_TOOLBAR_DATE,
					MF_BYCOMMAND | (m_ToolbarList[TOOLBAR_DATE]->IsBarVisible()?MF_CHECKED:MF_UNCHECKED));
	::CheckMenuItem(hmenu,CM_PROGRAMGUIDE_TOOLBAR_TIME,
					MF_BYCOMMAND | (m_ToolbarList[TOOLBAR_TIME]->IsBarVisible()?MF_CHECKED:MF_UNCHECKED));
}


void CProgramGuideFrameBase::OnWindowCreate(HWND hwnd,bool fBufferedPaint)
{
	m_pProgramGuide->SetFrame(this);
	m_pProgramGuide->Create(hwnd,WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL);

	for (int i=0;i<lengthof(m_ToolbarList);i++) {
		ProgramGuideBar::CProgramGuideBar *pBar=m_ToolbarList[i];

		pBar->SetBarVisible(m_pSettings->GetToolbarVisible(i));
		pBar->EnableBufferedPaint(fBufferedPaint);
		pBar->CreateBar(hwnd,WS_CHILD | (pBar->IsBarVisible()?WS_VISIBLE:0));
	}

	ProgramGuideBar::CDateToolbar *pDateToolbar=
		static_cast<ProgramGuideBar::CDateToolbar*>(m_ToolbarList[TOOLBAR_DATE]);
	pDateToolbar->SetButtonCount(m_pSettings->GetDateBarButtonCount());

	ProgramGuideBar::CTimeToolbar *pTimeToolbar=
		static_cast<ProgramGuideBar::CTimeToolbar*>(m_ToolbarList[TOOLBAR_TIME]);
	pTimeToolbar->SetSettings(m_pSettings->GetTimeBarSettings());
}


void CProgramGuideFrameBase::OnWindowDestroy()
{
	m_pProgramGuide->SetFrame(NULL);

	for (int i=0;i<lengthof(m_ToolbarList);i++)
		m_ToolbarList[i]->DestroyBar();
}


void CProgramGuideFrameBase::OnSizeChanged(int Width,int Height)
{
	int OrderList[TOOLBAR_NUM];
	m_pSettings->GetToolbarOrderList(OrderList);

	int x=m_ToolbarMargin.left;
	int y=m_ToolbarMargin.top;
	int BarHeight=0;

	for (int i=0;i<lengthof(m_ToolbarList);i++) {
		ProgramGuideBar::CProgramGuideBar *pBar=m_ToolbarList[OrderList[i]];

		if (pBar->IsBarVisible()) {
			SIZE sz;

			pBar->GetBarSize(&sz);
			if (x+sz.cx>Width-m_ToolbarMargin.right) {
				if (i>0) {
					x=m_ToolbarMargin.left;
					y+=BarHeight+m_ToolbarGap.y;
				}
				if (x+sz.cx>Width-m_ToolbarMargin.right)
					sz.cx=max(Width-m_ToolbarMargin.right-x,0);
				BarHeight=sz.cy;
			} else {
				if (sz.cy>BarHeight)
					BarHeight=sz.cy;
			}
			pBar->SetBarPosition(x,y,sz.cx,sz.cy);
			x+=sz.cx+m_ToolbarGap.x;
		}
	}

	if (BarHeight>0)
		y+=BarHeight+m_ToolbarMargin.bottom;
	else
		y=0;
	m_pProgramGuide->SetPosition(0,y,Width,max(Height-y,0));
}


LRESULT CProgramGuideFrameBase::DefaultMessageHandler(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_SIZE:
		OnSizeChanged(LOWORD(lParam),HIWORD(lParam));
		return 0;

	case WM_KEYDOWN:
		if (wParam==VK_ESCAPE) {
			::SendMessage(hwnd,WM_CLOSE,0,0);
			return 0;
		}
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		return m_pProgramGuide->SendMessage(uMsg,wParam,lParam);

	case WM_RBUTTONUP:
		{
			POINT pt={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
			::ClientToScreen(hwnd,&pt);

			HMENU hmenu=::LoadMenu(
				GetAppClass().GetResourceInstance(),
				MAKEINTRESOURCE(IDM_PROGRAMGUIDETOOLBAR));
			HMENU hmenuPopup=::GetSubMenu(hmenu,0);
			OnMenuInitialize(hmenuPopup);
			::TrackPopupMenu(hmenuPopup,TPM_RIGHTBUTTON,pt.x,pt.y,0,
							 m_pProgramGuide->GetHandle(),NULL);
			::DestroyMenu(hmenu);
		}
		return 0;

	case WM_SETCURSOR:
		{
			HWND hwndCursor=reinterpret_cast<HWND>(wParam);
			int HitTestCode=LOWORD(lParam);

			for (int i=0;i<lengthof(m_ToolbarList);i++) {
				if (m_ToolbarList[i]->OnSetCursor(hwndCursor,HitTestCode))
					return TRUE;
			}
		}
		break;

	case WM_NOTIFY:
		{
			LRESULT Result;

			for (int i=0;i<lengthof(m_ToolbarList);i++) {
				if (m_ToolbarList[i]->OnNotify(lParam,&Result))
					return Result;
			}
		}
		break;

	case WM_COMMAND:
		return m_pProgramGuide->SendMessage(WM_COMMAND,wParam,lParam);

	case WM_SHOWWINDOW:
		m_pProgramGuide->OnShowFrame(wParam!=FALSE);
		break;

	case WM_DESTROY:
		OnWindowDestroy();
		return 0;
	}

	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}




const CProgramGuideFrameSettings::ToolbarInfo
	CProgramGuideFrameSettings::m_ToolbarInfoList[TOOLBAR_NUM] =
{
	{TEXT("TunerMenu"),	TEXT("チューナーメニュー")},
	{TEXT("DateMenu"),	TEXT("日付メニュー")},
	{TEXT("Favorites"),	TEXT("番組表選択ボタン")},
	{TEXT("DateBar"),	TEXT("日付バー")},
	{TEXT("TimeBar"),	TEXT("時刻バー")},
};


CProgramGuideFrameSettings::CProgramGuideFrameSettings()
	: CSettingsBase(TEXT("ProgramGuide"))
	, m_DateBarButtonCount(DATEBAR_DEFAULTBUTTONCOUNT)
{
	for (int i=0;i<lengthof(m_ToolbarSettingsList);i++) {
		m_ToolbarSettingsList[i].fVisible=true;
		m_ToolbarSettingsList[i].Order=i;
	}
}


bool CProgramGuideFrameSettings::ReadSettings(CSettings &Settings)
{
	int OrderList[TOOLBAR_NUM];
	int Count=0;

	for (int i=0;i<TOOLBAR_NUM;i++) {
		TCHAR szText[32],szName[32];
		int ID;

		StdUtil::snprintf(szText,lengthof(szText),TEXT("Toolbar%d_Name"),i);
		if (Settings.Read(szText,szName,lengthof(szName))) {
			ID=ParseIDText(szName);
			if (ID<0)
				continue;
		} else {
			// ver.0.9.0 より前との互換用
			ID=i;
		}

		int j;
		for (j=0;j<Count;j++) {
			if (OrderList[j]==ID)
				break;
		}
		if (j<Count)
			continue;

		StdUtil::snprintf(szText,lengthof(szText),TEXT("Toolbar%d_Status"),i);
		unsigned int Status;
		if (Settings.Read(szText,&Status)) {
			m_ToolbarSettingsList[ID].fVisible=(Status & TOOLBAR_STATUS_VISIBLE)!=0;
		}

		OrderList[Count]=ID;
		Count++;
	}

	if (Count<TOOLBAR_NUM) {
		for (int i=0;i<TOOLBAR_NUM;i++) {
			int j;
			for (j=0;j<Count;j++) {
				if (OrderList[j]==i)
					break;
			}
			if (j==Count) {
				OrderList[Count]=i;
				Count++;
			}
		}
	}

	SetToolbarOrderList(OrderList);

	int Value;

	if (Settings.Read(TEXT("DateBar.ButtonCount"),&Value))
		m_DateBarButtonCount=CLAMP(Value,1,DATEBAR_MAXBUTTONCOUNT);

	if (Settings.Read(TEXT("TimeBar.TimeType"),&Value)
			&& (Value==TimeBarSettings::TIME_INTERVAL
				|| Value==TimeBarSettings::TIME_CUSTOM))
		m_TimeBarSettings.Time=static_cast<TimeBarSettings::TimeType>(Value);
	if (Settings.Read(TEXT("TimeBar.Interval"),&Value)
			&& Value>=TimeBarSettings::INTERVAL_MIN
			&& Value<=TimeBarSettings::INTERVAL_MAX)
		m_TimeBarSettings.Interval=Value;
	Settings.Read(TEXT("TimeBar.Custom"),&m_TimeBarSettings.CustomTime);
	if (Settings.Read(TEXT("TimeBar.MaxButtonCount"),&Value)
			&& Value>=TimeBarSettings::BUTTONCOUNT_MIN
			&& Value<=TimeBarSettings::BUTTONCOUNT_MAX)
		m_TimeBarSettings.MaxButtonCount=Value;

	return true;
}


bool CProgramGuideFrameSettings::WriteSettings(CSettings &Settings)
{
	int OrderList[TOOLBAR_NUM];

	GetToolbarOrderList(OrderList);

	for (int i=0;i<TOOLBAR_NUM;i++) {
		const int ID=OrderList[i];
		TCHAR szText[32];

		StdUtil::snprintf(szText,lengthof(szText),TEXT("Toolbar%d_Name"),i);
		Settings.Write(szText,m_ToolbarInfoList[ID].pszIDText);

		StdUtil::snprintf(szText,lengthof(szText),TEXT("Toolbar%d_Status"),i);
		unsigned int Status=0;
		if (m_ToolbarSettingsList[ID].fVisible)
			Status|=TOOLBAR_STATUS_VISIBLE;
		Settings.Write(szText,Status);
	}

	Settings.Write(TEXT("DateBar.ButtonCount"),m_DateBarButtonCount);

	Settings.Write(TEXT("TimeBar.TimeType"),static_cast<int>(m_TimeBarSettings.Time));
	Settings.Write(TEXT("TimeBar.Interval"),m_TimeBarSettings.Interval);
	Settings.Write(TEXT("TimeBar.Custom"),m_TimeBarSettings.CustomTime);
	Settings.Write(TEXT("TimeBar.MaxButtonCount"),m_TimeBarSettings.MaxButtonCount);

	return true;
}


LPCTSTR CProgramGuideFrameSettings::GetToolbarIDText(int Toolbar) const
{
	if (Toolbar<0 || Toolbar>=lengthof(m_ToolbarInfoList))
		return NULL;
	return m_ToolbarInfoList[Toolbar].pszIDText;
}


LPCTSTR CProgramGuideFrameSettings::GetToolbarName(int Toolbar) const
{
	if (Toolbar<0 || Toolbar>=lengthof(m_ToolbarInfoList))
		return NULL;
	return m_ToolbarInfoList[Toolbar].pszName;
}


bool CProgramGuideFrameSettings::SetToolbarVisible(int Toolbar,bool fVisible)
{
	if (Toolbar<0 || Toolbar>=lengthof(m_ToolbarSettingsList))
		return false;
	m_ToolbarSettingsList[Toolbar].fVisible=fVisible;
	return true;
}


bool CProgramGuideFrameSettings::GetToolbarVisible(int Toolbar) const
{
	if (Toolbar<0 || Toolbar>=lengthof(m_ToolbarSettingsList))
		return false;
	return m_ToolbarSettingsList[Toolbar].fVisible;
}


bool CProgramGuideFrameSettings::SetToolbarOrderList(const int *pOrder)
{
	if (pOrder==NULL)
		return false;

	for (int i=0;i<lengthof(m_ToolbarSettingsList);i++) {
		const int ID=pOrder[i];

		if (ID<0 || ID>=lengthof(m_ToolbarSettingsList))
			return false;

		for (int j=i+1;j<lengthof(m_ToolbarSettingsList);j++) {
			if (pOrder[j]==ID)
				return false;
		}
	}

	for (int i=0;i<lengthof(m_ToolbarSettingsList);i++)
		m_ToolbarSettingsList[pOrder[i]].Order=i;

	return true;
}


bool CProgramGuideFrameSettings::GetToolbarOrderList(int *pOrder) const
{
	if (pOrder==NULL)
		return false;

	for (int i=0;i<lengthof(m_ToolbarSettingsList);i++)
		pOrder[m_ToolbarSettingsList[i].Order]=i;

	return true;
}


bool CProgramGuideFrameSettings::SetDateBarButtonCount(int ButtonCount)
{
	if (ButtonCount<1 || ButtonCount>DATEBAR_MAXBUTTONCOUNT)
		return false;

	m_DateBarButtonCount=ButtonCount;

	return true;
}


bool CProgramGuideFrameSettings::SetTimeBarSettings(const TimeBarSettings &Settings)
{
	m_TimeBarSettings=Settings;
	return true;
}


int CProgramGuideFrameSettings::ParseIDText(LPCTSTR pszID) const
{
	if (IsStringEmpty(pszID))
		return -1;

	for (int i=0;i<lengthof(m_ToolbarInfoList);i++) {
		if (::lstrcmpi(m_ToolbarInfoList[i].pszIDText,pszID)==0)
			return i;
	}

	return -1;
}




CProgramGuideFrameSettings::TimeBarSettings::TimeBarSettings()
	: Time(TIME_INTERVAL)
	, Interval(4)
	, CustomTime(TEXT("0,3,6,9,12,15,18,21"))
	, MaxButtonCount(10)
{
}




const LPCTSTR CProgramGuideFrame::m_pszWindowClass=APP_NAME TEXT(" Program Guide Frame");
HINSTANCE CProgramGuideFrame::m_hinst=NULL;


bool CProgramGuideFrame::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=0;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=::LoadIcon(hinst,MAKEINTRESOURCE(IDI_PROGRAMGUIDE));
		wc.hCursor=::LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=::CreateSolidBrush(0xFF000000);
		wc.lpszMenuName=NULL;
		wc.lpszClassName=m_pszWindowClass;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CProgramGuideFrame::CProgramGuideFrame(CProgramGuide *pProgramGuide,CProgramGuideFrameSettings *pSettings)
	: CProgramGuideFrameBase(pProgramGuide,pSettings)
	, m_fAlwaysOnTop(false)
{
	m_WindowPosition.Width=640;
	m_WindowPosition.Height=480;
}


CProgramGuideFrame::~CProgramGuideFrame()
{
	Destroy();
}


bool CProgramGuideFrame::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	if (m_WindowPosition.fMaximized)
		Style|=WS_MAXIMIZE;
	if (m_fAlwaysOnTop)
		ExStyle|=WS_EX_TOPMOST;
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 m_pszWindowClass,TITLE_TEXT,m_hinst);
}


bool CProgramGuideFrame::SetAlwaysOnTop(bool fTop)
{
	if (m_fAlwaysOnTop!=fTop) {
		m_fAlwaysOnTop=fTop;
		if (m_hwnd!=NULL)
			::SetWindowPos(m_hwnd,fTop?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
	}
	return true;
}


void CProgramGuideFrame::SetTheme(const TVTest::Theme::CThemeManager *pThemeManager)
{
	CProgramGuideFrameBase::SetTheme(pThemeManager);
}


bool CProgramGuideFrame::Show()
{
	if (m_hwnd==NULL)
		return false;
	::ShowWindow(m_hwnd,m_WindowPosition.fMaximized?SW_SHOWMAXIMIZED:SW_SHOWNORMAL);
	return true;
}


void CProgramGuideFrame::OnLayoutChange()
{
	SendSizeMessage();
}


void CProgramGuideFrame::SetCaption(LPCTSTR pszFileName)
{
	::SetWindowText(m_hwnd,pszFileName);
}


LRESULT CProgramGuideFrame::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		OnWindowCreate(hwnd,true);
		return 0;

	case WM_SIZE:
		{
			RECT rcOld,rcNew;

			m_pProgramGuide->GetPosition(&rcOld);
			DefaultMessageHandler(hwnd,uMsg,wParam,lParam);
			m_pProgramGuide->GetPosition(&rcNew);
			if (rcNew.top!=rcOld.top) {
				SetAeroGlass();
			}
		}
		return 0;

	case WM_SHOWWINDOW:
		if (!wParam)
			break;
#ifndef WM_DWMCOMPOSITIONCHANGED
#define WM_DWMCOMPOSITIONCHANGED 0x031E
#endif
	case WM_DWMCOMPOSITIONCHANGED:
		SetAeroGlass();
		break;

	case WM_PAINT:
		if (m_AeroGlass.IsEnabled()) {
			::PAINTSTRUCT ps;

			::BeginPaint(hwnd,&ps);
			{
				CBufferedPaint BufferedPaint;
				RECT rc;
				::GetClientRect(hwnd,&rc);
				HDC hdc=BufferedPaint.Begin(ps.hdc,&rc);
				if (hdc!=NULL) {
					::FillRect(hdc,&ps.rcPaint,static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
					BufferedPaint.SetAlpha(32);
					BufferedPaint.End();
				}
			}
			::EndPaint(hwnd,&ps);
			return 0;
		}
		break;

	case WM_ERASEBKGND:
		if (!m_AeroGlass.IsEnabled()) {
			RECT rc;
			::GetClientRect(hwnd,&rc);
			::FillRect(reinterpret_cast<HDC>(wParam),&rc,
					   reinterpret_cast<HBRUSH>(COLOR_3DFACE+1));
			return 1;
		}
		break;

	case WM_CLOSE:
		if (!m_pProgramGuide->OnCloseFrame())
			return 0;
		break;
	}

	return DefaultMessageHandler(hwnd,uMsg,wParam,lParam);
}


void CProgramGuideFrame::SetAeroGlass()
{
	if (m_hwnd!=NULL) {
		RECT rc={0,0,0,0},rcPos;

		m_pProgramGuide->GetPosition(&rcPos);
		rc.top=rcPos.top;
		m_AeroGlass.ApplyAeroGlass(m_hwnd,&rc);
	}
}




const LPCTSTR CProgramGuideDisplay::m_pszWindowClass=APP_NAME TEXT(" Program Guide Display");
HINSTANCE CProgramGuideDisplay::m_hinst=NULL;


bool CProgramGuideDisplay::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=::LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=::CreateSolidBrush(RGB(0,0,0));
		wc.lpszMenuName=NULL;
		wc.lpszClassName=m_pszWindowClass;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CProgramGuideDisplay::CProgramGuideDisplay(CProgramGuide *pProgramGuide,CProgramGuideFrameSettings *pSettings)
	: CProgramGuideFrameBase(pProgramGuide,pSettings)
	, m_pProgramGuideDisplayEventHandler(NULL)
{
	m_ToolbarMargin.left=6;
	m_ToolbarMargin.top=m_ToolbarMargin.bottom;
}


CProgramGuideDisplay::~CProgramGuideDisplay()
{
	Destroy();
}


bool CProgramGuideDisplay::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 m_pszWindowClass,NULL,m_hinst);
}


void CProgramGuideDisplay::SetTheme(const TVTest::Theme::CThemeManager *pThemeManager)
{
	CProgramGuideFrameBase::SetTheme(pThemeManager);
	CDisplayView::SetTheme(pThemeManager);
}


void CProgramGuideDisplay::SetEventHandler(CProgramGuideDisplayEventHandler *pHandler)
{
	if (m_pProgramGuideDisplayEventHandler!=NULL)
		m_pProgramGuideDisplayEventHandler->m_pProgramGuideDisplay=NULL;
	if (pHandler!=NULL)
		pHandler->m_pProgramGuideDisplay=this;
	m_pProgramGuideDisplayEventHandler=pHandler;
	CDisplayView::SetEventHandler(pHandler);
}


bool CProgramGuideDisplay::Close()
{
	SetVisible(false);
	return true;
}


bool CProgramGuideDisplay::OnVisibleChange(bool fVisible)
{
	if (!fVisible && m_pProgramGuideDisplayEventHandler!=NULL)
		return m_pProgramGuideDisplayEventHandler->OnHide();
	return true;
}


bool CProgramGuideDisplay::SetAlwaysOnTop(bool fTop)
{
	if (m_pProgramGuideDisplayEventHandler==NULL)
		return false;
	return m_pProgramGuideDisplayEventHandler->SetAlwaysOnTop(fTop);
}


bool CProgramGuideDisplay::GetAlwaysOnTop() const
{
	if (m_pProgramGuideDisplayEventHandler==NULL)
		return false;
	return m_pProgramGuideDisplayEventHandler->GetAlwaysOnTop();
}


void CProgramGuideDisplay::OnLayoutChange()
{
	SendSizeMessage();
}


LRESULT CProgramGuideDisplay::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		InitializeUI();
		OnWindowCreate(hwnd,false);
		return 0;

	case WM_SIZE:
		{
			RECT rc;

			GetCloseButtonRect(&rc);
			m_ToolbarMargin.right=(LOWORD(lParam)-rc.left)+m_ToolbarMargin.left;
		}
		break;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd,&ps);
			DrawCloseButton(ps.hdc);
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);

			if (CloseButtonHitTest(x,y))
				Close();
			else
				::SetFocus(hwnd);
		}
		return 0;

	case WM_KEYDOWN:
		if (wParam==VK_ESCAPE) {
			Close();
			return 0;
		}
		break;

	default:
		{
			LRESULT Result;
			if (HandleMessage(hwnd,uMsg,wParam,lParam,&Result))
				return Result;
		}
		break;
	}

	return DefaultMessageHandler(hwnd,uMsg,wParam,lParam);
}


CProgramGuideDisplay::CProgramGuideDisplayEventHandler::CProgramGuideDisplayEventHandler()
	: m_pProgramGuideDisplay(NULL)
{
}




class CEpgVariableStringMap : public TVTest::CEventVariableStringMap
{
public:
	CEpgVariableStringMap();
	CEpgVariableStringMap(const EventInfo &Info);
	bool GetString(LPCWSTR pszKeyword,TVTest::String *pString) override;
	bool NormalizeString(TVTest::String *pString) const override { return false; }
	bool GetParameterInfo(int Index,ParameterInfo *pInfo) const override;
	int GetParameterCount() const override;
	const TVTest::String &GetiEpgFileName() const { return m_iEpgFileName; }

private:
	static const ParameterInfo m_EpgParameterList[];

	TVTest::String m_iEpgFileName;
};


const CEpgVariableStringMap::ParameterInfo CEpgVariableStringMap::m_EpgParameterList[] = {
//	{TEXT("%eid%"),				TEXT("イベントID")},
	{TEXT("%nid%"),				TEXT("ネットワークID")},
	{TEXT("%tsid%"),			TEXT("ストリームID")},
//	{TEXT("%sid%"),				TEXT("サービスID")},
	{TEXT("%tvpid%"),			TEXT("iEPGファイル")},
	{TEXT("%duration-sec%"),	TEXT("番組の長さ(秒単位)")},
	{TEXT("%duration-min%"),	TEXT("番組の長さ(分単位)")},
};


CEpgVariableStringMap::CEpgVariableStringMap()
{
}


CEpgVariableStringMap::CEpgVariableStringMap(const EventInfo &Info)
	: CEventVariableStringMap(Info)
{
}


bool CEpgVariableStringMap::GetString(LPCWSTR pszKeyword,TVTest::String *pString)
{
	if (::lstrcmpi(pszKeyword,TEXT("tvpid"))==0) {
		if (m_iEpgFileName.empty()) {
			TCHAR sziEpgFileName[MAX_PATH+11];

			GetAppClass().GetAppDirectory(sziEpgFileName);
			::PathAppend(sziEpgFileName,TEXT("iepg.tvpid"));
			m_iEpgFileName=sziEpgFileName;
		}
		*pString=m_iEpgFileName;
	} else if (::lstrcmpi(pszKeyword,TEXT("eid"))==0) {
		TVTest::StringUtility::Format(*pString,TEXT("%d"),m_EventInfo.Event.m_EventID);
	} else if (::lstrcmpi(pszKeyword,TEXT("nid"))==0) {
		TVTest::StringUtility::Format(*pString,TEXT("%d"),m_EventInfo.Channel.GetNetworkID());
	} else if (::lstrcmpi(pszKeyword,TEXT("tsid"))==0) {
		TVTest::StringUtility::Format(*pString,TEXT("%d"),m_EventInfo.Channel.GetTransportStreamID());
	} else if (::lstrcmpi(pszKeyword,TEXT("sid"))==0) {
		TVTest::StringUtility::Format(*pString,TEXT("%d"),m_EventInfo.Channel.GetServiceID());
	} else if (::lstrcmpi(pszKeyword,TEXT("duration-sec"))==0) {
		TVTest::StringUtility::Format(*pString,TEXT("%d"),m_EventInfo.Event.m_Duration);
	} else if (::lstrcmpi(pszKeyword,TEXT("duration-min"))==0) {
		TVTest::StringUtility::Format(*pString,TEXT("%d"),(m_EventInfo.Event.m_Duration+59)/60);
	} else {
		return CEventVariableStringMap::GetString(pszKeyword,pString);
	}

	return true;
}


bool CEpgVariableStringMap::GetParameterInfo(int Index,ParameterInfo *pInfo) const
{
	if (pInfo==NULL)
		return false;

	if (CEventVariableStringMap::GetParameterInfo(Index,pInfo))
		return true;

	Index-=CEventVariableStringMap::GetParameterCount();
	if (Index>=0 && Index<lengthof(m_EpgParameterList)) {
		*pInfo=m_EpgParameterList[Index];
		return true;
	}

	return false;
}


int CEpgVariableStringMap::GetParameterCount() const
{
	return CEventVariableStringMap::GetParameterCount()+lengthof(m_EpgParameterList);
}




CProgramGuideTool::CProgramGuideTool()
{
	m_szName[0]='\0';
	m_szCommand[0]='\0';
}


CProgramGuideTool::CProgramGuideTool(const CProgramGuideTool &Tool)
{
	*this=Tool;
}


CProgramGuideTool::CProgramGuideTool(LPCTSTR pszName,LPCTSTR pszCommand)
{
	::lstrcpy(m_szName,pszName);
	::lstrcpy(m_szCommand,pszCommand);
}


CProgramGuideTool::~CProgramGuideTool()
{
}


CProgramGuideTool &CProgramGuideTool::operator=(const CProgramGuideTool &Tool)
{
	if (&Tool!=this) {
		::lstrcpy(m_szName,Tool.m_szName);
		::lstrcpy(m_szCommand,Tool.m_szCommand);
		m_Icon=Tool.m_Icon;
	}
	return *this;
}


bool CProgramGuideTool::GetPath(LPTSTR pszPath,int MaxLength) const
{
	LPCTSTR p=m_szCommand;

	return GetCommandFileName(&p,pszPath,MaxLength);
}


HICON CProgramGuideTool::GetIcon()
{
	if (!m_Icon && m_szCommand[0]!='\0') {
		TCHAR szFileName[MAX_PATH];
		LPCTSTR p=m_szCommand;

		if (GetCommandFileName(&p,szFileName,lengthof(szFileName))) {
			SHFILEINFO shfi;
			if (::SHGetFileInfo(szFileName,0,&shfi,sizeof(shfi),
								SHGFI_ICON | SHGFI_SMALLICON))
				m_Icon.Attach(shfi.hIcon);
		}
	}
	return m_Icon;
}


bool CProgramGuideTool::Execute(const ProgramGuide::CServiceInfo *pServiceInfo,
								const CEventInfoData *pEventInfo,HWND hwnd)
{
	if (pServiceInfo==NULL || pEventInfo==NULL)
		return false;

	SYSTEMTIME stStart,stEnd;
	TCHAR szFileName[MAX_PATH];
	LPCTSTR p;

	pEventInfo->GetStartTime(&stStart);
	pEventInfo->GetEndTime(&stEnd);
	p=m_szCommand;
	if (!GetCommandFileName(&p,szFileName,lengthof(szFileName))) {
		::MessageBox(hwnd,TEXT("パスが長すぎます。"),NULL,MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	while (*p==_T(' '))
		p++;

	TVTest::CEventVariableStringMap::EventInfo Info;
	Info.Channel=pServiceInfo->GetChannelInfo();
	Info.Event=*pEventInfo;
	Info.ServiceName=pServiceInfo->GetServiceName();
	::GetLocalTime(&Info.TotTime);

	CEpgVariableStringMap VarStrMap(Info);
	TVTest::String Parameter;
	TVTest::FormatVariableString(&VarStrMap,p,&Parameter);
	const TVTest::String &iEpgFileName=VarStrMap.GetiEpgFileName();
	if (!iEpgFileName.empty()) {
		if (!pServiceInfo->SaveiEpgFile(pEventInfo,iEpgFileName.c_str(),true)) {
			::MessageBox(hwnd,TEXT("iEPGファイルの書き出しができません。"),NULL,
						 MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
	}

	TRACE(TEXT("外部ツール実行 : %s, %s\n"),szFileName,Parameter.c_str());

	return ::ShellExecute(NULL,NULL,szFileName,Parameter.c_str(),NULL,SW_SHOWNORMAL)>=(HINSTANCE)32;
}


bool CProgramGuideTool::ShowDialog(HWND hwndOwner)
{
	return ::DialogBoxParam(GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_PROGRAMGUIDETOOL),
							hwndOwner,DlgProc,reinterpret_cast<LPARAM>(this))==IDOK;
}


CProgramGuideTool *CProgramGuideTool::GetThis(HWND hDlg)
{
	return static_cast<CProgramGuideTool*>(::GetProp(hDlg,TEXT("This")));
}


INT_PTR CALLBACK CProgramGuideTool::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CProgramGuideTool *pThis=reinterpret_cast<CProgramGuideTool*>(lParam);

			::SetProp(hDlg,TEXT("This"),pThis);
			::SendDlgItemMessage(hDlg,IDC_PROGRAMGUIDETOOL_NAME,
								 EM_LIMITTEXT,MAX_NAME-1,0);
			::SetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_NAME,pThis->m_szName);
			::SendDlgItemMessage(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,
								 EM_LIMITTEXT,MAX_COMMAND-1,0);
			::SetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,pThis->m_szCommand);
			if (pThis->m_szName[0]!='\0' && pThis->m_szCommand[0]!='\0')
				EnableDlgItem(hDlg,IDOK,true);
			AdjustDialogPos(::GetParent(hDlg),hDlg);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_PROGRAMGUIDETOOL_NAME:
		case IDC_PROGRAMGUIDETOOL_COMMAND:
			if (HIWORD(wParam)==EN_CHANGE)
				EnableDlgItem(hDlg,IDOK,
					GetDlgItemTextLength(hDlg,IDC_PROGRAMGUIDETOOL_NAME)>0
					&& GetDlgItemTextLength(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND)>0);
			return TRUE;

		case IDC_PROGRAMGUIDETOOL_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szCommand[MAX_COMMAND];
				TCHAR szFileName[MAX_PATH],szDirectory[MAX_PATH];

				GetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,szCommand,MAX_COMMAND);
				if (szCommand[0]!='\0') {
					LPCTSTR p;

					p=szCommand;
					GetCommandFileName(&p,szFileName,lengthof(szFileName));
				} else
					szFileName[0]='\0';
				InitOpenFileName(&ofn);
				ofn.hwndOwner=hDlg;
				ofn.lpstrFilter=TEXT("実行ファイル(*.exe;*.bat)\0*.exe;*.bat\0")
								TEXT("すべてのファイル\0*.*\0");
				ofn.nFilterIndex=1;
				ofn.lpstrFile=szFileName;
				ofn.nMaxFile=MAX_PATH;
				if (szFileName[0]!='\0') {
					CFilePath Path(szFileName);
					Path.GetDirectory(szDirectory);
					ofn.lpstrInitialDir=szDirectory;
					::lstrcpy(szFileName,Path.GetFileName());
				}
				ofn.Flags=OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
				if (::GetOpenFileName(&ofn)) {
					::PathQuoteSpaces(szFileName);
					//::lstrcat(szFileName,TEXT(" \"%tvpid%\""));
					::SetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,szFileName);
					if (GetDlgItemTextLength(hDlg,IDC_PROGRAMGUIDETOOL_NAME)==0) {
						*::PathFindExtension(szFileName)='\0';
						::SetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_NAME,
										 ::PathFindFileName(szFileName));
					}
				}
				return TRUE;

			case IDC_PROGRAMGUIDETOOL_PARAMETER:
				{
					CEpgVariableStringMap VarStrMap;
					RECT rc;
					POINT pt;

					::GetWindowRect(::GetDlgItem(hDlg,IDC_PROGRAMGUIDETOOL_PARAMETER),&rc);
					pt.x=rc.left;
					pt.y=rc.bottom;
					VarStrMap.InputParameter(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,pt);
				}
				return TRUE;
			}
			return TRUE;

		case IDC_PROGRAMGUIDETOOL_HELP:
			GetAppClass().UICore.ShowHelpContent(HELP_ID_PROGRAMGUIDETOOL);
			return TRUE;

		case IDOK:
			{
				CProgramGuideTool *pThis=GetThis(hDlg);

				::GetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_NAME,
								 pThis->m_szName,MAX_NAME);
				::GetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,
								 pThis->m_szCommand,MAX_COMMAND);
				pThis->m_Icon.Destroy();
			}
		case IDCANCEL:
			EndDialog(hDlg,LOWORD(wParam));
		}
		return TRUE;

	case WM_NCDESTROY:
		::RemoveProp(hDlg,TEXT("This"));
		return TRUE;
	}
	return FALSE;
}


bool CProgramGuideTool::GetCommandFileName(LPCTSTR *ppszCommand,LPTSTR pszFileName,int MaxFileName)
{
	LPCTSTR p;
	LPTSTR q;
	TCHAR cDelimiter;

	p=*ppszCommand;
	q=pszFileName;
	if (*p==_T('"')) {
		cDelimiter=_T('"');
		p++;
	} else {
		cDelimiter=_T(' ');
	}
	int Length=0;
	while (*p!=_T('\0') && *p!=cDelimiter) {
		int CharLength=StringCharLength(p);
		if (CharLength==0 || Length+CharLength>=MaxFileName) {
			pszFileName[0]=_T('\0');
			return false;
		}
		for (int i=0;i<CharLength;i++)
			*q++=*p++;
		Length+=CharLength;
	}
	*q=_T('\0');
	if (*p==cDelimiter)
		p++;
	*ppszCommand=p;
	return true;
}




CProgramGuideToolList::CProgramGuideToolList()
{
}


CProgramGuideToolList::CProgramGuideToolList(const CProgramGuideToolList &Src)
{
	*this=Src;
}


CProgramGuideToolList::~CProgramGuideToolList()
{
	Clear();
}


CProgramGuideToolList &CProgramGuideToolList::operator=(const CProgramGuideToolList &Src)
{
	if (&Src!=this) {
		Clear();
		if (Src.m_ToolList.size()>0) {
			m_ToolList.resize(Src.m_ToolList.size());
			for (size_t i=0;i<Src.m_ToolList.size();i++)
				m_ToolList[i]=new CProgramGuideTool(*Src.m_ToolList[i]);
		}
	}
	return *this;
}


void CProgramGuideToolList::Clear()
{
	for (size_t i=0;i<m_ToolList.size();i++)
		delete m_ToolList[i];
	m_ToolList.clear();
}


bool CProgramGuideToolList::Add(CProgramGuideTool *pTool)
{
	if (pTool==NULL)
		return false;
	m_ToolList.push_back(pTool);
	return true;
}


CProgramGuideTool *CProgramGuideToolList::GetTool(size_t Index)
{
	if (Index>=m_ToolList.size())
		return NULL;
	return m_ToolList[Index];
}


const CProgramGuideTool *CProgramGuideToolList::GetTool(size_t Index) const
{
	if (Index>=m_ToolList.size())
		return NULL;
	return m_ToolList[Index];
}
