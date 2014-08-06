#include "stdafx.h"
#include "TVTest.h"
#include "View.h"
#include "DrawUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define VIEW_WINDOW_CLASS				APP_NAME TEXT(" View")
#define VIDEO_CONTAINER_WINDOW_CLASS	APP_NAME TEXT(" Video Container")




HINSTANCE CVideoContainerWindow::m_hinst=NULL;


CVideoContainerWindow::CVideoContainerWindow()
	: m_pDtvEngine(NULL)
	, m_pDisplayBase(NULL)
	, m_pEventHandler(NULL)
{
	m_ClientSize.cx=0;
	m_ClientSize.cy=0;
}


CVideoContainerWindow::~CVideoContainerWindow()
{
	Destroy();
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pVideoContainer=NULL;
}


bool CVideoContainerWindow::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_DBLCLKS;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=NULL;
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=VIDEO_CONTAINER_WINDOW_CLASS;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


LRESULT CVideoContainerWindow::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rcDest,rc;
			HBRUSH hbr;

			::BeginPaint(hwnd,&ps);
			hbr=static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
			if (!m_pDtvEngine->m_MediaViewer.GetDestRect(&rcDest)
					|| ::IsRectEmpty(&rcDest)) {
				::FillRect(ps.hdc,&ps.rcPaint,hbr);
			} else {
				m_pDtvEngine->m_MediaViewer.RepaintVideo(hwnd,ps.hdc);
				::GetClientRect(hwnd,&rc);
				if (!::EqualRect(&rc,&rcDest))
					DrawUtil::FillBorder(ps.hdc,&rc,&rcDest,&ps.rcPaint,hbr);
			}
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_MOVE:
		{
			CVideoRenderer::RendererType Renderer=
				m_pDtvEngine->m_MediaViewer.GetVideoRendererType();

			if (Renderer!=CVideoRenderer::RENDERER_VMR7
					&& Renderer!=CVideoRenderer::RENDERER_VMR9)
				break;
		}
	case WM_SIZE:
		{
			int Width=LOWORD(lParam),Height=HIWORD(lParam);

			m_pDtvEngine->m_MediaViewer.SetViewSize(Width,Height);
			if (m_pDisplayBase!=NULL)
				m_pDisplayBase->AdjustPosition();
			if (uMsg==WM_SIZE
					&& (Width!=m_ClientSize.cx || Height!=m_ClientSize.cy)) {
				if (m_pEventHandler!=NULL)
					m_pEventHandler->OnSizeChanged(Width,Height);
				m_ClientSize.cx=Width;
				m_ClientSize.cy=Height;
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
			POINT pt;

			if (uMsg==WM_LBUTTONDOWN || uMsg==WM_RBUTTONDOWN || uMsg==WM_MBUTTONDOWN)
				::SetFocus(hwnd);
			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			::MapWindowPoints(hwnd,::GetParent(hwnd),&pt,1);
			return ::SendMessage(::GetParent(hwnd),uMsg,wParam,MAKELONG(pt.x,pt.y));
		}

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		return ::SendMessage(::GetParent(hwnd),uMsg,wParam,lParam);
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


bool CVideoContainerWindow::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 VIDEO_CONTAINER_WINDOW_CLASS,NULL,m_hinst);
}


bool CVideoContainerWindow::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID,CDtvEngine *pDtvEngine)
{
	m_pDtvEngine=pDtvEngine;
	if (!Create(hwndParent,Style,ExStyle,ID)) {
		m_pDtvEngine=NULL;
		return false;
	}
	return true;
}


void CVideoContainerWindow::SetDisplayBase(CDisplayBase *pDisplayBase)
{
	m_pDisplayBase=pDisplayBase;
}


void CVideoContainerWindow::SetEventHandler(CEventHandler *pEventHandler)
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pVideoContainer=NULL;
	if (pEventHandler!=NULL)
		pEventHandler->m_pVideoContainer=this;
	m_pEventHandler=pEventHandler;
}




CVideoContainerWindow::CEventHandler::CEventHandler()
	: m_pVideoContainer(NULL)
{
}


CVideoContainerWindow::CEventHandler::~CEventHandler()
{
	if (m_pVideoContainer!=NULL)
		m_pVideoContainer->SetEventHandler(NULL);
}




