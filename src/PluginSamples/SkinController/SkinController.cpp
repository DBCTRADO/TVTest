/*
	TVTest プラグインサンプル

	スキン風コントローラを表示する
	画像は以前にPetitTV用に作ってお蔵入りになったものの流用
*/


#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <shlwapi.h>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"
#include "resource.h"


#pragma comment(lib,"shlwapi.lib")


#define lengthof(a) (sizeof(a)/sizeof(a[0]))

#define SKIN_CONTROLLER_WINDOW_CLASS TEXT("Skin Controller Window")

#define BASE_WIDTH	262
#define BASE_HEIGHT	72
#define TEXT_LEFT	6
#define TEXT_TOP	6
#define TEXT_WIDTH	188
#define TEXT_HEIGHT	34
#define TEXT_COLOR	RGB(192,192,176)

enum Function {
	FUNC_CH1,
	FUNC_CH2,
	FUNC_CH3,
	FUNC_CH4,
	FUNC_CH5,
	FUNC_CH6,
	FUNC_CH7,
	FUNC_CH8,
	FUNC_CH9,
	FUNC_CH10,
	FUNC_CH11,
	FUNC_CH12,
	FUNC_CHDOWN,
	FUNC_CHUP,
	FUNC_RECORD,
	FUNC_STOP,
	FUNC_CAPTURE,
	FUNC_OPTION,
	FUNC_CLOSE
};

struct ButtonInfo {
	Function Func;
	RECT ButtonRect;
};


static const ButtonInfo ButtonList[] = {
	{FUNC_CH1,		{  4, 54,   4+18, 54+18}},
	{FUNC_CH2,		{ 22, 54,  22+18, 54+18}},
	{FUNC_CH3,		{ 40, 54,  40+18, 54+18}},
	{FUNC_CH4,		{ 58, 54,  58+18, 54+18}},
	{FUNC_CH5,		{ 76, 54,  76+18, 54+18}},
	{FUNC_CH6,		{ 94, 54,  94+18, 54+18}},
	{FUNC_CH7,		{112, 54, 112+18, 54+18}},
	{FUNC_CH8,		{130, 54, 130+18, 54+18}},
	{FUNC_CH9,		{148, 54, 148+18, 54+18}},
	{FUNC_CH10,		{166, 54, 166+18, 54+18}},
	{FUNC_CH11,		{184, 54, 184+18, 54+18}},
	{FUNC_CH12,		{202, 54, 202+18, 54+18}},
	{FUNC_CHDOWN,	{222, 54, 222+18, 54+18}},
	{FUNC_CHUP,		{240, 54, 240+18, 54+18}},
	{FUNC_RECORD,	{198, 30, 198+18, 30+18}},
	{FUNC_STOP,		{218, 30, 218+18, 30+18}},
	{FUNC_CAPTURE,	{238, 30, 238+18, 30+18}},
	{FUNC_OPTION,	{198,  4, 198+18,  4+18}},
	{FUNC_CLOSE,	{246,  0, 246+16,  0+16}},
};




// プラグインクラス
class CSkinController : public TVTest::CTVTestPlugin
{
	TCHAR m_szIniFileName[MAX_PATH];
	POINT m_WindowPos;
	HWND m_hwnd;
	HFONT m_hfont;
	HBITMAP m_hbmBase;
	HBITMAP m_hbmPush;
	HBITMAP m_hbmOver;
	int m_CurButton;
	bool m_fTrackMouseEvent;
	static LRESULT CALLBACK EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData);
	static CSkinController *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
public:
	CSkinController()
	{
		m_hwnd=NULL;
	}
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();
};


bool CSkinController::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	// プラグインの情報を返す
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"Skin Controller";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"スキン風コントローラ";
	return true;
}


