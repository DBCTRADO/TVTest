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
#include "CoreEngine.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CCoreEngine::~CCoreEngine()
{
	Close();

	if (m_TimerResolution != 0)
		::timeEndPeriod(m_TimerResolution);
}


void CCoreEngine::Close()
{
	CloseEngine();

	if (!m_TSProcessorList.empty()) {
		for (auto &e : m_TSProcessorList) {
			m_FilterGraph.UnregisterFilter(e.pTSProcessor, false);
			e.pTSProcessor->Release();
		}
		m_TSProcessorList.clear();
	}
}


void CCoreEngine::CreateFilters()
{
	LibISDB::BonDriverSourceFilter *pSourceFilter = new LibISDB::BonDriverSourceFilter;
	RegisterFilter(pSourceFilter);

	LibISDB::TSPacketParserFilter *pPacketParser = new LibISDB::TSPacketParserFilter;
	RegisterFilter(pPacketParser);

	LibISDB::AnalyzerFilter *pAnalyzer = new LibISDB::AnalyzerFilter;
	RegisterFilter(pAnalyzer);

	LibISDB::TeeFilter *pTee = new LibISDB::TeeFilter;
	RegisterFilter(pTee);

	if (!m_fNoEpg) {
		LibISDB::EPGDatabaseFilter *pEPGDatabaseFilter = new LibISDB::EPGDatabaseFilter;
		RegisterFilter(pEPGDatabaseFilter);
	}

	LibISDB::LogoDownloaderFilter *pLogoDownloader = new LibISDB::LogoDownloaderFilter;
	RegisterFilter(pLogoDownloader);

	LibISDB::GrabberFilter *pGrabber = new LibISDB::GrabberFilter;
	RegisterFilter(pGrabber);

	LibISDB::RecorderFilter *pRecorder = new LibISDB::RecorderFilter;
	pRecorder->AddEventListener(this);
	RegisterFilter(pRecorder);

	LibISDB::TSPacketCounterFilter *pPacketCounter = new LibISDB::TSPacketCounterFilter;
	RegisterFilter(pPacketCounter);

	LibISDB::CaptionFilter *pCaptionFilter = new LibISDB::CaptionFilter;
	RegisterFilter(pCaptionFilter);

	LibISDB::ViewerFilter *pViewer = new LibISDB::ViewerFilter;
	pViewer->AddEventListener(this);
	RegisterFilter(pViewer);
}


bool CCoreEngine::BuildEngine()
{
	TSProcessorConnectionList ConnectionList;
	LibISDB::FilterGraph::IDType FilterID;
	int OutputIndex;

	FilterID = m_FilterGraph.GetFilterID<LibISDB::BonDriverSourceFilter>();
	ConnectTSProcessor(&ConnectionList, TSProcessorConnectPosition::Source, &FilterID);
	ConnectFilter(&ConnectionList, m_FilterGraph.GetFilterID<LibISDB::TSPacketParserFilter>(), &FilterID);
	ConnectTSProcessor(&ConnectionList, TSProcessorConnectPosition::Preprocessing, &FilterID);
	ConnectFilter(&ConnectionList, m_FilterGraph.GetFilterID<LibISDB::AnalyzerFilter>(), &FilterID);
	ConnectTSProcessor(&ConnectionList, TSProcessorConnectPosition::Postprocessing, &FilterID);
	ConnectFilter(&ConnectionList, m_FilterGraph.GetFilterID<LibISDB::TeeFilter>(), &FilterID);
	const LibISDB::FilterGraph::IDType TeeFilterID = FilterID;
	if (!m_fNoEpg) {
		ConnectFilter(&ConnectionList, m_FilterGraph.GetFilterID<LibISDB::EPGDatabaseFilter>(), &FilterID, 0);
	}
	ConnectFilter(&ConnectionList, m_FilterGraph.GetFilterID<LibISDB::LogoDownloaderFilter>(), &FilterID, 0);
	ConnectTSProcessor(&ConnectionList, TSProcessorConnectPosition::Recorder, &FilterID);
	ConnectFilter(&ConnectionList, m_FilterGraph.GetFilterID<LibISDB::GrabberFilter>(), &FilterID);
	ConnectFilter(&ConnectionList, m_FilterGraph.GetFilterID<LibISDB::RecorderFilter>(), &FilterID);
	FilterID = TeeFilterID;
	OutputIndex = 1;
	ConnectTSProcessor(&ConnectionList, TSProcessorConnectPosition::Viewer, &FilterID, &OutputIndex);
	ConnectFilter(&ConnectionList, m_FilterGraph.GetFilterID<LibISDB::TSPacketCounterFilter>(), &FilterID, OutputIndex);
	ConnectFilter(&ConnectionList, m_FilterGraph.GetFilterID<LibISDB::CaptionFilter>(), &FilterID);
	ConnectFilter(&ConnectionList, m_FilterGraph.GetFilterID<LibISDB::ViewerFilter>(), &FilterID);

	if (!TSEngine::BuildEngine(
				ConnectionList.List.data(), ConnectionList.List.size())) {
		return false;
	}

	return true;
}


