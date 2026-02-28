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
#include <dwmapi.h>
#include "TVTest.h"
#include "DarkMode.h"
#include "Util.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace
{

enum class PreferredAppMode {
	Default,
	AllowDark,
	ForceDark,
	ForceLight,
};


BOOLEAN WINAPI ShouldAppsUseDarkMode()
{
	if (!Util::OS::IsWindows10RS5OrLater())
		return FALSE;

	const auto pShouldAppsUseDarkMode =
		GET_MODULE_FUNCTION_ORDINAL(TEXT("uxtheme.dll"), ShouldAppsUseDarkMode, 132);
	if (pShouldAppsUseDarkMode == nullptr)
		return FALSE;

	return pShouldAppsUseDarkMode();
}


BOOL WINAPI AllowDarkModeForWindow(HWND hwnd, BOOL fAllow)
{
	if (!Util::OS::IsWindows10RS5OrLater())
		return FALSE;

	const auto pAllowDarkModeForWindow =
		GET_MODULE_FUNCTION_ORDINAL(TEXT("uxtheme.dll"), AllowDarkModeForWindow, 133);
	if (pAllowDarkModeForWindow == nullptr)
		return FALSE;

	pAllowDarkModeForWindow(hwnd, fAllow);

	return TRUE;
}


void WINAPI FlushMenuThemes()
{
	if (!Util::OS::IsWindows10RS5OrLater())
		return;

	const auto pFlushMenuThemes =
		GET_MODULE_FUNCTION_ORDINAL(TEXT("uxtheme.dll"), FlushMenuThemes, 136);
	if (pFlushMenuThemes == nullptr)
		return;

	pFlushMenuThemes();
}


void WINAPI RefreshImmersiveColorPolicyState()
{
	if (!Util::OS::IsWindows10RS5OrLater())
		return;

	const auto pRefreshImmersiveColorPolicyState =
		GET_MODULE_FUNCTION_ORDINAL(TEXT("uxtheme.dll"), RefreshImmersiveColorPolicyState, 104);
	if (pRefreshImmersiveColorPolicyState == nullptr)
		return;

	pRefreshImmersiveColorPolicyState();
}


BOOL WINAPI IsDarkModeAllowedForWindow(HWND hwnd)
{
	if (!Util::OS::IsWindows10RS5OrLater())
		return FALSE;

	const auto pIsDarkModeAllowedForWindow =
		GET_MODULE_FUNCTION_ORDINAL(TEXT("uxtheme.dll"), IsDarkModeAllowedForWindow, 137);
	if (pIsDarkModeAllowedForWindow == nullptr)
		return FALSE;

	return pIsDarkModeAllowedForWindow(hwnd);
}


BOOL WINAPI ShouldSystemUseDarkMode()
{
	if (!Util::OS::IsWindows10_19H1OrLater())
		return FALSE;

	const auto pShouldSystemUseDarkMode =
		GET_MODULE_FUNCTION_ORDINAL(TEXT("uxtheme.dll"), ShouldSystemUseDarkMode, 138);
	if (pShouldSystemUseDarkMode == nullptr)
		return FALSE;

	return pShouldSystemUseDarkMode();
}


PreferredAppMode WINAPI SetPreferredAppMode(PreferredAppMode Mode)
{
	if (!Util::OS::IsWindows10_19H1OrLater())
		return PreferredAppMode::Default;

	const auto pSetPreferredAppMode =
		GET_MODULE_FUNCTION_ORDINAL(TEXT("uxtheme.dll"), SetPreferredAppMode, 135);
	if (pSetPreferredAppMode == nullptr)
		return PreferredAppMode::Default;

	return pSetPreferredAppMode(Mode);
}


} // namespace


bool IsDarkThemeSupported()
{
	return Util::OS::IsWindows10RS5OrLater();
}


bool IsDarkThemeColor(const Theme::ThemeColor &Color)
{
	return Color.Red * 299 + Color.Green * 587 + Color.Blue * 114 < 255 * 500;
}


bool SetWindowDarkTheme(HWND hwnd, bool fDark)
{
	if (!IsDarkThemeSupported())
		return false;

	return SUCCEEDED(::SetWindowTheme(hwnd, fDark ? L"DarkMode_Explorer" : nullptr, nullptr));
}


bool IsDarkAppModeSupported()
{
	return Util::OS::IsWindows10_19H1OrLater();
}


bool SetAppAllowDarkMode(bool fAllow)
{
	if (!IsDarkAppModeSupported())
		return false;

	SetPreferredAppMode(
		fAllow ?
			PreferredAppMode::AllowDark :
			PreferredAppMode::Default);
	//FlushMenuThemes();
	RefreshImmersiveColorPolicyState();

	return true;
}


bool SetWindowAllowDarkMode(HWND hwnd, bool fAllow)
{
	if (!::IsWindow(hwnd))
		return false;

	return AllowDarkModeForWindow(hwnd, fAllow) != FALSE;
}


bool SetWindowFrameDarkMode(HWND hwnd, bool fDarkMode)
{
	if (!Util::OS::IsWindows10RS5OrLater())
		return false;
	if (!::IsWindow(hwnd))
		return false;

	const BOOL fDark = fDarkMode;

	// DWMWA_USE_IMMERSIVE_DARK_MODE = 20
	if (SUCCEEDED(::DwmSetWindowAttribute(hwnd, 20, &fDark, sizeof(fDark))))
		return true;
	return !Util::OS::IsWindows10_20H1OrLater()
		&& SUCCEEDED(::DwmSetWindowAttribute(hwnd, 19, &fDark, sizeof(fDark)));
}


bool IsDarkMode()
{
	return !IsHighContrast() && ShouldAppsUseDarkMode();
}


bool IsHighContrast()
{
	HIGHCONTRAST HighContrast = {sizeof(HIGHCONTRAST)};

	return ::SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(HighContrast), &HighContrast, FALSE)
		&& !!(HighContrast.dwFlags & HCF_HIGHCONTRASTON);
}


bool IsDarkModeSettingChanged(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (Message == WM_SETTINGCHANGE) {
		LPCTSTR pszName = reinterpret_cast<LPCTSTR>(lParam);

		if (pszName != nullptr && ::lstrcmpi(pszName, TEXT("ImmersiveColorSet")) == 0)
			return true;
	}

	return false;
}


} // namespace TVTest
