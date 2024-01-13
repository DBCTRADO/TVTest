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
#include "ChannelDisplay.h"
#include "EpgUtil.h"
#include "AppMain.h"
#include "Settings.h"
#include "DarkMode.h"
#include "Common/DebugDef.h"


namespace TVTest
{


const LPCTSTR CChannelDisplay::m_pszWindowClass = APP_NAME TEXT(" Channel Display");
HINSTANCE CChannelDisplay::m_hinst = nullptr;


bool CChannelDisplay::Initialize(HINSTANCE hinst)
{
	if (m_hinst == nullptr) {
		WNDCLASS wc;

		wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
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


CChannelDisplay::CChannelDisplay(LibISDB::EPGDatabase *pEPGDatabase)
	: m_pEPGDatabase(pEPGDatabase)
{
	GetBackgroundStyle(BackgroundType::Categories, &m_TunerAreaBackStyle);
	GetBackgroundStyle(BackgroundType::Content, &m_ChannelAreaBackStyle);
	GetItemStyle(ItemType::Normal, &m_TunerItemStyle);
	GetItemStyle(ItemType::Selected, &m_TunerItemSelStyle);
	GetItemStyle(ItemType::Current, &m_TunerItemCurStyle);
	GetItemStyle(ItemType::Normal1, &m_ChannelItemStyle[0]);
	GetItemStyle(ItemType::Normal2, &m_ChannelItemStyle[1]);
	GetItemStyle(ItemType::Hot, &m_ChannelItemCurStyle);

	m_ClockStyle.Back.Fill.Type = Theme::FillType::Solid;
	m_ClockStyle.Back.Fill.Solid.Color.Set(16, 16, 16);
	m_ClockStyle.Back.Border.Type = Theme::BorderType::None;
	m_ClockStyle.Fore.Fill.Type = Theme::FillType::Solid;
	m_ClockStyle.Fore.Fill.Solid.Color.Set(255, 255, 255);

	GetDefaultFont(&m_StyleFont);
}


CChannelDisplay::~CChannelDisplay()
{
	Destroy();
	Clear();
}


bool CChannelDisplay::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	return CreateBasicWindow(hwndParent, Style, ExStyle, ID, m_pszWindowClass, nullptr, m_hinst);
}


void CChannelDisplay::SetStyle(const Style::CStyleManager *pStyleManager)
{
	CDisplayView::SetStyle(pStyleManager);
	m_ChannelDisplayStyle.SetStyle(pStyleManager);
}


void CChannelDisplay::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	CDisplayView::NormalizeStyle(pStyleManager, pStyleScaling);
	m_ChannelDisplayStyle.NormalizeStyle(pStyleManager, pStyleScaling);
}


bool CChannelDisplay::Close()
{
	if (m_pChannelDisplayEventHandler != nullptr) {
		m_pChannelDisplayEventHandler->OnClose();
		return true;
	}
	return false;
}


bool CChannelDisplay::IsMessageNeed(const MSG *pMsg) const
{
	if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_KEYUP) {
		switch (pMsg->wParam) {
		case VK_LEFT:
		case VK_RIGHT:
		case VK_UP:
		case VK_DOWN:
		case VK_RETURN:
		case VK_SPACE:
		case VK_PRIOR:
		case VK_NEXT:
		case VK_HOME:
		case VK_END:
		case VK_TAB:
		case VK_ESCAPE:
			return true;
		}
	}
	return false;
}


bool CChannelDisplay::OnMouseWheel(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (Msg == WM_MOUSEWHEEL && m_hwnd != nullptr) {
		const int Delta = m_MouseWheel.OnMouseWheel(wParam, 1);

		if (Delta != 0) {
			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			::ScreenToClient(m_hwnd, &pt);

			if (TunerItemHitTest(pt.x, pt.y) >= 0) {
				SetTunerScrollPos(m_TunerScrollPos - Delta, true);
			} else if (ChannelItemHitTest(pt.x, pt.y) >= 0) {
				SetChannelScrollPos(m_ChannelScrollPos - Delta, true);
			} else {
				RECT rc;

				::ClientToScreen(m_hwnd, &pt);
				::GetWindowRect(m_hwndTunerScroll, &rc);
				if (::PtInRect(&rc, pt)) {
					SetTunerScrollPos(m_TunerScrollPos - Delta, true);
				} else {
					::GetWindowRect(m_hwndChannelScroll, &rc);
					if (::PtInRect(&rc, pt)) {
						SetChannelScrollPos(m_ChannelScrollPos - Delta, true);
					}
				}
			}
		}
		return true;
	}

	return false;
}


void CChannelDisplay::Clear()
{
	m_TunerList.clear();
	m_TotalTuningSpaces = 0;
	m_CurTuner = -1;
	m_CurChannel = -1;
}


bool CChannelDisplay::SetDriverManager(CDriverManager *pDriverManager)
{
	Clear();
	LoadSettings();
	for (int i = 0; i < pDriverManager->NumDrivers(); i++) {
		CDriverInfo *pDriverInfo = pDriverManager->GetDriverInfo(i);
		const TunerInfo *pTunerInfo = nullptr;

		LPCTSTR pszFileName = ::PathFindFileName(pDriverInfo->GetFileName());
		for (const TunerInfo &Info : m_TunerInfoList) {
			LPCTSTR p = Info.DriverMasks;
			while (*p != '\0') {
				if (::PathMatchSpec(pszFileName, p)) {
					pTunerInfo = &Info;
					goto End;
				}
				p += ::lstrlen(p) + 1;
			}
		}
End:
		const bool fUseDriverChannel = pTunerInfo != nullptr && pTunerInfo->fUseDriverChannel;
		pDriverInfo->LoadTuningSpaceList(
			fUseDriverChannel ?
				CDriverInfo::LoadTuningSpaceListMode::Default :
				CDriverInfo::LoadTuningSpaceListMode::NoLoadDriver);
		CTuner *pTuner = new CTuner(pDriverInfo);
		if (pTunerInfo != nullptr) {
			if (pTunerInfo->szDisplayName[0] != '\0')
				pTuner->SetDisplayName(pTunerInfo->szDisplayName);
			if (pTunerInfo->szIconFile[0] != '\0') {
				const HICON hico = ::ExtractIcon(
					GetAppClass().GetInstance(),
					pTunerInfo->szIconFile, pTunerInfo->Index);
				if (hico != nullptr && hico != reinterpret_cast<HICON>(1))
					pTuner->SetIcon(hico);
			}
		}
		m_TunerList.emplace_back(pTuner);
		m_TotalTuningSpaces += pTuner->NumSpaces();
	}
	if (m_hwnd != nullptr) {
		Layout();
		Invalidate();
	}
	return true;
}


