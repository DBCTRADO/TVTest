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


#ifndef TVTEST_APP_COMMAND_H
#define TVTEST_APP_COMMAND_H


#include "Command.h"
#include <initializer_list>


namespace TVTest
{

	class CAppMain;

	class CAppCommand
	{
	public:
		CAppCommand(CAppMain &App);

		void RegisterDefaultCommands();
		void RegisterDynamicCommands();
		void InitializeCommandState();

	private:
		struct CommandParameters
		{
			int FirstID;
			int LastID;
			int LastListingID;
			LPCTSTR pszIDText;
			const CCommandManager::CommandHandler &Handler;
			LPCTSTR pszText;
			LPCTSTR pszShortText;
			CCommandManager::CommandState State;

			static const CCommandManager::CommandHandler NullHandler;

			CommandParameters(
				int a_FirstID, int a_LastID, int a_LastListingID,
				LPCTSTR a_pszIDText,
				const CCommandManager::CommandHandler &a_Handler = NullHandler,
				LPCTSTR a_pszText = nullptr, LPCTSTR a_pszShortText = nullptr,
				CCommandManager::CommandState a_State = CCommandManager::CommandState::None)
				: FirstID(a_FirstID)
				, LastID(a_LastID)
				, LastListingID(a_LastListingID)
				, pszIDText(a_pszIDText)
				, Handler(a_Handler)
				, pszText(a_pszText)
				, pszShortText(a_pszShortText)
				, State(a_State)
			{
			}

			CommandParameters(
				int ID,
				LPCTSTR a_pszIDText,
				const CCommandManager::CommandHandler &a_Handler = NullHandler,
				LPCTSTR a_pszText = nullptr, LPCTSTR a_pszShortText = nullptr,
				CCommandManager::CommandState a_State = CCommandManager::CommandState::None)
				: FirstID(ID)
				, LastID(ID)
				, LastListingID(ID)
				, pszIDText(a_pszIDText)
				, Handler(a_Handler)
				, pszText(a_pszText)
				, pszShortText(a_pszShortText)
				, State(a_State)
			{
			}
		};

		void RegisterCommands(std::initializer_list<CommandParameters> List);
		void RegisterCommands(
			std::initializer_list<CommandParameters> List,
			const CCommandManager::CommandHandler &Handler);

		bool ZoomOptions(CCommandManager::InvokeParameters &Params);
		bool ToggleAspectRatio(CCommandManager::InvokeParameters &Params);
		bool SelectAspectRatio(CCommandManager::InvokeParameters &Params);
		bool SelectPanAndScanPreset(CCommandManager::InvokeParameters &Params);
		bool PanAndScanOptions(CCommandManager::InvokeParameters &Params);
		bool ToggleFrameCut(CCommandManager::InvokeParameters &Params);
		bool ToggleFullscreen(CCommandManager::InvokeParameters &Params);
		bool ToggleAlwaysOnTop(CCommandManager::InvokeParameters &Params);

		bool VolumeUpDown(CCommandManager::InvokeParameters &Params);
		bool ToggleMute(CCommandManager::InvokeParameters &Params);
		bool SelectAudioGain(CCommandManager::InvokeParameters &Params);
		bool SelectSurroundAudioGain(CCommandManager::InvokeParameters &Params);
		bool AudioDelay(CCommandManager::InvokeParameters &Params);
		bool DualMonoMode(CCommandManager::InvokeParameters &Params);
		bool SwitchAudio(CCommandManager::InvokeParameters &Params);
		bool SelectSPDIFMode(CCommandManager::InvokeParameters &Params);
		bool ToggleSPDIFMode(CCommandManager::InvokeParameters &Params);

		bool DefaultCapture(CCommandManager::InvokeParameters &Params);
		bool CaptureCopySave(CCommandManager::InvokeParameters &Params);
		bool CapturePreview(CCommandManager::InvokeParameters &Params);
		bool CaptureOptions(CCommandManager::InvokeParameters &Params);
		bool OpenCaptureFolder(CCommandManager::InvokeParameters &Params);
		bool CaptureSize(CCommandManager::InvokeParameters &Params);

		bool Reset(CCommandManager::InvokeParameters &Params);
		bool ResetViewer(CCommandManager::InvokeParameters &Params);

