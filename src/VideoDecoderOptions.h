#ifndef TVTEST_VIDEO_DECODER_OPTIONS_H
#define TVTEST_VIDEO_DECODER_OPTIONS_H


#include "Settings.h"
#include "DirectShowFilter/InternalDecoder.h"


class CVideoDecoderOptions : public CSettingsBase
{
public:
	typedef CInternalDecoderManager::VideoDecoderSettings VideoDecoderSettings;

	CVideoDecoderOptions();
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;
};


#endif
