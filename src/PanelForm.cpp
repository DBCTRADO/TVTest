#include "stdafx.h"
#include "TVTest.h"
#include "PanelForm.h"
#include "Common/DebugDef.h"




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
	: m_TabStyle(TABSTYLE_TEXT_ONLY)
	, m_TabHeight(0)
	, m_TabWidth(0)
	, m_fFitTabWidth(true)
	, m_CurTab(-1)
	, m_PrevActivePageID(-1)
	, m_pEventHandler(NULL)
	, m_fEnableTooltip(true)
{
	m_WindowPosition.Width=200;
	m_WindowPosition.Height=240;
}


CPanelForm::~CPanelForm()
{
	Destroy();

	for (auto it=m_WindowList.begin();it!=m_WindowList.end();++it) {
		(*it)->m_pWindow->OnFormDelete();
		delete *it;
	}
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


bool CPanelForm::AddPage(const PageInfo &Info)
{
	m_WindowList.push_back(new CWindowInfo(Info));
	m_TabOrder.push_back((int)m_WindowList.size()-1);
	if (m_hwnd!=NULL) {
		CalcTabSize();
		Invalidate();
		UpdateTooltip();
	}
	return true;
}


CPanelForm::CPage *CPanelForm::GetPageByIndex(int Index)
{
	if (Index<0 || (size_t)Index>=m_WindowList.size())
		return NULL;
	return m_WindowList[Index]->m_pWindow;
}


CPanelForm::CPage *CPanelForm::GetPageByID(int ID)
{
	int Index=IDToIndex(ID);

	if (Index<0)
		return NULL;
	return m_WindowList[Index]->m_pWindow;
}


bool CPanelForm::SetCurTab(int Index)
{
	if (Index<-1 || (size_t)Index>=m_WindowList.size())
		return false;

	if (!m_WindowList[Index]->m_fVisible)
		return false;

	if (m_CurTab!=Index) {
		if (m_CurTab>=0) {
			CWindowInfo *pWindow=m_WindowList[m_CurTab];
			m_PrevActivePageID=pWindow->m_ID;
			pWindow->m_pWindow->OnDeactivate();
			pWindow->m_pWindow->SetVisible(false);
		}

		if (Index>=0) {
			CWindowInfo *pWindow=m_WindowList[Index];
			RECT rc;

			GetClientRect(&rc);
			rc.top=m_TabHeight;
			TVTest::Style::Subtract(&rc,m_Style.ClientMargin);
			pWindow->m_pWindow->SetPosition(&rc);
			pWindow->m_pWindow->OnActivate();
			pWindow->m_pWindow->SetVisible(true);
		}

		m_CurTab=Index;

		Invalidate();
		//Update();

		if (m_pEventHandler!=NULL)
			m_pEventHandler->OnSelChange();
	}

	return true;
}


int CPanelForm::IDToIndex(int ID) const
{
	for (int i=0;i<(int)m_WindowList.size();i++) {
		if (m_WindowList[i]->m_ID==ID)
			return i;
	}
	return -1;
}


int CPanelForm::GetCurPageID() const
{
	if (m_CurTab<0)
		return -1;
	return m_WindowList[m_CurTab]->m_ID;
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

	CWindowInfo *pWindow=m_WindowList[Index];
	if (pWindow->m_fVisible!=fVisible) {
		pWindow->m_fVisible=fVisible;
		pWindow->m_pWindow->OnVisibilityChanged(fVisible);

		if (!fVisible && m_CurTab==Index) {
			int CurTab=-1;
			if (m_PrevActivePageID>=0) {
				int i=IDToIndex(m_PrevActivePageID);
				if (i>=0 && m_WindowList[i]->m_fVisible)
					CurTab=i;
			}
			if (CurTab<0) {
				for (int i=0;i<(int)m_WindowList.size();i++) {
					if (m_WindowList[i]->m_fVisible) {
						CurTab=i;
						break;
					}
				}
			}
			SetCurTab(CurTab);
		}

		if (m_hwnd!=NULL) {
			CalcTabSize();
			Invalidate();
			UpdateTooltip();
		}
	}

	return true;
}


bool CPanelForm::GetTabVisible(int ID) const
{
	int Index=IDToIndex(ID);

	if (Index<0)
		return false;
	return m_WindowList[Index]->m_fVisible;
}


bool CPanelForm::SetTabOrder(const int *pOrder,int Count)
{
	if (pOrder==NULL || Count<0)
		return false;

	std::vector<int> TabOrder;

	for (int i=0;i<Count;i++) {
		size_t j;
		for (j=0;j<m_WindowList.size();j++) {
			if (m_WindowList[j]->m_ID==pOrder[i])
				break;
		}
		if (j==m_WindowList.size())
			return false;
		TabOrder.push_back(pOrder[i]);
	}

	m_TabOrder=TabOrder;

	if (m_hwnd!=NULL) {
		Invalidate();
		UpdateTooltip();
	}

	return true;
}


bool CPanelForm::GetTabInfo(int Index,TabInfo *pInfo) const
{
	if (Index<0 || (size_t)Index>=m_TabOrder.size() || pInfo==NULL)
		return false;
	const CWindowInfo *pWindowInfo=m_WindowList[m_TabOrder[Index]];
	pInfo->ID=pWindowInfo->m_ID;
	pInfo->fVisible=pWindowInfo->m_fVisible;
	return true;
}


int CPanelForm::GetTabID(int Index) const
{
	if (Index<0 || (size_t)Index>=m_TabOrder.size())
		return -1;
	return m_WindowList[m_TabOrder[Index]]->m_ID;
}


bool CPanelForm::GetTabTitle(int ID,TVTest::String *pTitle) const
{
	if (pTitle==NULL)
		return false;

	int Index=IDToIndex(ID);
	if (Index<0) {
		pTitle->clear();
		return false;
	}

	*pTitle=m_WindowList[Index]->m_Title;

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
		SendSizeMessage();
		Invalidate();
	}
	return true;
}


