#include "stdafx.h"
#include <algorithm>
#include "TVTest.h"
#include "AppMain.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CUICore::CUICore(CAppMain &App)
	: m_App(App)
	, m_pSkin(nullptr)
	, m_fStandby(false)
	, m_fFullscreen(false)
	, m_fAlwaysOnTop(false)

	, m_hicoLogoBig(nullptr)
	, m_hicoLogoSmall(nullptr)

	, m_fViewerInitializeError(false)

	, m_fScreenSaverActiveOriginal(FALSE)
	/*
	, m_fLowPowerActiveOriginal(FALSE)
	, m_fPowerOffActiveOriginal(FALSE)
	*/
{
}


CUICore::~CUICore()
{
	if (m_hicoLogoBig!=nullptr)
		::DeleteObject(m_hicoLogoBig);
	if (m_hicoLogoSmall!=nullptr)
		::DeleteObject(m_hicoLogoSmall);
}


bool CUICore::SetSkin(CUISkin *pSkin)
{
	if (m_pSkin!=nullptr) {
		m_App.AppEventManager.RemoveEventHandler(m_pSkin);
		m_pSkin->m_pCore=nullptr;
	}
	if (pSkin!=nullptr) {
		pSkin->m_pCore=this;
		m_App.AppEventManager.AddEventHandler(pSkin);
	}
	m_pSkin=pSkin;
	return true;
}


HWND CUICore::GetMainWindow() const
{
	if (m_pSkin==nullptr)
		return nullptr;
	return m_pSkin->GetMainWindow();
}


HWND CUICore::GetDialogOwner() const
{
	if (m_pSkin==nullptr)
		return nullptr;
	return m_pSkin->GetVideoHostWindow();
}


bool CUICore::InitializeViewer(BYTE VideoStreamType)
{
	if (m_pSkin==nullptr)
		return false;
	bool fOK=m_pSkin->InitializeViewer(VideoStreamType);
	m_fViewerInitializeError=!fOK;
	return fOK;
}


bool CUICore::FinalizeViewer()
{
	if (m_pSkin==nullptr)
		return false;
	return m_pSkin->FinalizeViewer();
}


bool CUICore::IsViewerEnabled() const
{
	if (m_pSkin==nullptr)
		return false;
	return m_pSkin->IsViewerEnabled();
}


bool CUICore::EnableViewer(bool fEnable)
{
	if (m_pSkin==nullptr)
		return false;
	return m_pSkin->EnableViewer(fEnable);
}


HWND CUICore::GetViewerWindow() const
{
	if (m_pSkin==nullptr)
		return false;
	return m_pSkin->GetViewerWindow();
}


bool CUICore::SetZoomRate(int Rate,int Factor)
{
	if (m_pSkin==nullptr)
		return false;
	return m_pSkin->SetZoomRate(Rate,Factor);
}


bool CUICore::GetZoomRate(int *pRate,int *pFactor) const
{
	if (m_pSkin==nullptr)
		return false;
	if (!m_pSkin->GetZoomRate(pRate,pFactor)
			|| (pRate!=nullptr && *pRate<1) || (pFactor!=nullptr && *pFactor<1))
		return false;
	return true;
}


int CUICore::GetZoomPercentage() const
{
	if (m_pSkin==nullptr)
		return 0;
	int Rate,Factor;
	if (!m_pSkin->GetZoomRate(&Rate,&Factor) || Factor==0)
		return false;
	return (Rate*100+Factor/2)/Factor;
}


bool CUICore::GetPanAndScan(PanAndScanInfo *pInfo) const
{
	if (m_pSkin==nullptr || pInfo==nullptr)
		return false;
	return m_pSkin->GetPanAndScan(pInfo);
}


bool CUICore::SetPanAndScan(const PanAndScanInfo &Info)
{
	if (m_pSkin==nullptr)
		return false;
	return m_pSkin->SetPanAndScan(Info);
}


int CUICore::GetVolume() const
{
	return m_App.CoreEngine.GetVolume();
}


bool CUICore::SetVolume(int Volume,bool fOSD)
{
	if (!m_App.CoreEngine.SetVolume(Volume))
		return false;
	m_App.AppEventManager.OnVolumeChanged(Volume);
	if (fOSD && m_pSkin!=nullptr)
		m_pSkin->ShowVolumeOSD();
	return true;
}


bool CUICore::GetMute() const
{
	return m_App.CoreEngine.GetMute();
}


bool CUICore::SetMute(bool fMute)
{
	if (fMute!=GetMute()) {
		if (!m_App.CoreEngine.SetMute(fMute))
			return false;
		m_App.AppEventManager.OnMuteChanged(fMute);
	}
	return true;
}


int CUICore::GetStereoMode() const
{
	return m_App.CoreEngine.GetStereoMode();
}


bool CUICore::SetStereoMode(int StereoMode)
{
	/*if (StereoMode!=GetStereoMode())*/ {
		if (!m_App.CoreEngine.SetStereoMode(StereoMode))
			return false;
		m_App.AppEventManager.OnStereoModeChanged(StereoMode);
	}
	return true;
}


