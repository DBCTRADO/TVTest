#include "stdafx.h"
#include "TVTest.h"
#include "PseudoOSD.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// タイマーの識別子
#define TIMER_ID_HIDE		1
#define TIMER_ID_ANIMATION	2

#define ANIMATION_FRAMES	4	// アニメーションの段階数
#define ANIMATION_INTERVAL	50	// アニメーションの間隔




const LPCTSTR CPseudoOSD::m_pszWindowClass=APP_NAME TEXT(" Pseudo OSD");
HINSTANCE CPseudoOSD::m_hinst=NULL;


bool CPseudoOSD::Initialize(HINSTANCE hinst)
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
		wc.lpszClassName=m_pszWindowClass;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


bool CPseudoOSD::IsPseudoOSD(HWND hwnd)
{
	TCHAR szClass[64];

	return ::GetClassName(hwnd,szClass,lengthof(szClass))>0
		&& ::lstrcmpi(szClass,m_pszWindowClass)==0;
}


CPseudoOSD::CPseudoOSD()
	: m_hwnd(NULL)
	, m_crBackColor(RGB(16,0,16))
	, m_crTextColor(RGB(0,255,128))
	, m_hbmIcon(NULL)
	, m_IconWidth(0)
	, m_IconHeight(0)
	, m_hbm(NULL)
	, m_ImageEffect(0)
	, m_TimerID(0)
	, m_Opacity(80)
{
	LOGFONT lf;
	DrawUtil::GetSystemFont(DrawUtil::FONT_DEFAULT,&lf);
	lf.lfHeight=32;
	lf.lfQuality=NONANTIALIASED_QUALITY;
	m_Font.Create(&lf);

	m_Position.Left=0;
	m_Position.Top=0;
	m_Position.Width=0;
	m_Position.Height=0;
}


CPseudoOSD::~CPseudoOSD()
{
	Destroy();
}


bool CPseudoOSD::Create(HWND hwndParent,bool fLayeredWindow)
{
	if (m_hwnd!=NULL) {
		if (::GetParent(m_hwnd)==hwndParent
				&& m_fLayeredWindow==fLayeredWindow)
			return true;
		Destroy();
	}
	m_fLayeredWindow=fLayeredWindow;
	m_hwndParent=hwndParent;
	if (fLayeredWindow) {
		POINT pt;
		RECT rc;

		pt.x=m_Position.Left;
		pt.y=m_Position.Top;
		::ClientToScreen(hwndParent,&pt);
		if (::CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
							 m_pszWindowClass,NULL,WS_POPUP,
							 pt.x,pt.y,m_Position.Width,m_Position.Height,
							 hwndParent,NULL,m_hinst,this)==NULL)
			return false;
		/*
		::SetLayeredWindowAttributes(m_hwnd,m_crBackColor,m_Opacity*255/100,
									 LWA_COLORKEY | LWA_ALPHA);
		*/
		::GetWindowRect(hwndParent,&rc);
		m_ParentPosition.x=rc.left;
		m_ParentPosition.y=rc.top;
		return true;
	}
	return ::CreateWindowEx(0,m_pszWindowClass,NULL,WS_CHILD,
							m_Position.Left,m_Position.Top,
							m_Position.Width,m_Position.Height,
							hwndParent,NULL,m_hinst,this)!=NULL;
}


bool CPseudoOSD::Destroy()
{
	if (m_hwnd!=NULL)
		::DestroyWindow(m_hwnd);
	return true;
}


