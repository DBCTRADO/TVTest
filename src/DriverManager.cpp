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
#include "TVTest.h"
#include "AppMain.h"
#include "DriverManager.h"
#include <algorithm>
#include "Common/DebugDef.h"


namespace TVTest
{


CDriverInfo::CDriverInfo(LPCTSTR pszFileName)
	: m_FileName(pszFileName)
{
}


bool CDriverInfo::LoadTuningSpaceList(LoadTuningSpaceListMode Mode)
{
	CAppMain &App = GetAppClass();
	LPCTSTR pszFileName = m_FileName.c_str();

	bool fUseDriver;
	if (Mode == LoadTuningSpaceListMode::NoLoadDriver) {
		fUseDriver = false;
	} else if (Mode == LoadTuningSpaceListMode::UseDriver) {
		fUseDriver = true;
	} else {
		CDriverManager::TunerSpec Spec;
		if (App.DriverManager.GetTunerSpec(pszFileName, &Spec)
				&& !!(Spec.Flags & CDriverManager::TunerSpec::Flag::NoEnumChannel))
			fUseDriver = false;
		else
			fUseDriver = true;
	}

	if (!m_fChannelFileLoaded) {
		String ChannelFileName;

		App.Core.GetChannelFileName(pszFileName, &ChannelFileName);
		if (m_TuningSpaceList.LoadFromFile(ChannelFileName.c_str())) {
#if 0
			if (fUseDriver && Mode == LoadTuningSpaceListMode::Default) {
				const int NumSpaces = m_TuningSpaceList.NumSpaces();
				int i;
				for (i = 0; i < NumSpaces; i++) {
					if (m_TuningSpaceList.GetTuningSpaceName(i) == nullptr
							|| m_TuningSpaceList.GetChannelList(i)->NumChannels() == 0)
						break;
				}
				if (i == NumSpaces)
					fUseDriver = false;
			}
#else
			if (Mode == LoadTuningSpaceListMode::Default)
				fUseDriver = false;
#endif
			m_fChannelFileLoaded = true;
		} else {
			if (!fUseDriver && !m_fDriverSpaceLoaded)
				return false;
		}
	}

	if (fUseDriver && !m_fDriverSpaceLoaded) {
		CFilePath FilePath;

		if (::PathIsRelative(pszFileName)) {
			App.CoreEngine.GetDriverDirectoryPath(&FilePath);
			FilePath.Append(pszFileName);
			FilePath.Canonicalize();
		} else {
			FilePath = pszFileName;
		}

		const HMODULE hLib = ::GetModuleHandle(FilePath.c_str());
		if (hLib != nullptr) {
			String CurDriverPath;

			if (App.CoreEngine.GetDriverPath(&CurDriverPath)
					&& IsEqualFileName(FilePath.c_str(), CurDriverPath.c_str())) {
				m_DriverSpaceList = *App.ChannelManager.GetDriverTuningSpaceList();
				m_fDriverSpaceLoaded = true;
			}
		} else {
			LibISDB::BonDriver Driver;

			if (Driver.Load(FilePath) && Driver.CreateIBonDriver()) {
				if (Driver.IsIBonDriver2()) {
					int NumSpaces;

					for (NumSpaces = 0; Driver.EnumTuningSpace(NumSpaces) != nullptr; NumSpaces++);
					m_DriverSpaceList.Reserve(NumSpaces);
					StringUtility::Assign(m_TunerName, Driver.GetTunerName());
					for (int i = 0; i < NumSpaces; i++) {
						CTuningSpaceInfo *pTuningSpaceInfo = m_DriverSpaceList.GetTuningSpaceInfo(i);
						LPCTSTR pszName = Driver.EnumTuningSpace(i);

						pTuningSpaceInfo->SetName(pszName);
						CChannelList *pChannelList = pTuningSpaceInfo->GetChannelList();
						for (int j = 0; (pszName = Driver.EnumChannelName(i, j)) != nullptr; j++) {
							pChannelList->AddChannel(CChannelInfo(i, j, j + 1, pszName));
						}
					}
					m_fDriverSpaceLoaded = true;
				}
			}
		}
		for (int i = 0; i < m_TuningSpaceList.NumSpaces(); i++) {
			if (m_TuningSpaceList.GetTuningSpaceName(i) == nullptr)
				m_TuningSpaceList.GetTuningSpaceInfo(i)->SetName(m_DriverSpaceList.GetTuningSpaceName(i));
		}
	}
	if (!m_fChannelFileLoaded && !m_fDriverSpaceLoaded)
		return false;
	return true;
}


void CDriverInfo::ClearTuningSpaceList()
{
	m_TuningSpaceList.Clear();
	m_fChannelFileLoaded = false;
	m_DriverSpaceList.Clear();
	m_fDriverSpaceLoaded = false;
}


const CTuningSpaceList *CDriverInfo::GetAvailableTuningSpaceList() const
{
	if (m_fChannelFileLoaded)
		return &m_TuningSpaceList;
	if (m_fDriverSpaceLoaded)
		return &m_DriverSpaceList;
	return nullptr;
}


const CChannelList *CDriverInfo::GetChannelList(int Space) const
{
	const CChannelList *pChannelList = m_TuningSpaceList.GetChannelList(Space);

	if (pChannelList == nullptr) {
		pChannelList = m_DriverSpaceList.GetChannelList(Space);
	} else if (pChannelList->NumChannels() == 0) {
		const CChannelList *pDriverChannelList = m_DriverSpaceList.GetChannelList(Space);

		if (pDriverChannelList != nullptr && pDriverChannelList->NumChannels() > 0)
			pChannelList = pDriverChannelList;
	}
	return pChannelList;
}




void CDriverManager::Clear()
{
	m_DriverList.clear();
}


bool CDriverManager::Find(LPCTSTR pszDirectory)
{
	if (pszDirectory == nullptr)
		return false;

	Clear();

	TCHAR szMask[MAX_PATH];
	WIN32_FIND_DATA wfd;

	::PathCombine(szMask, pszDirectory, TEXT("BonDriver*.dll"));
	const HANDLE hFind = ::FindFirstFileEx(szMask, FindExInfoBasic, &wfd, FindExSearchNameMatch, nullptr, 0);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
				m_DriverList.emplace_back(std::make_unique<CDriverInfo>(wfd.cFileName));
			}
		} while (::FindNextFile(hFind, &wfd));
		::FindClose(hFind);
	}

	if (m_DriverList.size() > 1) {
		std::ranges::sort(
			m_DriverList,
			[](const std::unique_ptr<CDriverInfo> &Driver1,
			   const std::unique_ptr<CDriverInfo> &Driver2) {
				return ::lstrcmpi(Driver1->GetFileName(), Driver2->GetFileName()) < 0;
			});
	}

	m_BaseDirectory = pszDirectory;

	return true;
}


