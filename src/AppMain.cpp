#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "AppUtil.h"
#include "InitialSettings.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


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
				TEXT("終了処理がタイムアウトしました(%ums)。プロセスを強制的に終了させます。"),
				pThis->m_Timeout);

			if (pThis->m_hMainThread!=nullptr) {
				auto pCancelSynchronousIo=
					GET_MODULE_FUNCTION(TEXT("kernel32.dll"),CancelSynchronousIo);
				if (pCancelSynchronousIo!=nullptr)
					pCancelSynchronousIo(pThis->m_hMainThread);
			}

			pThis->Finalize();

			::ExitProcess(-1);
		}
	}

	return 0;
}

#endif	// WATCH_EXIT




CAppMain::CAppMain()
	: m_hInst(nullptr)
	, Core(*this)
	, UICore(*this)
	, EpgProgramList(&CoreEngine.m_DtvEngine.m_EventManager)
	, MainWindow(*this)
	, SideBar(&CommandList)
	, ChannelMenu(&EpgProgramList,&LogoManager)
	, HomeDisplay(EventSearchOptions)
	, ChannelDisplay(&EpgProgramList)
#ifdef NETWORK_REMOCON_SUPPORT
	, pNetworkRemocon(nullptr)
	, NetworkRemoconGetChannel(&MainWindow)
