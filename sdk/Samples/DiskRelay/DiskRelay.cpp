/*
	TVTest プラグインサンプル

	空きディスク容量が少なくなったら別のフォルダに録画する

	このサンプルでは主に以下の機能を実装しています。

	・録画の状況を取得する
	・録画ファイルを変更する
	・設定ダイアログを表示する
*/


#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <tchar.h>

#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"
#include "resource.h"

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")


// プラグインクラス
class CDiskRelay : public TVTest::CTVTestPlugin
{
	// 予備のフォルダの数
	static constexpr int NUM_SPARE_FOLDERS = 3;

	// ウィンドウクラス名
	static const LPCTSTR DISKRELAY_WINDOW_CLASS;

	// 空き容量を監視するタイマーの識別子
	static constexpr UINT WATCH_TIMER_ID = 1;
	// 空き容量を監視する間隔(ms)
	static constexpr DWORD WATCH_INTERVAL = 2000;

	bool m_fInitialized = false;                        // 初期化済みか?
	TCHAR m_szIniFileName[MAX_PATH];                    // INIファイルのパス
	TCHAR m_szSpareFolder[NUM_SPARE_FOLDERS][MAX_PATH]; // 予備のフォルダ
	UINT m_LowFreeSpace = 64;                           // 空き容量が少ないと判定する閾値
	HWND m_hwnd = nullptr;                              // ウィンドウハンドル
	bool m_fRecording = false;                          // 録画中か?
	int m_NextFolder = 0;                               // 次の録画先フォルダ

	bool InitializePlugin();
	bool CheckFreeSpace();
	bool SettingsDialog(HWND hwndOwner);
	void SaveSettings();
	static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);
	static CDiskRelay * GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, void *pClientData);

public:
	CDiskRelay();
	bool GetPluginInfo(TVTest::PluginInfo *pInfo) override;
	bool Initialize() override;
	bool Finalize() override;
};


const LPCTSTR CDiskRelay::DISKRELAY_WINDOW_CLASS = TEXT("TVTest DiskRelay Window");


CDiskRelay::CDiskRelay()
{
	for (int i = 0; i < NUM_SPARE_FOLDERS; i++)
		m_szSpareFolder[i][0] = '\0';
}


bool CDiskRelay::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	// プラグインの情報を返す
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = TVTest::PLUGIN_FLAG_HASSETTINGS;
	pInfo->pszPluginName  = L"予備の録画先";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"空き容量が少なくなった時、予備の録画先に録画します。";
	return true;
}


// 初期化処理
bool CDiskRelay::Initialize()
{
	// 対応状況をチェックする
	TVTest::HostInfo HostInfo;
	if (!m_pApp->GetHostInfo(&HostInfo)
			|| HostInfo.SupportedPluginVersion < TVTEST_PLUGIN_VERSION_(0, 0, 10)
			|| !m_pApp->QueryMessage(TVTest::MESSAGE_RELAYRECORD)) {
		m_pApp->AddLog(L"このバージョンでは利用できません。", TVTest::LOG_TYPE_ERROR);
		return false;
	}

	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(EventCallback, this);

	return true;
}


// プラグインが有効にされた時の初期化処理
bool CDiskRelay::InitializePlugin()
{
	if (m_fInitialized)
		return true;

	// 設定の読み込み
	::GetModuleFileName(g_hinstDLL, m_szIniFileName, MAX_PATH);
	::PathRenameExtension(m_szIniFileName, TEXT(".ini"));
	for (int i = 0; i < NUM_SPARE_FOLDERS; i++) {
		TCHAR szKey[16];
		::wsprintf(szKey, TEXT("Folder%d"), i);
		::GetPrivateProfileString(
			TEXT("Settings"), szKey, nullptr, m_szSpareFolder[i], MAX_PATH, m_szIniFileName);
	}
	m_LowFreeSpace = ::GetPrivateProfileInt(
		TEXT("Settings"), TEXT("LowFreeSpace"), m_LowFreeSpace, m_szIniFileName);

	// ウィンドウクラスの登録
	WNDCLASS wc;
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = g_hinstDLL;
	wc.hIcon = nullptr;
	wc.hCursor = nullptr;
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = DISKRELAY_WINDOW_CLASS;
	if (::RegisterClass(&wc) == 0)
		return false;

	// ウィンドウの作成
	m_hwnd = ::CreateWindowEx(
		0, DISKRELAY_WINDOW_CLASS, nullptr, WS_POPUP,
		0, 0, 0, 0, HWND_MESSAGE, nullptr, g_hinstDLL, this);
	if (m_hwnd == nullptr)
		return false;

	m_fInitialized = true;
	return true;
}


