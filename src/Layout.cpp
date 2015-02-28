#include "stdafx.h"
#include "TVTest.h"
#include "Layout.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


namespace Layout
{


CContainer::CContainer(int ID)
	: m_pBase(NULL)
	, m_ID(ID)
	, m_fVisible(false)
{
	::SetRectEmpty(&m_Position);
}


CContainer::~CContainer()
{
}


void CContainer::SetPosition(const RECT &Pos)
{
	m_Position=Pos;
}


void CContainer::GetMinSize(SIZE *pSize) const
{
	pSize->cx=0;
	pSize->cy=0;
}


void CContainer::SetVisible(bool fVisible)
{
	if (m_fVisible!=fVisible) {
		m_fVisible=fVisible;
		if (m_pBase!=NULL)
			m_pBase->Adjust();
	}
}


int CContainer::NumChildContainers() const
{
	return 0;
}


CContainer *CContainer::GetChildContainer(int Index) const
{
	return NULL;
}




CWindowContainer::CWindowContainer(int ID)
	: CContainer(ID)
	, m_pWindow(NULL)
	, m_MinWidth(0)
	, m_MinHeight(0)
{
}


CWindowContainer::~CWindowContainer()
{
}


void CWindowContainer::SetPosition(const RECT &Pos)
{
	CContainer::SetPosition(Pos);
	if (m_pWindow!=NULL)
		m_pWindow->SetPosition(&Pos);
}


void CWindowContainer::GetMinSize(SIZE *pSize) const
{
	pSize->cx=m_MinWidth;
	pSize->cy=m_MinHeight;
}


void CWindowContainer::SetVisible(bool fVisible)
{
	if (m_pWindow!=NULL) {
		if (!fVisible)
			m_pWindow->SetVisible(false);
		CContainer::SetVisible(fVisible);
		if (fVisible)
			m_pWindow->SetVisible(true);
	} else {
		CContainer::SetVisible(fVisible);
	}
}


void CWindowContainer::SetWindow(CBasicWindow *pWindow)
{
	m_pWindow=pWindow;
	if (pWindow!=NULL && m_pBase!=NULL
			&& pWindow->GetParent()!=m_pBase->GetHandle())
		pWindow->SetParent(m_pBase);
}


bool CWindowContainer::SetMinSize(int Width,int Height)
{
	m_MinWidth=max(Width,0);
	m_MinHeight=max(Height,0);
	return true;
}




CSplitter::CSplitter(int ID)
	: CContainer(ID)
	, m_Style(0)
	, m_AdjustPane(0)
	, m_BarPos(0)
	, m_BarWidth(4)
{
}


CSplitter::~CSplitter()
{
	for (int i=0;i<2;i++)
		delete m_PaneList[i].pContainer;
}


void CSplitter::SetPosition(const RECT &Pos)
{
	if (m_PaneList[0].pContainer!=NULL && m_PaneList[0].pContainer->GetVisible()
			&& m_PaneList[1].pContainer!=NULL
			&& m_AdjustPane==m_PaneList[0].pContainer->GetID()) {
		if ((m_Style&STYLE_FIXED)==0 || m_PaneList[1].FixedSize<0) {
			if ((m_Style&STYLE_VERT)==0)
				m_BarPos+=(Pos.right-Pos.left)-(m_Position.right-m_Position.left);
			else
				m_BarPos+=(Pos.bottom-Pos.top)-(m_Position.bottom-m_Position.top);
		} else {
			if ((m_Style&STYLE_VERT)==0)
				m_BarPos=(Pos.right-Pos.left)-m_PaneList[1].FixedSize;
			else
				m_BarPos=(Pos.bottom-Pos.top)-m_PaneList[1].FixedSize;
		}
		if (m_BarPos<0)
			m_BarPos=0;
	}
	CContainer::SetPosition(Pos);
	Adjust();
}


void CSplitter::GetMinSize(SIZE *pSize) const
{
	if (m_PaneList[0].pContainer!=NULL && m_PaneList[0].pContainer->GetVisible()) {
		m_PaneList[0].pContainer->GetMinSize(pSize);
		if (m_PaneList[1].pContainer!=NULL && m_PaneList[1].pContainer->GetVisible()) {
			SIZE sz;

			m_PaneList[1].pContainer->GetMinSize(&sz);
			if ((m_Style&STYLE_VERT)==0) {
				pSize->cx+=sz.cx;
				if (pSize->cy<sz.cy)
					pSize->cy=sz.cy;
				if ((m_Style&STYLE_FIXED)==0)
					pSize->cx+=m_BarWidth;
			} else {
				pSize->cy+=sz.cy;
				if (pSize->cx<sz.cx)
					pSize->cx=sz.cx;
				if ((m_Style&STYLE_FIXED)==0)
					pSize->cy+=m_BarWidth;
			}
		}
	} else if (m_PaneList[1].pContainer!=NULL && m_PaneList[1].pContainer->GetVisible()) {
		m_PaneList[1].pContainer->GetMinSize(pSize);
	} else {
		pSize->cx=0;
		pSize->cy=0;
	}
	return;
}


int CSplitter::NumChildContainers() const
{
	int NumChildren=0;

	for (int i=0;i<2;i++) {
		if (m_PaneList[i].pContainer!=NULL)
			NumChildren++;
	}
	return NumChildren;
}


CContainer *CSplitter::GetChildContainer(int Index) const
{
	int j=0;

	for (int i=0;i<2;i++) {
		if (m_PaneList[i].pContainer!=NULL) {
			if (Index==j)
				return m_PaneList[i].pContainer;
			j++;
		}
	}
	return NULL;
}


void CSplitter::OnLButtonDown(int x,int y)
{
	POINT pt={x,y};
	RECT rc;

	if ((m_Style&STYLE_FIXED)==0
			&& m_PaneList[0].pContainer!=NULL && m_PaneList[0].pContainer->GetVisible()
			&& m_PaneList[1].pContainer!=NULL && m_PaneList[1].pContainer->GetVisible()
			&& GetBarRect(&rc) && ::PtInRect(&rc,pt))
		::SetCapture(m_pBase->GetHandle());
}


void CSplitter::OnLButtonUp(int x,int y)
{
	if (::GetCapture()==m_pBase->GetHandle())
		::ReleaseCapture();
}


void CSplitter::OnMouseMove(int x,int y)
{
	POINT pt={x,y};
	RECT rc;
	LPCTSTR pszCursor;

	if (::GetCapture()==m_pBase->GetHandle()) {
		SIZE MinSize1,MinSize2;
		int BarPos;

		m_PaneList[0].pContainer->GetMinSize(&MinSize1);
		m_PaneList[1].pContainer->GetMinSize(&MinSize2);
		if ((m_Style&STYLE_VERT)==0) {
			BarPos=x-m_Position.left;
			if ((m_Position.right-m_Position.left)-BarPos-m_BarWidth<MinSize2.cx)
				BarPos=(m_Position.right-m_Position.left)-m_BarWidth-MinSize2.cx;
			if (BarPos<MinSize1.cx)
				BarPos=MinSize1.cx;
		} else {
			BarPos=y-m_Position.top;
			if ((m_Position.bottom-m_Position.top)-BarPos-m_BarWidth<MinSize2.cy)
				BarPos=(m_Position.bottom-m_Position.top)-m_BarWidth-MinSize2.cy;
			if (BarPos<MinSize1.cy)
				BarPos=MinSize1.cy;
		}
		if (m_BarPos!=BarPos) {
			m_BarPos=BarPos;
			Adjust();
		}
		pszCursor=(m_Style&STYLE_VERT)!=0?IDC_SIZENS:IDC_SIZEWE;
	} else {
		if ((m_Style&STYLE_FIXED)==0
				&& m_PaneList[0].pContainer!=NULL && m_PaneList[0].pContainer->GetVisible()
				&& m_PaneList[1].pContainer!=NULL && m_PaneList[1].pContainer->GetVisible()
				&& GetBarRect(&rc) && ::PtInRect(&rc,pt))
			pszCursor=(m_Style&STYLE_VERT)!=0?IDC_SIZENS:IDC_SIZEWE;
		else
			pszCursor=IDC_ARROW;
	}
	::SetCursor(::LoadCursor(NULL,pszCursor));
}


bool CSplitter::SetPane(int Index,CContainer *pContainer)
{
	if (Index<0 || Index>1)
		return false;
	if (m_PaneList[Index].pContainer!=NULL)
		delete m_PaneList[Index].pContainer;
	m_PaneList[Index].pContainer=pContainer;
	return true;
}


bool CSplitter::ReplacePane(int Index,CContainer *pContainer)
{
	if (Index<0 || Index>1)
		return false;
	m_PaneList[Index].pContainer=pContainer;
	return true;
}


CContainer *CSplitter::GetPane(int Index) const
{
	if (Index<0 || Index>1)
		return false;
	return m_PaneList[Index].pContainer;
}


CContainer *CSplitter::GetPaneByID(int ID) const
{
	int Index=IDToIndex(ID);

	if (Index<0)
		return NULL;
	return m_PaneList[Index].pContainer;
}


void CSplitter::SwapPane()
{
	PaneInfo Temp;

	Temp=m_PaneList[0];
	m_PaneList[0]=m_PaneList[1];
	m_PaneList[1]=Temp;
	if ((m_Style&STYLE_VERT)==0)
		m_BarPos=(m_Position.right-m_Position.left)-(m_BarPos+m_BarWidth);
	else
		m_BarPos=(m_Position.bottom-m_Position.top)-(m_BarPos+m_BarWidth);
	if (m_BarPos<0)
		m_BarPos=0;
	if (m_pBase!=NULL)
		Adjust();
}


bool CSplitter::SetPaneSize(int ID,int Size)
{
	int Index=IDToIndex(ID);

	if (Index<0)
		return false;
	if ((m_Style&STYLE_VERT)==0) {
		if (Index==0) {
			m_BarPos=Size;
		} else {
			m_BarPos=(m_Position.right-m_Position.left)-Size;
			if ((m_Style&STYLE_FIXED)==0)
				m_BarPos-=m_BarWidth;
			if (m_BarPos<0)
				m_BarPos=0;
		}
	} else {
		if (Index==0) {
			m_BarPos=Size;
		} else {
			m_BarPos=(m_Position.bottom-m_Position.top)-Size;
			if ((m_Style&STYLE_FIXED)==0)
				m_BarPos-=m_BarWidth;
			if (m_BarPos<0)
				m_BarPos=0;
		}
	}
	m_PaneList[Index].FixedSize=Size;
	if (m_pBase!=NULL)
		Adjust();
	return true;
}


int CSplitter::GetPaneSize(int ID)
{
	int Index=IDToIndex(ID);

	if (Index<0)
		return false;
	if (Index==0)
		return m_BarPos;
	int Size;
	if ((m_Style&STYLE_VERT)==0)
		Size=m_Position.right-m_Position.left;
	else
		Size=m_Position.bottom-m_Position.top;
	Size-=m_BarPos+m_BarWidth;
	return max(Size,0);
}


int CSplitter::IDToIndex(int ID) const
{
	for (int i=0;i<2;i++) {
		if (m_PaneList[i].pContainer!=NULL && m_PaneList[i].pContainer->GetID()==ID)
			return i;
	}
	return -1;
}


bool CSplitter::SetStyle(unsigned int Style,bool fAdjust)
{
	if (m_Style!=Style) {
		if ((m_Style&STYLE_VERT)!=(Style&STYLE_VERT)) {
			if ((Style&STYLE_FIXED)!=0
					&& m_PaneList[1].pContainer!=NULL
					&& m_PaneList[1].FixedSize>=0) {
				if ((Style&STYLE_VERT)!=0) {
					m_BarPos=(m_Position.bottom-m_Position.top)-m_PaneList[1].FixedSize;
				} else {
					m_BarPos=(m_Position.right-m_Position.left)-m_PaneList[1].FixedSize;
				}
			}
		}
		m_Style=Style;
		if (fAdjust && m_pBase!=NULL)
			Adjust();
	}
	return true;
}


bool CSplitter::SetAdjustPane(int ID)
{
	m_AdjustPane=ID;
	return true;
}


bool CSplitter::SetBarPos(int Pos)
{
	m_BarPos=Pos;
	return true;
}


void CSplitter::Adjust()
{
	if (m_PaneList[0].pContainer==NULL || !m_PaneList[0].pContainer->GetVisible()
			|| m_PaneList[1].pContainer==NULL || !m_PaneList[1].pContainer->GetVisible()) {
		if (m_PaneList[0].pContainer!=NULL && m_PaneList[0].pContainer->GetVisible())
			m_PaneList[0].pContainer->SetPosition(m_Position);
		else if (m_PaneList[1].pContainer!=NULL && m_PaneList[1].pContainer->GetVisible())
			m_PaneList[1].pContainer->SetPosition(m_Position);
	} else {
		RECT rcBar,rc;
		SIZE MinSize;

		GetBarRect(&rcBar);
		m_PaneList[1].pContainer->GetMinSize(&MinSize);
		if ((m_Style&STYLE_VERT)==0) {
			rc=m_Position;
			rc.right=rcBar.left;
			m_PaneList[0].pContainer->SetPosition(rc);
			rc.left=rcBar.right;
			rc.right=m_Position.right;
			if (rc.right-rc.left<MinSize.cx)
				rc.right=rc.left+MinSize.cx;
			m_PaneList[1].pContainer->SetPosition(rc);
		} else {
			rc=m_Position;
			rc.bottom=rcBar.top;
			m_PaneList[0].pContainer->SetPosition(rc);
			rc.top=rcBar.bottom;
			rc.bottom=m_Position.bottom;
			if (rc.bottom-rc.top<MinSize.cy)
				rc.bottom=rc.top+MinSize.cy;
			m_PaneList[1].pContainer->SetPosition(rc);
		}
	}
}


bool CSplitter::GetBarRect(RECT *pRect) const
{
	if (m_PaneList[0].pContainer==NULL || !m_PaneList[0].pContainer->GetVisible()
			|| m_PaneList[1].pContainer==NULL || !m_PaneList[1].pContainer->GetVisible())
		return false;

	int BarPos;
	SIZE MinSize1,MinSize2;
	RECT rc;

	BarPos=m_BarPos;
	m_PaneList[0].pContainer->GetMinSize(&MinSize1);
	m_PaneList[1].pContainer->GetMinSize(&MinSize2);
	rc=m_Position;
	if ((m_Style&STYLE_VERT)==0) {
		int Width;

		if ((m_Style&STYLE_FIXED)==0 || m_PaneList[1].FixedSize<0) {
			Width=(m_Position.right-m_Position.left)-BarPos;
			if ((m_Style&STYLE_FIXED)==0)
				Width-=m_BarWidth;
			if (Width<MinSize2.cx)
				Width=MinSize2.cx;
		} else {
			Width=m_PaneList[1].FixedSize;
		}
		BarPos=(m_Position.right-m_Position.left)-Width;
		if ((m_Style&STYLE_FIXED)==0)
			BarPos-=m_BarWidth;
		if (BarPos<MinSize1.cx)
			BarPos=MinSize1.cx;
		rc.left=m_Position.left+BarPos;
		rc.right=rc.left;
		if ((m_Style&STYLE_FIXED)==0)
			rc.right+=m_BarWidth;
	} else {
		int Height;

		if ((m_Style&STYLE_FIXED)==0 || m_PaneList[1].FixedSize<0) {
			Height=(m_Position.bottom-m_Position.top)-BarPos;
			if ((m_Style&STYLE_FIXED)==0)
				Height-=m_BarWidth;
			if (Height<MinSize2.cy)
				Height=MinSize2.cy;
		} else {
			Height=m_PaneList[1].FixedSize;
		}
		BarPos=(m_Position.bottom-m_Position.top)-Height;
		if ((m_Style&STYLE_FIXED)==0)
			BarPos-=m_BarWidth;
		if (BarPos<MinSize1.cy)
			BarPos=MinSize1.cy;
		rc.top=m_Position.top+BarPos;
		rc.bottom=rc.top;
		if ((m_Style&STYLE_FIXED)==0)
			rc.bottom+=m_BarWidth;
	}
	*pRect=rc;
	return true;
}




// �ߋ��̃o�[�W�����Ƃ̌݊��̂��߂� Splitter �ɂ��Ă���
//const LPCTSTR CLayoutBase::m_pszWindowClass=APP_NAME TEXT(" Layout Base");
const LPCTSTR CLayoutBase::m_pszWindowClass=APP_NAME TEXT(" Splitter");
HINSTANCE CLayoutBase::m_hinst=NULL;


bool CLayoutBase::Initialize(HINSTANCE hinst)
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
		wc.lpszClassName=m_pszWindowClass;
		if (RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CLayoutBase::CLayoutBase()
	: m_pEventHandler(NULL)
	, m_pContainer(NULL)
	, m_pFocusContainer(NULL)
	, m_BackColor(::GetSysColor(COLOR_3DFACE))
	, m_fLockLayout(false)
{
}


CLayoutBase::~CLayoutBase()
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pBase=NULL;
	delete m_pContainer;
}


bool CLayoutBase::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 m_pszWindowClass,NULL,m_hinst);
}


