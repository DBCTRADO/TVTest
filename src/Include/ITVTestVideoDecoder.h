/*
 *  TVTest DTV Video Decoder
 *  Copyright (C) 2015 DBCTRADO
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once


#define TVTVIDEODEC_FILTER_NAME L"TVTest DTV Video Decoder"

enum TVTVIDEODEC_DeinterlaceMethod : int
{
	TVTVIDEODEC_DEINTERLACE_WEAVE,
	TVTVIDEODEC_DEINTERLACE_BLEND,
	TVTVIDEODEC_DEINTERLACE_BOB,
	TVTVIDEODEC_DEINTERLACE_ELA,
	TVTVIDEODEC_DEINTERLACE_YADIF
};
#define TVTVIDEODEC_DEINTERLACE_FIRST TVTVIDEODEC_DEINTERLACE_WEAVE
#define TVTVIDEODEC_DEINTERLACE_LAST  TVTVIDEODEC_DEINTERLACE_YADIF

#define TVTVIDEODEC_BRIGHTNESS_MIN (-100)
#define TVTVIDEODEC_BRIGHTNESS_MAX 100
#define TVTVIDEODEC_CONTRAST_MIN   (-100)
#define TVTVIDEODEC_CONTRAST_MAX   100
#define TVTVIDEODEC_HUE_MIN        (-180)
#define TVTVIDEODEC_HUE_MAX        180
#define TVTVIDEODEC_SATURATION_MIN (-100)
#define TVTVIDEODEC_SATURATION_MAX 100

#define TVTVIDEODEC_MAX_THREADS 32

struct TVTVIDEODEC_Statistics
{
	DWORD Mask;
	int OutWidth;
	int OutHeight;
	int OutAspectX;
	int OutAspectY;
	DWORD IFrameCount;
	DWORD PFrameCount;
	DWORD BFrameCount;
	DWORD SkippedFrameCount;
	LONG PlaybackRate;
	LONGLONG BaseTimePerFrame;
};

#define TVTVIDEODEC_STAT_OUT_SIZE            0x00000001
#define TVTVIDEODEC_STAT_FRAME_COUNT         0x00000002
#define TVTVIDEODEC_STAT_PLAYBACK_RATE       0x00000004
#define TVTVIDEODEC_STAT_BASE_TIME_PER_FRAME 0x00000008
#define TVTVIDEODEC_STAT_ALL                 0x0000000f

#define TVTVIDEODEC_FRAME_TOP_FIELD_FIRST    0x00000001
#define TVTVIDEODEC_FRAME_REPEAT_FIRST_FIELD 0x00000002
#define TVTVIDEODEC_FRAME_PROGRESSIVE        0x00000004
#define TVTVIDEODEC_FRAME_WEAVE              0x00000008
#define TVTVIDEODEC_FRAME_TYPE_I             0x00000010
#define TVTVIDEODEC_FRAME_TYPE_P             0x00000020
#define TVTVIDEODEC_FRAME_TYPE_B             0x00000040

MIDL_INTERFACE("803267E1-0A79-4F9D-E167-3280790A9D4F")
ITVTestVideoDecoderFrameCapture : public IUnknown
{
	STDMETHOD(OnFrame)(int Width, int Height, REFGUID subtype, const BYTE * const *Buffer, const int *Pitch, DWORD Flags) PURE;
};

MIDL_INTERFACE("5BF96108-6F7E-4D89-0861-F95B7E6F894D")
ITVTestVideoDecoder : public IUnknown
{
	STDMETHOD(SetEnableDeinterlace)(BOOL fEnable) PURE;
	STDMETHOD_(BOOL, GetEnableDeinterlace)() PURE;
	STDMETHOD(SetDeinterlaceMethod)(TVTVIDEODEC_DeinterlaceMethod Method) PURE;
	STDMETHOD_(TVTVIDEODEC_DeinterlaceMethod, GetDeinterlaceMethod)() PURE;
	STDMETHOD(SetAdaptProgressive)(BOOL fEnable) PURE;
	STDMETHOD_(BOOL, GetAdaptProgressive)() PURE;
	STDMETHOD(SetAdaptTelecine)(BOOL fEnable) PURE;
	STDMETHOD_(BOOL, GetAdaptTelecine)() PURE;
	STDMETHOD(SetInterlacedFlag)(BOOL fEnable) PURE;
	STDMETHOD_(BOOL, GetInterlacedFlag)() PURE;

	STDMETHOD(SetBrightness)(int Brightness) PURE;
	STDMETHOD_(int, GetBrightness)() PURE;
	STDMETHOD(SetContrast)(int Contrast) PURE;
	STDMETHOD_(int, GetContrast)() PURE;
	STDMETHOD(SetHue)(int Hue) PURE;
	STDMETHOD_(int, GetHue)() PURE;
	STDMETHOD(SetSaturation)(int Saturation) PURE;
	STDMETHOD_(int, GetSaturation)() PURE;

	STDMETHOD(SetNumThreads)(int NumThreads) PURE;
	STDMETHOD_(int, GetNumThreads)() PURE;

	STDMETHOD(LoadOptions)() PURE;
	STDMETHOD(SaveOptions)() PURE;

	STDMETHOD(GetStatistics)(TVTVIDEODEC_Statistics *pStatistics) PURE;

	STDMETHOD(SetFrameCapture)(ITVTestVideoDecoderFrameCapture *pFrameCapture) PURE;
};

#ifdef TVTVIDEODEC_IMPL
#define TVTVIDEODEC_EXPORT extern "C" __declspec(dllexport)
#else
#define TVTVIDEODEC_EXPORT extern "C" __declspec(dllimport)
#endif

TVTVIDEODEC_EXPORT HRESULT WINAPI TVTestVideoDecoder_CreateInstance(ITVTestVideoDecoder **ppDecoder);
