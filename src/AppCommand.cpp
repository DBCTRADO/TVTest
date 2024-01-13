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
#include "AppCommand.h"
#include "AppMain.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


const std::uint8_t CAppCommand::m_AudioGainList[] = {100, 125, 150, 200};


CAppCommand::CAppCommand(CAppMain &App)
	 : m_App(App)
{
}


void CAppCommand::RegisterDefaultCommands()
{
	CCommandManager::CommandHandler MainWindowHandler =
		CCommandManager::BindHandler(&CMainWindow::HandleCommand, &m_App.MainWindow);

	RegisterCommands(
		{
			{CM_ZOOM_20,  TEXT("Zoom20")},
			{CM_ZOOM_25,  TEXT("Zoom25")},
			{CM_ZOOM_33,  TEXT("Zoom33")},
			{CM_ZOOM_50,  TEXT("Zoom50")},
			{CM_ZOOM_66,  TEXT("Zoom66")},
			{CM_ZOOM_75,  TEXT("Zoom75")},
			{CM_ZOOM_100, TEXT("Zoom100")},
			{CM_ZOOM_150, TEXT("Zoom150")},
			{CM_ZOOM_200, TEXT("Zoom200")},
			{CM_ZOOM_250, TEXT("Zoom250")},
			{CM_ZOOM_300, TEXT("Zoom300")},
			{CM_CUSTOMZOOM_FIRST, CM_CUSTOMZOOM_LAST, CM_CUSTOMZOOM_LAST, TEXT("CustomZoom")},
		},
		MainWindowHandler);

	RegisterCommands(
		{
			{CM_ZOOMOPTIONS, TEXT("ZoomOptions"), CCommandManager::BindHandler(&CAppCommand::ZoomOptions, this)},
			{CM_ASPECTRATIO_TOGGLE, TEXT("AspectRatio"), CCommandManager::BindHandler(&CAppCommand::ToggleAspectRatio, this)},
		});

	RegisterCommands(
		{
			{CM_ASPECTRATIO_DEFAULT,    TEXT("AspectDefault")},
			{CM_ASPECTRATIO_16x9,       TEXT("Aspect16x9")},
			{CM_ASPECTRATIO_LETTERBOX,  TEXT("Letterbox")},
			{CM_ASPECTRATIO_WINDOWBOX,  TEXT("Windowbox")},
			{CM_ASPECTRATIO_PILLARBOX,  TEXT("Pillarbox")},
			{CM_ASPECTRATIO_4x3,        TEXT("Aspect4x3")},
			{CM_ASPECTRATIO_32x9,       TEXT("Aspect32x9")},
			{CM_ASPECTRATIO_16x9_LEFT,  TEXT("Aspect16x9Left")},
			{CM_ASPECTRATIO_16x9_RIGHT, TEXT("Aspect16x9Right")},
		},
		CCommandManager::BindHandler(&CAppCommand::SelectAspectRatio, this));

	RegisterCommands(
		{
			{
				CM_PANANDSCAN_PRESET_FIRST, CM_PANANDSCAN_PRESET_LAST, CM_PANANDSCAN_PRESET_FIRST + 9,
				TEXT("PanAndScan"),
				CCommandManager::BindHandler(&CAppCommand::SelectPanAndScanPreset, this)
			},
			{CM_PANANDSCANOPTIONS, TEXT("PanAndScanOptions"), CCommandManager::BindHandler(&CAppCommand::PanAndScanOptions, this)},
			{CM_FRAMECUT, TEXT("FrameCut"), CCommandManager::BindHandler(&CAppCommand::ToggleFrameCut, this)},
			{CM_FULLSCREEN, TEXT("Fullscreen"), CCommandManager::BindHandler(&CAppCommand::ToggleFullscreen, this)},
			{CM_ALWAYSONTOP, TEXT("AlwaysOnTop"), CCommandManager::BindHandler(&CAppCommand::ToggleAlwaysOnTop, this)},
			{CM_VOLUME_UP, TEXT("VolumeUp"), CCommandManager::BindHandler(&CAppCommand::VolumeUpDown, this)},
			{CM_VOLUME_DOWN, TEXT("VolumeDown"), CCommandManager::BindHandler(&CAppCommand::VolumeUpDown, this)},
			{CM_VOLUME_MUTE, TEXT("Mute"), CCommandManager::BindHandler(&CAppCommand::ToggleMute, this)},
		});

	RegisterCommands(
		{
			{CM_DUALMONO_BOTH, TEXT("Stereo")},
			{CM_DUALMONO_MAIN, TEXT("StereoLeft")},
			{CM_DUALMONO_SUB,  TEXT("StereoRight")},
		},
		CCommandManager::BindHandler(&CAppCommand::DualMonoMode, this));

	RegisterCommands(
		{
			{CM_SWITCHAUDIO, TEXT("SwitchAudio"), CCommandManager::BindHandler(&CAppCommand::SwitchAudio, this)},
		});

	RegisterCommands(
		{
			{CM_AUDIOGAIN_NONE, TEXT("VolumeNormalizeNone")},
			{CM_AUDIOGAIN_125,  TEXT("VolumeNormalize125")},
			{CM_AUDIOGAIN_150,  TEXT("VolumeNormalize150")},
			{CM_AUDIOGAIN_200,  TEXT("VolumeNormalize200")},
		},
		CCommandManager::BindHandler(&CAppCommand::SelectAudioGain, this));

	RegisterCommands(
		{
			{CM_SURROUNDAUDIOGAIN_NONE, TEXT("SurroundGainNone")},
			{CM_SURROUNDAUDIOGAIN_125,  TEXT("SurroundGain125")},
			{CM_SURROUNDAUDIOGAIN_150,  TEXT("SurroundGain150")},
			{CM_SURROUNDAUDIOGAIN_200,  TEXT("SurroundGain200")},
		},
		CCommandManager::BindHandler(&CAppCommand::SelectSurroundAudioGain, this));

	RegisterCommands(
		{
			{CM_AUDIODELAY_MINUS, TEXT("AudioDelayMinus")},
			{CM_AUDIODELAY_PLUS,  TEXT("AudioDelayPlus")},
			{CM_AUDIODELAY_RESET, TEXT("AudioDelayReset")},
		},
		CCommandManager::BindHandler(&CAppCommand::AudioDelay, this));

	RegisterCommands(
		{
			{CM_SPDIF_DISABLED,    TEXT("SpdifDisabled")},
			{CM_SPDIF_PASSTHROUGH, TEXT("SpdifPassthrough")},
			{CM_SPDIF_AUTO,        TEXT("SpdifAuto")},
		},
		CCommandManager::BindHandler(&CAppCommand::SelectSPDIFMode, this));

	RegisterCommands(
		{
			{CM_SPDIF_TOGGLE, TEXT("SpdifToggle"), CCommandManager::BindHandler(&CAppCommand::ToggleSPDIFMode, this)},
		});

	RegisterCommands(
		{
			{CM_RECORD,       TEXT("Record")},
			{CM_RECORD_START, TEXT("RecordStart")},
			{CM_RECORD_STOP,  TEXT("RecordStop")},
		},
		CCommandManager::BindHandler(&CAppCommand::Record, this));

	RegisterCommands(
		{
			{CM_RECORD_PAUSE, TEXT("RecordPause"), CCommandManager::BindHandler(&CAppCommand::RecordPauseResume, this)},
			{CM_RECORDEVENT, TEXT("RecordEvent"), CCommandManager::BindHandler(&CAppCommand::RecordEvent, this)},
			{CM_RECORDOPTION, TEXT("RecordOption"), CCommandManager::BindHandler(&CAppCommand::RecordOptions, this)},
			{CM_TIMESHIFTRECORDING, TEXT("TimeShiftRecording"), CCommandManager::BindHandler(&CAppCommand::StartTimeShiftRecording, this)},
			{CM_ENABLETIMESHIFTRECORDING, TEXT("EnableTimeShiftRecording"), CCommandManager::BindHandler(&CAppCommand::EnableTimeShiftRecording, this)},
			{CM_STATUSBARRECORD, nullptr, CCommandManager::BindHandler(&CAppCommand::StatusBarRecord, this)},
			{CM_EXITONRECORDINGSTOP, nullptr, CCommandManager::BindHandler(&CAppCommand::ExitOnRecordingStop, this)},
			{CM_OPTIONS_RECORD, nullptr, CCommandManager::BindHandler(&CAppCommand::OptionsRecordPage, this)},

			{CM_DISABLEVIEWER, TEXT("DisableViewer"), MainWindowHandler},

			{CM_COPYIMAGE, TEXT("CopyImage"), CCommandManager::BindHandler(&CAppCommand::CaptureCopySave, this)},
			{CM_SAVEIMAGE, TEXT("SaveImage"), CCommandManager::BindHandler(&CAppCommand::CaptureCopySave, this)},
			{CM_CAPTURE, TEXT("CaptureImage"), CCommandManager::BindHandler(&CAppCommand::DefaultCapture, this)},
			{CM_CAPTUREPREVIEW, TEXT("CapturePreview"), CCommandManager::BindHandler(&CAppCommand::CapturePreview, this)},
			{CM_CAPTUREOPTIONS, nullptr, CCommandManager::BindHandler(&CAppCommand::CaptureOptions, this)},
			{CM_OPENCAPTUREFOLDER, nullptr, CCommandManager::BindHandler(&CAppCommand::OpenCaptureFolder, this)},
			{
				CM_CAPTURESIZE_FIRST, CM_CAPTURESIZE_LAST, 0,
				nullptr, CCommandManager::BindHandler(&CAppCommand::CaptureSize, this)
			},

			{CM_RESET, TEXT("Reset"), CCommandManager::BindHandler(&CAppCommand::Reset, this)},
			{CM_RESETVIEWER, TEXT("ResetViewer"), CCommandManager::BindHandler(&CAppCommand::ResetViewer, this)},
			{CM_REBUILDVIEWER, TEXT("RebuildViewer"), MainWindowHandler},

			{CM_CHANNEL_UP, TEXT("ChannelUp"), CCommandManager::BindHandler(&CAppCommand::ChannelUpDown, this)},
			{CM_CHANNEL_DOWN, TEXT("ChannelDown"), CCommandManager::BindHandler(&CAppCommand::ChannelUpDown, this)},
			{CM_CHANNEL_BACKWARD, TEXT("ChannelBackward"), CCommandManager::BindHandler(&CAppCommand::ChannelBackwardForward, this)},
			{CM_CHANNEL_FORWARD, TEXT("ChannelForward"), CCommandManager::BindHandler(&CAppCommand::ChannelBackwardForward, this)},
			{CM_CHANNEL_PREVIOUS, TEXT("ChannelPrevious"), CCommandManager::BindHandler(&CAppCommand::ChannelPrevious, this)},
#ifdef _DEBUG
			{CM_UPDATECHANNELLIST, nullptr, CCommandManager::BindHandler(&CAppCommand::UpdateChannelList, this)},
#endif
		});

	RegisterCommands(
		{
			{CM_PANEL, TEXT("Panel")},
			{CM_PROGRAMGUIDE, TEXT("ProgramGuide")},
			{CM_TITLEBAR, TEXT("TitleBar")},
			{CM_POPUPTITLEBAR, TEXT("PopupTitleBar")},
			{CM_STATUSBAR, TEXT("StatusBar")},
			{CM_POPUPSTATUSBAR, TEXT("PopupStatusBar")},
			{CM_SIDEBAR, TEXT("SideBar")},
			{CM_WINDOWFRAME_NORMAL, CM_WINDOWFRAME_NONE, 0, nullptr},
			{CM_CUSTOMTITLEBAR, nullptr},
			{CM_SPLITTITLEBAR, nullptr},
		},
		MainWindowHandler);

	RegisterCommands(
		{
			{CM_OPTIONS, TEXT("Options"), CCommandManager::BindHandler(&CAppCommand::Options, this)},
		});

	RegisterCommands(
		{
			{CM_VIDEODECODERPROPERTY,  TEXT("VideoDecoderProperty")},
			{CM_VIDEORENDERERPROPERTY, TEXT("VideoRendererProperty")},
			{CM_AUDIOFILTERPROPERTY,   TEXT("AudioFilterProperty")},
			{CM_AUDIORENDERERPROPERTY, TEXT("AudioRendererProperty")},
			{CM_DEMULTIPLEXERPROPERTY, TEXT("DemuxerProperty")},
		},
		MainWindowHandler);

	RegisterCommands(
		{
			{CM_STREAMINFO, TEXT("StreamInfo"), CCommandManager::BindHandler(&CAppCommand::StreamInfo, this)},

			{CM_CLOSE, TEXT("Close"), CCommandManager::BindHandler(&CAppCommand::Close, this)},
			{CM_EXIT, TEXT("Exit"), CCommandManager::BindHandler(&CAppCommand::Exit, this)},
			{CM_SHOW, TEXT("Show"), MainWindowHandler},

			{CM_MENU, TEXT("Menu"), MainWindowHandler},

			{CM_ACTIVATE, TEXT("Activate"), MainWindowHandler},
			{CM_MINIMIZE, TEXT("Minimize"), MainWindowHandler},
			{CM_MAXIMIZE, TEXT("Maximize"), MainWindowHandler},

			{CM_1SEGMODE, TEXT("OneSegMode"), CCommandManager::BindHandler(&CAppCommand::ToggleOneSegMode, this)},
			{CM_CLOSETUNER, TEXT("CloseTuner"), CCommandManager::BindHandler(&CAppCommand::CloseTuner, this)},

			{CM_HOMEDISPLAY, TEXT("HomeDisplay"), MainWindowHandler},
			{CM_CHANNELDISPLAY, TEXT("ChannelDisplayMenu"), MainWindowHandler},

			{CM_ENABLEBUFFERING, TEXT("Buffering"), CCommandManager::BindHandler(&CAppCommand::EnableBuffering, this)},
			{CM_RESETBUFFER, TEXT("ResetBuffer"), CCommandManager::BindHandler(&CAppCommand::ResetBuffer, this)},
			{CM_RESETERRORCOUNT, TEXT("ResetErrorCount"), CCommandManager::BindHandler(&CAppCommand::ResetErrorCount, this)},
			{CM_SHOWRECORDREMAINTIME, nullptr, CCommandManager::BindHandler(&CAppCommand::ShowRecordRemainTime, this)},
			{CM_SHOWTOTTIME, TEXT("ShowTOTTime"), CCommandManager::BindHandler(&CAppCommand::ShowTOTTime, this)},
			{CM_INTERPOLATETOTTIME, nullptr, CCommandManager::BindHandler(&CAppCommand::InterpolateTOTTime, this)},
			{CM_ADJUSTTOTTIME, TEXT("AdjustTOTTime"), CCommandManager::BindHandler(&CAppCommand::AdjustTOTTime, this)},
			{CM_PROGRAMINFOSTATUS_POPUPINFO, nullptr, CCommandManager::BindHandler(&CAppCommand::ProgramInfoStatus_PopupInfo, this)},
			{CM_PROGRAMINFOSTATUS_SHOWPROGRESS, nullptr, CCommandManager::BindHandler(&CAppCommand::ProgramInfoStatus_ShowProgress, this)},
		});

	RegisterCommands(
		{
			{CM_ZOOMMENU,           TEXT("ZoomMenu")},
			{CM_ASPECTRATIOMENU,    TEXT("AspectRatioMenu")},
			{CM_CHANNELMENU,        TEXT("ChannelMenu")},
			{CM_SERVICEMENU,        TEXT("ServiceMenu")},
			{CM_TUNINGSPACEMENU,    TEXT("TuningSpaceMenu")},
			{CM_FAVORITESMENU,      TEXT("FavoritesMenu")},
			{CM_RECENTCHANNELMENU,  TEXT("RecentChannelMenu")},
			{CM_VOLUMEMENU,         TEXT("VolumeMenu")},
			{CM_AUDIOMENU,          TEXT("AudioMenu")},
			{CM_VIDEOMENU,          TEXT("VideoMenu")},
			{CM_RESETMENU,          TEXT("ResetMenu")},
			{CM_BARMENU,            TEXT("BarMenu")},
			{CM_PLUGINMENU,         TEXT("PluginMenu")},
			{CM_FILTERPROPERTYMENU, TEXT("FilterPropertyMenu")}
		},
		MainWindowHandler);

	RegisterCommands(
		{
			{CM_ADDTOFAVORITES, TEXT("AddToFavorites"), CCommandManager::BindHandler(&CAppCommand::AddToFavorites, this)},
			{CM_ORGANIZEFAVORITES, TEXT("OrganizeFavorites"), CCommandManager::BindHandler(&CAppCommand::OrganizeFavorites, this)},

			{
				CM_CHANNELNO_1, CM_CHANNELNO_LAST, CM_CHANNELNO_12,
				TEXT("Channel"), CCommandManager::BindHandler(&CAppCommand::SelectChannelNo, this)
			},
			{
				CM_CHANNEL_FIRST, CM_CHANNEL_LAST, 0,
				nullptr, CCommandManager::BindHandler(&CAppCommand::SelectChannel, this)
			},
			{
				CM_SERVICE_FIRST, CM_SERVICE_LAST, CM_SERVICE_FIRST + 7,
				TEXT("Service"), CCommandManager::BindHandler(&CAppCommand::SelectService, this)
			},
			{
				CM_SPACE_FIRST, CM_SPACE_LAST, CM_SPACE_FIRST + 4,
				TEXT("TuningSpace"), CCommandManager::BindHandler(&CAppCommand::SelectSpace, this)
			},

			{CM_CHANNELNO_2DIGIT, TEXT("ChannelNo2Digit"), MainWindowHandler},
			{CM_CHANNELNO_3DIGIT, TEXT("ChannelNo3Digit"), MainWindowHandler},

			{CM_SWITCHVIDEO, TEXT("SwitchVideo"), CCommandManager::BindHandler(&CAppCommand::SwitchVideo, this)},
			{CM_VIDEOSTREAM_SWITCH, TEXT("SwitchVideoStream"), CCommandManager::BindHandler(&CAppCommand::VideoStreamSwitch, this)},
			{CM_MULTIVIEW_SWITCH, TEXT("SwitchMultiView"), CCommandManager::BindHandler(&CAppCommand::MultiViewSwitch, this)},

			{
				CM_SIDEBAR_PLACE_FIRST, CM_SIDEBAR_PLACE_LAST, 0,
				nullptr, MainWindowHandler
			},
			{CM_SIDEBAROPTIONS, nullptr, CCommandManager::BindHandler(&CAppCommand::SideBarOptions, this)},

			{CM_DRIVER_BROWSE, nullptr, CCommandManager::BindHandler(&CAppCommand::DriverBrowse, this)},

			{
				CM_SPACE_CHANNEL_FIRST, CM_SPACE_CHANNEL_LAST, 0,
				nullptr, CCommandManager::BindHandler(&CAppCommand::SelectSpaceChannel, this)
			},
			{
				CM_CHANNELHISTORY_FIRST, CM_CHANNELHISTORY_LAST, 0,
				nullptr, CCommandManager::BindHandler(&CAppCommand::ChannelHistory, this)
			},
			{CM_CHANNELHISTORY_CLEAR, nullptr, CCommandManager::BindHandler(&CAppCommand::ChannelHistoryClear, this)},
			{
				CM_FAVORITECHANNEL_FIRST, CM_FAVORITECHANNEL_LAST, 0,
				nullptr, CCommandManager::BindHandler(&CAppCommand::FavoriteChannel, this)
			},

			{
				CM_VIDEOSTREAM_FIRST, CM_VIDEOSTREAM_LAST, 0,
				nullptr, CCommandManager::BindHandler(&CAppCommand::VideoStream, this)
			},
			{
				CM_AUDIOSTREAM_FIRST, CM_AUDIOSTREAM_LAST, 0,
				nullptr, CCommandManager::BindHandler(&CAppCommand::AudioStream, this)
			},
			{
				CM_AUDIO_FIRST, CM_AUDIO_LAST, 0,
				nullptr, CCommandManager::BindHandler(&CAppCommand::SelectAudio, this)
			},
			{
				CM_MULTIVIEW_FIRST, CM_MULTIVIEW_LAST, 0,
				nullptr, CCommandManager::BindHandler(&CAppCommand::MultiView, this)
			},

			{
				CM_PANEL_FIRST, CM_PANEL_LAST, 0,
				nullptr, CCommandManager::BindHandler(&CAppCommand::SelectPanelPage, this)
			},

			{CM_EVENTINFOOSD, TEXT("EventInfoOSD"), CCommandManager::BindHandler(&CAppCommand::ToggleEventInfoOSD, this)},
		});
}


