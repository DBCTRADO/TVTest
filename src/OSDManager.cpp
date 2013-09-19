#include "stdafx.h"
#include "TVTest.h"
#include "OSDManager.h"
#include "AppMain.h"
#include "LogoManager.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




COSDManager::COSDManager(const COSDOptions *pOptions)
	: m_pOptions(pOptions)
	, m_pEventHandler(NULL)
{
}


COSDManager::~COSDManager()
{
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
	GetAppClass().GetCoreEngine()->m_DtvEngine.m_MediaViewer.ClearOSD();
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
	CCoreEngine &CoreEngine=*App.GetCoreEngine();

	HWND hwnd;
	RECT rc;
	bool fForcePseudoOSD=false;
	if (!m_pEventHandler->GetOSDWindow(&hwnd,&rc,&fForcePseudoOSD))
		return false;
	if ((Flags & SHOW_PSEUDO)!=0)
		fForcePseudoOSD=true;

	DWORD FadeTime;
	if ((Flags & SHOW_NO_FADE)!=0)
		FadeTime=0;
	else
		FadeTime=m_pOptions->GetFadeTime();

	if (!m_pOptions->GetPseudoOSD() && !fForcePseudoOSD
			&& CoreEngine.m_DtvEngine.m_MediaViewer.IsDrawTextSupported()) {
		RECT rcSrc;
		if (CoreEngine.m_DtvEngine.m_MediaViewer.GetSourceRect(&rcSrc)) {
			LOGFONT lf;
			HFONT hfont;

			lf=*m_pOptions->GetOSDFont();
			lf.lfHeight=(rcSrc.right-rcSrc.left)/20;
			lf.lfWidth=0;
			lf.lfQuality=NONANTIALIASED_QUALITY;
			hfont=::CreateFontIndirect(&lf);
			int Rate,Factor;
			if (!App.GetUICore()->GetZoomRate(&Rate,&Factor)) {
				if ((rc.right-rc.left)/(rcSrc.right-rcSrc.left)<
						(rc.bottom-rc.top)/(rcSrc.bottom-rcSrc.top)) {
					Rate=rc.right-rc.left;
					Factor=rcSrc.right-rcSrc.left;
				} else {
					Rate=rc.bottom-rc.top;
					Factor=rcSrc.bottom-rcSrc.top;
				}
			}
			if (Rate!=0) {
				rcSrc.left+=8*Factor/Rate;
				rcSrc.top+=(rc.top+8)*Factor/Rate;
			} else {
				rcSrc.left+=16;
				rcSrc.top+=48;
			}
			if (CoreEngine.m_DtvEngine.m_MediaViewer.DrawText(pszText,
					rcSrc.left,rcSrc.top,hfont,
					m_pOptions->GetTextColor(),m_pOptions->GetOpacity())) {
				if (FadeTime>0)
					m_pEventHandler->SetOSDHideTimer(FadeTime);
			}
			::DeleteObject(hfont);
		}
	} else {
		int TextHeight=max((rc.right-rc.left)/24,12);
		LOGFONT lf;
		SIZE sz;

		lf=*m_pOptions->GetOSDFont();
		lf.lfHeight=-TextHeight;
		m_OSD.Create(hwnd,m_pOptions->GetLayeredWindow());
		//m_OSD.SetTextHeight(TextHeight);
		m_OSD.SetFont(lf);
		m_OSD.SetText(pszText);
		m_OSD.CalcTextSize(&sz);
		m_OSD.SetPosition(rc.left+8,rc.top+8,sz.cx,sz.cy);
		m_OSD.SetTextColor(m_pOptions->GetTextColor());
		m_OSD.SetOpacity(m_pOptions->GetOpacity());
		m_OSD.Show(FadeTime,false);
	}

	return true;
}


void COSDManager::HideOSD()
{
	m_OSD.Hide();
}


