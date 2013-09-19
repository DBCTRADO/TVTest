#include "stdafx.h"
#include "TVTest.h"
#include "ChannelHistory.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CChannelHistory::CChannelHistory()
	: m_MaxChannelHistory(20)
	, m_CurrentChannel(-1)
{
}


CChannelHistory::~CChannelHistory()
{
	Clear();
}


void CChannelHistory::Clear()
{
	for (size_t i=0;i<m_ChannelList.size();i++)
		delete m_ChannelList[i];
	m_ChannelList.clear();
	m_CurrentChannel=-1;
}


bool CChannelHistory::SetCurrentChannel(LPCTSTR pszDriverName,const CChannelInfo *pChannelInfo)
{
	if (pszDriverName==NULL || pChannelInfo==NULL)
		return false;

	if (m_CurrentChannel>=0) {
		const CChannel *pCurChannel=m_ChannelList[m_CurrentChannel];

		if (IsEqualFileName(pCurChannel->GetDriverFileName(),pszDriverName)
				&& pCurChannel->GetChannelIndex()==pChannelInfo->GetChannelIndex()
				&& pCurChannel->GetNetworkID()==pChannelInfo->GetNetworkID()
				&& pCurChannel->GetTransportStreamID()==pChannelInfo->GetTransportStreamID()
				&& pCurChannel->GetServiceID()==pChannelInfo->GetServiceID())
			return true;
	}

	while ((int)m_ChannelList.size()-1>m_CurrentChannel) {
		delete m_ChannelList.back();
		m_ChannelList.pop_back();
	}
	m_ChannelList.push_back(new CChannel(pszDriverName,pChannelInfo));
	m_CurrentChannel++;
	if ((int)m_ChannelList.size()>m_MaxChannelHistory) {
		delete m_ChannelList.front();
		m_ChannelList.pop_front();
		m_CurrentChannel--;
	}
	return true;
}


const CChannelHistory::CChannel *CChannelHistory::Forward()
{
	if (m_CurrentChannel+1>=(int)m_ChannelList.size())
		return NULL;
	return m_ChannelList[++m_CurrentChannel];
}


const CChannelHistory::CChannel *CChannelHistory::Backward()
{
	if (m_CurrentChannel<1)
		return NULL;
	return m_ChannelList[--m_CurrentChannel];
}


CChannelHistory::CChannel::CChannel(LPCTSTR pszDriverName,const CChannelInfo *pChannelInfo)
	: CChannelInfo(*pChannelInfo)
	, m_DriverName(pszDriverName)
{
}




CRecentChannelList::CRecentChannelList()
	: CSettingsBase(TEXT("RecentChannel"))
	, m_MaxChannelHistory(20)
	, m_MaxChannelHistoryMenu(20)
{
}


CRecentChannelList::~CRecentChannelList()
{
	Clear();
}


int CRecentChannelList::NumChannels() const
{
	return (int)m_ChannelList.size();
}


void CRecentChannelList::Clear()
{
	for (size_t i=0;i<m_ChannelList.size();i++)
		delete m_ChannelList[i];
	m_ChannelList.clear();
}


const CRecentChannelList::CChannel *CRecentChannelList::GetChannelInfo(int Index) const
{
	if (Index<0 || Index>=NumChannels())
		return NULL;
	return m_ChannelList[Index];
}


bool CRecentChannelList::Add(LPCTSTR pszDriverName,const CChannelInfo *pChannelInfo)
{
	if (pszDriverName==NULL || pChannelInfo==NULL)
		return false;

	std::deque<CChannel*>::iterator itr;
	for (itr=m_ChannelList.begin();itr!=m_ChannelList.end();++itr) {
		if (IsEqualFileName((*itr)->GetDriverFileName(),pszDriverName)
				&& (*itr)->GetSpace()==pChannelInfo->GetSpace()
				&& (*itr)->GetChannelIndex()==pChannelInfo->GetChannelIndex()
				&& (*itr)->GetServiceID()==pChannelInfo->GetServiceID()) {
			if (itr==m_ChannelList.begin()
					&& (*itr)->GetNetworkID()==pChannelInfo->GetNetworkID())
				return true;
			delete *itr;
			m_ChannelList.erase(itr);
			break;
		}
	}
	m_ChannelList.push_front(new CChannel(pszDriverName,pChannelInfo));
	if ((int)m_ChannelList.size()>m_MaxChannelHistory) {
		delete m_ChannelList.back();
		m_ChannelList.pop_back();
	}
	return true;
}


