// BonSrcDecoder.cpp: CBonSrcDecoder クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "BonSrcDecoder.h"
#include "StdUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


typedef IBonDriver* (PFCREATEBONDRIVER)(void);


//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CBonSrcDecoder::CBonSrcDecoder(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 0UL, 1UL)
	, m_hBonDriverLib(NULL)
	, m_pBonDriver(NULL)
	, m_pBonDriver2(NULL)
	, m_hStreamRecvThread(NULL)
	, m_RequestTimeout(10000)
	, m_bIsPlaying(false)
	, m_StreamRemain(0)
	, m_SignalLevel(0.0f)
	, m_StreamThreadPriority(THREAD_PRIORITY_NORMAL)
	, m_bPurgeStreamOnChannelChange(true)
	, m_FirstChannelSetDelay(0)
	, m_MinChannelChangeInterval(0)
	, m_SetChannelCount(0)
{
}

CBonSrcDecoder::~CBonSrcDecoder()
{
	UnloadBonDriver();
}

void CBonSrcDecoder::Reset(void)
{
	if (m_pBonDriver == NULL)
		return;

	if (HasPendingRequest()) {
		Trace(TEXT("前回の要求が完了しないため新しい要求を行えません。"));
		return;
	}

	// 未処理のストリームを破棄する
	StreamingRequest Request;
	Request.Type = StreamingRequest::TYPE_PURGESTREAM;
	AddRequest(Request);

	if (!WaitAllRequests(m_RequestTimeout)) {
		Trace(TEXT("ストリーム受信スレッドが応答しません。"));
	}
}

void CBonSrcDecoder::ResetGraph(void)
{
	CBlockLock Lock(&m_DecoderLock);

	if (m_pBonDriver == NULL) {
		ResetDownstreamDecoder();
	} else {
		if (HasPendingRequest()) {
			Trace(TEXT("前回の要求が完了しないため新しい要求を行えません。"));
			return;
		}

		// 未処理のストリームを破棄する
		StreamingRequest Request[2];
		Request[0].Type = StreamingRequest::TYPE_PURGESTREAM;
		Request[1].Type = StreamingRequest::TYPE_RESET;
		AddRequests(Request, 2);

		if (!WaitAllRequests(m_RequestTimeout)) {
			Trace(TEXT("ストリーム受信スレッドが応答しません。"));
		}
	}
}

const bool CBonSrcDecoder::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	// ソースデコーダのため入力は処理しない
	return false;
}

bool CBonSrcDecoder::LoadBonDriver(LPCTSTR pszFileName)
{
	if (m_hBonDriverLib != NULL) {
		SetError(TEXT("既に読み込まれています。"));
		return false;
	}

	if (pszFileName == NULL) {
		SetError(TEXT("ファイルが指定されていません。"));
		return false;
	}

	Trace(TEXT("BonDriver \"%s\" を読み込みます..."), pszFileName);

	m_hBonDriverLib = ::LoadLibrary(pszFileName);

	if (m_hBonDriverLib == NULL) {
		const DWORD ErrorCode = ::GetLastError();
		TCHAR szText[MAX_PATH + 64];

		StdUtil::snprintf(szText, _countof(szText),
						  TEXT("\"%s\" が読み込めません。"), pszFileName);
		SetError(szText);

		switch (ErrorCode) {
		case ERROR_MOD_NOT_FOUND:
			SetErrorAdvise(TEXT("ファイルが見つかりません。"));
			break;

		case ERROR_BAD_EXE_FORMAT:
			SetErrorAdvise(
#ifndef _WIN64
				TEXT("32")
#else
				TEXT("64")
#endif
				TEXT("ビット用の BonDriver ではないか、ファイルが破損している可能性があります。"));
			break;

		case ERROR_SXS_CANT_GEN_ACTCTX:
			SetErrorAdvise(TEXT("この BonDriver に必要なランタイムがインストールされていない可能性があります。"));
			break;

		default:
			StdUtil::snprintf(szText, _countof(szText), TEXT("エラーコード: 0x%x"), ErrorCode);
			SetErrorAdvise(szText);
		}

		SetErrorSystemMessageByErrorCode(ErrorCode);

		return false;
	}

	Trace(TEXT("BonDriver を読み込みました。"));

	return true;
}

