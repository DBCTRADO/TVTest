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
#include "Dialog.h"
#include "DialogUtil.h"
#include "DPIUtil.h"
#include "DarkMode.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace
{

namespace DarkColorScheme
{

	constexpr COLORREF Text         = RGB(255, 255, 255);
	constexpr COLORREF Back         = RGB( 56,  56,  56);
	constexpr COLORREF ContentBack  = RGB( 32,  32,  32);
	constexpr COLORREF SelectedBack = RGB( 98,  98,  98);
	constexpr COLORREF MenuBack     = RGB( 43,  43,  43);
	constexpr COLORREF Edge         = RGB(102, 102, 102);
	constexpr COLORREF Border       = RGB(155, 155, 155);
	constexpr COLORREF HotBorder    = RGB(200, 200, 200);

} // namespace DarkColorScheme


struct UpDownInfo
{
	enum class ItemType {
		Invalid,
		Down,
		Up
	};

	static const LPCTSTR PROP_NAME;

	RECT DownRect;
	RECT UpRect;
	RECT CenterRect;
	ItemType HotItem = ItemType::Invalid;
	DrawUtil::CFont Font;

	void UpdateSize(HWND hwnd)
	{
		const DWORD Style = GetWindowStyle(hwnd);
		const bool fHorz = (Style & UDS_HORZ) != 0;

		RECT rc;
		::GetClientRect(hwnd, &rc);

		CenterRect = RECT{};

		if (fHorz) {
			const int ButtonWidth = rc.right / 2;
			DownRect = rc;
			DownRect.right = DownRect.left + ButtonWidth;
			UpRect = DownRect;
			UpRect.left = UpRect.right - ButtonWidth;
			if ((rc.right % 2) != 0)
				CenterRect = RECT{DownRect.right, rc.top, UpRect.left, rc.bottom};
		} else {
			const int ButtonHeight = rc.bottom / 2;
			UpRect = rc;
			UpRect.bottom = UpRect.top + ButtonHeight;
			DownRect = rc;
			DownRect.top = DownRect.bottom - ButtonHeight;
			if ((rc.bottom % 2) != 0)
				CenterRect = RECT{rc.left, UpRect.bottom, rc.right, DownRect.top};
		}
	}

	ItemType HitTest(int x, int y) const
	{
		const POINT pt = {x, y};
		if (::PtInRect(&DownRect, pt))
			return ItemType::Down;
		if (::PtInRect(&UpRect, pt))
			return ItemType::Up;
		return ItemType::Invalid;
	}

	static UpDownInfo * FromWindow(HWND hwnd)
	{
		return static_cast<UpDownInfo *>(::GetProp(hwnd, UpDownInfo::PROP_NAME));
	}
};

const LPCTSTR UpDownInfo::PROP_NAME = TEXT("TVTestDarkModeUpDown");

} // namespace



const LPCTSTR CBasicDialog::PROP_NAME = TEXT("TVTestDialog");


CBasicDialog::CBasicDialog()
{
	SetStyleScaling(&m_StyleScaling);
}


CBasicDialog::~CBasicDialog()
{
	Destroy();
}


bool CBasicDialog::IsCreated() const
{
	return m_hDlg != nullptr;
}


bool CBasicDialog::Destroy()
{
	if (m_hDlg != nullptr) {
		return ::DestroyWindow(m_hDlg) != FALSE;
	}
	return true;
}


bool CBasicDialog::ProcessMessage(LPMSG pMsg)
{
	if (m_hDlg == nullptr)
		return false;
	return ::IsDialogMessage(m_hDlg, pMsg) != FALSE;
}


bool CBasicDialog::IsVisible() const
{
	return m_hDlg != nullptr && ::IsWindowVisible(m_hDlg);
}


bool CBasicDialog::SetVisible(bool fVisible)
{
	if (m_hDlg == nullptr)
		return false;
	return ::ShowWindow(m_hDlg, fVisible ? SW_SHOW : SW_HIDE) != FALSE;
}


bool CBasicDialog::GetPosition(Position *pPosition) const
{
	if (pPosition == nullptr)
		return false;
	if (m_hDlg == nullptr) {
		*pPosition = m_Position;
	} else {
		RECT rc;
		::GetWindowRect(m_hDlg, &rc);
		pPosition->Set(&rc);
	}
	return true;
}


bool CBasicDialog::GetPosition(RECT *pPosition) const
{
	if (pPosition == nullptr)
		return false;
	if (m_hDlg == nullptr)
		m_Position.Get(pPosition);
	else
		::GetWindowRect(m_hDlg, pPosition);
	return true;
}


bool CBasicDialog::GetPosition(int *pLeft, int *pTop, int *pWidth, int *pHeight) const
{
	RECT rc;

	GetPosition(&rc);
	if (pLeft != nullptr)
		*pLeft = rc.left;
	if (pTop != nullptr)
		*pTop = rc.top;
	if (pWidth != nullptr)
		*pWidth = rc.right - rc.left;
	if (pHeight != nullptr)
		*pHeight = rc.bottom - rc.top;
	return true;
}


bool CBasicDialog::SetPosition(const Position &Pos)
{
	return SetPosition(Pos.x, Pos.y, Pos.Width, Pos.Height);
}


bool CBasicDialog::SetPosition(const RECT *pPosition)
{
	if (pPosition == nullptr)
		return false;
	return SetPosition(
		pPosition->left, pPosition->top,
		pPosition->right - pPosition->left,
		pPosition->bottom - pPosition->top);
}


bool CBasicDialog::SetPosition(int Left, int Top, int Width, int Height)
{
	if (Width < 0 || Height < 0)
		return false;
	if (m_hDlg == nullptr) {
		m_Position.x = Left;
		m_Position.y = Top;
		m_Position.Width = Width;
		m_Position.Height = Height;
		m_fSetPosition = true;
	} else {
		::MoveWindow(m_hDlg, Left, Top, Width, Height, TRUE);
	}
	return true;
}


bool CBasicDialog::SetPosition(int Left, int Top)
{
	if (m_hDlg == nullptr) {
		m_Position.x = Left;
		m_Position.y = Top;
		m_fSetPosition = true;
	} else {
		::SetWindowPos(m_hDlg, nullptr, Left, Top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
	return true;
}


LRESULT CBasicDialog::SendMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_hDlg == nullptr)
		return 0;
	return ::SendMessage(m_hDlg, uMsg, wParam, lParam);
}


int CBasicDialog::ShowDialog(HWND hwndOwner, HINSTANCE hinst, LPCTSTR pszTemplate)
{
	if (m_hDlg != nullptr)
		return -1;

	return static_cast<int>(::DialogBoxParam(
		hinst, pszTemplate, hwndOwner, DialogProc,
		reinterpret_cast<LPARAM>(this)));
}


bool CBasicDialog::CreateDialogWindow(HWND hwndOwner, HINSTANCE hinst, LPCTSTR pszTemplate)
{
	if (m_hDlg != nullptr)
		return false;

	if (::CreateDialogParam(
				hinst, pszTemplate, hwndOwner, DialogProc,
				reinterpret_cast<LPARAM>(this)) == nullptr)
		return false;
	m_fModeless = true;
	return true;
}


CBasicDialog *CBasicDialog::GetThis(HWND hDlg)
{
	return static_cast<CBasicDialog*>(::GetProp(hDlg, PROP_NAME));
}


INT_PTR CALLBACK CBasicDialog::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CBasicDialog *pThis;

	if (uMsg == WM_INITDIALOG) {
		pThis = reinterpret_cast<CBasicDialog*>(lParam);
		pThis->m_hDlg = hDlg;
		pThis->m_fOwnDPIScaling = IsWindowPerMonitorDPIV1(hDlg);
		::SetProp(hDlg, PROP_NAME, pThis);
	} else {
		pThis = GetThis(hDlg);
		if (uMsg == WM_NCDESTROY) {
			if (pThis != nullptr) {
				pThis->HandleDarkModeMessage(hDlg, uMsg, wParam, lParam);
				pThis->m_hDlg = nullptr;
			}
			::RemoveProp(hDlg, PROP_NAME);
			pThis->OnDestroyed();
			// ここで既に pThis が delete されている可能性がある
			return TRUE;
		}
	}
	if (pThis != nullptr)
		return pThis->HandleDarkModeMessage(hDlg, uMsg, wParam, lParam);
	return FALSE;
}


INT_PTR CBasicDialog::HandleMessage(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		m_fInitializing = true;
		if (m_fSetPosition) {
			m_Position.Width = 0;
			m_Position.Height = 0;
			ApplyPosition();
		}
		InitDialog();
		break;

	case WM_DPICHANGED:
		if (!m_fInitializing)
			OnDPIChanged(hDlg, wParam, lParam);
		break;

	case WM_DESTROY:
		StorePosition();

		if (m_fOwnDPIScaling) {
			if (!m_ItemList.empty()) {
				for (const auto &e : m_ItemList)
					SetWindowFont(e.hwnd, m_hOriginalFont, FALSE);
				m_ItemList.clear();
			}
			if (m_hOriginalFont != nullptr)
				SetWindowFont(hDlg, m_hOriginalFont, FALSE);
		}
		break;
	}

	return DlgProc(hDlg, uMsg, wParam, lParam);
}


INT_PTR CBasicDialog::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		return TRUE;
	}

	return FALSE;
}


