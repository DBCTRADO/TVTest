#include "stdafx.h"
#include "TVTest.h"
#include "ControlPanelItems.h"
#include "AppMain.h"
#include "Menu.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




void CTunerControlItem::CalcSize(int Width,SIZE *pSize)
{
	pSize->cx=Width;
	pSize->cy=GetTextItemHeight();
}

void CTunerControlItem::Draw(HDC hdc,const RECT &Rect)
{
	const CChannelManager &ChannelManager=GetAppClass().ChannelManager;
	const CChannelInfo *pChInfo=ChannelManager.GetCurrentChannelInfo();
	LPCTSTR pszText;

	if (pChInfo!=NULL || ChannelManager.GetCurrentSpace()>=0) {
		pszText=
			ChannelManager.GetTuningSpaceName(
				pChInfo!=NULL?pChInfo->GetSpace():ChannelManager.GetCurrentSpace());
		if (pszText==NULL)
			pszText=TEXT("<チューナー>");
	} else if (ChannelManager.GetCurrentSpace()==CChannelManager::SPACE_ALL) {
		pszText=TEXT("すべて");
	} else {
		pszText=TEXT("<チューナー>");
	}
	RECT rc=Rect;
	TVTest::Style::Subtract(&rc,m_pControlPanel->GetItemPadding());
	::DrawText(hdc,pszText,-1,&rc,
			   DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
}

void CTunerControlItem::OnLButtonDown(int x,int y)
{
	POINT pt;

	GetMenuPos(&pt);
	GetAppClass().UICore.PopupSubMenu(CMainMenu::SUBMENU_SPACE,&pt,TPM_RIGHTBUTTON);
}

void CTunerControlItem::OnRButtonDown(int x,int y)
{
	POINT pt;

	GetMenuPos(&pt);
	GetAppClass().UICore.ShowSpecialMenu(CUICore::MENU_TUNERSELECT,&pt,TPM_RIGHTBUTTON);
}


void CChannelControlItem::CalcSize(int Width,SIZE *pSize)
{
	pSize->cx=Width;
	pSize->cy=GetTextItemHeight();
}

void CChannelControlItem::Draw(HDC hdc,const RECT &Rect)
{
	CAppMain &App=GetAppClass();
	const CChannelManager &ChannelManager=App.ChannelManager;
	const CChannelInfo *pInfo;
	TCHAR szText[4+MAX_CHANNEL_NAME];

	if (App.UICore.GetSkin()->IsWheelChannelChanging()) {
		COLORREF crText,crBack;

		crText=::GetTextColor(hdc);
		crBack=::GetBkColor(hdc);
		::SetTextColor(hdc,MixColor(crText,crBack,128));
		pInfo=ChannelManager.GetChangingChannelInfo();
		::wsprintf(szText,TEXT("%d: %s"),pInfo->GetChannelNo(),pInfo->GetName());
	} else if ((pInfo=ChannelManager.GetCurrentChannelInfo())!=NULL) {
		::wsprintf(szText,TEXT("%d: %s"),pInfo->GetChannelNo(),pInfo->GetName());
	} else {
		::lstrcpy(szText,TEXT("<チャンネル>"));
	}
	RECT rc=Rect;
	TVTest::Style::Subtract(&rc,m_pControlPanel->GetItemPadding());
	::DrawText(hdc,szText,-1,&rc,
			   DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
}

void CChannelControlItem::OnLButtonDown(int x,int y)
{
	POINT pt;

	GetMenuPos(&pt);
	GetAppClass().UICore.PopupSubMenu(CMainMenu::SUBMENU_CHANNEL,&pt,TPM_RIGHTBUTTON);
}

void CChannelControlItem::OnRButtonDown(int x,int y)
{
	POINT pt;

	GetMenuPos(&pt);
	GetAppClass().UICore.PopupSubMenu(CMainMenu::SUBMENU_SERVICE,&pt,TPM_RIGHTBUTTON);
}


void CVideoControlItem::CalcSize(int Width,SIZE *pSize)
{
	pSize->cx=Width;
	pSize->cy=GetTextItemHeight();
}

void CVideoControlItem::Draw(HDC hdc,const RECT &Rect)
{
	CAppMain &App=GetAppClass();
	const CCoreEngine &CoreEngine=App.CoreEngine;
	TCHAR szText[32];

	::wsprintf(szText,TEXT("%d x %d (%d %%)"),
			   CoreEngine.GetOriginalVideoWidth(),
			   CoreEngine.GetOriginalVideoHeight(),
			   App.UICore.GetZoomPercentage());
	RECT rc=Rect;
	TVTest::Style::Subtract(&rc,m_pControlPanel->GetItemPadding());
	::DrawText(hdc,szText,-1,&rc,
			   DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
}

void CVideoControlItem::OnLButtonDown(int x,int y)
{
	POINT pt;

	GetMenuPos(&pt);
	GetAppClass().UICore.PopupSubMenu(CMainMenu::SUBMENU_ZOOM,&pt,TPM_RIGHTBUTTON);
}

void CVideoControlItem::OnRButtonDown(int x,int y)
{
	POINT pt;

	GetMenuPos(&pt);
	GetAppClass().UICore.PopupSubMenu(CMainMenu::SUBMENU_ASPECTRATIO,&pt,TPM_RIGHTBUTTON);
}


CVolumeControlItem::CVolumeControlItem()
	: m_BarHeight(8)
	, m_BarPadding(1)
	, m_BarBorderWidth(1)
{
}

void CVolumeControlItem::CalcSize(int Width,SIZE *pSize)
{
	const TVTest::Style::Margins &Padding=m_pControlPanel->GetItemPadding();
	pSize->cx=Width;
	pSize->cy=m_BarHeight+Padding.Vert();
}

void CVolumeControlItem::Draw(HDC hdc,const RECT &Rect)
{
	CUICore &UICore=GetAppClass().UICore;
	COLORREF TextColor=::GetTextColor(hdc),BarColor;
	LOGBRUSH lb;
	HPEN hpen,hpenOld;
	HBRUSH hbrOld;
	RECT rc;

	lb.lbStyle=BS_SOLID;
	lb.lbColor=TextColor;
	lb.lbHatch=0;
	hpen=::ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_INSIDEFRAME | PS_JOIN_MITER,
						m_BarBorderWidth,&lb,0,NULL);
	hpenOld=SelectPen(hdc,hpen);
	hbrOld=SelectBrush(hdc,::GetStockObject(NULL_BRUSH));
	rc=Rect;
	TVTest::Style::Subtract(&rc,m_pControlPanel->GetItemPadding());
	rc.top+=((rc.bottom-rc.top)-m_BarHeight)/2;
	rc.bottom=rc.top+m_BarHeight;
	::Rectangle(hdc,rc.left,rc.top,rc.right,rc.bottom);
	SelectBrush(hdc,hbrOld);
	SelectPen(hdc,hpenOld);
	::DeleteObject(hpen);
	if (!UICore.GetMute())
		BarColor=TextColor;
	else
		BarColor=MixColor(TextColor,::GetBkColor(hdc));
	::InflateRect(&rc,-m_BarBorderWidth,-m_BarBorderWidth);
	TVTest::Style::Subtract(&rc,m_BarPadding);
	rc.right=rc.left+(rc.right-rc.left)*UICore.GetVolume()/CCoreEngine::MAX_VOLUME;
	DrawUtil::Fill(hdc,&rc,BarColor);
}

void CVolumeControlItem::OnLButtonDown(int x,int y)
{
	OnMouseMove(x,y);
	::SetCapture(m_pControlPanel->GetHandle());
}

void CVolumeControlItem::OnRButtonDown(int x,int y)
{
	CUICore &UICore=GetAppClass().UICore;
	UICore.SetMute(!UICore.GetMute());
}

void CVolumeControlItem::OnMouseMove(int x,int y)
{
	CUICore &UICore=GetAppClass().UICore;
	RECT rc;
	int Volume;

	rc=m_Position;
	TVTest::Style::Subtract(&rc,m_pControlPanel->GetItemPadding());
	TVTest::Style::Subtract(&rc,m_BarPadding);
	Volume=(x-rc.left)*CCoreEngine::MAX_VOLUME/((rc.right-rc.left)-1);
	if (Volume<0)
		Volume=0;
	else if (Volume>CCoreEngine::MAX_VOLUME)
		Volume=CCoreEngine::MAX_VOLUME;
	if (UICore.GetMute() || Volume!=UICore.GetVolume())
		UICore.SetVolume(Volume,false);
}

void CVolumeControlItem::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	pStyleManager->Get(TEXT("control-panel.volume.bar.height"),&m_BarHeight);
	pStyleManager->Get(TEXT("control-panel.volume.bar.padding"),&m_BarPadding);
	pStyleManager->Get(TEXT("control-panel.volume.bar.border.width"),&m_BarBorderWidth);
}

void CVolumeControlItem::NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	pStyleManager->ToPixels(&m_BarHeight);
	pStyleManager->ToPixels(&m_BarPadding);
	pStyleManager->ToPixels(&m_BarBorderWidth);
}


