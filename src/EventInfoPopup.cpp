#include "stdafx.h"
#include "TVTest.h"
#include "EventInfoPopup.h"
#include "EpgUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




const LPCTSTR CEventInfoPopup::m_pszWindowClass=APP_NAME TEXT(" Event Info");
HINSTANCE CEventInfoPopup::m_hinst=NULL;


bool CEventInfoPopup::Initialize(HINSTANCE hinst)
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
		wc.lpszClassName=m_pszWindowClass;
		if (RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CEventInfoPopup::CEventInfoPopup()
	: m_hwndEdit(NULL)
	, m_BackColor(::GetSysColor(COLOR_WINDOW))
	, m_TextColor(::GetSysColor(COLOR_WINDOWTEXT))
	, m_TitleBackGradient(Theme::GRADIENT_NORMAL,Theme::DIRECTION_VERT,
						  RGB(255,255,255),RGB(228,228,240))
	, m_TitleTextColor(RGB(80,80,80))
	, m_TitleLineMargin(1)
	, m_TitleHeight(0)
	, m_ButtonSize(14)
	, m_ButtonMargin(3)
	, m_pEventHandler(NULL)
	, m_fDetailInfo(
#ifdef _DEBUG
		true
#else
		false
#endif
		)
{
	m_WindowPosition.Width=320;
	m_WindowPosition.Height=320;

	LOGFONT lf;
	DrawUtil::GetSystemFont(DrawUtil::FONT_MESSAGE,&lf);
	m_Font.Create(&lf);
	lf.lfWeight=FW_BOLD;
	m_TitleFont.Create(&lf);
}


CEventInfoPopup::~CEventInfoPopup()
{
	Destroy();
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pPopup=NULL;
}


bool CEventInfoPopup::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,m_pszWindowClass,NULL,m_hinst);
}


