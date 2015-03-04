#include "stdafx.h"
#include <algorithm>
#include "TVTest.h"
#include "ChannelList.h"
#include "HelperClass/StdUtil.h"
#include "Common/DebugDef.h"




CChannelInfo::CChannelInfo()
	: m_Space(-1)
	, m_ChannelIndex(-1)
	, m_ChannelNo(0)
	, m_PhysicalChannel(0)
	, m_NetworkID(0)
	, m_TransportStreamID(0)
	, m_ServiceID(0)
	, m_ServiceType(0)
	, m_fEnabled(true)
{
}


CChannelInfo::CChannelInfo(int Space,int ChannelIndex,int No,LPCTSTR pszName)
	: m_Space(Space)
	, m_ChannelIndex(ChannelIndex)
	, m_ChannelNo(No)
	, m_PhysicalChannel(0)
	, m_NetworkID(0)
	, m_TransportStreamID(0)
	, m_ServiceID(0)
	, m_ServiceType(0)
	, m_fEnabled(true)
{
	if (pszName!=NULL)
		m_Name=pszName;
}


bool CChannelInfo::SetSpace(int Space)
{
	m_Space=Space;
	return true;
}


bool CChannelInfo::SetChannelIndex(int Channel)
{
	m_ChannelIndex=Channel;
	return true;
}


bool CChannelInfo::SetChannelNo(int ChannelNo)
{
	if (ChannelNo<0)
		return false;
	m_ChannelNo=ChannelNo;
	return true;
}


bool CChannelInfo::SetPhysicalChannel(int Channel)
{
	m_PhysicalChannel=Channel;
	return true;
}


bool CChannelInfo::SetName(LPCTSTR pszName)
{
	TVTest::StringUtility::Assign(m_Name,pszName);
	return true;
}


void CChannelInfo::SetNetworkID(WORD NetworkID)
{
	m_NetworkID=NetworkID;
}


void CChannelInfo::SetTransportStreamID(WORD TransportStreamID)
{
	m_TransportStreamID=TransportStreamID;
}


void CChannelInfo::SetServiceID(WORD ServiceID)
{
	m_ServiceID=ServiceID;
}


void CChannelInfo::SetServiceType(BYTE ServiceType)
{
	m_ServiceType=ServiceType;
}




CTunerChannelInfo::CTunerChannelInfo()
{
}


CTunerChannelInfo::CTunerChannelInfo(const CChannelInfo &ChannelInfo,LPCTSTR pszTunerName)
	: CChannelInfo(ChannelInfo)
{
	SetTunerName(pszTunerName);
}


void CTunerChannelInfo::SetTunerName(LPCTSTR pszName)
{
	TVTest::StringUtility::Assign(m_TunerName,pszName);
}




CChannelList::CChannelList()
{
}


CChannelList::CChannelList(const CChannelList &Src)
{
	*this=Src;
}


CChannelList::~CChannelList()
{
	Clear();
}


CChannelList &CChannelList::operator=(const CChannelList &Src)
{
	if (&Src!=this) {
		Clear();

		if (!Src.m_ChannelList.empty()) {
			m_ChannelList.reserve(Src.m_ChannelList.size());
			for (auto i=Src.m_ChannelList.begin();i!=Src.m_ChannelList.end();i++) {
				m_ChannelList.push_back(new CChannelInfo(**i));
			}
		}
	}
	return *this;
}


int CChannelList::NumEnableChannels() const
{
	int Count=0;

	for (auto i=m_ChannelList.begin();i!=m_ChannelList.end();i++) {
		if ((*i)->IsEnabled())
			Count++;
	}
	return Count;
}


bool CChannelList::AddChannel(const CChannelInfo &Info)
{
	return AddChannel(new CChannelInfo(Info));
}


bool CChannelList::AddChannel(CChannelInfo *pInfo)
{
	if (pInfo==NULL)
		return false;
	m_ChannelList.push_back(pInfo);
	return true;
}


bool CChannelList::InsertChannel(int Index,const CChannelInfo &Info)
{
	if (Index<0 || (size_t)Index>m_ChannelList.size()) {
		TRACE(TEXT("CChannelList::InsertChannel() : Out of range [%d]\n"),Index);
		return false;
	}

	auto itr=m_ChannelList.begin();
	if (Index>0)
		std::advance(itr,Index);
	m_ChannelList.insert(itr,new CChannelInfo(Info));

	return true;
}