void CCoreEngine::ConnectFilter(
	TSProcessorConnectionList *pList,
	LibISDB::FilterGraph::IDType ID,
	LibISDB::FilterGraph::IDType *pFilterID,
	int OutputIndex)
{
	pList->Add(*pFilterID, ID, OutputIndex);
	*pFilterID = ID;
}


void CCoreEngine::ConnectTSProcessor(
	TSProcessorConnectionList *pList,
	TSProcessorConnectPosition ConnectPosition,
	LibISDB::FilterGraph::IDType *pFilterID,
	int *pOutputIndex)
{
	if (!m_fEnableTSProcessor)
		return;

	LibISDB::FilterGraph::IDType FilterID = *pFilterID;
	int OutputIndex = pOutputIndex != nullptr ? *pOutputIndex : 0;

	for (const auto &e : m_TSProcessorList) {
		if (e.ConnectPosition == ConnectPosition) {
			pList->Add(FilterID, e.FilterID, OutputIndex);
			FilterID = e.FilterID;
			OutputIndex = 0;
		}
	}

	*pFilterID = FilterID;
	if (pOutputIndex != nullptr)
		*pOutputIndex = OutputIndex;
}


CTSProcessor *CCoreEngine::GetTSProcessorByIndex(size_t Index)
{
	if (Index >= m_TSProcessorList.size())
		return nullptr;
	return m_TSProcessorList[Index].pTSProcessor;
}


bool CCoreEngine::RegisterTSProcessor(
	CTSProcessor *pTSProcessor,
	TSProcessorConnectPosition ConnectPosition)
{
	if (pTSProcessor == nullptr)
		return false;

	TSProcessorInfo Info;

	Info.pTSProcessor = pTSProcessor;
	Info.ConnectPosition = ConnectPosition;
	Info.FilterID = RegisterFilter(pTSProcessor);

	if (ConnectPosition == TSProcessorConnectPosition::Source)
		pTSProcessor->SetSourceProcessor(true);

	m_TSProcessorList.push_back(Info);

	return true;
}


void CCoreEngine::EnableTSProcessor(bool fEnable)
{
	m_fEnableTSProcessor = fEnable;
}


bool CCoreEngine::BuildMediaViewer(const LibISDB::ViewerFilter::OpenSettings &Settings)
{
	if (!IsViewerOpen()) {
		if (!BuildViewer(Settings)) {
			return false;
		}
	} else {
		if (!RebuildViewer(Settings)) {
			return false;
		}
	}

	m_pViewer->SetVolume(m_fMute ? -100.0f : LevelToDeciBel(m_Volume));
	m_pViewer->SetAudioGainControl(
		m_AudioGain != 100 || m_SurroundAudioGain != 100,
		static_cast<float>(m_AudioGain) / 100.0f, static_cast<float>(m_SurroundAudioGain) / 100.0f);
	m_pViewer->SetDualMonoMode(m_DualMonoMode);
	m_pViewer->SetSPDIFOptions(m_SPDIFOptions);

	return true;
}