HINSTANCE CViewWindow::m_hinst=NULL;


bool CViewWindow::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=NULL;
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=VIEW_WINDOW_CLASS;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CViewWindow::CViewWindow()
	: m_pVideoContainer(NULL)
	, m_hwndMessage(NULL)
	, m_pEventHandler(NULL)
	, m_hbmLogo(NULL)
	, m_BorderStyle(TVTest::Theme::BORDER_NONE,RGB(128,128,128))
	, m_fShowCursor(true)
{
}


CViewWindow::~CViewWindow()
{
	Destroy();
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pView=NULL;
	if (m_hbmLogo!=NULL)
		::DeleteObject(m_hbmLogo);
}


bool CViewWindow::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 VIEW_WINDOW_CLASS,NULL,m_hinst);
}


void CViewWindow::SetVideoContainer(CVideoContainerWindow *pVideoContainer)
{
	m_pVideoContainer=pVideoContainer;
	if (pVideoContainer!=NULL && m_hwnd!=NULL
			&& m_pVideoContainer->GetParent()==m_hwnd) {
		RECT rc;

		GetClientRect(&rc);
		pVideoContainer->SetPosition(&rc);
	}
}


void CViewWindow::SetMessageWindow(HWND hwnd)
{
	m_hwndMessage=hwnd;
}


void CViewWindow::SetEventHandler(CEventHandler *pEventHandler)
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pView=NULL;
	if (pEventHandler!=NULL)
		pEventHandler->m_pView=this;
	m_pEventHandler=pEventHandler;
}


bool CViewWindow::SetLogo(HBITMAP hbm)
{
	if (hbm==NULL && m_hbmLogo==NULL)
		return true;
	if (m_hbmLogo)
		::DeleteObject(m_hbmLogo);
	m_hbmLogo=hbm;
	if (m_hwnd)
		Redraw();
	return true;
}


void CViewWindow::SetBorder(const TVTest::Theme::BorderStyle &Style)
{
	if (m_BorderStyle!=Style) {
		const bool fResize=m_BorderStyle.Type!=Style.Type
			&& (m_BorderStyle.Type==TVTest::Theme::BORDER_NONE || Style.Type==TVTest::Theme::BORDER_NONE);
		m_BorderStyle=Style;
		if (m_hwnd) {
			if (fResize)
				SendSizeMessage();
			Invalidate();
		}
	}
}


void CViewWindow::ShowCursor(bool fShow)
{
	if (m_fShowCursor!=fShow) {
		m_fShowCursor=fShow;
		if (m_hwnd!=NULL) {
			POINT pt;
			HWND hwnd;

			::GetCursorPos(&pt);
			::ScreenToClient(m_hwnd,&pt);
			hwnd=::ChildWindowFromPointEx(m_hwnd,pt,CWP_SKIPINVISIBLE);
			if (hwnd==m_hwnd
					|| (m_pVideoContainer!=NULL
							&& hwnd==m_pVideoContainer->GetHandle()))
				::SetCursor(fShow?::LoadCursor(NULL,IDC_ARROW):NULL);
		}
	}
}


bool CViewWindow::CalcClientRect(RECT *pRect) const
{
	return TVTest::Theme::SubtractBorderRect(m_BorderStyle,pRect);
}


bool CViewWindow::CalcWindowRect(RECT *pRect) const
{
	return TVTest::Theme::AddBorderRect(m_BorderStyle,pRect);
}


