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


#include "stdafx.h"
#include "TVTest.h"
#include "DPIUtil.h"
#include "Util.h"
#include <ShellScalingApi.h>
#include "Common/DebugDef.h"


extern "C"
{

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
WINUSERAPI DPI_AWARENESS_CONTEXT WINAPI GetThreadDpiAwarenessContext();
WINUSERAPI DPI_AWARENESS_CONTEXT WINAPI GetWindowDpiAwarenessContext(
	HWND hwnd
);
WINUSERAPI BOOL WINAPI AreDpiAwarenessContextsEqual(
	DPI_AWARENESS_CONTEXT dpiContextA,
	DPI_AWARENESS_CONTEXT dpiContextB
);

}


namespace TVTest
{

namespace
{


DPI_AWARENESS_CONTEXT MySetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT Context)
{
	if (Util::OS::IsWindows10AnniversaryUpdateOrLater()) {
		const auto pSetThreadDpiAwarenessContext =
			GET_MODULE_FUNCTION(TEXT("user32.dll"), SetThreadDpiAwarenessContext);
		if (pSetThreadDpiAwarenessContext != nullptr)
			return pSetThreadDpiAwarenessContext(Context);
	}
	return nullptr;
}


DPI_AWARENESS_CONTEXT MyGetThreadDpiAwarenessContext()
{
	if (Util::OS::IsWindows10AnniversaryUpdateOrLater()) {
		const auto pGetThreadDpiAwarenessContext =
			GET_MODULE_FUNCTION(TEXT("user32.dll"), GetThreadDpiAwarenessContext);
		if (pGetThreadDpiAwarenessContext != nullptr)
			return pGetThreadDpiAwarenessContext();
	}
	return nullptr;
}


bool MyAreDpiAwarenessContextsEqual(DPI_AWARENESS_CONTEXT Context1, DPI_AWARENESS_CONTEXT Context2)
{
	if (Util::OS::IsWindows10AnniversaryUpdateOrLater()) {
		const auto pAreDpiAwarenessContextsEqual =
			GET_MODULE_FUNCTION(TEXT("user32.dll"), AreDpiAwarenessContextsEqual);
		if (pAreDpiAwarenessContextsEqual != nullptr)
			return pAreDpiAwarenessContextsEqual(Context1, Context2) != FALSE;
	}
	return Context1 == Context2;
}


}