bool CCoreEngine::CloseMediaViewer()
{
	return CloseViewer();
}


bool CCoreEngine::EnableMediaViewer(bool fEnable)
{
	if (!EnableViewer(fEnable))
		return false;
	if (fEnable)
		m_pViewer->SetVolume(m_fMute ? -100.0f : LevelToDeciBel(m_Volume));
	return true;
}


bool CCoreEngine::AddEventListener(EventListener *pEventListener)
{
	return m_EventListenerList.AddEventListener(pEventListener);
}


bool CCoreEngine::RemoveEventListener(EventListener *pEventListener)
{
	return m_EventListenerList.RemoveEventListener(pEventListener);
}


bool CCoreEngine::SetDriverDirectory(LPCTSTR pszDirectory)
{
	if (IsStringEmpty(pszDirectory)) {
		m_DriverDirectory.clear();
	} else {
		m_DriverDirectory = pszDirectory;
	}
	return true;
}


bool CCoreEngine::GetDriverDirectoryPath(String *pDirectory) const
{
	if (pDirectory == nullptr)
		return false;

	pDirectory->clear();

	if (!m_DriverDirectory.empty()) {
		if (m_DriverDirectory.IsRelative()) {
			if (!GetAbsolutePath(m_DriverDirectory, pDirectory))
				return false;
		} else {
			*pDirectory = m_DriverDirectory;
		}
	} else {
		TCHAR szDir[MAX_PATH];
		const DWORD Length = ::GetModuleFileName(nullptr, szDir, lengthof(szDir));
		if (Length == 0 || Length >= lengthof(szDir) - 1)
			return false;
		::PathRemoveFileSpec(szDir);
		*pDirectory = szDir;
	}

	return true;
}


bool CCoreEngine::SetDriverFileName(LPCTSTR pszFileName)
{
	if (IsStringEmpty(pszFileName)) {
		m_DriverFileName.clear();
	} else {
		m_DriverFileName = pszFileName;
	}
	return true;
}


bool CCoreEngine::GetDriverPath(String *pPath) const
{
	if (pPath == nullptr)
		return false;

	pPath->clear();

	if (m_DriverFileName.empty())
		return false;

	if (m_DriverFileName.IsRelative()) {
		String Dir;

		if (!GetDriverDirectoryPath(&Dir)
				|| !m_DriverFileName.GetAbsolute(pPath, Dir))
			return false;
	} else {
		*pPath = m_DriverFileName;
	}

	return true;
}


bool CCoreEngine::GetDriverPath(LPTSTR pszPath, int MaxLength) const
{
	if ((pszPath == nullptr) || (MaxLength < 1))
		return false;

	String Path;
	if (!GetDriverPath(&Path) || (Path.length() >= static_cast<size_t>(MaxLength))) {
		pszPath[0] = _T('\0');
		return false;
	}

	StringCopy(pszPath, Path.c_str());

	return true;
}


bool CCoreEngine::OpenTuner()
{
	TRACE(TEXT("CCoreEngine::OpenTuner()\n"));

	if (IsTunerOpen()) {
		SetError(std::errc::operation_in_progress, TEXT("チューナが既に開かれています。"));
		return false;
	}

	String DriverPath;
	if (!GetDriverPath(&DriverPath)) {
		SetError(std::errc::operation_not_permitted, TEXT("BonDriverのパスを取得できません。"));
		return false;
	}

	if (!OpenSource(DriverPath.c_str())) {
		return false;
	}

	LPCWSTR pszName = static_cast<LibISDB::BonDriverSourceFilter*>(m_pSource)->GetTunerName();
	m_DriverType = DriverType::Unknown;
	if (pszName != nullptr) {
		if (std::wcsncmp(pszName, L"UDP/", 4) == 0)
			m_DriverType = DriverType::UDP;
		else if (std::wcsncmp(pszName, L"TCP", 3) == 0)
			m_DriverType = DriverType::TCP;
	}

	return true;
}


