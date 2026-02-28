/*
	TVTest プラグインサンプル

	指定条件でスリープする

	このサンプルでは主に以下の機能を実装しています。

	・ダイアログを表示する
	・TVTest を終了させる
*/


#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <Powrprof.h>

#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "powrprof.lib")


// FILETIME の単位
static constexpr LONGLONG FILETIME_MS   = 10000LL;
static constexpr LONGLONG FILETIME_SEC  = 1000LL * FILETIME_MS;
static constexpr LONGLONG FILETIME_MIN  = 60LL * FILETIME_SEC;
static constexpr LONGLONG FILETIME_HOUR = 60LL * FILETIME_MIN;

// FILETIME の時間差を求める
static LONGLONG DiffFileTime(const FILETIME &ft1, const FILETIME &ft2)
{
	LARGE_INTEGER li1, li2;

	li1.LowPart = ft1.dwLowDateTime;
	li1.HighPart = ft1.dwHighDateTime;
	li2.LowPart = ft2.dwLowDateTime;
	li2.HighPart = ft2.dwHighDateTime;

	return li1.QuadPart - li2.QuadPart;
}

// SYSTEMTIME の時間差を求める(ms単位)
static LONGLONG DiffSystemTime(const SYSTEMTIME &st1, const SYSTEMTIME &st2)
{
	FILETIME ft1, ft2;

	::SystemTimeToFileTime(&st1, &ft1);
	::SystemTimeToFileTime(&st2, &ft2);

	return DiffFileTime(ft1, ft2) / FILETIME_MS;
}


// プラグインクラス
class CSleepTimer : public TVTest::CTVTestPlugin
{
	// スリープ条件
	enum class SleepCondition {
		Duration, // 時間経過
		DateTime, // 指定時刻
		EventEnd, // 番組終了
	};

	// スリープ方法
	enum class SleepMode {
		Exit,      // TVTest終了
		PowerOff,  // 電源オフ
		LogOff,    // ログオフ
		Suspend,   // サスペンド
		Hibernate, // ハイバネート
	};

	enum {
		TIMER_ID_SLEEP = 1,
		TIMER_ID_QUERY
	};

	static const LPCTSTR SLEEPTIMER_WINDOW_CLASS;        // ウィンドウクラス名

	static constexpr int DEFAULT_POS = INT_MIN;

	bool m_fInitialized = false;                           // 初期化済みか?
	TCHAR m_szIniFileName[MAX_PATH];                       // INIファイルのパス
	SleepCondition m_Condition = SleepCondition::Duration; // スリープする条件
	DWORD m_SleepDuration = 30 * 60;                       // スリープまでの時間(秒単位)
	SYSTEMTIME m_SleepDateTime;                            // スリープする時刻
	WORD m_EventID = 0;                                    // 現在の番組の event_id
	SleepMode m_Mode = SleepMode::Exit;                    // スリープ時の動作
	bool m_fForce = false;                                 // 強制
	bool m_fMonitorOff = false;                            // モニタをOFFにする
	bool m_fIgnoreRecStatus = false;                       // 録画中でもスリープする
	bool m_fConfirm = true;                                // 確認を取る
	int m_ConfirmTimeout = 10;                             // 確認のタイムアウト時間(秒単位)
	bool m_fShowSettings = true;                           // プラグイン有効時に設定表示
	POINT m_SettingsDialogPos{DEFAULT_POS, DEFAULT_POS};   // 設定ダイアログの位置
	HWND m_hwnd = nullptr;                                 // ウィンドウハンドル
	bool m_fEnabled = false;                               // プラグインが有効か?
	int m_ConfirmTimerCount;                               // 確認のタイマー

	static const LPCTSTR m_ModeTextList[];

	bool InitializePlugin();
	bool OnEnablePlugin(bool fEnable);
	bool BeginSleep();
	bool DoSleep();
	bool BeginTimer();
	void EndTimer();
	bool ShowSettingsDialog(HWND hwndOwner);