int CUICore::GetAudioStream() const
{
	return m_App.CoreEngine.m_DtvEngine.GetAudioStream();
}


int CUICore::GetNumAudioStreams() const
{
	return m_App.CoreEngine.m_DtvEngine.GetAudioStreamNum();
}


bool CUICore::SetAudioStream(int Stream)
{
	if (Stream!=GetAudioStream()) {
		if (!m_App.CoreEngine.m_DtvEngine.SetAudioStream(Stream))
			return false;
		m_App.AppEventManager.OnAudioStreamChanged(Stream);
	}
	return true;
}


bool CUICore::SwitchStereoMode()
{
	return SetStereoMode((GetStereoMode()+1)%3);
}


bool CUICore::SwitchAudio()
{
	const int NumChannels=m_App.CoreEngine.m_DtvEngine.GetAudioChannelNum();
	bool fResult;

	if (NumChannels==CMediaViewer::AUDIO_CHANNEL_DUALMONO) {
		fResult=SwitchStereoMode();
	} else {
		const int NumStreams=m_App.CoreEngine.m_DtvEngine.GetAudioStreamNum();

		if (NumStreams>1)
			fResult=SetAudioStream((GetAudioStream()+1)%NumStreams);
		else if (NumChannels==2)
			fResult=SwitchStereoMode();
		else
			fResult=false;
	}
	return fResult;
}


int CUICore::FormatCurrentAudioText(LPTSTR pszText,int MaxLength) const
{
	if (pszText==nullptr || MaxLength<1)
		return 0;

	CStaticStringFormatter Formatter(pszText,MaxLength);

	const int NumChannels=m_App.CoreEngine.m_DtvEngine.GetAudioChannelNum();
	if (NumChannels==CMediaViewer::AUDIO_CHANNEL_INVALID)
		return 0;

	const int NumAudio=m_App.CoreEngine.m_DtvEngine.GetAudioStreamNum();
	const int StereoMode=m_App.UICore.GetStereoMode();
	CTsAnalyzer::EventAudioInfo AudioInfo;

	if (NumAudio>1)
		Formatter.AppendFormat(TEXT("#%d: "),m_App.CoreEngine.m_DtvEngine.GetAudioStream()+1);

	if (NumChannels==CMediaViewer::AUDIO_CHANNEL_DUALMONO) {
		// Dual mono
		if (m_App.CoreEngine.m_DtvEngine.GetEventAudioInfo(&AudioInfo)
				&& AudioInfo.bESMultiLingualFlag
				// ES multilingual flag が立っているのに両方日本語の場合がある
				&& AudioInfo.LanguageCode!=AudioInfo.LanguageCode2) {
			// 二カ国語
			TCHAR szLang1[EpgUtil::MAX_LANGUAGE_TEXT_LENGTH];
			TCHAR szLang2[EpgUtil::MAX_LANGUAGE_TEXT_LENGTH];

			Formatter.Append(TEXT("[二] "));

			switch (StereoMode) {
			case 0:
				EpgUtil::GetLanguageText(AudioInfo.LanguageCode,
										 szLang1,lengthof(szLang1),
										 EpgUtil::LANGUAGE_TEXT_SHORT);
				EpgUtil::GetLanguageText(AudioInfo.LanguageCode2,
										 szLang2,lengthof(szLang2),
										 EpgUtil::LANGUAGE_TEXT_SHORT);
				Formatter.AppendFormat(TEXT("%s+%s"),szLang1,szLang2);
				break;
			case 1:
				EpgUtil::GetLanguageText(AudioInfo.LanguageCode,
										 szLang1,lengthof(szLang1),
										 EpgUtil::LANGUAGE_TEXT_SIMPLE);
				Formatter.Append(szLang1);
				break;
			case 2:
				EpgUtil::GetLanguageText(AudioInfo.LanguageCode2,
										 szLang2,lengthof(szLang2),
										 EpgUtil::LANGUAGE_TEXT_SIMPLE);
				Formatter.Append(szLang2);
				break;
			}
		} else {
			Formatter.Append(
				StereoMode==1?TEXT("主音声"):
				StereoMode==2?TEXT("副音声"):
				TEXT("主+副音声"));
		}
	} else if (NumAudio>1 && m_App.CoreEngine.m_DtvEngine.GetEventAudioInfo(&AudioInfo)) {
		switch (NumChannels) {
		case 1:
			Formatter.Append(TEXT("[M]"));
			break;

		case 2:
			Formatter.Append(
				StereoMode==1?TEXT("[S(L)]"):
				StereoMode==2?TEXT("[S(R)]"):
				TEXT("[S]"));
			break;

		case 6:
			Formatter.Append(TEXT("[5.1]"));
			break;

		default:
			Formatter.AppendFormat(TEXT("[%dch]"),NumChannels);
			break;
		}

		TCHAR szAudio[CTsAnalyzer::EventAudioInfo::MAX_TEXT];
		if (AudioInfo.szText[0]!='\0') {
			LPTSTR p=::StrChr(AudioInfo.szText,_T('\r'));
			if (p!=nullptr) {
				TCHAR szBuf[CTsAnalyzer::EventAudioInfo::MAX_TEXT];
				StdUtil::strncpy(szBuf,p-AudioInfo.szText,AudioInfo.szText);
				p++;
				if (*p==_T('\n'))
					p++;
				int Length=::lstrlen(szBuf);
				StdUtil::snprintf(szBuf+Length,lengthof(szBuf)-Length,TEXT("/%s"),p);
				TVTest::StringUtility::ToHalfWidthNoKatakana(
					szBuf,szAudio,lengthof(szAudio));
			} else {
				TVTest::StringUtility::ToHalfWidthNoKatakana(
					AudioInfo.szText,szAudio,lengthof(szAudio));
			}
		} else {
			EpgUtil::GetLanguageText(AudioInfo.LanguageCode,
									 szAudio,lengthof(szAudio),
									 EpgUtil::LANGUAGE_TEXT_SIMPLE);
		}
		Formatter.Append(TEXT(" "));
		Formatter.Append(szAudio);
	} else {
		switch (NumChannels) {
		case 1:
			Formatter.Append(TEXT("Mono"));
			break;

		case 2:
			Formatter.Append(TEXT("Stereo"));
			if (StereoMode!=0)
				Formatter.Append(StereoMode==1?TEXT("(L)"):TEXT("(R)"));
			break;

		case 6:
			Formatter.Append(TEXT("5.1ch"));
			break;

		default:
			Formatter.AppendFormat(TEXT("%dch"),NumChannels);
			break;
		}
	}

	return (int)Formatter.Length();
}


