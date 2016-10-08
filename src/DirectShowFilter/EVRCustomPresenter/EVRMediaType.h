#pragma once


#include <mferror.h>


inline MFOffset MakeOffset(float v)
{
	MFOffset offset;
	offset.value = short(v);
	offset.fract = WORD(65536 * (v - offset.value));
	return offset;
}

inline MFVideoArea MakeArea(float x, float y, DWORD width, DWORD height)
{
	MFVideoArea area;
	area.OffsetX = MakeOffset(x);
	area.OffsetY = MakeOffset(y);
	area.Area.cx = width;
	area.Area.cy = height;
	return area;
}

HRESULT GetFrameRate(IMFMediaType *pType, MFRatio *pRatio);
HRESULT GetVideoDisplayArea(IMFMediaType *pType, MFVideoArea *pArea);
HRESULT GetDefaultStride(IMFMediaType *pType, LONG *pStride);


class MediaType
{
protected:
	IMFMediaType *m_pType;

protected:
	bool IsValid()
	{
		return m_pType != nullptr;
	}

	IMFMediaType* GetMediaType()
	{
		_ASSERT(IsValid());
		return m_pType;
	}

public:
	MediaType(IMFMediaType *pType = nullptr)
		: m_pType(pType)
	{
		if (m_pType != nullptr) {
			m_pType->AddRef();
		}
	}

	MediaType(const MediaType &mt)
		: m_pType(mt.m_pType)
	{
		if (m_pType != nullptr) {
			m_pType->AddRef();
		}
	}

	virtual ~MediaType()
	{
		SafeRelease(m_pType);
	}

	MediaType &operator=(const MediaType &mt)
	{
		if (!AreComObjectsEqual(m_pType, mt.m_pType)) {
			if (m_pType != nullptr) {
				m_pType->Release();
			}

			m_pType = mt.m_pType;
			if (m_pType != nullptr) {
				m_pType->AddRef();
			}
		}

		return *this;
	}

	IMFMediaType **operator&()
	{
		return &m_pType;
	}

	operator IMFMediaType*()
	{
		return m_pType;
	}

	virtual HRESULT CreateEmptyType()
	{
		SafeRelease(m_pType);

		return ::MFCreateMediaType(&m_pType);
	}

	IMFMediaType *Detach()
	{
		IMFMediaType *p = m_pType;
		m_pType = nullptr;
		return p;
	}

	HRESULT GetMajorType(GUID *pGuid)
	{
		return GetMediaType()->GetMajorType(pGuid);
	}

	HRESULT IsCompressedFormat(BOOL *pbCompressed)
	{
		return GetMediaType()->IsCompressedFormat(pbCompressed);
	}

	HRESULT IsEqual(IMFMediaType *pType, DWORD *pdwFlags)
	{
		return GetMediaType()->IsEqual(pType, pdwFlags);
	}

	HRESULT GetRepresentation(GUID guidRepresentation, LPVOID *ppvRepresentation)
	{
		return GetMediaType()->GetRepresentation(guidRepresentation, ppvRepresentation);
	}

	HRESULT FreeRepresentation(GUID guidRepresentation, LPVOID pvRepresentation) 
	{
		return GetMediaType()->FreeRepresentation(guidRepresentation, pvRepresentation);
	}

	HRESULT CopyFrom(MediaType *pType)
	{
		if (pType == nullptr) {
			return E_POINTER;
		}
		if (!pType->IsValid()) {
			return E_UNEXPECTED;
		}
		return CopyFrom(pType->m_pType);
	}

	HRESULT CopyFrom(IMFMediaType *pType)
	{
		if (pType == nullptr) {
			return E_POINTER;
		}

		if (m_pType == nullptr) {
			HRESULT hr = CreateEmptyType();
			if (FAILED(hr)) {
				return hr;
			}
		}

		return pType->CopyAllItems(m_pType);
	}

	HRESULT GetMediaType(IMFMediaType **ppType)
	{
		if (ppType == nullptr) {
			return E_POINTER;
		}
		_ASSERT(IsValid());
		*ppType = m_pType;
		(*ppType)->AddRef();
		return S_OK;
	}

	HRESULT SetMajorType(GUID guid)
	{
		return GetMediaType()->SetGUID(MF_MT_MAJOR_TYPE, guid);
	}

