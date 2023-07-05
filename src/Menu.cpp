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
#include "Menu.h"
#include "AppMain.h"
#include "GUIUtil.h"
#include "DPIUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


static int MyTrackPopupMenu(
	HMENU hmenu, UINT Flags, HWND hwnd, const POINT *pPos = nullptr, const RECT *pExcludeRect = nullptr)
{
	POINT pt;
	TPMPARAMS tpm;

	if (pPos != nullptr)
		pt = *pPos;
	else
		::GetCursorPos(&pt);

	if (pExcludeRect != nullptr) {
		tpm.cbSize = sizeof(TPMPARAMS);
		tpm.rcExclude = *pExcludeRect;
	}

	return ::TrackPopupMenuEx(hmenu, Flags, pt.x, pt.y, hwnd, pExcludeRect != nullptr ? &tpm : nullptr);
}




CMainMenu::~CMainMenu()
{
	Destroy();
}


bool CMainMenu::Create(HINSTANCE hinst)
{
	if (m_hmenu)
		return false;
	m_hmenu = ::LoadMenu(hinst, MAKEINTRESOURCE(IDM_MENU));
	if (m_hmenu == nullptr)
		return false;
	m_hmenuPopup = ::GetSubMenu(m_hmenu, 0);
	return true;
}


void CMainMenu::Destroy()
{
	if (m_hmenu) {
		::DestroyMenu(m_hmenu);
		m_hmenu = nullptr;
		m_hmenuPopup = nullptr;
	}
}


bool CMainMenu::Show(UINT Flags, int x, int y, HWND hwnd, bool fToggle, const std::vector<int> *pItemList)
{
	if (m_hmenu == nullptr)
		return false;

	if (!m_fPopup || !fToggle || m_PopupMenu >= 0) {
		if (m_fPopup)
			::EndMenu();

		HMENU hmenuCustom = m_hmenuPopup;

		if (pItemList != nullptr && !pItemList->empty()) {
			hmenuCustom = ::CreatePopupMenu();
			const int OrigItemCount = ::GetMenuItemCount(m_hmenuPopup);
			const CCommandManager &CommandManager = GetAppClass().CommandManager;

			MENUITEMINFO mii;
			TCHAR szText[256];

			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_STRING;
			mii.dwTypeData = szText;

			for (const int ID : *pItemList) {
				if (ID < 0) {
					::AppendMenu(hmenuCustom, MF_SEPARATOR, 0, nullptr);
				} else if (ID >= CM_COMMAND_FIRST) {
					// トップレベルの項目にあればコピー
					int i;
					for (i = 0; i < OrigItemCount; i++) {
						if (::GetMenuItemID(m_hmenuPopup, i) == static_cast<UINT>(ID))
							break;
					}
					if (i < OrigItemCount) {
						mii.cch = lengthof(szText);
						if (::GetMenuItemInfo(m_hmenuPopup, i, TRUE, &mii))
							::InsertMenuItem(hmenuCustom, ::GetMenuItemCount(hmenuCustom), TRUE, &mii);
					} else if (CommandManager.GetCommandShortText(ID, szText, lengthof(szText)) > 0) {
						const CCommandManager::CommandState State = CommandManager.GetCommandState(ID);
						UINT Flags = MF_STRING;
						if (!!(State & CCommandManager::CommandState::Disabled))
							Flags |= MF_GRAYED;
						if (!!(State & CCommandManager::CommandState::Checked))
							Flags |= MF_CHECKED;
						::AppendMenu(hmenuCustom, Flags, ID, FormatMenuString(szText).c_str());
					}
				} else {
					mii.cch = lengthof(szText);
					if (::GetMenuItemInfo(m_hmenuPopup, ID, TRUE, &mii))
						::InsertMenuItem(hmenuCustom, ::GetMenuItemCount(hmenuCustom), TRUE, &mii);
				}
			}

			if (::GetMenuItemCount(hmenuCustom) == 0) {
				::DestroyMenu(hmenuCustom);
				hmenuCustom = m_hmenuPopup;
			}
		}

		m_hmenuShow = hmenuCustom;
		m_fPopup = true;
		m_PopupMenu = -1;
		::TrackPopupMenu(hmenuCustom, Flags, x, y, 0, hwnd, nullptr);
		m_fPopup = false;
		m_hmenuShow = nullptr;

		if (hmenuCustom != m_hmenuPopup) {
			for (int i = ::GetMenuItemCount(hmenuCustom) - 1; i >= 0; i--) {
				if ((::GetMenuState(hmenuCustom, i, MF_BYPOSITION)&MF_POPUP) != 0)
					::RemoveMenu(hmenuCustom, i, MF_BYPOSITION);
			}
			::DestroyMenu(hmenuCustom);
		}
	} else {
		::EndMenu();
	}

	return true;
}


bool CMainMenu::PopupSubMenu(
	int SubMenu, UINT Flags, HWND hwnd, const POINT *pPos, bool fToggle, const RECT *pExcludeRect)
{
	const HMENU hmenu = GetSubMenu(SubMenu);

	if (hmenu == nullptr)
		return false;
	if (!m_fPopup || !fToggle || m_PopupMenu != SubMenu) {
		if (m_fPopup)
			::EndMenu();
		m_fPopup = true;
		m_PopupMenu = SubMenu;
		MyTrackPopupMenu(hmenu, Flags, hwnd, pPos, pExcludeRect);
		m_fPopup = false;
	} else {
		::EndMenu();
	}
	return true;
}


void CMainMenu::EnableItem(UINT ID, bool fEnable)
{
	if (m_hmenuPopup != nullptr) {
		::EnableMenuItem(m_hmenuPopup, ID, MF_BYCOMMAND | (fEnable ? MF_ENABLED : MF_GRAYED));
		if (m_hmenuShow != nullptr && m_hmenuShow != m_hmenuPopup)
			::EnableMenuItem(m_hmenuShow, ID, MF_BYCOMMAND | (fEnable ? MF_ENABLED : MF_GRAYED));
	}
}


void CMainMenu::CheckItem(UINT ID, bool fCheck)
{
	if (m_hmenuPopup != nullptr) {
		::CheckMenuItem(m_hmenuPopup, ID, MF_BYCOMMAND | (fCheck ? MF_CHECKED : MF_UNCHECKED));
		if (m_hmenuShow != nullptr && m_hmenuShow != m_hmenuPopup)
			::CheckMenuItem(m_hmenuShow, ID, MF_BYCOMMAND | (fCheck ? MF_CHECKED : MF_UNCHECKED));
	}
}


void CMainMenu::CheckRadioItem(UINT FirstID, UINT LastID, UINT CheckID)
{
	if (m_hmenuPopup != nullptr) {
		::CheckMenuRadioItem(m_hmenuPopup, FirstID, LastID, CheckID, MF_BYCOMMAND);
		if (m_hmenuShow != nullptr && m_hmenuShow != m_hmenuPopup)
			::CheckMenuRadioItem(m_hmenuShow, FirstID, LastID, CheckID, MF_BYCOMMAND);
	}
}


bool CMainMenu::IsMainMenu(HMENU hmenu) const
{
	return hmenu != nullptr
		&& hmenu == m_hmenuShow;
}


HMENU CMainMenu::GetSubMenu(int SubMenu) const
{
	if (m_hmenuPopup != nullptr)
		return ::GetSubMenu(m_hmenuPopup, SubMenu);
	return nullptr;
}


bool CMainMenu::SetAccelerator(CAccelerator *pAccelerator)
{
	if (m_hmenu == nullptr)
		return false;
	pAccelerator->SetMenuAccel(m_hmenu);
	return true;
}




void CMenuPainter::Initialize(HWND hwnd, int DPI)
{
	m_hwnd = hwnd;
	if (DPI > 0)
		m_DPI = DPI;
	else if (IsWindowPerMonitorDPIV2(hwnd))
		m_DPI = GetWindowDPI(hwnd);
	else
		m_DPI = 0;
	m_fFlatMenu = false;

	if (m_UxTheme.Initialize() && m_UxTheme.IsActive()
			&& m_UxTheme.Open(hwnd, VSCLASS_MENU, DPI)) {
		// Use theme
	} else {
		BOOL fFlatMenu = FALSE;
		if (::SystemParametersInfo(SPI_GETFLATMENU, 0, &fFlatMenu, 0) && fFlatMenu)
			m_fFlatMenu = true;
	}
}


