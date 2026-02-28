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


#ifndef TVTEST_APP_EVENT_H
#define TVTEST_APP_EVENT_H


#include "LibISDB/LibISDB/Windows/Viewer/DirectShow/AudioDecoders/AudioDecoderFilter.hpp"
#include <vector>


namespace TVTest
{

	namespace AppEvent
	{

		enum class ChannelChangeStatus : unsigned int {
			None         = 0x0000U,
			SpaceChanged = 0x0001U,
			Detected     = 0x0002U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		struct RecordingStartInfo
		{
			const class CRecordManager *pRecordManager;
			LPTSTR pszFileName;
			int MaxFileName;
		};

	}

	class ABSTRACT_CLASS(CAppEventHandler)
	{
	public:
		virtual ~CAppEventHandler() = 0;
		virtual void OnTunerChanged() {}
		virtual void OnTunerOpened() {}
		virtual void OnTunerClosed() {}
		virtual void OnTunerShutDown() {}
		virtual void OnChannelChanged(AppEvent::ChannelChangeStatus Status) {}
		virtual void OnServiceChanged() {}
		virtual void OnServiceInfoUpdated() {}
		virtual void OnServiceListUpdated() {}
		virtual void OnChannelListChanged() {}
		virtual void OnRecordingStart(AppEvent::RecordingStartInfo * pInfo) {}
		virtual void OnRecordingStarted() {}
		virtual void OnRecordingStopped() {}
		virtual void OnRecordingPaused() {}
		virtual void OnRecordingResumed() {}
		virtual void OnRecordingFileChanged(LPCTSTR pszFileName) {}
		virtual void On1SegModeChanged(bool f1SegMode) {}
		virtual void OnFullscreenChanged(bool fFullscreen) {}
		virtual void OnPlaybackStateChanged(bool fPlayback) {}
		virtual void OnVideoFormatChanged() {}
		virtual void OnPanAndScanChanged() {}
		virtual void OnAspectRatioTypeChanged(int Type) {}
		virtual void OnVolumeChanged(int Volume) {}
		virtual void OnMuteChanged(bool fMute) {}
		virtual void OnDualMonoModeChanged(LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode Mode) {}
		virtual void OnAudioStreamChanged(int Stream) {}
		virtual void OnAudioFormatChanged() {}
		virtual void OnColorSchemeChanged() {}
		virtual void OnStandbyChanged(bool fStandby) {}
		virtual void OnExecute(LPCTSTR pszCommandLine) {}
		virtual void OnEngineReset() {}
		virtual void OnStatisticsReset() {}
		virtual void OnSettingsChanged() {}
		virtual void OnClose() {}
		virtual void OnStartupDone() {}
		virtual void OnFavoritesChanged() {}
		virtual void OnVariableChanged() {}
		virtual void OnDarkModeChanged(bool fDarkMode) {}
		virtual void OnMainWindowDarkModeChanged(bool fDarkMode) {}
		virtual void OnProgramGuideDarkModeChanged(bool fDarkMode) {}
		virtual void OnEventChanged() {}
		virtual void OnEventInfoChanged() {}
	};

	class CAppEventManager
	{
	public:
		bool AddEventHandler(CAppEventHandler *pHandler);
		bool RemoveEventHandler(CAppEventHandler *pHandler);

		void OnTunerChanged();
		void OnTunerOpened();
		void OnTunerClosed();
		void OnTunerShutDown();
		void OnChannelChanged(AppEvent::ChannelChangeStatus Status);
		void OnServiceChanged();
		void OnServiceInfoUpdated();
		void OnServiceListUpdated();
		void OnChannelListChanged();
		void OnRecordingStart(AppEvent::RecordingStartInfo *pInfo);
		void OnRecordingStarted();
		void OnRecordingStopped();
		void OnRecordingPaused();
		void OnRecordingResumed();
		void OnRecordingFileChanged(LPCTSTR pszFileName);
		void On1SegModeChanged(bool f1SegMode);
		void OnFullscreenChanged(bool fFullscreen);
		void OnPlaybackStateChanged(bool fPlayback);
		void OnVideoFormatChanged();
		void OnPanAndScanChanged();
		void OnAspectRatioTypeChanged(int Type);
		void OnVolumeChanged(int Volume);
		void OnMuteChanged(bool fMute);
		void OnDualMonoModeChanged(LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode Mode);
		void OnAudioStreamChanged(int Stream);
		void OnAudioFormatChanged();
		void OnColorSchemeChanged();
		void OnStandbyChanged(bool fStandby);
		void OnExecute(LPCTSTR pszCommandLine);
		void OnEngineReset();
		void OnStatisticsReset();
		void OnSettingsChanged();
		void OnClose();
		void OnStartupDone();
		void OnFavoritesChanged();
		void OnVariableChanged();
		void OnDarkModeChanged(bool fDarkMode);
		void OnMainWindowDarkModeChanged(bool fDarkMode);
		void OnProgramGuideDarkModeChanged(bool fDarkMode);
		void OnEventChanged();
		void OnEventInfoChanged();

	private:
		std::vector<CAppEventHandler*> m_HandlerList;

		template<typename T> void EnumHandlers(T Pred)
		{
			for (auto Handler : m_HandlerList)
				Pred(Handler);
		}

		template<typename TMember, typename... TArgs> void CallHandlers(TMember Member, TArgs... Args) const
		{
			for (auto Handler : m_HandlerList)
				(Handler->*Member)(Args...);
		}
	};

} // namespace TVTest


#endif
