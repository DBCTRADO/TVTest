/*
	TVTest �v���O�C���T���v��

	�w������ŃX���[�v����
*/


#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <Powrprof.h>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"
#include "resource.h"

#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"powrprof.lib")


// FILETIME �̒P��
static const LONGLONG FILETIME_MS   = 10000LL;
static const LONGLONG FILETIME_SEC  = 1000LL * FILETIME_MS;
static const LONGLONG FILETIME_MIN  = 60LL * FILETIME_SEC;
static const LONGLONG FILETIME_HOUR = 60LL * FILETIME_MIN;

// FILETIME �̎��ԍ������߂�
static LONGLONG DiffFileTime(const FILETIME &ft1, const FILETIME &ft2)
{
	LARGE_INTEGER li1, li2;

	li1.LowPart = ft1.dwLowDateTime;
	li1.HighPart = ft1.dwHighDateTime;
	li2.LowPart = ft2.dwLowDateTime;
	li2.HighPart = ft2.dwHighDateTime;

	return li1.QuadPart - li2.QuadPart;
}

// SYSTEMTIME �̎��ԍ������߂�(ms�P��)
static LONGLONG DiffSystemTime(const SYSTEMTIME &st1, const SYSTEMTIME &st2)
{
	FILETIME ft1, ft2;

	::SystemTimeToFileTime(&st1, &ft1);
	::SystemTimeToFileTime(&st2, &ft2);

	return DiffFileTime(ft1, ft2) / FILETIME_MS;
}


// �E�B���h�E�N���X��
#define SLEEPTIMER_WINDOW_CLASS TEXT("TVTest SleepTimer Window")


// �v���O�C���N���X
class CSleepTimer : public TVTest::CTVTestPlugin
{
	// �X���[�v����
	enum SleepCondition {
		CONDITION_DURATION,	// ���Ԍo��
		CONDITION_DATETIME,	// �w�莞��
		CONDITION_EVENTEND	// �ԑg�I��
	};

	// �X���[�v���@
	enum SleepMode {
		MODE_EXIT,			// TVTest�I��
		MODE_POWEROFF,		// �d���I�t
		MODE_LOGOFF,		// ���O�I�t
		MODE_SUSPEND,		// �T�X�y���h
		MODE_HIBERNATE		// �n�C�o�l�[�g
	};

	enum {
		TIMER_ID_SLEEP = 1,
		TIMER_ID_QUERY
	};

	bool m_fInitialized;				// �������ς݂�?
	TCHAR m_szIniFileName[MAX_PATH];	// INI�t�@�C���̃p�X
	SleepCondition m_Condition;			// �X���[�v�������
	DWORD m_SleepDuration;				// �X���[�v�܂ł̎���(�b�P��)
	SYSTEMTIME m_SleepDateTime;			// �X���[�v���鎞��
	WORD m_EventID;						// ���݂̔ԑg�� event_id
	SleepMode m_Mode;					// �X���[�v���̓���
	bool m_fForce;						// ����
	bool m_fMonitorOff;					// ���j�^��OFF�ɂ���
	bool m_fIgnoreRecStatus;			// �^�撆�ł��X���[�v����
	bool m_fConfirm;					// �m�F�����
	int m_ConfirmTimeout;				// �m�F�̃^�C���A�E�g����(�b�P��)
	bool m_fShowSettings;				// �v���O�C���L�����ɐݒ�\��
	HWND m_hwnd;						// �E�B���h�E�n���h��
	bool m_fEnabled;					// �v���O�C�����L����?
	int m_ConfirmTimerCount;			// �m�F�̃^�C�}�[

	static const LPCTSTR m_ModeTextList[];

	bool InitializePlugin();
	bool OnEnablePlugin(bool fEnable);
	bool BeginSleep();
	bool DoSleep();
	bool BeginTimer();
	void EndTimer();

	static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);
	static CSleepTimer *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK ConfirmDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	CSleepTimer();
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();
};


const LPCTSTR CSleepTimer::m_ModeTextList[] = {
	TEXT("TVTest ���I��"),
	TEXT("�d���I�t"),
	TEXT("���O�I�t"),
	TEXT("�T�X�y���h"),
	TEXT("�n�C�o�l�[�g"),
};


