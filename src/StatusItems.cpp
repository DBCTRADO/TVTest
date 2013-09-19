#include "stdafx.h"
#include "TVTest.h"
#include "StatusItems.h"
#include "AppMain.h"
#include "Menu.h"
#include "EpgUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CChannelStatusItem::CChannelStatusItem()
	: CStatusItem(STATUS_ITEM_CHANNEL,100)
{
}

void CChannelStatusItem::Draw(HDC hdc,const RECT *pRect)
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
		StdUtil::snprintf(szText,lengthof(szText),TEXT("%d: %s"),
						  pInfo->GetChannelNo(),pInfo->GetName());
	} else if ((pInfo=ChannelManager.GetCurrentChannelInfo())!=NULL) {
		TCHAR szService[MAX_CHANNEL_NAME];
		StdUtil::snprintf(szText,lengthof(szText),TEXT("%d: %s"),
			pInfo->GetChannelNo(),
			AppMain.GetCurrentServiceName(szService,lengthof(szService))?szService:pInfo->GetName());
	} else {
		::lstrcpy(szText,TEXT("<チャンネル>"));
	}
	DrawText(hdc,pRect,szText);
}

void CChannelStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	DrawText(hdc,pRect,TEXT("アフリカ中央テレビ"));
}

void CChannelStatusItem::OnLButtonDown(int x,int y)
{
	POINT pt;
	UINT Flags;

	GetMenuPos(&pt,&Flags);
	GetAppClass().GetUICore()->PopupSubMenu(CMainMenu::SUBMENU_CHANNEL,
											&pt,Flags | TPM_RIGHTBUTTON);
}

void CChannelStatusItem::OnRButtonDown(int x,int y)
{
	POINT pt;
	UINT Flags;

	GetMenuPos(&pt,&Flags);
	GetAppClass().GetUICore()->PopupSubMenu(CMainMenu::SUBMENU_SERVICE,
											&pt,Flags | TPM_RIGHTBUTTON);
}


CVideoSizeStatusItem::CVideoSizeStatusItem()
	: CStatusItem(STATUS_ITEM_VIDEOSIZE,120)
{
}

void CVideoSizeStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	CAppMain &AppMain=GetAppClass();
	const CCoreEngine &CoreEngine=*AppMain.GetCoreEngine();
	TCHAR szText[64];

	StdUtil::snprintf(szText,lengthof(szText),TEXT("%d x %d (%d %%)"),
					  CoreEngine.GetOriginalVideoWidth(),
					  CoreEngine.GetOriginalVideoHeight(),
					  AppMain.GetUICore()->GetZoomPercentage());
	DrawText(hdc,pRect,szText);
}

void CVideoSizeStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	DrawText(hdc,pRect,
#ifndef TVH264_FOR_1SEG
			 TEXT("1920 x 1080 (100 %)")
#else
			 TEXT("320 x 180 (100 %)")
#endif
			 );
}

void CVideoSizeStatusItem::OnLButtonDown(int x,int y)
{
	POINT pt;
	UINT Flags;

	GetMenuPos(&pt,&Flags);
	GetAppClass().GetUICore()->PopupSubMenu(CMainMenu::SUBMENU_ZOOM,
											&pt,Flags | TPM_RIGHTBUTTON);
}

void CVideoSizeStatusItem::OnRButtonDown(int x,int y)
{
	POINT pt;
	UINT Flags;

	GetMenuPos(&pt,&Flags);
	GetAppClass().GetUICore()->PopupSubMenu(CMainMenu::SUBMENU_ASPECTRATIO,
											&pt,Flags | TPM_RIGHTBUTTON);
}


CVolumeStatusItem::CVolumeStatusItem()
	: CStatusItem(STATUS_ITEM_VOLUME,80)
{
}

void CVolumeStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	CUICore *pUICore=GetAppClass().GetUICore();
	HPEN hpen,hpenOld;
	HBRUSH hbr,hbrOld;
	RECT rc;
	COLORREF crText=::GetTextColor(hdc),crBar;

	hpen=::CreatePen(PS_SOLID,1,crText);
	hpenOld=SelectPen(hdc,hpen);
	hbrOld=SelectBrush(hdc,::GetStockObject(NULL_BRUSH));
	rc.left=pRect->left;
	rc.top=pRect->top+(pRect->bottom-pRect->top-8)/2;
	rc.right=pRect->right;
	rc.bottom=rc.top+8;
	::Rectangle(hdc,rc.left,rc.top,rc.right,rc.bottom);
	::SelectObject(hdc,hbrOld);
	::SelectObject(hdc,hpenOld);
	::DeleteObject(hpen);
	if (!pUICore->GetMute())
		crBar=crText;
	else
		crBar=MixColor(crText,::GetBkColor(hdc),128);
	hbr=::CreateSolidBrush(crBar);
	rc.left+=2;
	rc.top+=2;
	rc.right=rc.left+(rc.right-2-rc.left)*pUICore->GetVolume()/CCoreEngine::MAX_VOLUME;
	rc.bottom-=2;
	::FillRect(hdc,&rc,hbr);
	::DeleteObject(hbr);
}

void CVolumeStatusItem::OnLButtonDown(int x,int y)
{
	OnMouseMove(x,y);
	SetCapture(m_pStatus->GetHandle());
}

void CVolumeStatusItem::OnRButtonDown(int x,int y)
{
	// メニューを出すようにしたら評判悪かった...
	/*
	POINT pt;
	UINT Flags;

	GetMenuPos(&pt,&Flags);
	GetAppClass().GetUICore()->PopupSubMenu(CMainMenu::SUBMENU_VOLUME,
											&pt,Flags | TPM_RIGHTBUTTON);
	*/
	CUICore *pUICore=GetAppClass().GetUICore();
	pUICore->SetMute(!pUICore->GetMute());
}

void CVolumeStatusItem::OnMouseMove(int x,int y)
{
	CUICore *pUICore=GetAppClass().GetUICore();
	RECT rc;
	int Volume;

	GetClientRect(&rc);
	Volume=(x-2)*CCoreEngine::MAX_VOLUME/(rc.right-rc.left-4-1);
	if (Volume<0)
		Volume=0;
	else if (Volume>CCoreEngine::MAX_VOLUME)
		Volume=CCoreEngine::MAX_VOLUME;
	if (pUICore->GetMute() || Volume!=pUICore->GetVolume())
		pUICore->SetVolume(Volume,false);
}


CAudioChannelStatusItem::CAudioChannelStatusItem()
	: CStatusItem(STATUS_ITEM_AUDIOCHANNEL,80)
{
}

void CAudioChannelStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	CAppMain &App=GetAppClass();
	RECT rc=*pRect;

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
	DrawText(hdc,&rc,szText);
}

void CAudioChannelStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	DrawText(hdc,pRect,TEXT("Stereo"));
}

void CAudioChannelStatusItem::OnLButtonDown(int x,int y)
{
	if (!GetAppClass().GetUICore()->SwitchAudio())
		OnRButtonDown(x,y);
}

void CAudioChannelStatusItem::OnRButtonDown(int x,int y)
{
	POINT pt;
	UINT Flags;

	GetMenuPos(&pt,&Flags);
	GetAppClass().GetUICore()->PopupSubMenu(CMainMenu::SUBMENU_AUDIO,
											&pt,Flags | TPM_RIGHTBUTTON);
}


CRecordStatusItem::CRecordStatusItem()
	: CStatusItem(STATUS_ITEM_RECORD,64)
	, m_fRemain(false)
	, m_CircleColor(RGB(223,63,0))
{
}

void CRecordStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	const CRecordManager &RecordManager=*GetAppClass().GetRecordManager();
	const int FontHeight=m_pStatus->GetFontHeight();
	RECT rc;
	TCHAR szText[32],*pszText;

	rc=*pRect;
	if (RecordManager.IsRecording()) {
		if (RecordManager.IsPaused()) {
			HBRUSH hbr=::CreateSolidBrush(::GetTextColor(hdc));
			RECT rc1;

			rc1.left=rc.left;
			rc1.top=rc.top+((rc.bottom-rc.top)-FontHeight)/2;
			rc1.right=rc1.left+FontHeight/2-1;
			rc1.bottom=rc1.top+FontHeight;
			::FillRect(hdc,&rc1,hbr);
			rc1.left=rc1.right+2;
			rc1.right=rc.left+FontHeight;
			::FillRect(hdc,&rc1,hbr);
			::DeleteObject(hbr);
		} else {
#if 0
			// Ellipseで小さい丸を描くと汚い
			HBRUSH hbr=::CreateSolidBrush(m_CircleColor);
			HBRUSH hbrOld;
			HPEN hpenOld;

			rc1.right=rc1.left+FontHeight;
			hbrOld=SelectBrush(hdc,hbr);
			hpenOld=SelectPen(hdc,::GetStockObject(NULL_PEN));
			::Ellipse(hdc,rc1.left,rc1.top,rc1.right,rc1.bottom);
			SelectPen(hdc,hpenOld);
			SelectBrush(hdc,hbrOld);
			::DeleteObject(hbr);
#else
			COLORREF OldTextColor=::SetTextColor(hdc,m_CircleColor);
			::DrawText(hdc,TEXT("●"),-1,&rc,
					   DT_LEFT | DT_SINGLELINE | DT_VCENTER);
			::SetTextColor(hdc,OldTextColor);
#endif
		}
		rc.left+=FontHeight+4;
		bool fRemain=m_fRemain && RecordManager.IsStopTimeSpecified();
		int RecordSec;
		if (fRemain) {
			RecordSec=(int)RecordManager.GetRemainTime()/1000;
			if (RecordSec<0)
				RecordSec=0;
		} else {
			RecordSec=(int)(RecordManager.GetRecordTime()/1000);
		}
		StdUtil::snprintf(szText,lengthof(szText),TEXT("%s%d:%02d:%02d"),
						  fRemain?TEXT("-"):TEXT(""),
						  RecordSec/(60*60),(RecordSec/60)%60,RecordSec%60);
		pszText=szText;
	} else if (RecordManager.IsReserved()) {
		pszText=TEXT("■ 録画待機");
	} else {
		pszText=TEXT("■ <録画>");
	}
	DrawText(hdc,&rc,pszText);
}

void CRecordStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	RECT rc=*pRect;
	COLORREF OldTextColor=::SetTextColor(hdc,m_CircleColor);
	::DrawText(hdc,TEXT("●"),-1,&rc,DT_LEFT | DT_SINGLELINE | DT_VCENTER);
	::SetTextColor(hdc,OldTextColor);
	rc.left+=m_pStatus->GetFontHeight()+4;
	DrawText(hdc,&rc,TEXT("0:24:15"));
}

void CRecordStatusItem::OnLButtonDown(int x,int y)
{
	CAppMain &AppMain=GetAppClass();
	const CRecordManager &RecordManager=*AppMain.GetRecordManager();
	CUICore &UICore=*AppMain.GetUICore();
	bool fRecording=RecordManager.IsRecording();

	if (fRecording && !RecordManager.IsPaused()) {
		if (!UICore.ConfirmStopRecording())
			return;
	}
	UICore.DoCommand(RecordManager.IsReserved()?CM_RECORDOPTION:
					 !fRecording?CM_STATUSBARRECORD:CM_RECORD);
}

void CRecordStatusItem::OnRButtonDown(int x,int y)
{
	POINT pt;
	UINT Flags;

	GetMenuPos(&pt,&Flags);
	GetAppClass().GetUICore()->ShowSpecialMenu(CUICore::MENU_RECORD,
											   &pt,Flags | TPM_RIGHTBUTTON);
}

bool CRecordStatusItem::OnMouseHover(int x,int y)
{
	const CRecordManager &RecordManager=*GetAppClass().GetRecordManager();

	if (RecordManager.IsRecording()) {
		const HWND hwndStatus=m_pStatus->GetHandle();

		if (!m_Tooltip.IsCreated()) {
			m_Tooltip.Create(hwndStatus);
			m_Tooltip.AddTrackingTip(1);
			m_Tooltip.SetMaxWidth(256);
		}

		TCHAR szText[256];
		GetTipText(szText,lengthof(szText));
		m_Tooltip.SetText(1,szText);
		if (!m_Tooltip.IsVisible())
			m_Tooltip.TrackActivate(1,true);
	}
	return true;
}

