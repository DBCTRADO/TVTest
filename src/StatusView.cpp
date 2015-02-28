#include "stdafx.h"
#include "TVTest.h"
#include "StatusView.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CStatusItem::CStatusItem(int ID,const SizeValue &DefaultWidth)
	: m_pStatus(NULL)
	, m_ID(ID)
	, m_DefaultWidth(DefaultWidth)
	, m_Width(-1)
	, m_MinWidth(8)
	, m_MaxWidth(-1)
	, m_ActualWidth(-1)
	, m_MinHeight(0)
	, m_fVisible(true)
	, m_fBreak(false)
	, m_Style(0)
{
}


int CStatusItem::GetIndex() const
{
	if (m_pStatus!=NULL) {
		for (int i=0;i<m_pStatus->NumItems();i++) {
			if (m_pStatus->GetItem(i)==this)
				return i;
		}
	}
	return -1;
}


bool CStatusItem::GetRect(RECT *pRect) const
{
	if (m_pStatus==NULL)
		return false;
	return m_pStatus->GetItemRect(m_ID,pRect);
}


bool CStatusItem::GetClientRect(RECT *pRect) const
{
	if (m_pStatus==NULL)
		return false;
	return m_pStatus->GetItemClientRect(m_ID,pRect);
}


bool CStatusItem::SetWidth(int Width)
{
	if (Width<0)
		return false;
	if (Width<m_MinWidth)
		m_Width=m_MinWidth;
	else if (m_MaxWidth>0 && Width<m_MaxWidth)
		m_Width=m_MaxWidth;
	else
		m_Width=Width;
	return true;
}


bool CStatusItem::SetActualWidth(int Width)
{
	if (Width<0)
		return false;
	const int OldWidth=m_ActualWidth;
	if (Width<m_MinWidth)
		m_ActualWidth=m_MinWidth;
	else if (m_MaxWidth>0 && Width<m_MaxWidth)
		m_ActualWidth=m_MaxWidth;
	else
		m_ActualWidth=Width;
	if (OldWidth>=0 && OldWidth!=m_ActualWidth)
		OnSizeChanged();
	return true;
}


void CStatusItem::SetVisible(bool fVisible)
{
	if (m_fVisible!=fVisible) {
		m_fVisible=fVisible;
		OnVisibilityChanged();
	}
	OnPresentStatusChange(fVisible && m_pStatus!=NULL && m_pStatus->GetVisible());
}


void CStatusItem::SetItemStyle(unsigned int Style)
{
	m_Style=Style;
}


void CStatusItem::SetItemStyle(unsigned int Mask,unsigned int Style)
{
	m_Style=(m_Style & ~Mask) | (Style & Mask);
}


bool CStatusItem::Update()
{
	if (m_pStatus==NULL)
		return false;
	m_pStatus->UpdateItem(m_ID);
	return true;
}


void CStatusItem::Redraw()
{
	if (m_pStatus!=NULL)
		m_pStatus->RedrawItem(m_ID);
}


bool CStatusItem::GetMenuPos(POINT *pPos,UINT *pFlags,RECT *pExcludeRect)
{
	if (m_pStatus==NULL)
		return false;

	RECT rc;

	if (!GetRect(&rc))
		return false;

	MapWindowRect(m_pStatus->GetHandle(),NULL,&rc);

	if (pFlags!=NULL)
		*pFlags=0;

	if (pPos!=NULL) {
		pPos->x=rc.left;
		pPos->y=rc.bottom;
	}

	if (pExcludeRect!=NULL) {
		*pExcludeRect=rc;
		*pFlags|=TPM_VERTICAL;
	}

	return true;
}


void CStatusItem::DrawText(HDC hdc,const RECT &Rect,LPCTSTR pszText,DWORD Flags) const
{
	RECT rc=Rect;
	::DrawText(hdc,pszText,-1,&rc,
			   ((Flags&DRAWTEXT_HCENTER)!=0?DT_CENTER:DT_LEFT) |
			   DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
}


void CStatusItem::DrawIcon(HDC hdc,const RECT &Rect,DrawUtil::CMonoColorIconList &IconList,
						   int IconIndex,bool fEnabled) const
{
	if (hdc==NULL)
		return;

	TVTest::Style::Size IconSize=m_pStatus->GetIconSize();
	COLORREF cr=::GetTextColor(hdc);
	//if (!fEnabled)
	//	cr=MixColor(cr,::GetBkColor(hdc));
	IconList.Draw(hdc,
				  Rect.left+((Rect.right-Rect.left)-IconSize.Width)/2,
				  Rect.top+((Rect.bottom-Rect.top)-IconSize.Height)/2,
				  IconSize.Width,IconSize.Height,
				  IconIndex,cr,fEnabled?255:128);
}




CIconStatusItem::CIconStatusItem(int ID,int DefaultWidth)
	: CStatusItem(ID,SizeValue(DefaultWidth,SIZE_PIXEL))
{
	m_MinWidth=DefaultWidth;
}


void CIconStatusItem::NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	m_MinWidth=m_pStatus->GetIconSize().Width;
	if (m_Width<m_MinWidth)
		m_Width=m_MinWidth;
}




