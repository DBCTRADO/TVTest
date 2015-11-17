#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "EventInfoPopup.h"
#include "EpgUtil.h"
#include "Aero.h"
#include "Common/DebugDef.h"




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
	, m_TitleBackColor(RGB(228,228,240))
	, m_TitleTextColor(RGB(80,80,80))
	, m_TitleHeight(0)
	, m_TitleLeftMargin(2)
	, m_TitleIconTextMargin(4)
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
	, m_fMenuShowing(false)
{
	m_WindowPosition.Width=320;
	m_WindowPosition.Height=320;
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
	if (m_EventInfo.IsEqual(*pEventInfo))
		return;

	m_EventInfo=*pEventInfo;

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

	TCHAR szText[4096];
	CStaticStringFormatter Formatter(szText,lengthof(szText));

	{
		TCHAR szBuf[EpgUtil::MAX_EVENT_TIME_LENGTH];
		if (EpgUtil::FormatEventTime(pEventInfo,szBuf,lengthof(szBuf),
									 EpgUtil::EVENT_TIME_DATE | EpgUtil::EVENT_TIME_YEAR)>0) {
			Formatter.Append(szBuf);
			Formatter.Append(TEXT("\r\n"));
		}
	}

	if (!m_EventInfo.m_EventName.empty()) {
		Formatter.Append(m_EventInfo.m_EventName.c_str());
		Formatter.Append(TEXT("\r\n"));
	}

	if (!Formatter.IsEmpty()) {
		Formatter.Append(TEXT("\r\n"));
		CHARFORMAT cfBold=cf;
		cfBold.dwMask|=CFM_BOLD;
		cfBold.dwEffects|=CFE_BOLD;
		CRichEditUtil::AppendText(m_hwndEdit,Formatter.GetString(),&cfBold);
		Formatter.Clear();
	}

	if (!m_EventInfo.m_EventText.empty()) {
		Formatter.Append(m_EventInfo.m_EventText.c_str());
		Formatter.RemoveTrailingWhitespace();
	}
	if (!m_EventInfo.m_EventExtendedText.empty()) {
		if (!m_EventInfo.m_EventText.empty())
			Formatter.Append(TEXT("\r\n\r\n"));
		Formatter.Append(m_EventInfo.m_EventExtendedText.c_str());
		Formatter.RemoveTrailingWhitespace();
	}

	Formatter.Append(TEXT("\r\n"));

	if (!m_EventInfo.m_VideoList.empty()) {
		// TODO: 複数映像対応
		LPCTSTR pszVideo=EpgUtil::GetComponentTypeText(
			m_EventInfo.m_VideoList[0].StreamContent,
			m_EventInfo.m_VideoList[0].ComponentType);
		if (pszVideo!=NULL) {
			Formatter.AppendFormat(TEXT("\r\n■ 映像： %s"),pszVideo);
		}
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

	int Genre1,Genre2;
	if (EpgUtil::GetEventGenre(m_EventInfo,&Genre1,&Genre2)) {
		CEpgGenre EpgGenre;
		LPCTSTR pszGenre=EpgGenre.GetText(Genre1,-1);
		if (pszGenre!=NULL) {
			Formatter.AppendFormat(TEXT("\r\n■ ジャンル： %s"),pszGenre);
			pszGenre=EpgGenre.GetText(Genre1,Genre2);
			if (pszGenre!=NULL)
				Formatter.AppendFormat(TEXT(" - %s"),pszGenre);
		}
	}

	if (m_fDetailInfo) {
		Formatter.AppendFormat(TEXT("\r\n■ イベントID： 0x%04X"),m_EventInfo.m_EventID);
		if (m_EventInfo.m_bCommonEvent)
			Formatter.AppendFormat(TEXT(" (イベント共有 サービスID 0x%04X / イベントID 0x%04X)"),
								   m_EventInfo.m_CommonEventInfo.ServiceID,
								   m_EventInfo.m_CommonEventInfo.EventID);
	}

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
		pszAudio=EpgUtil::GetComponentTypeText(
			pAudioInfo->StreamContent,pAudioInfo->ComponentType);
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
		TCHAR szLang1[EpgUtil::MAX_LANGUAGE_TEXT_LENGTH];
		TCHAR szLang2[EpgUtil::MAX_LANGUAGE_TEXT_LENGTH];
		EpgUtil::GetLanguageText(pAudioInfo->LanguageCode,szLang1,lengthof(szLang1));
		EpgUtil::GetLanguageText(pAudioInfo->LanguageCode2,szLang2,lengthof(szLang2));
		StdUtil::snprintf(szAudioComponent,lengthof(szAudioComponent),
						  TEXT(" [%s/%s]"),szLang1,szLang2);
	} else {
		TCHAR szLang[EpgUtil::MAX_LANGUAGE_TEXT_LENGTH];
		EpgUtil::GetLanguageText(pAudioInfo->LanguageCode,szLang,lengthof(szLang));
		StdUtil::snprintf(szAudioComponent,lengthof(szAudioComponent),
						  TEXT(" [%s]"),szLang);
	}

	StdUtil::snprintf(pszText,MaxLength,TEXT("%s%s"),
					  pszAudio!=NULL?pszAudio:TEXT("?"),
					  szAudioComponent);
}


