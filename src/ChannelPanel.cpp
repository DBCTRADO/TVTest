#include "stdafx.h"
#include "TVTest.h"
#include "ChannelPanel.h"
#include "StdUtil.h"
#include "DrawUtil.h"
#include "EpgUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define CHEVRON_WIDTH	10
#define CHEVRON_HEIGHT	10


const LPCTSTR CChannelPanel::m_pszClassName=APP_NAME TEXT(" Channel Panel");
HINSTANCE CChannelPanel::m_hinst=NULL;


bool CChannelPanel::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=NULL;
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=m_pszClassName;
		if (RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CChannelPanel::CChannelPanel()
	: m_EventInfoPopupManager(&m_EventInfoPopup)
	, m_EventInfoPopupHandler(this)
	, m_pProgramList(NULL)
	, m_FontHeight(0)
	, m_EventNameLines(2)
	, m_ChannelNameMargins(2,2,2,2)
	, m_EventNameMargins(8,1,2,1)
	, m_ChannelChevronMargin(12)
	, m_ChannelNameHeight(0)
	, m_EventNameHeight(0)
	, m_ItemHeight(0)
	, m_ExpandedItemHeight(0)

	, m_EventsPerChannel(2)
	, m_ExpandAdditionalEvents(4)
	, m_ExpandEvents(m_EventsPerChannel+m_ExpandAdditionalEvents)
	, m_ScrollPos(0)
	, m_CurChannel(-1)
	, m_fScrollToCurChannel(false)
	, m_pEventHandler(NULL)
	, m_fDetailToolTip(false)
	, m_pLogoManager(NULL)
{
	LOGFONT lf;
	GetDefaultFont(&lf);
	m_Font.Create(&lf);
	lf.lfWeight=FW_BOLD;
	m_ChannelFont.Create(&lf);

	::ZeroMemory(&m_UpdatedTime,sizeof(SYSTEMTIME));

	m_Theme.ChannelNameStyle.Gradient.Type=Theme::GRADIENT_NORMAL;
	m_Theme.ChannelNameStyle.Gradient.Direction=Theme::DIRECTION_VERT;
	m_Theme.ChannelNameStyle.Gradient.Color1=RGB(128,128,128);
	m_Theme.ChannelNameStyle.Gradient.Color2=RGB(128,128,128);
	m_Theme.ChannelNameStyle.Border.Type=Theme::BORDER_NONE;
	m_Theme.ChannelNameStyle.TextColor=RGB(255,255,255);
	m_Theme.CurChannelNameStyle=m_Theme.ChannelNameStyle;
	m_Theme.EventStyle[0].Gradient.Type=Theme::GRADIENT_NORMAL;
	m_Theme.EventStyle[0].Gradient.Direction=Theme::DIRECTION_VERT;
	m_Theme.EventStyle[0].Gradient.Color1=RGB(0,0,0);
	m_Theme.EventStyle[0].Gradient.Color2=RGB(0,0,0);
	m_Theme.EventStyle[0].Border.Type=Theme::BORDER_NONE;
	m_Theme.EventStyle[0].TextColor=RGB(255,255,255);
	m_Theme.EventStyle[1]=m_Theme.EventStyle[0];
	m_Theme.CurChannelEventStyle[0]=m_Theme.EventStyle[0];
	m_Theme.CurChannelEventStyle[1]=m_Theme.CurChannelEventStyle[0];
	m_Theme.MarginColor=RGB(0,0,0);
}


CChannelPanel::~CChannelPanel()
{
	Destroy();
	ClearChannels();
}


bool CChannelPanel::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 m_pszClassName,TEXT("チャンネル"),m_hinst);
}


bool CChannelPanel::ReadSettings(CSettings &Settings)
{
	bool f;
	if (Settings.Read(TEXT("ChannelPanelDetailToolTip"),&f))
		SetDetailToolTip(f);

	int EventsPerChannel=m_EventsPerChannel;
	int AdditionalEvents=m_ExpandAdditionalEvents;
	if (Settings.Read(TEXT("ChannelPanelEventsPerChannel"),&EventsPerChannel)
			|| Settings.Read(TEXT("ChannelPanelExpandEvents"),&AdditionalEvents))
		SetEventsPerChannel(EventsPerChannel,AdditionalEvents);

	Settings.Read(TEXT("ChannelPanelScrollToCurChannel"),&m_fScrollToCurChannel);

	return true;
}