CStatusView::CEventHandler::CEventHandler()
	: m_pStatusView(NULL)
{
}


CStatusView::CEventHandler::~CEventHandler()
{
	if (m_pStatusView!=NULL)
		m_pStatusView->SetEventHandler(NULL);
}




const LPCTSTR CStatusView::CLASS_NAME=APP_NAME TEXT(" Status");
HINSTANCE CStatusView::m_hinst=NULL;


bool CStatusView::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=::LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=CLASS_NAME;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CStatusView::CStatusView()
	: m_Font(/*DrawUtil::FONT_STATUS*/DrawUtil::FONT_DEFAULT)
	, m_FontHeight(0)
	, m_ItemHeight(0)
	, m_fMultiRow(false)
	, m_MaxRows(2)
	, m_Rows(1)
	, m_fSingleMode(false)
	, m_HotItem(-1)
	, m_fOnButtonDown(false)
	, m_pEventHandler(NULL)
	, m_fBufferedPaint(false)
	, m_fAdjustSize(true)
{
	m_Theme.ItemStyle.Back.Fill.Type=TVTest::Theme::FILL_SOLID;
	m_Theme.ItemStyle.Back.Fill.Solid.Color.Set(192,192,192);
	m_Theme.ItemStyle.Back.Border.Type=TVTest::Theme::BORDER_NONE;
	m_Theme.ItemStyle.Fore.Fill.Type=TVTest::Theme::FILL_SOLID;
	m_Theme.ItemStyle.Fore.Fill.Solid.Color.Set(0,0,0);
	m_Theme.HighlightItemStyle.Back.Fill.Type=TVTest::Theme::FILL_SOLID;
	m_Theme.HighlightItemStyle.Back.Fill.Solid.Color.Set(128,128,128);
	m_Theme.HighlightItemStyle.Back.Border.Type=TVTest::Theme::BORDER_NONE;
	m_Theme.HighlightItemStyle.Fore.Fill.Type=TVTest::Theme::FILL_SOLID;
	m_Theme.HighlightItemStyle.Fore.Fill.Solid.Color.Set(255,255,255);
	m_Theme.Border.Type=TVTest::Theme::BORDER_RAISED;
	m_Theme.Border.Color.Set(192,192,192);
}


CStatusView::~CStatusView()
{
	Destroy();

	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pStatusView=NULL;

	for (size_t i=0;i<m_ItemList.size();i++)
		delete m_ItemList[i];
}


bool CStatusView::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 CLASS_NAME,NULL,m_hinst);
}


void CStatusView::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);

	for (auto itr=m_ItemList.begin();itr!=m_ItemList.end();++itr)
		(*itr)->SetStyle(pStyleManager);
}


void CStatusView::NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	m_Style.NormalizeStyle(pStyleManager);

	for (auto itr=m_ItemList.begin();itr!=m_ItemList.end();++itr)
		(*itr)->NormalizeStyle(pStyleManager);
}


void CStatusView::SetTheme(const TVTest::Theme::CThemeManager *pThemeManager)
{
	StatusViewTheme Theme;
	GetStatusViewThemeFromThemeManager(pThemeManager,&Theme);
	SetStatusViewTheme(Theme);

	for (auto itr=m_ItemList.begin();itr!=m_ItemList.end();++itr)
		(*itr)->SetTheme(pThemeManager);
}


const CStatusItem *CStatusView::GetItem(int Index) const
{
	if (Index<0 || (size_t)Index>=m_ItemList.size())
		return NULL;
	return m_ItemList[Index];
}


CStatusItem *CStatusView::GetItem(int Index)
{
	if (Index<0 || (size_t)Index>=m_ItemList.size())
		return NULL;
	return m_ItemList[Index];
}


const CStatusItem *CStatusView::GetItemByID(int ID) const
{
	int Index=IDToIndex(ID);

	if (Index<0)
		return NULL;
	return m_ItemList[Index];
}


CStatusItem *CStatusView::GetItemByID(int ID)
{
	int Index=IDToIndex(ID);

	if (Index<0)
		return NULL;
	return m_ItemList[Index];
}


bool CStatusView::AddItem(CStatusItem *pItem)
{
	if (pItem==NULL)
		return false;

	m_ItemList.push_back(pItem);

	pItem->m_pStatus=this;

	if (m_hwnd!=NULL) {
		if (pItem->GetWidth()<0)
			pItem->SetWidth(CalcItemPixelSize(pItem->GetDefaultWidth()));
		pItem->SetActualWidth(pItem->GetWidth());
	}

	return true;
}


int CStatusView::IDToIndex(int ID) const
{
	for (size_t i=0;i<m_ItemList.size();i++) {
		if (m_ItemList[i]->GetID()==ID)
			return (int)i;
	}
	return -1;
}


