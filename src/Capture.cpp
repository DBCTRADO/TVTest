#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "Capture.h"
#include "Image.h"
#include "DrawUtil.h"
#include "Menu.h"
#include "resource.h"
#include "Common/DebugDef.h"


#define CAPTURE_WINDOW_CLASS			APP_NAME TEXT(" Capture Window")
#define CAPTURE_PREVIEW_WINDOW_CLASS	APP_NAME TEXT(" Capture Preview")
#define CAPTURE_TITLE_TEXT TEXT("キャプチャ")

enum {
	CAPTURE_ICON_CAPTURE,
	CAPTURE_ICON_SAVE,
	CAPTURE_ICON_COPY
};




CCaptureImage::CCaptureImage(HGLOBAL hData)
	: m_hData(hData)
	, m_fLocked(false)
{
	::GetLocalTime(&m_stCaptureTime);
}


CCaptureImage::CCaptureImage(const BITMAPINFO *pbmi,const void *pBits)
{
	SIZE_T InfoSize,BitsSize;

	InfoSize=CalcDIBInfoSize(&pbmi->bmiHeader);
	BitsSize=CalcDIBBitsSize(&pbmi->bmiHeader);
	m_hData=::GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE,InfoSize+BitsSize);
	if (m_hData!=NULL) {
		BYTE *pData=static_cast<BYTE*>(::GlobalLock(m_hData));

		if (pData!=NULL) {
			::CopyMemory(pData,pbmi,InfoSize);
			::CopyMemory(pData+InfoSize,pBits,BitsSize);
			::GlobalUnlock(m_hData);
		} else {
			::GlobalFree(m_hData);
			m_hData=NULL;
		}
	}
	m_fLocked=false;
	::GetLocalTime(&m_stCaptureTime);
}


CCaptureImage::~CCaptureImage()
{
	if (m_hData!=NULL) {
		if (m_fLocked)
			::GlobalUnlock(m_hData);
		::GlobalFree(m_hData);
	}
}


bool CCaptureImage::SetClipboard(HWND hwnd)
{
	if (m_hData==NULL || m_fLocked)
		return false;

	HGLOBAL hCopy;
	SIZE_T Size;

	Size=::GlobalSize(m_hData);
	hCopy=::GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE,Size);
	if (hCopy==NULL)
		return false;
	::CopyMemory(::GlobalLock(hCopy),::GlobalLock(m_hData),Size);
	::GlobalUnlock(hCopy);
	::GlobalUnlock(m_hData);
	if (!::OpenClipboard(hwnd)) {
		::GlobalFree(hCopy);
		return false;
	}
	::EmptyClipboard();
	bool fOK=::SetClipboardData(CF_DIB,hCopy)!=NULL;
	::CloseClipboard();
	return fOK;
}


bool CCaptureImage::GetBitmapInfoHeader(BITMAPINFOHEADER *pbmih) const
{
	if (m_hData==NULL || m_fLocked)
		return false;

	BITMAPINFOHEADER *pbmihSrc=static_cast<BITMAPINFOHEADER*>(::GlobalLock(m_hData));

	if (pbmihSrc==NULL)
		return false;
	*pbmih=*pbmihSrc;
	::GlobalUnlock(m_hData);
	return true;
}


bool CCaptureImage::LockData(BITMAPINFO **ppbmi,BYTE **ppBits)
{
	if (m_hData==NULL || m_fLocked)
		return false;

	void *pDib=::GlobalLock(m_hData);
	BITMAPINFO *pbmi;

	if (pDib==NULL)
		return false;
	pbmi=static_cast<BITMAPINFO*>(pDib);
	if (ppbmi!=NULL)
		*ppbmi=pbmi;
	if (ppBits!=NULL)
		*ppBits=static_cast<BYTE*>(pDib)+CalcDIBInfoSize(&pbmi->bmiHeader);
	m_fLocked=true;
	return true;
}


bool CCaptureImage::UnlockData()
{
	if (!m_fLocked)
		return false;
	::GlobalUnlock(m_hData);
	m_fLocked=false;
	return true;
}


