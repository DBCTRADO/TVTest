#include "stdafx.h"
#include "TVTest.h"
#include "ProgramListPanel.h"
#include "DrawUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define ICON_MARGIN_LEFT	1
#define ICON_MARGIN_RIGHT	0
#define ICON_MARGIN_TOP		1
#define ICON_MARGIN			1
#define TEXT_LEFT_MARGIN (CEpgIcons::ICON_WIDTH+ICON_MARGIN_LEFT+ICON_MARGIN_RIGHT)




class CProgramItemInfo
{
public:
	CProgramItemInfo(const CEventInfoData &EventInfo);
	//~CProgramItemInfo();
	const CEventInfoData &GetEventInfo() const { return m_EventInfo; }
	WORD GetEventID() const { return m_EventID; }
	int GetTitleLines() const { return m_NameLines; }
	int GetTextLines() const { return m_TextLines; }
	int GetLines() const { return m_NameLines+m_TextLines; }
	int CalcTitleLines(HDC hdc, int Width);
	int CalcTextLines(HDC hdc,int Width);
	void DrawTitle(HDC hdc,const RECT *pRect,int LineHeight);
	void DrawText(HDC hdc,const RECT *pRect,int LineHeight);
	bool IsChanged(const CProgramItemInfo *pItem) const;

private:
	enum {
		MAX_EVENT_TITLE = 256
	};

	CEventInfoData m_EventInfo;
	WORD m_EventID;
	int m_NameLines;
	int m_TextLines;

	LPCTSTR GetEventText() const;
	void GetEventTitleText(LPTSTR pszText,int MaxLength) const;
};


CProgramItemInfo::CProgramItemInfo(const CEventInfoData &EventInfo)
{
	m_EventInfo=EventInfo;
	m_EventID=EventInfo.m_EventID;
	m_NameLines=0;
	m_TextLines=0;
}


int CProgramItemInfo::CalcTitleLines(HDC hdc,int Width)
{
	TCHAR szText[MAX_EVENT_TITLE];

	GetEventTitleText(szText,lengthof(szText));
	m_NameLines=DrawUtil::CalcWrapTextLines(hdc,szText,Width);
	return m_NameLines;
}


int CProgramItemInfo::CalcTextLines(HDC hdc,int Width)
{
	LPCTSTR pszEventText=GetEventText();

	if (pszEventText!=NULL)
		m_TextLines=DrawUtil::CalcWrapTextLines(hdc,pszEventText,Width);
	else
		m_TextLines=0;
	return m_TextLines;
}


void CProgramItemInfo::DrawTitle(HDC hdc,const RECT *pRect,int LineHeight)
{
	TCHAR szText[MAX_EVENT_TITLE];

	GetEventTitleText(szText,lengthof(szText));
	DrawUtil::DrawWrapText(hdc,szText,pRect,LineHeight);
}


void CProgramItemInfo::DrawText(HDC hdc,const RECT *pRect,int LineHeight)
{
	LPCTSTR pszEventText=GetEventText();
	if (pszEventText!=NULL) {
		DrawUtil::DrawWrapText(hdc,pszEventText,pRect,LineHeight);
	}
}


bool CProgramItemInfo::IsChanged(const CProgramItemInfo *pItem) const
{
	return m_EventID!=pItem->m_EventID
		|| memcmp(&m_EventInfo.m_stStartTime,&pItem->m_EventInfo.m_stStartTime,sizeof(SYSTEMTIME))!=0
		|| m_EventInfo.m_DurationSec!=pItem->m_EventInfo.m_DurationSec;
}


LPCTSTR CProgramItemInfo::GetEventText() const
{
	LPCTSTR pszEventText,p;

	pszEventText=m_EventInfo.GetEventText();
	if (pszEventText!=NULL) {
		p=pszEventText;
		while (*p!='\0') {
			if (*p<=0x20) {
				p++;
				continue;
			}
			return p;
		}
	}
	pszEventText=m_EventInfo.GetEventExtText();
	if (pszEventText!=NULL) {
		p=pszEventText;
		if (memcmp(p,TEXT("番組内容"),4*(3-sizeof(TCHAR)))==0)
			p+=4*(3-sizeof(TCHAR));
		while (*p!='\0') {
			if (*p<=0x20) {
				p++;
				continue;
			}
			return p;
		}
	}
	return pszEventText;
}