void CAppCommand::RegisterDynamicCommands()
{
	// BonDriver
	for (int i = 0; i < m_App.DriverManager.NumDrivers() && CM_DRIVER_FIRST + i <= CM_DRIVER_LAST; i++) {
		const CDriverInfo *pDriverInfo = m_App.DriverManager.GetDriverInfo(i);
		LPCTSTR pszFileName = ::PathFindFileName(pDriverInfo->GetFileName());
		TCHAR szText[CCommandManager::MAX_COMMAND_TEXT];

		StringFormat(szText, TEXT("BonDriver切替 : {}"), pszFileName);
		m_App.CommandManager.RegisterCommand(
			CM_DRIVER_FIRST + i, pszFileName,
			CCommandManager::BindHandler(&CAppCommand::SelectDriver, this),
			szText);
	}

	// プラグイン
	for (int i = 0; i < m_App.PluginManager.NumPlugins(); i++) {
		const CPlugin *pPlugin = m_App.PluginManager.GetPlugin(i);
		TCHAR szText[CCommandManager::MAX_COMMAND_TEXT];

		StringFormat(
			szText,
			TEXT("プラグイン有効/無効 : {}"), pPlugin->GetPluginName());
		/*
		TCHAR szShortText[CCommandManager::MAX_COMMAND_TEXT];
		StringFormat(
			szShortText,
			TEXT("{} 有効/無効"), pPlugin->GetPluginName());
		*/
		m_App.CommandManager.RegisterCommand(
			CM_PLUGIN_FIRST + i,
			::PathFindFileName(pPlugin->GetFileName()),
			CCommandManager::BindHandler(&CAppCommand::EnablePlugin, this),
			szText,
			/*szShortName*/pPlugin->GetPluginName());
	}

	// プラグインの各コマンド
	int ID = CM_PLUGINCOMMAND_FIRST;
	for (int i = 0; i < m_App.PluginManager.NumPlugins(); i++) {
		CPlugin *pPlugin = m_App.PluginManager.GetPlugin(i);

		if (pPlugin->GetIcon().IsCreated())
			m_App.SideBarOptions.RegisterCommand(pPlugin->GetCommand());

		LPCTSTR pszFileName = ::PathFindFileName(pPlugin->GetFileName());
		String IDText;

		for (int j = 0; j < pPlugin->NumPluginCommands(); j++) {
			CPlugin::CPluginCommandInfo *pInfo = pPlugin->GetPluginCommandInfo(j);

			pInfo->SetCommand(ID);

			IDText = pszFileName;
			IDText += _T(':');
			IDText += pInfo->GetText();

			TCHAR szText[CCommandManager::MAX_COMMAND_TEXT];
			StringFormat(
				szText, TEXT("{} : {}"),
				pPlugin->GetPluginName(), pInfo->GetName());

			CCommandManager::CommandState State = CCommandManager::CommandState::None;
			if ((pInfo->GetState() & PLUGIN_COMMAND_STATE_DISABLED) != 0)
				State |= CCommandManager::CommandState::Disabled;
			if ((pInfo->GetState() & PLUGIN_COMMAND_STATE_CHECKED) != 0)
				State |= CCommandManager::CommandState::Checked;

			m_App.CommandManager.RegisterCommand(
				ID, IDText.c_str(),
				CCommandManager::BindHandler(&CAppCommand::PluginCommand, this),
				szText, pInfo->GetName(), State);

			if ((pInfo->GetFlags() & PLUGIN_COMMAND_FLAG_ICONIZE) != 0)
				m_App.SideBarOptions.RegisterCommand(ID);

			ID++;
		}
	}
}


