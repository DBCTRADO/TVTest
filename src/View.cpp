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
#include "View.h"
#include "DrawUtil.h"
#include <cmath>
#include "Common/DebugDef.h"


namespace TVTest
{

namespace
{

const LPCTSTR VIEW_WINDOW_CLASS            = APP_NAME TEXT(" View");
const LPCTSTR VIDEO_CONTAINER_WINDOW_CLASS = APP_NAME TEXT(" Video Container");

}




HINSTANCE CVideoContainerWindow::m_hinst = nullptr;


CVideoContainerWindow::~CVideoContainerWindow()
{
	Destroy();
	if (m_pEventHandler != nullptr)
		m_pEventHandler->m_pVideoContainer = nullptr;
}


bool CVideoContainerWindow::Initialize(HINSTANCE hinst)
{
	if (m_hinst == nullptr) {
		WNDCLASS wc;

		wc.style = CS_DBLCLKS;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hinst;
		wc.hIcon = nullptr;
		wc.hCursor = nullptr;
		wc.hbrBackground = nullptr;
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = VIDEO_CONTAINER_WINDOW_CLASS;
		if (::RegisterClass(&wc) == 0)
			return false;
		m_hinst = hinst;
	}
	return true;
}


LRESULT CVideoContainerWindow::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rcDest;

			::BeginPaint(hwnd, &ps);
			const HBRUSH hbr = static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
			if (!m_pViewer->GetDestRect(&rcDest)
					|| ::IsRectEmpty(&rcDest)) {
				::FillRect(ps.hdc, &ps.rcPaint, hbr);
			} else {
				m_pViewer->RepaintVideo(hwnd, ps.hdc);
				RECT rc;
				::GetClientRect(hwnd, &rc);
				if (rc != rcDest)
					DrawUtil::FillBorder(ps.hdc, &rc, &rcDest, &ps.rcPaint, hbr);
			}
			::EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_MOVE:
		{
			const LibISDB::DirectShow::VideoRenderer::RendererType Renderer =
				m_pViewer->GetVideoRendererType();

			if (Renderer != LibISDB::DirectShow::VideoRenderer::RendererType::VMR7
					&& Renderer != LibISDB::DirectShow::VideoRenderer::RendererType::VMR9)
				break;
		}
		[[fallthrough]];
	case WM_SIZE:
		{
			const int Width = LOWORD(lParam), Height = HIWORD(lParam);

			m_pViewer->SetViewSize(Width, Height);
			if (m_pDisplayBase != nullptr)
				m_pDisplayBase->AdjustPosition();
			if (uMsg == WM_SIZE
					&& (Width != m_ClientSize.cx || Height != m_ClientSize.cy)) {
				if (m_pEventHandler != nullptr)
					m_pEventHandler->OnSizeChanged(Width, Height);
				m_ClientSize.cx = Width;
				m_ClientSize.cy = Height;
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
			if (uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN || uMsg == WM_MBUTTONDOWN)
				::SetFocus(hwnd);
			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			::MapWindowPoints(hwnd, ::GetParent(hwnd), &pt, 1);
			return ::SendMessage(::GetParent(hwnd), uMsg, wParam, MAKELONG(pt.x, pt.y));
		}

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		return ::SendMessage(::GetParent(hwnd), uMsg, wParam, lParam);
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


bool CVideoContainerWindow::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	return CreateBasicWindow(
		hwndParent, Style, ExStyle, ID,
		VIDEO_CONTAINER_WINDOW_CLASS, nullptr, m_hinst);
}


bool CVideoContainerWindow::Create(
	HWND hwndParent, DWORD Style, DWORD ExStyle, int ID, LibISDB::ViewerFilter *pViewer)
{
	m_pViewer = pViewer;
	if (!Create(hwndParent, Style, ExStyle, ID)) {
		m_pViewer = nullptr;
		return false;
	}
	return true;
}


void CVideoContainerWindow::SetDisplayBase(CDisplayBase *pDisplayBase)
{
	m_pDisplayBase = pDisplayBase;
}


void CVideoContainerWindow::SetEventHandler(CEventHandler *pEventHandler)
{
	if (m_pEventHandler != nullptr)
		m_pEventHandler->m_pVideoContainer = nullptr;
	if (pEventHandler != nullptr)
		pEventHandler->m_pVideoContainer = this;
	m_pEventHandler = pEventHandler;
}




CVideoContainerWindow::CEventHandler::~CEventHandler()
{
	if (m_pVideoContainer != nullptr)
		m_pVideoContainer->SetEventHandler(nullptr);
}




HINSTANCE CViewWindow::m_hinst = nullptr;


bool CViewWindow::Initialize(HINSTANCE hinst)
{
	if (m_hinst == nullptr) {
		WNDCLASS wc;

		wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hinst;
		wc.hIcon = nullptr;
		wc.hCursor = nullptr;
		wc.hbrBackground = nullptr;
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = VIEW_WINDOW_CLASS;
		if (::RegisterClass(&wc) == 0)
			return false;
		m_hinst = hinst;
	}
	return true;
}


CViewWindow::~CViewWindow()
{
	Destroy();
	if (m_pEventHandler != nullptr)
		m_pEventHandler->m_pView = nullptr;
	if (m_hbmLogo != nullptr)
		::DeleteObject(m_hbmLogo);
}


bool CViewWindow::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	return CreateBasicWindow(
		hwndParent, Style, ExStyle, ID,
		VIEW_WINDOW_CLASS, nullptr, m_hinst);
}


