/*
  TVTest
  Copyright(c) 2008-2020 DBCTRADO

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "stdafx.h"
#include <propvarutil.h>
#include <propkey.h>
#include "TVTest.h"
#include "Taskbar.h"
#include "AppMain.h"
#include "LibISDB/LibISDB/Utilities/MD5.hpp"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


const CTaskbarManager::CommandIconInfo CTaskbarManager::m_CommandIconList[] =
{
	{CM_FULLSCREEN,    IDI_TASK_FULLSCREEN},
	{CM_DISABLEVIEWER, IDI_TASK_DISABLEVIEWER},
	{CM_PROGRAMGUIDE,  IDI_TASK_PROGRAMGUIDE},
};


CTaskbarManager::~CTaskbarManager()
{
	Finalize();
}


void CTaskbarManager::SetAppID(LPCTSTR pszID)
{
	StringUtility::Assign(m_AppID, pszID);
}


bool CTaskbarManager::Initialize(HWND hwnd)
{
	if (m_TaskbarButtonCreatedMessage == 0) {
		m_TaskbarButtonCreatedMessage = ::RegisterWindowMessage(TEXT("TaskbarButtonCreated"));

		::ChangeWindowMessageFilterEx(hwnd, m_TaskbarButtonCreatedMessage, MSGFLT_ALLOW, nullptr);
		::ChangeWindowMessageFilterEx(hwnd, WM_COMMAND, MSGFLT_ALLOW, nullptr);

		m_hwnd = hwnd;

		CAppMain &App = GetAppClass();
		HRESULT hr = S_OK;

		if (!m_AppID.empty()) {
			hr = ::SetCurrentProcessExplicitAppUserModelID(m_AppID.c_str());
			if (FAILED(hr)) {
				m_fAppIDInvalid = true;
				App.AddLog(
					CLogItem::LogType::Error,
					TEXT("AppID \"{}\" を設定できません。({:08x})"),
					m_AppID, hr);
			}
		}
		if (SUCCEEDED(hr)) {
			if (App.TaskbarOptions.IsJumpListEnabled()) {
				hr = InitializeJumpList();
				if (SUCCEEDED(hr))
					m_fJumpListInitialized = true;
			} else {
				ClearJumpList();
			}
		}
	}
	return true;
}


void CTaskbarManager::Finalize()
{
	if (m_pTaskbarList != nullptr) {
		m_pTaskbarList->Release();
		m_pTaskbarList = nullptr;
	}

	if (m_SharedProperties.IsOpened()) {
		if (m_SharedProperties.GetRecentChannelList(&m_RecentChannelList))
			m_fSaveRecentChannelList = true;
		m_SharedProperties.Close();
	}

	m_hwnd = nullptr;
}


bool CTaskbarManager::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg != 0 && uMsg == m_TaskbarButtonCreatedMessage) {
		if (m_pTaskbarList == nullptr) {
			if (FAILED(::CoCreateInstance(
						CLSID_TaskbarList, nullptr,
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
		const CAppMain &App = GetAppClass();
		const HINSTANCE hinst = App.GetResourceInstance();

		for (int i = 0; i < lengthof(tb); i++) {
			const int Command = ButtonList[i];

			tb[i].dwMask = static_cast<THUMBBUTTONMASK>(THB_ICON | THB_TOOLTIP | THB_FLAGS);
			tb[i].iId = Command;
			tb[i].hIcon = ::LoadIcon(hinst, MAKEINTRESOURCE(GetCommandIcon(Command)));
			App.CommandManager.GetCommandText(Command, tb[i].szTip, lengthof(tb[0].szTip));
			tb[i].dwFlags = static_cast<THUMBBUTTONFLAGS>(THBF_ENABLED | THBF_DISMISSONCLICK);
		}

		m_pTaskbarList->ThumbBarAddButtons(m_hwnd, lengthof(tb), tb);

		return true;
	}

	return false;
}


bool CTaskbarManager::SetRecordingStatus(bool fRecording)
{
	if (m_pTaskbarList != nullptr) {
		if (fRecording) {
			const HICON hico = static_cast<HICON>(
				::LoadImage(
					GetAppClass().GetResourceInstance(),
					MAKEINTRESOURCE(IDI_TASKBAR_RECORDING),
					IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));

			if (hico == nullptr)
				return false;
			m_pTaskbarList->SetOverlayIcon(m_hwnd, hico, TEXT("録画中"));
			::DestroyIcon(hico);
		} else {
			m_pTaskbarList->SetOverlayIcon(m_hwnd, nullptr, nullptr);
		}
		return true;
	}
	return false;
}


bool CTaskbarManager::SetProgress(int Pos, int Max)
{
	if (m_pTaskbarList != nullptr) {
		if (Pos >= Max) {
			m_pTaskbarList->SetProgressState(m_hwnd, TBPF_NOPROGRESS);
		} else {
			if (Pos == 0)
				m_pTaskbarList->SetProgressState(m_hwnd, TBPF_NORMAL);
			m_pTaskbarList->SetProgressValue(m_hwnd, Pos, Max);
		}
		return true;
	}
	return false;
}


bool CTaskbarManager::EndProgress()
{
	if (m_pTaskbarList != nullptr)
		m_pTaskbarList->SetProgressState(m_hwnd, TBPF_NOPROGRESS);
	return true;
}


bool CTaskbarManager::ReinitializeJumpList()
{
	if (m_fAppIDInvalid)
		return false;
	if (GetAppClass().TaskbarOptions.IsJumpListEnabled()) {
		const HRESULT hr = InitializeJumpList();
		if (FAILED(hr))
			return false;
		m_fJumpListInitialized = true;
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

	hr = ::CoCreateInstance(
		CLSID_DestinationList, nullptr,
		CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pcdl));
	if (FAILED(hr))
		return false;

	hr = pcdl->DeleteList(m_AppID.empty() ? nullptr : m_AppID.c_str());
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
		String PropName;

		if (m_AppID.empty()) {
			TCHAR szAppPath[MAX_PATH];
			const DWORD Length = ::GetModuleFileName(nullptr, szAppPath, lengthof(szAppPath));
			::CharLowerBuff(szAppPath, Length);
			LibISDB::MD5Value Hash =
				LibISDB::CalcMD5(reinterpret_cast<const uint8_t *>(szAppPath), Length * sizeof(TCHAR));
			StringFormat(
				&PropName,
				APP_NAME TEXT("_{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}_TaskbarProp"),
				Hash.Value[ 0], Hash.Value[ 1], Hash.Value[ 2], Hash.Value[ 3], Hash.Value[ 4], Hash.Value[ 5], Hash.Value[ 6], Hash.Value[ 7],
				Hash.Value[ 8], Hash.Value[ 9], Hash.Value[10], Hash.Value[11], Hash.Value[12], Hash.Value[13], Hash.Value[14], Hash.Value[15]);
		} else {
			PropName = m_AppID;
			PropName += TEXT("_TaskbarProp");
		}

		m_SharedProperties.Open(PropName.c_str(), &m_RecentChannelList);
	}

	HRESULT hr;
	ICustomDestinationList *pcdl;

	hr = ::CoCreateInstance(CLSID_DestinationList, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pcdl));
	if (FAILED(hr))
		return hr;

	if (!m_AppID.empty())
		pcdl->SetAppID(m_AppID.c_str());

	UINT MinSlots;
	IObjectArray *pRemovedList;

	hr = pcdl->BeginList(&MinSlots, IID_PPV_ARGS(&pRemovedList));
	if (SUCCEEDED(hr)) {
		const CTaskbarOptions &TaskbarOptions = GetAppClass().TaskbarOptions;

		if (TaskbarOptions.GetShowTasks())
			hr = AddTaskList(pcdl);
		if (SUCCEEDED(hr)) {
			if (TaskbarOptions.GetShowRecentChannels())
				hr = AddRecentChannelsCategory(pcdl);
		}

		pRemovedList->Release();
	}

	if (SUCCEEDED(hr))
		hr = pcdl->CommitList();

	pcdl->Release();

	return hr;
}


HRESULT CTaskbarManager::AddTaskList(ICustomDestinationList *pcdl)
{
	const CAppMain &App = GetAppClass();
	const CTaskbarOptions::TaskList &TaskList = App.TaskbarOptions.GetTaskList();

	if (TaskList.empty())
		return S_FALSE;

	HRESULT hr;
	IObjectCollection *pCollection;

	hr = ::CoCreateInstance(
		CLSID_EnumerableObjectCollection, nullptr,
		CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pCollection));
	if (SUCCEEDED(hr)) {
		WCHAR szIconPath[MAX_PATH];

		::GetModuleFileNameW(nullptr, szIconPath, lengthof(szIconPath));

		for (const int Command : TaskList) {
			IShellLink *pShellLink;

			if (Command == 0) {
				hr = CreateSeparatorShellLink(&pShellLink);
			} else {
				WCHAR szArgs[CCommandManager::MAX_COMMAND_ID_TEXT + 32];
				WCHAR szTitle[CCommandManager::MAX_COMMAND_TEXT];

				if (Command == CM_PROGRAMGUIDE) {
					StringCopy(szArgs, L"/jumplist /s /epgonly");
				} else {
					StringFormat(
						szArgs, L"/jumplist /s /command {}",
						App.CommandManager.GetCommandIDText(Command));
				}
				App.CommandManager.GetCommandText(Command, szTitle, lengthof(szTitle));
				const int Icon = GetCommandIcon(Command);
				hr = CreateAppShellLink(szArgs, szTitle, nullptr, Icon != 0 ? szIconPath : nullptr, -Icon, &pShellLink);
			}

			if (SUCCEEDED(hr)) {
				pCollection->AddObject(pShellLink);
				pShellLink->Release();
			}
		}

		IObjectArray *pArray;
		hr = pCollection->QueryInterface(IID_PPV_ARGS(&pArray));
		if (SUCCEEDED(hr)) {
			hr = pcdl->AddUserTasks(pArray);
			pArray->Release();
		}

		pCollection->Release();
	}

	return hr;
}


HRESULT CTaskbarManager::CreateAppShellLink(
	PCWSTR pszArgs, PCWSTR pszTitle, PCWSTR pszDescription,
	PCWSTR pszIconPath, int IconIndex,
	IShellLink **ppShellLink)
{
	WCHAR szAppPath[MAX_PATH];
	const DWORD Length = ::GetModuleFileNameW(nullptr, szAppPath, lengthof(szAppPath));
	if (Length == 0 || Length >= lengthof(szAppPath) - 1)
		return E_FAIL;

	HRESULT hr;
	IShellLink *pShellLink;

	hr = ::CoCreateInstance(
		CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pShellLink));
	if (SUCCEEDED(hr)) {
		hr = pShellLink->SetPath(szAppPath);
		if (SUCCEEDED(hr)) {
			hr = pShellLink->SetArguments(pszArgs);
			if (SUCCEEDED(hr)) {
				if (!IsStringEmpty(pszDescription))
					pShellLink->SetDescription(pszDescription);
				if (!IsStringEmpty(pszIconPath))
					pShellLink->SetIconLocation(pszIconPath, IconIndex);

				IPropertyStore *pPropStore;

				hr = pShellLink->QueryInterface(IID_PPV_ARGS(&pPropStore));
				if (SUCCEEDED(hr)) {
					PROPVARIANT PropVar;

					hr = ::InitPropVariantFromString(pszTitle, &PropVar);
					if (SUCCEEDED(hr)) {
						hr = pPropStore->SetValue(PKEY_Title, PropVar);
						if (SUCCEEDED(hr)) {
							hr = pPropStore->Commit();
							if (SUCCEEDED(hr)) {
								pShellLink->AddRef();
								*ppShellLink = pShellLink;
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

	hr = ::CoCreateInstance(
		CLSID_ShellLink, nullptr,
		CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pShellLink));
	if (SUCCEEDED(hr)) {
		IPropertyStore *pPropStore;

		hr = pShellLink->QueryInterface(IID_PPV_ARGS(&pPropStore));
		if (SUCCEEDED(hr)) {
			PROPVARIANT PropVar;

			hr = ::InitPropVariantFromBoolean(TRUE, &PropVar);
			if (SUCCEEDED(hr)) {
				hr = pPropStore->SetValue(PKEY_AppUserModel_IsDestListSeparator, PropVar);
				if (SUCCEEDED(hr)) {
					hr = pPropStore->Commit();
					if (SUCCEEDED(hr)) {
						pShellLink->AddRef();
						*ppShellLink = pShellLink;
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
	PCWSTR pszTitle, const JumpListItemList &ItemList)
{
	HRESULT hr;
	IObjectCollection *pCollection;

	hr = ::CoCreateInstance(
		CLSID_EnumerableObjectCollection, nullptr,
		CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pCollection));
	if (SUCCEEDED(hr)) {
		const CAppMain &App = GetAppClass();

		for (const auto &e : ItemList) {
			IShellLink *pShellLink;

			hr = CreateAppShellLink(
				e.Args.c_str(), e.Title.c_str(), e.Description.c_str(),
				e.IconPath.c_str(), e.IconIndex,
				&pShellLink);
			if (SUCCEEDED(hr)) {
				pCollection->AddObject(pShellLink);
				pShellLink->Release();
			}
		}

		IObjectArray *pArray;

		hr = pCollection->QueryInterface(IID_PPV_ARGS(&pArray));
		if (SUCCEEDED(hr)) {
			hr = pcdl->AppendCategory(pszTitle, pArray);
			pArray->Release();
		}

		pCollection->Release();
	}

	return hr;
}


HRESULT CTaskbarManager::AddRecentChannelsCategory(ICustomDestinationList *pcdl)
{
	CAppMain &App = GetAppClass();
	const CRecentChannelList *pRecentChannels;
	CRecentChannelList SharedRecentChannels;

	if (m_SharedProperties.GetRecentChannelList(&SharedRecentChannels))
		pRecentChannels = &SharedRecentChannels;
	else
		pRecentChannels = &App.RecentChannelList;

	int NumChannels = pRecentChannels->NumChannels();

	if (NumChannels == 0)
		return S_FALSE;

	if (NumChannels > App.TaskbarOptions.GetMaxRecentChannels())
		NumChannels = App.TaskbarOptions.GetMaxRecentChannels();

	bool fShowIcon = App.TaskbarOptions.GetShowChannelIcon();
	TCHAR szIconDir[MAX_PATH];
	if (fShowIcon) {
		if (GetAbsolutePath(
					App.TaskbarOptions.GetIconDirectory().c_str(),
					szIconDir, lengthof(szIconDir) - 13)) {
			if (!::PathIsDirectory(szIconDir)) {
				const int Result = ::SHCreateDirectoryEx(nullptr, szIconDir, nullptr);
				if (Result != ERROR_SUCCESS && Result != ERROR_ALREADY_EXISTS) {
					App.AddLog(
						CLogItem::LogType::Error,
						TEXT("ジャンプリストアイコンフォルダ \"{}\" が作成できません。"),
						szIconDir);
					fShowIcon = false;
				} else if (Result == ERROR_SUCCESS) {
					App.AddLog(
						CLogItem::LogType::Information,
						TEXT("ジャンプリストアイコンフォルダ \"{}\" を作成しました。"),
						szIconDir);
				}
			}
		} else {
			fShowIcon = false;
		}
	}

	JumpListItemList List;

	for (int i = 0; i < NumChannels; i++) {
		const CTunerChannelInfo *pChannel = pRecentChannels->GetChannelInfo(i);
		const WORD NetworkID = pChannel->GetNetworkID();
		const WORD ServiceID = pChannel->GetServiceID();
		const LPCWSTR pszTunerName = pChannel->GetTunerName();
		JumpListItem Item;
		String Driver;
		WCHAR szTuner[MAX_PATH];

		Item.Title = pChannel->GetName();
		if (::StrChrW(pszTunerName, L' ') != nullptr) {
			Driver = L'"';
			Driver += pszTunerName;
			Driver += L'"';
		}
		StringFormat(
			&Item.Args, L"/jumplist /d {} /chspace {} /chi {} /nid {} /sid {}",
			!Driver.empty() ? Driver.c_str() : pszTunerName,
			pChannel->GetSpace(),
			pChannel->GetChannelIndex(),
			NetworkID, ServiceID);
		StringCopy(szTuner, pszTunerName);
		::PathRemoveExtensionW(szTuner);
		StringFormat(
			&Item.Description, L"{} ({})",
			pChannel->GetName(),
			::StrCmpNIW(szTuner, L"BonDriver_", 10) == 0 ? szTuner + 10 : szTuner);

		if (fShowIcon && NetworkID != 0 && ServiceID != 0) {
			TCHAR szIconPath[MAX_PATH];
			TCHAR szFileName[16];

			StringFormat(
				szFileName,
				TEXT("{:04x}{:04x}.ico"), NetworkID, ServiceID);
			::PathCombine(szIconPath, szIconDir, szFileName);

			bool fUseIcon = true;
			const DWORD MapKey = ChannelIconMapKey(NetworkID, ServiceID);
			constexpr BYTE LogoType = CLogoManager::LOGOTYPE_48x24;
			CLogoManager::LogoInfo LogoInfo;
			if (App.LogoManager.GetLogoInfo(NetworkID, ServiceID, LogoType, &LogoInfo)) {
				bool fSave;
				auto it = m_ChannelIconMap.find(MapKey);
				if (it != m_ChannelIconMap.end())
					fSave = CompareFileTime(&it->second.UpdatedTime, &LogoInfo.UpdatedTime) < 0;
				else
					fSave = true;
				if (fSave) {
					WIN32_FILE_ATTRIBUTE_DATA AttributeData;
					const bool fFound = ::GetFileAttributesEx(szIconPath, GetFileExInfoStandard, &AttributeData) != FALSE;
					if (!fFound
							|| ::CompareFileTime(&AttributeData.ftLastWriteTime, &LogoInfo.UpdatedTime) < 0) {
						if (App.LogoManager.SaveLogoIcon(
									NetworkID, ServiceID, LogoType,
									::GetSystemMetrics(SM_CXSMICON),
									::GetSystemMetrics(SM_CYSMICON),
									szIconPath)) {
							m_ChannelIconMap[MapKey] = ChannelIconInfo(LogoInfo.UpdatedTime);
						} else {
							fUseIcon = false;
						}
					} else if (fFound) {
						m_ChannelIconMap[MapKey] = ChannelIconInfo(AttributeData.ftLastWriteTime);
					}
				}
			} else {
				auto it = m_ChannelIconMap.find(MapKey);
				if (it == m_ChannelIconMap.end()) {
					WIN32_FILE_ATTRIBUTE_DATA AttributeData;
					if (::GetFileAttributesEx(szIconPath, GetFileExInfoStandard, &AttributeData)) {
						m_ChannelIconMap[MapKey] = ChannelIconInfo(AttributeData.ftLastWriteTime);
					} else {
						fUseIcon = false;
					}
				}
			}

			if (fUseIcon) {
				Item.IconPath = szIconPath;
				Item.IconIndex = 0;
			}
		}

		List.push_back(Item);
	}

	WCHAR szTitle[64];

	::LoadStringW(
		App.GetResourceInstance(), IDS_MENU_CHANNELHISTORY,
		szTitle, lengthof(szTitle));

	return AddJumpListCategory(pcdl, szTitle, List);
}


int CTaskbarManager::GetCommandIcon(int Command) const
{
	for (const auto &e : m_CommandIconList) {
		if (e.Command == Command)
			return e.Icon;
	}

	return 0;
}


} // namespace TVTest