void CChannelDisplay::SetLogoManager(CLogoManager *pLogoManager)
{
	m_pLogoManager = pLogoManager;
}


void CChannelDisplay::SetEventHandler(CChannelDisplayEventHandler *pEventHandler)
{
	if (m_pChannelDisplayEventHandler != nullptr)
		m_pChannelDisplayEventHandler->m_pChannelDisplay = nullptr;
	if (pEventHandler != nullptr)
		pEventHandler->m_pChannelDisplay = this;
	m_pChannelDisplayEventHandler = pEventHandler;
	CDisplayView::SetEventHandler(pEventHandler);
}


bool CChannelDisplay::SetSelect(LPCTSTR pszDriverFileName, const CChannelInfo *pChannelInfo)
{
	int TunerIndex = 0;
	for (const auto &Tuner : m_TunerList) {
		if (IsEqualFileName(Tuner->GetDriverFileName(), pszDriverFileName)) {
			int Space = 0, Channel = -1;
			if (pChannelInfo != nullptr) {
				for (int j = 0; j < Tuner->NumSpaces(); j++) {
					const CChannelList *pChannelList = Tuner->GetTuningSpaceInfo(j)->GetChannelList();
					if (pChannelList != nullptr) {
						Channel = pChannelList->FindByIndex(
							pChannelInfo->GetSpace(),
							pChannelInfo->GetChannelIndex(),
							pChannelInfo->GetServiceID());
						if (Channel >= 0) {
							Space = j;
							break;
						}
					}
				}
			}
			SetCurTuner(TunerIndex + Space, true);
			if (Channel >= 0)
				SetCurChannel(Channel);
			return true;
		}
		TunerIndex += Tuner->NumSpaces();
	}
	return false;
}


bool CChannelDisplay::SetFont(const Style::Font &Font, bool fAutoSize)
{
	m_StyleFont = Font;
	m_fAutoFontSize = fAutoSize;
	if (m_hwnd != nullptr) {
		ApplyStyle();
		RealizeStyle();
	}
	return true;
}


void CChannelDisplay::LoadSettings()
{
	TCHAR szIniFileName[MAX_PATH];
	CSettings Settings;

	GetAppClass().GetAppDirectory(szIniFileName);
	::PathAppend(szIniFileName, TEXT("Tuner.ini"));
	if (Settings.Open(szIniFileName, CSettings::OpenFlag::Read)
			&& Settings.SetSection(TEXT("TunerSettings"))) {
		m_TunerInfoList.clear();
		for (int i = 0;; i++) {
			TCHAR szName[64];
			TunerInfo Info;

			StringFormat(szName, TEXT("Tuner{}_Driver"), i);
			if (!Settings.Read(szName, Info.DriverMasks, lengthof(Info.DriverMasks) - 1))
				break;
			LPTSTR p = Info.DriverMasks;
			while (*p != '\0') {
				if (*p == '|')
					*p = '\0';
				p++;
			}
			*(p + 1) = '\0';
			StringFormat(szName, TEXT("Tuner{}_Name"), i);
			if (!Settings.Read(szName, Info.szDisplayName, lengthof(Info.szDisplayName)))
				Info.szDisplayName[0] = '\0';
			StringFormat(szName, TEXT("Tuner{}_Icon"), i);
			if (!Settings.Read(szName, Info.szIconFile, lengthof(Info.szIconFile)))
				Info.szIconFile[0] = '\0';
			if (::PathIsRelative(Info.szIconFile)) {
				TCHAR szPath[MAX_PATH];

				GetAppClass().GetAppDirectory(szPath);
				::PathAppend(szPath, Info.szIconFile);
				::PathCanonicalize(Info.szIconFile, szPath);
			}
			Info.Index = 0;
			p = Info.szIconFile;
			while (*p != '\0') {
				if (*p == '|') {
					*p = '\0';
					Info.Index = ::StrToInt(p + 1);
					break;
				}
				p++;
			}
			StringFormat(szName, TEXT("Tuner{}_UseDriverChannel"), i);
			if (!Settings.Read(szName, &Info.fUseDriverChannel))
				Info.fUseDriverChannel = false;
			m_TunerInfoList.push_back(Info);
		}
	}
}


