#include "stdafx.h"
#include "TVTest.h"
#include "Panel.h"
#include "DrawUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define PANEL_WINDOW_CLASS			APP_NAME TEXT(" Panel")
#define PANEL_FRAME_WINDOW_CLASS	APP_NAME TEXT(" Panel Frame")
#define DROP_HELPER_WINDOW_CLASS	APP_NAME TEXT(" Drop Helper")




HINSTANCE CPanel::m_hinst=NULL;


bool CPanel::Initialize(HINSTANCE hinst)
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
		wc.lpszClassName=PANEL_WINDOW_CLASS;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CPanel::CPanel()
	: m_TitleHeight(0)
	, m_pWindow(NULL)
	, m_fShowTitle(false)
	, m_fEnableFloating(true)
	, m_pEventHandler(NULL)
	, m_HotItem(ITEM_NONE)
{
	m_Theme.TitleStyle.Back.Fill.Type=TVTest::Theme::FILL_SOLID;
	m_Theme.TitleStyle.Back.Fill.Solid.Color=::GetSysColor(COLOR_INACTIVECAPTION);
	m_Theme.TitleStyle.Fore.Fill.Type=TVTest::Theme::FILL_SOLID;
	m_Theme.TitleStyle.Fore.Fill.Solid.Color=::GetSysColor(COLOR_INACTIVECAPTIONTEXT);
	m_Theme.TitleIconStyle=m_Theme.TitleStyle;
	m_Theme.TitleIconHighlightStyle.Back.Fill.Type=TVTest::Theme::FILL_SOLID;
	m_Theme.TitleIconHighlightStyle.Back.Fill.Solid.Color.Set(0,0,128);
	m_Theme.TitleIconHighlightStyle.Back.Border.Type=TVTest::Theme::BORDER_NONE;
	m_Theme.TitleIconHighlightStyle.Fore.Fill.Type=TVTest::Theme::FILL_SOLID;
	m_Theme.TitleIconHighlightStyle.Fore.Fill.Solid.Color.Set(255,255,255);
}


CPanel::~CPanel()
{
	Destroy();
}


bool CPanel::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 PANEL_WINDOW_CLASS,NULL,m_hinst);
}


void CPanel::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);
}


void CPanel::NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	m_Style.NormalizeStyle(pStyleManager);
}


void CPanel::SetTheme(const TVTest::Theme::CThemeManager *pThemeManager)
{
	PanelTheme Theme;

	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_PANEL_TITLE,
							&Theme.TitleStyle);
	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_TITLEBAR_BUTTON,
							&Theme.TitleIconStyle);
	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_TITLEBAR_BUTTON_HOT,
							&Theme.TitleIconHighlightStyle);

	SetPanelTheme(Theme);
}


bool CPanel::SetWindow(CBasicWindow *pWindow,LPCTSTR pszTitle)
{
	RECT rc;

	m_pWindow=pWindow;
	if (m_pWindow!=NULL) {
		if (pWindow->GetParent()!=m_hwnd)
			pWindow->SetParent(m_hwnd);
		pWindow->SetVisible(true);
		TVTest::StringUtility::Assign(m_Title,pszTitle);
		GetPosition(&rc);
		rc.right=rc.left+pWindow->GetWidth();
		SetPosition(&rc);
	} else {
		m_Title.clear();
	}
	return true;
}


void CPanel::ShowTitle(bool fShow)
{
	if (m_fShowTitle!=fShow) {
		RECT rc;

		m_fShowTitle=fShow;
		GetClientRect(&rc);
		OnSize(rc.right,rc.bottom);
	}
}


void CPanel::EnableFloating(bool fEnable)
{
	m_fEnableFloating=fEnable;
}


void CPanel::SetEventHandler(CEventHandler *pHandler)
{
	m_pEventHandler=pHandler;
}


bool CPanel::SetPanelTheme(const PanelTheme &Theme)
{
	m_Theme=Theme;
	if (m_hwnd!=NULL && m_fShowTitle) {
		RECT rc;

		GetTitleRect(&rc);
		Invalidate(&rc);
	}
	return true;
}