void CViewWindow::SetVideoContainer(CVideoContainerWindow *pVideoContainer)
{
	m_pVideoContainer = pVideoContainer;
	if (pVideoContainer != nullptr && m_hwnd != nullptr
			&& m_pVideoContainer->GetParent() == m_hwnd) {
		RECT rc;

		GetClientRect(&rc);
		CalcClientRect(&rc);
		pVideoContainer->SetPosition(&rc);
	}
}


void CViewWindow::SetMessageWindow(HWND hwnd)
{
	m_hwndMessage = hwnd;
}


void CViewWindow::SetEventHandler(CEventHandler *pEventHandler)
{
	if (m_pEventHandler != nullptr)
		m_pEventHandler->m_pView = nullptr;
	if (pEventHandler != nullptr)
		pEventHandler->m_pView = this;
	m_pEventHandler = pEventHandler;
}


bool CViewWindow::SetLogo(HBITMAP hbm)
{
	if (hbm == nullptr && m_hbmLogo == nullptr)
		return true;
	if (m_hbmLogo)
		::DeleteObject(m_hbmLogo);
	m_hbmLogo = hbm;
	if (m_hwnd)
		Redraw();
	return true;
}


void CViewWindow::SetBorder(const Theme::BorderStyle &Style)
{
	if (m_BorderStyle != Style) {
		const bool fResize =
			m_BorderStyle.Type != Style.Type
			&& (m_BorderStyle.Type == Theme::BorderType::None || Style.Type == Theme::BorderType::None);
		m_BorderStyle = Style;
		if (m_hwnd) {
			if (fResize)
				SendSizeMessage();
			Invalidate();
		}
	}
}


void CViewWindow::SetMargin(const Style::Margins &Margin)
{
	if (m_Margin != Margin) {
		m_Margin = Margin;
		SendSizeMessage();
	}
}


void CViewWindow::SetShowCursor(bool fShow)
{
	m_fShowCursor = fShow;
}


bool CViewWindow::CalcClientRect(RECT *pRect) const
{
	Theme::BorderStyle Border = m_BorderStyle;
	ConvertBorderWidthsInPixels(&Border);
	if (!Theme::SubtractBorderRect(Border, pRect))
		return false;
	Style::Subtract(pRect, m_Margin);
	return true;
}


bool CViewWindow::CalcWindowRect(RECT *pRect) const
{
	Theme::BorderStyle Border = m_BorderStyle;
	ConvertBorderWidthsInPixels(&Border);
	if (!Theme::AddBorderRect(m_BorderStyle, pRect))
		return false;
	Style::Add(pRect, m_Margin);
	return true;
}