bool CCoreEngine::CloseTuner()
{
	TRACE(TEXT("CCoreEngine::CloseTuner()\n"));

	return CloseSource();
}


bool CCoreEngine::IsTunerOpen() const
{
	return IsSourceOpen();
}


bool CCoreEngine::SetPacketBufferLength(DWORD BufferLength)
{
	if (m_pViewer == nullptr)
		return false;
	return m_pViewer->SetBufferSize(BufferLength);
}


bool CCoreEngine::SetPacketBufferPool(bool fBuffering, int Percentage)
{
	if (m_pViewer == nullptr)
		return false;
	if (!m_pViewer->SetInitialPoolPercentage(fBuffering ? Percentage : 0))
		return false;
	m_fPacketBuffering = fBuffering;
	return true;
}


void CCoreEngine::ResetPacketBuffer()
{
	if (m_pViewer != nullptr)
		m_pViewer->ResetBuffer();
}


bool CCoreEngine::GetVideoViewSize(int *pWidth, int *pHeight)
{
	if (m_pViewer == nullptr)
		return false;

	int Width, Height;

	if (m_pViewer->GetCroppedVideoSize(&Width, &Height)
			&& Width > 0 && Height > 0) {
		int XAspect, YAspect;

		if (m_pViewer->GetEffectiveAspectRatio(&XAspect, &YAspect)) {
			Width = Height * XAspect / YAspect;
		}
		if (pWidth)
			*pWidth = Width;
		if (pHeight)
			*pHeight = Height;
		return true;
	}

	return false;
}


bool CCoreEngine::SetPanAndScan(const PanAndScanInfo &Info)
{
	if (m_pViewer == nullptr)
		return false;

	LibISDB::ViewerFilter::ClippingInfo Clipping;

	Clipping.Left = Info.XPos;
	Clipping.Right = Info.XFactor - (Info.XPos + Info.Width);
	Clipping.HorzFactor = Info.XFactor;
	Clipping.Top = Info.YPos;
	Clipping.Bottom = Info.YFactor - (Info.YPos + Info.Height);
	Clipping.VertFactor = Info.YFactor;

	return m_pViewer->SetPanAndScan(Info.XAspect, Info.YAspect, &Clipping);
}


bool CCoreEngine::GetPanAndScan(PanAndScanInfo *pInfo) const
{
	if (pInfo == nullptr || m_pViewer == nullptr)
		return false;

	LibISDB::ViewerFilter::ClippingInfo Clipping;

	m_pViewer->GetForcedAspectRatio(&pInfo->XAspect, &pInfo->YAspect);
	m_pViewer->GetClippingInfo(&Clipping);

	pInfo->XPos = Clipping.Left;
	pInfo->YPos = Clipping.Top;
	pInfo->Width = Clipping.HorzFactor - (Clipping.Left + Clipping.Right);
	pInfo->Height = Clipping.VertFactor - (Clipping.Top + Clipping.Bottom);
	pInfo->XFactor = Clipping.HorzFactor;
	pInfo->YFactor = Clipping.VertFactor;

	return true;
}


bool CCoreEngine::SetVolume(int Volume)
{
	if (Volume < 0 || Volume > MAX_VOLUME)
		return false;
	if (m_pViewer != nullptr)
		m_pViewer->SetVolume(LevelToDeciBel(Volume));
	m_Volume = Volume;
	m_fMute = false;
	return true;
}


bool CCoreEngine::SetMute(bool fMute)
{
	if (fMute != m_fMute) {
		if (m_pViewer != nullptr)
			m_pViewer->SetVolume(fMute ? -100.0f : LevelToDeciBel(m_Volume));
		m_fMute = fMute;
	}
	return true;
}


