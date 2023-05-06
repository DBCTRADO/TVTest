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
#include "InformationPanel.h"
#include "EpgUtil.h"
#include "EventInfoUtil.h"
#include "DarkMode.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace
{

constexpr int IDC_PROGRAMINFO = 1000;


constexpr UINT GetTooltipID(int Item, int Button)
{
	return ((Item + 1) << 8) | Button;
}

}




const LPCTSTR CInformationPanel::m_pszClassName = APP_NAME TEXT(" Information Panel");
HINSTANCE CInformationPanel::m_hinst = nullptr;


bool CInformationPanel::Initialize(HINSTANCE hinst)
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
		wc.lpszClassName = m_pszClassName;
		if (::RegisterClass(&wc) == 0)
			return false;
		m_hinst = hinst;
	}
	return true;
}


CInformationPanel::CInformationPanel()
	: CSettingsBase(TEXT("InformationPanel"))
{
	GetDefaultFont(&m_StyleFont);

	RegisterItem<CVideoInfoItem>();
	RegisterItem<CVideoDecoderItem>();
	RegisterItem<CVideoRendererItem>();
	RegisterItem<CAudioDeviceItem>(false);
	RegisterItem<CSignalLevelItem>();
	RegisterItem<CMediaBitRateItem>(false);
	RegisterItem<CErrorItem>();
	RegisterItem<CRecordItem>();
	RegisterItem<CServiceItem>(false);
	RegisterItem<CProgramInfoItem>();
}


CInformationPanel::~CInformationPanel()
{
	Destroy();
}


bool CInformationPanel::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	return CreateBasicWindow(
		hwndParent, Style, ExStyle, ID,
		m_pszClassName, TEXT("情報"), m_hinst);
}


void CInformationPanel::SetStyle(const Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);
}


void CInformationPanel::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	m_Style.NormalizeStyle(pStyleManager, pStyleScaling);
}


void CInformationPanel::SetTheme(const Theme::CThemeManager *pThemeManager)
{
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_PANEL_CONTENT,
		&m_Theme.Style);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_INFORMATIONPANEL_EVENTINFO,
		&m_Theme.ProgramInfoStyle);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_INFORMATIONPANEL_BUTTON,
		&m_Theme.ButtonStyle);
	pThemeManager->GetStyle(
		Theme::CThemeManager::STYLE_INFORMATIONPANEL_BUTTON_HOT,
		&m_Theme.ButtonHotStyle);

	if (m_hwnd != nullptr) {
		m_BackBrush.Create(m_Theme.Style.Back.Fill.GetSolidColor());
		m_ProgramInfoBackBrush.Create(m_Theme.ProgramInfoStyle.Back.Fill.GetSolidColor());

		Invalidate();

		if (m_hwndProgramInfo != nullptr) {
			if (IsDarkThemeSupported()) {
				SetWindowDarkTheme(m_hwndProgramInfo, IsDarkThemeStyle(m_Theme.ProgramInfoStyle.Back));
			}

			if (m_fUseRichEdit) {
				::SendMessage(
					m_hwndProgramInfo, EM_SETBKGNDCOLOR, 0,
					static_cast<COLORREF>(m_Theme.ProgramInfoStyle.Back.Fill.GetSolidColor()));
				POINT ptScroll;
				::SendMessage(m_hwndProgramInfo, EM_GETSCROLLPOS, 0, reinterpret_cast<LPARAM>(&ptScroll));
				UpdateProgramInfoText();
				::SendMessage(m_hwndProgramInfo, EM_SETSCROLLPOS, 0, reinterpret_cast<LPARAM>(&ptScroll));
			}
			::InvalidateRect(m_hwndProgramInfo, nullptr, TRUE);

			if (IsItemVisible(ITEM_PROGRAMINFO))
				SendSizeMessage();
		}
	}
}


bool CInformationPanel::SetFont(const Style::Font &Font)
{
	m_StyleFont = Font;

	if (m_hwnd != nullptr) {
		ApplyStyle();
		RealizeStyle();
	}

	return true;
}


bool CInformationPanel::IsVisible() const
{
	return m_hwnd != nullptr;
}


CInformationPanel::CItem *CInformationPanel::GetItem(int Item)
{
	if (Item < 0 || Item >= NUM_ITEMS)
		return nullptr;
	return m_ItemList[Item].get();
}


const CInformationPanel::CItem *CInformationPanel::GetItem(int Item) const
{
	if (Item < 0 || Item >= NUM_ITEMS)
		return nullptr;
	return m_ItemList[Item].get();
}


bool CInformationPanel::SetItemVisible(int Item, bool fVisible)
{
	CItem *pItem = GetItem(Item);

	if (pItem == nullptr)
		return false;

	if (pItem->IsVisible() != fVisible) {
		pItem->SetVisible(fVisible);

		if (m_hwnd != nullptr) {
			if (IsItemVisible(ITEM_PROGRAMINFO))
				SendSizeMessage();
			if (Item == ITEM_PROGRAMINFO)
				::ShowWindow(m_hwndProgramInfo, fVisible ? SW_SHOW : SW_HIDE);
			::InvalidateRect(m_hwnd, nullptr, TRUE);
		}
	}

	return true;
}


bool CInformationPanel::IsItemVisible(int Item) const
{
	const CItem *pItem = GetItem(Item);

	return pItem != nullptr && pItem->IsVisible();
}


bool CInformationPanel::UpdateItem(int Item)
{
	CItem *pItem = GetItem(Item);

	if (pItem == nullptr || !pItem->Update())
		return false;

	if (m_hwnd != nullptr && pItem->IsVisible())
		RedrawItem(Item);

	return true;
}


void CInformationPanel::RedrawItem(int Item)
{
	if (m_hwnd != nullptr && IsItemVisible(Item)) {
		RECT rc;

		GetItemRect(Item, &rc);
		if (Item == ITEM_PROGRAMINFO) {
			RECT rcClient;
			GetClientRect(&rcClient);
			rc.bottom = rcClient.bottom;
		}
		Invalidate(&rc);
	}
}


bool CInformationPanel::ResetItem(int Item)
{
	CItem *pItem = GetItem(Item);

	if (pItem == nullptr)
		return false;

	pItem->Reset();

	if (Item == ITEM_PROGRAMINFO) {
		m_RichEditLink.Reset();
		if (m_hwnd != nullptr)
			::SetWindowText(m_hwndProgramInfo, TEXT(""));
	}

	if (m_hwnd != nullptr && pItem->IsVisible())
		RedrawItem(Item);

	return true;
}


