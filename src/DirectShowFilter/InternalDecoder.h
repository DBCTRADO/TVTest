#ifndef TVTEST_INTERNAL_DECODER_H
#define TVTEST_INTERNAL_DECODER_H


#include "ITVTestVideoDecoder.h"


class CInternalDecoderManager
{
public:
	struct VideoDecoderSettings
	{
		bool bEnableDeinterlace;
		TVTVIDEODEC_DeinterlaceMethod DeinterlaceMethod;
		bool bAdaptProgressive;
		bool bAdaptTelecine;
		bool bSetInterlacedFlag;
		int Brightness;
		int Contrast;
		int Hue;
		int Saturation;
		int NumThreads;
		bool bEnableDXVA2;

		VideoDecoderSettings()
			: bEnableDeinterlace(true)
			, DeinterlaceMethod(TVTVIDEODEC_DEINTERLACE_BLEND)
			, bAdaptProgressive(true)
			, bAdaptTelecine(true)
			, bSetInterlacedFlag(true)
			, Brightness(0)
			, Contrast(0)
			, Hue(0)
			, Saturation(0)
			, NumThreads(0)
			, bEnableDXVA2(false)
		{
		}
	};

	CInternalDecoderManager();
	~CInternalDecoderManager();
	HRESULT CreateInstance(const GUID &MediaSubType, IBaseFilter **ppFilter);
	void SetVideoDecoderSettings(const VideoDecoderSettings &Settings);
	bool GetVideoDecoderSettings(VideoDecoderSettings *pSettings) const;
	bool SaveVideoDecoderSettings(IBaseFilter *pFilter);

	static bool IsMediaSupported(const GUID &MediaSubType);
	static bool IsDecoderAvailable(const GUID &MediaSubType);
	static LPCWSTR GetDecoderName(const GUID &MediaSubType);

private:
	VideoDecoderSettings m_VideoDecoderSettings;
	HMODULE m_hLib;

	HRESULT LoadDecoderModule();
	void FreeDecoderModule();
	static bool GetModulePath(LPTSTR pszPath);
};


#endif
