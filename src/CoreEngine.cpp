#include "stdafx.h"
#include "TVTest.h"
#include "CoreEngine.h"
#include <intrin.h>
#pragma intrinsic(_InterlockedOr)
#include "Common/DebugDef.h"




CCoreEngine::CCoreEngine()
	: m_DriverType(DRIVER_UNKNOWN)

	, m_fEnableTSProcessor(true)

	, m_fPacketBuffering(false)

	, m_OriginalVideoWidth(0)
	, m_OriginalVideoHeight(0)
	, m_DisplayVideoWidth(0)
	, m_DisplayVideoHeight(0)
	, m_NumAudioChannels(CMediaViewer::AUDIO_CHANNEL_INVALID)
	, m_NumAudioStreams(0)
	, m_AudioComponentType(0)
	, m_fMute(false)
	, m_Volume(50)
	, m_AudioGain(100)
	, m_SurroundAudioGain(100)
	, m_DualMonoMode(CAudioDecFilter::DUALMONO_MAIN)
	, m_StereoMode(CAudioDecFilter::STEREOMODE_STEREO)
	, m_fSpdifPassthrough(false)
	, m_ErrorPacketCount(0)
	, m_ContinuityErrorPacketCount(0)
	, m_ScramblePacketCount(0)
	, m_SignalLevel(0.0)
	, m_BitRate(0)
	, m_PacketBufferFillPercentage(0)
	, m_StreamRemain(0)
	, m_TimerResolution(0)
	, m_fNoEpg(false)

	, m_AsyncStatusUpdatedFlags(0)
{
	m_szDriverDirectory[0]='\0';
	m_szDriverFileName[0]='\0';
}


CCoreEngine::~CCoreEngine()
{
	Close();

	if (m_TimerResolution!=0)
		::timeEndPeriod(m_TimerResolution);
}


void CCoreEngine::Close()
{
	m_DtvEngine.CloseEngine();

	if (!m_TSProcessorList.empty()) {
		for (auto it=m_TSProcessorList.begin();it!=m_TSProcessorList.end();++it)
			it->pTSProcessor->Release();
		m_TSProcessorList.clear();
	}
}


bool CCoreEngine::BuildDtvEngine(CDtvEngine::CEventHandler *pEventHandler)
{
	TSProcessorConnectionList ConnectionList;
	int DecoderID,OutputIndex;

	DecoderID=CDtvEngine::DECODER_ID_BonSrcDecoder;
	ConnectTSProcessor(&ConnectionList,TSPROCESSOR_CONNECTPOSITION_SOURCE,&DecoderID);
	ConnectionList.Add(DecoderID,
					   CDtvEngine::DECODER_ID_TsPacketParser);
	DecoderID=CDtvEngine::DECODER_ID_TsPacketParser;
	ConnectTSProcessor(&ConnectionList,TSPROCESSOR_CONNECTPOSITION_PREPROCESSING,&DecoderID);
	ConnectionList.Add(DecoderID,
					   CDtvEngine::DECODER_ID_TsAnalyzer);
	DecoderID=CDtvEngine::DECODER_ID_TsAnalyzer;
	ConnectTSProcessor(&ConnectionList,TSPROCESSOR_CONNECTPOSITION_POSTPROCESSING,&DecoderID);
	ConnectionList.Add(DecoderID,
					   CDtvEngine::DECODER_ID_MediaTee);
	if (!m_fNoEpg) {
		ConnectionList.Add(CDtvEngine::DECODER_ID_MediaTee,
						   CDtvEngine::DECODER_ID_EventManager,0);
		ConnectionList.Add(CDtvEngine::DECODER_ID_EventManager,
						   CDtvEngine::DECODER_ID_LogoDownloader);
	} else {
		ConnectionList.Add(CDtvEngine::DECODER_ID_MediaTee,
						   CDtvEngine::DECODER_ID_LogoDownloader,0);
	}
	DecoderID=CDtvEngine::DECODER_ID_LogoDownloader;
	ConnectTSProcessor(&ConnectionList,TSPROCESSOR_CONNECTPOSITION_RECORDER,&DecoderID);
	ConnectionList.Add(DecoderID,
					   CDtvEngine::DECODER_ID_MediaGrabber);
	ConnectionList.Add(CDtvEngine::DECODER_ID_MediaGrabber,
					   CDtvEngine::DECODER_ID_TsSelector);
	ConnectionList.Add(CDtvEngine::DECODER_ID_TsSelector,
					   CDtvEngine::DECODER_ID_TsRecorder);
	DecoderID=CDtvEngine::DECODER_ID_MediaTee;
	OutputIndex=1;
	ConnectTSProcessor(&ConnectionList,TSPROCESSOR_CONNECTPOSITION_VIEWER,&DecoderID,&OutputIndex);
	ConnectionList.Add(DecoderID,
					   CDtvEngine::DECODER_ID_TsPacketCounter,OutputIndex);
	ConnectionList.Add(CDtvEngine::DECODER_ID_TsPacketCounter,
					   CDtvEngine::DECODER_ID_CaptionDecoder);
	ConnectionList.Add(CDtvEngine::DECODER_ID_CaptionDecoder,
					   CDtvEngine::DECODER_ID_MediaViewer);

	if (!m_DtvEngine.BuildEngine(
			ConnectionList.List.data(),static_cast<int>(ConnectionList.List.size()),
			pEventHandler)) {
		return false;
	}

	return true;
}