void CEventInfoPopup::SetEventInfo(const CEventInfoData *pEventInfo)
{
	if (m_EventInfo==*pEventInfo)
		return;

	m_EventInfo=*pEventInfo;

	TCHAR szText[4096];
	CStaticStringFormatter Formatter(szText,lengthof(szText));

	if (!IsStringEmpty(m_EventInfo.GetEventText())) {
		Formatter.Append(m_EventInfo.GetEventText());
		Formatter.RemoveTrailingWhitespace();
	}
	if (!IsStringEmpty(m_EventInfo.GetEventExtText())) {
		if (!Formatter.IsEmpty())
			Formatter.Append(TEXT("\r\n\r\n"));
		Formatter.Append(m_EventInfo.GetEventExtText());
		Formatter.RemoveTrailingWhitespace();
	}

	Formatter.Append(TEXT("\r\n"));

	LPCTSTR pszVideo=EpgUtil::GetVideoComponentTypeText(m_EventInfo.m_VideoInfo.ComponentType);
	if (pszVideo!=NULL) {
		Formatter.AppendFormat(TEXT("\r\n■ 映像： %s"),pszVideo);
	}

	if (!m_EventInfo.m_AudioList.empty()) {
		const CEventInfoData::AudioInfo *pMainAudioInfo=m_EventInfo.GetMainAudioInfo();
		TCHAR szBuff[64];

		Formatter.Append(TEXT("\r\n■ 音声： "));
		if (m_EventInfo.m_AudioList.size()==1) {
			FormatAudioInfo(pMainAudioInfo,szBuff,lengthof(szBuff));
			Formatter.Append(szBuff);
		} else {
			Formatter.Append(TEXT("主: "));
			FormatAudioInfo(pMainAudioInfo,szBuff,lengthof(szBuff));
			Formatter.Append(szBuff);
			for (size_t i=0;i<m_EventInfo.m_AudioList.size();i++) {
				const CEventInfoData::AudioInfo *pAudioInfo=&m_EventInfo.m_AudioList[i];
				if (pAudioInfo!=pMainAudioInfo) {
					Formatter.Append(TEXT(" / 副: "));
					FormatAudioInfo(pAudioInfo,szBuff,lengthof(szBuff));
					Formatter.Append(szBuff);
				}
			}
		}
	}

	for (int i=0;i<m_EventInfo.m_ContentNibble.NibbleCount;i++) {
		if (m_EventInfo.m_ContentNibble.NibbleList[i].ContentNibbleLevel1!=0xE) {
			CEpgGenre EpgGenre;
			LPCTSTR pszGenre=EpgGenre.GetText(m_EventInfo.m_ContentNibble.NibbleList[i].ContentNibbleLevel1,-1);
			if (pszGenre!=NULL) {
				Formatter.AppendFormat(TEXT("\r\n■ ジャンル： %s"),pszGenre);
				pszGenre=EpgGenre.GetText(
					m_EventInfo.m_ContentNibble.NibbleList[i].ContentNibbleLevel1,
					m_EventInfo.m_ContentNibble.NibbleList[i].ContentNibbleLevel2);
				if (pszGenre!=NULL)
					Formatter.AppendFormat(TEXT(" - %s"),pszGenre);
			}
			break;
		}
	}

	if (m_fDetailInfo) {
		Formatter.AppendFormat(TEXT("\r\n■ イベントID： 0x%04X"),m_EventInfo.m_EventID);
		if (m_EventInfo.m_fCommonEvent)
			Formatter.AppendFormat(TEXT(" (イベント共有 サービスID 0x%04X / イベントID 0x%04X)"),
								   m_EventInfo.m_CommonEventInfo.ServiceID,
								   m_EventInfo.m_CommonEventInfo.EventID);
	}

	LOGFONT lf;
	CHARFORMAT cf;
	HDC hdc=::GetDC(m_hwndEdit);
	m_Font.GetLogFont(&lf);
	CRichEditUtil::LogFontToCharFormat(hdc,&lf,&cf);
	cf.dwMask|=CFM_COLOR;
	cf.crTextColor=m_TextColor;
	::ReleaseDC(m_hwndEdit,hdc);
	::SendMessage(m_hwndEdit,WM_SETREDRAW,FALSE,0);
	::SetWindowText(m_hwndEdit,NULL);
	CRichEditUtil::AppendText(m_hwndEdit,
		SkipLeadingWhitespace(Formatter.GetString()),&cf);
	CRichEditUtil::DetectURL(m_hwndEdit,&cf);
	POINT pt={0,0};
	::SendMessage(m_hwndEdit,EM_SETSCROLLPOS,0,reinterpret_cast<LPARAM>(&pt));
	::SendMessage(m_hwndEdit,WM_SETREDRAW,TRUE,0);
	::InvalidateRect(m_hwndEdit,NULL,TRUE);

	CalcTitleHeight();
	RECT rc;
	GetClientRect(&rc);
	::MoveWindow(m_hwndEdit,0,m_TitleHeight,rc.right,max(rc.bottom-m_TitleHeight,0),TRUE);
	Invalidate();
}


void CEventInfoPopup::FormatAudioInfo(
	const CEventInfoData::AudioInfo *pAudioInfo,LPTSTR pszText,int MaxLength) const
{
	LPCTSTR pszAudio;
	bool fBilingual=false;

	if (pAudioInfo->ComponentType==0x02
			&& pAudioInfo->bESMultiLingualFlag
			&& pAudioInfo->LanguageCode!=pAudioInfo->LanguageCode2) {
		pszAudio=TEXT("Mono 二カ国語");
		fBilingual=true;
	} else {
		pszAudio=EpgUtil::GetAudioComponentTypeText(pAudioInfo->ComponentType);
	}

	LPCTSTR p=pAudioInfo->szText;
	TCHAR szAudioComponent[64];
	szAudioComponent[0]=_T('\0');
	if (*p!=_T('\0')) {
		szAudioComponent[0]=_T(' ');
		szAudioComponent[1]=_T('[');
		size_t i;
		for (i=2;*p!=_T('\0') && i<lengthof(szAudioComponent)-2;i++) {
			if (*p==_T('\r') || *p==_T('\n')) {
				szAudioComponent[i]=_T('/');
				p++;
				if (*p==_T('\n'))
					p++;
			} else {
				szAudioComponent[i]=*p++;
			}
		}
		szAudioComponent[i+0]=_T(']');
		szAudioComponent[i+1]=_T('\0');
	} else if (fBilingual) {
		StdUtil::snprintf(szAudioComponent,lengthof(szAudioComponent),
						  TEXT(" [%s/%s]"),
						  EpgUtil::GetLanguageText(pAudioInfo->LanguageCode),
						  EpgUtil::GetLanguageText(pAudioInfo->LanguageCode2));
	}

	StdUtil::snprintf(pszText,MaxLength,TEXT("%s%s"),
					  pszAudio!=NULL?pszAudio:TEXT("?"),
					  szAudioComponent);
}


