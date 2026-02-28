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
#include "PseudoOSD.h"
#include "Graphics.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace
{

// タイマーの識別子
constexpr UINT TIMER_ID_HIDE      = 0x0001U;
constexpr UINT TIMER_ID_ANIMATION = 0x0002U;

constexpr int ANIMATION_FRAMES = 4;      // アニメーションの段階数
constexpr DWORD ANIMATION_INTERVAL = 50; // アニメーションの間隔

}




static float GetOutlineWidth(int FontSize)
{
	return static_cast<float>(FontSize) / 5.0f;
}




const LPCTSTR CPseudoOSD::m_pszWindowClass = APP_NAME TEXT(" Pseudo OSD");
HINSTANCE CPseudoOSD::m_hinst = nullptr;


bool CPseudoOSD::Initialize(HINSTANCE hinst)
{
	if (m_hinst == nullptr) {
		WNDCLASS wc;

		wc.style = CS_HREDRAW;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hinst;
		wc.hIcon = nullptr;
		wc.hCursor = nullptr;
		wc.hbrBackground = nullptr;
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = m_pszWindowClass;
		if (::RegisterClass(&wc) == 0)
			return false;
		m_hinst = hinst;
	}
	return true;
}


bool CPseudoOSD::IsPseudoOSD(HWND hwnd)
{
	TCHAR szClass[64];

	return ::GetClassName(hwnd, szClass, lengthof(szClass)) > 0
		&& ::lstrcmpi(szClass, m_pszWindowClass) == 0;
}


CPseudoOSD::CPseudoOSD()
{
	LOGFONT lf;
	DrawUtil::GetDefaultUIFont(&lf);
	m_Font.Create(&lf);
}


CPseudoOSD::~CPseudoOSD()
{
	Destroy();
}


bool CPseudoOSD::Create(HWND hwndParent, bool fLayeredWindow)
{
	if (m_hwnd != nullptr) {
		if (::GetParent(m_hwnd) == hwndParent
				&& m_fLayeredWindow == fLayeredWindow)
			return true;
		Destroy();
	}

	m_fLayeredWindow = fLayeredWindow;
	m_fPopupLayeredWindow =
		fLayeredWindow && !Util::OS::IsWindows8OrLater();
	m_hwndParent = hwndParent;

	if (m_fPopupLayeredWindow) {
		POINT pt = {m_Position.Left, m_Position.Top};
		::ClientToScreen(hwndParent, &pt);

		if (::CreateWindowEx(
					WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
					m_pszWindowClass, nullptr, WS_POPUP,
					pt.x, pt.y, m_Position.Width, m_Position.Height,
					hwndParent, nullptr, m_hinst, this) == nullptr)
			return false;

		RECT rc;
		::GetWindowRect(hwndParent, &rc);
		m_ParentPosition.x = rc.left;
		m_ParentPosition.y = rc.top;
		return true;
	}

	return ::CreateWindowEx(
		fLayeredWindow ? (WS_EX_LAYERED | WS_EX_TRANSPARENT) : 0,
		m_pszWindowClass, nullptr, WS_CHILD,
		m_Position.Left, m_Position.Top,
		m_Position.Width, m_Position.Height,
		hwndParent, nullptr, m_hinst, this) != nullptr;
}


bool CPseudoOSD::Destroy()
{
	if (m_hwnd != nullptr)
		::DestroyWindow(m_hwnd);
	return true;
}


bool CPseudoOSD::IsCreated() const
{
	return m_hwnd != nullptr;
}


