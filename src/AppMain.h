#ifndef APP_MAIN_H
#define APP_MAIN_H


#include "AppCore.h"
#include "AppEvent.h"
#include "UICore.h"
#include "Graphics.h"
#include "Style.h"
#include "MainWindow.h"
#include "Menu.h"
#include "TaskTray.h"
#include "MainPanel.h"
#include "Accelerator.h"
#include "Controller.h"
#include "NetworkDefinition.h"
#include "TSProcessorManager.h"
#include "OptionDialog.h"
#include "GeneralOptions.h"
#include "ViewOptions.h"
#include "OSDOptions.h"
#include "StatusOptions.h"
#include "SideBarOptions.h"
#include "MenuOptions.h"
#include "PanelOptions.h"
#include "ColorScheme.h"
#include "OperationOptions.h"
#include "DriverOptions.h"
#include "AudioOptions.h"
#include "PlaybackOptions.h"
#include "RecordOptions.h"
#include "CaptureOptions.h"
#include "ChannelScan.h"
#include "EpgOptions.h"
#include "ProgramGuideOptions.h"
#include "TSProcessorOptions.h"
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
#ifndef _DEBUG
#include "DebugHelper.h"
#endif


class CTotTimeAdjuster
{
	bool m_fEnable;
	DWORD m_TimeOut;
	DWORD m_StartTime;
	SYSTEMTIME m_PrevTime;

public:
	CTotTimeAdjuster()
		: m_fEnable(false)
	{
	}
	bool BeginAdjust(DWORD TimeOut=10000UL);
	bool AdjustTime();
};

class CDisplayEventHandlerBase
{
protected:
	void RelayMouseMessage(CDisplayView *pView,UINT Message,int x,int y);
};

class CEpg
{
public:
	class CChannelProviderManager : public CProgramGuideChannelProviderManager
	{
	public:
		CChannelProviderManager();
		~CChannelProviderManager();
	// CProgramGuideChannelProviderManager
		size_t GetChannelProviderCount() const override;
		CProgramGuideChannelProvider *GetChannelProvider(size_t Index) const override;
	// CChannelProviderManager
		bool Create(LPCTSTR pszDefaultTuner=NULL);
		void Clear();
		int GetCurChannelProvider() const { return m_CurChannelProvider; }

	private:
		class CBonDriverChannelProvider : public CProgramGuideBaseChannelProvider
		{
		public:
			CBonDriverChannelProvider(LPCTSTR pszFileName);
			bool Update() override;
		};

		class CFavoritesChannelProvider : public CProgramGuideBaseChannelProvider
		{
		public:
			~CFavoritesChannelProvider();
			bool Update() override;
			bool GetName(LPTSTR pszName,int MaxName) const override;
			bool GetGroupID(size_t Group,TVTest::String *pID) const override;
			int ParseGroupID(LPCTSTR pszID) const override;
			bool GetBonDriver(LPTSTR pszFileName,int MaxLength) const override;
			bool GetBonDriverFileName(size_t Group,size_t Channel,LPTSTR pszFileName,int MaxLength) const override;

		private:
			struct GroupInfo
			{
				TVTest::String Name;
				TVTest::String ID;
				std::vector<TVTest::CFavoriteChannel> ChannelList;
			};

			std::vector<GroupInfo*> m_GroupList;

			void ClearGroupList();
			void AddFavoritesChannels(const TVTest::CFavoriteFolder &Folder,const TVTest::String &Path);
			void AddSubItems(GroupInfo *pGroup,const TVTest::CFavoriteFolder &Folder);
		};

		std::vector<CProgramGuideChannelProvider*> m_ChannelProviderList;
		int m_CurChannelProvider;
	};

	CProgramGuide ProgramGuide;
	CProgramGuideFrameSettings ProgramGuideFrameSettings;
	CProgramGuideFrame ProgramGuideFrame;
	CProgramGuideDisplay ProgramGuideDisplay;
	bool fShowProgramGuide;

	CEpg(CEpgProgramList &EpgProgramList,CEventSearchOptions &EventSearchOptions);
	CChannelProviderManager *CreateChannelProviderManager(LPCTSTR pszDefaultTuner=NULL);

private:
	class CProgramGuideEventHandler : public CProgramGuide::CEventHandler
	{
		bool OnClose() override;
		void OnDestroy() override;
		void OnServiceTitleLButtonDown(LPCTSTR pszDriverFileName,const CServiceInfoData *pServiceInfo) override;
		bool OnKeyDown(UINT KeyCode,UINT Flags) override;
		bool OnMenuInitialize(HMENU hmenu,UINT CommandBase) override;
		bool OnMenuSelected(UINT Command) override;