void CMenuPainter::Finalize()
{
	m_UxTheme.Close();
}


void CMenuPainter::GetFont(LOGFONT *pFont)
{
#if 0
	// OpenThemeDataForDpi を使ってもサイズがスケーリングされない模様
	if (m_UxTheme.IsOpen()
			//&& m_UxTheme.GetFont(MENU_POPUPITEM, 0, TMT_GLYPHFONT, pFont))
			&& m_UxTheme.GetSysFont(TMT_MENUFONT, pFont))
		return;
#endif
	if ((m_DPI > 0) && IsWindowPerMonitorDPIV2(m_hwnd))
		DrawUtil::GetSystemFontWithDPI(DrawUtil::FontType::Menu, pFont, m_DPI);
	else
		DrawUtil::GetSystemFont(DrawUtil::FontType::Menu, pFont);
}


COLORREF CMenuPainter::GetTextColor(UINT State)
{
	COLORREF Color;

	if (m_UxTheme.IsOpen()) {
		m_UxTheme.GetColor(MENU_POPUPITEM, ItemStateToID(State), TMT_TEXTCOLOR, &Color);
	} else {
		Color = ::GetSysColor(
			(State & (ODS_DISABLED | ODS_INACTIVE)) ? COLOR_GRAYTEXT :
			(State & ODS_SELECTED) ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT);
	}
	return Color;
}


void CMenuPainter::DrawItemBackground(HDC hdc, const RECT &Rect, UINT State)
{
	if (m_UxTheme.IsOpen()) {
		m_UxTheme.DrawBackground(
			hdc, MENU_POPUPITEM, ItemStateToID(State), MENU_POPUPBACKGROUND, 0, &Rect);
	} else {
		const bool fSelected = (State & ODS_SELECTED) != 0;
		if (m_fFlatMenu) {
			::FillRect(
				hdc, &Rect,
				reinterpret_cast<HBRUSH>(static_cast<INT_PTR>(fSelected ? COLOR_MENUHILIGHT + 1 : COLOR_MENU + 1)));
			if (fSelected)
				::FrameRect(hdc, &Rect, ::GetSysColorBrush(COLOR_HIGHLIGHT));
		} else {
			::FillRect(
				hdc, &Rect,
				reinterpret_cast<HBRUSH>(static_cast<INT_PTR>(fSelected ? COLOR_HIGHLIGHT + 1 : COLOR_MENU + 1)));
		}
	}
}


void CMenuPainter::GetItemMargins(MARGINS *pMargins)
{
	if (!m_UxTheme.IsOpen()
			|| !m_UxTheme.GetMargins(MENU_POPUPITEM, 0, TMT_CONTENTMARGINS, pMargins)) {
		pMargins->cxLeftWidth = 2;
		pMargins->cxRightWidth = 2;
		pMargins->cyTopHeight = 2;
		pMargins->cyBottomHeight = 2;
	}

	if ((m_DPI > 0) && IsWindowPerMonitorDPIV2(m_hwnd)) {
		const int SystemDPI = GetSystemDPI();
		pMargins->cxLeftWidth = ::MulDiv(pMargins->cxLeftWidth, m_DPI, SystemDPI);
		pMargins->cxRightWidth = ::MulDiv(pMargins->cxRightWidth, m_DPI, SystemDPI);
		pMargins->cyTopHeight = ::MulDiv(pMargins->cyTopHeight, m_DPI, SystemDPI);
		pMargins->cyBottomHeight = ::MulDiv(pMargins->cyBottomHeight, m_DPI, SystemDPI);
	}
}


void CMenuPainter::GetMargins(MARGINS *pMargins)
{
	int BorderSize;

	if (!m_UxTheme.IsOpen()
			|| !m_UxTheme.GetInt(MENU_POPUPBACKGROUND, 0, TMT_BORDERSIZE, &BorderSize))
		BorderSize = 2;

	if ((m_DPI > 0) && IsWindowPerMonitorDPIV2(m_hwnd)) {
		const int SystemDPI = GetSystemDPI();
		BorderSize = ::MulDiv(BorderSize, m_DPI, SystemDPI);
	}

	/*
	SIZE sz;
	GetBorderSize(&sz);
	pMargins->cxLeftWidth = BorderSize+sz.cx;
	pMargins->cxRightWidth = BorderSize+sz.cx;
	pMargins->cyTopHeight = BorderSize+sz.cy;
	pMargins->cyBottomHeight = BorderSize+sz.cy;
	*/
	pMargins->cxLeftWidth = BorderSize;
	pMargins->cxRightWidth = BorderSize;
	pMargins->cyTopHeight = BorderSize;
	pMargins->cyBottomHeight = BorderSize;
}


void CMenuPainter::GetBorderSize(SIZE *pSize)
{
	if (!m_UxTheme.IsOpen()
			|| !m_UxTheme.GetPartSize(nullptr, MENU_POPUPBORDERS, 0, pSize)) {
		pSize->cx = 1;
		pSize->cy = 1;
	}

	if ((m_DPI > 0) && IsWindowPerMonitorDPIV2(m_hwnd)) {
		const int SystemDPI = GetSystemDPI();
		pSize->cx = ::MulDiv(pSize->cx, m_DPI, SystemDPI);
		pSize->cy = ::MulDiv(pSize->cy, m_DPI, SystemDPI);
	}
}


void CMenuPainter::DrawItemText(HDC hdc, UINT State, LPCTSTR pszText, const RECT &Rect, DWORD Flags)
{
	if ((Flags & DT_NOPREFIX) == 0 && (State & ODS_NOACCEL) != 0)
		Flags |= DT_HIDEPREFIX;

	if (m_UxTheme.IsOpen()) {
		m_UxTheme.DrawText(hdc, MENU_POPUPITEM, ItemStateToID(State), pszText, Flags, &Rect);
	} else {
		RECT rc = Rect;
		::DrawText(hdc, pszText, -1, &rc, Flags);
	}
}


bool CMenuPainter::GetItemTextExtent(HDC hdc, UINT State, LPCTSTR pszText, RECT *pExtent, DWORD Flags)
{
	::SetRectEmpty(pExtent);
	if (m_UxTheme.IsOpen())
		return m_UxTheme.GetTextExtent(hdc, MENU_POPUPITEM, ItemStateToID(State), pszText, Flags, pExtent);
	return ::DrawText(hdc, pszText, -1, pExtent, Flags | DT_CALCRECT) != FALSE;
}


void CMenuPainter::DrawIcon(HIMAGELIST himl, int Icon, HDC hdc, int x, int y, UINT State)
{
	if ((State & ODS_DISABLED) == 0) {
		::ImageList_Draw(himl, Icon, hdc, x, y, ILD_TRANSPARENT);
	} else {
		IMAGELISTDRAWPARAMS ildp = {};

		ildp.cbSize = sizeof(ildp);
		ildp.himl = himl;
		ildp.i = Icon;
		ildp.hdcDst = hdc;
		ildp.x = x;
		ildp.y = y;
		ildp.rgbBk = CLR_NONE;
		ildp.fStyle = ILD_TRANSPARENT;
		ildp.fState = ILS_SATURATE;
		::ImageList_DrawIndirect(&ildp);
	}
}


void CMenuPainter::DrawBackground(HDC hdc, const RECT &Rect)
{
	if (m_UxTheme.IsOpen()) {
		m_UxTheme.DrawBackground(hdc, MENU_POPUPBACKGROUND, 0, &Rect);
	} else {
		::FillRect(hdc, &Rect, reinterpret_cast<HBRUSH>(COLOR_MENU + 1));
	}
}


void CMenuPainter::DrawBorder(HDC hdc, const RECT &Rect)
{
	if (m_UxTheme.IsOpen()) {
		m_UxTheme.DrawBackground(hdc, MENU_POPUPBORDERS, 0, MENU_POPUPBACKGROUND, 0, &Rect);
	} else {
		RECT rc = Rect;
		::DrawEdge(hdc, &rc, BDR_RAISEDOUTER, BF_RECT);
	}
}


