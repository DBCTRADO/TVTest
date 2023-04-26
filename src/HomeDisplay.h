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


#ifndef TVTEST_HOME_DISPLAY_H
#define TVTEST_HOME_DISPLAY_H


#include <vector>
#include <memory>
#include "View.h"
#include "DrawUtil.h"
#include "Theme.h"
#include "Aero.h"
#include "ProgramSearch.h"
#include "WindowUtil.h"


namespace TVTest
{

	class CHomeDisplay
		: public CDisplayView
		, public CDoubleBufferingDraw
		, public CSettingsBase
	{
	public:
		struct StyleInfo
		{
			Theme::BackgroundStyle CategoriesBackStyle;
			Theme::BackgroundStyle ContentBackStyle;
			Theme::Style CategoryItemStyle;
			Theme::Style CategoryItemSelStyle;
			Theme::Style CategoryItemCurStyle;
			Theme::Style ItemStyle[2];
			Theme::Style ItemHotStyle;
			COLORREF BannerTextColor;
			int FontHeight;
			Style::Margins ItemMargins;
			Style::Margins CategoryItemMargins;
			Style::IntValue CategoryIconMargin;
		};

		class ABSTRACT_CLASS(CHomeDisplayEventHandler)
			: public CDisplayView::CEventHandler
		{
		public:
			virtual void OnClose() = 0;

			friend class CHomeDisplay;

		protected:
			class CHomeDisplay *m_pHomeDisplay = nullptr;
		};

		class ABSTRACT_CLASS(CCategory)
		{
		public:
			CCategory(class CHomeDisplay * pHomeDisplay);
			virtual ~CCategory() = default;

			virtual int GetID() const = 0;
			virtual LPCTSTR GetTitle() const = 0;
			virtual int GetIconIndex() const = 0;
			virtual int GetHeight() const = 0;
			virtual bool Create() = 0;
			virtual void ReadSettings(CSettings & Settings) {}
			virtual void WriteSettings(CSettings & Settings) {}
			virtual void LayOut(const StyleInfo & Style, HDC hdc, const RECT & ContentRect) = 0;
			virtual void Draw(
				HDC hdc, const StyleInfo & Style, const RECT & ContentRect, const RECT & PaintRect,
				Theme::CThemeDraw & ThemeDraw) const = 0;
			virtual bool GetCurItemRect(RECT * pRect) const = 0;
			virtual bool SetFocus(bool fFocus) = 0;
			virtual bool IsFocused() const = 0;
			virtual bool OnDecide() { return false; }
			virtual void OnWindowCreate() {}
			virtual void OnWindowDestroy() {}
			virtual void OnCursorMove(int x, int y) {}
			virtual void OnCursorLeave() {}
			virtual void OnLButtonDown(int x, int y) {}
			virtual bool OnLButtonUp(int x, int y) { return false; }
			virtual bool OnRButtonUp(int x, int y) { return false; }
			virtual bool OnSetCursor() { return false; }
			virtual bool OnCursorKey(WPARAM KeyCode) { return false; }

		protected:
			class CHomeDisplay *m_pHomeDisplay;
		};

		CHomeDisplay();
		~CHomeDisplay();

	// CBasicWindow
		bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;

	// CDisplayView
		bool Close() override;
		bool IsMessageNeed(const MSG *pMsg) const override;
		bool OnMouseWheel(UINT Msg, WPARAM wParam, LPARAM lParam) override;

	// CSettingsBase
		bool LoadSettings(CSettings &Settings) override;
		bool SaveSettings(CSettings &Settings) override;

	// CHomeDisplay
		void Clear();
		bool UpdateContents();
		void SetEventHandler(CHomeDisplayEventHandler *pEventHandler);
		bool SetFont(const Style::Font &Font, bool fAutoSize);
		int GetScrollPos() const { return m_ScrollPos; }
		bool SetScrollPos(int Pos, bool fScroll = true);
		bool SetCurCategory(int Category);
		int GetCurCategoryID() const;
		bool UpdateCurContent();
		bool OnContentChanged();

		static bool Initialize(HINSTANCE hinst);

	private:
		enum class PartType {
			Margin,
			Category,
			Content,
		};

		Style::Font m_StyleFont;
		DrawUtil::CFont m_Font;
		bool m_fAutoFontSize = true;
		StyleInfo m_HomeDisplayStyle;
		int m_CategoriesAreaWidth = 0;
		int m_CategoryItemWidth = 0;
		int m_CategoryItemHeight = 0;
		int m_ContentHeight = 0;

		std::vector<std::unique_ptr<CCategory>> m_CategoryList;
		CHomeDisplayEventHandler *m_pHomeDisplayEventHandler = nullptr;
		int m_CurCategory = 0;
		HWND m_hwndScroll = nullptr;
		int m_ScrollPos = 0;
		CMouseWheelHandler m_MouseWheel;
		bool m_fHitCloseButton = false;
		PartType m_CursorPart = PartType::Margin;
		HIMAGELIST m_himlIcons = nullptr;

		static const LPCTSTR m_pszWindowClass;
		static HINSTANCE m_hinst;

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void ApplyStyle() override;
		void RealizeStyle() override;

	// CDoubleBufferingDraw
		void Draw(HDC hdc, const RECT &PaintRect) override;

		void LayOut();
		void SetScrollBar();
		void GetContentAreaRect(RECT *pRect) const;
		PartType HitTest(int x, int y, int *pCategoryIndex) const;
		void ScrollToCurItem();
		bool GetCategoryItemRect(int Category, RECT *pRect) const;
		bool RedrawCategoryItem(int Category);
	};

}	// namespace TVTest


#endif