bool CPanel::GetPanelTheme(PanelTheme *pTheme) const
{
	if (pTheme==NULL)
		return false;
	*pTheme=m_Theme;
	return true;
}


bool CPanel::GetTitleRect(RECT *pRect) const
{
	if (m_hwnd==NULL)
		return false;

	GetClientRect(pRect);
	pRect->bottom=m_TitleHeight;
	return true;
}


bool CPanel::GetContentRect(RECT *pRect) const
{
	if (m_hwnd==NULL)
		return false;

	GetClientRect(pRect);
	if (m_fShowTitle) {
		pRect->top=m_TitleHeight;
		if (pRect->bottom<pRect->top)
			pRect->bottom=pRect->top;
	}
	return true;
}


void CPanel::Draw(HDC hdc,const RECT &PaintRect) const
{
	if (m_fShowTitle && PaintRect.top<m_TitleHeight) {
		RECT rc;

		GetTitleRect(&rc);
		TVTest::Theme::Draw(hdc,rc,m_Theme.TitleStyle.Back);
		TVTest::Theme::SubtractBorderRect(m_Theme.TitleStyle.Back.Border,&rc);

		if (!m_Title.empty()) {
			TVTest::Style::Subtract(&rc,m_Style.TitlePadding);
			rc.right-=m_Style.TitleButtonIconSize.Width+m_Style.TitleButtonPadding.Horz();
			TVTest::Style::Subtract(&rc,m_Style.TitleLabelMargin);
			DrawUtil::DrawText(hdc,m_Title.c_str(),rc,
				DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX,
				&m_Font,m_Theme.TitleStyle.Fore.Fill.GetSolidColor());
		}

		GetCloseButtonRect(&rc);
		const TVTest::Theme::Style &Style=
			m_HotItem==ITEM_CLOSE?m_Theme.TitleIconHighlightStyle:m_Theme.TitleIconStyle;
		if (Style.Back.Border.Type!=TVTest::Theme::BORDER_NONE
				|| Style.Back.Fill!=m_Theme.TitleStyle.Back.Fill)
			TVTest::Theme::Draw(hdc,rc,Style.Back);
		DrawUtil::DrawText(hdc,TEXT("r"),rc,DT_CENTER | DT_VCENTER | DT_SINGLELINE,
						   &m_IconFont,Style.Fore.Fill.GetSolidColor());
	}
}


void CPanel::OnSize(int Width,int Height)
{
	if (m_pWindow!=NULL) {
		int y;

		if (m_fShowTitle)
			y=m_TitleHeight;
		else
			y=0;
		m_pWindow->SetPosition(0,y,Width,max(Height-y,0));
	}
	if (m_pEventHandler!=NULL)
		m_pEventHandler->OnSizeChanged(Width,Height);
}


void CPanel::GetCloseButtonRect(RECT *pRect) const
{
	const int ButtonWidth=m_Style.TitleButtonIconSize.Width+m_Style.TitleButtonPadding.Horz();
	const int ButtonHeight=m_Style.TitleButtonIconSize.Height+m_Style.TitleButtonPadding.Vert();
	RECT rc;

	GetClientRect(&rc);
	TVTest::Theme::SubtractBorderRect(m_Theme.TitleStyle.Back.Border,&rc);
	rc.right-=m_Style.TitlePadding.Right;
	rc.left=rc.right-ButtonWidth;
	rc.top=m_Style.TitlePadding.Top+
		(m_TitleHeight-m_Style.TitlePadding.Vert()-ButtonHeight)/2;
	rc.bottom=rc.top+ButtonHeight;
	*pRect=rc;
}


void CPanel::SetHotItem(ItemType Item)
{
	if (m_HotItem!=Item) {
		m_HotItem=Item;

		RECT rc;
		GetCloseButtonRect(&rc);
		Invalidate(&rc);
	}
}


