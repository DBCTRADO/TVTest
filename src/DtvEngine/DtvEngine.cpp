// DtvEngine.cpp: CDtvEngine クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DtvEngine.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


template<typename TPred> void WalkDecoderGraph(CMediaDecoder *pDecoder, TPred Pred)
{
	Pred(pDecoder);
	const DWORD OutputNum = pDecoder->GetOutputNum();
	for (DWORD i = 0; i < OutputNum; i++) {
		CMediaDecoder *pOutDecoder = pDecoder->GetOutputDecoder(i);
		if (pOutDecoder)
			WalkDecoderGraph(pOutDecoder, Pred);
	}
}


//////////////////////////////////////////////////////////////////////
// CDtvEngine 構築/消滅
//////////////////////////////////////////////////////////////////////

CDtvEngine::CDtvEngine(void)
	: m_pEventHandler(NULL)

	, m_wCurTransportStream(0U)
	, m_CurServiceIndex(SERVICE_INVALID)
	, m_CurServiceID(SID_INVALID)
	, m_VideoStreamType(STREAM_TYPE_UNINITIALIZED)
	, m_CurVideoStream(0)
	, m_CurVideoComponentTag(CTsAnalyzer::COMPONENTTAG_INVALID)
	, m_CurAudioStream(0)
	, m_CurAudioComponentTag(CTsAnalyzer::COMPONENTTAG_INVALID)
	, m_CurEventID(CTsAnalyzer::EVENTID_INVALID)

	, m_BonSrcDecoder(this)
	, m_TsPacketParser(this)
	, m_TsAnalyzer(this)
	, m_MediaViewer(this)
	, m_MediaTee(this)
	, m_TsRecorder(this)
	, m_MediaGrabber(this)
	, m_TsSelector(this)
	, m_EventManager(this)
	, m_CaptionDecoder(this)
	, m_LogoDownloader(this)
	, m_TsPacketCounter(this)

	, m_bBuiled(false)
	, m_bStartStreamingOnDriverOpen(false)

	, m_bWriteCurServiceOnly(false)
	, m_WriteStream(CTsSelector::STREAM_ALL)
{
}


CDtvEngine::~CDtvEngine(void)
{
	CloseEngine();
}


bool CDtvEngine::BuildEngine(const DecoderConnectionInfo *pDecoderConnectionList,
							 int DecoderConnectionCount,
							 CEventHandler *pEventHandler)
{
	if (m_bBuiled)
		return true;

	Trace(TEXT("デコーダグラフを構築しています..."));

	m_DecoderConnectionList.reserve(DecoderConnectionCount);

	for (int i = 0; i < DecoderConnectionCount; i++) {
		const DecoderConnectionInfo &ConnectionInfo = pDecoderConnectionList[i];

		CMediaDecoder *pOutputDecoder = GetDecoderByID(ConnectionInfo.OutputDecoder);
		CMediaDecoder *pInputDecoder = GetDecoderByID(ConnectionInfo.InputDecoder);
		if (pOutputDecoder == NULL || pInputDecoder == NULL)
			return false;

		pOutputDecoder->SetOutputDecoder(pInputDecoder, ConnectionInfo.OutputIndex);

		m_DecoderConnectionList.push_back(ConnectionInfo);
	}

	WalkDecoderGraph(&m_BonSrcDecoder,
					 [](CMediaDecoder *pDecoder) { pDecoder->Initialize(); });

	// イベントハンドラ設定
	m_pEventHandler = pEventHandler;
	m_pEventHandler->m_pDtvEngine = this;

	m_bBuiled = true;

	return true;
}


bool CDtvEngine::CloseEngine(void)
{
	if (!m_bBuiled)
		return true;

	Trace(TEXT("DtvEngineを閉じています..."));

	//m_MediaViewer.Stop();

	ReleaseSrcFilter();

	Trace(TEXT("メディアビューアを閉じています..."));
	m_MediaViewer.CloseViewer();

	WalkDecoderGraph(&m_BonSrcDecoder,
					 [](CMediaDecoder *pDecoder) { pDecoder->Finalize(); });

	// イベントハンドラ解除
	m_pEventHandler = NULL;

	m_DecoderConnectionList.clear();

	m_bBuiled = false;

	Trace(TEXT("DtvEngineを閉じました。"));

	return true;
}


bool CDtvEngine::ResetEngine(void)
{
	if (!m_bBuiled)
		return false;

	// デコーダグラフリセット
	m_BonSrcDecoder.ResetGraph();

	return true;
}


int CDtvEngine::RegisterDecoder(CMediaDecoder *pDecoder)
{
	if (pDecoder == NULL)
		return -1;

	DecoderInfo Info;

	Info.ID = DECODER_ID_External + (int)m_DecoderList.size();
	Info.pDecoder = pDecoder;

	m_DecoderList.push_back(Info);

	pDecoder->SetEventHandler(this);

	return Info.ID;
}