int CRecordStatusItem::GetTipText(LPTSTR pszText,int MaxLength)
{
	const CRecordManager &RecordManager=*GetAppClass().GetRecordManager();

	if (RecordManager.IsRecording()) {
		const CRecordTask *pRecordTask=RecordManager.GetRecordTask();

		unsigned int RecordSec=pRecordTask->GetRecordTime()/1000;
		unsigned int WroteSize=(unsigned int)(pRecordTask->GetWroteSize()/(1024*1024/100));
		LONGLONG DiskFreeSpace=pRecordTask->GetFreeSpace();
		unsigned int FreeSpace;
		if (DiskFreeSpace>0)
			FreeSpace=(unsigned int)(DiskFreeSpace/(ULONGLONG)(1024*1024*1024/100));
		else
			FreeSpace=0;
		return StdUtil::snprintf(pszText,MaxLength,
								 TEXT("● %d:%02d:%02d\r\nサイズ: %d.%02d MB\r\n空き容量: %d.%02d GB"),
								 RecordSec/(60*60),(RecordSec/60)%60,RecordSec%60,
								 WroteSize/100,WroteSize%100,
								 FreeSpace/100,FreeSpace%100);
	}
	pszText[0]='\0';
	return 0;
}

void CRecordStatusItem::OnFocus(bool fFocus)
{
	if (!fFocus && m_Tooltip.IsCreated())
		m_Tooltip.TrackActivate(1,false);
}

LRESULT CRecordStatusItem::OnNotifyMessage(LPNMHDR pnmh)
{
	if (pnmh->hwndFrom==m_Tooltip.GetHandle()) {
		if (pnmh->code==TTN_SHOW) {
			RECT rc,rcTip;
			int x,y;

			GetRect(&rc);
			MapWindowRect(m_pStatus->GetHandle(),NULL,&rc);
			::GetWindowRect(pnmh->hwndFrom,&rcTip);
			x=rc.left+((rc.right-rc.left)-(rcTip.right-rcTip.left))/2;
			y=rc.top-(rcTip.bottom-rcTip.top);
			::SendMessage(pnmh->hwndFrom,TTM_TRACKPOSITION,0,MAKELONG(x,y));
			::SetWindowPos(pnmh->hwndFrom,NULL,x,y,0,0,
						   SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
			return TRUE;
		}
	}
	return 0;
}

void CRecordStatusItem::ShowRemainTime(bool fRemain)
{
	if (m_fRemain!=fRemain) {
		m_fRemain=fRemain;
		Update();
	}
}

void CRecordStatusItem::SetCircleColor(COLORREF Color)
{
	if (m_CircleColor!=Color) {
		m_CircleColor=Color;
		Update();
	}
}


CCaptureStatusItem::CCaptureStatusItem()
	: CStatusItem(STATUS_ITEM_CAPTURE,16)
{
	m_MinWidth=16;
}

void CCaptureStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	if (!m_Icons.IsCreated())
		m_Icons.Load(GetAppClass().GetResourceInstance(),IDB_CAPTURE);
	DrawIcon(hdc,pRect,m_Icons);
}

void CCaptureStatusItem::OnLButtonDown(int x,int y)
{
	GetAppClass().GetUICore()->DoCommand(CM_CAPTURE);
}

void CCaptureStatusItem::OnRButtonDown(int x,int y)
{
	POINT pt;
	UINT Flags;

	GetMenuPos(&pt,&Flags);
	GetAppClass().GetUICore()->ShowSpecialMenu(CUICore::MENU_CAPTURE,
											   &pt,Flags | TPM_RIGHTBUTTON);
}


CErrorStatusItem::CErrorStatusItem()
	: CStatusItem(STATUS_ITEM_ERROR,120)
{
}

void CErrorStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	const CCoreEngine &CoreEngine=*GetAppClass().GetCoreEngine();
	TCHAR szText[64];

	StdUtil::snprintf(szText,lengthof(szText),TEXT("D %u / E %u / S %u"),
					  CoreEngine.GetContinuityErrorPacketCount(),
					  CoreEngine.GetErrorPacketCount(),
					  CoreEngine.GetScramblePacketCount());
	DrawText(hdc,pRect,szText);
}

void CErrorStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	DrawText(hdc,pRect,TEXT("D 2 / E 0 / S 127"));
}

void CErrorStatusItem::OnLButtonDown(int x,int y)
{
	GetAppClass().GetUICore()->DoCommand(CM_RESETERRORCOUNT);
}

void CErrorStatusItem::OnRButtonDown(int x,int y)
{
	POINT pt;
	UINT Flags;

	GetMenuPos(&pt,&Flags);
	GetAppClass().GetUICore()->ShowSpecialMenu(CUICore::MENU_STREAMERROR,
											   &pt,Flags | TPM_RIGHTBUTTON);
}


CSignalLevelStatusItem::CSignalLevelStatusItem()
	: CStatusItem(STATUS_ITEM_SIGNALLEVEL,120)
	, m_fShowSignalLevel(true)
{
}

void CSignalLevelStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	const CCoreEngine *pCoreEngine=GetAppClass().GetCoreEngine();
	TCHAR szText[64],szSignalLevel[32];
	int Length=0;

	if (m_fShowSignalLevel) {
		pCoreEngine->GetSignalLevelText(szSignalLevel,lengthof(szSignalLevel));
		Length=StdUtil::snprintf(szText,lengthof(szText),
								 TEXT("%s / "),szSignalLevel);
	}
	pCoreEngine->GetBitRateText(szText+Length,lengthof(szText)-Length);
	DrawText(hdc,pRect,szText);
}

void CSignalLevelStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	const CCoreEngine *pCoreEngine=GetAppClass().GetCoreEngine();
	TCHAR szText[64],szSignalLevel[32],szBitRate[32];

	pCoreEngine->GetSignalLevelText(24.52f,szSignalLevel,lengthof(szSignalLevel));
	pCoreEngine->GetBitRateText(16.73f,szBitRate,lengthof(szBitRate));
	StdUtil::snprintf(szText,lengthof(szText),TEXT("%s / %s"),szSignalLevel,szBitRate);
	DrawText(hdc,pRect,szText);
}

void CSignalLevelStatusItem::ShowSignalLevel(bool fShow)
{
	if (m_fShowSignalLevel!=fShow) {
		m_fShowSignalLevel=fShow;
		Update();
	}
}


CClockStatusItem::CClockStatusItem()
	: CStatusItem(STATUS_ITEM_CLOCK,48)
	, m_fTOT(false)
{
}

void CClockStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	SYSTEMTIME st;
	TCHAR szText[64];

	if (m_fTOT) {
		if (!GetAppClass().GetCoreEngine()->m_DtvEngine.m_TsAnalyzer.GetTotTime(&st))
			return;
		StdUtil::snprintf(szText,lengthof(szText),TEXT("TOT: %d/%d/%d %d:%02d:%02d"),
						  st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
	} else {
		::GetLocalTime(&st);
		StdUtil::snprintf(szText,lengthof(szText),TEXT("%d:%02d:%02d"),
						  st.wHour,st.wMinute,st.wSecond);
	}
	DrawText(hdc,pRect,szText);
}

void CClockStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	if (m_fTOT) {
		DrawText(hdc,pRect,TEXT("TOT: 2011/5/24 13:25:30"));
	} else {
		DrawText(hdc,pRect,TEXT("13:25:30"));
	}
}

void CClockStatusItem::OnLButtonDown(int x,int y)
{
	GetAppClass().GetUICore()->DoCommand(CM_SHOWTOTTIME);
}

void CClockStatusItem::OnRButtonDown(int x,int y)
{
	POINT pt;
	UINT Flags;

	GetMenuPos(&pt,&Flags);
	GetAppClass().GetUICore()->ShowSpecialMenu(CUICore::MENU_CLOCK,
											   &pt,Flags | TPM_RIGHTBUTTON);
}

void CClockStatusItem::SetTOT(bool fTOT)
{
	if (m_fTOT!=fTOT) {
		m_fTOT=fTOT;
		Update();
	}
}