	HRESULT GetSubType(GUID* pGuid)
	{
		if (pGuid == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetGUID(MF_MT_SUBTYPE, pGuid);
	}

	HRESULT SetSubType(GUID guid)
	{
		return GetMediaType()->SetGUID(MF_MT_SUBTYPE, guid);
	}

	HRESULT GetFourCC(DWORD *pFourCC)
	{
		if (pFourCC == nullptr) {
			return E_POINTER;
		}

		_ASSERT(IsValid());

		GUID guidSubType = GUID_NULL;

		HRESULT hr = GetSubType(&guidSubType);
		if (SUCCEEDED(hr)) {
			*pFourCC = guidSubType.Data1;
		}
		return hr;
	}

	HRESULT GetAllSamplesIndependent(BOOL *pbIndependent)
	{
		if (pbIndependent == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, (UINT32*)pbIndependent);
	}

	HRESULT SetAllSamplesIndependent(BOOL bIndependent)
	{
		return GetMediaType()->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, bIndependent);
	}

	HRESULT GetFixedSizeSamples(BOOL *pbFixed)
	{
		if (pbFixed == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_FIXED_SIZE_SAMPLES, (UINT32*)pbFixed);
	}

	HRESULT SetFixedSizeSamples(BOOL bFixed)
	{
		return GetMediaType()->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES, bFixed);
	}

	HRESULT GetSampleSize(UINT32 *pSize)
	{
		if (pSize == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_SAMPLE_SIZE, pSize);
	}

	HRESULT SetSampleSize(UINT32 Size)
	{
		return GetMediaType()->SetUINT32(MF_MT_SAMPLE_SIZE, Size);
	}

	HRESULT Unwrap(IMFMediaType **ppOriginal)
	{
		if (ppOriginal == nullptr) {
			return E_POINTER;
		}
		return ::MFUnwrapMediaType(GetMediaType(), ppOriginal);
	}

	BOOL AllSamplesIndependent()
	{
		return (BOOL)::MFGetAttributeUINT32(GetMediaType(), MF_MT_ALL_SAMPLES_INDEPENDENT, FALSE);
	}

	BOOL FixedSizeSamples()
	{
		return (BOOL)::MFGetAttributeUINT32(GetMediaType(), MF_MT_FIXED_SIZE_SAMPLES, FALSE);
	}

	UINT32 SampleSize()
	{	
		return ::MFGetAttributeUINT32(GetMediaType(), MF_MT_SAMPLE_SIZE, 0);
	}
};


class VideoType : public MediaType
{
	friend class MediaType; 

public:

	VideoType(IMFMediaType *pType = nullptr)
		: MediaType(pType)
	{
	}

	virtual HRESULT CreateEmptyType()
	{
		SafeRelease(m_pType);

		HRESULT hr = MediaType::CreateEmptyType();
		if (SUCCEEDED(hr)) {
			hr = SetMajorType(MFMediaType_Video);
		}
		return hr;
	}

	HRESULT GetInterlaceMode(MFVideoInterlaceMode *pmode)
	{
		if (pmode == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_INTERLACE_MODE, (UINT32*)pmode);
	}

	HRESULT SetInterlaceMode(MFVideoInterlaceMode mode)
	{
		return GetMediaType()->SetUINT32(MF_MT_INTERLACE_MODE, (UINT32)mode);
	}

	HRESULT GetDefaultStride(LONG *pStride)
	{
		return ::GetDefaultStride(GetMediaType(), pStride);
	}

	HRESULT SetDefaultStride(LONG Stride)
	{
		return GetMediaType()->SetUINT32(MF_MT_DEFAULT_STRIDE, Stride);
	}

	HRESULT GetFrameDimensions(UINT32 *pWidthInPixels, UINT32 *pHeightInPixels)
	{
		return ::MFGetAttributeSize(GetMediaType(), MF_MT_FRAME_SIZE, pWidthInPixels, pHeightInPixels);
	}

	HRESULT SetFrameDimensions(UINT32 WidthInPixels, UINT32 HeightInPixels)
	{
		return ::MFSetAttributeSize(GetMediaType(), MF_MT_FRAME_SIZE, WidthInPixels, HeightInPixels);
	}

	HRESULT GetDataBitErrorRate(UINT32 *pRate)
	{
		if (pRate == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_AVG_BIT_ERROR_RATE, pRate);
	}

	HRESULT SetDataBitErrorRate(UINT32 rate)
	{
		return GetMediaType()->SetUINT32(MF_MT_AVG_BIT_ERROR_RATE, rate);
	}

