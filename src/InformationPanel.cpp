#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "InformationPanel.h"
#include "EpgUtil.h"
#include "EventInfoUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


#define IDC_PROGRAMINFO		1000
#define CM_PROGRAMINFOPREV	1001
#define CM_PROGRAMINFONEXT	1002

static const LPCTSTR SUBCLASS_PROP_NAME=APP_NAME TEXT("This");




const LPCTSTR CInformationPanel::m_pszClassName=APP_NAME TEXT(" Information Panel");
HINSTANCE CInformationPanel::m_hinst=NULL;


bool CInformationPanel::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW | CS_VREDRAW;
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


CInformationPanel::CInformationPanel()
	: CSettingsBase(TEXT("InformationPanel"))

	, m_FontHeight(0)

	, m_hwndProgramInfo(NULL)
	, m_pOldProgramInfoProc(NULL)
	, m_fUseRichEdit(true)
	, m_fProgramInfoCursorOverLink(false)
	, m_HotButton(-1)
{
	RegisterItem<CVideoInfoItem>();
	RegisterItem<CVideoDecoderItem>();
	RegisterItem<CVideoRendererItem>();
	RegisterItem<CAudioDeviceItem>(false);
	RegisterItem<CSignalLevelItem>();
	RegisterItem<CMediaBitRateItem>(false);
	RegisterItem<CErrorItem>();
	RegisterItem<CRecordItem>();
	RegisterItem<CServiceItem>(false);
	RegisterItem<CProgramInfoItem>();
}


CInformationPanel::~CInformationPanel()
{
	Destroy();

	for (int i=0;i<NUM_ITEMS;i++)
		delete m_ItemList[i];
}


bool CInformationPanel::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 m_pszClassName,TEXT("情報"),m_hinst);
}


void CInformationPanel::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);
}


void CInformationPanel::NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	m_Style.NormalizeStyle(pStyleManager);
}


void CInformationPanel::SetTheme(const TVTest::Theme::CThemeManager *pThemeManager)
{
	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_PANEL_CONTENT,
							&m_Theme.Style);
	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_INFORMATIONPANEL_EVENTINFO,
							&m_Theme.ProgramInfoStyle);
	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_INFORMATIONPANEL_BUTTON,
							&m_Theme.ButtonStyle);
	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_INFORMATIONPANEL_BUTTON_HOT,
							&m_Theme.ButtonHotStyle);

	if (m_hwnd!=NULL) {
		m_BackBrush.Create(m_Theme.Style.Back.Fill.GetSolidColor());
		m_ProgramInfoBackBrush.Create(m_Theme.ProgramInfoStyle.Back.Fill.GetSolidColor());

		Invalidate();

		if (m_hwndProgramInfo!=NULL) {
			if (m_fUseRichEdit) {
				::SendMessage(m_hwndProgramInfo,EM_SETBKGNDCOLOR,0,
							  (COLORREF)m_Theme.ProgramInfoStyle.Back.Fill.GetSolidColor());
				POINT ptScroll;
				::SendMessage(m_hwndProgramInfo,EM_GETSCROLLPOS,0,reinterpret_cast<LPARAM>(&ptScroll));
				UpdateProgramInfoText();
				::SendMessage(m_hwndProgramInfo,EM_SETSCROLLPOS,0,reinterpret_cast<LPARAM>(&ptScroll));
			}
			::InvalidateRect(m_hwndProgramInfo,NULL,TRUE);

			if (IsItemVisible(ITEM_PROGRAMINFO))
				SendSizeMessage();
		}
	}
}


bool CInformationPanel::IsVisible() const
{
	return m_hwnd!=NULL;
}


bool CInformationPanel::SetFont(const LOGFONT *pFont)
{
	if (!m_Font.Create(pFont))
		return false;
	if (m_hwnd!=NULL) {
		CalcFontHeight();
		if (m_fUseRichEdit) {
			SetWindowFont(m_hwndProgramInfo,m_Font.GetHandle(),FALSE);
			UpdateProgramInfoText();
		} else {
			SetWindowFont(m_hwndProgramInfo,m_Font.GetHandle(),TRUE);
		}
		SendSizeMessage();
		Invalidate();
	}
	return true;
}



CInformationPanel::CItem *CInformationPanel::GetItem(int Item)
{
	if (Item<0 || Item>=NUM_ITEMS)
		return NULL;
	return m_ItemList[Item];
}


const CInformationPanel::CItem *CInformationPanel::GetItem(int Item) const
{
	if (Item<0 || Item>=NUM_ITEMS)
		return NULL;
	return m_ItemList[Item];
}


bool CInformationPanel::SetItemVisible(int Item,bool fVisible)
{
	CItem *pItem=GetItem(Item);

	if (pItem==NULL)
		return false;

	if (pItem->IsVisible()!=fVisible) {
		pItem->SetVisible(fVisible);

		if (m_hwnd!=NULL) {
			if (IsItemVisible(ITEM_PROGRAMINFO))
				SendSizeMessage();
			if (Item==ITEM_PROGRAMINFO)
				::ShowWindow(m_hwndProgramInfo,fVisible?SW_SHOW:SW_HIDE);
			::InvalidateRect(m_hwnd,NULL,TRUE);
		}
	}

	return true;
}


bool CInformationPanel::IsItemVisible(int Item) const
{
	const CItem *pItem=GetItem(Item);

	return pItem!=NULL && pItem->IsVisible();
}


bool CInformationPanel::UpdateItem(int Item)
{
	CItem *pItem=GetItem(Item);

	if (pItem==NULL || !pItem->Update())
		return false;

	if (m_hwnd!=NULL && pItem->IsVisible())
		RedrawItem(Item);

	return true;
}


void CInformationPanel::RedrawItem(int Item)
{
	if (m_hwnd!=NULL && IsItemVisible(Item)) {
		RECT rc;

		GetItemRect(Item,&rc);
		Invalidate(&rc);
	}
}