LRESULT CPanel::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			InitializeUI();

			if (!m_Font.IsCreated())
				m_Font.Create(DrawUtil::FONT_CAPTION);

			if (!m_IconFont.IsCreated()) {
				LOGFONT lf;
				::ZeroMemory(&lf,sizeof(lf));
				lf.lfHeight=-m_Style.TitleButtonIconSize.Height;
				lf.lfCharSet=SYMBOL_CHARSET;
				::lstrcpy(lf.lfFaceName,TEXT("Marlett"));
				m_IconFont.Create(&lf);
			}

			HDC hdc=::GetDC(hwnd);
			m_FontHeight=m_Font.GetHeight(hdc,false);
			int LabelHeight=m_FontHeight+
				m_Style.TitleLabelMargin.Vert()+
				m_Style.TitleLabelExtraHeight;
			int ButtonHeight=
				m_Style.TitleButtonIconSize.Height+
				m_Style.TitleButtonPadding.Vert();
			RECT Border;
			TVTest::Theme::GetBorderWidths(m_Theme.TitleStyle.Back.Border,&Border);
			m_TitleHeight=max(LabelHeight,ButtonHeight)+m_Style.TitlePadding.Vert()+
				Border.top+Border.bottom;
			::ReleaseDC(hwnd,hdc);

			m_HotItem=ITEM_NONE;
			m_fCloseButtonPushed=false;
		}
		return 0;

	case WM_SIZE:
		OnSize(LOWORD(lParam),HIWORD(lParam));
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd,&ps);
			Draw(ps.hdc,ps.rcPaint);
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			POINT pt;

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			if (m_fShowTitle && pt.y<m_TitleHeight) {
				RECT rc;

				GetCloseButtonRect(&rc);
				if (::PtInRect(&rc,pt)) {
					SetHotItem(ITEM_CLOSE);
					m_fCloseButtonPushed=true;
					::SetCapture(hwnd);
				} else {
					SetHotItem(ITEM_NONE);
					if (m_fEnableFloating) {
						::ClientToScreen(hwnd,&pt);
						m_fCloseButtonPushed=false;
						m_ptDragStartPos=pt;
						::SetCapture(hwnd);
					}
				}
			}
		}
		return 0;

	case WM_LBUTTONUP:
		if (::GetCapture()==hwnd) {
			bool fCloseButtonPushed=m_fCloseButtonPushed;

			::ReleaseCapture();
			if (fCloseButtonPushed) {
				POINT pt;
				RECT rc;

				pt.x=GET_X_LPARAM(lParam);
				pt.y=GET_Y_LPARAM(lParam);
				GetCloseButtonRect(&rc);
				if (::PtInRect(&rc,pt)) {
					if (m_pEventHandler!=NULL)
						m_pEventHandler->OnClose();
				}
			}
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			POINT pt;

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);

			RECT rc;
			GetCloseButtonRect(&rc);
			if (::PtInRect(&rc,pt))
				SetHotItem(ITEM_CLOSE);
			else
				SetHotItem(ITEM_NONE);

			if (::GetCapture()==hwnd) {
				if (!m_fCloseButtonPushed) {
					::ClientToScreen(hwnd,&pt);
					if (abs(pt.x-m_ptDragStartPos.x)>=4
							|| abs(pt.y-m_ptDragStartPos.y)>=4) {
						::ReleaseCapture();
						if (m_pEventHandler!=NULL
								&& m_pEventHandler->OnFloating()) {
							::SendMessage(GetParent(),WM_NCLBUTTONDOWN,
										  HTCAPTION,MAKELONG(pt.x,pt.y));
						}
					}
				}
			} else {
				TRACKMOUSEEVENT tme;
				tme.cbSize=sizeof(TRACKMOUSEEVENT);
				tme.dwFlags=TME_LEAVE;
				tme.hwndTrack=hwnd;
				::TrackMouseEvent(&tme);
			}
		}
		return 0;

	case WM_MOUSELEAVE:
		SetHotItem(ITEM_NONE);
		return 0;

	case WM_SETCURSOR:
		if ((HWND)wParam==hwnd && LOWORD(lParam)==HTCLIENT && m_HotItem!=ITEM_NONE) {
			::SetCursor(::LoadCursor(NULL,IDC_HAND));
			return TRUE;
		}
		break;

	case WM_CAPTURECHANGED:
		if (m_fCloseButtonPushed) {
			SetHotItem(ITEM_NONE);
			m_fCloseButtonPushed=false;
		}
		return 0;

	case WM_RBUTTONUP:
		{
			POINT pt;

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			if (m_fShowTitle && pt.y<m_TitleHeight
					&& m_pEventHandler!=NULL) {
				HMENU hmenu=::CreatePopupMenu();

				::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,1,TEXT("����(&C)"));
				if (m_fEnableFloating)
					::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,2,TEXT("�؂藣��(&F)"));
				m_pEventHandler->OnMenuPopup(hmenu);
				::ClientToScreen(hwnd,&pt);
				int Command=::TrackPopupMenu(hmenu,
											 TPM_RIGHTBUTTON | TPM_RETURNCMD,
											 pt.x,pt.y,0,hwnd,NULL);
				switch (Command) {
				case 0:
					break;
				case 1:
					m_pEventHandler->OnClose();
					break;
				case 2:
					m_pEventHandler->OnFloating();
					break;
				default:
					m_pEventHandler->OnMenuSelected(Command);
					break;
				}
			}
		}
		return 0;

	case WM_KEYDOWN:
		if (m_pEventHandler!=NULL
				&& m_pEventHandler->OnKeyDown((UINT)wParam,(UINT)lParam))
			return 0;
		break;
	}

	return CCustomWindow::OnMessage(hwnd,uMsg,wParam,lParam);
}


