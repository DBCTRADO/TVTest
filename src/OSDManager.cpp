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
#include "OSDManager.h"
#include "AppMain.h"
#include "LogoManager.h"
#include "Common/DebugDef.h"


namespace TVTest
{


COSDManager::COSDManager(const COSDOptions *pOptions)
	: m_pOptions(pOptions)
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
	m_EventInfoOSD.Hide();
}


void COSDManager::ClearOSD()
{
	ClearCompositedOSD();
	m_OSD.Hide();
	m_VolumeOSD.Hide();
}


void COSDManager::ClearCompositedOSD()
{
	LibISDB::ViewerFilter *pViewer = GetAppClass().CoreEngine.GetFilter<LibISDB::ViewerFilter>();
	if (pViewer != nullptr)
		pViewer->ClearOSD();
}


void COSDManager::OnParentMove()
{
	m_OSD.OnParentMove();
	m_VolumeOSD.OnParentMove();
	m_EventInfoOSD.OnParentMove();
}


void COSDManager::AdjustPosition()
{
	if (m_EventInfoOSD.IsCreated()) {
		OSDClientInfo ClientInfo;
		ClientInfo.fForcePseudoOSD = true;
		if (m_pEventHandler->GetOSDClientInfo(&ClientInfo)) {
			RECT Rect = ClientInfo.ClientRect;
			if (m_EventInfoOSD.AdjustPosition(&Rect)) {
				m_EventInfoOSD.SetPosition(Rect);
			}
		}
	}
}