bool CInformationPanel::ResetItem(int Item)
{
	CItem *pItem=GetItem(Item);

	if (pItem==NULL)
		return false;

	pItem->Reset();

	if (Item==ITEM_PROGRAMINFO) {
		m_ProgramInfoLinkList.clear();
		m_fProgramInfoCursorOverLink=false;
		if (m_hwnd!=NULL) {
			::SetWindowText(m_hwndProgramInfo,TEXT(""));
			RedrawButton(BUTTON_PROGRAMINFOPREV);
			RedrawButton(BUTTON_PROGRAMINFONEXT);
		}
	}

	if (m_hwnd!=NULL && pItem->IsVisible())
		RedrawItem(Item);

	return true;
}


bool CInformationPanel::UpdateAllItems()
{
	bool fUpdated=false;

	for (int i=0;i<NUM_ITEMS;i++) {
		if (m_ItemList[i]->Update())
			fUpdated=true;
	}

	if (!fUpdated)
		return false;

	if (m_hwnd!=NULL)
		Invalidate();

	return true;
}


bool CInformationPanel::SetProgramInfoRichEdit(bool fRichEdit)
{
	if (fRichEdit!=m_fUseRichEdit) {
		if (m_hwndProgramInfo!=NULL) {
			::DestroyWindow(m_hwndProgramInfo);
			m_hwndProgramInfo=NULL;
		}
		m_fUseRichEdit=fRichEdit;
		if (m_hwnd!=NULL) {
			CreateProgramInfoEdit();
			SendSizeMessage();
			if (IsItemVisible(ITEM_PROGRAMINFO))
				::ShowWindow(m_hwndProgramInfo,SW_SHOW);
		}
	}

	return true;
}


void CInformationPanel::UpdateProgramInfoText()
{
	if (m_hwndProgramInfo!=NULL) {
		const TVTest::String &InfoText=
			static_cast<const CProgramInfoItem*>(m_ItemList[ITEM_PROGRAMINFO])->GetInfoText();

		if (m_fUseRichEdit) {
			::SendMessage(m_hwndProgramInfo,WM_SETREDRAW,FALSE,0);
			::SetWindowText(m_hwndProgramInfo,TEXT(""));
			if (!InfoText.empty()) {
				LOGFONT lf;
				CHARFORMAT cf;
				HDC hdc=::GetDC(m_hwndProgramInfo);

				m_Font.GetLogFont(&lf);
				CRichEditUtil::LogFontToCharFormat(hdc,&lf,&cf);
				cf.dwMask|=CFM_COLOR;
				cf.crTextColor=m_Theme.ProgramInfoStyle.Fore.Fill.GetSolidColor();
				::ReleaseDC(m_hwndProgramInfo,hdc);
				CRichEditUtil::AppendText(m_hwndProgramInfo,InfoText.c_str(),&cf);
				CRichEditUtil::DetectURL(m_hwndProgramInfo,&cf,0,-1,
										 CRichEditUtil::URL_NO_LINK | CRichEditUtil::URL_TO_HALF_WIDTH,
										 &m_ProgramInfoLinkList);
				POINT pt={0,0};
				::SendMessage(m_hwndProgramInfo,EM_SETSCROLLPOS,0,reinterpret_cast<LPARAM>(&pt));
			}
			::SendMessage(m_hwndProgramInfo,WM_SETREDRAW,TRUE,0);
			::InvalidateRect(m_hwndProgramInfo,NULL,TRUE);
		} else {
			::SetWindowText(m_hwndProgramInfo,InfoText.c_str());
		}
	}
}


bool CInformationPanel::CreateProgramInfoEdit()
{
	if (m_hwnd==NULL || m_hwndProgramInfo!=NULL)
		return false;

	if (m_fUseRichEdit) {
		m_RichEditUtil.LoadRichEditLib();
		m_hwndProgramInfo=::CreateWindowEx(0,m_RichEditUtil.GetWindowClassName(),TEXT(""),
			WS_CHILD | WS_CLIPSIBLINGS | WS_VSCROLL
				| ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_NOHIDESEL,
			0,0,0,0,m_hwnd,reinterpret_cast<HMENU>(IDC_PROGRAMINFO),m_hinst,NULL);
		if (m_hwndProgramInfo==NULL)
			return false;
		::SendMessage(m_hwndProgramInfo,EM_SETEVENTMASK,0,ENM_MOUSEEVENTS | ENM_LINK);
		::SendMessage(m_hwndProgramInfo,EM_SETBKGNDCOLOR,0,
					  (COLORREF)m_Theme.ProgramInfoStyle.Back.Fill.GetSolidColor());
		m_fProgramInfoCursorOverLink=false;
	} else {
		m_hwndProgramInfo=::CreateWindowEx(0,TEXT("EDIT"),TEXT(""),
			WS_CHILD | WS_CLIPSIBLINGS | WS_VSCROLL
				| ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
			0,0,0,0,m_hwnd,reinterpret_cast<HMENU>(IDC_PROGRAMINFO),m_hinst,NULL);
		if (m_hwndProgramInfo==NULL)
			return false;
		::SetProp(m_hwndProgramInfo,SUBCLASS_PROP_NAME,this);
		m_pOldProgramInfoProc=SubclassWindow(m_hwndProgramInfo,ProgramInfoHookProc);
	}
	SetWindowFont(m_hwndProgramInfo,m_Font.GetHandle(),FALSE);
	UpdateProgramInfoText();

	return true;
}


