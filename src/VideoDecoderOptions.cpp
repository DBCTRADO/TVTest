#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "VideoDecoderOptions.h"
#include "Common/DebugDef.h"




CVideoDecoderOptions::CVideoDecoderOptions()
	: CSettingsBase(TEXT("VideoDecoder"))
{
}


bool CVideoDecoderOptions::ReadSettings(CSettings &Settings)
{
	CMediaViewer &MediaViewer = GetAppClass().CoreEngine.m_DtvEngine.m_MediaViewer;
	VideoDecoderSettings DecoderSettings;
	int Value;

	MediaViewer.GetVideoDecoderSettings(&DecoderSettings);

	Settings.Read(TEXT("EnableDeinterlace"), &DecoderSettings.bEnableDeinterlace);
	if (Settings.Read(TEXT("DeinterlaceMethod"), &Value))
		DecoderSettings.DeinterlaceMethod = static_cast<TVTVIDEODEC_DeinterlaceMethod>(Value);
	Settings.Read(TEXT("AdaptProgressive"), &DecoderSettings.bAdaptProgressive);
	Settings.Read(TEXT("AdaptTelecine"), &DecoderSettings.bAdaptTelecine);
	Settings.Read(TEXT("SetInterlacedFlag"), &DecoderSettings.bSetInterlacedFlag);
	Settings.Read(TEXT("Brightness"), &DecoderSettings.Brightness);
	Settings.Read(TEXT("Contrast"), &DecoderSettings.Contrast);
	Settings.Read(TEXT("Hue"), &DecoderSettings.Hue);
	Settings.Read(TEXT("Saturation"), &DecoderSettings.Saturation);
	Settings.Read(TEXT("NumThreads"), &DecoderSettings.NumThreads);
	Settings.Read(TEXT("EnableDXVA2"), &DecoderSettings.bEnableDXVA2);

	MediaViewer.SetVideoDecoderSettings(DecoderSettings);

	return true;
}


bool CVideoDecoderOptions::WriteSettings(CSettings &Settings)
{
	CMediaViewer &MediaViewer = GetAppClass().CoreEngine.m_DtvEngine.m_MediaViewer;
	VideoDecoderSettings DecoderSettings;

	if (!MediaViewer.GetVideoDecoderSettings(&DecoderSettings))
		return false;

	Settings.Write(TEXT("EnableDeinterlace"), DecoderSettings.bEnableDeinterlace);
	Settings.Write(TEXT("DeinterlaceMethod"), static_cast<int>(DecoderSettings.DeinterlaceMethod));
	Settings.Write(TEXT("AdaptProgressive"), DecoderSettings.bAdaptProgressive);
	Settings.Write(TEXT("AdaptTelecine"), DecoderSettings.bAdaptTelecine);
	Settings.Write(TEXT("SetInterlacedFlag"), DecoderSettings.bSetInterlacedFlag);
	Settings.Write(TEXT("Brightness"), DecoderSettings.Brightness);
	Settings.Write(TEXT("Contrast"), DecoderSettings.Contrast);
	Settings.Write(TEXT("Hue"), DecoderSettings.Hue);
	Settings.Write(TEXT("Saturation"), DecoderSettings.Saturation);
	Settings.Write(TEXT("NumThreads"), DecoderSettings.NumThreads);
	Settings.Write(TEXT("EnableDXVA2"), DecoderSettings.bEnableDXVA2);

	return true;
}
