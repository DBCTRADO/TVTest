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


#ifndef TVTEST_CONTROL_PANEL_H
#define TVTEST_CONTROL_PANEL_H


#include <vector>
#include <memory>
#include "PanelForm.h"
#include "UIBase.h"
#include "DrawUtil.h"
#include "Theme.h"


namespace TVTest
{

	class CControlPanelItem;

	class CControlPanel
		: public CPanelForm::CPage
	{
	public:
		struct ControlPanelTheme
		{
			Theme::Style ItemStyle;
			Theme::Style OverItemStyle;
			Theme::Style CheckedItemStyle;
			Theme::ThemeColor MarginColor;
		};

		static bool Initialize(HINSTANCE hinst);

		~CControlPanel();

	// CBasicWindow
		bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;

	// CUIBase
		void SetStyle(const Style::CStyleManager *pStyleManager) override;
		void NormalizeStyle(
			const Style::CStyleManager *pStyleManager,
			const Style::CStyleScaling *pStyleScaling) override;
		void SetTheme(const Theme::CThemeManager *pThemeManager) override;

	// CPage
		bool SetFont(const Style::Font &Font) override;

	// CControlPanel
		bool AddItem(CControlPanelItem *pItem);
		CControlPanelItem *GetItem(int Index) const;
		bool UpdateItem(int Index);
		bool GetItemPosition(int Index, RECT *pRect) const;
		void UpdateLayout();
		bool SetControlPanelTheme(const ControlPanelTheme &Theme);
		bool GetControlPanelTheme(ControlPanelTheme *pTheme) const;
		int GetFontHeight() const { return m_FontHeight; }
		void SetSendMessageWindow(HWND hwnd);
		bool CheckRadioItem(int FirstID, int LastID, int CheckID);
		const Style::Margins &GetItemPadding() const;
		const Style::Size &GetIconSize() const;

		friend CControlPanelItem;

	private:
		struct ControlPanelStyle
		{
			Style::Margins Padding{2};
			Style::Margins ItemPadding{4, 2, 4, 2};
			Style::IntValue TextExtraHeight{4};
			Style::Size IconSize{16, 16};

			void SetStyle(const Style::CStyleManager *pStyleManager);
			void NormalizeStyle(
				const Style::CStyleManager *pStyleManager,
				const Style::CStyleScaling *pStyleScaling);
		};

		std::vector<std::unique_ptr<CControlPanelItem>> m_ItemList;
		Style::Font m_StyleFont;
		DrawUtil::CFont m_Font;
		int m_FontHeight = 0;
		ControlPanelStyle m_Style;
		ControlPanelTheme m_Theme;
		DrawUtil::COffscreen m_Offscreen;
		HWND m_hwndMessage = nullptr;
		int m_HotItem = -1;
		bool m_fTrackMouseEvent = false;
		bool m_fOnButtonDown = false;

		static const LPCTSTR m_pszClassName;
		static HINSTANCE m_hinst;

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void ApplyStyle() override;
		void RealizeStyle() override;

	// CControlPanel
		void Draw(HDC hdc, const RECT &PaintRect);
		void SendCommand(int Command);
		bool CalcTextSize(LPCTSTR pszText, SIZE *pSize);
		int CalcFontHeight() const;
		int GetTextItemHeight() const;
	};

	class ABSTRACT_CLASS(CControlPanelItem)
		: public CUIBase
	{
	protected:
		RECT m_Position{};
		int m_Command = 0;
		bool m_fVisible = true;
		bool m_fEnable = true;
		bool m_fCheck = false;
		bool m_fBreak = true;
		CControlPanel * m_pControlPanel = nullptr;

		bool CalcTextSize(LPCTSTR pszText, SIZE * pSize) const;
		int GetTextItemHeight() const;
		void GetMenuPos(POINT * pPos) const;

	public:
		virtual ~CControlPanelItem() = default;

		void GetPosition(int *pLeft, int *pTop, int *pWidth, int *pHeight) const;
		bool SetPosition(int Left, int Top, int Width, int Height);
		void GetPosition(RECT * pRect) const;
		bool GetVisible() const { return m_fVisible; }
		void SetVisible(bool fVisible);
		bool GetEnable() const { return m_fEnable; }
		void SetEnable(bool fEnable);
		bool GetCheck() const { return m_fCheck; }
		void SetCheck(bool fCheck);
		bool GetBreak() const { return m_fBreak; }
		void SetBreak(bool fBreak);
		virtual void CalcSize(int Width, SIZE *pSize);
		virtual void Draw(HDC hdc, const RECT &Rect) = 0;
		virtual void OnLButtonDown(int x, int y);
		virtual void OnLButtonUp(int x, int y) {}
		virtual void OnRButtonDown(int x, int y) {}
		virtual void OnRButtonUp(int x, int y) {}
		virtual void OnMouseMove(int x, int y) {}

		friend CControlPanel;
	};

}	// namespace TVTest


#endif
