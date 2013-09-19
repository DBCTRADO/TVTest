/*
	TVTest プラグインサンプル

	ロゴの一覧を表示する
*/


#include <windows.h>
#include <tchar.h>
#include <vector>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT	// クラスとして実装
#include "TVTestPlugin.h"


// ウィンドウクラス名
#define LOGO_LIST_WINDOW_CLASS TEXT("TV Logo List Window")

#define ITEM_MARGIN	4	// アイテムの余白の大きさ
#define LOGO_MARGIN	4	// ロゴの間の余白の大きさ

// 更新用タイマーの識別子
#define TIMER_UPDATELOGO	1

// ロゴの大きさ
static const struct {
	int Width, Height;
} LogoSizeList[] = {
	{48, 24},	// logo_type 0
	{36, 24},	// logo_type 1
	{48, 27},	// logo_type 2
	{72, 36},	// logo_type 3
	{54, 36},	// logo_type 4
	{64, 36},	// logo_type 5
};


// プラグインクラス
class CLogoList : public TVTest::CTVTestPlugin
{
	HWND m_hwnd;
	HWND m_hwndList;
	struct Position {
		int Left,Top,Width,Height;
		Position() : Left(0), Top(0), Width(0), Height(0) {}
	};
	Position m_WindowPosition;
	COLORREF m_crBackColor;
	COLORREF m_crTextColor;
	int m_ServiceNameWidth;

	class CServiceInfo {
	public:
		TCHAR m_szServiceName[64];
		WORD m_NetworkID;
		WORD m_ServiceID;
		HBITMAP m_hbmLogo[6];
		CServiceInfo(const TVTest::ChannelInfo &ChInfo);
		~CServiceInfo();
	};
	std::vector<CServiceInfo*> m_ServiceList;

	bool Enable(bool fEnable);
	void GetServiceList();
	bool UpdateLogo();
	void ClearServiceList();
	void GetColors();

	static LRESULT CALLBACK EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData);
	static CLogoList *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	CLogoList();
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();
};


CLogoList::CLogoList()
	: m_hwnd(NULL)
	, m_hwndList(NULL)
{
}


// プラグインの情報を返す
bool CLogoList::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = TVTest::PLUGIN_FLAG_DISABLEONSTART;
	pInfo->pszPluginName  = L"局ロゴの一覧";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"局ロゴを一覧表示します。";
	return true;
}


// 初期化処理
bool CLogoList::Initialize()
{
	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(EventCallback, this);

	return true;
}


// 終了処理
bool CLogoList::Finalize()
{
	// ウィンドウを破棄する
	if (m_hwnd)
		::DestroyWindow(m_hwnd);

	return true;
}


// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CLogoList::EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
{
	CLogoList *pThis=static_cast<CLogoList*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// プラグインの有効状態が変化した
		return pThis->Enable(lParam1 != 0);

	case TVTest::EVENT_STANDBY:
		// 待機状態が変化した
		if (pThis->m_pApp->IsPluginEnabled()) {
			// 待機状態の時はウィンドウを隠す
			::ShowWindow(pThis->m_hwnd, lParam1 != 0 ? SW_HIDE : SW_SHOW);
		}
		return TRUE;

	case TVTest::EVENT_COLORCHANGE:
		// 色の設定が変化した
		if (pThis->m_hwndList != NULL) {
			pThis->GetColors();
			::RedrawWindow(pThis->m_hwndList, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
		}
		return TRUE;
	}
	return 0;
}


// 有効状態が変わった時の処理
bool CLogoList::Enable(bool fEnable)
{
	if (fEnable) {
		static bool fInitialized = false;

		if (!fInitialized) {
			// ウィンドウクラスの登録
			WNDCLASS wc;

			wc.style         = 0;
			wc.lpfnWndProc   = WndProc;
			wc.cbClsExtra    = 0;
			wc.cbWndExtra    = 0;
			wc.hInstance     = g_hinstDLL;
			wc.hIcon         = NULL;
			wc.hCursor       = ::LoadCursor(NULL,IDC_ARROW);
			wc.hbrBackground = NULL;
			wc.lpszMenuName  = NULL;
			wc.lpszClassName = LOGO_LIST_WINDOW_CLASS;
			if (::RegisterClass(&wc) == 0)
				return false;
			fInitialized = true;
		}

		if (m_hwnd == NULL) {
			// ウィンドウの作成
			const DWORD Style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
			const DWORD ExStyle = WS_EX_TOOLWINDOW;
			if (::CreateWindowEx(ExStyle, LOGO_LIST_WINDOW_CLASS,
								 TEXT("局ロゴの一覧"), Style,
								 m_WindowPosition.Left, m_WindowPosition.Top,
								 m_WindowPosition.Width, m_WindowPosition.Height,
								 m_pApp->GetAppWindow(), NULL, g_hinstDLL, this) == NULL)
				return false;

			// 初期サイズの設定
			if (m_WindowPosition.Width == 0) {
				RECT rc;
				::SetRect(&rc, 0, 0,
						  (LONG)::SendMessage(m_hwndList, LB_GETHORIZONTALEXTENT, 0, 0) + ::GetSystemMetrics(SM_CXVSCROLL),
						  (LONG)::SendMessage(m_hwndList, LB_GETITEMHEIGHT, 0, 0) * 8);
				::AdjustWindowRectEx(&rc, Style, FALSE, ExStyle);
				::SetWindowPos(m_hwnd, NULL, 0, 0,
							   rc.right - rc.left, rc.bottom - rc.top,
							   SWP_NOMOVE | SWP_NOZORDER);
			}
		}

		::ShowWindow(m_hwnd, SW_SHOWNORMAL);
	} else {
		// ウィンドウの破棄
		if (m_hwnd)
			::DestroyWindow(m_hwnd);
	}

	return true;
}


