#include "stdafx.h"
#include "TVTest.h"
#include "SideBar.h"
#include "resource.h"
#include "Common/DebugDef.h"




const int CSideBar::ICON_WIDTH=16;
const int CSideBar::ICON_HEIGHT=16;
const LPCTSTR CSideBar::CLASS_NAME=APP_NAME TEXT(" Side Bar");
HINSTANCE CSideBar::m_hinst=NULL;


bool CSideBar::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW | CS_VREDRAW;
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


CSideBar::CSideBar(const CCommandList *pCommandList)
	: m_fShowTooltips(true)
	, m_fVertical(true)
	, m_HotItem(-1)
	, m_ClickItem(-1)
	, m_pEventHandler(NULL)
	, m_pCommandList(pCommandList)
{
}


CSideBar::~CSideBar()
{
	Destroy();
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pSideBar=NULL;
}


bool CSideBar::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 CLASS_NAME,NULL,m_hinst);
}


void CSideBar::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);
}


void CSideBar::NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	m_Style.NormalizeStyle(pStyleManager);
}


void CSideBar::SetTheme(const TVTest::Theme::CThemeManager *pThemeManager)
{
	SideBarTheme Theme;

	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_SIDEBAR_ITEM,
							&Theme.ItemStyle);
	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_SIDEBAR_ITEM_HOT,
							&Theme.HighlightItemStyle);
	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_SIDEBAR_ITEM_CHECKED,
							&Theme.CheckItemStyle);
	pThemeManager->GetBorderStyle(TVTest::Theme::CThemeManager::STYLE_SIDEBAR,
								  &Theme.Border);

	SetSideBarTheme(Theme);
}


int CSideBar::GetBarWidth() const
{
	RECT rc;
	TVTest::Theme::GetBorderWidths(m_Theme.Border,&rc);
	int Width;
	if (m_fVertical) {
		Width=m_Style.IconSize.Width+m_Style.ItemPadding.Horz()+rc.left+rc.right;
	} else {
		Width=m_Style.IconSize.Height+m_Style.ItemPadding.Vert()+rc.top+rc.bottom;
	}
	return Width;
}


bool CSideBar::SetIconImage(HBITMAP hbm,int Width,int Height)
{
	if (hbm==NULL)
		return false;
	if (!m_Icons.Create(hbm,Width,Height))
		return false;
	if (m_hwnd!=NULL)
		Invalidate();
	return true;
}


void CSideBar::DeleteAllItems()
{
	m_ItemList.clear();
	m_Tooltip.DeleteAllTools();
}


bool CSideBar::AddItem(const SideBarItem *pItem)
{
	return AddItems(pItem,1);
}


bool CSideBar::AddItems(const SideBarItem *pItemList,int NumItems)
{
	if (pItemList==NULL || NumItems<=0)
		return false;

	size_t OldSize=m_ItemList.size();
	m_ItemList.resize(OldSize+NumItems);
	::CopyMemory(&m_ItemList[OldSize],pItemList,NumItems*sizeof(SideBarItem));

	if (m_Tooltip.IsCreated()) {
		for (int i=0;i<NumItems;i++) {
			if (pItemList[i].Command!=ITEM_SEPARATOR) {
				RECT rc;

				GetItemRect((int)OldSize+i,&rc);
				m_Tooltip.AddTool((UINT)OldSize+i,rc);
			}
		}
	}
	return true;
}


bool CSideBar::AddSeparator()
{
	SideBarItem Item;

	Item.Command=ITEM_SEPARATOR;
	Item.Icon=-1;
	Item.State=0;

	return AddItem(&Item);
}


int CSideBar::GetItemCount() const
{
	return (int)m_ItemList.size();
}


int CSideBar::GetItemCommand(int Index) const
{
	if (Index<0 || (size_t)Index>=m_ItemList.size())
		return -1;
	return m_ItemList[Index].Command;
}


