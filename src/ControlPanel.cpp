#include "stdafx.h"
#include "TVTest.h"
#include "ControlPanel.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




const LPCTSTR CControlPanel::m_pszClassName=APP_NAME TEXT(" Control Panel");
HINSTANCE CControlPanel::m_hinst=NULL;


bool CControlPanel::Initialize(HINSTANCE hinst)
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
		wc.lpszClassName=m_pszClassName;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CControlPanel::CControlPanel()
	: m_MarginSize(2)
	, m_hwndMessage(NULL)
	, m_HotItem(-1)
{
	LOGFONT lf;
	GetDefaultFont(&lf);
	m_Font.Create(&lf);
	m_FontHeight=abs(lf.lfHeight);

	m_Theme.ItemStyle.Gradient.Type=Theme::GRADIENT_NORMAL;
	m_Theme.ItemStyle.Gradient.Direction=Theme::DIRECTION_VERT;
	m_Theme.ItemStyle.Gradient.Color1=RGB(0,0,0);
	m_Theme.ItemStyle.Gradient.Color2=RGB(0,0,0);
	m_Theme.ItemStyle.Border.Type=Theme::BORDER_NONE;
	m_Theme.ItemStyle.TextColor=RGB(255,255,255);
	m_Theme.OverItemStyle.Gradient.Type=Theme::GRADIENT_NORMAL;
	m_Theme.OverItemStyle.Gradient.Direction=Theme::DIRECTION_VERT;
	m_Theme.OverItemStyle.Gradient.Color1=RGB(255,255,255);
	m_Theme.OverItemStyle.Gradient.Color2=RGB(255,255,255);
	m_Theme.OverItemStyle.Border.Type=Theme::BORDER_NONE;
	m_Theme.OverItemStyle.TextColor=RGB(0,0,0);
	m_Theme.MarginColor=RGB(0,0,0);
}


CControlPanel::~CControlPanel()
{
	Destroy();
	for (size_t i=0;i<m_ItemList.size();i++)
		delete m_ItemList[i];
}


bool CControlPanel::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 m_pszClassName,TEXT("‘€ì"),m_hinst);
}


bool CControlPanel::AddItem(CControlPanelItem *pItem)
{
	if (pItem==NULL)
		return false;
	m_ItemList.push_back(pItem);
	pItem->m_pControlPanel=this;
	return true;
}


CControlPanelItem *CControlPanel::GetItem(int Index) const
{
	if (Index<0 || Index>=(int)m_ItemList.size())
		return NULL;
	return m_ItemList[Index];
}


bool CControlPanel::UpdateItem(int Index)
{
	RECT rc;

	if (Index<0 || Index>=(int)m_ItemList.size() || m_hwnd==NULL)
		return false;
	m_ItemList[Index]->GetPosition(&rc);
	Invalidate(&rc);
	return true;
}


bool CControlPanel::GetItemPosition(int Index,RECT *pRect) const
{
	if (Index<0 || Index>=(int)m_ItemList.size())
		return false;
	m_ItemList[Index]->GetPosition(pRect);
	return true;
}


void CControlPanel::UpdateLayout()
{
	RECT rc;
	int Width;
	int x,y;
	int MaxHeight;

	GetClientRect(&rc);
	Width=(rc.right-rc.left)-m_MarginSize*2;
	x=m_MarginSize;
	y=m_MarginSize;
	for (size_t i=0;i<m_ItemList.size();i++) {
		CControlPanelItem *pItem=m_ItemList[i];

		if (!pItem->GetVisible())
			continue;
		if (pItem->GetBreak()) {
			x=m_MarginSize;
			if (i>0)
				y+=MaxHeight;
			MaxHeight=0;
		}
		SIZE sz;
		pItem->CalcSize(Width,&sz);
		rc.left=x;
		rc.top=y;
		rc.right=x+sz.cx;
		rc.bottom=y+sz.cy;
		pItem->m_Position=rc;
		if (sz.cy>MaxHeight)
			MaxHeight=sz.cy;
		x+=sz.cx;
	}
}


bool CControlPanel::SetTheme(const ThemeInfo *pTheme)
{
	if (pTheme==NULL)
		return false;
	m_Theme=*pTheme;
	if (m_hwnd!=NULL)
		Invalidate();
	return true;
}


