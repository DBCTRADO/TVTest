#include "stdafx.h"
#include "TVTest.h"
#include "PanelForm.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




const LPCTSTR CPanelForm::m_pszClassName=APP_NAME TEXT(" Panel Form");
HINSTANCE CPanelForm::m_hinst=NULL;


bool CPanelForm::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=0;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=m_pszClassName;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CPanelForm::CPanelForm()
	: m_NumWindows(0)
	, m_Font(DrawUtil::FONT_DEFAULT)
	, m_TabHeight(0)
	, m_TabWidth(0)
	, m_fFitTabWidth(true)
	, m_CurTab(-1)
	, m_pEventHandler(NULL)
{
	m_WindowPosition.Width=200;
	m_WindowPosition.Height=240;

	m_Theme.TabStyle.Back.Fill.Type=TVTest::Theme::FILL_SOLID;
	m_Theme.TabStyle.Back.Fill.Solid.Color.Set(192,192,192);
	m_Theme.TabStyle.Back.Border.Type=TVTest::Theme::BORDER_SOLID;
	m_Theme.TabStyle.Back.Border.Color.Set(128,128,128);
	m_Theme.TabStyle.Fore.Fill.Type=TVTest::Theme::FILL_SOLID;
	m_Theme.TabStyle.Fore.Fill.Solid.Color.Set(0,0,0);
	m_Theme.CurTabStyle.Back.Fill.Type=TVTest::Theme::FILL_SOLID;
	m_Theme.CurTabStyle.Back.Fill.Solid.Color.Set(224,224,224);
	m_Theme.CurTabStyle.Back.Border.Type=TVTest::Theme::BORDER_SOLID;
	m_Theme.CurTabStyle.Back.Border.Color.Set(128,128,128);
	m_Theme.CurTabStyle.Fore.Fill.Type=TVTest::Theme::FILL_SOLID;
	m_Theme.CurTabStyle.Fore.Fill.Solid.Color.Set(0,0,0);
	m_Theme.TabMarginStyle=m_Theme.TabStyle;
	m_Theme.TabMarginStyle.Back.Border.Type=TVTest::Theme::BORDER_NONE;
	m_Theme.BackColor=RGB(192,192,192);
	m_Theme.BorderColor=RGB(128,128,128);
}


CPanelForm::~CPanelForm()
{
	Destroy();
	for (int i=0;i<m_NumWindows;i++)
		delete m_pWindowList[i];
}


bool CPanelForm::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,m_pszClassName,TEXT("ƒpƒlƒ‹"),m_hinst);
}


void CPanelForm::SetVisible(bool fVisible)
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->OnVisibleChange(fVisible);
	CBasicWindow::SetVisible(fVisible);
}


void CPanelForm::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);
}


void CPanelForm::NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	m_Style.NormalizeStyle(pStyleManager);
}


void CPanelForm::SetTheme(const TVTest::Theme::CThemeManager *pThemeManager)
{
	PanelFormTheme Theme;

	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_PANEL_TAB,
							&Theme.TabStyle);
	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_PANEL_CURTAB,
							&Theme.CurTabStyle);
	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_PANEL_TABMARGIN,
							&Theme.TabMarginStyle);
	Theme.BackColor=
		pThemeManager->GetColor(CColorScheme::COLOR_PANELBACK);
	Theme.BorderColor=
		pThemeManager->GetColor(CColorScheme::COLOR_PANELTABLINE);

	SetPanelFormTheme(Theme);
}


bool CPanelForm::AddWindow(CPage *pWindow,int ID,LPCTSTR pszTitle)
{
	if (m_NumWindows==MAX_WINDOWS)
		return false;
	m_pWindowList[m_NumWindows]=new CWindowInfo(pWindow,ID,pszTitle);
	m_TabOrder[m_NumWindows]=m_NumWindows;
	m_NumWindows++;
	if (m_hwnd!=NULL) {
		CalcTabSize();
		Invalidate();
	}
	return true;
}


CPanelForm::CPage *CPanelForm::GetPageByIndex(int Index)
{
	if (Index<0 || Index>=m_NumWindows)
		return NULL;
	return m_pWindowList[Index]->m_pWindow;
}


CPanelForm::CPage *CPanelForm::GetPageByID(int ID)
{
	int Index=IDToIndex(ID);

	if (Index<0)
		return NULL;
	return m_pWindowList[Index]->m_pWindow;
}