void CAppCommand::InitializeCommandState()
{
	m_App.UICore.SetCommandCheckedState(CM_ALWAYSONTOP, m_App.UICore.GetAlwaysOnTop());

	m_App.UICore.SetCommandCheckedState(
		CM_FRAMECUT, m_App.VideoOptions.GetStretchMode() == LibISDB::ViewerFilter::ViewStretchMode::Crop);

	int Gain, SurroundGain;
	m_App.CoreEngine.GetAudioGainControl(&Gain, &SurroundGain);
	for (int i = 0; i < lengthof(m_AudioGainList); i++) {
		if (Gain == m_AudioGainList[i]) {
			m_App.UICore.SetCommandRadioCheckedState(
				CM_AUDIOGAIN_FIRST,
				CM_AUDIOGAIN_LAST,
				CM_AUDIOGAIN_FIRST + i);
		}
		if (SurroundGain == m_AudioGainList[i]) {
			m_App.UICore.SetCommandRadioCheckedState(
				CM_SURROUNDAUDIOGAIN_FIRST,
				CM_SURROUNDAUDIOGAIN_LAST,
				CM_SURROUNDAUDIOGAIN_FIRST + i);
		}
	}

	m_App.UICore.SetCommandRadioCheckedState(
		CM_CAPTURESIZE_FIRST, CM_CAPTURESIZE_LAST,
		CM_CAPTURESIZE_FIRST + m_App.CaptureOptions.GetPresetCaptureSize());
	m_App.UICore.SetCommandCheckedState(CM_PANEL, m_App.Panel.fShowPanelWindow);
	m_App.UICore.SetCommandCheckedState(CM_1SEGMODE, m_App.Core.Is1SegMode());

	m_App.UICore.SetCommandCheckedState(
		CM_SPDIF_TOGGLE,
		m_App.CoreEngine.IsSPDIFPassthroughEnabled());
}