CChannelInfo *CChannelList::GetChannelInfo(int Index)
{
	if (Index<0 || (size_t)Index>=m_ChannelList.size()) {
		//TRACE(TEXT("CChannelList::GetChannelInfo Out of range %d\n"),Index);
		return NULL;
	}
	return m_ChannelList[Index];
}


const CChannelInfo *CChannelList::GetChannelInfo(int Index) const
{
	if (Index<0 || (size_t)Index>=m_ChannelList.size()) {
		//TRACE(TEXT("CChannelList::GetChannelInfo Out of range %d\n"),Index);
		return NULL;
	}
	return m_ChannelList[Index];
}


int CChannelList::GetSpace(int Index) const
{
	if (Index<0 || (size_t)Index>=m_ChannelList.size())
		return -1;
	return m_ChannelList[Index]->GetSpace();
}


int CChannelList::GetChannelIndex(int Index) const
{
	if (Index<0 || (size_t)Index>=m_ChannelList.size())
		return -1;
	return m_ChannelList[Index]->GetChannelIndex();
}


int CChannelList::GetChannelNo(int Index) const
{
	if (Index<0 || (size_t)Index>=m_ChannelList.size())
		return -1;
	return m_ChannelList[Index]->GetChannelNo();
}


int CChannelList::GetPhysicalChannel(int Index) const
{
	if (Index<0 || (size_t)Index>=m_ChannelList.size())
		return -1;
	return m_ChannelList[Index]->GetPhysicalChannel();
}


LPCTSTR CChannelList::GetName(int Index) const
{
	if (Index<0 || (size_t)Index>=m_ChannelList.size())
		//return NULL;
		return TEXT("");
	return m_ChannelList[Index]->GetName();
}


bool CChannelList::IsEnabled(int Index) const
{
	if (Index<0 || (size_t)Index>=m_ChannelList.size())
		return false;
	return m_ChannelList[Index]->IsEnabled();
}


bool CChannelList::DeleteChannel(int Index)
{
	if (Index<0 || (size_t)Index>=m_ChannelList.size())
		return false;

	auto itr=m_ChannelList.begin();
	if (Index>0)
		std::advance(itr,Index);
	delete *itr;
	m_ChannelList.erase(itr);

	return true;
}


void CChannelList::Clear()
{
	for (auto i=m_ChannelList.begin();i!=m_ChannelList.end();i++)
		delete *i;
	m_ChannelList.clear();
}


int CChannelList::Find(const CChannelInfo *pInfo) const
{
	for (size_t i=0;i<m_ChannelList.size();i++) {
		if (m_ChannelList[i]==pInfo)
			return (int)i;
	}
	return -1;
}


int CChannelList::FindByIndex(int Space,int ChannelIndex,int ServiceID,bool fEnabledOnly) const
{
	for (size_t i=0;i<m_ChannelList.size();i++) {
		const CChannelInfo *pChInfo=m_ChannelList[i];

		if ((!fEnabledOnly || pChInfo->IsEnabled())
				&& (Space<0 || pChInfo->GetSpace()==Space)
				&& (ChannelIndex<0 || pChInfo->GetChannelIndex()==ChannelIndex)
				&& (ServiceID<=0 || pChInfo->GetServiceID()==ServiceID))
			return (int)i;
	}
	return -1;
}


int CChannelList::FindPhysicalChannel(int Channel) const
{
	for (size_t i=0;i<m_ChannelList.size();i++) {
		if (m_ChannelList[i]->GetPhysicalChannel()==Channel)
			return (int)i;
	}
	return -1;
}


int CChannelList::FindChannelNo(int No,bool fEnabledOnly) const
{
	for (size_t i=0;i<m_ChannelList.size();i++) {
		const CChannelInfo *pChInfo=m_ChannelList[i];
		if (pChInfo->GetChannelNo()==No
				&& (!fEnabledOnly || pChInfo->IsEnabled()))
			return (int)i;
	}
	return -1;
}


int CChannelList::FindServiceID(WORD ServiceID) const
{
	for (size_t i=0;i<m_ChannelList.size();i++) {
		if (m_ChannelList[i]->GetServiceID()==ServiceID)
			return (int)i;
	}
	return -1;
}