void CProgramItemInfo::GetEventTitleText(LPTSTR pszText,int MaxLength) const
{
	TCHAR szTime[EpgUtil::MAX_EVENT_TIME_LENGTH];

	EpgUtil::FormatEventTime(&m_EventInfo,szTime,lengthof(szTime),
							 EpgUtil::EVENT_TIME_HOUR_2DIGITS | EpgUtil::EVENT_TIME_START_ONLY);
	StdUtil::snprintf(pszText,MaxLength,TEXT("%s %s"),
					  szTime,NullToEmptyString(m_EventInfo.GetEventName()));
}




CProgramItemList::CProgramItemList()
	: m_NumItems(0)
	, m_ppItemList(NULL)
	, m_ItemListLength(0)
{
}


CProgramItemList::~CProgramItemList()
{
	Clear();
}


CProgramItemInfo *CProgramItemList::GetItem(int Index)
{
	if (Index<0 || Index>=m_NumItems)
		return NULL;
	return m_ppItemList[Index];
}


const CProgramItemInfo *CProgramItemList::GetItem(int Index) const
{
	if (Index<0 || Index>=m_NumItems)
		return NULL;
	return m_ppItemList[Index];
}


bool CProgramItemList::Add(CProgramItemInfo *pItem)
{
	if (m_NumItems==m_ItemListLength)
		return false;
	m_ppItemList[m_NumItems++]=pItem;
	return true;
}


void CProgramItemList::Clear()
{
	if (m_ppItemList!=NULL) {
		int i;

		for (i=0;i<m_NumItems;i++)
			delete m_ppItemList[i];
		delete [] m_ppItemList;
		m_ppItemList=NULL;
		m_NumItems=0;
		m_ItemListLength=0;
	}
}


void CProgramItemList::SortSub(CProgramItemInfo **ppFirst,CProgramItemInfo **ppLast)
{
	SYSTEMTIME stKey=ppFirst[(ppLast-ppFirst)/2]->GetEventInfo().m_stStartTime;
	CProgramItemInfo **p,**q;

	p=ppFirst;
	q=ppLast;
	while (p<=q) {
		while (CompareSystemTime(&(*p)->GetEventInfo().m_stStartTime,&stKey)<0)
			p++;
		while (CompareSystemTime(&(*q)->GetEventInfo().m_stStartTime,&stKey)>0)
			q--;
		if (p<=q) {
			CProgramItemInfo *pTemp;

			pTemp=*p;
			*p=*q;
			*q=pTemp;
			p++;
			q--;
		}
	}
	if (q>ppFirst)
		SortSub(ppFirst,q);
	if (p<ppLast)
		SortSub(p,ppLast);
}


void CProgramItemList::Sort()
{
	if (m_NumItems>1)
		SortSub(&m_ppItemList[0],&m_ppItemList[m_NumItems-1]);
}


void CProgramItemList::Reserve(int NumItems)
{
	Clear();
	m_ppItemList=new CProgramItemInfo*[NumItems];
	m_ItemListLength=NumItems;
}


void CProgramItemList::Attach(CProgramItemList *pList)
{
	Clear();
	m_NumItems=pList->m_NumItems;
	m_ppItemList=pList->m_ppItemList;
	m_ItemListLength=pList->m_ItemListLength;
	pList->m_NumItems=0;
	pList->m_ppItemList=NULL;
	pList->m_ItemListLength=0;
}




const LPCTSTR CProgramListPanel::m_pszClassName=APP_NAME TEXT(" Program List Panel");
HINSTANCE CProgramListPanel::m_hinst=NULL;