LRESULT CViewWindow::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_SIZE:
		{
			const int Width=LOWORD(lParam),Height=HIWORD(lParam);

			if (m_pVideoContainer!=NULL
					&& m_pVideoContainer->GetParent()==hwnd) {
				RECT rc;

				::SetRect(&rc,0,0,Width,Height);
				CalcClientRect(&rc);
				m_pVideoContainer->SetPosition(rc.left,rc.top,
											   max(rc.right-rc.left,0),
											   max(rc.bottom-rc.top,0));
			}
			if (m_pEventHandler!=NULL)
				m_pEventHandler->OnSizeChanged(Width,Height);
		}
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rcClient;
			HBRUSH hbr=static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));

			::BeginPaint(hwnd,&ps);
			::GetClientRect(hwnd,&rcClient);
			if (m_hbmLogo) {
				RECT rcImage;
				BITMAP bm;

				::GetObject(m_hbmLogo,sizeof(BITMAP),&bm);
				rcImage.left=(rcClient.right-bm.bmWidth)/2;
				rcImage.top=(rcClient.bottom-bm.bmHeight)/2;
				rcImage.right=rcImage.left+bm.bmWidth;
				rcImage.bottom=rcImage.top+bm.bmHeight;
				DrawUtil::DrawBitmap(ps.hdc,
									 rcImage.left,rcImage.top,bm.bmWidth,bm.bmHeight,
									 m_hbmLogo);
				DrawUtil::FillBorder(ps.hdc,&rcClient,&rcImage,&ps.rcPaint,hbr);
			} else {
				::FillRect(ps.hdc,&ps.rcPaint,hbr);
			}
			TVTest::Theme::Draw(ps.hdc,rcClient,m_BorderStyle);
			::EndPaint(hwnd,&ps);
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
		if (m_hwndMessage!=NULL) {
			POINT pt;

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			::MapWindowPoints(hwnd,m_hwndMessage,&pt,1);
			return ::SendMessage(m_hwndMessage,uMsg,wParam,MAKELONG(pt.x,pt.y));
		}
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		if (m_hwndMessage!=NULL)
			return ::SendMessage(m_hwndMessage,uMsg,wParam,lParam);
		break;

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT) {
			HWND hwndCursor=reinterpret_cast<HWND>(wParam);

			if (hwndCursor==hwnd
					|| (m_pVideoContainer!=NULL
						&& hwndCursor==m_pVideoContainer->GetHandle())) {
				::SetCursor(m_fShowCursor?::LoadCursor(NULL,IDC_ARROW):NULL);
				return TRUE;
			}
		}
		break;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}




CViewWindow::CEventHandler::CEventHandler()
	: m_pView(NULL)
{
}


CViewWindow::CEventHandler::~CEventHandler()
{
	if (m_pView!=NULL)
		m_pView->SetEventHandler(NULL);
}




CDisplayView::CDisplayView()
	: m_pDisplayBase(NULL)
	, m_pEventHandler(NULL)
{
}


CDisplayView::~CDisplayView()
{
}


bool CDisplayView::IsMessageNeed(const MSG *pMsg) const
{
	return false;
}


void CDisplayView::SetVisible(bool fVisible)
{
	if (m_pDisplayBase!=NULL)
		m_pDisplayBase->SetVisible(fVisible);
}


void CDisplayView::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);
}


void CDisplayView::NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	m_Style.NormalizeStyle(pStyleManager);
}


void CDisplayView::SetDisplayVisible(bool fVisible)
{
	/*if (GetVisible()!=fVisible)*/ {
		if (OnVisibleChange(fVisible)) {
			CBasicWindow::SetVisible(fVisible);
		}
	}
}


bool CDisplayView::OnVisibleChange(bool fVisible)
{
	return true;
}


bool CDisplayView::GetCloseButtonRect(RECT *pRect) const
{
	RECT rc;

	if (!GetClientRect(&rc))
		return false;
	pRect->right=rc.right-1;
	pRect->left=pRect->right-14;
	pRect->top=1;
	pRect->bottom=pRect->top+14;
	return true;
}


bool CDisplayView::CloseButtonHitTest(int x,int y) const
{
	RECT rc;
	POINT pt;

	if (!GetCloseButtonRect(&rc))
		return false;
	pt.x=x;
	pt.y=y;
	return ::PtInRect(&rc,pt)!=FALSE;
}


void CDisplayView::DrawCloseButton(HDC hdc) const
{
	RECT rc;

	if (GetCloseButtonRect(&rc))
		::DrawFrameControl(hdc,&rc,DFC_CAPTION,DFCS_CAPTIONCLOSE | DFCS_MONO);
}