int CStatusView::IndexToID(int Index) const
{
	if (Index<0 || (size_t)Index>=m_ItemList.size())
		return -1;
	return m_ItemList[Index]->GetID();
}


LRESULT CStatusView::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			InitializeUI();

			m_FontHeight=CalcFontHeight();
			m_ItemHeight=CalcItemHeight();

			for (auto itr=m_ItemList.begin();itr!=m_ItemList.end();++itr) {
				CStatusItem *pItem=*itr;
				if (pItem->GetWidth()<0)
					pItem->SetWidth(CalcItemPixelSize(pItem->GetDefaultWidth()));
				pItem->SetActualWidth(pItem->GetWidth());
			}

			LPCREATESTRUCT pcs=reinterpret_cast<LPCREATESTRUCT>(lParam);
			RECT rc;

			::SetRectEmpty(&rc);
			rc.bottom=m_ItemHeight;
			TVTest::Theme::AddBorderRect(m_Theme.Border,&rc);
			::AdjustWindowRectEx(&rc,pcs->style,FALSE,pcs->dwExStyle);
			::SetWindowPos(hwnd,NULL,0,0,pcs->cx,rc.bottom-rc.top,
						   SWP_NOZORDER | SWP_NOMOVE);

			m_HotItem=-1;
			m_MouseLeaveTrack.Initialize(hwnd);

			m_CapturedItem=-1;
		}
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd,&ps);
			if (m_fBufferedPaint) {
				HDC hdc=m_BufferedPaint.Begin(ps.hdc,&ps.rcPaint,true);

				if (hdc!=NULL) {
					Draw(hdc,&ps.rcPaint);
					m_BufferedPaint.SetOpaque();
					m_BufferedPaint.End();
				} else {
					Draw(ps.hdc,&ps.rcPaint);
				}
			} else {
				Draw(ps.hdc,&ps.rcPaint);
			}
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_SIZE:
		AdjustSize();
		return 0;

	case WM_MOUSEMOVE:
		{
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			RECT rc;

			if (::GetCapture()!=hwnd) {
				if (m_fSingleMode)
					break;

				POINT pt;
				pt.x=x;
				pt.y=y;
				int i;
				for (i=0;i<(int)m_ItemList.size();i++) {
					if (!m_ItemList[i]->GetVisible())
						continue;
					GetItemRectByIndex(i,&rc);
					if (::PtInRect(&rc,pt))
						break;
				}
				if (i==(int)m_ItemList.size())
					i=-1;
				if (i!=m_HotItem)
					SetHotItem(i);
				m_MouseLeaveTrack.OnMouseMove();
			}

			if (m_HotItem>=0) {
				GetItemRectByIndex(m_HotItem,&rc);
				x-=rc.left;
				y-=rc.top;
				m_ItemList[m_HotItem]->OnMouseMove(x,y);
			}
		}
		return 0;

	case WM_MOUSELEAVE:
		{
			bool fLeave=m_MouseLeaveTrack.OnMouseLeave();
			if (!m_fOnButtonDown) {
				if (m_HotItem>=0)
					SetHotItem(-1);
				if (fLeave && m_pEventHandler)
					m_pEventHandler->OnMouseLeave();
			}
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
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONDBLCLK:
		if (m_HotItem>=0) {
			CStatusItem *pItem=m_ItemList[m_HotItem];
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			RECT rc;

			GetItemRectByIndex(m_HotItem,&rc);
			x-=rc.left;
			y-=rc.top;

			bool fCaptured=::GetCapture()==hwnd;

			m_fOnButtonDown=true;

			switch (uMsg) {
			case WM_LBUTTONDOWN:
				pItem->OnLButtonDown(x,y);
				break;
			case WM_LBUTTONDBLCLK:
				pItem->OnLButtonDoubleClick(x,y);
				break;
			case WM_RBUTTONDOWN:
				pItem->OnRButtonDown(x,y);
				break;
			case WM_RBUTTONDBLCLK:
				pItem->OnRButtonDoubleClick(x,y);
				break;
			case WM_MBUTTONDOWN:
				pItem->OnMButtonDown(x,y);
				break;
			case WM_MBUTTONDBLCLK:
				pItem->OnMButtonDoubleClick(x,y);
				break;
			}

			m_fOnButtonDown=false;

			if (!fCaptured && ::GetCapture()==hwnd)
				m_CapturedItem=m_HotItem;

			if (!m_MouseLeaveTrack.IsClientTrack()) {
				POINT pt;

				::GetCursorPos(&pt);
				::ScreenToClient(hwnd,&pt);
				::GetClientRect(hwnd,&rc);
				if (::PtInRect(&rc,pt)) {
					::SendMessage(hwnd,WM_MOUSEMOVE,0,MAKELPARAM((SHORT)pt.x,(SHORT)pt.y));
				} else {
					SetHotItem(-1);
					if (m_pEventHandler)
						m_pEventHandler->OnMouseLeave();
				}
			}
		}
		return 0;

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
		if (m_HotItem>=0) {
			CStatusItem *pItem=m_ItemList[m_HotItem];
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			RECT rc;

			GetItemRectByIndex(m_HotItem,&rc);
			x-=rc.left;
			y-=rc.top;
			switch (uMsg) {
			case WM_LBUTTONUP:
				pItem->OnLButtonUp(x,y);
				break;
			case WM_RBUTTONUP:
				pItem->OnRButtonUp(x,y);
				break;
			case WM_MBUTTONUP:
				pItem->OnMButtonUp(x,y);
				break;
			}
		}

		if (::GetCapture()==hwnd) {
			::ReleaseCapture();
		}
		return 0;

	case WM_MOUSEHOVER:
		if (m_HotItem>=0) {
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			RECT rc;

			GetItemRectByIndex(m_HotItem,&rc);
			x-=rc.left;
			y-=rc.top;
			if (m_ItemList[m_HotItem]->OnMouseHover(x,y)) {
				TRACKMOUSEEVENT tme;
				tme.cbSize=sizeof(TRACKMOUSEEVENT);
				tme.dwFlags=TME_HOVER;
				tme.hwndTrack=hwnd;
				tme.dwHoverTime=HOVER_DEFAULT;
				::TrackMouseEvent(&tme);
			}
		}
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT) {
			if (m_HotItem>=0) {
				SetCursor(LoadCursor(NULL,IDC_HAND));
				return TRUE;
			}
		}
		break;

	case WM_CAPTURECHANGED:
		if (m_CapturedItem>=0) {
			CStatusItem *pItem=GetItem(m_CapturedItem);

			m_CapturedItem=-1;
			if (pItem!=NULL)
				pItem->OnCaptureReleased();
		}
		return 0;

	case WM_NOTIFY:
		if (m_HotItem>=0)
			return m_ItemList[m_HotItem]->OnNotifyMessage(reinterpret_cast<LPNMHDR>(lParam));
		break;

	case WM_DISPLAYCHANGE:
		m_Offscreen.Destroy();
		return 0;

	case WM_DESTROY:
		m_Offscreen.Destroy();
		return 0;
	}

	return CCustomWindow::OnMessage(hwnd,uMsg,wParam,lParam);
}


