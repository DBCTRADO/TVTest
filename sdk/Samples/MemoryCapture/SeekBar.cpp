#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include "TVTestPlugin.h"
#include "SeekBar.h"


const LPCTSTR CSeekBar::m_WindowClassName = TEXT("TVTest Memory Capture SeekBar");
HINSTANCE CSeekBar::m_hinst = nullptr;


bool CSeekBar::Initialize(HINSTANCE hinst)
{
	WNDCLASS wc = {};

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hinst;
	wc.lpszClassName = m_WindowClassName;

	if (::RegisterClass(&wc) == 0)
		return false;

	m_hinst = hinst;

	return true;
}


CSeekBar::CSeekBar()
{
	CalcMetrics();
}


bool CSeekBar::Create(HWND hwndParent, int ID, TVTest::CTVTestApp *pApp)
{
	m_pApp = pApp;

	return ::CreateWindowEx(
		0, m_WindowClassName, TEXT(""),
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
		0, 0, 0, 0,
		hwndParent,
		reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID)),
		m_hinst,
		this) != nullptr;
}


void CSeekBar::SetPosition(int Left, int Top, int Width, int Height)
{
	if (m_hwnd != nullptr)
		::MoveWindow(m_hwnd, Left, Top, Width, Height, TRUE);
}


void CSeekBar::SetDPI(int DPI)
{
	m_DPI = DPI;
	CalcMetrics();

	if (m_hwnd != nullptr)
		::InvalidateRect(m_hwnd, nullptr, TRUE);
}


int CSeekBar::GetHeight() const
{
	return m_BarHeight + (m_Margin * 2) + (m_BorderWidth * 4);
}


void CSeekBar::SetBarRange(int Min, int Max)
{
	m_Min = Min;
	m_Max = Max;

	if (m_hwnd != nullptr) {
		RECT rc = GetBarRect();
		::InvalidateRect(m_hwnd, &rc, TRUE);
	}
}


void CSeekBar::SetBarPos(int Pos)
{
	if (Pos < m_Min)
		Pos = m_Min;
	else if (Pos > m_Max)
		Pos = m_Max;

	if (m_Pos != Pos) {
		m_Pos = Pos;

		if (m_hwnd != nullptr) {
			RECT rc = GetBarRect();
			::InvalidateRect(m_hwnd, &rc, TRUE);
		}
	}
}


void CSeekBar::CalcMetrics()
{
	m_Margin = ::MulDiv(3, m_DPI, 96);
	m_BorderWidth = ::MulDiv(1, m_DPI, 96);
	m_BarHeight = ::MulDiv(8, m_DPI, 96);
}


void CSeekBar::Draw(HDC hdc)
{
	RECT rcClient;

	::GetClientRect(m_hwnd, &rcClient);

	m_pApp->ThemeDrawBackground(L"status-bar.item", hdc, rcClient, m_DPI);

	COLORREF crBar = m_pApp->GetColor(L"StatusText");

	RECT rc = rcClient;
	::InflateRect(&rc, -m_Margin, -m_Margin);

	LOGBRUSH lb;
	lb.lbStyle = BS_SOLID;
	lb.lbColor = crBar;
	lb.lbHatch = 0;
	HPEN hpen = ::ExtCreatePen(
		PS_GEOMETRIC | PS_SOLID | PS_INSIDEFRAME | PS_JOIN_MITER,
		m_BorderWidth, &lb, 0, nullptr);
	HGDIOBJ hOldPen = ::SelectObject(hdc, hpen);
	HGDIOBJ hOldBrush = ::SelectObject(hdc, ::GetStockObject(NULL_BRUSH));
	::Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
	::SelectObject(hdc, hOldBrush);
	::SelectObject(hdc, hOldPen);
	::DeleteObject(hpen);

	::InflateRect(&rc, -m_BorderWidth * 2, -m_BorderWidth * 2);

	if (m_Max >= m_Min && m_Pos >= m_Min && m_Pos <= m_Max) {
		int Range = m_Max - m_Min + 1;

		if (Range > 1) {
			int BarWidth = rc.right - rc.left;
			int KnobWidth = BarWidth / Range;
			int MinKnobWidth = ::MulDiv(4, m_DPI, 96);
			if (KnobWidth < MinKnobWidth)
				KnobWidth = MinKnobWidth;

			rc.left += (BarWidth - KnobWidth) * (m_Pos - m_Min) / (Range - 1);
			rc.right = rc.left + KnobWidth;
		}

		HBRUSH hbr = ::CreateSolidBrush(crBar);
		::FillRect(hdc, &rc, hbr);
		::DeleteObject(hbr);
	}
}


RECT CSeekBar::GetBarRect() const
{
	RECT rc;

	::GetClientRect(m_hwnd, &rc);
	::InflateRect(&rc, -(m_Margin + m_BorderWidth * 2), -(m_Margin + m_BorderWidth * 2));

	return rc;
}


void CSeekBar::OnLButtonDown(int x, int y)
{
	if (m_Max >= m_Min) {
		::SetCapture(m_hwnd);
		OnMouseMove(x, y);
	}
}


void CSeekBar::OnMouseMove(int x, int y)
{
	RECT rc = GetBarRect();
	m_fHot = (x >= rc.left) && (x < rc.right);

	if (::GetCapture() == m_hwnd) {
		int Pos;

		if (m_fHot && rc.right > rc.left + 1)
			Pos = m_Min + ((x - rc.left) * (m_Max - m_Min + 1) / (rc.right - rc.left - 1));
		else if (x <= rc.left)
			Pos = m_Min;
		else
			Pos = m_Max;

		if (Pos != m_Pos) {
			m_Pos = Pos;
			::InvalidateRect(m_hwnd, &rc, TRUE);
			::SendMessage(
				::GetParent(m_hwnd), WM_COMMAND,
				MAKEWPARAM(::GetWindowLong(m_hwnd, GWL_ID), Notify_PosChanged),
				reinterpret_cast<LPARAM>(m_hwnd));
		}
	}
}


// ウィンドウハンドルからthisを取得する
CSeekBar * CSeekBar::GetThis(HWND hwnd)
{
	return reinterpret_cast<CSeekBar *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
}


// ウィンドウプロシージャ
LRESULT CALLBACK CSeekBar::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
			CSeekBar *pThis = static_cast<CSeekBar *>(pcs->lpCreateParams);

			pThis->m_hwnd = hwnd;
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));

			pThis->m_fHot = false;
		}
		return 0;

	case WM_PAINT:
		{
			CSeekBar *pThis = GetThis(hwnd);
			PAINTSTRUCT ps;

			::BeginPaint(hwnd, &ps);
			pThis->Draw(ps.hdc);
			::EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_LBUTTONDOWN:
		GetThis(hwnd)->OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_LBUTTONUP:
		if (::GetCapture() == hwnd)
			::ReleaseCapture();
		return 0;

	case WM_MOUSEMOVE:
		GetThis(hwnd)->OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT) {
			CSeekBar *pThis = GetThis(hwnd);

			::SetCursor(::LoadCursor(nullptr, pThis->m_fHot ? IDC_HAND : IDC_ARROW));
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			CSeekBar *pThis = GetThis(hwnd);

			if (pThis != nullptr)
				pThis->m_hwnd = nullptr;
		}
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}