bool CDisplayView::GetItemStyle(ItemType Type,TVTest::Theme::Style *pStyle) const
{
	switch (Type) {
	case ITEM_STYLE_NORMAL:
	case ITEM_STYLE_NORMAL_1:
	case ITEM_STYLE_NORMAL_2:
		pStyle->Back.Fill.Type=TVTest::Theme::FILL_SOLID;
		if (Type!=ITEM_STYLE_NORMAL_2) {
			pStyle->Back.Fill.Solid.Color.Set(48,48,48);
		} else {
			pStyle->Back.Fill.Solid.Color.Set(24,24,24);
		}
		pStyle->Back.Border.Type=TVTest::Theme::BORDER_NONE;
		pStyle->Fore.Fill.Type=TVTest::Theme::FILL_SOLID;
		pStyle->Fore.Fill.Solid.Color.Set(255,255,255);
		break;

	case ITEM_STYLE_HOT:
		pStyle->Back.Fill.Type=TVTest::Theme::FILL_GRADIENT;
		pStyle->Back.Fill.Gradient.Type=TVTest::Theme::GRADIENT_NORMAL;
		pStyle->Back.Fill.Gradient.Direction=TVTest::Theme::DIRECTION_VERT;
		pStyle->Back.Fill.Gradient.Color1.Set(128,128,128);
		pStyle->Back.Fill.Gradient.Color2.Set(96,96,96);
		pStyle->Back.Border.Type=TVTest::Theme::BORDER_SOLID;
		pStyle->Back.Border.Color.Set(144,144,144);
		pStyle->Fore.Fill.Type=TVTest::Theme::FILL_SOLID;
		pStyle->Fore.Fill.Solid.Color.Set(255,255,255);
		break;

	case ITEM_STYLE_SELECTED:
	case ITEM_STYLE_CURRENT:
		pStyle->Back.Fill.Type=TVTest::Theme::FILL_GRADIENT;
		pStyle->Back.Fill.Gradient.Type=TVTest::Theme::GRADIENT_NORMAL;
		pStyle->Back.Fill.Gradient.Direction=TVTest::Theme::DIRECTION_VERT;
		pStyle->Back.Fill.Gradient.Color1.Set(96,96,96);
		pStyle->Back.Fill.Gradient.Color2.Set(128,128,128);
		if (Type==ITEM_STYLE_CURRENT) {
			pStyle->Back.Border.Type=TVTest::Theme::BORDER_SOLID;
			pStyle->Back.Border.Color.Set(144,144,144);
		} else {
			pStyle->Back.Border.Type=TVTest::Theme::BORDER_NONE;
		}
		pStyle->Fore.Fill.Type=TVTest::Theme::FILL_SOLID;
		pStyle->Fore.Fill.Solid.Color.Set(255,255,255);
		break;

	default:
		return false;
	}

	return true;
}


bool CDisplayView::GetBackgroundStyle(BackgroundType Type,TVTest::Theme::BackgroundStyle *pStyle) const
{
	switch (Type) {
	case BACKGROUND_STYLE_CONTENT:
		pStyle->Fill.Type=TVTest::Theme::FILL_GRADIENT;
		pStyle->Fill.Gradient.Direction=TVTest::Theme::DIRECTION_HORZ;
		pStyle->Fill.Gradient.Color1.Set(36,36,36);
		pStyle->Fill.Gradient.Color2.Set(16,16,16);
		pStyle->Border.Type=TVTest::Theme::BORDER_NONE;
		break;

	case BACKGROUND_STYLE_CATEGORIES:
		pStyle->Fill.Type=TVTest::Theme::FILL_GRADIENT;
		pStyle->Fill.Gradient.Direction=TVTest::Theme::DIRECTION_HORZ;
		pStyle->Fill.Gradient.Color1.Set(24,24,80);
		pStyle->Fill.Gradient.Color2.Set(24,24,32);
		pStyle->Border.Type=TVTest::Theme::BORDER_NONE;
		break;

	default:
		return false;
	}

	return true;
}


int CDisplayView::GetDefaultFontSize(int Width,int Height) const
{
	int Size=min(Width/m_Style.TextSizeRatioHorz,Height/m_Style.TextSizeRatioVert);
	return max(Size,m_Style.TextSizeMin);
}


void CDisplayView::SetEventHandler(CEventHandler *pEventHandler)
{
	m_pEventHandler=pEventHandler;
}


bool CDisplayView::HandleMessage(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam,LRESULT *pResult)
{
	*pResult=0;

	if (m_pEventHandler==NULL)
		return false;

	switch (Msg) {
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		m_pEventHandler->OnMouseMessage(Msg,GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		return true;
	}

	return false;
}


LRESULT CDisplayView::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	LRESULT Result;
	if (HandleMessage(hwnd,uMsg,wParam,lParam,&Result))
		return Result;

	return CCustomWindow::OnMessage(hwnd,uMsg,wParam,lParam);
}