bool CInformationPanel::UpdateAllItems()
{
	bool fUpdated = false;

	for (int i = 0; i < NUM_ITEMS; i++) {
		if (m_ItemList[i]->Update())
			fUpdated = true;
	}

	if (!fUpdated)
		return false;

	if (m_hwnd != nullptr)
		Invalidate();

	return true;
}


bool CInformationPanel::SetProgramInfoRichEdit(bool fRichEdit)
{
	if (fRichEdit != m_fUseRichEdit) {
		if (m_hwndProgramInfo != nullptr) {
			::DestroyWindow(m_hwndProgramInfo);
			m_hwndProgramInfo = nullptr;
		}
		m_fUseRichEdit = fRichEdit;
		if (m_hwnd != nullptr) {
			CreateProgramInfoEdit();
			SendSizeMessage();
			if (IsItemVisible(ITEM_PROGRAMINFO))
				::ShowWindow(m_hwndProgramInfo, SW_SHOW);
		}
	}

	return true;
}


void CInformationPanel::UpdateProgramInfoText()
{
	if (m_hwndProgramInfo != nullptr) {
		const String &InfoText =
			static_cast<const CProgramInfoItem*>(m_ItemList[ITEM_PROGRAMINFO].get())->GetInfoText();

		if (m_fUseRichEdit) {
			::SendMessage(m_hwndProgramInfo, WM_SETREDRAW, FALSE, 0);
			::SetWindowText(m_hwndProgramInfo, TEXT(""));
			if (!InfoText.empty()) {
				LOGFONT lf;
				CHARFORMAT cf;
				const HDC hdc = ::GetDC(m_hwndProgramInfo);

				m_Font.GetLogFont(&lf);
				CRichEditUtil::LogFontToCharFormat(hdc, &lf, &cf);
				cf.dwMask |= CFM_COLOR;
				cf.crTextColor = m_Theme.ProgramInfoStyle.Fore.Fill.GetSolidColor();
				::ReleaseDC(m_hwndProgramInfo, hdc);
				CRichEditUtil::AppendText(m_hwndProgramInfo, InfoText.c_str(), &cf);
				m_RichEditLink.DetectURL(m_hwndProgramInfo, &cf);
				POINT pt = {0, 0};
				::SendMessage(m_hwndProgramInfo, EM_SETSCROLLPOS, 0, reinterpret_cast<LPARAM>(&pt));
			}
			::SendMessage(m_hwndProgramInfo, WM_SETREDRAW, TRUE, 0);
			::InvalidateRect(m_hwndProgramInfo, nullptr, TRUE);
		} else {
			::SetWindowText(m_hwndProgramInfo, InfoText.c_str());
		}
	}
}


bool CInformationPanel::CreateProgramInfoEdit()
{
	if (m_hwnd == nullptr || m_hwndProgramInfo != nullptr)
		return false;

	if (m_fUseRichEdit) {
		m_RichEditUtil.LoadRichEditLib();
		m_hwndProgramInfo = ::CreateWindowEx(
			0, m_RichEditUtil.GetWindowClassName(), TEXT(""),
			WS_CHILD | WS_CLIPSIBLINGS | WS_VSCROLL
				| ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_NOHIDESEL,
			0, 0, 0, 0, m_hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_PROGRAMINFO)), m_hinst, nullptr);
		if (m_hwndProgramInfo == nullptr)
			return false;
		CRichEditUtil::DisableAutoFont(m_hwndProgramInfo);
		::SendMessage(m_hwndProgramInfo, EM_SETEVENTMASK, 0, ENM_MOUSEEVENTS | ENM_LINK);
		::SendMessage(
			m_hwndProgramInfo, EM_SETBKGNDCOLOR, 0,
			static_cast<COLORREF>(m_Theme.ProgramInfoStyle.Back.Fill.GetSolidColor()));
	} else {
		m_hwndProgramInfo = ::CreateWindowEx(
			0, TEXT("EDIT"), TEXT(""),
			WS_CHILD | WS_CLIPSIBLINGS | WS_VSCROLL
				| ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
			0, 0, 0, 0, m_hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_PROGRAMINFO)), m_hinst, nullptr);
		if (m_hwndProgramInfo == nullptr)
			return false;
		m_ProgramInfoSubclass.SetSubclass(m_hwndProgramInfo);
	}

	if (IsDarkThemeSupported()) {
		SetWindowDarkTheme(m_hwndProgramInfo, IsDarkThemeStyle(m_Theme.ProgramInfoStyle.Back));
	}

	SetWindowFont(m_hwndProgramInfo, m_Font.GetHandle(), FALSE);
	UpdateProgramInfoText();

	return true;
}


