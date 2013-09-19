#include "stdafx.h"
#include "TVTest.h"
#include "Menu.h"
#include "AppMain.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CMainMenu::CMainMenu()
	: m_hmenu(NULL)
	, m_hmenuPopup(NULL)
	, m_hmenuShow(NULL)
	, m_fPopup(false)
{
}


CMainMenu::~CMainMenu()
{
	Destroy();
}


bool CMainMenu::Create(HINSTANCE hinst)
{
	if (m_hmenu)
		return false;
	m_hmenu=::LoadMenu(hinst,MAKEINTRESOURCE(IDM_MENU));
	if (m_hmenu==NULL)
		return false;
	m_hmenuPopup=::GetSubMenu(m_hmenu,0);
	return true;
}


void CMainMenu::Destroy()
{
	if (m_hmenu) {
		::DestroyMenu(m_hmenu);
		m_hmenu=NULL;
		m_hmenuPopup=NULL;
	}
}


bool CMainMenu::Show(UINT Flags,int x,int y,HWND hwnd,bool fToggle,const std::vector<int> *pItemList)
{
	if (m_hmenu==NULL)
		return false;

	if (!m_fPopup || !fToggle || m_PopupMenu>=0) {
		if (m_fPopup)
			::EndMenu();

		HMENU hmenuCustom=m_hmenuPopup;

		if (pItemList!=NULL && !pItemList->empty()) {
			hmenuCustom=::CreatePopupMenu();
			int OrigItemCount=::GetMenuItemCount(m_hmenuPopup);

			MENUITEMINFO mii;
			TCHAR szText[256];

			mii.cbSize=sizeof(mii);
			mii.fMask=MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_STRING;
			mii.dwTypeData=szText;

			int ItemCount=0;

			for (auto itr=pItemList->begin();itr!=pItemList->end();++itr) {
				int ID=*itr;

				if (ID<0) {
					if (::AppendMenu(hmenuCustom,MF_SEPARATOR,0,NULL))
						ItemCount++;
				} else if (ID>=CM_COMMAND_FIRST) {
					for (int j=0;j<OrigItemCount;j++) {
						mii.cch=lengthof(szText);
						if (::GetMenuItemInfo(m_hmenuPopup,j,TRUE,&mii)
								&& mii.wID==ID) {
							if (::InsertMenuItem(hmenuCustom,ItemCount,TRUE,&mii))
								ItemCount++;
							break;
						}
					}
				} else {
					mii.cch=lengthof(szText);
					if (::GetMenuItemInfo(m_hmenuPopup,ID,TRUE,&mii)
							&& ::InsertMenuItem(hmenuCustom,ItemCount,TRUE,&mii))
						ItemCount++;
				}
			}

			if (ItemCount==0) {
				::DestroyMenu(hmenuCustom);
				hmenuCustom=m_hmenuPopup;
			}
		}

		m_hmenuShow=hmenuCustom;
		m_fPopup=true;
		m_PopupMenu=-1;
		::TrackPopupMenu(hmenuCustom,Flags,x,y,0,hwnd,NULL);
		m_fPopup=false;
		m_hmenuShow=NULL;

		if (hmenuCustom!=m_hmenuPopup) {
			for (int i=::GetMenuItemCount(hmenuCustom)-1;i>=0;i--) {
				if ((::GetMenuState(hmenuCustom,i,MF_BYPOSITION)&MF_POPUP)!=0)
					::RemoveMenu(hmenuCustom,i,MF_BYPOSITION);
			}
			::DestroyMenu(hmenuCustom);
		}
	} else {
		::EndMenu();
	}

	return true;
}


bool CMainMenu::PopupSubMenu(int SubMenu,UINT Flags,int x,int y,HWND hwnd,bool fToggle)
{
	HMENU hmenu=GetSubMenu(SubMenu);

	if (hmenu==NULL)
		return false;
	if (!m_fPopup || !fToggle || m_PopupMenu!=SubMenu) {
		if (m_fPopup)
			::EndMenu();
		m_fPopup=true;
		m_PopupMenu=SubMenu;
		::TrackPopupMenu(hmenu,Flags,x,y,0,hwnd,NULL);
		m_fPopup=false;
	} else {
		::EndMenu();
	}
	return true;
}


void CMainMenu::EnableItem(UINT ID,bool fEnable)
{
	if (m_hmenuPopup!=NULL) {
		::EnableMenuItem(m_hmenuPopup,ID,MF_BYCOMMAND | (fEnable?MFS_ENABLED:MFS_GRAYED));
		if (m_hmenuShow!=NULL && m_hmenuShow!=m_hmenuPopup)
			::EnableMenuItem(m_hmenuShow,ID,MF_BYCOMMAND | (fEnable?MFS_ENABLED:MFS_GRAYED));
	}
}


void CMainMenu::CheckItem(UINT ID,bool fCheck)
{
	if (m_hmenuPopup!=NULL) {
		::CheckMenuItem(m_hmenuPopup,ID,MF_BYCOMMAND | (fCheck?MFS_CHECKED:MFS_UNCHECKED));
		if (m_hmenuShow!=NULL && m_hmenuShow!=m_hmenuPopup)
			::CheckMenuItem(m_hmenuShow,ID,MF_BYCOMMAND | (fCheck?MFS_CHECKED:MFS_UNCHECKED));
	}
}


void CMainMenu::CheckRadioItem(UINT FirstID,UINT LastID,UINT CheckID)
{
	if (m_hmenuPopup!=NULL) {
		::CheckMenuRadioItem(m_hmenuPopup,FirstID,LastID,CheckID,MF_BYCOMMAND);
		if (m_hmenuShow!=NULL && m_hmenuShow!=m_hmenuPopup)
			::CheckMenuRadioItem(m_hmenuShow,FirstID,LastID,CheckID,MF_BYCOMMAND);
	}
}


bool CMainMenu::IsMainMenu(HMENU hmenu) const
{
	return hmenu!=NULL
		&& hmenu==m_hmenuShow;
}


HMENU CMainMenu::GetSubMenu(int SubMenu) const
{
	if (m_hmenuPopup!=NULL)
		return ::GetSubMenu(m_hmenuPopup,SubMenu);
	return NULL;
}


bool CMainMenu::SetAccelerator(CAccelerator *pAccelerator)
{
	if (m_hmenu==NULL)
		return false;
	pAccelerator->SetMenuAccel(m_hmenu);
	return true;
}




CMenuPainter::CMenuPainter()
	: m_fFlatMenu(false)
{
}


CMenuPainter::~CMenuPainter()
{
}


void CMenuPainter::Initialize(HWND hwnd)
{
	m_fFlatMenu=false;
	if (m_UxTheme.Initialize() && m_UxTheme.IsActive()
			&& m_UxTheme.Open(hwnd,VSCLASS_MENU)) {
		// Use theme
	} else {
		BOOL fFlatMenu=FALSE;
		if (::SystemParametersInfo(SPI_GETFLATMENU,0,&fFlatMenu,0) && fFlatMenu)
			m_fFlatMenu=true;
	}
}


void CMenuPainter::Finalize()
{
	m_UxTheme.Close();
}


void CMenuPainter::GetFont(LOGFONT *pFont)
{
	if (!m_UxTheme.IsActive()
			|| !m_UxTheme.GetFont(MENU_POPUPITEM,0,TMT_GLYPHFONT,pFont))
		DrawUtil::GetSystemFont(DrawUtil::FONT_MENU,pFont);
}