void CChannelDisplay::Layout()
{
	RECT rc;
	GetClientRect(&rc);

	if (m_fAutoFontSize) {
		LOGFONT lf = m_StyleFont.LogFont;
		lf.lfHeight = -GetDefaultFontSize(rc.right, rc.bottom);
		lf.lfWidth = 0;
		m_Font.Create(&lf);
	}

	const HDC hdc = ::GetDC(m_hwnd);
	const HFONT hfontOld = SelectFont(hdc, m_Font.GetHandle());
	m_FontHeight = m_Font.GetHeight(hdc);

	int TunerNameWidth = 0;
	bool fTunerIcon = false;
	for (const auto &Tuner : m_TunerList) {
		for (int j = 0; j < Tuner->NumSpaces(); j++) {
			TCHAR szText[256];
			Tuner->GetDisplayName(j, szText, lengthof(szText));
			RECT rc = {};
			::DrawText(hdc, szText, -1, &rc, DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);
			if (TunerNameWidth < rc.right)
				TunerNameWidth = rc.right;
			if (Tuner->GetIcon() != nullptr)
				fTunerIcon = true;
		}
	}

	m_TunerItemWidth = TunerNameWidth + m_ChannelDisplayStyle.TunerItemPadding.Horz();
	if (fTunerIcon)
		m_TunerItemWidth += m_ChannelDisplayStyle.TunerIconSize.Width + m_ChannelDisplayStyle.TunerIconTextMargin;
	m_TunerItemHeight =
		std::max(m_ChannelDisplayStyle.TunerIconSize.Height.Value, m_FontHeight) +
		m_ChannelDisplayStyle.TunerItemPadding.Vert();
	const int CategoriesHeight = rc.bottom - m_Style.CategoriesMargin.Vert();
	m_VisibleTunerItems = CategoriesHeight / m_TunerItemHeight;
	if (m_VisibleTunerItems < 1)
		m_VisibleTunerItems = 1;
	else if (m_VisibleTunerItems > m_TotalTuningSpaces)
		m_VisibleTunerItems = m_TotalTuningSpaces;
	m_TunerItemLeft = m_Style.CategoriesMargin.Left;
	m_TunerItemTop =
		m_Style.CategoriesMargin.Top +
		std::max((CategoriesHeight - m_VisibleTunerItems * m_TunerItemHeight) / 2, 0);
	m_TunerAreaWidth = m_TunerItemLeft + m_TunerItemWidth + m_Style.CategoriesMargin.Right;
	const int ScrollWidth = m_pStyleScaling->GetScaledSystemMetrics(SM_CXVSCROLL);
	if (m_TotalTuningSpaces > m_VisibleTunerItems) {
		SCROLLINFO si;

		if (m_TunerScrollPos > m_TotalTuningSpaces - m_VisibleTunerItems)
			m_TunerScrollPos = m_TotalTuningSpaces - m_VisibleTunerItems;
		si.cbSize = sizeof(si);
		si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
		si.nMin = 0;
		si.nMax = m_TotalTuningSpaces - 1;
		si.nPage = m_VisibleTunerItems;
		si.nPos = m_TunerScrollPos;
		::SetScrollInfo(m_hwndTunerScroll, SB_CTL, &si, TRUE);
		::MoveWindow(
			m_hwndTunerScroll,
			m_TunerItemLeft + m_TunerItemWidth, m_TunerItemTop,
			ScrollWidth, m_VisibleTunerItems * m_TunerItemHeight, TRUE);
		::ShowWindow(m_hwndTunerScroll, SW_SHOW);
		m_TunerAreaWidth += ScrollWidth;
	} else {
		m_TunerScrollPos = 0;
		::ShowWindow(m_hwndTunerScroll, SW_HIDE);
	}

	m_ChannelNameWidth = 0;
	int NumChannels = 0;
	if (m_CurTuner >= 0) {
		const CTuningSpaceInfo *pTuningSpace = GetTuningSpaceInfo(m_CurTuner);
		if (pTuningSpace != nullptr) {
			NumChannels = pTuningSpace->NumChannels();
			if (NumChannels > 0) {
				const CChannelList *pChannelList = pTuningSpace->GetChannelList();
				for (int i = 0; i < NumChannels; i++) {
					LPCTSTR pszName = pChannelList->GetName(i);
					SIZE sz;
					if (::GetTextExtentPoint32(hdc, pszName, ::lstrlen(pszName), &sz)
							&& sz.cx > m_ChannelNameWidth)
						m_ChannelNameWidth = sz.cx;
				}
				if (m_ChannelNameWidth > m_FontHeight * 12)
					m_ChannelNameWidth = m_FontHeight * 12;
			}
		}
	}
	m_ChannelItemLeft = m_TunerAreaWidth + m_Style.ContentMargin.Left;
	m_ChannelItemWidth = std::max(
		static_cast<int>(rc.right) - m_ChannelItemLeft - m_Style.ContentMargin.Right,
		m_ChannelNameWidth + m_ChannelDisplayStyle.ChannelItemPadding.Horz() +
			m_ChannelDisplayStyle.ChannelEventMargin + m_FontHeight * 8);
	m_ChannelItemHeight = m_FontHeight * 2 + m_ChannelDisplayStyle.ChannelItemPadding.Vert();
	int ContentTop =
		m_FontHeight +
		m_ChannelDisplayStyle.ClockPadding.Vert() +
		m_ChannelDisplayStyle.ClockMargin.Vert();
	if (ContentTop < m_Style.ContentMargin.Top)
		ContentTop = m_Style.ContentMargin.Top;
	const int ContentHeight = rc.bottom - m_Style.ContentMargin.Bottom - ContentTop;
	m_VisibleChannelItems = std::max(ContentHeight / m_ChannelItemHeight, 1);
	if (m_VisibleChannelItems > NumChannels)
		m_VisibleChannelItems = NumChannels;
	m_ChannelItemTop = ContentTop + std::max((ContentHeight - m_VisibleChannelItems * m_ChannelItemHeight) / 2, 0);
	if (NumChannels > m_VisibleChannelItems) {
		SCROLLINFO si;

		if (m_ChannelScrollPos > NumChannels - m_VisibleChannelItems)
			m_ChannelScrollPos = NumChannels - m_VisibleChannelItems;
		si.cbSize = sizeof(si);
		si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
		si.nMin = 0;
		si.nMax = NumChannels - 1;
		si.nPage = m_VisibleChannelItems;
		si.nPos = m_ChannelScrollPos;
		::SetScrollInfo(m_hwndChannelScroll, SB_CTL, &si, TRUE);
		m_ChannelItemWidth -= ScrollWidth;
		::MoveWindow(
			m_hwndChannelScroll,
			m_ChannelItemLeft + m_ChannelItemWidth, m_ChannelItemTop,
			ScrollWidth, m_VisibleChannelItems * m_ChannelItemHeight, TRUE);
		::ShowWindow(m_hwndChannelScroll, SW_SHOW);
	} else {
		m_ChannelScrollPos = 0;
		::ShowWindow(m_hwndChannelScroll, SW_HIDE);
	}

	::SelectObject(hdc, hfontOld);
	::ReleaseDC(m_hwnd, hdc);
}


const CTuningSpaceInfo *CChannelDisplay::GetTuningSpaceInfo(int Index) const
{
	int TunerIndex = 0;
	for (const auto &Tuner : m_TunerList) {
		if (Index >= TunerIndex && Index < TunerIndex + Tuner->NumSpaces())
			return Tuner->GetTuningSpaceInfo(Index - TunerIndex);
		TunerIndex += Tuner->NumSpaces();
	}
	return nullptr;
}


CTuningSpaceInfo *CChannelDisplay::GetTuningSpaceInfo(int Index)
{
	int TunerIndex = 0;
	for (const auto &Tuner : m_TunerList) {
		if (Index >= TunerIndex && Index < TunerIndex + Tuner->NumSpaces())
			return Tuner->GetTuningSpaceInfo(Index - TunerIndex);
		TunerIndex += Tuner->NumSpaces();
	}
	return nullptr;
}


const CChannelDisplay::CTuner *CChannelDisplay::GetTuner(int Index, int *pSpace) const
{
	int TunerIndex = 0;
	for (const auto &Tuner : m_TunerList) {
		if (Index >= TunerIndex && Index < TunerIndex + Tuner->NumSpaces()) {
			if (pSpace != nullptr)
				*pSpace = Index - TunerIndex;
			return Tuner.get();
		}
		TunerIndex += Tuner->NumSpaces();
	}
	return nullptr;
}


void CChannelDisplay::GetTunerItemRect(int Index, RECT *pRect) const
{
	pRect->left = m_TunerItemLeft;
	pRect->top = m_TunerItemTop + (Index - m_TunerScrollPos) * m_TunerItemHeight;
	pRect->right = pRect->left + m_TunerItemWidth;
	pRect->bottom = pRect->top + m_TunerItemHeight;
}


