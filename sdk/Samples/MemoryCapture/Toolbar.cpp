#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <tchar.h>
#include "TVTestPlugin.h"
#include "Toolbar.h"


const LPCTSTR CToolbar::m_WindowClassName = TEXT("TVTest Memory Capture Toolbar");
HINSTANCE CToolbar::m_hinst = nullptr;


bool CToolbar::Initialize(HINSTANCE hinst)
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


CToolbar::CToolbar()
{
	CalcMetrics();
}


CToolbar::~CToolbar()
{
	if (m_hbmIcons != nullptr)
		::DeleteObject(m_hbmIcons);
}


bool CToolbar::Create(HWND hwndParent, int ID, TVTest::CTVTestApp *pApp)
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


void CToolbar::SetPosition(int Left, int Top, int Width, int Height)
{
	if (m_hwnd != nullptr)
		::MoveWindow(m_hwnd, Left, Top, Width, Height, TRUE);
}


void CToolbar::SetDPI(int DPI)
{
	m_DPI = DPI;
	CalcMetrics();

	if (m_hwnd != nullptr) {
		UpdateTooltips();
		::InvalidateRect(m_hwnd, nullptr, TRUE);
	}
}


int CToolbar::GetHeight() const
{
	return m_ItemHeight + m_Margin.top + m_Margin.bottom;
}


void CToolbar::AddItem(const ItemInfo &Item)
{
	m_ItemList.push_back(Item);
}


void CToolbar::SetIconImage(HBITMAP hbm, int Width, int Height)
{
	if (m_hbmIcons != nullptr)
		::DeleteObject(m_hbmIcons);
	m_hbmIcons = hbm;
	m_IconWidth = Width;
	m_IconHeight = Height;
	CalcMetrics();

	if (m_hwnd != nullptr) {
		UpdateTooltips();
		::InvalidateRect(m_hwnd, nullptr, TRUE);
	}
}


int CToolbar::GetPressingItem() const
{
	if (m_ClickItem < 0 || m_ClickItem >= (int)m_ItemList.size())
		return 0;
	return m_ItemList[m_ClickItem].ID;
}


void CToolbar::CalcMetrics()
{
	m_Margin.left = m_Margin.top = m_Margin.right = m_Margin.bottom = ::MulDiv(1, m_DPI, 96);
	m_ItemPadding = ::MulDiv(3, m_DPI, 96);
	m_ItemWidth = m_IconWidth + m_ItemPadding * 2;
	m_ItemHeight = m_IconHeight + m_ItemPadding * 2;
}


void CToolbar::Draw(HDC hdc, const RECT &rcPaint)
{
	RECT rcClient;

	::GetClientRect(m_hwnd, &rcClient);

	m_pApp->ThemeDrawBackground(L"status-bar.item.bottom", hdc, rcClient, m_DPI);

	for (int i = 0; i < (int)m_ItemList.size(); i++) {
		RECT rc;

		GetItemRect(i, &rc);
		if (rc.left >= rcPaint.right)
			break;
		if (rc.right > rcPaint.left) {
			LPCWSTR pszStyle = (i == m_HotItem) ? L"status-bar.item.hot" : L"status-bar.item.bottom";

			m_pApp->ThemeDrawBackground(pszStyle, hdc, rc, m_DPI);
			m_pApp->ThemeDrawIcon(
				pszStyle, hdc,
				rc.left + (m_ItemWidth - m_IconWidth) / 2,
				rc.top + (m_ItemHeight - m_IconHeight) / 2,
				m_IconWidth, m_IconHeight,
				m_hbmIcons,
				m_ItemList[i].Icon * m_IconWidth, 0, m_IconWidth, m_IconHeight);
		}
	}
}


void CToolbar::OnLButtonDown(int x, int y)
{
	int Item = GetItemFromPoint(x, y);

	SetHotItem(Item);

	if (Item >= 0) {
		m_ClickItem = Item;

		if ((m_ItemList[Item].Flags & ItemFlag_NotifyPress) != 0) {
			::SendMessage(
				::GetParent(m_hwnd), WM_COMMAND,
				MAKEWPARAM(GetWindowID(m_hwnd), Notify_ItemPressed),
				reinterpret_cast<LPARAM>(m_hwnd));
		}

		::SetCapture(m_hwnd);
	}
}


void CToolbar::OnLButtonUp(int x, int y)
{
	if (::GetCapture() == m_hwnd) {
		if (GetItemFromPoint(x, y) == m_ClickItem
				&& (m_ItemList[m_ClickItem].Flags & ItemFlag_NotifyPress) == 0)
			::SendMessage(::GetParent(m_hwnd), WM_COMMAND, m_ItemList[m_ClickItem].ID, 0);

		::ReleaseCapture();
	}
}


