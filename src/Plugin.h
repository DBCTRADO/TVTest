#ifndef PLUGIN_H
#define PLUGIN_H


#include <vector>
#include "TVTestPlugin.h"
#include "Options.h"
#include "MediaData.h"
#include "BonTsEngine/Exception.h"
#include "BonTsEngine/TsUtilClass.h"


class CPlugin : public CBonErrorHandler
{
	class CPluginCommandInfo {
		int m_ID;
		CDynamicString m_Text;
		CDynamicString m_Name;
	public:
		CPluginCommandInfo(int ID,LPCWSTR pszText,LPCWSTR pszName);
		CPluginCommandInfo(const TVTest::CommandInfo &Info);
		virtual ~CPluginCommandInfo();
		int GetID() const { return m_ID; }
		LPCWSTR GetText() const { return m_Text.Get(); }
		LPCWSTR GetName() const { return m_Name.Get(); }
	};

	class CProgramGuideCommand : public CPluginCommandInfo {
		UINT m_Type;
	public:
		CProgramGuideCommand(const TVTest::ProgramGuideCommandInfo &Info);
		UINT GetType() const { return m_Type; }
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
	std::vector<CPluginCommandInfo> m_CommandList;
	std::vector<CProgramGuideCommand> m_ProgramGuideCommandList;
	std::vector<CDynamicString> m_ControllerList;

	static HWND m_hwndMessage;
	static UINT m_MessageCode;

	class CMediaGrabberInfo {
	public:
		CPlugin *m_pPlugin;
		TVTest::StreamCallbackInfo m_CallbackInfo;
		CMediaGrabberInfo(CPlugin *pPlugin,TVTest::StreamCallbackInfo *pCallbackInfo)
			: m_pPlugin(pPlugin)
			, m_CallbackInfo(*pCallbackInfo)
		{
		}
	};
	static bool m_fSetGrabber;
	static std::vector<CMediaGrabberInfo> m_GrabberList;
	static CCriticalLock m_GrabberLock;
	static bool CALLBACK GrabMediaCallback(const CMediaData *pMediaData,const PVOID pParam);
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
	static LRESULT SendPluginMessage(TVTest::PluginParam *pParam,UINT Message,LPARAM lParam1,LPARAM lParam2,
									 LRESULT FailedResult=0);

public:
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
	bool GetPluginCommandInfo(int Index,TVTest::CommandInfo *pInfo) const;
	bool NotifyCommand(LPCWSTR pszCommand);
	int NumProgramGuideCommands() const;
	bool GetProgramGuideCommandInfo(int Index,TVTest::ProgramGuideCommandInfo *pInfo) const;
	bool NotifyProgramGuideCommand(LPCTSTR pszCommand,UINT Action,const CEventInfoData *pEvent,
								   const POINT *pCursorPos,const RECT *pItemRect);
	bool IsDisableOnStart() const;
	bool IsProgramGuideEventEnabled(UINT EventFlag) const { return (m_ProgramGuideEventFlags&EventFlag)!=0; }

	static void SetMessageWindow(HWND hwnd,UINT Message);
	static LRESULT OnPluginMessage(WPARAM wParam,LPARAM lParam);
};

class CPluginManager
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
	bool DeletePlugin(int Index);
	bool SetMenu(HMENU hmenu) const;
	bool OnPluginCommand(LPCTSTR pszCommand);
	bool OnProgramGuideCommand(LPCTSTR pszCommand,UINT Action,const CEventInfoData *pEvent=NULL,
							   const POINT *pCursorPos=NULL,const RECT *pItemRect=NULL);
	bool SendChannelChangeEvent();
	bool SendServiceChangeEvent();
	bool SendDriverChangeEvent();
	bool SendServiceUpdateEvent();
	bool SendRecordStatusChangeEvent();
	bool SendFullscreenChangeEvent(bool fFullscreen);
	bool SendPreviewChangeEvent(bool fPreview);
	bool SendVolumeChangeEvent(int Volume,bool fMute);
	bool SendStereoModeChangeEvent(int StereoMode);
	bool SendColorChangeEvent();
	bool SendStandbyEvent(bool fStandby);
	bool SendExecuteEvent(LPCTSTR pszCommandLine);
	bool SendResetEvent();
	bool SendStatusResetEvent();
	bool SendAudioStreamChangeEvent(int Stream);
	bool SendSettingsChangeEvent();
	bool SendCloseEvent();
	bool SendStartRecordEvent(const class CRecordManager *pRecordManager,LPTSTR pszFileName,int MaxFileName);
	bool SendRelayRecordEvent(LPCTSTR pszFileName);
	bool SendStartupDoneEvent();
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
