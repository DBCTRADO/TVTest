// MediaViewer.cpp: CMediaViewer クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <Dvdmedia.h>
#include "MediaViewer.h"
#include "StdUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#pragma comment(lib,"quartz.lib")


//const CLSID CLSID_NullRenderer = {0xc1f400a4, 0x3f08, 0x11d3, {0x9f, 0x0b, 0x00, 0x60, 0x08, 0x03, 0x9e, 0x37}};
EXTERN_C const CLSID CLSID_NullRenderer;

static const DWORD LOCK_TIMEOUT = 2000;


static HRESULT SetVideoMediaType(CMediaType *pMediaType, int Width, int Height)
{
	static const REFERENCE_TIME TIME_PER_FRAME =
		static_cast<REFERENCE_TIME>(10000000.0 / 29.97 + 0.5);

#ifndef BONTSENGINE_H264_SUPPORT
	// MPEG-2

	// 映像メディアフォーマット設定
	pMediaType->InitMediaType();
	pMediaType->SetType(&MEDIATYPE_Video);
	pMediaType->SetSubtype(&MEDIASUBTYPE_MPEG2_VIDEO);
	pMediaType->SetVariableSize();
	pMediaType->SetTemporalCompression(TRUE);
	pMediaType->SetSampleSize(0);
	pMediaType->SetFormatType(&FORMAT_MPEG2Video);
	// フォーマット構造体確保
	MPEG2VIDEOINFO *pVideoInfo =
		pointer_cast<MPEG2VIDEOINFO *>(pMediaType->AllocFormatBuffer(sizeof(MPEG2VIDEOINFO)));
	if (!pVideoInfo)
		return E_OUTOFMEMORY;
	::ZeroMemory(pVideoInfo, sizeof(MPEG2VIDEOINFO));
	// ビデオヘッダ設定
	VIDEOINFOHEADER2 &VideoHeader = pVideoInfo->hdr;
	//::SetRect(&VideoHeader.rcSource, 0, 0, Width, Height);
	VideoHeader.AvgTimePerFrame = TIME_PER_FRAME;
	VideoHeader.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
	VideoHeader.bmiHeader.biWidth = Width;
	VideoHeader.bmiHeader.biHeight = Height;
#else
	// H.264
	pMediaType->InitMediaType();
	pMediaType->SetType(&MEDIATYPE_Video);
	pMediaType->SetSubtype(&MEDIASUBTYPE_H264);
	pMediaType->SetVariableSize();
	pMediaType->SetTemporalCompression(TRUE);
	pMediaType->SetSampleSize(0);
	pMediaType->SetFormatType(&FORMAT_VideoInfo);
	VIDEOINFOHEADER *pVideoInfo =
		pointer_cast<VIDEOINFOHEADER *>(pMediaType->AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
	if (!pVideoInfo)
		return E_OUTOFMEMORY;
	::ZeroMemory(pVideoInfo, sizeof(VIDEOINFOHEADER));
	pVideoInfo->dwBitRate = 32000000;
	pVideoInfo->AvgTimePerFrame = TIME_PER_FRAME;
	pVideoInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pVideoInfo->bmiHeader.biWidth = Width;
	pVideoInfo->bmiHeader.biHeight = Height;
	pVideoInfo->bmiHeader.biCompression = MAKEFOURCC('h','2','6','4');
#endif

	return S_OK;
}


//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CMediaViewer::CMediaViewer(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1UL, 0UL)
	, m_bInit(false)

	, m_pFilterGraph(NULL)
	, m_pMediaControl(NULL)

	, m_pSrcFilter(NULL)

	, m_pAudioDecoder(NULL)

	, m_pVideoDecoderFilter(NULL)

	, m_pVideoRenderer(NULL)
	, m_pAudioRenderer(NULL)

	, m_pszAudioFilterName(NULL)
	, m_pAudioFilter(NULL)

#ifndef BONTSENGINE_H264_SUPPORT
	, m_pMpeg2Parser(NULL)
#else
	, m_pH264Parser(NULL)
#endif

	, m_pszVideoDecoderName(NULL)

	, m_pMp2DemuxFilter(NULL)
	, m_pMp2DemuxVideoMap(NULL)
	, m_pMp2DemuxAudioMap(NULL)

	, m_wVideoEsPID(PID_INVALID)
	, m_wAudioEsPID(PID_INVALID)

	, m_wVideoWindowX(0)
	, m_wVideoWindowY(0)
	, m_VideoInfo()
	, m_hOwnerWnd(NULL)
#ifdef _DEBUG
	, m_dwRegister(0)
#endif

	, m_VideoRendererType(CVideoRenderer::RENDERER_UNDEFINED)
	, m_pszAudioRendererName(NULL)
	, m_ForceAspectX(0)
	, m_ForceAspectY(0)
	, m_ViewStretchMode(STRETCH_KEEPASPECTRATIO)
	, m_bNoMaskSideCut(false)
	, m_bIgnoreDisplayExtension(false)
	, m_bUseAudioRendererClock(true)
	, m_bAdjustAudioStreamTime(false)
	, m_bEnablePTSSync(false)
#ifdef BONTSENGINE_H264_SUPPORT
	, m_bAdjustVideoSampleTime(true)
	, m_bAdjustFrameRate(true)
#endif
	, m_pAudioStreamCallback(NULL)
	, m_pAudioStreamCallbackParam(NULL)
	, m_pImageMixer(NULL)
{
	// COMライブラリ初期化
	//::CoInitialize(NULL);
}

CMediaViewer::~CMediaViewer()
{
	CloseViewer();

	if (m_pszAudioFilterName)
		delete [] m_pszAudioFilterName;

	// COMライブラリ開放
	//::CoUninitialize();
}


void CMediaViewer::Reset(void)
{
	TRACE(TEXT("CMediaViewer::Reset()\n"));

	CTryBlockLock Lock(&m_DecoderLock);
	Lock.TryLock(LOCK_TIMEOUT);

	Flush();

	SetVideoPID(PID_INVALID);
	SetAudioPID(PID_INVALID);

	//m_VideoInfo.Reset();
}

const bool CMediaViewer::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	/*
	if(dwInputIndex >= GetInputNum())return false;

	CTsPacket *pTsPacket = dynamic_cast<CTsPacket *>(pMediaData);

	// 入力メディアデータは互換性がない
	if(!pTsPacket)return false;
	*/
	CTsPacket *pTsPacket = static_cast<CTsPacket *>(pMediaData);

	// フィルタグラフに入力
	if (m_pSrcFilter
			&& pTsPacket->GetPID() != 0x1FFF
			&& !pTsPacket->IsScrambled()) {
		return m_pSrcFilter->InputMedia(pTsPacket);
	}

	return false;
}

