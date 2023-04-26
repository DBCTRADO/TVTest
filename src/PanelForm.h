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


#ifndef TVTEST_PANEL_FORM_H
#define TVTEST_PANEL_FORM_H


#include "Panel.h"
#include "UIBase.h"
#include "Theme.h"
#include "DrawUtil.h"
#include "Tooltip.h"
#include <vector>
#include <memory>


namespace TVTest
{

	class CPanelForm
		: public CPanelContent
	{
	public:
		class ABSTRACT_CLASS(CPage)
			: public CCustomWindow
			, public CUIBase
		{
		public:
			virtual ~CPage() = 0;

			virtual bool SetFont(const Style::Font & Font) { return true; }
			virtual void OnActivate() {}
			virtual void OnDeactivate() {}
			virtual void OnVisibilityChanged(bool fVisible) {}
			virtual void OnFormDelete() {}
			virtual bool DrawIcon(
				HDC hdc, int x, int y, int Width, int Height,
				const Theme::ThemeColor &Color) { return false; }
			virtual bool NeedKeyboardFocus() const { return false; }

		protected:
			bool CreateDefaultFont(DrawUtil::CFont *pFont);
		};

		class ABSTRACT_CLASS(CEventHandler)
		{
		public:
			virtual void OnSelChange() {}
			virtual void OnRButtonUp(int x, int y) {}
			virtual void OnTabRButtonUp(int x, int y) {}
			virtual bool OnKeyDown(UINT KeyCode, UINT Flags) { return false; }
			virtual void OnVisibleChange(bool fVisible) {}
		};

		struct PageInfo
		{
			CPage *pPage;
			LPCTSTR pszTitle;
			int ID;
			int Icon;
			bool fVisible;
		};

		struct TabInfo
		{
			int ID;
			bool fVisible;
		};

		struct PanelFormTheme
		{
			Theme::Style TabStyle;
			Theme::Style CurTabStyle;
			Theme::Style TabMarginStyle;
			Theme::ThemeColor BackColor;
			Theme::ThemeColor BorderColor;
		};

		enum class TabStyle {
			TextOnly,
			IconOnly,
			IconAndText,
			TVTEST_ENUM_CLASS_TRAILER
		};

		static bool Initialize(HINSTANCE hinst);

		CPanelForm();
		~CPanelForm();

	// CBasicWindow
		bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;
		void SetVisible(bool fVisible) override;

	// CUIBase
		void SetStyle(const Style::CStyleManager *pStyleManager) override;
		void NormalizeStyle(
			const Style::CStyleManager *pStyleManager,
			const Style::CStyleScaling *pStyleScaling) override;
		void SetTheme(const Theme::CThemeManager *pThemeManager) override;

	// CPanelForm
		bool AddPage(const PageInfo &Info);
		int NumPages() const { return static_cast<int>(m_WindowList.size()); }
		CPage *GetPageByIndex(int Index);
		CPage *GetPageByID(int ID);
		int IDToIndex(int ID) const;
		int GetCurPageID() const;
		bool SetCurPageByID(int ID);
		bool SetTabVisible(int ID, bool fVisible);
		bool GetTabVisible(int ID) const;
		bool SetTabOrder(const int *pOrder, int Count);
		bool GetTabInfo(int Index, TabInfo *pInfo) const;
		int GetTabID(int Index) const;
		bool GetTabTitle(int ID, String *pTitle) const;
		void SetEventHandler(CEventHandler *pHandler);
		bool SetPanelFormTheme(const PanelFormTheme &Theme);
		bool GetPanelFormTheme(PanelFormTheme *pTheme) const;
		bool SetTabFont(const Style::Font &Font);
		bool SetPageFont(const Style::Font &Font);
		bool GetPageClientRect(RECT *pRect) const;
		bool SetTabStyle(TabStyle Style);
		bool SetIconImage(HBITMAP hbm, int Width, int Height);
		SIZE GetIconDrawSize() const;
		bool EnableTooltip(bool fEnable);

	private:
		class CWindowInfo
		{
		public:
			CPage *m_pWindow;
			String m_Title;
			int m_ID;
			int m_Icon;
			bool m_fVisible;

			CWindowInfo(const PageInfo &Info);
		};

		struct PanelFormStyle
		{
			Style::Margins TabPadding{3};
			Style::Size TabIconSize{16, 16};
			Style::Margins TabIconMargin{0};
			Style::Margins TabLabelMargin{0};
			Style::IntValue TabIconLabelMargin{4};
			Style::Margins ClientMargin{4};

			void SetStyle(const Style::CStyleManager *pStyleManager);
			void NormalizeStyle(
				const Style::CStyleManager *pStyleManager,
				const Style::CStyleScaling *pStyleScaling);
		};

		std::vector<std::unique_ptr<CWindowInfo>> m_WindowList;
		std::vector<int> m_TabOrder;
		PanelFormStyle m_Style;
		PanelFormTheme m_Theme;
		Style::Font m_StyleFont;
		DrawUtil::CFont m_Font;
		DrawUtil::CMonoColorIconList m_Icons;
		TabStyle m_TabStyle = TabStyle::TextOnly;
		int m_TabHeight = 0;
		int m_TabWidth = 0;
		int m_TabLineWidth = 1;
		bool m_fFitTabWidth = true;
		int m_CurTab = -1;
		int m_PrevActivePageID = -1;
		CEventHandler *m_pEventHandler = nullptr;
		CTooltip m_Tooltip;
		bool m_fEnableTooltip = true;

		static const LPCTSTR m_pszClassName;
		static HINSTANCE m_hinst;

		bool SetCurTab(int Index);
		void CalcTabSize();
		int GetRealTabWidth() const;
		int HitTest(int x, int y) const;
		void Draw(HDC hdc, const RECT &PaintRect);
		void UpdateTooltip();

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void ApplyStyle() override;
		void RealizeStyle() override;
	};

}	// namespace TVTest


#endif