bool CBonSrcDecoder::UnloadBonDriver()
{
	if (m_hBonDriverLib != NULL) {
		CloseTuner();

		Trace(TEXT("BonDriver を解放します..."));
		::FreeLibrary(m_hBonDriverLib);
		m_hBonDriverLib = NULL;
		Trace(TEXT("BonDriver を解放しました。"));
	}

	return true;
}

bool CBonSrcDecoder::IsBonDriverLoaded() const
{
	return m_hBonDriverLib != NULL;
}

bool CBonSrcDecoder::OpenTuner()
{
	if (m_hBonDriverLib == NULL) {
		SetError(ERR_NOTLOADED, TEXT("BonDriverが読み込まれていません。"));
		return false;
	}

	// オープンチェック
	if (m_pBonDriver) {
		SetError(ERR_ALREADYOPEN, TEXT("チューナは既に開かれています。"));
		return false;
	}

	Trace(TEXT("チューナを開いています..."));

	// ドライバポインタの取得
	PFCREATEBONDRIVER *pfCreateBonDriver =
		(PFCREATEBONDRIVER*)::GetProcAddress(m_hBonDriverLib, "CreateBonDriver");
	if (pfCreateBonDriver == NULL) {
		SetError(ERR_DRIVER,
				 TEXT("CreateBonDriver() のアドレスを取得できません。"),
				 TEXT("指定された DLL が BonDriver ではありません。"));
		return false;
	}
	m_pBonDriver = pfCreateBonDriver();
	if (m_pBonDriver == NULL) {
		SetError(ERR_DRIVER,
				 TEXT("IBonDriver を取得できません。"),
				 TEXT("CreateBonDriver() の呼び出しで NULL が返されました。"));
		return false;
	}

	const HANDLE hThread = ::GetCurrentThread();
	const int ThreadPriority = ::GetThreadPriority(hThread);
	BOOL bTunerOpened;

	try {
		// チューナを開く
		bTunerOpened = m_pBonDriver->OpenTuner();

		// なぜかスレッドの優先度を変えるBonDriverがあるので元に戻す
		::SetThreadPriority(hThread, ThreadPriority);

		if (!bTunerOpened) {
			SetError(ERR_TUNEROPEN,
					 TEXT("チューナを開けません。"),
					 TEXT("BonDriver にチューナを開くよう要求しましたがエラーが返されました。"));
			throw ERR_TUNEROPEN;
		}

		m_SetChannelCount = 0;
		m_TunerOpenTime = ::GetTickCount();

		// IBonDriver2インタフェース取得
		m_pBonDriver2 = dynamic_cast<IBonDriver2 *>(m_pBonDriver);

		// ストリーム受信スレッド起動
		m_EndEvent.Create();
		m_RequestEvent.Create();
		m_bIsPlaying = false;
		m_hStreamRecvThread = (HANDLE)::_beginthreadex(NULL, 0, StreamingThread, this, 0, NULL);
		if (!m_hStreamRecvThread) {
			SetError(ERR_INTERNAL, TEXT("ストリーム受信スレッドを作成できません。"));
			throw ERR_INTERNAL;
		}
	} catch (...) {
		if (bTunerOpened)
			m_pBonDriver->CloseTuner();
		m_pBonDriver->Release();
		m_pBonDriver = NULL;
		m_pBonDriver2 = NULL;
		return false;
	}

	ClearError();

	Trace(TEXT("チューナを開きました。"));

	return true;
}

bool CBonSrcDecoder::CloseTuner(void)
{
	// ストリーム停止
	m_bIsPlaying = false;

	if (m_hStreamRecvThread) {
		// ストリーム受信スレッド停止
		Trace(TEXT("ストリーム受信スレッドを停止しています..."));
		m_EndEvent.Set();
		if (::WaitForSingleObject(m_hStreamRecvThread, 5000UL) != WAIT_OBJECT_0) {
			// スレッド強制終了
			Trace(TEXT("ストリーム受信スレッドが応答しないため強制終了します。"));
			::TerminateThread(m_hStreamRecvThread, -1);
		}
		::CloseHandle(m_hStreamRecvThread);
		m_hStreamRecvThread = NULL;
	}

	m_EndEvent.Close();
	m_RequestQueue.clear();
	m_RequestEvent.Close();

	if (m_pBonDriver) {
		// チューナを閉じる
		Trace(TEXT("チューナを閉じています..."));
		m_pBonDriver->CloseTuner();

		// ドライバインスタンス開放
		Trace(TEXT("BonDriver インターフェースを解放しています..."));
		m_pBonDriver->Release();
		m_pBonDriver = NULL;
		m_pBonDriver2 = NULL;
		Trace(TEXT("BonDriver インターフェースを解放しました。"));
	}

	ResetStatus();

	m_SetChannelCount = 0;

	ClearError();

	return true;
}