void CAppCommand::RegisterCommands(std::initializer_list<CommandParameters> List)
{
	for (const auto &e : List) {
		m_App.CommandManager.RegisterCommand(
			e.FirstID, e.LastID, e.LastListingID,
			e.pszIDText,
			e.Handler,
			e.pszText, e.pszShortText,
			e.State);
	}
}


void CAppCommand::RegisterCommands(
	std::initializer_list<CommandParameters> List,
	const CCommandManager::CommandHandler &Handler)
{
	for (const auto &e : List) {
		m_App.CommandManager.RegisterCommand(
			e.FirstID, e.LastID, e.LastListingID,
			e.pszIDText,
			Handler,
			e.pszText, e.pszShortText,
			e.State);
	}
}


bool CAppCommand::ZoomOptions(CCommandManager::InvokeParameters &Params)
{
	if (m_App.ZoomOptions.Show(m_App.UICore.GetDialogOwner())) {
		m_App.SideBarOptions.SetSideBarImage();
		m_App.ZoomOptions.SaveSettings(m_App.GetIniFileName());
	}

	return true;
}


bool CAppCommand::ToggleAspectRatio(CCommandManager::InvokeParameters &Params)
{
	int AspectRatioType = m_App.UICore.GetAspectRatioType();

	if (AspectRatioType >= CUICore::ASPECTRATIO_CUSTOM_FIRST) {
		AspectRatioType = CUICore::ASPECTRATIO_DEFAULT;
	} else if (AspectRatioType < CUICore::ASPECTRATIO_3D_FIRST) {
		AspectRatioType = (AspectRatioType + 1) % (CUICore::ASPECTRATIO_2D_LAST - CUICore::ASPECTRATIO_FIRST + 1);
	} else {
		AspectRatioType = CUICore::ASPECTRATIO_3D_FIRST +
			(AspectRatioType - CUICore::ASPECTRATIO_3D_FIRST + 1) %
			(CUICore::ASPECTRATIO_3D_LAST - CUICore::ASPECTRATIO_3D_FIRST + 1);
	}

	if (m_App.UICore.SetAspectRatioType(AspectRatioType)) {
		m_App.UICore.SetCommandRadioCheckedState(
			CM_ASPECTRATIO_FIRST, CM_ASPECTRATIO_3D_LAST,
			AspectRatioType < CUICore::ASPECTRATIO_CUSTOM_FIRST ? CM_ASPECTRATIO_FIRST + AspectRatioType : 0);
	}

	return true;
}


bool CAppCommand::SelectAspectRatio(CCommandManager::InvokeParameters &Params)
{
	if (m_App.UICore.SetAspectRatioType(Params.ID - CM_ASPECTRATIO_FIRST)) {
		m_App.UICore.SetCommandRadioCheckedState(
			CM_ASPECTRATIO_FIRST, CM_ASPECTRATIO_3D_LAST, Params.ID);
	}

	return true;
}


bool CAppCommand::SelectPanAndScanPreset(CCommandManager::InvokeParameters &Params)
{
	if (m_App.UICore.SetAspectRatioType(CUICore::ASPECTRATIO_CUSTOM_FIRST + (Params.ID - CM_PANANDSCAN_PRESET_FIRST))) {
		m_App.UICore.SetCommandRadioCheckedState(
			CM_ASPECTRATIO_FIRST, CM_ASPECTRATIO_3D_LAST, 0);
	}

	return true;
}


bool CAppCommand::PanAndScanOptions(CCommandManager::InvokeParameters &Params)
{
	const int AspectRatioType = m_App.UICore.GetAspectRatioType();
	bool fSet = false;
	CPanAndScanOptions::PanAndScanInfo CurPanScan;

	if (AspectRatioType >= CUICore::ASPECTRATIO_CUSTOM_FIRST)
		fSet = m_App.PanAndScanOptions.GetPreset(AspectRatioType - CUICore::ASPECTRATIO_CUSTOM_FIRST, &CurPanScan);

	if (m_App.PanAndScanOptions.Show(m_App.UICore.GetDialogOwner())) {
		if (fSet) {
			const int Index = m_App.PanAndScanOptions.FindPresetByID(CurPanScan.ID);
			CPanAndScanOptions::PanAndScanInfo NewPanScan;

			if (Index >= 0 && m_App.PanAndScanOptions.GetPreset(Index, &NewPanScan)) {
				if (NewPanScan.Info != CurPanScan.Info) {
					// パンスキャンの設定が変更された
					m_App.CommandManager.InvokeCommand(CM_PANANDSCAN_PRESET_FIRST + Index);
				}
			} else {
				// パンスキャンの設定が削除された
				m_App.CommandManager.InvokeCommand(CM_ASPECTRATIO_DEFAULT);
			}
		}

		m_App.PanAndScanOptions.SaveSettings(m_App.GetIniFileName());
	}

	return true;
}


bool CAppCommand::ToggleFrameCut(CCommandManager::InvokeParameters &Params)
{
	LibISDB::ViewerFilter::ViewStretchMode Mode;
	if (m_App.VideoOptions.GetStretchMode() != LibISDB::ViewerFilter::ViewStretchMode::Crop)
		Mode = LibISDB::ViewerFilter::ViewStretchMode::Crop;
	else
		Mode = LibISDB::ViewerFilter::ViewStretchMode::KeepAspectRatio;
	m_App.VideoOptions.SetStretchMode(Mode);

	LibISDB::ViewerFilter *pViewer = m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();
	if (pViewer != nullptr) {
		pViewer->SetViewStretchMode(Mode);
	}

	m_App.UICore.SetCommandCheckedState(
		CM_FRAMECUT, Mode == LibISDB::ViewerFilter::ViewStretchMode::Crop);

	return true;
}


bool CAppCommand::ToggleFullscreen(CCommandManager::InvokeParameters &Params)
{
	m_App.UICore.ToggleFullscreen();

	return true;
}


bool CAppCommand::ToggleAlwaysOnTop(CCommandManager::InvokeParameters &Params)
{
	m_App.UICore.SetAlwaysOnTop(!m_App.UICore.GetAlwaysOnTop());

	return true;
}


bool CAppCommand::VolumeUpDown(CCommandManager::InvokeParameters &Params)
{
	const int CurVolume = m_App.UICore.GetVolume();
	int Volume = CurVolume;

	if (Params.ID == CM_VOLUME_UP) {
		Volume += m_App.OperationOptions.GetVolumeStep();
		if (Volume > CCoreEngine::MAX_VOLUME)
			Volume = CCoreEngine::MAX_VOLUME;
	} else {
		Volume -= m_App.OperationOptions.GetVolumeStep();
		if (Volume < 0)
			Volume = 0;
	}
	if ((Volume != CurVolume) || m_App.UICore.GetMute())
		m_App.UICore.SetVolume(Volume);

	return true;
}


bool CAppCommand::ToggleMute(CCommandManager::InvokeParameters &Params)
{
	m_App.UICore.SetMute(!m_App.UICore.GetMute());

	return true;
}


bool CAppCommand::SelectAudioGain(CCommandManager::InvokeParameters &Params)
{
	int SurroundGain;

	m_App.CoreEngine.GetAudioGainControl(nullptr, &SurroundGain);
	m_App.CoreEngine.SetAudioGainControl(
		m_AudioGainList[Params.ID - CM_AUDIOGAIN_FIRST], SurroundGain);
	m_App.UICore.SetCommandRadioCheckedState(CM_AUDIOGAIN_NONE, CM_AUDIOGAIN_LAST, Params.ID);

	return true;
}


bool CAppCommand::SelectSurroundAudioGain(CCommandManager::InvokeParameters &Params)
{
	int Gain;

	m_App.CoreEngine.GetAudioGainControl(&Gain, nullptr);
	m_App.CoreEngine.SetAudioGainControl(
		Gain, m_AudioGainList[Params.ID - CM_SURROUNDAUDIOGAIN_FIRST]);
	m_App.UICore.SetCommandRadioCheckedState(CM_SURROUNDAUDIOGAIN_NONE, CM_SURROUNDAUDIOGAIN_LAST, Params.ID);

	return true;
}


