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


#ifndef TVTEST_DARK_MODE_H
#define TVTEST_DARK_MODE_H


#include "Theme.h"


namespace TVTest
{

	bool IsDarkThemeSupported();
	bool IsDarkThemeColor(const Theme::ThemeColor &Color);
	inline bool IsDarkThemeStyle(const Theme::FillStyle &Style)
	{
		return IsDarkThemeColor(Style.GetSolidColor());
	}
	inline bool IsDarkThemeStyle(const Theme::BackgroundStyle &Style)
	{
		return IsDarkThemeStyle(Style.Fill);
	}
	bool SetWindowDarkTheme(HWND hwnd, bool fDark);
	bool IsDarkAppModeSupported();
	bool SetAppAllowDarkMode(bool fAllow);
	bool SetWindowAllowDarkMode(HWND hwnd, bool fAllow);
	bool SetWindowFrameDarkMode(HWND hwnd, bool fDarkMode);
	bool IsDarkMode();
	bool IsHighContrast();
	bool IsDarkModeSettingChanged(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

} // namespace TVTest


#endif