void CCoreEngine::ConnectTSProcessor(TSProcessorConnectionList *pList,
									 TSProcessorConnectPosition ConnectPosition,
									 int *pDecoderID,int *pOutputIndex)
{
	if (!m_fEnableTSProcessor)
		return;

	int DecoderID=*pDecoderID;
	int OutputIndex=pOutputIndex!=NULL?*pOutputIndex:0;

	for (auto it=m_TSProcessorList.begin();it!=m_TSProcessorList.end();++it) {
		if (it->ConnectPosition==ConnectPosition) {
			pList->Add(DecoderID,it->DecoderID,OutputIndex);
			DecoderID=it->DecoderID;
			OutputIndex=0;
		}
	}

	*pDecoderID=DecoderID;
	if (pOutputIndex!=NULL)
		*pOutputIndex=OutputIndex;
}


TVTest::CTSProcessor *CCoreEngine::GetTSProcessorByIndex(size_t Index)
{
	if (Index>=m_TSProcessorList.size())
		return NULL;
	return m_TSProcessorList[Index].pTSProcessor;
}


bool CCoreEngine::RegisterTSProcessor(TVTest::CTSProcessor *pTSProcessor,
									  TSProcessorConnectPosition ConnectPosition)
{
	if (pTSProcessor==NULL)
		return false;

	TSProcessorInfo Info;

	Info.pTSProcessor=pTSProcessor;
	Info.ConnectPosition=ConnectPosition;
	Info.DecoderID=m_DtvEngine.RegisterDecoder(pTSProcessor);

	if (ConnectPosition==TSPROCESSOR_CONNECTPOSITION_SOURCE)
		pTSProcessor->SetSourceProcessor(true);

	m_TSProcessorList.push_back(Info);

	return true;
}


void CCoreEngine::EnableTSProcessor(bool fEnable)
{
	m_fEnableTSProcessor=fEnable;
}