LRESULT CInformationPanel::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			InitializeUI();

			if (!m_Font.IsCreated())
				CreateDefaultFont(&m_Font);
			m_BackBrush.Create(m_Theme.Style.Back.Fill.GetSolidColor());
			m_ProgramInfoBackBrush.Create(m_Theme.ProgramInfoStyle.Back.Fill.GetSolidColor());
			CalcFontHeight();

			m_HotButton=-1;

			CProgramInfoItem *pProgramInfoItem=
				static_cast<CProgramInfoItem*>(GetItem(ITEM_PROGRAMINFO));
			CreateProgramInfoEdit();
			if (pProgramInfoItem->IsVisible())
				::ShowWindow(m_hwndProgramInfo,SW_SHOW);
		}
		return 0;

	HANDLE_MSG(hwnd,WM_COMMAND,OnCommand);

	case WM_SIZE:
		if (IsItemVisible(ITEM_PROGRAMINFO)) {
			RECT rc;

			GetItemRect(ITEM_PROGRAMINFO,&rc);
			TVTest::Theme::SubtractBorderRect(m_Theme.ProgramInfoStyle.Back.Border,&rc);
			::MoveWindow(m_hwndProgramInfo,rc.left,rc.top,
						 rc.right-rc.left,rc.bottom-rc.top,TRUE);
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

	case WM_CTLCOLORSTATIC:
		{
			HDC hdc=reinterpret_cast<HDC>(wParam);

			::SetTextColor(hdc,m_Theme.ProgramInfoStyle.Fore.Fill.GetSolidColor());
			::SetBkColor(hdc,m_Theme.ProgramInfoStyle.Back.Fill.GetSolidColor());
			return reinterpret_cast<LRESULT>(m_ProgramInfoBackBrush.GetHandle());
		}

	case WM_LBUTTONDOWN:
		{
			int Button=ButtonHitTest(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));

			if (Button>=0) {
				SetHotButton(Button);
				::SetCapture(hwnd);
			}
		}
		return 0;

	case WM_LBUTTONUP:
		if (::GetCapture()==hwnd) {
			if (m_HotButton>=0) {
				SetHotButton(ButtonHitTest(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)));

				switch (m_HotButton) {
				case BUTTON_PROGRAMINFOPREV:
					::SendMessage(hwnd,WM_COMMAND,CM_PROGRAMINFOPREV,0);
					break;
				case BUTTON_PROGRAMINFONEXT:
					::SendMessage(hwnd,WM_COMMAND,CM_PROGRAMINFONEXT,0);
					break;
				}
			}

			::ReleaseCapture();
		}
		return 0;

	case WM_RBUTTONUP:
		{
			HMENU hmenu;
			POINT pt;

			hmenu=::LoadMenu(m_hinst,MAKEINTRESOURCE(IDM_INFORMATIONPANEL));
			for (int i=0;i<NUM_ITEMS;i++)
				CheckMenuItem(hmenu,CM_INFORMATIONPANEL_ITEM_FIRST+i,
							  MF_BYCOMMAND | (IsItemVisible(i)?MFS_CHECKED:MFS_UNCHECKED));
			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			::ClientToScreen(hwnd,&pt);
			::TrackPopupMenu(::GetSubMenu(hmenu,0),TPM_RIGHTBUTTON,pt.x,pt.y,0,hwnd,NULL);
			::DestroyMenu(hmenu);
		}
		return TRUE;

	case WM_MOUSEMOVE:
		{
			SetHotButton(ButtonHitTest(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)));

			if (::GetCapture()==NULL) {
				TRACKMOUSEEVENT tme;
				tme.cbSize=sizeof(TRACKMOUSEEVENT);
				tme.dwFlags=TME_LEAVE;
				tme.hwndTrack=hwnd;
				::TrackMouseEvent(&tme);
			}
		}
		return 0;

	case WM_MOUSELEAVE:
	case WM_CAPTURECHANGED:
		if (m_HotButton>=0)
			SetHotButton(-1);
		return 0;

	case WM_SETCURSOR:
		if ((HWND)wParam==hwnd) {
			if (LOWORD(lParam)==HTCLIENT && m_HotButton>=0) {
				::SetCursor(::LoadCursor(NULL,IDC_HAND));
				return TRUE;
			}
		} else if ((HWND)wParam==m_hwndProgramInfo
				&& LOWORD(lParam)==HTCLIENT
				&& m_fUseRichEdit
				&& m_fProgramInfoCursorOverLink) {
			::SetCursor(::LoadCursor(NULL,IDC_HAND));
			return TRUE;
		}
		break;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case EN_MSGFILTER:
			{
				MSGFILTER *pMsgFilter=reinterpret_cast<MSGFILTER*>(lParam);

				switch (pMsgFilter->msg) {
				case WM_RBUTTONUP:
					TVTest::EventInfoUtil::EventInfoContextMenu(hwnd,m_hwndProgramInfo);
					break;

				case WM_LBUTTONDOWN:
					m_ProgramInfoClickPos.x=GET_X_LPARAM(pMsgFilter->lParam);
					m_ProgramInfoClickPos.y=GET_Y_LPARAM(pMsgFilter->lParam);
					break;

				case WM_MOUSEMOVE:
					m_ProgramInfoClickPos.x=-1;
					m_ProgramInfoClickPos.y=-1;
					{
						POINT pt={GET_X_LPARAM(pMsgFilter->lParam),GET_Y_LPARAM(pMsgFilter->lParam)};

						if (CRichEditUtil::LinkHitTest(m_hwndProgramInfo,pt,m_ProgramInfoLinkList)>=0) {
							m_fProgramInfoCursorOverLink=true;
							::SetCursor(::LoadCursor(NULL,IDC_HAND));
						} else {
							m_fProgramInfoCursorOverLink=false;
						}
					}
					break;

				case WM_LBUTTONUP:
					{
						POINT pt={GET_X_LPARAM(pMsgFilter->lParam),GET_Y_LPARAM(pMsgFilter->lParam)};

						if (m_ProgramInfoClickPos.x==pt.x && m_ProgramInfoClickPos.y==pt.y)
							CRichEditUtil::HandleLinkClick(m_hwndProgramInfo,pt,m_ProgramInfoLinkList);
					}
					break;
				}
			}
			return 0;

#if 0
		case EN_LINK:
			{
				ENLINK *penl=reinterpret_cast<ENLINK*>(lParam);

				if (penl->msg==WM_LBUTTONUP)
					CRichEditUtil::HandleLinkClick(penl);
			}
			return 0;
#endif
		}
		break;

	case WM_DISPLAYCHANGE:
		m_Offscreen.Destroy();
		return 0;

	case WM_DESTROY:
		m_BackBrush.Destroy();
		m_ProgramInfoBackBrush.Destroy();
		m_Offscreen.Destroy();
		m_hwndProgramInfo=NULL;
		return 0;
	}

	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


