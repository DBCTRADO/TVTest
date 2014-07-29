#include "stdafx.h"
#include <algorithm>
#include <utility>
#include "TVTest.h"
#include "CasLibraryManager.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


static const TVTest::String NullString;




bool CCasLibraryManager::LoadSettings(CSettings &Settings)
{
	if (Settings.SetSection(TEXT("Settings"))) {
		Settings.Read(TEXT("CasLibrary"),&m_DefaultCasLibrary);
	}

	if (Settings.SetSection(TEXT("CasLibraryNetwork"))) {
		CSettings::EntryList Entries;

		if (Settings.GetEntries(&Entries)) {
			m_CasLibraryNetworkMap.reserve(Entries.size());

			for (size_t i=0;i<Entries.size();i++) {
				DWORD ID=std::_tcstoul(Entries[i].Name.c_str(),nullptr,16);
				CasLibraryNetworkInfo Info;

				Info.NetworkID=static_cast<WORD>(ID>>16);
				Info.TSID=static_cast<WORD>(ID&0xFFFF);
				Info.FileName=Entries[i].Value;

				m_CasLibraryNetworkMap.push_back(std::move(Info));
			}
		}
	}

	if (Settings.SetSection(TEXT("TunerCasMap"))) {
		CSettings::EntryList Entries;

		if (Settings.GetEntries(&Entries)) {
			m_TunerCasMap.reserve(Entries.size());

			for (size_t i=0;i<Entries.size();i++) {
				// BonDriver=[Device][|Reader]
				TunerCasInfo Info;

				Info.TunerName=Entries[i].Name;
				TVTest::String::size_type Delim=Entries[i].Value.find(_T('|'));
				if (Delim!=TVTest::String::npos) {
					if (Delim>0)
						Info.DeviceName=Entries[i].Value.substr(0,Delim);
					if (Entries[i].Value.length()>Delim+1)
						Info.ReaderName=Entries[i].Value.substr(Delim+1);
				} else {
					Info.DeviceName=Entries[i].Value;
				}

				m_TunerCasMap.push_back(std::move(Info));
			}
		}
	}

	return true;
}


void CCasLibraryManager::SetDefaultCasLibrary(LPCTSTR pszFileName)
{
	if (IsStringEmpty(pszFileName))
		m_DefaultCasLibrary.clear();
	else
		m_DefaultCasLibrary=pszFileName;
}


bool CCasLibraryManager::FindCasLibraries(
	LPCTSTR pszDirectory,std::vector<TVTest::String> *pList) const
{
	if (pList==nullptr)
		return false;

	pList->clear();

	if (IsStringEmpty(pszDirectory))
		return false;

	TCHAR szMask[MAX_PATH];
	WIN32_FIND_DATA fd;

	if (::PathCombine(szMask,pszDirectory,TEXT("*.tvcas"))==nullptr)
		return false;
	HANDLE hFind=::FindFirstFile(szMask,&fd);
	if (hFind==INVALID_HANDLE_VALUE)
		return false;
	do {
		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0)
			pList->push_back(TVTest::String(fd.cFileName));
	} while (::FindNextFile(hFind,&fd));
	::FindClose(hFind);

	if (pList->size()>1) {
		std::sort(pList->begin(),pList->end(),
				  [](const TVTest::String &Lib1,const TVTest::String &Lib2) {
				      return TVTest::StringUtility::CompareNoCase(Lib1,Lib2)<0;
				  });
	}

	return true;
}


bool CCasLibraryManager::FindDefaultCasLibrary(LPCTSTR pszDirectory)
{
	std::vector<TVTest::String> List;

	if (!FindCasLibraries(pszDirectory,&List) || List.empty())
		return false;
	m_DefaultCasLibrary=std::move(List.front());
	return true;
}


const TVTest::String &CCasLibraryManager::GetCasLibraryFileName(WORD NetworkID,WORD TSID) const
{
	const CasLibraryNetworkInfo *pInfo=nullptr;

	for (auto itr=m_CasLibraryNetworkMap.begin();itr!=m_CasLibraryNetworkMap.end();++itr) {
		if ((itr->NetworkID==NetworkID
					|| (itr->NetworkID==0xFFFF && (pInfo==nullptr || pInfo->NetworkID==0xFFFF)))
				&& (itr->TSID==TSID
					|| (itr->TSID==0xFFFF && (pInfo==nullptr || pInfo->TSID==0xFFFF))))
			pInfo=&*itr;
	}

	if (pInfo!=nullptr)
		return pInfo->FileName;

	return m_DefaultCasLibrary;
}


const TVTest::String &CCasLibraryManager::GetCasDeviceNameByTunerName(LPCTSTR pszTunerName) const
{
	auto itr=FindTunerCasMap(pszTunerName);

	if (itr==m_TunerCasMap.end())
		return NullString;

	return itr->DeviceName;
}


const TVTest::String &CCasLibraryManager::GetCasReaderNameByTunerName(LPCTSTR pszTunerName) const
{
	auto itr=FindTunerCasMap(pszTunerName);

	if (itr==m_TunerCasMap.end())
		return NullString;

	return itr->ReaderName;
}


std::vector<CCasLibraryManager::TunerCasInfo>::const_iterator CCasLibraryManager::FindTunerCasMap(LPCTSTR pszTunerName) const
{
	if (IsStringEmpty(pszTunerName))
		return m_TunerCasMap.end();

	for (auto itr=m_TunerCasMap.begin();itr!=m_TunerCasMap.end();++itr) {
		if (::PathMatchSpec(pszTunerName,itr->TunerName.c_str())) {
			return itr;
		}
	}

	LPCTSTR pszName=::PathFindFileName(pszTunerName);
	if (pszName!=pszTunerName) {
		for (auto itr=m_TunerCasMap.begin();itr!=m_TunerCasMap.end();++itr) {
			if (::PathMatchSpec(pszName,itr->TunerName.c_str())) {
				return itr;
			}
		}
	}

	return m_TunerCasMap.end();
}
