/*
  TVTest
  Copyright(c) 2008-2020 DBCTRADO

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef TVTEST_VIDEO_DECODER_OPTIONS_H
#define TVTEST_VIDEO_DECODER_OPTIONS_H


#include "Settings.h"
#include "LibISDB/LibISDB/Windows/Viewer/DirectShow/KnownDecoderManager.hpp"


namespace TVTest
{

	class CVideoDecoderOptions
		: public CSettingsBase
	{
	public:
		typedef LibISDB::DirectShow::KnownDecoderManager::VideoDecoderSettings VideoDecoderSettings;

		CVideoDecoderOptions();

		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;
		bool ApplyVideoDecoderSettings();
		void SetVideoDecoderSettings(const VideoDecoderSettings &Settings) { m_VideoDecoderSettings = Settings; }
		const VideoDecoderSettings &GetVideoDecoderSettings() const { return m_VideoDecoderSettings; }

	private:
		VideoDecoderSettings m_VideoDecoderSettings;
	};

} // namespace TVTest


#endif