bool CBonSrcDecoder::IsOpen() const
{
	return m_pBonDriver != NULL;
}

bool CBonSrcDecoder::Play(void)
{
	TRACE(TEXT("CBonSrcDecoder::Play()\n"));

	if (m_pBonDriver == NULL) {
		// チューナが開かれていない
		SetError(ERR_NOTOPEN, NULL);
		return false;
	}

	if (m_bIsPlaying) {
		// 既に再生中
		/*
		SetError(ERR_ALREADYPLAYING, NULL);
		return false;
		*/
		return true;
	}

	if (HasPendingRequest()) {
		SetError(ERR_PENDING, TEXT("前回の要求が完了しないため新しい要求を行えません。"));
		return false;
	}

	StreamingRequest Request[2];

	// 未処理のストリームを破棄する
	Request[0].Type = StreamingRequest::TYPE_PURGESTREAM;
	// 下位デコーダをリセットする
	Request[1].Type = StreamingRequest::TYPE_RESET;

	AddRequests(Request, 2);

	if (!WaitAllRequests(m_RequestTimeout)) {
		SetRequestTimeoutError();
		return false;
	}

	// ストリームを再生状態にする
	m_bIsPlaying = true;

	ClearError();

	return true;
}

bool CBonSrcDecoder::Stop(void)
{
	TRACE(TEXT("CBonSrcDecoder::Stop()\n"));

	if (!m_bIsPlaying) {
		// ストリームは再生中でない
		/*
		SetError(ERR_NOTPLAYING, NULL);
		return false;
		*/
		return true;
	}

	m_bIsPlaying = false;

	ClearError();

	return true;
}

bool CBonSrcDecoder::SetChannel(const BYTE byChannel)
{
	TRACE(TEXT("CBonSrcDecoder::SetChannel(%d)\n"), byChannel);

	if (m_pBonDriver == NULL) {
		// チューナが開かれていない
		SetError(ERR_NOTOPEN, NULL);
		return false;
	}

	if (HasPendingRequest()) {
		SetError(ERR_PENDING, TEXT("前回の要求が完了しないため新しい要求を行えません。"));
		return false;
	}

	StreamingRequest Request[3];
	int RequestCount = 0;

	// 未処理のストリームを破棄する
	if (m_bPurgeStreamOnChannelChange) {
		Request[RequestCount].Type = StreamingRequest::TYPE_PURGESTREAM;
		RequestCount++;
	}

	// チャンネルを変更する
	Request[RequestCount].Type = StreamingRequest::TYPE_SETCHANNEL;
	Request[RequestCount].SetChannel.Channel = byChannel;
	RequestCount++;

	// 下位デコーダをリセットする
	Request[RequestCount].Type = StreamingRequest::TYPE_RESET;
	RequestCount++;

	AddRequests(Request, RequestCount);

	if (!WaitAllRequests(m_RequestTimeout)) {
		SetRequestTimeoutError();
		return false;
	}

	if (!m_bRequestResult) {
		SetError(ERR_TUNER,
				 TEXT("チャンネルの変更が BonDriver に受け付けられません。"),
				 TEXT("IBonDriver::SetChannel() の呼び出しでエラーが返されました。"));
		return false;
	}

	ClearError();

	return true;
}