const bool CMediaViewer::OpenViewer(HWND hOwnerHwnd, HWND hMessageDrainHwnd,
			CVideoRenderer::RendererType RendererType,
			LPCWSTR pszVideoDecoder, LPCWSTR pszAudioDevice)
{
	CTryBlockLock Lock(&m_DecoderLock);
	if (!Lock.TryLock(LOCK_TIMEOUT)) {
		SetError(TEXT("タイムアウトエラーです。"));
		return false;
	}

	if (m_bInit) {
		SetError(TEXT("既にフィルタグラフが構築されています。"));
		return false;
	}

	TRACE(TEXT("CMediaViewer::OpenViewer() フィルタグラフ作成開始\n"));

	HRESULT hr=S_OK;

	IPin *pOutput=NULL;
	IPin *pOutputVideo=NULL;
	IPin *pOutputAudio=NULL;

	try {
		// フィルタグラフマネージャを構築する
		hr=::CoCreateInstance(CLSID_FilterGraph,NULL,CLSCTX_INPROC_SERVER,
				IID_IGraphBuilder,pointer_cast<LPVOID*>(&m_pFilterGraph));
		if (FAILED(hr)) {
			throw CBonException(hr,TEXT("フィルタグラフマネージャを作成できません。"));
		}
#ifdef _DEBUG
		AddToRot(m_pFilterGraph, &m_dwRegister);
#endif

		// IMediaControlインタフェースのクエリー
		hr=m_pFilterGraph->QueryInterface(IID_IMediaControl, pointer_cast<void**>(&m_pMediaControl));
		if (FAILED(hr)) {
			throw CBonException(hr,TEXT("メディアコントロールを取得できません。"));
		}

		Trace(TEXT("ソースフィルタの接続中..."));

		/* CBonSrcFilter */
		{
			// インスタンス作成
			m_pSrcFilter = static_cast<CBonSrcFilter*>(CBonSrcFilter::CreateInstance(NULL, &hr));
			if (m_pSrcFilter == NULL || FAILED(hr))
				throw CBonException(hr, TEXT("ソースフィルタを作成できません。"));
			m_pSrcFilter->SetOutputWhenPaused(RendererType == CVideoRenderer::RENDERER_DEFAULT);
			// フィルタグラフに追加
			hr = m_pFilterGraph->AddFilter(m_pSrcFilter, L"BonSrcFilter");
			if (FAILED(hr))
				throw CBonException(hr, TEXT("ソースフィルタをフィルタグラフに追加できません。"));
			// 出力ピンを取得
			pOutput = DirectShowUtil::GetFilterPin(m_pSrcFilter, PINDIR_OUTPUT);
			if (pOutput==NULL)
				throw CBonException(TEXT("ソースフィルタの出力ピンを取得できません。"));
			m_pSrcFilter->EnableSync(m_bEnablePTSSync);
		}

		Trace(TEXT("MPEG-2 Demultiplexerフィルタの接続中..."));

		/* MPEG-2 Demultiplexer */
		{
			CMediaType MediaTypeVideo;
			CMediaType MediaTypeAudio;
			IMpeg2Demultiplexer *pMpeg2Demuxer;

			hr=::CoCreateInstance(CLSID_MPEG2Demultiplexer,NULL,
					CLSCTX_INPROC_SERVER,IID_IBaseFilter,
					pointer_cast<LPVOID*>(&m_pMp2DemuxFilter));
			if (FAILED(hr))
				throw CBonException(hr,TEXT("MPEG-2 Demultiplexerフィルタを作成できません。"),
									TEXT("MPEG-2 Demultiplexerフィルタがインストールされているか確認してください。"));
			hr=DirectShowUtil::AppendFilterAndConnect(m_pFilterGraph,
								m_pMp2DemuxFilter,L"Mpeg2Demuxer",&pOutput);
			if (FAILED(hr))
				throw CBonException(hr,TEXT("MPEG-2 Demultiplexerをフィルタグラフに追加できません。"));
			// この時点でpOutput==NULLのはずだが念のため
			SAFE_RELEASE(pOutput);

			// IMpeg2Demultiplexerインタフェースのクエリー
			hr=m_pMp2DemuxFilter->QueryInterface(IID_IMpeg2Demultiplexer,
												 pointer_cast<void**>(&pMpeg2Demuxer));
			if (FAILED(hr))
				throw CBonException(hr,TEXT("MPEG-2 Demultiplexerインターフェースを取得できません。"),
									TEXT("互換性のないスプリッタの優先度がMPEG-2 Demultiplexerより高くなっている可能性があります。"));

			// 映像メディアフォーマット設定
			hr = SetVideoMediaType(&MediaTypeVideo, 1920, 1080);
			if (FAILED(hr))
				throw CBonException(TEXT("メモリが確保できません。"));
			// 映像出力ピン作成
			hr = pMpeg2Demuxer->CreateOutputPin(&MediaTypeVideo, L"Video", &pOutputVideo);
			if (FAILED(hr)) {
				pMpeg2Demuxer->Release();
				throw CBonException(hr, TEXT("MPEG-2 Demultiplexerの映像出力ピンを作成できません。"));
			}
			// 音声メディアフォーマット設定
			MediaTypeAudio.InitMediaType();
			MediaTypeAudio.SetType(&MEDIATYPE_Audio);
			MediaTypeAudio.SetSubtype(&MEDIASUBTYPE_NULL);
			MediaTypeAudio.SetVariableSize();
			MediaTypeAudio.SetTemporalCompression(TRUE);
			MediaTypeAudio.SetSampleSize(0);
			MediaTypeAudio.SetFormatType(&FORMAT_None);
			// 音声出力ピン作成
			hr=pMpeg2Demuxer->CreateOutputPin(&MediaTypeAudio,L"Audio",&pOutputAudio);
			pMpeg2Demuxer->Release();
			if (FAILED(hr))
				throw CBonException(hr,TEXT("MPEG-2 Demultiplexerの音声出力ピンを作成できません。"));
			// 映像出力ピンのIMPEG2PIDMapインタフェースのクエリー
			hr=pOutputVideo->QueryInterface(__uuidof(IMPEG2PIDMap),pointer_cast<void**>(&m_pMp2DemuxVideoMap));
			if (FAILED(hr))
				throw CBonException(hr,TEXT("映像出力ピンのIMPEG2PIDMapを取得できません。"));
			// 音声出力ピンのIMPEG2PIDMapインタフェースのクエリ
			hr=pOutputAudio->QueryInterface(__uuidof(IMPEG2PIDMap),pointer_cast<void**>(&m_pMp2DemuxAudioMap));
			if (FAILED(hr))
				throw CBonException(hr,TEXT("音声出力ピンのIMPEG2PIDMapを取得できません。"));
		}

#ifndef BONTSENGINE_H264_SUPPORT
		Trace(TEXT("MPEG-2パーサフィルタの接続中..."));

		/* CMpeg2ParserFilter */
		{
			// インスタンス作成
			m_pMpeg2Parser = static_cast<CMpeg2ParserFilter*>(CMpeg2ParserFilter::CreateInstance(NULL, &hr));
			if ((!m_pMpeg2Parser) || FAILED(hr))
				throw CBonException(hr,TEXT("MPEG-2パーサフィルタを作成できません。"));
			m_pMpeg2Parser->SetVideoInfoCallback(OnVideoInfo,this);
			// madVR は映像サイズの変化時に MediaType を設定しないと新しいサイズが適用されない
			m_pMpeg2Parser->SetAttachMediaType(RendererType==CVideoRenderer::RENDERER_madVR);
			// フィルタの追加と接続
			hr=DirectShowUtil::AppendFilterAndConnect(
				m_pFilterGraph,m_pMpeg2Parser,L"Mpeg2ParserFilter",&pOutputVideo);
			if (FAILED(hr))
				throw CBonException(hr,TEXT("MPEG-2パーサフィルタをフィルタグラフに追加できません。"));
		}
#else
		Trace(TEXT("H.264パーサフィルタの接続中..."));

		/* CH264ParserFilter */
		{
			// インスタンス作成
			m_pH264Parser = static_cast<CH264ParserFilter*>(CH264ParserFilter::CreateInstance(NULL, &hr));
			if ((!m_pH264Parser) || FAILED(hr))
				throw CBonException(TEXT("H.264パーサフィルタを作成できません。"));
			m_pH264Parser->SetVideoInfoCallback(OnVideoInfo,this);
			m_pH264Parser->SetAdjustTime(m_bAdjustVideoSampleTime);
			m_pH264Parser->SetAdjustFrameRate(m_bAdjustFrameRate);
			// madVR は映像サイズの変化時に MediaType を設定しないと新しいサイズが適用されない
			m_pH264Parser->SetAttachMediaType(RendererType==CVideoRenderer::RENDERER_madVR);
			// フィルタの追加と接続
			hr=DirectShowUtil::AppendFilterAndConnect(
				m_pFilterGraph,m_pH264Parser,L"H264ParserFilter",&pOutputVideo);
			if (FAILED(hr))
				throw CBonException(hr,TEXT("H.264パーサフィルタをフィルタグラフに追加できません。"));
		}
#endif	// BONTSENGINE_H264_SUPPORT

		Trace(TEXT("音声デコーダの接続中..."));

#if 1
		/* CAudioDecFilter */
		{
			// CAudioDecFilterインスタンス作成
			m_pAudioDecoder = static_cast<CAudioDecFilter*>(CAudioDecFilter::CreateInstance(NULL, &hr));
			if (!m_pAudioDecoder || FAILED(hr))
				throw CBonException(hr,TEXT("音声デコーダフィルタを作成できません。"));
			// フィルタの追加と接続
			hr=DirectShowUtil::AppendFilterAndConnect(
				m_pFilterGraph,m_pAudioDecoder,L"AudioDecFilter",&pOutputAudio);
			if (FAILED(hr))
				throw CBonException(hr,TEXT("音声デコーダフィルタをフィルタグラフに追加できません。"));

			m_pAudioDecoder->SetJitterCorrection(m_bAdjustAudioStreamTime);
			if (m_pAudioStreamCallback)
				m_pAudioDecoder->SetStreamCallback(m_pAudioStreamCallback,
												   m_pAudioStreamCallbackParam);
		}
#else
		/*
			外部AACデコーダを利用すると、チャンネル数が切り替わった際に音が出なくなる、
			デュアルモノラルがステレオとして再生される、といった問題が出る
		*/

		/* CAacParserFilter */
		{
			CAacParserFilter *m_pAacParser;
			// CAacParserFilterインスタンス作成
			m_pAacParser=static_cast<CAacParserFilter*>(CAacParserFilter::CreateInstance(NULL, &hr));
			if (!m_pAacParser || FAILED(hr))
				throw CBonException(hr,TEXT("AACパーサフィルタを作成できません。"));
			// フィルタの追加と接続
			hr=DirectShowUtil::AppendFilterAndConnect(
				m_pFilterGraph,m_pAacParser,L"AacParserFilter",&pOutputAudio);
			if (FAILED(hr))
				throw CBonException(TEXT("AACパーサフィルタをフィルタグラフに追加できません。"));
			m_pAacParser->Release();
		}

		/* AACデコーダー */
		{
			CDirectShowFilterFinder FilterFinder;

			// 検索
			if(!FilterFinder.FindFilter(&MEDIATYPE_Audio,&MEDIASUBTYPE_AAC))
				throw CBonException(TEXT("AACデコーダが見付かりません。"),
									TEXT("AACデコーダがインストールされているか確認してください。"));

			WCHAR szAacDecoder[128];
			CLSID idAac;
			bool bConnectSuccess=false;
			IBaseFilter *pAacDecFilter=NULL;

			for (int i=0;i<FilterFinder.GetFilterCount();i++){
				if (FilterFinder.GetFilterInfo(i,&idAac,szAacDecoder,128)) {
					if (pszAudioDecoder!=NULL && pszAudioDecoder[0]!='\0'
							&& ::lstrcmpi(szAacDecoder,pszAudioDecoder)!=0)
						continue;
					hr=DirectShowUtil::AppendFilterAndConnect(m_pFilterGraph,
							idAac,szAacDecoder,&pAacDecFilter,
							&pOutputAudio);
					if (SUCCEEDED(hr)) {
						TRACE(TEXT("AAC decoder connected : %s\n"),szAacDecoder);
						bConnectSuccess=true;
						break;
					}
				}
			}
			// どれかのフィルタで接続できたか
			if (bConnectSuccess) {
				SAFE_RELEASE(pAacDecFilter);
				//m_pszAacDecoderName=StdUtil::strdup(szAacDecoder);
			} else {
				throw CBonException(TEXT("AACデコーダフィルタをフィルタグラフに追加できません。"),
									TEXT("設定で有効なAACデコーダが選択されているか確認してください。"));
			}
		}
#endif

		/* ユーザー指定の音声フィルタの接続 */
		if (m_pszAudioFilterName) {
			Trace(TEXT("音声フィルタの接続中..."));

			// 検索
			bool bConnectSuccess=false;
			CDirectShowFilterFinder FilterFinder;
			if (FilterFinder.FindFilter(&MEDIATYPE_Audio,&MEDIASUBTYPE_PCM)) {
				WCHAR szAudioFilter[128];
				CLSID idAudioFilter;

				for (int i=0;i<FilterFinder.GetFilterCount();i++) {
					if (FilterFinder.GetFilterInfo(i,&idAudioFilter,szAudioFilter,128)) {
						if (::lstrcmpi(m_pszAudioFilterName,szAudioFilter)!=0)
							continue;
						hr=DirectShowUtil::AppendFilterAndConnect(m_pFilterGraph,
								idAudioFilter,szAudioFilter,&m_pAudioFilter,
								&pOutputAudio,NULL,true);
						if (SUCCEEDED(hr)) {
							TRACE(TEXT("音声フィルタ接続 : %s\n"),szAudioFilter);
							bConnectSuccess=true;
						}
						break;
					}
				}
			}
			if (!bConnectSuccess) {
				throw CBonException(hr,
					TEXT("音声フィルタをフィルタグラフに追加できません。"),
					TEXT("音声フィルタが利用できないか、音声デバイスに対応していない可能性があります。"));
			}
		}

#ifndef BONTSENGINE_H264_SUPPORT
		Trace(TEXT("MPEG-2デコーダの接続中..."));

		/* Mpeg2デコーダー */
		{
			CDirectShowFilterFinder FilterFinder;

			// 検索
			if(!FilterFinder.FindFilter(&MEDIATYPE_Video,&MEDIASUBTYPE_MPEG2_VIDEO))
				throw CBonException(TEXT("MPEG-2デコーダが見付かりません。"),
									TEXT("MPEG-2デコーダがインストールされているか確認してください。"));

			WCHAR szMpeg2Decoder[128];
			CLSID idMpeg2Vid;
			bool bConnectSuccess=false;

			for (int i=0;i<FilterFinder.GetFilterCount();i++){
				if (FilterFinder.GetFilterInfo(i,&idMpeg2Vid,szMpeg2Decoder,128)) {
					if (pszVideoDecoder!=NULL && pszVideoDecoder[0]!='\0'
							&& ::lstrcmpi(szMpeg2Decoder,pszVideoDecoder)!=0)
						continue;
					hr=DirectShowUtil::AppendFilterAndConnect(m_pFilterGraph,
							idMpeg2Vid,szMpeg2Decoder,&m_pVideoDecoderFilter,
							&pOutputVideo,NULL,true);
					if (SUCCEEDED(hr)) {
						bConnectSuccess=true;
						break;
					}
				}
			}
			// どれかのフィルタで接続できたか
			if (bConnectSuccess) {
				m_pszVideoDecoderName=StdUtil::strdup(szMpeg2Decoder);
			} else {
				throw CBonException(hr,TEXT("MPEG-2デコーダフィルタをフィルタグラフに追加できません。"),
					TEXT("設定で有効なMPEG-2デコーダが選択されているか確認してください。\nまた、レンダラを変えてみてください。"));
			}
		}

#ifndef MPEG2PARSERFILTER_INPLACE
		/*
			CyberLinkのデコーダとデフォルトレンダラの組み合わせで
			1080x1080(4:3)の映像が正方形に表示される問題に対応
			…しようと思ったが変になるので保留
		*/
		if (::StrStrI(m_pszVideoDecoderName, TEXT("CyberLink")) != NULL)
			m_pMpeg2Parser->SetFixSquareDisplay(true);
#endif
#else	// ndef BONTSENGINE_H264_SUPPORT
		Trace(TEXT("H.264デコーダの接続中..."));

		/* H.264デコーダー */
		{
			CDirectShowFilterFinder FilterFinder;

			// 検索
			if(!FilterFinder.FindFilter(&MEDIATYPE_Video,&MEDIASUBTYPE_H264))
				throw CBonException(TEXT("H.264デコーダが見付かりません。"),
									TEXT("H.264デコーダがインストールされているか確認してください。"));

			WCHAR szH264Decoder[128];
			CLSID idH264Decoder;
			bool bConnectSuccess=false;

			for (int i=0;i<FilterFinder.GetFilterCount();i++){
				if (FilterFinder.GetFilterInfo(i,&idH264Decoder,szH264Decoder,128)) {
					if (pszVideoDecoder!=NULL && pszVideoDecoder[0]!='\0'
							&& ::lstrcmpi(szH264Decoder,pszVideoDecoder)!=0)
						continue;
					hr=DirectShowUtil::AppendFilterAndConnect(m_pFilterGraph,
							idH264Decoder,szH264Decoder,&m_pVideoDecoderFilter,
							&pOutputVideo,NULL,true);
					if (SUCCEEDED(hr)) {
						bConnectSuccess=true;
						break;
					}
				}
			}
			// どれかのフィルタで接続できたか
			if (bConnectSuccess) {
				m_pszVideoDecoderName=StdUtil::strdup(szH264Decoder);
			} else {
				throw CBonException(hr,TEXT("H.264デコーダフィルタをフィルタグラフに追加できません。"),
					TEXT("設定で有効なH.264デコーダが選択されているか確認してください。\nまた、レンダラを変えてみてください。"));
			}
		}
#endif	// BONTSENGINE_H264_SUPPORT

		Trace(TEXT("映像レンダラの構築中..."));

		if (!CVideoRenderer::CreateRenderer(RendererType,&m_pVideoRenderer)) {
			throw CBonException(TEXT("映像レンダラを作成できません。"),
								TEXT("設定で有効なレンダラが選択されているか確認してください。"));
		}
		if (!m_pVideoRenderer->Initialize(m_pFilterGraph,pOutputVideo,
										  hOwnerHwnd,hMessageDrainHwnd)) {
			throw CBonException(m_pVideoRenderer->GetLastErrorException());
		}
		m_VideoRendererType=RendererType;

		Trace(TEXT("音声レンダラの構築中..."));

		// 音声レンダラ構築
		{
			bool fOK = false;

			if (pszAudioDevice != NULL && pszAudioDevice[0] != '\0') {
				CDirectShowDeviceEnumerator DevEnum;

				if (DevEnum.CreateFilter(CLSID_AudioRendererCategory,
										 pszAudioDevice, &m_pAudioRenderer)) {
					m_pszAudioRendererName=StdUtil::strdup(pszAudioDevice);
					fOK = true;
				}
			}
			if (!fOK) {
				hr = ::CoCreateInstance(CLSID_DSoundRender, NULL,
										CLSCTX_INPROC_SERVER, IID_IBaseFilter,
										pointer_cast<LPVOID*>(&m_pAudioRenderer));
				if (SUCCEEDED(hr)) {
					m_pszAudioRendererName=StdUtil::strdup(TEXT("DirectSound Renderer"));
					fOK = true;
				}
			}
			if (fOK) {
				hr = DirectShowUtil::AppendFilterAndConnect(m_pFilterGraph,
						m_pAudioRenderer, L"Audio Renderer", &pOutputAudio);
				if (SUCCEEDED(hr)) {
#ifdef _DEBUG
					if (pszAudioDevice != NULL && pszAudioDevice[0] != '\0')
						TRACE(TEXT("音声デバイス %s を接続\n"), pszAudioDevice);
#endif
					if (m_bUseAudioRendererClock) {
						IMediaFilter *pMediaFilter;

						if (SUCCEEDED(m_pFilterGraph->QueryInterface(IID_IMediaFilter,
								pointer_cast<void**>(&pMediaFilter)))) {
							IReferenceClock *pReferenceClock;

							if (SUCCEEDED(m_pAudioRenderer->QueryInterface(IID_IReferenceClock,
									pointer_cast<void**>(&pReferenceClock)))) {
								pMediaFilter->SetSyncSource(pReferenceClock);
								pReferenceClock->Release();
								TRACE(TEXT("グラフのクロックに音声レンダラを選択\n"));
							}
							pMediaFilter->Release();
						}
					}
					fOK = true;
				} else {
					fOK = false;
				}
				if (!fOK) {
					hr = m_pFilterGraph->Render(pOutputAudio);
					if (FAILED(hr))
						throw CBonException(hr, TEXT("音声レンダラを接続できません。"),
							TEXT("設定で有効な音声デバイスが選択されているか確認してください。"));
				}
			} else {
				// 音声デバイスが無い?
				// Nullレンダラを繋げておく
				hr = ::CoCreateInstance(CLSID_NullRenderer, NULL,
										CLSCTX_INPROC_SERVER, IID_IBaseFilter,
										pointer_cast<LPVOID*>(&m_pAudioRenderer));
				if (SUCCEEDED(hr)) {
					hr = DirectShowUtil::AppendFilterAndConnect(m_pFilterGraph,
						m_pAudioRenderer, L"Null Audio Renderer", &pOutputAudio);
					if (FAILED(hr)) {
						throw CBonException(hr, TEXT("Null音声レンダラを接続できません。"));
					}
					m_pszAudioRendererName=StdUtil::strdup(TEXT("Null Renderer"));
					TRACE(TEXT("Nullレンダラを接続\n"));
				}
			}
		}

		/*
			デフォルトでMPEG-2 Demultiplexerがグラフのクロックに
			設定されるらしいが、一応設定しておく
		*/
		if (!m_bUseAudioRendererClock) {
			IMediaFilter *pMediaFilter;

			if (SUCCEEDED(m_pFilterGraph->QueryInterface(
					IID_IMediaFilter,pointer_cast<void**>(&pMediaFilter)))) {
				IReferenceClock *pReferenceClock;

				if (SUCCEEDED(m_pMp2DemuxFilter->QueryInterface(
						IID_IReferenceClock,pointer_cast<void**>(&pReferenceClock)))) {
					pMediaFilter->SetSyncSource(pReferenceClock);
					pReferenceClock->Release();
					TRACE(TEXT("グラフのクロックにMPEG-2 Demultiplexerを選択\n"));
				}
				pMediaFilter->Release();
			}
		}

		// オーナウィンドウ設定
		m_hOwnerWnd = hOwnerHwnd;
		RECT rc;
		::GetClientRect(hOwnerHwnd, &rc);
		m_wVideoWindowX = (WORD)rc.right;
		m_wVideoWindowY = (WORD)rc.bottom;

		m_bInit=true;

		ULONG PID;
		if (m_wVideoEsPID != PID_INVALID) {
			PID = m_wVideoEsPID;
			if (FAILED(m_pMp2DemuxVideoMap->MapPID(1, &PID, MEDIA_ELEMENTARY_STREAM)))
				m_wVideoEsPID = PID_INVALID;
		}
		if (m_wAudioEsPID != PID_INVALID) {
			PID = m_wAudioEsPID;
			if (FAILED(m_pMp2DemuxAudioMap->MapPID(1, &PID, MEDIA_ELEMENTARY_STREAM)))
				m_wAudioEsPID = PID_INVALID;
		}
	} catch (CBonException &Exception) {
		SetError(Exception);
		if (Exception.GetErrorCode()!=0) {
			TCHAR szText[MAX_ERROR_TEXT_LEN+32];
			int Length;

			Length=::AMGetErrorText(Exception.GetErrorCode(),szText,MAX_ERROR_TEXT_LEN);
			::wsprintf(szText+Length,TEXT("\nエラーコード(HRESULT) 0x%08X"),Exception.GetErrorCode());
			SetErrorSystemMessage(szText);
		}

		SAFE_RELEASE(pOutput);
		SAFE_RELEASE(pOutputVideo);
		SAFE_RELEASE(pOutputAudio);
		CloseViewer();

		TRACE(TEXT("フィルタグラフ構築失敗 : %s\n"), GetLastErrorText());
		return false;
	}

	SAFE_RELEASE(pOutputVideo);
	SAFE_RELEASE(pOutputAudio);

	ClearError();

	TRACE(TEXT("フィルタグラフ構築成功\n"));
	return true;
}

