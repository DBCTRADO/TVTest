#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "TitleBar.h"
#include "DrawUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define TITLE_BAR_CLASS APP_NAME TEXT(" Title Bar")

#define TITLE_BORDER				1
#define TITLE_MARGIN				4
#define TITLE_BUTTON_ICON_WIDTH		12
#define TITLE_BUTTON_ICON_HEIGHT	12
#define TITLE_BUTTON_WIDTH			(TITLE_BUTTON_ICON_WIDTH+TITLE_MARGIN*2)
#define TITLE_ICON_WIDTH			16
#define TITLE_ICON_HEIGHT			16
#define ICON_TEXT_MARGIN			4
#define NUM_BUTTONS 4

enum {
	ICON_MINIMIZE,
	ICON_MAXIMIZE,
	ICON_FULLSCREEN,
	ICON_CLOSE,
	ICON_RESTORE,
	ICON_FULLSCREENCLOSE
};




HINSTANCE CTitleBar::m_hinst=NULL;


bool CTitleBar::Initialize(HINSTANCE hinst)
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
		wc.lpszClassName=TITLE_BAR_CLASS;
		if (RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CTitleBar::CTitleBar()
	: m_Font(DrawUtil::FONT_CAPTION)
	, m_FontHeight(m_Font.GetHeight(false))
	, m_hbmIcons(NULL)
	, m_hIcon(NULL)
	, m_HotItem(-1)
	, m_ClickItem(-1)
	, m_fMaximized(false)
	, m_fFullscreen(false)
	, m_pEventHandler(NULL)
{
	m_Theme.CaptionStyle.Gradient.Type=Theme::GRADIENT_NORMAL;
	m_Theme.CaptionStyle.Gradient.Direction=Theme::DIRECTION_VERT;
	m_Theme.CaptionStyle.Gradient.Color1=RGB(192,192,192);
	m_Theme.CaptionStyle.Gradient.Color2=RGB(192,192,192);
	m_Theme.CaptionStyle.Border.Type=Theme::BORDER_NONE;
	m_Theme.CaptionStyle.TextColor=RGB(255,255,255);
	m_Theme.IconStyle=m_Theme.CaptionStyle;
	m_Theme.HighlightIconStyle.Gradient.Type=Theme::GRADIENT_NORMAL;
	m_Theme.HighlightIconStyle.Gradient.Direction=Theme::DIRECTION_VERT;
	m_Theme.HighlightIconStyle.Gradient.Color1=RGB(0,0,128);
	m_Theme.HighlightIconStyle.Gradient.Color2=RGB(0,0,128);
	m_Theme.HighlightIconStyle.Border.Type=Theme::BORDER_NONE;
	m_Theme.HighlightIconStyle.TextColor=RGB(255,255,255);
	m_Theme.Border.Type=Theme::BORDER_RAISED;
	m_Theme.Border.Color=RGB(192,192,192);
}


CTitleBar::~CTitleBar()
{
	Destroy();
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pTitleBar=NULL;
	if (m_hbmIcons!=NULL)
		::DeleteObject(m_hbmIcons);
}


bool CTitleBar::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 TITLE_BAR_CLASS,NULL,m_hinst);
}


void CTitleBar::SetVisible(bool fVisible)
{
	m_HotItem=-1;
	CBasicWindow::SetVisible(fVisible);
}


bool CTitleBar::SetLabel(LPCTSTR pszLabel)
{
	if (!m_Label.Set(pszLabel))
		return false;
	if (m_hwnd!=NULL)
		UpdateItem(ITEM_LABEL);
	return true;
}


void CTitleBar::SetMaximizeMode(bool fMaximize)
{
	if (m_fMaximized!=fMaximize) {
		m_fMaximized=fMaximize;
		if (m_hwnd!=NULL)
			UpdateItem(ITEM_MAXIMIZE);
	}
}


void CTitleBar::SetFullscreenMode(bool fFullscreen)
{
	if (m_fFullscreen!=fFullscreen) {
		m_fFullscreen=fFullscreen;
		if (m_hwnd!=NULL)
			UpdateItem(ITEM_FULLSCREEN);
	}
}


bool CTitleBar::SetEventHandler(CEventHandler *pHandler)
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pTitleBar=NULL;
	if (pHandler!=NULL)
		pHandler->m_pTitleBar=this;
	m_pEventHandler=pHandler;
	return true;
}


