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


//////////////////////////////////////////////////////////////////////
// CDtvEngine 構築/消滅
//////////////////////////////////////////////////////////////////////

CDtvEngine::CDtvEngine(void)
	: m_pEventHandler(NULL)

	, m_wCurTransportStream(0U)
	, m_CurServiceIndex(SERVICE_INVALID)
	, m_CurServiceID(SID_INVALID)
	, m_SpecServiceID(SID_INVALID)
	, m_bFollowViewableService(false)
	, m_CurAudioStream(0)

	, m_BonSrcDecoder(this)
	, m_TsPacketParser(this)
	, m_TsAnalyzer(this)
	, m_CasProcessor(this)
	, m_MediaViewer(this)
	, m_MediaTee(this)
	, m_FileWriter(this)
	//, m_FileReader(this)
	, m_MediaBuffer(this)
	, m_MediaGrabber(this)
	, m_TsSelector(this)
	, m_EventManager(this)
	, m_CaptionDecoder(this)
	, m_LogoDownloader(this)

	, m_bBuiled(false)
	, m_bDescramble(true)
	, m_bBuffering(false)
	, m_bStartStreamingOnDriverOpen(false)

	, m_bDescrambleCurServiceOnly(false)
	, m_bWriteCurServiceOnly(false)
	, m_WriteStream(CTsSelector::STREAM_ALL)
{
}


CDtvEngine::~CDtvEngine(void)
{
	CloseEngine();
}


const bool CDtvEngine::BuildEngine(CEventHandler *pEventHandler,
								   bool bDescramble, bool bBuffering, bool bEventManager)
{
	// 完全に暫定
	if (m_bBuiled)
		return true;

	/*
	グラフ構成図

	CBonSrcDecoder
	    ↓
	CTsPacketParser
	    ↓
	CTsAnalyzer
	    ↓
	CMediaTee──────┐
	    ↓               ↓
	CEventManager    CCasProcessor
	    ↓               ↓
	CLogoDownloader  CCaptionDecoder
	    ↓               ↓
	CTsSelector      CMediaGrabber
	    ↓               ↓
	CFileWriter      CMediaBuffer
	                     ↓
	                 CMediaViewer
	*/

	Trace(TEXT("デコーダグラフを構築しています..."));

	// デコーダグラフ構築
	m_TsPacketParser.SetOutputDecoder(&m_TsAnalyzer);
	m_TsAnalyzer.SetOutputDecoder(&m_MediaTee);
	if (bEventManager) {
		m_MediaTee.SetOutputDecoder(&m_EventManager, 0);
		m_EventManager.SetOutputDecoder(&m_LogoDownloader);
	} else {
		m_MediaTee.SetOutputDecoder(&m_LogoDownloader, 0);
	}
	m_MediaTee.SetOutputDecoder(&m_CasProcessor, 1);
	m_LogoDownloader.SetOutputDecoder(&m_TsSelector);
	m_TsSelector.SetOutputDecoder(&m_FileWriter);
	m_CasProcessor.SetOutputDecoder(&m_CaptionDecoder);
	m_CasProcessor.EnableDescramble(bDescramble);
	m_bDescramble = bDescramble;
	m_CaptionDecoder.SetOutputDecoder(&m_MediaGrabber);
	if (bBuffering) {
		m_MediaGrabber.SetOutputDecoder(&m_MediaBuffer);
		m_MediaBuffer.SetOutputDecoder(&m_MediaViewer);
		m_MediaBuffer.Play();
	} else {
		m_MediaGrabber.SetOutputDecoder(&m_MediaViewer);
	}
	m_bBuffering=bBuffering;

	// イベントハンドラ設定
	m_pEventHandler = pEventHandler;
	m_pEventHandler->m_pDtvEngine = this;

	m_bBuiled = true;

	return true;
}


const bool CDtvEngine::IsBuildComplete() const
{
	return m_bBuiled && IsSrcFilterOpen() && m_MediaViewer.IsOpen()
		&& (!m_bDescramble || m_CasProcessor.IsCasCardOpen());
}