CPanel::PanelStyle::PanelStyle()
	: TitlePadding(0,0,4,0)
	, TitleLabelMargin(4,2,4,2)
	, TitleLabelExtraHeight(4)
	, TitleButtonIconSize(12,12)
	, TitleButtonPadding(2)
{
}


void CPanel::PanelStyle::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	pStyleManager->Get(TEXT("panel.title.padding"),&TitlePadding);
	pStyleManager->Get(TEXT("panel.title.label.margin"),&TitleLabelMargin);
	pStyleManager->Get(TEXT("panel.title.label.extra-height"),&TitleLabelExtraHeight);
	pStyleManager->Get(TEXT("panel.title.button.icon"),&TitleButtonIconSize);
	pStyleManager->Get(TEXT("panel.title.button.padding"),&TitleButtonPadding);
}


void CPanel::PanelStyle::NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	pStyleManager->ToPixels(&TitlePadding);
	pStyleManager->ToPixels(&TitleLabelMargin);
	pStyleManager->ToPixels(&TitleLabelExtraHeight);
	pStyleManager->ToPixels(&TitleButtonIconSize);
	pStyleManager->ToPixels(&TitleButtonPadding);
}




HINSTANCE CPanelFrame::m_hinst=NULL;


bool CPanelFrame::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=0;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=NULL;
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=PANEL_FRAME_WINDOW_CLASS;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return CPanel::Initialize(hinst) && CDropHelper::Initialize(hinst);
}


CPanelFrame::CPanelFrame()
	: m_fFloating(true)
	, m_DockingWidth(-1)
	, m_fKeepWidth(true)
	, m_Opacity(255)
	, m_DragDockingTarget(DOCKING_NONE)
	, m_pEventHandler(NULL)
{
	m_WindowPosition.Left=120;
	m_WindowPosition.Top=120;
	m_WindowPosition.Width=200;
	m_WindowPosition.Height=240;
}


CPanelFrame::~CPanelFrame()
{
	Destroy();
}


bool CPanelFrame::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	if (!CreateBasicWindow(hwndParent,Style,ExStyle,ID,
						   PANEL_FRAME_WINDOW_CLASS,TEXT("�p�l��"),m_hinst))
		return false;
	if (m_Opacity<255) {
		SetWindowExStyle(ExStyle | WS_EX_LAYERED);
		::SetLayeredWindowAttributes(m_hwnd,0,m_Opacity,LWA_ALPHA);
	}
	return true;
}


