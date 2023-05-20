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


#ifndef TVTEST_APP_MAIN_H
#define TVTEST_APP_MAIN_H


#include "AppCore.h"
#include "AppEvent.h"
#include "AppCommand.h"
#include "UICore.h"
#include "Graphics.h"
#include "Style.h"
#include "MainWindow.h"
#include "Menu.h"
#include "TaskTray.h"
#include "MainPanel.h"
#include "Epg.h"
#include "Accelerator.h"
#include "Controller.h"
#include "NetworkDefinition.h"
#include "TSProcessorManager.h"
#include "OptionDialog.h"
#include "GeneralOptions.h"
#include "ViewOptions.h"
#include "OSDOptions.h"
#include "NotificationBarOptions.h"
#include "StatusOptions.h"
#include "SideBarOptions.h"
#include "MenuOptions.h"
#include "PanelOptions.h"
#include "ColorSchemeOptions.h"
#include "OperationOptions.h"
#include "DriverOptions.h"
#include "VideoOptions.h"
#include "AudioOptions.h"
#include "PlaybackOptions.h"
#include "RecordOptions.h"
#include "CaptureOptions.h"
#include "ChannelScan.h"
#include "EpgOptions.h"
#include "ProgramGuideOptions.h"
#include "TSProcessorOptions.h"
#include "TaskbarOptions.h"
#include "VideoDecoderOptions.h"
#include "Plugin.h"
#include "Logger.h"
#include "CommandLine.h"
#include "ChannelHistory.h"
#include "Favorites.h"
#include "FeaturedEvents.h"
#include "Help.h"
#include "StreamInfo.h"
#include "HomeDisplay.h"
#include "ChannelDisplay.h"
#include "Taskbar.h"
#include "ZoomOptions.h"
#include "PanAndScanOptions.h"
#include "AudioManager.h"
#include "LogoManager.h"
#include "EpgCapture.h"
#include "KeywordSearch.h"
#include "VariableManager.h"
#ifndef _DEBUG
#include "DebugHelper.h"
#endif


namespace TVTest
{

	class CTotTimeAdjuster
	{
		bool m_fEnable = false;
		DWORD m_TimeOut;
		DWORD m_StartTime;
		SYSTEMTIME m_PrevTime;

	public:
		bool BeginAdjust(DWORD TimeOut = 10000UL);
		bool AdjustTime();
	};

	class CEpgLoadEventHandler
		: public CEpgOptions::CEpgFileLoadEventHandler
	{
		void OnBeginEpgDataLoading() override;
		void OnEndEpgDataLoading(bool fSuccess) override;
		void OnBeginEdcbDataLoading() override;
		void OnEndEdcbDataLoading(bool fSuccess, LibISDB::EPGDatabase *pEPGDatabase) override;
	};

	class CServiceUpdateInfo
	{
	public:
		LibISDB::AnalyzerFilter::ServiceList m_ServiceList;
		int m_CurService;
		WORD m_NetworkID;
		WORD m_TransportStreamID;
		bool m_fStreamChanged;
		bool m_fServiceListEmpty;

		CServiceUpdateInfo(LibISDB::TSEngine *pEngine, LibISDB::AnalyzerFilter *pAnalyzer);
	};


	class CAppMain
	{
	public:
		static constexpr UINT WM_INTERPROCESS = WM_COPYDATA;
		static constexpr UINT PROCESS_MESSAGE_EXECUTE = 0x54565400;

#ifndef _DEBUG
		static CDebugHelper DebugHelper;
#endif

		CAppCore Core{*this};
		CAppEventManager AppEventManager;
		CCoreEngine CoreEngine;
		CUICore UICore{*this};
		CLogger Logger;
		Graphics::CGraphicsCore GraphicsCore;
		Style::CStyleManager StyleManager;
		CDirectWriteSystem DirectWriteSystem;
		CMainMenu MainMenu;
		CCommandManager CommandManager;
		CAppCommand AppCommand{*this};
		CCommandLineOptions CmdLineOptions;
		CPluginManager PluginManager;
		LibISDB::EPGDatabase EPGDatabase;
		CMainWindow MainWindow{*this};
		CStatusView StatusView;
		CSideBar SideBar;
		CMainPanel Panel;
		CHtmlHelp HtmlHelpClass;
		CChannelMenu ChannelMenu;
		CIconMenu AspectRatioIconMenu;
		CTaskbarManager TaskbarManager;
		CTaskTrayManager TaskTrayManager;
		CEventSearchOptions EventSearchOptions;
		CHomeDisplay HomeDisplay;
		CChannelDisplay ChannelDisplay;
		CBalloonTip NotifyBalloonTip;
		CTotTimeAdjuster TotTimeAdjuster;

		CNetworkDefinition NetworkDefinition;
		CChannelManager ChannelManager;

