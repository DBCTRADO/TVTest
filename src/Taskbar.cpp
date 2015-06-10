#include "stdafx.h"
#include <propvarutil.h>
#include <propkey.h>
#include "TVTest.h"
#include "Taskbar.h"
#include "AppMain.h"
#include "resource.h"
#include "Common/DebugDef.h"




const CTaskbarManager::CommandIconInfo CTaskbarManager::m_CommandIconList[] =
{
	{CM_FULLSCREEN,		IDI_TASK_FULLSCREEN},
	{CM_DISABLEVIEWER,	IDI_TASK_DISABLEVIEWER},
	{CM_PROGRAMGUIDE,	IDI_TASK_PROGRAMGUIDE},
};


CTaskbarManager::CTaskbarManager()
	: m_hwnd(NULL)
	, m_TaskbarButtonCreatedMessage(0)
	, m_pTaskbarList(NULL)
	, m_fAppIDInvalid(false)
	, m_fJumpListInitialized(false)
	, m_fSaveRecentChannelList(false)
{
}


CTaskbarManager::~CTaskbarManager()
{
	Finalize();
}


void CTaskbarManager::SetAppID(LPCTSTR pszID)
{
	TVTest::StringUtility::Assign(m_AppID,pszID);
}


bool CTaskbarManager::Initialize(HWND hwnd)
{
	if (m_TaskbarButtonCreatedMessage==0) {
		if (Util::OS::IsWindows7OrLater()) {
			m_TaskbarButtonCreatedMessage=::RegisterWindowMessage(TEXT("TaskbarButtonCreated"));

#ifdef WIN_XP_SUPPORT
			auto pChangeWindowMessageFilter=
				GET_MODULE_FUNCTION(TEXT("user32.dll"),ChangeWindowMessageFilter);
			if (pChangeWindowMessageFilter!=NULL) {
				pChangeWindowMessageFilter(m_TaskbarButtonCreatedMessage,MSGFLT_ADD);
			}
#else
			::ChangeWindowMessageFilter(m_TaskbarButtonCreatedMessage,MSGFLT_ADD);
#endif

			m_hwnd=hwnd;

			CAppMain &App=GetAppClass();
			HRESULT hr=S_OK;

			if (!m_AppID.empty()) {
				auto pSetCurrentProcessExplicitAppUserModelID=
					GET_MODULE_FUNCTION(TEXT("shell32.dll"),SetCurrentProcessExplicitAppUserModelID);
				if (pSetCurrentProcessExplicitAppUserModelID!=NULL) {
					hr=pSetCurrentProcessExplicitAppUserModelID(m_AppID.c_str());
					if (FAILED(hr)) {
						m_fAppIDInvalid=true;
						App.AddLog(
							CLogItem::TYPE_ERROR,
							TEXT("AppID \"%s\" を設定できません。(%08x)"),
							m_AppID.c_str(),hr);
					}
				}
			}
			if (SUCCEEDED(hr)) {
				if (App.TaskbarOptions.IsJumpListEnabled()) {
					hr=InitializeJumpList();
					if (SUCCEEDED(hr))
						m_fJumpListInitialized=true;
				} else {
					ClearJumpList();
				}
			}
		}
	}
	return true;
}


void CTaskbarManager::Finalize()
{
	if (m_pTaskbarList!=NULL) {
		m_pTaskbarList->Release();
		m_pTaskbarList=NULL;
	}

	if (m_SharedProperties.IsOpened()) {
		if (m_SharedProperties.GetRecentChannelList(&m_RecentChannelList))
			m_fSaveRecentChannelList=true;
		m_SharedProperties.Close();
	}

	m_hwnd=NULL;
}


