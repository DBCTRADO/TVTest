#ifndef PLUGIN_H
#define PLUGIN_H


#include <vector>
#include "TVTestPlugin.h"
#include "AppEvent.h"
#include "Options.h"
#include "MediaData.h"
#include "BonTsEngine/Exception.h"
#include "BonTsEngine/TsUtilClass.h"


class CPlugin
	: public CBonErrorHandler
	, public CMediaGrabber::IGrabber
{
public:
	class CPluginCommandInfo
	{
		int m_ID;
		int m_Command;
		unsigned int m_Flags;
		unsigned int m_State;
		TVTest::String m_Text;
		TVTest::String m_Name;
		TVTest::Theme::ThemeBitmap m_Icon;

	public:
		CPluginCommandInfo(int ID,LPCWSTR pszText,LPCWSTR pszName);
		CPluginCommandInfo(const TVTest::PluginCommandInfo &Info);
		CPluginCommandInfo(const TVTest::CommandInfo &Info);
		virtual ~CPluginCommandInfo();
		int GetID() const { return m_ID; }
		int GetCommand() const { return m_Command; }
		void SetCommand(int Command) { m_Command=Command; }
		unsigned int GetFlags() const { return m_Flags; }
		unsigned int GetState() const { return m_State; }
		void SetState(unsigned int State) { m_State=State; }
		LPCWSTR GetText() const { return m_Text.c_str(); }
		LPCWSTR GetName() const { return m_Name.c_str(); }
		TVTest::Theme::ThemeBitmap &GetIcon() { return m_Icon; }
	};

	CPlugin();
	~CPlugin();
	bool Load(LPCTSTR pszFileName);
	void Free();
	bool IsLoaded() const { return m_hLib!=NULL; }
	bool IsEnabled() const { return m_fEnabled; }
	bool Enable(bool fEnable);
	HMODULE GetModuleHandle() { return m_hLib; }
	LPCTSTR GetFileName() const { return m_FileName.Get(); }
	LPCTSTR GetPluginName() const { return m_PluginName.GetSafe(); }
	LPCTSTR GetCopyright() const { return m_Copyright.GetSafe(); }
	LPCTSTR GetDescription() const { return m_Description.GetSafe(); }
	LRESULT SendEvent(UINT Event,LPARAM lParam1=0,LPARAM lParam2=0);
	bool OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,LRESULT *pResult);
	bool Settings(HWND hwndOwner);
	bool HasSettings() const { return (m_Flags&TVTest::PLUGIN_FLAG_HASSETTINGS)!=0; }
	bool CanUnload() const { return (m_Flags&TVTest::PLUGIN_FLAG_NOUNLOAD)==0; }
	int GetCommand() const { return m_Command; }
	bool SetCommand(int Command);
	int NumPluginCommands() const;
	int ParsePluginCommand(LPCWSTR pszCommand) const;
	CPluginCommandInfo *GetPluginCommandInfo(int Index);
	const CPluginCommandInfo *GetPluginCommandInfo(int Index) const;
	CPluginCommandInfo *GetPluginCommandInfo(LPCWSTR pszCommand);
	const CPluginCommandInfo *GetPluginCommandInfo(LPCWSTR pszCommand) const;
	bool GetPluginCommandInfo(int Index,TVTest::CommandInfo *pInfo) const;
	bool NotifyCommand(LPCWSTR pszCommand);
	bool DrawPluginCommandIcon(const TVTest::DrawCommandIconInfo *pInfo);
	int NumProgramGuideCommands() const;
	bool GetProgramGuideCommandInfo(int Index,TVTest::ProgramGuideCommandInfo *pInfo) const;
	bool NotifyProgramGuideCommand(LPCTSTR pszCommand,UINT Action,const CEventInfoData *pEvent,
								   const POINT *pCursorPos,const RECT *pItemRect);
	bool IsDisableOnStart() const;
	bool IsProgramGuideEventEnabled(UINT EventFlag) const { return (m_ProgramGuideEventFlags&EventFlag)!=0; }
	void RegisterStatusItems();
	void SendStatusItemCreatedEvent();
	void SendStatusItemUpdateTimerEvent();

	static void SetMessageWindow(HWND hwnd,UINT Message);
	static LRESULT OnPluginMessage(WPARAM wParam,LPARAM lParam);