void CMenuPainter::DrawSeparator(HDC hdc, const RECT &Rect)
{
	RECT rc;

	if (m_UxTheme.IsOpen()) {
		SIZE sz;

		m_UxTheme.GetPartSize(hdc, MENU_POPUPSEPARATOR, 0, &sz);
		rc.left = Rect.left;
		rc.right = Rect.right;
		rc.top = Rect.top + ((Rect.bottom - Rect.top) - sz.cy) / 2;
		rc.bottom = rc.top + sz.cy;
		m_UxTheme.DrawBackground(hdc, MENU_POPUPSEPARATOR, 0, &rc);
	} else {
		rc.left = Rect.left;
		rc.right = Rect.right;
		rc.top = (Rect.top + Rect.bottom) / 2 - 1;
		rc.bottom = rc.top + 2;
		::DrawEdge(hdc, &rc, BDR_SUNKENOUTER, BF_RECT);
	}
}


int CMenuPainter::ItemStateToID(UINT State) const
{
	const bool fDisabled = (State & (ODS_INACTIVE | ODS_DISABLED)) != 0;
	const bool fHot = (State & (ODS_HOTLIGHT | ODS_SELECTED)) != 0;
	int StateID;
	if (fDisabled) {
		StateID = fHot ? MPI_DISABLEDHOT : MPI_DISABLED;
	} else if (fHot) {
		StateID = MPI_HOT;
	} else {
		StateID = MPI_NORMAL;
	}
	return StateID;
}


bool CMenuPainter::IsThemed() const
{
	return m_UxTheme.IsOpen();
}




bool CChannelMenuLogo::Initialize(int IconHeight, InitializeFlag Flags)
{
	if (IconHeight <= 16) {
		m_LogoHeight = 16;
		m_LogoWidth = 26;
	} else {
		m_LogoHeight = IconHeight;
		m_LogoWidth = ::MulDiv(52, m_LogoHeight, 32);
	}

	m_fNoFrame = !!(Flags & InitializeFlag::NoFrame);

	if (!m_fNoFrame) {
		m_FrameBitmap.Load(
			GetAppClass().GetResourceInstance(),
			m_LogoHeight <= 16 ? MAKEINTRESOURCE(IDB_LOGOFRAME16) : MAKEINTRESOURCE(IDB_LOGOFRAME32));
		m_FrameImage.CreateFromBitmap(m_FrameBitmap.GetHandle());
	}

	return true;
}


bool CChannelMenuLogo::DrawLogo(HDC hdc, int x, int y, const CChannelInfo &Channel)
{
	const HBITMAP hbmLogo = GetAppClass().LogoManager.GetAssociatedLogoBitmap(
		Channel.GetNetworkID(), Channel.GetServiceID(),
		m_LogoHeight <= 24 ? CLogoManager::LOGOTYPE_SMALL : CLogoManager::LOGOTYPE_BIG);
	if (hbmLogo == nullptr)
		return false;

	if (m_fNoFrame) {
		DrawUtil::DrawBitmap(hdc, x, y, m_LogoWidth, m_LogoHeight, hbmLogo);
		return true;
	}

	Graphics::CImage LogoImage;

	LogoImage.CreateFromBitmap(hbmLogo);

	Graphics::CCanvas Canvas(hdc);
	const int Margin = m_LogoHeight <= 16 ? 1 : 2;

	Canvas.SetComposition(true);
	if (m_FrameImage.GetHeight() == m_LogoHeight) {
		Canvas.DrawImage(
			x + Margin, y + Margin, m_LogoWidth - Margin * 3, m_LogoHeight - Margin * 3,
			&LogoImage, 0, 0, LogoImage.GetWidth(), LogoImage.GetHeight());
		Canvas.DrawImage(
			x, y, m_LogoWidth, m_LogoHeight,
			&m_FrameImage, 0, 0, m_LogoWidth, m_LogoHeight);
	} else {
		Graphics::CImage TempImage;
		const int DrawWidth = m_FrameImage.GetWidth(), DrawHeight = m_FrameImage.GetHeight();

		TempImage.Create(DrawWidth, DrawHeight, 32);
		{
			Graphics::CCanvas TempCanvas(&TempImage);

			TempCanvas.Clear(0, 0, 0, 0);
			TempCanvas.SetComposition(true);
			TempCanvas.DrawImage(
				Margin, Margin, DrawWidth - Margin * 3, DrawHeight - Margin * 3,
				&LogoImage, 0, 0, LogoImage.GetWidth(), LogoImage.GetHeight());
			TempCanvas.DrawImage(
				0, 0, DrawWidth, DrawHeight,
				&m_FrameImage, 0, 0, DrawWidth, DrawHeight);
		}
		Canvas.DrawImage(
			x, y, m_LogoWidth, m_LogoHeight,
			&TempImage, 0, 0, DrawWidth, DrawHeight);
	}

	return true;
}




class CChannelMenuItem
{
	const CChannelInfo *m_pChannelInfo;
	struct Event
	{
		bool fValid = false;
		LibISDB::EventInfo EventInfo;
	};
	Event m_EventList[2];

public:
	CChannelMenuItem(const CChannelInfo *pChannelInfo)
		: m_pChannelInfo(pChannelInfo) {}
	const LibISDB::EventInfo *GetEventInfo(
		LibISDB::EPGDatabase *pEPGDatabase, int Index, const LibISDB::DateTime *pCurTime = nullptr);
	const LibISDB::EventInfo *GetEventInfo(int Index) const;
	const CChannelInfo *GetChannelInfo() const { return m_pChannelInfo; }
};

const LibISDB::EventInfo *CChannelMenuItem::GetEventInfo(
	LibISDB::EPGDatabase *pEPGDatabase, int Index, const LibISDB::DateTime *pCurTime)
{
	if (Index < 0 || Index >= lengthof(m_EventList)
			|| (Index > 0 && !m_EventList[Index - 1].fValid)
			|| m_pChannelInfo->GetNetworkID() == 0
			|| m_pChannelInfo->GetServiceID() == 0)
		return nullptr;

	if (!m_EventList[Index].fValid) {
		LibISDB::DateTime Time;

		if (Index == 0) {
			if (pCurTime != nullptr)
				Time = *pCurTime;
			else
				LibISDB::GetCurrentEPGTime(&Time);
		} else {
			if (!m_EventList[Index - 1].EventInfo.GetEndTime(&Time))
				return nullptr;
		}

		bool fCurrent = false;
		if (pCurTime == nullptr && Index <= 1) {
			CAppMain &App = GetAppClass();
			CAppCore::StreamIDInfo StreamID;

			if (App.Core.GetCurrentStreamIDInfo(&StreamID)
					&& m_pChannelInfo->GetNetworkID() == StreamID.NetworkID
					&& (m_pChannelInfo->GetTransportStreamID() == 0
						|| m_pChannelInfo->GetTransportStreamID() == StreamID.TransportStreamID)) {
				if (!App.CoreEngine.GetCurrentEventInfo(
							&m_EventList[Index].EventInfo,
							m_pChannelInfo->GetServiceID(),
							Index > 0))
					return nullptr;
				fCurrent = true;
			}
		}

		if (!fCurrent) {
			if (!pEPGDatabase->GetEventInfo(
						m_pChannelInfo->GetNetworkID(),
						m_pChannelInfo->GetTransportStreamID(),
						m_pChannelInfo->GetServiceID(),
						Time, &m_EventList[Index].EventInfo))
				return nullptr;
		}

		m_EventList[Index].fValid = true;
	}

	return &m_EventList[Index].EventInfo;
}

const LibISDB::EventInfo *CChannelMenuItem::GetEventInfo(int Index) const
{
	if (!m_EventList[Index].fValid)
		return nullptr;
	return &m_EventList[Index].EventInfo;
}


CChannelMenu::~CChannelMenu()
{
	Destroy();
}