bool CBonSrcDecoder::SetChannel(const DWORD dwSpace, const DWORD dwChannel)
{
	TRACE(TEXT("CBonSrcDecoder::SetChannel(%lu, %lu)\n"), dwSpace, dwChannel);

	if (m_pBonDriver2 == NULL) {
		// チューナが開かれていない
		SetError(ERR_NOTOPEN, NULL);
		return false;
	}

	if (HasPendingRequest()) {
		SetError(ERR_PENDING, TEXT("前回の要求が完了しないため新しい要求を行えません。"));
		return false;
	}

	StreamingRequest Request[3];
	int RequestCount = 0;

	// 未処理のストリームを破棄する
	if (m_bPurgeStreamOnChannelChange) {
		Request[RequestCount].Type = StreamingRequest::TYPE_PURGESTREAM;
		RequestCount++;
	}

	// チャンネルを変更する
	Request[RequestCount].Type = StreamingRequest::TYPE_SETCHANNEL2;
	Request[RequestCount].SetChannel2.Space = dwSpace;
	Request[RequestCount].SetChannel2.Channel = dwChannel;
	RequestCount++;

	// 下位デコーダをリセットする
	Request[RequestCount].Type = StreamingRequest::TYPE_RESET;
	RequestCount++;

	AddRequests(Request, RequestCount);

	if (!WaitAllRequests(m_RequestTimeout)) {
		SetRequestTimeoutError();
		return false;
	}

	if (!m_bRequestResult) {
		SetError(ERR_TUNER,
				 TEXT("チャンネルの変更が BonDriver に受け付けられません。"),
				 TEXT("IBonDriver2::SetChannel() の呼び出しでエラーが返されました。"));
		return false;
	}

	ClearError();

	return true;
}

bool CBonSrcDecoder::SetChannelAndPlay(const DWORD dwSpace, const DWORD dwChannel)
{
	TRACE(TEXT("CBonSrcDecoder::SetChannelAndPlay(%lu, %lu)\n"), dwSpace, dwChannel);

	if (!SetChannel(dwSpace, dwChannel))
		return false;

	m_bIsPlaying = true;

	return true;
}

bool CBonSrcDecoder::IsBonDriver2(void) const
{
	// IBonDriver2インタフェースの使用可否を返す
	return m_pBonDriver2 != NULL;
}

LPCTSTR CBonSrcDecoder::GetSpaceName(const DWORD dwSpace) const
{
	// チューニング空間名を返す
	if (m_pBonDriver2 == NULL) {
		//SetError(ERR_NOTOPEN, NULL);
		return NULL;
	}
	return m_pBonDriver2->EnumTuningSpace(dwSpace);
}

LPCTSTR CBonSrcDecoder::GetChannelName(const DWORD dwSpace, const DWORD dwChannel) const
{
	// チャンネル名を返す
	if (m_pBonDriver2 == NULL) {
		//SetError(ERR_NOTOPEN, NULL);
		return NULL;
	}
	return m_pBonDriver2->EnumChannelName(dwSpace, dwChannel);
}

bool CBonSrcDecoder::PurgeStream(void)
{
	TRACE(TEXT("CBonSrcDecoder::PurgeStream()\n"));

	if (m_pBonDriver == NULL) {
		// チューナが開かれていない
		SetError(ERR_NOTOPEN, NULL);
		return false;
	}

	if (HasPendingRequest()) {
		SetError(ERR_PENDING, TEXT("前回の要求が完了しないため新しい要求を行えません。"));
		return false;
	}

	StreamingRequest Request;
	Request.Type = StreamingRequest::TYPE_PURGESTREAM;
	AddRequest(Request);

	if (!WaitAllRequests(m_RequestTimeout)) {
		SetRequestTimeoutError();
		return false;
	}

	ClearError();

	return true;
}

int CBonSrcDecoder::NumSpaces() const
{
	int i;

	for (i = 0; GetSpaceName(i) != NULL; i++);
	return i;
}

LPCTSTR CBonSrcDecoder::GetTunerName() const
{
	if (m_pBonDriver2 == NULL) {
		//SetError(ERR_NOTOPEN, NULL);
		return NULL;
	}
	return m_pBonDriver2->GetTunerName();
}

int CBonSrcDecoder::GetCurSpace() const
{
	if (m_pBonDriver2 == NULL) {
		//SetError(ERR_NOTOPEN, NULL);
		return -1;
	}
	return m_pBonDriver2->GetCurSpace();
}

int CBonSrcDecoder::GetCurChannel() const
{
	if (m_pBonDriver2 == NULL) {
		//SetError(ERR_NOTOPEN, NULL);
		return -1;
	}
	return m_pBonDriver2->GetCurChannel();
}

float CBonSrcDecoder::GetSignalLevel(void)
{
#if 0
	if (m_pBonDriver == NULL) {
		// チューナが開かれていない
		SetError(ERR_NOTOPEN, NULL);
		return 0.0f;
	}

	ClearError();

	// 信号レベルを返す
	return m_pBonDriver->GetSignalLevel();
#else
	return m_SignalLevel;
#endif
}

