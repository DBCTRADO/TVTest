#include "StdAfx.h"
#include <initguid.h>
#include <mmreg.h>
#include "AudioDecFilter.h"
#include "AacDecoder.h"
#include "../Common/DebugDef.h"


// 周波数(48kHz)
static const int FREQUENCY = 48000;

// フレーム当たりのサンプル数(最大)
static const int SAMPLES_PER_FRAME = 1024;

// REFERENCE_TIMEの一秒
static const REFERENCE_TIME REFERENCE_TIME_SECOND = 10000000LL;

// サンプルのバッファサイズ
// (フレーム当たりのサンプル数 * 16ビット * 5.1ch)
static const long SAMPLE_BUFFER_SIZE = SAMPLES_PER_FRAME * 2 * 6;

// MediaSample用に確保するバッファ数
static const long NUM_SAMPLE_BUFFERS = 4;

// ジッタの許容上限
static const REFERENCE_TIME MAX_JITTER = REFERENCE_TIME_SECOND / 5LL;


inline short ClampSample16(int v)
{
	return v > SHORT_MAX ? SHORT_MAX : v < SHORT_MIN ? SHORT_MIN : v;
}


#if 0
static void OutputLog(LPCTSTR pszFormat, ...)
{
	va_list Args;
	TCHAR szText[1024];

	va_start(Args,pszFormat);
	int Length=::wvsprintf(szText,pszFormat,Args);
	va_end(Args);

	TRACE(szText);

	static bool fStopLog=false;
	static SIZE_T OutputSize=0;

	if (!fStopLog) {
		TCHAR szFileName[MAX_PATH];
		::GetModuleFileName(NULL,szFileName,MAX_PATH);
		::PathRenameExtension(szFileName,TEXT(".PassthroughTest.log"));
		HANDLE hFile=::CreateFile(szFileName,GENERIC_WRITE,FILE_SHARE_READ,NULL,
								  OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		if (hFile!=INVALID_HANDLE_VALUE) {
			DWORD FileSize=::GetFileSize(hFile,NULL);
			DWORD Wrote;

#ifdef UNICODE
			if (FileSize==0) {
				static const WORD BOM=0xFEFF;
				::WriteFile(hFile,&BOM,2,&Wrote,NULL);
			}
#endif
			::SetFilePointer(hFile,0,NULL,FILE_END);
			SYSTEMTIME st;
			TCHAR szTime[32];
			::GetLocalTime(&st);
			int TimeLength=::wsprintf(szTime,TEXT("%02d:%02d:%02d.%03d "),st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);
			::WriteFile(hFile,szTime,TimeLength*sizeof(TCHAR),&Wrote,NULL);
			::WriteFile(hFile,szText,Length*sizeof(TCHAR),&Wrote,NULL);
			OutputSize+=Length;
			if (OutputSize>=0x8000) {
				Length=::wsprintf(szText,TEXT("多くなりすぎたのでログの記録を停止します。\r\n\r\n"));
				::WriteFile(hFile,szText,Length*sizeof(TCHAR),&Wrote,NULL);
				fStopLog=true;
			}
			::FlushFileBuffers(hFile);
			::CloseHandle(hFile);
		}
	}
}
#else
#define OutputLog TRACE
#endif


CAudioDecFilter::CAudioDecFilter(LPUNKNOWN pUnk, HRESULT *phr)
	: CTransformFilter(AUDIODECFILTER_NAME, pUnk, CLSID_AudioDecFilter)
	, m_pDecoder(NULL)
	, m_CurChannelNum(0)
	, m_bDualMono(false)

	, m_DualMonoMode(DUALMONO_MAIN)
	, m_StereoMode(STEREOMODE_STEREO)
	, m_bDownMixSurround(true)
	, m_bEnableCustomMixingMatrix(false)
	, m_bEnableCustomDownMixMatrix(false)

	, m_bGainControl(false)
	, m_Gain(1.0f)
	, m_SurroundGain(1.0f)

	, m_bJitterCorrection(false)
	, m_Delay(0)
	, m_DelayAdjustment(0)
	, m_StartTime(-1)
	, m_SampleCount(0)
	, m_bDiscontinuity(true)
	, m_bInputDiscontinuity(true)

	, m_SpdifOptions(SPDIF_MODE_DISABLED, 0)
	, m_bPassthrough(false)
	, m_bPassthroughError(false)

	, m_pEventHandler(NULL)
	, m_pStreamCallback(NULL)
{
	TRACE(TEXT("CAudioDecFilter::CAudioDecFilter %p\n"), this);

	*phr = S_OK;

	// メディアタイプ設定
	m_MediaType.InitMediaType();
	m_MediaType.SetType(&MEDIATYPE_Audio);
	m_MediaType.SetSubtype(&MEDIASUBTYPE_PCM);
	m_MediaType.SetTemporalCompression(FALSE);
	m_MediaType.SetSampleSize(0);
	m_MediaType.SetFormatType(&FORMAT_WaveFormatEx);

	 // フォーマット構造体確保
#if 1
	// 2ch
	WAVEFORMATEX *pWaveInfo = pointer_cast<WAVEFORMATEX *>(m_MediaType.AllocFormatBuffer(sizeof(WAVEFORMATEX)));
	if (pWaveInfo == NULL) {
		*phr = E_OUTOFMEMORY;
		return;
	}
	// WAVEFORMATEX構造体設定(48kHz 16bit ステレオ固定)
	pWaveInfo->wFormatTag = WAVE_FORMAT_PCM;
	pWaveInfo->nChannels = 2;
	pWaveInfo->nSamplesPerSec = FREQUENCY;
	pWaveInfo->wBitsPerSample = 16;
	pWaveInfo->nBlockAlign = pWaveInfo->wBitsPerSample * pWaveInfo->nChannels / 8;
	pWaveInfo->nAvgBytesPerSec = pWaveInfo->nSamplesPerSec * pWaveInfo->nBlockAlign;
	pWaveInfo->cbSize = 0;
#else
	// 5.1ch
	WAVEFORMATEXTENSIBLE *pWaveInfo = pointer_cast<WAVEFORMATEXTENSIBLE *>(m_MediaType.AllocFormatBuffer(sizeof(WAVEFORMATEXTENSIBLE)));
	if (pWaveInfo == NULL) {
		*phr = E_OUTOFMEMORY;
		return;
	}
	// WAVEFORMATEXTENSIBLE構造体設定(48KHz 16bit 5.1ch固定)
	pWaveInfo->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	pWaveInfo->Format.nChannels = 6;
	pWaveInfo->Format.nSamplesPerSec = FREQUENCY;
	pWaveInfo->Format.wBitsPerSample = 16;
	pWaveInfo->Format.nBlockAlign = pWaveInfo->Format.wBitsPerSample * pWaveInfo->Format.nChannels / 8;
	pWaveInfo->Format.nAvgBytesPerSec = pWaveInfo->Format.nSamplesPerSec * pWaveInfo->Format.nBlockAlign;
	pWaveInfo->Format.cbSize  = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
	pWaveInfo->dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT |
							   SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY |
							   SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
	pWaveInfo->Samples.wValidBitsPerSample = 16;
	pWaveInfo->SubFormat = MEDIASUBTYPE_PCM;
#endif

	if (m_OutData.GetBuffer(SAMPLE_BUFFER_SIZE) < (DWORD)SAMPLE_BUFFER_SIZE)
		*phr = E_OUTOFMEMORY;
}


CAudioDecFilter::~CAudioDecFilter()
{
	TRACE(TEXT("CAudioDecFilter::~CAudioDecFilter\n"));

	if (m_pDecoder)
		delete m_pDecoder;
}


IBaseFilter* WINAPI CAudioDecFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	// インスタンスを作成する
	CAudioDecFilter *pNewFilter = new CAudioDecFilter(pUnk, phr);
	if (FAILED(*phr))
		goto OnError;

	IBaseFilter *pFilter;
	*phr = pNewFilter->QueryInterface(IID_IBaseFilter,
									  pointer_cast<void**>(&pFilter));
	if (FAILED(*phr))
		goto OnError;

	return pFilter;

OnError:
	delete pNewFilter;
	return NULL;
}


