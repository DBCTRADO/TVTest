#include "stdafx.h"
#include "WindowUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




/*
	�E�B���h�E�̒[�����ۂɕ\������Ă��邩���肷��
	���ꂾ�ƕs���S(��ɍőO�ʂ̃E�B���h�E���l�����Ă��Ȃ�)
*/
static bool IsWindowEdgeVisible(HWND hwnd,HWND hwndTop,const RECT *pRect,HWND hwndTarget)
{
	RECT rc,rcEdge;
	HWND hwndNext;

	if (hwndTop==hwnd || hwndTop==NULL)
		return true;
	GetWindowRect(hwndTop,&rc);
	hwndNext=GetNextWindow(hwndTop,GW_HWNDNEXT);
	if (hwndTop==hwndTarget || !IsWindowVisible(hwndTop)
			|| rc.left==rc.right || rc.top==rc.bottom)
		return IsWindowEdgeVisible(hwnd,hwndNext,pRect,hwndTarget);
	if (pRect->top==pRect->bottom) {
		if (rc.top<=pRect->top && rc.bottom>pRect->top) {
			if (rc.left<=pRect->left && rc.right>=pRect->right)
				return false;
			if (rc.left<=pRect->left && rc.right>pRect->left) {
				rcEdge=*pRect;
				rcEdge.right=min(rc.right,pRect->right);
				return IsWindowEdgeVisible(hwnd,hwndNext,&rcEdge,hwndTarget);
			} else if (rc.left>pRect->left && rc.right>=pRect->right) {
				rcEdge=*pRect;
				rcEdge.left=rc.left;
				return IsWindowEdgeVisible(hwnd,hwndNext,&rcEdge,hwndTarget);
			} else if (rc.left>pRect->left && rc.right<pRect->right) {
				rcEdge=*pRect;
				rcEdge.right=rc.left;
				if (IsWindowEdgeVisible(hwnd,hwndNext,&rcEdge,hwndTarget))
					return true;
				rcEdge.left=rc.right;
				rcEdge.right=pRect->right;
				return IsWindowEdgeVisible(hwnd,hwndNext,&rcEdge,hwndTarget);
			}
		}
	} else {
		if (rc.left<=pRect->left && rc.right>pRect->left) {
			if (rc.top<=pRect->top && rc.bottom>=pRect->bottom)
				return false;
			if (rc.top<=pRect->top && rc.bottom>pRect->top) {
				rcEdge=*pRect;
				rcEdge.bottom=min(rc.bottom,pRect->bottom);
				return IsWindowEdgeVisible(hwnd,hwndNext,&rcEdge,hwndTarget);
			} else if (rc.top>pRect->top && rc.bottom>=pRect->bottom) {
				rcEdge=*pRect;
				rcEdge.top=rc.top;
				return IsWindowEdgeVisible(hwnd,hwndNext,&rcEdge,hwndTarget);
			} else if (rc.top>pRect->top && rc.bottom<pRect->bottom) {
				rcEdge=*pRect;
				rcEdge.bottom=rc.top;
				if (IsWindowEdgeVisible(hwnd,hwndNext,&rcEdge,hwndTarget))
					return true;
				rcEdge.top=rc.bottom;
				rcEdge.bottom=pRect->bottom;
				return IsWindowEdgeVisible(hwnd,hwndNext,&rcEdge,hwndTarget);
			}
		}
	}
	return IsWindowEdgeVisible(hwnd,hwndNext,pRect,hwndTarget);
}


struct SnapWindowInfo {
	HWND hwnd;
	RECT rcOriginal;
	RECT rcNearest;
	HWND hwndExclude;
};

