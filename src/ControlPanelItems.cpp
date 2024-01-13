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
#include "ControlPanelItems.h"
#include "AppMain.h"
#include "Menu.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


void CTunerControlItem::CalcSize(int Width, SIZE *pSize)
{
	pSize->cx = Width;
	pSize->cy = GetTextItemHeight();
}

void CTunerControlItem::Draw(HDC hdc, const RECT &Rect)
{
	const CChannelManager &ChannelManager = GetAppClass().ChannelManager;
	const CChannelInfo *pChInfo = ChannelManager.GetCurrentChannelInfo();
	LPCTSTR pszText;

	if (pChInfo != nullptr || ChannelManager.GetCurrentSpace() >= 0) {
		pszText =
			ChannelManager.GetTuningSpaceName(
				pChInfo != nullptr ? pChInfo->GetSpace() : ChannelManager.GetCurrentSpace());
		if (pszText == nullptr)
			pszText = TEXT("<チューナー>");
	} else if (ChannelManager.GetCurrentSpace() == CChannelManager::SPACE_ALL) {
		pszText = TEXT("すべて");
	} else {
		pszText = TEXT("<チューナー>");
	}
	RECT rc = Rect;
	Style::Subtract(&rc, m_pControlPanel->GetItemPadding());
	::DrawText(
		hdc, pszText, -1, &rc,
		DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
}

void CTunerControlItem::OnLButtonDown(int x, int y)
{
	POINT pt;

	GetMenuPos(&pt);
	GetAppClass().UICore.PopupSubMenu(CMainMenu::SUBMENU_SPACE, &pt, TPM_RIGHTBUTTON);
}

void CTunerControlItem::OnRButtonDown(int x, int y)
{
	POINT pt;

	GetMenuPos(&pt);
	GetAppClass().UICore.ShowSpecialMenu(CUICore::MenuType::TunerSelect, &pt, TPM_RIGHTBUTTON);
}


void CChannelControlItem::CalcSize(int Width, SIZE *pSize)
{
	pSize->cx = Width;
	pSize->cy = GetTextItemHeight();
}

void CChannelControlItem::Draw(HDC hdc, const RECT &Rect)
{
	const CAppMain &App = GetAppClass();
	const CChannelManager &ChannelManager = App.ChannelManager;
	const CChannelInfo *pInfo;
	TCHAR szText[4 + MAX_CHANNEL_NAME];

	if (App.UICore.GetSkin()->IsWheelChannelChanging()) {
		const COLORREF crText = ::GetTextColor(hdc);
		const COLORREF crBack = ::GetBkColor(hdc);
		::SetTextColor(hdc, MixColor(crText, crBack, 128));
		pInfo = ChannelManager.GetChangingChannelInfo();
		StringFormat(szText, TEXT("{}: {}"), pInfo->GetChannelNo(), pInfo->GetName());
	} else if ((pInfo = ChannelManager.GetCurrentChannelInfo()) != nullptr) {
		StringFormat(szText, TEXT("{}: {}"), pInfo->GetChannelNo(), pInfo->GetName());
	} else {
		StringCopy(szText, TEXT("<チャンネル>"));
	}
	RECT rc = Rect;
	Style::Subtract(&rc, m_pControlPanel->GetItemPadding());
	::DrawText(
		hdc, szText, -1, &rc,
		DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
}

void CChannelControlItem::OnLButtonDown(int x, int y)
{
	POINT pt;

	GetMenuPos(&pt);
	GetAppClass().UICore.PopupSubMenu(CMainMenu::SUBMENU_CHANNEL, &pt, TPM_RIGHTBUTTON);
}

void CChannelControlItem::OnRButtonDown(int x, int y)
{
	POINT pt;

	GetMenuPos(&pt);
	GetAppClass().UICore.PopupSubMenu(CMainMenu::SUBMENU_SERVICE, &pt, TPM_RIGHTBUTTON);
}


void CVideoControlItem::CalcSize(int Width, SIZE *pSize)
{
	pSize->cx = Width;
	pSize->cy = GetTextItemHeight();
}

void CVideoControlItem::Draw(HDC hdc, const RECT &Rect)
{
	const CAppMain &App = GetAppClass();
	const CCoreEngine &CoreEngine = App.CoreEngine;
	TCHAR szText[32];

	StringFormat(
		szText, TEXT("{} x {} ({} %)"),
		CoreEngine.GetOriginalVideoWidth(),
		CoreEngine.GetOriginalVideoHeight(),
		App.UICore.GetZoomPercentage());
	RECT rc = Rect;
	Style::Subtract(&rc, m_pControlPanel->GetItemPadding());
	::DrawText(
		hdc, szText, -1, &rc,
		DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
}

void CVideoControlItem::OnLButtonDown(int x, int y)
{
	POINT pt;

	GetMenuPos(&pt);
	GetAppClass().UICore.PopupSubMenu(CMainMenu::SUBMENU_ZOOM, &pt, TPM_RIGHTBUTTON);
}

void CVideoControlItem::OnRButtonDown(int x, int y)
{
	POINT pt;

	GetMenuPos(&pt);
	GetAppClass().UICore.PopupSubMenu(CMainMenu::SUBMENU_ASPECTRATIO, &pt, TPM_RIGHTBUTTON);
}


void CVolumeControlItem::CalcSize(int Width, SIZE *pSize)
{
	const Style::Margins &Padding = m_pControlPanel->GetItemPadding();
	pSize->cx = Width;
	pSize->cy = m_Style.BarHeight + Padding.Vert();
}

void CVolumeControlItem::Draw(HDC hdc, const RECT &Rect)
{
	const CUICore &UICore = GetAppClass().UICore;
	const COLORREF TextColor = ::GetTextColor(hdc);
	const LOGBRUSH lb = {BS_SOLID, TextColor, 0};
	const HPEN hpen = ::ExtCreatePen(
		PS_GEOMETRIC | PS_SOLID | PS_INSIDEFRAME | PS_JOIN_MITER,
		m_Style.BarBorderWidth, &lb, 0, nullptr);
	const HPEN hpenOld = SelectPen(hdc, hpen);
	const HBRUSH hbrOld = SelectBrush(hdc, ::GetStockObject(NULL_BRUSH));
	RECT rc = Rect;
	Style::Subtract(&rc, m_pControlPanel->GetItemPadding());
	rc.top += ((rc.bottom - rc.top) - m_Style.BarHeight) / 2;
	rc.bottom = rc.top + m_Style.BarHeight;
	::Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
	SelectBrush(hdc, hbrOld);
	SelectPen(hdc, hpenOld);
	::DeleteObject(hpen);

	COLORREF BarColor;
	if (!UICore.GetMute())
		BarColor = TextColor;
	else
		BarColor = MixColor(TextColor, ::GetBkColor(hdc));
	::InflateRect(&rc, -m_Style.BarBorderWidth, -m_Style.BarBorderWidth);
	Style::Subtract(&rc, m_Style.BarPadding);
	rc.right = rc.left + (rc.right - rc.left) * UICore.GetVolume() / CCoreEngine::MAX_VOLUME;
	DrawUtil::Fill(hdc, &rc, BarColor);
}

void CVolumeControlItem::OnLButtonDown(int x, int y)
{
	OnMouseMove(x, y);
	::SetCapture(m_pControlPanel->GetHandle());
}

void CVolumeControlItem::OnRButtonDown(int x, int y)
{
	CUICore &UICore = GetAppClass().UICore;
	UICore.SetMute(!UICore.GetMute());
}

void CVolumeControlItem::OnMouseMove(int x, int y)
{
	CUICore &UICore = GetAppClass().UICore;
	RECT rc = m_Position;
	Style::Subtract(&rc, m_pControlPanel->GetItemPadding());
	Style::Subtract(&rc, m_Style.BarPadding);
	int Volume = (x - rc.left) * CCoreEngine::MAX_VOLUME / ((rc.right - rc.left) - 1);
	if (Volume < 0)
		Volume = 0;
	else if (Volume > CCoreEngine::MAX_VOLUME)
		Volume = CCoreEngine::MAX_VOLUME;
	if (UICore.GetMute() || Volume != UICore.GetVolume())
		UICore.SetVolume(Volume, false);
}

void CVolumeControlItem::SetStyle(const Style::CStyleManager *pStyleManager)
{
	m_Style = VolumeControlStyle();
	pStyleManager->Get(TEXT("control-panel.volume.bar.height"), &m_Style.BarHeight);
	pStyleManager->Get(TEXT("control-panel.volume.bar.padding"), &m_Style.BarPadding);
	pStyleManager->Get(TEXT("control-panel.volume.bar.border.width"), &m_Style.BarBorderWidth);
}

void CVolumeControlItem::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	pStyleScaling->ToPixels(&m_Style.BarHeight);
	pStyleScaling->ToPixels(&m_Style.BarPadding);
	pStyleScaling->ToPixels(&m_Style.BarBorderWidth);
}


void CAudioControlItem::CalcSize(int Width, SIZE *pSize)
{
	pSize->cx = Width;
	pSize->cy = GetTextItemHeight();
	if (pSize->cy < 16)
		pSize->cy = 16;
}

void CAudioControlItem::Draw(HDC hdc, const RECT &Rect)
{
	const CAppMain &App = GetAppClass();
	RECT rc = Rect;
	Style::Subtract(&rc, m_pControlPanel->GetItemPadding());

	const LibISDB::ViewerFilter *pViewer = App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();
	if (pViewer != nullptr && pViewer->IsSPDIFPassthrough()) {
		const Style::Size IconSize = m_pControlPanel->GetIconSize();
		if (!m_Icons.IsCreated()) {
			static const Theme::IconList::ResourceInfo ResourceList[] = {
				{MAKEINTRESOURCE(IDB_PASSTHROUGH16), 16, 16},
				{MAKEINTRESOURCE(IDB_PASSTHROUGH32), 32, 32},
			};
			m_Icons.Load(
				App.GetResourceInstance(),
				IconSize.Width, IconSize.Height,
				ResourceList, lengthof(ResourceList));
		}
		m_Icons.Draw(
			hdc, rc.left, rc.top + ((rc.bottom - rc.top) - IconSize.Height) / 2,
			IconSize.Width, IconSize.Height, 0, ::GetTextColor(hdc));
		rc.left += IconSize.Width + IconSize.Width / 4;
	}

	TCHAR szText[64];
	if (App.UICore.FormatCurrentAudioText(szText, lengthof(szText)) <= 0)
		StringCopy(szText, TEXT("<音声>"));
	::DrawText(
		hdc, szText, -1, &rc,
		DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
}

void CAudioControlItem::OnLButtonDown(int x, int y)
{
	if (!GetAppClass().UICore.SwitchAudio())
		OnRButtonDown(x, y);
}

void CAudioControlItem::OnRButtonDown(int x, int y)
{
	POINT pt;

	GetMenuPos(&pt);
	GetAppClass().UICore.PopupSubMenu(CMainMenu::SUBMENU_AUDIO, &pt, TPM_RIGHTBUTTON);
}




CControlPanelButton::CControlPanelButton(int Command, LPCTSTR pszText, bool fBreak, int Width)
	: m_Text(pszText)
	, m_Width(Width)
{
	m_Command = Command;
	m_fBreak = fBreak;
}

void CControlPanelButton::CalcSize(int Width, SIZE *pSize)
{
	const Style::Margins &Padding = m_pControlPanel->GetItemPadding();

	if (m_Width < 0)
		CalcTextSize(m_Text.c_str(), pSize);
	else
		pSize->cx = m_Width * m_pControlPanel->GetFontHeight();
	pSize->cx += Padding.Horz();
	pSize->cy = GetTextItemHeight();
}

void CControlPanelButton::Draw(HDC hdc, const RECT &Rect)
{
	RECT rc = Rect;
	Style::Subtract(&rc, m_pControlPanel->GetItemPadding());
	::DrawText(
		hdc, m_Text.c_str(), -1, &rc,
		DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}


} // namespace TVTest