LRESULT CViewWindow::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		InitializeUI();
		return 0;

	case WM_SIZE:
		{
			const int Width = LOWORD(lParam), Height = HIWORD(lParam);

			if (m_pVideoContainer != nullptr
					&& m_pVideoContainer->GetParent() == hwnd) {
				RECT rc;

				::SetRect(&rc, 0, 0, Width, Height);
				CalcClientRect(&rc);
				m_pVideoContainer->SetPosition(&rc);
			}
			if (m_pEventHandler != nullptr)
				m_pEventHandler->OnSizeChanged(Width, Height);
		}
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rcClient;
			const HBRUSH hbr = static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));

			::BeginPaint(hwnd, &ps);
			::GetClientRect(hwnd, &rcClient);
			if (m_hbmLogo) {
				RECT rcImage;
				BITMAP bm;

				::GetObject(m_hbmLogo, sizeof(BITMAP), &bm);
				rcImage.left = (rcClient.right - bm.bmWidth) / 2;
				rcImage.top = (rcClient.bottom - bm.bmHeight) / 2;
				rcImage.right = rcImage.left + bm.bmWidth;
				rcImage.bottom = rcImage.top + bm.bmHeight;
				DrawUtil::DrawBitmap(
					ps.hdc,
					rcImage.left, rcImage.top, bm.bmWidth, bm.bmHeight,
					m_hbmLogo);
				DrawUtil::FillBorder(ps.hdc, &rcClient, &rcImage, &ps.rcPaint, hbr);
			} else {
				::FillRect(ps.hdc, &ps.rcPaint, hbr);
			}
			{
				Theme::CThemeDraw ThemeDraw(BeginThemeDraw(ps.hdc));
				ThemeDraw.Draw(m_BorderStyle, rcClient);
			}
			::EndPaint(hwnd, &ps);
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
		if (m_hwndMessage != nullptr) {
			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			::MapWindowPoints(hwnd, m_hwndMessage, &pt, 1);
			return ::SendMessage(m_hwndMessage, uMsg, wParam, MAKELONG(pt.x, pt.y));
		}
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		if (m_hwndMessage != nullptr)
			return ::SendMessage(m_hwndMessage, uMsg, wParam, lParam);
		break;

	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT && HIWORD(lParam) != 0) {
			const HWND hwndCursor = reinterpret_cast<HWND>(wParam);

			if (hwndCursor == hwnd
					|| (m_pVideoContainer != nullptr
						&& hwndCursor == m_pVideoContainer->GetHandle())) {
				::SetCursor(m_fShowCursor ?::LoadCursor(nullptr, IDC_ARROW) : nullptr);
				return TRUE;
			}
		}
		break;
	}
	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}




CViewWindow::CEventHandler::~CEventHandler()
{
	if (m_pView != nullptr)
		m_pView->SetEventHandler(nullptr);
}




CDisplayView::~CDisplayView() = default;


bool CDisplayView::IsMessageNeed(const MSG *pMsg) const
{
	return false;
}


void CDisplayView::SetVisible(bool fVisible)
{
	if (m_pDisplayBase != nullptr)
		m_pDisplayBase->SetVisible(fVisible);
}


void CDisplayView::SetStyle(const Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);
}


void CDisplayView::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	m_Style.NormalizeStyle(pStyleManager, pStyleScaling);
}


void CDisplayView::SetDisplayVisible(bool fVisible)
{
	/*if (GetVisible() != fVisible)*/ {
		if (OnVisibleChange(fVisible)) {
			CBasicWindow::SetVisible(fVisible);
		}
	}
}


bool CDisplayView::OnVisibleChange(bool fVisible)
{
	return true;
}


bool CDisplayView::GetCloseButtonRect(RECT *pRect) const
{
	RECT rc;

	if (!GetClientRect(&rc))
		return false;
	pRect->right = rc.right - m_Style.CloseButtonMargin.Right;
	pRect->left = pRect->right - m_Style.CloseButtonSize.Width;
	pRect->top = m_Style.CloseButtonMargin.Top;
	pRect->bottom = pRect->top + m_Style.CloseButtonSize.Height;
	return true;
}


bool CDisplayView::CloseButtonHitTest(int x, int y) const
{
	RECT rc;

	if (!GetCloseButtonRect(&rc))
		return false;
	return ::PtInRect(&rc, POINT{x, y}) != FALSE;
}


void CDisplayView::DrawCloseButton(HDC hdc) const
{
	RECT rc;

	if (GetCloseButtonRect(&rc))
		::DrawFrameControl(hdc, &rc, DFC_CAPTION, DFCS_CAPTIONCLOSE | DFCS_MONO);
}