#endif

	, Epg(EpgProgramList,EventSearchOptions)

	, OSDManager(&OSDOptions)
	, StatusOptions(&StatusView)
	, SideBarOptions(&SideBar,&ZoomOptions)
	, PanelOptions(&Panel.Frame)
	, ProgramGuideOptions(&Epg.ProgramGuide,&PluginManager)
	, PluginOptions(&PluginManager)

	, m_fFirstExecute(false)

	, m_DtvEngineHandler(*this)
	, m_StreamInfoEventHandler(*this)
	, m_CaptureWindowEventHandler(*this)

	, m_ExitTimeout(60000)
	, m_fEnablePlaybackOnStart(true)
	, m_fIncrementNetworkPort(true)
{
	UICore.SetSkin(&MainWindow);

	StreamInfo.SetEventHandler(&m_StreamInfoEventHandler);
	CaptureWindow.SetEventHandler(&m_CaptureWindowEventHandler);
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


void CAppMain::AddLog(LPCTSTR pszText, ...)
{
	va_list Args;

	va_start(Args,pszText);
	Logger.AddLogV(pszText,Args);
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
	if (CmdLineOptions.m_IniFileName.IsEmpty()) {
		::lstrcpy(m_szIniFileName,szModuleFileName);
		::PathRenameExtension(m_szIniFileName,TEXT(".ini"));
	} else {
		LPCTSTR pszIniFileName=CmdLineOptions.m_IniFileName.Get();
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
	m_fFirstExecute=!fExists && CmdLineOptions.m_IniFileName.IsEmpty();
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

#ifdef NETWORK_REMOCON_SUPPORT
	SAFE_DELETE(pNetworkRemocon);
#endif
	ResidentManager.Finalize();
	ChannelMenu.Destroy();
	FavoritesMenu.Destroy();
	MainMenu.Destroy();
	Accelerator.Finalize();
	ControllerManager.DeleteAllControllers();
	TaskbarManager.Finalize();
	Epg.ProgramGuideFrame.Destroy();
	NotifyBalloonTip.Finalize();

	Core.SaveCurrentChannel();
	Core.SaveChannelSettings();

	FINALIZE_CONTINUE

	CoreEngine.m_DtvEngine.SetTracer(&Logger);
	CoreEngine.Close();
	CoreEngine.m_DtvEngine.SetTracer(nullptr);
	CoreEngine.m_DtvEngine.m_BonSrcDecoder.SetTracer(nullptr);

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
		AddLog(TEXT("設定ファイル \"%s\" を開けません。"),m_szIniFileName);
		return false;
	}

	if (!CmdLineOptions.m_IniValueList.empty()) {
		for (size_t i=0;i<CmdLineOptions.m_IniValueList.size();i++) {
			const CCommandLineOptions::IniEntry &Entry=CmdLineOptions.m_IniValueList[i];

			TRACE(TEXT("Override INI entry : [%s] %s=%s\n"),
				  Entry.Section.GetSafe(),Entry.Name.GetSafe(),Entry.Value.GetSafe());
			if (Settings.SetSection(Entry.Section.IsEmpty()?TEXT("Settings"):Entry.Section.Get())) {
				Settings.Write(Entry.Name.Get(),Entry.Value.GetSafe());
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

		Settings.Read(TEXT("ExitTimeout"),&m_ExitTimeout);
		Settings.Read(TEXT("IncrementUDPPort"),&m_fIncrementNetworkPort);

		MainWindow.ReadSettings(Settings);
		GeneralOptions.ReadSettings(Settings);
		ViewOptions.ReadSettings(Settings);
		OSDOptions.ReadSettings(Settings);
		PanelOptions.ReadSettings(Settings);
		PlaybackOptions.ReadSettings(Settings);
		RecordOptions.ReadSettings(Settings);
		CaptureOptions.ReadSettings(Settings);
		ControllerManager.ReadSettings(Settings);
		ChannelScan.ReadSettings(Settings);
		EpgOptions.ReadSettings(Settings);
#ifdef NETWORK_REMOCON_SUPPORT
		NetworkRemoconOptions.ReadSettings(Settings);
#endif
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
	RecentChannelList.LoadSettings(Settings);
	Panel.InfoPanel.LoadSettings(Settings);
	Panel.ProgramListPanel.LoadSettings(Settings);
	Panel.ChannelPanel.LoadSettings(Settings);
	PanAndScanOptions.LoadSettings(Settings);
	HomeDisplay.LoadSettings(Settings);
	NetworkDefinition.LoadSettings(Settings);
	CasLibraryManager.LoadSettings(Settings);

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
		AddLog(TEXT("%s"),szMessage);
		if (!Core.IsSilent())
			UICore.GetSkin()->ShowErrorMessage(szMessage);
		return false;
	}

	if (Settings.SetSection(TEXT("Settings"))) {
		Settings.Write(TEXT("Version"),VERSION_TEXT);

		if ((Flags&SETTINGS_SAVE_STATUS)!=0) {
			int Left,Top,Width,Height;

			//Settings.Write(TEXT("CasLibrary"),CoreEngine.GetCasLibraryName());
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
			Settings.Write(TEXT("InfoCurTab"),Panel.Form.GetCurPageID());

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
#ifdef NETWORK_REMOCON_SUPPORT
		{&NetworkRemoconOptions,			false},
#endif
		{&Logger,							false},
	//	{&ZoomOptions,						false},
	//	{&PanAndScanOptions,				false},
		{&Epg.ProgramGuideFrameSettings,	true},
		{&HomeDisplay,						true},
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
	}

	return true;
}


int CAppMain::Main(HINSTANCE hInstance,LPCTSTR pszCmdLine,int nCmdShow)
{
	m_hInst=hInstance;

	// コマンドラインの解析
	if (pszCmdLine[0]!=_T('\0')) {
		Logger.AddLog(TEXT("コマンドラインオプション : %s"),pszCmdLine);

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

		if (CmdLineOptions.m_f1Seg)
			Core.Set1SegMode(true,false);
	}

	Initialize();

	CAppMutex AppMutex(true);

	// 複数起動のチェック
	if (AppMutex.AlreadyExists()
			&& (GeneralOptions.GetKeepSingleTask() || CmdLineOptions.m_fSingleTask)) {
		Logger.AddLog(TEXT("複数起動が禁止されています。"));
		CTVTestWindowFinder Finder;
		HWND hwnd=Finder.FindCommandLineTarget();
		if (::IsWindow(hwnd)) {
			ATOM atom;

			if (pszCmdLine[0]!=_T('\0'))
				atom=::GlobalAddAtom(pszCmdLine);
			else
				atom=0;
			// ATOM だと256文字までしか渡せないので、WM_COPYDATA 辺りの方がいいかも
			/*
			DWORD_PTR Result;
			::SendMessageTimeout(hwnd,WM_APP_EXECUTE,(WPARAM)atom,0,
								 SMTO_NORMAL,10000,&Result);
			*/
			::PostMessage(hwnd,WM_APP_EXECUTE,(WPARAM)atom,0);
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
	DriverOptions.Initialize(&DriverManager);

	// 初期設定ダイアログを表示するか
	const bool fInitialSettings=
		CmdLineOptions.m_fInitialSettings
			|| (m_fFirstExecute && CmdLineOptions.m_DriverName.IsEmpty());

	// CASライブラリの読み込み
	if (!CmdLineOptions.m_CasLibraryName.IsEmpty()) {
		CasLibraryManager.SetDefaultCasLibrary(CmdLineOptions.m_CasLibraryName.Get());
	} else if (!CasLibraryManager.HasDefaultCasLibrary()) {
		TCHAR szDir[MAX_PATH];
		GetAppDirectory(szDir);
		CasLibraryManager.FindDefaultCasLibrary(szDir);
	}
	if (CasLibraryManager.HasDefaultCasLibrary()
			&& (fInitialSettings || !CasLibraryManager.HasCasLibraryNetworkMap())) {
		//StatusView.SetSingleText(TEXT("CASライブラリの読み込み中..."));
		Core.LoadCasLibrary(nullptr);
	}

	TCHAR szDriverFileName[MAX_PATH];

	// 初期設定ダイアログの表示
	if (fInitialSettings) {
		CInitialSettings InitialSettings(&DriverManager);

		if (!InitialSettings.Show(nullptr))
			return 0;
		InitialSettings.GetDriverFileName(szDriverFileName,lengthof(szDriverFileName));
		GeneralOptions.SetDefaultDriverName(szDriverFileName);
		GeneralOptions.SetMpeg2DecoderName(InitialSettings.GetMpeg2DecoderName());
		GeneralOptions.SetH264DecoderName(InitialSettings.GetH264DecoderName());
		GeneralOptions.SetH265DecoderName(InitialSettings.GetH265DecoderName());
		GeneralOptions.SetVideoRendererType(InitialSettings.GetVideoRenderer());
		GeneralOptions.SetCasDevice(InitialSettings.GetCasDevice());
		RecordOptions.SetSaveFolder(InitialSettings.GetRecordFolder());
	} else if (!CmdLineOptions.m_DriverName.IsEmpty()) {
		::lstrcpy(szDriverFileName,CmdLineOptions.m_DriverName.Get());
	} else if (CmdLineOptions.m_fNoDriver) {
		szDriverFileName[0]=_T('\0');
	} else {
		GeneralOptions.GetFirstDriverName(szDriverFileName);
	}

	GeneralOptions.SetTemporaryNoDescramble(CmdLineOptions.m_fNoDescramble);

	GraphicsCore.Initialize();

	// スタイル設定の読み込み
	{
		TCHAR szStyleFileName[MAX_PATH];
		if (!CmdLineOptions.m_StyleFileName.IsEmpty()) {
			GetAbsolutePath(CmdLineOptions.m_StyleFileName.Get(),szStyleFileName);
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

	// ウィンドウの作成
	if (!MainWindow.Create(nullptr,WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN)) {
		Logger.AddLog(TEXT("ウィンドウが作成できません。"));
		if (!Core.IsSilent())
			MessageBox(nullptr,TEXT("ウィンドウが作成できません。"),nullptr,MB_OK | MB_ICONSTOP);
		return 0;
	}

	ColorSchemeOptions.SetEventHandler(this);
	ColorSchemeOptions.ApplyColorScheme();

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

	ResidentManager.SetResident(GeneralOptions.GetResident());
	ResidentManager.Initialize(MainWindow.GetHandle(),WM_APP_TRAYICON);
	ResidentManager.SetMinimizeToTray(CmdLineOptions.m_fTray || ViewOptions.GetMinimizeToTray());
	if (CmdLineOptions.m_fMinimize)
		MainWindow.InitMinimize();

	ViewOptions.Apply(COptions::UPDATE_ALL);

	CoreEngine.SetMinTimerResolution(PlaybackOptions.GetMinTimerResolution());
	CoreEngine.SetDescramble(!CmdLineOptions.m_fNoDescramble);
	CoreEngine.SetNoEpg(CmdLineOptions.m_fNoEpg);
	CoreEngine.m_DtvEngine.SetDescrambleCurServiceOnly(GeneralOptions.GetDescrambleCurServiceOnly());
	CoreEngine.m_DtvEngine.m_CasProcessor.SetInstruction(GeneralOptions.GetDescrambleInstruction());
	CoreEngine.m_DtvEngine.m_CasProcessor.EnableContract(GeneralOptions.GetEnableEmmProcess());
	PlaybackOptions.Apply(COptions::UPDATE_ALL);
	CoreEngine.m_DtvEngine.m_LogoDownloader.SetLogoHandler(&LogoManager);
	CoreEngine.m_DtvEngine.SetTracer(&StatusView);
	CoreEngine.m_DtvEngine.m_BonSrcDecoder.SetTracer(&Logger);
	CoreEngine.BuildDtvEngine(&m_DtvEngineHandler);
	RecordOptions.Apply(COptions::UPDATE_ALL);

	// BonDriver の読み込み
	CoreEngine.SetDriverFileName(szDriverFileName);
	if (!CmdLineOptions.m_fNoDriver && !CmdLineOptions.m_fStandby) {
		if (CoreEngine.IsDriverSpecified()) {
			StatusView.SetSingleText(TEXT("BonDriverの読み込み中..."));
			if (Core.OpenAndInitializeTuner(
					!Core.IsSilent()?CAppCore::OPEN_CAS_CARD_RETRY:0)) {
				UICore.OnTunerChanged();
			} else {
				Core.OnError(&CoreEngine,TEXT("BonDriverの初期化ができません。"));
			}
		} else {
			AddLog(TEXT("デフォルトのBonDriverはありません。"));
		}
	}

	// プラグインの読み込み
	if (!CmdLineOptions.m_fNoPlugin) {
		TCHAR szPluginDir[MAX_PATH];
		std::vector<LPCTSTR> ExcludePlugins;

		CPlugin::SetMessageWindow(MainWindow.GetHandle(),WM_APP_PLUGINMESSAGE);
		StatusView.SetSingleText(TEXT("プラグインを読み込んでいます..."));
		if (!CmdLineOptions.m_PluginsDirectory.IsEmpty()) {
			GetAbsolutePath(CmdLineOptions.m_PluginsDirectory.Get(),szPluginDir);
		} else {
			GetAppDirectory(szPluginDir);
			::PathAppend(szPluginDir,TEXT("Plugins"));
		}
		Logger.AddLog(TEXT("プラグインを \"%s\" から読み込みます..."),szPluginDir);
		if (CmdLineOptions.m_NoLoadPlugins.size()>0) {
			for (size_t i=0;i<CmdLineOptions.m_NoLoadPlugins.size();i++)
				ExcludePlugins.push_back(CmdLineOptions.m_NoLoadPlugins[i].Get());
		}
		PluginManager.LoadPlugins(szPluginDir,&ExcludePlugins);
	}

	CommandList.Initialize(&DriverManager,&PluginManager);
	CommandList.AddCommandCustomizer(&ZoomOptions);
	CommandList.AddCommandCustomizer(&PanAndScanOptions);

	if (!CmdLineOptions.m_fNoPlugin)
		PluginOptions.RestorePluginOptions();

	// 再生の初期化
	CoreEngine.m_DtvEngine.m_MediaViewer.SetUseAudioRendererClock(PlaybackOptions.GetUseAudioRendererClock());
	CoreEngine.SetDownMixSurround(PlaybackOptions.GetDownMixSurround());
	CoreEngine.SetSpdifOptions(PlaybackOptions.GetSpdifOptions());
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
#ifdef NETWORK_REMOCON_SUPPORT
			WORD RemoconPort=NetworkRemoconOptions.GetPort();
#endif

			StatusView.SetSingleText(TEXT("空きポートを検索しています..."));
			PortQuery.Query(MainWindow.GetHandle(),&UDPPort,CoreEngine.IsUDPDriver()?1243:2239
#ifdef NETWORK_REMOCON_SUPPORT
							,&RemoconPort
#endif
							);
			CmdLineOptions.m_UDPPort=UDPPort;
#ifdef NETWORK_REMOCON_SUPPORT
			NetworkRemoconOptions.SetTempPort(RemoconPort);
#endif
		}
#ifdef NETWORK_REMOCON_SUPPORT
		if (CmdLineOptions.m_fUseNetworkRemocon)
			NetworkRemoconOptions.SetTempEnable(true);
#endif
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

	CoreEngine.m_DtvEngine.SetTracer(nullptr);
	if (!MainWindow.GetStatusBarVisible())
		StatusView.SetVisible(false);
	StatusView.SetSingleText(nullptr);

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
			const CChannelList *pList=ChannelManager.GetCurrentChannelList();
			int i=pList->FindByIndex(
				CoreEngine.m_DtvEngine.m_BonSrcDecoder.GetCurSpace(),
				CoreEngine.m_DtvEngine.m_BonSrcDecoder.GetCurChannel());

			if (i>=0)
				UICore.DoCommand(CM_CHANNEL_FIRST+i);
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
	if (CmdLineOptions.m_fShowProgramGuide) {
		CMainWindow::ProgramGuideSpaceInfo SpaceInfo;

		if (!CmdLineOptions.m_ProgramGuideTuner.IsEmpty())
			SpaceInfo.pszTuner=CmdLineOptions.m_ProgramGuideTuner.Get();
		else
			SpaceInfo.pszTuner=nullptr;
		if (!CmdLineOptions.m_ProgramGuideSpace.IsEmpty())
			SpaceInfo.pszSpace=CmdLineOptions.m_ProgramGuideSpace.Get();
		else
			SpaceInfo.pszSpace=nullptr;

		MainWindow.ShowProgramGuide(true,
			UICore.GetFullscreen()?0:CMainWindow::PROGRAMGUIDE_SHOW_POPUP,
			&SpaceInfo);
	}

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

	PluginManager.SendStartupDoneEvent();

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

#ifdef NETWORK_REMOCON_SUPPORT
	if (NetworkRemoconOptions.GetUpdateFlags()!=0) {
		Core.InitializeChannel();
		if (pNetworkRemocon!=nullptr)
			pNetworkRemocon->GetChannel(&NetworkRemoconGetChannel);
	}
#endif

	if ((ProgramGuideOptions.GetUpdateFlags() & CProgramGuideOptions::UPDATE_EVENTICONS)!=0)
		Panel.ProgramListPanel.SetVisibleEventIcons(ProgramGuideOptions.GetVisibleEventIcons());
	ResidentManager.SetMinimizeToTray(ViewOptions.GetMinimizeToTray());

	SaveSettings(SETTINGS_SAVE_OPTIONS);

	PluginManager.SendSettingsChangeEvent();

	return true;
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


// 配色を適用する
bool CAppMain::ApplyColorScheme(const CColorScheme *pColorScheme)
{
	TVTest::Theme::CThemeManager ThemeManager(pColorScheme);

	MainWindow.SetTheme(&ThemeManager);
	StatusView.SetTheme(&ThemeManager);
	SideBar.SetTheme(&ThemeManager);
	Panel.SetTheme(&ThemeManager);
	CaptureWindow.SetTheme(&ThemeManager);
	Epg.ProgramGuide.SetTheme(&ThemeManager);
	Epg.ProgramGuideFrame.SetTheme(&ThemeManager);
	Epg.ProgramGuideDisplay.SetTheme(&ThemeManager);

	PluginManager.SendColorChangeEvent();

	return true;
}


#ifdef NETWORK_REMOCON_SUPPORT

CAppMain::CNetworkRemoconGetChannelReceiver::CNetworkRemoconGetChannelReceiver(CMainWindow *pMainWindow)
	: m_pMainWindow(pMainWindow)
{
}

void CAppMain::CNetworkRemoconGetChannelReceiver::OnReceive(LPCSTR pszText)
{
	int Channel=std::strtol(pszText,nullptr,10);
	::PostMessage(m_pMainWindow->GetHandle(),WM_APP_CHANNELCHANGE,Channel,0);
}


CAppMain::CNetworkRemoconGetDriverReceiver::CNetworkRemoconGetDriverReceiver()
	: m_hEvent(nullptr)
{
	m_szCurDriver[0]='\0';
}

CAppMain::CNetworkRemoconGetDriverReceiver::~CNetworkRemoconGetDriverReceiver()
{
	if (m_hEvent!=nullptr)
		::CloseHandle(m_hEvent);
}

void CAppMain::CNetworkRemoconGetDriverReceiver::OnReceive(LPCSTR pszText)
{
	LPCSTR p;
	int Sel,i;

	m_szCurDriver[0]='\0';
	p=pszText;
	while (*p!='\t') {
		if (*p=='\0')
			goto End;
		p++;
	}
	p++;
	Sel=0;
	for (;*p>='0' && *p<='9';p++)
		Sel=Sel*10+(*p-'0');
	if (*p!='\t')
		goto End;
	p++;
	for (i=0;i<=Sel && *p!='\0';i++) {
		while (*p!='\t') {
			if (*p=='\0')
				goto End;
			p++;
		}
		p++;
		if (i==Sel) {
			int j;

			for (j=0;*p!='\t' && *p!='\0';j++) {
				m_szCurDriver[j]=*p++;
			}
			m_szCurDriver[j]='\0';
			break;
		} else {
			while (*p!='\t' && *p!='\0')
				p++;
			if (*p=='\t')
				p++;
		}
	}
End:
	::SetEvent(m_hEvent);
}

void CAppMain::CNetworkRemoconGetDriverReceiver::Initialize()
{
	if (m_hEvent==nullptr)
		m_hEvent=::CreateEvent(nullptr,FALSE,FALSE,nullptr);
	else
		::ResetEvent(m_hEvent);
}

bool CAppMain::CNetworkRemoconGetDriverReceiver::Wait(DWORD TimeOut)
{
	return ::WaitForSingleObject(m_hEvent,TimeOut)==WAIT_OBJECT_0;
}

#endif	// NETWORK_REMOCON_SUPPORT


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
}

void CAppMain::CDtvEngineEventHandler::OnServiceInfoUpdated(CTsAnalyzer *pTsAnalyzer)
{
	OnServiceUpdated(pTsAnalyzer,false,false);
	m_App.MainWindow.PostMessage(WM_APP_CHANGECASLIBRARY,0,0);
}

void CAppMain::CDtvEngineEventHandler::OnServiceChanged(WORD ServiceID)
{
	m_App.MainWindow.PostMessage(WM_APP_SERVICECHANGED,ServiceID,0);
}

void CAppMain::CDtvEngineEventHandler::OnFileWriteError(CBufferedFileWriter *pFileWriter)
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

void CAppMain::CDtvEngineEventHandler::OnEmmProcessed()
{
	m_App.MainWindow.PostMessage(WM_APP_EMMPROCESSED,true,0);
}

void CAppMain::CDtvEngineEventHandler::OnEmmError(LPCTSTR pszText)
{
	m_App.MainWindow.PostMessage(WM_APP_EMMPROCESSED,false,0);
}

void CAppMain::CDtvEngineEventHandler::OnEcmError(LPCTSTR pszText)
{
	m_App.MainWindow.PostMessage(WM_APP_ECMERROR,0,(LPARAM)DuplicateString(pszText));
}

void CAppMain::CDtvEngineEventHandler::OnEcmRefused()
{
	m_App.MainWindow.PostMessage(WM_APP_ECMREFUSED,0,0);
}

void CAppMain::CDtvEngineEventHandler::OnCardReaderHung()
{
	m_App.MainWindow.PostMessage(WM_APP_CARDREADERHUNG,0,0);
}

void CAppMain::CDtvEngineEventHandler::OnTotUpdated(CTsAnalyzer *pTsAnalyzer)
{
	m_App.CoreEngine.SetAsyncStatusUpdatedFlag(CCoreEngine::STATUS_TOT);
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
	m_App.MainMenu.CheckItem(CM_STREAMINFO,false);
	m_App.SideBar.CheckItem(CM_STREAMINFO,false);
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
	m_App.MainMenu.CheckItem(CM_CAPTUREPREVIEW,false);
	m_App.SideBar.CheckItem(CM_CAPTUREPREVIEW,false);
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
