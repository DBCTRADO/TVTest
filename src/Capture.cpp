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
#include "AppMain.h"
#include "Capture.h"
#include "Image.h"
#include "DrawUtil.h"
#include "Menu.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace
{

const LPCTSTR CAPTURE_WINDOW_CLASS         = APP_NAME TEXT(" Capture Window");
const LPCTSTR CAPTURE_PREVIEW_WINDOW_CLASS = APP_NAME TEXT(" Capture Preview");
const LPCTSTR CAPTURE_TITLE_TEXT = TEXT("キャプチャ");

enum {
	CAPTURE_ICON_CAPTURE,
	CAPTURE_ICON_SAVE,
	CAPTURE_ICON_COPY
};

}




CCaptureImage::CCaptureImage(HGLOBAL hData)
	: m_hData(hData)
{
	m_CaptureTime.NowLocal();
}


CCaptureImage::CCaptureImage(const BITMAPINFO *pbmi, const void *pBits)
{
	const size_t InfoSize = CalcDIBInfoSize(&pbmi->bmiHeader);
	const size_t BitsSize = CalcDIBBitsSize(&pbmi->bmiHeader);
	m_hData = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, InfoSize + BitsSize);
	if (m_hData != nullptr) {
		BYTE *pData = static_cast<BYTE*>(::GlobalLock(m_hData));

		if (pData != nullptr) {
			std::memcpy(pData, pbmi, InfoSize);
			std::memcpy(pData + InfoSize, pBits, BitsSize);
			::GlobalUnlock(m_hData);
		} else {
			::GlobalFree(m_hData);
			m_hData = nullptr;
		}
	}
	m_fLocked = false;
	m_CaptureTime.NowLocal();
}


CCaptureImage::~CCaptureImage()
{
	if (m_hData != nullptr) {
		if (m_fLocked)
			::GlobalUnlock(m_hData);
		::GlobalFree(m_hData);
	}
}


bool CCaptureImage::SetClipboard(HWND hwnd)
{
	if (m_hData == nullptr || m_fLocked)
		return false;

	const SIZE_T Size = ::GlobalSize(m_hData);
	const HGLOBAL hCopy = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, Size);
	if (hCopy == nullptr)
		return false;
	std::memcpy(::GlobalLock(hCopy), ::GlobalLock(m_hData), Size);
	::GlobalUnlock(hCopy);
	::GlobalUnlock(m_hData);
	if (!::OpenClipboard(hwnd)) {
		::GlobalFree(hCopy);
		return false;
	}
	::EmptyClipboard();
	const bool fOK = ::SetClipboardData(CF_DIB, hCopy) != nullptr;
	::CloseClipboard();
	return fOK;
}


bool CCaptureImage::GetBitmapInfoHeader(BITMAPINFOHEADER *pbmih) const
{
	if (m_hData == nullptr || m_fLocked)
		return false;

	const BITMAPINFOHEADER *pbmihSrc = static_cast<const BITMAPINFOHEADER*>(::GlobalLock(m_hData));

	if (pbmihSrc == nullptr)
		return false;
	*pbmih = *pbmihSrc;
	::GlobalUnlock(m_hData);
	return true;
}


bool CCaptureImage::LockData(BITMAPINFO **ppbmi, BYTE **ppBits)
{
	if (m_hData == nullptr || m_fLocked)
		return false;

	void *pDib = ::GlobalLock(m_hData);
	if (pDib == nullptr)
		return false;
	BITMAPINFO *pbmi = static_cast<BITMAPINFO*>(pDib);
	if (ppbmi != nullptr)
		*ppbmi = pbmi;
	if (ppBits != nullptr)
		*ppBits = static_cast<BYTE*>(pDib) + CalcDIBInfoSize(&pbmi->bmiHeader);
	m_fLocked = true;
	return true;
}


