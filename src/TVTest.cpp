#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "resource.h"
#include "Common/DebugDef.h"


static TVTest::CAppMain g_App;


namespace TVTest
{


const COptionDialog::PageInfo COptionDialog::m_PageList[] = {
	{TEXT("一般"),               &g_App.GeneralOptions,      HELP_ID_OPTIONS_GENERAL},
	{TEXT("表示"),               &g_App.ViewOptions,         HELP_ID_OPTIONS_VIEW},
	{TEXT("OSD"),                &g_App.OSDOptions,          HELP_ID_OPTIONS_OSD},
	{TEXT("ステータスバー"),     &g_App.StatusOptions,       HELP_ID_OPTIONS_STATUSBAR},
	{TEXT("サイドバー"),         &g_App.SideBarOptions,      HELP_ID_OPTIONS_SIDEBAR},
	{TEXT("メニュー"),           &g_App.MenuOptions,         HELP_ID_OPTIONS_MENU},
	{TEXT("パネル"),             &g_App.PanelOptions,        HELP_ID_OPTIONS_PANEL},
	{TEXT("テーマ/配色"),        &g_App.ColorSchemeOptions,  HELP_ID_OPTIONS_COLORSCHEME},
	{TEXT("操作"),               &g_App.OperationOptions,    HELP_ID_OPTIONS_OPERATION},
	{TEXT("キー割り当て"),       &g_App.Accelerator,         HELP_ID_OPTIONS_ACCELERATOR},
	{TEXT("リモコン"),           &g_App.ControllerManager,   HELP_ID_OPTIONS_CONTROLLER},
	{TEXT("BonDriver設定"),      &g_App.DriverOptions,       HELP_ID_OPTIONS_DRIVER},
	{TEXT("映像"),               &g_App.VideoOptions,        HELP_ID_OPTIONS_VIDEO},
	{TEXT("音声"),               &g_App.AudioOptions,        HELP_ID_OPTIONS_AUDIO},
	{TEXT("再生"),               &g_App.PlaybackOptions,     HELP_ID_OPTIONS_PLAYBACK},
	{TEXT("録画"),               &g_App.RecordOptions,       HELP_ID_OPTIONS_RECORD},
	{TEXT("キャプチャ"),         &g_App.CaptureOptions,      HELP_ID_OPTIONS_CAPTURE},
	{TEXT("チャンネルスキャン"), &g_App.ChannelScan,         HELP_ID_OPTIONS_CHANNELSCAN},
	{TEXT("EPG/番組情報"),       &g_App.EpgOptions,          HELP_ID_OPTIONS_EPG},
	{TEXT("EPG番組表"),          &g_App.ProgramGuideOptions, HELP_ID_OPTIONS_PROGRAMGUIDE},
	{TEXT("プラグイン"),         &g_App.PluginOptions,       HELP_ID_OPTIONS_PLUGIN},
	{TEXT("TSプロセッサー"),     &g_App.TSProcessorOptions,  HELP_ID_OPTIONS_TSPROCESSOR},
	{TEXT("ログ"),               &g_App.Logger,              HELP_ID_OPTIONS_LOG},
};




CAppMain &GetAppClass()
{
	return g_App;
}




bool CTotTimeAdjuster::BeginAdjust(DWORD TimeOut)
{
	m_TimeOut = TimeOut;
	m_StartTime = ::GetTickCount();
	m_PrevTime.wYear = 0;
	m_fEnable = true;
	return true;
}


bool CTotTimeAdjuster::AdjustTime()
{
	if (!m_fEnable)
		return false;
	if (TickTimeSpan(m_StartTime, ::GetTickCount()) >= m_TimeOut) {
		m_fEnable = false;
		return false;
	}

	LibISDB::AnalyzerFilter *pAnalyzer =
		GetAppClass().CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
	if (pAnalyzer == nullptr)
		return false;
	LibISDB::DateTime TOTTime;
	if (!pAnalyzer->GetTOTTime(&TOTTime))
		return false;

	SYSTEMTIME st = TOTTime.ToSYSTEMTIME();
	if (m_PrevTime.wYear == 0) {
		m_PrevTime = st;
		return false;
	} else if (CompareSystemTime(&m_PrevTime, &st) == 0) {
		return false;
	}

	bool fOK = false;
	HANDLE hToken;
	if (::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		LUID luid;
		if (::LookupPrivilegeValue(nullptr, SE_SYSTEMTIME_NAME, &luid)) {
			TOKEN_PRIVILEGES tkp;
			tkp.PrivilegeCount = 1;
			tkp.Privileges[0].Luid = luid;
			tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			if (::AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(TOKEN_PRIVILEGES), nullptr, 0)
					&& ::GetLastError() == ERROR_SUCCESS) {
				// バッファがあるので少し時刻を戻す
				OffsetSystemTime(&st, -2 * TimeConsts::SYSTEMTIME_SECOND);
				if (::SetLocalTime(&st)) {
					g_App.AddLog(
						TEXT("TOTで時刻合わせを行いました。(%d/%d/%d %d:%02d:%02d)"),
						st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
					fOK = true;
				}
			}
		}
		::CloseHandle(hToken);
	}
	m_fEnable = false;
	return fOK;
}




void CEpgLoadEventHandler::OnBeginLoading()
{
	TRACE(TEXT("Start EPG file loading ...\n"));
}


void CEpgLoadEventHandler::OnEndLoading(bool fSuccess)
{
	TRACE(TEXT("End EPG file loading : %s\n"), fSuccess ? TEXT("Succeeded") : TEXT("Failed"));
	if (fSuccess)
		g_App.MainWindow.PostMessage(WM_APP_EPGLOADED, 0, 0);
}


void CEpgLoadEventHandler::OnStart()
{
	TRACE(TEXT("Start EDCB data loading ...\n"));
}


void CEpgLoadEventHandler::OnEnd(bool fSuccess, LibISDB::EPGDatabase *pEPGDatabase)
{
	TRACE(TEXT("End EDCB data loading : %s\n"), fSuccess ? TEXT("Succeeded") : TEXT("Failed"));
	if (fSuccess) {
		if (g_App.EPGDatabase.Merge(pEPGDatabase, LibISDB::EPGDatabase::MergeFlag::Database)) {
			g_App.MainWindow.PostMessage(WM_APP_EPGLOADED, 0, 0);
		}
	}
}




CServiceUpdateInfo::CServiceUpdateInfo(LibISDB::TSEngine *pEngine, LibISDB::AnalyzerFilter *pAnalyzer)
{
	pEngine->GetSelectableServiceList(&m_ServiceList);
	m_CurService = -1;
	if (!m_ServiceList.empty()) {
		WORD ServiceID = pEngine->GetServiceID();
		if (ServiceID != LibISDB::SERVICE_ID_INVALID) {
			for (size_t i = 0; i < m_ServiceList.size(); i++) {
				if (m_ServiceList[i].ServiceID == ServiceID) {
					m_CurService = (int)i;
					break;
				}
			}
		}
	}
	m_NetworkID = pAnalyzer->GetNetworkID();
	m_TransportStreamID = pAnalyzer->GetTransportStreamID();
	m_fServiceListEmpty = pAnalyzer->GetServiceCount() == 0;
}


}	// namespace TVTest