const bool CDtvEngine::CloseEngine(void)
{
	//if (!m_bBuiled)
	//	return true;

	Trace(TEXT("DtvEngineを閉じています..."));

	//m_MediaViewer.Stop();

	ReleaseSrcFilter();

	Trace(TEXT("バッファのストリーミングを停止しています..."));
	m_MediaBuffer.Stop();

	Trace(TEXT("カードリーダを閉じています..."));
	m_CasProcessor.CloseCasCard();

	Trace(TEXT("メディアビューアを閉じています..."));
	m_MediaViewer.CloseViewer();

	// イベントハンドラ解除
	m_pEventHandler = NULL;

	m_bBuiled = false;

	Trace(TEXT("DtvEngineを閉じました。"));

	return true;
}


const bool CDtvEngine::ResetEngine(void)
{
	if (!m_bBuiled)
		return false;

	// デコーダグラフリセット
	ResetStatus();
	m_BonSrcDecoder.ResetGraph();

	return true;
}


const bool CDtvEngine::OpenBonDriver(LPCTSTR pszFileName)
{
	ReleaseSrcFilter();

	// ソースフィルタを開く
	Trace(TEXT("BonDriverを読み込んでいます..."));
	if (!m_BonSrcDecoder.LoadBonDriver(pszFileName)) {
		SetError(m_BonSrcDecoder.GetLastErrorException());
		return false;
	}
	Trace(TEXT("チューナを開いています..."));
	if (!m_BonSrcDecoder.OpenTuner()) {
		m_BonSrcDecoder.UnloadBonDriver();
		SetError(m_BonSrcDecoder.GetLastErrorException());
		return false;
	}
	m_MediaBuffer.SetFileMode(false);
	m_BonSrcDecoder.SetOutputDecoder(&m_TsPacketParser);
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


const bool CDtvEngine::ReleaseSrcFilter()
{
	// ソースフィルタを開放する
	if (m_BonSrcDecoder.IsBonDriverLoaded()) {
		Trace(TEXT("BonDriver を解放しています..."));
		m_BonSrcDecoder.UnloadBonDriver();
		m_BonSrcDecoder.SetOutputDecoder(NULL);
	}

	return true;
}


const bool CDtvEngine::IsSrcFilterOpen() const
{
	return m_BonSrcDecoder.IsOpen();
}


const bool CDtvEngine::EnablePreview(const bool bEnable)
{
	if (!m_MediaViewer.IsOpen())
		return false;

	bool bOK;

	if (bEnable) {
		// プレビュー有効
		bOK = m_MediaViewer.Play();
	} else {
		// プレビュー無効
		bOK = m_MediaViewer.Stop();
	}

	return bOK;
}


const bool CDtvEngine::SetViewSize(const int x,const int y)
{
	// ウィンドウサイズを設定する
	return m_MediaViewer.SetViewSize(x,y);
}


const bool CDtvEngine::SetVolume(const float fVolume)
{
	// オーディオボリュームを設定する( -100.0(無音) < fVolume < 0(最大) )
	return m_MediaViewer.SetVolume(fVolume);
}


const bool CDtvEngine::GetVideoSize(WORD *pwWidth,WORD *pwHeight)
{
	return m_MediaViewer.GetVideoSize(pwWidth,pwHeight);
}


const bool CDtvEngine::GetVideoAspectRatio(BYTE *pbyAspectRateX,BYTE *pbyAspectRateY)
{
	return m_MediaViewer.GetVideoAspectRatio(pbyAspectRateX,pbyAspectRateY);
}


const BYTE CDtvEngine::GetAudioChannelNum()
{
	return m_MediaViewer.GetAudioChannelNum();
}


const int CDtvEngine::GetAudioStreamNum(const int Service)
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


const bool CDtvEngine::SetAudioStream(int StreamIndex)
{
	WORD AudioPID;

	if (!m_TsAnalyzer.GetAudioEsPID(m_CurServiceIndex, StreamIndex, &AudioPID))
		return false;

	if (!m_MediaViewer.SetAudioPID(AudioPID))
		return false;

	m_CurAudioStream = StreamIndex;

	return true;
}


const int CDtvEngine::GetAudioStream() const
{
	return m_CurAudioStream;
}


const BYTE CDtvEngine::GetAudioComponentType(const int AudioIndex)
{
	return m_TsAnalyzer.GetAudioComponentType(m_CurServiceIndex, AudioIndex < 0 ? m_CurAudioStream : AudioIndex);
}


const int CDtvEngine::GetAudioComponentText(LPTSTR pszText, int MaxLength, const int AudioIndex)
{
	return m_TsAnalyzer.GetAudioComponentText(m_CurServiceIndex, AudioIndex < 0 ? m_CurAudioStream : AudioIndex, pszText, MaxLength);
}


const bool CDtvEngine::SetStereoMode(int iMode)
{
	return m_MediaViewer.SetStereoMode(iMode);
}


const WORD CDtvEngine::GetEventID(bool bNext)
{
	return m_TsAnalyzer.GetEventID(m_CurServiceIndex, bNext);
}


const int CDtvEngine::GetEventName(LPTSTR pszName, int MaxLength, bool bNext)
{
	return m_TsAnalyzer.GetEventName(m_CurServiceIndex, pszName, MaxLength, bNext);
}


const int CDtvEngine::GetEventText(LPTSTR pszText, int MaxLength, bool bNext)
{
	return m_TsAnalyzer.GetEventText(m_CurServiceIndex, pszText, MaxLength, bNext);
}


const int CDtvEngine::GetEventExtendedText(LPTSTR pszText, int MaxLength, bool bNext)
{
	return m_TsAnalyzer.GetEventExtendedText(m_CurServiceIndex, pszText, MaxLength, true, bNext);
}


const bool CDtvEngine::GetEventTime(SYSTEMTIME *pStartTime, DWORD *pDuration, bool bNext)
{
	return m_TsAnalyzer.GetEventTime(m_CurServiceIndex, pStartTime, pDuration, bNext);
}


const bool CDtvEngine::GetEventSeriesInfo(CTsAnalyzer::EventSeriesInfo *pInfo, bool bNext)
{
	return m_TsAnalyzer.GetEventSeriesInfo(m_CurServiceIndex, pInfo, bNext);
}


const bool CDtvEngine::GetEventInfo(CTsAnalyzer::EventInfo *pInfo, bool bNext)
{
	return m_TsAnalyzer.GetEventInfo(m_CurServiceIndex, pInfo, true, bNext);
}


const bool CDtvEngine::GetEventAudioInfo(CTsAnalyzer::EventAudioInfo *pInfo, const int AudioIndex, bool bNext)
{
	return m_TsAnalyzer.GetEventAudioInfo(m_CurServiceIndex, AudioIndex < 0 ? m_CurAudioStream : AudioIndex, pInfo, bNext);
}

const bool CDtvEngine::GetVideoDecoderName(LPWSTR lpName,int iBufLen)
{
	return m_MediaViewer.GetVideoDecoderName(lpName, iBufLen);
}


const bool CDtvEngine::SetChannel(const BYTE byTuningSpace, const WORD wChannel, const WORD ServiceID, const bool bFollowViewableService)
{
	TRACE(TEXT("CDtvEngine::SetChannel(%d, %d, %04x)\n"),
		  byTuningSpace, wChannel, ServiceID);

	CBlockLock Lock(&m_EngineLock);

	// サービスはPATが来るまで保留
	const WORD PrevSpecServiceID = m_SpecServiceID;
	m_SpecServiceID = ServiceID;

	const bool bPrevFollowViewableService = m_bFollowViewableService;
	m_bFollowViewableService = bFollowViewableService || ServiceID == SID_INVALID;

	// チャンネル変更
	if (!m_BonSrcDecoder.SetChannelAndPlay((DWORD)byTuningSpace, (DWORD)wChannel)) {
		m_SpecServiceID = PrevSpecServiceID;
		m_bFollowViewableService = bPrevFollowViewableService;

		SetError(m_BonSrcDecoder.GetLastErrorException());
		return false;
	}

	ResetStatus();

	return true;
}


const bool CDtvEngine::SetServiceByID(const WORD ServiceID, const bool bReserve)
{
	CBlockLock Lock(&m_EngineLock);

	// bReserve == true の場合、まだPATが来ていなくてもエラーにしない

	int Index = m_TsAnalyzer.GetViewableServiceIndexByID(ServiceID);
	if (Index < 0) {
		if (!bReserve || m_wCurTransportStream != 0)
			return false;
	} else {
		SelectService(Index);
	}

	m_SpecServiceID = ServiceID;

	return true;
}


const bool CDtvEngine::SetServiceByIndex(const WORD Service)
{
	CBlockLock Lock(&m_EngineLock);

	if (!SelectService(Service))
		return false;

	m_SpecServiceID = m_CurServiceID;

	return true;
}


bool CDtvEngine::SelectService(const WORD wService)
{
	CBlockLock Lock(&m_EngineLock);

	// サービス変更(wService==SERVICE_DEFAULTならPAT先頭サービス)

	if (wService == SERVICE_DEFAULT || wService < m_TsAnalyzer.GetViewableServiceNum()) {
		WORD wServiceID;

		if (wService == SERVICE_DEFAULT) {
			TRACE(TEXT("Select default service\n"));
			// 先頭PMTが到着するまで失敗にする
			if (!m_TsAnalyzer.GetFirstViewableServiceID(&wServiceID)) {
				TRACE(TEXT("No viewable service\n"));
				return false;
			}
		} else {
			if (!m_TsAnalyzer.GetViewableServiceID(wService, &wServiceID)) {
				TRACE(TEXT("Service #%d not found\n"), wService);
				return false;
			}
		}
		m_CurServiceIndex = m_TsAnalyzer.GetServiceIndexByID(wServiceID);
		m_CurServiceID = wServiceID;

		WORD wVideoPID = CMediaViewer::PID_INVALID;
		WORD wAudioPID = CMediaViewer::PID_INVALID;

		m_TsAnalyzer.GetVideoEsPID(m_CurServiceIndex, &wVideoPID);
		if (!m_TsAnalyzer.GetAudioEsPID(m_CurServiceIndex, m_CurAudioStream, &wAudioPID)
				&& m_CurAudioStream != 0) {
			m_TsAnalyzer.GetAudioEsPID(m_CurServiceIndex, 0, &wAudioPID);
			m_CurAudioStream = 0;
		}

		TRACE(TEXT("------- Service Select -------\n"));
		TRACE(TEXT("%d (ServiceID = %04X)\n"), m_CurServiceIndex, wServiceID);

		m_MediaViewer.SetVideoPID(wVideoPID);
		m_MediaViewer.SetAudioPID(wAudioPID);

		if (m_bDescrambleCurServiceOnly)
			SetDescrambleService(wServiceID);

		if (m_bWriteCurServiceOnly)
			SetWriteStream(wServiceID, m_WriteStream);

		m_CaptionDecoder.SetTargetStream(wServiceID);

		if (m_pEventHandler)
			m_pEventHandler->OnServiceChanged(wServiceID);

		return true;
	}

	return false;
}


const bool CDtvEngine::GetServiceID(WORD *pServiceID)
{
	// サービスID取得
	return m_TsAnalyzer.GetServiceID(m_CurServiceIndex, pServiceID);
}


const unsigned __int64 CDtvEngine::GetPcrTimeStamp()
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
	if (pDecoder == &m_TsAnalyzer) {
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

					m_CurServiceIndex = SERVICE_INVALID;
					m_CurServiceID = SID_INVALID;
					m_CurAudioStream = 0;
					m_wCurTransportStream = wTransportStream;

					bool bSetService = true;
					WORD Service = SERVICE_DEFAULT;

					if (m_SpecServiceID != SID_INVALID) {
						// サービスが指定されている
						const int ServiceIndex = m_TsAnalyzer.GetServiceIndexByID(m_SpecServiceID);
						if (ServiceIndex < 0) {
							// サービスがPATにない
							TRACE(TEXT("Specified service ID %04x not found in PAT\n"), m_SpecServiceID);
							bSetService = false;
						} else {
							const int ViewServiceIndex = m_TsAnalyzer.GetViewableServiceIndexByID(m_SpecServiceID);
							if (ViewServiceIndex >= 0) {
								Service = (WORD)ViewServiceIndex;
							} else {
								bSetService = false;
							}
						}
					}

					if (!bSetService || !SelectService(Service)) {
						m_MediaViewer.SetVideoPID(CMediaViewer::PID_INVALID);
						m_MediaViewer.SetAudioPID(CMediaViewer::PID_INVALID);
					}
				} else {
					// ストリームIDは同じだが、構成ESのPIDが変わった可能性がある
					TRACE(TEXT("CDtvEngine ■Stream Updated!! %04X\n"), wTransportStream);

					bool bSetService = true;
					WORD Service = SERVICE_DEFAULT;

					if (m_SpecServiceID != SID_INVALID) {
						const int ServiceIndex = m_TsAnalyzer.GetServiceIndexByID(m_SpecServiceID);
						if (ServiceIndex < 0) {
							TRACE(TEXT("Specified service ID %04x not found in PAT\n"), m_SpecServiceID);
							if ((m_CurServiceID == SID_INVALID && !m_bFollowViewableService)
									|| m_TsAnalyzer.GetViewableServiceNum() == 0) {
								bSetService = false;
							}
						} else {
							const int ViewServiceIndex = m_TsAnalyzer.GetViewableServiceIndexByID(m_SpecServiceID);
							if (ViewServiceIndex >= 0) {
								Service = (WORD)ViewServiceIndex;
							} else if ((m_CurServiceID == SID_INVALID && !m_bFollowViewableService)
									|| !m_TsAnalyzer.IsServiceUpdated(ServiceIndex)) {
								// サービスはPATにあるが、まだPMTが来ていない
								bSetService = false;
							}
						}
					}

					if (bSetService && Service == SERVICE_DEFAULT && m_CurServiceID != SID_INVALID) {
						const int ServiceIndex = m_TsAnalyzer.GetServiceIndexByID(m_CurServiceID);
						if (ServiceIndex < 0) {
							// サービスがPATにない
							TRACE(TEXT("Current service ID %04x not found in PAT\n"), m_CurServiceID);
							if (m_bFollowViewableService
									&& m_TsAnalyzer.GetViewableServiceNum() > 0) {
								m_CurServiceID = SID_INVALID;
							} else {
								// まだ視聴可能なサービスのPMTが一つも来ていない場合は保留
								bSetService = false;
							}
						} else {
							const int ViewServiceIndex = m_TsAnalyzer.GetViewableServiceIndexByID(m_CurServiceID);
							if (ViewServiceIndex >= 0) {
								Service = (WORD)ViewServiceIndex;
							} else if (!m_bFollowViewableService
									|| !m_TsAnalyzer.IsServiceUpdated(ServiceIndex)) {
								bSetService = false;
							}
						}
					}

					if (bSetService)
						SelectService(Service);
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
		}
	} else if (pDecoder == &m_FileWriter) {
		switch (dwEventID) {
		case CBufferedFileWriter::EID_WRITE_ERROR:
			// 書き込みエラーが発生した
			if (m_pEventHandler)
				m_pEventHandler->OnFileWriteError(&m_FileWriter);
			return 0UL;
		}
	} else if (pDecoder == &m_MediaViewer) {
		switch (dwEventID) {
		case CMediaViewer::EID_VIDEO_SIZE_CHANGED:
			if (m_pEventHandler)
				m_pEventHandler->OnVideoSizeChanged(&m_MediaViewer);
			return 0UL;
		}
	} else if (pDecoder == &m_CasProcessor) {
		switch (dwEventID) {
		case CCasProcessor::EVENT_EMM_PROCESSED:
			// EMM処理が行われた
			if (m_pEventHandler)
				m_pEventHandler->OnEmmProcessed();
			return 0UL;

		case CCasProcessor::EVENT_EMM_ERROR:
			// EMM処理でエラーが発生した
			if (m_pEventHandler) {
				CCasProcessor::EmmErrorInfo *pInfo = static_cast<CCasProcessor::EmmErrorInfo*>(pParam);

				m_pEventHandler->OnEmmError(pInfo->pszText);
			}
			return 0UL;

		case CCasProcessor::EVENT_ECM_ERROR:
			// ECM処理でエラーが発生した
			if (m_pEventHandler) {
				CCasProcessor::EcmErrorInfo *pInfo = static_cast<CCasProcessor::EcmErrorInfo*>(pParam);

				if (m_CasProcessor.GetEcmPIDByServiceID(m_CurServiceID) == pInfo->EcmPID)
					m_pEventHandler->OnEcmError(pInfo->pszText);
			}
			return 0UL;

		case CCasProcessor::EVENT_ECM_REFUSED:
			// ECMが受け付けられない
			if (m_pEventHandler) {
				CCasProcessor::EcmErrorInfo *pInfo = static_cast<CCasProcessor::EcmErrorInfo*>(pParam);

				if (m_CasProcessor.GetEcmPIDByServiceID(m_CurServiceID) == pInfo->EcmPID)
					m_pEventHandler->OnEcmRefused();
			}
			return 0UL;

		case CCasProcessor::EVENT_CARD_READER_HUNG:
			// カードリーダーから応答が無い
			if (m_pEventHandler)
				m_pEventHandler->OnCardReaderHung();
			return 0UL;
		}
	}

	return 0UL;
}


