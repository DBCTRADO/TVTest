#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "InformationPanel.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define IDC_PROGRAMINFO		1000
#define IDC_PROGRAMINFOPREV	1001
#define IDC_PROGRAMINFONEXT	1002

#define PROGRAMINFO_BUTTON_SIZE	16

#define ITEM_FLAG(item) (1<<(item))

static const LPCTSTR SUBCLASS_PROP_NAME=APP_NAME TEXT("This");




const LPCTSTR CInformationPanel::m_pszClassName=APP_NAME TEXT(" Information Panel");
HINSTANCE CInformationPanel::m_hinst=NULL;


const LPCTSTR CInformationPanel::m_pszItemNameList[] = {
	TEXT("VideoInfo"),
	TEXT("VideoDecoder"),
	TEXT("VideoRenderer"),
	TEXT("AudioDevice"),
	TEXT("SignalLevel"),
	TEXT("MediaBitRate"),
	TEXT("Error"),
	TEXT("Record"),
	TEXT("ProgramInfo"),
};


bool CInformationPanel::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW;
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

	, m_hwndProgramInfo(NULL)
	, m_pOldProgramInfoProc(NULL)
	, m_hwndProgramInfoPrev(NULL)
	, m_hwndProgramInfoNext(NULL)
	, m_pEventHandler(NULL)
	, m_fUseRichEdit(true)

	, m_crBackColor(RGB(0,0,0))
	, m_crTextColor(RGB(255,255,255))
	, m_crProgramInfoBackColor(RGB(0,0,0))
	, m_crProgramInfoTextColor(RGB(255,255,255))
	, m_FontHeight(0)
	, m_LineMargin(1)
	, m_ItemVisibility(ITEM_FLAG(ITEM_VIDEO)
					 | ITEM_FLAG(ITEM_DECODER)
					 | ITEM_FLAG(ITEM_VIDEORENDERER)
					 | ITEM_FLAG(ITEM_SIGNALLEVEL)
					 | ITEM_FLAG(ITEM_ERROR)
					 | ITEM_FLAG(ITEM_RECORD)
					 | ITEM_FLAG(ITEM_PROGRAMINFO))

	, m_OriginalVideoWidth(0)
	, m_OriginalVideoHeight(0)
	, m_DisplayVideoWidth(0)
	, m_DisplayVideoHeight(0)
	, m_AspectX(0)
	, m_AspectY(0)
	, m_fSignalLevel(false)
	, m_VideoBitRate(0)
	, m_AudioBitRate(0)
	, m_fRecording(false)
	, m_fProgramInfoCursorOverLink(false)
	, m_fNextProgramInfo(false)
{
}


CInformationPanel::~CInformationPanel()
{
	Destroy();
}


bool CInformationPanel::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 m_pszClassName,TEXT("情報"),m_hinst);
}


void CInformationPanel::ResetStatistics()
{
	/*
	m_OriginalVideoWidth=0;
	m_OriginalVideoHeight=0;
	m_DisplayVideoWidth=0;
	m_DisplayVideoHeight=0;
	*/
	m_AspectX=0;
	m_AspectY=0;
	/*
	m_VideoDecoderName.Clear();
	m_VideoRendererName.Clear();
	m_AudioDeviceName.Clear();
	*/
	//m_fRecording=false;
	m_ProgramInfo.Clear();
	m_ProgramInfoLinkList.clear();
	m_fNextProgramInfo=false;

	if (m_hwnd!=NULL) {
		InvalidateRect(m_hwnd,NULL,TRUE);
		SetWindowText(m_hwndProgramInfo,TEXT(""));
		EnableWindow(m_hwndProgramInfoPrev,FALSE);
		EnableWindow(m_hwndProgramInfoNext,TRUE);
	}
}


bool CInformationPanel::IsVisible() const
{
	return m_hwnd!=NULL;
}


void CInformationPanel::SetColor(COLORREF crBackColor,COLORREF crTextColor)
{
	m_crBackColor=crBackColor;
	m_crTextColor=crTextColor;
	if (m_hwnd!=NULL) {
		m_BackBrush.Create(crBackColor);
		::InvalidateRect(m_hwnd,NULL,TRUE);
		::InvalidateRect(m_hwndProgramInfoPrev,NULL,TRUE);
		::InvalidateRect(m_hwndProgramInfoNext,NULL,TRUE);
	}
}