bool CTaskbarManager::HandleMessage(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	if (uMsg!=0 && uMsg==m_TaskbarButtonCreatedMessage) {
		if (m_pTaskbarList==NULL) {
			if (FAILED(::CoCreateInstance(CLSID_TaskbarList,NULL,
										  CLSCTX_INPROC_SERVER,
										  IID_ITaskbarList3,
										  reinterpret_cast<void**>(&m_pTaskbarList))))
				return true;
		}

		static const int ButtonList[] = {
			CM_FULLSCREEN,
			CM_DISABLEVIEWER,
			CM_PROGRAMGUIDE,
		};
		THUMBBUTTON tb[lengthof(ButtonList)];
		const CAppMain &App=GetAppClass();
		HINSTANCE hinst=App.GetResourceInstance();

		for (int i=0;i<lengthof(tb);i++) {
			const int Command=ButtonList[i];

			tb[i].dwMask=(THUMBBUTTONMASK)(THB_ICON | THB_TOOLTIP | THB_FLAGS);
			tb[i].iId=Command;
			tb[i].hIcon=::LoadIcon(hinst,MAKEINTRESOURCE(GetCommandIcon(Command)));
			App.CommandList.GetCommandNameByID(Command,tb[i].szTip,lengthof(tb[0].szTip));
			tb[i].dwFlags=(THUMBBUTTONFLAGS)(THBF_ENABLED | THBF_DISMISSONCLICK);
		}

		m_pTaskbarList->ThumbBarAddButtons(m_hwnd,lengthof(tb),tb);

		return true;
	}

	return false;
}


bool CTaskbarManager::SetRecordingStatus(bool fRecording)
{
	if (m_pTaskbarList!=NULL) {
		if (fRecording) {
			HICON hico=static_cast<HICON>(::LoadImage(GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDI_TASKBAR_RECORDING),IMAGE_ICON,16,16,LR_DEFAULTCOLOR));

			if (hico==NULL)
				return false;
			m_pTaskbarList->SetOverlayIcon(m_hwnd,hico,TEXT("録画中"));
			::DestroyIcon(hico);
		} else {
			m_pTaskbarList->SetOverlayIcon(m_hwnd,NULL,NULL);
		}
		return true;
	}
	return false;
}


bool CTaskbarManager::SetProgress(int Pos,int Max)
{
	if (m_pTaskbarList!=NULL) {
		if (Pos>=Max) {
			m_pTaskbarList->SetProgressState(m_hwnd,TBPF_NOPROGRESS);
		} else {
			if (Pos==0)
				m_pTaskbarList->SetProgressState(m_hwnd,TBPF_NORMAL);
			m_pTaskbarList->SetProgressValue(m_hwnd,Pos,Max);
		}
		return true;
	}
	return false;
}


bool CTaskbarManager::EndProgress()
{
	if (m_pTaskbarList!=NULL)
		m_pTaskbarList->SetProgressState(m_hwnd,TBPF_NOPROGRESS);
	return true;
}


bool CTaskbarManager::ReinitializeJumpList()
{
	if (m_fAppIDInvalid)
		return false;
	if (GetAppClass().TaskbarOptions.IsJumpListEnabled()) {
		HRESULT hr=InitializeJumpList();
		if (FAILED(hr))
			return false;
		m_fJumpListInitialized=true;
	} else {
		ClearJumpList();
	}

	return true;
}


bool CTaskbarManager::UpdateJumpList()
{
	if (!m_fJumpListInitialized)
		return false;
	return SUCCEEDED(InitializeJumpList());
}


bool CTaskbarManager::ClearJumpList()
{
	HRESULT hr;
	ICustomDestinationList *pcdl;

	hr=::CoCreateInstance(CLSID_DestinationList,NULL,
						  CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&pcdl));
	if (FAILED(hr))
		return false;

	hr=pcdl->DeleteList(m_AppID.empty()?nullptr:m_AppID.c_str());
	pcdl->Release();

	return SUCCEEDED(hr);
}


bool CTaskbarManager::AddRecentChannel(const CTunerChannelInfo &Info)
{
	return m_SharedProperties.AddRecentChannel(Info);
}


bool CTaskbarManager::ClearRecentChannelList()
{
	return m_SharedProperties.ClearRecentChannelList();
}


