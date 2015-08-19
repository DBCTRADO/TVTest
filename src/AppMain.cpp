#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "AppUtil.h"
#include "InitialSettings.h"
#include "resource.h"
#include "Common/DebugDef.h"


using namespace TVTest;




static HANDLE GetCurrentThreadHandle()
{
	HANDLE hThread;

	if (!::DuplicateHandle(::GetCurrentProcess(),
						   ::GetCurrentThread(),
						   ::GetCurrentProcess(),
						   &hThread,0,FALSE,DUPLICATE_SAME_ACCESS))
		return NULL;
	return hThread;
}




#ifndef _DEBUG
#define WATCH_EXIT
#endif

#ifdef WATCH_EXIT

class CAppTerminator
{
public:
	CAppTerminator(CAppMain &App);
	bool BeginWatching(DWORD Timeout);
	void EndWatching();
	void Continue();

private:
	CAppMain &m_App;
	HANDLE m_hThread;
	HANDLE m_hEndEvent;
	HANDLE m_hContinueEvent;
	HANDLE m_hMainThread;
	DWORD m_Timeout;

	void Finalize();
	static DWORD WINAPI WatchThread(LPVOID lpParameter);
};


CAppTerminator::CAppTerminator(CAppMain &App)
	: m_App(App)
	, m_hThread(nullptr)
	, m_hEndEvent(nullptr)
	, m_hContinueEvent(nullptr)
	, m_hMainThread(nullptr)
{
}


bool CAppTerminator::BeginWatching(DWORD Timeout)
{
	m_hEndEvent=::CreateEvent(nullptr,TRUE,FALSE,nullptr);
	m_hContinueEvent=::CreateEvent(nullptr,FALSE,FALSE,nullptr);
	if (m_hEndEvent==nullptr || m_hContinueEvent==nullptr) {
		Finalize();
		return false;
	}
	m_hMainThread=GetCurrentThreadHandle();
	m_Timeout=Timeout;
	m_hThread=::CreateThread(nullptr,0,WatchThread,this,0,nullptr);
	if (m_hThread==nullptr) {
		Finalize();
		return false;
	}

	return true;
}


void CAppTerminator::EndWatching()
{
	if (m_hThread!=nullptr) {
		if (::SignalObjectAndWait(m_hEndEvent,m_hThread,5000,FALSE)!=WAIT_OBJECT_0)
			::TerminateThread(m_hThread,-1);
		::CloseHandle(m_hThread);
		m_hThread=nullptr;
	}

	Finalize();
}


void CAppTerminator::Continue()
{
	if (m_hContinueEvent!=nullptr)
		::SetEvent(m_hContinueEvent);
}


void CAppTerminator::Finalize()
{
	if (m_hEndEvent!=nullptr) {
		::CloseHandle(m_hEndEvent);
		m_hEndEvent=nullptr;
	}
	if (m_hContinueEvent!=nullptr) {
		::CloseHandle(m_hContinueEvent);
		m_hContinueEvent=nullptr;
	}
	if (m_hMainThread!=nullptr) {
		::CloseHandle(m_hMainThread);
		m_hMainThread=nullptr;
	}
}


DWORD WINAPI CAppTerminator::WatchThread(LPVOID lpParameter)
{
	CAppTerminator *pThis=static_cast<CAppTerminator*>(lpParameter);
	HANDLE hEvents[2];

	hEvents[0]=pThis->m_hEndEvent;
	hEvents[1]=pThis->m_hContinueEvent;

	for (;;) {
		DWORD Result=::WaitForMultipleObjects(2,hEvents,FALSE,pThis->m_Timeout);
		if (Result==WAIT_OBJECT_0)
			break;

		if (Result==WAIT_TIMEOUT) {
			pThis->m_App.AddLog(
				CLogItem::TYPE_WARNING,
				TEXT("終了処理がタイムアウトしました(%ums)。プロセスを強制的に終了させます。"),
				pThis->m_Timeout);

			if (pThis->m_hMainThread!=nullptr) {
#ifdef WIN_XP_SUPPORT
				auto pCancelSynchronousIo=
					GET_MODULE_FUNCTION(TEXT("kernel32.dll"),CancelSynchronousIo);
				if (pCancelSynchronousIo!=nullptr)
					pCancelSynchronousIo(pThis->m_hMainThread);
#else
				::CancelSynchronousIo(pThis->m_hMainThread);
#endif
			}

			pThis->Finalize();

			::ExitProcess(-1);
		}
	}

	return 0;
}

#endif	// WATCH_EXIT



HICON CAppMain::m_hicoApp=NULL;
HICON CAppMain::m_hicoAppSmall=NULL;


CAppMain::CAppMain()
	: m_hInst(nullptr)
	, Core(*this)
	, UICore(*this)
	, EpgProgramList(&CoreEngine.m_DtvEngine.m_EventManager)
	, MainWindow(*this)
	, SideBar(&CommandList)
	, ChannelDisplay(&EpgProgramList)

	, Epg(EpgProgramList,EventSearchOptions)

	, OSDManager(&OSDOptions)
	, StatusOptions(&StatusView)
	, SideBarOptions(&SideBar,&ZoomOptions)
	, ProgramGuideOptions(&Epg.ProgramGuide,&PluginManager)
	, PluginOptions(&PluginManager)
	, TSProcessorOptions(TSProcessorManager)
	, FeaturedEvents(EventSearchOptions)

	, m_fFirstExecute(false)

	, m_DtvEngineHandler(*this)
	, m_StreamInfoEventHandler(*this)
	, m_CaptureWindowEventHandler(*this)

	, m_ExitTimeout(60000)
	, m_fEnablePlaybackOnStart(true)
	, m_fIncrementNetworkPort(true)
{
	UICore.SetSkin(&MainWindow);

	AppEventManager.AddEventHandler(&PluginManager);

	StreamInfo.SetEventHandler(&m_StreamInfoEventHandler);
	CaptureWindow.SetEventHandler(&m_CaptureWindowEventHandler);
}


CAppMain::~CAppMain()
{
	if (m_hicoApp!=NULL) {
		::DestroyIcon(m_hicoApp);
		m_hicoApp=NULL;
	}
	if (m_hicoAppSmall!=NULL) {
		::DestroyIcon(m_hicoAppSmall);
		m_hicoAppSmall=NULL;
	}
}


HINSTANCE CAppMain::GetInstance() const
{
	return m_hInst;
}


HINSTANCE CAppMain::GetResourceInstance() const
{
	return m_hInst;
}


bool CAppMain::GetAppDirectory(LPTSTR pszDirectory) const
{
	if (::GetModuleFileName(nullptr,pszDirectory,MAX_PATH)==0)
		return false;
	CFilePath FilePath(pszDirectory);
	FilePath.RemoveFileName();
	FilePath.GetPath(pszDirectory);
	return true;
}


void CAppMain::AddLog(CLogItem::LogType Type,LPCTSTR pszText, ...)
{
	va_list Args;

	va_start(Args,pszText);
	Logger.AddLogV(Type,pszText,Args);
	va_end(Args);
}


void CAppMain::AddLog(LPCTSTR pszText, ...)
{
	va_list Args;

	va_start(Args,pszText);
	Logger.AddLogV(CLogItem::TYPE_INFORMATION,pszText,Args);
	va_end(Args);
}


bool CAppMain::IsFirstExecute() const
{
	return m_fFirstExecute;
}


void CAppMain::Initialize()
{
	TCHAR szModuleFileName[MAX_PATH];

	::GetModuleFileName(nullptr,szModuleFileName,MAX_PATH);
	if (CmdLineOptions.m_IniFileName.empty()) {
		::lstrcpy(m_szIniFileName,szModuleFileName);
		::PathRenameExtension(m_szIniFileName,TEXT(".ini"));
	} else {
		LPCTSTR pszIniFileName=CmdLineOptions.m_IniFileName.c_str();
		if (::PathIsRelative(pszIniFileName)) {
			TCHAR szTemp[MAX_PATH];
			::lstrcpy(szTemp,szModuleFileName);
			::lstrcpy(::PathFindFileName(szTemp),pszIniFileName);
			::PathCanonicalize(m_szIniFileName,szTemp);
		} else {
			::lstrcpy(m_szIniFileName,pszIniFileName);
		}
	}
	::lstrcpy(m_szFavoritesFileName,szModuleFileName);
	::PathRenameExtension(m_szFavoritesFileName,TEXT(".tvfavorites"));

	bool fExists=::PathFileExists(m_szIniFileName)!=FALSE;
	m_fFirstExecute=!fExists && CmdLineOptions.m_IniFileName.empty();
	if (fExists) {
		AddLog(TEXT("設定を読み込んでいます..."));
		LoadSettings();
	}
}