LRESULT CALLBACK CInformationPanel::ProgramInfoHookProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	CInformationPanel *pThis=static_cast<CInformationPanel*>(::GetProp(hwnd,SUBCLASS_PROP_NAME));

	if (pThis==NULL)
		return ::DefWindowProc(hwnd,uMsg,wParam,lParam);

	switch (uMsg) {
	case WM_RBUTTONDOWN:
		return 0;

	case WM_RBUTTONUP:
		if (!static_cast<const CProgramInfoItem*>(pThis->GetItem(ITEM_PROGRAMINFO))->GetInfoText().empty()) {
			HMENU hmenu=::CreatePopupMenu();
			POINT pt;
			int Command;

			::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,1,TEXT("コピー(&C)"));
			::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,2,TEXT("すべて選択(&A)"));

			::GetCursorPos(&pt);
			Command=::TrackPopupMenu(hmenu,TPM_RETURNCMD,pt.x,pt.y,0,hwnd,NULL);
			if (Command==1) {
				DWORD Start,End;

				::SendMessage(hwnd,WM_SETREDRAW,FALSE,0);
				::SendMessage(hwnd,EM_GETSEL,(WPARAM)&Start,(LPARAM)&End);
				if (Start==End)
					::SendMessage(hwnd,EM_SETSEL,0,-1);
				::SendMessage(hwnd,WM_COPY,0,0);
				if (Start==End)
					::SendMessage(hwnd,EM_SETSEL,Start,End);
				::SendMessage(hwnd,WM_SETREDRAW,TRUE,0);
			} else if (Command==2) {
				::SendMessage(hwnd,EM_SETSEL,0,-1);
			}
			::DestroyMenu(hmenu);
		}
		return 0;

	case WM_NCDESTROY:
		SubclassWindow(hwnd,pThis->m_pOldProgramInfoProc);
		::RemoveProp(hwnd,SUBCLASS_PROP_NAME);
		pThis->m_hwndProgramInfo=NULL;
		break;
	}

	return ::CallWindowProc(pThis->m_pOldProgramInfoProc,hwnd,uMsg,wParam,lParam);
}


void CInformationPanel::OnCommand(HWND hwnd,int id,HWND hwndCtl,UINT codeNotify)
{
	switch (id) {
	case CM_PROGRAMINFOPREV:
	case CM_PROGRAMINFONEXT:
		{
			bool fNext=id==CM_PROGRAMINFONEXT;
			CProgramInfoItem *pItem=
				static_cast<CProgramInfoItem*>(GetItem(ITEM_PROGRAMINFO));

			if (fNext!=pItem->IsNext()) {
				pItem->SetNext(fNext);
				RedrawButton(BUTTON_PROGRAMINFOPREV);
				RedrawButton(BUTTON_PROGRAMINFONEXT);
			}
		}
		return;

	default:
		if (id>=CM_INFORMATIONPANEL_ITEM_FIRST
				&& id<CM_INFORMATIONPANEL_ITEM_FIRST+NUM_ITEMS) {
			int Item=id-CM_INFORMATIONPANEL_ITEM_FIRST;

			SetItemVisible(Item,!IsItemVisible(Item));
			return;
		}
	}
}


void CInformationPanel::GetItemRect(int Item,RECT *pRect) const
{
	int VisibleItemCount;

	GetClientRect(pRect);
	VisibleItemCount=0;
	for (int i=0;i<Item;i++) {
		if (IsItemVisible(i))
			VisibleItemCount++;
	}
	pRect->top=(m_FontHeight+m_Style.LineSpacing)*VisibleItemCount;
	if (!IsItemVisible(Item)) {
		pRect->bottom=pRect->top;
	} else {
		if (Item==ITEM_PROGRAMINFO)
			pRect->bottom-=m_Style.ButtonSize.Height;
		if (Item!=ITEM_PROGRAMINFO || pRect->top>=pRect->bottom)
			pRect->bottom=pRect->top+m_FontHeight;
	}
}


void CInformationPanel::CalcFontHeight()
{
	HDC hdc;

	hdc=::GetDC(m_hwnd);
	if (hdc!=NULL) {
		m_FontHeight=m_Font.GetHeight(hdc);
		::ReleaseDC(m_hwnd,hdc);
	}
}


void CInformationPanel::Draw(HDC hdc,const RECT &PaintRect)
{
	HDC hdcDst;
	RECT rc;
//	TCHAR szText[256];

	GetClientRect(&rc);
	if (rc.right>m_Offscreen.GetWidth()
			|| m_FontHeight+m_Style.LineSpacing>m_Offscreen.GetHeight())
		m_Offscreen.Create(rc.right,m_FontHeight+m_Style.LineSpacing);
	hdcDst=m_Offscreen.GetDC();
	if (hdcDst==NULL)
		hdcDst=hdc;

	HFONT hfontOld=DrawUtil::SelectObject(hdcDst,m_Font);
	COLORREF crOldTextColor=::SetTextColor(hdcDst,m_Theme.Style.Fore.Fill.GetSolidColor());
	int OldBkMode=::SetBkMode(hdcDst,TRANSPARENT);

	for (int i=0;i<ITEM_PROGRAMINFO;i++) {
		if (GetDrawItemRect(i,&rc,PaintRect))
			m_ItemList[i]->Draw(hdc,rc);
	}

	GetItemRect(ITEM_PROGRAMINFO-1,&rc);
	if (PaintRect.bottom>rc.bottom) {
		rc.left=PaintRect.left;
		rc.top=max(PaintRect.top,rc.bottom);
		rc.right=PaintRect.right;
		rc.bottom=PaintRect.bottom;
		::FillRect(hdc,&rc,m_BackBrush.GetHandle());
	}

	if (IsItemVisible(ITEM_PROGRAMINFO)) {
		if (m_Theme.ProgramInfoStyle.Back.Border.Type!=TVTest::Theme::BORDER_NONE) {
			GetItemRect(ITEM_PROGRAMINFO,&rc);
			TVTest::Theme::Draw(hdc,rc,m_Theme.ProgramInfoStyle.Back.Border);
		}

		GetButtonRect(BUTTON_PROGRAMINFOPREV,&rc);
		DrawProgramInfoPrevNextButton(
			hdc,rc,false,IsButtonEnabled(BUTTON_PROGRAMINFOPREV),
			m_HotButton==BUTTON_PROGRAMINFOPREV);
		GetButtonRect(BUTTON_PROGRAMINFONEXT,&rc);
		DrawProgramInfoPrevNextButton(
			hdc,rc,true,IsButtonEnabled(BUTTON_PROGRAMINFONEXT),
			m_HotButton==BUTTON_PROGRAMINFONEXT);
	}

	::SetBkMode(hdcDst,OldBkMode);
	::SetTextColor(hdcDst,crOldTextColor);
	::SelectObject(hdcDst,hfontOld);
}