bool CChannelPanel::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("ChannelPanelDetailToolTip"),m_fDetailToolTip);
	Settings.Write(TEXT("ChannelPanelEventsPerChannel"),m_EventsPerChannel);
	Settings.Write(TEXT("ChannelPanelExpandEvents"),m_ExpandAdditionalEvents);
	Settings.Write(TEXT("ChannelPanelScrollToCurChannel"),m_fScrollToCurChannel);

	return true;
}


bool CChannelPanel::SetEpgProgramList(CEpgProgramList *pList)
{
	m_pProgramList=pList;
	return true;
}


void CChannelPanel::ClearChannels()
{
	for (auto it=m_ChannelList.begin();it!=m_ChannelList.end();++it)
		delete *it;
	m_ChannelList.clear();
}


bool CChannelPanel::UpdateEvents(CChannelEventInfo *pInfo,const SYSTEMTIME *pTime)
{
	const WORD NetworkID=pInfo->GetNetworkID();
	const WORD TransportStreamID=pInfo->GetTransportStreamID();
	const WORD ServiceID=pInfo->GetServiceID();
	SYSTEMTIME st;
	CEventInfoData EventInfo;
	bool fChanged=false;

	if (pTime!=NULL)
		st=*pTime;
	else
		GetCurrentJST(&st);
	const int NumEvents=pInfo->IsExpanded()?m_ExpandEvents:m_EventsPerChannel;
	for (int i=0;i<NumEvents;i++) {
		if (m_pProgramList->GetEventInfo(NetworkID,TransportStreamID,ServiceID,&st,&EventInfo)) {
			if (pInfo->SetEventInfo(i,&EventInfo))
				fChanged=true;
		} else {
			if (i==0) {
				if (pInfo->SetEventInfo(0,NULL))
					fChanged=true;
				if (NumEvents==1)
					break;
				i++;
			}
			if (m_pProgramList->GetNextEventInfo(NetworkID,TransportStreamID,ServiceID,&st,&EventInfo)
					&& DiffSystemTime(&st,&EventInfo.m_stStartTime)<8*60*60*1000) {
				if (pInfo->SetEventInfo(i,&EventInfo))
					fChanged=true;
			} else {
				for (;i<m_EventsPerChannel;i++) {
					if (pInfo->SetEventInfo(i,NULL))
						fChanged=true;
				}
				break;
			}
		}
		EventInfo.GetEndTime(&st);
	}
	return fChanged;
}


bool CChannelPanel::SetChannelList(const CChannelList *pChannelList,bool fSetEvent)
{
	ClearChannels();

	m_ScrollPos=0;
	m_CurChannel=-1;

	if (pChannelList!=NULL) {
		SYSTEMTIME stCurrent;

		GetCurrentJST(&stCurrent);
		for (int i=0;i<pChannelList->NumChannels();i++) {
			const CChannelInfo *pChInfo=pChannelList->GetChannelInfo(i);

			if (!pChInfo->IsEnabled())
				continue;

			CChannelEventInfo *pEventInfo=new CChannelEventInfo(pChInfo,i);

			if (fSetEvent && m_pProgramList!=NULL)
				UpdateEvents(pEventInfo,&stCurrent);
			if (m_pLogoManager!=NULL) {
				HBITMAP hbmLogo=m_pLogoManager->GetAssociatedLogoBitmap(
					pEventInfo->GetNetworkID(),pEventInfo->GetServiceID(),
					CLogoManager::LOGOTYPE_SMALL);
				if (hbmLogo!=NULL)
					pEventInfo->SetLogo(hbmLogo);
			}
			m_ChannelList.push_back(pEventInfo);
		}
	}

	if (m_hwnd!=NULL) {
		SetScrollBar();
		SetTooltips();
		Invalidate();
		Update();
	}

	return true;
}


bool CChannelPanel::UpdateAllChannels(bool fUpdateProgramList)
{
	if (m_pProgramList!=NULL && !m_ChannelList.empty()) {
		bool fChanged=false;

		GetCurrentJST(&m_UpdatedTime);
		for (size_t i=0;i<m_ChannelList.size();i++) {
			CChannelEventInfo *pInfo=m_ChannelList[i];

			if (fUpdateProgramList) {
				m_pProgramList->UpdateService(pInfo->GetNetworkID(),
											  pInfo->GetTransportStreamID(),
											  pInfo->GetServiceID());
			}
			if (UpdateEvents(pInfo,&m_UpdatedTime))
				fChanged=true;
		}
		if (m_hwnd!=NULL && fChanged) {
			Redraw();
		}
	}
	return true;
}