void CInformationPanel::SetProgramInfoColor(COLORREF crBackColor,COLORREF crTextColor)
{
	m_crProgramInfoBackColor=crBackColor;
	m_crProgramInfoTextColor=crTextColor;
	if (m_hwnd!=NULL) {
		m_ProgramInfoBackBrush.Create(crBackColor);
		if (m_hwndProgramInfo!=NULL) {
			if (m_fUseRichEdit) {
				::SendMessage(m_hwndProgramInfo,EM_SETBKGNDCOLOR,0,m_crProgramInfoBackColor);
				POINT ptScroll;
				::SendMessage(m_hwndProgramInfo,EM_GETSCROLLPOS,0,reinterpret_cast<LPARAM>(&ptScroll));
				UpdateProgramInfoText();
				::SendMessage(m_hwndProgramInfo,EM_SETSCROLLPOS,0,reinterpret_cast<LPARAM>(&ptScroll));
			}
			::InvalidateRect(m_hwndProgramInfo,NULL,TRUE);
		}
	}
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


void CInformationPanel::UpdateItem(int Item)
{
	if (m_hwnd!=NULL && IsItemVisible(Item)) {
		RECT rc;

		GetItemRect(Item,&rc);
		::InvalidateRect(m_hwnd,&rc,TRUE);
	}
}


void CInformationPanel::SetVideoSize(int OriginalWidth,int OriginalHeight,int DisplayWidth,int DisplayHeight)
{
	if (OriginalWidth!=m_OriginalVideoWidth || OriginalHeight!=m_OriginalVideoHeight
			|| DisplayWidth!=m_DisplayVideoWidth || DisplayHeight!=m_DisplayVideoHeight) {
		m_OriginalVideoWidth=OriginalWidth;
		m_OriginalVideoHeight=OriginalHeight;
		m_DisplayVideoWidth=DisplayWidth;
		m_DisplayVideoHeight=DisplayHeight;
		UpdateItem(ITEM_VIDEO);
	}
}


void CInformationPanel::SetAspectRatio(int AspectX,int AspectY)
{
	if (AspectX!=m_AspectX || AspectY!=m_AspectY) {
		m_AspectX=AspectX;
		m_AspectY=AspectY;
		UpdateItem(ITEM_VIDEO);
	}
}


void CInformationPanel::SetVideoDecoderName(LPCTSTR pszName)
{
	if (m_VideoDecoderName.Compare(pszName)!=0) {
		m_VideoDecoderName.Set(pszName);
		UpdateItem(ITEM_DECODER);
	}
}


void CInformationPanel::SetVideoRendererName(LPCTSTR pszName)
{
	if (m_VideoRendererName.Compare(pszName)!=0) {
		m_VideoRendererName.Set(pszName);
		UpdateItem(ITEM_VIDEORENDERER);
	}
}


void CInformationPanel::SetAudioDeviceName(LPCTSTR pszName)
{
	if (m_AudioDeviceName.Compare(pszName)!=0) {
		m_AudioDeviceName.Set(pszName);
		UpdateItem(ITEM_AUDIODEVICE);
	}
}


void CInformationPanel::ShowSignalLevel(bool fShow)
{
	if (fShow!=m_fSignalLevel) {
		m_fSignalLevel=fShow;
		UpdateItem(ITEM_SIGNALLEVEL);
	}
}


void CInformationPanel::SetMediaBitRate(DWORD VideoBitRate,DWORD AudioBitRate)
{
	if (VideoBitRate!=m_VideoBitRate || AudioBitRate!=m_AudioBitRate) {
		m_VideoBitRate=VideoBitRate;
		m_AudioBitRate=AudioBitRate;
		UpdateItem(ITEM_MEDIABITRATE);
	}
}


void CInformationPanel::SetRecordStatus(bool fRecording,LPCTSTR pszFileName,
			ULONGLONG WroteSize,unsigned int RecordTime,ULONGLONG FreeSpace)
{
	m_fRecording=fRecording;
	if (fRecording) {
		m_RecordWroteSize=WroteSize;
		m_RecordTime=RecordTime;
		m_DiskFreeSpace=FreeSpace;
	}
	UpdateItem(ITEM_RECORD);
}


void CInformationPanel::SetProgramInfo(LPCTSTR pszInfo)
{
	if (m_ProgramInfo.Compare(pszInfo)!=0) {
		m_ProgramInfo.Set(pszInfo);
		if (m_hwndProgramInfo!=NULL)
			UpdateProgramInfoText();
	}
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
		if (m_fUseRichEdit) {
			::SendMessage(m_hwndProgramInfo,WM_SETREDRAW,FALSE,0);
			::SetWindowText(m_hwndProgramInfo,NULL);
			if (!m_ProgramInfo.IsEmpty()) {
				LOGFONT lf;
				CHARFORMAT cf;
				HDC hdc=::GetDC(m_hwndProgramInfo);

				m_Font.GetLogFont(&lf);
				CRichEditUtil::LogFontToCharFormat(hdc,&lf,&cf);
				cf.dwMask|=CFM_COLOR;
				cf.crTextColor=m_crProgramInfoTextColor;
				::ReleaseDC(m_hwndProgramInfo,hdc);
				CRichEditUtil::AppendText(m_hwndProgramInfo,m_ProgramInfo.Get(),&cf);
				CRichEditUtil::DetectURL(m_hwndProgramInfo,&cf,0,-1,
										 CRichEditUtil::URL_NO_LINK,&m_ProgramInfoLinkList);
				POINT pt={0,0};
				::SendMessage(m_hwndProgramInfo,EM_SETSCROLLPOS,0,reinterpret_cast<LPARAM>(&pt));
			}
			::SendMessage(m_hwndProgramInfo,WM_SETREDRAW,TRUE,0);
			::InvalidateRect(m_hwndProgramInfo,NULL,TRUE);
		} else {
			::SetWindowText(m_hwndProgramInfo,m_ProgramInfo.GetSafe());
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
		::SendMessage(m_hwndProgramInfo,EM_SETBKGNDCOLOR,0,m_crProgramInfoBackColor);
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
	if (!m_ProgramInfo.IsEmpty())
		UpdateProgramInfoText();

	return true;
}


bool CInformationPanel::SetEventHandler(CEventHandler *pHandler)
{
	m_pEventHandler=pHandler;
	return true;
}


LRESULT CInformationPanel::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			if (!m_Font.IsCreated())
				CreateDefaultFont(&m_Font);
			m_BackBrush.Create(m_crBackColor);
			m_ProgramInfoBackBrush.Create(m_crProgramInfoBackColor);
			CalcFontHeight();

			CreateProgramInfoEdit();
			if (IsItemVisible(ITEM_PROGRAMINFO))
				::ShowWindow(m_hwndProgramInfo,SW_SHOW);
			m_hwndProgramInfoPrev=CreateWindowEx(0,TEXT("BUTTON"),TEXT(""),
				WS_CHILD | (IsItemVisible(ITEM_PROGRAMINFO)?WS_VISIBLE:0) | WS_CLIPSIBLINGS | (m_fNextProgramInfo?0:WS_DISABLED)
												| BS_PUSHBUTTON | BS_OWNERDRAW,
				0,0,0,0,hwnd,(HMENU)IDC_PROGRAMINFOPREV,m_hinst,NULL);
			m_hwndProgramInfoNext=CreateWindowEx(0,TEXT("BUTTON"),TEXT(""),
				WS_CHILD | (IsItemVisible(ITEM_PROGRAMINFO)?WS_VISIBLE:0) | WS_CLIPSIBLINGS | (m_fNextProgramInfo?WS_DISABLED:0)
												| BS_PUSHBUTTON | BS_OWNERDRAW,
				0,0,0,0,hwnd,(HMENU)IDC_PROGRAMINFONEXT,m_hinst,NULL);
		}
		return 0;

	case WM_SIZE:
		if (IsItemVisible(ITEM_PROGRAMINFO)) {
			RECT rc;

			GetItemRect(ITEM_PROGRAMINFO,&rc);
			MoveWindow(m_hwndProgramInfo,rc.left,rc.top,
						rc.right-rc.left,rc.bottom-rc.top,TRUE);
			MoveWindow(m_hwndProgramInfoPrev,
						rc.right-PROGRAMINFO_BUTTON_SIZE*2,rc.bottom,
						PROGRAMINFO_BUTTON_SIZE,PROGRAMINFO_BUTTON_SIZE,TRUE);
			MoveWindow(m_hwndProgramInfoNext,
						rc.right-PROGRAMINFO_BUTTON_SIZE,rc.bottom,
						PROGRAMINFO_BUTTON_SIZE,PROGRAMINFO_BUTTON_SIZE,TRUE);
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

			SetTextColor(hdc,m_crProgramInfoTextColor);
			SetBkColor(hdc,m_crProgramInfoBackColor);
			return reinterpret_cast<LRESULT>(m_ProgramInfoBackBrush.GetHandle());
		}

	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT pdis=reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
			HBRUSH hbrOld,hbr;
			HPEN hpen,hpenOld;
			POINT Points[3];

			hpen=CreatePen(PS_SOLID,1,m_crTextColor);
			hbrOld=DrawUtil::SelectObject(pdis->hDC,m_BackBrush);
			hpenOld=SelectPen(pdis->hDC,hpen);
			Rectangle(pdis->hDC,pdis->rcItem.left,pdis->rcItem.top,
								pdis->rcItem.right,pdis->rcItem.bottom);
			if (pdis->CtlID==IDC_PROGRAMINFOPREV) {
				Points[0].x=pdis->rcItem.right-5;
				Points[0].y=pdis->rcItem.top+3;
				Points[1].x=Points[0].x;
				Points[1].y=pdis->rcItem.bottom-4;
				Points[2].x=pdis->rcItem.left+4;
				Points[2].y=pdis->rcItem.top+(pdis->rcItem.bottom-pdis->rcItem.top)/2;
			} else {
				Points[0].x=pdis->rcItem.left+4;
				Points[0].y=pdis->rcItem.top+3;
				Points[1].x=Points[0].x;
				Points[1].y=pdis->rcItem.bottom-4;
				Points[2].x=pdis->rcItem.right-5;
				Points[2].y=pdis->rcItem.top+(pdis->rcItem.bottom-pdis->rcItem.top)/2;
			}
			if ((pdis->itemState&ODS_DISABLED)!=0) {
				hbr=CreateSolidBrush(MixColor(m_crBackColor,m_crTextColor));
			} else {
				hbr=CreateSolidBrush(m_crTextColor);
			}
			SelectBrush(pdis->hDC,hbr);
			SelectObject(pdis->hDC,GetStockObject(NULL_PEN));
			Polygon(pdis->hDC,Points,3);
			SelectPen(pdis->hDC,hpenOld);
			SelectBrush(pdis->hDC,hbrOld);
			DeleteObject(hpen);
			DeleteObject(hbr);
		}
		return TRUE;

	case WM_RBUTTONDOWN:
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

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case CM_INFORMATIONPANEL_ITEM_VIDEO:
		case CM_INFORMATIONPANEL_ITEM_VIDEODECODER:
		case CM_INFORMATIONPANEL_ITEM_VIDEORENDERER:
		case CM_INFORMATIONPANEL_ITEM_AUDIODEVICE:
		case CM_INFORMATIONPANEL_ITEM_SIGNALLEVEL:
		case CM_INFORMATIONPANEL_ITEM_MEDIABITRATE:
		case CM_INFORMATIONPANEL_ITEM_ERROR:
		case CM_INFORMATIONPANEL_ITEM_RECORD:
		case CM_INFORMATIONPANEL_ITEM_PROGRAMINFO:
			{
				int Item=LOWORD(wParam)-CM_INFORMATIONPANEL_ITEM_FIRST;

				SetItemVisible(Item,!IsItemVisible(Item));
			}
			return 0;

		case IDC_PROGRAMINFOPREV:
		case IDC_PROGRAMINFONEXT:
			{
				bool fNext=LOWORD(wParam)==IDC_PROGRAMINFONEXT;

				if (fNext!=m_fNextProgramInfo) {
					m_fNextProgramInfo=fNext;
					m_ProgramInfo.Clear();
					EnableWindow(m_hwndProgramInfoPrev,fNext);
					EnableWindow(m_hwndProgramInfoNext,!fNext);
					if (m_pEventHandler!=NULL)
						m_pEventHandler->OnProgramInfoUpdate(fNext);
					if (m_ProgramInfo.IsEmpty())
						SetWindowText(m_hwndProgramInfo,TEXT(""));
				}
			}
			return 0;
		}
		return 0;

	case WM_SETCURSOR:
		if ((HWND)wParam==m_hwndProgramInfoPrev
				|| (HWND)wParam==m_hwndProgramInfoNext) {
			::SetCursor(::LoadCursor(NULL,IDC_HAND));
			return TRUE;
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
				case WM_RBUTTONDOWN:
					{
						HMENU hmenu=::CreatePopupMenu();

						::AppendMenu(hmenu,MF_STRING | MF_ENABLED,1,TEXT("コピー(&C)"));
						::AppendMenu(hmenu,MF_STRING | MF_ENABLED,2,TEXT("すべて選択(&A)"));
						POINT pt;
						::GetCursorPos(&pt);
						int Command=::TrackPopupMenu(hmenu,TPM_RIGHTBUTTON | TPM_RETURNCMD,pt.x,pt.y,0,hwnd,NULL);
						::DestroyMenu(hmenu);
						switch (Command) {
						case 1:
							if (::SendMessage(m_hwndProgramInfo,EM_SELECTIONTYPE,0,0)==SEL_EMPTY) {
								CRichEditUtil::CopyAllText(m_hwndProgramInfo);
							} else {
								::SendMessage(m_hwndProgramInfo,WM_COPY,0,0);
							}
							break;
						case 2:
							CRichEditUtil::SelectAll(m_hwndProgramInfo);
							break;
						}
					}
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
		m_hwndProgramInfoPrev=NULL;
		m_hwndProgramInfoNext=NULL;
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
		if (!pThis->m_ProgramInfo.IsEmpty()) {
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

	case WM_RBUTTONUP:
		return 0;

	case WM_NCDESTROY:
		SubclassWindow(hwnd,pThis->m_pOldProgramInfoProc);
		::RemoveProp(hwnd,SUBCLASS_PROP_NAME);
		pThis->m_hwndProgramInfo=NULL;
		break;
	}

	return ::CallWindowProc(pThis->m_pOldProgramInfoProc,hwnd,uMsg,wParam,lParam);
}