int CChannelList::FindByIDs(WORD NetworkID,WORD TransportStreamID,WORD ServiceID,bool fEnabledOnly) const
{
	for (size_t i=0;i<m_ChannelList.size();i++) {
		const CChannelInfo *pChannelInfo=m_ChannelList[i];
		if ((!fEnabledOnly || pChannelInfo->IsEnabled())
				&& (NetworkID==0 || pChannelInfo->GetNetworkID()==NetworkID)
				&& (TransportStreamID==0 || pChannelInfo->GetTransportStreamID()==TransportStreamID)
				&& (ServiceID==0 || pChannelInfo->GetServiceID()==ServiceID))
			return (int)i;
	}
	return -1;
}


int CChannelList::FindByName(LPCTSTR pszName) const
{
	if (pszName==NULL)
		return -1;
	for (size_t i=0;i<m_ChannelList.size();i++) {
		if (::lstrcmp(m_ChannelList[i]->GetName(),pszName)==0)
			return (int)i;
	}
	return -1;
}


int CChannelList::GetNextChannel(int Index,bool fWrap) const
{
	if (Index<0 || (size_t)Index>=m_ChannelList.size())
		return -1;

	const int ChannelNo=GetChannelNo(Index);
	int Channel,Min,No;

	Channel=INT_MAX;
	Min=INT_MAX;
	for (auto i=m_ChannelList.begin();i!=m_ChannelList.end();++i) {
		const CChannelInfo *pChInfo=*i;

		if (pChInfo->IsEnabled()) {
			No=pChInfo->GetChannelNo();
			if (No!=0) {
				if (No>ChannelNo && No<Channel)
					Channel=No;
				if (No<Min)
					Min=No;
			}
		}
	}
	if (Channel==INT_MAX) {
		if (Min==INT_MAX || !fWrap)
			return -1;
		return FindChannelNo(Min);
	}
	return FindChannelNo(Channel);
}


int CChannelList::GetPrevChannel(int Index,bool fWrap) const
{
	if (Index<0 || (size_t)Index>=m_ChannelList.size())
		return -1;

	const int ChannelNo=GetChannelNo(Index);
	int Channel,Max,No;

	Channel=0;
	Max=0;
	for (auto i=m_ChannelList.begin();i!=m_ChannelList.end();++i) {
		const CChannelInfo *pChInfo=*i;

		if (pChInfo->IsEnabled()) {
			No=pChInfo->GetChannelNo();
			if (No!=0) {
				if (No<ChannelNo && No>Channel)
					Channel=No;
				if (No>Max)
					Max=No;
			}
		}
	}
	if (Channel==0) {
		if (fWrap)
			return FindChannelNo(Max);
		return -1;
	}
	return FindChannelNo(Channel);
}


int CChannelList::GetMaxChannelNo() const
{
	int Max,No;

	Max=0;
	for (auto i=m_ChannelList.begin();i!=m_ChannelList.end();i++) {
		No=(*i)->GetChannelNo();
		if (No>Max)
			Max=No;
	}
	return Max;
}


bool CChannelList::Sort(SortType Type,bool fDescending)
{
	if (Type<0 || Type>=SORT_TRAILER)
		return false;

	if (m_ChannelList.size()>1) {
		class CPredicator
		{
			SortType m_Type;
			bool m_fDescending;

		public:
			CPredicator(SortType Type,bool fDescending)
				: m_Type(Type)
				, m_fDescending(fDescending)
			{
			}

			bool operator()(const CChannelInfo *pChannel1,const CChannelInfo *pChannel2)
			{
				int Cmp;

				switch (m_Type) {
				case SORT_SPACE:
					Cmp=pChannel1->GetSpace()-pChannel2->GetSpace();
					break;
				case SORT_CHANNELINDEX:
					Cmp=pChannel1->GetChannelIndex()-pChannel2->GetChannelIndex();
					break;
				case SORT_CHANNELNO:
					Cmp=pChannel1->GetChannelNo()-pChannel2->GetChannelNo();
					break;
				case SORT_PHYSICALCHANNEL:
					Cmp=pChannel1->GetPhysicalChannel()-pChannel2->GetPhysicalChannel();
					break;
				case SORT_NAME:
					Cmp=::lstrcmpi(pChannel1->GetName(),pChannel2->GetName());
					if (Cmp==0)
						Cmp=::lstrcmp(pChannel1->GetName(),pChannel2->GetName());
					break;
				case SORT_NETWORKID:
					Cmp=pChannel1->GetNetworkID()-pChannel2->GetNetworkID();
					break;
				case SORT_SERVICEID:
					Cmp=pChannel1->GetServiceID()-pChannel2->GetServiceID();
					break;
				default:
					__assume(0);
				}

				return m_fDescending?Cmp>0:Cmp<0;
			}
		};

		std::stable_sort(m_ChannelList.begin(),m_ChannelList.end(),
						 CPredicator(Type,fDescending));
	}

	return true;
}