void CEventInfoPopup::CalcTitleHeight()
{
	TEXTMETRIC tm;
	RECT rc;

	HDC hdc=::GetDC(m_hwnd);
	if (hdc==NULL)
		return;
	HFONT hfontOld=DrawUtil::SelectObject(hdc,m_TitleFont);
	::GetTextMetrics(hdc,&tm);
	//FontHeight=tm.tmHeight-tm.tmInternalLeading;
	int FontHeight=tm.tmHeight;
	m_TitleLineHeight=FontHeight+m_TitleLineMargin;
	GetClientRect(&rc);
	rc.right-=m_ButtonSize+m_ButtonMargin*2;
	m_TitleHeight=(DrawUtil::CalcWrapTextLines(hdc,m_EventInfo.GetEventName(),rc.right)+1)*m_TitleLineHeight;
	::SelectObject(hdc,hfontOld);
	::ReleaseDC(m_hwnd,hdc);
}


bool CEventInfoPopup::Show(const CEventInfoData *pEventInfo,const RECT *pPos)
{
	if (pEventInfo==NULL)
		return false;
	bool fExists=m_hwnd!=NULL;
	if (!fExists) {
		if (!Create(NULL,WS_POPUP | WS_CLIPCHILDREN | WS_THICKFRAME,WS_EX_TOPMOST | WS_EX_NOACTIVATE,0))
			return false;
	}
	if (pPos!=NULL) {
		if (!GetVisible())
			SetPosition(pPos);
	} else if (!IsVisible() || m_EventInfo!=*pEventInfo) {
		RECT rc;
		POINT pt;
		int Width,Height;

		GetPosition(&rc);
		Width=rc.right-rc.left;
		Height=rc.bottom-rc.top;
		::GetCursorPos(&pt);
		pt.y+=16;
		HMONITOR hMonitor=::MonitorFromPoint(pt,MONITOR_DEFAULTTONEAREST);
		if (hMonitor!=NULL) {
			MONITORINFO mi;

			mi.cbSize=sizeof(mi);
			if (::GetMonitorInfo(hMonitor,&mi)) {
				if (pt.x+Width>mi.rcMonitor.right)
					pt.x=mi.rcMonitor.right-Width;
				if (pt.y+Height>mi.rcMonitor.bottom) {
					pt.y=mi.rcMonitor.bottom-Height;
					if (pt.x+Width<mi.rcMonitor.right)
						pt.x+=min(16,mi.rcMonitor.right-(pt.x+Width));
				}
			}
		}
		::SetWindowPos(m_hwnd,HWND_TOPMOST,pt.x,pt.y,Width,Height,
					   SWP_NOACTIVATE);
	}
	SetEventInfo(pEventInfo);
	::ShowWindow(m_hwnd,SW_SHOWNA);
	return true;
}


bool CEventInfoPopup::Hide()
{
	if (m_hwnd!=NULL)
		::ShowWindow(m_hwnd,SW_HIDE);
	return true;
}


bool CEventInfoPopup::IsVisible()
{
	return m_hwnd!=NULL && GetVisible();
}


bool CEventInfoPopup::IsOwnWindow(HWND hwnd) const
{
	if (hwnd==NULL)
		return false;
	return hwnd==m_hwnd || hwnd==m_hwndEdit;
}