void CInformationPanel::GetItemRect(int Item,RECT *pRect) const
{
	int VisibleItemCount;

	GetClientRect(pRect);
	VisibleItemCount=0;
	for (int i=0;i<Item;i++) {
		if ((m_ItemVisibility&ITEM_FLAG(i))!=0)
			VisibleItemCount++;
	}
	pRect->top=(m_FontHeight+m_LineMargin)*VisibleItemCount;
	if ((m_ItemVisibility&ITEM_FLAG(Item))==0) {
		pRect->bottom=pRect->top;
	} else {
		if (Item==ITEM_PROGRAMINFO)
			pRect->bottom-=PROGRAMINFO_BUTTON_SIZE;
		if (Item!=ITEM_PROGRAMINFO || pRect->top>=pRect->bottom)
			pRect->bottom=pRect->top+m_FontHeight;
	}
}


bool CInformationPanel::SetItemVisible(int Item,bool fVisible)
{
	if (Item<0 || Item>=NUM_ITEMS)
		return false;
	if (IsItemVisible(Item)!=fVisible) {
		m_ItemVisibility^=ITEM_FLAG(Item);
		if (m_hwnd!=NULL) {
			if (IsItemVisible(ITEM_PROGRAMINFO)) {
				SendSizeMessage();
			}
			if (Item==ITEM_PROGRAMINFO) {
				const int Show=fVisible?SW_SHOW:SW_HIDE;
				::ShowWindow(m_hwndProgramInfo,Show);
				::ShowWindow(m_hwndProgramInfoPrev,Show);
				::ShowWindow(m_hwndProgramInfoNext,Show);
			} else {
				::InvalidateRect(m_hwnd,NULL,TRUE);
			}
		}
	}
	return true;
}


