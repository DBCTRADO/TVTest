#include "stdafx.h"
#include "TVTest.h"
#include "ColorPalette.h"


#define PALETTE_WINDOW_CLASS APP_NAME TEXT(" Color Palette")




HINSTANCE CColorPalette::m_hinst=NULL;


bool CColorPalette::Initialize(HINSTANCE hinst)
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
		wc.hbrBackground=(HBRUSH)(COLOR_3DFACE+1);
		wc.lpszMenuName=NULL;
		wc.lpszClassName=PALETTE_WINDOW_CLASS;
		if (RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CColorPalette::CColorPalette()
	: m_NumColors(0)
	, m_pPalette(NULL)
{
}


CColorPalette::~CColorPalette()
{
	delete [] m_pPalette;
}


bool CColorPalette::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 PALETTE_WINDOW_CLASS,NULL,m_hinst);
}


bool CColorPalette::GetPalette(RGBQUAD *pPalette)
{
	if (m_pPalette==NULL)
		return false;
	CopyMemory(pPalette,m_pPalette,m_NumColors*sizeof(RGBQUAD));
	return true;
}


bool CColorPalette::SetPalette(const RGBQUAD *pPalette,int NumColors)
{
	if (NumColors<1 || NumColors>256)
		return false;
	if (NumColors!=m_NumColors) {
		delete [] m_pPalette;
		m_pPalette=new RGBQUAD[NumColors];
		m_NumColors=NumColors;
	}
	CopyMemory(m_pPalette,pPalette,NumColors*sizeof(RGBQUAD));
	m_SelColor=-1;
	m_HotColor=-1;
	InvalidateRect(m_hwnd,NULL,TRUE);
	SetToolTip();
	return true;
}


COLORREF CColorPalette::GetColor(int Index) const
{
	if (Index<0 || Index>=m_NumColors)
		return CLR_INVALID;
	return RGB(m_pPalette[Index].rgbRed,m_pPalette[Index].rgbGreen,
			   m_pPalette[Index].rgbBlue);
}


bool CColorPalette::SetColor(int Index,COLORREF Color)
{
	RECT rc;

	if (Index<0 || Index>=m_NumColors)
		return false;
	m_pPalette[Index].rgbBlue=GetBValue(Color);
	m_pPalette[Index].rgbGreen=GetGValue(Color);
	m_pPalette[Index].rgbRed=GetRValue(Color);
	GetItemRect(Index,&rc);
	InvalidateRect(m_hwnd,&rc,TRUE);
	return true;
}


int CColorPalette::GetSel() const
{
	return m_SelColor;
}


bool CColorPalette::SetSel(int Sel)
{
	if (m_pPalette==NULL)
		return false;
	if (Sel<0 || Sel>=m_NumColors)
		Sel=-1;
	if (Sel!=m_SelColor) {
		DrawNewSelHighlight(m_SelColor,Sel);
		m_SelColor=Sel;
	}
	return true;
}


int CColorPalette::GetHot() const
{
	return m_HotColor;
}


int CColorPalette::FindColor(COLORREF Color) const
{
	int i;

	for (i=0;i<m_NumColors;i++) {
		if (RGB(m_pPalette[i].rgbRed,m_pPalette[i].rgbGreen,m_pPalette[i].rgbBlue)==Color)
			return i;
	}
	return -1;
}


void CColorPalette::GetItemRect(int Index,RECT *pRect) const
{
	int x,y;

	x=m_Left+Index%16*m_ItemWidth;
	y=m_Top+Index/16*m_ItemHeight;
	pRect->left=x;
	pRect->top=y;
	pRect->right=x+m_ItemWidth;
	pRect->bottom=y+m_ItemHeight;
}


void CColorPalette::DrawSelRect(HDC hdc,int Sel,bool fSel)
{
	HPEN hpen,hpenOld;
	HBRUSH hbrOld;
	RECT rc;

	hpen=CreatePen(PS_SOLID,1,GetSysColor(fSel?COLOR_HIGHLIGHT:COLOR_3DFACE));
	hpenOld=SelectPen(hdc,hpen);
	hbrOld=SelectBrush(hdc,GetStockObject(NULL_BRUSH));
	GetItemRect(Sel,&rc);
	Rectangle(hdc,rc.left,rc.top,rc.right,rc.bottom);
	SelectPen(hdc,hpenOld);
	SelectBrush(hdc,hbrOld);
	DeleteObject(hpen);
}


void CColorPalette::DrawNewSelHighlight(int OldSel,int NewSel)
{
	HDC hdc;

	hdc=GetDC(m_hwnd);
	if (OldSel>=0)
		DrawSelRect(hdc,OldSel,false);
	if (NewSel>=0)
		DrawSelRect(hdc,NewSel,true);
	ReleaseDC(m_hwnd,hdc);
}


