/*
  TVTest
  Copyright(c) 2008-2017 DBCTRADO

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
#include "DPIUtil.h"
#include "Util.h"
#include "Common/DebugDef.h"


extern "C"
{

// from <ShellScalingApi.h>
#ifndef DPI_ENUMS_DECLARED
typedef enum MONITOR_DPI_TYPE {
	MDT_EFFECTIVE_DPI = 0,
	MDT_ANGULAR_DPI   = 1,
	MDT_RAW_DPI       = 2,
	MDT_DEFAULT       = MDT_EFFECTIVE_DPI
} MONITOR_DPI_TYPE;
#endif

STDAPI GetDpiForMonitor(
	HMONITOR hmonitor,
	MONITOR_DPI_TYPE dpiType,
	UINT *dpiX,
	UINT *dpiY
);

WINUSERAPI UINT WINAPI GetDpiForSystem(void);
WINUSERAPI UINT WINAPI GetDpiForWindow(HWND hwnd);
WINUSERAPI BOOL WINAPI EnableNonClientDpiScaling(HWND hwnd);
WINUSERAPI BOOL WINAPI AdjustWindowRectExForDpi(
	LPRECT lpRect,
	DWORD dwStyle,
	BOOL bMenu,
	DWORD dwExStyle,
	UINT dpi
);
WINUSERAPI BOOL WINAPI SystemParametersInfoForDpi(
	UINT uiAction,
	UINT uiParam,
	PVOID pvParam,
	UINT fWinIni,
	UINT dpi
);
WINUSERAPI int WINAPI GetSystemMetricsForDpi(
	int  nIndex,
	UINT dpi
);
WINUSERAPI DPI_AWARENESS_CONTEXT WINAPI SetThreadDpiAwarenessContext(
	DPI_AWARENESS_CONTEXT dpiContext
);

}


namespace TVTest
{

namespace
{


DPI_AWARENESS_CONTEXT MySetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT Context)
{
	if (Util::OS::IsWindows10AnniversaryUpdateOrLater()) {
		auto pSetThreadDpiAwarenessContext =
			GET_MODULE_FUNCTION(TEXT("user32.dll"), SetThreadDpiAwarenessContext);
		if (pSetThreadDpiAwarenessContext != nullptr)
			return pSetThreadDpiAwarenessContext(Context);
	}
	return nullptr;
}


}


bool EnableNonClientDPIScaling(HWND hwnd)
{
	if (Util::OS::IsWindows10AnniversaryUpdateOrLater()) {
		auto pEnableNonClientDpiScaling =
			GET_MODULE_FUNCTION(TEXT("user32.dll"), EnableNonClientDpiScaling);
		if (pEnableNonClientDpiScaling != nullptr)
			return pEnableNonClientDpiScaling(hwnd) != FALSE;
	}
	return false;
}


int GetSystemDPI()
{
	static int SystemDPI = 0;

	if (SystemDPI != 0)
		return SystemDPI;

	int DPI;

	auto pGetDpiForSystem = GET_MODULE_FUNCTION(TEXT("user32.dll"), GetDpiForSystem);
	if (pGetDpiForSystem != nullptr) {
		// GetDpiForSystem の結果はキャッシュしてはいけない
		DPI = pGetDpiForSystem();
	} else {
		if (SystemDPI == 0) {
			HDC hdc = ::GetDC(nullptr);
			if (hdc != nullptr) {
				SystemDPI = ::GetDeviceCaps(hdc, LOGPIXELSY);
				::ReleaseDC(nullptr, hdc);
			}
		}
		DPI = SystemDPI;
	}

	return DPI;
}


int GetMonitorDPI(HMONITOR hMonitor)
{
	if (hMonitor != nullptr && Util::OS::IsWindows8_1OrLater()) {
		HMODULE hLib = Util::LoadSystemLibrary(TEXT("shcore.dll"));
		if (hLib != nullptr) {
			UINT DpiX, DpiY;
			auto pGetDpiForMonitor = GET_LIBRARY_FUNCTION(hLib, GetDpiForMonitor);
			bool fOK =
				pGetDpiForMonitor != nullptr &&
				pGetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &DpiX, &DpiY) == S_OK;
			::FreeLibrary(hLib);

			if (fOK)
				return DpiY;
		}
	}

	return 0;
}


int GetWindowDPI(HWND hwnd)
{
	if (!::IsWindow(hwnd))
		return 0;

	if (Util::OS::IsWindows10AnniversaryUpdateOrLater()) {
		auto pGetDpiForWindow = GET_MODULE_FUNCTION(TEXT("user32.dll"), GetDpiForWindow);
		if (pGetDpiForWindow != nullptr)
			return pGetDpiForWindow(hwnd);
	}

	int DPI;

	HMONITOR hMonitor = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL);
	if (hMonitor != nullptr)
		DPI = GetMonitorDPI(hMonitor);
	else
		DPI = GetSystemDPI();

	return DPI;
}


int GetSystemMetricsWithDPI(int Index, int DPI, bool fFallbackScaling)
{
	if (Util::OS::IsWindows10AnniversaryUpdateOrLater()) {
		auto pGetSystemMetricsForDpi =
			GET_MODULE_FUNCTION(TEXT("user32.dll"), GetSystemMetricsForDpi);
		if (pGetSystemMetricsForDpi == nullptr)
			return pGetSystemMetricsForDpi(Index, DPI);
	}

	int Value = ::GetSystemMetrics(Index);
	if (fFallbackScaling && Value != 0)
		Value = ::MulDiv(Value, GetSystemDPI(), 96);

	return Value;
}


bool SystemParametersInfoWithDPI(UINT Action, UINT Param, void *pParam, UINT Flags, int DPI)
{
	if (Util::OS::IsWindows10AnniversaryUpdateOrLater()) {
		auto pSystemParametersInfoForDpi =
			GET_MODULE_FUNCTION(TEXT("user32.dll"), SystemParametersInfoForDpi);
		if (pSystemParametersInfoForDpi != nullptr)
			return pSystemParametersInfoForDpi(Action, Param, pParam, Flags, DPI) != FALSE;
	}

	return false;
}


bool AdjustWindowRectWithDPI(RECT *pRect, DWORD Style, DWORD ExStyle, bool fMenu, int DPI)
{
	if (Util::OS::IsWindows10AnniversaryUpdateOrLater()) {
		auto pAdjustWindowRectExForDpi =
			GET_MODULE_FUNCTION(TEXT("user32.dll"), AdjustWindowRectExForDpi);
		if (pAdjustWindowRectExForDpi != nullptr)
			return pAdjustWindowRectExForDpi(pRect, Style, fMenu, ExStyle, DPI) != FALSE;
	}

	return ::AdjustWindowRectEx(pRect, Style, fMenu, ExStyle) != FALSE;
}


DPIBlockBase::DPIBlockBase(DPI_AWARENESS_CONTEXT Context)
	: m_OldContext(MySetThreadDpiAwarenessContext(Context))
{
}

DPIBlockBase::~DPIBlockBase()
{
	if (m_OldContext != nullptr)
		MySetThreadDpiAwarenessContext(m_OldContext);
}


}