// 終了処理
bool CDiskRelay::Finalize()
{
	// ウィンドウの破棄
	if (m_hwnd)
		::DestroyWindow(m_hwnd);

	return true;
}


bool CDiskRelay::CheckFreeSpace()
{
	if (m_NextFolder >= NUM_SPARE_FOLDERS)
		return false;

	// 録画の状況を取得する
	TVTest::RecordStatusInfo RecStat;
	TCHAR szFileName[MAX_PATH];
	RecStat.pszFileName = szFileName;
	RecStat.MaxFileName = MAX_PATH;
	if (m_pApp->GetRecordStatus(&RecStat)
			&& RecStat.Status == TVTest::RECORD_STATUS_RECORDING) {
		// 空き容量を取得
		TCHAR szPath[MAX_PATH + 8];
		::lstrcpy(szPath, szFileName);
		*::PathFindFileName(szPath) = '\0';
		ULARGE_INTEGER FreeSpace;
		if (::GetDiskFreeSpaceEx(szPath, &FreeSpace, nullptr, nullptr)
				&& FreeSpace.QuadPart <= (ULONGLONG)m_LowFreeSpace * 0x100000) {
			m_pApp->AddLog(
				TEXT("空き容量が少ないため続きを予備のフォルダに録画します。"),
				TVTest::LOG_TYPE_WARNING);
			for (; m_NextFolder < NUM_SPARE_FOLDERS; m_NextFolder++) {
				if (m_szSpareFolder[m_NextFolder][0] != '\0'
						&& ::PathIsDirectory(m_szSpareFolder[m_NextFolder])) {
					::lstrcpy(szPath, m_szSpareFolder[m_NextFolder]);
					::PathAddBackslash(szPath);
					if (::GetDiskFreeSpaceEx(szPath, &FreeSpace, nullptr, nullptr)
							&& FreeSpace.QuadPart > (ULONGLONG)m_LowFreeSpace * 0x100000) {
						::lstrcat(szPath, ::PathFindFileName(szFileName));
						::wsprintf(
							::PathFindExtension(szPath), TEXT(".part%d%s"),
							m_NextFolder + 2,
							::PathFindExtension(szFileName));
						if (m_pApp->RelayRecord(szPath))
							return true;
					}
				}
			}
			m_pApp->AddLog(TEXT("空き容量のあるフォルダがありません。"), TVTest::LOG_TYPE_ERROR);
		}
	}
	return false;
}


// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CDiskRelay::EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
{
	CDiskRelay *pThis = static_cast<CDiskRelay *>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// プラグインの有効状態が変化した
		if (lParam1 != 0) {
			if (!pThis->InitializePlugin())
				return FALSE;

			// まだフォルダが設定されていなければ設定ダイアログを表示する
			int i;
			for (i = 0; i < NUM_SPARE_FOLDERS; i++) {
				if (pThis->m_szSpareFolder[i][0] != '\0')
					break;
			}
			if (i == NUM_SPARE_FOLDERS) {
				if (!pThis->SettingsDialog(pThis->m_pApp->GetAppWindow()))
					return FALSE;
			}

			if (pThis->m_fRecording)
				::SetTimer(pThis->m_hwnd, WATCH_TIMER_ID, WATCH_INTERVAL, nullptr);
		} else {
			if (pThis->m_hwnd != nullptr && pThis->m_fRecording)
				::KillTimer(pThis->m_hwnd, WATCH_TIMER_ID);
		}
		return TRUE;

	case TVTest::EVENT_PLUGINSETTINGS:
		// プラグインの設定を行う
		pThis->InitializePlugin();
		return pThis->SettingsDialog(reinterpret_cast<HWND>(lParam1));

	case TVTest::EVENT_RECORDSTATUSCHANGE:
		// 録画状態が変化した
		if (lParam1 != TVTest::RECORD_STATUS_NOTRECORDING) {
			if (!pThis->m_fRecording) {
				pThis->m_fRecording = true;
				pThis->m_NextFolder = 0;
				if (pThis->m_pApp->IsPluginEnabled()) {
					pThis->CheckFreeSpace();
					::SetTimer(pThis->m_hwnd, WATCH_TIMER_ID, WATCH_INTERVAL, nullptr);
				}
			}
		} else {
			if (pThis->m_fRecording) {
				pThis->m_fRecording = false;
				if (pThis->m_hwnd != nullptr)
					::KillTimer(pThis->m_hwnd, WATCH_TIMER_ID);
			}
		}
		return TRUE;
	}

	return 0;
}


