#include "stdafx.h"
#include "TVTest.h"
#include "ChannelManager.h"
#include "Settings.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CChannelManager::CChannelManager()
{
	Reset();
}


CChannelManager::~CChannelManager()
{
}


void CChannelManager::Reset()
{
	m_CurrentSpace=SPACE_INVALID;
	m_CurrentChannel=-1;
	m_CurrentServiceID=-1;
	m_ChangingChannel=-1;

	m_TuningSpaceList.Clear();
	m_DriverTuningSpaceList.Clear();
	m_fUseDriverChannelList=false;
	m_fChannelFileHasStreamIDs=false;
	m_ChannelFileName.clear();
}


bool CChannelManager::LoadChannelList(LPCTSTR pszFileName)
{
	if (IsStringEmpty(pszFileName))
		return false;

	if (::PathMatchSpec(pszFileName,TEXT("*") CHANNEL_FILE_EXTENSION)
#ifdef DEFERRED_CHANNEL_FILE_EXTENSION
		|| ::PathMatchSpec(pszFileName,TEXT("*") DEFERRED_CHANNEL_FILE_EXTENSION)
#endif
		) {
		if (!m_TuningSpaceList.LoadFromFile(pszFileName))
			return false;

		// �Â��o�[�W�����̃`�����l���t�@�C����TSID��NID�������ꍇ������
		m_fChannelFileHasStreamIDs=true;
		for (int i=0;i<m_TuningSpaceList.NumSpaces();i++) {
			const CTuningSpaceInfo *pSpace=m_TuningSpaceList.GetTuningSpaceInfo(i);
			if (pSpace!=NULL) {
				const int NumChannels=pSpace->NumChannels();
				for (int j=0;j<NumChannels;j++) {
					const CChannelInfo *pChannel=pSpace->GetChannelInfo(j);
					if (pChannel!=NULL) {
						if (pChannel->GetServiceID()==0
								|| pChannel->GetNetworkID()==0
								|| pChannel->GetTransportStreamID()==0) {
							m_fChannelFileHasStreamIDs=false;
							goto Break;
						}
					}
				}
			}
		}
	Break:
		;
	} else {
		return false;
	}

	m_ChannelFileName=pszFileName;

	for (int i=0;i<m_DriverTuningSpaceList.NumSpaces();i++) {
		CTuningSpaceInfo *pTuningSpace=m_TuningSpaceList.GetTuningSpaceInfo(i);

		if (pTuningSpace!=NULL) {
			const CTuningSpaceInfo *pDriverTuningSpace=m_DriverTuningSpaceList.GetTuningSpaceInfo(i);
			const CChannelList *pDriverChannelList=pDriverTuningSpace->GetChannelList();

			pTuningSpace->SetName(pDriverTuningSpace->GetName());

			const int NumChannels=pTuningSpace->NumChannels();
			for (int j=0;j<NumChannels;j++) {
				CChannelInfo *pChInfo=pTuningSpace->GetChannelInfo(j);

				if (pChInfo->GetPhysicalChannel()==0) {
					int Channel=pDriverChannelList->GetPhysicalChannel(pChInfo->GetChannelIndex());

					if (Channel>0)
						pChInfo->SetPhysicalChannel(Channel);
				}
			}
		}
	}

	return true;
}


bool CChannelManager::SetTuningSpaceList(const CTuningSpaceList *pList)
{
	m_TuningSpaceList=*pList;
	return true;
}