bool CDisplayView::GetItemStyle(ItemType Type, Theme::Style *pStyle) const
{
	switch (Type) {
	case ItemType::Normal:
	case ItemType::Normal1:
	case ItemType::Normal2:
		pStyle->Back.Fill.Type = Theme::FillType::Solid;
		if (Type != ItemType::Normal2) {
			pStyle->Back.Fill.Solid.Color.Set(48, 48, 48);
		} else {
			pStyle->Back.Fill.Solid.Color.Set(24, 24, 24);
		}
		pStyle->Back.Border.Type = Theme::BorderType::None;
		pStyle->Fore.Fill.Type = Theme::FillType::Solid;
		pStyle->Fore.Fill.Solid.Color.Set(255, 255, 255);
		break;

	case ItemType::Hot:
		pStyle->Back.Fill.Type = Theme::FillType::Gradient;
		pStyle->Back.Fill.Gradient.Type = Theme::GradientType::Normal;
		pStyle->Back.Fill.Gradient.Direction = Theme::GradientDirection::Vert;
		pStyle->Back.Fill.Gradient.Color1.Set(128, 128, 128);
		pStyle->Back.Fill.Gradient.Color2.Set(96, 96, 96);
		pStyle->Back.Border.Type = Theme::BorderType::Solid;
		pStyle->Back.Border.Color.Set(144, 144, 144);
		pStyle->Fore.Fill.Type = Theme::FillType::Solid;
		pStyle->Fore.Fill.Solid.Color.Set(255, 255, 255);
		break;

	case ItemType::Selected:
	case ItemType::Current:
		pStyle->Back.Fill.Type = Theme::FillType::Gradient;
		pStyle->Back.Fill.Gradient.Type = Theme::GradientType::Normal;
		pStyle->Back.Fill.Gradient.Direction = Theme::GradientDirection::Vert;
		pStyle->Back.Fill.Gradient.Color1.Set(96, 96, 96);
		pStyle->Back.Fill.Gradient.Color2.Set(128, 128, 128);
		if (Type == ItemType::Current) {
			pStyle->Back.Border.Type = Theme::BorderType::Solid;
			pStyle->Back.Border.Color.Set(144, 144, 144);
		} else {
			pStyle->Back.Border.Type = Theme::BorderType::None;
		}
		pStyle->Fore.Fill.Type = Theme::FillType::Solid;
		pStyle->Fore.Fill.Solid.Color.Set(255, 255, 255);
		break;

	default:
		return false;
	}

	return true;
}


bool CDisplayView::GetBackgroundStyle(BackgroundType Type, Theme::BackgroundStyle *pStyle) const
{
	switch (Type) {
	case BackgroundType::Content:
		pStyle->Fill.Type = Theme::FillType::Gradient;
		pStyle->Fill.Gradient.Direction = Theme::GradientDirection::Horz;
		pStyle->Fill.Gradient.Color1.Set(36, 36, 36);
		pStyle->Fill.Gradient.Color2.Set(16, 16, 16);
		pStyle->Border.Type = Theme::BorderType::None;
		break;

	case BackgroundType::Categories:
		pStyle->Fill.Type = Theme::FillType::Gradient;
		pStyle->Fill.Gradient.Direction = Theme::GradientDirection::Horz;
		pStyle->Fill.Gradient.Color1.Set(24, 24, 80);
		pStyle->Fill.Gradient.Color2.Set(24, 24, 32);
		pStyle->Border.Type = Theme::BorderType::None;
		break;

	default:
		return false;
	}

	return true;
}


int CDisplayView::GetDefaultFontSize(int Width, int Height) const
{
	int Size = std::min(Width / m_Style.TextSizeRatioHorz, Height / m_Style.TextSizeRatioVert);
	const double DPI = static_cast<double>(m_pStyleScaling->GetDPI());
	double Points = static_cast<double>(Size) * 72.0 / DPI;
	constexpr double BasePoints = 9.0;
	if (Points > BasePoints && m_Style.TextSizeScaleBase > 0) {
		Points = static_cast<int>(
			(std::log(Points - (BasePoints - 1.0)) /
			 std::log(static_cast<double>(m_Style.TextSizeScaleBase) * 0.01) +
			 ((BasePoints - 1.0) + 0.5)));
		Size = static_cast<int>(Points * DPI / 72.0 + 0.5);
	}
	if (Size < m_Style.TextSizeMin)
		Size = m_Style.TextSizeMin;
	else if (m_Style.TextSizeMax > 0 && Size > m_Style.TextSizeMax)
		Size = m_Style.TextSizeMax;
	return Size;
}


void CDisplayView::SetEventHandler(CEventHandler *pEventHandler)
{
	m_pEventHandler = pEventHandler;
}


bool CDisplayView::HandleMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
	*pResult = 0;

	if (m_pEventHandler == nullptr)
		return false;

	switch (Msg) {
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_LBUTTONDBLCLK:
		m_pEventHandler->OnMouseMessage(Msg, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return true;
	}

	return false;
}


LRESULT CDisplayView::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT Result;
	if (HandleMessage(hwnd, uMsg, wParam, lParam, &Result))
		return Result;

	return CCustomWindow::OnMessage(hwnd, uMsg, wParam, lParam);
}


CDisplayView::CEventHandler::~CEventHandler() = default;


