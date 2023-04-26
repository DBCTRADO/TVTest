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


#ifndef TVTEST_UI_CORE_H
#define TVTEST_UI_CORE_H


#include <vector>
#include "UISkin.h"
#include "ColorSchemeOptions.h"
#include "CoreEngine.h"
#include "AudioManager.h"
#include "Menu.h"
#include "GUIUtil.h"
#include "VariableString.h"
#include "LibISDB/LibISDB/Base/Logger.hpp"


namespace TVTest
{

	class CAppMain;
	class CBasicDialog;

	class CUICore
		: public CColorSchemeOptions::CEventHandler
		, public LibISDB::Logger
	{
	public:
		enum class PopupMenuFlag : unsigned int {
			None    = 0x0000U,
			Default = 0x0001U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		enum class MenuType {
			TunerSelect,
			Record,
			Capture,
			Buffering,
			StreamError,
			Clock,
			ProgramInfo,
		};

		enum {
			ASPECTRATIO_DEFAULT,
			ASPECTRATIO_16X9,
			ASPECTRATIO_LETTERBOX,
			ASPECTRATIO_WINDOWBOX,
			ASPECTRATIO_PILLARBOX,
			ASPECTRATIO_4X3,
			ASPECTRATIO_32X9,
			ASPECTRATIO_16X9_LEFT,
			ASPECTRATIO_16X9_RIGHT,
			ASPECTRATIO_CUSTOM_FIRST,
			ASPECTRATIO_FIRST    = ASPECTRATIO_DEFAULT,
			ASPECTRATIO_2D_LAST  = ASPECTRATIO_4X3,
			ASPECTRATIO_3D_FIRST = ASPECTRATIO_32X9,
			ASPECTRATIO_3D_LAST  = ASPECTRATIO_16X9_RIGHT,
		};

		class CTitleStringMap
			: public CEventVariableStringMap
		{
		public:
			CTitleStringMap(CAppMain &App, const EventInfo *pInfo = nullptr);

			bool GetParameterList(ParameterGroupList *pList) const override;

		private:
			bool GetLocalString(LPCWSTR pszKeyword, String *pString) override;

			CAppMain &m_App;
		};

		CUICore(CAppMain &App);
		~CUICore();

		CUICore(const CUICore &) = delete;
		CUICore &operator=(const CUICore &) = delete;

		bool SetSkin(CUISkin *pSkin);
		CUISkin *GetSkin() const { return m_pSkin; }
		HWND GetMainWindow() const;
		HWND GetDialogOwner() const;

		int GetNextChannel(bool fUp);

		bool InitializeViewer(BYTE VideoStreamType = 0);
		bool IsViewerInitializeError() const { return m_fViewerInitializeError; }
		bool FinalizeViewer();
		bool IsViewerEnabled() const;
		bool EnableViewer(bool fEnable);
		HWND GetViewerWindow() const;

		bool SetZoomRate(int Rate, int Factor = 100);
		bool GetZoomRate(int *pRate, int *pFactor) const;
		int GetZoomPercentage() const;
		bool GetPanAndScan(CCoreEngine::PanAndScanInfo *pInfo) const;
		bool SetPanAndScan(const CCoreEngine::PanAndScanInfo &Info);
		bool SetAspectRatioType(int Type);
		int GetAspectRatioType() const { return m_AspectRatioType; }
		void ResetAspectRatioType();

		int GetVolume() const;
		bool SetVolume(int Volume, bool fOSD = true);
		bool GetMute() const;
		bool SetMute(bool fMute);
		bool SetDualMonoMode(LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode Mode);
		LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode GetDualMonoMode() const;
		int GetNumAudioStreams() const;
		int GetAudioStream() const;
		bool SetAudioStream(int Stream);
		int GetAudioChannelCount() const;
		bool SelectAudio(int Index);
		bool AutoSelectAudio();
		bool SwitchAudio();
		bool SwitchDualMonoMode();
		int FormatCurrentAudioText(LPTSTR pszText, int MaxLength) const;
		bool GetSelectedAudioText(LPTSTR pszText, int MaxLength) const;
		void ShowAudioOSD();

		bool GetStandby() const { return m_fStandby; }
		bool SetStandby(bool fStandby, bool fTransient = false);
		bool GetTransientStandby() const { return m_fTransientStandby; }
		bool GetResident() const;
		bool SetResident(bool fResident);
		bool GetFullscreen() const { return m_fFullscreen; }
		bool SetFullscreen(bool fFullscreen);
		bool ToggleFullscreen();
		bool GetAlwaysOnTop() const { return m_fAlwaysOnTop; }
		bool SetAlwaysOnTop(bool fTop);
		bool PreventDisplaySave(bool fPrevent);

		void PopupMenu(const POINT *pPos = nullptr, PopupMenuFlag Flags = PopupMenuFlag::None);
		void PopupSubMenu(
			int SubMenu, const POINT *pPos = nullptr, UINT Flags = 0,
			const RECT *pExcludeRect = nullptr);
		bool ShowSpecialMenu(
			MenuType Menu, const POINT *pPos = nullptr, UINT Flags = 0,
			const RECT *pExcludeRect = nullptr);
		void InitChannelMenu(HMENU hmenu);
		void InitTunerMenu(HMENU hmenu);
		bool ProcessTunerMenu(int Command);
		bool HandleInitMenuPopup(HMENU hmenu);
		void SetPopupMenuDPI(int DPI);
		int GetPopupMenuDPI() const { return m_PopupMenuDPI; }

		bool DoCommand(int Command);
		bool DoCommand(LPCTSTR pszCommand);
		bool DoCommandAsync(int Command);
		bool DoCommandAsync(LPCTSTR pszCommand);

		bool SetCommandEnabledState(int Command, bool fEnabled);
		bool GetCommandEnabledState(int Command) const;
		bool SetCommandCheckedState(int Command, bool fChecked);
		bool GetCommandCheckedState(int Command) const;
		bool SetCommandRadioCheckedState(int FirstCommand, int LastCommand, int CheckedCommand);

		bool ConfirmChannelChange();
		bool ConfirmStopRecording();

		bool UpdateIcon();
		bool UpdateTitle();
		bool SetTitleFont(const Style::Font &Font);
		bool SetLogo(LPCTSTR pszFileName);

		bool RegisterModelessDialog(CBasicDialog *pDialog);
		bool UnregisterModelessDialog(CBasicDialog *pDialog);
		bool ProcessDialogMessage(MSG *pMessage);

		COLORREF GetColor(LPCTSTR pszText) const;
		const CColorScheme *GetCurrentColorScheme() const;

		HCURSOR GetActionCursor() const;
		HCURSOR GetLinkCursor() const;

		bool ShowHelpContent(int ID);

		void SetProgress(int Pos, int Max);
		void EndProgress();

		void SetStatusBarTrace(bool fStatusBarTrace);

	private:
		class CTunerSelectMenu
		{
		public:
			CTunerSelectMenu(CUICore &UICore);
			~CTunerSelectMenu();

			bool Create(HWND hwnd);
			void Destroy();
			int Show(UINT Flags, int x, int y, const RECT *pExcludeRect = nullptr);
			bool OnInitMenuPopup(HMENU hmenu);

		private:
			struct PopupInfo
			{
				const CChannelList *pChannelList;
				int Command;

				PopupInfo(const CChannelList *pList, int Cmd)
					: pChannelList(pList)
					, Command(Cmd)
				{
				}
			};

			CUICore &m_UICore;
			CPopupMenu m_Menu;
			HWND m_hwnd = nullptr;
			std::vector<PopupInfo> m_PopupList;
		};

		CAppMain &m_App;
		CUISkin *m_pSkin = nullptr;
		bool m_fStandby = false;
		bool m_fTransientStandby = false;
		bool m_fResident = false;
		bool m_fFullscreen = false;
		bool m_fAlwaysOnTop = false;

		int m_AspectRatioType = ASPECTRATIO_DEFAULT;

		CIcon m_LogoIconBig;
		CIcon m_LogoIconSmall;

		bool m_fViewerInitializeError = false;

		HANDLE m_hPowerRequest = INVALID_HANDLE_VALUE;
		bool m_fPowerRequestSet = false;
		BOOL m_fScreenSaverActiveOriginal = FALSE;

		int m_PopupMenuDPI = 0;
		CTunerSelectMenu m_TunerSelectMenu{*this};

		std::vector<CBasicDialog*> m_ModelessDialogList;

		const CColorScheme *m_pColorScheme = nullptr;

		bool m_fStatusBarTrace = false;

		bool SelectAudio(const CAudioManager::AudioSelectInfo &Info, bool fUpdate = true);
		bool SelectAudioStream(int Stream);
		bool SelectDualMonoMode(LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode Mode, bool fUpdate);

		bool CreateChannelMenu(
			const CChannelList *pChannelList, int CurChannel,
			UINT Command, UINT LastCommand, HMENU hmenu, HWND hwnd,
			CChannelMenu::CreateFlag Flags = CChannelMenu::CreateFlag::None);
		bool InitChannelMenuPopup(HMENU hmenuParent, HMENU hmenu);

		bool SetPowerRequest(bool fSet);

	// CColorSchemeOptions::CEventHandler
		bool ApplyColorScheme(const CColorScheme *pColorScheme) override;

	// LibISDB::Logger
		void OnLog(LibISDB::Logger::LogType Type, LPCTSTR pszOutput) override;
	};

}	// namespace TVTest


#endif