bool CDtvEngine::BuildMediaViewer(HWND hwndHost,HWND hwndMessage,
	CVideoRenderer::RendererType VideoRenderer,LPCWSTR pszMpeg2Decoder,LPCWSTR pszAudioDevice)
{
	if (!m_MediaViewer.OpenViewer(hwndHost,hwndMessage,VideoRenderer,
								  pszMpeg2Decoder,pszAudioDevice)) {
		SetError(m_MediaViewer.GetLastErrorException());
		return false;
	}
	return true;
}


bool CDtvEngine::RebuildMediaViewer(HWND hwndHost,HWND hwndMessage,
	CVideoRenderer::RendererType VideoRenderer,LPCWSTR pszMpeg2Decoder,LPCWSTR pszAudioDevice)
{
	bool bOK;

	EnablePreview(false);
	m_MediaBuffer.SetOutputDecoder(NULL);
	m_MediaViewer.CloseViewer();
	bOK=m_MediaViewer.OpenViewer(hwndHost,hwndMessage,VideoRenderer,
								 pszMpeg2Decoder,pszAudioDevice);
	if (!bOK) {
		SetError(m_MediaViewer.GetLastErrorException());
	}
	if (m_bBuffering)
		m_MediaBuffer.SetOutputDecoder(&m_MediaViewer);

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

	if (m_bBuffering)
		m_MediaBuffer.Reset();

	CBlockLock Lock(&m_EngineLock);

	WORD VideoPID = CMediaViewer::PID_INVALID;
	WORD AudioPID = CMediaViewer::PID_INVALID;
	if (m_TsAnalyzer.GetVideoEsPID(m_CurServiceIndex, &VideoPID))
		m_MediaViewer.SetVideoPID(VideoPID);
	if (m_TsAnalyzer.GetAudioEsPID(m_CurServiceIndex, m_CurAudioStream, &AudioPID))
		m_MediaViewer.SetAudioPID(AudioPID);

	return true;
}


