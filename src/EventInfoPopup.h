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


#ifndef TVTEST_EVENT_INFO_POPUP_H
#define TVTEST_EVENT_INFO_POPUP_H


#include "BasicWindow.h"
#include "UIBase.h"
#include "DrawUtil.h"
#include "RichEditUtil.h"
#include "GUIUtil.h"
#include "WindowUtil.h"
#include "EpgUtil.h"
#include "LibISDB/LibISDB/EPG/EventInfo.hpp"


namespace TVTest
{

	class CEventInfoPopup
		: protected CCustomWindow
		, public CUIBase
	{
	public:
		class ABSTRACT_CLASS(CEventHandler)
		{
		protected:
			CEventInfoPopup *m_pPopup = nullptr;

		public:
			static constexpr int COMMAND_FIRST = 100;

			virtual ~CEventHandler();

			virtual bool OnMenuPopup(HMENU hmenu) { return true; }
			virtual void OnMenuSelected(int Command) {}
			friend class CEventInfoPopup;
		};

		CEventInfoPopup();
		~CEventInfoPopup();

		bool Show(
			const LibISDB::EventInfo *pEventInfo, const RECT *pPos = nullptr,
			HICON hIcon = nullptr, LPCTSTR pszTitle = nullptr);
		bool Hide();
		bool IsVisible();
		bool IsOwnWindow(HWND hwnd) const;
		void GetSize(int *pWidth, int *pHeight) const;
		bool SetSize(int Width, int Height);
		void SetTheme(const Theme::CThemeManager *pThemeManager) override;
		bool SetFont(const Style::Font &Font);
		void SetEventHandler(CEventHandler *pEventHandler);
		bool IsSelected() const;
		String GetSelectedText() const;
		void GetPreferredIconSize(int *pWidth, int *pHeight) const;
		bool GetPopupPosition(int x, int y, RECT *pPos) const;
		bool AdjustPopupPosition(POINT *pPos) const;
		bool GetDefaultPopupPosition(RECT *pPos) const;
		bool GetDefaultPopupPosition(POINT *pPos) const;

		static bool Initialize(HINSTANCE hinst);

	private:
		LibISDB::EventInfo m_EventInfo;
		HWND m_hwndEdit = nullptr;
		CRichEditUtil m_RichEditUtil;
		CRichEditLinkHandler m_RichEditLink;
		Style::CStyleScaling m_StyleScaling;
		CEpgTheme m_EpgTheme;
		Theme::ThemeColor m_BackColor{::GetSysColor(COLOR_WINDOW)};
		Theme::ThemeColor m_TextColor{::GetSysColor(COLOR_WINDOWTEXT)};
		Theme::ThemeColor m_EventTitleColor{m_TextColor};
		Theme::ThemeColor m_TitleBackColor{RGB(228, 228, 240)};
		Theme::ThemeColor m_TitleTextColor{RGB(80, 80, 80)};
		Style::Font m_StyleFont;
		DrawUtil::CFont m_Font;
		DrawUtil::CFont m_TitleFont;
		int m_TitleHeight = 0;
		int m_TitleLeftMargin = 2;
		int m_TitleIconTextMargin = 4;
		int m_ButtonSize = 14;
		int m_ButtonMargin = 3;
		bool m_fDetailInfo =
#ifdef _DEBUG
			true
#else
			false
#endif
			;
		String m_TitleText;
		CIcon m_TitleIcon;
		CEventHandler *m_pEventHandler = nullptr;
		bool m_fCursorInWindow = false;
		bool m_fMenuShowing = false;

		static const LPCTSTR m_pszWindowClass;
		static HINSTANCE m_hinst;
		static constexpr UINT TIMER_ID_HIDE = 1;

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void ApplyStyle() override;
		void RealizeStyle() override;

		bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID) override;
		void SetEventInfo(const LibISDB::EventInfo *pEventInfo);
		void UpdateEventInfo();
		void FormatAudioInfo(
			const LibISDB::EventInfo::AudioInfo *pAudioInfo,
			LPTSTR pszText, int MaxLength) const;
		void CalcTitleHeight();
		void GetCloseButtonRect(RECT *pRect) const;
		void SetNcRendering();
		void ShowContextMenu();
	};

	class CEventInfoPopupManager
		: protected CWindowSubclass
	{
	public:
		class ABSTRACT_CLASS(CEventHandler)
		{
		protected:
			CEventInfoPopup *m_pPopup = nullptr;

		public:
			virtual ~CEventHandler() = default;

			virtual bool HitTest(int x, int y, LPARAM * pParam) = 0;
			virtual bool ShowPopup(LPARAM Param, CEventInfoPopup * pPopup) = 0;
			friend class CEventInfoPopupManager;
		};

		CEventInfoPopupManager(CEventInfoPopup *pPopup);
		~CEventInfoPopupManager();

		bool Initialize(HWND hwnd, CEventHandler *pEventHandler);
		void Finalize();
		bool SetEnable(bool fEnable);
		bool GetEnable() const { return m_fEnable; }
		bool Popup(int x, int y);

	private:
		CEventInfoPopup *m_pPopup;
		bool m_fEnable = true;
		CEventHandler *m_pEventHandler = nullptr;
		bool m_fTrackMouseEvent = false;
		LPARAM m_HitTestParam = -1;

		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	};

} // namespace TVTest


#endif
