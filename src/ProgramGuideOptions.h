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


#ifndef TVTEST_PROGRAM_GUIDE_OPTIONS_H
#define TVTEST_PROGRAM_GUIDE_OPTIONS_H


#include "ProgramGuide.h"
#include "Options.h"
#include "Plugin.h"
#include "Tooltip.h"


namespace TVTest
{

	class CProgramGuideOptions
		: public COptions
	{
	public:
		enum {
			UPDATE_EVENTICONS = 0x00000001UL,
			UPDATE_ARIBSYMBOL = 0x00000002UL
		};

		CProgramGuideOptions(CProgramGuide *pProgramGuide, CPluginManager *pPluginManager);
		~CProgramGuideOptions();

	// CSettingsBase
		bool LoadSettings(CSettings &Settings) override;
		bool SaveSettings(CSettings &Settings) override;

	// CBasicDialog
		bool Create(HWND hwndOwner) override;

	// CProgramGuideOptions
		bool GetTimeRange(LibISDB::DateTime *pFirst, LibISDB::DateTime *pLast);
		bool GetOnScreen() const { return m_fOnScreen; }
		bool ScrollToCurChannel() const { return m_fScrollToCurChannel; }
		const Style::Font &GetFont() const { return m_Font; }
		UINT GetVisibleEventIcons() const { return m_VisibleEventIcons; }
		bool GetUseARIBSymbol() const { return m_fUseARIBSymbol; }
		LPCTSTR GetProgramLDoubleClickCommand() const { return StringUtility::GetCStrOrNull(m_ProgramLDoubleClickCommand); }
		int ParseCommand(LPCTSTR pszCommand) const;

	private:
		static constexpr int MIN_VIEW_HOURS = 1;
		static constexpr int MAX_VIEW_HOURS = 24 * 8;

		CProgramGuide *m_pProgramGuide;
		CPluginManager *m_pPluginManager;
		bool m_fOnScreen = false;
		bool m_fScrollToCurChannel = false;
		int m_BeginHour = -1;
		int m_ViewHours = 26;
		int m_ItemWidth;
		int m_LinesPerHour;
		Style::Font m_Font;
		Style::Font m_CurSettingFont;
		bool m_fUseDirectWrite = false;
		CDirectWriteRenderer::RenderingParams m_DirectWriteRenderingParams;
		bool m_fUseARIBSymbol = false;
		UINT m_VisibleEventIcons;
		CProgramGuideToolList m_ToolList;
		int m_WheelScrollLines;
		String m_ProgramLDoubleClickCommand;
		CTooltip m_Tooltip;

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void RealizeStyle() override;

		void SetDlgItemState();
		void DeleteAllTools();
	};

}	// namespace TVTest


#endif