void CAppMain::Finalize()
{
#ifdef WATCH_EXIT
	CAppTerminator Terminator(*this);
	if (m_ExitTimeout!=0)
		Terminator.BeginWatching(m_ExitTimeout);
#define FINALIZE_CONTINUE Terminator.Continue();
#else
#define FINALIZE_CONTINUE
#endif

	TaskTrayManager.Finalize();
	ChannelMenu.Destroy();
	FavoritesMenu.Destroy();
	MainMenu.Destroy();
	Accelerator.Finalize();
	ControllerManager.DeleteAllControllers();
	TaskbarManager.Finalize();
	Epg.ProgramGuideFrame.Destroy();
	NotifyBalloonTip.Finalize();

	Core.SaveCurrentChannel();

	FINALIZE_CONTINUE

	CoreEngine.Close();
	CoreEngine.m_DtvEngine.SetTracer(nullptr);

	FINALIZE_CONTINUE

	if (!CmdLineOptions.m_fNoPlugin)
		PluginOptions.StorePluginOptions();
	PluginManager.FreePlugins();

	FINALIZE_CONTINUE

	// 終了時の負荷で他のプロセスの録画がドロップすることがあるらしい...
	::SetPriorityClass(::GetCurrentProcess(),BELOW_NORMAL_PRIORITY_CLASS);

	if (!CmdLineOptions.m_fNoEpg) {
		EpgProgramList.UpdateProgramList(CEpgProgramList::SERVICE_UPDATE_DISCARD_ENDED_EVENTS);
		EpgOptions.SaveEpgFile(&EpgProgramList);
	}

	FINALIZE_CONTINUE

	EpgOptions.SaveLogoFile();
	EpgOptions.Finalize();

#ifdef WATCH_EXIT
	Terminator.EndWatching();
#endif

	if (FavoritesManager.GetModified())
		FavoritesManager.Save(m_szFavoritesFileName);

	AddLog(TEXT("設定を保存しています..."));
	SaveSettings(SETTINGS_SAVE_STATUS);
}


void CAppMain::Exit()
{
	UICore.DoCommandAsync(CM_EXIT);
}


bool CAppMain::LoadSettings()
{
	CSettings &Settings=m_Settings;

	if (!Settings.Open(m_szIniFileName,CSettings::OPEN_READ | CSettings::OPEN_WRITE_VOLATILE)) {
		AddLog(CLogItem::TYPE_ERROR,TEXT("設定ファイル \"%s\" を開けません。"),m_szIniFileName);
		return false;
	}

	if (!CmdLineOptions.m_IniValueList.empty()) {
		for (size_t i=0;i<CmdLineOptions.m_IniValueList.size();i++) {
			const CCommandLineOptions::IniEntry &Entry=CmdLineOptions.m_IniValueList[i];

			TRACE(TEXT("Override INI entry : [%s] %s=%s\n"),
				  Entry.Section.c_str(),Entry.Name.c_str(),Entry.Value.c_str());
			if (Settings.SetSection(Entry.Section.empty()?TEXT("Settings"):Entry.Section.c_str())) {
				Settings.Write(Entry.Name.c_str(),Entry.Value);
			}
		}
	}

	if (Settings.SetSection(TEXT("Settings"))) {
		int Value;
		TCHAR szText[MAX_PATH];
		int Left,Top,Width,Height;
		bool f;

		Settings.Read(TEXT("EnablePlay"),&m_fEnablePlaybackOnStart);
		if (Settings.Read(TEXT("Volume"),&Value))
			CoreEngine.SetVolume(CLAMP(Value,0,CCoreEngine::MAX_VOLUME));
		int Gain=100,SurroundGain;
		Settings.Read(TEXT("VolumeNormalizeLevel"),&Gain);
		if (!Settings.Read(TEXT("SurroundGain"),&SurroundGain))
			SurroundGain=Gain;
		CoreEngine.SetAudioGainControl(Gain,SurroundGain);
		Settings.Read(TEXT("ShowInfoWindow"),&Panel.fShowPanelWindow);
		if (Settings.Read(TEXT("OptionPage"),&Value))
			m_OptionDialog.SetCurrentPage(Value);

		if (Settings.Read(TEXT("RecOptionFileName"),szText,MAX_PATH) && szText[0]!='\0')
			RecordManager.SetFileName(szText);
		/*
		if (Settings.Read(TEXT("RecOptionExistsOperation"),&Value))
			RecordManager.SetFileExistsOperation(
								(CRecordManager::FileExistsOperation)Value);
		*/
		/*
		if (Settings.Read(TEXT("RecOptionStopTimeSpec"),&f))
			RecordManager.SetStopTimeSpec(f);
		unsigned int Time;
		if (Settings.Read(TEXT("RecOptionStopTime"),&Time))
			RecordManager.SetStopTime(Time);
		*/

		Panel.Frame.GetPosition(&Left,&Top,&Width,&Height);
		Settings.Read(TEXT("InfoLeft"),&Left);
		Settings.Read(TEXT("InfoTop"),&Top);
		Settings.Read(TEXT("InfoWidth"),&Width);
		Settings.Read(TEXT("InfoHeight"),&Height);
		Panel.Frame.SetPosition(Left,Top,Width,Height);
		Panel.Frame.MoveToMonitorInside();
		if (Settings.Read(TEXT("PanelFloating"),&f))
			Panel.Frame.SetFloating(f);
		if (Settings.Read(TEXT("PanelDockingWidth"),&Value))
			Panel.Frame.SetDockingWidth(Value);
		if (Settings.Read(TEXT("PanelDockingHeight"),&Value))
			Panel.Frame.SetDockingHeight(Value);

		Epg.ProgramGuideFrame.GetPosition(&Left,&Top,&Width,&Height);
		Settings.Read(TEXT("ProgramGuideLeft"),&Left);
		Settings.Read(TEXT("ProgramGuideTop"),&Top);
		Settings.Read(TEXT("ProgramGuideWidth"),&Width);
		Settings.Read(TEXT("ProgramGuideHeight"),&Height);
		Epg.ProgramGuideFrame.SetPosition(Left,Top,Width,Height);
		Epg.ProgramGuideFrame.MoveToMonitorInside();
		if (Settings.Read(TEXT("ProgramGuideMaximized"),&f) && f)
			Epg.ProgramGuideFrame.SetMaximize(f);
		if (Settings.Read(TEXT("ProgramGuideAlwaysOnTop"),&f))
			Epg.ProgramGuideFrame.SetAlwaysOnTop(f);

		CaptureWindow.GetPosition(&Left,&Top,&Width,&Height);
		Settings.Read(TEXT("CapturePreviewLeft"),&Left);
		Settings.Read(TEXT("CapturePreviewTop"),&Top);
		Settings.Read(TEXT("CapturePreviewWidth"),&Width);
		Settings.Read(TEXT("CapturePreviewHeight"),&Height);
		CaptureWindow.SetPosition(Left,Top,Width,Height);
		CaptureWindow.MoveToMonitorInside();
		if (Settings.Read(TEXT("CaptureStatusBar"),&f))
			CaptureWindow.ShowStatusBar(f);

		StreamInfo.GetPosition(&Left,&Top,&Width,&Height);
		Settings.Read(TEXT("StreamInfoLeft"),&Left);
		Settings.Read(TEXT("StreamInfoTop"),&Top);
		Settings.Read(TEXT("StreamInfoWidth"),&Width);
		Settings.Read(TEXT("StreamInfoHeight"),&Height);
		StreamInfo.SetPosition(Left,Top,Width,Height);
		//StreamInfo.MoveToMonitorInside();

		CBasicDialog::Position Pos;
		Settings.Read(TEXT("OrganizeFavoritesLeft"),&Pos.x);
		Settings.Read(TEXT("OrganizeFavoritesTop"),&Pos.y);
		Settings.Read(TEXT("OrganizeFavoritesWidth"),&Pos.Width);
		Settings.Read(TEXT("OrganizeFavoritesHeight"),&Pos.Height);
		FavoritesManager.SetOrganizeDialogPos(Pos);

		Settings.Read(TEXT("ExitTimeout"),&m_ExitTimeout);
		Settings.Read(TEXT("IncrementUDPPort"),&m_fIncrementNetworkPort);

		MainWindow.ReadSettings(Settings);
		GeneralOptions.ReadSettings(Settings);
		ViewOptions.ReadSettings(Settings);
		OSDOptions.ReadSettings(Settings);
		PanelOptions.ReadSettings(Settings);
		PlaybackOptions.ReadSettings(Settings);
		VideoOptions.ReadSettings(Settings);
		AudioOptions.ReadSettings(Settings);
		RecordOptions.ReadSettings(Settings);
		CaptureOptions.ReadSettings(Settings);
		ControllerManager.ReadSettings(Settings);
		ChannelScan.ReadSettings(Settings);
		EpgOptions.ReadSettings(Settings);
		Logger.ReadSettings(Settings);
		ZoomOptions.ReadSettings(Settings);
	}

	StatusOptions.LoadSettings(Settings);
	SideBarOptions.LoadSettings(Settings);
	MenuOptions.LoadSettings(Settings);
	ColorSchemeOptions.LoadSettings(Settings);
	DriverOptions.LoadSettings(Settings);
	ProgramGuideOptions.LoadSettings(Settings);
	Epg.ProgramGuideFrameSettings.LoadSettings(Settings);
	PluginOptions.LoadSettings(Settings);
	TaskbarOptions.LoadSettings(Settings);
	RecentChannelList.LoadSettings(Settings);
	Panel.InfoPanel.LoadSettings(Settings);
	Panel.ProgramListPanel.LoadSettings(Settings);
	Panel.ChannelPanel.LoadSettings(Settings);
	Panel.CaptionPanel.LoadSettings(Settings);
	PanAndScanOptions.LoadSettings(Settings);
	HomeDisplay.LoadSettings(Settings);
	NetworkDefinition.LoadSettings(Settings);
	TSProcessorOptions.LoadSettings(Settings);
	FeaturedEvents.LoadSettings(Settings);
	TaskbarManager.LoadSettings(Settings);

	return true;
}


