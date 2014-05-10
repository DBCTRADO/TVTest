#include "stdafx.h"
#include "TVTest.h"
#include "NotificationBar.h"
#include "DrawUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define NOTIFICATION_BAR_WINDOW_CLASS APP_NAME TEXT(" Notification Bar")

#define BAR_MARGIN			4
#define ICON_TEXT_MARGIN	4




HINSTANCE CNotificationBar::m_hinst=NULL;


bool CNotificationBar::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=NOTIFICATION_BAR_WINDOW_CLASS;
		if (RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CNotificationBar::CNotificationBar()
	: m_fAnimate(true)
	, m_BarHeight(0)
	, m_TimerCount(0)
{
	m_BackGradient.Type=Theme::GRADIENT_NORMAL;
	m_BackGradient.Direction=Theme::DIRECTION_VERT;
	m_BackGradient.Color1=RGB(128,128,128);
	m_BackGradient.Color2=RGB(64,64,64);
	m_TextColor[MESSAGE_INFO]=RGB(224,224,224);
	m_TextColor[MESSAGE_WARNING]=RGB(255,160,64);
	m_TextColor[MESSAGE_ERROR]=RGB(224,64,64);
}


CNotificationBar::~CNotificationBar()
{
}


bool CNotificationBar::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 NOTIFICATION_BAR_WINDOW_CLASS,NULL,m_hinst);
}


bool CNotificationBar::Show(LPCTSTR pszText,MessageType Type,DWORD Timeout,bool fSkippable)
{
	if (m_hwnd==NULL || pszText==NULL)
		return false;

	MessageInfo Info;
	Info.Text.Set(pszText);
	Info.Type=Type;
	if (Type==MESSAGE_WARNING || Type==MESSAGE_ERROR) {
		Info.hIcon=static_cast<HICON>(
			::LoadImage(NULL,Type==MESSAGE_WARNING?IDI_WARNING:IDI_ERROR,
						IMAGE_ICON,
						::GetSystemMetrics(SM_CXSMICON),
						::GetSystemMetrics(SM_CYSMICON),
						LR_SHARED));
	} else {
		Info.hIcon=NULL;
	}
	Info.Timeout=Timeout;
	Info.fSkippable=fSkippable;
	m_MessageQueue.push_back(Info);

	EndTimer(TIMER_ID_SHOWANIMATION);
	EndTimer(TIMER_ID_FADEANIMATION);

	RECT rc;
	GetBarPosition(&rc);

	if (!GetVisible()) {
		while (m_MessageQueue.size()>1)
			m_MessageQueue.pop_front();
		::BringWindowToTop(m_hwnd);
		if (m_fAnimate)
			GetAnimatedBarPosition(&rc,0,SHOW_ANIMATION_COUNT);
		SetPosition(&rc);
		SetVisible(true);
		Update();
		if (m_fAnimate) {
			m_TimerCount=0;
			BeginTimer(TIMER_ID_SHOWANIMATION,SHOW_ANIMATION_INTERVAL);
		} else if (Timeout!=0) {
			BeginTimer(TIMER_ID_HIDE,Timeout);
		}
	} else {
		if (m_MessageQueue.size()>1) {
			auto itr=m_MessageQueue.begin()+(m_MessageQueue.size()-2);
			if (itr->fSkippable) {
				m_MessageQueue.erase(itr);
				if (m_MessageQueue.size()==1)
					EndTimer(TIMER_ID_HIDE);
			}
		}
		SetPosition(&rc);
		Redraw();
		Timeout=m_MessageQueue.front().Timeout;
		if (Timeout!=0 && !IsTimerEnabled(TIMER_ID_HIDE))
			BeginTimer(TIMER_ID_HIDE,Timeout);
	}

	return true;
}


bool CNotificationBar::Hide()
{
	if (m_hwnd==NULL)
		return false;

	EndAllTimers();

	if (m_fAnimate) {
		RECT rc;

		GetAnimatedBarPosition(&rc,FADE_ANIMATION_COUNT-1,FADE_ANIMATION_COUNT);
		SetPosition(&rc);
		Redraw();
		m_TimerCount=0;
		if (BeginTimer(TIMER_ID_FADEANIMATION,FADE_ANIMATION_INTERVAL))
			return true;
	}

	SetVisible(false);
	m_MessageQueue.clear();

	return true;
}


bool CNotificationBar::SetColors(const Theme::GradientInfo *pBackGradient,
	COLORREF crTextColor,COLORREF crWarningTextColor,COLORREF crErrorTextColor)
{
	m_BackGradient=*pBackGradient;
	m_TextColor[MESSAGE_INFO]=crTextColor;
	m_TextColor[MESSAGE_WARNING]=crWarningTextColor;
	m_TextColor[MESSAGE_ERROR]=crErrorTextColor;
	if (m_hwnd!=NULL)
		Invalidate();
	return true;
}