bool CChannelPanel::UpdateChannel(int ChannelIndex)
{
	if (m_pProgramList!=NULL) {
		for (int i=0;i<(int)m_ChannelList.size();i++) {
			CChannelEventInfo *pEventInfo=m_ChannelList[i];

			if (pEventInfo->GetOriginalChannelIndex()==ChannelIndex) {
				if (UpdateEvents(pEventInfo) && m_hwnd!=NULL) {
					RECT rc;

					GetItemRect(i,&rc);
					Invalidate(&rc);
					Update();
				}
				return true;
			}
		}
	}
	return false;
}


bool CChannelPanel::UpdateChannels(WORD NetworkID,WORD TransportStreamID)
{
	if (NetworkID==0 && TransportStreamID==0)
		return false;
	if (m_pProgramList==NULL)
		return false;

	SYSTEMTIME st;
	bool fChanged=false;

	GetCurrentJST(&st);
	for (size_t i=0;i<m_ChannelList.size();i++) {
		CChannelEventInfo *pInfo=m_ChannelList[i];

		if ((NetworkID==0 || pInfo->GetNetworkID()==NetworkID)
				&& (TransportStreamID==0 || pInfo->GetTransportStreamID()==TransportStreamID)) {
			if (UpdateEvents(pInfo,&st))
				fChanged=true;
		}
	}
	if (m_hwnd!=NULL && fChanged) {
		Redraw();
	}
	return true;
}


bool CChannelPanel::IsChannelListEmpty() const
{
	return m_ChannelList.empty();
}


bool CChannelPanel::SetCurrentChannel(int CurChannel)
{
	if (CurChannel<-1)
		return false;

	m_CurChannel=CurChannel;

	if (m_hwnd!=NULL) {
		if (m_fScrollToCurChannel && m_CurChannel>=0)
			ScrollToCurrentChannel();
		Invalidate();
	}

	return true;
}


bool CChannelPanel::ScrollToChannel(int Channel)
{
	if (Channel<0 || (size_t)Channel>=m_ChannelList.size())
		return false;

	RECT rcClient,rcItem;
	GetClientRect(&rcClient);
	GetItemRect(Channel,&rcItem);
	if (rcItem.top>=rcClient.top && rcItem.bottom<=rcClient.bottom)
		return true;
	bool fBottom=rcItem.bottom>rcClient.bottom;
	if (m_ScrollPos!=0)
		::OffsetRect(&rcItem,0,m_ScrollPos);
	int Pos=rcItem.top;
	if (fBottom) {
		Pos-=rcClient.bottom-(rcItem.bottom-rcItem.top);
		if (Pos>rcItem.top)
			Pos=rcItem.top;
	}
	SetScrollPos(Pos);

	return true;
}


bool CChannelPanel::ScrollToCurrentChannel()
{
	for (int i=0;i<(int)m_ChannelList.size();i++) {
		if (m_ChannelList[i]->GetOriginalChannelIndex()==m_CurChannel) {
			return ScrollToChannel(i);
		}
	}

	return false;
}


void CChannelPanel::SetEventHandler(CEventHandler *pEventHandler)
{
	m_pEventHandler=pEventHandler;
}


bool CChannelPanel::SetTheme(const ThemeInfo *pTheme)
{
	if (pTheme==NULL)
		return false;
	m_Theme=*pTheme;
	if (m_hwnd!=NULL)
		Invalidate();
	return true;
}


bool CChannelPanel::GetTheme(ThemeInfo *pTheme) const
{
	if (pTheme==NULL)
		return false;
	*pTheme=m_Theme;
	return true;
}


bool CChannelPanel::SetFont(const LOGFONT *pFont)
{
	if (!m_Font.Create(pFont))
		return false;
	LOGFONT lf=*pFont;
	lf.lfWeight=FW_BOLD;
	m_ChannelFont.Create(&lf);
	if (m_hwnd!=NULL) {
		CalcItemHeight();
		m_ScrollPos=0;
		SetScrollBar();
		Invalidate();
	}
	return true;
}


bool CChannelPanel::SetEventInfoFont(const LOGFONT *pFont)
{
	return m_EventInfoPopup.SetFont(pFont);
}


void CChannelPanel::SetDetailToolTip(bool fDetail)
{
	if (m_fDetailToolTip!=fDetail) {
		m_fDetailToolTip=fDetail;
		if (m_hwnd!=NULL) {
			if (fDetail) {
				m_Tooltip.Destroy();
				m_EventInfoPopupManager.Initialize(m_hwnd,&m_EventInfoPopupHandler);
			} else {
				m_EventInfoPopupManager.Finalize();
				CreateTooltip();
			}
		}
	}
}