bool CProgramListPanel::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=::LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=m_pszClassName;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CProgramListPanel::CProgramListPanel()
	: m_EventInfoPopupManager(&m_EventInfoPopup)
	, m_EventInfoPopupHandler(this)
	, m_pProgramList(NULL)
	, m_FontHeight(0)
	, m_LineMargin(1)
	, m_TitleMargin(2)
	, m_VisibleEventIcons(((1<<(CEpgIcons::ICON_LAST+1))-1)^CEpgIcons::IconFlag(CEpgIcons::ICON_PAY))
	, m_CurEventID(-1)
	, m_ScrollPos(0)
	//, m_hwndToolTip(NULL)
	, m_fShowRetrievingMessage(false)
{
	m_Theme.EventNameStyle.Gradient.Type=Theme::GRADIENT_NORMAL;
	m_Theme.EventNameStyle.Gradient.Direction=Theme::DIRECTION_VERT;
	m_Theme.EventNameStyle.Gradient.Color1=RGB(0,0,0);
	m_Theme.EventNameStyle.Gradient.Color2=RGB(0,0,0);
	m_Theme.EventNameStyle.Border.Type=Theme::BORDER_NONE;
	m_Theme.EventNameStyle.TextColor=RGB(255,255,255);
	m_Theme.CurEventNameStyle=m_Theme.EventNameStyle;
	m_Theme.EventTextStyle.Gradient.Type=Theme::GRADIENT_NORMAL;
	m_Theme.EventTextStyle.Gradient.Direction=Theme::DIRECTION_VERT;
	m_Theme.EventTextStyle.Gradient.Color1=RGB(128,128,128);
	m_Theme.EventTextStyle.Gradient.Color2=RGB(128,128,128);
	m_Theme.EventTextStyle.Border.Type=Theme::BORDER_NONE;
	m_Theme.EventTextStyle.TextColor=RGB(255,255,255);
	m_Theme.CurEventTextStyle=m_Theme.EventTextStyle;
	m_Theme.MarginColor=RGB(0,0,0);
}


CProgramListPanel::~CProgramListPanel()
{
	Destroy();
}


bool CProgramListPanel::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 m_pszClassName,TEXT("番組表"),m_hinst);
}


bool CProgramListPanel::UpdateProgramList(WORD NetworkID,WORD TransportStreamID,WORD ServiceID)
{
	if (m_pProgramList==NULL)
		return false;
	if (m_hwnd!=NULL) {
		const bool fRetrieving=m_fShowRetrievingMessage;

		m_fShowRetrievingMessage=false;
		if (UpdateListInfo(NetworkID,TransportStreamID,ServiceID)) {
			CalcDimensions();
			SetScrollBar();
			//SetToolTip();
			Invalidate();
		} else if (fRetrieving) {
			Invalidate();
		}
	}
	return true;
}


bool CProgramListPanel::OnProgramListChanged()
{
	/*
	if (m_hwnd!=NULL) {
		if (UpdateListInfo(ServiceID)) {
			CalcDimensions();
			SetScrollBar();
			Invalidate();
		}
	}
	*/
	return true;
}


bool CProgramListPanel::UpdateListInfo(WORD NetworkID,WORD TransportStreamID,WORD ServiceID)
{
	if (m_pProgramList==NULL)
		return false;

	CEpgServiceInfo *pServiceInfo;
	CEventInfoList::EventIterator itrEvent;
	int NumEvents;
	int i,j;
	CProgramItemList NewItemList;
	SYSTEMTIME stFirst,stLast;
	bool fChanged;

	pServiceInfo=m_pProgramList->GetServiceInfo(NetworkID,TransportStreamID,ServiceID);
	if (pServiceInfo==NULL)
		return false;
	NumEvents=(int)pServiceInfo->m_EventList.EventDataMap.size();
	if (NumEvents==0) {
		if (m_ItemList.NumItems()>0) {
			m_ItemList.Clear();
			return true;
		}
		return false;
	}
	NewItemList.Reserve(NumEvents);
	GetCurrentJST(&stFirst);
	stFirst.wSecond=0;
	stFirst.wMilliseconds=0;
	stLast=stFirst;
	OffsetSystemTime(&stLast,24*60*60*1000);
	fChanged=false;
	i=0;
	for (itrEvent=pServiceInfo->m_EventList.EventDataMap.begin();
			itrEvent!=pServiceInfo->m_EventList.EventDataMap.end();++itrEvent) {
		SYSTEMTIME stEnd;

		itrEvent->second.GetEndTime(&stEnd);
		if (CompareSystemTime(&stFirst,&stEnd)<0
				&& CompareSystemTime(&stLast,&itrEvent->second.m_stStartTime)>0) {
			NewItemList.Add(new CProgramItemInfo(itrEvent->second));
			i++;
		}
	}
	if (i>1)
		NewItemList.Sort();
	if (i>m_ItemList.NumItems()) {
		fChanged=true;
	} else {
		for (j=0;j<i;j++) {
			if (m_ItemList.GetItem(j)->IsChanged(NewItemList.GetItem(j))) {
				fChanged=true;
				break;
			}
		}
	}
	if (i==m_ItemList.NumItems() && !fChanged)
		return false;
	m_ItemList.Attach(&NewItemList);
	return true;
}