	static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);
	static CSleepTimer * GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, void *pClientData);
	static INT_PTR CALLBACK ConfirmDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, void *pClientData);

public:
	bool GetPluginInfo(TVTest::PluginInfo *pInfo) override;
	bool Initialize() override;
	bool Finalize() override;
};


const LPCTSTR CSleepTimer::SLEEPTIMER_WINDOW_CLASS = TEXT("TVTest SleepTimer Window");

const LPCTSTR CSleepTimer::m_ModeTextList[] = {
	TEXT("TVTest を終了"),
	TEXT("電源オフ"),
	TEXT("ログオフ"),
	TEXT("サスペンド"),
	TEXT("ハイバネート"),
};


bool CSleepTimer::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	// プラグインの情報を返す
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = TVTest::PLUGIN_FLAG_HASSETTINGS     // 設定あり
	                      | TVTest::PLUGIN_FLAG_DISABLEONSTART; // 起動時は常に無効
	pInfo->pszPluginName  = L"スリープタイマー";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"指定条件で終了させます。";
	return true;
}


bool CSleepTimer::Initialize()
{
	// 初期化処理

	// アイコンを登録
	m_pApp->RegisterPluginIconFromResource(g_hinstDLL, MAKEINTRESOURCE(IDB_ICON));

	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(EventCallback, this);

	return true;
}


// プラグインが有効にされた時の初期化処理
bool CSleepTimer::InitializePlugin()
{
	if (m_fInitialized)
		return true;

	// 設定の読み込み
	::GetModuleFileName(g_hinstDLL, m_szIniFileName, MAX_PATH);
	::PathRenameExtension(m_szIniFileName, TEXT(".ini"));
	m_Condition = (SleepCondition)
		::GetPrivateProfileInt(TEXT("Settings"), TEXT("Condition"), (int)m_Condition, m_szIniFileName);
	m_SleepDuration =
		::GetPrivateProfileInt(TEXT("Settings"), TEXT("Time"), m_SleepDuration, m_szIniFileName);
	m_Mode = (SleepMode)
		::GetPrivateProfileInt(TEXT("Settings"), TEXT("Mode"), (int)m_Mode, m_szIniFileName);
	m_fForce =
		::GetPrivateProfileInt(TEXT("Settings"), TEXT("Force"), m_fForce, m_szIniFileName) != 0;
	m_fMonitorOff =
		::GetPrivateProfileInt(TEXT("Settings"), TEXT("MonitorOff"), m_fMonitorOff, m_szIniFileName) != 0;
	m_fIgnoreRecStatus =
		::GetPrivateProfileInt(TEXT("Settings"), TEXT("IgnoreRecStatus"), m_fIgnoreRecStatus, m_szIniFileName) != 0;
	m_fConfirm =
		::GetPrivateProfileInt(TEXT("Settings"), TEXT("Confirm"), m_fConfirm, m_szIniFileName) != 0;
	m_ConfirmTimeout =
		::GetPrivateProfileInt(TEXT("Settings"), TEXT("ConfirmTimeout"), m_ConfirmTimeout, m_szIniFileName);
	m_fShowSettings =
		::GetPrivateProfileInt(TEXT("Settings"), TEXT("ShowSettings"), m_fShowSettings, m_szIniFileName) != 0;
	m_SettingsDialogPos.x = (int)
		::GetPrivateProfileInt(TEXT("Settings"), TEXT("SettingsDialogX"), m_SettingsDialogPos.x, m_szIniFileName);
	m_SettingsDialogPos.y = (int)
		::GetPrivateProfileInt(TEXT("Settings"), TEXT("SettingsDialogY"), m_SettingsDialogPos.y, m_szIniFileName);

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
	wc.lpszClassName = SLEEPTIMER_WINDOW_CLASS;
	if (::RegisterClass(&wc) == 0)
		return false;

	// ウィンドウの作成
	m_hwnd = ::CreateWindowEx(
		0, SLEEPTIMER_WINDOW_CLASS, nullptr, WS_POPUP,
		0, 0, 0, 0, HWND_MESSAGE, nullptr, g_hinstDLL, this);
	if (m_hwnd == nullptr)
		return false;

	m_fInitialized = true;
	return true;
}