DWORD CBonSrcDecoder::GetBitRate() const
{
	return m_BitRateCalculator.GetBitRate();
}

DWORD CBonSrcDecoder::GetStreamRemain() const
{
	return m_StreamRemain;
}

bool CBonSrcDecoder::SetStreamThreadPriority(int Priority)
{
	if (m_StreamThreadPriority != Priority) {
		TRACE(TEXT("CBonSrcDecoder::SetStreamThreadPriority(%d)\n"), Priority);
		if (m_hStreamRecvThread) {
			if (!::SetThreadPriority(m_hStreamRecvThread, Priority))
				return false;
		}
		m_StreamThreadPriority = Priority;
	}
	return true;
}

void CBonSrcDecoder::SetPurgeStreamOnChannelChange(bool bPurge)
{
	TRACE(TEXT("CBonSrcDecoder::SetPurgeStreamOnChannelChange(%s)\n"),
		  bPurge ? TEXT("true") : TEXT("false"));
	m_bPurgeStreamOnChannelChange = bPurge;
}

bool CBonSrcDecoder::SetFirstChannelSetDelay(const DWORD Delay)
{
	if (Delay > FIRST_CHANNEL_SET_DELAY_MAX)
		return false;

	TRACE(TEXT("CBonSrcDecoder::SetFirstChannelSetDelay(%u)\n"), Delay);

	m_FirstChannelSetDelay = Delay;

	return true;
}

bool CBonSrcDecoder::SetMinChannelChangeInterval(const DWORD Interval)
{
	if (Interval > CHANNEL_CHANGE_INTERVAL_MAX)
		return false;

	TRACE(TEXT("CBonSrcDecoder::SetMinChannelChangeInterval(%u)\n"), Interval);

	m_MinChannelChangeInterval = Interval;

	return true;
}

unsigned int __stdcall CBonSrcDecoder::StreamingThread(LPVOID pParam)
{
	// チューナからTSデータを取り出すスレッド
	CBonSrcDecoder *pThis = static_cast<CBonSrcDecoder *>(pParam);

	::CoInitialize(NULL);

	::SetThreadPriority(::GetCurrentThread(), pThis->m_StreamThreadPriority);

	try {
		pThis->StreamingMain();
	} catch (...) {
		pThis->Trace(TEXT("ストリーム処理で例外が発生しました。"));
	}

	::CoUninitialize();

	TRACE(TEXT("CBonSrcDecoder::StreamingThread() return\n"));

	return 0;
}

void CBonSrcDecoder::StreamingMain()
{
	CMediaData TsStream(0x10000UL);

	m_BitRateCalculator.Initialize();

	DWORD Wait;
	do {
		m_RequestLock.Lock();
		if (!m_RequestQueue.empty()) {
			StreamingRequest &Req = m_RequestQueue.front();
			Req.bProcessing = true;
			StreamingRequest Request = Req;
			m_RequestLock.Unlock();

			switch (Request.Type) {
			case StreamingRequest::TYPE_SETCHANNEL:
				SetChannelWait();
				TRACE(TEXT("IBonDriver::SetChannel(%d)\n"), Request.SetChannel.Channel);
				m_bRequestResult = m_pBonDriver->SetChannel(Request.SetChannel.Channel);
				m_SetChannelTime = ::GetTickCount();
				break;

			case StreamingRequest::TYPE_SETCHANNEL2:
				SetChannelWait();
				TRACE(TEXT("IBonDriver2::SetChannel(%u, %u)\n"),
					  Request.SetChannel2.Space, Request.SetChannel2.Channel);
				m_bRequestResult = m_pBonDriver2->SetChannel(Request.SetChannel2.Space,
															 Request.SetChannel2.Channel);
				m_SetChannelTime = ::GetTickCount();
				break;

			case StreamingRequest::TYPE_PURGESTREAM:
				TRACE(TEXT("IBonDriver::PurgeStream()\n"));
				m_pBonDriver->PurgeTsStream();
				break;

			case StreamingRequest::TYPE_RESET:
				TRACE(TEXT("ResetDownstreamDecoder()\n"));
				ResetStatus();
				ResetDownstreamDecoder();
				break;
			}

			m_RequestLock.Lock();
			m_RequestQueue.pop_front();
			m_RequestEvent.Set();
			m_RequestLock.Unlock();

			Wait = 0;
		} else {
			m_RequestLock.Unlock();

			BYTE *pStreamData = NULL;
			DWORD dwStreamSize = 0;
			DWORD dwStreamRemain = 0;

			if (m_pBonDriver->GetTsStream(&pStreamData, &dwStreamSize, &dwStreamRemain)
					&& pStreamData && dwStreamSize) {
				if (m_bIsPlaying) {
					// 最上位デコーダに入力する
					TsStream.SetData(pStreamData, dwStreamSize);
					OutputMedia(&TsStream);
				}
			} else {
				dwStreamSize = 0;
				dwStreamRemain = 0;
			}

			if (m_BitRateCalculator.Update(dwStreamSize))
				m_SignalLevel = m_pBonDriver->GetSignalLevel();
			m_StreamRemain = dwStreamRemain;

			if (dwStreamRemain != 0)
				Wait = 0;
			else
				Wait = 10;
		}
	} while (m_EndEvent.Wait(Wait) == WAIT_TIMEOUT);
}