bool CCoreEngine::BuildMediaViewer(HWND hwndHost,HWND hwndMessage,
	CVideoRenderer::RendererType VideoRenderer,
	BYTE VideoStreamType,LPCWSTR pszVideoDecoder,
	LPCWSTR pszAudioDevice)
{
	if (!m_DtvEngine.m_MediaViewer.IsOpen()) {
		if (!m_DtvEngine.BuildMediaViewer(hwndHost,hwndMessage,VideoRenderer,
										  VideoStreamType,pszVideoDecoder,
										  pszAudioDevice)) {
			SetError(m_DtvEngine.GetLastErrorException());
			return false;
		}
	} else {
		if (!m_DtvEngine.RebuildMediaViewer(hwndHost,hwndMessage,VideoRenderer,
											VideoStreamType,pszVideoDecoder,
											pszAudioDevice)) {
			SetError(m_DtvEngine.GetLastErrorException());
			return false;
		}
	}
	m_DtvEngine.m_MediaViewer.SetVolume(m_fMute?-100.0f:LevelToDeciBel(m_Volume));
	m_DtvEngine.m_MediaViewer.SetAudioGainControl(
		m_AudioGain!=100 || m_SurroundAudioGain!=100,
		(float)m_AudioGain/100.0f,(float)m_SurroundAudioGain/100.0f);
	m_DtvEngine.m_MediaViewer.SetStereoMode(m_StereoMode);
	m_DtvEngine.m_MediaViewer.SetDualMonoMode(m_DualMonoMode);
	m_DtvEngine.m_MediaViewer.SetSpdifOptions(&m_SpdifOptions);
	return true;
}


bool CCoreEngine::CloseMediaViewer()
{
	return m_DtvEngine.CloseMediaViewer();
}


bool CCoreEngine::EnableMediaViewer(bool fEnable)
{
	if (!m_DtvEngine.EnableMediaViewer(fEnable))
		return false;
	if (fEnable)
		m_DtvEngine.m_MediaViewer.SetVolume(m_fMute?-100.0f:LevelToDeciBel(m_Volume));
	return true;
}


bool CCoreEngine::GetDriverDirectory(LPTSTR pszDirectory,int MaxLength) const
{
	if (pszDirectory==NULL || MaxLength<1)
		return false;

	pszDirectory[0]='\0';

	if (m_szDriverDirectory[0]!='\0') {
		if (::PathIsRelative(m_szDriverDirectory)) {
			TCHAR szBaseDir[MAX_PATH],szPath[MAX_PATH];

			::GetModuleFileName(NULL,szBaseDir,lengthof(szBaseDir));
			::PathRemoveFileSpec(szBaseDir);
			if (::PathCombine(szPath,szBaseDir,m_szDriverDirectory)==NULL
					|| ::lstrlen(szPath)>=MaxLength)
				return false;
			::lstrcpy(pszDirectory,szPath);
		} else {
			if (::lstrlen(m_szDriverDirectory)>=MaxLength)
				return false;
			::lstrcpy(pszDirectory,m_szDriverDirectory);
		}
	} else {
		DWORD Length=::GetModuleFileName(NULL,pszDirectory,MaxLength);
		if (Length==0 || Length>=(DWORD)(MaxLength-1))
			return false;
		::PathRemoveFileSpec(pszDirectory);
	}

	return true;
}


bool CCoreEngine::SetDriverDirectory(LPCTSTR pszDirectory)
{
	if (pszDirectory==NULL) {
		m_szDriverDirectory[0]='\0';
	} else {
		if (::lstrlen(pszDirectory)>=MAX_PATH)
			return false;
		::lstrcpy(m_szDriverDirectory,pszDirectory);
	}
	return true;
}


bool CCoreEngine::SetDriverFileName(LPCTSTR pszFileName)
{
	if (IsStringEmpty(pszFileName)) {
		m_szDriverFileName[0]='\0';
	} else {
		if (::lstrlen(pszFileName)>=MAX_PATH)
			return false;
		::lstrcpy(m_szDriverFileName,pszFileName);
	}
	return true;
}


bool CCoreEngine::GetDriverPath(LPTSTR pszPath,int MaxLength) const
{
	if (pszPath==NULL || MaxLength<1)
		return false;

	pszPath[0]='\0';

	if (m_szDriverFileName[0]=='\0')
		return false;

	if (::PathIsRelative(m_szDriverFileName)) {
		TCHAR szDir[MAX_PATH],szPath[MAX_PATH];

		if (!GetDriverDirectory(szDir,lengthof(szDir))
				|| ::PathCombine(szPath,szDir,m_szDriverFileName)==NULL
				|| ::lstrlen(szPath)>=MaxLength)
			return false;
		::lstrcpy(pszPath,szPath);
	} else {
		if (::lstrlen(m_szDriverFileName)>=MaxLength)
			return false;
		::lstrcpy(pszPath,m_szDriverFileName);
	}

	return true;
}