COLORREF CMenuPainter::GetTextColor(UINT State)
{
	COLORREF Color;

	if (m_UxTheme.IsOpen()) {
		m_UxTheme.GetColor(MENU_POPUPITEM,ItemStateToID(State),TMT_TEXTCOLOR,&Color);
	} else {
		Color=::GetSysColor(
			(State & (ODS_DISABLED | ODS_INACTIVE))?COLOR_GRAYTEXT:
			(State & ODS_SELECTED)?COLOR_HIGHLIGHTTEXT:COLOR_MENUTEXT);
	}
	return Color;
}


void CMenuPainter::DrawItemBackground(HDC hdc,const RECT &Rect,UINT State)
{
	if (m_UxTheme.IsOpen()) {
		m_UxTheme.DrawBackground(hdc,MENU_POPUPITEM,ItemStateToID(State),
								 MENU_POPUPBACKGROUND,0,&Rect);
	} else {
		bool fSelected=(State & ODS_SELECTED)!=0;
		if (m_fFlatMenu) {
			::FillRect(hdc,&Rect,
				reinterpret_cast<HBRUSH>(fSelected?COLOR_MENUHILIGHT+1:COLOR_MENU+1));
			if (fSelected)
				::FrameRect(hdc,&Rect,::GetSysColorBrush(COLOR_HIGHLIGHT));
		} else {
			::FillRect(hdc,&Rect,
				reinterpret_cast<HBRUSH>(fSelected?COLOR_HIGHLIGHT+1:COLOR_MENU+1));
		}
	}
}


void CMenuPainter::GetItemMargins(MARGINS *pMargins)
{
	if (!m_UxTheme.IsOpen()
			|| !m_UxTheme.GetMargins(MENU_POPUPITEM,0,TMT_CONTENTMARGINS,pMargins)) {
		pMargins->cxLeftWidth=2;
		pMargins->cxRightWidth=2;
		pMargins->cyTopHeight=2;
		pMargins->cyBottomHeight=2;
	}
}


void CMenuPainter::GetMargins(MARGINS *pMargins)
{
	int BorderSize;

	if (!m_UxTheme.IsOpen()
			|| !m_UxTheme.GetInt(MENU_POPUPBACKGROUND,0,TMT_BORDERSIZE,&BorderSize))
		BorderSize=2;
	/*
	SIZE sz;
	GetBorderSize(&sz);
	pMargins->cxLeftWidth=BorderSize+sz.cx;
	pMargins->cxRightWidth=BorderSize+sz.cx;
	pMargins->cyTopHeight=BorderSize+sz.cy;
	pMargins->cyBottomHeight=BorderSize+sz.cy;
	*/
	pMargins->cxLeftWidth=BorderSize;
	pMargins->cxRightWidth=BorderSize;
	pMargins->cyTopHeight=BorderSize;
	pMargins->cyBottomHeight=BorderSize;
}


void CMenuPainter::GetBorderSize(SIZE *pSize)
{
	if (!m_UxTheme.IsOpen()
			|| !m_UxTheme.GetPartSize(NULL,MENU_POPUPBORDERS,0,pSize)) {
		pSize->cx=1;
		pSize->cy=1;
	}
}


void CMenuPainter::DrawItemText(HDC hdc,UINT State,LPCTSTR pszText,const RECT &Rect,DWORD Flags)
{
	if ((Flags & DT_NOPREFIX)==0 && (State & ODS_NOACCEL)!=0)
		Flags|=DT_HIDEPREFIX;

	if (m_UxTheme.IsOpen()) {
		m_UxTheme.DrawText(hdc,MENU_POPUPITEM,ItemStateToID(State),pszText,Flags,&Rect);
	} else {
		RECT rc=Rect;
		::DrawText(hdc,pszText,-1,&rc,Flags);
	}
}


bool CMenuPainter::GetItemTextExtent(HDC hdc,UINT State,LPCTSTR pszText,RECT *pExtent,DWORD Flags)
{
	::SetRectEmpty(pExtent);
	if (m_UxTheme.IsOpen())
		return m_UxTheme.GetTextExtent(hdc,MENU_POPUPITEM,ItemStateToID(State),pszText,Flags,pExtent);
	return ::DrawText(hdc,pszText,-1,pExtent,Flags | DT_CALCRECT)!=FALSE;
}


void CMenuPainter::DrawIcon(HIMAGELIST himl,int Icon,HDC hdc,int x,int y,UINT State)
{
	if ((State & ODS_DISABLED)==0) {
		::ImageList_Draw(himl,Icon,hdc,x,y,ILD_TRANSPARENT);
	} else {
		IMAGELISTDRAWPARAMS ildp;

		::ZeroMemory(&ildp,sizeof(ildp));
		ildp.cbSize=sizeof(ildp);
		ildp.himl=himl;
		ildp.i=Icon;
		ildp.hdcDst=hdc;
		ildp.x=x;
		ildp.y=y;
		ildp.rgbBk=CLR_NONE;
		ildp.fStyle=ILD_TRANSPARENT;
		ildp.fState=ILS_SATURATE;
		::ImageList_DrawIndirect(&ildp);
	}
}


void CMenuPainter::DrawBackground(HDC hdc,const RECT &Rect)
{
	if (m_UxTheme.IsOpen()) {
		m_UxTheme.DrawBackground(hdc,MENU_POPUPBACKGROUND,0,&Rect);
	} else {
		::FillRect(hdc,&Rect,reinterpret_cast<HBRUSH>(COLOR_MENU+1));
	}
}


void CMenuPainter::DrawBorder(HDC hdc,const RECT &Rect)
{
	if (m_UxTheme.IsOpen()) {
		m_UxTheme.DrawBackground(hdc,MENU_POPUPBORDERS,0,
								 MENU_POPUPBACKGROUND,0,&Rect);
	} else {
		Theme::DrawBorder(hdc,Rect,Theme::BORDER_RAISED);
	}
}


void CMenuPainter::DrawSeparator(HDC hdc,const RECT &Rect)
{
	RECT rc;

	if (m_UxTheme.IsOpen()) {
		SIZE sz;

		m_UxTheme.GetPartSize(hdc,MENU_POPUPSEPARATOR,0,&sz);
		rc.left=Rect.left;
		rc.right=Rect.right;
		rc.top=Rect.top+((Rect.bottom-Rect.top)-sz.cy)/2;
		rc.bottom=rc.top+sz.cy;
		m_UxTheme.DrawBackground(hdc,MENU_POPUPSEPARATOR,0,&rc);
	} else {
		rc.left=Rect.left;
		rc.right=Rect.right;
		rc.top=(Rect.top+Rect.bottom)/2-1;
		rc.bottom=rc.top+2;
		::DrawEdge(hdc,&rc,BDR_SUNKENOUTER,BF_RECT);
	}
}


int CMenuPainter::ItemStateToID(UINT State) const
{
	bool fDisabled=(State & (ODS_INACTIVE | ODS_DISABLED))!=0;
	bool fHot=(State & (ODS_HOTLIGHT | ODS_SELECTED))!=0;
	int StateID;
	if (fDisabled) {
		StateID=fHot?MPI_DISABLEDHOT:MPI_DISABLED;
	} else if (fHot) {
		StateID=MPI_HOT;
	} else {
		StateID=MPI_NORMAL;
	}
	return StateID;
}