void CBonSrcDecoder::ResetStatus()
{
	m_SignalLevel = 0.0f;
	m_BitRateCalculator.Reset();
	m_StreamRemain = 0;
}

void CBonSrcDecoder::AddRequest(const StreamingRequest &Request)
{
	StreamingRequest Req = Request;
	Req.bProcessing = false;
	m_RequestLock.Lock();
	m_RequestQueue.push_back(Req);
	m_RequestEvent.Reset();
	m_RequestLock.Unlock();
}

void CBonSrcDecoder::AddRequests(const StreamingRequest *pRequestList, int RequestCount)
{
	m_RequestLock.Lock();
	for (int i = 0; i < RequestCount; i++) {
		StreamingRequest Request = pRequestList[i];
		Request.bProcessing = false;
		m_RequestQueue.push_back(Request);
	}
	m_RequestEvent.Reset();
	m_RequestLock.Unlock();
}

bool CBonSrcDecoder::WaitAllRequests(DWORD Timeout)
{
	while (HasPendingRequest()) {
		if (m_RequestEvent.Wait(Timeout) != WAIT_OBJECT_0)
			return false;
	}
	return true;
}

bool CBonSrcDecoder::HasPendingRequest()
{
	m_RequestLock.Lock();
	bool bPending = !m_RequestQueue.empty();
	m_RequestLock.Unlock();
	return bPending;
}

void CBonSrcDecoder::SetRequestTimeoutError()
{
	StreamingRequest Request;
	bool bRequest = false;

	m_RequestLock.Lock();
	if (!m_RequestQueue.empty()) {
		Request = m_RequestQueue.front();
		bRequest = true;
	}
	m_RequestLock.Unlock();

	if (bRequest && Request.bProcessing) {
		LPCTSTR pszText;

		switch (Request.Type) {
		case StreamingRequest::TYPE_SETCHANNEL:
		case StreamingRequest::TYPE_SETCHANNEL2:
			pszText = TEXT("BonDriver にチャンネル変更を要求しましたが応答がありません。");
			break;
		case StreamingRequest::TYPE_PURGESTREAM:
			pszText = TEXT("BonDriver に残りデータの破棄を要求しましたが応答がありません。");
			break;
		case StreamingRequest::TYPE_RESET:
			pszText = TEXT("リセット処理が完了しません。");
			break;
		default:
			pszText = TEXT("Internal error (Invalid request type)");
		}

		SetError(ERR_TIMEOUT, pszText);
	} else {
		SetError(ERR_TIMEOUT, TEXT("ストリーム受信スレッドが応答しません。"));
	}
}

void CBonSrcDecoder::SetChannelWait()
{
	DWORD Time, Wait;

	if (m_SetChannelCount == 0) {
		Time = m_TunerOpenTime;
		Wait = m_FirstChannelSetDelay;
	} else {
		Time = m_SetChannelTime;
		Wait = m_MinChannelChangeInterval;
	}

	if (Wait > 0) {
		DWORD Interval = ::GetTickCount() - Time;
		if (Interval < Wait) {
			TRACE(TEXT("SetChannel wait %u ms\n"), Wait - Interval);
			::Sleep(Wait - Interval);
		}
	}

	m_SetChannelCount++;
}
