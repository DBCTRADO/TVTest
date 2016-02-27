#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "resource.h"
#include "Common/DebugDef.h"


using namespace TVTest;


static CAppMain g_App;


const COptionDialog::PageInfo COptionDialog::m_PageList[] = {
	{TEXT("一般"),					&g_App.GeneralOptions,			HELP_ID_OPTIONS_GENERAL},
	{TEXT("表示"),					&g_App.ViewOptions,				HELP_ID_OPTIONS_VIEW},
	{TEXT("OSD"),					&g_App.OSDOptions,				HELP_ID_OPTIONS_OSD},
	{TEXT("ステータスバー"),		&g_App.StatusOptions,			HELP_ID_OPTIONS_STATUSBAR},
	{TEXT("サイドバー"),			&g_App.SideBarOptions,			HELP_ID_OPTIONS_SIDEBAR},
	{TEXT("メニュー"),				&g_App.MenuOptions,				HELP_ID_OPTIONS_MENU},
	{TEXT("パネル"),				&g_App.PanelOptions,			HELP_ID_OPTIONS_PANEL},
	{TEXT("テーマ/配色"),			&g_App.ColorSchemeOptions,		HELP_ID_OPTIONS_COLORSCHEME},
	{TEXT("操作"),					&g_App.OperationOptions,		HELP_ID_OPTIONS_OPERATION},
	{TEXT("キー割り当て"),			&g_App.Accelerator,				HELP_ID_OPTIONS_ACCELERATOR},
	{TEXT("リモコン"),				&g_App.ControllerManager,		HELP_ID_OPTIONS_CONTROLLER},
	{TEXT("BonDriver設定"),			&g_App.DriverOptions,			HELP_ID_OPTIONS_DRIVER},
	{TEXT("映像"),					&g_App.VideoOptions,			HELP_ID_OPTIONS_VIDEO},
	{TEXT("音声"),					&g_App.AudioOptions,			HELP_ID_OPTIONS_AUDIO},
	{TEXT("再生"),					&g_App.PlaybackOptions,			HELP_ID_OPTIONS_PLAYBACK},
	{TEXT("録画"),					&g_App.RecordOptions,			HELP_ID_OPTIONS_RECORD},
	{TEXT("キャプチャ"),			&g_App.CaptureOptions,			HELP_ID_OPTIONS_CAPTURE},
	{TEXT("チャンネルスキャン"),	&g_App.ChannelScan,				HELP_ID_OPTIONS_CHANNELSCAN},
	{TEXT("EPG/番組情報"),			&g_App.EpgOptions,				HELP_ID_OPTIONS_EPG},
	{TEXT("EPG番組表"),				&g_App.ProgramGuideOptions,		HELP_ID_OPTIONS_PROGRAMGUIDE},
	{TEXT("プラグイン"),			&g_App.PluginOptions,			HELP_ID_OPTIONS_PLUGIN},
	{TEXT("TSプロセッサー"),		&g_App.TSProcessorOptions,		HELP_ID_OPTIONS_TSPROCESSOR},
	{TEXT("ログ"),					&g_App.Logger,					HELP_ID_OPTIONS_LOG},
};




CAppMain &GetAppClass()
{
	return g_App;
}




bool CTotTimeAdjuster::BeginAdjust(DWORD TimeOut)
{
	m_TimeOut=TimeOut;
	m_StartTime=::GetTickCount();
	m_PrevTime.wYear=0;
	m_fEnable=true;
	return true;
}


