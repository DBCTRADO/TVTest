#include "stdafx.h"
#include <algorithm>
#include "TVTest.h"
#include "ChannelList.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




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
	if (pszName!=NULL)
		m_Name=pszName;
	else
		m_Name.clear();
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


int CChannelList::Find(int Space,int ChannelIndex,int ServiceID) const
{
	for (size_t i=0;i<m_ChannelList.size();i++) {
		const CChannelInfo *pChInfo=m_ChannelList[i];

		if ((Space<0 || pChInfo->GetSpace()==Space)
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


int CChannelList::FindByIDs(WORD NetworkID,WORD TransportStreamID,WORD ServiceID) const
{
	for (size_t i=0;i<m_ChannelList.size();i++) {
		const CChannelInfo *pChannelInfo=m_ChannelList[i];
		if ((NetworkID==0 || pChannelInfo->GetNetworkID()==NetworkID)
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


int CChannelList::GetNextChannelNo(int ChannelNo,bool fWrap) const
{
	int Channel,Min,No;

	Channel=INT_MAX;
	Min=INT_MAX;
	for (auto i=m_ChannelList.begin();i!=m_ChannelList.end();i++) {
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
			return 0;
		return Min;
	}
	return Channel;
}


int CChannelList::GetPrevChannelNo(int ChannelNo,bool fWrap) const
{
	int Channel,Max,No;

	Channel=0;
	Max=0;
	for (auto i=m_ChannelList.begin();i!=m_ChannelList.end();i++) {
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
			return Max;
		return 0;
	}
	return Channel;
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


bool CChannelList::UpdateStreamInfo(int Space,int ChannelIndex,
	WORD NetworkID,WORD TransportStreamID,WORD ServiceID)
{
	if (ServiceID==0)
		return false;

	for (auto i=m_ChannelList.begin();i!=m_ChannelList.end();i++) {
		CChannelInfo *pChannelInfo=*i;

		if (pChannelInfo->GetSpace()==Space
				&& pChannelInfo->GetChannelIndex()==ChannelIndex
				&& pChannelInfo->GetServiceID()==ServiceID) {
			if (NetworkID!=0 && pChannelInfo->GetNetworkID()==0)
				pChannelInfo->SetNetworkID(NetworkID);
			if (TransportStreamID!=0 && pChannelInfo->GetTransportStreamID()==0)
				pChannelInfo->SetTransportStreamID(TransportStreamID);
		}
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


// TODO: 必要に応じてUnicodeで保存
bool CTuningSpaceList::SaveToFile(LPCTSTR pszFileName) const
{
	HANDLE hFile;
	DWORD Length,Write;

	hFile=::CreateFile(pszFileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,
					   FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return false;

	static const char szComment[]=
		"; " APP_NAME_A " チャンネル設定ファイル\r\n"
		"; 名称,チューニング空間,チャンネル,リモコン番号,サービスタイプ,サービスID,ネットワークID,TSID,状態\r\n";
	if (!::WriteFile(hFile,szComment,sizeof(szComment)-1,&Write,NULL) || Write!=sizeof(szComment)-1) {
		::CloseHandle(hFile);
		return false;
	}

	for (int i=0;i<NumSpaces();i++) {
		const CChannelList *pChannelList=m_TuningSpaceList[i]->GetChannelList();
		char szText[MAX_CHANNEL_NAME*2+256];

		if (pChannelList->NumChannels()==0)
			continue;

		if (GetTuningSpaceName(i)!=NULL) {
			Length=::wsprintfA(szText,";#SPACE(%d,%S)\r\n",i,GetTuningSpaceName(i));
			if (!::WriteFile(hFile,szText,Length,&Write,NULL) || Write!=Length) {
				::CloseHandle(hFile);
				return false;
			}
		}

		for (int j=0;j<pChannelList->NumChannels();j++) {
			const CChannelInfo *pChInfo=pChannelList->GetChannelInfo(j);
			LPCTSTR pszName=pChInfo->GetName();
			char aszName[MAX_CHANNEL_NAME*2];

			// 必要に応じて " で囲む
			if (pszName[0]==_T('#') || pszName[0]==_T(';')
					|| ::StrChr(pszName,_T(','))!=NULL
					|| ::StrChr(pszName,_T('"'))!=NULL) {
				LPCTSTR p=pszName;
				int NameLength=1;
				aszName[0]='"';
				while (*p!=_T('\0') && NameLength<lengthof(aszName)-2) {
					if (*p==_T('"')) {
						if (NameLength>=lengthof(aszName)-3)
							break;
						aszName[NameLength++]='"';
						aszName[NameLength++]='"';
						p++;
					} else {
#ifdef UNICODE
						int SrcLength;
						for (SrcLength=1;p[SrcLength]!=L'"' && p[SrcLength]!=L'\0';SrcLength++);
						int DstLength=::WideCharToMultiByte(CP_ACP,0,p,SrcLength,
											&aszName[NameLength],lengthof(aszName)-2-NameLength,NULL,NULL);
						NameLength+=DstLength;
						p+=SrcLength;
#else
						if (::IsDBCSLeadByteEx(CP_ACP,*p) && *(p+1)!='\0') {
							if (NameLength>=lengthof(aszName)-3)
								break;
							aszName[NameLength++]=*p++;
						}
						aszName[NameLength++]=*p++;
#endif
					}
				}
				aszName[NameLength++]='"';
				aszName[NameLength]='\0';
			} else {
#ifdef UNICODE
				::WideCharToMultiByte(CP_ACP,0,pszName,-1,
									  aszName,lengthof(aszName),NULL,NULL);
#else
				::lstrcpy(aszName,pszName);
#endif
			}

			Length=::wsprintfA(szText,"%s,%d,%d,%d,",
				aszName,
				pChInfo->GetSpace(),
				pChInfo->GetChannelIndex(),
				pChInfo->GetChannelNo());
			if (pChInfo->GetServiceType()!=0)
				Length+=::wsprintfA(szText+Length,"%d",pChInfo->GetServiceType());
			szText[Length++]=',';
			if (pChInfo->GetServiceID()!=0)
				Length+=::wsprintfA(szText+Length,"%d",pChInfo->GetServiceID());
			szText[Length++]=',';
			if (pChInfo->GetNetworkID()!=0)
				Length+=::wsprintfA(szText+Length,"%d",pChInfo->GetNetworkID());
			szText[Length++]=',';
			if (pChInfo->GetTransportStreamID()!=0)
				Length+=::wsprintfA(szText+Length,"%d",pChInfo->GetTransportStreamID());
			szText[Length++]=',';
			szText[Length++]=pChInfo->IsEnabled()?'1':'0';
			szText[Length++]='\r';
			szText[Length++]='\n';

			if (!::WriteFile(hFile,szText,Length,&Write,NULL) || Write!=Length) {
				::CloseHandle(hFile);
				return false;
			}
		}
	}

	::CloseHandle(hFile);

	return true;
}


static void SkipSpaces(char **ppText)
{
	char *p=*ppText;

	while (*p==' ' || *p=='\t')
		p++;
	*ppText=p;
}

bool inline IsDigit(char c)
{
	return c>='0' && c<='9';
}

static int ParseDigits(char **ppText)
{
	char *p=*ppText;
	int Value=0;

	SkipSpaces(&p);
	while (IsDigit(*p)) {
		Value=Value*10+(*p-'0');
		p++;
	}
	*ppText=p;
	return Value;
}

// TODO: UTF-16対応
bool CTuningSpaceList::LoadFromFile(LPCTSTR pszFileName)
{
	static const LONGLONG MAX_FILE_SIZE=8LL*1024*1024;

	HANDLE hFile;
	LARGE_INTEGER FileSize;
	DWORD Read;
	char *pszFile,*p;

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
	pszFile=new char[FileSize.LowPart+1];
	if (!::ReadFile(hFile,pszFile,FileSize.LowPart,&Read,NULL) || Read!=FileSize.LowPart) {
		delete [] pszFile;
		::CloseHandle(hFile);
		return false;
	}
	pszFile[FileSize.LowPart]='\0';
	::CloseHandle(hFile);

	m_AllChannelList.Clear();
	p=pszFile;

	do {
		TCHAR szName[MAX_CHANNEL_NAME];
		int Space,Channel,ControlKey,ServiceType,ServiceID,NetworkID,TransportStreamID;
		bool fEnabled;

		while (*p=='\r' || *p=='\n' || *p==' ' || *p=='\t')
			p++;
		if (*p=='#' || *p==';') {	// コメント
			p++;
			if (*p=='#') {
				p++;
				if (::strnicmp(p,"space(",6)==0) {
					// チューニング空間名 #space(インデックス,名前)
					p+=6;
					SkipSpaces(&p);
					if (IsDigit(*p)) {
						Space=ParseDigits(&p);
						SkipSpaces(&p);
						if (Space>=0 && Space<10 && *p==',') {
							int i;

							p++;
							SkipSpaces(&p);
							for (i=0;p[i]!=')' && p[i]!='\0' && p[i]!='\r' && p[i]!='\n';i++);
							if (p[i]==')' && p[i+1]==')')
								i++;
							if (i>0) {
#ifdef UNICODE
								szName[::MultiByteToWideChar(CP_ACP,0,p,i,szName,lengthof(szName)-1)]='\0';
#else
								::lstrcpyn(szName,p,min(i+1,lengthof(szName)));
#endif
								if ((int)m_TuningSpaceList.size()<=Space) {
									Reserve(Space+1);
									m_TuningSpaceList[Space]->SetName(szName);
								}
								p+=i;
								if (*p=='\0')
									break;
								p++;
							}
						}
					}
				}
			}
			goto Next;
		}
		if (*p=='\0')
			break;

		// チャンネル名
		char cName[MAX_CHANNEL_NAME*2];
		int NameLength=0;
		bool fTruncate=false;
		bool fQuote=false;
		if (*p=='"') {
			fQuote=true;
			p++;
		}
		while (*p!='\0') {
			if (fQuote) {
				if (*p=='"') {
					p++;
					if (*p!='"') {
						SkipSpaces(&p);
						break;
					}
				}
			} else {
				if (*p==',')
					break;
			}
			if (::IsDBCSLeadByteEx(CP_ACP,*p)) {
				if (*(p+1)=='\0') {
					p++;
					break;
				}
				if (NameLength<lengthof(cName)-2) {
					cName[NameLength++]=*p;
					cName[NameLength++]=*(p+1);
				} else {
					fTruncate=true;
				}
				p+=2;
			} else {
				if (!fTruncate && NameLength<lengthof(cName)-1) {
					cName[NameLength++]=*p;
				}
				p++;
			}
		}
		if (*p!=',')
			goto Next;
		p++;
		SkipSpaces(&p);
		cName[NameLength]='\0';
#ifdef UNICODE
		::MultiByteToWideChar(CP_ACP,0,cName,-1,szName,MAX_CHANNEL_NAME);
#else
		::lstrcpynA(szName,cName,MAX_CHANNEL_NAME);
#endif

		// チューニング空間
		if (!IsDigit(*p))
			goto Next;
		Space=ParseDigits(&p);
		SkipSpaces(&p);
		if (*p!=',')
			goto Next;
		p++;
		SkipSpaces(&p);

		// チャンネル
		if (!IsDigit(*p))
			goto Next;
		Channel=ParseDigits(&p);
		SkipSpaces(&p);

		ControlKey=0;
		ServiceType=0;
		ServiceID=0;
		NetworkID=0;
		TransportStreamID=0;
		fEnabled=true;

		if (*p==',') {
			p++;
			// リモコン番号(オプション)
			ControlKey=ParseDigits(&p);
			SkipSpaces(&p);
			if (*p==',') {
				// サービスタイプ(オプション)
				p++;
				ServiceType=ParseDigits(&p);
				SkipSpaces(&p);
				if (*p==',') {
					// サービスID(オプション)
					p++;
					ServiceID=ParseDigits(&p);
					SkipSpaces(&p);
					if (*p==',') {
						// ネットワークID(オプション)
						p++;
						NetworkID=ParseDigits(&p);
						SkipSpaces(&p);
						if (*p==',') {
							// トランスポートストリームID(オプション)
							p++;
							TransportStreamID=ParseDigits(&p);
							SkipSpaces(&p);
							if (*p==',') {
								// 有効状態(オプション)
								p++;
								SkipSpaces(&p);
								if (IsDigit(*p)) {
									int Value=ParseDigits(&p);
									fEnabled=(Value&1)!=0;
								}
							}
						}
					}
				}
			}
		}
		{
			CChannelInfo ChInfo(Space,Channel,ControlKey,szName);
			if (ServiceID!=0)
				ChInfo.SetServiceID(ServiceID);
			if (NetworkID!=0)
				ChInfo.SetNetworkID(NetworkID);
			if (TransportStreamID!=0)
				ChInfo.SetTransportStreamID(TransportStreamID);
			if (ServiceType!=0)
				ChInfo.SetServiceType(ServiceType);
			if (!fEnabled)
				ChInfo.Enable(false);
			m_AllChannelList.AddChannel(ChInfo);
		}

	Next:
		while (*p!='\r' && *p!='\n' && *p!='\0')
			p++;
	} while (*p!='\0');

	delete [] pszFile;

	return MakeTuningSpaceList(&m_AllChannelList);
}


bool CTuningSpaceList::UpdateStreamInfo(int Space,int ChannelIndex,
	WORD NetworkID,WORD TransportStreamID,WORD ServiceID)
{
	if (Space<0 || Space>=NumSpaces())
		return false;
	m_TuningSpaceList[Space]->GetChannelList()->UpdateStreamInfo(
			Space,ChannelIndex,NetworkID,TransportStreamID,ServiceID);
	m_AllChannelList.UpdateStreamInfo(Space,ChannelIndex,
									  NetworkID,TransportStreamID,ServiceID);
	return true;
}