bool CDtvEngine::OpenCasCard(int Device, LPCTSTR pszReaderName)
{
	// CASカードを開く
	if (Device>=0) {
		Trace(TEXT("CASカードを開いています..."));
		if (!m_CasProcessor.OpenCasCard(Device,pszReaderName)) {
			TCHAR szText[256];

			if (m_CasProcessor.GetLastErrorText()!=NULL)
				::wsprintf(szText,TEXT("CASカードの初期化に失敗しました。%s"),m_CasProcessor.GetLastErrorText());
			else
				::lstrcpy(szText,TEXT("CASカードの初期化に失敗しました。"));
			SetError(0,szText,
					 TEXT("カードリーダが接続されているか、設定で有効なカードリーダが選択されているか確認してください。"),
					 m_CasProcessor.GetLastErrorSystemMessage());
			return false;
		}
	} else if (m_CasProcessor.IsCasCardOpen()) {
		m_CasProcessor.CloseCasCard();
	}
	return true;
}


bool CDtvEngine::CloseCasCard()
{
	if (m_CasProcessor.IsCasCardOpen())
		m_CasProcessor.CloseCasCard();
	return true;
}


bool CDtvEngine::SetDescramble(bool bDescramble)
{
	if (!m_bBuiled) {
		SetError(0,TEXT("内部エラー : DtvEngineが構築されていません。"));
		return false;
	}

	if (m_bDescramble != bDescramble) {
		m_CasProcessor.EnableDescramble(bDescramble);
		m_bDescramble = bDescramble;
	}
	return true;
}


bool CDtvEngine::ResetBuffer()
{
	m_MediaBuffer.ResetBuffer();
	return true;
}


bool CDtvEngine::GetOriginalVideoSize(WORD *pWidth,WORD *pHeight)
{
	return m_MediaViewer.GetOriginalVideoSize(pWidth,pHeight);
}


bool CDtvEngine::SetDescrambleService(WORD ServiceID)
{
	return m_CasProcessor.SetTargetServiceID(ServiceID);
}


bool CDtvEngine::SetDescrambleCurServiceOnly(bool bOnly)
{
	if (m_bDescrambleCurServiceOnly != bOnly) {
		WORD ServiceID = 0;

		m_bDescrambleCurServiceOnly = bOnly;
		if (bOnly)
			GetServiceID(&ServiceID);
		SetDescrambleService(ServiceID);
	}
	return true;
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