void CColorPalette::SetToolTip()
{
	int NumTools,i;
	RECT rc;

	NumTools=m_Tooltip.NumTools();
	if (NumTools>m_NumColors) {
		do {
			m_Tooltip.DeleteTool(--NumTools);
		} while (NumTools>m_NumColors);
	}
	for (i=0;i<m_NumColors;i++) {
		GetItemRect(i,&rc);
		if (i<NumTools)
			m_Tooltip.SetToolRect(i,rc);
		else
			m_Tooltip.AddTool(i,rc);
	}
}


void CColorPalette::SendNotify(int Code)
{
	::SendMessage(GetParent(),WM_COMMAND,
				MAKEWPARAM(GetWindowLong(m_hwnd,GWL_ID),Code),(LPARAM)m_hwnd);
}


LRESULT CColorPalette::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		m_SelColor=-1;
		m_HotColor=-1;
		m_Tooltip.Create(hwnd);
		return 0;

	case WM_SIZE:
		{
			int sx=LOWORD(lParam),sy=HIWORD(lParam);

			m_ItemWidth=max(sx/16,6);
			m_ItemHeight=max(sy/16,6);
			m_Left=(sx-m_ItemWidth*16)/2;
			m_Top=(sy-m_ItemHeight*16)/2;
			if (m_pPalette!=NULL)
				SetToolTip();
		}
		return 0;

	case WM_PAINT:
		if (m_pPalette!=NULL) {
			PAINTSTRUCT ps;
			int x,y;
			int i;
			HBRUSH hbr;
			RECT rc;

			::BeginPaint(hwnd,&ps);
			for (i=0;i<m_NumColors;i++) {
				x=i%16;
				y=i/16;
				rc.left=m_Left+x*m_ItemWidth+2;
				rc.top=m_Top+y*m_ItemHeight+2;
				rc.right=rc.left+m_ItemWidth-4;
				rc.bottom=rc.top+m_ItemHeight-4;
				if (rc.left<ps.rcPaint.right && rc.top<ps.rcPaint.bottom
						&& rc.right>ps.rcPaint.left && rc.bottom>ps.rcPaint.top) {
					hbr=::CreateSolidBrush(RGB(m_pPalette[i].rgbRed,
											   m_pPalette[i].rgbGreen,
											   m_pPalette[i].rgbBlue));
					::FillRect(ps.hdc,&rc,hbr);
					::DeleteObject(hbr);
				}
			}
			if (m_SelColor>=0)
				DrawSelRect(ps.hdc,m_SelColor,true);
			::EndPaint(hwnd,&ps);
			return 0;
		}
		break;

	case WM_MOUSEMOVE:
		if (m_pPalette!=NULL) {
			POINT ptCursor;
			int Hot;

			ptCursor.x=GET_X_LPARAM(lParam);
			ptCursor.y=GET_Y_LPARAM(lParam);
			Hot=(ptCursor.y-m_Top)/m_ItemHeight*16+
				(ptCursor.x-m_Left)/m_ItemWidth;
			if (ptCursor.x<m_Left
					|| ptCursor.x>=m_Left+m_ItemWidth*16
					|| ptCursor.y<m_Top
					|| ptCursor.y>=m_Top+m_ItemHeight*16
					|| Hot>=m_NumColors)
				Hot=-1;
			if (Hot==m_HotColor)
				return 0;
			m_HotColor=Hot;
			SendNotify(NOTIFY_HOTCHANGE);
		}
		return 0;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		if (m_pPalette!=NULL) {
			POINT ptCursor;
			int Sel;

			ptCursor.x=GET_X_LPARAM(lParam);
			ptCursor.y=GET_Y_LPARAM(lParam);
			Sel=(ptCursor.y-m_Top)/m_ItemHeight*16+
				(ptCursor.x-m_Left)/m_ItemWidth;
			if (ptCursor.x<m_Left
					|| ptCursor.x>=m_Left+m_ItemWidth*16
					|| ptCursor.y<m_Top
					|| ptCursor.y>=m_Top+m_ItemHeight*16
					|| Sel>=m_NumColors || Sel==m_SelColor)
				return 0;
			DrawNewSelHighlight(m_SelColor,Sel);
			m_SelColor=Sel;
			SendNotify(NOTIFY_SELCHANGE);
			if (uMsg==WM_RBUTTONDOWN)
				SendNotify(NOTIFY_RBUTTONDOWN);
		}
		return 0;

	case WM_LBUTTONDBLCLK:
		if (m_SelColor>=0)
			SendNotify(NOTIFY_DOUBLECLICK);
		return 0;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case TTN_NEEDTEXT:
			{
				LPNMTTDISPINFO pttdi=reinterpret_cast<LPNMTTDISPINFO>(lParam);
				int Index=(int)pttdi->hdr.idFrom;

				pttdi->lpszText=pttdi->szText;
				pttdi->hinst=NULL;
				if (Index>=0 && Index<m_NumColors) {
					int r,g,b;

					r=m_pPalette[Index].rgbRed;
					g=m_pPalette[Index].rgbGreen;
					b=m_pPalette[Index].rgbBlue;
					::wsprintf(pttdi->szText,TEXT("%d,%d,%d #%02X%02X%02X"),r,g,b,r,g,b);
				} else {
					pttdi->szText[0]='\0';
				}
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