bool CCaptureImage::UnlockData()
{
	if (!m_fLocked)
		return false;
	::GlobalUnlock(m_hData);
	m_fLocked = false;
	return true;
}


void CCaptureImage::SetComment(LPCTSTR pszComment)
{
	StringUtility::Assign(m_Comment, pszComment);
}


LPCTSTR CCaptureImage::GetComment() const
{
	return StringUtility::GetCStrOrNull(m_Comment);
}




CCapturePreview::CEventHandler::~CEventHandler() = default;




HINSTANCE CCapturePreview::m_hinst = nullptr;


bool CCapturePreview::Initialize(HINSTANCE hinst)
{
	if (m_hinst == nullptr) {
		WNDCLASS wc;

		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hinst;
		wc.hIcon = nullptr;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = nullptr;
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = CAPTURE_PREVIEW_WINDOW_CLASS;
		if (::RegisterClass(&wc) == 0)
			return false;
		m_hinst = hinst;
	}
	return true;
}


CCapturePreview::~CCapturePreview()
{
	Destroy();
}


bool CCapturePreview::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	return CreateBasicWindow(
		hwndParent, Style, ExStyle, ID,
		CAPTURE_PREVIEW_WINDOW_CLASS, nullptr, m_hinst);
}


bool CCapturePreview::SetImage(const std::shared_ptr<CCaptureImage> &Image)
{
	ClearImage();
	m_Image = Image;
	if (m_hwnd != nullptr) {
		Invalidate();
		Update();
	}
	return true;
}


bool CCapturePreview::ClearImage()
{
	if (m_Image) {
		m_Image.reset();
		if (m_hwnd != nullptr) {
			Invalidate();
		}
	}
	return true;
}


bool CCapturePreview::HasImage() const
{
	return static_cast<bool>(m_Image);
}


bool CCapturePreview::SetEventHandler(CEventHandler *pEventHandler)
{
	if (m_pEventHandler != nullptr)
		m_pEventHandler->m_pCapturePreview = nullptr;
	if (pEventHandler != nullptr)
		pEventHandler->m_pCapturePreview = this;
	m_pEventHandler = pEventHandler;
	return true;
}


LRESULT CCapturePreview::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rc;
			BITMAPINFO *pbmi;
			BYTE *pBits;

			::BeginPaint(hwnd, &ps);
			GetClientRect(&rc);
			if (m_Image && m_Image->LockData(&pbmi, &pBits)) {
				int DstWidth = pbmi->bmiHeader.biWidth * rc.bottom / std::abs(pbmi->bmiHeader.biHeight);
				if (DstWidth > rc.right)
					DstWidth = rc.right;
				int DstHeight = pbmi->bmiHeader.biHeight * rc.right / pbmi->bmiHeader.biWidth;
				if (DstHeight > rc.bottom)
					DstHeight = rc.bottom;
				const int DstX = (rc.right - DstWidth) / 2;
				const int DstY = (rc.bottom - DstHeight) / 2;
				if (DstWidth > 0 && DstHeight > 0) {
					const int OldStretchBltMode = ::SetStretchBltMode(ps.hdc, STRETCH_HALFTONE);
					::StretchDIBits(ps.hdc, DstX, DstY, DstWidth, DstHeight,
									0, 0, pbmi->bmiHeader.biWidth, pbmi->bmiHeader.biHeight,
									pBits, pbmi, DIB_RGB_COLORS, SRCCOPY);
					::SetStretchBltMode(ps.hdc, OldStretchBltMode);
				}
				m_Image->UnlockData();
				const RECT rcDest = {DstX, DstY, DstX + DstWidth, DstY + DstHeight};
				DrawUtil::FillBorder(ps.hdc, &rc, &rcDest, &ps.rcPaint, m_crBackColor);
			} else {
				DrawUtil::Fill(ps.hdc, &rc, m_crBackColor);
			}
			::EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_LBUTTONDOWN:
		if (m_pEventHandler != nullptr)
			m_pEventHandler->OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return TRUE;

	case WM_RBUTTONUP:
		if (m_pEventHandler != nullptr)
			m_pEventHandler->OnRButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return TRUE;

	case WM_KEYDOWN:
		if (m_pEventHandler != nullptr
				&& m_pEventHandler->OnKeyDown(static_cast<UINT>(wParam), static_cast<UINT>(lParam)))
			return 0;
		break;
	}
	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}




