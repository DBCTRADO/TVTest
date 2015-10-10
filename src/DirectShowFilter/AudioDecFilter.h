#pragma once

#include "../BonTsEngine/MediaData.h"
#include "../BonTsEngine/Simd.h"
#include "AudioDecoder.h"
#include "TsUtilClass.h"

// テンプレート名
#define AUDIODECFILTER_NAME L"Audio Decoder Filter"

// このフィルタのGUID {2AD583EC-1D57-4d0d-8991-487F2A0A0E8B}
DEFINE_GUID(CLSID_AudioDecFilter, 0x2ad583ec, 0x1d57, 0x4d0d, 0x89, 0x91, 0x48, 0x7f, 0x2a, 0xa, 0xe, 0x8b);

class __declspec(uuid("2AD583EC-1D57-4d0d-8991-487F2A0A0E8B")) CAudioDecFilter
	: public CTransformFilter
{
public:
	DECLARE_IUNKNOWN

	static IBaseFilter* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

// CTransformFilter
	HRESULT CheckInputType(const CMediaType* mtIn) override;
	HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut) override;
	HRESULT DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop) override;
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType) override;
	HRESULT StartStreaming() override;
	HRESULT StopStreaming() override;
	HRESULT BeginFlush() override;
	HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate) override;
	HRESULT Receive(IMediaSample *pSample) override;

// CAudioDecFilter
	enum {
		CHANNEL_DUALMONO	= 0x00,
		CHANNEL_INVALID		= 0xFF
	};

	enum DualMonoMode {
		DUALMONO_INVALID,
		DUALMONO_MAIN,
		DUALMONO_SUB,
		DUALMONO_BOTH
	};

	enum StereoMode {
		STEREOMODE_STEREO,
		STEREOMODE_LEFT,
		STEREOMODE_RIGHT
	};

	enum SpdifMode {
		SPDIF_MODE_DISABLED,
		SPDIF_MODE_PASSTHROUGH,
		SPDIF_MODE_AUTO
	};

	enum {
		SPDIF_CHANNELS_MONO		= 0x01U,
		SPDIF_CHANNELS_STEREO	= 0x02U,
		SPDIF_CHANNELS_DUALMONO	= 0x04U,
		SPDIF_CHANNELS_SURROUND	= 0x08U
	};

	struct SpdifOptions {
		SpdifMode Mode;
		UINT PassthroughChannels;

		SpdifOptions() : Mode(SPDIF_MODE_DISABLED), PassthroughChannels(0) {}
		SpdifOptions(SpdifMode mode,UINT channels = 0) : Mode(mode), PassthroughChannels(channels) {}
		bool operator==(const SpdifOptions &Op) const {
			return Mode == Op.Mode && PassthroughChannels == Op.PassthroughChannels;
		}
		bool operator!=(const SpdifOptions &Op) const { return !(*this==Op); }
	};

	struct SurroundMixingMatrix {
		double Matrix[6][6];
	};

	struct DownMixMatrix {
		double Matrix[2][6];
	};

	class IEventHandler
	{
	public:
		virtual ~IEventHandler() {}
		virtual void OnSpdifPassthroughError(HRESULT hr) {}
	};

	BYTE GetCurrentChannelNum() const;
	bool SetDualMonoMode(DualMonoMode Mode);
	DualMonoMode GetDualMonoMode() const { return m_DualMonoMode; }
	bool SetStereoMode(StereoMode Mode);
	StereoMode GetStereoMode() const { return m_StereoMode; }
	bool SetDownMixSurround(bool bDownMix);
	bool GetDownMixSurround() const { return m_bDownMixSurround; }
	bool SetSurroundMixingMatrix(const SurroundMixingMatrix *pMatrix);
	bool SetDownMixMatrix(const DownMixMatrix *pMatrix);
	bool SetGainControl(bool bGainControl, float Gain = 1.0f, float SurroundGain = 1.0f);
	bool GetGainControl(float *pGain = NULL, float *pSurroundGain = NULL) const;
	bool SetJitterCorrection(bool bEnable);
	bool GetJitterCorrection() const { return m_bJitterCorrection; }
	bool SetDelay(LONGLONG Delay);
	LONGLONG GetDelay() const { return m_Delay; }
	bool SetSpdifOptions(const SpdifOptions *pOptions);
	bool GetSpdifOptions(SpdifOptions *pOptions) const;
	bool IsSpdifPassthrough() const { return m_bPassthrough; }
	void SetEventHandler(IEventHandler *pEventHandler);

	typedef void (CALLBACK *StreamCallback)(short *pData, DWORD Samples, int Channels, void *pParam);
	bool SetStreamCallback(StreamCallback pCallback, void *pParam = NULL);

	DWORD GetBitRate() const;

private:
	CAudioDecFilter(LPUNKNOWN pUnk, HRESULT *phr);
	~CAudioDecFilter();

// CTransformFilter
	HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut) override;

// CAudioDecFilter
	struct FrameSampleInfo
	{
		CMediaData *pData;
		DWORD Samples;
		bool bMediaTypeChanged;
		CMediaType MediaType;
		long MediaBufferSize;
	};

	HRESULT OnFrame(const BYTE *pData, const DWORD Samples,
					const CAudioDecoder::AudioInfo &Info,
					FrameSampleInfo *pSampleInfo);
	HRESULT ProcessPcm(const BYTE *pData, const DWORD Samples,
					   const CAudioDecoder::AudioInfo &Info,
					   FrameSampleInfo *pSampleInfo);
	HRESULT ProcessSpdif(FrameSampleInfo *pSampleInfo);
	HRESULT ReconnectOutput(long BufferSize, const CMediaType &mt);
	void ResetSync();
	DWORD MonoToStereo(short *pDst, const short *pSrc, const DWORD Samples);
	DWORD DownMixStereo(short *pDst, const short *pSrc, const DWORD Samples);
	DWORD DownMixSurround(short *pDst, const short *pSrc, const DWORD Samples);
	DWORD MapSurroundChannels(short *pDst, const short *pSrc, const DWORD Samples);
	void GainControl(short *pBuffer, const DWORD Samples, const float Gain);
	void SelectDualMonoStereoMode();

	CAudioDecoder *m_pDecoder;
	mutable CCritSec m_cPropLock;
	CMediaType m_MediaType;
	CSimdMediaData m_OutData;
	BYTE m_CurChannelNum;
	bool m_bDualMono;

	DualMonoMode m_DualMonoMode;
	StereoMode m_StereoMode;
	bool m_bDownMixSurround;
	bool m_bEnableCustomMixingMatrix;
	SurroundMixingMatrix m_MixingMatrix;
	bool m_bEnableCustomDownMixMatrix;
	DownMixMatrix m_DownMixMatrix;

	bool m_bGainControl;
	float m_Gain;
	float m_SurroundGain;

	bool m_bJitterCorrection;
	LONGLONG m_Delay;
	LONGLONG m_DelayAdjustment;
	REFERENCE_TIME m_StartTime;
	LONGLONG m_SampleCount;
	bool m_bDiscontinuity;
	bool m_bInputDiscontinuity;

	SpdifOptions m_SpdifOptions;
	bool m_bPassthrough;
	bool m_bPassthroughError;

	IEventHandler *m_pEventHandler;

	StreamCallback m_pStreamCallback;
	void *m_pStreamCallbackParam;

	CBitRateCalculator m_BitRateCalculator;
};