bool CDtvEngine::OpenBonDriver(LPCTSTR pszFileName)
{
	ReleaseSrcFilter();

	m_BonSrcDecoder.SetOutputDecoder(NULL);

	bool bOK = false;

	// ソースフィルタを開く
	Trace(TEXT("BonDriverを読み込んでいます..."));
	if (m_BonSrcDecoder.LoadBonDriver(pszFileName)) {
		Trace(TEXT("チューナを開いています..."));
		if (m_BonSrcDecoder.OpenTuner()) {
			bOK = true;
		} else {
			SetError(m_BonSrcDecoder.GetLastErrorException());
			m_BonSrcDecoder.UnloadBonDriver();
		}
	} else {
		SetError(m_BonSrcDecoder.GetLastErrorException());
	}

	const DecoderConnectionInfo *pOutputConnection = GetOutputConnectionInfo(DECODER_ID_BonSrcDecoder);
	if (pOutputConnection != NULL) {
		m_BonSrcDecoder.SetOutputDecoder(GetDecoderByID(pOutputConnection->InputDecoder),
										 pOutputConnection->OutputIndex);
	}

	if (!bOK)
		return false;

	if (m_bStartStreamingOnDriverOpen) {
		Trace(TEXT("ストリームの再生を開始しています..."));
		if (!m_BonSrcDecoder.Play()) {
			SetError(m_BonSrcDecoder.GetLastErrorException());
			return false;
		}
	}
	//ResetEngine();
	ResetStatus();

	return true;
}


bool CDtvEngine::ReleaseSrcFilter()
{
	// ソースフィルタを開放する
	if (m_BonSrcDecoder.IsBonDriverLoaded()) {
		Trace(TEXT("BonDriver を解放しています..."));
		m_BonSrcDecoder.UnloadBonDriver();
	}

	return true;
}


bool CDtvEngine::IsSrcFilterOpen() const
{
	return m_BonSrcDecoder.IsOpen();
}


bool CDtvEngine::SetChannel(const BYTE byTuningSpace, const WORD wChannel,
							const ServiceSelectInfo *pServiceSelInfo)
{
	TRACE(TEXT("CDtvEngine::SetChannel(%d, %d, %04x)\n"),
		  byTuningSpace, wChannel,
		  pServiceSelInfo ? pServiceSelInfo->ServiceID : 0);

	m_EngineLock.Lock();
	if (pServiceSelInfo) {
		m_SetChannelServiceSel = *pServiceSelInfo;
	} else {
		m_SetChannelServiceSel.Reset();
	}
	m_EngineLock.Unlock();

	// チャンネル変更
	if (!m_BonSrcDecoder.SetChannelAndPlay(byTuningSpace, wChannel)) {
		SetError(m_BonSrcDecoder.GetLastErrorException());
		return false;
	}

	return true;
}


bool CDtvEngine::SetService(const ServiceSelectInfo *pServiceSelInfo, const bool bReserve)
{
	if (!pServiceSelInfo)
		return false;

	WORD ServiceID = pServiceSelInfo->ServiceID;

	TRACE(TEXT("CDtvEngine::SetService() : ServiceID %04x\n"), ServiceID);

	CBlockLock Lock(&m_EngineLock);

	// bReserve == true の場合、まだPATが来ていなくてもエラーにしない

	bool bSetService = true, b1Seg = false;

	if (pServiceSelInfo->OneSegSelect == ONESEG_SELECT_HIGHPRIORITY) {
		// ワンセグ優先
		WORD SID;
		if (pServiceSelInfo->PreferredServiceIndex != SERVICE_INVALID
				&& m_TsAnalyzer.Get1SegServiceIDByIndex(
					pServiceSelInfo->PreferredServiceIndex, &SID)) {
			b1Seg = true;
			ServiceID = SID;
		} else if (m_TsAnalyzer.GetFirst1SegServiceID(&SID)) {
			b1Seg = true;
			ServiceID = SID;
		}
	}

	if (!b1Seg && ServiceID != SID_DEFAULT) {
		int Index = m_TsAnalyzer.GetViewableServiceIndexByID(ServiceID);
		if (Index < 0) {
			if (!bReserve || m_wCurTransportStream != 0)
				return false;
			bSetService = false;
		}
	}

	if (bSetService)
		SelectService(ServiceID, pServiceSelInfo->OneSegSelect == ONESEG_SELECT_REFUSE);

	m_ServiceSel = *pServiceSelInfo;

	return true;
}