bool CChannelManager::MakeDriverTuningSpaceList(const CBonSrcDecoder *pSrcDecoder)
{
	const int NumSpaces=pSrcDecoder->NumSpaces();

	m_DriverTuningSpaceList.Clear();
	if (!m_DriverTuningSpaceList.Reserve(NumSpaces))
		return false;

	for (int i=0;i<NumSpaces;i++) {
		CChannelList *pList=m_DriverTuningSpaceList.GetChannelList(i);
		LPCTSTR pszName;

		for (int j=0;(pszName=pSrcDecoder->GetChannelName(i,j))!=NULL;j++) {
			CChannelInfo ChInfo(i,j,j+1,pszName);

			LPCTSTR p=pszName;
			int Channel=0;
			while (*p!=_T('\0')) {
				if (*p>=_T('0') && *p<=_T('9')) {
					do {
						Channel=Channel*10+(*p-_T('0'));
						p++;
					} while (*p>=_T('0') && *p<=_T('9'));
					if (::StrCmpNI(p,TEXT("ch"),2)!=0)
						Channel=0;
				} else {
					p++;
				}
			}
			ChInfo.SetPhysicalChannel(Channel);

			pList->AddChannel(ChInfo);
		}
		m_DriverTuningSpaceList.GetTuningSpaceInfo(i)->SetName(pSrcDecoder->GetSpaceName(i));
	}

	m_DriverTuningSpaceList.MakeAllChannelList();

	return true;
}


bool CChannelManager::SetUseDriverChannelList(bool fUse)
{
	m_fUseDriverChannelList=fUse;
	return true;
}


bool CChannelManager::SetCurrentChannel(int Space,int Channel)
{
	if (Space!=SPACE_ALL && Space!=SPACE_INVALID) {
		if (Space<0 || Space>=NumSpaces())
			return false;
	}
	if (Space!=SPACE_INVALID) {
		const CChannelList *pList=GetChannelList(Space);
		if (pList==NULL || Channel<-1 || Channel>=pList->NumChannels())
			return false;
	} else {
		Channel=-1;
	}
	m_CurrentSpace=Space;
	m_CurrentChannel=Channel;
	return true;
}


bool CChannelManager::SetCurrentServiceID(int ServiceID)
{
	m_CurrentServiceID=ServiceID;
	return true;
}


bool CChannelManager::SetChangingChannel(int Channel)
{
	m_ChangingChannel=Channel;
	return true;
}


const CChannelInfo *CChannelManager::GetCurrentChannelInfo() const
{
	const CChannelList *pList=GetCurrentChannelList();

	if (pList==NULL)
		return NULL;
	return pList->GetChannelInfo(m_CurrentChannel);
}


const CChannelInfo *CChannelManager::GetChangingChannelInfo() const
{
	const CChannelList *pList=GetCurrentChannelList();

	if (pList!=NULL)
		return pList->GetChannelInfo(m_ChangingChannel);
	return NULL;
}


int CChannelManager::GetNextChannel(bool fNext) const
{
	const CChannelList *pList=GetCurrentChannelList();
	if (pList==NULL)
		return -1;

	int Channel=-1;

	if (m_ChangingChannel>=0)
		Channel=m_ChangingChannel;

	if (Channel<0) {
		if (m_CurrentChannel<0)
			return -1;
		Channel=m_CurrentChannel;
	}
	//if (pList->HasRemoteControlKeyID()) {
	if (pList->GetChannelNo(Channel)>0) {
		if (fNext)
			Channel=pList->GetNextChannel(Channel,true);
		else
			Channel=pList->GetPrevChannel(Channel,true);
	} else {
		int i;

		for (i=pList->NumChannels();i>0;i--) {
			if (fNext) {
				Channel++;
				if (Channel>=pList->NumChannels())
					Channel=0;
			} else {
				Channel--;
				if (Channel<0)
					Channel=pList->NumChannels()-1;
			}
			if (pList->IsEnabled(Channel))
				break;
		}
		if (i==0)
			return -1;
	}

	return Channel;
}


const CChannelInfo *CChannelManager::GetNextChannelInfo(bool fNext) const
{
	int Channel=GetNextChannel(fNext);
	if (Channel<0)
		return NULL;
	return GetCurrentChannelList()->GetChannelInfo(Channel);
}


const CChannelList *CChannelManager::GetCurrentChannelList() const
{
	return GetChannelList(m_CurrentSpace);
}


const CChannelList *CChannelManager::GetChannelList(int Space) const
{
	if (Space==SPACE_ALL)
		return GetAllChannelList();
	if (!m_fUseDriverChannelList) {
		const CChannelList *pList=m_TuningSpaceList.GetChannelList(Space);
		if (pList!=NULL && pList->NumChannels()>0)
			return pList;
	}
	return m_DriverTuningSpaceList.GetChannelList(Space);
}