void CCaptureImage::SetComment(LPCTSTR pszComment)
{
	TVTest::StringUtility::Assign(m_Comment,pszComment);
}


LPCTSTR CCaptureImage::GetComment() const
{
	return TVTest::StringUtility::GetCStrOrNull(m_Comment);
}




CCapturePreview::CEventHandler::CEventHandler()
	: m_pCapturePreview(NULL)
{
}


CCapturePreview::CEventHandler::~CEventHandler()
{
}




HINSTANCE CCapturePreview::m_hinst=NULL;


bool CCapturePreview::Initialize(HINSTANCE hinst)
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
		wc.lpszClassName=CAPTURE_PREVIEW_WINDOW_CLASS;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CCapturePreview::CCapturePreview()
	: m_pImage(NULL)
	, m_crBackColor(RGB(0,0,0))
	, m_pEventHandler(NULL)
{
}


CCapturePreview::~CCapturePreview()
{
	Destroy();
}


bool CCapturePreview::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 CAPTURE_PREVIEW_WINDOW_CLASS,NULL,m_hinst);
}


bool CCapturePreview::SetImage(CCaptureImage *pImage)
{
	ClearImage();
	m_pImage=pImage;
	if (m_hwnd!=NULL) {
		Invalidate();
		Update();
	}
	return true;
}


bool CCapturePreview::ClearImage()
{
	if (m_pImage!=NULL) {
		m_pImage=NULL;
		if (m_hwnd!=NULL) {
			Invalidate();
		}
	}
	return true;
}


bool CCapturePreview::HasImage() const
{
	return m_pImage!=NULL;
}


bool CCapturePreview::SetEventHandler(CEventHandler *pEventHandler)
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pCapturePreview=NULL;
	if (pEventHandler!=NULL)
		pEventHandler->m_pCapturePreview=this;
	m_pEventHandler=pEventHandler;
	return true;
}


LRESULT CCapturePreview::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rc;
			BITMAPINFO *pbmi;
			BYTE *pBits;

			::BeginPaint(hwnd,&ps);
			GetClientRect(&rc);
			if (m_pImage!=NULL && m_pImage->LockData(&pbmi,&pBits)) {
				int DstX,DstY,DstWidth,DstHeight;
				RECT rcDest;

				DstWidth=pbmi->bmiHeader.biWidth*rc.bottom/abs(pbmi->bmiHeader.biHeight);
				if (DstWidth>rc.right)
					DstWidth=rc.right;
				DstHeight=pbmi->bmiHeader.biHeight*rc.right/pbmi->bmiHeader.biWidth;
				if (DstHeight>rc.bottom)
					DstHeight=rc.bottom;
				DstX=(rc.right-DstWidth)/2;
				DstY=(rc.bottom-DstHeight)/2;
				if (DstWidth>0 && DstHeight>0) {
					int OldStretchBltMode;

					OldStretchBltMode=::SetStretchBltMode(ps.hdc,STRETCH_HALFTONE);
					::StretchDIBits(ps.hdc,DstX,DstY,DstWidth,DstHeight,
						0,0,pbmi->bmiHeader.biWidth,pbmi->bmiHeader.biHeight,
						pBits,pbmi,DIB_RGB_COLORS,SRCCOPY);
					::SetStretchBltMode(ps.hdc,OldStretchBltMode);
				}
				m_pImage->UnlockData();
				rcDest.left=DstX;
				rcDest.top=DstY;
				rcDest.right=DstX+DstWidth;
				rcDest.bottom=DstY+DstHeight;
				DrawUtil::FillBorder(ps.hdc,&rc,&rcDest,&ps.rcPaint,m_crBackColor);
			} else {
				DrawUtil::Fill(ps.hdc,&rc,m_crBackColor);
			}
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_LBUTTONDOWN:
		if (m_pEventHandler!=NULL)
			m_pEventHandler->OnLButtonDown(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		return TRUE;

	case WM_RBUTTONUP:
		if (m_pEventHandler!=NULL)
			m_pEventHandler->OnRButtonUp(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		return TRUE;

	case WM_KEYDOWN:
		if (m_pEventHandler!=NULL
				&& m_pEventHandler->OnKeyDown((UINT)wParam,(UINT)lParam))
			return 0;
		break;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}




CCaptureWindow::CEventHandler::CEventHandler()
	: m_pCaptureWindow(NULL)
{
}


CCaptureWindow::CEventHandler::~CEventHandler()
{
}




HINSTANCE CCaptureWindow::m_hinst=NULL;


bool CCaptureWindow::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=0;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=(HBRUSH)(COLOR_3DFACE+1);
		wc.lpszMenuName=NULL;
		wc.lpszClassName=CAPTURE_WINDOW_CLASS;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	if (!CCapturePreview::Initialize(hinst))
		return false;
	return true;
}