	HRESULT GetAverageBitRate(UINT32 *pRate)
	{
		if (pRate == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_AVG_BITRATE, pRate);
	}

	HRESULT SetAvgerageBitRate(UINT32 rate)
	{
		return GetMediaType()->SetUINT32(MF_MT_AVG_BITRATE, rate);
	}

	HRESULT GetCustomVideoPrimaries(MT_CUSTOM_VIDEO_PRIMARIES *pPrimaries)
	{
		if (pPrimaries == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetBlob(MF_MT_CUSTOM_VIDEO_PRIMARIES, (UINT8*)pPrimaries, sizeof(MT_CUSTOM_VIDEO_PRIMARIES), nullptr);
	}

	HRESULT SetCustomVideoPrimaries(const MT_CUSTOM_VIDEO_PRIMARIES &primary)
	{
		return GetMediaType()->SetBlob(MF_MT_CUSTOM_VIDEO_PRIMARIES, (const UINT8*)&primary, sizeof(MT_CUSTOM_VIDEO_PRIMARIES));
	}

	HRESULT GetFrameRate(UINT32 *pNumerator, UINT32 *pDenominator)
	{
		if ((pNumerator == nullptr) || (pDenominator == nullptr)) {
			return E_POINTER;
		}
		return ::MFGetAttributeRatio(GetMediaType(), MF_MT_FRAME_RATE, pNumerator, pDenominator);
	}

	HRESULT GetFrameRate(MFRatio *pRatio)
	{
		if (pRatio == nullptr) {
			return E_POINTER;
		}
		return GetFrameRate((UINT32*)&pRatio->Numerator, (UINT32*)&pRatio->Denominator);
	}

	HRESULT SetFrameRate(UINT32 Numerator, UINT32 Denominator)
	{
		return ::MFSetAttributeRatio(GetMediaType(), MF_MT_FRAME_RATE, Numerator, Denominator);
	}

	HRESULT SetFrameRate(const MFRatio &ratio)
	{
		return ::MFSetAttributeRatio(GetMediaType(), MF_MT_FRAME_RATE, ratio.Numerator, ratio.Denominator);
	}

	HRESULT GetGeometricAperture(MFVideoArea *pArea)
	{
		if (pArea == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetBlob(MF_MT_GEOMETRIC_APERTURE, (UINT8*)pArea, sizeof(MFVideoArea), nullptr);
	}

	HRESULT SetGeometricAperture(const MFVideoArea& area)
	{
		return GetMediaType()->SetBlob(MF_MT_GEOMETRIC_APERTURE, (UINT8*)&area, sizeof(MFVideoArea));
	}

	HRESULT GetMaxKeyframeSpacing(UINT32 *pSpacing)
	{
		if (pSpacing == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_MAX_KEYFRAME_SPACING, pSpacing);
	}

	HRESULT SetMaxKeyframeSpacing(UINT32 Spacing)
	{
		return GetMediaType()->SetUINT32(MF_MT_MAX_KEYFRAME_SPACING, Spacing);
	}

	HRESULT GetMinDisplayAperture(MFVideoArea *pArea)
	{
		if (pArea == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE, (UINT8*)pArea, sizeof(MFVideoArea), nullptr);
	}

	HRESULT SetMinDisplayAperture(const MFVideoArea &area)
	{
		return GetMediaType()->SetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE, (const UINT8*)&area, sizeof(MFVideoArea));
	}

	HRESULT GetPadControlFlags(MFVideoPadFlags *pFlags)
	{
		if (pFlags == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_PAD_CONTROL_FLAGS, (UINT32*)pFlags);
	}

	HRESULT SetPadControlFlags(MFVideoPadFlags flags)
	{
		return GetMediaType()->SetUINT32(MF_MT_PAD_CONTROL_FLAGS, flags);
	}

	HRESULT GetPaletteEntries(MFPaletteEntry *paEntries, UINT32 nEntries)
	{
		if (paEntries == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetBlob(MF_MT_PALETTE, (UINT8*)paEntries, sizeof(MFPaletteEntry) * nEntries, nullptr);
	}

	HRESULT SetPaletteEntries(MFPaletteEntry *paEntries, UINT32 nEntries)
	{
		if (paEntries == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->SetBlob(MF_MT_PALETTE, (UINT8*)paEntries, sizeof(MFPaletteEntry) * nEntries);
	}

	HRESULT GetNumPaletteEntries(UINT32 *pEntries)
	{
		if (pEntries == nullptr) {
			return E_POINTER;
		}

		UINT32 Bytes = 0;
		HRESULT hr = GetMediaType()->GetBlobSize(MF_MT_PALETTE, &Bytes);
		if (SUCCEEDED(hr)) {
			if (Bytes % sizeof(MFPaletteEntry) != 0) {
				hr = E_UNEXPECTED;
			} else {
				*pEntries = Bytes / sizeof(MFPaletteEntry);
			}
		}
		return hr;
	}

	HRESULT GetPanScanAperture(MFVideoArea *pArea)
	{
		if (pArea == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetBlob(MF_MT_PAN_SCAN_APERTURE, (UINT8*)pArea, sizeof(MFVideoArea), nullptr);
	}

	HRESULT SetPanScanAperture(const MFVideoArea &area)
	{
		return GetMediaType()->SetBlob(MF_MT_PAN_SCAN_APERTURE, (const UINT8*)&area, sizeof(MFVideoArea));
	}

	HRESULT IsPanScanEnabled(BOOL *pBool)
	{
		if (pBool == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_PAN_SCAN_ENABLED, (UINT32*)pBool);
	}

	HRESULT SetPanScanEnabled(BOOL bEnabled)
	{
		return GetMediaType()->SetUINT32(MF_MT_PAN_SCAN_ENABLED, bEnabled);
	}

	HRESULT GetPixelAspectRatio(UINT32 *pNumerator, UINT32 *pDenominator)
	{
		if ((pNumerator == nullptr) || (pDenominator == nullptr)) {
			return E_POINTER;
		}
		return ::MFGetAttributeRatio(GetMediaType(), MF_MT_PIXEL_ASPECT_RATIO, pNumerator, pDenominator);
	}

	HRESULT SetPixelAspectRatio(UINT32 Numerator, UINT32 Denominator)
	{
		return ::MFSetAttributeRatio(GetMediaType(), MF_MT_PIXEL_ASPECT_RATIO, Numerator, Denominator);
	}

	HRESULT SetPixelAspectRatio(const MFRatio &ratio)
	{
		return ::MFSetAttributeRatio(GetMediaType(), MF_MT_PIXEL_ASPECT_RATIO, ratio.Numerator, ratio.Denominator);
	}

	HRESULT GetSourceContentHint(MFVideoSrcContentHintFlags *pFlags)
	{
		if (pFlags == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_SOURCE_CONTENT_HINT, (UINT32*)pFlags);
	}

	HRESULT SetSourceContentHint(MFVideoSrcContentHintFlags Flags)
	{
		return GetMediaType()->SetUINT32(MF_MT_SOURCE_CONTENT_HINT, (UINT32)Flags);
	}

	HRESULT GetTransferFunction(MFVideoTransferFunction *pFxn)
	{
		if (pFxn == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_TRANSFER_FUNCTION, (UINT32*)pFxn);
	}

	HRESULT SetTransferFunction(MFVideoTransferFunction Fxn)
	{
		return GetMediaType()->SetUINT32(MF_MT_TRANSFER_FUNCTION, (UINT32)Fxn);
	}

	HRESULT GetChromaSiting(MFVideoChromaSubsampling *pSampling)
	{
		if (pSampling == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_VIDEO_CHROMA_SITING, (UINT32*)pSampling);
	}

	HRESULT SetChromaSiting(MFVideoChromaSubsampling Sampling)
	{
		return GetMediaType()->SetUINT32(MF_MT_VIDEO_CHROMA_SITING, (UINT32)Sampling);
	}

	HRESULT GetVideoLighting(MFVideoLighting *pLighting)
	{
		if (pLighting == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_VIDEO_LIGHTING, (UINT32*)pLighting);
	}

	HRESULT SetVideoLighting(MFVideoLighting Lighting)
	{
		return GetMediaType()->SetUINT32(MF_MT_VIDEO_LIGHTING, (UINT32)Lighting);
	}

	HRESULT GetVideoNominalRange(MFNominalRange *pRange)
	{
		if (pRange == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, (UINT32*)pRange);
	}

	HRESULT SetVideoNominalRange(MFNominalRange Range)
	{
		return GetMediaType()->SetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, (UINT32)Range);
	}

	HRESULT GetVideoPrimaries(MFVideoPrimaries *pPrimaries)
	{
		if (pPrimaries == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_VIDEO_PRIMARIES, (UINT32*)pPrimaries);
	}

	HRESULT SetVideoPrimaries(MFVideoPrimaries Primaries)
	{
		return GetMediaType()->SetUINT32(MF_MT_VIDEO_PRIMARIES, (UINT32)Primaries);
	}

	HRESULT GetYUVMatrix(MFVideoTransferMatrix *pMatrix)
	{
		if (pMatrix == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_YUV_MATRIX, (UINT32*)pMatrix);
	}

	HRESULT SetYUVMatrix(MFVideoTransferMatrix Matrix)
	{
		return GetMediaType()->SetUINT32(MF_MT_YUV_MATRIX, (UINT32)Matrix);
	}

	MFRatio GetPixelAspectRatio()
	{
		MFRatio PAR = {0, 0};

		HRESULT hr = ::MFGetAttributeRatio(GetMediaType(), MF_MT_PIXEL_ASPECT_RATIO, (UINT32*)&PAR.Numerator, (UINT32*)&PAR.Denominator);
		if (FAILED(hr)) {
			PAR.Numerator = 1;
			PAR.Denominator = 1;
		}
		return PAR;
	}

	BOOL IsPanScanEnabled()
	{
		return (BOOL)::MFGetAttributeUINT32(GetMediaType(), MF_MT_PAN_SCAN_ENABLED, FALSE);
	}

	HRESULT GetVideoDisplayArea(MFVideoArea *pArea)
	{
		if (pArea == nullptr) {
			return E_POINTER;
		}
		return ::GetVideoDisplayArea(GetMediaType(), pArea);
	}
};


class AudioType : public MediaType
{
	friend class MediaType; 

public:
	AudioType(IMFMediaType *pType = nullptr)
		: MediaType(pType)
	{
	}

	virtual HRESULT CreateEmptyType()
	{
		SafeRelease(m_pType);

		HRESULT hr = MediaType::CreateEmptyType();

		if (SUCCEEDED(hr)) {
			hr = SetMajorType(MFMediaType_Audio);
		}
		return hr;
	}

	HRESULT GetAvgerageBytesPerSecond(UINT32 *pBytes)
	{
		if (pBytes == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, pBytes);
	}

	HRESULT SetAvgerageBytesPerSecond(UINT32 Bytes)
	{
		return GetMediaType()->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, Bytes);
	}

	HRESULT GetBitsPerSample(UINT32 *pBits)
	{
		if (pBits == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, pBits);
	}

	HRESULT SetBitsPerSample(UINT32 Bits)
	{
		return GetMediaType()->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, Bits);
	}

	HRESULT GetBlockAlignment(UINT32 *pBytes)
	{
		if (pBytes == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, pBytes);
	}

	HRESULT SetBlockAlignment(UINT32 Bytes)
	{
		return GetMediaType()->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, Bytes);
	}