void CEventInfoPopup::GetSize(int *pWidth,int *pHeight)
{
	RECT rc;

	GetPosition(&rc);
	if (pWidth!=NULL)
		*pWidth=rc.right-rc.left;
	if (pHeight!=NULL)
		*pHeight=rc.bottom-rc.top;
}


bool CEventInfoPopup::SetSize(int Width,int Height)
{
	if (Width<0 || Height<0)
		return false;
	RECT rc;
	GetPosition(&rc);
	rc.right=rc.left+Width;
	rc.bottom=rc.top+Height;
	return SetPosition(&rc);
}


void CEventInfoPopup::SetColor(COLORREF BackColor,COLORREF TextColor)
{
	m_BackColor=BackColor;
	m_TextColor=TextColor;
	if (m_hwnd!=NULL) {
		::SendMessage(m_hwndEdit,EM_SETBKGNDCOLOR,0,m_BackColor);
		//::InvalidateRect(m_hwndEdit,NULL,TRUE);
	}
}


void CEventInfoPopup::SetTitleColor(Theme::GradientInfo *pBackGradient,COLORREF TextColor)
{
	m_TitleBackGradient=*pBackGradient;
	m_TitleTextColor=TextColor;
	if (m_hwnd!=NULL) {
		RECT rc;

		GetClientRect(&rc);
		rc.bottom=m_TitleHeight;
		::InvalidateRect(m_hwnd,&rc,TRUE);
	}
}


bool CEventInfoPopup::SetFont(const LOGFONT *pFont)
{
	LOGFONT lf=*pFont;

	m_Font.Create(&lf);
	lf.lfWeight=FW_BOLD;
	m_TitleFont.Create(&lf);
	if (m_hwnd!=NULL) {
		CalcTitleHeight();
		RECT rc;
		GetClientRect(&rc);
		::MoveWindow(m_hwndEdit,0,m_TitleHeight,rc.right,max(rc.bottom-m_TitleHeight,0),TRUE);
		Invalidate();

		SetWindowFont(m_hwndEdit,m_Font.GetHandle(),TRUE);
	}
	return true;
}


void CEventInfoPopup::SetEventHandler(CEventHandler *pEventHandler)
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pPopup=NULL;
	if (pEventHandler!=NULL)
		pEventHandler->m_pPopup=this;
	m_pEventHandler=pEventHandler;
}


bool CEventInfoPopup::IsSelected() const
{
	return CRichEditUtil::IsSelected(m_hwndEdit);
}


LPTSTR CEventInfoPopup::GetSelectedText() const
{
	return CRichEditUtil::GetSelectedText(m_hwndEdit);
}