int CSideBar::CommandToIndex(int Command) const
{
	for (size_t i=0;i<m_ItemList.size();i++) {
		if (m_ItemList[i].Command==Command)
			return (int)i;
	}
	return -1;
}


bool CSideBar::EnableItem(int Command,bool fEnable)
{
	int Index=CommandToIndex(Command);

	if (Index<0)
		return false;
	if (m_ItemList[Index].IsEnabled()!=fEnable) {
		m_ItemList[Index].State^=ITEM_STATE_DISABLED;
		if (!fEnable && m_HotItem==Index)
			m_HotItem=-1;
		UpdateItem(Index);
	}
	return true;
}


bool CSideBar::EnableItemByIndex(int Index,bool fEnable)
{
	if (Index<0 || (size_t)Index>=m_ItemList.size())
		return false;
	if (m_ItemList[Index].IsEnabled()!=fEnable) {
		m_ItemList[Index].State^=ITEM_STATE_DISABLED;
		if (!fEnable && m_HotItem==Index)
			m_HotItem=-1;
		UpdateItem(Index);
	}
	return true;
}


bool CSideBar::IsItemEnabled(int Command) const
{
	int Index=CommandToIndex(Command);

	if (Index<0)
		return false;
	return m_ItemList[Index].IsEnabled();
}


bool CSideBar::CheckItem(int Command,bool fCheck)
{
	int Index=CommandToIndex(Command);

	if (Index<0)
		return false;
	if (m_ItemList[Index].IsChecked()!=fCheck) {
		m_ItemList[Index].State^=ITEM_STATE_CHECKED;
		UpdateItem(Index);
	}
	return true;
}


bool CSideBar::CheckItemByIndex(int Index,bool fCheck)
{
	if (Index<0 || (size_t)Index>=m_ItemList.size())
		return false;
	if (m_ItemList[Index].IsChecked()!=fCheck) {
		m_ItemList[Index].State^=ITEM_STATE_CHECKED;
		UpdateItem(Index);
	}
	return true;
}


bool CSideBar::CheckRadioItem(int First,int Last,int Check)
{
	if (First>Last)
		return false;
	for (int i=First;i<=Last;i++)
		CheckItem(i,i==Check);
	return true;
}


bool CSideBar::IsItemChecked(int Command) const
{
	int Index=CommandToIndex(Command);

	if (Index<0)
		return false;
	return m_ItemList[Index].IsChecked();
}


bool CSideBar::RedrawItem(int Command)
{
	int Index=CommandToIndex(Command);

	if (Index<0)
		return false;

	UpdateItem(Index);

	return true;
}


bool CSideBar::SetSideBarTheme(const SideBarTheme &Theme)
{
	int OldBarWidth;
	if (m_hwnd!=NULL && m_pEventHandler!=NULL)
		OldBarWidth=GetBarWidth();

	m_Theme=Theme;

	if (m_hwnd!=NULL) {
		if (m_pEventHandler!=NULL) {
			int NewBarWidth=GetBarWidth();
			if (NewBarWidth!=OldBarWidth)
				m_pEventHandler->OnBarWidthChanged(NewBarWidth);
		}
		Invalidate();
	}

	return true;
}


bool CSideBar::GetSideBarTheme(SideBarTheme *pTheme) const
{
	if (pTheme==NULL)
		return false;
	*pTheme=m_Theme;
	return true;
}


void CSideBar::ShowToolTips(bool fShow)
{
	if (m_fShowTooltips!=fShow) {
		m_fShowTooltips=fShow;
		m_Tooltip.Enable(fShow);
	}
}


void CSideBar::SetVertical(bool fVertical)
{
	if (m_fVertical!=fVertical) {
		m_fVertical=fVertical;
		if (m_hwnd!=NULL) {
			Invalidate();
			UpdateTooltipsRect();
		}
	}
}