bool CPanelForm::SetPageFont(const LOGFONT *pFont)
{
	for (size_t i=0;i<m_WindowList.size();i++)
		m_WindowList[i]->m_pWindow->SetFont(pFont);
	return true;
}


bool CPanelForm::GetPageClientRect(RECT *pRect) const
{
	if (pRect==NULL)
		return false;
	if (!GetClientRect(pRect))
		return false;
	pRect->top=m_TabHeight;
	TVTest::Style::Subtract(pRect,m_Style.ClientMargin);
	return true;
}


bool CPanelForm::SetTabStyle(TabStyle Style)
{
	if (m_TabStyle!=Style) {
		m_TabStyle=Style;
		if (m_hwnd!=NULL) {
			CalcTabSize();
			SendSizeMessage();
			Invalidate();
		}
	}
	return true;
}


bool CPanelForm::SetIconImage(HBITMAP hbm,int Width,int Height)
{
	if (hbm==NULL)
		return false;
	if (!m_Icons.Create(hbm,Width,Height))
		return false;
	if (m_hwnd!=NULL)
		Invalidate();
	return true;
}


SIZE CPanelForm::GetIconDrawSize() const
{
	SIZE sz;
	sz.cx=m_Style.TabIconSize.Width;
	sz.cy=m_Style.TabIconSize.Height;
	return sz;
}


bool CPanelForm::EnableTooltip(bool fEnable)
{
	if (m_fEnableTooltip!=fEnable) {
		m_fEnableTooltip=fEnable;
		if (m_hwnd!=NULL)
			UpdateTooltip();
	}
	return true;
}


LRESULT CPanelForm::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			InitializeUI();
			if (!m_Font.IsCreated()) {
				LOGFONT lf;
				DrawUtil::GetDefaultUIFont(&lf);
				m_Font.Create(&lf);
			}
			CalcTabSize();

			m_Tooltip.Create(hwnd);
			UpdateTooltip();
		}
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
			UpdateTooltip();
		}

		if (m_CurTab>=0) {
			RECT rc;
			::SetRect(&rc,0,m_TabHeight,LOWORD(lParam),HIWORD(lParam));
			TVTest::Style::Subtract(&rc,m_Style.ClientMargin);
			m_WindowList[m_CurTab]->m_pWindow->SetPosition(&rc);
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			int Index=HitTest(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));

			if (Index>=0) {
				if (Index!=m_CurTab)
					SetCurTab(Index);

				const CPage *pPage=m_WindowList[Index]->m_pWindow;
				if (pPage->NeedKeyboardFocus())
					::SetFocus(pPage->GetHandle());
			}
		}
		return 0;

	case WM_RBUTTONUP:
		if (m_pEventHandler!=NULL) {
			POINT pt;
			RECT rc;

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			GetClientRect(&rc);
			if (::PtInRect(&rc,pt)) {
				if (pt.y<m_TabHeight)
					m_pEventHandler->OnTabRButtonUp(pt.x,pt.y);
				else
					m_pEventHandler->OnRButtonUp(pt.x,pt.y);
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
				::SetCursor(GetActionCursor());
				return TRUE;
			}
		}
		break;

	case WM_KEYDOWN:
		if (m_pEventHandler!=NULL
				&& m_pEventHandler->OnKeyDown((UINT)wParam,(UINT)lParam))
			return 0;
		break;

	case WM_DESTROY:
		m_Tooltip.Destroy();
		return 0;
	}

	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