bool CDtvEngine::SelectService(WORD ServiceID, bool bNo1Seg)
{
	TRACE(TEXT("CDtvEngine::SelectService(%04x)\n"), ServiceID);

	CBlockLock Lock(&m_EngineLock);

	// サービス変更(ServiceID==SID_DEFAULTならPAT先頭サービス)

	if (ServiceID == SID_DEFAULT) {
		TRACE(TEXT("Select default service\n"));
		// 先頭PMTが到着するまで失敗にする
		if (!m_TsAnalyzer.GetFirstViewableServiceID(&ServiceID)) {
			TRACE(TEXT("No viewable service\n"));
			return false;
		}
		if (bNo1Seg && m_TsAnalyzer.Is1SegService(m_TsAnalyzer.GetServiceIndexByID(ServiceID))) {
			return false;
		}
	} else {
		if (m_TsAnalyzer.GetViewableServiceIndexByID(ServiceID) < 0) {
			TRACE(TEXT("Service %04x not found\n"), ServiceID);
			return false;
		}
	}

	const bool bServiceChanged = (ServiceID != m_CurServiceID);

	m_CurServiceIndex = m_TsAnalyzer.GetServiceIndexByID(ServiceID);
	m_CurServiceID = ServiceID;

	TRACE(TEXT("------- Service Select -------\n"));
	TRACE(TEXT("%d (ServiceID = %04X)\n"), m_CurServiceIndex, ServiceID);

	WORD VideoPID = CMediaViewer::PID_INVALID;
	WORD AudioPID = CMediaViewer::PID_INVALID;
	BYTE VideoStreamType = STREAM_TYPE_INVALID;

	int VideoIndex;
	if (m_CurVideoComponentTag != CTsAnalyzer::COMPONENTTAG_INVALID
			/* && !bServiceChanged */) {
		VideoIndex = m_TsAnalyzer.GetVideoIndexByComponentTag(m_CurServiceIndex, m_CurVideoComponentTag);
		if (VideoIndex < 0)
			VideoIndex = 0;
	} else {
		VideoIndex = 0;
	}
	if (!m_TsAnalyzer.GetVideoEsPID(m_CurServiceIndex, VideoIndex, &VideoPID)
			&& VideoIndex != 0) {
		m_TsAnalyzer.GetVideoEsPID(m_CurServiceIndex, 0, &VideoPID);
		VideoIndex = 0;
	}
	m_CurVideoStream = VideoIndex;
	m_CurVideoComponentTag = m_TsAnalyzer.GetVideoComponentTag(m_CurServiceIndex, VideoIndex);
	m_TsAnalyzer.GetVideoStreamType(m_CurServiceIndex, VideoIndex, &VideoStreamType);
	TRACE(TEXT("Select video : %d (component_tag %02X)\n"), m_CurVideoStream, m_CurVideoComponentTag);

	int AudioIndex;
	if (m_CurAudioComponentTag != CTsAnalyzer::COMPONENTTAG_INVALID
			/* && !bServiceChanged */) {
		AudioIndex = m_TsAnalyzer.GetAudioIndexByComponentTag(m_CurServiceIndex, m_CurAudioComponentTag);
		if (AudioIndex < 0)
			AudioIndex = 0;
	} else {
		AudioIndex = 0;
	}
	if (!m_TsAnalyzer.GetAudioEsPID(m_CurServiceIndex, AudioIndex, &AudioPID)
			&& AudioIndex != 0) {
		m_TsAnalyzer.GetAudioEsPID(m_CurServiceIndex, 0, &AudioPID);
		AudioIndex = 0;
	}
	m_CurAudioStream = AudioIndex;
	m_CurAudioComponentTag = m_TsAnalyzer.GetAudioComponentTag(m_CurServiceIndex, AudioIndex);
	TRACE(TEXT("Select audio : %d (component_tag %02X)\n"), m_CurAudioStream, m_CurAudioComponentTag);

	if (m_VideoStreamType != VideoStreamType) {
		TRACE(TEXT("Video stream_type changed (%02x -> %02x)\n"),
			  m_VideoStreamType, VideoStreamType);
		m_VideoStreamType = VideoStreamType;
		if (m_pEventHandler!=NULL)
			m_pEventHandler->OnVideoStreamTypeChanged(VideoStreamType);
	}

	m_MediaViewer.Set1SegMode(m_TsAnalyzer.Is1SegService(m_CurServiceIndex));
	m_MediaViewer.SetVideoPID(VideoPID);
	m_MediaViewer.SetAudioPID(AudioPID);

	WalkDecoderGraph(&m_BonSrcDecoder,
					 [=](CMediaDecoder *pDecoder) { pDecoder->SetActiveServiceID(ServiceID); });

	if (m_bWriteCurServiceOnly)
		SetWriteStream(ServiceID, m_WriteStream);

	m_CaptionDecoder.SetTargetStream(ServiceID);

	if (m_pEventHandler && bServiceChanged)
		m_pEventHandler->OnServiceChanged(ServiceID);

	const WORD EventID = GetEventID();
	if (m_CurEventID != EventID) {
		m_CurEventID = EventID;
		if (m_pEventHandler)
			m_pEventHandler->OnEventChanged(&m_TsAnalyzer, EventID);
	}

	return true;
}


bool CDtvEngine::GetServiceID(WORD *pServiceID) const
{
	// サービスID取得
	return m_TsAnalyzer.GetServiceID(m_CurServiceIndex, pServiceID);
}