bool CTitleBar::SetTheme(const ThemeInfo *pTheme)
{
	if (pTheme==NULL)
		return false;
	m_Theme=*pTheme;
	if (m_hwnd!=NULL)
		Invalidate();
	return true;
}


bool CTitleBar::GetTheme(ThemeInfo *pTheme) const
{
	if (pTheme==NULL)
		return false;
	*pTheme=m_Theme;
	return true;
}


bool CTitleBar::SetFont(const LOGFONT *pFont)
{
	if (!m_Font.Create(pFont))
		return false;
	m_FontHeight=m_Font.GetHeight(false);
	return true;
}


void CTitleBar::SetIcon(HICON hIcon)
{
	if (m_hIcon!=hIcon) {
		m_hIcon=hIcon;
		if (m_hwnd!=NULL) {
			RECT rc;

			GetItemRect(ITEM_LABEL,&rc);
			Invalidate(&rc);
		}
	}
}


LRESULT CTitleBar::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs=reinterpret_cast<LPCREATESTRUCT>(lParam);
			RECT rc;

			rc.left=0;
			rc.top=0;
			rc.right=0;
			rc.bottom=max(m_FontHeight,TITLE_BUTTON_ICON_HEIGHT)+TITLE_MARGIN*2+TITLE_BORDER*2;
			::AdjustWindowRectEx(&rc,pcs->style,FALSE,pcs->dwExStyle);
			::MoveWindow(hwnd,0,0,0,rc.bottom-rc.top,FALSE);

			if (m_hbmIcons==NULL)
				m_hbmIcons=static_cast<HBITMAP>(::LoadImage(
					GetAppClass().GetResourceInstance(),
					MAKEINTRESOURCE(IDB_TITLEBAR),IMAGE_BITMAP,0,0,
					LR_DEFAULTCOLOR | LR_CREATEDIBSECTION));

			m_Tooltip.Create(hwnd);
			for (int i=ITEM_BUTTON_FIRST;i<=ITEM_LAST;i++) {
				RECT rc;
				GetItemRect(i,&rc);
				m_Tooltip.AddTool(i,rc);
			}

			m_HotItem=-1;
			m_ClickItem=-1;

			m_MouseLeaveTrack.Initialize(hwnd);
		}
		return 0;

	case WM_SIZE:
		if (m_HotItem>=0) {
			UpdateItem(m_HotItem);
			m_HotItem=-1;
		}
		UpdateTooltipsRect();
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd,&ps);
			Draw(ps.hdc,ps.rcPaint);
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			int HotItem=HitTest(x,y);

			if (GetCapture()==hwnd) {
				if (HotItem!=m_ClickItem)
					HotItem=-1;
				if (HotItem!=m_HotItem) {
					int OldHotItem;

					OldHotItem=m_HotItem;
					m_HotItem=HotItem;
					if (OldHotItem>=0)
						UpdateItem(OldHotItem);
					if (m_HotItem>=0)
						UpdateItem(m_HotItem);
				}
			} else {
				if (HotItem!=m_HotItem) {
					int OldHotItem;

					OldHotItem=m_HotItem;
					m_HotItem=HotItem;
					if (OldHotItem>=0)
						UpdateItem(OldHotItem);
					if (m_HotItem>=0)
						UpdateItem(m_HotItem);
				}
				m_MouseLeaveTrack.OnMouseMove();
			}
		}
		return 0;

	case WM_MOUSELEAVE:
		if (m_HotItem>=0) {
			UpdateItem(m_HotItem);
			m_HotItem=-1;
		}
		if (m_MouseLeaveTrack.OnMouseLeave()) {
			if (m_pEventHandler)
				m_pEventHandler->OnMouseLeave();
		}
		return 0;

	case WM_NCMOUSEMOVE:
		m_MouseLeaveTrack.OnNcMouseMove();
		return 0;

	case WM_NCMOUSELEAVE:
		if (m_MouseLeaveTrack.OnNcMouseLeave()) {
			if (m_pEventHandler)
				m_pEventHandler->OnMouseLeave();
		}
		return 0;

	case WM_LBUTTONDOWN:
		m_ClickItem=m_HotItem;
		if (m_ClickItem==ITEM_LABEL) {
			if (m_pEventHandler!=NULL) {
				int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);

				if (x<16)
					m_pEventHandler->OnIconLButtonDown(x,y);
				else
					m_pEventHandler->OnLabelLButtonDown(x,y);
			}
		} else {
			::SetCapture(hwnd);
		}
		return 0;

	case WM_RBUTTONDOWN:
		if (m_HotItem==ITEM_LABEL) {
			if (m_pEventHandler!=NULL)
				m_pEventHandler->OnLabelRButtonDown(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		}
		return 0;

	case WM_LBUTTONUP:
		if (GetCapture()==hwnd) {
			::ReleaseCapture();
			if (m_HotItem>=0) {
				if (m_pEventHandler!=NULL) {
					switch (m_HotItem) {
					case ITEM_MINIMIZE:
						m_pEventHandler->OnMinimize();
						break;
					case ITEM_MAXIMIZE:
						m_pEventHandler->OnMaximize();
						break;
					case ITEM_FULLSCREEN:
						m_pEventHandler->OnFullscreen();
						break;
					case ITEM_CLOSE:
						m_pEventHandler->OnClose();
						break;
					}
				}
			}
		}
		return 0;

	case WM_LBUTTONDBLCLK:
		{
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);

			if (m_HotItem<0 && HitTest(x,y)==ITEM_LABEL)
				m_HotItem=ITEM_LABEL;
			if (m_HotItem==ITEM_LABEL) {
				if (m_pEventHandler!=NULL) {
					if (x>=TITLE_BORDER+TITLE_MARGIN
							&& x<TITLE_BORDER+TITLE_MARGIN+TITLE_ICON_WIDTH)
						m_pEventHandler->OnIconLButtonDoubleClick(x,y);
					else
						m_pEventHandler->OnLabelLButtonDoubleClick(x,y);
				}
			}
		}
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT) {
			if (m_HotItem>0) {
				::SetCursor(::LoadCursor(NULL,IDC_HAND));
				return TRUE;
			}
		}
		break;

	case WM_NOTIFY:
		switch (reinterpret_cast<NMHDR*>(lParam)->code) {
		case TTN_NEEDTEXT:
			{
				static const LPTSTR pszToolTip[] = {
					TEXT("最小化"),
					TEXT("最大化"),
					TEXT("全画面表示"),
					TEXT("閉じる"),
				};
				LPNMTTDISPINFO pnmttdi=reinterpret_cast<LPNMTTDISPINFO>(lParam);

				if (m_fMaximized && pnmttdi->hdr.idFrom==ITEM_MAXIMIZE)
					pnmttdi->lpszText=TEXT("元のサイズに戻す");
				else if (m_fFullscreen && pnmttdi->hdr.idFrom==ITEM_FULLSCREEN)
					pnmttdi->lpszText=TEXT("全画面表示解除");
				else
					pnmttdi->lpszText=pszToolTip[pnmttdi->hdr.idFrom-ITEM_BUTTON_FIRST];
				pnmttdi->hinst=NULL;
			}
			return 0;
		}
		break;

	case WM_DESTROY:
		m_Tooltip.Destroy();
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