bool CRecentChannelList::SetMenu(HMENU hmenu,bool fClear) const
{
	ClearMenu(hmenu);
	for (int i=0;i<m_MaxChannelHistoryMenu;i++) {
		const CChannel *pChannelInfo=GetChannelInfo(i);
		if (pChannelInfo==NULL)
			break;

		TCHAR szText[64];
		int Length=0;
		if (i<36)
			Length=::wsprintf(szText,TEXT("&%c: "),i<10?i+_T('0'):(i-10)+_T('A'));
		CopyToMenuText(pChannelInfo->GetName(),
					   szText+Length,lengthof(szText)-Length);
		::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_CHANNELHISTORY_FIRST+i,szText);
	}
	if (fClear && NumChannels()>0) {
		::AppendMenu(hmenu,MFT_SEPARATOR,0,NULL);
		::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_CHANNELHISTORY_CLEAR,TEXT("—š—ð‚ðƒNƒŠƒA"));
	}
	return true;
}


bool CRecentChannelList::ReadSettings(CSettings &Settings)
{
	m_ChannelList.clear();

	int Count;

	if (Settings.Read(TEXT("Count"),&Count) && Count>0) {
		if (Count>m_MaxChannelHistory)
			Count=m_MaxChannelHistory;
		for (int i=0;i<Count;i++) {
			TCHAR szName[32],szDriverName[MAX_PATH],szChannelName[MAX_CHANNEL_NAME];
			int Space,Channel,ServiceID,NetworkID;

			::wsprintf(szName,TEXT("History%d_Driver"),i);
			if (!Settings.Read(szName,szDriverName,lengthof(szDriverName))
					|| szDriverName[0]=='\0')
				break;
			::wsprintf(szName,TEXT("History%d_Name"),i);
			if (!Settings.Read(szName,szChannelName,lengthof(szChannelName))
					|| szChannelName[0]=='\0')
				break;
			::wsprintf(szName,TEXT("History%d_Space"),i);
			if (!Settings.Read(szName,&Space))
				break;
			::wsprintf(szName,TEXT("History%d_Channel"),i);
			if (!Settings.Read(szName,&Channel))
				break;
			::wsprintf(szName,TEXT("History%d_ServiceID"),i);
			if (!Settings.Read(szName,&ServiceID))
				break;
			::wsprintf(szName,TEXT("History%d_NetworkID"),i);
			if (!Settings.Read(szName,&NetworkID))
				NetworkID=0;
			CChannelInfo ChannelInfo(Space,Channel,0,szChannelName);
			ChannelInfo.SetServiceID(ServiceID);
			ChannelInfo.SetNetworkID(NetworkID);
			m_ChannelList.push_back(new CChannel(szDriverName,&ChannelInfo));
		}
	}
	return true;
}


bool CRecentChannelList::WriteSettings(CSettings &Settings)
{
	const int Channels=NumChannels();

	Settings.Clear();
	Settings.Write(TEXT("Count"),Channels);
	for (int i=0;i<Channels;i++) {
		const CChannel *pChannelInfo=m_ChannelList[i];
		TCHAR szName[64];

		::wsprintf(szName,TEXT("History%d_Driver"),i);
		Settings.Write(szName,pChannelInfo->GetDriverFileName());
		::wsprintf(szName,TEXT("History%d_Name"),i);
		Settings.Write(szName,pChannelInfo->GetName());
		::wsprintf(szName,TEXT("History%d_Space"),i);
		Settings.Write(szName,pChannelInfo->GetSpace());
		::wsprintf(szName,TEXT("History%d_Channel"),i);
		Settings.Write(szName,pChannelInfo->GetChannelIndex());
		::wsprintf(szName,TEXT("History%d_ServiceID"),i);
		Settings.Write(szName,pChannelInfo->GetServiceID());
		::wsprintf(szName,TEXT("History%d_NetworkID"),i);
		Settings.Write(szName,pChannelInfo->GetNetworkID());
	}
	return true;
}


CRecentChannelList::CChannel::CChannel(LPCTSTR pszDriverName,const CChannelInfo *pChannelInfo)
	: CChannelInfo(*pChannelInfo)
	, m_DriverName(pszDriverName)
{
}