class CChannelMenuItem
{
	const CChannelInfo *m_pChannelInfo;
	struct Event {
		bool fValid;
		CEventInfoData EventInfo;
		Event() : fValid(false) {}
	};
	Event m_EventList[2];
public:
	CChannelMenuItem(const CChannelInfo *pChannelInfo)
		: m_pChannelInfo(pChannelInfo) {}
	const CEventInfoData *GetEventInfo(CEpgProgramList *pProgramList,
									   int Index,const SYSTEMTIME *pCurTime=NULL);
	const CEventInfoData *GetEventInfo(int Index) const;
	const CChannelInfo *GetChannelInfo() const { return m_pChannelInfo; }
};

const CEventInfoData *CChannelMenuItem::GetEventInfo(CEpgProgramList *pProgramList,
													 int Index,const SYSTEMTIME *pCurTime)
{
	if (Index<0 || Index>=lengthof(m_EventList)
			|| (Index>0 && !m_EventList[Index-1].fValid)
			|| m_pChannelInfo->GetServiceID()==0)
		return NULL;
	if (!m_EventList[Index].fValid) {
		SYSTEMTIME st;

		if (Index==0) {
			if (pCurTime!=NULL)
				st=*pCurTime;
			else
				GetCurrentJST(&st);
		} else {
			if (!m_EventList[Index-1].EventInfo.GetEndTime(&st))
				return NULL;
		}
		if (!pProgramList->GetEventInfo(m_pChannelInfo->GetNetworkID(),
										m_pChannelInfo->GetTransportStreamID(),
										m_pChannelInfo->GetServiceID(),
										&st,&m_EventList[Index].EventInfo))
			return NULL;
		m_EventList[Index].fValid=true;
	}
	return &m_EventList[Index].EventInfo;
}

const CEventInfoData *CChannelMenuItem::GetEventInfo(int Index) const
{
	if (!m_EventList[Index].fValid)
		return NULL;
	return &m_EventList[Index].EventInfo;
}


CChannelMenu::CChannelMenu(CEpgProgramList *pProgramList,CLogoManager *pLogoManager)
	: m_Flags(0)
	, m_hwnd(NULL)
	, m_hmenu(NULL)
	, m_pProgramList(pProgramList)
	, m_pLogoManager(pLogoManager)
	, m_pChannelList(NULL)
	, m_TextHeight(0)
	, m_ChannelNameWidth(0)
	, m_EventNameWidth(0)
	, m_LogoWidth(26)
	, m_LogoHeight(16)
	, m_MenuLogoMargin(3)
{
}


CChannelMenu::~CChannelMenu()
{
	Destroy();
}


bool CChannelMenu::Create(const CChannelList *pChannelList,int CurChannel,UINT Command,
						  HMENU hmenu,HWND hwnd,unsigned int Flags,int MaxRows)
{
	Destroy();

	m_pChannelList=pChannelList;
	m_CurChannel=CurChannel;
	m_FirstCommand=Command;
	m_LastCommand=Command+pChannelList->NumChannels()-1;
	m_Flags=Flags;
	m_hwnd=hwnd;

	m_MenuPainter.Initialize(hwnd);
	m_MenuPainter.GetItemMargins(&m_Margins);
	if (m_Margins.cxLeftWidth<2)
		m_Margins.cxLeftWidth=2;
	if (m_Margins.cxRightWidth<2)
		m_Margins.cxRightWidth=2;

	HDC hdc=::GetDC(hwnd);

	CreateFont(hdc);
	HFONT hfontOld=DrawUtil::SelectObject(hdc,m_Font);

	SYSTEMTIME st;
	GetBaseTime(&st);
	m_ChannelNameWidth=0;
	m_EventNameWidth=0;
	if (hmenu==NULL) {
		m_hmenu=::CreatePopupMenu();
	} else {
		m_hmenu=hmenu;
		m_Flags|=FLAG_SHARED;
		ClearMenu(hmenu);
	}
	MENUITEMINFO mii;
	mii.cbSize=sizeof(MENUITEMINFO);
	mii.fMask=MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_DATA;
	int PrevSpace=-1;
	for (int i=0,j=0;i<pChannelList->NumChannels();i++) {
		const CChannelInfo *pChInfo=pChannelList->GetChannelInfo(i);
		if (!pChInfo->IsEnabled())
			continue;

		TCHAR szText[256];
		RECT rc;

		if (i==CurChannel)
			DrawUtil::SelectObject(hdc,m_FontCurrent);
		StdUtil::snprintf(szText,lengthof(szText),TEXT("%d: %s"),
						  pChInfo->GetChannelNo(),pChInfo->GetName());
		/*
		m_MenuPainter.GetItemTextExtent(hdc,0,szText,&rc,
										DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
		*/
		::SetRectEmpty(&rc);
		::DrawText(hdc,szText,-1,&rc,DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);
		if (rc.right>m_ChannelNameWidth)
			m_ChannelNameWidth=rc.right;
		mii.wID=m_FirstCommand+i;
		mii.fType=MFT_OWNERDRAW;
		if ((MaxRows>0 && j==MaxRows)
				|| ((Flags&FLAG_SPACEBREAK)!=0 && pChInfo->GetSpace()!=PrevSpace)) {
			mii.fType|=MFT_MENUBREAK;
			j=0;
		}
		mii.fState=MFS_ENABLED;
		if (i==CurChannel)
			mii.fState|=MFS_CHECKED;
		CChannelMenuItem *pItem=new CChannelMenuItem(pChInfo);
		mii.dwItemData=reinterpret_cast<ULONG_PTR>(pItem);
		if ((Flags&FLAG_SHOWEVENTINFO)!=0) {
			const CEventInfoData *pEventInfo=pItem->GetEventInfo(m_pProgramList,0,&st);

			if (pEventInfo!=NULL) {
				GetEventText(pEventInfo,szText,lengthof(szText));
				/*
				m_MenuPainter.GetItemTextExtent(hdc,0,szText,&rc,
												DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
				*/
				::SetRectEmpty(&rc);
				::DrawText(hdc,szText,-1,&rc,DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);
				if (rc.right>m_EventNameWidth)
					m_EventNameWidth=rc.right;
			}
		}
		::InsertMenuItem(m_hmenu,i,TRUE,&mii);
		if (i==CurChannel)
			DrawUtil::SelectObject(hdc,m_Font);
		PrevSpace=pChInfo->GetSpace();
		j++;
	}
	::SelectObject(hdc,hfontOld);
	::ReleaseDC(hwnd,hdc);

	if ((Flags&FLAG_SHOWTOOLTIP)!=0) {
		m_Tooltip.Create(hwnd);
		m_Tooltip.SetMaxWidth(480);
		m_Tooltip.SetPopDelay(30*1000);
		m_Tooltip.AddTrackingTip(1,TEXT(""));
	}

	if ((Flags & FLAG_SHOWLOGO)!=0) {
		if (!m_LogoFrameImage.IsCreated()) {
			m_LogoFrameImage.Load(GetAppClass().GetResourceInstance(),
								  MAKEINTRESOURCE(IDB_LOGOFRAME),LR_CREATEDIBSECTION);
		}
	}

	return true;
}


void CChannelMenu::Destroy()
{
	if (m_hmenu) {
		MENUITEMINFO mii;
		int i;

		mii.cbSize=sizeof(MENUITEMINFO);
		mii.fMask=MIIM_DATA;
		for (i=::GetMenuItemCount(m_hmenu)-1;i>=0;i--) {
			if (::GetMenuItemInfo(m_hmenu,i,TRUE,&mii))
				delete reinterpret_cast<CChannelMenuItem*>(mii.dwItemData);
		}
		if ((m_Flags&FLAG_SHARED)==0)
			::DestroyMenu(m_hmenu);
		else
			ClearMenu(m_hmenu);
		m_hmenu=NULL;
	}
	m_MenuPainter.Finalize();
	m_Tooltip.Destroy();
	m_hwnd=NULL;
}