LRESULT CEventInfoPopup::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		m_RichEditUtil.LoadRichEditLib();
		m_hwndEdit=::CreateWindowEx(0,m_RichEditUtil.GetWindowClassName(),TEXT(""),
			WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_NOHIDESEL,
			0,0,0,0,hwnd,(HMENU)1,m_hinst,NULL);
		SetWindowFont(m_hwndEdit,m_Font.GetHandle(),FALSE);
		::SendMessage(m_hwndEdit,EM_SETEVENTMASK,0,ENM_MOUSEEVENTS | ENM_LINK);
		::SendMessage(m_hwndEdit,EM_SETBKGNDCOLOR,0,m_BackColor);
		return 0;

	case WM_SIZE:
		CalcTitleHeight();
		::MoveWindow(m_hwndEdit,0,m_TitleHeight,
					 LOWORD(lParam),max(HIWORD(lParam)-m_TitleHeight,0),TRUE);
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rc;
			HFONT hfontOld;
			int OldBkMode;
			COLORREF OldTextColor;

			::BeginPaint(hwnd,&ps);
			::GetClientRect(hwnd,&rc);
			rc.bottom=m_TitleHeight;
			Theme::FillGradient(ps.hdc,&rc,&m_TitleBackGradient);
			hfontOld=DrawUtil::SelectObject(ps.hdc,m_TitleFont);
			OldBkMode=::SetBkMode(ps.hdc,TRANSPARENT);
			OldTextColor=::SetTextColor(ps.hdc,m_TitleTextColor);
			TCHAR szText[EpgUtil::MAX_EVENT_TIME_LENGTH];
			int Length=EpgUtil::FormatEventTime(&m_EventInfo,szText,lengthof(szText),
												EpgUtil::EVENT_TIME_DATE | EpgUtil::EVENT_TIME_YEAR);
			if (Length>0)
				::TextOut(ps.hdc,0,0,szText,Length);
			rc.top+=m_TitleLineHeight;
			rc.right-=m_ButtonSize+m_ButtonMargin*2;
			DrawUtil::DrawWrapText(ps.hdc,m_EventInfo.GetEventName(),&rc,m_TitleLineHeight);
			::SelectObject(ps.hdc,hfontOld);
			::SetBkMode(ps.hdc,OldBkMode);
			::SetTextColor(ps.hdc,OldTextColor);
			GetCloseButtonRect(&rc);
			::DrawFrameControl(ps.hdc,&rc,DFC_CAPTION,DFCS_CAPTIONCLOSE | DFCS_MONO);
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_ACTIVATE:
		if (LOWORD(wParam)==WA_INACTIVE) {
			Hide();
		}
		return 0;

	case WM_ACTIVATEAPP:
		if (wParam==0) {
			Hide();
		}
		return 0;

	case WM_NCHITTEST:
		{
			POINT pt;
			RECT rc;

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			::ScreenToClient(hwnd,&pt);
			GetCloseButtonRect(&rc);
			if (::PtInRect(&rc,pt))
				return HTCLOSE;
			::GetClientRect(hwnd,&rc);
			rc.bottom=m_TitleHeight;
			if (::PtInRect(&rc,pt))
				return HTCAPTION;
		}
		break;

	case WM_NCLBUTTONDOWN:
		if (wParam==HTCLOSE) {
			::SendMessage(hwnd,WM_CLOSE,0,0);
			return 0;
		}
		break;

	case WM_NCRBUTTONDOWN:
		if (wParam==HTCAPTION) {
			POINT pt={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
			HMENU hmenu=::CreatePopupMenu();

			::AppendMenu(hmenu,MF_STRING | MF_ENABLED,1,TEXT("番組名をコピー(&C)"));
			int Command=::TrackPopupMenu(hmenu,TPM_RIGHTBUTTON | TPM_RETURNCMD,pt.x,pt.y,0,hwnd,NULL);
			::DestroyMenu(hmenu);
			switch (Command) {
			case 1:
				CopyTextToClipboard(hwnd,m_EventInfo.GetEventName());
				break;
			}
			return 0;
		}
		break;

	case WM_MOUSEWHEEL:
		return ::SendMessage(m_hwndEdit,uMsg,wParam,lParam);

	case WM_NCMOUSEMOVE:
		{
			TRACKMOUSEEVENT tme;

			tme.cbSize=sizeof(TRACKMOUSEEVENT);
			tme.dwFlags=TME_LEAVE | TME_NONCLIENT;
			tme.hwndTrack=hwnd;
			::TrackMouseEvent(&tme);
		}
		return 0;

	case WM_NCMOUSELEAVE:
		{
			POINT pt;
			RECT rc;

			::GetCursorPos(&pt);
			::GetWindowRect(hwnd,&rc);
			if (!::PtInRect(&rc,pt))
				Hide();
		}
		return 0;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case EN_MSGFILTER:
			if (reinterpret_cast<MSGFILTER*>(lParam)->msg==WM_RBUTTONDOWN) {
				HMENU hmenu=::CreatePopupMenu();

				::AppendMenu(hmenu,MF_STRING | MF_ENABLED,1,TEXT("コピー(&C)"));
				::AppendMenu(hmenu,MF_STRING | MF_ENABLED,2,TEXT("すべて選択(&A)"));
				::AppendMenu(hmenu,MF_STRING | MF_ENABLED,3,TEXT("番組名をコピー(&E)"));
				if (m_pEventHandler!=NULL)
					m_pEventHandler->OnMenuPopup(hmenu);
				POINT pt;
				::GetCursorPos(&pt);
				int Command=::TrackPopupMenu(hmenu,TPM_RIGHTBUTTON | TPM_RETURNCMD,pt.x,pt.y,0,hwnd,NULL);
				::DestroyMenu(hmenu);
				switch (Command) {
				case 1:
					if (::SendMessage(m_hwndEdit,EM_SELECTIONTYPE,0,0)==SEL_EMPTY) {
						CRichEditUtil::CopyAllText(m_hwndEdit);
					} else {
						::SendMessage(m_hwndEdit,WM_COPY,0,0);
					}
					break;
				case 2:
					CRichEditUtil::SelectAll(m_hwndEdit);
					break;
				case 3:
					CopyTextToClipboard(hwnd,m_EventInfo.GetEventName());
					break;
				default:
					if (Command>=CEventHandler::COMMAND_FIRST)
						m_pEventHandler->OnMenuSelected(Command);
					break;
				}
			}
			return 0;

		case EN_LINK:
			{
				ENLINK *penl=reinterpret_cast<ENLINK*>(lParam);

				if (penl->msg==WM_LBUTTONUP)
					CRichEditUtil::HandleLinkClick(penl);
			}
			return 0;
		}
		break;

	case WM_CLOSE:
		Hide();
		return 0;
	}

	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