bool EnableNonClientDPIScaling(HWND hwnd)
{
	if (Util::OS::IsWindows10AnniversaryUpdateOrLater()) {
		const auto pEnableNonClientDpiScaling =
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

	const auto pGetDpiForSystem = GET_MODULE_FUNCTION(TEXT("user32.dll"), GetDpiForSystem);
	if (pGetDpiForSystem != nullptr) {
		// GetDpiForSystem の結果はキャッシュしてはいけない
		DPI = pGetDpiForSystem();
	} else {
		if (SystemDPI == 0) {
			const HDC hdc = ::GetDC(nullptr);
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
		const HMODULE hLib = Util::LoadSystemLibrary(TEXT("shcore.dll"));
		if (hLib != nullptr) {
			UINT DpiX, DpiY;
			const auto pGetDpiForMonitor = GET_LIBRARY_FUNCTION(hLib, GetDpiForMonitor);
			const bool fOK =
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
		const auto pGetDpiForWindow = GET_MODULE_FUNCTION(TEXT("user32.dll"), GetDpiForWindow);
		if (pGetDpiForWindow != nullptr)
			return pGetDpiForWindow(hwnd);
	}

	int DPI;

	const HMONITOR hMonitor = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL);
	if (hMonitor != nullptr)
		DPI = GetMonitorDPI(hMonitor);
	else
		DPI = GetSystemDPI();

	return DPI;
}


int GetSystemMetricsWithDPI(int Index, int DPI, bool fFallbackScaling)
{
	if (Util::OS::IsWindows10AnniversaryUpdateOrLater()) {
		const auto pGetSystemMetricsForDpi =
			GET_MODULE_FUNCTION(TEXT("user32.dll"), GetSystemMetricsForDpi);
		if (pGetSystemMetricsForDpi != nullptr)
			return pGetSystemMetricsForDpi(Index, DPI);
	}

	int Value = ::GetSystemMetrics(Index);
	if (fFallbackScaling && Value != 0)
		Value = ::MulDiv(Value, DPI, GetSystemDPI());

	return Value;
}


bool SystemParametersInfoWithDPI(UINT Action, UINT Param, void *pParam, UINT Flags, int DPI)
{
	if (Util::OS::IsWindows10AnniversaryUpdateOrLater()) {
		const auto pSystemParametersInfoForDpi =
			GET_MODULE_FUNCTION(TEXT("user32.dll"), SystemParametersInfoForDpi);
		if (pSystemParametersInfoForDpi != nullptr)
			return pSystemParametersInfoForDpi(Action, Param, pParam, Flags, DPI) != FALSE;
	}

	return false;
}


bool AdjustWindowRectWithDPI(RECT *pRect, DWORD Style, DWORD ExStyle, bool fMenu, int DPI)
{
	if (Util::OS::IsWindows10AnniversaryUpdateOrLater()) {
		const auto pAdjustWindowRectExForDpi =
			GET_MODULE_FUNCTION(TEXT("user32.dll"), AdjustWindowRectExForDpi);
		if (pAdjustWindowRectExForDpi != nullptr)
			return pAdjustWindowRectExForDpi(pRect, Style, fMenu, ExStyle, DPI) != FALSE;
	}

	return ::AdjustWindowRectEx(pRect, Style, fMenu, ExStyle) != FALSE;
}


int GetScrollBarWidth(HWND hwnd)
{
	if (IsWindowPerMonitorDPIV2(hwnd))
		return GetSystemMetricsWithDPI(SM_CXVSCROLL, GetWindowDPI(hwnd));

	return ::GetSystemMetrics(SM_CXVSCROLL);
}


DPI_AWARENESS_CONTEXT GetWindowDPIAwareness(HWND hwnd)
{
	if (Util::OS::IsWindows10AnniversaryUpdateOrLater()) {
		const auto pGetWindowDpiAwarenessContext = GET_MODULE_FUNCTION(TEXT("user32.dll"), GetWindowDpiAwarenessContext);
		if (pGetWindowDpiAwarenessContext != nullptr)
			return pGetWindowDpiAwarenessContext(hwnd);
	}

	if (Util::OS::IsWindows8_1OrLater()) {
		const HMODULE hLib = Util::LoadSystemLibrary(TEXT("shcore.dll"));
		if (hLib != nullptr) {
			const auto pGetProcessDpiAwareness = GET_LIBRARY_FUNCTION(hLib, GetProcessDpiAwareness);
			PROCESS_DPI_AWARENESS Awareness;
			if ((pGetProcessDpiAwareness != nullptr)
					&& (pGetProcessDpiAwareness(nullptr, &Awareness) == S_OK)) {
				::FreeLibrary(hLib);
				switch (Awareness) {
				case PROCESS_DPI_UNAWARE:           return DPI_AWARENESS_CONTEXT_UNAWARE;
				case PROCESS_SYSTEM_DPI_AWARE:      return DPI_AWARENESS_CONTEXT_SYSTEM_AWARE;
				case PROCESS_PER_MONITOR_DPI_AWARE: return DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE;
				}
			}
			::FreeLibrary(hLib);
		}
	}

	return nullptr;
}


bool IsWindowPerMonitorDPIV1(HWND hwnd)
{
	return MyAreDpiAwarenessContextsEqual(GetWindowDPIAwareness(hwnd), DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
}


bool IsWindowPerMonitorDPIV2(HWND hwnd)
{
	if (Util::OS::IsWindows10CreatorsUpdateOrLater()) {
		const auto pGetWindowDpiAwarenessContext = GET_MODULE_FUNCTION(TEXT("user32.dll"), GetWindowDpiAwarenessContext);
		if (pGetWindowDpiAwarenessContext != nullptr) {
			return MyAreDpiAwarenessContextsEqual(
				pGetWindowDpiAwarenessContext(hwnd), DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
		}
	}
	return false;
}


DPIBlockBase::DPIBlockBase(DPI_AWARENESS_CONTEXT Context)
	: m_OldContext((Context != nullptr) ? MySetThreadDpiAwarenessContext(Context) : nullptr)
{
}

DPIBlockBase::~DPIBlockBase()
{
	if (m_OldContext != nullptr)
		MySetThreadDpiAwarenessContext(m_OldContext);
}


CommonDialogDPIBlock::CommonDialogDPIBlock()
	: DPIBlockBase(
		MyAreDpiAwarenessContextsEqual(
			MyGetThreadDpiAwarenessContext(), DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) ?
			nullptr :
			DPI_AWARENESS_CONTEXT_SYSTEM_AWARE)
{
}


}