void CProgramListPanel::ClearProgramList()
{
	if (m_ItemList.NumItems()>0) {
		m_ItemList.Clear();
		m_CurEventID=-1;
		m_ScrollPos=0;
		m_TotalLines=0;
		if (m_hwnd!=NULL) {
			SetScrollBar();
			//SetToolTip();
			Invalidate();
		}
	}
	//if (m_pProgramList!=NULL)
	//	m_pProgramList->Clear();
}


void CProgramListPanel::SetCurrentEventID(int EventID)
{
	m_CurEventID=EventID;
	if (m_hwnd!=NULL)
		Invalidate();
}


void CProgramListPanel::CalcDimensions()
{
	HDC hdc;
	RECT rc;
	HFONT hfontOld;

	hdc=::GetDC(m_hwnd);
	GetClientRect(&rc);
	hfontOld=static_cast<HFONT>(::GetCurrentObject(hdc,OBJ_FONT));
	m_TotalLines=0;
	for (int i=0;i<m_ItemList.NumItems();i++) {
		CProgramItemInfo *pItem=m_ItemList.GetItem(i);

		DrawUtil::SelectObject(hdc,m_TitleFont);
		m_TotalLines+=pItem->CalcTitleLines(hdc,rc.right);
		DrawUtil::SelectObject(hdc,m_Font);
		m_TotalLines+=pItem->CalcTextLines(hdc,rc.right-TEXT_LEFT_MARGIN);
	}
	::SelectObject(hdc,hfontOld);
	::ReleaseDC(m_hwnd,hdc);
}


void CProgramListPanel::SetScrollPos(int Pos)
{
	RECT rc;

	GetClientRect(&rc);
	if (Pos<0) {
		Pos=0;
	} else {
		int Max=m_TotalLines*(m_FontHeight+m_LineMargin)+
				m_ItemList.NumItems()*(m_TitleMargin*2-m_LineMargin)-rc.bottom;
		if (Max<0)
			Max=0;
		if (Pos>Max)
			Pos=Max;
	}
	if (Pos!=m_ScrollPos) {
		int Offset=Pos-m_ScrollPos;
		SCROLLINFO si;

		m_ScrollPos=Pos;
		si.cbSize=sizeof(SCROLLINFO);
		si.fMask=SIF_POS;
		si.nPos=Pos;
		::SetScrollInfo(m_hwnd,SB_VERT,&si,TRUE);
		if (abs(Offset)<rc.bottom) {
			::ScrollWindowEx(m_hwnd,0,-Offset,
							 NULL,NULL,NULL,NULL,SW_ERASE | SW_INVALIDATE);
		} else {
			Invalidate();
		}
		//SetToolTip();
	}
}


void CProgramListPanel::SetScrollBar()
{
	SCROLLINFO si;
	RECT rc;

	si.cbSize=sizeof(SCROLLINFO);
	si.fMask=SIF_PAGE | SIF_RANGE | SIF_POS | SIF_DISABLENOSCROLL;
	si.nMin=0;
	si.nMax=m_TotalLines<1?0:m_TotalLines*(m_FontHeight+m_LineMargin)+
						m_ItemList.NumItems()*(m_TitleMargin*2-m_LineMargin);
	GetClientRect(&rc);
	si.nPage=rc.bottom;
	si.nPos=m_ScrollPos;
	::SetScrollInfo(m_hwnd,SB_VERT,&si,TRUE);
}