CCaptureWindow::CEventHandler::~CEventHandler() = default;




HINSTANCE CCaptureWindow::m_hinst = nullptr;


bool CCaptureWindow::Initialize(HINSTANCE hinst)
{
	if (m_hinst == nullptr) {
		WNDCLASS wc;

		wc.style = 0;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hinst;
		wc.hIcon = nullptr;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1);
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = CAPTURE_WINDOW_CLASS;
		if (::RegisterClass(&wc) == 0)
			return false;
		m_hinst = hinst;
	}
	if (!CCapturePreview::Initialize(hinst))
		return false;
	return true;
}


CCaptureWindow::CCaptureWindow()
{
	m_WindowPosition.Width = 320;
	m_WindowPosition.Height = 240;

	RegisterUIChild(&m_Status);
	SetStyleScaling(&m_StyleScaling);
}


CCaptureWindow::~CCaptureWindow()
{
	Destroy();
}


bool CCaptureWindow::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	if (m_fCreateFirst) {
		if (m_pEventHandler != nullptr)
			m_pEventHandler->OnRestoreSettings();
		m_fCreateFirst = false;
	}

	return CreateBasicWindow(
		hwndParent, Style, ExStyle, ID,
		CAPTURE_WINDOW_CLASS, CAPTURE_TITLE_TEXT, m_hinst);
}


void CCaptureWindow::SetTheme(const Theme::CThemeManager *pThemeManager)
{
	m_Status.SetTheme(pThemeManager);
}


bool CCaptureWindow::SetImage(const BITMAPINFO *pbmi, const void *pBits)
{
	ClearImage();
	m_Image = std::make_shared<CCaptureImage>(pbmi, pBits);
	if (m_hwnd != nullptr) {
		m_Preview.SetImage(m_Image);
		SetTitle();
	}
	return true;
}


bool CCaptureWindow::SetImage(CCaptureImage *pImage)
{
	ClearImage();
	m_Image.reset(pImage);
	if (m_hwnd != nullptr) {
		m_Preview.SetImage(m_Image);
		SetTitle();
	}
	return true;
}


bool CCaptureWindow::ClearImage()
{
	if (m_Image) {
		m_Image.reset();
		if (m_hwnd != nullptr) {
			m_Preview.ClearImage();
			Invalidate();
			SetTitle();
		}
	}
	return true;
}


bool CCaptureWindow::HasImage() const
{
	return static_cast<bool>(m_Image);
}


bool CCaptureWindow::SetEventHandler(CEventHandler *pEventHandler)
{
	if (m_pEventHandler != nullptr)
		m_pEventHandler->m_pCaptureWindow = nullptr;
	if (pEventHandler != nullptr)
		pEventHandler->m_pCaptureWindow = this;
	m_pEventHandler = pEventHandler;
	return true;
}


void CCaptureWindow::ShowStatusBar(bool fShow)
{
	if (m_fShowStatusBar != fShow) {
		m_fShowStatusBar = fShow;
		if (m_hwnd != nullptr) {
			SendSizeMessage();
			m_Status.SetVisible(fShow);
		}
	}
}


void CCaptureWindow::SetTitle()
{
	if (m_hwnd != nullptr) {
		TCHAR szTitle[64];

		StringCopy(szTitle, CAPTURE_TITLE_TEXT);
		if (m_Image) {
			BITMAPINFOHEADER bmih;

			if (m_Image->GetBitmapInfoHeader(&bmih)) {
				StringFormat(
					szTitle,
					TEXT("{} - {} x {} ({} bpp)"),
					CAPTURE_TITLE_TEXT, bmih.biWidth, std::abs(bmih.biHeight), bmih.biBitCount);
			}
		}
		::SetWindowText(m_hwnd, szTitle);
	}
}