bool CInformationPanel::GetDrawItemRect(int Item,RECT *pRect,const RECT &PaintRect) const
{
	if (!IsItemVisible(Item))
		return false;
	GetItemRect(Item,pRect);
	pRect->bottom+=m_Style.LineSpacing;
	return ::IsRectIntersect(&PaintRect,pRect)!=FALSE;
}


void CInformationPanel::DrawItem(HDC hdc,LPCTSTR pszText,const RECT &Rect)
{
	HDC hdcDst;
	RECT rcDraw;

	hdcDst=m_Offscreen.GetDC();
	if (hdcDst!=NULL) {
		::SetRect(&rcDraw,0,0,Rect.right-Rect.left,Rect.bottom-Rect.top);
	} else {
		hdcDst=hdc;
		rcDraw=Rect;
	}
	::FillRect(hdcDst,&rcDraw,m_BackBrush.GetHandle());
	if (pszText!=NULL && pszText[0]!='\0')
		::DrawText(hdcDst,pszText,-1,&rcDraw,DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
	if (hdcDst!=hdc)
		m_Offscreen.CopyTo(hdc,&Rect);
}


void CInformationPanel::DrawProgramInfoPrevNextButton(
	HDC hdc,const RECT &Rect,bool fNext,bool fEnabled,bool fHot) const
{
	const TVTest::Theme::Style &Style=
		fEnabled && fHot?m_Theme.ButtonHotStyle:m_Theme.ButtonStyle;

	TVTest::Theme::Draw(hdc,Rect,Style.Back);

	HBRUSH hbr;
	POINT Points[3];

	if (!fNext) {
		Points[0].x=Rect.right-5;
		Points[0].y=Rect.top+3;
		Points[1].x=Points[0].x;
		Points[1].y=Rect.bottom-4;
		Points[2].x=Rect.left+4;
		Points[2].y=Rect.top+(Rect.bottom-Rect.top)/2;
	} else {
		Points[0].x=Rect.left+4;
		Points[0].y=Rect.top+3;
		Points[1].x=Points[0].x;
		Points[1].y=Rect.bottom-4;
		Points[2].x=Rect.right-5;
		Points[2].y=Rect.top+(Rect.bottom-Rect.top)/2;
	}
	if (fEnabled) {
		hbr=::CreateSolidBrush(Style.Fore.Fill.GetSolidColor());
	} else {
		hbr=::CreateSolidBrush(MixColor(
			Style.Back.Fill.GetSolidColor(),
			Style.Fore.Fill.GetSolidColor()));
	}
	HGDIOBJ hOldBrush=::SelectObject(hdc,hbr);
	HGDIOBJ hOldPen=::SelectObject(hdc,::GetStockObject(NULL_PEN));
	::Polygon(hdc,Points,3);
	::SelectObject(hdc,hOldPen);
	::SelectObject(hdc,hOldBrush);
	::DeleteObject(hbr);
}


bool CInformationPanel::GetButtonRect(int Button,RECT *pRect) const
{
	RECT rc;

	GetItemRect(ITEM_PROGRAMINFO,&rc);
	pRect->left=rc.right-m_Style.ButtonSize.Width*(2-Button);
	pRect->right=pRect->left+m_Style.ButtonSize.Width;
	pRect->top=rc.bottom;
	if (!IsItemVisible(ITEM_PROGRAMINFO)) {
		pRect->bottom=rc.bottom;
		return false;
	}
	pRect->bottom=rc.bottom+m_Style.ButtonSize.Height;

	return true;
}


void CInformationPanel::RedrawButton(int Button)
{
	RECT rc;

	if (GetButtonRect(Button,&rc))
		Invalidate(&rc);
}


int CInformationPanel::ButtonHitTest(int x,int y) const
{
	POINT pt={x,y};

	for (int i=0;i<NUM_BUTTONS;i++) {
		if (IsButtonEnabled(i)) {
			RECT rc;

			if (GetButtonRect(i,&rc) && ::PtInRect(&rc,pt))
				return i;
		}
	}

	return -1;
}


void CInformationPanel::SetHotButton(int Button)
{
	if (Button!=m_HotButton) {
		if (m_HotButton>=0)
			RedrawButton(m_HotButton);
		m_HotButton=Button;
		if (m_HotButton>=0)
			RedrawButton(m_HotButton);
	}
}


bool CInformationPanel::IsButtonEnabled(int Button) const
{
	const CProgramInfoItem *pProgramInfoItem=
		static_cast<const CProgramInfoItem*>(GetItem(ITEM_PROGRAMINFO));

	switch (Button) {
	case BUTTON_PROGRAMINFOPREV:
		return pProgramInfoItem->IsNext();

	case BUTTON_PROGRAMINFONEXT:
		return !pProgramInfoItem->IsNext();
	}

	return false;
}


bool CInformationPanel::ReadSettings(CSettings &Settings)
{
	for (int i=0;i<NUM_ITEMS;i++) {
		CItem *pItem=m_ItemList[i];
		bool f;

		if (Settings.Read(pItem->GetName(),&f))
			pItem->SetVisible(f);
	}
	return true;
}


bool CInformationPanel::WriteSettings(CSettings &Settings)
{
	for (int i=0;i<NUM_ITEMS;i++) {
		const CItem *pItem=m_ItemList[i];
		Settings.Write(pItem->GetName(),pItem->IsVisible());
	}
	return true;
}




CInformationPanel::InformationPanelStyle::InformationPanelStyle()
	: ButtonSize(16,16)
	, LineSpacing(1)
{
}


void CInformationPanel::InformationPanelStyle::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	pStyleManager->Get(TEXT("info-panel.button"),&ButtonSize);
	pStyleManager->Get(TEXT("info-panel.line-spacing"),&LineSpacing);
}


void CInformationPanel::InformationPanelStyle::NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	pStyleManager->ToPixels(&ButtonSize);
	pStyleManager->ToPixels(&LineSpacing);
}




