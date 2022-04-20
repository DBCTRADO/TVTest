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
#include "DarkMode.h"
#include "Util.h"
#include "Common/DebugDef.h"


namespace TVTest
{


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


}	// namespace TVTest