bool CChannelList::HasRemoteControlKeyID() const
{
	for (auto i=m_ChannelList.begin();i!=m_ChannelList.end();i++) {
		if ((*i)->GetChannelNo()!=0)
			return true;
	}
	return false;
}


bool CChannelList::HasMultiService() const
{
	for (size_t i=0;i+1<m_ChannelList.size();i++) {
		const CChannelInfo *pChannelInfo1=m_ChannelList[i];

		for (size_t j=i+1;j<m_ChannelList.size();j++) {
			const CChannelInfo *pChannelInfo2=m_ChannelList[j];

			if (pChannelInfo1->GetNetworkID()==pChannelInfo2->GetNetworkID()
					&& pChannelInfo1->GetTransportStreamID()==pChannelInfo2->GetTransportStreamID()
					&& pChannelInfo1->GetServiceID()!=pChannelInfo2->GetServiceID())
				return true;
		}
	}
	return false;
}




CTuningSpaceInfo::CTuningSpaceInfo()
	: m_pChannelList(NULL)
	, m_Space(SPACE_UNKNOWN)
{
}


CTuningSpaceInfo::CTuningSpaceInfo(const CTuningSpaceInfo &Info)
	: m_pChannelList(NULL)
	, m_Space(SPACE_UNKNOWN)
{
	Create(Info.m_pChannelList,Info.m_Name.c_str());
}


CTuningSpaceInfo::~CTuningSpaceInfo()
{
	delete m_pChannelList;
}


CTuningSpaceInfo &CTuningSpaceInfo::operator=(const CTuningSpaceInfo &Info)
{
	if (&Info!=this)
		Create(Info.m_pChannelList,Info.m_Name.c_str());
	return *this;
}


bool CTuningSpaceInfo::Create(const CChannelList *pList,LPCTSTR pszName)
{
	delete m_pChannelList;
	if (pList!=NULL)
		m_pChannelList=new CChannelList(*pList);
	else
		m_pChannelList=new CChannelList;
	SetName(pszName);
	return true;
}


const CChannelInfo *CTuningSpaceInfo::GetChannelInfo(int Index) const
{
	if (m_pChannelList==NULL)
		return NULL;
	return m_pChannelList->GetChannelInfo(Index);
}


CChannelInfo *CTuningSpaceInfo::GetChannelInfo(int Index)
{
	if (m_pChannelList==NULL)
		return NULL;
	return m_pChannelList->GetChannelInfo(Index);
}


bool CTuningSpaceInfo::SetName(LPCTSTR pszName)
{
	// チューニング空間の種類を判定する
	// BonDriverから取得できないので苦肉の策
	m_Space=SPACE_UNKNOWN;
	if (!IsStringEmpty(pszName)) {
		m_Name=pszName;
		if (::StrStr(pszName,TEXT("地"))!=NULL
				|| ::StrStrI(pszName,TEXT("VHF"))!=NULL
				|| ::StrStrI(pszName,TEXT("UHF"))!=NULL
				|| ::StrStrI(pszName,TEXT("CATV"))!=NULL) {
			m_Space=SPACE_TERRESTRIAL;
		} else if (::StrStrI(pszName,TEXT("BS"))!=NULL) {
			m_Space=SPACE_BS;
		} else if (::StrStrI(pszName,TEXT("CS"))!=NULL) {
			m_Space=SPACE_110CS;
		}
	} else {
		m_Name.clear();
	}
	return true;
}


int CTuningSpaceInfo::NumChannels() const
{
	if (m_pChannelList==NULL)
		return 0;
	return m_pChannelList->NumChannels();
}




CTuningSpaceList::CTuningSpaceList()
{
}


CTuningSpaceList::CTuningSpaceList(const CTuningSpaceList &List)
{
	*this=List;
}


