/*
	TVTest プラグインサンプル

	ストリームの各種情報を表示する
*/


#include <windows.h>
#include <tchar.h>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"
#include "resource.h"


// プラグインクラス
class CTSInfo : public TVTest::CTVTestPlugin
{
	HWND m_hwnd;
	HBRUSH m_hbrBack;
	COLORREF m_crTextColor;

	void SetItemText(int ID,LPCTSTR pszText);
	void UpdateItems();

	static LRESULT CALLBACK EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData);
	static CTSInfo *GetThis(HWND hDlg);
	static INT_PTR CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	CTSInfo()
		: m_hwnd(NULL)
		, m_hbrBack(NULL)
	{
	}
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();
};


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
	m_pApp->SetEventCallback(EventCallback,this);

	return true;
}


// 終了処理
bool CTSInfo::Finalize()
{
	// ウィンドウの破棄
	if (m_hwnd!=NULL)
		::DestroyWindow(m_hwnd);

	return true;
}


// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CTSInfo::EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
{
	CTSInfo *pThis=static_cast<CTSInfo*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// プラグインの有効状態が変化した
		{
			bool fEnable=lParam1!=0;

			if (fEnable) {
				if (pThis->m_hwnd==NULL) {
					if (::CreateDialogParam(g_hinstDLL,MAKEINTRESOURCE(IDD_MAIN),
											pThis->m_pApp->GetAppWindow(),DlgProc,
											reinterpret_cast<LPARAM>(pThis))==NULL)
						return FALSE;
				}
				pThis->UpdateItems();
			}
			::ShowWindow(pThis->m_hwnd,fEnable?SW_SHOW:SW_HIDE);
			if (fEnable)
				::SetTimer(pThis->m_hwnd,1,1000,NULL);
			else
				::KillTimer(pThis->m_hwnd,1);
		}
		return TRUE;

	case TVTest::EVENT_COLORCHANGE:
		// 色の設定が変化した
		if (pThis->m_hwnd!=NULL) {
			HBRUSH hbrBack=::CreateSolidBrush(pThis->m_pApp->GetColor(L"PanelBack"));

			if (hbrBack!=NULL) {
				if (pThis->m_hbrBack!=NULL)
					::DeleteObject(pThis->m_hbrBack);
				pThis->m_hbrBack=hbrBack;
			}
			pThis->m_crTextColor=pThis->m_pApp->GetColor(L"PanelText");
			::RedrawWindow(pThis->m_hwnd,NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW);
		}
		return TRUE;
	}
	return 0;
}


// 項目の文字列を設定する
void CTSInfo::SetItemText(int ID,LPCTSTR pszText)
{
	TCHAR szCurText[256];

	// 選択が解除されるのとちらつき防止のために、変化があった時のみ設定する
	::GetDlgItemText(m_hwnd,ID,szCurText,256);
	if (::lstrcmp(szCurText,pszText)!=0)
		::SetDlgItemText(m_hwnd,ID,pszText);
}