void CPanelForm::CalcTabSize()
{
	HDC hdc;
	HFONT hfontOld;
	int IconHeight,LabelHeight;

	hdc=::GetDC(m_hwnd);
	IconHeight=m_Style.TabIconSize.Height+m_Style.TabIconMargin.Vert();
	LabelHeight=m_Font.GetHeight(hdc)+m_Style.TabLabelMargin.Vert();
	m_TabHeight=max(IconHeight,LabelHeight)+m_Style.TabPadding.Vert();
	hfontOld=DrawUtil::SelectObject(hdc,m_Font);

	m_TabWidth=m_Style.TabPadding.Horz();

	if (m_TabStyle!=TABSTYLE_ICON_ONLY) {
		int MaxWidth=0;
		SIZE sz;

		for (size_t i=0;i<m_WindowList.size();i++) {
			const CWindowInfo *pWindow=m_WindowList[i];
			if (pWindow->m_fVisible) {
				::GetTextExtentPoint32(hdc,pWindow->m_Title.data(),(int)pWindow->m_Title.length(),&sz);
				if (sz.cx>MaxWidth)
					MaxWidth=sz.cx;
			}
		}
		m_TabWidth+=MaxWidth+m_Style.TabLabelMargin.Horz();
	}

	if (m_TabStyle!=TABSTYLE_TEXT_ONLY) {
		m_TabWidth+=m_Style.TabIconSize.Width+m_Style.TabIconMargin.Horz();
		if (m_TabStyle==TABSTYLE_ICON_AND_TEXT)
			m_TabWidth+=m_Style.TabIconLabelMargin;
	}

	SelectFont(hdc,hfontOld);
	::ReleaseDC(m_hwnd,hdc);
}


