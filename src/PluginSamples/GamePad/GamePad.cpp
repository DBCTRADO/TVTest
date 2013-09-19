/*
	TVTest プラグインサンプル

	ゲームパッドで操作できるようにする
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


// コントローラ名
#define CONTROLLER_NAME L"GamePad"

// ボタンのリスト
static TVTest::ControllerButtonInfo g_ButtonList[] = {
	{L"↑",			L"VolumeUp"},
	{L"↓",			L"VolumeDown"},
	{L"←",			L"ChannelDown"},
	{L"→",			L"ChannelUp"},
	{L"ボタン1",	NULL},
	{L"ボタン2",	NULL},
	{L"ボタン3",	NULL},
	{L"ボタン4",	NULL},
	{L"ボタン5",	NULL},
	{L"ボタン6",	NULL},
	{L"ボタン7",	NULL},
	{L"ボタン8",	NULL},
	{L"ボタン9",	NULL},
	{L"ボタン10",	NULL},
	{L"ボタン11",	NULL},
	{L"ボタン12",	NULL},
	{L"ボタン13",	NULL},
	{L"ボタン14",	NULL},
	{L"ボタン15",	NULL},
	{L"ボタン16",	NULL},
};

#define NUM_BUTTONS (sizeof(g_ButtonList)/sizeof(g_ButtonList[0]))


// 対象ウィンドウを設定/取得するクラス
class CTargetWindow
{
	HANDLE m_hMap;
	HWND *m_phwnd;

public:
	CTargetWindow()
		: m_phwnd(NULL)
	{
		// 共有メモリを作成する
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


// プラグインクラス
class CGamePad : public TVTest::CTVTestPlugin
{
	struct ButtonStatus {
		bool fPressed;
		DWORD PressTime;
		DWORD RepeatCount;
	};

	CTargetWindow m_TargetWindow;	// 操作対象ウィンドウ
	HANDLE m_hThread;				// スレッドのハンドル
	HANDLE m_hEvent;				// イベントのハンドル
	ButtonStatus m_ButtonStatus[NUM_BUTTONS];	// ボタンの状態
	volatile bool m_fInitDevCaps;	// デバイス情報の初期化

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
	// プラグインの情報を返す
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"ゲームパッド";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"ゲームパッドで操作できるようにします。";
	return true;
}


bool CGamePad::Initialize()
{
	// 初期化処理

	// Ini ファイルのパスを取得
	TCHAR szIniFileName[MAX_PATH];
	::GetModuleFileName(g_hinstDLL,szIniFileName,MAX_PATH);
	::PathRenameExtension(szIniFileName,TEXT(".ini"));

	// コントローラの登録
	TVTest::ControllerInfo Info;
	Info.Flags             = 0;
	Info.pszName           = CONTROLLER_NAME;
	Info.pszText           = L"ゲームパッド";
	Info.NumButtons        = NUM_BUTTONS;
	Info.pButtonList       = g_ButtonList;
	Info.pszIniFileName    = szIniFileName;
	Info.pszSectionName    = L"Settings";
	Info.ControllerImageID = IDB_CONTROLLER;
	Info.SelButtonsImageID = 0;
	Info.pTranslateMessage = NULL;
	Info.pClientData       = NULL;
	if (!m_pApp->RegisterController(&Info)) {
		m_pApp->AddLog(L"コントローラを登録できません。");
		return false;
	}

	// 共有メモリにウィンドウハンドルを設定
	if (!m_TargetWindow.SetWindow(m_pApp->GetAppWindow())) {
		m_pApp->AddLog(L"共有メモリを作成できません。");
		return false;
	}

	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(EventCallback,this);

	return true;
}


bool CGamePad::Finalize()
{
	// 終了処理

	End();

	return true;
}


bool CGamePad::Start()
{
	// スレッド開始
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

	// メッセージコールバック関数を登録
	m_pApp->SetWindowMessageCallback(MessageCallback,this);

	return true;
}


void CGamePad::End()
{
	// スレッド終了
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

	// メッセージコールバック関数を登録解除
	m_pApp->SetWindowMessageCallback(NULL);
}


// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CGamePad::EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
{
	CGamePad *pThis=static_cast<CGamePad*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// プラグインの有効状態が変化した
		if (lParam1!=0) {
			if (!pThis->Start()) {
				pThis->m_pApp->AddLog(L"初期化でエラーが発生しました。");
				if (!pThis->m_pApp->GetSilentMode()) {
					::MessageBoxW(pThis->m_pApp->GetAppWindow(),
								  L"初期化でエラーが発生しました。",
								  L"ゲームパッド",
								  MB_OK | MB_ICONEXCLAMATION);
				}
				return FALSE;
			}
		} else {
			pThis->End();
		}
		return TRUE;

	case TVTest::EVENT_CONTROLLERFOCUS:
		// このプロセスが操作の対象になった
		pThis->m_TargetWindow.SetWindow(reinterpret_cast<HWND>(lParam1));
		return TRUE;
	}
	return 0;
}


// メッセージコールバック関数
BOOL CALLBACK CGamePad::MessageCallback(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,
										LRESULT *pResult,void *pClientData)
{
	// デバイスの状態が変わったら再初期化
	if (uMsg==WM_DEVICECHANGE && wParam==DBT_DEVNODES_CHANGED)
		static_cast<CGamePad*>(pClientData)->m_fInitDevCaps=true;
	return FALSE;
}


// ボタンの状態を取得するスレッド
DWORD WINAPI CGamePad::ThreadProc(LPVOID pParameter)
{
	CGamePad *pThis=static_cast<CGamePad*>(pParameter);

	// スレッドの優先度を下げておく
	::SetThreadPriority(::GetCurrentThread(),THREAD_PRIORITY_IDLE);

	// デバイスの数を取得
	const UINT NumDevices=::joyGetNumDevs();
	if (NumDevices==0)
		return 0;

	struct JoystickInfo {
		bool fEnable;
		JOYCAPS Caps;
	};
	JoystickInfo *pJoystickList=new JoystickInfo[NumDevices];
	pThis->m_fInitDevCaps=true;

	// ステータス初期化
	for (int i=0;i<NUM_BUTTONS;i++)
		pThis->m_ButtonStatus[i].fPressed=false;

	JOYINFOEX jix;
	jix.dwSize=sizeof(jix);
	jix.dwFlags=JOY_RETURNBUTTONS | JOY_RETURNX | JOY_RETURNY | JOY_RETURNPOV;

	// ボタンの状態をポーリング
	while (::WaitForSingleObject(pThis->m_hEvent,100)==WAIT_TIMEOUT) {
		// このウィンドウが対象か?
		if (pThis->m_TargetWindow.GetWindow()!=pThis->m_pApp->GetAppWindow())
			continue;

		// 各デバイスの情報を取得する
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
				// この break を消せば接続されている全てのゲームパッドの操作を受け付ける
				break;
			}
		}
	}

	delete [] pJoystickList;

	return 0;
}




// プラグインクラスのインスタンスを生成する
TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CGamePad;
}