void CMediaViewer::CloseViewer(void)
{
	CTryBlockLock Lock(&m_DecoderLock);
	Lock.TryLock(LOCK_TIMEOUT);

	/*
	if (!m_bInit)
		return;
	*/

	if (m_pFilterGraph) {
		Trace(TEXT("フィルタグラフを停止しています..."));
		m_pFilterGraph->Abort();
		Stop();
	}

	Trace(TEXT("COMインスタンスを解放しています..."));

	// COMインスタンスを開放する
	if (m_pVideoRenderer!=NULL) {
		m_pVideoRenderer->Finalize();
	}

	if (m_pImageMixer!=NULL) {
		delete m_pImageMixer;
		m_pImageMixer=NULL;
	}

	if (m_pszVideoDecoderName!=NULL) {
		delete [] m_pszVideoDecoderName;
		m_pszVideoDecoderName=NULL;
	}

	SAFE_RELEASE(m_pVideoDecoderFilter);

	SAFE_RELEASE(m_pAudioDecoder);

	SAFE_RELEASE(m_pAudioRenderer);

#ifndef BONTSENGINE_H264_SUPPORT
	SAFE_RELEASE(m_pMpeg2Parser);
#else
	SAFE_RELEASE(m_pH264Parser);
#endif

	SAFE_RELEASE(m_pMp2DemuxAudioMap);
	SAFE_RELEASE(m_pMp2DemuxVideoMap);
	SAFE_RELEASE(m_pMp2DemuxFilter);

	SAFE_RELEASE(m_pSrcFilter);

	SAFE_RELEASE(m_pAudioFilter);

	SAFE_RELEASE(m_pMediaControl);

#ifdef _DEBUG
	if(m_dwRegister!=0){
		RemoveFromRot(m_dwRegister);
		m_dwRegister = 0;
	}
#endif

	if (m_pFilterGraph) {
		Trace(TEXT("フィルタグラフを解放しています..."));
#ifdef _DEBUG
		TRACE(TEXT("FilterGraph RefCount = %d\n"),DirectShowUtil::GetRefCount(m_pFilterGraph));
#endif
		SAFE_RELEASE(m_pFilterGraph);
	}

	if (m_pVideoRenderer!=NULL) {
		delete m_pVideoRenderer;
		m_pVideoRenderer=NULL;
	}

	if (m_pszAudioRendererName!=NULL) {
		delete [] m_pszAudioRendererName;
		m_pszAudioRendererName=NULL;
	}

	m_bInit=false;
}