CSleepTimer::CSleepTimer()
	: m_fInitialized(false)
	, m_Condition(CONDITION_DURATION)
	, m_SleepDuration(30 * 60)
	, m_EventID(0)
	, m_Mode(MODE_EXIT)
	, m_fForce(false)
	, m_fMonitorOff(false)
	, m_fIgnoreRecStatus(false)
	, m_fConfirm(true)
	, m_ConfirmTimeout(10)
	, m_fShowSettings(true)
	, m_hwnd(nullptr)
	, m_fEnabled(false)
{
}


bool CSleepTimer::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	// �v���O�C���̏���Ԃ�
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = TVTest::PLUGIN_FLAG_HASSETTINGS		// �ݒ肠��
	                      | TVTest::PLUGIN_FLAG_DISABLEONSTART;	// �N�����͏�ɖ���
	pInfo->pszPluginName  = L"�X���[�v�^�C�}�[";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"�w������ŏI�������܂��B";
	return true;
}


bool CSleepTimer::Initialize()
{
	// ����������

	// �A�C�R����o�^
	m_pApp->RegisterPluginIconFromResource(g_hinstDLL, MAKEINTRESOURCE(IDB_ICON));

	// �C�x���g�R�[���o�b�N�֐���o�^
	m_pApp->SetEventCallback(EventCallback, this);

	return true;
}


// �v���O�C�����L���ɂ��ꂽ���̏���������
bool CSleepTimer::InitializePlugin()
{
	if (m_fInitialized)
		return true;

	// �ݒ�̓ǂݍ���
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

	// �E�B���h�E�N���X�̓o�^
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

	// �E�B���h�E�̍쐬
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
	// �I������

	m_pApp->EnablePlugin(false);	// ����N�����ɗL���ɂȂ�Ȃ��悤��

	// �E�B���h�E�̔j��
	if (m_hwnd)
		::DestroyWindow(m_hwnd);

	if (m_fInitialized) {
		// �ݒ�̕ۑ�
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
	}

	return true;
}


// �v���O�C���̗L��/�������؂�ւ�������̏���
bool CSleepTimer::OnEnablePlugin(bool fEnable)
{
	InitializePlugin();

	if (fEnable && (m_fShowSettings || m_Condition == CONDITION_DATETIME)) {
		if (::DialogBoxParam(g_hinstDLL, MAKEINTRESOURCE(IDD_SETTINGS),
							 m_pApp->GetAppWindow(), SettingsDlgProc,
							 reinterpret_cast<LPARAM>(this)) != IDOK)
			return false;
	}

	m_fEnabled = fEnable;

	if (m_fEnabled)
		BeginTimer();
	else
		EndTimer();

	return true;
}


// �X���[�v�J�n
bool CSleepTimer::BeginSleep()
{
	m_pApp->AddLog(L"�X���[�v���J�n���܂��B");

	m_pApp->EnablePlugin(false);	// �^�C�}�[�͈�����L��
	EndTimer();		// EventCallback�ŌĂ΂��͂������A�O�̂���

	if (m_fConfirm) {
		// �m�F�_�C�A���O��\��
		if (::DialogBoxParam(g_hinstDLL, MAKEINTRESOURCE(IDD_CONFIRM),
							 m_pApp->GetAppWindow(), ConfirmDlgProc,
							 reinterpret_cast<LPARAM>(this)) != IDOK) {
			m_pApp->AddLog(L"���[�U�[�ɂ���ăX���[�v���L�����Z������܂����B");
			return false;
		}
	}

	if (!m_fIgnoreRecStatus) {
		// �^�撆�̓X���[�v���s���Ȃ�
		TVTest::RecordStatusInfo RecStat;
		if (!m_pApp->GetRecordStatus(&RecStat)) {
			m_pApp->AddLog(L"�^���Ԃ��擾�ł��Ȃ��̂ŃX���[�v���L�����Z������܂����B",
						   TVTest::LOG_TYPE_WARNING);
			return false;
		}
		if (RecStat.Status != TVTest::RECORD_STATUS_NOTRECORDING) {
			m_pApp->AddLog(L"�^�撆�Ȃ̂ŃX���[�v���L�����Z������܂����B");
			return false;
		}
	}

	// �X���[�v���s
	return DoSleep();
}