WORD CDtvEngine::GetServiceIndex() const
{
	return m_CurServiceIndex;
}


bool CDtvEngine::SetOneSegSelectType(OneSegSelectType Type)
{
	CBlockLock Lock(&m_EngineLock);

	m_ServiceSel.OneSegSelect = Type;
	m_ServiceSel.PreferredServiceIndex = SERVICE_INVALID;

	return true;
}


WORD CDtvEngine::GetEventID(bool bNext)
{
	return m_TsAnalyzer.GetEventID(m_CurServiceIndex, bNext);
}


int CDtvEngine::GetEventName(LPTSTR pszName, int MaxLength, bool bNext)
{
	return m_TsAnalyzer.GetEventName(m_CurServiceIndex, pszName, MaxLength, bNext);
}


int CDtvEngine::GetEventText(LPTSTR pszText, int MaxLength, bool bNext)
{
	return m_TsAnalyzer.GetEventText(m_CurServiceIndex, pszText, MaxLength, bNext);
}


int CDtvEngine::GetEventExtendedText(LPTSTR pszText, int MaxLength, bool bNext)
{
	return m_TsAnalyzer.GetEventExtendedText(m_CurServiceIndex, pszText, MaxLength, true, bNext);
}


bool CDtvEngine::GetEventTime(SYSTEMTIME *pStartTime, DWORD *pDuration, bool bNext)
{
	return m_TsAnalyzer.GetEventTime(m_CurServiceIndex, pStartTime, pDuration, bNext);
}


bool CDtvEngine::GetEventSeriesInfo(CTsAnalyzer::EventSeriesInfo *pInfo, bool bNext)
{
	return m_TsAnalyzer.GetEventSeriesInfo(m_CurServiceIndex, pInfo, bNext);
}


bool CDtvEngine::GetEventInfo(CEventInfo *pInfo, bool bNext)
{
	return m_TsAnalyzer.GetEventInfo(m_CurServiceIndex, pInfo, true, bNext);
}


bool CDtvEngine::GetEventAudioInfo(CTsAnalyzer::EventAudioInfo *pInfo, const int AudioIndex, bool bNext)
{
	return m_TsAnalyzer.GetEventAudioInfo(m_CurServiceIndex, AudioIndex < 0 ? m_CurAudioStream : AudioIndex, pInfo, bNext);
}


int CDtvEngine::GetCurComponentGroup() const
{
	CBlockLock Lock(&m_EngineLock);

	if (m_CurVideoComponentTag != CTsAnalyzer::COMPONENTTAG_INVALID) {
		return m_TsAnalyzer.GetEventComponentGroupIndexByComponentTag(m_CurServiceIndex, m_CurVideoComponentTag);
	}

	return -1;
}


unsigned __int64 CDtvEngine::GetPcrTimeStamp()
{
	// PCRタイムスタンプ取得
	unsigned __int64 TimeStamp;
	if (m_TsAnalyzer.GetPcrTimeStamp(m_CurServiceIndex, &TimeStamp))
		return TimeStamp;
	return 0ULL;
}


