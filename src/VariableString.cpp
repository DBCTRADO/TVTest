#include "stdafx.h"
#include "TVTest.h"
#include "VariableString.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


namespace TVTest
{


bool FormatVariableString(CVariableStringMap *pVariableMap,LPCWSTR pszFormat,String *pString)
{
	if (pString==nullptr)
		return false;

	pString->clear();

	if (pszFormat==nullptr || pVariableMap==nullptr)
		return false;

	if (!pVariableMap->BeginFormat())
		return false;

	LPCWSTR p=pszFormat;

	while (*p!=L'\0') {
		if (*p==L'%') {
			p++;
			if (*p==L'%') {
				pString->append(L"%");
				p++;
			} else {
				String Keyword,Text;

				while (*p!=L'%' && *p!=L'\0')
					Keyword.push_back(*p++);
				if (*p==L'%') {
					p++;
					if (pVariableMap->GetString(Keyword.c_str(),&Text)) {
						pVariableMap->NormalizeString(&Text);
						pString->append(Text);
					} else {
						pString->append(L"%");
						pVariableMap->NormalizeString(&Keyword);
						pString->append(Keyword);
						pString->append(L"%");
					}
				} else {
					pString->append(L"%");
					pVariableMap->NormalizeString(&Keyword);
					pString->append(Keyword);
				}
			}
		} else {
			pString->push_back(*p++);
		}
	}

	pVariableMap->EndFormat();

	return true;
}




bool CVariableStringMap::InputParameter(HWND hDlg,int EditID,const POINT &MenuPos)
{
	HMENU hmenuRoot=::CreatePopupMenu();
	HMENU hmenu=hmenuRoot;
	ParameterInfo Info;

	for (int i=0;GetParameterInfo(i,&Info);i++) {
		if (Info.pszParameter==nullptr) {
			if (Info.pszText!=nullptr) {
				hmenu=::CreatePopupMenu();
				::AppendMenu(hmenuRoot,MF_POPUP | MF_STRING | MF_ENABLED,
							 reinterpret_cast<UINT_PTR>(hmenu),
							 Info.pszText);
			} else {
				hmenu=hmenuRoot;
			}
		} else {
			TCHAR szText[128];
			StdUtil::snprintf(szText,lengthof(szText),TEXT("%s\t%s"),
							  Info.pszText,Info.pszParameter);
			::AppendMenu(hmenu,MF_STRING | MF_ENABLED,i+1,szText);
		}
	}

	int Command=::TrackPopupMenu(hmenuRoot,TPM_RETURNCMD,MenuPos.x,MenuPos.y,0,hDlg,NULL);
	::DestroyMenu(hmenuRoot);
	if (Command<=0 || !GetParameterInfo(Command-1,&Info))
		return false;

	DWORD Start,End;
	::SendDlgItemMessage(hDlg,EditID,EM_GETSEL,
		reinterpret_cast<WPARAM>(&Start),reinterpret_cast<LPARAM>(&End));
	::SendDlgItemMessage(hDlg,EditID,EM_REPLACESEL,
		TRUE,reinterpret_cast<LPARAM>(Info.pszParameter));
	::SetFocus(::GetDlgItem(hDlg,EditID));
	if (End<Start)
		Start=End;
	::SendDlgItemMessage(hDlg,EditID,EM_SETSEL,
		Start,Start+::lstrlen(Info.pszParameter));

	return true;
}


bool CVariableStringMap::GetTimeString(LPCWSTR pszKeyword,const SYSTEMTIME &Time,String *pString) const
{
	if (::lstrcmpiW(pszKeyword,L"date")==0) {
		StringUtility::Format(*pString,L"%d%02d%02d",
							  Time.wYear,Time.wMonth,Time.wDay);
	} else if (::lstrcmpiW(pszKeyword,L"time")==0) {
		StringUtility::Format(*pString,L"%02d%02d%02d",
							  Time.wHour,Time.wMinute,Time.wSecond);
	} else if (::lstrcmpiW(pszKeyword,L"year")==0) {
		StringUtility::Format(*pString,L"%d",Time.wYear);
	} else if (::lstrcmpiW(pszKeyword,L"year2")==0) {
		StringUtility::Format(*pString,L"%02d",Time.wYear%100);
	} else if (::lstrcmpiW(pszKeyword,L"month")==0) {
		StringUtility::Format(*pString,L"%d",Time.wMonth);
	} else if (::lstrcmpiW(pszKeyword,L"month2")==0) {
		StringUtility::Format(*pString,L"%02d",Time.wMonth);
	} else if (::lstrcmpiW(pszKeyword,L"day")==0) {
		StringUtility::Format(*pString,L"%d",Time.wDay);
	} else if (::lstrcmpiW(pszKeyword,L"day2")==0) {
		StringUtility::Format(*pString,L"%02d",Time.wDay);
	} else if (::lstrcmpiW(pszKeyword,L"hour")==0) {
		StringUtility::Format(*pString,L"%d",Time.wHour);
	} else if (::lstrcmpiW(pszKeyword,L"hour2")==0) {
		StringUtility::Format(*pString,L"%02d",Time.wHour);
	} else if (::lstrcmpiW(pszKeyword,L"minute")==0) {
		StringUtility::Format(*pString,L"%d",Time.wMinute);
	} else if (::lstrcmpiW(pszKeyword,L"minute2")==0) {
		StringUtility::Format(*pString,L"%02d",Time.wMinute);
	} else if (::lstrcmpiW(pszKeyword,L"second")==0) {
		StringUtility::Format(*pString,L"%d",Time.wSecond);
	} else if (::lstrcmpiW(pszKeyword,L"second2")==0) {
		StringUtility::Format(*pString,L"%02d",Time.wSecond);
	} else if (::lstrcmpiW(pszKeyword,L"day-of-week")==0) {
		*pString=GetDayOfWeekText(Time.wDayOfWeek);
	} else {
		return false;
	}

	return true;
}


bool CVariableStringMap::IsDateTimeParameter(LPCTSTR pszKeyword)
{
	static const LPCTSTR ParameterList[] = {
		TEXT("date"),
		TEXT("year"),
		TEXT("year2"),
		TEXT("month"),
		TEXT("month2"),
		TEXT("day"),
		TEXT("day2"),
		TEXT("time"),
		TEXT("hour"),
		TEXT("hour2"),
		TEXT("minute"),
		TEXT("minute2"),
		TEXT("second"),
		TEXT("second2"),
		TEXT("day-of-week"),
	};

	for (int i=0;i<lengthof(ParameterList);i++) {
		if (::lstrcmpi(ParameterList[i],pszKeyword)==0)
			return true;
	}

	return false;
}



const CVariableStringMap::ParameterInfo CEventVariableStringMap::m_ParameterList[] =
{
	{nullptr,							TEXT("録画開始日時")},
	{TEXT("%date%"),					TEXT("年月日")},
	{TEXT("%year%"),					TEXT("年")},
	{TEXT("%year2%"),					TEXT("年(下2桁)")},
	{TEXT("%month%"),					TEXT("月")},
	{TEXT("%month2%"),					TEXT("月(2桁)")},
	{TEXT("%day%"),						TEXT("日")},
	{TEXT("%day2%"),					TEXT("日(2桁)")},
	{TEXT("%time%"),					TEXT("時刻(時+分+秒)")},
	{TEXT("%hour%"),					TEXT("時")},
	{TEXT("%hour2%"),					TEXT("時(2桁)")},
	{TEXT("%minute%"),					TEXT("分")},
	{TEXT("%minute2%"),					TEXT("分(2桁)")},
	{TEXT("%second%"),					TEXT("秒")},
	{TEXT("%second2%"),					TEXT("秒(2桁)")},
	{TEXT("%day-of-week%"),				TEXT("曜日(漢字)")},
	{nullptr,							TEXT("番組開始日時")},
	{TEXT("%start-date%"),				TEXT("年月日")},
	{TEXT("%start-year%"),				TEXT("年")},
	{TEXT("%start-year2%"),				TEXT("年(下2桁)")},
	{TEXT("%start-month%"),				TEXT("月")},
	{TEXT("%start-month2%"),			TEXT("月(2桁)")},
	{TEXT("%start-day%"),				TEXT("日")},
	{TEXT("%start-day2%"),				TEXT("日(2桁)")},
	{TEXT("%start-time%"),				TEXT("時刻(時+分+秒)")},
	{TEXT("%start-hour%"),				TEXT("時")},
	{TEXT("%start-hour2%"),				TEXT("時(2桁)")},
	{TEXT("%start-minute%"),			TEXT("分")},
	{TEXT("%start-minute2%"),			TEXT("分(2桁)")},
	{TEXT("%start-second%"),			TEXT("秒")},
	{TEXT("%start-second2%"),			TEXT("秒(2桁)")},
	{TEXT("%start-day-of-week%"),		TEXT("曜日(漢字)")},
	{nullptr,							TEXT("番組終了日時")},
	{TEXT("%end-date%"),				TEXT("年月日")},
	{TEXT("%end-year%"),				TEXT("年")},
	{TEXT("%end-year2%"),				TEXT("年(下2桁)")},
	{TEXT("%end-month%"),				TEXT("月")},
	{TEXT("%end-month2%"),				TEXT("月(2桁)")},
	{TEXT("%end-day%"),					TEXT("日")},
	{TEXT("%end-day2%"),				TEXT("日(2桁)")},
	{TEXT("%end-time%"),				TEXT("時刻(時+分+秒)")},
	{TEXT("%end-hour%"),				TEXT("時")},
	{TEXT("%end-hour2%"),				TEXT("時(2桁)")},
	{TEXT("%end-minute%"),				TEXT("分")},
	{TEXT("%end-minute2%"),				TEXT("分(2桁)")},
	{TEXT("%end-second%"),				TEXT("秒")},
	{TEXT("%end-second2%"),				TEXT("秒(2桁)")},
	{TEXT("%end-day-of-week%"),			TEXT("曜日(漢字)")},
	{nullptr,							TEXT("TOT日時")},
	{TEXT("%tot-date%"),				TEXT("年月日")},
	{TEXT("%tot-year%"),				TEXT("年")},
	{TEXT("%tot-year2%"),				TEXT("年(下2桁)")},
	{TEXT("%tot-month%"),				TEXT("月")},
	{TEXT("%tot-month2%"),				TEXT("月(2桁)")},
	{TEXT("%tot-day%"),					TEXT("日")},
	{TEXT("%tot-day2%"),				TEXT("日(2桁)")},
	{TEXT("%tot-time%"),				TEXT("時刻(時+分+秒)")},
	{TEXT("%tot-hour%"),				TEXT("時")},
	{TEXT("%tot-hour2%"),				TEXT("時(2桁)")},
	{TEXT("%tot-minute%"),				TEXT("分")},
	{TEXT("%tot-minute2%"),				TEXT("分(2桁)")},
	{TEXT("%tot-second%"),				TEXT("秒")},
	{TEXT("%tot-second2%"),				TEXT("秒(2桁)")},
	{TEXT("%tot-day-of-week%"),			TEXT("曜日(漢字)")},
	{nullptr,							TEXT("番組の長さ")},
	{TEXT("%event-duration-hour%"),		TEXT("時間")},
	{TEXT("%event-duration-hour2%"),	TEXT("時間(2桁)")},
	{TEXT("%event-duration-min%"),		TEXT("分")},
	{TEXT("%event-duration-min2%"),		TEXT("分(2桁)")},
	{TEXT("%event-duration-sec%"),		TEXT("秒")},
	{TEXT("%event-duration-sec2%"),		TEXT("秒(2桁)")},
	{nullptr,							nullptr},
	{TEXT("%channel-name%"),			TEXT("チャンネル名")},
	{TEXT("%channel-no%"),				TEXT("チャンネル番号")},
	{TEXT("%channel-no2%"),				TEXT("チャンネル番号(2桁)")},
	{TEXT("%channel-no3%"),				TEXT("チャンネル番号(3桁)")},
	{TEXT("%event-name%"),				TEXT("番組名")},
	{TEXT("%event-title%"),				TEXT("番組タイトル")},
	{TEXT("%event-id%"),				TEXT("イベントID")},
	{TEXT("%service-name%"),			TEXT("サービス名")},
	{TEXT("%service-id%"),				TEXT("サービスID")},
};


CEventVariableStringMap::CEventVariableStringMap()
	: m_fCurrentTimeSet(false)
{
}


CEventVariableStringMap::CEventVariableStringMap(const EventInfo &Info)
	: m_EventInfo(Info)
	, m_fCurrentTimeSet(false)
{
}


bool CEventVariableStringMap::BeginFormat()
{
	if (!m_fCurrentTimeSet)
		::GetLocalTime(&m_CurrentTime);

	return true;
}


bool CEventVariableStringMap::GetString(LPCWSTR pszKeyword,String *pString)
{
	if (::lstrcmpi(pszKeyword,TEXT("channel-name"))==0) {
		*pString=m_EventInfo.Channel.GetName();
	} else if (::lstrcmpi(pszKeyword,TEXT("channel-no"))==0) {
		StringUtility::Format(*pString,TEXT("%d"),m_EventInfo.Channel.GetChannelNo());
	} else if (::lstrcmpi(pszKeyword,TEXT("channel-no2"))==0) {
		StringUtility::Format(*pString,TEXT("%02d"),m_EventInfo.Channel.GetChannelNo());
	} else if (::lstrcmpi(pszKeyword,TEXT("channel-no3"))==0) {
		StringUtility::Format(*pString,TEXT("%03d"),m_EventInfo.Channel.GetChannelNo());
	} else if (::lstrcmpi(pszKeyword,TEXT("event-name"))==0) {
		*pString=m_EventInfo.Event.m_EventName;
	} else if (::lstrcmpi(pszKeyword,TEXT("event-title"))==0) {
		GetEventTitle(m_EventInfo.Event.m_EventName,pString);
	} else if (::lstrcmpi(pszKeyword,TEXT("event-id"))==0) {
		StringUtility::Format(*pString,TEXT("%04X"),m_EventInfo.Event.m_EventID);
	} else if (::lstrcmpi(pszKeyword,TEXT("service-name"))==0) {
		*pString=m_EventInfo.ServiceName;
	} else if (::lstrcmpi(pszKeyword,TEXT("service-id"))==0) {
		StringUtility::Format(*pString,TEXT("%04X"),m_EventInfo.Event.m_ServiceID);
	} else if (::lstrcmpi(pszKeyword,TEXT("event-duration-hour"))==0) {
		StringUtility::Format(*pString,TEXT("%d"),
							  (int)(m_EventInfo.Event.m_Duration/(60*60)));
	} else if (::lstrcmpi(pszKeyword,TEXT("event-duration-hour2"))==0) {
		StringUtility::Format(*pString,TEXT("%02d"),
							  (int)(m_EventInfo.Event.m_Duration/(60*60)));
	} else if (::lstrcmpi(pszKeyword,TEXT("event-duration-min"))==0) {
		StringUtility::Format(*pString,TEXT("%d"),
							  (int)(m_EventInfo.Event.m_Duration/60%60));
	} else if (::lstrcmpi(pszKeyword,TEXT("event-duration-min2"))==0) {
		StringUtility::Format(*pString,TEXT("%02d"),
							  (int)(m_EventInfo.Event.m_Duration/60%60));
	} else if (::lstrcmpi(pszKeyword,TEXT("event-duration-sec"))==0) {
		StringUtility::Format(*pString,TEXT("%d"),
							  (int)(m_EventInfo.Event.m_Duration%60));
	} else if (::lstrcmpi(pszKeyword,TEXT("event-duration-sec2"))==0) {
		StringUtility::Format(*pString,TEXT("%02d"),
							  (int)(m_EventInfo.Event.m_Duration%60));
	} else if (IsDateTimeParameter(pszKeyword)) {
		return GetTimeString(pszKeyword,m_CurrentTime,pString);
	} else if (::StrCmpNI(pszKeyword,TEXT("start-"),6)==0
			&& IsDateTimeParameter(pszKeyword+6)) {
		if (!m_EventInfo.Event.m_bValidStartTime) {
			*pString=TEXT("x");
			return true;
		}
		return GetTimeString(pszKeyword+6,m_EventInfo.Event.m_StartTime,pString);
	} else if (::StrCmpNI(pszKeyword,TEXT("end-"),4)==0
			&& IsDateTimeParameter(pszKeyword+4)) {
		SYSTEMTIME EndTime;
		if (!m_EventInfo.Event.GetEndTime(&EndTime)) {
			*pString=TEXT("x");
			return true;
		}
		return GetTimeString(pszKeyword+4,EndTime,pString);
	} else if (::StrCmpNI(pszKeyword,TEXT("tot-"),4)==0
			&& IsDateTimeParameter(pszKeyword+4)) {
		return GetTimeString(pszKeyword+4,m_EventInfo.TotTime,pString);
	} else {
		return false;
	}

	return true;
}


bool CEventVariableStringMap::NormalizeString(String *pString) const
{
	// ファイル名に使用できない文字を置き換える
	for (auto i=pString->begin();i!=pString->end();++i) {
		static const struct {
			WCHAR From;
			WCHAR To;
		} CharMap[] = {
			{L'\\',	L'￥'},
			{L'/',	L'／'},
			{L':',	L'：'},
			{L'*',	L'＊'},
			{L'?',	L'？'},
			{L'"',	L'”'},
			{L'<',	L'＜'},
			{L'>',	L'＞'},
			{L'|',	L'｜'},
		};

		for (int j=0;j<lengthof(CharMap);j++) {
			if (CharMap[j].From==*i) {
				*i=CharMap[j].To;
				break;
			}
		}
	}

	return true;
}


bool CEventVariableStringMap::GetParameterInfo(int Index,ParameterInfo *pInfo) const
{
	if (Index<0 || Index>=lengthof(m_ParameterList))
		return false;

	*pInfo=m_ParameterList[Index];

	return true;
}


int CEventVariableStringMap::GetParameterCount() const
{
	return lengthof(m_ParameterList);
}


void CEventVariableStringMap::SetSampleEventInfo()
{
	GetSampleEventInfo(&m_EventInfo);
}


void CEventVariableStringMap::SetCurrentTime(const SYSTEMTIME *pTime)
{
	if (pTime!=nullptr) {
		m_fCurrentTimeSet=true;
		m_CurrentTime=*pTime;
	} else {
		m_fCurrentTimeSet=false;
	}
}


bool CEventVariableStringMap::GetSampleEventInfo(EventInfo *pInfo)
{
	if (pInfo==nullptr)
		return false;

	SYSTEMTIME st;

	::GetLocalTime(&st);
	st.wMilliseconds=0;

	pInfo->Channel.SetSpace(0);
	pInfo->Channel.SetChannelIndex(8);
	pInfo->Channel.SetChannelNo(13);
	pInfo->Channel.SetName(TEXT("アフリカ中央テレビ"));
	pInfo->Channel.SetNetworkID(0x1234);
	pInfo->Channel.SetTransportStreamID(0x1234);
	pInfo->Channel.SetServiceID(123);
	pInfo->ServiceName=TEXT("アフテレ1");
	pInfo->TotTime=st;
	pInfo->Event.m_NetworkID=0x1234;
	pInfo->Event.m_TransportStreamID=0x1234;
	pInfo->Event.m_ServiceID=123;
	pInfo->Event.m_EventID=0xABCD;
	pInfo->Event.m_EventName=TEXT("[二][字]今日のニュース");
	pInfo->Event.m_EventText=TEXT("本日のニュースをお伝えします。");
	pInfo->Event.m_bValidStartTime=true;
	OffsetSystemTime(&st,5*TimeConsts::SYSTEMTIME_MINUTE);
	st.wMinute=st.wMinute/5*5;
	st.wSecond=0;
	pInfo->Event.m_StartTime=st;
	pInfo->Event.m_Duration=60*60;

	return true;
}


void CEventVariableStringMap::GetEventTitle(const String &EventName,String *pTitle)
{
	// 番組名から [字] のようなものを除去する

	pTitle->clear();

	String::size_type Next=0;

	while (Next<EventName.length()) {
		String::size_type Left=EventName.find(L'[',Next);
		if (Left==String::npos) {
			pTitle->append(EventName.substr(Next));
			break;
		}

		String::size_type Right=EventName.find(L']',Left+1);
		if (Right==String::npos) {
			pTitle->append(EventName.substr(Next));
			break;
		}

		if (Left>Next)
			pTitle->append(EventName.substr(Next,Left-Next));

		if (Right-Left>3)
			pTitle->append(EventName.substr(Left,Right-Left));

		Next=Right+1;
	}

	StringUtility::Trim(*pTitle,L" ");
}


}