CProgramInfoStatusItem::CProgramInfoStatusItem()
	: CStatusItem(STATUS_ITEM_PROGRAMINFO,256)
	, m_fNext(false)
	, m_fEnablePopupInfo(true)
{
}

void CProgramInfoStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	if (!m_Text.IsEmpty())
		DrawText(hdc,pRect,m_Text.Get());
}

void CProgramInfoStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	DrawText(hdc,pRect,TEXT("1:00～1:30 今日のニュース"));
}

void CProgramInfoStatusItem::OnLButtonDown(int x,int y)
{
	if (!m_fEnablePopupInfo) {
		if (!m_EventInfoPopup.IsVisible())
			ShowPopupInfo();
		else
			m_EventInfoPopup.Hide();
	} else {
		m_fNext=!m_fNext;
		m_EventInfoPopup.Hide();
		UpdateContent();
		Update();
	}
}

void CProgramInfoStatusItem::OnRButtonDown(int x,int y)
{
	m_EventInfoPopup.Hide();

	POINT pt;
	UINT Flags;

	GetMenuPos(&pt,&Flags);
	GetAppClass().GetUICore()->ShowSpecialMenu(CUICore::MENU_PROGRAMINFO,
											   &pt,Flags | TPM_RIGHTBUTTON);
}

void CProgramInfoStatusItem::OnLButtonDoubleClick(int x,int y)
{
	if (!m_fEnablePopupInfo) {
		m_fNext=!m_fNext;
		UpdateContent();
		Update();
		m_EventInfoPopup.Hide();
		ShowPopupInfo();
	} else {
		OnLButtonDown(x,y);
	}
}

void CProgramInfoStatusItem::OnFocus(bool fFocus)
{
	if (!fFocus && m_EventInfoPopup.IsVisible()) {
		POINT pt;

		::GetCursorPos(&pt);
		if (!m_EventInfoPopup.IsOwnWindow(::WindowFromPoint(pt)))
			m_EventInfoPopup.Hide();
	}
}

bool CProgramInfoStatusItem::OnMouseHover(int x,int y)
{
	if (!m_fEnablePopupInfo
			|| ::GetActiveWindow()!=::GetForegroundWindow())
		return true;

	ShowPopupInfo();
	return true;
}

void CProgramInfoStatusItem::EnablePopupInfo(bool fEnable)
{
	m_fEnablePopupInfo=fEnable;
	if (!fEnable)
		m_EventInfoPopup.Hide();
}

bool CProgramInfoStatusItem::UpdateContent()
{
	CCoreEngine &CoreEngine=*GetAppClass().GetCoreEngine();
	TCHAR szText[256],szEventName[256];
	CStaticStringFormatter Formatter(szText,lengthof(szText));
	SYSTEMTIME StartTime;
	DWORD Duration;

	if (m_fNext)
		Formatter.Append(TEXT("次: "));
	if (CoreEngine.m_DtvEngine.GetEventTime(&StartTime,&Duration,m_fNext)) {
		TCHAR szTime[EpgUtil::MAX_EVENT_TIME_LENGTH];
		if (EpgUtil::FormatEventTime(StartTime,Duration,szTime,lengthof(szTime))>0) {
			Formatter.Append(szTime);
			Formatter.Append(TEXT(" "));
		}
	}
	if (CoreEngine.m_DtvEngine.GetEventName(szEventName,lengthof(szEventName),m_fNext)>0)
		Formatter.Append(szEventName);
	if (m_Text.Compare(Formatter.GetString())==0)
		return false;
	m_Text.Set(Formatter.GetString());
	return true;
}

void CProgramInfoStatusItem::ShowPopupInfo()
{
	if (m_EventInfoPopup.IsVisible())
		return;

	CCoreEngine &CoreEngine=*GetAppClass().GetCoreEngine();
	WORD ServiceID;
	if (CoreEngine.m_DtvEngine.GetServiceID(&ServiceID)) {
		CEventInfoData EventInfo;

		if (CoreEngine.GetCurrentEventInfo(&EventInfo,ServiceID,m_fNext)) {
			RECT rc;
			POINT pt;

			GetRect(&rc);
			pt.x=rc.left;
			pt.y=rc.top;
			::ClientToScreen(m_pStatus->GetHandle(),&pt);
			::SetRect(&rc,pt.x,pt.y-300,pt.x+300,pt.y);
			m_EventInfoPopup.Show(&EventInfo,&rc);
		}
	}
}


