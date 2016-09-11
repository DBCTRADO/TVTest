/*
 *  TVTest DTV Video Decoder
 *  Copyright (C) 2015-2016 DBCTRADO
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


#if defined(_WIN64)
#include <pshpack8.h>
#else
#include <pshpack4.h>
#endif


#define TVTVIDEODEC_FILTER_NAME L"TVTest DTV Video Decoder"

enum TVTVIDEODEC_DeinterlaceMethod : int
{
	TVTVIDEODEC_DEINTERLACE_WEAVE,
	TVTVIDEODEC_DEINTERLACE_BLEND,
	TVTVIDEODEC_DEINTERLACE_BOB,
	TVTVIDEODEC_DEINTERLACE_ELA,
	TVTVIDEODEC_DEINTERLACE_YADIF,
	TVTVIDEODEC_DEINTERLACE_YADIF_BOB
};
#define TVTVIDEODEC_DEINTERLACE_FIRST TVTVIDEODEC_DEINTERLACE_WEAVE
#define TVTVIDEODEC_DEINTERLACE_LAST  TVTVIDEODEC_DEINTERLACE_YADIF_BOB

#define TVTVIDEODEC_BRIGHTNESS_MIN (-100)
#define TVTVIDEODEC_BRIGHTNESS_MAX 100
#define TVTVIDEODEC_CONTRAST_MIN   (-100)
#define TVTVIDEODEC_CONTRAST_MAX   100
#define TVTVIDEODEC_HUE_MIN        (-180)
#define TVTVIDEODEC_HUE_MAX        180
#define TVTVIDEODEC_SATURATION_MIN (-100)
#define TVTVIDEODEC_SATURATION_MAX 100

#define TVTVIDEODEC_MAX_THREADS 32

struct TVTVIDEODEC_DXVADeviceInfo
{
	WCHAR Description[512];
	WORD Product;
	WORD Version;
	WORD SubVersion;
	WORD Build;
	DWORD VendorID;
	DWORD DeviceID;
	DWORD SubSystemID;
	DWORD Revision;
};

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
	DWORD RepeatFieldCount;
	DWORD Mode;
	TVTVIDEODEC_DXVADeviceInfo DXVADeviceInfo;
};

#define TVTVIDEODEC_STAT_OUT_SIZE            0x00000001
#define TVTVIDEODEC_STAT_FRAME_COUNT         0x00000002
#define TVTVIDEODEC_STAT_PLAYBACK_RATE       0x00000004
#define TVTVIDEODEC_STAT_BASE_TIME_PER_FRAME 0x00000008
#define TVTVIDEODEC_STAT_FIELD_COUNT         0x00000010
#define TVTVIDEODEC_STAT_MODE                0x00000020
#define TVTVIDEODEC_STAT_DXVA_DEVICE_INFO    0x00000040
#define TVTVIDEODEC_STAT_ALL                 0x0000007f

#define TVTVIDEODEC_MODE_DXVA2               0x00000001

#define TVTVIDEODEC_FRAME_TOP_FIELD_FIRST    0x00000001
#define TVTVIDEODEC_FRAME_REPEAT_FIRST_FIELD 0x00000002
#define TVTVIDEODEC_FRAME_PROGRESSIVE        0x00000004
#define TVTVIDEODEC_FRAME_WEAVE              0x00000008
#define TVTVIDEODEC_FRAME_TYPE_I             0x00000010
#define TVTVIDEODEC_FRAME_TYPE_P             0x00000020
#define TVTVIDEODEC_FRAME_TYPE_B             0x00000040

struct TVTVIDEODEC_ColorDescription
{
	BYTE Flags;
	BYTE ColorPrimaries;
	BYTE MatrixCoefficients;
	BYTE TransferCharacteristics;
};

#define TVTVIDEODEC_COLOR_DESCRIPTION_PRESENT 0x01

struct TVTVIDEODEC_FrameInfo
{
	int Width;
	int Height;
	int AspectX;
	int AspectY;
	GUID Subtype;
	const BYTE * const *Buffer;
	const int *Pitch;
	DWORD Flags;
	TVTVIDEODEC_ColorDescription ColorDescription;
};

/* Frame capture interface */
MIDL_INTERFACE("37669070-5574-44DA-BF20-A84A959990B5")
ITVTestVideoDecoderFrameCapture : public IUnknown
{
	STDMETHOD(OnFrame)(const TVTVIDEODEC_FrameInfo *pFrameInfo) PURE;
};

