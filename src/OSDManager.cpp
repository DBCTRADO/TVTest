#include "stdafx.h"
#include "TVTest.h"
#include "OSDManager.h"
#include "AppMain.h"
#include "LogoManager.h"
#include "Common/DebugDef.h"




COSDManager::COSDManager(const COSDOptions *pOptions)
	: m_pOptions(pOptions)
	, m_pEventHandler(NULL)
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
	m_pEventHandler=pEventHandler;
}


void COSDManager::Reset()
{
	m_OSD.Destroy();
	m_VolumeOSD.Destroy();
}


void COSDManager::ClearOSD()
{
	GetAppClass().CoreEngine.m_DtvEngine.m_MediaViewer.ClearOSD();
	m_OSD.Hide();
	m_VolumeOSD.Hide();
}


void COSDManager::OnParentMove()
{
	m_OSD.OnParentMove();
	m_VolumeOSD.OnParentMove();
}


bool COSDManager::ShowOSD(LPCTSTR pszText,unsigned int Flags)
{
	if (IsStringEmpty(pszText))
		return false;

	CAppMain &App=GetAppClass();
	CCoreEngine &CoreEngine=App.CoreEngine;

	OSDClientInfo ClientInfo;
	ClientInfo.fForcePseudoOSD=false;
	if (!m_pEventHandler->GetOSDClientInfo(&ClientInfo))
		return false;
	if ((Flags & SHOW_PSEUDO)!=0)
		ClientInfo.fForcePseudoOSD=true;

	DWORD FadeTime;
	if ((Flags & SHOW_NO_FADE)!=0)
		FadeTime=0;
	else
		FadeTime=m_pOptions->GetFadeTime();

	if (!m_pOptions->GetPseudoOSD() && !ClientInfo.fForcePseudoOSD
			&& CoreEngine.m_DtvEngine.m_MediaViewer.IsDrawTextSupported()) {
		CompositeText(pszText,ClientInfo.ClientRect,0,FadeTime);
	} else {
		int FontSize=max((ClientInfo.ClientRect.right-ClientInfo.ClientRect.left)/m_Style.TextSizeRatio,12);
		LOGFONT lf;
		SIZE sz;

		lf=*m_pOptions->GetOSDFont();
		lf.lfHeight=-FontSize;
		m_OSD.Create(ClientInfo.hwndParent,m_pOptions->GetLayeredWindow());
		//m_OSD.SetTextHeight(FontSize);
		m_OSD.SetFont(lf);
		m_OSD.SetText(pszText);
		m_OSD.CalcTextSize(&sz);
		m_OSD.SetPosition(ClientInfo.ClientRect.left+m_Style.Margin.Left,
						  ClientInfo.ClientRect.top+m_Style.Margin.Top,
						  sz.cx+FontSize/4,sz.cy);
		m_OSD.SetTextStyle(
			CPseudoOSD::TEXT_STYLE_HORZ_CENTER | CPseudoOSD::TEXT_STYLE_VERT_CENTER |
			CPseudoOSD::TEXT_STYLE_OUTLINE | CPseudoOSD::TEXT_STYLE_FILL_BACKGROUND);
		m_OSD.SetTextColor(m_pOptions->GetTextColor());
		m_OSD.Show(FadeTime,false);
	}

	return true;
}


void COSDManager::HideOSD()
{
	m_OSD.Hide();
}