/*
void CProgramListPanel:: SetColors(
	const Theme::GradientInfo *pEventBackGradient,COLORREF EventTextColor,
	const Theme::GradientInfo *pCurEventBackGradient,COLORREF CurEventTextColor,
	const Theme::GradientInfo *pTitleBackGradient,COLORREF TitleTextColor,
	const Theme::GradientInfo *pCurTitleBackGradient,COLORREF CurTitleTextColor,
	COLORREF MarginColor)
{
	m_EventBackGradient=*pEventBackGradient;
	m_EventTextColor=EventTextColor;
	m_CurEventBackGradient=*pCurEventBackGradient;
	m_CurEventTextColor=CurEventTextColor;
	m_TitleBackGradient=*pTitleBackGradient;
	m_TitleTextColor=TitleTextColor;
	m_CurTitleBackGradient=*pCurTitleBackGradient;
	m_CurTitleTextColor=CurTitleTextColor;
	m_MarginColor=MarginColor;
	if (m_hwnd!=NULL)
		Invalidate();
}
*/


bool CProgramListPanel::SetTheme(const ThemeInfo *pTheme)
{
	if (pTheme==NULL)
		return false;
	m_Theme=*pTheme;
	if (m_hwnd!=NULL)
		Invalidate();
	return true;
}


bool CProgramListPanel::GetTheme(ThemeInfo *pTheme) const
{
	if (pTheme==NULL)
		return false;
	*pTheme=m_Theme;
	return true;
}


bool CProgramListPanel::SetFont(const LOGFONT *pFont)
{
	if (!m_Font.Create(pFont))
		return false;
	LOGFONT lf=*pFont;
	lf.lfWeight=FW_BOLD;
	m_TitleFont.Create(&lf);
	m_ScrollPos=0;
	if (m_hwnd!=NULL) {
		CalcFontHeight();
		CalcDimensions();
		SetScrollBar();
		//SetToolTip();
		Invalidate();
	}
	return true;
}


bool CProgramListPanel::SetEventInfoFont(const LOGFONT *pFont)
{
	return m_EventInfoPopup.SetFont(pFont);
}


void CProgramListPanel::CalcFontHeight()
{
	HDC hdc;

	hdc=::GetDC(m_hwnd);
	if (hdc==NULL)
		return;
	m_FontHeight=m_Font.GetHeight();
	::ReleaseDC(m_hwnd,hdc);
}


void CProgramListPanel::ShowRetrievingMessage(bool fShow)
{
	if (m_fShowRetrievingMessage!=fShow) {
		m_fShowRetrievingMessage=fShow;
		if (m_hwnd!=NULL)
			Invalidate();
	}
}


void CProgramListPanel::SetVisibleEventIcons(UINT VisibleIcons)
{
	if (m_VisibleEventIcons!=VisibleIcons) {
		m_VisibleEventIcons=VisibleIcons;
		if (m_hwnd!=NULL)
			Invalidate();
	}
}


int CProgramListPanel::HitTest(int x,int y) const
{
	POINT pt;
	RECT rc;

	pt.x=x;
	pt.y=y;
	GetClientRect(&rc);
	if (!::PtInRect(&rc,pt))
		return -1;
	rc.top=-m_ScrollPos;
	for (int i=0;i<m_ItemList.NumItems();i++) {
		const CProgramItemInfo *pItem=m_ItemList.GetItem(i);

		rc.bottom=rc.top+(pItem->GetTitleLines()+pItem->GetTextLines())*(m_FontHeight+m_LineMargin)+
			(m_TitleMargin*2-m_LineMargin);
		if (::PtInRect(&rc,pt))
			return i;
		rc.top=rc.bottom;
	}
	return -1;
}


