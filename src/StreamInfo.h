/*
  TVTest
  Copyright(c) 2008-2019 DBCTRADO

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


#include "BasicWindow.h"
#include "UIBase.h"
#include "Settings.h"
#include "Dialog.h"


namespace TVTest
{

	class CStreamInfo
		: public CCustomWindow
		, public CUIBase
	{
	public:
		class CEventHandler
		{
		public:
			virtual ~CEventHandler() = default;

			virtual void OnRestoreSettings() {}
			virtual bool OnClose() { return true; }
		};

		static bool Initialize(HINSTANCE hinst);

		CStreamInfo();
		~CStreamInfo();

	// CBasicWindow
		bool Create(
			HWND hwndParent,
			DWORD Style = WS_OVERLAPPED | WS_SYSMENU | WS_THICKFRAME | WS_CLIPCHILDREN,
			DWORD ExStyle = 0, int ID = 0) override;

	// CStreamInfo
		void SetEventHandler(CEventHandler *pHandler);
		bool IsPositionSet() const;
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

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void ApplyStyle() override;
		void RealizeStyle() override;

	// CStreamInfo
		void OnSizeChanged(int Width, int Height);
		bool CreatePage(int Page);
		bool SetPage(int Page);
		void GetPagePosition(RECT *pPosition) const;

		PageInfo m_PageList[NUM_PAGES];
		int m_CurrentPage;
		HWND m_hwndTab;
		DrawUtil::CFont m_TabFont;
		CEventHandler *m_pEventHandler;
		bool m_fCreateFirst;
		SIZE m_DefaultPageSize;
		Style::CStyleScaling m_StyleScaling;

		static HINSTANCE m_hinst;
	};

}	// namespace TVTest


#endif