int CPanelForm::GetRealTabWidth() const
{
	if (m_fFitTabWidth) {
		int NumVisibleTabs=0;
		for (size_t i=0;i<m_WindowList.size();i++) {
			if (m_WindowList[i]->m_fVisible)
				NumVisibleTabs++;
		}
		RECT rc;
		GetClientRect(&rc);
		if (NumVisibleTabs*m_TabWidth>rc.right) {
			int Width=rc.right/NumVisibleTabs;
			int MinWidth=m_Style.TabPadding.Horz();
			if (m_TabStyle!=TABSTYLE_TEXT_ONLY)
				MinWidth+=m_Style.TabIconSize.Width;
			else
				MinWidth+=16;
			return max(Width,MinWidth);
		}
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
	for (size_t i=0;i<m_TabOrder.size();i++) {
		int Index=m_TabOrder[i];
		if (m_WindowList[Index]->m_fVisible) {
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

		for (i=0;i<(int)m_TabOrder.size();i++) {
			int Index=m_TabOrder[i];
			const CWindowInfo *pWindow=m_WindowList[Index];

			if (!pWindow->m_fVisible)
				continue;

			const bool fCur=Index==m_CurTab;
			const TVTest::Theme::Style &Style=fCur?m_Theme.CurTabStyle:m_Theme.TabStyle;
			RECT rcTab,rcContent,rcText;

			rcTab=rc;
			if (fCur)
				rcTab.bottom++;
			TVTest::Theme::Draw(hdc,rcTab,Style.Back);
			if (!fCur) {
				::MoveToEx(hdc,rc.left,rc.bottom-1,NULL);
				::LineTo(hdc,rc.right,rc.bottom-1);
			}

			rcContent=rc;
			TVTest::Style::Subtract(&rcContent,m_Style.TabPadding);
			rcText=rcContent;

			if (m_TabStyle!=TABSTYLE_TEXT_ONLY) {
				RECT rcIcon=rcContent;
				TVTest::Style::Subtract(&rcIcon,m_Style.TabIconMargin);
				int x=rcIcon.left;
				int y=rcIcon.top+((rcIcon.bottom-rcIcon.top)-m_Style.TabIconSize.Height)/2;
				if (m_TabStyle==TABSTYLE_ICON_ONLY)
					x+=((rcIcon.right-rcIcon.left)-m_Style.TabIconSize.Width)/2;
				bool fIcon;
				if (pWindow->m_Icon>=0) {
					m_Icons.Draw(
						hdc,x,y,
						m_Style.TabIconSize.Width,
						m_Style.TabIconSize.Height,
						pWindow->m_Icon,
						Style.Fore.Fill.GetSolidColor());
					fIcon=true;
				} else {
					fIcon=pWindow->m_pWindow->DrawIcon(
						hdc,x,y,
						m_Style.TabIconSize.Width,
						m_Style.TabIconSize.Height,
						Style.Fore.Fill.GetSolidColor());
				}
				if (fIcon) {
					rcText.left=rcIcon.left+m_Style.TabIconSize.Width+
						m_Style.TabIconMargin.Right+m_Style.TabIconLabelMargin;
				}
			}

			if (m_TabStyle!=TABSTYLE_ICON_ONLY) {
				TVTest::Style::Subtract(&rcText,m_Style.TabLabelMargin);
				TVTest::Theme::Draw(hdc,rcText,Style.Fore,pWindow->m_Title.c_str(),
					(m_TabStyle!=TABSTYLE_TEXT_ONLY ? DT_LEFT : DT_CENTER)
						| DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
			}

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


void CPanelForm::UpdateTooltip()
{
	const int TabWidth=GetRealTabWidth();

	if (!m_fEnableTooltip || !m_fFitTabWidth
			|| (m_TabStyle!=TABSTYLE_ICON_ONLY && TabWidth>=m_TabWidth)) {
		m_Tooltip.Enable(false);
		return;
	}

	int ToolCount=m_Tooltip.NumTools();
	int TabCount=0;
	RECT rc;

	rc.left=0;
	rc.top=0;
	rc.bottom=m_TabHeight;

	for (size_t i=0;i<m_TabOrder.size();i++) {
		const CWindowInfo *pInfo=m_WindowList[m_TabOrder[i]];

		if (pInfo->m_fVisible) {
			rc.right=rc.left+TabWidth;
			if (TabCount<ToolCount) {
				m_Tooltip.SetToolRect(TabCount,rc);
				m_Tooltip.SetText(TabCount,pInfo->m_Title.c_str());
			} else {
				m_Tooltip.AddTool(TabCount,rc,pInfo->m_Title.c_str());
			}
			TabCount++;
			rc.left=rc.right;
		}
	}

	if (ToolCount>TabCount) {
		for (int i=ToolCount-1;i>=TabCount;i--)
			m_Tooltip.DeleteTool(i);
	}

	m_Tooltip.Enable(true);
}




CPanelForm::CWindowInfo::CWindowInfo(const PageInfo &Info)
	: m_pWindow(Info.pPage)
	, m_Title(Info.pszTitle)
	, m_ID(Info.ID)
	, m_Icon(Info.Icon)
	, m_fVisible(Info.fVisible)
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
	return DrawUtil::GetDefaultUIFont(pFont);
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
	, TabIconSize(16,16)
	, TabIconMargin(0)
	, TabLabelMargin(0)
	, TabIconLabelMargin(4)
	, ClientMargin(4)
{
}


void CPanelForm::PanelFormStyle::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	pStyleManager->Get(TEXT("panel.tab.padding"),&TabPadding);
	pStyleManager->Get(TEXT("panel.tab.icon"),&TabIconSize);
	pStyleManager->Get(TEXT("panel.tab.icon.margin"),&TabIconMargin);
	pStyleManager->Get(TEXT("panel.tab.label.margin"),&TabLabelMargin);
	pStyleManager->Get(TEXT("panel.tab.icon-label-margin"),&TabIconLabelMargin);
	pStyleManager->Get(TEXT("panel.client.margin"),&ClientMargin);
}


void CPanelForm::PanelFormStyle::NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	pStyleManager->ToPixels(&TabPadding);
	pStyleManager->ToPixels(&TabIconSize);
	pStyleManager->ToPixels(&TabIconMargin);
	pStyleManager->ToPixels(&TabLabelMargin);
	pStyleManager->ToPixels(&TabIconLabelMargin);
	pStyleManager->ToPixels(&ClientMargin);
}