HRESULT CAudioDecFilter::CheckInputType(const CMediaType* mtIn)
{
	CheckPointer(mtIn, E_POINTER);

	// 何でもOK

	return S_OK;
}


HRESULT CAudioDecFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	CheckPointer(mtIn, E_POINTER);
	CheckPointer(mtOut, E_POINTER);

	if (*mtOut->Type() == MEDIATYPE_Audio) {
		if (*mtOut->Subtype() == MEDIASUBTYPE_PCM) {

			// GUID_NULLではデバッグアサートが発生するのでダミーを設定して回避
			CMediaType MediaType;
			MediaType.InitMediaType();
			MediaType.SetType(&MEDIATYPE_Stream);
			MediaType.SetSubtype(&MEDIASUBTYPE_None);

			m_pInput->SetMediaType(&MediaType);

			return S_OK;
		}
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}


HRESULT CAudioDecFilter::DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop)
{
	CheckPointer(pAllocator, E_POINTER);
	CheckPointer(pprop, E_POINTER);

	if (pprop->cBuffers < NUM_SAMPLE_BUFFERS)
		pprop->cBuffers = NUM_SAMPLE_BUFFERS;

	if (pprop->cbBuffer < SAMPLE_BUFFER_SIZE)
		pprop->cbBuffer = SAMPLE_BUFFER_SIZE;

	// アロケータプロパティを設定しなおす
	ALLOCATOR_PROPERTIES Actual;
	HRESULT hr = pAllocator->SetProperties(pprop, &Actual);
	if (FAILED(hr))
		return hr;

	// 要求を受け入れられたか判定
	if (Actual.cBuffers < pprop->cBuffers || Actual.cbBuffer < pprop->cbBuffer)
		return E_FAIL;

	return S_OK;
}


HRESULT CAudioDecFilter::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	CheckPointer(pMediaType, E_POINTER);
	CAutoLock AutoLock(m_pLock);

	if (iPosition < 0)
		return E_INVALIDARG;
	if (iPosition > 0)
		return VFW_S_NO_MORE_ITEMS;

	*pMediaType = m_MediaType;

	return S_OK;
}


HRESULT CAudioDecFilter::StartStreaming()
{
	CAutoLock AutoLock(&m_cPropLock);

	if (m_pDecoder) {
		m_pDecoder->Open();
	}

	ResetSync();

	m_bPassthroughError = false;
	m_BitRateCalculator.Initialize();

	return S_OK;
}


HRESULT CAudioDecFilter::StopStreaming()
{
	CAutoLock AutoLock(&m_cPropLock);

	if (m_pDecoder) {
		m_pDecoder->Close();
	}

	m_BitRateCalculator.Reset();

	return S_OK;
}


HRESULT CAudioDecFilter::BeginFlush()
{
	HRESULT hr = __super::BeginFlush();

	CAutoLock AutoLock(&m_cPropLock);

	if (m_pDecoder) {
		m_pDecoder->Reset();
	}

	ResetSync();

	return hr;
}


