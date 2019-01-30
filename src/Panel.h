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


#ifndef TVTEST_PANEL_H
#define TVTEST_PANEL_H


#include "Layout.h"
#include "UIBase.h"
#include "DrawUtil.h"
#include "Theme.h"


namespace TVTest
{

	class CPanelContent
		: public CCustomWindow
		, public CUIBase
	{
	};

	class CPanel
		: public CCustomWindow
		, public CUIBase
	{
	public:
		class ABSTRACT_CLASS(CEventHandler)
		{
		public:
			virtual ~CEventHandler() = default;

			virtual bool OnFloating() { return false; }
			virtual bool OnClose() { return false; }
			virtual bool OnEnterSizeMove() { return false; }
			virtual bool OnMoving(RECT * pRect) { return false; }
			virtual bool OnKeyDown(UINT KeyCode, UINT Flags) { return false; }
			virtual void OnSizeChanged(int Width, int Height) {}
			virtual bool OnMenuPopup(HMENU hmenu) { return true; }
			virtual bool OnMenuSelected(int Command) { return false; }
		};

		static constexpr int MENU_USER = 100;

		struct PanelTheme
		{
			Theme::Style TitleStyle;
			Theme::Style TitleIconStyle;
			Theme::Style TitleIconHighlightStyle;
		};

		static bool Initialize(HINSTANCE hinst);

		CPanel();
		~CPanel();

	// CBasicWindow
		bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;

	// CUIBase
		void SetStyle(const Style::CStyleManager *pStyleManager) override;
		void NormalizeStyle(
			const Style::CStyleManager *pStyleManager,
			const Style::CStyleScaling *pStyleScaling) override;
		void SetTheme(const Theme::CThemeManager *pThemeManager) override;

	// CPanel
		bool SetWindow(CPanelContent *pContent, LPCTSTR pszTitle);
		void ShowTitle(bool fShow);
		void EnableFloating(bool fEnable);
		void SetEventHandler(CEventHandler *pHandler);
		CBasicWindow *GetWindow() { return m_pContent; }
		bool SetPanelTheme(const PanelTheme &Theme);
		bool GetPanelTheme(PanelTheme *pTheme) const;
		int GetTitleHeight() const { return m_TitleHeight; }
		bool GetTitleRect(RECT *pRect) const;
		bool GetContentRect(RECT *pRect) const;
		bool SetTitleFont(const Style::Font &Font);

	private:
		struct PanelStyle
		{
			Style::Margins TitlePadding;
			Style::Margins TitleLabelMargin;
			Style::IntValue TitleLabelExtraHeight;
			Style::Size TitleButtonIconSize;
			Style::Margins TitleButtonPadding;

			PanelStyle();
			void SetStyle(const Style::CStyleManager *pStyleManager);
			void NormalizeStyle(
				const Style::CStyleManager *pStyleManager,
				const Style::CStyleScaling *pStyleScaling);
		};

		enum class ItemType {
			None,
			Close,
		};

		Style::Font m_StyleFont;
		DrawUtil::CFont m_Font;
		DrawUtil::CFont m_IconFont;
		int m_FontHeight;
		int m_TitleHeight;
		CPanelContent *m_pContent;
		String m_Title;
		bool m_fShowTitle;
		bool m_fEnableFloating;
		PanelStyle m_Style;
		PanelTheme m_Theme;
		CEventHandler *m_pEventHandler;
		ItemType m_HotItem;
		bool m_fCloseButtonPushed;
		POINT m_ptDragStartPos;
		POINT m_ptMovingWindowPos;

		static HINSTANCE m_hinst;

		void CalcDimensions();
		void Draw(HDC hdc, const RECT &PaintRect) const;
		void OnSize(int Width, int Height);
		void GetCloseButtonRect(RECT *pRect) const;
		void SetHotItem(ItemType Item);

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void ApplyStyle() override;
		void RealizeStyle() override;
	};

	class CDropHelper
		: public CCustomWindow
	{
		int m_Opacity;
		static HINSTANCE m_hinst;

	// CBasicWindow
		bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	public:
		static bool Initialize(HINSTANCE hinst);

		CDropHelper();
		~CDropHelper();

		bool Show(const RECT *pRect);
		bool Hide();
	};

	class CPanelFrame
		: public CCustomWindow
		, public CUIBase
		, public CPanel::CEventHandler
	{
	public:
		enum class DockingPlace {
			None,
			Left,
			Right,
			Top,
			Bottom,
		};

		class ABSTRACT_CLASS(CEventHandler)
		{
		public:
			virtual ~CEventHandler() = default;

			virtual bool OnClose() { return true; }
			virtual bool OnMoving(RECT * pRect) { return false; }
			virtual bool OnEnterSizeMove() { return false; }
			virtual bool OnKeyDown(UINT KeyCode, UINT Flags) { return false; }
			virtual bool OnMouseWheel(WPARAM wParam, LPARAM lParam) { return false; }
			virtual void OnVisibleChange(bool fVisible) {}
			virtual bool OnFloatingChange(bool fFloating) { return true; }
			virtual void OnDocking(DockingPlace Place) {}
			virtual bool OnActivate(bool fActive) { return false; }
		};

		static bool Initialize(HINSTANCE hinst);

		CPanelFrame();
		~CPanelFrame();

		bool Create(
			HWND hwndOwner, Layout::CSplitter *pSplitter, int PanelID,
			CPanelContent *pContent, LPCTSTR pszTitle);
		CPanel *GetPanel() { return &m_Panel; }
		CBasicWindow *GetWindow() { return m_Panel.GetWindow(); }
		bool SetFloating(bool fFloating);
		bool GetFloating() const { return m_fFloating; }
		void SetEventHandler(CEventHandler *pHandler);
		bool SetPanelVisible(bool fVisible, bool fNoActivate = false);
		int GetDockingWidth() const { return m_DockingWidth; }
		bool SetDockingWidth(int Width);
		int GetDockingHeight() const { return m_DockingHeight; }
		bool SetDockingHeight(int Height);
		bool IsDockingVertical() const;
		bool SetDockingPlace(DockingPlace Place);
		bool SetPanelTheme(const CPanel::PanelTheme &Theme);
		bool GetPanelTheme(CPanel::PanelTheme *pTheme) const;
		bool SetPanelOpacity(int Opacity);
		int GetPanelOpacity() const { return m_Opacity; }

	// CUIBase
		void SetTheme(const Theme::CThemeManager *pThemeManager) override;

	private:
		Layout::CSplitter *m_pSplitter;
		int m_PanelID;
		CPanel m_Panel;
		bool m_fFloating;
		bool m_fFloatingTransition;
		int m_DockingWidth;
		int m_DockingHeight;
		int m_Opacity;
		CDropHelper m_DropHelper;
		DockingPlace m_DragDockingTarget;
		bool m_fDragMoving;
		CEventHandler *m_pEventHandler;
		Style::CStyleScaling m_StyleScaling;

		static HINSTANCE m_hinst;

	// CBasicWindow
		bool Create(HWND hwndParent, DWORD Sytle, DWORD ExStyle = 0, int ID = 0) override;

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CPanel::CEventHandler
		bool OnFloating() override;
		bool OnClose() override;
		bool OnEnterSizeMove() override;
		bool OnMoving(RECT *pRect) override;
		bool OnKeyDown(UINT KeyCode, UINT Flags) override;
		void OnSizeChanged(int Width, int Height) override;
		bool OnMenuPopup(HMENU hmenu) override;
		bool OnMenuSelected(int Command) override;
	};

}	// namespace TVTest


#endif
