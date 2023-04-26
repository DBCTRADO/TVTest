#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <tchar.h>
#include "PreviewWindow.h"


const LPCTSTR CPreviewWindow::m_WindowClassName = TEXT("TVTest Memory Capture Preview Window");
HINSTANCE CPreviewWindow::m_hinst = nullptr;


bool CPreviewWindow::Initialize(HINSTANCE hinst)
{
	WNDCLASS wc = {};

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hinst;
	wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
	wc.lpszClassName = m_WindowClassName;

	if (::RegisterClass(&wc) == 0)
		return false;

	m_hinst = hinst;

	return true;
}


bool CPreviewWindow::Create(HWND hwndParent)
{
	return ::CreateWindowEx(
		0, m_WindowClassName, TEXT(""),
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
		0, 0, 0, 0,
		hwndParent, nullptr, m_hinst, this) != nullptr;
}


void CPreviewWindow::SetPosition(int Left, int Top, int Width, int Height)
{
	if (m_hwnd != nullptr)
		::MoveWindow(m_hwnd, Left, Top, Width, Height, TRUE);
}


void CPreviewWindow::SetImage(const CImage *pImage)
{
	m_pImage = pImage;

	if (m_hwnd != nullptr)
		::RedrawWindow(m_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
}


void CPreviewWindow::SetZoomRate(int Num, int Denom)
{
	if (Num < 1 || Denom < 1)
		return;

	m_ZoomNum = Num;
	m_ZoomDenom = Denom;
	m_fFitImageToWindow = false;

	if (m_hwnd != nullptr)
		::InvalidateRect(m_hwnd, nullptr, TRUE);
}


void CPreviewWindow::GetZoomRate(int *pNum, int *pDenom) const
{
	if (pNum != nullptr)
		*pNum = m_ZoomNum;
	if (pDenom != nullptr)
		*pDenom = m_ZoomDenom;
}


void CPreviewWindow::SetFitImageToWindow(bool fFit)
{
	if (m_fFitImageToWindow != fFit) {
		m_fFitImageToWindow = fFit;

		if (m_hwnd != nullptr)
			::InvalidateRect(m_hwnd, nullptr, TRUE);
	}
}


// 画像の表示サイズを取得する
bool CPreviewWindow::GetDisplaySize(int *pWidth, int *pHeight) const
{
	if (m_pImage == nullptr)
		return false;

	RECT rcClient;

	::GetClientRect(m_hwnd, &rcClient);
	if (rcClient.right < 1 || rcClient.bottom < 1)
		return false;

	int OrigWidth, OrigHeight, DispWidth, DispHeight;

	OrigWidth = m_pImage->GetDisplayWidth();
	OrigHeight = m_pImage->GetDisplayHeight();

	if (m_fFitImageToWindow) {
		if ((double)rcClient.right / (double)OrigWidth <= (double)rcClient.bottom / (double)OrigHeight) {
			DispWidth = rcClient.right;
			DispHeight = ::MulDiv(OrigHeight, rcClient.right, OrigWidth);
			if (DispHeight < 1)
				DispHeight = 1;
		} else {
			DispHeight = rcClient.bottom;
			DispWidth = ::MulDiv(OrigWidth, rcClient.bottom, OrigHeight);
			if (DispWidth < 1)
				DispWidth = 1;
		}
	} else {
		DispWidth = ::MulDiv(OrigWidth, m_ZoomNum, m_ZoomDenom);
		DispHeight = ::MulDiv(OrigHeight, m_ZoomNum, m_ZoomDenom);
	}

	if (pWidth != nullptr)
		*pWidth = DispWidth;
	if (pHeight != nullptr)
		*pHeight = DispHeight;

	return true;
}


// 描画
void CPreviewWindow::Draw(HDC hdc)
{
	RECT rcClient;

	::GetClientRect(m_hwnd, &rcClient);
	if (rcClient.right < 1 || rcClient.bottom < 1)
		return;

	HBRUSH hbrBack = static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));

	if (m_pImage != nullptr) {
		int DispWidth, DispHeight;

		GetDisplaySize(&DispWidth, &DispHeight);

		int x = (rcClient.right - DispWidth) / 2;
		int y = (rcClient.bottom - DispHeight) / 2;

		BITMAPINFO bmi = {};

		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = m_pImage->GetWidth();
		bmi.bmiHeader.biHeight = -m_pImage->GetHeight();	// トップダウン
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = m_pImage->GetBitsPerPixel();

		int OldStretchMode = ::SetStretchBltMode(hdc, STRETCH_HALFTONE);
		::StretchDIBits(
			hdc,
			x, y, DispWidth, DispHeight,
			0, 0, m_pImage->GetWidth(), m_pImage->GetHeight(),
			m_pImage->GetPixels(), &bmi, DIB_RGB_COLORS, SRCCOPY);
		::SetStretchBltMode(hdc, OldStretchMode);

		// 余白を塗りつぶす
		RECT rc = rcClient;
		if (y > 0) {
			rc.bottom = y;
			::FillRect(hdc, &rc, hbrBack);
		}
		if (y + DispHeight < rcClient.bottom) {
			rc.top = y + DispHeight;
			rc.bottom = rcClient.bottom;
			::FillRect(hdc, &rc, hbrBack);
		}
		rc.top = y;
		rc.bottom = y + DispHeight;
		if (x > 0) {
			rc.right = x;
			::FillRect(hdc, &rc, hbrBack);
		}
		if (x + DispWidth < rcClient.right) {
			rc.left = x + DispWidth;
			rc.right = rcClient.right;
			::FillRect(hdc, &rc, hbrBack);
		}
	} else {
		::FillRect(hdc, &rcClient, hbrBack);
	}
}


// ウィンドウハンドルからthisを取得する
CPreviewWindow * CPreviewWindow::GetThis(HWND hwnd)
{
	return reinterpret_cast<CPreviewWindow *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
}


// ウィンドウプロシージャ
LRESULT CALLBACK CPreviewWindow::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
			CPreviewWindow *pThis = static_cast<CPreviewWindow *>(pcs->lpCreateParams);

			pThis->m_hwnd = hwnd;
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
		}
		return 0;

	case WM_PAINT:
		{
			CPreviewWindow *pThis = GetThis(hwnd);
			PAINTSTRUCT ps;

			::BeginPaint(hwnd, &ps);
			pThis->Draw(ps.hdc);
			::EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_DESTROY:
		{
			CPreviewWindow *pThis = GetThis(hwnd);

			if (pThis != nullptr)
				pThis->m_hwnd = nullptr;
		}
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}