bool CCoreEngine::OpenTuner()
{
	TRACE(TEXT("CCoreEngine::OpenTuner()\n"));

	if (IsTunerOpen()) {
		SetError(TEXT("チューナが既に開かれています。"));
		return false;
	}

	TCHAR szDriverPath[MAX_PATH];
	if (!GetDriverPath(szDriverPath,lengthof(szDriverPath))) {
		SetError(TEXT("BonDriverのパスを取得できません。"));
		return false;
	}

	if (!m_DtvEngine.OpenBonDriver(szDriverPath)) {
		SetError(m_DtvEngine.GetLastErrorException());
		return false;
	}

	LPCTSTR pszName=m_DtvEngine.m_BonSrcDecoder.GetTunerName();
	m_DriverType=DRIVER_UNKNOWN;
	if (pszName!=NULL) {
		if (::_tcsncmp(pszName,TEXT("UDP/"),4)==0)
			m_DriverType=DRIVER_UDP;
		else if (::_tcsncmp(pszName,TEXT("TCP"),3)==0)
			m_DriverType=DRIVER_TCP;
	}

	return true;
}


bool CCoreEngine::CloseTuner()
{
	TRACE(TEXT("CCoreEngine::CloseTuner()\n"));

	return m_DtvEngine.ReleaseSrcFilter();
}


bool CCoreEngine::IsTunerOpen() const
{
	return m_DtvEngine.IsSrcFilterOpen();
}


bool CCoreEngine::SetPacketBufferLength(DWORD BufferLength)
{
	return m_DtvEngine.m_MediaViewer.SetBufferSize(BufferLength);
}


bool CCoreEngine::SetPacketBufferPool(bool fBuffering,int Percentage)
{
	if (!m_DtvEngine.m_MediaViewer.SetInitialPoolPercentage(fBuffering?Percentage:0))
		return false;
	m_fPacketBuffering=fBuffering;
	return true;
}


void CCoreEngine::ResetPacketBuffer()
{
	m_DtvEngine.m_MediaViewer.ResetBuffer();
}


bool CCoreEngine::GetVideoViewSize(int *pWidth,int *pHeight)
{
	WORD Width,Height;

	if (m_DtvEngine.m_MediaViewer.GetCroppedVideoSize(&Width,&Height)
			&& Width>0 && Height>0) {
		BYTE XAspect,YAspect;

		if (m_DtvEngine.m_MediaViewer.GetEffectiveAspectRatio(&XAspect,&YAspect)) {
			Width=Height*XAspect/YAspect;
		}
		if (pWidth)
			*pWidth=Width;
		if (pHeight)
			*pHeight=Height;
		return true;
	}
	return false;
}


bool CCoreEngine::SetPanAndScan(const PanAndScanInfo &Info)
{
	CMediaViewer::ClippingInfo Clipping;

	Clipping.Left=Info.XPos;
	Clipping.Right=Info.XFactor-(Info.XPos+Info.Width);
	Clipping.HorzFactor=Info.XFactor;
	Clipping.Top=Info.YPos;
	Clipping.Bottom=Info.YFactor-(Info.YPos+Info.Height);
	Clipping.VertFactor=Info.YFactor;

	return m_DtvEngine.m_MediaViewer.SetPanAndScan(Info.XAspect,Info.YAspect,&Clipping);
}


