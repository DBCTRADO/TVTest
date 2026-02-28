/*
  TVTest
  Copyright(c) 2008-2022 DBCTRADO

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


#ifndef TVTEST_PLUGIN_H_
#define TVTEST_PLUGIN_H_


#include <vector>
#include <memory>
#define TVTEST_PLUGIN_USE_NAMED_ENUM
#include "TVTestPlugin.h"
#include "AppEvent.h"
#include "Options.h"
#include "VariableManager.h"
#include "LibISDB/LibISDB/Base/ErrorHandler.hpp"


namespace TVTest
{

	class CPlugin
		: public LibISDB::ErrorHandler
	{
	public:
		class CPluginCommandInfo
		{
			int m_ID;
			int m_Command = 0;
			PluginCommandFlag m_Flags = PLUGIN_COMMAND_FLAG_NONE;
			PluginCommandState m_State = PLUGIN_COMMAND_STATE_NONE;
			String m_Text;
			String m_Name;
			Theme::ThemeBitmap m_Icon;

		public:
			CPluginCommandInfo(int ID, LPCWSTR pszText, LPCWSTR pszName);
			CPluginCommandInfo(const PluginCommandInfo &Info);
			CPluginCommandInfo(const CommandInfo &Info);
			virtual ~CPluginCommandInfo() = default;

			int GetID() const { return m_ID; }
			int GetCommand() const { return m_Command; }
			void SetCommand(int Command) { m_Command = Command; }
			PluginCommandFlag GetFlags() const { return m_Flags; }
			PluginCommandState GetState() const { return m_State; }
			void SetState(PluginCommandState State) { m_State = State; }
			LPCWSTR GetText() const { return m_Text.c_str(); }
			LPCWSTR GetName() const { return m_Name.c_str(); }
			Theme::ThemeBitmap &GetIcon() { return m_Icon; }
		};

		CPlugin() = default;
		~CPlugin();

		CPlugin(const CPlugin &) = delete;
		CPlugin &operator=(const CPlugin &) = delete;

		bool Load(LPCTSTR pszFileName);
		void Free();
		bool IsLoaded() const { return m_hLib != nullptr; }
		bool IsEnabled() const { return m_fEnabled; }
		bool Enable(bool fEnable);
		HMODULE GetModuleHandle() { return m_hLib; }
		LPCTSTR GetFileName() const { return m_FileName.c_str(); }
		LPCTSTR GetPluginName() const { return m_PluginName.c_str(); }
		LPCTSTR GetCopyright() const { return m_Copyright.c_str(); }
		LPCTSTR GetDescription() const { return m_Description.c_str(); }
		LRESULT SendEvent(EventCode Event, LPARAM lParam1 = 0, LPARAM lParam2 = 0);
		bool OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult);
		bool Settings(HWND hwndOwner);
		bool HasSettings() const { return (m_Flags & PLUGIN_FLAG_HASSETTINGS) != 0; }
		bool CanUnload() const { return (m_Flags & PLUGIN_FLAG_NOUNLOAD) == 0; }
		bool IsNoEnabledDisabled() const { return (m_Flags & PLUGIN_FLAG_NOENABLEDDISABLED) != 0; }
		int GetCommand() const { return m_Command; }
		bool SetCommand(int Command);
		Theme::ThemeBitmap &GetIcon() { return m_PluginIcon; }
		int NumPluginCommands() const;
		int ParsePluginCommand(LPCWSTR pszCommand) const;
		CPluginCommandInfo *GetPluginCommandInfo(int Index);
		const CPluginCommandInfo *GetPluginCommandInfo(int Index) const;
		CPluginCommandInfo *GetPluginCommandInfo(LPCWSTR pszCommand);
		const CPluginCommandInfo *GetPluginCommandInfo(LPCWSTR pszCommand) const;
		bool GetPluginCommandInfo(int Index, CommandInfo *pInfo) const;
		bool NotifyCommand(LPCWSTR pszCommand);
		bool DrawPluginCommandIcon(const DrawCommandIconInfo *pInfo);
		int NumProgramGuideCommands() const;
		bool GetProgramGuideCommandInfo(int Index, ProgramGuideCommandInfo *pInfo) const;
		bool NotifyProgramGuideCommand(
			LPCTSTR pszCommand, ProgramGuideCommandAction Action, const LibISDB::EventInfo *pEvent,
			const POINT *pCursorPos, const RECT *pItemRect);
		bool IsDisableOnStart() const;
		bool IsProgramGuideEventEnabled(UINT EventFlag) const { return (m_ProgramGuideEventFlags & EventFlag) != 0; }
		void RegisterStatusItems();
		void SendStatusItemCreatedEvent();
		void SendStatusItemUpdateTimerEvent();
		void RegisterPanelItems();

		static void SetMessageWindow(HWND hwnd, UINT Message);
		static LRESULT OnPluginMessage(WPARAM wParam, LPARAM lParam);

	private:
		class CProgramGuideCommand
			: public CPluginCommandInfo
		{
			ProgramGuideCommandType m_Type;

		public:
			CProgramGuideCommand(const ProgramGuideCommandInfo &Info);

			ProgramGuideCommandType GetType() const { return m_Type; }
		};

		class CStreamGrabber
			: public LibISDB::GrabberFilter::Grabber
		{
		public:
			CStreamGrabber(StreamCallbackFunc Callback, void *pClientData);

			StreamCallbackFunc GetCallbackFunc() const { return m_Callback; }
			void SetClientData(void *pClientData);

		private:
			StreamCallbackFunc m_Callback;
			void *m_pClientData;

		// GrabberFilter::Grabber
			bool ReceiveData(LibISDB::DataBuffer *pData) override;
		};

		class CPluginStatusItem;

		struct StatusItem
		{
			StatusItemFlag Flags;
			int ID;
			String IDText;
			String Name;
			int MinWidth;
			int MaxWidth;
			int DefaultWidth;
			int MinHeight;
			int ItemID;
			StatusItemStyle Style;
			StatusItemState State;
			CPluginStatusItem *pItem;
		};

		class CPluginStatusItem
			: public CStatusItem
		{
		public:
			CPluginStatusItem(CPlugin *pPlugin, StatusItem *pItem);
			~CPluginStatusItem();

		// CStatusItem
			LPCTSTR GetIDText() const override { return m_IDText.c_str(); }
			LPCTSTR GetName() const override { return m_pItem->Name.c_str(); }
			void Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags) override;
			void OnLButtonDown(int x, int y) override;
			void OnLButtonUp(int x, int y) override;
			void OnLButtonDoubleClick(int x, int y) override;
			void OnRButtonDown(int x, int y) override;
			void OnRButtonUp(int x, int y) override;
			void OnRButtonDoubleClick(int x, int y) override;
			void OnMButtonDown(int x, int y) override;
			void OnMButtonUp(int x, int y) override;
			void OnMButtonDoubleClick(int x, int y) override;
			void OnMouseMove(int x, int y) override;
			bool OnMouseWheel(int x, int y, bool fHorz, int Delta, int *pCommand) override;
			void OnVisibilityChanged() override;
			void OnFocus(bool fFocus) override;
			void OnSizeChanged() override;
			void OnCaptureReleased() override;
			void OnFontChanged() override;

		// CPluginStatusItem
			void DetachItem();
			HWND GetWindowHandle() const;
			void ApplyItemStyle();

		private:
			CPlugin *m_pPlugin;
			StatusItem *m_pItem;
			String m_IDText;

		// CUIBase
			void RealizeStyle() override;

			void NotifyDraw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags);
			LRESULT NotifyMouseEvent(StatusItemMouseAction Action, int x, int y, int WheelDelta = 0);
		};

		class CPluginPanelItem;

		struct PanelItem
		{
			int ID;
			String IDText;
			String Title;
			PanelItemState StateMask;
			PanelItemState State;
			PanelItemStyle Style;
			int ItemID;
			CPluginPanelItem *pItem;
			Theme::ThemeBitmap Icon;
		};

		class CPluginPanelItem
			: public CPanelForm::CPage
		{
		public:
			CPluginPanelItem(CPlugin *pPlugin, PanelItem *pItem);
			~CPluginPanelItem();

			bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;
			void DetachItem();
			HWND GetItemHandle() const { return m_hwndItem; }

		private:
			LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
			void RealizeStyle() override;
			bool SetFont(const Style::Font &Font) override;
			void OnActivate() override;
			void OnDeactivate() override;
			void OnVisibilityChanged(bool fVisible) override;
			void OnFormDelete() override;
			bool DrawIcon(
				HDC hdc, int x, int y, int Width, int Height,
				const Theme::ThemeColor &Color) override;
			bool NeedKeyboardFocus() const override;

			CPlugin *m_pPlugin;
			PanelItem *m_pItem;
			HWND m_hwndItem = nullptr;

			static bool m_fInitialized;
		};

		class CGetVariable
			: public CVariableManager::IGetVariable
		{
		public:
			CGetVariable(CPlugin *pPlugin);
			bool GetVariable(LPCWSTR pszKeyword, String *pValue) override;

		private:
			CPlugin *m_pPlugin;
		};

		struct AppCommand
		{
			int ID;
			String IDText;
			String Text;
		};

		HMODULE m_hLib = nullptr;
		String m_FileName;
		PluginParam m_PluginParam;
		DWORD m_Version = 0;
		PluginType m_Type = PLUGIN_TYPE_NORMAL;
		PluginFlag m_Flags = PLUGIN_FLAG_NONE;
		String m_PluginName;
		String m_Copyright;
		String m_Description;
		bool m_fEnabled = false;
		bool m_fSetting = false;
		int m_Command = 0;
		Theme::ThemeBitmap m_PluginIcon;
		EventCallbackFunc m_pEventCallback = nullptr;
		void *m_pEventCallbackClientData;
		UINT m_ProgramGuideEventFlags = 0;
		WindowMessageCallbackFunc m_pMessageCallback = nullptr;
		void *m_pMessageCallbackClientData;
		std::vector<std::unique_ptr<CStreamGrabber>> m_StreamGrabberList;
		MutexLock m_GrabberLock;
		std::vector<CPluginCommandInfo> m_CommandList;
		std::vector<CProgramGuideCommand> m_ProgramGuideCommandList;
		std::vector<String> m_ControllerList;
		std::vector<std::unique_ptr<StatusItem>> m_StatusItemList;
		std::vector<std::unique_ptr<PanelItem>> m_PanelItemList;
		CGetVariable m_GetVariable{this};
		std::vector<AppCommand> m_AppCommandList;

		static HWND m_hwndMessage;
		static UINT m_MessageCode;

		class CAudioStreamCallbackInfo
		{
		public:
			CPlugin *m_pPlugin;
			AudioCallbackFunc m_pCallback;
			void *m_pClientData;
			CAudioStreamCallbackInfo(CPlugin *pPlugin, AudioCallbackFunc pCallback, void *pClientData)
				: m_pPlugin(pPlugin)
				, m_pCallback(pCallback)
				, m_pClientData(pClientData)
			{
			}
		};
		static MutexLock m_AudioStreamLock;
		static std::vector<CAudioStreamCallbackInfo> m_AudioStreamCallbackList;

		class CVideoStreamCallbackInfo
		{
		public:
			CPlugin *m_pPlugin;
			VideoStreamCallbackFunc m_pCallback;
			void *m_pClientData;
			CVideoStreamCallbackInfo(CPlugin *pPlugin, VideoStreamCallbackFunc pCallback, void *pClientData)
				: m_pPlugin(pPlugin)
				, m_pCallback(pCallback)
				, m_pClientData(pClientData)
			{
			}
		};

		class CVideoStreamCallback
			: public LibISDB::DirectShow::VideoParser::StreamCallback
		{
			void OnStream(DWORD Format, const void *pData, size_t Size) override;
		};

		static std::vector<CVideoStreamCallbackInfo> m_VideoStreamCallbackList;
		static CVideoStreamCallback m_VideoStreamCallback;
		static MutexLock m_VideoStreamLock;

		class CAudioSampleCallback
			: public LibISDB::DirectShow::AudioDecoderFilter::SampleCallback
		{
			void OnSamples(short *pData, size_t Length, int Channels) override;
		};

		static CAudioSampleCallback m_AudioSampleCallback;

		static LRESULT CALLBACK Callback(PluginParam *pParam, MessageCode Message, LPARAM lParam1, LPARAM lParam2);

	// CPlugin
		LRESULT SendPluginMessage(
			PluginParam *pParam, MessageCode Message, LPARAM lParam1, LPARAM lParam2,
			LRESULT FailedResult = 0);
		LRESULT OnCallback(PluginParam *pParam, MessageCode Message, LPARAM lParam1, LPARAM lParam2);
		bool OnGetSetting(SettingInfo *pSetting) const;
		void CreateAppCommandList();
	};

	class CPluginManager
		: public CAppEventHandler
	{
		struct MenuCommandInfo
		{
			CPlugin *pPlugin;
			UINT CommandFirst;
			UINT CommandEnd;
		};

		std::vector<std::unique_ptr<CPlugin>> m_PluginList;
		std::vector<MenuCommandInfo> m_ProgramGuideMenuList;

		void SortPluginsByName();
		static bool CompareName(
			const std::unique_ptr<CPlugin> &Plugin1,
			const std::unique_ptr<CPlugin> &Plugin2);
		bool SendEvent(EventCode Event, LPARAM lParam1 = 0, LPARAM lParam2 = 0);
		bool SendProgramGuideEvent(EventCode Event, LPARAM Param1 = 0, LPARAM Param2 = 0);
		bool SendProgramGuideProgramEvent(EventCode Event, const LibISDB::EventInfo &EventInfo, LPARAM Param);
		bool SendFilterGraphEvent(EventCode Event, LibISDB::ViewerFilter *pMediaViewer, IGraphBuilder *pGraphBuilder);
		void OnRecordingStateChanged();

	// CAppEventHandler
		void OnTunerChanged() override;
		void OnTunerShutDown() override;
		void OnChannelChanged(AppEvent::ChannelChangeStatus Status) override;
		void OnServiceChanged() override;
		void OnServiceInfoUpdated() override;
		void OnServiceListUpdated() override;
		void OnRecordingStart(AppEvent::RecordingStartInfo *pInfo) override;
		void OnRecordingStarted() override;
		void OnRecordingStopped() override;
		void OnRecordingPaused() override;
		void OnRecordingResumed() override;
		void OnRecordingFileChanged(LPCTSTR pszFileName) override;
		void On1SegModeChanged(bool f1SegMode) override;
		void OnFullscreenChanged(bool fFullscreen) override;
		void OnPlaybackStateChanged(bool fPlayback) override;
		void OnVideoFormatChanged() override;
		void OnVolumeChanged(int Volume) override;
		void OnMuteChanged(bool fMute) override;
		void OnDualMonoModeChanged(LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode Mode) override;
		void OnAudioStreamChanged(int Stream) override;
		void OnAudioFormatChanged() override;
		void OnColorSchemeChanged() override;
		void OnStandbyChanged(bool fStandby) override;
		void OnExecute(LPCTSTR pszCommandLine) override;
		void OnEngineReset() override;
		void OnStatisticsReset() override;
		void OnSettingsChanged() override;
		void OnClose() override;
		void OnStartupDone() override;
		void OnFavoritesChanged() override;
		void OnDarkModeChanged(bool fDarkMode) override;
		void OnMainWindowDarkModeChanged(bool fDarkMode) override;
		void OnProgramGuideDarkModeChanged(bool fDarkMode) override;
		void OnEventChanged() override;
		void OnEventInfoChanged() override;

	public:
		~CPluginManager();

		bool LoadPlugins(LPCTSTR pszDirectory, const std::vector<LPCTSTR> *pExcludePlugins = nullptr);
		void FreePlugins();
		int NumPlugins() const { return static_cast<int>(m_PluginList.size()); }
		CPlugin *GetPlugin(int Index);
		const CPlugin *GetPlugin(int Index) const;
		bool EnablePlugins(bool fEnable = true);
		int FindPlugin(const CPlugin *pPlugin) const;
		int FindPluginByFileName(LPCTSTR pszFileName) const;
		int FindPluginByCommand(int Command) const;
		CPlugin *GetPluginByCommand(int Command);
		CPlugin *GetPluginByPluginCommand(LPCTSTR pszCommand, LPCTSTR *ppszCommandText = nullptr);
		bool DeletePlugin(int Index);
		bool SetMenu(HMENU hmenu) const;
		bool OnPluginCommand(LPCTSTR pszCommand);
		bool OnProgramGuideCommand(
			LPCTSTR pszCommand, ProgramGuideCommandAction Action, const LibISDB::EventInfo *pEvent = nullptr,
			const POINT *pCursorPos = nullptr, const RECT *pItemRect = nullptr);
		bool SendProgramGuideInitializeEvent(HWND hwnd);
		bool SendProgramGuideFinalizeEvent(HWND hwnd);
		bool SendProgramGuideInitializeMenuEvent(HMENU hmenu, UINT *pCommand);
		bool SendProgramGuideMenuSelectedEvent(UINT Command);
		bool SendProgramGuideProgramDrawBackgroundEvent(
			const LibISDB::EventInfo &Event, HDC hdc,
			const RECT &ItemRect, const RECT &TitleRect, const RECT &ContentRect, COLORREF BackgroundColor);
		bool SendProgramGuideProgramInitializeMenuEvent(
			const LibISDB::EventInfo &Event,
			HMENU hmenu, UINT *pCommand, const POINT &CursorPos, const RECT &ItemRect);
		bool SendProgramGuideProgramMenuSelectedEvent(const LibISDB::EventInfo &Event, UINT Command);
		bool OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult);
		void SendFilterGraphInitializeEvent(LibISDB::ViewerFilter *pMediaViewer, IGraphBuilder *pGraphBuilder);
		void SendFilterGraphInitializedEvent(LibISDB::ViewerFilter *pMediaViewer, IGraphBuilder *pGraphBuilder);
		void SendFilterGraphFinalizeEvent(LibISDB::ViewerFilter *pMediaViewer, IGraphBuilder *pGraphBuilder);
		void SendFilterGraphFinalizedEvent(LibISDB::ViewerFilter *pMediaViewer, IGraphBuilder *pGraphBuilder);
		void RegisterStatusItems();
		void SendStatusItemCreatedEvent();
		void SendStatusItemUpdateTimerEvent();
		void RegisterPanelItems();
	};

	class CPluginOptions
		: public COptions
	{
		enum {
			COLUMN_FILENAME,
			COLUMN_PLUGINNAME,
			COLUMN_DESCRIPTION,
			COLUMN_COPYRIGHT,
			NUM_COLUMNS
		};

		CPluginManager *m_pPluginManager;
		std::vector<String> m_EnablePluginList;

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void RealizeStyle() override;

	// CPluginOptions
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

} // namespace TVTest


#endif