bool CChannelPanel::SetEventsPerChannel(int Events,int Expand)
{
	if (Events>0 && Events>MAX_EVENTS_PER_CHANNEL)
		return false;
	if (Expand>0 && Expand>MAX_EXPAND_EVENTS)
		return false;
	if ((Events>0 && m_EventsPerChannel!=Events)
			|| (Expand>0 && m_ExpandAdditionalEvents!=Expand)) {
		if (Events>0)
			m_EventsPerChannel=Events;
		if (Expand>0)
			m_ExpandAdditionalEvents=Expand;
		m_ExpandEvents=m_EventsPerChannel+m_ExpandAdditionalEvents;
		for (size_t i=0;i<m_ChannelList.size();i++) {
			CChannelEventInfo *pInfo=m_ChannelList[i];

			pInfo->SetMaxEvents(pInfo->IsExpanded()?m_ExpandEvents:m_EventsPerChannel);
		}
		if (m_hwnd!=NULL)
			CalcItemHeight();
		UpdateAllChannels(false);
		if (m_hwnd!=NULL) {
			m_ScrollPos=0;
			SetScrollBar();
			Invalidate();
			SetTooltips();
		}
	}
	return true;
}


bool CChannelPanel::ExpandChannel(int Channel,bool fExpand)
{
	if (Channel<0 || (size_t)Channel>=m_ChannelList.size())
		return false;
	CChannelEventInfo *pInfo=m_ChannelList[Channel];
	if (pInfo->IsExpanded()!=fExpand) {
		pInfo->Expand(fExpand);
		pInfo->SetMaxEvents(fExpand?m_ExpandEvents:m_EventsPerChannel);
		UpdateEvents(pInfo);
		if (m_hwnd!=NULL) {
			RECT rcClient,rc;

			GetClientRect(&rcClient);
			GetItemRect(Channel,&rc);
			if (fExpand) {
				rc.bottom=rcClient.bottom;
				Invalidate(&rc);
			} else {
				int Height=CalcHeight();

				if (m_ScrollPos>Height-rcClient.bottom) {
					m_ScrollPos=max(Height-rcClient.bottom,0);
					Invalidate();
				} else {
					rc.bottom=rcClient.bottom;
					Invalidate(&rc);
				}
			}
			SetScrollBar();
			SetTooltips();
		}
	}
	return true;
}


void CChannelPanel::SetScrollToCurChannel(bool fScroll)
{
	m_fScrollToCurChannel=fScroll;
}


void CChannelPanel::SetLogoManager(CLogoManager *pLogoManager)
{
	m_pLogoManager=pLogoManager;
}


bool CChannelPanel::QueryUpdate() const
{
	SYSTEMTIME st;

	GetCurrentJST(&st);
	return m_UpdatedTime.wMinute!=st.wMinute
		|| m_UpdatedTime.wHour!=st.wHour
		|| m_UpdatedTime.wDay!=st.wDay
		|| m_UpdatedTime.wMonth!=st.wMonth
		|| m_UpdatedTime.wYear!=st.wYear;
}


