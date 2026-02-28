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


#ifndef TVTEST_OPTION_DIALOG_H
#define TVTEST_OPTION_DIALOG_H


#include "Options.h"
#include "DrawUtil.h"


namespace TVTest
{

	class COptionDialog
		: public COptionFrame
		, public CResizableDialog
	{
	public:
		enum {
			PAGE_GENERAL,
			PAGE_VIEW,
			PAGE_OSD,
			PAGE_NOTIFICATIONBAR,
			PAGE_STATUS,
			PAGE_SIDEBAR,
			PAGE_MENU,
			PAGE_PANEL,
			PAGE_COLORSCHEME,
			PAGE_OPERATION,
			PAGE_ACCELERATOR,
			PAGE_CONTROLLER,
			PAGE_DRIVER,
			PAGE_VIDEO,
			PAGE_AUDIO,
			PAGE_PLAYBACK,
			PAGE_RECORD,
			PAGE_CAPTURE,
			PAGE_CHANNELSCAN,
			PAGE_EPG,
			PAGE_PROGRAMGUIDE,
			PAGE_PLUGIN,
			PAGE_TSPROCESSOR,
			PAGE_LOG,
			PAGE_LAST = PAGE_LOG
		};

		~COptionDialog();

		bool Show(HWND hwndOwner, int StartPage = -1);
		int GetCurrentPage() const { return m_CurrentPage; }
		bool SetCurrentPage(int Page);

	private:
		static constexpr int NUM_PAGES = PAGE_LAST + 1;

		struct PageInfo
		{
			LPCTSTR pszTitle;
			COptions *pOptions;
			int HelpID;
		};

		static bool m_fInitialized;
		static PageInfo m_PageList[NUM_PAGES];

		int m_CurrentPage = 0;
		int m_StartPage = -1;
		HIMAGELIST m_himlIcons = nullptr;
		DrawUtil::CFont m_TitleFont;
		bool m_fSettingError = false;
		bool m_fApplied = false;
		int m_IconWidth;
		int m_IconHeight;
		int m_ListMargin;
		int m_IconTextMargin;
		int m_ListItemHeight;

		void Initialize();
		void CreatePage(int Page);
		void SetPage(int Page);
		void SetPagePos(int Page);
		COLORREF GetTitleColor(int Page) const;

	// COptionFrame
		void ActivatePage(COptions *pOptions) override;
		void OnSettingError(COptions *pOptions) override;

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void ApplyStyle() override;
		void RealizeStyle() override;
	};

} // namespace TVTest


#endif