		CDriverManager DriverManager;
		CTSProcessorManager TSProcessorManager;
		CAudioManager AudioManager;
		CLogoManager LogoManager;
		CEpgCaptureManager EpgCaptureManager;

		CEpg Epg;

		CStreamInfo StreamInfo;
		CCaptureWindow CaptureWindow;

		CZoomOptions ZoomOptions;
		CPanAndScanOptions PanAndScanOptions;
		CGeneralOptions GeneralOptions;
		CViewOptions ViewOptions;
		COSDOptions OSDOptions;
		CNotificationBarOptions NotificationBarOptions;
		COSDManager OSDManager;
		CStatusOptions StatusOptions;
		CSideBarOptions SideBarOptions;
		CMenuOptions MenuOptions;
		CPanelOptions PanelOptions;
		CColorSchemeOptions ColorSchemeOptions;
		COperationOptions OperationOptions;
		CAccelerator Accelerator;
		CControllerManager ControllerManager;
		CDriverOptions DriverOptions;
		CVideoOptions VideoOptions;
		CAudioOptions AudioOptions;
		CPlaybackOptions PlaybackOptions;
		CRecordOptions RecordOptions;
		CRecordManager RecordManager;
		CCaptureOptions CaptureOptions;
		CChannelScan ChannelScan;
		CEpgOptions EpgOptions;
		CProgramGuideOptions ProgramGuideOptions;
		CTSProcessorOptions TSProcessorOptions;
		CPluginOptions PluginOptions;
		CTaskbarOptions TaskbarOptions;
		CVideoDecoderOptions VideoDecoderOptions;
		CRecentChannelList RecentChannelList;
		CChannelHistory ChannelHistory;
		CFavoritesManager FavoritesManager;
		CFavoritesMenu FavoritesMenu;
		CKeywordSearch KeywordSearch;
		CFeaturedEvents FeaturedEvents;
		CVariableManager VariableManager;

		struct {
			int Space;
			int Channel;
			int ServiceID;
			int TransportStreamID;
			bool fAllChannels;
		} RestoreChannelInfo;

		CEpgLoadEventHandler EpgLoadEventHandler;

		CAppMain();
		~CAppMain();

		CAppMain(const CAppMain &) = delete;
		CAppMain &operator=(const CAppMain &) = delete;

		HINSTANCE GetInstance() const;
		HINSTANCE GetResourceInstance() const;
		bool GetAppFilePath(String *pPath) const;
		bool GetAppDirectory(String *pDirectory) const;
		bool GetAppDirectory(LPTSTR pszDirectory) const;
		LPCTSTR GetIniFileName() const { return m_IniFileName.c_str(); }
		LPCTSTR GetFavoritesFileName() const { return m_FavoritesFileName.c_str(); }

		template<typename... TArgs> void AddLog(CLogItem::LogType Type, StringView Format, const TArgs&... Args)
		{
			Logger.AddLog(Type, Format, Args...);
		}
		template<typename... TArgs> void AddLog(StringView Format, const TArgs&... Args)
		{
			Logger.AddLog(CLogItem::LogType::Information, Format, Args...);
		}
		void AddLogV(CLogItem::LogType Type, StringView Format, FormatArgs Args)
		{
			Logger.AddLogV(Type, Format, Args);
		}
		void AddLogV(StringView Format, FormatArgs Args)
		{
			Logger.AddLogV(CLogItem::LogType::Information, Format, Args);
		}
		void AddLogRaw(CLogItem::LogType Type, StringView Text)
		{
			Logger.AddLogRaw(Type, Text);
		}
		void AddLogRaw(StringView Text)
		{
			Logger.AddLogRaw(CLogItem::LogType::Information, Text);
		}

		bool IsFirstExecute() const;
		int Main(HINSTANCE hInstance, LPCTSTR pszCmdLine, int nCmdShow);
		void Initialize();
		void Finalize();
		void Exit();
		bool LoadSettings();
		enum class SaveSettingsFlag : unsigned int {
			None    = 0x0000U,
			Status  = 0x0001U,
			Options = 0x0002U,
			TVTEST_ENUM_FLAGS_TRAILER,
			All     = Status | Options,
		};
		bool SaveSettings(SaveSettingsFlag Flags);
		bool ShowOptionDialog(HWND hwndOwner, int StartPage = -1);
		enum class CreateDirectoryResult {
			Success,
			Error,
			Cancelled,
			NoPath,
		};
		CreateDirectoryResult CreateDirectory(HWND hwnd, LPCTSTR pszDirectory, LPCTSTR pszMessage);
		bool SendInterprocessMessage(HWND hwnd, UINT Message, const void *pParam, DWORD ParamSize);
		LRESULT ReceiveInterprocessMessage(HWND hwnd, WPARAM wParam, LPARAM lParam);
		bool BroadcastControllerFocusMessage(
			HWND hwnd, bool fSkipSelf, bool fFocus, DWORD ActiveThreadID = 0);
		void SetEnablePlaybackOnStart(bool fEnable) { m_fEnablePlaybackOnStart = fEnable; }
		bool GetEnablePlaybackOnStart() const { return m_fEnablePlaybackOnStart; }

