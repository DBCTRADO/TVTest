/*
	TVTest プラグインサンプル

	ロゴの一覧を表示する

	このサンプルでは主に以下の機能を実装しています。

	・サービスを列挙する
	・局ロゴを取得する
	・ウィンドウを表示する
	・配色を取得し、配色の変更に追従する
	・DPI に応じてスケーリングする
	・TVTest に合わせてウィンドウをダークモードにする
*/


#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <tchar.h>
#include <algorithm>
#include <memory>
#include <vector>

#define TVTEST_PLUGIN_CLASS_IMPLEMENT // クラスとして実装
#include "TVTestPlugin.h"


// ロゴの大きさ
static const struct {
	int Width, Height;
} LogoSizeList[] = {
	{48, 24}, // logo_type 0
	{36, 24}, // logo_type 1
	{48, 27}, // logo_type 2
	{72, 36}, // logo_type 3
	{54, 36}, // logo_type 4
	{64, 36}, // logo_type 5
};


// プラグインクラス
class CLogoList : public TVTest::CTVTestPlugin
{
	struct Position
	{
		int Left = 0, Top = 0, Width = 0, Height = 0;
	};

	class CServiceInfo
	{
	public:
		TCHAR m_szServiceName[64];
		WORD m_NetworkID;
		WORD m_ServiceID;
		HBITMAP m_hbmLogo[6];
		CServiceInfo(const TVTest::ChannelInfo &ChInfo);
		~CServiceInfo();
	};

	// ウィンドウクラス名
	static const LPCTSTR LOGO_LIST_WINDOW_CLASS;

	// 更新用タイマーの識別子
	static constexpr UINT TIMER_UPDATELOGO = 1;

	bool m_fInitialized = false;
	HWND m_hwnd = nullptr;
	HWND m_hwndList = nullptr;
	Position m_WindowPosition;
	COLORREF m_crBackColor;
	COLORREF m_crTextColor;
	int m_DPI;
	int m_ItemMargin;
	int m_LogoMargin;
	int m_ServiceNameWidth;
	int m_ItemWidth;
	int m_ItemHeight;
	HFONT m_hfont = nullptr;
	HBRUSH m_hbrBack = nullptr;
	std::vector<std::unique_ptr<CServiceInfo>> m_ServiceList;

	bool Enable(bool fEnable);
	void GetServiceList();
	bool UpdateLogo();
	void ClearServiceList();
	void GetColors();
	void CalcMetrics();

	static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);
	static CLogoList * GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	bool GetPluginInfo(TVTest::PluginInfo *pInfo) override;
	bool Initialize() override;
	bool Finalize() override;
};


const LPCTSTR CLogoList::LOGO_LIST_WINDOW_CLASS = TEXT("TV Logo List Window");


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
LRESULT CALLBACK CLogoList::EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
{
	CLogoList *pThis = static_cast<CLogoList *>(pClientData);

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
		if (pThis->m_hwndList != nullptr) {
			pThis->GetColors();
			::RedrawWindow(pThis->m_hwndList, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
		}
		return TRUE;

	case TVTest::EVENT_MAINWINDOWDARKMODECHANGED:
		// メインウィンドウのダークモード状態が変わった
		if (pThis->m_hwnd != nullptr) {
			// メインウィンドウに合わせてダークモード状態を変更する
			pThis->m_pApp->SetWindowDarkMode(
				pThis->m_hwnd,
				(pThis->m_pApp->GetDarkModeStatus() & TVTest::DARK_MODE_STATUS_MAINWINDOW_DARK) != 0);
		}
		return TRUE;
	}

	return 0;
}