bool CTitleBar::GetItemRect(int Item,RECT *pRect) const
{
	RECT rc;
	int ButtonPos;

	if (m_hwnd==NULL || Item<0 || Item>ITEM_LAST)
		return false;
	GetClientRect(&rc);
	rc.left+=TITLE_BORDER;
	rc.top+=TITLE_BORDER;
	rc.right-=TITLE_BORDER;
	rc.bottom-=TITLE_BORDER;
	ButtonPos=rc.right-NUM_BUTTONS*TITLE_BUTTON_WIDTH;
	if (ButtonPos<0)
		ButtonPos=0;
	if (Item==ITEM_LABEL) {
		rc.right=ButtonPos;
	} else {
		rc.left=ButtonPos+(Item-1)*TITLE_BUTTON_WIDTH;
		rc.right=rc.left+TITLE_BUTTON_WIDTH;
	}
	*pRect=rc;
	return true;
}


bool CTitleBar::UpdateItem(int Item)
{
	RECT rc;

	if (m_hwnd==NULL)
		return false;
	if (!GetItemRect(Item,&rc))
		return false;
	Invalidate(&rc,false);
	return true;
}


int CTitleBar::HitTest(int x,int y) const
{
	POINT pt;
	int i;
	RECT rc;

	pt.x=x;
	pt.y=y;
	for (i=ITEM_LAST;i>=0;i--) {
		GetItemRect(i,&rc);
		if (::PtInRect(&rc,pt))
			break;
	}
	return i;
}