void CDisplayView::DisplayViewStyle::SetStyle(const Style::CStyleManager *pStyleManager)
{
	Style::IntValue Value;

	*this = DisplayViewStyle();
	if (pStyleManager->Get(TEXT("display.text-size-ratio.horz"), &Value) && Value.Value > 0)
		TextSizeRatioHorz = Value;
	if (pStyleManager->Get(TEXT("display.text-size-ratio.vert"), &Value) && Value.Value > 0)
		TextSizeRatioVert = Value;
	pStyleManager->Get(TEXT("display.text-size-scale-base"), &TextSizeScaleBase);
	pStyleManager->Get(TEXT("display.text-size-min"), &TextSizeMin);
	pStyleManager->Get(TEXT("display.text-size-max"), &TextSizeMax);
	pStyleManager->Get(TEXT("display.content.margin"), &ContentMargin);
	pStyleManager->Get(TEXT("display.categories.margin"), &CategoriesMargin);
	pStyleManager->Get(TEXT("display.close-button"), &CloseButtonSize);
	pStyleManager->Get(TEXT("display.close-button.margin"), &CloseButtonMargin);
}


void CDisplayView::DisplayViewStyle::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	pStyleScaling->ToPixels(&TextSizeMin);
	pStyleScaling->ToPixels(&TextSizeMax);
	pStyleScaling->ToPixels(&ContentMargin);
	pStyleScaling->ToPixels(&CategoriesMargin);
	pStyleScaling->ToPixels(&CloseButtonSize);
	pStyleScaling->ToPixels(&CloseButtonMargin);
}




void CDisplayBase::SetEventHandler(CEventHandler *pHandler)
{
	m_pEventHandler = pHandler;
}


void CDisplayBase::SetParent(CBasicWindow *pWindow)
{
	m_pParentWindow = pWindow;
}


void CDisplayBase::SetDisplayView(CDisplayView *pView)
{
	if (m_pDisplayView == pView)
		return;
	if (m_pDisplayView != nullptr) {
		SetVisible(false);
		m_pDisplayView->m_pDisplayBase = nullptr;
	}
	m_pDisplayView = pView;
	if (pView != nullptr)
		pView->m_pDisplayBase = this;
	m_fVisible = false;
}


bool CDisplayBase::SetVisible(bool fVisible)
{
	if (m_pDisplayView == nullptr)
		return false;
	if (m_fVisible == fVisible)
		return true;

	const bool fFocus = !fVisible && m_pDisplayView->GetHandle() == ::GetFocus();

	if (m_pEventHandler != nullptr && !m_pEventHandler->OnVisibleChange(fVisible))
		return false;

	if (fVisible) {
		if (m_pParentWindow != nullptr) {
			RECT rc;
			m_pParentWindow->GetClientRect(&rc);
			m_pDisplayView->SetPosition(&rc);
		}
		m_pDisplayView->SetDisplayVisible(true);
		::BringWindowToTop(m_pDisplayView->GetHandle());
		m_pDisplayView->Update();
		::SetFocus(m_pDisplayView->GetHandle());
	} else {
		m_pDisplayView->SetDisplayVisible(false);
	}

	m_fVisible = fVisible;

	if (fFocus && m_pParentWindow != nullptr) {
		if (m_pParentWindow->GetVisible())
			::SetFocus(m_pParentWindow->GetHandle());
		else
			::SetFocus(::GetAncestor(m_pParentWindow->GetHandle(), GA_ROOT));
	}

	return true;
}


bool CDisplayBase::IsVisible() const
{
	return m_pDisplayView != nullptr && m_fVisible;
}


void CDisplayBase::AdjustPosition()
{
	if (m_pParentWindow != nullptr && m_pDisplayView != nullptr && m_fVisible) {
		RECT rc;
		m_pParentWindow->GetClientRect(&rc);
		m_pDisplayView->SetPosition(&rc);
	}
}


void CDisplayBase::SetPosition(int Left, int Top, int Width, int Height)
{
	if (m_pDisplayView != nullptr)
		m_pDisplayView->SetPosition(Left, Top, Width, Height);
}


void CDisplayBase::SetPosition(const RECT *pRect)
{
	if (m_pDisplayView != nullptr)
		m_pDisplayView->SetPosition(pRect);
}


void CDisplayBase::SetFocus()
{
	if (m_pDisplayView != nullptr && m_pDisplayView->GetVisible())
		::SetFocus(m_pDisplayView->GetHandle());
}




CDisplayBase::CEventHandler::~CEventHandler() = default;




void CDisplayEventHandlerBase::RelayMouseMessage(CDisplayView *pView, UINT Message, int x, int y)
{
	if (pView == nullptr)
		return;
	const HWND hwndParent = pView->GetParent();
	POINT pt = {x, y};
	::MapWindowPoints(pView->GetHandle(), hwndParent, &pt, 1);
	::SendMessage(hwndParent, Message, 0, MAKELPARAM(pt.x, pt.y));
}


} // namespace TVTest