bool CAppMain::SaveSettings(unsigned int Flags)
{
	CSettings Settings;
	if (!Settings.Open(m_szIniFileName,CSettings::OPEN_WRITE)) {
		TCHAR szMessage[64+MAX_PATH];
		StdUtil::snprintf(szMessage,lengthof(szMessage),
						  TEXT("設定ファイル \"%s\" を開けません。"),
						  m_szIniFileName);
		AddLog(CLogItem::TYPE_ERROR,TEXT("%s"),szMessage);
		if (!Core.IsSilent())
			UICore.GetSkin()->ShowErrorMessage(szMessage);
		return false;
	}

	if (Settings.SetSection(TEXT("Settings"))) {
		Settings.Write(TEXT("Version"),VERSION_TEXT);

		if ((Flags&SETTINGS_SAVE_STATUS)!=0) {
			int Left,Top,Width,Height;

			Settings.Write(TEXT("EnablePlay"),m_fEnablePlaybackOnStart);
			Settings.Write(TEXT("Volume"),CoreEngine.GetVolume());
			int Gain,SurroundGain;
			CoreEngine.GetAudioGainControl(&Gain,&SurroundGain);
			Settings.Write(TEXT("VolumeNormalizeLevel"),Gain);
			Settings.Write(TEXT("SurroundGain"),SurroundGain);
			Settings.Write(TEXT("ShowInfoWindow"),Panel.fShowPanelWindow);
			Settings.Write(TEXT("OptionPage"),m_OptionDialog.GetCurrentPage());

			if (RecordManager.GetFileName()!=nullptr)
				Settings.Write(TEXT("RecOptionFileName"),RecordManager.GetFileName());
			/*
			Settings.Write(TEXT("RecOptionExistsOperation"),
						   RecordManager.GetFileExistsOperation());
			*/
			/*
			Settings.Write(TEXT("RecOptionStopTimeSpec"),RecordManager.GetStopTimeSpec());
			Settings.Write(TEXT("RecOptionStopTime"),RecordManager.GetStopTime());
			*/

			Panel.Frame.GetPosition(&Left,&Top,&Width,&Height);
			Settings.Write(TEXT("InfoLeft"),Left);
			Settings.Write(TEXT("InfoTop"),Top);
			Settings.Write(TEXT("InfoWidth"),Width);
			Settings.Write(TEXT("InfoHeight"),Height);
			Settings.Write(TEXT("PanelFloating"),Panel.Frame.GetFloating());
			Settings.Write(TEXT("PanelDockingWidth"),Panel.Frame.GetDockingWidth());
			Settings.Write(TEXT("PanelDockingHeight"),Panel.Frame.GetDockingHeight());

			Epg.ProgramGuideFrame.GetPosition(&Left,&Top,&Width,&Height);
			Settings.Write(TEXT("ProgramGuideLeft"),Left);
			Settings.Write(TEXT("ProgramGuideTop"),Top);
			Settings.Write(TEXT("ProgramGuideWidth"),Width);
			Settings.Write(TEXT("ProgramGuideHeight"),Height);
			Settings.Write(TEXT("ProgramGuideMaximized"),Epg.ProgramGuideFrame.GetMaximize());
			Settings.Write(TEXT("ProgramGuideAlwaysOnTop"),Epg.ProgramGuideFrame.GetAlwaysOnTop());

			CaptureWindow.GetPosition(&Left,&Top,&Width,&Height);
			Settings.Write(TEXT("CapturePreviewLeft"),Left);
			Settings.Write(TEXT("CapturePreviewTop"),Top);
			Settings.Write(TEXT("CapturePreviewWidth"),Width);
			Settings.Write(TEXT("CapturePreviewHeight"),Height);
			Settings.Write(TEXT("CaptureStatusBar"),CaptureWindow.IsStatusBarVisible());

			StreamInfo.GetPosition(&Left,&Top,&Width,&Height);
			Settings.Write(TEXT("StreamInfoLeft"),Left);
			Settings.Write(TEXT("StreamInfoTop"),Top);
			Settings.Write(TEXT("StreamInfoWidth"),Width);
			Settings.Write(TEXT("StreamInfoHeight"),Height);

			const CBasicDialog::Position Pos=FavoritesManager.GetOrganizeDialogPos();
			Settings.Write(TEXT("OrganizeFavoritesLeft"),Pos.x);
			Settings.Write(TEXT("OrganizeFavoritesTop"),Pos.y);
			Settings.Write(TEXT("OrganizeFavoritesWidth"),Pos.Width);
			Settings.Write(TEXT("OrganizeFavoritesHeight"),Pos.Height);

			//Settings.Write(TEXT("ExitTimeout"),m_ExitTimeout);
			//Settings.Write(TEXT("IncrementUDPPort"),m_fIncrementNetworkPort);

			MainWindow.WriteSettings(Settings);
		}
	}

	static const struct {
		CSettingsBase *pSettings;
		bool fHasStatus;
	} SettingsList[] = {
		{&GeneralOptions,					true},
		{&StatusOptions,					true},
		{&SideBarOptions,					true},
		{&PanelOptions,						true},
		{&DriverOptions,					true},
		{&VideoOptions,						false},
		{&AudioOptions,						false},
		{&PlaybackOptions,					true},
		{&RecordOptions,					true},
		{&CaptureOptions,					true},
		{&PluginOptions,					true},
		{&ViewOptions,						false},
		{&OSDOptions,						false},
		{&MenuOptions,						false},
		{&ColorSchemeOptions,				false},
		{&OperationOptions,					false},
		{&Accelerator,						false},
		{&ControllerManager,				false},
		{&ChannelScan,						false},
		{&EpgOptions,						false},
		{&ProgramGuideOptions,				true},
		{&TSProcessorOptions,				true},
		{&TaskbarOptions,					false},
		{&Logger,							false},
	//	{&ZoomOptions,						false},
	//	{&PanAndScanOptions,				false},
		{&Epg.ProgramGuideFrameSettings,	true},
		{&HomeDisplay,						true},
		{&FeaturedEvents,					true},
	};

	for (size_t i=0;i<lengthof(SettingsList);i++) {
		CSettingsBase *pSettings=SettingsList[i].pSettings;
		if (((Flags&SETTINGS_SAVE_OPTIONS)!=0 && pSettings->IsChanged())
				|| ((Flags&SETTINGS_SAVE_STATUS)!=0 && SettingsList[i].fHasStatus)) {
			if (pSettings->SaveSettings(Settings))
				pSettings->ClearChanged();
		}
	}

	if ((Flags&SETTINGS_SAVE_STATUS)!=0) {
		RecentChannelList.SaveSettings(Settings);
		Panel.InfoPanel.SaveSettings(Settings);
		Panel.ProgramListPanel.SaveSettings(Settings);
		Panel.ChannelPanel.SaveSettings(Settings);
		Panel.CaptionPanel.SaveSettings(Settings);
		TaskbarManager.SaveSettings(Settings);
	}

	return true;
}


