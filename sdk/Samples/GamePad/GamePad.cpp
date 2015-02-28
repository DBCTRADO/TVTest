/*
	TVTest �v���O�C���T���v��

	�Q�[���p�b�h�ő���ł���悤�ɂ���
*/


#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <dbt.h>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"
#include "resource.h"

#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"shlwapi.lib")


// �R���g���[����
#define CONTROLLER_NAME L"GamePad"

// �{�^���̃��X�g
static TVTest::ControllerButtonInfo g_ButtonList[] = {
	{L"��",			L"VolumeUp"},
	{L"��",			L"VolumeDown"},
	{L"��",			L"ChannelDown"},
	{L"��",			L"ChannelUp"},
	{L"�{�^��1",	NULL},
	{L"�{�^��2",	NULL},
	{L"�{�^��3",	NULL},
	{L"�{�^��4",	NULL},
	{L"�{�^��5",	NULL},
	{L"�{�^��6",	NULL},
	{L"�{�^��7",	NULL},
	{L"�{�^��8",	NULL},
	{L"�{�^��9",	NULL},
	{L"�{�^��10",	NULL},
	{L"�{�^��11",	NULL},
	{L"�{�^��12",	NULL},
	{L"�{�^��13",	NULL},
	{L"�{�^��14",	NULL},
	{L"�{�^��15",	NULL},
	{L"�{�^��16",	NULL},
};

#define NUM_BUTTONS (sizeof(g_ButtonList)/sizeof(g_ButtonList[0]))


// �ΏۃE�B���h�E��ݒ�/�擾����N���X
class CTargetWindow
{
	HANDLE m_hMap;
	HWND *m_phwnd;

public:
	CTargetWindow()
		: m_phwnd(NULL)
	{
		// ���L���������쐬����
		SECURITY_DESCRIPTOR sd;
		SECURITY_ATTRIBUTES sa;
		::ZeroMemory(&sd,sizeof(sd));
		::InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION);
		::SetSecurityDescriptorDacl(&sd,TRUE,NULL,FALSE);
		::ZeroMemory(&sa,sizeof(sa));
		sa.nLength=sizeof(sa);
		sa.lpSecurityDescriptor=&sd;
		m_hMap=::CreateFileMapping(INVALID_HANDLE_VALUE,&sa,
								   PAGE_READWRITE,0,sizeof(HWND),
								   TEXT("TVTest GamePad Target"));
		if (m_hMap!=NULL) {
			bool fExists=::GetLastError()==ERROR_ALREADY_EXISTS;

			m_phwnd=(HWND*)::MapViewOfFile(m_hMap,FILE_MAP_WRITE,0,0,0);
			if (m_phwnd!=NULL) {
				if (!fExists)
					*m_phwnd=NULL;
			} else {
				::CloseHandle(m_hMap);
				m_hMap=NULL;
			}
		}
	}

	~CTargetWindow()
	{
		if (m_hMap!=NULL) {
			::UnmapViewOfFile(m_phwnd);
			::CloseHandle(m_hMap);
		}
	}

	bool SetWindow(HWND hwnd)
	{
		if (m_phwnd==NULL)
			return false;
		*m_phwnd=hwnd;
		return true;
	}

	HWND GetWindow() const
	{
		if (m_phwnd==NULL)
			return NULL;
		return *m_phwnd;
	}
};


// �v���O�C���N���X
class CGamePad : public TVTest::CTVTestPlugin
{
	struct ButtonStatus {
		bool fPressed;
		DWORD PressTime;
		DWORD RepeatCount;
	};

	CTargetWindow m_TargetWindow;	// ����ΏۃE�B���h�E
	HANDLE m_hThread;				// �X���b�h�̃n���h��
	HANDLE m_hEvent;				// �C�x���g�̃n���h��
	ButtonStatus m_ButtonStatus[NUM_BUTTONS];	// �{�^���̏��
	volatile bool m_fInitDevCaps;	// �f�o�C�X���̏�����

	bool Start();
	void End();

	static LRESULT CALLBACK EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData);
	static BOOL CALLBACK MessageCallback(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,
										 LRESULT *pResult,void *pClientData);
	static DWORD WINAPI ThreadProc(LPVOID pParameter);

public:
	CGamePad();
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();
};


CGamePad::CGamePad()
	: m_hThread(NULL)
	, m_hEvent(NULL)
{
}


bool CGamePad::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	// �v���O�C���̏���Ԃ�
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"�Q�[���p�b�h";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"�Q�[���p�b�h�ő���ł���悤�ɂ��܂��B";
	return true;
}