const bool CMediaViewer::IsOpen() const
{
	return m_bInit;
}

const bool CMediaViewer::Play(void)
{
	TRACE(TEXT("CMediaViewer::Play()\n"));

	CTryBlockLock Lock(&m_DecoderLock);
	if (!Lock.TryLock(LOCK_TIMEOUT))
		return false;

	if(!m_pMediaControl)return false;

	// フィルタグラフを再生する

	//return m_pMediaControl->Run()==S_OK;

	if (m_pMediaControl->Run()!=S_OK) {
		int i;
		OAFilterState fs;

		for (i=0;i<20;i++) {
			if (m_pMediaControl->GetState(100,&fs)==S_OK && fs==State_Running)
				return true;
		}
		return false;
	}
	return true;
}

const bool CMediaViewer::Stop(void)
{
	TRACE(TEXT("CMediaViewer::Stop()\n"));

	CTryBlockLock Lock(&m_DecoderLock);
	if (!Lock.TryLock(LOCK_TIMEOUT))
		return false;

	if (!m_pMediaControl)
		return false;

	if (m_pSrcFilter)
		//m_pSrcFilter->Reset();
		m_pSrcFilter->Flush();

	// フィルタグラフを停止する
	return m_pMediaControl->Stop()==S_OK;
}

const bool CMediaViewer::Pause()
{
	TRACE(TEXT("CMediaViewer::Pause()\n"));

	CTryBlockLock Lock(&m_DecoderLock);
	if (!Lock.TryLock(LOCK_TIMEOUT))
		return false;

	if (!m_pMediaControl)
		return false;

	if (m_pSrcFilter)
		//m_pSrcFilter->Reset();
		m_pSrcFilter->Flush();

	if (m_pMediaControl->Pause()!=S_OK) {
		int i;
		OAFilterState fs;
		HRESULT hr;

		for (i=0;i<20;i++) {
			hr=m_pMediaControl->GetState(100,&fs);
			if ((hr==S_OK || hr==VFW_S_CANT_CUE) && fs==State_Paused)
				return true;
		}
		return false;
	}
	return true;
}

