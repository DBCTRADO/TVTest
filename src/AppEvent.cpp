#include "stdafx.h"
#include "TVTest.h"
#include "AppEvent.h"
#include <algorithm>
#include "Common/DebugDef.h"


#define CALL_HANDLERS(func) \
	EnumHandlers([&](CAppEventHandler *pHandler) { pHandler->func; })


namespace TVTest
{


bool CAppEventManager::AddEventHandler(CAppEventHandler *pHandler)
{
	if (pHandler==nullptr)
		return false;

	m_HandlerList.push_back(pHandler);

	return true;
}


bool CAppEventManager::RemoveEventHandler(CAppEventHandler *pHandler)
{
	auto itr=std::find(m_HandlerList.begin(),m_HandlerList.end(),pHandler);
	if (itr==m_HandlerList.end())
		return false;
	m_HandlerList.erase(itr);
	return true;
}


void CAppEventManager::OnTunerChanged()
{
	CALL_HANDLERS(OnTunerChanged());
}


void CAppEventManager::OnTunerOpened()
{
	CALL_HANDLERS(OnTunerOpened());
}


void CAppEventManager::OnTunerClosed()
{
	CALL_HANDLERS(OnTunerClosed());
}


void CAppEventManager::OnTunerShutDown()
{
	CALL_HANDLERS(OnTunerShutDown());
}


void CAppEventManager::OnChannelChanged(unsigned int Status)
{
	CALL_HANDLERS(OnChannelChanged(Status));
}


void CAppEventManager::OnServiceChanged()
{
	CALL_HANDLERS(OnServiceChanged());
}


void CAppEventManager::OnServiceInfoUpdated()
{
	CALL_HANDLERS(OnServiceInfoUpdated());
}


void CAppEventManager::OnServiceListUpdated()
{
	CALL_HANDLERS(OnServiceListUpdated());
}


void CAppEventManager::OnChannelListChanged()
{
	CALL_HANDLERS(OnChannelListChanged());
}


void CAppEventManager::OnRecordingStart(AppEvent::RecordingStartInfo *pInfo)
{
	CALL_HANDLERS(OnRecordingStart(pInfo));
}


void CAppEventManager::OnRecordingStarted()
{
	CALL_HANDLERS(OnRecordingStarted());
}


void CAppEventManager::OnRecordingStopped()
{
	CALL_HANDLERS(OnRecordingStopped());
}


void CAppEventManager::OnRecordingPaused()
{
	CALL_HANDLERS(OnRecordingPaused());
}


void CAppEventManager::OnRecordingResumed()
{
	CALL_HANDLERS(OnRecordingResumed());
}


void CAppEventManager::OnRecordingFileChanged(LPCTSTR pszFileName)
{
	CALL_HANDLERS(OnRecordingFileChanged(pszFileName));
}


void CAppEventManager::On1SegModeChanged(bool f1SegMode)
{
	CALL_HANDLERS(On1SegModeChanged(f1SegMode));
}


void CAppEventManager::OnFullscreenChanged(bool fFullscreen)
{
	CALL_HANDLERS(OnFullscreenChanged(fFullscreen));
}


void CAppEventManager::OnPlaybackStateChanged(bool fPlayback)
{
	CALL_HANDLERS(OnPlaybackStateChanged(fPlayback));
}


void CAppEventManager::OnPanAndScanChanged()
{
	CALL_HANDLERS(OnPanAndScanChanged());
}


void CAppEventManager::OnVolumeChanged(int Volume)
{
	CALL_HANDLERS(OnVolumeChanged(Volume));
}


void CAppEventManager::OnMuteChanged(bool fMute)
{
	CALL_HANDLERS(OnMuteChanged(fMute));
}


void CAppEventManager::OnDualMonoModeChanged(CAudioDecFilter::DualMonoMode Mode)
{
	CALL_HANDLERS(OnDualMonoModeChanged(Mode));
}


void CAppEventManager::OnStereoModeChanged(CAudioDecFilter::StereoMode Mode)
{
	CALL_HANDLERS(OnStereoModeChanged(Mode));
}


void CAppEventManager::OnAudioStreamChanged(int Stream)
{
	CALL_HANDLERS(OnAudioStreamChanged(Stream));
}


void CAppEventManager::OnColorSchemeChanged()
{
	CALL_HANDLERS(OnColorSchemeChanged());
}


void CAppEventManager::OnStandbyChanged(bool fStandby)
{
	CALL_HANDLERS(OnStandbyChanged(fStandby));
}


void CAppEventManager::OnExecute(LPCTSTR pszCommandLine)
{
	CALL_HANDLERS(OnExecute(pszCommandLine));
}


void CAppEventManager::OnEngineReset()
{
	CALL_HANDLERS(OnEngineReset());
}


void CAppEventManager::OnStatisticsReset()
{
	CALL_HANDLERS(OnStatisticsReset());
}


void CAppEventManager::OnSettingsChanged()
{
	CALL_HANDLERS(OnSettingsChanged());
}


void CAppEventManager::OnClose()
{
	CALL_HANDLERS(OnClose());
}


void CAppEventManager::OnStartupDone()
{
	CALL_HANDLERS(OnStartupDone());
}


void CAppEventManager::OnFavoritesChanged()
{
	CALL_HANDLERS(OnFavoritesChanged());
}




CAppEventHandler::~CAppEventHandler()
{
}


}	// namespace TVTest