bool CAppCommand::AudioDelay(CCommandManager::InvokeParameters &Params)
{
	LibISDB::ViewerFilter *pViewer = m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();
	if (pViewer == nullptr)
		return true;

	static constexpr LONGLONG MaxDelay = 10000000LL;
	const LONGLONG Step = m_App.OperationOptions.GetAudioDelayStep() * 10000LL;
	LONGLONG Delay;

	switch (Params.ID) {
	case CM_AUDIODELAY_MINUS:
		Delay = pViewer->GetAudioDelay() - Step;
		break;
	case CM_AUDIODELAY_PLUS:
		Delay = pViewer->GetAudioDelay() + Step;
		break;
	case CM_AUDIODELAY_RESET:
		Delay = 0;
		break;
	}

	pViewer->SetAudioDelay(std::clamp(Delay, -MaxDelay, MaxDelay));

	return true;
}


bool CAppCommand::DualMonoMode(CCommandManager::InvokeParameters &Params)
{
	m_App.UICore.SetDualMonoMode(
		Params.ID == CM_DUALMONO_MAIN ?
			LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Main :
		Params.ID == CM_DUALMONO_SUB ?
			LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Sub :
			LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Both);
	m_App.UICore.ShowAudioOSD();

	return true;
}


bool CAppCommand::SwitchAudio(CCommandManager::InvokeParameters &Params)
{
	m_App.UICore.SwitchAudio();
	m_App.UICore.ShowAudioOSD();

	return true;
}


bool CAppCommand::SelectSPDIFMode(CCommandManager::InvokeParameters &Params)
{
	LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions Options(m_App.AudioOptions.GetSpdifOptions());

	Options.Mode = static_cast<LibISDB::DirectShow::AudioDecoderFilter::SPDIFMode>(Params.ID - CM_SPDIF_DISABLED);
	m_App.CoreEngine.SetSPDIFOptions(Options);
	m_App.UICore.SetCommandCheckedState(
		CM_SPDIF_TOGGLE, m_App.CoreEngine.IsSPDIFPassthroughEnabled());

	return true;
}


bool CAppCommand::ToggleSPDIFMode(CCommandManager::InvokeParameters &Params)
{
	LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions Options;

	m_App.CoreEngine.GetSPDIFOptions(&Options);
	if (m_App.CoreEngine.IsSPDIFPassthroughEnabled())
		Options.Mode = LibISDB::DirectShow::AudioDecoderFilter::SPDIFMode::Disabled;
	else
		Options.Mode = LibISDB::DirectShow::AudioDecoderFilter::SPDIFMode::Passthrough;
	m_App.CoreEngine.SetSPDIFOptions(Options);
	m_App.UICore.SetCommandCheckedState(
		CM_SPDIF_TOGGLE, m_App.CoreEngine.IsSPDIFPassthroughEnabled());

	return true;
}


bool CAppCommand::DefaultCapture(CCommandManager::InvokeParameters &Params)
{
	m_App.CommandManager.InvokeCommand(m_App.CaptureOptions.TranslateCommand(CM_CAPTURE));

	return true;
}


bool CAppCommand::CaptureCopySave(CCommandManager::InvokeParameters &Params)
{
	if (!m_App.UICore.IsViewerEnabled())
		return true;

	const HCURSOR hcurOld = ::SetCursor(::LoadCursor(nullptr, IDC_WAIT));
	LibISDB::COMMemoryPointer<> Image(m_App.CoreEngine.GetCurrentImage());

	if (!Image) {
		::SetCursor(hcurOld);
		m_App.UICore.GetSkin()->ShowMessage(
			TEXT("現在の画像を取得できません。\n")
			TEXT("レンダラやデコーダを変えてみてください。"),
			TEXT("画像キャプチャ"),
			MB_OK | MB_ICONEXCLAMATION);
		return true;
	}

	const LibISDB::ViewerFilter *pViewer = m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();
	const LibISDB::DirectShow::VideoRenderer::RendererType VideoRenderer = pViewer->GetVideoRendererType();
	const std::uint8_t *pDib = Image.get();
	const BITMAPINFOHEADER *pbmih = reinterpret_cast<const BITMAPINFOHEADER*>(pDib);
	const int OrigWidth = pbmih->biWidth;
	const int OrigHeight = std::abs(pbmih->biHeight);
	RECT rc;

	// EVR は表示されている画像がそのまま取得される
	if (VideoRenderer != LibISDB::DirectShow::VideoRenderer::RendererType::EVR
			&& VideoRenderer != LibISDB::DirectShow::VideoRenderer::RendererType::EVRCustomPresenter
			&& pViewer->GetSourceRect(&rc)) {
		int VideoWidth, VideoHeight;

		if (pViewer->GetOriginalVideoSize(&VideoWidth, &VideoHeight)
				&& (VideoWidth != OrigWidth
					|| VideoHeight != OrigHeight)) {
			rc.left = rc.left * OrigWidth / VideoWidth;
			rc.top = rc.top * OrigHeight / VideoHeight;
			rc.right = rc.right * OrigWidth / VideoWidth;
			rc.bottom = rc.bottom * OrigHeight / VideoHeight;
		}
		if (rc.right > OrigWidth)
			rc.right = OrigWidth;
		if (rc.bottom > OrigHeight)
			rc.bottom = OrigHeight;
	} else {
		rc.left = 0;
		rc.top = 0;
		rc.right = OrigWidth;
		rc.bottom = OrigHeight;
	}

	if (OrigHeight == 1088) {
		rc.top = rc.top * 1080 / 1088;
		rc.bottom = rc.bottom * 1080 / 1088;
	}

	int Width, Height;

	switch (m_App.CaptureOptions.GetCaptureSizeType()) {
	case CCaptureOptions::SIZE_TYPE_ORIGINAL:
		m_App.CoreEngine.GetVideoViewSize(&Width, &Height);
		break;

	case CCaptureOptions::SIZE_TYPE_VIEW:
		pViewer->GetDestSize(&Width, &Height);
		break;

	/*
	case CCaptureOptions::SIZE_RAW:
		rc.left = rc.top = 0;
		rc.right = OrigWidth;
		rc.bottom = OrigHeight;
		Width = OrigWidth;
		Height = OrigHeight;
		break;
	*/

	case CCaptureOptions::SIZE_TYPE_PERCENTAGE:
		{
			int Num, Denom;

			m_App.CoreEngine.GetVideoViewSize(&Width, &Height);
			m_App.CaptureOptions.GetSizePercentage(&Num, &Denom);
			Width = Width * Num / Denom;
			Height = Height * Num / Denom;
		}
		break;

	case CCaptureOptions::SIZE_TYPE_CUSTOM:
		m_App.CaptureOptions.GetCustomSize(&Width, &Height);
		break;
	}

	const HGLOBAL hGlobal = ResizeImage(
		reinterpret_cast<const BITMAPINFO*>(pbmih),
		pDib + CalcDIBInfoSize(pbmih), &rc, Width, Height);
	Image.reset();
	::SetCursor(hcurOld);
	if (hGlobal == nullptr) {
		return true;
	}

	CCaptureImage *pImage = new CCaptureImage(hGlobal);

	if (m_App.CaptureOptions.GetWriteComment()) {
		String Comment;
		if (m_App.CaptureOptions.GetCommentText(&Comment, pImage))
			pImage->SetComment(Comment.c_str());
	}

	m_App.CaptureWindow.SetImage(pImage);

	if (Params.ID == CM_COPYIMAGE) {
		if (!pImage->SetClipboard(m_App.UICore.GetMainWindow())) {
			m_App.UICore.GetSkin()->ShowErrorMessage(TEXT("クリップボードにデータを設定できません。"));
		}
	} else {
		if (!m_App.CaptureOptions.SaveImage(pImage)) {
			m_App.UICore.GetSkin()->ShowErrorMessage(TEXT("画像の保存でエラーが発生しました。"));
		}
	}

	if (!m_App.CaptureWindow.HasImage())
		delete pImage;

	return true;
}


bool CAppCommand::CapturePreview(CCommandManager::InvokeParameters &Params)
{
	if (!m_App.CaptureWindow.GetVisible()) {
		if (!m_App.CaptureWindow.IsCreated()) {
			m_App.CaptureWindow.Create(
				m_App.UICore.GetMainWindow(),
				WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_VISIBLE | WS_CLIPCHILDREN,
				WS_EX_TOOLWINDOW);
		} else {
			m_App.CaptureWindow.SetVisible(true);
		}
	} else {
		m_App.CaptureWindow.Destroy();
		m_App.CaptureWindow.ClearImage();
	}

	m_App.UICore.SetCommandCheckedState(CM_CAPTUREPREVIEW, m_App.CaptureWindow.GetVisible());

	return true;
}


bool CAppCommand::CaptureOptions(CCommandManager::InvokeParameters &Params)
{
	const HWND hwnd = m_App.UICore.GetDialogOwner();

	if (::IsWindowEnabled(hwnd))
		m_App.ShowOptionDialog(hwnd, COptionDialog::PAGE_CAPTURE);

	return true;
}