CInformationPanel::CItem::CItem(CInformationPanel *pPanel,bool fVisible)
	: m_pPanel(pPanel)
	, m_fVisible(fVisible)
{
}


void CInformationPanel::CItem::DrawItem(HDC hdc,const RECT &Rect,LPCTSTR pszText)
{
	m_pPanel->DrawItem(hdc,pszText,Rect);
}


void CInformationPanel::CItem::UpdateItem()
{
	m_pPanel->UpdateItem(GetID());
}


void CInformationPanel::CItem::Redraw()
{
	m_pPanel->RedrawItem(GetID());
}




CInformationPanel::CVideoInfoItem::CVideoInfoItem(CInformationPanel *pPanel,bool fVisible)
	: CItemTemplate(pPanel,fVisible)
{
	Reset();
}


void CInformationPanel::CVideoInfoItem::Reset()
{
	m_OriginalVideoWidth=0;
	m_OriginalVideoHeight=0;
	m_DisplayVideoWidth=0;
	m_DisplayVideoHeight=0;
	m_AspectX=0;
	m_AspectY=0;
}


bool CInformationPanel::CVideoInfoItem::Update()
{
	const CCoreEngine &CoreEngine=GetAppClass().CoreEngine;
	int OriginalVideoWidth,OriginalVideoHeight;
	int DisplayVideoWidth,DisplayVideoHeight;
	BYTE AspectX,AspectY;

	OriginalVideoWidth=CoreEngine.GetOriginalVideoWidth();
	OriginalVideoHeight=CoreEngine.GetOriginalVideoHeight();
	DisplayVideoWidth=CoreEngine.GetDisplayVideoWidth();
	DisplayVideoHeight=CoreEngine.GetDisplayVideoHeight();
	if (!CoreEngine.m_DtvEngine.m_MediaViewer.GetEffectiveAspectRatio(&AspectX,&AspectY))
		AspectX=AspectY=0;

	if (OriginalVideoWidth==m_OriginalVideoWidth
			&& OriginalVideoHeight==m_OriginalVideoHeight
			&& DisplayVideoWidth==m_DisplayVideoWidth
			&& DisplayVideoHeight==m_DisplayVideoHeight
			&& AspectX==m_AspectX
			&& AspectY==m_AspectY)
		return false;

	m_OriginalVideoWidth=OriginalVideoWidth;
	m_OriginalVideoHeight=OriginalVideoHeight;
	m_DisplayVideoWidth=DisplayVideoWidth;
	m_DisplayVideoHeight=DisplayVideoHeight;
	m_AspectX=AspectX;
	m_AspectY=AspectY;

	return true;
}


void CInformationPanel::CVideoInfoItem::Draw(HDC hdc,const RECT &Rect)
{
	TCHAR szText[256];

	StdUtil::snprintf(szText,lengthof(szText),
					  TEXT("%d x %d [%d x %d (%d:%d)]"),
					  m_OriginalVideoWidth,m_OriginalVideoHeight,
					  m_DisplayVideoWidth,m_DisplayVideoHeight,
					  m_AspectX,m_AspectY);
	DrawItem(hdc,Rect,szText);
}




CInformationPanel::CVideoDecoderItem::CVideoDecoderItem(CInformationPanel *pPanel,bool fVisible)
	: CItemTemplate(pPanel,fVisible)
{
}


void CInformationPanel::CVideoDecoderItem::Reset()
{
	m_VideoDecoderName.clear();
}


bool CInformationPanel::CVideoDecoderItem::Update()
{
	TCHAR szDecoder[256];

	if (GetAppClass().CoreEngine.m_DtvEngine.m_MediaViewer.GetVideoDecoderName(
			szDecoder,lengthof(szDecoder))) {
		if (m_VideoDecoderName.compare(szDecoder)==0)
			return false;
		m_VideoDecoderName=szDecoder;
	} else {
		if (m_VideoDecoderName.empty())
			return false;
		m_VideoDecoderName.clear();
	}

	return true;
}


void CInformationPanel::CVideoDecoderItem::Draw(HDC hdc,const RECT &Rect)
{
	DrawItem(hdc,Rect,m_VideoDecoderName.c_str());
}




CInformationPanel::CVideoRendererItem::CVideoRendererItem(CInformationPanel *pPanel,bool fVisible)
	: CItemTemplate(pPanel,fVisible)
{
}


void CInformationPanel::CVideoRendererItem::Reset()
{
	m_VideoRendererName.clear();
}


bool CInformationPanel::CVideoRendererItem::Update()
{
	TCHAR szRenderer[256];

	if (GetAppClass().CoreEngine.m_DtvEngine.m_MediaViewer.GetVideoRendererName(
			szRenderer,lengthof(szRenderer))) {
		if (m_VideoRendererName.compare(szRenderer)==0)
			return false;
		m_VideoRendererName=szRenderer;
	} else {
		if (m_VideoRendererName.empty())
			return false;
		m_VideoRendererName.clear();
	}

	return true;
}


void CInformationPanel::CVideoRendererItem::Draw(HDC hdc,const RECT &Rect)
{
	DrawItem(hdc,Rect,m_VideoRendererName.c_str());
}




CInformationPanel::CAudioDeviceItem::CAudioDeviceItem(CInformationPanel *pPanel,bool fVisible)
	: CItemTemplate(pPanel,fVisible)
{
}


void CInformationPanel::CAudioDeviceItem::Reset()
{
	m_AudioDeviceName.clear();
}


bool CInformationPanel::CAudioDeviceItem::Update()
{
	TCHAR szDevice[256];

	if (GetAppClass().CoreEngine.m_DtvEngine.m_MediaViewer.GetAudioRendererName(
			szDevice,lengthof(szDevice))) {
		if (m_AudioDeviceName.compare(szDevice)==0)
			return false;
		m_AudioDeviceName=szDevice;
	} else {
		if (m_AudioDeviceName.empty())
			return false;
		m_AudioDeviceName.clear();
	}

	return true;
}


void CInformationPanel::CAudioDeviceItem::Draw(HDC hdc,const RECT &Rect)
{
	DrawItem(hdc,Rect,m_AudioDeviceName.c_str());
}