bool CBasicDialog::ApplyPosition()
{
	if (m_hDlg == nullptr || !m_fSetPosition)
		return false;

	RECT rc;
	m_Position.Get(&rc);
	const HMONITOR hMonitor = ::MonitorFromRect(&rc, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	if (::GetMonitorInfo(hMonitor, &mi)) {
		if (rc.left < mi.rcMonitor.left)
			m_Position.x = mi.rcMonitor.left;
		else if (rc.right > mi.rcMonitor.right)
			m_Position.x = mi.rcMonitor.right - m_Position.Width;
		if (rc.top < mi.rcMonitor.top)
			m_Position.y = mi.rcMonitor.top;
		else if (rc.bottom > mi.rcMonitor.bottom)
			m_Position.y = mi.rcMonitor.bottom - m_Position.Height;
	}

	UINT Flags = SWP_NOZORDER | SWP_NOACTIVATE;
	if (m_Position.Width <= 0 || m_Position.Height <= 0)
		Flags |= SWP_NOSIZE;

	return ::SetWindowPos(
		m_hDlg, nullptr,
		m_Position.x, m_Position.y,
		m_Position.Width, m_Position.Height,
		Flags) != FALSE;
}


void CBasicDialog::StorePosition()
{
	if (m_hDlg != nullptr) {
		RECT rc;

		if (::GetWindowRect(m_hDlg, &rc)) {
			m_Position.Set(&rc);
			m_fSetPosition = true;
		}
	}
}


void CBasicDialog::InitDialog()
{
	m_fInitializing = true;

	if (m_pStyleScaling == &m_StyleScaling) {
		InitStyleScaling(m_hDlg, false);
	}

	if (IsWindowPerMonitorDPIV2(m_hDlg))
		m_OriginalDPI = GetWindowDPI(m_hDlg);
	else
		m_OriginalDPI = m_pStyleScaling->GetSystemDPI();
	m_CurrentDPI = m_OriginalDPI;
	m_hOriginalFont = GetWindowFont(m_hDlg);
	::GetObject(m_hOriginalFont, sizeof(LOGFONT), &m_lfOriginalFont);

	InitializeUI();

	if (m_fOwnDPIScaling && m_pStyleScaling->GetDPI() != m_OriginalDPI) {
		RealizeStyle();

		RECT rc;
		::GetClientRect(m_hDlg, &rc);
		rc.right = ::MulDiv(rc.right, m_CurrentDPI, m_OriginalDPI);
		rc.bottom = ::MulDiv(rc.bottom, m_CurrentDPI, m_OriginalDPI);
		::AdjustWindowRectEx(&rc, GetWindowStyle(m_hDlg), FALSE, GetWindowExStyle(m_hDlg));
		::SetWindowPos(
			m_hDlg, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
			SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
	}

	m_fInitializing = false;
}


void CBasicDialog::ApplyStyle()
{
	if (m_hDlg != nullptr) {
		const int DPI = m_pStyleScaling->GetDPI();

		LOGFONT lf = m_lfOriginalFont;
		const LONG Height = ::MulDiv(std::abs(lf.lfHeight), DPI, m_OriginalDPI);
		lf.lfHeight = lf.lfHeight < 0 ? -Height : Height;
		lf.lfWidth = 0;
		m_Font.Create(&lf);
	}
}


void CBasicDialog::RealizeStyle()
{
	if (m_hDlg != nullptr && m_fOwnDPIScaling) {
		const int DPI = m_pStyleScaling->GetDPI();

		if (m_CurrentDPI != DPI) {
			m_CurrentDPI = DPI;

			if (m_ItemList.empty()) {
				HWND hwnd = nullptr;

				while ((hwnd = ::FindWindowEx(m_hDlg, hwnd, nullptr, nullptr)) != nullptr) {
					ItemInfo Item;

					Item.hwnd = hwnd;
					::GetWindowRect(hwnd, &Item.rcOriginal);
					MapWindowRect(nullptr, m_hDlg, &Item.rcOriginal);
					m_ItemList.push_back(Item);
				}
			}

			HDWP hdwp = ::BeginDeferWindowPos(static_cast<int>(m_ItemList.size()));

			for (const auto &e : m_ItemList) {
				HWND hwnd = e.hwnd;
				RECT rc = e.rcOriginal;

				rc.left = ::MulDiv(rc.left, DPI, m_OriginalDPI);
				rc.top = ::MulDiv(rc.top, DPI, m_OriginalDPI);
				rc.right = ::MulDiv(rc.right, DPI, m_OriginalDPI);
				rc.bottom = ::MulDiv(rc.bottom, DPI, m_OriginalDPI);
				if (hdwp != nullptr) {
					hdwp = ::DeferWindowPos(
						hdwp, hwnd, nullptr,
						rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
						SWP_NOZORDER | SWP_NOACTIVATE);
				} else {
					::SetWindowPos(
						hwnd, nullptr,
						rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
						SWP_NOZORDER | SWP_NOACTIVATE);
				}

				TCHAR szClass[32];
				::GetClassName(hwnd, szClass, lengthof(szClass));
				if (::lstrcmpi(szClass, TEXT("BUTTON")) == 0
						|| ::lstrcmpi(szClass, WC_LISTVIEW) == 0
						|| ::lstrcmpi(szClass, WC_TREEVIEW) == 0)
					::SendMessage(hwnd, CCM_DPISCALE, TRUE, 0);

				SetWindowFont(hwnd, m_Font.GetHandle(), FALSE);
				::InvalidateRect(hwnd, nullptr, TRUE);
			}

			if (hdwp != nullptr)
				::EndDeferWindowPos(hdwp);

			SetWindowFont(m_hDlg, m_Font.GetHandle(), FALSE);
			::InvalidateRect(m_hDlg, nullptr, TRUE);
		}
	}
}


void CBasicDialog::HandleDarkModeChanged(bool fDarkMode)
{
	UpdateColors(fDarkMode);

	if (m_hDlg != nullptr)
		UpdateControlsTheme(m_hDlg, fDarkMode);

	OnDarkModeChanged(fDarkMode);

	if (m_hDlg != nullptr)
		::RedrawWindow(m_hDlg, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
}


INT_PTR CBasicDialog::HandleDarkModeMessage(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			m_fAllowDarkMode = false;
			m_fDarkMode = false;
			if (!m_fDisableDarkMode && GetStyleManager()->IsDarkDialog()) {
				if ((GetWindowStyle(hDlg) & WS_CHILD) == 0) {
					if (SetWindowAllowDarkMode(hDlg, true)) {
						m_fAllowDarkMode = true;
						if (TVTest::IsDarkMode()) {
							if (SetWindowFrameDarkMode(hDlg, true))
								m_fDarkMode = true;
						}
					}
				} else {
					const CBasicDialog *pParentDialog = GetThis(::GetParent(hDlg));
					if (pParentDialog != nullptr) {
						m_fDarkMode = pParentDialog->m_fDarkMode;
					}
				}
			}

			UpdateColors(m_fDarkMode);

			const INT_PTR Result = HandleMessage(hDlg, uMsg, wParam, lParam);

			InitializeControls(hDlg, m_fDarkMode);
			return Result;
		}

	case WM_SYSCOLORCHANGE:
		UpdateColors(m_fDarkMode);
		UpdateControlsColors(hDlg, m_fDarkMode);
		break;

	case WM_ERASEBKGND:
		if (m_fDarkMode) {
			const INT_PTR Result = HandleMessage(hDlg, uMsg, wParam, lParam);
			if (Result != 0)
				return Result;

			if (m_FaceBrush.IsCreated()) {
				const HDC hdc = reinterpret_cast<HDC>(wParam);
				RECT rc;
				::GetClientRect(hDlg, &rc);
				::FillRect(hdc, &rc, m_FaceBrush.GetHandle());
				return TRUE;
			}
			return FALSE;
		}
		break;

	case WM_PRINTCLIENT:
		if (m_fDarkMode && m_FaceBrush.IsCreated()) {
			const HDC hdc = reinterpret_cast<HDC>(wParam);
			RECT rc;
			::GetClientRect(hDlg, &rc);
			::FillRect(hdc, &rc, m_FaceBrush.GetHandle());
			return TRUE;
		}
		break;

	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLORBTN:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
		if (m_fDarkMode) {
			return HandleDarkModeCtlColor(hDlg, uMsg, wParam, lParam);
		}
		break;

	case WM_SETTINGCHANGE:
		if (m_fAllowDarkMode) {
			if (IsDarkModeSettingChanged(hDlg, uMsg, wParam, lParam)) {
				const bool fDarkMode = TVTest::IsDarkMode();

				if (m_fDarkMode != fDarkMode) {
					if (SetWindowFrameDarkMode(hDlg, fDarkMode)) {
						m_fDarkMode = fDarkMode;
						HandleDarkModeChanged(fDarkMode);
					}
				}
			}
		}
		break;

	case WM_THEMECHANGED:
		m_ControlThemeMap.clear();
		break;

	case WM_NOTIFY:
		{
			const INT_PTR Result = HandleMessage(hDlg, uMsg, wParam, lParam);
			if (Result != 0)
				return Result;
		}
		return HandleDarkModeNotifyMessage(reinterpret_cast<NMHDR *>(lParam));

	case WM_DESTROY:
		m_BackBrush.Destroy();
		m_FaceBrush.Destroy();
		break;
	}

	return HandleMessage(hDlg, uMsg, wParam, lParam);
}


LRESULT CBasicDialog::HandleDarkModeCtlColor(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const INT_PTR Result = HandleMessage(hDlg, uMsg, wParam, lParam);
	if (Result != 0)
		return Result;

	const HWND hwnd = reinterpret_cast<HWND>(lParam);

	if (IsDisableThemingControl(hwnd))
		return FALSE;

	bool fFaceColor = false;

	switch (uMsg) {
	case WM_CTLCOLORBTN:
	case WM_CTLCOLORSTATIC:
		fFaceColor = true;
		break;

	case WM_CTLCOLOREDIT:
		{
			// コンボボックスが CBS_DROPDOWNLIST の場合の現在の項目の外枠の色
			TCHAR szClassName[64];
			if (::GetClassName(hwnd, szClassName, lengthof(szClassName)) == 8
					&& ::lstrcmpi(szClassName, TEXT("COMBOBOX")) == 0) {
				fFaceColor = true;
			}
		}
		break;

	case WM_CTLCOLORLISTBOX:
		{
			// コンボボックスのドロップダウンリスト
			TCHAR szClassName[64];
			if (::GetClassName(hwnd, szClassName, lengthof(szClassName)) == 9
					&& ::lstrcmpi(szClassName, TEXT("ComboLBox")) == 0) {
				fFaceColor = true;
			}
		}
		break;
	}

	const DrawUtil::CBrush &Brush = fFaceColor ? m_FaceBrush : m_BackBrush;
	if (!Brush.IsCreated())
		return FALSE;

	const HDC hdc = reinterpret_cast<HDC>(wParam);

	::SetTextColor(hdc, m_TextColor);
	::SetBkColor(hdc, fFaceColor ? m_FaceColor : m_BackColor);

	return reinterpret_cast<LRESULT>(Brush.GetHandle());
}


bool CBasicDialog::HandleDarkModeNotifyMessage(NMHDR *pnmh)
{
	switch (pnmh->code) {
	case NM_CUSTOMDRAW:
		if (m_fDarkMode && !IsDisableThemingControl(pnmh->hwndFrom)) {
			TCHAR szClassName[64];
			if (::GetClassName(pnmh->hwndFrom, szClassName, lengthof(szClassName)) > 0) {
				LRESULT Result;

				if (::lstrcmpi(szClassName, TEXT("BUTTON")) == 0) {
					Result = CustomDrawDarkModeButton(reinterpret_cast<NMCUSTOMDRAW *>(pnmh));
				} else if (::lstrcmpi(szClassName, WC_LISTVIEW) == 0) {
					Result = CustomDrawDarkModeListView(reinterpret_cast<NMLVCUSTOMDRAW *>(pnmh));
				} else if (::lstrcmpi(szClassName, WC_TREEVIEW) == 0) {
					Result = CustomDrawDarkModeTreeView(reinterpret_cast<NMTVCUSTOMDRAW *>(pnmh));
				} else if (::lstrcmpi(szClassName, TRACKBAR_CLASS) == 0) {
					Result = CustomDrawDarkModeTrackBar(reinterpret_cast<NMCUSTOMDRAW *>(pnmh));
				} else {
					break;
				}

				::SetWindowLongPtr(m_hDlg, DWLP_MSGRESULT, Result);
				return true;
			}
		}
		break;
	}

	return false;
}


LRESULT CBasicDialog::CustomDrawDarkModeButton(NMCUSTOMDRAW *pnmcd)
{
	const HWND hwnd = pnmcd->hdr.hwndFrom;
	const DWORD Style = GetWindowStyle(hwnd);
	const DWORD Type = Style & BS_TYPEMASK;
	LRESULT Result = CDRF_DODEFAULT;

	if ((Type == BS_CHECKBOX || Type == BS_AUTOCHECKBOX
				|| Type == BS_3STATE || Type == BS_AUTO3STATE
				|| Type == BS_RADIOBUTTON || Type == BS_AUTORADIOBUTTON)
			&& (Style & BS_PUSHLIKE) == 0) {
		const HDC hdc = pnmcd->hdc;

		switch (pnmcd->dwDrawStage) {
		case CDDS_PREPAINT:
			if (::BufferedPaintRenderAnimation(hwnd, hdc)) {
				Result = CDRF_SKIPDEFAULT;
				break;
			}

			{
				ControlThemeInfo *pThemeInfo = GetThemeInfo(hwnd, VSCLASS_BUTTON);
				if (pThemeInfo == nullptr)
					break;

				const bool fChecked = Button_GetCheck(hwnd) == BST_CHECKED;
				const bool fHot = (pnmcd->uItemState & CDIS_HOT) != 0;
				const bool fPressed = (pnmcd->uItemState & CDIS_SELECTED) != 0;
				const bool fDisabled = (pnmcd->uItemState & CDIS_DISABLED) != 0;
				const bool fIndeterminate = (pnmcd->uItemState & CDIS_INDETERMINATE) != 0;
				HANIMATIONBUFFER hAnimation = nullptr;
				HDC hdcFrom = nullptr, hdcTo = nullptr;
				int Part, State;

				switch (Type) {
				case BS_CHECKBOX:
				case BS_AUTOCHECKBOX:
				case BS_3STATE:
				case BS_AUTO3STATE:
					Part = BP_CHECKBOX;
					if (fChecked) {
						if (fPressed) {
							State = CBS_CHECKEDPRESSED;
						} else if (fDisabled) {
							State = CBS_CHECKEDDISABLED;
						} else if (fHot) {
							State = CBS_CHECKEDHOT;
						} else {
							State = CBS_CHECKEDNORMAL;
						}
					} else if (fIndeterminate) {
						if (fPressed) {
							State = CBS_MIXEDPRESSED;
						} else if (fDisabled) {
							State = CBS_MIXEDDISABLED;
						} else if (fHot) {
							State = CBS_MIXEDHOT;
						} else {
							State = CBS_MIXEDNORMAL;
						}
					} else {
						if (fPressed) {
							State = CBS_UNCHECKEDPRESSED;
						} else if (fDisabled) {
							State = CBS_UNCHECKEDDISABLED;
						} else if (fHot) {
							State = CBS_UNCHECKEDHOT;
						} else {
							State = CBS_UNCHECKEDNORMAL;
						}
					}
					break;

				case BS_RADIOBUTTON:
				case BS_AUTORADIOBUTTON:
					Part = BP_RADIOBUTTON;
					if (fChecked) {
						if (fPressed) {
							State = RBS_CHECKEDPRESSED;
						} else if (fDisabled) {
							State = RBS_CHECKEDDISABLED;
						} else if (fHot) {
							State = RBS_CHECKEDHOT;
						} else {
							State = RBS_CHECKEDNORMAL;
						}
					} else {
						if (fPressed) {
							State = RBS_UNCHECKEDPRESSED;
						} else if (fDisabled) {
							State = RBS_UNCHECKEDDISABLED;
						} else if (fHot) {
							State = RBS_UNCHECKEDHOT;
						} else {
							State = RBS_UNCHECKEDNORMAL;
						}
					}
					break;
				}

				SIZE CheckSize;
				pThemeInfo->Theme.GetPartSize(hdc, Part, State, &CheckSize);
				RECT rc = pnmcd->rc;
				rc.top = rc.top + ((rc.bottom - rc.top) - CheckSize.cy) / 2;
				rc.right = rc.left + CheckSize.cx;
				rc.bottom = rc.top + CheckSize.cy;

				if (pThemeInfo->State >= 0 && pThemeInfo->State != State) {
					BP_ANIMATIONPARAMS AnimationParams = {sizeof(BP_ANIMATIONPARAMS)};
					if (pThemeInfo->Theme.GetTransitionDuration(
							Part, pThemeInfo->State, State, &AnimationParams.dwDuration)) {
						AnimationParams.style = BPAS_LINEAR;
						hAnimation = ::BeginBufferedAnimation(
							hwnd, hdc, &pnmcd->rc, BPBF_COMPATIBLEBITMAP, nullptr, &AnimationParams,
							&hdcFrom, &hdcTo);
						if (hAnimation != nullptr) {
							if (hdcFrom != nullptr) {
								DrawUtil::Fill(hdcFrom, &pnmcd->rc, m_FaceColor);
								pThemeInfo->Theme.DrawBackground(hdcFrom, Part, pThemeInfo->State, &rc);
							}
							if (hdcTo != nullptr) {
								DrawUtil::Fill(hdcTo, &pnmcd->rc, m_FaceColor);
								pThemeInfo->Theme.DrawBackground(hdcTo, Part, State, &rc);
							}
						}
					}
				}

				pThemeInfo->State = State;

				if (hAnimation == nullptr) {
					DrawUtil::Fill(hdc, &pnmcd->rc, m_FaceColor);
					pThemeInfo->Theme.DrawBackground(hdc, Part, State, &rc);
				}

				TCHAR szText[256];
				const int TextLength = ::GetWindowText(hwnd, szText, lengthof(szText));

				SIZE Size;
				::GetTextExtentPoint32(hdc, TEXT("0"), 1, &Size);
				const int CheckMargin = Size.cx / 2;

				rc = pnmcd->rc;
				rc.left += CheckSize.cx + CheckMargin;

				const LRESULT UIState = ::SendMessage(hwnd, WM_QUERYUISTATE, 0, 0);
				UINT Format = DT_LEFT;
				if ((Style & BS_MULTILINE) != 0)
					Format |= DT_WORDBREAK;
				else
					Format |= DT_SINGLELINE | DT_VCENTER;
				if ((UIState & UISF_HIDEACCEL) != 0)
					Format |= DT_HIDEPREFIX;

				bool fNoText = false;
				const HBITMAP hbm = reinterpret_cast<HBITMAP>(::SendMessage(hwnd, BM_GETIMAGE, IMAGE_BITMAP, 0));
				BITMAP bm;
				if (hbm != nullptr) {
					if (::GetObject(hbm, sizeof(BITMAP), &bm) == sizeof(BITMAP)) {
						if ((Style & BS_BITMAP) != 0)
							fNoText = true;
					}
				}

				for (int i = 0; i < (hAnimation != nullptr ? 2 : 1); i++) {
					HDC hdcDraw;
					HGDIOBJ hOldFont;
					bool fDisabledText = fDisabled;
					if (hAnimation != nullptr) {
						if (i == 0) {
							hdcDraw = hdcFrom;
							fDisabledText = pThemeInfo->fDisabled;
						} else {
							hdcDraw = hdcTo;
						}
						if (hdcDraw == nullptr)
							continue;
						hOldFont = ::SelectObject(hdcDraw, ::GetCurrentObject(hdc, OBJ_FONT));
					} else {
						hdcDraw = hdc;
					}

					if (hbm != nullptr) {
						DrawUtil::DrawBitmap(
							hdcDraw, rc.left, rc.top + ((rc.bottom - rc.top) - bm.bmHeight) / 2,
							bm.bmWidth, bm.bmHeight, hbm);
						rc.left += bm.bmWidth + CheckMargin;
					}

					if (!fNoText && TextLength > 0) {
						const COLORREF OldTextColor = ::SetTextColor(hdcDraw, fDisabledText ? GetThemeColor(COLOR_GRAYTEXT) : m_TextColor);
						const COLORREF OldBkColor = ::SetBkColor(hdcDraw, m_FaceColor);
						::DrawText(hdcDraw, szText, TextLength, &rc, Format);
						::SetTextColor(hdcDraw, OldTextColor);
						::SetBkColor(hdcDraw, OldBkColor);
					}

					if ((pnmcd->uItemState & CDIS_FOCUS) != 0 && (UIState & UISF_HIDEFOCUS) == 0) {
						if (!fNoText && TextLength > 0) {
							RECT rcText = {0, 0, rc.right - rc.left, rc.bottom - rc.top};
							::DrawText(hdcDraw, szText, TextLength, &rcText, Format | DT_CALCRECT);
							rc.left += rcText.right;
						}
						const RECT rcFocus = {rc.left - 1, rc.top, std::min(rc.left + 1, rc.right), rc.bottom};
						::DrawFocusRect(hdcDraw, &rcFocus);
					}

					if (hdcDraw != hdc)
						::SelectObject(hdcDraw, hOldFont);
				}

				if (hAnimation != nullptr) {
					::EndBufferedAnimation(hAnimation, TRUE);
					pThemeInfo->fDisabled = fDisabled;
				}

				Result = CDRF_SKIPDEFAULT;
			}
			break;
		}
	}

	return Result;
}


LRESULT CBasicDialog::CustomDrawDarkModeListView(NMLVCUSTOMDRAW *pnmcd)
{
	LRESULT Result = CDRF_DODEFAULT;

	switch (pnmcd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		if (pnmcd->dwItemType == LVCDI_GROUP) {
			// グループヘッダの文字色は変更できないため、文字が読みづらくならないように背景を塗りつぶす
			RECT rcHeader;
			ListView_GetGroupRect(pnmcd->nmcd.hdr.hwndFrom, static_cast<int>(pnmcd->nmcd.dwItemSpec), LVGGR_HEADER, &rcHeader);
			DrawUtil::Fill(pnmcd->nmcd.hdc, &rcHeader, RGB(224, 224, 224));
		} else {
			// ウィンドウが無効状態の時、背景色が COLOR_3DFACE になってしまうため塗りつぶす。
			if (!::IsWindowEnabled(pnmcd->nmcd.hdr.hwndFrom)) {
				RECT rc;
				::GetClientRect(pnmcd->nmcd.hdr.hwndFrom, &rc);
				DrawUtil::Fill(pnmcd->nmcd.hdc, &rc, ListView_GetBkColor(pnmcd->nmcd.hdr.hwndFrom));
			}

			Result = CDRF_NOTIFYITEMDRAW;
		}
		break;

	case CDDS_ITEMPREPAINT:
		// ウィンドウが無効状態の時、デフォルトの色で描画されてしまうため色を指定する。
		if (!::IsWindowEnabled(pnmcd->nmcd.hdr.hwndFrom)) {
			const COLORREF TextColor = ListView_GetTextColor(pnmcd->nmcd.hdr.hwndFrom);
			const COLORREF BkColor = ListView_GetTextBkColor(pnmcd->nmcd.hdr.hwndFrom);
			pnmcd->clrText = MixColor(TextColor, BkColor);
			pnmcd->clrTextBk = BkColor;
			Result = CDRF_NEWFONT;
		}
		break;
	}

	return Result;
}


LRESULT CBasicDialog::CustomDrawDarkModeTreeView(NMTVCUSTOMDRAW *pnmcd)
{
	LRESULT Result = CDRF_DODEFAULT;

	switch (pnmcd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		Result = CDRF_NOTIFYITEMDRAW;
		break;

	case CDDS_ITEMPREPAINT:
		Result = CDRF_NOTIFYPOSTPAINT;
		break;

	case CDDS_ITEMPOSTPAINT:
		if ((pnmcd->nmcd.uItemState & CDIS_SELECTED) != 0) {
			const HTREEITEM hItem = reinterpret_cast<HTREEITEM>(pnmcd->nmcd.dwItemSpec);
			RECT rc;
			if (TreeView_GetItemRect(
					pnmcd->nmcd.hdr.hwndFrom, hItem, &rc,
					(GetWindowStyle(pnmcd->nmcd.hdr.hwndFrom) & TVS_FULLROWSELECT) == 0)) {
				DrawUtil::FillBorder(pnmcd->nmcd.hdc, rc, 1, DarkColorScheme::Border);
			}
		}
		break;
	}

	return Result;
}


LRESULT CBasicDialog::CustomDrawDarkModeTrackBar(NMCUSTOMDRAW *pnmcd)
{
	LRESULT Result = CDRF_DODEFAULT;

	switch (pnmcd->dwDrawStage) {
	case CDDS_PREPAINT:
		Result = CDRF_NOTIFYITEMDRAW;
		break;

	case CDDS_ITEMPREPAINT:
		if (pnmcd->dwItemSpec == TBCD_CHANNEL) {
			DrawUtil::Fill(
				pnmcd->hdc, &pnmcd->rc,
				MixColor(RGB(255, 255, 255), m_FaceColor, 32));
			Result = CDRF_SKIPDEFAULT;
		}
		break;
	}

	return Result;
}


void CBasicDialog::UpdateColors(bool fDarkMode)
{
	if (fDarkMode) {
		m_TextColor = DarkColorScheme::Text;
		m_BackColor = DarkColorScheme::ContentBack;
		m_FaceColor = DarkColorScheme::Back;
	} else {
		m_TextColor = ::GetSysColor(COLOR_WINDOWTEXT);
		m_BackColor = ::GetSysColor(COLOR_WINDOW);
		m_FaceColor = ::GetSysColor(COLOR_3DFACE);
	}

	if (m_hDlg != nullptr) {
		m_BackBrush.Create(m_BackColor);
		m_FaceBrush.Create(m_FaceColor);
	}
}


void CBasicDialog::InitializeControls(HWND hDlg, bool fDark)
{
	for (HWND hwnd = ::GetWindow(hDlg, GW_CHILD); hwnd != nullptr; hwnd = ::GetWindow(hwnd, GW_HWNDNEXT)) {
		if (!IsDisableThemingControl(hwnd)) {
			InitializeControl(hwnd, fDark);
			SetControlDarkTheme(hwnd, fDark);
		}
	}
}


void CBasicDialog::UpdateControlsTheme(HWND hDlg, bool fDark)
{
	for (HWND hwnd = ::GetWindow(hDlg, GW_CHILD); hwnd != nullptr; hwnd = ::GetWindow(hwnd, GW_HWNDNEXT)) {
		if (!IsDisableThemingControl(hwnd)) {
			TCHAR szClassName[64];
			if (::GetClassName(hwnd, szClassName, lengthof(szClassName)) == 6 && ::lstrcmp(szClassName, TEXT("#32770")) == 0) {
				CBasicDialog *pChildDialog = GetThis(hwnd);
				if (pChildDialog != nullptr) {
					pChildDialog->m_fDarkMode = fDark;
					pChildDialog->HandleDarkModeChanged(fDark);
				}
			}

			SetControlDarkTheme(hwnd, fDark);
		}
	}
}


void CBasicDialog::UpdateControlsColors(HWND hDlg, bool fDark)
{
	for (HWND hwnd = ::GetWindow(hDlg, GW_CHILD); hwnd != nullptr; hwnd = ::GetWindow(hwnd, GW_HWNDNEXT)) {
		if (!IsDisableThemingControl(hwnd))
			UpdateControlColors(hwnd, fDark);
	}
}


bool CBasicDialog::InitializeControl(HWND hwnd, bool fDark)
{
	TCHAR szClassName[64];

	if (::GetClassName(hwnd, szClassName, lengthof(szClassName)) < 1)
		return false;

	if (::lstrcmpi(szClassName, TEXT("BUTTON")) == 0) {
		::SetWindowSubclass(hwnd, ButtonSubclassProc, SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this));
		return true;
	}

	if (::lstrcmpi(szClassName, TEXT("STATIC")) == 0) {
		::SetWindowSubclass(hwnd, StaticSubclassProc, SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this));
		return true;
	}

	if (::lstrcmpi(szClassName, WC_LISTVIEW) == 0) {
		::SetWindowSubclass(hwnd, ListViewSubclassProc, SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this));
		return true;
	}

	if (::lstrcmpi(szClassName, WC_TABCONTROL) == 0) {
		::SetWindowSubclass(hwnd, TabSubclassProc, SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this));
		return true;
	}

	if (::lstrcmpi(szClassName, UPDOWN_CLASS) == 0) {
		UpDownInfo *pInfo = new UpDownInfo;
		pInfo->UpdateSize(hwnd);
		if (::SetWindowSubclass(hwnd, UpDownSubclassProc, SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this)))
			::SetProp(hwnd, UpDownInfo::PROP_NAME, pInfo);
		else
			delete pInfo;
		return true;
	}

	return false;
}