bool COSDManager::ShowChannelOSD(const CChannelInfo *pInfo,LPCTSTR pszText,bool fChanging)
{
	if (m_pEventHandler==NULL || pInfo==NULL)
		return false;

	CAppMain &App=GetAppClass();
	CCoreEngine &CoreEngine=App.CoreEngine;

	OSDClientInfo ClientInfo;
	ClientInfo.fForcePseudoOSD=false;
	ClientInfo.fAnimation=m_Style.fChannelAnimation && !fChanging;
	if (!m_pEventHandler->GetOSDClientInfo(&ClientInfo))
		return false;
	if (!m_Style.fChannelAnimation || m_OSD.IsVisible())
		ClientInfo.fAnimation=false;

	COSDOptions::ChannelChangeType ChangeType=m_pOptions->GetChannelChangeType();

	HBITMAP hbmLogo=NULL;
	unsigned int ImageEffect=0;
	if (ChangeType!=COSDOptions::CHANNELCHANGE_TEXTONLY) {
		hbmLogo=App.LogoManager.GetAssociatedLogoBitmap(
			pInfo->GetNetworkID(),pInfo->GetServiceID(),CLogoManager::LOGOTYPE_BIG);
		if (hbmLogo!=NULL) {
			if (fChanging)
				ImageEffect=CPseudoOSD::IMAGEEFFECT_DARK;
			else if (TVTest::StringUtility::CompareNoCase(m_Style.LogoEffect,TEXT("gloss"))==0)
				ImageEffect=CPseudoOSD::IMAGEEFFECT_GLOSS;
		}

		if (ChangeType==COSDOptions::CHANNELCHANGE_LOGOONLY && hbmLogo!=NULL) {
			m_OSD.Create(ClientInfo.hwndParent,m_pOptions->GetLayeredWindow());
			m_OSD.SetImage(hbmLogo,ImageEffect);
			m_OSD.SetPosition(ClientInfo.ClientRect.left+m_Style.Margin.Left,
							  ClientInfo.ClientRect.top+m_Style.Margin.Top,
							  m_Style.LogoSize.Width,m_Style.LogoSize.Height);
			m_OSD.Show(m_pOptions->GetFadeTime(),ClientInfo.fAnimation);
			return true;
		}
	}

	if (!m_pOptions->GetPseudoOSD() && !ClientInfo.fForcePseudoOSD
			&& CoreEngine.m_DtvEngine.m_MediaViewer.IsDrawTextSupported()) {
		if (hbmLogo!=NULL) {
			m_OSD.Create(ClientInfo.hwndParent,m_pOptions->GetLayeredWindow());
			m_OSD.SetImage(hbmLogo,ImageEffect);
			m_OSD.SetPosition(ClientInfo.ClientRect.left+m_Style.Margin.Left,
							  ClientInfo.ClientRect.top+m_Style.Margin.Top,
							  m_Style.LogoSize.Width,m_Style.LogoSize.Height);
			m_OSD.Show(m_pOptions->GetFadeTime(),ClientInfo.fAnimation);
		}

		if (ChangeType!=COSDOptions::CHANNELCHANGE_LOGOONLY && !IsStringEmpty(pszText)) {
			CompositeText(pszText,ClientInfo.ClientRect,hbmLogo!=NULL?m_Style.LogoSize.Width:0,m_pOptions->GetFadeTime());
		}
	} else {
		int FontSize=max((ClientInfo.ClientRect.right-ClientInfo.ClientRect.left)/m_Style.TextSizeRatio,12);
		LOGFONT lf;
		SIZE sz;
		COLORREF cr;

		lf=*m_pOptions->GetOSDFont();
		lf.lfHeight=-FontSize;
		m_OSD.Create(ClientInfo.hwndParent,m_pOptions->GetLayeredWindow());
		//m_OSD.SetTextHeight(FontSize);
		m_OSD.SetFont(lf);
		m_OSD.SetText(pszText,hbmLogo,m_Style.LogoSize.Width,m_Style.LogoSize.Height,ImageEffect);
		m_OSD.CalcTextSize(&sz);
		m_OSD.SetPosition(ClientInfo.ClientRect.left+m_Style.Margin.Left,
						  ClientInfo.ClientRect.top+m_Style.Margin.Top,
						  sz.cx+FontSize/4+m_Style.LogoSize.Width,
						  max(sz.cy,m_Style.LogoSize.Height));
		m_OSD.SetTextStyle(
			CPseudoOSD::TEXT_STYLE_HORZ_CENTER | CPseudoOSD::TEXT_STYLE_VERT_CENTER |
			CPseudoOSD::TEXT_STYLE_OUTLINE | CPseudoOSD::TEXT_STYLE_FILL_BACKGROUND);
		if (fChanging)
			cr=MixColor(m_pOptions->GetTextColor(),RGB(0,0,0),160);
		else
			cr=m_pOptions->GetTextColor();
		m_OSD.SetTextColor(cr);
		m_OSD.Show(m_pOptions->GetFadeTime(),ClientInfo.fAnimation);
	}

	return true;
}