LRESULT CInformationPanel::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			InitializeUI();

			m_BackBrush.Create(m_Theme.Style.Back.Fill.GetSolidColor());
			m_ProgramInfoBackBrush.Create(m_Theme.ProgramInfoStyle.Back.Fill.GetSolidColor());

			m_HotButton.Item = -1;
			m_HotButton.Button = -1;

			m_Tooltip.Create(hwnd);
			m_Tooltip.SetFont(m_Font.GetHandle());
			for (int i = 0; i < NUM_ITEMS; i++) {
				const CItem *pItem = m_ItemList[i].get();
				const int ButtonCount = pItem->GetButtonCount();
				for (int j = 0; j < ButtonCount; j++) {
					RECT rc;
					TCHAR szText[CCommandManager::MAX_COMMAND_TEXT];
					pItem->GetButtonRect(j, &rc);
					pItem->GetButtonTipText(j, szText, lengthof(szText));
					m_Tooltip.AddTool(GetTooltipID(i, j), rc, szText);
				}
			}

			const CProgramInfoItem *pProgramInfoItem =
				static_cast<CProgramInfoItem*>(GetItem(ITEM_PROGRAMINFO));
			CreateProgramInfoEdit();
			if (pProgramInfoItem->IsVisible())
				::ShowWindow(m_hwndProgramInfo, SW_SHOW);
		}
		return 0;

		HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);

	case WM_SIZE:
		{
			for (int i = 0; i < NUM_ITEMS; i++) {
				const CItem *pItem = m_ItemList[i].get();
				const int ButtonCount = pItem->GetButtonCount();
				for (int j = 0; j < ButtonCount; j++) {
					RECT rc;
					pItem->GetButtonRect(j, &rc);
					m_Tooltip.SetToolRect(GetTooltipID(i, j), rc);
				}
			}
		}

		if (IsItemVisible(ITEM_PROGRAMINFO)) {
			Theme::BorderStyle Style = m_Theme.ProgramInfoStyle.Back.Border;
			ConvertBorderWidthsInPixels(&Style);
			RECT rc;
			GetItemRect(ITEM_PROGRAMINFO, &rc);
			Theme::SubtractBorderRect(Style, &rc);
			::MoveWindow(
				m_hwndProgramInfo, rc.left, rc.top,
				rc.right - rc.left, rc.bottom - rc.top, TRUE);
		}
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd, &ps);
			Draw(ps.hdc, ps.rcPaint);
			::EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_CTLCOLORSTATIC:
		{
			const HDC hdc = reinterpret_cast<HDC>(wParam);

			::SetTextColor(hdc, m_Theme.ProgramInfoStyle.Fore.Fill.GetSolidColor());
			::SetBkColor(hdc, m_Theme.ProgramInfoStyle.Back.Fill.GetSolidColor());
			return reinterpret_cast<LRESULT>(m_ProgramInfoBackBrush.GetHandle());
		}

	case WM_LBUTTONDOWN:
		{
			const ItemButtonNumber Button = ButtonHitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

			if (Button.Item >= 0) {
				SetHotButton(Button);
				::SetCapture(hwnd);
			}
		}
		return 0;

	case WM_LBUTTONUP:
		if (::GetCapture() == hwnd) {
			if (m_HotButton.IsValid()) {
				SetHotButton(ButtonHitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
				const ItemButtonNumber Button = m_HotButton;
				::ReleaseCapture();
				if (Button.IsValid())
					m_ItemList[Button.Item]->OnButtonPushed(Button.Button);
			} else {
				::ReleaseCapture();
			}
		}
		return 0;

	case WM_RBUTTONUP:
		{
			const HMENU hmenu = ::LoadMenu(m_hinst, MAKEINTRESOURCE(IDM_INFORMATIONPANEL));

			for (int i = 0; i < NUM_ITEMS; i++) {
				CheckMenuItem(
					hmenu, CM_INFORMATIONPANEL_ITEM_FIRST + i,
					MF_BYCOMMAND | (IsItemVisible(i) ? MFS_CHECKED : MFS_UNCHECKED));
			}

			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			::ClientToScreen(hwnd, &pt);
			::TrackPopupMenu(::GetSubMenu(hmenu, 0), TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
			::DestroyMenu(hmenu);
		}
		return TRUE;

	case WM_MOUSEMOVE:
		{
			SetHotButton(ButtonHitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));

			if (::GetCapture() == nullptr) {
				TRACKMOUSEEVENT tme;
				tme.cbSize = sizeof(TRACKMOUSEEVENT);
				tme.dwFlags = TME_LEAVE;
				tme.hwndTrack = hwnd;
				::TrackMouseEvent(&tme);
			}
		}
		return 0;

	case WM_MOUSELEAVE:
	case WM_CAPTURECHANGED:
		if (m_HotButton.IsValid()) {
			SetHotButton(ItemButtonNumber());
		}
		return 0;

	case WM_SETCURSOR:
		if (reinterpret_cast<HWND>(wParam) == hwnd) {
			if (LOWORD(lParam) == HTCLIENT && m_HotButton.IsValid()) {
				::SetCursor(GetActionCursor());
				return TRUE;
			}
		} else if (m_RichEditLink.OnSetCursor(hwnd, wParam, lParam)) {
			return TRUE;
		}
		break;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case EN_MSGFILTER:
			{
				MSGFILTER *pMsgFilter = reinterpret_cast<MSGFILTER*>(lParam);

				switch (pMsgFilter->msg) {
				case WM_RBUTTONUP:
					EventInfoUtil::EventInfoContextMenu(hwnd, m_hwndProgramInfo);
					break;

				default:
					m_RichEditLink.OnMsgFilter(pMsgFilter);
					break;
				}
			}
			return 0;

#if 0
		case EN_LINK:
			{
				ENLINK *penl = reinterpret_cast<ENLINK*>(lParam);

				if (penl->msg == WM_LBUTTONUP)
					CRichEditUtil::HandleLinkClick(penl);
			}
			return 0;
#endif
		}
		break;

	case WM_DISPLAYCHANGE:
		m_Offscreen.Destroy();
		return 0;

	case WM_DESTROY:
		m_BackBrush.Destroy();
		m_ProgramInfoBackBrush.Destroy();
		m_Offscreen.Destroy();
		m_IconFont.Destroy();
		m_hwndProgramInfo = nullptr;
		m_RichEditLink.Reset();
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


void CInformationPanel::ApplyStyle()
{
	if (m_hwnd != nullptr) {
		CreateDrawFont(m_StyleFont, &m_Font);

		LOGFONT lf = {};
		lf.lfHeight = m_Style.ButtonSize.Height;
		lf.lfCharSet = SYMBOL_CHARSET;
		StringCopy(lf.lfFaceName, TEXT("Marlett"));
		m_IconFont.Create(&lf);

		CalcFontHeight();

		m_ItemButtonWidth = m_FontHeight * 15 / 10 + m_Style.ItemButtonPadding.Horz();
	}
}


void CInformationPanel::RealizeStyle()
{
	if (m_hwnd != nullptr) {
		if (m_fUseRichEdit) {
			SetWindowFont(m_hwndProgramInfo, m_Font.GetHandle(), FALSE);
			UpdateProgramInfoText();
		} else {
			SetWindowFont(m_hwndProgramInfo, m_Font.GetHandle(), TRUE);
		}
		SendSizeMessage();
		Invalidate();
		m_Tooltip.SetFont(m_Font.GetHandle());
	}
}


void CInformationPanel::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	if (id >= CM_INFORMATIONPANEL_ITEM_FIRST
			&& id < CM_INFORMATIONPANEL_ITEM_FIRST + NUM_ITEMS) {
		const int Item = id - CM_INFORMATIONPANEL_ITEM_FIRST;

		SetItemVisible(Item, !IsItemVisible(Item));
		return;
	}
}


void CInformationPanel::GetItemRect(int Item, RECT *pRect) const
{
	GetClientRect(pRect);
	int VisibleItemCount = 0;
	for (int i = 0; i < Item; i++) {
		if (IsItemVisible(i))
			VisibleItemCount++;
	}
	pRect->top = (m_FontHeight + m_Style.LineSpacing) * VisibleItemCount;
	if (!IsItemVisible(Item)) {
		pRect->bottom = pRect->top;
	} else {
		if (Item == ITEM_PROGRAMINFO)
			pRect->bottom -= m_Style.ButtonSize.Height;
		if (Item != ITEM_PROGRAMINFO || pRect->top >= pRect->bottom)
			pRect->bottom = pRect->top + m_FontHeight;
	}
}