// エントリポイント
int APIENTRY _tWinMain(
	HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/,
	LPTSTR pszCmdLine, int nCmdShow)
{
	// DLLハイジャック対策
	SetDllDirectory(TEXT(""));

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF/* | _CRTDBG_CHECK_ALWAYS_DF*/);
#else
	TVTest::CDebugHelper::Initialize();
	TVTest::CDebugHelper::SetExceptionFilterMode(TVTest::CDebugHelper::ExceptionFilterMode::Dialog);
#endif

	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

	g_App.AddLog(
		TEXT("******** ") ABOUT_VERSION_TEXT TEXT(" (")
#ifdef _DEBUG
		TEXT("Debug")
#else
		TEXT("Release")
#endif
#ifdef VERSION_PLATFORM
		TEXT(" ") VERSION_PLATFORM
#endif
		TEXT(") 起動 ********"));
	g_App.AddLog(TEXT("Work with LibISDB ver.") LIBISDB_VERSION_STRING);
#ifdef _MSC_FULL_VER
	g_App.AddLog(
		TEXT("Compiled with MSVC %d.%d.%d.%d"),
		_MSC_FULL_VER / 10000000, (_MSC_FULL_VER / 100000) % 100, _MSC_FULL_VER % 100000, _MSC_BUILD);
#endif

	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE | COINIT_SPEED_OVER_MEMORY);

	const int Result = g_App.Main(hInstance, pszCmdLine, nCmdShow);

	CoUninitialize();

	g_App.AddLog(TEXT("******** 終了 ********"));
	if (g_App.CmdLineOptions.m_fSaveLog && !g_App.Logger.GetOutputToFile()) {
		TCHAR szFileName[MAX_PATH];

		if (g_App.Logger.GetDefaultLogFileName(szFileName, lengthof(szFileName)))
			g_App.Logger.SaveToFile(szFileName, true);
	}

	return Result;
}