bool CPanelFrame::Create(HWND hwndOwner,Layout::CSplitter *pSplitter,int PanelID,
						 CBasicWindow *pWindow,LPCTSTR pszTitle)
{
	RECT rc;

	m_pSplitter=pSplitter;
	m_PanelID=PanelID;
	if (!Create(hwndOwner,
				WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_CLIPCHILDREN,
				WS_EX_TOOLWINDOW))
		return false;
	m_Panel.Create(m_hwnd,WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN);
	m_Panel.SetWindow(pWindow,pszTitle);
	m_Panel.SetEventHandler(this);
	m_Panel.GetPosition(&rc);
	if (m_DockingWidth<0)
		m_DockingWidth=rc.right;
	if (m_fFloating) {
		m_Panel.SetVisible(false);
		m_Panel.SetParent(this);
		GetClientRect(&rc);
		m_Panel.SetPosition(&rc);
		m_Panel.ShowTitle(false);
		m_Panel.SetVisible(true);
	} else {
		Layout::CWindowContainer *pContainer=
			dynamic_cast<Layout::CWindowContainer*>(m_pSplitter->GetPaneByID(PanelID));

		pContainer->SetWindow(&m_Panel);
		m_pSplitter->SetPaneSize(PanelID,m_DockingWidth);
		rc.right=rc.left+m_DockingWidth;
		m_Panel.SetPosition(&rc);
		m_Panel.ShowTitle(true);
	}
	return true;
}


bool CPanelFrame::SetFloating(bool fFloating)
{
	if (m_fFloating!=fFloating) {
		if (m_hwnd!=NULL) {
			Layout::CWindowContainer *pContainer=
				dynamic_cast<Layout::CWindowContainer*>(m_pSplitter->GetPaneByID(m_PanelID));
			RECT rc;

			if (fFloating) {
				pContainer->SetVisible(false);
				pContainer->SetWindow(NULL);
				m_Panel.SetParent(this);
				m_Panel.SetVisible(true);
				GetClientRect(&rc);
				m_Panel.SetPosition(&rc);
				m_Panel.ShowTitle(false);
				SetVisible(true);
			} else {
				if (m_fKeepWidth) {
					m_Panel.GetClientRect(&rc);
					m_DockingWidth=rc.right;
				}
				SetVisible(false);
				m_Panel.SetVisible(false);
				m_Panel.ShowTitle(true);
				pContainer->SetWindow(&m_Panel);
				m_pSplitter->SetPaneSize(m_PanelID,m_DockingWidth);
				pContainer->SetVisible(true);
			}
		}
		m_fFloating=fFloating;
	}
	return true;
}


bool CPanelFrame::SetDockingWidth(int Width)
{
	m_DockingWidth=Width;
	return true;
}


void CPanelFrame::SetEventHandler(CEventHandler *pHandler)
{
	m_pEventHandler=pHandler;
}


bool CPanelFrame::SetPanelVisible(bool fVisible,bool fNoActivate)
{
	if (m_hwnd==NULL)
		return false;
	if (m_pEventHandler!=NULL)
		m_pEventHandler->OnVisibleChange(fVisible);
	if (m_fFloating) {
		if (fVisible) {
			SendSizeMessage();
			m_Panel.SetVisible(true);
			m_Panel.Invalidate();
		}
		if (fVisible && fNoActivate)
			::ShowWindow(m_hwnd,SW_SHOWNA);
		else
			SetVisible(fVisible);
	} else {
		Layout::CWindowContainer *pContainer=
			dynamic_cast<Layout::CWindowContainer*>(m_pSplitter->GetPaneByID(m_PanelID));

		pContainer->SetVisible(fVisible);
	}
	return true;
}


bool CPanelFrame::SetPanelTheme(const CPanel::PanelTheme &Theme)
{
	return m_Panel.SetPanelTheme(Theme);
}


bool CPanelFrame::GetPanelTheme(CPanel::PanelTheme *pTheme) const
{
	return m_Panel.GetPanelTheme(pTheme);
}