bool CSkinController::Initialize()
{
	// 初期化処理

	WNDCLASS wc;
	wc.style=0;
	wc.lpfnWndProc=WndProc;
	wc.cbClsExtra=0;
	wc.cbWndExtra=0;
	wc.hInstance=g_hinstDLL;
	wc.hIcon=NULL;
	wc.hCursor=NULL;
	wc.hbrBackground=NULL;
	wc.lpszMenuName=NULL;
	wc.lpszClassName=SKIN_CONTROLLER_WINDOW_CLASS;
	if (::RegisterClass(&wc)==0)
		return false;

	::GetModuleFileName(g_hinstDLL,m_szIniFileName,MAX_PATH);
	::PathRenameExtension(m_szIniFileName,TEXT(".ini"));
	m_WindowPos.x=::GetPrivateProfileInt(TEXT("Settings"),TEXT("WindowLeft"),
										  CW_USEDEFAULT,m_szIniFileName);
	m_WindowPos.y=::GetPrivateProfileInt(TEXT("Settings"),TEXT("WindowTop"),
										 CW_USEDEFAULT,m_szIniFileName);

	if (::CreateWindowEx(0,SKIN_CONTROLLER_WINDOW_CLASS,TEXT("Skin Controller"),
						 WS_POPUP,
						 m_WindowPos.x,m_WindowPos.y,BASE_WIDTH,BASE_HEIGHT,
						 m_pApp->GetAppWindow(),NULL,g_hinstDLL,this)==NULL)
		return false;

	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(EventCallback,this);

	return true;
}


BOOL WritePrivateProfileInt(LPCTSTR pszAppName,LPCTSTR pszKeyName,int Value,LPCTSTR pszFileName)
{
	TCHAR szValue[16];

	wsprintf(szValue,TEXT("%d"),Value);
	return WritePrivateProfileString(pszAppName,pszKeyName,szValue,pszFileName);
}


bool CSkinController::Finalize()
{
	// 終了処理

	::DestroyWindow(m_hwnd);

	::WritePrivateProfileInt(TEXT("Settings"),TEXT("WindowLeft"),m_WindowPos.x,m_szIniFileName);
	::WritePrivateProfileInt(TEXT("Settings"),TEXT("WindowTop"),m_WindowPos.y,m_szIniFileName);

	return true;
}


// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CSkinController::EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
{
	CSkinController *pThis=static_cast<CSkinController*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// プラグインの有効状態が変化した
		::ShowWindow(pThis->m_hwnd,lParam1!=0?SW_SHOW:SW_HIDE);
		return TRUE;

	case TVTest::EVENT_CHANNELCHANGE:
	case TVTest::EVENT_DRIVERCHANGE:
		{
			RECT rc;

			::SetRect(&rc,TEXT_LEFT,TEXT_TOP,TEXT_LEFT+TEXT_WIDTH,TEXT_TOP+TEXT_HEIGHT);
			::InvalidateRect(pThis->m_hwnd,&rc,FALSE);
		}
		return TRUE;

	case TVTest::EVENT_STANDBY:
		// 待機状態が変化した
		if (pThis->m_pApp->IsPluginEnabled()) {
			// 待機状態の時はウィンドウを隠す
			::ShowWindow(pThis->m_hwnd,lParam1!=0?SW_HIDE:SW_SHOW);
		}
		return TRUE;
	}
	return 0;
}


