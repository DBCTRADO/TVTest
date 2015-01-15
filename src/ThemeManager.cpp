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


#define GRADIENT_SOLID_FLAG 0x1000
#define GRADIENT_SOLID(Color) ((Color) | GRADIENT_SOLID_FLAG)
#define GRADIENT_IS_SOLID(Gradient) (((Gradient) & GRADIENT_SOLID_FLAG)!=0)
#define GRADIENT_GET_SOLID(Gradient) ((Gradient) & 0x0FFF)


const CThemeManager::StyleInfo CThemeManager::m_StyleList[NUM_STYLES] = {
	{
		TEXT("screen"),
		-1,
		CColorScheme::BORDER_SCREEN,
		-1
	},
	{
		TEXT("window.frame"),
		GRADIENT_SOLID(CColorScheme::COLOR_WINDOWFRAMEBACK),
		CColorScheme::BORDER_WINDOWFRAME,
		-1
	},
	{
		TEXT("status-bar"),
		-1,
		CColorScheme::BORDER_STATUS,
		-1
	},
	{
		TEXT("status-bar.item"),
		CColorScheme::GRADIENT_STATUSBACK,
		CColorScheme::BORDER_STATUSITEM,
		CColorScheme::COLOR_STATUSTEXT
	},
	{
		TEXT("status-bar.item.bottom"),
		CColorScheme::GRADIENT_STATUSBOTTOMITEMBACK,
		CColorScheme::BORDER_STATUSBOTTOMITEM,
		CColorScheme::COLOR_STATUSBOTTOMITEMTEXT
	},
	{
		TEXT("status-bar.item.hot"),
		CColorScheme::GRADIENT_STATUSHIGHLIGHTBACK,
		CColorScheme::BORDER_STATUSHIGHLIGHT,
		CColorScheme::COLOR_STATUSHIGHLIGHTTEXT
	},
	{
		TEXT("status-bar.event.progress"),
		CColorScheme::GRADIENT_STATUSEVENTPROGRESSBACK,
		CColorScheme::BORDER_STATUSEVENTPROGRESS,
		-1
	},
	{
		TEXT("status-bar.event.progress.elapsed"),
		CColorScheme::GRADIENT_STATUSEVENTPROGRESSELAPSED,
		CColorScheme::BORDER_STATUSEVENTPROGRESSELAPSED,
		-1
	},
	{
		TEXT("title-bar"),
		-1,
		CColorScheme::BORDER_TITLEBAR,
		-1
	},
	{
		TEXT("title-bar.caption"),
		CColorScheme::GRADIENT_TITLEBARBACK,
		CColorScheme::BORDER_TITLEBARCAPTION,
		CColorScheme::COLOR_TITLEBARTEXT
	},
	{
		TEXT("title-bar.icon"),
		CColorScheme::GRADIENT_TITLEBARICON,
		CColorScheme::BORDER_TITLEBARICON,
		CColorScheme::COLOR_TITLEBARICON
	},
	{
		TEXT("title-bar.icon.hot"),
		CColorScheme::GRADIENT_TITLEBARHIGHLIGHTBACK,
		CColorScheme::BORDER_TITLEBARHIGHLIGHT,
		CColorScheme::COLOR_TITLEBARHIGHLIGHTICON
	},
	{
		TEXT("side-bar"),
		-1,
		CColorScheme::BORDER_SIDEBAR,
		-1
	},
	{
		TEXT("side-bar.item"),
		CColorScheme::GRADIENT_SIDEBARBACK,
		CColorScheme::BORDER_SIDEBARITEM,
		CColorScheme::COLOR_SIDEBARICON
	},
	{
		TEXT("side-bar.item.hot"),
		CColorScheme::GRADIENT_SIDEBARHIGHLIGHTBACK,
		CColorScheme::BORDER_SIDEBARHIGHLIGHT,
		CColorScheme::COLOR_SIDEBARHIGHLIGHTICON
	},
	{
		TEXT("side-bar.item.checked"),
		CColorScheme::GRADIENT_SIDEBARCHECKBACK,
		CColorScheme::BORDER_SIDEBARCHECK,
		CColorScheme::COLOR_SIDEBARCHECKICON
	},
	{
		TEXT("panel.tab"),
		CColorScheme::GRADIENT_PANELTABBACK,
		CColorScheme::BORDER_PANEL_TAB,
		CColorScheme::COLOR_PANELTABTEXT
	},
	{
		TEXT("panel.tab.current"),
		CColorScheme::GRADIENT_PANELCURTABBACK,
		CColorScheme::BORDER_PANEL_CURTAB,
		CColorScheme::COLOR_PANELCURTABTEXT
	},
	{
		TEXT("panel.tab.margin"),
		CColorScheme::GRADIENT_PANELTABMARGIN,
		CColorScheme::BORDER_PANEL_TABMARGIN,
		-1
	},
	{
		TEXT("panel.title"),
		CColorScheme::GRADIENT_PANELTITLEBACK,
		CColorScheme::BORDER_PANEL_TITLE,
		CColorScheme::COLOR_PANELTITLETEXT
	},
	{
		TEXT("panel.content"),
		GRADIENT_SOLID(CColorScheme::COLOR_PANELBACK),
		-1,
		CColorScheme::COLOR_PANELTEXT
	},
	{
		TEXT("program-list-panel.event"),
		CColorScheme::GRADIENT_PROGRAMLISTPANEL_EVENTBACK,
		CColorScheme::BORDER_PROGRAMLISTPANEL_EVENT,
		CColorScheme::COLOR_PROGRAMLISTPANEL_EVENTTEXT
	},
	{
		TEXT("program-list-panel.event.current"),
		CColorScheme::GRADIENT_PROGRAMLISTPANEL_CUREVENTBACK,
		CColorScheme::BORDER_PROGRAMLISTPANEL_CUREVENT,
		CColorScheme::COLOR_PROGRAMLISTPANEL_CUREVENTTEXT
	},
	{
		TEXT("program-list-panel.title"),
		CColorScheme::GRADIENT_PROGRAMLISTPANEL_TITLEBACK,
		CColorScheme::BORDER_PROGRAMLISTPANEL_TITLE,
		CColorScheme::COLOR_PROGRAMLISTPANEL_TITLETEXT
	},
	{
		TEXT("program-list-panel.title.current"),
		CColorScheme::GRADIENT_PROGRAMLISTPANEL_CURTITLEBACK,
		CColorScheme::BORDER_PROGRAMLISTPANEL_CURTITLE,
		CColorScheme::COLOR_PROGRAMLISTPANEL_CURTITLETEXT
	},
	{
		TEXT("channel-list-panel.channel-name"),
		CColorScheme::GRADIENT_CHANNELPANEL_CHANNELNAMEBACK,
		CColorScheme::BORDER_CHANNELPANEL_CHANNELNAME,
		CColorScheme::COLOR_CHANNELPANEL_CHANNELNAMETEXT
	},
	{
		TEXT("channel-list-panel.channel-name.current"),
		CColorScheme::GRADIENT_CHANNELPANEL_CURCHANNELNAMEBACK,
		CColorScheme::BORDER_CHANNELPANEL_CURCHANNELNAME,
		CColorScheme::COLOR_CHANNELPANEL_CURCHANNELNAMETEXT
	},
	{
		TEXT("channel-list-panel.event-name"),
		CColorScheme::GRADIENT_CHANNELPANEL_EVENTNAMEBACK1,
		CColorScheme::BORDER_CHANNELPANEL_EVENTNAME1,
		CColorScheme::COLOR_CHANNELPANEL_EVENTNAME1TEXT
	},
	{
		TEXT("channel-list-panel.event-name.odd"),
		CColorScheme::GRADIENT_CHANNELPANEL_EVENTNAMEBACK2,
		CColorScheme::BORDER_CHANNELPANEL_EVENTNAME2,
		CColorScheme::COLOR_CHANNELPANEL_EVENTNAME2TEXT
	},
	{
		TEXT("channel-list-panel.event-name.current"),
		CColorScheme::GRADIENT_CHANNELPANEL_CUREVENTNAMEBACK1,
		CColorScheme::BORDER_CHANNELPANEL_CUREVENTNAME1,
		CColorScheme::COLOR_CHANNELPANEL_CUREVENTNAME1TEXT
	},
	{
		TEXT("channel-list-panel.event-name.current.odd"),
		CColorScheme::GRADIENT_CHANNELPANEL_CUREVENTNAMEBACK2,
		CColorScheme::BORDER_CHANNELPANEL_CUREVENTNAME2,
		CColorScheme::COLOR_CHANNELPANEL_CUREVENTNAME2TEXT
	},
	{
		TEXT("channel-list-panel.featured-mark"),
		CColorScheme::GRADIENT_CHANNELPANEL_FEATUREDMARK,
		CColorScheme::BORDER_CHANNELPANEL_FEATUREDMARK,
		-1
	},
	{
		TEXT("channel-list-panel.progress"),
		CColorScheme::GRADIENT_CHANNELPANEL_PROGRESS,
		CColorScheme::BORDER_CHANNELPANEL_PROGRESS,
		-1
	},
	{
		TEXT("channel-list-panel.progress.current"),
		CColorScheme::GRADIENT_CHANNELPANEL_CURPROGRESS,
		CColorScheme::BORDER_CHANNELPANEL_CURPROGRESS,
		-1
	},
	{
		TEXT("control-panel.item"),
		CColorScheme::GRADIENT_CONTROLPANELBACK,
		CColorScheme::BORDER_CONTROLPANELITEM,
		CColorScheme::COLOR_CONTROLPANELTEXT
	},
	{
		TEXT("control-panel.item.hot"),
		CColorScheme::GRADIENT_CONTROLPANELHIGHLIGHTBACK,
		CColorScheme::BORDER_CONTROLPANELHIGHLIGHTITEM,
		CColorScheme::COLOR_CONTROLPANELHIGHLIGHTTEXT
	},
	{
		TEXT("notification-bar"),
		CColorScheme::GRADIENT_NOTIFICATIONBARBACK,
		-1,
		CColorScheme::COLOR_NOTIFICATIONBARTEXT
	},
	{
		TEXT("program-guide.event.featured-mark"),
		CColorScheme::GRADIENT_PROGRAMGUIDE_FEATUREDMARK,
		CColorScheme::BORDER_PROGRAMGUIDE_FEATUREDMARK,
		-1
	},
	{
		TEXT("program-guide.header.channel-name"),
		CColorScheme::GRADIENT_PROGRAMGUIDECHANNELBACK,
		-1,
		CColorScheme::COLOR_PROGRAMGUIDE_CHANNELTEXT
	},
	{
		TEXT("program-guide.header.channel-name.current"),
		CColorScheme::GRADIENT_PROGRAMGUIDECURCHANNELBACK,
		-1,
		CColorScheme::COLOR_PROGRAMGUIDE_CURCHANNELTEXT
	},
	{
		TEXT("program-guide.time-bar"),
		CColorScheme::GRADIENT_PROGRAMGUIDETIMEBACK,
		-1,
		CColorScheme::COLOR_PROGRAMGUIDE_TIMETEXT
	},
	{
		TEXT("program-guide.time-bar.time-0-2"),
		CColorScheme::GRADIENT_PROGRAMGUIDETIME0TO2BACK,
		-1,
		CColorScheme::COLOR_PROGRAMGUIDE_TIMETEXT
	},
	{
		TEXT("program-guide.time-bar.time-3-5"),
		CColorScheme::GRADIENT_PROGRAMGUIDETIME3TO5BACK,
		-1,
		CColorScheme::COLOR_PROGRAMGUIDE_TIMETEXT
	},
	{
		TEXT("program-guide.time-bar.time-6-8"),
		CColorScheme::GRADIENT_PROGRAMGUIDETIME6TO8BACK,
		-1,
		CColorScheme::COLOR_PROGRAMGUIDE_TIMETEXT
	},
	{
		TEXT("program-guide.time-bar.time-9-11"),
		CColorScheme::GRADIENT_PROGRAMGUIDETIME9TO11BACK,
		-1,
		CColorScheme::COLOR_PROGRAMGUIDE_TIMETEXT
	},
	{
		TEXT("program-guide.time-bar.time-12-14"),
		CColorScheme::GRADIENT_PROGRAMGUIDETIME12TO14BACK,
		-1,
		CColorScheme::COLOR_PROGRAMGUIDE_TIMETEXT
	},
	{
		TEXT("program-guide.time-bar.time-15-17"),
		CColorScheme::GRADIENT_PROGRAMGUIDETIME15TO17BACK,
		-1,
		CColorScheme::COLOR_PROGRAMGUIDE_TIMETEXT
	},
	{
		TEXT("program-guide.time-bar.time-18-20"),
		CColorScheme::GRADIENT_PROGRAMGUIDETIME18TO20BACK,
		-1,
		CColorScheme::COLOR_PROGRAMGUIDE_TIMETEXT
	},
	{
		TEXT("program-guide.time-bar.time-21-23"),
		CColorScheme::GRADIENT_PROGRAMGUIDETIME21TO23BACK,
		-1,
		CColorScheme::COLOR_PROGRAMGUIDE_TIMETEXT
	},
	{
		TEXT("program-guide.status-bar"),
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

	if (Info.Gradient>=0) {
		if (GRADIENT_IS_SOLID(Info.Gradient)) {
			pStyle->Type=FILL_SOLID;
			pStyle->Solid.Color=ThemeColor(m_pColorScheme->GetColor(GRADIENT_GET_SOLID(Info.Gradient)));
		} else {
			pStyle->Type=FILL_GRADIENT;
			m_pColorScheme->GetGradientStyle(Info.Gradient,&pStyle->Gradient);
			if (pStyle->Gradient.IsSolid()) {
				pStyle->Type=FILL_SOLID;
				pStyle->Solid.Color=pStyle->Gradient.Color1;
			}
		}
	} else {
		pStyle->Type=FILL_NONE;
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


LPCTSTR CThemeManager::GetStyleName(int Type) const
{
	if (Type<0 || Type>=NUM_STYLES)
		return nullptr;
	return m_StyleList[Type].pszName;
}


int CThemeManager::ParseStyleName(LPCTSTR pszName) const
{
	if (IsStringEmpty(pszName))
		return -1;

	for (int i=0;i<lengthof(m_StyleList);i++) {
		if (::lstrcmpi(m_StyleList[i].pszName,pszName)==0)
			return i;
	}

	return -1;
}


}	// namespace Theme

}	// namespace TVTest