/*
void CProgramListPanel::SetToolTip()
{
	if (m_hwndToolTip!=NULL) {
		int NumTools=::SendMessage(m_hwndToolTip,TTM_GETTOOLCOUNT,0,0);
		int NumItems=m_ItemList.NumItems();
		TOOLINFO ti;

		ti.cbSize=TTTOOLINFOA_V2_SIZE;
		ti.hwnd=m_hwnd;
		if (NumTools<NumItems) {
			ti.uFlags=TTF_SUBCLASS;
			ti.hinst=NULL;
			ti.lpszText=LPSTR_TEXTCALLBACK;
			::SetRect(&ti.rect,0,0,0,0);
			for (int i=NumTools;i<NumItems;i++) {
				ti.uId=i;
				ti.lParam=i;
				::SendMessage(m_hwndToolTip,TTM_ADDTOOL,0,(LPARAM)&ti);
			}
		} else if (NumTools>NumItems) {
			for (int i=NumItems;i<NumTools;i++) {
				ti.uId=i;
				::SendMessage(m_hwndToolTip,TTM_DELTOOL,0,(LPARAM)&ti);
			}
		}
		GetClientRect(&ti.rect);
		ti.rect.top=-m_ScrollPos;
		ti.uId=0;
		for (int i=0;i<NumItems;i++) {
			const CProgramItemInfo *pItem=m_ItemList.GetItem(i);

			ti.rect.bottom=ti.rect.top+(pItem->GetTitleLines()+pItem->GetTextLines())*(m_FontHeight+m_LineMargin)+
				(m_TitleMargin*2-m_LineMargin);
			::SendMessage(m_hwndToolTip,TTM_NEWTOOLRECT,0,(LPARAM)&ti);
			ti.uId++;
			ti.rect.top=ti.rect.bottom;
		}
	}
}
*/


LRESULT CProgramListPanel::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			if (!m_Font.IsCreated()) {
				LOGFONT lf;
				GetDefaultFont(&lf);
				m_Font.Create(&lf);
				lf.lfWeight=FW_BOLD;
				m_TitleFont.Create(&lf);
			}
			CalcFontHeight();
			m_EpgIcons.Load();
			/*
			m_hwndToolTip=::CreateWindowEx(WS_EX_TOPMOST,TOOLTIPS_CLASS,NULL,
				WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,0,0,0,0,
				hwnd,NULL,m_hinst,NULL);
			::SendMessage(m_hwndToolTip,TTM_SETMAXTIPWIDTH,0,320);
			::SendMessage(m_hwndToolTip,TTM_SETDELAYTIME,TTDT_AUTOPOP,30000);
			*/
			m_EventInfoPopupManager.Initialize(hwnd,&m_EventInfoPopupHandler);
		}
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			BeginPaint(hwnd,&ps);
			DrawProgramList(ps.hdc,&ps.rcPaint);
			EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_SIZE:
		CalcDimensions();
		SetScrollBar();
		//SetToolTip();
		return 0;

	case WM_MOUSEWHEEL:
		SetScrollPos(m_ScrollPos-
			GET_WHEEL_DELTA_WPARAM(wParam)*(m_FontHeight+m_LineMargin)/WHEEL_DELTA);
		return 0;

	case WM_VSCROLL:
		{
			const int LineHeight=m_FontHeight+m_LineMargin;
			int Pos,Page,Max;
			RECT rc;

			Pos=m_ScrollPos;
			GetClientRect(&rc);
			Page=rc.bottom;
			Max=m_TotalLines*LineHeight+
				m_ItemList.NumItems()*(m_TitleMargin*2-m_LineMargin)-Page;
			if (Max<0)
				Max=0;
			switch (LOWORD(wParam)) {
			case SB_LINEUP:		Pos-=LineHeight;	break;
			case SB_LINEDOWN:	Pos+=LineHeight;	break;
			case SB_PAGEUP:		Pos-=Page;			break;
			case SB_PAGEDOWN:	Pos+=Page;			break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:	Pos=HIWORD(wParam);	break;
			case SB_TOP:		Pos=0;				break;
			case SB_BOTTOM:		Pos=Max;			break;
			default:	return 0;
			}
			SetScrollPos(Pos);
		}
		return 0;

	case WM_LBUTTONDOWN:
		SetFocus(hwnd);
		return 0;