CSkinController *CSkinController::GetThis(HWND hwnd)
{
	return reinterpret_cast<CSkinController*>(::GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


LRESULT CALLBACK CSkinController::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs=reinterpret_cast<LPCREATESTRUCT>(lParam);
			CSkinController *pThis=static_cast<CSkinController*>(pcs->lpCreateParams);

			::SetWindowLongPtr(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(pThis));
			pThis->m_hwnd=hwnd;
			LOGFONT lf;
			::GetObject(::GetStockObject(DEFAULT_GUI_FONT),sizeof(LOGFONT),&lf);
			pThis->m_hfont=::CreateFontIndirect(&lf);
			pThis->m_hbmBase=::LoadBitmap(g_hinstDLL,MAKEINTRESOURCE(IDB_BASE));
			pThis->m_hbmPush=::LoadBitmap(g_hinstDLL,MAKEINTRESOURCE(IDB_PUSH));
			pThis->m_hbmOver=::LoadBitmap(g_hinstDLL,MAKEINTRESOURCE(IDB_OVER));
			pThis->m_CurButton=-1;
			pThis->m_fTrackMouseEvent=false;
		}
		return TRUE;

	case WM_PAINT:
		{
			CSkinController *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;
			HDC hdcMem;
			HBITMAP hbmOld;
			HFONT hfontOld;
			int OldBkMode;
			COLORREF crOldTextColor;
			TVTest::ChannelInfo ChannelInfo;
			TCHAR szText[1024];
			RECT rc;

			::BeginPaint(hwnd,&ps);
			hdcMem=::CreateCompatibleDC(ps.hdc);
			hbmOld=static_cast<HBITMAP>(::SelectObject(hdcMem,pThis->m_hbmBase));
			::BitBlt(ps.hdc,ps.rcPaint.left,ps.rcPaint.top,
					 ps.rcPaint.right-ps.rcPaint.left,
					 ps.rcPaint.bottom-ps.rcPaint.top,
					 hdcMem,ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);
			if (pThis->m_CurButton>=0) {
				::SelectObject(hdcMem,
					::GetKeyState(VK_LBUTTON)<0?pThis->m_hbmPush:pThis->m_hbmOver);
				::BitBlt(ps.hdc,
						 ButtonList[pThis->m_CurButton].ButtonRect.left,
						 ButtonList[pThis->m_CurButton].ButtonRect.top,
						 ButtonList[pThis->m_CurButton].ButtonRect.right-
							ButtonList[pThis->m_CurButton].ButtonRect.left,
						 ButtonList[pThis->m_CurButton].ButtonRect.bottom-
							ButtonList[pThis->m_CurButton].ButtonRect.top,
						 hdcMem,
						 ButtonList[pThis->m_CurButton].ButtonRect.left,
						 ButtonList[pThis->m_CurButton].ButtonRect.top,
						 SRCCOPY);
			}
			::SelectObject(hdcMem,hbmOld);
			::DeleteDC(hdcMem);
			if (pThis->m_pApp->GetCurrentChannelInfo(&ChannelInfo)) {
				hfontOld=static_cast<HFONT>(::SelectObject(ps.hdc,pThis->m_hfont));
				OldBkMode=::SetBkMode(ps.hdc,TRANSPARENT);
				crOldTextColor=::SetTextColor(ps.hdc,TEXT_COLOR);
				::wnsprintf(szText,lengthof(szText),TEXT("%d: %s"),
							ChannelInfo.RemoteControlKeyID,
							ChannelInfo.szChannelName);
				::SetRect(&rc,TEXT_LEFT,TEXT_TOP,TEXT_LEFT+TEXT_WIDTH,TEXT_TOP+TEXT_HEIGHT);
				::DrawText(ps.hdc,szText,-1,&rc,
					   DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);
				::SelectObject(ps.hdc,hfontOld);
				::SetBkMode(ps.hdc,OldBkMode);
				::SetTextColor(ps.hdc,crOldTextColor);
			}
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			CSkinController *pThis=GetThis(hwnd);
			POINT pt;
			int i;

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			for (i=lengthof(ButtonList)-1;i>=0;i--) {
				if (::PtInRect(&ButtonList[i].ButtonRect,pt)) {
					break;
				}
			}
			if (i!=pThis->m_CurButton) {
				if (pThis->m_CurButton>=0)
					::InvalidateRect(hwnd,&ButtonList[pThis->m_CurButton].ButtonRect,FALSE);
				if (i>=0)
					::InvalidateRect(hwnd,&ButtonList[i].ButtonRect,FALSE);
				pThis->m_CurButton=i;
			}
			if (!pThis->m_fTrackMouseEvent) {
				TRACKMOUSEEVENT tme;

				tme.cbSize=sizeof(TRACKMOUSEEVENT);
				tme.dwFlags=TME_LEAVE;
				tme.hwndTrack=hwnd;
				if (::TrackMouseEvent(&tme))
					pThis->m_fTrackMouseEvent=true;
			}
		}
		return 0;

	case WM_MOUSELEAVE:
		{
			CSkinController *pThis=GetThis(hwnd);

			pThis->m_fTrackMouseEvent=false;
			if (pThis->m_CurButton>=0) {
				::InvalidateRect(hwnd,&ButtonList[pThis->m_CurButton].ButtonRect,FALSE);
				pThis->m_CurButton=-1;
			}
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			CSkinController *pThis=GetThis(hwnd);

			if (pThis->m_CurButton>=0) {
				::InvalidateRect(hwnd,&ButtonList[pThis->m_CurButton].ButtonRect,FALSE);
				::UpdateWindow(hwnd);
				switch (ButtonList[pThis->m_CurButton].Func) {
				case FUNC_CHDOWN:
					pThis->m_pApp->SetNextChannel(false);
					break;
				case FUNC_CHUP:
					pThis->m_pApp->SetNextChannel(true);
					break;
				case FUNC_RECORD:
					pThis->m_pApp->StartRecord();
					break;
				case FUNC_STOP:
					pThis->m_pApp->StopRecord();
					break;
				case FUNC_CAPTURE:
					pThis->m_pApp->SaveImage();
					break;
				case FUNC_OPTION:
					pThis->m_pApp->DoCommand(L"Options");
					break;
				case FUNC_CLOSE:
					pThis->m_pApp->Close();
					break;
				default:
					if (ButtonList[pThis->m_CurButton].Func>=FUNC_CH1
							&& ButtonList[pThis->m_CurButton].Func<=FUNC_CH12) {
						WCHAR szCommand[16];

						::wsprintfW(szCommand,L"Channel%d",(ButtonList[pThis->m_CurButton].Func-FUNC_CH1)+1);
						pThis->m_pApp->DoCommand(szCommand);
						break;
					}
				}
			}
		}
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT) {
			CSkinController *pThis=GetThis(hwnd);

			::SetCursor(::LoadCursor(NULL,pThis->m_CurButton>=0?IDC_HAND:IDC_ARROW));
			return TRUE;
		}
		break;

	case WM_NCHITTEST:
		{
			CSkinController *pThis=GetThis(hwnd);
			POINT pt;
			int i;

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			::ScreenToClient(hwnd,&pt);
			for (i=lengthof(ButtonList)-1;i>=0;i--) {
				if (::PtInRect(&ButtonList[i].ButtonRect,pt)) {
					break;
				}
			}
			if (i<0)
				return HTCAPTION;
		}
		break;

	case WM_SYSCOMMAND:
		if ((wParam&0xFFF0)==SC_CLOSE) {
			// 閉じる時はプラグインを無効にする
			CSkinController *pThis=GetThis(hwnd);

			pThis->m_pApp->EnablePlugin(false);
			return 0;
		}
		break;

	case WM_DESTROY:
		{
			CSkinController *pThis=GetThis(hwnd);
			RECT rc;

			// ウィンドウ位置保存
			::GetWindowRect(hwnd,&rc);
			pThis->m_WindowPos.x=rc.left;
			pThis->m_WindowPos.y=rc.top;

			// リソース開放
			::DeleteObject(pThis->m_hfont);
			::DeleteObject(pThis->m_hbmBase);
			::DeleteObject(pThis->m_hbmPush);
			::DeleteObject(pThis->m_hbmOver);
			pThis->m_hwnd=NULL;
		}
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}




TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CSkinController;
}
