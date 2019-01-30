/*
  TVTest
  Copyright(c) 2008-2019 DBCTRADO

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
#include "OSDManager.h"
#include "AppMain.h"
#include "LogoManager.h"
#include "Common/DebugDef.h"


namespace TVTest
{


COSDManager::COSDManager(const COSDOptions *pOptions)
	: m_pOptions(pOptions)
	, m_pEventHandler(nullptr)
{
}


COSDManager::~COSDManager()
{
}


bool COSDManager::Initialize()
{
	InitializeUI();

	return true;
}


void COSDManager::SetEventHandler(CEventHandler *pEventHandler)
{
	m_pEventHandler = pEventHandler;
}


void COSDManager::Reset()
{
	m_OSD.Destroy();
	m_VolumeOSD.Destroy();
}


void COSDManager::ClearOSD()
{
	LibISDB::ViewerFilter *pViewer = GetAppClass().CoreEngine.GetFilter<LibISDB::ViewerFilter>();
	if (pViewer != nullptr)
		pViewer->ClearOSD();
	m_OSD.Hide();
	m_VolumeOSD.Hide();
}


void COSDManager::OnParentMove()
{
	m_OSD.OnParentMove();
	m_VolumeOSD.OnParentMove();
}


bool COSDManager::ShowOSD(LPCTSTR pszText, ShowFlag Flags)
{
	if (IsStringEmpty(pszText))
		return false;

	CAppMain &App = GetAppClass();
	CCoreEngine &CoreEngine = App.CoreEngine;
	LibISDB::ViewerFilter *pViewer = CoreEngine.GetFilter<LibISDB::ViewerFilter>();

	if (pViewer == nullptr)
		return false;

	OSDClientInfo ClientInfo;
	ClientInfo.fForcePseudoOSD = false;
	if (!m_pEventHandler->GetOSDClientInfo(&ClientInfo))
		return false;
	if (!!(Flags & ShowFlag::Pseudo))
		ClientInfo.fForcePseudoOSD = true;

	DWORD FadeTime;
	if (!!(Flags & ShowFlag::NoFade))
		FadeTime = 0;
	else
		FadeTime = m_pOptions->GetFadeTime();

	if (!m_pOptions->GetPseudoOSD() && !ClientInfo.fForcePseudoOSD
			&& pViewer->IsDrawTextSupported()) {
		CompositeText(pszText, ClientInfo.ClientRect, 0, FadeTime);
	} else {
		m_OSD.SetText(pszText);
		CreateTextOSD(pszText, ClientInfo);
		m_OSD.SetTextColor(m_pOptions->GetTextColor());
		m_OSD.Show(FadeTime, false);
	}

	return true;
}


void COSDManager::HideOSD()
{
	m_OSD.Hide();
}


bool COSDManager::ShowChannelOSD(const CChannelInfo *pInfo, LPCTSTR pszText, bool fChanging)
{
	if (m_pEventHandler == nullptr || pInfo == nullptr)
		return false;

	CAppMain &App = GetAppClass();
	CCoreEngine &CoreEngine = App.CoreEngine;
	LibISDB::ViewerFilter *pViewer = CoreEngine.GetFilter<LibISDB::ViewerFilter>();

	if (pViewer == nullptr)
		return false;

	OSDClientInfo ClientInfo;
	ClientInfo.fForcePseudoOSD = false;
	ClientInfo.fAnimation = m_Style.fChannelAnimation && !fChanging;
	if (!m_pEventHandler->GetOSDClientInfo(&ClientInfo))
		return false;
	if (!m_Style.fChannelAnimation || m_OSD.IsVisible())
		ClientInfo.fAnimation = false;

	COSDOptions::ChannelChangeType ChangeType = m_pOptions->GetChannelChangeType();

	HBITMAP hbmLogo = nullptr;
	CPseudoOSD::ImageEffect ImageEffect = CPseudoOSD::ImageEffect::None;
	if (ChangeType != COSDOptions::ChannelChangeType::TextOnly) {
		hbmLogo = App.LogoManager.GetAssociatedLogoBitmap(
			pInfo->GetNetworkID(), pInfo->GetServiceID(), CLogoManager::LOGOTYPE_BIG);
		if (hbmLogo != nullptr) {
			if (fChanging)
				ImageEffect = CPseudoOSD::ImageEffect::Dark;
			else if (StringUtility::CompareNoCase(m_Style.LogoEffect, TEXT("gloss")) == 0)
				ImageEffect = CPseudoOSD::ImageEffect::Gloss;
		}

		if (ChangeType == COSDOptions::ChannelChangeType::LogoOnly && hbmLogo != nullptr) {
			m_OSD.Create(ClientInfo.hwndParent, m_pOptions->GetLayeredWindow());
			m_OSD.SetImage(hbmLogo, ImageEffect);
			m_OSD.SetPosition(
				ClientInfo.ClientRect.left + m_Style.Margin.Left,
				ClientInfo.ClientRect.top + m_Style.Margin.Top,
				m_Style.LogoSize.Width, m_Style.LogoSize.Height);
			m_OSD.Show(m_pOptions->GetFadeTime(), ClientInfo.fAnimation);
			return true;
		}
	}

	if (!m_pOptions->GetPseudoOSD() && !ClientInfo.fForcePseudoOSD
			&& pViewer->IsDrawTextSupported()) {
		if (hbmLogo != nullptr) {
			m_OSD.Create(ClientInfo.hwndParent, m_pOptions->GetLayeredWindow());
			m_OSD.SetImage(hbmLogo, ImageEffect);
			m_OSD.SetPosition(
				ClientInfo.ClientRect.left + m_Style.Margin.Left,
				ClientInfo.ClientRect.top + m_Style.Margin.Top,
				m_Style.LogoSize.Width, m_Style.LogoSize.Height);
			m_OSD.Show(m_pOptions->GetFadeTime(), ClientInfo.fAnimation);
		}

		if (ChangeType != COSDOptions::ChannelChangeType::LogoOnly && !IsStringEmpty(pszText)) {
			CompositeText(
				pszText,
				ClientInfo.ClientRect,
				hbmLogo != nullptr ? static_cast<int>(m_Style.LogoSize.Width) : 0,
				m_pOptions->GetFadeTime());
		}
	} else {
		m_OSD.SetText(pszText, hbmLogo, m_Style.LogoSize.Width, m_Style.LogoSize.Height, ImageEffect);
		CreateTextOSD(pszText, ClientInfo, m_Style.LogoSize.Width, m_Style.LogoSize.Height);
		COLORREF cr;
		if (fChanging)
			cr = MixColor(m_pOptions->GetTextColor(), RGB(0, 0, 0), 160);
		else
			cr = m_pOptions->GetTextColor();
		m_OSD.SetTextColor(cr);
		m_OSD.Show(m_pOptions->GetFadeTime(), ClientInfo.fAnimation);
	}

	return true;
}


void COSDManager::HideChannelOSD()
{
	m_OSD.Hide();
}


bool COSDManager::ShowVolumeOSD(int Volume)
{
	if (m_pEventHandler == nullptr)
		return false;

	CAppMain &App = GetAppClass();
	CCoreEngine &CoreEngine = App.CoreEngine;
	LibISDB::ViewerFilter *pViewer = CoreEngine.GetFilter<LibISDB::ViewerFilter>();

	if (pViewer == nullptr)
		return false;

	OSDClientInfo ClientInfo;
	ClientInfo.fForcePseudoOSD = false;
	if (!m_pEventHandler->GetOSDClientInfo(&ClientInfo))
		return false;

	static const int VolumeSteps = 20;
	WCHAR szText[64];
	int i;

	for (i = 0; i < std::min(Volume, 100) / (100 / VolumeSteps); i++)
		szText[i] = L'■';
	for (; i < VolumeSteps; i++)
		szText[i] = L'□';
	StringPrintf(szText + i, lengthof(szText) - i, TEXT(" %d"), Volume);

	if (!m_pOptions->GetPseudoOSD() && !ClientInfo.fForcePseudoOSD
			&& pViewer->IsDrawTextSupported()) {
		RECT rcSrc;

		if (pViewer->GetSourceRect(&rcSrc)) {
			int FontSize = ((rcSrc.right - rcSrc.left) - m_Style.VolumeMargin.Horz()) / (VolumeSteps * 2);
			if (FontSize < m_Style.VolumeTextSizeMin)
				FontSize = m_Style.VolumeTextSizeMin;
			LOGFONT lf;
			HFONT hfont;

			lf = *m_pOptions->GetOSDFont();
			lf.lfHeight = -FontSize;
			lf.lfQuality = NONANTIALIASED_QUALITY;
			hfont = ::CreateFontIndirect(&lf);
			rcSrc.left +=
				m_Style.VolumeMargin.Left * (rcSrc.right - rcSrc.left) /
				(ClientInfo.ClientRect.right - ClientInfo.ClientRect.left);
			rcSrc.top =
				rcSrc.bottom - FontSize -
				m_Style.VolumeMargin.Bottom * (rcSrc.bottom - rcSrc.top) /
				(ClientInfo.ClientRect.bottom - ClientInfo.ClientRect.top);
			if (pViewer->DrawText(
						szText,
						rcSrc.left, rcSrc.top, hfont,
						m_pOptions->GetTextColor(), m_pOptions->GetOpacity())) {
				if (m_pOptions->GetFadeTime() > 0)
					m_pEventHandler->SetOSDHideTimer(m_pOptions->GetFadeTime());
			}
			::DeleteObject(hfont);
		}
	} else {
		int FontSize = ((ClientInfo.ClientRect.right - ClientInfo.ClientRect.left) - m_Style.VolumeMargin.Horz()) / (VolumeSteps * 2);
		if (FontSize < m_Style.VolumeTextSizeMin)
			FontSize = m_Style.VolumeTextSizeMin;
		LOGFONT lf;

		lf = *m_pOptions->GetOSDFont();
		lf.lfHeight = -FontSize;
		m_VolumeOSD.Create(ClientInfo.hwndParent, m_pOptions->GetLayeredWindow());
		m_VolumeOSD.SetTextStyle(
			CPseudoOSD::TextStyle::Left | CPseudoOSD::TextStyle::VertCenter |
			CPseudoOSD::TextStyle::FillBackground);
		//m_VolumeOSD.SetTextHeight(FontSize);
		m_VolumeOSD.SetFont(lf);
		m_VolumeOSD.SetText(szText);
		SIZE sz = {
			ClientInfo.ClientRect.right - ClientInfo.ClientRect.left - m_Style.VolumeMargin.Left,
			ClientInfo.ClientRect.bottom - ClientInfo.ClientRect.top - m_Style.VolumeMargin.Bottom
		};
		m_VolumeOSD.CalcTextSize(&sz);
		m_VolumeOSD.SetPosition(
			ClientInfo.ClientRect.left + m_Style.VolumeMargin.Left,
			ClientInfo.ClientRect.bottom - sz.cy - m_Style.VolumeMargin.Bottom,
			sz.cx + FontSize / 4, sz.cy);
		m_VolumeOSD.SetTextColor(m_pOptions->GetTextColor());
		m_VolumeOSD.Show(m_pOptions->GetFadeTime());
	}

	return true;
}


void COSDManager::HideVolumeOSD()
{
	m_VolumeOSD.Hide();
}


bool COSDManager::CompositeText(
	LPCTSTR pszText, const RECT &rcClient, int LeftOffset, DWORD FadeTime)
{
	CAppMain &App = GetAppClass();
	LibISDB::ViewerFilter *pViewer = App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();

	if (pViewer == nullptr)
		return false;

	RECT rcSrc;
	if (!pViewer->GetSourceRect(&rcSrc))
		return false;

	LOGFONT lf = *m_pOptions->GetOSDFont();
	int FontSize = std::max((rcSrc.right - rcSrc.left) / m_Style.CompositeTextSizeRatio, 12L);
	lf.lfHeight = -FontSize;
	lf.lfWidth = 0;
	lf.lfQuality = NONANTIALIASED_QUALITY;
	HFONT hfont = ::CreateFontIndirect(&lf);

	int Rate, Factor;
	if (!App.UICore.GetZoomRate(&Rate, &Factor)) {
		if ((rcClient.right - rcClient.left) / (rcSrc.right - rcSrc.left) <
				(rcClient.bottom - rcClient.top) / (rcSrc.bottom - rcSrc.top)) {
			Rate = rcClient.right - rcClient.left;
			Factor = rcSrc.right - rcSrc.left;
		} else {
			Rate = rcClient.bottom - rcClient.top;
			Factor = rcSrc.bottom - rcSrc.top;
		}
	}
	if (Rate != 0) {
		rcSrc.left += (m_Style.Margin.Left + LeftOffset) * Factor / Rate;
		rcSrc.top += (rcClient.top + m_Style.Margin.Top) * Factor / Rate;
	} else {
		rcSrc.left += 16;
		rcSrc.top += 48;
	}
	bool fOK = false;
	if (pViewer->DrawText(
				pszText,
				rcSrc.left, rcSrc.top, hfont,
				m_pOptions->GetTextColor(), m_pOptions->GetOpacity())) {
		fOK = true;
		if (FadeTime > 0)
			m_pEventHandler->SetOSDHideTimer(FadeTime);
	}
	::DeleteObject(hfont);

	return fOK;
}


bool COSDManager::CreateTextOSD(LPCTSTR pszText, const OSDClientInfo &ClientInfo, int ImageWidth, int ImageHeight)
{
	if (!m_OSD.Create(ClientInfo.hwndParent, m_pOptions->GetLayeredWindow()))
		return false;

	const int FontSize = std::max((ClientInfo.ClientRect.right - ClientInfo.ClientRect.left) / m_Style.TextSizeRatio, 12L);
	const int TextMargin = FontSize / 2;
	LOGFONT lf = *m_pOptions->GetOSDFont();
	lf.lfHeight = -FontSize;

	m_OSD.SetTextStyle(
		CPseudoOSD::TextStyle::Left |
		CPseudoOSD::TextStyle::VertCenter |
		CPseudoOSD::TextStyle::Outline |
		CPseudoOSD::TextStyle::FillBackground |
		CPseudoOSD::TextStyle::MultiLine);
	//m_OSD.SetTextHeight(FontSize);
	m_OSD.SetFont(lf);
	SIZE sz = {
		(ClientInfo.ClientRect.right - ClientInfo.ClientRect.left) -
			(m_Style.Margin.Left + m_Style.Margin.Right) -
			ImageWidth - TextMargin,
		(ClientInfo.ClientRect.bottom - ClientInfo.ClientRect.top) -
			(m_Style.Margin.Top + m_Style.Margin.Bottom)
	};
	m_OSD.CalcTextSize(&sz);

	bool fSingleLine = (sz.cy < FontSize * 3 / 2);
	if (fSingleLine) {
		// 単一行(のはず)
		m_OSD.SetTextStyle(
			CPseudoOSD::TextStyle::HorzCenter |
			CPseudoOSD::TextStyle::VertCenter |
			CPseudoOSD::TextStyle::Outline |
			CPseudoOSD::TextStyle::FillBackground);
	}

	m_OSD.SetPosition(
		ClientInfo.ClientRect.left + m_Style.Margin.Left,
		ClientInfo.ClientRect.top + m_Style.Margin.Top,
		sz.cx + TextMargin + ImageWidth,
		std::max(sz.cy, (LONG)ImageHeight));

	return true;
}


void COSDManager::SetStyle(const Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);
}


void COSDManager::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	m_Style.NormalizeStyle(pStyleManager, pStyleScaling);
}




COSDManager::OSDStyle::OSDStyle()
	: Margin(8)
	, TextSizeRatio(28)
	, CompositeTextSizeRatio(24)
	, LogoSize(64, 36)
	, LogoEffect(TEXT("gloss"))
	, fChannelAnimation(true)
	, VolumeMargin(16)
	, VolumeTextSizeMin(10)
{
}


void COSDManager::OSDStyle::SetStyle(const Style::CStyleManager *pStyleManager)
{
	Style::IntValue Value;

	*this = OSDStyle();
	pStyleManager->Get(TEXT("osd.margin"), &Margin);
	if (pStyleManager->Get(TEXT("osd.text-size-ratio"), &Value) && Value.Value > 0)
		TextSizeRatio = Value;
	if (pStyleManager->Get(TEXT("osd.composite-text-size-ratio"), &Value) && Value.Value > 0)
		CompositeTextSizeRatio = Value;
	pStyleManager->Get(TEXT("channel-osd.logo"), &LogoSize);
	pStyleManager->Get(TEXT("channel-osd.logo.effect"), &LogoEffect);
	pStyleManager->Get(TEXT("channel-osd.animation"), &fChannelAnimation);
	pStyleManager->Get(TEXT("volume-osd.margin"), &VolumeMargin);
	pStyleManager->Get(TEXT("volume-osd.text-size-min"), &VolumeTextSizeMin);
}


void COSDManager::OSDStyle::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	pStyleScaling->ToPixels(&Margin);
	pStyleScaling->ToPixels(&LogoSize);
	pStyleScaling->ToPixels(&VolumeMargin);
	pStyleScaling->ToPixels(&VolumeTextSizeMin);
}


}	// namespace TVTest
