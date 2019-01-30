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


#ifndef TVTEST_SIDE_BAR_H
#define TVTEST_SIDE_BAR_H


#include <vector>
#include "BasicWindow.h"
#include "UIBase.h"
#include "Command.h"
#include "DrawUtil.h"
#include "Theme.h"
#include "Tooltip.h"
#include "WindowUtil.h"


namespace TVTest
{

	class CSideBar
		: public CCustomWindow
		, public CUIBase
	{
	public:
		static constexpr int ITEM_SEPARATOR = 0;

		enum class ItemState : unsigned int {
			None     = 0x0000U,
			Disabled = 0x0001U,
			Checked  = 0x0002U,
			Hot      = 0x0004U,
		};

		struct SideBarItem
		{
			int Command;
			int Icon;
			ItemState State;

			bool IsDisabled() const { return EnumAnd(State, ItemState::Disabled) == ItemState::Disabled; }
			bool IsEnabled() const { return !IsDisabled(); }
			bool IsChecked() const { return EnumAnd(State, ItemState::Checked) == ItemState::Checked; }
		};

		struct SideBarTheme
		{
			Theme::Style ItemStyle;
			Theme::Style HighlightItemStyle;
			Theme::Style CheckItemStyle;
			Theme::BorderStyle Border;
		};

		struct DrawIconInfo
		{
			int Command;
			ItemState State;
			HDC hdc;
			RECT IconRect;
			COLORREF Color;
			BYTE Opacity;
			HDC hdcBuffer;
		};

		class ABSTRACT_CLASS(CEventHandler)
		{
		protected:
			CSideBar *m_pSideBar;

		public:
			CEventHandler();
			virtual ~CEventHandler();

			virtual void OnCommand(int Command) {}
			virtual void OnRButtonUp(int x, int y) {}
			virtual void OnMouseLeave() {}
			virtual bool GetTooltipText(int Command, LPTSTR pszText, int MaxText) { return false; }
			virtual bool DrawIcon(const DrawIconInfo * pInfo) { return false; }
			virtual void OnBarWidthChanged(int BarWidth) {}
			virtual void OnStyleChanged() {}
			friend class CSideBar;
		};

		static bool Initialize(HINSTANCE hinst);

		CSideBar(const CCommandManager *pCommandManager);
		~CSideBar();

	// CBasicWindow
		bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;

	// CUIBase
		void SetStyle(const Style::CStyleManager *pStyleManager) override;
		void NormalizeStyle(
			const Style::CStyleManager *pStyleManager,
			const Style::CStyleScaling *pStyleScaling) override;
		void SetTheme(const Theme::CThemeManager *pThemeManager) override;

	// CSideBar
		int GetBarWidth() const;
		bool SetIconImage(HBITMAP hbm, int Width, int Height);
		void DeleteAllItems();
		bool AddItem(const SideBarItem *pItem);
		bool AddItems(const SideBarItem *pItemList, int NumItems);
		bool AddSeparator();
		int GetItemCount() const;
		int GetItemCommand(int Index) const;
		int CommandToIndex(int Command) const;
		bool EnableItem(int Command, bool fEnable);
		bool EnableItemByIndex(int Index, bool fEnable);
		bool IsItemEnabled(int Command) const;
		bool CheckItem(int Command, bool fCheck);
		bool CheckItemByIndex(int Index, bool fCheck);
		bool CheckRadioItem(int First, int Last, int Check);
		bool IsItemChecked(int Command) const;
		bool RedrawItem(int Command);
		bool SetSideBarTheme(const SideBarTheme &Theme);
		bool GetSideBarTheme(SideBarTheme *pTheme) const;
		void ShowToolTips(bool fShow);
		void SetVertical(bool fVertical);
		bool GetVertical() const { return m_fVertical; }
		void SetEventHandler(CEventHandler *pHandler);
		const CCommandManager *GetCommandManager() const { return m_pCommandManager; }
		Style::Size GetIconDrawSize() const;

	protected:
		struct SideBarStyle
		{
			Style::Size IconSize;
			Style::Margins ItemPadding;
			Style::IntValue SeparatorWidth;

			SideBarStyle();
			void SetStyle(const Style::CStyleManager *pStyleManager);
			void NormalizeStyle(
				const Style::CStyleManager *pStyleManager,
				const Style::CStyleScaling *pStyleScaling);
		};

		SideBarStyle m_Style;
		CTooltip m_Tooltip;
		DrawUtil::CFont m_TooltipFont;
		bool m_fShowTooltips;
		DrawUtil::CMonoColorIconList m_Icons;
		bool m_fVertical;
		SideBarTheme m_Theme;
		std::vector<SideBarItem> m_ItemList;
		int m_HotItem;
		int m_ClickItem;
		CMouseLeaveTrack m_MouseLeaveTrack;
		CEventHandler *m_pEventHandler;
		const CCommandManager *m_pCommandManager;

		static const int ICON_WIDTH;
		static const int ICON_HEIGHT;
		static const LPCTSTR CLASS_NAME;
		static HINSTANCE m_hinst;

		void GetItemRect(int Item, RECT *pRect) const;
		void UpdateItem(int Item);
		int HitTest(int x, int y) const;
		void UpdateTooltipsRect();
		void SetTooltipFont();
		void Draw(HDC hdc, const RECT &PaintRect);

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void RealizeStyle() override;
	};

	TVTEST_ENUM_FLAGS(CSideBar::ItemState)

}	// namespace TVTest


#endif