bool CCoreEngine::SetAudioGainControl(int Gain, int SurroundGain)
{
	if (Gain < 0 || SurroundGain < 0)
		return false;
	if (Gain != m_AudioGain || SurroundGain != m_SurroundAudioGain) {
		if (m_pViewer != nullptr) {
			m_pViewer->SetAudioGainControl(
				Gain != 100 || SurroundGain != 100,
				static_cast<float>(Gain) / 100.0f, static_cast<float>(SurroundGain) / 100.0f);
		}
		m_AudioGain = Gain;
		m_SurroundAudioGain = SurroundGain;
	}
	return true;
}


bool CCoreEngine::GetAudioGainControl(int *pGain, int *pSurroundGain) const
{
	if (pGain)
		*pGain = m_AudioGain;
	if (pSurroundGain)
		*pSurroundGain = m_SurroundAudioGain;
	return true;
}


bool CCoreEngine::SetDualMonoMode(LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode Mode)
{
	m_DualMonoMode = Mode;
	if (m_pViewer != nullptr)
		m_pViewer->SetDualMonoMode(Mode);
	return true;
}


bool CCoreEngine::SetSPDIFOptions(const LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions &Options)
{
	m_SPDIFOptions = Options;
	if (m_pViewer != nullptr)
		m_pViewer->SetSPDIFOptions(Options);
	return true;
}


bool CCoreEngine::GetSPDIFOptions(LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions *pOptions) const
{
	if (pOptions == nullptr)
		return false;
	*pOptions = m_SPDIFOptions;
	return true;
}


bool CCoreEngine::IsSPDIFPassthroughEnabled() const
{
	if (IsViewerOpen())
		return m_pViewer->IsSPDIFPassthrough();
	return m_SPDIFOptions.Mode == LibISDB::DirectShow::AudioDecoderFilter::SPDIFMode::Passthrough;
}


// TODO: 変化があった場合 DtvEngine 側から通知するようにする
CCoreEngine::StatusFlag CCoreEngine::UpdateAsyncStatus()
{
	StatusFlag Updated = StatusFlag::None;

	if (m_pViewer != nullptr) {
		int Width, Height;

		if (m_pViewer->GetOriginalVideoSize(&Width, &Height)) {
			if (Width != m_OriginalVideoWidth || Height != m_OriginalVideoHeight) {
				m_OriginalVideoWidth = Width;
				m_OriginalVideoHeight = Height;
				Updated |= StatusFlag::VideoSize;
			}
		}

		if (m_pViewer->GetCroppedVideoSize(&Width, &Height)) {
			if (Width != m_DisplayVideoWidth || Height != m_DisplayVideoHeight) {
				m_DisplayVideoWidth = Width;
				m_DisplayVideoHeight = Height;
				Updated |= StatusFlag::VideoSize;
			}
		}

		const int NumAudioChannels = m_pViewer->GetAudioChannelCount();
		if (NumAudioChannels != m_NumAudioChannels) {
			m_NumAudioChannels = NumAudioChannels;
			Updated |= StatusFlag::AudioChannels;
			TRACE(TEXT("Audio channels = {}ch\n"), NumAudioChannels);
		}

		const bool fSPDIFPassthrough = m_pViewer->IsSPDIFPassthrough();
		if (fSPDIFPassthrough != m_fSPDIFPassthrough) {
			m_fSPDIFPassthrough = fSPDIFPassthrough;
			Updated |= StatusFlag::SPDIFPassthrough;
			TRACE(TEXT("S/PDIF passthrough {}\n"), fSPDIFPassthrough ? TEXT("ON") : TEXT("OFF"));
		}
	}

	const int NumAudioStreams = GetAudioStreamCount();
	if (NumAudioStreams != m_NumAudioStreams) {
		m_NumAudioStreams = NumAudioStreams;
		Updated |= StatusFlag::AudioStreams;
		TRACE(TEXT("Audio streams = {}ch\n"), NumAudioStreams);
	}

	if (m_pAnalyzer != nullptr) {
		const BYTE AudioComponentType = m_pAnalyzer->GetAudioComponentType(
			m_pAnalyzer->GetServiceIndexByID(m_CurServiceID), m_CurAudioStream);
		if (AudioComponentType != m_AudioComponentType) {
			m_AudioComponentType = AudioComponentType;
			Updated |= StatusFlag::AudioComponentType;
			TRACE(TEXT("AudioComponentType = {:x}\n"), AudioComponentType);
		}
	}

	Updated |= static_cast<StatusFlag>(m_AsyncStatusUpdatedFlags.exchange(0));

	return Updated;
}