bool CSleepTimer::Finalize()
{
	// 終了処理

	m_pApp->EnablePlugin(false); // 次回起動時に有効にならないように

	// ウィンドウの破棄
	if (m_hwnd)
		::DestroyWindow(m_hwnd);

	if (m_fInitialized) {
		// 設定の保存
		struct IntString {
			IntString(int Value) { ::wsprintf(m_szBuffer, TEXT("%d"), Value); }
			operator LPCTSTR() const { return m_szBuffer; }
			TCHAR m_szBuffer[16];
		};
		::WritePrivateProfileString(TEXT("Settings"), TEXT("Condition"), IntString((int)m_Condition), m_szIniFileName);
		::WritePrivateProfileString(TEXT("Settings"), TEXT("Time"), IntString(m_SleepDuration), m_szIniFileName);
		::WritePrivateProfileString(TEXT("Settings"), TEXT("Mode"), IntString((int)m_Mode), m_szIniFileName);
		::WritePrivateProfileString(TEXT("Settings"), TEXT("Force"), IntString(m_fForce), m_szIniFileName);
		::WritePrivateProfileString(TEXT("Settings"), TEXT("MonitorOff"), IntString(m_fMonitorOff), m_szIniFileName);
		::WritePrivateProfileString(TEXT("Settings"), TEXT("IgnoreRecStatus"), IntString(m_fIgnoreRecStatus), m_szIniFileName);
		::WritePrivateProfileString(TEXT("Settings"), TEXT("Confirm"), IntString(m_fConfirm), m_szIniFileName);
		::WritePrivateProfileString(TEXT("Settings"), TEXT("ConfirmTimeout"), IntString(m_ConfirmTimeout), m_szIniFileName);
		::WritePrivateProfileString(TEXT("Settings"), TEXT("ShowSettings"), IntString(m_fShowSettings), m_szIniFileName);
		if (m_SettingsDialogPos.x != DEFAULT_POS)
			::WritePrivateProfileString(TEXT("Settings"), TEXT("SettingsDialogX"), IntString(m_SettingsDialogPos.x), m_szIniFileName);
		if (m_SettingsDialogPos.y != DEFAULT_POS)
			::WritePrivateProfileString(TEXT("Settings"), TEXT("SettingsDialogY"), IntString(m_SettingsDialogPos.y), m_szIniFileName);
	}

	return true;
}


// プラグインの有効/無効が切り替わった時の処理
bool CSleepTimer::OnEnablePlugin(bool fEnable)
{
	InitializePlugin();

	if (fEnable && (m_fShowSettings || m_Condition == SleepCondition::DateTime)) {
		if (!ShowSettingsDialog(m_pApp->GetAppWindow()))
			return false;
	}

	m_fEnabled = fEnable;

	if (m_fEnabled)
		BeginTimer();
	else
		EndTimer();

	return true;
}


// スリープ開始
bool CSleepTimer::BeginSleep()
{
	m_pApp->AddLog(L"スリープを開始します。");

	m_pApp->EnablePlugin(false); // タイマーは一回限り有効
	EndTimer(); // EventCallbackで呼ばれるはずだが、念のため

	if (m_fConfirm) {
		// 確認ダイアログを表示
		TVTest::ShowDialogInfo Info;

		Info.Flags = 0;
		Info.hinst = g_hinstDLL;
		Info.pszTemplate = MAKEINTRESOURCE(IDD_CONFIRM);
		Info.pMessageFunc = ConfirmDlgProc;
		Info.pClientData = this;
		Info.hwndOwner = m_pApp->GetAppWindow();

		if (m_pApp->ShowDialog(&Info) != IDOK) {
			m_pApp->AddLog(L"ユーザーによってスリープがキャンセルされました。");
			return false;
		}
	}

	if (!m_fIgnoreRecStatus) {
		// 録画中はスリープ実行しない
		TVTest::RecordStatusInfo RecStat;
		if (!m_pApp->GetRecordStatus(&RecStat)) {
			m_pApp->AddLog(L"録画状態を取得できないのでスリープがキャンセルされました。", TVTest::LOG_TYPE_WARNING);
			return false;
		}
		if (RecStat.Status != TVTest::RECORD_STATUS_NOTRECORDING) {
			m_pApp->AddLog(L"録画中なのでスリープがキャンセルされました。");
			return false;
		}
	}

	// スリープ実行
	return DoSleep();
}


