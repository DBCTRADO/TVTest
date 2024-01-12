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
#include "LibISDB/LibISDB/TS/TSInformation.hpp"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CMainDisplay::CMainDisplay(CAppMain &App)
	: m_App(App)
	, ChannelDisplayEventHandler(App)
{
}


bool CMainDisplay::Create(HWND hwndParent, int ViewID, int ContainerID, HWND hwndMessage)
{
	m_ViewWindow.Create(
		hwndParent, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, ViewID);
	m_ViewWindow.SetMessageWindow(hwndMessage);
	m_VideoContainer.Create(
		m_ViewWindow.GetHandle(),
		WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, ContainerID,
		m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>());
	m_ViewWindow.SetVideoContainer(&m_VideoContainer);

	m_DisplayBase.SetParent(&m_VideoContainer);
	m_VideoContainer.SetDisplayBase(&m_DisplayBase);

	return true;
}


bool CMainDisplay::EnableViewer(bool fEnable)
{
	if (m_fViewerEnabled != fEnable) {
		LibISDB::ViewerFilter *pViewer = m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();

		if (pViewer == nullptr)
			return false;
		if (fEnable && !pViewer->IsOpen())
			return false;
		if (fEnable || (!fEnable && !m_DisplayBase.IsVisible()))
			m_VideoContainer.SetVisible(fEnable);
		pViewer->SetVisible(fEnable);
		if (!m_App.CoreEngine.EnableMediaViewer(fEnable))
			return false;
		if (m_App.PlaybackOptions.GetMinTimerResolution())
			m_App.CoreEngine.SetMinTimerResolution(fEnable);
		m_fViewerEnabled = fEnable;
		m_App.AppEventManager.OnPlaybackStateChanged(fEnable);
	}
	return true;
}


bool CMainDisplay::BuildViewer(BYTE VideoStreamType)
{
	if (VideoStreamType == 0) {
		VideoStreamType = m_App.CoreEngine.GetVideoStreamType();
		if (VideoStreamType == LibISDB::STREAM_TYPE_UNINITIALIZED)
			return false;
	}
	LPCWSTR pszVideoDecoder = nullptr;

	switch (VideoStreamType) {
	case LibISDB::STREAM_TYPE_MPEG2_VIDEO:
		pszVideoDecoder = m_App.VideoOptions.GetMpeg2DecoderName();
		break;

	case LibISDB::STREAM_TYPE_H264:
		pszVideoDecoder = m_App.VideoOptions.GetH264DecoderName();
		break;

	case LibISDB::STREAM_TYPE_H265:
		pszVideoDecoder = m_App.VideoOptions.GetH265DecoderName();
		break;

	default:
		if (m_App.CoreEngine.GetAudioStreamCount() == 0)
			return false;
		VideoStreamType = LibISDB::STREAM_TYPE_INVALID;
		break;
	}

	if (m_fViewerEnabled)
		EnableViewer(false);

	m_App.AddLog(
		TEXT("DirectShowの初期化を行います({})..."),
		VideoStreamType == LibISDB::STREAM_TYPE_INVALID ?
			TEXT("映像なし") :
			LibISDB::GetStreamTypeText(VideoStreamType));

	LibISDB::ViewerFilter *pViewer = m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();
	if (pViewer == nullptr)
		return false;

	LibISDB::ViewerFilter::OpenSettings OpenSettings;

	OpenSettings.hwndRender = m_VideoContainer.GetHandle();
	OpenSettings.hwndMessageDrain = m_VideoContainer.GetHandle();
	OpenSettings.VideoRenderer = m_App.VideoOptions.GetVideoRendererType();
	OpenSettings.VideoStreamType = VideoStreamType;
	OpenSettings.pszVideoDecoder = pszVideoDecoder;
	OpenSettings.AudioDevice.FriendlyName = m_App.AudioOptions.GetAudioDeviceName();
	OpenSettings.AudioDevice.MonikerName = m_App.AudioOptions.GetAudioDeviceMoniker();
	if (m_App.AudioOptions.GetAudioFilterName() != nullptr)
		OpenSettings.AudioFilterList.emplace_back(m_App.AudioOptions.GetAudioFilterName());

	pViewer->SetAACDecoderType(m_App.AudioOptions.GetAACDecoder());

	if (!m_App.CoreEngine.BuildMediaViewer(OpenSettings)) {
		m_App.Core.OnError(&m_App.CoreEngine, TEXT("DirectShowの初期化ができません。"));
		return false;
	}
	m_App.AudioOptions.ApplyMediaViewerOptions();

	m_App.AddLog(TEXT("DirectShowの初期化を行いました。"));

	return true;
}