bool CChannelMenu::Show(UINT Flags,int x,int y)
{
	if (m_hmenu==NULL)
		return false;
	::TrackPopupMenu(m_hmenu,Flags,x,y,0,m_hwnd,NULL);
	return true;
}


bool CChannelMenu::OnMeasureItem(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
	LPMEASUREITEMSTRUCT pmis=reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);

	if (m_hmenu!=NULL && pmis->CtlType==ODT_MENU
			&& pmis->itemID>=m_FirstCommand && pmis->itemID<=m_LastCommand) {
		pmis->itemWidth=m_ChannelNameWidth+
						m_Margins.cxLeftWidth+m_Margins.cxRightWidth;
		if ((m_Flags&FLAG_SHOWLOGO)!=0)
			pmis->itemWidth+=m_LogoWidth+m_MenuLogoMargin;
		if (m_EventNameWidth>0)
			pmis->itemWidth+=m_TextHeight+m_EventNameWidth;
		pmis->itemHeight=max(m_TextHeight,m_LogoHeight)+
							m_Margins.cyTopHeight+m_Margins.cyBottomHeight;
		return true;
	}
	return false;
}


bool CChannelMenu::OnDrawItem(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
	LPDRAWITEMSTRUCT pdis=reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

	if (m_hmenu==NULL || hwnd!=m_hwnd || pdis->CtlType!=ODT_MENU
			|| pdis->itemID<m_FirstCommand || pdis->itemID>m_LastCommand)
		return false;

	const CChannelMenuItem *pItem=reinterpret_cast<CChannelMenuItem*>(pdis->itemData);
	const CChannelInfo *pChInfo=pItem->GetChannelInfo();
	TCHAR szText[256];

	m_MenuPainter.DrawItemBackground(pdis->hDC,pdis->rcItem,pdis->itemState);
	COLORREF crTextColor=m_MenuPainter.GetTextColor(pdis->itemState);

	HFONT hfontOld=DrawUtil::SelectObject(pdis->hDC,
						(pdis->itemState&ODS_CHECKED)==0?m_Font:m_FontCurrent);
	int OldBkMode=::SetBkMode(pdis->hDC,TRANSPARENT);
	COLORREF crOldTextColor=::SetTextColor(pdis->hDC,crTextColor);

	RECT rc;
	rc.left=pdis->rcItem.left+m_Margins.cxLeftWidth;
	rc.top=pdis->rcItem.top+m_Margins.cyTopHeight;
	rc.bottom=pdis->rcItem.bottom-m_Margins.cyBottomHeight;

	if ((m_Flags&FLAG_SHOWLOGO)!=0) {
		HBITMAP hbmLogo=m_pLogoManager->GetAssociatedLogoBitmap(
			pChInfo->GetNetworkID(),pChInfo->GetServiceID(),CLogoManager::LOGOTYPE_SMALL);
		if (hbmLogo!=NULL) {
			DrawUtil::CMemoryDC MemoryDC(pdis->hDC);
			MemoryDC.SetBitmap(hbmLogo);
			int y=rc.top+((rc.bottom-rc.top)-m_LogoHeight)/2;
			BITMAP bm;
			::GetObject(hbmLogo,sizeof(bm),&bm);
			MemoryDC.DrawStretch(pdis->hDC,rc.left+1,y+1,m_LogoWidth-3,m_LogoHeight-3,
								 0,0,bm.bmWidth,bm.bmHeight);
			MemoryDC.SetBitmap(m_LogoFrameImage);
			MemoryDC.DrawAlpha(pdis->hDC,rc.left,y,0,0,m_LogoWidth,m_LogoHeight);
		}
		rc.left+=m_LogoWidth+m_MenuLogoMargin;
	}

	rc.right=rc.left+m_ChannelNameWidth;
	StdUtil::snprintf(szText,lengthof(szText),TEXT("%d: %s"),
					  pChInfo->GetChannelNo(),pChInfo->GetName());
	::DrawText(pdis->hDC,szText,-1,&rc,
			   DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	if ((m_Flags&FLAG_SHOWEVENTINFO)!=0) {
		const CEventInfoData *pEventInfo=pItem->GetEventInfo(0);
		if (pEventInfo!=NULL) {
			int Length=GetEventText(pEventInfo,szText,lengthof(szText));
			rc.left=rc.right+m_TextHeight;
			rc.right=pdis->rcItem.right-m_Margins.cxRightWidth;
			::DrawText(pdis->hDC,szText,Length,&rc,
					   DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		}
	}

	::SetTextColor(pdis->hDC,crOldTextColor);
	::SetBkMode(pdis->hDC,OldBkMode);
	::SelectObject(pdis->hDC,hfontOld);

	return true;
}


bool CChannelMenu::OnMenuSelect(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
	HMENU hmenu=reinterpret_cast<HMENU>(lParam);
	UINT Command=LOWORD(wParam);

	if (hmenu==NULL || hmenu!=m_hmenu || hwnd!=m_hwnd || HIWORD(wParam)==0xFFFF
			|| Command<m_FirstCommand || Command>m_LastCommand) {
		if (m_Tooltip.IsVisible())
			m_Tooltip.TrackActivate(1,false);
		return false;
	}

	if ((m_Flags&FLAG_SHOWTOOLTIP)!=0) {
		MENUITEMINFO mii;

		mii.cbSize=sizeof(mii);
		mii.fMask=MIIM_DATA;
		if (::GetMenuItemInfo(hmenu,Command,FALSE,&mii)) {
			CChannelMenuItem *pItem=reinterpret_cast<CChannelMenuItem*>(mii.dwItemData);
			if (pItem==NULL)
				return false;

			const CEventInfoData *pEventInfo1,*pEventInfo2;
			pEventInfo1=pItem->GetEventInfo(0);
			if (pEventInfo1==NULL) {
				pEventInfo1=pItem->GetEventInfo(m_pProgramList,0);
			}
			if (pEventInfo1!=NULL) {
				TCHAR szText[256*2+1];
				int Length;
				POINT pt;

				Length=GetEventText(pEventInfo1,szText,lengthof(szText)/2);
				pEventInfo2=pItem->GetEventInfo(m_pProgramList,1);
				if (pEventInfo2!=NULL) {
					szText[Length++]=_T('\r');
					szText[Length++]=_T('\n');
					GetEventText(pEventInfo2,szText+Length,lengthof(szText)/2);
				}
				m_Tooltip.SetText(1,szText);
				::GetCursorPos(&pt);
				pt.x+=16;
				pt.y+=max(m_TextHeight,m_LogoHeight)+
							m_Margins.cyTopHeight+m_Margins.cyBottomHeight;
				m_Tooltip.TrackPosition(pt.x,pt.y);
				m_Tooltip.TrackActivate(1,true);
			} else {
				m_Tooltip.TrackActivate(1,false);
			}
		}
	}
	return true;
}


bool CChannelMenu::OnUninitMenuPopup(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
	if (hwnd==m_hwnd && reinterpret_cast<HMENU>(wParam)==m_hmenu) {
		Destroy();
		return true;
	}
	return false;
}


int CChannelMenu::GetEventText(const CEventInfoData *pEventInfo,
							   LPTSTR pszText,int MaxLength) const
{
	SYSTEMTIME stStart,stEnd;
	TCHAR szEnd[16];

	pEventInfo->GetStartTime(&stStart);
	if (pEventInfo->GetEndTime(&stEnd))
		StdUtil::snprintf(szEnd,lengthof(szEnd),
						  TEXT("%02d:%02d"),stEnd.wHour,stEnd.wMinute);
	else
		szEnd[0]='\0';
	return StdUtil::snprintf(pszText,MaxLength,TEXT("%02d:%02d`%s %ls"),
							 stStart.wHour,stStart.wMinute,szEnd,
							 NullToEmptyString(pEventInfo->GetEventName()));
}


void CChannelMenu::CreateFont(HDC hdc)
{
	if (m_Font.IsCreated())
		return;

	LOGFONT lf;
	m_MenuPainter.GetFont(&lf);
	m_Font.Create(&lf);
	lf.lfWeight=FW_BOLD;
	m_FontCurrent.Create(&lf);

	if (hdc!=NULL)
		m_TextHeight=m_Font.GetHeight(hdc);
	else
		m_TextHeight=abs(lf.lfHeight);
	//m_LogoHeight=min(m_TextHeight,14);
	//m_LogoWidth=m_LogoHeight*16/9;
}


void CChannelMenu::GetBaseTime(SYSTEMTIME *pTime)
{
	GetCurrentJST(pTime);
	OffsetSystemTime(pTime,120*1000);
}




CPopupMenu::CPopupMenu()
	: m_hmenu(NULL)
{
}


CPopupMenu::CPopupMenu(HINSTANCE hinst,LPCTSTR pszName)
	: m_hmenu(NULL)
{
	Load(hinst,pszName);
}


CPopupMenu::CPopupMenu(HINSTANCE hinst,UINT ID)
	: m_hmenu(NULL)
{
	Load(hinst,ID);
}


CPopupMenu::CPopupMenu(HMENU hmenu)
	: m_hmenu(NULL)
{
	Attach(hmenu);
}


CPopupMenu::~CPopupMenu()
{
	Destroy();
}


bool CPopupMenu::Create()
{
	Destroy();

	m_hmenu=::CreatePopupMenu();
	if (m_hmenu==NULL)
		return false;
	m_Type=TYPE_CREATED;
	return true;
}


bool CPopupMenu::Load(HINSTANCE hinst,LPCTSTR pszName)
{
	Destroy();

	m_hmenu=::LoadMenu(hinst,pszName);
	if (m_hmenu==NULL)
		return false;
	m_Type=TYPE_RESOURCE;
	return true;
}


bool CPopupMenu::Attach(HMENU hmenu)
{
	if (hmenu==NULL)
		return false;
	Destroy();
	m_hmenu=hmenu;
	m_Type=TYPE_ATTACHED;
	return true;
}


void CPopupMenu::Destroy()
{
	if (m_hmenu!=NULL) {
		if (m_Type!=TYPE_ATTACHED)
			::DestroyMenu(m_hmenu);
		m_hmenu=NULL;
	}
}


int CPopupMenu::GetItemCount() const
{
	if (m_hmenu==NULL)
		return 0;
	return ::GetMenuItemCount(GetPopupHandle());
}


void CPopupMenu::Clear()
{
	if (m_hmenu!=NULL)
		ClearMenu(GetPopupHandle());
}


HMENU CPopupMenu::GetPopupHandle() const
{
	if (m_hmenu==NULL)
		return NULL;
	if (m_Type==TYPE_RESOURCE)
		return ::GetSubMenu(m_hmenu,0);
	return m_hmenu;
}


bool CPopupMenu::Append(UINT ID,LPCTSTR pszText,UINT Flags)
{
	if (m_hmenu==NULL || pszText==NULL)
		return false;
	return ::AppendMenu(GetPopupHandle(),MF_STRING | Flags,ID,pszText)!=FALSE;
}


bool CPopupMenu::AppendUnformatted(UINT ID,LPCTSTR pszText,UINT Flags)
{
	if (m_hmenu==NULL || pszText==NULL)
		return false;
	TCHAR szText[256];
	CopyToMenuText(pszText,szText,lengthof(szText));
	return Append(ID,szText);
}


bool CPopupMenu::Append(HMENU hmenu,LPCTSTR pszText,UINT Flags)
{
	if (m_hmenu==NULL || hmenu==NULL || pszText==NULL)
		return false;
	return ::AppendMenu(GetPopupHandle(),MF_POPUP | Flags,
						reinterpret_cast<UINT_PTR>(hmenu),pszText)!=FALSE;
}


bool CPopupMenu::AppendSeparator()
{
	if (m_hmenu==NULL)
		return false;
	return ::AppendMenu(GetPopupHandle(),MF_SEPARATOR,0,NULL)!=FALSE;
}


bool CPopupMenu::EnableItem(UINT ID,bool fEnable)
{
	if (m_hmenu==NULL)
		return false;
	return ::EnableMenuItem(m_hmenu,ID,MF_BYCOMMAND | (fEnable?MFS_ENABLED:MFS_GRAYED))>=0;
}


bool CPopupMenu::CheckItem(UINT ID,bool fCheck)
{
	if (m_hmenu==NULL)
		return false;
	return ::CheckMenuItem(m_hmenu,ID,MF_BYCOMMAND | (fCheck?MFS_CHECKED:MFS_UNCHECKED))>=0;
}


bool CPopupMenu::CheckRadioItem(UINT FirstID,UINT LastID,UINT CheckID,UINT Flags)
{
	if (m_hmenu==NULL)
		return false;
	return ::CheckMenuRadioItem(GetPopupHandle(),FirstID,LastID,CheckID,Flags)!=FALSE;
}


HMENU CPopupMenu::GetSubMenu(int Pos) const
{
	if (m_hmenu==NULL)
		return false;
	return ::GetSubMenu(GetPopupHandle(),Pos);
}


bool CPopupMenu::Show(HWND hwnd,const POINT *pPos,UINT Flags)
{
	if (m_hmenu==NULL)
		return false;
	POINT pt;
	if (pPos!=NULL)
		pt=*pPos;
	else
		::GetCursorPos(&pt);
	::TrackPopupMenu(GetPopupHandle(),Flags,pt.x,pt.y,0,hwnd,NULL);
	return true;
}


bool CPopupMenu::Show(HMENU hmenu,HWND hwnd,const POINT *pPos,UINT Flags,bool fToggle)
{
	if (m_hmenu==NULL) {
		m_hmenu=hmenu;
		m_Type=TYPE_ATTACHED;
		POINT pt;
		if (pPos!=NULL)
			pt=*pPos;
		else
			::GetCursorPos(&pt);
		::TrackPopupMenu(m_hmenu,Flags,pt.x,pt.y,0,hwnd,NULL);
		m_hmenu=NULL;
	} else {
		if (fToggle)
			::EndMenu();
	}
	return true;
}


bool CPopupMenu::Show(HINSTANCE hinst,LPCTSTR pszName,HWND hwnd,const POINT *pPos,UINT Flags,bool fToggle)
{
	if (m_hmenu==NULL) {
		if (!Load(hinst,pszName))
			return false;
		POINT pt;
		if (pPos!=NULL)
			pt=*pPos;
		else
			::GetCursorPos(&pt);
		::TrackPopupMenu(GetPopupHandle(),Flags,pt.x,pt.y,0,hwnd,NULL);
		Destroy();
	} else {
		if (fToggle)
			::EndMenu();
	}
	return true;
}




CIconMenu::CIconMenu()
	: m_hmenu(NULL)
	, m_hImageList(NULL)
{
}


CIconMenu::~CIconMenu()
{
	Finalize();
}


bool CIconMenu::Initialize(HMENU hmenu,HINSTANCE hinst,LPCTSTR pszImageName,
						   int IconWidth,const ItemInfo *pItemList,int ItemCount)
{
	Finalize();

	if (hmenu==NULL || pszImageName==NULL || IconWidth<1 || pItemList==NULL || ItemCount<1)
		return false;

	int IconCount;

	if (!Util::OS::IsWindowsVistaOrLater()) {
		m_hImageList=::ImageList_LoadImage(hinst,pszImageName,IconWidth,1,0,
										   IMAGE_BITMAP,LR_CREATEDIBSECTION);
		if (m_hImageList==NULL)
			return false;
		IconCount=::ImageList_GetImageCount(m_hImageList);
	} else {
		HBITMAP hbm=static_cast<HBITMAP>(::LoadImage(hinst,pszImageName,IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION));
		if (hbm==NULL)
			return false;
		BITMAP bm;
		if (::GetObject(hbm,sizeof(bm),&bm)!=sizeof(bm) || bm.bmBits==NULL) {
			::DeleteObject(hbm);
			return false;
		}
		for (int x=0;x+IconWidth<=bm.bmWidth;x+=IconWidth) {
			void *pBits;
			HBITMAP hbmIcon=DrawUtil::CreateDIB(IconWidth,bm.bmHeight,32,&pBits);
			BYTE *q=static_cast<BYTE*>(pBits);
			BYTE *p=static_cast<BYTE*>(bm.bmBits)+x*4;
			for (int y=0;y<bm.bmHeight;y++) {
				::CopyMemory(q,p,bm.bmWidth*4);
				q+=IconWidth*4;
				p+=bm.bmWidth*4;
			}
			m_BitmapList.push_back(hbmIcon);
		}
		::DeleteObject(hbm);
		IconCount=bm.bmWidth/IconWidth;
	}

	m_hmenu=hmenu;

	m_ItemList.reserve(ItemCount);
	for (int i=0;i<ItemCount;i++) {
		ItemInfo Item=pItemList[i];
		if (Item.Image<IconCount)
			m_ItemList.push_back(Item);
	}

	return true;
}


void CIconMenu::Finalize()
{
	m_hmenu=NULL;
	m_ItemList.clear();
	if (m_hImageList!=NULL) {
		::ImageList_Destroy(m_hImageList);
		m_hImageList=NULL;
	}
	if (!m_BitmapList.empty()) {
		for (auto i=m_BitmapList.begin();i!=m_BitmapList.end();i++)
			::DeleteObject(*i);
		m_BitmapList.clear();
	}
}


bool CIconMenu::OnInitMenuPopup(HWND hwnd,HMENU hmenu)
{
	if (hmenu!=m_hmenu)
		return false;

	MENUITEMINFO mii;
	mii.cbSize=sizeof(mii);
	int Count=::GetMenuItemCount(hmenu);
	int j=0;
	for (int i=0;i<Count;i++) {
		mii.fMask=MIIM_ID | MIIM_STATE | MIIM_DATA;
		if (::GetMenuItemInfo(hmenu,i,TRUE,&mii)) {
			for (auto itrItem=m_ItemList.begin();itrItem!=m_ItemList.end();++itrItem) {
				if (itrItem->ID==mii.wID) {
					mii.fMask=MIIM_STATE | MIIM_BITMAP | MIIM_DATA;
					mii.dwItemData=(mii.dwItemData & ~ITEM_DATA_IMAGEMASK) | (itrItem->Image+1);
					if (m_hImageList!=NULL) {
						mii.hbmpItem=HBMMENU_CALLBACK;
						if ((mii.fState & MFS_CHECKED)!=0) {
							mii.fState&=~MFS_CHECKED;
							mii.dwItemData|=ITEM_DATA_CHECKED;
						}
					} else {
						mii.hbmpItem=m_BitmapList[itrItem->Image];
						if ((mii.dwItemData & ITEM_DATA_CHECKED)!=0) {
							mii.fState|=MFS_CHECKED;
						}
					}
					::SetMenuItemInfo(hmenu,i,TRUE,&mii);
				}
			}
		}
	}

	if (m_hImageList!=NULL) {
		MENUINFO mi;
		mi.cbSize=sizeof(mi);
		mi.fMask=MIM_STYLE;
		::GetMenuInfo(hmenu,&mi);
		if ((mi.dwStyle & MNS_CHECKORBMP)==0) {
			mi.dwStyle|=MNS_CHECKORBMP;
			::SetMenuInfo(hmenu,&mi);
		}
	}

	return true;
}


bool CIconMenu::OnMeasureItem(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
	LPMEASUREITEMSTRUCT pmis=reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);

	if (pmis->CtlType!=ODT_MENU)
		return false;

	for (auto i=m_ItemList.begin();i!=m_ItemList.end();i++) {
		if (i->ID==pmis->itemID) {
			int cx,cy;
			::ImageList_GetIconSize(m_hImageList,&cx,&cy);
			pmis->itemHeight=max(pmis->itemHeight+3,(UINT)cy+ICON_MARGIN*2);
			pmis->itemWidth=cx+ICON_MARGIN*2+TEXT_MARGIN;
			return true;
		}
	}

	return false;
}


bool CIconMenu::OnDrawItem(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
	LPDRAWITEMSTRUCT pdis=reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

	if (pdis->CtlType!=ODT_MENU || (HMENU)pdis->hwndItem!=m_hmenu)
		return false;

	const int Image=(int)(pdis->itemData & ITEM_DATA_IMAGEMASK)-1;
	if (Image<0)
		return false;

	int cx,cy;
	::ImageList_GetIconSize(m_hImageList,&cx,&cy);
	int x=pdis->rcItem.left+ICON_MARGIN;
	int y=pdis->rcItem.top+((pdis->rcItem.bottom-pdis->rcItem.top)-cy)/2;

	if ((pdis->itemData & ITEM_DATA_CHECKED)!=0) {
		RECT rc;
		COLORREF cr;
		int r,g,b;
		HPEN hpen1,hpen2,hpenOld;

		::SetRect(&rc,x,y,x+cx,y+cy);
		::FillRect(pdis->hDC,&rc,reinterpret_cast<HBRUSH>(COLOR_MENU+1));
		cr=::GetSysColor(COLOR_MENU);
		r=GetRValue(cr);
		g=GetGValue(cr);
		b=GetBValue(cr);
		hpen1=::CreatePen(PS_SOLID,1,RGB(r/2,g/2,b/2));
		hpen2=::CreatePen(PS_SOLID,1,RGB(r+(255-r)/2,g+(255-g)/2,b+(255-b)/2));
		hpenOld=static_cast<HPEN>(::SelectObject(pdis->hDC,hpen1));
		::MoveToEx(pdis->hDC,x+cx+ICON_MARGIN,y-ICON_MARGIN,NULL);
		::LineTo(pdis->hDC,x-ICON_MARGIN,y-ICON_MARGIN);
		::LineTo(pdis->hDC,x-ICON_MARGIN,y+cy+ICON_MARGIN-1);
		::SelectObject(pdis->hDC,hpen2);
		::LineTo(pdis->hDC,x+cx+ICON_MARGIN-1,y+cy+ICON_MARGIN-1);
		::LineTo(pdis->hDC,x+cx+ICON_MARGIN-1,y-ICON_MARGIN);
		::SelectObject(pdis->hDC,hpenOld);
		::DeleteObject(hpen1);
		::DeleteObject(hpen2);
	}

	m_MenuPainter.DrawIcon(m_hImageList,Image,pdis->hDC,x,y,pdis->itemState);

	return true;
}


bool CIconMenu::CheckItem(UINT ID,bool fCheck)
{
	if (m_hmenu==NULL)
		return false;

	MENUITEMINFO mii;
	mii.cbSize=sizeof(mii);
	mii.fMask=MIIM_STATE | MIIM_DATA;
	if (!::GetMenuItemInfo(m_hmenu,ID,FALSE,&mii))
		return false;
	if (fCheck) {
		mii.fState|=MFS_CHECKED;
		mii.dwItemData|=ITEM_DATA_CHECKED;
	} else {
		mii.fState&=~MFS_CHECKED;
		mii.dwItemData&=~ITEM_DATA_CHECKED;
	}
	::SetMenuItemInfo(m_hmenu,ID,FALSE,&mii);
	return true;
}


bool CIconMenu::CheckRadioItem(UINT FirstID,UINT LastID,UINT CheckID)
{
	if (m_hmenu==NULL)
		return false;

	const int ItemCount=::GetMenuItemCount(m_hmenu);

	MENUITEMINFO mii;
	mii.cbSize=sizeof(mii);

	for (int i=0;i<ItemCount;i++) {
		mii.fMask=MIIM_ID | MIIM_STATE | MIIM_DATA;
		if (::GetMenuItemInfo(m_hmenu,i,TRUE,&mii)
				&& mii.wID>=FirstID && mii.wID<=LastID) {
			mii.fMask=MIIM_STATE | MIIM_DATA;
			if (mii.wID==CheckID) {
				mii.fState|=MFS_CHECKED;
				mii.dwItemData|=ITEM_DATA_CHECKED;
			} else {
				mii.fState&=~MFS_CHECKED;
				mii.dwItemData&=~ITEM_DATA_CHECKED;
			}
			::SetMenuItemInfo(m_hmenu,i,TRUE,&mii);
		}
	}

	return true;
}




#define DROPDOWNMENU_WINDOW_CLASS APP_NAME TEXT(" Drop Down Menu")


HINSTANCE CDropDownMenu::m_hinst=NULL;


bool CDropDownMenu::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		if (Util::OS::IsWindowsXPOrLater()) {
			wc.style=CS_DROPSHADOW;
		} else {
			wc.style=0;
		}
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=DROPDOWNMENU_WINDOW_CLASS;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CDropDownMenu::CDropDownMenu()
	: m_hwnd(NULL)
	, m_hwndMessage(NULL)
{
}


CDropDownMenu::~CDropDownMenu()
{
	Clear();
}


void CDropDownMenu::Clear()
{
	for (size_t i=0;i<m_ItemList.size();i++)
		delete m_ItemList[i];
	m_ItemList.clear();
}


bool CDropDownMenu::AppendItem(CItem *pItem)
{
	if (pItem==NULL)
		return false;
	m_ItemList.push_back(pItem);
	return true;
}


bool CDropDownMenu::InsertItem(int Index,CItem *pItem)
{
	if (pItem==NULL || Index<0 || (size_t)Index>m_ItemList.size())
		return false;
	std::vector<CItem*>::iterator i=m_ItemList.begin();
	std::advance(i,Index);
	m_ItemList.insert(i,pItem);
	return true;
}


bool CDropDownMenu::AppendSeparator()
{
	CItem *pItem=new CItem(-1,NULL);

	if (!AppendItem(pItem)) {
		delete pItem;
		return false;
	}
	return true;
}


bool CDropDownMenu::InsertSeparator(int Index)
{
	CItem *pItem=new CItem(-1,NULL);

	if (!InsertItem(Index,pItem)) {
		delete pItem;
		return false;
	}
	return true;
}


bool CDropDownMenu::DeleteItem(int Command)
{
	int Index=CommandToIndex(Command);

	if (Index<0)
		return false;
	std::vector<CItem*>::iterator i=m_ItemList.begin();
	std::advance(i,Index);
	delete *i;
	m_ItemList.erase(i);
	return true;
}


bool CDropDownMenu::SetItemText(int Command,LPCTSTR pszText)
{
	int Index=CommandToIndex(Command);

	if (Index<0)
		return false;
	return m_ItemList[Index]->SetText(pszText);
}


int CDropDownMenu::CommandToIndex(int Command) const
{
	if (Command<0)
		return -1;
	for (size_t i=0;i<m_ItemList.size();i++) {
		if (m_ItemList[i]->GetCommand()==Command)
			return (int)i;
	}
	return -1;
}


bool CDropDownMenu::Show(HWND hwndOwner,HWND hwndMessage,const POINT *pPos,int CurItem,UINT Flags)
{
	if (m_ItemList.empty() || m_hwnd!=NULL)
		return false;

	HWND hwnd=::CreateWindowEx(WS_EX_NOACTIVATE | WS_EX_TOPMOST,DROPDOWNMENU_WINDOW_CLASS,
							   NULL,WS_POPUP,0,0,0,0,hwndOwner,NULL,m_hinst,this);
	if (hwnd==NULL)
		return false;

	m_HotItem=CommandToIndex(CurItem);
	m_hwndMessage=hwndMessage;

	HDC hdc=::GetDC(hwnd);
	HFONT hfontOld=DrawUtil::SelectObject(hdc,m_Font);
	int MaxWidth=0;
	for (size_t i=0;i<m_ItemList.size();i++) {
		int Width=m_ItemList[i]->GetWidth(hdc);
		if (Width>MaxWidth)
			MaxWidth=Width;
	}
	TEXTMETRIC tm;
	::GetTextMetrics(hdc,&tm);
	m_ItemWidth=MaxWidth+m_ItemMargin.cxLeftWidth+m_ItemMargin.cxRightWidth;
	m_ItemHeight=tm.tmHeight/*-tm.tmInternalLeading*/+
		m_ItemMargin.cyTopHeight+m_ItemMargin.cyBottomHeight;
	::SelectObject(hdc,hfontOld);
	::ReleaseDC(hwnd,hdc);

	const int HorzMargin=m_WindowMargin.cxLeftWidth+m_WindowMargin.cxRightWidth;
	const int VertMargin=m_WindowMargin.cyTopHeight+m_WindowMargin.cyBottomHeight;
	m_MaxRows=(int)m_ItemList.size();
	int Columns=1;
	int x=pPos->x,y=pPos->y;
	MONITORINFO mi;
	mi.cbSize=sizeof(mi);
	if (::GetMonitorInfo(::MonitorFromPoint(*pPos,MONITOR_DEFAULTTONEAREST),&mi)) {
		int Rows=((mi.rcMonitor.bottom-y)-VertMargin)/m_ItemHeight;

		if ((int)m_ItemList.size()>Rows) {
			int MaxColumns=((mi.rcMonitor.right-mi.rcMonitor.left)-HorzMargin)/m_ItemWidth;
			if (MaxColumns>1) {
				if (Rows*MaxColumns>=(int)m_ItemList.size()) {
					Columns=((int)m_ItemList.size()+Rows-1)/Rows;
					m_MaxRows=Rows;
				} else {
					Columns=MaxColumns;
					m_MaxRows=((int)m_ItemList.size()+Columns-1)/Columns;
					if ((Columns-1)*m_MaxRows>=(int)m_ItemList.size())
						Columns--;
				}
			}
		}
		int Width=Columns*m_ItemWidth+HorzMargin;
		int Height=m_MaxRows*m_ItemHeight+VertMargin;
		if (x+Width>mi.rcMonitor.right)
			x=max(mi.rcMonitor.right-Width,0);
		if (y+Height>mi.rcMonitor.bottom)
			y=max(mi.rcMonitor.bottom-Height,0);
	}

	::MoveWindow(hwnd,x,y,
				 m_ItemWidth*Columns+HorzMargin,
				 m_ItemHeight*m_MaxRows+VertMargin,
				 FALSE);
	::ShowWindow(hwnd,SW_SHOWNA);

	return true;
}


bool CDropDownMenu::Hide()
{
	if (m_hwnd!=NULL)
		::DestroyWindow(m_hwnd);
	return true;
}


bool CDropDownMenu::GetPosition(RECT *pRect)
{
	if (m_hwnd==NULL)
		return false;
	return ::GetWindowRect(m_hwnd,pRect)!=FALSE;
}


bool CDropDownMenu::GetItemRect(int Index,RECT *pRect) const
{
	if (Index<0 || (size_t)Index>=m_ItemList.size())
		return false;
	pRect->left=(Index/m_MaxRows)*m_ItemWidth;
	pRect->top=(Index%m_MaxRows)*m_ItemHeight;
	pRect->right=pRect->left+m_ItemWidth;
	pRect->bottom=pRect->top+m_ItemHeight;
	::OffsetRect(pRect,m_WindowMargin.cxLeftWidth,m_WindowMargin.cyTopHeight);
	return true;
}


int CDropDownMenu::HitTest(int x,int y) const
{
	POINT pt;

	pt.x=x;
	pt.y=y;
	for (int i=0;i<(int)m_ItemList.size();i++) {
		RECT rc;

		GetItemRect(i,&rc);
		if (::PtInRect(&rc,pt)) {
			if (m_ItemList[i]->IsSeparator())
				return -1;
			return i;
		}
	}
	return -1;
}


void CDropDownMenu::UpdateItem(int Index) const
{
	if (m_hwnd!=NULL) {
		RECT rc;
		if (GetItemRect(Index,&rc))
			::InvalidateRect(m_hwnd,&rc,TRUE);
	}
}


void CDropDownMenu::Draw(HDC hdc,const RECT *pPaintRect)
{
	HFONT hfontOld=DrawUtil::SelectObject(hdc,m_Font);
	int OldBkMode=::SetBkMode(hdc,TRANSPARENT);
	COLORREF OldTextColor=::GetTextColor(hdc);
	RECT rc;

	::GetClientRect(m_hwnd,&rc);
	m_MenuPainter.DrawBackground(hdc,rc);
	m_MenuPainter.DrawBorder(hdc,rc);

	for (int i=0;i<(int)m_ItemList.size();i++) {
		GetItemRect(i,&rc);
		if (rc.bottom>pPaintRect->top && rc.top<pPaintRect->bottom) {
			CItem *pItem=m_ItemList[i];

			if (pItem->IsSeparator()) {
				m_MenuPainter.DrawSeparator(hdc,rc);
			} else {
				const bool fHighlight=i==m_HotItem;
				const UINT State=fHighlight?ODS_SELECTED:0;

				m_MenuPainter.DrawItemBackground(hdc,rc,State);
				::SetTextColor(hdc,m_MenuPainter.GetTextColor(State));
				rc.left+=m_ItemMargin.cxLeftWidth;
				rc.right-=m_ItemMargin.cxRightWidth;
				pItem->Draw(hdc,&rc);
			}
		}
	}
	::SetTextColor(hdc,OldTextColor);
	::SetBkMode(hdc,OldBkMode);
	::SelectObject(hdc,hfontOld);
}


CDropDownMenu *CDropDownMenu::GetThis(HWND hwnd)
{
	return reinterpret_cast<CDropDownMenu*>(::GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


LRESULT CALLBACK CDropDownMenu::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CDropDownMenu *pThis=static_cast<CDropDownMenu*>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);

			::SetWindowLongPtr(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(pThis));

			pThis->m_hwnd=hwnd;
			pThis->m_fTrackMouseEvent=false;

			pThis->m_MenuPainter.Initialize(hwnd);
			pThis->m_MenuPainter.GetItemMargins(&pThis->m_ItemMargin);
			if (pThis->m_ItemMargin.cxLeftWidth<4)
				pThis->m_ItemMargin.cxLeftWidth=4;
			if (pThis->m_ItemMargin.cxRightWidth<4)
				pThis->m_ItemMargin.cxRightWidth=4;
			pThis->m_MenuPainter.GetMargins(&pThis->m_WindowMargin);
			LOGFONT lf;
			pThis->m_MenuPainter.GetFont(&lf);
			pThis->m_Font.Create(&lf);
		}
		return 0;

	case WM_PAINT:
		{
			CDropDownMenu *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;

			::BeginPaint(hwnd,&ps);
			pThis->Draw(ps.hdc,&ps.rcPaint);
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			CDropDownMenu *pThis=GetThis(hwnd);
			int Item=pThis->HitTest(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));

			if (Item!=pThis->m_HotItem) {
				int OldHotItem=pThis->m_HotItem;

				pThis->m_HotItem=Item;
				if (OldHotItem>=0)
					pThis->UpdateItem(OldHotItem);
				if (Item>=0)
					pThis->UpdateItem(Item);
			}
			if (!pThis->m_fTrackMouseEvent) {
				TRACKMOUSEEVENT tme;

				tme.cbSize=sizeof(tme);
				tme.dwFlags=TME_LEAVE;
				tme.hwndTrack=hwnd;
				if (::TrackMouseEvent(&tme))
					pThis->m_fTrackMouseEvent=true;
			}
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			CDropDownMenu *pThis=GetThis(hwnd);

			if (pThis->m_HotItem>=0) {
				::DestroyWindow(hwnd);
				::SendMessage(pThis->m_hwndMessage,WM_COMMAND,
							  pThis->m_ItemList[pThis->m_HotItem]->GetCommand(),0);
			}
		}
		return 0;

	case WM_MOUSELEAVE:
		::DestroyWindow(hwnd);
		return 0;

	case WM_MOUSEACTIVATE:
		return MA_NOACTIVATE;

	case WM_DESTROY:
		{
			CDropDownMenu *pThis=GetThis(hwnd);

			pThis->m_Font.Destroy();
			pThis->m_MenuPainter.Finalize();
			pThis->m_hwnd=NULL;
		}
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}




CDropDownMenu::CItem::CItem(int Command,LPCTSTR pszText)
	: m_Command(Command)
	, m_Text(pszText)
	, m_Width(0)
{
}


CDropDownMenu::CItem::~CItem()
{
}


bool CDropDownMenu::CItem::SetText(LPCTSTR pszText)
{
	return m_Text.Set(pszText);
}


int CDropDownMenu::CItem::GetWidth(HDC hdc)
{
	if (!m_Text.IsEmpty() && m_Width==0) {
		SIZE sz;

		::GetTextExtentPoint32(hdc,m_Text.Get(),m_Text.Length(),&sz);
		m_Width=sz.cx;
	}
	return m_Width;
}


void CDropDownMenu::CItem::Draw(HDC hdc,const RECT *pRect)
{
	if (!m_Text.IsEmpty()) {
		RECT rc=*pRect;

		::DrawText(hdc,m_Text.Get(),-1,&rc,DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
	}
}