private:
	class CProgramGuideCommand : public CPluginCommandInfo
	{
		UINT m_Type;

	public:
		CProgramGuideCommand(const TVTest::ProgramGuideCommandInfo &Info);
		UINT GetType() const { return m_Type; }
	};

	class CPluginStatusItem;

	struct StatusItem
	{
		DWORD Flags;
		int ID;
		TVTest::String IDText;
		TVTest::String Name;
		int MinWidth;
		int MaxWidth;
		int DefaultWidth;
		int MinHeight;
		int ItemID;
		DWORD Style;
		DWORD State;
		CPluginStatusItem *pItem;
	};

	class CPluginStatusItem : public CStatusItem
	{
	public:
		CPluginStatusItem(CPlugin *pPlugin,StatusItem *pItem);
		~CPluginStatusItem();

	// CStatusItem
		LPCTSTR GetIDText() const override { return m_IDText.c_str(); }
		LPCTSTR GetName() const override { return m_pItem->Name.c_str(); }
		void Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags) override;
		void OnLButtonDown(int x,int y) override;
		void OnLButtonUp(int x,int y) override;
		void OnLButtonDoubleClick(int x,int y) override;
		void OnRButtonDown(int x,int y) override;
		void OnRButtonUp(int x,int y) override;
		void OnMouseMove(int x,int y) override;
		void OnVisibilityChanged() override;
		void OnFocus(bool fFocus) override;
		void OnSizeChanged() override;

	// CPluginStatusItem
		void DetachItem();
		HWND GetWindowHandle() const;
		void ApplyStyle();

	private:
		CPlugin *m_pPlugin;
		StatusItem *m_pItem;
		TVTest::String m_IDText;

		void NotifyDraw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags);
		void NotifyMouseEvent(UINT Action,int x,int y);
	};

	HMODULE m_hLib;
	CDynamicString m_FileName;
	TVTest::PluginParam m_PluginParam;
	DWORD m_Version;
	DWORD m_Type;
	DWORD m_Flags;
	CDynamicString m_PluginName;
	CDynamicString m_Copyright;
	CDynamicString m_Description;
	bool m_fEnabled;
	bool m_fSetting;
	int m_Command;
	TVTest::EventCallbackFunc m_pEventCallback;
	void *m_pEventCallbackClientData;
	UINT m_ProgramGuideEventFlags;
	TVTest::WindowMessageCallbackFunc m_pMessageCallback;
	void *m_pMessageCallbackClientData;
	std::vector<TVTest::StreamCallbackInfo> m_StreamCallbackList;
	CCriticalLock m_GrabberLock;
	std::vector<CPluginCommandInfo> m_CommandList;
	std::vector<CProgramGuideCommand> m_ProgramGuideCommandList;
	std::vector<CDynamicString> m_ControllerList;
	std::vector<StatusItem*> m_StatusItemList;

	static HWND m_hwndMessage;
	static UINT m_MessageCode;

	class CAudioStreamCallbackInfo {
	public:
		CPlugin *m_pPlugin;
		TVTest::AudioCallbackFunc m_pCallback;
		void *m_pClientData;
		CAudioStreamCallbackInfo(CPlugin *pPlugin,TVTest::AudioCallbackFunc pCallback,void *pClientData)
			: m_pPlugin(pPlugin)
			, m_pCallback(pCallback)
			, m_pClientData(pClientData)
		{
		}
	};
	static CCriticalLock m_AudioStreamLock;
	static std::vector<CAudioStreamCallbackInfo> m_AudioStreamCallbackList;

	static void CALLBACK AudioStreamCallback(short *pData,DWORD Samples,int Channels,void *pParam);
	static LRESULT CALLBACK Callback(TVTest::PluginParam *pParam,UINT Message,LPARAM lParam1,LPARAM lParam2);

// CMediaGrabber::IGrabber
	bool OnInputMedia(CMediaData *pMediaData) override;

// CPlugin
	LRESULT SendPluginMessage(TVTest::PluginParam *pParam,UINT Message,LPARAM lParam1,LPARAM lParam2,
							  LRESULT FailedResult=0);
	LRESULT OnCallback(TVTest::PluginParam *pParam,UINT Message,LPARAM lParam1,LPARAM lParam2);
	bool OnGetSetting(TVTest::SettingInfo *pSetting) const;
};

class CPluginManager : public TVTest::CAppEventHandler
{
	struct MenuCommandInfo {
		CPlugin *pPlugin;
		UINT CommandFirst;
		UINT CommandEnd;
	};

	std::vector<CPlugin*> m_PluginList;
	std::vector<MenuCommandInfo> m_ProgramGuideMenuList;