LRESULT CLayoutBase::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			m_pFocusContainer=NULL;
		}
		return 0;

	case WM_SIZE:
		{
			RECT rc;

			rc.left=0;
			rc.top=0;
			rc.right=LOWORD(lParam);
			rc.bottom=HIWORD(lParam);
			if (m_pContainer!=NULL)
				m_pContainer->SetPosition(rc);
		}
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd,&ps);
			::FillRect(ps.hdc,&ps.rcPaint,GetBackBrush());
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_LBUTTONDOWN:
		if (m_pContainer!=NULL) {
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			CContainer *pContainer=GetContainerFromPoint(x,y);

			if (pContainer!=NULL)
				pContainer->OnLButtonDown(x,y);
			m_pFocusContainer=pContainer;
		}
		return 0;

	case WM_LBUTTONUP:
		if (m_pFocusContainer!=NULL) {
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);

			m_pFocusContainer->OnLButtonUp(x,y);
			m_pFocusContainer=NULL;
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);

			if (m_pFocusContainer!=NULL) {
				m_pFocusContainer->OnMouseMove(x,y);
			} else {
				CContainer *pContainer=GetContainerFromPoint(x,y);

				if (pContainer!=NULL)
					pContainer->OnMouseMove(x,y);
			}
		}
		return 0;

	case WM_CAPTURECHANGED:
		m_pFocusContainer=NULL;
		return 0;

	case WM_COMMAND:
	case WM_NOTIFY:
		return ::SendMessage(::GetParent(hwnd),uMsg,wParam,lParam);
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