void COSDManager::HideChannelOSD()
{
	m_OSD.Hide();
}


bool COSDManager::ShowVolumeOSD(int Volume)
{
	if (m_pEventHandler==NULL)
		return false;

	CAppMain &App=GetAppClass();
	CCoreEngine &CoreEngine=App.CoreEngine;

	OSDClientInfo ClientInfo;
	ClientInfo.fForcePseudoOSD=false;
	if (!m_pEventHandler->GetOSDClientInfo(&ClientInfo))
		return false;

	static const int VolumeSteps=20;
	TCHAR szText[64];
	int i;

	szText[0]='\0';
	for (i=0;i<Volume/(100/VolumeSteps);i++)
		::lstrcat(szText,TEXT("¡"));
	for (;i<VolumeSteps;i++)
		::lstrcat(szText,TEXT(" "));
	::wsprintf(szText+::lstrlen(szText),TEXT(" %d"),Volume);

	if (!m_pOptions->GetPseudoOSD() && !ClientInfo.fForcePseudoOSD
			&& CoreEngine.m_DtvEngine.m_MediaViewer.IsDrawTextSupported()) {
		RECT rcSrc;

		if (CoreEngine.m_DtvEngine.m_MediaViewer.GetSourceRect(&rcSrc)) {
			int FontSize=((rcSrc.right-rcSrc.left)-m_Style.VolumeMargin.Horz())/(VolumeSteps*2);
			if (FontSize<m_Style.VolumeTextSizeMin)
				FontSize=m_Style.VolumeTextSizeMin;
			LOGFONT lf;
			HFONT hfont;

			lf=*m_pOptions->GetOSDFont();
			lf.lfHeight=-FontSize;
			lf.lfQuality=NONANTIALIASED_QUALITY;
			hfont=::CreateFontIndirect(&lf);
			rcSrc.left+=m_Style.VolumeMargin.Left*(rcSrc.right-rcSrc.left)/
				(ClientInfo.ClientRect.right-ClientInfo.ClientRect.left);
			rcSrc.top=rcSrc.bottom-FontSize-
				m_Style.VolumeMargin.Bottom*(rcSrc.bottom-rcSrc.top)/
					(ClientInfo.ClientRect.bottom-ClientInfo.ClientRect.top);
			if (CoreEngine.m_DtvEngine.m_MediaViewer.DrawText(szText,
					rcSrc.left,rcSrc.top,hfont,
					m_pOptions->GetTextColor(),m_pOptions->GetOpacity())) {
				if (m_pOptions->GetFadeTime()>0)
					m_pEventHandler->SetOSDHideTimer(m_pOptions->GetFadeTime());
			}
			::DeleteObject(hfont);
		}
	} else {
		int FontSize=((ClientInfo.ClientRect.right-ClientInfo.ClientRect.left)-m_Style.VolumeMargin.Horz())/(VolumeSteps*2);
		if (FontSize<m_Style.VolumeTextSizeMin)
			FontSize=m_Style.VolumeTextSizeMin;
		LOGFONT lf;
		SIZE sz;

		lf=*m_pOptions->GetOSDFont();
		lf.lfHeight=-FontSize;
		m_VolumeOSD.Create(ClientInfo.hwndParent,m_pOptions->GetLayeredWindow());
		//m_VolumeOSD.SetTextHeight(FontSize);
		m_VolumeOSD.SetFont(lf);
		m_VolumeOSD.SetText(szText);
		m_VolumeOSD.CalcTextSize(&sz);
		m_VolumeOSD.SetPosition(ClientInfo.ClientRect.left+m_Style.VolumeMargin.Left,
								ClientInfo.ClientRect.bottom-sz.cy-m_Style.VolumeMargin.Bottom,
								sz.cx+FontSize/4,sz.cy);
		m_VolumeOSD.SetTextStyle(
			CPseudoOSD::TEXT_STYLE_LEFT | CPseudoOSD::TEXT_STYLE_VERT_CENTER |
			CPseudoOSD::TEXT_STYLE_FILL_BACKGROUND);
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
	LPCTSTR pszText,const RECT &rcClient,int LeftOffset,DWORD FadeTime)
{
	CAppMain &App=GetAppClass();

	RECT rcSrc;
	if (!App.CoreEngine.m_DtvEngine.m_MediaViewer.GetSourceRect(&rcSrc))
		return false;

	LOGFONT lf=*m_pOptions->GetOSDFont();
	int FontSize=max((rcSrc.right-rcSrc.left)/m_Style.CompositeTextSizeRatio,12);
	lf.lfHeight=-FontSize;
	lf.lfWidth=0;
	lf.lfQuality=NONANTIALIASED_QUALITY;
	HFONT hfont=::CreateFontIndirect(&lf);

	int Rate,Factor;
	if (!App.UICore.GetZoomRate(&Rate,&Factor)) {
		if ((rcClient.right-rcClient.left)/(rcSrc.right-rcSrc.left)<
				(rcClient.bottom-rcClient.top)/(rcSrc.bottom-rcSrc.top)) {
			Rate=rcClient.right-rcClient.left;
			Factor=rcSrc.right-rcSrc.left;
		} else {
			Rate=rcClient.bottom-rcClient.top;
			Factor=rcSrc.bottom-rcSrc.top;
		}
	}
	if (Rate!=0) {
		rcSrc.left+=(m_Style.Margin.Left+LeftOffset)*Factor/Rate;
		rcSrc.top+=(rcClient.top+m_Style.Margin.Top)*Factor/Rate;
	} else {
		rcSrc.left+=16;
		rcSrc.top+=48;
	}
	bool fOK=false;
	if (App.CoreEngine.m_DtvEngine.m_MediaViewer.DrawText(pszText,
			rcSrc.left,rcSrc.top,hfont,
			m_pOptions->GetTextColor(),m_pOptions->GetOpacity())) {
		fOK=true;
		if (FadeTime>0)
			m_pEventHandler->SetOSDHideTimer(FadeTime);
	}
	::DeleteObject(hfont);

	return fOK;
}


void COSDManager::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);
}