bool CBasicDialog::SetControlDarkTheme(HWND hwnd, bool fDark)
{
	TCHAR szClassName[64];

	if (::GetClassName(hwnd, szClassName, lengthof(szClassName)) < 1)
		return false;

	if (::lstrcmpi(szClassName, TEXT("BUTTON")) == 0)
		return SetWindowDarkTheme(hwnd, fDark);

	if (::lstrcmpi(szClassName, TEXT("COMBOBOX")) == 0) {
		COMBOBOXINFO cbi;
		cbi.cbSize = sizeof(COMBOBOXINFO);
		if (!::GetComboBoxInfo(hwnd, &cbi))
			return false;

		// テーマを設定するとなぜか全て選択状態になるので後で戻す
		const bool fDropDown = (GetWindowStyle(hwnd) & 3) == CBS_DROPDOWN;
		DWORD SelStart = 0, SelEnd = 0;
		if (fDropDown)
			::SendMessage(hwnd, CB_GETEDITSEL, reinterpret_cast<WPARAM>(&SelStart), reinterpret_cast<LPARAM>(&SelEnd));

		const bool fOK = SUCCEEDED(::SetWindowTheme(hwnd, fDark ? L"DarkMode_CFD" : nullptr, nullptr))
			&& SetWindowDarkTheme(cbi.hwndList, fDark);

		if (fDropDown)
			::SendMessage(hwnd, CB_SETEDITSEL, SelStart, SelEnd);

		return fOK;
	}

	if (::lstrcmpi(szClassName, TEXT("EDIT")) == 0) {
		return SUCCEEDED(::SetWindowTheme(
			hwnd,
			fDark ? ((GetWindowStyle(hwnd) & ES_MULTILINE) == 0 ? L"DarkMode_CFD" : L"DarkMode_Explorer") : nullptr,
			nullptr));
	}

	if (::lstrcmpi(szClassName, TEXT("LISTBOX")) == 0)
		return SetWindowDarkTheme(hwnd, fDark);

	if (::lstrcmpi(szClassName, TEXT("SCROLLBAR")) == 0)
		return SetWindowDarkTheme(hwnd, fDark);

	if (::lstrcmpi(szClassName, TEXT("STATIC")) == 0)
		return true;

	if (::lstrcmpi(szClassName, WC_LISTVIEW) == 0) {
		/*
			TODO:
			グリッドの色は変更できないためダークモード時にはグリッドを無効にしているが、
			カスタム描画でグリッドを描くべきか？
		*/
		if (fDark)
			ListView_SetExtendedListViewStyleEx(hwnd, LVS_EX_GRIDLINES, 0);

		::SetWindowTheme(hwnd, fDark ? L"DarkMode_Explorer" : nullptr, nullptr);

		const HWND hwndHeader = ListView_GetHeader(hwnd);
		if (hwndHeader != nullptr)
			::SetWindowTheme(hwndHeader, fDark ? L"DarkMode_ItemsView" : nullptr, nullptr);

		const HWND hwndToolTips = ListView_GetToolTips(hwnd);
		if (hwndToolTips != nullptr)
			SetWindowDarkTheme(hwndToolTips, fDark);

		UpdateControlColors(hwnd, fDark);
		return true;
	}

	if (::lstrcmpi(szClassName, WC_TABCONTROL) == 0) {
		const HWND hwndToolTips = TabCtrl_GetToolTips(hwnd);
		if (hwndToolTips != nullptr)
			SetWindowDarkTheme(hwndToolTips, fDark);
		return true;
	}

	if (::lstrcmpi(szClassName, TOOLTIPS_CLASS) == 0)
		return SetWindowDarkTheme(hwnd, fDark);

	if (::lstrcmpi(szClassName, TRACKBAR_CLASS) == 0) {
		const DWORD Style = GetWindowStyle(hwnd);
		if (((Style & TBS_TRANSPARENTBKGND) != 0) != fDark)
			::SetWindowLong(hwnd, GWL_STYLE, Style ^ TBS_TRANSPARENTBKGND);
		return true;
	}

	if (::lstrcmpi(szClassName, WC_TREEVIEW) == 0) {
		::SetWindowTheme(hwnd, fDark ? L"DarkMode_Explorer" : nullptr, nullptr);

		const HWND hwndToolTips = TreeView_GetToolTips(hwnd);
		if (hwndToolTips != nullptr)
			SetWindowDarkTheme(hwndToolTips, fDark);

		UpdateControlColors(hwnd, fDark);
		return true;
	}

	return SetWindowDarkTheme(hwnd, fDark);
}