bool CLayoutBase::SetTopContainer(CContainer *pContainer)
{
	if (m_pContainer!=NULL)
		delete m_pContainer;
	m_pContainer=pContainer;
	m_pFocusContainer=NULL;
	if (pContainer!=NULL) {
		SetBasePointer(pContainer,this);
		Adjust();
	}
	return true;
}


void CLayoutBase::SetBasePointer(CContainer *pContainer,CLayoutBase *pBase)
{
	int NumChildren=pContainer->NumChildContainers();

	pContainer->m_pBase=pBase;
	for (int i=0;i<NumChildren;i++) {
		CContainer *pChild=pContainer->GetChildContainer(i);

		if (pChild!=NULL)
			SetBasePointer(pChild,pBase);
	}
}


CContainer *CLayoutBase::GetContainerByID(int ID) const
{
	if (m_pContainer==NULL)
		return NULL;
	if (m_pContainer->GetID()==ID)
		return m_pContainer;
	return GetChildContainerByID(m_pContainer,ID);
}


CContainer *CLayoutBase::GetChildContainerByID(const CContainer *pContainer,int ID) const
{
	int NumChildren=pContainer->NumChildContainers();

	for (int i=0;i<NumChildren;i++) {
		CContainer *pChild=pContainer->GetChildContainer(i);

		if (pChild!=NULL) {
			if (pChild->GetID()==ID)
				return pChild;
			pChild=GetChildContainerByID(pChild,ID);
			if (pChild!=NULL)
				return pChild;
		}
	}
	return NULL;
}