int CAppMain::Main(HINSTANCE hInstance,LPCTSTR pszCmdLine,int nCmdShow)
{
	m_hInst=hInstance;

	// コマンドラインの解析
	if (pszCmdLine[0]!=_T('\0')) {
		AddLog(TEXT("コマンドラインオプション : %s"),pszCmdLine);

		CmdLineOptions.Parse(pszCmdLine);

		if (CmdLineOptions.m_TvRockDID>=0)
			CmdLineOptions.m_fSilent=true;
		if (CmdLineOptions.m_fSilent)
			Core.SetSilent(true);
		if (CmdLineOptions.m_fTray)
			CmdLineOptions.m_fMinimize=true;

		if (CmdLineOptions.m_fProgramGuideOnly) {
			CmdLineOptions.m_fShowProgramGuide=true;
			CmdLineOptions.m_fStandby=true;
		}

		if (CmdLineOptions.m_fMpeg2 || CmdLineOptions.m_fH264 || CmdLineOptions.m_fH265) {
			CoreEngine.m_DtvEngine.m_TsAnalyzer.SetVideoStreamTypeViewable(
				STREAM_TYPE_MPEG2_VIDEO,CmdLineOptions.m_fMpeg2);
			CoreEngine.m_DtvEngine.m_TsAnalyzer.SetVideoStreamTypeViewable(
				STREAM_TYPE_H264,CmdLineOptions.m_fH264);
			CoreEngine.m_DtvEngine.m_TsAnalyzer.SetVideoStreamTypeViewable(
				STREAM_TYPE_H265,CmdLineOptions.m_fH265);
		}
	}

	Initialize();

	CAppMutex AppMutex(true);

	if (CmdLineOptions.m_fJumpList && TaskbarOptions.GetJumpListKeepSingleTask())
		CmdLineOptions.m_fSingleTask=true;

	// 複数起動のチェック
	if (AppMutex.AlreadyExists()
			&& (GeneralOptions.GetKeepSingleTask() || CmdLineOptions.m_fSingleTask)) {
		AddLog(TEXT("複数起動が禁止されています。"));
		CTVTestWindowFinder Finder;
		HWND hwnd=Finder.FindCommandLineTarget();
		if (::IsWindow(hwnd)) {
			if (!SendInterprocessMessage(hwnd,PROCESS_MESSAGE_EXECUTE,
										 pszCmdLine,(::lstrlen(pszCmdLine)+1)*sizeof(TCHAR))) {
				AddLog(CLogItem::TYPE_ERROR,TEXT("既存のプロセスにメッセージを送信できません。"));
			}
			return 0;
		}
		if (!CmdLineOptions.m_fSingleTask) {
			if (!Core.IsSilent()) {
				::MessageBox(nullptr,
					APP_NAME TEXT(" は既に起動しています。\n")
					TEXT("ウィンドウが見当たらない場合はタスクマネージャに隠れていますので\n")
					TEXT("強制終了させてください。"),
					APP_NAME,
					MB_OK | MB_ICONEXCLAMATION);
			}
			return 0;
		}
	}

	// コモンコントロールの初期化
	{
		INITCOMMONCONTROLSEX iccex;

		iccex.dwSize=sizeof(INITCOMMONCONTROLSEX);
		iccex.dwICC=ICC_UPDOWN_CLASS | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_DATE_CLASSES | ICC_PROGRESS_CLASS;
		::InitCommonControlsEx(&iccex);
	}

	// Buffered paint の初期化
	CBufferedPaint::Initialize();

	// BonDriver の検索
	{
		TCHAR szDirectory[MAX_PATH];
		CoreEngine.GetDriverDirectory(szDirectory,lengthof(szDirectory));
		DriverManager.Find(szDirectory);
	}
	// チューナー仕様定義の読み込み
	{
		TCHAR szTunerSpecFileName[MAX_PATH];
		::GetModuleFileName(nullptr,szTunerSpecFileName,lengthof(szTunerSpecFileName));
		::PathRenameExtension(szTunerSpecFileName,TEXT(".tuner.ini"));
		if (::PathFileExists(szTunerSpecFileName)) {
			AddLog(TEXT("チューナー仕様定義を \"%s\" から読み込みます..."),szTunerSpecFileName);
			DriverManager.LoadTunerSpec(szTunerSpecFileName);
		}
	}
	DriverOptions.Initialize(&DriverManager);

	// 初期設定ダイアログを表示するか
	const bool fInitialSettings=
		CmdLineOptions.m_fInitialSettings
			|| (m_fFirstExecute && CmdLineOptions.m_DriverName.empty());

	GraphicsCore.Initialize();

	TCHAR szDriverFileName[MAX_PATH];

	// 初期設定ダイアログの表示
	if (fInitialSettings) {
		CInitialSettings InitialSettings(&DriverManager);

		if (!InitialSettings.Show(nullptr))
			return 0;
		InitialSettings.GetDriverFileName(szDriverFileName,lengthof(szDriverFileName));
		GeneralOptions.SetDefaultDriverName(szDriverFileName);
		VideoOptions.SetMpeg2DecoderName(InitialSettings.GetMpeg2DecoderName());
		VideoOptions.SetH264DecoderName(InitialSettings.GetH264DecoderName());
		VideoOptions.SetH265DecoderName(InitialSettings.GetH265DecoderName());
		VideoOptions.SetVideoRendererType(InitialSettings.GetVideoRenderer());
		RecordOptions.SetSaveFolder(InitialSettings.GetRecordFolder());
	} else if (!CmdLineOptions.m_DriverName.empty()) {
		::lstrcpy(szDriverFileName,CmdLineOptions.m_DriverName.c_str());
	} else if (CmdLineOptions.m_fNoDriver) {
		szDriverFileName[0]=_T('\0');
	} else {
		GeneralOptions.GetFirstDriverName(szDriverFileName);
	}

	// スタイル設定の読み込み
	{
		TCHAR szStyleFileName[MAX_PATH];
		if (!CmdLineOptions.m_StyleFileName.empty()) {
			GetAbsolutePath(CmdLineOptions.m_StyleFileName.c_str(),szStyleFileName);
		} else {
			::GetModuleFileName(nullptr,szStyleFileName,lengthof(szStyleFileName));
			::PathRenameExtension(szStyleFileName,TEXT(".style.ini"));
		}
		if (::PathFileExists(szStyleFileName)) {
			AddLog(TEXT("スタイル設定を \"%s\" から読み込みます..."),szStyleFileName);
			StyleManager.Load(szStyleFileName);
		}
	}

	// 各ウィンドウの初期化
	CMainWindow::Initialize(m_hInst);
	CViewWindow::Initialize(m_hInst);
	CVideoContainerWindow::Initialize(m_hInst);
	CStatusView::Initialize(m_hInst);
	CSideBar::Initialize(m_hInst);
	Layout::CLayoutBase::Initialize(m_hInst);
	CTitleBar::Initialize(m_hInst);
	CPanelFrame::Initialize(m_hInst);
	CPanelForm::Initialize(m_hInst);
	CInformationPanel::Initialize(m_hInst);
	CProgramListPanel::Initialize(m_hInst);
	CChannelPanel::Initialize(m_hInst);
	CControlPanel::Initialize(m_hInst);
	CCaptionPanel::Initialize(m_hInst);
	CProgramGuide::Initialize(m_hInst);
	CProgramGuideFrame::Initialize(m_hInst);
	CProgramGuideDisplay::Initialize(m_hInst);
	CCaptureWindow::Initialize(m_hInst);
	CPseudoOSD::Initialize(m_hInst);
	CNotificationBar::Initialize(m_hInst);
	CEventInfoPopup::Initialize(m_hInst);
	CDropDownMenu::Initialize(m_hInst);
	CHomeDisplay::Initialize(m_hInst);
	CChannelDisplay::Initialize(m_hInst);

	// ウィンドウ位置とサイズの設定
	if (CmdLineOptions.m_fMaximize)
		MainWindow.SetMaximize(true);
	{
		int Left,Top,Width,Height;
		MainWindow.GetPosition(&Left,&Top,&Width,&Height);
		if (CmdLineOptions.m_WindowLeft!=CCommandLineOptions::INVALID_WINDOW_POS)
			Left=CmdLineOptions.m_WindowLeft;
		if (CmdLineOptions.m_WindowTop!=CCommandLineOptions::INVALID_WINDOW_POS)
			Top=CmdLineOptions.m_WindowTop;
		if (CmdLineOptions.m_WindowWidth>0)
			Width=CmdLineOptions.m_WindowWidth;
		if (CmdLineOptions.m_WindowHeight>0)
			Height=CmdLineOptions.m_WindowHeight;
		MainWindow.SetPosition(Left,Top,Width,Height);
	}

	ColorSchemeOptions.SetEventHandler(&UICore);
	ColorSchemeOptions.ApplyColorScheme();

	// ウィンドウの作成
	if (!MainWindow.Create(nullptr,WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN)) {
		AddLog(CLogItem::TYPE_ERROR,TEXT("ウィンドウが作成できません。"));
		if (!Core.IsSilent())
			MessageBox(nullptr,TEXT("ウィンドウが作成できません。"),nullptr,MB_OK | MB_ICONSTOP);
		return 0;
	}

	if (nCmdShow==SW_SHOWMINIMIZED || nCmdShow==SW_SHOWMINNOACTIVE || nCmdShow==SW_MINIMIZE)
		CmdLineOptions.m_fMinimize=true;
	if (CmdLineOptions.m_fStandby && CmdLineOptions.m_fMinimize)
		CmdLineOptions.m_fMinimize=false;
	if (!CmdLineOptions.m_fStandby && !CmdLineOptions.m_fMinimize) {
		MainWindow.Show(nCmdShow);
		MainWindow.Update();
	}
	if (!MainWindow.GetTitleBarVisible() || MainWindow.GetCustomTitleBar()) {
		// WS_CAPTION を外す場合、この段階でスタイルを変えないとおかしくなる
		// (最初からこのスタイルにしてもキャプションが表示される
		//  ShowWindow の前に入れると、キャプションを表示させた時にアイコンが出ない)
		MainWindow.SetWindowStyle(MainWindow.GetWindowStyle()^WS_CAPTION,true);
		MainWindow.Update();
	}

	GeneralOptions.Apply(COptions::UPDATE_ALL);

	TaskTrayManager.Initialize(MainWindow.GetHandle(),WM_APP_TRAYICON);
	TaskTrayManager.SetMinimizeToTray(CmdLineOptions.m_fTray || ViewOptions.GetMinimizeToTray());
	if (CmdLineOptions.m_fMinimize)
		MainWindow.InitMinimize();

	ViewOptions.Apply(COptions::UPDATE_ALL);
	VideoOptions.Apply(COptions::UPDATE_ALL);

	if (CmdLineOptions.m_f1Seg)
		Core.Set1SegMode(true,false);
	if (CmdLineOptions.m_fNoTSProcessor)
		CoreEngine.EnableTSProcessor(false);
	CoreEngine.SetMinTimerResolution(PlaybackOptions.GetMinTimerResolution());
	CoreEngine.SetNoEpg(CmdLineOptions.m_fNoEpg);
	PlaybackOptions.Apply(COptions::UPDATE_ALL);
	CoreEngine.m_DtvEngine.m_LogoDownloader.SetLogoHandler(&LogoManager);
	CoreEngine.m_DtvEngine.SetTracer(&UICore);
	UICore.SetStatusBarTrace(true);
	RecordOptions.Apply(COptions::UPDATE_ALL);

	// プラグインの読み込み
	if (!CmdLineOptions.m_fNoPlugin) {
		TCHAR szPluginDir[MAX_PATH];
		std::vector<LPCTSTR> ExcludePlugins;

		CPlugin::SetMessageWindow(MainWindow.GetHandle(),WM_APP_PLUGINMESSAGE);
		StatusView.SetSingleText(TEXT("プラグインを読み込んでいます..."));
		if (!CmdLineOptions.m_PluginsDirectory.empty()) {
			GetAbsolutePath(CmdLineOptions.m_PluginsDirectory.c_str(),szPluginDir);
		} else {
			GetAppDirectory(szPluginDir);
			::PathAppend(szPluginDir,TEXT("Plugins"));
		}
		AddLog(TEXT("プラグインを \"%s\" から読み込みます..."),szPluginDir);
		if (CmdLineOptions.m_NoLoadPlugins.size()>0) {
			for (size_t i=0;i<CmdLineOptions.m_NoLoadPlugins.size();i++)
				ExcludePlugins.push_back(CmdLineOptions.m_NoLoadPlugins[i].c_str());
		}
		PluginManager.LoadPlugins(szPluginDir,&ExcludePlugins);
	}

	RegisterCommands();
	CommandList.AddCommandCustomizer(&ZoomOptions);
	CommandList.AddCommandCustomizer(&PanAndScanOptions);

	if (!CmdLineOptions.m_fNoPlugin)
		PluginOptions.RestorePluginOptions();

	PluginManager.RegisterStatusItems();
	StatusOptions.ApplyItemList();
	PluginManager.SendStatusItemCreatedEvent();
	MainWindow.OnStatusBarInitialized();

	SideBarOptions.ApplyItemList();
	if (MainWindow.GetSideBarVisible()) {
		MainWindow.GetLayoutBase().SetContainerVisible(CONTAINER_ID_SIDEBAR,true);
		SideBar.Update();
	}

	CoreEngine.BuildDtvEngine(&m_DtvEngineHandler);
	TSProcessorManager.OpenDefaultFilters();

	// BonDriver の読み込み
	CoreEngine.SetDriverFileName(szDriverFileName);
	if (!CmdLineOptions.m_fNoDriver && !CmdLineOptions.m_fStandby) {
		if (CoreEngine.IsDriverSpecified()) {
			StatusView.SetSingleText(TEXT("BonDriverの読み込み中..."));
			if (Core.OpenAndInitializeTuner(
					Core.IsSilent()?
						CAppCore::OPENTUNER_NO_UI:
						CAppCore::OPENTUNER_RETRY_DIALOG)) {
				AppEventManager.OnTunerChanged();
			} else {
				Core.OnError(&CoreEngine,TEXT("BonDriverの初期化ができません。"));
			}
		} else {
			AddLog(TEXT("デフォルトのBonDriverはありません。"));
		}
	}

	// 再生の初期化
	CoreEngine.m_DtvEngine.m_MediaViewer.SetUseAudioRendererClock(PlaybackOptions.GetUseAudioRendererClock());
	CoreEngine.SetSpdifOptions(AudioOptions.GetSpdifOptions());
	if (CmdLineOptions.m_Volume>=0)
		CoreEngine.SetVolume(min(CmdLineOptions.m_Volume,CCoreEngine::MAX_VOLUME));
	if (PlaybackOptions.IsMuteOnStartUp() || CmdLineOptions.m_fMute)
		UICore.SetMute(true);

	// 一つのコーデックのみ指定されている場合、ver.0.8.xまでとの互換動作として先にフィルタグラフを構築
	if (!CmdLineOptions.m_fStandby && !CmdLineOptions.m_fNoDirectShow) {
		unsigned int StreamTypeFlags=0;
		if (CmdLineOptions.m_fMpeg2)
			StreamTypeFlags|=0x01;
		if (CmdLineOptions.m_fH264)
			StreamTypeFlags|=0x02;
		if (CmdLineOptions.m_fH265)
			StreamTypeFlags|=0x04;
		BYTE VideoStreamType=0;
		switch (StreamTypeFlags) {
		case 0x01:	VideoStreamType=STREAM_TYPE_MPEG2_VIDEO;	break;
		case 0x02:	VideoStreamType=STREAM_TYPE_H264;			break;
		case 0x04:	VideoStreamType=STREAM_TYPE_H265;			break;
		}
		if (VideoStreamType!=0)
			UICore.InitializeViewer(VideoStreamType);
	}

	const bool fEnablePlayback=
		!PlaybackOptions.GetRestorePlayStatus() || m_fEnablePlaybackOnStart;
	if (fEnablePlayback
			&& CoreEngine.m_DtvEngine.m_MediaViewer.IsOpen()
			&& !CmdLineOptions.m_fNoView
			&& !CmdLineOptions.m_fMinimize) {
		UICore.EnableViewer(true);
	} else {
		MainWindow.EnablePlayback(
			fEnablePlayback
			&& !CmdLineOptions.m_fNoView
			&& !CmdLineOptions.m_fNoDirectShow
			&& !CmdLineOptions.m_fMinimize);
	}

	if (CoreEngine.IsNetworkDriver()) {
		if (m_fIncrementNetworkPort) {
			CPortQuery PortQuery;
			WORD UDPPort=CmdLineOptions.m_UDPPort>0?(WORD)CmdLineOptions.m_UDPPort:
											CoreEngine.IsUDPDriver()?1234:2230;

			StatusView.SetSingleText(TEXT("空きポートを検索しています..."));
			PortQuery.Query(MainWindow.GetHandle(),&UDPPort,CoreEngine.IsUDPDriver()?1243:2239);
			CmdLineOptions.m_UDPPort=UDPPort;
		}
	}

	StatusView.SetSingleText(TEXT("チャンネル設定を読み込んでいます..."));
	Core.InitializeChannel();

	StatusView.SetSingleText(TEXT("ロゴを読み込んでいます..."));
	EpgOptions.LoadLogoFile();

	{
		TCHAR szDRCSMapName[MAX_PATH];

		GetAppDirectory(szDRCSMapName);
		::PathAppend(szDRCSMapName,TEXT("DRCSMap.ini"));
		if (::PathFileExists(szDRCSMapName)) {
			StatusView.SetSingleText(TEXT("DRCSマップを読み込んでいます..."));
			Panel.CaptionPanel.LoadDRCSMap(szDRCSMapName);
		}
	}

	{
		TCHAR szSearchFileName[MAX_PATH];

		::GetModuleFileName(nullptr,szSearchFileName,lengthof(szSearchFileName));
		::PathRenameExtension(szSearchFileName,TEXT(".search.ini"));
		if (::PathFileExists(szSearchFileName)) {
			StatusView.SetSingleText(TEXT("キーワード検索設定を読み込んでいます..."));
			KeywordSearch.Load(szSearchFileName);
		}
	}

	MainWindow.OnStatusBarTraceEnd();
	UICore.SetStatusBarTrace(false);

	MainWindow.CreatePanel();
	if (Panel.fShowPanelWindow
			&& (!Panel.IsFloating()
				|| (!CmdLineOptions.m_fStandby && !CmdLineOptions.m_fMinimize))) {
		Panel.Frame.SetPanelVisible(true,true);
		Panel.Frame.Update();
	}

	if (!CmdLineOptions.m_fNoEpg) {
		EpgOptions.AsyncLoadEpgFile(&EpgProgramList,&EpgLoadEventHandler);
		EpgOptions.AsyncLoadEDCBData(&EpgLoadEventHandler);
	}

	ApplyEventInfoFont();

	if (CmdLineOptions.m_fFullscreen)
		UICore.SetFullscreen(true);

	// 初期チャンネルを設定する
	if (CoreEngine.m_DtvEngine.IsSrcFilterOpen()) {
		if (CoreEngine.IsNetworkDriver()) {
			const int FirstPort=CoreEngine.IsUDPDriver()?1234:2230;
			int Port=FirstPort;
			if ((int)CmdLineOptions.m_UDPPort>=FirstPort && (int)CmdLineOptions.m_UDPPort<FirstPort+10)
				Port=CmdLineOptions.m_UDPPort;
			else if (RestoreChannelInfo.Channel>=0 && RestoreChannelInfo.Channel<10)
				Port=FirstPort+RestoreChannelInfo.Channel;
			Core.SetChannel(0,Port-FirstPort,CmdLineOptions.m_ServiceID);
			if (CmdLineOptions.m_ControllerChannel>0)
				Core.SetCommandLineChannel(&CmdLineOptions);
		} else if (m_fFirstExecute
				&& ChannelManager.GetFileAllChannelList()->NumChannels()==0) {
			if (MainWindow.ShowMessage(
					TEXT("最初にチャンネルスキャンを行うことをおすすめします。\r\n")
					TEXT("今すぐチャンネルスキャンを行いますか?"),
					TEXT("チャンネルスキャンの確認"),
					MB_YESNO | MB_ICONQUESTION)==IDYES) {
				ShowOptionDialog(MainWindow.GetHandle(),
								 COptionDialog::PAGE_CHANNELSCAN);
			}
		} else if (CmdLineOptions.IsChannelSpecified()) {
			Core.SetCommandLineChannel(&CmdLineOptions);
		} else if (RestoreChannelInfo.Space>=0
				&& RestoreChannelInfo.Channel>=0) {
			Core.RestoreChannel();
		} else {
			const int CurSpace=(int)CoreEngine.m_DtvEngine.m_BonSrcDecoder.GetCurSpace();
			const int CurChannel=(int)CoreEngine.m_DtvEngine.m_BonSrcDecoder.GetCurChannel();
			if (CurSpace>=0 && CurChannel>=0) {
				const CChannelList *pList=ChannelManager.GetCurrentChannelList();
				int i=pList->FindByIndex(CurSpace,CurChannel);
				if (i>=0)
					UICore.DoCommand(CM_CHANNEL_FIRST+i);
			}
		}
	}

	if (CmdLineOptions.m_fStandby)
		MainWindow.InitStandby();

	if (CmdLineOptions.m_fRecord)
		Core.CommandLineRecord(&CmdLineOptions);
	if (CmdLineOptions.m_fExitOnRecordEnd)
		Core.SetExitOnRecordingStop(true);

	if (Panel.fShowPanelWindow && Panel.Form.GetCurPageID()==PANEL_ID_CHANNEL)
		Panel.ChannelPanel.SetChannelList(ChannelManager.GetCurrentChannelList(),false);

	Accelerator.Initialize(UICore.GetMainWindow(),&MainMenu,
						   m_Settings,&CommandList);
	OperationOptions.Initialize(m_Settings,&CommandList);

	m_Settings.Close();

	FavoritesManager.Load(m_szFavoritesFileName);

	// EPG番組表の表示
	if (CmdLineOptions.m_fShowProgramGuide)
		ShowProgramGuideByCommandLine(CmdLineOptions);

	if (CmdLineOptions.m_fHomeDisplay) {
		UICore.DoCommandAsync(CM_HOMEDISPLAY);
	} else if (CmdLineOptions.m_fChannelDisplay) {
		UICore.DoCommandAsync(CM_CHANNELDISPLAY);
	}

	::SetFocus(MainWindow.GetHandle());

	{
		HWND hwndForeground=::GetForegroundWindow();
		if (hwndForeground!=nullptr) {
			DWORD ProcessID=0;
			::GetWindowThreadProcessId(hwndForeground,&ProcessID);
			if (ProcessID==::GetCurrentProcessId())
				BroadcastControllerFocusMessage(nullptr,false,true);
		}
	}

	AppEventManager.OnStartupDone();

	// メッセージループ
	MSG msg;

	UICore.RegisterModelessDialog(&StreamInfo);

	while (::GetMessage(&msg,nullptr,0,0)>0) {
		if (HtmlHelpClass.PreTranslateMessage(&msg)
				|| UICore.ProcessDialogMessage(&msg))
			continue;
		if ((IsNoAcceleratorMessage(&msg)
				|| !Accelerator.TranslateMessage(MainWindow.GetHandle(),&msg))
				&& !ControllerManager.TranslateMessage(MainWindow.GetHandle(),&msg)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}


// アクセラレータにしないメッセージの判定
bool CAppMain::IsNoAcceleratorMessage(const MSG *pmsg)
{
	HWND hwnd=::GetFocus();

	if (hwnd!=nullptr && ::IsWindowVisible(hwnd)) {
		if (MainWindow.IsNoAcceleratorMessage(pmsg))
			return true;

		const CDisplayView *pDisplayView=MainWindow.GetDisplayBase().GetDisplayView();
		if (pDisplayView!=nullptr && hwnd==pDisplayView->GetHandle()) {
			return pDisplayView->IsMessageNeed(pmsg);
		} else if (pmsg->message==WM_KEYDOWN || pmsg->message==WM_KEYUP) {
			TCHAR szClass[64];

			if (::GetClassName(hwnd,szClass,lengthof(szClass))>0) {
				bool fNeedCursor=false;

				if (::lstrcmpi(szClass,TEXT("EDIT"))==0
						|| ::StrCmpNI(szClass,TEXT("RICHEDIT"),8)==0) {
					if ((GetWindowStyle(hwnd)&ES_READONLY)==0)
						return true;
					fNeedCursor=true;
				} else if (::lstrcmpi(szClass,TEXT("COMBOBOX"))==0
						|| ::lstrcmpi(szClass,TEXT("LISTBOX"))==0
						|| ::StrCmpNI(szClass,TEXT("SysListView"),11)==0
						|| ::StrCmpNI(szClass,TEXT("SysTreeView"),11)==0) {
					fNeedCursor=true;
				}
				if (fNeedCursor) {
					switch (pmsg->wParam) {
					case VK_LEFT:
					case VK_RIGHT:
					case VK_UP:
					case VK_DOWN:
						return true;
					}
				}
			}
		}
	}
	return false;
}


// 設定ダイアログを表示する
bool CAppMain::ShowOptionDialog(HWND hwndOwner,int StartPage)
{
	if (!m_OptionDialog.Show(hwndOwner,StartPage))
		return false;

	if ((COptions::GetGeneralUpdateFlags() & COptions::UPDATE_GENERAL_BUILDMEDIAVIEWER)!=0) {
		if (CoreEngine.m_DtvEngine.m_MediaViewer.IsOpen()
				|| UICore.IsViewerInitializeError()) {
			bool fOldError=UICore.IsViewerInitializeError();
			bool fResult=UICore.InitializeViewer();
			// エラーで再生オフになっていた場合はオンにする
			if (fResult && fOldError && !UICore.IsViewerEnabled())
				UICore.EnableViewer(true);
		}
	}

	if ((COptions::GetGeneralUpdateFlags() & COptions::UPDATE_GENERAL_EVENTINFOFONT)!=0) {
		ApplyEventInfoFont();
	}

	if ((ProgramGuideOptions.GetUpdateFlags() & CProgramGuideOptions::UPDATE_EVENTICONS)!=0)
		Panel.ProgramListPanel.SetVisibleEventIcons(ProgramGuideOptions.GetVisibleEventIcons());
	TaskTrayManager.SetMinimizeToTray(ViewOptions.GetMinimizeToTray());

	SaveSettings(SETTINGS_SAVE_OPTIONS);

	AppEventManager.OnSettingsChanged();

	return true;
}


CAppMain::CreateDirectoryResult CAppMain::CreateDirectory(
	HWND hwnd,LPCTSTR pszDirectory,LPCTSTR pszMessage)
{
	if (IsStringEmpty(pszDirectory))
		return CREATEDIRECTORY_RESULT_NOPATH;

	TCHAR szPath[MAX_PATH];
	TCHAR szMessage[MAX_PATH+80];

	if (!GetAbsolutePath(pszDirectory,szPath)) {
		StdUtil::snprintf(szMessage,lengthof(szMessage),
						  TEXT("フォルダ \"%s\" のパスが長過ぎます。"),szPath);
		::MessageBox(hwnd,szMessage,nullptr,MB_OK | MB_ICONEXCLAMATION);
		return CREATEDIRECTORY_RESULT_ERROR;
	}

	if (!::PathIsDirectory(szPath)) {
		StdUtil::snprintf(szMessage,lengthof(szMessage),pszMessage,szPath);
		if (::MessageBox(hwnd,szMessage,TEXT("フォルダ作成の確認"),
						 MB_YESNO | MB_ICONQUESTION)!=IDYES)
			return CREATEDIRECTORY_RESULT_CANCELLED;

		int Result=::SHCreateDirectoryEx(hwnd,szPath,nullptr);
		if (Result!=ERROR_SUCCESS && Result!=ERROR_ALREADY_EXISTS) {
			StdUtil::snprintf(szMessage,lengthof(szMessage),
							  TEXT("フォルダ \"%s\" を作成できません。(エラーコード %#x)"),szPath,Result);
			AddLog(CLogItem::TYPE_ERROR,szMessage);
			::MessageBox(hwnd,szMessage,nullptr,MB_OK | MB_ICONEXCLAMATION);
			return CREATEDIRECTORY_RESULT_ERROR;
		}

		AddLog(CLogItem::TYPE_INFORMATION,TEXT("フォルダ \"%s\" を作成しました。"),szPath);
	}

	return CREATEDIRECTORY_RESULT_SUCCESS;
}


bool CAppMain::SendInterprocessMessage(HWND hwnd,UINT Message,const void *pParam,DWORD ParamSize)
{
	COPYDATASTRUCT cds;
	cds.dwData=Message;
	cds.cbData=ParamSize;
	cds.lpData=const_cast<void*>(pParam);

	DWORD_PTR Result=0;
	return ::SendMessageTimeout(hwnd,WM_COPYDATA,0,reinterpret_cast<LPARAM>(&cds),
								SMTO_NORMAL | SMTO_NOTIMEOUTIFNOTHUNG,10000,&Result)!=0
		&& Result!=0;
}


LRESULT CAppMain::ReceiveInterprocessMessage(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
	::ReplyMessage(TRUE);

	const COPYDATASTRUCT *pcds=reinterpret_cast<const COPYDATASTRUCT*>(lParam);

	switch (pcds->dwData) {
	case PROCESS_MESSAGE_EXECUTE:
		if (pcds->cbData>=sizeof(TCHAR) && pcds->lpData!=nullptr) {
			LPCTSTR pszCommandLine=static_cast<LPCTSTR>(pcds->lpData);
			if (pszCommandLine[pcds->cbData/sizeof(TCHAR)-1]==_T('\0'))
				ProcessCommandLine(pszCommandLine);
		}
		break;

	default:
		AddLog(CLogItem::TYPE_WARNING,TEXT("未知のメッセージ %Ix を受信しました。"),pcds->dwData);
		break;
	}

	return TRUE;
}


bool CAppMain::BroadcastControllerFocusMessage(
	HWND hwnd,bool fSkipSelf,bool fFocus,DWORD ActiveThreadID)
{
	ControllerFocusInfo Info;

	Info.hwnd=hwnd;
	Info.fSkipSelf=fSkipSelf;
	Info.fFocus=fFocus;
	return EnumTVTestWindows(ControllerFocusCallback,
							 reinterpret_cast<LPARAM>(&Info));
}


BOOL CALLBACK CAppMain::ControllerFocusCallback(HWND hwnd,LPARAM Param)
{
	ControllerFocusInfo *pInfo=reinterpret_cast<ControllerFocusInfo*>(Param);

	if (!pInfo->fSkipSelf || hwnd!=pInfo->hwnd) {
		::PostMessage(hwnd,WM_APP_CONTROLLERFOCUS,pInfo->fFocus,0);
		pInfo->fFocus=false;
	}
	return TRUE;
}


void CAppMain::RegisterCommands()
{
	// BonDriver
	for (int i=0;i<DriverManager.NumDrivers();i++) {
		const CDriverInfo *pDriverInfo=DriverManager.GetDriverInfo(i);
		LPCTSTR pszFileName=::PathFindFileName(pDriverInfo->GetFileName());
		TCHAR szName[CCommandList::MAX_COMMAND_NAME];

		StdUtil::snprintf(szName,lengthof(szName),TEXT("BonDriver切替 : %s"),pszFileName);
		CommandList.RegisterCommand(CM_DRIVER_FIRST+i,pszFileName,szName);
	}

	// プラグイン
	for (int i=0;i<PluginManager.NumPlugins();i++) {
		const CPlugin *pPlugin=PluginManager.GetPlugin(i);
		LPCTSTR pszFileName=::PathFindFileName(pPlugin->GetFileName());
		TCHAR szName[CCommandList::MAX_COMMAND_NAME];
		TCHAR szShortName[CCommandList::MAX_COMMAND_NAME];

		StdUtil::snprintf(szName,lengthof(szName),TEXT("プラグイン有効/無効 : %s"),pszFileName);
		StdUtil::snprintf(szShortName,lengthof(szShortName),
						  TEXT("%s 有効/無効"),pPlugin->GetPluginName());
		CommandList.RegisterCommand(CM_PLUGIN_FIRST+i,pszFileName,szName,szShortName);
	}

	// プラグインの各コマンド
	int ID=CM_PLUGINCOMMAND_FIRST;
	for (int i=0;i<PluginManager.NumPlugins();i++) {
		CPlugin *pPlugin=PluginManager.GetPlugin(i);

		if (pPlugin->GetIcon().IsCreated())
			SideBarOptions.RegisterCommand(pPlugin->GetCommand());

		LPCTSTR pszFileName=::PathFindFileName(pPlugin->GetFileName());

		for (int j=0;j<pPlugin->NumPluginCommands();j++) {
			CPlugin::CPluginCommandInfo *pInfo=pPlugin->GetPluginCommandInfo(j);

			pInfo->SetCommand(ID);

			TVTest::String Text;
			Text=pszFileName;
			Text+=_T(':');
			Text+=pInfo->GetText();

			TCHAR szName[CCommandList::MAX_COMMAND_NAME];
			StdUtil::snprintf(szName,lengthof(szName),TEXT("%s : %s"),pszFileName,pInfo->GetName());

			unsigned int State=0;
			if ((pInfo->GetState() & TVTest::PLUGIN_COMMAND_STATE_DISABLED)!=0)
				State|=CCommandList::COMMAND_STATE_DISABLED;
			if ((pInfo->GetState() & TVTest::PLUGIN_COMMAND_STATE_CHECKED)!=0)
				State|=CCommandList::COMMAND_STATE_CHECKED;

			CommandList.RegisterCommand(ID,Text.c_str(),szName,pInfo->GetName(),State);

			if ((pInfo->GetFlags() & TVTest::PLUGIN_COMMAND_FLAG_ICONIZE)!=0)
				SideBarOptions.RegisterCommand(ID);

			ID++;
		}
	}
}


void CAppMain::ApplyEventInfoFont()
{
	Panel.ProgramListPanel.SetEventInfoFont(EpgOptions.GetEventInfoFont());
	Panel.ChannelPanel.SetEventInfoFont(EpgOptions.GetEventInfoFont());
	Epg.ProgramGuide.SetEventInfoFont(EpgOptions.GetEventInfoFont());
	CProgramInfoStatusItem *pProgramInfo=dynamic_cast<CProgramInfoStatusItem*>(StatusView.GetItemByID(STATUS_ITEM_PROGRAMINFO));
	if (pProgramInfo!=nullptr)
		pProgramInfo->SetEventInfoFont(EpgOptions.GetEventInfoFont());
}


bool CAppMain::GetAbsolutePath(LPCTSTR pszPath,LPTSTR pszAbsolutePath) const
{
	if (pszAbsolutePath==nullptr)
		return false;

	pszAbsolutePath[0]=_T('\0');

	if (IsStringEmpty(pszPath))
		return false;

	if (::PathIsRelative(pszPath)) {
		TCHAR szDir[MAX_PATH],szTemp[MAX_PATH];
		GetAppDirectory(szDir);
		if (::PathCombine(szTemp,szDir,pszPath)==nullptr
				|| !::PathCanonicalize(pszAbsolutePath,szTemp))
			return false;
	} else {
		if (::lstrlen(pszPath)>=MAX_PATH)
			return false;
		::lstrcpy(pszAbsolutePath,pszPath);
	}

	return true;
}


bool CAppMain::ProcessCommandLine(LPCTSTR pszCmdLine)
{
	AddLog(TEXT("新しいコマンドラインオプションを受信しました。"));
	AddLog(TEXT("コマンドラインオプション : %s"),pszCmdLine);

	CCommandLineOptions CmdLine;

	AppEventManager.OnExecute(pszCmdLine);

	CmdLine.Parse(pszCmdLine);

	if (!CmdLine.m_fMinimize
			&& !CmdLine.m_fStandby
			&& !CmdLine.m_fNoView
			&& !CmdLine.m_fNoDirectShow
			&& !CmdLine.m_fTray
			&& !CmdLine.m_fProgramGuideOnly)
		UICore.DoCommand(CM_SHOW);

	if (MainWindow.GetVisible()) {
		if (CmdLine.m_fFullscreen)
			UICore.SetFullscreen(true);
		else if (CmdLine.m_fMaximize)
			MainWindow.SetMaximize(true);
		else if (CmdLine.m_fMinimize)
			::ShowWindow(MainWindow.GetHandle(),SW_MINIMIZE);
	}

	if (CmdLine.m_fSilent || CmdLine.m_TvRockDID>=0)
		Core.SetSilent(true);
	if (CmdLine.m_fSaveLog)
		CmdLineOptions.m_fSaveLog=true;

	if (!CmdLine.m_DriverName.empty()) {
		if (Core.OpenTuner(CmdLine.m_DriverName.c_str())) {
			if (CmdLine.IsChannelSpecified())
				Core.SetCommandLineChannel(&CmdLine);
			else
				Core.RestoreChannel();
		}
	} else {
		if (CmdLine.IsChannelSpecified())
			Core.SetCommandLineChannel(&CmdLine);
	}

	if (CmdLine.m_fRecord) {
		if (CmdLine.m_fRecordCurServiceOnly)
			CmdLineOptions.m_fRecordCurServiceOnly=true;
		Core.CommandLineRecord(&CmdLine);
	} else if (CmdLine.m_fRecordStop) {
		Core.StopRecord();
	}

	if (CmdLine.m_Volume>=0)
		UICore.SetVolume(min(CmdLine.m_Volume,CCoreEngine::MAX_VOLUME),false);
	if (CmdLine.m_fMute)
		UICore.SetMute(true);

	if (CmdLine.m_fShowProgramGuide || CmdLine.m_fProgramGuideOnly)
		ShowProgramGuideByCommandLine(CmdLine);
	if (CmdLine.m_fHomeDisplay)
		UICore.DoCommandAsync(CM_HOMEDISPLAY);
	else if (CmdLine.m_fChannelDisplay)
		UICore.DoCommandAsync(CM_CHANNELDISPLAY);

	if (!CmdLine.m_Command.empty()) {
		int Command=CommandList.ParseText(CmdLine.m_Command.c_str());
		if (Command!=0) {
			UICore.DoCommand(Command);
		} else {
			AddLog(CLogItem::TYPE_ERROR,
				   TEXT("指定されたコマンド \"%s\" は無効です。"),
				   CmdLine.m_Command.c_str());
		}
	}

	return true;
}


void CAppMain::ShowProgramGuideByCommandLine(const CCommandLineOptions &CmdLine)
{
	CMainWindow::ProgramGuideSpaceInfo SpaceInfo;

	if (!CmdLine.m_ProgramGuideTuner.empty())
		SpaceInfo.pszTuner=CmdLine.m_ProgramGuideTuner.c_str();
	else
		SpaceInfo.pszTuner=nullptr;
	if (!CmdLine.m_ProgramGuideSpace.empty())
		SpaceInfo.pszSpace=CmdLine.m_ProgramGuideSpace.c_str();
	else
		SpaceInfo.pszSpace=nullptr;

	MainWindow.ShowProgramGuide(true,
		UICore.GetFullscreen()?0:CMainWindow::PROGRAMGUIDE_SHOW_POPUP,
		&SpaceInfo);
}


HICON CAppMain::GetAppIcon()
{
	if (m_hicoApp==nullptr) {
		m_hicoApp=LoadIconStandardSize(
			::GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_ICON),ICON_SIZE_NORMAL);
	}
	return m_hicoApp;
}


HICON CAppMain::GetAppIconSmall()
{
	if (m_hicoAppSmall==nullptr) {
		m_hicoAppSmall=LoadIconStandardSize(
			::GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_ICON),ICON_SIZE_SMALL);
	}
	return m_hicoAppSmall;
}


