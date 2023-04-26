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


#ifndef TVTEST_CORE_ENGINE_H
#define TVTEST_CORE_ENGINE_H


#include "LibISDB/LibISDB/Windows/Viewer/ViewerEngine.hpp"
#include "LibISDB/LibISDB/Filters/TSPacketParserFilter.hpp"
#include "LibISDB/LibISDB/Filters/TeeFilter.hpp"
#include "LibISDB/LibISDB/Filters/EPGDatabaseFilter.hpp"
#include "LibISDB/LibISDB/Filters/LogoDownloaderFilter.hpp"
#include "LibISDB/LibISDB/Filters/GrabberFilter.hpp"
#include "LibISDB/LibISDB/Filters/RecorderFilter.hpp"
#include "LibISDB/LibISDB/Filters/TSPacketCounterFilter.hpp"
#include "LibISDB/LibISDB/Filters/CaptionFilter.hpp"
#include "LibISDB/LibISDB/Windows/Filters/BonDriverSourceFilter.hpp"
#include "TSProcessor.h"
#include <atomic>


namespace TVTest
{

	class CCoreEngine
		: public LibISDB::ViewerEngine
		, protected LibISDB::ViewerFilter::EventListener
		, protected LibISDB::RecorderFilter::EventListener
	{
	public:
		enum class TSProcessorConnectPosition {
			Source,
			Preprocessing,
			Postprocessing,
			Viewer,
			Recorder,
		};

		enum class DriverType {
			Unknown,
			UDP,
			TCP,
		};

		static constexpr int MAX_VOLUME = 100;

		struct PanAndScanInfo
		{
			int XPos, YPos;
			int Width, Height;
			int XFactor, YFactor;
			int XAspect, YAspect;

			bool operator==(const PanAndScanInfo &Op) const noexcept = default;
		};

		class EventListener
			: public LibISDB::EventListener
		{
		public:
			virtual void OnServiceChanged(uint16_t ServiceID) {}
			virtual void OnPATUpdated(LibISDB::AnalyzerFilter *pAnalyzer, bool StreamChanged) {}
			virtual void OnPMTUpdated(LibISDB::AnalyzerFilter *pAnalyzer, uint16_t ServiceID) {}
			virtual void OnSDTUpdated(LibISDB::AnalyzerFilter *pAnalyzer) {}
			virtual void OnWriteError(LibISDB::RecorderFilter *pRecorder) {}
			virtual void OnVideoStreamTypeChanged(uint8_t VideoStreamType) {}
			virtual void OnVideoSizeChanged(LibISDB::ViewerFilter *pViewer) {}
			virtual void OnEventChanged(LibISDB::AnalyzerFilter *pAnalyzer, uint16_t EventID) {}
			virtual void OnEventUpdated(LibISDB::AnalyzerFilter *pAnalyzer) {}
			virtual void OnTOTUpdated(LibISDB::AnalyzerFilter *pAnalyzer) {}
			virtual void OnFilterGraphInitialize(LibISDB::ViewerFilter *pViewer, IGraphBuilder *pGraphBuilder) {}
			virtual void OnFilterGraphInitialized(LibISDB::ViewerFilter *pViewer, IGraphBuilder *pGraphBuilder) {}
			virtual void OnFilterGraphFinalize(LibISDB::ViewerFilter *pViewer, IGraphBuilder *pGraphBuilder) {}
			virtual void OnFilterGraphFinalized(LibISDB::ViewerFilter *pViewer, IGraphBuilder *pGraphBuilder) {}
			virtual void OnSPDIFPassthroughError(LibISDB::ViewerFilter *pViewer, HRESULT hr) {}
		};

		~CCoreEngine();

		void Close();
		void CreateFilters();
		bool BuildEngine();
		bool RegisterTSProcessor(
			CTSProcessor *pTSProcessor,
			TSProcessorConnectPosition ConnectPosition);
		size_t GetTSProcessorCount() const { return m_TSProcessorList.size(); }
		CTSProcessor *GetTSProcessorByIndex(size_t Index);
		void EnableTSProcessor(bool fEnable);
		bool IsTSProcessorEnabled() const { return m_fEnableTSProcessor; }
		bool BuildMediaViewer(const LibISDB::ViewerFilter::OpenSettings &Settings);
		bool CloseMediaViewer();
		bool EnableMediaViewer(bool fEnable);

		bool AddEventListener(EventListener *pEventListener);
		bool RemoveEventListener(EventListener *pEventListener);

		bool SetDriverDirectory(LPCTSTR pszDirectory);
		LPCTSTR GetDriverDirectory() const { return m_DriverDirectory.c_str(); }
		bool GetDriverDirectoryPath(String *pDirectory) const;
		bool SetDriverFileName(LPCTSTR pszFileName);
		LPCTSTR GetDriverFileName() const { return m_DriverFileName.c_str(); }
		bool GetDriverPath(String *pPath) const;
		bool GetDriverPath(LPTSTR pszPath, int MaxLength) const;
		bool IsDriverSpecified() const { return !m_DriverFileName.empty(); }
		bool OpenTuner();
		bool CloseTuner();
		bool IsTunerOpen() const;
		DriverType GetDriverType() const { return m_DriverType; }
		bool IsUDPDriver() const { return m_DriverType == DriverType::UDP; }
		bool IsTCPDriver() const { return m_DriverType == DriverType::TCP; }
		bool IsNetworkDriver() const { return IsUDPDriver() || IsTCPDriver(); }

		bool SetPacketBufferLength(DWORD BufferLength);
		bool GetPacketBuffering() const { return m_fPacketBuffering; }
		bool SetPacketBufferPool(bool fBuffering, int Percentage);
		void ResetPacketBuffer();

		bool GetVideoViewSize(int *pWidth, int *pHeight);
		int GetOriginalVideoWidth() const { return m_OriginalVideoWidth; }
		int GetOriginalVideoHeight() const { return m_OriginalVideoHeight; }
		int GetDisplayVideoWidth() const { return m_DisplayVideoWidth; }
		int GetDisplayVideoHeight() const { return m_DisplayVideoHeight; }
		bool SetPanAndScan(const PanAndScanInfo &Info);
		bool GetPanAndScan(PanAndScanInfo *pInfo) const;

		bool SetVolume(int Volume);
		int GetVolume() const { return m_Volume; }
		bool SetMute(bool fMute);
		bool GetMute() const { return m_fMute; }
		bool SetAudioGainControl(int Gain, int SurroundGain);
		bool GetAudioGainControl(int *pGain, int *pSurroundGain) const;
		bool SetDualMonoMode(LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode Mode);
		LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode GetDualMonoMode() const { return m_DualMonoMode; }
		bool SetSPDIFOptions(const LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions &Options);
		bool GetSPDIFOptions(LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions *pOptions) const;
		bool IsSPDIFPassthroughEnabled() const;

		enum class StatusFlag : unsigned int {
			None               = 0x0000U,
			VideoSize          = 0x0001U,
			AudioChannels      = 0x0002U,
			AudioStreams       = 0x0004U,
			AudioComponentType = 0x0008U,
			SPDIFPassthrough   = 0x0010U,
			EventInfo          = 0x0020U,
			EventID            = 0x0040U,
			TOT                = 0x0080U,
			TVTEST_ENUM_FLAGS_TRAILER
		};
		StatusFlag UpdateAsyncStatus();
		void SetAsyncStatusUpdatedFlag(StatusFlag Status);
		enum class StatisticsFlag : unsigned int {
			None                       = 0x0000U,
			ErrorPacketCount           = 0x0001U,
			ContinuityErrorPacketCount = 0x0002U,
			ScrambledPacketCount       = 0x0004U,
			SignalLevel                = 0x0008U,
			BitRate                    = 0x0010U,
			StreamRemain               = 0x0020U,
			PacketBufferRate           = 0x0040U,
			TVTEST_ENUM_FLAGS_TRAILER
		};
		StatisticsFlag UpdateStatistics();
		unsigned long long GetErrorPacketCount() const { return m_ErrorPacketCount; }
		unsigned long long GetContinuityErrorPacketCount() const { return m_ContinuityErrorPacketCount; }
		unsigned long long GetScramblePacketCount() const { return m_ScrambledPacketCount; }
		void ResetErrorCount();
		float GetSignalLevel() const { return m_SignalLevel; }
		int GetSignalLevelText(LPTSTR pszText, int MaxLength) const;
		int GetSignalLevelText(float SignalLevel, LPTSTR pszText, int MaxLength) const;
		unsigned long GetBitRate() const { return m_BitRate; }
		static float BitRateToFloat(unsigned long BitRate) { return static_cast<float>(BitRate) / static_cast<float>(1000 * 1000); }
		float GetBitRateFloat() const { return BitRateToFloat(m_BitRate); }
		int GetBitRateText(LPTSTR pszText, int MaxLength) const;
		int GetBitRateText(unsigned long BitRate, LPTSTR pszText, int MaxLength) const;
		int GetBitRateText(float BitRate, LPTSTR pszText, int MaxLength, int Precision = 2) const;
		uint32_t GetStreamRemain() const { return m_StreamRemain; }
		int GetPacketBufferUsedPercentage() const;
		bool GetCurrentEventInfo(LibISDB::EventInfo *pInfo, uint16_t ServiceID = LibISDB::SERVICE_ID_INVALID, bool fNext = false);
		bool GetCurrentEventInfo(LibISDB::EventInfo *pInfo, bool fNext) { return GetCurrentEventInfo(pInfo, LibISDB::SERVICE_ID_INVALID, fNext); }
		LibISDB::COMMemoryPointer<> GetCurrentImage();
		bool SetMinTimerResolution(bool fMin);
		bool SetNoEpg(bool fNoEpg) { m_fNoEpg = fNoEpg; return true; }

	private:
		struct TSProcessorInfo
		{
			CTSProcessor *pTSProcessor;
			TSProcessorConnectPosition ConnectPosition;
			LibISDB::FilterGraph::IDType FilterID;
		};

		struct TSProcessorConnectionList
		{
			std::vector<LibISDB::FilterGraph::ConnectionInfo> List;

			void Add(
				LibISDB::FilterGraph::IDType UpstreamFilter,
				LibISDB::FilterGraph::IDType DownstreamFilter,
				int OutputIndex = 0)
			{
				LibISDB::FilterGraph::ConnectionInfo Info;
				Info.UpstreamFilterID = UpstreamFilter;
				Info.DownstreamFilterID = DownstreamFilter;
				Info.OutputIndex = OutputIndex;
				List.push_back(Info);
			}
		};

		LibISDB::EventListenerList<EventListener> m_EventListenerList;

		CFilePath m_DriverDirectory;
		CFilePath m_DriverFileName;
		DriverType m_DriverType = DriverType::Unknown;

		std::vector<TSProcessorInfo> m_TSProcessorList;
		bool m_fEnableTSProcessor = true;

		bool m_fPacketBuffering = false;

		int m_OriginalVideoWidth = 0;
		int m_OriginalVideoHeight = 0;
		int m_DisplayVideoWidth = 0;
		int m_DisplayVideoHeight = 0;
		int m_NumAudioChannels = LibISDB::ViewerFilter::AudioChannelCount_Invalid;
		int m_NumAudioStreams = 0;
		uint8_t m_AudioComponentType = 0;
		bool m_fMute = false;
		int m_Volume = 50;
		int m_AudioGain = 100;
		int m_SurroundAudioGain = 100;
		LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode m_DualMonoMode = LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Main;
		LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions m_SPDIFOptions;
		bool m_fSPDIFPassthrough = false;
		unsigned long long m_ErrorPacketCount = 0;
		unsigned long long m_ContinuityErrorPacketCount = 0;
		unsigned long long m_ScrambledPacketCount = 0;
		float m_SignalLevel = 0.0f;
		unsigned long m_BitRate = 0;
		uint32_t m_StreamRemain = 0;
		int m_PacketBufferFillPercentage = 0;
		UINT m_TimerResolution = 0;
		bool m_fNoEpg = false;

		std::atomic<std::underlying_type_t<StatusFlag>> m_AsyncStatusUpdatedFlags{0};

		void ConnectFilter(
			TSProcessorConnectionList *pList,
			LibISDB::FilterGraph::IDType ID,
			LibISDB::FilterGraph::IDType *pFilterID,
			int OutputIndex = 0);
		void ConnectTSProcessor(
			TSProcessorConnectionList *pList,
			TSProcessorConnectPosition ConnectPosition,
			LibISDB::FilterGraph::IDType *pFilterID,
			int *pOutputIndex = nullptr);

	// TSEngine
		void OnServiceChanged(uint16_t ServiceID) override;
		void OnEventChanged(uint16_t EventID) override;
		void OnVideoStreamTypeChanged(uint8_t StreamType) override;

	// AnalyzerFilter::EventListener
		void OnPATUpdated(LibISDB::AnalyzerFilter *pAnalyzer) override;
		void OnPMTUpdated(LibISDB::AnalyzerFilter *pAnalyzer, uint16_t ServiceID) override;
		void OnEITUpdated(LibISDB::AnalyzerFilter *pAnalyzer) override;
		void OnSDTUpdated(LibISDB::AnalyzerFilter *pAnalyzer) override;
		void OnTOTUpdated(LibISDB::AnalyzerFilter *pAnalyzer) override;

	// ViewerFilter::EventListener
		void OnVideoSizeChanged(LibISDB::ViewerFilter *pViewer, const LibISDB::DirectShow::VideoParser::VideoInfo &Info) override;
		void OnFilterGraphInitialize(LibISDB::ViewerFilter *pViewer, IGraphBuilder *pGraphBuilder) override;
		void OnFilterGraphInitialized(LibISDB::ViewerFilter *pViewer, IGraphBuilder *pGraphBuilder) override;
		void OnFilterGraphFinalize(LibISDB::ViewerFilter *pViewer, IGraphBuilder *pGraphBuilder) override;
		void OnFilterGraphFinalized(LibISDB::ViewerFilter *pViewer, IGraphBuilder *pGraphBuilder) override;
		void OnSPDIFPassthroughError(LibISDB::ViewerFilter *pViewer, HRESULT hr) override;

	// RecorderFilter::EventListener
		void OnWriteError(LibISDB::RecorderFilter *pRecorder, LibISDB::RecorderFilter::RecordingTask *pTask) override;
	};

}	// namespace TVTest


#endif
