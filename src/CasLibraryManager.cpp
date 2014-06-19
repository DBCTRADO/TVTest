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