void CCoreEngine::SetAsyncStatusUpdatedFlag(StatusFlag Status)
{
	m_AsyncStatusUpdatedFlags.fetch_or(static_cast<std::underlying_type_t<StatusFlag>>(Status));
}


CCoreEngine::StatisticsFlag CCoreEngine::UpdateStatistics()
{
	StatisticsFlag Updated = StatisticsFlag::None;

	const LibISDB::TSPacketParserFilter *pParser = GetFilter<LibISDB::TSPacketParserFilter>();
	if (pParser != nullptr) {
		const LibISDB::TSPacketParserFilter::PacketCountInfo Count = pParser->GetPacketCount();
		if (Count.TransportError + Count.FormatError != m_ErrorPacketCount) {
			m_ErrorPacketCount = Count.TransportError + Count.FormatError;
			Updated |= StatisticsFlag::ErrorPacketCount;
		}
		if (Count.ContinuityError != m_ContinuityErrorPacketCount) {
			m_ContinuityErrorPacketCount = Count.ContinuityError;
			Updated |= StatisticsFlag::ContinuityErrorPacketCount;
		}
	}

	const LibISDB::TSPacketCounterFilter *pCounter = GetFilter<LibISDB::TSPacketCounterFilter>();
	if (pCounter != nullptr) {
		const unsigned long long ScrambledCount = pCounter->GetScrambledPacketCount();
		if (ScrambledCount != m_ScrambledPacketCount) {
			m_ScrambledPacketCount = ScrambledCount;
			Updated |= StatisticsFlag::ScrambledPacketCount;
		}
	}

	float SignalLevel;
	unsigned long BitRate;
	uint32_t StreamRemain;
	const LibISDB::BonDriverSourceFilter *pSource = GetFilter<LibISDB::BonDriverSourceFilter>();
	if (pSource != nullptr) {
		SignalLevel = pSource->GetSignalLevel();
		BitRate = pSource->GetBitRate();
		StreamRemain = pSource->GetStreamRemain();
	} else {
		SignalLevel = 0.0f;
		BitRate = 0;
		StreamRemain = 0;
	}
	if (SignalLevel != m_SignalLevel) {
		m_SignalLevel = SignalLevel;
		Updated |= StatisticsFlag::SignalLevel;
	}
	if (BitRate != m_BitRate) {
		m_BitRate = BitRate;
		Updated |= StatisticsFlag::BitRate;
	}
	if (StreamRemain != m_StreamRemain) {
		m_StreamRemain = StreamRemain;
		Updated |= StatisticsFlag::StreamRemain;
	}

	int BufferFillPercentage;
	if (m_pViewer != nullptr)
		BufferFillPercentage = m_pViewer->GetBufferFillPercentage();
	else
		BufferFillPercentage = 0;
	if (BufferFillPercentage != m_PacketBufferFillPercentage) {
		m_PacketBufferFillPercentage = BufferFillPercentage;
		Updated |= StatisticsFlag::PacketBufferRate;
	}

	return Updated;
}


void CCoreEngine::ResetErrorCount()
{
	LibISDB::TSPacketParserFilter *pParser = GetFilter<LibISDB::TSPacketParserFilter>();
	if (pParser != nullptr)
		pParser->ResetErrorPacketCount();
	m_ErrorPacketCount = 0;
	m_ContinuityErrorPacketCount = 0;
	LibISDB::TSPacketCounterFilter *pCounter = GetFilter<LibISDB::TSPacketCounterFilter>();
	if (pCounter != nullptr)
		pCounter->ResetScrambledPacketCount();
	m_ScrambledPacketCount = 0;
}