void CInformationPanel::CalcFontHeight()
{
	const HDC hdc = ::GetDC(m_hwnd);

	if (hdc != nullptr) {
		m_FontHeight = m_Font.GetHeight(hdc);
		::ReleaseDC(m_hwnd, hdc);
	}
}


void CInformationPanel::Draw(HDC hdc, const RECT &PaintRect)
{
	RECT rc;

	GetClientRect(&rc);
	if (rc.right > m_Offscreen.GetWidth()
			|| m_FontHeight + m_Style.LineSpacing > m_Offscreen.GetHeight())
		m_Offscreen.Create(rc.right, m_FontHeight + m_Style.LineSpacing);
	HDC hdcDst = m_Offscreen.GetDC();
	if (hdcDst == nullptr)
		hdcDst = hdc;

	GetItemRect(ITEM_PROGRAMINFO, &rc);
	if (PaintRect.bottom > rc.bottom) {
		rc.left = PaintRect.left;
		rc.top = std::max(PaintRect.top, rc.bottom);
		rc.right = PaintRect.right;
		rc.bottom = PaintRect.bottom;
		::FillRect(hdc, &rc, m_BackBrush.GetHandle());
	}

	const HFONT hfontOld = DrawUtil::SelectObject(hdcDst, m_Font);

	for (int i = 0; i < NUM_ITEMS; i++) {
		if (GetDrawItemRect(i, &rc, PaintRect))
			m_ItemList[i]->Draw(hdc, rc);
	}

	if (IsItemVisible(ITEM_PROGRAMINFO)) {
		if (m_Theme.ProgramInfoStyle.Back.Border.Type != Theme::BorderType::None) {
			Theme::CThemeDraw ThemeDraw(BeginThemeDraw(hdc));

			GetItemRect(ITEM_PROGRAMINFO, &rc);
			ThemeDraw.Draw(m_Theme.ProgramInfoStyle.Back.Border, rc);
		}
	}

	::SelectObject(hdcDst, hfontOld);
}


bool CInformationPanel::GetDrawItemRect(int Item, RECT *pRect, const RECT &PaintRect) const
{
	if (!IsItemVisible(Item))
		return false;
	GetItemRect(Item, pRect);
	pRect->bottom += m_Style.LineSpacing;
	return IsRectIntersect(&PaintRect, pRect);
}


void CInformationPanel::DrawItem(
	CItem *pItem, HDC hdc, LPCTSTR pszText, const RECT &Rect)
{
	HDC hdcDst = nullptr;
	RECT rcDraw;

	if (pItem->IsSingleRow())
		hdcDst = m_Offscreen.GetDC();
	if (hdcDst != nullptr) {
		::SetRect(&rcDraw, 0, 0, Rect.right - Rect.left, Rect.bottom - Rect.top);
	} else {
		hdcDst = hdc;
		rcDraw = Rect;
	}
	Theme::CThemeDraw ThemeDraw(BeginThemeDraw(hdcDst));

	::FillRect(hdcDst, &rcDraw, m_BackBrush.GetHandle());

	const int ButtonCount = pItem->GetButtonCount();

	if (pszText != nullptr && pszText[0] != '\0') {
		RECT rc = rcDraw;
		if (ButtonCount > 0)
			rc.right -= ButtonCount * m_ItemButtonWidth + m_Style.ItemButtonMargin.Horz();
		ThemeDraw.Draw(m_Theme.Style.Fore, rc, pszText, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
	}

	for (int i = 0; i < ButtonCount; i++) {
		RECT rc;
		pItem->GetButtonRect(i, &rc);
		if (hdcDst != hdc)
			::OffsetRect(&rc, -Rect.left, -Rect.top);

		const bool fEnabled = pItem->IsButtonEnabled(i);
		const bool fHot = m_HotButton.Item == pItem->GetID() && m_HotButton.Button == i;
		const Theme::Style &Style =
			fEnabled && fHot ? m_Theme.ButtonHotStyle : m_Theme.ButtonStyle;

		ThemeDraw.Draw(Style.Back, &rc);
		RECT rcText = rc;
		Style::Subtract(&rcText, m_Style.ItemButtonPadding);
		Theme::ForegroundStyle Fore;
		if (fEnabled)
			Fore.Fill = Style.Fore.Fill;
		else
			Fore.Fill = Theme::MixStyle(Style.Fore.Fill, Style.Back.Fill);
		pItem->DrawButton(hdcDst, ThemeDraw, Fore, rc, rcText, i);
	}

	ThemeDraw.End();

	if (hdcDst != hdc)
		m_Offscreen.CopyTo(hdc, &Rect);
}


void CInformationPanel::RedrawButton(ItemButtonNumber Button)
{
	if (Button.IsValid() && IsItemVisible(Button.Item)) {
		RECT rc;
		if (m_ItemList[Button.Item]->GetButtonRect(Button.Button, &rc))
			Invalidate(&rc);
	}
}


CInformationPanel::ItemButtonNumber CInformationPanel::ButtonHitTest(int x, int y) const
{
	ItemButtonNumber ItemButton;

	for (int i = 0; i < NUM_ITEMS; i++) {
		if (IsItemVisible(i)) {
			const CItem *pItem = m_ItemList[i].get();
			const int Button = pItem->ButtonHitTest(x, y);
			if (Button >= 0 && pItem->IsButtonEnabled(Button)) {
				ItemButton.Item = i;
				ItemButton.Button = Button;
				break;
			}
		}
	}

	return ItemButton;
}


void CInformationPanel::SetHotButton(ItemButtonNumber Button)
{
	if (Button != m_HotButton) {
		if (m_HotButton.IsValid())
			RedrawButton(m_HotButton);
		m_HotButton = Button;
		if (m_HotButton.IsValid())
			RedrawButton(m_HotButton);
	}
}


bool CInformationPanel::ReadSettings(CSettings &Settings)
{
	for (int i = 0; i < NUM_ITEMS; i++) {
		CItem *pItem = m_ItemList[i].get();
		bool f;

		if (Settings.Read(pItem->GetName(), &f))
			pItem->SetVisible(f);
	}
	return true;
}


bool CInformationPanel::WriteSettings(CSettings &Settings)
{
	for (int i = 0; i < NUM_ITEMS; i++) {
		const CItem *pItem = m_ItemList[i].get();
		Settings.Write(pItem->GetName(), pItem->IsVisible());
	}
	return true;
}


CInformationPanel::CProgramInfoSubclass::CProgramInfoSubclass(CInformationPanel *pInfoPanel)
	: m_pInfoPanel(pInfoPanel)
{
}


LRESULT CInformationPanel::CProgramInfoSubclass::OnMessage(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_RBUTTONDOWN:
		return 0;

	case WM_RBUTTONUP:
		if (!static_cast<const CProgramInfoItem*>(m_pInfoPanel->GetItem(ITEM_PROGRAMINFO))->GetInfoText().empty()) {
			const HMENU hmenu = ::CreatePopupMenu();
			POINT pt;

			::AppendMenu(hmenu, MF_STRING | MF_ENABLED, 1, TEXT("コピー(&C)"));
			::AppendMenu(hmenu, MF_STRING | MF_ENABLED, 2, TEXT("すべて選択(&A)"));

			::GetCursorPos(&pt);
			const int Command = ::TrackPopupMenu(hmenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, nullptr);
			if (Command == 1) {
				DWORD Start, End;

				::SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
				::SendMessage(hwnd, EM_GETSEL, reinterpret_cast<WPARAM>(&Start), reinterpret_cast<LPARAM>(&End));
				if (Start == End)
					::SendMessage(hwnd, EM_SETSEL, 0, -1);
				::SendMessage(hwnd, WM_COPY, 0, 0);
				if (Start == End)
					::SendMessage(hwnd, EM_SETSEL, Start, End);
				::SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
			} else if (Command == 2) {
				::SendMessage(hwnd, EM_SETSEL, 0, -1);
			}
			::DestroyMenu(hmenu);
		}
		return 0;

	case WM_NCDESTROY:
		m_pInfoPanel->m_hwndProgramInfo = nullptr;
		break;
	}

	return CWindowSubclass::OnMessage(hwnd, uMsg, wParam, lParam);
}