// 各サービスのロゴの取得
void CLogoList::GetServiceList()
{
	ClearServiceList();

	// サービスのリストを取得する
	int NumSpaces = 0;
	int CurTuningSpace = m_pApp->GetTuningSpace(&NumSpaces);

	TVTest::ChannelInfo ChInfo;
	CServiceInfo *pServiceInfo;
	if (CurTuningSpace >= 0) {
		// 現在のチューニング空間のチャンネルを取得する
		for (int Channel = 0; m_pApp->GetChannelInfo(CurTuningSpace, Channel, &ChInfo); Channel++) {
			pServiceInfo = new CServiceInfo(ChInfo);
			m_ServiceList.push_back(pServiceInfo);
		}
	} else {
		// 全てのチューニング空間のチャンネルを取得する
		for (int Space = 0; Space < NumSpaces; Space++) {
			for (int Channel = 0; m_pApp->GetChannelInfo(Space, Channel, &ChInfo); Channel++) {
				pServiceInfo = new CServiceInfo(ChInfo);
				m_ServiceList.push_back(pServiceInfo);
			}
		}
	}

	// ロゴを取得する
	UpdateLogo();
}


// ロゴの更新
bool CLogoList::UpdateLogo()
{
	bool fUpdated = false;

	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		CServiceInfo *pServiceInfo = m_ServiceList[i];

		UINT ExistsType = 0;
		for (BYTE j = 0; j < 6; j++) {
			if (pServiceInfo->m_hbmLogo[j] != NULL)
				ExistsType |= 1U << j;
		}
		if ((ExistsType & 0x3F) != 0x3F) {
			// まだ取得していないロゴがある
			UINT AvailableType =
				m_pApp->GetAvailableLogoType(pServiceInfo->m_NetworkID, pServiceInfo->m_ServiceID);
			if (AvailableType != ExistsType) {
				// 新しくロゴが取得されたので更新する
				for (BYTE j = 0; j < 6; j++) {
					if (pServiceInfo->m_hbmLogo[j] == NULL
							&& (AvailableType & (1U << j)) != 0) {
						pServiceInfo->m_hbmLogo[j] =
							m_pApp->GetLogo(pServiceInfo->m_NetworkID, pServiceInfo->m_ServiceID, j);
						if (pServiceInfo->m_hbmLogo[j] != NULL)
							fUpdated = true;
					}
				}
			}
		}
	}
	return fUpdated;
}


// サービスのリストをクリアする
void CLogoList::ClearServiceList()
{
	for (size_t i = 0; i < m_ServiceList.size(); i++)
		delete m_ServiceList[i];
	m_ServiceList.clear();
}


// 配色を取得する
void CLogoList::GetColors()
{
	m_crBackColor = m_pApp->GetColor(L"PanelBack");
	m_crTextColor = m_pApp->GetColor(L"PanelText");
}


