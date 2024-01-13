/*
  TVTest
  Copyright(c) 2008-2022 DBCTRADO

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
	Settings.Read(TEXT("EnableD3D11"), &m_VideoDecoderSettings.bEnableD3D11);
	Settings.Read(TEXT("NumQueueFrames"), &m_VideoDecoderSettings.NumQueueFrames);

	CSettings::EntryList EntryList;
	if (Settings.GetEntries(&EntryList)) {
		m_VideoDecoderSettings.Properties.clear();
		m_VideoDecoderSettings.Properties.reserve(EntryList.size());

		for (const auto &Entry : EntryList) {
			if (Entry.Value.empty())
				continue;

			auto &Property = m_VideoDecoderSettings.Properties.emplace_back();

			Property.Name = Entry.Name;

			if (::lstrcmpi(Entry.Value.c_str(), TEXT("yes")) == 0
					|| ::lstrcmpi(Entry.Value.c_str(), TEXT("true")) == 0) {
				Property.Value.vt = VT_BOOL;
				Property.Value.boolVal = VARIANT_TRUE;
			} else if (::lstrcmpi(Entry.Value.c_str(), TEXT("no")) == 0
					|| ::lstrcmpi(Entry.Value.c_str(), TEXT("false")) == 0) {
				Property.Value.vt = VT_BOOL;
				Property.Value.boolVal = VARIANT_FALSE;
			} else {
				Property.Value.vt = VT_INT;
				Property.Value.intVal = ::StrToInt(Entry.Value.c_str());
			}
		}
	}

	return true;
}


bool CVideoDecoderOptions::WriteSettings(CSettings &Settings)
{
	if (!m_VideoDecoderSettings.Properties.empty()) {
		for (const auto &Property : m_VideoDecoderSettings.Properties) {
			switch (Property.Value.vt) {
			case VT_INT:
				Settings.Write(Property.Name.c_str(), Property.Value.intVal);
				break;
			case VT_BOOL:
				Settings.Write(Property.Name.c_str(), Property.Value.boolVal != VARIANT_FALSE);
				break;
			}
		}
	} else {
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
		Settings.Write(TEXT("EnableD3D11"), m_VideoDecoderSettings.bEnableD3D11);
		Settings.Write(TEXT("NumQueueFrames"), m_VideoDecoderSettings.NumQueueFrames);
	}

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


} // namespace TVTest