void CInformationPanel::InformationPanelStyle::SetStyle(const Style::CStyleManager *pStyleManager)
{
	*this = InformationPanelStyle();
	pStyleManager->Get(TEXT("info-panel.button"), &ButtonSize);
	pStyleManager->Get(TEXT("info-panel.line-spacing"), &LineSpacing);
	pStyleManager->Get(TEXT("info-panel.item.button.margin"), &ItemButtonMargin);
	pStyleManager->Get(TEXT("info-panel.item.button.padding"), &ItemButtonPadding);
}


void CInformationPanel::InformationPanelStyle::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	pStyleScaling->ToPixels(&ButtonSize);
	pStyleScaling->ToPixels(&LineSpacing);
	pStyleScaling->ToPixels(&ItemButtonMargin);
	pStyleScaling->ToPixels(&ItemButtonPadding);
}




CInformationPanel::CItem::CItem(CInformationPanel *pPanel, bool fVisible, int PropertyID)
	: m_pPanel(pPanel)
	, m_fVisible(fVisible)
	, m_PropertyID(PropertyID)
{
}


void CInformationPanel::CItem::DrawItem(HDC hdc, const RECT &Rect, LPCTSTR pszText)
{
	m_pPanel->DrawItem(this, hdc, pszText, Rect);
}


void CInformationPanel::CItem::DrawButton(
	HDC hdc, Theme::CThemeDraw &ThemeDraw,
	const Theme::ForegroundStyle Style,
	const RECT &ButtonRect, const RECT &TextRect,
	int Button)
{
	if (Button == 0)
		ThemeDraw.Draw(Style, TextRect, TEXT("..."), DT_CENTER | DT_SINGLELINE | DT_BOTTOM);
}


int CInformationPanel::CItem::GetButtonCount() const
{
	return HasProperty() ? 1 : 0;
}


bool CInformationPanel::CItem::GetButtonRect(int Button, RECT *pRect) const
{
	if (HasProperty() && Button == 0) {
		m_pPanel->GetItemRect(GetID(), pRect);
		pRect->right -= m_pPanel->m_Style.ItemButtonMargin.Right;
		pRect->left = pRect->right - m_pPanel->m_ItemButtonWidth;
		pRect->top -= m_pPanel->m_Style.ItemButtonMargin.Top;
		pRect->bottom -= m_pPanel->m_Style.ItemButtonMargin.Bottom;
		return true;
	}
	return false;
}


bool CInformationPanel::CItem::IsButtonEnabled(int Button) const
{
	return Button == 0 && HasProperty()
		&& GetAppClass().UICore.GetCommandEnabledState(m_PropertyID);
}


bool CInformationPanel::CItem::OnButtonPushed(int Button)
{
	if (Button == 0 && HasProperty()) {
		GetAppClass().UICore.DoCommand(m_PropertyID);
		return true;
	}
	return false;
}


bool CInformationPanel::CItem::GetButtonTipText(int Button, LPTSTR pszText, int MaxText) const
{
	if (Button == 0 && HasProperty()) {
		return GetAppClass().CommandManager.GetCommandText(m_PropertyID, pszText, MaxText) > 0;
	}
	return false;
}


int CInformationPanel::CItem::ButtonHitTest(int x, int y) const
{
	const POINT pt = {x, y};
	const int ButtonCount = GetButtonCount();
	for (int i = 0; i < ButtonCount; i++) {
		RECT rc;
		GetButtonRect(i, &rc);
		if (::PtInRect(&rc, pt))
			return i;
	}
	return -1;
}


void CInformationPanel::CItem::UpdateItem()
{
	m_pPanel->UpdateItem(GetID());
}


void CInformationPanel::CItem::Redraw()
{
	m_pPanel->RedrawItem(GetID());
}




CInformationPanel::CVideoInfoItem::CVideoInfoItem(CInformationPanel *pPanel, bool fVisible)
	: CItemTemplate(pPanel, fVisible)
{
	Reset();
}


void CInformationPanel::CVideoInfoItem::Reset()
{
	m_VideoInfo = {};
}


bool CInformationPanel::CVideoInfoItem::Update()
{
	const CCoreEngine &CoreEngine = GetAppClass().CoreEngine;
	const LibISDB::ViewerFilter *pViewer =
		GetAppClass().CoreEngine.GetFilter<LibISDB::ViewerFilter>();

	VideoInfo NewInfo;
	NewInfo.OriginalWidth = CoreEngine.GetOriginalVideoWidth();
	NewInfo.OriginalHeight = CoreEngine.GetOriginalVideoHeight();
	NewInfo.DisplayWidth = CoreEngine.GetDisplayVideoWidth();
	NewInfo.DisplayHeight = CoreEngine.GetDisplayVideoHeight();
	if (pViewer == nullptr || !pViewer->GetEffectiveAspectRatio(&NewInfo.AspectX, &NewInfo.AspectY))
		NewInfo.AspectX = NewInfo.AspectY = 0;

	if (m_VideoInfo == NewInfo)
		return false;

	m_VideoInfo = NewInfo;

	return true;
}