bool CCoreEngine::GetPanAndScan(PanAndScanInfo *pInfo) const
{
	if (pInfo==nullptr)
		return false;

	const CMediaViewer &MediaViewer=m_DtvEngine.m_MediaViewer;
	CMediaViewer::ClippingInfo Clipping;

	MediaViewer.GetForceAspectRatio(&pInfo->XAspect,&pInfo->YAspect);
	MediaViewer.GetClippingInfo(&Clipping);

	pInfo->XPos=Clipping.Left;
	pInfo->YPos=Clipping.Top;
	pInfo->Width=Clipping.HorzFactor-(Clipping.Left+Clipping.Right);
	pInfo->Height=Clipping.VertFactor-(Clipping.Top+Clipping.Bottom);
	pInfo->XFactor=Clipping.HorzFactor;
	pInfo->YFactor=Clipping.VertFactor;

	return true;
}


bool CCoreEngine::SetVolume(int Volume)
{
	if (Volume<0 || Volume>MAX_VOLUME)
		return false;
	m_DtvEngine.m_MediaViewer.SetVolume(LevelToDeciBel(Volume));
	m_Volume=Volume;
	m_fMute=false;
	return true;
}


bool CCoreEngine::SetMute(bool fMute)
{
	if (fMute!=m_fMute) {
		m_DtvEngine.m_MediaViewer.SetVolume(fMute?-100.0f:LevelToDeciBel(m_Volume));
		m_fMute=fMute;
	}
	return true;
}


bool CCoreEngine::SetAudioGainControl(int Gain,int SurroundGain)
{
	if (Gain<0 || SurroundGain<0)
		return false;
	if (Gain!=m_AudioGain || SurroundGain!=m_SurroundAudioGain) {
		m_DtvEngine.m_MediaViewer.SetAudioGainControl(
			Gain!=100 || SurroundGain!=100,
			(float)Gain/100.0f,(float)SurroundGain/100.0f);
		m_AudioGain=Gain;
		m_SurroundAudioGain=SurroundGain;
	}
	return true;
}


bool CCoreEngine::GetAudioGainControl(int *pGain,int *pSurroundGain) const
{
	if (pGain)
		*pGain=m_AudioGain;
	if (pSurroundGain)
		*pSurroundGain=m_SurroundAudioGain;
	return true;
}


bool CCoreEngine::SetDualMonoMode(CAudioDecFilter::DualMonoMode Mode)
{
	m_DualMonoMode=Mode;
	m_DtvEngine.m_MediaViewer.SetDualMonoMode(Mode);
	return true;
}


bool CCoreEngine::SetStereoMode(CAudioDecFilter::StereoMode Mode)
{
	m_StereoMode=Mode;
	m_DtvEngine.m_MediaViewer.SetStereoMode(Mode);
	return true;
}


bool CCoreEngine::SetSpdifOptions(const CAudioDecFilter::SpdifOptions &Options)
{
	m_SpdifOptions=Options;
	m_DtvEngine.m_MediaViewer.SetSpdifOptions(&Options);
	return true;
}


bool CCoreEngine::GetSpdifOptions(CAudioDecFilter::SpdifOptions *pOptions) const
{
	if (pOptions==NULL)
		return false;
	*pOptions=m_SpdifOptions;
	return true;
}


bool CCoreEngine::IsSpdifPassthroughEnabled() const
{
	if (m_DtvEngine.m_MediaViewer.IsOpen())
		return m_DtvEngine.m_MediaViewer.IsSpdifPassthrough();
	return m_SpdifOptions.Mode==CAudioDecFilter::SPDIF_MODE_PASSTHROUGH;
}