// �X���[�v���s
bool CSleepTimer::DoSleep()
{
	if (m_Mode!=MODE_EXIT && m_Mode!=MODE_LOGOFF) {
		// �����ݒ�
		HANDLE hToken;

		if (::OpenProcessToken(::GetCurrentProcess(),
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
	case MODE_EXIT:
		if (!m_pApp->Close(m_fForce ? TVTest::CLOSE_EXIT : 0))
			return false;
		if (m_fMonitorOff) {
			// ���j�^��OFF�ɂ���
			::PostMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, 1);
			::Sleep(1000);
			::PostMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, 2);
		}
		break;

	case MODE_POWEROFF:
		if (!::ExitWindowsEx((m_fForce ? EWX_FORCE : 0) | EWX_POWEROFF, 0))
			return false;
		break;

	case MODE_LOGOFF:
		if (!::ExitWindowsEx((m_fForce ? EWX_FORCE : 0) | EWX_LOGOFF, 0))
			return false;
		break;

	case MODE_SUSPEND:
		if (!::SetSuspendState(FALSE, m_fForce, FALSE))
			return false;
		break;

	case MODE_HIBERNATE:
		if (!::SetSuspendState(TRUE, m_fForce, FALSE))
			return false;
		break;

	default:
		return false;
	}

	return true;
}