		int FindChannel(const CChannelList *pChannelList,const CServiceInfoData *pServiceInfo);
	};

	class CProgramGuideDisplayEventHandler
		: public CProgramGuideDisplay::CProgramGuideDisplayEventHandler
		, protected CDisplayEventHandlerBase
	{
	// CProgramGuideDisplay::CProgramGuideDisplayEventHandler
		bool OnHide() override;
		bool SetAlwaysOnTop(bool fTop) override;
		bool GetAlwaysOnTop() const override;
		void OnMouseMessage(UINT Msg,int x,int y) override;
	};

	class CProgramGuideProgramCustomizer : public CProgramGuide::CProgramCustomizer
	{
		bool Initialize() override;
		void Finalize() override;
		bool DrawBackground(const CEventInfoData &Event,HDC hdc,
			const RECT &ItemRect,const RECT &TitleRect,const RECT &ContentRect,
			COLORREF BackgroundColor) override;
		bool InitializeMenu(const CEventInfoData &Event,HMENU hmenu,UINT CommandBase,
							const POINT &CursorPos,const RECT &ItemRect) override;
		bool ProcessMenu(const CEventInfoData &Event,UINT Command) override;
		bool OnLButtonDoubleClick(const CEventInfoData &Event,
								  const POINT &CursorPos,const RECT &ItemRect) override;
	};

	CProgramGuideEventHandler m_ProgramGuideEventHandler;
	CProgramGuideDisplayEventHandler m_ProgramGuideDisplayEventHandler;
	CProgramGuideProgramCustomizer m_ProgramCustomizer;
	CChannelProviderManager m_ChannelProviderManager;
};

class CEpgLoadEventHandler
	: public CEpgOptions::CEpgFileLoadEventHandler
	, public CEpgOptions::CEDCBDataLoadEventHandler
{
// CEpgFileLoadEventHandler
	void OnBeginLoad() override;
	void OnEndLoad(bool fSuccess) override;

// CEDCBDataLoadEventHandler
	void OnStart() override;
	void OnEnd(bool fSuccess,CEventManager *pEventManager) override;
};

class CSideBarOptionsEventHandler : public CSideBarOptions::CEventHandler
{
// CSideBarOptions::CEventHandler
	void OnItemChanged() override;
};

class CHomeDisplayEventHandler
	: public CHomeDisplay::CHomeDisplayEventHandler
	, protected CDisplayEventHandlerBase
{
	void OnClose() override;
	void OnMouseMessage(UINT Msg,int x,int y) override;
};

class CChannelDisplayEventHandler
	: public CChannelDisplay::CChannelDisplayEventHandler
	, protected CDisplayEventHandlerBase
{
	void OnTunerSelect(LPCTSTR pszDriverFileName,int TuningSpace) override;
	void OnChannelSelect(LPCTSTR pszDriverFileName,const CChannelInfo *pChannelInfo) override;
	void OnClose() override;
	void OnMouseMessage(UINT Msg,int x,int y) override;
};

class CServiceUpdateInfo
{
public:
	struct ServiceInfo {
		WORD ServiceID;
		TCHAR szServiceName[256];
		WORD LogoID;
	};
	ServiceInfo *m_pServiceList;
	int m_NumServices;
	int m_CurService;
	WORD m_NetworkID;
	WORD m_TransportStreamID;
	bool m_fStreamChanged;
	bool m_fServiceListEmpty;
	CServiceUpdateInfo(CDtvEngine *pEngine,CTsAnalyzer *pTsAnalyzer);
	~CServiceUpdateInfo();
};


class CAppMain
{
public:
	static const UINT WM_INTERPROCESS=WM_COPYDATA;
	static const UINT PROCESS_MESSAGE_EXECUTE=0x54565400;

#ifndef _DEBUG
	static CDebugHelper DebugHelper;
#endif