CDisplayView::CEventHandler::~CEventHandler()
{
}


CDisplayView::DisplayViewStyle::DisplayViewStyle()
	: TextSizeRatioHorz(40)
	, TextSizeRatioVert(24)
	, TextSizeMin(12)
{
}


void CDisplayView::DisplayViewStyle::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	TVTest::Style::IntValue Value;

	if (pStyleManager->Get(TEXT("display.text-size-ratio.horz"),&Value) && Value.Value>0)
		TextSizeRatioHorz=Value;
	if (pStyleManager->Get(TEXT("display.text-size-ratio.vert"),&Value) && Value.Value>0)
		TextSizeRatioVert=Value;
	pStyleManager->Get(TEXT("display.text-size-min"),&TextSizeMin);
}


void CDisplayView::DisplayViewStyle::NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	pStyleManager->ToPixels(&TextSizeMin);
}




CDisplayBase::CDisplayBase()
	: m_pParentWindow(NULL)
	, m_pDisplayView(NULL)
	, m_pEventHandler(NULL)
	, m_fVisible(false)
{
}


CDisplayBase::~CDisplayBase()
{
}


void CDisplayBase::SetEventHandler(CEventHandler *pHandler)
{
	m_pEventHandler=pHandler;
}


void CDisplayBase::SetParent(CBasicWindow *pWindow)
{
	m_pParentWindow=pWindow;
}


void CDisplayBase::SetDisplayView(CDisplayView *pView)
{
	if (m_pDisplayView==pView)
		return;
	if (m_pDisplayView!=NULL) {
		SetVisible(false);
		m_pDisplayView->m_pDisplayBase=NULL;
	}
	m_pDisplayView=pView;
	if (pView!=NULL)
		pView->m_pDisplayBase=this;
	m_fVisible=false;
}


bool CDisplayBase::SetVisible(bool fVisible)
{
	if (m_pDisplayView==NULL)
		return false;
	if (m_fVisible==fVisible)
		return true;

	bool fFocus=!fVisible && m_pDisplayView->GetHandle()==::GetFocus();

	if (m_pEventHandler!=NULL && !m_pEventHandler->OnVisibleChange(fVisible))
		return false;

	if (fVisible) {
		if (m_pParentWindow!=NULL) {
			RECT rc;
			m_pParentWindow->GetClientRect(&rc);
			m_pDisplayView->SetPosition(&rc);
		}
		m_pDisplayView->SetDisplayVisible(true);
		::BringWindowToTop(m_pDisplayView->GetHandle());
		m_pDisplayView->Update();
		::SetFocus(m_pDisplayView->GetHandle());
	} else {
		m_pDisplayView->SetDisplayVisible(false);
	}

	m_fVisible=fVisible;

	if (fFocus && m_pParentWindow!=NULL) {
		if (m_pParentWindow->GetVisible())
			::SetFocus(m_pParentWindow->GetHandle());
		else
			::SetFocus(::GetAncestor(m_pParentWindow->GetHandle(),GA_ROOT));
	}

	return true;
}


bool CDisplayBase::IsVisible() const
{
	return m_pDisplayView!=NULL && m_fVisible;
}


void CDisplayBase::AdjustPosition()
{
	if (m_pParentWindow!=NULL && m_pDisplayView!=NULL && m_fVisible) {
		RECT rc;
		m_pParentWindow->GetClientRect(&rc);
		m_pDisplayView->SetPosition(&rc);
	}
}


void CDisplayBase::SetPosition(int Left,int Top,int Width,int Height)
{
	if (m_pDisplayView!=NULL)
		m_pDisplayView->SetPosition(Left,Top,Width,Height);
}


void CDisplayBase::SetPosition(const RECT *pRect)
{
	if (m_pDisplayView!=NULL)
		m_pDisplayView->SetPosition(pRect);
}


void CDisplayBase::SetFocus()
{
	if (m_pDisplayView!=NULL && m_pDisplayView->GetVisible())
		::SetFocus(m_pDisplayView->GetHandle());
}




CDisplayBase::CEventHandler::~CEventHandler()
{
}