bool CPanelFrame::SetOpacity(int Opacity)
{
	if (Opacity<0 || Opacity>255)
		return false;
	if (Opacity!=m_Opacity) {
		if (m_hwnd!=NULL) {
			DWORD ExStyle=GetWindowExStyle();

			if (Opacity<255) {
				if ((ExStyle&WS_EX_LAYERED)==0)
					SetWindowExStyle(ExStyle|WS_EX_LAYERED);
				::SetLayeredWindowAttributes(m_hwnd,0,Opacity,LWA_ALPHA);
			} else {
				if ((ExStyle&WS_EX_LAYERED)!=0)
					SetWindowExStyle(ExStyle^WS_EX_LAYERED);
			}
		}
		m_Opacity=Opacity;
	}
	return true;
}


void CPanelFrame::SetTheme(const TVTest::Theme::CThemeManager *pThemeManager)
{
	m_Panel.SetTheme(pThemeManager);
}


LRESULT CPanelFrame::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			m_fDragMoving=false;
			HMENU hmenu=GetSystemMenu(hwnd,FALSE);
			InsertMenu(hmenu,0,MF_BYPOSITION | MFT_STRING | MFS_ENABLED,SC_DOCKING,TEXT("�h�b�L���O(&D)"));
			InsertMenu(hmenu,1,MF_BYPOSITION | MFT_SEPARATOR,0,NULL);
		}
		return 0;

	case WM_SIZE:
		if (m_fFloating)
			m_Panel.SetPosition(0,0,LOWORD(lParam),HIWORD(lParam));
		return 0;

	case WM_KEYDOWN:
		if (m_pEventHandler!=NULL
				&& m_pEventHandler->OnKeyDown((UINT)wParam,(UINT)lParam))
			return 0;
		break;

	case WM_MOUSEWHEEL:
		if (m_pEventHandler!=NULL
				&& m_pEventHandler->OnMouseWheel(wParam,lParam))
			return 0;
		break;

	case WM_ACTIVATE:
		if (m_pEventHandler!=NULL
				&& m_pEventHandler->OnActivate(LOWORD(wParam)!=WA_INACTIVE))
			return 0;
		break;

	case WM_MOVING:
		if (m_pEventHandler!=NULL
				&& m_pEventHandler->OnMoving(reinterpret_cast<LPRECT>(lParam)))
			return TRUE;
		break;

	case WM_MOVE:
		{
			POINT pt;
			RECT rcTarget,rc;
			DockingPlace Target;

			if (!m_fDragMoving)
				break;
			::GetCursorPos(&pt);
			m_pSplitter->GetLayoutBase()->GetScreenPosition(&rcTarget);
			Target=DOCKING_NONE;
			rc=rcTarget;
			rc.right=rc.left+16;
			if (::PtInRect(&rc,pt)) {
				Target=DOCKING_LEFT;
			} else {
				rc.right=rcTarget.right;
				rc.left=rc.right-16;
				if (PtInRect(&rc,pt))
					Target=DOCKING_RIGHT;
			}
			if (Target!=m_DragDockingTarget) {
				if (Target==DOCKING_NONE) {
					m_DropHelper.Hide();
				} else {
					int DockingWidth;

					if (m_fKeepWidth) {
						SIZE sz;
						m_Panel.GetClientSize(&sz);
						DockingWidth=sz.cx;
					} else {
						DockingWidth=m_DockingWidth;
					}
					if (Target==DOCKING_LEFT) {
						rc.right=rcTarget.left;
						rc.left=rc.right-DockingWidth;
					} else if (Target==DOCKING_RIGHT) {
						rc.left=rcTarget.right;
						rc.right=rc.left+DockingWidth;
					}
					m_DropHelper.Show(&rc);
				}
				m_DragDockingTarget=Target;
			}
		}
		break;

	case WM_EXITSIZEMOVE:
		if (m_DragDockingTarget!=DOCKING_NONE) {
			int Index;

			m_DropHelper.Hide();
			if (m_DragDockingTarget==DOCKING_LEFT)
				Index=0;
			else
				Index=1;
			//if (m_pSplitter->IDToIndex(m_PanelID)!=Index)
			if (m_pSplitter->GetPane(Index)->GetID()!=m_PanelID)
				m_pSplitter->SwapPane();
			::SendMessage(hwnd,WM_SYSCOMMAND,SC_DOCKING,0);
		}
		m_fDragMoving=false;
		return 0;

	case WM_ENTERSIZEMOVE:
		m_DragDockingTarget=DOCKING_NONE;
		m_fDragMoving=true;
		if (m_pEventHandler!=NULL
				&& m_pEventHandler->OnEnterSizeMove())
			return 0;
		break;

	case WM_SYSCOMMAND:
		switch (wParam) {
		case SC_DOCKING:
			if (m_pEventHandler!=NULL
					&& !m_pEventHandler->OnFloatingChange(false))
				return 0;
			SetFloating(false);
			return 0;
		}
		break;

	case WM_CLOSE:
		if (m_pEventHandler!=NULL
				&& !m_pEventHandler->OnClose())
			return 0;
		break;
	}

	return CCustomWindow::OnMessage(hwnd,uMsg,wParam,lParam);
}