bool COSDManager::ShowChannelOSD(const CChannelInfo *pInfo,bool fChanging)
{
	if (m_pEventHandler==NULL || pInfo==NULL)
		return false;

	CAppMain &App=GetAppClass();
	CCoreEngine &CoreEngine=*App.GetCoreEngine();

	HWND hwnd;
	RECT rc;
	bool fForcePseudoOSD=false;
	if (!m_pEventHandler->GetOSDWindow(&hwnd,&rc,&fForcePseudoOSD))
		return false;

	COSDOptions::ChannelChangeType ChangeType=m_pOptions->GetChannelChangeType();

	HBITMAP hbmLogo=NULL;
	int LogoWidth=0,LogoHeight=0;
	unsigned int ImageEffect=0;
	if (ChangeType!=COSDOptions::CHANNELCHANGE_TEXTONLY) {
		hbmLogo=App.GetLogoManager()->GetAssociatedLogoBitmap(
			pInfo->GetNetworkID(),pInfo->GetServiceID(),CLogoManager::LOGOTYPE_BIG);
		if (hbmLogo!=NULL) {
#ifndef TVH264_FOR_1SEG
			LogoHeight=36;
#else
			LogoHeight=18;
#endif
			LogoWidth=LogoHeight*16/9;
			ImageEffect=fChanging?
				CPseudoOSD::IMAGEEFFECT_DARK:CPseudoOSD::IMAGEEFFECT_GLOSS;
		}

		if (ChangeType==COSDOptions::CHANNELCHANGE_LOGOONLY && hbmLogo!=NULL) {
			m_OSD.Create(hwnd,m_pOptions->GetLayeredWindow());
			m_OSD.SetImage(hbmLogo,ImageEffect);
			m_OSD.SetPosition(rc.left+8,rc.top+8,LogoWidth,LogoHeight);
			m_OSD.SetOpacity(m_pOptions->GetOpacity());
			m_OSD.Show(m_pOptions->GetFadeTime(),
					   !fChanging && !m_OSD.IsVisible());
			return true;
		}
	}

	TCHAR szText[4+MAX_CHANNEL_NAME];
	int Length=0;
	if (pInfo->GetChannelNo()!=0)
		Length=StdUtil::snprintf(szText,lengthof(szText),TEXT("%d "),pInfo->GetChannelNo());
	StdUtil::snprintf(szText+Length,lengthof(szText)-Length,TEXT("%s"),pInfo->GetName());

	if (!m_pOptions->GetPseudoOSD() && !fForcePseudoOSD
			&& CoreEngine.m_DtvEngine.m_MediaViewer.IsDrawTextSupported()) {
		if (hbmLogo!=NULL) {
			m_OSD.Create(hwnd,m_pOptions->GetLayeredWindow());
			m_OSD.SetImage(hbmLogo,ImageEffect);
			m_OSD.SetPosition(rc.left+8,rc.top+8,LogoWidth,LogoHeight);
			m_OSD.SetOpacity(m_pOptions->GetOpacity());
			m_OSD.Show(m_pOptions->GetFadeTime(),
					   !fChanging && !m_OSD.IsVisible());
		}

		if (ChangeType!=COSDOptions::CHANNELCHANGE_LOGOONLY) {
			RECT rcSrc;
			if (CoreEngine.m_DtvEngine.m_MediaViewer.GetSourceRect(&rcSrc)) {
				LOGFONT lf;
				HFONT hfont;

				lf=*m_pOptions->GetOSDFont();
				lf.lfHeight=(rcSrc.right-rcSrc.left)/20;
				lf.lfWidth=0;
				lf.lfQuality=NONANTIALIASED_QUALITY;
				hfont=::CreateFontIndirect(&lf);
				int Rate,Factor;
				if (!App.GetUICore()->GetZoomRate(&Rate,&Factor)) {
					if ((rc.right-rc.left)/(rcSrc.right-rcSrc.left)<
							(rc.bottom-rc.top)/(rcSrc.bottom-rcSrc.top)) {
						Rate=rc.right-rc.left;
						Factor=rcSrc.right-rcSrc.left;
					} else {
						Rate=rc.bottom-rc.top;
						Factor=rcSrc.bottom-rcSrc.top;
					}
				}
				if (Rate!=0) {
					rcSrc.left+=8*Factor/Rate;
					rcSrc.top+=(rc.top+8)*Factor/Rate;
					if (hbmLogo!=NULL)
						rcSrc.left+=LogoWidth*Factor/Rate;
				} else {
					rcSrc.left+=16;
					rcSrc.top+=48;
				}
				if (CoreEngine.m_DtvEngine.m_MediaViewer.DrawText(szText,
						rcSrc.left,rcSrc.top,hfont,
						m_pOptions->GetTextColor(),m_pOptions->GetOpacity())) {
					if (m_pOptions->GetFadeTime()>0)
						m_pEventHandler->SetOSDHideTimer(m_pOptions->GetFadeTime());
				}
				::DeleteObject(hfont);
			}
		}
	} else {
		int TextHeight=max((rc.right-rc.left)/24,12);
		LOGFONT lf;
		SIZE sz;
		COLORREF cr;

		lf=*m_pOptions->GetOSDFont();
		lf.lfHeight=-TextHeight;
		m_OSD.Create(hwnd,m_pOptions->GetLayeredWindow());
		//m_OSD.SetTextHeight(TextHeight);
		m_OSD.SetFont(lf);
		m_OSD.SetText(szText,hbmLogo,LogoWidth,LogoHeight,ImageEffect);
		m_OSD.CalcTextSize(&sz);
		m_OSD.SetPosition(rc.left+8,rc.top+8,
						  sz.cx+8+LogoWidth,max(sz.cy+8,LogoHeight));
		if (fChanging)
			cr=MixColor(m_pOptions->GetTextColor(),RGB(0,0,0),160);
		else
			cr=m_pOptions->GetTextColor();
		m_OSD.SetTextColor(cr);
		m_OSD.SetOpacity(m_pOptions->GetOpacity());
		m_OSD.Show(m_pOptions->GetFadeTime(),
				   !fChanging && !m_OSD.IsVisible());
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
	CCoreEngine &CoreEngine=*App.GetCoreEngine();

	HWND hwnd;
	RECT rc;
	bool fForcePseudoOSD=false;
	if (!m_pEventHandler->GetOSDWindow(&hwnd,&rc,&fForcePseudoOSD))
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
	if (!m_pOptions->GetPseudoOSD() && !fForcePseudoOSD
			&& CoreEngine.m_DtvEngine.m_MediaViewer.IsDrawTextSupported()) {
		RECT rcSrc;

		if (CoreEngine.m_DtvEngine.m_MediaViewer.GetSourceRect(&rcSrc)) {
			LOGFONT lf;
			HFONT hfont;

			lf=*m_pOptions->GetOSDFont();
			lf.lfHeight=(rcSrc.right-rcSrc.left)/(VolumeSteps*2);
			lf.lfQuality=NONANTIALIASED_QUALITY;
			hfont=::CreateFontIndirect(&lf);
			rcSrc.left+=16;
			rcSrc.top=rcSrc.bottom-(lf.lfHeight+16);
			if (CoreEngine.m_DtvEngine.m_MediaViewer.DrawText(szText,
					rcSrc.left,rcSrc.top,hfont,
					m_pOptions->GetTextColor(),m_pOptions->GetOpacity())) {
				if (m_pOptions->GetFadeTime()>0)
					m_pEventHandler->SetOSDHideTimer(m_pOptions->GetFadeTime());
			}
			::DeleteObject(hfont);
		}
	} else {
		int TextHeight=CLAMP((int)(rc.right-rc.left-32)/VolumeSteps,6,16);
		LOGFONT lf;
		SIZE sz;

		lf=*m_pOptions->GetOSDFont();
		lf.lfHeight=-TextHeight;
		m_VolumeOSD.Create(hwnd,m_pOptions->GetLayeredWindow());
		//m_VolumeOSD.SetTextHeight(TextHeight);
		m_VolumeOSD.SetFont(lf);
		m_VolumeOSD.SetText(szText);
		m_VolumeOSD.CalcTextSize(&sz);
		m_VolumeOSD.SetPosition(rc.left+8,rc.bottom-sz.cy-8,sz.cx,sz.cy);
		m_VolumeOSD.SetTextColor(m_pOptions->GetTextColor());
		m_VolumeOSD.SetOpacity(m_pOptions->GetOpacity());
		m_VolumeOSD.Show(m_pOptions->GetFadeTime());
	}

	return true;
}


void COSDManager::HideVolumeOSD()
{
	m_VolumeOSD.Hide();
}