bool CStatusView::UpdateItem(int ID)
{
	CStatusItem *pItem=GetItemByID(ID);

	if (pItem==NULL || !pItem->UpdateContent())
		return false;

	RedrawItem(ID);

	return true;
}


void CStatusView::RedrawItem(int ID)
{
	if (m_hwnd!=NULL) {
		RECT rc;

		GetItemRect(ID,&rc);
		if (rc.right>rc.left)
			Invalidate(&rc);
	}
}


bool CStatusView::GetItemRect(int ID,RECT *pRect) const
{
	int Index;

	Index=IDToIndex(ID);
	if (Index<0)
		return false;
	return GetItemRectByIndex(Index,pRect);
}


bool CStatusView::GetItemRectByIndex(int Index,RECT *pRect) const
{
	if (Index<0 || (size_t)Index>=m_ItemList.size())
		return false;

	RECT rc;
	GetClientRect(&rc);
	TVTest::Theme::SubtractBorderRect(m_Theme.Border,&rc);
	if (m_Rows>1)
		rc.bottom=rc.top+m_ItemHeight;
	const int HorzMargin=m_Style.ItemPadding.Horz();
	int Left=rc.left;
	const CStatusItem *pItem;
	for (int i=0;i<Index;i++) {
		pItem=m_ItemList[i];
		if (pItem->m_fBreak) {
			rc.left=Left;
			rc.top=rc.bottom;
			rc.bottom+=m_ItemHeight;
		} else if (pItem->GetVisible()) {
			rc.left+=pItem->GetActualWidth()+HorzMargin;
		}
	}
	rc.right=rc.left;
	pItem=m_ItemList[Index];
	if (pItem->GetVisible())
		rc.right+=pItem->GetActualWidth()+HorzMargin;
	*pRect=rc;
	return true;
}


bool CStatusView::GetItemClientRect(int ID,RECT *pRect) const
{
	RECT rc;

	if (!GetItemRect(ID,&rc))
		return false;
	TVTest::Style::Subtract(&rc,m_Style.ItemPadding);
	if (rc.right<rc.left)
		rc.right=rc.left;
	if (rc.bottom<rc.top)
		rc.bottom=rc.top;
	*pRect=rc;
	return true;
}


int CStatusView::GetItemHeight() const
{
	RECT rc;

	if (m_Rows>1)
		return m_ItemHeight;
	GetClientRect(&rc);
	TVTest::Theme::SubtractBorderRect(m_Theme.Border,&rc);
	return rc.bottom-rc.top;
}


int CStatusView::CalcItemHeight(const DrawUtil::CFont &Font) const
{
	return CalcItemHeight(CalcFontHeight(Font));
}


const TVTest::Style::Margins &CStatusView::GetItemPadding() const
{
	return m_Style.ItemPadding;
}


const TVTest::Style::Size &CStatusView::GetIconSize() const
{
	return m_Style.IconSize;
}