LRESULT CCaptureWindow::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		InitializeUI();

		m_Preview.Create(hwnd, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, WS_EX_CLIENTEDGE);
		m_Preview.SetEventHandler(&m_PreviewEventHandler);
		m_Status.Create(
			hwnd,
			WS_CHILD | WS_CLIPSIBLINGS | (m_fShowStatusBar ? WS_VISIBLE : 0),
			/*WS_EX_STATICEDGE*/0);
		//m_Status.SetEventHandler(pThis);
		if (m_Status.NumItems() == 0) {
			if (!m_StatusIcons.IsCreated()) {
				static const Theme::IconList::ResourceInfo ResourceList[] = {
					{MAKEINTRESOURCE(IDB_CAPTURE16), 16, 16},
					{MAKEINTRESOURCE(IDB_CAPTURE32), 32, 32},
				};
				const Style::Size IconSize = m_Status.GetIconSize();
				m_StatusIcons.Load(
					GetAppClass().GetResourceInstance(),
					IconSize.Width, IconSize.Height,
					ResourceList, lengthof(ResourceList));
			}
			m_Status.AddItem(new CCaptureStatusItem(this, m_StatusIcons));
			//m_Status.AddItem(new CContinuousStatusItem(m_StatusIcons));
			m_Status.AddItem(new CSaveStatusItem(this, m_StatusIcons));
			m_Status.AddItem(new CCopyStatusItem(this, m_StatusIcons));
		}
		if (m_Image) {
			m_Preview.SetImage(m_Image);
			SetTitle();
		}
		return 0;

	case WM_SIZE:
		{
			const int Width = LOWORD(lParam);
			int Height = HIWORD(lParam);

			if (m_fShowStatusBar) {
				Height -= m_Status.GetHeight();
				m_Status.SetPosition(0, Height, Width, m_Status.GetHeight());
			}
			m_Preview.SetPosition(0, 0, Width, Height);
		}
		return 0;

	case WM_KEYDOWN:
		if (m_pEventHandler != nullptr
				&& m_pEventHandler->OnKeyDown(static_cast<UINT>(wParam), static_cast<UINT>(lParam)))
			return 0;
		break;

	case WM_ACTIVATE:
		if (m_pEventHandler != nullptr
				&& m_pEventHandler->OnActivate(LOWORD(wParam) != WA_INACTIVE))
			return 0;
		break;

	case WM_DPICHANGED:
		OnDPIChanged(hwnd, wParam, lParam);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case CM_SAVEIMAGE:
			if (m_Image && m_pEventHandler != nullptr) {
				if (!m_pEventHandler->OnSave(m_Image.get())) {
					::MessageBox(
						hwnd, TEXT("画像の保存ができません。"), nullptr,
						MB_OK | MB_ICONEXCLAMATION);
				}
			}
			return 0;

		case CM_COPYIMAGE:
			if (m_Image) {
				if (!m_Image->SetClipboard(hwnd)) {
					::MessageBox(
						hwnd, TEXT("クリップボードにデータを設定できません。"), nullptr,
						MB_OK | MB_ICONEXCLAMATION);
				}
			}
			return 0;

		case CM_CAPTURESTATUSBAR:
			ShowStatusBar(!m_fShowStatusBar);
			return 0;
		}
		return 0;

	case WM_CLOSE:
		if (m_pEventHandler != nullptr
				&& !m_pEventHandler->OnClose())
			return 0;
		break;
	}
	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


void CCaptureWindow::RealizeStyle()
{
	if (m_hwnd != nullptr)
		SendSizeMessage();
}