bool CControlPanel::GetTheme(ThemeInfo *pTheme) const
{
	if (pTheme==NULL)
		return false;
	*pTheme=m_Theme;
	return true;
}


bool CControlPanel::SetFont(const LOGFONT *pFont)
{
	if (!m_Font.Create(pFont))
		return false;
	if (m_hwnd!=NULL) {
		UpdateLayout();
		Invalidate();
	}
	return true;
}


void CControlPanel::SetSendMessageWindow(HWND hwnd)
{
	m_hwndMessage=hwnd;
}


void CControlPanel::SendCommand(int Command)
{
	if (m_hwndMessage!=NULL)
		::SendMessage(m_hwndMessage,WM_COMMAND,Command,(LPARAM)GetHandle());
}


bool CControlPanel::CheckRadioItem(int FirstID,int LastID,int CheckID)
{
	for (size_t i=0;i<m_ItemList.size();i++) {
		CControlPanelItem *pItem=m_ItemList[i];

		if (pItem->m_Command>=FirstID && pItem->m_Command<=LastID)
			pItem->m_fCheck=pItem->m_Command==CheckID;
	}
	if (m_hwnd!=NULL)
		Invalidate();
	return true;
}


bool CControlPanel::CalcTextSize(LPCTSTR pszText,SIZE *pSize)
{
	pSize->cx=0;
	pSize->cy=0;

	if (m_hwnd==NULL || pszText==NULL)
		return false;

	HDC hdc=::GetDC(m_hwnd);
	if (hdc==NULL)
		return false;
	HFONT hfontOld=DrawUtil::SelectObject(hdc,m_Font);
	::GetTextExtentPoint32(hdc,pszText,::lstrlen(pszText),pSize);
	SelectFont(hdc,hfontOld);
	::ReleaseDC(m_hwnd,hdc);
	return true;
}