bool CChannelMenu::Create(
	const CChannelList *pChannelList, int CurChannel,
	UINT Command, UINT LastCommand,
	HMENU hmenu, HWND hwnd, CreateFlag Flags, int MaxRows, int DPI)
{
	Destroy();

	if (pChannelList == nullptr)
		return false;

	if (DPI <= 0) {
		/*
			本来はメニューそのものの DPI が必要だが、WM_MEASUREITEM で DPI を取得できないため
			とりあえずウィンドウの DPI を使っている
		*/
		DPI = GetWindowDPI(hwnd);
	}

	m_ChannelList = *pChannelList;
	m_CurChannel = CurChannel;
	m_FirstCommand = Command;
	m_LastCommand = std::min(Command + m_ChannelList.NumChannels() - 1, LastCommand);
	m_Flags = Flags;
	m_hwnd = hwnd;

	m_MenuPainter.Initialize(hwnd, DPI);
	m_MenuPainter.GetItemMargins(&m_Margins);
	if (m_Margins.cxLeftWidth < 2)
		m_Margins.cxLeftWidth = 2;
	if (m_Margins.cxRightWidth < 2)
		m_Margins.cxRightWidth = 2;

	const HDC hdc = ::GetDC(hwnd);

	CreateFont(hdc);
	const HFONT hfontOld = DrawUtil::SelectObject(hdc, m_Font);

	LibISDB::EPGDatabase &EPGDatabase = GetAppClass().EPGDatabase;
	const bool fCurServices = !!(Flags & CreateFlag::CurrentServices);
	LibISDB::DateTime Time;
	if (!fCurServices)
		GetBaseTime(&Time);

	if (hmenu == nullptr) {
		m_hmenu = ::CreatePopupMenu();
	} else {
		m_hmenu = hmenu;
		m_Flags |= CreateFlag::Shared;
		if (!(Flags & CreateFlag::NoClear))
			ClearMenu(hmenu);
	}

	m_ChannelNameWidth = 0;
	m_EventNameWidth = 0;

	MENUITEMINFO mii;
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_DATA;
	int PrevSpace = -1;

	for (int i = 0, MenuPos = 0, Rows = 0; i < m_ChannelList.NumChannels() && m_FirstCommand + i <= m_LastCommand; i++) {
		const CChannelInfo *pChInfo = m_ChannelList.GetChannelInfo(i);
		if (!pChInfo->IsEnabled() && !(Flags & CreateFlag::IncludeDisabled))
			continue;

		TCHAR szText[256];
		RECT rc;

		if (i == CurChannel)
			DrawUtil::SelectObject(hdc, m_FontCurrent);
		StringFormat(
			szText, TEXT("{}: {}"),
			pChInfo->GetChannelNo(), pChInfo->GetName());
		/*
		m_MenuPainter.GetItemTextExtent(
			hdc, 0, szText, &rc, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
		*/
		::SetRectEmpty(&rc);
		::DrawText(hdc, szText, -1, &rc, DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);
		if (rc.right > m_ChannelNameWidth)
			m_ChannelNameWidth = rc.right;
		mii.wID = m_FirstCommand + i;
		mii.fType = MFT_OWNERDRAW;
		if ((MaxRows > 0 && Rows == MaxRows)
				|| (!!(Flags & CreateFlag::SpaceBreak) && pChInfo->GetSpace() != PrevSpace)) {
			mii.fType |= MFT_MENUBREAK;
			Rows = 0;
		}
		mii.fState = MFS_ENABLED;
		if (i == CurChannel)
			mii.fState |= MFS_CHECKED;
		CChannelMenuItem *pItem = new CChannelMenuItem(pChInfo);
		mii.dwItemData = reinterpret_cast<ULONG_PTR>(pItem);
		if (!!(Flags & CreateFlag::ShowEventInfo)) {
			const LibISDB::EventInfo *pEventInfo =
				pItem->GetEventInfo(&EPGDatabase, 0, fCurServices ? nullptr : &Time);

			if (pEventInfo != nullptr) {
				GetEventText(pEventInfo, szText, lengthof(szText));
				/*
				m_MenuPainter.GetItemTextExtent(
					hdc, 0, szText, &rc, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
				*/
				::SetRectEmpty(&rc);
				::DrawText(hdc, szText, -1, &rc, DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);
				if (rc.right > m_EventNameWidth)
					m_EventNameWidth = rc.right;
			}
		}
		::InsertMenuItem(m_hmenu, MenuPos, TRUE, &mii);
		if (i == CurChannel)
			DrawUtil::SelectObject(hdc, m_Font);
		PrevSpace = pChInfo->GetSpace();
		MenuPos++;
		Rows++;
	}
	::SelectObject(hdc, hfontOld);
	::ReleaseDC(hwnd, hdc);

	if (!!(Flags & CreateFlag::ShowToolTip)) {
		m_Tooltip.Create(hwnd);
		m_Tooltip.SetFont(m_Font.GetHandle());
		m_Tooltip.SetMaxWidth(m_TextHeight * 40);
		m_Tooltip.SetPopDelay(30 * 1000);
		m_Tooltip.AddTrackingTip(1, TEXT(""));
	}

	if (!!(Flags & CreateFlag::ShowLogo)) {
		m_Logo.Initialize(GetSystemMetricsWithDPI(SM_CYSMICON, DPI));
	}

	return true;
}


void CChannelMenu::Destroy()
{
	if (m_hmenu) {
		const bool fDestroy = !(m_Flags & CreateFlag::Shared);
		MENUITEMINFO mii;

		mii.cbSize = sizeof(MENUITEMINFO);
		for (int i = ::GetMenuItemCount(m_hmenu) - 1; i >= 0; i--) {
			mii.fMask = MIIM_ID | MIIM_DATA | MIIM_FTYPE;
			if (::GetMenuItemInfo(m_hmenu, i, TRUE, &mii)) {
				if (mii.wID >= m_FirstCommand && mii.wID <= m_LastCommand) {
					delete reinterpret_cast<CChannelMenuItem*>(mii.dwItemData);
					if (!fDestroy)
						::DeleteMenu(m_hmenu, i, MF_BYPOSITION);
				} else if (!fDestroy
						&& std::ranges::find(m_ExtraItemList, mii.wID) != m_ExtraItemList.end()) {
					mii.fMask = MIIM_FTYPE;
					mii.fType &= ~MFT_OWNERDRAW;
					::SetMenuItemInfo(m_hmenu, i, TRUE, &mii);
				}
			}
		}
		if (fDestroy)
			::DestroyMenu(m_hmenu);
		m_hmenu = nullptr;
	}

	m_ChannelList.Clear();
	m_MenuPainter.Finalize();
	m_Tooltip.Destroy();
	m_hwnd = nullptr;
	m_ExtraItemList.clear();
}


int CChannelMenu::Show(UINT Flags, int x, int y, const RECT *pExcludeRect)
{
	if (m_hmenu == nullptr)
		return false;
	POINT pt = {x, y};
	return MyTrackPopupMenu(m_hmenu, Flags, m_hwnd, &pt, pExcludeRect);
}


bool CChannelMenu::SetHighlightedItem(int Index)
{
	if (m_hmenu == nullptr)
		return false;

	MENUITEMINFO mii;

	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;
	if (!::GetMenuItemInfo(m_hmenu, Index, TRUE, &mii))
		return false;
	mii.fState |= MFS_HILITE;
	return ::SetMenuItemInfo(m_hmenu, Index, TRUE, &mii) != FALSE;
}


bool CChannelMenu::AppendExtraItem(UINT ID, LPCTSTR pszText, UINT Flags)
{
	if (m_hmenu == nullptr || pszText == nullptr)
		return false;
	if (!(Flags & MF_SEPARATOR))
		Flags |= MF_OWNERDRAW;
	if (!::AppendMenu(m_hmenu, Flags, ID, pszText))
		return false;
	if (!(Flags & MF_SEPARATOR))
		m_ExtraItemList.push_back(ID);
	return true;
}


bool CChannelMenu::RegisterExtraItem(UINT ID)
{
	m_ExtraItemList.push_back(ID);

	if (m_hmenu != nullptr) {
		MENUITEMINFO mii;
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_FTYPE;
		if (::GetMenuItemInfo(m_hmenu, ID, FALSE, &mii)) {
			mii.fType |= MFT_OWNERDRAW;
			::SetMenuItemInfo(m_hmenu, ID, FALSE, &mii);
		}
	}

	return true;
}