int CStatusView::GetIntegralWidth() const
{
	int Width;

	Width=0;
	for (size_t i=0;i<m_ItemList.size();i++) {
		if (m_ItemList[i]->GetVisible())
			Width+=m_ItemList[i]->GetWidth()+m_Style.ItemPadding.Horz();
	}
	RECT rc;
	TVTest::Theme::GetBorderWidths(m_Theme.Border,&rc);
	return Width+rc.left+rc.right;
}


void CStatusView::SetVisible(bool fVisible)
{
	if (m_HotItem>=0)
		SetHotItem(-1);
	CBasicWindow::SetVisible(fVisible);
	for (size_t i=0;i<m_ItemList.size();i++)
		m_ItemList[i]->OnPresentStatusChange(fVisible && m_ItemList[i]->GetVisible());
}


bool CStatusView::AdjustSize()
{
	if (m_hwnd==NULL || !m_fAdjustSize)
		return false;

	int OldRows=m_Rows;
	RECT rcWindow,rc;

	CalcLayout();
	GetPosition(&rcWindow);
	::SetRectEmpty(&rc);
	rc.bottom=m_ItemHeight*m_Rows;
	TVTest::Theme::AddBorderRect(m_Theme.Border,&rc);
	CalcPositionFromClientRect(&rc);
	int Height=rc.bottom-rc.top;
	if (Height!=rcWindow.bottom-rcWindow.top) {
		::SetWindowPos(m_hwnd,NULL,0,0,rcWindow.right-rcWindow.left,Height,
					   SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
		Invalidate();
		if (m_pEventHandler!=NULL)
			m_pEventHandler->OnHeightChanged(Height);
	} else {
		Invalidate();
	}

	return true;
}


void CStatusView::SetSingleText(LPCTSTR pszText)
{
	if (pszText!=NULL) {
		m_SingleText=pszText;
		m_fSingleMode=true;
		SetHotItem(-1);
	} else {
		if (!m_fSingleMode)
			return;
		m_SingleText.clear();
		m_fSingleMode=false;
	}
	if (m_hwnd!=NULL)
		Redraw(NULL,RDW_INVALIDATE | RDW_UPDATENOW);
}


bool CStatusView::GetStatusViewThemeFromThemeManager(
	const TVTest::Theme::CThemeManager *pThemeManager,StatusViewTheme *pTheme)
{
	if (pThemeManager==NULL || pTheme==NULL)
		return false;

	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_STATUSBAR_ITEM,
							&pTheme->ItemStyle);
	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_STATUSBAR_BOTTOMITEM,
							&pTheme->BottomItemStyle);
	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_STATUSBAR_ITEM_HOT,
							&pTheme->HighlightItemStyle);
	pThemeManager->GetBorderStyle(TVTest::Theme::CThemeManager::STYLE_STATUSBAR,
								  &pTheme->Border);

	return true;
}


bool CStatusView::SetStatusViewTheme(const StatusViewTheme &Theme)
{
	m_Theme=Theme;
	if (m_hwnd!=NULL) {
		AdjustSize();
		Invalidate();
	}
	return true;
}


bool CStatusView::GetStatusViewTheme(StatusViewTheme *pTheme) const
{
	if (pTheme==NULL)
		return false;
	*pTheme=m_Theme;
	return true;
}


bool CStatusView::SetFont(const LOGFONT *pFont)
{
	if (!m_Font.Create(pFont))
		return false;
	if (m_hwnd!=NULL) {
		m_FontHeight=CalcFontHeight();
		m_ItemHeight=CalcItemHeight();
		AdjustSize();
		Invalidate();
	}
	return true;
}


bool CStatusView::GetFont(LOGFONT *pFont) const
{
	return m_Font.GetLogFont(pFont);
}


int CStatusView::GetCurItem() const
{
	if (m_HotItem<0)
		return -1;
	return m_ItemList[m_HotItem]->GetID();
}


bool CStatusView::SetMultiRow(bool fMultiRow)
{
	if (m_fMultiRow!=fMultiRow) {
		m_fMultiRow=fMultiRow;
		if (m_hwnd!=NULL)
			AdjustSize();
	}
	return true;
}


bool CStatusView::SetMaxRows(int MaxRows)
{
	if (MaxRows<1)
		return false;
	if (m_MaxRows!=MaxRows) {
		m_MaxRows=MaxRows;
		if (m_hwnd!=NULL)
			AdjustSize();
	}
	return true;
}


int CStatusView::CalcHeight(int Width) const
{
	std::vector<const CStatusItem*> ItemList;
	ItemList.reserve(m_ItemList.size());
	for (auto itr=m_ItemList.begin();itr!=m_ItemList.end();++itr) {
		const CStatusItem *pItem=*itr;

		if (pItem->GetVisible())
			ItemList.push_back(pItem);
	}

	RECT rcBorder;
	TVTest::Theme::GetBorderWidths(m_Theme.Border,&rcBorder);

	int Rows=CalcRows(ItemList,Width-(rcBorder.left+rcBorder.right));

	return m_ItemHeight*Rows+rcBorder.top+rcBorder.bottom;
}


