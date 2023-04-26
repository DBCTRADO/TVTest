/*
	TVTest プラグインサンプル

	一定時間ごとにキャプチャを行う

	このサンプルでは主に以下の機能を実装しています。

	・キャプチャ画像を保存する
	・設定ダイアログを表示する
*/


#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <tchar.h>

#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"
#include "resource.h"


// プラグインクラス
class CAutoSnapShot : public TVTest::CTVTestPlugin
{
	DWORD m_Interval = 10; // キャプチャ間隔(秒単位)
	HWND m_hwnd = nullptr;
	bool m_fEnabled = false;

	static const LPCTSTR SNAPSHOT_WINDOW_CLASS;

	static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);
	static CAutoSnapShot * GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, void *pClientData);

public:
	bool GetPluginInfo(TVTest::PluginInfo *pInfo) override;
	bool Initialize() override;
	bool Finalize() override;
};


const LPCTSTR CAutoSnapShot::SNAPSHOT_WINDOW_CLASS = TEXT("Auto Snap Shot Window");


bool CAutoSnapShot::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	// プラグインの情報を返す
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = TVTest::PLUGIN_FLAG_HASSETTINGS;
	pInfo->pszPluginName  = L"Auto Snap Shot";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"一定時間ごとにキャプチャする";
	return true;
}


bool CAutoSnapShot::Initialize()
{
	// 初期化処理

	WNDCLASS wc;

	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = g_hinstDLL;
	wc.hIcon = nullptr;
	wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = SNAPSHOT_WINDOW_CLASS;
	if (::RegisterClass(&wc) == 0)
		return false;

	m_hwnd = ::CreateWindowEx(
		0, SNAPSHOT_WINDOW_CLASS, nullptr, WS_POPUP,
		0, 0, 0, 0, m_pApp->GetAppWindow(), nullptr, g_hinstDLL, this);
	if (m_hwnd == nullptr)
		return false;

	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(EventCallback, this);

	return true;
}


bool CAutoSnapShot::Finalize()
{
	// 終了処理

	::DestroyWindow(m_hwnd);

	return true;
}


// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CAutoSnapShot::EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
{
	CAutoSnapShot *pThis = static_cast<CAutoSnapShot *>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// プラグインの有効状態が変化した
		pThis->m_fEnabled = lParam1 != 0;
		if (pThis->m_fEnabled)
			::SetTimer(pThis->m_hwnd, 1, pThis->m_Interval * 1000, nullptr);
		else
			::KillTimer(pThis->m_hwnd, 1);
		return TRUE;

	case TVTest::EVENT_PLUGINSETTINGS:
		// プラグインの設定を行う
		{
			TVTest::ShowDialogInfo Info;

			Info.Flags = 0;
			Info.hinst = g_hinstDLL;
			Info.pszTemplate = MAKEINTRESOURCE(IDD_SETTINGS);
			Info.pMessageFunc = DlgProc;
			Info.pClientData = pThis;
			Info.hwndOwner = reinterpret_cast<HWND>(lParam1);

			return pThis->m_pApp->ShowDialog(&Info) == IDOK;
		}
	}

	return 0;
}


CAutoSnapShot * CAutoSnapShot::GetThis(HWND hwnd)
{
	return reinterpret_cast<CAutoSnapShot *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
}


LRESULT CALLBACK CAutoSnapShot::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
			CAutoSnapShot *pThis = static_cast<CAutoSnapShot *>(pcs->lpCreateParams);

			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
		}
		return TRUE;

	case WM_TIMER:
		{
			CAutoSnapShot *pThis = GetThis(hwnd);

			// キャプチャ実行
			pThis->m_pApp->SaveImage();
		}
		return 0;

	case WM_DESTROY:
		{
			CAutoSnapShot *pThis = GetThis(hwnd);

			if (pThis->m_fEnabled)
				::KillTimer(hwnd, 1); // 別にしなくてもいいけど...
		}
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


// 設定ダイアログプロシージャ
INT_PTR CALLBACK CAutoSnapShot::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, void *pClientData)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CAutoSnapShot *pThis = static_cast<CAutoSnapShot *>(pClientData);

			::SetDlgItemInt(hDlg, IDC_SETTINGS_INTERVAL, pThis->m_Interval, FALSE);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			{
				CAutoSnapShot *pThis = static_cast<CAutoSnapShot *>(pClientData);

				pThis->m_Interval = ::GetDlgItemInt(hDlg, IDC_SETTINGS_INTERVAL, nullptr, FALSE);
				if (pThis->m_fEnabled)
					::SetTimer(pThis->m_hwnd, 1, pThis->m_Interval * 1000, nullptr);
			}
		case IDCANCEL:
			::EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		return TRUE;
	}

	return FALSE;
}




TVTest::CTVTestPlugin * CreatePluginClass()
{
	return new CAutoSnapShot;
}