		static HICON GetAppIcon();
		static HICON GetAppIconSmall();

	private:
		class CEngineEventListener
			: public CCoreEngine::EventListener
		{
		public:
			CEngineEventListener(CAppMain &App);

		private:
			CAppMain &m_App;

			// CCoreEngine::EventListener
			void OnServiceChanged(uint16_t ServiceID) override;
			void OnPATUpdated(LibISDB::AnalyzerFilter *pAnalyzer, bool StreamChanged) override;
			void OnPMTUpdated(LibISDB::AnalyzerFilter *pAnalyzer, uint16_t ServiceID) override;
			void OnSDTUpdated(LibISDB::AnalyzerFilter *pAnalyzer) override;
			void OnWriteError(LibISDB::RecorderFilter *pRecorder) override;
			void OnVideoStreamTypeChanged(uint8_t VideoStreamType) override;
			void OnVideoSizeChanged(LibISDB::ViewerFilter *pViewer) override;
			void OnEventChanged(LibISDB::AnalyzerFilter *pAnalyzer, uint16_t EventID) override;
			void OnEventUpdated(LibISDB::AnalyzerFilter *pAnalyzer) override;
			void OnTOTUpdated(LibISDB::AnalyzerFilter *pAnalyzer) override;
			void OnFilterGraphInitialize(LibISDB::ViewerFilter *pViewer, IGraphBuilder *pGraphBuilder) override;
			void OnFilterGraphInitialized(LibISDB::ViewerFilter *pViewer, IGraphBuilder *pGraphBuilder) override;
			void OnFilterGraphFinalize(LibISDB::ViewerFilter *pViewer, IGraphBuilder *pGraphBuilder) override;
			void OnFilterGraphFinalized(LibISDB::ViewerFilter *pViewer, IGraphBuilder *pGraphBuilder) override;
			void OnSPDIFPassthroughError(LibISDB::ViewerFilter *pViewer, HRESULT hr) override;

			// CEngineEventListener
			void OnServiceUpdated(LibISDB::AnalyzerFilter *pAnalyzer, bool fListUpdated, bool fStreamChanged);
			void OnServiceInfoUpdated(LibISDB::AnalyzerFilter *pAnalyzer);
		};

		class CStreamInfoEventHandler
			: public CStreamInfo::CEventHandler
		{
		public:
			CStreamInfoEventHandler(CAppMain &App);

		private:
			CAppMain &m_App;

			void OnRestoreSettings() override;
			void OnClose() override;
		};

		class CCaptureWindowEventHandler
			: public CCaptureWindow::CEventHandler
		{
		public:
			CCaptureWindowEventHandler(CAppMain &App);

		private:
			CAppMain &m_App;

			void OnRestoreSettings() override;
			bool OnClose() override;
			bool OnSave(CCaptureImage *pImage) override;
			bool OnKeyDown(UINT KeyCode, UINT Flags) override;
		};

		struct ControllerFocusInfo {
			HWND hwnd;
			bool fSkipSelf;
			bool fFocus;
		};

		HINSTANCE m_hInst = nullptr;
		CFilePath m_IniFileName;
		CFilePath m_FavoritesFileName;
		CSettings m_Settings;
		bool m_fFirstExecute = false;
		bool m_fInitialSettings = false;
		CEngineEventListener m_EngineEventListener{*this};
		CStreamInfoEventHandler m_StreamInfoEventHandler{*this};
		CCaptureWindowEventHandler m_CaptureWindowEventHandler{*this};
		COptionDialog m_OptionDialog;
		unsigned int m_ExitTimeout = 60000;
		bool m_fEnablePlaybackOnStart = true;
		bool m_fIncrementNetworkPort = true;

		static HICON m_hicoApp;
		static HICON m_hicoAppSmall;

		bool IsNoAcceleratorMessage(const MSG *pmsg);
		void ApplyEventInfoFont();
		bool GetAbsolutePath(LPCTSTR pszPath, LPTSTR pszAbsolutePath) const;
		bool ProcessCommandLine(LPCTSTR pszCmdLine);
		void ShowProgramGuideByCommandLine(const CCommandLineOptions &CmdLine);
		static BOOL CALLBACK ControllerFocusCallback(HWND hwnd, LPARAM Param);
	};


	CAppMain &GetAppClass();

}	// namespace TVTest


#endif
