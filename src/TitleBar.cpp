#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "TitleBar.h"
#include "DrawUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


#define TITLE_BUTTON_ICON_WIDTH		12
#define TITLE_BUTTON_ICON_HEIGHT	12
#define NUM_BUTTONS 4

enum {
	ICON_MINIMIZE,
	ICON_MAXIMIZE,
	ICON_FULLSCREEN,
	ICON_CLOSE,
	ICON_RESTORE,
	ICON_FULLSCREENCLOSE
};




const LPCTSTR CTitleBar::CLASS_NAME=APP_NAME TEXT(" Title Bar");
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
		wc.lpszClassName=CLASS_NAME;
		if (RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CTitleBar::CTitleBar()
	: m_FontHeight(0)
	, m_hIcon(NULL)
	, m_HotItem(-1)
	, m_ClickItem(-1)
	, m_fMaximized(false)
	, m_fFullscreen(false)
	, m_pEventHandler(NULL)
{
}


CTitleBar::~CTitleBar()
{
	Destroy();
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pTitleBar=NULL;
}


bool CTitleBar::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 CLASS_NAME,NULL,m_hinst);
}


void CTitleBar::SetVisible(bool fVisible)
{
	m_HotItem=-1;
	CBasicWindow::SetVisible(fVisible);
}


void CTitleBar::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);
}


void CTitleBar::NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	m_Style.NormalizeStyle(pStyleManager);
}


void CTitleBar::SetTheme(const TVTest::Theme::CThemeManager *pThemeManager)
{
	TitleBarTheme Theme;

	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_TITLEBAR_CAPTION,
							&Theme.CaptionStyle);
	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_TITLEBAR_BUTTON,
							&Theme.IconStyle);
	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_TITLEBAR_BUTTON_HOT,
							&Theme.HighlightIconStyle);
	pThemeManager->GetBorderStyle(TVTest::Theme::CThemeManager::STYLE_TITLEBAR,
								  &Theme.Border);

	SetTitleBarTheme(Theme);
}


int CTitleBar::CalcHeight() const
{
	int LabelHeight=m_FontHeight+m_Style.LabelMargin.Vert();
	int IconHeight=m_Style.IconSize.Height+m_Style.IconMargin.Vert();
	int ButtonHeight=GetButtonHeight();
	int Height=max(LabelHeight,IconHeight);
	if (Height<ButtonHeight)
		Height=ButtonHeight;
	RECT Border;
	TVTest::Theme::GetBorderWidths(m_Theme.Border,&Border);

	return Height+m_Style.Padding.Vert()+Border.top+Border.bottom;
}


int CTitleBar::GetButtonWidth() const
{
	return m_Style.ButtonIconSize.Width+m_Style.ButtonPadding.Horz();
}


int CTitleBar::GetButtonHeight() const
{
	return m_Style.ButtonIconSize.Height+m_Style.ButtonPadding.Vert();
}