static BOOL CALLBACK SnapWindowProc(HWND hwnd,LPARAM lParam)
{
	SnapWindowInfo *pInfo=reinterpret_cast<SnapWindowInfo*>(lParam);

	if (IsWindowVisible(hwnd) && hwnd!=pInfo->hwnd && hwnd!=pInfo->hwndExclude) {
		RECT rc,rcEdge;

		GetWindowRect(hwnd,&rc);
		if (rc.right>rc.left && rc.bottom>rc.top) {
			if (rc.top<pInfo->rcOriginal.bottom && rc.bottom>pInfo->rcOriginal.top) {
				if (abs(rc.left-pInfo->rcOriginal.right)<abs(pInfo->rcNearest.right)) {
					rcEdge.left=rc.left;
					rcEdge.right=rc.left;
					rcEdge.top=max(rc.top,pInfo->rcOriginal.top);
					rcEdge.bottom=min(rc.bottom,pInfo->rcOriginal.bottom);
					if (IsWindowEdgeVisible(hwnd,GetTopWindow(GetDesktopWindow()),&rcEdge,pInfo->hwnd))
						pInfo->rcNearest.right=rc.left-pInfo->rcOriginal.right;
				}
				if (abs(rc.right-pInfo->rcOriginal.left)<abs(pInfo->rcNearest.left)) {
					rcEdge.left=rc.right;
					rcEdge.right=rc.right;
					rcEdge.top=max(rc.top,pInfo->rcOriginal.top);
					rcEdge.bottom=min(rc.bottom,pInfo->rcOriginal.bottom);
					if (IsWindowEdgeVisible(hwnd,GetTopWindow(GetDesktopWindow()),&rcEdge,pInfo->hwnd))
						pInfo->rcNearest.left=rc.right-pInfo->rcOriginal.left;
				}
			}
			if (rc.left<pInfo->rcOriginal.right && rc.right>pInfo->rcOriginal.left) {
				if (abs(rc.top-pInfo->rcOriginal.bottom)<abs(pInfo->rcNearest.bottom)) {
					rcEdge.left=max(rc.left,pInfo->rcOriginal.left);
					rcEdge.right=min(rc.right,pInfo->rcOriginal.right);
					rcEdge.top=rc.top;
					rcEdge.bottom=rc.top;
					if (IsWindowEdgeVisible(hwnd,GetTopWindow(GetDesktopWindow()),&rcEdge,pInfo->hwnd))
						pInfo->rcNearest.bottom=rc.top-pInfo->rcOriginal.bottom;
				}
				if (abs(rc.bottom-pInfo->rcOriginal.top)<abs(pInfo->rcNearest.top)) {
					rcEdge.left=max(rc.left,pInfo->rcOriginal.left);
					rcEdge.right=min(rc.right,pInfo->rcOriginal.right);
					rcEdge.top=rc.bottom;
					rcEdge.bottom=rc.bottom;
					if (IsWindowEdgeVisible(hwnd,GetTopWindow(GetDesktopWindow()),&rcEdge,pInfo->hwnd))
						pInfo->rcNearest.top=rc.bottom-pInfo->rcOriginal.top;
				}
			}
		}
	}
	return TRUE;
}