HRESULT CAudioDecFilter::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	HRESULT hr = __super::NewSegment(tStart, tStop, dRate);

	CAutoLock AutoLock(&m_cPropLock);

	ResetSync();

	return hr;
}


HRESULT CAudioDecFilter::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	// 入力データポインタを取得する
	const DWORD InSize = pIn->GetActualDataLength();
	BYTE *pInData = NULL;
	HRESULT hr = pIn->GetPointer(&pInData);
	if (FAILED(hr))
		return hr;

	{
		CAutoLock Lock(&m_cPropLock);

		/*
			複数の音声フォーマットに対応する場合、この辺りでフォーマットの判定をする
		*/
		if (!m_pDecoder) {
			m_pDecoder = new CAacDecoder();
			m_pDecoder->Open();
		}

		REFERENCE_TIME rtStart, rtEnd;
		hr = pIn->GetTime(&rtStart, &rtEnd);
		if (FAILED(hr))
			rtStart = -1;
		if (pIn->IsDiscontinuity() == S_OK) {
			m_bDiscontinuity = true;
			m_bInputDiscontinuity = true;
		} else if (hr == S_OK || hr == VFW_S_NO_STOP_TIME) {
			if (!m_bJitterCorrection) {
				m_StartTime = rtStart;
			} else if (m_StartTime >= 0
					&& _abs64(rtStart - m_StartTime) > MAX_JITTER) {
				TRACE(TEXT("Resync audio stream time (%lld -> %lld [%f])\n"),
					  m_StartTime, rtStart,
					  (double)(rtStart - m_StartTime) / (double)REFERENCE_TIME_SECOND);
				m_StartTime = rtStart;
			}
		}
		if (m_StartTime < 0 || m_bDiscontinuity) {
			TRACE(TEXT("Initialize audio stream time (%lld)\n"), rtStart);
			m_StartTime = rtStart;
		}

		m_BitRateCalculator.Update(InSize);
	}

	DWORD InDataPos = 0;
	FrameSampleInfo SampleInfo;
	SampleInfo.pData = &m_OutData;

	hr = S_OK;

	while (InDataPos < InSize) {
		{
			CAutoLock Lock(&m_cPropLock);

			CAudioDecoder::DecodeFrameInfo FrameInfo;
			const DWORD DataSize = InSize - InDataPos;
			DWORD DecodeSize = DataSize;
			if (!m_pDecoder->Decode(&pInData[InDataPos], &DecodeSize, &FrameInfo)) {
				if (DecodeSize < DataSize) {
					InDataPos += DecodeSize;
					continue;
				}
				break;
			}
			InDataPos += DecodeSize;

			if (FrameInfo.bDiscontinuity)
				m_bDiscontinuity = true;

			SampleInfo.bMediaTypeChanged = false;

			hr = OnFrame(FrameInfo.pData, FrameInfo.Samples, FrameInfo.Info,
						 &SampleInfo);
		}

		if (SUCCEEDED(hr)) {
			if (SampleInfo.bMediaTypeChanged) {
				hr = ReconnectOutput(SampleInfo.MediaBufferSize, SampleInfo.MediaType);
				if (FAILED(hr))
					break;
				OutputLog(TEXT("出力メディアタイプを更新します。\r\n"));
				hr = m_pOutput->SetMediaType(&SampleInfo.MediaType);
				if (FAILED(hr)) {
					OutputLog(TEXT("出力メディアタイプを設定できません。(%08x)\r\n"), hr);
					break;
				}
				m_MediaType = SampleInfo.MediaType;
				m_bDiscontinuity = true;
				m_bInputDiscontinuity = true;
			}

			IMediaSample *pOutSample = NULL;
			hr = m_pOutput->GetDeliveryBuffer(&pOutSample, NULL, NULL, 0);
			if (FAILED(hr)) {
				OutputLog(TEXT("出力メディアサンプルを取得できません。(%08x)\r\n"), hr);
				break;
			}

			if (SampleInfo.bMediaTypeChanged)
				pOutSample->SetMediaType(&m_MediaType);

			// 出力ポインタ取得
			BYTE *pOutBuff = NULL;
			hr = pOutSample->GetPointer(&pOutBuff);
			if (FAILED(hr)) {
				OutputLog(TEXT("出力サンプルのバッファを取得できません。(%08x)\r\n"), hr);
				pOutSample->Release();
				break;
			}

			::CopyMemory(pOutBuff, m_OutData.GetData(), m_OutData.GetSize());
			pOutSample->SetActualDataLength(m_OutData.GetSize());

			if (m_StartTime >= 0) {
				REFERENCE_TIME rtDuration, rtStart, rtEnd;
				rtDuration = REFERENCE_TIME_SECOND * (LONGLONG)SampleInfo.Samples / FREQUENCY;
				rtStart = m_StartTime;
				m_StartTime += rtDuration;
				// 音ずれ補正用時間シフト
				if (m_DelayAdjustment > 0) {
					// 最大2倍まで時間を遅らせる
					if (rtDuration >= m_DelayAdjustment) {
						rtDuration += m_DelayAdjustment;
						m_DelayAdjustment = 0;
					} else {
						m_DelayAdjustment -= rtDuration;
						rtDuration *= 2;
					}
				} else if (m_DelayAdjustment < 0) {
					// 最短1/2まで時間を早める
					if (rtDuration >= -m_DelayAdjustment * 2) {
						rtDuration += m_DelayAdjustment;
						m_DelayAdjustment = 0;
					} else {
						m_DelayAdjustment += rtDuration;
						rtDuration /= 2;
					}
				} else {
					rtStart += m_Delay;
				}
				rtEnd = rtStart + rtDuration;
				pOutSample->SetTime(&rtStart, &rtEnd);
			}
			pOutSample->SetMediaTime(NULL, NULL);
			pOutSample->SetPreroll(FALSE);
#if 0
			// Discontinuityを設定すると倍速再生がおかしくなる模様
			pOutSample->SetDiscontinuity(m_bDiscontinuity);
#else
			pOutSample->SetDiscontinuity(m_bInputDiscontinuity);
#endif
			m_bDiscontinuity = false;
			m_bInputDiscontinuity = false;
			pOutSample->SetSyncPoint(TRUE);

			hr = m_pOutput->Deliver(pOutSample);
#ifdef _DEBUG
			if (FAILED(hr)) {
				OutputLog(TEXT("サンプルを送信できません。(%08x)\r\n"), hr);
				if (m_bPassthrough && !m_bPassthroughError) {
					m_bPassthroughError = true;
					if (m_pEventHandler)
						m_pEventHandler->OnSpdifPassthroughError(hr);
				}
			}
#endif
			pOutSample->Release();
			if (FAILED(hr))
				break;
		}
	}

	return hr;
}