CTuningSpaceList::~CTuningSpaceList()
{
	Clear();
}


CTuningSpaceList &CTuningSpaceList::operator=(const CTuningSpaceList &List)
{
	if (&List!=this) {
		Clear();
		if (List.NumSpaces()>0) {
			m_TuningSpaceList.resize(List.m_TuningSpaceList.size());
			for (size_t i=0;i<List.m_TuningSpaceList.size();i++)
				m_TuningSpaceList[i]=new CTuningSpaceInfo(*List.m_TuningSpaceList[i]);
		}
		m_AllChannelList=List.m_AllChannelList;
	}
	return *this;
}


CTuningSpaceInfo *CTuningSpaceList::GetTuningSpaceInfo(int Space)
{
	if (Space<0 || Space>=NumSpaces())
		return NULL;
	return m_TuningSpaceList[Space];
}


const CTuningSpaceInfo *CTuningSpaceList::GetTuningSpaceInfo(int Space) const
{
	if (Space<0 || Space>=NumSpaces())
		return NULL;
	return m_TuningSpaceList[Space];
}


CChannelList *CTuningSpaceList::GetChannelList(int Space)
{
	if (Space<0 || Space>=NumSpaces())
		return NULL;
	return m_TuningSpaceList[Space]->GetChannelList();
}


const CChannelList *CTuningSpaceList::GetChannelList(int Space) const
{
	if (Space<0 || Space>=NumSpaces())
		return NULL;
	return m_TuningSpaceList[Space]->GetChannelList();
}


LPCTSTR CTuningSpaceList::GetTuningSpaceName(int Space) const
{
	if (Space<0 || Space>=NumSpaces())
		return NULL;
	return m_TuningSpaceList[Space]->GetName();
}


CTuningSpaceInfo::TuningSpaceType CTuningSpaceList::GetTuningSpaceType(int Space) const
{
	if (Space<0 || Space>=NumSpaces()) {
		return CTuningSpaceInfo::SPACE_ERROR;
	}
	return m_TuningSpaceList[Space]->GetType();
}


bool CTuningSpaceList::MakeTuningSpaceList(const CChannelList *pList,int Spaces)
{
	int i;
	int Space;

	for (i=0;i<pList->NumChannels();i++) {
		Space=pList->GetSpace(i);
		if (Space+1>Spaces)
			Spaces=Space+1;
	}
	if (Spaces<1)
		return false;
	if (!Reserve(Spaces))
		return false;
	for (i=0;i<pList->NumChannels();i++) {
		const CChannelInfo *pChInfo=pList->GetChannelInfo(i);

		m_TuningSpaceList[pChInfo->GetSpace()]->GetChannelList()->AddChannel(*pChInfo);
	}
	return true;
}


bool CTuningSpaceList::Create(const CChannelList *pList,int Spaces)
{
	Clear();
	if (!MakeTuningSpaceList(pList,Spaces))
		return false;
	m_AllChannelList=*pList;
	return true;
}


bool CTuningSpaceList::Reserve(int Spaces)
{
	int i;

	if (Spaces<0)
		return false;
	if (Spaces==NumSpaces())
		return true;
	if (Spaces==0) {
		Clear();
		return true;
	}
	if (Spaces<NumSpaces()) {
		for (i=NumSpaces()-1;i>=Spaces;i--)
			delete m_TuningSpaceList[i];
		m_TuningSpaceList.resize(Spaces);
	} else {
		for (i=NumSpaces();i<Spaces;i++) {
			CTuningSpaceInfo *pInfo=new CTuningSpaceInfo;

			pInfo->Create();
			m_TuningSpaceList.push_back(pInfo);
		}
	}
	return true;
}


void CTuningSpaceList::Clear()
{
	for (size_t i=0;i<m_TuningSpaceList.size();i++)
		delete m_TuningSpaceList[i];
	m_TuningSpaceList.clear();
	m_AllChannelList.Clear();
}


bool CTuningSpaceList::MakeAllChannelList()
{
	m_AllChannelList.Clear();
	for (int i=0;i<NumSpaces();i++) {
		CChannelList *pList=m_TuningSpaceList[i]->GetChannelList();

		for (int j=0;j<pList->NumChannels();j++) {
			m_AllChannelList.AddChannel(*pList->GetChannelInfo(j));
		}
	}
	return true;
}


