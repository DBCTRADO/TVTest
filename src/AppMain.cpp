/*
  TVTest
  Copyright(c) 2008-2020 DBCTRADO

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "AppUtil.h"
#include "DarkMode.h"
#include "InitialSettings.h"
#include "TVTestVersion.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


static HANDLE GetCurrentThreadHandle()
{
	HANDLE hThread = nullptr;

	if (!::DuplicateHandle(
				::GetCurrentProcess(),
				::GetCurrentThread(),
				::GetCurrentProcess(),
				&hThread, 0, FALSE, DUPLICATE_SAME_ACCESS))
		return nullptr;
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
	HANDLE m_hThread = nullptr;
	HANDLE m_hEndEvent = nullptr;
	HANDLE m_hContinueEvent = nullptr;
	HANDLE m_hMainThread = nullptr;
	DWORD m_Timeout;

	void Finalize();
	static DWORD WINAPI WatchThread(LPVOID lpParameter);
};


CAppTerminator::CAppTerminator(CAppMain &App)
	: m_App(App)
{
}


bool CAppTerminator::BeginWatching(DWORD Timeout)
{
	m_hEndEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
	m_hContinueEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_hEndEvent == nullptr || m_hContinueEvent == nullptr) {
		Finalize();
		return false;
	}
	m_hMainThread = GetCurrentThreadHandle();
	m_Timeout = Timeout;
	m_hThread = ::CreateThread(nullptr, 0, WatchThread, this, 0, nullptr);
	if (m_hThread == nullptr) {
		Finalize();
		return false;
	}

	return true;
}


void CAppTerminator::EndWatching()
{
	if (m_hThread != nullptr) {
		if (::SignalObjectAndWait(m_hEndEvent, m_hThread, 5000, FALSE) != WAIT_OBJECT_0)
			::TerminateThread(m_hThread, -1);
		::CloseHandle(m_hThread);
		m_hThread = nullptr;
	}

	Finalize();
}


void CAppTerminator::Continue()
{
	if (m_hContinueEvent != nullptr)
		::SetEvent(m_hContinueEvent);
}


void CAppTerminator::Finalize()
{
	if (m_hEndEvent != nullptr) {
		::CloseHandle(m_hEndEvent);
		m_hEndEvent = nullptr;
	}
	if (m_hContinueEvent != nullptr) {
		::CloseHandle(m_hContinueEvent);
		m_hContinueEvent = nullptr;
	}
	if (m_hMainThread != nullptr) {
		::CloseHandle(m_hMainThread);
		m_hMainThread = nullptr;
	}
}


DWORD WINAPI CAppTerminator::WatchThread(LPVOID lpParameter)
{
	CAppTerminator *pThis = static_cast<CAppTerminator*>(lpParameter);
	HANDLE hEvents[2];

	hEvents[0] = pThis->m_hEndEvent;
	hEvents[1] = pThis->m_hContinueEvent;

	for (;;) {
		const DWORD Result = ::WaitForMultipleObjects(2, hEvents, FALSE, pThis->m_Timeout);
		if (Result == WAIT_OBJECT_0)
			break;

		if (Result == WAIT_TIMEOUT) {
			pThis->m_App.AddLog(
				CLogItem::LogType::Warning,
				TEXT("終了処理がタイムアウトしました({}ms)。プロセスを強制的に終了させます。"),
				pThis->m_Timeout);

			if (pThis->m_hMainThread != nullptr) {
				::CancelSynchronousIo(pThis->m_hMainThread);
			}

			pThis->Finalize();

			::ExitProcess(-1);
		}
	}

	return 0;
}

#endif	// WATCH_EXIT



HICON CAppMain::m_hicoApp = nullptr;
HICON CAppMain::m_hicoAppSmall = nullptr;


CAppMain::CAppMain()
	: SideBar(&CommandManager)
	, ChannelDisplay(&EPGDatabase)

	, Epg(EPGDatabase, EventSearchOptions)

	, OSDManager(&OSDOptions)
	, StatusOptions(&StatusView)
	, SideBarOptions(&SideBar, &ZoomOptions)
	, ProgramGuideOptions(&Epg.ProgramGuide, &PluginManager)
	, PluginOptions(&PluginManager)
	, TSProcessorOptions(TSProcessorManager)
	, FeaturedEvents(EventSearchOptions)
{
	UICore.SetSkin(&MainWindow);

	AppEventManager.AddEventHandler(&PluginManager);

	StreamInfo.SetEventHandler(&m_StreamInfoEventHandler);
	CaptureWindow.SetEventHandler(&m_CaptureWindowEventHandler);
}


CAppMain::~CAppMain()
{
	if (m_hicoApp != nullptr) {
		::DestroyIcon(m_hicoApp);
		m_hicoApp = nullptr;
	}
	if (m_hicoAppSmall != nullptr) {
		::DestroyIcon(m_hicoAppSmall);
		m_hicoAppSmall = nullptr;
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


bool CAppMain::GetAppFilePath(String *pPath) const
{
	if (pPath == nullptr)
		return false;

	TCHAR szPath[MAX_PATH];
	const DWORD Length = ::GetModuleFileName(nullptr, szPath, MAX_PATH);
	if ((Length == 0) || (Length >= MAX_PATH)) {
		pPath->clear();
		return false;
	}

	pPath->assign(szPath);

	return true;
}


bool CAppMain::GetAppDirectory(String *pDirectory) const
{
	if (pDirectory == nullptr)
		return false;

	TCHAR szDir[MAX_PATH];
	const DWORD Length = ::GetModuleFileName(nullptr, szDir, MAX_PATH);
	if ((Length == 0) || (Length >= MAX_PATH)) {
		pDirectory->clear();
		return false;
	}

	::PathRemoveFileSpec(szDir);

	pDirectory->assign(szDir);

	return true;
}


bool CAppMain::GetAppDirectory(LPTSTR pszDirectory) const
{
	if (pszDirectory == nullptr)
		return false;

	const DWORD Length = ::GetModuleFileName(nullptr, pszDirectory, MAX_PATH);
	if ((Length == 0) || (Length >= MAX_PATH)) {
		pszDirectory[0] = _T('\0');
		return false;
	}

	::PathRemoveFileSpec(pszDirectory);

	return true;
}


bool CAppMain::IsFirstExecute() const
{
	return m_fFirstExecute;
}


void CAppMain::Initialize()
{
	TCHAR szModuleFileName[MAX_PATH];

	::GetModuleFileName(nullptr, szModuleFileName, MAX_PATH);

	if (CmdLineOptions.m_IniFileName.empty()) {
		m_IniFileName = szModuleFileName;
		m_IniFileName.RenameExtension(TEXT(".ini"));
	} else {
		if (PathUtil::IsRelative(CmdLineOptions.m_IniFileName)) {
			CFilePath Dir(szModuleFileName);
			Dir.RemoveFileName();
			PathUtil::RelativeToAbsolute(
				&m_IniFileName, Dir, CmdLineOptions.m_IniFileName);
		} else {
			m_IniFileName = CmdLineOptions.m_IniFileName;
		}
	}

	m_FavoritesFileName = szModuleFileName;
	m_FavoritesFileName.RenameExtension(TEXT(".tvfavorites"));

	const bool fExists = m_IniFileName.IsFileExists();
	m_fFirstExecute = !fExists && CmdLineOptions.m_IniFileName.empty();
	if (fExists) {
		AddLog(TEXT("設定を読み込んでいます..."));
		LoadSettings();
	}
}


void CAppMain::Finalize()
{
#ifdef WATCH_EXIT
	CAppTerminator Terminator(*this);
	if (m_ExitTimeout != 0)
		Terminator.BeginWatching(m_ExitTimeout);
#define FINALIZE_CONTINUE Terminator.Continue();
#else
#define FINALIZE_CONTINUE
#endif

	RecordManager.Terminate();

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
	CoreEngine.SetLogger(nullptr);

	FINALIZE_CONTINUE

	if (!CmdLineOptions.m_fNoPlugin)
		PluginOptions.StorePluginOptions();
	PluginManager.FreePlugins();

	FINALIZE_CONTINUE

	// 終了時の負荷で他のプロセスの録画がドロップすることがあるらしい...
	::SetPriorityClass(::GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);

	if (!CmdLineOptions.m_fNoEpg) {
		EpgOptions.SaveEpgFile(&EPGDatabase);
	}

	FINALIZE_CONTINUE

	EpgOptions.SaveLogoFile();
	EpgOptions.Finalize();

#ifdef WATCH_EXIT
	Terminator.EndWatching();
#endif

	if (FavoritesManager.GetModified())
		FavoritesManager.Save(m_FavoritesFileName.c_str());

	AddLog(TEXT("設定を保存しています..."));
	SaveSettings(SaveSettingsFlag::Status | (m_fInitialSettings ? SaveSettingsFlag::Options : SaveSettingsFlag::None));
}


void CAppMain::Exit()
{
	UICore.DoCommandAsync(CM_EXIT);
}


bool CAppMain::LoadSettings()
{
	CSettings &Settings = m_Settings;

	if (!Settings.Open(m_IniFileName.c_str(), CSettings::OpenFlag::Read | CSettings::OpenFlag::WriteVolatile)) {
		AddLog(CLogItem::LogType::Error, TEXT("設定ファイル \"{}\" を開けません。"), m_IniFileName);
		return false;
	}

	if (!CmdLineOptions.m_IniValueList.empty()) {
		for (const auto &Entry : CmdLineOptions.m_IniValueList) {
			TRACE(
				TEXT("Override INI entry : [{}] {}={}\n"),
				Entry.Section, Entry.Name, Entry.Value);
			if (Settings.SetSection(Entry.Section.empty() ? TEXT("Settings") : Entry.Section.c_str())) {
				Settings.Write(Entry.Name.c_str(), Entry.Value);
			}
		}
	}

	if (Settings.SetSection(TEXT("Settings"))) {
		int Value;
		TCHAR szText[MAX_PATH];
		int Left, Top, Width, Height;
		bool f;

		Settings.Read(TEXT("EnablePlay"), &m_fEnablePlaybackOnStart);
		if (Settings.Read(TEXT("Volume"), &Value))
			CoreEngine.SetVolume(std::clamp(Value, 0, CCoreEngine::MAX_VOLUME));
		int Gain = 100, SurroundGain;
		Settings.Read(TEXT("VolumeNormalizeLevel"), &Gain);
		if (!Settings.Read(TEXT("SurroundGain"), &SurroundGain))
			SurroundGain = Gain;
		CoreEngine.SetAudioGainControl(Gain, SurroundGain);
		Settings.Read(TEXT("ShowInfoWindow"), &Panel.fShowPanelWindow);
		if (Settings.Read(TEXT("OptionPage"), &Value))
			m_OptionDialog.SetCurrentPage(Value);

		if (Settings.Read(TEXT("RecOptionFileName"), szText, MAX_PATH) && szText[0] != '\0')
			RecordManager.SetFileName(szText);
		/*
		if (Settings.Read(TEXT("RecOptionExistsOperation"), &Value))
			RecordManager.SetFileExistsOperation(static_cast<CRecordManager::FileExistsOperation>(Value));
		*/
		/*
		if (Settings.Read(TEXT("RecOptionStopTimeSpec"), &f))
			RecordManager.SetStopTimeSpec(f);
		unsigned int Time;
		if (Settings.Read(TEXT("RecOptionStopTime"), &Time))
			RecordManager.SetStopTime(Time);
		*/

		Panel.Frame.GetPosition(&Left, &Top, &Width, &Height);
		Settings.Read(TEXT("InfoLeft"), &Left);
		Settings.Read(TEXT("InfoTop"), &Top);
		Settings.Read(TEXT("InfoWidth"), &Width);
		Settings.Read(TEXT("InfoHeight"), &Height);
		Panel.Frame.SetPosition(Left, Top, Width, Height);
		Panel.Frame.MoveToMonitorInside();
		if (Settings.Read(TEXT("PanelFloating"), &f))
			Panel.Frame.SetFloating(f);
		if (Settings.Read(TEXT("PanelDockingWidth"), &Value))
			Panel.Frame.SetDockingWidth(Value);
		if (Settings.Read(TEXT("PanelDockingHeight"), &Value))
			Panel.Frame.SetDockingHeight(Value);

		Epg.ProgramGuideFrame.GetPosition(&Left, &Top, &Width, &Height);
		Settings.Read(TEXT("ProgramGuideLeft"), &Left);
		Settings.Read(TEXT("ProgramGuideTop"), &Top);
		Settings.Read(TEXT("ProgramGuideWidth"), &Width);
		Settings.Read(TEXT("ProgramGuideHeight"), &Height);
		Epg.ProgramGuideFrame.SetPosition(Left, Top, Width, Height);
		Epg.ProgramGuideFrame.MoveToMonitorInside();
		if (Settings.Read(TEXT("ProgramGuideMaximized"), &f) && f)
			Epg.ProgramGuideFrame.SetMaximize(f);
		if (Settings.Read(TEXT("ProgramGuideAlwaysOnTop"), &f))
			Epg.ProgramGuideFrame.SetAlwaysOnTop(f);

		CaptureWindow.GetPosition(&Left, &Top, &Width, &Height);
		Settings.Read(TEXT("CapturePreviewLeft"), &Left);
		Settings.Read(TEXT("CapturePreviewTop"), &Top);
		Settings.Read(TEXT("CapturePreviewWidth"), &Width);
		Settings.Read(TEXT("CapturePreviewHeight"), &Height);
		CaptureWindow.SetPosition(Left, Top, Width, Height);
		CaptureWindow.MoveToMonitorInside();
		if (Settings.Read(TEXT("CaptureStatusBar"), &f))
			CaptureWindow.ShowStatusBar(f);

		StreamInfo.LoadSettings(Settings);

		CBasicDialog::Position Pos;
		if (Settings.Read(TEXT("OrganizeFavoritesLeft"), &Pos.x)
				&& Settings.Read(TEXT("OrganizeFavoritesTop"), &Pos.y)) {
			Settings.Read(TEXT("OrganizeFavoritesWidth"), &Pos.Width);
			Settings.Read(TEXT("OrganizeFavoritesHeight"), &Pos.Height);
			FavoritesManager.SetOrganizeDialogPos(Pos);
		}

		if (Settings.Read(TEXT("OptionDialogLeft"), &Pos.x)
				&& Settings.Read(TEXT("OptionDialogTop"), &Pos.y)) {
			Pos.Width = 0;
			Pos.Height = 0;
			Settings.Read(TEXT("OptionDialogWidth"), &Pos.Width);
			Settings.Read(TEXT("OptionDialogHeight"), &Pos.Height);
			m_OptionDialog.SetPosition(Pos);
		}

		Settings.Read(TEXT("ExitTimeout"), &m_ExitTimeout);
		Settings.Read(TEXT("IncrementUDPPort"), &m_fIncrementNetworkPort);

		MainWindow.ReadSettings(Settings);
		GeneralOptions.ReadSettings(Settings);
		ViewOptions.ReadSettings(Settings);
		OSDOptions.ReadSettings(Settings);
		NotificationBarOptions.ReadSettings(Settings);
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
	VideoDecoderOptions.LoadSettings(Settings);

	return true;
}


