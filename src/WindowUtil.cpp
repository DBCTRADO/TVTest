#include "stdafx.h"
#include "WindowUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




/*
	ウィンドウの端が実際に表示されているか判定する
	これだと不完全(常に最前面のウィンドウを考慮していない)
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
	// WM_MOUSELEAVE が送られなくても無効にされる事があるようだ
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