// ウィンドウハンドルからthisを取得する
CLogoList *CLogoList::GetThis(HWND hwnd)
{
	return reinterpret_cast<CLogoList*>(::GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


// ウィンドウプロシージャ
LRESULT CALLBACK CLogoList::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
			CLogoList *pThis = static_cast<CLogoList*>(pcs->lpCreateParams);

			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
			pThis->m_hwnd = hwnd;

			pThis->m_hwndList = ::CreateWindowEx(0, TEXT("LISTBOX"), NULL,
				WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | LBS_OWNERDRAWFIXED | LBS_NOINTEGRALHEIGHT,
				0, 0, 0, 0, hwnd, NULL, g_hinstDLL, NULL);

			pThis->GetColors();

			pThis->GetServiceList();

			// アイテムの大きさを設定する
			pThis->m_ServiceNameWidth = 120;
			::SendMessage(pThis->m_hwndList, LB_SETITEMHEIGHT, 0, 36 + ITEM_MARGIN * 2);
			int Width = pThis->m_ServiceNameWidth + ITEM_MARGIN * 2 + LOGO_MARGIN * 6;
			for (int i = 0; i < 6; i++)
				Width += LogoSizeList[i].Width;
			::SendMessage(pThis->m_hwndList, LB_SETHORIZONTALEXTENT, Width, 0);

			for (size_t i = 0; i < pThis->m_ServiceList.size(); i++)
				::SendMessage(pThis->m_hwndList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(pThis->m_ServiceList[i]));

			// 更新用タイマー設定
			::SetTimer(hwnd, TIMER_UPDATELOGO, 60 * 1000, NULL);
		}
		return 0;

	case WM_SIZE:
		{
			CLogoList *pThis = GetThis(hwnd);

			::MoveWindow(pThis->m_hwndList, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		}
		return 0;

	case WM_DRAWITEM:
		// ロゴのリストのアイテムを描画
		{
			CLogoList *pThis = GetThis(hwnd);
			LPDRAWITEMSTRUCT pdis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

			HBRUSH hbr = ::CreateSolidBrush(pThis->m_crBackColor);
			::FillRect(pdis->hDC, &pdis->rcItem, hbr);
			::DeleteObject(hbr);
			if ((int)pdis->itemID < 0 || pdis->itemID >= pThis->m_ServiceList.size())
				return TRUE;

			const CServiceInfo *pService = pThis->m_ServiceList[pdis->itemID];

			HFONT hfontOld = static_cast<HFONT>(::SelectObject(pdis->hDC, ::GetStockObject(DEFAULT_GUI_FONT)));
			int OldBkMode = ::SetBkMode(pdis->hDC, TRANSPARENT);
			COLORREF OldTextColor = ::SetTextColor(pdis->hDC, pThis->m_crTextColor);

			RECT rc;

			rc.left = pdis->rcItem.left + ITEM_MARGIN;
			rc.top = pdis->rcItem.top + ITEM_MARGIN;
			rc.right = rc.left + pThis->m_ServiceNameWidth;
			rc.bottom = pdis->rcItem.bottom - ITEM_MARGIN;
			::DrawText(pdis->hDC, pService->m_szServiceName, -1, &rc,
					   DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);

			::SetTextColor(pdis->hDC, OldTextColor);
			::SetBkMode(pdis->hDC, OldBkMode);
			::SelectObject(pdis->hDC, hfontOld);

			HDC hdcMemory = ::CreateCompatibleDC(pdis->hDC);
			HGDIOBJ hOldBitmap = ::GetCurrentObject(hdcMemory, OBJ_BITMAP);
			int x = rc.right + LOGO_MARGIN;
			for (int i = 0; i < 6; i++) {
				if (pService->m_hbmLogo[i] != NULL) {
					::SelectObject(hdcMemory, pService->m_hbmLogo[i]);
					::BitBlt(pdis->hDC,
							 x, rc.top + ((rc.bottom - rc.top) - LogoSizeList[i].Height) / 2,
							 LogoSizeList[i].Width, LogoSizeList[i].Height,
							 hdcMemory, 0, 0, SRCCOPY);
				}
				x += LogoSizeList[i].Width + LOGO_MARGIN;
			}
			if (::GetCurrentObject(hdcMemory, OBJ_BITMAP) != hOldBitmap)
				::SelectObject(hdcMemory, hOldBitmap);
			::DeleteDC(hdcMemory);
		}
		return TRUE;

	case WM_TIMER:
		// ロゴの更新
		if (wParam == TIMER_UPDATELOGO) {
			CLogoList *pThis = GetThis(hwnd);

			if (pThis->UpdateLogo())
				::RedrawWindow(pThis->m_hwndList, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
		}
		return 0;

	case WM_SYSCOMMAND:
		if ((wParam & 0xFFF0) == SC_CLOSE) {
			// 閉じる時はプラグインを無効にする
			CLogoList *pThis = GetThis(hwnd);

			pThis->m_pApp->EnablePlugin(false);
			return TRUE;
		}
		break;

	case WM_NCDESTROY:
		{
			CLogoList *pThis = GetThis(hwnd);

			// ウィンドウ位置の記憶
			RECT rc;
			::GetWindowRect(hwnd, &rc);
			pThis->m_WindowPosition.Left = rc.left;
			pThis->m_WindowPosition.Top = rc.top;
			pThis->m_WindowPosition.Width = rc.right - rc.left;
			pThis->m_WindowPosition.Height = rc.bottom - rc.top;

			pThis->m_hwnd = NULL;
			pThis->m_hwndList = NULL;
			pThis->ClearServiceList();
		}
		return 0;
	}
	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}




CLogoList::CServiceInfo::CServiceInfo(const TVTest::ChannelInfo &ChInfo)
{
	::lstrcpy(m_szServiceName, ChInfo.szChannelName);
	m_NetworkID = ChInfo.NetworkID;
	m_ServiceID = ChInfo.ServiceID;
	for (int i = 0; i < 6; i++)
		m_hbmLogo[i] = NULL;
}


CLogoList::CServiceInfo::~CServiceInfo()
{
	for (int i = 0; i < 6; i++) {
		if (m_hbmLogo[i] != NULL)
			::DeleteObject(m_hbmLogo[i]);
	}
}




// プラグインクラスのインスタンスを生成する
TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CLogoList;
}