bool CAppMain::SaveSettings(SaveSettingsFlag Flags)
{
	CSettings Settings;
	if (!Settings.Open(m_IniFileName.c_str(), CSettings::OpenFlag::Write)) {
		TCHAR szMessage[64 + MAX_PATH];
		StringFormat(
			szMessage,
			TEXT("設定ファイル \"{}\" を開けません。"),
			m_IniFileName);
		AddLogRaw(CLogItem::LogType::Error, szMessage);
		if (!Core.IsSilent())
			UICore.GetSkin()->ShowErrorMessage(szMessage);
		return false;
	}

	if (Settings.SetSection(TEXT("Settings"))) {
		Settings.Write(TEXT("Version"), VERSION_TEXT);

		if (!!(Flags & SaveSettingsFlag::Status)) {
			int Left, Top, Width, Height;
			CBasicDialog::Position Pos;

			Settings.Write(TEXT("EnablePlay"), m_fEnablePlaybackOnStart);
			Settings.Write(TEXT("Volume"), CoreEngine.GetVolume());
			int Gain, SurroundGain;
			CoreEngine.GetAudioGainControl(&Gain, &SurroundGain);
			Settings.Write(TEXT("VolumeNormalizeLevel"), Gain);
			Settings.Write(TEXT("SurroundGain"), SurroundGain);
			Settings.Write(TEXT("ShowInfoWindow"), Panel.fShowPanelWindow);
			Settings.Write(TEXT("OptionPage"), m_OptionDialog.GetCurrentPage());

			if (RecordManager.GetFileName() != nullptr)
				Settings.Write(TEXT("RecOptionFileName"), RecordManager.GetFileName());
			/*
			Settings.Write(TEXT("RecOptionExistsOperation"), RecordManager.GetFileExistsOperation());
			*/
			/*
			Settings.Write(TEXT("RecOptionStopTimeSpec"), RecordManager.GetStopTimeSpec());
			Settings.Write(TEXT("RecOptionStopTime"), RecordManager.GetStopTime());
			*/

			Panel.Frame.GetPosition(&Left, &Top, &Width, &Height);
			Settings.Write(TEXT("InfoLeft"), Left);
			Settings.Write(TEXT("InfoTop"), Top);
			Settings.Write(TEXT("InfoWidth"), Width);
			Settings.Write(TEXT("InfoHeight"), Height);
			Settings.Write(TEXT("PanelFloating"), Panel.Frame.GetFloating());
			Settings.Write(TEXT("PanelDockingWidth"), Panel.Frame.GetDockingWidth());
			Settings.Write(TEXT("PanelDockingHeight"), Panel.Frame.GetDockingHeight());

			Epg.ProgramGuideFrame.GetPosition(&Left, &Top, &Width, &Height);
			Settings.Write(TEXT("ProgramGuideLeft"), Left);
			Settings.Write(TEXT("ProgramGuideTop"), Top);
			Settings.Write(TEXT("ProgramGuideWidth"), Width);
			Settings.Write(TEXT("ProgramGuideHeight"), Height);
			Settings.Write(TEXT("ProgramGuideMaximized"), Epg.ProgramGuideFrame.GetMaximize());
			Settings.Write(TEXT("ProgramGuideAlwaysOnTop"), Epg.ProgramGuideFrame.GetAlwaysOnTop());

			CaptureWindow.GetPosition(&Left, &Top, &Width, &Height);
			Settings.Write(TEXT("CapturePreviewLeft"), Left);
			Settings.Write(TEXT("CapturePreviewTop"), Top);
			Settings.Write(TEXT("CapturePreviewWidth"), Width);
			Settings.Write(TEXT("CapturePreviewHeight"), Height);
			Settings.Write(TEXT("CaptureStatusBar"), CaptureWindow.IsStatusBarVisible());

			StreamInfo.SaveSettings(Settings);

			if (FavoritesManager.IsOrganizeDialogPosSet()) {
				FavoritesManager.GetOrganizeDialogPos(&Pos);
				Settings.Write(TEXT("OrganizeFavoritesLeft"), Pos.x);
				Settings.Write(TEXT("OrganizeFavoritesTop"), Pos.y);
				Settings.Write(TEXT("OrganizeFavoritesWidth"), Pos.Width);
				Settings.Write(TEXT("OrganizeFavoritesHeight"), Pos.Height);
			}

			if (m_OptionDialog.IsPositionSet()) {
				m_OptionDialog.GetPosition(&Pos);
				Settings.Write(TEXT("OptionDialogLeft"), Pos.x);
				Settings.Write(TEXT("OptionDialogTop"), Pos.y);
				if (Pos.Width > 0)
					Settings.Write(TEXT("OptionDialogWidth"), Pos.Width);
				if (Pos.Height > 0)
					Settings.Write(TEXT("OptionDialogHeight"), Pos.Height);
			}

			//Settings.Write(TEXT("ExitTimeout"), m_ExitTimeout);
			//Settings.Write(TEXT("IncrementUDPPort"), m_fIncrementNetworkPort);

			MainWindow.WriteSettings(Settings);
		}
	}

	static const struct {
		CSettingsBase *pSettings;
		bool fHasStatus;
	} SettingsList[] = {
		{&GeneralOptions,                true},
		{&StatusOptions,                 true},
		{&SideBarOptions,                true},
		{&PanelOptions,                  true},
		{&DriverOptions,                 true},
		{&VideoOptions,                  false},
		{&AudioOptions,                  false},
		{&PlaybackOptions,               true},
		{&RecordOptions,                 true},
		{&CaptureOptions,                true},
		{&PluginOptions,                 true},
		{&ViewOptions,                   false},
		{&OSDOptions,                    false},
		{&NotificationBarOptions,        false},
		{&MenuOptions,                   false},
		{&ColorSchemeOptions,            false},
		{&OperationOptions,              false},
		{&Accelerator,                   false},
		{&ControllerManager,             false},
		{&ChannelScan,                   false},
		{&EpgOptions,                    false},
		{&ProgramGuideOptions,           true},
		{&TSProcessorOptions,            true},
		{&TaskbarOptions,                false},
		{&VideoDecoderOptions,           true},
		{&Logger,                        false},
//		{&ZoomOptions,                   false},
//		{&PanAndScanOptions,             false},
		{&Epg.ProgramGuideFrameSettings, true},
		{&HomeDisplay,                   true},
		{&FeaturedEvents,                true},
	};

	for (size_t i = 0; i < lengthof(SettingsList); i++) {
		CSettingsBase *pSettings = SettingsList[i].pSettings;
		if ((!!(Flags & SaveSettingsFlag::Options) && pSettings->IsChanged())
				|| (!!(Flags & SaveSettingsFlag::Status) && SettingsList[i].fHasStatus)) {
			if (pSettings->SaveSettings(Settings))
				pSettings->ClearChanged();
		}
	}

	if (!!(Flags & SaveSettingsFlag::Status)) {
		RecentChannelList.SaveSettings(Settings);
		Panel.InfoPanel.SaveSettings(Settings);
		Panel.ProgramListPanel.SaveSettings(Settings);
		Panel.ChannelPanel.SaveSettings(Settings);
		Panel.CaptionPanel.SaveSettings(Settings);
		TaskbarManager.SaveSettings(Settings);
	}

	return true;
}