bool CGamePad::Initialize()
{
	// ����������

	// Ini �t�@�C���̃p�X���擾
	TCHAR szIniFileName[MAX_PATH];
	::GetModuleFileName(g_hinstDLL,szIniFileName,MAX_PATH);
	::PathRenameExtension(szIniFileName,TEXT(".ini"));

	// �R���g���[���̓o�^
	TVTest::ControllerInfo Info;
	Info.Flags             = 0;
	Info.pszName           = CONTROLLER_NAME;
	Info.pszText           = L"�Q�[���p�b�h";
	Info.NumButtons        = NUM_BUTTONS;
	Info.pButtonList       = g_ButtonList;
	Info.pszIniFileName    = szIniFileName;
	Info.pszSectionName    = L"Settings";
	Info.ControllerImageID = IDB_CONTROLLER;
	Info.SelButtonsImageID = 0;
	Info.pTranslateMessage = NULL;
	Info.pClientData       = NULL;
	if (!m_pApp->RegisterController(&Info)) {
		m_pApp->AddLog(L"�R���g���[����o�^�ł��܂���B",TVTest::LOG_TYPE_ERROR);
		return false;
	}

	// ���L�������ɃE�B���h�E�n���h����ݒ�
	if (!m_TargetWindow.SetWindow(m_pApp->GetAppWindow())) {
		m_pApp->AddLog(L"���L���������쐬�ł��܂���B",TVTest::LOG_TYPE_ERROR);
		return false;
	}

	// �C�x���g�R�[���o�b�N�֐���o�^
	m_pApp->SetEventCallback(EventCallback,this);

	return true;
}


bool CGamePad::Finalize()
{
	// �I������

	End();

	return true;
}


bool CGamePad::Start()
{
	// �X���b�h�J�n
	if (m_hThread==NULL) {
		if (m_hEvent==NULL) {
			m_hEvent=::CreateEvent(NULL,FALSE,FALSE,NULL);
			if (m_hEvent==NULL)
				return false;
		} else {
			::ResetEvent(m_hEvent);
		}
		m_hThread=::CreateThread(NULL,0,ThreadProc,this,0,NULL);
		if (m_hThread==NULL)
			return false;
	}

	// ���b�Z�[�W�R�[���o�b�N�֐���o�^
	m_pApp->SetWindowMessageCallback(MessageCallback,this);

	return true;
}


void CGamePad::End()
{
	// �X���b�h�I��
	if (m_hThread!=NULL) {
		::SetEvent(m_hEvent);
		if (::WaitForSingleObject(m_hThread,5000)==WAIT_TIMEOUT)
			::TerminateThread(m_hThread,-1);
		::CloseHandle(m_hThread);
		m_hThread=NULL;
	}
	if (m_hEvent!=NULL) {
		::CloseHandle(m_hEvent);
		m_hEvent=NULL;
	}

	// ���b�Z�[�W�R�[���o�b�N�֐���o�^����
	m_pApp->SetWindowMessageCallback(NULL);
}


// �C�x���g�R�[���o�b�N�֐�
// �����C�x���g���N����ƌĂ΂��
LRESULT CALLBACK CGamePad::EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
{
	CGamePad *pThis=static_cast<CGamePad*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// �v���O�C���̗L����Ԃ��ω�����
		if (lParam1!=0) {
			if (!pThis->Start()) {
				pThis->m_pApp->AddLog(L"�������ŃG���[���������܂����B",TVTest::LOG_TYPE_ERROR);
				if (!pThis->m_pApp->GetSilentMode()) {
					::MessageBoxW(pThis->m_pApp->GetAppWindow(),
								  L"�������ŃG���[���������܂����B",
								  L"�Q�[���p�b�h",
								  MB_OK | MB_ICONEXCLAMATION);
				}
				return FALSE;
			}
		} else {
			pThis->End();
		}
		return TRUE;

	case TVTest::EVENT_CONTROLLERFOCUS:
		// ���̃v���Z�X������̑ΏۂɂȂ���
		pThis->m_TargetWindow.SetWindow(reinterpret_cast<HWND>(lParam1));
		return TRUE;
	}

	return 0;
}


// ���b�Z�[�W�R�[���o�b�N�֐�
BOOL CALLBACK CGamePad::MessageCallback(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,
										LRESULT *pResult,void *pClientData)
{
	// �f�o�C�X�̏�Ԃ��ς������ď�����
	if (uMsg==WM_DEVICECHANGE && wParam==DBT_DEVNODES_CHANGED)
		static_cast<CGamePad*>(pClientData)->m_fInitDevCaps=true;
	return FALSE;
}