bool CAppCommand::OpenCaptureFolder(CCommandManager::InvokeParameters &Params)
{
	m_App.CaptureOptions.OpenSaveFolder();

	return true;
}


bool CAppCommand::CaptureSize(CCommandManager::InvokeParameters &Params)
{
	const int CaptureSize = Params.ID - CM_CAPTURESIZE_FIRST;

	m_App.CaptureOptions.SetPresetCaptureSize(CaptureSize);
	m_App.UICore.SetCommandRadioCheckedState(CM_CAPTURESIZE_FIRST, CM_CAPTURESIZE_LAST, Params.ID);

	return true;
}


bool CAppCommand::Reset(CCommandManager::InvokeParameters &Params)
{
	m_App.Core.ResetEngine();

	return true;
}


bool CAppCommand::ResetViewer(CCommandManager::InvokeParameters &Params)
{
	m_App.CoreEngine.ResetViewer();

	return true;
}


bool CAppCommand::Record(CCommandManager::InvokeParameters &Params)
{
	if (Params.ID == CM_RECORD) {
		if (m_App.RecordManager.IsPaused()) {
			m_App.CommandManager.InvokeCommand(CM_RECORD_PAUSE);
			return true;
		}
	} else if (Params.ID == CM_RECORD_START) {
		if (m_App.RecordManager.IsRecording()) {
			if (m_App.RecordManager.IsPaused())
				m_App.CommandManager.InvokeCommand(CM_RECORD_PAUSE);
			return true;
		}
	} else if (Params.ID == CM_RECORD_STOP) {
		if (!m_App.RecordManager.IsRecording())
			return true;
	}

	if (m_App.RecordManager.IsRecording()) {
		if (!m_App.RecordManager.IsPaused()
				&& !m_App.RecordOptions.ConfirmStop(m_App.UICore.GetDialogOwner()))
			return true;

		m_App.Core.StopRecord();
	} else {
		if (m_App.RecordManager.IsReserved()) {
			if (m_App.UICore.GetSkin()->ShowMessage(
						TEXT("既に設定されている録画があります。\n")
						TEXT("録画を開始すると既存の設定が破棄されます。\n")
						TEXT("録画を開始してもいいですか?"),
						TEXT("録画開始の確認"),
						MB_OKCANCEL | MB_ICONEXCLAMATION | MB_DEFBUTTON2) != IDOK) {
				return true;
			}
		}

		m_App.Core.StartRecord();
	}

	return true;
}


bool CAppCommand::RecordPauseResume(CCommandManager::InvokeParameters &Params)
{
	m_App.Core.PauseResumeRecording();

	return true;
}


bool CAppCommand::RecordOptions(CCommandManager::InvokeParameters &Params)
{
	const HWND hwnd = m_App.UICore.GetDialogOwner();

	if (!::IsWindowEnabled(hwnd))
		return true;

	if (m_App.RecordManager.IsRecording()) {
		if (m_App.RecordManager.RecordDialog(hwnd))
			m_App.StatusView.UpdateItem(STATUS_ITEM_RECORD);
	} else {
		if (IsStringEmpty(m_App.RecordManager.GetFileName())) {
			TCHAR szFileName[MAX_PATH];

			if (m_App.RecordOptions.GetFilePath(szFileName, MAX_PATH))
				m_App.RecordManager.SetFileName(szFileName);
		}

		if (m_App.RecordManager.RecordDialog(hwnd)) {
			m_App.RecordManager.SetClient(CRecordManager::RecordClient::User);
			if (m_App.RecordManager.IsReserved()) {
				m_App.StatusView.UpdateItem(STATUS_ITEM_RECORD);
			} else {
				m_App.Core.StartReservedRecord();
			}
		} else {
			// 予約がキャンセルされた場合も表示を更新する
			m_App.StatusView.UpdateItem(STATUS_ITEM_RECORD);
		}
	}

	return true;
}


bool CAppCommand::RecordEvent(CCommandManager::InvokeParameters &Params)
{
	if (m_App.RecordManager.IsRecording()) {
		m_App.RecordManager.SetStopOnEventEnd(!m_App.RecordManager.GetStopOnEventEnd());
	} else {
		m_App.CommandManager.InvokeCommand(CM_RECORD_START);
		if (m_App.RecordManager.IsRecording())
			m_App.RecordManager.SetStopOnEventEnd(true);
	}

	return true;
}


bool CAppCommand::ExitOnRecordingStop(CCommandManager::InvokeParameters &Params)
{
	m_App.Core.SetExitOnRecordingStop(!m_App.Core.GetExitOnRecordingStop());

	return true;
}


bool CAppCommand::OptionsRecordPage(CCommandManager::InvokeParameters &Params)
{
	const HWND hwnd = m_App.UICore.GetDialogOwner();

	if (::IsWindowEnabled(hwnd))
		m_App.ShowOptionDialog(hwnd, COptionDialog::PAGE_RECORD);

	return true;
}


bool CAppCommand::StartTimeShiftRecording(CCommandManager::InvokeParameters &Params)
{
	if (!m_App.RecordManager.IsRecording()) {
		if (m_App.RecordManager.IsReserved()) {
			if (m_App.UICore.GetSkin()->ShowMessage(
						TEXT("既に設定されている録画があります。\n")
						TEXT("録画を開始すると既存の設定が破棄されます。\n")
						TEXT("録画を開始してもいいですか?"),
						TEXT("録画開始の確認"),
						MB_OKCANCEL | MB_ICONEXCLAMATION | MB_DEFBUTTON2) != IDOK) {
				return true;
			}
		}

		m_App.Core.StartRecord(nullptr, nullptr, nullptr, CRecordManager::RecordClient::User, true);
	}

	return true;
}


bool CAppCommand::EnableTimeShiftRecording(CCommandManager::InvokeParameters &Params)
{
	m_App.RecordOptions.EnableTimeShiftRecording(!m_App.RecordOptions.IsTimeShiftRecordingEnabled());

	return true;
}


bool CAppCommand::StatusBarRecord(CCommandManager::InvokeParameters &Params)
{
	const int Command = m_App.RecordOptions.GetStatusBarRecordCommand();

	if (Command != 0)
		m_App.CommandManager.InvokeCommand(Command);

	return true;
}


bool CAppCommand::Options(CCommandManager::InvokeParameters &Params)
{
	const HWND hwndOwner = m_App.UICore.GetDialogOwner();

	if (hwndOwner == nullptr || ::IsWindowEnabled(hwndOwner))
		m_App.ShowOptionDialog(hwndOwner);

	return true;
}


bool CAppCommand::StreamInfo(CCommandManager::InvokeParameters &Params)
{
	if (!m_App.StreamInfo.IsCreated()) {
		m_App.StreamInfo.Create(m_App.UICore.GetMainWindow());
	} else {
		m_App.StreamInfo.Destroy();
	}

	m_App.UICore.SetCommandCheckedState(CM_STREAMINFO, m_App.StreamInfo.IsCreated());

	return true;
}


bool CAppCommand::Close(CCommandManager::InvokeParameters &Params)
{
	if (m_App.UICore.GetStandby()) {
		m_App.UICore.SetStandby(false);
	} else if (m_App.TaskTrayManager.GetResident()) {
		m_App.UICore.SetStandby(true, false);
	} else if (m_App.GeneralOptions.GetStandaloneProgramGuide()
			&& m_App.Epg.ProgramGuideFrame.GetVisible()) {
		m_App.UICore.SetStandby(true, true);
	} else {
		m_App.CommandManager.InvokeCommand(CM_EXIT);
	}

	return true;
}


bool CAppCommand::Exit(CCommandManager::InvokeParameters &Params)
{
	::PostMessage(m_App.UICore.GetMainWindow(), WM_CLOSE, 0, 0);

	return true;
}


bool CAppCommand::ChannelUpDown(CCommandManager::InvokeParameters &Params)
{
	const int Channel = m_App.UICore.GetNextChannel(Params.ID == CM_CHANNEL_UP);

	if (Channel >= 0)
		m_App.Core.SwitchChannel(Channel);

	return true;
}


bool CAppCommand::ChannelBackwardForward(CCommandManager::InvokeParameters &Params)
{
	const CTunerChannelInfo *pChannel =
		(Params.ID == CM_CHANNEL_BACKWARD) ?
			m_App.ChannelHistory.Backward() :
			m_App.ChannelHistory.Forward();

	if (pChannel != nullptr) {
		m_App.Core.SelectChannel(pChannel->GetTunerName(), *pChannel);
	}

	return true;
}


bool CAppCommand::ChannelPrevious(CCommandManager::InvokeParameters &Params)
{
	if (m_App.RecentChannelList.NumChannels() > 1) {
		const CTunerChannelInfo *pChannel =
			m_App.RecentChannelList.GetChannelInfo(1);

		if (pChannel != nullptr) {
			m_App.Core.SelectChannel(pChannel->GetTunerName(), *pChannel);
		}
	}

	return true;
}