void CInformationPanel::CVideoInfoItem::Draw(HDC hdc, const RECT &Rect)
{
	TCHAR szText[256];

	if (m_VideoInfo.OriginalWidth == m_VideoInfo.DisplayWidth
			&& m_VideoInfo.OriginalHeight == m_VideoInfo.DisplayHeight) {
		StringFormat(
			szText,
			TEXT("{} x {} ({}:{})"),
			m_VideoInfo.DisplayWidth, m_VideoInfo.DisplayHeight,
			m_VideoInfo.AspectX, m_VideoInfo.AspectY);
	} else {
		StringFormat(
			szText,
			TEXT("{} x {} [{} x {} ({}:{})]"),
			m_VideoInfo.OriginalWidth, m_VideoInfo.OriginalHeight,
			m_VideoInfo.DisplayWidth, m_VideoInfo.DisplayHeight,
			m_VideoInfo.AspectX, m_VideoInfo.AspectY);
	}

	DrawItem(hdc, Rect, szText);
}




CInformationPanel::CVideoDecoderItem::CVideoDecoderItem(CInformationPanel *pPanel, bool fVisible)
	: CItemTemplate(pPanel, fVisible, CM_VIDEODECODERPROPERTY)
{
}


void CInformationPanel::CVideoDecoderItem::Reset()
{
	m_VideoDecoderName.clear();
}


bool CInformationPanel::CVideoDecoderItem::Update()
{
	const LibISDB::ViewerFilter *pViewer =
		GetAppClass().CoreEngine.GetFilter<LibISDB::ViewerFilter>();
	LibISDB::String DecoderName;

	if (pViewer != nullptr && pViewer->GetVideoDecoderName(&DecoderName)) {
		if (m_VideoDecoderName == DecoderName)
			return false;
		m_VideoDecoderName = DecoderName;
	} else {
		if (m_VideoDecoderName.empty())
			return false;
		m_VideoDecoderName.clear();
	}

	return true;
}


void CInformationPanel::CVideoDecoderItem::Draw(HDC hdc, const RECT &Rect)
{
	DrawItem(hdc, Rect, m_VideoDecoderName.c_str());
}




CInformationPanel::CVideoRendererItem::CVideoRendererItem(CInformationPanel *pPanel, bool fVisible)
	: CItemTemplate(pPanel, fVisible, CM_VIDEORENDERERPROPERTY)
{
}


void CInformationPanel::CVideoRendererItem::Reset()
{
	m_VideoRendererName.clear();
}


bool CInformationPanel::CVideoRendererItem::Update()
{
	const LibISDB::ViewerFilter *pViewer =
		GetAppClass().CoreEngine.GetFilter<LibISDB::ViewerFilter>();
	LibISDB::String RendererName;

	if (pViewer != nullptr && pViewer->GetVideoRendererName(&RendererName)) {
		if (m_VideoRendererName == RendererName)
			return false;
		m_VideoRendererName = RendererName;
	} else {
		if (m_VideoRendererName.empty())
			return false;
		m_VideoRendererName.clear();
	}

	return true;
}


void CInformationPanel::CVideoRendererItem::Draw(HDC hdc, const RECT &Rect)
{
	DrawItem(hdc, Rect, m_VideoRendererName.c_str());
}




CInformationPanel::CAudioDeviceItem::CAudioDeviceItem(CInformationPanel *pPanel, bool fVisible)
	: CItemTemplate(pPanel, fVisible, CM_AUDIORENDERERPROPERTY)
{
}


void CInformationPanel::CAudioDeviceItem::Reset()
{
	m_AudioDeviceName.clear();
}


bool CInformationPanel::CAudioDeviceItem::Update()
{
	const LibISDB::ViewerFilter *pViewer =
		GetAppClass().CoreEngine.GetFilter<LibISDB::ViewerFilter>();
	LibISDB::String DeviceName;

	if (pViewer != nullptr && pViewer->GetAudioRendererName(&DeviceName)) {
		if (m_AudioDeviceName == DeviceName)
			return false;
		m_AudioDeviceName = DeviceName;
	} else {
		if (m_AudioDeviceName.empty())
			return false;
		m_AudioDeviceName.clear();
	}

	return true;
}


void CInformationPanel::CAudioDeviceItem::Draw(HDC hdc, const RECT &Rect)
{
	DrawItem(hdc, Rect, m_AudioDeviceName.c_str());
}




CInformationPanel::CSignalLevelItem::CSignalLevelItem(CInformationPanel *pPanel, bool fVisible)
	: CItemTemplate(pPanel, fVisible)
{
	Reset();
}


void CInformationPanel::CSignalLevelItem::Reset()
{
	m_SignalLevel = 0.0f;
	m_BitRate = 0;
}


bool CInformationPanel::CSignalLevelItem::Update()
{
	const CCoreEngine &CoreEngine = GetAppClass().CoreEngine;
	const float SignalLevel =
		m_fShowSignalLevel ? CoreEngine.GetSignalLevel() : 0.0f;
	const DWORD BitRate = CoreEngine.GetBitRate();

	if (SignalLevel == m_SignalLevel && BitRate == m_BitRate)
		return false;

	m_SignalLevel = SignalLevel;
	m_BitRate = BitRate;

	return true;
}


void CInformationPanel::CSignalLevelItem::Draw(HDC hdc, const RECT &Rect)
{
	const CCoreEngine &CoreEngine = GetAppClass().CoreEngine;
	TCHAR szText[64];
	size_t Length = 0;

	if (m_fShowSignalLevel) {
		TCHAR szSignalLevel[32];
		CoreEngine.GetSignalLevelText(m_SignalLevel, szSignalLevel, lengthof(szSignalLevel));
		Length = StringFormat(szText, TEXT("{} / "), szSignalLevel);
	}
	CoreEngine.GetBitRateText(m_BitRate, szText + Length, static_cast<int>(lengthof(szText) - Length));

	DrawItem(hdc, Rect, szText);
}


void CInformationPanel::CSignalLevelItem::ShowSignalLevel(bool fShow)
{
	if (m_fShowSignalLevel != fShow) {
		m_fShowSignalLevel = fShow;
		Update();
		Redraw();
	}
}




CInformationPanel::CMediaBitRateItem::CMediaBitRateItem(CInformationPanel *pPanel, bool fVisible)
	: CItemTemplate(pPanel, fVisible)
{
	Reset();
}


void CInformationPanel::CMediaBitRateItem::Reset()
{
	m_VideoBitRate = 0;
	m_AudioBitRate = 0;
}