bool CPanelForm::SetCurTab(int Index)
{
	if (Index<-1 || Index>=m_NumWindows)
		return false;
	if (m_CurTab!=Index) {
		if (m_CurTab>=0)
			m_pWindowList[m_CurTab]->m_pWindow->SetVisible(false);
		if (Index>=0) {
			RECT rc;

			GetClientRect(&rc);
			rc.top=m_TabHeight;
			TVTest::Style::Subtract(&rc,m_Style.ClientMargin);
			m_pWindowList[Index]->m_pWindow->SetPosition(&rc);
			m_pWindowList[Index]->m_pWindow->SetVisible(true);
			//SetFocus(m_pWindowList[Index]->m_pWindow->GetHandle());
		}
		m_CurTab=Index;
		Invalidate();
		Update();
	}
	return true;
}


int CPanelForm::IDToIndex(int ID) const
{
	for (int i=0;i<m_NumWindows;i++) {
		if (m_pWindowList[i]->m_ID==ID)
			return i;
	}
	return -1;
}


int CPanelForm::GetCurPageID() const
{
	if (m_CurTab<0)
		return -1;
	return m_pWindowList[m_CurTab]->m_ID;
}


bool CPanelForm::SetCurPageByID(int ID)
{
	int Index=IDToIndex(ID);

	if (Index<0)
		return false;
	return SetCurTab(Index);
}


bool CPanelForm::SetTabVisible(int ID,bool fVisible)
{
	int Index=IDToIndex(ID);

	if (Index<0)
		return false;
	if (m_pWindowList[Index]->m_fVisible!=fVisible) {
		m_pWindowList[Index]->m_fVisible=fVisible;
		if (m_hwnd!=NULL) {
			CalcTabSize();
			Invalidate();
		}
	}
	return true;
}


bool CPanelForm::GetTabVisible(int ID) const
{
	int Index=IDToIndex(ID);

	if (Index<0)
		return false;
	return m_pWindowList[Index]->m_fVisible;
}


bool CPanelForm::SetTabOrder(const int *pOrder)
{
	for (int i=0;i<m_NumWindows;i++) {
		int j;
		for (j=0;j<m_NumWindows;j++) {
			if (m_pWindowList[j]->m_ID==pOrder[i])
				break;
		}
		if (j==m_NumWindows)
			return false;
	}
	::CopyMemory(m_TabOrder,pOrder,m_NumWindows*sizeof(int));
	if (m_hwnd!=NULL)
		Invalidate();
	return true;
}


bool CPanelForm::GetTabInfo(int Index,TabInfo *pInfo) const
{
	if (Index<0 || Index>=m_NumWindows || pInfo==NULL)
		return false;
	const CWindowInfo *pWindowInfo=m_pWindowList[m_TabOrder[Index]];
	pInfo->ID=pWindowInfo->m_ID;
	pInfo->fVisible=pWindowInfo->m_fVisible;
	return true;
}


void CPanelForm::SetEventHandler(CEventHandler *pHandler)
{
	m_pEventHandler=pHandler;
}


bool CPanelForm::SetPanelFormTheme(const PanelFormTheme &Theme)
{
	m_Theme=Theme;
	if (m_hwnd!=NULL)
		Invalidate();
	return true;
}


bool CPanelForm::GetPanelFormTheme(PanelFormTheme *pTheme) const
{
	if (pTheme==NULL)
		return false;
	*pTheme=m_Theme;
	return true;
}


bool CPanelForm::SetTabFont(const LOGFONT *pFont)
{
	if (!m_Font.Create(pFont))
		return false;
	if (m_hwnd!=NULL) {
		CalcTabSize();
		RECT rc;
		GetClientRect(&rc);
		SendMessage(WM_SIZE,0,MAKELPARAM(rc.right,rc.bottom));
		Invalidate();
	}
	return true;
}


bool CPanelForm::SetPageFont(const LOGFONT *pFont)
{
	for (int i=0;i<m_NumWindows;i++)
		m_pWindowList[i]->m_pWindow->SetFont(pFont);
	return true;
}