// スリープ実行
bool CSleepTimer::DoSleep()
{
	if (m_Mode != SleepMode::Exit && m_Mode != SleepMode::LogOff) {
		// 権限設定
		HANDLE hToken;

		if (::OpenProcessToken(
				::GetCurrentProcess(),
				TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken)) {
			LUID ld;
			LUID_AND_ATTRIBUTES la;
			TOKEN_PRIVILEGES tp;

			::LookupPrivilegeValue(nullptr, SE_SHUTDOWN_NAME, &ld);
			la.Luid = ld;
			la.Attributes = SE_PRIVILEGE_ENABLED;
			tp.PrivilegeCount = 1;
			tp.Privileges[0] = la;
			::AdjustTokenPrivileges(hToken, FALSE, &tp, 0, nullptr, nullptr);
			::CloseHandle(hToken);
		}
	}

	switch (m_Mode) {
	case SleepMode::Exit:
		if (!m_pApp->Close(m_fForce ? TVTest::CLOSE_EXIT : 0))
			return false;
		if (m_fMonitorOff) {
			// モニタをOFFにする
			::PostMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, 1);
			::Sleep(1000);
			::PostMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, 2);
		}
		break;

	case SleepMode::PowerOff:
		if (!::ExitWindowsEx((m_fForce ? EWX_FORCE : 0) | EWX_POWEROFF, 0))
			return false;
		break;

	case SleepMode::LogOff:
		if (!::ExitWindowsEx((m_fForce ? EWX_FORCE : 0) | EWX_LOGOFF, 0))
			return false;
		break;

	case SleepMode::Suspend:
		if (!::SetSuspendState(FALSE, m_fForce, FALSE))
			return false;
		break;

	case SleepMode::Hibernate:
		if (!::SetSuspendState(TRUE, m_fForce, FALSE))
			return false;
		break;

	default:
		return false;
	}

	return true;
}


// タイマー開始
bool CSleepTimer::BeginTimer()
{
	UINT_PTR Result;
	WCHAR szLog[256];

	if (m_Condition == SleepCondition::Duration) {
		::wsprintfW(szLog, L"%lu 秒後にスリープします。", (unsigned long)m_SleepDuration);
		m_pApp->AddLog(szLog);
		Result = ::SetTimer(m_hwnd, TIMER_ID_SLEEP, m_SleepDuration * 1000, nullptr);
	} else if (m_Condition == SleepCondition::DateTime || m_Condition == SleepCondition::EventEnd) {
		if (m_Condition == SleepCondition::DateTime) {
			::wsprintfW(
				szLog, L"%d/%d/%d %02d:%02d:%02d (UTC) にスリープします。",
				m_SleepDateTime.wYear, m_SleepDateTime.wMonth, m_SleepDateTime.wDay,
				m_SleepDateTime.wHour, m_SleepDateTime.wMinute, m_SleepDateTime.wSecond);
			m_pApp->AddLog(szLog);
		} else {
			m_EventID = 0;
		}
		Result = ::SetTimer(m_hwnd, TIMER_ID_QUERY, 3000, nullptr);
	} else {
		return false;
	}

	return Result != 0;
}


// タイマー停止
void CSleepTimer::EndTimer()
{
	::KillTimer(m_hwnd, TIMER_ID_SLEEP);
	::KillTimer(m_hwnd, TIMER_ID_QUERY);
}