void COSDManager::NormalizeStyle(
	const TVTest::Style::CStyleManager *pStyleManager,
	const TVTest::Style::CStyleScaling *pStyleScaling)
{
	m_Style.NormalizeStyle(pStyleManager,pStyleScaling);
}




COSDManager::OSDStyle::OSDStyle()
	: Margin(8)
	, TextSizeRatio(28)
	, CompositeTextSizeRatio(24)
	, LogoSize(64,36)
	, LogoEffect(TEXT("gloss"))
	, fChannelAnimation(true)
	, VolumeMargin(16)
	, VolumeTextSizeMin(10)
{
}


void COSDManager::OSDStyle::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	TVTest::Style::IntValue Value;

	*this=OSDStyle();
	pStyleManager->Get(TEXT("osd.margin"),&Margin);
	if (pStyleManager->Get(TEXT("osd.text-size-ratio"),&Value) && Value.Value>0)
		TextSizeRatio=Value;
	if (pStyleManager->Get(TEXT("osd.composite-text-size-ratio"),&Value) && Value.Value>0)
		CompositeTextSizeRatio=Value;
	pStyleManager->Get(TEXT("channel-osd.logo"),&LogoSize);
	pStyleManager->Get(TEXT("channel-osd.logo.effect"),&LogoEffect);
	pStyleManager->Get(TEXT("channel-osd.animation"),&fChannelAnimation);
	pStyleManager->Get(TEXT("volume-osd.margin"),&VolumeMargin);
	pStyleManager->Get(TEXT("volume-osd.text-size-min"),&VolumeTextSizeMin);
}


void COSDManager::OSDStyle::NormalizeStyle(
	const TVTest::Style::CStyleManager *pStyleManager,
	const TVTest::Style::CStyleScaling *pStyleScaling)
{
	pStyleScaling->ToPixels(&Margin);
	pStyleScaling->ToPixels(&LogoSize);
	pStyleScaling->ToPixels(&VolumeMargin);
	pStyleScaling->ToPixels(&VolumeTextSizeMin);
}