void CEventInfoPopup::CalcTitleHeight()
{
	int FontHeight=0;
	HDC hdc=::GetDC(m_hwnd);
	if (hdc!=NULL) {
		HFONT hfontOld=DrawUtil::SelectObject(hdc,m_TitleFont);
		TEXTMETRIC tm;
		::GetTextMetrics(hdc,&tm);
		FontHeight=tm.tmHeight;
		::SelectObject(hdc,hfontOld);
		::ReleaseDC(m_hwnd,hdc);
	}

	int IconHeight=::GetSystemMetrics(SM_CYSMICON)+2;
	int ButtonHeight=m_ButtonSize+m_ButtonMargin*2;

	m_TitleHeight=max(IconHeight,ButtonHeight);
	m_TitleHeight=max(m_TitleHeight,FontHeight+2);
}


bool CEventInfoPopup::Show(const CEventInfoData *pEventInfo,const RECT *pPos,
						   HICON hIcon,LPCTSTR pszTitle)
{
	if (pEventInfo==NULL)
		return false;

	if (m_hwnd==NULL) {
		if (!Create(NULL,WS_POPUP | WS_CLIPCHILDREN | WS_THICKFRAME,WS_EX_TOPMOST | WS_EX_NOACTIVATE,0))
			return false;
	}

	if (!GetVisible() || m_EventInfo!=*pEventInfo) {
		if (pPos!=NULL) {
			SetPosition(pPos);
		} else {
			RECT rc;

			GetDefaultPopupPosition(&rc);
			::SetWindowPos(m_hwnd,HWND_TOPMOST,
						   rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,
						   SWP_NOACTIVATE);
		}
	}

	SetEventInfo(pEventInfo);

	if (pszTitle!=NULL)
		m_TitleText=pszTitle;
	else
		m_TitleText.clear();

	m_TitleIcon.Attach(hIcon);

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


void CEventInfoPopup::SetTitleColor(COLORREF BackColor,COLORREF TextColor)
{
	m_TitleBackColor=BackColor;
	m_TitleTextColor=TextColor;
	if (m_hwnd!=NULL) {
		RECT rc;

		GetClientRect(&rc);
		rc.bottom=m_TitleHeight;
		::InvalidateRect(m_hwnd,&rc,TRUE);
		::RedrawWindow(m_hwnd,NULL,NULL,RDW_FRAME | RDW_INVALIDATE);
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


void CEventInfoPopup::GetPreferredIconSize(int *pWidth,int *pHeight) const
{
	if (pWidth!=NULL)
		*pWidth=::GetSystemMetrics(SM_CXSMICON);
	if (pHeight!=NULL)
		*pHeight=::GetSystemMetrics(SM_CYSMICON);
}


bool CEventInfoPopup::GetPopupPosition(int x,int y,RECT *pPos) const
{
	if (pPos==NULL)
		return false;

	RECT rc;
	int Width,Height;

	GetPosition(&rc);
	Width=rc.right-rc.left;
	Height=rc.bottom-rc.top;

	POINT pt={x,y};
	HMONITOR hMonitor=::MonitorFromPoint(pt,MONITOR_DEFAULTTONEAREST);
	if (hMonitor!=NULL) {
		MONITORINFO mi;

		mi.cbSize=sizeof(mi);
		if (::GetMonitorInfo(hMonitor,&mi)) {
			if (x+Width>mi.rcMonitor.right)
				x=mi.rcMonitor.right-Width;
			if (y+Height>mi.rcMonitor.bottom) {
				y=mi.rcMonitor.bottom-Height;
				if (x+Width<mi.rcMonitor.right)
					x+=min(16,mi.rcMonitor.right-(x+Width));
			}
		}
	}

	pPos->left=x;
	pPos->right=x+Width;
	pPos->top=y;
	pPos->bottom=y+Height;

	return true;
}


bool CEventInfoPopup::AdjustPopupPosition(POINT *pPos) const
{
	if (pPos==NULL)
		return false;

	RECT rc;
	if (!GetPopupPosition(pPos->x,pPos->y,&rc))
		return false;

	pPos->x=rc.left;
	pPos->y=rc.top;

	return true;
}


bool CEventInfoPopup::GetDefaultPopupPosition(RECT *pPos) const
{
	if (pPos==NULL)
		return false;

	POINT pt;
	::GetCursorPos(&pt);

	return GetPopupPosition(pt.x,pt.y+16,pPos);
}


bool CEventInfoPopup::GetDefaultPopupPosition(POINT *pPos) const
{
	if (pPos==NULL)
		return false;

	RECT rc;
	if (!GetDefaultPopupPosition(&rc))
		return false;

	pPos->x=rc.left;
	pPos->y=rc.top;

	return true;
}


LRESULT CEventInfoPopup::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		if (!m_Font.IsCreated()) {
			LOGFONT lf;
			DrawUtil::GetSystemFont(DrawUtil::FONT_MESSAGE,&lf);
			m_Font.Create(&lf);
			lf.lfWeight=FW_BOLD;
			m_TitleFont.Create(&lf);
		}

		m_RichEditUtil.LoadRichEditLib();
		m_hwndEdit=::CreateWindowEx(0,m_RichEditUtil.GetWindowClassName(),TEXT(""),
			WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_NOHIDESEL,
			0,0,0,0,hwnd,(HMENU)1,m_hinst,NULL);
		SetWindowFont(m_hwndEdit,m_Font.GetHandle(),FALSE);
		::SendMessage(m_hwndEdit,EM_SETEVENTMASK,0,ENM_MOUSEEVENTS | ENM_LINK);
		::SendMessage(m_hwndEdit,EM_SETBKGNDCOLOR,0,m_BackColor);

		SetNcRendering();
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

			::BeginPaint(hwnd,&ps);

			::GetClientRect(hwnd,&rc);
			rc.bottom=m_TitleHeight;
			DrawUtil::Fill(ps.hdc,&rc,m_TitleBackColor);

			rc.left+=m_TitleLeftMargin;

			if (m_TitleIcon) {
				int IconWidth=::GetSystemMetrics(SM_CXSMICON);
				int IconHeight=::GetSystemMetrics(SM_CYSMICON);

				::DrawIconEx(ps.hdc,
					rc.left,rc.top+(m_TitleHeight-IconHeight)/2,
					m_TitleIcon,IconWidth,IconHeight,0,NULL,DI_NORMAL);
				rc.left+=IconWidth+m_TitleIconTextMargin;
			}

			if (!m_TitleText.empty()) {
				rc.right-=m_ButtonSize+m_ButtonMargin*2;
				if (rc.left<rc.right) {
					HFONT hfontOld=DrawUtil::SelectObject(ps.hdc,m_TitleFont);
					int OldBkMode=::SetBkMode(ps.hdc,TRANSPARENT);
					COLORREF OldTextColor=::SetTextColor(ps.hdc,m_TitleTextColor);
					::DrawText(ps.hdc,m_TitleText.data(),(int)m_TitleText.length(),&rc,
							   DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
					::SelectObject(ps.hdc,hfontOld);
					::SetBkMode(ps.hdc,OldBkMode);
					::SetTextColor(ps.hdc,OldTextColor);
				}
			}

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
				CopyTextToClipboard(hwnd,m_EventInfo.m_EventName.c_str());
				break;
			}
			return 0;
		}
		break;

	case WM_MOUSEWHEEL:
		return ::SendMessage(m_hwndEdit,uMsg,wParam,lParam);

	case WM_NCMOUSEMOVE:
		m_fCursorInWindow=true;
		return 0;

	case WM_SHOWWINDOW:
		if (wParam!=0) {
			::SetTimer(hwnd,TIMER_ID_HIDE,200,NULL);
			m_fCursorInWindow=false;
		} else {
			::KillTimer(hwnd,TIMER_ID_HIDE);
		}
		return 0;

	case WM_TIMER:
		if (wParam==TIMER_ID_HIDE) {
			if (!m_fMenuShowing) {
				POINT pt;

				::GetCursorPos(&pt);
				if (!m_fCursorInWindow) {
					if (IsOwnWindow(::WindowFromPoint(pt)))
						m_fCursorInWindow=true;
				} else {
					RECT rc;

					::GetWindowRect(hwnd,&rc);
					::InflateRect(&rc,::GetSystemMetrics(SM_CXSIZEFRAME)*2,::GetSystemMetrics(SM_CYSIZEFRAME)*2);
					if (!::PtInRect(&rc,pt))
						Hide();
				}
			}
		}
		return 0;

	case WM_ENTERMENULOOP:
		m_fMenuShowing=true;
		return 0;

	case WM_EXITMENULOOP:
		m_fMenuShowing=false;
		m_fCursorInWindow=false;
		return 0;

	case WM_NCACTIVATE:
		return TRUE;

	case WM_NCPAINT:
		{
			HDC hdc=::GetWindowDC(hwnd);
			RECT rcWindow,rcClient;

			::GetWindowRect(hwnd,&rcWindow);
			::GetClientRect(hwnd,&rcClient);
			MapWindowRect(hwnd,NULL,&rcClient);
			::OffsetRect(&rcClient,-rcWindow.left,-rcWindow.top);
			::OffsetRect(&rcWindow,-rcWindow.left,-rcWindow.top);
			DrawUtil::FillBorder(hdc,&rcWindow,&rcClient,&rcWindow,m_TitleBackColor);
			HPEN hpen=::CreatePen(PS_SOLID,1,MixColor(m_TitleBackColor,RGB(0,0,0),192));
			HGDIOBJ hOldPen=::SelectObject(hdc,hpen);
			HGDIOBJ hOldBrush=::SelectObject(hdc,::GetStockObject(NULL_BRUSH));
			::Rectangle(hdc,rcWindow.left,rcWindow.top,rcWindow.right,rcWindow.bottom);
			::Rectangle(hdc,rcClient.left-1,rcClient.top-1,rcClient.right+1,rcClient.bottom+1);
			::SelectObject(hdc,hOldBrush);
			::SelectObject(hdc,hOldPen);
			::DeleteObject(hpen);
			::ReleaseDC(hwnd,hdc);
		}
		return 0;

	case WM_DWMCOMPOSITIONCHANGED:
		SetNcRendering();
		return 0;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case EN_MSGFILTER:
			if (reinterpret_cast<MSGFILTER*>(lParam)->msg==WM_RBUTTONUP) {
				enum {
					COMMAND_COPY=1,
					COMMAND_SELECTALL,
					COMMAND_COPYEVENTNAME,
					COMMAND_SEARCH
				};
				HMENU hmenu=::CreatePopupMenu();

				::AppendMenu(hmenu,MF_STRING | MF_ENABLED,COMMAND_COPY,TEXT("コピー(&C)"));
				::AppendMenu(hmenu,MF_STRING | MF_ENABLED,COMMAND_SELECTALL,TEXT("すべて選択(&A)"));
				::AppendMenu(hmenu,MF_STRING | MF_ENABLED,COMMAND_COPYEVENTNAME,TEXT("番組名をコピー(&E)"));
				if (m_pEventHandler!=NULL)
					m_pEventHandler->OnMenuPopup(hmenu);
				if (CRichEditUtil::IsSelected(m_hwndEdit)) {
					const TVTest::CKeywordSearch &KeywordSearch=GetAppClass().KeywordSearch;
					if (KeywordSearch.GetSearchEngineCount()>0) {
						::AppendMenu(hmenu,MF_SEPARATOR,0,NULL);
						KeywordSearch.InitializeMenu(hmenu,COMMAND_SEARCH,CEventHandler::COMMAND_FIRST-COMMAND_SEARCH);
					}
				}

				POINT pt;
				::GetCursorPos(&pt);
				int Command=::TrackPopupMenu(hmenu,TPM_RIGHTBUTTON | TPM_RETURNCMD,pt.x,pt.y,0,hwnd,NULL);
				::DestroyMenu(hmenu);

				switch (Command) {
				case COMMAND_COPY:
					if (::SendMessage(m_hwndEdit,EM_SELECTIONTYPE,0,0)==SEL_EMPTY) {
						CRichEditUtil::CopyAllText(m_hwndEdit);
					} else {
						::SendMessage(m_hwndEdit,WM_COPY,0,0);
					}
					break;

				case COMMAND_SELECTALL:
					CRichEditUtil::SelectAll(m_hwndEdit);
					break;

				case COMMAND_COPYEVENTNAME:
					CopyTextToClipboard(hwnd,m_EventInfo.m_EventName.c_str());
					break;

				default:
					if (Command>=CEventHandler::COMMAND_FIRST) {
						m_pEventHandler->OnMenuSelected(Command);
					} else if (Command>=COMMAND_SEARCH) {
						LPTSTR pszKeyword=CRichEditUtil::GetSelectedText(m_hwndEdit);
						if (pszKeyword!=NULL) {
							GetAppClass().KeywordSearch.Search(Command-COMMAND_SEARCH,pszKeyword);
							delete [] pszKeyword;
						}
					}
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

	case WM_DESTROY:
		m_TitleIcon.Destroy();
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


void CEventInfoPopup::SetNcRendering()
{
	CAeroGlass Aero;

	if (Aero.IsEnabled())
		Aero.EnableNcRendering(m_hwnd,false);
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




CEventInfoPopupManager::CEventInfoPopupManager(CEventInfoPopup *pPopup)
	: m_pPopup(pPopup)
	, m_fEnable(true)
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
	if (!SetSubclass(hwnd))
		return false;
	m_pEventHandler=pEventHandler;
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pPopup=m_pPopup;
	m_fTrackMouseEvent=false;
	return true;
}


void CEventInfoPopupManager::Finalize()
{
	if (m_hwnd!=NULL) {
		m_pPopup->Hide();
		RemoveSubclass();
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
		if (m_pEventHandler->ShowPopup(m_HitTestParam,m_pPopup)) {
			return true;
		}
	}
	return false;
}


LRESULT CEventInfoPopupManager::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_MOUSEMOVE:
		if (!m_fTrackMouseEvent) {
			TRACKMOUSEEVENT tme;

			tme.cbSize=sizeof(tme);
			tme.dwFlags=TME_HOVER | TME_LEAVE;
			tme.hwndTrack=hwnd;
			tme.dwHoverTime=1000;
			if (::TrackMouseEvent(&tme))
				m_fTrackMouseEvent=true;
		}
		if (m_pPopup->IsVisible() && m_pEventHandler!=NULL) {
			LPARAM Param;
			if (m_pEventHandler->HitTest(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam),&Param)) {
				if (Param!=m_HitTestParam) {
					m_HitTestParam=Param;
					m_pEventHandler->ShowPopup(m_HitTestParam,m_pPopup);
				}
			} else {
				m_pPopup->Hide();
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
		m_pPopup->Hide();
		break;

	case WM_MOUSELEAVE:
		if (m_pPopup->IsVisible()) {
			POINT pt;
			::GetCursorPos(&pt);
			HWND hwndCur=::WindowFromPoint(pt);
			if (!m_pPopup->IsOwnWindow(hwndCur))
				m_pPopup->Hide();
		}
		m_fTrackMouseEvent=false;
		return 0;

	case WM_ACTIVATE:
		if (LOWORD(wParam)==WA_INACTIVE) {
			HWND hwndActive=reinterpret_cast<HWND>(lParam);
			if (!m_pPopup->IsOwnWindow(hwndActive))
				m_pPopup->Hide();
		}
		break;

	case WM_MOUSEHOVER:
		if (m_pEventHandler!=NULL && m_fEnable
				&& ::GetActiveWindow()==::GetForegroundWindow()) {
			bool fHit=false;
			m_HitTestParam=-1;
			if (m_pEventHandler->HitTest(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam),&m_HitTestParam)) {
				if (m_pEventHandler->ShowPopup(m_HitTestParam,m_pPopup)) {
					fHit=true;
				}
			}
			if (!fHit)
				m_pPopup->Hide();
		}
		m_fTrackMouseEvent=false;
		return 0;

	case WM_SHOWWINDOW:
		if (!wParam)
			m_pPopup->Hide();
		break;

	case WM_DESTROY:
		m_pPopup->Hide();
		break;
	}

	return CWindowSubclass::OnMessage(hwnd,uMsg,wParam,lParam);
}




CEventInfoPopupManager::CEventHandler::CEventHandler()
	: m_pPopup(NULL)
{
}


CEventInfoPopupManager::CEventHandler::~CEventHandler()
{
}