CContainer *CLayoutBase::GetContainerFromPoint(int x,int y) const
{
	if (m_pContainer==NULL)
		return NULL;
	return GetChildContainerFromPoint(m_pContainer,x,y);
}


CContainer *CLayoutBase::GetChildContainerFromPoint(const CContainer *pContainer,int x,int y) const
{
	if (pContainer==NULL || !pContainer->GetVisible())
		return NULL;

	POINT pt={x,y};
	RECT rc;

	rc=pContainer->GetPosition();
	if (::PtInRect(&rc,pt)) {
		int NumChildren=pContainer->NumChildContainers();

		for (int i=0;i<NumChildren;i++) {
			CContainer *pChild=pContainer->GetChildContainer(i);

			if (pChild!=NULL) {
				rc=pChild->GetPosition();
				if (::PtInRect(&rc,pt)) {
					if (pChild->GetVisible())
						return GetChildContainerFromPoint(pChild,x,y);
					break;;
				}
			}
		}
		return const_cast<CContainer*>(pContainer);
	}
	return NULL;
}


bool CLayoutBase::SetContainerVisible(int ID,bool fVisible)
{
	CContainer *pContainer=GetContainerByID(ID);

	if (pContainer==NULL)
		return false;
	if (pContainer->GetVisible()!=fVisible) {
		pContainer->SetVisible(fVisible);
		Adjust();
	}
	return true;
}


