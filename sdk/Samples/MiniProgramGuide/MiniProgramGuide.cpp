/*
	TVTest プラグインサンプル

	簡易番組表を表示する

	このサンプルでは主に以下の機能を実装しています。

	・チャンネルを列挙する
	・番組情報を取得する
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
#include <vector>

#define TVTEST_PLUGIN_CLASS_IMPLEMENT // クラスとして実装
#include "TVTestPlugin.h"


// プラグインクラス
class CMiniProgramGuide : public TVTest::CTVTestPlugin
{
	struct Position
	{
		int Left = 0, Top = 0, Width = 0, Height = 0;
	};

	// ウィンドウクラス名
	static const LPCTSTR MINI_PROGRAM_GUIDE_WINDOW_CLASS;

	// コントロールの識別子
	static constexpr int IDC_TUNERLIST   = 100;
	static constexpr int IDC_CHANNELLIST = 101;
	static constexpr int IDC_EVENTLIST   = 102;

	// 番組あたりの行数
	static constexpr int LINES_PER_EVENT = 3;

	HWND m_hwnd = nullptr;
	HWND m_hwndTunerList = nullptr;
	HWND m_hwndChannelList = nullptr;
	HWND m_hwndEventList = nullptr;
	Position m_WindowPosition;
	COLORREF m_crBackColor;
	COLORREF m_crTextColor;
	HBRUSH m_hbrBackground = nullptr;
	HFONT m_hfont = nullptr;
	int m_DPI;
	int m_FontHeight;
	int m_ItemMargin;
	int m_ItemHeight;
	TVTest::EpgEventList m_EventList{};

	bool Enable(bool fEnable);
	void SetTunerList();
	void SetChannelList();
	void SetEventList();
	void GetColors();
	void CalcMetrics();
	void SetControlsFont();

	static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);
	static CMiniProgramGuide * GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	bool GetPluginInfo(TVTest::PluginInfo *pInfo) override;
	bool Initialize() override;
	bool Finalize() override;
};


const LPCTSTR CMiniProgramGuide::MINI_PROGRAM_GUIDE_WINDOW_CLASS = TEXT("TV Mini Program Guide Window");


// プラグインの情報を返す
bool CMiniProgramGuide::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"ミニ番組表";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"小さい番組表を表示します。";
	return true;
}


// 初期化処理
bool CMiniProgramGuide::Initialize()
{
	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(EventCallback, this);

	return true;
}


// 終了処理
bool CMiniProgramGuide::Finalize()
{
	// ウィンドウを破棄する
	if (m_hwnd)
		::DestroyWindow(m_hwnd);

	return true;
}


// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CMiniProgramGuide::EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
{
	CMiniProgramGuide *pThis = static_cast<CMiniProgramGuide *>(pClientData);

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
		if (pThis->m_hwndEventList != nullptr) {
			pThis->GetColors();
			::RedrawWindow(pThis->m_hwndEventList, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
		}
		return TRUE;

	case TVTest::EVENT_DRIVERCHANGE:
		// BonDriver が変わった
		if (pThis->m_hwnd != nullptr) {
			pThis->SetTunerList();
			pThis->SetChannelList();
			pThis->SetEventList();
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
bool CMiniProgramGuide::Enable(bool fEnable)
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
			wc.hIcon         = nullptr;
			wc.hCursor       = ::LoadCursor(nullptr,IDC_ARROW);
			wc.hbrBackground = nullptr;
			wc.lpszMenuName  = nullptr;
			wc.lpszClassName = MINI_PROGRAM_GUIDE_WINDOW_CLASS;
			if (::RegisterClass(&wc) == 0)
				return false;
			fInitialized = true;
		}

		if (m_hwnd == nullptr) {
			// ウィンドウの作成
			const DWORD Style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
			const DWORD ExStyle = WS_EX_TOOLWINDOW;
			if (::CreateWindowEx(
					ExStyle, MINI_PROGRAM_GUIDE_WINDOW_CLASS, TEXT("ミニ番組表"), Style,
					0, 0, 320, 320, m_pApp->GetAppWindow(), nullptr, g_hinstDLL, this) == nullptr)
				return false;

			// デフォルトサイズの計算
			if (m_WindowPosition.Width <= 0 || m_WindowPosition.Height <= 0) {
				RECT rcList;
				::GetWindowRect(m_hwndEventList, &rcList);
				::MapWindowPoints(nullptr, m_hwnd, reinterpret_cast<POINT *>(&rcList), 2);
				RECT rc;
				rc.left = 0;
				rc.top = 0;
				rc.right = (m_FontHeight * 15) + ::GetSystemMetrics(SM_CXVSCROLL);
				rc.bottom = rcList.top + (m_ItemHeight * 5);
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


// チューニング空間のリストを設定する
void CMiniProgramGuide::SetTunerList()
{
	::SendMessage(m_hwndTunerList, CB_RESETCONTENT, 0, 0);

	WCHAR szName[64];
	for (int i = 0; m_pApp->GetTuningSpaceName(i, szName, sizeof(szName) / sizeof(WCHAR)) > 0; i++) {
		::SendMessage(m_hwndTunerList, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(szName));
	}

	::SendMessage(m_hwndTunerList, CB_SETCURSEL, m_pApp->GetTuningSpace(), 0);
}


// チャンネルのリストを設定する
void CMiniProgramGuide::SetChannelList()
{
	::SendMessage(m_hwndChannelList, CB_RESETCONTENT, 0, 0);

	int CurTuningSpace = (int)::SendMessage(m_hwndTunerList, CB_GETCURSEL, 0, 0);
	if (CurTuningSpace >= 0) {
		TVTest::ChannelInfo CurChannelInfo,ChannelInfo;

		bool fCurChannel = m_pApp->GetCurrentChannelInfo(&CurChannelInfo);

		int Sel = -1;
		for (int i = 0; m_pApp->GetChannelInfo(CurTuningSpace, i, &ChannelInfo); i++) {
			if ((ChannelInfo.Flags&TVTest::CHANNEL_FLAG_DISABLED) == 0) {
				int Index = (int)::SendMessage(
					m_hwndChannelList, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(ChannelInfo.szChannelName));
				::SendMessage(m_hwndChannelList, CB_SETITEMDATA, Index, i);
				if (Sel < 0 && fCurChannel
						&& ChannelInfo.Channel == CurChannelInfo.Channel
						&& ChannelInfo.ServiceID == CurChannelInfo.ServiceID)
					Sel = Index;
			}
		}

		if (Sel >= 0)
			::SendMessage(m_hwndChannelList, CB_SETCURSEL, Sel, 0);
	}
}


// 番組のリストを設定する
void CMiniProgramGuide::SetEventList()
{
	if (m_EventList.EventList != nullptr)
		m_pApp->FreeEpgEventList(&m_EventList);

	::SendMessage(m_hwndEventList, LB_RESETCONTENT, 0, 0);

	int TuningSpace = (int)::SendMessage(m_hwndTunerList, CB_GETCURSEL, 0, 0);
	int Channel = (int)::SendMessage(m_hwndChannelList, CB_GETCURSEL, 0, 0);
	if (TuningSpace >= 0 && Channel >= 0) {
		TVTest::ChannelInfo ChannelInfo;

		Channel = (int)::SendMessage(m_hwndChannelList, CB_GETITEMDATA, Channel, 0);
		if (m_pApp->GetChannelInfo(TuningSpace, Channel, &ChannelInfo)) {
			m_EventList.NetworkID = ChannelInfo.NetworkID;
			m_EventList.TransportStreamID = ChannelInfo.TransportStreamID;
			m_EventList.ServiceID = ChannelInfo.ServiceID;
			if (m_pApp->GetEpgEventList(&m_EventList)) {
				for (WORD i = 0; i < m_EventList.NumEvents; i++) {
					::SendMessage(
						m_hwndEventList, LB_ADDSTRING, 0,
						reinterpret_cast<LPARAM>(m_EventList.EventList[i]));
				}
			}
		}
	}
}


// 配色を取得する
void CMiniProgramGuide::GetColors()
{
	m_crBackColor = m_pApp->GetColor(L"ProgramGuideBack");
	m_crTextColor = m_pApp->GetColor(L"ProgramGuideText");

	if (m_hbrBackground != nullptr)
		::DeleteObject(m_hbrBackground);
	m_hbrBackground = ::CreateSolidBrush(m_crBackColor);
}


// 寸法を計算する
void CMiniProgramGuide::CalcMetrics()
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

	m_FontHeight = tm.tmHeight;
	m_ItemMargin = ::MulDiv(2, m_DPI, 96);
	m_ItemHeight = ((m_FontHeight + tm.tmExternalLeading) * LINES_PER_EVENT) + (m_ItemMargin * 2);
}


void CMiniProgramGuide::SetControlsFont()
{
	::SendMessage(m_hwndTunerList, WM_SETFONT, reinterpret_cast<WPARAM>(m_hfont), 0);
	::SendMessage(m_hwndChannelList, WM_SETFONT, reinterpret_cast<WPARAM>(m_hfont), 0);
	::SendMessage(m_hwndEventList, WM_SETFONT, reinterpret_cast<WPARAM>(m_hfont), 0);
	::SendMessage(m_hwndEventList, LB_SETITEMHEIGHT, 0, m_ItemHeight);
}


// ウィンドウハンドルからthisを取得する
CMiniProgramGuide * CMiniProgramGuide::GetThis(HWND hwnd)
{
	return reinterpret_cast<CMiniProgramGuide *>(::GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


// ウィンドウプロシージャ
LRESULT CALLBACK CMiniProgramGuide::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
			CMiniProgramGuide *pThis = static_cast<CMiniProgramGuide *>(pcs->lpCreateParams);

			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
			pThis->m_hwnd = hwnd;

			pThis->m_DPI = pThis->m_pApp->GetDPIFromWindow(hwnd);
			if (pThis->m_DPI == 0)
				pThis->m_DPI = 96;

			pThis->CalcMetrics();

			pThis->m_hwndTunerList = ::CreateWindowEx(
				0, TEXT("COMBOBOX"), nullptr,
				WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
				0, 0, 0, pThis->m_FontHeight * 20,
				hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_TUNERLIST)), g_hinstDLL, nullptr);

			pThis->m_hwndChannelList = ::CreateWindowEx(
				0, TEXT("COMBOBOX"), nullptr,
				WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
				0, 0, 0, pThis->m_FontHeight * 20,
				hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_CHANNELLIST)), g_hinstDLL, nullptr);

			pThis->m_hwndEventList = ::CreateWindowEx(
				0, TEXT("LISTBOX"), nullptr,
				WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | LBS_OWNERDRAWFIXED | LBS_NOINTEGRALHEIGHT,
				0, 0, 0, 0,
				hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_EVENTLIST)), g_hinstDLL, nullptr);

			pThis->SetControlsFont();
			pThis->GetColors();

			pThis->SetTunerList();
			pThis->SetChannelList();
			pThis->SetEventList();

			// メインウィンドウがダークモードであればそれに合わせてダークモードにする
			if (pThis->m_pApp->GetDarkModeStatus() & TVTest::DARK_MODE_STATUS_MAINWINDOW_DARK)
				pThis->m_pApp->SetWindowDarkMode(hwnd, true);
		}
		return 0;

	case WM_SIZE:
		{
			CMiniProgramGuide *pThis = GetThis(hwnd);
			const int Width = LOWORD(lParam), Height = HIWORD(lParam);
			int y;
			RECT rc;

			::GetWindowRect(pThis->m_hwndTunerList, &rc);
			::MoveWindow(pThis->m_hwndTunerList, 0, 0, Width, rc.bottom - rc.top, TRUE);
			y = rc.bottom - rc.top;
			::GetWindowRect(pThis->m_hwndChannelList, &rc);
			::MoveWindow(pThis->m_hwndChannelList, 0, y, Width, rc.bottom - rc.top, TRUE);
			y += rc.bottom - rc.top;
			::MoveWindow(pThis->m_hwndEventList, 0, y, Width, std::max(Height - y, 0), TRUE);
			::InvalidateRect(pThis->m_hwndEventList, nullptr, TRUE);
		}
		return 0;

	case WM_DRAWITEM:
		// 番組のリストのアイテムを描画
		{
			CMiniProgramGuide *pThis = GetThis(hwnd);
			LPDRAWITEMSTRUCT pdis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

			if ((int)pdis->itemID < 0 || pdis->itemID >= pThis->m_EventList.NumEvents) {
				::FillRect(pdis->hDC, &pdis->rcItem, pThis->m_hbrBackground);
				return TRUE;
			}

			const TVTest::EpgEventInfo *pEventInfo = reinterpret_cast<TVTest::EpgEventInfo*>(pdis->itemData);

			int OldBkMode = ::SetBkMode(pdis->hDC, TRANSPARENT);
			COLORREF OldTextColor = ::SetTextColor(pdis->hDC, pThis->m_crTextColor);

			static const LPCWSTR ContentColorList[] = {
				L"EPGContentNews",
				L"EPGContentSports",
				L"EPGContentInformation",
				L"EPGContentDrama",
				L"EPGContentMusic",
				L"EPGContentVariety",
				L"EPGContentMovie",
				L"EPGContentAnime",
				L"EPGContentDocumentary",
				L"EPGContentTheater",
				L"EPGContentEducation",
				L"EPGContentWelfare",
			};
			COLORREF cr;
			if (pEventInfo->ContentListLength > 0
					&& pEventInfo->ContentList[0].ContentNibbleLevel1 <= 11)
				cr=pThis->m_pApp->GetColor(ContentColorList[pEventInfo->ContentList[0].ContentNibbleLevel1]);
			else
				cr=pThis->m_pApp->GetColor(L"EPGContentOther");
			HBRUSH hbr = ::CreateSolidBrush(cr);
			::FillRect(pdis->hDC, &pdis->rcItem, hbr);
			::DeleteObject(hbr);

			RECT rc = pdis->rcItem;
			::InflateRect(&rc, -pThis->m_ItemMargin, -pThis->m_ItemMargin);

			// EPG 日時から表示用日時に変換
			SYSTEMTIME StartTime;
			if (!pThis->m_pApp->ConvertEpgTimeTo(
					pEventInfo->StartTime, TVTest::CONVERT_TIME_TYPE_EPG_DISPLAY, &StartTime))
				StartTime = pEventInfo->StartTime;

			TCHAR szText[256];
			::wsprintf(
				szText, TEXT("%d/%02d/%02d %02d:%02d %s"),
				StartTime.wYear,
				StartTime.wMonth,
				StartTime.wDay,
				StartTime.wHour,
				StartTime.wMinute,
				pEventInfo->pszEventName != nullptr?
				pEventInfo->pszEventName : TEXT(""));

			::DrawText(pdis->hDC, szText, -1, &rc, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);

			::SetTextColor(pdis->hDC, OldTextColor);
			::SetBkMode(pdis->hDC, OldBkMode);
		}
		return TRUE;

	case WM_CTLCOLORLISTBOX:
		{
			CMiniProgramGuide *pThis = GetThis(hwnd);

			if (reinterpret_cast<HWND>(lParam) == pThis->m_hwndEventList)
				return reinterpret_cast<LRESULT>(pThis->m_hbrBackground);
		}
		break;

	case WM_SYSCOMMAND:
		if ((wParam & 0xFFF0) == SC_CLOSE) {
			// 閉じる時はプラグインを無効にする
			CMiniProgramGuide *pThis = GetThis(hwnd);

			pThis->m_pApp->EnablePlugin(false);
			return TRUE;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_TUNERLIST:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				CMiniProgramGuide *pThis = GetThis(hwnd);

				pThis->SetChannelList();
			}
			return 0;

		case IDC_CHANNELLIST:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				CMiniProgramGuide *pThis = GetThis(hwnd);

				pThis->SetEventList();
			}
			return 0;
		}
		return 0;

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif
	case WM_DPICHANGED:
		// DPI が変わった
		{
			CMiniProgramGuide *pThis = GetThis(hwnd);
			const RECT *prc = reinterpret_cast<const RECT *>(lParam);

			pThis->m_DPI = HIWORD(wParam);

			pThis->CalcMetrics();
			pThis->SetControlsFont();

			::SetWindowPos(
				hwnd, nullptr,
				prc->left, prc->top,
				prc->right - prc->left, prc->bottom - prc->top,
				SWP_NOZORDER | SWP_NOACTIVATE);
			::InvalidateRect(pThis->m_hwndEventList, nullptr, TRUE);
		}
		break;

	case WM_NCDESTROY:
		{
			CMiniProgramGuide *pThis = GetThis(hwnd);

			if (pThis->m_EventList.EventList != nullptr)
				pThis->m_pApp->FreeEpgEventList(&pThis->m_EventList);

			// ウィンドウ位置の記憶
			WINDOWPLACEMENT wp;
			wp.length = sizeof (WINDOWPLACEMENT);
			if (::GetWindowPlacement(hwnd, &wp)) {
				pThis->m_WindowPosition.Left = wp.rcNormalPosition.left;
				pThis->m_WindowPosition.Top = wp.rcNormalPosition.top;
				pThis->m_WindowPosition.Width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
				pThis->m_WindowPosition.Height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
			}

			pThis->m_hwnd = nullptr;
			pThis->m_hwndTunerList = nullptr;
			pThis->m_hwndChannelList = nullptr;
			pThis->m_hwndEventList = nullptr;

			if (pThis->m_hbrBackground != nullptr) {
				::DeleteObject(pThis->m_hbrBackground);
				pThis->m_hbrBackground = nullptr;
			}
			if (pThis->m_hfont != nullptr) {
				::DeleteObject(pThis->m_hfont);
				pThis->m_hfont = nullptr;
			}
		}
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}




// プラグインクラスのインスタンスを生成する
TVTest::CTVTestPlugin * CreatePluginClass()
{
	return new CMiniProgramGuide;
}