// TODO: 変化があった場合 DtvEngine 側から通知するようにする
DWORD CCoreEngine::UpdateAsyncStatus()
{
	DWORD Updated=0;

	WORD Width,Height;
	if (m_DtvEngine.m_MediaViewer.GetOriginalVideoSize(&Width,&Height)) {
		if (Width!=m_OriginalVideoWidth || Height!=m_OriginalVideoHeight) {
			m_OriginalVideoWidth=Width;
			m_OriginalVideoHeight=Height;
			Updated|=STATUS_VIDEOSIZE;
		}
	}

	if (m_DtvEngine.m_MediaViewer.GetCroppedVideoSize(&Width,&Height)) {
		if (Width!=m_DisplayVideoWidth || Height!=m_DisplayVideoHeight) {
			m_DisplayVideoWidth=Width;
			m_DisplayVideoHeight=Height;
			Updated|=STATUS_VIDEOSIZE;
		}
	}

	int NumAudioChannels=m_DtvEngine.GetAudioChannelNum();
	if (NumAudioChannels!=m_NumAudioChannels) {
		m_NumAudioChannels=NumAudioChannels;
		Updated|=STATUS_AUDIOCHANNELS;
		TRACE(TEXT("Audio channels = %dch\n"),NumAudioChannels);
	}

	int NumAudioStreams=m_DtvEngine.GetAudioStreamNum();
	if (NumAudioStreams!=m_NumAudioStreams) {
		m_NumAudioStreams=NumAudioStreams;
		Updated|=STATUS_AUDIOSTREAMS;
		TRACE(TEXT("Audio streams = %dch\n"),NumAudioChannels);
	}

	BYTE AudioComponentType=m_DtvEngine.GetAudioComponentType();
	if (AudioComponentType!=m_AudioComponentType) {
		m_AudioComponentType=AudioComponentType;
		Updated|=STATUS_AUDIOCOMPONENTTYPE;
		TRACE(TEXT("AudioComponentType = %x\n"),AudioComponentType);
	}

	bool fSpdifPassthrough=m_DtvEngine.m_MediaViewer.IsSpdifPassthrough();
	if (fSpdifPassthrough!=m_fSpdifPassthrough) {
		m_fSpdifPassthrough=fSpdifPassthrough;
		Updated|=STATUS_SPDIFPASSTHROUGH;
		TRACE(TEXT("S/PDIF passthrough %s\n"),fSpdifPassthrough?TEXT("ON"):TEXT("OFF"));
	}

	if (m_AsyncStatusUpdatedFlags!=0) {
		Updated|=::InterlockedExchange(pointer_cast<LONG*>(&m_AsyncStatusUpdatedFlags),0);
	}

	return Updated;
}


void CCoreEngine::SetAsyncStatusUpdatedFlag(DWORD Status)
{
	_InterlockedOr(pointer_cast<long*>(&m_AsyncStatusUpdatedFlags),Status);
}


DWORD CCoreEngine::UpdateStatistics()
{
	DWORD Updated=0;

	ULONGLONG ErrorCount=m_DtvEngine.m_TsPacketParser.GetErrorPacketCount();
	if (ErrorCount!=m_ErrorPacketCount) {
		m_ErrorPacketCount=ErrorCount;
		Updated|=STATISTIC_ERRORPACKETCOUNT;
	}
	ULONGLONG ContinuityErrorCount=m_DtvEngine.m_TsPacketParser.GetContinuityErrorPacketCount();
	if (ContinuityErrorCount!=m_ContinuityErrorPacketCount) {
		m_ContinuityErrorPacketCount=ContinuityErrorCount;
		Updated|=STATISTIC_CONTINUITYERRORPACKETCOUNT;
	}
	ULONGLONG ScrambleCount=m_DtvEngine.m_TsPacketCounter.GetScrambledPacketCount();
	if (ScrambleCount!=m_ScramblePacketCount) {
		m_ScramblePacketCount=ScrambleCount;
		Updated|=STATISTIC_SCRAMBLEPACKETCOUNT;
	}
	float SignalLevel=m_DtvEngine.m_BonSrcDecoder.GetSignalLevel();
	DWORD BitRate=m_DtvEngine.m_BonSrcDecoder.GetBitRate();
	DWORD StreamRemain=m_DtvEngine.m_BonSrcDecoder.GetStreamRemain();
	if (SignalLevel!=m_SignalLevel) {
		m_SignalLevel=SignalLevel;
		Updated|=STATISTIC_SIGNALLEVEL;
	}
	if (BitRate!=m_BitRate) {
		m_BitRate=BitRate;
		Updated|=STATISTIC_BITRATE;
	}
	if (StreamRemain!=m_StreamRemain) {
		m_StreamRemain=StreamRemain;
		Updated|=STATISTIC_STREAMREMAIN;
	}
	int BufferFillPercentage=m_DtvEngine.m_MediaViewer.GetBufferFillPercentage();
	if (BufferFillPercentage!=m_PacketBufferFillPercentage) {
		m_PacketBufferFillPercentage=BufferFillPercentage;
		Updated|=STATISTIC_PACKETBUFFERRATE;
	}
	return Updated;
}


