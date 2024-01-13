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


#include "stdafx.h"
#include "TVTest.h"
#include "AppEvent.h"
#include <algorithm>
#include "Common/DebugDef.h"


namespace TVTest
{


bool CAppEventManager::AddEventHandler(CAppEventHandler *pHandler)
{
	if (pHandler == nullptr)
		return false;

	m_HandlerList.push_back(pHandler);

	return true;
}


bool CAppEventManager::RemoveEventHandler(CAppEventHandler *pHandler)
{
	auto itr = std::ranges::find(m_HandlerList, pHandler);
	if (itr == m_HandlerList.end())
		return false;
	m_HandlerList.erase(itr);
	return true;
}


void CAppEventManager::OnTunerChanged()
{
	CallHandlers(&CAppEventHandler::OnTunerChanged);
}


void CAppEventManager::OnTunerOpened()
{
	CallHandlers(&CAppEventHandler::OnTunerOpened);
}


void CAppEventManager::OnTunerClosed()
{
	CallHandlers(&CAppEventHandler::OnTunerClosed);
}


void CAppEventManager::OnTunerShutDown()
{
	CallHandlers(&CAppEventHandler::OnTunerShutDown);
}


void CAppEventManager::OnChannelChanged(AppEvent::ChannelChangeStatus Status)
{
	CallHandlers(&CAppEventHandler::OnChannelChanged, Status);
}


void CAppEventManager::OnServiceChanged()
{
	CallHandlers(&CAppEventHandler::OnServiceChanged);
}


void CAppEventManager::OnServiceInfoUpdated()
{
	CallHandlers(&CAppEventHandler::OnServiceInfoUpdated);
}


void CAppEventManager::OnServiceListUpdated()
{
	CallHandlers(&CAppEventHandler::OnServiceListUpdated);
}


void CAppEventManager::OnChannelListChanged()
{
	CallHandlers(&CAppEventHandler::OnChannelListChanged);
}


void CAppEventManager::OnRecordingStart(AppEvent::RecordingStartInfo *pInfo)
{
	CallHandlers(&CAppEventHandler::OnRecordingStart, pInfo);
}


void CAppEventManager::OnRecordingStarted()
{
	CallHandlers(&CAppEventHandler::OnRecordingStarted);
}


void CAppEventManager::OnRecordingStopped()
{
	CallHandlers(&CAppEventHandler::OnRecordingStopped);
}


void CAppEventManager::OnRecordingPaused()
{
	CallHandlers(&CAppEventHandler::OnRecordingPaused);
}


void CAppEventManager::OnRecordingResumed()
{
	CallHandlers(&CAppEventHandler::OnRecordingResumed);
}


void CAppEventManager::OnRecordingFileChanged(LPCTSTR pszFileName)
{
	CallHandlers(&CAppEventHandler::OnRecordingFileChanged, pszFileName);
}


void CAppEventManager::On1SegModeChanged(bool f1SegMode)
{
	CallHandlers(&CAppEventHandler::On1SegModeChanged, f1SegMode);
}


void CAppEventManager::OnFullscreenChanged(bool fFullscreen)
{
	CallHandlers(&CAppEventHandler::OnFullscreenChanged, fFullscreen);
}


void CAppEventManager::OnPlaybackStateChanged(bool fPlayback)
{
	CallHandlers(&CAppEventHandler::OnPlaybackStateChanged, fPlayback);
}


void CAppEventManager::OnVideoFormatChanged()
{
	CallHandlers(&CAppEventHandler::OnVideoFormatChanged);
}


void CAppEventManager::OnPanAndScanChanged()
{
	CallHandlers(&CAppEventHandler::OnPanAndScanChanged);
}


void CAppEventManager::OnAspectRatioTypeChanged(int Type)
{
	CallHandlers(&CAppEventHandler::OnAspectRatioTypeChanged, Type);
}


void CAppEventManager::OnVolumeChanged(int Volume)
{
	CallHandlers(&CAppEventHandler::OnVolumeChanged, Volume);
}


void CAppEventManager::OnMuteChanged(bool fMute)
{
	CallHandlers(&CAppEventHandler::OnMuteChanged, fMute);
}


void CAppEventManager::OnDualMonoModeChanged(LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode Mode)
{
	CallHandlers(&CAppEventHandler::OnDualMonoModeChanged, Mode);
}


void CAppEventManager::OnAudioStreamChanged(int Stream)
{
	CallHandlers(&CAppEventHandler::OnAudioStreamChanged, Stream);
}


void CAppEventManager::OnAudioFormatChanged()
{
	CallHandlers(&CAppEventHandler::OnAudioFormatChanged);
}


void CAppEventManager::OnColorSchemeChanged()
{
	CallHandlers(&CAppEventHandler::OnColorSchemeChanged);
}


void CAppEventManager::OnStandbyChanged(bool fStandby)
{
	CallHandlers(&CAppEventHandler::OnStandbyChanged, fStandby);
}


void CAppEventManager::OnExecute(LPCTSTR pszCommandLine)
{
	CallHandlers(&CAppEventHandler::OnExecute, pszCommandLine);
}


void CAppEventManager::OnEngineReset()
{
	CallHandlers(&CAppEventHandler::OnEngineReset);
}


void CAppEventManager::OnStatisticsReset()
{
	CallHandlers(&CAppEventHandler::OnStatisticsReset);
}


void CAppEventManager::OnSettingsChanged()
{
	CallHandlers(&CAppEventHandler::OnSettingsChanged);
}


void CAppEventManager::OnClose()
{
	CallHandlers(&CAppEventHandler::OnClose);
}


void CAppEventManager::OnStartupDone()
{
	CallHandlers(&CAppEventHandler::OnStartupDone);
}


void CAppEventManager::OnFavoritesChanged()
{
	CallHandlers(&CAppEventHandler::OnFavoritesChanged);
}


void CAppEventManager::OnVariableChanged()
{
	CallHandlers(&CAppEventHandler::OnVariableChanged);
}


void CAppEventManager::OnDarkModeChanged(bool fDarkMode)
{
	CallHandlers(&CAppEventHandler::OnDarkModeChanged, fDarkMode);
}


void CAppEventManager::OnMainWindowDarkModeChanged(bool fDarkMode)
{
	CallHandlers(&CAppEventHandler::OnMainWindowDarkModeChanged, fDarkMode);
}


void CAppEventManager::OnProgramGuideDarkModeChanged(bool fDarkMode)
{
	CallHandlers(&CAppEventHandler::OnProgramGuideDarkModeChanged, fDarkMode);
}


void CAppEventManager::OnEventChanged()
{
	CallHandlers(&CAppEventHandler::OnEventChanged);
}


void CAppEventManager::OnEventInfoChanged()
{
	CallHandlers(&CAppEventHandler::OnEventInfoChanged);
}




CAppEventHandler::~CAppEventHandler() = default;


} // namespace TVTest
