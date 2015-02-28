/*
	TVTest �v���O�C���T���v��

	�w�莞�Ԍ�ɃX���[�v����
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


// �E�B���h�E�N���X��
#define SLEEPTIMER_WINDOW_CLASS TEXT("TVTest SleepTimer Window")

// �X���[�v�����s����^�C�}�[�̎��ʎq
#define SLEEP_TIMER_ID 1


// �v���O�C���N���X
class CSleepTimer : public TVTest::CTVTestPlugin
{
	bool m_fInitialized;				// �������ς݂�?
	TCHAR m_szIniFileName[MAX_PATH];	// INI�t�@�C���̃p�X
	DWORD m_SleepTime;					// �X���[�v�܂ł̎���(�b�P��)
	enum SleepMode {
		MODE_EXIT,		// TVTest�I��
		MODE_POWEROFF,	// �d���I�t
		MODE_LOGOFF,	// ���O�I�t
		MODE_SUSPEND,	// �T�X�y���h
		MODE_HIBERNATE	// �n�C�o�l�[�g
	};
	SleepMode m_Mode;					// �X���[�v���̓���
	bool m_fForce;						// ����
	bool m_fMonitorOff;					// ���j�^��OFF�ɂ���
	bool m_fConfirm;					// �m�F�����
	int m_ConfirmTimeout;				// �m�F�̃^�C���A�E�g����(�b�P��)
	bool m_fShowSettings;				// �v���O�C���L�����ɐݒ�\��
	HWND m_hwnd;						// �E�B���h�E�n���h��
	bool m_fEnabled;					// �v���O�C�����L����?
	int m_ConfirmTimerCount;			// �m�F�̃^�C�}�[

	bool InitializePlugin();
	bool OnEnablePlugin(bool fEnable);
	static LRESULT CALLBACK EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData);
	static CSleepTimer *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static INT_PTR CALLBACK SettingsDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static INT_PTR CALLBACK ConfirmDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	bool DoSleep();

public:
	CSleepTimer();
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();
};


CSleepTimer::CSleepTimer()
	: m_fInitialized(false)
	, m_SleepTime(600)
	, m_Mode(MODE_EXIT)
	, m_fForce(false)
	, m_fMonitorOff(false)
	, m_fConfirm(true)
	, m_ConfirmTimeout(10)
	, m_fShowSettings(true)
	, m_hwnd(NULL)
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
	pInfo->pszDescription = L"�w�莞�Ԍ�ɏI�������܂��B";
	return true;
}


bool CSleepTimer::Initialize()
{
	// ����������

	// �A�C�R����o�^
	m_pApp->RegisterPluginIconFromResource(g_hinstDLL,MAKEINTRESOURCE(IDB_ICON));

	// �C�x���g�R�[���o�b�N�֐���o�^
	m_pApp->SetEventCallback(EventCallback,this);

	return true;
}


// �v���O�C�����L���ɂ��ꂽ���̏���������
bool CSleepTimer::InitializePlugin()
{
	if (m_fInitialized)
		return true;

	// �ݒ�̓ǂݍ���
	::GetModuleFileName(g_hinstDLL,m_szIniFileName,MAX_PATH);
	::PathRenameExtension(m_szIniFileName,TEXT(".ini"));
	m_SleepTime=::GetPrivateProfileInt(TEXT("Settings"),TEXT("Time"),m_SleepTime,m_szIniFileName);
	m_Mode=(SleepMode)::GetPrivateProfileInt(TEXT("Settings"),TEXT("Mode"),m_Mode,m_szIniFileName);
	m_fForce=::GetPrivateProfileInt(TEXT("Settings"),TEXT("Force"),m_fForce,m_szIniFileName)!=0;
	m_fMonitorOff=::GetPrivateProfileInt(TEXT("Settings"),TEXT("MonitorOff"),m_fMonitorOff,m_szIniFileName)!=0;
	m_fConfirm=::GetPrivateProfileInt(TEXT("Settings"),TEXT("Confirm"),m_fConfirm,m_szIniFileName)!=0;
	m_ConfirmTimeout=::GetPrivateProfileInt(TEXT("Settings"),TEXT("ConfirmTimeout"),m_ConfirmTimeout,m_szIniFileName);
	m_fShowSettings=::GetPrivateProfileInt(TEXT("Settings"),TEXT("ShowSettings"),m_fShowSettings,m_szIniFileName)!=0;

	// �E�B���h�E�N���X�̓o�^
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
	wc.lpszClassName=SLEEPTIMER_WINDOW_CLASS;
	if (::RegisterClass(&wc)==0)
		return false;

	// �E�B���h�E�̍쐬
	m_hwnd=::CreateWindowEx(0,SLEEPTIMER_WINDOW_CLASS,NULL,WS_POPUP,
							0,0,0,0,HWND_MESSAGE,NULL,g_hinstDLL,this);
	if (m_hwnd==NULL)
		return false;

	m_fInitialized=true;
	return true;
}


BOOL WritePrivateProfileInt(LPCTSTR pszAppName,LPCTSTR pszKeyName,int Value,LPCTSTR pszFileName)
{
	TCHAR szValue[16];

	wsprintf(szValue,TEXT("%d"),Value);
	return WritePrivateProfileString(pszAppName,pszKeyName,szValue,pszFileName);
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
		::WritePrivateProfileInt(TEXT("Settings"),TEXT("Time"),m_SleepTime,m_szIniFileName);
		::WritePrivateProfileInt(TEXT("Settings"),TEXT("Mode"),m_Mode,m_szIniFileName);
		::WritePrivateProfileInt(TEXT("Settings"),TEXT("Force"),m_fForce,m_szIniFileName);
		::WritePrivateProfileInt(TEXT("Settings"),TEXT("MonitorOff"),m_fMonitorOff,m_szIniFileName);
		::WritePrivateProfileInt(TEXT("Settings"),TEXT("Confirm"),m_fConfirm,m_szIniFileName);
		::WritePrivateProfileInt(TEXT("Settings"),TEXT("ConfirmTimeout"),m_ConfirmTimeout,m_szIniFileName);
		::WritePrivateProfileInt(TEXT("Settings"),TEXT("ShowSettings"),m_fShowSettings,m_szIniFileName);
	}

	return true;
}


// �v���O�C���̗L��/�������؂�ւ�������̏���
bool CSleepTimer::OnEnablePlugin(bool fEnable)
{
	InitializePlugin();

	if (fEnable && m_fShowSettings) {
		if (::DialogBoxParam(g_hinstDLL,MAKEINTRESOURCE(IDD_SETTINGS),
							 m_pApp->GetAppWindow(),SettingsDlgProc,
							 reinterpret_cast<LPARAM>(this))!=IDOK)
			return false;
	}

	m_fEnabled=fEnable;

	if (m_fEnabled)
		::SetTimer(m_hwnd,SLEEP_TIMER_ID,m_SleepTime*1000,NULL);
	else
		::KillTimer(m_hwnd,SLEEP_TIMER_ID);

	return true;
}


// �X���[�v���s
bool CSleepTimer::DoSleep()
{
	if (m_Mode!=MODE_EXIT && m_Mode!=MODE_LOGOFF) {
		// �����ݒ�
		HANDLE hToken;

		if (::OpenProcessToken(::GetCurrentProcess(),
							   TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,&hToken)) {
			LUID ld;
			LUID_AND_ATTRIBUTES la;
			TOKEN_PRIVILEGES tp;

			::LookupPrivilegeValue(NULL,SE_SHUTDOWN_NAME,&ld);
			la.Luid=ld;
			la.Attributes=SE_PRIVILEGE_ENABLED;
			tp.PrivilegeCount=1;
			tp.Privileges[0]=la;
			::AdjustTokenPrivileges(hToken,FALSE,&tp,0,NULL,NULL);
			::CloseHandle(hToken);
		}
	}

	switch (m_Mode) {
	case MODE_EXIT:
		if (!m_pApp->Close(m_fForce?TVTest::CLOSE_EXIT:0))
			return false;
		if (m_fMonitorOff) {
			// ���j�^��OFF�ɂ���
			::PostMessage(HWND_BROADCAST,WM_SYSCOMMAND,SC_MONITORPOWER,1);
			::Sleep(1000);
			::PostMessage(HWND_BROADCAST,WM_SYSCOMMAND,SC_MONITORPOWER,2);
		}
		break;

	case MODE_POWEROFF:
		if (!::ExitWindowsEx((m_fForce?EWX_FORCE:0) | EWX_POWEROFF,0))
			return false;
		break;

	case MODE_LOGOFF:
		if (!::ExitWindowsEx((m_fForce?EWX_FORCE:0) | EWX_LOGOFF,0))
			return false;
		break;

	case MODE_SUSPEND:
#if 0	// SetSystemPowerState() ���Ǝ������A�ł��Ȃ��Ȃ�݂���
		if (!::SetSystemPowerState(TRUE,m_fForce))
			return false;
#else
		if (!::SetSuspendState(FALSE,m_fForce,FALSE))
			return false;
#endif
		break;

	case MODE_HIBERNATE:
#if 0	// SetSystemPowerState() ���Ǝ������A�ł��Ȃ��Ȃ�݂���
		if (!::SetSystemPowerState(FALSE,m_fForce))
			return false;
#else
		if (!::SetSuspendState(TRUE,m_fForce,FALSE))
			return false;
#endif
		break;

	default:
		return false;
	}

	return true;
}


// �C�x���g�R�[���o�b�N�֐�
// �����C�x���g���N����ƌĂ΂��
LRESULT CALLBACK CSleepTimer::EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
{
	CSleepTimer *pThis=static_cast<CSleepTimer*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// �v���O�C���̗L����Ԃ��ω�����
		return pThis->OnEnablePlugin(lParam1!=0);

	case TVTest::EVENT_PLUGINSETTINGS:
		// �v���O�C���̐ݒ���s��
		pThis->InitializePlugin();
		return ::DialogBoxParam(g_hinstDLL,MAKEINTRESOURCE(IDD_SETTINGS),
								reinterpret_cast<HWND>(lParam1),SettingsDlgProc,
								reinterpret_cast<LPARAM>(pThis))==IDOK;
	}

	return 0;
}


// �E�B���h�E�n���h������this���擾����
CSleepTimer *CSleepTimer::GetThis(HWND hwnd)
{
	return reinterpret_cast<CSleepTimer*>(::GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


// �E�B���h�E�v���V�[�W��
// �P�Ƀ^�C�}�[���������邾��
LRESULT CALLBACK CSleepTimer::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs=reinterpret_cast<LPCREATESTRUCT>(lParam);
			CSleepTimer *pThis=static_cast<CSleepTimer*>(pcs->lpCreateParams);

			::SetWindowLongPtr(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(pThis));
		}
		return TRUE;

	case WM_TIMER:
		{
			CSleepTimer *pThis=GetThis(hwnd);

			pThis->m_pApp->EnablePlugin(false);	// �^�C�}�[�͈�����L��
			::KillTimer(hwnd,wParam);			// EventCallback�ŌĂ΂��͂������A�O�̂���

			if (pThis->m_fConfirm) {
				// �m�F�_�C�A���O��\��
				if (::DialogBoxParam(g_hinstDLL,MAKEINTRESOURCE(IDD_CONFIRM),
									 pThis->m_pApp->GetAppWindow(),ConfirmDlgProc,
									 reinterpret_cast<LPARAM>(pThis))!=IDOK)
					return 0;
			}

			// �^�撆�̓X���[�v���s���Ȃ�
			TVTest::RecordStatusInfo Info;
			if (!pThis->m_pApp->GetRecordStatus(&Info)
					|| Info.Status!=TVTest::RECORD_STATUS_NOTRECORDING) {
				pThis->m_pApp->AddLog(L"�^�撆�Ȃ̂ŃX���[�v���L�����Z������܂����B");
				return 0;
			}

			// �X���[�v���s
			pThis->DoSleep();
		}
		return 0;
	}

	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


static void EnableDlgItems(HWND hDlg,int FirstID,int LastID,BOOL fEnable)
{
	for (int i=FirstID;i<=LastID;i++)
		::EnableWindow(::GetDlgItem(hDlg,i),fEnable);
}

// �ݒ�_�C�A���O�v���V�[�W��
INT_PTR CALLBACK CSleepTimer::SettingsDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static const LPCTSTR PROP_NAME=TEXT("926EDC25-D769-4166-9DC9-8BB93F958493");

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CSleepTimer *pThis=reinterpret_cast<CSleepTimer*>(lParam);
			static LPCTSTR ModeList[] = {
				TEXT("�I��"),
				TEXT("�d���I�t"),
				TEXT("���O�I�t"),
				TEXT("�T�X�y���h"),
				TEXT("�n�C�o�l�[�g"),
			};

			::SetProp(hDlg,PROP_NAME,pThis);

			::SetDlgItemInt(hDlg,IDC_SETTINGS_TIME_HOURS,pThis->m_SleepTime/(60*60),FALSE);
			::SendDlgItemMessage(hDlg,IDC_SETTINGS_TIME_HOURS_UD,UDM_SETRANGE,0,MAKELPARAM(UD_MAXVAL,0));
			::SetDlgItemInt(hDlg,IDC_SETTINGS_TIME_MINUTES,pThis->m_SleepTime/60%60,FALSE);
			::SendDlgItemMessage(hDlg,IDC_SETTINGS_TIME_MINUTES_UD,UDM_SETRANGE,0,MAKELPARAM(59,0));
			::SetDlgItemInt(hDlg,IDC_SETTINGS_TIME_SECONDS,pThis->m_SleepTime%60,FALSE);
			::SendDlgItemMessage(hDlg,IDC_SETTINGS_TIME_SECONDS_UD,UDM_SETRANGE,0,MAKELPARAM(59,0));
			for (int i=0;i<sizeof(ModeList)/sizeof(LPCTSTR);i++)
				::SendDlgItemMessage(hDlg,IDC_SETTINGS_MODE,CB_ADDSTRING,0,(LPARAM)ModeList[i]);
			::SendDlgItemMessage(hDlg,IDC_SETTINGS_MODE,CB_SETCURSEL,(WPARAM)pThis->m_Mode,0);
			::CheckDlgButton(hDlg,IDC_SETTINGS_FORCE,pThis->m_fForce?BST_CHECKED:BST_UNCHECKED);
			::CheckDlgButton(hDlg,IDC_SETTINGS_MONITOROFF,pThis->m_fMonitorOff?BST_CHECKED:BST_UNCHECKED);
			::CheckDlgButton(hDlg,IDC_SETTINGS_CONFIRM,pThis->m_fConfirm?BST_CHECKED:BST_UNCHECKED);
			::SetDlgItemInt(hDlg,IDC_SETTINGS_CONFIRMTIMEOUT,pThis->m_ConfirmTimeout,TRUE);
			::SendDlgItemMessage(hDlg,IDC_SETTINGS_CONFIRMTIMEOUT_UD,UDM_SETRANGE,0,MAKELPARAM(UD_MAXVAL,1));
			EnableDlgItems(hDlg,IDC_SETTINGS_CONFIRMTIMEOUT_LABEL,IDC_SETTINGS_CONFIRMTIMEOUT_UNIT,
						   pThis->m_fConfirm);
			::CheckDlgButton(hDlg,IDC_SETTINGS_SHOWSETTINGS,pThis->m_fShowSettings?BST_CHECKED:BST_UNCHECKED);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SETTINGS_CONFIRM:
			EnableDlgItems(hDlg,IDC_SETTINGS_CONFIRMTIMEOUT_LABEL,IDC_SETTINGS_CONFIRMTIMEOUT_UNIT,
						   ::IsDlgButtonChecked(hDlg,IDC_SETTINGS_CONFIRM)==BST_CHECKED);
			return TRUE;

		case IDOK:
			{
				CSleepTimer *pThis=static_cast<CSleepTimer*>(::GetProp(hDlg,PROP_NAME));

				ULONGLONG Time=
					(ULONGLONG)::GetDlgItemInt(hDlg,IDC_SETTINGS_TIME_HOURS,NULL,FALSE)*(60*60)+
					(ULONGLONG)::GetDlgItemInt(hDlg,IDC_SETTINGS_TIME_MINUTES,NULL,FALSE)*60+
					(ULONGLONG)::GetDlgItemInt(hDlg,IDC_SETTINGS_TIME_SECONDS,NULL,FALSE);
				if (Time*1000>USER_TIMER_MAXIMUM) {
					::MessageBox(hDlg,TEXT("�X���[�v�܂ł̎��Ԃ����������B"),NULL,MB_OK | MB_ICONEXCLAMATION);
					return TRUE;
				}
				if (Time==0) {
					::MessageBox(hDlg,TEXT("���Ԃ̎w�肪����������B"),NULL,MB_OK | MB_ICONEXCLAMATION);
					return TRUE;
				}
				pThis->m_SleepTime=(DWORD)Time;
				pThis->m_Mode=(SleepMode)::SendDlgItemMessage(hDlg,IDC_SETTINGS_MODE,CB_GETCURSEL,0,0);
				pThis->m_fForce=::IsDlgButtonChecked(hDlg,IDC_SETTINGS_FORCE)==BST_CHECKED;
				pThis->m_fMonitorOff=::IsDlgButtonChecked(hDlg,IDC_SETTINGS_MONITOROFF)==BST_CHECKED;
				pThis->m_fConfirm=::IsDlgButtonChecked(hDlg,IDC_SETTINGS_CONFIRM)==BST_CHECKED;
				pThis->m_ConfirmTimeout=::GetDlgItemInt(hDlg,IDC_SETTINGS_CONFIRMTIMEOUT,NULL,FALSE);
				pThis->m_fShowSettings=::IsDlgButtonChecked(hDlg,IDC_SETTINGS_SHOWSETTINGS)==BST_CHECKED;

				// �^�C�}�[�ݒ�
				if (pThis->m_fEnabled)
					::SetTimer(pThis->m_hwnd,SLEEP_TIMER_ID,pThis->m_SleepTime*1000,NULL);
			}
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_NCDESTROY:
		::RemoveProp(hDlg,PROP_NAME);
		return TRUE;
	}

	return FALSE;
}


// �m�F�_�C�A���O�v���V�[�W��
INT_PTR CALLBACK CSleepTimer::ConfirmDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static const LPCTSTR PROP_NAME=TEXT("E3A245D3-A7D9-499C-95D5-97DF7A666C77");

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CSleepTimer *pThis=reinterpret_cast<CSleepTimer*>(lParam);

			::SetProp(hDlg,PROP_NAME,pThis);

			::SetDlgItemInt(hDlg,IDC_CONFIRM_TIMEOUT,pThis->m_ConfirmTimeout,TRUE);
			pThis->m_ConfirmTimerCount=0;
			::SetTimer(hDlg,1,1000,NULL);
		}
		return TRUE;

	case WM_TIMER:
		{
			CSleepTimer *pThis=static_cast<CSleepTimer*>(::GetProp(hDlg,PROP_NAME));

			pThis->m_ConfirmTimerCount++;
			if (pThis->m_ConfirmTimerCount<pThis->m_ConfirmTimeout) {
				::SetDlgItemInt(hDlg,IDC_CONFIRM_TIMEOUT,pThis->m_ConfirmTimeout-pThis->m_ConfirmTimerCount,TRUE);
			} else {
				::EndDialog(hDlg,IDOK);
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
		}
		return TRUE;

	case WM_NCDESTROY:
		::RemoveProp(hDlg,PROP_NAME);
		return TRUE;
	}

	return FALSE;
}




// �v���O�C���N���X�̃C���X�^���X�𐶐�����
TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CSleepTimer;
}