void CChannelDisplay::GetChannelItemRect(int Index, RECT *pRect) const
{
	pRect->left = m_ChannelItemLeft;
	pRect->top = m_ChannelItemTop + (Index - m_ChannelScrollPos) * m_ChannelItemHeight;
	pRect->right = pRect->left + m_ChannelItemWidth;
	pRect->bottom = pRect->top + m_ChannelItemHeight;
}


void CChannelDisplay::UpdateTunerItem(int Index) const
{
	if (m_hwnd != nullptr) {
		RECT rc;
		GetTunerItemRect(Index, &rc);
		::InvalidateRect(m_hwnd, &rc, TRUE);
	}
}


void CChannelDisplay::UpdateChannelItem(int Index) const
{
	if (m_hwnd != nullptr) {
		RECT rc;
		GetChannelItemRect(Index, &rc);
		::InvalidateRect(m_hwnd, &rc, TRUE);
	}
}


int CChannelDisplay::TunerItemHitTest(int x, int y) const
{
	if (x >= m_TunerItemLeft && x < m_TunerItemLeft + m_TunerItemWidth
			&& y >= m_TunerItemTop && y < m_TunerItemTop + m_VisibleTunerItems * m_TunerItemHeight) {
		const int Index = (y - m_TunerItemTop) / m_TunerItemHeight + m_TunerScrollPos;
		if (Index < m_TotalTuningSpaces)
			return Index;
	}
	return -1;
}


int CChannelDisplay::ChannelItemHitTest(int x, int y) const
{
	if (m_CurTuner >= 0
			&& x >= m_ChannelItemLeft && x < m_ChannelItemLeft + m_ChannelItemWidth
			&& y >= m_ChannelItemTop && y < m_ChannelItemTop + m_VisibleChannelItems * m_ChannelItemHeight) {
		const int Index = (y - m_ChannelItemTop) / m_ChannelItemHeight + m_ChannelScrollPos;
		const CTuningSpaceInfo *pTuningSpace = GetTuningSpaceInfo(m_CurTuner);

		if (pTuningSpace != nullptr && Index < pTuningSpace->NumChannels())
			return Index;
	}
	return -1;
}


bool CChannelDisplay::SetCurTuner(int Index, bool fUpdate)
{
	if (Index < -1 || Index >= m_TotalTuningSpaces)
		return false;
	if (Index != m_CurTuner || fUpdate) {
		if (Index != m_CurTuner) {
			m_CurTuner = Index;
			if (Index >= 0) {
				if (Index < m_TunerScrollPos)
					SetTunerScrollPos(Index, false);
				else if (Index >= m_TunerScrollPos + m_VisibleTunerItems)
					SetTunerScrollPos(Index - (m_VisibleTunerItems - 1), false);
			}
		}
		m_CurChannel = -1;
		m_ChannelScrollPos = 0;
		LibISDB::GetCurrentEPGTime(&m_EpgBaseTime);
		m_EpgBaseTime.Second = 0;
		m_EpgBaseTime.Millisecond = 0;
		UpdateChannelInfo(Index);
		Layout();
		Invalidate();
	}
	return true;
}


bool CChannelDisplay::UpdateChannelInfo(int Index)
{
	CTuningSpaceInfo *pTuningSpace = GetTuningSpaceInfo(Index);

	if (pTuningSpace == nullptr)
		return false;
	CChannelList *pChannelList = pTuningSpace->GetChannelList();
	for (int i = 0; i < pChannelList->NumChannels(); i++) {
		CTuner::CChannel *pChannel = static_cast<CTuner::CChannel*>(pChannelList->GetChannelInfo(i));

		LibISDB::EventInfo EventInfo;
		if (m_pEPGDatabase->GetEventInfo(
					pChannel->GetNetworkID(),
					pChannel->GetTransportStreamID(),
					pChannel->GetServiceID(),
					m_EpgBaseTime, &EventInfo)) {
			pChannel->SetEvent(0, &EventInfo);
			LibISDB::DateTime EndTime;
			if (EventInfo.StartTime.IsValid()
					&& EventInfo.Duration > 0
					&& EventInfo.GetEndTime(&EndTime)
					&& m_pEPGDatabase->GetEventInfo(
						pChannel->GetNetworkID(),
						pChannel->GetTransportStreamID(),
						pChannel->GetServiceID(),
						EndTime, &EventInfo)) {
				pChannel->SetEvent(1, &EventInfo);
			} else {
				pChannel->SetEvent(1, nullptr);
			}
		} else {
			pChannel->SetEvent(0, nullptr);
			if (m_pEPGDatabase->GetNextEventInfo(
						pChannel->GetNetworkID(),
						pChannel->GetTransportStreamID(),
						pChannel->GetServiceID(),
						m_EpgBaseTime, &EventInfo)
					&& EventInfo.StartTime.DiffSeconds(m_EpgBaseTime) < 8 * 60 * 60) {
				pChannel->SetEvent(1, &EventInfo);
			} else {
				pChannel->SetEvent(1, nullptr);
			}
		}
		if (m_pLogoManager != nullptr) {
			HBITMAP hbmLogo = m_pLogoManager->GetAssociatedLogoBitmap(
				pChannel->GetNetworkID(), pChannel->GetServiceID(), CLogoManager::LOGOTYPE_SMALL);
			if (hbmLogo != nullptr)
				pChannel->SetSmallLogo(hbmLogo);
			hbmLogo = m_pLogoManager->GetAssociatedLogoBitmap(
				pChannel->GetNetworkID(), pChannel->GetServiceID(), CLogoManager::LOGOTYPE_BIG);
			if (hbmLogo != nullptr)
				pChannel->SetBigLogo(hbmLogo);
		}
	}
	return true;
}


bool CChannelDisplay::SetCurChannel(int Index)
{
	const CTuningSpaceInfo *pTuningSpace = GetTuningSpaceInfo(m_CurTuner);

	if (pTuningSpace == nullptr || Index < -1 || Index >= pTuningSpace->NumChannels())
		return false;
	if (Index != m_CurChannel) {
		if (m_CurChannel >= 0)
			UpdateChannelItem(m_CurChannel);
		if ((m_CurChannel >= 0) != (Index >= 0))
			UpdateTunerItem(m_CurTuner);
		m_CurChannel = Index;
		if (Index >= 0) {
			UpdateChannelItem(Index);

			int ScrollPos = m_ChannelScrollPos;
			if (Index < ScrollPos)
				ScrollPos = Index;
			else if (Index >= ScrollPos + m_VisibleChannelItems)
				ScrollPos = Index - (m_VisibleChannelItems - 1);
			if (ScrollPos != m_ChannelScrollPos) {
				Update();
				SetChannelScrollPos(ScrollPos, true);
			}
		}
	}
	return true;
}