void CControlPanel::Draw(HDC hdc,const RECT &PaintRect)
{
	RECT rcClient;
	GetClientRect(&rcClient);
	int Width=(rcClient.right-rcClient.left)-m_MarginSize*2;

	int MaxHeight;
	MaxHeight=0;
	for (int i=0;i<(int)m_ItemList.size();i++) {
		CControlPanelItem *pItem=m_ItemList[i];

		if (pItem->GetVisible()) {
			int Height=pItem->m_Position.bottom-pItem->m_Position.top;
			if (Height>MaxHeight)
				MaxHeight=Height;
		}
	}
	if (MaxHeight==0)
		return;
	if (!m_Offscreen.IsCreated()
			|| m_Offscreen.GetWidth()<Width
			|| m_Offscreen.GetHeight()<MaxHeight) {
		if (!m_Offscreen.Create(Width+32,MaxHeight,hdc))
			return;
	}

	HBRUSH hbrMargin=::CreateSolidBrush(m_Theme.MarginColor);
	HDC hdcOffscreen=m_Offscreen.GetDC();
	HFONT hfontOld;
	COLORREF crOldTextColor;
	int OldBkMode;
	RECT rc,rcOffscreen,rcDest;

	rc=rcClient;
	::InflateRect(&rc,-m_MarginSize,-m_MarginSize);
	DrawUtil::FillBorder(hdc,&rcClient,&rc,&PaintRect,hbrMargin);
	hfontOld=DrawUtil::SelectObject(hdcOffscreen,m_Font);
	crOldTextColor=::GetTextColor(hdcOffscreen);
	OldBkMode=::SetBkMode(hdcOffscreen,TRANSPARENT);
	::SetRect(&rcOffscreen,0,0,Width,MaxHeight);
	::SetRect(&rcDest,m_MarginSize,-1,m_MarginSize+Width,0);
	for (int i=0;i<(int)m_ItemList.size();i++) {
		CControlPanelItem *pItem=m_ItemList[i];

		if (!pItem->GetVisible())
			continue;
		pItem->GetPosition(&rc);
		if (rc.left<PaintRect.right && rc.right>PaintRect.left
				&& rc.top<PaintRect.bottom && rc.bottom>PaintRect.top) {
			COLORREF crText,crBack;

			if (rc.top!=rcDest.top) {
				if (rcDest.top>=0)
					m_Offscreen.CopyTo(hdc,&rcDest);
				::FillRect(hdcOffscreen,&rcOffscreen,hbrMargin);
				rcDest.top=rc.top;
			}
			if (rcDest.bottom<rc.bottom)
				rcDest.bottom=rc.bottom;
			::OffsetRect(&rc,-m_MarginSize,-rc.top);
			if (i==m_HotItem) {
				crText=m_Theme.OverItemStyle.TextColor;
				crBack=MixColor(m_Theme.OverItemStyle.Gradient.Color1,
								m_Theme.OverItemStyle.Gradient.Color2);
				Theme::DrawStyleBackground(hdcOffscreen,&rc,&m_Theme.OverItemStyle);
			} else {
				Theme::Style Style=m_Theme.ItemStyle;

				crText=Style.TextColor;
				crBack=MixColor(Style.Gradient.Color1,Style.Gradient.Color2);
				if (!pItem->GetEnable()) {
					crText=MixColor(crText,crBack);
				} else if (pItem->GetCheck()) {
					Style.Gradient.Color1=MixColor(Style.Gradient.Color1,
												   m_Theme.OverItemStyle.Gradient.Color1);
					Style.Gradient.Color2=MixColor(Style.Gradient.Color2,
												  m_Theme.OverItemStyle.Gradient.Color2);
					crBack=MixColor(Style.Gradient.Color1,Style.Gradient.Color2);
				}
				Theme::DrawStyleBackground(hdcOffscreen,&rc,&Style);
			}
			::SetTextColor(hdcOffscreen,crText);
			::SetBkColor(hdcOffscreen,crBack);
			pItem->Draw(hdcOffscreen,rc);
		}
	}
	if (rcDest.top>=0) {
		m_Offscreen.CopyTo(hdc,&rcDest);
		rc.top=rcDest.bottom;
	} else {
		rc.top=m_MarginSize;
	}
	if (rc.top<PaintRect.bottom) {
		rc.left=PaintRect.left;
		rc.right=PaintRect.right;
		rc.bottom=PaintRect.bottom;
		::FillRect(hdc,&rc,hbrMargin);
	}
	::SetBkMode(hdc,OldBkMode);
	::SetTextColor(hdc,crOldTextColor);
	SelectFont(hdc,hfontOld);
	::DeleteObject(hbrMargin);
}


LRESULT CControlPanel::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		m_HotItem=-1;
		m_fTrackMouseEvent=false;
		m_fOnButtonDown=false;
		return 0;

	case WM_SIZE:
		UpdateLayout();
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
			RECT rc;

			if (::GetCapture()==hwnd) {
				m_ItemList[m_HotItem]->GetPosition(&rc);
				x-=rc.left;
				y-=rc.top;
				m_ItemList[m_HotItem]->OnMouseMove(x,y);
			} else {
				int i;
				POINT pt;

				pt.x=x;
				pt.y=y;
				for (i=(int)m_ItemList.size()-1;i>=0;i--) {
					const CControlPanelItem *pItem=m_ItemList[i];

					if (pItem->GetVisible() && pItem->GetEnable()) {
						pItem->GetPosition(&rc);
						if (::PtInRect(&rc,pt))
							break;
					}
				}
				if (i!=m_HotItem) {
					int OldHotItem;

					OldHotItem=m_HotItem;
					m_HotItem=i;
					if (OldHotItem>=0)
						UpdateItem(OldHotItem);
					if (m_HotItem>=0)
						UpdateItem(m_HotItem);
				}
				if (!m_fTrackMouseEvent) {
					TRACKMOUSEEVENT tme;

					tme.cbSize=sizeof(TRACKMOUSEEVENT);
					tme.dwFlags=TME_LEAVE;
					tme.hwndTrack=hwnd;
					if (::TrackMouseEvent(&tme))
						m_fTrackMouseEvent=true;
				}
			}
		}
		return 0;

	case WM_MOUSELEAVE:
		if (!m_fOnButtonDown) {
			if (m_HotItem>=0) {
				int i=m_HotItem;

				m_HotItem=-1;
				UpdateItem(i);
			}
		}
		m_fTrackMouseEvent=false;
		return 0;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		{
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);

			if (m_HotItem>=0) {
				RECT rc;

				m_ItemList[m_HotItem]->GetPosition(&rc);
				x-=rc.left;
				y-=rc.top;
				m_fOnButtonDown=true;
				if (uMsg==WM_LBUTTONDOWN)
					m_ItemList[m_HotItem]->OnLButtonDown(x,y);
				else
					m_ItemList[m_HotItem]->OnRButtonDown(x,y);
				m_fOnButtonDown=false;
				if (!m_fTrackMouseEvent) {
					POINT pt;

					::GetCursorPos(&pt);
					::ScreenToClient(hwnd,&pt);
					::GetClientRect(hwnd,&rc);
					if (::PtInRect(&rc,pt)) {
						::SendMessage(hwnd,WM_MOUSEMOVE,0,MAKELPARAM(pt.x,pt.y));
					} else {
						::SendMessage(hwnd,WM_MOUSELEAVE,0,0);
					}
				}
			} else if (uMsg==WM_RBUTTONDOWN) {
				POINT pt;

				pt.x=x;
				pt.y=y;
				::MapWindowPoints(hwnd,GetParent(),&pt,1);
				::SendMessage(GetParent(),uMsg,wParam,MAKELPARAM(pt.x,pt.y));
			}
		}
		return 0;

	case WM_LBUTTONUP:
		if (GetCapture()==hwnd) {
			::ReleaseCapture();
		}
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT) {
			if (m_HotItem>=0) {
				::SetCursor(LoadCursor(NULL,IDC_HAND));
				return TRUE;
			}
		}
		break;

	case WM_DISPLAYCHANGE:
		m_Offscreen.Destroy();
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}