// 有効状態が変わった時の処理
bool CLogoList::Enable(bool fEnable)
{
	if (fEnable) {
		if (!m_fInitialized) {
			// ウィンドウクラスの登録
			WNDCLASS wc;

			wc.style         = 0;
			wc.lpfnWndProc   = WndProc;
			wc.cbClsExtra    = 0;
			wc.cbWndExtra    = 0;
			wc.hInstance     = g_hinstDLL;
			wc.hIcon         = nullptr;
			wc.hCursor       = ::LoadCursor(nullptr,IDC_ARROW);
			wc.hbrBackground = nullptr;
			wc.lpszMenuName  = nullptr;
			wc.lpszClassName = LOGO_LIST_WINDOW_CLASS;
			if (::RegisterClass(&wc) == 0)
				return false;
			m_fInitialized = true;
		}

		if (m_hwnd == nullptr) {
			// ウィンドウの作成
			const DWORD Style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
			const DWORD ExStyle = WS_EX_TOOLWINDOW;
			if (::CreateWindowEx(
					ExStyle, LOGO_LIST_WINDOW_CLASS, TEXT("局ロゴの一覧"), Style,
					0, 0, 320, 320, m_pApp->GetAppWindow(), nullptr, g_hinstDLL, this) == nullptr)
				return false;

			// デフォルトサイズの計算
			if (m_WindowPosition.Width <= 0 || m_WindowPosition.Height <= 0) {
				RECT rc;
				rc.left = 0;
				rc.top = 0;
				rc.right = m_ItemWidth + ::GetSystemMetrics(SM_CXVSCROLL);
				rc.bottom = m_ItemHeight * 8;
				::AdjustWindowRectEx(&rc, Style, FALSE, ExStyle);
				if (m_WindowPosition.Width <= 0)
					m_WindowPosition.Width = rc.right - rc.left;
				if (m_WindowPosition.Height <= 0)
					m_WindowPosition.Height = rc.bottom - rc.top;
			}

			// ウィンドウ位置の復元
			WINDOWPLACEMENT wp;
			wp.length = sizeof(WINDOWPLACEMENT);
			::GetWindowPlacement(m_hwnd, &wp);
			wp.flags = 0;
			wp.showCmd = SW_SHOWNOACTIVATE;
			wp.rcNormalPosition.left = m_WindowPosition.Left;
			wp.rcNormalPosition.top = m_WindowPosition.Top;
			wp.rcNormalPosition.right = m_WindowPosition.Left + m_WindowPosition.Width;
			wp.rcNormalPosition.bottom = m_WindowPosition.Top + m_WindowPosition.Height;
			::SetWindowPlacement(m_hwnd, &wp);
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
	if (CurTuningSpace >= 0) {
		// 現在のチューニング空間のチャンネルを取得する
		for (int Channel = 0; m_pApp->GetChannelInfo(CurTuningSpace, Channel, &ChInfo); Channel++) {
			m_ServiceList.emplace_back(std::make_unique<CServiceInfo>(ChInfo));
		}
	} else {
		// 全てのチューニング空間のチャンネルを取得する
		for (int Space = 0; Space < NumSpaces; Space++) {
			for (int Channel = 0; m_pApp->GetChannelInfo(Space, Channel, &ChInfo); Channel++) {
				m_ServiceList.emplace_back(std::make_unique<CServiceInfo>(ChInfo));
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

	for (std::size_t i = 0; i < m_ServiceList.size(); i++) {
		CServiceInfo *pServiceInfo = m_ServiceList[i].get();

		UINT ExistsType = 0;
		for (BYTE j = 0; j < 6; j++) {
			if (pServiceInfo->m_hbmLogo[j] != nullptr)
				ExistsType |= 1U << j;
		}
		if ((ExistsType & 0x3F) != 0x3F) {
			// まだ取得していないロゴがある
			UINT AvailableType =
				m_pApp->GetAvailableLogoType(pServiceInfo->m_NetworkID, pServiceInfo->m_ServiceID);
			if (AvailableType != ExistsType) {
				// 新しくロゴが取得されたので更新する
				for (BYTE j = 0; j < 6; j++) {
					if (pServiceInfo->m_hbmLogo[j] == nullptr
							&& (AvailableType & (1U << j)) != 0) {
						pServiceInfo->m_hbmLogo[j] =
							m_pApp->GetLogo(pServiceInfo->m_NetworkID, pServiceInfo->m_ServiceID, j);
						if (pServiceInfo->m_hbmLogo[j] != nullptr)
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
	m_ServiceList.clear();
}


// 配色を取得する
void CLogoList::GetColors()
{
	m_crBackColor = m_pApp->GetColor(L"PanelBack");
	m_crTextColor = m_pApp->GetColor(L"PanelText");

	if (m_hbrBack != nullptr)
		::DeleteObject(m_hbrBack);
	m_hbrBack = ::CreateSolidBrush(m_crBackColor);

	if (m_pApp->GetDarkModeStatus() & TVTest::DARK_MODE_STATUS_PANEL_SUPPORTED)
		m_pApp->SetWindowDarkMode(m_hwndList, m_pApp->IsDarkModeColor(m_crBackColor));
}


// 寸法を計算する
void CLogoList::CalcMetrics()
{
	LOGFONT lf;
	m_pApp->GetFont(L"PanelFont", &lf, m_DPI);
	if (m_hfont != nullptr)
		::DeleteObject(m_hfont);
	m_hfont = ::CreateFontIndirect(&lf);

	HDC hdc = ::GetDC(m_hwnd);
	HGDIOBJ hOldFont = ::SelectObject(hdc, m_hfont);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	::SelectObject(hdc, hOldFont);
	::ReleaseDC(m_hwnd, hdc);

	m_ItemMargin = ::MulDiv(2, m_DPI, 96);
	m_LogoMargin = ::MulDiv(4, m_DPI, 96);
	m_ServiceNameWidth = tm.tmAveCharWidth * 20;

	int LogoWidth = 0;
	for (int i = 0; i < 6; i++)
		LogoWidth += LogoSizeList[i].Width;
	m_ItemWidth = m_ServiceNameWidth + (m_ItemMargin * 2) + (m_LogoMargin * 6) + LogoWidth;
	m_ItemHeight = std::max<int>(36, tm.tmHeight) + (m_ItemMargin * 2);
}


// ウィンドウハンドルからthisを取得する
CLogoList * CLogoList::GetThis(HWND hwnd)
{
	return reinterpret_cast<CLogoList *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
}


// ウィンドウプロシージャ
LRESULT CALLBACK CLogoList::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
			CLogoList *pThis = static_cast<CLogoList *>(pcs->lpCreateParams);

			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
			pThis->m_hwnd = hwnd;

			pThis->m_DPI = pThis->m_pApp->GetDPIFromWindow(hwnd);
			if (pThis->m_DPI == 0)
				pThis->m_DPI = 96;

			pThis->CalcMetrics();

			pThis->m_hwndList = ::CreateWindowEx(
				0, TEXT("LISTBOX"), nullptr,
				WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | LBS_OWNERDRAWFIXED | LBS_NOINTEGRALHEIGHT,
				0, 0, 0, 0, hwnd, nullptr, g_hinstDLL, nullptr);

			pThis->GetColors();

			pThis->GetServiceList();

			// アイテムの大きさを設定する
			::SendMessage(pThis->m_hwndList, LB_SETITEMHEIGHT, 0, pThis->m_ItemHeight);
			::SendMessage(pThis->m_hwndList, LB_SETHORIZONTALEXTENT, pThis->m_ItemWidth, 0);

			for (std::size_t i = 0; i < pThis->m_ServiceList.size(); i++)
				::SendMessage(pThis->m_hwndList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(pThis->m_ServiceList[i].get()));

			// メインウィンドウがダークモードであればそれに合わせてダークモードにする
			if (pThis->m_pApp->GetDarkModeStatus() & TVTest::DARK_MODE_STATUS_MAINWINDOW_DARK)
				pThis->m_pApp->SetWindowDarkMode(hwnd, true);

			// 更新用タイマー設定
			::SetTimer(hwnd, TIMER_UPDATELOGO, 60 * 1000, nullptr);
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

			::FillRect(pdis->hDC, &pdis->rcItem, pThis->m_hbrBack);
			if ((int)pdis->itemID < 0 || pdis->itemID >= pThis->m_ServiceList.size())
				return TRUE;

			const CServiceInfo *pService = pThis->m_ServiceList[pdis->itemID].get();

			HFONT hfontOld = static_cast<HFONT>(::SelectObject(pdis->hDC, pThis->m_hfont));
			int OldBkMode = ::SetBkMode(pdis->hDC, TRANSPARENT);
			COLORREF OldTextColor = ::SetTextColor(pdis->hDC, pThis->m_crTextColor);

			RECT rc;

			rc.left = pdis->rcItem.left + pThis->m_ItemMargin;
			rc.top = pdis->rcItem.top + pThis->m_ItemMargin;
			rc.right = rc.left + pThis->m_ServiceNameWidth;
			rc.bottom = pdis->rcItem.bottom - pThis->m_ItemMargin;
			::DrawText(
				pdis->hDC, pService->m_szServiceName, -1, &rc,
				DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);

			::SetTextColor(pdis->hDC, OldTextColor);
			::SetBkMode(pdis->hDC, OldBkMode);
			::SelectObject(pdis->hDC, hfontOld);

			HDC hdcMemory = ::CreateCompatibleDC(pdis->hDC);
			HGDIOBJ hOldBitmap = ::GetCurrentObject(hdcMemory, OBJ_BITMAP);
			int x = rc.right + pThis->m_LogoMargin;
			for (int i = 0; i < 6; i++) {
				if (pService->m_hbmLogo[i] != nullptr) {
					::SelectObject(hdcMemory, pService->m_hbmLogo[i]);
					::BitBlt(
						pdis->hDC,
						x, rc.top + ((rc.bottom - rc.top) - LogoSizeList[i].Height) / 2,
						LogoSizeList[i].Width, LogoSizeList[i].Height,
						hdcMemory, 0, 0, SRCCOPY);
				}
				x += LogoSizeList[i].Width + pThis->m_LogoMargin;
			}
			if (::GetCurrentObject(hdcMemory, OBJ_BITMAP) != hOldBitmap)
				::SelectObject(hdcMemory, hOldBitmap);
			::DeleteDC(hdcMemory);
		}
		return TRUE;

	case WM_CTLCOLORLISTBOX:
		{
			CLogoList *pThis = GetThis(hwnd);

			return reinterpret_cast<LRESULT>(pThis->m_hbrBack);
		}

	case WM_TIMER:
		// ロゴの更新
		if (wParam == TIMER_UPDATELOGO) {
			CLogoList *pThis = GetThis(hwnd);

			if (pThis->UpdateLogo())
				::RedrawWindow(pThis->m_hwndList, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
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

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif
	case WM_DPICHANGED:
		// DPI が変わった
		{
			CLogoList *pThis = GetThis(hwnd);
			const RECT *prc = reinterpret_cast<const RECT *>(lParam);

			pThis->m_DPI = HIWORD(wParam);

			pThis->CalcMetrics();

			::SendMessage(pThis->m_hwndList, LB_SETITEMHEIGHT, 0, pThis->m_ItemHeight);
			::SendMessage(pThis->m_hwndList, LB_SETHORIZONTALEXTENT, pThis->m_ItemWidth, 0);

			::SetWindowPos(
				hwnd, nullptr,
				prc->left, prc->top,
				prc->right - prc->left, prc->bottom - prc->top,
				SWP_NOZORDER | SWP_NOACTIVATE);
			::InvalidateRect(pThis->m_hwndList, nullptr, TRUE);
		}
		break;

	case WM_NCDESTROY:
		{
			CLogoList *pThis = GetThis(hwnd);

			// ウィンドウ位置の記憶
			WINDOWPLACEMENT wp;
			wp.length = sizeof (WINDOWPLACEMENT);
			if (::GetWindowPlacement(hwnd, &wp)) {
				pThis->m_WindowPosition.Left = wp.rcNormalPosition.left;
				pThis->m_WindowPosition.Top = wp.rcNormalPosition.top;
				pThis->m_WindowPosition.Width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
				pThis->m_WindowPosition.Height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
			}

			// リソース解放
			if (pThis->m_hfont != nullptr) {
				::DeleteObject(pThis->m_hfont);
				pThis->m_hfont = nullptr;
			}
			if (pThis->m_hbrBack != nullptr) {
				::DeleteObject(pThis->m_hbrBack);
				pThis->m_hbrBack = nullptr;
			}

			pThis->m_hwnd = nullptr;
			pThis->m_hwndList = nullptr;
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
		m_hbmLogo[i] = nullptr;
}


CLogoList::CServiceInfo::~CServiceInfo()
{
	for (int i = 0; i < 6; i++) {
		if (m_hbmLogo[i] != nullptr)
			::DeleteObject(m_hbmLogo[i]);
	}
}




// プラグインクラスのインスタンスを生成する
TVTest::CTVTestPlugin * CreatePluginClass()
{
	return new CLogoList;
}