void CChannelDisplay::SetTunerScrollPos(int Pos, bool fScroll)
{
	if (Pos < 0)
		Pos = 0;
	else if (Pos > m_TotalTuningSpaces - m_VisibleTunerItems)
		Pos = m_TotalTuningSpaces - m_VisibleTunerItems;
	if (Pos != m_TunerScrollPos) {
		SCROLLINFO si;

		si.cbSize = sizeof(si);
		si.fMask = SIF_POS;
		si.nPos = Pos;
		::SetScrollInfo(m_hwndTunerScroll, SB_CTL, &si, TRUE);
		if (fScroll) {
			RECT rc;
			::SetRect(
				&rc, m_TunerItemLeft, m_TunerItemTop,
				m_TunerItemLeft + m_TunerItemWidth,
				m_TunerItemTop + m_TunerItemHeight * m_VisibleTunerItems);
			::ScrollWindowEx(
				m_hwnd, 0, (m_TunerScrollPos - Pos) * m_TunerItemHeight,
				&rc, &rc, nullptr, nullptr, SW_INVALIDATE);
		}
		m_TunerScrollPos = Pos;
	}
}


void CChannelDisplay::SetChannelScrollPos(int Pos, bool fScroll)
{
	int NumChannels = 0;
	if (m_CurTuner >= 0) {
		const CTuningSpaceInfo *pTuningSpace = GetTuningSpaceInfo(m_CurTuner);
		if (pTuningSpace != nullptr)
			NumChannels = pTuningSpace->NumChannels();
	}
	if (Pos < 0)
		Pos = 0;
	else if (Pos > NumChannels - m_VisibleChannelItems)
		Pos = NumChannels - m_VisibleChannelItems;
	if (Pos != m_ChannelScrollPos) {
		SCROLLINFO si;

		si.cbSize = sizeof(si);
		si.fMask = SIF_POS;
		si.nPos = Pos;
		::SetScrollInfo(m_hwndChannelScroll, SB_CTL, &si, TRUE);
		if (fScroll) {
			RECT rc;
			::SetRect(
				&rc, m_ChannelItemLeft, m_ChannelItemTop,
				m_ChannelItemLeft + m_ChannelItemWidth,
				m_ChannelItemTop + m_ChannelItemHeight * m_VisibleChannelItems);
			::ScrollWindowEx(
				m_hwnd, 0, (m_ChannelScrollPos - Pos) * m_ChannelItemHeight,
				&rc, &rc, nullptr, nullptr, SW_INVALIDATE);
		}
		m_ChannelScrollPos = Pos;
	}
}


