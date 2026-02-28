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


#ifndef TVTEST_STATUS_VIEW_H
#define TVTEST_STATUS_VIEW_H


#include <vector>
#include <memory>
#include "BasicWindow.h"
#include "UIBase.h"
#include "Theme.h"
#include "DrawUtil.h"
#include "WindowUtil.h"
#include "Aero.h"


namespace TVTest
{

	class CStatusView;

	class ABSTRACT_CLASS(CStatusItem)
		: public CUIBase
	{
	public:
		enum class StyleFlag : unsigned int {
			None          = 0x0000U,
			VariableWidth = 0x0001U,
			FullRow       = 0x0002U,
			ForceFullRow  = 0x0004U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		enum class SizeUnit {
			Pixel,
			EM,
		};

		static constexpr int EM_FACTOR = 1000;

		struct SizeValue
		{
			int Value;
			SizeUnit Unit;

			SizeValue(int v, SizeUnit u) : Value(v), Unit(u) {}
		};

		enum class DrawFlag : unsigned int {
			None      = 0x0000U,
			Highlight = 0x0001U,
			Bottom    = 0x0002U,
			Preview   = 0x0004U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		CStatusItem(int ID, const SizeValue &DefaultWidth);
		virtual ~CStatusItem() = default;

		int GetIndex() const;
		bool GetRect(RECT *pRect) const;
		bool GetClientRect(RECT *pRect) const;
		int GetID() const { return m_ID; }
		const SizeValue &GetDefaultWidth() const { return m_DefaultWidth; }
		int GetWidth() const { return m_Width; }
		bool SetWidth(int Width);
		int GetMinWidth() const { return m_MinWidth; }
		int GetMaxWidth() const { return m_MaxWidth; }
		int GetActualWidth() const { return m_ActualWidth; }
		bool SetActualWidth(int Width);
		int GetMinHeight() const { return m_MinHeight; }
		void SetVisible(bool fVisible);
		bool GetVisible() const { return m_fVisible; }
		void SetItemStyle(StyleFlag Style);
		void SetItemStyle(StyleFlag Mask, StyleFlag Style);
		StyleFlag GetItemStyle() const { return m_Style; }
		bool IsVariableWidth() const { return EnumAnd(m_Style, StyleFlag::VariableWidth) == StyleFlag::VariableWidth; }
		bool IsFullRow() const { return EnumAnd(m_Style, StyleFlag::FullRow) == StyleFlag::FullRow; }
		bool IsForceFullRow() const { return EnumAnd(m_Style, StyleFlag::ForceFullRow) == StyleFlag::ForceFullRow; }
		bool Update();
		void Redraw();
		virtual LPCTSTR GetIDText() const = 0;
		virtual LPCTSTR GetName() const = 0;
		virtual bool UpdateContent() { return true; }
		virtual void Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags) = 0;
		virtual void OnLButtonDown(int x, int y) {}
		virtual void OnLButtonUp(int x, int y) {}
		virtual void OnLButtonDoubleClick(int x, int y) { OnLButtonDown(x, y); }
		virtual void OnRButtonDown(int x, int y) { OnLButtonDown(x, y); }
		virtual void OnRButtonUp(int x, int y) {}
		virtual void OnRButtonDoubleClick(int x, int y) {}
		virtual void OnMButtonDown(int x, int y) {}
		virtual void OnMButtonUp(int x, int y) {}
		virtual void OnMButtonDoubleClick(int x, int y) {}
		virtual void OnMouseMove(int x, int y) {}
		virtual bool OnMouseWheel(int x, int y, bool fHorz, int Delta, int *pCommand) { return false; }
		virtual void OnVisibilityChanged() {}
		virtual void OnPresentStatusChange(bool fPresent) {}
		virtual void OnFocus(bool fFocus) {}
		virtual bool OnMouseHover(int x, int y) { return false; }
		virtual void OnSizeChanged() {}
		virtual void OnCaptureReleased() {}
		virtual LRESULT OnNotifyMessage(LPNMHDR pnmh) { return 0; }
		virtual void OnFontChanged() {}

		friend CStatusView;

	protected:
		CStatusView *m_pStatus = nullptr;
		int m_ID;
		SizeValue m_DefaultWidth;
		int m_Width = -1;
		int m_MinWidth = 8;
		int m_MaxWidth = -1;
		int m_ActualWidth = -1;
		int m_MinHeight = 0;
		bool m_fVisible = true;
		bool m_fBreak = false;
		StyleFlag m_Style = StyleFlag::None;

		bool GetMenuPos(POINT *pPos, UINT *pFlags, RECT *pExcludeRect);
		enum class DrawTextFlag : unsigned int {
			None             = 0x0000U,
			HorizontalCenter = 0x0001U,
			NoEndEllipsis    = 0x0002U,
			TVTEST_ENUM_FLAGS_TRAILER
		};
		void DrawText(HDC hdc, const RECT &Rect, LPCTSTR pszText, DrawTextFlag Flags = DrawTextFlag::None) const;
		void DrawIcon(
			HDC hdc, const RECT &Rect, DrawUtil::CMonoColorIconList &IconList,
			int IconIndex = 0, bool fEnabled = true) const;
	};

	class CIconStatusItem
		: public CStatusItem
	{
	public:
		CIconStatusItem(int ID, int DefaultWidth);

		void NormalizeStyle(
			const Style::CStyleManager *pStyleManager,
			const Style::CStyleScaling *pStyleScaling) override;
	};

	class CStatusView
		: public CCustomWindow
		, public CUIBase
	{
	public:
		class ABSTRACT_CLASS(CEventHandler)
		{
		protected:
			CStatusView *m_pStatusView = nullptr;

		public:
			virtual ~CEventHandler();

			virtual void OnMouseLeave() {}
			virtual void OnHeightChanged(int Height) {}
			virtual void OnStyleChanged() {}
			friend CStatusView;
		};

		struct StatusViewTheme
		{
			Theme::Style ItemStyle;
			Theme::Style HighlightItemStyle;
			Theme::Style BottomItemStyle;
			Theme::BorderStyle Border;
		};

		static bool Initialize(HINSTANCE hinst);

		CStatusView();
		~CStatusView();

	// CBasicWindow
		bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;
		void SetVisible(bool fVisible) override;

	// CUIBase
		void SetStyle(const Style::CStyleManager *pStyleManager) override;
		void NormalizeStyle(
			const Style::CStyleManager *pStyleManager,
			const Style::CStyleScaling *pStyleScaling) override;
		void SetTheme(const Theme::CThemeManager *pThemeManager) override;

	// CStatusView
		int NumItems() const { return static_cast<int>(m_ItemList.size()); }
		const CStatusItem *GetItem(int Index) const;
		CStatusItem *GetItem(int Index);
		const CStatusItem *GetItemByID(int ID) const;
		CStatusItem *GetItemByID(int ID);
		bool AddItem(CStatusItem *pItem);
		int IDToIndex(int ID) const;
		int IndexToID(int Index) const;
		bool UpdateItem(int ID);
		void RedrawItem(int ID);
		bool GetItemRect(int ID, RECT *pRect) const;
		bool GetItemRectByIndex(int Index, RECT *pRect) const;
		bool GetItemClientRect(int ID, RECT *pRect) const;
		int GetItemHeight() const;
		int CalcItemHeight(const DrawUtil::CFont &Font) const;
		const Style::Margins &GetItemPadding() const;
		const Style::Size &GetIconSize() const;
		int GetFontHeight() const { return m_FontHeight; }
		int GetIntegralWidth() const;
		bool AdjustSize();
		void SetSingleText(LPCTSTR pszText);
		static bool GetStatusViewThemeFromThemeManager(
			const Theme::CThemeManager *pThemeManager, StatusViewTheme *pTheme);
		bool SetStatusViewTheme(const StatusViewTheme &Theme);
		bool GetStatusViewTheme(StatusViewTheme *pTheme) const;
		void SetItemTheme(const Theme::CThemeManager *pThemeManager);
		bool SetFont(const Style::Font &Font);
		bool GetFont(Style::Font *pFont) const;
		HFONT GetFont() const;
		bool SetMultiRow(bool fMultiRow);
		bool SetMaxRows(int MaxRows);
		int CalcHeight(int Width) const;
		int GetCurItem() const;
		bool SetEventHandler(CEventHandler *pEventHandler);
		bool SetItemOrder(const int *pOrderList);
		bool CreateItemPreviewFont(
			const Style::Font &Font, DrawUtil::CFont *pDrawFont) const;
		bool DrawItemPreview(
			CStatusItem *pItem, HDC hdc, const RECT &ItemRect,
			bool fHighlight = false, HFONT hfont = nullptr) const;
		bool EnableBufferedPaint(bool fEnable);
		void EnableSizeAdjustment(bool fEnable);
		int CalcItemPixelSize(const CStatusItem::SizeValue &Size) const;

	private:
		struct StatusViewStyle
		{
			Style::Margins ItemPadding{4, 2, 4, 2};
			Style::IntValue TextExtraHeight{4};
			Style::Size IconSize{16, 16};

			void SetStyle(const Style::CStyleManager *pStyleManager);
			void NormalizeStyle(
				const Style::CStyleManager *pStyleManager,
				const Style::CStyleScaling *pStyleScaling);
		};

		static const LPCTSTR CLASS_NAME;
		static HINSTANCE m_hinst;

		StatusViewStyle m_Style;
		Style::Font m_Font;
		DrawUtil::CFont m_DrawFont;
		int m_FontHeight = 0;
		int m_TextHeight = 0;
		int m_ItemHeight = 0;
		bool m_fMultiRow = false;
		int m_MaxRows = 2;
		int m_Rows = 1;
		StatusViewTheme m_Theme;
		std::vector<std::unique_ptr<CStatusItem>> m_ItemList;
		bool m_fSingleMode = false;
		String m_SingleText;
		int m_HotItem = -1;
		CMouseLeaveTrack m_MouseLeaveTrack;
		bool m_fOnButtonDown = false;
		int m_CapturedItem = -1;
		CEventHandler *m_pEventHandler = nullptr;
		DrawUtil::COffscreen m_Offscreen;
		bool m_fBufferedPaint = false;
		CBufferedPaint m_BufferedPaint;
		bool m_fAdjustSize = true;

		void SetHotItem(int Item);
		void Draw(HDC hdc, const RECT *pPaintRect);
		void CalcLayout();
		int CalcRows(const std::vector<const CStatusItem*> &ItemList, int MaxRowWidth) const;
		int CalcRows(const std::vector<CStatusItem*> &ItemList, int MaxRowWidth);
		int CalcTextHeight(const DrawUtil::CFont &Font, int *pFontHeight = nullptr) const;
		int CalcTextHeight(int *pFontHeight = nullptr) const;
		int CalcItemHeight(int TextHeight) const;
		int CalcItemHeight() const;

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void ApplyStyle() override;
		void RealizeStyle() override;
	};

} // namespace TVTest


#endif