int CAppMain::Main(HINSTANCE hInstance, LPCTSTR pszCmdLine, int nCmdShow)
{
	m_hInst = hInstance;

	// コマンドラインの解析
	if (pszCmdLine[0] != _T('\0')) {
		AddLog(TEXT("コマンドラインオプション : {}"), pszCmdLine);

		CmdLineOptions.Parse(pszCmdLine);

		if (CmdLineOptions.m_TvRockDID >= 0)
			CmdLineOptions.m_fSilent = true;
		if (CmdLineOptions.m_fSilent)
			Core.SetSilent(true);
		if (CmdLineOptions.m_fTray)
			CmdLineOptions.m_fMinimize = true;

		if (CmdLineOptions.m_fProgramGuideOnly) {
			CmdLineOptions.m_fShowProgramGuide = true;
			CmdLineOptions.m_fStandby = true;
		}
	}

	AppCommand.RegisterDefaultCommands();

	Initialize();

	CAppMutex AppMutex(true);

	if (CmdLineOptions.m_fJumpList && TaskbarOptions.GetJumpListKeepSingleTask())
		CmdLineOptions.m_fSingleTask = true;

	// 複数起動のチェック
	if (AppMutex.AlreadyExists()
			&& (GeneralOptions.GetKeepSingleTask() || CmdLineOptions.m_fSingleTask)) {
		AddLog(TEXT("複数起動が禁止されています。"));
		CTVTestWindowFinder Finder;
		const HWND hwnd = Finder.FindCommandLineTarget();
		if (::IsWindow(hwnd)) {
			if (!SendInterprocessMessage(
						hwnd, PROCESS_MESSAGE_EXECUTE,
						pszCmdLine, (::lstrlen(pszCmdLine) + 1) * sizeof(TCHAR))) {
				AddLog(CLogItem::LogType::Error, TEXT("既存のプロセスにメッセージを送信できません。"));
			}
			return 0;
		}
		if (!CmdLineOptions.m_fSingleTask) {
			if (!Core.IsSilent()) {
				::MessageBox(
					nullptr,
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

		iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
		iccex.dwICC =
			ICC_UPDOWN_CLASS |
			ICC_BAR_CLASSES |
			ICC_LISTVIEW_CLASSES |
			ICC_TREEVIEW_CLASSES |
			ICC_DATE_CLASSES |
			ICC_PROGRESS_CLASS |
			ICC_TAB_CLASSES;
		::InitCommonControlsEx(&iccex);
	}

	// Buffered paint の初期化
	CBufferedPaint::Initialize();

	// BonDriver の検索
	{
		String Directory;
		CoreEngine.GetDriverDirectoryPath(&Directory);
		DriverManager.Find(Directory.c_str());
	}
	// チューナー仕様定義の読み込み
	{
		TCHAR szTunerSpecFileName[MAX_PATH];
		::GetModuleFileName(nullptr, szTunerSpecFileName, lengthof(szTunerSpecFileName));
		::PathRenameExtension(szTunerSpecFileName, TEXT(".tuner.ini"));
		if (::PathFileExists(szTunerSpecFileName)) {
			AddLog(TEXT("チューナー仕様定義を \"{}\" から読み込みます..."), szTunerSpecFileName);
			DriverManager.LoadTunerSpec(szTunerSpecFileName);
		}
	}
	DriverOptions.Initialize(&DriverManager);

	// スタイル設定の読み込み
	{
		TCHAR szStyleFileName[MAX_PATH];
		if (!CmdLineOptions.m_StyleFileName.empty()) {
			GetAbsolutePath(CmdLineOptions.m_StyleFileName.c_str(), szStyleFileName);
		} else {
			::GetModuleFileName(nullptr, szStyleFileName, lengthof(szStyleFileName));
			::PathRenameExtension(szStyleFileName, TEXT(".style.ini"));
		}
		if (::PathFileExists(szStyleFileName)) {
			AddLog(TEXT("スタイル設定を \"{}\" から読み込みます..."), szStyleFileName);
			StyleManager.Load(szStyleFileName);
		}
	}

	if (StyleManager.IsUseDarkMenu())
		SetAppAllowDarkMode(true);

	// 初期設定ダイアログを表示するか
	m_fInitialSettings =
		CmdLineOptions.m_fInitialSettings
		|| (m_fFirstExecute && CmdLineOptions.m_DriverName.empty());

	GraphicsCore.Initialize();

	String DriverFileName;

	// 初期設定ダイアログの表示
	if (m_fInitialSettings) {
		CInitialSettings InitialSettings(&DriverManager);

		if (!InitialSettings.Show(nullptr))
			return 0;
		DriverFileName = InitialSettings.GetDriverFileName();
		GeneralOptions.SetDefaultDriverName(DriverFileName.c_str());
		GeneralOptions.SetChanged();
		VideoOptions.SetMpeg2DecoderName(InitialSettings.GetMpeg2DecoderName());
		VideoOptions.SetH264DecoderName(InitialSettings.GetH264DecoderName());
		VideoOptions.SetH265DecoderName(InitialSettings.GetH265DecoderName());
		VideoOptions.SetVideoRendererType(InitialSettings.GetVideoRenderer());
		VideoOptions.SetChanged();
		RecordOptions.SetSaveFolder(InitialSettings.GetRecordFolder());
		RecordOptions.SetChanged();
	} else if (!CmdLineOptions.m_DriverName.empty()) {
		DriverFileName = CmdLineOptions.m_DriverName;
	} else if (!CmdLineOptions.m_fNoDriver) {
		GeneralOptions.GetFirstDriverName(&DriverFileName);
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
		int Left, Top, Width, Height;
		MainWindow.GetPosition(&Left, &Top, &Width, &Height);
		if (CmdLineOptions.m_WindowLeft != CCommandLineOptions::INVALID_WINDOW_POS)
			Left = CmdLineOptions.m_WindowLeft;
		if (CmdLineOptions.m_WindowTop != CCommandLineOptions::INVALID_WINDOW_POS)
			Top = CmdLineOptions.m_WindowTop;
		if (CmdLineOptions.m_WindowWidth > 0)
			Width = CmdLineOptions.m_WindowWidth;
		if (CmdLineOptions.m_WindowHeight > 0)
			Height = CmdLineOptions.m_WindowHeight;
		MainWindow.SetPosition(Left, Top, Width, Height);
	}

	ColorSchemeOptions.SetEventHandler(&UICore);
	ColorSchemeOptions.ApplyColorScheme();

	if (CmdLineOptions.m_fNoTSProcessor)
		CoreEngine.EnableTSProcessor(false);
	CoreEngine.SetNoEpg(CmdLineOptions.m_fNoEpg);

	CoreEngine.CreateFilters();

	LibISDB::EPGDatabaseFilter *pEPGDatabaseFilter = CoreEngine.GetFilter<LibISDB::EPGDatabaseFilter>();
	if (pEPGDatabaseFilter != nullptr)
		pEPGDatabaseFilter->SetEPGDatabase(&EPGDatabase);

	if (nCmdShow == SW_SHOWMINIMIZED || nCmdShow == SW_SHOWMINNOACTIVE || nCmdShow == SW_MINIMIZE)
		CmdLineOptions.m_fMinimize = true;
	if (CmdLineOptions.m_fStandby && CmdLineOptions.m_fMinimize)
		CmdLineOptions.m_fMinimize = false;

	const bool fMinimizeToTray = CmdLineOptions.m_fTray || ViewOptions.GetMinimizeToTray();

	DWORD WindowStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
	DWORD WindowExStyle = 0;
	if (CmdLineOptions.m_fMinimize) {
		WindowStyle |= WS_MINIMIZE;
		WindowExStyle |= WS_EX_NOACTIVATE;
		if (!fMinimizeToTray)
			WindowExStyle |= WS_EX_APPWINDOW;
	}

	// ウィンドウの作成
	if (!MainWindow.Create(nullptr, WindowStyle, WindowExStyle)) {
		AddLog(CLogItem::LogType::Error, TEXT("ウィンドウが作成できません。"));
		if (!Core.IsSilent())
			MessageBox(nullptr, TEXT("ウィンドウが作成できません。"), nullptr, MB_OK | MB_ICONSTOP);
		return 0;
	}

	if (!CmdLineOptions.m_fStandby && !CmdLineOptions.m_fMinimize)
		MainWindow.Show(nCmdShow);

	GeneralOptions.Apply(COptions::UPDATE_ALL);

	TaskTrayManager.Initialize(MainWindow.GetHandle(), WM_APP_TRAYICON);
	TaskTrayManager.SetMinimizeToTray(fMinimizeToTray);
	if (CmdLineOptions.m_fMinimize) {
		MainWindow.InitMinimize();
		MainWindow.SetWindowExStyle(MainWindow.GetWindowExStyle() & ~(WS_EX_NOACTIVATE | WS_EX_APPWINDOW));
	}

	if (CmdLineOptions.m_fMpeg2 || CmdLineOptions.m_fH264 || CmdLineOptions.m_fH265) {
		CoreEngine.SetStreamTypePlayable(
			LibISDB::STREAM_TYPE_MPEG2_VIDEO, CmdLineOptions.m_fMpeg2);
		CoreEngine.SetStreamTypePlayable(
			LibISDB::STREAM_TYPE_H264, CmdLineOptions.m_fH264);
		CoreEngine.SetStreamTypePlayable(
			LibISDB::STREAM_TYPE_H265, CmdLineOptions.m_fH265);
	} else {
		CoreEngine.SetStreamTypePlayable(LibISDB::STREAM_TYPE_MPEG2_VIDEO, true);
		CoreEngine.SetStreamTypePlayable(LibISDB::STREAM_TYPE_H264, true);
		CoreEngine.SetStreamTypePlayable(LibISDB::STREAM_TYPE_H265, true);
	}
	CoreEngine.SetStreamTypePlayable(LibISDB::STREAM_TYPE_MPEG1_AUDIO, true);
	CoreEngine.SetStreamTypePlayable(LibISDB::STREAM_TYPE_MPEG2_AUDIO, true);
	CoreEngine.SetStreamTypePlayable(LibISDB::STREAM_TYPE_AAC, true);
	CoreEngine.SetStreamTypePlayable(LibISDB::STREAM_TYPE_AC3, true);

	if (CmdLineOptions.m_f1Seg || PlaybackOptions.Is1SegModeOnStartup())
		Core.Set1SegMode(true, false);
	CoreEngine.SetMinTimerResolution(PlaybackOptions.GetMinTimerResolution());

	RecordManager.SetRecorderFilter(&CoreEngine, CoreEngine.GetFilter<LibISDB::RecorderFilter>());

	ViewOptions.Apply(COptions::UPDATE_ALL);
	VideoOptions.Apply(COptions::UPDATE_ALL);
	PlaybackOptions.Apply(COptions::UPDATE_ALL);
	VideoDecoderOptions.ApplyVideoDecoderSettings();
	LibISDB::LogoDownloaderFilter *pLogoDownloader =
		CoreEngine.GetFilter<LibISDB::LogoDownloaderFilter>();
	if (pLogoDownloader != nullptr)
		pLogoDownloader->SetLogoHandler(&LogoManager);
	CoreEngine.SetLogger(&UICore);
	UICore.SetStatusBarTrace(true);
	RecordOptions.Apply(COptions::UPDATE_ALL);

	// プラグインの読み込み
	if (!CmdLineOptions.m_fNoPlugin) {
		TCHAR szPluginDir[MAX_PATH];
		std::vector<LPCTSTR> ExcludePlugins;

		CPlugin::SetMessageWindow(MainWindow.GetHandle(), WM_APP_PLUGINMESSAGE);
		StatusView.SetSingleText(TEXT("プラグインを読み込んでいます..."));
		if (!CmdLineOptions.m_PluginsDirectory.empty()) {
			GetAbsolutePath(CmdLineOptions.m_PluginsDirectory.c_str(), szPluginDir);
		} else {
			GetAppDirectory(szPluginDir);
			::PathAppend(szPluginDir, TEXT("Plugins"));
		}
		AddLog(TEXT("プラグインを \"{}\" から読み込みます..."), szPluginDir);
		if (CmdLineOptions.m_NoLoadPlugins.size() > 0) {
			for (const String &Plugin : CmdLineOptions.m_NoLoadPlugins)
				ExcludePlugins.push_back(Plugin.c_str());
		}
		PluginManager.LoadPlugins(szPluginDir, &ExcludePlugins);
	}

	AppCommand.RegisterDynamicCommands();
	CommandManager.AddCommandCustomizer(&ZoomOptions);
	CommandManager.AddCommandCustomizer(&PanAndScanOptions);

	if (!CmdLineOptions.m_fNoPlugin)
		PluginOptions.RestorePluginOptions();

	PluginManager.RegisterStatusItems();
	StatusOptions.ApplyItemList();
	PluginManager.SendStatusItemCreatedEvent();
	MainWindow.OnStatusBarInitialized();

	SideBarOptions.ApplyItemList();
	if (MainWindow.GetSideBarVisible()) {
		MainWindow.GetLayoutBase().SetContainerVisible(CONTAINER_ID_SIDEBAR, true);
		SideBar.Update();
	}

	CoreEngine.AddEventListener(&m_EngineEventListener);
	CoreEngine.BuildEngine();
	TSProcessorManager.OpenDefaultFilters();

	// BonDriver の読み込み
	CoreEngine.SetDriverFileName(DriverFileName.c_str());
	if (!CmdLineOptions.m_fNoDriver && !CmdLineOptions.m_fStandby) {
		if (CoreEngine.IsDriverSpecified()) {
			StatusView.SetSingleText(TEXT("BonDriverの読み込み中..."));
			if (Core.OpenAndInitializeTuner(
						Core.IsSilent() ?
							CAppCore::OpenTunerFlag::NoUI :
							CAppCore::OpenTunerFlag::RetryDialog)) {
				AppEventManager.OnTunerChanged();
			} else {
				Core.OnError(&CoreEngine, TEXT("BonDriverの初期化ができません。"));
			}
		} else {
			AddLog(TEXT("デフォルトのBonDriverはありません。"));
		}
	}

	// 再生の初期化
	LibISDB::ViewerFilter *pViewerFilter = CoreEngine.GetFilter<LibISDB::ViewerFilter>();
	if (pViewerFilter != nullptr)
		pViewerFilter->SetUseAudioRendererClock(PlaybackOptions.GetUseAudioRendererClock());
	CoreEngine.SetSPDIFOptions(AudioOptions.GetSpdifOptions());
	if (CmdLineOptions.m_Volume >= 0)
		CoreEngine.SetVolume(std::min(CmdLineOptions.m_Volume, CCoreEngine::MAX_VOLUME));
	if (PlaybackOptions.IsMuteOnStartUp() || CmdLineOptions.m_fMute)
		UICore.SetMute(true);

	// 一つのコーデックのみ指定されている場合、ver.0.8.xまでとの互換動作として先にフィルタグラフを構築
	if (!CmdLineOptions.m_fStandby && !CmdLineOptions.m_fNoDirectShow) {
		unsigned int StreamTypeFlags = 0;
		if (CmdLineOptions.m_fMpeg2)
			StreamTypeFlags |= 0x01;
		if (CmdLineOptions.m_fH264)
			StreamTypeFlags |= 0x02;
		if (CmdLineOptions.m_fH265)
			StreamTypeFlags |= 0x04;
		BYTE VideoStreamType = 0;
		switch (StreamTypeFlags) {
		case 0x01: VideoStreamType = LibISDB::STREAM_TYPE_MPEG2_VIDEO; break;
		case 0x02: VideoStreamType = LibISDB::STREAM_TYPE_H264;        break;
		case 0x04: VideoStreamType = LibISDB::STREAM_TYPE_H265;        break;
		}
		if (VideoStreamType != 0)
			UICore.InitializeViewer(VideoStreamType);
	}

	const bool fEnablePlayback =
		!PlaybackOptions.GetRestorePlayStatus() || m_fEnablePlaybackOnStart;
	if (fEnablePlayback
			&& CoreEngine.IsViewerOpen()
			&& !CmdLineOptions.m_fNoView
			&& !CmdLineOptions.m_fMinimize) {
		UICore.EnableViewer(true);
	} else {
		MainWindow.EnablePlayback(
			fEnablePlayback
			&& !CmdLineOptions.m_fNoView
			&& !CmdLineOptions.m_fNoDirectShow);
	}

	if (CoreEngine.IsNetworkDriver()) {
		if (m_fIncrementNetworkPort) {
			CPortQuery PortQuery;
			WORD UDPPort =
				CmdLineOptions.m_UDPPort > 0 ? static_cast<uint16_t>(CmdLineOptions.m_UDPPort) :
				CoreEngine.IsUDPDriver() ? 1234_u16 : 2230_u16;

			StatusView.SetSingleText(TEXT("空きポートを検索しています..."));
			PortQuery.Query(MainWindow.GetHandle(), &UDPPort, CoreEngine.IsUDPDriver() ? 1243 : 2239);
			CmdLineOptions.m_UDPPort = UDPPort;
		}
	}

	StatusView.SetSingleText(TEXT("チャンネル設定を読み込んでいます..."));
	Core.InitializeChannel();

	StatusView.SetSingleText(TEXT("ロゴを読み込んでいます..."));
	EpgOptions.LoadLogoFile();

	{
		TCHAR szDRCSMapName[MAX_PATH];

		GetAppDirectory(szDRCSMapName);
		::PathAppend(szDRCSMapName, TEXT("DRCSMap.ini"));
		if (::PathFileExists(szDRCSMapName)) {
			StatusView.SetSingleText(TEXT("DRCSマップを読み込んでいます..."));
			Panel.CaptionPanel.LoadDRCSMap(szDRCSMapName);
		}
	}

	{
		TCHAR szSearchFileName[MAX_PATH];

		::GetModuleFileName(nullptr, szSearchFileName, lengthof(szSearchFileName));
		::PathRenameExtension(szSearchFileName, TEXT(".search.ini"));
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
		Panel.Frame.SetPanelVisible(true, true);
		Panel.Frame.Update();
	}

	if (!CmdLineOptions.m_fNoEpg) {
		EpgOptions.LoadEpgFile(
			&EPGDatabase, &EpgLoadEventHandler,
			CEpgOptions::EpgFileLoadFlag::AllData);
	}

	ApplyEventInfoFont();

	if (CmdLineOptions.m_fFullscreen)
		UICore.SetFullscreen(true);

	// 初期チャンネルを設定する
	if (CoreEngine.IsSourceOpen()) {
		if (CoreEngine.IsNetworkDriver()) {
			const int FirstPort = CoreEngine.IsUDPDriver() ? 1234 : 2230;
			int Port = FirstPort;
			if (static_cast<int>(CmdLineOptions.m_UDPPort) >= FirstPort && static_cast<int>(CmdLineOptions.m_UDPPort) < FirstPort + 10)
				Port = CmdLineOptions.m_UDPPort;
			else if (RestoreChannelInfo.Channel >= 0 && RestoreChannelInfo.Channel < 10)
				Port = FirstPort + RestoreChannelInfo.Channel;
			Core.SetChannel(0, Port - FirstPort, CmdLineOptions.m_ServiceID);
			if (CmdLineOptions.m_ControllerChannel > 0)
				Core.SetCommandLineChannel(&CmdLineOptions);
		} else if (m_fFirstExecute
				&& ChannelManager.GetFileAllChannelList()->NumChannels() == 0) {
			if (MainWindow.ShowMessage(
						TEXT("最初にチャンネルスキャンを行うことをおすすめします。\r\n")
						TEXT("今すぐチャンネルスキャンを行いますか?"),
						TEXT("チャンネルスキャンの確認"),
						MB_YESNO | MB_ICONINFORMATION) == IDYES) {
				ShowOptionDialog(MainWindow.GetHandle(), COptionDialog::PAGE_CHANNELSCAN);
			}
		} else if (CmdLineOptions.IsChannelSpecified()) {
			Core.SetCommandLineChannel(&CmdLineOptions);
		} else if (RestoreChannelInfo.Space >= 0
				&& RestoreChannelInfo.Channel >= 0) {
			Core.RestoreChannel();
		} else {
			const LibISDB::BonDriverSourceFilter *pSourceFilter =
				CoreEngine.GetFilter<LibISDB::BonDriverSourceFilter>();
			if (pSourceFilter != nullptr) {
				const int CurSpace = static_cast<int>(pSourceFilter->GetCurSpace());
				const int CurChannel = static_cast<int>(pSourceFilter->GetCurChannel());
				if (CurSpace >= 0 && CurChannel >= 0) {
					const CChannelList *pList = ChannelManager.GetCurrentChannelList();
					const int i = pList->FindByIndex(CurSpace, CurChannel);
					if (i >= 0)
						Core.SwitchChannel(i);
				}
			}
		}
	}

	if (CmdLineOptions.m_fStandby)
		MainWindow.InitStandby();

	if (CmdLineOptions.m_fRecord)
		Core.CommandLineRecord(&CmdLineOptions);
	if (CmdLineOptions.m_fExitOnRecordEnd)
		Core.SetExitOnRecordingStop(true);

	if (Panel.fShowPanelWindow && Panel.Form.GetCurPageID() == PANEL_ID_CHANNEL)
		Panel.ChannelPanel.SetChannelList(ChannelManager.GetCurrentChannelList(), false);

	Accelerator.Initialize(UICore.GetMainWindow(), &MainMenu, m_Settings, &CommandManager);
	OperationOptions.Initialize(m_Settings, &CommandManager);

	m_Settings.Close();

	FavoritesManager.Load(m_FavoritesFileName.c_str());

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
		const HWND hwndForeground = ::GetForegroundWindow();
		if (hwndForeground != nullptr) {
			DWORD ProcessID = 0;
			::GetWindowThreadProcessId(hwndForeground, &ProcessID);
			if (ProcessID == ::GetCurrentProcessId())
				BroadcastControllerFocusMessage(nullptr, false, true);
		}
	}

	AppEventManager.OnStartupDone();

	// メッセージループ
	MSG msg;

	while (::GetMessage(&msg, nullptr, 0, 0) > 0) {
		if (HtmlHelpClass.PreTranslateMessage(&msg)
				|| UICore.ProcessDialogMessage(&msg))
			continue;
		if ((IsNoAcceleratorMessage(&msg)
					|| !Accelerator.TranslateMessage(MainWindow.GetHandle(), &msg))
				&& !ControllerManager.TranslateMessage(MainWindow.GetHandle(), &msg)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	return static_cast<int>(msg.wParam);
}


// アクセラレータにしないメッセージの判定
bool CAppMain::IsNoAcceleratorMessage(const MSG *pmsg)
{
	const HWND hwnd = ::GetFocus();

	if (hwnd != nullptr && ::IsWindowVisible(hwnd)) {
		if (MainWindow.IsNoAcceleratorMessage(pmsg))
			return true;

		const CDisplayView *pDisplayView = MainWindow.GetDisplayBase().GetDisplayView();
		if (pDisplayView != nullptr && hwnd == pDisplayView->GetHandle()) {
			return pDisplayView->IsMessageNeed(pmsg);
		} else if (pmsg->message == WM_KEYDOWN || pmsg->message == WM_KEYUP) {
			const LRESULT Result = ::SendMessage(hwnd, WM_GETDLGCODE, pmsg->wParam, reinterpret_cast<LPARAM>(pmsg));
			if ((Result & (DLGC_WANTALLKEYS | DLGC_WANTCHARS)) != 0)
				return true;
			if ((Result & DLGC_WANTARROWS) != 0) {
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
	return false;
}


// 設定ダイアログを表示する
bool CAppMain::ShowOptionDialog(HWND hwndOwner, int StartPage)
{
	if (!m_OptionDialog.Show(hwndOwner, StartPage))
		return false;

	if ((COptions::GetGeneralUpdateFlags() & COptions::UPDATE_GENERAL_BUILDMEDIAVIEWER) != 0) {
		const LibISDB::ViewerFilter *pViewer = CoreEngine.GetFilter<LibISDB::ViewerFilter>();
		if (pViewer != nullptr
				&& (pViewer->IsOpen()
					|| UICore.IsViewerInitializeError())) {
			const bool fOldError = UICore.IsViewerInitializeError();
			const bool fResult = UICore.InitializeViewer();
			// エラーで再生オフになっていた場合はオンにする
			if (fResult && fOldError && !UICore.IsViewerEnabled())
				UICore.EnableViewer(true);
		}
	}

	if ((COptions::GetGeneralUpdateFlags() & COptions::UPDATE_GENERAL_EVENTINFOFONT) != 0) {
		ApplyEventInfoFont();
	}

	if ((ProgramGuideOptions.GetUpdateFlags() & CProgramGuideOptions::UPDATE_EVENTICONS) != 0)
		Panel.ProgramListPanel.SetVisibleEventIcons(ProgramGuideOptions.GetVisibleEventIcons());
	if ((ProgramGuideOptions.GetUpdateFlags() & CProgramGuideOptions::UPDATE_ARIBSYMBOL) != 0) {
		Panel.ProgramListPanel.SetUseARIBSymbol(ProgramGuideOptions.GetUseARIBSymbol());
		Panel.ChannelPanel.SetUseARIBSymbol(ProgramGuideOptions.GetUseARIBSymbol());
	}

	TaskTrayManager.SetMinimizeToTray(ViewOptions.GetMinimizeToTray());

	SaveSettings(SaveSettingsFlag::Options);

	AppEventManager.OnSettingsChanged();

	return true;
}


CAppMain::CreateDirectoryResult CAppMain::CreateDirectory(
	HWND hwnd, LPCTSTR pszDirectory, LPCTSTR pszMessage)
{
	if (IsStringEmpty(pszDirectory))
		return CreateDirectoryResult::NoPath;

	TCHAR szPath[MAX_PATH];
	TCHAR szMessage[MAX_PATH + 80];

	if (!GetAbsolutePath(pszDirectory, szPath)) {
		StringFormat(
			szMessage,
			TEXT("フォルダ \"{}\" のパスが長過ぎます。"), szPath);
		::MessageBox(hwnd, szMessage, nullptr, MB_OK | MB_ICONEXCLAMATION);
		return CreateDirectoryResult::Error;
	}

	if (!::PathIsDirectory(szPath)) {
		StringVFormat(szMessage, pszMessage, szPath);
		if (::MessageBox(
					hwnd, szMessage, TEXT("フォルダ作成の確認"),
					MB_YESNO | MB_ICONINFORMATION) != IDYES)
			return CreateDirectoryResult::Cancelled;

		const int Result = ::SHCreateDirectoryEx(hwnd, szPath, nullptr);
		if (Result != ERROR_SUCCESS && Result != ERROR_ALREADY_EXISTS) {
			StringFormat(
				szMessage,
				TEXT("フォルダ \"{}\" を作成できません。(エラーコード {:#x})"), szPath, Result);
			AddLog(CLogItem::LogType::Error, szMessage);
			::MessageBox(hwnd, szMessage, nullptr, MB_OK | MB_ICONEXCLAMATION);
			return CreateDirectoryResult::Error;
		}

		AddLog(CLogItem::LogType::Information, TEXT("フォルダ \"{}\" を作成しました。"), szPath);
	}

	return CreateDirectoryResult::Success;
}


bool CAppMain::SendInterprocessMessage(HWND hwnd, UINT Message, const void *pParam, DWORD ParamSize)
{
	COPYDATASTRUCT cds;
	cds.dwData = Message;
	cds.cbData = ParamSize;
	cds.lpData = const_cast<void*>(pParam);

	DWORD_PTR Result = 0;
	return ::SendMessageTimeout(
		hwnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cds),
		SMTO_NORMAL | SMTO_NOTIMEOUTIFNOTHUNG, 10000, &Result) != 0
		&& Result != 0;
}


LRESULT CAppMain::ReceiveInterprocessMessage(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	::ReplyMessage(TRUE);

	const COPYDATASTRUCT *pcds = reinterpret_cast<const COPYDATASTRUCT*>(lParam);

	switch (pcds->dwData) {
	case PROCESS_MESSAGE_EXECUTE:
		if (pcds->cbData >= sizeof(TCHAR) && pcds->lpData != nullptr) {
			LPCTSTR pszCommandLine = static_cast<LPCTSTR>(pcds->lpData);
			if (pszCommandLine[pcds->cbData / sizeof(TCHAR) - 1] == _T('\0'))
				ProcessCommandLine(pszCommandLine);
		}
		break;

	default:
		AddLog(CLogItem::LogType::Warning, TEXT("未知のメッセージ {} を受信しました。"), pcds->dwData);
		break;
	}

	return TRUE;
}


bool CAppMain::BroadcastControllerFocusMessage(
	HWND hwnd, bool fSkipSelf, bool fFocus, DWORD ActiveThreadID)
{
	ControllerFocusInfo Info;

	Info.hwnd = hwnd;
	Info.fSkipSelf = fSkipSelf;
	Info.fFocus = fFocus;
	return EnumTVTestWindows(ControllerFocusCallback, reinterpret_cast<LPARAM>(&Info));
}


BOOL CALLBACK CAppMain::ControllerFocusCallback(HWND hwnd, LPARAM Param)
{
	ControllerFocusInfo *pInfo = reinterpret_cast<ControllerFocusInfo*>(Param);

	if (!pInfo->fSkipSelf || hwnd != pInfo->hwnd) {
		::PostMessage(hwnd, WM_APP_CONTROLLERFOCUS, pInfo->fFocus, 0);
		pInfo->fFocus = false;
	}
	return TRUE;
}


void CAppMain::ApplyEventInfoFont()
{
	Panel.ProgramListPanel.SetEventInfoFont(EpgOptions.GetEventInfoFont());
	Panel.ChannelPanel.SetEventInfoFont(EpgOptions.GetEventInfoFont());
	Epg.ProgramGuide.SetEventInfoFont(EpgOptions.GetEventInfoFont());
	CProgramInfoStatusItem *pProgramInfo = dynamic_cast<CProgramInfoStatusItem*>(StatusView.GetItemByID(STATUS_ITEM_PROGRAMINFO));
	if (pProgramInfo != nullptr)
		pProgramInfo->SetEventInfoFont(EpgOptions.GetEventInfoFont());
}


bool CAppMain::GetAbsolutePath(LPCTSTR pszPath, LPTSTR pszAbsolutePath) const
{
	if (pszAbsolutePath == nullptr)
		return false;

	pszAbsolutePath[0] = _T('\0');

	if (IsStringEmpty(pszPath))
		return false;

	if (::PathIsRelative(pszPath)) {
		TCHAR szDir[MAX_PATH], szTemp[MAX_PATH];
		GetAppDirectory(szDir);
		if (::PathCombine(szTemp, szDir, pszPath) == nullptr
				|| !::PathCanonicalize(pszAbsolutePath, szTemp))
			return false;
	} else {
		if (::lstrlen(pszPath) >= MAX_PATH)
			return false;
		StringCopy(pszAbsolutePath, pszPath);
	}

	return true;
}


bool CAppMain::ProcessCommandLine(LPCTSTR pszCmdLine)
{
	AddLog(TEXT("新しいコマンドラインオプションを受信しました。"));
	AddLog(TEXT("コマンドラインオプション : {}"), pszCmdLine);

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
			::ShowWindow(MainWindow.GetHandle(), SW_MINIMIZE);
	}

	if (CmdLine.m_fSilent || CmdLine.m_TvRockDID >= 0)
		Core.SetSilent(true);
	if (CmdLine.m_fSaveLog)
		CmdLineOptions.m_fSaveLog = true;

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
			CmdLineOptions.m_fRecordCurServiceOnly = true;
		Core.CommandLineRecord(&CmdLine);
	} else if (CmdLine.m_fRecordStop) {
		Core.StopRecord();
	}

	if (CmdLine.m_Volume >= 0)
		UICore.SetVolume(std::min(CmdLine.m_Volume, CCoreEngine::MAX_VOLUME), false);
	if (CmdLine.m_fMute)
		UICore.SetMute(true);

	if (CmdLine.m_fShowProgramGuide || CmdLine.m_fProgramGuideOnly)
		ShowProgramGuideByCommandLine(CmdLine);
	if (CmdLine.m_fHomeDisplay)
		UICore.DoCommandAsync(CM_HOMEDISPLAY);
	else if (CmdLine.m_fChannelDisplay)
		UICore.DoCommandAsync(CM_CHANNELDISPLAY);

	if (!CmdLine.m_Command.empty()) {
		const int Command = CommandManager.ParseIDText(CmdLine.m_Command);
		if (Command != 0) {
			UICore.DoCommand(Command);
		} else {
			AddLog(
				CLogItem::LogType::Error,
				TEXT("指定されたコマンド \"{}\" は無効です。"),
				CmdLine.m_Command);
		}
	}

	return true;
}


void CAppMain::ShowProgramGuideByCommandLine(const CCommandLineOptions &CmdLine)
{
	CMainWindow::ProgramGuideSpaceInfo SpaceInfo;

	if (!CmdLine.m_ProgramGuideTuner.empty())
		SpaceInfo.pszTuner = CmdLine.m_ProgramGuideTuner.c_str();
	else
		SpaceInfo.pszTuner = nullptr;
	if (!CmdLine.m_ProgramGuideSpace.empty())
		SpaceInfo.pszSpace = CmdLine.m_ProgramGuideSpace.c_str();
	else
		SpaceInfo.pszSpace = nullptr;

	MainWindow.ShowProgramGuide(
		true,
		UICore.GetFullscreen() ? CMainWindow::ShowProgramGuideFlag::None : CMainWindow::ShowProgramGuideFlag::Popup,
		&SpaceInfo);
}


HICON CAppMain::GetAppIcon()
{
	if (m_hicoApp == nullptr) {
		m_hicoApp = LoadIconStandardSize(
			::GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_ICON), IconSizeType::Normal);
	}
	return m_hicoApp;
}


HICON CAppMain::GetAppIconSmall()
{
	if (m_hicoAppSmall == nullptr) {
		m_hicoAppSmall = LoadIconStandardSize(
			::GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_ICON), IconSizeType::Small);
	}
	return m_hicoAppSmall;
}


CAppMain::CEngineEventListener::CEngineEventListener(CAppMain &App)
	: m_App(App)
{
}

void CAppMain::CEngineEventListener::OnServiceChanged(uint16_t ServiceID)
{
	m_App.MainWindow.PostMessage(WM_APP_SERVICECHANGED, ServiceID, 0);
	if (m_App.AudioManager.OnServiceUpdated())
		m_App.MainWindow.PostMessage(WM_APP_AUDIOLISTCHANGED, 0, 0);
	if (m_App.AudioOptions.GetResetAudioDelayOnChannelChange()) {
		LibISDB::ViewerFilter *pViewer = m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();
		if (pViewer != nullptr)
			pViewer->SetAudioDelay(0);
	}
}

void CAppMain::CEngineEventListener::OnPATUpdated(
	LibISDB::AnalyzerFilter *pAnalyzer, bool StreamChanged)
{
	OnServiceUpdated(pAnalyzer, true, StreamChanged);
	if (m_App.AudioManager.OnServiceUpdated())
		m_App.MainWindow.PostMessage(WM_APP_AUDIOLISTCHANGED, 0, 0);
}

void CAppMain::CEngineEventListener::OnPMTUpdated(LibISDB::AnalyzerFilter *pAnalyzer, uint16_t ServiceID)
{
	OnServiceInfoUpdated(pAnalyzer);
}

void CAppMain::CEngineEventListener::OnSDTUpdated(LibISDB::AnalyzerFilter *pAnalyzer)
{
	// サービスとロゴを関連付ける
	LibISDB::AnalyzerFilter::SDTServiceList ServiceList;
	if (pAnalyzer->GetSDTServiceList(&ServiceList)) {
		const uint16_t NetworkID = pAnalyzer->GetNetworkID();
		for (auto const &e : ServiceList) {
			if (e.LogoID != LibISDB::AnalyzerFilter::LOGO_ID_INVALID)
				m_App.LogoManager.AssociateLogoID(NetworkID, e.ServiceID, e.LogoID);
		}
	}

	OnServiceInfoUpdated(pAnalyzer);
}

void CAppMain::CEngineEventListener::OnWriteError(LibISDB::RecorderFilter *pRecorder)
{
	m_App.MainWindow.PostMessage(WM_APP_FILEWRITEERROR, 0, 0);
}

void CAppMain::CEngineEventListener::OnVideoStreamTypeChanged(uint8_t VideoStreamType)
{
	m_App.MainWindow.PostMessage(WM_APP_VIDEOSTREAMTYPECHANGED, VideoStreamType, 0);
}

void CAppMain::CEngineEventListener::OnVideoSizeChanged(LibISDB::ViewerFilter *pViewer)
{
	/*
		この通知が送られた段階ではまだレンダラの映像サイズは変わっていないため、
		後でパンスキャンの設定を行う必要がある
	*/
	m_App.MainWindow.PostMessage(WM_APP_VIDEOSIZECHANGED, 0, 0);
}

void CAppMain::CEngineEventListener::OnEventChanged(LibISDB::AnalyzerFilter *pAnalyzer, uint16_t EventID)
{
	TRACE(TEXT("CEngineEventListener::OnEventChanged() : event_id {:#04x}\n"), EventID);
	if (EventID != LibISDB::EVENT_ID_INVALID) {
		m_App.CoreEngine.SetAsyncStatusUpdatedFlag(CCoreEngine::StatusFlag::EventID);
		if (m_App.AudioManager.OnEventUpdated())
			m_App.MainWindow.PostMessage(WM_APP_AUDIOLISTCHANGED, 0, 0);
	}
}

void CAppMain::CEngineEventListener::OnEventUpdated(LibISDB::AnalyzerFilter *pAnalyzer)
{
	m_App.CoreEngine.SetAsyncStatusUpdatedFlag(CCoreEngine::StatusFlag::EventInfo);
	if (m_App.AudioManager.OnEventUpdated())
		m_App.MainWindow.PostMessage(WM_APP_AUDIOLISTCHANGED, 0, 0);
}

void CAppMain::CEngineEventListener::OnTOTUpdated(LibISDB::AnalyzerFilter *pAnalyzer)
{
	m_App.CoreEngine.SetAsyncStatusUpdatedFlag(CCoreEngine::StatusFlag::TOT);
}

void CAppMain::CEngineEventListener::OnFilterGraphInitialize(
	LibISDB::ViewerFilter *pViewer, IGraphBuilder *pGraphBuilder)
{
	m_App.PluginManager.SendFilterGraphInitializeEvent(pViewer, pGraphBuilder);
}

void CAppMain::CEngineEventListener::OnFilterGraphInitialized(
	LibISDB::ViewerFilter *pViewer, IGraphBuilder *pGraphBuilder)
{
	m_App.PluginManager.SendFilterGraphInitializedEvent(pViewer, pGraphBuilder);
}

void CAppMain::CEngineEventListener::OnFilterGraphFinalize(
	LibISDB::ViewerFilter *pViewer, IGraphBuilder *pGraphBuilder)
{
	m_App.PluginManager.SendFilterGraphFinalizeEvent(pViewer, pGraphBuilder);
}

void CAppMain::CEngineEventListener::OnFilterGraphFinalized(
	LibISDB::ViewerFilter *pViewer, IGraphBuilder *pGraphBuilder)
{
	m_App.PluginManager.SendFilterGraphFinalizedEvent(pViewer, pGraphBuilder);
}

void CAppMain::CEngineEventListener::OnSPDIFPassthroughError(LibISDB::ViewerFilter *pViewer, HRESULT hr)
{
	m_App.MainWindow.PostMessage(WM_APP_SPDIFPASSTHROUGHERROR, hr, 0);
}

void CAppMain::CEngineEventListener::OnServiceUpdated(
	LibISDB::AnalyzerFilter *pAnalyzer, bool fListUpdated, bool fStreamChanged)
{
	CServiceUpdateInfo *pInfo = new CServiceUpdateInfo(&m_App.CoreEngine, pAnalyzer);

	pInfo->m_fStreamChanged = fStreamChanged;
	m_App.MainWindow.PostMessage(WM_APP_SERVICEUPDATE, fListUpdated, reinterpret_cast<LPARAM>(pInfo));
}

void CAppMain::CEngineEventListener::OnServiceInfoUpdated(LibISDB::AnalyzerFilter *pAnalyzer)
{
	OnServiceUpdated(pAnalyzer, false, false);
	m_App.MainWindow.PostMessage(
		WM_APP_SERVICEINFOUPDATED, 0,
		MAKELPARAM(pAnalyzer->GetNetworkID(), pAnalyzer->GetTransportStreamID()));
}


CAppMain::CStreamInfoEventHandler::CStreamInfoEventHandler(CAppMain &App)
	: m_App(App)
{
}

void CAppMain::CStreamInfoEventHandler::OnRestoreSettings()
{
#if 0
	CSettings Settings;

	if (Settings.Open(m_App.Core.GetIniFileName(), CSettings::OPEN_READ)
			&& Settings.SetSection(TEXT("Settings"))) {
		int Left, Top, Width, Height;

		m_App.StreamInfo.GetPosition(&Left, &Top, &Width, &Height);
		Settings.Read(TEXT("StreamInfoLeft"), &Left);
		Settings.Read(TEXT("StreamInfoTop"), &Top);
		Settings.Read(TEXT("StreamInfoWidth"), &Width);
		Settings.Read(TEXT("StreamInfoHeight"), &Height);
		m_App.StreamInfo.SetPosition(Left, Top, Width, Height);
		//m_App.StreamInfo.MoveToMonitorInside();
	}
#endif
}

void CAppMain::CStreamInfoEventHandler::OnClose()
{
	m_App.UICore.SetCommandCheckedState(CM_STREAMINFO, false);
}


CAppMain::CCaptureWindowEventHandler::CCaptureWindowEventHandler(CAppMain &App)
	: m_App(App)
{
}

void CAppMain::CCaptureWindowEventHandler::OnRestoreSettings()
{
#if 0
	CSettings Settings;

	if (Settings.Open(m_App.Core.GetIniFileName(), CSettings::OPEN_READ)
			&& Settings.SetSection(TEXT("Settings"))) {
		int Left, Top, Width, Height;
		m_pCaptureWindow->GetPosition(&Left, &Top, &Width, &Height);
		Settings.Read(TEXT("CapturePreviewLeft"), &Left);
		Settings.Read(TEXT("CapturePreviewTop"), &Top);
		Settings.Read(TEXT("CapturePreviewWidth"), &Width);
		Settings.Read(TEXT("CapturePreviewHeight"), &Height);
		m_pCaptureWindow->SetPosition(Left, Top, Width, Height);
		m_pCaptureWindow->MoveToMonitorInside();

		bool fStatusBar;
		if (Settings.Read(TEXT("CaptureStatusBar"), &fStatusBar))
			m_pCaptureWindow->ShowStatusBar(fStatusBar);
	}
#endif
}

bool CAppMain::CCaptureWindowEventHandler::OnClose()
{
	m_App.UICore.SetCommandCheckedState(CM_CAPTUREPREVIEW, false);
	m_pCaptureWindow->ClearImage();
	return true;
}

bool CAppMain::CCaptureWindowEventHandler::OnSave(CCaptureImage *pImage)
{
	return m_App.CaptureOptions.SaveImage(pImage);
}

bool CAppMain::CCaptureWindowEventHandler::OnKeyDown(UINT KeyCode, UINT Flags)
{
	m_App.MainWindow.SendMessage(WM_KEYDOWN, KeyCode, Flags);
	return true;
}


}	// namespace TVTest