bool COSDManager::ShowOSD(LPCTSTR pszText, ShowFlag Flags)
{
	if (IsStringEmpty(pszText))
		return false;

	const CAppMain &App = GetAppClass();
	const CCoreEngine &CoreEngine = App.CoreEngine;
	const LibISDB::ViewerFilter *pViewer = CoreEngine.GetFilter<LibISDB::ViewerFilter>();

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
	const CCoreEngine &CoreEngine = App.CoreEngine;
	const LibISDB::ViewerFilter *pViewer = CoreEngine.GetFilter<LibISDB::ViewerFilter>();

	if (pViewer == nullptr)
		return false;

	OSDClientInfo ClientInfo;
	ClientInfo.fForcePseudoOSD = false;
	ClientInfo.fAnimation = m_Style.fChannelAnimation && !fChanging;
	if (!m_pEventHandler->GetOSDClientInfo(&ClientInfo))
		return false;
	if (!m_Style.fChannelAnimation || m_OSD.IsVisible())
		ClientInfo.fAnimation = false;

	const COSDOptions::ChannelChangeType ChangeType = m_pOptions->GetChannelChangeType();

	HBITMAP hbmLogo = nullptr;
	CPseudoOSD::ImageEffect ImageEffect = CPseudoOSD::ImageEffect::None;
	if (ChangeType != COSDOptions::ChannelChangeType::TextOnly) {
		hbmLogo = App.LogoManager.GetAssociatedLogoBitmap(
			pInfo->GetNetworkID(), pInfo->GetServiceID(), CLogoManager::LOGOTYPE_BIG);
		if (hbmLogo != nullptr) {
			if (fChanging)
				ImageEffect = CPseudoOSD::ImageEffect::Dark;
			else if (StringUtility::IsEqualNoCase(m_Style.LogoEffect, TEXT("gloss")))
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

	const CAppMain &App = GetAppClass();
	const CCoreEngine &CoreEngine = App.CoreEngine;
	LibISDB::ViewerFilter *pViewer = CoreEngine.GetFilter<LibISDB::ViewerFilter>();

	if (pViewer == nullptr)
		return false;

	OSDClientInfo ClientInfo;
	ClientInfo.fForcePseudoOSD = false;
	if (!m_pEventHandler->GetOSDClientInfo(&ClientInfo))
		return false;

	const int VolumeSteps = std::clamp(m_Style.VolumeSteps.Value, 2, 100);
	TCHAR szText[256];
	CStaticStringFormatter Formatter(szText, lengthof(szText));
	int i;

	for (i = 0; i < std::min(Volume, 100) / (100 / VolumeSteps); i++)
		Formatter.Append(m_Style.VolumeTextFill.c_str());
	for (; i < VolumeSteps; i++)
		Formatter.Append(m_Style.VolumeTextRemain.c_str());
	Formatter.AppendFormat(TEXT(" {}"), Volume);

	LOGFONT lf = *m_pOptions->GetOSDFont();
	lf.lfWidth = 0;

	if (m_VolumeOSDMaxWidth == 0) {
		lf.lfHeight = -m_Style.VolumeTextSizeMax;
		const HFONT hfont = ::CreateFontIndirect(&lf);
		const HDC hdc = ::GetDC(ClientInfo.hwndParent);
		const HGDIOBJ hOldFont = ::SelectObject(hdc, hfont);

		RECT rcFill = {};
		::DrawText(
			hdc,
			m_Style.VolumeTextFill.data(), static_cast<int>(m_Style.VolumeTextFill.length()),
			&rcFill,
			DT_NOPREFIX | DT_SINGLELINE | DT_CALCRECT);

		RECT rcRemain = {};
		::DrawText(
			hdc,
			m_Style.VolumeTextRemain.data(), static_cast<int>(m_Style.VolumeTextRemain.length()),
			&rcRemain,
			DT_NOPREFIX | DT_SINGLELINE | DT_CALCRECT);

		RECT rcPercentage = {};
		::DrawText(hdc, TEXT(" 100"), 4, &rcPercentage, DT_NOPREFIX | DT_SINGLELINE | DT_CALCRECT);

		::SelectObject(hdc, hOldFont);
		::ReleaseDC(ClientInfo.hwndParent, hdc);
		::DeleteObject(hfont);

		m_VolumeOSDMaxWidth = std::max(rcFill.right, rcRemain.right) * VolumeSteps + rcPercentage.right;
	}

	if (!m_pOptions->GetPseudoOSD() && !ClientInfo.fForcePseudoOSD
			&& pViewer->IsDrawTextSupported()) {
		RECT rcSrc;

		if (pViewer->GetSourceRect(&rcSrc)) {
			const int FontSize = std::clamp<int>(
				::MulDiv(
					(rcSrc.right - rcSrc.left) - m_Style.VolumeMargin.Horz(),
					m_Style.VolumeTextSizeMax * m_Style.VolumeHorizontalScale,
					m_VolumeOSDMaxWidth * 100),
				m_Style.VolumeTextSizeMin, m_Style.VolumeTextSizeMax);
			lf.lfHeight = -FontSize;
			lf.lfQuality = NONANTIALIASED_QUALITY;
			const HFONT hfont = ::CreateFontIndirect(&lf);
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
		const int FontSize = std::clamp<int>(
			::MulDiv(
				(ClientInfo.ClientRect.right - ClientInfo.ClientRect.left) - m_Style.VolumeMargin.Horz(),
				m_Style.VolumeTextSizeMax * m_Style.VolumeHorizontalScale,
				m_VolumeOSDMaxWidth * 100),
			m_Style.VolumeTextSizeMin, m_Style.VolumeTextSizeMax);
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


bool COSDManager::ShowEventInfoOSD(const LibISDB::EventInfo &EventInfo, EventInfoOSDFlag Flags)
{
	if (m_pEventHandler == nullptr)
		return false;

	OSDClientInfo ClientInfo;
	ClientInfo.fForcePseudoOSD = true;
	if (!m_pEventHandler->GetOSDClientInfo(&ClientInfo))
		return false;
	RECT Rect = ClientInfo.ClientRect;
	if (!m_EventInfoOSD.AdjustPosition(&Rect))
		return false;

	const LOGFONT &lf = m_pOptions->GetEventInfoOSDFont();
	m_EventInfoOSD.SetFont(lf);
	m_EventInfoOSD.SetTitleFont(lf);

	CEventInfoOSD::ColorScheme ColorScheme;
	ColorScheme.Back.Alpha = static_cast<BYTE>(m_pOptions->GetEventInfoOSDOpacity() * 255 / 100);
	m_EventInfoOSD.SetColorScheme(ColorScheme);

	m_EventInfoOSD.SetEventInfo(&EventInfo);

	m_EventInfoOSD.SetPosition(Rect);

	if (!!(Flags & EventInfoOSDFlag::Manual)) {
		m_EventInfoOSDFlags |= EventInfoOSDFlag::Manual;
		m_EventInfoOSDFlags &= ~EventInfoOSDFlag::Auto;
	} else if (!!(Flags & EventInfoOSDFlag::Auto)) {
		m_EventInfoOSDFlags |= EventInfoOSDFlag::Auto;
		m_EventInfoOSDFlags &= ~EventInfoOSDFlag::Manual;
	}

	return m_EventInfoOSD.Show(
		ClientInfo.hwndParent,
		!!(m_EventInfoOSDFlags & EventInfoOSDFlag::Manual)
				&& m_pOptions->GetEventInfoOSDManualShowNoAutoHide() ?
			0U :
			std::min(m_pOptions->GetEventInfoOSDDuration(), std::numeric_limits<unsigned int>::max() / 1000U) * 1000U);
}


void COSDManager::HideEventInfoOSD()
{
	m_EventInfoOSD.Hide();
}


bool COSDManager::IsEventInfoOSDVisible() const
{
	return m_EventInfoOSD.IsVisible();
}


bool COSDManager::IsEventInfoOSDCreated() const
{
	return m_EventInfoOSD.IsCreated();
}


void COSDManager::OnOSDFontChanged()
{
	m_VolumeOSDMaxWidth = 0;
}


void COSDManager::OnEventInfoOSDFontChanged()
{
	if (m_EventInfoOSD.IsCreated()) {
		const LOGFONT &lf = m_pOptions->GetEventInfoOSDFont();
		m_EventInfoOSD.SetFont(lf);
		m_EventInfoOSD.SetTitleFont(lf);

		m_EventInfoOSD.Update();
	}
}


bool COSDManager::CompositeText(
	LPCTSTR pszText, const RECT &rcClient, int LeftOffset, DWORD FadeTime)
{
	const CAppMain &App = GetAppClass();
	LibISDB::ViewerFilter *pViewer = App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();

	if (pViewer == nullptr)
		return false;

	RECT rcSrc;
	if (!pViewer->GetSourceRect(&rcSrc))
		return false;

	LOGFONT lf = *m_pOptions->GetOSDFont();
	const int FontSize = std::clamp<int>(
		(rcSrc.right - rcSrc.left) / m_Style.CompositeTextSizeRatio,
		m_Style.CompositeTextSizeMin, m_Style.CompositeTextSizeMax);
	lf.lfHeight = -FontSize;
	lf.lfWidth = 0;
	lf.lfQuality = NONANTIALIASED_QUALITY;
	const HFONT hfont = ::CreateFontIndirect(&lf);

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

	const int FontSize = std::clamp<int>(
		(ClientInfo.ClientRect.right - ClientInfo.ClientRect.left) / m_Style.TextSizeRatio,
		m_Style.TextSizeMin, m_Style.TextSizeMax);
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

	const bool fSingleLine = (sz.cy < FontSize * 3 / 2);
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
		std::max(static_cast<int>(sz.cy), ImageHeight));

	return true;
}


void COSDManager::SetStyle(const Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);
	m_VolumeOSDMaxWidth = 0;
	m_EventInfoOSD.SetStyle(pStyleManager);
}


void COSDManager::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	m_Style.NormalizeStyle(pStyleManager, pStyleScaling);
	m_VolumeOSDMaxWidth = 0;
	m_EventInfoOSD.NormalizeStyle(pStyleManager, pStyleScaling);
}




void COSDManager::OSDStyle::SetStyle(const Style::CStyleManager *pStyleManager)
{
	Style::IntValue Value;

	*this = OSDStyle();
	pStyleManager->Get(TEXT("osd.margin"), &Margin);
	if (pStyleManager->Get(TEXT("osd.text-size-ratio"), &Value) && Value.Value > 0)
		TextSizeRatio = Value;
	pStyleManager->Get(TEXT("osd.text-size-min"), &TextSizeMin);
	pStyleManager->Get(TEXT("osd.text-size-max"), &TextSizeMax);
	if (pStyleManager->Get(TEXT("osd.composite-text-size-ratio"), &Value) && Value.Value > 0)
		CompositeTextSizeRatio = Value;
	pStyleManager->Get(TEXT("osd.composite-text-size-min"), &CompositeTextSizeMin);
	pStyleManager->Get(TEXT("osd.composite-text-size-max"), &CompositeTextSizeMax);
	pStyleManager->Get(TEXT("channel-osd.logo"), &LogoSize);
	pStyleManager->Get(TEXT("channel-osd.logo.effect"), &LogoEffect);
	pStyleManager->Get(TEXT("channel-osd.animation"), &fChannelAnimation);
	pStyleManager->Get(TEXT("volume-osd.margin"), &VolumeMargin);
	pStyleManager->Get(TEXT("volume-osd.text-size-min"), &VolumeTextSizeMin);
	pStyleManager->Get(TEXT("volume-osd.text-size-max"), &VolumeTextSizeMax);
	pStyleManager->Get(TEXT("volume-osd.horizontal-scale"), &VolumeHorizontalScale);
	pStyleManager->Get(TEXT("volume-osd.steps"), &VolumeSteps);
	pStyleManager->Get(TEXT("volume-osd.text.fill"), &VolumeTextFill);
	pStyleManager->Get(TEXT("volume-osd.text.remain"), &VolumeTextRemain);
}


void COSDManager::OSDStyle::NormalizeStyle(
	const Style::CStyleManager *pStyleManager,
	const Style::CStyleScaling *pStyleScaling)
{
	pStyleScaling->ToPixels(&TextSizeMin);
	pStyleScaling->ToPixels(&TextSizeMax);
	pStyleScaling->ToPixels(&CompositeTextSizeMin);
	pStyleScaling->ToPixels(&CompositeTextSizeMax);
	pStyleScaling->ToPixels(&Margin);
	pStyleScaling->ToPixels(&LogoSize);
	pStyleScaling->ToPixels(&VolumeMargin);
	pStyleScaling->ToPixels(&VolumeTextSizeMin);
	pStyleScaling->ToPixels(&VolumeTextSizeMax);
}


} // namespace TVTest
