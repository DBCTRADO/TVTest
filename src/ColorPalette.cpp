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


#include "stdafx.h"
#include "TVTest.h"
#include "ColorPalette.h"
#include "DrawUtil.h"


namespace TVTest
{

namespace
{

const LPCTSTR PALETTE_WINDOW_CLASS = APP_NAME TEXT(" Color Palette");

}




HINSTANCE CColorPalette::m_hinst = nullptr;


bool CColorPalette::Initialize(HINSTANCE hinst)
{
	if (m_hinst == nullptr) {
		WNDCLASS wc;

		wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hinst;
		wc.hIcon = nullptr;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = nullptr;
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = PALETTE_WINDOW_CLASS;
		if (RegisterClass(&wc) == 0)
			return false;
		m_hinst = hinst;
	}
	return true;
}


CColorPalette::~CColorPalette()
{
	Destroy();
}


bool CColorPalette::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	return CreateBasicWindow(
		hwndParent, Style, ExStyle, ID,
		PALETTE_WINDOW_CLASS, nullptr, m_hinst);
}


bool CColorPalette::GetPalette(RGBQUAD *pPalette)
{
	if (!m_Palette)
		return false;
	std::memcpy(pPalette, m_Palette.get(), m_NumColors * sizeof(RGBQUAD));
	return true;
}


bool CColorPalette::SetPalette(const RGBQUAD *pPalette, int NumColors)
{
	if (NumColors < 1 || NumColors > 256)
		return false;
	if (NumColors != m_NumColors) {
		m_Palette = std::make_unique<RGBQUAD[]>(NumColors);
		m_NumColors = NumColors;
	}
	std::memcpy(m_Palette.get(), pPalette, NumColors * sizeof(RGBQUAD));
	m_SelColor = -1;
	m_HotColor = -1;
	InvalidateRect(m_hwnd, nullptr, TRUE);
	SetToolTip();
	return true;
}


COLORREF CColorPalette::GetColor(int Index) const
{
	if (Index < 0 || Index >= m_NumColors)
		return CLR_INVALID;
	return RGB(m_Palette[Index].rgbRed, m_Palette[Index].rgbGreen, m_Palette[Index].rgbBlue);
}


bool CColorPalette::SetColor(int Index, COLORREF Color)
{
	if (Index < 0 || Index >= m_NumColors)
		return false;

	m_Palette[Index].rgbBlue = GetBValue(Color);
	m_Palette[Index].rgbGreen = GetGValue(Color);
	m_Palette[Index].rgbRed = GetRValue(Color);

	RECT rc;
	GetItemRect(Index, &rc);
	InvalidateRect(m_hwnd, &rc, TRUE);

	return true;
}


int CColorPalette::GetSel() const
{
	return m_SelColor;
}


bool CColorPalette::SetSel(int Sel)
{
	if (!m_Palette)
		return false;
	if (Sel < 0 || Sel >= m_NumColors)
		Sel = -1;
	if (Sel != m_SelColor) {
		DrawNewSelHighlight(m_SelColor, Sel);
		m_SelColor = Sel;
	}
	return true;
}


int CColorPalette::GetHot() const
{
	return m_HotColor;
}


int CColorPalette::FindColor(COLORREF Color) const
{
	for (int i = 0; i < m_NumColors; i++) {
		if (RGB(m_Palette[i].rgbRed, m_Palette[i].rgbGreen, m_Palette[i].rgbBlue) == Color)
			return i;
	}
	return -1;
}


bool CColorPalette::SetTooltipFont(HFONT hfont)
{
	return m_Tooltip.SetFont(hfont);
}


void CColorPalette::SetBackColor(COLORREF Color)
{
	if (m_BackColor != Color) {
		m_BackColor = Color;
		if (m_hwnd != nullptr)
			::InvalidateRect(m_hwnd, nullptr, TRUE);
	}
}


void CColorPalette::GetItemRect(int Index, RECT *pRect) const
{
	const int x = m_Left + Index % 16 * m_ItemWidth;
	const int y = m_Top + Index / 16 * m_ItemHeight;
	pRect->left = x;
	pRect->top = y;
	pRect->right = x + m_ItemWidth;
	pRect->bottom = y + m_ItemHeight;
}


void CColorPalette::DrawSelRect(HDC hdc, int Sel, bool fSel)
{
	const HPEN hpen = CreatePen(
		PS_INSIDEFRAME, 2,
		fSel || m_BackColor == CLR_INVALID ? GetSysColor(fSel ? COLOR_HIGHLIGHT : COLOR_3DFACE) : m_BackColor);
	const HPEN hpenOld = SelectPen(hdc, hpen);
	const HBRUSH hbrOld = SelectBrush(hdc, GetStockObject(NULL_BRUSH));
	RECT rc;
	GetItemRect(Sel, &rc);
	InflateRect(&rc, 1, 1);
	Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
	SelectPen(hdc, hpenOld);
	SelectBrush(hdc, hbrOld);
	DeleteObject(hpen);
}


void CColorPalette::DrawNewSelHighlight(int OldSel, int NewSel)
{
	const HDC hdc = GetDC(m_hwnd);

	if (OldSel >= 0)
		DrawSelRect(hdc, OldSel, false);
	if (NewSel >= 0)
		DrawSelRect(hdc, NewSel, true);
	ReleaseDC(m_hwnd, hdc);
}