void CAudioControlItem::CalcSize(int Width,SIZE *pSize)
{
	pSize->cx=Width;
	pSize->cy=GetTextItemHeight();
	if (pSize->cy<16)
		pSize->cy=16;
}

void CAudioControlItem::Draw(HDC hdc,const RECT &Rect)
{
	CAppMain &App=GetAppClass();
	RECT rc=Rect;
	TVTest::Style::Subtract(&rc,m_pControlPanel->GetItemPadding());

	if (App.CoreEngine.m_DtvEngine.m_MediaViewer.IsSpdifPassthrough()) {
		if (!m_Icons.IsCreated())
			m_Icons.Load(App.GetResourceInstance(),IDB_PASSTHROUGH,16,16);
		m_Icons.Draw(hdc,rc.left,rc.top+((rc.bottom-rc.top)-16)/2,
					 16,16,0,::GetTextColor(hdc));
		rc.left+=16+4;
	}

	TCHAR szText[64];
	if (App.UICore.FormatCurrentAudioText(szText,lengthof(szText))<=0)
		::lstrcpy(szText,TEXT("<音声>"));
	::DrawText(hdc,szText,-1,&rc,
			   DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
}

void CAudioControlItem::OnLButtonDown(int x,int y)
{
	if (!GetAppClass().UICore.SwitchAudio())
		OnRButtonDown(x,y);
}

void CAudioControlItem::OnRButtonDown(int x,int y)
{
	POINT pt;

	GetMenuPos(&pt);
	GetAppClass().UICore.PopupSubMenu(CMainMenu::SUBMENU_AUDIO,&pt,TPM_RIGHTBUTTON);
}




CControlPanelButton::CControlPanelButton(int Command,LPCTSTR pszText,bool fBreak,int Width)
	: m_Text(pszText)
	, m_Width(Width)
{
	m_Command=Command;
	m_fBreak=fBreak;
}

CControlPanelButton::~CControlPanelButton()
{
}

void CControlPanelButton::CalcSize(int Width,SIZE *pSize)
{
	const TVTest::Style::Margins &Padding=m_pControlPanel->GetItemPadding();

	if (m_Width<0)
		CalcTextSize(m_Text.Get(),pSize);
	else
		pSize->cx=m_Width*m_pControlPanel->GetFontHeight();
	pSize->cx+=Padding.Horz();
	pSize->cy=GetTextItemHeight();
}

void CControlPanelButton::Draw(HDC hdc,const RECT &Rect)
{
	RECT rc=Rect;
	TVTest::Style::Subtract(&rc,m_pControlPanel->GetItemPadding());
	::DrawText(hdc,m_Text.Get(),-1,&rc,
			   DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}