bool CStatusView::SetEventHandler(CEventHandler *pEventHandler)
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pStatusView=NULL;
	if (pEventHandler!=NULL)
		pEventHandler->m_pStatusView=this;
	m_pEventHandler=pEventHandler;
	return true;
}


bool CStatusView::SetItemOrder(const int *pOrderList)
{
	std::vector<CStatusItem*> NewList;

	for (size_t i=0;i<m_ItemList.size();i++) {
		CStatusItem *pItem=GetItem(IDToIndex(pOrderList[i]));

		if (pItem==NULL)
			return false;
		NewList.push_back(pItem);
		for (size_t j=0;j<i;j++) {
			if (NewList[i]==NewList[j])
				return false;
		}
	}
	m_ItemList=NewList;
	if (m_hwnd!=NULL && !m_fSingleMode) {
		AdjustSize();
		Invalidate();
	}
	return true;
}


bool CStatusView::DrawItemPreview(CStatusItem *pItem,HDC hdc,const RECT &ItemRect,
								  bool fHighlight,HFONT hfont) const
{
	if (pItem==NULL || hdc==NULL)
		return false;

	HFONT hfontOld;
	int OldBkMode;
	COLORREF crOldTextColor,crOldBkColor;
	const TVTest::Theme::Style &Style=
		fHighlight?m_Theme.HighlightItemStyle:m_Theme.ItemStyle;

	if (hfont!=NULL)
		hfontOld=SelectFont(hdc,hfont);
	else
		hfontOld=DrawUtil::SelectObject(hdc,m_Font);
	OldBkMode=::SetBkMode(hdc,TRANSPARENT);
	crOldTextColor=::SetTextColor(hdc,Style.Fore.Fill.GetSolidColor());
	crOldBkColor=::SetBkColor(hdc,Style.Back.Fill.GetSolidColor());
	TVTest::Theme::Draw(hdc,ItemRect,Style.Back);
	RECT rcDraw=ItemRect;
	TVTest::Style::Subtract(&rcDraw,m_Style.ItemPadding);
	unsigned int Flags=CStatusItem::DRAW_PREVIEW;
	if (fHighlight)
		Flags|=CStatusItem::DRAW_HIGHLIGHT;
	pItem->Draw(hdc,ItemRect,rcDraw,Flags);
	::SetBkColor(hdc,crOldBkColor);
	::SetTextColor(hdc,crOldTextColor);
	::SetBkMode(hdc,OldBkMode);
	SelectFont(hdc,hfontOld);
	return true;
}


bool CStatusView::EnableBufferedPaint(bool fEnable)
{
	m_fBufferedPaint=fEnable;
	return true;
}


void CStatusView::EnableSizeAdjustment(bool fEnable)
{
	if (m_fAdjustSize!=fEnable) {
		m_fAdjustSize=fEnable;
		if (fEnable && m_hwnd!=NULL)
			AdjustSize();
	}
}


void CStatusView::SetHotItem(int Item)
{
	if (Item<0 || (size_t)Item>=m_ItemList.size())
		Item=-1;
	if (m_HotItem!=Item) {
		int OldHotItem=m_HotItem;

		m_HotItem=Item;
		if (OldHotItem>=0) {
			m_ItemList[OldHotItem]->OnFocus(false);
			RedrawItem(IndexToID(OldHotItem));
		}
		if (m_HotItem>=0) {
			m_ItemList[m_HotItem]->OnFocus(true);
			RedrawItem(IndexToID(m_HotItem));
		}

		TRACKMOUSEEVENT tme;
		tme.cbSize=sizeof(TRACKMOUSEEVENT);
		tme.dwFlags=TME_HOVER;
		if (m_HotItem<0)
			tme.dwFlags|=TME_CANCEL;
		tme.hwndTrack=m_hwnd;
		tme.dwHoverTime=HOVER_DEFAULT;
		::TrackMouseEvent(&tme);
	}
}