void CChannelDisplay::Draw(HDC hdc, const RECT *pPaintRect)
{
	RECT rcClient, rc;
	TCHAR szText[1024];

	::GetClientRect(m_hwnd, &rcClient);
	const HFONT hfontOld = SelectFont(hdc, m_Font.GetHandle());
	const COLORREF OldTextColor = ::GetTextColor(hdc);
	const int OldBkMode = ::SetBkMode(hdc, TRANSPARENT);

	if (pPaintRect->left < m_TunerAreaWidth) {
		rc.left = rcClient.left;
		rc.top = rcClient.top;
		rc.right = m_TunerAreaWidth;
		rc.bottom = rcClient.bottom;
		Theme::Draw(hdc, rc, m_TunerAreaBackStyle);

		bool fTunerIcon = false;
		for (auto it = m_TunerList.begin(); it != m_TunerList.end(); ++it) {
			if ((*it)->GetIcon() != nullptr) {
				fTunerIcon = true;
				break;
			}
		}

		int TunerIndex = 0;
		for (const auto &Tuner : m_TunerList) {
			for (int j = 0; j < Tuner->NumSpaces(); j++) {
				if (TunerIndex >= m_TunerScrollPos
						&& TunerIndex < m_TunerScrollPos + m_VisibleTunerItems) {
					const Theme::Style *pStyle;
					if (TunerIndex == m_CurTuner) {
						pStyle = m_CurChannel >= 0 ? &m_TunerItemSelStyle : &m_TunerItemCurStyle;
					} else {
						pStyle = &m_TunerItemStyle;
					}
					GetTunerItemRect(TunerIndex, &rc);
					Theme::Draw(hdc, rc, pStyle->Back);
					Style::Subtract(&rc, m_ChannelDisplayStyle.TunerItemPadding);
					if (Tuner->GetIcon() != nullptr) {
						::DrawIconEx(
							hdc,
							rc.left,
							rc.top + ((rc.bottom - rc.top) - m_ChannelDisplayStyle.TunerIconSize.Height) / 2,
							Tuner->GetIcon(),
							m_ChannelDisplayStyle.TunerIconSize.Width,
							m_ChannelDisplayStyle.TunerIconSize.Height,
							0, nullptr, DI_NORMAL);
					}
					if (fTunerIcon)
						rc.left += m_ChannelDisplayStyle.TunerIconSize.Width + m_ChannelDisplayStyle.TunerIconTextMargin;
					Tuner->GetDisplayName(j, szText, lengthof(szText));
					Theme::Draw(
						hdc, rc, pStyle->Fore, szText,
						DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
				}
				TunerIndex++;
			}
		}
	}

	if (pPaintRect->right > m_TunerAreaWidth) {
		rc.left = m_TunerAreaWidth;
		rc.top = rcClient.top;
		rc.right = rcClient.right;
		rc.bottom = rcClient.bottom;
		Theme::Draw(hdc, rc, m_ChannelAreaBackStyle);

		DrawClock(hdc);

		if (m_CurTuner >= 0) {
			const CTuningSpaceInfo *pTuningSpace = GetTuningSpaceInfo(m_CurTuner);

			if (pTuningSpace != nullptr) {
				const CChannelList *pChannelList = pTuningSpace->GetChannelList();
				for (int i = m_ChannelScrollPos; i < pChannelList->NumChannels() && i < m_ChannelScrollPos + m_VisibleChannelItems; i++) {
					const CTuner::CChannel *pChannel = static_cast<const CTuner::CChannel*>(pChannelList->GetChannelInfo(i));
					const Theme::Style *pStyle;
					if (i == m_CurChannel) {
						pStyle = &m_ChannelItemCurStyle;
					} else {
						pStyle = &m_ChannelItemStyle[i % 2];
					}
					RECT rcItem;
					GetChannelItemRect(i, &rcItem);
					Theme::Draw(hdc, rcItem, pStyle->Back);
					Style::Subtract(&rcItem, m_ChannelDisplayStyle.ChannelItemPadding);
					rc = rcItem;
					rc.right = rc.left + m_ChannelNameWidth;
					if (pChannel->HasLogo()) {
						const int LogoHeight = std::min(m_FontHeight - 4, 36);
						const int LogoWidth = LogoHeight * 16 / 9;
						const HBITMAP hbmLogo =
							LogoHeight <= 14 || pChannel->GetBigLogo() == nullptr ?
								pChannel->GetSmallLogo() : pChannel->GetBigLogo();
						DrawUtil::DrawBitmap(
							hdc, rc.left, rc.top + (m_FontHeight - LogoHeight) / 2,
							LogoWidth, LogoHeight, hbmLogo, nullptr, 224);
						rc.top += m_FontHeight;
					}
					Theme::Draw(
						hdc, rc, pStyle->Fore, pChannel->GetName(),
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
					rc = rcItem;
					rc.left += m_ChannelNameWidth + m_ChannelDisplayStyle.ChannelEventMargin;
					rc.bottom = (rc.top + rc.bottom) / 2;
					for (int j = 0; j < 2; j++) {
						const LibISDB::EventInfo *pEventInfo = pChannel->GetEvent(j);
						if (pEventInfo != nullptr) {
							size_t Length = EpgUtil::FormatEventTime(
								*pEventInfo, szText, lengthof(szText),
								EpgUtil::FormatEventTimeFlag::Hour2Digits | EpgUtil::FormatEventTimeFlag::StartOnly);
							if (!pEventInfo->EventName.empty()) {
								Length += StringFormat(
									szText + Length, lengthof(szText) - Length,
									TEXT("{}{}"),
									Length > 0 ? TEXT(" ") : TEXT(""),
									pEventInfo->EventName);
							}
							if (Length > 0) {
								Theme::Draw(
									hdc, rc, pStyle->Fore, szText,
									DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
							}
						}
						rc.top = rc.bottom;
						rc.bottom = rcItem.bottom;
					}
				}
			}
		}
	}

	::SelectObject(hdc, hfontOld);
	::SetTextColor(hdc, OldTextColor);
	::SetBkMode(hdc, OldBkMode);

	DrawCloseButton(hdc);
}


void CChannelDisplay::DrawClock(HDC hdc) const
{
	const HFONT hfontOld = SelectFont(hdc, m_Font.GetHandle());
	SIZE sz;
	RECT rc;
	TCHAR szText[32];

	GetTextExtentPoint32(hdc, TEXT("88:88"), 5, &sz);
	rc.left = m_ChannelItemLeft + m_ChannelDisplayStyle.ClockMargin.Left;
	rc.top = m_ChannelDisplayStyle.ClockMargin.Top;
	rc.right = rc.left + sz.cx + m_ChannelDisplayStyle.ClockPadding.Horz();
	rc.bottom = rc.top + m_FontHeight + m_ChannelDisplayStyle.ClockPadding.Vert();
	Theme::Draw(hdc, rc, m_ClockStyle.Back);
	Style::Subtract(&rc, m_ChannelDisplayStyle.ClockPadding);
	const int OldBkMode = SetBkMode(hdc, TRANSPARENT);
	StringFormat(szText, TEXT("{}:{:02}"), m_ClockTime.Hour, m_ClockTime.Minute);
	Theme::Draw(
		hdc, rc, m_ClockStyle.Fore, szText,
		DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	SetBkMode(hdc, OldBkMode);
	SelectObject(hdc, hfontOld);
}


void CChannelDisplay::NotifyTunerSelect() const
{
	int Space;
	const CTuner *pTuner = GetTuner(m_CurTuner, &Space);
	if (pTuner == nullptr)
		return;
	const CTuningSpaceInfo *pTuningSpace = pTuner->GetTuningSpaceInfo(Space);

	if (pTuningSpace != nullptr && pTuningSpace->NumChannels() > 0) {
		const CChannelList *pChannelList = pTuningSpace->GetChannelList();

		for (int i = 0; i < pChannelList->NumChannels(); i++) {
			const int ChannelSpace = pChannelList->GetSpace(i);
			if (i == 0) {
				Space = ChannelSpace;
			} else {
				if (Space != ChannelSpace) {
					Space = CChannelDisplayEventHandler::SPACE_ALL;
					break;
				}
			}
		}
	} else {
		Space = CChannelDisplayEventHandler::SPACE_NOTSPECIFIED;
	}
	m_pChannelDisplayEventHandler->OnTunerSelect(pTuner->GetDriverFileName(), Space);
}


void CChannelDisplay::NotifyChannelSelect() const
{
	const CTuner *pTuner = GetTuner(m_CurTuner);
	if (pTuner == nullptr)
		return;
	const CTuningSpaceInfo *pTuningSpace = GetTuningSpaceInfo(m_CurTuner);

	m_pChannelDisplayEventHandler->OnChannelSelect(
		pTuner->GetDriverFileName(),
		pTuningSpace->GetChannelList()->GetChannelInfo(m_CurChannel));
}


LRESULT CChannelDisplay::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			InitializeUI();

			m_hwndTunerScroll = ::CreateWindowEx(
				0, TEXT("SCROLLBAR"), TEXT(""),
				WS_CHILD | SBS_VERT, 0, 0, 0, 0, hwnd, nullptr, m_hinst, nullptr);
			m_hwndChannelScroll = ::CreateWindowEx(
				0, TEXT("SCROLLBAR"), TEXT(""),
				WS_CHILD | SBS_VERT, 0, 0, 0, 0, hwnd, nullptr, m_hinst, nullptr);

			if (IsDarkThemeSupported()) {
				SetWindowDarkTheme(m_hwndTunerScroll, IsDarkThemeStyle(m_TunerAreaBackStyle));
				SetWindowDarkTheme(m_hwndChannelScroll, IsDarkThemeStyle(m_ChannelAreaBackStyle));
			}

			m_TunerScrollPos = 0;
			m_ChannelScrollPos = 0;
			m_CurTuner = -1;
			m_CurChannel = -1;
			m_LastCursorPos.x = -1;
			m_LastCursorPos.y = -1;
			::SetTimer(hwnd, TIMER_CLOCK, 1000, nullptr);
			m_ClockTime.NowLocal();
		}
		return 0;

	case WM_SIZE:
		Layout();
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd, &ps);
			Draw(ps.hdc, &ps.rcPaint);
			::EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_VSCROLL:
		{
			const HWND hwndScroll = reinterpret_cast<HWND>(lParam);
			int Pos;

			if (hwndScroll == m_hwndTunerScroll) {
				Pos = m_TunerScrollPos;
				switch (LOWORD(wParam)) {
				case SB_LINEUP:        Pos--; break;
				case SB_LINEDOWN:      Pos++; break;
				case SB_PAGEUP:        Pos -= m_VisibleTunerItems; break;
				case SB_PAGEDOWN:      Pos += m_VisibleTunerItems; break;
				case SB_TOP:           Pos = 0; break;
				case SB_BOTTOM:        Pos = m_TotalTuningSpaces - m_VisibleTunerItems; break;
				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:    Pos = HIWORD(wParam); break;
				default: return 0;
				}
				SetTunerScrollPos(Pos, true);
			} else if (hwndScroll == m_hwndChannelScroll) {
				Pos = m_ChannelScrollPos;
				switch (LOWORD(wParam)) {
				case SB_LINEUP:        Pos--; break;
				case SB_LINEDOWN:      Pos++; break;
				case SB_PAGEUP:        Pos -= m_VisibleChannelItems; break;
				case SB_PAGEDOWN:      Pos += m_VisibleChannelItems; break;
				case SB_TOP:           Pos = 0; break;
				case SB_BOTTOM:        Pos = 1000; break;
				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:    Pos = HIWORD(wParam); break;
				default: return 0;
				}
				SetChannelScrollPos(Pos, true);
			}
		}
		return 0;

	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		if (OnMouseWheel(uMsg, wParam, lParam))
			return 0;
		break;

	case WM_LBUTTONDOWN:
		::SetFocus(hwnd);
		if (m_pChannelDisplayEventHandler != nullptr) {
			const int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

			if (CloseButtonHitTest(x, y)) {
				Close();
				return 0;
			}
			if (m_CurTuner >= 0) {
				if (TunerItemHitTest(x, y) == m_CurTuner) {
					NotifyTunerSelect();
					return 0;
				}
			}
			if (m_CurChannel >= 0) {
				if (ChannelItemHitTest(x, y) == m_CurChannel) {
					NotifyChannelSelect();
				}
			}
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			const int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
			if (m_LastCursorPos.x == x && m_LastCursorPos.y == y)
				return 0;
			m_LastCursorPos.x = x;
			m_LastCursorPos.y = y;
			int Index = TunerItemHitTest(x, y);
			if (Index >= 0 && Index != m_CurTuner) {
				SetCurTuner(Index);
			}
			if (Index < 0) {
				Index = ChannelItemHitTest(x, y);
				if (Index != m_CurChannel) {
					SetCurChannel(Index);
				}
			}
		}
		return 0;

	case WM_SETCURSOR:
		if (reinterpret_cast<HWND>(wParam) == hwnd
				&& LOWORD(lParam) == HTCLIENT) {
			const DWORD Pos = ::GetMessagePos();
			POINT pt;
			HCURSOR hCursor;

			pt.x = static_cast<SHORT>(LOWORD(Pos));
			pt.y = static_cast<SHORT>(HIWORD(Pos));
			::ScreenToClient(hwnd, &pt);
			if (TunerItemHitTest(pt.x, pt.y) >= 0
					|| ChannelItemHitTest(pt.x, pt.y) >= 0
					|| CloseButtonHitTest(pt.x, pt.y)) {
				hCursor = GetActionCursor();
			} else {
				hCursor = ::LoadCursor(nullptr, IDC_ARROW);
			}
			::SetCursor(hCursor);
			return TRUE;
		}
		break;

	case WM_KEYDOWN:
		switch (wParam) {
		case VK_UP:
		case VK_DOWN:
			if (m_CurChannel >= 0) {
				if (wParam == VK_DOWN || m_CurChannel > 0)
					SetCurChannel(wParam == VK_UP ? m_CurChannel - 1 : m_CurChannel + 1);
			} else {
				if (wParam == VK_DOWN || m_CurTuner > 0)
					SetCurTuner(wParam == VK_UP ? m_CurTuner - 1 : m_CurTuner + 1);
			}
			break;

		case VK_LEFT:
			if (m_CurChannel >= 0) {
				SetCurChannel(-1);
			}
			break;

		case VK_RIGHT:
			if (m_CurTuner >= 0 && m_CurChannel < 0) {
				const CTuningSpaceInfo *pTuningSpace = GetTuningSpaceInfo(m_CurTuner);

				if (pTuningSpace != nullptr && pTuningSpace->NumChannels() > 0) {
					SetCurChannel(0);
				}
			}
			break;

		case VK_RETURN:
		case VK_SPACE:
			if (m_CurChannel >= 0) {
				NotifyChannelSelect();
			} else if (m_CurTuner >= 0) {
				NotifyTunerSelect();
			}
			break;

		case VK_PRIOR:
		case VK_NEXT:
		case VK_HOME:
		case VK_END:
			{
				static const struct {
					WORD KeyCode;
					WORD Scroll;
				} KeyMap[] = {
					{VK_PRIOR, SB_PAGEUP},
					{VK_NEXT,  SB_PAGEDOWN},
					{VK_HOME,  SB_TOP},
					{VK_END,   SB_BOTTOM},
				};
				for (auto &Map : KeyMap) {
					if (Map.KeyCode == wParam) {
						::SendMessage(
							hwnd, WM_VSCROLL, Map.Scroll,
							reinterpret_cast<LPARAM>(m_CurChannel >= 0 ? m_hwndChannelScroll : m_hwndTunerScroll));
						break;
					}
				}
			}
			break;

		case VK_TAB:
			{
				int CurTuner = m_CurTuner;

				if (::GetKeyState(VK_SHIFT) < 0) {
					if (CurTuner < 1)
						break;
					CurTuner--;
				} else {
					CurTuner++;
				}
				SetCurTuner(CurTuner);
			}
			break;

		case VK_ESCAPE:
			Close();
			break;
		}
		return 0;

	case WM_SHOWWINDOW:
		if (wParam) {
			POINT pt;

			::GetCursorPos(&pt);
			::ScreenToClient(hwnd, &pt);
			m_LastCursorPos = pt;
		}
		break;

	case WM_TIMER:
		if (wParam == TIMER_CLOCK) {
			LibISDB::DateTime CurTime;

			CurTime.NowLocal();
			if (m_ClockTime.Hour != CurTime.Hour
					|| m_ClockTime.Minute != CurTime.Minute) {
				m_ClockTime = CurTime;
				const HDC hdc = GetDC(hwnd);
				DrawClock(hdc);
				ReleaseDC(hwnd, hdc);
			}
		}
		return 0;
	}

	return CDisplayView::OnMessage(hwnd, uMsg, wParam, lParam);
}