HRESULT CAudioDecFilter::Receive(IMediaSample *pSample)
{
	const AM_SAMPLE2_PROPERTIES *pProps = m_pInput->SampleProps();
	if (pProps->dwStreamId != AM_STREAM_MEDIA)
		return m_pOutput->Deliver(pSample);

	HRESULT hr;

	hr = Transform(pSample, NULL);
	if (SUCCEEDED(hr)) {
		hr = S_OK;
		m_bSampleSkipped = FALSE;
	}

	return hr;
}


BYTE CAudioDecFilter::GetCurrentChannelNum() const
{
	CAutoLock AutoLock(&m_cPropLock);

	if (m_CurChannelNum == 0)
		return CHANNEL_INVALID;
	if (m_bDualMono)
		return CHANNEL_DUALMONO;
	return m_CurChannelNum;
}


bool CAudioDecFilter::SetDualMonoMode(DualMonoMode Mode)
{
	CAutoLock AutoLock(&m_cPropLock);

	switch (Mode) {
	case DUALMONO_INVALID:
	case DUALMONO_MAIN:
	case DUALMONO_SUB:
	case DUALMONO_BOTH:
		TRACE(TEXT("CAudioDecFilter::SetDualMonoMode() : Mode %d\n"), Mode);
		m_DualMonoMode = Mode;
		if (m_bDualMono)
			SelectDualMonoStereoMode();
		return true;
	}

	return false;
}


bool CAudioDecFilter::SetStereoMode(StereoMode Mode)
{
	CAutoLock AutoLock(&m_cPropLock);

	switch (Mode) {
	case STEREOMODE_STEREO:
	case STEREOMODE_LEFT:
	case STEREOMODE_RIGHT:
		m_StereoMode = Mode;
		TRACE(TEXT("CAudioDecFilter::SetStereoMode() : Stereo mode %d\n"), Mode);
		return true;
	}

	return false;
}


bool CAudioDecFilter::SetDownMixSurround(bool bDownMix)
{
	CAutoLock AutoLock(&m_cPropLock);

	m_bDownMixSurround = bDownMix;
	return true;
}


bool CAudioDecFilter::SetSurroundMixingMatrix(const SurroundMixingMatrix *pMatrix)
{
	CAutoLock AutoLock(&m_cPropLock);

	if (pMatrix) {
		m_bEnableCustomMixingMatrix = true;
		m_MixingMatrix = *pMatrix;
	} else {
		m_bEnableCustomMixingMatrix = false;
	}

	return true;
}


bool CAudioDecFilter::SetDownMixMatrix(const DownMixMatrix *pMatrix)
{
	CAutoLock AutoLock(&m_cPropLock);

	if (pMatrix) {
		m_bEnableCustomDownMixMatrix = true;
		m_DownMixMatrix = *pMatrix;
	} else {
		m_bEnableCustomDownMixMatrix = false;
	}

	return true;
}


bool CAudioDecFilter::SetGainControl(bool bGainControl, float Gain, float SurroundGain)
{
	CAutoLock AutoLock(&m_cPropLock);

	m_bGainControl = bGainControl;
	m_Gain = Gain;
	m_SurroundGain = SurroundGain;
	return true;
}


bool CAudioDecFilter::GetGainControl(float *pGain, float *pSurroundGain) const
{
	CAutoLock AutoLock(&m_cPropLock);

	if (pGain)
		*pGain = m_Gain;
	if (pSurroundGain)
		*pSurroundGain = m_SurroundGain;
	return m_bGainControl;
}


bool CAudioDecFilter::SetSpdifOptions(const SpdifOptions *pOptions)
{
	if (!pOptions)
		return false;

	CAutoLock AutoLock(&m_cPropLock);

	m_SpdifOptions = *pOptions;

	return true;
}


bool CAudioDecFilter::GetSpdifOptions(SpdifOptions *pOptions) const
{
	if (!pOptions)
		return false;

	CAutoLock AutoLock(&m_cPropLock);

	*pOptions = m_SpdifOptions;

	return true;
}


bool CAudioDecFilter::SetJitterCorrection(bool bEnable)
{
	CAutoLock AutoLock(&m_cPropLock);

	m_bJitterCorrection = bEnable;

	m_StartTime = -1;
	m_SampleCount = 0;

	return true;
}


bool CAudioDecFilter::SetDelay(LONGLONG Delay)
{
	CAutoLock AutoLock(&m_cPropLock);

	TRACE(TEXT("CAudioDecFilter::SetDelay() : %lld\n"), Delay);

	m_DelayAdjustment += Delay - m_Delay;
	m_Delay = Delay;

	return true;
}