const DWORD CDtvEngine::OnDecoderEvent(CMediaDecoder *pDecoder, const DWORD dwEventID, PVOID pParam)
{
	// デコーダからのイベントを受け取る(暫定)
	if (pDecoder == &m_BonSrcDecoder) {
		switch (dwEventID) {
		case CBonSrcDecoder::EVENT_GRAPH_RESET:
			// デコーダグラフがリセットされた
			ResetStatus();
			return 0;

		case CBonSrcDecoder::EVENT_CHANNEL_CHANGED:
			// チャンネルが変更された
			{
				CBlockLock Lock(&m_EngineLock);

				m_ServiceSel = m_SetChannelServiceSel;
				ResetStatus();
			}
			return 0;
		}
	} else if (pDecoder == &m_TsAnalyzer) {
		switch (dwEventID) {
		case CTsAnalyzer::EVENT_PAT_UPDATED:
		case CTsAnalyzer::EVENT_PMT_UPDATED:
			// サービスの構成が変化した
			{
				CBlockLock Lock(&m_EngineLock);
				WORD wTransportStream = m_TsAnalyzer.GetTransportStreamID();
				bool bStreamChanged = m_wCurTransportStream != wTransportStream;

				if (bStreamChanged) {
					// ストリームIDが変わっているなら初期化
					TRACE(TEXT("CDtvEngine ■Stream Change!! %04X\n"), wTransportStream);

					m_wCurTransportStream = wTransportStream;
					m_CurServiceIndex = SERVICE_INVALID;
					m_CurServiceID = SID_INVALID;
					m_CurVideoStream = 0;
					m_CurVideoComponentTag = CTsAnalyzer::COMPONENTTAG_INVALID;
					m_CurAudioStream = 0;
					m_CurAudioComponentTag = CTsAnalyzer::COMPONENTTAG_INVALID;
					m_CurEventID = CTsAnalyzer::EVENTID_INVALID;

					bool bSetService = true;
					WORD ServiceID = SID_DEFAULT;

					if (m_ServiceSel.OneSegSelect == ONESEG_SELECT_HIGHPRIORITY) {
						// ワンセグ優先
						// この段階ではまだ保留
						bSetService = false;
					} else if (m_ServiceSel.ServiceID != SID_INVALID) {
						// サービスが指定されている
						const int ServiceIndex = m_TsAnalyzer.GetServiceIndexByID(m_ServiceSel.ServiceID);
						if (ServiceIndex < 0) {
							// サービスがPATにない
							TRACE(TEXT("Specified service ID %04x not found in PAT\n"), m_ServiceSel.ServiceID);
							bSetService = false;
						} else {
							if (m_TsAnalyzer.GetViewableServiceIndexByID(m_ServiceSel.ServiceID) >= 0) {
								ServiceID = m_ServiceSel.ServiceID;
							} else {
								bSetService = false;
							}
						}
					}

					if (!bSetService || !SelectService(ServiceID, m_ServiceSel.OneSegSelect == ONESEG_SELECT_REFUSE)) {
						m_MediaViewer.SetVideoPID(CMediaViewer::PID_INVALID);
						m_MediaViewer.SetAudioPID(CMediaViewer::PID_INVALID);
					}
				} else {
					// ストリームIDは同じだが、構成ESのPIDが変わった可能性がある
					TRACE(TEXT("CDtvEngine ■Stream Updated!! %04X\n"), wTransportStream);

					bool bSetService = true, b1Seg = false;
					WORD ServiceID = SID_DEFAULT;

					if (m_ServiceSel.OneSegSelect == ONESEG_SELECT_HIGHPRIORITY) {
						// ワンセグ優先
						WORD SID;
						if (m_ServiceSel.PreferredServiceIndex != SERVICE_INVALID) {
							if (m_TsAnalyzer.Get1SegServiceIDByIndex(
									m_ServiceSel.PreferredServiceIndex, &SID)) {
								b1Seg = true;
								if (m_TsAnalyzer.IsServiceUpdated(m_TsAnalyzer.GetServiceIndexByID(SID))) {
									ServiceID = SID;
								}
							}
						}
						if (ServiceID == SID_DEFAULT
								&& m_TsAnalyzer.GetFirst1SegServiceID(&SID)) {
							b1Seg = true;
							if (m_TsAnalyzer.IsServiceUpdated(m_TsAnalyzer.GetServiceIndexByID(SID))) {
								ServiceID = SID;
							} else {
								bSetService = false;
							}
						}
					}

					if (!b1Seg && m_ServiceSel.ServiceID != SID_INVALID) {
						const int ServiceIndex = m_TsAnalyzer.GetServiceIndexByID(m_ServiceSel.ServiceID);
						if (ServiceIndex < 0) {
							TRACE(TEXT("Specified service ID %04x not found in PAT\n"), m_ServiceSel.ServiceID);
							if ((m_CurServiceID == SID_INVALID && !m_ServiceSel.bFollowViewableService)
									|| m_TsAnalyzer.GetViewableServiceNum() == 0) {
								bSetService = false;
							}
						} else {
							if (m_TsAnalyzer.GetViewableServiceIndexByID(m_ServiceSel.ServiceID) >= 0) {
								ServiceID = m_ServiceSel.ServiceID;
							} else if ((m_CurServiceID == SID_INVALID && !m_ServiceSel.bFollowViewableService)
									|| !m_TsAnalyzer.IsServiceUpdated(ServiceIndex)) {
								// サービスはPATにあるが、まだPMTが来ていない
								bSetService = false;
							}
						}
					}

					if (bSetService && ServiceID == SID_DEFAULT && m_CurServiceID != SID_INVALID) {
						const int ServiceIndex = m_TsAnalyzer.GetServiceIndexByID(m_CurServiceID);
						if (ServiceIndex < 0) {
							// サービスがPATにない
							TRACE(TEXT("Current service ID %04x not found in PAT\n"), m_CurServiceID);
							if (m_ServiceSel.bFollowViewableService
									&& m_TsAnalyzer.GetViewableServiceNum() > 0) {
								m_CurServiceID = SID_INVALID;
							} else {
								// まだ視聴可能なサービスのPMTが一つも来ていない場合は保留
								bSetService = false;
							}
						} else {
							if (m_TsAnalyzer.GetViewableServiceIndexByID(m_CurServiceID) >= 0) {
								ServiceID = m_CurServiceID;
							} else if (!m_ServiceSel.bFollowViewableService
									|| !m_TsAnalyzer.IsServiceUpdated(ServiceIndex)) {
								bSetService = false;
							}
						}
					}

					if (bSetService)
						SelectService(ServiceID, m_ServiceSel.OneSegSelect == ONESEG_SELECT_REFUSE);
				}

				if (m_pEventHandler)
					m_pEventHandler->OnServiceListUpdated(&m_TsAnalyzer, bStreamChanged);
			}
			return 0UL;

		case CTsAnalyzer::EVENT_SDT_UPDATED:
			// サービスの情報が更新された
			if (m_pEventHandler)
				m_pEventHandler->OnServiceInfoUpdated(&m_TsAnalyzer);
			return 0UL;

		case CTsAnalyzer::EVENT_EIT_UPDATED:
			//  EITが更新された
			{
				const WORD EventID = GetEventID();

				if (m_CurEventID != EventID) {
					m_CurEventID = EventID;

					if (m_pEventHandler)
						m_pEventHandler->OnEventChanged(&m_TsAnalyzer, EventID);
				}

				if (m_pEventHandler)
					m_pEventHandler->OnEventUpdated(&m_TsAnalyzer);
			}
			return 0UL;

		case CTsAnalyzer::EVENT_TOT_UPDATED:
			//  TOTが更新された
			if (m_pEventHandler)
				m_pEventHandler->OnTotUpdated(&m_TsAnalyzer);
			return 0UL;
		}
	} else if (pDecoder == &m_TsRecorder) {
		switch (dwEventID) {
		case CTsRecorder::EVENT_WRITE_ERROR:
			// 書き込みエラーが発生した
			if (m_pEventHandler)
				m_pEventHandler->OnFileWriteError(&m_TsRecorder);
			return 0UL;
		}
	} else if (pDecoder == &m_MediaViewer) {
		switch (dwEventID) {
		case CMediaViewer::EID_VIDEO_SIZE_CHANGED:
			if (m_pEventHandler)
				m_pEventHandler->OnVideoSizeChanged(&m_MediaViewer);
			return 0UL;

		case CMediaViewer::EID_FILTER_GRAPH_INITIALIZE:
			if (m_pEventHandler) {
				m_pEventHandler->OnFilterGraphInitialize(
					&m_MediaViewer, static_cast<IGraphBuilder*>(pParam));
			}
			return 0;

		case CMediaViewer::EID_FILTER_GRAPH_INITIALIZED:
			if (m_pEventHandler) {
				m_pEventHandler->OnFilterGraphInitialized(
					&m_MediaViewer, static_cast<IGraphBuilder*>(pParam));
			}
			return 0;

		case CMediaViewer::EID_FILTER_GRAPH_FINALIZE:
			if (m_pEventHandler) {
				m_pEventHandler->OnFilterGraphFinalize(
					&m_MediaViewer, static_cast<IGraphBuilder*>(pParam));
			}
			return 0;

		case CMediaViewer::EID_FILTER_GRAPH_FINALIZED:
			if (m_pEventHandler) {
				m_pEventHandler->OnFilterGraphFinalized(
					&m_MediaViewer, static_cast<IGraphBuilder*>(pParam));
			}
			return 0;

		case CMediaViewer::EID_SPDIF_PASSTHROUGH_ERROR:
			if (m_pEventHandler)
				m_pEventHandler->OnSpdifPassthroughError(*static_cast<HRESULT*>(pParam));
			return 0;
		}
	}

	return 0UL;
}