const bool CMediaViewer::Flush()
{
	TRACE(TEXT("CMediaViewer::Flush()\n"));

	/*
	CTryBlockLock Lock(&m_DecoderLock);
	if (!Lock.TryLock(LOCK_TIMEOUT))
		return false;
	*/

	if (!m_pSrcFilter)
		return false;

	m_pSrcFilter->Flush();
	return true;
}

const bool CMediaViewer::SetVideoPID(const WORD wPID)
{
	// 映像出力ピンにPIDをマッピングする

	if (wPID == m_wVideoEsPID)
		return true;

	TRACE(TEXT("CMediaViewer::SetVideoPID() %04X <- %04X\n"), wPID, m_wVideoEsPID);

	if (m_pMp2DemuxVideoMap) {
		ULONG TempPID;

		// 現在のPIDをアンマップ
		if (m_wVideoEsPID != PID_INVALID) {
			TempPID = m_wVideoEsPID;
			if (m_pMp2DemuxVideoMap->UnmapPID(1UL, &TempPID) != S_OK)
				return false;
		}

		// 新規にPIDをマップ
		if (wPID != PID_INVALID) {
			TempPID = wPID;
			if (m_pMp2DemuxVideoMap->MapPID(1UL, &TempPID, MEDIA_ELEMENTARY_STREAM) != S_OK)
				return false;
		}
	}

	if (m_pSrcFilter!=NULL)
		m_pSrcFilter->SetVideoPID(wPID);

	m_wVideoEsPID = wPID;

	return true;
}

const bool CMediaViewer::SetAudioPID(const WORD wPID)
{
	// 音声出力ピンにPIDをマッピングする

	if (wPID == m_wAudioEsPID)
		return true;

	TRACE(TEXT("CMediaViewer::SetAudioPID() %04X <- %04X\n"), wPID, m_wAudioEsPID);

	if (m_pMp2DemuxAudioMap) {
		ULONG TempPID;

		// 現在のPIDをアンマップ
		if (m_wAudioEsPID != PID_INVALID) {
			TempPID = m_wAudioEsPID;
			if (m_pMp2DemuxAudioMap->UnmapPID(1UL, &TempPID) != S_OK)
				return false;
		}

		// 新規にPIDをマップ
		if (wPID != PID_INVALID) {
			TempPID = wPID;
			if (m_pMp2DemuxAudioMap->MapPID(1UL, &TempPID, MEDIA_ELEMENTARY_STREAM) != S_OK)
				return false;
		}
	}

	if (m_pSrcFilter!=NULL)
		m_pSrcFilter->SetAudioPID(wPID);

	m_wAudioEsPID = wPID;

	return true;
}

const WORD CMediaViewer::GetVideoPID(void) const
{
	return m_wVideoEsPID;
}

const WORD CMediaViewer::GetAudioPID(void) const
{
	return m_wAudioEsPID;
}

void CMediaViewer::OnVideoInfo(const CVideoParser::VideoInfo *pVideoInfo,const LPVOID pParam)
{
	// ビデオ情報の更新
	CMediaViewer *pThis=static_cast<CMediaViewer*>(pParam);

	/*if (pThis->m_VideoInfo != *pVideoInfo)*/ {
		// ビデオ情報の更新
		CBlockLock Lock(&pThis->m_ResizeLock);

		pThis->m_VideoInfo = *pVideoInfo;
		//pThis->AdjustVideoPosition();
	}

	pThis->SendDecoderEvent(EID_VIDEO_SIZE_CHANGED);
}