bool CPanelFrame::OnFloating()
{
	if (m_fFloating)
		return false;

	RECT rc;
	m_Panel.GetContentRect(&rc);
	MapWindowRect(m_Panel.GetHandle(),NULL,&rc);
	if (m_pEventHandler!=NULL && !m_pEventHandler->OnFloatingChange(true))
		return false;
	CalcPositionFromClientRect(&rc);
	SetPosition(&rc);
	SetFloating(true);
	return true;
}


bool CPanelFrame::OnClose()
{
	return m_pEventHandler==NULL || m_pEventHandler->OnClose();
}


bool CPanelFrame::OnMoving(RECT *pRect)
{
	return m_pEventHandler!=NULL && m_pEventHandler->OnMoving(pRect);
}


bool CPanelFrame::OnEnterSizeMove()
{
	return m_pEventHandler!=NULL && m_pEventHandler->OnEnterSizeMove();
}


bool CPanelFrame::OnKeyDown(UINT KeyCode,UINT Flags)
{
	return m_pEventHandler!=NULL && m_pEventHandler->OnKeyDown(KeyCode,Flags);
}


void CPanelFrame::OnSizeChanged(int Width,int Height)
{
	if (!m_fFloating)
		m_DockingWidth=Width;
}


#define PANEL_MENU_PLACEMENT	CPanel::MENU_USER

bool CPanelFrame::OnMenuPopup(HMENU hmenu)
{
	::AppendMenu(hmenu,MF_STRING | MF_ENABLED,PANEL_MENU_PLACEMENT,
				 m_pSplitter->IDToIndex(m_PanelID)==0?TEXT("�E��(&R)"):TEXT("����(&L)"));
	return true;
}


bool CPanelFrame::OnMenuSelected(int Command)
{
	switch (Command) {
	case PANEL_MENU_PLACEMENT:
		m_pSplitter->SwapPane();
		break;
	default:
		return false;
	}
	return true;
}




HINSTANCE CDropHelper::m_hinst=NULL;


bool CDropHelper::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=0;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=NULL;
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=DROP_HELPER_WINDOW_CLASS;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CDropHelper::CDropHelper()
	: m_Opacity(128)
{
}


CDropHelper::~CDropHelper()
{
	Destroy();
}


bool CDropHelper::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 DROP_HELPER_WINDOW_CLASS,TEXT(""),m_hinst);
}


bool CDropHelper::Show(const RECT *pRect)
{
	if (m_hwnd==NULL) {
		if (!Create(NULL,WS_POPUP,
				WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE))
			return false;
		::SetLayeredWindowAttributes(m_hwnd,CLR_INVALID,m_Opacity,LWA_ALPHA);
	}
	SetPosition(pRect);
	::ShowWindow(m_hwnd,SW_SHOWNA);
	return true;
}


bool CDropHelper::Hide()
{
	if (m_hwnd!=NULL)
		SetVisible(false);
	return true;
}


LRESULT CDropHelper::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd,&ps);
			::FillRect(ps.hdc,&ps.rcPaint,static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
			::EndPaint(hwnd,&ps);
		}
		return 0;
	}

	return CCustomWindow::OnMessage(hwnd,uMsg,wParam,lParam);
}