bool CDtvEngine::BuildMediaViewer(HWND hwndHost, HWND hwndMessage,
	CVideoRenderer::RendererType VideoRenderer,
	BYTE VideoStreamType, LPCWSTR pszVideoDecoder,
	LPCWSTR pszAudioDevice)
{
	DisconnectDecoder(DECODER_ID_MediaViewer);
	bool bOK = m_MediaViewer.OpenViewer(
		hwndHost, hwndMessage, VideoRenderer,
		VideoStreamType, pszVideoDecoder,
		pszAudioDevice);
	if (!bOK)
		SetError(m_MediaViewer.GetLastErrorException());
	ConnectDecoder(DECODER_ID_MediaViewer);

	return true;
}


bool CDtvEngine::RebuildMediaViewer(HWND hwndHost, HWND hwndMessage,
	CVideoRenderer::RendererType VideoRenderer,
	BYTE VideoStreamType, LPCWSTR pszVideoDecoder,
	LPCWSTR pszAudioDevice)
{
	bool bOK;

	EnableMediaViewer(false);
	DisconnectDecoder(DECODER_ID_MediaViewer);
	m_MediaViewer.CloseViewer();
	bOK=m_MediaViewer.OpenViewer(hwndHost, hwndMessage, VideoRenderer,
								 VideoStreamType, pszVideoDecoder,
								 pszAudioDevice);
	if (!bOK) {
		SetError(m_MediaViewer.GetLastErrorException());
	}
	ConnectDecoder(DECODER_ID_MediaViewer);

	return bOK;
}


bool CDtvEngine::CloseMediaViewer()
{
	m_MediaViewer.CloseViewer();
	return true;
}