bool CAudioDecFilter::SetStreamCallback(StreamCallback pCallback, void *pParam)
{
	CAutoLock AutoLock(&m_cPropLock);

	m_pStreamCallback = pCallback;
	m_pStreamCallbackParam = pParam;

	return true;
}


void CAudioDecFilter::SetEventHandler(IEventHandler *pEventHandler)
{
	m_pEventHandler = pEventHandler;
}


DWORD CAudioDecFilter::GetBitRate() const
{
	CAutoLock AutoLock(&m_cPropLock);

	return m_BitRateCalculator.GetBitRate();
}


HRESULT CAudioDecFilter::OnFrame(const BYTE *pData, const DWORD Samples,
								 const CAudioDecoder::AudioInfo &Info,
								 FrameSampleInfo *pSampleInfo)
{
	if (Info.Channels != 1 && Info.Channels != 2 && Info.Channels != 6)
		return E_FAIL;

	const bool bDualMono = (Info.Channels == 2 && Info.bDualMono);

	bool bPassthrough = false;
	if (m_SpdifOptions.Mode == SPDIF_MODE_PASSTHROUGH) {
		bPassthrough = true;
	} else if (m_SpdifOptions.Mode == SPDIF_MODE_AUTO) {
		UINT ChannelFlag;
		if (bDualMono) {
			ChannelFlag = SPDIF_CHANNELS_DUALMONO;
		} else {
			switch (Info.Channels) {
			case 1: ChannelFlag = SPDIF_CHANNELS_MONO;		break;
			case 2: ChannelFlag = SPDIF_CHANNELS_STEREO;	break;
			case 6: ChannelFlag = SPDIF_CHANNELS_SURROUND;	break;
			}
		}
		if ((m_SpdifOptions.PassthroughChannels & ChannelFlag) != 0)
			bPassthrough = true;
	}

	if (m_bPassthrough != bPassthrough)
		m_bPassthroughError = false;
	m_bPassthrough = bPassthrough;

	if (bDualMono != m_bDualMono) {
		m_bDualMono = bDualMono;
		if (bDualMono) {
			SelectDualMonoStereoMode();
		} else {
			m_StereoMode = STEREOMODE_STEREO;
		}
	}

	m_CurChannelNum = Info.OrigChannels;

	HRESULT hr;

	if (m_bPassthrough) {
		hr = ProcessSpdif(pSampleInfo);
	} else {
		hr = ProcessPcm(pData, Samples, Info, pSampleInfo);
	}

	return hr;
}


HRESULT CAudioDecFilter::ProcessPcm(const BYTE *pData, const DWORD Samples,
									const CAudioDecoder::AudioInfo &Info,
									FrameSampleInfo *pSampleInfo)
{
	const bool bSurround = (Info.Channels == 6 && !m_bDownMixSurround);
	const int OutChannels = bSurround ? 6 : 2;

	// メディアタイプの更新
	bool bMediaTypeChanged = false;
	WAVEFORMATEX *pwfx = pointer_cast<WAVEFORMATEX*>(m_MediaType.Format());
	if (*m_MediaType.FormatType() != FORMAT_WaveFormatEx
			|| (!bSurround && pwfx->wFormatTag != WAVE_FORMAT_PCM)
			|| (bSurround && pwfx->wFormatTag != WAVE_FORMAT_EXTENSIBLE)) {
		CMediaType &mt = pSampleInfo->MediaType;
		mt.SetType(&MEDIATYPE_Audio);
		mt.SetSubtype(&MEDIASUBTYPE_PCM);
		mt.SetFormatType(&FORMAT_WaveFormatEx);

		pwfx = pointer_cast<WAVEFORMATEX*>(
			mt.AllocFormatBuffer(bSurround ? sizeof (WAVEFORMATEXTENSIBLE) : sizeof(WAVEFORMATEX)));
		if (pwfx == NULL)
			return E_OUTOFMEMORY;
		if (!bSurround) {
			pwfx->wFormatTag = WAVE_FORMAT_PCM;
			pwfx->nChannels = 2;
			pwfx->cbSize = 0;
		} else {
			WAVEFORMATEXTENSIBLE *pExtensible = pointer_cast<WAVEFORMATEXTENSIBLE*>(pwfx);
			pExtensible->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
			pExtensible->Format.nChannels = 6;
			pExtensible->Format.cbSize  = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
			pExtensible->dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT |
										 SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY |
										 SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
			pExtensible->Samples.wValidBitsPerSample = 16;
			pExtensible->SubFormat = MEDIASUBTYPE_PCM;
		}
		pwfx->nSamplesPerSec = FREQUENCY;
		pwfx->wBitsPerSample = 16;
		pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
		pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
		mt.SetSampleSize(pwfx->nBlockAlign);

		pSampleInfo->bMediaTypeChanged = true;
		pSampleInfo->MediaBufferSize = SAMPLES_PER_FRAME * pwfx->nBlockAlign;
	}

	const DWORD BuffSize = Samples * OutChannels * sizeof(short);
	if (pSampleInfo->pData->SetSize(BuffSize) < BuffSize)
		return E_OUTOFMEMORY;
	BYTE *pOutBuff = pSampleInfo->pData->GetData();

	if (pData != NULL) {
		DWORD OutSize;

		// ダウンミックス
		switch (Info.Channels) {
		case 1:
			OutSize = MonoToStereo(pointer_cast<short *>(pOutBuff),
								   pointer_cast<const short *>(pData),
								   Samples);
			break;
		case 2:
			OutSize = DownMixStereo(pointer_cast<short *>(pOutBuff),
									pointer_cast<const short *>(pData),
									Samples);
			break;
		case 6:
			if (bSurround) {
				OutSize = MapSurroundChannels(pointer_cast<short *>(pOutBuff),
											  pointer_cast<const short *>(pData),
											  Samples);
			} else {
				OutSize = DownMixSurround(pointer_cast<short *>(pOutBuff),
										  pointer_cast<const short *>(pData),
										  Samples);
			}
			break;
		}

		if (m_bGainControl && (Info.Channels < 6 || bSurround)) {
			GainControl(pointer_cast<short *>(pOutBuff), OutSize / sizeof(short),
						bSurround ? m_SurroundGain : m_Gain);
		}
	} else {
		::ZeroMemory(pOutBuff, BuffSize);
	}

	if (m_pStreamCallback) {
		m_pStreamCallback(pointer_cast<short *>(pOutBuff), Samples, OutChannels,
						  m_pStreamCallbackParam);
	}

	pSampleInfo->Samples = Samples;

	return S_OK;
}