bool CBasicDialog::UpdateControlColors(HWND hwnd, bool fDark)
{
	TCHAR szClassName[64];

	if (::GetClassName(hwnd, szClassName, lengthof(szClassName)) < 1)
		return false;

	if (::lstrcmpi(szClassName, WC_LISTVIEW) == 0) {
		ListView_SetTextColor(hwnd, m_TextColor);
		ListView_SetBkColor(hwnd, m_BackColor);
		ListView_SetTextBkColor(hwnd, m_BackColor);

		for (int i = 0; i < 2; i++) {
			const HIMAGELIST himl = ListView_GetImageList(hwnd, i);
			if (himl != nullptr)
				::ImageList_SetBkColor(himl, m_BackColor);
		}

		return true;
	}

	if (::lstrcmpi(szClassName, WC_TREEVIEW) == 0) {
		TreeView_SetTextColor(hwnd, m_TextColor);
		TreeView_SetBkColor(hwnd, m_BackColor);

		const HIMAGELIST himl = TreeView_GetImageList(hwnd, TVSIL_NORMAL);
		if (himl != nullptr)
			::ImageList_SetBkColor(himl, m_BackColor);

		return true;
	}

	return false;
}


COLORREF CBasicDialog::GetThemeColor(int Type) const
{
	switch (Type) {
	case COLOR_WINDOW:
		return m_BackColor;

	case COLOR_3DFACE:
		return m_FaceColor;

	case COLOR_WINDOWTEXT:
		return m_TextColor;

	case COLOR_MENU:
		if (m_fDarkMode)
			return DarkColorScheme::MenuBack;
		break;

	case COLOR_MENUTEXT:
		if (m_fDarkMode)
			return DarkColorScheme::Text;
		break;

	case COLOR_GRAYTEXT:
		if (m_fDarkMode) {
			return MixColor(m_TextColor, m_FaceColor);
		}
		break;
	}

	return ::GetSysColor(Type);
}


