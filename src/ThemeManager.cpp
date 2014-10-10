#include "stdafx.h"
#include "TVTest.h"
#include "ThemeManager.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


namespace TVTest
{

namespace Theme
{


const CThemeManager::StyleInfo CThemeManager::m_StyleList[NUM_STYLES] = {
	{
		-1,
		CColorScheme::BORDER_SCREEN,
		-1
	},
	{
		-1,
		CColorScheme::BORDER_STATUS,
		-1
	},
	{
		CColorScheme::GRADIENT_STATUSBACK,
		CColorScheme::BORDER_STATUSITEM,
		CColorScheme::COLOR_STATUSTEXT
	},
	{
		CColorScheme::GRADIENT_STATUSBOTTOMITEMBACK,
		CColorScheme::BORDER_STATUSBOTTOMITEM,
		CColorScheme::COLOR_STATUSBOTTOMITEMTEXT
	},
	{
		CColorScheme::GRADIENT_STATUSHIGHLIGHTBACK,
		CColorScheme::BORDER_STATUSHIGHLIGHT,
		CColorScheme::COLOR_STATUSHIGHLIGHTTEXT
	},
	{
		CColorScheme::GRADIENT_STATUSEVENTPROGRESSBACK,
		CColorScheme::BORDER_STATUSEVENTPROGRESS,
		-1
	},
	{
		CColorScheme::GRADIENT_STATUSEVENTPROGRESSELAPSED,
		CColorScheme::BORDER_STATUSEVENTPROGRESSELAPSED,
		-1
	},
	{
		-1,
		CColorScheme::BORDER_TITLEBAR,
		-1
	},
	{
		CColorScheme::GRADIENT_TITLEBARBACK,
		CColorScheme::BORDER_TITLEBARCAPTION,
		CColorScheme::COLOR_TITLEBARTEXT
	},
	{
		CColorScheme::GRADIENT_TITLEBARICON,
		CColorScheme::BORDER_TITLEBARICON,
		CColorScheme::COLOR_TITLEBARICON
	},
	{
		CColorScheme::GRADIENT_TITLEBARHIGHLIGHTBACK,
		CColorScheme::BORDER_TITLEBARHIGHLIGHT,
		CColorScheme::COLOR_TITLEBARHIGHLIGHTICON
	},
	{
		-1,
		CColorScheme::BORDER_SIDEBAR,
		-1
	},
	{
		CColorScheme::GRADIENT_SIDEBARBACK,
		CColorScheme::BORDER_SIDEBARITEM,
		CColorScheme::COLOR_SIDEBARICON
	},
	{
		CColorScheme::GRADIENT_SIDEBARHIGHLIGHTBACK,
		CColorScheme::BORDER_SIDEBARHIGHLIGHT,
		CColorScheme::COLOR_SIDEBARHIGHLIGHTICON
	},
	{
		CColorScheme::GRADIENT_SIDEBARCHECKBACK,
		CColorScheme::BORDER_SIDEBARCHECK,
		CColorScheme::COLOR_SIDEBARCHECKICON
	},
	{
		CColorScheme::GRADIENT_PANELTABBACK,
		CColorScheme::BORDER_PANEL_TAB,
		CColorScheme::COLOR_PANELTABTEXT
	},
	{
		CColorScheme::GRADIENT_PANELCURTABBACK,
		CColorScheme::BORDER_PANEL_CURTAB,
		CColorScheme::COLOR_PANELCURTABTEXT
	},
	{
		CColorScheme::GRADIENT_PANELTABMARGIN,
		CColorScheme::BORDER_PANEL_TABMARGIN,
		-1
	},
	{
		CColorScheme::GRADIENT_PANELTITLEBACK,
		CColorScheme::BORDER_PANEL_TITLE,
		CColorScheme::COLOR_PANELTITLETEXT
	},
	{
		CColorScheme::GRADIENT_PROGRAMLISTPANEL_EVENTBACK,
		CColorScheme::BORDER_PROGRAMLISTPANEL_EVENT,
		CColorScheme::COLOR_PROGRAMLISTPANEL_EVENTTEXT
	},
	{
		CColorScheme::GRADIENT_PROGRAMLISTPANEL_CUREVENTBACK,
		CColorScheme::BORDER_PROGRAMLISTPANEL_CUREVENT,
		CColorScheme::COLOR_PROGRAMLISTPANEL_CUREVENTTEXT
	},
	{
		CColorScheme::GRADIENT_PROGRAMLISTPANEL_TITLEBACK,
		CColorScheme::BORDER_PROGRAMLISTPANEL_TITLE,
		CColorScheme::COLOR_PROGRAMLISTPANEL_TITLETEXT
	},
	{
		CColorScheme::GRADIENT_PROGRAMLISTPANEL_CURTITLEBACK,
		CColorScheme::BORDER_PROGRAMLISTPANEL_CURTITLE,
		CColorScheme::COLOR_PROGRAMLISTPANEL_CURTITLETEXT
	},
	{
		CColorScheme::GRADIENT_CHANNELPANEL_CHANNELNAMEBACK,
		CColorScheme::BORDER_CHANNELPANEL_CHANNELNAME,
		CColorScheme::COLOR_CHANNELPANEL_CHANNELNAMETEXT
	},
	{
		CColorScheme::GRADIENT_CHANNELPANEL_CURCHANNELNAMEBACK,
		CColorScheme::BORDER_CHANNELPANEL_CURCHANNELNAME,
		CColorScheme::COLOR_CHANNELPANEL_CURCHANNELNAMETEXT
	},
	{
		CColorScheme::GRADIENT_CHANNELPANEL_EVENTNAMEBACK1,
		CColorScheme::BORDER_CHANNELPANEL_EVENTNAME1,
		CColorScheme::COLOR_CHANNELPANEL_EVENTNAME1TEXT
	},
	{
		CColorScheme::GRADIENT_CHANNELPANEL_EVENTNAMEBACK2,
		CColorScheme::BORDER_CHANNELPANEL_EVENTNAME2,
		CColorScheme::COLOR_CHANNELPANEL_EVENTNAME2TEXT
	},
	{
		CColorScheme::GRADIENT_CHANNELPANEL_CUREVENTNAMEBACK1,
		CColorScheme::BORDER_CHANNELPANEL_CUREVENTNAME1,
		CColorScheme::COLOR_CHANNELPANEL_CUREVENTNAME1TEXT
	},
	{
		CColorScheme::GRADIENT_CHANNELPANEL_CUREVENTNAMEBACK2,
		CColorScheme::BORDER_CHANNELPANEL_CUREVENTNAME2,
		CColorScheme::COLOR_CHANNELPANEL_CUREVENTNAME2TEXT
	},
	{
		CColorScheme::GRADIENT_CHANNELPANEL_FEATUREDMARK,
		CColorScheme::BORDER_CHANNELPANEL_FEATUREDMARK,
		-1
	},
	{
		CColorScheme::GRADIENT_CONTROLPANELBACK,
		CColorScheme::BORDER_CONTROLPANELITEM,
		CColorScheme::COLOR_CONTROLPANELTEXT
	},
	{
		CColorScheme::GRADIENT_CONTROLPANELHIGHLIGHTBACK,
		CColorScheme::BORDER_CONTROLPANELHIGHLIGHTITEM,
		CColorScheme::COLOR_CONTROLPANELHIGHLIGHTTEXT
	},
	{
		CColorScheme::GRADIENT_NOTIFICATIONBARBACK,
		-1,
		CColorScheme::COLOR_NOTIFICATIONBARTEXT
	},
	{
		CColorScheme::GRADIENT_PROGRAMGUIDE_FEATUREDMARK,
		CColorScheme::BORDER_PROGRAMGUIDE_FEATUREDMARK,
		-1
	},
	{
		CColorScheme::GRADIENT_PROGRAMGUIDECHANNELBACK,
		-1,
		CColorScheme::COLOR_PROGRAMGUIDE_CHANNELTEXT
	},
	{
		CColorScheme::GRADIENT_PROGRAMGUIDECURCHANNELBACK,
		-1,
		CColorScheme::COLOR_PROGRAMGUIDE_CURCHANNELTEXT
	},
	{
		CColorScheme::GRADIENT_PROGRAMGUIDETIMEBACK,
		-1,
		CColorScheme::COLOR_PROGRAMGUIDE_TIMETEXT
	},
	{
		CColorScheme::GRADIENT_PROGRAMGUIDETIME0TO2BACK,
		-1,
		CColorScheme::COLOR_PROGRAMGUIDE_TIMETEXT
	},
	{
		CColorScheme::GRADIENT_PROGRAMGUIDETIME3TO5BACK,
		-1,
		CColorScheme::COLOR_PROGRAMGUIDE_TIMETEXT
	},
	{
		CColorScheme::GRADIENT_PROGRAMGUIDETIME6TO8BACK,
		-1,
		CColorScheme::COLOR_PROGRAMGUIDE_TIMETEXT
	},
	{
		CColorScheme::GRADIENT_PROGRAMGUIDETIME9TO11BACK,
		-1,
		CColorScheme::COLOR_PROGRAMGUIDE_TIMETEXT
	},
	{
		CColorScheme::GRADIENT_PROGRAMGUIDETIME12TO14BACK,
		-1,
		CColorScheme::COLOR_PROGRAMGUIDE_TIMETEXT
	},
	{
		CColorScheme::GRADIENT_PROGRAMGUIDETIME15TO17BACK,
		-1,
		CColorScheme::COLOR_PROGRAMGUIDE_TIMETEXT
	},
	{
		CColorScheme::GRADIENT_PROGRAMGUIDETIME18TO20BACK,
		-1,
		CColorScheme::COLOR_PROGRAMGUIDE_TIMETEXT
	},
	{
		CColorScheme::GRADIENT_PROGRAMGUIDETIME21TO23BACK,
		-1,
		CColorScheme::COLOR_PROGRAMGUIDE_TIMETEXT
	},
	{
		-1,
		CColorScheme::BORDER_PROGRAMGUIDESTATUS,
		-1
	},
};


CThemeManager::CThemeManager(const CColorScheme *pColorScheme)
	: m_pColorScheme(pColorScheme)
{
}


ThemeColor CThemeManager::GetColor(int Type) const
{
	COLORREF cr=m_pColorScheme->GetColor(Type);
	if (cr==CLR_INVALID)
		return ThemeColor();
	return ThemeColor(cr);
}


ThemeColor CThemeManager::GetColor(LPCTSTR pszName) const
{
	COLORREF cr=m_pColorScheme->GetColor(pszName);
	if (cr==CLR_INVALID)
		return ThemeColor();
	return ThemeColor(cr);
}


bool CThemeManager::GetStyle(int Type,Style *pStyle) const
{
	if (Type<0 || Type>=NUM_STYLES || pStyle==nullptr)
		return false;

	GetBackgroundStyle(Type,&pStyle->Back);
	GetForegroundStyle(Type,&pStyle->Fore);

	return true;
}


bool CThemeManager::GetFillStyle(int Type,FillStyle *pStyle) const
{
	if (Type<0 || Type>=NUM_STYLES || pStyle==nullptr)
		return false;

	const StyleInfo &Info=m_StyleList[Type];

	pStyle->Type=FILL_GRADIENT;
	m_pColorScheme->GetGradientStyle(Info.Gradient,&pStyle->Gradient);
	if (pStyle->Gradient.IsSolid()) {
		pStyle->Type=FILL_SOLID;
		pStyle->Solid.Color=pStyle->Gradient.Color1;
	}

	return true;
}


bool CThemeManager::GetBorderStyle(int Type,BorderStyle *pStyle) const
{
	if (Type<0 || Type>=NUM_STYLES || pStyle==nullptr)
		return false;

	const StyleInfo &Info=m_StyleList[Type];

	if (Info.Border>=0)
		m_pColorScheme->GetBorderStyle(Info.Border,pStyle);
	else
		pStyle->Type=BORDER_NONE;

	return true;
}


bool CThemeManager::GetBackgroundStyle(int Type,BackgroundStyle *pStyle) const
{
	if (Type<0 || Type>=NUM_STYLES || pStyle==nullptr)
		return false;

	GetFillStyle(Type,&pStyle->Fill);
	GetBorderStyle(Type,&pStyle->Border);

	return true;
}


bool CThemeManager::GetForegroundStyle(int Type,ForegroundStyle *pStyle) const
{
	if (Type<0 || Type>=NUM_STYLES || pStyle==nullptr)
		return false;

	const StyleInfo &Info=m_StyleList[Type];

	if (Info.ForeColor>=0) {
		pStyle->Fill.Type=FILL_SOLID;
		pStyle->Fill.Solid.Color=GetColor(Info.ForeColor);
	} else {
		pStyle->Fill.Type=FILL_NONE;
	}

	return true;
}


}	// namespace Theme

}	// namespace TVTest