void CTitleBar::UpdateTooltipsRect()
{
	for (int i=ITEM_BUTTON_FIRST;i<=ITEM_LAST;i++) {
		RECT rc;
		GetItemRect(i,&rc);
		m_Tooltip.SetToolRect(i,rc);
	}
}


void CTitleBar::Draw(HDC hdc,const RECT &PaintRect)
{
	HDC hdcMem=NULL;
	HBITMAP hbmOld;
	RECT rc,rcDraw;

	HFONT hfontOld=DrawUtil::SelectObject(hdc,m_Font);
	int OldBkMode=::SetBkMode(hdc,TRANSPARENT);
	COLORREF crOldTextColor=::GetTextColor(hdc);
	COLORREF crOldBkColor=::GetBkColor(hdc);
	for (int i=0;i<=ITEM_LAST;i++) {
		GetItemRect(i,&rc);
		if (rc.right>rc.left
				&& rc.left<PaintRect.right && rc.right>PaintRect.left
				&& rc.top<PaintRect.bottom && rc.bottom>PaintRect.top) {
			bool fHighlight=i==m_HotItem && i!=ITEM_LABEL;

			rcDraw.left=rc.left+TITLE_MARGIN;
			rcDraw.top=rc.top+TITLE_MARGIN;
			rcDraw.right=rc.right-TITLE_MARGIN;
			rcDraw.bottom=rc.bottom-TITLE_MARGIN;
			if (i==ITEM_LABEL) {
				Theme::DrawStyleBackground(hdc,&rc,&m_Theme.CaptionStyle);
				if (m_hIcon!=NULL) {
					::DrawIconEx(hdc,
								 rcDraw.left,
								 rc.top+((rc.bottom-rc.top)-TITLE_ICON_HEIGHT)/2,
								 m_hIcon,
								 TITLE_ICON_WIDTH,TITLE_ICON_HEIGHT,
								 0,NULL,DI_NORMAL);
					rcDraw.left+=TITLE_ICON_WIDTH+ICON_TEXT_MARGIN;
				}
				if (!m_Label.IsEmpty()) {
					::SetTextColor(hdc,m_Theme.CaptionStyle.TextColor);
					::DrawText(hdc,m_Label.Get(),-1,&rcDraw,
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
				}
			} else {
				const Theme::Style &Style=
					fHighlight?m_Theme.HighlightIconStyle:m_Theme.IconStyle;

				if (hdcMem==NULL) {
					hdcMem=::CreateCompatibleDC(hdc);
					hbmOld=SelectBitmap(hdcMem,m_hbmIcons);
				}
				Theme::DrawStyleBackground(hdc,&rc,&Style);
				DrawUtil::DrawMonoColorDIB(hdc,
					rc.left+((rc.right-rc.left)-TITLE_BUTTON_ICON_WIDTH)/2,
					rc.top+((rc.bottom-rc.top)-TITLE_BUTTON_ICON_HEIGHT)/2,
					hdcMem,
					((i==ITEM_MAXIMIZE && m_fMaximized)?ICON_RESTORE:
					((i==ITEM_FULLSCREEN && m_fFullscreen)?ICON_FULLSCREENCLOSE:
						i-1))*TITLE_BUTTON_ICON_WIDTH,
					0,
					TITLE_BUTTON_ICON_WIDTH,TITLE_BUTTON_ICON_HEIGHT,
					Style.TextColor);
			}
		}
	}
	if (hdcMem!=NULL) {
		::SelectObject(hdcMem,hbmOld);
		::DeleteDC(hdcMem);
	}
	if (rc.right<PaintRect.right) {
		rc.left=rc.right;
		rc.right=PaintRect.right;
		Theme::FillGradient(hdc,&rc,&m_Theme.CaptionStyle.Gradient);
	}
	GetClientRect(&rc);
	Theme::DrawBorder(hdc,rc,&m_Theme.Border);
	::SetBkColor(hdc,crOldBkColor);
	::SetTextColor(hdc,crOldTextColor);
	::SetBkMode(hdc,OldBkMode);
	::SelectObject(hdc,hfontOld);
}




CTitleBar::CEventHandler::CEventHandler()
	: m_pTitleBar(NULL)
{
}


CTitleBar::CEventHandler::~CEventHandler()
{
	if (m_pTitleBar!=NULL)
		m_pTitleBar->SetEventHandler(NULL);
}