void CBasicDialog::DrawDarkModeStatic(HWND hwnd, HDC hdc, const RECT &PaintRect, DWORD Style)
{
	RECT rc;
	::GetClientRect(hwnd, &rc);

	RECT rcFill;
	if (::IntersectRect(&rcFill, &rc, &PaintRect)) {
		::FillRect(hdc, &rcFill, m_FaceBrush.GetHandle());

		TCHAR szText[256];
		const int TextLength = ::GetWindowText(hwnd, szText, lengthof(szText));
		if (TextLength > 0) {
			UINT Format = 0;

			switch (Style & SS_TYPEMASK) {
			case SS_CENTER:         Format |= DT_CENTER;     break;
			case SS_RIGHT:          Format |= DT_RIGHT;      break;
			case SS_LEFTNOWORDWRAP: Format |= DT_SINGLELINE; break;
			}

			if ((Style & SS_NOPREFIX) != 0)
				Format |= DT_NOPREFIX;
			else if ((::SendMessage(hwnd, WM_QUERYUISTATE, 0, 0) & UISF_HIDEACCEL) != 0)
				Format |= DT_HIDEPREFIX;

			if ((Style & SS_EDITCONTROL) != 0)
				Format |= DT_EDITCONTROL;

			switch (Style & SS_ELLIPSISMASK) {
			case SS_ENDELLIPSIS:  Format |= DT_END_ELLIPSIS;  break;
			case SS_PATHELLIPSIS: Format |= DT_PATH_ELLIPSIS; break;
			case SS_WORDELLIPSIS: Format |= DT_WORD_ELLIPSIS; break;
			}

			const HGDIOBJ hOldFont = ::SelectObject(hdc, GetWindowFont(hwnd));
			const COLORREF OldTextColor = ::SetTextColor(hdc, GetThemeColor(COLOR_GRAYTEXT));
			const COLORREF OldBkColor = ::SetBkColor(hdc, m_FaceColor);
			::DrawText(hdc, szText, TextLength, &rc, Format);
			::SelectObject(hdc, hOldFont);
			::SetTextColor(hdc, OldTextColor);
			::SetBkColor(hdc, OldBkColor);
		}
	}
}


