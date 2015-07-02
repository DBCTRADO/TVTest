#include "stdafx.h"
#include <algorithm>
#include "TVTest.h"
#include "AppMain.h"
#include "resource.h"
#include "Common/DebugDef.h"




CUICore::CUICore(CAppMain &App)
	: m_App(App)
	, m_pSkin(nullptr)
	, m_fStandby(false)
	, m_fTransientStandby(false)
	, m_fFullscreen(false)
	, m_fAlwaysOnTop(false)

	, m_fViewerInitializeError(false)

	, m_fScreenSaverActiveOriginal(FALSE)
	/*
	, m_fLowPowerActiveOriginal(FALSE)
	, m_fPowerOffActiveOriginal(FALSE)
	*/

	, m_TunerSelectMenu(*this)

	, m_pColorScheme(NULL)

	, m_fStatusBarTrace(false)
{
}


CUICore::~CUICore()
{
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
	int Rate,Factor;

	if (!GetZoomRate(&Rate,&Factor))
		return 0;
	return ::MulDiv(Rate,100,Factor);
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


bool CUICore::SetDualMonoMode(CAudioDecFilter::DualMonoMode Mode)
{
	return SelectDualMonoMode(Mode);
}


CAudioDecFilter::DualMonoMode CUICore::GetDualMonoMode() const
{
	return m_App.CoreEngine.GetDualMonoMode();
}


CAudioDecFilter::DualMonoMode CUICore::GetActualDualMonoMode() const
{
	CAudioDecFilter::DualMonoMode Mode=GetDualMonoMode();

	switch (Mode) {
	case CAudioDecFilter::DUALMONO_MAIN:
	case CAudioDecFilter::DUALMONO_SUB:
	case CAudioDecFilter::DUALMONO_BOTH:
		break;
	default:
		switch (GetStereoMode()) {
		case CAudioDecFilter::STEREOMODE_STEREO:
			Mode=CAudioDecFilter::DUALMONO_BOTH;
			break;
		case CAudioDecFilter::STEREOMODE_LEFT:
			Mode=CAudioDecFilter::DUALMONO_MAIN;
			break;
		case CAudioDecFilter::STEREOMODE_RIGHT:
			Mode=CAudioDecFilter::DUALMONO_SUB;
			break;
		}
		break;
	}

	return Mode;
}


bool CUICore::SetStereoMode(CAudioDecFilter::StereoMode Mode)
{
	if (!m_App.CoreEngine.SetStereoMode(Mode))
		return false;
	m_App.AppEventManager.OnStereoModeChanged(Mode);
	return true;
}


CAudioDecFilter::StereoMode CUICore::GetStereoMode() const
{
	return m_App.CoreEngine.GetStereoMode();
}


int CUICore::GetNumAudioStreams() const
{
	return m_App.CoreEngine.m_DtvEngine.GetAudioStreamNum();
}


int CUICore::GetAudioStream() const
{
	return m_App.CoreEngine.m_DtvEngine.GetAudioStream();
}


bool CUICore::SetAudioStream(int Stream)
{
	const BYTE ComponentTag=m_App.CoreEngine.m_DtvEngine.GetAudioComponentTag(Stream);
	if (ComponentTag==CTsAnalyzer::COMPONENTTAG_INVALID)
		return false;

	TVTest::CAudioManager::AudioSelectInfo SelInfo;
	if (!m_App.AudioManager.GetAudioSelectInfoByComponentTag(ComponentTag,&SelInfo))
		return false;

	return SelectAudio(SelInfo);
}


bool CUICore::SelectAudio(int Index)
{
	TVTest::CAudioManager::AudioInfo Info;

	if (!m_App.AudioManager.GetAudioInfo(Index,&Info))
		return false;

	TVTest::CAudioManager::AudioSelectInfo SelInfo;

	SelInfo.ComponentTag=Info.ComponentTag;
	SelInfo.DualMono=Info.DualMono;

	return SelectAudio(SelInfo);
}


bool CUICore::AutoSelectAudio()
{
	int Index=m_App.AudioManager.FindSelectedAudio();
	if (Index>=0)
		return SelectAudio(Index);

	TVTest::CAudioManager::AudioSelectInfo DefaultAudio;
	if (m_App.AudioManager.GetDefaultAudio(&DefaultAudio)<0)
		return false;

	return SelectAudio(DefaultAudio,false);
}


bool CUICore::SelectAudio(const TVTest::CAudioManager::AudioSelectInfo &Info,bool fUpdate)
{
	const int AudioIndex=
		m_App.CoreEngine.m_DtvEngine.m_TsAnalyzer.GetAudioIndexByComponentTag(
			m_App.CoreEngine.m_DtvEngine.GetServiceIndex(),Info.ComponentTag);

	if (AudioIndex>=0) {
		CAudioDecFilter::DualMonoMode DualMonoMode=CAudioDecFilter::DUALMONO_INVALID;

		switch (Info.DualMono) {
		case TVTest::CAudioManager::DUALMONO_MAIN:
			DualMonoMode=CAudioDecFilter::DUALMONO_MAIN;
			break;
		case TVTest::CAudioManager::DUALMONO_SUB:
			DualMonoMode=CAudioDecFilter::DUALMONO_SUB;
			break;
		case TVTest::CAudioManager::DUALMONO_BOTH:
			DualMonoMode=CAudioDecFilter::DUALMONO_BOTH;
			break;
		}

		SelectAudioStream(AudioIndex);
		if (DualMonoMode!=CAudioDecFilter::DUALMONO_INVALID)
			SelectDualMonoMode(DualMonoMode,fUpdate);
	}

	if (fUpdate)
		m_App.AudioManager.SetSelectedComponentTag(Info.ComponentTag);

	return true;
}


bool CUICore::SelectAudioStream(int Stream)
{
	if (Stream!=GetAudioStream()) {
		if (!m_App.CoreEngine.m_DtvEngine.SetAudioStream(Stream))
			return false;
		m_App.AppEventManager.OnAudioStreamChanged(Stream);
	}

	return true;
}


bool CUICore::SelectDualMonoMode(CAudioDecFilter::DualMonoMode Mode,bool fUpdate)
{
	if (!m_App.CoreEngine.SetDualMonoMode(Mode))
		return false;
	if (fUpdate) {
		m_App.AudioManager.SetSelectedDualMonoMode(
			Mode==CAudioDecFilter::DUALMONO_MAIN?TVTest::CAudioManager::DUALMONO_MAIN:
			Mode==CAudioDecFilter::DUALMONO_SUB ?TVTest::CAudioManager::DUALMONO_SUB:
			Mode==CAudioDecFilter::DUALMONO_BOTH?TVTest::CAudioManager::DUALMONO_BOTH:
			                                     TVTest::CAudioManager::DUALMONO_INVALID);
	}
	m_App.AppEventManager.OnDualMonoModeChanged(Mode);
	return true;
}


bool CUICore::SwitchAudio()
{
	const int NumChannels=m_App.CoreEngine.m_DtvEngine.GetAudioChannelNum();
	bool fResult;

	if (NumChannels==CMediaViewer::AUDIO_CHANNEL_DUALMONO) {
		fResult=SwitchDualMonoMode();
	} else {
		const int NumStreams=GetNumAudioStreams();

		if (NumStreams>1)
			fResult=SetAudioStream((GetAudioStream()+1)%NumStreams);
		else
			fResult=false;
	}

	return fResult;
}


bool CUICore::SwitchDualMonoMode()
{
	CAudioDecFilter::DualMonoMode Mode;

	switch (GetDualMonoMode()) {
	case CAudioDecFilter::DUALMONO_MAIN:
		Mode=CAudioDecFilter::DUALMONO_SUB;
		break;
	case CAudioDecFilter::DUALMONO_SUB:
		Mode=CAudioDecFilter::DUALMONO_BOTH;
		break;
	case CAudioDecFilter::DUALMONO_BOTH:
	default:
		Mode=CAudioDecFilter::DUALMONO_MAIN;
		break;
	}

	return SetDualMonoMode(Mode);
}


int CUICore::FormatCurrentAudioText(LPTSTR pszText,int MaxLength) const
{
	if (pszText==nullptr || MaxLength<1)
		return 0;

	CStaticStringFormatter Formatter(pszText,MaxLength);

	const int NumChannels=m_App.CoreEngine.m_DtvEngine.GetAudioChannelNum();
	if (NumChannels==CMediaViewer::AUDIO_CHANNEL_INVALID)
		return 0;

	const int NumAudio=GetNumAudioStreams();
	const CAudioDecFilter::StereoMode StereoMode=GetStereoMode();
	CTsAnalyzer::EventAudioInfo AudioInfo;

	if (NumAudio>1)
		Formatter.AppendFormat(TEXT("#%d: "),GetAudioStream()+1);

	if (NumChannels==CMediaViewer::AUDIO_CHANNEL_DUALMONO) {
		// Dual mono
		const CAudioDecFilter::DualMonoMode DualMonoMode=GetActualDualMonoMode();

		if (m_App.CoreEngine.m_DtvEngine.GetEventAudioInfo(&AudioInfo)
				&& AudioInfo.ComponentType==0x02
				&& AudioInfo.bESMultiLingualFlag
				// ES multilingual flag が立っているのに両方日本語の場合がある
				&& AudioInfo.LanguageCode!=AudioInfo.LanguageCode2) {
			// 二カ国語
			TCHAR szLang1[EpgUtil::MAX_LANGUAGE_TEXT_LENGTH];
			TCHAR szLang2[EpgUtil::MAX_LANGUAGE_TEXT_LENGTH];

			Formatter.Append(TEXT("[二] "));

			switch (DualMonoMode) {
			case CAudioDecFilter::DUALMONO_MAIN:
				EpgUtil::GetLanguageText(AudioInfo.LanguageCode,
										 szLang1,lengthof(szLang1),
										 EpgUtil::LANGUAGE_TEXT_SIMPLE);
				Formatter.Append(szLang1);
				break;
			case CAudioDecFilter::DUALMONO_SUB:
				EpgUtil::GetLanguageText(AudioInfo.LanguageCode2,
										 szLang2,lengthof(szLang2),
										 EpgUtil::LANGUAGE_TEXT_SIMPLE);
				Formatter.Append(szLang2);
				break;
			case CAudioDecFilter::DUALMONO_BOTH:
				EpgUtil::GetLanguageText(AudioInfo.LanguageCode,
										 szLang1,lengthof(szLang1),
										 EpgUtil::LANGUAGE_TEXT_SHORT);
				EpgUtil::GetLanguageText(AudioInfo.LanguageCode2,
										 szLang2,lengthof(szLang2),
										 EpgUtil::LANGUAGE_TEXT_SHORT);
				Formatter.AppendFormat(TEXT("%s+%s"),szLang1,szLang2);
				break;
			}
		} else {
			Formatter.Append(
				DualMonoMode==CAudioDecFilter::DUALMONO_MAIN?
					TEXT("主音声"):
				DualMonoMode==CAudioDecFilter::DUALMONO_SUB?
					TEXT("副音声"):
					TEXT("主+副音声"));
		}
	} else if (NumAudio>1 && m_App.CoreEngine.m_DtvEngine.GetEventAudioInfo(&AudioInfo)) {
		TCHAR szFormat[16];

		switch (NumChannels) {
		case 1:
			::lstrcpy(szFormat,TEXT("[M]"));
			break;

		case 2:
			::lstrcpy(szFormat,
				StereoMode==CAudioDecFilter::STEREOMODE_LEFT?
					TEXT("[S(L)]"):
				StereoMode==CAudioDecFilter::STEREOMODE_RIGHT?
					TEXT("[S(R)]"):
					TEXT("[S]"));
			break;

		case 6:
			::lstrcpy(szFormat,TEXT("[5.1]"));
			break;

		default:
			StdUtil::snprintf(szFormat,lengthof(szFormat),TEXT("[%dch]"),NumChannels);
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

			// [S] などがあれば除去する
			p=::StrStrI(szAudio,szFormat);
			if (p!=NULL) {
				int Length=::lstrlen(szFormat);
				if (p>szAudio && *(p-1)==_T(' ')) {
					p--;
					Length++;
				}
				if (p[Length]==_T(' '))
					Length++;
				if (p==szAudio && ::lstrlen(szAudio)==Length) {
					EpgUtil::GetLanguageText(AudioInfo.LanguageCode,
											 szAudio,lengthof(szAudio),
											 EpgUtil::LANGUAGE_TEXT_SIMPLE);
				} else {
					std::memmove(p,p+Length,(::lstrlen(p+Length)+1)*sizeof(TCHAR));
				}
			}
		} else {
			EpgUtil::GetLanguageText(AudioInfo.LanguageCode,
									 szAudio,lengthof(szAudio),
									 EpgUtil::LANGUAGE_TEXT_SIMPLE);
		}

		Formatter.AppendFormat(TEXT("%s %s"),szFormat,szAudio);
	} else {
		switch (NumChannels) {
		case 1:
			Formatter.Append(TEXT("Mono"));
			break;

		case 2:
			Formatter.Append(TEXT("Stereo"));
			if (StereoMode!=CAudioDecFilter::STEREOMODE_STEREO)
				Formatter.Append(StereoMode==CAudioDecFilter::STEREOMODE_LEFT?TEXT("(L)"):TEXT("(R)"));
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


bool CUICore::GetSelectedAudioText(LPTSTR pszText,int MaxLength) const
{
	if (pszText==nullptr || MaxLength<1)
		return false;

	CTsAnalyzer::EventAudioInfo AudioInfo;

	if (m_App.CoreEngine.m_DtvEngine.GetEventAudioInfo(&AudioInfo)) {
		if (AudioInfo.ComponentType==0x02) {
			// Dual mono
			TCHAR szAudio1[64],szAudio2[64];

			szAudio1[0]=_T('\0');
			szAudio2[0]=_T('\0');
			if (AudioInfo.szText[0]!=_T('\0')) {
				LPTSTR pszDelimiter=::StrChr(AudioInfo.szText,_T('\r'));
				if (pszDelimiter!=nullptr) {
					*pszDelimiter=_T('\0');
					if (*(pszDelimiter+1)==_T('\n'))
						pszDelimiter++;
					::lstrcpyn(szAudio1,AudioInfo.szText,lengthof(szAudio1));
					::lstrcpyn(szAudio2,pszDelimiter+1,lengthof(szAudio2));
				}
			}
			if (AudioInfo.bESMultiLingualFlag
					&& AudioInfo.LanguageCode!=AudioInfo.LanguageCode2) {
				// 二カ国語
				if (szAudio1[0]==_T('\0'))
					EpgUtil::GetLanguageText(AudioInfo.LanguageCode,szAudio1,lengthof(szAudio1));
				if (szAudio2[0]==_T('\0'))
					EpgUtil::GetLanguageText(AudioInfo.LanguageCode2,szAudio2,lengthof(szAudio2));
			} else {
				if (szAudio1[0]==_T('\0'))
					::lstrcpy(szAudio1,TEXT("主音声"));
				if (szAudio2[0]==_T('\0'))
					::lstrcpy(szAudio2,TEXT("副音声"));
			}
			switch (GetActualDualMonoMode()) {
			case CAudioDecFilter::DUALMONO_MAIN:
				::lstrcpyn(pszText,szAudio1,MaxLength);
				break;
			case CAudioDecFilter::DUALMONO_SUB:
				::lstrcpyn(pszText,szAudio2,MaxLength);
				break;
			case CAudioDecFilter::DUALMONO_BOTH:
				StdUtil::snprintf(pszText,MaxLength,TEXT("%s+%s"),szAudio1,szAudio2);
				break;
			default:
				return false;
			}
		} else {
			if (AudioInfo.szText[0]==_T('\0')) {
				EpgUtil::GetLanguageText(AudioInfo.LanguageCode,
										 AudioInfo.szText,lengthof(AudioInfo.szText));
			}
			StdUtil::snprintf(pszText,MaxLength,TEXT("音声%d: %s"),
							  GetAudioStream()+1,AudioInfo.szText);
		}
	} else {
		StdUtil::snprintf(pszText,MaxLength,TEXT("音声%d"),GetAudioStream()+1);
	}

	return true;
}


bool CUICore::SetStandby(bool fStandby,bool fTransient)
{
	if (m_fStandby!=fStandby) {
		if (m_pSkin!=nullptr) {
			if (!m_pSkin->SetStandby(fStandby))
				return false;
		}
		m_fStandby=fStandby;
		m_fTransientStandby=fStandby && fTransient;
		m_App.TaskTrayManager.SetStatus(
			fStandby?TVTest::CTaskTrayManager::STATUS_STANDBY:0,
			TVTest::CTaskTrayManager::STATUS_STANDBY);
		m_App.AppEventManager.OnStandbyChanged(fStandby);
	}
	return true;
}


bool CUICore::GetResident() const
{
	return m_fResident;
}


bool CUICore::SetResident(bool fResident)
{
	if (m_fResident!=fResident) {
		if (!m_App.TaskTrayManager.SetResident(fResident))
			return false;
		m_fResident=fResident;
	}
	return true;
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


void CUICore::PopupSubMenu(int SubMenu,const POINT *pPos,UINT Flags,const RECT *pExcludeRect)
{
	m_App.MainMenu.PopupSubMenu(SubMenu,Flags,m_pSkin->GetMainWindow(),pPos,true,pExcludeRect);
}


bool CUICore::ShowSpecialMenu(MenuType Menu,const POINT *pPos,UINT Flags,const RECT *pExcludeRect)
{
	POINT pt;

	if (pPos!=nullptr)
		pt=*pPos;
	else
		::GetCursorPos(&pt);

	switch (Menu) {
	case MENU_TUNERSELECT:
		m_TunerSelectMenu.Create(GetMainWindow());
		m_TunerSelectMenu.Show(Flags,pt.x,pt.y,pExcludeRect);
		m_TunerSelectMenu.Destroy();
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
			Menu.Show(GetMainWindow(),&pt,Flags,pExcludeRect);
		}
		break;

	case MENU_CAPTURE:
		{
			CPopupMenu Menu(m_App.GetResourceInstance(),IDM_CAPTURE);

			Menu.CheckRadioItem(CM_CAPTURESIZE_FIRST,CM_CAPTURESIZE_LAST,
				CM_CAPTURESIZE_FIRST+m_App.CaptureOptions.GetPresetCaptureSize());
			Menu.CheckItem(CM_CAPTUREPREVIEW,m_App.CaptureWindow.GetVisible());
			m_App.Accelerator.SetMenuAccel(Menu.GetPopupHandle());
			Menu.Show(GetMainWindow(),&pt,Flags,pExcludeRect);
		}
		break;

	case MENU_BUFFERING:
		{
			CPopupMenu Menu(m_App.GetResourceInstance(),IDM_BUFFERING);

			Menu.CheckItem(CM_ENABLEBUFFERING,m_App.CoreEngine.GetPacketBuffering());
			Menu.Show(GetMainWindow(),&pt,Flags,pExcludeRect);
		}
		break;

	case MENU_STREAMERROR:
		{
			CPopupMenu Menu(m_App.GetResourceInstance(),IDM_ERROR);

			Menu.Show(GetMainWindow(),&pt,Flags,pExcludeRect);
		}
		break;

	case MENU_CLOCK:
		{
			CPopupMenu Menu(m_App.GetResourceInstance(),IDM_TIME);

			Menu.CheckItem(CM_SHOWTOTTIME,m_App.StatusOptions.GetShowTOTTime());
			Menu.Show(GetMainWindow(),&pt,Flags,pExcludeRect);
		}
		break;

	case MENU_PROGRAMINFO:
		{
			CPopupMenu Menu(m_App.GetResourceInstance(),IDM_PROGRAMINFOSTATUS);

			Menu.CheckItem(CM_PROGRAMINFOSTATUS_POPUPINFO,
						   m_App.StatusOptions.IsPopupProgramInfoEnabled());
			Menu.CheckItem(CM_PROGRAMINFOSTATUS_SHOWPROGRESS,
						   m_App.StatusOptions.GetShowEventProgress());
			Menu.Show(GetMainWindow(),&pt,Flags,pExcludeRect);
		}
		break;

	default:
		return false;
	}

	return true;
}


void CUICore::InitChannelMenu(HMENU hmenu)
{
	const CChannelList *pList=m_App.ChannelManager.GetCurrentChannelList();

	m_App.ChannelMenu.Destroy();
	ClearMenu(hmenu);
	if (pList==nullptr)
		return;

	if (!m_App.CoreEngine.IsNetworkDriver()) {
		CreateChannelMenu(pList,m_App.ChannelManager.GetCurrentChannel(),
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

	if (CurDriver>=0)
		Menu.CheckRadioItem(CM_DRIVER_FIRST,CM_DRIVER_FIRST+i-1,
							CM_DRIVER_FIRST+CurDriver);

	Menu.Append(CM_DRIVER_BROWSE,TEXT("参照..."));
	Menu.AppendSeparator();
	m_App.CommandList.GetCommandNameByID(CM_CLOSETUNER,szText,lengthof(szText));
	Menu.Append(CM_CLOSETUNER,szText);
	Menu.EnableItem(CM_CLOSETUNER,m_App.CoreEngine.IsTunerOpen());

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


bool CUICore::HandleInitMenuPopup(HMENU hmenu)
{
	if (InitChannelMenuPopup(m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_SPACE),hmenu))
		return true;

	if (m_TunerSelectMenu.OnInitMenuPopup(hmenu))
		return true;

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


bool CUICore::SetCommandEnabledState(int Command,bool fEnabled)
{
	return m_App.CommandList.SetCommandStateByID(Command,
		CCommandList::COMMAND_STATE_DISABLED,
		fEnabled ? 0 : CCommandList::COMMAND_STATE_DISABLED);
}


bool CUICore::GetCommandEnabledState(int Command) const
{
	return (m_App.CommandList.GetCommandStateByID(Command) & CCommandList::COMMAND_STATE_DISABLED)==0;
}


bool CUICore::SetCommandCheckedState(int Command,bool fChecked)
{
	return m_App.CommandList.SetCommandStateByID(Command,
		CCommandList::COMMAND_STATE_CHECKED,
		fChecked ? CCommandList::COMMAND_STATE_CHECKED : 0);
}


bool CUICore::GetCommandCheckedState(int Command) const
{
	return (m_App.CommandList.GetCommandStateByID(Command) & CCommandList::COMMAND_STATE_CHECKED)!=0;
}


bool CUICore::SetCommandRadioCheckedState(int FirstCommand,int LastCommand,int CheckedCommand)
{
	return m_App.CommandList.SetCommandRadioCheckedState(FirstCommand,LastCommand,CheckedCommand);
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
		::SendMessage(hwnd,WM_SETICON,ICON_BIG,
					  reinterpret_cast<LPARAM>(hicoBig!=nullptr?hicoBig:m_App.GetAppIcon()));
		::SendMessage(hwnd,WM_SETICON,ICON_SMALL,
					  reinterpret_cast<LPARAM>(hicoSmall!=nullptr?hicoSmall:m_App.GetAppIconSmall()));
	}

	m_LogoIconBig.Attach(hicoBig);
	m_LogoIconSmall.Attach(hicoSmall);

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
		m_App.TaskTrayManager.SetTipText(pszText);
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


COLORREF CUICore::GetColor(LPCTSTR pszText) const
{
	return m_App.ColorSchemeOptions.GetColor(pszText);
}


const CColorScheme *CUICore::GetCurrentColorScheme() const
{
	if (m_pColorScheme==NULL)
		return m_App.ColorSchemeOptions.GetColorScheme();
	return m_pColorScheme;
}


// 配色を適用する
bool CUICore::ApplyColorScheme(const CColorScheme *pColorScheme)
{
	TVTest::Theme::CThemeManager ThemeManager(pColorScheme);

	m_pColorScheme=pColorScheme;

	m_App.MainWindow.SetTheme(&ThemeManager);
	m_App.StatusView.SetTheme(&ThemeManager);
	m_App.SideBar.SetTheme(&ThemeManager);
	m_App.Panel.SetTheme(&ThemeManager);
	m_App.CaptureWindow.SetTheme(&ThemeManager);
	m_App.Epg.ProgramGuide.SetTheme(&ThemeManager);
	m_App.Epg.ProgramGuideFrame.SetTheme(&ThemeManager);
	m_App.Epg.ProgramGuideDisplay.SetTheme(&ThemeManager);

	m_App.AppEventManager.OnColorSchemeChanged();

	return true;
}


HCURSOR CUICore::GetActionCursor() const
{
	return ::LoadCursor(NULL,IDC_HAND);
}


HCURSOR CUICore::GetLinkCursor() const
{
	return ::LoadCursor(NULL,IDC_HAND);
}


bool CUICore::ShowHelpContent(int ID)
{
	return m_App.HtmlHelpClass.ShowContent(ID);
}


void CUICore::SetProgress(int Pos,int Max)
{
	m_App.TaskbarManager.SetProgress(Pos,Max);
}


void CUICore::EndProgress()
{
	m_App.TaskbarManager.EndProgress();
}


void CUICore::SetStatusBarTrace(bool fStatusBarTrace)
{
	if (m_fStatusBarTrace!=fStatusBarTrace) {
		m_fStatusBarTrace=fStatusBarTrace;
		if (!m_fStatusBarTrace)
			m_App.StatusView.SetSingleText(NULL);
	}
}


bool CUICore::CreateChannelMenu(
	const CChannelList *pChannelList,int CurChannel,
	UINT Command,HMENU hmenu,HWND hwnd,unsigned int Flags)
{
	if (pChannelList==NULL)
		return false;
	const bool fEventInfo=
		(Flags & CChannelMenu::FLAG_SHOWEVENTINFO)!=0
		|| pChannelList->NumEnableChannels()<=m_App.MenuOptions.GetMaxChannelMenuEventInfo();
	unsigned int MenuFlags=CChannelMenu::FLAG_SHOWLOGO | Flags;
	if (fEventInfo)
		MenuFlags|=CChannelMenu::FLAG_SHOWEVENTINFO;
	else
		MenuFlags|=CChannelMenu::FLAG_SHOWTOOLTIP;
	return m_App.ChannelMenu.Create(pChannelList,CurChannel,
									Command,hmenu,hwnd,MenuFlags,
									fEventInfo?0:m_App.MenuOptions.GetMaxChannelMenuRows());
}


bool CUICore::InitChannelMenuPopup(HMENU hmenuParent,HMENU hmenu)
{
	bool fChannelMenu=false;
	int Count=::GetMenuItemCount(hmenuParent);
	int i;
	for (i=0;i<Count;i++) {
		if (::GetSubMenu(hmenuParent,i)==hmenu) {
			fChannelMenu=true;
			break;
		}
		if ((::GetMenuState(hmenuParent,i,MF_BYPOSITION) & MF_POPUP)==0)
			break;
	}

	if (!fChannelMenu)
		return false;

	const CChannelManager &ChannelManager=m_App.ChannelManager;
	const CChannelList *pChannelList;
	int Command=CM_SPACE_CHANNEL_FIRST;

	pChannelList=ChannelManager.GetAllChannelList();
	if (ChannelManager.NumSpaces()>1) {
		if (i==0) {
			CreateChannelMenu(
				pChannelList,
				ChannelManager.GetCurrentSpace()==CChannelManager::SPACE_ALL?
					ChannelManager.GetCurrentChannel():-1,
				Command,hmenu,GetMainWindow(),
				CChannelMenu::FLAG_SPACEBREAK);
			return true;
		}
		i--;
	}
	if (i>=ChannelManager.NumSpaces()) {
		TRACE(TEXT("CUICore::InitChannelMenuPopup() : Invalid space %d\n"),i);
		ClearMenu(hmenu);
		return true;
	}
	Command+=pChannelList->NumChannels();
	for (int j=0;j<i;j++) {
		pChannelList=ChannelManager.GetChannelList(j);
		Command+=pChannelList->NumChannels();
	}
	CreateChannelMenu(
		ChannelManager.GetChannelList(i),
		ChannelManager.GetCurrentSpace()==i?
			ChannelManager.GetCurrentChannel():-1,
		Command,hmenu,GetMainWindow());

	return true;
}


void CUICore::OnTrace(CTracer::TraceType Type,LPCTSTR pszOutput)
{
	CLogItem::LogType LogType;

	switch (Type) {
	case CTracer::TYPE_INFORMATION:	LogType=CLogItem::TYPE_INFORMATION;	break;
	case CTracer::TYPE_WARNING:		LogType=CLogItem::TYPE_WARNING;		break;
	case CTracer::TYPE_ERROR:		LogType=CLogItem::TYPE_ERROR;		break;
	default:
		return;
	}

	if (m_fStatusBarTrace && Type==CTracer::TYPE_INFORMATION)
		m_App.StatusView.SetSingleText(pszOutput);
	else
		m_App.Logger.AddLogRaw(LogType,pszOutput);
}




CUICore::CTunerSelectMenu::CTunerSelectMenu(CUICore &UICore)
	: m_UICore(UICore)
{
}


CUICore::CTunerSelectMenu::~CTunerSelectMenu()
{
	Destroy();
}


bool CUICore::CTunerSelectMenu::Create(HWND hwnd)
{
	Destroy();

	m_Menu.Create();
	m_hwnd=hwnd;

	const CChannelManager &ChannelManager=m_UICore.m_App.ChannelManager;
	HMENU hmenuSpace;
	const CChannelList *pChannelList;
	int Command;
	int i,j;
	LPCTSTR pszName;
	TCHAR szText[MAX_PATH*2];
	int Length;

	Command=CM_SPACE_CHANNEL_FIRST;
	pChannelList=ChannelManager.GetAllChannelList();
	if (ChannelManager.NumSpaces()>1) {
		hmenuSpace=::CreatePopupMenu();
		m_Menu.Append(hmenuSpace,TEXT("&A: すべて"));
	}
	Command+=pChannelList->NumChannels();
	for (i=0;i<ChannelManager.NumSpaces();i++) {
		pChannelList=ChannelManager.GetChannelList(i);
		hmenuSpace=::CreatePopupMenu();
		Length=StdUtil::snprintf(szText,lengthof(szText),TEXT("&%d: "),i);
		pszName=ChannelManager.GetTuningSpaceName(i);
		if (!IsStringEmpty(pszName))
			CopyToMenuText(pszName,szText+Length,lengthof(szText)-Length);
		else
			StdUtil::snprintf(szText+Length,lengthof(szText)-Length,TEXT("チューニング空間%d"),i);
		m_Menu.Append(hmenuSpace,szText,
					  pChannelList->NumEnableChannels()>0?MF_ENABLED:MF_GRAYED);
		Command+=pChannelList->NumChannels();
	}

	if (Command>CM_SPACE_CHANNEL_FIRST)
		m_Menu.AppendSeparator();

	CDriverManager &DriverManager=m_UICore.m_App.DriverManager;

	for (i=0;i<DriverManager.NumDrivers();i++) {
		CDriverInfo *pDriverInfo=DriverManager.GetDriverInfo(i);

		if (IsEqualFileName(pDriverInfo->GetFileName(),
							m_UICore.m_App.CoreEngine.GetDriverFileName())) {
			continue;
		}
		TCHAR szFileName[MAX_PATH];
		::lstrcpyn(szFileName,pDriverInfo->GetFileName(),lengthof(szFileName));
		::PathRemoveExtension(szFileName);

		const CTuningSpaceList *pTuningSpaceList;
		if (pDriverInfo->LoadTuningSpaceList(CDriverInfo::LOADTUNINGSPACE_NOLOADDRIVER)
				&& (pTuningSpaceList=pDriverInfo->GetAvailableTuningSpaceList())!=NULL) {
			HMENU hmenuDriver=::CreatePopupMenu();

			for (j=0;j<pTuningSpaceList->NumSpaces();j++) {
				pChannelList=pTuningSpaceList->GetChannelList(j);
				if (pChannelList->NumEnableChannels()==0) {
					Command+=pChannelList->NumChannels();
					continue;
				}
				if (pTuningSpaceList->NumSpaces()>1)
					hmenuSpace=::CreatePopupMenu();
				else
					hmenuSpace=hmenuDriver;
				m_PopupList.push_back(PopupInfo(pChannelList,Command));
				MENUINFO mi;
				mi.cbSize=sizeof(mi);
				mi.fMask=MIM_MENUDATA;
				mi.dwMenuData=m_PopupList.size()-1;
				::SetMenuInfo(hmenuSpace,&mi);
				Command+=pChannelList->NumChannels();
				if (hmenuSpace!=hmenuDriver) {
					pszName=pTuningSpaceList->GetTuningSpaceName(j);
					Length=StdUtil::snprintf(szText,lengthof(szText),TEXT("&%d: "),j);
					if (!IsStringEmpty(pszName))
						CopyToMenuText(pszName,szText+Length,lengthof(szText)-Length);
					else
						StdUtil::snprintf(szText+Length,lengthof(szText)-Length,
										  TEXT("チューニング空間%d"),j);
					::AppendMenu(hmenuDriver,MF_POPUP | MF_ENABLED,
								 reinterpret_cast<UINT_PTR>(hmenuSpace),szText);
				}
			}
			if (!IsStringEmpty(pDriverInfo->GetTunerName())) {
				TCHAR szTemp[lengthof(szText)];

				StdUtil::snprintf(szTemp,lengthof(szTemp),TEXT("%s [%s]"),
								  pDriverInfo->GetTunerName(),
								  szFileName);
				CopyToMenuText(szTemp,szText,lengthof(szText));
			} else {
				CopyToMenuText(szFileName,szText,lengthof(szText));
			}
			m_Menu.Append(hmenuDriver,szText);
		} else {
			m_Menu.AppendUnformatted(CM_DRIVER_FIRST+i,szFileName);
		}
	}

	return true;
}


void CUICore::CTunerSelectMenu::Destroy()
{
	m_Menu.Destroy();
	m_hwnd=NULL;
	m_PopupList.clear();
}


int CUICore::CTunerSelectMenu::Show(UINT Flags,int x,int y,const RECT *pExcludeRect)
{
	POINT pt={x,y};
	return m_Menu.Show(m_hwnd,&pt,Flags,pExcludeRect);
}


bool CUICore::CTunerSelectMenu::OnInitMenuPopup(HMENU hmenu)
{
	if (!m_Menu.IsCreated())
		return false;

	if (m_UICore.InitChannelMenuPopup(m_Menu.GetPopupHandle(),hmenu))
		return true;

	bool fChannelMenu=false;
	int Count=m_Menu.GetItemCount();
	int i,j;
	i=m_UICore.m_App.ChannelManager.NumSpaces();
	if (i>1)
		i++;
	for (i++;i<Count;i++) {
		HMENU hmenuChannel=m_Menu.GetSubMenu(i);
		int Items=::GetMenuItemCount(hmenuChannel);

		if (hmenuChannel==hmenu) {
			if (Items>0)
				return true;
			fChannelMenu=true;
			break;
		}
		if (Items>0) {
			for (j=0;j<Items;j++) {
				if (::GetSubMenu(hmenuChannel,j)==hmenu)
					break;
			}
			if (j<Items) {
				fChannelMenu=true;
				break;
			}
		}
	}

	if (fChannelMenu) {
		MENUINFO mi;

		mi.cbSize=sizeof(mi);
		mi.fMask=MIM_MENUDATA;
		if (!::GetMenuInfo(hmenu,&mi) || mi.dwMenuData>=m_PopupList.size())
			return false;
		const PopupInfo &Info=m_PopupList[mi.dwMenuData];
		m_UICore.CreateChannelMenu(Info.pChannelList,-1,Info.Command,hmenu,m_hwnd);
		return true;
	}

	return false;
}