void CSideBar::SetEventHandler(CEventHandler *pHandler)
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pSideBar=NULL;
	if (pHandler!=NULL)
		pHandler->m_pSideBar=this;
	m_pEventHandler=pHandler;
}


TVTest::Style::Size CSideBar::GetIconDrawSize() const
{
	return m_Style.IconSize;
}


LRESULT CSideBar::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			InitializeUI();

			LPCREATESTRUCT pcs=reinterpret_cast<LPCREATESTRUCT>(lParam);

			m_Tooltip.Create(hwnd);
			m_Tooltip.Enable(m_fShowTooltips);
			for (int i=0;i<(int)m_ItemList.size();i++) {
				if (m_ItemList[i].Command!=ITEM_SEPARATOR) {
					RECT rc;
					GetItemRect(i,&rc);
					m_Tooltip.AddTool(i,rc);
				}
			}

			m_HotItem=-1;
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

			if (HotItem>=0
					&& (m_ItemList[HotItem].Command==ITEM_SEPARATOR
						|| m_ItemList[HotItem].IsDisabled()))
				HotItem=-1;
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
		if (m_HotItem>=0) {
			m_ClickItem=m_HotItem;
			::SetCapture(hwnd);
		}
		return 0;

	case WM_LBUTTONUP:
		if (::GetCapture()==hwnd) {
			::ReleaseCapture();
			if (m_HotItem>=0) {
				if (m_pEventHandler!=NULL)
					m_pEventHandler->OnCommand(m_ItemList[m_HotItem].Command);
			}
		}
		return 0;

	case WM_RBUTTONUP:
		if (m_pEventHandler!=NULL)
			m_pEventHandler->OnRButtonUp(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT) {
			if (m_HotItem>=0) {
				::SetCursor(GetActionCursor());
				return TRUE;
			}
		}
		break;

	case WM_NOTIFY:
		switch (reinterpret_cast<NMHDR*>(lParam)->code) {
		case TTN_NEEDTEXT:
			{
				LPNMTTDISPINFO pnmttdi=reinterpret_cast<LPNMTTDISPINFO>(lParam);

				if (m_pEventHandler==NULL
						|| !m_pEventHandler->GetTooltipText(
								m_ItemList[pnmttdi->hdr.idFrom].Command,
								pnmttdi->szText,lengthof(pnmttdi->szText))) {
					m_pCommandList->GetCommandShortNameByID(
						m_ItemList[pnmttdi->hdr.idFrom].Command,
						pnmttdi->szText,lengthof(pnmttdi->szText));
				}
				pnmttdi->lpszText=pnmttdi->szText;
				pnmttdi->hinst=NULL;
			}
			return 0;

		case TTN_SHOW:
			if (m_fVertical) {
				LPNMHDR pnmh=reinterpret_cast<LPNMHDR>(lParam);
				RECT rcBar,rcTip;
				int x,y;

				::GetWindowRect(hwnd,&rcBar);
				TVTest::Theme::SubtractBorderRect(m_Theme.Border,&rcBar);
				::GetWindowRect(pnmh->hwndFrom,&rcTip);
				x=rcBar.right;
				y=rcTip.top;
				HMONITOR hMonitor=::MonitorFromRect(&rcTip,MONITOR_DEFAULTTONULL);
				if (hMonitor!=NULL) {
					MONITORINFO mi;

					mi.cbSize=sizeof(mi);
					if (::GetMonitorInfo(hMonitor,&mi)) {
						if (x>=mi.rcMonitor.right-16)
							x=rcBar.left-(rcTip.right-rcTip.left);
					}
				}
				::SetWindowPos(pnmh->hwndFrom,HWND_TOPMOST,
							   x,y,rcTip.right-rcTip.left,rcTip.bottom-rcTip.top,
							   SWP_NOACTIVATE);
				return TRUE;
			}
			break;
		}
		break;

	case WM_DESTROY:
		m_Tooltip.Destroy();
		return 0;
	}

	return CCustomWindow::OnMessage(hwnd,uMsg,wParam,lParam);
}