// �^�C�}�[�J�n
bool CSleepTimer::BeginTimer()
{
	UINT_PTR Result;
	WCHAR szLog[256];

	if (m_Condition == CONDITION_DURATION) {
		::wsprintfW(szLog, L"%lu �b��ɃX���[�v���܂��B", (unsigned long)m_SleepDuration);
		m_pApp->AddLog(szLog);
		Result = ::SetTimer(m_hwnd, TIMER_ID_SLEEP, m_SleepDuration * 1000, nullptr);
	} else if (m_Condition == CONDITION_DATETIME || m_Condition == CONDITION_EVENTEND) {
		if (m_Condition == CONDITION_DATETIME) {
			::wsprintfW(szLog, L"%d/%d/%d %02d:%02d:%02d (UTC) �ɃX���[�v���܂��B",
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


// �^�C�}�[��~
void CSleepTimer::EndTimer()
{
	::KillTimer(m_hwnd, TIMER_ID_SLEEP);
	::KillTimer(m_hwnd, TIMER_ID_QUERY);
}


// �C�x���g�R�[���o�b�N�֐�
// �����C�x���g���N����ƌĂ΂��
LRESULT CALLBACK CSleepTimer::EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
{
	CSleepTimer *pThis = static_cast<CSleepTimer*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// �v���O�C���̗L����Ԃ��ω�����
		return pThis->OnEnablePlugin(lParam1 != 0);

	case TVTest::EVENT_PLUGINSETTINGS:
		// �v���O�C���̐ݒ���s��
		pThis->InitializePlugin();
		return ::DialogBoxParam(g_hinstDLL, MAKEINTRESOURCE(IDD_SETTINGS),
								reinterpret_cast<HWND>(lParam1), SettingsDlgProc,
								reinterpret_cast<LPARAM>(pThis)) == IDOK;
	}

	return 0;
}


// �E�B���h�E�n���h������this���擾����
CSleepTimer *CSleepTimer::GetThis(HWND hwnd)
{
	return reinterpret_cast<CSleepTimer*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
}


// �E�B���h�E�v���V�[�W��
// �P�Ƀ^�C�}�[���������邾��
LRESULT CALLBACK CSleepTimer::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
			CSleepTimer *pThis = static_cast<CSleepTimer*>(pcs->lpCreateParams);

			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
		}
		return TRUE;

	case WM_TIMER:
		{
			CSleepTimer *pThis = GetThis(hwnd);

			if (wParam == TIMER_ID_SLEEP) {
				// �w�莞�Ԃ��o�߂����̂ŃX���[�v�J�n
				pThis->BeginSleep();
			} else if (wParam == TIMER_ID_QUERY) {
				if (pThis->m_Condition == CONDITION_DATETIME) {
					SYSTEMTIME st;

					::GetSystemTime(&st);
					if (DiffSystemTime(st, pThis->m_SleepDateTime) >= 0) {
						// �w�莞���������̂ŃX���[�v�J�n
						pThis->BeginSleep();
					}
				} else if (pThis->m_Condition == CONDITION_EVENTEND) {
					TVTest::ProgramInfo Info = {};
					WCHAR szEventName[128];

					// ���݂̔ԑg�̏����擾
					Info.pszEventName = szEventName;
					Info.MaxEventName = _countof(szEventName);
					if (pThis->m_pApp->GetCurrentProgramInfo(&Info)) {
						if (pThis->m_EventID == 0) {
							bool fSet = false;

							if (Info.Duration == 0) {
								// �I����������
								fSet = true;
							} else {
								FILETIME ft;
								::SystemTimeToFileTime(&Info.StartTime, &ft);
								LARGE_INTEGER li;
								li.LowPart = ft.dwLowDateTime;
								li.HighPart = ft.dwHighDateTime;
								li.QuadPart -= 9LL * FILETIME_HOUR;				// EPG����(UTC+9) -> UTC
								li.QuadPart += Info.Duration * FILETIME_SEC;	// �I������
								ft.dwLowDateTime = li.LowPart;
								ft.dwHighDateTime = li.HighPart;
								FILETIME CurrentTime;
								::GetSystemTimeAsFileTime(&CurrentTime);
								// �ԑg�I����2���ȓ��̏ꍇ�́A���̔ԑg��Ώۂɂ���
								if (DiffFileTime(ft, CurrentTime) > 2LL * FILETIME_MIN)
									fSet = true;
							}

							if (fSet) {
								pThis->m_EventID = Info.EventID;
								pThis->m_pApp->AddLog(L"���̔ԑg���I��������X���[�v���܂��B");
								pThis->m_pApp->AddLog(szEventName);
							}
						} else if (pThis->m_EventID != Info.EventID) {
							// �ԑg���ς�����̂ŃX���[�v�J�n
							pThis->BeginSleep();
						}
					}
				}
			}
		}
		return 0;
	}

	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
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

// �ݒ�_�C�A���O�v���V�[�W��
INT_PTR CALLBACK CSleepTimer::SettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static const LPCTSTR PROP_NAME = TEXT("926EDC25-D769-4166-9DC9-8BB93F958493");

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CSleepTimer *pThis = reinterpret_cast<CSleepTimer*>(lParam);

			::SetProp(hDlg, PROP_NAME, pThis);

			::CheckRadioButton(
				hDlg, IDC_SETTINGS_CONDITION_DURATION, IDC_SETTINGS_CONDITION_EVENTEND,
				IDC_SETTINGS_CONDITION_DURATION + (int)pThis->m_Condition);
			EnableDlgItems(
				hDlg, IDC_SETTINGS_DURATION_HOURS, IDC_SETTINGS_DURATION_SECONDS_UD,
				pThis->m_Condition == CONDITION_DURATION);
			EnableDlgItem(
				hDlg, IDC_SETTINGS_DATETIME, pThis->m_Condition == CONDITION_DATETIME);

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
				CSleepTimer *pThis = static_cast<CSleepTimer*>(::GetProp(hDlg, PROP_NAME));
				SleepCondition Condition;

				if (::IsDlgButtonChecked(hDlg, IDC_SETTINGS_CONDITION_DURATION)) {
					Condition = CONDITION_DURATION;
				} else if (::IsDlgButtonChecked(hDlg, IDC_SETTINGS_CONDITION_DATETIME)) {
					Condition = CONDITION_DATETIME;
				} else if (::IsDlgButtonChecked(hDlg, IDC_SETTINGS_CONDITION_EVENTEND)) {
					Condition = CONDITION_EVENTEND;
				} else {
					::MessageBox(hDlg, TEXT("�X���[�v���������I�����Ă��������B"), nullptr, MB_OK | MB_ICONEXCLAMATION);
					return TRUE;
				}

				ULONGLONG Duration =
					(ULONGLONG)::GetDlgItemInt(hDlg, IDC_SETTINGS_DURATION_HOURS, nullptr, FALSE) * (60 * 60) +
					(ULONGLONG)::GetDlgItemInt(hDlg, IDC_SETTINGS_DURATION_MINUTES, nullptr, FALSE) * 60 +
					(ULONGLONG)::GetDlgItemInt(hDlg, IDC_SETTINGS_DURATION_SECONDS, nullptr, FALSE);
				if (Condition == CONDITION_DURATION) {
					if (Duration * 1000 > USER_TIMER_MAXIMUM) {
						::MessageBox(hDlg, TEXT("�X���[�v�܂ł̎��Ԃ��������܂��B"), nullptr, MB_OK | MB_ICONEXCLAMATION);
						return TRUE;
					}
					if (Duration == 0) {
						::MessageBox(hDlg, TEXT("���Ԃ̎w�肪����������܂���B"), nullptr, MB_OK | MB_ICONEXCLAMATION);
						return TRUE;
					}
				}

				SYSTEMTIME DateTime;
				DWORD Result = DateTime_GetSystemtime(::GetDlgItem(hDlg, IDC_SETTINGS_DATETIME), &DateTime);
				if (Condition == CONDITION_DATETIME) {
					if (Result != GDT_VALID) {
						::MessageBox(hDlg, TEXT("�������w�肵�Ă��������B"), nullptr, MB_OK | MB_ICONEXCLAMATION);
						return TRUE;
					}
					SYSTEMTIME UTCTime;
					::TzSpecificLocalTimeToSystemTime(nullptr, &DateTime, &UTCTime);
					DateTime = UTCTime;
					SYSTEMTIME CurTime;
					::GetSystemTime(&CurTime);
					if (DiffSystemTime(DateTime, CurTime) <= 0) {
						::MessageBox(hDlg, TEXT("�w�肳�ꂽ���������ɉ߂��Ă��܂��B"), nullptr, MB_OK | MB_ICONEXCLAMATION);
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

				// �^�C�}�[�ݒ�
				if (pThis->m_fEnabled)
					pThis->BeginTimer();
			}
		case IDCANCEL:
			::EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_NCDESTROY:
		::RemoveProp(hDlg, PROP_NAME);
		return TRUE;
	}

	return FALSE;
}