#if 0	// テキストが長過ぎてツールチップを使うと問題がある
	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case TTN_NEEDTEXT:
			{
				LPNMTTDISPINFO pnmtdi=reinterpret_cast<LPNMTTDISPINFO>(lParam);
				const CProgramItemInfo *pItem=m_ItemList.GetItem((int)pnmtdi->lParam);

				if (pItem!=NULL) {
					static TCHAR szText[1024];
					const CEventInfoData &EventInfo=pItem->GetEventInfo();
					TCHAR szEndTime[16];
					SYSTEMTIME stEnd;
					if (EventInfo.m_DurationSec>0 && EventInfo.GetEndTime(&stEnd))
						StdUtil::snprintf(szEndTime,lengthof(szEndTime),
										  TEXT("～%d:%02d"),stEnd.wHour,stEnd.wMinute);
					else
						szEndTime[0]='\0';
					StdUtil::snprintf(szText,lengthof(szText),
						TEXT("%d/%d(%s) %d:%02d%s\n%s\n\n%s%s%s%s"),
						EventInfo.m_stStartTime.wMonth,
						EventInfo.m_stStartTime.wDay,
						GetDayOfWeekText(EventInfo.m_stStartTime.wDayOfWeek),
						EventInfo.m_stStartTime.wHour,
						EventInfo.m_stStartTime.wMinute,
						szEndTime,
						NullToEmptyString(EventInfo.GetEventName()),
						NullToEmptyString(EventInfo.GetEventText()),
						EventInfo.GetEventText()!=NULL?TEXT("\n\n"):TEXT(""),
						NullToEmptyString(EventInfo.GetEventExtText()),
						EventInfo.GetEventExtText()!=NULL?TEXT("\n\n"):TEXT(""));
					pnmtdi->lpszText=szText;
				} else {
					pnmtdi->lpszText=TEXT("");
				}
				pnmtdi->szText[0]='\0';
				pnmtdi->hinst=NULL;
			}
			return 0;

		case TTN_SHOW:
			{
				// ツールチップの位置がカーソルと重なっていると
				// 出たり消えたりを繰り返しておかしくなるのでずらす
				LPNMHDR pnmh=reinterpret_cast<LPNMHDR>(lParam);
				RECT rcTip;
				POINT pt;

				::GetWindowRect(pnmh->hwndFrom,&rcTip);
				::GetCursorPos(&pt);
				if (::PtInRect(&rcTip,pt)) {
					HMONITOR hMonitor=::MonitorFromRect(&rcTip,MONITOR_DEFAULTTONEAREST);
					if (hMonitor!=NULL) {
						MONITORINFO mi;

						mi.cbSize=sizeof(mi);
						if (::GetMonitorInfo(hMonitor,&mi)) {
							if (rcTip.left<=mi.rcMonitor.left+16)
								rcTip.left=pt.x+16;
							else if (rcTip.right>=mi.rcMonitor.right-16)
								rcTip.left=pt.x-(rcTip.right-rcTip.left)-8;
							else
								break;
							::SetWindowPos(pnmh->hwndFrom,HWND_TOPMOST,
										   rcTip.left,rcTip.top,0,0,
										   SWP_NOSIZE | SWP_NOACTIVATE);
							return TRUE;
						}
					}
				}
			}
			break;
		}
		break;
#endif

	case WM_DESTROY:
		m_EpgIcons.Destroy();
		//m_hwndToolTip=NULL;
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


