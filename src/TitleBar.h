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


#ifndef TVTEST_TITLE_BAR_H
#define TVTEST_TITLE_BAR_H


#include "BasicWindow.h"
#include "UIBase.h"
#include "Theme.h"
#include "DrawUtil.h"
#include "Tooltip.h"
#include "WindowUtil.h"


namespace TVTest
{

	class CTitleBar
		: public CCustomWindow
		, public CUIBase
	{
	public:
		struct TitleBarTheme
		{
			Theme::Style CaptionStyle;
			Theme::Style IconStyle;
			Theme::Style HighlightIconStyle;
			Theme::BorderStyle Border;
		};

		class ABSTRACT_CLASS(CEventHandler)
		{
		protected:
			class CTitleBar *m_pTitleBar;

		public:
			CEventHandler();
			virtual ~CEventHandler();

			virtual bool OnClose() { return false; }
			virtual bool OnMinimize() { return false; }
			virtual bool OnMaximize() { return false; }
			virtual bool OnFullscreen() { return false; }
			virtual void OnMouseLeave() {}
			virtual void OnLabelLButtonDown(int x, int y) {}
			virtual void OnLabelLButtonDoubleClick(int x, int y) {}
			virtual void OnLabelRButtonUp(int x, int y) {}
			virtual void OnIconLButtonDown(int x, int y) {}
			virtual void OnIconLButtonDoubleClick(int x, int y) {}
			virtual void OnHeightChanged(int Height) {}
			friend class CTitleBar;
		};

		static bool Initialize(HINSTANCE hinst);

		CTitleBar();
		~CTitleBar();

	// CBasicWindow
		bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;
		void SetVisible(bool fVisible) override;

	// CUIBase
		void SetStyle(const Style::CStyleManager *pStyleManager) override;
		void NormalizeStyle(
			const Style::CStyleManager *pStyleManager,
			const Style::CStyleScaling *pStyleScaling) override;
		void SetTheme(const Theme::CThemeManager *pThemeManager) override;

	// CTitleBar
		int CalcHeight() const;
		int GetButtonWidth() const;
		int GetButtonHeight() const;
		bool SetLabel(LPCTSTR pszLabel);
		LPCTSTR GetLabel() const { return m_Label.c_str(); }
		void SetMaximizeMode(bool fMaximize);
		void SetFullscreenMode(bool fFullscreen);
		bool SetEventHandler(CEventHandler *pHandler);
		bool SetTitleBarTheme(const TitleBarTheme &Theme);
		bool GetTitleBarTheme(TitleBarTheme *pTheme) const;
		bool SetFont(const Style::Font &Font);
		void SetIcon(HICON hIcon);
		SIZE GetIconDrawSize() const;
		bool IsIconDrawSmall() const;

	private:
		enum {
			ITEM_LABEL,
			ITEM_MINIMIZE,
			ITEM_MAXIMIZE,
			ITEM_FULLSCREEN,
			ITEM_CLOSE,
			ITEM_BUTTON_FIRST = ITEM_MINIMIZE,
			ITEM_LAST = ITEM_CLOSE
		};

		struct TitleBarStyle
		{
			Style::Margins Padding;
			Style::Margins LabelMargin;
			Style::IntValue LabelExtraHeight;
			Style::Size IconSize;
			Style::Margins IconMargin;
			Style::Size ButtonIconSize;
			Style::Margins ButtonPadding;

			TitleBarStyle();
			void SetStyle(const Style::CStyleManager *pStyleManager);
			void NormalizeStyle(
				const Style::CStyleManager *pStyleManager,
				const Style::CStyleScaling *pStyleScaling);
		};

		TitleBarStyle m_Style;
		Style::Font m_StyleFont;
		DrawUtil::CFont m_Font;
		int m_FontHeight;
		TitleBarTheme m_Theme;
		Theme::IconList m_ButtonIcons;
		CTooltip m_Tooltip;
		String m_Label;
		HICON m_hIcon;
		int m_HotItem;
		int m_ClickItem;
		CMouseLeaveTrack m_MouseLeaveTrack;
		bool m_fMaximized;
		bool m_fFullscreen;
		CEventHandler *m_pEventHandler;

		static const LPCTSTR CLASS_NAME;
		static HINSTANCE m_hinst;

		void AdjustSize();
		int CalcFontHeight() const;
		bool GetItemRect(int Item, RECT *pRect) const;
		bool UpdateItem(int Item);
		int HitTest(int x, int y) const;
		bool PtInIcon(int x, int y) const;
		void UpdateTooltipsRect();
		void Draw(HDC hdc, const RECT &PaintRect);

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void ApplyStyle() override;
		void RealizeStyle() override;
	};

}	// namespace TVTest


#endif