void CEventInfoPopup::GetCloseButtonRect(RECT *pRect) const
{
	RECT rc;

	GetClientRect(&rc);
	rc.right-=m_ButtonMargin;
	rc.left=rc.right-m_ButtonSize;
	rc.top=m_ButtonMargin;
	rc.bottom=rc.top+m_ButtonSize;
	*pRect=rc;
}




CEventInfoPopup::CEventHandler::CEventHandler()
	: m_pPopup(NULL)
{
}


CEventInfoPopup::CEventHandler::~CEventHandler()
{
	if (m_pPopup!=NULL)
		m_pPopup->m_pEventHandler=NULL;
}




const LPCTSTR CEventInfoPopupManager::m_pszPropName=TEXT("EventInfoPopup");


CEventInfoPopupManager::CEventInfoPopupManager(CEventInfoPopup *pPopup)
	: m_pPopup(pPopup)
	, m_fEnable(true)
	, m_hwnd(NULL)
	, m_pOldWndProc(NULL)
	, m_pEventHandler(NULL)
	, m_HitTestParam(-1)
{
}


CEventInfoPopupManager::~CEventInfoPopupManager()
{
	Finalize();
}


bool CEventInfoPopupManager::Initialize(HWND hwnd,CEventHandler *pEventHandler)
{
	if (hwnd==NULL)
		return false;
	m_hwnd=hwnd;
	m_pOldWndProc=(WNDPROC)::SetWindowLongPtr(hwnd,GWLP_WNDPROC,(LONG_PTR)HookWndProc);
	m_pEventHandler=pEventHandler;
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pPopup=m_pPopup;
	m_fTrackMouseEvent=false;
	::SetProp(hwnd,m_pszPropName,this);
	return true;
}


void CEventInfoPopupManager::Finalize()
{
	if (m_hwnd!=NULL) {
		m_pPopup->Hide();
		if (m_pOldWndProc!=NULL) {
			::SetWindowLongPtr(m_hwnd,GWLP_WNDPROC,(LONG_PTR)m_pOldWndProc);
			m_pOldWndProc=NULL;
		}
		::RemoveProp(m_hwnd,m_pszPropName);
		m_hwnd=NULL;
	}
}


bool CEventInfoPopupManager::SetEnable(bool fEnable)
{
	m_fEnable=fEnable;
	if (!fEnable)
		m_pPopup->Hide();
	return true;
}


bool CEventInfoPopupManager::Popup(int x,int y)
{
	if (m_pEventHandler==NULL)
		return false;
	m_HitTestParam=-1;
	if (m_pEventHandler->HitTest(x,y,&m_HitTestParam)) {
		const CEventInfoData *pEventInfo;

		if (m_pEventHandler->GetEventInfo(m_HitTestParam,&pEventInfo)
				&& m_pEventHandler->OnShow(pEventInfo)) {
			m_pPopup->Show(pEventInfo);
			return true;
		}
	}
	return false;
}