void CBasicDialog::DrawDarkModeGroupBox(HWND hwnd, HDC hdc)
{
	RECT rc;
	::GetClientRect(hwnd, &rc);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	RECT rcBox = rc;
	rcBox.top += (tm.tmHeight - tm.tmDescent) / 2;

	TCHAR szText[256];
	const int TextLength = ::GetWindowText(hwnd, szText, lengthof(szText));
	RECT rcText = {}, rcTextBack;
	UINT TextFormat;
	HGDIOBJ hOldFont;
	if (TextLength > 0) {
		hOldFont = ::SelectObject(hdc, GetWindowFont(hwnd));
		TextFormat = DT_SINGLELINE;
		if ((::SendMessage(hwnd, WM_QUERYUISTATE, 0, 0) & UISF_HIDEACCEL) != 0)
			TextFormat |= DT_HIDEPREFIX;
		::DrawText(hdc, szText, TextLength, &rcText, TextFormat | DT_CALCRECT);
		constexpr int Margin = 2;
		::OffsetRect(&rcText, rcBox.top + Margin, rc.top);
		if (rcText.right > rc.right - Margin)
			rcText.right = rc.right - Margin;
		rcTextBack = rcText;
		::InflateRect(&rcTextBack, Margin, 0);
		::ExcludeClipRect(hdc, rcTextBack.left, rcTextBack.top, rcTextBack.right, rcTextBack.bottom);
	}

	DrawUtil::FillBorder(hdc, rcBox, 1, DarkColorScheme::Border);

	::SelectClipRgn(hdc, nullptr);

	if (TextLength > 0) {
		DrawUtil::Fill(hdc, &rcTextBack, m_FaceColor);
		const COLORREF OldTextColor = ::SetTextColor(hdc, m_TextColor);
		const COLORREF OldBkColor = ::SetBkColor(hdc, m_FaceColor);
		::DrawText(hdc, szText, TextLength, &rcText, TextFormat);
		::SetTextColor(hdc, OldTextColor);
		::SetBkColor(hdc, OldBkColor);
		::SelectObject(hdc, hOldFont);
	}
}


