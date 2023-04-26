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


#ifndef TVTEST_STREAM_INFO_H
#define TVTEST_STREAM_INFO_H


#include "Settings.h"
#include "Dialog.h"


namespace TVTest
{

	class CStreamInfo
		: public CResizableDialog
	{
	public:
		class CEventHandler
		{
		public:
			virtual ~CEventHandler() = default;

			virtual void OnRestoreSettings() {}
			virtual void OnClose() {}
		};

		CStreamInfo();
		~CStreamInfo();

	// CBasicDialog
		bool Create(HWND hwndOwner) override;

	// CStreamInfo
		void SetEventHandler(CEventHandler *pHandler);
		void LoadSettings(CSettings &Settings);
		void SaveSettings(CSettings &Settings) const;

	private:
		struct PageInfo
		{
			LPCTSTR pszTitle;
			std::unique_ptr<CResizableDialog> Dialog;
		};

		enum {
			PAGE_STREAMINFO,
			PAGE_PIDINFO,
			NUM_PAGES
		};

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CStreamInfo
		void OnSizeChanged(int Width, int Height);
		bool CreatePage(int Page);
		bool SetPage(int Page);
		void GetPagePosition(RECT *pPosition) const;

		PageInfo m_PageList[NUM_PAGES];
		int m_CurrentPage = 0;
		CEventHandler *m_pEventHandler = nullptr;
		bool m_fCreateFirst = true;
		SIZE m_DefaultPageSize{};
	};

}	// namespace TVTest


#endif
