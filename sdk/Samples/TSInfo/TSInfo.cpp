/*
	TVTest プラグインサンプル

	ストリームの各種情報を表示する

	このサンプルでは主に以下の機能を実装しています。

	・ダイアログテンプレートを元にウィンドウを表示する
	・チャンネルやサービスの情報を取得する
	・配色を取得し、配色の変更に追従する
*/


#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <tchar.h>

#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"
#include "resource.h"


// プラグインクラス
class CTSInfo : public TVTest::CTVTestPlugin
{
	HWND m_hwnd = nullptr;
	HBRUSH m_hbrBack = nullptr;
	COLORREF m_crTextColor;

	static const LPCTSTR PROP_NAME;

	void SetItemText(int ID,LPCTSTR pszText);
	void UpdateItems();

	static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, void *pClientData);

public:
	bool GetPluginInfo(TVTest::PluginInfo *pInfo) override;
	bool Initialize() override;
	bool Finalize() override;
};


const LPCTSTR CTSInfo::PROP_NAME = TEXT("52058115-8095-444B-B472-0DE1E8AB7A44");


// プラグインの情報を返す
bool CTSInfo::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"チャンネルの情報";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"チャンネルの情報を表示します。";
	return true;
}


// 初期化処理
bool CTSInfo::Initialize()
{
	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(EventCallback, this);

	return true;
}


// 終了処理
bool CTSInfo::Finalize()
{
	// ウィンドウの破棄
	if (m_hwnd != nullptr)
		::DestroyWindow(m_hwnd);

	return true;
}


// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CTSInfo::EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
{
	CTSInfo *pThis = static_cast<CTSInfo *>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// プラグインの有効状態が変化した
		{
			bool fEnable = lParam1 != 0;

			if (fEnable) {
				if (pThis->m_hwnd == nullptr) {
					TVTest::ShowDialogInfo Info;

					Info.Flags = TVTest::SHOW_DIALOG_FLAG_MODELESS;
					Info.hinst = g_hinstDLL;
					Info.pszTemplate = MAKEINTRESOURCE(IDD_MAIN);
					Info.pMessageFunc = DlgProc;
					Info.pClientData = pThis;
					Info.hwndOwner = pThis->m_pApp->GetAppWindow();

					if ((HWND)pThis->m_pApp->ShowDialog(&Info) == nullptr)
						return FALSE;
				}
				pThis->UpdateItems();
			}
			::ShowWindow(pThis->m_hwnd, fEnable ? SW_SHOW : SW_HIDE);
			if (fEnable)
				::SetTimer(pThis->m_hwnd, 1, 1000, nullptr);
			else
				::KillTimer(pThis->m_hwnd, 1);
		}
		return TRUE;

	case TVTest::EVENT_COLORCHANGE:
		// 色の設定が変化した
		if (pThis->m_hwnd != nullptr) {
			HBRUSH hbrBack = ::CreateSolidBrush(pThis->m_pApp->GetColor(L"PanelBack"));

			if (hbrBack != nullptr) {
				if (pThis->m_hbrBack != nullptr)
					::DeleteObject(pThis->m_hbrBack);
				pThis->m_hbrBack = hbrBack;
			}
			pThis->m_crTextColor = pThis->m_pApp->GetColor(L"PanelText");
			::RedrawWindow(pThis->m_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
		}
		return TRUE;
	}

	return 0;
}


// 項目の文字列を設定する
void CTSInfo::SetItemText(int ID, LPCTSTR pszText)
{
	TCHAR szCurText[256];

	// 選択が解除されるのとちらつき防止のために、変化があった時のみ設定する
	::GetDlgItemText(m_hwnd, ID, szCurText, 256);
	if (::lstrcmp(szCurText, pszText) != 0)
		::SetDlgItemText(m_hwnd, ID, pszText);
}