		bool Record(CCommandManager::InvokeParameters &Params);
		bool RecordPauseResume(CCommandManager::InvokeParameters &Params);
		bool RecordOptions(CCommandManager::InvokeParameters &Params);
		bool RecordEvent(CCommandManager::InvokeParameters &Params);
		bool ExitOnRecordingStop(CCommandManager::InvokeParameters &Params);
		bool OptionsRecordPage(CCommandManager::InvokeParameters &Params);
		bool StartTimeShiftRecording(CCommandManager::InvokeParameters &Params);
		bool EnableTimeShiftRecording(CCommandManager::InvokeParameters &Params);
		bool StatusBarRecord(CCommandManager::InvokeParameters &Params);

		bool Options(CCommandManager::InvokeParameters &Params);
		bool StreamInfo(CCommandManager::InvokeParameters &Params);
		bool Close(CCommandManager::InvokeParameters &Params);
		bool Exit(CCommandManager::InvokeParameters &Params);

		bool ChannelUpDown(CCommandManager::InvokeParameters &Params);
		bool ChannelBackwardForward(CCommandManager::InvokeParameters &Params);
		bool ChannelPrevious(CCommandManager::InvokeParameters &Params);
		bool UpdateChannelList(CCommandManager::InvokeParameters &Params);
		bool ToggleOneSegMode(CCommandManager::InvokeParameters &Params);
		bool CloseTuner(CCommandManager::InvokeParameters &Params);

		bool EnableBuffering(CCommandManager::InvokeParameters &Params);
		bool ResetBuffer(CCommandManager::InvokeParameters &Params);
		bool ResetErrorCount(CCommandManager::InvokeParameters &Params);
		bool ShowRecordRemainTime(CCommandManager::InvokeParameters &Params);
		bool ShowTOTTime(CCommandManager::InvokeParameters &Params);
		bool InterpolateTOTTime(CCommandManager::InvokeParameters &Params);
		bool ProgramInfoStatus_PopupInfo(CCommandManager::InvokeParameters &Params);
		bool ProgramInfoStatus_ShowProgress(CCommandManager::InvokeParameters &Params);
		bool AdjustTOTTime(CCommandManager::InvokeParameters &Params);
		bool SideBarOptions(CCommandManager::InvokeParameters &Params);

		bool DriverBrowse(CCommandManager::InvokeParameters &Params);
		bool ChannelHistoryClear(CCommandManager::InvokeParameters &Params);
		bool AddToFavorites(CCommandManager::InvokeParameters &Params);
		bool OrganizeFavorites(CCommandManager::InvokeParameters &Params);

		bool SwitchVideo(CCommandManager::InvokeParameters &Params);
		bool VideoStreamSwitch(CCommandManager::InvokeParameters &Params);
		bool MultiViewSwitch(CCommandManager::InvokeParameters &Params);

		bool SelectChannelNo(CCommandManager::InvokeParameters &Params);
		bool SelectChannel(CCommandManager::InvokeParameters &Params);
		bool SelectService(CCommandManager::InvokeParameters &Params);
		bool SelectSpace(CCommandManager::InvokeParameters &Params);
		bool SelectDriver(CCommandManager::InvokeParameters &Params);
		bool SelectSpaceChannel(CCommandManager::InvokeParameters &Params);
		bool ChannelHistory(CCommandManager::InvokeParameters &Params);
		bool FavoriteChannel(CCommandManager::InvokeParameters &Params);

		bool EnablePlugin(CCommandManager::InvokeParameters &Params);
		bool PluginCommand(CCommandManager::InvokeParameters &Params);

		bool VideoStream(CCommandManager::InvokeParameters &Params);
		bool AudioStream(CCommandManager::InvokeParameters &Params);
		bool SelectAudio(CCommandManager::InvokeParameters &Params);
		bool MultiView(CCommandManager::InvokeParameters &Params);

		bool SelectPanelPage(CCommandManager::InvokeParameters &Params);

		bool ToggleEventInfoOSD(CCommandManager::InvokeParameters &Params);

		CAppMain &m_App;

		static const std::uint8_t m_AudioGainList[];
	};

}	// namespace TVTest


#endif