const bool CMediaViewer::AdjustVideoPosition()
{
	// 映像の位置を調整する
	if (m_pVideoRenderer && m_wVideoWindowX > 0 && m_wVideoWindowY > 0
			&& m_VideoInfo.m_OrigWidth > 0 && m_VideoInfo.m_OrigHeight > 0) {
		long WindowWidth, WindowHeight, DestWidth, DestHeight;

		WindowWidth = m_wVideoWindowX;
		WindowHeight = m_wVideoWindowY;
		if (m_ViewStretchMode == STRETCH_FIT) {
			// ウィンドウサイズに合わせる
			DestWidth = WindowWidth;
			DestHeight = WindowHeight;
		} else {
			int AspectX, AspectY;
			double window_rate = (double)WindowWidth / (double)WindowHeight;
			double aspect_rate;

			if (m_ForceAspectX > 0 && m_ForceAspectY > 0) {
				// アスペクト比が指定されている
				AspectX = m_ForceAspectX;
				AspectY = m_ForceAspectY;
			} else if (m_VideoInfo.m_AspectRatioX > 0 && m_VideoInfo.m_AspectRatioY > 0) {
				// 映像のアスペクト比を使用する
				AspectX = m_VideoInfo.m_AspectRatioX;
				AspectY = m_VideoInfo.m_AspectRatioY;
				if (m_bIgnoreDisplayExtension
						&& m_VideoInfo.m_DisplayWidth > 0
						&& m_VideoInfo.m_DisplayHeight > 0) {
					AspectX = AspectX * 3 * m_VideoInfo.m_OrigWidth / m_VideoInfo.m_DisplayWidth;
					AspectY = AspectY * 3 * m_VideoInfo.m_OrigHeight / m_VideoInfo.m_DisplayHeight;
				}
			} else {
				// アスペクト比不明
				if (m_VideoInfo.m_DisplayHeight == 1080) {
					AspectX = 16;
					AspectY = 9;
				} else if (m_VideoInfo.m_DisplayWidth > 0 && m_VideoInfo.m_DisplayHeight > 0) {
					AspectX = m_VideoInfo.m_DisplayWidth;
					AspectY = m_VideoInfo.m_DisplayHeight;
				} else {
					AspectX = WindowWidth;
					AspectY = WindowHeight;
				}
			}
			aspect_rate = (double)AspectX / (double)AspectY;
			if ((m_ViewStretchMode==STRETCH_KEEPASPECTRATIO && aspect_rate>window_rate)
					|| (m_ViewStretchMode==STRETCH_CUTFRAME && aspect_rate<window_rate)) {
				DestWidth = WindowWidth;
				DestHeight = DestWidth * AspectY  / AspectX;
			} else {
				DestHeight = WindowHeight;
				DestWidth = DestHeight * AspectX / AspectY;
			}
		}

		RECT rcSrc,rcDst,rcWindow;
		CalcSourceRect(&rcSrc);
#if 0
		// 座標値がマイナスになるとマルチディスプレイでおかしくなる?
		rcDst.left = (WindowWidth - DestWidth) / 2;
		rcDst.top = (WindowHeight - DestHeight) / 2,
		rcDst.right = rcDst.left + DestWidth;
		rcDst.bottom = rcDst.top + DestHeight;
#else
		if (WindowWidth < DestWidth) {
			rcDst.left = 0;
			rcDst.right = WindowWidth;
			rcSrc.left += (DestWidth - WindowWidth) * (rcSrc.right - rcSrc.left) / DestWidth / 2;
			rcSrc.right = m_VideoInfo.m_OrigWidth - rcSrc.left;
		} else {
			if (m_bNoMaskSideCut
					&& WindowWidth > DestWidth
					&& rcSrc.right - rcSrc.left < m_VideoInfo.m_OrigWidth) {
				int NewDestWidth=m_VideoInfo.m_OrigWidth*DestWidth/(rcSrc.right-rcSrc.left);
				if (NewDestWidth > WindowWidth)
					NewDestWidth=WindowWidth;
				int NewSrcWidth=(rcSrc.right-rcSrc.left)*NewDestWidth/DestWidth;
				rcSrc.left=(m_VideoInfo.m_OrigWidth-NewSrcWidth)/2;
				rcSrc.right=rcSrc.left+NewSrcWidth;
				TRACE(TEXT("Adjust %d x %d -> %d x %d [%d - %d (%d)]\n"),
					  DestWidth,DestHeight,NewDestWidth,DestHeight,
					  rcSrc.left,rcSrc.right,NewSrcWidth);
				DestWidth=NewDestWidth;
			}
			rcDst.left = (WindowWidth - DestWidth) / 2;
			rcDst.right = rcDst.left + DestWidth;
		}
		if (WindowHeight < DestHeight) {
			rcDst.top = 0;
			rcDst.bottom = WindowHeight;
			rcSrc.top += (DestHeight - WindowHeight) * (rcSrc.bottom - rcSrc.top) / DestHeight / 2;
			rcSrc.bottom = m_VideoInfo.m_OrigHeight - rcSrc.top;
		} else {
			rcDst.top = (WindowHeight - DestHeight) / 2,
			rcDst.bottom = rcDst.top + DestHeight;
		}
#endif
		rcWindow.left = 0;
		rcWindow.top = 0;
		rcWindow.right = WindowWidth;
		rcWindow.bottom = WindowHeight;
		return m_pVideoRenderer->SetVideoPosition(
			m_VideoInfo.m_OrigWidth, m_VideoInfo.m_OrigHeight, &rcSrc, &rcDst, &rcWindow);
	}
	return false;
}

const bool CMediaViewer::SetViewSize(const int x,const int y)
{
	CBlockLock Lock(&m_ResizeLock);

	// ウィンドウサイズを設定する
	if (x>0 && y>0) {
		m_wVideoWindowX = x;
		m_wVideoWindowY = y;
		return AdjustVideoPosition();
	}
	return false;
}

const bool CMediaViewer::SetVolume(const float fVolume)
{
	// オーディオボリュームをdBで設定する( -100.0(無音) < fVolume < 0(最大) )
	IBasicAudio *pBasicAudio;
	bool fOK=false;

	if (m_pFilterGraph) {
		if (SUCCEEDED(m_pFilterGraph->QueryInterface(
				IID_IBasicAudio, pointer_cast<void**>(&pBasicAudio)))) {
			long lVolume = (long)(fVolume * 100.0f);

			if (lVolume>=-10000 && lVolume<=0) {
					TRACE(TEXT("Volume Control = %d\n"),lVolume);
				if (SUCCEEDED(pBasicAudio->put_Volume(lVolume)))
					fOK=true;
			}
			pBasicAudio->Release();
		}
	}
	return fOK;
}

const bool CMediaViewer::GetVideoSize(WORD *pwWidth,WORD *pwHeight)
{
	if (m_bIgnoreDisplayExtension)
		return GetOriginalVideoSize(pwWidth, pwHeight);

	CBlockLock Lock(&m_ResizeLock);

	// ビデオのサイズを取得する
	if (m_VideoInfo.m_DisplayWidth > 0 && m_VideoInfo.m_DisplayHeight > 0) {
		if (pwWidth)
			*pwWidth = m_VideoInfo.m_DisplayWidth;
		if (pwHeight)
			*pwHeight = m_VideoInfo.m_DisplayHeight;
		return true;
	}
	return false;
}

const bool CMediaViewer::GetVideoAspectRatio(BYTE *pbyAspectRatioX,BYTE *pbyAspectRatioY)
{
	CBlockLock Lock(&m_ResizeLock);

	// ビデオのアスペクト比を取得する
	if (m_VideoInfo.m_AspectRatioX > 0 && m_VideoInfo.m_AspectRatioY > 0) {
		if (pbyAspectRatioX)
			*pbyAspectRatioX = m_VideoInfo.m_AspectRatioX;
		if (pbyAspectRatioY)
			*pbyAspectRatioY = m_VideoInfo.m_AspectRatioY;
		return true;
	}
	return false;
}

const BYTE CMediaViewer::GetAudioChannelNum()
{
	// オーディオの入力チャンネル数を取得する
	if (m_pAudioDecoder)
		return m_pAudioDecoder->GetCurrentChannelNum();
	return AUDIO_CHANNEL_INVALID;
}

const bool CMediaViewer::SetStereoMode(const int iMode)
{
	// ステレオ出力チャンネルの設定
	if (m_pAudioDecoder)
		return m_pAudioDecoder->SetStereoMode(iMode);
	return false;
}

const int CMediaViewer::GetStereoMode() const
{
	if (m_pAudioDecoder)
		return m_pAudioDecoder->GetStereoMode();
	return CAudioDecFilter::STEREOMODE_STEREO;
}

const bool CMediaViewer::GetVideoDecoderName(LPWSTR lpName,int iBufLen)
{
	// 選択されているビデオデコーダー名の取得
	if (lpName == NULL || iBufLen < 1)
		return false;
	if (m_pszVideoDecoderName == NULL) {
		if (iBufLen > 0)
			lpName[0] = '\0';
		return false;
	}
	::lstrcpynW(lpName, m_pszVideoDecoderName, iBufLen);
	return true;
}

const bool CMediaViewer::DisplayFilterProperty(PropertyFilter Filter, HWND hwndOwner)
{
	switch (Filter) {
	case PROPERTY_FILTER_VIDEODECODER:
		if (m_pVideoDecoderFilter)
			return DirectShowUtil::ShowPropertyPage(m_pVideoDecoderFilter,hwndOwner);
		break;
	case PROPERTY_FILTER_VIDEORENDERER:
		if (m_pVideoRenderer)
			return m_pVideoRenderer->ShowProperty(hwndOwner);
		break;
	case PROPERTY_FILTER_MPEG2DEMULTIPLEXER:
		if (m_pMp2DemuxFilter)
			return DirectShowUtil::ShowPropertyPage(m_pMp2DemuxFilter,hwndOwner);
		break;
	case PROPERTY_FILTER_AUDIOFILTER:
		if (m_pAudioFilter)
			return DirectShowUtil::ShowPropertyPage(m_pAudioFilter,hwndOwner);
		break;
	case PROPERTY_FILTER_AUDIORENDERER:
		if (m_pAudioRenderer)
			return DirectShowUtil::ShowPropertyPage(m_pAudioRenderer,hwndOwner);
		break;
	}
	return false;
}

const bool CMediaViewer::FilterHasProperty(PropertyFilter Filter)
{
	switch (Filter) {
	case PROPERTY_FILTER_VIDEODECODER:
		if (m_pVideoDecoderFilter)
			return DirectShowUtil::HasPropertyPage(m_pVideoDecoderFilter);
		break;
	case PROPERTY_FILTER_VIDEORENDERER:
		if (m_pVideoRenderer)
			return m_pVideoRenderer->HasProperty();
		break;
	case PROPERTY_FILTER_MPEG2DEMULTIPLEXER:
		if (m_pMp2DemuxFilter)
			return DirectShowUtil::HasPropertyPage(m_pMp2DemuxFilter);
		break;
	case PROPERTY_FILTER_AUDIOFILTER:
		if (m_pAudioFilter)
			return DirectShowUtil::HasPropertyPage(m_pAudioFilter);
		break;
	case PROPERTY_FILTER_AUDIORENDERER:
		if (m_pAudioRenderer)
			return DirectShowUtil::HasPropertyPage(m_pAudioRenderer);
		break;
	}
	return false;
}