bool CUICore::SetStandby(bool fStandby)
{
	if (m_fStandby!=fStandby) {
		if (m_pSkin!=nullptr) {
			if (!m_pSkin->SetStandby(fStandby))
				return false;
		}
		m_fStandby=fStandby;
		m_App.AppEventManager.OnStandbyChanged(fStandby);
	}
	return true;
}


bool CUICore::GetResident() const
{
	return m_App.ResidentManager.GetResident();
}


bool CUICore::SetResident(bool fResident)
{
	return m_App.ResidentManager.SetResident(fResident);
}


bool CUICore::SetFullscreen(bool fFullscreen)
{
	if (m_fFullscreen!=fFullscreen) {
		if (m_pSkin==nullptr)
			return false;
		if (!m_pSkin->SetFullscreen(fFullscreen))
			return false;
		m_fFullscreen=fFullscreen;
		m_App.AppEventManager.OnFullscreenChanged(fFullscreen);
	}
	return true;
}


bool CUICore::ToggleFullscreen()
{
	return SetFullscreen(!m_fFullscreen);
}


bool CUICore::SetAlwaysOnTop(bool fTop)
{
	if (m_fAlwaysOnTop!=fTop) {
		if (m_pSkin==nullptr)
			return false;
		if (!m_pSkin->SetAlwaysOnTop(fTop))
			return false;
		m_fAlwaysOnTop=fTop;
	}
	return true;
}