bool CLayoutBase::GetContainerVisible(int ID) const
{
	const CContainer *pContainer=GetContainerByID(ID);

	if (pContainer==NULL)
		return false;
	return pContainer->GetVisible();
}


void CLayoutBase::GetMinSize(SIZE *pSize) const
{
	if (m_pContainer==NULL) {
		pSize->cx=0;
		pSize->cy=0;
	} else {
		m_pContainer->GetMinSize(pSize);
	}
}


void CLayoutBase::Adjust()
{
	if (m_pContainer!=NULL && m_hwnd!=NULL && !m_fLockLayout) {
		RECT rc;

		GetClientRect(&rc);
		m_pContainer->SetPosition(rc);
	}
}


void CLayoutBase::LockLayout()
{
	m_fLockLayout=true;
}


void CLayoutBase::UnlockLayout(bool fAdjust)
{
	if (m_fLockLayout) {
		m_fLockLayout=false;

		if (fAdjust)
			Adjust();
	}
}


void CLayoutBase::SetEventHandler(CEventHandler *pHandler)
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pBase=NULL;
	if (pHandler!=NULL)
		pHandler->m_pBase=this;
	m_pEventHandler=pHandler;
}


bool CLayoutBase::SetBackColor(COLORREF Color)
{
	if (m_BackColor!=Color) {
		m_BackColor=Color;
		m_BackBrush.Destroy();
		if (m_hwnd!=NULL)
			Invalidate();
	}
	return true;
}


HBRUSH CLayoutBase::GetBackBrush()
{
	if (!m_BackBrush.IsCreated())
		m_BackBrush.Create(m_BackColor);
	return m_BackBrush.GetHandle();
}




CLayoutBase::CEventHandler::CEventHandler()
	: m_pBase(NULL)
{
}


CLayoutBase::CEventHandler::~CEventHandler()
{
	if (m_pBase!=NULL)
		m_pBase->SetEventHandler(NULL);
}


}	// namespace Layout