void CToolbar::OnMouseMove(int x, int y)
{
	int Item = GetItemFromPoint(x, y);

	if (::GetCapture() == m_hwnd) {
		if (m_ClickItem != Item)
			Item = -1;
	} else {
		TRACKMOUSEEVENT tme;

		tme.cbSize = sizeof(TRACKMOUSEEVENT);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = m_hwnd;
		::TrackMouseEvent(&tme);
	}

	SetHotItem(Item);
}


void CToolbar::GetItemRect(int Item, RECT *pRect) const
{
	pRect->left = Item * m_ItemWidth;
	pRect->top = m_Margin.top;
	pRect->right = pRect->left + m_ItemWidth;
	pRect->bottom = pRect->top + m_ItemHeight;
}


void CToolbar::RedrawItem(int Item) const
{
	RECT rc;

	GetItemRect(Item, &rc);
	::RedrawWindow(m_hwnd, &rc, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
}


int CToolbar::GetItemFromPoint(int x, int y) const
{
	if (y < m_Margin.top || y >= m_Margin.top + m_ItemHeight)
		return -1;
	if (x < m_Margin.left || x >= m_Margin.left + (int)m_ItemList.size() * m_ItemWidth)
		return -1;
	return (x - m_Margin.left) / m_ItemWidth;
}


void CToolbar::SetHotItem(int Item)
{
	if (m_HotItem != Item) {
		int OldItem = m_HotItem;

		m_HotItem = Item;
		if (OldItem >= 0)
			RedrawItem(OldItem);
		if (Item >= 0)
			RedrawItem(Item);
	}
}


// ツールチップを設定する
void CToolbar::SetTooltips()
{
	TOOLINFO ti = {};

	ti.cbSize = sizeof(TOOLINFO);
	ti.hwnd = m_hwnd;
	ti.uFlags = TTF_SUBCLASS | TTF_TRANSPARENT;
	ti.hinst = m_hinst;

	for (int i = 0; i < (int)m_ItemList.size(); i++) {
		ti.uId = i;
		GetItemRect(i, &ti.rect);
		ti.lpszText = MAKEINTRESOURCE(m_ItemList[i].ID);
		::SendMessage(m_hwndTooltips, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));
	}
}


// ツールチップの範囲を更新する
void CToolbar::UpdateTooltips()
{
	TOOLINFO ti = {};

	ti.cbSize = sizeof(TOOLINFO);
	ti.hwnd = m_hwnd;

	for (int i = 0; i < (int)m_ItemList.size(); i++) {
		ti.uId = i;
		GetItemRect(i, &ti.rect);
		::SendMessage(m_hwndTooltips, TTM_NEWTOOLRECT, 0, reinterpret_cast<LPARAM>(&ti));
	}
}


// ウィンドウハンドルからthisを取得する
CToolbar * CToolbar::GetThis(HWND hwnd)
{
	return reinterpret_cast<CToolbar *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
}


// ウィンドウプロシージャ
LRESULT CALLBACK CToolbar::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
			CToolbar *pThis = static_cast<CToolbar *>(pcs->lpCreateParams);

			pThis->m_hwnd = hwnd;
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));

			pThis->m_hwndTooltips =
				::CreateWindowEx(
					WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr, WS_POPUP,
					0, 0, 0, 0, hwnd, nullptr, m_hinst, nullptr);
			pThis->SetTooltips();

			pThis->m_HotItem = -1;
			pThis->m_ClickItem = -1;
		}
		return 0;

	case WM_PAINT:
		{
			CToolbar *pThis = GetThis(hwnd);
			PAINTSTRUCT ps;

			::BeginPaint(hwnd, &ps);
			pThis->Draw(ps.hdc, ps.rcPaint);
			::EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_LBUTTONDOWN:
		GetThis(hwnd)->OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_LBUTTONUP:
		GetThis(hwnd)->OnLButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_MOUSEMOVE:
		GetThis(hwnd)->OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_MOUSELEAVE:
		GetThis(hwnd)->SetHotItem(-1);
		return 0;

	case WM_CAPTURECHANGED:
		{
			CToolbar *pThis = GetThis(hwnd);

			if (pThis->m_ClickItem >= 0 && pThis->m_ClickItem < (int)pThis->m_ItemList.size()
					&& (pThis->m_ItemList[pThis->m_ClickItem].Flags & ItemFlag_NotifyPress) != 0) {
				::SendMessage(
					::GetParent(hwnd), WM_COMMAND,
					MAKEWPARAM(GetWindowID(hwnd), Notify_ItemReleased),
					reinterpret_cast<LPARAM>(hwnd));
			}
			pThis->m_ClickItem = -1;
		}
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT) {
			CToolbar *pThis = GetThis(hwnd);

			::SetCursor(::LoadCursor(nullptr, pThis->m_HotItem >= 0 ? IDC_HAND : IDC_ARROW));
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			CToolbar *pThis = GetThis(hwnd);

			if (pThis != nullptr) {
				pThis->m_hwnd = nullptr;
				pThis->m_hwndTooltips = nullptr;
			}
		}
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}