void CSideBar::GetItemRect(int Item,RECT *pRect) const
{
	const int ItemWidth=m_Style.IconSize.Width+m_Style.ItemPadding.Horz();
	const int ItemHeight=m_Style.IconSize.Height+m_Style.ItemPadding.Vert();
	int Offset=0;

	for (int i=0;i<Item;i++) {
		if (m_ItemList[i].Command==ITEM_SEPARATOR) {
			Offset+=m_Style.SeparatorWidth;
		} else {
			if (m_fVertical)
				Offset+=ItemHeight;
			else
				Offset+=ItemWidth;
		}
	}
	RECT rcBorder;
	TVTest::Theme::GetBorderWidths(m_Theme.Border,&rcBorder);
	if (m_fVertical) {
		pRect->left=rcBorder.left;
		pRect->right=rcBorder.left+ItemWidth;
		pRect->top=rcBorder.top+Offset;
		pRect->bottom=pRect->top+(m_ItemList[Item].Command==ITEM_SEPARATOR?m_Style.SeparatorWidth:ItemHeight);
	} else {
		pRect->top=rcBorder.top;
		pRect->bottom=rcBorder.top+ItemHeight;
		pRect->left=rcBorder.left+Offset;
		pRect->right=pRect->left+(m_ItemList[Item].Command==ITEM_SEPARATOR?m_Style.SeparatorWidth:ItemWidth);
	}
}


void CSideBar::UpdateItem(int Item)
{
	if (m_hwnd!=NULL) {
		RECT rc;

		GetItemRect(Item,&rc);
		Invalidate(&rc);
	}
}


int CSideBar::HitTest(int x,int y) const
{
	POINT pt;

	pt.x=x;
	pt.y=y;
	for (int i=0;i<(int)m_ItemList.size();i++) {
		RECT rc;
		GetItemRect(i,&rc);
		if (::PtInRect(&rc,pt))
			return i;
	}
	return -1;
}


void CSideBar::UpdateTooltipsRect()
{
	for (int i=0;i<(int)m_ItemList.size();i++) {
		RECT rc;

		GetItemRect(i,&rc);
		m_Tooltip.SetToolRect(i,rc);
	}
}