CDriverInfo *CDriverManager::GetDriverInfo(int Index)
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_DriverList.size())
		return nullptr;
	return m_DriverList[Index].get();
}


const CDriverInfo *CDriverManager::GetDriverInfo(int Index) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_DriverList.size())
		return nullptr;
	return m_DriverList[Index].get();
}


int CDriverManager::FindByFileName(LPCTSTR pszFileName) const
{
	if (IsStringEmpty(pszFileName))
		return -1;
	for (size_t i = 0; i < m_DriverList.size(); i++) {
		if (IsEqualFileName(m_DriverList[i]->GetFileName(), pszFileName))
			return static_cast<int>(i);
	}
	return -1;
}


bool CDriverManager::GetAllServiceList(CChannelList *pList) const
{
	if (pList == nullptr)
		return false;

	pList->Clear();

	for (const auto &e : m_DriverList) {
		const LPCTSTR pszFileName = e->GetFileName();
		const LPCTSTR pszExtension = ::PathFindExtension(pszFileName);
		if (pszExtension > pszFileName
				&& *(pszExtension - 1) >= _T('1') && *(pszExtension - 1) <= _T('9')) {
			TCHAR szFirstFile[MAX_PATH];
			StringCopy(szFirstFile, pszFileName);
			szFirstFile[pszExtension - 1 - pszFileName] = _T('0');
			if (FindByFileName(szFirstFile) >= 0)
				continue;
		}

		if (e->LoadTuningSpaceList(CDriverInfo::LoadTuningSpaceListMode::NoLoadDriver)) {
			const CTuningSpaceList *pTuningSpaceList = e->GetTuningSpaceList();
			const CChannelList *pChannelList = pTuningSpaceList->GetAllChannelList();
			const int NumChannels = pChannelList->NumChannels();

			for (int i = 0; i < NumChannels; i++) {
				const CChannelInfo *pChannelInfo = pChannelList->GetChannelInfo(i);

				if (pChannelInfo->IsEnabled()
						&& pChannelInfo->GetNetworkID() > 0
						&& pChannelInfo->GetTransportStreamID() > 0
						&& pChannelInfo->GetServiceID() > 0
						&& pList->FindByIDs(
							pChannelInfo->GetNetworkID(),
							pChannelInfo->GetTransportStreamID(),
							pChannelInfo->GetServiceID()) < 0) {
					pList->AddChannel(new CTunerChannelInfo(*pChannelInfo, e->GetFileName()));
				}
			}
		}
	}

	return true;
}


bool CDriverManager::LoadTunerSpec(LPCTSTR pszFileName)
{
	CSettings Settings;

	if (!Settings.Open(pszFileName, CSettings::OpenFlag::Read)
			|| !Settings.SetSection(TEXT("TunerSpec")))
		return false;

	CSettings::EntryList Entries;
	if (!Settings.GetEntries(&Entries))
		return false;

	for (const auto &e : Entries) {
		TunerSpecInfo Info;

		Info.TunerMask = e.Name;
		Info.Spec.Flags = TunerSpec::Flag::None;

		std::vector<String> Attributes;
		if (StringUtility::Split(e.Value, TEXT("|"), &Attributes)) {
			static const struct {
				StringView Name;
				TunerSpec::Flag Flag;
			} FlagList[] = {
				{TEXT("network"),         TunerSpec::Flag::Network},
				{TEXT("file"),            TunerSpec::Flag::File},
				{TEXT("virtual"),         TunerSpec::Flag::Virtual},
				{TEXT("volatile"),        TunerSpec::Flag::Volatile},
				{TEXT("no-enum-channel"), TunerSpec::Flag::NoEnumChannel},
			};
			for (String Attribute : Attributes) {
				StringUtility::Trim(Attribute);
				for (auto &Map : FlagList) {
					if (StringUtility::IsEqualNoCase(Attribute, Map.Name)) {
						Info.Spec.Flags |= Map.Flag;
						break;
					}
				}
			}
		}

		m_TunerSpecList.push_back(Info);
	}

	return true;
}


bool CDriverManager::GetTunerSpec(LPCTSTR pszTunerName, TunerSpec *pSpec) const
{
	if (IsStringEmpty(pszTunerName) || pSpec == nullptr)
		return false;

	const LPCTSTR pszName = ::PathFindFileName(pszTunerName);
	if (pszName[0] == _T('\0'))
		return false;

	for (const auto &e : m_TunerSpecList) {
		if (::PathMatchSpec(pszName, e.TunerMask.c_str())) {
			*pSpec = e.Spec;
			return true;
		}
	}

	return false;
}


} // namespace TVTest