bool CTaskbarManager::LoadSettings(CSettings &Settings)
{
	if (Settings.IsSectionExists(TEXT("TaskbarRecentChannels"))) {
		if (Settings.SetSection(TEXT("TaskbarRecentChannels")))
			m_RecentChannelList.ReadSettings(Settings);
	} else {
		if (Settings.SetSection(TEXT("RecentChannel")))
			m_RecentChannelList.ReadSettings(Settings);
	}

	return true;
}


bool CTaskbarManager::SaveSettings(CSettings &Settings)
{
	if (m_fSaveRecentChannelList) {
		if (Settings.SetSection(TEXT("TaskbarRecentChannels")))
			m_RecentChannelList.WriteSettings(Settings);
	}

	return true;
}


HRESULT CTaskbarManager::InitializeJumpList()
{
	if (!m_SharedProperties.IsOpened()) {
		TVTest::String PropName;

		if (m_AppID.empty()) {
			TCHAR szAppPath[MAX_PATH];
			DWORD Length=::GetModuleFileName(NULL,szAppPath,lengthof(szAppPath));
			::CharLowerBuff(szAppPath,Length);
			BYTE Hash[16];
			CMD5Calculator::CalcMD5(szAppPath,Length*sizeof(TCHAR),Hash);
			TVTest::StringUtility::Format(
				PropName,
				APP_NAME TEXT("_%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x_TaskbarProp"),
				Hash[ 0],Hash[ 1],Hash[ 2],Hash[ 3],Hash[ 4],Hash[ 5],Hash[ 6],Hash[ 7],
				Hash[ 8],Hash[ 9],Hash[10],Hash[11],Hash[12],Hash[13],Hash[14],Hash[15]);
		} else {
			PropName=m_AppID;
			PropName+=TEXT("_TaskbarProp");
		}

		m_SharedProperties.Open(PropName.c_str(),&m_RecentChannelList);
	}

	HRESULT hr;
	ICustomDestinationList *pcdl;

	hr=::CoCreateInstance(CLSID_DestinationList,NULL,
						  CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&pcdl));
	if (FAILED(hr))
		return hr;

	if (!m_AppID.empty())
		pcdl->SetAppID(m_AppID.c_str());

	UINT MinSlots;
	IObjectArray *pRemovedList;

	hr=pcdl->BeginList(&MinSlots,IID_PPV_ARGS(&pRemovedList));
	if (SUCCEEDED(hr)) {
		const CTaskbarOptions &TaskbarOptions=GetAppClass().TaskbarOptions;

		if (TaskbarOptions.GetShowTasks())
			hr=AddTaskList(pcdl);
		if (SUCCEEDED(hr)) {
			if (TaskbarOptions.GetShowRecentChannels())
				hr=AddRecentChannelsCategory(pcdl);
		}

		pRemovedList->Release();
	}

	if (SUCCEEDED(hr))
		hr=pcdl->CommitList();

	pcdl->Release();

	return hr;
}


