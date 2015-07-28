#ifndef TVTEST_APP_EVENT_H
#define TVTEST_APP_EVENT_H


#include "DirectShowFilter/AudioDecFilter.h"
#include <vector>


namespace TVTest
{

	namespace AppEvent
	{

		enum {
			CHANNEL_CHANGED_STATUS_SPACE_CHANGED = 0x0001U,
			CHANNEL_CHANGED_STATUS_DETECTED      = 0x0002U
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
		virtual void OnChannelChanged(unsigned int Status) {}
		virtual void OnServiceChanged() {}
		virtual void OnServiceInfoUpdated() {}
		virtual void OnServiceListUpdated() {}
		virtual void OnChannelListChanged() {}
		virtual void OnRecordingStart(AppEvent::RecordingStartInfo *pInfo) {}
		virtual void OnRecordingStarted() {}
		virtual void OnRecordingStopped() {}
		virtual void OnRecordingPaused() {}
		virtual void OnRecordingResumed() {}
		virtual void OnRecordingFileChanged(LPCTSTR pszFileName) {}
		virtual void On1SegModeChanged(bool f1SegMode) {}
		virtual void OnFullscreenChanged(bool fFullscreen) {}
		virtual void OnPlaybackStateChanged(bool fPlayback) {}
		virtual void OnPanAndScanChanged() {}
		virtual void OnVolumeChanged(int Volume) {}
		virtual void OnMuteChanged(bool fMute) {}
		virtual void OnDualMonoModeChanged(CAudioDecFilter::DualMonoMode Mode) {}
		virtual void OnStereoModeChanged(CAudioDecFilter::StereoMode Mode) {}
		virtual void OnAudioStreamChanged(int Stream) {}
		virtual void OnColorSchemeChanged() {}
		virtual void OnStandbyChanged(bool fStandby) {}
		virtual void OnExecute(LPCTSTR pszCommandLine) {}
		virtual void OnEngineReset() {}
		virtual void OnStatisticsReset() {}
		virtual void OnSettingsChanged() {}
		virtual void OnClose() {}
		virtual void OnStartupDone() {}
		virtual void OnFavoritesChanged() {}
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
		void OnChannelChanged(unsigned int Status);
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
		void OnPanAndScanChanged();
		void OnVolumeChanged(int Volume);
		void OnMuteChanged(bool fMute);
		void OnDualMonoModeChanged(CAudioDecFilter::DualMonoMode Mode);
		void OnStereoModeChanged(CAudioDecFilter::StereoMode Mode);
		void OnAudioStreamChanged(int Stream);
		void OnColorSchemeChanged();
		void OnStandbyChanged(bool fStandby);
		void OnExecute(LPCTSTR pszCommandLine);
		void OnEngineReset();
		void OnStatisticsReset();
		void OnSettingsChanged();
		void OnClose();
		void OnStartupDone();
		void OnFavoritesChanged();

	private:
		std::vector<CAppEventHandler*> m_HandlerList;

		template<typename T> void EnumHandlers(T Pred) {
			for (auto i=m_HandlerList.begin();i!=m_HandlerList.end();++i)
				Pred(*i);
		}
	};

}	// namespace TVTest


#endif