CCaptureWindow::CCaptureWindow()
	: m_PreviewEventHandler(this)
	, m_fShowStatusBar(true)
	, m_pImage(NULL)
	, m_pEventHandler(NULL)
	, m_fCreateFirst(true)
{
	m_WindowPosition.Width=320;
	m_WindowPosition.Height=240;
}


CCaptureWindow::~CCaptureWindow()
{
	Destroy();

	delete m_pImage;
}


bool CCaptureWindow::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	if (m_fCreateFirst) {
		if (m_pEventHandler!=NULL)
			m_pEventHandler->OnRestoreSettings();
		m_fCreateFirst=false;
	}

	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 CAPTURE_WINDOW_CLASS,CAPTURE_TITLE_TEXT,m_hinst);
}


void CCaptureWindow::SetTheme(const TVTest::Theme::CThemeManager *pThemeManager)
{
	m_Status.SetTheme(pThemeManager);
}


bool CCaptureWindow::SetImage(const BITMAPINFO *pbmi,const void *pBits)
{
	ClearImage();
	m_pImage=new CCaptureImage(pbmi,pBits);
	if (m_hwnd!=NULL) {
		m_Preview.SetImage(m_pImage);
		SetTitle();
	}
	return true;
}


bool CCaptureWindow::SetImage(CCaptureImage *pImage)
{
	ClearImage();
	m_pImage=pImage;
	if (m_hwnd!=NULL) {
		m_Preview.SetImage(m_pImage);
		SetTitle();
	}
	return true;
}


bool CCaptureWindow::ClearImage()
{
	if (m_pImage!=NULL) {
		delete m_pImage;
		m_pImage=NULL;
		if (m_hwnd!=NULL) {
			m_Preview.ClearImage();
			Invalidate();
			SetTitle();
		}
	}
	return true;
}


bool CCaptureWindow::HasImage() const
{
	return m_pImage!=NULL;
}


bool CCaptureWindow::SetEventHandler(CEventHandler *pEventHandler)
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pCaptureWindow=NULL;
	if (pEventHandler!=NULL)
		pEventHandler->m_pCaptureWindow=this;
	m_pEventHandler=pEventHandler;
	return true;
}


void CCaptureWindow::ShowStatusBar(bool fShow)
{
	if (m_fShowStatusBar!=fShow) {
		m_fShowStatusBar=fShow;
		if (m_hwnd!=NULL) {
			SendSizeMessage();
			m_Status.SetVisible(fShow);
		}
	}
}


void CCaptureWindow::SetTitle()
{
	if (m_hwnd!=NULL) {
		TCHAR szTitle[64];

		::lstrcpy(szTitle,CAPTURE_TITLE_TEXT);
		if (m_pImage!=NULL) {
			BITMAPINFOHEADER bmih;

			if (m_pImage->GetBitmapInfoHeader(&bmih)) {
				StdUtil::snprintf(szTitle,lengthof(szTitle),
								  CAPTURE_TITLE_TEXT TEXT(" - %d x %d (%d bpp)"),
								  bmih.biWidth,abs(bmih.biHeight),bmih.biBitCount);
			}
		}
		::SetWindowText(m_hwnd,szTitle);
	}
}