bool CDtvEngine::ResetMediaViewer()
{
	if (!m_MediaViewer.IsOpen())
		return false;

	m_MediaViewer.Reset();

	CBlockLock Lock(&m_EngineLock);

	WORD VideoPID = CMediaViewer::PID_INVALID;
	WORD AudioPID = CMediaViewer::PID_INVALID;
	if (m_TsAnalyzer.GetVideoEsPID(m_CurServiceIndex, m_CurVideoStream, &VideoPID))
		m_MediaViewer.SetVideoPID(VideoPID);
	if (m_TsAnalyzer.GetAudioEsPID(m_CurServiceIndex, m_CurAudioStream, &AudioPID))
		m_MediaViewer.SetAudioPID(AudioPID);

	return true;
}


bool CDtvEngine::EnableMediaViewer(const bool bEnable)
{
	if (!m_MediaViewer.IsOpen())
		return false;

	bool bOK;

	if (bEnable) {
		// ビューア有効
		bOK = m_MediaViewer.Play();
	} else {
		// ビューア無効
		bOK = m_MediaViewer.Stop();
	}

	return bOK;
}


BYTE CDtvEngine::GetVideoStreamType() const
{
	return m_VideoStreamType;
}


int CDtvEngine::GetVideoStreamNum(const int Service) const
{
	WORD ServiceID;
	if (Service<0) {
		if (!GetServiceID(&ServiceID))
			return 0;
	} else {
		if (!m_TsAnalyzer.GetViewableServiceID(Service, &ServiceID))
			return 0;
	}
	return m_TsAnalyzer.GetVideoEsNum(m_TsAnalyzer.GetServiceIndexByID(ServiceID));
}


bool CDtvEngine::SetVideoStream(const int StreamIndex)
{
	CBlockLock Lock(&m_EngineLock);

	if (StreamIndex < 0 || StreamIndex >= GetVideoStreamNum())
		return false;

	WORD VideoPID = CMediaViewer::PID_INVALID;
	BYTE VideoStreamType = STREAM_TYPE_INVALID;

	m_TsAnalyzer.GetVideoEsPID(m_CurServiceIndex, StreamIndex, &VideoPID);
	m_TsAnalyzer.GetVideoStreamType(m_CurServiceIndex, StreamIndex, &VideoStreamType);

	m_CurVideoStream = StreamIndex;
	m_CurVideoComponentTag = m_TsAnalyzer.GetVideoComponentTag(m_CurServiceIndex, StreamIndex);

	TRACE(TEXT("Select video : %d (component_tag %02X)\n"), m_CurVideoStream, m_CurVideoComponentTag);

	if (m_VideoStreamType != VideoStreamType) {
		TRACE(TEXT("Video stream_type changed (%02x -> %02x)\n"),
			  m_VideoStreamType, VideoStreamType);
		m_VideoStreamType = VideoStreamType;
		if (m_pEventHandler!=NULL)
			m_pEventHandler->OnVideoStreamTypeChanged(VideoStreamType);
	}

	m_MediaViewer.SetVideoPID(VideoPID);

	return true;
}


int CDtvEngine::GetVideoStream() const
{
	return m_CurVideoStream;
}


BYTE CDtvEngine::GetVideoComponentTag() const
{
	return m_CurVideoComponentTag;
}


BYTE CDtvEngine::GetAudioChannelNum()
{
	return m_MediaViewer.GetAudioChannelNum();
}


int CDtvEngine::GetAudioStreamNum(const int Service) const
{
	WORD ServiceID;
	if (Service<0) {
		if (!GetServiceID(&ServiceID))
			return 0;
	} else {
		if (!m_TsAnalyzer.GetViewableServiceID(Service, &ServiceID))
			return 0;
	}
	return m_TsAnalyzer.GetAudioEsNum(m_TsAnalyzer.GetServiceIndexByID(ServiceID));
}


bool CDtvEngine::SetAudioStream(const int StreamIndex)
{
	CBlockLock Lock(&m_EngineLock);

	WORD AudioPID;

	if (!m_TsAnalyzer.GetAudioEsPID(m_CurServiceIndex, StreamIndex, &AudioPID))
		return false;

	if (!m_MediaViewer.SetAudioPID(AudioPID, true))
		return false;

	m_CurAudioStream = StreamIndex;
	m_CurAudioComponentTag = m_TsAnalyzer.GetAudioComponentTag(m_CurServiceIndex, StreamIndex);

	TRACE(TEXT("Select audio : %d (component_tag %02X)\n"), m_CurAudioStream, m_CurAudioComponentTag);

	return true;
}


int CDtvEngine::GetAudioStream() const
{
	return m_CurAudioStream;
}


BYTE CDtvEngine::GetAudioComponentTag(const int AudioIndex) const
{
	if (AudioIndex < 0)
		return m_CurAudioComponentTag;

	return m_TsAnalyzer.GetAudioComponentTag(m_CurServiceIndex, AudioIndex);
}


BYTE CDtvEngine::GetAudioComponentType(const int AudioIndex) const
{
	return m_TsAnalyzer.GetAudioComponentType(m_CurServiceIndex, AudioIndex < 0 ? m_CurAudioStream : AudioIndex);
}