void CProgramListPanel::DrawProgramList(HDC hdc,const RECT *prcPaint)
{
	const int LineHeight=m_FontHeight+m_LineMargin;
	RECT rc,rcMargin;

	HBRUSH hbr=::CreateSolidBrush(m_Theme.MarginColor);
	HFONT hfontOld=static_cast<HFONT>(::GetCurrentObject(hdc,OBJ_FONT));
	COLORREF crOldTextColor=::GetTextColor(hdc);
	int OldBkMode=::SetBkMode(hdc,TRANSPARENT);

	HDC hdcIcons=::CreateCompatibleDC(hdc);
	HBITMAP hbmOld=DrawUtil::SelectObject(hdcIcons,m_EpgIcons);

	GetClientRect(&rc);

	if (m_fShowRetrievingMessage && m_ItemList.NumItems()==0) {
		::FillRect(hdc,prcPaint,hbr);
		DrawUtil::SelectObject(hdc,m_Font);
		::SetTextColor(hdc,m_Theme.EventTextStyle.TextColor);
		::InflateRect(&rc,-m_TitleMargin,-m_TitleMargin);
		DrawUtil::DrawWrapText(hdc,TEXT("番組表の取得中です..."),&rc,LineHeight);
	} else {
		rc.top=-m_ScrollPos;
		for (int i=0;i<m_ItemList.NumItems();i++) {
			CProgramItemInfo *pItem=m_ItemList.GetItem(i);
			bool fCur=pItem->GetEventID()==m_CurEventID;

			rc.bottom=rc.top+pItem->GetTitleLines()*LineHeight+
													(m_TitleMargin*2-m_LineMargin);
			if (rc.bottom>prcPaint->top) {
				const Theme::Style &Style=
					fCur?m_Theme.CurEventNameStyle:m_Theme.EventNameStyle;

				DrawUtil::SelectObject(hdc,m_TitleFont);
				::SetTextColor(hdc,Style.TextColor);
				rc.left=0;
				Theme::DrawStyleBackground(hdc,&rc,&Style);
				RECT rcTitle=rc;
				rcTitle.top+=m_TitleMargin;
				rcTitle.bottom-=m_TitleMargin;
				pItem->DrawTitle(hdc,&rcTitle,LineHeight);
			}

			rc.top=rc.bottom;
			rc.bottom=rc.top+pItem->GetTextLines()*LineHeight;
			if (rc.bottom>prcPaint->top) {
				const Theme::Style &Style=
					fCur?m_Theme.CurEventTextStyle:m_Theme.EventTextStyle;

				/*
				rc.left=TEXT_LEFT_MARGIN;
				if (prcPaint->left<rc.left) {
					rcMargin.left=prcPaint->left;
					rcMargin.top=rc.top;
					rcMargin.right=min(rc.left,prcPaint->right);
					rcMargin.bottom=rc.bottom;
					::FillRect(hdc,&rcMargin,hbr);
				}
				*/
				DrawUtil::SelectObject(hdc,m_Font);
				::SetTextColor(hdc,Style.TextColor);
				rc.left=0;
				Theme::DrawStyleBackground(hdc,&rc,&Style);
				rc.left=TEXT_LEFT_MARGIN;
				pItem->DrawText(hdc,&rc,LineHeight);

				const unsigned int ShowIcons=
					CEpgIcons::GetEventIcons(&pItem->GetEventInfo()) & m_VisibleEventIcons;
				if (ShowIcons!=0) {
					int y=rc.top+ICON_MARGIN_TOP;
					int Icon=0;
					for (unsigned int Flag=ShowIcons;Flag!=0;Flag>>=1) {
						if (y>=rc.bottom)
							break;
						if ((Flag&1)!=0) {
							CEpgIcons::Draw(hdc,ICON_MARGIN_LEFT,y,
											hdcIcons,Icon,
											CEpgIcons::ICON_WIDTH,
											min(CEpgIcons::ICON_HEIGHT,rc.bottom-y),
											192);
							y+=CEpgIcons::ICON_HEIGHT+ICON_MARGIN;
						}
						Icon++;
					}
				}
			}

			rc.top=rc.bottom;
			if (rc.top>=prcPaint->bottom)
				break;
		}

		if (rc.top<prcPaint->bottom) {
			rcMargin.left=prcPaint->left;
			rcMargin.top=max(rc.top,prcPaint->top);
			rcMargin.right=prcPaint->right;
			rcMargin.bottom=prcPaint->bottom;
			::FillRect(hdc,&rcMargin,hbr);
		}
	}

	::SelectObject(hdcIcons,hbmOld);
	::DeleteDC(hdcIcons);

	::SetTextColor(hdc,crOldTextColor);
	::SetBkMode(hdc,OldBkMode);
	::SelectObject(hdc,hfontOld);
	::DeleteObject(hbr);
}


CProgramListPanel::CEventInfoPopupHandler::CEventInfoPopupHandler(CProgramListPanel *pPanel)
	: m_pPanel(pPanel)
{
}


bool CProgramListPanel::CEventInfoPopupHandler::HitTest(int x,int y,LPARAM *pParam)
{
	int Program=m_pPanel->HitTest(x,y);

	if (Program>=0) {
		*pParam=Program;
		return true;
	}
	return false;
}


bool CProgramListPanel::CEventInfoPopupHandler::GetEventInfo(LPARAM Param,const CEventInfoData **ppInfo)
{
	const CProgramItemInfo *pItem=m_pPanel->m_ItemList.GetItem((int)Param);
	if (pItem==NULL)
		return false;
	*ppInfo=&pItem->GetEventInfo();
	return true;
}
