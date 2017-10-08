#ifndef CORE_ENGINE_H
#define CORE_ENGINE_H


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

		bool operator==(const PanAndScanInfo &Op) const {
			return XPos == Op.XPos && YPos == Op.YPos
				&& Width == Op.Width && Height == Op.Height
				&& XFactor == Op.XFactor && YFactor == Op.YFactor
				&& XAspect == Op.XAspect && YAspect == Op.YAspect;
		}
		bool operator!=(const PanAndScanInfo &Op) const { return !(*this == Op); }
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

	CCoreEngine();
	~CCoreEngine();
	void Close();
	void CreateFilters();
	bool BuildEngine();
	bool RegisterTSProcessor(
		TVTest::CTSProcessor *pTSProcessor,
		TSProcessorConnectPosition ConnectPosition);
	size_t GetTSProcessorCount() const { return m_TSProcessorList.size(); }
	TVTest::CTSProcessor *GetTSProcessorByIndex(size_t Index);
	void EnableTSProcessor(bool fEnable);
	bool IsTSProcessorEnabled() const { return m_fEnableTSProcessor; }
	bool BuildMediaViewer(const LibISDB::ViewerFilter::OpenSettings &Settings);
	bool CloseMediaViewer();
	bool EnableMediaViewer(bool fEnable);

	bool AddEventListener(EventListener *pEventListener);
	bool RemoveEventListener(EventListener *pEventListener);

	bool SetDriverDirectory(LPCTSTR pszDirectory);
	LPCTSTR GetDriverDirectory() const { return m_DriverDirectory.c_str(); }
	bool GetDriverDirectoryPath(TVTest::String *pDirectory) const;
	bool SetDriverFileName(LPCTSTR pszFileName);
	LPCTSTR GetDriverFileName() const { return m_DriverFileName.c_str(); }
	bool GetDriverPath(TVTest::String *pPath) const;
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
	bool SetStereoMode(LibISDB::DirectShow::AudioDecoderFilter::StereoMode Mode);
	LibISDB::DirectShow::AudioDecoderFilter::StereoMode GetStereoMode() const { return m_StereoMode; }
	bool SetSPDIFOptions(const LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions &Options);
	bool GetSPDIFOptions(LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions *pOptions) const;
	bool IsSPDIFPassthroughEnabled() const;

	enum {
		STATUS_VIDEOSIZE			= 0x00000001UL,
		STATUS_AUDIOCHANNELS		= 0x00000002UL,
		STATUS_AUDIOSTREAMS			= 0x00000004UL,
		STATUS_AUDIOCOMPONENTTYPE	= 0x00000008UL,
		STATUS_SPDIFPASSTHROUGH		= 0x00000010UL,
		STATUS_EVENTINFO			= 0x00000020UL,
		STATUS_EVENTID				= 0x00000040UL,
		STATUS_TOT					= 0x00000080UL
	};
	DWORD UpdateAsyncStatus();
	void SetAsyncStatusUpdatedFlag(DWORD Status);
	enum {
		STATISTIC_ERRORPACKETCOUNT				= 0x00000001UL,
		STATISTIC_CONTINUITYERRORPACKETCOUNT	= 0x00000002UL,
		STATISTIC_SCRAMBLEPACKETCOUNT			= 0x00000004UL,
		STATISTIC_SIGNALLEVEL					= 0x00000008UL,
		STATISTIC_BITRATE						= 0x00000010UL,
		STATISTIC_STREAMREMAIN					= 0x00000020UL,
		STATISTIC_PACKETBUFFERRATE				= 0x00000040UL
	};
	DWORD UpdateStatistics();
	unsigned long long GetErrorPacketCount() const { return m_ErrorPacketCount; }
	unsigned long long GetContinuityErrorPacketCount() const { return m_ContinuityErrorPacketCount; }
	unsigned long long GetScramblePacketCount() const { return m_ScrambledPacketCount; }
	void ResetErrorCount();
	float GetSignalLevel() const { return m_SignalLevel; }
	int GetSignalLevelText(LPTSTR pszText, int MaxLength) const;
	int GetSignalLevelText(float SignalLevel, LPTSTR pszText, int MaxLength) const;
	unsigned long GetBitRate() const { return m_BitRate; }
	static float BitRateToFloat(unsigned long BitRate) { return (float)BitRate / (float)(1000 * 1000); }
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
		TVTest::CTSProcessor *pTSProcessor;
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

	TVTest::CFilePath m_DriverDirectory;
	TVTest::CFilePath m_DriverFileName;
	DriverType m_DriverType;

	std::vector<TSProcessorInfo> m_TSProcessorList;
	bool m_fEnableTSProcessor;

	bool m_fPacketBuffering;

	int m_OriginalVideoWidth;
	int m_OriginalVideoHeight;
	int m_DisplayVideoWidth;
	int m_DisplayVideoHeight;
	int m_NumAudioChannels;
	int m_NumAudioStreams;
	uint8_t m_AudioComponentType;
	bool m_fMute;
	int m_Volume;
	int m_AudioGain;
	int m_SurroundAudioGain;
	LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode m_DualMonoMode;
	LibISDB::DirectShow::AudioDecoderFilter::StereoMode m_StereoMode;
	LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions m_SPDIFOptions;
	bool m_fSPDIFPassthrough;
	unsigned long long m_ErrorPacketCount;
	unsigned long long m_ContinuityErrorPacketCount;
	unsigned long long m_ScrambledPacketCount;
	float m_SignalLevel;
	unsigned long m_BitRate;
	uint32_t m_StreamRemain;
	int m_PacketBufferFillPercentage;
	UINT m_TimerResolution;
	bool m_fNoEpg;

	DWORD m_AsyncStatusUpdatedFlags;

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


#endif