bool CTotTimeAdjuster::AdjustTime()
{
	if (!m_fEnable)
		return false;
	if (TickTimeSpan(m_StartTime,::GetTickCount())>=m_TimeOut) {
		m_fEnable=false;
		return false;
	}

	SYSTEMTIME st;
	if (!g_App.CoreEngine.m_DtvEngine.m_TsAnalyzer.GetTotTime(&st))
		return false;
	if (m_PrevTime.wYear==0) {
		m_PrevTime=st;
		return false;
	} else if (CompareSystemTime(&m_PrevTime,&st)==0) {
		return false;
	}

	bool fOK=false;
	HANDLE hToken;
	if (::OpenProcessToken(::GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&hToken)) {
		LUID luid;
		if (::LookupPrivilegeValue(NULL,SE_SYSTEMTIME_NAME,&luid)) {
			TOKEN_PRIVILEGES tkp;
			tkp.PrivilegeCount=1;
			tkp.Privileges[0].Luid=luid;
			tkp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
			if (::AdjustTokenPrivileges(hToken,FALSE, &tkp,sizeof(TOKEN_PRIVILEGES),NULL,0)
					&& ::GetLastError()==ERROR_SUCCESS) {
				// バッファがあるので少し時刻を戻す
				OffsetSystemTime(&st,-2*TimeConsts::SYSTEMTIME_SECOND);
				if (::SetLocalTime(&st)) {
					g_App.AddLog(TEXT("TOTで時刻合わせを行いました。(%d/%d/%d %d:%02d:%02d)"),
								 st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
					fOK=true;
				}
			}
		}
		::CloseHandle(hToken);
	}
	m_fEnable=false;
	return fOK;
}




void CEpgLoadEventHandler::OnBeginLoad()
{
	TRACE(TEXT("Start EPG file loading ...\n"));
}


void CEpgLoadEventHandler::OnEndLoad(bool fSuccess)
{
	TRACE(TEXT("End EPG file loading : %s\n"),fSuccess?TEXT("Succeeded"):TEXT("Failed"));
	if (fSuccess)
		g_App.MainWindow.PostMessage(WM_APP_EPGLOADED,0,0);
}


void CEpgLoadEventHandler::OnStart()
{
	TRACE(TEXT("Start EDCB data loading ...\n"));
}


void CEpgLoadEventHandler::OnEnd(bool fSuccess,CEventManager *pEventManager)
{
	TRACE(TEXT("End EDCB data loading : %s\n"),fSuccess?TEXT("Succeeded"):TEXT("Failed"));
	if (fSuccess) {
		CEventManager::ServiceList ServiceList;

		if (pEventManager->GetServiceList(&ServiceList)) {
			for (size_t i=0;i<ServiceList.size();i++) {
				g_App.EpgProgramList.UpdateService(pEventManager,&ServiceList[i],
					CEpgProgramList::SERVICE_UPDATE_DATABASE);
			}
			g_App.MainWindow.PostMessage(WM_APP_EPGLOADED,0,0);
		}
	}
}




CServiceUpdateInfo::CServiceUpdateInfo(CDtvEngine *pEngine,CTsAnalyzer *pTsAnalyzer)
{
	CTsAnalyzer::ServiceList ServiceList;

	pTsAnalyzer->GetViewableServiceList(&ServiceList);
	m_NumServices=(int)ServiceList.size();
	m_CurService=-1;
	if (m_NumServices>0) {
		m_pServiceList=new ServiceInfo[m_NumServices];
		for (int i=0;i<m_NumServices;i++) {
			const CTsAnalyzer::ServiceInfo *pServiceInfo=&ServiceList[i];
			m_pServiceList[i].ServiceID=pServiceInfo->ServiceID;
			::lstrcpy(m_pServiceList[i].szServiceName,pServiceInfo->szServiceName);
			m_pServiceList[i].LogoID=pServiceInfo->LogoID;
		}
		WORD ServiceID;
		if (pEngine->GetServiceID(&ServiceID)) {
			for (int i=0;i<m_NumServices;i++) {
				if (m_pServiceList[i].ServiceID==ServiceID) {
					m_CurService=i;
					break;
				}
			}
		}
	} else {
		m_pServiceList=NULL;
	}
	m_NetworkID=pTsAnalyzer->GetNetworkID();
	m_TransportStreamID=pTsAnalyzer->GetTransportStreamID();
	m_fServiceListEmpty=pTsAnalyzer->GetServiceNum()==0;
}


CServiceUpdateInfo::~CServiceUpdateInfo()
{
	delete [] m_pServiceList;
}




// エントリポイント
int APIENTRY _tWinMain(HINSTANCE hInstance,HINSTANCE /*hPrevInstance*/,
					   LPTSTR pszCmdLine,int nCmdShow)
{
	// DLLハイジャック対策
	SetDllDirectory(TEXT(""));

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF/* | _CRTDBG_CHECK_ALWAYS_DF*/);
#else
	CDebugHelper::Initialize();
	CDebugHelper::SetExceptionFilterMode(CDebugHelper::EXCEPTION_FILTER_DIALOG);
#endif

	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

	g_App.AddLog(TEXT("******** ") ABOUT_VERSION_TEXT
#ifdef VERSION_PLATFORM
				 TEXT(" (") VERSION_PLATFORM TEXT(")")
#endif
				 TEXT(" 起動 ********"));

	CoInitializeEx(NULL,COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE | COINIT_SPEED_OVER_MEMORY);

	const int Result=g_App.Main(hInstance,pszCmdLine,nCmdShow);

	CoUninitialize();

	g_App.AddLog(TEXT("******** 終了 ********"));
	if (g_App.CmdLineOptions.m_fSaveLog && !g_App.Logger.GetOutputToFile()) {
		TCHAR szFileName[MAX_PATH];

		if (g_App.Logger.GetDefaultLogFileName(szFileName,lengthof(szFileName)))
			g_App.Logger.SaveToFile(szFileName,true);
	}

	return Result;
}