	void SortPluginsByName();
	static bool CompareName(const CPlugin *pPlugin1,const CPlugin *pPlugin2);
	bool SendEvent(UINT Event,LPARAM lParam1=0,LPARAM lParam2=0);
	bool SendProgramGuideEvent(UINT Event,LPARAM Param1=0,LPARAM Param2=0);
	bool SendProgramGuideProgramEvent(UINT Event,const CEventInfoData &EventInfo,LPARAM Param);
	bool SendFilterGraphEvent(UINT Event,CMediaViewer *pMediaViewer,IGraphBuilder *pGraphBuilder);
	void OnRecordingStateChanged();

// CAppEventHandler
	void OnTunerChanged() override;
	void OnChannelChanged(unsigned int Status) override;
	void OnServiceChanged() override;
	void OnServiceInfoUpdated() override;
	void OnServiceListUpdated() override;
	void OnRecordingStart(TVTest::AppEvent::RecordingStartInfo *pInfo) override;
	void OnRecordingStarted() override;
	void OnRecordingStopped() override;
	void OnRecordingPaused() override;
	void OnRecordingResumed() override;
	void OnRecordingFileChanged(LPCTSTR pszFileName) override;
	void OnFullscreenChanged(bool fFullscreen) override;
	void OnPlaybackStateChanged(bool fPlayback) override;
	void OnVolumeChanged(int Volume) override;
	void OnMuteChanged(bool fMute) override;
	void OnStereoModeChanged(int StereoMode) override;
	void OnAudioStreamChanged(int Stream) override;
	void OnColorSchemeChanged() override;
	void OnStandbyChanged(bool fStandby) override;
	void OnExecute(LPCTSTR pszCommandLine) override;
	void OnEngineReset() override;
	void OnStatisticsReset() override;
	void OnSettingsChanged() override;
	void OnClose() override;
	void OnStartupDone() override;

public:
	CPluginManager();
	~CPluginManager();
	bool LoadPlugins(LPCTSTR pszDirectory,const std::vector<LPCTSTR> *pExcludePlugins=NULL);
	void FreePlugins();
	int NumPlugins() const { return (int)m_PluginList.size(); }
	CPlugin *GetPlugin(int Index);
	const CPlugin *GetPlugin(int Index) const;
	bool EnablePlugins(bool fEnable=true);
	int FindPlugin(const CPlugin *pPlugin) const;
	int FindPluginByFileName(LPCTSTR pszFileName) const;
	int FindPluginByCommand(int Command) const;
	CPlugin *GetPluginByPluginCommand(LPCTSTR pszCommand,LPCTSTR *ppszCommandText=NULL);
	bool DeletePlugin(int Index);
	bool SetMenu(HMENU hmenu) const;
	bool OnPluginCommand(LPCTSTR pszCommand);
	bool OnProgramGuideCommand(LPCTSTR pszCommand,UINT Action,const CEventInfoData *pEvent=NULL,
							   const POINT *pCursorPos=NULL,const RECT *pItemRect=NULL);
	bool SendProgramGuideInitializeEvent(HWND hwnd);
	bool SendProgramGuideFinalizeEvent(HWND hwnd);
	bool SendProgramGuideInitializeMenuEvent(HMENU hmenu,UINT *pCommand);
	bool SendProgramGuideMenuSelectedEvent(UINT Command);
	bool SendProgramGuideProgramDrawBackgroundEvent(const CEventInfoData &Event,HDC hdc,
		const RECT &ItemRect,const RECT &TitleRect,const RECT &ContentRect,COLORREF BackgroundColor);
	bool SendProgramGuideProgramInitializeMenuEvent(const CEventInfoData &Event,
		HMENU hmenu,UINT *pCommand,const POINT &CursorPos,const RECT &ItemRect);
	bool SendProgramGuideProgramMenuSelectedEvent(const CEventInfoData &Event,UINT Command);
	bool OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,LRESULT *pResult);
	void SendFilterGraphInitializeEvent(CMediaViewer *pMediaViewer,IGraphBuilder *pGraphBuilder);
	void SendFilterGraphInitializedEvent(CMediaViewer *pMediaViewer,IGraphBuilder *pGraphBuilder);
	void SendFilterGraphFinalizeEvent(CMediaViewer *pMediaViewer,IGraphBuilder *pGraphBuilder);
	void SendFilterGraphFinalizedEvent(CMediaViewer *pMediaViewer,IGraphBuilder *pGraphBuilder);
	void RegisterStatusItems();
	void SendStatusItemCreatedEvent();
	void SendStatusItemUpdateTimerEvent();
};

class CPluginOptions : public COptions
{
	CPluginManager *m_pPluginManager;
	std::vector<LPTSTR> m_EnablePluginList;

	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	void ClearList();

public:
	CPluginOptions(CPluginManager *pPluginManager);
	~CPluginOptions();
// CSettingsBase
	bool LoadSettings(CSettings &Settings) override;
	bool SaveSettings(CSettings &Settings) override;
// CBasicDialog
	bool Create(HWND hwndOwner) override;
// CPluginOptions
	bool RestorePluginOptions();
	bool StorePluginOptions();
};


#endif