void CSideBar::Draw(HDC hdc,const RECT &PaintRect)
{
	RECT rcClient,rc;

	GetClientRect(&rcClient);
	rc=rcClient;
	if (m_fVertical) {
		rc.top=PaintRect.top;
		rc.bottom=PaintRect.bottom;
	} else {
		rc.left=PaintRect.left;
		rc.right=PaintRect.right;
	}

	TVTest::Theme::BackgroundStyle BackStyle;
	BackStyle=m_Theme.ItemStyle.Back;
	if (!m_fVertical && BackStyle.Fill.Type==TVTest::Theme::FILL_GRADIENT)
		BackStyle.Fill.Gradient.Rotate(TVTest::Theme::GradientStyle::ROTATE_RIGHT);
	TVTest::Theme::Draw(hdc,rc,BackStyle);

	HDC hdcMemory=::CreateCompatibleDC(hdc);
	HBITMAP hbmOld=static_cast<HBITMAP>(::GetCurrentObject(hdcMemory,OBJ_BITMAP));

	for (int i=0;i<(int)m_ItemList.size();i++) {
		GetItemRect(i,&rc);
		if (m_ItemList[i].Command!=ITEM_SEPARATOR
				&& rc.left<PaintRect.right && rc.right>PaintRect.left
				&& rc.top<PaintRect.bottom && rc.bottom>PaintRect.top) {
			const bool fHot=m_HotItem==i;
			COLORREF ForeColor;
			BYTE Opacity=255;
			RECT rcItem;

			if (fHot) {
				TVTest::Theme::Style Style=m_Theme.HighlightItemStyle;
				if (!m_fVertical && Style.Back.Fill.Type==TVTest::Theme::FILL_GRADIENT)
					Style.Back.Fill.Gradient.Rotate(TVTest::Theme::GradientStyle::ROTATE_RIGHT);
				if (m_ItemList[i].IsChecked())
					Style.Back.Border=m_Theme.CheckItemStyle.Back.Border;
				TVTest::Theme::Draw(hdc,rc,Style.Back);
				ForeColor=m_Theme.HighlightItemStyle.Fore.Fill.GetSolidColor();
			} else {
				if (m_ItemList[i].IsChecked()) {
					TVTest::Theme::Style Style=m_Theme.CheckItemStyle;
					if (!m_fVertical && Style.Back.Fill.Type==TVTest::Theme::FILL_GRADIENT)
						Style.Back.Fill.Gradient.Rotate(TVTest::Theme::GradientStyle::ROTATE_RIGHT);
					TVTest::Theme::Draw(hdc,rc,Style.Back);
					ForeColor=m_Theme.CheckItemStyle.Fore.Fill.GetSolidColor();
				} else {
					ForeColor=m_Theme.ItemStyle.Fore.Fill.GetSolidColor();
				}
				if (m_ItemList[i].IsDisabled()) {
#if 0
					ForeColor=MixColor(ForeColor,
									   m_Theme.ItemStyle.Fore.Fill.GetSolidColor());
#else
					Opacity=128;
#endif
				}
			}

			rcItem.left=rc.left+m_Style.ItemPadding.Left;
			rcItem.top=rc.top+m_Style.ItemPadding.Top;
			rcItem.right=rcItem.left+m_Style.IconSize.Width;
			rcItem.bottom=rcItem.top+m_Style.IconSize.Height;

			bool fIconDrew=false;
			if (m_pEventHandler!=NULL) {
				DrawIconInfo Info;
				Info.Command=m_ItemList[i].Command;
				Info.State=m_ItemList[i].State;
				if (fHot)
					Info.State|=ITEM_STATE_HOT;
				Info.hdc=hdc;
				Info.IconRect=rcItem;
				Info.Color=ForeColor;
				Info.Opacity=Opacity;
				Info.hdcBuffer=hdcMemory;
				if (m_pEventHandler->DrawIcon(&Info))
					fIconDrew=true;
			}
			if (!fIconDrew && m_ItemList[i].Icon>=0) {
				m_Icons.Draw(hdc,rcItem.left,rcItem.top,
							 m_Style.IconSize.Width,m_Style.IconSize.Height,
							 m_ItemList[i].Icon,ForeColor,Opacity);
			}
		}
	}

	::SelectObject(hdcMemory,hbmOld);
	::DeleteDC(hdcMemory);

	TVTest::Theme::Draw(hdc,rcClient,m_Theme.Border);
}




CSideBar::CEventHandler::CEventHandler()
	: m_pSideBar(NULL)
{
}


CSideBar::CEventHandler::~CEventHandler()
{
	if (m_pSideBar!=NULL)
		m_pSideBar->SetEventHandler(NULL);
}




CSideBar::SideBarStyle::SideBarStyle()
	: IconSize(ICON_WIDTH,ICON_HEIGHT)
	, ItemPadding(3)
	, SeparatorWidth(8)
{
}


void CSideBar::SideBarStyle::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	pStyleManager->Get(TEXT("side-bar.item.icon"),&IconSize);
	pStyleManager->Get(TEXT("side-bar.item.padding"),&ItemPadding);
	pStyleManager->Get(TEXT("side-bar.separator.width"),&SeparatorWidth);
}


void CSideBar::SideBarStyle::NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	pStyleManager->ToPixels(&IconSize);
	pStyleManager->ToPixels(&ItemPadding);
	pStyleManager->ToPixels(&SeparatorWidth);
}