CBufferingStatusItem::CBufferingStatusItem()
	: CStatusItem(STATUS_ITEM_BUFFERING,80)
{
}

void CBufferingStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	CCoreEngine &CoreEngine=*GetAppClass().GetCoreEngine();
	TCHAR szText[32];

	StdUtil::snprintf(szText,lengthof(szText),TEXT("R %lu / B %d%%"),
					  CoreEngine.GetStreamRemain(),CoreEngine.GetPacketBufferUsedPercentage());
	DrawText(hdc,pRect,szText);
}

void CBufferingStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	DrawText(hdc,pRect,TEXT("R 2 / B 48%"));
}

void CBufferingStatusItem::OnLButtonDown(int x,int y)
{
	POINT pt;
	UINT Flags;

	GetMenuPos(&pt,&Flags);
	GetAppClass().GetUICore()->ShowSpecialMenu(CUICore::MENU_BUFFERING,
											   &pt,Flags | TPM_RIGHTBUTTON);
}


CTunerStatusItem::CTunerStatusItem()
	: CStatusItem(STATUS_ITEM_TUNER,80)
{
}

void CTunerStatusItem::Draw(HDC hdc,const RECT *pRect)
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
	DrawText(hdc,pRect,pszText);
}

void CTunerStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	DrawText(hdc,pRect,TEXT("地デジ"));
}

void CTunerStatusItem::OnLButtonDown(int x,int y)
{
	POINT pt;
	UINT Flags;

	GetMenuPos(&pt,&Flags);
	GetAppClass().GetUICore()->PopupSubMenu(CMainMenu::SUBMENU_SPACE,
											&pt,Flags | TPM_RIGHTBUTTON);
}

void CTunerStatusItem::OnRButtonDown(int x,int y)
{
	POINT pt;
	UINT Flags;

	GetMenuPos(&pt,&Flags);
	GetAppClass().GetUICore()->ShowSpecialMenu(CUICore::MENU_TUNERSELECT,
											   &pt,Flags | TPM_RIGHTBUTTON);
}


CMediaBitRateStatusItem::CMediaBitRateStatusItem()
	: CStatusItem(STATUS_ITEM_MEDIABITRATE,140)
{
}

void CMediaBitRateStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	CCoreEngine &CoreEngine=*GetAppClass().GetCoreEngine();
	TCHAR szText[64];

	StdUtil::snprintf(szText,lengthof(szText),TEXT("V %u Kbps / A %u Kbps"),
					  CoreEngine.m_DtvEngine.m_MediaViewer.GetVideoBitRate()/1024,
					  CoreEngine.m_DtvEngine.m_MediaViewer.GetAudioBitRate()/1024);
	DrawText(hdc,pRect,szText);
}

void CMediaBitRateStatusItem::DrawPreview(HDC hdc,const RECT *pRect)
{
	DrawText(hdc,pRect,TEXT("V 12504 Kbps / A 185 Kbps"));
}


CFavoritesStatusItem::CFavoritesStatusItem()
	: CStatusItem(STATUS_ITEM_FAVORITES,16)
{
	m_MinWidth=16;
}

void CFavoritesStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	if (!m_IconBitmap.IsCreated())
		m_IconBitmap.Load(GetAppClass().GetResourceInstance(),IDB_STATUSBAR_FAVORITES);
	DrawIcon(hdc,pRect,m_IconBitmap);
}

void CFavoritesStatusItem::OnLButtonDown(int x,int y)
{
	POINT pt;
	UINT Flags;

	GetMenuPos(&pt,&Flags);
	GetAppClass().GetUICore()->PopupSubMenu(CMainMenu::SUBMENU_FAVORITES,
											&pt,Flags | TPM_RIGHTBUTTON);
}

void CFavoritesStatusItem::OnRButtonDown(int x,int y)
{
	GetAppClass().GetUICore()->DoCommand(CM_ADDTOFAVORITES);
}