bool CPseudoOSD::Show(DWORD Time,bool fAnimation)
{
	if (m_hwnd==NULL)
		return false;

	if (m_fLayeredWindow) {
		if (Time>0) {
			POINT pt;

			pt.x=m_Position.Left;
			pt.y=m_Position.Top;
			::ClientToScreen(m_hwndParent,&pt);
			m_TimerID|=::SetTimer(m_hwnd,TIMER_ID_HIDE,Time,NULL);
			if (fAnimation) {
				m_AnimationCount=0;
				::SetWindowPos(m_hwnd,NULL,pt.x,pt.y,
							   m_Position.Width/ANIMATION_FRAMES,m_Position.Height,
							   SWP_NOZORDER | SWP_NOACTIVATE);
				m_TimerID|=::SetTimer(m_hwnd,TIMER_ID_ANIMATION,ANIMATION_INTERVAL,NULL);
			} else {
				::SetWindowPos(m_hwnd,NULL,pt.x,pt.y,
							   m_Position.Width,m_Position.Height,
							   SWP_NOZORDER | SWP_NOACTIVATE);
			}
		} else if ((m_TimerID&TIMER_ID_HIDE)!=0) {
			::KillTimer(m_hwnd,TIMER_ID_HIDE);
			m_TimerID&=~TIMER_ID_HIDE;
		}
		UpdateLayeredWindow();
		::ShowWindow(m_hwnd,SW_SHOWNA);
		::UpdateWindow(m_hwnd);
		return true;
	}

	if (Time>0) {
		m_TimerID|=::SetTimer(m_hwnd,TIMER_ID_HIDE,Time,NULL);
		if (fAnimation) {
			m_AnimationCount=0;
			::MoveWindow(m_hwnd,m_Position.Left,m_Position.Top,
						 m_Position.Width/ANIMATION_FRAMES,m_Position.Height,
						 TRUE);
			m_TimerID|=::SetTimer(m_hwnd,TIMER_ID_ANIMATION,ANIMATION_INTERVAL,NULL);
		} else {
			::MoveWindow(m_hwnd,m_Position.Left,m_Position.Top,
						 m_Position.Width,m_Position.Height,TRUE);
		}
	} else if ((m_TimerID&TIMER_ID_HIDE)!=0) {
		::KillTimer(m_hwnd,TIMER_ID_HIDE);
		m_TimerID&=~TIMER_ID_HIDE;
	}
	if (::IsWindowVisible(m_hwnd)) {
		::RedrawWindow(m_hwnd,NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW);
	} else {
		::ShowWindow(m_hwnd,SW_SHOW);
		::BringWindowToTop(m_hwnd);
		::UpdateWindow(m_hwnd);
	}
	return true;
}


bool CPseudoOSD::Hide()
{
	if (m_hwnd==NULL)
		return false;
	::ShowWindow(m_hwnd,SW_HIDE);
	m_Text.Clear();
	m_hbm=NULL;
	return true;
}


bool CPseudoOSD::IsVisible() const
{
	if (m_hwnd==NULL)
		return false;
	return ::IsWindowVisible(m_hwnd)!=FALSE;
}


bool CPseudoOSD::SetText(LPCTSTR pszText,HBITMAP hbmIcon,int IconWidth,int IconHeight,unsigned int ImageEffect)
{
	m_Text.Set(pszText);
	m_hbmIcon=hbmIcon;
	if (hbmIcon!=NULL) {
		m_IconWidth=IconWidth;
		m_IconHeight=IconHeight;
		m_ImageEffect=ImageEffect;
	} else {
		m_IconWidth=0;
		m_IconHeight=0;
	}
	m_hbm=NULL;
	/*
	if (IsVisible()) {
		if (m_fLayeredWindow)
			UpdateLayeredWindow();
		else
			::RedrawWindow(m_hwnd,NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW);
	}
	*/
	return true;
}


bool CPseudoOSD::SetPosition(int Left,int Top,int Width,int Height)
{
	if (Width<=0 || Height<=0)
		return false;
	m_Position.Left=Left;
	m_Position.Top=Top;
	m_Position.Width=Width;
	m_Position.Height=Height;
	if (m_hwnd!=NULL) {
		if (m_fLayeredWindow) {
			POINT pt;

			pt.x=Left;
			pt.y=Top;
			::ClientToScreen(m_hwndParent,&pt);
			::SetWindowPos(m_hwnd,NULL,pt.x,pt.y,Width,Height,
						   SWP_NOZORDER | SWP_NOACTIVATE);
		} else {
			::SetWindowPos(m_hwnd,HWND_TOP,Left,Top,Width,Height,0);
		}
	}
	return true;
}