HRESULT CTaskbarManager::AddTaskList(ICustomDestinationList *pcdl)
{
	CAppMain &App=GetAppClass();
	const CTaskbarOptions::TaskList &TaskList=App.TaskbarOptions.GetTaskList();

	if (TaskList.empty())
		return S_FALSE;

	HRESULT hr;
	IObjectCollection *pCollection;

	hr=::CoCreateInstance(CLSID_EnumerableObjectCollection,NULL,
						  CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&pCollection));
	if (SUCCEEDED(hr)) {
		WCHAR szIconPath[MAX_PATH];

		::GetModuleFileNameW(NULL,szIconPath,lengthof(szIconPath));

		for (auto it=TaskList.begin();it!=TaskList.end();++it) {
			const int Command=*it;
			IShellLink *pShellLink;

			if (Command==0) {
				hr=CreateSeparatorShellLink(&pShellLink);
			} else {
				WCHAR szArgs[CCommandList::MAX_COMMAND_TEXT+32];
				WCHAR szTitle[CCommandList::MAX_COMMAND_NAME];

				if (Command==CM_PROGRAMGUIDE) {
					StdUtil::strncpy(szArgs,lengthof(szArgs),L"/jumplist /s /epgonly");
				} else {
					StdUtil::snprintf(
						szArgs,lengthof(szArgs),L"/jumplist /s /command %s",
						App.CommandList.GetCommandTextByID(Command));
				}
				App.CommandList.GetCommandNameByID(Command,szTitle,lengthof(szTitle));
				int Icon=GetCommandIcon(Command);
				hr=CreateAppShellLink(szArgs,szTitle,NULL,Icon!=0?szIconPath:NULL,-Icon,&pShellLink);
			}

			if (SUCCEEDED(hr)) {
				pCollection->AddObject(pShellLink);
				pShellLink->Release();
			}
		}

		IObjectArray *pArray;
		hr=pCollection->QueryInterface(IID_PPV_ARGS(&pArray));
		if (SUCCEEDED(hr)) {
			hr=pcdl->AddUserTasks(pArray);
			pArray->Release();
		}

		pCollection->Release();
	}

	return hr;
}


HRESULT CTaskbarManager::CreateAppShellLink(
	PCWSTR pszArgs,PCWSTR pszTitle,PCWSTR pszDescription,
	PCWSTR pszIconPath,int IconIndex,
	IShellLink **ppShellLink)
{
	WCHAR szAppPath[MAX_PATH];
	DWORD Length=::GetModuleFileNameW(NULL,szAppPath,lengthof(szAppPath));
	if (Length==0 || Length>=lengthof(szAppPath)-1)
		return E_FAIL;

	HRESULT hr;
	IShellLink *pShellLink;

	hr=::CoCreateInstance(CLSID_ShellLink,NULL,CLSCTX_INPROC_SERVER,
						  IID_PPV_ARGS(&pShellLink));
	if (SUCCEEDED(hr)) {
		hr=pShellLink->SetPath(szAppPath);
		if (SUCCEEDED(hr)) {
			hr=pShellLink->SetArguments(pszArgs);
			if (SUCCEEDED(hr)) {
				if (!IsStringEmpty(pszDescription))
					pShellLink->SetDescription(pszDescription);
				if (!IsStringEmpty(pszIconPath))
					pShellLink->SetIconLocation(pszIconPath,IconIndex);

				IPropertyStore *pPropStore;

				hr=pShellLink->QueryInterface(IID_PPV_ARGS(&pPropStore));
				if (SUCCEEDED(hr)) {
					PROPVARIANT PropVar;

					hr=::InitPropVariantFromString(pszTitle,&PropVar);
					if (SUCCEEDED(hr)) {
						hr=pPropStore->SetValue(PKEY_Title,PropVar);
						if (SUCCEEDED(hr)) {
							hr=pPropStore->Commit();
							if (SUCCEEDED(hr)) {
								pShellLink->AddRef();
								*ppShellLink=pShellLink;
							}
						}
						::PropVariantClear(&PropVar);
					}
					pPropStore->Release();
				}
			}
		}
		pShellLink->Release();
	}

	return hr;
}


HRESULT CTaskbarManager::CreateSeparatorShellLink(IShellLink **ppShellLink)
{
	HRESULT hr;
	IShellLink *pShellLink;

	hr=::CoCreateInstance(CLSID_ShellLink,NULL,
						  CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&pShellLink));
	if (SUCCEEDED(hr)) {
		IPropertyStore *pPropStore;

		hr=pShellLink->QueryInterface(IID_PPV_ARGS(&pPropStore));
		if (SUCCEEDED(hr)) {
			PROPVARIANT PropVar;

			hr=::InitPropVariantFromBoolean(TRUE,&PropVar);
			if (SUCCEEDED(hr)) {
				hr=pPropStore->SetValue(PKEY_AppUserModel_IsDestListSeparator,PropVar);
				if (SUCCEEDED(hr)) {
					hr=pPropStore->Commit();
					if (SUCCEEDED(hr)) {
						pShellLink->AddRef();
						*ppShellLink=pShellLink;
					}
				}
				::PropVariantClear(&PropVar);
			}
			pPropStore->Release();
		}
		pShellLink->Release();
	}

	return hr;
}