void CStatusView::Draw(HDC hdc,const RECT *pPaintRect)
{
	const int HorzMargin=m_Style.ItemPadding.Horz();
	RECT rcClient,rc;
	HDC hdcDst;
	HFONT hfontOld;
	COLORREF crOldTextColor,crOldBkColor;
	int OldBkMode;

	GetClientRect(&rcClient);
	rc=rcClient;
	TVTest::Theme::SubtractBorderRect(m_Theme.Border,&rc);
	const int ItemHeight=m_Rows>1?m_ItemHeight:rc.bottom-rc.top;

	if (!m_fSingleMode) {
		int MaxWidth=0;
		for (size_t i=0;i<m_ItemList.size();i++) {
			const CStatusItem *pItem=m_ItemList[i];
			if (pItem->GetVisible() && pItem->GetActualWidth()>MaxWidth)
				MaxWidth=pItem->GetActualWidth();
		}
		if (MaxWidth+HorzMargin>m_Offscreen.GetWidth()
				|| ItemHeight>m_Offscreen.GetHeight())
			m_Offscreen.Create(MaxWidth+HorzMargin,ItemHeight);
		hdcDst=m_Offscreen.GetDC();
		if (hdcDst==NULL)
			hdcDst=hdc;
	} else {
		hdcDst=hdc;
	}

	hfontOld=DrawUtil::SelectObject(hdcDst,m_Font);
	OldBkMode=::SetBkMode(hdcDst,TRANSPARENT);
	crOldTextColor=::GetTextColor(hdcDst);
	crOldBkColor=::GetBkColor(hdcDst);

	if (m_Rows>1)
		rc.bottom=rc.top+ItemHeight;

	if (m_fSingleMode) {
		RECT rcRow=rc;

		for (int i=0;i<m_Rows;i++) {
			const TVTest::Theme::Style &Style=i==0?m_Theme.ItemStyle:m_Theme.BottomItemStyle;

			TVTest::Theme::Draw(hdcDst,rcRow,Style.Back.Fill);
			rcRow.top=rcRow.bottom;
			rcRow.bottom+=ItemHeight;
		}
		rc.left+=m_Style.ItemPadding.Left;
		rc.right-=m_Style.ItemPadding.Right;
		TVTest::Theme::Draw(hdcDst,rc,m_Theme.ItemStyle.Fore,m_SingleText.c_str(),
							DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
	} else {
		const int Left=rc.left;
		int Row=0;

		rc.right=Left;
		for (int i=0;i<(int)m_ItemList.size();i++) {
			CStatusItem *pItem=m_ItemList[i];

			if (pItem->GetVisible()) {
				rc.left=rc.right;
				rc.right=rc.left+pItem->GetActualWidth()+HorzMargin;
				if (rc.right>pPaintRect->left && rc.left<pPaintRect->right) {
					const bool fHighlight=i==m_HotItem;
					const TVTest::Theme::Style &Style=
						fHighlight?m_Theme.HighlightItemStyle:
						Row==0?m_Theme.ItemStyle:m_Theme.BottomItemStyle;
					RECT rcItem=rc;
					if (hdcDst!=hdc)
						::OffsetRect(&rcItem,-rcItem.left,-rcItem.top);
					TVTest::Theme::Draw(hdcDst,rcItem,Style.Back);
					::SetTextColor(hdcDst,Style.Fore.Fill.GetSolidColor());
					::SetBkColor(hdcDst,Style.Back.Fill.GetSolidColor());
					RECT rcDraw=rcItem;
					TVTest::Style::Subtract(&rcDraw,m_Style.ItemPadding);
					unsigned int Flags=0;
					if (fHighlight)
						Flags|=CStatusItem::DRAW_HIGHLIGHT;
					if (Row>0)
						Flags|=CStatusItem::DRAW_BOTTOM;
					pItem->Draw(hdcDst,rcItem,rcDraw,Flags);
					if (hdcDst!=hdc)
						m_Offscreen.CopyTo(hdc,&rc);
				}
			}
			if (m_Rows>1 && pItem->m_fBreak) {
				if (rc.right<pPaintRect->right) {
					rc.left=max(rc.right,pPaintRect->left);
					rc.right=pPaintRect->right;
					TVTest::Theme::Draw(hdc,rc,
										Row==0?m_Theme.ItemStyle.Back.Fill:m_Theme.BottomItemStyle.Back.Fill);
				}
				rc.right=Left;
				rc.top=rc.bottom;
				rc.bottom+=ItemHeight;
				Row++;
			}
		}
		if (rc.right<pPaintRect->right) {
			rc.left=max(rc.right,pPaintRect->left);
			rc.right=pPaintRect->right;
			TVTest::Theme::Draw(hdc,rc,
								Row==0?m_Theme.ItemStyle.Back.Fill:m_Theme.BottomItemStyle.Back.Fill);
		}
	}

	TVTest::Theme::Draw(hdc,rcClient,m_Theme.Border);

	::SetBkColor(hdcDst,crOldBkColor);
	::SetTextColor(hdcDst,crOldTextColor);
	::SetBkMode(hdcDst,OldBkMode);
	::SelectObject(hdcDst,hfontOld);
}


void CStatusView::CalcLayout()
{
	std::vector<CStatusItem*> ItemList;
	ItemList.reserve(m_ItemList.size());
	for (auto itr=m_ItemList.begin();itr!=m_ItemList.end();++itr) {
		CStatusItem *pItem=*itr;

		pItem->m_fBreak=false;
		pItem->SetActualWidth(pItem->GetWidth());

		if (pItem->GetVisible())
			ItemList.push_back(pItem);
	}

	RECT rc;
	GetClientRect(&rc);
	TVTest::Theme::SubtractBorderRect(m_Theme.Border,&rc);
	const int MaxRowWidth=rc.right-rc.left;

	m_Rows=CalcRows(ItemList,MaxRowWidth);

	CStatusItem *pVariableItem=NULL;
	int RowWidth=0;
	for (size_t i=0;i<ItemList.size();i++) {
		CStatusItem *pItem=ItemList[i];

		if (pItem->IsVariableWidth())
			pVariableItem=pItem;
		RowWidth+=pItem->GetActualWidth()+m_Style.ItemPadding.Horz();
		if (pItem->m_fBreak || i+1==ItemList.size()) {
			if (pVariableItem!=NULL) {
				int Add=MaxRowWidth-RowWidth;
				if (Add>0)
					pVariableItem->SetActualWidth(pVariableItem->GetActualWidth()+Add);
			}
			pVariableItem=NULL;
			RowWidth=0;
		}
	}
}


int CStatusView::CalcRows(const std::vector<const CStatusItem*> &ItemList,int MaxRowWidth) const
{
	const int MaxRegularRows=m_fMultiRow?m_MaxRows:1;
	int Rows=1;
	int RowWidth=0;

	for (size_t i=0;i<ItemList.size();i++) {
		const CStatusItem *pItem=ItemList[i];

		if (pItem->IsFullRow()) {
			if (pItem->IsForceFullRow()
					|| (Rows<MaxRegularRows
						&& (i==0 || i+1==ItemList.size() || Rows+1<MaxRegularRows))) {
				Rows++;
				if (i+1<ItemList.size() && Rows<MaxRegularRows)
					Rows++;
				RowWidth=0;
				continue;
			}
		}

		const int ItemWidth=pItem->GetWidth()+m_Style.ItemPadding.Horz();
		if (Rows<MaxRegularRows && RowWidth>0 && RowWidth+ItemWidth>MaxRowWidth) {
			Rows++;
			RowWidth=ItemWidth;
		} else {
			RowWidth+=ItemWidth;
		}
	}

	return Rows;
}


int CStatusView::CalcRows(const std::vector<CStatusItem*> &ItemList,int MaxRowWidth)
{
	const int MaxRegularRows=m_fMultiRow?m_MaxRows:1;
	int Rows=1;
	int RowWidth=0;

	for (size_t i=0;i<ItemList.size();i++) {
		CStatusItem *pItem=ItemList[i];

		if (pItem->IsFullRow()) {
			if (pItem->IsForceFullRow()
					|| (Rows<MaxRegularRows
						&& (i==0 || i+1==ItemList.size() || Rows+1<MaxRegularRows))) {
				if (i>0)
					ItemList[i-1]->m_fBreak=true;
				pItem->SetActualWidth(MaxRowWidth-m_Style.ItemPadding.Horz());
				pItem->m_fBreak=true;
				Rows++;
				if (i+1<ItemList.size() && Rows<MaxRegularRows)
					Rows++;
				RowWidth=0;
				continue;
			}
		}

		const int ItemWidth=pItem->GetWidth()+m_Style.ItemPadding.Horz();
		if (Rows<MaxRegularRows && RowWidth>0 && RowWidth+ItemWidth>MaxRowWidth) {
			if (i>0)
				ItemList[i-1]->m_fBreak=true;
			Rows++;
			RowWidth=ItemWidth;
		} else {
			RowWidth+=ItemWidth;
		}
	}

	return Rows;
}


int CStatusView::CalcFontHeight(const DrawUtil::CFont &Font) const
{
	HDC hdc=::GetDC(m_hwnd);
	int FontHeight=Font.GetHeight(hdc,false);
	::ReleaseDC(m_hwnd,hdc);
	return FontHeight;
}


int CStatusView::CalcFontHeight() const
{
	return CalcFontHeight(m_Font);
}


int CStatusView::CalcItemHeight(int FontHeight) const
{
	return max(FontHeight+m_Style.TextExtraHeight,m_Style.IconSize.Height)+m_Style.ItemPadding.Vert();
}


int CStatusView::CalcItemHeight() const
{
	return CalcItemHeight(m_FontHeight);
}


int CStatusView::CalcItemPixelSize(const CStatusItem::SizeValue &Size) const
{
	switch (Size.Unit) {
	case CStatusItem::SIZE_PIXEL:
		return Size.Value;

	case CStatusItem::SIZE_EM:
		return ::MulDiv(Size.Value,m_FontHeight,CStatusItem::EM_FACTOR);
	}

	return 0;
}




CStatusView::StatusViewStyle::StatusViewStyle()
	: ItemPadding(4,2,4,2)
	, TextExtraHeight(4)
	, IconSize(16,16)
{
}


void CStatusView::StatusViewStyle::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	pStyleManager->Get(TEXT("status-bar.item.padding"),&ItemPadding);
	pStyleManager->Get(TEXT("status-bar.item.text.extra-height"),&TextExtraHeight);
	pStyleManager->Get(TEXT("status-bar.item.icon"),&IconSize);
}


void CStatusView::StatusViewStyle::NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	pStyleManager->ToPixels(&ItemPadding);
	pStyleManager->ToPixels(&TextExtraHeight);
	pStyleManager->ToPixels(&IconSize);
}