void CBasicDialog::DrawDarkModeTab(HWND hwnd, HDC hdc, const RECT &PaintRect)
{
	const DWORD Style = GetWindowStyle(hwnd);
	const int ItemCount = TabCtrl_GetItemCount(hwnd);
	const int SelectedItem = TabCtrl_GetCurSel(hwnd);

	const HIMAGELIST himl = TabCtrl_GetImageList(hwnd);
	int IconWidth, IconHeight;
	if (himl != nullptr)
		ImageList_GetIconSize(himl, &IconWidth, &IconHeight);

	const COLORREF OldTextColor = ::GetTextColor(hdc);
	const int OldBkMode = ::SetBkMode(hdc, TRANSPARENT);
	const HFONT hfont = GetWindowFont(hwnd);
	const HGDIOBJ hOldFont = ::SelectObject(hdc, hfont);
	constexpr int IconTextMargin = 3;
	const int BorderWidth = ::MulDiv(1, m_CurrentDPI, 96);
	int TabBottom = 0;

	RECT rcClient;
	::GetClientRect(hwnd, &rcClient);
	RECT rc = rcClient;
	TabCtrl_AdjustRect(hwnd, FALSE, &rc);
	rc.bottom = std::min(rc.top, PaintRect.bottom);
	rc.left = PaintRect.left;
	rc.top = PaintRect.top;
	rc.right = PaintRect.right;
	if (rc.top < rc.bottom)
		::FillRect(hdc, &rc, m_BackBrush.GetHandle());

	TCITEM tci;
	TCHAR Text[256];

	tci.mask = TCIF_STATE | TCIF_TEXT;
	if (himl != nullptr)
		tci.mask |= TCIF_IMAGE;
	tci.dwStateMask = TCIS_HIGHLIGHTED;
	tci.pszText = Text;
	tci.cchTextMax = lengthof(Text);

	for (int i = 0; i < ItemCount; i++) {
		TabCtrl_GetItemRect(hwnd, i, &rc);

		if ((Style & TCS_BUTTONS) == 0) {
			rc.right++;
			rc.top--;
			rc.bottom += 2;
		}

		if (TabBottom < rc.bottom)
			TabBottom = rc.bottom;

		TabCtrl_GetItem(hwnd, i, &tci);

		COLORREF TextColor = m_TextColor;

		if (i == SelectedItem) {
			DrawUtil::Fill(hdc, &rc, DarkColorScheme::SelectedBack);
		} else {
			if ((Style & TCS_BUTTONS) == 0)
				rc.top += 2;
			if ((Style & (TCS_BUTTONS | TCS_FLATBUTTONS)) != (TCS_BUTTONS | TCS_FLATBUTTONS)) {
				DrawUtil::FillBorder(hdc, rc, BorderWidth, DarkColorScheme::Edge);
				::InflateRect(&rc, -BorderWidth, -BorderWidth);
			}
			if ((tci.dwState & TCIS_HIGHLIGHTED) != 0) {
				DrawUtil::Fill(hdc, &rc, GetThemeColor(COLOR_HIGHLIGHT));
				TextColor = GetThemeColor(COLOR_HIGHLIGHTTEXT);
			}
		}

		RECT rcText = {};
		::DrawText(hdc, Text, -1, &rcText, DT_CALCRECT | DT_SINGLELINE | DT_HIDEPREFIX);
		if (himl != nullptr && tci.iImage >= 0)
			rcText.right += IconWidth + IconTextMargin;
		RECT rcContent = rc;
		rcContent.left += std::max<int>(((rc.right - rc.left) - rcText.right) / 2, 0);

		if (himl != nullptr && tci.iImage >= 0) {
			ImageList_Draw(
				himl, tci.iImage, hdc,
				rcContent.left,
				rcContent.top + ((rcContent.bottom - rcContent.top) - IconHeight) / 2,
				ILD_TRANSPARENT);
			rcContent.left += IconWidth + IconTextMargin;
		}

		::SetTextColor(hdc, TextColor);
		::DrawText(hdc, Text, -1, &rcContent, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_HIDEPREFIX);

		if (i == SelectedItem
				&& ::GetFocus() == hwnd
				&& (::SendMessage(hwnd, WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS) == 0) {
			::InflateRect(&rc, -2, -2);
			::DrawFocusRect(hdc, &rc);
		}
	}

	rc = rcClient;
	rc.top = TabBottom - BorderWidth;
	if (rc.top < rc.bottom) {
		DrawUtil::FillBorder(hdc, rc, BorderWidth, DarkColorScheme::Edge);
		::InflateRect(&rc, -BorderWidth, -BorderWidth);
		::FillRect(hdc, &rc, m_FaceBrush.GetHandle());
	}

	::SetTextColor(hdc, OldTextColor);
	::SetBkMode(hdc, OldBkMode);
	::SelectObject(hdc, hOldFont);
}


void CBasicDialog::DrawDarkModeUpDown(HWND hwnd, HDC hdc, const RECT &PaintRect)
{
	UpDownInfo *pInfo = UpDownInfo::FromWindow(hwnd);
	if (pInfo == nullptr)
		return;

	const DWORD Style = GetWindowStyle(hwnd);
	const bool fDisabled = (Style & WS_DISABLED) != 0;
	const bool fHorz = (Style & UDS_HORZ) != 0;

	if (!pInfo->Font.IsCreated()) {
		LOGFONT lf = {};

		lf.lfHeight = -::MulDiv(9, m_CurrentDPI, 96);
		lf.lfWeight = FW_NORMAL;
		lf.lfCharSet = SYMBOL_CHARSET;
		lf.lfQuality = DRAFT_QUALITY;
		::lstrcpy(lf.lfFaceName, TEXT("Marlett"));

		pInfo->Font.Create(&lf);
	}

	const HGDIOBJ hOldFont = ::SelectObject(hdc, pInfo->Font.GetHandle());
	const COLORREF OldTextColor = ::SetTextColor(hdc, fDisabled ? MixColor(m_TextColor, m_FaceColor) : m_TextColor);
	const int OldBkMode = ::SetBkMode(hdc, TRANSPARENT);

	const int BorderWidth = ::MulDiv(1, m_CurrentDPI, 96);
	RECT rc;

	rc = pInfo->UpRect;
	DrawUtil::FillBorder(
		hdc, rc, BorderWidth, &PaintRect,
		fDisabled ? MixColor(DarkColorScheme::Border, m_BackColor) :
		pInfo->HotItem == UpDownInfo::ItemType::Up ? DarkColorScheme::HotBorder : DarkColorScheme::Border);
	::InflateRect(&rc, -BorderWidth, -BorderWidth);
	DrawUtil::Fill(
		hdc, &rc,
		pInfo->HotItem == UpDownInfo::ItemType::Up ?
			DarkColorScheme::SelectedBack :
			m_FaceColor);
	::DrawText(
		hdc,
		fHorz ? TEXT("\x33") : TEXT("\x35"),
		1, &rc, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX);

	rc = pInfo->CenterRect;
	if (rc.left < rc.right && rc.top < rc.bottom)
		DrawUtil::Fill(hdc, &rc, m_BackColor);

	rc = pInfo->DownRect;
	DrawUtil::FillBorder(
		hdc, rc, BorderWidth, &PaintRect,
		fDisabled ? MixColor(DarkColorScheme::Border, m_BackColor) :
		pInfo->HotItem == UpDownInfo::ItemType::Down ? DarkColorScheme::HotBorder : DarkColorScheme::Border);
	::InflateRect(&rc, -BorderWidth, -BorderWidth);
	DrawUtil::Fill(
		hdc, &rc,
		pInfo->HotItem == UpDownInfo::ItemType::Down ?
			DarkColorScheme::SelectedBack :
			m_FaceColor);
	::DrawText(
		hdc,
		fHorz ? TEXT("\x34") : TEXT("\x36"),
		1, &rc, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX);

	::SelectObject(hdc, hOldFont);
	::SetTextColor(hdc, OldTextColor);
	::SetBkMode(hdc, OldBkMode);
}


LRESULT CBasicDialog::DarkModeHeaderCustomDraw(NMCUSTOMDRAW *pnmcd)
{
	switch (pnmcd->dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		::SetTextColor(
			pnmcd->hdc,
			!::IsWindowEnabled(::GetParent(pnmcd->hdr.hwndFrom)) ?
				GetThemeColor(COLOR_GRAYTEXT) :
				m_TextColor);
		return CDRF_DODEFAULT;
	}

	return CDRF_DODEFAULT;
}


CBasicDialog::ControlThemeInfo * CBasicDialog::GetThemeInfo(HWND hwnd, LPCWSTR pszClassList)
{
	if (!::IsAppThemed())
		return nullptr;

	auto Result = m_ControlThemeMap.emplace(
		std::piecewise_construct,
		std::forward_as_tuple(hwnd),
		std::forward_as_tuple());
	ControlThemeInfo *pThemeInfo = &Result.first->second;
	if (Result.second)
		pThemeInfo->Theme.Open(hwnd, pszClassList, m_CurrentDPI);
	if (!pThemeInfo->Theme.IsOpen())
		return nullptr;
	return pThemeInfo;
}


void CBasicDialog::DeleteTheme(HWND hwnd)
{
	m_ControlThemeMap.erase(hwnd);
}


LRESULT CALLBACK CBasicDialog::StaticSubclassProc(
	HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	CBasicDialog *pThis = reinterpret_cast<CBasicDialog *>(dwRefData);

	switch (uMsg) {
	case WM_PAINT:
		if (pThis->m_fDarkMode) {
			const DWORD Style = GetWindowStyle(hWnd);
			const DWORD Type = Style & SS_TYPEMASK;

			switch (Type) {
			case SS_LEFT:
			case SS_CENTER:
			case SS_RIGHT:
			case SS_LEFTNOWORDWRAP:
				if ((Style & WS_DISABLED) != 0) {
					PAINTSTRUCT ps;

					::BeginPaint(hWnd, &ps);
					pThis->DrawDarkModeStatic(hWnd, ps.hdc, ps.rcPaint, Style);
					::EndPaint(hWnd, &ps);

					return 0;
				}
				break;
			}
		}
		break;

	case WM_ENABLE:
		// 無効状態になった時、WM_PAINT が呼ばれずに勝手に描かれてしまう
		if (wParam != FALSE)
			break;
		[[fallthrough]];
	case WM_UPDATEUISTATE:
		// アクセスキーの下線が表示される時、線だけが勝手に描かれてしまう
		if (pThis->m_fDarkMode && ::IsWindowVisible(hWnd)) {
			::SendMessage(hWnd, WM_SETREDRAW, FALSE, 0);
			const LRESULT Result = ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
			::SendMessage(hWnd, WM_SETREDRAW, TRUE, 0);
			::InvalidateRect(hWnd, nullptr, TRUE);
			return Result;
		}
		break;

	case WM_NCPAINT:
		if (pThis->m_fDarkMode
				&& (GetWindowExStyle(hWnd) & (WS_EX_CLIENTEDGE | WS_EX_STATICEDGE)) != 0) {
			UINT Flags = DCX_WINDOW | DCX_CACHE | DCX_LOCKWINDOWUPDATE | DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS | 0x00010000/*DCX_USESTYLE*/;
			if (wParam != 0)
				Flags |= DCX_INTERSECTRGN | 0x00040000/*DCX_NODELETERGN*/;
			HDC hdc = ::GetDCEx(hWnd, reinterpret_cast<HRGN>(wParam), Flags);
			if (hdc == nullptr)
				hdc = ::GetWindowDC(hWnd);
			if (hdc != nullptr) {
				RECT rcWindow, rcClient;
				::GetWindowRect(hWnd, &rcWindow);
				::GetClientRect(hWnd, &rcClient);
				MapWindowRect(hWnd, nullptr, &rcClient);
				::OffsetRect(&rcClient, -rcWindow.left, -rcWindow.top);
				::OffsetRect(&rcWindow, -rcWindow.left, -rcWindow.top);
				DrawUtil::FillBorder(hdc, &rcWindow, &rcClient, &rcWindow, DarkColorScheme::Edge);
				::ReleaseDC(hWnd, hdc);
				return 0;
			}
		}
		break;

	case WM_NCDESTROY:
		::RemoveWindowSubclass(hWnd, StaticSubclassProc, uIdSubclass);
		break;
	}

	return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


LRESULT CALLBACK CBasicDialog::ButtonSubclassProc(
	HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	CBasicDialog *pThis = reinterpret_cast<CBasicDialog *>(dwRefData);

	switch (uMsg) {
	case WM_PAINT:
		if (pThis->m_fDarkMode) {
			const DWORD Style = GetWindowStyle(hWnd);
			const DWORD Type = Style & BS_TYPEMASK;

			if (Type == BS_GROUPBOX) {
				PAINTSTRUCT ps;

				::BeginPaint(hWnd, &ps);
				pThis->DrawDarkModeGroupBox(hWnd, ps.hdc);
				::EndPaint(hWnd, &ps);

				return 0;
			}
		}
		break;

	case WM_ENABLE:
		// 無効状態になった時、WM_PAINT が呼ばれずに勝手に描かれてしまう
		if (wParam == FALSE && pThis->m_fDarkMode) {
			::SendMessage(hWnd, WM_SETREDRAW, FALSE, 0);
			const LRESULT Result = ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
			::SendMessage(hWnd, WM_SETREDRAW, TRUE, 0);
			::InvalidateRect(hWnd, nullptr, TRUE);
			return Result;
		}
		break;

	case WM_SIZE:
		::BufferedPaintStopAllAnimations(hWnd);
		break;

	case WM_DESTROY:
		::BufferedPaintStopAllAnimations(hWnd);
		break;

	case WM_NCDESTROY:
		pThis->DeleteTheme(hWnd);
		::RemoveWindowSubclass(hWnd, ButtonSubclassProc, uIdSubclass);
		break;
	}

	return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


LRESULT CALLBACK CBasicDialog::ListViewSubclassProc(
	HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	CBasicDialog *pThis = reinterpret_cast<CBasicDialog *>(dwRefData);

	switch (uMsg) {
	case WM_NOTIFY:
		if (pThis->m_fDarkMode) {
			NMHDR *pnmh = reinterpret_cast<NMHDR *>(lParam);

			switch (pnmh->code) {
			case NM_CUSTOMDRAW:
				if (pnmh->hwndFrom == ListView_GetHeader(hWnd)) {
					return pThis->DarkModeHeaderCustomDraw(reinterpret_cast<NMCUSTOMDRAW *>(pnmh));
				}
				break;
			}
		}
		break;

	case WM_NCDESTROY:
		::RemoveWindowSubclass(hWnd, ListViewSubclassProc, uIdSubclass);
		break;
	}

	return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


LRESULT CALLBACK CBasicDialog::TabSubclassProc(
	HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	CBasicDialog *pThis = reinterpret_cast<CBasicDialog *>(dwRefData);

	switch (uMsg) {
	case WM_PAINT:
		if (pThis->m_fDarkMode) {
			PAINTSTRUCT ps;

			::BeginPaint(hWnd, &ps);
			pThis->DrawDarkModeTab(hWnd, ps.hdc, ps.rcPaint);
			::EndPaint(hWnd, &ps);

			return 0;
		}
		break;

	case WM_NCDESTROY:
		::RemoveWindowSubclass(hWnd, TabSubclassProc, uIdSubclass);
		break;
	}

	return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


LRESULT CALLBACK CBasicDialog::UpDownSubclassProc(
	HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	CBasicDialog *pThis = reinterpret_cast<CBasicDialog *>(dwRefData);

	switch (uMsg) {
	case WM_PAINT:
		if (pThis->m_fDarkMode) {
			PAINTSTRUCT ps;

			::BeginPaint(hWnd, &ps);
			pThis->DrawDarkModeUpDown(hWnd, ps.hdc, ps.rcPaint);
			::EndPaint(hWnd, &ps);

			return 0;
		}
		break;

	case WM_SIZE:
		{
			UpDownInfo *pInfo = UpDownInfo::FromWindow(hWnd);
			if (pInfo != nullptr) {
				pInfo->UpdateSize(hWnd);
				pInfo->HotItem = UpDownInfo::ItemType::Invalid;
			}
		}
		break;

	case WM_MOUSEMOVE:
		{
			UpDownInfo *pInfo = UpDownInfo::FromWindow(hWnd);
			if (pInfo != nullptr)
				pInfo->HotItem = pInfo->HitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		break;

	case WM_MOUSELEAVE:
		{
			UpDownInfo *pInfo = UpDownInfo::FromWindow(hWnd);
			if (pInfo != nullptr)
				pInfo->HotItem = UpDownInfo::ItemType::Invalid;
		}
		break;

	case WM_NCDESTROY:
		{
			UpDownInfo *pInfo = UpDownInfo::FromWindow(hWnd);
			if (pInfo != nullptr) {
				::RemoveProp(hWnd, UpDownInfo::PROP_NAME);
				delete pInfo;
			}

			::RemoveWindowSubclass(hWnd, UpDownSubclassProc, uIdSubclass);
		}
		break;
	}

	return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}




CResizableDialog::~CResizableDialog()
{
	Destroy();
}


INT_PTR CResizableDialog::HandleMessage(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			RECT rc;

			if (IsWindowPerMonitorDPIV2(hDlg))
				m_BaseDPI = GetWindowDPI(hDlg);
			else
				m_BaseDPI = m_pStyleScaling->GetSystemDPI();

			::GetClientRect(hDlg, &rc);
			m_OriginalClientSize.cx = rc.right;
			m_OriginalClientSize.cy = rc.bottom;

			InitDialog();

			if ((::GetWindowLong(hDlg, GWL_STYLE)&WS_CHILD) == 0) {
				::GetClientRect(hDlg, &rc);
				m_hwndSizeGrip = ::CreateWindowEx(
					0, TEXT("SCROLLBAR"), nullptr,
					WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SBS_SIZEGRIP |
					SBS_SIZEBOXBOTTOMRIGHTALIGN,
					0, 0, rc.right, rc.bottom, m_hDlg, (HMENU)0,
					reinterpret_cast<HINSTANCE>(::GetWindowLongPtr(m_hDlg, GWLP_HINSTANCE)), nullptr);
				::SetWindowPos(m_hwndSizeGrip, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			} else {
				m_hwndSizeGrip = nullptr;
			}
		}
		break;

	case WM_GETMINMAXINFO:
		{
			MINMAXINFO *pmmi = reinterpret_cast<MINMAXINFO*>(lParam);

			pmmi->ptMinTrackSize.x = m_MinSize.cx;
			pmmi->ptMinTrackSize.y = m_MinSize.cy;
		}
		return TRUE;

	case WM_SIZE:
		DoLayout();
		break;

	case WM_DPICHANGED:
		if (!m_fInitializing)
			OnDPIChanged(hDlg, wParam, lParam);
		break;

	case WM_DESTROY:
		return CBasicDialog::HandleMessage(hDlg, uMsg, wParam, lParam);
	}

	return DlgProc(hDlg, uMsg, wParam, lParam);
}


bool CResizableDialog::ApplyPosition()
{
	if (m_Position.Width < m_MinSize.cx)
		m_Position.Width = m_MinSize.cx;
	if (m_Position.Height < m_MinSize.cy)
		m_Position.Height = m_MinSize.cy;

	return CBasicDialog::ApplyPosition();
}


void CResizableDialog::DoLayout()
{
	RECT rc;
	::GetClientRect(m_hDlg, &rc);
	const int Width = rc.right;
	const int Height = rc.bottom;
	const int WidthDiff = Width - m_ScaledClientSize.cx;
	const int HeightDiff = Height - m_ScaledClientSize.cy;

	HDWP hdwp = ::BeginDeferWindowPos(static_cast<int>(m_ControlList.size()));

	for (const auto &e : m_ControlList) {
		rc = e.rcOriginal;

		const int DPI = e.DPI;
		if (DPI != m_CurrentDPI) {
			rc.left = ::MulDiv(rc.left, m_CurrentDPI, DPI);
			rc.top = ::MulDiv(rc.top, m_CurrentDPI, DPI);
			rc.right = ::MulDiv(rc.right, m_CurrentDPI, DPI);
			rc.bottom = ::MulDiv(rc.bottom, m_CurrentDPI, DPI);
		}

		if (!!(e.Align & AlignFlag::RightHalf)) {
			rc.right += WidthDiff / 2;
			if (!!(e.Align & AlignFlag::LeftHalf) || !(e.Align & AlignFlag::Left))
				rc.left += WidthDiff / 2;
		} else if (!!(e.Align & AlignFlag::Right)) {
			rc.right += WidthDiff;
			if (!!(e.Align & AlignFlag::LeftHalf))
				rc.left += WidthDiff / 2;
			else if (!(e.Align & AlignFlag::Left))
				rc.left += WidthDiff;
		} else if (!!(e.Align & AlignFlag::LeftHalf)) {
			rc.left += WidthDiff / 2;
			rc.right += WidthDiff / 2;
		}
		if (rc.right < rc.left)
			rc.right = rc.left;

		if (!!(e.Align & AlignFlag::BottomHalf)) {
			rc.bottom += HeightDiff / 2;
			if (!!(e.Align & AlignFlag::TopHalf) || !(e.Align & AlignFlag::Top))
				rc.top += HeightDiff / 2;
		} else if (!!(e.Align & AlignFlag::Bottom)) {
			rc.bottom += HeightDiff;
			if (!!(e.Align & AlignFlag::TopHalf))
				rc.top += HeightDiff / 2;
			else if (!(e.Align & AlignFlag::Top))
				rc.top += HeightDiff;
		} else if (!!(e.Align & AlignFlag::TopHalf)) {
			rc.top += HeightDiff / 2;
			rc.bottom += HeightDiff / 2;
		}
		if (rc.bottom < rc.top)
			rc.bottom = rc.top;

		if (hdwp != nullptr) {
			hdwp = ::DeferWindowPos(
				hdwp, ::GetDlgItem(m_hDlg, e.ID), nullptr,
				rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
				SWP_NOZORDER | SWP_NOACTIVATE);
		} else {
			::SetWindowPos(
				::GetDlgItem(m_hDlg, e.ID), nullptr,
				rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
				SWP_NOZORDER | SWP_NOACTIVATE);
		}
	}

	if (hdwp != nullptr)
		::EndDeferWindowPos(hdwp);

	if (m_hwndSizeGrip != nullptr) {
		::GetWindowRect(m_hwndSizeGrip, &rc);
		::OffsetRect(&rc, -rc.left, -rc.top);
		::MoveWindow(
			m_hwndSizeGrip, Width - rc.right, Height - rc.bottom,
			rc.right, rc.bottom, TRUE);
	}
}


bool CResizableDialog::AddControl(int ID, AlignFlag Align)
{
	const HWND hwnd = ::GetDlgItem(m_hDlg, ID);
	if (hwnd == nullptr)
		return false;

	LayoutItem Item;

	Item.ID = ID;
	::GetWindowRect(hwnd, &Item.rcOriginal);
	::MapWindowPoints(nullptr, m_hDlg, reinterpret_cast<LPPOINT>(&Item.rcOriginal), 2);
	Item.DPI = m_CurrentDPI;
	Item.Align = Align;
	m_ControlList.push_back(Item);
	return true;
}


bool CResizableDialog::AddControls(int FirstID, int LastID, AlignFlag Align)
{
	if (FirstID > LastID)
		return false;
	for (int i = FirstID; i <= LastID; i++) {
		if (!AddControl(i, Align))
			return false;
	}
	return true;
}


void CResizableDialog::AddControls(std::initializer_list<ControlAlignInfo> List)
{
	for (const ControlAlignInfo &Info : List)
		AddControl(Info.ID, Info.Align);
}


bool CResizableDialog::UpdateControlPosition(int ID)
{
	for (auto &e : m_ControlList) {
		if (e.ID == ID) {
			GetDlgItemRect(m_hDlg, ID, &e.rcOriginal);
			e.DPI = m_CurrentDPI;
			return true;
		}
	}

	return false;
}


void CResizableDialog::ApplyStyle()
{
	CBasicDialog::ApplyStyle();

	if (m_hDlg != nullptr) {
		const int DPI = m_pStyleScaling->GetDPI();
		RECT rc;

		m_ScaledClientSize.cx = ::MulDiv(m_OriginalClientSize.cx, DPI, m_BaseDPI);
		m_ScaledClientSize.cy = ::MulDiv(m_OriginalClientSize.cy, DPI, m_BaseDPI);

		rc.left = 0;
		rc.top = 0;
		rc.right = m_ScaledClientSize.cx;
		rc.bottom = m_ScaledClientSize.cy;
		::AdjustWindowRectEx(&rc, GetWindowStyle(m_hDlg), FALSE, GetWindowExStyle(m_hDlg));
		m_MinSize.cx = rc.right - rc.left;
		m_MinSize.cy = rc.bottom - rc.top;
	}
}


void CResizableDialog::RealizeStyle()
{
	CBasicDialog::RealizeStyle();
}


}	// namespace TVTest
