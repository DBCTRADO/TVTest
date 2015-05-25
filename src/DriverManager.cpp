#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "DriverManager.h"
#include "IBonDriver2.h"
#include <algorithm>
#include "Common/DebugDef.h"


typedef IBonDriver *(*CreateBonDriverFunc)();




CDriverInfo::CDriverInfo(LPCTSTR pszFileName)
	: m_FileName(pszFileName)
	, m_fChannelFileLoaded(false)
	, m_fDriverSpaceLoaded(false)
{
}


CDriverInfo::~CDriverInfo()
{
}


bool CDriverInfo::LoadTuningSpaceList(LoadTuningSpaceListMode Mode)
{
	CAppMain &App=GetAppClass();
	LPCTSTR pszFileName=m_FileName.c_str();

	bool fUseDriver;
	if (Mode==LOADTUNINGSPACE_NOLOADDRIVER) {
		fUseDriver=false;
	} else if (Mode==LOADTUNINGSPACE_USEDRIVER) {
		fUseDriver=true;
	} else {
		CDriverManager::TunerSpec Spec;
		if (App.DriverManager.GetTunerSpec(pszFileName,&Spec)
				&& (Spec.Flags & CDriverManager::TunerSpec::FLAG_NOENUMCHANNEL)!=0)
			fUseDriver=false;
		else
			fUseDriver=true;
	}

	if (!m_fChannelFileLoaded) {
		TCHAR szChannelFileName[MAX_PATH];

		App.Core.GetChannelFileName(pszFileName,szChannelFileName,lengthof(szChannelFileName));
		if (m_TuningSpaceList.LoadFromFile(szChannelFileName)) {
#if 0
			if (fUseDriver && Mode==LOADTUNINGSPACE_DEFAULT) {
				const int NumSpaces=m_TuningSpaceList.NumSpaces();
				int i;
				for (i=0;i<NumSpaces;i++) {
					if (m_TuningSpaceList.GetTuningSpaceName(i)==NULL
							|| m_TuningSpaceList.GetChannelList(i)->NumChannels()==0)
						break;
				}
				if (i==NumSpaces)
					fUseDriver=false;
			}
#else
			if (Mode==LOADTUNINGSPACE_DEFAULT)
				fUseDriver=false;
#endif
			m_fChannelFileLoaded=true;
		} else {
			if (!fUseDriver && !m_fDriverSpaceLoaded)
				return false;
		}
	}

	if (fUseDriver && !m_fDriverSpaceLoaded) {
		TCHAR szFilePath[MAX_PATH];

		if (::PathIsRelative(pszFileName)) {
			TCHAR szTemp[MAX_PATH];
			App.Core.GetDriverDirectory(szTemp,lengthof(szTemp));
			::PathAppend(szTemp,pszFileName);
			::PathCanonicalize(szFilePath,szTemp);
		} else {
			::lstrcpy(szFilePath,pszFileName);
		}

		HMODULE hLib=::GetModuleHandle(szFilePath);
		if (hLib!=NULL) {
			TCHAR szCurDriverPath[MAX_PATH];

			if (App.CoreEngine.GetDriverPath(szCurDriverPath,lengthof(szCurDriverPath))
					&& IsEqualFileName(szFilePath,szCurDriverPath)) {
				m_DriverSpaceList=*App.ChannelManager.GetDriverTuningSpaceList();
				m_fDriverSpaceLoaded=true;
			}
		} else if ((hLib=::LoadLibrary(szFilePath))!=NULL) {
			CreateBonDriverFunc pCreate=
				reinterpret_cast<CreateBonDriverFunc>(::GetProcAddress(hLib,"CreateBonDriver"));
			IBonDriver *pBonDriver;

			if (pCreate!=NULL && (pBonDriver=pCreate())!=NULL) {
				IBonDriver2 *pBonDriver2=dynamic_cast<IBonDriver2*>(pBonDriver);

				if (pBonDriver2!=NULL) {
					int NumSpaces;

					for (NumSpaces=0;pBonDriver2->EnumTuningSpace(NumSpaces)!=NULL;NumSpaces++);
					m_DriverSpaceList.Reserve(NumSpaces);
					TVTest::StringUtility::Assign(m_TunerName,pBonDriver2->GetTunerName());
					for (int i=0;i<NumSpaces;i++) {
						CTuningSpaceInfo *pTuningSpaceInfo=m_DriverSpaceList.GetTuningSpaceInfo(i);
						LPCTSTR pszName=pBonDriver2->EnumTuningSpace(i);

						pTuningSpaceInfo->SetName(pszName);
						CChannelList *pChannelList=pTuningSpaceInfo->GetChannelList();
						for (int j=0;(pszName=pBonDriver2->EnumChannelName(i,j))!=NULL;j++) {
							pChannelList->AddChannel(CChannelInfo(i,j,j+1,pszName));
						}
					}
					m_fDriverSpaceLoaded=true;
				}
				pBonDriver->Release();
			}
			::FreeLibrary(hLib);
		}
		for (int i=0;i<m_TuningSpaceList.NumSpaces();i++) {
			if (m_TuningSpaceList.GetTuningSpaceName(i)==NULL)
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
	m_fChannelFileLoaded=false;
	m_DriverSpaceList.Clear();
	m_fDriverSpaceLoaded=false;
}


const CTuningSpaceList *CDriverInfo::GetAvailableTuningSpaceList() const
{
	if (m_fChannelFileLoaded)
		return &m_TuningSpaceList;
	if (m_fDriverSpaceLoaded)
		return &m_DriverSpaceList;
	return NULL;
}


const CChannelList *CDriverInfo::GetChannelList(int Space) const
{
	const CChannelList *pChannelList;

	pChannelList=m_TuningSpaceList.GetChannelList(Space);
	if (pChannelList==NULL) {
		pChannelList=m_DriverSpaceList.GetChannelList(Space);
	} else if (pChannelList->NumChannels()==0) {
		const CChannelList *pDriverChannelList=m_DriverSpaceList.GetChannelList(Space);

		if (pDriverChannelList!=NULL && pDriverChannelList->NumChannels()>0)
			pChannelList=pDriverChannelList;
	}
	return pChannelList;
}




CDriverManager::CDriverManager()
{
}


CDriverManager::~CDriverManager()
{
	Clear();
}


void CDriverManager::Clear()
{
	for (size_t i=0;i<m_DriverList.size();i++)
		delete m_DriverList[i];
	m_DriverList.clear();
}


bool CDriverManager::Find(LPCTSTR pszDirectory)
{
	if (pszDirectory==NULL)
		return false;

	Clear();

	TCHAR szMask[MAX_PATH];
	HANDLE hFind;
	WIN32_FIND_DATA wfd;

	::PathCombine(szMask,pszDirectory,TEXT("BonDriver*.dll"));
	hFind=::FindFirstFile(szMask,&wfd);
	if (hFind!=INVALID_HANDLE_VALUE) {
		do {
			if ((wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)==0) {
				m_DriverList.push_back(new CDriverInfo(wfd.cFileName));
			}
		} while (::FindNextFile(hFind,&wfd));
		::FindClose(hFind);
	}

	if (m_DriverList.size()>1) {
		std::sort(m_DriverList.begin(),m_DriverList.end(),
				  [](const CDriverInfo *pDriver1,const CDriverInfo *pDriver2) {
				      return ::lstrcmpi(pDriver1->GetFileName(),pDriver2->GetFileName())<0;
				  });
	}

	m_BaseDirectory=pszDirectory;

	return true;
}


CDriverInfo *CDriverManager::GetDriverInfo(int Index)
{
	if (Index<0 || (size_t)Index>=m_DriverList.size())
		return NULL;
	return m_DriverList[Index];
}


const CDriverInfo *CDriverManager::GetDriverInfo(int Index) const
{
	if (Index<0 || (size_t)Index>=m_DriverList.size())
		return NULL;
	return m_DriverList[Index];
}


int CDriverManager::FindByFileName(LPCTSTR pszFileName) const
{
	if (IsStringEmpty(pszFileName))
		return -1;
	for (size_t i=0;i<m_DriverList.size();i++) {
		if (IsEqualFileName(m_DriverList[i]->GetFileName(),pszFileName))
			return (int)i;
	}
	return -1;
}


bool CDriverManager::GetAllServiceList(CChannelList *pList) const
{
	if (pList==NULL)
		return false;

	pList->Clear();

	for (auto it=m_DriverList.begin();it!=m_DriverList.end();++it) {
		LPCTSTR pszFileName=(*it)->GetFileName();
		LPCTSTR pszExtension=::PathFindExtension(pszFileName);
		if (pszExtension>pszFileName
				&& *(pszExtension-1)>=_T('1') && *(pszExtension-1)<=_T('9')) {
			TCHAR szFirstFile[MAX_PATH];
			::lstrcpy(szFirstFile,pszFileName);
			szFirstFile[pszExtension-1-pszFileName]=_T('0');
			if (FindByFileName(szFirstFile)>=0)
				continue;
		}

		if ((*it)->LoadTuningSpaceList(CDriverInfo::LOADTUNINGSPACE_NOLOADDRIVER)) {
			const CTuningSpaceList *pTuningSpaceList=(*it)->GetTuningSpaceList();
			const CChannelList *pChannelList=pTuningSpaceList->GetAllChannelList();
			const int NumChannels=pChannelList->NumChannels();

			for (int i=0;i<NumChannels;i++) {
				const CChannelInfo *pChannelInfo=pChannelList->GetChannelInfo(i);

				if (pChannelInfo->IsEnabled()
						&& pChannelInfo->GetNetworkID()>0
						&& pChannelInfo->GetTransportStreamID()>0
						&& pChannelInfo->GetServiceID()>0
						&& pList->FindByIDs(
							pChannelInfo->GetNetworkID(),
							pChannelInfo->GetTransportStreamID(),
							pChannelInfo->GetServiceID())<0) {
					pList->AddChannel(new CTunerChannelInfo(*pChannelInfo,(*it)->GetFileName()));
				}
			}
		}
	}

	return true;
}


bool CDriverManager::LoadTunerSpec(LPCTSTR pszFileName)
{
	CSettings Settings;

	if (!Settings.Open(pszFileName,CSettings::OPEN_READ)
			|| !Settings.SetSection(TEXT("TunerSpec")))
		return false;

	CSettings::EntryList Entries;
	if (!Settings.GetEntries(&Entries))
		return false;

	for (auto it=Entries.begin();it!=Entries.end();++it) {
		TunerSpecInfo Info;

		Info.TunerMask=it->Name;
		Info.Spec.Flags=0;

		std::vector<TVTest::String> Attributes;
		if (TVTest::StringUtility::Split(it->Value,TEXT("|"),&Attributes)) {
			static const struct {
				LPCTSTR pszName;
				unsigned int Flag;
			} FlagList[] = {
				{TEXT("network"),			TunerSpec::FLAG_NETWORK},
				{TEXT("file"),				TunerSpec::FLAG_FILE},
				{TEXT("virtual"),			TunerSpec::FLAG_VIRTUAL},
				{TEXT("volatile"),			TunerSpec::FLAG_VOLATILE},
				{TEXT("no-enum-channel"),	TunerSpec::FLAG_NOENUMCHANNEL},
			};
			for (int i=0;i<lengthof(FlagList);i++) {
				for (auto itAttr=Attributes.begin();itAttr!=Attributes.end();++itAttr) {
					TVTest::StringUtility::Trim(*itAttr);
					if (TVTest::StringUtility::CompareNoCase(*itAttr,FlagList[i].pszName)==0) {
						Info.Spec.Flags|=FlagList[i].Flag;
						break;
					}
				}
			}
		}

		m_TunerSpecList.push_back(Info);
	}

	return true;
}


bool CDriverManager::GetTunerSpec(LPCTSTR pszTunerName,TunerSpec *pSpec) const
{
	if (IsStringEmpty(pszTunerName) || pSpec==nullptr)
		return false;

	LPCTSTR pszName=::PathFindFileName(pszTunerName);
	if (pszName[0]==_T('\0'))
		return false;

	for (auto it=m_TunerSpecList.begin();it!=m_TunerSpecList.end();++it) {
		if (::PathMatchSpec(pszName,it->TunerMask.c_str())) {
			*pSpec=it->Spec;
			return true;
		}
	}

	return false;
}