LRESULT CPanelForm::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		InitializeUI();
		CalcTabSize();
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd,&ps);
			Draw(ps.hdc,ps.rcPaint);
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_SIZE:
		if (m_fFitTabWidth) {
			RECT rc;
			::SetRect(&rc,0,0,LOWORD(lParam),m_TabHeight);
			::InvalidateRect(hwnd,&rc,FALSE);
		}
		if (m_CurTab>=0) {
			RECT rc;
			::SetRect(&rc,0,m_TabHeight,LOWORD(lParam),HIWORD(lParam));
			TVTest::Style::Subtract(&rc,m_Style.ClientMargin);
			m_pWindowList[m_CurTab]->m_pWindow->SetPosition(&rc);
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			int Index=HitTest(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));

			if (Index>=0 && Index!=m_CurTab) {
				SetCurTab(Index);
				if (m_pEventHandler!=NULL)
					m_pEventHandler->OnSelChange();
			}
		}
		return 0;

	case WM_RBUTTONDOWN:
		if (m_pEventHandler!=NULL) {
			POINT pt;
			RECT rc;

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			GetClientRect(&rc);
			if (::PtInRect(&rc,pt)) {
				if (pt.y<m_TabHeight)
					m_pEventHandler->OnTabRButtonDown(pt.x,pt.y);
				else
					m_pEventHandler->OnRButtonDown();
				return 0;
			}
		}
		break;

	case WM_SETCURSOR:
		{
			POINT pt;

			::GetCursorPos(&pt);
			::ScreenToClient(hwnd,&pt);
			int Index=HitTest(pt.x,pt.y);
			if (Index>=0) {
				::SetCursor(::LoadCursor(NULL,IDC_HAND));
				return TRUE;
			}
		}
		break;

	case WM_KEYDOWN:
		if (m_pEventHandler!=NULL
				&& m_pEventHandler->OnKeyDown((UINT)wParam,(UINT)lParam))
			return 0;
		break;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


void CPanelForm::CalcTabSize()
{
	HDC hdc;
	HFONT hfontOld;
	int MaxWidth;
	SIZE sz;

	hdc=::GetDC(m_hwnd);
	m_TabHeight=m_Font.GetHeight(hdc)+m_Style.TabPadding.Top+m_Style.TabPadding.Bottom;
	hfontOld=DrawUtil::SelectObject(hdc,m_Font);
	MaxWidth=0;
	for (int i=0;i<m_NumWindows;i++) {
		const CWindowInfo *pWindow=m_pWindowList[i];
		if (pWindow->m_fVisible) {
			::GetTextExtentPoint32(hdc,pWindow->m_Title.data(),(int)pWindow->m_Title.length(),&sz);
			if (sz.cx>MaxWidth)
				MaxWidth=sz.cx;
		}
	}
	SelectFont(hdc,hfontOld);
	::ReleaseDC(m_hwnd,hdc);
	m_TabWidth=MaxWidth+m_Style.TabPadding.Left+m_Style.TabPadding.Right;
}


int CPanelForm::GetRealTabWidth() const
{
	if (m_fFitTabWidth) {
		int NumVisibleTabs=0;
		for (int i=0;i<m_NumWindows;i++) {
			if (m_pWindowList[i]->m_fVisible)
				NumVisibleTabs++;
		}
		RECT rc;
		GetClientRect(&rc);
		if (NumVisibleTabs*m_TabWidth>rc.right)
			return max(rc.right/NumVisibleTabs,16+m_Style.TabPadding.Left+m_Style.TabPadding.Right);
	}
	return m_TabWidth;
}


int CPanelForm::HitTest(int x,int y) const
{
	if (y<0 || y>=m_TabHeight)
		return -1;

	const int TabWidth=GetRealTabWidth();
	POINT pt;
	RECT rc;

	pt.x=x;
	pt.y=y;
	::SetRect(&rc,0,0,TabWidth,m_TabHeight);
	for (int i=0;i<m_NumWindows;i++) {
		int Index=m_TabOrder[i];
		if (m_pWindowList[Index]->m_fVisible) {
			if (::PtInRect(&rc,pt))
				return Index;
			::OffsetRect(&rc,TabWidth,0);
		}
	}
	return -1;
}