int CDtvEngine::GetAudioComponentText(LPTSTR pszText, int MaxLength, const int AudioIndex) const
{
	return m_TsAnalyzer.GetAudioComponentText(m_CurServiceIndex, AudioIndex < 0 ? m_CurAudioStream : AudioIndex, pszText, MaxLength);
}


bool CDtvEngine::SetWriteStream(WORD ServiceID, DWORD Stream)
{
	return m_TsSelector.SetTargetServiceID(ServiceID, Stream);
}


bool CDtvEngine::GetWriteStream(WORD *pServiceID, DWORD *pStream)
{
	if (pServiceID)
		*pServiceID = m_TsSelector.GetTargetServiceID();
	if (pStream)
		*pStream = m_TsSelector.GetTargetStream();
	return true;
}


bool CDtvEngine::SetWriteCurServiceOnly(bool bOnly, DWORD Stream)
{
	if (m_bWriteCurServiceOnly != bOnly || m_WriteStream != Stream) {
		m_bWriteCurServiceOnly = bOnly;
		m_WriteStream = Stream;
		if (bOnly) {
			WORD ServiceID = 0;

			GetServiceID(&ServiceID);
			SetWriteStream(ServiceID, Stream);
		} else {
			SetWriteStream(0, Stream);
		}
	}
	return true;
}


void CDtvEngine::SetStartStreamingOnDriverOpen(bool bStart)
{
	m_bStartStreamingOnDriverOpen = bStart;
}


void CDtvEngine::SetTracer(CTracer *pTracer)
{
	CBonBaseClass::SetTracer(pTracer);
	m_MediaViewer.SetTracer(pTracer);
}


void CDtvEngine::ResetStatus()
{
	m_wCurTransportStream = 0;
	m_CurServiceIndex = SERVICE_INVALID;
	m_CurServiceID = SID_INVALID;
}


CMediaDecoder *CDtvEngine::GetDecoderByID(int ID)
{
	switch (ID) {
	case DECODER_ID_BonSrcDecoder:		return &m_BonSrcDecoder;
	case DECODER_ID_TsPacketParser:		return &m_TsPacketParser;
	case DECODER_ID_TsAnalyzer:			return &m_TsAnalyzer;
	case DECODER_ID_MediaViewer:		return &m_MediaViewer;
	case DECODER_ID_MediaTee:			return &m_MediaTee;
	case DECODER_ID_TsRecorder:			return &m_TsRecorder;
	case DECODER_ID_MediaGrabber:		return &m_MediaGrabber;
	case DECODER_ID_TsSelector:			return &m_TsSelector;
	case DECODER_ID_EventManager:		return &m_EventManager;
	case DECODER_ID_CaptionDecoder:		return &m_CaptionDecoder;
	case DECODER_ID_LogoDownloader:		return &m_LogoDownloader;
	case DECODER_ID_TsPacketCounter:	return &m_TsPacketCounter;
	}

	for (auto itr = m_DecoderList.begin(); itr != m_DecoderList.end(); ++itr) {
		if (itr->ID == ID)
			return itr->pDecoder;
	}

	return NULL;
}


const CDtvEngine::DecoderConnectionInfo *CDtvEngine::GetInputConnectionInfo(int ID) const
{
	for (auto itr = m_DecoderConnectionList.begin(); itr != m_DecoderConnectionList.end(); ++itr) {
		if (itr->InputDecoder == ID)
			return &*itr;
	}

	return NULL;
}


const CDtvEngine::DecoderConnectionInfo *CDtvEngine::GetOutputConnectionInfo(int ID) const
{
	for (auto itr = m_DecoderConnectionList.begin(); itr != m_DecoderConnectionList.end(); ++itr) {
		if (itr->OutputDecoder == ID)
			return &*itr;
	}

	return NULL;
}


void CDtvEngine::ConnectDecoder(int ID)
{
	const DecoderConnectionInfo *pInputConnection = GetInputConnectionInfo(ID);

	if (pInputConnection != NULL) {
		CMediaDecoder *pInputDecoder = GetDecoderByID(pInputConnection->OutputDecoder);

		if (pInputDecoder != NULL)
			pInputDecoder->SetOutputDecoder(GetDecoderByID(ID), pInputConnection->OutputIndex);
	}
}


void CDtvEngine::DisconnectDecoder(int ID)
{
	// MediaTee は非対応
	const DecoderConnectionInfo *pInputConnection = GetInputConnectionInfo(ID);

	if (pInputConnection != NULL) {
		CMediaDecoder *pInputDecoder = GetDecoderByID(pInputConnection->OutputDecoder);

		if (pInputDecoder != NULL) {
			const DecoderConnectionInfo *pOutputConnection = GetOutputConnectionInfo(ID);

			pInputDecoder->SetOutputDecoder(
				pOutputConnection != NULL ? GetDecoderByID(pOutputConnection->InputDecoder) : NULL,
				pInputConnection->OutputIndex);
		}
	}
}