bool CChannelMenu::OnMeasureItem(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	LPMEASUREITEMSTRUCT pmis = reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);

	if (m_hmenu == nullptr || pmis->CtlType != ODT_MENU)
		return false;

	if (pmis->itemID >= m_FirstCommand && pmis->itemID <= m_LastCommand) {
		pmis->itemWidth =
			m_ChannelNameWidth + m_Margins.cxLeftWidth + m_Margins.cxRightWidth;
		if (!!(m_Flags & CreateFlag::ShowLogo))
			pmis->itemWidth += m_Logo.GetLogoWidth() + m_MenuLogoMargin;
		if (m_EventNameWidth > 0)
			pmis->itemWidth += m_TextHeight + m_EventNameWidth;
		pmis->itemHeight =
			std::max(m_TextHeight, m_Logo.GetLogoHeight()) +
			m_Margins.cyTopHeight + m_Margins.cyBottomHeight;
		return true;
	}

	if (std::ranges::find(m_ExtraItemList, pmis->itemID) != m_ExtraItemList.end()) {
		TCHAR szText[256];
		const int Length = ::GetMenuString(m_hmenu, pmis->itemID, szText, lengthof(szText), MF_BYCOMMAND);
		if (Length > 0) {
			HDC hdc = ::GetDC(hwnd);
			RECT rc = {};
			::DrawText(hdc, szText, Length, &rc, DT_SINGLELINE | DT_CALCRECT);
			pmis->itemWidth = rc.right + m_Margins.cxLeftWidth + m_Margins.cxRightWidth;
			if (!!(m_Flags & CreateFlag::ShowLogo))
				pmis->itemWidth += m_Logo.GetLogoWidth() + m_MenuLogoMargin;
			pmis->itemHeight = m_TextHeight + m_Margins.cyTopHeight + m_Margins.cyBottomHeight;
			::ReleaseDC(hwnd, hdc);
			return true;
		}
	}

	return false;
}