bool CInformationPanel::CMediaBitRateItem::Update()
{
	const LibISDB::TSPacketCounterFilter *pPacketCounter =
		GetAppClass().CoreEngine.GetFilter<LibISDB::TSPacketCounterFilter>();
	DWORD VideoBitRate, AudioBitRate;

	if (pPacketCounter != nullptr) {
		VideoBitRate = pPacketCounter->GetVideoBitRate();
		AudioBitRate = pPacketCounter->GetAudioBitRate();
	} else {
		VideoBitRate = 0;
		AudioBitRate = 0;
	}

	if (VideoBitRate == m_VideoBitRate && AudioBitRate == m_AudioBitRate)
		return false;

	m_VideoBitRate = VideoBitRate;
	m_AudioBitRate = AudioBitRate;

	return true;
}


void CInformationPanel::CMediaBitRateItem::Draw(HDC hdc, const RECT &Rect)
{
	TCHAR szText[64];
	size_t Length;

	if (m_VideoBitRate < 1000 * 1000) {
		Length = StringFormat(
			szText,
			TEXT("映像 {} kbps"),
			(m_VideoBitRate + 500) / 1000);
	} else {
		Length = StringFormat(
			szText,
			TEXT("映像 {:.2f} Mbps"),
			static_cast<double>(m_VideoBitRate) / static_cast<double>(1000 * 1000));
	}
	StringFormat(
		szText + Length, lengthof(szText) - Length,
		TEXT(" / 音声 {} kbps"),
		(m_AudioBitRate + 500) / 1000);

	DrawItem(hdc, Rect, szText);
}




CInformationPanel::CErrorItem::CErrorItem(CInformationPanel *pPanel, bool fVisible)
	: CItemTemplate(pPanel, fVisible)
{
	Reset();
}


void CInformationPanel::CErrorItem::Reset()
{
	m_fShowScramble = true;
	m_ContinuityErrorPacketCount = 0;
	m_ErrorPacketCount = 0;
	m_ScramblePacketCount = 0;
}


bool CInformationPanel::CErrorItem::Update()
{
	const CCoreEngine &CoreEngine = GetAppClass().CoreEngine;
	const ULONGLONG ContinuityErrorPacketCount = CoreEngine.GetContinuityErrorPacketCount();
	const ULONGLONG ErrorPacketCount = CoreEngine.GetErrorPacketCount();
	const ULONGLONG ScramblePacketCount = CoreEngine.GetScramblePacketCount();

	if (ContinuityErrorPacketCount == m_ContinuityErrorPacketCount
			&& ErrorPacketCount == m_ErrorPacketCount
			&& ScramblePacketCount == m_ScramblePacketCount)
		return false;

	m_ContinuityErrorPacketCount = ContinuityErrorPacketCount;
	m_ErrorPacketCount = ErrorPacketCount;
	m_ScramblePacketCount = ScramblePacketCount;

	return true;
}


void CInformationPanel::CErrorItem::Draw(HDC hdc, const RECT &Rect)
{
	TCHAR szText[256];
	const size_t Length = StringFormat(
		szText,
		TEXT("D {} / E {}"),
		m_ContinuityErrorPacketCount,
		m_ErrorPacketCount);
	if (m_fShowScramble) {
		StringFormat(
			szText + Length, lengthof(szText) - Length,
			TEXT(" / S {}"), m_ScramblePacketCount);
	}
	DrawItem(hdc, Rect, szText);
}




CInformationPanel::CRecordItem::CRecordItem(CInformationPanel *pPanel, bool fVisible)
	: CItemTemplate(pPanel, fVisible)
{
	Reset();
}


void CInformationPanel::CRecordItem::Reset()
{
	m_fRecording = false;
}


bool CInformationPanel::CRecordItem::Update()
{
	const CAppMain &App = GetAppClass();
	const bool fRecording = App.RecordManager.IsRecording();
	LONGLONG WroteSize;
	CRecordTask::DurationType RecordTime;
	LONGLONG DiskFreeSpace;

	if (fRecording) {
		const CRecordTask *pRecordTask = App.RecordManager.GetRecordTask();
		WroteSize = pRecordTask->GetWroteSize();
		RecordTime = pRecordTask->GetRecordTime();
		DiskFreeSpace = pRecordTask->GetFreeSpace();
	}

	if (fRecording == m_fRecording
			&& (!fRecording
				|| (WroteSize == m_WroteSize
					&& RecordTime == m_RecordTime
					&& DiskFreeSpace == m_DiskFreeSpace)))
		return false;

	m_fRecording = fRecording;
	if (fRecording) {
		m_WroteSize = WroteSize;
		m_RecordTime = RecordTime;
		m_DiskFreeSpace = DiskFreeSpace;
	}

	return true;
}


void CInformationPanel::CRecordItem::Draw(HDC hdc, const RECT &Rect)
{
	if (m_fRecording) {
		TCHAR szText[256];

		const unsigned int RecordSec = static_cast<unsigned int>(m_RecordTime / 1000);
		size_t Length = StringFormat(
			szText,
			TEXT("● {}:{:02}:{:02}"),
			RecordSec / (60 * 60), (RecordSec / 60) % 60, RecordSec % 60);
		if (m_WroteSize >= 0) {
			const unsigned int Size =
				static_cast<unsigned int>(m_WroteSize / static_cast<ULONGLONG>(1024 * 1024 / 100));
			Length += StringFormat(
				szText + Length, lengthof(szText) - Length,
				TEXT(" / {}.{:02} MB"),
				Size / 100, Size % 100);
		}
		if (m_DiskFreeSpace >= 0) {
			const unsigned int FreeSpace =
				static_cast<unsigned int>(m_DiskFreeSpace / static_cast<ULONGLONG>(1024 * 1024 * 1024 / 100));
			StringFormat(
				szText + Length, lengthof(szText) - Length,
				TEXT(" / {}.{:02} GB空き"),
				FreeSpace / 100, FreeSpace % 100);
		}
		DrawItem(hdc, Rect, szText);
	} else {
		DrawItem(hdc, Rect, TEXT("■ <録画していません>"));
	}
}




CInformationPanel::CServiceItem::CServiceItem(CInformationPanel *pPanel, bool fVisible)
	: CItemTemplate(pPanel, fVisible)
{
}


void CInformationPanel::CServiceItem::Reset()
{
	m_ServiceName.clear();
}