CInformationPanel::CSignalLevelItem::CSignalLevelItem(CInformationPanel *pPanel,bool fVisible)
	: CItemTemplate(pPanel,fVisible)
	, m_fShowSignalLevel(true)
{
	Reset();
}


void CInformationPanel::CSignalLevelItem::Reset()
{
	m_SignalLevel=0.0f;
	m_BitRate=0;
}


bool CInformationPanel::CSignalLevelItem::Update()
{
	const CCoreEngine &CoreEngine=GetAppClass().CoreEngine;
	const float SignalLevel=
		m_fShowSignalLevel ? CoreEngine.GetSignalLevel() : 0.0f;
	const DWORD BitRate=CoreEngine.GetBitRate();

	if (SignalLevel==m_SignalLevel && BitRate==m_BitRate)
		return false;

	m_SignalLevel=SignalLevel;
	m_BitRate=BitRate;

	return true;
}


void CInformationPanel::CSignalLevelItem::Draw(HDC hdc,const RECT &Rect)
{
	const CCoreEngine &CoreEngine=GetAppClass().CoreEngine;
	TCHAR szText[64];
	int Length=0;

	if (m_fShowSignalLevel) {
		TCHAR szSignalLevel[32];
		CoreEngine.GetSignalLevelText(m_SignalLevel,szSignalLevel,lengthof(szSignalLevel));
		Length=StdUtil::snprintf(szText,lengthof(szText),TEXT("%s / "),szSignalLevel);
	}
	CoreEngine.GetBitRateText(m_BitRate,szText+Length,lengthof(szText)-Length);

	DrawItem(hdc,Rect,szText);
}


void CInformationPanel::CSignalLevelItem::ShowSignalLevel(bool fShow)
{
	if (m_fShowSignalLevel!=fShow) {
		m_fShowSignalLevel=fShow;
		Update();
		Redraw();
	}
}




CInformationPanel::CMediaBitRateItem::CMediaBitRateItem(CInformationPanel *pPanel,bool fVisible)
	: CItemTemplate(pPanel,fVisible)
{
	Reset();
}


void CInformationPanel::CMediaBitRateItem::Reset()
{
	m_VideoBitRate=0;
	m_AudioBitRate=0;
}


bool CInformationPanel::CMediaBitRateItem::Update()
{
	const CMediaViewer &MediaViewer=GetAppClass().CoreEngine.m_DtvEngine.m_MediaViewer;
	const DWORD VideoBitRate=MediaViewer.GetVideoBitRate();
	const DWORD AudioBitRate=MediaViewer.GetAudioBitRate();

	if (VideoBitRate==m_VideoBitRate && AudioBitRate==m_AudioBitRate)
		return false;

	m_VideoBitRate=VideoBitRate;
	m_AudioBitRate=AudioBitRate;

	return true;
}


void CInformationPanel::CMediaBitRateItem::Draw(HDC hdc,const RECT &Rect)
{
	TCHAR szText[64];
	int Length;

	if (m_VideoBitRate<1000*1000) {
		Length=StdUtil::snprintf(szText,lengthof(szText),
								 TEXT("映像 %u kbps"),
								 (m_VideoBitRate+500)/1000);
	} else {
		Length=StdUtil::snprintf(szText,lengthof(szText),
								 TEXT("映像 %.2f Mbps"),
								 (double)(m_VideoBitRate)/(double)(1000*1000));
	}
	StdUtil::snprintf(szText+Length,lengthof(szText)-Length,
					  TEXT(" / 音声 %u kbps"),
					  (m_AudioBitRate+500)/1000);

	DrawItem(hdc,Rect,szText);
}




CInformationPanel::CErrorItem::CErrorItem(CInformationPanel *pPanel,bool fVisible)
	: CItemTemplate(pPanel,fVisible)
{
	Reset();
}


void CInformationPanel::CErrorItem::Reset()
{
	m_fShowScramble=true;
	m_ContinuityErrorPacketCount=0;
	m_ErrorPacketCount=0;
	m_ScramblePacketCount=0;
}


bool CInformationPanel::CErrorItem::Update()
{
	const CCoreEngine &CoreEngine=GetAppClass().CoreEngine;
	const ULONGLONG ContinuityErrorPacketCount=CoreEngine.GetContinuityErrorPacketCount();
	const ULONGLONG ErrorPacketCount=CoreEngine.GetErrorPacketCount();
	const ULONGLONG ScramblePacketCount=CoreEngine.GetScramblePacketCount();

	if (ContinuityErrorPacketCount==m_ContinuityErrorPacketCount
			&& ErrorPacketCount==m_ErrorPacketCount
			&& ScramblePacketCount==m_ScramblePacketCount)
		return false;

	m_ContinuityErrorPacketCount=ContinuityErrorPacketCount;
	m_ErrorPacketCount=ErrorPacketCount;
	m_ScramblePacketCount=ScramblePacketCount;

	return true;
}


void CInformationPanel::CErrorItem::Draw(HDC hdc,const RECT &Rect)
{
	TCHAR szText[256];
	int Length;

	Length=StdUtil::snprintf(szText,lengthof(szText),
							 TEXT("D %llu / E %llu"),
							 m_ContinuityErrorPacketCount,
							 m_ErrorPacketCount);
	if (m_fShowScramble) {
		StdUtil::snprintf(szText+Length,lengthof(szText)-Length,
						  TEXT(" / S %llu"),m_ScramblePacketCount);
	}
	DrawItem(hdc,Rect,szText);
}




CInformationPanel::CRecordItem::CRecordItem(CInformationPanel *pPanel,bool fVisible)
	: CItemTemplate(pPanel,fVisible)
{
	Reset();
}


void CInformationPanel::CRecordItem::Reset()
{
	m_fRecording=false;
}


bool CInformationPanel::CRecordItem::Update()
{
	CAppMain &App=GetAppClass();
	bool fRecording=App.RecordManager.IsRecording();
	LONGLONG WroteSize;
	CRecordTask::DurationType RecordTime;
	LONGLONG DiskFreeSpace;

	if (fRecording) {
		const CRecordTask *pRecordTask=App.RecordManager.GetRecordTask();
		WroteSize=pRecordTask->GetWroteSize();
		RecordTime=pRecordTask->GetRecordTime();
		DiskFreeSpace=pRecordTask->GetFreeSpace();
	}

	if (fRecording==m_fRecording
			&& (!fRecording
				|| (WroteSize==m_WroteSize
					&& RecordTime==m_RecordTime
					&& DiskFreeSpace==m_DiskFreeSpace)))
		return false;

	m_fRecording=fRecording;
	if (fRecording) {
		m_WroteSize=WroteSize;
		m_RecordTime=RecordTime;
		m_DiskFreeSpace=DiskFreeSpace;
	}

	return true;
}