void CColorPalette::SetToolTip()
{
	int NumTools = m_Tooltip.NumTools();
	if (NumTools > m_NumColors) {
		do {
			m_Tooltip.DeleteTool(--NumTools);
		} while (NumTools > m_NumColors);
	}
	for (int i = 0; i < m_NumColors; i++) {
		RECT rc;
		GetItemRect(i, &rc);
		if (i < NumTools)
			m_Tooltip.SetToolRect(i, rc);
		else
			m_Tooltip.AddTool(i, rc);
	}
}


void CColorPalette::SendNotify(int Code)
{
	::SendMessage(GetParent(), WM_COMMAND, MAKEWPARAM(GetWindowLong(m_hwnd, GWL_ID), Code), reinterpret_cast<LPARAM>(m_hwnd));
}


LRESULT CColorPalette::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		m_SelColor = -1;
		m_HotColor = -1;
		m_Tooltip.Create(hwnd);
		return 0;

	case WM_SIZE:
		{
			const int sx = LOWORD(lParam), sy = HIWORD(lParam);

			m_ItemWidth = std::max(sx / 16, 6);
			m_ItemHeight = std::max(sy / 16, 6);
			m_Left = (sx - m_ItemWidth * 16) / 2;
			m_Top = (sy - m_ItemHeight * 16) / 2;
			if (m_Palette)
				SetToolTip();
		}
		return 0;

	case WM_PAINT:
		if (m_Palette) {
			PAINTSTRUCT ps;

			::BeginPaint(hwnd, &ps);

			if (m_BackColor != CLR_INVALID)
				DrawUtil::Fill(ps.hdc, &ps.rcPaint, m_BackColor);
			else
				::FillRect(ps.hdc, &ps.rcPaint, reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1));

			for (int i = 0; i < m_NumColors; i++) {
				const int x = i % 16;
				const int y = i / 16;
				RECT rc;
				rc.left = m_Left + x * m_ItemWidth + 2;
				rc.top = m_Top + y * m_ItemHeight + 2;
				rc.right = rc.left + m_ItemWidth - 4;
				rc.bottom = rc.top + m_ItemHeight - 4;
				if (rc.left < ps.rcPaint.right && rc.top < ps.rcPaint.bottom
						&& rc.right > ps.rcPaint.left && rc.bottom > ps.rcPaint.top) {
					DrawUtil::Fill(
						ps.hdc, &rc,
						RGB(m_Palette[i].rgbRed, m_Palette[i].rgbGreen, m_Palette[i].rgbBlue));
				}
			}

			if (m_SelColor >= 0)
				DrawSelRect(ps.hdc, m_SelColor, true);

			::EndPaint(hwnd, &ps);
			return 0;
		}
		break;

	case WM_MOUSEMOVE:
		if (m_Palette) {
			const POINT ptCursor = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			int Hot =
				(ptCursor.y - m_Top) / m_ItemHeight * 16 +
				(ptCursor.x - m_Left) / m_ItemWidth;
			if (ptCursor.x < m_Left
					|| ptCursor.x >= m_Left + m_ItemWidth * 16
					|| ptCursor.y < m_Top
					|| ptCursor.y >= m_Top + m_ItemHeight * 16
					|| Hot >= m_NumColors)
				Hot = -1;
			if (Hot == m_HotColor)
				return 0;
			m_HotColor = Hot;
			SendNotify(NOTIFY_HOTCHANGE);
		}
		return 0;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		if (m_Palette) {
			const POINT ptCursor = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			const int Sel =
				(ptCursor.y - m_Top) / m_ItemHeight * 16 +
				(ptCursor.x - m_Left) / m_ItemWidth;
			if (ptCursor.x < m_Left
					|| ptCursor.x >= m_Left + m_ItemWidth * 16
					|| ptCursor.y < m_Top
					|| ptCursor.y >= m_Top + m_ItemHeight * 16
					|| Sel >= m_NumColors || Sel == m_SelColor)
				return 0;
			DrawNewSelHighlight(m_SelColor, Sel);
			m_SelColor = Sel;
			SendNotify(NOTIFY_SELCHANGE);
			if (uMsg == WM_RBUTTONDOWN)
				SendNotify(NOTIFY_RBUTTONDOWN);
		}
		return 0;

	case WM_LBUTTONDBLCLK:
		if (m_SelColor >= 0)
			SendNotify(NOTIFY_DOUBLECLICK);
		return 0;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case TTN_NEEDTEXT:
			{
				LPNMTTDISPINFO pttdi = reinterpret_cast<LPNMTTDISPINFO>(lParam);
				const int Index = static_cast<int>(pttdi->hdr.idFrom);

				pttdi->lpszText = pttdi->szText;
				pttdi->hinst = nullptr;
				if (Index >= 0 && Index < m_NumColors) {
					const int r = m_Palette[Index].rgbRed;
					const int g = m_Palette[Index].rgbGreen;
					const int b = m_Palette[Index].rgbBlue;
					StringFormat(pttdi->szText, TEXT("{0},{1},{2} #{0:02X}{1:02X}{2:02X}"), r, g, b);
				} else {
					pttdi->szText[0] = '\0';
				}
			}
			return 0;
		}
		break;

	case WM_DESTROY:
		m_Tooltip.Destroy();
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


} // namespace TVTest