bool CInformationPanel::IsItemVisible(int Item) const
{
	if (Item<0 || Item>=NUM_ITEMS)
		return false;
	return (m_ItemVisibility&ITEM_FLAG(Item))!=0;
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
	TCHAR szText[256];

	GetClientRect(&rc);
	if (rc.right>m_Offscreen.GetWidth()
			|| m_FontHeight+m_LineMargin>m_Offscreen.GetHeight())
		m_Offscreen.Create(rc.right,m_FontHeight+m_LineMargin);
	hdcDst=m_Offscreen.GetDC();
	if (hdcDst==NULL)
		hdcDst=hdc;

	HFONT hfontOld=DrawUtil::SelectObject(hdcDst,m_Font);
	COLORREF crOldTextColor=::SetTextColor(hdcDst,m_crTextColor);
	int OldBkMode=::SetBkMode(hdcDst,TRANSPARENT);

	if (GetDrawItemRect(ITEM_VIDEO,&rc,PaintRect)) {
		StdUtil::snprintf(szText,lengthof(szText),
						  TEXT("%d x %d [%d x %d (%d:%d)]"),
						  m_OriginalVideoWidth,m_OriginalVideoHeight,
						  m_DisplayVideoWidth,m_DisplayVideoHeight,
						  m_AspectX,m_AspectY);
		DrawItem(hdc,szText,rc);
	}

	if (GetDrawItemRect(ITEM_DECODER,&rc,PaintRect)) {
		DrawItem(hdc,m_VideoDecoderName.Get(),rc);
	}

	if (GetDrawItemRect(ITEM_VIDEORENDERER,&rc,PaintRect)) {
		DrawItem(hdc,m_VideoRendererName.Get(),rc);
	}

	if (GetDrawItemRect(ITEM_AUDIODEVICE,&rc,PaintRect)) {
		DrawItem(hdc,m_AudioDeviceName.Get(),rc);
	}

	if (GetDrawItemRect(ITEM_SIGNALLEVEL,&rc,PaintRect)) {
		const CCoreEngine *pCoreEngine=GetAppClass().GetCoreEngine();
		int Length=0;
		if (m_fSignalLevel) {
			TCHAR szSignalLevel[32];
			pCoreEngine->GetSignalLevelText(szSignalLevel,lengthof(szSignalLevel));
			Length=StdUtil::snprintf(szText,lengthof(szText),
									 TEXT("%s / "),szSignalLevel);
		}
		pCoreEngine->GetBitRateText(szText+Length,lengthof(szText)-Length);
		DrawItem(hdc,szText,rc);
	}

	if (GetDrawItemRect(ITEM_MEDIABITRATE,&rc,PaintRect)) {
		/*
		unsigned int VideoBitRate=m_VideoBitRate*100/1024;
		unsigned int AudioBitRate=m_AudioBitRate*100/1024;
		StdUtil::snprintf(szText,lengthof(szText),
						  TEXT("映像 %u.%02u Kbps / 音声 %u.%02u Kbps"),
						  VideoBitRate/100,VideoBitRate%100,
						  AudioBitRate/100,AudioBitRate%100);
		*/
		StdUtil::snprintf(szText,lengthof(szText),TEXT("映像 %u Kbps / 音声 %u Kbps"),
						  m_VideoBitRate/1024,m_AudioBitRate/1024);
		DrawItem(hdc,szText,rc);
	}

	if (GetDrawItemRect(ITEM_ERROR,&rc,PaintRect)) {
		const CCoreEngine *pCoreEngine=GetAppClass().GetCoreEngine();
		int Length;

		Length=StdUtil::snprintf(szText,lengthof(szText),TEXT("D %u / E %u"),
								 pCoreEngine->GetContinuityErrorPacketCount(),
								 pCoreEngine->GetErrorPacketCount());
		if (pCoreEngine->GetDescramble()
				&& pCoreEngine->GetCasDevice()>=0)
			StdUtil::snprintf(szText+Length,lengthof(szText)-Length,TEXT(" / S %u"),
							  pCoreEngine->GetScramblePacketCount());
		DrawItem(hdc,szText,rc);
	}

	if (GetDrawItemRect(ITEM_RECORD,&rc,PaintRect)) {
		if (m_fRecording) {
			unsigned int RecordSec=m_RecordTime/1000;
			unsigned int Size=
				(unsigned int)(m_RecordWroteSize/(ULONGLONG)(1024*1024/100));
			unsigned int FreeSpace=
				(unsigned int)(m_DiskFreeSpace/(ULONGLONG)(1024*1024*1024/100));

			StdUtil::snprintf(szText,lengthof(szText),
				TEXT("● %d:%02d:%02d / %d.%02d MB / %d.%02d GB空き"),
				RecordSec/(60*60),(RecordSec/60)%60,RecordSec%60,
				Size/100,Size%100,FreeSpace/100,FreeSpace%100);
			DrawItem(hdc,szText,rc);
		} else {
			DrawItem(hdc,TEXT("■ <録画していません>"),rc);
		}
	}

	GetItemRect(ITEM_PROGRAMINFO-1,&rc);
	if (PaintRect.bottom>rc.bottom) {
		rc.left=PaintRect.left;
		rc.top=max(PaintRect.top,rc.bottom);
		rc.right=PaintRect.right;
		rc.bottom=PaintRect.bottom;
		::FillRect(hdc,&rc,m_BackBrush.GetHandle());
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
	pRect->bottom+=m_LineMargin;
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


bool CInformationPanel::ReadSettings(CSettings &Settings)
{
	for (int i=0;i<lengthof(m_pszItemNameList);i++) {
		bool f;

		if (Settings.Read(m_pszItemNameList[i],&f))
			SetItemVisible(i,f);
	}
	return true;
}


bool CInformationPanel::WriteSettings(CSettings &Settings)
{
	for (int i=0;i<lengthof(m_pszItemNameList);i++)
		Settings.Write(m_pszItemNameList[i],IsItemVisible(i));
	return true;
}
