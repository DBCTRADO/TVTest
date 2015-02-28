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
	{nullptr,							TEXT("���ݓ���")},
	{TEXT("%date%"),					TEXT("�N����")},
	{TEXT("%year%"),					TEXT("�N")},
	{TEXT("%year2%"),					TEXT("�N(��2��)")},
	{TEXT("%month%"),					TEXT("��")},
	{TEXT("%month2%"),					TEXT("��(2��)")},
	{TEXT("%day%"),						TEXT("��")},
	{TEXT("%day2%"),					TEXT("��(2��)")},
	{TEXT("%time%"),					TEXT("����(��+��+�b)")},
	{TEXT("%hour%"),					TEXT("��")},
	{TEXT("%hour2%"),					TEXT("��(2��)")},
	{TEXT("%minute%"),					TEXT("��")},
	{TEXT("%minute2%"),					TEXT("��(2��)")},
	{TEXT("%second%"),					TEXT("�b")},
	{TEXT("%second2%"),					TEXT("�b(2��)")},
	{TEXT("%day-of-week%"),				TEXT("�j��(����)")},
	{nullptr,							TEXT("�ԑg�J�n����")},
	{TEXT("%start-date%"),				TEXT("�N����")},
	{TEXT("%start-year%"),				TEXT("�N")},
	{TEXT("%start-year2%"),				TEXT("�N(��2��)")},
	{TEXT("%start-month%"),				TEXT("��")},
	{TEXT("%start-month2%"),			TEXT("��(2��)")},
	{TEXT("%start-day%"),				TEXT("��")},
	{TEXT("%start-day2%"),				TEXT("��(2��)")},
	{TEXT("%start-time%"),				TEXT("����(��+��+�b)")},
	{TEXT("%start-hour%"),				TEXT("��")},
	{TEXT("%start-hour2%"),				TEXT("��(2��)")},
	{TEXT("%start-minute%"),			TEXT("��")},
	{TEXT("%start-minute2%"),			TEXT("��(2��)")},
	{TEXT("%start-second%"),			TEXT("�b")},
	{TEXT("%start-second2%"),			TEXT("�b(2��)")},
	{TEXT("%start-day-of-week%"),		TEXT("�j��(����)")},
	{nullptr,							TEXT("�ԑg�I������")},
	{TEXT("%end-date%"),				TEXT("�N����")},
	{TEXT("%end-year%"),				TEXT("�N")},
	{TEXT("%end-year2%"),				TEXT("�N(��2��)")},
	{TEXT("%end-month%"),				TEXT("��")},
	{TEXT("%end-month2%"),				TEXT("��(2��)")},
	{TEXT("%end-day%"),					TEXT("��")},
	{TEXT("%end-day2%"),				TEXT("��(2��)")},
	{TEXT("%end-time%"),				TEXT("����(��+��+�b)")},
	{TEXT("%end-hour%"),				TEXT("��")},
	{TEXT("%end-hour2%"),				TEXT("��(2��)")},
	{TEXT("%end-minute%"),				TEXT("��")},
	{TEXT("%end-minute2%"),				TEXT("��(2��)")},
	{TEXT("%end-second%"),				TEXT("�b")},
	{TEXT("%end-second2%"),				TEXT("�b(2��)")},
	{TEXT("%end-day-of-week%"),			TEXT("�j��(����)")},
	{nullptr,							TEXT("TOT����")},
	{TEXT("%tot-date%"),				TEXT("�N����")},
	{TEXT("%tot-year%"),				TEXT("�N")},
	{TEXT("%tot-year2%"),				TEXT("�N(��2��)")},
	{TEXT("%tot-month%"),				TEXT("��")},
	{TEXT("%tot-month2%"),				TEXT("��(2��)")},
	{TEXT("%tot-day%"),					TEXT("��")},
	{TEXT("%tot-day2%"),				TEXT("��(2��)")},
	{TEXT("%tot-time%"),				TEXT("����(��+��+�b)")},
	{TEXT("%tot-hour%"),				TEXT("��")},
	{TEXT("%tot-hour2%"),				TEXT("��(2��)")},
	{TEXT("%tot-minute%"),				TEXT("��")},
	{TEXT("%tot-minute2%"),				TEXT("��(2��)")},
	{TEXT("%tot-second%"),				TEXT("�b")},
	{TEXT("%tot-second2%"),				TEXT("�b(2��)")},
	{TEXT("%tot-day-of-week%"),			TEXT("�j��(����)")},
	{nullptr,							TEXT("�ԑg�̒���")},
	{TEXT("%event-duration-hour%"),		TEXT("����")},
	{TEXT("%event-duration-hour2%"),	TEXT("����(2��)")},
	{TEXT("%event-duration-min%"),		TEXT("��")},
	{TEXT("%event-duration-min2%"),		TEXT("��(2��)")},
	{TEXT("%event-duration-sec%"),		TEXT("�b")},
	{TEXT("%event-duration-sec2%"),		TEXT("�b(2��)")},
	{nullptr,							nullptr},
	{TEXT("%channel-name%"),			TEXT("�`�����l����")},
	{TEXT("%channel-no%"),				TEXT("�`�����l���ԍ�")},
	{TEXT("%channel-no2%"),				TEXT("�`�����l���ԍ�(2��)")},
	{TEXT("%channel-no3%"),				TEXT("�`�����l���ԍ�(3��)")},
	{TEXT("%event-name%"),				TEXT("�ԑg��")},
	{TEXT("%event-title%"),				TEXT("�ԑg�^�C�g��")},
	{TEXT("%event-id%"),				TEXT("�C�x���gID")},
	{TEXT("%service-name%"),			TEXT("�T�[�r�X��")},
	{TEXT("%service-id%"),				TEXT("�T�[�r�XID")},
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
	// �t�@�C�����Ɏg�p�ł��Ȃ�������u��������
	for (auto i=pString->begin();i!=pString->end();++i) {
		static const struct {
			WCHAR From;
			WCHAR To;
		} CharMap[] = {
			{L'\\',	L'��'},
			{L'/',	L'�^'},
			{L':',	L'�F'},
			{L'*',	L'��'},
			{L'?',	L'�H'},
			{L'"',	L'�h'},
			{L'<',	L'��'},
			{L'>',	L'��'},
			{L'|',	L'�b'},
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
	pInfo->Channel.SetName(TEXT("�A�t���J�����e���r"));
	pInfo->Channel.SetNetworkID(0x1234);
	pInfo->Channel.SetTransportStreamID(0x1234);
	pInfo->Channel.SetServiceID(123);
	pInfo->ServiceName=TEXT("�A�t�e��1");
	pInfo->TotTime=st;
	pInfo->Event.m_NetworkID=0x1234;
	pInfo->Event.m_TransportStreamID=0x1234;
	pInfo->Event.m_ServiceID=123;
	pInfo->Event.m_EventID=0xABCD;
	pInfo->Event.m_EventName=TEXT("[��][��]�����̃j���[�X");
	pInfo->Event.m_EventText=TEXT("�{���̃j���[�X�����`�����܂��B");
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
	// �ԑg������ [��] �̂悤�Ȃ��̂���������

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