void SnapWindow(HWND hwnd,RECT *prc,int Margin,HWND hwndExclude)
{
	HMONITOR hMonitor;
	RECT rc;
	SnapWindowInfo Info;
	int XOffset,YOffset;

	hMonitor=MonitorFromWindow(hwnd,MONITOR_DEFAULTTONEAREST);
	if (hMonitor!=NULL) {
		MONITORINFO mi;

		mi.cbSize=sizeof(MONITORINFO);
		GetMonitorInfo(hMonitor,&mi);
		rc=mi.rcMonitor;
	} else {
		rc.left=0;
		rc.top=0;
		rc.right=GetSystemMetrics(SM_CXSCREEN);
		rc.bottom=GetSystemMetrics(SM_CYSCREEN);
	}
	Info.hwnd=hwnd;
	Info.rcOriginal=*prc;
	Info.rcNearest.left=rc.left-prc->left;
	Info.rcNearest.top=rc.top-prc->top;
	Info.rcNearest.right=rc.right-prc->right;
	Info.rcNearest.bottom=rc.bottom-prc->bottom;
	Info.hwndExclude=hwndExclude;
	EnumWindows(SnapWindowProc,reinterpret_cast<LPARAM>(&Info));
	if (abs(Info.rcNearest.left)<abs(Info.rcNearest.right)
			|| Info.rcNearest.left==Info.rcNearest.right)
		XOffset=Info.rcNearest.left;
	else if (abs(Info.rcNearest.left)>abs(Info.rcNearest.right))
		XOffset=Info.rcNearest.right;
	else
		XOffset=0;
	if (abs(Info.rcNearest.top)<abs(Info.rcNearest.bottom)
			|| Info.rcNearest.top==Info.rcNearest.bottom)
		YOffset=Info.rcNearest.top;
	else if (abs(Info.rcNearest.top)>abs(Info.rcNearest.bottom))
		YOffset=Info.rcNearest.bottom;
	else
		YOffset=0;
	if (abs(XOffset)<=Margin)
		prc->left+=XOffset;
	if (abs(YOffset)<=Margin)
		prc->top+=YOffset;
	prc->right=prc->left+(Info.rcOriginal.right-Info.rcOriginal.left);
	prc->bottom=prc->top+(Info.rcOriginal.bottom-Info.rcOriginal.top);
}


bool IsMessageInQueue(HWND hwnd,UINT Message)
{
	MSG msg;

	if (::PeekMessage(&msg,hwnd,Message,Message,PM_NOREMOVE)) {
		if (msg.message==WM_QUIT) {
			::PostQuitMessage(static_cast<int>(msg.wParam));
		} else {
			return true;
		}
	}

	return false;
}




CMouseLeaveTrack::CMouseLeaveTrack()
	: m_hwnd(NULL)
	, m_fClientTrack(false)
	, m_fNonClientTrack(false)
{
}

void CMouseLeaveTrack::Initialize(HWND hwnd)
{
	m_hwnd=hwnd;
	m_fClientTrack=false;
	m_fNonClientTrack=false;
}

bool CMouseLeaveTrack::OnMouseMove()
{
	// WM_MOUSELEAVE �������Ȃ��Ă������ɂ���鎖������悤��
	/*if (!m_fClientTrack)*/ {
		TRACKMOUSEEVENT tme;

		tme.cbSize=sizeof(tme);
		tme.dwFlags=TME_LEAVE;
		tme.hwndTrack=m_hwnd;
		if (!::TrackMouseEvent(&tme))
			return false;
		m_fClientTrack=true;
	}
	return true;
}

bool CMouseLeaveTrack::OnMouseLeave()
{
	m_fClientTrack=false;
	//return !IsCursorInWindow();
	return !m_fNonClientTrack;
}

bool CMouseLeaveTrack::OnNcMouseMove()
{
	/*if (!m_fNonClientTrack)*/ {
		TRACKMOUSEEVENT tme;

		tme.cbSize=sizeof(tme);
		tme.dwFlags=TME_LEAVE | TME_NONCLIENT;
		tme.hwndTrack=m_hwnd;
		if (!::TrackMouseEvent(&tme))
			return false;
		m_fNonClientTrack=true;
	}
	return true;
}

bool CMouseLeaveTrack::OnNcMouseLeave()
{
	m_fNonClientTrack=false;
	//return !IsCursorInWindow();
	return !m_fClientTrack;
}

bool CMouseLeaveTrack::OnMessage(UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch (Msg) {
	case WM_MOUSEMOVE:
		OnMouseMove();
		return true;

	case WM_MOUSELEAVE:
		OnMouseLeave();
		return true;

	case WM_NCMOUSEMOVE:
		OnNcMouseMove();
		return true;

	case WM_NCMOUSELEAVE:
		OnNcMouseLeave();
		return true;
	}

	return false;
}