#ifdef _DEBUG

HRESULT CMediaViewer::AddToRot(IUnknown *pUnkGraph, DWORD *pdwRegister) const
{
	// デバッグ用
	IMoniker * pMoniker;
	IRunningObjectTable *pROT;
	if(FAILED(::GetRunningObjectTable(0, &pROT)))return E_FAIL;

	WCHAR wsz[256];
	wsprintfW(wsz, L"FilterGraph %08p pid %08x", (DWORD_PTR)pUnkGraph, ::GetCurrentProcessId());

	HRESULT hr = ::CreateItemMoniker(L"!", wsz, &pMoniker);

	if(SUCCEEDED(hr)){
		hr = pROT->Register(0, pUnkGraph, pMoniker, pdwRegister);
		pMoniker->Release();
		}

	pROT->Release();

	return hr;
}

void CMediaViewer::RemoveFromRot(const DWORD dwRegister) const
{
	// デバッグ用
	IRunningObjectTable *pROT;

	if(SUCCEEDED(::GetRunningObjectTable(0, &pROT))){
		pROT->Revoke(dwRegister);
		pROT->Release();
		}
}

#endif	// _DEBUG


const bool CMediaViewer::GetVideoRendererName(LPTSTR pszName,int Length) const
{
	if (pszName == NULL || Length < 1)
		return false;

	LPCTSTR pszRenderer=CVideoRenderer::EnumRendererName((int)m_VideoRendererType);
	if (pszRenderer == NULL)
		return false;

	::lstrcpyn(pszName, pszRenderer, Length);
	return true;
}


const bool CMediaViewer::GetAudioRendererName(LPWSTR pszName,int Length) const
{
	if (pszName == NULL || Length < 1 || m_pszAudioRendererName==NULL)
		return false;
	::lstrcpyn(pszName, m_pszAudioRendererName, Length);
	return true;
}


const bool CMediaViewer::ForceAspectRatio(int AspectX,int AspectY)
{
	m_ForceAspectX=AspectX;
	m_ForceAspectY=AspectY;
	return true;
}


const bool CMediaViewer::GetForceAspectRatio(int *pAspectX,int *pAspectY) const
{
	if (pAspectX)
		*pAspectX=m_ForceAspectX;
	if (pAspectY)
		*pAspectY=m_ForceAspectY;
	return true;
}


const bool CMediaViewer::GetEffectiveAspectRatio(BYTE *pAspectX, BYTE *pAspectY)
{
	if (m_ForceAspectX != 0 && m_ForceAspectY != 0) {
		if (pAspectX)
			*pAspectX = m_ForceAspectX;
		if (pAspectY)
			*pAspectY = m_ForceAspectY;
		return true;
	}
	BYTE AspectX, AspectY;
	if (!GetVideoAspectRatio(&AspectX, &AspectY))
		return false;
	if (m_bIgnoreDisplayExtension
			&& (m_VideoInfo.m_DisplayWidth != m_VideoInfo.m_OrigWidth
				|| m_VideoInfo.m_DisplayHeight != m_VideoInfo.m_OrigHeight)) {
		if (m_VideoInfo.m_DisplayWidth == 0
				|| m_VideoInfo.m_DisplayHeight == 0)
			return false;
		AspectX = AspectX * 3 * m_VideoInfo.m_OrigWidth / m_VideoInfo.m_DisplayWidth;
		AspectY = AspectY * 3 * m_VideoInfo.m_OrigHeight / m_VideoInfo.m_DisplayHeight;
		if (AspectX % 4 == 0 && AspectY % 4 == 0) {
			AspectX /= 4;
			AspectY /= 4;
		}
	}
	if (pAspectX)
		*pAspectX = AspectX;
	if (pAspectY)
		*pAspectY = AspectY;
	return true;
}


const bool CMediaViewer::SetPanAndScan(int AspectX,int AspectY,const ClippingInfo *pClipping)
{
	if (m_ForceAspectX!=AspectX || m_ForceAspectY!=AspectY || pClipping!=NULL) {
		CBlockLock Lock(&m_ResizeLock);

		m_ForceAspectX=AspectX;
		m_ForceAspectY=AspectY;
		if (pClipping!=NULL)
			m_Clipping=*pClipping;
		else
			::ZeroMemory(&m_Clipping,sizeof(m_Clipping));
		AdjustVideoPosition();
	}
	return true;
}


const bool CMediaViewer::GetClippingInfo(ClippingInfo *pClipping) const
{
	if (pClipping==NULL)
		return false;
	*pClipping=m_Clipping;
	return true;
}


const bool CMediaViewer::SetViewStretchMode(ViewStretchMode Mode)
{
	if (m_ViewStretchMode!=Mode) {
		CBlockLock Lock(&m_ResizeLock);

		m_ViewStretchMode=Mode;
		return AdjustVideoPosition();
	}
	return true;
}


const bool CMediaViewer::SetNoMaskSideCut(bool bNoMask, bool bAdjust)
{
	if (m_bNoMaskSideCut != bNoMask) {
		CBlockLock Lock(&m_ResizeLock);

		m_bNoMaskSideCut = bNoMask;
		if (bAdjust)
			AdjustVideoPosition();
	}
	return true;
}


const bool CMediaViewer::SetIgnoreDisplayExtension(bool bIgnore)
{
	if (bIgnore != m_bIgnoreDisplayExtension) {
		CBlockLock Lock(&m_ResizeLock);

		m_bIgnoreDisplayExtension = bIgnore;
		if (m_VideoInfo.m_DisplayWidth != m_VideoInfo.m_OrigWidth
				|| m_VideoInfo.m_DisplayHeight != m_VideoInfo.m_OrigHeight)
			AdjustVideoPosition();
	}
	return true;
}


const bool CMediaViewer::GetOriginalVideoSize(WORD *pWidth,WORD *pHeight)
{
	CBlockLock Lock(&m_ResizeLock);

	if (m_VideoInfo.m_OrigWidth > 0 && m_VideoInfo.m_OrigHeight > 0) {
		if (pWidth)
			*pWidth = m_VideoInfo.m_OrigWidth;
		if (pHeight)
			*pHeight = m_VideoInfo.m_OrigHeight;
		return true;
	}
	return false;
}


const bool CMediaViewer::GetCroppedVideoSize(WORD *pWidth,WORD *pHeight)
{
	RECT rc;

	if (!GetSourceRect(&rc))
		return false;
	if (pWidth)
		*pWidth = (WORD)(rc.right - rc.left);
	if (pHeight)
		*pHeight = (WORD)(rc.bottom - rc.top);
	return true;
}


const bool CMediaViewer::GetSourceRect(RECT *pRect)
{
	CBlockLock Lock(&m_ResizeLock);

	if (!pRect)
		return false;
	return CalcSourceRect(pRect);
}


const bool CMediaViewer::CalcSourceRect(RECT *pRect)
{
	long SrcX,SrcY,SrcWidth,SrcHeight;

	if (m_VideoInfo.m_OrigWidth==0 || m_VideoInfo.m_OrigHeight==0)
		return false;
	if (m_Clipping.HorzFactor!=0) {
		SrcWidth=m_VideoInfo.m_OrigWidth-
			m_VideoInfo.m_OrigWidth*(m_Clipping.Left+m_Clipping.Right)/m_Clipping.HorzFactor;
		SrcX=m_VideoInfo.m_OrigWidth*m_Clipping.Left/m_Clipping.HorzFactor;
	} else if (m_bIgnoreDisplayExtension) {
		SrcWidth=m_VideoInfo.m_OrigWidth;
		SrcX=0;
	} else {
		SrcWidth=m_VideoInfo.m_DisplayWidth;
		SrcX=m_VideoInfo.m_PosX;
	}
	if (m_Clipping.VertFactor!=0) {
		SrcHeight=m_VideoInfo.m_OrigHeight-
			m_VideoInfo.m_OrigHeight*(m_Clipping.Top+m_Clipping.Bottom)/m_Clipping.VertFactor;
		SrcY=m_VideoInfo.m_OrigHeight*m_Clipping.Top/m_Clipping.VertFactor;
	} else if (m_bIgnoreDisplayExtension) {
		SrcHeight=m_VideoInfo.m_OrigHeight;
		SrcY=0;
	} else {
		SrcHeight=m_VideoInfo.m_DisplayHeight;
		SrcY=m_VideoInfo.m_PosY;
	}
	pRect->left=SrcX;
	pRect->top=SrcY;
	pRect->right=SrcX+SrcWidth;
	pRect->bottom=SrcY+SrcHeight;
	return true;
}