// 項目を更新する
void CTSInfo::UpdateItems()
{
	TVTest::ChannelInfo ChannelInfo;
	int CurService, NumServices;
	TVTest::ServiceInfo ServiceInfo;
	TCHAR szText[256];

	if (m_pApp->GetCurrentChannelInfo(&ChannelInfo)) {
		TCHAR szSpaceName[32];

		if (m_pApp->GetTuningSpaceName(ChannelInfo.Space, szSpaceName, 32) == 0)
			::lstrcpy(szSpaceName, TEXT("???"));
		::wsprintf(szText, TEXT("%d (%s)"), ChannelInfo.Space, szSpaceName);
		SetItemText(IDC_SPACE, szText);
		::wsprintf(szText, TEXT("%d (%s)"), ChannelInfo.Channel, ChannelInfo.szChannelName);
		SetItemText(IDC_CHANNEL, szText);
		::wsprintf(szText, TEXT("%#x"), ChannelInfo.NetworkID);
		SetItemText(IDC_NETWORKID, szText);
		SetItemText(IDC_NETWORKNAME, ChannelInfo.szNetworkName);
		::wsprintf(szText, TEXT("%#x"), ChannelInfo.TransportStreamID);
		SetItemText(IDC_TRANSPORTSTREAMID, szText);
		SetItemText(IDC_TRANSPORTSTREAMNAME, ChannelInfo.szTransportStreamName);
		if (ChannelInfo.RemoteControlKeyID != 0)
			::wsprintf(szText, TEXT("%d"), ChannelInfo.RemoteControlKeyID);
		else
			szText[0] = '\0';
		SetItemText(IDC_REMOTECONTROLKEYID, szText);
	}

	CurService = m_pApp->GetService(&NumServices);
	if (CurService >= 0
			&& m_pApp->GetServiceInfo(CurService, &ServiceInfo)) {
		::wsprintf(szText, TEXT("%d / %d"), CurService + 1, NumServices);
		SetItemText(IDC_SERVICE, szText);
		::wsprintf(szText, TEXT("%#x"), ServiceInfo.ServiceID);
		SetItemText(IDC_SERVICEID, szText);
		SetItemText(IDC_SERVICENAME, ServiceInfo.szServiceName);
		::wsprintf(szText, TEXT("%#x"), ServiceInfo.VideoPID);
		SetItemText(IDC_VIDEOPID, szText);
		::wsprintf(szText, TEXT("%#x"), ServiceInfo.AudioPID[0]);
		if (ServiceInfo.NumAudioPIDs > 1) {
			::wsprintf(szText + ::lstrlen(szText), TEXT(" / %#x"), ServiceInfo.AudioPID[1]);
		}
		SetItemText(IDC_AUDIOPID, szText);
		if (ServiceInfo.SubtitlePID != 0)
			::wsprintf(szText, TEXT("%#x"), ServiceInfo.SubtitlePID);
		else
			::lstrcpy(szText, TEXT("<none>"));
		SetItemText(IDC_SUBTITLEPID, szText);
	}
}


// ダイアログプロシージャ
INT_PTR CALLBACK CTSInfo::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, void *pClientData)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CTSInfo *pThis = static_cast<CTSInfo *>(pClientData);

			pThis->m_hwnd = hDlg;
			pThis->m_hbrBack = ::CreateSolidBrush(pThis->m_pApp->GetColor(L"PanelBack"));
			pThis->m_crTextColor = pThis->m_pApp->GetColor(L"PanelText");
		}
		return TRUE;

	case WM_TIMER:
		{
			// 情報更新
			CTSInfo *pThis = static_cast<CTSInfo *>(pClientData);

			pThis->UpdateItems();
		}
		return TRUE;

	case WM_CTLCOLORSTATIC:
		// 項目の背景色を設定
		{
			CTSInfo *pThis = static_cast<CTSInfo *>(pClientData);
			HDC hdc = reinterpret_cast<HDC>(wParam);

			::SetBkMode(hdc, TRANSPARENT);
			::SetTextColor(hdc, pThis->m_crTextColor);
			return reinterpret_cast<INT_PTR>(pThis->m_hbrBack);
		}

#if 0 // WM_CTLCOLORDLG は Windows 11 で機能しない
	case WM_CTLCOLORDLG:
		// ダイアログの背景色を設定
		{
			CTSInfo *pThis = static_cast<CTSInfo *>(pClientData);

			return reinterpret_cast<INT_PTR>(pThis->m_hbrBack);
		}
#else
	case WM_ERASEBKGND:
		// ダイアログの背景を塗りつぶす
		{
			CTSInfo *pThis = static_cast<CTSInfo *>(pClientData);
			HDC hdc = reinterpret_cast<HDC>(wParam);
			RECT rc;

			::GetClientRect(hDlg, &rc);
			::FillRect(hdc, &rc, pThis->m_hbrBack);
			return TRUE;
		}
#endif

	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL) {
			// 閉じる時はプラグインを無効にする
			CTSInfo *pThis = static_cast<CTSInfo *>(pClientData);

			pThis->m_pApp->EnablePlugin(false);
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			CTSInfo *pThis = static_cast<CTSInfo *>(pClientData);

			::KillTimer(hDlg, 1);
			if (pThis->m_hbrBack != nullptr) {
				::DeleteObject(pThis->m_hbrBack);
				pThis->m_hbrBack = nullptr;
			}
			::RemoveProp(hDlg, PROP_NAME);
		}
		return TRUE;
	}

	return FALSE;
}




// プラグインクラスのインスタンスを生成する
TVTest::CTVTestPlugin * CreatePluginClass()
{
	return new CTSInfo;
}