	CAppCore Core;
	TVTest::CAppEventManager AppEventManager;
	CCoreEngine CoreEngine;
	CUICore UICore;
	CLogger Logger;
	TVTest::Graphics::CGraphicsCore GraphicsCore;
	TVTest::Style::CStyleManager StyleManager;
	CMainMenu MainMenu;
	CCommandList CommandList;
	CCommandLineOptions CmdLineOptions;
	CPluginManager PluginManager;
	CEpgProgramList EpgProgramList;
	CMainWindow MainWindow;
	CStatusView StatusView;
	CSideBar SideBar;
	CMainPanel Panel;
	CHtmlHelp HtmlHelpClass;
	CChannelMenu ChannelMenu;
	CIconMenu AspectRatioIconMenu;
	CTaskbarManager TaskbarManager;
	TVTest::CTaskTrayManager TaskTrayManager;
	CEventSearchOptions EventSearchOptions;
	CHomeDisplay HomeDisplay;
	CChannelDisplay ChannelDisplay;
	CBalloonTip NotifyBalloonTip;
	CTotTimeAdjuster TotTimeAdjuster;

	TVTest::CNetworkDefinition NetworkDefinition;
	CChannelManager ChannelManager;

	CDriverManager DriverManager;
	TVTest::CTSProcessorManager TSProcessorManager;
	TVTest::CAudioManager AudioManager;
	CLogoManager LogoManager;
	TVTest::CEpgCaptureManager EpgCaptureManager;

	CEpg Epg;

	CStreamInfo StreamInfo;
	CCaptureWindow CaptureWindow;

	CZoomOptions ZoomOptions;
	CPanAndScanOptions PanAndScanOptions;
	CGeneralOptions GeneralOptions;
	CViewOptions ViewOptions;
	COSDOptions OSDOptions;
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
	CAudioOptions AudioOptions;
	CPlaybackOptions PlaybackOptions;
	CRecordOptions RecordOptions;
	CRecordManager RecordManager;
	CCaptureOptions CaptureOptions;
	CChannelScan ChannelScan;
	CEpgOptions EpgOptions;
	CProgramGuideOptions ProgramGuideOptions;
	TVTest::CTSProcessorOptions TSProcessorOptions;
	CPluginOptions PluginOptions;
	CRecentChannelList RecentChannelList;
	CChannelHistory ChannelHistory;
	TVTest::CFavoritesManager FavoritesManager;
	TVTest::CFavoritesMenu FavoritesMenu;
	TVTest::CKeywordSearch KeywordSearch;
	CFeaturedEvents FeaturedEvents;

	struct {
		int Space;
		int Channel;
		int ServiceID;
		int TransportStreamID;
		bool fAllChannels;
	} RestoreChannelInfo;

	CEpgLoadEventHandler EpgLoadEventHandler;

	CSideBarOptionsEventHandler SideBarOptionsEventHandler;

	CHomeDisplayEventHandler HomeDisplayEventHandler;
	CChannelDisplayEventHandler ChannelDisplayEventHandler;

	CAppMain();
	~CAppMain();
	HINSTANCE GetInstance() const;
	HINSTANCE GetResourceInstance() const;
	bool GetAppDirectory(LPTSTR pszDirectory) const;
	LPCTSTR GetIniFileName() const { return m_szIniFileName; }
	LPCTSTR GetFavoritesFileName() const { return m_szFavoritesFileName; }
	void AddLog(CLogItem::LogType Type,LPCTSTR pszText, ...);
	void AddLog(LPCTSTR pszText, ...);
	bool IsFirstExecute() const;
	int Main(HINSTANCE hInstance,LPCTSTR pszCmdLine,int nCmdShow);
	void Initialize();
	void Finalize();
	void Exit();
	bool LoadSettings();
	enum {
		SETTINGS_SAVE_STATUS  = 0x0001U,
		SETTINGS_SAVE_OPTIONS = 0x0002U,
		SETTINGS_SAVE_ALL     = SETTINGS_SAVE_STATUS | SETTINGS_SAVE_OPTIONS
	};
	bool SaveSettings(unsigned int Flags);
	bool ShowOptionDialog(HWND hwndOwner,int StartPage=-1);
	bool SendInterprocessMessage(HWND hwnd,UINT Message,const void *pParam,DWORD ParamSize);
	LRESULT ReceiveInterprocessMessage(HWND hwnd,WPARAM wParam,LPARAM lParam);
	bool BroadcastControllerFocusMessage(HWND hwnd,bool fSkipSelf,bool fFocus,
										 DWORD ActiveThreadID=0);
	void SetEnablePlaybackOnStart(bool fEnable) { m_fEnablePlaybackOnStart=fEnable; }
	bool GetEnablePlaybackOnStart() const { return m_fEnablePlaybackOnStart; }