// �{�^���̏�Ԃ��擾����X���b�h
DWORD WINAPI CGamePad::ThreadProc(LPVOID pParameter)
{
	CGamePad *pThis=static_cast<CGamePad*>(pParameter);

	// �X���b�h�̗D��x�������Ă���
	::SetThreadPriority(::GetCurrentThread(),THREAD_PRIORITY_IDLE);

	// �f�o�C�X�̐����擾
	const UINT NumDevices=::joyGetNumDevs();
	if (NumDevices==0)
		return 0;

	struct JoystickInfo {
		bool fEnable;
		JOYCAPS Caps;
	};
	JoystickInfo *pJoystickList=new JoystickInfo[NumDevices];
	pThis->m_fInitDevCaps=true;

	// �X�e�[�^�X������
	for (int i=0;i<NUM_BUTTONS;i++)
		pThis->m_ButtonStatus[i].fPressed=false;

	JOYINFOEX jix;
	jix.dwSize=sizeof(jix);
	jix.dwFlags=JOY_RETURNBUTTONS | JOY_RETURNX | JOY_RETURNY | JOY_RETURNPOV;

	// �{�^���̏�Ԃ��|�[�����O
	while (::WaitForSingleObject(pThis->m_hEvent,100)==WAIT_TIMEOUT) {
		// ���̃E�B���h�E���Ώۂ�?
		if (pThis->m_TargetWindow.GetWindow()!=pThis->m_pApp->GetAppWindow())
			continue;

		// �e�f�o�C�X�̏����擾����
		if (pThis->m_fInitDevCaps) {
			for (UINT i=0;i<NumDevices;i++) {
				pJoystickList[i].fEnable=
					::joyGetDevCaps(i,&pJoystickList[i].Caps,sizeof(JOYCAPS))==JOYERR_NOERROR;
			}
			pThis->m_fInitDevCaps=false;

#ifdef _DEBUG
			int AvailableDevices=0;
			for (UINT i=0;i<NumDevices;i++) {
				if (pJoystickList[i].fEnable)
					AvailableDevices++;
			}
			TCHAR szText[64];
			::wsprintf(szText,TEXT("Joystick : %d device(s) available\n"),AvailableDevices);
			::OutputDebugString(szText);
#endif
		}

		for (UINT i=0;i<NumDevices;i++) {
			if (pJoystickList[i].fEnable && ::joyGetPosEx(i,&jix)==JOYERR_NOERROR) {
				const JOYCAPS &Caps=pJoystickList[i].Caps;
				const bool fPOV=(Caps.wCaps&(JOYCAPS_HASPOV | JOYCAPS_POV4DIR))!=0;

				for (int j=0;j<NUM_BUTTONS;j++) {
					ButtonStatus &Status=pThis->m_ButtonStatus[j];
					bool fPressed;

					switch (j) {
					case 0:
						fPressed=jix.dwYpos<Caps.wYmin+(Caps.wYmax-Caps.wYmin)/3;
						if (!fPressed && fPOV)
							fPressed=jix.dwPOV==JOY_POVFORWARD;
						break;
					case 1:
						fPressed=jix.dwYpos>Caps.wYmax-(Caps.wYmax-Caps.wYmin)/3;
						if (!fPressed && fPOV)
							fPressed=jix.dwPOV==JOY_POVBACKWARD;
						break;
					case 2:
						fPressed=jix.dwXpos<Caps.wXmin+(Caps.wXmax-Caps.wXmin)/3;
						if (!fPressed && fPOV)
							fPressed=jix.dwPOV==JOY_POVLEFT;
						break;
					case 3:
						fPressed=jix.dwXpos>Caps.wXmax-(Caps.wXmax-Caps.wXmin)/3;
						if (!fPressed && fPOV)
							fPressed=jix.dwPOV==JOY_POVRIGHT;
						break;
					default:
						fPressed=(jix.dwButtons&(1<<(j-4)))!=0;
						break;
					}

					if (fPressed) {
						bool fInput=false;
						const DWORD CurTime=::GetTickCount();
						DWORD Span;

						if (Status.fPressed) {
							if (Status.RepeatCount==0) {
								int Delay;
								if (::SystemParametersInfo(SPI_GETKEYBOARDDELAY,0,&Delay,0))
									Span=(Delay+1)*250;
								else
									Span=500;
							} else {
								DWORD Speed;
								if (::SystemParametersInfo(SPI_GETKEYBOARDSPEED,0,&Speed,0))
									Span=1000/(Speed+2);
								else
									Span=250;
							}
							if ((Status.PressTime<=CurTime?
									(CurTime-Status.PressTime):
									(0xFFFFFFFFUL-Status.PressTime+1+CurTime))>=Span) {
								Status.PressTime=CurTime;
								Status.RepeatCount++;
								fInput=true;
							}
						} else {
							Status.fPressed=true;
							Status.PressTime=CurTime;
							Status.RepeatCount=0;
							fInput=true;
						}
						if (fInput)
							pThis->m_pApp->OnControllerButtonDown(CONTROLLER_NAME,j);
					} else {
						Status.fPressed=false;
					}
				}
				// ���� break �������ΐڑ�����Ă���S�ẴQ�[���p�b�h�̑�����󂯕t����
				break;
			}
		}
	}

	delete [] pJoystickList;

	return 0;
}




// �v���O�C���N���X�̃C���X�^���X�𐶐�����
TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CGamePad;
}
