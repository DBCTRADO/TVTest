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


#define TEXT_MARGIN 4




void CTunerControlItem::CalcSize(int Width,SIZE *pSize)
{
	pSize->cx=Width;
	pSize->cy=m_pControlPanel->GetFontHeight()+TEXT_MARGIN*2;
}

void CTunerControlItem::Draw(HDC hdc,const RECT &Rect)
{
	const CChannelManager &ChannelManager=*GetAppClass().GetChannelManager();
	const CChannelInfo *pChInfo=ChannelManager.GetCurrentRealChannelInfo();
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
	rc.left+=TEXT_MARGIN;
	rc.right-=TEXT_MARGIN;
	::DrawText(hdc,pszText,-1,&rc,
			   DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
}

void CTunerControlItem::OnLButtonDown(int x,int y)
{
	POINT pt;

	GetMenuPos(&pt);
	GetAppClass().GetUICore()->PopupSubMenu(CMainMenu::SUBMENU_SPACE,
											&pt,TPM_RIGHTBUTTON);
}

void CTunerControlItem::OnRButtonDown(int x,int y)
{
	POINT pt;

	GetMenuPos(&pt);
	GetAppClass().GetUICore()->ShowSpecialMenu(CUICore::MENU_TUNERSELECT,
											   &pt,TPM_RIGHTBUTTON);
}


void CChannelControlItem::CalcSize(int Width,SIZE *pSize)
{
	pSize->cx=Width;
	pSize->cy=m_pControlPanel->GetFontHeight()+TEXT_MARGIN*2;
}

void CChannelControlItem::Draw(HDC hdc,const RECT &Rect)
{
	CAppMain &AppMain=GetAppClass();
	const CChannelManager &ChannelManager=*AppMain.GetChannelManager();
	const CChannelInfo *pInfo;
	TCHAR szText[4+MAX_CHANNEL_NAME];

	if (AppMain.GetUICore()->GetSkin()->IsWheelChannelChanging()) {
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
	rc.left+=TEXT_MARGIN;
	rc.right-=TEXT_MARGIN;
	::DrawText(hdc,szText,-1,&rc,
			   DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
}

void CChannelControlItem::OnLButtonDown(int x,int y)
{
	POINT pt;

	GetMenuPos(&pt);
	GetAppClass().GetUICore()->PopupSubMenu(CMainMenu::SUBMENU_CHANNEL,
											&pt,TPM_RIGHTBUTTON);
}

void CChannelControlItem::OnRButtonDown(int x,int y)
{
	POINT pt;

	GetMenuPos(&pt);
	GetAppClass().GetUICore()->PopupSubMenu(CMainMenu::SUBMENU_SERVICE,
											&pt,TPM_RIGHTBUTTON);
}


void CVideoControlItem::CalcSize(int Width,SIZE *pSize)
{
	pSize->cx=Width;
	pSize->cy=m_pControlPanel->GetFontHeight()+TEXT_MARGIN*2;
}

void CVideoControlItem::Draw(HDC hdc,const RECT &Rect)
{
	CAppMain &AppMain=GetAppClass();
	const CCoreEngine &CoreEngine=*AppMain.GetCoreEngine();
	TCHAR szText[32];

	::wsprintf(szText,TEXT("%d x %d (%d %%)"),
			   CoreEngine.GetOriginalVideoWidth(),
			   CoreEngine.GetOriginalVideoHeight(),
			   AppMain.GetUICore()->GetZoomPercentage());
	RECT rc=Rect;
	rc.left+=TEXT_MARGIN;
	rc.right-=TEXT_MARGIN;
	::DrawText(hdc,szText,-1,&rc,
			   DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
}

void CVideoControlItem::OnLButtonDown(int x,int y)
{
	POINT pt;

	GetMenuPos(&pt);
	GetAppClass().GetUICore()->PopupSubMenu(CMainMenu::SUBMENU_ZOOM,
											&pt,TPM_RIGHTBUTTON);
}

void CVideoControlItem::OnRButtonDown(int x,int y)
{
	POINT pt;

	GetMenuPos(&pt);
	GetAppClass().GetUICore()->PopupSubMenu(CMainMenu::SUBMENU_ASPECTRATIO,
											&pt,TPM_RIGHTBUTTON);
}


const int CVolumeControlItem::m_BarHeight=8;
const int CVolumeControlItem::m_Margin=4;

CVolumeControlItem::CVolumeControlItem()
{
	m_Position.bottom=m_BarHeight+m_Margin*2;
}

void CVolumeControlItem::Draw(HDC hdc,const RECT &Rect)
{
	CUICore *pUICore=GetAppClass().GetUICore();
	COLORREF TextColor=::GetTextColor(hdc),BarColor;
	HPEN hpen,hpenOld;
	HBRUSH hbr,hbrOld;
	RECT rc;

	hpen=::CreatePen(PS_SOLID,1,TextColor);
	hpenOld=SelectPen(hdc,hpen);
	hbrOld=SelectBrush(hdc,::GetStockObject(NULL_BRUSH));
	rc=Rect;
	rc.left+=m_Margin;
	rc.right-=m_Margin;
	rc.top+=(rc.bottom-rc.top-m_BarHeight)/2;
	rc.bottom=rc.top+m_BarHeight;
	::Rectangle(hdc,rc.left,rc.top,rc.right,rc.bottom);
	SelectBrush(hdc,hbrOld);
	SelectPen(hdc,hpenOld);
	::DeleteObject(hpen);
	if (!pUICore->GetMute())
		BarColor=TextColor;
	else
		BarColor=MixColor(TextColor,::GetBkColor(hdc));
	hbr=::CreateSolidBrush(BarColor);
	rc.left+=2;
	rc.top+=2;
	rc.right=rc.left+(rc.right-2-rc.left)*pUICore->GetVolume()/CCoreEngine::MAX_VOLUME;
	rc.bottom-=2;
	::FillRect(hdc,&rc,hbr);
	::DeleteObject(hbr);
}

void CVolumeControlItem::OnLButtonDown(int x,int y)
{
	OnMouseMove(x,y);
	::SetCapture(m_pControlPanel->GetHandle());
}

void CVolumeControlItem::OnRButtonDown(int x,int y)
{
	CUICore *pUICore=GetAppClass().GetUICore();
	pUICore->SetMute(!pUICore->GetMute());
}

void CVolumeControlItem::OnMouseMove(int x,int y)
{
	CUICore *pUICore=GetAppClass().GetUICore();
	int Volume;

	Volume=(x-(m_Margin+2))*CCoreEngine::MAX_VOLUME/((m_Position.right-m_Position.left)-(m_Margin+2)*2-1);
	if (Volume<0)
		Volume=0;
	else if (Volume>CCoreEngine::MAX_VOLUME)
		Volume=CCoreEngine::MAX_VOLUME;
	if (pUICore->GetMute() || Volume!=pUICore->GetVolume())
		pUICore->SetVolume(Volume,false);
}


void CAudioControlItem::CalcSize(int Width,SIZE *pSize)
{
	pSize->cx=Width;
	pSize->cy=m_pControlPanel->GetFontHeight()+TEXT_MARGIN*2;
	if (pSize->cy<16)
		pSize->cy=16;
}

void CAudioControlItem::Draw(HDC hdc,const RECT &Rect)
{
	CAppMain &App=GetAppClass();
	RECT rc=Rect;
	rc.left+=TEXT_MARGIN;
	rc.right-=TEXT_MARGIN;

	if (App.GetCoreEngine()->m_DtvEngine.m_MediaViewer.IsSpdifPassthrough()) {
		if (!m_Icons.IsCreated())
			m_Icons.Load(App.GetResourceInstance(),IDB_PASSTHROUGH);
		m_Icons.Draw(hdc,rc.left,rc.top+((rc.bottom-rc.top)-16)/2,
					 ::GetTextColor(hdc));
		rc.left+=16+4;
	}

	TCHAR szText[64];
	if (App.GetUICore()->FormatCurrentAudioText(szText,lengthof(szText))<=0)
		::lstrcpy(szText,TEXT("<音声>"));
	::DrawText(hdc,szText,-1,&rc,
			   DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
}

void CAudioControlItem::OnLButtonDown(int x,int y)
{
	if (!GetAppClass().GetUICore()->SwitchAudio())
		OnRButtonDown(x,y);
}

void CAudioControlItem::OnRButtonDown(int x,int y)
{
	POINT pt;

	GetMenuPos(&pt);
	GetAppClass().GetUICore()->PopupSubMenu(CMainMenu::SUBMENU_AUDIO,
											&pt,TPM_RIGHTBUTTON);
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
	int FontHeight=m_pControlPanel->GetFontHeight();

	if (m_Width<0)
		CalcTextSize(m_Text.Get(),pSize);
	else
		pSize->cx=m_Width*FontHeight;
	pSize->cx+=TEXT_MARGIN*2;
	pSize->cy=FontHeight+TEXT_MARGIN*2;
}

void CControlPanelButton::Draw(HDC hdc,const RECT &Rect)
{
	RECT rc=Rect;
	rc.left+=TEXT_MARGIN;
	rc.right-=TEXT_MARGIN;
	::DrawText(hdc,m_Text.Get(),-1,&rc,
			   DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}