// 設定ダイアログを表示
bool CSleepTimer::ShowSettingsDialog(HWND hwndOwner)
{
	TVTest::ShowDialogInfo Info;

	Info.Flags = 0;
	Info.hinst = g_hinstDLL;
	Info.pszTemplate = MAKEINTRESOURCE(IDD_SETTINGS);
	Info.pMessageFunc = SettingsDlgProc;
	Info.pClientData = this;
	Info.hwndOwner = hwndOwner;
	if (m_SettingsDialogPos.x != DEFAULT_POS && m_SettingsDialogPos.y != DEFAULT_POS) {
		Info.Flags |= TVTest::SHOW_DIALOG_FLAG_POSITION;
		Info.Position = m_SettingsDialogPos;
	}

	return m_pApp->ShowDialog(&Info) == IDOK;
}


// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CSleepTimer::EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
{
	CSleepTimer *pThis = static_cast<CSleepTimer *>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// プラグインの有効状態が変化した
		return pThis->OnEnablePlugin(lParam1 != 0);

	case TVTest::EVENT_PLUGINSETTINGS:
		// プラグインの設定を行う
		pThis->InitializePlugin();
		return pThis->ShowSettingsDialog(reinterpret_cast<HWND>(lParam1));
	}

	return 0;
}


// ウィンドウハンドルからthisを取得する
CSleepTimer * CSleepTimer::GetThis(HWND hwnd)
{
	return reinterpret_cast<CSleepTimer *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
}


// ウィンドウプロシージャ
// 単にタイマーを処理するだけ
LRESULT CALLBACK CSleepTimer::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
			CSleepTimer *pThis = static_cast<CSleepTimer *>(pcs->lpCreateParams);

			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
		}
		return TRUE;

	case WM_TIMER:
		{
			CSleepTimer *pThis = GetThis(hwnd);

			if (wParam == TIMER_ID_SLEEP) {
				// 指定時間が経過したのでスリープ開始
				pThis->BeginSleep();
			} else if (wParam == TIMER_ID_QUERY) {
				if (pThis->m_Condition == SleepCondition::DateTime) {
					SYSTEMTIME st;

					::GetSystemTime(&st);
					if (DiffSystemTime(st, pThis->m_SleepDateTime) >= 0) {
						// 指定時刻が来たのでスリープ開始
						pThis->BeginSleep();
					}
				} else if (pThis->m_Condition == SleepCondition::EventEnd) {
					TVTest::ProgramInfo Info = {};
					WCHAR szEventName[128];

					// 現在の番組の情報を取得
					Info.pszEventName = szEventName;
					Info.MaxEventName = _countof(szEventName);
					if (pThis->m_pApp->GetCurrentProgramInfo(&Info)) {
						if (pThis->m_EventID == 0) {
							bool fSet = false;

							if (Info.Duration == 0) {
								// 終了時刻未定
								fSet = true;
							} else {
								FILETIME ft;
								::SystemTimeToFileTime(&Info.StartTime, &ft);
								LARGE_INTEGER li;
								li.LowPart = ft.dwLowDateTime;
								li.HighPart = ft.dwHighDateTime;
								li.QuadPart -= 9LL * FILETIME_HOUR;          // EPG日時(UTC+9) -> UTC
								li.QuadPart += Info.Duration * FILETIME_SEC; // 終了時刻
								ft.dwLowDateTime = li.LowPart;
								ft.dwHighDateTime = li.HighPart;
								FILETIME CurrentTime;
								::GetSystemTimeAsFileTime(&CurrentTime);
								// 番組終了が2分以内の場合は、次の番組を対象にする
								if (DiffFileTime(ft, CurrentTime) > 2LL * FILETIME_MIN)
									fSet = true;
							}

							if (fSet) {
								pThis->m_EventID = Info.EventID;
								pThis->m_pApp->AddLog(L"この番組が終了したらスリープします。");
								pThis->m_pApp->AddLog(szEventName);
							}
						} else if (pThis->m_EventID != Info.EventID) {
							// 番組が変わったのでスリープ開始
							pThis->BeginSleep();
						}
					}
				}
			}
		}
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


