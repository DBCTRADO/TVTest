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
#include "StyleUtil.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace StyleUtil
{


bool GetDefaultUIFont(Style::Font *pFont)
{
	if (pFont == nullptr)
		return false;

	if (!DrawUtil::GetDefaultUIFont(&pFont->LogFont))
		return false;

	Style::CStyleManager::AssignFontSizeFromLogFont(pFont);

	return true;
}


bool GetSystemFont(DrawUtil::FontType Type, Style::Font *pFont)
{
	if (pFont == nullptr)
		return false;

	if (!DrawUtil::GetSystemFont(Type, &pFont->LogFont))
		return false;

	Style::CStyleManager::AssignFontSizeFromLogFont(pFont);

	return true;
}


void SetFontInfoItem(HWND hDlg, int ID, const Style::Font &Font)
{
	if (hDlg==nullptr)
		return;

	int Size;
	TCHAR szText[LF_FACESIZE+16];

	if (Font.Size.Unit == Style::UnitType::Point && Font.Size.Value != 0) {
		Size = Font.Size.Value;
	} else {
		const HDC hdc = ::GetDC(hDlg);
		Size = CalcFontPointHeight(hdc, &Font.LogFont);
		::ReleaseDC(hDlg, hdc);
	}
	StringFormat(
		szText, TEXT("{}, {} pt"),
		Font.LogFont.lfFaceName, Size);
	SetDlgItemText(hDlg, ID, szText);
}


bool ChooseStyleFont(HWND hwndOwner, Style::Font *pFont)
{
	if (pFont == nullptr)
		return false;

	if (!ChooseFontDialog(hwndOwner, &pFont->LogFont, &pFont->Size.Value))
		return false;

	pFont->Size.Value /= 10;
	pFont->Size.Unit = Style::UnitType::Point;

	return true;
}


bool ReadFontSettings(
	CSettings &Settings, LPCTSTR pszValueName, Style::Font *pFont,
	bool fCompatible, bool *pfPointSize)
{
	if (pfPointSize != nullptr)
		*pfPointSize = false;

	if (pFont == nullptr)
		return false;

	String Key;

	if (!Settings.Read(pszValueName, &pFont->LogFont) && fCompatible) {
		// 旧バージョンとの互換用
		TCHAR szFont[LF_FACESIZE];
		int Value;

		Key = pszValueName;
		Key += TEXT("Name");
		if (Settings.Read(Key.c_str(), szFont, LF_FACESIZE)
				&& szFont[0] != _T('\0')) {
			StringCopy(pFont->LogFont.lfFaceName, szFont);
			pFont->LogFont.lfEscapement = 0;
			pFont->LogFont.lfOrientation = 0;
			pFont->LogFont.lfUnderline = 0;
			pFont->LogFont.lfStrikeOut = 0;
			pFont->LogFont.lfCharSet = DEFAULT_CHARSET;
			pFont->LogFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
			pFont->LogFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
			pFont->LogFont.lfQuality = DRAFT_QUALITY;
			pFont->LogFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
		}

		Key = pszValueName;
		Key += TEXT("Size");
		if (Settings.Read(Key.c_str(), &Value)) {
			pFont->LogFont.lfHeight = Value;
			pFont->LogFont.lfWidth = 0;
		}

		Key = pszValueName;
		Key += TEXT("Weight");
		if (Settings.Read(TEXT("NotificationBarFontWeight"), &Value))
			pFont->LogFont.lfWeight = Value;

		Key = pszValueName;
		Key += TEXT("Italic");
		if (Settings.Read(TEXT("NotificationBarFontItalic"), &Value))
			pFont->LogFont.lfItalic = Value;
	}

	Key = pszValueName;
	Key += TEXT(".PointSize");
	if (ReadPointSize(Settings, Key.c_str(), pFont)) {
		if (pfPointSize != nullptr)
			*pfPointSize = true;
	} else {
		Style::CStyleManager::AssignFontSizeFromLogFont(pFont);
	}

	return true;
}


bool WriteFontSettings(CSettings &Settings, LPCTSTR pszValueName, const Style::Font &Font)
{
	if (!Settings.Write(pszValueName, &Font.LogFont))
		return false;

	String Key(pszValueName);
	Key += TEXT(".PointSize");
	WritePointSize(Settings, Key.c_str(), Font);

	return true;
}


bool ReadPointSize(CSettings &Settings, LPCTSTR pszValueName, Style::Font *pFont)
{
	if (pFont == nullptr)
		return false;

	if (!Settings.Read(pszValueName, &pFont->Size.Value))
		return false;

	if (pFont->Size.Value != 0)
		pFont->Size.Unit = Style::UnitType::Point;
	else
		pFont->Size.Unit = Style::UnitType::Undefined;

	return true;
}


bool WritePointSize(CSettings &Settings, LPCTSTR pszValueName, const Style::Font &Font)
{
	return Settings.Write(pszValueName, Font.Size.Unit == Style::UnitType::Point ? Font.Size.Value : 0);
}


} // namespace StyleUtil

} // namespace TVTest
