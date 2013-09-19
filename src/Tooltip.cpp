#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "Tooltip.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#ifndef TTTOOLINFO_V2_SIZE
#ifdef UNICODE
#define TTTOOLINFO_V2_SIZE TTTOOLINFOW_V2_SIZE
#else
#define TTTOOLINFO_V2_SIZE TTTOOLINFOA_V2_SIZE
#endif
#endif




CTooltip::CTooltip()
	: m_hwndTooltip(NULL)
	, m_hwndParent(NULL)
{
}


CTooltip::~CTooltip()
{
	Destroy();
}


bool CTooltip::Create(HWND hwnd)
{
	if (m_hwndTooltip!=NULL)
		return false;

	m_hwndTooltip=::CreateWindowEx(WS_EX_TOPMOST,TOOLTIPS_CLASS,NULL,
								   WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
								   0,0,0,0,
								   hwnd,NULL,GetWindowInstance(hwnd),NULL);
	if (m_hwndTooltip==NULL)
		return false;

	m_hwndParent=hwnd;

	return true;
}


void CTooltip::Destroy()
{
	if (m_hwndTooltip!=NULL) {
		::DestroyWindow(m_hwndTooltip);
		m_hwndTooltip=NULL;
	}
}


void CTooltip::DeleteAllTools()
{
	if (m_hwndTooltip!=NULL) {
		int Count=(int)::SendMessage(m_hwndTooltip,TTM_GETTOOLCOUNT,0,0);
		TOOLINFO ti;

		ti.cbSize=TTTOOLINFO_V1_SIZE;
		ti.lpszText=NULL;
		for (int i=Count-1;i>=0;i--) {
			if (::SendMessage(m_hwndTooltip,TTM_ENUMTOOLS,i,reinterpret_cast<LPARAM>(&ti)))
				::SendMessage(m_hwndTooltip,TTM_DELTOOL,0,reinterpret_cast<LPARAM>(&ti));
		}
	}
}


bool CTooltip::Enable(bool fEnable)
{
	if (m_hwndTooltip==NULL)
		return false;
	::SendMessage(m_hwndTooltip,TTM_ACTIVATE,fEnable,0);
	return true;
}


bool CTooltip::IsVisible() const
{
	return m_hwndTooltip!=NULL && ::IsWindowVisible(m_hwndTooltip)!=FALSE;
}


bool CTooltip::SetMaxWidth(int Width)
{
	if (m_hwndTooltip==NULL)
		return false;
	::SendMessage(m_hwndTooltip,TTM_SETMAXTIPWIDTH,0,Width);
	return true;
}


bool CTooltip::SetPopDelay(int Delay)
{
	if (m_hwndTooltip==NULL)
		return false;
	::SendMessage(m_hwndTooltip,TTM_SETDELAYTIME,TTDT_AUTOPOP,
				  MAKELONG(min(Delay,32767),0));
	return true;
}


int CTooltip::NumTools() const
{
	if (m_hwndTooltip==NULL)
		return 0;
	return (int)::SendMessage(m_hwndTooltip,TTM_GETTOOLCOUNT,0,0);
}


bool CTooltip::AddTool(UINT ID,const RECT &Rect,LPCTSTR pszText,LPARAM lParam)
{
	if (m_hwndTooltip==NULL)
		return false;

	TOOLINFO ti;

	ti.cbSize=TTTOOLINFO_V2_SIZE;
	ti.uFlags=TTF_SUBCLASS | TTF_TRANSPARENT;
	ti.hwnd=m_hwndParent;
	ti.uId=ID;
	ti.rect=Rect;
	ti.hinst=NULL;
	ti.lpszText=const_cast<LPTSTR>(pszText);
	ti.lParam=lParam;
	return ::SendMessage(m_hwndTooltip,TTM_ADDTOOL,0,reinterpret_cast<LPARAM>(&ti))!=FALSE;
}


bool CTooltip::AddTool(HWND hwnd,LPCTSTR pszText,LPARAM lParam)
{
	if (m_hwndTooltip==NULL || hwnd==NULL)
		return false;

	TOOLINFO ti;

	ti.cbSize=TTTOOLINFO_V2_SIZE;
	ti.uFlags=TTF_IDISHWND | TTF_SUBCLASS | TTF_TRANSPARENT;
	ti.hwnd=m_hwndParent;
	ti.uId=reinterpret_cast<UINT_PTR>(hwnd);
	ti.hinst=NULL;
	ti.lpszText=const_cast<LPTSTR>(pszText);
	ti.lParam=lParam;
	return ::SendMessage(m_hwndTooltip,TTM_ADDTOOL,0,reinterpret_cast<LPARAM>(&ti))!=FALSE;
}


bool CTooltip::DeleteTool(UINT ID)
{
	if (m_hwndTooltip==NULL)
		return false;

	TOOLINFO ti;

	ti.cbSize=TTTOOLINFO_V2_SIZE;
	ti.hwnd=m_hwndParent;
	ti.uId=ID;
	::SendMessage(m_hwndTooltip,TTM_DELTOOL,0,reinterpret_cast<LPARAM>(&ti));
	return true;
}


bool CTooltip::SetToolRect(UINT ID,const RECT &Rect)
{
	if (m_hwndTooltip==NULL)
		return false;

	TOOLINFO ti;

	ti.cbSize=TTTOOLINFO_V2_SIZE;
	ti.hwnd=m_hwndParent;
	ti.uId=ID;
	ti.rect=Rect;
	::SendMessage(m_hwndTooltip,TTM_NEWTOOLRECT,0,reinterpret_cast<LPARAM>(&ti));
	return true;
}