static void EnableDlgItem(HWND hDlg, int ID, BOOL fEnable)
{
	::EnableWindow(::GetDlgItem(hDlg, ID), fEnable);
}

static void EnableDlgItems(HWND hDlg, int FirstID, int LastID, BOOL fEnable)
{
	for (int i = FirstID; i <= LastID; i++)
		EnableDlgItem(hDlg, i, fEnable);
}

// 設定ダイアログプロシージャ
INT_PTR CALLBACK CSleepTimer::SettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, void *pClientData)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CSleepTimer *pThis = static_cast<CSleepTimer *>(pClientData);

			::CheckRadioButton(
				hDlg, IDC_SETTINGS_CONDITION_DURATION, IDC_SETTINGS_CONDITION_EVENTEND,
				IDC_SETTINGS_CONDITION_DURATION + (int)pThis->m_Condition);
			EnableDlgItems(
				hDlg, IDC_SETTINGS_DURATION_HOURS, IDC_SETTINGS_DURATION_SECONDS_UD,
				pThis->m_Condition == SleepCondition::Duration);
			EnableDlgItem(
				hDlg, IDC_SETTINGS_DATETIME, pThis->m_Condition == SleepCondition::DateTime);

			::SetDlgItemInt(hDlg, IDC_SETTINGS_DURATION_HOURS, pThis->m_SleepDuration / (60 * 60), FALSE);
			::SendDlgItemMessage(hDlg, IDC_SETTINGS_DURATION_HOURS_UD, UDM_SETRANGE32, 0, 24 * 24);
			::SetDlgItemInt(hDlg, IDC_SETTINGS_DURATION_MINUTES, pThis->m_SleepDuration / 60 % 60, FALSE);
			::SendDlgItemMessage(hDlg, IDC_SETTINGS_DURATION_MINUTES_UD, UDM_SETRANGE32, 0, 59);
			::SetDlgItemInt(hDlg, IDC_SETTINGS_DURATION_SECONDS, pThis->m_SleepDuration % 60, FALSE);
			::SendDlgItemMessage(hDlg, IDC_SETTINGS_DURATION_SECONDS_UD, UDM_SETRANGE32, 0, 59);

			HWND hwndDateTime = ::GetDlgItem(hDlg, IDC_SETTINGS_DATETIME);
			DateTime_SetFormat(hwndDateTime, TEXT("yyyy'/'MM'/'dd' 'HH':'mm':'ss"));
			SYSTEMTIME st;
			::GetLocalTime(&st);
			DateTime_SetSystemtime(hwndDateTime, GDT_VALID, &st);

			for (int i = 0; i < sizeof(m_ModeTextList) / sizeof(LPCTSTR); i++)
				::SendDlgItemMessage(hDlg, IDC_SETTINGS_MODE, CB_ADDSTRING, 0, (LPARAM)m_ModeTextList[i]);
			::SendDlgItemMessage(hDlg, IDC_SETTINGS_MODE, CB_SETCURSEL, (WPARAM)pThis->m_Mode, 0);

			::CheckDlgButton(hDlg, IDC_SETTINGS_FORCE, pThis->m_fForce ? BST_CHECKED : BST_UNCHECKED);
			::CheckDlgButton(hDlg, IDC_SETTINGS_MONITOROFF, pThis->m_fMonitorOff ? BST_CHECKED : BST_UNCHECKED);
			::CheckDlgButton(hDlg, IDC_SETTINGS_IGNORERECSTATUS, pThis->m_fIgnoreRecStatus ? BST_CHECKED : BST_UNCHECKED);
			::CheckDlgButton(hDlg, IDC_SETTINGS_CONFIRM, pThis->m_fConfirm ? BST_CHECKED : BST_UNCHECKED);
			::SetDlgItemInt(hDlg, IDC_SETTINGS_CONFIRMTIMEOUT, pThis->m_ConfirmTimeout, TRUE);
			::SendDlgItemMessage(hDlg, IDC_SETTINGS_CONFIRMTIMEOUT_UD, UDM_SETRANGE32, 1, 60 * 60);
			EnableDlgItems(
				hDlg, IDC_SETTINGS_CONFIRMTIMEOUT_LABEL, IDC_SETTINGS_CONFIRMTIMEOUT_UNIT,
				pThis->m_fConfirm);
			::CheckDlgButton(hDlg, IDC_SETTINGS_SHOWSETTINGS, pThis->m_fShowSettings ? BST_CHECKED : BST_UNCHECKED);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SETTINGS_CONDITION_DURATION:
		case IDC_SETTINGS_CONDITION_DATETIME:
		case IDC_SETTINGS_CONDITION_EVENTEND:
			{
				EnableDlgItems(
					hDlg, IDC_SETTINGS_DURATION_HOURS, IDC_SETTINGS_DURATION_SECONDS_UD,
					::IsDlgButtonChecked(hDlg, IDC_SETTINGS_CONDITION_DURATION) == BST_CHECKED);
				EnableDlgItem(
					hDlg, IDC_SETTINGS_DATETIME,
					::IsDlgButtonChecked(hDlg, IDC_SETTINGS_CONDITION_DATETIME) == BST_CHECKED);
			}
			return TRUE;

		case IDC_SETTINGS_CONFIRM:
			EnableDlgItems(
				hDlg, IDC_SETTINGS_CONFIRMTIMEOUT_LABEL, IDC_SETTINGS_CONFIRMTIMEOUT_UNIT,
				::IsDlgButtonChecked(hDlg, IDC_SETTINGS_CONFIRM) == BST_CHECKED);
			return TRUE;

		case IDOK:
			{
				CSleepTimer *pThis = static_cast<CSleepTimer *>(pClientData);
				SleepCondition Condition;

				if (::IsDlgButtonChecked(hDlg, IDC_SETTINGS_CONDITION_DURATION)) {
					Condition = SleepCondition::Duration;
				} else if (::IsDlgButtonChecked(hDlg, IDC_SETTINGS_CONDITION_DATETIME)) {
					Condition = SleepCondition::DateTime;
				} else if (::IsDlgButtonChecked(hDlg, IDC_SETTINGS_CONDITION_EVENTEND)) {
					Condition = SleepCondition::EventEnd;
				} else {
					::MessageBox(hDlg, TEXT("スリープする条件を選択してください。"), nullptr, MB_OK | MB_ICONEXCLAMATION);
					return TRUE;
				}

				ULONGLONG Duration =
					(ULONGLONG)::GetDlgItemInt(hDlg, IDC_SETTINGS_DURATION_HOURS, nullptr, FALSE) * (60 * 60) +
					(ULONGLONG)::GetDlgItemInt(hDlg, IDC_SETTINGS_DURATION_MINUTES, nullptr, FALSE) * 60 +
					(ULONGLONG)::GetDlgItemInt(hDlg, IDC_SETTINGS_DURATION_SECONDS, nullptr, FALSE);
				if (Condition == SleepCondition::Duration) {
					if (Duration * 1000 > USER_TIMER_MAXIMUM) {
						::MessageBox(hDlg, TEXT("スリープまでの時間が長すぎます。"), nullptr, MB_OK | MB_ICONEXCLAMATION);
						return TRUE;
					}
					if (Duration == 0) {
						::MessageBox(hDlg, TEXT("時間の指定が正しくありません。"), nullptr, MB_OK | MB_ICONEXCLAMATION);
						return TRUE;
					}
				}

				SYSTEMTIME DateTime;
				DWORD Result = DateTime_GetSystemtime(::GetDlgItem(hDlg, IDC_SETTINGS_DATETIME), &DateTime);
				if (Condition == SleepCondition::DateTime) {
					if (Result != GDT_VALID) {
						::MessageBox(hDlg, TEXT("時刻を指定してください。"), nullptr, MB_OK | MB_ICONEXCLAMATION);
						return TRUE;
					}
					SYSTEMTIME UTCTime;
					::TzSpecificLocalTimeToSystemTime(nullptr, &DateTime, &UTCTime);
					DateTime = UTCTime;
					SYSTEMTIME CurTime;
					::GetSystemTime(&CurTime);
					if (DiffSystemTime(DateTime, CurTime) <= 0) {
						::MessageBox(hDlg, TEXT("指定された時刻を既に過ぎています。"), nullptr, MB_OK | MB_ICONEXCLAMATION);
						return TRUE;
					}
				}

				pThis->m_Condition = Condition;
				pThis->m_SleepDuration = (DWORD)Duration;
				pThis->m_SleepDateTime = DateTime;

				pThis->m_Mode = (SleepMode)
					::SendDlgItemMessage(hDlg, IDC_SETTINGS_MODE, CB_GETCURSEL, 0, 0);
				pThis->m_fForce =
					::IsDlgButtonChecked(hDlg, IDC_SETTINGS_FORCE) == BST_CHECKED;
				pThis->m_fMonitorOff =
					::IsDlgButtonChecked(hDlg, IDC_SETTINGS_MONITOROFF) == BST_CHECKED;
				pThis->m_fIgnoreRecStatus =
					::IsDlgButtonChecked(hDlg, IDC_SETTINGS_IGNORERECSTATUS) == BST_CHECKED;
				pThis->m_fConfirm =
					::IsDlgButtonChecked(hDlg, IDC_SETTINGS_CONFIRM) == BST_CHECKED;
				pThis->m_ConfirmTimeout =
					::GetDlgItemInt(hDlg, IDC_SETTINGS_CONFIRMTIMEOUT, nullptr, FALSE);
				pThis->m_fShowSettings =
					::IsDlgButtonChecked(hDlg, IDC_SETTINGS_SHOWSETTINGS) == BST_CHECKED;

				// タイマー設定
				if (pThis->m_fEnabled)
					pThis->BeginTimer();
			}
		case IDCANCEL:
			{
				CSleepTimer *pThis = static_cast<CSleepTimer *>(pClientData);
				RECT rc;

				::GetWindowRect(hDlg, &rc);
				pThis->m_SettingsDialogPos.x = rc.left;
				pThis->m_SettingsDialogPos.y = rc.top;

				::EndDialog(hDlg, LOWORD(wParam));
			}
			return TRUE;
		}
		return TRUE;
	}

	return FALSE;
}