LRESULT CChannelPanel::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CalcItemHeight();
			m_ScrollPos=0;
			if (m_fDetailToolTip)
				m_EventInfoPopupManager.Initialize(hwnd,&m_EventInfoPopupHandler);
			else
				CreateTooltip();
			m_Chevron.Load(m_hinst,IDB_CHEVRON);
		}
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			BeginPaint(hwnd,&ps);
			Draw(ps.hdc,&ps.rcPaint);
			EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_SIZE:
		{
			int Height=HIWORD(lParam),Max;
			int TotalHeight=CalcHeight();

			Max=max(TotalHeight-Height,0);
			if (m_ScrollPos>Max) {
				m_ScrollPos=Max;
				Invalidate();
				SetTooltips(true);
			}
			SetScrollBar();
		}
		return 0;

	case WM_MOUSEWHEEL:
		SetScrollPos(m_ScrollPos-GET_WHEEL_DELTA_WPARAM(wParam)*m_FontHeight/WHEEL_DELTA);
		return 0;

	case WM_VSCROLL:
		{
			int Height=CalcHeight();
			int Pos,Page;
			RECT rc;

			Pos=m_ScrollPos;
			GetClientRect(&rc);
			Page=rc.bottom;
			switch (LOWORD(wParam)) {
			case SB_LINEUP:		Pos-=m_FontHeight;		break;
			case SB_LINEDOWN:	Pos+=m_FontHeight;		break;
			case SB_PAGEUP:		Pos-=Page;				break;
			case SB_PAGEDOWN:	Pos+=Page;				break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:	Pos=HIWORD(wParam);		break;
			case SB_TOP:		Pos=0;					break;
			case SB_BOTTOM:		Pos=max(Height-Page,0);	break;
			default:	return 0;
			}
			SetScrollPos(Pos);
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			HitType Type;
			int Channel;

			SetFocus(hwnd);
			Channel=HitTest(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam),&Type);
			if (Channel>=0) {
				if (Type==HIT_CHEVRON)
					ExpandChannel(Channel,!m_ChannelList[Channel]->IsExpanded());
				else if (Type!=HIT_MARGIN
						&& m_pEventHandler!=NULL)
					m_pEventHandler->OnChannelClick(&m_ChannelList[Channel]->GetChannelInfo());
			}
		}
		return 0;

	case WM_RBUTTONDOWN:
		if (m_pEventHandler!=NULL)
			m_pEventHandler->OnRButtonDown();
		return 0;

	case WM_MOUSEMOVE:
		{
			HitType Type;
			int Channel;

			Channel=HitTest(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam),&Type);
			if (Channel>=0 && Type!=HIT_MARGIN)
				::SetCursor(::LoadCursor(NULL,IDC_HAND));
			else
				::SetCursor(::LoadCursor(NULL,IDC_ARROW));
		}
		return 0;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case TTN_NEEDTEXT:
			{
				LPNMTTDISPINFO pnmtdi=reinterpret_cast<LPNMTTDISPINFO>(lParam);
				int Channel=LOWORD(pnmtdi->lParam),Event=HIWORD(pnmtdi->lParam);

				if (Channel>=0 && (size_t)Channel<m_ChannelList.size()) {
					static TCHAR szText[1024];

					m_ChannelList[Channel]->FormatEventText(szText,lengthof(szText),Event);
					RemoveTrailingWhitespace(szText);
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

	case WM_DESTROY:
		m_EventInfoPopupManager.Finalize();
		m_Tooltip.Destroy();
		m_Chevron.Destroy();
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


void CChannelPanel::Draw(HDC hdc,const RECT *prcPaint)
{
	HFONT hfontOld=static_cast<HFONT>(::GetCurrentObject(hdc,OBJ_FONT));
	COLORREF crOldTextColor=::GetTextColor(hdc);
	int OldBkMode=::SetBkMode(hdc,TRANSPARENT);

	RECT rcClient,rc;
	GetClientRect(&rcClient);
	rc.top=-m_ScrollPos;
	for (int i=0;i<(int)m_ChannelList.size() && rc.top<prcPaint->bottom;i++) {
		CChannelEventInfo *pChannelInfo=m_ChannelList[i];
		const bool fCurrent=pChannelInfo->GetOriginalChannelIndex()==m_CurChannel;

		rc.bottom=rc.top+m_ChannelNameHeight;
		if (rc.bottom>prcPaint->top) {
			const Theme::Style &Style=
				fCurrent?m_Theme.CurChannelNameStyle:m_Theme.ChannelNameStyle;

			DrawUtil::SelectObject(hdc,m_ChannelFont);
			::SetTextColor(hdc,Style.TextColor);
			rc.left=0;
			rc.right=rcClient.right;
			Theme::DrawStyleBackground(hdc,&rc,&Style);
			rc.left=m_ChannelNameMargins.left;
			rc.right-=m_ChannelChevronMargin+CHEVRON_WIDTH+m_ChannelNameMargins.right;
			pChannelInfo->DrawChannelName(hdc,&rc);
			m_Chevron.Draw(hdc,
						   rc.right+m_ChannelChevronMargin,
						   rc.top+((rc.bottom-rc.top)-CHEVRON_HEIGHT)/2,
						   Style.TextColor,
						   pChannelInfo->IsExpanded()?CHEVRON_WIDTH:0,0,
						   CHEVRON_WIDTH,CHEVRON_HEIGHT);
		}

		int NumEvents=pChannelInfo->IsExpanded()?m_ExpandEvents:m_EventsPerChannel;
		rc.left=0;
		rc.right=rcClient.right;
		for (int j=0;j<NumEvents;j++) {
			rc.top=rc.bottom;
			rc.bottom=rc.top+m_EventNameHeight;
			if (rc.bottom>prcPaint->top) {
				const Theme::Style &Style=
					(fCurrent?m_Theme.CurChannelEventStyle:m_Theme.EventStyle)[j%2];

				DrawUtil::SelectObject(hdc,m_Font);
				::SetTextColor(hdc,Style.TextColor);
				Theme::DrawStyleBackground(hdc,&rc,&Style);
				RECT rcText;
				rcText.left=rc.left+m_EventNameMargins.left;
				rcText.top=rc.top+m_EventNameMargins.top;
				rcText.right=rc.right-m_EventNameMargins.right;
				rcText.bottom=rc.bottom-m_EventNameMargins.bottom;
				pChannelInfo->DrawEventName(hdc,&rcText,j);
			}
		}
		rc.top=rc.bottom;
	}

	if (rc.top<prcPaint->bottom) {
		rc.left=prcPaint->left;
		if (rc.top<prcPaint->top)
			rc.top=prcPaint->top;
		rc.right=prcPaint->right;
		rc.bottom=prcPaint->bottom;
		DrawUtil::Fill(hdc,&rc,m_Theme.MarginColor);
	}

	::SetTextColor(hdc,crOldTextColor);
	::SetBkMode(hdc,OldBkMode);
	SelectFont(hdc,hfontOld);
}


void CChannelPanel::SetScrollPos(int Pos)
{
	RECT rc;

	GetClientRect(&rc);
	if (Pos<0) {
		Pos=0;
	} else {
		int Height=CalcHeight();
		int Max=max(Height-rc.bottom,0);
		if (Pos>Max)
			Pos=Max;
	}
	if (Pos!=m_ScrollPos) {
		int Offset=Pos-m_ScrollPos;

		m_ScrollPos=Pos;
		if (abs(Offset)<rc.bottom) {
			::ScrollWindowEx(m_hwnd,0,-Offset,
							 NULL,NULL,NULL,NULL,SW_ERASE | SW_INVALIDATE);
		} else {
			Invalidate();
		}
		SetScrollBar();
		SetTooltips(true);
	}
}


void CChannelPanel::SetScrollBar()
{
	SCROLLINFO si;
	RECT rc;

	si.cbSize=sizeof(SCROLLINFO);
	si.fMask=SIF_PAGE | SIF_RANGE | SIF_POS | SIF_DISABLENOSCROLL;
	si.nMin=0;
	si.nMax=CalcHeight();
	GetClientRect(&rc);
	si.nPage=rc.bottom;
	si.nPos=m_ScrollPos;
	::SetScrollInfo(m_hwnd,SB_VERT,&si,TRUE);
}


void CChannelPanel::CalcItemHeight()
{
	HDC hdc;

	hdc=::GetDC(m_hwnd);
	if (hdc==NULL)
		return;
	m_FontHeight=m_Font.GetHeight(hdc);
	m_ChannelNameHeight=m_FontHeight+m_ChannelNameMargins.top+m_ChannelNameMargins.bottom;
	m_EventNameHeight=m_FontHeight*m_EventNameLines+
		m_EventNameMargins.top+m_EventNameMargins.bottom;
	m_ItemHeight=m_ChannelNameHeight+m_EventNameHeight*m_EventsPerChannel;
	m_ExpandedItemHeight=m_ChannelNameHeight+m_EventNameHeight*m_ExpandEvents;
	::ReleaseDC(m_hwnd,hdc);
}


int CChannelPanel::CalcHeight() const
{
	int Height;

	Height=0;
	for (size_t i=0;i<m_ChannelList.size();i++) {
		if (m_ChannelList[i]->IsExpanded())
			Height+=m_ExpandedItemHeight;
		else
			Height+=m_ItemHeight;
	}
	return Height;
}


void CChannelPanel::GetItemRect(int Index,RECT *pRect)
{
	int y;

	y=-m_ScrollPos;
	for (int i=0;i<Index;i++)
		y+=m_ChannelList[i]->IsExpanded()?m_ExpandedItemHeight:m_ItemHeight;
	GetClientRect(pRect);
	pRect->top=y;
	pRect->bottom=y+(m_ChannelList[Index]->IsExpanded()?
											m_ExpandedItemHeight:m_ItemHeight);
}


int CChannelPanel::HitTest(int x,int y,HitType *pType) const
{
	POINT pt;
	RECT rc;

	pt.x=x;
	pt.y=y;
	GetClientRect(&rc);
	rc.top=-m_ScrollPos;
	for (int i=0;i<(int)m_ChannelList.size();i++) {
		rc.bottom=rc.top+m_ChannelNameHeight;
		if (::PtInRect(&rc,pt)) {
			if (pType!=NULL) {
				if (x<rc.right-(m_ChannelChevronMargin+CHEVRON_WIDTH+m_ChannelNameMargins.right))
					*pType=HIT_CHANNELNAME;
				else if (x>=rc.right-(CHEVRON_WIDTH+m_ChannelNameMargins.right))
					*pType=HIT_CHEVRON;
				else
					*pType=HIT_MARGIN;
			}
			return i;
		}
		const int NumEvents=
			m_ChannelList[i]->IsExpanded()?m_ExpandEvents:m_EventsPerChannel;
		for (int j=0;j<NumEvents;j++) {
			rc.top=rc.bottom;
			rc.bottom=rc.top+m_EventNameHeight;
			if (::PtInRect(&rc,pt)) {
				if (pType!=NULL)
					*pType=(HitType)(HIT_EVENT1+j);
				return i;
			}
		}
		rc.top=rc.bottom;
	}
	return -1;
}


bool CChannelPanel::CreateTooltip()
{
	if (!m_Tooltip.Create(m_hwnd))
		return false;
	m_Tooltip.SetMaxWidth(256);
	m_Tooltip.SetPopDelay(30*1000);
	SetTooltips();
	return true;
}


void CChannelPanel::SetTooltips(bool fRectOnly)
{
	if (m_Tooltip.IsCreated()) {
		int NumTools;
		if (fRectOnly) {
			NumTools=m_Tooltip.NumTools();
		} else {
			m_Tooltip.DeleteAllTools();
			NumTools=0;
		}

		int ToolCount;
		RECT rc;

		GetClientRect(&rc);
		rc.top=-m_ScrollPos;
		ToolCount=0;
		for (int i=0;i<(int)m_ChannelList.size();i++) {
			rc.top+=m_ChannelNameHeight;
			int NumEvents=m_ChannelList[i]->IsExpanded()?
											m_ExpandEvents:m_EventsPerChannel;
			for (int j=0;j<NumEvents;j++) {
				rc.bottom=rc.top+m_EventNameHeight;
				if (ToolCount<NumTools)
					m_Tooltip.SetToolRect(ToolCount,rc);
				else
					m_Tooltip.AddTool(ToolCount,rc,LPSTR_TEXTCALLBACK,MAKELPARAM(i,j));
				ToolCount++;
				rc.top=rc.bottom;
			}
		}
		if (NumTools>ToolCount) {
			for (int i=NumTools-1;i>=ToolCount;i--)
				m_Tooltip.DeleteTool(i);
		}
	}
}


bool CChannelPanel::EventInfoPopupHitTest(int x,int y,LPARAM *pParam)
{
	if (m_fDetailToolTip) {
		HitType Type;
		int Channel=HitTest(x,y,&Type);

		if (Channel>=0 && Type>=HIT_EVENT1) {
			int Event=Type-HIT_EVENT1;
			if (m_ChannelList[Channel]->IsEventEnabled(Event)) {
				*pParam=MAKELONG(Channel,Event);
				return true;
			}
		}
	}
	return false;
}


bool CChannelPanel::GetEventInfoPopupEventInfo(LPARAM Param,const CEventInfoData **ppInfo)
{
	int Channel=LOWORD(Param),Event=HIWORD(Param);

	if (Channel<0 || (size_t)Channel>=m_ChannelList.size()
			|| !m_ChannelList[Channel]->IsEventEnabled(Event))
		return false;
	*ppInfo=&m_ChannelList[Channel]->GetEventInfo(Event);
	return true;
}


CChannelPanel::CEventInfoPopupHandler::CEventInfoPopupHandler(CChannelPanel *pChannelPanel)
	: m_pChannelPanel(pChannelPanel)
{
}


bool CChannelPanel::CEventInfoPopupHandler::HitTest(int x,int y,LPARAM *pParam)
{
	return m_pChannelPanel->EventInfoPopupHitTest(x,y,pParam);
}


bool CChannelPanel::CEventInfoPopupHandler::GetEventInfo(LPARAM Param,const CEventInfoData **ppInfo)
{
	return m_pChannelPanel->GetEventInfoPopupEventInfo(Param,ppInfo);
}




CChannelPanel::CChannelEventInfo::CChannelEventInfo(const CChannelInfo *pInfo,int OriginalIndex)
	: m_ChannelInfo(*pInfo)
	, m_OriginalChannelIndex(OriginalIndex)
	, m_hbmLogo(NULL)
	, m_fExpanded(false)
{
}


CChannelPanel::CChannelEventInfo::~CChannelEventInfo()
{
}


bool CChannelPanel::CChannelEventInfo::SetEventInfo(int Index,const CEventInfoData *pInfo)
{
	if (Index<0)
		return false;
	bool fChanged=false;
	if (pInfo!=NULL) {
		if ((int)m_EventList.size()<=Index)
			m_EventList.resize(Index+1);
		if (m_EventList[Index]!=*pInfo) {
			m_EventList[Index]=*pInfo;
			fChanged=true;
		}
	} else {
		if (Index<(int)m_EventList.size()
				&& m_EventList[Index].m_fValidStartTime) {
			CEventInfoData Info;

			m_EventList[Index]=Info;
			fChanged=true;
		}
	}
	return fChanged;
}


void CChannelPanel::CChannelEventInfo::SetMaxEvents(int Events)
{
	if (Events>(int)m_EventList.size())
		m_EventList.resize(Events);
}


bool CChannelPanel::CChannelEventInfo::IsEventEnabled(int Index) const
{
	if (Index<0 || Index>=(int)m_EventList.size())
		return false;
	return m_EventList[Index].m_fValidStartTime;
}


int CChannelPanel::CChannelEventInfo::FormatEventText(LPTSTR pszText,int MaxLength,int Index) const
{
	if (!IsEventEnabled(Index)) {
		pszText[0]='\0';
		return 0;
	}

	const CEventInfoData &Info=m_EventList[Index];
	TCHAR szTime[EpgUtil::MAX_EVENT_TIME_LENGTH];
	EpgUtil::FormatEventTime(&Info,szTime,lengthof(szTime));
	return StdUtil::snprintf(pszText,MaxLength,TEXT("%s %s%s%s"),
							 szTime,
							 NullToEmptyString(Info.GetEventName()),
							 Info.GetEventText()!=NULL?TEXT("\n\n"):TEXT(""),
							 NullToEmptyString(Info.GetEventText()));
}


void CChannelPanel::CChannelEventInfo::DrawChannelName(HDC hdc,const RECT *pRect)
{
	RECT rc=*pRect;

	if (m_hbmLogo!=NULL) {
		int LogoWidth,LogoHeight;

		LogoHeight=rc.bottom-rc.top-4;
		LogoWidth=LogoHeight*16/9;
		// AlphaBlendでリサイズすると汚いので、予めリサイズした画像を作成しておく
		if (m_StretchedLogo.IsCreated()) {
			if (m_StretchedLogo.GetWidth()!=LogoWidth || m_StretchedLogo.GetHeight()!=LogoHeight)
				m_StretchedLogo.Destroy();
		}
		if (!m_StretchedLogo.IsCreated()) {
			HBITMAP hbm=DrawUtil::ResizeBitmap(m_hbmLogo,LogoWidth,LogoHeight);
			if (hbm!=NULL)
				m_StretchedLogo.Attach(hbm);
		}
		DrawUtil::DrawBitmap(hdc,rc.left,rc.top+(rc.bottom-rc.top-LogoHeight)/2,
							 LogoWidth,LogoHeight,
							 m_StretchedLogo.IsCreated()?m_StretchedLogo.GetHandle():m_hbmLogo,NULL,192);
		rc.left+=LogoWidth+3;
	}

	TCHAR szText[MAX_CHANNEL_NAME+16];
	if (m_ChannelInfo.GetChannelNo()!=0)
		StdUtil::snprintf(szText,lengthof(szText),TEXT("%d: %s"),
						  m_ChannelInfo.GetChannelNo(),m_ChannelInfo.GetName());
	else
		::lstrcpyn(szText,m_ChannelInfo.GetName(),lengthof(szText));
	::DrawText(hdc,szText,-1,&rc,
			   DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
}


void CChannelPanel::CChannelEventInfo::DrawEventName(HDC hdc,const RECT *pRect,int Index)
{
	if (IsEventEnabled(Index)) {
		const CEventInfoData &Info=m_EventList[Index];
		TCHAR szText[256],szTime[EpgUtil::MAX_EVENT_TIME_LENGTH];

		EpgUtil::FormatEventTime(&Info,szTime,lengthof(szTime),
								 EpgUtil::EVENT_TIME_HOUR_2DIGITS);
		StdUtil::snprintf(szText,lengthof(szText),TEXT("%s %s"),
						  szTime,NullToEmptyString(Info.GetEventName()));
		RECT rc=*pRect;
		::DrawText(hdc,szText,-1,&rc,
				   DT_WORDBREAK | DT_NOPREFIX | DT_END_ELLIPSIS);
	}
}