bool CPseudoOSD::Show(DWORD Time, bool fAnimation)
{
	if (m_hwnd == nullptr)
		return false;

	if (m_fPopupLayeredWindow) {
		if (Time > 0) {
			POINT pt = {m_Position.Left, m_Position.Top};
			::ClientToScreen(m_hwndParent, &pt);
			m_Timer.BeginTimer(TIMER_ID_HIDE, Time);
			if (fAnimation) {
				m_AnimationCount = 0;
				::SetWindowPos(
					m_hwnd, nullptr, pt.x, pt.y,
					m_Position.Width / ANIMATION_FRAMES, m_Position.Height,
					SWP_NOZORDER | SWP_NOACTIVATE);
				m_Timer.BeginTimer(TIMER_ID_ANIMATION, ANIMATION_INTERVAL);
			} else {
				::SetWindowPos(
					m_hwnd, nullptr, pt.x, pt.y,
					m_Position.Width, m_Position.Height,
					SWP_NOZORDER | SWP_NOACTIVATE);
			}
		} else {
			m_Timer.EndTimer(TIMER_ID_HIDE);
		}
		UpdateLayeredWindow();
		::ShowWindow(m_hwnd, SW_SHOWNA);
		::UpdateWindow(m_hwnd);
		return true;
	}

	if (Time > 0) {
		m_Timer.BeginTimer(TIMER_ID_HIDE, Time);
		if (fAnimation) {
			m_AnimationCount = 0;
			::MoveWindow(
				m_hwnd, m_Position.Left, m_Position.Top,
				m_Position.Width / ANIMATION_FRAMES, m_Position.Height,
				TRUE);
			m_Timer.BeginTimer(TIMER_ID_ANIMATION, ANIMATION_INTERVAL);
		} else {
			::MoveWindow(
				m_hwnd, m_Position.Left, m_Position.Top,
				m_Position.Width, m_Position.Height, TRUE);
		}
	} else {
		m_Timer.EndTimer(TIMER_ID_HIDE);
	}
	if (m_fLayeredWindow) {
		UpdateLayeredWindow();
		::ShowWindow(m_hwnd, SW_SHOW);
		::BringWindowToTop(m_hwnd);
		::UpdateWindow(m_hwnd);
	} else {
		if (::IsWindowVisible(m_hwnd)) {
			::RedrawWindow(m_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
		} else {
			::ShowWindow(m_hwnd, SW_SHOW);
			::BringWindowToTop(m_hwnd);
			::UpdateWindow(m_hwnd);
		}
	}

	return true;
}


bool CPseudoOSD::Hide()
{
	if (m_hwnd == nullptr)
		return false;
	::ShowWindow(m_hwnd, SW_HIDE);
	m_Text.clear();
	m_hbm = nullptr;
	return true;
}


bool CPseudoOSD::IsVisible() const
{
	if (m_hwnd == nullptr)
		return false;
	//return ::IsWindowVisible(m_hwnd) != FALSE;
	return (GetWindowStyle(m_hwnd) & WS_VISIBLE) != 0;
}


bool CPseudoOSD::Update()
{
	if (m_hwnd != nullptr) {
		if (m_fLayeredWindow)
			UpdateLayeredWindow();
		else
			::RedrawWindow(m_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
	}

	return true;
}


bool CPseudoOSD::SetText(LPCTSTR pszText, HBITMAP hbmIcon, int IconWidth, int IconHeight, ImageEffect Effect)
{
	StringUtility::Assign(m_Text, pszText);
	m_hbmIcon = hbmIcon;
	if (hbmIcon != nullptr) {
		m_IconWidth = IconWidth;
		m_IconHeight = IconHeight;
		m_ImageEffect = Effect;
	} else {
		m_IconWidth = 0;
		m_IconHeight = 0;
	}
	m_hbm = nullptr;
	return true;
}


bool CPseudoOSD::SetPosition(int Left, int Top, int Width, int Height)
{
	if (Width <= 0 || Height <= 0)
		return false;

	m_Position.Left = Left;
	m_Position.Top = Top;
	m_Position.Width = Width;
	m_Position.Height = Height;

	if (m_hwnd != nullptr) {
		if (m_fPopupLayeredWindow) {
			POINT pt = {Left, Top};
			::ClientToScreen(m_hwndParent, &pt);
			::SetWindowPos(
				m_hwnd, nullptr, pt.x, pt.y, Width, Height,
				SWP_NOZORDER | SWP_NOACTIVATE);
		} else {
			::SetWindowPos(m_hwnd, HWND_TOP, Left, Top, Width, Height, 0);
		}
	}

	return true;
}


void CPseudoOSD::GetPosition(int *pLeft, int *pTop, int *pWidth, int *pHeight) const
{
	if (pLeft)
		*pLeft = m_Position.Left;
	if (pTop)
		*pTop = m_Position.Top;
	if (pWidth)
		*pWidth = m_Position.Width;
	if (pHeight)
		*pHeight = m_Position.Height;
}


void CPseudoOSD::SetTextColor(COLORREF crText)
{
	m_crTextColor = crText;
	/*
	if (m_hwnd != nullptr)
		::InvalidateRect(m_hwnd, nullptr, TRUE);
	*/
}


bool CPseudoOSD::SetTextHeight(int Height)
{
	LOGFONT lf;

	if (!m_Font.GetLogFont(&lf))
		return false;
	lf.lfWidth = 0;
	lf.lfHeight = -Height;
	return m_Font.Create(&lf);
}


bool CPseudoOSD::SetTextStyle(TextStyle Style)
{
	m_TextStyle = Style;
	return true;
}


bool CPseudoOSD::SetFont(const LOGFONT &Font)
{
	return m_Font.Create(&Font);
}


bool CPseudoOSD::CalcTextSize(SIZE *pSize)
{
	if (m_Text.empty()) {
		pSize->cx = 0;
		pSize->cy = 0;
		return true;
	}

	HDC hdc;
	bool fResult;

	if (m_hwnd != nullptr)
		hdc = ::GetDC(m_hwnd);
	else
		hdc = ::CreateCompatibleDC(nullptr);

	if (!m_fLayeredWindow) {
		const HFONT hfontOld = DrawUtil::SelectObject(hdc, m_Font);
		RECT rc = {0, 0, pSize->cx, 0};
		UINT Format = DT_CALCRECT | DT_NOPREFIX;
		if (!!(m_TextStyle & TextStyle::MultiLine))
			Format |= DT_WORDBREAK;
		else
			Format |= DT_SINGLELINE;
		fResult = ::DrawText(hdc, m_Text.data(), static_cast<int>(m_Text.length()), &rc, Format) != 0;
		if (fResult) {
			pSize->cx = rc.right;
			pSize->cy = rc.bottom;
		} else {
			pSize->cx = 0;
			pSize->cy = 0;
		}
		::SelectObject(hdc, hfontOld);
	} else {
		Graphics::CCanvas Canvas(hdc);
		LOGFONT lf;
		m_Font.GetLogFont(&lf);
		Graphics::CFont Font(lf);

		Graphics::TextFlag TextFlags =
			Graphics::TextFlag::Draw_Antialias | Graphics::TextFlag::Draw_Hinting;
		if (!(m_TextStyle & TextStyle::MultiLine))
			TextFlags |= Graphics::TextFlag::Format_NoWrap;

		if (!!(m_TextStyle & TextStyle::Outline)) {
			fResult = Canvas.GetOutlineTextSize(
				m_Text.c_str(), Font, GetOutlineWidth(std::abs(lf.lfHeight)), TextFlags, pSize);
		} else {
			fResult = Canvas.GetTextSize(
				m_Text.c_str(), Font, TextFlags, pSize);
		}
	}

	if (m_hwnd != nullptr)
		::ReleaseDC(m_hwnd, hdc);
	else
		::DeleteDC(hdc);

	return fResult;
}


bool CPseudoOSD::SetImage(HBITMAP hbm, ImageEffect Effect, ImageFlag Flags)
{
	m_hbm = hbm;
	m_Text.clear();
	m_hbmIcon = nullptr;
	m_ImageEffect = Effect;
	m_ImageFlags = Flags;
#if 0
	if (m_hwnd != nullptr) {
		/*
		BITMAP bm;

		::GetObject(m_hbm, sizeof(BITMAP), &bm);
		m_Position.Width = bm.bmWidth;
		m_Position.Height = bm.bmHeight;
		::MoveWindow(m_hwnd, Left, Top, bm.bmWidth, bm.bmHeight, TRUE);
		*/
		if (m_fLayeredWindow)
			UpdateLayeredWindow();
		else
			::RedrawWindow(m_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
	}
#endif
	return true;
}


void CPseudoOSD::OnParentMove()
{
	if (m_hwnd != nullptr && m_fPopupLayeredWindow) {
		RECT rcParent, rc;

		::GetWindowRect(m_hwndParent, &rcParent);
		::GetWindowRect(m_hwnd, &rc);
		::OffsetRect(
			&rc,
			rcParent.left - m_ParentPosition.x,
			rcParent.top - m_ParentPosition.y);
		::SetWindowPos(
			m_hwnd, nullptr, rc.left, rc.top,
			rc.right - rc.left, rc.bottom - rc.top,
			SWP_NOZORDER | SWP_NOACTIVATE);
		m_ParentPosition.x = rcParent.left;
		m_ParentPosition.y = rcParent.top;
	}
}


void CPseudoOSD::Draw(HDC hdc, const RECT &PaintRect) const
{
	RECT rc;

	::GetClientRect(m_hwnd, &rc);

	if (!m_Text.empty()) {
		DrawUtil::Fill(hdc, &PaintRect, m_crBackColor);

		if (m_hbmIcon != nullptr) {
			int IconWidth;
			if (m_Timer.IsTimerEnabled(TIMER_ID_ANIMATION))
				IconWidth = m_IconWidth * (m_AnimationCount + 1) / ANIMATION_FRAMES;
			else
				IconWidth = m_IconWidth;
			DrawUtil::DrawBitmap(
				hdc,
				0, (rc.bottom - m_IconHeight) / 2,
				IconWidth, m_IconHeight,
				m_hbmIcon);
			RECT rcIcon;
			rcIcon.left = 0;
			rcIcon.top = (rc.bottom - m_IconHeight) / 2;
			rcIcon.right = rcIcon.left + IconWidth;
			rcIcon.bottom = rcIcon.top + m_IconHeight;
			DrawImageEffect(hdc, &rcIcon);
			rc.left += IconWidth;
		}

		const HFONT hfontOld = DrawUtil::SelectObject(hdc, m_Font);
		const COLORREF crOldTextColor = ::SetTextColor(hdc, m_crTextColor);
		const int OldBkMode = ::SetBkMode(hdc, TRANSPARENT);

		UINT Format = DT_NOPREFIX;
		if (!!(m_TextStyle & TextStyle::MultiLine)) {
			Format |= DT_WORDBREAK;
			if (m_Timer.IsTimerEnabled(TIMER_ID_ANIMATION))
				rc.right = rc.left + m_Position.Width - m_IconWidth;
		} else {
			Format |= DT_SINGLELINE;
		}
		switch (m_TextStyle & TextStyle::HorzAlignMask) {
		case TextStyle::Right:      Format |= DT_RIGHT;  break;
		case TextStyle::HorzCenter: Format |= DT_CENTER; break;
		}
		const TextStyle VertAlign = m_TextStyle & TextStyle::VertAlignMask;
		if ((VertAlign == TextStyle::Bottom) || (VertAlign == TextStyle::VertCenter)) {
			RECT rcText = {0, 0, rc.right - rc.left, 0};
			::DrawText(hdc, m_Text.data(), static_cast<int>(m_Text.length()), &rcText, Format | DT_CALCRECT);
			if (rcText.bottom < rc.bottom - rc.top) {
				if (VertAlign == TextStyle::Bottom)
					rc.top = rc.bottom - rcText.bottom;
				else
					rc.top += ((rc.bottom - rc.top) - rcText.bottom) / 2;
			}
		}

		::DrawText(hdc, m_Text.data(), static_cast<int>(m_Text.length()), &rc, Format);

		::SetBkMode(hdc, OldBkMode);
		::SetTextColor(hdc, crOldTextColor);
		::SelectObject(hdc, hfontOld);
	} else if (m_hbm != nullptr) {
		BITMAP bm;
		::GetObject(m_hbm, sizeof(BITMAP), &bm);
		const RECT rcBitmap = {0, 0, bm.bmWidth, bm.bmHeight};
		DrawUtil::DrawBitmap(hdc, 0, 0, rc.right, rc.bottom, m_hbm, &rcBitmap);
		DrawImageEffect(hdc, &rc);
	}
}


void CPseudoOSD::DrawImageEffect(HDC hdc, const RECT *pRect) const
{
	if (!!(m_ImageEffect & ImageEffect::Gloss))
		DrawUtil::GlossOverlay(hdc, pRect);
	if (!!(m_ImageEffect & ImageEffect::Dark))
		DrawUtil::ColorOverlay(hdc, pRect, RGB(0, 0, 0), 64);
}


void CPseudoOSD::DrawImageEffect(Graphics::CCanvas &Canvas, const RECT *pRect) const
{
	if (!!(m_ImageEffect & ImageEffect::Gloss)) {
		RECT Rect = *pRect;
		Rect.bottom = (Rect.top + Rect.bottom) / 2;
		Canvas.FillGradient(
			Graphics::CColor(255, 255, 255, 192),
			Graphics::CColor(255, 255, 255, 32),
			Rect, Graphics::GradientDirection::Vert);
		Rect.top = Rect.bottom;
		Rect.bottom = pRect->bottom;
		Canvas.FillGradient(
			Graphics::CColor(0, 0, 0, 32),
			Graphics::CColor(0, 0, 0, 0),
			Rect, Graphics::GradientDirection::Vert);
	}

	if (!!(m_ImageEffect & ImageEffect::Dark)) {
		Graphics::CBrush Brush(0, 0, 0, 64);
		Canvas.FillRect(&Brush, *pRect);
	}
}


void CPseudoOSD::UpdateLayeredWindow()
{
	RECT rc;
	::GetWindowRect(m_hwnd, &rc);
	const int Width = rc.right - rc.left;
	const int Height = rc.bottom - rc.top;
	if (Width < 1 || Height < 1)
		return;

	const HDC hdc = ::GetDC(m_hwnd);
	const HDC hdcSrc = ::CreateCompatibleDC(hdc);
	DrawUtil::CBitmap Bitmap;
	HBITMAP hbmOld;

	if (m_hbm != nullptr && !!(m_ImageFlags & ImageFlag::DirectSource)) {
		hbmOld = static_cast<HBITMAP>(::SelectObject(hdcSrc, m_hbm));
	} else {
		Graphics::CImage CanvasImage;

		if (!CanvasImage.Create(Width, Height, 32))
			return;
		CanvasImage.Clear();

		{
			Graphics::CCanvas Canvas(&CanvasImage);

			::SetRect(&rc, 0, 0, Width, Height);

			if (!m_Text.empty()) {
				if (m_hbmIcon != nullptr) {
					Graphics::CImage Image;

					Image.CreateFromBitmap(m_hbmIcon);
					int IconWidth;
					if (m_Timer.IsTimerEnabled(TIMER_ID_ANIMATION))
						IconWidth = m_IconWidth * (m_AnimationCount + 1) / ANIMATION_FRAMES;
					else
						IconWidth = m_IconWidth;
					Canvas.DrawImage(
						0, (Height - m_IconHeight) / 2,
						IconWidth, m_IconHeight,
						&Image, 0, 0, m_IconWidth, m_IconHeight);
					RECT rcIcon;
					rcIcon.left = 0;
					rcIcon.top = (Height - m_IconHeight) / 2;
					rcIcon.right = rcIcon.left + IconWidth;
					rcIcon.bottom = rcIcon.top + m_IconHeight;
					if (rcIcon.top < 0)
						rcIcon.top = 0;
					if (rcIcon.right > Width)
						rcIcon.right = Width;
					if (rcIcon.bottom > Height)
						rcIcon.bottom = Height;
					DrawImageEffect(Canvas, &rcIcon);
					rc.left += IconWidth;
				}

				if (!!(m_TextStyle & TextStyle::FillBackground)) {
					Graphics::CBrush BackBrush(0, 0, 0, 128);
					Canvas.FillRect(&BackBrush, rc);
				}

				Graphics::CBrush TextBrush(
					GetRValue(m_crTextColor),
					GetGValue(m_crTextColor),
					GetBValue(m_crTextColor),
					255);
				LOGFONT lf;
				m_Font.GetLogFont(&lf);
				Graphics::CFont Font(lf);

				Graphics::TextFlag DrawTextFlags =
					Graphics::TextFlag::Draw_Antialias | Graphics::TextFlag::Draw_Hinting;
				if (!!(m_TextStyle & TextStyle::Right))
					DrawTextFlags |= Graphics::TextFlag::Format_Right;
				else if (!!(m_TextStyle & TextStyle::HorzCenter))
					DrawTextFlags |= Graphics::TextFlag::Format_HorzCenter;
				if (!!(m_TextStyle & TextStyle::Bottom))
					DrawTextFlags |= Graphics::TextFlag::Format_Bottom;
				if (!!(m_TextStyle & TextStyle::VertCenter))
					DrawTextFlags |= Graphics::TextFlag::Format_VertCenter;
				if (!(m_TextStyle & TextStyle::MultiLine))
					DrawTextFlags |= Graphics::TextFlag::Format_NoWrap;

				if (!!(m_TextStyle & TextStyle::MultiLine)
						&& m_Timer.IsTimerEnabled(TIMER_ID_ANIMATION))
					rc.right = rc.left + m_Position.Width - m_IconWidth;

				if (!!(m_TextStyle & TextStyle::Outline)) {
					Canvas.DrawOutlineText(
						m_Text.c_str(), Font, rc, &TextBrush,
						Graphics::CColor(0, 0, 0, 160),
						GetOutlineWidth(std::abs(lf.lfHeight)),
						DrawTextFlags);
				} else {
					Canvas.DrawText(m_Text.c_str(), Font, rc, &TextBrush, DrawTextFlags);
				}
			} else if (m_hbm != nullptr) {
				Graphics::CImage Image;
				if (Image.CreateFromBitmap(m_hbm)) {
					Canvas.DrawImage(&Image, 0, 0);
					const RECT rcImage = {0, 0, Image.GetWidth(), Image.GetHeight()};
					DrawImageEffect(Canvas, &rcImage);
				}
			}
		}

		Bitmap.Attach(CanvasImage.CreateBitmap());
		hbmOld = DrawUtil::SelectObject(hdcSrc, Bitmap);
	}

	SIZE sz = {Width, Height};
	POINT ptSrc = {0, 0};
	BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
	::UpdateLayeredWindow(m_hwnd, hdc, nullptr, &sz, hdcSrc, &ptSrc, 0, &blend, ULW_ALPHA);

	::SelectObject(hdcSrc, hbmOld);
	::DeleteDC(hdcSrc);
	::ReleaseDC(m_hwnd, hdc);
}


CPseudoOSD *CPseudoOSD::GetThis(HWND hwnd)
{
	return reinterpret_cast<CPseudoOSD*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
}


LRESULT CALLBACK CPseudoOSD::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			const CREATESTRUCT *pcs = reinterpret_cast<const CREATESTRUCT*>(lParam);
			CPseudoOSD *pThis = static_cast<CPseudoOSD*>(pcs->lpCreateParams);

			pThis->m_hwnd = hwnd;
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));

			pThis->m_Timer.InitializeTimer(hwnd);
		}
		return 0;

	case WM_SIZE:
		{
			CPseudoOSD *pThis = GetThis(hwnd);

			if (pThis->m_fLayeredWindow && ::IsWindowVisible(hwnd))
				pThis->UpdateLayeredWindow();
		}
		return 0;

	case WM_PAINT:
		{
			const CPseudoOSD *pThis = GetThis(hwnd);
			PAINTSTRUCT ps;

			::BeginPaint(hwnd, &ps);
			if (!pThis->m_fLayeredWindow)
				pThis->Draw(ps.hdc, ps.rcPaint);
			::EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_TIMER:
		{
			CPseudoOSD *pThis = GetThis(hwnd);

			switch (wParam) {
			case TIMER_ID_HIDE:
				pThis->Hide();
				pThis->m_Timer.EndTimer(TIMER_ID_HIDE);
				pThis->m_Timer.EndTimer(TIMER_ID_ANIMATION);
				break;

			case TIMER_ID_ANIMATION:
				pThis->m_AnimationCount++;
				if (pThis->m_fPopupLayeredWindow) {
					RECT rc;

					::GetWindowRect(hwnd, &rc);
					::SetWindowPos(
						hwnd, nullptr, rc.left, rc.top,
						pThis->m_Position.Width * (pThis->m_AnimationCount + 1) / ANIMATION_FRAMES,
						pThis->m_Position.Height,
						SWP_NOZORDER | SWP_NOACTIVATE);
				} else {
					::MoveWindow(
						hwnd, pThis->m_Position.Left, pThis->m_Position.Top,
						pThis->m_Position.Width * (pThis->m_AnimationCount + 1) / ANIMATION_FRAMES,
						pThis->m_Position.Height,
						TRUE);
				}
				::UpdateWindow(hwnd);
				if (pThis->m_AnimationCount + 1 == ANIMATION_FRAMES) {
					pThis->m_Timer.EndTimer(TIMER_ID_ANIMATION);
				}
				break;
			}
		}
		return 0;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_MOUSEMOVE:
		{
			CPseudoOSD *pThis = GetThis(hwnd);
			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			RECT rc;

			::MapWindowPoints(hwnd, pThis->m_hwndParent, &pt, 1);
			::GetClientRect(pThis->m_hwndParent, &rc);
			if (::PtInRect(&rc, pt))
				return ::SendMessage(pThis->m_hwndParent, uMsg, wParam, MAKELPARAM(pt.x, pt.y));
		}
		return 0;

	case WM_NCHITTEST:
		return HTTRANSPARENT;

	case WM_DESTROY:
		{
			CPseudoOSD *pThis = GetThis(hwnd);

			pThis->m_hwnd = nullptr;
		}
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


} // namespace TVTest