void CChannelDisplay::ApplyStyle()
{
	if (m_hwnd != nullptr) {
		if (!m_fAutoFontSize)
			CreateDrawFont(m_StyleFont, &m_Font);
	}
}


void CChannelDisplay::RealizeStyle()
{
	if (m_hwnd != nullptr) {
		Layout();
		Invalidate();
	}
}




CChannelDisplay::CTuner::CTuner(const CDriverInfo *pDriverInfo)
	: m_DriverFileName(pDriverInfo->GetFileName())
	, m_TunerName(pDriverInfo->GetTunerName())
{
	const CTuningSpaceList *pList = pDriverInfo->GetAvailableTuningSpaceList();

	if (pList != nullptr) {
		for (int i = 0; i < pList->NumSpaces(); i++) {
			const CTuningSpaceInfo *pSrcTuningSpace = pList->GetTuningSpaceInfo(i);
			if (pSrcTuningSpace == nullptr)
				break;
			const CChannelList *pSrcChannelList = pSrcTuningSpace->GetChannelList();

			if (pSrcChannelList != nullptr && pSrcChannelList->NumEnableChannels() > 0) {
				CTuningSpaceInfo *pTuningSpace;

				if (!m_TuningSpaceList.empty()
						&& pSrcTuningSpace->GetType() == CTuningSpaceInfo::TuningSpaceType::Terrestrial
						&& m_TuningSpaceList[m_TuningSpaceList.size() - 1]->GetType() == CTuningSpaceInfo::TuningSpaceType::Terrestrial) {
					pTuningSpace = m_TuningSpaceList[m_TuningSpaceList.size() - 1].get();
					pTuningSpace->SetName(TEXT("地上"));
				} else {
					pTuningSpace = new CTuningSpaceInfo;
					pTuningSpace->Create(nullptr, pSrcTuningSpace->GetName());
					m_TuningSpaceList.emplace_back(pTuningSpace);
				}
				for (int j = 0; j < pSrcChannelList->NumChannels(); j++) {
					const CChannelInfo *pChannelInfo = pSrcChannelList->GetChannelInfo(j);

					if (pChannelInfo->IsEnabled()) {
						pTuningSpace->GetChannelList()->AddChannel(new CChannel(*pChannelInfo));
					}
				}
			}
		}
	}
	if (m_TuningSpaceList.empty()) {
		CTuningSpaceInfo *pTuningSpace = new CTuningSpaceInfo;

		pTuningSpace->Create();
		m_TuningSpaceList.emplace_back(pTuningSpace);
	}
}