CCaptureWindow::CPreviewEventHandler::CPreviewEventHandler(CCaptureWindow *pCaptureWindow)
	: m_pCaptureWindow(pCaptureWindow)
{
}


void CCaptureWindow::CPreviewEventHandler::OnRButtonUp(int x, int y)
{
	CPopupMenu Menu(GetAppClass().GetResourceInstance(), IDM_CAPTUREPREVIEW);

	Menu.EnableItem(CM_COPYIMAGE, m_pCaptureWindow->HasImage());
	Menu.EnableItem(CM_SAVEIMAGE, m_pCaptureWindow->HasImage());
	Menu.CheckItem(CM_CAPTURESTATUSBAR, m_pCaptureWindow->IsStatusBarVisible());
	POINT pt = {x, y};
	::ClientToScreen(m_pCapturePreview->GetHandle(), &pt);
	Menu.Show(m_pCaptureWindow->GetHandle(), &pt, TPM_RIGHTBUTTON);
}


bool CCaptureWindow::CPreviewEventHandler::OnKeyDown(UINT KeyCode, UINT Flags)
{
	::SendMessage(m_pCaptureWindow->GetHandle(), WM_KEYDOWN, KeyCode, Flags);
	return true;
}




CCaptureWindow::CCaptureStatusItem::CCaptureStatusItem(
	CCaptureWindow *pCaptureWindow, Theme::IconList &Icons)
	: CIconStatusItem(STATUS_ITEM_CAPTURE, pCaptureWindow->m_Status.GetIconSize().Width)
	, m_pCaptureWindow(pCaptureWindow)
	, m_Icons(Icons)
{
}

void CCaptureWindow::CCaptureStatusItem::Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags)
{
	DrawIcon(hdc, DrawRect, m_Icons, CAPTURE_ICON_CAPTURE);
}

void CCaptureWindow::CCaptureStatusItem::OnLButtonDown(int x, int y)
{
	GetAppClass().UICore.DoCommand(CM_CAPTURE);
}

void CCaptureWindow::CCaptureStatusItem::OnRButtonDown(int x, int y)
{
	/*
	POINT pt;
	UINT Flags;

	GetMenuPos(&pt, &Flags);
	GetAppClass().UICore.ShowSpecialMenu(CUICore::MENU_CAPTURE, &pt, Flags | TPM_RIGHTBUTTON);
	*/
}


CCaptureWindow::CSaveStatusItem::CSaveStatusItem(
	CCaptureWindow *pCaptureWindow, Theme::IconList &Icons)
	: CIconStatusItem(STATUS_ITEM_SAVE, pCaptureWindow->m_Status.GetIconSize().Width)
	, m_pCaptureWindow(pCaptureWindow)
	, m_Icons(Icons)
{
}

void CCaptureWindow::CSaveStatusItem::Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags)
{
	DrawIcon(hdc, DrawRect, m_Icons, CAPTURE_ICON_SAVE);
}

void CCaptureWindow::CSaveStatusItem::OnLButtonDown(int x, int y)
{
	m_pCaptureWindow->SendMessage(WM_COMMAND, CM_SAVEIMAGE, 0);
}


CCaptureWindow::CCopyStatusItem::CCopyStatusItem(
	CCaptureWindow *pCaptureWindow, Theme::IconList &Icons)
	: CIconStatusItem(STATUS_ITEM_COPY, pCaptureWindow->m_Status.GetIconSize().Width)
	, m_pCaptureWindow(pCaptureWindow)
	, m_Icons(Icons)
{
}

void CCaptureWindow::CCopyStatusItem::Draw(HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags)
{
	DrawIcon(hdc, DrawRect, m_Icons, CAPTURE_ICON_COPY);
}

void CCaptureWindow::CCopyStatusItem::OnLButtonDown(int x, int y)
{
	m_pCaptureWindow->SendMessage(WM_COMMAND, CM_COPYIMAGE, 0);
}


} // namespace TVTest