bool CTooltip::SetText(UINT ID,LPCTSTR pszText)
{
	if (m_hwndTooltip==NULL)
		return false;

	TOOLINFO ti;

	ti.cbSize=TTTOOLINFO_V2_SIZE;
	ti.hwnd=m_hwndParent;
	ti.uId=ID;
	ti.hinst=NULL;
	ti.lpszText=const_cast<LPTSTR>(pszText);
	::SendMessage(m_hwndTooltip,TTM_UPDATETIPTEXT,0,reinterpret_cast<LPARAM>(&ti));
	return true;
}


bool CTooltip::AddTrackingTip(UINT ID,LPCTSTR pszText,LPARAM lParam)
{
	if (m_hwndTooltip==NULL)
		return false;

	TOOLINFO ti;

	ti.cbSize=TTTOOLINFO_V2_SIZE;
	ti.uFlags=TTF_SUBCLASS | TTF_TRANSPARENT | TTF_TRACK | TTF_ABSOLUTE;
	ti.hwnd=m_hwndParent;
	ti.uId=ID;
	::SetRectEmpty(&ti.rect);
	ti.hinst=NULL;
	ti.lpszText=const_cast<LPTSTR>(pszText);
	ti.lParam=lParam;
	return ::SendMessage(m_hwndTooltip,TTM_ADDTOOL,0,reinterpret_cast<LPARAM>(&ti))!=FALSE;
}


bool CTooltip::TrackActivate(UINT ID,bool fActivate)
{
	if (m_hwndTooltip==NULL)
		return false;

	TOOLINFO ti;

	ti.cbSize=TTTOOLINFO_V2_SIZE;
	ti.hwnd=m_hwndParent;
	ti.uId=ID;
	::SendMessage(m_hwndTooltip,TTM_TRACKACTIVATE,fActivate,reinterpret_cast<LPARAM>(&ti));
	return true;
}


bool CTooltip::TrackPosition(int x,int y)
{
	if (m_hwndTooltip==NULL)
		return false;

	::SendMessage(m_hwndTooltip,TTM_TRACKPOSITION,0,MAKELONG(x,y));
	return true;
}




CBalloonTip::CBalloonTip()
	: m_hwndToolTips(NULL)
{
}


CBalloonTip::~CBalloonTip()
{
	Finalize();
}


bool CBalloonTip::Initialize(HWND hwnd)
{
	if (m_hwndToolTips!=NULL)
		return false;

	m_hwndToolTips=::CreateWindowEx(WS_EX_TOPMOST,TOOLTIPS_CLASS,NULL,
									WS_POPUP | TTS_NOPREFIX | TTS_BALLOON/* | TTS_CLOSE*/,
									0,0,0,0,
									NULL,NULL,GetAppClass().GetInstance(),NULL);
	if (m_hwndToolTips==NULL)
		return false;

	::SendMessage(m_hwndToolTips,TTM_SETMAXTIPWIDTH,0,320);

	TOOLINFO ti;

	::ZeroMemory(&ti,sizeof(ti));
	ti.cbSize=TTTOOLINFO_V1_SIZE;
	ti.uFlags=TTF_SUBCLASS | TTF_TRACK;
	ti.hwnd=hwnd;
	ti.uId=0;
	ti.hinst=NULL;
	ti.lpszText=TEXT("");
	::SendMessage(m_hwndToolTips,TTM_ADDTOOL,0,(LPARAM)&ti);

	m_hwndOwner=hwnd;

	return true;
}


void CBalloonTip::Finalize()
{
	if (m_hwndToolTips!=NULL) {
		::DestroyWindow(m_hwndToolTips);
		m_hwndToolTips=NULL;
	}
}


bool CBalloonTip::Show(LPCTSTR pszText,LPCTSTR pszTitle,const POINT *pPos,int Icon)
{
	if (m_hwndToolTips==NULL || pszText==NULL)
		return false;
	TOOLINFO ti;
	ti.cbSize=TTTOOLINFO_V1_SIZE;
	ti.hwnd=m_hwndOwner;
	ti.uId=0;
	ti.lpszText=const_cast<LPTSTR>(pszText);
	::SendMessage(m_hwndToolTips,TTM_UPDATETIPTEXT,0,(LPARAM)&ti);
	::SendMessage(m_hwndToolTips,TTM_SETTITLE,Icon,(LPARAM)(pszTitle!=NULL?pszTitle:TEXT("")));
	POINT pt;
	if (pPos!=NULL) {
		pt=*pPos;
	} else {
		RECT rc;
		::SystemParametersInfo(SPI_GETWORKAREA,0,&rc,0);
		pt.x=rc.right-32;
		pt.y=rc.bottom;
	}
	::SendMessage(m_hwndToolTips,TTM_TRACKPOSITION,0,MAKELPARAM(pt.x,pt.y));
	::SendMessage(m_hwndToolTips,TTM_TRACKACTIVATE,TRUE,(LPARAM)&ti);
	return true;
}


bool CBalloonTip::Hide()
{
	TOOLINFO ti;

	ti.cbSize=TTTOOLINFO_V1_SIZE;
	ti.hwnd=m_hwndOwner;
	ti.uId=0;
	::SendMessage(m_hwndToolTips,TTM_TRACKACTIVATE,FALSE,(LPARAM)&ti);
	return true;
}
