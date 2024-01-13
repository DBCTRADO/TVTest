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


#ifndef TVTEST_STYLE_UTIL_H
#define TVTEST_STYLE_UTIL_H


#include "Style.h"
#include "DrawUtil.h"
#include "Settings.h"


namespace TVTest
{

	namespace StyleUtil
	{

		bool GetDefaultUIFont(Style::Font *pFont);
		bool GetSystemFont(DrawUtil::FontType Type, Style::Font *pFont);
		void SetFontInfoItem(HWND hDlg, int ID, const Style::Font &Font);
		bool ChooseStyleFont(HWND hwndOwner, Style::Font *pFont);

		bool ReadFontSettings(
			CSettings &Settings, LPCTSTR pszValueName, Style::Font *pFont,
			bool fCompatible = false, bool *pfPointSize = nullptr);
		bool WriteFontSettings(CSettings &Settings, LPCTSTR pszValueName, const Style::Font &Font);
		bool ReadPointSize(CSettings &Settings, LPCTSTR pszValueName, Style::Font *pFont);
		bool WritePointSize(CSettings &Settings, LPCTSTR pszValueName, const Style::Font &Font);

	}

}


#endif // ndef TVTEST_STYLE_UTIL_H