HRESULT CAudioDecFilter::ProcessSpdif(FrameSampleInfo *pSampleInfo)
{
	static const int PREAMBLE_SIZE = sizeof(WORD) * 4;

	CAudioDecoder::SpdifFrameInfo FrameInfo;
	if (!m_pDecoder->GetSpdifFrameInfo(&FrameInfo))
		return E_FAIL;

	const int FrameSize = FrameInfo.FrameSize;
	const int DataBurstSize = PREAMBLE_SIZE + FrameSize;
	const int PacketSize = FrameInfo.SamplesPerFrame * 4;
	if (DataBurstSize > PacketSize) {
		OutputLog(TEXT("S/PDIFビットレートが不正です。(Frame size %d / Data-burst size %d / Packet size %d)\r\n"),
				  FrameSize, DataBurstSize, PacketSize);
		return E_FAIL;
	}

#ifdef _DEBUG
	static bool bFirst=true;
	if (bFirst) {
		OutputLog(TEXT("S/PDIF出力開始(Frame size %d / Data-burst size %d / Packet size %d)\r\n"),
				  FrameSize, DataBurstSize, PacketSize);
		bFirst=false;
	}
#endif

	WAVEFORMATEX *pwfx = pointer_cast<WAVEFORMATEX*>(m_MediaType.Format());
	if (*m_MediaType.FormatType() != FORMAT_WaveFormatEx
			|| pwfx->wFormatTag != WAVE_FORMAT_DOLBY_AC3_SPDIF) {
		CMediaType &mt = pSampleInfo->MediaType;
		mt.SetType(&MEDIATYPE_Audio);
		mt.SetSubtype(&MEDIASUBTYPE_PCM);
		mt.SetFormatType(&FORMAT_WaveFormatEx);

		pwfx = pointer_cast<WAVEFORMATEX*>(mt.AllocFormatBuffer(sizeof(WAVEFORMATEX)));
		if (pwfx == NULL)
			return E_OUTOFMEMORY;
		pwfx->wFormatTag = WAVE_FORMAT_DOLBY_AC3_SPDIF;
		pwfx->nChannels = 2;
		pwfx->nSamplesPerSec = FREQUENCY;
		pwfx->wBitsPerSample = 16;
		pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
		pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
		pwfx->cbSize = 0;
		/*
		// Windows 7 では WAVEFORMATEXTENSIBLE_IEC61937 を使った方がいい?
		WAVEFORMATEXTENSIBLE_IEC61937 *pwfx;
		...
		pwfx->FormatExt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		...
		pwfx->FormatExt.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE_IEC61937) - sizeof(WAVEFORMATEX);
		pwfx->FormatExt.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT |
										SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY |
										SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
		pwfx->FormatExt.Samples.wValidBitsPerSample = 16;
		pwfx->FormatExt.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_AAC;
		pwfx->dwEncodedSamplesPerSec = FREQUENCY;
		pwfx->dwEncodedChannelCount = 6;
		pwfx->dwAverageBytesPerSec = 0;
		*/
		mt.SetSampleSize(pwfx->nBlockAlign);

		pSampleInfo->bMediaTypeChanged = true;
		pSampleInfo->MediaBufferSize = PacketSize;
	}

	if (pSampleInfo->pData->SetSize(PacketSize) < (DWORD)PacketSize)
		return E_OUTOFMEMORY;
	BYTE *pOutBuff = pSampleInfo->pData->GetData();
	WORD *pWordData = pointer_cast<WORD*>(pOutBuff);
	// Burst-preamble
	pWordData[0] = 0xF872;						// Pa(Sync word 1)
	pWordData[1] = 0x4E1F;						// Pb(Sync word 2)
	pWordData[2] = FrameInfo.Pc;				// Pc(Burst-info)
//	pWordData[3] = ((FrameSize + 1) & ~1) * 8;	// Pd(Length-code)
	pWordData[3] = FrameSize * 8;				// Pd(Length-code)
	// Burst-payload
	int PayloadSize = m_pDecoder->GetSpdifBurstPayload(
			&pOutBuff[PREAMBLE_SIZE],
			pSampleInfo->pData->GetBufferSize() - PREAMBLE_SIZE);
	if ((PayloadSize < 1) || (PREAMBLE_SIZE + PayloadSize > PacketSize)) {
		OutputLog(TEXT("S/PDIF Burst-payload サイズが不正です。(Packet size %d / Payload size %d)\r\n"),
				  PacketSize, PayloadSize);
		return E_FAIL;
	}
	// Stuffing
	PayloadSize += PREAMBLE_SIZE;
	if (PayloadSize < PacketSize)
		::ZeroMemory(&pOutBuff[PayloadSize], PacketSize - PayloadSize);

	pSampleInfo->Samples = FrameInfo.SamplesPerFrame;

	return S_OK;
}


