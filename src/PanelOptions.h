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


#ifndef TVTEST_PANEL_OPTIONS_H
#define TVTEST_PANEL_OPTIONS_H


#include "Options.h"
#include "Panel.h"
#include "PanelForm.h"
#include "ListView.h"
#include <vector>


namespace TVTest
{

	enum {
		PANEL_ID_INFORMATION,
		PANEL_ID_PROGRAMLIST,
		PANEL_ID_CHANNEL,
		PANEL_ID_CONTROL,
		PANEL_ID_CAPTION,
		NUM_PANELS,
		PANEL_ID_FIRST = PANEL_ID_INFORMATION,
		PANEL_ID_LAST  = PANEL_ID_CAPTION
	};

	class CPanelOptions
		: public COptions
	{
	public:
		CPanelOptions();
		~CPanelOptions();

	// CSettingsBase
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	// CBasicDialog
		bool Create(HWND hwndOwner) override;

	// CPanelOptions
		bool InitializePanelForm(CPanelForm *pPanelForm);
		bool GetSnapAtMainWindow() const { return m_fSnapAtMainWindow; }
		int GetSnapMargin() const { return m_SnapMargin; }
		bool GetAttachToMainWindow() const { return m_fAttachToMainWindow; }
		const Style::Font &GetFont() const { return m_Font; }
		int GetInitialTab() const;
		CPanelForm::TabStyle GetTabStyle() const { return m_TabStyle; }
		bool GetProgramInfoUseRichEdit() const { return m_fProgramInfoUseRichEdit; }
		int RegisterPanelItem(LPCTSTR pszID, LPCTSTR pszTitle);
		bool SetPanelItemVisibility(int ID, bool fVisible);
		bool GetPanelItemVisibility(int ID) const;
		bool ApplyItemList(CPanelForm *pPanelForm) const;

	private:
		struct PanelItemInfo
		{
			String ID;
			String Title;
			bool fVisible;
		};

		typedef std::vector<PanelItemInfo> PanelItemInfoList;

		static bool CompareID(const String &ID1, const String &ID2)
		{
			return StringUtility::CompareNoCase(ID1, ID2) == 0;
		}

		bool m_fSnapAtMainWindow = true;
		int m_SnapMargin = 4;
		bool m_fAttachToMainWindow = true;
		int m_Opacity = 100;
		Style::Font m_Font;
		Style::Font m_CurSettingFont;
		bool m_fSpecCaptionFont = true;
		Style::Font m_CaptionFont;
		Style::Font m_CurSettingCaptionFont;
		String m_InitialTab;
		String m_LastTab;
		PanelItemInfoList m_AvailItemList;
		PanelItemInfoList m_ItemList;
		CPanelForm::TabStyle m_TabStyle = CPanelForm::TabStyle::TextOnly;
		bool m_fTabTooltip = true;
		bool m_fProgramInfoUseRichEdit = true;
		CListView m_ItemListView;

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		void UpdateItemListControlsState();
		int GetItemIDFromIDText(const String &IDText) const;
	};

}	// namespace TVTest


#endif