void CPseudoOSD::GetPosition(int *pLeft,int *pTop,int *pWidth,int *pHeight) const
{
	if (pLeft)
		*pLeft=m_Position.Left;
	if (pTop)
		*pTop=m_Position.Top;
	if (pWidth)
		*pWidth=m_Position.Width;
	if (pHeight)
		*pHeight=m_Position.Height;
}


void CPseudoOSD::SetTextColor(COLORREF crText)
{
	m_crTextColor=crText;
	/*
	if (m_hwnd!=NULL)
		::InvalidateRect(m_hwnd,NULL,TRUE);
	*/
}


bool CPseudoOSD::SetTextHeight(int Height)
{
	LOGFONT lf;

	if (!m_Font.GetLogFont(&lf))
		return false;
	lf.lfWidth=0;
	lf.lfHeight=-Height;
	return m_Font.Create(&lf);
}


bool CPseudoOSD::SetFont(const LOGFONT &Font)
{
	LOGFONT lf;

	lf=Font;
	lf.lfQuality=NONANTIALIASED_QUALITY;
	return m_Font.Create(&lf);
}


bool CPseudoOSD::CalcTextSize(SIZE *pSize)
{
	HDC hdc;
	HFONT hfontOld;
	bool fResult;

	if (m_Text.IsEmpty()) {
		pSize->cx=0;
		pSize->cy=0;
		return true;
	}
	if (m_hwnd!=NULL)
		hdc=::GetDC(m_hwnd);
	else
		hdc=::CreateCompatibleDC(NULL);
	hfontOld=DrawUtil::SelectObject(hdc,m_Font);
	RECT rc={0,0,0,0};
	fResult=::DrawText(hdc,m_Text.Get(),-1,&rc,DT_CALCRECT | DT_NOPREFIX)!=0;
	if (fResult) {
		pSize->cx=rc.right-rc.left;
		pSize->cy=rc.bottom-rc.top;
	}
	::SelectObject(hdc,hfontOld);
	if (m_hwnd!=NULL)
		::ReleaseDC(m_hwnd,hdc);
	else
		::DeleteDC(hdc);
	return fResult;
}


bool CPseudoOSD::SetImage(HBITMAP hbm,unsigned int ImageEffect)
{
	m_hbm=hbm;
	m_Text.Clear();
	m_hbmIcon=NULL;
	m_ImageEffect=ImageEffect;
#if 0
	if (m_hwnd!=NULL) {
		/*
		BITMAP bm;

		::GetObject(m_hbm,sizeof(BITMAP),&bm);
		m_Position.Width=bm.bmWidth;
		m_Position.Height=bm.bmHeight;
		::MoveWindow(m_hwnd,Left,Top,bm.bmWidth,bm.bmHeight,TRUE);
		*/
		if (m_fLayeredWindow)
			UpdateLayeredWindow();
		else
			::RedrawWindow(m_hwnd,NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW);
	}
#endif
	return true;
}


bool CPseudoOSD::SetOpacity(int Opacity)
{
	if (m_Opacity!=Opacity) {
		if (m_fLayeredWindow && IsVisible()) {
			/*
			::SetLayeredWindowAttributes(m_hwnd,m_crBackColor,m_Opacity*255/100,
										 LWA_COLORKEY | LWA_ALPHA);
			*/
			UpdateLayeredWindow();
		}
		m_Opacity=Opacity;
	}
	return true;
}


void CPseudoOSD::OnParentMove()
{
	if (m_hwnd!=NULL && m_fLayeredWindow) {
		RECT rcParent,rc;

		::GetWindowRect(m_hwndParent,&rcParent);
		::GetWindowRect(m_hwnd,&rc);
		::OffsetRect(&rc,
					 rcParent.left-m_ParentPosition.x,
					 rcParent.top-m_ParentPosition.y);
		::SetWindowPos(m_hwnd,NULL,rc.left,rc.top,
					   rc.right-rc.left,rc.bottom-rc.top,
					   SWP_NOZORDER | SWP_NOACTIVATE);
		m_ParentPosition.x=rcParent.left;
		m_ParentPosition.y=rcParent.top;
	}
}