bool CMouseLeaveTrack::IsCursorInWindow() const
{
	DWORD Pos=::GetMessagePos();
	POINT pt={GET_X_LPARAM(Pos),GET_Y_LPARAM(Pos)};
	RECT rc;

	::GetWindowRect(m_hwnd,&rc);
	return ::PtInRect(&rc,pt)!=FALSE;
}


CMouseWheelHandler::CMouseWheelHandler()
{
	Reset();
}

void CMouseWheelHandler::Reset()
{
	m_DeltaSum=0;
	m_LastDelta=0;
	m_LastTime=0;
}

void CMouseWheelHandler::ResetDelta()
{
	m_DeltaSum=0;
	m_LastDelta=0;
}

int CMouseWheelHandler::OnWheel(int Delta)
{
	DWORD CurTime=::GetTickCount();

	if ((DWORD)(CurTime-m_LastTime)>500
			|| (Delta>0)!=(m_LastDelta>0)) {
		m_DeltaSum=0;
	}

	m_DeltaSum+=Delta;
	m_LastDelta=Delta;
	m_LastTime=CurTime;

	return m_DeltaSum;
}

int CMouseWheelHandler::OnMouseWheel(WPARAM wParam,int ScrollLines)
{
	int Delta=OnWheel(GET_WHEEL_DELTA_WPARAM(wParam));
	if (abs(Delta)<WHEEL_DELTA)
		return 0;

	if (ScrollLines==0)
		ScrollLines=GetDefaultScrollLines();

	ResetDelta();

	return ::MulDiv(Delta,ScrollLines,WHEEL_DELTA);
}

int CMouseWheelHandler::OnMouseHWheel(WPARAM wParam,int ScrollChars)
{
	int Delta=OnWheel(GET_WHEEL_DELTA_WPARAM(wParam));
	if (abs(Delta)<WHEEL_DELTA)
		return 0;

	if (ScrollChars==0)
		ScrollChars=GetDefaultScrollChars();

	ResetDelta();

	return ::MulDiv(Delta,ScrollChars,WHEEL_DELTA);
}

int CMouseWheelHandler::GetDefaultScrollLines() const
{
	UINT Lines;

	if (::SystemParametersInfo(SPI_GETWHEELSCROLLLINES,0,&Lines,0))
		return Lines;
	return 2;
}

int CMouseWheelHandler::GetDefaultScrollChars() const
{
	UINT Chars;

	if (::SystemParametersInfo(SPI_GETWHEELSCROLLCHARS,0,&Chars,0))
		return Chars;
	return 3;
}


CWindowTimerManager::CWindowTimerManager()
	: m_hwndTimer(NULL)
	, m_TimerIDs(0)
{
}


void CWindowTimerManager::InitializeTimer(HWND hwnd)
{
	m_hwndTimer=hwnd;
	m_TimerIDs=0;
}


bool CWindowTimerManager::BeginTimer(unsigned int ID,DWORD Interval)
{
	if (m_hwndTimer==NULL
			|| ::SetTimer(m_hwndTimer,ID,Interval,NULL)==0)
		return false;
	m_TimerIDs|=ID;
	return true;
}


void CWindowTimerManager::EndTimer(unsigned int ID)
{
	if ((m_TimerIDs & ID)!=0) {
		::KillTimer(m_hwndTimer,ID);
		m_TimerIDs&=~ID;
	}
}


void CWindowTimerManager::EndAllTimers()
{
	unsigned int Flags=m_TimerIDs;
	for (int i=0;Flags!=0;i++,Flags>>=1) {
		unsigned int ID=m_TimerIDs&(1U<<i);
		if (ID!=0)
			EndTimer(ID);
	}
}


bool CWindowTimerManager::IsTimerEnabled(unsigned int ID) const
{
	return (m_TimerIDs & ID)==ID;
}