LRESULT CCaptureWindow::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		m_Preview.Create(hwnd,WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,WS_EX_CLIENTEDGE);
		m_Preview.SetEventHandler(&m_PreviewEventHandler);
		m_Status.Create(hwnd,
						WS_CHILD | WS_CLIPSIBLINGS | (m_fShowStatusBar?WS_VISIBLE:0),
						/*WS_EX_STATICEDGE*/0);
		//m_Status.SetEventHandler(pThis);
		if (m_Status.NumItems()==0) {
			if (!m_StatusIcons.IsCreated()) {
				m_StatusIcons.Load(GetAppClass().GetResourceInstance(),IDB_CAPTURE,16,16);
			}
			m_Status.AddItem(new CCaptureStatusItem(m_StatusIcons));
			//m_Status.AddItem(new CContinuousStatusItem(m_StatusIcons));
			m_Status.AddItem(new CSaveStatusItem(this,m_StatusIcons));
			m_Status.AddItem(new CCopyStatusItem(this,m_StatusIcons));
		}
		if (m_pImage!=NULL) {
			m_Preview.SetImage(m_pImage);
			SetTitle();
		}
		return 0;

	case WM_SIZE:
		{
			int Width=LOWORD(lParam),Height=HIWORD(lParam);

			if (m_fShowStatusBar) {
				Height-=m_Status.GetHeight();
				m_Status.SetPosition(0,Height,Width,m_Status.GetHeight());
			}
			m_Preview.SetPosition(0,0,Width,Height);
		}
		return 0;

	case WM_KEYDOWN:
		if (m_pEventHandler!=NULL
				&& m_pEventHandler->OnKeyDown((UINT)wParam,(UINT)lParam))
			return 0;
		break;

	case WM_ACTIVATE:
		if (m_pEventHandler!=NULL
				&& m_pEventHandler->OnActivate(LOWORD(wParam)!=WA_INACTIVE))
			return 0;
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case CM_SAVEIMAGE:
			if (m_pImage!=NULL && m_pEventHandler!=NULL) {
				if (!m_pEventHandler->OnSave(m_pImage)) {
					::MessageBox(hwnd,TEXT("画像の保存ができません。"),NULL,
								 MB_OK | MB_ICONEXCLAMATION);
				}
			}
			return 0;

		case CM_COPY:
			if (m_pImage!=NULL) {
				if (!m_pImage->SetClipboard(hwnd)) {
					::MessageBox(hwnd,TEXT("クリップボードにデータを設定できません。"),NULL,
								 MB_OK | MB_ICONEXCLAMATION);
				}
			}
			return 0;

		case CM_CAPTURESTATUSBAR:
			ShowStatusBar(!m_fShowStatusBar);
			return 0;
		}
		return 0;

	case WM_CLOSE:
		if (m_pEventHandler!=NULL
				&& !m_pEventHandler->OnClose())
			return 0;
		break;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}




CCaptureWindow::CPreviewEventHandler::CPreviewEventHandler(CCaptureWindow *pCaptureWindow)
	: m_pCaptureWindow(pCaptureWindow)
{
}


void CCaptureWindow::CPreviewEventHandler::OnRButtonUp(int x,int y)
{
	CPopupMenu Menu(GetAppClass().GetResourceInstance(),IDM_CAPTUREPREVIEW);

	Menu.EnableItem(CM_COPY,m_pCaptureWindow->HasImage());
	Menu.EnableItem(CM_SAVEIMAGE,m_pCaptureWindow->HasImage());
	Menu.CheckItem(CM_CAPTURESTATUSBAR,m_pCaptureWindow->IsStatusBarVisible());
	POINT pt={x,y};
	::ClientToScreen(m_pCapturePreview->GetHandle(),&pt);
	Menu.Show(m_pCaptureWindow->GetHandle(),&pt,TPM_RIGHTBUTTON);
}