// 確認ダイアログプロシージャ
INT_PTR CALLBACK CSleepTimer::ConfirmDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, void *pClientData)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CSleepTimer *pThis = static_cast<CSleepTimer *>(pClientData);

			TCHAR szText[64];
			::wsprintf(szText, TEXT("%sしますか？"), m_ModeTextList[static_cast<int>(pThis->m_Mode)]);
			::SetDlgItemText(hDlg, IDC_CONFIRM_MODE, szText);

			if (pThis->m_ConfirmTimeout > 0) {
				::SetDlgItemInt(hDlg, IDC_CONFIRM_TIMEOUT, pThis->m_ConfirmTimeout, TRUE);
				pThis->m_ConfirmTimerCount = 0;
				::SetTimer(hDlg, 1, 1000, nullptr);
			} else {
				::SetDlgItemText(hDlg, IDC_CONFIRM_TIMEOUT, TEXT("∞"));
			}
		}
		return TRUE;

	case WM_TIMER:
		{
			CSleepTimer *pThis = static_cast<CSleepTimer *>(pClientData);

			pThis->m_ConfirmTimerCount++;
			if (pThis->m_ConfirmTimerCount < pThis->m_ConfirmTimeout) {
				::SetDlgItemInt(hDlg, IDC_CONFIRM_TIMEOUT, pThis->m_ConfirmTimeout - pThis->m_ConfirmTimerCount, TRUE);
			} else {
				::EndDialog(hDlg, IDOK);
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
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
	return new CSleepTimer;
}