	HRESULT GetChannelMask(UINT32 *pMask)
	{
		if (pMask == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_AUDIO_CHANNEL_MASK, pMask);
	}

	HRESULT SetChannelMask(UINT32 Mask)
	{
		return GetMediaType()->SetUINT32(MF_MT_AUDIO_CHANNEL_MASK, Mask);
	}

	HRESULT GetFloatSamplesPerSecond(double *pfSampleRate)
	{
		if (pfSampleRate == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetDouble(MF_MT_AUDIO_FLOAT_SAMPLES_PER_SECOND, pfSampleRate);
	}

	HRESULT SetFloatSamplesPerSecond(double fSampleRate)
	{
		return GetMediaType()->SetDouble(MF_MT_AUDIO_FLOAT_SAMPLES_PER_SECOND, fSampleRate);
	}

	HRESULT GetNumChannels(UINT32 *pChannels)
	{
		if (pChannels == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, pChannels);
	}

	HRESULT SetNumChannels(UINT32 Channels)
	{
		return GetMediaType()->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, Channels);
	}

	HRESULT GetSamplesPerBlock(UINT32 *pSamples)
	{
		if (pSamples == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_BLOCK, pSamples);
	}

	HRESULT SetSamplesPerBlock(UINT32 Samples)
	{
		return GetMediaType()->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_BLOCK, Samples);
	}

	HRESULT GetSamplesPerSecond(UINT32 *pSampleRate)
	{
		if (pSampleRate) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, pSampleRate);
	}

	HRESULT SetSamplesPerSecond(UINT32 SampleRate)
	{
		return GetMediaType()->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, SampleRate);
	}

	HRESULT GetValidBitsPerSample(UINT32 *pBits)
	{
		if (pBits == nullptr) {
			return E_POINTER;
		}
		HRESULT hr = GetMediaType()->GetUINT32(MF_MT_AUDIO_VALID_BITS_PER_SAMPLE, pBits);
		if (FAILED(hr)) {
			hr = GetBitsPerSample(pBits);
		}
		return hr;
	}

	HRESULT SetValidBitsPerSample(UINT32 Bits)
	{
		return GetMediaType()->SetUINT32(MF_MT_AUDIO_VALID_BITS_PER_SAMPLE, Bits);
	}

	UINT32 AvgerageBytesPerSecond()
	{
		return ::MFGetAttributeUINT32(GetMediaType(), MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 0);
	}

	UINT32 BitsPerSample()
	{
		return ::MFGetAttributeUINT32(GetMediaType(), MF_MT_AUDIO_BITS_PER_SAMPLE, 0);
	}
	
	UINT32 GetBlockAlignment()
	{
		return ::MFGetAttributeUINT32(GetMediaType(), MF_MT_AUDIO_BLOCK_ALIGNMENT, 0);
	}
	
	double FloatSamplesPerSecond()
	{
		return ::MFGetAttributeDouble(GetMediaType(), MF_MT_AUDIO_FLOAT_SAMPLES_PER_SECOND, 0.0);
	}

	UINT32 NumChannels()
	{
		return ::MFGetAttributeUINT32(GetMediaType(), MF_MT_AUDIO_NUM_CHANNELS, 0);
	}

	UINT32 SamplesPerSecond()
	{
		return ::MFGetAttributeUINT32(GetMediaType(), MF_MT_AUDIO_SAMPLES_PER_SECOND, 0);
	}
};