// 項目を更新する
void CTSInfo::UpdateItems()
{
	TVTest::ChannelInfo ChannelInfo;
	int CurService,NumServices;
	TVTest::ServiceInfo ServiceInfo;
	TCHAR szText[256];

	if (m_pApp->GetCurrentChannelInfo(&ChannelInfo)) {
		TCHAR szSpaceName[32];

		if (m_pApp->GetTuningSpaceName(ChannelInfo.Space,szSpaceName,32)==0)
			::lstrcpy(szSpaceName,TEXT("???"));
		::wsprintf(szText,TEXT("%d (%s)"),ChannelInfo.Space,szSpaceName);
		SetItemText(IDC_SPACE,szText);
		::wsprintf(szText,TEXT("%d (%s)"),ChannelInfo.Channel,ChannelInfo.szChannelName);
		SetItemText(IDC_CHANNEL,szText);
		::wsprintf(szText,TEXT("%#x"),ChannelInfo.NetworkID);
		SetItemText(IDC_NETWORKID,szText);
		SetItemText(IDC_NETWORKNAME,ChannelInfo.szNetworkName);
		::wsprintf(szText,TEXT("%#x"),ChannelInfo.TransportStreamID);
		SetItemText(IDC_TRANSPORTSTREAMID,szText);
		SetItemText(IDC_TRANSPORTSTREAMNAME,ChannelInfo.szTransportStreamName);
		if (ChannelInfo.RemoteControlKeyID!=0)
			::wsprintf(szText,TEXT("%d"),ChannelInfo.RemoteControlKeyID);
		else
			szText[0]='\0';
		SetItemText(IDC_REMOTECONTROLKEYID,szText);
	}
	CurService=m_pApp->GetService(&NumServices);
	if (CurService>=0
			&& m_pApp->GetServiceInfo(CurService,&ServiceInfo)) {
		::wsprintf(szText,TEXT("%d / %d"),CurService+1,NumServices);
		SetItemText(IDC_SERVICE,szText);
		::wsprintf(szText,TEXT("%#x"),ServiceInfo.ServiceID);
		SetItemText(IDC_SERVICEID,szText);
		SetItemText(IDC_SERVICENAME,ServiceInfo.szServiceName);
		::wsprintf(szText,TEXT("%#x"),ServiceInfo.VideoPID);
		SetItemText(IDC_VIDEOPID,szText);
		::wsprintf(szText,TEXT("%#x"),ServiceInfo.AudioPID[0]);
		if (ServiceInfo.NumAudioPIDs>1) {
			::wsprintf(szText+::lstrlen(szText),TEXT(" / %#x"),ServiceInfo.AudioPID[1]);
		}
		SetItemText(IDC_AUDIOPID,szText);
		if (ServiceInfo.SubtitlePID!=0)
			::wsprintf(szText,TEXT("%#x"),ServiceInfo.SubtitlePID);
		else
			::lstrcpy(szText,TEXT("<none>"));
		SetItemText(IDC_SUBTITLEPID,szText);
	}
}


// ダイアログのハンドルからthisを取得する
CTSInfo *CTSInfo::GetThis(HWND hDlg)
{
	return static_cast<CTSInfo*>(::GetProp(hDlg,TEXT("This")));
}


// ダイアログプロシージャ
INT_PTR CALLBACK CTSInfo::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CTSInfo *pThis=reinterpret_cast<CTSInfo*>(lParam);

			::SetProp(hDlg,TEXT("This"),pThis);
			pThis->m_hwnd=hDlg;
			pThis->m_hbrBack=::CreateSolidBrush(pThis->m_pApp->GetColor(L"PanelBack"));
			pThis->m_crTextColor=pThis->m_pApp->GetColor(L"PanelText");
		}
		return TRUE;

	case WM_TIMER:
		{
			// 情報更新
			CTSInfo *pThis=GetThis(hDlg);

			pThis->UpdateItems();
		}
		return TRUE;

	case WM_CTLCOLORSTATIC:
		// 項目の背景色を設定
		{
			CTSInfo *pThis=GetThis(hDlg);
			HDC hdc=reinterpret_cast<HDC>(wParam);

			::SetBkMode(hdc,TRANSPARENT);
			::SetTextColor(hdc,pThis->m_crTextColor);
			return reinterpret_cast<INT_PTR>(pThis->m_hbrBack);
		}

	case WM_CTLCOLORDLG:
		// ダイアログの背景色を設定
		{
			CTSInfo *pThis=GetThis(hDlg);

			return reinterpret_cast<INT_PTR>(pThis->m_hbrBack);
		}

	case WM_COMMAND:
		if (LOWORD(wParam)==IDCANCEL) {
			// 閉じる時はプラグインを無効にする
			CTSInfo *pThis=GetThis(hDlg);

			pThis->m_pApp->EnablePlugin(false);
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			CTSInfo *pThis=GetThis(hDlg);

			::KillTimer(hDlg,1);
			if (pThis->m_hbrBack!=NULL) {
				::DeleteObject(pThis->m_hbrBack);
				pThis->m_hbrBack=NULL;
			}
			::RemoveProp(hDlg,TEXT("This"));
		}
		return TRUE;
	}
	return FALSE;
}




// プラグインクラスのインスタンスを生成する
TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CTSInfo;
}