bool CCaptureWindow::CPreviewEventHandler::OnKeyDown(UINT KeyCode,UINT Flags)
{
	::SendMessage(m_pCaptureWindow->GetHandle(),WM_KEYDOWN,KeyCode,Flags);
	return true;
}




CCaptureWindow::CCaptureStatusItem::CCaptureStatusItem(DrawUtil::CMonoColorIconList &Icons)
	: CIconStatusItem(STATUS_ITEM_CAPTURE,16)
	, m_Icons(Icons)
{
}

void CCaptureWindow::CCaptureStatusItem::Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags)
{
	DrawIcon(hdc,DrawRect,m_Icons,CAPTURE_ICON_CAPTURE);
}

void CCaptureWindow::CCaptureStatusItem::OnLButtonDown(int x,int y)
{
	GetAppClass().UICore.DoCommand(CM_CAPTURE);
}

void CCaptureWindow::CCaptureStatusItem::OnRButtonDown(int x,int y)
{
	/*
	POINT pt;
	UINT Flags;

	GetMenuPos(&pt,&Flags);
	GetAppClass().UICore.ShowSpecialMenu(CUICore::MENU_CAPTURE,
										 &pt,Flags | TPM_RIGHTBUTTON);
	*/
}


CCaptureWindow::CSaveStatusItem::CSaveStatusItem(CCaptureWindow *pCaptureWindow,
												 DrawUtil::CMonoColorIconList &Icons)
	: CIconStatusItem(STATUS_ITEM_SAVE,16)
	, m_pCaptureWindow(pCaptureWindow)
	, m_Icons(Icons)
{
}

void CCaptureWindow::CSaveStatusItem::Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags)
{
	DrawIcon(hdc,DrawRect,m_Icons,CAPTURE_ICON_SAVE);
}

void CCaptureWindow::CSaveStatusItem::OnLButtonDown(int x,int y)
{
	m_pCaptureWindow->SendMessage(WM_COMMAND,CM_SAVEIMAGE,0);
}


CCaptureWindow::CCopyStatusItem::CCopyStatusItem(CCaptureWindow *pCaptureWindow,
												 DrawUtil::CMonoColorIconList &Icons)
	: CIconStatusItem(STATUS_ITEM_COPY,16)
	, m_pCaptureWindow(pCaptureWindow)
	, m_Icons(Icons)
{
}

void CCaptureWindow::CCopyStatusItem::Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags)
{
	DrawIcon(hdc,DrawRect,m_Icons,CAPTURE_ICON_COPY);
}

void CCaptureWindow::CCopyStatusItem::OnLButtonDown(int x,int y)
{
	m_pCaptureWindow->SendMessage(WM_COMMAND,CM_COPY,0);
}




/*
CImageSaveThread::CImageSaveThread(CCaptureImage *pImage,LPCTSTR pszFileName,int Format,
										LPCTSTR pszOption,LPCTSTR pszComment)
{
	m_pImage=new CCaptureImage(*pImage);
	m_pszFileName=DuplicateString(pszFileName);
	m_Format=Format;
	m_pszOptions=DuplicateString(pszOption);
	m_pszComment=DuplicateString(pszComment);
}


~CImageSaveThread::CImageSaveThread()
{
	delete m_pImage;
	delete [] m_pszFileName;
	delete [] m_pszOptions;
	delete [] m_pszComment;
}


bool CImageSaveThread::BeginSave()
{
	_beginthread(SaveProc,0,this);
	return true;
}


void CImageSaveThread::SaveProc(void *pParam)
{
	CImageSaveThread *pThis=static_cast<CImageSaveThread*>(pParam);
	BITMAPINFO *pbmi;
	BYTE *pBits;
	bool fResult=false;

	if (pThis->m_pImage->LockData(&pbmi,&pBits)) {
		fResult=m_ImageCodec.SaveImage(pThis->m_pszFileName,pThis->m_Format,
							pThis->m_pszOption,pbmi,pBits,pThis->m_pszComment);
		pThis->m_pImage->UnlockData();
	}
	delete pThis;
	_endthread();
}
*/