bool CAppCommand::UpdateChannelList(CCommandManager::InvokeParameters &Params)
{
	// チャンネルリストの自動更新(現状役には立たない)
	//if (m_App.DriverOptions.IsChannelAutoUpdate(m_App.CoreEngine.GetDriverFileName()))
	{
		CTuningSpaceList TuningSpaceList(*m_App.ChannelManager.GetTuningSpaceList());
		std::vector<String> MessageList;

		TRACE(TEXT("チャンネルリスト自動更新開始\n"));
		if (m_App.ChannelScan.AutoUpdateChannelList(&TuningSpaceList, &MessageList)) {
			m_App.AddLog(TEXT("チャンネルリストの自動更新を行いました。"));
			for (const String &e : MessageList)
				m_App.AddLog(TEXT("{}"), e);

			TuningSpaceList.MakeAllChannelList();
			m_App.Core.UpdateCurrentChannelList(&TuningSpaceList);

			TCHAR szFileName[MAX_PATH];
			if (!m_App.ChannelManager.GetChannelFileName(szFileName, lengthof(szFileName))
					|| ::lstrcmpi(::PathFindExtension(szFileName), CHANNEL_FILE_EXTENSION) != 0
					|| !::PathFileExists(szFileName)) {
				m_App.CoreEngine.GetDriverPath(szFileName, lengthof(szFileName));
				::PathRenameExtension(szFileName, CHANNEL_FILE_EXTENSION);
			}
			if (TuningSpaceList.SaveToFile(szFileName))
				m_App.AddLog(TEXT("チャンネルファイルを \"{}\" に保存しました。"), szFileName);
			else
				m_App.AddLog(CLogItem::LogType::Error, TEXT("チャンネルファイル \"{}\" を保存できません。"), szFileName);
		}
	}

	return true;
}


bool CAppCommand::ToggleOneSegMode(CCommandManager::InvokeParameters &Params)
{
	m_App.Core.Set1SegMode(!m_App.Core.Is1SegMode(), true);

	return true;
}


bool CAppCommand::CloseTuner(CCommandManager::InvokeParameters &Params)
{
	m_App.Core.ShutDownTuner();

	return true;
}


bool CAppCommand::EnableBuffering(CCommandManager::InvokeParameters &Params)
{
	m_App.CoreEngine.SetPacketBufferPool(
		!m_App.CoreEngine.GetPacketBuffering(),
		m_App.PlaybackOptions.GetPacketBufferPoolPercentage());
	m_App.PlaybackOptions.SetPacketBuffering(m_App.CoreEngine.GetPacketBuffering());

	return true;
}


bool CAppCommand::ResetBuffer(CCommandManager::InvokeParameters &Params)
{
	m_App.CoreEngine.ResetPacketBuffer();

	return true;
}


bool CAppCommand::ResetErrorCount(CCommandManager::InvokeParameters &Params)
{
	m_App.CoreEngine.ResetErrorCount();
	m_App.StatusView.UpdateItem(STATUS_ITEM_ERROR);
	m_App.Panel.InfoPanel.UpdateItem(CInformationPanel::ITEM_ERROR);
	m_App.AppEventManager.OnStatisticsReset();

	return true;
}


bool CAppCommand::ShowRecordRemainTime(CCommandManager::InvokeParameters &Params)
{
	const bool fRemain = !m_App.RecordOptions.GetShowRemainTime();
	m_App.RecordOptions.SetShowRemainTime(fRemain);

	CRecordStatusItem *pItem =
		dynamic_cast<CRecordStatusItem*>(m_App.StatusView.GetItemByID(STATUS_ITEM_RECORD));
	if (pItem != nullptr)
		pItem->ShowRemainTime(fRemain);

	return true;
}


bool CAppCommand::ShowTOTTime(CCommandManager::InvokeParameters &Params)
{
	const bool fTOT = !m_App.StatusOptions.GetShowTOTTime();
	m_App.StatusOptions.SetShowTOTTime(fTOT);

	CClockStatusItem *pItem =
		dynamic_cast<CClockStatusItem*>(m_App.StatusView.GetItemByID(STATUS_ITEM_CLOCK));
	if (pItem != nullptr)
		pItem->SetTOT(fTOT);

	return true;
}


bool CAppCommand::InterpolateTOTTime(CCommandManager::InvokeParameters &Params)
{
	const bool fInterpolateTOT = !m_App.StatusOptions.GetInterpolateTOTTime();
	m_App.StatusOptions.SetInterpolateTOTTime(fInterpolateTOT);

	CClockStatusItem *pItem =
		dynamic_cast<CClockStatusItem*>(m_App.StatusView.GetItemByID(STATUS_ITEM_CLOCK));
	if (pItem != nullptr)
		pItem->SetInterpolateTOT(fInterpolateTOT);

	return true;
}


bool CAppCommand::ProgramInfoStatus_PopupInfo(CCommandManager::InvokeParameters &Params)
{
	const bool fEnable = !m_App.StatusOptions.IsPopupProgramInfoEnabled();
	m_App.StatusOptions.EnablePopupProgramInfo(fEnable);

	CProgramInfoStatusItem *pItem =
		dynamic_cast<CProgramInfoStatusItem*>(m_App.StatusView.GetItemByID(STATUS_ITEM_PROGRAMINFO));
	if (pItem != nullptr)
		pItem->EnablePopupInfo(fEnable);

	return true;
}


bool CAppCommand::ProgramInfoStatus_ShowProgress(CCommandManager::InvokeParameters &Params)
{
	CProgramInfoStatusItem *pItem =
		dynamic_cast<CProgramInfoStatusItem*>(m_App.StatusView.GetItemByID(STATUS_ITEM_PROGRAMINFO));
	if (pItem != nullptr) {
		pItem->SetShowProgress(!pItem->GetShowProgress());
		m_App.StatusOptions.SetShowEventProgress(pItem->GetShowProgress());
	}

	return true;
}


bool CAppCommand::AdjustTOTTime(CCommandManager::InvokeParameters &Params)
{
	m_App.TotTimeAdjuster.BeginAdjust();

	return true;
}


bool CAppCommand::SideBarOptions(CCommandManager::InvokeParameters &Params)
{
	const HWND hwnd = m_App.UICore.GetDialogOwner();

	if (::IsWindowEnabled(hwnd))
		m_App.ShowOptionDialog(hwnd, COptionDialog::PAGE_SIDEBAR);

	return true;
}


bool CAppCommand::DriverBrowse(CCommandManager::InvokeParameters &Params)
{
	OPENFILENAME ofn;
	TCHAR szFileName[MAX_PATH];
	String Path, Name, InitDir;

	m_App.CoreEngine.GetDriverPath(&Path);
	if (PathUtil::Split(Path, &InitDir, &Name)) {
		StringCopy(szFileName, Name.c_str());
	} else {
		m_App.GetAppDirectory(&InitDir);
		szFileName[0] = '\0';
	}

	InitOpenFileName(&ofn);

	ofn.hwndOwner = m_App.UICore.GetDialogOwner();
	ofn.lpstrFilter =
		TEXT("BonDriver(BonDriver*.dll)\0BonDriver*.dll\0")
		TEXT("すべてのファイル\0*.*\0");
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = lengthof(szFileName);
	ofn.lpstrInitialDir = InitDir.c_str();
	ofn.lpstrTitle = TEXT("BonDriverの選択");
	ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_EXPLORER;

	if (FileOpenDialog(&ofn)) {
		m_App.Core.OpenTuner(szFileName);
	}

	return true;
}


bool CAppCommand::ChannelHistoryClear(CCommandManager::InvokeParameters &Params)
{
	m_App.RecentChannelList.Clear();
	m_App.TaskbarManager.ClearRecentChannelList();
	m_App.TaskbarManager.UpdateJumpList();

	return true;
}


bool CAppCommand::AddToFavorites(CCommandManager::InvokeParameters &Params)
{
	const CChannelInfo *pChannel = m_App.ChannelManager.GetCurrentChannelInfo();

	if (pChannel != nullptr) {
		if (m_App.FavoritesManager.AddChannel(pChannel, m_App.CoreEngine.GetDriverFileName()))
			m_App.AppEventManager.OnFavoritesChanged();
	}

	return true;
}


bool CAppCommand::OrganizeFavorites(CCommandManager::InvokeParameters &Params)
{
	if (m_App.FavoritesManager.ShowOrganizeDialog(m_App.UICore.GetDialogOwner()))
		m_App.AppEventManager.OnFavoritesChanged();

	return true;
}