CAppMain::CDtvEngineEventHandler::CDtvEngineEventHandler(CAppMain &App)
	: m_App(App)
{
}

void CAppMain::CDtvEngineEventHandler::OnServiceUpdated(
	CTsAnalyzer *pTsAnalyzer,bool fListUpdated,bool fStreamChanged)
{
	CServiceUpdateInfo *pInfo=new CServiceUpdateInfo(m_pDtvEngine,pTsAnalyzer);

	pInfo->m_fStreamChanged=fStreamChanged;
	m_App.MainWindow.PostMessage(WM_APP_SERVICEUPDATE,fListUpdated,
								 reinterpret_cast<LPARAM>(pInfo));
}

void CAppMain::CDtvEngineEventHandler::OnServiceListUpdated(
	CTsAnalyzer *pTsAnalyzer,bool bStreamChanged)
{
	OnServiceUpdated(pTsAnalyzer,true,bStreamChanged);
	if (m_App.AudioManager.OnServiceUpdated())
		m_App.MainWindow.PostMessage(WM_APP_AUDIOLISTCHANGED,0,0);
}

void CAppMain::CDtvEngineEventHandler::OnServiceInfoUpdated(CTsAnalyzer *pTsAnalyzer)
{
	OnServiceUpdated(pTsAnalyzer,false,false);
	m_App.MainWindow.PostMessage(WM_APP_SERVICEINFOUPDATED,0,
								 MAKELPARAM(pTsAnalyzer->GetNetworkID(),
											pTsAnalyzer->GetTransportStreamID()));
}

