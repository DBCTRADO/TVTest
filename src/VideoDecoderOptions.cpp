#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "VideoDecoderOptions.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CVideoDecoderOptions::CVideoDecoderOptions()
	: CSettingsBase(TEXT("VideoDecoder"))
{
}


bool CVideoDecoderOptions::ReadSettings(CSettings &Settings)
{
	int Value;

	Settings.Read(TEXT("EnableDeinterlace"), &m_VideoDecoderSettings.bEnableDeinterlace);
	if (Settings.Read(TEXT("DeinterlaceMethod"), &Value))
		m_VideoDecoderSettings.DeinterlaceMethod = static_cast<TVTVIDEODEC_DeinterlaceMethod>(Value);
	Settings.Read(TEXT("AdaptProgressive"), &m_VideoDecoderSettings.bAdaptProgressive);
	Settings.Read(TEXT("AdaptTelecine"), &m_VideoDecoderSettings.bAdaptTelecine);
	Settings.Read(TEXT("SetInterlacedFlag"), &m_VideoDecoderSettings.bSetInterlacedFlag);
	Settings.Read(TEXT("Brightness"), &m_VideoDecoderSettings.Brightness);
	Settings.Read(TEXT("Contrast"), &m_VideoDecoderSettings.Contrast);
	Settings.Read(TEXT("Hue"), &m_VideoDecoderSettings.Hue);
	Settings.Read(TEXT("Saturation"), &m_VideoDecoderSettings.Saturation);
	Settings.Read(TEXT("NumThreads"), &m_VideoDecoderSettings.NumThreads);
	Settings.Read(TEXT("EnableDXVA2"), &m_VideoDecoderSettings.bEnableDXVA2);

	return true;
}


bool CVideoDecoderOptions::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("EnableDeinterlace"), m_VideoDecoderSettings.bEnableDeinterlace);
	Settings.Write(TEXT("DeinterlaceMethod"), static_cast<int>(m_VideoDecoderSettings.DeinterlaceMethod));
	Settings.Write(TEXT("AdaptProgressive"), m_VideoDecoderSettings.bAdaptProgressive);
	Settings.Write(TEXT("AdaptTelecine"), m_VideoDecoderSettings.bAdaptTelecine);
	Settings.Write(TEXT("SetInterlacedFlag"), m_VideoDecoderSettings.bSetInterlacedFlag);
	Settings.Write(TEXT("Brightness"), m_VideoDecoderSettings.Brightness);
	Settings.Write(TEXT("Contrast"), m_VideoDecoderSettings.Contrast);
	Settings.Write(TEXT("Hue"), m_VideoDecoderSettings.Hue);
	Settings.Write(TEXT("Saturation"), m_VideoDecoderSettings.Saturation);
	Settings.Write(TEXT("NumThreads"), m_VideoDecoderSettings.NumThreads);
	Settings.Write(TEXT("EnableDXVA2"), m_VideoDecoderSettings.bEnableDXVA2);

	return true;
}


bool CVideoDecoderOptions::ApplyVideoDecoderSettings()
{
	LibISDB::ViewerFilter *pViewer = GetAppClass().CoreEngine.GetFilter<LibISDB::ViewerFilter>();

	if (pViewer == nullptr)
		return false;

	pViewer->SetVideoDecoderSettings(m_VideoDecoderSettings);

	return true;
}


}	// namespace TVTest