void CPseudoOSD::Draw(HDC hdc,const RECT &PaintRect) const
{
	RECT rc;

	::GetClientRect(m_hwnd,&rc);
	if (!m_Text.IsEmpty()) {
		HFONT hfontOld;
		COLORREF crOldTextColor;
		int OldBkMode;

		DrawUtil::Fill(hdc,&PaintRect,m_crBackColor);
		if (m_hbmIcon!=NULL) {
			int IconWidth;
			if ((m_TimerID&TIMER_ID_ANIMATION)!=0)
				IconWidth=m_IconWidth*(m_AnimationCount+1)/ANIMATION_FRAMES;
			else
				IconWidth=m_IconWidth;
			DrawUtil::DrawBitmap(hdc,
								 0,(rc.bottom-m_IconHeight)/2,
								 IconWidth,m_IconHeight,
								 m_hbmIcon);
			RECT rcIcon;
			rcIcon.left=0;
			rcIcon.top=(rc.bottom-m_IconHeight)/2;
			rcIcon.right=rcIcon.left+IconWidth;
			rcIcon.bottom=rcIcon.top+m_IconHeight;
			DrawImageEffect(hdc,&rcIcon);
			rc.left+=IconWidth;
		}
		hfontOld=DrawUtil::SelectObject(hdc,m_Font);
		crOldTextColor=::SetTextColor(hdc,m_crTextColor);
		OldBkMode=::SetBkMode(hdc,TRANSPARENT);
		::DrawText(hdc,m_Text.Get(),-1,&rc,
				   DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		::SetBkMode(hdc,OldBkMode);
		::SetTextColor(hdc,crOldTextColor);
		::SelectObject(hdc,hfontOld);
	} else if (m_hbm!=NULL) {
		BITMAP bm;
		::GetObject(m_hbm,sizeof(BITMAP),&bm);
		RECT rcBitmap={0,0,bm.bmWidth,bm.bmHeight};
		DrawUtil::DrawBitmap(hdc,0,0,rc.right,rc.bottom,
							 m_hbm,&rcBitmap);
		DrawImageEffect(hdc,&rc);
	}
}


void CPseudoOSD::DrawImageEffect(HDC hdc,const RECT *pRect) const
{
	if ((m_ImageEffect&IMAGEEFFECT_GLOSS)!=0)
		DrawUtil::GlossOverlay(hdc,pRect);
	if ((m_ImageEffect&IMAGEEFFECT_DARK)!=0)
		DrawUtil::ColorOverlay(hdc,pRect,RGB(0,0,0),64);
}


static void SetBitmapOpacity(void *pBits,int Width,int Height,const RECT &Rect,BYTE Opacity)
{
	const unsigned int Alpha=Opacity*256/255;

	for (int y=Rect.top;y<Rect.bottom;y++) {
		BYTE *p=static_cast<BYTE*>(pBits)+(y*Width+Rect.left)*4;
		for (int x=Rect.left;x<Rect.right;x++) {
			p[0]=((unsigned int)p[0]*Alpha)>>8;
			p[1]=((unsigned int)p[1]*Alpha)>>8;
			p[2]=((unsigned int)p[2]*Alpha)>>8;
			p[3]=Opacity;
			p+=4;
		}
	}
}

static void SetBitmapTextOpacity(void *pBits,int Width,int Height,const RECT &Rect,BYTE TextOpacity,BYTE BackOpacity)
{
	for (int y=Rect.top;y<Rect.bottom;y++) {
		BYTE *p=static_cast<BYTE*>(pBits)+(y*Width+Rect.left)*4;
		for (int x=Rect.left;x<Rect.right;x++) {
			unsigned int Opacity;

			if (p[0]==0 && p[1]==0 && p[2]==0)
				Opacity=BackOpacity;
			else
				Opacity=TextOpacity;
			/*
			p[0]=p[0]*Opacity/255;
			p[1]=p[1]*Opacity/255;
			p[2]=p[2]*Opacity/255;
			*/
			p[0]=(p[0]*Opacity+255)>>8;
			p[1]=(p[1]*Opacity+255)>>8;
			p[2]=(p[2]*Opacity+255)>>8;
			p[3]=Opacity;
			p+=4;
		}
	}
}

void CPseudoOSD::UpdateLayeredWindow()
{
	RECT rc;
	int Width,Height;

	::GetWindowRect(m_hwnd,&rc);
	Width=rc.right-rc.left;
	Height=rc.bottom-rc.top;
	if (Width<1 || Height<1)
		return;

	BITMAPINFO bmi;
	::ZeroMemory(&bmi,sizeof(bmi));
	bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth=Width;
	bmi.bmiHeader.biHeight=-Height;
	bmi.bmiHeader.biPlanes=1;
	bmi.bmiHeader.biBitCount=32;
	bmi.bmiHeader.biCompression=BI_RGB;
	HBITMAP hbmSurface;
	void *pBits;
	hbmSurface=::CreateDIBSection(NULL,&bmi,DIB_RGB_COLORS,&pBits,NULL,0);
	if (hbmSurface==NULL)
		return;
	::ZeroMemory(pBits,Width*4*Height);

	HDC hdc=::GetDC(m_hwnd);
	HDC hdcSrc=::CreateCompatibleDC(hdc);
	HBITMAP hbmOld=static_cast<HBITMAP>(::SelectObject(hdcSrc,hbmSurface));

	::SetRect(&rc,0,0,Width,Height);
	if (!m_Text.IsEmpty()) {
		HFONT hfontOld;
		COLORREF crOldTextColor;
		int OldBkMode;

		if (m_hbmIcon!=NULL) {
			int IconWidth;
			if ((m_TimerID&TIMER_ID_ANIMATION)!=0)
				IconWidth=m_IconWidth*(m_AnimationCount+1)/ANIMATION_FRAMES;
			else
				IconWidth=m_IconWidth;
			DrawUtil::DrawBitmap(hdcSrc,
								 0,(Height-m_IconHeight)/2,
								 IconWidth,m_IconHeight,
								 m_hbmIcon);
			RECT rcIcon;
			rcIcon.left=0;
			rcIcon.top=(Height-m_IconHeight)/2;
			rcIcon.right=rcIcon.left+IconWidth;
			rcIcon.bottom=rcIcon.top+m_IconHeight;
			if (rcIcon.top<0)
				rcIcon.top=0;
			if (rcIcon.right>Width)
				rcIcon.right=Width;
			if (rcIcon.bottom>Height)
				rcIcon.bottom=Height;
			DrawImageEffect(hdcSrc,&rcIcon);
			SetBitmapOpacity(pBits,Width,Height,rcIcon,255);
			rc.left+=IconWidth;
		}
		hfontOld=DrawUtil::SelectObject(hdcSrc,m_Font);
		crOldTextColor=::SetTextColor(hdcSrc,m_crTextColor);
		OldBkMode=::SetBkMode(hdcSrc,TRANSPARENT);
		::DrawText(hdcSrc,m_Text.Get(),-1,&rc,
				   DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		SetBitmapTextOpacity(pBits,Width,Height,rc,m_Opacity*255/100,128);
		::SetBkMode(hdcSrc,OldBkMode);
		::SetTextColor(hdcSrc,crOldTextColor);
		::SelectObject(hdcSrc,hfontOld);
	} else if (m_hbm!=NULL) {
		BITMAP bm;
		::GetObject(m_hbm,sizeof(BITMAP),&bm);
		RECT rcBitmap={0,0,bm.bmWidth,bm.bmHeight};
		DrawUtil::DrawBitmap(hdcSrc,0,0,Width,Height,m_hbm,&rcBitmap);
		DrawImageEffect(hdcSrc,&rc);
		SetBitmapOpacity(pBits,Width,Height,rc,255);
	}

	SIZE sz={Width,Height};
	POINT ptSrc={0,0};
	BLENDFUNCTION blend={AC_SRC_OVER,0,255,AC_SRC_ALPHA};
	::UpdateLayeredWindow(m_hwnd,hdc,NULL,&sz,hdcSrc,&ptSrc,0,&blend,ULW_ALPHA);

	::SelectObject(hdcSrc,hbmOld);
	::DeleteDC(hdcSrc);
	::ReleaseDC(m_hwnd,hdc);
	::DeleteObject(hbmSurface);
}


CPseudoOSD *CPseudoOSD::GetThis(HWND hwnd)
{
	return reinterpret_cast<CPseudoOSD*>(::GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


LRESULT CALLBACK CPseudoOSD::WndProc(HWND hwnd,UINT uMsg,
												WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs=reinterpret_cast<LPCREATESTRUCT>(lParam);
			CPseudoOSD *pThis=static_cast<CPseudoOSD*>(pcs->lpCreateParams);

			pThis->m_hwnd=hwnd;
			::SetWindowLongPtr(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(pThis));
		}
		return 0;

	case WM_SIZE:
		{
			CPseudoOSD *pThis=GetThis(hwnd);

			if (pThis->m_fLayeredWindow && ::IsWindowVisible(hwnd))
				pThis->UpdateLayeredWindow();
		}
		return 0;

	case WM_PAINT:
		{
			CPseudoOSD *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;

			::BeginPaint(hwnd,&ps);
			if (!pThis->m_fLayeredWindow)
				pThis->Draw(ps.hdc,ps.rcPaint);
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_TIMER:
		{
			CPseudoOSD *pThis=GetThis(hwnd);

			switch (wParam) {
			case TIMER_ID_HIDE:
				pThis->Hide();
				::KillTimer(hwnd,TIMER_ID_HIDE);
				pThis->m_TimerID&=~TIMER_ID_HIDE;
				if ((pThis->m_TimerID&TIMER_ID_ANIMATION)!=0) {
					::KillTimer(hwnd,TIMER_ID_ANIMATION);
					pThis->m_TimerID&=~TIMER_ID_ANIMATION;
				}
				break;

			case TIMER_ID_ANIMATION:
				pThis->m_AnimationCount++;
				if (pThis->m_fLayeredWindow) {
					RECT rc;

					::GetWindowRect(hwnd,&rc);
					::SetWindowPos(hwnd,NULL,rc.left,rc.top,
								   pThis->m_Position.Width*(pThis->m_AnimationCount+1)/ANIMATION_FRAMES,
								   pThis->m_Position.Height,
								   SWP_NOZORDER | SWP_NOACTIVATE);
				} else {
					::MoveWindow(hwnd,pThis->m_Position.Left,pThis->m_Position.Top,
								 pThis->m_Position.Width*(pThis->m_AnimationCount+1)/ANIMATION_FRAMES,
								 pThis->m_Position.Height,
								 TRUE);
				}
				::UpdateWindow(hwnd);
				if (pThis->m_AnimationCount+1==ANIMATION_FRAMES) {
					::KillTimer(hwnd,TIMER_ID_ANIMATION);
					pThis->m_TimerID&=~TIMER_ID_ANIMATION;
				}
				break;
			}
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
			CPseudoOSD *pThis=GetThis(hwnd);
			POINT pt;
			RECT rc;

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			::MapWindowPoints(hwnd,pThis->m_hwndParent,&pt,1);
			::GetClientRect(pThis->m_hwndParent,&rc);
			if (::PtInRect(&rc,pt))
				return ::SendMessage(pThis->m_hwndParent,uMsg,wParam,MAKELPARAM(pt.x,pt.y));
		}
		return 0;

	case WM_SETCURSOR:
		{
			CPseudoOSD *pThis=GetThis(hwnd);

			return ::SendMessage(pThis->m_hwndParent,uMsg,wParam,lParam);
		}

	case WM_DESTROY:
		{
			CPseudoOSD *pThis=GetThis(hwnd);

			pThis->m_hwnd=NULL;
		}
		return 0;
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}