int CCoreEngine::GetSignalLevelText(LPTSTR pszText, int MaxLength) const
{
	return GetSignalLevelText(m_SignalLevel, pszText, MaxLength);
}


int CCoreEngine::GetSignalLevelText(float SignalLevel, LPTSTR pszText, int MaxLength) const
{
	return static_cast<int>(StringFormat(pszText, MaxLength, TEXT("{:.2f} dB"), SignalLevel));
}


int CCoreEngine::GetBitRateText(LPTSTR pszText, int MaxLength) const
{
	return GetBitRateText(GetBitRateFloat(), pszText, MaxLength);
}


int CCoreEngine::GetBitRateText(unsigned long BitRate, LPTSTR pszText, int MaxLength) const
{
	return GetBitRateText(BitRateToFloat(BitRate), pszText, MaxLength);
}


int CCoreEngine::GetBitRateText(float BitRate, LPTSTR pszText, int MaxLength, int Precision) const
{
	return static_cast<int>(StringFormat(pszText, MaxLength, TEXT("{:.{}f} Mbps"), BitRate, Precision));
}


int CCoreEngine::GetPacketBufferUsedPercentage() const
{
	return m_PacketBufferFillPercentage;
}


bool CCoreEngine::GetCurrentEventInfo(LibISDB::EventInfo *pInfo, uint16_t ServiceID, bool fNext)
{
	if (pInfo == nullptr || m_pAnalyzer == nullptr)
		return false;

	if (ServiceID == LibISDB::SERVICE_ID_INVALID) {
		if (m_CurServiceID == LibISDB::SERVICE_ID_INVALID)
			return false;
		ServiceID = m_CurServiceID;
	}

	const int ServiceIndex = m_pAnalyzer->GetServiceIndexByID(ServiceID);
	if (ServiceIndex < 0)
		return false;

	return m_pAnalyzer->GetEventInfo(ServiceIndex, pInfo, true, fNext);
}


LibISDB::COMMemoryPointer<> CCoreEngine::GetCurrentImage()
{
	if (m_pViewer == nullptr)
		return nullptr;

	const bool fPause = m_pViewer->GetVideoRendererType() == LibISDB::DirectShow::VideoRenderer::RendererType::Default;

	if (fPause)
		m_pViewer->Pause();

	LibISDB::COMMemoryPointer<> Image(m_pViewer->GetCurrentImage());

	if (fPause)
		m_pViewer->Play();

	return std::move(Image);
}


bool CCoreEngine::SetMinTimerResolution(bool fMin)
{
	if ((m_TimerResolution != 0) != fMin) {
		if (fMin) {
			TIMECAPS tc;

			if (::timeGetDevCaps(&tc, sizeof(tc)) != TIMERR_NOERROR)
				tc.wPeriodMin = 1;
			if (::timeBeginPeriod(tc.wPeriodMin) != TIMERR_NOERROR)
				return false;
			m_TimerResolution = tc.wPeriodMin;
			TRACE(TEXT("CCoreEngine::SetMinTimerResolution() Set {}\n"), m_TimerResolution);
		} else {
			::timeEndPeriod(m_TimerResolution);
			m_TimerResolution = 0;
			TRACE(TEXT("CCoreEngine::SetMinTimerResolution() Reset\n"));
		}
	}
	return true;
}


void CCoreEngine::OnServiceChanged(uint16_t ServiceID)
{
	m_EventListenerList.CallEventListener(&CCoreEngine::EventListener::OnServiceChanged, ServiceID);
}


void CCoreEngine::OnEventChanged(uint16_t EventID)
{
	m_EventListenerList.CallEventListener(&CCoreEngine::EventListener::OnEventChanged, m_pAnalyzer, EventID);
}