void CPanelForm::Draw(HDC hdc,const RECT &PaintRect)
{
	if (PaintRect.top<m_TabHeight) {
		const int TabWidth=GetRealTabWidth();
		COLORREF crOldTextColor;
		int OldBkMode;
		HFONT hfontOld;
		int i;
		RECT rc;
		HBRUSH hbrOld;
		HPEN hpen,hpenOld;

		hpen=::CreatePen(PS_SOLID,1,m_Theme.BorderColor);
		hpenOld=SelectPen(hdc,hpen);
		crOldTextColor=::GetTextColor(hdc);
		OldBkMode=::SetBkMode(hdc,TRANSPARENT);
		hfontOld=DrawUtil::SelectObject(hdc,m_Font);
		hbrOld=SelectBrush(hdc,::GetStockObject(NULL_BRUSH));
		rc.left=0;
		rc.top=0;
		rc.right=TabWidth;
		rc.bottom=m_TabHeight;
		for (i=0;i<m_NumWindows;i++) {
			int Index=m_TabOrder[i];
			const CWindowInfo *pWindow=m_pWindowList[Index];

			if (!pWindow->m_fVisible)
				continue;

			const bool fCur=Index==m_CurTab;
			const TVTest::Theme::Style &Style=fCur?m_Theme.CurTabStyle:m_Theme.TabStyle;
			RECT rcTab,rcText;

			rcTab=rc;
			if (fCur)
				rcTab.bottom++;
			TVTest::Theme::Draw(hdc,rcTab,Style.Back);
			if (!fCur) {
				::MoveToEx(hdc,rc.left,rc.bottom-1,NULL);
				::LineTo(hdc,rc.right,rc.bottom-1);
			}
			rcText=rc;
			TVTest::Style::Subtract(&rcText,m_Style.TabPadding);
			TVTest::Theme::Draw(hdc,rcText,Style.Fore,pWindow->m_Title.c_str(),
				DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
			rc.left=rc.right;
			rc.right=rc.left+TabWidth;
		}
		SelectBrush(hdc,hbrOld);
		SelectFont(hdc,hfontOld);
		::SetBkMode(hdc,OldBkMode);
		::SetTextColor(hdc,crOldTextColor);
		if (PaintRect.right>rc.left) {
			if (PaintRect.left>rc.left)
				rc.left=PaintRect.left;
			rc.right=PaintRect.right;
			TVTest::Theme::Draw(hdc,rc,m_Theme.TabMarginStyle.Back);
			::MoveToEx(hdc,rc.left,rc.bottom-1,NULL);
			::LineTo(hdc,rc.right,rc.bottom-1);
		}
		SelectPen(hdc,hpenOld);
		::DeleteObject(hpen);
	}
	if (PaintRect.bottom>m_TabHeight) {
		RECT rc;

		rc.left=PaintRect.left;
		rc.top=max(PaintRect.top,(long)m_TabHeight);
		rc.right=PaintRect.right;
		rc.bottom=PaintRect.bottom;
		DrawUtil::Fill(hdc,&rc,m_Theme.BackColor);
	}
}




CPanelForm::CWindowInfo::CWindowInfo(CPage *pWindow,int ID,LPCTSTR pszTitle)
	: m_pWindow(pWindow)
	, m_ID(ID)
	, m_Title(pszTitle)
	, m_fVisible(true)
{
}


CPanelForm::CWindowInfo::~CWindowInfo()
{
}




CPanelForm::CPage::CPage()
{
}


CPanelForm::CPage::~CPage()
{
}


bool CPanelForm::CPage::GetDefaultFont(LOGFONT *pFont)
{
	return ::GetObject(::GetStockObject(DEFAULT_GUI_FONT),sizeof(LOGFONT),pFont)==sizeof(LOGFONT);
}


HFONT CPanelForm::CPage::CreateDefaultFont()
{
	LOGFONT lf;

	if (!GetDefaultFont(&lf))
		return NULL;
	return ::CreateFontIndirect(&lf);
}


bool CPanelForm::CPage::CreateDefaultFont(DrawUtil::CFont *pFont)
{
	LOGFONT lf;

	if (!GetDefaultFont(&lf))
		return false;
	return pFont->Create(&lf);
}




CPanelForm::PanelFormStyle::PanelFormStyle()
	: TabPadding(3)
	, ClientMargin(4)
{
}


void CPanelForm::PanelFormStyle::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	pStyleManager->Get(TEXT("panel.tab.padding"),&TabPadding);
	pStyleManager->Get(TEXT("panel.client.margin"),&ClientMargin);
}


void CPanelForm::PanelFormStyle::NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	pStyleManager->ToPixels(&TabPadding);
	pStyleManager->ToPixels(&ClientMargin);
}