bool CChannelMenu::OnDrawItem(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	LPDRAWITEMSTRUCT pdis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

	if (m_hmenu == nullptr || hwnd != m_hwnd || pdis->CtlType != ODT_MENU
			|| ((pdis->itemID < m_FirstCommand || pdis->itemID > m_LastCommand)
				&& std::ranges::find(m_ExtraItemList, pdis->itemID) == m_ExtraItemList.end()))
		return false;

	m_MenuPainter.DrawItemBackground(pdis->hDC, pdis->rcItem, pdis->itemState);
	const COLORREF crTextColor = m_MenuPainter.GetTextColor(pdis->itemState);

	const HFONT hfontOld = DrawUtil::SelectObject(
		pdis->hDC, (pdis->itemState & ODS_CHECKED) == 0 ? m_Font : m_FontCurrent);
	const int OldBkMode = ::SetBkMode(pdis->hDC, TRANSPARENT);
	const COLORREF crOldTextColor = ::SetTextColor(pdis->hDC, crTextColor);

	RECT rc;
	rc.left = pdis->rcItem.left + m_Margins.cxLeftWidth;
	rc.top = pdis->rcItem.top + m_Margins.cyTopHeight;
	rc.bottom = pdis->rcItem.bottom - m_Margins.cyBottomHeight;

	if (pdis->itemID >= m_FirstCommand && pdis->itemID <= m_LastCommand) {
		const CChannelMenuItem *pItem = reinterpret_cast<CChannelMenuItem*>(pdis->itemData);
		const CChannelInfo *pChInfo = pItem->GetChannelInfo();
		TCHAR szText[256];

		if (!!(m_Flags & CreateFlag::ShowLogo)) {
			m_Logo.DrawLogo(
				pdis->hDC,
				rc.left,
				rc.top + ((rc.bottom - rc.top) - m_Logo.GetLogoHeight()) / 2,
				*pChInfo);
			rc.left += m_Logo.GetLogoWidth() + m_MenuLogoMargin;
		}

		rc.right = rc.left + m_ChannelNameWidth;
		StringFormat(
			szText, TEXT("{}: {}"),
			pChInfo->GetChannelNo(), pChInfo->GetName());
		::DrawText(
			pdis->hDC, szText, -1, &rc,
			DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

		if (!!(m_Flags & CreateFlag::ShowEventInfo)) {
			const LibISDB::EventInfo *pEventInfo = pItem->GetEventInfo(0);
			if (pEventInfo != nullptr) {
				const int Length = GetEventText(pEventInfo, szText, lengthof(szText));
				rc.left = rc.right + m_TextHeight;
				rc.right = pdis->rcItem.right - m_Margins.cxRightWidth;
				::DrawText(
					pdis->hDC, szText, Length, &rc,
					DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
			}
		}
	} else {
		TCHAR szText[256];
		const int Length = ::GetMenuString(m_hmenu, pdis->itemID, szText, lengthof(szText), MF_BYCOMMAND);
		if (Length > 0) {
			if (!!(m_Flags & CreateFlag::ShowLogo))
				rc.left += m_Logo.GetLogoWidth() + m_MenuLogoMargin;
			rc.right = pdis->rcItem.right - m_Margins.cxRightWidth;
			::DrawText(pdis->hDC, szText, Length, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
		}
	}

	::SetTextColor(pdis->hDC, crOldTextColor);
	::SetBkMode(pdis->hDC, OldBkMode);
	::SelectObject(pdis->hDC, hfontOld);

	return true;
}


bool CChannelMenu::OnMenuSelect(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	const HMENU hmenu = reinterpret_cast<HMENU>(lParam);
	const UINT Command = LOWORD(wParam);

	if (hmenu == nullptr || hmenu != m_hmenu || hwnd != m_hwnd || HIWORD(wParam) == 0xFFFF
			|| Command < m_FirstCommand || Command > m_LastCommand) {
		if (m_Tooltip.IsVisible())
			m_Tooltip.TrackActivate(1, false);
		return false;
	}

	if (!!(m_Flags & CreateFlag::ShowToolTip)) {
		MENUITEMINFO mii;

		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_DATA;
		if (::GetMenuItemInfo(hmenu, Command, FALSE, &mii)) {
			CChannelMenuItem *pItem = reinterpret_cast<CChannelMenuItem*>(mii.dwItemData);
			if (pItem == nullptr)
				return false;

			const LibISDB::EventInfo *pEventInfo1, *pEventInfo2;
			pEventInfo1 = pItem->GetEventInfo(0);
			if (pEventInfo1 == nullptr) {
				pEventInfo1 = pItem->GetEventInfo(&GetAppClass().EPGDatabase, 0);
			}
			if (pEventInfo1 != nullptr) {
				TCHAR szText[256 * 2 + 1];
				int Length = GetEventText(pEventInfo1, szText, lengthof(szText) / 2);
				pEventInfo2 = pItem->GetEventInfo(&GetAppClass().EPGDatabase, 1);
				if (pEventInfo2 != nullptr) {
					szText[Length++] = _T('\r');
					szText[Length++] = _T('\n');
					GetEventText(pEventInfo2, szText + Length, lengthof(szText) / 2);
				}
				m_Tooltip.SetText(1, szText);

				POINT pt;
				::GetCursorPos(&pt);
				pt.x += 16;
				pt.y +=
					std::max(m_TextHeight, m_Logo.GetLogoHeight()) +
					m_Margins.cyTopHeight + m_Margins.cyBottomHeight;
				m_Tooltip.TrackPosition(pt.x, pt.y);
				m_Tooltip.TrackActivate(1, true);
			} else {
				m_Tooltip.TrackActivate(1, false);
			}
		}
	}
	return true;
}


bool CChannelMenu::OnUninitMenuPopup(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	if (hwnd == m_hwnd && reinterpret_cast<HMENU>(wParam) == m_hmenu) {
		Destroy();
		return true;
	}
	return false;
}


bool CChannelMenu::HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
	if (m_hwnd == nullptr || hwnd != m_hwnd)
		return false;

	switch (uMsg) {
	case WM_MEASUREITEM:
		if (OnMeasureItem(hwnd, wParam, lParam)) {
			*pResult = TRUE;
			return true;
		}
		break;

	case WM_DRAWITEM:
		if (OnDrawItem(hwnd, wParam, lParam)) {
			*pResult = TRUE;
			return true;
		}
		break;

	case WM_UNINITMENUPOPUP:
		if (OnUninitMenuPopup(hwnd, wParam, lParam)) {
			*pResult = 0;
			return true;
		}
		break;

	case WM_MENUSELECT:
		if (OnMenuSelect(hwnd, wParam, lParam)) {
			*pResult = 0;
			return true;
		}
		break;
	}

	return false;
}


int CChannelMenu::GetEventText(
	const LibISDB::EventInfo *pEventInfo, LPTSTR pszText, int MaxLength) const
{
	TCHAR szTime[EpgUtil::MAX_EVENT_TIME_LENGTH];

	EpgUtil::FormatEventTime(
		*pEventInfo, szTime, lengthof(szTime), EpgUtil::FormatEventTimeFlag::Hour2Digits);

	return static_cast<int>(StringFormat(
		pszText, MaxLength, TEXT("{} {}"), szTime, pEventInfo->EventName));
}


void CChannelMenu::CreateFont(HDC hdc)
{
	LOGFONT lf;
	m_MenuPainter.GetFont(&lf);
	m_Font.Create(&lf);
	lf.lfWeight = FW_BOLD;
	m_FontCurrent.Create(&lf);

	if (hdc != nullptr)
		m_TextHeight = m_Font.GetHeight(hdc);
	else
		m_TextHeight = std::abs(lf.lfHeight);
}


void CChannelMenu::GetBaseTime(LibISDB::DateTime *pTime)
{
	LibISDB::GetCurrentEPGTime(pTime);
	pTime->OffsetMinutes(2);
}




CPopupMenu::CPopupMenu(HINSTANCE hinst, LPCTSTR pszName)
{
	Load(hinst, pszName);
}


CPopupMenu::CPopupMenu(HINSTANCE hinst, UINT ID)
{
	Load(hinst, ID);
}


CPopupMenu::CPopupMenu(HMENU hmenu, bool fOwn)
{
	Attach(hmenu, fOwn);
}


CPopupMenu::~CPopupMenu()
{
	Destroy();
}


bool CPopupMenu::Create()
{
	Destroy();

	m_hmenu = ::CreatePopupMenu();
	if (m_hmenu == nullptr)
		return false;
	m_Type = PopupMenuType::Created;
	return true;
}


bool CPopupMenu::Load(HINSTANCE hinst, LPCTSTR pszName)
{
	Destroy();

	m_hmenu = ::LoadMenu(hinst, pszName);
	if (m_hmenu == nullptr)
		return false;
	m_Type = PopupMenuType::Resource;
	return true;
}


bool CPopupMenu::Attach(HMENU hmenu, bool fOwn)
{
	if (hmenu == nullptr)
		return false;
	Destroy();
	m_hmenu = hmenu;
	m_Type = fOwn ? PopupMenuType::Created : PopupMenuType::Shared;
	return true;
}


void CPopupMenu::Destroy()
{
	if (m_hmenu != nullptr) {
		if (m_Type != PopupMenuType::Shared)
			::DestroyMenu(m_hmenu);
		m_hmenu = nullptr;
	}
}


int CPopupMenu::GetItemCount() const
{
	if (m_hmenu == nullptr)
		return 0;
	return ::GetMenuItemCount(GetPopupHandle());
}


void CPopupMenu::Clear()
{
	if (m_hmenu != nullptr)
		ClearMenu(GetPopupHandle());
}


HMENU CPopupMenu::GetPopupHandle() const
{
	if (m_hmenu == nullptr)
		return nullptr;
	if (m_Type == PopupMenuType::Resource)
		return ::GetSubMenu(m_hmenu, 0);
	return m_hmenu;
}


bool CPopupMenu::Append(UINT ID, LPCTSTR pszText, UINT Flags)
{
	if (m_hmenu == nullptr || pszText == nullptr)
		return false;
	return ::AppendMenu(GetPopupHandle(), MF_STRING | Flags, ID, pszText) != FALSE;
}


bool CPopupMenu::AppendUnformatted(UINT ID, LPCTSTR pszText, UINT Flags)
{
	if (m_hmenu == nullptr || pszText == nullptr)
		return false;
	TCHAR szText[256];
	CopyToMenuText(pszText, szText, lengthof(szText));
	return Append(ID, szText);
}


bool CPopupMenu::Append(HMENU hmenu, LPCTSTR pszText, UINT Flags)
{
	if (m_hmenu == nullptr || hmenu == nullptr || pszText == nullptr)
		return false;
	return ::AppendMenu(
		GetPopupHandle(), MF_POPUP | Flags,
		reinterpret_cast<UINT_PTR>(hmenu), pszText) != FALSE;
}


bool CPopupMenu::AppendSeparator()
{
	if (m_hmenu == nullptr)
		return false;
	return ::AppendMenu(GetPopupHandle(), MF_SEPARATOR, 0, nullptr) != FALSE;
}


bool CPopupMenu::EnableItem(UINT ID, bool fEnable)
{
	if (m_hmenu == nullptr)
		return false;
	return ::EnableMenuItem(m_hmenu, ID, MF_BYCOMMAND | (fEnable ? MF_ENABLED : MF_GRAYED)) >= 0;
}


bool CPopupMenu::EnableSubMenu(UINT Pos, bool fEnable)
{
	if (m_hmenu == nullptr)
		return false;
	return ::EnableMenuItem(GetPopupHandle(), Pos, MF_BYPOSITION | (fEnable ? MF_ENABLED : MF_GRAYED)) >= 0;
}


bool CPopupMenu::CheckItem(UINT ID, bool fCheck)
{
	if (m_hmenu == nullptr)
		return false;
	return ::CheckMenuItem(m_hmenu, ID, MF_BYCOMMAND | (fCheck ? MF_CHECKED : MF_UNCHECKED)) != static_cast<DWORD>(-1);
}


bool CPopupMenu::CheckRadioItem(UINT FirstID, UINT LastID, UINT CheckID, UINT Flags)
{
	if (m_hmenu == nullptr)
		return false;
	return ::CheckMenuRadioItem(GetPopupHandle(), FirstID, LastID, CheckID, Flags) != FALSE;
}


HMENU CPopupMenu::GetSubMenu(int Pos) const
{
	if (m_hmenu == nullptr)
		return nullptr;
	return ::GetSubMenu(GetPopupHandle(), Pos);
}


int CPopupMenu::Show(HWND hwnd, const POINT *pPos, UINT Flags, const RECT *pExcludeRect)
{
	if (m_hmenu == nullptr)
		return 0;

	return MyTrackPopupMenu(GetPopupHandle(), Flags, hwnd, pPos, pExcludeRect);
}




CIconMenu::~CIconMenu()
{
	Finalize();
}


bool CIconMenu::Initialize(HMENU hmenu, HINSTANCE hinst, const ItemInfo *pItemList, int ItemCount)
{
	Finalize();

	if (hmenu == nullptr || pItemList == nullptr || ItemCount < 1)
		return false;

	for (int i = 0; i < ItemCount; i++) {
		const HICON hicon = LoadIconStandardSize(hinst, pItemList[i].pszIcon, IconSizeType::Small);

		if (hicon != nullptr) {
			ICONINFO ii;

			if (::GetIconInfo(hicon, &ii)) {
				/*
					ii.hbmColor は DDB になっていて、そのままメニューの画像に指定すると
					アルファチャンネルが無視されるため、DIB に変換する
				*/
				const HBITMAP hbm = static_cast<HBITMAP>(::CopyImage(ii.hbmColor, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
				if (hbm != nullptr) {
					m_BitmapList.push_back(hbm);
					ItemIconInfo Item;
					Item.ID = pItemList[i].ID;
					Item.Icon = static_cast<int>(m_BitmapList.size()) - 1;
					m_ItemList.push_back(Item);
				}
				::DeleteObject(ii.hbmColor);
				::DeleteObject(ii.hbmMask);
			}
			::DestroyIcon(hicon);
		}
	}

	m_hmenu = hmenu;

	return true;
}


void CIconMenu::Finalize()
{
	m_hmenu = nullptr;
	m_ItemList.clear();
	if (!m_BitmapList.empty()) {
		for (HBITMAP hbm : m_BitmapList)
			::DeleteObject(hbm);
		m_BitmapList.clear();
	}
}


bool CIconMenu::OnInitMenuPopup(HWND hwnd, HMENU hmenu)
{
	if (hmenu != m_hmenu)
		return false;

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	const int Count = ::GetMenuItemCount(hmenu);

	for (int i = 0; i < Count; i++) {
		mii.fMask = MIIM_ID | MIIM_STATE | MIIM_DATA;
		if (::GetMenuItemInfo(hmenu, i, TRUE, &mii)) {
			for (const ItemIconInfo &Item : m_ItemList) {
				if (Item.ID == mii.wID) {
					mii.fMask = MIIM_STATE | MIIM_BITMAP | MIIM_DATA;
					mii.dwItemData = (mii.dwItemData & ~ITEM_DATA_IMAGEMASK) | (Item.Icon + 1);
					mii.hbmpItem = m_BitmapList[Item.Icon];
					if ((mii.dwItemData & ITEM_DATA_CHECKED) != 0) {
						mii.fState |= MFS_CHECKED;
					}
					::SetMenuItemInfo(hmenu, i, TRUE, &mii);
				}
			}
		}
	}

	return true;
}


bool CIconMenu::OnMeasureItem(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	return false;
}


bool CIconMenu::OnDrawItem(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	return false;
}


bool CIconMenu::CheckItem(UINT ID, bool fCheck)
{
	if (m_hmenu == nullptr)
		return false;

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE | MIIM_DATA;
	if (!::GetMenuItemInfo(m_hmenu, ID, FALSE, &mii))
		return false;
	if (fCheck) {
		mii.fState |= MFS_CHECKED;
		mii.dwItemData |= ITEM_DATA_CHECKED;
	} else {
		mii.fState &= ~MFS_CHECKED;
		mii.dwItemData &= ~ITEM_DATA_CHECKED;
	}
	::SetMenuItemInfo(m_hmenu, ID, FALSE, &mii);
	return true;
}


bool CIconMenu::CheckRadioItem(UINT FirstID, UINT LastID, UINT CheckID)
{
	if (m_hmenu == nullptr)
		return false;

	const int ItemCount = ::GetMenuItemCount(m_hmenu);

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);

	for (int i = 0; i < ItemCount; i++) {
		mii.fMask = MIIM_ID | MIIM_STATE | MIIM_DATA;
		if (::GetMenuItemInfo(m_hmenu, i, TRUE, &mii)
				&& mii.wID >= FirstID && mii.wID <= LastID) {
			mii.fMask = MIIM_STATE | MIIM_DATA;
			if (mii.wID == CheckID) {
				mii.fState |= MFS_CHECKED;
				mii.dwItemData |= ITEM_DATA_CHECKED;
			} else {
				mii.fState &= ~MFS_CHECKED;
				mii.dwItemData &= ~ITEM_DATA_CHECKED;
			}
			::SetMenuItemInfo(m_hmenu, i, TRUE, &mii);
		}
	}

	return true;
}




const LPCTSTR DROPDOWNMENU_WINDOW_CLASS = APP_NAME TEXT(" Drop Down Menu");


HINSTANCE CDropDownMenu::m_hinst = nullptr;


bool CDropDownMenu::Initialize(HINSTANCE hinst)
{
	if (m_hinst == nullptr) {
		WNDCLASS wc;

		wc.style = CS_DROPSHADOW;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hinst;
		wc.hIcon = nullptr;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = nullptr;
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = DROPDOWNMENU_WINDOW_CLASS;
		if (::RegisterClass(&wc) == 0)
			return false;
		m_hinst = hinst;
	}
	return true;
}


void CDropDownMenu::Clear()
{
	m_ItemList.clear();
}


bool CDropDownMenu::AppendItem(CItem *pItem)
{
	if (pItem == nullptr)
		return false;
	m_ItemList.emplace_back(pItem);
	return true;
}


bool CDropDownMenu::InsertItem(int Index, CItem *pItem)
{
	if (pItem == nullptr || Index < 0 || static_cast<size_t>(Index) > m_ItemList.size())
		return false;
	auto it = m_ItemList.begin();
	std::advance(it, Index);
	m_ItemList.emplace(it, pItem);
	return true;
}


bool CDropDownMenu::AppendSeparator()
{
	CItem *pItem = new CItem(-1, nullptr);

	if (!AppendItem(pItem)) {
		delete pItem;
		return false;
	}
	return true;
}


bool CDropDownMenu::InsertSeparator(int Index)
{
	CItem *pItem = new CItem(-1, nullptr);

	if (!InsertItem(Index, pItem)) {
		delete pItem;
		return false;
	}
	return true;
}


bool CDropDownMenu::DeleteItem(int Command)
{
	const int Index = CommandToIndex(Command);

	if (Index < 0)
		return false;
	auto it = m_ItemList.begin();
	std::advance(it, Index);
	m_ItemList.erase(it);
	return true;
}


bool CDropDownMenu::SetItemText(int Command, LPCTSTR pszText)
{
	const int Index = CommandToIndex(Command);

	if (Index < 0)
		return false;
	m_ItemList[Index]->SetText(pszText);
	return true;
}


int CDropDownMenu::CommandToIndex(int Command) const
{
	if (Command < 0)
		return -1;
	for (size_t i = 0; i < m_ItemList.size(); i++) {
		if (m_ItemList[i]->GetCommand() == Command)
			return static_cast<int>(i);
	}
	return -1;
}


bool CDropDownMenu::Show(HWND hwndOwner, HWND hwndMessage, const POINT *pPos, int CurItem, UINT Flags, int DPI)
{
	if (m_ItemList.empty() || m_hwnd != nullptr)
		return false;

	const HMONITOR hMonitor = ::MonitorFromPoint(*pPos, MONITOR_DEFAULTTONEAREST);

	if (DPI != 0)
		m_DPI = DPI;
	else
		m_DPI = GetMonitorDPI(hMonitor);

	const HWND hwnd = ::CreateWindowEx(
		WS_EX_NOACTIVATE | WS_EX_TOPMOST, DROPDOWNMENU_WINDOW_CLASS,
		nullptr, WS_POPUP, 0, 0, 0, 0, hwndOwner, nullptr, m_hinst, this);
	if (hwnd == nullptr)
		return false;

	m_HotItem = CommandToIndex(CurItem);
	m_hwndMessage = hwndMessage;

	const HDC hdc = ::GetDC(hwnd);
	const HFONT hfontOld = DrawUtil::SelectObject(hdc, m_Font);
	int MaxWidth = 0;
	for (const auto &Item : m_ItemList) {
		const int Width = Item->GetWidth(hdc);
		if (Width > MaxWidth)
			MaxWidth = Width;
	}
	::SelectObject(hdc, hfontOld);
	m_ItemWidth = MaxWidth + m_ItemMargin.cxLeftWidth + m_ItemMargin.cxRightWidth;
	m_ItemHeight =
		m_Font.GetHeight(hdc) +
		m_ItemMargin.cyTopHeight + m_ItemMargin.cyBottomHeight;
	::ReleaseDC(hwnd, hdc);

	const int HorzMargin = m_WindowMargin.cxLeftWidth + m_WindowMargin.cxRightWidth;
	const int VertMargin = m_WindowMargin.cyTopHeight + m_WindowMargin.cyBottomHeight;
	m_MaxRows = static_cast<int>(m_ItemList.size());
	int Columns = 1;
	int x = pPos->x, y = pPos->y;
	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	if (::GetMonitorInfo(hMonitor, &mi)) {
		const int Rows = ((mi.rcMonitor.bottom - y) - VertMargin) / m_ItemHeight;

		if (static_cast<int>(m_ItemList.size()) > Rows) {
			const int MaxColumns = ((mi.rcMonitor.right - mi.rcMonitor.left) - HorzMargin) / m_ItemWidth;
			if (MaxColumns > 1) {
				if (Rows * MaxColumns >= static_cast<int>(m_ItemList.size())) {
					Columns = (static_cast<int>(m_ItemList.size()) + Rows - 1) / Rows;
					m_MaxRows = Rows;
				} else {
					Columns = MaxColumns;
					m_MaxRows = (static_cast<int>(m_ItemList.size()) + Columns - 1) / Columns;
					if ((Columns - 1) * m_MaxRows >= static_cast<int>(m_ItemList.size()))
						Columns--;
				}
			}
		}
		const int Width = Columns * m_ItemWidth + HorzMargin;
		const int Height = m_MaxRows * m_ItemHeight + VertMargin;
		if (x + Width > mi.rcMonitor.right)
			x = std::max(mi.rcMonitor.right - Width, 0L);
		if (y + Height > mi.rcMonitor.bottom)
			y = std::max(mi.rcMonitor.bottom - Height, 0L);
	}

	::MoveWindow(
		hwnd, x, y,
		m_ItemWidth * Columns + HorzMargin,
		m_ItemHeight * m_MaxRows + VertMargin,
		FALSE);
	::ShowWindow(hwnd, SW_SHOWNA);

	return true;
}


bool CDropDownMenu::Hide()
{
	if (m_hwnd != nullptr)
		::DestroyWindow(m_hwnd);
	return true;
}


bool CDropDownMenu::GetPosition(RECT *pRect)
{
	if (m_hwnd == nullptr)
		return false;
	return ::GetWindowRect(m_hwnd, pRect) != FALSE;
}


bool CDropDownMenu::GetItemRect(int Index, RECT *pRect) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_ItemList.size())
		return false;
	pRect->left = (Index / m_MaxRows) * m_ItemWidth;
	pRect->top = (Index % m_MaxRows) * m_ItemHeight;
	pRect->right = pRect->left + m_ItemWidth;
	pRect->bottom = pRect->top + m_ItemHeight;
	::OffsetRect(pRect, m_WindowMargin.cxLeftWidth, m_WindowMargin.cyTopHeight);
	return true;
}


int CDropDownMenu::HitTest(int x, int y) const
{
	const POINT pt = {x, y};

	for (int i = 0; i < static_cast<int>(m_ItemList.size()); i++) {
		RECT rc;

		GetItemRect(i, &rc);
		if (::PtInRect(&rc, pt)) {
			if (m_ItemList[i]->IsSeparator())
				return -1;
			return i;
		}
	}
	return -1;
}


void CDropDownMenu::UpdateItem(int Index) const
{
	if (m_hwnd != nullptr) {
		RECT rc;
		if (GetItemRect(Index, &rc))
			::InvalidateRect(m_hwnd, &rc, TRUE);
	}
}


void CDropDownMenu::Draw(HDC hdc, const RECT *pPaintRect)
{
	const HFONT hfontOld = DrawUtil::SelectObject(hdc, m_Font);
	const int OldBkMode = ::SetBkMode(hdc, TRANSPARENT);
	const COLORREF OldTextColor = ::GetTextColor(hdc);
	RECT rc;

	::GetClientRect(m_hwnd, &rc);
	m_MenuPainter.DrawBackground(hdc, rc);
	m_MenuPainter.DrawBorder(hdc, rc);

	for (int i = 0; i < static_cast<int>(m_ItemList.size()); i++) {
		GetItemRect(i, &rc);
		if (rc.bottom > pPaintRect->top && rc.top < pPaintRect->bottom) {
			CItem *pItem = m_ItemList[i].get();

			if (pItem->IsSeparator()) {
				m_MenuPainter.DrawSeparator(hdc, rc);
			} else {
				const bool fHighlight = i == m_HotItem;
				const UINT State = fHighlight ? ODS_SELECTED : 0;

				m_MenuPainter.DrawItemBackground(hdc, rc, State);
				::SetTextColor(hdc, m_MenuPainter.GetTextColor(State));
				rc.left += m_ItemMargin.cxLeftWidth;
				rc.right -= m_ItemMargin.cxRightWidth;
				pItem->Draw(hdc, &rc);
			}
		}
	}
	::SetTextColor(hdc, OldTextColor);
	::SetBkMode(hdc, OldBkMode);
	::SelectObject(hdc, hfontOld);
}


CDropDownMenu *CDropDownMenu::GetThis(HWND hwnd)
{
	return reinterpret_cast<CDropDownMenu*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
}


LRESULT CALLBACK CDropDownMenu::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CDropDownMenu *pThis = static_cast<CDropDownMenu*>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);

			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));

			pThis->m_hwnd = hwnd;
			pThis->m_fTrackMouseEvent = false;

			if (pThis->m_fDarkMode)
				::SetWindowTheme(hwnd, L"DarkMode", nullptr);

			pThis->m_MenuPainter.Initialize(hwnd, GetWindowDPI(hwnd));
			pThis->m_MenuPainter.GetItemMargins(&pThis->m_ItemMargin);
			if (pThis->m_ItemMargin.cxLeftWidth < 4)
				pThis->m_ItemMargin.cxLeftWidth = 4;
			if (pThis->m_ItemMargin.cxRightWidth < 4)
				pThis->m_ItemMargin.cxRightWidth = 4;
			pThis->m_MenuPainter.GetMargins(&pThis->m_WindowMargin);

			const int SystemDPI = GetAppClass().StyleManager.GetSystemDPI();
			CUxTheme::ScaleMargins(&pThis->m_ItemMargin, pThis->m_DPI, SystemDPI);
			CUxTheme::ScaleMargins(&pThis->m_WindowMargin, pThis->m_DPI, SystemDPI);

			LOGFONT lf;
			pThis->m_MenuPainter.GetFont(&lf);
			lf.lfHeight = ::MulDiv(lf.lfHeight, pThis->m_DPI, SystemDPI);
			pThis->m_Font.Create(&lf);
		}
		return 0;

	case WM_PAINT:
		{
			CDropDownMenu *pThis = GetThis(hwnd);
			PAINTSTRUCT ps;

			::BeginPaint(hwnd, &ps);
			pThis->Draw(ps.hdc, &ps.rcPaint);
			::EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			CDropDownMenu *pThis = GetThis(hwnd);
			const int Item = pThis->HitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

			if (Item != pThis->m_HotItem) {
				const int OldHotItem = pThis->m_HotItem;

				pThis->m_HotItem = Item;
				if (OldHotItem >= 0)
					pThis->UpdateItem(OldHotItem);
				if (Item >= 0)
					pThis->UpdateItem(Item);
			}
			if (!pThis->m_fTrackMouseEvent) {
				TRACKMOUSEEVENT tme;

				tme.cbSize = sizeof(tme);
				tme.dwFlags = TME_LEAVE;
				tme.hwndTrack = hwnd;
				if (::TrackMouseEvent(&tme))
					pThis->m_fTrackMouseEvent = true;
			}
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			CDropDownMenu *pThis = GetThis(hwnd);

			if (pThis->m_HotItem >= 0) {
				::DestroyWindow(hwnd);
				::SendMessage(
					pThis->m_hwndMessage, WM_COMMAND,
					pThis->m_ItemList[pThis->m_HotItem]->GetCommand(), 0);
			}
		}
		return 0;

	case WM_MOUSELEAVE:
		::DestroyWindow(hwnd);
		return 0;

	case WM_MOUSEACTIVATE:
		return MA_NOACTIVATE;

	case WM_DESTROY:
		{
			CDropDownMenu *pThis = GetThis(hwnd);

			pThis->m_Font.Destroy();
			pThis->m_MenuPainter.Finalize();
			pThis->m_hwnd = nullptr;
		}
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}




CDropDownMenu::CItem::CItem(int Command, LPCTSTR pszText)
	: m_Command(Command)
	, m_Text(StringFromCStr(pszText))
{
}


void CDropDownMenu::CItem::SetText(LPCTSTR pszText)
{
	StringUtility::Assign(m_Text, pszText);
}


int CDropDownMenu::CItem::GetWidth(HDC hdc)
{
	if (!m_Text.empty() && m_Width == 0) {
		SIZE sz;

		::GetTextExtentPoint32(hdc, m_Text.data(), static_cast<int>(m_Text.length()), &sz);
		m_Width = sz.cx;
	}
	return m_Width;
}


void CDropDownMenu::CItem::Draw(HDC hdc, const RECT *pRect)
{
	if (!m_Text.empty()) {
		RECT rc = *pRect;

		::DrawText(
			hdc, m_Text.data(), static_cast<int>(m_Text.length()), &rc,
			DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
	}
}


}	// namespace TVTest