void CCoreEngine::OnVideoStreamTypeChanged(uint8_t StreamType)
{
	m_EventListenerList.CallEventListener(&CCoreEngine::EventListener::OnVideoStreamTypeChanged, StreamType);
}


void CCoreEngine::OnPATUpdated(LibISDB::AnalyzerFilter *pAnalyzer)
{
	const uint16_t OldTSID = m_CurTransportStreamID;
	ViewerEngine::OnPATUpdated(pAnalyzer);
	m_EventListenerList.CallEventListener(&CCoreEngine::EventListener::OnPATUpdated, pAnalyzer, OldTSID != m_CurTransportStreamID);
}


void CCoreEngine::OnPMTUpdated(LibISDB::AnalyzerFilter *pAnalyzer, uint16_t ServiceID)
{
	ViewerEngine::OnPMTUpdated(pAnalyzer, ServiceID);
	m_EventListenerList.CallEventListener(&CCoreEngine::EventListener::OnPMTUpdated, pAnalyzer, ServiceID);
}


void CCoreEngine::OnSDTUpdated(LibISDB::AnalyzerFilter *pAnalyzer)
{
	ViewerEngine::OnEITUpdated(pAnalyzer);
	m_EventListenerList.CallEventListener(&CCoreEngine::EventListener::OnSDTUpdated, pAnalyzer);
}


void CCoreEngine::OnEITUpdated(LibISDB::AnalyzerFilter *pAnalyzer)
{
	ViewerEngine::OnEITUpdated(pAnalyzer);
	m_EventListenerList.CallEventListener(&CCoreEngine::EventListener::OnEventUpdated, pAnalyzer);
}


void CCoreEngine::OnTOTUpdated(LibISDB::AnalyzerFilter *pAnalyzer)
{
	m_EventListenerList.CallEventListener(&CCoreEngine::EventListener::OnTOTUpdated, pAnalyzer);
}


void CCoreEngine::OnVideoSizeChanged(LibISDB::ViewerFilter *pViewer, const LibISDB::DirectShow::VideoParser::VideoInfo &Info)
{
	m_EventListenerList.CallEventListener(&CCoreEngine::EventListener::OnVideoSizeChanged, pViewer);
}


void CCoreEngine::OnFilterGraphInitialize(LibISDB::ViewerFilter *pViewer, IGraphBuilder *pGraphBuilder)
{
	m_EventListenerList.CallEventListener(&CCoreEngine::EventListener::OnFilterGraphInitialize, pViewer, pGraphBuilder);
}


void CCoreEngine::OnFilterGraphInitialized(LibISDB::ViewerFilter *pViewer, IGraphBuilder *pGraphBuilder)
{
	m_EventListenerList.CallEventListener(&CCoreEngine::EventListener::OnFilterGraphInitialized, pViewer, pGraphBuilder);
}


void CCoreEngine::OnFilterGraphFinalize(LibISDB::ViewerFilter *pViewer, IGraphBuilder *pGraphBuilder)
{
	m_EventListenerList.CallEventListener(&CCoreEngine::EventListener::OnFilterGraphFinalize, pViewer, pGraphBuilder);
}


void CCoreEngine::OnFilterGraphFinalized(LibISDB::ViewerFilter *pViewer, IGraphBuilder *pGraphBuilder)
{
	m_EventListenerList.CallEventListener(&CCoreEngine::EventListener::OnFilterGraphFinalized, pViewer, pGraphBuilder);
}


void CCoreEngine::OnSPDIFPassthroughError(LibISDB::ViewerFilter *pViewer, HRESULT hr)
{
	m_EventListenerList.CallEventListener(&CCoreEngine::EventListener::OnSPDIFPassthroughError, pViewer, hr);
}


void CCoreEngine::OnWriteError(LibISDB::RecorderFilter *pRecorder, LibISDB::RecorderFilter::RecordingTask *pTask)
{
	m_EventListenerList.CallEventListener(&CCoreEngine::EventListener::OnWriteError, pRecorder);
}


} // namespace TVTest