const bool CMediaViewer::GetDestRect(RECT *pRect)
{
	if (m_pVideoRenderer && pRect) {
		if (m_pVideoRenderer->GetDestPosition(pRect))
			return true;
	}
	return false;
}


const bool CMediaViewer::GetDestSize(WORD *pWidth,WORD *pHeight)
{
	RECT rc;

	if (!GetDestRect(&rc))
		return false;
	if (pWidth)
		*pWidth=(WORD)(rc.right-rc.left);
	if (pHeight)
		*pHeight=(WORD)(rc.bottom-rc.top);
	return true;
}


bool CMediaViewer::SetVisible(bool fVisible)
{
	if (m_pVideoRenderer)
		return m_pVideoRenderer->SetVisible(fVisible);
	return false;
}


const void CMediaViewer::HideCursor(bool bHide)
{
	if (m_pVideoRenderer)
		m_pVideoRenderer->ShowCursor(!bHide);
}


const bool CMediaViewer::GetCurrentImage(BYTE **ppDib)
{
	bool fOK=false;

	if (m_pVideoRenderer) {
		void *pBuffer;

		if (m_pVideoRenderer->GetCurrentImage(&pBuffer)) {
			fOK=true;
			*ppDib=static_cast<BYTE*>(pBuffer);
		}
	}
	return fOK;
}


bool CMediaViewer::SetSpdifOptions(const CAudioDecFilter::SpdifOptions *pOptions)
{
	if (m_pAudioDecoder)
		return m_pAudioDecoder->SetSpdifOptions(pOptions);
	return false;
}


bool CMediaViewer::GetSpdifOptions(CAudioDecFilter::SpdifOptions *pOptions) const
{
	if (m_pAudioDecoder)
		return m_pAudioDecoder->GetSpdifOptions(pOptions);
	return false;
}


bool CMediaViewer::IsSpdifPassthrough() const
{
	if (m_pAudioDecoder)
		return m_pAudioDecoder->IsSpdifPassthrough();
	return false;
}


bool CMediaViewer::SetAutoStereoMode(int Mode)
{
	if (m_pAudioDecoder)
		return m_pAudioDecoder->SetAutoStereoMode(Mode);
	return false;
}


bool CMediaViewer::SetDownMixSurround(bool bDownMix)
{
	if (m_pAudioDecoder)
		return m_pAudioDecoder->SetDownMixSurround(bDownMix);
	return false;
}


bool CMediaViewer::GetDownMixSurround() const
{
	if (m_pAudioDecoder)
		return m_pAudioDecoder->GetDownMixSurround();
	return false;
}


bool CMediaViewer::SetAudioGainControl(bool bGainControl, float Gain, float SurroundGain)
{
	if (m_pAudioDecoder==NULL)
		return false;
	return m_pAudioDecoder->SetGainControl(bGainControl, Gain, SurroundGain);
}


CVideoRenderer::RendererType CMediaViewer::GetVideoRendererType() const
{
	return m_VideoRendererType;
}


bool CMediaViewer::SetUseAudioRendererClock(bool bUse)
{
	m_bUseAudioRendererClock = bUse;
	return true;
}


bool CMediaViewer::SetAdjustAudioStreamTime(bool bAdjust)
{
	m_bAdjustAudioStreamTime = bAdjust;
	if (m_pAudioDecoder == NULL)
		return true;
	return m_pAudioDecoder->SetJitterCorrection(bAdjust);
}


bool CMediaViewer::SetAudioStreamCallback(CAudioDecFilter::StreamCallback pCallback, void *pParam)
{
	m_pAudioStreamCallback=pCallback;
	m_pAudioStreamCallbackParam=pParam;
	if (m_pAudioDecoder == NULL)
		return true;
	return m_pAudioDecoder->SetStreamCallback(pCallback, pParam);
}


bool CMediaViewer::SetAudioFilter(LPCWSTR pszFilterName)
{
	if (m_pszAudioFilterName) {
		delete [] m_pszAudioFilterName;
		m_pszAudioFilterName = NULL;
	}
	if (pszFilterName && pszFilterName[0] != '\0')
		m_pszAudioFilterName = StdUtil::strdup(pszFilterName);
	return true;
}


const bool CMediaViewer::RepaintVideo(HWND hwnd,HDC hdc)
{
	if (m_pVideoRenderer)
		return m_pVideoRenderer->RepaintVideo(hwnd,hdc);
	return false;
}


const bool CMediaViewer::DisplayModeChanged()
{
	if (m_pVideoRenderer)
		return m_pVideoRenderer->DisplayModeChanged();
	return false;
}


const bool CMediaViewer::DrawText(LPCTSTR pszText,int x,int y,
								  HFONT hfont,COLORREF crColor,int Opacity)
{
	IBaseFilter *pRenderer;
	int Width,Height;

	if (m_pVideoRenderer==NULL || !IsDrawTextSupported())
		return false;
	pRenderer=m_pVideoRenderer->GetRendererFilter();
	if (pRenderer==NULL)
		return false;
	if (m_pImageMixer==NULL) {
		m_pImageMixer=CImageMixer::CreateImageMixer(m_VideoRendererType,pRenderer);
		if (m_pImageMixer==NULL)
			return false;
	}
	if (!m_pImageMixer->GetMapSize(&Width,&Height))
		return false;
	m_ResizeLock.Lock();
	if (m_VideoInfo.m_OrigWidth==0 || m_VideoInfo.m_OrigHeight==0)
		return false;
	x=x*Width/m_VideoInfo.m_OrigWidth;
	y=y*Height/m_VideoInfo.m_OrigHeight;
	m_ResizeLock.Unlock();
	return m_pImageMixer->SetText(pszText,x,y,hfont,crColor,Opacity);
}


const bool CMediaViewer::IsDrawTextSupported() const
{
	return CImageMixer::IsSupported(m_VideoRendererType);
}


const bool CMediaViewer::ClearOSD()
{
	if (m_pVideoRenderer==NULL)
		return false;
	if (m_pImageMixer!=NULL)
		m_pImageMixer->Clear();
	return true;
}


bool CMediaViewer::EnablePTSSync(bool bEnable)
{
	TRACE(TEXT("CMediaViewer::EnablePTSSync(%s)\n"), bEnable ? TEXT("true") : TEXT("false"));
	if (m_pSrcFilter != NULL) {
		if (!m_pSrcFilter->EnableSync(bEnable))
			return false;
	}
	m_bEnablePTSSync = bEnable;
	return true;
}


bool CMediaViewer::IsPTSSyncEnabled() const
{
	/*
	if (m_pSrcFilter != NULL)
		return m_pSrcFilter->IsSyncEnabled();
	*/
	return m_bEnablePTSSync;
}


#ifdef BONTSENGINE_H264_SUPPORT
bool CMediaViewer::SetAdjustVideoSampleTime(bool bAdjust)
{
	TRACE(TEXT("CMediaViewer::SetAdjustSampleTime(%s)\n"), bAdjust ? TEXT("true") : TEXT("false"));
	m_bAdjustVideoSampleTime = bAdjust;
	if (m_pH264Parser != NULL)
		return m_pH264Parser->SetAdjustTime(bAdjust);
	return true;
}


bool CMediaViewer::SetAdjustFrameRate(bool bAdjust)
{
	TRACE(TEXT("CMediaViewer::SetAdjustFrameRate(%s)\n"), bAdjust ? TEXT("true") : TEXT("false"));
	m_bAdjustFrameRate = bAdjust;
	if (m_pH264Parser != NULL)
		return m_pH264Parser->SetAdjustFrameRate(bAdjust);
	return true;
}
#endif


DWORD CMediaViewer::GetAudioBitRate() const
{
	if (m_pAudioDecoder != NULL)
		return m_pAudioDecoder->GetBitRate();
	return 0;
}


DWORD CMediaViewer::GetVideoBitRate() const
{
#ifndef BONTSENGINE_H264_SUPPORT
	if (m_pMpeg2Parser != NULL)
		return m_pMpeg2Parser->GetBitRate();
#else
	if (m_pH264Parser != NULL)
		return m_pH264Parser->GetBitRate();
#endif
	return 0;
}