const CChannelList *CChannelManager::GetFileChannelList(int Space) const
{
	if (Space==SPACE_ALL)
		return m_TuningSpaceList.GetAllChannelList();
	return m_TuningSpaceList.GetChannelList(Space);
}


const CChannelList *CChannelManager::GetDriverChannelList(int Space) const
{
	if (Space==SPACE_ALL)
		return m_DriverTuningSpaceList.GetAllChannelList();
	return m_DriverTuningSpaceList.GetChannelList(Space);
}


const CChannelList *CChannelManager::GetAllChannelList() const
{
	/*
	CChannelList *pList=m_TuningSpaceList.GetAllChannelList();

	if (pList->NumChannels()>0)
		return pList;
	return m_DriverTuningSpaceList.GetAllChannelList();
	*/
	if (m_fUseDriverChannelList)
		return m_DriverTuningSpaceList.GetAllChannelList();
	return m_TuningSpaceList.GetAllChannelList();
}


const CChannelList *CChannelManager::GetFileAllChannelList() const
{
	return m_TuningSpaceList.GetAllChannelList();
}


const CChannelList *CChannelManager::GetDriverAllChannelList() const
{
	return m_DriverTuningSpaceList.GetAllChannelList();
}


LPCTSTR CChannelManager::GetTuningSpaceName(int Space) const
{
	return m_DriverTuningSpaceList.GetTuningSpaceName(Space);
}


int CChannelManager::FindChannelInfo(const CChannelInfo *pInfo) const
{
	const CChannelList *pList=GetCurrentChannelList();
	int i;

	if (pList==NULL)
		return -1;
	for (i=0;i<pList->NumChannels();i++) {
		if (pList->GetChannelInfo(i)==pInfo)
			return i;
	}
	return -1;
}


int CChannelManager::FindChannelByIDs(int Space,WORD NetworkID,WORD TransportStreamID,WORD ServiceID,
									  bool fEnabledOnly) const
{
	const CChannelList *pChannelList;

	if (Space==SPACE_ALL)
		pChannelList=m_TuningSpaceList.GetAllChannelList();
	else
		pChannelList=m_TuningSpaceList.GetChannelList(Space);
	if (pChannelList==nullptr)
		return -1;

	return pChannelList->FindByIDs(NetworkID,TransportStreamID,ServiceID,fEnabledOnly);
}


int CChannelManager::NumSpaces() const
{
	if (m_fUseDriverChannelList)
		return m_DriverTuningSpaceList.NumSpaces();
	return max(m_TuningSpaceList.NumSpaces(),m_DriverTuningSpaceList.NumSpaces());
}


bool CChannelManager::GetChannelFileName(LPTSTR pszFileName,int MaxLength) const
{
	if (pszFileName==NULL
			|| m_ChannelFileName.empty()
			|| MaxLength<=(int)m_ChannelFileName.length())
		return false;

	::lstrcpy(pszFileName,m_ChannelFileName.c_str());

	return true;
}




CChannelSpec::CChannelSpec()
	: m_Space(CChannelManager::SPACE_INVALID)
	, m_Channel(-1)
	, m_ServiceID(-1)
{
}


CChannelSpec::~CChannelSpec()
{
}


bool CChannelSpec::Store(const CChannelManager *pChannelManager)
{
	m_Space=pChannelManager->GetCurrentSpace();
	m_Channel=pChannelManager->GetCurrentChannel();
	m_ServiceID=pChannelManager->GetCurrentServiceID();
	return true;
}


bool CChannelSpec::SetSpace(int Space)
{
	m_Space=Space;
	return true;
}


bool CChannelSpec::SetChannel(int Channel)
{
	m_Channel=Channel;
	return true;
}


bool CChannelSpec::SetServiceID(int ServiceID)
{
	m_ServiceID=ServiceID;
	return true;
}