void CAppMain::CDtvEngineEventHandler::OnServiceChanged(WORD ServiceID)
{
	m_App.MainWindow.PostMessage(WM_APP_SERVICECHANGED,ServiceID,0);
	if (m_App.AudioManager.OnServiceUpdated())
		m_App.MainWindow.PostMessage(WM_APP_AUDIOLISTCHANGED,0,0);
	if (m_App.AudioOptions.GetResetAudioDelayOnChannelChange())
		m_App.CoreEngine.m_DtvEngine.m_MediaViewer.SetAudioDelay(0);
}

void CAppMain::CDtvEngineEventHandler::OnFileWriteError(CTsRecorder *pTsRecorder)
{
	m_App.MainWindow.PostMessage(WM_APP_FILEWRITEERROR,0,0);
}

void CAppMain::CDtvEngineEventHandler::OnVideoStreamTypeChanged(BYTE VideoStreamType)
{
	m_App.MainWindow.PostMessage(WM_APP_VIDEOSTREAMTYPECHANGED,VideoStreamType,0);
}

void CAppMain::CDtvEngineEventHandler::OnVideoSizeChanged(CMediaViewer *pMediaViewer)
{
	/*
		この通知が送られた段階ではまだレンダラの映像サイズは変わっていないため、
		後でパンスキャンの設定を行う必要がある
	*/
	m_App.MainWindow.PostMessage(WM_APP_VIDEOSIZECHANGED,0,0);
}