void CInformationPanel::CRecordItem::Draw(HDC hdc,const RECT &Rect)
{
	if (m_fRecording) {
		TCHAR szText[256];
		int Length;

		unsigned int RecordSec=(unsigned int)(m_RecordTime/1000);
		Length=StdUtil::snprintf(szText,lengthof(szText),
			TEXT("● %d:%02d:%02d"),
			RecordSec/(60*60),(RecordSec/60)%60,RecordSec%60);
		if (m_WroteSize>=0) {
			unsigned int Size=
				(unsigned int)(m_WroteSize/(ULONGLONG)(1024*1024/100));
			Length+=StdUtil::snprintf(szText+Length,lengthof(szText)-Length,
									  TEXT(" / %d.%02d MB"),
									  Size/100,Size%100);
		}
		if (m_DiskFreeSpace>=0) {
			unsigned int FreeSpace=
				(unsigned int)(m_DiskFreeSpace/(ULONGLONG)(1024*1024*1024/100));
			StdUtil::snprintf(szText+Length,lengthof(szText)-Length,
							  TEXT(" / %d.%02d GB空き"),
							  FreeSpace/100,FreeSpace%100);
		}
		DrawItem(hdc,Rect,szText);
	} else {
		DrawItem(hdc,Rect,TEXT("■ <録画していません>"));
	}
}




CInformationPanel::CServiceItem::CServiceItem(CInformationPanel *pPanel,bool fVisible)
	: CItemTemplate(pPanel,fVisible)
{
}


void CInformationPanel::CServiceItem::Reset()
{
	m_ServiceName.clear();
}


bool CInformationPanel::CServiceItem::Update()
{
	const CCoreEngine &CoreEngine=GetAppClass().CoreEngine;
	TCHAR szService[256];

	if (CoreEngine.m_DtvEngine.m_TsAnalyzer.GetServiceName(
			CoreEngine.m_DtvEngine.GetServiceIndex(),
			szService,lengthof(szService))>0) {
		if (m_ServiceName.compare(szService)==0)
			return false;
		m_ServiceName=szService;
	} else {
		if (m_ServiceName.empty())
			return false;
		m_ServiceName.clear();
	}

	return true;
}


void CInformationPanel::CServiceItem::Draw(HDC hdc,const RECT &Rect)
{
	DrawItem(hdc,Rect,m_ServiceName.c_str());
}




CInformationPanel::CProgramInfoItem::CProgramInfoItem(CInformationPanel *pPanel,bool fVisible)
	: CItemTemplate(pPanel,fVisible)
{
	Reset();
}


void CInformationPanel::CProgramInfoItem::Reset()
{
	m_fNext=false;
	m_InfoText.clear();
}


bool CInformationPanel::CProgramInfoItem::Update()
{
	CCoreEngine &CoreEngine=GetAppClass().CoreEngine;
	TCHAR szText[4096],szTemp[256];
	CStaticStringFormatter Formatter(szText,lengthof(szText));

	if (m_fNext)
		Formatter.Append(TEXT("次 : "));

	CEventInfoData EventInfo;

	if (CoreEngine.GetCurrentEventInfo(&EventInfo,0xFFFF,m_fNext)) {
		if (EpgUtil::FormatEventTime(&EventInfo,szTemp,lengthof(szTemp),
									 EpgUtil::EVENT_TIME_DATE |
									 EpgUtil::EVENT_TIME_YEAR |
									 EpgUtil::EVENT_TIME_UNDECIDED_TEXT)>0) {
			Formatter.Append(szTemp);
			Formatter.Append(TEXT("\r\n"));
		}
		if (!EventInfo.m_EventName.empty()) {
			Formatter.Append(EventInfo.m_EventName.c_str());
			Formatter.Append(TEXT("\r\n\r\n"));
		}
		if (!EventInfo.m_EventText.empty()) {
			Formatter.Append(EventInfo.m_EventText.c_str());
			Formatter.Append(TEXT("\r\n\r\n"));
		}
		if (!EventInfo.m_EventExtendedText.empty()) {
			Formatter.Append(EventInfo.m_EventExtendedText.c_str());
		}
	}

	CTsAnalyzer::EventSeriesInfo SeriesInfo;
	if (CoreEngine.m_DtvEngine.GetEventSeriesInfo(&SeriesInfo,m_fNext)
			&& SeriesInfo.EpisodeNumber!=0 && SeriesInfo.LastEpisodeNumber!=0) {
		Formatter.Append(TEXT("\r\n\r\n(シリーズ"));
		if (SeriesInfo.RepeatLabel!=0)
			Formatter.Append(TEXT(" [再]"));
		if (SeriesInfo.EpisodeNumber!=0 && SeriesInfo.LastEpisodeNumber!=0)
			Formatter.AppendFormat(TEXT(" 第%d回 / 全%d回"),
								   SeriesInfo.EpisodeNumber,SeriesInfo.LastEpisodeNumber);
		// expire_date は実際の最終回の日時でないので、紛らわしいため表示しない
		/*
		if (SeriesInfo.bIsExpireDateValid)
			Formatter.AppendFormat(TEXT(" 終了予定%d/%d/%d"),
								   SeriesInfo.ExpireDate.wYear,
								   SeriesInfo.ExpireDate.wMonth,
								   SeriesInfo.ExpireDate.wDay);
		*/
		Formatter.Append(TEXT(")"));
	}

	if (m_InfoText.compare(Formatter.GetString())==0)
		return false;

	m_InfoText=Formatter.GetString();

	m_pPanel->UpdateProgramInfoText();

	return true;
}


void CInformationPanel::CProgramInfoItem::SetNext(bool fNext)
{
	if (m_fNext!=fNext) {
		m_fNext=fNext;
		Update();
	}
}