bool CAppCommand::SwitchVideo(CCommandManager::InvokeParameters &Params)
{
	const LibISDB::AnalyzerFilter *pAnalyzer =
		m_App.CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();

	if ((pAnalyzer != nullptr)
			&& (pAnalyzer->GetEventComponentGroupCount(m_App.CoreEngine.GetServiceIndex()) > 0))
		m_App.CommandManager.InvokeCommand(CM_MULTIVIEW_SWITCH);
	else
		m_App.CommandManager.InvokeCommand(CM_VIDEOSTREAM_SWITCH);

	return true;
}


bool CAppCommand::VideoStreamSwitch(CCommandManager::InvokeParameters &Params)
{
	const int StreamCount = m_App.CoreEngine.GetVideoStreamCount();

	if (StreamCount > 1) {
		m_App.CoreEngine.SetVideoStream((m_App.CoreEngine.GetVideoStream() + 1) % StreamCount);
	}

	return true;
}


bool CAppCommand::MultiViewSwitch(CCommandManager::InvokeParameters &Params)
{
	const LibISDB::AnalyzerFilter *pAnalyzer =
		m_App.CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();

	if (pAnalyzer != nullptr) {
		const int ServiceIndex = m_App.CoreEngine.GetServiceIndex();
		const int GroupCount = pAnalyzer->GetEventComponentGroupCount(ServiceIndex);

		if (GroupCount > 1) {
			m_App.CommandManager.InvokeCommand(
				CM_MULTIVIEW_FIRST +
					(pAnalyzer->GetEventComponentGroupIndexByComponentTag(
						ServiceIndex, m_App.CoreEngine.GetVideoComponentTag()) + 1) % GroupCount);
		}
	}

	return true;
}


bool CAppCommand::SelectChannelNo(CCommandManager::InvokeParameters &Params)
{
	m_App.Core.SwitchChannelByNo((Params.ID - CM_CHANNELNO_FIRST) + 1, true);

	return true;
}


bool CAppCommand::SelectChannel(CCommandManager::InvokeParameters &Params)
{
	m_App.Core.SwitchChannel(Params.ID - CM_CHANNEL_FIRST);

	return true;
}


bool CAppCommand::SelectService(CCommandManager::InvokeParameters &Params)
{
	if (m_App.RecordManager.IsRecording()) {
		if (!m_App.RecordOptions.ConfirmServiceChange(m_App.UICore.GetDialogOwner(), &m_App.RecordManager))
			return true;
	}

	m_App.Core.SetServiceByIndex(Params.ID - CM_SERVICE_FIRST, CAppCore::SetServiceFlag::StrictID);

	return true;
}


bool CAppCommand::SelectSpace(CCommandManager::InvokeParameters &Params)
{
	const int Space = Params.ID - CM_SPACE_FIRST;

	if (Space != m_App.ChannelManager.GetCurrentSpace()) {
		const CChannelList *pChannelList = m_App.ChannelManager.GetChannelList(Space);

		if (pChannelList != nullptr) {
			for (int i = 0; i < pChannelList->NumChannels(); i++) {
				if (pChannelList->IsEnabled(i)) {
					m_App.Core.SetChannel(Space, i);
					return true;
				}
			}
		}
	}

	return true;
}


bool CAppCommand::SelectDriver(CCommandManager::InvokeParameters &Params)
{
	const CDriverInfo *pDriverInfo = m_App.DriverManager.GetDriverInfo(Params.ID - CM_DRIVER_FIRST);

	if (pDriverInfo != nullptr) {
		if (!m_App.CoreEngine.IsTunerOpen()
				|| !IsEqualFileName(pDriverInfo->GetFileName(), m_App.CoreEngine.GetDriverFileName())) {
			if (m_App.Core.OpenTuner(pDriverInfo->GetFileName())) {
				m_App.Core.RestoreChannel();
			}
		}
	}

	return true;
}


bool CAppCommand::EnablePlugin(CCommandManager::InvokeParameters &Params)
{
	CPlugin *pPlugin = m_App.PluginManager.GetPlugin(m_App.PluginManager.FindPluginByCommand(Params.ID));

	if (pPlugin != nullptr)
		pPlugin->Enable(!pPlugin->IsEnabled());

	return true;
}


bool CAppCommand::SelectSpaceChannel(CCommandManager::InvokeParameters &Params)
{
	if (!m_App.UICore.ConfirmChannelChange())
		return true;

	m_App.UICore.ProcessTunerMenu(Params.ID);

	return true;
}


bool CAppCommand::ChannelHistory(CCommandManager::InvokeParameters &Params)
{
	const CTunerChannelInfo *pChannel =
		m_App.RecentChannelList.GetChannelInfo(Params.ID - CM_CHANNELHISTORY_FIRST);

	if (pChannel != nullptr)
		m_App.Core.SelectChannel(pChannel->GetTunerName(), *pChannel);

	return true;
}


bool CAppCommand::FavoriteChannel(CCommandManager::InvokeParameters &Params)
{
	CFavoritesManager::ChannelInfo ChannelInfo;

	if (m_App.FavoritesManager.GetChannelByCommand(Params.ID, &ChannelInfo)) {
		m_App.Core.SelectChannel(
			ChannelInfo.BonDriverFileName.c_str(),
			ChannelInfo.Channel,
			ChannelInfo.fForceBonDriverChange ?
				CAppCore::SelectChannelFlag::None :
				CAppCore::SelectChannelFlag::UseCurrentTuner);
	}

	return true;
}


bool CAppCommand::PluginCommand(CCommandManager::InvokeParameters &Params)
{
	m_App.PluginManager.OnPluginCommand(m_App.CommandManager.GetCommandIDText(Params.ID).c_str());

	return true;
}


bool CAppCommand::VideoStream(CCommandManager::InvokeParameters &Params)
{
	m_App.CoreEngine.SetVideoStream(Params.ID - CM_VIDEOSTREAM_FIRST);

	return true;
}


bool CAppCommand::AudioStream(CCommandManager::InvokeParameters &Params)
{
	m_App.UICore.SetAudioStream(Params.ID - CM_AUDIOSTREAM_FIRST);
	m_App.UICore.ShowAudioOSD();

	return true;
}


bool CAppCommand::SelectAudio(CCommandManager::InvokeParameters &Params)
{
	m_App.UICore.SelectAudio(Params.ID - CM_AUDIO_FIRST);
	m_App.UICore.ShowAudioOSD();

	return true;
}


bool CAppCommand::MultiView(CCommandManager::InvokeParameters &Params)
{
	const int ServiceIndex = m_App.CoreEngine.GetServiceIndex();
	const LibISDB::AnalyzerFilter *pAnalyzer = m_App.CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
	LibISDB::AnalyzerFilter::EventComponentGroupInfo GroupInfo;

	if (ServiceIndex >= 0
			&& pAnalyzer != nullptr
			&& pAnalyzer->GetEventComponentGroupInfo(ServiceIndex, Params.ID - CM_MULTIVIEW_FIRST, &GroupInfo)) {
		int VideoIndex = -1, AudioIndex = -1;

		for (int i = 0; i < GroupInfo.NumOfCAUnit; i++) {
			for (int j = 0; j < GroupInfo.CAUnitList[i].NumOfComponent; j++) {
				const std::uint8_t ComponentTag = GroupInfo.CAUnitList[i].ComponentTag[j];

				if (VideoIndex < 0) {
					const int Index = pAnalyzer->GetVideoIndexByComponentTag(ServiceIndex, ComponentTag);
					if (Index >= 0)
						VideoIndex = Index;
				}

				if (AudioIndex < 0) {
					const int Index = pAnalyzer->GetAudioIndexByComponentTag(ServiceIndex, ComponentTag);
					if (Index >= 0)
						AudioIndex = Index;
				}

				if (VideoIndex >= 0 && AudioIndex >= 0) {
					m_App.CoreEngine.SetVideoStream(VideoIndex);
					m_App.UICore.SetAudioStream(AudioIndex);
					return true;
				}
			}
		}
	}

	return true;
}


bool CAppCommand::SelectPanelPage(CCommandManager::InvokeParameters &Params)
{
	m_App.Panel.Form.SetCurPageByID(Params.ID - CM_PANEL_FIRST);

	return true;
}


bool CAppCommand::ToggleEventInfoOSD(CCommandManager::InvokeParameters &Params)
{
	if (!m_App.OSDManager.IsEventInfoOSDVisible())
		m_App.UICore.ShowEventInfoOSD(COSDManager::EventInfoOSDFlag::Manual);
	else
		m_App.OSDManager.HideEventInfoOSD();

	return true;
}




const CCommandManager::CommandHandler CAppCommand::CommandParameters::NullHandler;


} // namespace TVTest
