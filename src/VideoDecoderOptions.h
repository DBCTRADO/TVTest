#ifndef TVTEST_VIDEO_DECODER_OPTIONS_H
#define TVTEST_VIDEO_DECODER_OPTIONS_H


#include "Settings.h"
#include "LibISDB/LibISDB/Windows/Viewer/DirectShow/KnownDecoderManager.hpp"


class CVideoDecoderOptions : public CSettingsBase
{
public:
	typedef LibISDB::DirectShow::KnownDecoderManager::VideoDecoderSettings VideoDecoderSettings;

	CVideoDecoderOptions();
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;
	bool ApplyVideoDecoderSettings();
	void SetVideoDecoderSettings(const VideoDecoderSettings &Settings) { m_VideoDecoderSettings = Settings; }
	const VideoDecoderSettings &GetVideoDecoderSettings() const { m_VideoDecoderSettings; }

private:
	VideoDecoderSettings m_VideoDecoderSettings;
};


#endif