void CChannelDisplay::CTuner::Clear()
{
	m_TuningSpaceList.clear();
}


LPCTSTR CChannelDisplay::CTuner::GetDisplayName() const
{
	if (!m_DisplayName.empty())
		return m_DisplayName.c_str();
	return m_TunerName.c_str();
}


void CChannelDisplay::CTuner::GetDisplayName(int Space, LPTSTR pszName, int MaxName) const
{
	if (!IsStringEmpty(GetDisplayName())) {
		StringCopy(pszName, GetDisplayName(), MaxName);
	} else {
		LPCTSTR pszDriver = GetDriverFileName();
		if (::StrCmpNI(pszDriver, TEXT("BonDriver_"), 10) == 0)
			pszDriver += 10;
		StringCopy(pszName, pszDriver, MaxName);
		::PathRemoveExtension(pszName);
	}

	if (m_TuningSpaceList.size() > 1) {
		const CTuningSpaceInfo *pTuningSpace = GetTuningSpaceInfo(Space);
		if (pTuningSpace != nullptr) {
			const int Length = ::lstrlen(pszName);
			if (!IsStringEmpty(pTuningSpace->GetName()))
				StringFormat(pszName + Length, MaxName - Length, TEXT(" [{}]"), pTuningSpace->GetName());
			else
				StringFormat(pszName + Length, MaxName - Length, TEXT(" [{}]"), Space + 1);
		}
	}
}


void CChannelDisplay::CTuner::SetDisplayName(LPCTSTR pszName)
{
	StringUtility::Assign(m_DisplayName, pszName);
}


int CChannelDisplay::CTuner::NumSpaces() const
{
	return static_cast<int>(m_TuningSpaceList.size());
}


CTuningSpaceInfo *CChannelDisplay::CTuner::GetTuningSpaceInfo(int Index)
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_TuningSpaceList.size())
		return nullptr;
	return m_TuningSpaceList[Index].get();
}


const CTuningSpaceInfo *CChannelDisplay::CTuner::GetTuningSpaceInfo(int Index) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_TuningSpaceList.size())
		return nullptr;
	return m_TuningSpaceList[Index].get();
}


void CChannelDisplay::CTuner::SetIcon(HICON hico)
{
	m_Icon.Attach(hico);
}




bool CChannelDisplay::CTuner::CChannel::SetEvent(int Index, const LibISDB::EventInfo *pEvent)
{
	if (Index < 0 || Index > 1)
		return false;
	if (pEvent != nullptr)
		m_Event[Index] = *pEvent;
	else
		m_Event[Index].EventName.clear();
	return true;
}


const LibISDB::EventInfo *CChannelDisplay::CTuner::CChannel::GetEvent(int Index) const
{
	if (Index < 0 || Index > 1)
		return nullptr;
	if (m_Event[Index].EventName.empty())
		return nullptr;
	return &m_Event[Index];
}




CChannelDisplay::ChannelDisplayStyle::ChannelDisplayStyle()
	: TunerItemPadding(8, 4, 8, 4)
	, TunerIconSize(32, 32)
	, TunerIconTextMargin(4)
	, ChannelItemPadding(8, 2, 4, 2)
	, ChannelEventMargin(8)
	, ClockPadding(2)
	, ClockMargin(6)
{
}


void CChannelDisplay::ChannelDisplayStyle::SetStyle(const Style::CStyleManager *pStyleManager)
{
	*this = ChannelDisplayStyle();
	pStyleManager->Get(TEXT("channel-display.tuner.padding"), &TunerItemPadding);
	pStyleManager->Get(TEXT("channel-display.tuner.icon"), &TunerIconSize);
	pStyleManager->Get(TEXT("channel-display.tuner.icon-text-margin"), &TunerIconTextMargin);
	pStyleManager->Get(TEXT("channel-display.channel.padding"), &ChannelItemPadding);
	pStyleManager->Get(TEXT("channel-display.channel.event-margin"), &ChannelEventMargin);
	pStyleManager->Get(TEXT("channel-display.clock.padding"), &ClockPadding);
	pStyleManager->Get(TEXT("channel-display.clock.margin"), &ClockMargin);
}


void CChannelDisplay::ChannelDisplayStyle::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	pStyleScaling->ToPixels(&TunerItemPadding);
	pStyleScaling->ToPixels(&TunerIconSize);
	pStyleScaling->ToPixels(&TunerIconTextMargin);
	pStyleScaling->ToPixels(&ChannelItemPadding);
	pStyleScaling->ToPixels(&ChannelEventMargin);
	pStyleScaling->ToPixels(&ClockPadding);
	pStyleScaling->ToPixels(&ClockMargin);
}


} // namespace TVTest