/* DirectShow decoder interface */
MIDL_INTERFACE("AE0BF9FF-EBCE-4412-9EFC-C6EE86B20855")
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
	STDMETHOD(SetEnableDXVA2)(BOOL fEnable) PURE;
	STDMETHOD_(BOOL, GetEnableDXVA2)() PURE;

	STDMETHOD(LoadOptions)() PURE;
	STDMETHOD(SaveOptions)() PURE;

	STDMETHOD(GetStatistics)(TVTVIDEODEC_Statistics *pStatistics) PURE;

	STDMETHOD(SetFrameCapture)(ITVTestVideoDecoderFrameCapture *pFrameCapture) PURE;
};

/* Stand-alone decoder interface */
MIDL_INTERFACE("B08074A7-7033-4ABA-AC28-972F55543736")
ITVTestVideoFrameDecoder : public IUnknown
{
	STDMETHOD(Open)(REFGUID VideoSubtype) PURE;
	STDMETHOD(Close)() PURE;

	STDMETHOD(InputStream)(const void *pData, SIZE_T Size) PURE;

	STDMETHOD(SetFrameCapture)(ITVTestVideoDecoderFrameCapture *pFrameCapture, REFGUID Subtype) PURE;

	STDMETHOD(SetDeinterlaceMethod)(TVTVIDEODEC_DeinterlaceMethod Method) PURE;
	STDMETHOD_(TVTVIDEODEC_DeinterlaceMethod, GetDeinterlaceMethod)() PURE;
	STDMETHOD(SetWaitForKeyFrame)(BOOL fWait) PURE;
	STDMETHOD_(BOOL, GetWaitForKeyFrame)() PURE;
	STDMETHOD(SetSkipBFrames)(BOOL fSkip) PURE;
	STDMETHOD_(BOOL, GetSkipBFrames)() PURE;
	STDMETHOD(SetNumThreads)(int NumThreads) PURE;
	STDMETHOD_(int, GetNumThreads)() PURE;
};

struct TVTestVideoDecoderInfo
{
	DWORD HostVersion;
	DWORD ModuleVersion;
	DWORD InterfaceVersion;
};

#define TVTVIDEODEC_VERSION_(major, minor, rev) \
	(((major) << 24) | ((minor) << 12) | (rev))
#define TVTVIDEODEC_VERSION_GET_MAJOR(ver) ((ver) >> 24)
#define TVTVIDEODEC_VERSION_GET_MINOR(ver) (((ver) >> 12) & 0xfff)
#define TVTVIDEODEC_VERSION_GET_REV(ver)   ((ver) & 0xfff)

#define TVTVIDEODEC_HOST_VERSION      TVTVIDEODEC_VERSION_(0, 0, 0)
#define TVTVIDEODEC_INTERFACE_VERSION TVTVIDEODEC_VERSION_(0, 2, 0)

#ifdef TVTVIDEODEC_IMPL
#define TVTVIDEODEC_EXPORT extern "C" __declspec(dllexport)
#else
#define TVTVIDEODEC_EXPORT extern "C" __declspec(dllimport)
#endif

TVTVIDEODEC_EXPORT BOOL WINAPI TVTestVideoDecoder_GetInfo(TVTestVideoDecoderInfo *pInfo);
TVTVIDEODEC_EXPORT HRESULT WINAPI TVTestVideoDecoder_CreateInstance(REFIID riid, void **ppObject);


#include <poppack.h>