bool CTitleBar::SetLabel(LPCTSTR pszLabel)
{
	TVTest::StringUtility::Assign(m_Label,pszLabel);
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


bool CTitleBar::SetTitleBarTheme(const TitleBarTheme &Theme)
{
	m_Theme=Theme;

	if (m_hwnd!=NULL) {
		RECT rc;
		GetClientRect(&rc);
		int Height=CalcHeight();
		if (Height!=rc.bottom) {
			rc.bottom=Height;
			SetPosition(&rc);
			if (m_pEventHandler!=NULL)
				m_pEventHandler->OnHeightChanged(Height);
		}
		Invalidate();
	}

	return true;
}


bool CTitleBar::GetTitleBarTheme(TitleBarTheme *pTheme) const
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
	if (m_hwnd!=NULL) {
		m_FontHeight=CalcFontHeight();
	}
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


SIZE CTitleBar::GetIconDrawSize() const
{
	SIZE sz;

	sz.cx=m_Style.IconSize.Width;
	sz.cy=m_Style.IconSize.Height;
	return sz;
}


bool CTitleBar::IsIconDrawSmall() const
{
	return m_Style.IconSize.Width<=::GetSystemMetrics(SM_CXSMICON)
		&& m_Style.IconSize.Height<=::GetSystemMetrics(SM_CYSMICON);
}


LRESULT CTitleBar::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			InitializeUI();

			if (!m_Font.IsCreated())
				m_Font.Create(DrawUtil::FONT_CAPTION);
			m_FontHeight=CalcFontHeight();

			LPCREATESTRUCT pcs=reinterpret_cast<LPCREATESTRUCT>(lParam);
			RECT rc;

			rc.left=0;
			rc.top=0;
			rc.right=0;
			rc.bottom=CalcHeight();
			::AdjustWindowRectEx(&rc,pcs->style,FALSE,pcs->dwExStyle);
			::MoveWindow(hwnd,0,0,0,rc.bottom-rc.top,FALSE);

			if (!m_ButtonIcons.IsCreated()) {
				m_ButtonIcons.Load(GetAppClass().GetResourceInstance(),IDB_TITLEBAR,
								   TITLE_BUTTON_ICON_WIDTH,TITLE_BUTTON_ICON_HEIGHT);
			}

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

				if (PtInIcon(x,y))
					m_pEventHandler->OnIconLButtonDown(x,y);
				else
					m_pEventHandler->OnLabelLButtonDown(x,y);
			}
		} else {
			::SetCapture(hwnd);
		}
		return 0;

	case WM_RBUTTONUP:
		if (m_HotItem==ITEM_LABEL) {
			if (m_pEventHandler!=NULL)
				m_pEventHandler->OnLabelRButtonUp(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
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
					if (PtInIcon(x,y))
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
				::SetCursor(GetActionCursor());
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

	return CCustomWindow::OnMessage(hwnd,uMsg,wParam,lParam);
}


int CTitleBar::CalcFontHeight() const
{
	return TVTest::Style::GetFontHeight(m_hwnd,m_Font.GetHandle(),m_Style.LabelExtraHeight);
}


bool CTitleBar::GetItemRect(int Item,RECT *pRect) const
{
	if (m_hwnd==NULL || Item<0 || Item>ITEM_LAST)
		return false;

	RECT rc;

	GetClientRect(&rc);
	TVTest::Theme::SubtractBorderRect(m_Theme.Border,&rc);
	TVTest::Style::Subtract(&rc,m_Style.Padding);
	const int ButtonWidth=GetButtonWidth();
	int ButtonPos=rc.right-NUM_BUTTONS*ButtonWidth;
	if (ButtonPos<0)
		ButtonPos=0;
	if (Item==ITEM_LABEL) {
		rc.right=ButtonPos;
		if (rc.right<rc.left)
			rc.right=rc.left;
	} else {
		const int ButtonHeight=GetButtonHeight();
		rc.left=ButtonPos+(Item-1)*ButtonWidth;
		rc.right=rc.left+ButtonWidth;
		rc.top+=((rc.bottom-rc.top)-ButtonHeight)/2;
		rc.bottom=rc.top+ButtonHeight;
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


bool CTitleBar::PtInIcon(int x,int y) const
{
	RECT Border;
	TVTest::Theme::GetBorderWidths(m_Theme.Border,&Border);
	int IconLeft=Border.left+m_Style.Padding.Left+m_Style.IconMargin.Left;
	if (x>=IconLeft && x<IconLeft+m_Style.IconSize.Width)
		return true;
	return false;
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
	RECT rc;

	HFONT hfontOld=DrawUtil::SelectObject(hdc,m_Font);
	int OldBkMode=::SetBkMode(hdc,TRANSPARENT);
	COLORREF crOldTextColor=::GetTextColor(hdc);
	COLORREF crOldBkColor=::GetBkColor(hdc);

	GetClientRect(&rc);
	TVTest::Theme::SubtractBorderRect(m_Theme.Border,&rc);
	if (rc.left<PaintRect.left)
		rc.left=PaintRect.left;
	if (rc.right>PaintRect.right)
		rc.right=PaintRect.right;
	TVTest::Theme::Draw(hdc,rc,m_Theme.CaptionStyle.Back);

	for (int i=0;i<=ITEM_LAST;i++) {
		GetItemRect(i,&rc);
		if (rc.right>rc.left
				&& rc.left<PaintRect.right && rc.right>PaintRect.left
				&& rc.top<PaintRect.bottom && rc.bottom>PaintRect.top) {
			bool fHighlight=i==m_HotItem && i!=ITEM_LABEL;

			if (i==ITEM_LABEL) {
				if (m_hIcon!=NULL) {
					int Height=m_Style.IconSize.Height+m_Style.IconMargin.Vert();
					rc.left+=m_Style.IconMargin.Left;
					::DrawIconEx(hdc,
								 rc.left,
								 rc.top+m_Style.IconMargin.Top+((rc.bottom-rc.top)-Height)/2,
								 m_hIcon,
								 m_Style.IconSize.Width,m_Style.IconSize.Height,
								 0,NULL,DI_NORMAL);
					rc.left+=m_Style.IconSize.Width;
				}
				if (!m_Label.empty()) {
					TVTest::Style::Subtract(&rc,m_Style.LabelMargin);
					TVTest::Theme::Draw(hdc,rc,m_Theme.CaptionStyle.Fore,m_Label.c_str(),
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
				}
			} else {
				const TVTest::Theme::Style &Style=
					fHighlight?m_Theme.HighlightIconStyle:m_Theme.IconStyle;

				// とりあえず変にならないようにする。
				// 背景を透過指定できるようにした方が良い。
				if (Style.Back.Border.Type!=TVTest::Theme::BORDER_NONE
						|| Style.Back.Fill!=m_Theme.CaptionStyle.Back.Fill)
					TVTest::Theme::Draw(hdc,rc,Style.Back);
				m_ButtonIcons.Draw(hdc,
					rc.left+m_Style.ButtonPadding.Left,
					rc.top+m_Style.ButtonPadding.Top,
					m_Style.ButtonIconSize.Width,
					m_Style.ButtonIconSize.Height,
					(i==ITEM_MAXIMIZE && m_fMaximized)?ICON_RESTORE:
					((i==ITEM_FULLSCREEN && m_fFullscreen)?ICON_FULLSCREENCLOSE:
						i-1),
					Style.Fore.Fill.GetSolidColor());
			}
		}
	}

	GetClientRect(&rc);
	TVTest::Theme::Draw(hdc,rc,m_Theme.Border);

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




CTitleBar::TitleBarStyle::TitleBarStyle()
	: Padding(0,0,0,0)
	, LabelMargin(4,2,4,2)
	, LabelExtraHeight(4)
	, IconSize(16,16)
	, IconMargin(4,0,0,0)
	, ButtonIconSize(12,12)
	, ButtonPadding(4)
{
}


void CTitleBar::TitleBarStyle::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	pStyleManager->Get(TEXT("title-bar.padding"),&Padding);
	pStyleManager->Get(TEXT("title-bar.label.margin"),&LabelMargin);
	pStyleManager->Get(TEXT("title-bar.label.extra-height"),&LabelExtraHeight);
	pStyleManager->Get(TEXT("title-bar.icon"),&IconSize);
	pStyleManager->Get(TEXT("title-bar.icon.margin"),&IconMargin);
	pStyleManager->Get(TEXT("title-bar.button.icon"),&ButtonIconSize);
	pStyleManager->Get(TEXT("title-bar.button.padding"),&ButtonPadding);
}


void CTitleBar::TitleBarStyle::NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	pStyleManager->ToPixels(&Padding);
	pStyleManager->ToPixels(&LabelMargin);
	pStyleManager->ToPixels(&LabelExtraHeight);
	pStyleManager->ToPixels(&IconSize);
	pStyleManager->ToPixels(&IconMargin);
	pStyleManager->ToPixels(&ButtonIconSize);
	pStyleManager->ToPixels(&ButtonPadding);
}