HRESULT CAudioDecFilter::ReconnectOutput(long BufferSize, const CMediaType &mt)
{
	HRESULT hr;

	IPin *pPin = m_pOutput->GetConnected();
	if (pPin == NULL)
		return E_POINTER;

	IMemInputPin *pMemInputPin = NULL;
	hr = pPin->QueryInterface(IID_IMemInputPin, pointer_cast<void**>(&pMemInputPin));
	if (FAILED(hr)) {
		OutputLog(TEXT("IMemInputPinインターフェースが取得できません。(%08x)\r\n"), hr);
	} else {
		IMemAllocator *pAllocator = NULL;
		hr = pMemInputPin->GetAllocator(&pAllocator);
		if (FAILED(hr)) {
			OutputLog(TEXT("IMemAllocatorインターフェースが取得できません。(%08x)\r\n"), hr);
		} else {
			ALLOCATOR_PROPERTIES Props;
			hr = pAllocator->GetProperties(&Props);
			if (FAILED(hr)) {
				OutputLog(TEXT("IMemAllocatorのプロパティが取得できません。(%08x)\r\n"), hr);
			} else {
				if (mt != m_pOutput->CurrentMediaType()
						|| Props.cBuffers < NUM_SAMPLE_BUFFERS
						|| Props.cbBuffer < BufferSize) {
					hr = S_OK;
					if (Props.cBuffers < NUM_SAMPLE_BUFFERS
							|| Props.cbBuffer < BufferSize) {
						ALLOCATOR_PROPERTIES ActualProps;

						Props.cBuffers = NUM_SAMPLE_BUFFERS;
						Props.cbBuffer = BufferSize * 3 / 2;
						OutputLog(TEXT("バッファサイズを設定します。(%ld bytes)\r\n"), Props.cbBuffer);
						if (SUCCEEDED(hr = m_pOutput->DeliverBeginFlush())
								&& SUCCEEDED(hr = m_pOutput->DeliverEndFlush())
								&& SUCCEEDED(hr = pAllocator->Decommit())
								&& SUCCEEDED(hr = pAllocator->SetProperties(&Props, &ActualProps))
								&& SUCCEEDED(hr = pAllocator->Commit())) {
							if (ActualProps.cBuffers < Props.cBuffers
									|| ActualProps.cbBuffer < BufferSize) {
								OutputLog(TEXT("バッファサイズの要求が受け付けられません。(%ld / %ld)\r\n"),
										  ActualProps.cbBuffer, Props.cbBuffer);
								hr = E_FAIL;
								NotifyEvent(EC_ERRORABORT, hr, 0);
							} else {
								OutputLog(TEXT("ピンの再接続成功\r\n"));
								hr = S_OK;
							}
						} else {
							OutputLog(TEXT("ピンの再接続ができません。(%08x)\r\n"), hr);
						}
					}
				} else {
					hr = S_FALSE;
				}
			}

			pAllocator->Release();
		}

		pMemInputPin->Release();
	}

	return hr;
}


void CAudioDecFilter::ResetSync()
{
	m_DelayAdjustment = 0;
	m_StartTime = -1;
	m_SampleCount = 0;
	m_bDiscontinuity = true;
	m_bInputDiscontinuity = true;
}


DWORD CAudioDecFilter::MonoToStereo(short *pDst, const short *pSrc, const DWORD Samples)
{
	// 1ch → 2ch 二重化
	const short *p = pSrc, *pEnd = pSrc + Samples;
	short *q = pDst;

	while (p < pEnd) {
		short Value = *p++;
		*q++ = Value;	// L
		*q++ = Value;	// R
	}

	// バッファサイズを返す
	return Samples * (sizeof(short) * 2);
}


DWORD CAudioDecFilter::DownMixStereo(short *pDst, const short *pSrc, const DWORD Samples)
{
	if (m_StereoMode == STEREOMODE_STEREO) {
		// 2ch → 2ch スルー
		::CopyMemory(pDst, pSrc, Samples * (sizeof(short) * 2));
	} else {
		const short *p = pSrc, *pEnd = pSrc + Samples * 2;
		short *q = pDst;

		if (m_StereoMode == STEREOMODE_LEFT) {
			// 左のみ
			while (p < pEnd) {
				short Value = *p;
				*q++ = Value;	// L
				*q++ = Value;	// R
				p += 2;
			}
		} else {
			// 右のみ
			while (p < pEnd) {
				short Value = p[1];
				*q++ = Value;	// L
				*q++ = Value;	// R
				p += 2;
			}
		}
	}

	// バッファサイズを返す
	return Samples * (sizeof(short) * 2);
}