bool CUICore::PreventDisplaySave(bool fPrevent)
{
	HWND hwnd=GetMainWindow();

	if (fPrevent) {
		bool fNoScreenSaver=m_App.ViewOptions.GetNoScreenSaver();
		bool fNoMonitorLowPower=m_App.ViewOptions.GetNoMonitorLowPower();
		bool fNoMonitorLowPowerActiveOnly=m_App.ViewOptions.GetNoMonitorLowPowerActiveOnly();

		if (!fNoScreenSaver && m_fScreenSaverActiveOriginal) {
			SystemParametersInfo(SPI_SETSCREENSAVEACTIVE,TRUE,nullptr,
								 SPIF_UPDATEINIFILE/* | SPIF_SENDWININICHANGE*/);
			m_fScreenSaverActiveOriginal=FALSE;
		}
		if (!fNoMonitorLowPower || fNoMonitorLowPowerActiveOnly) {
#if 1
			if (hwnd!=nullptr)
				::KillTimer(hwnd,CUISkin::TIMER_ID_DISPLAY);
#else
			if (m_fPowerOffActiveOriginal) {
				SystemParametersInfo(SPI_SETPOWEROFFACTIVE,TRUE,nullptr,
									 SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
				m_fPowerOffActiveOriginal=FALSE;
			}
			if (m_fLowPowerActiveOriginal) {
				SystemParametersInfo(SPI_SETLOWPOWERACTIVE,TRUE,nullptr,
									 SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
				m_fLowPowerActiveOriginal=FALSE;
			}
#endif
		}
		if (fNoScreenSaver && !m_fScreenSaverActiveOriginal) {
			if (!SystemParametersInfo(SPI_GETSCREENSAVEACTIVE,0,
									  &m_fScreenSaverActiveOriginal,0))
				m_fScreenSaverActiveOriginal=FALSE;
			if (m_fScreenSaverActiveOriginal)
				SystemParametersInfo(SPI_SETSCREENSAVEACTIVE,FALSE,nullptr,
									 0/*SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE*/);
		}
		if (fNoMonitorLowPower && !fNoMonitorLowPowerActiveOnly) {
#if 1
			// SetThreadExecutionState() を呼ぶタイマー
			if (hwnd!=nullptr)
				::SetTimer(hwnd,CUISkin::TIMER_ID_DISPLAY,10000,nullptr);
#else
			if (!m_fPowerOffActiveOriginal) {
				if (!SystemParametersInfo(SPI_GETPOWEROFFACTIVE,0,
										  &m_fPowerOffActiveOriginal,0))
					m_fPowerOffActiveOriginal=FALSE;
				if (m_fPowerOffActiveOriginal)
					SystemParametersInfo(SPI_SETPOWEROFFACTIVE,FALSE,nullptr,
								SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			}
			if (!m_fLowPowerActiveOriginal) {
				if (!SystemParametersInfo(SPI_GETLOWPOWERACTIVE,0,
										  &m_fLowPowerActiveOriginal,0))
					m_fLowPowerActiveOriginal=FALSE;
				if (m_fLowPowerActiveOriginal)
					SystemParametersInfo(SPI_SETLOWPOWERACTIVE,FALSE,nullptr,
								SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			}
#endif
		}
	} else {
		if (hwnd!=nullptr)
			::KillTimer(hwnd,CUISkin::TIMER_ID_DISPLAY);
		if (m_fScreenSaverActiveOriginal) {
			::SystemParametersInfo(SPI_SETSCREENSAVEACTIVE,TRUE,nullptr,
								SPIF_UPDATEINIFILE/* | SPIF_SENDWININICHANGE*/);
			m_fScreenSaverActiveOriginal=FALSE;
		}
#if 0
		if (m_fPowerOffActiveOriginal) {
			::SystemParametersInfo(SPI_SETPOWEROFFACTIVE,TRUE,nullptr,
								   SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			m_fPowerOffActiveOriginal=FALSE;
		}
		if (m_fLowPowerActiveOriginal) {
			::SystemParametersInfo(SPI_SETLOWPOWERACTIVE,TRUE,nullptr,
								   SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			m_fLowPowerActiveOriginal=FALSE;
		}
#endif
	}
	return true;
}


void CUICore::PopupMenu(const POINT *pPos,unsigned int Flags)
{
	POINT pt;

	if (pPos!=nullptr)
		pt=*pPos;
	else
		::GetCursorPos(&pt);

	const bool fDefault=(Flags&POPUPMENU_DEFAULT)!=0;
	std::vector<int> ItemList;
	if (!fDefault)
		m_App.MenuOptions.GetMenuItemList(&ItemList);

	m_App.MainMenu.Show(TPM_RIGHTBUTTON,pt.x,pt.y,m_pSkin->GetMainWindow(),true,
						fDefault?nullptr:&ItemList);
}


void CUICore::PopupSubMenu(int SubMenu,const POINT *pPos,UINT Flags)
{
	POINT pt;

	if (pPos!=nullptr)
		pt=*pPos;
	else
		::GetCursorPos(&pt);
	m_App.MainMenu.PopupSubMenu(SubMenu,Flags,pt.x,pt.y,m_pSkin->GetMainWindow());
}


bool CUICore::ShowSpecialMenu(MenuType Menu,const POINT *pPos,UINT Flags)
{
	POINT pt;

	if (pPos!=nullptr)
		pt=*pPos;
	else
		::GetCursorPos(&pt);
	switch (Menu) {
	case MENU_TUNERSELECT:
		m_App.TunerSelectMenu.Create(GetMainWindow());
		m_App.TunerSelectMenu.Show(Flags,pt.x,pt.y);
		m_App.TunerSelectMenu.Destroy();
		break;

	case MENU_RECORD:
		{
			CPopupMenu Menu(m_App.GetResourceInstance(),IDM_RECORD);

			Menu.CheckItem(CM_RECORDEVENT,m_App.RecordManager.GetStopOnEventEnd());
			Menu.EnableItem(CM_RECORD_PAUSE,m_App.RecordManager.IsRecording());
			Menu.CheckItem(CM_RECORD_PAUSE,m_App.RecordManager.IsPaused());
			bool fTimeShift=m_App.RecordOptions.IsTimeShiftRecordingEnabled();
			Menu.EnableItem(CM_TIMESHIFTRECORDING,fTimeShift && !m_App.RecordManager.IsRecording());
			Menu.CheckItem(CM_ENABLETIMESHIFTRECORDING,fTimeShift);
			Menu.EnableItem(CM_SHOWRECORDREMAINTIME,
				m_App.RecordManager.IsRecording() && m_App.RecordManager.IsStopTimeSpecified());
			Menu.CheckItem(CM_SHOWRECORDREMAINTIME,m_App.RecordOptions.GetShowRemainTime());
			Menu.CheckItem(CM_EXITONRECORDINGSTOP,m_App.Core.GetExitOnRecordingStop());
			m_App.Accelerator.SetMenuAccel(Menu.GetPopupHandle());
			Menu.Show(GetMainWindow(),&pt,Flags);
		}
		break;

	case MENU_CAPTURE:
		{
			CPopupMenu Menu(m_App.GetResourceInstance(),IDM_CAPTURE);

			Menu.CheckRadioItem(CM_CAPTURESIZE_FIRST,CM_CAPTURESIZE_LAST,
				CM_CAPTURESIZE_FIRST+m_App.CaptureOptions.GetPresetCaptureSize());
			Menu.CheckItem(CM_CAPTUREPREVIEW,m_App.CaptureWindow.GetVisible());
			m_App.Accelerator.SetMenuAccel(Menu.GetPopupHandle());
			Menu.Show(GetMainWindow(),&pt,Flags);
		}
		break;

	case MENU_BUFFERING:
		{
			CPopupMenu Menu(m_App.GetResourceInstance(),IDM_BUFFERING);

			Menu.CheckItem(CM_ENABLEBUFFERING,m_App.CoreEngine.GetPacketBuffering());
			Menu.Show(GetMainWindow(),&pt,Flags);
		}
		break;

	case MENU_STREAMERROR:
		{
			CPopupMenu Menu(m_App.GetResourceInstance(),IDM_ERROR);

			Menu.Show(GetMainWindow(),&pt,Flags);
		}
		break;

	case MENU_CLOCK:
		{
			CPopupMenu Menu(m_App.GetResourceInstance(),IDM_TIME);

			Menu.CheckItem(CM_SHOWTOTTIME,m_App.StatusOptions.GetShowTOTTime());
			Menu.Show(GetMainWindow(),&pt,Flags);
		}
		break;

	case MENU_PROGRAMINFO:
		{
			CPopupMenu Menu(m_App.GetResourceInstance(),IDM_PROGRAMINFOSTATUS);

			Menu.CheckItem(CM_PROGRAMINFOSTATUS_POPUPINFO,
						   m_App.StatusOptions.IsPopupProgramInfoEnabled());
			Menu.CheckItem(CM_PROGRAMINFOSTATUS_SHOWPROGRESS,
						   m_App.StatusOptions.GetShowEventProgress());
			Menu.Show(GetMainWindow(),&pt,Flags);
		}
		break;

	default:
		return false;
	}

	return true;
}


void CUICore::InitChannelMenu(HMENU hmenu)
{
#ifdef NETWORK_REMOCON_SUPPORT
	if (m_App.pNetworkRemocon!=nullptr) {
		InitNetworkRemoconChannelMenu(hmenu);
		return;
	}
#endif

	const CChannelList *pList=m_App.ChannelManager.GetCurrentChannelList();

	m_App.ChannelMenu.Destroy();
	ClearMenu(hmenu);
	if (pList==nullptr)
		return;

	if (!m_App.CoreEngine.IsNetworkDriver()) {
		m_App.ChannelMenuManager.CreateChannelMenu(pList,m_App.ChannelManager.GetCurrentChannel(),
											 CM_CHANNEL_FIRST,hmenu,GetMainWindow());
	} else {
		bool fControlKeyID=pList->HasRemoteControlKeyID();
		for (int i=0,j=0;i<pList->NumChannels();i++) {
			const CChannelInfo *pChInfo=pList->GetChannelInfo(i);
			TCHAR szText[MAX_CHANNEL_NAME+4];

			if (pChInfo->IsEnabled()) {
				StdUtil::snprintf(szText,lengthof(szText),TEXT("%d: %s"),
								  fControlKeyID?pChInfo->GetChannelNo():i+1,pChInfo->GetName());
				::AppendMenu(hmenu,MF_STRING | MF_ENABLED
							 | (j!=0 && j%m_App.MenuOptions.GetMaxChannelMenuRows()==0?MF_MENUBREAK:0),
							 CM_CHANNEL_FIRST+i,szText);
				j++;
			}
		}
		if (m_App.ChannelManager.GetCurrentChannel()>=0
				&& pList->IsEnabled(m_App.ChannelManager.GetCurrentChannel())) {
			::CheckMenuRadioItem(hmenu,CM_CHANNEL_FIRST,
				CM_CHANNEL_FIRST+pList->NumChannels()-1,
				CM_CHANNEL_FIRST+m_App.ChannelManager.GetCurrentChannel(),
				MF_BYCOMMAND);
		}
	}
}


#ifdef NETWORK_REMOCON_SUPPORT
void CUICore::InitNetworkRemoconChannelMenu(HMENU hmenu)
{
	const CChannelList &RemoconChList=m_App.pNetworkRemocon->GetChannelList();
	int i;
	TCHAR szText[MAX_CHANNEL_NAME+4];
	const CChannelList *pPortList;

	ClearMenu(hmenu);
	if (RemoconChList.NumChannels()>0) {
		int No,Min,Max;

		Min=1000;
		Max=0;
		for (i=0;i<RemoconChList.NumChannels();i++) {
			No=RemoconChList.GetChannelNo(i);
			if (No<Min)
				Min=No;
			if (No>Max)
				Max=No;
		}
		for (No=Min;No<=Max;No++) {
			for (i=0;i<RemoconChList.NumChannels();i++) {
				if (RemoconChList.GetChannelNo(i)==No) {
					StdUtil::snprintf(szText,lengthof(szText),
									  TEXT("%d: %s"),No,RemoconChList.GetName(i));
					AppendMenu(hmenu,MF_STRING | MF_ENABLED,
							   CM_CHANNELNO_FIRST+No-1,szText);
				}
			}
		}
		if (m_App.ChannelManager.GetNetworkRemoconCurrentChannel()>=0) {
			::CheckMenuRadioItem(hmenu,
				CM_CHANNELNO_FIRST,CM_CHANNELNO_FIRST+Max-1,
				CM_CHANNEL_FIRST+m_App.ChannelManager.GetNetworkRemoconCurrentChannel(),
				MF_BYCOMMAND);
		}
	}
	pPortList=m_App.ChannelManager.GetDriverChannelList(0);
	for (i=0;i<pPortList->NumChannels();i++) {
		StdUtil::snprintf(szText,lengthof(szText),TEXT("%d: %s"),
						  pPortList->GetChannelNo(i),pPortList->GetName(i));
		AppendMenu(hmenu,MF_STRING | MF_ENABLED
			| ((i!=0 && i%m_App.MenuOptions.GetMaxChannelMenuRows()==0) || (i==0 && RemoconChList.NumChannels()>0)?MF_MENUBREAK:0),
			CM_CHANNEL_FIRST+i,szText);
	}
	if (m_App.ChannelManager.GetCurrentChannel()>=0) {
		::CheckMenuRadioItem(hmenu,
			CM_CHANNEL_FIRST,CM_CHANNEL_FIRST+pPortList->NumChannels()-1,
			CM_CHANNEL_FIRST+m_App.ChannelManager.GetCurrentChannel(),
			MF_BYCOMMAND);
	}
}
#endif	// NETWORK_REMOCON_SUPPORT


void CUICore::InitTunerMenu(HMENU hmenu)
{
	m_App.ChannelMenu.Destroy();

	CPopupMenu Menu(hmenu);
	Menu.Clear();

	TCHAR szText[256];
	int Length;
	int i;

	// 各チューニング空間のメニューを追加する
	// 実際のメニューの設定は WM_INITMENUPOPUP で行っている
	if (m_App.ChannelManager.NumSpaces()>0) {
		HMENU hmenuSpace;
		LPCTSTR pszName;

		if (m_App.ChannelManager.NumSpaces()>1) {
			hmenuSpace=::CreatePopupMenu();
			Menu.Append(hmenuSpace,TEXT("&A: すべて"));
		}
		for (i=0;i<m_App.ChannelManager.NumSpaces();i++) {
			const CChannelList *pChannelList=m_App.ChannelManager.GetChannelList(i);

			hmenuSpace=::CreatePopupMenu();
			Length=StdUtil::snprintf(szText,lengthof(szText),TEXT("&%d: "),i);
			pszName=m_App.ChannelManager.GetTuningSpaceName(i);
			if (pszName!=nullptr)
				CopyToMenuText(pszName,szText+Length,lengthof(szText)-Length);
			else
				StdUtil::snprintf(szText+Length,lengthof(szText)-Length,
								  TEXT("チューニング空間%d"),i);
			Menu.Append(hmenuSpace,szText,
						pChannelList->NumEnableChannels()>0?MF_ENABLED:MF_GRAYED);
		}

		Menu.AppendSeparator();
	}

	::LoadString(m_App.GetResourceInstance(),CM_CHANNELDISPLAY,szText,lengthof(szText));
	Menu.Append(CM_CHANNELDISPLAY,szText,
				MF_ENABLED | (m_App.ChannelDisplay.GetVisible()?MF_CHECKED:MF_UNCHECKED));
	::AppendMenu(hmenu,MF_SEPARATOR,0,nullptr);
	int CurDriver=-1;
	for (i=0;i<m_App.DriverManager.NumDrivers();i++) {
		const CDriverInfo *pDriverInfo=m_App.DriverManager.GetDriverInfo(i);
		::lstrcpyn(szText,pDriverInfo->GetFileName(),lengthof(szText));
		::PathRemoveExtension(szText);
		Menu.AppendUnformatted(CM_DRIVER_FIRST+i,szText);
		if (m_App.CoreEngine.IsTunerOpen()
				&& IsEqualFileName(pDriverInfo->GetFileName(),m_App.CoreEngine.GetDriverFileName()))
			CurDriver=i;
	}
	if (CurDriver<0 && m_App.CoreEngine.IsTunerOpen()) {
		Menu.AppendUnformatted(CM_DRIVER_FIRST+i,m_App.CoreEngine.GetDriverFileName());
		CurDriver=i++;
	}
	Menu.Append(CM_DRIVER_BROWSE,TEXT("参照..."));
	if (CurDriver>=0)
		Menu.CheckRadioItem(CM_DRIVER_FIRST,CM_DRIVER_FIRST+i-1,
							CM_DRIVER_FIRST+CurDriver);
	m_App.Accelerator.SetMenuAccel(hmenu);
}


bool CUICore::ProcessTunerMenu(int Command)
{
	if (Command<CM_SPACE_CHANNEL_FIRST || Command>CM_SPACE_CHANNEL_LAST)
		return false;

	const CChannelList *pChannelList;
	int CommandBase;
	int i,j;

	CommandBase=CM_SPACE_CHANNEL_FIRST;
	pChannelList=m_App.ChannelManager.GetAllChannelList();
	if (pChannelList->NumChannels()>0) {
		if (Command-CommandBase<pChannelList->NumChannels())
			return m_App.Core.SetChannel(-1,Command-CommandBase);
		CommandBase+=pChannelList->NumChannels();
	}
	for (int i=0;i<m_App.ChannelManager.NumSpaces();i++) {
		pChannelList=m_App.ChannelManager.GetChannelList(i);
		if (Command-CommandBase<pChannelList->NumChannels())
			return m_App.Core.SetChannel(i,Command-CommandBase);
		CommandBase+=pChannelList->NumChannels();
	}
	for (i=0;i<m_App.DriverManager.NumDrivers();i++) {
		const CDriverInfo *pDriverInfo=m_App.DriverManager.GetDriverInfo(i);

		if (IsEqualFileName(pDriverInfo->GetFileName(),m_App.CoreEngine.GetDriverFileName()))
			continue;
		if (pDriverInfo->IsTuningSpaceListLoaded()) {
			const CTuningSpaceList *pTuningSpaceList=pDriverInfo->GetAvailableTuningSpaceList();

			for (j=0;j<pTuningSpaceList->NumSpaces();j++) {
				pChannelList=pTuningSpaceList->GetChannelList(j);
				if (Command-CommandBase<pChannelList->NumChannels()) {
					if (!m_App.Core.OpenTuner(pDriverInfo->GetFileName()))
						return false;
					return m_App.Core.SetChannel(j,Command-CommandBase);
				}
				CommandBase+=pChannelList->NumChannels();
			}
		}
	}
	return false;
}


bool CUICore::DoCommand(int Command)
{
	if (m_pSkin==nullptr || Command<=0 || Command>0xFFFF)
		return false;
	::SendMessage(m_pSkin->GetMainWindow(),WM_COMMAND,MAKEWPARAM(Command,0),0);
	return true;
}


bool CUICore::DoCommand(LPCTSTR pszCommand)
{
	if (pszCommand==nullptr)
		return false;
	int Command=m_App.CommandList.ParseText(pszCommand);
	if (Command==0)
		return false;
	return DoCommand(Command);
}


bool CUICore::DoCommandAsync(int Command)
{
	if (m_pSkin==nullptr || Command<=0 || Command>0xFFFF)
		return false;
	::PostMessage(m_pSkin->GetMainWindow(),WM_COMMAND,MAKEWPARAM(Command,0),0);
	return true;
}


bool CUICore::DoCommandAsync(LPCTSTR pszCommand)
{
	if (pszCommand==nullptr)
		return false;
	int Command=m_App.CommandList.ParseText(pszCommand);
	if (Command==0)
		return false;
	return DoCommandAsync(Command);
}


bool CUICore::ConfirmChannelChange()
{
	if (m_App.RecordManager.IsRecording()) {
		if (!m_App.RecordOptions.ConfirmChannelChange(GetDialogOwner()))
			return false;
	}
	return true;
}


bool CUICore::ConfirmStopRecording()
{
	return m_App.RecordOptions.ConfirmStatusBarStop(GetDialogOwner());
}


bool CUICore::UpdateIcon()
{
	HICON hicoBig=nullptr,hicoSmall=nullptr;

	if (m_App.ViewOptions.GetUseLogoIcon() && m_App.CoreEngine.IsTunerOpen()) {
		const CChannelInfo *pCurChannel=m_App.ChannelManager.GetCurrentChannelInfo();

		if (pCurChannel!=nullptr) {
			hicoBig=m_App.LogoManager.CreateLogoIcon(
				pCurChannel->GetNetworkID(),
				pCurChannel->GetServiceID(),
				::GetSystemMetrics(SM_CXICON),
				::GetSystemMetrics(SM_CYICON));
			hicoSmall=m_App.LogoManager.CreateLogoIcon(
				pCurChannel->GetNetworkID(),
				pCurChannel->GetServiceID(),
				::GetSystemMetrics(SM_CXSMICON),
				::GetSystemMetrics(SM_CYSMICON));
		}
	}
	HWND hwnd=GetMainWindow();
	if (hwnd!=nullptr) {
		HICON hicoDefault;

		if (hicoBig==nullptr || hicoSmall==nullptr)
			hicoDefault=::LoadIcon(m_App.GetInstance(),MAKEINTRESOURCE(IDI_ICON));
		::SendMessage(hwnd,WM_SETICON,ICON_BIG,
					  reinterpret_cast<LPARAM>(hicoBig!=nullptr?hicoBig:hicoDefault));
		::SendMessage(hwnd,WM_SETICON,ICON_SMALL,
					  reinterpret_cast<LPARAM>(hicoSmall!=nullptr?hicoSmall:hicoDefault));
	}
	if (m_hicoLogoBig!=nullptr)
		::DestroyIcon(m_hicoLogoBig);
	m_hicoLogoBig=hicoBig;
	if (m_hicoLogoSmall!=nullptr)
		::DestroyIcon(m_hicoLogoSmall);
	m_hicoLogoSmall=hicoSmall;
	return true;
}


bool CUICore::UpdateTitle()
{
#define MAIN_TITLE_TEXT APP_NAME

	HWND hwnd=GetMainWindow();

	if (hwnd==nullptr)
		return false;

	TCHAR szText[256],szOld[256],szService[MAX_CHANNEL_NAME];
	CStaticStringFormatter Formatter(szText,lengthof(szText));
	LPCTSTR pszText;

	// TODO: ユーザーが "%service-name% / %event-time% %event-name%" のようにフォーマットを指定できるようにする
	if (m_App.Core.GetCurrentServiceName(szService,lengthof(szService))) {
		Formatter.AppendFormat(
			MAIN_TITLE_TEXT TEXT(" %s %s"),
			m_App.RecordManager.IsRecording()?TEXT("●"):TEXT("-"),
			szService);

		if (m_App.CoreEngine.IsTunerOpen()) {
			TCHAR szTime[EpgUtil::MAX_EVENT_TIME_LENGTH+1],szEvent[256];

			szTime[0]=_T('\0');
			if (m_App.ViewOptions.GetShowTitleEventTime()) {
				SYSTEMTIME StartTime;
				DWORD Duration;

				if (m_App.CoreEngine.m_DtvEngine.GetEventTime(&StartTime,&Duration)) {
					if (EpgUtil::FormatEventTime(StartTime,Duration,szTime,lengthof(szTime)-1)>0)
						::lstrcat(szTime,TEXT(" "));
				}
			}
			if (m_App.CoreEngine.m_DtvEngine.GetEventName(szEvent,lengthof(szEvent))<1)
				szEvent[0]=_T('\0');
			if (szTime[0]!=_T('\0') || szEvent[0]!=_T('\0'))
				Formatter.AppendFormat(TEXT(" / %s%s"),szTime,szEvent);
		}

		pszText=Formatter.GetString();
	} else {
		pszText=MAIN_TITLE_TEXT;
	}

	if (::GetWindowText(hwnd,szOld,lengthof(szOld))<1
			|| ::lstrcmp(pszText,szOld)!=0) {
		::SetWindowText(hwnd,pszText);
		m_App.ResidentManager.SetTipText(pszText);
	}

	return true;
}


bool CUICore::SetLogo(LPCTSTR pszFileName)
{
	if (m_pSkin==nullptr)
		return false;

	if (IsStringEmpty(pszFileName))
		return m_pSkin->SetLogo(nullptr);

	TCHAR szFileName[MAX_PATH];

	if (::PathIsRelative(pszFileName)) {
		TCHAR szTemp[MAX_PATH];
		m_App.GetAppDirectory(szTemp);
		::PathAppend(szTemp,pszFileName);
		::PathCanonicalize(szFileName,szTemp);
	} else {
		::lstrcpy(szFileName,pszFileName);
	}

	HBITMAP hbm=static_cast<HBITMAP>(
		::LoadImage(nullptr,szFileName,IMAGE_BITMAP,
					0,0,LR_LOADFROMFILE | LR_CREATEDIBSECTION));
	if (hbm==nullptr)
		return false;

	if (!m_pSkin->SetLogo(hbm)) {
		::DeleteObject(hbm);
		return false;
	}

	return true;
}


bool CUICore::RegisterModelessDialog(CBasicDialog *pDialog)
{
	if (pDialog==nullptr)
		return false;
	if (std::find(m_ModelessDialogList.begin(),m_ModelessDialogList.end(),pDialog)!=m_ModelessDialogList.end())
		return false;
	m_ModelessDialogList.push_back(pDialog);
	return true;
}


bool CUICore::UnregisterModelessDialog(CBasicDialog *pDialog)
{
	auto itr=std::find(m_ModelessDialogList.begin(),m_ModelessDialogList.end(),pDialog);
	if (itr==m_ModelessDialogList.end())
		return false;
	m_ModelessDialogList.erase(itr);
	return true;
}


bool CUICore::ProcessDialogMessage(MSG *pMessage)
{
	for (auto itr=m_ModelessDialogList.begin();itr!=m_ModelessDialogList.end();++itr) {
		if ((*itr)->ProcessMessage(pMessage))
			return true;
	}
	return false;
}
