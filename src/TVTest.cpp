#include "stdafx.h"
#include <algorithm>
#include "DtvEngine.h"
#include "TVTest.h"
#include "AppMain.h"
#include "AppUtil.h"
#include "MainWindow.h"
#include "Menu.h"
#include "ResidentManager.h"
#include "InformationPanel.h"
#include "ProgramListPanel.h"
#include "ChannelPanel.h"
#include "CaptionPanel.h"
#include "ControlPanel.h"
#include "ControlPanelItems.h"
#include "Accelerator.h"
#include "Controller.h"
#include "OptionDialog.h"
#include "GeneralOptions.h"
#include "ViewOptions.h"
#include "OSDOptions.h"
#include "StatusOptions.h"
#include "SideBarOptions.h"
#include "MenuOptions.h"
#include "PanelOptions.h"
#include "ColorScheme.h"
#include "OperationOptions.h"
#include "DriverOptions.h"
#include "PlaybackOptions.h"
#include "RecordOptions.h"
#include "CaptureOptions.h"
#include "ChannelScan.h"
#include "EpgOptions.h"
#include "ProgramGuideOptions.h"
#include "Plugin.h"
#ifdef NETWORK_REMOCON_SUPPORT
#include "NetworkRemocon.h"
#endif
#include "Logger.h"
#include "CommandLine.h"
#include "InitialSettings.h"
#include "ChannelHistory.h"
#include "Favorites.h"
#include "Help.h"
#include "StreamInfo.h"
#include "MiscDialog.h"
#include "DialogUtil.h"
#include "DrawUtil.h"
#include "WindowUtil.h"
#include "PseudoOSD.h"
#include "HomeDisplay.h"
#include "ChannelDisplay.h"
#include "Taskbar.h"
#include "EventInfoPopup.h"
#include "CardReaderDialog.h"
#include "ZoomOptions.h"
#include "PanAndScanOptions.h"
#include "LogoManager.h"
#include "ToolTip.h"
#include "HelperClass/StdUtil.h"
#include "resource.h"

#pragma comment(lib,"imm32.lib")	// for ImmAssociateContext(Ex)

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define MAIN_TITLE_TEXT APP_NAME


using namespace TVTest;


static HINSTANCE hInst;
static CAppMain AppMain;
static CCoreEngine CoreEngine;
static CMainMenu MainMenu;
static CCommandList CommandList;
static CPluginManager PluginManager;
static CEpgProgramList EpgProgramList(&CoreEngine.m_DtvEngine.m_EventManager);
static CMainWindow MainWindow;
static CStatusView StatusView;
static CSideBar SideBar(&CommandList);
static CNotificationBar NotificationBar;
static CHtmlHelp HtmlHelpClass;
static CIconMenu AspectRatioIconMenu;
static CTaskbarManager TaskbarManager;
static CEventSearchOptions EventSearchOptions;
static CHomeDisplay HomeDisplay(EventSearchOptions);
static CChannelDisplay ChannelDisplay(&EpgProgramList);
static CBalloonTip NotifyBalloonTip;

#ifndef _DEBUG
#include "DebugHelper.h"
static CDebugHelper DebugHelper;
#endif

static bool fIncrementUDPPort=true;

static CCommandLineOptions CmdLineOptions;

static CChannelManager ChannelManager;
#ifdef NETWORK_REMOCON_SUPPORT
static CNetworkRemocon *pNetworkRemocon=NULL;
#endif
static CResidentManager ResidentManager;
static CDriverManager DriverManager;
static CLogoManager LogoManager;

static bool fShowPanelWindow=false;
static CPanelFrame PanelFrame;
static CPanelForm PanelForm;

static CInformationPanel InfoPanel;
static CProgramListPanel ProgramListPanel;
static CChannelPanel ChannelPanel;
static CCaptionPanel CaptionPanel;
static CControlPanel ControlPanel;

static CProgramGuide g_ProgramGuide(EventSearchOptions);
static CProgramGuideFrameSettings ProgramGuideFrameSettings;
static CProgramGuideFrame ProgramGuideFrame(&g_ProgramGuide,&ProgramGuideFrameSettings);
static CProgramGuideDisplay ProgramGuideDisplay(&g_ProgramGuide,&ProgramGuideFrameSettings);
static bool fShowProgramGuide=false;

static CStreamInfo StreamInfo;

static CChannelMenu ChannelMenu(&EpgProgramList,&LogoManager);

static CZoomOptions ZoomOptions;
static CPanAndScanOptions PanAndScanOptions;
static CGeneralOptions GeneralOptions;
static CViewOptions ViewOptions;
static COSDOptions OSDOptions;
static COSDManager OSDManager(&OSDOptions);
static CStatusOptions StatusOptions(&StatusView);
static CSideBarOptions SideBarOptions(&SideBar,&ZoomOptions);
static CMenuOptions MenuOptions;
static CPanelOptions PanelOptions(&PanelFrame);
static CColorSchemeOptions ColorSchemeOptions;
static COperationOptions OperationOptions;
static CAccelerator Accelerator;
static CControllerManager ControllerManager;
static CDriverOptions DriverOptions;
static CPlaybackOptions PlaybackOptions;
static CRecordOptions RecordOptions;
static CRecordManager RecordManager;
static CCaptureOptions CaptureOptions;
static CChannelScan ChannelScan;
static CEpgOptions EpgOptions;
static CProgramGuideOptions ProgramGuideOptions(&g_ProgramGuide,&PluginManager);
static CPluginOptions PluginOptions(&PluginManager);
#ifdef NETWORK_REMOCON_SUPPORT
static CNetworkRemoconOptions NetworkRemoconOptions;
#endif
static CLogger Logger;
static CRecentChannelList RecentChannelList;
static CChannelHistory ChannelHistory;
static CFavoritesManager FavoritesManager;
static CFavoritesMenu FavoritesMenu;

static COptionDialog OptionDialog;

static struct {
	int Space;
	int Channel;
	int ServiceID;
	int TransportStreamID;
	bool fAllChannels;
} RestoreChannelInfo;

static bool fEnablePlay=true;

static CImageCodec ImageCodec;
static CCaptureWindow CaptureWindow;

static const BYTE g_AudioGainList[] = {100, 125, 150, 200};

static const struct {
	CMediaViewer::PropertyFilter Filter;
	int Command;
} g_DirectShowFilterPropertyList[] = {
	{CMediaViewer::PROPERTY_FILTER_VIDEODECODER,		CM_VIDEODECODERPROPERTY},
	{CMediaViewer::PROPERTY_FILTER_VIDEORENDERER,		CM_VIDEORENDERERPROPERTY},
	{CMediaViewer::PROPERTY_FILTER_AUDIOFILTER,			CM_AUDIOFILTERPROPERTY},
	{CMediaViewer::PROPERTY_FILTER_AUDIORENDERER,		CM_AUDIORENDERERPROPERTY},
	{CMediaViewer::PROPERTY_FILTER_MPEG2DEMULTIPLEXER,	CM_DEMULTIPLEXERPROPERTY},
};


#ifdef NETWORK_REMOCON_SUPPORT

class CMyGetChannelReceiver : public CNetworkRemoconReceiver {
public:
	void OnReceive(LPCSTR pszText);
};

void CMyGetChannelReceiver::OnReceive(LPCSTR pszText)
{
	int Channel;
	LPCSTR p;

	Channel=0;
	for (p=pszText;*p!='\0';p++)
		Channel=Channel*10+(*p-'0');
	PostMessage(MainWindow.GetHandle(),WM_APP_CHANNELCHANGE,Channel,0);
}


class CMyGetDriverReceiver : public CNetworkRemoconReceiver {
	HANDLE m_hEvent;
	TCHAR m_szCurDriver[64];
public:
	CMyGetDriverReceiver() { m_hEvent=::CreateEvent(NULL,FALSE,FALSE,NULL); }
	~CMyGetDriverReceiver() { ::CloseHandle(m_hEvent); }
	void OnReceive(LPCSTR pszText);
	void Initialize() { ::ResetEvent(m_hEvent); }
	bool Wait(DWORD TimeOut) { return ::WaitForSingleObject(m_hEvent,TimeOut)==WAIT_OBJECT_0; }
	LPCTSTR GetCurDriver() const { return m_szCurDriver; }
};

void CMyGetDriverReceiver::OnReceive(LPCSTR pszText)
{
	LPCSTR p;
	int Sel,i;

	m_szCurDriver[0]='\0';
	p=pszText;
	while (*p!='\t') {
		if (*p=='\0')
			goto End;
		p++;
	}
	p++;
	Sel=0;
	for (;*p>='0' && *p<='9';p++)
		Sel=Sel*10+(*p-'0');
	if (*p!='\t')
		goto End;
	p++;
	for (i=0;i<=Sel && *p!='\0';i++) {
		while (*p!='\t') {
			if (*p=='\0')
				goto End;
			p++;
		}
		p++;
		if (i==Sel) {
			int j;

			for (j=0;*p!='\t' && *p!='\0';j++) {
				m_szCurDriver[j]=*p++;
			}
			m_szCurDriver[j]='\0';
			break;
		} else {
			while (*p!='\t' && *p!='\0')
				p++;
			if (*p=='\t')
				p++;
		}
	}
End:
	::SetEvent(m_hEvent);
}


static CMyGetChannelReceiver GetChannelReceiver;
static CMyGetDriverReceiver GetDriverReceiver;

#endif	// NETWORK_REMOCON_SUPPORT


class CTotTimeAdjuster {
	bool m_fEnable;
	DWORD m_TimeOut;
	DWORD m_StartTime;
	SYSTEMTIME m_PrevTime;
public:
	CTotTimeAdjuster()
		: m_fEnable(false)
	{
	}
	bool BeginAdjust(DWORD TimeOut=10000UL)
	{
		m_TimeOut=TimeOut;
		m_StartTime=::GetTickCount();
		m_PrevTime.wYear=0;
		m_fEnable=true;
		return true;
	}
	bool AdjustTime()
	{
		if (!m_fEnable)
			return false;
		if (TickTimeSpan(m_StartTime,::GetTickCount())>=m_TimeOut) {
			m_fEnable=false;
			return false;
		}

		SYSTEMTIME st;
		if (!CoreEngine.m_DtvEngine.m_TsAnalyzer.GetTotTime(&st))
			return false;
		if (m_PrevTime.wYear==0) {
			m_PrevTime=st;
			return false;
		} else if (CompareSystemTime(&m_PrevTime,&st)==0) {
			return false;
		}

		bool fOK=false;
		HANDLE hToken;
		if (::OpenProcessToken(::GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&hToken)) {
			LUID luid;
			if (::LookupPrivilegeValue(NULL,SE_SYSTEMTIME_NAME,&luid)) {
				TOKEN_PRIVILEGES tkp;
				tkp.PrivilegeCount=1;
				tkp.Privileges[0].Luid=luid;
				tkp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
				if (::AdjustTokenPrivileges(hToken,FALSE, &tkp,sizeof(TOKEN_PRIVILEGES),NULL,0)
						&& ::GetLastError()==ERROR_SUCCESS) {
					// バッファがあるので少し時刻を戻す
					OffsetSystemTime(&st,-2000);
					if (::SetLocalTime(&st)) {
						Logger.AddLog(TEXT("TOTで時刻合わせを行いました。(%d/%d/%d %d:%02d:%02d)"),
									  st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
						fOK=true;
					}
				}
			}
			::CloseHandle(hToken);
		}
		m_fEnable=false;
		return fOK;
	}
};

static CTotTimeAdjuster TotTimeAdjuster;




CAppMain::CAppMain()
	: m_fSilent(false)
	, m_fFirstExecute(false)
{
}


bool CAppMain::Initialize()
{
	m_UICore.SetSkin(&MainWindow);

	TCHAR szModuleFileName[MAX_PATH];

	::GetModuleFileName(NULL,szModuleFileName,MAX_PATH);
	if (CmdLineOptions.m_IniFileName.IsEmpty()) {
		::lstrcpy(m_szIniFileName,szModuleFileName);
		::PathRenameExtension(m_szIniFileName,TEXT(".ini"));
	} else {
		LPCTSTR pszIniFileName=CmdLineOptions.m_IniFileName.Get();
		if (::PathIsRelative(pszIniFileName)) {
			TCHAR szTemp[MAX_PATH];
			::lstrcpy(szTemp,szModuleFileName);
			::lstrcpy(::PathFindFileName(szTemp),pszIniFileName);
			::PathCanonicalize(m_szIniFileName,szTemp);
		} else {
			::lstrcpy(m_szIniFileName,pszIniFileName);
		}
	}
	::lstrcpy(m_szChannelSettingFileName,szModuleFileName);
	::PathRenameExtension(m_szChannelSettingFileName,TEXT(".ch.ini"));
	::lstrcpy(m_szFavoritesFileName,szModuleFileName);
	::PathRenameExtension(m_szFavoritesFileName,TEXT(".tvfavorites"));

	bool fExists=::PathFileExists(m_szIniFileName)!=FALSE;
	m_fFirstExecute=!fExists && CmdLineOptions.m_IniFileName.IsEmpty();
	if (fExists) {
		AddLog(TEXT("設定を読み込んでいます..."));
		LoadSettings();
	}

	return true;
}


bool CAppMain::Finalize()
{
	AddLog(TEXT("設定を保存しています..."));
	SaveSettings(SETTINGS_SAVE_STATUS);
	return true;
}


HINSTANCE CAppMain::GetResourceInstance() const
{
	return hInst;
}


HINSTANCE CAppMain::GetInstance() const
{
	return hInst;
}


bool CAppMain::GetAppDirectory(LPTSTR pszDirectory) const
{
	if (::GetModuleFileName(NULL,pszDirectory,MAX_PATH)==0)
		return false;
	CFilePath FilePath(pszDirectory);
	FilePath.RemoveFileName();
	FilePath.GetPath(pszDirectory);
	return true;
}


bool CAppMain::GetDriverDirectory(LPTSTR pszDirectory,int MaxLength) const
{
	return CoreEngine.GetDriverDirectory(pszDirectory,MaxLength);
}


void CAppMain::AddLog(LPCTSTR pszText, ...)
{
	va_list Args;

	va_start(Args,pszText);
	Logger.AddLogV(pszText,Args);
	va_end(Args);
}


void CAppMain::OnError(const CBonErrorHandler *pErrorHandler,LPCTSTR pszTitle)
{
	if (pErrorHandler==NULL)
		return;
	Logger.AddLog(TEXT("%s"),pErrorHandler->GetLastErrorText());
	if (!m_fSilent)
		m_UICore.GetSkin()->ShowErrorMessage(pErrorHandler,pszTitle);
}


void CAppMain::SetSilent(bool fSilent)
{
	m_fSilent=fSilent;
#ifndef _DEBUG
	DebugHelper.SetExceptionFilterMode(
		fSilent?CDebugHelper::EXCEPTION_FILTER_NONE:CDebugHelper::EXCEPTION_FILTER_DIALOG);
#endif
}


bool CAppMain::InitializeChannel()
{
	const bool fNetworkDriver=CoreEngine.IsNetworkDriver();
	CFilePath ChannelFilePath;
#ifdef NETWORK_REMOCON_SUPPORT
	TCHAR szNetworkDriverName[MAX_PATH];
#endif

	ChannelManager.Reset();
	ChannelManager.MakeDriverTuningSpaceList(&CoreEngine.m_DtvEngine.m_BonSrcDecoder);

	if (!fNetworkDriver) {
		TCHAR szPath[MAX_PATH];
		GetChannelFileName(CoreEngine.GetDriverFileName(),szPath,MAX_PATH);
		ChannelFilePath.SetPath(szPath);
	}
#ifdef NETWORK_REMOCON_SUPPORT
	else {
		bool fOK=false;

		if (NetworkRemoconOptions.IsEnable()) {
			if (NetworkRemoconOptions.CreateNetworkRemocon(&pNetworkRemocon)) {
				GetDriverReceiver.Initialize();
				if (pNetworkRemocon->GetDriverList(&GetDriverReceiver)
						&& GetDriverReceiver.Wait(2000)
						&& GetDriverReceiver.GetCurDriver()[0]!=_T('\0')) {
					TCHAR szFileName[MAX_PATH];

					if (NetworkRemoconOptions.FindChannelFile(
								GetDriverReceiver.GetCurDriver(),szFileName)) {
						LPTSTR p;

						NetworkRemoconOptions.SetDefaultChannelFileName(szFileName);
						p=szFileName;
						while (*p!=_T('('))
							p++;
						*p=_T('\0');
						::wsprintf(szNetworkDriverName,TEXT("%s.dll"),szFileName);
						::lstrcpy(p,CHANNEL_FILE_EXTENSION);
						ChannelFilePath.SetPath(szFileName);
						GetAppDirectory(szFileName);
						ChannelFilePath.SetDirectory(szFileName);
						fOK=ChannelFilePath.IsExists();
#ifdef DEFERRED_CHANNEL_FILE_EXTENSION
						if (!fOK) {
							ChannelFilePath.SetExtension(DEFERRED_CHANNEL_FILE_EXTENSION);
							fOK=ChannelFilePath.IsExists();
						}
#endif
					}
				}
			}
			if (!fOK && !IsStringEmpty(NetworkRemoconOptions.GetChannelFileName())) {
				TCHAR szFileName[MAX_PATH],*q;
				LPCTSTR p;

				GetAppDirectory(szFileName);
				ChannelFilePath.SetPath(szFileName);
				p=NetworkRemoconOptions.GetChannelFileName();
				q=szFileName;
				while (*p!=_T('(') && *p!=_T('\0')) {
					int Length=StringCharLength(p);
					if (Length==0)
						break;
					for (int i=0;i<Length;i++)
						*q++=*p++;
				}
				*q=_T('\0');
				::wsprintf(szNetworkDriverName,TEXT("%s.dll"),szFileName);
				::lstrcpy(q,CHANNEL_FILE_EXTENSION);
				ChannelFilePath.Append(szFileName);
				fOK=ChannelFilePath.IsExists();
#ifdef DEFERRED_CHANNEL_FILE_EXTENSION
				if (!fOK) {
					ChannelFilePath.SetExtension(DEFERRED_CHANNEL_FILE_EXTENSION);
					fOK=ChannelFilePath.IsExists();
				}
#endif
			}
		}
		if (!fOK)
			szNetworkDriverName[0]=_T('\0');
	}
#endif	// NETWORK_REMOCON_SUPPORT

	if (!ChannelFilePath.IsEmpty()) {
		if (ChannelManager.LoadChannelList(ChannelFilePath.GetPath())) {
			Logger.AddLog(TEXT("チャンネル設定を \"%s\" から読み込みました。"),
						  ChannelFilePath.GetPath());
			if (!ChannelManager.ChannelFileHasStreamIDs())
				Logger.AddLog(TEXT("(チャンネルファイルが古いので再スキャンをお薦めします)"));
		}
	}

	TCHAR szFileName[MAX_PATH];
	bool fLoadChannelSettings=true;
	if (!fNetworkDriver) {
		::lstrcpy(szFileName,CoreEngine.GetDriverFileName());
	} else {
#ifdef NETWORK_REMOCON_SUPPORT
		if (szNetworkDriverName[0]!=_T('\0')) {
			::lstrcpy(szFileName,szNetworkDriverName);
		} else
#endif
		{
			fLoadChannelSettings=false;
		}
	}
	if (fLoadChannelSettings)
		ChannelManager.LoadChannelSettings(m_szChannelSettingFileName,szFileName);

	CDriverOptions::ChannelInfo InitChInfo;
	if (DriverOptions.GetInitialChannel(CoreEngine.GetDriverFileName(),&InitChInfo)) {
		RestoreChannelInfo.Space=InitChInfo.Space;
		RestoreChannelInfo.Channel=InitChInfo.Channel;
		RestoreChannelInfo.ServiceID=InitChInfo.ServiceID;
		RestoreChannelInfo.TransportStreamID=InitChInfo.TransportStreamID;
		RestoreChannelInfo.fAllChannels=InitChInfo.fAllChannels;
	} else {
		RestoreChannelInfo.Space=-1;
		RestoreChannelInfo.Channel=-1;
		RestoreChannelInfo.ServiceID=-1;
		RestoreChannelInfo.TransportStreamID=-1;
		RestoreChannelInfo.fAllChannels=false;
	}

	ChannelManager.SetUseDriverChannelList(fNetworkDriver);
	ChannelManager.SetCurrentChannel(
		RestoreChannelInfo.fAllChannels?CChannelManager::SPACE_ALL:max(RestoreChannelInfo.Space,0),
		-1);
	ChannelManager.SetCurrentServiceID(0);
#ifdef NETWORK_REMOCON_SUPPORT
	NetworkRemoconOptions.InitNetworkRemocon(&pNetworkRemocon,
											 &CoreEngine,&ChannelManager);
#endif
	m_UICore.OnChannelListChanged();
	ChannelScan.SetTuningSpaceList(ChannelManager.GetTuningSpaceList());
	return true;
}


bool CAppMain::GetChannelFileName(LPCTSTR pszDriverFileName,
								  LPTSTR pszChannelFileName,int MaxChannelFileName)
{
	if (IsStringEmpty(pszDriverFileName) || pszChannelFileName==NULL)
		return false;

	const bool fRelative=::PathIsRelative(pszDriverFileName)!=FALSE;
	TCHAR szPath[MAX_PATH],szPath2[MAX_PATH],szDir[MAX_PATH];
	if (fRelative) {
		if (!CoreEngine.GetDriverDirectory(szDir,lengthof(szDir)))
			return false;
		if (::PathCombine(szPath,szDir,pszDriverFileName)==NULL)
			return false;
	} else {
		if (::lstrlen(pszDriverFileName)>=lengthof(szPath))
			return false;
		::lstrcpy(szPath,pszDriverFileName);
	}
	::PathRenameExtension(szPath,CHANNEL_FILE_EXTENSION);
#ifdef DEFERRED_CHANNEL_FILE_EXTENSION
	if (!::PathFileExists(szPath)) {
		::lstrcpy(szPath2,szPath);
		::PathRenameExtension(szPath2,DEFERRED_CHANNEL_FILE_EXTENSION);
		if (::PathFileExists(szPath2))
			::lstrcpy(szPath,szPath2);
	}
#endif
	if (fRelative && !::PathFileExists(szPath)) {
		GetAppDirectory(szDir);
		if (::PathCombine(szPath2,szDir,pszDriverFileName)!=NULL) {
			::PathRenameExtension(szPath2,CHANNEL_FILE_EXTENSION);
			if (::PathFileExists(szPath2)) {
				::lstrcpy(szPath,szPath2);
			}
#ifdef DEFERRED_CHANNEL_FILE_EXTENSION
			else {
				::PathRenameExtension(szPath2,DEFERRED_CHANNEL_FILE_EXTENSION);
				if (::PathFileExists(szPath2))
					::lstrcpy(szPath,szPath2);
			}
#endif
		}
	}
	if (::lstrlen(szPath)>=MaxChannelFileName)
		return false;
	::lstrcpy(pszChannelFileName,szPath);
	return true;
}


bool CAppMain::RestoreChannel()
{
	if (RestoreChannelInfo.Space>=0 && RestoreChannelInfo.Channel>=0) {
		int Space=RestoreChannelInfo.fAllChannels?CChannelManager::SPACE_ALL:RestoreChannelInfo.Space;
		const CChannelList *pList=ChannelManager.GetChannelList(Space);
		if (pList!=NULL) {
			int Index=pList->Find(RestoreChannelInfo.Space,
								  RestoreChannelInfo.Channel,
								  RestoreChannelInfo.ServiceID);
			if (Index<0) {
				if (RestoreChannelInfo.TransportStreamID>0 && RestoreChannelInfo.ServiceID>0) {
					Index=pList->FindByIDs(0,
										   (WORD)RestoreChannelInfo.TransportStreamID,
										   (WORD)RestoreChannelInfo.ServiceID);
				}
				if (Index<0 && RestoreChannelInfo.ServiceID>0) {
					Index=pList->Find(RestoreChannelInfo.Space,
									  RestoreChannelInfo.Channel);
				}
			}
			if (Index>=0)
				return AppMain.SetChannel(Space,Index);
		}
	}
	return false;
}


bool CAppMain::UpdateCurrentChannelList(const CTuningSpaceList *pList)
{
	bool fNetworkDriver=CoreEngine.IsNetworkDriver();

	ChannelManager.SetTuningSpaceList(pList);
	ChannelManager.SetUseDriverChannelList(fNetworkDriver);
	/*
	ChannelManager.SetCurrentChannel(
		(!fNetworkDriver && ChannelManager.GetAllChannelList()->NumChannels()>0)?
												CChannelManager::SPACE_ALL:0,
		CoreEngine.IsUDPDriver()?0:-1);
	*/
	int Space=-1;
	bool fAllChannels=false;
	for (int i=0;i<pList->NumSpaces();i++) {
		if (pList->GetTuningSpaceType(i)!=CTuningSpaceInfo::SPACE_TERRESTRIAL) {
			fAllChannels=false;
			break;
		}
		if (pList->GetChannelList(i)->NumChannels()>0) {
			if (Space>=0)
				fAllChannels=true;
			else
				Space=i;
		}
	}
	ChannelManager.SetCurrentChannel(
		fAllChannels?CChannelManager::SPACE_ALL:(Space>=0?Space:0),
		-1);
	ChannelManager.SetCurrentServiceID(0);
	WORD ServiceID;
	if (CoreEngine.m_DtvEngine.GetServiceID(&ServiceID))
		FollowChannelChange(CoreEngine.m_DtvEngine.m_TsAnalyzer.GetTransportStreamID(),ServiceID);
#ifdef NETWORK_REMOCON_SUPPORT
	NetworkRemoconOptions.InitNetworkRemocon(&pNetworkRemocon,
											 &CoreEngine,&ChannelManager);
#endif

	m_UICore.OnChannelListChanged();

	UpdateChannelList(CoreEngine.GetDriverFileName(),pList);

	return true;
}


bool CAppMain::UpdateChannelList(LPCTSTR pszBonDriverName,const CTuningSpaceList *pList)
{
	if (IsStringEmpty(pszBonDriverName) || pList==NULL)
		return false;

	int Index=DriverManager.FindByFileName(::PathFindFileName(pszBonDriverName));
	if (Index>=0) {
		CDriverInfo *pDriverInfo=DriverManager.GetDriverInfo(Index);
		if (pDriverInfo!=NULL) {
			pDriverInfo->ClearTuningSpaceList();
		}
	}

	// お気に入りチャンネルの更新
	class CFavoritesChannelUpdator : public CFavoriteItemEnumerator
	{
		LPCTSTR m_pszBonDriver;
		const CTuningSpaceList *m_pTuningSpaceList;
		bool m_fUpdated;

		bool ChannelItem(CFavoriteFolder &Folder,CFavoriteChannel &Channel) override
		{
			if (IsEqualFileName(Channel.GetBonDriverFileName(),m_pszBonDriver)) {
				const CChannelInfo &ChannelInfo=Channel.GetChannelInfo();
				const CChannelList *pChannelList=m_pTuningSpaceList->GetChannelList(ChannelInfo.GetSpace());

				if (pChannelList!=NULL) {
					if (pChannelList->FindByIDs(
							ChannelInfo.GetNetworkID(),
							ChannelInfo.GetTransportStreamID(),
							ChannelInfo.GetServiceID())<0) {
						const int ChannelCount=pChannelList->NumChannels();
						const NetworkType ChannelNetworkType=GetNetworkType(ChannelInfo.GetNetworkID());

						for (int i=0;i<ChannelCount;i++) {
							const CChannelInfo *pChInfo=pChannelList->GetChannelInfo(i);

							if (GetNetworkType(pChInfo->GetNetworkID())==ChannelNetworkType
									&& (pChInfo->GetServiceID()==ChannelInfo.GetServiceID()
										|| ::lstrcmp(pChInfo->GetName(),ChannelInfo.GetName())==0)) {
								TRACE(TEXT("お気に入りチャンネル更新 : %s -> %s / NID %d -> %d / TSID %04x -> %04x / SID %d -> %d\n"),
									  ChannelInfo.GetName(),pChInfo->GetName(),
									  ChannelInfo.GetNetworkID(),pChInfo->GetNetworkID(),
									  ChannelInfo.GetTransportStreamID(),pChInfo->GetTransportStreamID(),
									  ChannelInfo.GetServiceID(),pChInfo->GetServiceID());
								Channel.SetChannelInfo(*pChInfo);
								m_fUpdated=true;
								break;
							}
						}
					}
				}
			}

			return true;
		}

	public:
		CFavoritesChannelUpdator(LPCTSTR pszBonDriver,const CTuningSpaceList *pTuningSpaceList)
			: m_pszBonDriver(pszBonDriver)
			, m_pTuningSpaceList(pTuningSpaceList)
			, m_fUpdated(false)
		{
		}

		bool IsUpdated() const { return m_fUpdated; }
	};

	CFavoritesChannelUpdator FavoritesUpdator(pszBonDriverName,pList);
	FavoritesUpdator.EnumItems(FavoritesManager.GetRootFolder());
	if (FavoritesUpdator.IsUpdated())
		FavoritesManager.SetModified(true);

	return true;
}


bool CAppMain::SaveChannelSettings()
{
	if (!CoreEngine.IsTunerOpen() || CoreEngine.IsNetworkDriver())
		return true;
	return ChannelManager.SaveChannelSettings(m_szChannelSettingFileName,
											  CoreEngine.GetDriverFileName());
}


const CChannelInfo *CAppMain::GetCurrentChannelInfo() const
{
	return ChannelManager.GetCurrentChannelInfo();
}


bool CAppMain::SetChannel(int Space,int Channel,int ServiceID/*=-1*/)
{
	const CChannelInfo *pPrevChInfo=ChannelManager.GetCurrentRealChannelInfo();
	int OldSpace=ChannelManager.GetCurrentSpace(),OldChannel=ChannelManager.GetCurrentChannel();

	if (!ChannelManager.SetCurrentChannel(Space,Channel))
		return false;
	const CChannelInfo *pChInfo=ChannelManager.GetCurrentRealChannelInfo();
	if (pChInfo==NULL) {
		ChannelManager.SetCurrentChannel(OldSpace,OldChannel);
		return false;
	}
	if (pPrevChInfo==NULL
			|| pChInfo->GetSpace()!=pPrevChInfo->GetSpace()
			|| pChInfo->GetChannelIndex()!=pPrevChInfo->GetChannelIndex()) {
		if (ServiceID<=0 && pChInfo->GetServiceID()>0)
			ServiceID=pChInfo->GetServiceID();

		LPCTSTR pszTuningSpace=ChannelManager.GetDriverTuningSpaceList()->GetTuningSpaceName(pChInfo->GetSpace());
		AddLog(TEXT("BonDriverにチャンネル変更を要求します。(チューニング空間 %d[%s] / Ch %d[%s] / Sv %d)"),
			   pChInfo->GetSpace(),pszTuningSpace!=NULL?pszTuningSpace:TEXT("\?\?\?"),
			   pChInfo->GetChannelIndex(),pChInfo->GetName(),ServiceID);

		if (!CoreEngine.m_DtvEngine.SetChannel(
				pChInfo->GetSpace(),pChInfo->GetChannelIndex(),
				ServiceID>0?ServiceID:CDtvEngine::SID_INVALID,
				!IsCSNetworkID(pChInfo->GetNetworkID()))) {
			AddLog(TEXT("%s"),CoreEngine.m_DtvEngine.GetLastErrorText());
			ChannelManager.SetCurrentChannel(OldSpace,OldChannel);
			return false;
		}
#ifdef TVH264_FOR_1SEG
		// 予めTSIDを設定して表示を早くする(有意な差があるかは謎…)
		if ((pChInfo->GetTransportStreamID() & 0xFF00) == 0x7F00
				|| (pChInfo->GetTransportStreamID() & 0xFF00) == 0x7E00) {
			CoreEngine.m_DtvEngine.m_TsPacketParser.SetTransportStreamID(pChInfo->GetTransportStreamID());
		}
#endif
		ChannelManager.SetCurrentServiceID(ServiceID);
		PluginManager.SendChannelChangeEvent();
	} else {
		if (ServiceID>0) {
			SetServiceByID(ServiceID);
		} else if (pChInfo->GetServiceID()>0) {
			SetServiceByID(pChInfo->GetServiceID());
		}
	}

	m_UICore.OnChannelChanged(Space!=OldSpace ? CUICore::CHANNEL_CHANGED_STATUS_SPACE_CHANGED : 0);

	return true;
}


bool CAppMain::SetChannelByIndex(int Space,int Channel,int ServiceID)
{
	const CChannelList *pChannelList=ChannelManager.GetChannelList(Space);
	if (pChannelList==NULL)
		return false;

	int ListChannel=pChannelList->Find(Space,Channel,ServiceID);
	if (ListChannel<0) {
		if (ServiceID>0)
			ListChannel=pChannelList->Find(Space,Channel);
		if (ListChannel<0)
			return false;
	}

	return SetChannel(Space,ListChannel,ServiceID);
}


bool CAppMain::SelectChannel(const ChannelSelectInfo &SelInfo)
{
	if (SelInfo.fUseCurTuner
			&& CoreEngine.IsTunerOpen()) {
		int Space=ChannelManager.GetCurrentSpace();
		if (Space!=CChannelManager::SPACE_INVALID) {
			int Index=ChannelManager.FindChannelByIDs(Space,
				SelInfo.Channel.GetNetworkID(),
				SelInfo.Channel.GetTransportStreamID(),
				SelInfo.Channel.GetServiceID());
			if (Index<0 && Space!=CChannelManager::SPACE_ALL) {
				for (Space=0;Space<ChannelManager.NumSpaces();Space++) {
					Index=ChannelManager.FindChannelByIDs(Space,
						SelInfo.Channel.GetNetworkID(),
						SelInfo.Channel.GetTransportStreamID(),
						SelInfo.Channel.GetServiceID());
					if (Index>=0)
						break;
				}
			}
			if (Index>=0) {
				if (RecordManager.IsRecording()) {
					if (!RecordOptions.ConfirmChannelChange(m_UICore.GetSkin()->GetVideoHostWindow()))
						return false;
				}
				return SetChannel(Space,Index);
			}
		}
	} else if (SelInfo.TunerName.empty()) {
		return false;
	}

	return OpenTunerAndSetChannel(SelInfo.TunerName.c_str(),&SelInfo.Channel);
}


bool CAppMain::FollowChannelChange(WORD TransportStreamID,WORD ServiceID)
{
	const CChannelList *pChannelList;
	int Space,Channel;
	bool fFound=false;

	pChannelList=ChannelManager.GetCurrentRealChannelList();
	if (pChannelList!=NULL) {
		Channel=pChannelList->FindByIDs(0,TransportStreamID,ServiceID);
		if (Channel>=0) {
			Space=ChannelManager.GetCurrentSpace();
			fFound=true;
		}
	} else {
		for (int i=0;i<ChannelManager.NumSpaces();i++) {
			pChannelList=ChannelManager.GetChannelList(i);
			Channel=pChannelList->FindByIDs(0,TransportStreamID,ServiceID);
			if (Channel>=0) {
				Space=i;
				fFound=true;
				break;
			}
		}
	}
	if (!fFound)
		return false;
	if (Space==ChannelManager.GetCurrentSpace()
			&& Channel==ChannelManager.GetCurrentChannel())
		return true;
	Logger.AddLog(TEXT("ストリームの変化を検知しました。(TSID %d / SID %d)"),
				  TransportStreamID,ServiceID);
	const bool fSpaceChanged=Space!=ChannelManager.GetCurrentSpace();
	if (!ChannelManager.SetCurrentChannel(Space,Channel))
		return false;
	ChannelManager.SetCurrentServiceID(0);
	m_UICore.OnChannelChanged(CUICore::CHANNEL_CHANGED_STATUS_DETECTED
		| (fSpaceChanged ? CUICore::CHANNEL_CHANGED_STATUS_SPACE_CHANGED : 0));
	PluginManager.SendChannelChangeEvent();
	return true;
}


bool CAppMain::SetServiceByIndex(int Service)
{
	if (Service < 0)
		return false;

	WORD ServiceID;

	if (!CoreEngine.m_DtvEngine.m_TsAnalyzer.GetViewableServiceID(Service, &ServiceID))
		return false;

	return SetServiceByID(ServiceID);
}


bool CAppMain::SetServiceByID(WORD ServiceID)
{
	bool fResult;

	if (ServiceID==0) {
		AddLog(TEXT("デフォルトのサービスを選択します..."));
		fResult=CoreEngine.m_DtvEngine.SetServiceByIndex(CDtvEngine::SERVICE_DEFAULT);
		if (fResult)
			CoreEngine.m_DtvEngine.GetServiceID(&ServiceID);
	} else {
		AddLog(TEXT("サービスを選択します(SID %d)..."),ServiceID);
		fResult=CoreEngine.m_DtvEngine.SetServiceByID(ServiceID);
	}
	if (!fResult) {
		AddLog(TEXT("サービスを選択できません。"));
		return false;
	}

	ChannelManager.SetCurrentServiceID(ServiceID);

	if (ServiceID!=0) {
		int ServiceIndex=CoreEngine.m_DtvEngine.m_TsAnalyzer.GetServiceIndexByID(ServiceID);
		if (ServiceIndex>=0) {
#ifdef NETWORK_REMOCON_SUPPORT
			if (pNetworkRemocon!=NULL)
				pNetworkRemocon->SetService(ServiceIndex);
#endif

			AddLog(TEXT("サービスを変更しました。(SID %d)"),ServiceID);
		}
	}

	m_UICore.OnServiceChanged();

	return true;
}


bool CAppMain::GetCurrentServiceName(LPTSTR pszName,int MaxLength,bool fUseChannelName)
{
	if (pszName==NULL || MaxLength<1)
		return false;

	WORD ServiceID;
	if (!CoreEngine.m_DtvEngine.GetServiceID(&ServiceID))
		ServiceID=0;

	const CChannelInfo *pChannelInfo=NULL;
	if (fUseChannelName) {
		pChannelInfo=ChannelManager.GetCurrentChannelInfo();
		if (pChannelInfo!=NULL) {
			if (ServiceID==0 || pChannelInfo->GetServiceID()<=0
					|| pChannelInfo->GetServiceID()==ServiceID) {
				::lstrcpyn(pszName,pChannelInfo->GetName(),MaxLength);
				return true;
			}
		}
	}

	pszName[0]=_T('\0');

	if (ServiceID==0)
		return false;

	int Index=CoreEngine.m_DtvEngine.m_TsAnalyzer.GetServiceIndexByID(ServiceID);
	if (Index<0)
		return false;
#if 0
	if (pChannelInfo!=NULL) {
		int Length=StdUtil::snprintf(pszName,MaxLength,TEXT("#%d "),Index+1);
		pszName+=Length;
		MaxLength-=Length;
	}
#endif
	if (CoreEngine.m_DtvEngine.m_TsAnalyzer.GetServiceName(Index,pszName,MaxLength)<1)
		return false;

	return true;
}


bool CAppMain::OpenTuner(LPCTSTR pszFileName)
{
	if (IsStringEmpty(pszFileName))
		return false;
	if (CoreEngine.IsTunerOpen()
			&& IsEqualFileName(CoreEngine.GetDriverFileName(),pszFileName))
		return true;

	TRACE(TEXT("CAppMain::OpenTuner(%s)\n"),pszFileName);

	HCURSOR hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));
	bool fOK;

	SaveCurrentChannel();
	SaveChannelSettings();

	CoreEngine.m_DtvEngine.SetTracer(&StatusView);

	if (CoreEngine.IsTunerOpen()) {
		CoreEngine.CloseTuner();
	}
	if (CoreEngine.IsCasCardOpen()
			&& (DriverOptions.IsNoDescramble(pszFileName))) {
		if (CoreEngine.CloseCasCard()) {
			Logger.AddLog(TEXT("カードリーダを閉じました。"));
		}
	}

	CoreEngine.SetDriverFileName(pszFileName);
	fOK=OpenAndInitializeTuner();
	if (fOK) {
		if (!CoreEngine.IsCasCardOpen()
				&& !DriverOptions.IsNoDescramble(pszFileName)) {
			OpenCasCard();
		}

		InitializeChannel();
		PluginManager.SendDriverChangeEvent();
		::SetCursor(hcurOld);
	} else {
		PluginManager.SendDriverChangeEvent();
		::SetCursor(hcurOld);
		OnError(&CoreEngine,TEXT("BonDriverの初期化ができません。"));
	}

	CoreEngine.m_DtvEngine.SetTracer(NULL);
	StatusView.SetSingleText(NULL);
	m_UICore.OnTunerChanged();

	return fOK;
}


bool CAppMain::OpenTunerAndSetChannel(LPCTSTR pszDriverFileName,const CChannelInfo *pChannelInfo)
{
	if (IsStringEmpty(pszDriverFileName) || pChannelInfo==NULL)
		return false;

	if (RecordManager.IsRecording()) {
		if (!RecordOptions.ConfirmChannelChange(m_UICore.GetSkin()->GetVideoHostWindow()))
			return false;
	}

	if (!OpenTuner(pszDriverFileName))
		return false;

	const CChannelList *pList=ChannelManager.GetChannelList(pChannelInfo->GetSpace());
	if (pList==NULL)
		return false;
	int Index=pList->Find(-1,pChannelInfo->GetChannelIndex(),
						  pChannelInfo->GetServiceID());
	if (Index<0) {
		if (pChannelInfo->GetServiceID()!=0
				&& pChannelInfo->GetTransportStreamID()!=0) {
			Index=pList->FindByIDs(pChannelInfo->GetNetworkID(),
								   pChannelInfo->GetTransportStreamID(),
								   pChannelInfo->GetServiceID());
		}
		if (Index<0)
			return false;
	}

	return AppMain.SetChannel(pChannelInfo->GetSpace(),Index);
}


bool CAppMain::OpenTuner()
{
	TRACE(TEXT("CAppMain::OpenTuner()\n"));

	if (!CoreEngine.IsDriverSpecified())
		return false;

	CoreEngine.m_DtvEngine.SetTracer(&StatusView);
	bool fOK=true;

	if (!CoreEngine.IsTunerOpen()) {
		if (OpenAndInitializeTuner()) {
			if (!CoreEngine.IsCasCardOpen()
					&& !DriverOptions.IsNoDescramble(CoreEngine.GetDriverFileName())) {
				OpenCasCard(OPEN_CAS_CARD_NO_UI);
			}

			m_UICore.OnTunerOpened();
		} else {
			OnError(&CoreEngine,TEXT("BonDriverの初期化ができません。"));
			fOK=false;
		}
	}

	CoreEngine.m_DtvEngine.SetTracer(NULL);
	StatusView.SetSingleText(NULL);

	return fOK;
}


bool CAppMain::OpenAndInitializeTuner()
{
	CDriverOptions::BonDriverOptions Options(CoreEngine.GetDriverFileName());
	DriverOptions.GetBonDriverOptions(CoreEngine.GetDriverFileName(),&Options);

	CoreEngine.m_DtvEngine.SetStartStreamingOnDriverOpen(!Options.fIgnoreInitialStream);

	if (!CoreEngine.OpenTuner())
		return false;

	ApplyBonDriverOptions();

	return true;
}


bool CAppMain::CloseTuner()
{
	TRACE(TEXT("CAppMain::CloseTuner()\n"));

	if (CoreEngine.IsCasCardOpen()) {
		if (CoreEngine.CloseCasCard()) {
			Logger.AddLog(TEXT("カードリーダを閉じました。"));
		}
	}

	if (CoreEngine.IsTunerOpen()) {
		CoreEngine.CloseTuner();
		SaveCurrentChannel();
		ChannelManager.SetCurrentChannel(ChannelManager.GetCurrentSpace(),-1);
		m_UICore.OnTunerClosed();
	}

	return true;
}


bool CAppMain::OpenCasCard(unsigned int Flags)
{
	TRACE(TEXT("CAppMain::OpenCasCard()\n"));

	if (!CoreEngine.IsCasCardOpen()) {
		if (!CoreEngine.OpenCasCard()) {
			Logger.AddLog(TEXT("カードリーダがオープンできません。"));
			if ((Flags&OPEN_CAS_CARD_RETRY)!=0) {
				TCHAR szText[1024];
				CStaticStringFormatter Formatter(szText,lengthof(szText));
				CCardReaderErrorDialog Dialog;

				Formatter.AppendFormat(TEXT("%s\r\n"),CoreEngine.GetLastErrorText());
				if (!IsStringEmpty(CoreEngine.GetLastErrorSystemMessage()))
					Formatter.AppendFormat(TEXT("(%s)\r\n"),CoreEngine.GetLastErrorSystemMessage());
				Formatter.Append(
					TEXT("※もし正常に視聴できるのにこのダイアログが表示される場合、")
					TEXT("設定でカードリーダに「なし」を選択してください。"));
				Dialog.SetMessage(Formatter.GetString());
				while (Dialog.Show(MainWindow.GetHandle())) {
					if (CoreEngine.SetCasDevice(Dialog.GetCasDevice(),Dialog.GetReaderName())
							|| Dialog.GetCasDevice()<0)
						break;
				}
			} else {
				if ((Flags&OPEN_CAS_CARD_NO_UI)==0 && !m_fSilent)
					MainWindow.ShowErrorMessage(&CoreEngine);
			}
		}

		OutCasCardInfo();
	}

	return true;
}


bool CAppMain::ChangeCasCard(int Device,LPCTSTR pszName)
{
	if (!CoreEngine.SetCasDevice(Device,pszName)) {
		OnError(&CoreEngine);
		return false;
	}

	OutCasCardInfo();

	return true;
}


void CAppMain::OutCasCardInfo()
{
	if (CoreEngine.IsCasCardOpen()) {
		TCHAR szName[MAX_PATH];
		if (CoreEngine.m_DtvEngine.m_CasProcessor.GetCasCardName(szName,lengthof(szName))>0) {
			Logger.AddLog(TEXT("カードリーダ \"%s\" をオープンしました。"),szName);
			CCasProcessor::CasCardInfo CardInfo;
			if (CoreEngine.m_DtvEngine.m_CasProcessor.GetCasCardInfo(&CardInfo)) {
				Logger.AddLog(TEXT("(カードID %s / カード識別 %c%03d)"),
					CardInfo.CardIDText,CardInfo.CardManufacturerID,CardInfo.CardVersion);
			}
		}
	}
}


void CAppMain::ApplyBonDriverOptions()
{
	if (!CoreEngine.IsTunerOpen())
		return;

	CDriverOptions::BonDriverOptions Options(CoreEngine.GetDriverFileName());
	DriverOptions.GetBonDriverOptions(CoreEngine.GetDriverFileName(),&Options);

	//CoreEngine.m_DtvEngine.SetStartStreamingOnDriverOpen(!Options.fIgnoreInitialStream);

	CBonSrcDecoder &BonSrcDecoder=CoreEngine.m_DtvEngine.m_BonSrcDecoder;

	BonSrcDecoder.SetPurgeStreamOnChannelChange(Options.fPurgeStreamOnChannelChange);
	BonSrcDecoder.SetFirstChannelSetDelay(Options.FirstChannelSetDelay);
	BonSrcDecoder.SetMinChannelChangeInterval(Options.MinChannelChangeInterval);
}


bool CAppMain::LoadSettings()
{
	CSettings &Settings=m_Settings;

	if (!Settings.Open(m_szIniFileName,CSettings::OPEN_READ | CSettings::OPEN_WRITE_VOLATILE)) {
		Logger.AddLog(TEXT("設定ファイル \"%s\" を開けません。"),m_szIniFileName);
		return false;
	}

	if (!CmdLineOptions.m_IniValueList.empty()) {
		for (size_t i=0;i<CmdLineOptions.m_IniValueList.size();i++) {
			const CCommandLineOptions::IniEntry &Entry=CmdLineOptions.m_IniValueList[i];

			TRACE(TEXT("Override INI entry : [%s] %s=%s\n"),
				  Entry.Section.GetSafe(),Entry.Name.GetSafe(),Entry.Value.GetSafe());
			if (Settings.SetSection(Entry.Section.IsEmpty()?TEXT("Settings"):Entry.Section.Get())) {
				Settings.Write(Entry.Name.Get(),Entry.Value.GetSafe());
			}
		}
	}

	if (Settings.SetSection(TEXT("Settings"))) {
		int Value;
		TCHAR szText[MAX_PATH];
		int Left,Top,Width,Height;
		bool f;

		if (Settings.Read(TEXT("CasLibrary"),szText,MAX_PATH))
			CoreEngine.SetCasLibraryName(szText);

		Settings.Read(TEXT("EnablePlay"),&fEnablePlay);
		if (Settings.Read(TEXT("Volume"),&Value))
			CoreEngine.SetVolume(CLAMP(Value,0,CCoreEngine::MAX_VOLUME));
		int Gain=100,SurroundGain;
		Settings.Read(TEXT("VolumeNormalizeLevel"),&Gain);
		if (!Settings.Read(TEXT("SurroundGain"),&SurroundGain))
			SurroundGain=Gain;
		CoreEngine.SetAudioGainControl(Gain,SurroundGain);
		Settings.Read(TEXT("ShowInfoWindow"),&fShowPanelWindow);
		if (Settings.Read(TEXT("OptionPage"),&Value))
			OptionDialog.SetCurrentPage(Value);

		if (Settings.Read(TEXT("RecOptionFileName"),szText,MAX_PATH) && szText[0]!='\0')
			RecordManager.SetFileName(szText);
		/*
		if (Settings.Read(TEXT("RecOptionExistsOperation"),&Value))
			RecordManager.SetFileExistsOperation(
								(CRecordManager::FileExistsOperation)Value);
		*/
		/*
		if (Settings.Read(TEXT("RecOptionStopTimeSpec"),&f))
			RecordManager.SetStopTimeSpec(f);
		unsigned int Time;
		if (Settings.Read(TEXT("RecOptionStopTime"),&Time))
			RecordManager.SetStopTime(Time);
		*/

		PanelFrame.GetPosition(&Left,&Top,&Width,&Height);
		Settings.Read(TEXT("InfoLeft"),&Left);
		Settings.Read(TEXT("InfoTop"),&Top);
		Settings.Read(TEXT("InfoWidth"),&Width);
		Settings.Read(TEXT("InfoHeight"),&Height);
		PanelFrame.SetPosition(Left,Top,Width,Height);
		PanelFrame.MoveToMonitorInside();
		if (Settings.Read(TEXT("PanelFloating"),&f))
			PanelFrame.SetFloating(f);
		if (Settings.Read(TEXT("PanelDockingWidth"),&Value))
			PanelFrame.SetDockingWidth(Value);

		ProgramGuideFrame.GetPosition(&Left,&Top,&Width,&Height);
		Settings.Read(TEXT("ProgramGuideLeft"),&Left);
		Settings.Read(TEXT("ProgramGuideTop"),&Top);
		Settings.Read(TEXT("ProgramGuideWidth"),&Width);
		Settings.Read(TEXT("ProgramGuideHeight"),&Height);
		ProgramGuideFrame.SetPosition(Left,Top,Width,Height);
		ProgramGuideFrame.MoveToMonitorInside();
		if (Settings.Read(TEXT("ProgramGuideMaximized"),&f) && f)
			ProgramGuideFrame.SetMaximize(f);
		if (Settings.Read(TEXT("ProgramGuideAlwaysOnTop"),&f))
			ProgramGuideFrame.SetAlwaysOnTop(f);

		CaptureWindow.GetPosition(&Left,&Top,&Width,&Height);
		Settings.Read(TEXT("CapturePreviewLeft"),&Left);
		Settings.Read(TEXT("CapturePreviewTop"),&Top);
		Settings.Read(TEXT("CapturePreviewWidth"),&Width);
		Settings.Read(TEXT("CapturePreviewHeight"),&Height);
		CaptureWindow.SetPosition(Left,Top,Width,Height);
		CaptureWindow.MoveToMonitorInside();
		if (Settings.Read(TEXT("CaptureStatusBar"),&f))
			CaptureWindow.ShowStatusBar(f);

		StreamInfo.GetPosition(&Left,&Top,&Width,&Height);
		Settings.Read(TEXT("StreamInfoLeft"),&Left);
		Settings.Read(TEXT("StreamInfoTop"),&Top);
		Settings.Read(TEXT("StreamInfoWidth"),&Width);
		Settings.Read(TEXT("StreamInfoHeight"),&Height);
		StreamInfo.SetPosition(Left,Top,Width,Height);
		//StreamInfo.MoveToMonitorInside();

		// Experimental options
		Settings.Read(TEXT("IncrementUDPPort"),&fIncrementUDPPort);

		MainWindow.ReadSettings(Settings);
		GeneralOptions.ReadSettings(Settings);
		ViewOptions.ReadSettings(Settings);
		OSDOptions.ReadSettings(Settings);
		PanelOptions.ReadSettings(Settings);
		PlaybackOptions.ReadSettings(Settings);
		RecordOptions.ReadSettings(Settings);
		CaptureOptions.ReadSettings(Settings);
		ControllerManager.ReadSettings(Settings);
		ChannelScan.ReadSettings(Settings);
		EpgOptions.ReadSettings(Settings);
#ifdef NETWORK_REMOCON_SUPPORT
		NetworkRemoconOptions.ReadSettings(Settings);
#endif
		Logger.ReadSettings(Settings);
		ZoomOptions.ReadSettings(Settings);
	}

	StatusOptions.LoadSettings(Settings);
	SideBarOptions.LoadSettings(Settings);
	MenuOptions.LoadSettings(Settings);
	ColorSchemeOptions.LoadSettings(Settings);
	DriverOptions.LoadSettings(Settings);
	ProgramGuideOptions.LoadSettings(Settings);
	ProgramGuideFrameSettings.LoadSettings(Settings);
	PluginOptions.LoadSettings(Settings);
	RecentChannelList.LoadSettings(Settings);
	InfoPanel.LoadSettings(Settings);
	ChannelPanel.LoadSettings(Settings);
	PanAndScanOptions.LoadSettings(Settings);
	HomeDisplay.LoadSettings(Settings);

	return true;
}


bool CAppMain::SaveSettings(unsigned int Flags)
{
	CSettings Settings;
	if (!Settings.Open(m_szIniFileName,CSettings::OPEN_WRITE)) {
		TCHAR szMessage[64+MAX_PATH];
		StdUtil::snprintf(szMessage,lengthof(szMessage),
						  TEXT("設定ファイル \"%s\" を開けません。"),
						  m_szIniFileName);
		Logger.AddLog(TEXT("%s"),szMessage);
		if (!m_fSilent)
			MainWindow.ShowErrorMessage(szMessage);
		return false;
	}

	if (Settings.SetSection(TEXT("Settings"))) {
		Settings.Write(TEXT("Version"),VERSION_TEXT);

		if ((Flags&SETTINGS_SAVE_STATUS)!=0) {
			int Left,Top,Width,Height;

			//Settings.Write(TEXT("CasLibrary"),CoreEngine.GetCasLibraryName());
			Settings.Write(TEXT("EnablePlay"),fEnablePlay);
			Settings.Write(TEXT("Volume"),CoreEngine.GetVolume());
			int Gain,SurroundGain;
			CoreEngine.GetAudioGainControl(&Gain,&SurroundGain);
			Settings.Write(TEXT("VolumeNormalizeLevel"),Gain);
			Settings.Write(TEXT("SurroundGain"),SurroundGain);
			Settings.Write(TEXT("ShowInfoWindow"),fShowPanelWindow);
			Settings.Write(TEXT("OptionPage"),OptionDialog.GetCurrentPage());

			if (RecordManager.GetFileName()!=NULL)
				Settings.Write(TEXT("RecOptionFileName"),RecordManager.GetFileName());
			/*
			Settings.Write(TEXT("RecOptionExistsOperation"),
											RecordManager.GetFileExistsOperation());
			*/
			/*
			Settings.Write(TEXT("RecOptionStopTimeSpec"),RecordManager.GetStopTimeSpec());
			Settings.Write(TEXT("RecOptionStopTime"),RecordManager.GetStopTime());
			*/

			PanelFrame.GetPosition(&Left,&Top,&Width,&Height);
			Settings.Write(TEXT("InfoLeft"),Left);
			Settings.Write(TEXT("InfoTop"),Top);
			Settings.Write(TEXT("InfoWidth"),Width);
			Settings.Write(TEXT("InfoHeight"),Height);
			Settings.Write(TEXT("PanelFloating"),PanelFrame.GetFloating());
			Settings.Write(TEXT("PanelDockingWidth"),PanelFrame.GetDockingWidth());
			Settings.Write(TEXT("InfoCurTab"),PanelForm.GetCurPageID());

			ProgramGuideFrame.GetPosition(&Left,&Top,&Width,&Height);
			Settings.Write(TEXT("ProgramGuideLeft"),Left);
			Settings.Write(TEXT("ProgramGuideTop"),Top);
			Settings.Write(TEXT("ProgramGuideWidth"),Width);
			Settings.Write(TEXT("ProgramGuideHeight"),Height);
			Settings.Write(TEXT("ProgramGuideMaximized"),ProgramGuideFrame.GetMaximize());
			Settings.Write(TEXT("ProgramGuideAlwaysOnTop"),ProgramGuideFrame.GetAlwaysOnTop());

			CaptureWindow.GetPosition(&Left,&Top,&Width,&Height);
			Settings.Write(TEXT("CapturePreviewLeft"),Left);
			Settings.Write(TEXT("CapturePreviewTop"),Top);
			Settings.Write(TEXT("CapturePreviewWidth"),Width);
			Settings.Write(TEXT("CapturePreviewHeight"),Height);
			Settings.Write(TEXT("CaptureStatusBar"),CaptureWindow.IsStatusBarVisible());

			StreamInfo.GetPosition(&Left,&Top,&Width,&Height);
			Settings.Write(TEXT("StreamInfoLeft"),Left);
			Settings.Write(TEXT("StreamInfoTop"),Top);
			Settings.Write(TEXT("StreamInfoWidth"),Width);
			Settings.Write(TEXT("StreamInfoHeight"),Height);
			Settings.Write(TEXT("IncrementUDPPort"),fIncrementUDPPort);

			MainWindow.WriteSettings(Settings);
		}
	}

	static const struct {
		CSettingsBase *pSettings;
		bool fHasStatus;
	} SettingsList[] = {
		{&GeneralOptions,				true},
		{&StatusOptions,				true},
		{&SideBarOptions,				true},
		{&PanelOptions,					true},
		{&DriverOptions,				true},
		{&PlaybackOptions,				true},
		{&RecordOptions,				true},
		{&CaptureOptions,				true},
		{&PluginOptions,				true},
		{&ViewOptions,					false},
		{&OSDOptions,					false},
		{&MenuOptions,					false},
		{&ColorSchemeOptions,			false},
		{&OperationOptions,				false},
		{&Accelerator,					false},
		{&ControllerManager,			false},
		{&ChannelScan,					false},
		{&EpgOptions,					false},
		{&ProgramGuideOptions,			true},
#ifdef NETWORK_REMOCON_SUPPORT
		{&NetworkRemoconOptions,		false},
#endif
		{&Logger,						false},
	//	{&ZoomOptions,					false},
	//	{&PanAndScanOptions,			false},
		{&ProgramGuideFrameSettings,	true},
		{&HomeDisplay,					true},
	};

	for (size_t i=0;i<lengthof(SettingsList);i++) {
		CSettingsBase *pSettings=SettingsList[i].pSettings;
		if (((Flags&SETTINGS_SAVE_OPTIONS)!=0 && pSettings->IsChanged())
				|| ((Flags&SETTINGS_SAVE_STATUS)!=0 && SettingsList[i].fHasStatus)) {
			if (pSettings->SaveSettings(Settings))
				pSettings->ClearChanged();
		}
	}

	if ((Flags&SETTINGS_SAVE_STATUS)!=0) {
		RecentChannelList.SaveSettings(Settings);
		InfoPanel.SaveSettings(Settings);
		ChannelPanel.SaveSettings(Settings);
	}

	return true;
}


void CAppMain::InitializeCommandSettings()
{
	Accelerator.Initialize(MainWindow.GetHandle(),&MainMenu,
						   m_Settings,&CommandList);
	OperationOptions.Initialize(m_Settings,&CommandList);

	m_Settings.Close();
}


bool CAppMain::SaveCurrentChannel()
{
	if (!IsStringEmpty(CoreEngine.GetDriverFileName())) {
		const CChannelInfo *pInfo=ChannelManager.GetCurrentRealChannelInfo();

		if (pInfo!=NULL) {
			CDriverOptions::ChannelInfo ChInfo;

			ChInfo.Space=pInfo->GetSpace();
			ChInfo.Channel=pInfo->GetChannelIndex();
			ChInfo.ServiceID=pInfo->GetServiceID();
			ChInfo.TransportStreamID=pInfo->GetTransportStreamID();
			ChInfo.fAllChannels=ChannelManager.GetCurrentSpace()==CChannelManager::SPACE_ALL;
			DriverOptions.SetLastChannel(CoreEngine.GetDriverFileName(),&ChInfo);
		}
	}
	return true;
}


bool CAppMain::ShowHelpContent(int ID)
{
	return HtmlHelpClass.ShowContent(ID);
}


bool CAppMain::GenerateRecordFileName(LPTSTR pszFileName,int MaxFileName) const
{
	CRecordManager::EventInfo EventInfo;
	const CChannelInfo *pChannelInfo=ChannelManager.GetCurrentChannelInfo();
	WORD ServiceID;
	TCHAR szServiceName[32],szEventName[256];

	EventInfo.pszChannelName=NULL;
	EventInfo.ChannelNo=0;
	EventInfo.pszServiceName=NULL;
	EventInfo.pszEventName=NULL;
	if (pChannelInfo!=NULL) {
		EventInfo.pszChannelName=pChannelInfo->GetName();
		if (pChannelInfo->GetChannelNo()!=0)
			EventInfo.ChannelNo=pChannelInfo->GetChannelNo();
		else if (pChannelInfo->GetServiceID()>0)
			EventInfo.ChannelNo=pChannelInfo->GetServiceID();
	}
	if (CoreEngine.m_DtvEngine.GetServiceID(&ServiceID)) {
		int Index=CoreEngine.m_DtvEngine.m_TsAnalyzer.GetServiceIndexByID(ServiceID);
		if (CoreEngine.m_DtvEngine.m_TsAnalyzer.GetServiceName(Index,szServiceName,lengthof(szServiceName)))
			EventInfo.pszServiceName=szServiceName;
		if (!CoreEngine.m_DtvEngine.GetServiceID(&EventInfo.ServiceID))
			EventInfo.ServiceID=0;
		bool fNext=false;
		SYSTEMTIME stCur,stStart;
		if (!CoreEngine.m_DtvEngine.m_TsAnalyzer.GetTotTime(&stCur))
			GetCurrentJST(&stCur);
		if (CoreEngine.m_DtvEngine.GetEventTime(&stStart,NULL,true)) {
			LONGLONG Diff=DiffSystemTime(&stCur,&stStart);
			if (Diff>=0 && Diff<60*1000)
				fNext=true;
		}
		if (CoreEngine.m_DtvEngine.GetEventName(szEventName,lengthof(szEventName),fNext))
			EventInfo.pszEventName=szEventName;
		EventInfo.EventID=CoreEngine.m_DtvEngine.GetEventID(fNext);
		EventInfo.stTotTime=stCur;
	}
	return RecordManager.GenerateFileName(pszFileName,MaxFileName,&EventInfo);
}


bool CAppMain::StartRecord(LPCTSTR pszFileName,
						   const CRecordManager::TimeSpecInfo *pStartTime,
						   const CRecordManager::TimeSpecInfo *pStopTime,
						   CRecordManager::RecordClient Client,
						   bool fTimeShift)
{
	if (RecordManager.IsRecording())
		return false;
	RecordManager.SetFileName(pszFileName);
	RecordManager.SetStartTimeSpec(pStartTime);
	RecordManager.SetStopTimeSpec(pStopTime);
	RecordManager.SetStopOnEventEnd(false);
	RecordManager.SetClient(Client);
	RecordOptions.ApplyOptions(&RecordManager);
	if (CmdLineOptions.m_fRecordCurServiceOnly)
		RecordManager.SetCurServiceOnly(true);
	if (RecordManager.IsReserved()) {
		StatusView.UpdateItem(STATUS_ITEM_RECORD);
		return true;
	}

	OpenTuner();

	TCHAR szFileName[MAX_PATH*2];
	if (IsStringEmpty(pszFileName)) {
		LPCTSTR pszErrorMessage;

		if (!RecordOptions.GenerateFilePath(szFileName,lengthof(szFileName),
											&pszErrorMessage)) {
			MainWindow.ShowErrorMessage(pszErrorMessage);
			return false;
		}
		RecordManager.SetFileName(szFileName);
	}
	if (!GenerateRecordFileName(szFileName,lengthof(szFileName)))
		return false;
	PluginManager.SendStartRecordEvent(&RecordManager,szFileName,lengthof(szFileName));
	CoreEngine.ResetErrorCount();
	if (!RecordManager.StartRecord(&CoreEngine.m_DtvEngine,szFileName,fTimeShift)) {
		OnError(&RecordManager,TEXT("録画を開始できません。"));
		return false;
	}
	ResidentManager.SetStatus(CResidentManager::STATUS_RECORDING,
							  CResidentManager::STATUS_RECORDING);
	Logger.AddLog(TEXT("録画開始 %s"),szFileName);
	m_UICore.OnRecordingStarted();
	return true;
}


bool CAppMain::ModifyRecord(LPCTSTR pszFileName,
							const CRecordManager::TimeSpecInfo *pStartTime,
							const CRecordManager::TimeSpecInfo *pStopTime,
							CRecordManager::RecordClient Client)
{
	RecordManager.SetFileName(pszFileName);
	RecordManager.SetStartTimeSpec(pStartTime);
	RecordManager.SetStopTimeSpec(pStopTime);
	RecordManager.SetClient(Client);
	StatusView.UpdateItem(STATUS_ITEM_RECORD);
	return true;
}


bool CAppMain::StartReservedRecord()
{
	TCHAR szFileName[MAX_PATH];

	/*
	if (!RecordManager.IsReserved())
		return false;
	*/
	if (!IsStringEmpty(RecordManager.GetFileName())) {
		if (!GenerateRecordFileName(szFileName,MAX_PATH))
			return false;
		/*
		if (!RecordManager.DoFileExistsOperation(MainWindow.GetVideoHostWindow(),szFileName))
			return false;
		*/
	} else {
		LPCTSTR pszErrorMessage;

		if (!RecordOptions.GenerateFilePath(szFileName,lengthof(szFileName),
											&pszErrorMessage)) {
			MainWindow.ShowErrorMessage(pszErrorMessage);
			return false;
		}
		RecordManager.SetFileName(szFileName);
		if (!GenerateRecordFileName(szFileName,MAX_PATH))
			return false;
	}
	OpenTuner();
	PluginManager.SendStartRecordEvent(&RecordManager,szFileName,lengthof(szFileName));
	CoreEngine.ResetErrorCount();
	if (!RecordManager.StartRecord(&CoreEngine.m_DtvEngine,szFileName)) {
		RecordManager.CancelReserve();
		OnError(&RecordManager,TEXT("録画を開始できません。"));
		return false;
	}
	ResidentManager.SetStatus(CResidentManager::STATUS_RECORDING,
							  CResidentManager::STATUS_RECORDING);
	Logger.AddLog(TEXT("録画開始 %s"),szFileName);
	m_UICore.OnRecordingStarted();
	return true;
}


bool CAppMain::CancelReservedRecord()
{
	if (!RecordManager.CancelReserve())
		return false;
	StatusView.UpdateItem(STATUS_ITEM_RECORD);
	return true;
}


bool CAppMain::StopRecord()
{
	if (!RecordManager.IsRecording())
		return false;

	TCHAR szFileName[MAX_PATH],szSize[32];
	StdUtil::strncpy(szFileName,lengthof(szFileName),
					 RecordManager.GetRecordTask()->GetFileName());

	RecordManager.StopRecord();
	RecordOptions.Apply(CRecordOptions::UPDATE_RECORDSTREAM);
	CoreEngine.m_DtvEngine.SetDescrambleCurServiceOnly(
						GeneralOptions.GetDescrambleCurServiceOnly());

	UInt64ToString(CoreEngine.m_DtvEngine.m_FileWriter.GetWriteSize(),
				   szSize,lengthof(szSize));
	Logger.AddLog(TEXT("録画停止 %s (書き出しサイズ %s Bytes)"),szFileName,szSize);

	ResidentManager.SetStatus(0,CResidentManager::STATUS_RECORDING);
	m_UICore.OnRecordingStopped();
	return true;
}


bool CAppMain::RelayRecord(LPCTSTR pszFileName)
{
	if (IsStringEmpty(pszFileName) || !RecordManager.IsRecording())
		return false;
	if (!RecordManager.RelayFile(pszFileName)) {
		OnError(&RecordManager,TEXT("録画ファイルを切り替えできません。"));
		return false;
	}
	Logger.AddLog(TEXT("録画ファイルを切り替えました %s"),pszFileName);
	PluginManager.SendRelayRecordEvent(pszFileName);
	return true;
}


LPCTSTR CAppMain::GetDefaultRecordFolder() const
{
	return RecordOptions.GetSaveFolder();
}


void CAppMain::BeginChannelScan(int Space)
{
	ChannelManager.SetCurrentChannel(Space,-1);
	m_UICore.OnChannelChanged(CUICore::CHANNEL_CHANGED_STATUS_SPACE_CHANGED);
}


bool CAppMain::IsChannelScanning() const
{
	return ChannelScan.IsScanning();
}


bool CAppMain::IsDriverNoSignalLevel(LPCTSTR pszFileName) const
{
	return DriverOptions.IsNoSignalLevel(pszFileName);
}


bool CAppMain::IsFirstExecute() const
{
	return m_fFirstExecute;
}


void CAppMain::SetProgress(int Pos,int Max)
{
	TaskbarManager.SetProgress(Pos,Max);
}


void CAppMain::EndProgress()
{
	TaskbarManager.EndProgress();
}


COLORREF CAppMain::GetColor(LPCTSTR pszText) const
{
	return ColorSchemeOptions.GetColor(pszText);
}


CCoreEngine *CAppMain::GetCoreEngine()
{
	return &CoreEngine;
}


const CCoreEngine *CAppMain::GetCoreEngine() const
{
	return &CoreEngine;
}


CUICore *CAppMain::GetUICore()
{
	return &m_UICore;
}


const CCommandList *CAppMain::GetCommandList() const
{
	return &CommandList;
}


const CChannelManager *CAppMain::GetChannelManager() const
{
	return &ChannelManager;
}


const CRecordManager *CAppMain::GetRecordManager() const
{
	return &RecordManager;
}


const CDriverManager *CAppMain::GetDriverManager() const
{
	return &DriverManager;
}


CEpgProgramList *CAppMain::GetEpgProgramList() const
{
	return &EpgProgramList;
}


CLogoManager *CAppMain::GetLogoManager() const
{
	return &LogoManager;
}


CControllerManager *CAppMain::GetControllerManager() const
{
	return &ControllerManager;
}


CFavoritesManager *CAppMain::GetFavoritesManager() const
{
	return &FavoritesManager;
}


CRecentChannelList *CAppMain::GetRecentChannelList() const
{
	return &RecentChannelList;
}


CAppMain &GetAppClass()
{
	return AppMain;
}




class CChannelMenuManager
{
public:
	bool InitPopup(HMENU hmenuParent,HMENU hmenu);
	bool CreateChannelMenu(const CChannelList *pChannelList,int CurChannel,
						   UINT Command,HMENU hmenu,HWND hwnd,unsigned int Flags=0);
};

bool CChannelMenuManager::InitPopup(HMENU hmenuParent,HMENU hmenu)
{
	bool fChannelMenu=false;
	int Count=::GetMenuItemCount(hmenuParent);
	int i;
	for (i=0;i<Count;i++) {
		if (::GetSubMenu(hmenuParent,i)==hmenu) {
			fChannelMenu=true;
			break;
		}
		if ((::GetMenuState(hmenuParent,i,MF_BYPOSITION)&MF_POPUP)==0)
			break;
	}

	if (fChannelMenu) {
		const CChannelList *pChannelList;
		int Command=CM_SPACE_CHANNEL_FIRST;

		pChannelList=ChannelManager.GetAllChannelList();
		if (ChannelManager.NumSpaces()>1) {
			if (i==0) {
				CreateChannelMenu(
					pChannelList,
					ChannelManager.GetCurrentSpace()==CChannelManager::SPACE_ALL?
						ChannelManager.GetCurrentChannel():-1,
					Command,hmenu,MainWindow.GetHandle(),
					CChannelMenu::FLAG_SPACEBREAK);
				return true;
			}
			i--;
		}
		if (i>=ChannelManager.NumSpaces()) {
			TRACE(TEXT("CChannelMenuManager::InitPopup() : Invalid space %d\n"),i);
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
			Command,hmenu,MainWindow.GetHandle());
		return true;
	}

	return false;
}

bool CChannelMenuManager::CreateChannelMenu(
	const CChannelList *pChannelList,int CurChannel,
	UINT Command,HMENU hmenu,HWND hwnd,unsigned int Flags)
{
	if (pChannelList==NULL)
		return false;
	const bool fEventInfo=
		(Flags&CChannelMenu::FLAG_SHOWEVENTINFO)!=0
		|| pChannelList->NumEnableChannels()<=MenuOptions.GetMaxChannelMenuEventInfo();
	unsigned int MenuFlags=CChannelMenu::FLAG_SHOWLOGO | Flags;
	if (fEventInfo)
		MenuFlags|=CChannelMenu::FLAG_SHOWEVENTINFO;
	else
		MenuFlags|=CChannelMenu::FLAG_SHOWTOOLTIP;
	return ChannelMenu.Create(pChannelList,CurChannel,
							  Command,hmenu,hwnd,MenuFlags,
							  fEventInfo?0:MenuOptions.GetMaxChannelMenuRows());
}

static CChannelMenuManager ChannelMenuManager;


class CTunerSelectMenu
{
	struct PopupInfo {
		const CChannelList *pChannelList;
		int Command;
		PopupInfo(const CChannelList *pList,int Cmd)
			: pChannelList(pList)
			, Command(Cmd)
		{
		}
	};

	CPopupMenu m_Menu;
	HWND m_hwnd;
	std::vector<PopupInfo> m_PopupList;

public:
	CTunerSelectMenu();
	~CTunerSelectMenu();
	bool Create(HWND hwnd);
	void Destroy();
	bool Show(UINT Flags,int x,int y);
	bool OnInitMenuPopup(HMENU hmenu);
};

CTunerSelectMenu::CTunerSelectMenu()
{
}

CTunerSelectMenu::~CTunerSelectMenu()
{
	Destroy();
}

bool CTunerSelectMenu::Create(HWND hwnd)
{
	Destroy();

	m_Menu.Create();
	m_hwnd=hwnd;

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

	for (i=0;i<DriverManager.NumDrivers();i++) {
		CDriverInfo *pDriverInfo=DriverManager.GetDriverInfo(i);

		if (IsEqualFileName(pDriverInfo->GetFileName(),CoreEngine.GetDriverFileName())) {
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

void CTunerSelectMenu::Destroy()
{
	m_Menu.Destroy();
	m_hwnd=NULL;
	m_PopupList.clear();
}

bool CTunerSelectMenu::Show(UINT Flags,int x,int y)
{
	POINT pt={x,y};
	return m_Menu.Show(m_hwnd,&pt,Flags);
}

bool CTunerSelectMenu::OnInitMenuPopup(HMENU hmenu)
{
	if (!m_Menu.IsCreated())
		return false;

	if (ChannelMenuManager.InitPopup(m_Menu.GetPopupHandle(),hmenu))
		return true;

	bool fChannelMenu=false;
	int Count=m_Menu.GetItemCount();
	int i,j;
	i=ChannelManager.NumSpaces();
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
		ChannelMenuManager.CreateChannelMenu(Info.pChannelList,-1,Info.Command,hmenu,m_hwnd);
		return true;
	}

	return false;
}

static CTunerSelectMenu TunerSelectMenu;




CUICore::CUICore()
	: m_pSkin(NULL)
	, m_fStandby(false)
	, m_fFullscreen(false)
	, m_fAlwaysOnTop(false)

	, m_hicoLogoBig(NULL)
	, m_hicoLogoSmall(NULL)

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
	if (m_hicoLogoBig!=NULL)
		::DeleteObject(m_hicoLogoBig);
	if (m_hicoLogoSmall!=NULL)
		::DeleteObject(m_hicoLogoSmall);
}

bool CUICore::SetSkin(CUISkin *pSkin)
{
	if (m_pSkin!=NULL)
		m_pSkin->m_pCore=NULL;
	if (pSkin!=NULL)
		pSkin->m_pCore=this;
	m_pSkin=pSkin;
	return true;
}

HWND CUICore::GetMainWindow() const
{
	if (m_pSkin==NULL)
		return NULL;
	return m_pSkin->GetMainWindow();
}

bool CUICore::InitializeViewer()
{
	if (m_pSkin==NULL)
		return false;
	bool fOK=m_pSkin->InitializeViewer();
	m_fViewerInitializeError=!fOK;
	return fOK;
}

bool CUICore::FinalizeViewer()
{
	if (m_pSkin==NULL)
		return false;
	return m_pSkin->FinalizeViewer();
}

bool CUICore::IsViewerEnabled() const
{
	if (m_pSkin==NULL)
		return false;
	return m_pSkin->IsViewerEnabled();
}

bool CUICore::EnableViewer(bool fEnable)
{
	if (m_pSkin==NULL)
		return false;
	return m_pSkin->EnableViewer(fEnable);
}

HWND CUICore::GetViewerWindow() const
{
	if (m_pSkin==NULL)
		return false;
	return m_pSkin->GetViewerWindow();
}

bool CUICore::SetZoomRate(int Rate,int Factor)
{
	if (m_pSkin==NULL)
		return false;
	return m_pSkin->SetZoomRate(Rate,Factor);
}

bool CUICore::GetZoomRate(int *pRate,int *pFactor) const
{
	if (m_pSkin==NULL)
		return false;
	if (!m_pSkin->GetZoomRate(pRate,pFactor)
			|| (pRate!=NULL && *pRate<1) || (pFactor!=NULL && *pFactor<1))
		return false;
	return true;
}

int CUICore::GetZoomPercentage() const
{
	if (m_pSkin==NULL)
		return 0;
	int Rate,Factor;
	if (!m_pSkin->GetZoomRate(&Rate,&Factor) || Factor==0)
		return false;
	return (Rate*100+Factor/2)/Factor;
}

bool CUICore::GetPanAndScan(PanAndScanInfo *pInfo) const
{
	if (m_pSkin==NULL || pInfo==NULL)
		return false;
	return m_pSkin->GetPanAndScan(pInfo);
}

bool CUICore::SetPanAndScan(const PanAndScanInfo &Info)
{
	if (m_pSkin==NULL)
		return false;
	return m_pSkin->SetPanAndScan(Info);
}

int CUICore::GetVolume() const
{
	return CoreEngine.GetVolume();
}

bool CUICore::SetVolume(int Volume,bool fOSD)
{
	if (!CoreEngine.SetVolume(Volume))
		return false;
	if (m_pSkin!=NULL)
		m_pSkin->OnVolumeChanged(fOSD);
	PluginManager.SendVolumeChangeEvent(Volume,false);
	return true;
}

bool CUICore::GetMute() const
{
	return CoreEngine.GetMute();
}

bool CUICore::SetMute(bool fMute)
{
	if (fMute!=GetMute()) {
		if (!CoreEngine.SetMute(fMute))
			return false;
		if (m_pSkin!=NULL)
			m_pSkin->OnMuteChanged();
		PluginManager.SendVolumeChangeEvent(GetVolume(),fMute);
	}
	return true;
}

int CUICore::GetStereoMode() const
{
	return CoreEngine.GetStereoMode();
}

bool CUICore::SetStereoMode(int StereoMode)
{
	/*if (StereoMode!=GetStereoMode())*/ {
		if (!CoreEngine.SetStereoMode(StereoMode))
			return false;
		if (m_pSkin!=NULL)
			m_pSkin->OnStereoModeChanged();
		PluginManager.SendStereoModeChangeEvent(StereoMode);
	}
	return true;
}

int CUICore::GetAudioStream() const
{
	return CoreEngine.m_DtvEngine.GetAudioStream();
}

int CUICore::GetNumAudioStreams() const
{
	return CoreEngine.m_DtvEngine.GetAudioStreamNum();
}

bool CUICore::SetAudioStream(int Stream)
{
	if (Stream!=GetAudioStream()) {
		if (!CoreEngine.m_DtvEngine.SetAudioStream(Stream))
			return false;
		if (m_pSkin!=NULL)
			m_pSkin->OnAudioStreamChanged();
		PluginManager.SendAudioStreamChangeEvent(Stream);
	}
	return true;
}

bool CUICore::SwitchStereoMode()
{
	return SetStereoMode((GetStereoMode()+1)%3);
}

bool CUICore::SwitchAudio()
{
	const int NumChannels=CoreEngine.m_DtvEngine.GetAudioChannelNum();
	bool fResult;

	if (NumChannels==CMediaViewer::AUDIO_CHANNEL_DUALMONO) {
		fResult=SwitchStereoMode();
	} else {
		const int NumStreams=CoreEngine.m_DtvEngine.GetAudioStreamNum();

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
	if (pszText==NULL || MaxLength<1)
		return 0;

	CStaticStringFormatter Formatter(pszText,MaxLength);

	const int NumChannels=CoreEngine.m_DtvEngine.GetAudioChannelNum();
	if (NumChannels==CMediaViewer::AUDIO_CHANNEL_INVALID)
		return 0;

	if (CoreEngine.m_DtvEngine.GetAudioStreamNum()>1)
		Formatter.AppendFormat(TEXT("#%d: "),CoreEngine.m_DtvEngine.GetAudioStream()+1);

	switch (NumChannels) {
	case 1:
		Formatter.Append(TEXT("Mono"));
		break;
	case CMediaViewer::AUDIO_CHANNEL_DUALMONO:
	case 2:
		{
			const int StereoMode=AppMain.GetUICore()->GetStereoMode();
			CTsAnalyzer::EventAudioInfo AudioInfo;
			bool fValidAudioInfo=CoreEngine.m_DtvEngine.GetEventAudioInfo(&AudioInfo);

			if (NumChannels==CMediaViewer::AUDIO_CHANNEL_DUALMONO
					/*|| (fValidAudioInfo && AudioInfo.ComponentType==0x02)*/) {
				// Dual mono
				// ES multilingual flag が立っているのに両方日本語の場合がある
				if (fValidAudioInfo
						&& AudioInfo.bESMultiLingualFlag
						&& AudioInfo.LanguageCode!=AudioInfo.LanguageCode2) {
					// 二カ国語
					Formatter.Append(TEXT("[二] "));
					switch (StereoMode) {
					case 0:
						Formatter.AppendFormat(TEXT("%s+%s"),
							EpgUtil::GetLanguageText(AudioInfo.LanguageCode,EpgUtil::LANGUAGE_TEXT_SHORT),
							EpgUtil::GetLanguageText(AudioInfo.LanguageCode2,EpgUtil::LANGUAGE_TEXT_SHORT));
						break;
					case 1:
						Formatter.Append(EpgUtil::GetLanguageText(AudioInfo.LanguageCode,EpgUtil::LANGUAGE_TEXT_SIMPLE));
						break;
					case 2:
						Formatter.Append(EpgUtil::GetLanguageText(AudioInfo.LanguageCode2,EpgUtil::LANGUAGE_TEXT_SIMPLE));
						break;
					}
				} else {
					Formatter.AppendFormat(TEXT("Mono (%s)"),
										   StereoMode==0?TEXT("主+副"):
										   StereoMode==1?TEXT("主"):TEXT("副"));
				}
			} else {
				Formatter.Append(TEXT("Stereo"));
				if (StereoMode!=0)
					Formatter.Append(StereoMode==1?TEXT("(L)"):TEXT("(R)"));
			}
		}
		break;
	case 6:
		Formatter.Append(TEXT("5.1ch"));
		break;
	default:
		Formatter.AppendFormat(TEXT("%dch"),NumChannels);
		break;
	}

	return (int)Formatter.Length();
}

bool CUICore::SetStandby(bool fStandby)
{
	if (m_fStandby!=fStandby) {
		if (m_pSkin!=NULL) {
			if (!m_pSkin->OnStandbyChange(fStandby))
				return false;
		}
		m_fStandby=fStandby;
	}
	return true;
}

bool CUICore::GetResident() const
{
	return ResidentManager.GetResident();
}

bool CUICore::SetResident(bool fResident)
{
	return ResidentManager.SetResident(fResident);
}

bool CUICore::SetFullscreen(bool fFullscreen)
{
	if (m_fFullscreen!=fFullscreen) {
		if (m_pSkin==NULL)
			return false;
		if (!m_pSkin->OnFullscreenChange(fFullscreen))
			return false;
		m_fFullscreen=fFullscreen;
		PluginManager.SendFullscreenChangeEvent(fFullscreen);
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
		if (m_pSkin==NULL)
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
		bool fNoScreenSaver=ViewOptions.GetNoScreenSaver();
		bool fNoMonitorLowPower=ViewOptions.GetNoMonitorLowPower();
		bool fNoMonitorLowPowerActiveOnly=ViewOptions.GetNoMonitorLowPowerActiveOnly();

		if (!fNoScreenSaver && m_fScreenSaverActiveOriginal) {
			SystemParametersInfo(SPI_SETSCREENSAVEACTIVE,TRUE,NULL,
								 SPIF_UPDATEINIFILE/* | SPIF_SENDWININICHANGE*/);
			m_fScreenSaverActiveOriginal=FALSE;
		}
		if (!fNoMonitorLowPower || fNoMonitorLowPowerActiveOnly) {
#if 1
			if (hwnd!=NULL)
				::KillTimer(hwnd,CUISkin::TIMER_ID_DISPLAY);
#else
			if (m_fPowerOffActiveOriginal) {
				SystemParametersInfo(SPI_SETPOWEROFFACTIVE,TRUE,NULL,
									 SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
				m_fPowerOffActiveOriginal=FALSE;
			}
			if (m_fLowPowerActiveOriginal) {
				SystemParametersInfo(SPI_SETLOWPOWERACTIVE,TRUE,NULL,
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
				SystemParametersInfo(SPI_SETSCREENSAVEACTIVE,FALSE,NULL,
									 0/*SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE*/);
		}
		if (fNoMonitorLowPower && !fNoMonitorLowPowerActiveOnly) {
#if 1
			// SetThreadExecutionState() を呼ぶタイマー
			if (hwnd!=NULL)
				::SetTimer(hwnd,CUISkin::TIMER_ID_DISPLAY,10000,NULL);
#else
			if (!m_fPowerOffActiveOriginal) {
				if (!SystemParametersInfo(SPI_GETPOWEROFFACTIVE,0,
										  &m_fPowerOffActiveOriginal,0))
					m_fPowerOffActiveOriginal=FALSE;
				if (m_fPowerOffActiveOriginal)
					SystemParametersInfo(SPI_SETPOWEROFFACTIVE,FALSE,NULL,
								SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			}
			if (!m_fLowPowerActiveOriginal) {
				if (!SystemParametersInfo(SPI_GETLOWPOWERACTIVE,0,
										  &m_fLowPowerActiveOriginal,0))
					m_fLowPowerActiveOriginal=FALSE;
				if (m_fLowPowerActiveOriginal)
					SystemParametersInfo(SPI_SETLOWPOWERACTIVE,FALSE,NULL,
								SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			}
#endif
		}
	} else {
		if (hwnd!=NULL)
			::KillTimer(hwnd,CUISkin::TIMER_ID_DISPLAY);
		if (m_fScreenSaverActiveOriginal) {
			::SystemParametersInfo(SPI_SETSCREENSAVEACTIVE,TRUE,NULL,
								SPIF_UPDATEINIFILE/* | SPIF_SENDWININICHANGE*/);
			m_fScreenSaverActiveOriginal=FALSE;
		}
#if 0
		if (m_fPowerOffActiveOriginal) {
			::SystemParametersInfo(SPI_SETPOWEROFFACTIVE,TRUE,NULL,
								   SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
			m_fPowerOffActiveOriginal=FALSE;
		}
		if (m_fLowPowerActiveOriginal) {
			::SystemParametersInfo(SPI_SETLOWPOWERACTIVE,TRUE,NULL,
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

	if (pPos!=NULL)
		pt=*pPos;
	else
		::GetCursorPos(&pt);

	const bool fDefault=(Flags&POPUPMENU_DEFAULT)!=0;
	std::vector<int> ItemList;
	if (!fDefault)
		MenuOptions.GetMenuItemList(&ItemList);

	MainMenu.Show(TPM_RIGHTBUTTON,pt.x,pt.y,m_pSkin->GetMainWindow(),true,
				  fDefault?NULL:&ItemList);
}

void CUICore::PopupSubMenu(int SubMenu,const POINT *pPos,UINT Flags)
{
	POINT pt;

	if (pPos!=NULL)
		pt=*pPos;
	else
		::GetCursorPos(&pt);
	MainMenu.PopupSubMenu(SubMenu,Flags,pt.x,pt.y,m_pSkin->GetMainWindow());
}

bool CUICore::ShowSpecialMenu(MenuType Menu,const POINT *pPos,UINT Flags)
{
	POINT pt;

	if (pPos!=NULL)
		pt=*pPos;
	else
		::GetCursorPos(&pt);
	switch (Menu) {
	case MENU_TUNERSELECT:
		TunerSelectMenu.Create(GetMainWindow());
		TunerSelectMenu.Show(Flags,pt.x,pt.y);
		TunerSelectMenu.Destroy();
		break;

	case MENU_RECORD:
		{
			CPopupMenu Menu(hInst,IDM_RECORD);

			Menu.CheckItem(CM_RECORDEVENT,RecordManager.GetStopOnEventEnd());
			Menu.EnableItem(CM_RECORD_PAUSE,RecordManager.IsRecording());
			Menu.CheckItem(CM_RECORD_PAUSE,RecordManager.IsPaused());
			bool fTimeShift=RecordOptions.IsTimeShiftRecordingEnabled();
			Menu.EnableItem(CM_TIMESHIFTRECORDING,fTimeShift && !RecordManager.IsRecording());
			Menu.CheckItem(CM_ENABLETIMESHIFTRECORDING,fTimeShift);
			Menu.EnableItem(CM_SHOWRECORDREMAINTIME,
				RecordManager.IsRecording() && RecordManager.IsStopTimeSpecified());
			Menu.CheckItem(CM_SHOWRECORDREMAINTIME,RecordOptions.GetShowRemainTime());
			Menu.CheckItem(CM_EXITONRECORDINGSTOP,MainWindow.GetExitOnRecordingStop());
			Accelerator.SetMenuAccel(Menu.GetPopupHandle());
			Menu.Show(GetMainWindow(),&pt,Flags);
		}
		break;

	case MENU_CAPTURE:
		{
			CPopupMenu Menu(hInst,IDM_CAPTURE);

			Menu.CheckRadioItem(CM_CAPTURESIZE_FIRST,CM_CAPTURESIZE_LAST,
				CM_CAPTURESIZE_FIRST+CaptureOptions.GetPresetCaptureSize());
			Menu.CheckItem(CM_CAPTUREPREVIEW,CaptureWindow.GetVisible());
			Accelerator.SetMenuAccel(Menu.GetPopupHandle());
			Menu.Show(GetMainWindow(),&pt,Flags);
		}
		break;

	case MENU_BUFFERING:
		{
			CPopupMenu Menu(hInst,IDM_BUFFERING);

			Menu.CheckItem(CM_ENABLEBUFFERING,CoreEngine.GetPacketBuffering());
			Menu.EnableItem(CM_RESETBUFFER,CoreEngine.GetPacketBuffering());
			Menu.Show(GetMainWindow(),&pt,Flags);
		}
		break;

	case MENU_STREAMERROR:
		{
			CPopupMenu Menu(hInst,IDM_ERROR);

			Menu.Show(GetMainWindow(),&pt,Flags);
		}
		break;

	case MENU_CLOCK:
		{
			CPopupMenu Menu(hInst,IDM_TIME);

			Menu.CheckItem(CM_SHOWTOTTIME,StatusOptions.GetShowTOTTime());
			Menu.Show(GetMainWindow(),&pt,Flags);
		}
		break;

	case MENU_PROGRAMINFO:
		{
			CPopupMenu Menu(hInst,IDM_PROGRAMINFOSTATUS);

			Menu.CheckItem(CM_PROGRAMINFOSTATUS_POPUPINFO,
						   StatusOptions.IsPopupProgramInfoEnabled());
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
	if (pNetworkRemocon!=NULL) {
		InitNetworkRemoconChannelMenu(hmenu);
		return;
	}
#endif

	const CChannelList *pList=ChannelManager.GetCurrentChannelList();

	ChannelMenu.Destroy();
	ClearMenu(hmenu);
	if (pList==NULL)
		return;

	if (!CoreEngine.IsNetworkDriver()) {
		ChannelMenuManager.CreateChannelMenu(pList,ChannelManager.GetCurrentChannel(),
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
							 | (j!=0 && j%MenuOptions.GetMaxChannelMenuRows()==0?MF_MENUBREAK:0),
							 CM_CHANNEL_FIRST+i,szText);
				j++;
			}
		}
		if (ChannelManager.GetCurrentChannel()>=0
				&& pList->IsEnabled(ChannelManager.GetCurrentChannel())) {
			::CheckMenuRadioItem(hmenu,CM_CHANNEL_FIRST,
				CM_CHANNEL_FIRST+pList->NumChannels()-1,
				CM_CHANNEL_FIRST+ChannelManager.GetCurrentChannel(),
				MF_BYCOMMAND);
		}
	}
}

#ifdef NETWORK_REMOCON_SUPPORT
void CUICore::InitNetworkRemoconChannelMenu(HMENU hmenu)
{
	const CChannelList &RemoconChList=pNetworkRemocon->GetChannelList();
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
		if (ChannelManager.GetNetworkRemoconCurrentChannel()>=0) {
			::CheckMenuRadioItem(hmenu,
				CM_CHANNELNO_FIRST,CM_CHANNELNO_FIRST+Max-1,
				CM_CHANNEL_FIRST+ChannelManager.GetNetworkRemoconCurrentChannel(),
				MF_BYCOMMAND);
		}
	}
	pPortList=ChannelManager.GetDriverChannelList(0);
	for (i=0;i<pPortList->NumChannels();i++) {
		StdUtil::snprintf(szText,lengthof(szText),TEXT("%d: %s"),
						  pPortList->GetChannelNo(i),pPortList->GetName(i));
		AppendMenu(hmenu,MF_STRING | MF_ENABLED
			| ((i!=0 && i%MenuOptions.GetMaxChannelMenuRows()==0) || (i==0 && RemoconChList.NumChannels()>0)?MF_MENUBREAK:0),
			CM_CHANNEL_FIRST+i,szText);
	}
	if (ChannelManager.GetCurrentChannel()>=0) {
		::CheckMenuRadioItem(hmenu,
			CM_CHANNEL_FIRST,CM_CHANNEL_FIRST+pPortList->NumChannels()-1,
			CM_CHANNEL_FIRST+ChannelManager.GetCurrentChannel(),
			MF_BYCOMMAND);
	}
}
#endif	// NETWORK_REMOCON_SUPPORT

void CUICore::InitTunerMenu(HMENU hmenu)
{
	ChannelMenu.Destroy();

	CPopupMenu Menu(hmenu);
	Menu.Clear();

	TCHAR szText[256];
	int Length;
	int i;

	// 各チューニング空間のメニューを追加する
	// 実際のメニューの設定は WM_INITMENUPOPUP で行っている
	if (ChannelManager.NumSpaces()>0) {
		HMENU hmenuSpace;
		LPCTSTR pszName;

		if (ChannelManager.NumSpaces()>1) {
			hmenuSpace=::CreatePopupMenu();
			Menu.Append(hmenuSpace,TEXT("&A: すべて"));
		}
		for (i=0;i<ChannelManager.NumSpaces();i++) {
			const CChannelList *pChannelList=ChannelManager.GetChannelList(i);

			hmenuSpace=::CreatePopupMenu();
			Length=StdUtil::snprintf(szText,lengthof(szText),TEXT("&%d: "),i);
			pszName=ChannelManager.GetTuningSpaceName(i);
			if (pszName!=NULL)
				CopyToMenuText(pszName,szText+Length,lengthof(szText)-Length);
			else
				StdUtil::snprintf(szText+Length,lengthof(szText)-Length,
								  TEXT("チューニング空間%d"),i);
			Menu.Append(hmenuSpace,szText,
						pChannelList->NumEnableChannels()>0?MF_ENABLED:MF_GRAYED);
		}

		Menu.AppendSeparator();
	}

	::LoadString(hInst,CM_CHANNELDISPLAY,szText,lengthof(szText));
	Menu.Append(CM_CHANNELDISPLAY,szText,
				MF_ENABLED | (ChannelDisplay.GetVisible()?MF_CHECKED:MF_UNCHECKED));
	::AppendMenu(hmenu,MF_SEPARATOR,0,NULL);
	int CurDriver=-1;
	for (i=0;i<DriverManager.NumDrivers();i++) {
		const CDriverInfo *pDriverInfo=DriverManager.GetDriverInfo(i);
		::lstrcpyn(szText,pDriverInfo->GetFileName(),lengthof(szText));
		::PathRemoveExtension(szText);
		Menu.AppendUnformatted(CM_DRIVER_FIRST+i,szText);
		if (CoreEngine.IsTunerOpen()
				&& IsEqualFileName(pDriverInfo->GetFileName(),CoreEngine.GetDriverFileName()))
			CurDriver=i;
	}
	if (CurDriver<0 && CoreEngine.IsTunerOpen()) {
		Menu.AppendUnformatted(CM_DRIVER_FIRST+i,CoreEngine.GetDriverFileName());
		CurDriver=i++;
	}
	Menu.Append(CM_DRIVER_BROWSE,TEXT("参照..."));
	if (CurDriver>=0)
		Menu.CheckRadioItem(CM_DRIVER_FIRST,CM_DRIVER_FIRST+i-1,
							CM_DRIVER_FIRST+CurDriver);
	Accelerator.SetMenuAccel(hmenu);
}

bool CUICore::ProcessTunerMenu(int Command)
{
	if (Command<CM_SPACE_CHANNEL_FIRST || Command>CM_SPACE_CHANNEL_LAST)
		return false;

	const CChannelList *pChannelList;
	int CommandBase;
	int i,j;

	CommandBase=CM_SPACE_CHANNEL_FIRST;
	pChannelList=ChannelManager.GetAllChannelList();
	if (pChannelList->NumChannels()>0) {
		if (Command-CommandBase<pChannelList->NumChannels())
			return AppMain.SetChannel(-1,Command-CommandBase);
		CommandBase+=pChannelList->NumChannels();
	}
	for (int i=0;i<ChannelManager.NumSpaces();i++) {
		pChannelList=ChannelManager.GetChannelList(i);
		if (Command-CommandBase<pChannelList->NumChannels())
			return AppMain.SetChannel(i,Command-CommandBase);
		CommandBase+=pChannelList->NumChannels();
	}
	for (i=0;i<DriverManager.NumDrivers();i++) {
		const CDriverInfo *pDriverInfo=DriverManager.GetDriverInfo(i);

		if (IsEqualFileName(pDriverInfo->GetFileName(),CoreEngine.GetDriverFileName()))
			continue;
		if (pDriverInfo->IsTuningSpaceListLoaded()) {
			const CTuningSpaceList *pTuningSpaceList=pDriverInfo->GetAvailableTuningSpaceList();

			for (j=0;j<pTuningSpaceList->NumSpaces();j++) {
				pChannelList=pTuningSpaceList->GetChannelList(j);
				if (Command-CommandBase<pChannelList->NumChannels()) {
					if (!AppMain.OpenTuner(pDriverInfo->GetFileName()))
						return false;
					return AppMain.SetChannel(j,Command-CommandBase);
				}
				CommandBase+=pChannelList->NumChannels();
			}
		}
	}
	return false;
}

bool CUICore::DoCommand(int Command)
{
	if (m_pSkin==NULL || Command<=0 || Command>0xFFFF)
		return false;
	::SendMessage(m_pSkin->GetMainWindow(),WM_COMMAND,MAKEWPARAM(Command,0),0);
	return true;
}

bool CUICore::DoCommand(LPCTSTR pszCommand)
{
	if (pszCommand==NULL)
		return false;
	int Command=CommandList.ParseText(pszCommand);
	if (Command==0)
		return false;
	return DoCommand(Command);
}

bool CUICore::ConfirmStopRecording()
{
	HWND hwnd;

	if (m_pSkin!=NULL)
		hwnd=m_pSkin->GetVideoHostWindow();
	else
		hwnd=NULL;
	return RecordOptions.ConfirmStatusBarStop(hwnd);
}

bool CUICore::UpdateIcon()
{
	HICON hicoBig=NULL,hicoSmall=NULL;

	if (ViewOptions.GetUseLogoIcon() && CoreEngine.IsTunerOpen()) {
		const CChannelInfo *pCurChannel=ChannelManager.GetCurrentChannelInfo();

		if (pCurChannel!=NULL) {
			hicoBig=LogoManager.CreateLogoIcon(
				pCurChannel->GetNetworkID(),
				pCurChannel->GetServiceID(),
				::GetSystemMetrics(SM_CXICON),
				::GetSystemMetrics(SM_CYICON));
			hicoSmall=LogoManager.CreateLogoIcon(
				pCurChannel->GetNetworkID(),
				pCurChannel->GetServiceID(),
				::GetSystemMetrics(SM_CXSMICON),
				::GetSystemMetrics(SM_CYSMICON));
		}
	}
	HWND hwnd=GetMainWindow();
	if (hwnd!=NULL) {
		HICON hicoDefault;

		if (hicoBig==NULL || hicoSmall==NULL)
			hicoDefault=::LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON));
		::SendMessage(hwnd,WM_SETICON,ICON_BIG,
					  reinterpret_cast<LPARAM>(hicoBig!=NULL?hicoBig:hicoDefault));
		::SendMessage(hwnd,WM_SETICON,ICON_SMALL,
					  reinterpret_cast<LPARAM>(hicoSmall!=NULL?hicoSmall:hicoDefault));
	}
	if (m_hicoLogoBig!=NULL)
		::DestroyIcon(m_hicoLogoBig);
	m_hicoLogoBig=hicoBig;
	if (m_hicoLogoSmall!=NULL)
		::DestroyIcon(m_hicoLogoSmall);
	m_hicoLogoSmall=hicoSmall;
	return true;
}

bool CUICore::UpdateTitle()
{
	HWND hwnd=GetMainWindow();

	if (hwnd==NULL)
		return false;

	TCHAR szText[256],szOld[256],szService[MAX_CHANNEL_NAME];
	CStaticStringFormatter Formatter(szText,lengthof(szText));
	LPCTSTR pszText;

	// TODO: ユーザーが "%service-name% / %event-time% %event-name%" のようにフォーマットを指定できるようにする
	if (AppMain.GetCurrentServiceName(szService,lengthof(szService))) {
		Formatter.AppendFormat(
			MAIN_TITLE_TEXT TEXT(" %s %s"),
			RecordManager.IsRecording()?TEXT("●"):TEXT("-"),
			szService);

		if (CoreEngine.IsTunerOpen()) {
			TCHAR szTime[EpgUtil::MAX_EVENT_TIME_LENGTH+1],szEvent[256];

			szTime[0]=_T('\0');
			if (ViewOptions.GetShowTitleEventTime()) {
				SYSTEMTIME StartTime;
				DWORD Duration;

				if (CoreEngine.m_DtvEngine.GetEventTime(&StartTime,&Duration)) {
					if (EpgUtil::FormatEventTime(StartTime,Duration,szTime,lengthof(szTime)-1)>0)
						::lstrcat(szTime,TEXT(" "));
				}
			}
			if (CoreEngine.m_DtvEngine.GetEventName(szEvent,lengthof(szEvent))<1)
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
		ResidentManager.SetTipText(pszText);
	}

	return true;
}

bool CUICore::RegisterModelessDialog(CBasicDialog *pDialog)
{
	if (pDialog==NULL)
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

void CUICore::OnTunerChanged()
{
	if (m_pSkin!=NULL)
		m_pSkin->OnTunerChanged();
	UpdateTitle();
	UpdateIcon();
}

void CUICore::OnTunerOpened()
{
	if (m_pSkin!=NULL)
		m_pSkin->OnTunerOpened();
	UpdateTitle();
	UpdateIcon();
}

void CUICore::OnTunerClosed()
{
	if (m_pSkin!=NULL)
		m_pSkin->OnTunerClosed();
	UpdateTitle();
	UpdateIcon();
}

void CUICore::OnChannelListChanged()
{
	if (m_pSkin!=NULL)
		m_pSkin->OnChannelListChanged();
	UpdateTitle();
	UpdateIcon();
}

void CUICore::OnChannelChanged(unsigned int Status)
{
	if (m_pSkin!=NULL)
		m_pSkin->OnChannelChanged(Status);
	UpdateTitle();
	UpdateIcon();
}

void CUICore::OnServiceChanged()
{
	if (m_pSkin!=NULL)
		m_pSkin->OnServiceChanged();
	UpdateTitle();
	PluginManager.SendServiceChangeEvent();
}

void CUICore::OnRecordingStarted()
{
	if (m_pSkin!=NULL)
		m_pSkin->OnRecordingStarted();
	UpdateTitle();
	PluginManager.SendRecordStatusChangeEvent();
}

void CUICore::OnRecordingStopped()
{
	if (m_pSkin!=NULL)
		m_pSkin->OnRecordingStopped();
	UpdateTitle();
	PluginManager.SendRecordStatusChangeEvent();
}




// 操作パネルのアイテムを設定する
static void InitControlPanel()
{
	ControlPanel.AddItem(new CTunerControlItem);
	ControlPanel.AddItem(new CChannelControlItem);

	const CChannelList *pList=ChannelManager.GetCurrentChannelList();
	for (int i=0;i<12;i++) {
		TCHAR szText[4];
		CControlPanelButton *pItem;

		wsprintf(szText,TEXT("%d"),i+1);
		pItem=new CControlPanelButton(CM_CHANNELNO_FIRST+i,szText,i%6==0,1);
		if (pList==NULL || pList->FindChannelNo(i+1)<0)
			pItem->SetEnable(false);
		ControlPanel.AddItem(pItem);
	}

	ControlPanel.AddItem(new CVideoControlItem);
	ControlPanel.AddItem(new CVolumeControlItem);
	ControlPanel.AddItem(new CAudioControlItem);
}


static void UpdateControlPanelStatus()
{
	const CChannelList *pList=ChannelManager.GetCurrentChannelList();
	const CChannelInfo *pCurChannel=ChannelManager.GetCurrentChannelInfo();

	for (int i=0;i<12;i++) {
		CControlPanelItem *pItem=ControlPanel.GetItem(CONTROLPANEL_ITEM_CHANNEL_1+i);
		if (pItem!=NULL) {
			pItem->SetEnable(pList!=NULL && pList->FindChannelNo(i+1)>=0);
			pItem->SetCheck(false);
		}
	}
	if (pCurChannel!=NULL) {
		if (pCurChannel->GetChannelNo()>=1 && pCurChannel->GetChannelNo()<=12)
			ControlPanel.CheckRadioItem(CM_CHANNELNO_FIRST,CM_CHANNELNO_LAST,
										CM_CHANNELNO_FIRST+pCurChannel->GetChannelNo()-1);
	}
}




/*
	配色を適用する
*/
static bool ColorSchemeApplyProc(const CColorScheme *pColorScheme)
{
	Theme::GradientInfo Gradient1,Gradient2,Gradient3,Gradient4;
	Theme::BorderInfo Border;

	MainWindow.ApplyColorScheme(pColorScheme);

	CStatusView::ThemeInfo StatusTheme;
	pColorScheme->GetStyle(CColorScheme::STYLE_STATUSITEM,
						   &StatusTheme.ItemStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_STATUSBOTTOMITEM,
						   &StatusTheme.BottomItemStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_STATUSHIGHLIGHTITEM,
						   &StatusTheme.HighlightItemStyle);
	pColorScheme->GetBorderInfo(CColorScheme::BORDER_STATUS,
								&StatusTheme.Border);
	StatusView.SetTheme(&StatusTheme);
	CaptureWindow.SetStatusTheme(&StatusTheme);
	pColorScheme->GetBorderInfo(CColorScheme::BORDER_PROGRAMGUIDESTATUS,
								&StatusTheme.Border);
	ProgramGuideFrame.SetStatusTheme(&StatusTheme);
	ProgramGuideDisplay.SetStatusTheme(&StatusTheme);
	CRecordStatusItem *pRecordStatus=dynamic_cast<CRecordStatusItem*>(StatusView.GetItemByID(STATUS_ITEM_RECORD));
	if (pRecordStatus!=NULL)
		pRecordStatus->SetCircleColor(pColorScheme->GetColor(CColorScheme::COLOR_STATUSRECORDINGCIRCLE));

	CSideBar::ThemeInfo SideBarTheme;
	pColorScheme->GetStyle(CColorScheme::STYLE_SIDEBARITEM,
						   &SideBarTheme.ItemStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_SIDEBARHIGHLIGHTITEM,
						   &SideBarTheme.HighlightItemStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_SIDEBARCHECKITEM,
						   &SideBarTheme.CheckItemStyle);
	pColorScheme->GetBorderInfo(CColorScheme::BORDER_SIDEBAR,
								&SideBarTheme.Border);
	SideBar.SetTheme(&SideBarTheme);

	CPanel::ThemeInfo PanelTheme;
	pColorScheme->GetStyle(CColorScheme::STYLE_PANEL_TITLE,
						   &PanelTheme.TitleStyle);
	PanelFrame.SetTheme(&PanelTheme);

	CPanelForm::ThemeInfo PanelFormTheme;
	pColorScheme->GetStyle(CColorScheme::STYLE_PANEL_TAB,
						   &PanelFormTheme.TabStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_PANEL_CURTAB,
						   &PanelFormTheme.CurTabStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_PANEL_TABMARGIN,
						   &PanelFormTheme.TabMarginStyle);
	PanelFormTheme.BackColor=
		pColorScheme->GetColor(CColorScheme::COLOR_PANELBACK);
	PanelFormTheme.BorderColor=
		pColorScheme->GetColor(CColorScheme::COLOR_PANELTABLINE);
	PanelForm.SetTheme(&PanelFormTheme);

	InfoPanel.SetColor(
		pColorScheme->GetColor(CColorScheme::COLOR_PANELBACK),
		pColorScheme->GetColor(CColorScheme::COLOR_PANELTEXT));
	InfoPanel.SetProgramInfoColor(
		pColorScheme->GetColor(CColorScheme::COLOR_PROGRAMINFOBACK),
		pColorScheme->GetColor(CColorScheme::COLOR_PROGRAMINFOTEXT));

	CProgramListPanel::ThemeInfo ProgramListPanelTheme;
	pColorScheme->GetStyle(CColorScheme::STYLE_PROGRAMLISTPANEL_EVENT,
						   &ProgramListPanelTheme.EventTextStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_PROGRAMLISTPANEL_CUREVENT,
						   &ProgramListPanelTheme.CurEventTextStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_PROGRAMLISTPANEL_TITLE,
						   &ProgramListPanelTheme.EventNameStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_PROGRAMLISTPANEL_CURTITLE,
						   &ProgramListPanelTheme.CurEventNameStyle);
	ProgramListPanelTheme.MarginColor=
		pColorScheme->GetColor(CColorScheme::COLOR_PANELBACK);
	ProgramListPanel.SetTheme(&ProgramListPanelTheme);

	CChannelPanel::ThemeInfo ChannelPanelTheme;
	pColorScheme->GetStyle(CColorScheme::STYLE_CHANNELPANEL_CHANNELNAME,
						   &ChannelPanelTheme.ChannelNameStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_CHANNELPANEL_CURCHANNELNAME,
						   &ChannelPanelTheme.CurChannelNameStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_CHANNELPANEL_EVENTNAME1,
						   &ChannelPanelTheme.EventStyle[0]);
	pColorScheme->GetStyle(CColorScheme::STYLE_CHANNELPANEL_EVENTNAME2,
						   &ChannelPanelTheme.EventStyle[1]);
	pColorScheme->GetStyle(CColorScheme::STYLE_CHANNELPANEL_CURCHANNELEVENTNAME1,
						   &ChannelPanelTheme.CurChannelEventStyle[0]);
	pColorScheme->GetStyle(CColorScheme::STYLE_CHANNELPANEL_CURCHANNELEVENTNAME2,
						   &ChannelPanelTheme.CurChannelEventStyle[1]);
	ChannelPanelTheme.MarginColor=pColorScheme->GetColor(CColorScheme::COLOR_PANELBACK);
	ChannelPanel.SetTheme(&ChannelPanelTheme);

	CControlPanel::ThemeInfo ControlPanelTheme;
	pColorScheme->GetStyle(CColorScheme::STYLE_CONTROLPANELITEM,
						   &ControlPanelTheme.ItemStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_CONTROLPANELHIGHLIGHTITEM,
						   &ControlPanelTheme.OverItemStyle);
	ControlPanelTheme.MarginColor=
		pColorScheme->GetColor(CColorScheme::COLOR_CONTROLPANELMARGIN);
	ControlPanel.SetTheme(&ControlPanelTheme);

	CaptionPanel.SetColor(
		pColorScheme->GetColor(CColorScheme::COLOR_CAPTIONPANELBACK),
		pColorScheme->GetColor(CColorScheme::COLOR_CAPTIONPANELTEXT));

	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_NOTIFICATIONBARBACK,&Gradient1);
	NotificationBar.SetColors(
		&Gradient1,
		pColorScheme->GetColor(CColorScheme::COLOR_NOTIFICATIONBARTEXT),
		pColorScheme->GetColor(CColorScheme::COLOR_NOTIFICATIONBARWARNINGTEXT),
		pColorScheme->GetColor(CColorScheme::COLOR_NOTIFICATIONBARERRORTEXT));

	static const struct {
		int From,To;
	} ProgramGuideColorMap[] = {
		{CColorScheme::COLOR_PROGRAMGUIDE_BACK,				CProgramGuide::COLOR_BACK},
		{CColorScheme::COLOR_PROGRAMGUIDE_TEXT,				CProgramGuide::COLOR_EVENTTEXT},
		{CColorScheme::COLOR_PROGRAMGUIDE_EVENTTITLE,		CProgramGuide::COLOR_EVENTTITLE},
		{CColorScheme::COLOR_PROGRAMGUIDE_HIGHLIGHTTEXT,	CProgramGuide::COLOR_HIGHLIGHT_TEXT},
		{CColorScheme::COLOR_PROGRAMGUIDE_HIGHLIGHTTITLE,	CProgramGuide::COLOR_HIGHLIGHT_TITLE},
		{CColorScheme::COLOR_PROGRAMGUIDE_HIGHLIGHTBORDER,	CProgramGuide::COLOR_HIGHLIGHT_BORDER},
		{CColorScheme::COLOR_PROGRAMGUIDE_CHANNELTEXT,		CProgramGuide::COLOR_CHANNELNAMETEXT},
		{CColorScheme::COLOR_PROGRAMGUIDE_CURCHANNELTEXT,	CProgramGuide::COLOR_CURCHANNELNAMETEXT},
		{CColorScheme::COLOR_PROGRAMGUIDE_TIMETEXT,			CProgramGuide::COLOR_TIMETEXT},
		{CColorScheme::COLOR_PROGRAMGUIDE_TIMELINE,			CProgramGuide::COLOR_TIMELINE},
		{CColorScheme::COLOR_PROGRAMGUIDE_CURTIMELINE,		CProgramGuide::COLOR_CURTIMELINE},
	};
	for (int i=0;i<lengthof(ProgramGuideColorMap);i++)
		g_ProgramGuide.SetColor(ProgramGuideColorMap[i].To,
								pColorScheme->GetColor(ProgramGuideColorMap[i].From));
	for (int i=CProgramGuide::COLOR_CONTENT_FIRST,j=0;i<=CProgramGuide::COLOR_CONTENT_LAST;i++,j++)
		g_ProgramGuide.SetColor(i,pColorScheme->GetColor(CColorScheme::COLOR_PROGRAMGUIDE_CONTENT_FIRST+j));
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_PROGRAMGUIDECHANNELBACK,&Gradient1);
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_PROGRAMGUIDECURCHANNELBACK,&Gradient2);
	pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_PROGRAMGUIDETIMEBACK,&Gradient3);
	Theme::GradientInfo TimeGradients[CProgramGuide::TIME_BAR_BACK_COLORS];
	for (int i=0;i<CProgramGuide::TIME_BAR_BACK_COLORS;i++)
		pColorScheme->GetGradientInfo(CColorScheme::GRADIENT_PROGRAMGUIDETIME0TO2BACK+i,&TimeGradients[i]);
	g_ProgramGuide.SetBackColors(&Gradient1,&Gradient2,&Gradient3,TimeGradients);

	PluginManager.SendColorChangeEvent();

	return true;
}


static void ApplyEventInfoFont()
{
	ProgramListPanel.SetEventInfoFont(EpgOptions.GetEventInfoFont());
	ChannelPanel.SetEventInfoFont(EpgOptions.GetEventInfoFont());
	g_ProgramGuide.SetEventInfoFont(EpgOptions.GetEventInfoFont());
	CProgramInfoStatusItem *pProgramInfo=dynamic_cast<CProgramInfoStatusItem*>(StatusView.GetItemByID(STATUS_ITEM_PROGRAMINFO));
	if (pProgramInfo!=NULL)
		pProgramInfo->SetEventInfoFont(EpgOptions.GetEventInfoFont());
}


const COptionDialog::PageInfo COptionDialog::m_PageList[] = {
	{TEXT("一般"),					&GeneralOptions,		HELP_ID_OPTIONS_GENERAL},
	{TEXT("表示"),					&ViewOptions,			HELP_ID_OPTIONS_VIEW},
	{TEXT("OSD"),					&OSDOptions,			HELP_ID_OPTIONS_OSD},
	{TEXT("ステータスバー"),		&StatusOptions,			HELP_ID_OPTIONS_STATUSBAR},
	{TEXT("サイドバー"),			&SideBarOptions,		HELP_ID_OPTIONS_SIDEBAR},
	{TEXT("メニュー"),				&MenuOptions,			HELP_ID_OPTIONS_MENU},
	{TEXT("パネル"),				&PanelOptions,			HELP_ID_OPTIONS_PANEL},
	{TEXT("テーマ/配色"),			&ColorSchemeOptions,	HELP_ID_OPTIONS_COLORSCHEME},
	{TEXT("操作"),					&OperationOptions,		HELP_ID_OPTIONS_OPERATION},
	{TEXT("キー割り当て"),			&Accelerator,			HELP_ID_OPTIONS_ACCELERATOR},
	{TEXT("リモコン"),				&ControllerManager,		HELP_ID_OPTIONS_CONTROLLER},
	{TEXT("BonDriver設定"),			&DriverOptions,			HELP_ID_OPTIONS_DRIVER},
	{TEXT("再生"),					&PlaybackOptions,		HELP_ID_OPTIONS_PLAYBACK},
	{TEXT("録画"),					&RecordOptions,			HELP_ID_OPTIONS_RECORD},
	{TEXT("キャプチャ"),			&CaptureOptions,		HELP_ID_OPTIONS_CAPTURE},
	{TEXT("チャンネルスキャン"),	&ChannelScan,			HELP_ID_OPTIONS_CHANNELSCAN},
	{TEXT("EPG/番組情報"),			&EpgOptions,			HELP_ID_OPTIONS_EPG},
	{TEXT("EPG番組表"),				&ProgramGuideOptions,	HELP_ID_OPTIONS_PROGRAMGUIDE},
	{TEXT("プラグイン"),			&PluginOptions,			HELP_ID_OPTIONS_PLUGIN},
#ifdef NETWORK_REMOCON_SUPPORT
	{TEXT("ネットワークリモコン"),	&NetworkRemoconOptions,	HELP_ID_OPTIONS_NETWORKREMOCON},
#endif
	{TEXT("ログ"),					&Logger,				HELP_ID_OPTIONS_LOG},
};


// 設定ダイアログを表示する
static bool ShowOptionDialog(HWND hwndOwner,int StartPage=-1)
{
	if (!OptionDialog.Show(hwndOwner,StartPage))
		return false;

	if ((COptions::GetGeneralUpdateFlags() & COptions::UPDATE_GENERAL_BUILDMEDIAVIEWER)!=0) {
		CUICore *pUICore=AppMain.GetUICore();

		if (CoreEngine.m_DtvEngine.m_MediaViewer.IsOpen()
				|| pUICore->IsViewerInitializeError()) {
			bool fOldError=pUICore->IsViewerInitializeError();
			CoreEngine.m_DtvEngine.SetTracer(&StatusView);
			bool fResult=pUICore->InitializeViewer();
			CoreEngine.m_DtvEngine.SetTracer(NULL);
			StatusView.SetSingleText(NULL);
			// エラーで再生オフになっていた場合はオンにする
			if (fResult && fOldError && !pUICore->IsViewerEnabled())
				pUICore->EnableViewer(true);
		}
	}

	if ((COptions::GetGeneralUpdateFlags() & COptions::UPDATE_GENERAL_EVENTINFOFONT)!=0) {
		ApplyEventInfoFont();
	}

#ifdef NETWORK_REMOCON_SUPPORT
	if (NetworkRemoconOptions.GetUpdateFlags()!=0) {
		AppMain.InitializeChannel();
		if (pNetworkRemocon!=NULL)
			pNetworkRemocon->GetChannel(&GetChannelReceiver);
	}
#endif

	if ((ProgramGuideOptions.GetUpdateFlags() & CProgramGuideOptions::UPDATE_EVENTICONS)!=0)
		ProgramListPanel.SetVisibleEventIcons(ProgramGuideOptions.GetVisibleEventIcons());
	ResidentManager.SetMinimizeToTray(ViewOptions.GetMinimizeToTray());

	AppMain.SaveSettings(CAppMain::SETTINGS_SAVE_OPTIONS);

	PluginManager.SendSettingsChangeEvent();

	return true;
}




class CMyPanelEventHandler : public CPanelFrame::CEventHandler
{
	POINT m_ptDragStartCursorPos;
	POINT m_ptStartPos;
	enum {
		EDGE_NONE,
		EDGE_LEFT,
		EDGE_RIGHT,
		EDGE_TOP,
		EDGE_BOTTOM
	} m_SnapEdge;
	int m_AttachOffset;

public:
	CMyPanelEventHandler();
// CPanelFrame::CEventHandler
	bool OnClose();
	bool OnMoving(RECT *pRect);
	bool OnEnterSizeMove();
	bool OnKeyDown(UINT KeyCode,UINT Flags);
	bool OnMouseWheel(WPARAM wParam,LPARAM lParam);
	void OnVisibleChange(bool fVisible);
	bool OnFloatingChange(bool fFloating);
// CMyPanelEventHandler
	bool OnOwnerMovingOrSizing(const RECT *pOldRect,const RECT *pNewRect);
	bool IsAttached();
};

CMyPanelEventHandler::CMyPanelEventHandler()
	: m_SnapEdge(EDGE_NONE)
{
}

bool CMyPanelEventHandler::OnClose()
{
	MainWindow.SendCommand(CM_PANEL);
	return false;
}

bool CMyPanelEventHandler::OnMoving(RECT *pRect)
{
	if (!PanelFrame.GetFloating())
		return false;

	POINT pt;
	RECT rc;

	::GetCursorPos(&pt);
	pt.x=m_ptStartPos.x+(pt.x-m_ptDragStartCursorPos.x);
	pt.y=m_ptStartPos.y+(pt.y-m_ptDragStartCursorPos.y);
	::OffsetRect(pRect,pt.x-pRect->left,pt.y-pRect->top);
	if (PanelOptions.GetSnapAtMainWindow()) {
		// メインウィンドウにスナップさせる
		int SnapMargin=PanelOptions.GetSnapMargin();
		int XOffset,YOffset;
		bool fSnap;

		MainWindow.GetPosition(&rc);
		XOffset=YOffset=0;
		fSnap=false;
		if (pRect->top<rc.bottom && pRect->bottom>rc.top) {
			if (pRect->right>=rc.left-SnapMargin && pRect->right<=rc.left+SnapMargin) {
				XOffset=rc.left-pRect->right;
				fSnap=true;
			} else if (pRect->left>=rc.right-SnapMargin && pRect->left<=rc.right+SnapMargin) {
				XOffset=rc.right-pRect->left;
				fSnap=true;
			}
			if (fSnap) {
				if (pRect->top>=rc.top-SnapMargin && pRect->top<=rc.top+SnapMargin) {
					YOffset=rc.top-pRect->top;
				} else if (pRect->bottom>=rc.bottom-SnapMargin && pRect->bottom<=rc.bottom+SnapMargin) {
					YOffset=rc.bottom-pRect->bottom;
				}
			}
		}
		if (!fSnap && pRect->left<rc.right && pRect->right>rc.left) {
			if (pRect->bottom>=rc.top-SnapMargin && pRect->bottom<=rc.top+SnapMargin) {
				YOffset=rc.top-pRect->bottom;
				fSnap=true;
			} else if (pRect->top>=rc.bottom-SnapMargin && pRect->top<=rc.bottom+SnapMargin) {
				YOffset=rc.bottom-pRect->top;
				fSnap=true;
			}
			if (fSnap) {
				if (pRect->left>=rc.left-SnapMargin && pRect->left<=rc.left+SnapMargin) {
					XOffset=rc.left-pRect->left;
				} else if (pRect->right>=rc.right-SnapMargin && pRect->right<=rc.right+SnapMargin) {
					XOffset=rc.right-pRect->right;
				}
			}
		}
		::OffsetRect(pRect,XOffset,YOffset);
	}
	return true;
}

bool CMyPanelEventHandler::OnEnterSizeMove()
{
	if (!PanelFrame.GetFloating())
		return false;

	::GetCursorPos(&m_ptDragStartCursorPos);
	int x,y;
	PanelFrame.GetPosition(&x,&y,NULL,NULL);
	m_ptStartPos.x=x;
	m_ptStartPos.y=y;
	return true;
}

bool CMyPanelEventHandler::OnKeyDown(UINT KeyCode,UINT Flags)
{
	MainWindow.SendMessage(WM_KEYDOWN,KeyCode,Flags);
	return true;
}

bool CMyPanelEventHandler::OnMouseWheel(WPARAM wParam,LPARAM lParam)
{
	SendMessage(MainWindow.GetVideoHostWindow(),WM_MOUSEWHEEL,wParam,lParam);
	return true;
}

void CMyPanelEventHandler::OnVisibleChange(bool fVisible)
{
	if (!PanelFrame.GetFloating())
		return;
	if (!fVisible) {
		m_SnapEdge=EDGE_NONE;
		if (PanelOptions.GetAttachToMainWindow()) {
			RECT rcPanel,rcMain;

			PanelFrame.GetPosition(&rcPanel);
			MainWindow.GetPosition(&rcMain);
			if (rcPanel.top<rcMain.bottom && rcPanel.bottom>rcMain.top) {
				if (rcPanel.right==rcMain.left)
					m_SnapEdge=EDGE_LEFT;
				else if (rcPanel.left==rcMain.right)
					m_SnapEdge=EDGE_RIGHT;
				if (m_SnapEdge!=EDGE_NONE)
					m_AttachOffset=rcPanel.top-rcMain.top;
			}
			if (rcPanel.left<rcMain.right && rcPanel.right>rcMain.left) {
				if (rcPanel.bottom==rcMain.top)
					m_SnapEdge=EDGE_TOP;
				else if (rcPanel.top==rcMain.bottom)
					m_SnapEdge=EDGE_BOTTOM;
				if (m_SnapEdge!=EDGE_NONE)
					m_AttachOffset=rcPanel.left-rcMain.left;
			}
		}
	} else {
		if (m_SnapEdge!=EDGE_NONE) {
			RECT rcPanel,rcMain;
			int x,y;

			PanelFrame.GetPosition(&rcPanel);
			OffsetRect(&rcPanel,-rcPanel.left,-rcPanel.top);
			MainWindow.GetPosition(&rcMain);
			switch (m_SnapEdge) {
			case EDGE_LEFT:
				x=rcMain.left-rcPanel.right;
				y=rcMain.top+m_AttachOffset;
				break;
			case EDGE_RIGHT:
				x=rcMain.right;
				y=rcMain.top+m_AttachOffset;
				break;
			case EDGE_TOP:
				y=rcMain.top-rcPanel.bottom;
				x=rcMain.left+m_AttachOffset;
				break;
			case EDGE_BOTTOM:
				y=rcMain.bottom;
				x=rcMain.left+m_AttachOffset;
				break;
			}
			PanelFrame.SetPosition(x,y,rcPanel.right,rcPanel.bottom);
			PanelFrame.MoveToMonitorInside();
		}
	}
}

bool CMyPanelEventHandler::OnFloatingChange(bool fFloating)
{
	Layout::CSplitter *pSplitter=
		dynamic_cast<Layout::CSplitter*>(MainWindow.GetLayoutBase().GetContainerByID(CONTAINER_ID_PANELSPLITTER));
	int Size;
	RECT rc;

	int PanelPaneIndex=pSplitter->IDToIndex(CONTAINER_ID_PANEL);
	if (fFloating)
		pSplitter->GetPane(PanelPaneIndex)->SetVisible(false);
	MainWindow.GetPosition(&rc);
	Size=PanelFrame.GetDockingWidth()+pSplitter->GetBarWidth();
	if (!fFloating || rc.right-rc.left>Size) {
		if (PanelPaneIndex==0) {
			if (fFloating)
				rc.left+=Size;
			else
				rc.left-=Size;
		} else {
			if (fFloating)
				rc.right-=Size;
			else
				rc.right+=Size;
		}
		MainWindow.SetPosition(&rc);
	}
	return true;
}

bool CMyPanelEventHandler::OnOwnerMovingOrSizing(const RECT *pOldRect,const RECT *pNewRect)
{
	if (fShowPanelWindow && PanelOptions.GetAttachToMainWindow()
			&& PanelFrame.GetFloating()) {
		RECT rc;
		int XOffset,YOffset;
		bool fAttached=false;

		PanelFrame.GetPosition(&rc);
		XOffset=YOffset=0;
		if (rc.top<pOldRect->bottom && rc.bottom>pOldRect->top) {
			if (rc.right==pOldRect->left) {
				XOffset=pNewRect->left-rc.right;
				fAttached=true;
			} else if (rc.left==pOldRect->right) {
				XOffset=pNewRect->right-rc.left;
				fAttached=true;
			}
			if (fAttached)
				YOffset=pNewRect->top-pOldRect->top;
		}
		if (!fAttached && rc.left<pOldRect->right && rc.right>pOldRect->left) {
			if (rc.bottom==pOldRect->top) {
				YOffset=pNewRect->top-rc.bottom;
				fAttached=true;
			} else if (rc.top==pOldRect->bottom) {
				YOffset=pNewRect->bottom-rc.top;
				fAttached=true;
			}
			if (fAttached)
				XOffset=pNewRect->left-pOldRect->left;
		}
		if (XOffset!=0 || YOffset!=0) {
			::OffsetRect(&rc,XOffset,YOffset);
			PanelFrame.SetPosition(&rc);
			PanelFrame.MoveToMonitorInside();
		}
		return true;
	}
	return false;
}

bool CMyPanelEventHandler::IsAttached()
{
	bool fAttached=false;

	if (fShowPanelWindow && PanelOptions.GetAttachToMainWindow()
			&& PanelFrame.GetFloating()) {
		RECT rcPanel,rcMain;

		PanelFrame.GetPosition(&rcPanel);
		MainWindow.GetPosition(&rcMain);
		if (rcPanel.top<rcMain.bottom && rcPanel.bottom>rcMain.top) {
			if (rcPanel.right==rcMain.left || rcPanel.left==rcMain.right)
				fAttached=true;
		}
		if (!fAttached && rcPanel.left<rcMain.right && rcPanel.right>rcMain.left) {
			if (rcPanel.bottom==rcMain.top || rcPanel.top==rcMain.bottom)
				fAttached=true;
		}
	}
	return fAttached;
}


class CMyPanelFormEventHandler : public CPanelForm::CEventHandler
{
	void OnSelChange()
	{
		MainWindow.UpdatePanel();
	}

	void OnRButtonDown()
	{
		AppMain.GetUICore()->PopupMenu();
	}

	void OnTabRButtonDown(int x,int y)
	{
		CPopupMenu Menu;
		Menu.Create();

		int Cur=-1;
		int VisibleCount=0;
		for (int i=0;i<PanelForm.NumPages();i++) {
			CPanelForm::TabInfo TabInfo;

			PanelForm.GetTabInfo(i,&TabInfo);
			if (TabInfo.fVisible) {
				TCHAR szText[64];
				::LoadString(hInst,CM_PANEL_FIRST+TabInfo.ID,szText,lengthof(szText));
				Menu.Append(CM_PANEL_FIRST+TabInfo.ID,szText);
				if (TabInfo.ID==PanelForm.GetCurPageID())
					Cur=VisibleCount;
				VisibleCount++;
			}
		}
		if (VisibleCount>0) {
			if (Cur>=0)
				Menu.CheckRadioItem(0,VisibleCount-1,Cur,MF_BYPOSITION);
			POINT pt={x,y};
			::ClientToScreen(PanelForm.GetHandle(),&pt);
			Menu.Show(MainWindow.GetHandle(),&pt,TPM_RIGHTBUTTON);
		}
	}

	bool OnKeyDown(UINT KeyCode,UINT Flags)
	{
		MainWindow.SendMessage(WM_KEYDOWN,KeyCode,Flags);
		return true;
	}
};


class CMyInformationPanelEventHandler : public CInformationPanel::CEventHandler
{
	bool OnProgramInfoUpdate(bool fNext) {
		MainWindow.UpdateProgramInfo();
		return true;
	}
};


class CMyChannelPanelEventHandler : public CChannelPanel::CEventHandler
{
	void OnChannelClick(const CChannelInfo *pChannelInfo) override
	{
		const CChannelList *pList=ChannelManager.GetCurrentChannelList();

		if (pList!=NULL) {
#ifdef NETWORK_REMOCON_SUPPORT
			if (pNetworkRemocon!=NULL) {
				MainWindow.PostCommand(CM_CHANNELNO_FIRST+pChannelInfo->GetChannelNo()-1);
			} else
#endif
			{
				int Index=pList->Find(pChannelInfo->GetSpace(),
									  pChannelInfo->GetChannelIndex(),
									  pChannelInfo->GetServiceID());
				if (Index<0 && pChannelInfo->GetServiceID()>0)
					Index=pList->Find(pChannelInfo->GetSpace(),
									  pChannelInfo->GetChannelIndex());
				if (Index>=0)
					MainWindow.PostCommand(CM_CHANNEL_FIRST+Index);
			}
		}
	}

	void OnRButtonDown() override
	{
		CPopupMenu Menu(hInst,IDM_CHANNELPANEL);

		Menu.CheckItem(CM_CHANNELPANEL_DETAILPOPUP,ChannelPanel.GetDetailToolTip());
		Menu.CheckItem(CM_CHANNELPANEL_SCROLLTOCURCHANNEL,ChannelPanel.GetScrollToCurChannel());
		Menu.CheckRadioItem(CM_CHANNELPANEL_EVENTS_1,CM_CHANNELPANEL_EVENTS_4,
							CM_CHANNELPANEL_EVENTS_1+ChannelPanel.GetEventsPerChannel()-1);
		Menu.CheckRadioItem(CM_CHANNELPANEL_EXPANDEVENTS_2,CM_CHANNELPANEL_EXPANDEVENTS_8,
							CM_CHANNELPANEL_EXPANDEVENTS_2+ChannelPanel.GetExpandAdditionalEvents()-2);
		Menu.Show(MainWindow.GetHandle());
	}
};


class CMyEpgLoadEventHandler
	: public CEpgOptions::CEpgFileLoadEventHandler
	, public CEpgOptions::CEDCBDataLoadEventHandler
{
// CEpgFileLoadEventHandler
	void OnBeginLoad() override
	{
		TRACE(TEXT("Start EPG file loading ...\n"));
	}

	void OnEndLoad(bool fSuccess) override
	{
		TRACE(TEXT("End EPG file loading : %s\n"),fSuccess?TEXT("Succeeded"):TEXT("Failed"));
		if (fSuccess)
			MainWindow.PostMessage(WM_APP_EPGLOADED,0,0);
	}

// CEDCBDataLoadEventHandler
	void OnStart() override
	{
		TRACE(TEXT("Start EDCB data loading ...\n"));
	}

	void OnEnd(bool fSuccess,CEventManager *pEventManager) override
	{
		TRACE(TEXT("End EDCB data loading : %s\n"),fSuccess?TEXT("Succeeded"):TEXT("Failed"));
		if (fSuccess) {
			CEventManager::ServiceList ServiceList;

			if (pEventManager->GetServiceList(&ServiceList)) {
				for (size_t i=0;i<ServiceList.size();i++) {
					EpgProgramList.UpdateService(pEventManager,&ServiceList[i],
						CEpgProgramList::SERVICE_UPDATE_DATABASE);
				}
				MainWindow.PostMessage(WM_APP_EPGLOADED,0,0);
			}
		}
	}
};


class CMyProgramGuideChannelProviderManager : public CProgramGuideChannelProviderManager
{
public:
	CMyProgramGuideChannelProviderManager();
	~CMyProgramGuideChannelProviderManager();
// CProgramGuideChannelProviderManager
	size_t GetChannelProviderCount() const override;
	CProgramGuideChannelProvider *GetChannelProvider(size_t Index) const override;
// CMyProgramGuideChannelProviderManager
	bool Create(LPCTSTR pszDefaultTuner=NULL);
	void Clear();
	int GetCurChannelProvider() const { return m_CurChannelProvider; }

private:
	class CBonDriverChannelProvider : public CProgramGuideBaseChannelProvider
	{
	public:
		CBonDriverChannelProvider(LPCTSTR pszFileName);
		bool Update() override;
	};

	class CFavoritesChannelProvider : public CProgramGuideBaseChannelProvider
	{
	public:
		bool Update() override;
		bool GetName(LPTSTR pszName,int MaxName) const override;
		bool GetBonDriver(LPTSTR pszFileName,int MaxLength) const override;
		bool GetBonDriverFileName(size_t Group,size_t Channel,LPTSTR pszFileName,int MaxLength) const override;
	private:
		void AddFavoritesChannels(const CFavoriteFolder &Folder);
		std::vector<CFavoriteChannel> m_ChannelList;
	};

	std::vector<CProgramGuideChannelProvider*> m_ChannelProviderList;
	int m_CurChannelProvider;
};

CMyProgramGuideChannelProviderManager::CMyProgramGuideChannelProviderManager()
	: m_CurChannelProvider(-1)
{
}

CMyProgramGuideChannelProviderManager::~CMyProgramGuideChannelProviderManager()
{
	Clear();
}

size_t CMyProgramGuideChannelProviderManager::GetChannelProviderCount() const
{
	return m_ChannelProviderList.size();
}

CProgramGuideChannelProvider *CMyProgramGuideChannelProviderManager::GetChannelProvider(size_t Index) const
{
	if (Index>=m_ChannelProviderList.size())
		return NULL;
	return m_ChannelProviderList[Index];
}

bool CMyProgramGuideChannelProviderManager::Create(LPCTSTR pszDefaultTuner)
{
	Clear();

	m_ChannelProviderList.push_back(new CFavoritesChannelProvider);
	if (pszDefaultTuner!=NULL && ::lstrcmpi(pszDefaultTuner,TEXT("favorites"))==0)
		m_CurChannelProvider=0;

	CProgramGuideBaseChannelProvider *pCurChannelProvider=NULL;
	String DefaultTuner;
	if (m_CurChannelProvider<0) {
		if (!IsStringEmpty(pszDefaultTuner)) {
			pCurChannelProvider=new CBonDriverChannelProvider(pszDefaultTuner);
			DefaultTuner=pszDefaultTuner;
		} else if (pszDefaultTuner==NULL
				&& CoreEngine.IsDriverSpecified()
				&& !CoreEngine.IsNetworkDriver()) {
			DefaultTuner=CoreEngine.GetDriverFileName();
			pCurChannelProvider=new CProgramGuideBaseChannelProvider(
				ChannelManager.GetTuningSpaceList(),
				DefaultTuner.c_str());
		}
	}

	for (int i=0;i<DriverManager.NumDrivers();i++) {
		const CDriverInfo *pDriverInfo=DriverManager.GetDriverInfo(i);

		if (pDriverInfo!=NULL) {
			if (pCurChannelProvider!=NULL
					&& IsEqualFileName(DefaultTuner.c_str(),pDriverInfo->GetFileName())) {
				m_CurChannelProvider=(int)m_ChannelProviderList.size();
				m_ChannelProviderList.push_back(pCurChannelProvider);
				pCurChannelProvider=NULL;
			} else if (!CCoreEngine::IsNetworkDriverFileName(pDriverInfo->GetFileName())) {
				CBonDriverChannelProvider *pDriverChannelProvider=
					new CBonDriverChannelProvider(pDriverInfo->GetFileName());

				m_ChannelProviderList.push_back(pDriverChannelProvider);
			}
		}
	}

	if (pCurChannelProvider!=NULL) {
		auto itr=m_ChannelProviderList.begin();
		++itr;
		m_ChannelProviderList.insert(itr,pCurChannelProvider);
		m_CurChannelProvider=1;
	}

	return true;
}

void CMyProgramGuideChannelProviderManager::Clear()
{
	for (size_t i=0;i<m_ChannelProviderList.size();i++)
		delete m_ChannelProviderList[i];
	m_ChannelProviderList.clear();
	m_CurChannelProvider=-1;
}


CMyProgramGuideChannelProviderManager::CBonDriverChannelProvider::CBonDriverChannelProvider(LPCTSTR pszFileName)
	: CProgramGuideBaseChannelProvider(NULL,pszFileName)
{
}

bool CMyProgramGuideChannelProviderManager::CBonDriverChannelProvider::Update()
{
	CDriverInfo DriverInfo(m_BonDriverFileName.c_str());

	if (!DriverInfo.LoadTuningSpaceList())
		return false;

	m_TuningSpaceList=*DriverInfo.GetTuningSpaceList();

	return true;
}


bool CMyProgramGuideChannelProviderManager::CFavoritesChannelProvider::GetName(LPTSTR pszName,int MaxName) const
{
	if (pszName==NULL || MaxName<1)
		return false;

	::lstrcpyn(pszName,TEXT("お気に入りチャンネル"),MaxName);

	return true;
}

bool CMyProgramGuideChannelProviderManager::CFavoritesChannelProvider::Update()
{
	m_TuningSpaceList.Clear();
	m_TuningSpaceList.Reserve(1);
	m_TuningSpaceList.GetTuningSpaceInfo(0)->SetName(TEXT("お気に入り"));

	m_ChannelList.clear();

	AddFavoritesChannels(FavoritesManager.GetRootFolder());

	return true;
}

void CMyProgramGuideChannelProviderManager::CFavoritesChannelProvider::AddFavoritesChannels(const CFavoriteFolder &Folder)
{
	for (size_t i=0;i<Folder.GetItemCount();i++) {
		const CFavoriteItem *pItem=Folder.GetItem(i);

		if (pItem->GetType()==CFavoriteItem::ITEM_FOLDER) {
			AddFavoritesChannels(*static_cast<const CFavoriteFolder*>(pItem));
		} else if (pItem->GetType()==CFavoriteItem::ITEM_CHANNEL) {
			const CFavoriteChannel *pChannel=static_cast<const CFavoriteChannel*>(pItem);
			m_TuningSpaceList.GetChannelList(0)->AddChannel(pChannel->GetChannelInfo());
			m_ChannelList.push_back(*pChannel);
		}
	}
}

bool CMyProgramGuideChannelProviderManager::CFavoritesChannelProvider::GetBonDriver(LPTSTR pszFileName,int MaxLength) const
{
	if (pszFileName==NULL || MaxLength<1 || m_ChannelList.empty())
		return false;

	LPCTSTR pszBonDriver=m_ChannelList[0].GetBonDriverFileName();
	if (IsStringEmpty(pszBonDriver))
		return false;

	for (size_t i=1;i<m_ChannelList.size();i++) {
		if (!IsEqualFileName(m_ChannelList[i].GetBonDriverFileName(),pszBonDriver))
			return false;
	}

	::lstrcpyn(pszFileName,pszBonDriver,MaxLength);

	return false;
}

bool CMyProgramGuideChannelProviderManager::CFavoritesChannelProvider::GetBonDriverFileName(size_t Group,size_t Channel,LPTSTR pszFileName,int MaxLength) const
{
	if (Group!=0 || Channel>=m_ChannelList.size()
			|| pszFileName==NULL || MaxLength<1)
		return false;

	const CFavoriteChannel &FavoriteChannel=m_ChannelList[Channel];
	const CChannelInfo &ChannelInfo=FavoriteChannel.GetChannelInfo();

	if (!FavoriteChannel.GetForceBonDriverChange()
			&& CoreEngine.IsTunerOpen()) {
		int Space=ChannelManager.GetCurrentSpace();
		if (Space!=CChannelManager::SPACE_INVALID) {
			int Index=ChannelManager.FindChannelByIDs(Space,
				ChannelInfo.GetNetworkID(),
				ChannelInfo.GetTransportStreamID(),
				ChannelInfo.GetServiceID());
			if (Index<0 && Space!=CChannelManager::SPACE_ALL) {
				for (Space=0;Space<ChannelManager.NumSpaces();Space++) {
					Index=ChannelManager.FindChannelByIDs(Space,
						ChannelInfo.GetNetworkID(),
						ChannelInfo.GetTransportStreamID(),
						ChannelInfo.GetServiceID());
					if (Index>=0)
						break;
				}
			}
			if (Index>=0) {
				pszFileName[0]=_T('\0');
				return true;
			}
		}
	}

	::lstrcpyn(pszFileName,FavoriteChannel.GetBonDriverFileName(),MaxLength);

	return true;
}


class CMyProgramGuideEventHandler : public CProgramGuide::CEventHandler
{
	bool OnClose() override
	{
		fShowProgramGuide=false;
		MainMenu.CheckItem(CM_PROGRAMGUIDE,false);
		SideBar.CheckItem(CM_PROGRAMGUIDE,false);
		return true;
	}

	void OnDestroy() override
	{
		m_pProgramGuide->Clear();

		if (CmdLineOptions.m_fProgramGuideOnly
				&& AppMain.GetUICore()->GetStandby()
				&& !RecordManager.IsRecording()
				&& !RecordManager.IsReserved())
			MainWindow.PostCommand(CM_EXIT);
	}

	int FindChannel(const CChannelList *pChannelList,const CServiceInfoData *pServiceInfo)
	{
		for (int i=0;i<pChannelList->NumChannels();i++) {
			const CChannelInfo *pChannelInfo=pChannelList->GetChannelInfo(i);

			if (pChannelInfo->GetTransportStreamID()==pServiceInfo->m_TSID
					&& pChannelInfo->GetServiceID()==pServiceInfo->m_ServiceID)
				return i;
		}
		return -1;
	}

	void OnServiceTitleLButtonDown(LPCTSTR pszDriverFileName,const CServiceInfoData *pServiceInfo) override
	{
		if (RecordManager.IsRecording()) {
			if (!RecordOptions.ConfirmChannelChange(MainWindow.GetVideoHostWindow()))
				return;
		}

		const bool fSetBonDriver=!IsStringEmpty(pszDriverFileName);
		CMainWindow::ResumeInfo &ResumeInfo=MainWindow.GetResumeInfo();
		ResumeInfo.fSetChannel=false;
		ResumeInfo.fOpenTuner=!fSetBonDriver;

		MainWindow.SendCommand(CM_SHOW);

		if (fSetBonDriver) {
			if (!AppMain.OpenTuner(pszDriverFileName))
				return;
		}

		const CChannelList *pChannelList=ChannelManager.GetCurrentChannelList();
		if (pChannelList!=NULL) {
			int Index=FindChannel(pChannelList,pServiceInfo);
			if (Index>=0) {
#ifdef NETWORK_REMOCON_SUPPORT
				if (pNetworkRemocon!=NULL) {
					pNetworkRemocon->SetChannel(pChannelList->GetChannelInfo(Index)->GetChannelNo()-1);
					ChannelManager.SetNetworkRemoconCurrentChannel(Index);
					AppMain.GetUICore()->OnChannelChanged(0);
					PluginManager.SendChannelChangeEvent();
				} else
#endif
				{
					AppMain.SetChannel(ChannelManager.GetCurrentSpace(),Index);
				}
				return;
			}
		}
		for (int i=0;i<ChannelManager.NumSpaces();i++) {
			pChannelList=ChannelManager.GetChannelList(i);
			if (pChannelList!=NULL) {
				int Index=FindChannel(pChannelList,pServiceInfo);
				if (Index>=0) {
					AppMain.SetChannel(i,Index);
					return;
				}
			}
		}
	}

	bool OnBeginUpdate(LPCTSTR pszBonDriver,const CChannelList *pChannelList) override
	{
		if (CmdLineOptions.m_fNoEpg) {
			MainWindow.ShowMessage(TEXT("コマンドラインオプションでEPG情報を取得しないように指定されているため、\n番組表の取得ができません。"),
								   TEXT("お知らせ"),MB_OK | MB_ICONINFORMATION);
			return false;
		}
		if (RecordManager.IsRecording()) {
			MainWindow.ShowMessage(TEXT("録画を停止させてから番組表を取得してください。"),
								   TEXT("お知らせ"),MB_OK | MB_ICONINFORMATION);
			return false;
		}
		if (CoreEngine.IsNetworkDriverFileName(pszBonDriver)) {
			MainWindow.ShowMessage(TEXT("UDP/TCPでは番組表の取得はできません。"),
								   TEXT("お知らせ"),MB_OK | MB_ICONINFORMATION);
			return false;
		}

		return MainWindow.BeginProgramGuideUpdate(pszBonDriver,pChannelList);
	}

	void OnEndUpdate() override
	{
		MainWindow.OnProgramGuideUpdateEnd();
	}

	bool OnKeyDown(UINT KeyCode,UINT Flags) override
	{
		MainWindow.SendMessage(WM_KEYDOWN,KeyCode,Flags);
		return true;
	}

	bool OnMenuInitialize(HMENU hmenu,UINT CommandBase) override
	{
		return PluginManager.SendProgramGuideInitializeMenuEvent(hmenu,&CommandBase);
	}

	bool OnMenuSelected(UINT Command) override
	{
		return PluginManager.SendProgramGuideMenuSelectedEvent(Command);
	}
};


class CMyProgramGuideDisplayEventHandler : public CProgramGuideDisplay::CEventHandler
{
// CProgramGuideDisplay::CEventHandler
	bool OnHide() override
	{
		m_pProgramGuideDisplay->Destroy();
		fShowProgramGuide=false;
		MainMenu.CheckItem(CM_PROGRAMGUIDE,fShowProgramGuide);
		SideBar.CheckItem(CM_PROGRAMGUIDE,fShowProgramGuide);
		return true;
	}

	bool SetAlwaysOnTop(bool fTop) override
	{
		return AppMain.GetUICore()->SetAlwaysOnTop(fTop);
	}

	bool GetAlwaysOnTop() const override
	{
		return AppMain.GetUICore()->GetAlwaysOnTop();
	}

	void OnRButtonDown(int x,int y) override
	{
		RelayMouseMessage(WM_RBUTTONDOWN,x,y);
	}

	void OnLButtonDoubleClick(int x,int y) override
	{
		RelayMouseMessage(WM_LBUTTONDBLCLK,x,y);
	}

// CMyProgramGuideDisplayEventHandler
	void RelayMouseMessage(UINT Message,int x,int y)
	{
		HWND hwndParent=m_pProgramGuideDisplay->GetParent();
		POINT pt={x,y};
		::MapWindowPoints(m_pProgramGuideDisplay->GetHandle(),hwndParent,&pt,1);
		::SendMessage(hwndParent,Message,0,MAKELPARAM(pt.x,pt.y));
	}
};


class CMyProgramGuideProgramCustomizer : public CProgramGuide::CProgramCustomizer
{
	bool Initialize() override
	{
		return PluginManager.SendProgramGuideInitializeEvent(m_pProgramGuide->GetHandle());
	}

	void Finalize() override
	{
		PluginManager.SendProgramGuideFinalizeEvent(m_pProgramGuide->GetHandle());
	}

	bool DrawBackground(const CEventInfoData &Event,HDC hdc,
		const RECT &ItemRect,const RECT &TitleRect,const RECT &ContentRect,
		COLORREF BackgroundColor) override
	{
		return PluginManager.SendProgramGuideProgramDrawBackgroundEvent(
			Event,hdc,ItemRect,TitleRect,ContentRect,BackgroundColor);
	}

	bool InitializeMenu(const CEventInfoData &Event,HMENU hmenu,UINT CommandBase,
						const POINT &CursorPos,const RECT &ItemRect) override
	{
		return PluginManager.SendProgramGuideProgramInitializeMenuEvent(
			Event,hmenu,&CommandBase,CursorPos,ItemRect);
	}

	bool ProcessMenu(const CEventInfoData &Event,UINT Command) override
	{
		return PluginManager.SendProgramGuideProgramMenuSelectedEvent(Event,Command);
	}

	bool OnLButtonDoubleClick(const CEventInfoData &Event,
							  const POINT &CursorPos,const RECT &ItemRect) override
	{
		LPCTSTR pszCommand=ProgramGuideOptions.GetProgramLDoubleClickCommand();
		if (IsStringEmpty(pszCommand))
			return false;
		int Command=ProgramGuideOptions.ParseCommand(pszCommand);
		if (Command>0) {
			m_pProgramGuide->SendMessage(WM_COMMAND,Command,0);
			return true;
		}
		return PluginManager.OnProgramGuideCommand(pszCommand,
			TVTest::PROGRAMGUIDE_COMMAND_ACTION_MOUSE,&Event,&CursorPos,&ItemRect);
	}
};


class CMyCaptureWindowEvent : public CCaptureWindow::CEventHandler
{
	void OnRestoreSettings() override
	{
#if 0
		CSettings Settings;

		if (Settings.Open(AppMain.GetIniFileName(),CSettings::OPEN_READ)
				&& Settings.SetSection(TEXT("Settings"))) {
			int Left,Top,Width,Height;
			m_pCaptureWindow->GetPosition(&Left,&Top,&Width,&Height);
			Settings.Read(TEXT("CapturePreviewLeft"),&Left);
			Settings.Read(TEXT("CapturePreviewTop"),&Top);
			Settings.Read(TEXT("CapturePreviewWidth"),&Width);
			Settings.Read(TEXT("CapturePreviewHeight"),&Height);
			m_pCaptureWindow->SetPosition(Left,Top,Width,Height);
			m_pCaptureWindow->MoveToMonitorInside();

			bool fStatusBar;
			if (Settings.Read(TEXT("CaptureStatusBar"),&fStatusBar))
				m_pCaptureWindow->ShowStatusBar(fStatusBar);
		}
#endif
	}

	bool OnClose() override
	{
		MainMenu.CheckItem(CM_CAPTUREPREVIEW,false);
		SideBar.CheckItem(CM_CAPTUREPREVIEW,false);
		m_pCaptureWindow->ClearImage();
		return true;
	}

	bool OnSave(CCaptureImage *pImage) override
	{
		return CaptureOptions.SaveImage(pImage);
	}

	bool OnKeyDown(UINT KeyCode,UINT Flags) override
	{
		MainWindow.SendMessage(WM_KEYDOWN,KeyCode,Flags);
		return true;
	}
};


class CStreamInfoEventHandler : public CStreamInfo::CEventHandler
{
	void OnRestoreSettings() override
	{
#if 0
		CSettings Settings;

		if (Settings.Open(AppMain.GetIniFileName(),CSettings::OPEN_READ)
				&& Settings.SetSection(TEXT("Settings"))) {
			int Left,Top,Width,Height;

			StreamInfo.GetPosition(&Left,&Top,&Width,&Height);
			Settings.Read(TEXT("StreamInfoLeft"),&Left);
			Settings.Read(TEXT("StreamInfoTop"),&Top);
			Settings.Read(TEXT("StreamInfoWidth"),&Width);
			Settings.Read(TEXT("StreamInfoHeight"),&Height);
			StreamInfo.SetPosition(Left,Top,Width,Height);
			//StreamInfo.MoveToMonitorInside();
		}
#endif
	}

	bool OnClose() override
	{
		MainMenu.CheckItem(CM_STREAMINFO,false);
		SideBar.CheckItem(CM_STREAMINFO,false);
		return true;
	}
};


static CMyPanelEventHandler PanelEventHandler;
static CMyPanelFormEventHandler PanelFormEventHandler;
static CMyInformationPanelEventHandler InformationPanelEventHandler;
static CMyChannelPanelEventHandler ChannelPanelEventHandler;
static CMyEpgLoadEventHandler EpgLoadEventHandler;

static CMyProgramGuideEventHandler ProgramGuideEventHandler;
static CMyProgramGuideDisplayEventHandler ProgramGuideDisplayEventHandler;
static CMyProgramGuideProgramCustomizer ProgramGuideProgramCustomizer;
static CMyProgramGuideChannelProviderManager ProgramGuideChannelProviderManager;

static CMyCaptureWindowEvent CaptureWindowEventHandler;
static CStreamInfoEventHandler StreamInfoEventHandler;




class CBarLayout
{
public:
	CBarLayout() {}
	virtual ~CBarLayout() {}
	virtual void Layout(RECT *pArea,RECT *pBarRect)=0;

	bool IsSpot(const RECT *pArea,const POINT *pPos)
	{
		RECT rcArea=*pArea,rcBar;

		Layout(&rcArea,&rcBar);
		return ::PtInRect(&rcBar,*pPos)!=FALSE;
	}

	void AdjustArea(RECT *pArea)
	{
		RECT rcBar;
		Layout(pArea,&rcBar);
	}

	void ReserveArea(RECT *pArea,bool fNoMove)
	{
		RECT rc;

		rc=*pArea;
		AdjustArea(&rc);
		if (fNoMove) {
			pArea->right+=(pArea->right-pArea->left)-(rc.right-rc.left);
			pArea->bottom+=(pArea->bottom-pArea->top)-(rc.bottom-rc.top);
		} else {
			pArea->left-=rc.left-pArea->left;
			pArea->top-=rc.top-pArea->top;
			pArea->right+=pArea->right-rc.right;
			pArea->bottom+=pArea->bottom-rc.bottom;
		}
	}
};


class CMyStatusViewEventHandler : public CStatusView::CEventHandler
{
	void OnMouseLeave() override
	{
		if (!AppMain.GetUICore()->GetFullscreen())
			MainWindow.OnBarMouseLeave(m_pStatusView->GetHandle());
	}

	void OnHeightChanged(int Height) override
	{
		Layout::CWindowContainer *pContainer=
			dynamic_cast<Layout::CWindowContainer*>(MainWindow.GetLayoutBase().GetContainerByID(CONTAINER_ID_STATUS));
		Layout::CSplitter *pSplitter=
			dynamic_cast<Layout::CSplitter*>(MainWindow.GetLayoutBase().GetContainerByID(CONTAINER_ID_STATUSSPLITTER));

		if (pContainer!=NULL)
			pContainer->SetMinSize(0,Height);
		if (pSplitter!=NULL)
			pSplitter->SetPaneSize(CONTAINER_ID_STATUS,Height);
	}
};


class CTitleBarUtil : public CTitleBar::CEventHandler, public CBarLayout
{
	bool m_fMainWindow;
	bool m_fFixed;
	void ShowSystemMenu(int x,int y);

public:
	CTitleBarUtil(bool fMainWindow);
// CTitleBar::CEventHandler
	bool OnClose() override;
	bool OnMinimize() override;
	bool OnMaximize() override;
	bool OnFullscreen() override;
	void OnMouseLeave() override;
	void OnLabelLButtonDown(int x,int y) override;
	void OnLabelLButtonDoubleClick(int x,int y) override;
	void OnLabelRButtonDown(int x,int y) override;
	void OnIconLButtonDown(int x,int y) override;
	void OnIconLButtonDoubleClick(int x,int y) override;
// CBarLayout
	void Layout(RECT *pArea,RECT *pBarRect) override;
// CTitleBarUtil
	void EndDrag();
};

CTitleBarUtil::CTitleBarUtil(bool fMainWindow)
	: m_fMainWindow(fMainWindow)
	, m_fFixed(false)
{
}

bool CTitleBarUtil::OnClose()
{
	MainWindow.PostCommand(CM_CLOSE);
	return true;
}

bool CTitleBarUtil::OnMinimize()
{
	MainWindow.SendMessage(WM_SYSCOMMAND,SC_MINIMIZE,0);
	return true;
}

bool CTitleBarUtil::OnMaximize()
{
	MainWindow.SendMessage(WM_SYSCOMMAND,
						   MainWindow.GetMaximize()?SC_RESTORE:SC_MAXIMIZE,0);
	return true;
}

bool CTitleBarUtil::OnFullscreen()
{
	AppMain.GetUICore()->ToggleFullscreen();
	return true;
}

void CTitleBarUtil::OnMouseLeave()
{
	if (m_fMainWindow && !m_fFixed)
		MainWindow.OnBarMouseLeave(m_pTitleBar->GetHandle());
}

void CTitleBarUtil::OnLabelLButtonDown(int x,int y)
{
	if (m_fMainWindow) {
		POINT pt;

		pt.x=x;
		pt.y=y;
		::ClientToScreen(m_pTitleBar->GetHandle(),&pt);
		MainWindow.SendMessage(WM_NCLBUTTONDOWN,HTCAPTION,MAKELPARAM(pt.x,pt.y));
		m_fFixed=true;
	}
}

void CTitleBarUtil::OnLabelLButtonDoubleClick(int x,int y)
{
	if (m_fMainWindow)
		OnMaximize();
	else
		OnFullscreen();
}

void CTitleBarUtil::OnLabelRButtonDown(int x,int y)
{
	POINT pt;

	pt.x=x;
	pt.y=y;
	::ClientToScreen(m_pTitleBar->GetHandle(),&pt);
	ShowSystemMenu(pt.x,pt.y);
}

void CTitleBarUtil::OnIconLButtonDown(int x,int y)
{
	RECT rc;

	m_pTitleBar->GetScreenPosition(&rc);
	ShowSystemMenu(rc.left,rc.bottom);
}

void CTitleBarUtil::OnIconLButtonDoubleClick(int x,int y)
{
	MainWindow.PostCommand(CM_CLOSE);
}

void CTitleBarUtil::Layout(RECT *pArea,RECT *pBarRect)
{
	pBarRect->left=pArea->left;
	pBarRect->top=pArea->top;
	pBarRect->right=pArea->right;
	pBarRect->bottom=pArea->top+m_pTitleBar->GetHeight();
	pArea->top+=m_pTitleBar->GetHeight();
}

void CTitleBarUtil::EndDrag()
{
	m_fFixed=false;
}

void CTitleBarUtil::ShowSystemMenu(int x,int y)
{
	m_fFixed=true;
	MainWindow.SendMessage(0x0313,0,MAKELPARAM(x,y));
	m_fFixed=false;

	RECT rc;
	POINT pt;
	m_pTitleBar->GetScreenPosition(&rc);
	::GetCursorPos(&pt);
	if (!::PtInRect(&rc,pt))
		OnMouseLeave();
}


class CSideBarUtil : public CSideBar::CEventHandler, public CBarLayout
{
public:
	CSideBarUtil()
		: m_fFixed(false)
	{
	}

// CBarLayout
	void Layout(RECT *pArea,RECT *pBarRect) override
	{
		const int BarWidth=SideBar.GetBarWidth();

		switch (SideBarOptions.GetPlace()) {
		case CSideBarOptions::PLACE_LEFT:
			pBarRect->left=pArea->left;
			pBarRect->top=pArea->top;
			pBarRect->right=pBarRect->left+BarWidth;
			pBarRect->bottom=pArea->bottom;
			pArea->left+=BarWidth;
			break;
		case CSideBarOptions::PLACE_RIGHT:
			pBarRect->left=pArea->right-BarWidth;
			pBarRect->top=pArea->top;
			pBarRect->right=pArea->right;
			pBarRect->bottom=pArea->bottom;
			pArea->right-=BarWidth;
			break;
		case CSideBarOptions::PLACE_TOP:
			pBarRect->left=pArea->left;
			pBarRect->top=pArea->top;
			pBarRect->right=pArea->right;
			pBarRect->bottom=pArea->top+BarWidth;
			pArea->top+=BarWidth;
			break;
		case CSideBarOptions::PLACE_BOTTOM:
			pBarRect->left=pArea->left;
			pBarRect->top=pArea->bottom-BarWidth;
			pBarRect->right=pArea->right;
			pBarRect->bottom=pArea->bottom;
			pArea->bottom-=BarWidth;
			break;
		}
	}

private:
	bool m_fFixed;

	static const CChannelInfo *GetChannelInfoByCommand(int Command)
	{
		const CChannelList *pList=ChannelManager.GetCurrentChannelList();
		if (pList!=NULL) {
			int No=Command-CM_CHANNELNO_FIRST;
			int Index;

			if (pList->HasRemoteControlKeyID()) {
				Index=pList->FindChannelNo(No+1);
				if (Index<0)
					return NULL;
			} else {
				Index=No;
			}
			return pList->GetChannelInfo(Index);
		}
		return NULL;
	}

// CSideBar::CEventHandler
	void OnCommand(int Command) override
	{
		MainWindow.SendCommand(Command);
	}

	void OnRButtonDown(int x,int y) override
	{
		CPopupMenu Menu(hInst,IDM_SIDEBAR);
		POINT pt;

		Menu.CheckItem(CM_SIDEBAR,MainWindow.GetSideBarVisible());
		Menu.EnableItem(CM_SIDEBAR,!AppMain.GetUICore()->GetFullscreen());
		Menu.CheckRadioItem(CM_SIDEBAR_PLACE_FIRST,CM_SIDEBAR_PLACE_LAST,
							CM_SIDEBAR_PLACE_FIRST+(int)SideBarOptions.GetPlace());
		pt.x=x;
		pt.y=y;
		::ClientToScreen(m_pSideBar->GetHandle(),&pt);
		m_fFixed=true;
		Menu.Show(MainWindow.GetHandle(),&pt);
		m_fFixed=false;

		RECT rc;
		m_pSideBar->GetScreenPosition(&rc);
		::GetCursorPos(&pt);
		if (!::PtInRect(&rc,pt))
			OnMouseLeave();
	}

	void OnMouseLeave() override
	{
		if (!m_fFixed && !AppMain.GetUICore()->GetFullscreen())
			MainWindow.OnBarMouseLeave(m_pSideBar->GetHandle());
	}

	bool GetTooltipText(int Command,LPTSTR pszText,int MaxText) override
	{
		if (Command>=CM_CHANNELNO_FIRST && Command<=CM_CHANNELNO_LAST
#ifdef NETWORK_REMOCON_SUPPORT
				&& pNetworkRemocon==NULL)
#endif
		{
			const CChannelInfo *pChInfo=GetChannelInfoByCommand(Command);
			if (pChInfo!=NULL) {
				StdUtil::snprintf(pszText,MaxText,TEXT("%d: %s"),
								  (Command-CM_CHANNELNO_FIRST)+1,pChInfo->GetName());
				return true;
			}
		}
		return false;
	}

	bool DrawIcon(int Command,HDC hdc,const RECT &ItemRect,COLORREF ForeColor,HDC hdcBuffer) override
	{
		if (Command>=CM_CHANNELNO_FIRST && Command<=CM_CHANNELNO_LAST
				&& SideBarOptions.GetShowChannelLogo()
#ifdef NETWORK_REMOCON_SUPPORT
				&& pNetworkRemocon==NULL)
#endif
		{
			// アイコンに局ロゴを表示
			// TODO: 新しくロゴが取得された時に再描画する
			const CChannelInfo *pChannel=GetChannelInfoByCommand(Command);
			if (pChannel!=NULL) {
				HBITMAP hbmLogo=LogoManager.GetAssociatedLogoBitmap(
					pChannel->GetNetworkID(),pChannel->GetServiceID(),
					CLogoManager::LOGOTYPE_SMALL);
				if (hbmLogo!=NULL) {
					const int Width=ItemRect.right-ItemRect.left;
					const int Height=ItemRect.bottom-ItemRect.top;
					const int IconHeight=Height*11/16;	// 本来の比率より縦長にしている(見栄えのため)
					HBITMAP hbmOld=SelectBitmap(hdcBuffer,hbmLogo);
					int OldStretchMode=::SetStretchBltMode(hdc,STRETCH_HALFTONE);
					BITMAP bm;
					::GetObject(hbmLogo,sizeof(bm),&bm);
					::StretchBlt(hdc,ItemRect.left,ItemRect.top+(Height-IconHeight)/2,Width,IconHeight,
								 hdcBuffer,0,0,bm.bmWidth,bm.bmHeight,SRCCOPY);
					::SetStretchBltMode(hdc,OldStretchMode);
					::SelectObject(hdcBuffer,hbmOld);
					return true;
				}
			}
		}
		return false;
	}
};


class CSideBarOptionsEventHandler : public CSideBarOptions::CEventHandler
{
// CSideBarOptions::CEventHandler
	void OnItemChanged()
	{
		const CUICore *pUICore=AppMain.GetUICore();

		SideBar.CheckRadioItem(CM_ASPECTRATIO_FIRST,CM_ASPECTRATIO_LAST,
							   CM_ASPECTRATIO_FIRST+MainWindow.GetAspectRatioType());
		SideBar.CheckItem(CM_FULLSCREEN,pUICore->GetFullscreen());
		SideBar.CheckItem(CM_ALWAYSONTOP,pUICore->GetAlwaysOnTop());
		SideBar.CheckItem(CM_DISABLEVIEWER,!pUICore->IsViewerEnabled());
		SideBar.CheckItem(CM_CAPTUREPREVIEW,CaptureWindow.GetVisible());
		SideBar.CheckItem(CM_PANEL,pUICore->GetFullscreen()?MainWindow.IsFullscreenPanelVisible():fShowPanelWindow);
		SideBar.CheckItem(CM_PROGRAMGUIDE,fShowProgramGuide);
		SideBar.CheckItem(CM_STATUSBAR,MainWindow.GetStatusBarVisible());
		SideBar.CheckItem(CM_STREAMINFO,StreamInfo.IsVisible());
		//SideBar.CheckItem(CM_HOMEDISPLAY,HomeDisplay.GetVisible());
		//SideBar.CheckItem(CM_CHANNELDISPLAY,ChannelDisplay.GetVisible());
		const CChannelInfo *pCurChannel=ChannelManager.GetCurrentChannelInfo();
		int ChannelNo;
		if (pCurChannel!=NULL)
			ChannelNo=pCurChannel->GetChannelNo();
		SideBar.CheckRadioItem(CM_CHANNELNO_1,CM_CHANNELNO_12,
							   pCurChannel!=NULL && ChannelNo>=1 && ChannelNo<=12?
							   CM_CHANNELNO_1+ChannelNo-1:0);
	}
};


static CMyStatusViewEventHandler StatusViewEventHandler;
static CTitleBarUtil TitleBarUtil(true);
static CTitleBarUtil FullscreenTitleBarUtil(false);
static CSideBarUtil SideBarUtil;
static CSideBarOptionsEventHandler SideBarOptionsEventHandler;


class CMyVideoContainerEventHandler : public CVideoContainerWindow::CEventHandler
{
	void OnSizeChanged(int Width,int Height)
	{
		if (NotificationBar.GetVisible()) {
			RECT rc,rcView;

			NotificationBar.GetPosition(&rc);
			::GetClientRect(NotificationBar.GetParent(),&rcView);
			rc.left=rcView.left;
			rc.right=rcView.right;
			NotificationBar.SetPosition(&rc);
		}
		OSDManager.HideVolumeOSD();
	}
};

class CMyViewWindowEventHandler : public CViewWindow::CEventHandler
{
	CTitleBar *m_pTitleBar;
	CStatusView *m_pStatusView;
	CSideBar *m_pSideBar;

	void OnSizeChanged(int Width,int Height)
	{
		// 一時的に表示されているバーのサイズを合わせる
		RECT rcView,rc;

		m_pView->GetPosition(&rcView);
		if (!MainWindow.GetTitleBarVisible() && m_pTitleBar->GetVisible()) {
			TitleBarUtil.Layout(&rcView,&rc);
			m_pTitleBar->SetPosition(&rc);
		}
		if (!MainWindow.GetStatusBarVisible() && m_pStatusView->GetVisible()
				&& m_pStatusView->GetParent()==m_pView->GetParent()) {
			rc=rcView;
			rc.top=rc.bottom-m_pStatusView->GetHeight();
			rcView.bottom-=m_pStatusView->GetHeight();
			m_pStatusView->SetPosition(&rc);
		}
		if (!MainWindow.GetSideBarVisible() && m_pSideBar->GetVisible()) {
			SideBarUtil.Layout(&rcView,&rc);
			m_pSideBar->SetPosition(&rc);
		}
	}

public:
	void Initialize(CTitleBar *pTitleBar,CStatusView *pStatusView,CSideBar *pSideBar)
	{
		m_pTitleBar=pTitleBar;
		m_pStatusView=pStatusView;
		m_pSideBar=pSideBar;
	}
};


class CMyHomeDisplayEventHandler : public CHomeDisplay::CEventHandler
{
	void OnClose() override
	{
		HomeDisplay.SetVisible(false);
	}

	void OnRButtonDown(int x,int y) override
	{
		RelayMouseMessage(WM_RBUTTONDOWN,x,y);
	}

	void OnLButtonDoubleClick(int x,int y) override
	{
		RelayMouseMessage(WM_LBUTTONDBLCLK,x,y);
	}

	void RelayMouseMessage(UINT Message,int x,int y)
	{
		HWND hwndParent=m_pHomeDisplay->GetParent();
		POINT pt={x,y};
		::MapWindowPoints(m_pHomeDisplay->GetHandle(),hwndParent,&pt,1);
		::SendMessage(hwndParent,Message,0,MAKELPARAM(pt.x,pt.y));
	}
};


class CMyChannelDisplayEventHandler : public CChannelDisplay::CEventHandler
{
	void OnTunerSelect(LPCTSTR pszDriverFileName,int TuningSpace)
	{
		if (CoreEngine.IsTunerOpen()
				&& IsEqualFileName(CoreEngine.GetDriverFileName(),pszDriverFileName)) {
			MainWindow.SendCommand(CM_CHANNELDISPLAY);
		} else {
			if (RecordManager.IsRecording()) {
				if (!RecordOptions.ConfirmChannelChange(MainWindow.GetVideoHostWindow()))
					return;
			}
			if (AppMain.OpenTuner(pszDriverFileName)) {
				if (TuningSpace!=SPACE_NOTSPECIFIED) {
					MainWindow.SendCommand(CM_SPACE_FIRST+TuningSpace);
					if (TuningSpace==SPACE_ALL
							|| TuningSpace==RestoreChannelInfo.Space)
						AppMain.RestoreChannel();
				} else {
					AppMain.RestoreChannel();
				}
			}
			MainWindow.SendCommand(CM_CHANNELDISPLAY);
		}
	}

	void OnChannelSelect(LPCTSTR pszDriverFileName,const CChannelInfo *pChannelInfo)
	{
		if (RecordManager.IsRecording()) {
			if (!RecordOptions.ConfirmChannelChange(MainWindow.GetVideoHostWindow()))
				return;
		}
		if (AppMain.OpenTuner(pszDriverFileName)) {
			int Space;
			if (RestoreChannelInfo.fAllChannels)
				Space=CChannelManager::SPACE_ALL;
			else
				Space=pChannelInfo->GetSpace();
			const CChannelList *pList=ChannelManager.GetChannelList(Space);
			if (pList!=NULL) {
				int Index=pList->Find(pChannelInfo->GetSpace(),
									  pChannelInfo->GetChannelIndex(),
									  pChannelInfo->GetServiceID());

				if (Index<0 && Space==CChannelManager::SPACE_ALL) {
					Space=pChannelInfo->GetSpace();
					pList=ChannelManager.GetChannelList(Space);
					if (pList!=NULL)
						Index=pList->Find(-1,pChannelInfo->GetChannelIndex(),
										  pChannelInfo->GetServiceID());
				}
				if (Index>=0)
					AppMain.SetChannel(Space,Index);
			}
			MainWindow.SendCommand(CM_CHANNELDISPLAY);
		}
	}

	void OnClose()
	{
		ChannelDisplay.SetVisible(false);
	}

	void OnRButtonDown(int x,int y)
	{
		RelayMouseMessage(WM_RBUTTONDOWN,x,y);
	}

	void OnLButtonDoubleClick(int x,int y)
	{
		RelayMouseMessage(WM_LBUTTONDBLCLK,x,y);
	}

	void RelayMouseMessage(UINT Message,int x,int y)
	{
		HWND hwndParent=m_pChannelDisplay->GetParent();
		POINT pt={x,y};
		::MapWindowPoints(m_pChannelDisplay->GetHandle(),hwndParent,&pt,1);
		::SendMessage(hwndParent,Message,0,MAKELPARAM(pt.x,pt.y));
	}
};


static CMyVideoContainerEventHandler VideoContainerEventHandler;
static CMyViewWindowEventHandler ViewWindowEventHandler;

static CMyHomeDisplayEventHandler HomeDisplayEventHandler;
static CMyChannelDisplayEventHandler ChannelDisplayEventHandler;




CBasicViewer::CBasicViewer(CDtvEngine *pDtvEngine)
	: m_pDtvEngine(pDtvEngine)
	, m_fEnabled(false)
{
}


bool CBasicViewer::Create(HWND hwndParent,int ViewID,int ContainerID,HWND hwndMessage)
{
	m_ViewWindow.Create(hwndParent,
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,0,ViewID);
	m_ViewWindow.SetMessageWindow(hwndMessage);
	const CColorScheme *pColorScheme=ColorSchemeOptions.GetColorScheme();
	Theme::BorderInfo Border;
	pColorScheme->GetBorderInfo(CColorScheme::BORDER_SCREEN,&Border);
	if (!MainWindow.GetViewWindowEdge())
		Border.Type=Theme::BORDER_NONE;
	m_ViewWindow.SetBorder(&Border);
	m_VideoContainer.Create(m_ViewWindow.GetHandle(),
		WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,0,ContainerID,m_pDtvEngine);
	m_ViewWindow.SetVideoContainer(&m_VideoContainer);

	m_DisplayBase.SetParent(&m_VideoContainer);
	m_VideoContainer.SetDisplayBase(&m_DisplayBase);

	return true;
}


bool CBasicViewer::EnableViewer(bool fEnable)
{
	if (m_fEnabled!=fEnable) {
		if (fEnable && !m_pDtvEngine->m_MediaViewer.IsOpen()) {
			bool fOK=BuildViewer();
			if (!fOK)
				return false;
		}
		if (fEnable || (!fEnable && !m_DisplayBase.IsVisible()))
			m_VideoContainer.SetVisible(fEnable);
		m_pDtvEngine->m_MediaViewer.SetVisible(fEnable);
		if (!CoreEngine.EnablePreview(fEnable))
			return false;
		if (PlaybackOptions.GetMinTimerResolution())
			CoreEngine.SetMinTimerResolution(fEnable);
		m_fEnabled=fEnable;
		PluginManager.SendPreviewChangeEvent(fEnable);
	}
	return true;
}


bool CBasicViewer::BuildViewer()
{
	if (m_fEnabled)
		EnableViewer(false);

	m_pDtvEngine->m_MediaViewer.SetAudioFilter(PlaybackOptions.GetAudioFilterName());
	bool fOK=CoreEngine.BuildMediaViewer(m_VideoContainer.GetHandle(),
										 m_VideoContainer.GetHandle(),
										 GeneralOptions.GetVideoRendererType(),
										 GeneralOptions.GetMpeg2DecoderName(),
										 PlaybackOptions.GetAudioDeviceName());
	if (!fOK) {
		AppMain.OnError(&CoreEngine,TEXT("DirectShowの初期化ができません。"));
	}

	return fOK;
}


bool CBasicViewer::CloseViewer()
{
	EnableViewer(false);
	CoreEngine.CloseMediaViewer();
	return true;
}




bool CFullscreen::Initialize()
{
	WNDCLASS wc;

	wc.style=CS_DBLCLKS;
	wc.lpfnWndProc=WndProc;
	wc.cbClsExtra=0;
	wc.cbWndExtra=0;
	wc.hInstance=hInst;
	wc.hIcon=NULL;
	wc.hCursor=NULL;
	wc.hbrBackground=::CreateSolidBrush(RGB(0,0,0));
	wc.lpszMenuName=NULL;
	wc.lpszClassName=FULLSCREEN_WINDOW_CLASS;
	return RegisterClass(&wc)!=0;
}


CFullscreen::CFullscreen()
	: m_pViewer(NULL)
	, m_PanelWidth(-1)
{
}


CFullscreen::~CFullscreen()
{
	Destroy();
}


LRESULT CFullscreen::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		return OnCreate()?0:-1;

	case WM_SIZE:
		m_LayoutBase.SetPosition(0,0,LOWORD(lParam),HIWORD(lParam));
		return 0;

	case WM_RBUTTONDOWN:
		OnRButtonDown();
		return 0;

	case WM_MBUTTONDOWN:
		OnMButtonDown();
		return 0;

	case WM_LBUTTONDBLCLK:
		OnLButtonDoubleClick();
		return 0;

	case WM_MOUSEMOVE:
		OnMouseMove();
		return 0;

	case WM_TIMER:
		if (wParam==TIMER_ID_HIDECURSOR) {
			if (!m_fMenu) {
				POINT pt;
				RECT rc;
				::GetCursorPos(&pt);
				m_ViewWindow.GetScreenPosition(&rc);
				if (::PtInRect(&rc,pt)) {
					ShowCursor(false);
					::SetCursor(NULL);
				}
			}
			::KillTimer(hwnd,TIMER_ID_HIDECURSOR);
		}
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT) {
			HWND hwndCursor=reinterpret_cast<HWND>(wParam);

			if (hwndCursor==hwnd
					|| hwndCursor==m_pViewer->GetVideoContainer().GetHandle()
					|| hwndCursor==m_ViewWindow.GetHandle()
					|| CPseudoOSD::IsPseudoOSD(hwndCursor)) {
				::SetCursor(m_fShowCursor?::LoadCursor(NULL,IDC_ARROW):NULL);
				return TRUE;
			}
		}
		break;

	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		{
			bool fHorz=uMsg==WM_MOUSEHWHEEL;

			MainWindow.OnMouseWheel(wParam,lParam,fHorz);
			return fHorz;
		}

#if 0	// これはやらない方がいい気がする
	case WM_WINDOWPOSCHANGING:
		{
			WINDOWPOS *pwp=reinterpret_cast<WINDOWPOS*>(lParam);

			pwp->hwndInsertAfter=HWND_TOPMOST;
		}
		return 0;
#endif

	case WM_SYSKEYDOWN:
		if (wParam!=VK_F10)
			break;
	case WM_KEYDOWN:
		if (wParam==VK_ESCAPE) {
			AppMain.GetUICore()->SetFullscreen(false);
			return 0;
		}
	case WM_COMMAND:
		return MainWindow.SendMessage(uMsg,wParam,lParam);

	case WM_SYSCOMMAND:
		switch (wParam&0xFFFFFFF0) {
		case SC_MONITORPOWER:
			if (ViewOptions.GetNoMonitorLowPower()
					&& AppMain.GetUICore()->IsViewerEnabled())
				return 0;
			break;

		case SC_SCREENSAVE:
			if (ViewOptions.GetNoScreenSaver()
					&& AppMain.GetUICore()->IsViewerEnabled())
				return 0;
			break;
		}
		break;

	case WM_APPCOMMAND:
		{
			int Command=Accelerator.TranslateAppCommand(wParam,lParam);

			if (Command!=0) {
				MainWindow.SendCommand(Command);
				return TRUE;
			}
		}
		break;

	case WM_SETFOCUS:
		if (m_pViewer->GetDisplayBase().IsVisible())
			m_pViewer->GetDisplayBase().SetFocus();
		return 0;

	case WM_SETTEXT:
		m_TitleBar.SetLabel(reinterpret_cast<LPCTSTR>(lParam));
		break;

	case WM_SETICON:
		if (wParam==ICON_SMALL)
			m_TitleBar.SetIcon(reinterpret_cast<HICON>(lParam));
		break;

	case WM_DESTROY:
		m_pViewer->GetVideoContainer().SetParent(&m_pViewer->GetViewWindow());
		m_pViewer->GetViewWindow().SendSizeMessage();
		ShowCursor(true);
		m_pViewer->GetDisplayBase().AdjustPosition();
		m_TitleBar.Destroy();
		OSDManager.Reset();
		ShowStatusView(false);
		ShowSideBar(false);
		ShowPanel(false);
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


bool CFullscreen::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 FULLSCREEN_WINDOW_CLASS,NULL,hInst);
}


bool CFullscreen::Create(HWND hwndOwner,CBasicViewer *pViewer)
{
	HMONITOR hMonitor;
	int x,y,Width,Height;

	hMonitor=::MonitorFromWindow(MainWindow.GetHandle(),MONITOR_DEFAULTTONEAREST);
	if (hMonitor!=NULL) {
		MONITORINFO mi;

		mi.cbSize=sizeof(MONITORINFO);
		::GetMonitorInfo(hMonitor,&mi);
		x=mi.rcMonitor.left;
		y=mi.rcMonitor.top;
		Width=mi.rcMonitor.right-mi.rcMonitor.left;
		Height=mi.rcMonitor.bottom-mi.rcMonitor.top;
	} else {
		x=y=0;
		Width=::GetSystemMetrics(SM_CXSCREEN);
		Height=::GetSystemMetrics(SM_CYSCREEN);
	}
#ifdef _DEBUG
	// デバッグし易いように小さく表示
	/*
	Width/=2;
	Height/=2;
	*/
#endif
	SetPosition(x,y,Width,Height);
	m_pViewer=pViewer;
	return Create(hwndOwner,WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN,WS_EX_TOPMOST);
}


bool CFullscreen::OnCreate()
{
	m_LayoutBase.Create(m_hwnd,WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

	m_ViewWindow.Create(m_LayoutBase.GetHandle(),
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN  | WS_CLIPSIBLINGS,0,IDC_VIEW);
	m_ViewWindow.SetMessageWindow(m_hwnd);
	m_pViewer->GetVideoContainer().SetParent(m_ViewWindow.GetHandle());
	m_ViewWindow.SetVideoContainer(&m_pViewer->GetVideoContainer());

	m_Panel.Create(m_LayoutBase.GetHandle(),
				   WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	m_Panel.ShowTitle(true);
	m_Panel.EnableFloating(false);
	m_Panel.SetEventHandler(&m_PanelEventHandler);
	CPanel::ThemeInfo PanelTheme;
	PanelFrame.GetTheme(&PanelTheme);
	m_Panel.SetTheme(&PanelTheme);

	Layout::CSplitter *pSplitter=new Layout::CSplitter(CONTAINER_ID_PANELSPLITTER);
	pSplitter->SetVisible(true);
	int PanelPaneIndex=MainWindow.GetPanelPaneIndex();
	Layout::CWindowContainer *pViewContainer=new Layout::CWindowContainer(CONTAINER_ID_VIEW);
	pViewContainer->SetWindow(&m_ViewWindow);
	pViewContainer->SetMinSize(32,32);
	pViewContainer->SetVisible(true);
	pSplitter->SetPane(1-PanelPaneIndex,pViewContainer);
	Layout::CWindowContainer *pPanelContainer=new Layout::CWindowContainer(CONTAINER_ID_PANEL);
	pPanelContainer->SetWindow(&m_Panel);
	pPanelContainer->SetMinSize(64,32);
	pSplitter->SetPane(PanelPaneIndex,pPanelContainer);
	pSplitter->SetAdjustPane(CONTAINER_ID_VIEW);
	m_LayoutBase.SetTopContainer(pSplitter);

	RECT rc;
	m_pViewer->GetDisplayBase().GetParent()->GetClientRect(&rc);
	m_pViewer->GetDisplayBase().SetPosition(&rc);

	m_TitleBar.Create(m_ViewWindow.GetHandle(),
					  WS_CHILD | WS_CLIPSIBLINGS,0,IDC_TITLEBAR);
	m_TitleBar.SetEventHandler(&FullscreenTitleBarUtil);
	HICON hico=reinterpret_cast<HICON>(MainWindow.SendMessage(WM_GETICON,ICON_SMALL,0));
	if (hico==NULL)
		hico=::LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON));
	m_TitleBar.SetIcon(hico);
	m_TitleBar.SetMaximizeMode(MainWindow.GetMaximize());
	m_TitleBar.SetFullscreenMode(true);

	OSDManager.Reset();

	CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(
									ViewOptions.GetFullscreenStretchMode());

	m_fShowCursor=true;
	m_fMenu=false;
	m_fShowStatusView=false;
	m_fShowTitleBar=false;
	m_fShowSideBar=false;
	m_fShowPanel=false;

	m_LastCursorMovePos.x=LONG_MAX/2;
	m_LastCursorMovePos.y=LONG_MAX/2;
	::SetTimer(m_hwnd,TIMER_ID_HIDECURSOR,HIDE_CURSOR_DELAY,NULL);

	return true;
}


void CFullscreen::ShowCursor(bool fShow)
{
	CoreEngine.m_DtvEngine.m_MediaViewer.HideCursor(!fShow);
	m_ViewWindow.ShowCursor(fShow);
	m_fShowCursor=fShow;
}


void CFullscreen::ShowPanel(bool fShow)
{
	if (m_fShowPanel!=fShow) {
		Layout::CSplitter *pSplitter=
			dynamic_cast<Layout::CSplitter*>(m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));

		if (fShow) {
			if (m_Panel.GetWindow()==NULL) {
				if (m_PanelWidth<0)
					m_PanelWidth=PanelFrame.GetDockingWidth();
				PanelFrame.SetPanelVisible(false);
				PanelFrame.GetPanel()->SetWindow(NULL,NULL);
				m_Panel.SetWindow(&PanelForm,TEXT("パネル"));
				pSplitter->SetPaneSize(CONTAINER_ID_PANEL,m_PanelWidth);
			}
			m_Panel.SendSizeMessage();
			m_LayoutBase.SetContainerVisible(CONTAINER_ID_PANEL,true);
			MainWindow.UpdatePanel();
		} else {
			m_PanelWidth=m_Panel.GetWidth();
			m_LayoutBase.SetContainerVisible(CONTAINER_ID_PANEL,false);
			m_Panel.SetWindow(NULL,NULL);
			CPanel *pPanel=PanelFrame.GetPanel();
			pPanel->SetWindow(&PanelForm,TEXT("パネル"));
			pPanel->SendSizeMessage();
			if (fShowPanelWindow) {
				PanelFrame.SetPanelVisible(true);
			}
		}
		m_fShowPanel=fShow;
	}
}


bool CFullscreen::SetPanelWidth(int Width)
{
	m_PanelWidth=Width;
	return true;
}


void CFullscreen::OnMouseCommand(int Command)
{
	if (Command==0)
		return;
	// メニュー表示中はカーソルを消さない
	KillTimer(m_hwnd,TIMER_ID_HIDECURSOR);
	ShowCursor(true);
	::SetCursor(LoadCursor(NULL,IDC_ARROW));
	m_fMenu=true;
	MainWindow.SendMessage(WM_COMMAND,MAKEWPARAM(Command,CMainWindow::COMMAND_FROM_MOUSE),0);
	m_fMenu=false;
	if (m_hwnd!=NULL)
		::SetTimer(m_hwnd,TIMER_ID_HIDECURSOR,HIDE_CURSOR_DELAY,NULL);
}


void CFullscreen::OnRButtonDown()
{
	OnMouseCommand(OperationOptions.GetRightClickCommand());
}


void CFullscreen::OnMButtonDown()
{
	OnMouseCommand(OperationOptions.GetMiddleClickCommand());
}


void CFullscreen::OnLButtonDoubleClick()
{
	OnMouseCommand(OperationOptions.GetLeftDoubleClickCommand());
}


void CFullscreen::OnMouseMove()
{
	if (m_fMenu)
		return;

	POINT pt;
	RECT rcClient,rcStatus,rcTitle,rc;
	bool fShowStatusView=false,fShowTitleBar=false,fShowSideBar=false;

	::GetCursorPos(&pt);
	::ScreenToClient(m_hwnd,&pt);
	m_ViewWindow.GetClientRect(&rcClient);

	rcStatus=rcClient;
	rcStatus.top=rcStatus.bottom-StatusView.CalcHeight(rcClient.right-rcClient.left);
	if (::PtInRect(&rcStatus,pt))
		fShowStatusView=true;
	rc=rcClient;
	FullscreenTitleBarUtil.Layout(&rc,&rcTitle);
	if (::PtInRect(&rcTitle,pt))
		fShowTitleBar=true;

	if (SideBarOptions.ShowPopup()) {
		RECT rcSideBar;
		switch (SideBarOptions.GetPlace()) {
		case CSideBarOptions::PLACE_LEFT:
		case CSideBarOptions::PLACE_RIGHT:
			if (!fShowStatusView && !fShowTitleBar) {
				SideBarUtil.Layout(&rcClient,&rcSideBar);
				if (::PtInRect(&rcSideBar,pt))
					fShowSideBar=true;
			}
			break;
		case CSideBarOptions::PLACE_TOP:
			rcClient.top=rcTitle.bottom;
			SideBarUtil.Layout(&rcClient,&rcSideBar);
			if (::PtInRect(&rcSideBar,pt)) {
				fShowSideBar=true;
				fShowTitleBar=true;
			}
			break;
		case CSideBarOptions::PLACE_BOTTOM:
			rcClient.bottom=rcStatus.top;
			SideBarUtil.Layout(&rcClient,&rcSideBar);
			if (::PtInRect(&rcSideBar,pt)) {
				fShowSideBar=true;
				fShowStatusView=true;
			}
			break;
		}
	}

	ShowStatusView(fShowStatusView);
	ShowTitleBar(fShowTitleBar);
	ShowSideBar(fShowSideBar);

	if (fShowStatusView || fShowTitleBar || fShowSideBar) {
		::KillTimer(m_hwnd,TIMER_ID_HIDECURSOR);
		return;
	}

	if (abs(m_LastCursorMovePos.x-pt.x)>=4 || abs(m_LastCursorMovePos.y-pt.y)>=4) {
		m_LastCursorMovePos=pt;
		if (!m_fShowCursor) {
			::SetCursor(::LoadCursor(NULL,IDC_ARROW));
			ShowCursor(true);
		}
	}

	::SetTimer(m_hwnd,TIMER_ID_HIDECURSOR,HIDE_CURSOR_DELAY,NULL);
}


void CFullscreen::ShowStatusView(bool fShow)
{
	if (fShow==m_fShowStatusView)
		return;

	Layout::CLayoutBase &LayoutBase=MainWindow.GetLayoutBase();

	if (fShow) {
		RECT rc;

		ShowSideBar(false);
		m_ViewWindow.GetClientRect(&rc);
		rc.top=rc.bottom-StatusView.CalcHeight(rc.right-rc.left);
		StatusView.SetVisible(false);
		LayoutBase.SetContainerVisible(CONTAINER_ID_STATUS,false);
		StatusView.SetParent(&m_ViewWindow);
		StatusView.SetPosition(&rc);
		StatusView.SetVisible(true);
		::BringWindowToTop(StatusView.GetHandle());
	} else {
		StatusView.SetVisible(false);
		StatusView.SetParent(&LayoutBase);
		if (MainWindow.GetStatusBarVisible()) {
			/*
			LayoutBase.Adjust();
			StatusView.SetVisible(true);
			*/
			LayoutBase.SetContainerVisible(CONTAINER_ID_STATUS,true);
		}
	}

	m_fShowStatusView=fShow;
}


void CFullscreen::ShowTitleBar(bool fShow)
{
	if (fShow==m_fShowTitleBar)
		return;

	if (fShow) {
		RECT rc,rcBar;
		const CColorScheme *pColorScheme=ColorSchemeOptions.GetColorScheme();
		Theme::GradientInfo Gradient1,Gradient2;
		Theme::BorderInfo Border;

		ShowSideBar(false);
		m_ViewWindow.GetClientRect(&rc);
		FullscreenTitleBarUtil.Layout(&rc,&rcBar);
		m_TitleBar.SetPosition(&rcBar);
		m_TitleBar.SetLabel(MainWindow.GetTitleBar().GetLabel());
		m_TitleBar.SetMaximizeMode(MainWindow.GetMaximize());
		CTitleBar::ThemeInfo TitleBarTheme;
		MainWindow.GetTitleBar().GetTheme(&TitleBarTheme);
		m_TitleBar.SetTheme(&TitleBarTheme);
		m_TitleBar.SetVisible(true);
		::BringWindowToTop(m_TitleBar.GetHandle());
	} else {
		m_TitleBar.SetVisible(false);
	}

	m_fShowTitleBar=fShow;
}


void CFullscreen::ShowSideBar(bool fShow)
{
	if (fShow==m_fShowSideBar)
		return;

	Layout::CLayoutBase &LayoutBase=MainWindow.GetLayoutBase();

	if (fShow) {
		RECT rcClient,rcBar;

		m_ViewWindow.GetClientRect(&rcClient);
		if (m_fShowStatusView)
			rcClient.bottom-=StatusView.GetHeight();
		if (m_fShowTitleBar)
			rcClient.top+=m_TitleBar.GetHeight();
		SideBarUtil.Layout(&rcClient,&rcBar);
		SideBar.SetVisible(false);
		LayoutBase.SetContainerVisible(CONTAINER_ID_SIDEBAR,false);
		SideBar.SetParent(&m_ViewWindow);
		SideBar.SetPosition(&rcBar);
		SideBar.SetVisible(true);
		::BringWindowToTop(SideBar.GetHandle());
	} else {
		SideBar.SetVisible(false);
		SideBar.SetParent(&LayoutBase);
		if (MainWindow.GetSideBarVisible()) {
			/*
			LayoutBase.Adjust();
			SideBar.SetVisible(true);
			*/
			LayoutBase.SetContainerVisible(CONTAINER_ID_SIDEBAR,true);
		}
	}

	m_fShowSideBar=fShow;
}


bool CFullscreen::CPanelEventHandler::OnClose()
{
	MainWindow.SendCommand(CM_PANEL);
	return true;
}




class CServiceUpdateInfo
{
public:
	struct ServiceInfo {
		WORD ServiceID;
		TCHAR szServiceName[256];
		WORD LogoID;
	};
	ServiceInfo *m_pServiceList;
	int m_NumServices;
	int m_CurService;
	WORD m_NetworkID;
	WORD m_TransportStreamID;
	bool m_fStreamChanged;
	bool m_fServiceListEmpty;
	CServiceUpdateInfo(CDtvEngine *pEngine,CTsAnalyzer *pTsAnalyzer);
	~CServiceUpdateInfo();
};

CServiceUpdateInfo::CServiceUpdateInfo(CDtvEngine *pEngine,CTsAnalyzer *pTsAnalyzer)
{
	CTsAnalyzer::ServiceList ServiceList;

	pTsAnalyzer->GetViewableServiceList(&ServiceList);
	m_NumServices=(int)ServiceList.size();
	m_CurService=-1;
	if (m_NumServices>0) {
		m_pServiceList=new ServiceInfo[m_NumServices];
		for (int i=0;i<m_NumServices;i++) {
			const CTsAnalyzer::ServiceInfo *pServiceInfo=&ServiceList[i];
			m_pServiceList[i].ServiceID=pServiceInfo->ServiceID;
			::lstrcpy(m_pServiceList[i].szServiceName,pServiceInfo->szServiceName);
			m_pServiceList[i].LogoID=pServiceInfo->LogoID;
		}
		WORD ServiceID;
		if (pEngine->GetServiceID(&ServiceID)) {
			for (int i=0;i<m_NumServices;i++) {
				if (m_pServiceList[i].ServiceID==ServiceID) {
					m_CurService=i;
					break;
				}
			}
		}
	} else {
		m_pServiceList=NULL;
	}
	m_NetworkID=pTsAnalyzer->GetNetworkID();
	m_TransportStreamID=pTsAnalyzer->GetTransportStreamID();
	m_fServiceListEmpty=pTsAnalyzer->GetServiceNum()==0;
}

CServiceUpdateInfo::~CServiceUpdateInfo()
{
	delete [] m_pServiceList;
}


class CMyDtvEngineHandler : public CDtvEngine::CEventHandler
{
// CEventHandler
	void OnServiceListUpdated(CTsAnalyzer *pTsAnalyzer,bool bStreamChanged) override;
	void OnServiceInfoUpdated(CTsAnalyzer *pTsAnalyzer) override;
	void OnServiceChanged(WORD ServiceID) override;
	void OnFileWriteError(CBufferedFileWriter *pFileWriter) override;
	void OnVideoSizeChanged(CMediaViewer *pMediaViewer) override;
	void OnEmmProcessed() override;
	void OnEmmError(LPCTSTR pszText) override;
	void OnEcmError(LPCTSTR pszText) override;
	void OnEcmRefused() override;
	void OnCardReaderHung() override;
// CMyDtvEngineHandler
	void OnServiceUpdated(CTsAnalyzer *pTsAnalyzer,bool fListUpdated,bool fStreamChanged);
};

void CMyDtvEngineHandler::OnServiceUpdated(CTsAnalyzer *pTsAnalyzer,bool fListUpdated,bool fStreamChanged)
{
	CServiceUpdateInfo *pInfo=new CServiceUpdateInfo(m_pDtvEngine,pTsAnalyzer);

	pInfo->m_fStreamChanged=fStreamChanged;
	MainWindow.PostMessage(WM_APP_SERVICEUPDATE,fListUpdated,
						   reinterpret_cast<LPARAM>(pInfo));
}

void CMyDtvEngineHandler::OnServiceListUpdated(CTsAnalyzer *pTsAnalyzer,bool bStreamChanged)
{
	OnServiceUpdated(pTsAnalyzer,true,bStreamChanged);
}

void CMyDtvEngineHandler::OnServiceInfoUpdated(CTsAnalyzer *pTsAnalyzer)
{
	OnServiceUpdated(pTsAnalyzer,false,false);
}

void CMyDtvEngineHandler::OnServiceChanged(WORD ServiceID)
{
	MainWindow.PostMessage(WM_APP_SERVICECHANGED,ServiceID,0);
}

void CMyDtvEngineHandler::OnFileWriteError(CBufferedFileWriter *pFileWriter)
{
	MainWindow.PostMessage(WM_APP_FILEWRITEERROR,0,0);
}

void CMyDtvEngineHandler::OnVideoSizeChanged(CMediaViewer *pMediaViewer)
{
	/*
		この通知が送られた段階ではまだレンダラの映像サイズは変わっていないため、
		後でパンスキャンの設定を行う必要がある
	*/
	MainWindow.PostMessage(WM_APP_VIDEOSIZECHANGED,0,0);
}

void CMyDtvEngineHandler::OnEmmProcessed()
{
	MainWindow.PostMessage(WM_APP_EMMPROCESSED,true,0);
}

void CMyDtvEngineHandler::OnEmmError(LPCTSTR pszText)
{
	MainWindow.PostMessage(WM_APP_EMMPROCESSED,false,0);
}

void CMyDtvEngineHandler::OnEcmError(LPCTSTR pszText)
{
	MainWindow.PostMessage(WM_APP_ECMERROR,0,(LPARAM)DuplicateString(pszText));
}

void CMyDtvEngineHandler::OnEcmRefused()
{
	MainWindow.PostMessage(WM_APP_ECMREFUSED,0,0);
}

void CMyDtvEngineHandler::OnCardReaderHung()
{
	MainWindow.PostMessage(WM_APP_CARDREADERHUNG,0,0);
}


static CMyDtvEngineHandler DtvEngineHandler;




struct ControllerFocusInfo {
	HWND hwnd;
	bool fSkipSelf;
	bool fFocus;
};

static BOOL CALLBACK ControllerFocusCallback(HWND hwnd,LPARAM Param)
{
	ControllerFocusInfo *pInfo=reinterpret_cast<ControllerFocusInfo*>(Param);

	if (!pInfo->fSkipSelf || hwnd!=pInfo->hwnd) {
		::PostMessage(hwnd,WM_APP_CONTROLLERFOCUS,pInfo->fFocus,0);
		pInfo->fFocus=false;
	}
	return TRUE;
}

static bool BroadcastControllerFocusMessage(HWND hwnd,bool fSkipSelf,bool fFocus,
											DWORD ActiveThreadID=0)
{
	ControllerFocusInfo Info;

	Info.hwnd=hwnd;
	Info.fSkipSelf=fSkipSelf;
	Info.fFocus=fFocus;
	return EnumTVTestWindows(ControllerFocusCallback,
							 reinterpret_cast<LPARAM>(&Info));
}




ATOM CMainWindow::m_atomChildOldWndProcProp=0;


bool CMainWindow::Initialize()
{
	WNDCLASS wc;

	wc.style=0;
	wc.lpfnWndProc=WndProc;
	wc.cbClsExtra=0;
	wc.cbWndExtra=0;
	wc.hInstance=hInst;
	wc.hIcon=::LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON));
	wc.hCursor=::LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground=(HBRUSH)(COLOR_3DFACE+1);
	wc.lpszMenuName=NULL;
	wc.lpszClassName=MAIN_WINDOW_CLASS;
	return ::RegisterClass(&wc)!=0 && CFullscreen::Initialize();
}


CMainWindow::CMainWindow()
	: m_Viewer(&CoreEngine.m_DtvEngine)

	, m_fShowStatusBar(true)
	, m_fPopupStatusBar(true)
	, m_fShowTitleBar(true)
	, m_fCustomTitleBar(true)
	, m_fPopupTitleBar(true)
	, m_fSplitTitleBar(true)
	, m_fShowSideBar(false)
	, m_PanelPaneIndex(1)
	, m_fCustomFrame(false)
	, m_CustomFrameWidth(0)
	, m_ThinFrameWidth(1)
	, m_fViewWindowEdge(true)

	, m_fStandbyInit(false)
	, m_fMinimizeInit(false)

	, m_fProgramGuideUpdating(false)
	, m_fEpgUpdateChannelChange(false)

	, m_fExitOnRecordingStop(false)
	, m_fClosing(false)

	, m_WheelCount(0)
	, m_PrevWheelMode(COperationOptions::WHEEL_MODE_NONE)
	, m_PrevWheelTime(0)

	, m_AspectRatioType(ASPECTRATIO_DEFAULT)
	, m_AspectRatioResetTime(0)
	, m_fForceResetPanAndScan(false)
	, m_DefaultAspectRatioMenuItemCount(0)
	, m_fFrameCut(false)
	, m_ProgramListUpdateTimerCount(0)
	, m_CurEventStereoMode(-1)
	, m_fAlertedLowFreeSpace(false)
	, m_ResetErrorCountTimer(TIMER_ID_RESETERRORCOUNT)
	, m_ChannelNoInputTimeout(3000)
	, m_ChannelNoInputTimer(TIMER_ID_CHANNELNO)
	, m_DisplayBaseEventHandler(this)
{
	// 適当にデフォルトサイズを設定
#ifndef TVH264_FOR_1SEG
	m_WindowPosition.Width=960;
	m_WindowPosition.Height=540;
#else
	m_WindowPosition.Width=400;
	m_WindowPosition.Height=320;
#endif
	m_WindowPosition.Left=
		(::GetSystemMetrics(SM_CXSCREEN)-m_WindowPosition.Width)/2;
	m_WindowPosition.Top=
		(::GetSystemMetrics(SM_CYSCREEN)-m_WindowPosition.Height)/2;
}


CMainWindow::~CMainWindow()
{
	Destroy();
	if (m_atomChildOldWndProcProp!=0) {
		::GlobalDeleteAtom(m_atomChildOldWndProcProp);
		m_atomChildOldWndProcProp=0;
	}
}


bool CMainWindow::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	if (m_pCore->GetAlwaysOnTop())
		ExStyle|=WS_EX_TOPMOST;
	if (!CreateBasicWindow(NULL,Style,ExStyle,ID,MAIN_WINDOW_CLASS,MAIN_TITLE_TEXT,hInst))
		return false;
	return true;
}


bool CMainWindow::Show(int CmdShow)
{
	return ::ShowWindow(m_hwnd,m_WindowPosition.fMaximized?SW_SHOWMAXIMIZED:CmdShow)!=FALSE;
}


void CMainWindow::CreatePanel()
{
	PanelForm.SetEventHandler(&PanelFormEventHandler);
	PanelForm.Create(m_hwnd,WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	PanelForm.SetTabFont(PanelOptions.GetFont());

	InfoPanel.SetEventHandler(&InformationPanelEventHandler);
	InfoPanel.SetProgramInfoRichEdit(PanelOptions.GetProgramInfoUseRichEdit());
	InfoPanel.Create(PanelForm.GetHandle(),WS_CHILD | WS_CLIPCHILDREN);
	InfoPanel.ShowSignalLevel(!DriverOptions.IsNoSignalLevel(CoreEngine.GetDriverFileName()));
	PanelForm.AddWindow(&InfoPanel,PANEL_ID_INFORMATION,TEXT("情報"));

	ProgramListPanel.SetEpgProgramList(&EpgProgramList);
	ProgramListPanel.SetVisibleEventIcons(ProgramGuideOptions.GetVisibleEventIcons());
	ProgramListPanel.Create(PanelForm.GetHandle(),WS_CHILD | WS_VSCROLL);
	PanelForm.AddWindow(&ProgramListPanel,PANEL_ID_PROGRAMLIST,TEXT("番組表"));

	ChannelPanel.SetEpgProgramList(&EpgProgramList);
	ChannelPanel.SetEventHandler(&ChannelPanelEventHandler);
	ChannelPanel.SetLogoManager(&LogoManager);
	ChannelPanel.Create(PanelForm.GetHandle(),WS_CHILD | WS_VSCROLL);
	PanelForm.AddWindow(&ChannelPanel,PANEL_ID_CHANNEL,TEXT("チャンネル"));

	ControlPanel.SetSendMessageWindow(m_hwnd);
	InitControlPanel();
	ControlPanel.Create(PanelForm.GetHandle(),WS_CHILD);
	PanelForm.AddWindow(&ControlPanel,PANEL_ID_CONTROL,TEXT("操作"));

	CaptionPanel.Create(PanelForm.GetHandle(),WS_CHILD | WS_CLIPCHILDREN);
	PanelForm.AddWindow(&CaptionPanel,PANEL_ID_CAPTION,TEXT("字幕"));

	PanelOptions.InitializePanelForm(&PanelForm);
	PanelFrame.Create(m_hwnd,
					  dynamic_cast<Layout::CSplitter*>(m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER)),
					  CONTAINER_ID_PANEL,&PanelForm,TEXT("パネル"));
	PanelFrame.SetEventHandler(&PanelEventHandler);

	if (m_fCustomFrame) {
		HookWindows(PanelFrame.GetPanel()->GetHandle());
	}
}


bool CMainWindow::InitializeViewer()
{
	const bool fEnableViewer=IsViewerEnabled();

	if (m_Viewer.BuildViewer()) {
		TCHAR szText[256];

		if (CoreEngine.m_DtvEngine.GetVideoDecoderName(szText,lengthof(szText)))
			InfoPanel.SetVideoDecoderName(szText);
		if (CoreEngine.m_DtvEngine.m_MediaViewer.GetVideoRendererName(szText,lengthof(szText)))
			InfoPanel.SetVideoRendererName(szText);
		if (CoreEngine.m_DtvEngine.m_MediaViewer.GetAudioRendererName(szText,lengthof(szText)))
			InfoPanel.SetAudioDeviceName(szText);
		if (fEnableViewer)
			m_pCore->EnableViewer(true);
		if (m_fCustomFrame)
			HookWindows(m_Viewer.GetVideoContainer().GetHandle());
	} else {
		InfoPanel.SetVideoDecoderName(NULL);
		InfoPanel.SetVideoRendererName(NULL);
		InfoPanel.SetAudioDeviceName(NULL);
		MainMenu.CheckItem(CM_DISABLEVIEWER,true);
		SideBar.CheckItem(CM_DISABLEVIEWER,true);
	}

	return true;
}


bool CMainWindow::FinalizeViewer()
{
	m_Viewer.CloseViewer();
	MainMenu.CheckItem(CM_DISABLEVIEWER,true);
	SideBar.CheckItem(CM_DISABLEVIEWER,true);
	return true;
}


bool CMainWindow::OnFullscreenChange(bool fFullscreen)
{
	if (fFullscreen) {
		if (::IsIconic(m_hwnd))
			::ShowWindow(m_hwnd,SW_RESTORE);
		if (!m_Fullscreen.Create(m_hwnd,&m_Viewer))
			return false;
	} else {
		ForegroundWindow(m_hwnd);
		m_Fullscreen.Destroy();
		CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(
							m_fFrameCut?CMediaViewer::STRETCH_CUTFRAME:
										CMediaViewer::STRETCH_KEEPASPECTRATIO);
	}
	StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
	ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VIDEO);
	MainMenu.CheckItem(CM_FULLSCREEN,fFullscreen);
	SideBar.CheckItem(CM_FULLSCREEN,fFullscreen);
	SideBar.CheckItem(CM_PANEL,fFullscreen?m_Fullscreen.IsPanelVisible():fShowPanelWindow);
	return true;
}


HWND CMainWindow::GetVideoHostWindow() const
{
	if (m_pCore->GetStandby())
		return NULL;
	if (m_pCore->GetFullscreen())
		return m_Fullscreen.GetHandle();
	return m_hwnd;
}


void CMainWindow::ShowNotificationBar(LPCTSTR pszText,
									  CNotificationBar::MessageType Type,DWORD Duration)
{
	NotificationBar.SetFont(OSDOptions.GetNotificationBarFont());
	NotificationBar.Show(pszText,Type,max((DWORD)OSDOptions.GetNotificationBarDuration(),Duration));
}


void CMainWindow::AdjustWindowSize(int Width,int Height)
{
	if (IsZoomed(m_hwnd))
		return;

	RECT rcOld,rc;
	GetPosition(&rcOld);

	const HMONITOR hMonitor=::MonitorFromRect(&rcOld,MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize=sizeof(mi);
	::GetMonitorInfo(hMonitor,&mi);

	::SetRect(&rc,0,0,Width,Height);
	m_Viewer.GetViewWindow().CalcWindowRect(&rc);
	Width=rc.right-rc.left;
	Height=rc.bottom-rc.top;
	m_LayoutBase.GetScreenPosition(&rc);
	rc.right=rc.left+Width;
	rc.bottom=rc.top+Height;
	if (m_fShowTitleBar && m_fCustomTitleBar)
		TitleBarUtil.ReserveArea(&rc,true);
	if (m_fShowSideBar)
		SideBarUtil.ReserveArea(&rc,true);
	if (fShowPanelWindow && !PanelFrame.GetFloating()) {
		Layout::CSplitter *pSplitter=dynamic_cast<Layout::CSplitter*>(
			m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));
		rc.right+=pSplitter->GetBarWidth()+pSplitter->GetPaneSize(CONTAINER_ID_PANEL);
	}
	if (m_fShowStatusBar)
		rc.bottom+=StatusView.CalcHeight(rc.right-rc.left);
	if (m_fCustomFrame) {
		rc.left-=m_CustomFrameWidth;
		rc.right+=m_CustomFrameWidth;
		rc.top-=m_CustomFrameWidth;
		rc.bottom+=m_CustomFrameWidth;
	} else {
		CalcPositionFromClientRect(&rc);
	}
	if (ViewOptions.GetNearCornerResizeOrigin()) {
		if (abs(rcOld.left-mi.rcWork.left)>abs(rcOld.right-mi.rcWork.right)) {
			rc.left=rcOld.right-(rc.right-rc.left);
			rc.right=rcOld.right;
		}
		if (abs(rcOld.top-mi.rcWork.top)>abs(rcOld.bottom-mi.rcWork.bottom)) {
			rc.top=rcOld.bottom-(rc.bottom-rc.top);
			rc.bottom=rcOld.bottom;
		}
	}

	// ウィンドウがモニタの外に出ないようにする
	if (rcOld.left>=mi.rcWork.left && rcOld.top>=mi.rcWork.top
			&& rcOld.right<=mi.rcWork.right && rcOld.bottom<=mi.rcWork.bottom) {
		if (rc.right>mi.rcWork.right && rc.left>mi.rcWork.left)
			::OffsetRect(&rc,max(mi.rcWork.right-rc.right,mi.rcWork.left-rc.left),0);
		if (rc.bottom>mi.rcWork.bottom && rc.top>mi.rcWork.top)
			::OffsetRect(&rc,0,max(mi.rcWork.bottom-rc.bottom,mi.rcWork.top-rc.top));
	}

	SetPosition(&rc);
	PanelEventHandler.OnOwnerMovingOrSizing(&rcOld,&rc);
}


bool CMainWindow::ReadSettings(CSettings &Settings)
{
	int Left,Top,Width,Height,Value;
	bool f;

	GetPosition(&Left,&Top,&Width,&Height);
	Settings.Read(TEXT("WindowLeft"),&Left);
	Settings.Read(TEXT("WindowTop"),&Top);
	Settings.Read(TEXT("WindowWidth"),&Width);
	Settings.Read(TEXT("WindowHeight"),&Height);
	SetPosition(Left,Top,Width,Height);
	MoveToMonitorInside();
	if (Settings.Read(TEXT("WindowMaximize"),&f))
		SetMaximize(f);
	if (Settings.Read(TEXT("AlwaysOnTop"),&f))
		m_pCore->SetAlwaysOnTop(f);
	if (Settings.Read(TEXT("ShowStatusBar"),&f))
		SetStatusBarVisible(f);
	Settings.Read(TEXT("PopupStatusBar"),&m_fPopupStatusBar);
	if (Settings.Read(TEXT("ShowTitleBar"),&f))
		SetTitleBarVisible(f);
	Settings.Read(TEXT("PopupTitleBar"),&m_fPopupTitleBar);
	if (Settings.Read(TEXT("PanelDockingIndex"),&Value)
			&& (Value==0 || Value==1))
		m_PanelPaneIndex=Value;
	if (Settings.Read(TEXT("FullscreenPanelWidth"),&Value))
		m_Fullscreen.SetPanelWidth(Value);
	if (Settings.Read(TEXT("ThinFrameWidth"),&Value))
		m_ThinFrameWidth=max(Value,1);
	Value=FRAME_NORMAL;
	if (!Settings.Read(TEXT("FrameType"),&Value)) {
		if (Settings.Read(TEXT("ThinFrame"),&f) && f)	// 以前のバージョンとの互換用
			Value=FRAME_CUSTOM;
	}
	SetCustomFrame(Value!=FRAME_NORMAL,Value==FRAME_CUSTOM?m_ThinFrameWidth:0);
	if (!m_fCustomFrame && Settings.Read(TEXT("CustomTitleBar"),&f))
		SetCustomTitleBar(f);
	Settings.Read(TEXT("SplitTitleBar"),&m_fSplitTitleBar);
	Settings.Read(TEXT("ClientEdge"),&m_fViewWindowEdge);
	if (Settings.Read(TEXT("ShowSideBar"),&f))
		SetSideBarVisible(f);
	Settings.Read(TEXT("FrameCut"),&m_fFrameCut);

	return true;
}


bool CMainWindow::WriteSettings(CSettings &Settings)
{
	int Left,Top,Width,Height;

	GetPosition(&Left,&Top,&Width,&Height);
	Settings.Write(TEXT("WindowLeft"),Left);
	Settings.Write(TEXT("WindowTop"),Top);
	Settings.Write(TEXT("WindowWidth"),Width);
	Settings.Write(TEXT("WindowHeight"),Height);
	Settings.Write(TEXT("WindowMaximize"),GetMaximize());
	Settings.Write(TEXT("AlwaysOnTop"),m_pCore->GetAlwaysOnTop());
	Settings.Write(TEXT("ShowStatusBar"),m_fShowStatusBar);
//	Settings.Write(TEXT("PopupStatusBar"),m_fPopupStatusBar);
	Settings.Write(TEXT("ShowTitleBar"),m_fShowTitleBar);
//	Settings.Write(TEXT("PopupTitleBar"),m_fPopupTitleBar);
	Settings.Write(TEXT("PanelDockingIndex"),m_PanelPaneIndex);
	Settings.Write(TEXT("FullscreenPanelWidth"),m_Fullscreen.GetPanelWidth());
	Settings.Write(TEXT("FrameType"),
		!m_fCustomFrame?FRAME_NORMAL:(m_CustomFrameWidth==0?FRAME_NONE:FRAME_CUSTOM));
//	Settings.Write(TEXT("ThinFrameWidth"),m_ThinFrameWidth);
	Settings.Write(TEXT("CustomTitleBar"),m_fCustomTitleBar);
	Settings.Write(TEXT("SplitTitleBar"),m_fSplitTitleBar);
	Settings.Write(TEXT("ClientEdge"),m_fViewWindowEdge);
	Settings.Write(TEXT("ShowSideBar"),m_fShowSideBar);
	Settings.Write(TEXT("FrameCut"),m_fFrameCut);

	return true;
}


bool CMainWindow::SetAlwaysOnTop(bool fTop)
{
	if (m_hwnd!=NULL) {
		::SetWindowPos(m_hwnd,fTop?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,
					   SWP_NOMOVE | SWP_NOSIZE);
		MainMenu.CheckItem(CM_ALWAYSONTOP,fTop);
		SideBar.CheckItem(CM_ALWAYSONTOP,fTop);
	}
	return true;
}


void CMainWindow::SetStatusBarVisible(bool fVisible)
{
	if (m_fShowStatusBar!=fVisible) {
		if (!m_pCore->GetFullscreen()) {
			m_fShowStatusBar=fVisible;
			m_LayoutBase.SetContainerVisible(CONTAINER_ID_STATUS,fVisible);
			if (!GetMaximize()) {
				RECT rc;

				GetPosition(&rc);
				if (fVisible)
					rc.bottom+=StatusView.GetHeight();
				else
					rc.bottom-=StatusView.GetHeight();
				SetPosition(&rc);
			}
			//MainMenu.CheckItem(CM_STATUSBAR,fVisible);
			SideBar.CheckItem(CM_STATUSBAR,fVisible);
		}
	}
}


void CMainWindow::SetTitleBarVisible(bool fVisible)
{
	if (m_fShowTitleBar!=fVisible) {
		m_fShowTitleBar=fVisible;
		if (m_hwnd!=NULL) {
			bool fMaximize=GetMaximize();
			RECT rc;

			if (!fMaximize)
				GetPosition(&rc);
			if (!m_fCustomTitleBar)
				SetStyle(GetStyle()^WS_CAPTION,fMaximize);
			else if (!fVisible)
				m_LayoutBase.SetContainerVisible(CONTAINER_ID_TITLEBAR,false);
			if (!fMaximize) {
				int CaptionHeight;

				if (!m_fCustomTitleBar)
					CaptionHeight=::GetSystemMetrics(SM_CYCAPTION);
				else
					CaptionHeight=m_TitleBar.GetHeight();
				if (fVisible)
					rc.top-=CaptionHeight;
				else
					rc.top+=CaptionHeight;
				::SetWindowPos(m_hwnd,NULL,rc.left,rc.top,
							   rc.right-rc.left,rc.bottom-rc.top,
							   SWP_NOZORDER | SWP_FRAMECHANGED | SWP_DRAWFRAME);
			}
			if (m_fCustomTitleBar && fVisible)
				m_LayoutBase.SetContainerVisible(CONTAINER_ID_TITLEBAR,true);
			//MainMenu.CheckItem(CM_TITLEBAR,fVisible);
		}
	}
}


void CMainWindow::SetCustomTitleBar(bool fCustom)
{
	if (m_fCustomTitleBar!=fCustom) {
		if (!fCustom && m_fCustomFrame)
			SetCustomFrame(false);
		m_fCustomTitleBar=fCustom;
		if (m_hwnd!=NULL) {
			if (m_fShowTitleBar) {
				if (!fCustom)
					m_LayoutBase.SetContainerVisible(CONTAINER_ID_TITLEBAR,false);
				SetStyle(GetStyle()^WS_CAPTION,true);
				if (fCustom)
					m_LayoutBase.SetContainerVisible(CONTAINER_ID_TITLEBAR,true);
			}
			//MainMenu.CheckItem(CM_CUSTOMTITLEBAR,fCustom);
			MainMenu.EnableItem(CM_SPLITTITLEBAR,!m_fCustomFrame && fCustom);
		}
	}
}


void CMainWindow::SetSplitTitleBar(bool fSplit)
{
	if (m_fSplitTitleBar!=fSplit) {
		m_fSplitTitleBar=fSplit;
		if (m_fCustomTitleBar && m_hwnd!=NULL) {
			Layout::CSplitter *pSideBarSplitter,*pTitleBarSplitter,*pPanelSplitter;
			Layout::CSplitter *pStatusSplitter,*pParentSplitter;

			pSideBarSplitter=dynamic_cast<Layout::CSplitter*>(
				m_LayoutBase.GetContainerByID(CONTAINER_ID_SIDEBARSPLITTER));
			pTitleBarSplitter=dynamic_cast<Layout::CSplitter*>(
				m_LayoutBase.GetContainerByID(CONTAINER_ID_TITLEBARSPLITTER));
			pPanelSplitter=dynamic_cast<Layout::CSplitter*>(
				m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));
			pStatusSplitter=dynamic_cast<Layout::CSplitter*>(
				m_LayoutBase.GetContainerByID(CONTAINER_ID_STATUSSPLITTER));
			if (pSideBarSplitter==NULL || pTitleBarSplitter==NULL
					|| pPanelSplitter==NULL || pStatusSplitter==NULL)
				return;
			const int PanelPane=GetPanelPaneIndex();
			if (fSplit) {
				pTitleBarSplitter->ReplacePane(1,pSideBarSplitter);
				pTitleBarSplitter->SetAdjustPane(CONTAINER_ID_SIDEBARSPLITTER);
				pPanelSplitter->ReplacePane(1-PanelPane,pTitleBarSplitter);
				pPanelSplitter->SetAdjustPane(CONTAINER_ID_TITLEBARSPLITTER);
				pParentSplitter=pPanelSplitter;
			} else {
				pPanelSplitter->ReplacePane(1-PanelPane,pSideBarSplitter);
				pPanelSplitter->SetAdjustPane(CONTAINER_ID_SIDEBARSPLITTER);
				pTitleBarSplitter->ReplacePane(1,pPanelSplitter);
				pTitleBarSplitter->SetAdjustPane(CONTAINER_ID_PANELSPLITTER);
				pParentSplitter=pTitleBarSplitter;
			}
			pStatusSplitter->ReplacePane(0,pParentSplitter);
			pStatusSplitter->SetAdjustPane(pParentSplitter->GetID());
			m_LayoutBase.Adjust();
			//MainMenu.CheckItem(CM_SPLITTITLEBAR,fSplit);
		}
	}
}


void CMainWindow::SetCustomFrame(bool fCustomFrame,int Width)
{
	if (m_fCustomFrame!=fCustomFrame || (fCustomFrame && m_CustomFrameWidth!=Width)) {
		if (fCustomFrame && Width<0)
			return;
		if (fCustomFrame && !m_fCustomTitleBar)
			SetCustomTitleBar(true);
		m_fCustomFrame=fCustomFrame;
		if (fCustomFrame)
			m_CustomFrameWidth=Width;
		if (m_hwnd!=NULL) {
			::SetWindowPos(m_hwnd,NULL,0,0,0,0,
						   SWP_FRAMECHANGED | SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
			CAeroGlass Aero;
			Aero.EnableNcRendering(m_hwnd,!fCustomFrame);
			if (fCustomFrame) {
				HookWindows(m_LayoutBase.GetHandle());
				HookWindows(PanelForm.GetHandle());
			}
			/*
			MainMenu.CheckRadioItem(CM_WINDOWFRAME_NORMAL,CM_WINDOWFRAME_NONE,
									!fCustomFrame?CM_WINDOWFRAME_NORMAL:
									(m_CustomFrameWidth==0?CM_WINDOWFRAME_NONE:CM_WINDOWFRAME_CUSTOM));
			MainMenu.EnableItem(CM_CUSTOMTITLEBAR,!fCustomFrame);
			MainMenu.EnableItem(CM_SPLITTITLEBAR,!fCustomFrame && m_fCustomTitleBar);
			*/
		}
	}
}


void CMainWindow::SetSideBarVisible(bool fVisible)
{
	if (m_fShowSideBar!=fVisible) {
		m_fShowSideBar=fVisible;
		if (m_hwnd!=NULL) {
			RECT rc;

			if (!fVisible)
				m_LayoutBase.SetContainerVisible(CONTAINER_ID_SIDEBAR,false);
			if (!GetMaximize()) {
				GetPosition(&rc);
				RECT rcArea=rc;
				if (fVisible)
					SideBarUtil.ReserveArea(&rcArea,true);
				else
					SideBarUtil.AdjustArea(&rcArea);
				rc.right=rc.left+(rcArea.right-rcArea.left);
				rc.bottom=rc.top+(rcArea.bottom-rcArea.top);
				SetPosition(&rc);
			}
			if (fVisible)
				m_LayoutBase.SetContainerVisible(CONTAINER_ID_SIDEBAR,true);
			//MainMenu.CheckItem(CM_SIDEBAR,fVisible);
		}
	}
}


bool CMainWindow::OnBarMouseLeave(HWND hwnd)
{
	POINT pt;

	::GetCursorPos(&pt);
	::ScreenToClient(::GetParent(hwnd),&pt);
	if (hwnd==m_TitleBar.GetHandle()) {
		if (!m_fShowTitleBar) {
			if (!m_fShowSideBar && SideBar.GetVisible()
					&& SideBarOptions.GetPlace()==CSideBarOptions::PLACE_TOP) {
				if (::RealChildWindowFromPoint(SideBar.GetParent(),pt)==SideBar.GetHandle())
					return false;
			}
			m_TitleBar.SetVisible(false);
			if (!m_fShowSideBar && SideBar.GetVisible())
				SideBar.SetVisible(false);
			return true;
		}
	} else if (hwnd==StatusView.GetHandle()) {
		if (!m_fShowStatusBar) {
			if (!m_fShowSideBar && SideBar.GetVisible()
					&& SideBarOptions.GetPlace()==CSideBarOptions::PLACE_BOTTOM) {
				if (::RealChildWindowFromPoint(SideBar.GetParent(),pt)==SideBar.GetHandle())
					return false;
			}
			StatusView.SetVisible(false);
			if (!m_fShowSideBar && SideBar.GetVisible())
				SideBar.SetVisible(false);
			return true;
		}
	} else if (hwnd==SideBar.GetHandle()) {
		if (!m_fShowSideBar) {
			SideBar.SetVisible(false);
			if (!m_fShowTitleBar && m_TitleBar.GetVisible()
					&& ::RealChildWindowFromPoint(m_TitleBar.GetParent(),pt)!=m_TitleBar.GetHandle())
				m_TitleBar.SetVisible(false);
			if (!m_fShowStatusBar && StatusView.GetVisible()
					&& ::RealChildWindowFromPoint(StatusView.GetParent(),pt)!=StatusView.GetHandle())
				StatusView.SetVisible(false);
			return true;
		}
	}

	return false;
}


int CMainWindow::GetPanelPaneIndex() const
{
	Layout::CSplitter *pSplitter=
		dynamic_cast<Layout::CSplitter*>(m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));

	if (pSplitter==NULL)
		return m_PanelPaneIndex;
	return pSplitter->IDToIndex(CONTAINER_ID_PANEL);
}


LRESULT CMainWindow::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	HANDLE_MSG(hwnd,WM_COMMAND,OnCommand);
	HANDLE_MSG(hwnd,WM_TIMER,OnTimer);

	case WM_SIZE:
		OnSizeChanged((UINT)wParam,LOWORD(lParam),HIWORD(lParam));
		return 0;

	case WM_SIZING:
		if (OnSizeChanging((UINT)wParam,reinterpret_cast<LPRECT>(lParam)))
			return TRUE;
		break;

	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO pmmi=reinterpret_cast<LPMINMAXINFO>(lParam);
			SIZE sz;
			RECT rc;

			m_LayoutBase.GetMinSize(&sz);
			::SetRect(&rc,0,0,sz.cx,sz.cy);
			CalcPositionFromClientRect(&rc);
			pmmi->ptMinTrackSize.x=rc.right-rc.left;
			pmmi->ptMinTrackSize.y=rc.bottom-rc.top;
		}
		return 0;

	case WM_MOVE:
		OSDManager.OnParentMove();
		return 0;

	case WM_SHOWWINDOW:
		if (!wParam) {
			OSDManager.ClearOSD();
			OSDManager.Reset();
		}
		break;

	case WM_RBUTTONDOWN:
		if (m_pCore->GetFullscreen()) {
			m_Fullscreen.OnRButtonDown();
		} else {
			::SendMessage(hwnd,WM_COMMAND,
				MAKEWPARAM(OperationOptions.GetRightClickCommand(),COMMAND_FROM_MOUSE),0);
		}
		return 0;

	case WM_MBUTTONDOWN:
		if (m_pCore->GetFullscreen()) {
			m_Fullscreen.OnMButtonDown();
		} else {
			::SendMessage(hwnd,WM_COMMAND,
				MAKEWPARAM(OperationOptions.GetMiddleClickCommand(),COMMAND_FROM_MOUSE),0);
		}
		return 0;

	case WM_NCLBUTTONDOWN:
		if (wParam!=HTCAPTION)
			break;
		ForegroundWindow(hwnd);
	case WM_LBUTTONDOWN:
		if (uMsg==WM_NCLBUTTONDOWN || OperationOptions.GetDisplayDragMove()) {
			/*
			m_ptDragStartPos.x=GET_X_LPARAM(lParam);
			m_ptDragStartPos.y=GET_Y_LPARAM(lParam);
			::ClientToScreen(hwnd,&m_ptDragStartPos);
			*/
			::GetCursorPos(&m_ptDragStartPos);
			::GetWindowRect(hwnd,&m_rcDragStart);
			::SetCapture(hwnd);
		}
		return 0;

	case WM_NCLBUTTONUP:
	case WM_LBUTTONUP:
		if (::GetCapture()==hwnd)
			::ReleaseCapture();
		return 0;

	case WM_CAPTURECHANGED:
		TitleBarUtil.EndDrag();
		return 0;

	case WM_MOUSEMOVE:
		OnMouseMove(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		return 0;

	case WM_LBUTTONDBLCLK:
		::SendMessage(hwnd,WM_COMMAND,
			MAKEWPARAM(OperationOptions.GetLeftDoubleClickCommand(),COMMAND_FROM_MOUSE),0);
		return 0;

	case WM_SYSKEYDOWN:
		if (wParam!=VK_F10)
			break;
	case WM_KEYDOWN:
		{
			int Command;

			if (wParam>=VK_F1 && wParam<=VK_F12) {
				if (!Accelerator.IsFunctionKeyChannelChange())
					break;
				Command=CM_CHANNELNO_FIRST+((int)wParam-VK_F1);
			} else if (wParam>=VK_NUMPAD0 && wParam<=VK_NUMPAD9) {
				if (m_ChannelNoInput.fInputting) {
					OnChannelNoInput((int)wParam-VK_NUMPAD0);
					break;
				}
				if (!Accelerator.IsNumPadChannelChange())
					break;
				if (wParam==VK_NUMPAD0)
					Command=CM_CHANNELNO_FIRST+9;
				else
					Command=CM_CHANNELNO_FIRST+((int)wParam-VK_NUMPAD1);
			} else if (wParam>='0' && wParam<='9') {
				if (m_ChannelNoInput.fInputting) {
					OnChannelNoInput((int)wParam-'0');
					break;
				}
				if (!Accelerator.IsDigitKeyChannelChange())
					break;
				if (wParam=='0')
					Command=CM_CHANNELNO_FIRST+9;
				else
					Command=CM_CHANNELNO_FIRST+((int)wParam-'1');
			} else if (wParam>=VK_F13 && wParam<=VK_F24
					&& !ControllerManager.IsControllerEnabled(TEXT("HDUS Remocon"))
					&& (::GetKeyState(VK_SHIFT)<0 || ::GetKeyState(VK_CONTROL)<0)) {
				ShowMessage(TEXT("リモコンを使用するためには、メニューの [プラグイン] -> [HDUSリモコン] でリモコンを有効にしてください。"),
							TEXT("お知らせ"),MB_OK | MB_ICONINFORMATION);
				break;
			} else {
				break;
			}
			SendCommand(Command);
		}
		return 0;

	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		{
			bool fHorz=uMsg==WM_MOUSEHWHEEL;

			OnMouseWheel(wParam,lParam,fHorz);
			// WM_MOUSEHWHEEL は 1を返さないと繰り返し送られて来ないらしい
			return fHorz;
		}

	case WM_MEASUREITEM:
		{
			LPMEASUREITEMSTRUCT pmis=reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);

			if (pmis->itemID>=CM_ASPECTRATIO_FIRST && pmis->itemID<=CM_ASPECTRATIO_3D_LAST) {
				if (AspectRatioIconMenu.OnMeasureItem(hwnd,wParam,lParam))
					return TRUE;
				break;
			}
			if (ChannelMenu.OnMeasureItem(hwnd,wParam,lParam))
				return TRUE;
			if (FavoritesMenu.OnMeasureItem(hwnd,wParam,lParam))
				return TRUE;
		}
		break;

	case WM_DRAWITEM:
		if (AspectRatioIconMenu.OnDrawItem(hwnd,wParam,lParam))
			return TRUE;
		if (ChannelMenu.OnDrawItem(hwnd,wParam,lParam))
			return TRUE;
		if (FavoritesMenu.OnDrawItem(hwnd,wParam,lParam))
			return TRUE;
		break;

// ウィンドウ枠を独自のものにするためのコード
	case WM_NCACTIVATE:
		if (m_fCustomFrame)
			return TRUE;
		break;

	case WM_NCCALCSIZE:
		if (m_fCustomFrame) {
			if (wParam!=0) {
				NCCALCSIZE_PARAMS *pnccsp=reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);

				::InflateRect(&pnccsp->rgrc[0],-m_CustomFrameWidth,-m_CustomFrameWidth);
			}
			return 0;
		}
		break;

	case WM_NCPAINT:
		if (m_fCustomFrame) {
			HDC hdc=::GetWindowDC(hwnd);
			RECT rc,rcEmpty;

			::GetWindowRect(hwnd,&rc);
			::OffsetRect(&rc,-rc.left,-rc.top);
			rcEmpty=rc;
			::InflateRect(&rcEmpty,-m_CustomFrameWidth,-m_CustomFrameWidth);
			DrawUtil::FillBorder(hdc,&rc,&rcEmpty,&rc,
								 static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
			::ReleaseDC(hwnd,hdc);
			return 0;
		}
		break;

	case WM_NCHITTEST:
		if (m_fCustomFrame) {
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			int BorderWidth=m_CustomFrameWidth;
			RECT rc;
			int Code=HTNOWHERE;

			::GetWindowRect(hwnd,&rc);
			if (x>=rc.left && x<rc.left+BorderWidth) {
				if (y>=rc.top) {
					if (y<rc.top+BorderWidth)
						Code=HTTOPLEFT;
					else if (y<rc.bottom-BorderWidth)
						Code=HTLEFT;
					else if (y<rc.bottom)
						Code=HTBOTTOMLEFT;
				}
			} else if (x>=rc.right-BorderWidth && x<rc.right) {
				if (y>=rc.top) {
					if (y<rc.top+BorderWidth)
						Code=HTTOPRIGHT;
					else if (y<rc.bottom-BorderWidth)
						Code=HTRIGHT;
					else if (y<rc.bottom)
						Code=HTBOTTOMRIGHT;
				}
			} else if (y>=rc.top && y<rc.top+BorderWidth) {
				Code=HTTOP;
			} else if (y>=rc.bottom-BorderWidth && y<rc.bottom) {
				Code=HTBOTTOM;
			} else {
				POINT pt={x,y};
				if (::PtInRect(&rc,pt))
					Code=HTCLIENT;
			}
			return Code;
		}
		break;
// ウィンドウ枠を独自のものにするためのコード終わり

	case WM_INITMENUPOPUP:
		if (OnInitMenuPopup(reinterpret_cast<HMENU>(wParam)))
			return 0;
		break;

	case WM_UNINITMENUPOPUP:
		if (ChannelMenu.OnUninitMenuPopup(hwnd,wParam,lParam))
			return 0;
		if (FavoritesMenu.OnUninitMenuPopup(hwnd,wParam,lParam))
			return 0;
		break;

	case WM_MENUSELECT:
		if (ChannelMenu.OnMenuSelect(hwnd,wParam,lParam))
			return 0;
		if (FavoritesMenu.OnMenuSelect(hwnd,wParam,lParam))
			return 0;
		break;

	case WM_SYSCOMMAND:
		switch ((wParam&0xFFFFFFF0UL)) {
		case SC_MONITORPOWER:
			if (ViewOptions.GetNoMonitorLowPower()
					&& AppMain.GetUICore()->IsViewerEnabled())
				return 0;
			break;

		case SC_SCREENSAVE:
			if (ViewOptions.GetNoScreenSaver()
					&& AppMain.GetUICore()->IsViewerEnabled())
				return 0;
			break;

		case SC_ABOUT:
			{
				CAboutDialog AboutDialog;

				AboutDialog.Show(GetVideoHostWindow());
			}
			return 0;

		case SC_MINIMIZE:
		case SC_MAXIMIZE:
		case SC_RESTORE:
			if (m_pCore->GetFullscreen())
				m_pCore->SetFullscreen(false);
			break;

		case SC_CLOSE:
			SendCommand(CM_CLOSE);
			return 0;
		}
		break;

	case WM_APPCOMMAND:
		{
			int Command=Accelerator.TranslateAppCommand(wParam,lParam);

			if (Command!=0) {
				SendCommand(Command);
				return TRUE;
			}
		}
		break;

	case WM_INPUT:
		return Accelerator.OnInput(hwnd,wParam,lParam);

	case WM_HOTKEY:
		{
			int Command=Accelerator.TranslateHotKey(wParam,lParam);

			if (Command>0)
				PostMessage(WM_COMMAND,Command,0);
		}
		return 0;

	case WM_SETFOCUS:
		m_Viewer.GetDisplayBase().SetFocus();
		return 0;

	case WM_SETTEXT:
		{
			LPCTSTR pszText=reinterpret_cast<LPCTSTR>(lParam);

			m_TitleBar.SetLabel(pszText);
			if (m_pCore->GetFullscreen())
				::SetWindowText(m_Fullscreen.GetHandle(),pszText);
		}
		break;

	case WM_SETICON:
		if (wParam==ICON_SMALL) {
			m_TitleBar.SetIcon(reinterpret_cast<HICON>(lParam));
			m_Fullscreen.SendMessage(uMsg,wParam,lParam);
		}
		break;

	case WM_POWERBROADCAST:
		if (wParam==PBT_APMSUSPEND) {
			Logger.AddLog(TEXT("サスペンドへの移行通知を受けました。"));
			if (m_fProgramGuideUpdating) {
				EndProgramGuideUpdate(0);
			} else if (!m_pCore->GetStandby()) {
				StoreTunerResumeInfo();
			}
			SuspendViewer(ResumeInfo::VIEWERSUSPEND_SUSPEND);
			AppMain.CloseTuner();
			FinalizeViewer();
		} else if (wParam==PBT_APMRESUMESUSPEND) {
			Logger.AddLog(TEXT("サスペンドからの復帰通知を受けました。"));
			if (!m_pCore->GetStandby()) {
				// 遅延させた方がいいかも?
				ResumeTuner();
			}
			ResumeViewer(ResumeInfo::VIEWERSUSPEND_SUSPEND);
		}
		break;

#ifndef WM_DWMCOMPOSITIONCHANGED
#define WM_DWMCOMPOSITIONCHANGED 0x031E
#endif
	case WM_DWMCOMPOSITIONCHANGED:
		OSDOptions.OnDwmCompositionChanged();
		return 0;

	case WM_APP_SERVICEUPDATE:
		// サービスが更新された
		{
			CServiceUpdateInfo *pInfo=reinterpret_cast<CServiceUpdateInfo*>(lParam);
			int i;

			if (pInfo->m_fStreamChanged) {
				if (m_ResetErrorCountTimer.IsEnabled())
					m_ResetErrorCountTimer.Begin(hwnd,2000);

				m_Viewer.GetDisplayBase().SetVisible(false);
			}

			if (!AppMain.IsChannelScanning()
					&& pInfo->m_NumServices>0 && pInfo->m_CurService>=0) {
				const CChannelInfo *pChInfo=ChannelManager.GetCurrentRealChannelInfo();
				WORD ServiceID,TransportStreamID;

				TransportStreamID=pInfo->m_TransportStreamID;
				ServiceID=pInfo->m_pServiceList[pInfo->m_CurService].ServiceID;
				if (/*pInfo->m_fStreamChanged
						&& */TransportStreamID!=0 && ServiceID!=0
						&& !CoreEngine.IsNetworkDriver()
						&& (pChInfo==NULL
						|| ((pChInfo->GetTransportStreamID()!=0
						&& pChInfo->GetTransportStreamID()!=TransportStreamID)
						|| (pChInfo->GetServiceID()!=0
						&& pChInfo->GetServiceID()!=ServiceID)))) {
					// 外部からチャンネル変更されたか、
					// BonDriverが開かれたときのデフォルトチャンネル
					AppMain.FollowChannelChange(TransportStreamID,ServiceID);
				}
				if (pChInfo!=NULL && !CoreEngine.IsNetworkDriver()) {
					// チャンネルの情報を更新する
					// 古いチャンネル設定ファイルにはNIDとTSIDの情報が含まれていないため
					const WORD NetworkID=pInfo->m_NetworkID;

					if (NetworkID!=0) {
						for (i=0;i<pInfo->m_NumServices;i++) {
							ServiceID=pInfo->m_pServiceList[i].ServiceID;
							if (ServiceID!=0) {
								ChannelManager.UpdateStreamInfo(
									pChInfo->GetSpace(),
									pChInfo->GetChannelIndex(),
									NetworkID,TransportStreamID,ServiceID);
							}
						}
					}
				}
				PluginManager.SendServiceUpdateEvent();
			} else if (pInfo->m_fServiceListEmpty && pInfo->m_fStreamChanged
					&& !AppMain.IsChannelScanning()
					&& !m_fProgramGuideUpdating) {
				ShowNotificationBar(TEXT("このチャンネルは放送休止中です"),
									CNotificationBar::MESSAGE_INFO);
			}

			delete pInfo;
#ifdef NETWORK_REMOCON_SUPPORT
			if (pNetworkRemocon!=NULL)
				pNetworkRemocon->GetChannel(&GetChannelReceiver);
#endif
		}
		return 0;

	case WM_APP_SERVICECHANGED:
		m_pCore->UpdateTitle();
		StatusView.UpdateItem(STATUS_ITEM_CHANNEL);
		return 0;

#ifdef NETWORK_REMOCON_SUPPORT
	case WM_APP_CHANNELCHANGE:
		{
			const CChannelList &List=pNetworkRemocon->GetChannelList();

			ChannelManager.SetNetworkRemoconCurrentChannel((int)wParam);
			StatusView.UpdateItem(STATUS_ITEM_CHANNEL);
			ControlPanel.UpdateItem(CONTROLPANEL_ITEM_CHANNEL);
			const int ChannelNo=List.GetChannelNo(ChannelManager.GetNetworkRemoconCurrentChannel());
			MainMenu.CheckRadioItem(CM_CHANNELNO_FIRST,CM_CHANNELNO_LAST,
									CM_CHANNELNO_FIRST+ChannelNo-1);
			SideBar.CheckRadioItem(CM_CHANNELNO_FIRST,CM_CHANNELNO_LAST,
								   CM_CHANNELNO_FIRST+ChannelNo-1);
		}
		return 0;
#endif

	/*
	case WM_APP_IMAGESAVE:
		{
			MessageBox(NULL,TEXT("画像の保存でエラーが発生しました。"),NULL,
												MB_OK | MB_ICONEXCLAMATION);
		}
		return 0;
	*/

	case WM_APP_TRAYICON:
		switch (lParam) {
		case WM_RBUTTONDOWN:
			{
				CPopupMenu Menu(hInst,IDM_TRAY);

				Menu.EnableItem(CM_SHOW,
								m_pCore->GetStandby() || IsMinimizeToTray());
				// お約束が必要な理由は以下を参照
				// http://support.microsoft.com/kb/135788/en-us
				ForegroundWindow(hwnd);				// お約束
				Menu.Show(hwnd);
				::PostMessage(hwnd,WM_NULL,0,0);	// お約束
			}
			break;

		case WM_LBUTTONUP:
			SendCommand(CM_SHOW);
			break;
		}
		return 0;

	case WM_APP_EXECUTE:
		// 複数起動禁止時に複数起動された
		// (新しく起動されたプロセスから送られてくる)
		{
			ATOM atom=(ATOM)wParam;
			TCHAR szCmdLine[256];

			szCmdLine[0]='\0';
			if (atom!=0) {
				::GlobalGetAtomName(atom,szCmdLine,lengthof(szCmdLine));
				::GlobalDeleteAtom(atom);
			}
			OnExecute(szCmdLine);
		}
		return 0;

	case WM_APP_QUERYPORT:
		// 使っているポートを返す
		if (!m_fClosing && CoreEngine.IsNetworkDriver()) {
			WORD Port=ChannelManager.GetCurrentChannel()+
										(CoreEngine.IsUDPDriver()?1234:2230);
#ifdef NETWORK_REMOCON_SUPPORT
			WORD RemoconPort=pNetworkRemocon!=NULL?pNetworkRemocon->GetPort():0;
			return MAKELRESULT(Port,RemoconPort);
#else
			return MAKELRESULT(Port,0);
#endif
		}
		return 0;

	case WM_APP_FILEWRITEERROR:
		// ファイルの書き出しエラー
		ShowErrorMessage(TEXT("ファイルへの書き出しでエラーが発生しました。"));
		return 0;

	case WM_APP_VIDEOSIZECHANGED:
		// 映像サイズが変わった
		/*
			ストリームの映像サイズの変化を検知してから、それが実際に
			表示されるまでにはタイムラグがあるため、後で調整を行う
		*/
		m_VideoSizeChangedTimerCount=0;
		::SetTimer(hwnd,TIMER_ID_VIDEOSIZECHANGED,500,NULL);
		if (m_AspectRatioResetTime!=0
				&& !m_pCore->GetFullscreen() && !::IsZoomed(hwnd)
				&& IsViewerEnabled()
				&& TickTimeSpan(m_AspectRatioResetTime,::GetTickCount())<6000) {
			int Width,Height;

			if (CoreEngine.GetVideoViewSize(&Width,&Height)) {
				SIZE sz;
				m_Viewer.GetVideoContainer().GetClientSize(&sz);
				if (sz.cx<Width*sz.cy/Height)
					AdjustWindowSize(Width*sz.cy/Height,sz.cy);
				m_AspectRatioResetTime=0;
			}
		}
		return 0;

	case WM_APP_EMMPROCESSED:
		// EMM 処理が行われた
		Logger.AddLog(wParam!=0?TEXT("EMM処理を行いました。"):TEXT("EMM処理でエラーが発生しました。"));
		return 0;

	case WM_APP_ECMERROR:
		// ECM 処理のエラーが発生した
		{
			LPTSTR pszText=reinterpret_cast<LPTSTR>(lParam);

			if (OSDOptions.IsNotifyEnabled(COSDOptions::NOTIFY_ECMERROR))
				ShowNotificationBar(TEXT("スクランブル解除でエラーが発生しました"),
									CNotificationBar::MESSAGE_ERROR);
			if (pszText!=NULL) {
				TCHAR szText[256];
				StdUtil::snprintf(szText,lengthof(szText),
								  TEXT("ECM処理でエラーが発生しました。(%s)"),pszText);
				Logger.AddLog(szText);
				delete [] pszText;
			} else {
				Logger.AddLog(TEXT("ECM処理でエラーが発生しました。"));
			}
		}
		return 0;

	case WM_APP_ECMREFUSED:
		// ECM が受け付けられない
		if (OSDOptions.IsNotifyEnabled(COSDOptions::NOTIFY_ECMERROR)
				&& IsViewerEnabled())
			ShowNotificationBar(TEXT("契約されていないため視聴できません"),
								CNotificationBar::MESSAGE_WARNING,6000);
		return 0;

	case WM_APP_CARDREADERHUNG:
		// カードリーダーから応答が無い
		if (OSDOptions.IsNotifyEnabled(COSDOptions::NOTIFY_ECMERROR))
			ShowNotificationBar(TEXT("カードリーダーから応答がありません"),
								CNotificationBar::MESSAGE_ERROR,6000);
		Logger.AddLog(TEXT("カードリーダーから応答がありません。"));
		return 0;

	case WM_APP_EPGLOADED:
		// EPGファイルが読み込まれた
		if (fShowPanelWindow
				&& (PanelForm.GetCurPageID()==PANEL_ID_PROGRAMLIST
				 || PanelForm.GetCurPageID()==PANEL_ID_CHANNEL)) {
			UpdatePanel();
		}
		return 0;

	case WM_APP_CONTROLLERFOCUS:
		// コントローラの操作対象が変わった
		ControllerManager.OnFocusChange(hwnd,wParam!=0);
		return 0;

	case WM_APP_PLUGINMESSAGE:
		// プラグインのメッセージの処理
		return CPlugin::OnPluginMessage(wParam,lParam);

	case WM_ACTIVATEAPP:
		{
			bool fActive=wParam!=FALSE;

			ControllerManager.OnActiveChange(hwnd,fActive);
			if (fActive)
				BroadcastControllerFocusMessage(hwnd,fActive || m_fClosing,!fActive);
		}
		return 0;

	case WM_DISPLAYCHANGE:
		CoreEngine.m_DtvEngine.m_MediaViewer.DisplayModeChanged();
		break;

	case WM_THEMECHANGED:
		ChannelMenu.Destroy();
		FavoritesMenu.Destroy();
		return 0;

	case WM_CLOSE:
		if (!ConfirmExit())
			return 0;

		m_fClosing=true;

		::SetCursor(::LoadCursor(NULL,IDC_WAIT));

		Logger.AddLog(TEXT("ウィンドウを閉じています..."));

		::KillTimer(hwnd,TIMER_ID_UPDATE);

		//CoreEngine.m_DtvEngine.EnablePreview(false);

		PluginManager.SendCloseEvent();

		m_Fullscreen.Destroy();

		ShowFloatingWindows(false);
		break;

	case WM_DESTROY:
		HtmlHelpClass.Finalize();
		m_pCore->PreventDisplaySave(false);

#ifndef _DEBUG
		// 終了監視スレッド開始(本当はこういう事はしたくないが…)
		HANDLE hEvent,hThread;
		hEvent=::CreateEvent(NULL,FALSE,FALSE,NULL);
		if (hEvent!=NULL)
			hThread=::CreateThread(NULL,0,ExitWatchThread,hEvent,0,NULL);
#endif

#ifdef NETWORK_REMOCON_SUPPORT
		SAFE_DELETE(pNetworkRemocon);
#endif
		ResidentManager.Finalize();
		ChannelMenu.Destroy();
		FavoritesMenu.Destroy();
		MainMenu.Destroy();
		Accelerator.Finalize();
		ControllerManager.DeleteAllControllers();
		TaskbarManager.Finalize();
		ProgramGuideFrame.Destroy();
		NotifyBalloonTip.Finalize();

		fEnablePlay=IsViewerEnabled();

		AppMain.SaveCurrentChannel();
		AppMain.SaveChannelSettings();

		m_PanelPaneIndex=GetPanelPaneIndex();

		CoreEngine.m_DtvEngine.SetTracer(&Logger);
		CoreEngine.Close();
		CoreEngine.m_DtvEngine.SetTracer(NULL);
		CoreEngine.m_DtvEngine.m_BonSrcDecoder.SetTracer(NULL);

		if (!CmdLineOptions.m_fNoPlugin)
			PluginOptions.StorePluginOptions();
		PluginManager.FreePlugins();

		// 終了時の負荷で他のプロセスの録画がドロップすることがあるらしい...
		::SetPriorityClass(::GetCurrentProcess(),BELOW_NORMAL_PRIORITY_CLASS);

		if (!CmdLineOptions.m_fNoEpg) {
			EpgProgramList.UpdateProgramList(CEpgProgramList::SERVICE_UPDATE_DISCARD_ENDED_EVENTS);
			EpgOptions.SaveEpgFile(&EpgProgramList);
		}
		EpgOptions.SaveLogoFile();
		EpgOptions.Finalize();

#ifndef _DEBUG
		if (hThread!=NULL) {
			if (::SignalObjectAndWait(hEvent,hThread,5000,FALSE)!=WAIT_OBJECT_0)
				::TerminateThread(hThread,-1);
			::CloseHandle(hThread);
		}
		if (hEvent!=NULL)
			::CloseHandle(hEvent);
#endif

		if (FavoritesManager.GetModified())
			FavoritesManager.Save(AppMain.GetFavoritesFileName());

		// Finalize()ではエラー時にダイアログを出すことがあるので、
		// 終了監視の外に出す必要がある
		AppMain.Finalize();
		return 0;

	default:
		/*
		if (ControllerManager.HandleMessage(hwnd,uMsg,wParam,lParam))
			return 0;
		*/
		if (ResidentManager.HandleMessage(uMsg,wParam,lParam))
			return 0;
		if (TaskbarManager.HandleMessage(uMsg,wParam,lParam))
			return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


bool CMainWindow::OnCreate(const CREATESTRUCT *pcs)
{
	RECT rc;
	GetClientRect(&rc);
	m_LayoutBase.SetPosition(&rc);
	m_LayoutBase.Create(m_hwnd,
						WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

	m_Viewer.Create(m_LayoutBase.GetHandle(),IDC_VIEW,IDC_VIDEOCONTAINER,m_hwnd);
	ViewWindowEventHandler.Initialize(&m_TitleBar,&StatusView,&SideBar);
	m_Viewer.GetViewWindow().SetEventHandler(&ViewWindowEventHandler);
	m_Viewer.GetVideoContainer().SetEventHandler(&VideoContainerEventHandler);
	m_Viewer.GetDisplayBase().SetEventHandler(&m_DisplayBaseEventHandler);

	m_TitleBar.Create(m_LayoutBase.GetHandle(),
					  WS_CHILD | WS_CLIPSIBLINGS | (m_fShowTitleBar && m_fCustomTitleBar?WS_VISIBLE:0),
					  0,IDC_TITLEBAR);
	m_TitleBar.SetEventHandler(&TitleBarUtil);
	m_TitleBar.SetLabel(pcs->lpszName);
	m_TitleBar.SetIcon(::LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON)));
	m_TitleBar.SetMaximizeMode((pcs->style&WS_MAXIMIZE)!=0);

	StatusView.Create(m_LayoutBase.GetHandle(),
		//WS_CHILD | (m_fShowStatusBar?WS_VISIBLE:0) | WS_CLIPSIBLINGS,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,0,IDC_STATUS);
	StatusView.SetEventHandler(&StatusViewEventHandler);
	StatusView.AddItem(new CChannelStatusItem);
	StatusView.AddItem(new CVideoSizeStatusItem);
	StatusView.AddItem(new CVolumeStatusItem);
	StatusView.AddItem(new CAudioChannelStatusItem);
	CRecordStatusItem *pRecordStatusItem=new CRecordStatusItem;
	pRecordStatusItem->ShowRemainTime(RecordOptions.GetShowRemainTime());
	StatusView.AddItem(pRecordStatusItem);
	StatusView.AddItem(new CCaptureStatusItem);
	StatusView.AddItem(new CErrorStatusItem);
	StatusView.AddItem(new CSignalLevelStatusItem);
	CClockStatusItem *pClockStatusItem=new CClockStatusItem;
	pClockStatusItem->SetTOT(StatusOptions.GetShowTOTTime());
	StatusView.AddItem(pClockStatusItem);
	CProgramInfoStatusItem *pProgramInfoStatusItem=new CProgramInfoStatusItem;
	pProgramInfoStatusItem->EnablePopupInfo(StatusOptions.IsPopupProgramInfoEnabled());
	StatusView.AddItem(pProgramInfoStatusItem);
	StatusView.AddItem(new CBufferingStatusItem);
	StatusView.AddItem(new CTunerStatusItem);
	StatusView.AddItem(new CMediaBitRateStatusItem);
	StatusView.AddItem(new CFavoritesStatusItem);
	StatusOptions.ApplyOptions();

	NotificationBar.Create(m_Viewer.GetVideoContainer().GetHandle(),
						   WS_CHILD | WS_CLIPSIBLINGS);

	SideBarOptions.SetEventHandler(&SideBarOptionsEventHandler);
	SideBarOptions.ApplySideBarOptions();
	SideBar.SetEventHandler(&SideBarUtil);
	SideBar.Create(m_LayoutBase.GetHandle(),
				   WS_CHILD | WS_CLIPSIBLINGS | (m_fShowSideBar?WS_VISIBLE:0),
				   0,IDC_SIDEBAR);

	Layout::CWindowContainer *pWindowContainer;
	Layout::CSplitter *pSideBarSplitter=new Layout::CSplitter(CONTAINER_ID_SIDEBARSPLITTER);
	CSideBarOptions::PlaceType SideBarPlace=SideBarOptions.GetPlace();
	bool fSideBarVertical=SideBarPlace==CSideBarOptions::PLACE_LEFT
						|| SideBarPlace==CSideBarOptions::PLACE_RIGHT;
	int SideBarWidth=SideBar.GetBarWidth();
	pSideBarSplitter->SetStyle(Layout::CSplitter::STYLE_FIXED |
		(fSideBarVertical?Layout::CSplitter::STYLE_HORZ:Layout::CSplitter::STYLE_VERT));
	pSideBarSplitter->SetVisible(true);
	pWindowContainer=new Layout::CWindowContainer(CONTAINER_ID_VIEW);
	pWindowContainer->SetWindow(&m_Viewer.GetViewWindow());
	pWindowContainer->SetMinSize(32,32);
	pWindowContainer->SetVisible(true);
	pSideBarSplitter->SetPane(0,pWindowContainer);
	pSideBarSplitter->SetAdjustPane(CONTAINER_ID_VIEW);
	pWindowContainer=new Layout::CWindowContainer(CONTAINER_ID_SIDEBAR);
	pWindowContainer->SetWindow(&SideBar);
	pWindowContainer->SetMinSize(SideBarWidth,SideBarWidth);
	pWindowContainer->SetVisible(m_fShowSideBar);
	pSideBarSplitter->SetPane(1,pWindowContainer);
	pSideBarSplitter->SetPaneSize(CONTAINER_ID_SIDEBAR,SideBarWidth);
	if (SideBarPlace==CSideBarOptions::PLACE_LEFT
			|| SideBarPlace==CSideBarOptions::PLACE_TOP)
		pSideBarSplitter->SwapPane();

	Layout::CSplitter *pTitleBarSplitter=new Layout::CSplitter(CONTAINER_ID_TITLEBARSPLITTER);
	pTitleBarSplitter->SetStyle(Layout::CSplitter::STYLE_VERT | Layout::CSplitter::STYLE_FIXED);
	pTitleBarSplitter->SetVisible(true);
	Layout::CWindowContainer *pTitleBarContainer=new Layout::CWindowContainer(CONTAINER_ID_TITLEBAR);
	pTitleBarContainer->SetWindow(&m_TitleBar);
	pTitleBarContainer->SetMinSize(0,m_TitleBar.GetHeight());
	pTitleBarContainer->SetVisible(m_fShowTitleBar && m_fCustomTitleBar);
	pTitleBarSplitter->SetPane(0,pTitleBarContainer);
	pTitleBarSplitter->SetPaneSize(CONTAINER_ID_TITLEBAR,m_TitleBar.GetHeight());

	Layout::CSplitter *pPanelSplitter=new Layout::CSplitter(CONTAINER_ID_PANELSPLITTER);
	pPanelSplitter->SetVisible(true);
	Layout::CWindowContainer *pPanelContainer=new Layout::CWindowContainer(CONTAINER_ID_PANEL);
	pPanelContainer->SetMinSize(64,0);
	pPanelSplitter->SetPane(m_PanelPaneIndex,pPanelContainer);

	Layout::CSplitter *pParentSplitter;
	if (m_fSplitTitleBar) {
		pTitleBarSplitter->SetPane(1,pSideBarSplitter);
		pTitleBarSplitter->SetAdjustPane(CONTAINER_ID_SIDEBARSPLITTER);
		pPanelSplitter->SetPane(1-m_PanelPaneIndex,pTitleBarSplitter);
		pPanelSplitter->SetAdjustPane(CONTAINER_ID_TITLEBARSPLITTER);
		pParentSplitter=pPanelSplitter;
	} else {
		pPanelSplitter->SetPane(1-m_PanelPaneIndex,pSideBarSplitter);
		pPanelSplitter->SetAdjustPane(CONTAINER_ID_SIDEBARSPLITTER);
		pTitleBarSplitter->SetPane(1,pPanelSplitter);
		pTitleBarSplitter->SetAdjustPane(CONTAINER_ID_PANELSPLITTER);
		pParentSplitter=pTitleBarSplitter;
	}

	Layout::CSplitter *pStatusSplitter=new Layout::CSplitter(CONTAINER_ID_STATUSSPLITTER);
	pStatusSplitter->SetStyle(Layout::CSplitter::STYLE_VERT | Layout::CSplitter::STYLE_FIXED);
	pStatusSplitter->SetVisible(true);
	pStatusSplitter->SetPane(0,pParentSplitter);
	pStatusSplitter->SetAdjustPane(pParentSplitter->GetID());
	pWindowContainer=new Layout::CWindowContainer(CONTAINER_ID_STATUS);
	pWindowContainer->SetWindow(&StatusView);
	pWindowContainer->SetMinSize(0,StatusView.GetHeight());
	pWindowContainer->SetVisible(m_fShowStatusBar);
	pStatusSplitter->SetPane(1,pWindowContainer);
	pStatusSplitter->SetPaneSize(CONTAINER_ID_STATUS,StatusView.GetHeight());

	m_LayoutBase.SetTopContainer(pStatusSplitter);

	// 起動状況を表示するために、起動時は常にステータスバーを表示する
	if (!m_fShowStatusBar) {
		RECT rc;

		GetClientRect(&rc);
		rc.top=rc.bottom-StatusView.GetHeight();
		StatusView.SetPosition(&rc);
		StatusView.SetVisible(true);
		::BringWindowToTop(StatusView.GetHandle());
	}
	StatusView.SetSingleText(TEXT("起動中..."));

	OSDManager.SetEventHandler(this);

	if (m_fCustomFrame) {
		CAeroGlass Aero;
		Aero.EnableNcRendering(m_hwnd,false);
		HookWindows(m_LayoutBase.GetHandle());
		//HookWindows(PanelForm.GetHandle());
	}

	// IME無効化
	::ImmAssociateContext(m_hwnd,NULL);
	::ImmAssociateContextEx(m_hwnd,NULL,IACE_CHILDREN);

	MainMenu.Create(hInst);
	/*
	MainMenu.CheckRadioItem(CM_ASPECTRATIO_FIRST,CM_ASPECTRATIO_LAST,
							CM_ASPECTRATIO_FIRST+m_AspectRatioType);
	*/
	MainMenu.CheckItem(CM_ALWAYSONTOP,m_pCore->GetAlwaysOnTop());
	int Gain,SurroundGain;
	CoreEngine.GetAudioGainControl(&Gain,&SurroundGain);
	for (int i=0;i<lengthof(g_AudioGainList);i++) {
		if (Gain==g_AudioGainList[i])
			MainMenu.CheckRadioItem(CM_AUDIOGAIN_FIRST,CM_AUDIOGAIN_LAST,
									CM_AUDIOGAIN_FIRST+i);
		if (SurroundGain==g_AudioGainList[i])
			MainMenu.CheckRadioItem(CM_SURROUNDAUDIOGAIN_FIRST,CM_SURROUNDAUDIOGAIN_LAST,
									CM_SURROUNDAUDIOGAIN_FIRST+i);
	}
	/*
	MainMenu.CheckRadioItem(CM_STEREO_THROUGH,CM_STEREO_RIGHT,
							CM_STEREO_THROUGH+m_pCore->GetStereoMode());
	*/
	MainMenu.CheckRadioItem(CM_CAPTURESIZE_FIRST,CM_CAPTURESIZE_LAST,
							CM_CAPTURESIZE_FIRST+CaptureOptions.GetPresetCaptureSize());
	//MainMenu.CheckItem(CM_CAPTUREPREVIEW,CaptureWindow.GetVisible());
	MainMenu.CheckItem(CM_DISABLEVIEWER,true);
	MainMenu.CheckItem(CM_PANEL,fShowPanelWindow);

	HMENU hSysMenu=::GetSystemMenu(m_hwnd,FALSE);
	::AppendMenu(hSysMenu,MF_SEPARATOR,0,NULL);
	::AppendMenu(hSysMenu,MF_STRING | MF_ENABLED,SC_ABOUT,TEXT("バージョン情報(&A)"));

	static const CIconMenu::ItemInfo AspectRatioMenuItems[] = {
		{CM_ASPECTRATIO_DEFAULT,	0},
		{CM_ASPECTRATIO_16x9,		1},
		{CM_ASPECTRATIO_LETTERBOX,	2},
		{CM_ASPECTRATIO_SUPERFRAME,	3},
		{CM_ASPECTRATIO_SIDECUT,	4},
		{CM_ASPECTRATIO_4x3,		5},
		{CM_ASPECTRATIO_32x9,		6},
		{CM_ASPECTRATIO_16x9_LEFT,	7},
		{CM_ASPECTRATIO_16x9_RIGHT,	8},
		{CM_FRAMECUT,				9},
		{CM_PANANDSCANOPTIONS,		10},
	};
	HMENU hmenuAspectRatio=MainMenu.GetSubMenu(CMainMenu::SUBMENU_ASPECTRATIO);
	AspectRatioIconMenu.Initialize(hmenuAspectRatio,
								   hInst,MAKEINTRESOURCE(IDB_PANSCAN),16,
								   AspectRatioMenuItems,lengthof(AspectRatioMenuItems));
	if (m_AspectRatioType<ASPECTRATIO_CUSTOM) {
		AspectRatioIconMenu.CheckRadioItem(
			CM_ASPECTRATIO_FIRST,CM_ASPECTRATIO_3D_LAST,
			CM_ASPECTRATIO_FIRST+m_AspectRatioType);
	}
	m_DefaultAspectRatioMenuItemCount=::GetMenuItemCount(hmenuAspectRatio);

	TaskbarManager.Initialize(m_hwnd);

	NotifyBalloonTip.Initialize(m_hwnd);

	CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(
		(pcs->style&WS_MAXIMIZE)!=0?ViewOptions.GetMaximizeStretchMode():
						m_fFrameCut?CMediaViewer::STRETCH_CUTFRAME:
									CMediaViewer::STRETCH_KEEPASPECTRATIO);

	::SetTimer(m_hwnd,TIMER_ID_UPDATE,UPDATE_TIMER_INTERVAL,NULL);

	return true;
}


void CMainWindow::OnSizeChanged(UINT State,int Width,int Height)
{
	const bool fMinimized=State==SIZE_MINIMIZED;
	const bool fMaximized=State==SIZE_MAXIMIZED;

	if (fMinimized) {
		OSDManager.ClearOSD();
		OSDManager.Reset();
		ResidentManager.SetStatus(CResidentManager::STATUS_MINIMIZED,
								  CResidentManager::STATUS_MINIMIZED);
		if (ViewOptions.GetDisablePreviewWhenMinimized()) {
			SuspendViewer(ResumeInfo::VIEWERSUSPEND_MINIMIZE);
		}
	} else if ((ResidentManager.GetStatus()&CResidentManager::STATUS_MINIMIZED)!=0) {
		SetWindowVisible();
	}

	if (fMaximized && (!m_fShowTitleBar || m_fCustomTitleBar)) {
		HMONITOR hMonitor=::MonitorFromWindow(m_hwnd,MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi;

		mi.cbSize=sizeof(MONITORINFO);
		::GetMonitorInfo(hMonitor,&mi);
		SetPosition(&mi.rcWork);
		SIZE sz;
		GetClientSize(&sz);
		Width=sz.cx;
		Height=sz.cy;
	}
	m_TitleBar.SetMaximizeMode(fMaximized);

	// ウィンドウ枠を細くしていると最小化時に変なサイズにされる
	if (fMinimized)
		return;

	m_LayoutBase.SetPosition(0,0,Width,Height);

	if (!m_pCore->GetFullscreen()) {
		if (State==SIZE_MAXIMIZED)
			CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(
										ViewOptions.GetMaximizeStretchMode());
		else if (State==SIZE_RESTORED)
			CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(
							m_fFrameCut?CMediaViewer::STRETCH_CUTFRAME:
										CMediaViewer::STRETCH_KEEPASPECTRATIO);
	}

	StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
	ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VIDEO);
}


bool CMainWindow::OnSizeChanging(UINT Edge,RECT *pRect)
{
	RECT rcOld;
	bool fChanged=false;

	GetPosition(&rcOld);
	bool fKeepRatio=ViewOptions.GetAdjustAspectResizing();
	if (::GetKeyState(VK_SHIFT)<0)
		fKeepRatio=!fKeepRatio;
	if (fKeepRatio) {
		BYTE XAspect,YAspect;

		if (CoreEngine.m_DtvEngine.m_MediaViewer.GetEffectiveAspectRatio(
														&XAspect,&YAspect)) {
			RECT rcWindow,rcClient;
			int XMargin,YMargin,Width,Height;

			GetPosition(&rcWindow);
			GetClientRect(&rcClient);
			m_Viewer.GetViewWindow().CalcClientRect(&rcClient);
			if (m_fShowStatusBar)
				rcClient.bottom-=StatusView.GetHeight();
			if (m_fShowTitleBar && m_fCustomTitleBar)
				TitleBarUtil.AdjustArea(&rcClient);
			if (m_fShowSideBar)
				SideBarUtil.AdjustArea(&rcClient);
			if (fShowPanelWindow && !PanelFrame.GetFloating()) {
				Layout::CSplitter *pSplitter=dynamic_cast<Layout::CSplitter*>(
					m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));
				rcClient.right-=pSplitter->GetPaneSize(CONTAINER_ID_PANEL)+pSplitter->GetBarWidth();
			}
			::OffsetRect(&rcClient,-rcClient.left,-rcClient.top);
			if (rcClient.right<=0 || rcClient.bottom<=0)
				goto SizingEnd;
			XMargin=(rcWindow.right-rcWindow.left)-rcClient.right;
			YMargin=(rcWindow.bottom-rcWindow.top)-rcClient.bottom;
			Width=(pRect->right-pRect->left)-XMargin;
			Height=(pRect->bottom-pRect->top)-YMargin;
			if (Width<=0 || Height<=0)
				goto SizingEnd;
			if (Edge==WMSZ_LEFT || Edge==WMSZ_RIGHT)
				Height=Width*YAspect/XAspect;
			else if (Edge==WMSZ_TOP || Edge==WMSZ_BOTTOM)
				Width=Height*XAspect/YAspect;
			else if (Width*YAspect<Height*XAspect)
				Width=Height*XAspect/YAspect;
			else if (Width*YAspect>Height*XAspect)
				Height=Width*YAspect/XAspect;
			if (Edge==WMSZ_LEFT || Edge==WMSZ_TOPLEFT || Edge==WMSZ_BOTTOMLEFT)
				pRect->left=pRect->right-(Width+XMargin);
			else
				pRect->right=pRect->left+Width+XMargin;
			if (Edge==WMSZ_TOP || Edge==WMSZ_TOPLEFT || Edge==WMSZ_TOPRIGHT)
				pRect->top=pRect->bottom-(Height+YMargin);
			else
				pRect->bottom=pRect->top+Height+YMargin;
			fChanged=true;
		}
	}
SizingEnd:
	PanelEventHandler.OnOwnerMovingOrSizing(&rcOld,pRect);
	return fChanged;
}


void CMainWindow::OnMouseMove(int x,int y)
{
	if (::GetCapture()==m_hwnd) {
		// ウィンドウ移動中
		POINT pt;
		RECT rcOld,rc;

		/*
		pt.x=x;
		pt.y=y;
		::ClientToScreen(hwnd,&pt);
		*/
		::GetCursorPos(&pt);
		::GetWindowRect(m_hwnd,&rcOld);
		rc.left=m_rcDragStart.left+(pt.x-m_ptDragStartPos.x);
		rc.top=m_rcDragStart.top+(pt.y-m_ptDragStartPos.y);
		rc.right=rc.left+(m_rcDragStart.right-m_rcDragStart.left);
		rc.bottom=rc.top+(m_rcDragStart.bottom-m_rcDragStart.top);
		bool fSnap=ViewOptions.GetSnapAtWindowEdge();
		if (::GetKeyState(VK_SHIFT)<0)
			fSnap=!fSnap;
		if (fSnap)
			SnapWindow(m_hwnd,&rc,
					   ViewOptions.GetSnapAtWindowEdgeMargin(),
					   PanelEventHandler.IsAttached()?NULL:PanelFrame.GetHandle());
		SetPosition(&rc);
		PanelEventHandler.OnOwnerMovingOrSizing(&rcOld,&rc);
	} else if (!m_pCore->GetFullscreen()) {
		POINT pt={x,y};
		RECT rcClient,rcTitle,rcStatus,rcSideBar,rc;
		bool fShowTitleBar=false,fShowStatusBar=false,fShowSideBar=false;

		m_Viewer.GetViewWindow().GetClientRect(&rcClient);
		MapWindowRect(m_Viewer.GetViewWindow().GetHandle(),m_LayoutBase.GetHandle(),&rcClient);
		if (!m_fShowTitleBar && m_fPopupTitleBar) {
			rc=rcClient;
			TitleBarUtil.Layout(&rc,&rcTitle);
			if (::PtInRect(&rcTitle,pt))
				fShowTitleBar=true;
		}
		if (!m_fShowStatusBar && m_fPopupStatusBar) {
			rcStatus=rcClient;
			rcStatus.top=rcStatus.bottom-StatusView.CalcHeight(rcClient.right-rcClient.left);
			if (::PtInRect(&rcStatus,pt))
				fShowStatusBar=true;
		}
		if (!m_fShowSideBar && SideBarOptions.ShowPopup()) {
			switch (SideBarOptions.GetPlace()) {
			case CSideBarOptions::PLACE_LEFT:
			case CSideBarOptions::PLACE_RIGHT:
				if (!fShowStatusBar && !fShowTitleBar) {
					SideBarUtil.Layout(&rcClient,&rcSideBar);
					if (::PtInRect(&rcSideBar,pt))
						fShowSideBar=true;
				}
				break;
			case CSideBarOptions::PLACE_TOP:
				if (!m_fShowTitleBar && m_fPopupTitleBar)
					rcClient.top=rcTitle.bottom;
				SideBarUtil.Layout(&rcClient,&rcSideBar);
				if (::PtInRect(&rcSideBar,pt)) {
					fShowSideBar=true;
					if (!m_fShowTitleBar && m_fPopupTitleBar)
						fShowTitleBar=true;
				}
				break;
			case CSideBarOptions::PLACE_BOTTOM:
				if (!m_fShowStatusBar && m_fPopupStatusBar)
					rcClient.bottom=rcStatus.top;
				SideBarUtil.Layout(&rcClient,&rcSideBar);
				if (::PtInRect(&rcSideBar,pt)) {
					fShowSideBar=true;
					if (!m_fShowStatusBar && m_fPopupStatusBar)
						fShowStatusBar=true;
				}
				break;
			}
		}

		if (fShowTitleBar) {
			if (!m_TitleBar.GetVisible()) {
				m_TitleBar.SetPosition(&rcTitle);
				m_TitleBar.SetVisible(true);
				::BringWindowToTop(m_TitleBar.GetHandle());
			}
		} else if (!m_fShowTitleBar && m_TitleBar.GetVisible()) {
			m_TitleBar.SetVisible(false);
		}
		if (fShowStatusBar) {
			if (!StatusView.GetVisible()) {
				StatusView.SetPosition(&rcStatus);
				StatusView.SetVisible(true);
				::BringWindowToTop(StatusView.GetHandle());
			}
		} else if (!m_fShowStatusBar && StatusView.GetVisible()) {
			StatusView.SetVisible(false);
		}
		if (fShowSideBar) {
			if (!SideBar.GetVisible()) {
				SideBar.SetPosition(&rcSideBar);
				SideBar.SetVisible(true);
				::BringWindowToTop(SideBar.GetHandle());
			}
		} else if (!m_fShowSideBar && SideBar.GetVisible()) {
			SideBar.SetVisible(false);
		}
	} else {
		m_Fullscreen.OnMouseMove();
	}
}


void CMainWindow::OnCommand(HWND hwnd,int id,HWND hwndCtl,UINT codeNotify)
{
	switch (id) {
	case CM_ZOOMOPTIONS:
		if (ZoomOptions.Show(GetVideoHostWindow())) {
			SideBarOptions.SetSideBarImage();
			ZoomOptions.SaveSettings(AppMain.GetIniFileName());
		}
		return;

	case CM_ASPECTRATIO:
		{
			int Command;

			if (m_AspectRatioType>=ASPECTRATIO_CUSTOM)
				Command=CM_ASPECTRATIO_DEFAULT;
			else if (m_AspectRatioType<ASPECTRATIO_32x9)
				Command=CM_ASPECTRATIO_FIRST+
					(m_AspectRatioType+1)%(CM_ASPECTRATIO_LAST-CM_ASPECTRATIO_FIRST+1);
			else
				Command=CM_ASPECTRATIO_3D_FIRST+
					(m_AspectRatioType-ASPECTRATIO_32x9+1)%(CM_ASPECTRATIO_3D_LAST-CM_ASPECTRATIO_3D_FIRST+1);
			SetPanAndScan(Command);
		}
		return;

	case CM_ASPECTRATIO_DEFAULT:
	case CM_ASPECTRATIO_16x9:
	case CM_ASPECTRATIO_LETTERBOX:
	case CM_ASPECTRATIO_SUPERFRAME:
	case CM_ASPECTRATIO_SIDECUT:
	case CM_ASPECTRATIO_4x3:
	case CM_ASPECTRATIO_32x9:
	case CM_ASPECTRATIO_16x9_LEFT:
	case CM_ASPECTRATIO_16x9_RIGHT:
		SetPanAndScan(id);
		return;

	case CM_PANANDSCANOPTIONS:
		{
			bool fSet=false;
			CPanAndScanOptions::PanAndScanInfo CurPanScan;

			if (m_AspectRatioType>=ASPECTRATIO_CUSTOM)
				fSet=PanAndScanOptions.GetPreset(m_AspectRatioType-ASPECTRATIO_CUSTOM,&CurPanScan);

			if (PanAndScanOptions.Show(GetVideoHostWindow())) {
				if (fSet) {
					CPanAndScanOptions::PanAndScanInfo NewPanScan;
					int Index=PanAndScanOptions.FindPresetByID(CurPanScan.ID);
					if (Index>=0 && PanAndScanOptions.GetPreset(Index,&NewPanScan)) {
						if (NewPanScan.Info!=CurPanScan.Info)
							SetPanAndScan(CM_PANANDSCAN_PRESET_FIRST+Index);
					} else {
						SetPanAndScan(CM_ASPECTRATIO_DEFAULT);
					}
				}
				PanAndScanOptions.SaveSettings(AppMain.GetIniFileName());
			}
		}
		return;

	case CM_FRAMECUT:
		m_fFrameCut=!m_fFrameCut;
		CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(
						m_fFrameCut?CMediaViewer::STRETCH_CUTFRAME:
									CMediaViewer::STRETCH_KEEPASPECTRATIO);
		return;

	case CM_FULLSCREEN:
		m_pCore->ToggleFullscreen();
		return;

	case CM_ALWAYSONTOP:
		m_pCore->SetAlwaysOnTop(!m_pCore->GetAlwaysOnTop());
		return;

	case CM_VOLUME_UP:
	case CM_VOLUME_DOWN:
		{
			const int CurVolume=m_pCore->GetVolume();
			int Volume=CurVolume;

			if (id==CM_VOLUME_UP) {
				Volume+=OperationOptions.GetVolumeStep();
				if (Volume>CCoreEngine::MAX_VOLUME)
					Volume=CCoreEngine::MAX_VOLUME;
			} else {
				Volume-=OperationOptions.GetVolumeStep();
				if (Volume<0)
					Volume=0;
			}
			if (Volume!=CurVolume || m_pCore->GetMute())
				m_pCore->SetVolume(Volume);
		}
		return;

	case CM_VOLUME_MUTE:
		m_pCore->SetMute(!m_pCore->GetMute());
		return;

	case CM_AUDIOGAIN_NONE:
	case CM_AUDIOGAIN_125:
	case CM_AUDIOGAIN_150:
	case CM_AUDIOGAIN_200:
		{
			int SurroundGain;

			CoreEngine.GetAudioGainControl(NULL,&SurroundGain);
			CoreEngine.SetAudioGainControl(
				g_AudioGainList[id-CM_AUDIOGAIN_FIRST],SurroundGain);
			MainMenu.CheckRadioItem(CM_AUDIOGAIN_NONE,CM_AUDIOGAIN_LAST,id);
		}
		return;

	case CM_SURROUNDAUDIOGAIN_NONE:
	case CM_SURROUNDAUDIOGAIN_125:
	case CM_SURROUNDAUDIOGAIN_150:
	case CM_SURROUNDAUDIOGAIN_200:
		{
			int Gain;

			CoreEngine.GetAudioGainControl(&Gain,NULL);
			CoreEngine.SetAudioGainControl(
				Gain,g_AudioGainList[id-CM_SURROUNDAUDIOGAIN_FIRST]);
			MainMenu.CheckRadioItem(CM_SURROUNDAUDIOGAIN_NONE,CM_SURROUNDAUDIOGAIN_LAST,id);
		}
		return;

	case CM_STEREO_THROUGH:
	case CM_STEREO_LEFT:
	case CM_STEREO_RIGHT:
		m_pCore->SetStereoMode(id-CM_STEREO_THROUGH);
		ShowAudioOSD();
		return;

	case CM_SWITCHAUDIO:
		m_pCore->SwitchAudio();
		ShowAudioOSD();
		return;

	case CM_SPDIF_DISABLED:
	case CM_SPDIF_PASSTHROUGH:
	case CM_SPDIF_AUTO:
		{
			CAudioDecFilter::SpdifOptions Options(PlaybackOptions.GetSpdifOptions());

			Options.Mode=(CAudioDecFilter::SpdifMode)(id-CM_SPDIF_DISABLED);
			CoreEngine.SetSpdifOptions(Options);
		}
		return;

	case CM_SPDIF_TOGGLE:
		{
			CAudioDecFilter::SpdifOptions Options(PlaybackOptions.GetSpdifOptions());

			if (CoreEngine.m_DtvEngine.m_MediaViewer.IsSpdifPassthrough())
				Options.Mode=CAudioDecFilter::SPDIF_MODE_DISABLED;
			else
				Options.Mode=CAudioDecFilter::SPDIF_MODE_PASSTHROUGH;
			CoreEngine.SetSpdifOptions(Options);
			SideBar.CheckItem(CM_SPDIF_TOGGLE,
							  Options.Mode==CAudioDecFilter::SPDIF_MODE_PASSTHROUGH);
		}
		return;

	case CM_CAPTURE:
		SendCommand(CaptureOptions.TranslateCommand(CM_CAPTURE));
		return;

	case CM_COPY:
	case CM_SAVEIMAGE:
		if (IsViewerEnabled()) {
			HCURSOR hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));
			BYTE *pDib;

			pDib=static_cast<BYTE*>(CoreEngine.GetCurrentImage());
			if (pDib==NULL) {
				::SetCursor(hcurOld);
				ShowMessage(TEXT("現在の画像を取得できません。\n")
							TEXT("レンダラやデコーダを変えてみてください。"),TEXT("ごめん"),
							MB_OK | MB_ICONEXCLAMATION);
				return;
			}
			{
				BITMAPINFOHEADER *pbmih=(BITMAPINFOHEADER*)pDib;
				RECT rc;
				int Width,Height,OrigWidth,OrigHeight;
				HGLOBAL hGlobal=NULL;

				OrigWidth=pbmih->biWidth;
				OrigHeight=abs(pbmih->biHeight);
				if (CoreEngine.m_DtvEngine.m_MediaViewer.GetSourceRect(&rc)) {
					WORD VideoWidth,VideoHeight;

					if (CoreEngine.m_DtvEngine.m_MediaViewer.GetOriginalVideoSize(
													&VideoWidth,&VideoHeight)
							&& (VideoWidth!=OrigWidth
								|| VideoHeight!=OrigHeight)) {
						rc.left=rc.left*OrigWidth/VideoWidth;
						rc.top=rc.top*OrigHeight/VideoHeight;
						rc.right=rc.right*OrigWidth/VideoWidth;
						rc.bottom=rc.bottom*OrigHeight/VideoHeight;
					}
					if (rc.right>OrigWidth)
						rc.right=OrigWidth;
					if (rc.bottom>OrigHeight)
						rc.bottom=OrigHeight;
				} else {
					rc.left=0;
					rc.top=0;
					rc.right=OrigWidth;
					rc.bottom=OrigHeight;
				}
				if (OrigHeight==1088) {
					rc.top=rc.top*1080/1088;
					rc.bottom=rc.bottom*1080/1088;
				}
				switch (CaptureOptions.GetCaptureSizeType()) {
				case CCaptureOptions::SIZE_TYPE_ORIGINAL:
					CoreEngine.GetVideoViewSize(&Width,&Height);
					break;
				case CCaptureOptions::SIZE_TYPE_VIEW:
					{
						WORD w,h;

						CoreEngine.m_DtvEngine.m_MediaViewer.GetDestSize(&w,&h);
						Width=w;
						Height=h;
					}
					break;
				/*
				case CCaptureOptions::SIZE_RAW:
					rc.left=rc.top=0;
					rc.right=OrigWidth;
					rc.bottom=OrigHeight;
					Width=OrigWidth;
					Height=OrigHeight;
					break;
				*/
				case CCaptureOptions::SIZE_TYPE_PERCENTAGE:
					{
						int Num,Denom;

						CoreEngine.GetVideoViewSize(&Width,&Height);
						CaptureOptions.GetSizePercentage(&Num,&Denom);
						Width=Width*Num/Denom;
						Height=Height*Num/Denom;
					}
					break;
				case CCaptureOptions::SIZE_TYPE_CUSTOM:
					CaptureOptions.GetCustomSize(&Width,&Height);
					break;
				}
				hGlobal=ResizeImage((BITMAPINFO*)pbmih,
								pDib+CalcDIBInfoSize(pbmih),&rc,Width,Height);
				CoTaskMemFree(pDib);
				::SetCursor(hcurOld);
				if (hGlobal==NULL) {
					return;
				}
				CCaptureImage *pImage=new CCaptureImage(hGlobal);
				const CChannelInfo *pChInfo=ChannelManager.GetCurrentChannelInfo();
				TCHAR szComment[512],szEventName[256];
				CaptureOptions.GetCommentText(szComment,lengthof(szComment),
					pChInfo!=NULL?pChInfo->GetName():NULL,
					CoreEngine.m_DtvEngine.GetEventName(szEventName,lengthof(szEventName))>0?szEventName:NULL);
				pImage->SetComment(szComment);
				CaptureWindow.SetImage(pImage);
				if (id==CM_COPY) {
					if (!pImage->SetClipboard(hwnd)) {
						ShowErrorMessage(TEXT("クリップボードにデータを設定できません。"));
					}
				} else {
					if (!CaptureOptions.SaveImage(pImage)) {
						ShowErrorMessage(TEXT("画像の保存でエラーが発生しました。"));
					}
				}
				if (!CaptureWindow.HasImage())
					delete pImage;
			}
		}
		return;

	case CM_CAPTUREPREVIEW:
		{
			if (!CaptureWindow.GetVisible()) {
				if (!CaptureWindow.IsCreated()) {
					CaptureWindow.Create(hwnd,
						WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |
							WS_VISIBLE | WS_CLIPCHILDREN,
						WS_EX_TOOLWINDOW);
				} else {
					CaptureWindow.SetVisible(true);
				}
			} else {
				CaptureWindow.Destroy();
				CaptureWindow.ClearImage();
			}

			const bool fVisible=CaptureWindow.GetVisible();
			MainMenu.CheckItem(CM_CAPTUREPREVIEW,fVisible);
			SideBar.CheckItem(CM_CAPTUREPREVIEW,fVisible);
		}
		return;

	case CM_CAPTUREOPTIONS:
		if (IsWindowEnabled(hwnd))
			ShowOptionDialog(hwnd,COptionDialog::PAGE_CAPTURE);
		return;

	case CM_OPENCAPTUREFOLDER:
		CaptureOptions.OpenSaveFolder();
		return;

	case CM_RESET:
		CoreEngine.m_DtvEngine.ResetEngine();
		PluginManager.SendResetEvent();
		return;

	case CM_RESETVIEWER:
		CoreEngine.m_DtvEngine.ResetMediaViewer();
		return;

	case CM_REBUILDVIEWER:
		InitializeViewer();
		return;

	case CM_RECORD:
	case CM_RECORD_START:
	case CM_RECORD_STOP:
		if (id==CM_RECORD) {
			if (RecordManager.IsPaused()) {
				SendCommand(CM_RECORD_PAUSE);
				return;
			}
		} else if (id==CM_RECORD_START) {
			if (RecordManager.IsRecording()) {
				if (RecordManager.IsPaused())
					SendCommand(CM_RECORD_PAUSE);
				return;
			}
		} else if (id==CM_RECORD_STOP) {
			if (!RecordManager.IsRecording())
				return;
		}
		if (RecordManager.IsRecording()) {
			if (!RecordManager.IsPaused()
					&& !RecordOptions.ConfirmStop(GetVideoHostWindow()))
				return;
			AppMain.StopRecord();
		} else {
			if (RecordManager.IsReserved()) {
				if (ShowMessage(
						TEXT("既に設定されている録画があります。\n")
						TEXT("録画を開始すると既存の設定が破棄されます。\n")
						TEXT("録画を開始してもいいですか?"),
						TEXT("録画開始の確認"),
						MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2)!=IDOK) {
					return;
				}
			}
			AppMain.StartRecord();
		}
		return;

	case CM_RECORD_PAUSE:
		if (RecordManager.IsRecording()) {
			RecordManager.PauseRecord();
			StatusView.UpdateItem(STATUS_ITEM_RECORD);
			Logger.AddLog(RecordManager.IsPaused()?TEXT("録画一時停止"):TEXT("録画再開"));
			PluginManager.SendRecordStatusChangeEvent();
		}
		return;

	case CM_RECORDOPTION:
		if (IsWindowEnabled(GetVideoHostWindow())) {
			if (RecordManager.IsRecording()) {
				if (RecordManager.RecordDialog(GetVideoHostWindow()))
					StatusView.UpdateItem(STATUS_ITEM_RECORD);
			} else {
				if (RecordManager.GetFileName()==NULL) {
					TCHAR szFileName[MAX_PATH];

					if (RecordOptions.GetFilePath(szFileName,MAX_PATH))
						RecordManager.SetFileName(szFileName);
				}
				if (!RecordManager.IsReserved())
					RecordOptions.ApplyOptions(&RecordManager);
				if (RecordManager.RecordDialog(GetVideoHostWindow())) {
					RecordManager.SetClient(CRecordManager::CLIENT_USER);
					if (RecordManager.IsReserved()) {
						StatusView.UpdateItem(STATUS_ITEM_RECORD);
					} else {
						AppMain.StartReservedRecord();
					}
				} else {
					// 予約がキャンセルされた場合も表示を更新する
					StatusView.UpdateItem(STATUS_ITEM_RECORD);
				}
			}
		}
		return;

	case CM_RECORDEVENT:
		if (RecordManager.IsRecording()) {
			RecordManager.SetStopOnEventEnd(!RecordManager.GetStopOnEventEnd());
		} else {
			SendCommand(CM_RECORD_START);
			if (RecordManager.IsRecording())
				RecordManager.SetStopOnEventEnd(true);
		}
		return;

	case CM_EXITONRECORDINGSTOP:
		m_fExitOnRecordingStop=!m_fExitOnRecordingStop;
		return;

	case CM_OPTIONS_RECORD:
		if (IsWindowEnabled(hwnd))
			ShowOptionDialog(hwnd,COptionDialog::PAGE_RECORD);
		return;

	case CM_TIMESHIFTRECORDING:
		if (!RecordManager.IsRecording()) {
			if (RecordManager.IsReserved()) {
				if (ShowMessage(
						TEXT("既に設定されている録画があります。\n")
						TEXT("録画を開始すると既存の設定が破棄されます。\n")
						TEXT("録画を開始してもいいですか?"),
						TEXT("録画開始の確認"),
						MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2)!=IDOK) {
					return;
				}
			}
			AppMain.StartRecord(NULL,NULL,NULL,CRecordManager::CLIENT_USER,true);
		}
		return;

	case CM_ENABLETIMESHIFTRECORDING:
		RecordOptions.EnableTimeShiftRecording(!RecordOptions.IsTimeShiftRecordingEnabled());
		return;

	case CM_STATUSBARRECORD:
		{
			int Command=RecordOptions.GetStatusBarRecordCommand();
			if (Command!=0)
				OnCommand(hwnd,Command,NULL,0);
		}
		return;

	case CM_DISABLEVIEWER:
		m_pCore->EnableViewer(!IsViewerEnabled());
		return;

	case CM_PANEL:
		if (m_pCore->GetFullscreen()) {
			m_Fullscreen.ShowPanel(!m_Fullscreen.IsPanelVisible());
			return;
		}
		fShowPanelWindow=!fShowPanelWindow;
		if (fShowPanelWindow) {
			PanelFrame.SetPanelVisible(true);
		} else {
			PanelFrame.SetPanelVisible(false);
			InfoPanel.ResetStatistics();
			//ProgramListPanel.ClearProgramList();
			ChannelPanel.ClearChannelList();
		}
		if (!PanelFrame.GetFloating()) {
			// パネルの幅に合わせてウィンドウサイズを拡縮
			Layout::CSplitter *pSplitter=dynamic_cast<Layout::CSplitter*>(m_LayoutBase.GetContainerByID(CONTAINER_ID_PANELSPLITTER));
			const int Width=PanelFrame.GetDockingWidth()+pSplitter->GetBarWidth();
			RECT rc;

			GetPosition(&rc);
			if (pSplitter->GetPane(0)->GetID()==CONTAINER_ID_PANEL) {
				if (fShowPanelWindow)
					rc.left-=Width;
				else
					rc.left+=Width;
			} else {
				if (fShowPanelWindow)
					rc.right+=Width;
				else
					rc.right-=Width;
			}
			SetPosition(&rc);
			if (!fShowPanelWindow)
				::SetFocus(hwnd);
		}
		if (fShowPanelWindow)
			UpdatePanel();
		MainMenu.CheckItem(CM_PANEL,fShowPanelWindow);
		SideBar.CheckItem(CM_PANEL,fShowPanelWindow);
		return;

	case CM_PROGRAMGUIDE:
		ShowProgramGuide(!fShowProgramGuide);
		return;

	case CM_STATUSBAR:
		SetStatusBarVisible(!m_fShowStatusBar);
		return;

	case CM_TITLEBAR:
		SetTitleBarVisible(!m_fShowTitleBar);
		return;

	case CM_SIDEBAR:
		SetSideBarVisible(!m_fShowSideBar);
		return;

	case CM_WINDOWFRAME_NORMAL:
		SetCustomFrame(false);
		return;

	case CM_WINDOWFRAME_CUSTOM:
		SetCustomFrame(true,m_ThinFrameWidth);
		return;

	case CM_WINDOWFRAME_NONE:
		SetCustomFrame(true,0);
		return;

	case CM_CUSTOMTITLEBAR:
		SetCustomTitleBar(!m_fCustomTitleBar);
		return;

	case CM_SPLITTITLEBAR:
		SetSplitTitleBar(!m_fSplitTitleBar);
		return;

	case CM_VIDEOEDGE:
		SetViewWindowEdge(!m_fViewWindowEdge);
		return;

	case CM_VIDEODECODERPROPERTY:
	case CM_VIDEORENDERERPROPERTY:
	case CM_AUDIOFILTERPROPERTY:
	case CM_AUDIORENDERERPROPERTY:
	case CM_DEMULTIPLEXERPROPERTY:
		{
			HWND hwndOwner=GetVideoHostWindow();

			if (hwndOwner==NULL || ::IsWindowEnabled(hwndOwner)) {
				for (int i=0;i<lengthof(g_DirectShowFilterPropertyList);i++) {
					if (g_DirectShowFilterPropertyList[i].Command==id) {
						CoreEngine.m_DtvEngine.m_MediaViewer.DisplayFilterProperty(
							g_DirectShowFilterPropertyList[i].Filter,hwndOwner);
						break;
					}
				}
			}
		}
		return;

	case CM_OPTIONS:
		{
			HWND hwndOwner=GetVideoHostWindow();

			if (hwndOwner==NULL || IsWindowEnabled(hwndOwner))
				ShowOptionDialog(hwndOwner);
		}
		return;

	case CM_STREAMINFO:
		{
			if (!StreamInfo.IsVisible()) {
				if (!StreamInfo.IsCreated())
					StreamInfo.Create(hwnd);
				else
					StreamInfo.SetVisible(true);
			} else {
				StreamInfo.Destroy();
			}

			const bool fVisible=StreamInfo.IsVisible();
			MainMenu.CheckItem(CM_STREAMINFO,fVisible);
			SideBar.CheckItem(CM_STREAMINFO,fVisible);
		}
		return;

	case CM_CLOSE:
		if (m_pCore->GetStandby()) {
			m_pCore->SetStandby(false);
		} else if (ResidentManager.GetResident()) {
			m_pCore->SetStandby(true);
		} else {
			PostMessage(WM_CLOSE,0,0);
		}
		return;

	case CM_EXIT:
		PostMessage(WM_CLOSE,0,0);
		return;

	case CM_SHOW:
		if (m_pCore->GetStandby()) {
			m_pCore->SetStandby(false);
		} else {
			SetWindowVisible();
		}
		return;

	case CM_CHANNEL_UP:
	case CM_CHANNEL_DOWN:
		{
			const CChannelInfo *pInfo=ChannelManager.GetNextChannelInfo(id==CM_CHANNEL_UP);

			if (pInfo!=NULL) {
				const CChannelList *pList=ChannelManager.GetCurrentChannelList();

				if (pList->HasRemoteControlKeyID() && pInfo->GetChannelNo()!=0)
					SendCommand(CM_CHANNELNO_FIRST+pInfo->GetChannelNo()-1);
				else
					SendCommand(CM_CHANNEL_FIRST+pInfo->GetChannelIndex());
			} else {
				SendCommand(CM_CHANNEL_FIRST);
			}
		}
		return;

	case CM_CHANNEL_BACKWARD:
	case CM_CHANNEL_FORWARD:
		{
			const CChannelHistory::CChannel *pChannel;

			if (id==CM_CHANNEL_BACKWARD)
				pChannel=ChannelHistory.Backward();
			else
				pChannel=ChannelHistory.Forward();
			if (pChannel!=NULL) {
				AppMain.OpenTunerAndSetChannel(pChannel->GetDriverFileName(),pChannel);
			}
		}
		return;

#ifdef _DEBUG
	case CM_UPDATECHANNELLIST:
		// チャンネルリストの自動更新(現状役には立たない)
		//if (DriverOptions.IsChannelAutoUpdate(CoreEngine.GetDriverFileName()))
		{
			CTuningSpaceList TuningSpaceList(*ChannelManager.GetTuningSpaceList());
			std::vector<TVTest::String> MessageList;

			TRACE(TEXT("チャンネルリスト自動更新開始\n"));
			if (ChannelScan.AutoUpdateChannelList(&TuningSpaceList,&MessageList)) {
				AppMain.AddLog(TEXT("チャンネルリストの自動更新を行いました。"));
				for (size_t i=0;i<MessageList.size();i++)
					AppMain.AddLog(TEXT("%s"),MessageList[i].c_str());

				TuningSpaceList.MakeAllChannelList();
				AppMain.UpdateCurrentChannelList(&TuningSpaceList);

				TCHAR szFileName[MAX_PATH];
				if (!AppMain.GetChannelManager()->GetChannelFileName(szFileName,lengthof(szFileName))
						|| ::lstrcmpi(::PathFindExtension(szFileName),CHANNEL_FILE_EXTENSION)!=0
						|| !::PathFileExists(szFileName)) {
					AppMain.GetCoreEngine()->GetDriverPath(szFileName,lengthof(szFileName));
					::PathRenameExtension(szFileName,CHANNEL_FILE_EXTENSION);
				}
				if (TuningSpaceList.SaveToFile(szFileName))
					AppMain.AddLog(TEXT("チャンネルファイルを \"%s\" に保存しました。"),szFileName);
				else
					AppMain.AddLog(TEXT("チャンネルファイル \"%s\" を保存できません。"),szFileName);
			}
		}
		return;
#endif

	case CM_MENU:
		{
			POINT pt;
			bool fDefault=false;

			if (codeNotify==COMMAND_FROM_MOUSE) {
				::GetCursorPos(&pt);
				if (::GetKeyState(VK_SHIFT)<0)
					fDefault=true;
			} else {
				pt.x=0;
				pt.y=0;
				::ClientToScreen(m_Viewer.GetViewWindow().GetHandle(),&pt);
			}
			m_pCore->PopupMenu(&pt,fDefault?CUICore::POPUPMENU_DEFAULT:0);
		}
		return;

	case CM_ACTIVATE:
		{
			HWND hwndHost=GetVideoHostWindow();

			if (hwndHost!=NULL)
				ForegroundWindow(hwndHost);
		}
		return;

	case CM_MINIMIZE:
		::ShowWindow(hwnd,::IsIconic(hwnd)?SW_RESTORE:SW_MINIMIZE);
		return;

	case CM_MAXIMIZE:
		::ShowWindow(hwnd,::IsZoomed(hwnd)?SW_RESTORE:SW_MAXIMIZE);
		return;

	case CM_HOMEDISPLAY:
		if (!HomeDisplay.GetVisible()) {
			Util::CWaitCursor WaitCursor;

			HomeDisplay.SetFont(OSDOptions.GetDisplayFont(),
								OSDOptions.IsDisplayFontAutoSize());
			if (!HomeDisplay.IsCreated()) {
				HomeDisplay.SetEventHandler(&HomeDisplayEventHandler);
				HomeDisplay.Create(m_Viewer.GetDisplayBase().GetParent()->GetHandle(),
								   WS_CHILD | WS_CLIPCHILDREN);
				if (m_fCustomFrame)
					HookWindows(HomeDisplay.GetHandle());
			}
			HomeDisplay.UpdateContents();
			m_Viewer.GetDisplayBase().SetDisplayView(&HomeDisplay);
			m_Viewer.GetDisplayBase().SetVisible(true);
			HomeDisplay.Update();
		} else {
			m_Viewer.GetDisplayBase().SetVisible(false);
		}
		return;

	case CM_CHANNELDISPLAY:
		if (!ChannelDisplay.GetVisible()) {
			Util::CWaitCursor WaitCursor;

			ChannelDisplay.SetFont(OSDOptions.GetDisplayFont(),
								   OSDOptions.IsDisplayFontAutoSize());
			if (!ChannelDisplay.IsCreated()) {
				ChannelDisplay.SetEventHandler(&ChannelDisplayEventHandler);
				ChannelDisplay.Create(m_Viewer.GetDisplayBase().GetParent()->GetHandle(),
									  WS_CHILD | WS_CLIPCHILDREN);
				ChannelDisplay.SetDriverManager(&DriverManager);
				ChannelDisplay.SetLogoManager(&LogoManager);
				if (m_fCustomFrame)
					HookWindows(ChannelDisplay.GetHandle());
			}
			m_Viewer.GetDisplayBase().SetDisplayView(&ChannelDisplay);
			m_Viewer.GetDisplayBase().SetVisible(true);
			if (CoreEngine.IsDriverSpecified())
				ChannelDisplay.SetSelect(CoreEngine.GetDriverFileName(),
										 ChannelManager.GetCurrentChannelInfo());
			ChannelDisplay.Update();
		} else {
			m_Viewer.GetDisplayBase().SetVisible(false);
		}
		return;

	case CM_ENABLEBUFFERING:
		CoreEngine.SetPacketBuffering(!CoreEngine.GetPacketBuffering());
		PlaybackOptions.SetPacketBuffering(CoreEngine.GetPacketBuffering());
		return;

	case CM_RESETBUFFER:
		CoreEngine.m_DtvEngine.ResetBuffer();
		return;

	case CM_RESETERRORCOUNT:
		CoreEngine.ResetErrorCount();
		StatusView.UpdateItem(STATUS_ITEM_ERROR);
		InfoPanel.UpdateItem(CInformationPanel::ITEM_ERROR);
		PluginManager.SendStatusResetEvent();
		return;

	case CM_SHOWRECORDREMAINTIME:
		{
			CRecordStatusItem *pItem=
				dynamic_cast<CRecordStatusItem*>(StatusView.GetItemByID(STATUS_ITEM_RECORD));

			if (pItem!=NULL) {
				bool fRemain=!RecordOptions.GetShowRemainTime();
				RecordOptions.SetShowRemainTime(fRemain);
				pItem->ShowRemainTime(fRemain);
			}
		}
		return;

	case CM_SHOWTOTTIME:
		{
			const bool fTOT=!StatusOptions.GetShowTOTTime();
			StatusOptions.SetShowTOTTime(fTOT);

			CClockStatusItem *pItem=
				dynamic_cast<CClockStatusItem*>(StatusView.GetItemByID(STATUS_ITEM_CLOCK));
			if (pItem!=NULL)
				pItem->SetTOT(fTOT);
		}
		return;

	case CM_PROGRAMINFOSTATUS_POPUPINFO:
		{
			const bool fEnable=!StatusOptions.IsPopupProgramInfoEnabled();
			StatusOptions.EnablePopupProgramInfo(fEnable);

			CProgramInfoStatusItem *pItem=
				dynamic_cast<CProgramInfoStatusItem*>(StatusView.GetItemByID(STATUS_ITEM_PROGRAMINFO));
			if (pItem!=NULL)
				pItem->EnablePopupInfo(fEnable);
		}
		return;

	case CM_ADJUSTTOTTIME:
		TotTimeAdjuster.BeginAdjust();
		return;

	case CM_ZOOMMENU:
	case CM_ASPECTRATIOMENU:
	case CM_CHANNELMENU:
	case CM_SERVICEMENU:
	case CM_TUNINGSPACEMENU:
	case CM_FAVORITESMENU:
	case CM_RECENTCHANNELMENU:
	case CM_VOLUMEMENU:
	case CM_AUDIOMENU:
	case CM_RESETMENU:
	case CM_BARMENU:
	case CM_PLUGINMENU:
	case CM_FILTERPROPERTYMENU:
		{
			int SubMenu=MenuOptions.GetSubMenuPosByCommand(id);
			POINT pt;

			if (codeNotify==COMMAND_FROM_MOUSE) {
				::GetCursorPos(&pt);
			} else {
				pt.x=0;
				pt.y=0;
				::ClientToScreen(m_Viewer.GetViewWindow().GetHandle(),&pt);
			}
			MainMenu.PopupSubMenu(SubMenu,TPM_RIGHTBUTTON,
								  pt.x,pt.y,MainWindow.GetHandle());
		}
		return;

	case CM_SIDEBAR_PLACE_LEFT:
	case CM_SIDEBAR_PLACE_RIGHT:
	case CM_SIDEBAR_PLACE_TOP:
	case CM_SIDEBAR_PLACE_BOTTOM:
		{
			CSideBarOptions::PlaceType Place=(CSideBarOptions::PlaceType)(id-CM_SIDEBAR_PLACE_FIRST);

			if (Place!=SideBarOptions.GetPlace()) {
				bool fVertical=
					Place==CSideBarOptions::PLACE_LEFT || Place==CSideBarOptions::PLACE_RIGHT;
				int Pane=
					Place==CSideBarOptions::PLACE_LEFT || Place==CSideBarOptions::PLACE_TOP?0:1;

				SideBarOptions.SetPlace(Place);
				SideBar.SetVertical(fVertical);
				Layout::CSplitter *pSplitter=
					dynamic_cast<Layout::CSplitter*>(m_LayoutBase.GetContainerByID(CONTAINER_ID_SIDEBARSPLITTER));
				bool fSwap=pSplitter->IDToIndex(CONTAINER_ID_SIDEBAR)!=Pane;
				pSplitter->SetStyle(
					(fVertical?Layout::CSplitter::STYLE_HORZ:Layout::CSplitter::STYLE_VERT) |
					Layout::CSplitter::STYLE_FIXED,
					!fSwap);
				if (fSwap)
					pSplitter->SwapPane();
			}
		}
		return;

	case CM_SIDEBAROPTIONS:
		if (::IsWindowEnabled(hwnd))
			ShowOptionDialog(hwnd,COptionDialog::PAGE_SIDEBAR);
		return;

	case CM_DRIVER_BROWSE:
		{
			OPENFILENAME ofn;
			TCHAR szFileName[MAX_PATH],szInitDir[MAX_PATH];
			CFilePath FilePath;

			FilePath.SetPath(CoreEngine.GetDriverFileName());
			if (FilePath.GetDirectory(szInitDir)) {
				::lstrcpy(szFileName,FilePath.GetFileName());
			} else {
				GetAppClass().GetAppDirectory(szInitDir);
				szFileName[0]='\0';
			}
			InitOpenFileName(&ofn);
			ofn.hwndOwner=GetVideoHostWindow();
			ofn.lpstrFilter=
				TEXT("BonDriver(BonDriver*.dll)\0BonDriver*.dll\0")
				TEXT("すべてのファイル\0*.*\0");
			ofn.lpstrFile=szFileName;
			ofn.nMaxFile=lengthof(szFileName);
			ofn.lpstrInitialDir=szInitDir;
			ofn.lpstrTitle=TEXT("BonDriverの選択");
			ofn.Flags=OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_EXPLORER;
			if (::GetOpenFileName(&ofn)) {
				AppMain.OpenTuner(szFileName);
			}
		}
		return;

	case CM_CHANNELHISTORY_CLEAR:
		RecentChannelList.Clear();
		return;

	case CM_PANEL_INFORMATION:
	case CM_PANEL_PROGRAMLIST:
	case CM_PANEL_CHANNEL:
	case CM_PANEL_CONTROL:
	case CM_PANEL_CAPTION:
		PanelForm.SetCurPageByID(id-CM_PANEL_FIRST);
		return;

	case CM_CHANNELPANEL_UPDATE:
		ChannelPanel.UpdateAllChannels(true);
		return;

	case CM_CHANNELPANEL_CURCHANNEL:
		ChannelPanel.ScrollToCurrentChannel();
		return;

	case CM_CHANNELPANEL_DETAILPOPUP:
		ChannelPanel.SetDetailToolTip(!ChannelPanel.GetDetailToolTip());
		return;

	case CM_CHANNELPANEL_SCROLLTOCURCHANNEL:
		ChannelPanel.SetScrollToCurChannel(!ChannelPanel.GetScrollToCurChannel());
		return;

	case CM_CHANNELPANEL_EVENTS_1:
	case CM_CHANNELPANEL_EVENTS_2:
	case CM_CHANNELPANEL_EVENTS_3:
	case CM_CHANNELPANEL_EVENTS_4:
		ChannelPanel.SetEventsPerChannel(id-CM_CHANNELPANEL_EVENTS_1+1);
		return;

	case CM_CHANNELPANEL_EXPANDEVENTS_2:
	case CM_CHANNELPANEL_EXPANDEVENTS_3:
	case CM_CHANNELPANEL_EXPANDEVENTS_4:
	case CM_CHANNELPANEL_EXPANDEVENTS_5:
	case CM_CHANNELPANEL_EXPANDEVENTS_6:
	case CM_CHANNELPANEL_EXPANDEVENTS_7:
	case CM_CHANNELPANEL_EXPANDEVENTS_8:
		ChannelPanel.SetEventsPerChannel(-1,id-CM_CHANNELPANEL_EXPANDEVENTS_2+2);
		return;

	case CM_CHANNELNO_2DIGIT:
	case CM_CHANNELNO_3DIGIT:
		{
			int Digits=id==CM_CHANNELNO_2DIGIT?2:3;

			if (m_ChannelNoInput.fInputting) {
				EndChannelNoInput();
				if (Digits==m_ChannelNoInput.Digits)
					return;
			}
			BeginChannelNoInput(Digits);
		}
		return;

	case CM_ADDTOFAVORITES:
		{
			const CChannelInfo *pChannel=ChannelManager.GetCurrentRealChannelInfo();
			if (pChannel!=NULL)
				FavoritesManager.AddChannel(pChannel,CoreEngine.GetDriverFileName());
		}
		return;

	case CM_ORGANIZEFAVORITES:
		{
			COrganizeFavoritesDialog Dialog(&FavoritesManager);

			Dialog.Show(GetVideoHostWindow());
		}
		return;

	default:
		if ((id>=CM_ZOOM_FIRST && id<=CM_ZOOM_LAST)
				|| (id>=CM_CUSTOMZOOM_FIRST && id<=CM_CUSTOMZOOM_LAST)) {
			CZoomOptions::ZoomInfo Info;

			if (m_pCore->GetFullscreen())
				m_pCore->SetFullscreen(false);
			if (::IsZoomed(hwnd))
				::ShowWindow(hwnd,SW_RESTORE);
			if (ZoomOptions.GetZoomInfoByCommand(id,&Info)) {
				if (Info.Type==CZoomOptions::ZOOM_RATE)
					SetZoomRate(Info.Rate.Rate,Info.Rate.Factor);
				else if (Info.Type==CZoomOptions::ZOOM_SIZE)
					AdjustWindowSize(Info.Size.Width,Info.Size.Height);
			}
			return;
		}

		if (id>=CM_AUDIOSTREAM_FIRST && id<=CM_AUDIOSTREAM_LAST) {
			m_pCore->SetAudioStream(id-CM_AUDIOSTREAM_FIRST);
			ShowAudioOSD();
			return;
		}

		if (id>=CM_CAPTURESIZE_FIRST && id<=CM_CAPTURESIZE_LAST) {
			int CaptureSize=id-CM_CAPTURESIZE_FIRST;

			CaptureOptions.SetPresetCaptureSize(CaptureSize);
			MainMenu.CheckRadioItem(CM_CAPTURESIZE_FIRST,CM_CAPTURESIZE_LAST,id);
			return;
		}

		if (id>=CM_CHANNELNO_FIRST && id<=CM_CHANNELNO_LAST) {
			int No=id-CM_CHANNELNO_FIRST;

#ifdef NETWORK_REMOCON_SUPPORT
			if (pNetworkRemocon!=NULL) {
				if (RecordManager.IsRecording()) {
					if (!RecordOptions.ConfirmChannelChange(GetVideoHostWindow()))
						return;
				}
				pNetworkRemocon->SetChannel(No);
				ChannelManager.SetNetworkRemoconCurrentChannel(
					ChannelManager.GetCurrentChannelList()->FindChannelNo(No+1));
				OnChannelChanged(0);
				PluginManager.SendChannelChangeEvent();
				return;
			} else
#endif
			{
				const CChannelList *pList=ChannelManager.GetCurrentChannelList();
				if (pList==NULL)
					return;

				int Index;

				if (pList->HasRemoteControlKeyID()) {
					Index=pList->FindChannelNo(No+1);
					if (Index<0)
						return;
				} else {
					Index=No;
				}
				id=CM_CHANNEL_FIRST+Index;
			}
		}
		// 上から続いているため、ここに別なコードを入れてはいけないので注意
		if (id>=CM_CHANNEL_FIRST && id<=CM_CHANNEL_LAST) {
			int Channel=id-CM_CHANNEL_FIRST;

			const CChannelList *pChList=ChannelManager.GetCurrentRealChannelList();
			if (pChList==NULL)
				return;
			const CChannelInfo *pChInfo=pChList->GetChannelInfo(Channel);
			if (pChInfo==NULL)
				return;
			if (RecordManager.IsRecording()) {
				if (!RecordOptions.ConfirmChannelChange(GetVideoHostWindow()))
					return;
			}
			AppMain.SetChannel(ChannelManager.GetCurrentSpace(),Channel);
			return;
		}

		if (id>=CM_SERVICE_FIRST && id<=CM_SERVICE_LAST) {
			if (RecordManager.IsRecording()) {
				if (!RecordOptions.ConfirmServiceChange(GetVideoHostWindow(),
														&RecordManager))
					return;
			}
			AppMain.SetServiceByIndex(id-CM_SERVICE_FIRST);
			return;
		}

		if (id>=CM_SPACE_ALL && id<=CM_SPACE_LAST) {
			int Space=id-CM_SPACE_FIRST;

			if (Space!=ChannelManager.GetCurrentSpace()) {
				const CChannelList *pChannelList=ChannelManager.GetChannelList(Space);
				if (pChannelList!=NULL) {
					for (int i=0;i<pChannelList->NumChannels();i++) {
						if (pChannelList->IsEnabled(i)) {
							AppMain.SetChannel(Space,i);
							return;
						}
					}
				}
			}
			return;
		}

		if (id>=CM_DRIVER_FIRST && id<=CM_DRIVER_LAST) {
			const CDriverInfo *pDriverInfo=DriverManager.GetDriverInfo(id-CM_DRIVER_FIRST);

			if (pDriverInfo!=NULL) {
				if (!CoreEngine.IsTunerOpen()
						|| !IsEqualFileName(pDriverInfo->GetFileName(),CoreEngine.GetDriverFileName())) {
					if (AppMain.OpenTuner(pDriverInfo->GetFileName())) {
						AppMain.RestoreChannel();
					}
				}
			}
			return;
		}

		if (id>=CM_PLUGIN_FIRST && id<=CM_PLUGIN_LAST) {
			CPlugin *pPlugin=PluginManager.GetPlugin(PluginManager.FindPluginByCommand(id));

			if (pPlugin!=NULL)
				pPlugin->Enable(!pPlugin->IsEnabled());
			return;
		}

		if (id>=CM_SPACE_CHANNEL_FIRST && id<=CM_SPACE_CHANNEL_LAST) {
			if (RecordManager.IsRecording()) {
				if (!RecordOptions.ConfirmChannelChange(GetVideoHostWindow()))
					return;
			}
			m_pCore->ProcessTunerMenu(id);
			return;
		}

		if (id>=CM_CHANNELHISTORY_FIRST && id<=CM_CHANNELHISTORY_LAST) {
			const CRecentChannelList::CChannel *pChannel=
				RecentChannelList.GetChannelInfo(id-CM_CHANNELHISTORY_FIRST);

			if (pChannel!=NULL)
				AppMain.OpenTunerAndSetChannel(pChannel->GetDriverFileName(),pChannel);
			return;
		}

		if (id>=CM_FAVORITECHANNEL_FIRST && id<=CM_FAVORITECHANNEL_LAST) {
			CFavoritesManager::ChannelInfo ChannelInfo;

			if (FavoritesManager.GetChannelByCommand(id,&ChannelInfo)) {
				CAppMain::ChannelSelectInfo SelInfo;

				SelInfo.Channel=ChannelInfo.Channel;
				SelInfo.TunerName=ChannelInfo.BonDriverFileName;
				SelInfo.fUseCurTuner=!ChannelInfo.fForceBonDriverChange;
				AppMain.SelectChannel(SelInfo);
			}
			return;
		}

		if (id>=CM_PLUGINCOMMAND_FIRST && id<=CM_PLUGINCOMMAND_LAST) {
			PluginManager.OnPluginCommand(CommandList.GetCommandText(CommandList.IDToIndex(id)));
			return;
		}

		if (id>=CM_PANANDSCAN_PRESET_FIRST && id<=CM_PANANDSCAN_PRESET_LAST) {
			SetPanAndScan(id);
			return;
		}
	}
}


void CMainWindow::OnTimer(HWND hwnd,UINT id)
{
	switch (id) {
	case TIMER_ID_UPDATE:
		// 情報更新
		{
			static unsigned int TimerCount=0;
			const CChannelInfo *pChInfo=ChannelManager.GetCurrentChannelInfo();

			DWORD UpdateStatus=CoreEngine.UpdateAsyncStatus();
			DWORD UpdateStatistics=CoreEngine.UpdateStatistics();

			// 映像サイズの変化
			if ((UpdateStatus&CCoreEngine::STATUS_VIDEOSIZE)!=0) {
				StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
				InfoPanel.SetVideoSize(CoreEngine.GetOriginalVideoWidth(),
										CoreEngine.GetOriginalVideoHeight(),
										CoreEngine.GetDisplayVideoWidth(),
										CoreEngine.GetDisplayVideoHeight());
				ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VIDEO);
			}

			// 音声形式の変化
			if ((UpdateStatus&(CCoreEngine::STATUS_AUDIOCHANNELS
							 | CCoreEngine::STATUS_AUDIOSTREAMS
							 | CCoreEngine::STATUS_AUDIOCOMPONENTTYPE
							 | CCoreEngine::STATUS_SPDIFPASSTHROUGH))!=0) {
				TRACE(TEXT("Audio status changed.\n"));
				if ((UpdateStatus&CCoreEngine::STATUS_SPDIFPASSTHROUGH)==0)
					AutoSelectStereoMode();
				StatusView.UpdateItem(STATUS_ITEM_AUDIOCHANNEL);
				ControlPanel.UpdateItem(CONTROLPANEL_ITEM_AUDIO);
				SideBar.CheckItem(CM_SPDIF_TOGGLE,
								  CoreEngine.m_DtvEngine.m_MediaViewer.IsSpdifPassthrough());
			}

			// 番組の切り替わり
			if ((UpdateStatus&CCoreEngine::STATUS_EVENTID)!=0) {
				// 番組の最後まで録画
				if (RecordManager.GetStopOnEventEnd())
					AppMain.StopRecord();

				m_pCore->UpdateTitle();

				if (OSDOptions.IsNotifyEnabled(COSDOptions::NOTIFY_EVENTNAME)
						&& !AppMain.IsChannelScanning()) {
					TCHAR szEventName[256];

					if (CoreEngine.m_DtvEngine.GetEventName(szEventName,lengthof(szEventName))>0) {
						TCHAR szBarText[EpgUtil::MAX_EVENT_TIME_LENGTH+lengthof(szEventName)];
						int Length=0;
						SYSTEMTIME StartTime;
						DWORD Duration;

						if (CoreEngine.m_DtvEngine.GetEventTime(&StartTime,&Duration)) {
							Length=EpgUtil::FormatEventTime(StartTime,Duration,
															szBarText,EpgUtil::MAX_EVENT_TIME_LENGTH);
							if (Length>0)
								szBarText[Length++]=_T(' ');
						}
						::lstrcpy(szBarText+Length,szEventName);
						ShowNotificationBar(szBarText);
					}
				}

				if (fShowPanelWindow && PanelForm.GetVisible()
						&& PanelForm.GetCurPageID()==PANEL_ID_INFORMATION)
					UpdateProgramInfo();

				ProgramListPanel.SetCurrentEventID(CoreEngine.m_DtvEngine.GetEventID());

				CProgramInfoStatusItem *pProgramInfoItem=
					dynamic_cast<CProgramInfoStatusItem*>(StatusView.GetItemByID(STATUS_ITEM_PROGRAMINFO));
				if (pProgramInfoItem!=NULL) {
					pProgramInfoItem->UpdateContent();
					pProgramInfoItem->Update();
				}

				if (m_AspectRatioType!=ASPECTRATIO_DEFAULT
						&& (m_fForceResetPanAndScan
						|| (ViewOptions.GetResetPanScanEventChange()
							&& m_AspectRatioType<ASPECTRATIO_CUSTOM))) {
					CoreEngine.m_DtvEngine.m_MediaViewer.SetPanAndScan(0,0);
					if (!m_pCore->GetFullscreen() && !::IsZoomed(hwnd)
							&& IsViewerEnabled()) {
						int Width,Height;

						if (CoreEngine.GetVideoViewSize(&Width,&Height)) {
							SIZE sz;
							m_Viewer.GetVideoContainer().GetClientSize(&sz);
							if (sz.cx<Width*sz.cy/Height)
								AdjustWindowSize(Width*sz.cy/Height,sz.cy);
						}
						// この時点でまだ新しい映像サイズが取得できない場合があるため、
						// WM_APP_VIDEOSIZECHANGED が来た時に調整するようにする
						m_AspectRatioResetTime=::GetTickCount();
					}
					m_AspectRatioType=ASPECTRATIO_DEFAULT;
					m_fForceResetPanAndScan=false;
					StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
					ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VIDEO);
					/*
					MainMenu.CheckRadioItem(CM_ASPECTRATIO_FIRST,CM_ASPECTRATIO_3D_LAST,
											CM_ASPECTRATIO_DEFAULT);
					*/
					AspectRatioIconMenu.CheckRadioItem(
						CM_ASPECTRATIO_FIRST,CM_ASPECTRATIO_3D_LAST,
						CM_ASPECTRATIO_DEFAULT);
					SideBar.CheckRadioItem(CM_ASPECTRATIO_FIRST,CM_ASPECTRATIO_LAST,
										   CM_ASPECTRATIO_DEFAULT);
				}

				m_CurEventStereoMode=-1;
				AutoSelectStereoMode();
			}

			// 時間変更などを反映させるために番組情報を更新
			if (TimerCount%(10000/UPDATE_TIMER_INTERVAL)==0) {
				m_pCore->UpdateTitle();

				CProgramInfoStatusItem *pProgramInfoItem=
					dynamic_cast<CProgramInfoStatusItem*>(StatusView.GetItemByID(STATUS_ITEM_PROGRAMINFO));
				if (pProgramInfoItem!=NULL) {
					if (pProgramInfoItem->UpdateContent())
						pProgramInfoItem->Update();
				}
			}

			if (RecordManager.IsRecording()) {
				if (RecordManager.QueryStop()) {
					AppMain.StopRecord();
				} else if (!RecordManager.IsPaused()) {
					StatusView.UpdateItem(STATUS_ITEM_RECORD);
				}
			} else {
				if (RecordManager.QueryStart())
					AppMain.StartReservedRecord();
			}

			if ((UpdateStatistics&(CCoreEngine::STATISTIC_ERRORPACKETCOUNT
								 | CCoreEngine::STATISTIC_CONTINUITYERRORPACKETCOUNT
								 | CCoreEngine::STATISTIC_SCRAMBLEPACKETCOUNT))!=0) {
				StatusView.UpdateItem(STATUS_ITEM_ERROR);
			}

			if ((UpdateStatistics&(CCoreEngine::STATISTIC_SIGNALLEVEL
								 | CCoreEngine::STATISTIC_BITRATE))!=0)
				StatusView.UpdateItem(STATUS_ITEM_SIGNALLEVEL);

			if ((UpdateStatistics&(CCoreEngine::STATISTIC_STREAMREMAIN
								 | CCoreEngine::STATISTIC_PACKETBUFFERRATE))!=0)
				StatusView.UpdateItem(STATUS_ITEM_BUFFERING);

			StatusView.UpdateItem(STATUS_ITEM_CLOCK);
			TotTimeAdjuster.AdjustTime();

			StatusView.UpdateItem(STATUS_ITEM_MEDIABITRATE);

			if (fShowPanelWindow && PanelForm.GetVisible()) {
				// パネルの更新
				if (PanelForm.GetCurPageID()==PANEL_ID_INFORMATION) {
					// 情報タブ更新
					BYTE AspectX,AspectY;
					if (CoreEngine.m_DtvEngine.m_MediaViewer.GetEffectiveAspectRatio(&AspectX,&AspectY))
						InfoPanel.SetAspectRatio(AspectX,AspectY);

					if ((UpdateStatistics&(CCoreEngine::STATISTIC_SIGNALLEVEL
										 | CCoreEngine::STATISTIC_BITRATE))!=0) {
						InfoPanel.UpdateItem(CInformationPanel::ITEM_SIGNALLEVEL);
					}

					InfoPanel.SetMediaBitRate(
						CoreEngine.m_DtvEngine.m_MediaViewer.GetVideoBitRate(),
						CoreEngine.m_DtvEngine.m_MediaViewer.GetAudioBitRate());

					if ((UpdateStatistics&(CCoreEngine::STATISTIC_ERRORPACKETCOUNT
										 | CCoreEngine::STATISTIC_CONTINUITYERRORPACKETCOUNT
										 | CCoreEngine::STATISTIC_SCRAMBLEPACKETCOUNT))!=0) {
						InfoPanel.UpdateItem(CInformationPanel::ITEM_ERROR);
					}

					if (RecordManager.IsRecording()) {
						const CRecordTask *pRecordTask=RecordManager.GetRecordTask();
						const LONGLONG FreeSpace=pRecordTask->GetFreeSpace();

						InfoPanel.SetRecordStatus(true,pRecordTask->GetFileName(),
							pRecordTask->GetWroteSize(),pRecordTask->GetRecordTime(),
							FreeSpace<0?0:FreeSpace);
					}

					if (TimerCount%(10000/UPDATE_TIMER_INTERVAL)==0)
						UpdateProgramInfo();
				} else if (PanelForm.GetCurPageID()==PANEL_ID_CHANNEL) {
					// チャンネルタブ更新
					if (!EpgOptions.IsEpgFileLoading()
							&& ChannelPanel.QueryUpdate())
						ChannelPanel.UpdateAllChannels(false);
				}
			}

			// 空き容量が少ない場合の注意表示
			if (RecordOptions.GetAlertLowFreeSpace()
					&& !m_fAlertedLowFreeSpace
					&& RecordManager.IsRecording()) {
				LONGLONG FreeSpace=RecordManager.GetRecordTask()->GetFreeSpace();

				if (FreeSpace>=0
						&& (ULONGLONG)FreeSpace<=RecordOptions.GetLowFreeSpaceThresholdBytes()) {
					NotifyBalloonTip.Show(
						APP_NAME TEXT("の録画ファイルの保存先の空き容量が少なくなっています。"),
						TEXT("空き容量が少なくなっています。"),
						NULL,CBalloonTip::ICON_WARNING);
					::SetTimer(m_hwnd,TIMER_ID_HIDETOOLTIP,10000,NULL);
					ShowNotificationBar(
						TEXT("録画ファイルの保存先の空き容量が少なくなっています"),
						CNotificationBar::MESSAGE_WARNING,6000);
					m_fAlertedLowFreeSpace=true;
				}
			}

			TimerCount++;
		}
		break;

	case TIMER_ID_OSD:
		// OSD を消す
		OSDManager.ClearOSD();
		::KillTimer(hwnd,TIMER_ID_OSD);
		break;

	case TIMER_ID_DISPLAY:
		// モニタがオフにならないようにする
		::SetThreadExecutionState(ES_DISPLAY_REQUIRED);
		break;

	case TIMER_ID_WHEELCHANNELCHANGE:
		// ホイールでのチャンネル変更
		{
			const CChannelInfo *pInfo=ChannelManager.GetChangingChannelInfo();

			SetWheelChannelChanging(false);
			ChannelManager.SetChangingChannel(-1);
			if (pInfo!=NULL) {
				const CChannelList *pList=ChannelManager.GetCurrentChannelList();

				if (pList->HasRemoteControlKeyID())
					SendCommand(CM_CHANNELNO_FIRST+pInfo->GetChannelNo()-1);
				else
					SendCommand(CM_CHANNELNO_FIRST+pInfo->GetChannelIndex());
			}
		}
		break;

	case TIMER_ID_PROGRAMLISTUPDATE:
		if (m_ProgramListUpdateTimerCount==0) {
			// サービスとロゴを関連付ける
			CTsAnalyzer *pAnalyzer=&CoreEngine.m_DtvEngine.m_TsAnalyzer;
			const WORD NetworkID=pAnalyzer->GetNetworkID();
			if (NetworkID!=0) {
				CTsAnalyzer::ServiceList ServiceList;
				if (pAnalyzer->GetServiceList(&ServiceList)) {
					for (size_t i=0;i<ServiceList.size();i++) {
						const CTsAnalyzer::ServiceInfo *pServiceInfo=&ServiceList[i];
						const WORD LogoID=pServiceInfo->LogoID;
						if (LogoID!=0xFFFF)
							LogoManager.AssociateLogoID(NetworkID,pServiceInfo->ServiceID,LogoID);
					}
				}
			}
		}

		// EPG情報の同期
		if (!EpgOptions.IsEpgFileLoading()
				&& !EpgOptions.IsEDCBDataLoading()) {
			WORD NetworkID=CoreEngine.m_DtvEngine.m_TsAnalyzer.GetNetworkID();
			WORD TransportStreamID=CoreEngine.m_DtvEngine.m_TsAnalyzer.GetTransportStreamID();

			if (fShowPanelWindow && PanelForm.GetCurPageID()==PANEL_ID_PROGRAMLIST) {
				int ServiceID=ChannelManager.GetCurrentServiceID();
				if (ServiceID<=0) {
					WORD SID;
					if (CoreEngine.m_DtvEngine.GetServiceID(&SID))
						ServiceID=SID;
				}
				if (ServiceID>0) {
					const HANDLE hThread=::GetCurrentThread();
					const int OldPriority=::GetThreadPriority(hThread);
					::SetThreadPriority(hThread,THREAD_PRIORITY_BELOW_NORMAL);

					EpgProgramList.UpdateService(NetworkID,TransportStreamID,(WORD)ServiceID);
					ProgramListPanel.UpdateProgramList(NetworkID,TransportStreamID,(WORD)ServiceID);

					::SetThreadPriority(hThread,OldPriority);
				}
			} else if (fShowPanelWindow && PanelForm.GetCurPageID()==PANEL_ID_CHANNEL) {
				ChannelPanel.UpdateChannels(NetworkID,TransportStreamID);
			}
			m_ProgramListUpdateTimerCount++;
			// 更新頻度を下げる
			if (m_ProgramListUpdateTimerCount>=6 && m_ProgramListUpdateTimerCount<=10)
				::SetTimer(hwnd,TIMER_ID_PROGRAMLISTUPDATE,(m_ProgramListUpdateTimerCount-5)*(60*1000),NULL);
		}
		break;

	case TIMER_ID_PROGRAMGUIDEUPDATE:
		// 番組表の取得
		if (m_fProgramGuideUpdating) {
			CEventManager *pEventManager=&CoreEngine.m_DtvEngine.m_EventManager;
			const CChannelList *pChannelList=ChannelManager.GetCurrentRealChannelList();
			const CChannelInfo *pCurChannelInfo=ChannelManager.GetCurrentChannelInfo();
			if (pChannelList==NULL || pCurChannelInfo==NULL) {
				EndProgramGuideUpdate();
				break;
			}

			bool fComplete=true,fBasic=false,fNoBasic=false;
			EpgChannelGroup &CurChGroup=m_EpgUpdateChannelList[m_EpgUpdateCurChannel];
			for (int i=0;i<CurChGroup.ChannelList.NumChannels();i++) {
				const CChannelInfo *pChannelInfo=CurChGroup.ChannelList.GetChannelInfo(i);
				const WORD NetworkID=pChannelInfo->GetNetworkID();
				const WORD TSID=pChannelInfo->GetTransportStreamID();
				const WORD ServiceID=pChannelInfo->GetServiceID();
				const NetworkType Network=GetNetworkType(NetworkID);

				if (pEventManager->HasSchedule(NetworkID,TSID,ServiceID,false)) {
					fBasic=true;
					if (!pEventManager->IsScheduleComplete(NetworkID,TSID,ServiceID,false)) {
						fComplete=false;
						break;
					}
					if (Network==NETWORK_TERRESTRIAL
							|| (Network==NETWORK_BS && EpgOptions.GetUpdateBSExtended())
							|| (Network==NETWORK_CS && EpgOptions.GetUpdateCSExtended())) {
						if (pEventManager->HasSchedule(NetworkID,TSID,ServiceID,true)
								&& !pEventManager->IsScheduleComplete(NetworkID,TSID,ServiceID,true)) {
							fComplete=false;
							break;
						}
					}
				} else {
					fNoBasic=true;
				}
			}

			if (fComplete && fBasic && fNoBasic
					&& m_EpgAccumulateClock.GetSpan()<60000)
				fComplete=false;

			if (fComplete) {
				TRACE(TEXT("EPG schedule complete\n"));
				if (!m_pCore->GetStandby())
					g_ProgramGuide.SendMessage(WM_COMMAND,CM_PROGRAMGUIDE_REFRESH,0);
			} else {
				WORD NetworkID=CoreEngine.m_DtvEngine.m_TsAnalyzer.GetNetworkID();
				DWORD Timeout;

				// 真面目に判定する場合BITから周期を取ってくる必要がある
				if (IsBSNetworkID(NetworkID) || IsCSNetworkID(NetworkID))
					Timeout=360000;
				else
					Timeout=120000;
				if (m_EpgAccumulateClock.GetSpan()>=Timeout) {
					TRACE(TEXT("EPG schedule timeout\n"));
					fComplete=true;
				} else {
					::SetTimer(m_hwnd,TIMER_ID_PROGRAMGUIDEUPDATE,5000,NULL);
				}
			}

			if (fComplete) {
				SetEpgUpdateNextChannel();
			}
		}
		break;

	case TIMER_ID_VIDEOSIZECHANGED:
		// 映像サイズの変化に合わせる
		m_Viewer.GetVideoContainer().SendSizeMessage();
		StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
		ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VIDEO);
		if (m_VideoSizeChangedTimerCount==3)
			::KillTimer(hwnd,TIMER_ID_VIDEOSIZECHANGED);
		else
			m_VideoSizeChangedTimerCount++;
		break;

	case TIMER_ID_RESETERRORCOUNT:
		// エラーカウントをリセットする
		// (既にサービスの情報が取得されている場合のみ)
		if (CoreEngine.m_DtvEngine.m_TsAnalyzer.GetServiceNum()>0) {
			SendCommand(CM_RESETERRORCOUNT);
			m_ResetErrorCountTimer.End();
		}
		break;

	case TIMER_ID_HIDETOOLTIP:
		// ツールチップを非表示にする
		NotifyBalloonTip.Hide();
		::KillTimer(hwnd,TIMER_ID_HIDETOOLTIP);
		break;

	case TIMER_ID_CHANNELNO:
		// チャンネル番号入力の時間切れ
		EndChannelNoInput();
		return;
	}
}


bool CMainWindow::UpdateProgramInfo()
{
	const bool fNext=InfoPanel.GetProgramInfoNext();
	TCHAR szText[4096],szTemp[2048];
	CStaticStringFormatter Formatter(szText,lengthof(szText));

	if (fNext)
		Formatter.Append(TEXT("次 : "));

	SYSTEMTIME StartTime;
	DWORD Duration;
	if (CoreEngine.m_DtvEngine.GetEventTime(&StartTime,&Duration,fNext)
			&& EpgUtil::FormatEventTime(StartTime,Duration,szTemp,lengthof(szTemp),
				EpgUtil::EVENT_TIME_DATE | EpgUtil::EVENT_TIME_YEAR | EpgUtil::EVENT_TIME_UNDECIDED_TEXT)>0) {
		Formatter.Append(szTemp);
		Formatter.Append(TEXT("\r\n"));
	}
	if (CoreEngine.m_DtvEngine.GetEventName(szTemp,lengthof(szTemp),fNext)>0) {
		Formatter.Append(szTemp);
		Formatter.Append(TEXT("\r\n\r\n"));
	}
	if (CoreEngine.m_DtvEngine.GetEventText(szTemp,lengthof(szTemp),fNext)>0) {
		Formatter.Append(szTemp);
		Formatter.Append(TEXT("\r\n\r\n"));
	}
	if (CoreEngine.m_DtvEngine.GetEventExtendedText(szTemp,lengthof(szTemp),fNext)>0) {
		Formatter.Append(szTemp);
	}

	CTsAnalyzer::EventSeriesInfo SeriesInfo;
	if (CoreEngine.m_DtvEngine.GetEventSeriesInfo(&SeriesInfo,fNext)
			&& SeriesInfo.EpisodeNumber!=0 && SeriesInfo.LastEpisodeNumber!=0) {
		Formatter.Append(TEXT("\r\n\r\n(シリーズ"));
		if (SeriesInfo.RepeatLabel!=0)
			Formatter.Append(TEXT(" [再]"));
		if (SeriesInfo.EpisodeNumber!=0 && SeriesInfo.LastEpisodeNumber!=0)
			Formatter.AppendFormat(TEXT(" 第%d回 / 全%d回"),
								   SeriesInfo.EpisodeNumber,SeriesInfo.LastEpisodeNumber);
		// expire_date は実際の最終回の日時でないので、紛らわしいため表示しない
		/*
		if (SeriesInfo.bIsExpireDateValid)
			Formatter.AppendFormat(TEXT(" 終了予定%d/%d/%d"),
								   SeriesInfo.ExpireDate.wYear,
								   SeriesInfo.ExpireDate.wMonth,
								   SeriesInfo.ExpireDate.wDay);
		*/
		Formatter.Append(TEXT(")"));
	}

	InfoPanel.SetProgramInfo(Formatter.GetString());
	return true;
}


bool CMainWindow::OnInitMenuPopup(HMENU hmenu)
{
	if (MainMenu.IsMainMenu(hmenu)) {
		bool fFullscreen=m_pCore->GetFullscreen();
		bool fView=IsViewerEnabled();

		MainMenu.EnableItem(CM_COPY,fView);
		MainMenu.EnableItem(CM_SAVEIMAGE,fView);
		MainMenu.CheckItem(CM_PANEL,
						   fFullscreen?m_Fullscreen.IsPanelVisible():fShowPanelWindow);
	} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_ZOOM)) {
		CZoomOptions::ZoomInfo Zoom;

		if (!GetZoomRate(&Zoom.Rate.Rate,&Zoom.Rate.Factor)) {
			Zoom.Rate.Rate=0;
			Zoom.Rate.Factor=0;
		}

		SIZE sz;
		if (m_Viewer.GetVideoContainer().GetClientSize(&sz)) {
			Zoom.Size.Width=sz.cx;
			Zoom.Size.Height=sz.cy;
		} else {
			Zoom.Size.Width=0;
			Zoom.Size.Height=0;
		}

		ZoomOptions.SetMenu(hmenu,&Zoom);
		Accelerator.SetMenuAccel(hmenu);
	} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_SPACE)) {
		m_pCore->InitTunerMenu(hmenu);
	} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_PLUGIN)) {
		PluginManager.SetMenu(hmenu);
		Accelerator.SetMenuAccel(hmenu);
	} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_FAVORITES)) {
		//FavoritesManager.SetMenu(hmenu);
		FavoritesMenu.Create(&FavoritesManager.GetRootFolder(),
							 CM_FAVORITECHANNEL_FIRST,hmenu,m_hwnd,
							 CFavoritesMenu::FLAG_SHOWEVENTINFO | CFavoritesMenu::FLAG_SHOWLOGO);
		::EnableMenuItem(hmenu,CM_ADDTOFAVORITES,
						 MF_BYCOMMAND | (ChannelManager.GetCurrentRealChannelInfo()!=NULL?MF_ENABLED:MF_GRAYED));
	} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_CHANNELHISTORY)) {
		RecentChannelList.SetMenu(hmenu);
	} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_ASPECTRATIO)) {
		int ItemCount=::GetMenuItemCount(hmenu);

		if (ItemCount>m_DefaultAspectRatioMenuItemCount) {
			for (;ItemCount>m_DefaultAspectRatioMenuItemCount;ItemCount--) {
				::DeleteMenu(hmenu,ItemCount-3,MF_BYPOSITION);
			}
		}

		size_t PresetCount=PanAndScanOptions.GetPresetCount();
		if (PresetCount>0) {
			::InsertMenu(hmenu,ItemCount-2,MF_BYPOSITION | MF_SEPARATOR,0,NULL);
			for (size_t i=0;i<PresetCount;i++) {
				CPanAndScanOptions::PanAndScanInfo Info;
				TCHAR szText[CPanAndScanOptions::MAX_NAME*2];

				PanAndScanOptions.GetPreset(i,&Info);
				CopyToMenuText(Info.szName,szText,lengthof(szText));
				::InsertMenu(hmenu,ItemCount-2+(UINT)i,
							 MF_BYPOSITION | MF_STRING | MF_ENABLED
							 | (m_AspectRatioType==ASPECTRATIO_CUSTOM+(int)i?MF_CHECKED:MF_UNCHECKED),
							 CM_PANANDSCAN_PRESET_FIRST+i,szText);
			}
		}

		AspectRatioIconMenu.CheckItem(CM_FRAMECUT,
			CoreEngine.m_DtvEngine.m_MediaViewer.GetViewStretchMode()==CMediaViewer::STRETCH_CUTFRAME);

		Accelerator.SetMenuAccel(hmenu);
		if (!AspectRatioIconMenu.OnInitMenuPopup(m_hwnd,hmenu))
			return false;
	} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_CHANNEL)) {
		m_pCore->InitChannelMenu(hmenu);
	} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_SERVICE)) {
		CTsAnalyzer::ServiceList ServiceList;
		WORD CurServiceID;
		int CurService=-1;

		CoreEngine.m_DtvEngine.m_TsAnalyzer.GetViewableServiceList(&ServiceList);
		if (!CoreEngine.m_DtvEngine.GetServiceID(&CurServiceID))
			CurServiceID=0;
		ClearMenu(hmenu);
		for (size_t i=0;i<ServiceList.size();i++) {
			const CTsAnalyzer::ServiceInfo &ServiceInfo=ServiceList[i];
			TCHAR szText[512],szEventName[256],szMenu[256];
			CStaticStringFormatter Formatter(szText,lengthof(szText));

			if (ServiceInfo.szServiceName[0]!='\0') {
				CopyToMenuText(ServiceInfo.szServiceName,szMenu,lengthof(szMenu));
				Formatter.AppendFormat(TEXT("&%d: %s"),i+1,szMenu);
			} else {
				Formatter.AppendFormat(TEXT("&%d: サービス%d"),i+1,i+1);
			}
			if (CoreEngine.m_DtvEngine.m_TsAnalyzer.GetEventName(
					CoreEngine.m_DtvEngine.m_TsAnalyzer.GetServiceIndexByID(ServiceInfo.ServiceID),
					szEventName,lengthof(szEventName))>0) {
				CopyToMenuText(szEventName,szMenu,lengthof(szMenu));
				Formatter.AppendFormat(TEXT(" (%s)"),szMenu);
			}
			::AppendMenu(hmenu,MF_STRING | MF_ENABLED,CM_SERVICE_FIRST+i,Formatter.GetString());
			if (ServiceInfo.ServiceID==CurServiceID)
				CurService=(int)i;
		}
		if (CurService>=0)
			MainMenu.CheckRadioItem(CM_SERVICE_FIRST,
									CM_SERVICE_FIRST+(int)ServiceList.size()-1,
									CM_SERVICE_FIRST+CurService);
	} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_AUDIO)) {
		CPopupMenu Menu(hmenu);
		Menu.Clear();

		CTsAnalyzer::EventAudioInfo AudioInfo;
		const bool fDualMono=CoreEngine.m_DtvEngine.GetEventAudioInfo(&AudioInfo)
								&& AudioInfo.ComponentType==0x02;
		if (fDualMono) {
			// Dual mono
			TCHAR szText[80],szAudio1[64],szAudio2[64];

			szAudio1[0]='\0';
			szAudio2[0]='\0';
			if (AudioInfo.szText[0]!='\0') {
				LPTSTR pszDelimiter=::StrChr(AudioInfo.szText,_T('\r'));
				if (pszDelimiter!=NULL) {
					*pszDelimiter='\0';
					CopyToMenuText(AudioInfo.szText,szAudio1,lengthof(szAudio1));
					CopyToMenuText(pszDelimiter+1,szAudio2,lengthof(szAudio2));
				}
			}
			// ES multilingual flag が立っているのに両方日本語の場合がある
			if (AudioInfo.bESMultiLingualFlag
					&& AudioInfo.LanguageCode!=AudioInfo.LanguageCode2) {
				// 二カ国語
				LPCTSTR pszLang1=szAudio1[0]!='\0'?szAudio1:
					EpgUtil::GetLanguageText(AudioInfo.LanguageCode);
				LPCTSTR pszLang2=szAudio2[0]!='\0'?szAudio2:
					EpgUtil::GetLanguageText(AudioInfo.LanguageCode2);
				StdUtil::snprintf(szText,lengthof(szText),TEXT("%s+%s(&S)"),pszLang1,pszLang2);
				Menu.Append(CM_STEREO_THROUGH,szText);
				::wsprintf(szText,TEXT("%s(&L)"),pszLang1);
				Menu.Append(CM_STEREO_LEFT,szText);
				::wsprintf(szText,TEXT("%s(&R)"),pszLang2);
				Menu.Append(CM_STEREO_RIGHT,szText);
			} else {
				Menu.Append(CM_STEREO_THROUGH,TEXT("主+副音声(&S)"));
				if (szAudio1[0]!='\0')
					StdUtil::snprintf(szText,lengthof(szText),TEXT("主音声(%s)(&L)"),szAudio1);
				else
					::lstrcpy(szText,TEXT("主音声(&L)"));
				Menu.Append(CM_STEREO_LEFT,szText);
				if (szAudio2[0]!='\0')
					StdUtil::snprintf(szText,lengthof(szText),TEXT("副音声(%s)(&R)"),szAudio2);
				else
					::lstrcpy(szText,TEXT("副音声(&R)"));
				Menu.Append(CM_STEREO_RIGHT,szText);
			}
			Menu.AppendSeparator();
		}

		const int NumAudioStreams=m_pCore->GetNumAudioStreams();
		if (NumAudioStreams>0) {
			for (int i=0;i<NumAudioStreams;i++) {
				TCHAR szText[64];
				int Length;

				Length=::wsprintf(szText,TEXT("&%d: 音声%d"),i+1,i+1);
				if (NumAudioStreams>1
						&& CoreEngine.m_DtvEngine.GetEventAudioInfo(&AudioInfo,i)) {
					if (AudioInfo.szText[0]!='\0') {
						LPTSTR p=::StrChr(AudioInfo.szText,_T('\r'));
						if (p!=NULL) {
							*p++=_T('/');
							if (*p==_T('\n'))
								::MoveMemory(p,p+1,::lstrlen(p)*sizeof(*p));
						}
						StdUtil::snprintf(szText+Length,lengthof(szText)-Length,
							TEXT(" (%s)"),AudioInfo.szText);
					} else {
						StdUtil::snprintf(szText+Length,lengthof(szText)-Length,
							TEXT(" (%s)"),
							EpgUtil::GetLanguageText(AudioInfo.LanguageCode));
					}
				}
				Menu.Append(CM_AUDIOSTREAM_FIRST+i,szText);
			}
			Menu.CheckRadioItem(CM_AUDIOSTREAM_FIRST,
								CM_AUDIOSTREAM_FIRST+NumAudioStreams-1,
								CM_AUDIOSTREAM_FIRST+m_pCore->GetAudioStream());
		}

		if (!fDualMono) {
			if (NumAudioStreams>0)
				Menu.AppendSeparator();
			Menu.Append(CM_STEREO_THROUGH,TEXT("ステレオ/スルー(&S)"));
			Menu.Append(CM_STEREO_LEFT,TEXT("左(主音声)(&L)"));
			Menu.Append(CM_STEREO_RIGHT,TEXT("右(副音声)(&R)"));
		}
		Menu.CheckRadioItem(CM_STEREO_THROUGH,CM_STEREO_RIGHT,
							CM_STEREO_THROUGH+m_pCore->GetStereoMode());

		Menu.AppendSeparator();
		Menu.Append(CM_SPDIF_DISABLED,TEXT("S/PDIFパススルー : 無効"));
		Menu.Append(CM_SPDIF_PASSTHROUGH,TEXT("S/PDIFパススルー : 有効"));
		Menu.Append(CM_SPDIF_AUTO,TEXT("S/PDIFパススルー : 自動切替"));
		CAudioDecFilter::SpdifOptions SpdifOptions;
		CoreEngine.GetSpdifOptions(&SpdifOptions);
		Menu.CheckRadioItem(CM_SPDIF_DISABLED,CM_SPDIF_AUTO,
							CM_SPDIF_DISABLED+(int)SpdifOptions.Mode);
	} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_FILTERPROPERTY)) {
		for (int i=0;i<lengthof(g_DirectShowFilterPropertyList);i++) {
			MainMenu.EnableItem(g_DirectShowFilterPropertyList[i].Command,
				CoreEngine.m_DtvEngine.m_MediaViewer.FilterHasProperty(
									g_DirectShowFilterPropertyList[i].Filter));
		}
	} else if (hmenu==MainMenu.GetSubMenu(CMainMenu::SUBMENU_BAR)) {
		MainMenu.CheckItem(CM_TITLEBAR,m_fShowTitleBar);
		MainMenu.CheckItem(CM_STATUSBAR,m_fShowStatusBar);
		MainMenu.CheckItem(CM_SIDEBAR,m_fShowSideBar);
		MainMenu.CheckRadioItem(CM_WINDOWFRAME_NORMAL,CM_WINDOWFRAME_NONE,
								!m_fCustomFrame?CM_WINDOWFRAME_NORMAL:
								(m_CustomFrameWidth==0?CM_WINDOWFRAME_NONE:CM_WINDOWFRAME_CUSTOM));
		MainMenu.CheckItem(CM_CUSTOMTITLEBAR,m_fCustomTitleBar);
		MainMenu.EnableItem(CM_CUSTOMTITLEBAR,!m_fCustomFrame);
		MainMenu.CheckItem(CM_SPLITTITLEBAR,m_fSplitTitleBar);
		MainMenu.EnableItem(CM_SPLITTITLEBAR,!m_fCustomFrame && m_fCustomTitleBar);
		MainMenu.CheckItem(CM_VIDEOEDGE,m_fViewWindowEdge);
	} else {
		if (ChannelMenuManager.InitPopup(MainMenu.GetSubMenu(CMainMenu::SUBMENU_SPACE),hmenu))
			return true;

		if (TunerSelectMenu.OnInitMenuPopup(hmenu))
			return true;

		return false;
	}

	return true;
}


void CMainWindow::OnTunerChanged()
{
	SetWheelChannelChanging(false);

	if (m_fProgramGuideUpdating)
		EndProgramGuideUpdate(0);

	ProgramListPanel.ClearProgramList();
	InfoPanel.ResetStatistics();
	bool fNoSignalLevel=DriverOptions.IsNoSignalLevel(CoreEngine.GetDriverFileName());
	InfoPanel.ShowSignalLevel(!fNoSignalLevel);
	CSignalLevelStatusItem *pItem=dynamic_cast<CSignalLevelStatusItem*>(
							StatusView.GetItemByID(STATUS_ITEM_SIGNALLEVEL));
	if (pItem!=NULL)
		pItem->ShowSignalLevel(!fNoSignalLevel);
	/*
	if (fShowPanelWindow && PanelForm.GetCurPageID()==PANEL_ID_CHANNEL) {
		ChannelPanel.SetChannelList(ChannelManager.GetCurrentChannelList(),
									!EpgOptions.IsEpgFileLoading());
	} else {
		ChannelPanel.ClearChannelList();
	}
	*/
	CaptionPanel.Clear();
	g_ProgramGuide.ClearCurrentService();
	ClearMenu(MainMenu.GetSubMenu(CMainMenu::SUBMENU_SERVICE));
	m_ResetErrorCountTimer.End();
	StatusView.UpdateItem(STATUS_ITEM_TUNER);
	ControlPanel.UpdateItem(CONTROLPANEL_ITEM_TUNER);
	if (SideBarOptions.GetShowChannelLogo())
		SideBar.Invalidate();
	m_fForceResetPanAndScan=true;
}


void CMainWindow::OnTunerOpened()
{
	if (m_fProgramGuideUpdating)
		EndProgramGuideUpdate(0);
}


void CMainWindow::OnTunerClosed()
{
}


void CMainWindow::OnChannelListChanged()
{
	if (fShowPanelWindow && PanelForm.GetCurPageID()==PANEL_ID_CHANNEL) {
		ChannelPanel.SetChannelList(ChannelManager.GetCurrentChannelList());
		ChannelPanel.SetCurrentChannel(ChannelManager.GetCurrentChannel());
	} else {
		ChannelPanel.ClearChannelList();
	}
	if (SideBarOptions.GetShowChannelLogo())
		SideBar.Invalidate();
}


void CMainWindow::OnChannelChanged(unsigned int Status)
{
	const bool fSpaceChanged=(Status & CUICore::CHANNEL_CHANGED_STATUS_SPACE_CHANGED)!=0;
	const int CurSpace=ChannelManager.GetCurrentSpace();
	const CChannelInfo *pCurChannel=ChannelManager.GetCurrentChannelInfo();

	SetWheelChannelChanging(false);

	if (m_fProgramGuideUpdating && !m_fEpgUpdateChannelChange
			&& (Status & CUICore::CHANNEL_CHANGED_STATUS_DETECTED)==0)
		EndProgramGuideUpdate(0);

	if (CurSpace>CChannelManager::SPACE_INVALID)
		MainMenu.CheckRadioItem(CM_SPACE_ALL,CM_SPACE_ALL+ChannelManager.NumSpaces(),
								CM_SPACE_FIRST+CurSpace);
	ClearMenu(MainMenu.GetSubMenu(CMainMenu::SUBMENU_SERVICE));
	StatusView.UpdateItem(STATUS_ITEM_CHANNEL);
	StatusView.UpdateItem(STATUS_ITEM_TUNER);
	ControlPanel.UpdateItem(CONTROLPANEL_ITEM_CHANNEL);
	ControlPanel.UpdateItem(CONTROLPANEL_ITEM_TUNER);
	if (pCurChannel!=NULL && OSDOptions.IsOSDEnabled(COSDOptions::OSD_CHANNEL))
		ShowChannelOSD();
	ProgramListPanel.ClearProgramList();
	::SetTimer(m_hwnd,TIMER_ID_PROGRAMLISTUPDATE,10000,NULL);
	m_ProgramListUpdateTimerCount=0;
	InfoPanel.ResetStatistics();
	ProgramListPanel.ShowRetrievingMessage(true);
	if (fSpaceChanged) {
		if (fShowPanelWindow && PanelForm.GetCurPageID()==PANEL_ID_CHANNEL) {
			ChannelPanel.SetChannelList(ChannelManager.GetCurrentChannelList(),
										!EpgOptions.IsEpgFileLoading());
			ChannelPanel.SetCurrentChannel(ChannelManager.GetCurrentChannel());
		} else {
			ChannelPanel.ClearChannelList();
		}
	} else {
		if (fShowPanelWindow && PanelForm.GetCurPageID()==PANEL_ID_CHANNEL)
			ChannelPanel.SetCurrentChannel(ChannelManager.GetCurrentChannel());
	}
	if (pCurChannel!=NULL) {
		g_ProgramGuide.SetCurrentService(pCurChannel->GetNetworkID(),
										 pCurChannel->GetTransportStreamID(),
										 pCurChannel->GetServiceID());
	} else {
		g_ProgramGuide.ClearCurrentService();
	}
	int ChannelNo;
	if (pCurChannel!=NULL)
		ChannelNo=pCurChannel->GetChannelNo();
	if (fSpaceChanged && SideBarOptions.GetShowChannelLogo())
		SideBar.Invalidate();
	SideBar.CheckRadioItem(CM_CHANNELNO_1,CM_CHANNELNO_12,
						   pCurChannel!=NULL && ChannelNo>=1 && ChannelNo<=12?
						   CM_CHANNELNO_1+ChannelNo-1:0);
	CaptionPanel.Clear();
	UpdateControlPanelStatus();

	LPCTSTR pszDriverFileName=CoreEngine.GetDriverFileName();
	pCurChannel=ChannelManager.GetCurrentRealChannelInfo();
	if (pCurChannel!=NULL) {
		RecentChannelList.Add(pszDriverFileName,pCurChannel);
		ChannelHistory.SetCurrentChannel(pszDriverFileName,pCurChannel);
	}
	if (DriverOptions.IsResetChannelChangeErrorCount(pszDriverFileName))
		m_ResetErrorCountTimer.Begin(m_hwnd,5000);
	else
		m_ResetErrorCountTimer.End();
	/*
	m_pCore->SetStereoMode(0);
	m_CurEventStereoMode=-1;
	*/
	m_fForceResetPanAndScan=true;
}


void CMainWindow::ShowChannelOSD()
{
	if (GetVisible() && !::IsIconic(m_hwnd)) {
		const CChannelInfo *pInfo;

		if (m_fWheelChannelChanging)
			pInfo=ChannelManager.GetChangingChannelInfo();
		else
			pInfo=ChannelManager.GetCurrentChannelInfo();
		if (pInfo!=NULL)
			OSDManager.ShowChannelOSD(pInfo,m_fWheelChannelChanging);
	}
}


void CMainWindow::OnServiceChanged()
{
	int CurService=0;
	WORD ServiceID;
	if (CoreEngine.m_DtvEngine.GetServiceID(&ServiceID))
		CurService=CoreEngine.m_DtvEngine.m_TsAnalyzer.GetViewableServiceIndexByID(ServiceID);
	MainMenu.CheckRadioItem(CM_SERVICE_FIRST,
							CM_SERVICE_FIRST+CoreEngine.m_DtvEngine.m_TsAnalyzer.GetViewableServiceNum()-1,
							CM_SERVICE_FIRST+CurService);

	StatusView.UpdateItem(STATUS_ITEM_CHANNEL);

	if (PanelForm.GetCurPageID()==PANEL_ID_INFORMATION)
		UpdateProgramInfo();
}


void CMainWindow::OnRecordingStarted()
{
	EndProgramGuideUpdate(0);

	StatusView.UpdateItem(STATUS_ITEM_RECORD);
	StatusView.UpdateItem(STATUS_ITEM_ERROR);
	//MainMenu.EnableItem(CM_RECORDOPTION,false);
	//MainMenu.EnableItem(CM_RECORDSTOPTIME,true);
	TaskbarManager.SetRecordingStatus(true);

	m_ResetErrorCountTimer.End();
	m_fAlertedLowFreeSpace=false;
	if (OSDOptions.IsOSDEnabled(COSDOptions::OSD_RECORDING))
		OSDManager.ShowOSD(TEXT("●録画"));
}


void CMainWindow::OnRecordingStopped()
{
	StatusView.UpdateItem(STATUS_ITEM_RECORD);
	InfoPanel.SetRecordStatus(false);
	//MainMenu.EnableItem(CM_RECORDOPTION,true);
	//MainMenu.EnableItem(CM_RECORDSTOPTIME,false);
	TaskbarManager.SetRecordingStatus(false);
	RecordManager.SetStopOnEventEnd(false);
	if (OSDOptions.IsOSDEnabled(COSDOptions::OSD_RECORDING))
		OSDManager.ShowOSD(TEXT("■録画停止"));
	if (m_pCore->GetStandby())
		AppMain.CloseTuner();
	if (m_fExitOnRecordingStop)
		PostCommand(CM_EXIT);
}


void CMainWindow::OnMouseWheel(WPARAM wParam,LPARAM lParam,bool fHorz)
{
	POINT pt;
	pt.x=GET_X_LPARAM(lParam);
	pt.y=GET_Y_LPARAM(lParam);

	if (m_Viewer.GetDisplayBase().IsVisible()) {
		CDisplayView *pDisplayView=m_Viewer.GetDisplayBase().GetDisplayView();

		if (pDisplayView!=NULL) {
			RECT rc;

			m_Viewer.GetDisplayBase().GetParent()->GetScreenPosition(&rc);
			if (::PtInRect(&rc,pt)) {
				if (pDisplayView->OnMouseWheel(fHorz?WM_MOUSEHWHEEL:WM_MOUSEWHEEL,wParam,lParam))
					return;
			}
		}
	}

	COperationOptions::WheelMode Mode;

	if (fHorz) {
		Mode=OperationOptions.GetWheelTiltMode();
	} else {
		if ((wParam&MK_SHIFT)!=0)
			Mode=OperationOptions.GetWheelShiftMode();
		else if ((wParam&MK_CONTROL)!=0)
			Mode=OperationOptions.GetWheelCtrlMode();
		else
			Mode=OperationOptions.GetWheelMode();
	}

	if (OperationOptions.IsStatusBarWheelEnabled() && StatusView.GetVisible()) {
		RECT rc;

		StatusView.GetScreenPosition(&rc);
		if (::PtInRect(&rc,pt)) {
			switch (StatusView.GetCurItem()) {
			case STATUS_ITEM_CHANNEL:
				Mode=COperationOptions::WHEEL_MODE_CHANNEL;
				break;
#if 0
			// 倍率が変わるとウィンドウサイズも変わるので使いづらい
			case STATUS_ITEM_VIDEOSIZE:
				Mode=COperationOptions::WHEEL_MODE_ZOOM;
				break;
#endif
			case STATUS_ITEM_VOLUME:
				Mode=COperationOptions::WHEEL_MODE_VOLUME;
				break;
			case STATUS_ITEM_AUDIOCHANNEL:
				Mode=COperationOptions::WHEEL_MODE_AUDIO;
				break;
			}
		}
	}

	const bool fReverse=OperationOptions.IsWheelModeReverse(Mode);
	int Delta=GET_WHEEL_DELTA_WPARAM(wParam);
	if (fReverse)
		Delta=-Delta;
	const DWORD CurTime=::GetTickCount();
	bool fProcessed=false;

	if (Mode!=m_PrevWheelMode)
		m_WheelCount=0;
	else
		m_WheelCount++;

	switch (Mode) {
	case COperationOptions::WHEEL_MODE_VOLUME:
		SendCommand(Delta>=0?CM_VOLUME_UP:CM_VOLUME_DOWN);
		fProcessed=true;
		break;

	case COperationOptions::WHEEL_MODE_CHANNEL:
		{
			bool fUp;

			if (fHorz)
				fUp=Delta>0;
			else
				fUp=Delta<0;
			const CChannelInfo *pInfo=ChannelManager.GetNextChannelInfo(fUp);
			if (pInfo!=NULL) {
				if (m_fWheelChannelChanging
						&& m_WheelCount<5
						&& TickTimeSpan(m_PrevWheelTime,CurTime)<(5UL-m_WheelCount)*100UL) {
					break;
				}
				SetWheelChannelChanging(true,OperationOptions.GetWheelChannelDelay());
				ChannelManager.SetChangingChannel(ChannelManager.FindChannelInfo(pInfo));
				StatusView.UpdateItem(STATUS_ITEM_CHANNEL);
				if (OSDOptions.IsOSDEnabled(COSDOptions::OSD_CHANNEL))
					ShowChannelOSD();
			}
			fProcessed=true;
		}
		break;

	case COperationOptions::WHEEL_MODE_AUDIO:
		if (Mode!=m_PrevWheelMode || TickTimeSpan(m_PrevWheelTime,CurTime)>=300) {
			SendCommand(CM_SWITCHAUDIO);
			fProcessed=true;
		}
		break;

	case COperationOptions::WHEEL_MODE_ZOOM:
		if (Mode!=m_PrevWheelMode || TickTimeSpan(m_PrevWheelTime,CurTime)>=500) {
			if (!IsZoomed(m_hwnd) && !m_pCore->GetFullscreen()) {
				int Zoom;

				Zoom=GetZoomPercentage();
				if (Delta>=0)
					Zoom+=OperationOptions.GetWheelZoomStep();
				else
					Zoom-=OperationOptions.GetWheelZoomStep();
				SetZoomRate(Zoom,100);
			}
			fProcessed=true;
		}
		break;

	case COperationOptions::WHEEL_MODE_ASPECTRATIO:
		if (Mode!=m_PrevWheelMode || TickTimeSpan(m_PrevWheelTime,CurTime)>=300) {
			SendCommand(CM_ASPECTRATIO);
			fProcessed=true;
		}
		break;
	}

	m_PrevWheelMode=Mode;
	if (fProcessed)
		m_PrevWheelTime=CurTime;
}


bool CMainWindow::EnableViewer(bool fEnable)
{
	if (fEnable && !CoreEngine.m_DtvEngine.m_MediaViewer.IsOpen()) {
		CoreEngine.m_DtvEngine.SetTracer(&StatusView);
		bool fOK=InitializeViewer();
		CoreEngine.m_DtvEngine.SetTracer(NULL);
		StatusView.SetSingleText(NULL);
		if (!fOK)
			return false;
	}
	if (!m_Viewer.EnableViewer(fEnable))
		return false;
	MainMenu.CheckItem(CM_DISABLEVIEWER,!fEnable);
	SideBar.CheckItem(CM_DISABLEVIEWER,!fEnable);
	m_pCore->PreventDisplaySave(fEnable);
	return true;
}


bool CMainWindow::IsViewerEnabled() const
{
	return m_Viewer.IsViewerEnabled();
}


HWND CMainWindow::GetViewerWindow() const
{
	return m_Viewer.GetVideoContainer().GetHandle();
}


void CMainWindow::OnVolumeChanged(bool fOSD)
{
	const int Volume=m_pCore->GetVolume();

	StatusView.UpdateItem(STATUS_ITEM_VOLUME);
	ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VOLUME);
	MainMenu.CheckItem(CM_VOLUME_MUTE,false);
	if (fOSD && OSDOptions.IsOSDEnabled(COSDOptions::OSD_VOLUME)
			&& GetVisible() && !::IsIconic(m_hwnd))
		OSDManager.ShowVolumeOSD(Volume);
}


void CMainWindow::OnMuteChanged()
{
	const bool fMute=m_pCore->GetMute();

	StatusView.UpdateItem(STATUS_ITEM_VOLUME);
	ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VOLUME);
	MainMenu.CheckItem(CM_VOLUME_MUTE,fMute);
}


void CMainWindow::OnStereoModeChanged()
{
	const int StereoMode=m_pCore->GetStereoMode();

	m_CurEventStereoMode=StereoMode;
	/*
	MainMenu.CheckRadioItem(CM_STEREO_THROUGH,CM_STEREO_RIGHT,
							CM_STEREO_THROUGH+StereoMode);
	*/
	StatusView.UpdateItem(STATUS_ITEM_AUDIOCHANNEL);
	ControlPanel.UpdateItem(CONTROLPANEL_ITEM_AUDIO);
}


void CMainWindow::OnAudioStreamChanged()
{
	const int Stream=m_pCore->GetAudioStream();

	MainMenu.CheckRadioItem(CM_AUDIOSTREAM_FIRST,
							CM_AUDIOSTREAM_FIRST+m_pCore->GetNumAudioStreams()-1,
							CM_AUDIOSTREAM_FIRST+Stream);
	StatusView.UpdateItem(STATUS_ITEM_AUDIOCHANNEL);
	ControlPanel.UpdateItem(CONTROLPANEL_ITEM_AUDIO);
}


void CMainWindow::AutoSelectStereoMode()
{
	/*
		Dual Mono 時に音声を自動で選択する
		一つの番組で本編Dual Mono/CMステレオのような場合
		  Aパート -> CM       -> Bパート
		  副音声  -> ステレオ -> 副音声
		のように、ユーザーの選択を記憶しておく必要がある
	*/
	const bool fDualMono=CoreEngine.m_DtvEngine.GetAudioChannelNum()==
										CMediaViewer::AUDIO_CHANNEL_DUALMONO
					/*|| CoreEngine.m_DtvEngine.GetAudioComponentType()==0x02*/;

	if (m_CurEventStereoMode<0) {
		m_pCore->SetStereoMode(fDualMono?CoreEngine.GetAutoStereoMode():CCoreEngine::STEREOMODE_STEREO);
		m_CurEventStereoMode=-1;
	} else {
		int OldStereoMode=m_CurEventStereoMode;
		m_pCore->SetStereoMode(fDualMono?m_CurEventStereoMode:CCoreEngine::STEREOMODE_STEREO);
		m_CurEventStereoMode=OldStereoMode;
	}
}


void CMainWindow::ShowAudioOSD()
{
	if (OSDOptions.IsOSDEnabled(COSDOptions::OSD_AUDIO)) {
		CTsAnalyzer::EventAudioInfo AudioInfo;
		TCHAR szText[128];

		if (CoreEngine.m_DtvEngine.GetEventAudioInfo(&AudioInfo)) {
			if (AudioInfo.ComponentType==0x02) {
				// Dual mono
				TCHAR szAudio1[64],szAudio2[64];
				LPCTSTR pszAudio1,pszAudio2;

				szAudio1[0]=_T('\0');
				szAudio2[0]=_T('\0');
				if (AudioInfo.szText[0]!=_T('\0')) {
					LPTSTR pszDelimiter=::StrChr(AudioInfo.szText,_T('\r'));
					if (pszDelimiter!=NULL) {
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
					pszAudio1=szAudio1[0]!=_T('\0')?szAudio1:
						EpgUtil::GetLanguageText(AudioInfo.LanguageCode);
					pszAudio2=szAudio2[0]!=_T('\0')?szAudio2:
						EpgUtil::GetLanguageText(AudioInfo.LanguageCode2);
				} else {
					pszAudio1=szAudio1[0]!=_T('\0')?szAudio1:TEXT("主音声");
					pszAudio2=szAudio2[0]!=_T('\0')?szAudio2:TEXT("副音声");
				}
				switch (m_pCore->GetStereoMode()) {
				case 0:
					StdUtil::snprintf(szText,lengthof(szText),TEXT("%s+%s"),pszAudio1,pszAudio2);
					break;
				case 1:
					::lstrcpyn(szText,pszAudio1,lengthof(szText));
					break;
				case 2:
					::lstrcpyn(szText,pszAudio2,lengthof(szText));
					break;
				default:
					return;
				}
			} else {
				StdUtil::snprintf(szText,lengthof(szText),TEXT("音声%d (%s)"),
					m_pCore->GetAudioStream()+1,
					AudioInfo.szText[0]!=_T('\0')?AudioInfo.szText:
					EpgUtil::GetLanguageText(AudioInfo.LanguageCode));
			}
		} else {
			StdUtil::snprintf(szText,lengthof(szText),TEXT("音声%d"),m_pCore->GetAudioStream()+1);
		}

		OSDManager.ShowOSD(szText);
	}
}


inline int CalcZoomSize(int Size,int Rate,int Factor)
{
	return (Size*Rate+Factor/2)/Factor;
}

bool CMainWindow::SetZoomRate(int Rate,int Factor)
{
	if (Rate<1 || Factor<1)
		return false;

	int Width,Height;

	if (CoreEngine.GetVideoViewSize(&Width,&Height) && Width>0 && Height>0) {
		int ZoomWidth,ZoomHeight;

		ZoomWidth=CalcZoomSize(Width,Rate,Factor);
		ZoomHeight=CalcZoomSize(Height,Rate,Factor);

		if (ViewOptions.GetZoomKeepAspectRatio()) {
			SIZE ScreenSize;

			m_Viewer.GetVideoContainer().GetClientSize(&ScreenSize);
			if (ScreenSize.cx>0 && ScreenSize.cy>0) {
				if ((double)ZoomWidth/(double)ScreenSize.cx<=(double)ZoomHeight/(double)ScreenSize.cy) {
					ZoomWidth=CalcZoomSize(ScreenSize.cx,ZoomHeight,ScreenSize.cy);
				} else {
					ZoomHeight=CalcZoomSize(ScreenSize.cy,ZoomWidth,ScreenSize.cx);
				}
			}
		}

		AdjustWindowSize(ZoomWidth,ZoomHeight);
	}

	return true;
}


bool CMainWindow::GetZoomRate(int *pRate,int *pFactor)
{
	bool fOK=false;
	int Width,Height;
	int Rate=0,Factor=1;

	if (CoreEngine.GetVideoViewSize(&Width,&Height) && Width>0 && Height>0) {
		/*
		SIZE sz;

		m_Viewer.GetVideoContainer().GetClientSize(&sz);
		Rate=sz.cy;
		Factor=Height;
		*/
		WORD DstWidth,DstHeight;
		if (CoreEngine.m_DtvEngine.m_MediaViewer.GetDestSize(&DstWidth,&DstHeight)) {
			Rate=DstHeight;
			Factor=Height;
		}
		fOK=true;
	}
	if (pRate)
		*pRate=Rate;
	if (pFactor)
		*pFactor=Factor;
	return fOK;
}


int CMainWindow::GetZoomPercentage()
{
	int Rate,Factor;

	if (!GetZoomRate(&Rate,&Factor) || Factor==0)
		return 0;
	return (Rate*100+Factor/2)/Factor;
}


bool CMainWindow::SetPanAndScan(const PanAndScanInfo &Info)
{
	CMediaViewer::ClippingInfo Clipping;

	Clipping.Left=Info.XPos;
	Clipping.Right=Info.XFactor-(Info.XPos+Info.Width);
	Clipping.HorzFactor=Info.XFactor;
	Clipping.Top=Info.YPos;
	Clipping.Bottom=Info.YFactor-(Info.YPos+Info.Height);
	Clipping.VertFactor=Info.YFactor;

	CoreEngine.m_DtvEngine.m_MediaViewer.SetPanAndScan(Info.XAspect,Info.YAspect,&Clipping);

	if (!m_pCore->GetFullscreen() && !::IsZoomed(m_hwnd)) {
		switch (ViewOptions.GetPanScanAdjustWindowMode()) {
		case CViewOptions::ADJUSTWINDOW_FIT:
			{
				int ZoomRate,ZoomFactor;
				int Width,Height;

				if (GetZoomRate(&ZoomRate,&ZoomFactor)
						&& CoreEngine.GetVideoViewSize(&Width,&Height)) {
					AdjustWindowSize(CalcZoomSize(Width,ZoomRate,ZoomFactor),
									 CalcZoomSize(Height,ZoomRate,ZoomFactor));
				} else {
					WORD DstWidth,DstHeight;

					if (CoreEngine.m_DtvEngine.m_MediaViewer.GetDestSize(&DstWidth,&DstHeight))
						AdjustWindowSize(DstWidth,DstHeight);
				}
			}
			break;

		case CViewOptions::ADJUSTWINDOW_WIDTH:
			{
				SIZE sz;
				int Width,Height;

				m_Viewer.GetVideoContainer().GetClientSize(&sz);
				if (CoreEngine.GetVideoViewSize(&Width,&Height))
					AdjustWindowSize(CalcZoomSize(Width,sz.cy,Height),sz.cy);
			}
			break;
		}
	}

	StatusView.UpdateItem(STATUS_ITEM_VIDEOSIZE);
	ControlPanel.UpdateItem(CONTROLPANEL_ITEM_VIDEO);

	return true;
}


bool CMainWindow::GetPanAndScan(PanAndScanInfo *pInfo) const
{
	if (pInfo==NULL)
		return false;

	const CMediaViewer &MediaViewer=CoreEngine.m_DtvEngine.m_MediaViewer;
	CMediaViewer::ClippingInfo Clipping;

	MediaViewer.GetForceAspectRatio(&pInfo->XAspect,&pInfo->YAspect);
	MediaViewer.GetClippingInfo(&Clipping);

	pInfo->XPos=Clipping.Left;
	pInfo->YPos=Clipping.Top;
	pInfo->Width=Clipping.HorzFactor-(Clipping.Left+Clipping.Right);
	pInfo->Height=Clipping.VertFactor-(Clipping.Top+Clipping.Bottom);
	pInfo->XFactor=Clipping.HorzFactor;
	pInfo->YFactor=Clipping.VertFactor;

	return true;
}


bool CMainWindow::SetPanAndScan(int Command)
{
	PanAndScanInfo Info;
	int Type;

	if (Command>=CM_ASPECTRATIO_FIRST && Command<=CM_ASPECTRATIO_3D_LAST) {
		static const CUICore::PanAndScanInfo PanAndScanList[] = {
			{0, 0,  0,  0,  0,  0,  0,  0},	// デフォルト
			{0, 0,  1,  1,  1,  1, 16,  9},	// 16:9
			{0, 3,  1, 18,  1, 24, 16,  9},	// 16:9 レターボックス
			{2, 3, 12, 18, 16, 24, 16,  9},	// 16:9 超額縁
			{2, 0, 12,  1, 16,  1,  4,  3},	// 4:3 サイドカット
			{0, 0,  1,  1,  1,  1,  4,  3},	// 4:3
			{0, 0,  1,  1,  1,  1, 32,  9},	// 32:9
			{0, 0,  1,  1,  2,  1, 16,  9},	// 16:9 左
			{1, 0,  1,  1,  2,  1, 16,  9},	// 16:9 右
		};

		Type=Command-CM_ASPECTRATIO_FIRST;
		Info=PanAndScanList[Type];
	} else if (Command>=CM_PANANDSCAN_PRESET_FIRST && Command<=CM_PANANDSCAN_PRESET_LAST) {
		CPanAndScanOptions::PanAndScanInfo PanScan;
		if (!PanAndScanOptions.GetPreset(Command-CM_PANANDSCAN_PRESET_FIRST,&PanScan))
			return false;
		Info=PanScan.Info;
		Type=ASPECTRATIO_CUSTOM+(Command-CM_PANANDSCAN_PRESET_FIRST);
	} else {
		return false;
	}

	SetPanAndScan(Info);

	m_AspectRatioType=Type;
	m_AspectRatioResetTime=0;

	AspectRatioIconMenu.CheckRadioItem(
		CM_ASPECTRATIO_FIRST,CM_ASPECTRATIO_3D_LAST,
		m_AspectRatioType<ASPECTRATIO_CUSTOM?CM_ASPECTRATIO_FIRST+m_AspectRatioType:0);
	SideBar.CheckRadioItem(CM_ASPECTRATIO_FIRST,CM_ASPECTRATIO_LAST,
		m_AspectRatioType<ASPECTRATIO_CUSTOM?CM_ASPECTRATIO_FIRST+m_AspectRatioType:0);

	return true;
}


bool CMainWindow::ShowProgramGuide(bool fShow,unsigned int Flags,const ProgramGuideSpaceInfo *pSpaceInfo)
{
	if (fShowProgramGuide==fShow)
		return true;

	if (fShow) {
		const bool fOnScreen=
			(Flags & PROGRAMGUIDE_SHOW_POPUP)==0
			&& ((Flags & PROGRAMGUIDE_SHOW_ONSCREEN)!=0
				|| ProgramGuideOptions.GetOnScreen()
				|| (m_pCore->GetFullscreen() && ::GetSystemMetrics(SM_CMONITORS)==1));

		Util::CWaitCursor WaitCursor;

		if (fOnScreen) {
			ProgramGuideDisplay.SetEventHandler(&ProgramGuideDisplayEventHandler);
			ProgramGuideDisplay.Create(m_Viewer.GetDisplayBase().GetParent()->GetHandle(),
				WS_CHILD | WS_CLIPCHILDREN);
			m_Viewer.GetDisplayBase().SetDisplayView(&ProgramGuideDisplay);
			if (m_fCustomFrame)
				HookWindows(ProgramGuideDisplay.GetHandle());
		} else {
			ProgramGuideFrame.Create(NULL,
				WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX |
					WS_THICKFRAME | WS_CLIPCHILDREN);
		}

		SYSTEMTIME stFirst,stLast;
		ProgramGuideOptions.GetTimeRange(&stFirst,&stLast);
		g_ProgramGuide.SetTimeRange(&stFirst,&stLast);
		g_ProgramGuide.SetViewDay(CProgramGuide::DAY_TODAY);

		if (fOnScreen) {
			m_Viewer.GetDisplayBase().SetVisible(true);
		} else {
			ProgramGuideFrame.Show();
			ProgramGuideFrame.Update();
		}

		if (EpgOptions.IsEpgFileLoading()
				|| EpgOptions.IsEDCBDataLoading()) {
			g_ProgramGuide.SetMessage(TEXT("EPGファイルの読み込み中..."));
			EpgOptions.WaitEpgFileLoad();
			EpgOptions.WaitEDCBDataLoad();
			g_ProgramGuide.SetMessage(NULL);
		}

		ProgramGuideChannelProviderManager.Create(pSpaceInfo!=NULL?pSpaceInfo->pszTuner:NULL);
		int Provider=ProgramGuideChannelProviderManager.GetCurChannelProvider();
		int Space;
		if (Provider>=0) {
			if (pSpaceInfo!=NULL && pSpaceInfo->Space>=-1)
				Space=pSpaceInfo->Space;
			else
				Space=ChannelManager.GetCurrentSpace();
			if (Space<0) {
				Space=0;
			} else {
				CProgramGuideBaseChannelProvider *pChannelProvider=
					dynamic_cast<CProgramGuideBaseChannelProvider*>(
						ProgramGuideChannelProviderManager.GetChannelProvider(Provider));
				if (pChannelProvider!=NULL) {
					if (pChannelProvider->HasAllChannelGroup())
						Space++;
					if ((size_t)Space>=pChannelProvider->GetGroupCount())
						Space=0;
				}
			}
		} else {
			Space=-1;
		}
		g_ProgramGuide.SetCurrentChannelProvider(Provider,Space);
		g_ProgramGuide.UpdateProgramGuide(true);
		if (ProgramGuideOptions.ScrollToCurChannel())
			g_ProgramGuide.ScrollToCurrentService();
	} else {
		if (ProgramGuideFrame.IsCreated()) {
			ProgramGuideFrame.Destroy();
		} else {
			m_Viewer.GetDisplayBase().SetVisible(false);
		}
	}

	fShowProgramGuide=fShow;

	MainMenu.CheckItem(CM_PROGRAMGUIDE,fShowProgramGuide);
	SideBar.CheckItem(CM_PROGRAMGUIDE,fShowProgramGuide);

	return true;
}


DWORD WINAPI CMainWindow::ExitWatchThread(LPVOID lpParameter)
{
	HANDLE hEvent=lpParameter;

	if (::WaitForSingleObject(hEvent,60000)!=WAIT_OBJECT_0) {
		Logger.AddLog(TEXT("終了処理がタイムアウトしました。プロセスを強制的に終了させます。"));
		::ExitProcess(-1);
	}
	return 0;
}


LRESULT CALLBACK CMainWindow::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	CMainWindow *pThis;

	if (uMsg==WM_NCCREATE) {
		pThis=static_cast<CMainWindow*>(CBasicWindow::OnCreate(hwnd,lParam));
	} else {
		pThis=static_cast<CMainWindow*>(GetBasicWindow(hwnd));
	}

	if (pThis==NULL) {
		return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
	}

	if (uMsg==WM_CREATE) {
		if (!pThis->OnCreate(reinterpret_cast<LPCREATESTRUCT>(lParam)))
			return -1;
		return 0;
	}

	LRESULT Result=0;
	if (PluginManager.OnMessage(hwnd,uMsg,wParam,lParam,&Result))
		return Result;

	if (uMsg==WM_DESTROY) {
		pThis->OnMessage(hwnd,uMsg,wParam,lParam);
		pThis->OnDestroy();
		::PostQuitMessage(0);
		return 0;
	}

	return pThis->OnMessage(hwnd,uMsg,wParam,lParam);
}


void CMainWindow::HookWindows(HWND hwnd)
{
	if (hwnd!=NULL) {
		HookChildWindow(hwnd);
		for (hwnd=::GetWindow(hwnd,GW_CHILD);hwnd!=NULL;hwnd=::GetWindow(hwnd,GW_HWNDNEXT))
			HookWindows(hwnd);
	}
}


#define CHILD_PROP_THIS	APP_NAME TEXT("ChildThis")

void CMainWindow::HookChildWindow(HWND hwnd)
{
	if (hwnd==NULL)
		return;

	if (m_atomChildOldWndProcProp==0) {
		m_atomChildOldWndProcProp=::GlobalAddAtom(APP_NAME TEXT("ChildOldWndProc"));
		if (m_atomChildOldWndProcProp==0)
			return;
	}

	if (::GetProp(hwnd,MAKEINTATOM(m_atomChildOldWndProcProp))==NULL) {
#ifdef _DEBUG
		TCHAR szClass[256];
		::GetClassName(hwnd,szClass,lengthof(szClass));
		TRACE(TEXT("Hook window %p \"%s\"\n"),hwnd,szClass);
#endif
		WNDPROC pOldWndProc=SubclassWindow(hwnd,ChildHookProc);
		::SetProp(hwnd,MAKEINTATOM(m_atomChildOldWndProcProp),pOldWndProc);
		::SetProp(hwnd,CHILD_PROP_THIS,this);
	}
}


LRESULT CALLBACK CMainWindow::ChildHookProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	WNDPROC pOldWndProc=static_cast<WNDPROC>(::GetProp(hwnd,MAKEINTATOM(m_atomChildOldWndProcProp)));

	if (pOldWndProc==NULL)
		return ::DefWindowProc(hwnd,uMsg,wParam,lParam);

	switch (uMsg) {
	case WM_NCHITTEST:
		{
			CMainWindow *pThis=static_cast<CMainWindow*>(::GetProp(hwnd,CHILD_PROP_THIS));

			if (pThis!=NULL && pThis->m_fCustomFrame && ::GetAncestor(hwnd,GA_ROOT)==pThis->m_hwnd) {
				POINT pt={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
				RECT rc;

				pThis->GetScreenPosition(&rc);
				if (::PtInRect(&rc,pt)) {
					int FrameWidth=max(::GetSystemMetrics(SM_CXSIZEFRAME),pThis->m_CustomFrameWidth);
					int FrameHeight=max(::GetSystemMetrics(SM_CYSIZEFRAME),pThis->m_CustomFrameWidth);
					int Code=HTNOWHERE;

					if (pt.x<rc.left+FrameWidth) {
						if (pt.y<rc.top+FrameHeight)
							Code=HTTOPLEFT;
						else if (pt.y>=rc.bottom-FrameHeight)
							Code=HTBOTTOMLEFT;
						else
							Code=HTLEFT;
					} else if (pt.x>=rc.right-FrameWidth) {
						if (pt.y<rc.top+FrameHeight)
							Code=HTTOPRIGHT;
						else if (pt.y>=rc.bottom-FrameHeight)
							Code=HTBOTTOMRIGHT;
						else
							Code=HTRIGHT;
					} else if (pt.y<rc.top+FrameHeight) {
						Code=HTTOP;
					} else if (pt.y>=rc.bottom-FrameHeight) {
						Code=HTBOTTOM;
					}
					if (Code!=HTNOWHERE) {
						return Code;
					}
				}
			}
		}
		break;

	case WM_NCLBUTTONDOWN:
		{
			CMainWindow *pThis=static_cast<CMainWindow*>(::GetProp(hwnd,CHILD_PROP_THIS));

			if (pThis!=NULL && pThis->m_fCustomFrame && ::GetAncestor(hwnd,GA_ROOT)==pThis->m_hwnd) {
				BYTE Flag=0;

				switch (wParam) {
				case HTTOP:			Flag=WMSZ_TOP;			break;
				case HTTOPLEFT:		Flag=WMSZ_TOPLEFT;		break;
				case HTTOPRIGHT:	Flag=WMSZ_TOPRIGHT;		break;
				case HTLEFT:		Flag=WMSZ_LEFT;			break;
				case HTRIGHT:		Flag=WMSZ_RIGHT;		break;
				case HTBOTTOM:		Flag=WMSZ_BOTTOM;		break;
				case HTBOTTOMLEFT:	Flag=WMSZ_BOTTOMLEFT;	break;
				case HTBOTTOMRIGHT:	Flag=WMSZ_BOTTOMRIGHT;	break;
				}

				if (Flag!=0) {
					::SendMessage(pThis->m_hwnd,WM_SYSCOMMAND,SC_SIZE | Flag,lParam);
					return 0;
				}
			}
		}
		break;

	case WM_DESTROY:
#ifdef _DEBUG
		{
			TCHAR szClass[256];
			::GetClassName(hwnd,szClass,lengthof(szClass));
			TRACE(TEXT("Unhook window %p \"%s\"\n"),hwnd,szClass);
		}
#endif
		::SetWindowLongPtr(hwnd,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(pOldWndProc));
		::RemoveProp(hwnd,MAKEINTATOM(m_atomChildOldWndProcProp));
		::RemoveProp(hwnd,CHILD_PROP_THIS);
		break;
	}

	return ::CallWindowProc(pOldWndProc,hwnd,uMsg,wParam,lParam);
}


void CMainWindow::SetWindowVisible()
{
	bool fRestore=false,fShow=false;

	if ((ResidentManager.GetStatus()&CResidentManager::STATUS_MINIMIZED)!=0) {
		ResidentManager.SetStatus(0,CResidentManager::STATUS_MINIMIZED);
		ResidentManager.SetMinimizeToTray(ViewOptions.GetMinimizeToTray());
		fRestore=true;
	}
	if (!GetVisible()) {
		SetVisible(true);
		ForegroundWindow(m_hwnd);
		Update();
		fShow=true;
	}
	if (::IsIconic(m_hwnd)) {
		::ShowWindow(m_hwnd,SW_RESTORE);
		Update();
		fRestore=true;
	} else if (!fShow) {
		ForegroundWindow(m_hwnd);
	}
	if (m_fMinimizeInit) {
		// 最小化状態での起動後最初の表示
		ShowFloatingWindows(true);
		m_fMinimizeInit=false;
	}
	if (fRestore) {
		ResumeViewer(ResumeInfo::VIEWERSUSPEND_MINIMIZE);
	}
}


void CMainWindow::ShowFloatingWindows(bool fShow)
{
	if (fShowPanelWindow && PanelFrame.GetFloating()) {
		PanelFrame.SetPanelVisible(fShow);
		if (fShow)
			PanelFrame.Update();
	}
	if (fShowProgramGuide)
		ProgramGuideFrame.SetVisible(fShow);
	if (CaptureWindow.IsCreated())
		CaptureWindow.SetVisible(fShow);
	if (StreamInfo.IsCreated())
		StreamInfo.SetVisible(fShow);
}


bool CMainWindow::OnStandbyChange(bool fStandby)
{
	if (fStandby) {
		if (m_fStandbyInit)
			return true;
		Logger.AddLog(TEXT("待機状態に移行します。"));
		SuspendViewer(ResumeInfo::VIEWERSUSPEND_STANDBY);
		//FinalizeViewer();
		m_Resume.fFullscreen=m_pCore->GetFullscreen();
		if (m_Resume.fFullscreen)
			m_pCore->SetFullscreen(false);
		ShowFloatingWindows(false);
		SetVisible(false);
		PluginManager.SendStandbyEvent(true);

		if (!m_fProgramGuideUpdating) {
			StoreTunerResumeInfo();

			if (EpgOptions.GetUpdateWhenStandby()
					&& CoreEngine.IsTunerOpen()
					&& !RecordManager.IsRecording()
					&& !CoreEngine.IsNetworkDriver()
					&& !CmdLineOptions.m_fNoEpg)
				BeginProgramGuideUpdate(NULL,NULL,true);

			if (!RecordManager.IsRecording() && !m_fProgramGuideUpdating)
				AppMain.CloseTuner();
		}
	} else {
		Logger.AddLog(TEXT("待機状態から復帰します。"));
		SetWindowVisible();
		Util::CWaitCursor WaitCursor;
		if (m_fStandbyInit) {
			bool fSetChannel=m_Resume.fSetChannel;
			m_Resume.fSetChannel=false;
			ResumeTuner();
			m_Resume.fSetChannel=fSetChannel;
			AppMain.InitializeChannel();
			CoreEngine.m_DtvEngine.SetTracer(&StatusView);
			InitializeViewer();
			CoreEngine.m_DtvEngine.SetTracer(NULL);
			StatusView.SetSingleText(NULL);
			if (!GeneralOptions.GetResident())
				ResidentManager.SetResident(false);
			m_fStandbyInit=false;
		}
		if (m_Resume.fFullscreen)
			m_pCore->SetFullscreen(true);
		ShowFloatingWindows(true);
		ForegroundWindow(m_hwnd);
		PluginManager.SendStandbyEvent(false);
		ResumeTuner();
		ResumeViewer(ResumeInfo::VIEWERSUSPEND_STANDBY);
	}

	return true;
}


bool CMainWindow::InitStandby()
{
	if (!CmdLineOptions.m_fNoDirectShow && !CmdLineOptions.m_fNoView
			&& (!PlaybackOptions.GetRestorePlayStatus() || fEnablePlay)) {
		m_Resume.fEnableViewer=true;
		m_Resume.ViewerSuspendFlags=ResumeInfo::VIEWERSUSPEND_STANDBY;
	}
	m_Resume.fFullscreen=CmdLineOptions.m_fFullscreen;

	if (CoreEngine.IsDriverSpecified())
		m_Resume.fOpenTuner=true;

	if (RestoreChannelInfo.Space>=0 && RestoreChannelInfo.Channel>=0) {
		int Space=RestoreChannelInfo.fAllChannels?CChannelManager::SPACE_ALL:RestoreChannelInfo.Space;
		const CChannelList *pList=ChannelManager.GetChannelList(Space);
		if (pList!=NULL) {
			int Index=pList->Find(RestoreChannelInfo.Space,
								  RestoreChannelInfo.Channel,
								  RestoreChannelInfo.ServiceID);
			if (Index>=0) {
				m_Resume.Channel.SetSpace(Space);
				m_Resume.Channel.SetChannel(Index);
				m_Resume.Channel.SetServiceID(RestoreChannelInfo.ServiceID);
				m_Resume.fSetChannel=true;
			}
		}
	}

	ResidentManager.SetResident(true);
	m_fStandbyInit=true;
	m_pCore->SetStandby(true);

	return true;
}


bool CMainWindow::InitMinimize()
{
	if (!CmdLineOptions.m_fNoDirectShow && !CmdLineOptions.m_fNoView
			&& (!PlaybackOptions.GetRestorePlayStatus() || fEnablePlay)) {
		m_Resume.fEnableViewer=true;
		m_Resume.ViewerSuspendFlags=ResumeInfo::VIEWERSUSPEND_MINIMIZE;
	}

	ResidentManager.SetStatus(CResidentManager::STATUS_MINIMIZED,
							  CResidentManager::STATUS_MINIMIZED);
	if (!ResidentManager.GetMinimizeToTray())
		::ShowWindow(m_hwnd,SW_SHOWMINNOACTIVE);

	m_fMinimizeInit=true;

	return true;
}


bool CMainWindow::IsMinimizeToTray() const
{
	return ResidentManager.GetMinimizeToTray()
		&& (ResidentManager.GetStatus()&CResidentManager::STATUS_MINIMIZED)!=0;
}


void CMainWindow::StoreTunerResumeInfo()
{
	m_Resume.Channel.Store(&ChannelManager);
	m_Resume.fSetChannel=m_Resume.Channel.IsValid();
	m_Resume.fOpenTuner=CoreEngine.IsTunerOpen();
}


bool CMainWindow::ResumeTuner()
{
	if (m_fProgramGuideUpdating)
		EndProgramGuideUpdate(0);

	if (m_Resume.fOpenTuner) {
		m_Resume.fOpenTuner=false;
		if (!AppMain.OpenTuner()) {
			m_Resume.fSetChannel=false;
			return false;
		}
	}

	ResumeChannel();

	return true;
}


void CMainWindow::ResumeChannel()
{
	if (m_Resume.fSetChannel) {
		if (CoreEngine.IsTunerOpen()
				&& !RecordManager.IsRecording()) {
			AppMain.SetChannel(m_Resume.Channel.GetSpace(),
							   m_Resume.Channel.GetChannel(),
							   m_Resume.Channel.GetServiceID());
		}
		m_Resume.fSetChannel=false;
	}
}


void CMainWindow::SuspendViewer(unsigned int Flags)
{
	if (IsViewerEnabled()) {
		TRACE(TEXT("Suspend viewer\n"));
		m_pCore->EnableViewer(false);
		m_Resume.fEnableViewer=true;
	}
	m_Resume.ViewerSuspendFlags|=Flags;
}


void CMainWindow::ResumeViewer(unsigned int Flags)
{
	if ((m_Resume.ViewerSuspendFlags & Flags)!=0) {
		m_Resume.ViewerSuspendFlags&=~Flags;
		if (m_Resume.ViewerSuspendFlags==0) {
			if (m_Resume.fEnableViewer) {
				TRACE(TEXT("Resume viewer\n"));
				m_pCore->EnableViewer(true);
				m_Resume.fEnableViewer=false;
			}
		}
	}
}


bool CMainWindow::ConfirmExit()
{
	return RecordOptions.ConfirmExit(GetVideoHostWindow(),&RecordManager);
}


bool CMainWindow::CommandLineRecord(LPCTSTR pszFileName,const FILETIME *pStartTime,int Delay,int Duration)
{
	CRecordManager::TimeSpecInfo StartTime,StopTime;

	if (pStartTime!=NULL && (pStartTime->dwLowDateTime!=0 || pStartTime->dwHighDateTime!=0)) {
		StartTime.Type=CRecordManager::TIME_DATETIME;
		StartTime.Time.DateTime=*pStartTime;
		if (Delay!=0)
			StartTime.Time.DateTime+=(LONGLONG)Delay*FILETIME_SECOND;
		SYSTEMTIME st;
		::FileTimeToSystemTime(pStartTime,&st);
		AppMain.AddLog(TEXT("コマンドラインから録画指定されました。(%d/%d/%d %d:%02d:%02d 開始)"),
					   st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
	} else if (Delay>0) {
		StartTime.Type=CRecordManager::TIME_DURATION;
		StartTime.Time.Duration=Delay*1000;
		AppMain.AddLog(TEXT("コマンドラインから録画指定されました。(%d 秒後開始)"),Delay);
	} else {
		StartTime.Type=CRecordManager::TIME_NOTSPECIFIED;
		AppMain.AddLog(TEXT("コマンドラインから録画指定されました。"));
	}
	if (Duration>0) {
		StopTime.Type=CRecordManager::TIME_DURATION;
		StopTime.Time.Duration=Duration*1000;
	} else {
		StopTime.Type=CRecordManager::TIME_NOTSPECIFIED;
	}
	return AppMain.StartRecord(
		IsStringEmpty(pszFileName)?NULL:pszFileName,
		&StartTime,&StopTime,
		CRecordManager::CLIENT_COMMANDLINE);
}


static bool SetCommandLineChannel(const CCommandLineOptions *pCmdLine)
{
#ifdef NETWORK_REMOCON_SUPPORT
	if (ChannelManager.IsNetworkRemoconMode()) {
		if (pCmdLine->m_ControllerChannel==0)
			return false;
		if (ChannelManager.GetCurrentChannelList()->FindChannelNo(pCmdLine->m_ControllerChannel)>=0) {
			MainWindow.SendCommand(CM_CHANNELNO_FIRST+pCmdLine->m_ControllerChannel-1);
			return true;
		}
		return false;
	}
#endif

	const CChannelList *pChannelList;

	for (int i=0;(pChannelList=ChannelManager.GetChannelList(i))!=NULL;i++) {
		if (pCmdLine->m_TuningSpace<0 || i==pCmdLine->m_TuningSpace) {
			for (int j=0;j<pChannelList->NumChannels();j++) {
				const CChannelInfo *pChannelInfo=pChannelList->GetChannelInfo(j);

				if ((pCmdLine->m_Channel==0
						|| pCmdLine->m_Channel==pChannelInfo->GetPhysicalChannel())
					&& (pCmdLine->m_ControllerChannel==0
						|| pCmdLine->m_ControllerChannel==pChannelInfo->GetChannelNo())
					&& (pCmdLine->m_ServiceID==0
						|| pCmdLine->m_ServiceID==pChannelInfo->GetServiceID())
					&& (pCmdLine->m_NetworkID==0
						|| pCmdLine->m_NetworkID==pChannelInfo->GetNetworkID())
					&& (pCmdLine->m_TransportStreamID==0
						|| pCmdLine->m_TransportStreamID==pChannelInfo->GetTransportStreamID())) {
					return AppMain.SetChannel(i,j);
				}
			}
		}
	}

	if (pCmdLine->m_ServiceID>0
			&& (pCmdLine->m_Channel>0 || pCmdLine->m_ControllerChannel>0
				|| pCmdLine->m_NetworkID>0 || pCmdLine->m_TransportStreamID>0)) {
		for (int i=0;(pChannelList=ChannelManager.GetChannelList(i))!=NULL;i++) {
			if (pCmdLine->m_TuningSpace<0 || i==pCmdLine->m_TuningSpace) {
				for (int j=0;j<pChannelList->NumChannels();j++) {
					const CChannelInfo *pChannelInfo=pChannelList->GetChannelInfo(j);

					if ((pCmdLine->m_Channel==0
							|| pCmdLine->m_Channel==pChannelInfo->GetPhysicalChannel())
						&& (pCmdLine->m_ControllerChannel==0
							|| pCmdLine->m_ControllerChannel==pChannelInfo->GetChannelNo())
						&& (pCmdLine->m_NetworkID==0
							|| pCmdLine->m_NetworkID==pChannelInfo->GetNetworkID())
						&& (pCmdLine->m_TransportStreamID==0
							|| pCmdLine->m_TransportStreamID==pChannelInfo->GetTransportStreamID())) {
						return AppMain.SetChannel(i,j,pCmdLine->m_ServiceID);
					}
				}
			}
		}
	}

	Logger.AddLog(TEXT("コマンドラインで指定されたチャンネルが見付かりません。"));

	return false;
}


bool CMainWindow::OnExecute(LPCTSTR pszCmdLine)
{
	CCommandLineOptions CmdLine;

	PluginManager.SendExecuteEvent(pszCmdLine);

	CmdLine.Parse(pszCmdLine);

	if (!CmdLine.m_fMinimize && !CmdLine.m_fTray) {
		SendCommand(CM_SHOW);

		if (CmdLine.m_fFullscreen)
			m_pCore->SetFullscreen(true);
		else if (CmdLine.m_fMaximize)
			SetMaximize(true);
	}

	if (CmdLine.m_fSilent || CmdLine.m_TvRockDID>=0)
		AppMain.SetSilent(true);
	if (CmdLine.m_fSaveLog)
		CmdLineOptions.m_fSaveLog=true;

	if (!CmdLine.m_DriverName.IsEmpty()) {
		if (AppMain.OpenTuner(CmdLine.m_DriverName.Get())) {
			if (CmdLine.IsChannelSpecified())
				SetCommandLineChannel(&CmdLine);
			else
				AppMain.RestoreChannel();
		}
	} else {
		if (CmdLine.IsChannelSpecified())
			SetCommandLineChannel(&CmdLine);
	}

	if (CmdLine.m_fRecord) {
		if (CmdLine.m_fRecordCurServiceOnly)
			CmdLineOptions.m_fRecordCurServiceOnly=true;
		CommandLineRecord(CmdLine.m_RecordFileName.Get(),
						  &CmdLine.m_RecordStartTime,
						  CmdLine.m_RecordDelay,
						  CmdLine.m_RecordDuration);
	} else if (CmdLine.m_fRecordStop) {
		AppMain.StopRecord();
	}

	if (CmdLine.m_Volume>=0)
		m_pCore->SetVolume(min(CmdLine.m_Volume,CCoreEngine::MAX_VOLUME),false);
	if (CmdLine.m_fMute)
		m_pCore->SetMute(true);

	if (CmdLine.m_fShowProgramGuide)
		ShowProgramGuide(true);
	if (CmdLine.m_fHomeDisplay)
		PostCommand(CM_HOMEDISPLAY);
	else if (CmdLine.m_fChannelDisplay)
		PostCommand(CM_CHANNELDISPLAY);

	return true;
}


bool CMainWindow::BeginChannelNoInput(int Digits)
{
	if (m_ChannelNoInput.fInputting)
		EndChannelNoInput();

	if (Digits<1 || Digits>5)
		return false;

	TRACE(TEXT("チャンネル番号%d桁入力開始\n"),Digits);

	m_ChannelNoInput.fInputting=true;
	m_ChannelNoInput.Digits=Digits;
	m_ChannelNoInput.CurDigit=0;
	m_ChannelNoInput.Number=0;

	m_ChannelNoInputTimer.Begin(m_hwnd,m_ChannelNoInputTimeout);

	if (OSDOptions.IsOSDEnabled(COSDOptions::OSD_CHANNELNOINPUT)) {
		OSDManager.ShowOSD(TEXT("---"),COSDManager::SHOW_NO_FADE);
	}

	return true;
}


void CMainWindow::EndChannelNoInput()
{
	if (m_ChannelNoInput.fInputting) {
		TRACE(TEXT("チャンネル番号入力終了\n"));
		m_ChannelNoInput.fInputting=false;
		m_ChannelNoInputTimer.End();
		OSDManager.ClearOSD();
	}
}


bool CMainWindow::OnChannelNoInput(int Number)
{
	if (!m_ChannelNoInput.fInputting
			|| Number<0 || Number>9)
		return false;

	m_ChannelNoInput.Number=m_ChannelNoInput.Number*10+Number;
	m_ChannelNoInput.CurDigit++;

	if (OSDOptions.IsOSDEnabled(COSDOptions::OSD_CHANNELNOINPUT)) {
		TCHAR szText[8];
		int Number=m_ChannelNoInput.Number;
		for (int i=m_ChannelNoInput.Digits-1;i>=0;i--) {
			if (i<m_ChannelNoInput.CurDigit) {
				szText[i]=Number%10+_T('0');
				Number/=10;
			} else {
				szText[i]=_T('-');
			}
		}
		szText[m_ChannelNoInput.Digits]=_T('\0');
		OSDManager.ShowOSD(szText,COSDManager::SHOW_NO_FADE);
	}

	if (m_ChannelNoInput.CurDigit<m_ChannelNoInput.Digits) {
		m_ChannelNoInputTimer.Begin(m_hwnd,m_ChannelNoInputTimeout);
	} else {
		EndChannelNoInput();

		if (m_ChannelNoInput.Number>0) {
			const CChannelList *pChannelList=ChannelManager.GetCurrentChannelList();
			if (pChannelList!=NULL) {
				int Index=pChannelList->FindChannelNo(m_ChannelNoInput.Number);
				if (Index<0 && m_ChannelNoInput.Number<=0xFFFF)
					Index=pChannelList->FindServiceID((WORD)m_ChannelNoInput.Number);
				if (Index>=0)
					SendCommand(CM_CHANNEL_FIRST+Index);
			}
		}
	}

	return true;
}


bool CMainWindow::BeginProgramGuideUpdate(LPCTSTR pszBonDriver,const CChannelList *pChannelList,bool fStandby)
{
	if (!m_fProgramGuideUpdating) {
		if (CmdLineOptions.m_fNoEpg) {
			if (!fStandby)
				ShowMessage(TEXT("コマンドラインオプションでEPG情報を取得しないように指定されているため、\n番組表の取得ができません。"),
							TEXT("お知らせ"),MB_OK | MB_ICONINFORMATION);
			return false;
		}
		if (RecordManager.IsRecording()) {
			if (!fStandby)
				ShowMessage(TEXT("録画中は番組表の取得を行えません。"),
							TEXT("お知らせ"),MB_OK | MB_ICONINFORMATION);
			return false;
		}

		const bool fTunerOpen=CoreEngine.IsTunerOpen();

		if (!IsStringEmpty(pszBonDriver)) {
			if (!AppMain.OpenTuner(pszBonDriver))
				return false;
		}

		if (pChannelList==NULL) {
			pChannelList=ChannelManager.GetCurrentRealChannelList();
			if (pChannelList==NULL) {
				if (!fTunerOpen)
					AppMain.CloseTuner();
				return false;
			}
		}
		m_EpgUpdateChannelList.clear();
		for (int i=0;i<pChannelList->NumChannels();i++) {
			const CChannelInfo *pChInfo=pChannelList->GetChannelInfo(i);

			if (pChInfo->IsEnabled()) {
				const NetworkType Network=GetNetworkType(pChInfo->GetNetworkID());
				std::vector<EpgChannelGroup>::iterator itr;

				for (itr=m_EpgUpdateChannelList.begin();itr!=m_EpgUpdateChannelList.end();++itr) {
					if (pChInfo->GetSpace()==itr->Space && pChInfo->GetChannelIndex()==itr->Channel)
						break;
					if (pChInfo->GetNetworkID()==itr->ChannelList.GetChannelInfo(0)->GetNetworkID()
							&& ((Network==NETWORK_BS && !EpgOptions.GetUpdateBSExtended())
							 || (Network==NETWORK_CS && !EpgOptions.GetUpdateCSExtended())))
						break;
				}
				if (itr==m_EpgUpdateChannelList.end()) {
					m_EpgUpdateChannelList.push_back(EpgChannelGroup());
					itr=m_EpgUpdateChannelList.end();
					--itr;
					itr->Space=pChInfo->GetSpace();
					itr->Channel=pChInfo->GetChannelIndex();
					itr->Time=0;
				}
				itr->ChannelList.AddChannel(*pChInfo);
			}
		}
		if (m_EpgUpdateChannelList.empty()) {
			if (!fTunerOpen)
				AppMain.CloseTuner();
			return false;
		}

		if (m_pCore->GetStandby()) {
			if (!AppMain.OpenTuner())
				return false;
		} else {
			if (!CoreEngine.IsTunerOpen())
				return false;
			if (!fStandby) {
				StoreTunerResumeInfo();
				m_Resume.fOpenTuner=fTunerOpen;
			}
		}

		Logger.AddLog(TEXT("番組表の取得を開始します。"));
		m_fProgramGuideUpdating=true;
		SuspendViewer(ResumeInfo::VIEWERSUSPEND_EPGUPDATE);
		m_EpgUpdateCurChannel=-1;
		SetEpgUpdateNextChannel();
	}

	return true;
}


void CMainWindow::OnProgramGuideUpdateEnd(unsigned int Flags)
{
	if (m_fProgramGuideUpdating) {
		HANDLE hThread;
		int OldPriority;

		Logger.AddLog(TEXT("番組表の取得を終了します。"));
		::KillTimer(m_hwnd,TIMER_ID_PROGRAMGUIDEUPDATE);
		m_fProgramGuideUpdating=false;
		m_EpgUpdateChannelList.clear();
		if (m_pCore->GetStandby()) {
			hThread=::GetCurrentThread();
			OldPriority=::GetThreadPriority(hThread);
			::SetThreadPriority(hThread,THREAD_PRIORITY_LOWEST);
		} else {
			::SetCursor(::LoadCursor(NULL,IDC_WAIT));
		}
		EpgProgramList.UpdateProgramList();
		EpgOptions.SaveEpgFile(&EpgProgramList);
		if (m_pCore->GetStandby()) {
			g_ProgramGuide.SendMessage(WM_COMMAND,CM_PROGRAMGUIDE_REFRESH,0);
			::SetThreadPriority(hThread,OldPriority);
			if ((Flags&EPG_UPDATE_END_CLOSE_TUNER)!=0)
				AppMain.CloseTuner();
		} else {
			::SetCursor(::LoadCursor(NULL,IDC_ARROW));
			if ((Flags&EPG_UPDATE_END_RESUME)!=0)
				ResumeChannel();
			if (fShowPanelWindow && PanelForm.GetCurPageID()==PANEL_ID_CHANNEL)
				ChannelPanel.UpdateAllChannels(false);
		}
		ResumeViewer(ResumeInfo::VIEWERSUSPEND_EPGUPDATE);
	}
}


void CMainWindow::EndProgramGuideUpdate(unsigned int Flags)
{
	OnProgramGuideUpdateEnd(Flags);

	if (g_ProgramGuide.IsCreated())
		g_ProgramGuide.SendMessage(WM_COMMAND,CM_PROGRAMGUIDE_ENDUPDATE,0);
}


bool CMainWindow::SetEpgUpdateNextChannel()
{
	size_t i;

	for (i=m_EpgUpdateCurChannel+1;i<m_EpgUpdateChannelList.size();i++) {
		const EpgChannelGroup &ChGroup=m_EpgUpdateChannelList[i];

		m_fEpgUpdateChannelChange=true;
		bool fOK=AppMain.SetChannelByIndex(ChGroup.Space,ChGroup.Channel);
		m_fEpgUpdateChannelChange=false;
		if (fOK) {
			::SetTimer(m_hwnd,TIMER_ID_PROGRAMGUIDEUPDATE,30000,NULL);
			m_EpgUpdateCurChannel=(int)i;
			m_EpgAccumulateClock.Start();

			// TODO: 残り時間をちゃんと算出する
			DWORD Time=0;
			for (size_t j=i;j<m_EpgUpdateChannelList.size();j++) {
				WORD NetworkID=m_EpgUpdateChannelList[j].ChannelList.GetChannelInfo(0)->GetNetworkID();
				if (IsBSNetworkID(NetworkID) || IsCSNetworkID(NetworkID))
					Time+=180000;
				else
					Time+=60000;
			}
			g_ProgramGuide.SetEpgUpdateProgress(
				m_EpgUpdateCurChannel,(int)m_EpgUpdateChannelList.size(),Time);
			return true;
		}
	}

	EndProgramGuideUpdate();
	return false;
}


void CMainWindow::UpdatePanel()
{
	switch (PanelForm.GetCurPageID()) {
	case PANEL_ID_INFORMATION:
		{
			BYTE AspectX,AspectY;
			if (CoreEngine.m_DtvEngine.m_MediaViewer.GetEffectiveAspectRatio(&AspectX,&AspectY))
				InfoPanel.SetAspectRatio(AspectX,AspectY);
			if (RecordManager.IsRecording()) {
				const CRecordTask *pRecordTask=RecordManager.GetRecordTask();
				InfoPanel.SetRecordStatus(true,pRecordTask->GetFileName(),
					pRecordTask->GetWroteSize(),pRecordTask->GetRecordTime());
			}
			UpdateProgramInfo();
		}
		break;

	case PANEL_ID_PROGRAMLIST:
		if (m_ProgramListUpdateTimerCount>0) {
			int ServiceID=ChannelManager.GetCurrentServiceID();

			if (ServiceID<=0) {
				WORD SID;
				if (CoreEngine.m_DtvEngine.GetServiceID(&SID))
					ServiceID=SID;
			}
			if (ServiceID>0) {
				WORD NetworkID=CoreEngine.m_DtvEngine.m_TsAnalyzer.GetNetworkID();
				WORD TransportStreamID=CoreEngine.m_DtvEngine.m_TsAnalyzer.GetTransportStreamID();

				EpgProgramList.UpdateService(NetworkID,TransportStreamID,(WORD)ServiceID);
				ProgramListPanel.UpdateProgramList(NetworkID,TransportStreamID,(WORD)ServiceID);
			}
		}
		break;

	case PANEL_ID_CHANNEL:
		RefreshChannelPanel();
		break;
	}
}


void CMainWindow::RefreshChannelPanel()
{
	Util::CWaitCursor WaitCursor;

	if (ChannelPanel.IsChannelListEmpty()) {
		ChannelPanel.SetChannelList(ChannelManager.GetCurrentChannelList(),
									!EpgOptions.IsEpgFileLoading());
	} else {
		if (!EpgOptions.IsEpgFileLoading())
			ChannelPanel.UpdateAllChannels(false);
	}
	ChannelPanel.SetCurrentChannel(ChannelManager.GetCurrentChannel());
}


void CMainWindow::ApplyColorScheme(const CColorScheme *pColorScheme)
{
	Theme::BorderInfo Border;

	m_LayoutBase.SetBackColor(pColorScheme->GetColor(CColorScheme::COLOR_SPLITTER));
	pColorScheme->GetBorderInfo(CColorScheme::BORDER_SCREEN,&Border);
	if (!m_fViewWindowEdge)
		Border.Type=Theme::BORDER_NONE;
	m_Viewer.GetViewWindow().SetBorder(&Border);

	CTitleBar::ThemeInfo TitleBarTheme;
	pColorScheme->GetStyle(CColorScheme::STYLE_TITLEBARCAPTION,
						   &TitleBarTheme.CaptionStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_TITLEBARICON,
						   &TitleBarTheme.IconStyle);
	pColorScheme->GetStyle(CColorScheme::STYLE_TITLEBARHIGHLIGHTITEM,
						   &TitleBarTheme.HighlightIconStyle);
	pColorScheme->GetBorderInfo(CColorScheme::BORDER_TITLEBAR,
								&TitleBarTheme.Border);
	m_TitleBar.SetTheme(&TitleBarTheme);
}


bool CMainWindow::SetLogo(LPCTSTR pszFileName)
{
	if (IsStringEmpty(pszFileName))
		return m_Viewer.GetViewWindow().SetLogo(NULL);

	TCHAR szFileName[MAX_PATH];

	if (::PathIsRelative(pszFileName)) {
		TCHAR szTemp[MAX_PATH];
		AppMain.GetAppDirectory(szTemp);
		::PathAppend(szTemp,pszFileName);
		::PathCanonicalize(szFileName,szTemp);
	} else {
		::lstrcpy(szFileName,pszFileName);
	}
	HBITMAP hbm=static_cast<HBITMAP>(::LoadImage(NULL,szFileName,IMAGE_BITMAP,
								0,0,LR_LOADFROMFILE | LR_CREATEDIBSECTION));
	if (hbm==NULL)
		return false;
	return m_Viewer.GetViewWindow().SetLogo(hbm);
}


bool CMainWindow::SetViewWindowEdge(bool fEdge)
{
	if (m_fViewWindowEdge!=fEdge) {
		const CColorScheme *pColorScheme=ColorSchemeOptions.GetColorScheme();
		Theme::BorderInfo Border;

		pColorScheme->GetBorderInfo(CColorScheme::BORDER_SCREEN,&Border);
		if (!fEdge)
			Border.Type=Theme::BORDER_NONE;
		m_Viewer.GetViewWindow().SetBorder(&Border);
		m_fViewWindowEdge=fEdge;
	}
	return true;
}


bool CMainWindow::GetOSDWindow(HWND *phwndParent,RECT *pRect,bool *pfForcePseudoOSD)
{
	if (!GetVisible() || ::IsIconic(m_hwnd))
		return false;
	if (m_Viewer.GetVideoContainer().GetVisible()) {
		*phwndParent=m_Viewer.GetVideoContainer().GetHandle();
	} else {
		*phwndParent=m_Viewer.GetVideoContainer().GetParent();
		*pfForcePseudoOSD=true;
	}
	::GetClientRect(*phwndParent,pRect);
	pRect->top+=NotificationBar.GetBarHeight();
	pRect->bottom-=StatusView.GetHeight();
	return true;
}


bool CMainWindow::SetOSDHideTimer(DWORD Delay)
{
	return ::SetTimer(m_hwnd,TIMER_ID_OSD,Delay,NULL)!=0;
}


CStatusView *CMainWindow::GetStatusView() const
{
	return &StatusView;
}




CMainWindow::CDisplayBaseEventHandler::CDisplayBaseEventHandler(CMainWindow *pMainWindow)
	: m_pMainWindow(pMainWindow)
{
}

bool CMainWindow::CDisplayBaseEventHandler::OnVisibleChange(bool fVisible)
{
	if (!m_pMainWindow->IsViewerEnabled()) {
		m_pMainWindow->m_Viewer.GetVideoContainer().SetVisible(fVisible);
	}
	return true;
}




// アクセラレータにしないメッセージの判定
static bool IsNoAcceleratorMessage(const MSG *pmsg)
{
	HWND hwnd=::GetFocus();

	if (hwnd!=NULL && ::IsWindowVisible(hwnd)) {
		CDisplayView *pDisplayView=MainWindow.GetDisplayBase().GetDisplayView();

		if (pDisplayView!=NULL && hwnd==pDisplayView->GetHandle()) {
			return pDisplayView->IsMessageNeed(pmsg);
		} else if (pmsg->message==WM_KEYDOWN || pmsg->message==WM_KEYUP) {
			TCHAR szClass[64];

			if (::GetClassName(hwnd,szClass,lengthof(szClass))>0) {
				bool fNeedCursor=false;

				if (::lstrcmpi(szClass,TEXT("EDIT"))==0
						|| ::StrCmpNI(szClass,TEXT("RICHEDIT"),8)==0) {
					if ((GetWindowStyle(hwnd)&ES_READONLY)==0)
						return true;
					fNeedCursor=true;
				} else if (::lstrcmpi(szClass,TEXT("COMBOBOX"))==0
						|| ::lstrcmpi(szClass,TEXT("LISTBOX"))==0
						|| ::StrCmpNI(szClass,TEXT("SysListView"),11)==0
						|| ::StrCmpNI(szClass,TEXT("SysTreeView"),11)==0) {
					fNeedCursor=true;
				}
				if (fNeedCursor) {
					switch (pmsg->wParam) {
					case VK_LEFT:
					case VK_RIGHT:
					case VK_UP:
					case VK_DOWN:
						return true;
					}
				}
			}
		}
	}
	return false;
}


static int ApplicationMain(HINSTANCE hInstance,LPCTSTR pszCmdLine,int nCmdShow)
{
	hInst=hInstance;

	// コマンドラインの解析
	if (pszCmdLine[0]!=_T('\0')) {
		Logger.AddLog(TEXT("コマンドラインオプション : %s"),pszCmdLine);

		CmdLineOptions.Parse(pszCmdLine);

		if (CmdLineOptions.m_TvRockDID>=0)
			CmdLineOptions.m_fSilent=true;
		if (CmdLineOptions.m_fSilent)
			AppMain.SetSilent(true);
		if (CmdLineOptions.m_fTray)
			CmdLineOptions.m_fMinimize=true;

		if (CmdLineOptions.m_fProgramGuideOnly) {
			CmdLineOptions.m_fShowProgramGuide=true;
			CmdLineOptions.m_fStandby=true;
		}
	}

//#ifdef TVH264_FOR_1SEG
	PanelFrame.SetFloating(false);
//#endif

	AppMain.Initialize();

	CAppMutex AppMutex(true);

	// 複数起動のチェック
	if (AppMutex.AlreadyExists()
			&& (GeneralOptions.GetKeepSingleTask() || CmdLineOptions.m_fSingleTask)) {
		Logger.AddLog(TEXT("複数起動が禁止されています。"));
		CTVTestWindowFinder Finder;
		HWND hwnd=Finder.FindCommandLineTarget();
		if (::IsWindow(hwnd)) {
			ATOM atom;

			if (pszCmdLine[0]!=_T('\0'))
				atom=::GlobalAddAtom(pszCmdLine);
			else
				atom=0;
			// ATOM だと256文字までしか渡せないので、WM_COPYDATA 辺りの方がいいかも
			/*
			DWORD_PTR Result;
			::SendMessageTimeout(hwnd,WM_APP_EXECUTE,(WPARAM)atom,0,
								 SMTO_NORMAL,10000,&Result);
			*/
			::PostMessage(hwnd,WM_APP_EXECUTE,(WPARAM)atom,0);
			return 0;
		}
		if (!CmdLineOptions.m_fSingleTask) {
			if (!AppMain.IsSilent()) {
				::MessageBox(NULL,
					APP_NAME TEXT(" は既に起動しています。\n")
					TEXT("ウィンドウが見当たらない場合はタスクマネージャに隠れていますので\n")
					TEXT("強制終了させてください。"),MAIN_TITLE_TEXT,
					MB_OK | MB_ICONEXCLAMATION);
			}
			return 0;
		}
	}

	// コモンコントロールの初期化
	{
		INITCOMMONCONTROLSEX iccex;

		iccex.dwSize=sizeof(INITCOMMONCONTROLSEX);
		iccex.dwICC=ICC_UPDOWN_CLASS | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_DATE_CLASSES | ICC_PROGRESS_CLASS;
		::InitCommonControlsEx(&iccex);
	}

	CBufferedPaint::Initialize();

	// BonDriverの検索
	{
		TCHAR szDirectory[MAX_PATH];
		CoreEngine.GetDriverDirectory(szDirectory,lengthof(szDirectory));
		DriverManager.Find(szDirectory);
	}
	DriverOptions.Initialize(&DriverManager);

	// CASライブラリの読み込み
	if (!CmdLineOptions.m_CasLibraryName.IsEmpty()) {
		CoreEngine.SetCasLibraryName(CmdLineOptions.m_CasLibraryName.Get());
	} else {
		TCHAR szDir[MAX_PATH];
		std::vector<String> List;
		AppMain.GetAppDirectory(szDir);
		if (CoreEngine.FindCasLibraries(szDir,&List) && !List.empty())
			CoreEngine.SetCasLibraryName(List[0].c_str());
	}
	if (!IsStringEmpty(CoreEngine.GetCasLibraryName())) {
		//StatusView.SetSingleText(TEXT("CASライブラリの読み込み中..."));
		if (CoreEngine.LoadCasLibrary()) {
			CCasProcessor::CasModuleInfo ModInfo;

			CoreEngine.m_DtvEngine.m_CasProcessor.GetCasModuleInfo(&ModInfo);
			Logger.AddLog(TEXT("CASライブラリ \"%s\" (%s %s) を読み込みました。"),
						  CoreEngine.GetCasLibraryName(),
						  ModInfo.Name,ModInfo.Version);
		} else {
			AppMain.OnError(&CoreEngine,TEXT("CASライブラリを読み込めません。"));
		}
	}

	TCHAR szDriverFileName[MAX_PATH];

	// 初期設定ダイアログの表示
	if (CmdLineOptions.m_fInitialSettings
			|| (AppMain.IsFirstExecute() && CmdLineOptions.m_DriverName.IsEmpty())) {
		CInitialSettings InitialSettings(&DriverManager);

		if (!InitialSettings.Show(NULL))
			return 0;
		InitialSettings.GetDriverFileName(szDriverFileName,lengthof(szDriverFileName));
		GeneralOptions.SetDefaultDriverName(szDriverFileName);
		GeneralOptions.SetMpeg2DecoderName(InitialSettings.GetMpeg2DecoderName());
		GeneralOptions.SetVideoRendererType(InitialSettings.GetVideoRenderer());
		GeneralOptions.SetCasDevice(InitialSettings.GetCasDevice());
		RecordOptions.SetSaveFolder(InitialSettings.GetRecordFolder());
	} else if (!CmdLineOptions.m_DriverName.IsEmpty()) {
		::lstrcpy(szDriverFileName,CmdLineOptions.m_DriverName.Get());
	} else if (CmdLineOptions.m_fNoDriver) {
		szDriverFileName[0]=_T('\0');
	} else {
		GeneralOptions.GetFirstDriverName(szDriverFileName);
	}

	GeneralOptions.SetTemporaryNoDescramble(CmdLineOptions.m_fNoDescramble);

	if (CmdLineOptions.m_Volume>=0)
		CoreEngine.SetVolume(min(CmdLineOptions.m_Volume,CCoreEngine::MAX_VOLUME));

	ColorSchemeOptions.SetApplyCallback(ColorSchemeApplyProc);
	ColorSchemeOptions.ApplyColorScheme();

	// たくさん初期化
	CMainWindow::Initialize();
	CViewWindow::Initialize(hInst);
	CVideoContainerWindow::Initialize(hInst);
	CStatusView::Initialize(hInst);
	CSideBar::Initialize(hInst);
	Layout::CLayoutBase::Initialize(hInst);
	CTitleBar::Initialize(hInst);
	CPanelFrame::Initialize(hInst);
	CPanelForm::Initialize(hInst);
	CInformationPanel::Initialize(hInst);
	CProgramListPanel::Initialize(hInst);
	CChannelPanel::Initialize(hInst);
	CControlPanel::Initialize(hInst);
	CCaptionPanel::Initialize(hInst);
	CProgramGuide::Initialize(hInst);
	CProgramGuideFrame::Initialize(hInst);
	CProgramGuideDisplay::Initialize(hInst);
	CCaptureWindow::Initialize(hInst);
	CPseudoOSD::Initialize(hInst);
	CNotificationBar::Initialize(hInst);
	CEventInfoPopup::Initialize(hInst);
	CDropDownMenu::Initialize(hInst);
	CHomeDisplay::Initialize(hInst);
	CChannelDisplay::Initialize(hInst);

	StreamInfo.SetEventHandler(&StreamInfoEventHandler);
	CaptureWindow.SetEventHandler(&CaptureWindowEventHandler);

	// ウィンドウ位置とサイズの設定
	if (CmdLineOptions.m_fMaximize)
		MainWindow.SetMaximize(true);
	{
		int Left,Top,Width,Height;
		MainWindow.GetPosition(&Left,&Top,&Width,&Height);
		if (CmdLineOptions.m_WindowLeft!=CCommandLineOptions::INVALID_WINDOW_POS)
			Left=CmdLineOptions.m_WindowLeft;
		if (CmdLineOptions.m_WindowTop!=CCommandLineOptions::INVALID_WINDOW_POS)
			Top=CmdLineOptions.m_WindowTop;
		if (CmdLineOptions.m_WindowWidth>0)
			Width=CmdLineOptions.m_WindowWidth;
		if (CmdLineOptions.m_WindowHeight>0)
			Height=CmdLineOptions.m_WindowHeight;
		MainWindow.SetPosition(Left,Top,Width,Height);
	}

	// ウィンドウの作成
	if (!MainWindow.Create(NULL,WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN)) {
		Logger.AddLog(TEXT("ウィンドウが作成できません。"));
		if (!AppMain.IsSilent())
			MessageBox(NULL,TEXT("ウィンドウが作成できません。"),NULL,MB_OK | MB_ICONSTOP);
		return 0;
	}
	if (nCmdShow==SW_SHOWMINIMIZED || nCmdShow==SW_SHOWMINNOACTIVE || nCmdShow==SW_MINIMIZE)
		CmdLineOptions.m_fMinimize=true;
	if (CmdLineOptions.m_fStandby && CmdLineOptions.m_fMinimize)
		CmdLineOptions.m_fMinimize=false;
	if (!CmdLineOptions.m_fStandby && !CmdLineOptions.m_fMinimize) {
		MainWindow.Show(nCmdShow);
		MainWindow.Update();
	}
	if (!MainWindow.GetTitleBarVisible() || MainWindow.GetCustomTitleBar()) {
		// WS_CAPTION を外す場合、この段階でスタイルを変えないとおかしくなる
		// (最初からこのスタイルにしてもキャプションが表示される
		//  ShowWindow の前に入れると、キャプションを表示させた時にアイコンが出ない)
		MainWindow.SetStyle(MainWindow.GetStyle()^WS_CAPTION,true);
		MainWindow.Update();
	}

	ResidentManager.SetResident(GeneralOptions.GetResident());
	ResidentManager.Initialize(MainWindow.GetHandle(),WM_APP_TRAYICON);
	ResidentManager.SetMinimizeToTray(CmdLineOptions.m_fTray || ViewOptions.GetMinimizeToTray());
	if (CmdLineOptions.m_fMinimize)
		MainWindow.InitMinimize();

	ViewOptions.Apply(COptions::UPDATE_ALL);

	CoreEngine.SetMinTimerResolution(PlaybackOptions.GetMinTimerResolution());
	CoreEngine.SetDescramble(!CmdLineOptions.m_fNoDescramble);
	CoreEngine.SetCasDevice(GeneralOptions.GetCasDevice(true));
	CoreEngine.SetNoEpg(CmdLineOptions.m_fNoEpg);
	CoreEngine.m_DtvEngine.SetDescrambleCurServiceOnly(GeneralOptions.GetDescrambleCurServiceOnly());
	CoreEngine.m_DtvEngine.m_CasProcessor.SetInstruction(GeneralOptions.GetDescrambleInstruction());
	CoreEngine.m_DtvEngine.m_CasProcessor.EnableContract(GeneralOptions.GetEnableEmmProcess());
	PlaybackOptions.Apply(COptions::UPDATE_ALL);
	CoreEngine.m_DtvEngine.m_LogoDownloader.SetLogoHandler(&LogoManager);
	CoreEngine.m_DtvEngine.SetTracer(&StatusView);
	CoreEngine.m_DtvEngine.m_BonSrcDecoder.SetTracer(&Logger);
	CoreEngine.BuildDtvEngine(&DtvEngineHandler);
	RecordOptions.Apply(COptions::UPDATE_ALL);

	// BonDriver の読み込み
	CoreEngine.SetDriverFileName(szDriverFileName);
	if (!CmdLineOptions.m_fNoDriver && !CmdLineOptions.m_fStandby) {
		if (CoreEngine.IsDriverSpecified()) {
			StatusView.SetSingleText(TEXT("BonDriverの読み込み中..."));
			if (AppMain.OpenAndInitializeTuner()) {
				Logger.AddLog(TEXT("%s を読み込みました。"),CoreEngine.GetDriverFileName());
				AppMain.GetUICore()->OnTunerChanged();
			} else {
				AppMain.OnError(&CoreEngine,TEXT("BonDriverの初期化ができません。"));
			}
		} else {
			Logger.AddLog(TEXT("BonDriverが設定されていません。"));
		}
	}

	if (CoreEngine.IsTunerOpen()
			&& !DriverOptions.IsNoDescramble(CoreEngine.GetDriverFileName())) {
		AppMain.OpenCasCard(!AppMain.IsSilent()?CAppMain::OPEN_CAS_CARD_RETRY:0);
	}

	// プラグインの読み込み
	if (!CmdLineOptions.m_fNoPlugin) {
		TCHAR szPluginDir[MAX_PATH];
		std::vector<LPCTSTR> ExcludePlugins;

		CPlugin::SetMessageWindow(MainWindow.GetHandle(),WM_APP_PLUGINMESSAGE);
		StatusView.SetSingleText(TEXT("プラグインを読み込んでいます..."));
		AppMain.GetAppDirectory(szPluginDir);
		if (!CmdLineOptions.m_PluginsDirectory.IsEmpty()) {
			LPCTSTR pszDir=CmdLineOptions.m_PluginsDirectory.Get();
			if (::PathIsRelative(pszDir)) {
				TCHAR szTemp[MAX_PATH];
				::PathCombine(szTemp,szPluginDir,pszDir);
				::PathCanonicalize(szPluginDir,szTemp);
			} else {
				::lstrcpy(szPluginDir,pszDir);
			}
		} else {
			::PathAppend(szPluginDir,
#ifndef TVH264
						 TEXT("Plugins")
#else
						 TEXT("H264Plugins")
#endif
						 );
		}
		Logger.AddLog(TEXT("プラグインを \"%s\" から読み込みます..."),szPluginDir);
		if (CmdLineOptions.m_NoLoadPlugins.size()>0) {
			for (size_t i=0;i<CmdLineOptions.m_NoLoadPlugins.size();i++)
				ExcludePlugins.push_back(CmdLineOptions.m_NoLoadPlugins[i].Get());
		}
		PluginManager.LoadPlugins(szPluginDir,&ExcludePlugins);
	}

	CommandList.Initialize(&DriverManager,&PluginManager);
	CommandList.AddCommandCustomizer(&ZoomOptions);
	CommandList.AddCommandCustomizer(&PanAndScanOptions);

	if (!CmdLineOptions.m_fNoPlugin)
		PluginOptions.RestorePluginOptions();

	// 再生の初期化
	CoreEngine.m_DtvEngine.m_MediaViewer.SetUseAudioRendererClock(PlaybackOptions.GetUseAudioRendererClock());
	CoreEngine.SetDownMixSurround(PlaybackOptions.GetDownMixSurround());
	CoreEngine.SetSpdifOptions(PlaybackOptions.GetSpdifOptions());
	if (!CmdLineOptions.m_fStandby && !CmdLineOptions.m_fNoDirectShow)
		AppMain.GetUICore()->InitializeViewer();

	if (PlaybackOptions.IsMuteOnStartUp() || CmdLineOptions.m_fMute)
		AppMain.GetUICore()->SetMute(true);
	if ((!PlaybackOptions.GetRestorePlayStatus() || fEnablePlay)
			&& CoreEngine.m_DtvEngine.m_MediaViewer.IsOpen()) {
		if (!CmdLineOptions.m_fNoView && !CmdLineOptions.m_fMinimize)
			AppMain.GetUICore()->EnableViewer(true);
	}

	if (CoreEngine.IsNetworkDriver()) {
		if (fIncrementUDPPort) {
			CPortQuery PortQuery;
			WORD UDPPort=CmdLineOptions.m_UDPPort>0?(WORD)CmdLineOptions.m_UDPPort:
											CoreEngine.IsUDPDriver()?1234:2230;
#ifdef NETWORK_REMOCON_SUPPORT
			WORD RemoconPort=NetworkRemoconOptions.GetPort();
#endif

			StatusView.SetSingleText(TEXT("空きポートを検索しています..."));
			PortQuery.Query(MainWindow.GetHandle(),&UDPPort,CoreEngine.IsUDPDriver()?1243:2239
#ifdef NETWORK_REMOCON_SUPPORT
							,&RemoconPort
#endif
							);
			CmdLineOptions.m_UDPPort=UDPPort;
#ifdef NETWORK_REMOCON_SUPPORT
			NetworkRemoconOptions.SetTempPort(RemoconPort);
#endif
		}
#ifdef NETWORK_REMOCON_SUPPORT
		if (CmdLineOptions.m_fUseNetworkRemocon)
			NetworkRemoconOptions.SetTempEnable(true);
#endif
	}

	StatusView.SetSingleText(TEXT("チャンネル設定を読み込んでいます..."));
	AppMain.InitializeChannel();

	StatusView.SetSingleText(TEXT("ロゴを読み込んでいます..."));
	EpgOptions.LoadLogoFile();

	{
		TCHAR szDRCSMapName[MAX_PATH];

		AppMain.GetAppDirectory(szDRCSMapName);
		::PathAppend(szDRCSMapName,TEXT("DRCSMap.ini"));
		if (::PathFileExists(szDRCSMapName)) {
			StatusView.SetSingleText(TEXT("DRCSマップを読み込んでいます..."));
			CaptionPanel.LoadDRCSMap(szDRCSMapName);
		}
	}

	CoreEngine.m_DtvEngine.SetTracer(NULL);
	if (!MainWindow.GetStatusBarVisible())
		StatusView.SetVisible(false);
	StatusView.SetSingleText(NULL);

	MainWindow.CreatePanel();
	if (fShowPanelWindow
			&& (!PanelFrame.GetFloating()
				|| (!CmdLineOptions.m_fStandby && !CmdLineOptions.m_fMinimize))) {
		PanelFrame.SetPanelVisible(true,true);
		PanelFrame.Update();
	}

	if (!CmdLineOptions.m_fNoEpg) {
		EpgOptions.AsyncLoadEpgFile(&EpgProgramList,&EpgLoadEventHandler);
		EpgOptions.AsyncLoadEDCBData(&EpgLoadEventHandler);
	}

	g_ProgramGuide.SetEpgProgramList(&EpgProgramList);
	g_ProgramGuide.SetEventHandler(&ProgramGuideEventHandler);
	g_ProgramGuide.SetProgramCustomizer(&ProgramGuideProgramCustomizer);
	g_ProgramGuide.SetChannelProviderManager(&ProgramGuideChannelProviderManager);

	ApplyEventInfoFont();

	if (CoreEngine.m_DtvEngine.IsSrcFilterOpen()) {
		if (CoreEngine.IsBuildComplete()) {
			if (CmdLineOptions.m_fFullscreen)
				AppMain.GetUICore()->SetFullscreen(true);
		}

		if (CoreEngine.IsNetworkDriver()) {
			const int FirstPort=CoreEngine.IsUDPDriver()?1234:2230;
			int Port=FirstPort;
			if ((int)CmdLineOptions.m_UDPPort>=FirstPort && (int)CmdLineOptions.m_UDPPort<FirstPort+10)
				Port=CmdLineOptions.m_UDPPort;
			else if (RestoreChannelInfo.Channel>=0 && RestoreChannelInfo.Channel<10)
				Port=FirstPort+RestoreChannelInfo.Channel;
			AppMain.SetChannel(0,Port-FirstPort,CmdLineOptions.m_ServiceID);
			if (CmdLineOptions.m_ControllerChannel>0)
				SetCommandLineChannel(&CmdLineOptions);
		} else if (AppMain.IsFirstExecute()
				&& ChannelManager.GetFileAllChannelList()->NumChannels()==0) {
			if (MainWindow.ShowMessage(
					TEXT("最初にチャンネルスキャンを行うことをおすすめします。\r\n")
					TEXT("今すぐチャンネルスキャンを行いますか?"),
					TEXT("チャンネルスキャンの確認"),
					MB_YESNO | MB_ICONQUESTION)==IDYES) {
				ShowOptionDialog(MainWindow.GetHandle(),
								 COptionDialog::PAGE_CHANNELSCAN);
			}
		} else if (CmdLineOptions.IsChannelSpecified()) {
			SetCommandLineChannel(&CmdLineOptions);
		} else if (RestoreChannelInfo.Space>=0
				&& RestoreChannelInfo.Channel>=0) {
			AppMain.RestoreChannel();
		} else {
			// 初期チャンネルに設定する
			const CChannelList *pList=ChannelManager.GetCurrentChannelList();
			int i=pList->Find(
				CoreEngine.m_DtvEngine.m_BonSrcDecoder.GetCurSpace(),
				CoreEngine.m_DtvEngine.m_BonSrcDecoder.GetCurChannel());

			if (i>=0)
				MainWindow.SendCommand(CM_CHANNEL_FIRST+i);
		}
	}

	if (CmdLineOptions.m_fStandby)
		MainWindow.InitStandby();

	if (CmdLineOptions.m_fRecord)
		MainWindow.CommandLineRecord(CmdLineOptions.m_RecordFileName.Get(),
									 &CmdLineOptions.m_RecordStartTime,
									 CmdLineOptions.m_RecordDelay,
									 CmdLineOptions.m_RecordDuration);
	if (CmdLineOptions.m_fExitOnRecordEnd)
		MainWindow.SendCommand(CM_EXITONRECORDINGSTOP);

	if (fShowPanelWindow && PanelForm.GetCurPageID()==PANEL_ID_CHANNEL)
		ChannelPanel.SetChannelList(ChannelManager.GetCurrentChannelList(),false);

	AppMain.InitializeCommandSettings();
	FavoritesManager.Load(AppMain.GetFavoritesFileName());

	// EPG番組表の表示
	if (CmdLineOptions.m_fShowProgramGuide) {
		CMainWindow::ProgramGuideSpaceInfo SpaceInfo;

		if (!CmdLineOptions.m_ProgramGuideTuner.IsEmpty())
			SpaceInfo.pszTuner=CmdLineOptions.m_ProgramGuideTuner.Get();
		else
			SpaceInfo.pszTuner=NULL;
		SpaceInfo.Space=CmdLineOptions.m_ProgramGuideSpace;

		MainWindow.ShowProgramGuide(true,
			AppMain.GetUICore()->GetFullscreen()?0:CMainWindow::PROGRAMGUIDE_SHOW_POPUP,
			&SpaceInfo);
	}

	if (CmdLineOptions.m_fHomeDisplay) {
		MainWindow.PostCommand(CM_HOMEDISPLAY);
	} else if (CmdLineOptions.m_fChannelDisplay) {
		MainWindow.PostCommand(CM_CHANNELDISPLAY);
	}

	SetFocus(MainWindow.GetHandle());

	{
		HWND hwndForeground=GetForegroundWindow();
		if (hwndForeground!=NULL) {
			DWORD ProcessID=0;
			GetWindowThreadProcessId(hwndForeground,&ProcessID);
			if (ProcessID==GetCurrentProcessId())
				BroadcastControllerFocusMessage(NULL,false,true);
		}
	}

	PluginManager.SendStartupDoneEvent();

	// メッセージループ
	MSG msg;

	AppMain.GetUICore()->RegisterModelessDialog(&StreamInfo);

	while (GetMessage(&msg,NULL,0,0)>0) {
		if (HtmlHelpClass.PreTranslateMessage(&msg)
				|| AppMain.GetUICore()->ProcessDialogMessage(&msg))
			continue;
		if ((IsNoAcceleratorMessage(&msg)
				|| !Accelerator.TranslateMessage(MainWindow.GetHandle(),&msg))
				&& !ControllerManager.TranslateMessage(MainWindow.GetHandle(),&msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}


// エントリポイント
int APIENTRY _tWinMain(HINSTANCE hInstance,HINSTANCE /*hPrevInstance*/,
					   LPTSTR pszCmdLine,int nCmdShow)
{
	// 一応例の脆弱性対策をしておく
#ifdef WINDOWS2000_SUPPORT
	HMODULE hKernel=GetModuleHandle(TEXT("kernel32.dll"));
	if (hKernel!=NULL) {
		typedef BOOL (WINAPI *SetDllDirectoryFunc)(LPCWSTR);
		SetDllDirectoryFunc pSetDllDirectory=
			reinterpret_cast<SetDllDirectoryFunc>(GetProcAddress(hKernel,"SetDllDirectoryW"));
		if (pSetDllDirectory!=NULL)
			pSetDllDirectory(L"");
	}
#else
	SetDllDirectory(TEXT(""));
#endif

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF/* | _CRTDBG_CHECK_ALWAYS_DF*/);
#else
	DebugHelper.Initialize();
	DebugHelper.SetExceptionFilterMode(CDebugHelper::EXCEPTION_FILTER_DIALOG);
#endif

	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

	Logger.AddLog(TEXT("******** ") ABOUT_VERSION_TEXT
#ifdef VERSION_PLATFORM
				  TEXT(" (") VERSION_PLATFORM TEXT(")")
#endif
				  TEXT(" 起動 ********"));

	CoInitializeEx(NULL,COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE | COINIT_SPEED_OVER_MEMORY);

	const int Result=ApplicationMain(hInstance,pszCmdLine,nCmdShow);

	CoUninitialize();

	Logger.AddLog(TEXT("******** 終了 ********"));
	if (CmdLineOptions.m_fSaveLog && !Logger.GetOutputToFile()) {
		TCHAR szFileName[MAX_PATH];

		Logger.GetDefaultLogFileName(szFileName);
		Logger.SaveToFile(szFileName,true);
	}

	return Result;
}