DWORD CAudioDecFilter::DownMixSurround(short *pDst, const short *pSrc, const DWORD Samples)
{
	// 5.1ch → 2ch ダウンミックス

	const double Level = m_bGainControl ? m_SurroundGain : 1.0;
	int ChannelMap[6];

	if (!m_pDecoder->GetChannelMap(6, ChannelMap)) {
		for (int i = 0; i < 6; i++)
			ChannelMap[i] = i;
	}

	if (m_bEnableCustomDownMixMatrix) {
		// カスタムマトリックスを使用
		for (DWORD Pos = 0; Pos < Samples; Pos++) {
			double Data[6];

			for (int i = 0; i < 6; i++)
				Data[i] = (double)pSrc[Pos * 6 + ChannelMap[i]];

			for (int i = 0; i < 2; i++) {
				int Value = (int)((
					Data[0] * m_DownMixMatrix.Matrix[i][0] +
					Data[1] * m_DownMixMatrix.Matrix[i][1] +
					Data[2] * m_DownMixMatrix.Matrix[i][2] +
					Data[3] * m_DownMixMatrix.Matrix[i][3] +
					Data[4] * m_DownMixMatrix.Matrix[i][4] +
					Data[5] * m_DownMixMatrix.Matrix[i][5]
					) * Level);
				pDst[Pos * 2 + i] = ClampSample16(Value);
			}
		}
	} else {
		// デフォルトの係数を使用
		CAudioDecoder::DownmixInfo Info;
		m_pDecoder->GetDownmixInfo(&Info);

		for (DWORD Pos = 0; Pos < Samples; Pos++) {
			int Left = (int)((
				(double)pSrc[Pos * 6 + ChannelMap[CAudioDecoder::CHANNEL_6_FL ]] * Info.Front +
				(double)pSrc[Pos * 6 + ChannelMap[CAudioDecoder::CHANNEL_6_BL ]] * Info.Rear +
				(double)pSrc[Pos * 6 + ChannelMap[CAudioDecoder::CHANNEL_6_FC ]] * Info.Center +
				(double)pSrc[Pos * 6 + ChannelMap[CAudioDecoder::CHANNEL_6_LFE]] * Info.LFE
				) * Level);

			int Right = (int)((
				(double)pSrc[Pos * 6 + ChannelMap[CAudioDecoder::CHANNEL_6_FR ]] * Info.Front +
				(double)pSrc[Pos * 6 + ChannelMap[CAudioDecoder::CHANNEL_6_BR ]] * Info.Rear +
				(double)pSrc[Pos * 6 + ChannelMap[CAudioDecoder::CHANNEL_6_FC ]] * Info.Center +
				(double)pSrc[Pos * 6 + ChannelMap[CAudioDecoder::CHANNEL_6_LFE]] * Info.LFE
				) * Level);

			pDst[Pos * 2 + 0] = ClampSample16(Left);
			pDst[Pos * 2 + 1] = ClampSample16(Right);
		}
	}

	// バッファサイズを返す
	return Samples * (sizeof(short) * 2);
}


DWORD CAudioDecFilter::MapSurroundChannels(short *pDst, const short *pSrc, const DWORD Samples)
{
	if (m_bEnableCustomMixingMatrix) {
		// カスタムマトリックスを使用
		int ChannelMap[6];

		if (!m_pDecoder->GetChannelMap(6, ChannelMap)) {
			for (int i = 0; i < 6; i++)
				ChannelMap[i] = i;
		}

		for (DWORD i = 0 ; i < Samples ; i++) {
			double Data[6];

			for (int j = 0; j < 6; j++)
				Data[j] = (double)pSrc[i * 6 + ChannelMap[j]];

			for (int j = 0; j < 6; j++) {
				int Value = (int)(
					Data[0] * m_MixingMatrix.Matrix[j][0] +
					Data[1] * m_MixingMatrix.Matrix[j][1] +
					Data[2] * m_MixingMatrix.Matrix[j][2] +
					Data[3] * m_MixingMatrix.Matrix[j][3] +
					Data[4] * m_MixingMatrix.Matrix[j][4] +
					Data[5] * m_MixingMatrix.Matrix[j][5]);
				pDst[i * 6 + j] = ClampSample16(Value);
			}
		}
	} else {
		// デフォルトの割り当てを使用
		int ChannelMap[6];

		if (m_pDecoder->GetChannelMap(6, ChannelMap)) {
			for (DWORD i = 0 ; i < Samples ; i++) {
				pDst[i * 6 + 0] = pSrc[i * 6 + ChannelMap[CAudioDecoder::CHANNEL_6_FL]];
				pDst[i * 6 + 1] = pSrc[i * 6 + ChannelMap[CAudioDecoder::CHANNEL_6_FR]];
				pDst[i * 6 + 2] = pSrc[i * 6 + ChannelMap[CAudioDecoder::CHANNEL_6_FC]];
				pDst[i * 6 + 3] = pSrc[i * 6 + ChannelMap[CAudioDecoder::CHANNEL_6_LFE]];
				pDst[i * 6 + 4] = pSrc[i * 6 + ChannelMap[CAudioDecoder::CHANNEL_6_BL]];
				pDst[i * 6 + 5] = pSrc[i * 6 + ChannelMap[CAudioDecoder::CHANNEL_6_BR]];
			}
		} else {
			::CopyMemory(pDst, pSrc, Samples * 6 * sizeof(short));
		}
	}

	// バッファサイズを返す
	return Samples * (sizeof(short) * 6);
}


void CAudioDecFilter::GainControl(short *pBuffer, const DWORD Samples, const float Gain)
{
	static const int Factor = 0x1000;
	const int Level = (int)(Gain * (float)Factor);

	if (Level != Factor) {
		short *p, *pEnd;

		p = pBuffer;
		pEnd= p + Samples;
		while (p < pEnd) {
			int Value = ((int)*p * Level) / Factor;
			*p++ = ClampSample16(Value);
		}
	}
}


void CAudioDecFilter::SelectDualMonoStereoMode()
{
	switch (m_DualMonoMode) {
	case DUALMONO_MAIN:	m_StereoMode = STEREOMODE_LEFT;		break;
	case DUALMONO_SUB:	m_StereoMode = STEREOMODE_RIGHT;	break;
	case DUALMONO_BOTH:	m_StereoMode = STEREOMODE_STEREO;	break;
	}
}