// 設定ダイアログの表示
bool CDiskRelay::SettingsDialog(HWND hwndOwner)
{
	TVTest::ShowDialogInfo Info;

	Info.Flags = 0;
	Info.hinst = g_hinstDLL;
	Info.pszTemplate = MAKEINTRESOURCE(IDD_SETTINGS);
	Info.pMessageFunc = SettingsDlgProc;
	Info.pClientData = this;
	Info.hwndOwner = hwndOwner;

	return m_pApp->ShowDialog(&Info) == IDOK;
}


// 設定の保存
void CDiskRelay::SaveSettings()
{
	for (int i = 0; i < NUM_SPARE_FOLDERS; i++) {
		TCHAR szKey[16];
		::wsprintf(szKey, TEXT("Folder%d"), i);
		::WritePrivateProfileString(TEXT("Settings"), szKey, m_szSpareFolder[i], m_szIniFileName);
	}
	TCHAR szValue[16];
	::wsprintf(szValue, TEXT("%u"), m_LowFreeSpace);
	::WritePrivateProfileString(TEXT("Settings"), TEXT("LowFreeSpace"), szValue, m_szIniFileName);
}


// ウィンドウハンドルからthisを取得する
CDiskRelay * CDiskRelay::GetThis(HWND hwnd)
{
	return reinterpret_cast<CDiskRelay *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
}


// ウィンドウプロシージャ
// 単にタイマーを処理するだけ
LRESULT CALLBACK CDiskRelay::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
			CDiskRelay *pThis = static_cast<CDiskRelay *>(pcs->lpCreateParams);

			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
		}
		return TRUE;

	case WM_TIMER:
		{
			CDiskRelay *pThis = GetThis(hwnd);

			pThis->CheckFreeSpace();
		}
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


int CALLBACK BrowseFolderCallback(HWND hwnd, UINT uMsg, LPARAM lpData, LPARAM lParam)
{
	switch (uMsg) {
	case BFFM_INITIALIZED:
		{
			// フォルダ参照時の初期フォルダを設定する
			LPCTSTR pszDirectory = reinterpret_cast<LPCTSTR>(lParam);

			if (pszDirectory[0] != '\0') {
				TCHAR szFolder[MAX_PATH];

				lstrcpy(szFolder, pszDirectory);
				PathRemoveBackslash(szFolder);
				SendMessage(hwnd, BFFM_SETSELECTION, TRUE, reinterpret_cast<LPARAM>(szFolder));
			}
		}
		break;
	}

	return 0;
}

// フォルダ参照ダイアログ
bool BrowseFolderDialog(HWND hwndOwner, LPTSTR pszDirectory, LPCTSTR pszTitle)
{
	BROWSEINFO bi;
	PIDLIST_ABSOLUTE pidl;
	BOOL fRet;

	bi.hwndOwner = hwndOwner;
	bi.pidlRoot = nullptr;
	bi.pszDisplayName = pszDirectory;
	bi.lpszTitle = pszTitle;
	// BIF_NEWDIALOGSTYLE を使ってるが、CoInitialize は TVTest 本体で呼んでいるので不要
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	bi.lpfn = BrowseFolderCallback;
	bi.lParam = reinterpret_cast<LPARAM>(pszDirectory);
	pidl = SHBrowseForFolder(&bi);
	if (pidl == nullptr)
		return false;
	fRet = SHGetPathFromIDList(pidl, pszDirectory);
	CoTaskMemFree(pidl);
	return fRet != FALSE;
}