static const UINT CP_SHIFT_JIS=932;

bool CTuningSpaceList::SaveToFile(LPCTSTR pszFileName) const
{
	TRACE(TEXT("CTuningSpaceList::SaveToFile() : \"%s\"\n"),pszFileName);

	TVTest::String Buffer;

	Buffer=
		TEXT("; ") APP_NAME TEXT(" チャンネル設定ファイル\r\n")
		TEXT("; 名称,チューニング空間,チャンネル,リモコン番号,サービスタイプ,サービスID,ネットワークID,TSID,状態\r\n");

	for (int i=0;i<NumSpaces();i++) {
		const CChannelList *pChannelList=m_TuningSpaceList[i]->GetChannelList();
		TCHAR szText[MAX_CHANNEL_NAME+256];

		if (pChannelList->NumChannels()==0)
			continue;

		if (GetTuningSpaceName(i)!=NULL) {
			StdUtil::snprintf(szText,lengthof(szText),
							  TEXT(";#SPACE(%d,%s)\r\n"),i,GetTuningSpaceName(i));
			Buffer+=szText;
		}

		for (int j=0;j<pChannelList->NumChannels();j++) {
			const CChannelInfo *pChInfo=pChannelList->GetChannelInfo(j);
			LPCTSTR pszName=pChInfo->GetName();
			TVTest::String Name;

			// 必要に応じて " で囲む
			if (pszName[0]==_T('#') || pszName[0]==_T(';')
					|| ::StrChr(pszName,_T(','))!=NULL
					|| ::StrChr(pszName,_T('"'))!=NULL) {
				LPCTSTR p=pszName;
				Name=_T('"');
				while (*p!=_T('\0')) {
					if (*p==_T('"')) {
						Name+=TEXT("\"\"");
						p++;
					} else {
#ifdef UNICODE
						int SrcLength;
						for (SrcLength=1;p[SrcLength]!=L'"' && p[SrcLength]!=L'\0';SrcLength++);
						Name.append(p,SrcLength);
						p+=SrcLength;
#else
						if (::IsDBCSLeadByteEx(CP_ACP,*p)) {
							if (*(p+1)==_T('\0'))
								break;
							Name+=*p++;
						}
						Name+=*p++;
#endif
					}
				}
				Name+=_T('"');
			}

			StdUtil::snprintf(szText,lengthof(szText),TEXT("%s,%d,%d,%d,"),
				Name.empty()?pszName:Name.c_str(),
				pChInfo->GetSpace(),
				pChInfo->GetChannelIndex(),
				pChInfo->GetChannelNo());
			Buffer+=szText;
			if (pChInfo->GetServiceType()!=0) {
				StdUtil::snprintf(szText,lengthof(szText),TEXT("%d"),pChInfo->GetServiceType());
				Buffer+=szText;
			}
			Buffer+=_T(',');
			if (pChInfo->GetServiceID()!=0) {
				StdUtil::snprintf(szText,lengthof(szText),TEXT("%d"),pChInfo->GetServiceID());
				Buffer+=szText;
			}
			Buffer+=_T(',');
			if (pChInfo->GetNetworkID()!=0) {
				StdUtil::snprintf(szText,lengthof(szText),TEXT("%d"),pChInfo->GetNetworkID());
				Buffer+=szText;
			}
			Buffer+=_T(',');
			if (pChInfo->GetTransportStreamID()!=0) {
				StdUtil::snprintf(szText,lengthof(szText),TEXT("%d"),pChInfo->GetTransportStreamID());
				Buffer+=szText;
			}
			Buffer+=_T(',');
			Buffer+=pChInfo->IsEnabled()?_T('1'):_T('0');
			Buffer+=TEXT("\r\n");
		}
	}

	HANDLE hFile;
	DWORD Write;

	hFile=::CreateFile(pszFileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,
					   FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return false;

#ifdef UNICODE
	bool fUnicode=true;

	if (::GetACP()==CP_SHIFT_JIS) {
		BOOL fUsedDefaultChar=FALSE;
		int Length=::WideCharToMultiByte(
			CP_SHIFT_JIS,0,
			Buffer.data(),static_cast<int>(Buffer.length()),
			NULL,0,NULL,&fUsedDefaultChar);
		if (Length>0 && !fUsedDefaultChar) {
			char *pMBCSBuffer=new char[Length];
			Length=::WideCharToMultiByte(
				CP_SHIFT_JIS,0,
				Buffer.data(),static_cast<int>(Buffer.length()),
				pMBCSBuffer,Length,NULL,NULL);
			if (Length<1
					|| !::WriteFile(hFile,pMBCSBuffer,Length,&Write,NULL)
					|| Write!=static_cast<DWORD>(Length)) {
				delete [] pMBCSBuffer;
				::CloseHandle(hFile);
				return false;
			}
			delete [] pMBCSBuffer;
			fUnicode=false;
		}
	}

	if (fUnicode) {
		static const WCHAR BOM=0xFEFF;
		if (!::WriteFile(hFile,&BOM,sizeof(BOM),&Write,NULL)
					|| Write!=sizeof(BOM)
				|| !::WriteFile(hFile,Buffer.data(),static_cast<DWORD>(Buffer.length()),&Write,NULL)
					|| Write!=static_cast<DWORD>(Buffer.length())) {
			::CloseHandle(hFile);
			return false;
		}
	}
#else
	if (!::WriteFile(hFile,Buffer.data(),static_cast<DWORD>(Buffer.length()),&Write,NULL)
			|| Write!=static_cast<DWORD>(Buffer.length())) {
		::CloseHandle(hFile);
		return false;
	}
#endif

	::CloseHandle(hFile);

	return true;
}


static void SkipSpaces(LPTSTR *ppText)
{
	LPTSTR p=*ppText;
	p+=::StrSpn(p,TEXT(" \t"));
	*ppText=p;
}

static bool NextToken(LPTSTR *ppText)
{
	LPTSTR p=*ppText;

	SkipSpaces(&p);
	if (*p!=_T(','))
		return false;
	p++;
	SkipSpaces(&p);
	*ppText=p;
	return true;
}

bool inline IsDigit(TCHAR c)
{
	return c>=_T('0') && c<=_T('9');
}

static int ParseDigits(LPTSTR *ppText)
{
	LPTSTR pEnd;
	int Value=std::_tcstol(*ppText,&pEnd,10);
	*ppText=pEnd;
	return Value;
}

bool CTuningSpaceList::LoadFromFile(LPCTSTR pszFileName)
{
	TRACE(TEXT("CTuningSpaceList::LoadFromFile() : \"%s\"\n"),pszFileName);

	static const LONGLONG MAX_FILE_SIZE=8LL*1024*1024;

	HANDLE hFile;
	LARGE_INTEGER FileSize;
	DWORD Read;

	hFile=::CreateFile(pszFileName,GENERIC_READ,FILE_SHARE_READ,NULL,
					   OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return false;
	if (!::GetFileSizeEx(hFile,&FileSize)
			|| FileSize.QuadPart<1
			|| FileSize.QuadPart>MAX_FILE_SIZE) {
		::CloseHandle(hFile);
		return false;
	}
	BYTE *pFileBuffer=new BYTE[FileSize.LowPart+sizeof(TCHAR)];
	if (!::ReadFile(hFile,pFileBuffer,FileSize.LowPart,&Read,NULL) || Read!=FileSize.LowPart) {
		delete [] pFileBuffer;
		::CloseHandle(hFile);
		return false;
	}
	::CloseHandle(hFile);
	LPTSTR pszBuffer=NULL,p;
	if (FileSize.LowPart>=2 && *pointer_cast<LPWSTR>(pFileBuffer)==0xFEFF) {
#ifdef UNICODE
		p=pointer_cast<LPWSTR>(pFileBuffer)+1;
		p[FileSize.LowPart/2-1]=L'\0';
#else
		int Length=::WideCharToMultiByte(
			CP_ACP,0,
			pointer_cast<LPCSTR>(pFileBuffer),FileSize.LowPart,
			NULL,0,NULL,NULL);
		if (Length<1) {
			delete [] pFileBuffer;
			return false;
		}
		pszBuffer=new char[Length+1];
		Length=::WideCharToMultiByte(
			CP_ACP,0,
			pointer_cast<LPCWTR>(pFileBuffer)+1,FileSize.LowPart/2-1,
			pszBuffer,Length,NULL,NULL);
		pszBuffer[Length]='\0';
		p=pszBuffer;
#endif
	} else {
#ifdef UNICODE
		int Length=::MultiByteToWideChar(
			CP_SHIFT_JIS,0,
			pointer_cast<LPCSTR>(pFileBuffer),FileSize.LowPart,
			NULL,0);
		if (Length<1) {
			delete [] pFileBuffer;
			return false;
		}
		pszBuffer=new WCHAR[Length+1];
		Length=::MultiByteToWideChar(
			CP_SHIFT_JIS,0,
			pointer_cast<LPCSTR>(pFileBuffer),FileSize.LowPart,
			pszBuffer,Length);
		pszBuffer[Length]=L'\0';
		p=pszBuffer;
#else
		p=pointer_cast<LPSTR>(pFileBuffer);
		p[FileSize.LowPart]='\0';
#endif
	}

	m_AllChannelList.Clear();

	do {
		TCHAR szName[MAX_CHANNEL_NAME];

		p+=::StrSpn(p,TEXT("\r\n \t"));

		if (*p==_T('#') || *p==_T(';')) {	// コメント
			p++;
			if (*p==_T('#')) {
				p++;
				if (::StrCmpNI(p,TEXT("SPACE("),6)==0) {
					// チューニング空間名 #space(インデックス,名前)
					p+=6;
					SkipSpaces(&p);
					if (IsDigit(*p)) {
						int Space=ParseDigits(&p);
						if (Space>=0 && Space<100 && NextToken(&p)) {
							int Length=::StrCSpn(p,TEXT(")\r\n"));
							if (p[Length]==_T(')') && p[Length+1]==_T(')'))
								Length++;
							if (Length>0) {
								::lstrcpyn(szName,p,min(Length+1,lengthof(szName)));
								if ((int)m_TuningSpaceList.size()<=Space) {
									Reserve(Space+1);
									m_TuningSpaceList[Space]->SetName(szName);
								}
								p+=Length;
								if (*p==_T('\0'))
									break;
								p++;
							}
						}
					}
				}
			}
			goto Next;
		}
		if (*p==_T('\0'))
			break;

		{
			CChannelInfo ChInfo;

			// チャンネル名
			int NameLength=0;
			bool fQuote=false;
			if (*p==_T('"')) {
				fQuote=true;
				p++;
			}
			while (*p!=_T('\0')) {
				if (fQuote) {
					if (*p==_T('"')) {
						p++;
						if (*p!=_T('"')) {
							SkipSpaces(&p);
							break;
						}
					}
				} else {
					if (*p==_T(','))
						break;
				}
				if (NameLength<lengthof(szName)-1)
					szName[NameLength++]=*p;
				p++;
			}
			szName[NameLength]=_T('\0');
			ChInfo.SetName(szName);
			if (!NextToken(&p))
				goto Next;

			// チューニング空間
			if (!IsDigit(*p))
				goto Next;
			ChInfo.SetSpace(ParseDigits(&p));
			if (!NextToken(&p))
				goto Next;

			// チャンネル
			if (!IsDigit(*p))
				goto Next;
			ChInfo.SetChannelIndex(ParseDigits(&p));

			if (NextToken(&p)) {
				// リモコン番号(オプション)
				ChInfo.SetChannelNo(ParseDigits(&p));
				if (NextToken(&p)) {
					// サービスタイプ(オプション)
					ChInfo.SetServiceType(static_cast<BYTE>(ParseDigits(&p)));
					if (NextToken(&p)) {
						// サービスID(オプション)
						ChInfo.SetServiceID(static_cast<WORD>(ParseDigits(&p)));
						if (NextToken(&p)) {
							// ネットワークID(オプション)
							ChInfo.SetNetworkID(static_cast<WORD>(ParseDigits(&p)));
							if (NextToken(&p)) {
								// トランスポートストリームID(オプション)
								ChInfo.SetTransportStreamID(static_cast<WORD>(ParseDigits(&p)));
								if (NextToken(&p)) {
									// 状態(オプション)
									if (IsDigit(*p)) {
										int Flags=ParseDigits(&p);
										ChInfo.Enable((Flags&1)!=0);
									}
								}
							}
						}
					}
				}
			}

			m_AllChannelList.AddChannel(ChInfo);
		}

	Next:
		p+=::StrCSpn(p,TEXT("\r\n"));
	} while (*p!=_T('\0'));

	delete [] pszBuffer;
	delete [] pFileBuffer;

	return MakeTuningSpaceList(&m_AllChannelList);
}