void CAppMain::CDtvEngineEventHandler::OnEventChanged(CTsAnalyzer *pTsAnalyzer,WORD EventID)
{
	TRACE(TEXT("CDtvEngineEventHandler::OnEventChanged() : event_id %#04x\n"),EventID);
	if (EventID!=CTsAnalyzer::EVENTID_INVALID) {
		m_App.CoreEngine.SetAsyncStatusUpdatedFlag(CCoreEngine::STATUS_EVENTID);
		if (m_App.AudioManager.OnEventUpdated())
			m_App.MainWindow.PostMessage(WM_APP_AUDIOLISTCHANGED,0,0);
	}
}

void CAppMain::CDtvEngineEventHandler::OnEventUpdated(CTsAnalyzer *pTsAnalyzer)
{
	m_App.CoreEngine.SetAsyncStatusUpdatedFlag(CCoreEngine::STATUS_EVENTINFO);
	if (m_App.AudioManager.OnEventUpdated())
		m_App.MainWindow.PostMessage(WM_APP_AUDIOLISTCHANGED,0,0);
}

void CAppMain::CDtvEngineEventHandler::OnTotUpdated(CTsAnalyzer *pTsAnalyzer)
{
	m_App.CoreEngine.SetAsyncStatusUpdatedFlag(CCoreEngine::STATUS_TOT);
}