HRESULT CTaskbarManager::AddJumpListCategory(
	ICustomDestinationList *pcdl,
	PCWSTR pszTitle,const JumpListItemList &ItemList)
{
	HRESULT hr;
	IObjectCollection *pCollection;

	hr=::CoCreateInstance(CLSID_EnumerableObjectCollection,NULL,
						  CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&pCollection));
	if (SUCCEEDED(hr)) {
		CAppMain &App=GetAppClass();

		for (auto it=ItemList.begin();it!=ItemList.end();++it) {
			IShellLink *pShellLink;

			hr=CreateAppShellLink(
				it->Args.c_str(),it->Title.c_str(),it->Description.c_str(),
				it->IconPath.c_str(),it->IconIndex,
				&pShellLink);
			if (SUCCEEDED(hr)) {
				pCollection->AddObject(pShellLink);
				pShellLink->Release();
			}
		}

		IObjectArray *pArray;

		hr=pCollection->QueryInterface(IID_PPV_ARGS(&pArray));
		if (SUCCEEDED(hr)) {
			hr=pcdl->AppendCategory(pszTitle,pArray);
			pArray->Release();
		}

		pCollection->Release();
	}

	return hr;
}


HRESULT CTaskbarManager::AddRecentChannelsCategory(ICustomDestinationList *pcdl)
{
	CAppMain &App=GetAppClass();
	const CRecentChannelList *pRecentChannels;
	CRecentChannelList SharedRecentChannels;

	if (m_SharedProperties.GetRecentChannelList(&SharedRecentChannels))
		pRecentChannels=&SharedRecentChannels;
	else
		pRecentChannels=&App.RecentChannelList;

	int NumChannels=pRecentChannels->NumChannels();

	if (NumChannels==0)
		return S_FALSE;

	if (NumChannels>App.TaskbarOptions.GetMaxRecentChannels())
		NumChannels=App.TaskbarOptions.GetMaxRecentChannels();

	bool fShowIcon=App.TaskbarOptions.GetShowChannelIcon();
	TCHAR szIconDir[MAX_PATH];
	if (fShowIcon) {
		if (GetAbsolutePath(App.TaskbarOptions.GetIconDirectory().c_str(),
							szIconDir,lengthof(szIconDir)-13)) {
			if (!::PathIsDirectory(szIconDir)) {
				int Result=::SHCreateDirectoryEx(NULL,szIconDir,NULL);
				if (Result!=ERROR_SUCCESS && Result!=ERROR_ALREADY_EXISTS) {
					App.AddLog(
						CLogItem::TYPE_ERROR,
						TEXT("ジャンプリストアイコンフォルダ \"%s\" が作成できません。"),
						szIconDir);
					fShowIcon=false;
				} else if (Result==ERROR_SUCCESS) {
					App.AddLog(
						CLogItem::TYPE_INFORMATION,
						TEXT("ジャンプリストアイコンフォルダ \"%s\" を作成しました。"),
						szIconDir);
				}
			}
		} else {
			fShowIcon=false;
		}
	}

	JumpListItemList List;

	for (int i=0;i<NumChannels;i++) {
		const CTunerChannelInfo *pChannel=pRecentChannels->GetChannelInfo(i);
		const WORD NetworkID=pChannel->GetNetworkID();
		const WORD ServiceID=pChannel->GetServiceID();
		LPCWSTR pszTunerName=pChannel->GetTunerName();
		JumpListItem Item;
		TVTest::String Driver;
		WCHAR szTuner[MAX_PATH];

		Item.Title=pChannel->GetName();
		if (::StrChrW(pszTunerName,L' ')!=NULL) {
			Driver=L'"';
			Driver+=pszTunerName;
			Driver+=L'"';
		}
		TVTest::StringUtility::Format(
			Item.Args,L"/jumplist /d %s /chspace %d /chi %d /nid %d /sid %d",
			!Driver.empty()?Driver.c_str():pszTunerName,
			pChannel->GetSpace(),
			pChannel->GetChannelIndex(),
			NetworkID,ServiceID);
		::lstrcpynW(szTuner,pszTunerName,lengthof(szTuner));
		::PathRemoveExtensionW(szTuner);
		TVTest::StringUtility::Format(
			Item.Description,L"%s (%s)",
			pChannel->GetName(),
			::StrCmpNIW(szTuner,L"BonDriver_",10)==0 ? szTuner+10 : szTuner);

		if (fShowIcon && NetworkID!=0 && ServiceID!=0) {
			TCHAR szIconPath[MAX_PATH];
			TCHAR szFileName[16];

			StdUtil::snprintf(
				szFileName,lengthof(szFileName),
				TEXT("%04x%04x.ico"),NetworkID,ServiceID);
			::PathCombine(szIconPath,szIconDir,szFileName);

			bool fUseIcon=true;
			const DWORD MapKey=ChannelIconMapKey(NetworkID,ServiceID);
			const BYTE LogoType=CLogoManager::LOGOTYPE_48x24;
			CLogoManager::LogoInfo LogoInfo;
			if (App.LogoManager.GetLogoInfo(NetworkID,ServiceID,LogoType,&LogoInfo)) {
				bool fSave;
				auto it=m_ChannelIconMap.find(MapKey);
				if (it!=m_ChannelIconMap.end())
					fSave=CompareFileTime(&it->second.UpdatedTime,&LogoInfo.UpdatedTime)<0;
				else
					fSave=true;
				if (fSave) {
					WIN32_FIND_DATA FindData;
					HANDLE hFind=::FindFirstFile(szIconPath,&FindData);
					const bool fFound=hFind!=INVALID_HANDLE_VALUE;
					if (fFound)
						::FindClose(hFind);
					if (!fFound
							|| CompareFileTime(&FindData.ftLastWriteTime,&LogoInfo.UpdatedTime)<0) {
						if (App.LogoManager.SaveLogoIcon(NetworkID,ServiceID,LogoType,
														 ::GetSystemMetrics(SM_CXSMICON),
														 ::GetSystemMetrics(SM_CYSMICON),
														 szIconPath)) {
							m_ChannelIconMap[MapKey]=ChannelIconInfo(LogoInfo.UpdatedTime);
						} else {
							fUseIcon=false;
						}
					} else if (fFound) {
						m_ChannelIconMap[MapKey]=ChannelIconInfo(FindData.ftLastWriteTime);
					}
				}
			} else {
				auto it=m_ChannelIconMap.find(MapKey);
				if (it==m_ChannelIconMap.end()) {
					WIN32_FIND_DATA FindData;
					HANDLE hFind=::FindFirstFile(szIconPath,&FindData);
					if (hFind!=INVALID_HANDLE_VALUE) {
						::FindClose(hFind);
						m_ChannelIconMap[MapKey]=ChannelIconInfo(FindData.ftLastWriteTime);
					} else {
						fUseIcon=false;
					}
				}
			}

			if (fUseIcon) {
				Item.IconPath=szIconPath;
				Item.IconIndex=0;
			}
		}

		List.push_back(Item);
	}

	WCHAR szTitle[64];

	::LoadStringW(App.GetResourceInstance(),IDS_MENU_CHANNELHISTORY,
				  szTitle,lengthof(szTitle));

	return AddJumpListCategory(pcdl,szTitle,List);
}


int CTaskbarManager::GetCommandIcon(int Command) const
{
	for (int i=0;i<lengthof(m_CommandIconList);i++) {
		if (m_CommandIconList[i].Command==Command)
			return m_CommandIconList[i].Icon;
	}

	return 0;
}
