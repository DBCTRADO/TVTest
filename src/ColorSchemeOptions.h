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


#ifndef TVTEST_COLOR_SCHEME_OPTIONS_H
#define TVTEST_COLOR_SCHEME_OPTIONS_H


#include "ColorScheme.h"
#include "Options.h"
#include "ColorPalette.h"
#include <memory>


namespace TVTest
{

	class CColorSchemeOptions
		: public COptions
	{
	public:
		class ABSTRACT_CLASS(CEventHandler)
		{
		public:
			virtual bool ApplyColorScheme(const CColorScheme * pColorScheme) = 0;
		};

		CColorSchemeOptions();
		~CColorSchemeOptions();

	// COptions
		bool LoadSettings(CSettings &Settings) override;
		bool SaveSettings(CSettings &Settings) override;

	// CBasicDialog
		bool Create(HWND hwndOwner) override;

	// CColorSchemeOptions
		bool SetEventHandler(CEventHandler *pEventHandler);
		bool ApplyColorScheme() const;
		const CColorScheme *GetColorScheme() const { return m_ColorScheme.get(); }
		COLORREF GetColor(int Type) const;
		COLORREF GetColor(LPCTSTR pszText) const;
		static bool GetThemesDirectory(CFilePath *pDirectory, bool fCreate = false);

	private:
		std::unique_ptr<CColorScheme> m_ColorScheme;
		CColorSchemeList m_PresetList;
		CColorScheme::GradientStyle m_GradientList[CColorScheme::NUM_GRADIENTS];
		Theme::BorderType m_BorderList[CColorScheme::NUM_BORDERS];
		std::unique_ptr<CColorScheme> m_PreviewColorScheme;
		bool m_fPreview = false;
		CEventHandler *m_pEventHandler = nullptr;
		CColorPalette m_ColorPalette;
		int m_ColorListMargin = 0;

		static const LPCTSTR m_pszExtension;

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		void OnDarkModeChanged(bool fDarkMode) override;

	// CUIBase
		void ApplyStyle() override;
		void RealizeStyle() override;

		bool Apply(const CColorScheme *pColorScheme) const;
		void GetCurrentSettings(CColorScheme *pColorScheme);
		void SetListItemSize();

		static INT_PTR CALLBACK SaveDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	};

}	// namespace TVTest


#endif