bool CNotificationBar::SetFont(const LOGFONT *pFont)
{
	if (!m_Font.Create(pFont))
		return false;
	if (m_hwnd!=NULL)
		CalcBarHeight();
	return true;
}


void CNotificationBar::CalcBarHeight()
{
	HDC hdc=::GetDC(m_hwnd);

	m_BarHeight=m_Font.GetHeight(hdc,false)+BAR_MARGIN*2;
	::ReleaseDC(m_hwnd,hdc);
	int IconHeight=::GetSystemMetrics(SM_CYSMICON);
	if (m_BarHeight<IconHeight+2)
		m_BarHeight=IconHeight+2;
}


void CNotificationBar::GetBarPosition(RECT *pRect) const
{
	::GetClientRect(::GetParent(m_hwnd),pRect);
	pRect->bottom=m_BarHeight;
}


void CNotificationBar::GetAnimatedBarPosition(RECT *pRect,int Frame,int NumFrames) const
{
	GetBarPosition(pRect);
	pRect->bottom=(Frame+1)*m_BarHeight/NumFrames;
}


void CNotificationBar::SetHideTimer()
{
	if (!m_MessageQueue.empty()) {
		DWORD Timeout=m_MessageQueue.front().Timeout;
		if (Timeout!=0)
			BeginTimer(TIMER_ID_HIDE,Timeout);
		else
			EndTimer(TIMER_ID_HIDE);
	}
}


LRESULT CNotificationBar::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			if (!m_Font.IsCreated()) {
				LOGFONT lf;
				DrawUtil::GetSystemFont(DrawUtil::FONT_MESSAGE,&lf);
				lf.lfHeight=-14;
				m_Font.Create(&lf);
			}

			CalcBarHeight();

			InitializeTimer(hwnd);
			m_TimerCount=0;
		}
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rc;

			::BeginPaint(hwnd,&ps);
			::GetClientRect(hwnd,&rc);
			Theme::FillGradient(ps.hdc,&rc,&m_BackGradient);
			if (!m_MessageQueue.empty()) {
				const MessageInfo &Info=m_MessageQueue.front();

				rc.left+=BAR_MARGIN;
				rc.right-=BAR_MARGIN;
				if (rc.left<rc.right) {
					if (Info.hIcon!=NULL) {
						int IconWidth=::GetSystemMetrics(SM_CXSMICON);
						int IconHeight=::GetSystemMetrics(SM_CYSMICON);
						::DrawIconEx(ps.hdc,rc.left,(rc.bottom-IconHeight)/2,
									 Info.hIcon,IconWidth,IconHeight,0,NULL,DI_NORMAL);
						rc.left+=IconWidth+ICON_TEXT_MARGIN;
					}
					DrawUtil::DrawText(ps.hdc,Info.Text.Get(),rc,
						DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS,
						&m_Font,m_TextColor[Info.Type]);
				}
			}
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_TIMER:
		switch (wParam) {
		case TIMER_ID_SHOWANIMATION:
			{
				RECT rc;

				GetAnimatedBarPosition(&rc,m_TimerCount+1,SHOW_ANIMATION_COUNT);
				SetPosition(&rc);
				Update();
				if (m_TimerCount<SHOW_ANIMATION_COUNT-2) {
					m_TimerCount++;
				} else {
					EndTimer(TIMER_ID_SHOWANIMATION);
					m_TimerCount=0;
					SetHideTimer();
				}
			}
			break;

		case TIMER_ID_FADEANIMATION:
			if (m_TimerCount<FADE_ANIMATION_COUNT-1) {
				RECT rc;

				GetAnimatedBarPosition(&rc,
					FADE_ANIMATION_COUNT-2-m_TimerCount,FADE_ANIMATION_COUNT);
				SetPosition(&rc);
				Update();
				m_TimerCount++;
			} else {
				SetVisible(false);
				EndTimer(TIMER_ID_FADEANIMATION);
				m_TimerCount=0;
				m_MessageQueue.clear();
			}
			break;

		case TIMER_ID_HIDE:
			if (!m_MessageQueue.empty())
				m_MessageQueue.pop_front();
			if (m_MessageQueue.empty()) {
				Hide();
			} else {
				Redraw();
				SetHideTimer();
			}
			break;
		}
		return 0;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_MOUSEMOVE:
		{
			POINT pt;
			HWND hwndParent=::GetParent(hwnd);

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			::MapWindowPoints(hwnd,hwndParent,&pt,1);
			return ::SendMessage(hwndParent,uMsg,wParam,MAKELPARAM(pt.x,pt.y));
		}
	}

	return CCustomWindow::OnMessage(hwnd,uMsg,wParam,lParam);
}