LRESULT CALLBACK CEventInfoPopupManager::HookWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	CEventInfoPopupManager *pThis=static_cast<CEventInfoPopupManager*>(::GetProp(hwnd,m_pszPropName));

	if (pThis==NULL)
		return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
	switch (uMsg) {
	case WM_MOUSEMOVE:
		if (!pThis->m_fTrackMouseEvent) {
			TRACKMOUSEEVENT tme;

			tme.cbSize=sizeof(tme);
			tme.dwFlags=TME_HOVER | TME_LEAVE;
			tme.hwndTrack=hwnd;
			tme.dwHoverTime=1000;
			if (::TrackMouseEvent(&tme))
				pThis->m_fTrackMouseEvent=true;
		}
		if (pThis->m_pPopup->IsVisible() && pThis->m_pEventHandler!=NULL) {
			LPARAM Param;
			if (pThis->m_pEventHandler->HitTest(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam),&Param)) {
				if (Param!=pThis->m_HitTestParam) {
					const CEventInfoData *pEventInfo;

					pThis->m_HitTestParam=Param;
					if (pThis->m_pEventHandler->GetEventInfo(pThis->m_HitTestParam,&pEventInfo)
							&& pThis->m_pEventHandler->OnShow(pEventInfo))
						pThis->m_pPopup->Show(pEventInfo);
				}
			} else {
				pThis->m_pPopup->Hide();
			}
		}
		break;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
	case WM_NCLBUTTONDOWN:
	case WM_NCRBUTTONDOWN:
	case WM_NCMBUTTONDOWN:
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
	case WM_VSCROLL:
	case WM_HSCROLL:
		pThis->m_pPopup->Hide();
		break;

	case WM_MOUSELEAVE:
		if (pThis->m_pPopup->IsVisible()) {
			POINT pt;
			::GetCursorPos(&pt);
			HWND hwndCur=::WindowFromPoint(pt);
			if (!pThis->m_pPopup->IsOwnWindow(hwndCur))
				pThis->m_pPopup->Hide();
		}
		pThis->m_fTrackMouseEvent=false;
		return 0;

	case WM_ACTIVATE:
		if (LOWORD(wParam)==WA_INACTIVE) {
			HWND hwndActive=reinterpret_cast<HWND>(lParam);
			if (!pThis->m_pPopup->IsOwnWindow(hwndActive))
				pThis->m_pPopup->Hide();
		}
		break;

	case WM_MOUSEHOVER:
		if (pThis->m_pEventHandler!=NULL && pThis->m_fEnable
				&& ::GetActiveWindow()==::GetForegroundWindow()) {
			bool fHit=false;
			pThis->m_HitTestParam=-1;
			if (pThis->m_pEventHandler->HitTest(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam),&pThis->m_HitTestParam)) {
				const CEventInfoData *pEventInfo;

				if (pThis->m_pEventHandler->GetEventInfo(pThis->m_HitTestParam,&pEventInfo)
						&& pThis->m_pEventHandler->OnShow(pEventInfo)) {
					pThis->m_pPopup->Show(pEventInfo);
					fHit=true;
				}
			}
			if (!fHit)
				pThis->m_pPopup->Hide();
		}
		pThis->m_fTrackMouseEvent=false;
		return 0;

	case WM_SHOWWINDOW:
		if (!wParam)
			pThis->m_pPopup->Hide();
		return 0;

	case WM_DESTROY:
		::CallWindowProc(pThis->m_pOldWndProc,hwnd,uMsg,wParam,lParam);
		pThis->Finalize();
		return 0;
	}
	return ::CallWindowProc(pThis->m_pOldWndProc,hwnd,uMsg,wParam,lParam);
}




CEventInfoPopupManager::CEventHandler::CEventHandler()
	: m_pPopup(NULL)
{
}


CEventInfoPopupManager::CEventHandler::~CEventHandler()
{
}