class MPEGVideoType : public VideoType
{
	friend class MediaType;

public:

	MPEGVideoType(IMFMediaType *pType = nullptr)
		: VideoType(pType)
	{
	}

	HRESULT GetMpegSeqHeader(BYTE *pData, UINT32 cbSize)
	{
		if (pData == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetBlob(MF_MT_MPEG_SEQUENCE_HEADER, pData, cbSize, nullptr);
	}

	HRESULT SetMpegSeqHeader(const BYTE *pData, UINT32 cbSize)
	{
		if (pData == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->SetBlob(MF_MT_MPEG_SEQUENCE_HEADER, pData, cbSize);
	}

	HRESULT GetMpegSeqHeaderSize(UINT32 *pcbSize)
	{
		if (pcbSize == nullptr) {
			return E_POINTER;
		}

		*pcbSize = 0;

		HRESULT hr = GetMediaType()->GetBlobSize(MF_MT_MPEG_SEQUENCE_HEADER, pcbSize);
		if (hr == MF_E_ATTRIBUTENOTFOUND) {
			hr = S_OK;
		}
		return hr;
	}

	HRESULT GetStartTimeCode(UINT32 *pTime)
	{
		if (pTime == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_MPEG_START_TIME_CODE, pTime);
	}

	HRESULT SetStartTimeCode(UINT32 Time)
	{
		return GetMediaType()->SetUINT32(MF_MT_MPEG_START_TIME_CODE, Time);
	}

	HRESULT GetMPEG2Flags(UINT32 *pFlags)
	{
		if (pFlags == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_MPEG2_FLAGS, pFlags);
	}

	HRESULT SetMPEG2Flags(UINT32 Flags)
	{
		return GetMediaType()->SetUINT32(MF_MT_MPEG2_FLAGS, Flags);
	}

	HRESULT GetMPEG2Level(UINT32 *pLevel)
	{
		if (pLevel == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_MPEG2_LEVEL, pLevel);
	}

	HRESULT SetMPEG2Level(UINT32 Level)
	{
		return GetMediaType()->SetUINT32(MF_MT_MPEG2_LEVEL, Level);
	}

	HRESULT GetMPEG2Profile(UINT32 *pProfile)
	{
		if (pProfile == nullptr) {
			return E_POINTER;
		}
		return GetMediaType()->GetUINT32(MF_MT_MPEG2_PROFILE, pProfile);
	}

	HRESULT SetMPEG2Profile(UINT32 Profile)
	{
		return GetMediaType()->SetUINT32(MF_MT_MPEG2_PROFILE, Profile);
	}
};