bool CMainDisplay::CloseViewer()
{
	EnableViewer(false);
	m_App.CoreEngine.CloseMediaViewer();
	return true;
}


HWND CMainDisplay::GetDisplayViewParent() const
{
	return m_DisplayBase.GetParent()->GetHandle();
}




void CMainDisplay::CHomeDisplayEventHandler::OnClose()
{
	m_pHomeDisplay->SetVisible(false);
}


void CMainDisplay::CHomeDisplayEventHandler::OnMouseMessage(UINT Msg, int x, int y)
{
	RelayMouseMessage(m_pHomeDisplay, Msg, x, y);
}




CMainDisplay::CChannelDisplayEventHandler::CChannelDisplayEventHandler(CAppMain &App)
	: m_App(App)
{
}


void CMainDisplay::CChannelDisplayEventHandler::OnTunerSelect(
	LPCTSTR pszDriverFileName, int TuningSpace)
{
	if (m_App.CoreEngine.IsTunerOpen()
			&& IsEqualFileName(m_App.CoreEngine.GetDriverFileName(), pszDriverFileName)) {
		m_App.UICore.DoCommand(CM_CHANNELDISPLAY);
	} else {
		if (!m_App.UICore.ConfirmChannelChange())
			return;

		if (m_App.Core.OpenTuner(pszDriverFileName)) {
			if (TuningSpace != SPACE_NOTSPECIFIED) {
				m_App.UICore.DoCommand(CM_SPACE_FIRST + TuningSpace);
				if (TuningSpace == SPACE_ALL
						|| TuningSpace == m_App.RestoreChannelInfo.Space)
					m_App.Core.RestoreChannel();
			} else {
				m_App.Core.RestoreChannel();
			}
		}
		m_App.UICore.DoCommand(CM_CHANNELDISPLAY);
	}
}


void CMainDisplay::CChannelDisplayEventHandler::OnChannelSelect(
	LPCTSTR pszDriverFileName, const CChannelInfo *pChannelInfo)
{
	if (!m_App.UICore.ConfirmChannelChange())
		return;

	if (m_App.Core.OpenTuner(pszDriverFileName)) {
		int Space;
		if (m_App.RestoreChannelInfo.fAllChannels)
			Space = CChannelManager::SPACE_ALL;
		else
			Space = pChannelInfo->GetSpace();
		const CChannelList *pList = m_App.ChannelManager.GetChannelList(Space);
		if (pList != nullptr) {
			int Index = pList->FindByIndex(
				pChannelInfo->GetSpace(),
				pChannelInfo->GetChannelIndex(),
				pChannelInfo->GetServiceID());

			if (Index < 0 && Space == CChannelManager::SPACE_ALL) {
				Space = pChannelInfo->GetSpace();
				pList = m_App.ChannelManager.GetChannelList(Space);
				if (pList != nullptr)
					Index = pList->FindByIndex(
						-1,
						pChannelInfo->GetChannelIndex(),
						pChannelInfo->GetServiceID());
			}
			if (Index >= 0)
				m_App.Core.SetChannel(Space, Index);
		}
		m_App.UICore.DoCommand(CM_CHANNELDISPLAY);
	}
}


void CMainDisplay::CChannelDisplayEventHandler::OnClose()
{
	m_pChannelDisplay->SetVisible(false);
}


void CMainDisplay::CChannelDisplayEventHandler::OnMouseMessage(UINT Msg, int x, int y)
{
	RelayMouseMessage(m_pChannelDisplay, Msg, x, y);
}


}	// namespace TVTest