// �m�F�_�C�A���O�v���V�[�W��
INT_PTR CALLBACK CSleepTimer::ConfirmDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static const LPCTSTR PROP_NAME = TEXT("E3A245D3-A7D9-499C-95D5-97DF7A666C77");

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CSleepTimer *pThis = reinterpret_cast<CSleepTimer*>(lParam);

			::SetProp(hDlg, PROP_NAME, pThis);

			TCHAR szText[64];
			::wsprintf(szText, TEXT("%s���܂����H"), m_ModeTextList[pThis->m_Mode]);
			::SetDlgItemText(hDlg, IDC_CONFIRM_MODE, szText);

			if (pThis->m_ConfirmTimeout > 0) {
				::SetDlgItemInt(hDlg, IDC_CONFIRM_TIMEOUT, pThis->m_ConfirmTimeout, TRUE);
				pThis->m_ConfirmTimerCount = 0;
				::SetTimer(hDlg, 1, 1000, nullptr);
			} else {
				::SetDlgItemText(hDlg, IDC_CONFIRM_TIMEOUT, TEXT("��"));
			}
		}
		return TRUE;

	case WM_TIMER:
		{
			CSleepTimer *pThis = static_cast<CSleepTimer*>(::GetProp(hDlg, PROP_NAME));

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

	case WM_NCDESTROY:
		::RemoveProp(hDlg, PROP_NAME);
		return TRUE;
	}

	return FALSE;
}




// �v���O�C���N���X�̃C���X�^���X�𐶐�����
TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CSleepTimer;
}
