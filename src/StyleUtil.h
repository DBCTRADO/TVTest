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


#endif	// ndef TVTEST_STYLE_UTIL_H
