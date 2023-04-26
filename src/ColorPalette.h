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


#ifndef TVTEST_COLOR_PALETTE_H
#define TVTEST_COLOR_PALETTE_H


#include <memory>
#include "BasicWindow.h"
#include "Tooltip.h"


namespace TVTest
{

	class CColorPalette
		: public CCustomWindow
	{
		int m_NumColors = 0;
		std::unique_ptr<RGBQUAD[]> m_Palette;
		int m_SelColor = -1;
		int m_HotColor = -1;
		int m_Left = 0;
		int m_Top = 0;
		int m_ItemWidth = 6;
		int m_ItemHeight = 6;
		CTooltip m_Tooltip;
		COLORREF m_BackColor = CLR_INVALID;

		static HINSTANCE m_hinst;

		void GetItemRect(int Index, RECT *pRect) const;
		void DrawSelRect(HDC hdc, int Sel, bool fSel);
		void DrawNewSelHighlight(int OldSel, int NewSel);
		void SetToolTip();
		void SendNotify(int Code);

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	public:
		~CColorPalette();

	// CBasicWindow
		bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;

	// CColorPalette
		bool GetPalette(RGBQUAD *pPalette);
		bool SetPalette(const RGBQUAD *pPalette, int NumColors);
		COLORREF GetColor(int Index) const;
		bool SetColor(int Index, COLORREF Color);
		int GetSel() const;
		bool SetSel(int Sel);
		int GetHot() const;
		int FindColor(COLORREF Color) const;
		bool SetTooltipFont(HFONT hfont);
		void SetBackColor(COLORREF Color);
		static bool Initialize(HINSTANCE hinst);
		enum {
			NOTIFY_SELCHANGE = 1,
			NOTIFY_HOTCHANGE,
			NOTIFY_RBUTTONDOWN,
			NOTIFY_DOUBLECLICK
		};
	};

}	// namespace TVTest


#endif