CControlPanelItem::CControlPanelItem()
	: m_Command(0)
	, m_fVisible(true)
	, m_fEnable(true)
	, m_fCheck(false)
	, m_fBreak(true)
	, m_pControlPanel(NULL)
{
	::SetRectEmpty(&m_Position);
}


CControlPanelItem::~CControlPanelItem()
{
}


void CControlPanelItem::GetPosition(int *pLeft,int *pTop,int *pWidth,int *pHeight) const
{
	if (pLeft)
		*pLeft=m_Position.left;
	if (pTop)
		*pTop=m_Position.top;
	if (pWidth)
		*pWidth=m_Position.right-m_Position.left;
	if (pHeight)
		*pHeight=m_Position.bottom-m_Position.top;
}


bool CControlPanelItem::SetPosition(int Left,int Top,int Width,int Height)
{
	if (Width<0 || Height<0)
		return false;
	m_Position.left=Left;
	m_Position.top=Top;
	m_Position.right=Left+Width;
	m_Position.bottom=Top+Height;
	return true;
}


void CControlPanelItem::GetPosition(RECT *pRect) const
{
	*pRect=m_Position;
}


void CControlPanelItem::SetVisible(bool fVisible)
{
	m_fVisible=fVisible;
}


void CControlPanelItem::SetEnable(bool fEnable)
{
	m_fEnable=fEnable;
}


void CControlPanelItem::SetCheck(bool fCheck)
{
	m_fCheck=fCheck;
}


void CControlPanelItem::SetBreak(bool fBreak)
{
	m_fBreak=fBreak;
}


void CControlPanelItem::CalcSize(int Width,SIZE *pSize)
{
	pSize->cx=Width;
	pSize->cy=m_Position.bottom-m_Position.top;
}


void CControlPanelItem::OnLButtonDown(int x,int y)
{
	m_pControlPanel->SendCommand(m_Command);
}


bool CControlPanelItem::CalcTextSize(LPCTSTR pszText,SIZE *pSize) const
{
	if (m_pControlPanel==NULL || pszText==NULL) {
		pSize->cx=0;
		pSize->cy=0;
		return false;
	}
	return m_pControlPanel->CalcTextSize(pszText,pSize);
}


void CControlPanelItem::GetMenuPos(POINT *pPos) const
{
	if (m_pControlPanel==NULL) {
		::GetCursorPos(pPos);
		return;
	}
	pPos->x=m_Position.left;
	pPos->y=m_Position.bottom;
	::ClientToScreen(m_pControlPanel->GetHandle(),pPos);
}