bool CInformationPanel::CServiceItem::Update()
{
	const CCoreEngine &CoreEngine = GetAppClass().CoreEngine;
	const LibISDB::AnalyzerFilter *pAnalyzer = CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
	String ServiceName;

	if (pAnalyzer != nullptr
			&& pAnalyzer->GetServiceName(CoreEngine.GetServiceIndex(), &ServiceName)) {
		if (m_ServiceName == ServiceName)
			return false;
		m_ServiceName = ServiceName;
	} else {
		if (m_ServiceName.empty())
			return false;
		m_ServiceName.clear();
	}

	return true;
}


void CInformationPanel::CServiceItem::Draw(HDC hdc, const RECT &Rect)
{
	DrawItem(hdc, Rect, m_ServiceName.c_str());
}




CInformationPanel::CProgramInfoItem::CProgramInfoItem(CInformationPanel *pPanel, bool fVisible)
	: CItemTemplate(pPanel, fVisible)
{
	Reset();
}


void CInformationPanel::CProgramInfoItem::Reset()
{
	m_fNext = false;
	m_InfoText.clear();
}


bool CInformationPanel::CProgramInfoItem::Update()
{
	CCoreEngine &CoreEngine = GetAppClass().CoreEngine;
	TCHAR szText[4096], szTemp[256];
	CStaticStringFormatter Formatter(szText, lengthof(szText));

	if (m_fNext)
		Formatter.Append(TEXT("次 : "));

	LibISDB::EventInfo EventInfo;

	if (CoreEngine.GetCurrentEventInfo(&EventInfo, m_fNext)) {
		if (EpgUtil::FormatEventTime(
					EventInfo, szTemp, lengthof(szTemp),
					EpgUtil::FormatEventTimeFlag::Date |
					EpgUtil::FormatEventTimeFlag::Year |
					EpgUtil::FormatEventTimeFlag::UndecidedText) > 0) {
			Formatter.Append(szTemp);
			Formatter.Append(TEXT("\r\n"));
		}
		if (!EventInfo.EventName.empty()) {
			Formatter.Append(EventInfo.EventName.c_str());
			Formatter.Append(TEXT("\r\n\r\n"));
		}
		if (!EventInfo.EventText.empty()) {
			Formatter.Append(EventInfo.EventText.c_str());
			Formatter.Append(TEXT("\r\n\r\n"));
		}
		if (!EventInfo.ExtendedText.empty()) {
			LibISDB::String ExtendedText;
			EventInfo.GetConcatenatedExtendedText(&ExtendedText);
			Formatter.Append(ExtendedText.c_str());
		}
	}

	const LibISDB::AnalyzerFilter *pAnalyzer = CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
	LibISDB::AnalyzerFilter::EventSeriesInfo SeriesInfo;
	if (pAnalyzer->GetEventSeriesInfo(CoreEngine.GetServiceIndex(), &SeriesInfo, m_fNext)
			&& SeriesInfo.EpisodeNumber != 0 && SeriesInfo.LastEpisodeNumber != 0) {
		Formatter.Append(TEXT("\r\n\r\n(シリーズ"));
		if (SeriesInfo.RepeatLabel != 0)
			Formatter.Append(TEXT(" [再]"));
		if (SeriesInfo.EpisodeNumber != 0 && SeriesInfo.LastEpisodeNumber != 0)
			Formatter.AppendFormat(
				TEXT(" 第{}回 / 全{}回"),
				SeriesInfo.EpisodeNumber, SeriesInfo.LastEpisodeNumber);
		// expire_date は実際の最終回の日時でないので、紛らわしいため表示しない
		/*
		if (SeriesInfo.ExpireDate.IsValid()) {
			Formatter.AppendFormat(
				TEXT(" 終了予定{}/{}/{}"),
				SeriesInfo.ExpireDate.Year,
				SeriesInfo.ExpireDate.Month,
				SeriesInfo.ExpireDate.Day);
		}
		*/
		Formatter.Append(TEXT(")"));
	}

	if (m_InfoText.compare(Formatter.GetString()) == 0)
		return false;

	m_InfoText = Formatter.GetString();

	m_pPanel->UpdateProgramInfoText();

	return true;
}


void CInformationPanel::CProgramInfoItem::Draw(HDC hdc, const RECT &Rect)
{
	DrawItem(hdc, Rect, nullptr);
}


void CInformationPanel::CProgramInfoItem::DrawButton(
	HDC hdc, Theme::CThemeDraw &ThemeDraw,
	const Theme::ForegroundStyle Style,
	const RECT &ButtonRect, const RECT &TextRect,
	int Button)
{
	const HFONT hfontOld = DrawUtil::SelectObject(hdc, m_pPanel->m_IconFont);
	ThemeDraw.Draw(
		Style, ButtonRect,
		Button == BUTTON_NEXT ? TEXT("4") : TEXT("3"),
		DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	::SelectObject(hdc, hfontOld);
}


bool CInformationPanel::CProgramInfoItem::GetButtonRect(int Button, RECT *pRect) const
{
	if (Button == BUTTON_PREV || Button == BUTTON_NEXT) {
		RECT rc;

		m_pPanel->GetItemRect(GetID(), &rc);
		pRect->left = rc.right - m_pPanel->m_Style.ButtonSize.Width * (2 - Button);
		pRect->right = pRect->left + m_pPanel->m_Style.ButtonSize.Width;
		pRect->top = rc.bottom;
		pRect->bottom = rc.bottom + m_pPanel->m_Style.ButtonSize.Height;

		return true;
	}

	return false;
}


bool CInformationPanel::CProgramInfoItem::IsButtonEnabled(int Button) const
{
	switch (Button) {
	case BUTTON_PREV:
		return m_fNext;

	case BUTTON_NEXT:
		return !m_fNext;
	}

	return false;
}


bool CInformationPanel::CProgramInfoItem::OnButtonPushed(int Button)
{
	switch (Button) {
	case BUTTON_PREV:
	case BUTTON_NEXT:
		SetNext(Button == BUTTON_NEXT);
		m_pPanel->RedrawButton(GetID(), BUTTON_PREV);
		m_pPanel->RedrawButton(GetID(), BUTTON_NEXT);
		return true;
	}

	return false;
}


bool CInformationPanel::CProgramInfoItem::GetButtonTipText(int Button, LPTSTR pszText, int MaxText) const
{
	switch (Button) {
	case BUTTON_PREV:
		StringCopy(pszText, TEXT("現在の番組"), MaxText);
		return true;

	case BUTTON_NEXT:
		StringCopy(pszText, TEXT("次の番組"), MaxText);
		return true;
	}

	return false;
}


void CInformationPanel::CProgramInfoItem::SetNext(bool fNext)
{
	if (m_fNext != fNext) {
		m_fNext = fNext;
		Update();
	}
}


}	// namespace TVTest