	static HICON GetAppIcon();
	static HICON GetAppIconSmall();

private:
	class CDtvEngineEventHandler : public CDtvEngine::CEventHandler
	{
	public:
		CDtvEngineEventHandler(CAppMain &App);

	private:
		CAppMain &m_App;

	// CDtvEngine::CEventHandler
		void OnServiceListUpdated(CTsAnalyzer *pTsAnalyzer,bool bStreamChanged) override;
		void OnServiceInfoUpdated(CTsAnalyzer *pTsAnalyzer) override;
		void OnServiceChanged(WORD ServiceID) override;
		void OnFileWriteError(CTsRecorder *pTsRecorder) override;
		void OnVideoStreamTypeChanged(BYTE VideoStreamType) override;
		void OnVideoSizeChanged(CMediaViewer *pMediaViewer) override;
		void OnEventChanged(CTsAnalyzer *pTsAnalyzer,WORD EventID) override;
		void OnEventUpdated(CTsAnalyzer *pTsAnalyzer) override;
		void OnTotUpdated(CTsAnalyzer *pTsAnalyzer) override;
		void OnFilterGraphInitialize(CMediaViewer *pMediaViewer,IGraphBuilder *pGraphBuilder) override;
		void OnFilterGraphInitialized(CMediaViewer *pMediaViewer,IGraphBuilder *pGraphBuilder) override;
		void OnFilterGraphFinalize(CMediaViewer *pMediaViewer,IGraphBuilder *pGraphBuilder) override;
		void OnFilterGraphFinalized(CMediaViewer *pMediaViewer,IGraphBuilder *pGraphBuilder) override;
		void OnSpdifPassthroughError(HRESULT hr) override;
	// CDtvEngineEventHandler
		void OnServiceUpdated(CTsAnalyzer *pTsAnalyzer,bool fListUpdated,bool fStreamChanged);
	};

	class CStreamInfoEventHandler : public CStreamInfo::CEventHandler
	{
	public:
		CStreamInfoEventHandler(CAppMain &App);

	private:
		CAppMain &m_App;

		void OnRestoreSettings() override;
		bool OnClose() override;
	};

	class CCaptureWindowEventHandler : public CCaptureWindow::CEventHandler
	{
	public:
		CCaptureWindowEventHandler(CAppMain &App);

	private:
		CAppMain &m_App;

		void OnRestoreSettings() override;
		bool OnClose() override;
		bool OnSave(CCaptureImage *pImage) override;
		bool OnKeyDown(UINT KeyCode,UINT Flags) override;
	};

	struct ControllerFocusInfo {
		HWND hwnd;
		bool fSkipSelf;
		bool fFocus;
	};

	HINSTANCE m_hInst;
	TCHAR m_szIniFileName[MAX_PATH];
	TCHAR m_szFavoritesFileName[MAX_PATH];
	CSettings m_Settings;
	bool m_fFirstExecute;
	CDtvEngineEventHandler m_DtvEngineHandler;
	CStreamInfoEventHandler m_StreamInfoEventHandler;
	CCaptureWindowEventHandler m_CaptureWindowEventHandler;
	COptionDialog m_OptionDialog;
	unsigned int m_ExitTimeout;
	bool m_fEnablePlaybackOnStart;
	bool m_fIncrementNetworkPort;

	static HICON m_hicoApp;
	static HICON m_hicoAppSmall;

	// ÉRÉsÅ[ã÷é~
	CAppMain(const CAppMain &) /* = delete */;
	CAppMain &operator=(const CAppMain &) /* = delete */;

	void RegisterCommands();
	bool IsNoAcceleratorMessage(const MSG *pmsg);
	void ApplyEventInfoFont();
	bool GetAbsolutePath(LPCTSTR pszPath,LPTSTR pszAbsolutePath) const;
	bool ProcessCommandLine(LPCTSTR pszCmdLine);
	void ShowProgramGuideByCommandLine(const CCommandLineOptions &CmdLine);
	static BOOL CALLBACK ControllerFocusCallback(HWND hwnd,LPARAM Param);
};


CAppMain &GetAppClass();


#endif