void CCoreEngine::ResetErrorCount()
{
	m_DtvEngine.m_TsPacketParser.ResetErrorPacketCount();
	m_ErrorPacketCount=0;
	m_ContinuityErrorPacketCount=0;
	m_DtvEngine.m_TsPacketCounter.ResetScrambledPacketCount();
	m_ScramblePacketCount=0;
}


int CCoreEngine::GetSignalLevelText(LPTSTR pszText,int MaxLength) const
{
	return GetSignalLevelText(m_SignalLevel,pszText,MaxLength);
}


int CCoreEngine::GetSignalLevelText(float SignalLevel,LPTSTR pszText,int MaxLength) const
{
	return StdUtil::snprintf(pszText,MaxLength,TEXT("%.2f dB"),SignalLevel);
}


int CCoreEngine::GetBitRateText(LPTSTR pszText,int MaxLength) const
{
	return GetBitRateText(GetBitRateFloat(),pszText,MaxLength);
}


int CCoreEngine::GetBitRateText(DWORD BitRate,LPTSTR pszText,int MaxLength) const
{
	return GetBitRateText(BitRateToFloat(BitRate),pszText,MaxLength);
}


int CCoreEngine::GetBitRateText(float BitRate,LPTSTR pszText,int MaxLength,int Precision) const
{
	return StdUtil::snprintf(pszText,MaxLength,TEXT("%.*f Mbps"),Precision,BitRate);
}


int CCoreEngine::GetPacketBufferUsedPercentage() const
{
	return m_PacketBufferFillPercentage;
}


bool CCoreEngine::GetCurrentEventInfo(CEventInfoData *pInfo,WORD ServiceID,bool fNext)
{
	if (pInfo==NULL)
		return false;

	if (ServiceID==0xFFFF) {
		if (!m_DtvEngine.GetServiceID(&ServiceID))
			return false;
	}
	int ServiceIndex=m_DtvEngine.m_TsAnalyzer.GetServiceIndexByID(ServiceID);
	if (ServiceIndex<0)
		return false;

	CEventInfo EventInfo;
	if (!m_DtvEngine.m_TsAnalyzer.GetEventInfo(ServiceIndex,&EventInfo,true,fNext))
		return false;
	*pInfo=EventInfo;

	return true;
}


void *CCoreEngine::GetCurrentImage()
{
	BYTE *pDib;

	bool fPause=m_DtvEngine.m_MediaViewer.GetVideoRendererType()==CVideoRenderer::RENDERER_DEFAULT;

	if (fPause)
		m_DtvEngine.m_MediaViewer.Pause();
	if (!m_DtvEngine.m_MediaViewer.GetCurrentImage(&pDib))
		pDib=NULL;
	if (fPause)
		m_DtvEngine.m_MediaViewer.Play();
	return pDib;
}


bool CCoreEngine::SetMinTimerResolution(bool fMin)
{
	if ((m_TimerResolution!=0)!=fMin) {
		if (fMin) {
			TIMECAPS tc;

			if (::timeGetDevCaps(&tc,sizeof(tc))!=TIMERR_NOERROR)
				tc.wPeriodMin=1;
			if (::timeBeginPeriod(tc.wPeriodMin)!=TIMERR_NOERROR)
				return false;
			m_TimerResolution=tc.wPeriodMin;
			TRACE(TEXT("CCoreEngine::SetMinTimerResolution() Set %u\n"),m_TimerResolution);
		} else {
			::timeEndPeriod(m_TimerResolution);
			m_TimerResolution=0;
			TRACE(TEXT("CCoreEngine::SetMinTimerResolution() Reset\n"));
		}
	}
	return true;
}