void CAppMain::CDtvEngineEventHandler::OnFilterGraphInitialize(
	CMediaViewer *pMediaViewer,IGraphBuilder *pGraphBuilder)
{
	m_App.PluginManager.SendFilterGraphInitializeEvent(pMediaViewer,pGraphBuilder);
}

void CAppMain::CDtvEngineEventHandler::OnFilterGraphInitialized(
	CMediaViewer *pMediaViewer,IGraphBuilder *pGraphBuilder)
{
	m_App.PluginManager.SendFilterGraphInitializedEvent(pMediaViewer,pGraphBuilder);
}

void CAppMain::CDtvEngineEventHandler::OnFilterGraphFinalize(
	CMediaViewer *pMediaViewer,IGraphBuilder *pGraphBuilder)
{
	m_App.PluginManager.SendFilterGraphFinalizeEvent(pMediaViewer,pGraphBuilder);
}

void CAppMain::CDtvEngineEventHandler::OnFilterGraphFinalized(
	CMediaViewer *pMediaViewer,IGraphBuilder *pGraphBuilder)
{
	m_App.PluginManager.SendFilterGraphFinalizedEvent(pMediaViewer,pGraphBuilder);
}

void CAppMain::CDtvEngineEventHandler::OnSpdifPassthroughError(HRESULT hr)
{
	m_App.MainWindow.PostMessage(WM_APP_SPDIFPASSTHROUGHERROR,hr,0);
}


CAppMain::CStreamInfoEventHandler::CStreamInfoEventHandler(CAppMain &App)
	: m_App(App)
{
}

void CAppMain::CStreamInfoEventHandler::OnRestoreSettings()
{
#if 0
	CSettings Settings;

	if (Settings.Open(m_App.Core.GetIniFileName(),CSettings::OPEN_READ)
			&& Settings.SetSection(TEXT("Settings"))) {
		int Left,Top,Width,Height;

		m_App.StreamInfo.GetPosition(&Left,&Top,&Width,&Height);
		Settings.Read(TEXT("StreamInfoLeft"),&Left);
		Settings.Read(TEXT("StreamInfoTop"),&Top);
		Settings.Read(TEXT("StreamInfoWidth"),&Width);
		Settings.Read(TEXT("StreamInfoHeight"),&Height);
		m_App.StreamInfo.SetPosition(Left,Top,Width,Height);
		//m_App.StreamInfo.MoveToMonitorInside();
	}
#endif
}

bool CAppMain::CStreamInfoEventHandler::OnClose()
{
	m_App.UICore.SetCommandCheckedState(CM_STREAMINFO,false);
	return true;
}


CAppMain::CCaptureWindowEventHandler::CCaptureWindowEventHandler(CAppMain &App)
	: m_App(App)
{
}

void CAppMain::CCaptureWindowEventHandler::OnRestoreSettings()
{
#if 0
	CSettings Settings;

	if (Settings.Open(m_App.Core.GetIniFileName(),CSettings::OPEN_READ)
			&& Settings.SetSection(TEXT("Settings"))) {
		int Left,Top,Width,Height;
		m_pCaptureWindow->GetPosition(&Left,&Top,&Width,&Height);
		Settings.Read(TEXT("CapturePreviewLeft"),&Left);
		Settings.Read(TEXT("CapturePreviewTop"),&Top);
		Settings.Read(TEXT("CapturePreviewWidth"),&Width);
		Settings.Read(TEXT("CapturePreviewHeight"),&Height);
		m_pCaptureWindow->SetPosition(Left,Top,Width,Height);
		m_pCaptureWindow->MoveToMonitorInside();

		bool fStatusBar;
		if (Settings.Read(TEXT("CaptureStatusBar"),&fStatusBar))
			m_pCaptureWindow->ShowStatusBar(fStatusBar);
	}
#endif
}

bool CAppMain::CCaptureWindowEventHandler::OnClose()
{
	m_App.UICore.SetCommandCheckedState(CM_CAPTUREPREVIEW,false);
	m_pCaptureWindow->ClearImage();
	return true;
}

bool CAppMain::CCaptureWindowEventHandler::OnSave(CCaptureImage *pImage)
{
	return m_App.CaptureOptions.SaveImage(pImage);
}

bool CAppMain::CCaptureWindowEventHandler::OnKeyDown(UINT KeyCode,UINT Flags)
{
	m_App.MainWindow.SendMessage(WM_KEYDOWN,KeyCode,Flags);
	return true;
}