// 設定ダイアログプロシージャ
INT_PTR CALLBACK CDiskRelay::SettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, void *pClientData)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CDiskRelay *pThis = static_cast<CDiskRelay *>(pClientData);

			// デフォルトの録画フォルダを取得する
			TCHAR szDefaultFolder[MAX_PATH];
			if (pThis->m_pApp->GetSetting(L"RecordFolder", szDefaultFolder, MAX_PATH) > 0)
				::SetDlgItemText(hDlg, IDC_SETTINGS_DEFAULTFOLDER, szDefaultFolder);

			for (int i = 0; i < NUM_SPARE_FOLDERS; i++) {
				::SendDlgItemMessage(hDlg, IDC_SETTINGS_FOLDER1 + i * 3, EM_LIMITTEXT, MAX_PATH - 1, 0);
				::SetDlgItemText(hDlg, IDC_SETTINGS_FOLDER1 + i * 3, pThis->m_szSpareFolder[i]);
			}

			::SetDlgItemInt(hDlg, IDC_SETTINGS_LOWSPACE, pThis->m_LowFreeSpace, FALSE);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SETTINGS_FOLDER1_BROWSE:
		case IDC_SETTINGS_FOLDER2_BROWSE:
		case IDC_SETTINGS_FOLDER3_BROWSE:
			{
				int EditID = LOWORD(wParam) - 1;
				TCHAR szFolder[MAX_PATH];

				::GetDlgItemText(hDlg, EditID, szFolder, MAX_PATH);
				if (BrowseFolderDialog(hDlg, szFolder, TEXT("予備のフォルダを選択してください"))) {
					::SetDlgItemText(hDlg, EditID, szFolder);
				}
			}
			return TRUE;

		case IDOK:
			{
				CDiskRelay *pThis = static_cast<CDiskRelay *>(pClientData);

				// フォルダがあるかチェックする
				for (int i = 0; i < NUM_SPARE_FOLDERS; i++) {
					TCHAR szFolder[MAX_PATH];
					::GetDlgItemText(hDlg, IDC_SETTINGS_FOLDER1 + i * 3, szFolder, MAX_PATH);
					if (szFolder[0] != '\0' && !::PathIsDirectory(szFolder)) {
						TCHAR szMessage[MAX_PATH + 80];
						::wsprintf(szMessage, TEXT("フォルダ \"%s\" がありません。\n作成しますか?"), szFolder);
						switch (::MessageBox(hDlg, szMessage, TEXT("フォルダ作成の確認"), MB_YESNOCANCEL | MB_ICONQUESTION)) {
						case IDYES:
							{
								int Result = ::SHCreateDirectoryEx(hDlg, szFolder, nullptr);
								if (Result != ERROR_SUCCESS
										&& Result != ERROR_ALREADY_EXISTS) {
									::wsprintf(szMessage, TEXT("フォルダ \"%s\" が作成できません。"), szFolder);
									::MessageBox(hDlg, szMessage, nullptr, MB_OK | MB_ICONEXCLAMATION);
									return TRUE;
								}
							}
							break;
						case IDNO:
							break;
						default:
							return TRUE;
						}
					}
				}

				for (int i = 0; i < NUM_SPARE_FOLDERS; i++) {
					::GetDlgItemText(hDlg, IDC_SETTINGS_FOLDER1 + i * 3, pThis->m_szSpareFolder[i], MAX_PATH);
				}

				pThis->m_LowFreeSpace =
					::GetDlgItemInt(hDlg, IDC_SETTINGS_LOWSPACE, nullptr, FALSE);

				pThis->SaveSettings();
			}
		case IDCANCEL:
			::EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		return TRUE;
	}

	return FALSE;
}




// プラグインクラスのインスタンスを生成する
TVTest::CTVTestPlugin * CreatePluginClass()
{
	return new CDiskRelay;
}
