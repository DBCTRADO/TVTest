#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "Plugin.h"
#include "Image.h"
#include "DialogUtil.h"
#include "Command.h"
#include "EpgProgramList.h"
#include "DriverManager.h"
#include "LogoManager.h"
#include "Controller.h"
#include "TSProcessor.h"
#include "BonTsEngine/TsEncode.h"
#include "resource.h"
#include <algorithm>
#include "Common/DebugDef.h"


struct PluginMessageParam
{
	CPlugin *pPlugin;
	UINT Message;
	LPARAM lParam1;
	LPARAM lParam2;
};


static void EventInfoToProgramGuideProgramInfo(const CEventInfoData &EventInfo,
											   TVTest::ProgramGuideProgramInfo *pProgramInfo)
{
	pProgramInfo->NetworkID=EventInfo.m_NetworkID;
	pProgramInfo->TransportStreamID=EventInfo.m_TransportStreamID;
	pProgramInfo->ServiceID=EventInfo.m_ServiceID;
	pProgramInfo->EventID=EventInfo.m_EventID;
	pProgramInfo->StartTime=EventInfo.m_StartTime;
	pProgramInfo->Duration=EventInfo.m_Duration;
}




class CControllerPlugin : public CController
{
	CPlugin *m_pPlugin;
	DWORD m_Flags;
	TVTest::String m_Name;
	TVTest::String m_Text;
	int m_NumButtons;
	CController::ButtonInfo *m_pButtonList;
	LPTSTR m_pButtonNameList;
	TVTest::String m_IniFileName;
	TVTest::String m_SectionName;
	UINT m_ControllerImageID;
	UINT m_SelButtonsImageID;
	TVTest::ControllerInfo::TranslateMessageCallback m_pTranslateMessage;
	void *m_pClientData;

public:
	CControllerPlugin(CPlugin *pPlugin,const TVTest::ControllerInfo *pInfo);
	~CControllerPlugin();
	LPCTSTR GetName() const { return m_Name.c_str(); }
	LPCTSTR GetText() const { return m_Text.c_str(); }
	int NumButtons() const { return m_NumButtons; }
	bool GetButtonInfo(int Index,ButtonInfo *pInfo) const;
	bool Enable(bool fEnable) { return m_pPlugin->Enable(fEnable); }
	bool IsEnabled() const { return m_pPlugin->IsEnabled(); }
	bool IsActiveOnly() const { return (m_Flags&TVTest::CONTROLLER_FLAG_ACTIVEONLY)!=0; }
	bool SetTargetWindow(HWND hwnd) {
		return m_pPlugin->SendEvent(TVTest::EVENT_CONTROLLERFOCUS,(LPARAM)hwnd)!=0;
	}
	bool TranslateMessage(HWND hwnd,MSG *pMessage);
	bool GetIniFileName(LPTSTR pszFileName,int MaxLength) const;
	LPCTSTR GetIniFileSection() const;
	HBITMAP GetImage(ImageType Type) const;
};


CControllerPlugin::CControllerPlugin(CPlugin *pPlugin,const TVTest::ControllerInfo *pInfo)
	: m_pPlugin(pPlugin)
	, m_Flags(pInfo->Flags)
	, m_Name(pInfo->pszName)
	, m_Text(pInfo->pszText)
	, m_NumButtons(pInfo->NumButtons)
	, m_IniFileName(TVTest::StringFromCStr(pInfo->pszIniFileName))
	, m_SectionName(TVTest::StringFromCStr(pInfo->pszSectionName))
	, m_ControllerImageID(pInfo->ControllerImageID)
	, m_SelButtonsImageID(pInfo->SelButtonsImageID)
	, m_pTranslateMessage(pInfo->pTranslateMessage)
	, m_pClientData(pInfo->pClientData)
{
	const CCommandList &CommandList=GetAppClass().CommandList;

	int Length=m_NumButtons+1;
	for (int i=0;i<m_NumButtons;i++)
		Length+=::lstrlen(pInfo->pButtonList[i].pszName);
	m_pButtonNameList=new TCHAR[Length];
	LPTSTR pszName=m_pButtonNameList;

	m_pButtonList=new CController::ButtonInfo[m_NumButtons];
	for (int i=0;i<m_NumButtons;i++) {
		const TVTest::ControllerButtonInfo &ButtonInfo=pInfo->pButtonList[i];

		::lstrcpy(pszName,ButtonInfo.pszName);
		m_pButtonList[i].pszName=pszName;
		m_pButtonList[i].DefaultCommand=ButtonInfo.pszDefaultCommand!=NULL?
					CommandList.ParseText(ButtonInfo.pszDefaultCommand):0;
		m_pButtonList[i].ImageButtonRect.Left=ButtonInfo.ButtonRect.Left;
		m_pButtonList[i].ImageButtonRect.Top=ButtonInfo.ButtonRect.Top;
		m_pButtonList[i].ImageButtonRect.Width=ButtonInfo.ButtonRect.Width;
		m_pButtonList[i].ImageButtonRect.Height=ButtonInfo.ButtonRect.Height;
		m_pButtonList[i].ImageSelButtonPos.Left=ButtonInfo.SelButtonPos.Left;
		m_pButtonList[i].ImageSelButtonPos.Top=ButtonInfo.SelButtonPos.Top;
		pszName+=::lstrlen(pszName)+1;
	}
}


CControllerPlugin::~CControllerPlugin()
{
	delete [] m_pButtonList;
	delete [] m_pButtonNameList;
}


bool CControllerPlugin::GetButtonInfo(int Index,ButtonInfo *pInfo) const
{
	if (Index<0 || Index>=m_NumButtons)
		return false;
	*pInfo=m_pButtonList[Index];
	return true;
}


bool CControllerPlugin::TranslateMessage(HWND hwnd,MSG *pMessage)
{
	if (m_pTranslateMessage==NULL)
		return false;
	return m_pTranslateMessage(hwnd,pMessage,m_pClientData)!=FALSE;
}


bool CControllerPlugin::GetIniFileName(LPTSTR pszFileName,int MaxLength) const
{
	if (m_IniFileName.empty())
		return CController::GetIniFileName(pszFileName,MaxLength);
	if (m_IniFileName.length()>=(TVTest::String::size_type)MaxLength)
		return false;
	::lstrcpy(pszFileName,m_IniFileName.c_str());
	return true;
}


LPCTSTR CControllerPlugin::GetIniFileSection() const
{
	if (m_SectionName.empty())
		return m_Name.c_str();
	return m_SectionName.c_str();
}


HBITMAP CControllerPlugin::GetImage(ImageType Type) const
{
	UINT ID;

	switch (Type) {
	case IMAGE_CONTROLLER:	ID=m_ControllerImageID;	break;
	case IMAGE_SELBUTTONS:	ID=m_SelButtonsImageID;	break;
	default:	return NULL;
	}
	if (ID==0)
		return NULL;
	return static_cast<HBITMAP>(::LoadImage(m_pPlugin->GetModuleHandle(),
											MAKEINTRESOURCE(ID),
											IMAGE_BITMAP,0,0,
											LR_CREATEDIBSECTION));
}



static void ConvertChannelInfo(const CChannelInfo *pChInfo,TVTest::ChannelInfo *pChannelInfo)
{
	pChannelInfo->Space=pChInfo->GetSpace();
	pChannelInfo->Channel=pChInfo->GetChannelIndex();
	pChannelInfo->RemoteControlKeyID=pChInfo->GetChannelNo();
	pChannelInfo->NetworkID=pChInfo->GetNetworkID();
	pChannelInfo->TransportStreamID=pChInfo->GetTransportStreamID();
	pChannelInfo->szNetworkName[0]='\0';
	pChannelInfo->szTransportStreamName[0]='\0';
	::lstrcpyn(pChannelInfo->szChannelName,pChInfo->GetName(),lengthof(pChannelInfo->szChannelName));
	if (pChannelInfo->Size>=TVTest::CHANNELINFO_SIZE_V2) {
		pChannelInfo->PhysicalChannel=pChInfo->GetPhysicalChannel();
		pChannelInfo->ServiceIndex=0;	// 使用不可
		pChannelInfo->ServiceID=pChInfo->GetServiceID();
		if (pChannelInfo->Size==sizeof(TVTest::ChannelInfo)) {
			pChannelInfo->Flags=0;
			if (!pChInfo->IsEnabled())
				pChannelInfo->Flags|=TVTest::CHANNEL_FLAG_DISABLED;
		}
	}
}




class CEpgDataConverter
{
	static void GetEventInfoSize(const CEventInfoData &EventInfo,SIZE_T *pInfoSize,SIZE_T *pStringSize);
	static void ConvertEventInfo(const CEventInfoData &EventData,
								 TVTest::EpgEventInfo **ppEventInfo,LPWSTR *ppStringBuffer);
	static void SortEventList(TVTest::EpgEventInfo **ppFirst,TVTest::EpgEventInfo **ppLast);
	static LPWSTR CopyString(LPWSTR pDstString,LPCWSTR pSrcString)
	{
		LPCWSTR p=pSrcString;
		LPWSTR q=pDstString;

		while ((*q=*p)!=L'\0') {
			p++;
			q++;
		}
		return q+1;
	}

public:
	TVTest::EpgEventInfo *Convert(const CEventInfoData &EventData) const;
	TVTest::EpgEventInfo **Convert(const CEventInfoList &EventList) const;
	static void FreeEventInfo(TVTest::EpgEventInfo *pEventInfo);
	static void FreeEventList(TVTest::EpgEventInfo **ppEventList);
};


void CEpgDataConverter::GetEventInfoSize(const CEventInfoData &EventInfo,
										 SIZE_T *pInfoSize,SIZE_T *pStringSize)
{
	SIZE_T InfoSize=sizeof(TVTest::EpgEventInfo);
	SIZE_T StringSize=0;

	if (!EventInfo.m_EventName.empty())
		StringSize+=EventInfo.m_EventName.length()+1;
	if (!EventInfo.m_EventText.empty())
		StringSize+=EventInfo.m_EventText.length()+1;
	if (!EventInfo.m_EventExtendedText.empty())
		StringSize+=EventInfo.m_EventExtendedText.length()+1;
	InfoSize+=(sizeof(TVTest::EpgEventVideoInfo)+sizeof(TVTest::EpgEventVideoInfo*))*EventInfo.m_VideoList.size();
	for (size_t i=0;i<EventInfo.m_VideoList.size();i++) {
		if (EventInfo.m_VideoList[i].szText[0]!='\0')
			StringSize+=::lstrlenW(EventInfo.m_VideoList[i].szText)+1;
	}
	InfoSize+=(sizeof(TVTest::EpgEventAudioInfo)+sizeof(TVTest::EpgEventAudioInfo*))*EventInfo.m_AudioList.size();
	for (size_t i=0;i<EventInfo.m_AudioList.size();i++) {
		if (EventInfo.m_AudioList[i].szText[0]!='\0')
			StringSize+=::lstrlenW(EventInfo.m_AudioList[i].szText)+1;
	}
	InfoSize+=sizeof(TVTest::EpgEventContentInfo)*EventInfo.m_ContentNibble.NibbleCount;
	InfoSize+=(sizeof(TVTest::EpgEventGroupInfo)+sizeof(TVTest::EpgEventGroupInfo*))*EventInfo.m_EventGroupList.size();
	for (size_t i=0;i<EventInfo.m_EventGroupList.size();i++)
		InfoSize+=sizeof(TVTest::EpgGroupEventInfo)*EventInfo.m_EventGroupList[i].EventList.size();

	*pInfoSize=InfoSize;
	*pStringSize=StringSize*sizeof(WCHAR);
}


void CEpgDataConverter::ConvertEventInfo(const CEventInfoData &EventData,
										 TVTest::EpgEventInfo **ppEventInfo,LPWSTR *ppStringBuffer)
{
	LPWSTR pString=*ppStringBuffer;

	TVTest::EpgEventInfo *pEventInfo=*ppEventInfo;
	pEventInfo->EventID=EventData.m_EventID;
	pEventInfo->RunningStatus=EventData.m_RunningStatus;
	pEventInfo->FreeCaMode=EventData.m_bFreeCaMode;
	pEventInfo->Reserved=0;
	pEventInfo->StartTime=EventData.m_StartTime;
	pEventInfo->Duration=EventData.m_Duration;
	pEventInfo->VideoListLength=(BYTE)EventData.m_VideoList.size();
	pEventInfo->AudioListLength=(BYTE)EventData.m_AudioList.size();
	pEventInfo->ContentListLength=(BYTE)EventData.m_ContentNibble.NibbleCount;
	pEventInfo->EventGroupListLength=(BYTE)EventData.m_EventGroupList.size();
	if (!EventData.m_EventName.empty()) {
		pEventInfo->pszEventName=pString;
		pString=CopyString(pString,EventData.m_EventName.c_str());
	} else {
		pEventInfo->pszEventName=NULL;
	}
	if (!EventData.m_EventText.empty()) {
		pEventInfo->pszEventText=pString;
		pString=CopyString(pString,EventData.m_EventText.c_str());
	} else {
		pEventInfo->pszEventText=NULL;
	}
	if (!EventData.m_EventExtendedText.empty()) {
		pEventInfo->pszEventExtendedText=pString;
		pString=CopyString(pString,EventData.m_EventExtendedText.c_str());
	} else {
		pEventInfo->pszEventExtendedText=NULL;
	}

	BYTE *p=(BYTE*)(pEventInfo+1);

	if (!EventData.m_VideoList.empty()) {
		pEventInfo->VideoList=(TVTest::EpgEventVideoInfo**)p;
		p+=sizeof(TVTest::EpgEventVideoInfo*)*EventData.m_VideoList.size();
		for (size_t i=0;i<EventData.m_VideoList.size();i++) {
			const CEventInfoData::VideoInfo &Video=EventData.m_VideoList[i];
			pEventInfo->VideoList[i]=(TVTest::EpgEventVideoInfo*)p;
			pEventInfo->VideoList[i]->StreamContent=Video.StreamContent;
			pEventInfo->VideoList[i]->ComponentType=Video.ComponentType;
			pEventInfo->VideoList[i]->ComponentTag=Video.ComponentTag;
			pEventInfo->VideoList[i]->Reserved=0;
			pEventInfo->VideoList[i]->LanguageCode=Video.LanguageCode;
			p+=sizeof(TVTest::EpgEventVideoInfo);
			if (Video.szText[i]!='\0') {
				pEventInfo->VideoList[i]->pszText=pString;
				pString=CopyString(pString,Video.szText);
			} else {
				pEventInfo->VideoList[i]->pszText=NULL;
			}
		}
	} else {
		pEventInfo->VideoList=NULL;
	}

	if (EventData.m_AudioList.size()>0) {
		pEventInfo->AudioList=(TVTest::EpgEventAudioInfo**)p;
		p+=sizeof(TVTest::EpgEventAudioInfo*)*EventData.m_AudioList.size();
		for (size_t i=0;i<EventData.m_AudioList.size();i++) {
			const CEventInfoData::AudioInfo &Audio=EventData.m_AudioList[i];
			TVTest::EpgEventAudioInfo *pAudioInfo=(TVTest::EpgEventAudioInfo*)p;
			pEventInfo->AudioList[i]=pAudioInfo;
			pAudioInfo->Flags=0;
			if (Audio.bESMultiLingualFlag)
				pAudioInfo->Flags|=TVTest::EPG_EVENT_AUDIO_FLAG_MULTILINGUAL;
			if (Audio.bMainComponentFlag)
				pAudioInfo->Flags|=TVTest::EPG_EVENT_AUDIO_FLAG_MAINCOMPONENT;
			pAudioInfo->StreamContent=Audio.StreamContent;
			pAudioInfo->ComponentType=Audio.ComponentType;
			pAudioInfo->ComponentTag=Audio.ComponentTag;
			pAudioInfo->SimulcastGroupTag=Audio.SimulcastGroupTag;
			pAudioInfo->QualityIndicator=Audio.QualityIndicator;
			pAudioInfo->SamplingRate=Audio.SamplingRate;
			pAudioInfo->Reserved=0;
			pAudioInfo->LanguageCode=Audio.LanguageCode;
			pAudioInfo->LanguageCode2=Audio.LanguageCode2;
			p+=sizeof(TVTest::EpgEventAudioInfo);
			if (Audio.szText[0]!='\0') {
				pAudioInfo->pszText=pString;
				pString=CopyString(pString,Audio.szText);
			} else {
				pAudioInfo->pszText=NULL;
			}
		}
	} else {
		pEventInfo->AudioList=NULL;
	}

	if (EventData.m_ContentNibble.NibbleCount>0) {
		pEventInfo->ContentList=(TVTest::EpgEventContentInfo*)p;
		p+=sizeof(TVTest::EpgEventContentInfo)*EventData.m_ContentNibble.NibbleCount;
		for (int i=0;i<EventData.m_ContentNibble.NibbleCount;i++) {
			pEventInfo->ContentList[i].ContentNibbleLevel1=
				EventData.m_ContentNibble.NibbleList[i].ContentNibbleLevel1;
			pEventInfo->ContentList[i].ContentNibbleLevel2=
				EventData.m_ContentNibble.NibbleList[i].ContentNibbleLevel2;
			pEventInfo->ContentList[i].UserNibble1=
				EventData.m_ContentNibble.NibbleList[i].UserNibble1;
			pEventInfo->ContentList[i].UserNibble2=
				EventData.m_ContentNibble.NibbleList[i].UserNibble2;
		}
	} else {
		pEventInfo->ContentList=NULL;
	}

	if (EventData.m_EventGroupList.size()>0) {
		pEventInfo->EventGroupList=(TVTest::EpgEventGroupInfo**)p;
		p+=sizeof(TVTest::EpgEventGroupInfo*)*EventData.m_EventGroupList.size();
		for (size_t i=0;i<EventData.m_EventGroupList.size();i++) {
			const CEventInfoData::EventGroupInfo &Group=EventData.m_EventGroupList[i];
			TVTest::EpgEventGroupInfo *pGroupInfo=(TVTest::EpgEventGroupInfo*)p;
			p+=sizeof(TVTest::EpgEventGroupInfo);
			pEventInfo->EventGroupList[i]=pGroupInfo;
			pGroupInfo->GroupType=Group.GroupType;
			pGroupInfo->EventListLength=(BYTE)Group.EventList.size();
			::ZeroMemory(pGroupInfo->Reserved,sizeof(pGroupInfo->Reserved));
			pGroupInfo->EventList=(TVTest::EpgGroupEventInfo*)p;
			for (size_t j=0;j<Group.EventList.size();j++) {
				pGroupInfo->EventList[j].NetworkID=Group.EventList[j].OriginalNetworkID;
				pGroupInfo->EventList[j].TransportStreamID=Group.EventList[j].TransportStreamID;
				pGroupInfo->EventList[j].ServiceID=Group.EventList[j].ServiceID;
				pGroupInfo->EventList[j].EventID=Group.EventList[j].EventID;
			}
			p+=sizeof(TVTest::EpgGroupEventInfo)*Group.EventList.size();
		}
	} else {
		pEventInfo->EventGroupList=NULL;
	}

	*ppEventInfo=(TVTest::EpgEventInfo*)p;
	*ppStringBuffer=pString;
}


void CEpgDataConverter::SortEventList(TVTest::EpgEventInfo **ppFirst,TVTest::EpgEventInfo **ppLast)
{
	SYSTEMTIME stKey=ppFirst[(ppLast-ppFirst)/2]->StartTime;
	TVTest::EpgEventInfo **p,**q;

	p=ppFirst;
	q=ppLast;
	while (p<=q) {
		while (CompareSystemTime(&(*p)->StartTime,&stKey)<0)
			p++;
		while (CompareSystemTime(&(*q)->StartTime,&stKey)>0)
			q--;
		if (p<=q) {
			TVTest::EpgEventInfo *pTemp;

			pTemp=*p;
			*p=*q;
			*q=pTemp;
			p++;
			q--;
		}
	}
	if (q>ppFirst)
		SortEventList(ppFirst,q);
	if (p<ppLast)
		SortEventList(p,ppLast);
}


TVTest::EpgEventInfo *CEpgDataConverter::Convert(const CEventInfoData &EventData) const
{
	SIZE_T InfoSize,StringSize;

	GetEventInfoSize(EventData,&InfoSize,&StringSize);
	BYTE *pBuffer=(BYTE*)malloc(InfoSize+StringSize);
	if (pBuffer==NULL)
		return NULL;
	TVTest::EpgEventInfo *pEventInfo=(TVTest::EpgEventInfo*)pBuffer;
	LPWSTR pString=(LPWSTR)(pBuffer+InfoSize);
	ConvertEventInfo(EventData,&pEventInfo,&pString);
#ifdef _DEBUG
	if ((BYTE*)pEventInfo-pBuffer!=InfoSize
			|| (BYTE*)pString-(pBuffer+InfoSize)!=StringSize)
		::DebugBreak();
#endif
	return (TVTest::EpgEventInfo*)pBuffer;
}


TVTest::EpgEventInfo **CEpgDataConverter::Convert(const CEventInfoList &EventList) const
{
	const SIZE_T ListSize=EventList.EventDataMap.size()*sizeof(TVTest::EpgEventInfo*);
	CEventInfoList::EventMap::const_iterator i;
	SIZE_T InfoSize=0,StringSize=0;

	for (i=EventList.EventDataMap.begin();i!=EventList.EventDataMap.end();i++) {
		SIZE_T Info,String;

		GetEventInfoSize(i->second,&Info,&String);
		InfoSize+=Info;
		StringSize+=String;
	}
	BYTE *pBuffer=(BYTE*)malloc(ListSize+InfoSize+StringSize);
	if (pBuffer==NULL)
		return NULL;
	TVTest::EpgEventInfo **ppEventList=(TVTest::EpgEventInfo**)pBuffer;
	TVTest::EpgEventInfo *pEventInfo=(TVTest::EpgEventInfo*)(pBuffer+ListSize);
	LPWSTR pString=(LPWSTR)(pBuffer+ListSize+InfoSize);
	int j=0;
	for (i=EventList.EventDataMap.begin();i!=EventList.EventDataMap.end();i++) {
		ppEventList[j++]=pEventInfo;
		ConvertEventInfo(i->second,&pEventInfo,&pString);
	}
#ifdef _DEBUG
	if ((BYTE*)pEventInfo-(pBuffer+ListSize)!=InfoSize
			|| (BYTE*)pString-(pBuffer+ListSize+InfoSize)!=StringSize)
		::DebugBreak();
#endif
	if (j>1)
		SortEventList(&ppEventList[0],&ppEventList[j-1]);
	return ppEventList;
}


void CEpgDataConverter::FreeEventInfo(TVTest::EpgEventInfo *pEventInfo)
{
	free(pEventInfo);
}


void CEpgDataConverter::FreeEventList(TVTest::EpgEventInfo **ppEventList)
{
	free(ppEventList);
}




static void GetFavoriteItemSize(const TVTest::CFavoriteItem *pItem,
								size_t *pStructSize,size_t *pStringSize)
{
	*pStructSize+=sizeof(TVTest::FavoriteItemInfo);
	*pStringSize+=::lstrlen(pItem->GetName())+1;

	if (pItem->GetType()==TVTest::CFavoriteItem::ITEM_FOLDER) {
		const TVTest::CFavoriteFolder *pFolder=
			static_cast<const TVTest::CFavoriteFolder*>(pItem);

		for (size_t i=0;i<pFolder->GetItemCount();i++)
			GetFavoriteItemSize(pFolder->GetItem(i),pStructSize,pStringSize);
	} else if (pItem->GetType()==TVTest::CFavoriteItem::ITEM_CHANNEL) {
		const TVTest::CFavoriteChannel *pChannel=
			static_cast<const TVTest::CFavoriteChannel*>(pItem);

		*pStringSize+=::lstrlen(pChannel->GetBonDriverFileName())+1;
	}
}


static void GetFavoriteItemInfo(const TVTest::CFavoriteItem *pItem,
								TVTest::FavoriteItemInfo **ppItemInfo,
								LPWSTR *ppStringBuffer)
{
	TVTest::FavoriteItemInfo *pItemInfo=*ppItemInfo;
	LPWSTR pStringBuffer=*ppStringBuffer;
	size_t Length;

	::ZeroMemory(pItemInfo,sizeof(TVTest::FavoriteItemInfo));
	pItemInfo->pszName=pStringBuffer;
	Length=::lstrlen(pItem->GetName())+1;
	::CopyMemory(pStringBuffer,pItem->GetName(),Length*sizeof(WCHAR));
	pStringBuffer+=Length;

	++*ppItemInfo;

	if (pItem->GetType()==TVTest::CFavoriteItem::ITEM_FOLDER) {
		const TVTest::CFavoriteFolder *pFolder=
			static_cast<const TVTest::CFavoriteFolder*>(pItem);

		pItemInfo->Type=TVTest::FAVORITE_ITEM_TYPE_FOLDER;
		pItemInfo->Folder.ItemCount=static_cast<DWORD>(pFolder->GetItemCount());
		if (pFolder->GetItemCount()>0) {
			pItemInfo->Folder.ItemList=*ppItemInfo;
			for (size_t i=0;i<pFolder->GetItemCount();i++)
				GetFavoriteItemInfo(pFolder->GetItem(i),ppItemInfo,&pStringBuffer);
		}
	} else if (pItem->GetType()==TVTest::CFavoriteItem::ITEM_CHANNEL) {
		const TVTest::CFavoriteChannel *pChannel=
			static_cast<const TVTest::CFavoriteChannel*>(pItem);
		const CChannelInfo &ChannelInfo=pChannel->GetChannelInfo();

		pItemInfo->Type=TVTest::FAVORITE_ITEM_TYPE_CHANNEL;
		if (pChannel->GetForceBonDriverChange())
			pItemInfo->Channel.Flags|=TVTest::FAVORITE_CHANNEL_FLAG_FORCETUNERCHANGE;
		pItemInfo->Channel.Space=ChannelInfo.GetSpace();
		pItemInfo->Channel.Channel=ChannelInfo.GetChannelIndex();
		pItemInfo->Channel.ChannelNo=ChannelInfo.GetChannelNo();
		pItemInfo->Channel.NetworkID=ChannelInfo.GetNetworkID();
		pItemInfo->Channel.TransportStreamID=ChannelInfo.GetTransportStreamID();
		pItemInfo->Channel.ServiceID=ChannelInfo.GetServiceID();
		pItemInfo->Channel.pszTuner=pStringBuffer;
		Length=::lstrlen(pChannel->GetBonDriverFileName())+1;
		::CopyMemory(pStringBuffer,pChannel->GetBonDriverFileName(),Length*sizeof(WCHAR));
		pStringBuffer+=Length;
	}

	*ppStringBuffer=pStringBuffer;
}


static bool GetFavoriteList(const TVTest::CFavoriteFolder &Folder,TVTest::FavoriteList *pList)
{
	pList->ItemCount=0;
	pList->ItemList=NULL;

	if (Folder.GetItemCount()==0)
		return true;

	size_t StructSize=0,StringSize=0;

	for (size_t i=0;i<Folder.GetItemCount();i++)
		GetFavoriteItemSize(Folder.GetItem(i),&StructSize,&StringSize);

	StringSize*=sizeof(WCHAR);
	BYTE *pBuffer=static_cast<BYTE*>(std::malloc(StructSize+StringSize));
	if (pBuffer==NULL)
		return false;
	pList->ItemList=pointer_cast<TVTest::FavoriteItemInfo*>(pBuffer);
	pList->ItemCount=static_cast<DWORD>(Folder.GetItemCount());

	TVTest::FavoriteItemInfo *pItemInfo=pList->ItemList;
	LPWSTR pStringBuffer=pointer_cast<LPWSTR>(pBuffer+StructSize);

	for (size_t i=0;i<Folder.GetItemCount();i++)
		GetFavoriteItemInfo(Folder.GetItem(i),&pItemInfo,&pStringBuffer);

	_ASSERT(((BYTE*)pItemInfo-(BYTE*)pList->ItemList)==StructSize &&
			((BYTE*)pStringBuffer-(BYTE*)pItemInfo)==StringSize);

	return true;
}


static void FreeFavoriteList(TVTest::FavoriteList *pList)
{
	pList->ItemCount=0;
	if (pList->ItemList!=NULL) {
		std::free(pList->ItemList);
		pList->ItemList=NULL;
	}
}




HWND CPlugin::m_hwndMessage=NULL;
UINT CPlugin::m_MessageCode=0;

std::vector<CPlugin::CAudioStreamCallbackInfo> CPlugin::m_AudioStreamCallbackList;
CCriticalLock CPlugin::m_AudioStreamLock;


CPlugin::CPlugin()
	: m_hLib(NULL)
	, m_Version(0)
	, m_Type(0)
	, m_Flags(0)
	, m_fEnabled(false)
	, m_fSetting(false)
	, m_Command(0)
	, m_pEventCallback(NULL)
	, m_ProgramGuideEventFlags(0)
	, m_pMessageCallback(NULL)
{
}


CPlugin::~CPlugin()
{
	Free();
}


bool CPlugin::Load(LPCTSTR pszFileName)
{
	if (m_hLib!=NULL)
		Free();

	HMODULE hLib=::LoadLibrary(pszFileName);
	if (hLib==NULL) {
		const int ErrorCode=::GetLastError();
		TCHAR szText[256];

		StdUtil::snprintf(szText,lengthof(szText),
						  TEXT("DLLがロードできません。(エラーコード 0x%lx)"),ErrorCode);
		SetError(szText);
		switch (ErrorCode) {
		case ERROR_BAD_EXE_FORMAT:
			SetErrorAdvise(
#ifndef _WIN64
				TEXT("32")
#else
				TEXT("64")
#endif
				TEXT("ビット用のプラグインではないか、ファイルが破損している可能性があります。"));
			break;
		case ERROR_SXS_CANT_GEN_ACTCTX:
			SetErrorAdvise(TEXT("必要なランタイムがインストールされていない可能性があります。"));
			break;
		}
		return false;
	}
	TVTest::GetVersionFunc pGetVersion=
		reinterpret_cast<TVTest::GetVersionFunc>(::GetProcAddress(hLib,"TVTGetVersion"));
	if (pGetVersion==NULL) {
		::FreeLibrary(hLib);
		SetError(TEXT("TVTGetVersion()関数のアドレスを取得できません。"));
		return false;
	}
	m_Version=pGetVersion();
	if (TVTest::GetMajorVersion(m_Version)!=TVTest::GetMajorVersion(TVTEST_PLUGIN_VERSION)
		|| TVTest::GetMinorVersion(m_Version)!=TVTest::GetMinorVersion(TVTEST_PLUGIN_VERSION)) {
		::FreeLibrary(hLib);
		SetError(TEXT("対応していないバージョンです。"));
		return false;
	}
	TVTest::GetPluginInfoFunc pGetPluginInfo=
		reinterpret_cast<TVTest::GetPluginInfoFunc>(::GetProcAddress(hLib,"TVTGetPluginInfo"));
	if (pGetPluginInfo==NULL) {
		::FreeLibrary(hLib);
		SetError(TEXT("TVTGetPluginInfo()関数のアドレスを取得できません。"));
		return false;
	}
	TVTest::PluginInfo PluginInfo;
	::ZeroMemory(&PluginInfo,sizeof(PluginInfo));
	if (!pGetPluginInfo(&PluginInfo)
			|| IsStringEmpty(PluginInfo.pszPluginName)) {
		::FreeLibrary(hLib);
		SetError(TEXT("プラグインの情報を取得できません。"));
		return false;
	}
	TVTest::InitializeFunc pInitialize=
		reinterpret_cast<TVTest::InitializeFunc>(::GetProcAddress(hLib,"TVTInitialize"));
	if (pInitialize==NULL) {
		::FreeLibrary(hLib);
		SetError(TEXT("TVTInitialize()関数のアドレスを取得できません。"));
		return false;
	}
	m_FileName=pszFileName;
	m_PluginName=PluginInfo.pszPluginName;
	TVTest::StringUtility::Assign(m_Copyright,PluginInfo.pszCopyright);
	TVTest::StringUtility::Assign(m_Description,PluginInfo.pszDescription);
	m_Type=PluginInfo.Type;
	m_Flags=PluginInfo.Flags;
	m_fEnabled=(m_Flags&TVTest::PLUGIN_FLAG_ENABLEDEFAULT)!=0;
	m_PluginParam.Callback=Callback;
	m_PluginParam.hwndApp=GetAppClass().UICore.GetMainWindow();
	m_PluginParam.pClientData=NULL;
	m_PluginParam.pInternalData=this;
	if (!pInitialize(&m_PluginParam)) {
		Free();
		::FreeLibrary(hLib);
		SetError(TEXT("プラグインの初期化でエラー発生しました。"));
		return false;
	}
	m_hLib=hLib;
	ClearError();
	return true;
}


void CPlugin::Free()
{
	CAppMain &App=GetAppClass();

	m_pEventCallback=NULL;
	m_ProgramGuideEventFlags=0;
	m_pMessageCallback=NULL;

	m_GrabberLock.Lock();
	if (!m_StreamGrabberList.empty()) {
		for (auto it=m_StreamGrabberList.begin();it!=m_StreamGrabberList.end();++it) {
			App.CoreEngine.m_DtvEngine.m_MediaGrabber.RemoveGrabber(*it);
			delete *it;
		}
		m_StreamGrabberList.clear();
	}
	m_GrabberLock.Unlock();

	m_AudioStreamLock.Lock();
	for (std::vector<CAudioStreamCallbackInfo>::iterator i=m_AudioStreamCallbackList.begin();
		 	i!=m_AudioStreamCallbackList.end();i++) {
		if (i->m_pPlugin==this) {
			m_AudioStreamCallbackList.erase(i);
			break;
		}
	}
	m_AudioStreamLock.Unlock();

	m_CommandList.clear();
	m_ProgramGuideCommandList.clear();

	for (size_t i=0;i<m_ControllerList.size();i++)
		App.ControllerManager.DeleteController(m_ControllerList[i].c_str());
	m_ControllerList.clear();

	if (m_hLib!=NULL) {
		LPCTSTR pszFileName=::PathFindFileName(m_FileName.c_str());

		App.AddLog(TEXT("%s の終了処理を行っています..."),pszFileName);

		TVTest::FinalizeFunc pFinalize=
			reinterpret_cast<TVTest::FinalizeFunc>(::GetProcAddress(m_hLib,"TVTFinalize"));
		if (pFinalize==NULL) {
			App.AddLog(CLogItem::TYPE_ERROR,
					   TEXT("%s のTVTFinalize()関数のアドレスを取得できません。"),
					   pszFileName);
		} else {
			pFinalize();
		}
		::FreeLibrary(m_hLib);
		m_hLib=NULL;
		App.AddLog(TEXT("%s を解放しました。"),pszFileName);
	}

	for (auto itr=m_StatusItemList.begin();itr!=m_StatusItemList.end();++itr) {
		StatusItem *pItem=*itr;
		if (pItem->pItem!=NULL)
			pItem->pItem->DetachItem();
		delete pItem;
	}
	m_StatusItemList.clear();

	for (auto itr=m_PanelItemList.begin();itr!=m_PanelItemList.end();++itr) {
		PanelItem *pItem=*itr;
		if (pItem->pItem!=NULL)
			pItem->pItem->DetachItem();
		delete pItem;
	}
	m_PanelItemList.clear();

	m_FileName.clear();
	m_PluginName.clear();
	m_Copyright.clear();
	m_Description.clear();
	m_fEnabled=false;
	m_fSetting=false;
	m_PluginParam.pInternalData=NULL;
}


bool CPlugin::Enable(bool fEnable)
{
	if ((m_Flags & TVTest::PLUGIN_FLAG_NOENABLEDDISABLED)!=0)
		return false;

	if (m_fEnabled!=fEnable) {
		if (m_fSetting)
			return false;
		m_fSetting=true;
		bool fResult=SendEvent(TVTest::EVENT_PLUGINENABLE,(LPARAM)fEnable)!=0;
		m_fSetting=false;
		if (!fResult)
			return false;
		m_fEnabled=fEnable;

		if (m_Command>0) {
			GetAppClass().CommandList.SetCommandStateByID(
				m_Command,
				CCommandList::COMMAND_STATE_CHECKED,
				fEnable?CCommandList::COMMAND_STATE_CHECKED:0);
		}

		if (fEnable) {
			for (size_t i=0;i<m_ControllerList.size();i++)
				GetAppClass().ControllerManager.LoadControllerSettings(m_ControllerList[i].c_str());
		}
	}
	return true;
}


bool CPlugin::SetCommand(int Command)
{
	if (Command<CM_PLUGIN_FIRST || Command>CM_PLUGIN_LAST)
		return false;
	m_Command=Command;
	return true;
}


int CPlugin::NumPluginCommands() const
{
	return (int)m_CommandList.size();
}


int CPlugin::ParsePluginCommand(LPCWSTR pszCommand) const
{
	if (IsStringEmpty(pszCommand))
		return -1;

	for (size_t i=0;i<m_CommandList.size();i++) {
		if (::lstrcmpi(m_CommandList[i].GetText(),pszCommand)==0)
			return (int)i;
	}

	return -1;
}


CPlugin::CPluginCommandInfo *CPlugin::GetPluginCommandInfo(int Index)
{
	if (Index<0 || (size_t)Index>=m_CommandList.size())
		return NULL;
	return &m_CommandList[Index];
}


const CPlugin::CPluginCommandInfo *CPlugin::GetPluginCommandInfo(int Index) const
{
	if (Index<0 || (size_t)Index>=m_CommandList.size())
		return NULL;
	return &m_CommandList[Index];
}


CPlugin::CPluginCommandInfo *CPlugin::GetPluginCommandInfo(LPCWSTR pszCommand)
{
	int Index=ParsePluginCommand(pszCommand);
	if (Index<0)
		return NULL;
	return &m_CommandList[Index];
}


const CPlugin::CPluginCommandInfo *CPlugin::GetPluginCommandInfo(LPCWSTR pszCommand) const
{
	int Index=ParsePluginCommand(pszCommand);
	if (Index<0)
		return NULL;
	return &m_CommandList[Index];
}


bool CPlugin::GetPluginCommandInfo(int Index,TVTest::CommandInfo *pInfo) const
{
	if (Index<0 || (size_t)Index>=m_CommandList.size() || pInfo==NULL)
		return false;
	const CPluginCommandInfo &Command=m_CommandList[Index];
	pInfo->ID=Command.GetID();
	pInfo->pszText=Command.GetText();
	pInfo->pszName=Command.GetName();
	return true;
}


bool CPlugin::NotifyCommand(LPCWSTR pszCommand)
{
	int i=ParsePluginCommand(pszCommand);
	if (i<0)
		return false;
	return SendEvent(TVTest::EVENT_COMMAND,m_CommandList[i].GetID(),0)!=FALSE;
}


bool CPlugin::DrawPluginCommandIcon(const TVTest::DrawCommandIconInfo *pInfo)
{
	if (pInfo==NULL)
		return false;
	return SendEvent(TVTest::EVENT_DRAWCOMMANDICON,reinterpret_cast<LPARAM>(pInfo),0)!=FALSE;
}


int CPlugin::NumProgramGuideCommands() const
{
	return (int)m_ProgramGuideCommandList.size();
}


bool CPlugin::GetProgramGuideCommandInfo(int Index,TVTest::ProgramGuideCommandInfo *pInfo) const
{
	if (Index<0 || (size_t)Index>=m_ProgramGuideCommandList.size())
		return false;
	const CProgramGuideCommand &Command=m_ProgramGuideCommandList[Index];
	pInfo->Type=Command.GetType();
	pInfo->Flags=0;
	pInfo->ID=Command.GetID();
	pInfo->pszText=Command.GetText();
	pInfo->pszName=Command.GetName();
	return true;
}


bool CPlugin::NotifyProgramGuideCommand(LPCTSTR pszCommand,UINT Action,const CEventInfoData *pEvent,
										const POINT *pCursorPos,const RECT *pItemRect)
{
	for (size_t i=0;i<m_ProgramGuideCommandList.size();i++) {
		if (::lstrcmpi(m_ProgramGuideCommandList[i].GetText(),pszCommand)==0) {
			TVTest::ProgramGuideCommandParam Param;

			Param.ID=m_ProgramGuideCommandList[i].GetID();
			Param.Action=Action;
			if (pEvent!=NULL)
				EventInfoToProgramGuideProgramInfo(*pEvent,&Param.Program);
			else
				::ZeroMemory(&Param.Program,sizeof(Param.Program));
			if (pCursorPos!=NULL)
				Param.CursorPos=*pCursorPos;
			else
				::GetCursorPos(&Param.CursorPos);
			if (pItemRect!=NULL)
				Param.ItemRect=*pItemRect;
			else
				::SetRectEmpty(&Param.ItemRect);
			return SendEvent(TVTest::EVENT_PROGRAMGUIDE_COMMAND,
							 Param.ID,reinterpret_cast<LPARAM>(&Param))!=0;
		}
	}
	return false;
}


bool CPlugin::IsDisableOnStart() const
{
	return (m_Flags&TVTest::PLUGIN_FLAG_DISABLEONSTART)!=0;
}


void CPlugin::RegisterStatusItems()
{
	CAppMain &App=GetAppClass();

	for (auto itr=m_StatusItemList.begin();itr!=m_StatusItemList.end();++itr) {
		StatusItem *pItem=*itr;
		TVTest::String IDText;

		IDText=::PathFindFileName(GetFileName());
		IDText+=_T(':');
		IDText+=pItem->IDText;

		int ItemID=App.StatusOptions.RegisterItem(IDText.c_str());
		if (ItemID>=0) {
			pItem->ItemID=ItemID;
			App.StatusView.AddItem(new CPluginStatusItem(this,pItem));
		}
	}
}


void CPlugin::SendStatusItemCreatedEvent()
{
	for (auto itr=m_StatusItemList.begin();itr!=m_StatusItemList.end();++itr) {
		const StatusItem *pItem=*itr;

		if (pItem->pItem!=NULL) {
			TVTest::StatusItemEventInfo Info;

			Info.ID=pItem->ID;
			Info.Event=TVTest::STATUS_ITEM_EVENT_CREATED;
			Info.Param=0;

			SendEvent(TVTest::EVENT_STATUSITEM_NOTIFY,
					  reinterpret_cast<LPARAM>(&Info),0);
		}
	}
}


void CPlugin::SendStatusItemUpdateTimerEvent()
{
	for (auto itr=m_StatusItemList.begin();itr!=m_StatusItemList.end();++itr) {
		const StatusItem *pItem=*itr;

		if ((pItem->Flags & TVTest::STATUS_ITEM_FLAG_TIMERUPDATE)!=0
				&& pItem->pItem!=NULL) {
			TVTest::StatusItemEventInfo Info;

			Info.ID=pItem->ID;
			Info.Event=TVTest::STATUS_ITEM_EVENT_UPDATETIMER;
			Info.Param=0;

			if (SendEvent(TVTest::EVENT_STATUSITEM_NOTIFY,
						  reinterpret_cast<LPARAM>(&Info),0)!=FALSE) {
				pItem->pItem->Redraw();
			}
		}
	}
}


void CPlugin::RegisterPanelItems()
{
	CAppMain &App=GetAppClass();

	for (auto itr=m_PanelItemList.begin();itr!=m_PanelItemList.end();++itr) {
		PanelItem *pItem=*itr;
		TVTest::String IDText;

		IDText=::PathFindFileName(GetFileName());
		IDText+=_T(':');
		IDText+=pItem->IDText;

		int ItemID=App.PanelOptions.RegisterPanelItem(IDText.c_str(),pItem->Title.c_str());
		if (ItemID>=0) {
			pItem->ItemID=ItemID;
			if ((pItem->StateMask & TVTest::PANEL_ITEM_STATE_ENABLED)!=0) {
				App.PanelOptions.SetPanelItemVisibility(
					ItemID,(pItem->State & TVTest::PANEL_ITEM_STATE_ENABLED)!=0);
			} else {
				if (App.PanelOptions.GetPanelItemVisibility(ItemID))
					pItem->State|=TVTest::PANEL_ITEM_STATE_ENABLED;
				else
					pItem->State&=~TVTest::PANEL_ITEM_STATE_ENABLED;
			}
			CPluginPanelItem *pPanelItem=new CPluginPanelItem(this,pItem);
			pPanelItem->Create(App.Panel.Form.GetHandle(),WS_CHILD | WS_CLIPCHILDREN);
			CPanelForm::PageInfo PageInfo;
			PageInfo.pPage=pPanelItem;
			PageInfo.pszTitle=pItem->Title.c_str();
			PageInfo.ID=ItemID;
			PageInfo.Icon=-1;
			PageInfo.fVisible=(pItem->State & TVTest::PANEL_ITEM_STATE_ENABLED)!=0;
			App.Panel.Form.AddPage(PageInfo);
		}
	}
}


LRESULT CPlugin::SendEvent(UINT Event,LPARAM lParam1,LPARAM lParam2)
{
	if (m_pEventCallback!=NULL)
		return m_pEventCallback(Event,lParam1,lParam2,m_pEventCallbackClientData);
	return 0;
}


bool CPlugin::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,LRESULT *pResult)
{
	if (m_pMessageCallback!=NULL)
		return m_pMessageCallback(hwnd,uMsg,wParam,lParam,pResult,m_pMessageCallbackClientData)!=FALSE;
	return false;
}


bool CPlugin::Settings(HWND hwndOwner)
{
	if ((m_Flags&TVTest::PLUGIN_FLAG_HASSETTINGS)==0)
		return false;
	if (m_fSetting)
		return true;
	m_fSetting=true;
	bool fResult=SendEvent(TVTest::EVENT_PLUGINSETTINGS,(LPARAM)hwndOwner)!=0;
	m_fSetting=false;
	return fResult;
}


LRESULT CALLBACK CPlugin::Callback(TVTest::PluginParam *pParam,UINT Message,LPARAM lParam1,LPARAM lParam2)
{
	if (pParam==NULL)
		return 0;

	CPlugin *pPlugin=static_cast<CPlugin*>(pParam->pInternalData);
	if (pPlugin==NULL)
		return 0;

	return pPlugin->OnCallback(pParam,Message,lParam1,lParam2);
}


LRESULT CPlugin::SendPluginMessage(TVTest::PluginParam *pParam,UINT Message,LPARAM lParam1,LPARAM lParam2,
								   LRESULT FailedResult)
{
	PluginMessageParam MessageParam;
	DWORD_PTR Result;

	MessageParam.pPlugin=this;
	if (MessageParam.pPlugin==NULL)
		return FailedResult;
	MessageParam.Message=Message;
	MessageParam.lParam1=lParam1;
	MessageParam.lParam2=lParam2;
	if (::SendMessageTimeout(m_hwndMessage,m_MessageCode,
							 Message,reinterpret_cast<LPARAM>(&MessageParam),
							 SMTO_NORMAL,10000,&Result))
		return Result;
	GetAppClass().AddLog(
		CLogItem::TYPE_ERROR,
		TEXT("応答が無いためプラグインからのメッセージを処理できません。(%s : %u)"),
		::PathFindFileName(MessageParam.pPlugin->m_FileName.c_str()),Message);
	return FailedResult;
}


LRESULT CPlugin::OnCallback(TVTest::PluginParam *pParam,UINT Message,LPARAM lParam1,LPARAM lParam2)
{
	switch (Message) {
	case TVTest::MESSAGE_GETVERSION:
		return TVTest::MakeVersion(VERSION_MAJOR,VERSION_MINOR,VERSION_BUILD);

	case TVTest::MESSAGE_QUERYMESSAGE:
		if (lParam1<0 || lParam1>=TVTest::MESSAGE_TRAILER)
			return FALSE;
		if (lParam1==TVTest::MESSAGE_GETBCASINFO
				|| lParam1==TVTest::MESSAGE_SENDBCASCOMMAND)
			return FALSE;
		return TRUE;

	case TVTest::MESSAGE_MEMORYALLOC:
		{
			void *pData=reinterpret_cast<void*>(lParam1);
			SIZE_T Size=lParam2;

			if (Size>0) {
				return (LRESULT)realloc(pData,Size);
			} else if (pData!=NULL) {
				free(pData);
			}
		}
		return (LRESULT)(void*)0;

	case TVTest::MESSAGE_SETEVENTCALLBACK:
		m_pEventCallback=reinterpret_cast<TVTest::EventCallbackFunc>(lParam1);
		m_pEventCallbackClientData=reinterpret_cast<void*>(lParam2);
		return TRUE;

	case TVTest::MESSAGE_GETCURRENTCHANNELINFO:
	case TVTest::MESSAGE_SETCHANNEL:
		return SendPluginMessage(pParam,Message,lParam1,lParam2);

	case TVTest::MESSAGE_GETSERVICE:
		{
			CDtvEngine &DtvEngine=GetAppClass().CoreEngine.m_DtvEngine;
			int *pNumServices=reinterpret_cast<int*>(lParam1);
			WORD ServiceID;

			if (pNumServices)
				*pNumServices=DtvEngine.m_TsAnalyzer.GetViewableServiceNum();
			if (!DtvEngine.GetServiceID(&ServiceID))
				return -1;
			return DtvEngine.m_TsAnalyzer.GetViewableServiceIndexByID(ServiceID);;
		}

	case TVTest::MESSAGE_SETSERVICE:
	case TVTest::MESSAGE_GETTUNINGSPACENAME:
	case TVTest::MESSAGE_GETCHANNELINFO:
		return SendPluginMessage(pParam,Message,lParam1,lParam2);

	case TVTest::MESSAGE_GETSERVICEINFO:
		{
			int Index=(int)lParam1;
			TVTest::ServiceInfo *pServiceInfo=reinterpret_cast<TVTest::ServiceInfo*>(lParam2);

			if (Index<0 || pServiceInfo==NULL
					|| (pServiceInfo->Size!=sizeof(TVTest::ServiceInfo)
						&& pServiceInfo->Size!=TVTest::SERVICEINFO_SIZE_V1))
				return FALSE;
			CTsAnalyzer *pTsAnalyzer=&GetAppClass().CoreEngine.m_DtvEngine.m_TsAnalyzer;
			WORD ServiceID;
			CTsAnalyzer::ServiceInfo Info;
			if (!pTsAnalyzer->GetViewableServiceID(Index,&ServiceID)
					|| !pTsAnalyzer->GetServiceInfo(pTsAnalyzer->GetServiceIndexByID(ServiceID),&Info))
				return FALSE;
			pServiceInfo->ServiceID=ServiceID;
			pServiceInfo->VideoPID=
				Info.VideoEsList.empty()?CTsAnalyzer::PID_INVALID:Info.VideoEsList[0].PID;
			pServiceInfo->NumAudioPIDs=(int)Info.AudioEsList.size();
			for (size_t i=0;i<Info.AudioEsList.size();i++)
				pServiceInfo->AudioPID[i]=Info.AudioEsList[i].PID;
			::lstrcpyn(pServiceInfo->szServiceName,Info.szServiceName,32);
			if (pServiceInfo->Size==sizeof(TVTest::ServiceInfo)) {
				int ServiceIndex=pTsAnalyzer->GetServiceIndexByID(ServiceID);
				for (size_t i=0;i<Info.AudioEsList.size();i++) {
					pServiceInfo->AudioComponentType[i]=
						pTsAnalyzer->GetAudioComponentType(ServiceIndex,(int)i);
				}
				if (Info.CaptionEsList.size()>0)
					pServiceInfo->SubtitlePID=Info.CaptionEsList[0].PID;
				else
					pServiceInfo->SubtitlePID=0;
				pServiceInfo->Reserved=0;
			}
		}
		return TRUE;

	case TVTest::MESSAGE_GETDRIVERNAME:
	case TVTest::MESSAGE_SETDRIVERNAME:

	case TVTest::MESSAGE_STARTRECORD:
	case TVTest::MESSAGE_STOPRECORD:
	case TVTest::MESSAGE_PAUSERECORD:
	case TVTest::MESSAGE_GETRECORD:
	case TVTest::MESSAGE_MODIFYRECORD:

	case TVTest::MESSAGE_GETZOOM:
	case TVTest::MESSAGE_SETZOOM:
		return SendPluginMessage(pParam,Message,lParam1,lParam2);

	case TVTest::MESSAGE_GETPANSCAN:
		{
			TVTest::PanScanInfo *pInfo=reinterpret_cast<TVTest::PanScanInfo*>(lParam1);

			if (pInfo==NULL || pInfo->Size!=sizeof(TVTest::PanScanInfo))
				return FALSE;

			CCoreEngine::PanAndScanInfo PanScan;
			if (!GetAppClass().UICore.GetPanAndScan(&PanScan))
				return FALSE;

			pInfo->Type=TVTest::PANSCAN_NONE;
			if (PanScan.Width<PanScan.XFactor) {
				if (PanScan.Height<PanScan.YFactor)
					pInfo->Type=TVTest::PANSCAN_SUPERFRAME;
				else
					pInfo->Type=TVTest::PANSCAN_SIDECUT;
			} else if (PanScan.Height<PanScan.YFactor) {
				pInfo->Type=TVTest::PANSCAN_LETTERBOX;
			}
		}
		return TRUE;

	case TVTest::MESSAGE_SETPANSCAN:
		{
			const TVTest::PanScanInfo *pInfo=reinterpret_cast<const TVTest::PanScanInfo*>(lParam1);

			if (pInfo==NULL || pInfo->Size!=sizeof(TVTest::PanScanInfo))
				return FALSE;
			const CMediaViewer *pMediaViewer=&GetAppClass().CoreEngine.m_DtvEngine.m_MediaViewer;
			int Command;
			switch (pInfo->Type) {
			case TVTest::PANSCAN_NONE:
				if (pInfo->XAspect==0 && pInfo->YAspect==0)
					Command=CM_ASPECTRATIO_DEFAULT;
				else if (pInfo->XAspect==16 && pInfo->YAspect==9)
					Command=CM_ASPECTRATIO_16x9;
				else if (pInfo->XAspect==4 && pInfo->YAspect==3)
					Command=CM_ASPECTRATIO_4x3;
				else
					return FALSE;
				break;
			case TVTest::PANSCAN_LETTERBOX:
				if (pInfo->XAspect==16 && pInfo->YAspect==9)
					Command=CM_ASPECTRATIO_LETTERBOX;
				else
					return FALSE;
				break;
			case TVTest::PANSCAN_SIDECUT:
				if (pInfo->XAspect==4 && pInfo->YAspect==3)
					Command=CM_ASPECTRATIO_SIDECUT;
				else
					return FALSE;
				break;
			case TVTest::PANSCAN_SUPERFRAME:
				if (pInfo->XAspect==16 && pInfo->YAspect==9)
					Command=CM_ASPECTRATIO_SUPERFRAME;
				else
					return FALSE;
				break;
			default:
				return FALSE;
			}
			GetAppClass().UICore.DoCommand(Command);
		}
		return TRUE;

	case TVTest::MESSAGE_GETSTATUS:
		{
			TVTest::StatusInfo *pInfo=reinterpret_cast<TVTest::StatusInfo*>(lParam1);

			if (pInfo==NULL || (pInfo->Size!=sizeof(TVTest::StatusInfo)
								&& pInfo->Size!=TVTest::STATUSINFO_SIZE_V1))
				return FALSE;
			const CCoreEngine *pCoreEngine=&GetAppClass().CoreEngine;
			ULONGLONG DropCount=pCoreEngine->GetContinuityErrorPacketCount();
			pInfo->SignalLevel=pCoreEngine->GetSignalLevel();
			pInfo->BitRate=pCoreEngine->GetBitRate();
			pInfo->ErrorPacketCount=(DWORD)(pCoreEngine->GetErrorPacketCount()+DropCount);
			pInfo->ScramblePacketCount=(DWORD)pCoreEngine->GetScramblePacketCount();
			if (pInfo->Size==sizeof(TVTest::StatusInfo)) {
				pInfo->DropPacketCount=(DWORD)DropCount;
				pInfo->BcasCardStatus=TVTest::BCAS_STATUS_OK;	// 非対応
			}
		}
		return TRUE;

	case TVTest::MESSAGE_GETRECORDSTATUS:
		return SendPluginMessage(pParam,Message,lParam1,lParam2);

	case TVTest::MESSAGE_GETVIDEOINFO:
		{
			TVTest::VideoInfo *pVideoInfo=reinterpret_cast<TVTest::VideoInfo*>(lParam1);
			CMediaViewer *pMediaViewer;
			WORD VideoWidth,VideoHeight;
			BYTE XAspect,YAspect;

			if (pVideoInfo==NULL || pVideoInfo->Size!=sizeof(TVTest::VideoInfo))
				return FALSE;
			pMediaViewer=&GetAppClass().CoreEngine.m_DtvEngine.m_MediaViewer;
			if (pMediaViewer->GetOriginalVideoSize(&VideoWidth,&VideoHeight)
					&& pMediaViewer->GetVideoAspectRatio(&XAspect,&YAspect)
					&& pMediaViewer->GetSourceRect(&pVideoInfo->SourceRect)) {
				pVideoInfo->Width=VideoWidth;
				pVideoInfo->Height=VideoHeight;
				pVideoInfo->XAspect=XAspect;
				pVideoInfo->YAspect=YAspect;
				return TRUE;
			}
		}
		return FALSE;

	case TVTest::MESSAGE_GETVOLUME:
		{
			const CUICore &UICore=GetAppClass().UICore;
			int Volume=UICore.GetVolume();
			bool fMute=UICore.GetMute();

			return MAKELONG(Volume,fMute);
		}

	case TVTest::MESSAGE_SETVOLUME:
		return SendPluginMessage(pParam,Message,lParam1,lParam2);

	case TVTest::MESSAGE_GETSTEREOMODE:
#if 0	// ver.0.9.0 より前
		return GetAppClass().UICore.GetStereoMode();
#else
		{
			int StereoMode;

			switch (GetAppClass().UICore.GetActualDualMonoMode()) {
			case CAudioDecFilter::DUALMONO_MAIN:
				StereoMode=TVTest::STEREOMODE_LEFT;
				break;
			case CAudioDecFilter::DUALMONO_SUB:
				StereoMode=TVTest::STEREOMODE_RIGHT;
				break;
			case CAudioDecFilter::DUALMONO_BOTH:
			default:
				StereoMode=TVTest::STEREOMODE_STEREO;
				break;
			}

			return StereoMode;
		}
#endif

	case TVTest::MESSAGE_SETSTEREOMODE:
		return SendPluginMessage(pParam,Message,lParam1,lParam2);

	case TVTest::MESSAGE_GETFULLSCREEN:
		return GetAppClass().UICore.GetFullscreen();

	case TVTest::MESSAGE_SETFULLSCREEN:
		return SendPluginMessage(pParam,Message,lParam1,lParam2);

	case TVTest::MESSAGE_GETPREVIEW:
		return GetAppClass().UICore.IsViewerEnabled();

	case TVTest::MESSAGE_SETPREVIEW:
		return SendPluginMessage(pParam,Message,lParam1,lParam2);

	case TVTest::MESSAGE_GETSTANDBY:
		return GetAppClass().UICore.GetStandby();

	case TVTest::MESSAGE_SETSTANDBY:
		return SendPluginMessage(pParam,Message,lParam1,lParam2);

	case TVTest::MESSAGE_GETALWAYSONTOP:
		return GetAppClass().UICore.GetAlwaysOnTop();

	case TVTest::MESSAGE_SETALWAYSONTOP:
		return SendPluginMessage(pParam,Message,lParam1,lParam2);

	case TVTest::MESSAGE_CAPTUREIMAGE:
		{
			void *pBuffer=GetAppClass().CoreEngine.GetCurrentImage();

			if (pBuffer!=NULL) {
				SIZE_T Size=CalcDIBSize(static_cast<BITMAPINFOHEADER*>(pBuffer));
				void *pDib;

				pDib=malloc(Size);
				if (pDib!=NULL)
					::CopyMemory(pDib,pBuffer,Size);
				::CoTaskMemFree(pBuffer);
				return (LRESULT)pDib;
			}
		}
		return (LRESULT)(LPVOID)NULL;

	case TVTest::MESSAGE_SAVEIMAGE:
		GetAppClass().UICore.DoCommand(CM_SAVEIMAGE);
		return TRUE;

	case TVTest::MESSAGE_RESET:
		{
			DWORD Flags=(DWORD)lParam1;

			if (Flags==TVTest::RESET_ALL)
				GetAppClass().UICore.DoCommand(CM_RESET);
			else if (Flags==TVTest::RESET_VIEWER)
				GetAppClass().UICore.DoCommand(CM_RESETVIEWER);
			else
				return FALSE;
		}
		return TRUE;

	case TVTest::MESSAGE_CLOSE:
		::PostMessage(GetAppClass().UICore.GetMainWindow(),
					  WM_COMMAND,
					  (lParam1&TVTest::CLOSE_EXIT)!=0?CM_EXIT:CM_CLOSE,0);
		return TRUE;

	case TVTest::MESSAGE_SETSTREAMCALLBACK:
		{
			TVTest::StreamCallbackInfo *pInfo=reinterpret_cast<TVTest::StreamCallbackInfo*>(lParam1);

			if (pInfo==NULL
					|| pInfo->Size!=sizeof(TVTest::StreamCallbackInfo)
					|| pInfo->Callback==NULL)
				return FALSE;

			CBlockLock Lock(&m_GrabberLock);

			if ((pInfo->Flags & TVTest::STREAM_CALLBACK_REMOVE)==0) {
				// コールバック登録
				if (!m_StreamGrabberList.empty()) {
					for (auto it=m_StreamGrabberList.begin();it!=m_StreamGrabberList.end();++it) {
						CStreamGrabber *pGrabber=*it;
						if (pGrabber->GetCallbackFunc()==pInfo->Callback) {
							pGrabber->SetClientData(pInfo->pClientData);
							return TRUE;
						}
					}
				}
				CStreamGrabber *pGrabber=new CStreamGrabber(pInfo->Callback,pInfo->pClientData);
				m_StreamGrabberList.push_back(pGrabber);
				GetAppClass().CoreEngine.m_DtvEngine.m_MediaGrabber.AddGrabber(pGrabber);
			} else {
				// コールバック削除
				for (auto it=m_StreamGrabberList.begin();it!=m_StreamGrabberList.end();++it) {
					CStreamGrabber *pGrabber=*it;
					if (pGrabber->GetCallbackFunc()==pInfo->Callback) {
						GetAppClass().CoreEngine.m_DtvEngine.m_MediaGrabber.RemoveGrabber(pGrabber);
						m_StreamGrabberList.erase(it);
						delete pGrabber;
						return TRUE;
					}
				}
				return FALSE;
			}
		}
		return TRUE;

	case TVTest::MESSAGE_ENABLEPLUGIN:
		return Enable(lParam1!=0);

	case TVTest::MESSAGE_GETCOLOR:
		{
			LPCWSTR pszName=reinterpret_cast<LPCWSTR>(lParam1);

			if (pszName==NULL)
				return CLR_INVALID;
			return GetAppClass().UICore.GetColor(pszName);
		}

	case TVTest::MESSAGE_DECODEARIBSTRING:
		{
			TVTest::ARIBStringDecodeInfo *pInfo=reinterpret_cast<TVTest::ARIBStringDecodeInfo*>(lParam1);

			if (pInfo==NULL || pInfo->Size!=sizeof(TVTest::ARIBStringDecodeInfo)
					|| pInfo->pSrcData==NULL
					|| pInfo->pszDest==NULL || pInfo->DestLength==0)
				return FALSE;
			if (CAribString::AribToString(pInfo->pszDest,pInfo->DestLength,
					static_cast<const BYTE*>(pInfo->pSrcData),pInfo->SrcLength)==0) {
				pInfo->pszDest[0]='\0';
			}
		}
		return TRUE;

	case TVTest::MESSAGE_GETCURRENTPROGRAMINFO:
		{
			TVTest::ProgramInfo *pProgramInfo=reinterpret_cast<TVTest::ProgramInfo*>(lParam1);

			if (pProgramInfo==NULL
					|| pProgramInfo->Size!=sizeof(TVTest::ProgramInfo))
				return FALSE;

			const bool fNext=lParam2!=0;
			CDtvEngine *pDtvEngine=&GetAppClass().CoreEngine.m_DtvEngine;

			pProgramInfo->ServiceID=0;
			pDtvEngine->GetServiceID(&pProgramInfo->ServiceID);
			pProgramInfo->EventID=pDtvEngine->GetEventID(fNext);
			if (pProgramInfo->pszEventName!=NULL && pProgramInfo->MaxEventName>0) {
				pProgramInfo->pszEventName[0]='\0';
				pDtvEngine->GetEventName(pProgramInfo->pszEventName,
										 pProgramInfo->MaxEventName,fNext);
			}
			if (pProgramInfo->pszEventText!=NULL && pProgramInfo->MaxEventText>0) {
				pProgramInfo->pszEventText[0]='\0';
				pDtvEngine->GetEventText(pProgramInfo->pszEventText,
										 pProgramInfo->MaxEventText,fNext);
			}
			if (pProgramInfo->pszEventExtText!=NULL && pProgramInfo->MaxEventExtText>0) {
				pProgramInfo->pszEventExtText[0]='\0';
				pDtvEngine->GetEventExtendedText(pProgramInfo->pszEventExtText,
												 pProgramInfo->MaxEventExtText,
												 fNext);
			}
			if (!pDtvEngine->GetEventTime(&pProgramInfo->StartTime,&pProgramInfo->Duration,fNext)) {
				::ZeroMemory(&pProgramInfo->StartTime,sizeof(SYSTEMTIME));
				pProgramInfo->Duration=0;
			}
		}
		return TRUE;

	case TVTest::MESSAGE_QUERYEVENT:
		return lParam1>=0 && lParam1<TVTest::EVENT_TRAILER;

	case TVTest::MESSAGE_GETTUNINGSPACE:
		{
			int *pNumSpaces=reinterpret_cast<int*>(lParam1);
			if (pNumSpaces!=NULL)
				*pNumSpaces=0;
		}
		return SendPluginMessage(pParam,Message,lParam1,lParam2,-1);

	case TVTest::MESSAGE_GETTUNINGSPACEINFO:
		return SendPluginMessage(pParam,Message,lParam1,lParam2,-1);

	case TVTest::MESSAGE_SETNEXTCHANNEL:
		GetAppClass().UICore.DoCommand((lParam1&1)!=0?CM_CHANNEL_UP:CM_CHANNEL_DOWN);
		return TRUE;

	case TVTest::MESSAGE_GETAUDIOSTREAM:
		return GetAppClass().UICore.GetAudioStream();

	case TVTest::MESSAGE_SETAUDIOSTREAM:
		return SendPluginMessage(pParam,Message,lParam1,lParam2);

	case TVTest::MESSAGE_ISPLUGINENABLED:
		return IsEnabled();

	case TVTest::MESSAGE_REGISTERCOMMAND:
		{
			const TVTest::CommandInfo *pCommandList=reinterpret_cast<TVTest::CommandInfo*>(lParam1);
			int NumCommands=(int)lParam2;

			if (pCommandList==NULL || NumCommands<=0)
				return FALSE;
			for (int i=0;i<NumCommands;i++) {
				m_CommandList.push_back(CPluginCommandInfo(pCommandList[i]));
			}
		}
		return TRUE;

	case TVTest::MESSAGE_ADDLOG:
		{
			LPCWSTR pszText=reinterpret_cast<LPCWSTR>(lParam1);
			if (pszText==NULL)
				return FALSE;

			LPCTSTR pszFileName=::PathFindFileName(m_FileName.c_str());
			GetAppClass().AddLog((CLogItem::LogType)lParam2,TEXT("%s : %s"),pszFileName,pszText);
		}
		return TRUE;

	case TVTest::MESSAGE_RESETSTATUS:
		GetAppClass().UICore.DoCommand(CM_RESETERRORCOUNT);
		return TRUE;

	case TVTest::MESSAGE_SETAUDIOCALLBACK:
		{
			TVTest::AudioCallbackFunc pCallback=reinterpret_cast<TVTest::AudioCallbackFunc>(lParam1);

			m_AudioStreamLock.Lock();
			if (pCallback!=NULL) {
				if (m_AudioStreamCallbackList.empty()) {
					GetAppClass().CoreEngine.m_DtvEngine.m_MediaViewer.SetAudioStreamCallback(AudioStreamCallback);
				} else {
					for (std::vector<CAudioStreamCallbackInfo>::iterator i=m_AudioStreamCallbackList.begin();
							i!=m_AudioStreamCallbackList.end();i++) {
						if (i->m_pPlugin==this) {
							m_AudioStreamCallbackList.erase(i);
							break;
						}
					}
				}
				m_AudioStreamCallbackList.push_back(CAudioStreamCallbackInfo(this,pCallback,reinterpret_cast<void*>(lParam2)));
				m_AudioStreamLock.Unlock();
			} else {
				bool fFound=false;
				for (std::vector<CAudioStreamCallbackInfo>::iterator i=m_AudioStreamCallbackList.begin();
						i!=m_AudioStreamCallbackList.end();i++) {
					if (i->m_pPlugin==this) {
						m_AudioStreamCallbackList.erase(i);
						fFound=true;
						break;
					}
				}
				m_AudioStreamLock.Unlock();
				if (!fFound)
					return FALSE;
				if (m_AudioStreamCallbackList.empty())
					GetAppClass().CoreEngine.m_DtvEngine.m_MediaViewer.SetAudioStreamCallback(NULL);
			}
		}
		return TRUE;

	case TVTest::MESSAGE_DOCOMMAND:
		{
			LPCWSTR pszCommand=reinterpret_cast<LPCWSTR>(lParam1);

			if (pszCommand==NULL || pszCommand[0]==L'\0')
				return FALSE;
			return GetAppClass().UICore.DoCommand(pszCommand);
		}

	case TVTest::MESSAGE_GETBCASINFO:
		// このメッセージは現在サポートされない
		{
			TVTest::BCasInfo *pBCasInfo=reinterpret_cast<TVTest::BCasInfo*>(lParam1);

			if (pBCasInfo==NULL || pBCasInfo->Size!=sizeof(TVTest::BCasInfo))
				return FALSE;

			::ZeroMemory(pBCasInfo,sizeof(TVTest::BCasInfo));
		}
		return FALSE;

	case TVTest::MESSAGE_SENDBCASCOMMAND:
		// このメッセージは現在サポートされない
		return FALSE;

	case TVTest::MESSAGE_GETHOSTINFO:
		{
			TVTest::HostInfo *pHostInfo=reinterpret_cast<TVTest::HostInfo*>(lParam1);

			if (pHostInfo==NULL || pHostInfo->Size!=sizeof(TVTest::HostInfo))
				return FALSE;
			pHostInfo->pszAppName=APP_NAME_W;
			pHostInfo->Version.Major=VERSION_MAJOR;
			pHostInfo->Version.Minor=VERSION_MINOR;
			pHostInfo->Version.Build=VERSION_BUILD;
			pHostInfo->pszVersionText=VERSION_TEXT_W;
			pHostInfo->SupportedPluginVersion=TVTEST_PLUGIN_VERSION;
		}
		return TRUE;

	case TVTest::MESSAGE_GETSETTING:
		return OnGetSetting(reinterpret_cast<TVTest::SettingInfo*>(lParam1));

	case TVTest::MESSAGE_GETDRIVERFULLPATHNAME:
		return SendPluginMessage(pParam,Message,lParam1,lParam2);

	case TVTest::MESSAGE_GETLOGO:
		{
			const WORD NetworkID=LOWORD(lParam1),ServiceID=HIWORD(lParam1);
			const BYTE LogoType=(BYTE)(lParam2&0xFF);
			HBITMAP hbm=GetAppClass().LogoManager.GetAssociatedLogoBitmap(NetworkID,ServiceID,LogoType);
			if (hbm!=NULL) {
				return reinterpret_cast<LRESULT>(::CopyImage(hbm,IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION));
			}
		}
		return reinterpret_cast<LRESULT>((HBITMAP)NULL);

	case TVTest::MESSAGE_GETAVAILABLELOGOTYPE:
		{
			const WORD NetworkID=LOWORD(lParam1),ServiceID=HIWORD(lParam1);

			return GetAppClass().LogoManager.GetAvailableLogoType(NetworkID,ServiceID);
		}

	case TVTest::MESSAGE_RELAYRECORD:
		return SendPluginMessage(pParam,Message,lParam1,lParam2);

	case TVTest::MESSAGE_SILENTMODE:
		if (lParam1==TVTest::SILENTMODE_GET) {
			return GetAppClass().Core.IsSilent();
		} else if (lParam1==TVTest::SILENTMODE_SET) {
			GetAppClass().Core.SetSilent(lParam2!=0);
			return TRUE;
		}
		return FALSE;

	case TVTest::MESSAGE_SETWINDOWMESSAGECALLBACK:
		return SendPluginMessage(pParam,Message,lParam1,lParam2);

	case TVTest::MESSAGE_REGISTERCONTROLLER:
		{
			const TVTest::ControllerInfo *pInfo=reinterpret_cast<const TVTest::ControllerInfo*>(lParam1);

			if (pInfo==NULL
					|| pInfo->Size!=sizeof(TVTest::ControllerInfo)
					|| pInfo->pszName==NULL || pInfo->pszName[0]=='\0'
					|| pInfo->pszText==NULL || pInfo->pszText[0]=='\0'
					|| pInfo->NumButtons<1
					|| pInfo->pButtonList==NULL)
				return FALSE;
			CControllerPlugin *pController=new CControllerPlugin(this,pInfo);
			CControllerManager *pControllerManager=&GetAppClass().ControllerManager;
			if (!pControllerManager->AddController(pController)) {
				delete pController;
				return FALSE;
			}
			m_ControllerList.push_back(TVTest::String(pInfo->pszName));
			if (m_fEnabled)
				pControllerManager->LoadControllerSettings(pInfo->pszName);
		}
		return TRUE;

	case TVTest::MESSAGE_ONCONTROLLERBUTTONDOWN:
		return GetAppClass().ControllerManager.OnButtonDown(
			reinterpret_cast<LPCWSTR>(lParam1),(int)lParam2);

	case TVTest::MESSAGE_GETCONTROLLERSETTINGS:
		{
			static const DWORD ValidMask=TVTest::CONTROLLER_SETTINGS_MASK_FLAGS;
			LPCWSTR pszName=reinterpret_cast<LPCWSTR>(lParam1);
			TVTest::ControllerSettings *pSettings=reinterpret_cast<TVTest::ControllerSettings*>(lParam2);

			if (pSettings==NULL
					|| (pSettings->Mask|ValidMask)!=ValidMask)
				return FALSE;
			const CControllerManager::ControllerSettings *pControllerSettings=
				GetAppClass().ControllerManager.GetControllerSettings(pszName);
			if (pControllerSettings==NULL)
				return FALSE;
			if ((pSettings->Mask&TVTest::CONTROLLER_SETTINGS_MASK_FLAGS)!=0) {
				pSettings->Flags=0;
				if (pControllerSettings->fActiveOnly)
					pSettings->Flags|=TVTest::CONTROLLER_SETTINGS_FLAG_ACTIVEONLY;
			}
		}
		return TRUE;

	case TVTest::MESSAGE_GETEPGEVENTINFO:
		{
			const TVTest::EpgEventQueryInfo *pQueryInfo=
				reinterpret_cast<const TVTest::EpgEventQueryInfo*>(lParam1);
			if (pQueryInfo==NULL)
				return reinterpret_cast<LRESULT>((TVTest::EpgEventInfo*)NULL);

			CEpgProgramList *pEpgProgramList=&GetAppClass().EpgProgramList;
			CEventInfoData EventData;
			if (pQueryInfo->Type==TVTest::EPG_EVENT_QUERY_EVENTID) {
				if (!pEpgProgramList->GetEventInfo(pQueryInfo->NetworkID,
												   pQueryInfo->TransportStreamID,
												   pQueryInfo->ServiceID,
												   pQueryInfo->EventID,
												   &EventData))
					return reinterpret_cast<LRESULT>((TVTest::EpgEventInfo*)NULL);
			} else if (pQueryInfo->Type==TVTest::EPG_EVENT_QUERY_TIME) {
				SYSTEMTIME stUTC,stEpg;

				if (!::FileTimeToSystemTime(&pQueryInfo->Time,&stUTC)
						|| !UtcToEpgTime(&stUTC,&stEpg))
					return reinterpret_cast<LRESULT>((TVTest::EpgEventInfo*)NULL);
				if (!pEpgProgramList->GetEventInfo(pQueryInfo->NetworkID,
												   pQueryInfo->TransportStreamID,
												   pQueryInfo->ServiceID,
												   &stEpg,
												   &EventData))
					return reinterpret_cast<LRESULT>((TVTest::EpgEventInfo*)NULL);
			} else {
				return reinterpret_cast<LRESULT>((TVTest::EpgEventInfo*)NULL);
			}

			CEpgDataConverter Converter;
			return reinterpret_cast<LRESULT>(Converter.Convert(EventData));
		}

	case TVTest::MESSAGE_FREEEPGEVENTINFO:
		{
			TVTest::EpgEventInfo *pEventInfo=reinterpret_cast<TVTest::EpgEventInfo*>(lParam1);

			if (pEventInfo!=NULL)
				CEpgDataConverter::FreeEventInfo(pEventInfo);
		}
		return TRUE;

	case TVTest::MESSAGE_GETEPGEVENTLIST:
		{
			TVTest::EpgEventList *pEventList=reinterpret_cast<TVTest::EpgEventList*>(lParam1);
			if (pEventList==NULL)
				return FALSE;

			pEventList->NumEvents=0;
			pEventList->EventList=NULL;

			CEpgProgramList *pEpgProgramList=&GetAppClass().EpgProgramList;
			pEpgProgramList->UpdateService(pEventList->NetworkID,
										   pEventList->TransportStreamID,
										   pEventList->ServiceID);
			const CEpgServiceInfo *pServiceInfo=
				pEpgProgramList->GetServiceInfo(pEventList->NetworkID,
												pEventList->TransportStreamID,
												pEventList->ServiceID);
			if (pServiceInfo==NULL || pServiceInfo->m_EventList.EventDataMap.size()==0)
				return FALSE;

			CEpgDataConverter Converter;
			pEventList->EventList=Converter.Convert(pServiceInfo->m_EventList);
			if (pEventList->EventList==NULL)
				return FALSE;
			pEventList->NumEvents=(WORD)pServiceInfo->m_EventList.EventDataMap.size();
		}
		return TRUE;

	case TVTest::MESSAGE_FREEEPGEVENTLIST:
		{
			TVTest::EpgEventList *pEventList=reinterpret_cast<TVTest::EpgEventList*>(lParam1);

			if (pEventList!=NULL) {
				CEpgDataConverter::FreeEventList(pEventList->EventList);
				pEventList->NumEvents=0;
				pEventList->EventList=NULL;
			}
		}
		return TRUE;

	case TVTest::MESSAGE_ENUMDRIVER:
		{
			LPWSTR pszFileName=reinterpret_cast<LPWSTR>(lParam1);
			const int Index=LOWORD(lParam2);
			const int MaxLength=HIWORD(lParam2);

			const CDriverManager *pDriverManager=&GetAppClass().DriverManager;
			const CDriverInfo *pDriverInfo=pDriverManager->GetDriverInfo(Index);
			if (pDriverInfo!=NULL) {
				if (pszFileName!=NULL)
					::lstrcpyn(pszFileName,pDriverInfo->GetFileName(),MaxLength);
				return ::lstrlen(pDriverInfo->GetFileName());
			}
		}
		return 0;

	case TVTest::MESSAGE_GETDRIVERTUNINGSPACELIST:
		{
			LPCWSTR pszDriverName=reinterpret_cast<LPCWSTR>(lParam1);
			TVTest::DriverTuningSpaceList *pList=
				reinterpret_cast<TVTest::DriverTuningSpaceList*>(lParam2);

			if (pszDriverName==NULL || pList==NULL)
				return FALSE;

			pList->NumSpaces=0;
			pList->SpaceList=NULL;

			CDriverInfo DriverInfo(pszDriverName);
			if (!DriverInfo.LoadTuningSpaceList())
				return FALSE;

			const CTuningSpaceList *pTuningSpaceList=DriverInfo.GetTuningSpaceList();
			if (pTuningSpaceList->NumSpaces()==0)
				return FALSE;

			const int NumSpaces=pTuningSpaceList->NumSpaces();
			SIZE_T BufferSize=NumSpaces*
				(sizeof(TVTest::DriverTuningSpaceInfo)+sizeof(TVTest::DriverTuningSpaceInfo*)+
				 sizeof(TVTest::TuningSpaceInfo));
			for (int i=0;i<NumSpaces;i++) {
				const CChannelList *pChannelList=pTuningSpaceList->GetChannelList(i);
				BufferSize+=pChannelList->NumChannels()*
					(sizeof(TVTest::ChannelInfo)+sizeof(TVTest::ChannelInfo*));
			}
			BYTE *pBuffer=(BYTE*)malloc(BufferSize);
			if (pBuffer==NULL)
				return FALSE;
			BYTE *p=pBuffer;
			pList->NumSpaces=NumSpaces;
			pList->SpaceList=(TVTest::DriverTuningSpaceInfo**)p;
			p+=NumSpaces*sizeof(TVTest::DriverTuningSpaceInfo*);
			for (int i=0;i<NumSpaces;i++) {
				const CTuningSpaceInfo *pSpaceInfo=pTuningSpaceList->GetTuningSpaceInfo(i);
				const CChannelList *pChannelList=pSpaceInfo->GetChannelList();
				const int NumChannels=pChannelList->NumChannels();
				TVTest::DriverTuningSpaceInfo *pDriverSpaceInfo=(TVTest::DriverTuningSpaceInfo*)p;

				p+=sizeof(TVTest::DriverTuningSpaceInfo);
				pList->SpaceList[i]=pDriverSpaceInfo;
				pDriverSpaceInfo->Flags=0;
				pDriverSpaceInfo->NumChannels=NumChannels;
				pDriverSpaceInfo->pInfo=(TVTest::TuningSpaceInfo*)p;
				pDriverSpaceInfo->pInfo->Size=sizeof(TVTest::TuningSpaceInfo);
				pDriverSpaceInfo->pInfo->Space=(int)pSpaceInfo->GetType();
				if (pSpaceInfo->GetName()!=NULL)
					::lstrcpyn(pDriverSpaceInfo->pInfo->szName,pSpaceInfo->GetName(),
							   lengthof(pDriverSpaceInfo->pInfo->szName));
				else
					pDriverSpaceInfo->pInfo->szName[0]='\0';
				p+=sizeof(TVTest::TuningSpaceInfo);
				pDriverSpaceInfo->ChannelList=(TVTest::ChannelInfo**)p;
				p+=NumChannels*sizeof(TVTest::ChannelInfo*);
				for (int j=0;j<NumChannels;j++) {
					TVTest::ChannelInfo *pChannelInfo=(TVTest::ChannelInfo*)p;
					p+=sizeof(TVTest::ChannelInfo);
					pDriverSpaceInfo->ChannelList[j]=pChannelInfo;
					pChannelInfo->Size=sizeof(TVTest::ChannelInfo);
					ConvertChannelInfo(pChannelList->GetChannelInfo(j),pChannelInfo);
				}
			}
#ifdef _DEBUG
			if (p-pBuffer!=BufferSize)
				::DebugBreak();
#endif
		}
		return TRUE;

	case TVTest::MESSAGE_FREEDRIVERTUNINGSPACELIST:
		{
			TVTest::DriverTuningSpaceList *pList=
				reinterpret_cast<TVTest::DriverTuningSpaceList*>(lParam1);

			if (pList!=NULL) {
				free(pList->SpaceList);
				pList->NumSpaces=0;
				pList->SpaceList=NULL;
			}
		}
		return TRUE;

	case TVTest::MESSAGE_ENABLEPROGRAMGUIDEEVENT:
		m_ProgramGuideEventFlags=(UINT)lParam1;
		return TRUE;

	case TVTest::MESSAGE_REGISTERPROGRAMGUIDECOMMAND:
		{
			const TVTest::ProgramGuideCommandInfo *pCommandList=
				reinterpret_cast<TVTest::ProgramGuideCommandInfo*>(lParam1);
			const int NumCommands=(int)lParam2;

			if (pCommandList==NULL || NumCommands<1)
				return FALSE;
			for (int i=0;i<NumCommands;i++) {
				m_ProgramGuideCommandList.push_back(CProgramGuideCommand(pCommandList[i]));
			}
		}
		return TRUE;

	case TVTest::MESSAGE_GETSTYLEVALUE:
		{
			TVTest::StyleValueInfo *pInfo=reinterpret_cast<TVTest::StyleValueInfo*>(lParam1);

			if (pInfo==NULL
					|| pInfo->Size!=sizeof(TVTest::StyleValueInfo)
					|| pInfo->Flags!=0
					|| IsStringEmpty(pInfo->pszName))
				return FALSE;

			const TVTest::Style::CStyleManager &StyleManager=GetAppClass().StyleManager;
			TVTest::Style::StyleInfo Style;

			if (!StyleManager.Get(pInfo->pszName,&Style))
				return FALSE;
			if (Style.Type==TVTest::Style::TYPE_INT) {
				if (pInfo->Unit==TVTest::STYLE_UNIT_UNDEFINED) {
					pInfo->Value=Style.Value.Int;
					switch (Style.Unit) {
					case TVTest::Style::UNIT_LOGICAL_PIXEL:
						pInfo->Value=StyleManager.LogicalPixelsToPhysicalPixels(pInfo->Value);
						pInfo->Unit=TVTest::STYLE_UNIT_PIXEL;
						break;
					case TVTest::Style::UNIT_PHYSICAL_PIXEL:
						pInfo->Unit=TVTest::STYLE_UNIT_PIXEL;
						break;
					case TVTest::Style::UNIT_POINT:
						pInfo->Unit=TVTest::STYLE_UNIT_POINT;
						break;
					case TVTest::Style::UNIT_DIP:
						pInfo->Unit=TVTest::STYLE_UNIT_DIP;
						break;
					}
				} else {
					TVTest::Style::UnitType Unit;
					switch (pInfo->Unit) {
					case TVTest::STYLE_UNIT_PIXEL:
						Unit=TVTest::Style::UNIT_PHYSICAL_PIXEL;
						break;
					case TVTest::STYLE_UNIT_POINT:
						Unit=TVTest::Style::UNIT_POINT;
						break;
					case TVTest::STYLE_UNIT_DIP:
						Unit=TVTest::Style::UNIT_DIP;
						break;
					default:
						return FALSE;
					}
					pInfo->Value=StyleManager.ConvertUnit(Style.Value.Int,Style.Unit,Unit);
				}
			} else if (Style.Type==TVTest::Style::TYPE_BOOL) {
				pInfo->Value=Style.Value.Bool;
			} else {
				return FALSE;
			}
		}
		return TRUE;

	case TVTest::MESSAGE_THEMEDRAWBACKGROUND:
		{
			TVTest::ThemeDrawBackgroundInfo *pInfo=
				reinterpret_cast<TVTest::ThemeDrawBackgroundInfo*>(lParam1);

			if (pInfo==NULL
					|| pInfo->Size!=sizeof(TVTest::ThemeDrawBackgroundInfo)
					|| IsStringEmpty(pInfo->pszStyle)
					|| pInfo->hdc==NULL)
				return FALSE;

			TVTest::Theme::CThemeManager ThemeManager(GetAppClass().UICore.GetCurrentColorScheme());
			int Type=ThemeManager.ParseStyleName(pInfo->pszStyle);
			if (Type<0)
				return FALSE;
			TVTest::Theme::BackgroundStyle Style;
			ThemeManager.GetBackgroundStyle(Type,&Style);
			TVTest::Theme::Draw(pInfo->hdc,pInfo->DrawRect,Style);
			if ((pInfo->Flags & TVTest::THEME_DRAW_BACKGROUND_FLAG_ADJUSTRECT)!=0)
				TVTest::Theme::SubtractBorderRect(Style.Border,&pInfo->DrawRect);
		}
		return TRUE;

	case TVTest::MESSAGE_THEMEDRAWTEXT:
		{
			TVTest::ThemeDrawTextInfo *pInfo=
				reinterpret_cast<TVTest::ThemeDrawTextInfo*>(lParam1);

			if (pInfo==NULL
					|| pInfo->Size!=sizeof(TVTest::ThemeDrawTextInfo)
					|| IsStringEmpty(pInfo->pszStyle)
					|| pInfo->hdc==NULL
					|| pInfo->pszText==NULL)
				return FALSE;

			TVTest::Theme::CThemeManager ThemeManager(GetAppClass().UICore.GetCurrentColorScheme());
			int Type=ThemeManager.ParseStyleName(pInfo->pszStyle);
			if (Type<0)
				return FALSE;
			TVTest::Theme::ForegroundStyle Style;
			ThemeManager.GetForegroundStyle(Type,&Style);
			if (pInfo->Color!=CLR_INVALID) {
				Style.Fill.Type=TVTest::Theme::FILL_SOLID;
				Style.Fill.Solid.Color=TVTest::Theme::ThemeColor(pInfo->Color);
			}
			TVTest::Theme::Draw(pInfo->hdc,pInfo->DrawRect,Style,pInfo->pszText,pInfo->DrawFlags);
		}
		return TRUE;

	case TVTest::MESSAGE_THEMEDRAWICON:
		{
			TVTest::ThemeDrawIconInfo *pInfo=
				reinterpret_cast<TVTest::ThemeDrawIconInfo*>(lParam1);

			if (pInfo==NULL
					|| pInfo->Size!=sizeof(TVTest::ThemeDrawTextInfo)
					|| IsStringEmpty(pInfo->pszStyle)
					|| pInfo->hdc==NULL
					|| pInfo->hbm==NULL)
				return FALSE;

			TVTest::Theme::CThemeManager ThemeManager(GetAppClass().UICore.GetCurrentColorScheme());
			int Type=ThemeManager.ParseStyleName(pInfo->pszStyle);
			if (Type<0)
				return FALSE;
			TVTest::Theme::ForegroundStyle Style;
			ThemeManager.GetForegroundStyle(Type,&Style);
			DrawUtil::CMonoColorBitmap Bitmap;
			if (!Bitmap.Create(pInfo->hbm))
				return FALSE;
			Bitmap.Draw(pInfo->hdc,
						pInfo->DstRect.left,pInfo->DstRect.top,
						pInfo->DstRect.right-pInfo->DstRect.left,
						pInfo->DstRect.bottom-pInfo->DstRect.top,
						pInfo->SrcRect.left,pInfo->SrcRect.top,
						pInfo->SrcRect.right-pInfo->SrcRect.left,
						pInfo->SrcRect.bottom-pInfo->SrcRect.top,
						pInfo->Color!=CLR_INVALID?pInfo->Color:Style.Fill.GetSolidColor(),
						pInfo->Opacity);
		}
		return TRUE;

	case TVTest::MESSAGE_GETEPGCAPTURESTATUS:
		{
			TVTest::EpgCaptureStatusInfo *pInfo=
				reinterpret_cast<TVTest::EpgCaptureStatusInfo*>(lParam1);

			if (pInfo==NULL
					|| pInfo->Size!=sizeof(TVTest::EpgCaptureStatusInfo)
					|| pInfo->Flags!=0)
				return FALSE;

			CEventManager &EventManager=GetAppClass().CoreEngine.m_DtvEngine.m_EventManager;
			DWORD Status=0;

			if ((pInfo->Status & TVTest::EPG_CAPTURE_STATUS_SCHEDULEBASICCOMPLETED)!=0) {
				if (EventManager.IsScheduleComplete(pInfo->NetworkID,pInfo->TransportStreamID,pInfo->ServiceID,false))
					Status|=TVTest::EPG_CAPTURE_STATUS_SCHEDULEBASICCOMPLETED;
			}
			if ((pInfo->Status & TVTest::EPG_CAPTURE_STATUS_SCHEDULEEXTENDEDCOMPLETED)!=0) {
				if (EventManager.IsScheduleComplete(pInfo->NetworkID,pInfo->TransportStreamID,pInfo->ServiceID,true))
					Status|=TVTest::EPG_CAPTURE_STATUS_SCHEDULEEXTENDEDCOMPLETED;
			}
			if ((pInfo->Status & TVTest::EPG_CAPTURE_STATUS_HASSCHEDULEBASIC)!=0) {
				if (EventManager.HasSchedule(pInfo->NetworkID,pInfo->TransportStreamID,pInfo->ServiceID,false))
					Status|=TVTest::EPG_CAPTURE_STATUS_HASSCHEDULEBASIC;
			}
			if ((pInfo->Status & TVTest::EPG_CAPTURE_STATUS_HASSCHEDULEEXTENDED)!=0) {
				if (EventManager.HasSchedule(pInfo->NetworkID,pInfo->TransportStreamID,pInfo->ServiceID,true))
					Status|=TVTest::EPG_CAPTURE_STATUS_HASSCHEDULEEXTENDED;
			}

			pInfo->Status=Status;
		}
		return TRUE;

	case TVTest::MESSAGE_GETAPPCOMMANDINFO:
		{
			TVTest::AppCommandInfo *pInfo=reinterpret_cast<TVTest::AppCommandInfo*>(lParam1);

			if (pInfo==NULL || pInfo->Size!=sizeof(TVTest::AppCommandInfo))
				return FALSE;

			const CCommandList &CommandList=GetAppClass().CommandList;

			LPCTSTR pszText=CommandList.GetCommandText(pInfo->Index);
			if (pszText==NULL)
				return FALSE;
			if (pInfo->pszText!=NULL) {
				::lstrcpynW(pInfo->pszText,pszText,pInfo->MaxText);
			} else {
				pInfo->MaxText=::lstrlenW(pszText)+1;
			}
			TCHAR szName[CCommandList::MAX_COMMAND_NAME];
			CommandList.GetCommandName(pInfo->Index,szName,lengthof(szName));
			if (pInfo->pszName!=NULL) {
				::lstrcpynW(pInfo->pszName,szName,pInfo->MaxName);
			} else {
				pInfo->MaxName=::lstrlenW(szName)+1;
			}
		}
		return TRUE;

	case TVTest::MESSAGE_GETAPPCOMMANDCOUNT:
		return GetAppClass().CommandList.NumCommands();

	case TVTest::MESSAGE_GETVIDEOSTREAMCOUNT:
		return GetAppClass().CoreEngine.m_DtvEngine.GetVideoStreamNum();

	case TVTest::MESSAGE_GETVIDEOSTREAM:
		return GetAppClass().CoreEngine.m_DtvEngine.GetVideoStream();

	case TVTest::MESSAGE_SETVIDEOSTREAM:
		return SendPluginMessage(pParam,Message,lParam1,lParam2);

	case TVTest::MESSAGE_GETLOG:
		{
			TVTest::GetLogInfo *pInfo=reinterpret_cast<TVTest::GetLogInfo*>(lParam1);

			if (pInfo==NULL || pInfo->Size!=sizeof(TVTest::GetLogInfo))
				return FALSE;

			const CLogger &Logger=GetAppClass().Logger;
			CLogItem Log;

			if ((pInfo->Flags & TVTest::GET_LOG_FLAG_BYSERIAL)==0) {
				if (!Logger.GetLog(pInfo->Index,&Log))
					return FALSE;
				pInfo->Serial=Log.GetSerialNumber();
			} else {
				if (!Logger.GetLogBySerialNumber(pInfo->Serial,&Log))
					return FALSE;
			}
			LPCTSTR pszText=Log.GetText();
			if (pInfo->pszText!=NULL) {
				::lstrcpyn(pInfo->pszText,pszText,pInfo->MaxText);
			} else {
				pInfo->MaxText=::lstrlen(pszText)+1;
			}
			pInfo->Type=(int)Log.GetType();
		}
		return TRUE;

	case TVTest::MESSAGE_GETLOGCOUNT:
		return GetAppClass().Logger.GetLogCount();

	case TVTest::MESSAGE_REGISTERPLUGINCOMMAND:
		{
			const TVTest::PluginCommandInfo *pInfo=
				reinterpret_cast<const TVTest::PluginCommandInfo*>(lParam1);

			if (pInfo==NULL || pInfo->Size!=sizeof(TVTest::PluginCommandInfo))
				return FALSE;

			m_CommandList.push_back(CPluginCommandInfo(*pInfo));
		}
		return TRUE;

	case TVTest::MESSAGE_SETPLUGINCOMMANDSTATE:
		{
			const int ID=(int)lParam1;

			for (auto itr=m_CommandList.begin();itr!=m_CommandList.end();++itr) {
				if (itr->GetID()==ID) {
					const DWORD State=(DWORD)lParam2;

					itr->SetState(State);

					unsigned int CommandState=0;
					if ((State & TVTest::PLUGIN_COMMAND_STATE_DISABLED)!=0)
						CommandState|=CCommandList::COMMAND_STATE_DISABLED;
					if ((State & TVTest::PLUGIN_COMMAND_STATE_CHECKED)!=0)
						CommandState|=CCommandList::COMMAND_STATE_CHECKED;
					GetAppClass().CommandList.SetCommandStateByID(
						itr->GetCommand(),
						CCommandList::COMMAND_STATE_DISABLED |
						CCommandList::COMMAND_STATE_CHECKED,
						CommandState);

					return TRUE;
				}
			}
		}
		return FALSE;

	case TVTest::MESSAGE_PLUGINCOMMANDNOTIFY:
		{
			const int ID=(int)lParam1;

			for (auto itr=m_CommandList.begin();itr!=m_CommandList.end();++itr) {
				if (itr->GetID()==ID) {
					const unsigned int Type=(unsigned int)lParam2;

					if ((Type & TVTest::PLUGIN_COMMAND_NOTIFY_CHANGEICON))
						GetAppClass().SideBar.RedrawItem(itr->GetCommand());

					return TRUE;
				}
			}
		}
		return FALSE;

	case TVTest::MESSAGE_REGISTERPLUGINICON:
		{
			const TVTest::PluginIconInfo *pInfo=
				reinterpret_cast<const TVTest::PluginIconInfo*>(lParam1);

			if (pInfo==NULL
					|| pInfo->Size!=sizeof(TVTest::PluginIconInfo)
					|| pInfo->Flags!=0
					|| pInfo->hbmIcon==NULL)
				return FALSE;

			if (!m_PluginIcon.Create(pInfo->hbmIcon))
				return FALSE;
		}
		return TRUE;

	case TVTest::MESSAGE_REGISTERSTATUSITEM:
		{
			const TVTest::StatusItemInfo *pInfo=
				reinterpret_cast<const TVTest::StatusItemInfo*>(lParam1);

			if (pInfo==NULL
					|| pInfo->Size!=sizeof(TVTest::StatusItemInfo)
					|| IsStringEmpty(pInfo->pszIDText)
					|| IsStringEmpty(pInfo->pszName))
				return FALSE;

			StatusItem *pItem=new StatusItem;

			pItem->Flags=pInfo->Flags;
			pItem->ID=pInfo->ID;
			pItem->IDText=pInfo->pszIDText;
			pItem->Name=pInfo->pszName;
			pItem->MinWidth=pInfo->MinWidth;
			pItem->MaxWidth=pInfo->MaxWidth;
			pItem->DefaultWidth=pInfo->DefaultWidth;
			pItem->MinHeight=pInfo->MinHeight;
			pItem->ItemID=-1;
			pItem->Style=pInfo->Style;
			pItem->State=0;
			pItem->pItem=NULL;

			m_StatusItemList.push_back(pItem);
		}
		return TRUE;

	case TVTest::MESSAGE_SETSTATUSITEM:
		{
			TVTest::StatusItemSetInfo *pInfo=
				reinterpret_cast<TVTest::StatusItemSetInfo*>(lParam1);

			if (pInfo==NULL || pInfo->Size!=sizeof(TVTest::StatusItemSetInfo))
				return FALSE;

			for (auto itr=m_StatusItemList.begin();itr!=m_StatusItemList.end();++itr) {
				StatusItem *pItem=*itr;
				if (pItem->ID==pInfo->ID) {
					if ((pInfo->Mask & TVTest::STATUS_ITEM_SET_INFO_MASK_STATE)!=0) {
						DWORD OldState=pItem->State;
						DWORD NewState=(pItem->State & ~pInfo->StateMask) | (pInfo->State & pInfo->StateMask);
						pItem->State=NewState;
						if (((NewState ^ OldState) & TVTest::STATUS_ITEM_STATE_VISIBLE)!=0) {
							const bool fVisible=(NewState & TVTest::STATUS_ITEM_STATE_VISIBLE)!=0;
							GetAppClass().StatusOptions.SetItemVisibility(pItem->ItemID,fVisible);
							if (pItem->pItem!=NULL) {
								GetAppClass().MainWindow.ShowStatusBarItem(pItem->ItemID,fVisible);
							}
						}
					}

					if ((pInfo->Mask & TVTest::STATUS_ITEM_SET_INFO_MASK_STYLE)!=0) {
						DWORD OldStyle=pItem->Style;
						DWORD NewStyle=(OldStyle & ~pInfo->StyleMask) | (pInfo->Style & pInfo->StyleMask);
						if (NewStyle!=OldStyle) {
							pItem->Style=NewStyle;
							if (pItem->pItem!=NULL) {
								pItem->pItem->ApplyStyle();
								GetAppClass().StatusView.AdjustSize();
							}
						}
					}

					return TRUE;
				}
			}
		}
		return FALSE;

	case TVTest::MESSAGE_GETSTATUSITEMINFO:
		{
			TVTest::StatusItemGetInfo *pInfo=
				reinterpret_cast<TVTest::StatusItemGetInfo*>(lParam1);

			if (pInfo==NULL || pInfo->Size!=sizeof(TVTest::StatusItemGetInfo))
				return FALSE;

			for (auto itr=m_StatusItemList.begin();itr!=m_StatusItemList.end();++itr) {
				StatusItem *pItem=*itr;

				if (pItem->ID==pInfo->ID) {
					if ((pInfo->Mask & TVTest::STATUS_ITEM_GET_INFO_MASK_STATE)!=0)
						pInfo->State=pItem->State;

					if ((pInfo->Mask & TVTest::STATUS_ITEM_GET_INFO_MASK_HWND)!=0) {
						if (pItem->pItem!=NULL)
							pInfo->hwnd=pItem->pItem->GetWindowHandle();
						else
							pInfo->hwnd=NULL;
					}

					if ((pInfo->Mask & TVTest::STATUS_ITEM_GET_INFO_MASK_ITEMRECT)!=0) {
						if (pItem->pItem==NULL || !pItem->pItem->GetRect(&pInfo->ItemRect))
							::SetRectEmpty(&pInfo->ItemRect);
					}

					if ((pInfo->Mask & TVTest::STATUS_ITEM_GET_INFO_MASK_CONTENTRECT)!=0) {
						if (pItem->pItem==NULL || !pItem->pItem->GetClientRect(&pInfo->ContentRect))
							::SetRectEmpty(&pInfo->ContentRect);
					}

					if ((pInfo->Mask & TVTest::STATUS_ITEM_GET_INFO_MASK_STYLE)!=0)
						pInfo->Style=pItem->Style;

					return TRUE;
				}
			}
		}
		return FALSE;

	case TVTest::MESSAGE_STATUSITEMNOTIFY:
		for (auto itr=m_StatusItemList.begin();itr!=m_StatusItemList.end();++itr) {
			StatusItem *pItem=*itr;

			if (pItem->ID==lParam1) {
				switch (lParam2) {
				case TVTest::STATUS_ITEM_NOTIFY_REDRAW:
					if (pItem->pItem!=NULL)
						pItem->pItem->Redraw();
					return TRUE;
				}
			}
		}
		return FALSE;

	case TVTest::MESSAGE_REGISTERTSPROCESSOR:
		{
			const TVTest::TSProcessorInfo *pInfo=
				reinterpret_cast<const TVTest::TSProcessorInfo*>(lParam1);

			if (pInfo==NULL
					|| pInfo->Size!=sizeof(TVTest::TSProcessorInfo)
					|| pInfo->Flags!=0
					|| pInfo->pTSProcessor==NULL)
				return FALSE;

			TVTest::CTSProcessor *pTSProcessor=
				new TVTest::CTSProcessor(pInfo->pTSProcessor);
			if (!GetAppClass().TSProcessorManager.RegisterTSProcessor(
					pTSProcessor,
					(CCoreEngine::TSProcessorConnectPosition)pInfo->ConnectPosition)) {
				pTSProcessor->Release();
				return FALSE;
			}

			m_Flags|=TVTest::PLUGIN_FLAG_NOUNLOAD;
		}
		return TRUE;

	case TVTest::MESSAGE_REGISTERPANELITEM:
		{
			const TVTest::PanelItemInfo *pInfo=
				reinterpret_cast<const TVTest::PanelItemInfo*>(lParam1);

			if (pInfo==NULL
					|| pInfo->Size!=sizeof(TVTest::PanelItemInfo)
					|| pInfo->Flags!=0
					|| IsStringEmpty(pInfo->pszIDText)
					|| IsStringEmpty(pInfo->pszTitle))
				return FALSE;

			PanelItem *pItem=new PanelItem;

			pItem->ID=pInfo->ID;
			pItem->IDText=pInfo->pszIDText;
			pItem->Title=pInfo->pszTitle;
			pItem->StateMask=0;
			pItem->State=0;
			pItem->Style=pInfo->Style;
			pItem->ItemID=-1;
			pItem->pItem=NULL;
			if (pInfo->hbmIcon!=NULL)
				pItem->Icon.Create(pInfo->hbmIcon);

			m_PanelItemList.push_back(pItem);
		}
		return TRUE;

	case TVTest::MESSAGE_SETPANELITEM:
		{
			TVTest::PanelItemSetInfo *pInfo=
				reinterpret_cast<TVTest::PanelItemSetInfo*>(lParam1);

			if (pInfo==NULL || pInfo->Size!=sizeof(TVTest::PanelItemSetInfo))
				return FALSE;

			for (auto itr=m_PanelItemList.begin();itr!=m_PanelItemList.end();++itr) {
				PanelItem *pItem=*itr;
				if (pItem->ID==pInfo->ID) {
					if ((pInfo->Mask & TVTest::PANEL_ITEM_SET_INFO_MASK_STATE)!=0) {
						if (pItem->pItem==NULL || pItem->pItem->GetItemHandle()==NULL) {
							pItem->StateMask|=pInfo->StateMask;
							pItem->State=(pItem->State & ~pInfo->StateMask) | (pInfo->State & pInfo->StateMask);
						} else {
							if ((pInfo->StateMask & TVTest::PANEL_ITEM_STATE_ENABLED)!=0) {
								GetAppClass().PanelOptions.SetPanelItemVisibility(
									pItem->ItemID,
									(pInfo->State & TVTest::PANEL_ITEM_STATE_ENABLED)!=0);
							}

							if ((pInfo->StateMask & TVTest::PANEL_ITEM_STATE_ACTIVE)!=0) {
								if ((pInfo->State & TVTest::PANEL_ITEM_STATE_ACTIVE)!=0) {
									GetAppClass().Panel.Form.SetCurPageByID(pItem->ItemID);
								}
							}
						}
					}

					if ((pInfo->Mask & TVTest::PANEL_ITEM_SET_INFO_MASK_STYLE)!=0) {
						pItem->Style=(pItem->Style & ~pInfo->StyleMask) | (pInfo->Style & pInfo->StyleMask);
					}

					return TRUE;
				}
			}
		}
		return FALSE;

	case TVTest::MESSAGE_GETPANELITEMINFO:
		{
			TVTest::PanelItemGetInfo *pInfo=
				reinterpret_cast<TVTest::PanelItemGetInfo*>(lParam1);

			if (pInfo==NULL || pInfo->Size!=sizeof(TVTest::PanelItemGetInfo))
				return FALSE;

			auto it=std::find_if(m_PanelItemList.begin(),m_PanelItemList.end(),
								 [&](const PanelItem *pItem) -> bool { return pItem->ID==pInfo->ID; });
			if (it==m_PanelItemList.end())
				return FALSE;

			const PanelItem *pItem=*it;

			if ((pInfo->Mask & TVTest::PANEL_ITEM_GET_INFO_MASK_STATE)!=0) {
				pInfo->State=pItem->State;
			}

			if ((pInfo->Mask & TVTest::PANEL_ITEM_GET_INFO_MASK_HWNDPARENT)!=0) {
				if (pItem->pItem!=NULL)
					pInfo->hwndParent=pItem->pItem->GetHandle();
				else
					pInfo->hwndParent=NULL;
			}

			if ((pInfo->Mask & TVTest::PANEL_ITEM_GET_INFO_MASK_HWNDITEM)!=0) {
				if (pItem->pItem!=NULL)
					pInfo->hwndItem=pItem->pItem->GetItemHandle();
				else
					pInfo->hwndItem=NULL;
			}

			if ((pInfo->Mask & TVTest::PANEL_ITEM_GET_INFO_MASK_STYLE)!=0) {
				pInfo->Style=pItem->Style;
			}
		}
		return TRUE;

	case TVTest::MESSAGE_SELECTCHANNEL:
		return SendPluginMessage(pParam,Message,lParam1,lParam2);

	case TVTest::MESSAGE_GETFAVORITELIST:
		{
			TVTest::FavoriteList *pList=reinterpret_cast<TVTest::FavoriteList*>(lParam1);

			if (pList==NULL || pList->Size!=sizeof(TVTest::FavoriteList))
				return FALSE;

			return GetFavoriteList(GetAppClass().FavoritesManager.GetRootFolder(),pList);
		}

	case TVTest::MESSAGE_FREEFAVORITELIST:
		{
			TVTest::FavoriteList *pList=reinterpret_cast<TVTest::FavoriteList*>(lParam1);

			if (pList!=NULL && pList->Size==sizeof(TVTest::FavoriteList))
				FreeFavoriteList(pList);
		}
		return 0;

	case TVTest::MESSAGE_GET1SEGMODE:
		return GetAppClass().Core.Is1SegMode();

	case TVTest::MESSAGE_SET1SEGMODE:
		return SendPluginMessage(pParam,Message,lParam1,lParam2);

#ifdef _DEBUG
	default:
		TRACE(TEXT("CPluign::OnCallback() : Unknown message %u\n"),Message);
		break;
#endif
	}

	return 0;
}


void CPlugin::SetMessageWindow(HWND hwnd,UINT Message)
{
	m_hwndMessage=hwnd;
	m_MessageCode=Message;
}


LRESULT CPlugin::OnPluginMessage(WPARAM wParam,LPARAM lParam)
{
	PluginMessageParam *pParam=reinterpret_cast<PluginMessageParam*>(lParam);

	if (pParam==NULL || wParam!=pParam->Message)
		return 0;

	switch ((UINT)wParam) {
	case TVTest::MESSAGE_GETCURRENTCHANNELINFO:
		{
			TVTest::ChannelInfo *pChannelInfo=reinterpret_cast<TVTest::ChannelInfo*>(pParam->lParam1);

			if (pChannelInfo==NULL
					|| (pChannelInfo->Size!=sizeof(TVTest::ChannelInfo)
						&& pChannelInfo->Size!=TVTest::CHANNELINFO_SIZE_V1
						&& pChannelInfo->Size!=TVTest::CHANNELINFO_SIZE_V2))
				return FALSE;
			const CChannelInfo *pChInfo=GetAppClass().Core.GetCurrentChannelInfo();
			if (pChInfo==NULL)
				return FALSE;
			ConvertChannelInfo(pChInfo,pChannelInfo);
			CTsAnalyzer *pTsAnalyzer=&GetAppClass().CoreEngine.m_DtvEngine.m_TsAnalyzer;
			if (!pTsAnalyzer->GetNetworkName(pChannelInfo->szNetworkName,
											 lengthof(pChannelInfo->szNetworkName)))
				pChannelInfo->szNetworkName[0]='\0';
			if (!pTsAnalyzer->GetTsName(pChannelInfo->szTransportStreamName,
										lengthof(pChannelInfo->szTransportStreamName)))
				pChannelInfo->szTransportStreamName[0]='\0';
		}
		return TRUE;

	case TVTest::MESSAGE_SETCHANNEL:
		{
			CAppMain &App=GetAppClass();

			App.Core.OpenTuner();

			int Space=(int)pParam->lParam1;

			if (pParam->pPlugin->m_Version<TVTEST_PLUGIN_VERSION_(0,0,8))
				return App.Core.SetChannel(Space,(int)pParam->lParam2,-1);

			WORD ServiceID=HIWORD(pParam->lParam2);
			return App.Core.SetChannel(Space,(SHORT)LOWORD(pParam->lParam2),
									   ServiceID!=0?(int)ServiceID:-1);
		}

	case TVTest::MESSAGE_SETSERVICE:
		{
			if (pParam->lParam2==0)
				return GetAppClass().Core.SetServiceByIndex((int)pParam->lParam1,CAppCore::SET_SERVICE_STRICT_ID);
			return GetAppClass().Core.SetServiceByID((WORD)pParam->lParam1,CAppCore::SET_SERVICE_STRICT_ID);
		}

	case TVTest::MESSAGE_GETTUNINGSPACENAME:
		{
			LPWSTR pszName=reinterpret_cast<LPWSTR>(pParam->lParam1);
			int Index=LOWORD(pParam->lParam2);
			int MaxLength=HIWORD(pParam->lParam2);
			const CTuningSpaceList *pTuningSpaceList=GetAppClass().ChannelManager.GetDriverTuningSpaceList();
			LPCTSTR pszTuningSpaceName=pTuningSpaceList->GetTuningSpaceName(Index);

			if (pszTuningSpaceName==NULL)
				return 0;
			if (pszName!=NULL)
				::lstrcpyn(pszName,pszTuningSpaceName,MaxLength);
			return ::lstrlen(pszTuningSpaceName);
		}

	case TVTest::MESSAGE_GETCHANNELINFO:
		{
			TVTest::ChannelInfo *pChannelInfo=reinterpret_cast<TVTest::ChannelInfo*>(pParam->lParam1);
			int Space=LOWORD(pParam->lParam2);
			int Channel=HIWORD(pParam->lParam2);

			if (pChannelInfo==NULL
					|| (pChannelInfo->Size!=sizeof(TVTest::ChannelInfo)
						&& pChannelInfo->Size!=TVTest::CHANNELINFO_SIZE_V1
						&& pChannelInfo->Size!=TVTest::CHANNELINFO_SIZE_V2)
					|| Space<0 || Channel<0)
				return FALSE;

			const CChannelManager *pChannelManager=&GetAppClass().ChannelManager;
			const CChannelList *pChannelList=pChannelManager->GetChannelList(Space);
			if (pChannelList==NULL)
				return FALSE;
			const CChannelInfo *pChInfo=pChannelList->GetChannelInfo(Channel);
			if (pChInfo==NULL)
				return FALSE;
			ConvertChannelInfo(pChInfo,pChannelInfo);
		}
		return TRUE;

	case TVTest::MESSAGE_GETDRIVERNAME:
		{
			LPWSTR pszName=reinterpret_cast<LPWSTR>(pParam->lParam1);
			int MaxLength=(int)pParam->lParam2;
			LPCTSTR pszDriverName=GetAppClass().CoreEngine.GetDriverFileName();

			if (pszName!=NULL && MaxLength>0)
				::lstrcpyn(pszName,pszDriverName,MaxLength);
			return ::lstrlen(pszDriverName);
		}

	case TVTest::MESSAGE_SETDRIVERNAME:
		{
			LPCWSTR pszDriverName=reinterpret_cast<LPCWSTR>(pParam->lParam1);

			if (pszDriverName==NULL) {
				GetAppClass().Core.ShutDownTuner();
				return TRUE;
			}

			return GetAppClass().Core.OpenTuner(pszDriverName);
		}

	case TVTest::MESSAGE_STARTRECORD:
		{
			CAppMain &App=GetAppClass();
			TVTest::RecordInfo *pRecInfo=reinterpret_cast<TVTest::RecordInfo*>(pParam->lParam1);
			CRecordManager::TimeSpecInfo StartTime,StopTime;

			if (pRecInfo==NULL)
				return App.Core.StartRecord(NULL,NULL,NULL,CRecordManager::CLIENT_PLUGIN);
			if (pRecInfo->Size!=sizeof(TVTest::RecordInfo))
				return FALSE;
			StartTime.Type=CRecordManager::TIME_NOTSPECIFIED;
			if ((pRecInfo->Mask&TVTest::RECORD_MASK_STARTTIME)!=0) {
				switch (pRecInfo->StartTimeSpec) {
				case TVTest::RECORD_START_NOTSPECIFIED:
					break;
				case TVTest::RECORD_START_TIME:
					StartTime.Type=CRecordManager::TIME_DATETIME;
					StartTime.Time.DateTime=pRecInfo->StartTime.Time;
					break;
				case TVTest::RECORD_START_DELAY:
					StartTime.Type=CRecordManager::TIME_DURATION;
					StartTime.Time.Duration=pRecInfo->StartTime.Delay;
					break;
				default:
					return FALSE;
				}
			}
			StopTime.Type=CRecordManager::TIME_NOTSPECIFIED;
			if ((pRecInfo->Mask&TVTest::RECORD_MASK_STOPTIME)!=0) {
				switch (pRecInfo->StopTimeSpec) {
				case TVTest::RECORD_STOP_NOTSPECIFIED:
					break;
				case TVTest::RECORD_STOP_TIME:
					StopTime.Type=CRecordManager::TIME_DATETIME;
					StopTime.Time.DateTime=pRecInfo->StopTime.Time;
					break;
				case TVTest::RECORD_STOP_DURATION:
					StopTime.Type=CRecordManager::TIME_DURATION;
					StopTime.Time.Duration=pRecInfo->StopTime.Duration;
					break;
				default:
					return FALSE;
				}
			}
			return App.Core.StartRecord(
				(pRecInfo->Mask&TVTest::RECORD_MASK_FILENAME)!=0?pRecInfo->pszFileName:NULL,
				&StartTime,&StopTime,
				CRecordManager::CLIENT_PLUGIN);
		}

	case TVTest::MESSAGE_STOPRECORD:
		return GetAppClass().Core.StopRecord();

	case TVTest::MESSAGE_PAUSERECORD:
		{
			CAppMain &App=GetAppClass();
			const CRecordManager *pRecordManager=&App.RecordManager;
			bool fPause=pParam->lParam1!=0;

			if (!pRecordManager->IsRecording())
				return FALSE;
			if (fPause==pRecordManager->IsPaused())
				return FALSE;
			App.UICore.DoCommand(CM_RECORD_PAUSE);
		}
		return TRUE;

	case TVTest::MESSAGE_GETRECORD:
		{
			TVTest::RecordInfo *pRecInfo=reinterpret_cast<TVTest::RecordInfo*>(pParam->lParam1);

			if (pRecInfo==NULL || pRecInfo->Size!=sizeof(TVTest::RecordInfo))
				return FALSE;

			const CRecordManager *pRecordManager=&GetAppClass().RecordManager;

			if ((pRecInfo->Mask&TVTest::RECORD_MASK_FILENAME)!=0
					&& pRecInfo->pszFileName!=NULL && pRecInfo->MaxFileName>0) {
				if (pRecordManager->GetFileName()!=NULL)
					::lstrcpyn(pRecInfo->pszFileName,pRecordManager->GetFileName(),
							   pRecInfo->MaxFileName);
				else
					pRecInfo->pszFileName[0]='\0';
			}

			if (!pRecordManager->GetReserveTime(&pRecInfo->ReserveTime))
				pRecInfo->ReserveTime=FILETIME_NULL;

			if ((pRecInfo->Mask&TVTest::RECORD_MASK_STARTTIME)!=0) {
				CRecordManager::TimeSpecInfo StartTime;

				if (!pRecordManager->GetStartTimeSpec(&StartTime))
					StartTime.Type=CRecordManager::TIME_NOTSPECIFIED;
				switch (StartTime.Type) {
				case CRecordManager::TIME_NOTSPECIFIED:
					pRecInfo->StartTimeSpec=TVTest::RECORD_START_NOTSPECIFIED;
					break;
				case CRecordManager::TIME_DATETIME:
					pRecInfo->StartTimeSpec=TVTest::RECORD_START_TIME;
					pRecInfo->StartTime.Time=StartTime.Time.DateTime;
					break;
				case CRecordManager::TIME_DURATION:
					pRecInfo->StartTimeSpec=TVTest::RECORD_START_DELAY;
					pRecInfo->StartTime.Delay=StartTime.Time.Duration;
					break;
				}
			}

			if ((pRecInfo->Mask&TVTest::RECORD_MASK_STOPTIME)!=0) {
				CRecordManager::TimeSpecInfo StopTime;

				if (!pRecordManager->GetStopTimeSpec(&StopTime))
					StopTime.Type=CRecordManager::TIME_NOTSPECIFIED;
				switch (StopTime.Type) {
				case CRecordManager::TIME_NOTSPECIFIED:
					pRecInfo->StopTimeSpec=TVTest::RECORD_STOP_NOTSPECIFIED;
					break;
				case CRecordManager::TIME_DATETIME:
					pRecInfo->StopTimeSpec=TVTest::RECORD_STOP_TIME;
					pRecInfo->StopTime.Time=StopTime.Time.DateTime;
					break;
				case CRecordManager::TIME_DURATION:
					pRecInfo->StopTimeSpec=TVTest::RECORD_STOP_DURATION;
					pRecInfo->StopTime.Duration=StopTime.Time.Duration;
					break;
				}
			}
		}
		return TRUE;

	case TVTest::MESSAGE_MODIFYRECORD:
		{
			CAppMain &App=GetAppClass();
			TVTest::RecordInfo *pRecInfo=reinterpret_cast<TVTest::RecordInfo*>(pParam->lParam1);
			CRecordManager::TimeSpecInfo StartTime,StopTime;

			if (pRecInfo==NULL || pRecInfo->Size!=sizeof(TVTest::RecordInfo))
				return false;
			if ((pRecInfo->Mask&TVTest::RECORD_MASK_FLAGS)!=0) {
				if ((pRecInfo->Flags&TVTest::RECORD_FLAG_CANCEL)!=0)
					return App.Core.CancelReservedRecord();
			}
			if ((pRecInfo->Mask&TVTest::RECORD_MASK_STARTTIME)!=0) {
				switch (pRecInfo->StartTimeSpec) {
				case TVTest::RECORD_START_NOTSPECIFIED:
					StartTime.Type=CRecordManager::TIME_NOTSPECIFIED;
					break;
				case TVTest::RECORD_START_TIME:
					StartTime.Type=CRecordManager::TIME_DATETIME;
					StartTime.Time.DateTime=pRecInfo->StartTime.Time;
					break;
				case TVTest::RECORD_START_DELAY:
					StartTime.Type=CRecordManager::TIME_DURATION;
					StartTime.Time.Duration=pRecInfo->StartTime.Delay;
					break;
				default:
					return FALSE;
				}
			}
			if ((pRecInfo->Mask&TVTest::RECORD_MASK_STOPTIME)!=0) {
				switch (pRecInfo->StopTimeSpec) {
				case TVTest::RECORD_STOP_NOTSPECIFIED:
					StopTime.Type=CRecordManager::TIME_NOTSPECIFIED;
					break;
				case TVTest::RECORD_STOP_TIME:
					StopTime.Type=CRecordManager::TIME_DATETIME;
					StopTime.Time.DateTime=pRecInfo->StopTime.Time;
					break;
				case TVTest::RECORD_STOP_DURATION:
					StopTime.Type=CRecordManager::TIME_DURATION;
					StopTime.Time.Duration=pRecInfo->StopTime.Duration;
					break;
				default:
					return FALSE;
				}
			}
			return App.Core.ModifyRecord(
				(pRecInfo->Mask&TVTest::RECORD_MASK_FILENAME)!=0?pRecInfo->pszFileName:NULL,
				(pRecInfo->Mask&TVTest::RECORD_MASK_STARTTIME)!=0?&StartTime:NULL,
				(pRecInfo->Mask&TVTest::RECORD_MASK_STOPTIME)!=0?&StopTime:NULL,
				CRecordManager::CLIENT_PLUGIN);
		}

	case TVTest::MESSAGE_GETZOOM:
		return GetAppClass().UICore.GetZoomPercentage();

	case TVTest::MESSAGE_SETZOOM:
		return GetAppClass().UICore.SetZoomRate((int)pParam->lParam1,(int)pParam->lParam2);

	case TVTest::MESSAGE_GETRECORDSTATUS:
		{
			TVTest::RecordStatusInfo *pInfo=reinterpret_cast<TVTest::RecordStatusInfo*>(pParam->lParam1);
			const CRecordManager *pRecordManager=&GetAppClass().RecordManager;

			if (pInfo==NULL
					|| (pInfo->Size!=sizeof(TVTest::RecordStatusInfo)
						&& pInfo->Size!=TVTest::RECORDSTATUSINFO_SIZE_V1))
				return FALSE;
			pInfo->Status=pRecordManager->IsRecording()?
				(pRecordManager->IsPaused()?TVTest::RECORD_STATUS_PAUSED:
											TVTest::RECORD_STATUS_RECORDING):
											TVTest::RECORD_STATUS_NOTRECORDING;
			if (pInfo->Status!=TVTest::RECORD_STATUS_NOTRECORDING) {
				pRecordManager->GetStartTime(&pInfo->StartTime);
				CRecordManager::TimeSpecInfo StopTimeInfo;
				pRecordManager->GetStopTimeSpec(&StopTimeInfo);
				pInfo->StopTimeSpec=(DWORD)StopTimeInfo.Type;
				if (StopTimeInfo.Type==CRecordManager::TIME_DATETIME)
					pInfo->StopTime.Time=StopTimeInfo.Time.DateTime;
				else
					pInfo->StopTime.Duration=StopTimeInfo.Time.Duration;
			} else {
				pInfo->StopTimeSpec=TVTest::RECORD_STOP_NOTSPECIFIED;
			}
			pInfo->RecordTime=(DWORD)pRecordManager->GetRecordTime();
			pInfo->PauseTime=(DWORD)pRecordManager->GetPauseTime();
			if (pInfo->Size>TVTest::RECORDSTATUSINFO_SIZE_V1) {
				if (pInfo->pszFileName!=NULL && pInfo->MaxFileName>0) {
					if (pRecordManager->IsRecording()) {
						pRecordManager->GetRecordTask()->GetFileName(
							pInfo->pszFileName,pInfo->MaxFileName);
					} else {
						pInfo->pszFileName[0]='\0';
					}
				}
			}
		}
		return TRUE;

	case TVTest::MESSAGE_SETVOLUME:
		{
			CUICore *pUICore=&GetAppClass().UICore;
			int Volume=(int)pParam->lParam1;

			if (Volume<0)
				return pUICore->SetMute(pParam->lParam2!=0);
			return pUICore->SetVolume(Volume,true);
		}

	case TVTest::MESSAGE_SETSTEREOMODE:
#if 0	// ver.0.9.0 より前
		return GetAppClass().UICore.SetStereoMode(static_cast<CAudioDecFilter::StereoMode>(pParam->lParam1));
#else
		{
			CAudioDecFilter::DualMonoMode Mode;

			switch ((int)pParam->lParam1) {
			case TVTest::STEREOMODE_STEREO:	Mode=CAudioDecFilter::DUALMONO_BOTH;	break;
			case TVTest::STEREOMODE_LEFT:	Mode=CAudioDecFilter::DUALMONO_MAIN;	break;
			case TVTest::STEREOMODE_RIGHT:	Mode=CAudioDecFilter::DUALMONO_SUB;		break;
			default:
				return FALSE;
			}

			return GetAppClass().UICore.SetDualMonoMode(Mode);
		}
#endif

	case TVTest::MESSAGE_SETFULLSCREEN:
		return GetAppClass().UICore.SetFullscreen(pParam->lParam1!=0);

	case TVTest::MESSAGE_SETPREVIEW:
		return GetAppClass().UICore.EnableViewer(pParam->lParam1!=0);

	case TVTest::MESSAGE_SETSTANDBY:
		return GetAppClass().UICore.SetStandby(pParam->lParam1!=0);

	case TVTest::MESSAGE_SETALWAYSONTOP:
		GetAppClass().UICore.SetAlwaysOnTop(pParam->lParam1!=0);
		return TRUE;

	case TVTest::MESSAGE_GETTUNINGSPACE:
		{
			const CChannelManager *pChannelManager=&GetAppClass().ChannelManager;
			int *pNumSpaces=reinterpret_cast<int*>(pParam->lParam1);
			int CurSpace;

			if (pNumSpaces!=NULL)
				*pNumSpaces=pChannelManager->GetDriverTuningSpaceList()->NumSpaces();
			CurSpace=pChannelManager->GetCurrentSpace();
			if (CurSpace<0) {
				const CChannelInfo *pChannelInfo;

				if (CurSpace==CChannelManager::SPACE_ALL
						&& (pChannelInfo=pChannelManager->GetCurrentChannelInfo())!=NULL) {
					CurSpace=pChannelInfo->GetSpace();
				} else {
					CurSpace=-1;
				}
			}
			return CurSpace;
		}

	case TVTest::MESSAGE_GETTUNINGSPACEINFO:
		{
			int Index=(int)pParam->lParam1;
			TVTest::TuningSpaceInfo *pInfo=reinterpret_cast<TVTest::TuningSpaceInfo*>(pParam->lParam2);

			if (pInfo==NULL || pInfo->Size!=sizeof(TVTest::TuningSpaceInfo))
				return FALSE;

			const CTuningSpaceList *pTuningSpaceList=GetAppClass().ChannelManager.GetDriverTuningSpaceList();
			LPCTSTR pszTuningSpaceName=pTuningSpaceList->GetTuningSpaceName(Index);

			if (pszTuningSpaceName==NULL)
				return FALSE;
			::lstrcpyn(pInfo->szName,pszTuningSpaceName,lengthof(pInfo->szName));
			pInfo->Space=(int)pTuningSpaceList->GetTuningSpaceType(Index);
		}
		return TRUE;

	case TVTest::MESSAGE_SETAUDIOSTREAM:
		{
			CUICore *pUICore=&GetAppClass().UICore;
			int Index=(int)pParam->lParam1;

			if (Index<0 || Index>=pUICore->GetNumAudioStreams()
					|| !pUICore->SetAudioStream(Index))
				return FALSE;
		}
		return TRUE;

	case TVTest::MESSAGE_GETDRIVERFULLPATHNAME:
		{
			LPWSTR pszPath=reinterpret_cast<LPWSTR>(pParam->lParam1);
			int MaxLength=(int)pParam->lParam2;
			TCHAR szFileName[MAX_PATH];

			if (!GetAppClass().CoreEngine.GetDriverPath(szFileName,lengthof(szFileName)))
				return 0;
			if (pszPath!=NULL && MaxLength>0)
				::lstrcpyn(pszPath,szFileName,MaxLength);
			return ::lstrlen(szFileName);
		}

	case TVTest::MESSAGE_RELAYRECORD:
		{
			LPCWSTR pszFileName=reinterpret_cast<LPCWSTR>(pParam->lParam1);

			return GetAppClass().Core.RelayRecord(pszFileName);
		}

	case TVTest::MESSAGE_SETWINDOWMESSAGECALLBACK:
		pParam->pPlugin->m_pMessageCallback=reinterpret_cast<TVTest::WindowMessageCallbackFunc>(pParam->lParam1);
		pParam->pPlugin->m_pMessageCallbackClientData=reinterpret_cast<void*>(pParam->lParam2);
		return TRUE;

	case TVTest::MESSAGE_SETVIDEOSTREAM:
		return GetAppClass().CoreEngine.m_DtvEngine.SetVideoStream((int)pParam->lParam1);

	case TVTest::MESSAGE_SELECTCHANNEL:
		{
			const TVTest::ChannelSelectInfo *pInfo=
				reinterpret_cast<const TVTest::ChannelSelectInfo*>(pParam->lParam1);

			if (pInfo==NULL || pInfo->Size!=sizeof(TVTest::ChannelSelectInfo))
				return FALSE;

			CChannelInfo Channel;
			unsigned int Flags;

			Channel.SetSpace(pInfo->Space);
			Channel.SetChannelIndex(pInfo->Channel);
			Channel.SetNetworkID(pInfo->NetworkID);
			Channel.SetTransportStreamID(pInfo->TransportStreamID);
			Channel.SetServiceID(pInfo->ServiceID);
			Flags=0;
			if (pInfo->pszTuner==NULL)
				Flags|=CAppCore::SELECT_CHANNEL_USE_CUR_TUNER;
			if ((pInfo->Flags & TVTest::CHANNEL_SELECT_FLAG_STRICTSERVICE)!=0)
				Flags|=CAppCore::SELECT_CHANNEL_STRICT_SERVICE;

			return GetAppClass().Core.SelectChannel(pInfo->pszTuner,Channel,Flags);
		}

	case TVTest::MESSAGE_SET1SEGMODE:
		return GetAppClass().Core.Set1SegMode(pParam->lParam1!=0,true);

#ifdef _DEBUG
	default:
		TRACE(TEXT("CPlugin::OnPluginMessage() : Unknown message %u\n"),pParam->Message);
		break;
#endif
	}
	return 0;
}


void CALLBACK CPlugin::AudioStreamCallback(short *pData,DWORD Samples,int Channels,void *pParam)
{
	CBlockLock Lock(&m_AudioStreamLock);

	for (size_t i=0;i<m_AudioStreamCallbackList.size();i++) {
		CAudioStreamCallbackInfo &Info=m_AudioStreamCallbackList[i];

		(Info.m_pCallback)(pData,Samples,Channels,Info.m_pClientData);
	}
}


static bool GetSettingString(TVTest::SettingInfo *pSetting,LPCWSTR pszString)
{
	if (pSetting->Type!=TVTest::SETTING_TYPE_STRING)
		return false;
	if (pSetting->Value.pszString!=NULL) {
		if (pSetting->ValueSize==0)
			return false;
		::lstrcpynW(pSetting->Value.pszString,pszString,pSetting->ValueSize);
		pSetting->ValueSize=(::lstrlenW(pSetting->Value.pszString)+1)*sizeof(WCHAR);
	} else {
		pSetting->ValueSize=(::lstrlenW(pszString)+1)*sizeof(WCHAR);
	}
	return true;
}

static bool GetSettingFont(TVTest::SettingInfo *pSetting,const LOGFONT *pFont)
{
	if (pSetting->Type!=TVTest::SETTING_TYPE_DATA)
		return false;
	if (pSetting->Value.pData!=NULL) {
		if (pSetting->ValueSize!=sizeof(LOGFONT))
			return false;
		::CopyMemory(pSetting->Value.pData,pFont,sizeof(LOGFONT));
	} else {
		pSetting->ValueSize=sizeof(LOGFONT);
	}
	return true;
}

static bool GetSettingInt(TVTest::SettingInfo *pSetting,int Value)
{
	if (pSetting->Type!=TVTest::SETTING_TYPE_INT)
		return false;
	if (pSetting->ValueSize!=sizeof(int))
		return false;
	pSetting->Value.Int=Value;
	return true;
}

bool CPlugin::OnGetSetting(TVTest::SettingInfo *pSetting) const
{
	if (pSetting==NULL || pSetting->pszName==NULL)
		return false;

	CAppMain &App=GetAppClass();

	if (::lstrcmpiW(pSetting->pszName,L"DriverDirectory")==0) {
		TCHAR szDirectory[MAX_PATH];
		App.Core.GetDriverDirectory(szDirectory,lengthof(szDirectory));
		return GetSettingString(pSetting,szDirectory);
	} else if (::lstrcmpiW(pSetting->pszName,L"IniFilePath")==0) {
		return GetSettingString(pSetting,App.GetIniFileName());
	} else if (::lstrcmpiW(pSetting->pszName,L"RecordFolder")==0) {
		return GetSettingString(pSetting,App.Core.GetDefaultRecordFolder());
	} else if (::lstrcmpiW(pSetting->pszName,L"OSDFont")==0) {
		return GetSettingFont(pSetting,App.OSDOptions.GetOSDFont());
	} else if (::lstrcmpiW(pSetting->pszName,L"PanelFont")==0) {
		return GetSettingFont(pSetting,App.PanelOptions.GetFont());
	} else if (::lstrcmpiW(pSetting->pszName,L"ProgramGuideFont")==0) {
		return GetSettingFont(pSetting,&App.ProgramGuideOptions.GetFont());
	} else if (::lstrcmpiW(pSetting->pszName,L"StatusBarFont")==0) {
		return GetSettingFont(pSetting,&App.StatusOptions.GetFont());
	} else if (::lstrcmpiW(pSetting->pszName,L"DPI")==0) {
		return GetSettingInt(pSetting,App.StyleManager.GetDPI());
	}
#ifdef _DEBUG
	else {
		TRACE(TEXT("CPlugin::OnGetSettings() : Unknown setting \"%s\"\n"),pSetting->pszName);
	}
#endif

	return false;
}




CPlugin::CPluginCommandInfo::CPluginCommandInfo(int ID,LPCWSTR pszText,LPCWSTR pszName)
	: m_ID(ID)
	, m_Command(0)
	, m_Flags(0)
	, m_State(0)
	, m_Text(pszText)
	, m_Name(pszName)
{
}


CPlugin::CPluginCommandInfo::CPluginCommandInfo(const TVTest::CommandInfo &Info)
	: m_ID(Info.ID)
	, m_Command(0)
	, m_Flags(0)
	, m_State(0)
	, m_Text(Info.pszText)
	, m_Name(Info.pszName)
{
}


CPlugin::CPluginCommandInfo::CPluginCommandInfo(const TVTest::PluginCommandInfo &Info)
	: m_ID(Info.ID)
	, m_Command(0)
	, m_Flags(Info.Flags)
	, m_State(Info.State)
	, m_Text(Info.pszText)
	, m_Name(Info.pszName)
{
	m_Icon.Create(Info.hbmIcon);
}


CPlugin::CPluginCommandInfo::~CPluginCommandInfo()
{
}


CPlugin::CProgramGuideCommand::CProgramGuideCommand(const TVTest::ProgramGuideCommandInfo &Info)
	: CPluginCommandInfo(Info.ID,Info.pszText,Info.pszName)
	, m_Type(Info.Type)
{
}




CPlugin::CStreamGrabber::CStreamGrabber(TVTest::StreamCallbackFunc Callback,void *pClientData)
	: m_Callback(Callback)
	, m_pClientData(pClientData)
{
}


void CPlugin::CStreamGrabber::SetClientData(void *pClientData)
{
	m_pClientData=pClientData;
}


bool CPlugin::CStreamGrabber::OnInputMedia(CMediaData *pMediaData)
{
	return m_Callback(pMediaData->GetData(),m_pClientData)!=FALSE;
}




CPlugin::CPluginStatusItem::CPluginStatusItem(CPlugin *pPlugin,StatusItem *pItem)
	: CStatusItem(pItem->ItemID,
				  SizeValue(std::abs(pItem->DefaultWidth),
							pItem->DefaultWidth>=0?SIZE_PIXEL:SIZE_EM))
	, m_pPlugin(pPlugin)
	, m_pItem(pItem)
{
	m_MinWidth=pItem->MinWidth;
	m_MaxWidth=pItem->MaxWidth;
	m_MinHeight=pItem->MinHeight;
	m_fVisible=(pItem->State & TVTest::STATUS_ITEM_STATE_VISIBLE)!=0;
	ApplyStyle();

	m_IDText=::PathFindFileName(m_pPlugin->GetFileName());
	m_IDText+=_T(':');
	m_IDText+=pItem->IDText;

	pItem->pItem=this;
}


CPlugin::CPluginStatusItem::~CPluginStatusItem()
{
	if (m_pItem!=NULL)
		m_pItem->pItem=NULL;
}


void CPlugin::CPluginStatusItem::Draw(
	HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags)
{
	NotifyDraw(hdc,ItemRect,DrawRect,Flags);
}


void CPlugin::CPluginStatusItem::OnLButtonDown(int x,int y)
{
	NotifyMouseEvent(TVTest::STATUS_ITEM_MOUSE_ACTION_LDOWN,x,y);
}


void CPlugin::CPluginStatusItem::OnLButtonUp(int x,int y)
{
	NotifyMouseEvent(TVTest::STATUS_ITEM_MOUSE_ACTION_LUP,x,y);
}


void CPlugin::CPluginStatusItem::OnLButtonDoubleClick(int x,int y)
{
	NotifyMouseEvent(TVTest::STATUS_ITEM_MOUSE_ACTION_LDOUBLECLICK,x,y);
}


void CPlugin::CPluginStatusItem::OnRButtonDown(int x,int y)
{
	NotifyMouseEvent(TVTest::STATUS_ITEM_MOUSE_ACTION_RDOWN,x,y);
}


void CPlugin::CPluginStatusItem::OnRButtonUp(int x,int y)
{
	NotifyMouseEvent(TVTest::STATUS_ITEM_MOUSE_ACTION_RUP,x,y);
}


void CPlugin::CPluginStatusItem::OnRButtonDoubleClick(int x,int y)
{
	NotifyMouseEvent(TVTest::STATUS_ITEM_MOUSE_ACTION_RDOUBLECLICK,x,y);
}


void CPlugin::CPluginStatusItem::OnMButtonDown(int x,int y)
{
	NotifyMouseEvent(TVTest::STATUS_ITEM_MOUSE_ACTION_MDOWN,x,y);
}


void CPlugin::CPluginStatusItem::OnMButtonUp(int x,int y)
{
	NotifyMouseEvent(TVTest::STATUS_ITEM_MOUSE_ACTION_MUP,x,y);
}


void CPlugin::CPluginStatusItem::OnMButtonDoubleClick(int x,int y)
{
	NotifyMouseEvent(TVTest::STATUS_ITEM_MOUSE_ACTION_MDOUBLECLICK,x,y);
}


void CPlugin::CPluginStatusItem::OnMouseMove(int x,int y)
{
	NotifyMouseEvent(TVTest::STATUS_ITEM_MOUSE_ACTION_MOVE,x,y);
}


bool CPlugin::CPluginStatusItem::OnMouseWheel(int x,int y,bool fHorz,int Delta,int *pCommand)
{
	return NotifyMouseEvent(fHorz?
								TVTest::STATUS_ITEM_MOUSE_ACTION_HORZWHEEL:
								TVTest::STATUS_ITEM_MOUSE_ACTION_WHEEL,
							x,y,Delta)!=0;
}


void CPlugin::CPluginStatusItem::OnVisibilityChanged()
{
	if (m_pItem!=NULL) {
		if (m_fVisible)
			m_pItem->State|=TVTest::STATUS_ITEM_STATE_VISIBLE;
		else
			m_pItem->State&=~TVTest::STATUS_ITEM_STATE_VISIBLE;

		if (m_pPlugin!=NULL) {
			TVTest::StatusItemEventInfo Info;
			Info.ID=m_pItem->ID;
			Info.Event=TVTest::STATUS_ITEM_EVENT_VISIBILITYCHANGED;
			Info.Param=m_fVisible;
			m_pPlugin->SendEvent(TVTest::EVENT_STATUSITEM_NOTIFY,
								 reinterpret_cast<LPARAM>(&Info),0);
		}
	}
}


void CPlugin::CPluginStatusItem::OnFocus(bool fFocus)
{
	if (m_pItem!=NULL) {
		if (fFocus)
			m_pItem->State|=TVTest::STATUS_ITEM_STATE_HOT;
		else
			m_pItem->State&=~TVTest::STATUS_ITEM_STATE_HOT;

		if (m_pPlugin!=NULL) {
			TVTest::StatusItemEventInfo Info;
			Info.ID=m_pItem->ID;
			Info.Event=fFocus?
				TVTest::STATUS_ITEM_EVENT_ENTER:
				TVTest::STATUS_ITEM_EVENT_LEAVE;
			Info.Param=0;
			m_pPlugin->SendEvent(TVTest::EVENT_STATUSITEM_NOTIFY,
								 reinterpret_cast<LPARAM>(&Info),0);
		}
	}
}


void CPlugin::CPluginStatusItem::OnSizeChanged()
{
	if (m_pPlugin!=NULL && m_pItem!=NULL) {
		TVTest::StatusItemEventInfo Info;
		Info.ID=m_pItem->ID;
		Info.Event=TVTest::STATUS_ITEM_EVENT_SIZECHANGED;
		Info.Param=0;
		m_pPlugin->SendEvent(TVTest::EVENT_STATUSITEM_NOTIFY,
							 reinterpret_cast<LPARAM>(&Info),0);
	}
}


void CPlugin::CPluginStatusItem::OnCaptureReleased()
{
	NotifyMouseEvent(TVTest::STATUS_ITEM_MOUSE_ACTION_CAPTURERELEASE,0,0);
}


void CPlugin::CPluginStatusItem::DetachItem()
{
	m_pPlugin=NULL;
	m_pItem=NULL;
}


HWND CPlugin::CPluginStatusItem::GetWindowHandle() const
{
	if (m_pStatus==NULL)
		return NULL;
	return m_pStatus->GetHandle();
}


void CPlugin::CPluginStatusItem::ApplyStyle()
{
	if (m_pItem!=NULL) {
		m_Style=0;
		if ((m_pItem->Style & TVTest::STATUS_ITEM_STYLE_VARIABLEWIDTH)!=0)
			m_Style|=STYLE_VARIABLEWIDTH;
		if ((m_pItem->Style & TVTest::STATUS_ITEM_STYLE_FULLROW)!=0)
			m_Style|=STYLE_FULLROW;
		if ((m_pItem->Style & TVTest::STATUS_ITEM_STYLE_FORCEFULLROW)!=0)
			m_Style|=STYLE_FULLROW | STYLE_FORCEFULLROW;
	}
}


void CPlugin::CPluginStatusItem::NotifyDraw(
	HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags)
{
	if (m_pPlugin!=NULL && m_pItem!=NULL) {
		TVTest::StatusItemDrawInfo Info;

		Info.ID=m_pItem->ID;
		Info.Flags=0;
		if ((Flags & DRAW_PREVIEW)!=0)
			Info.Flags|=TVTest::STATUS_ITEM_DRAW_FLAG_PREVIEW;
		Info.State=0;
		if ((Flags & DRAW_HIGHLIGHT)!=0) {
			Info.State|=TVTest::STATUS_ITEM_DRAW_STATE_HOT;
			Info.pszStyle=L"status-bar.item.hot";
		} else if ((Flags & DRAW_BOTTOM)!=0) {
			Info.pszStyle=L"status-bar.item.bottom";
		} else {
			Info.pszStyle=L"status-bar.item";
		}
		Info.hdc=hdc;
		Info.ItemRect=ItemRect;
		Info.DrawRect=DrawRect;
		Info.Color=::GetTextColor(hdc);

		m_pPlugin->SendEvent(TVTest::EVENT_STATUSITEM_DRAW,
							 reinterpret_cast<LPARAM>(&Info),0);
	}
}


LRESULT CPlugin::CPluginStatusItem::NotifyMouseEvent(UINT Action,int x,int y,int WheelDelta)
{
	if (m_pPlugin!=NULL && m_pItem!=NULL) {
		TVTest::StatusItemMouseEventInfo Info;

		Info.ID=m_pItem->ID;
		Info.Action=Action;
		Info.hwnd=m_pStatus->GetHandle();
		Info.CursorPos.x=x;
		Info.CursorPos.y=y;
		GetRect(&Info.ItemRect);
		Info.CursorPos.x+=Info.ItemRect.left;
		Info.CursorPos.y+=Info.ItemRect.top;
		GetClientRect(&Info.ContentRect);
		Info.WheelDelta=WheelDelta;

		return m_pPlugin->SendEvent(TVTest::EVENT_STATUSITEM_MOUSE,
									reinterpret_cast<LPARAM>(&Info),0);
	}

	return 0;
}




bool CPlugin::CPluginPanelItem::m_fInitialized=false;


CPlugin::CPluginPanelItem::CPluginPanelItem(CPlugin *pPlugin,PanelItem *pItem)
	: m_pPlugin(pPlugin)
	, m_pItem(pItem)
	, m_hwndItem(NULL)
{
	pItem->pItem=this;
}


CPlugin::CPluginPanelItem::~CPluginPanelItem()
{
	if (m_pItem!=NULL)
		m_pItem->pItem=NULL;
}


bool CPlugin::CPluginPanelItem::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	static const LPCTSTR CLASS_NAME=TEXT("TVTest Plugin Panel");

	if (m_pPlugin==NULL || m_pItem==NULL)
		return false;

	HINSTANCE hinst=GetAppClass().GetInstance();

	if (!m_fInitialized) {
		WNDCLASS wc;

		wc.style=0;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=::LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=CLASS_NAME;
		if (::RegisterClass(&wc)==0)
			return false;
		m_fInitialized=true;
	}

	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 CLASS_NAME,m_pItem->Title.c_str(),hinst);
}


void CPlugin::CPluginPanelItem::DetachItem()
{
	m_pPlugin=NULL;
	m_pItem=NULL;
	m_hwndItem=NULL;
}


LRESULT CPlugin::CPluginPanelItem::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			TVTest::PanelItemCreateEventInfo CreateEvent;

			CreateEvent.EventInfo.ID=m_pItem->ID;
			CreateEvent.EventInfo.Event=TVTest::PANEL_ITEM_EVENT_CREATE;
			GetAppClass().Panel.Form.GetPageClientRect(&CreateEvent.ItemRect);
			CreateEvent.hwndParent=hwnd;
			CreateEvent.hwndItem=NULL;

			if (m_pPlugin->SendEvent(TVTest::EVENT_PANELITEM_NOTIFY,
									 reinterpret_cast<LPARAM>(&CreateEvent),0)!=0)
				m_hwndItem=CreateEvent.hwndItem;
		}
		return 0;

	case WM_SIZE:
		if (m_hwndItem!=NULL)
			::MoveWindow(m_hwndItem,0,0,LOWORD(lParam),HIWORD(lParam),TRUE);
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd,&ps);
			DrawUtil::Fill(ps.hdc,&ps.rcPaint,
				GetAppClass().ColorSchemeOptions.GetColor(CColorScheme::COLOR_PANELBACK));
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_SETFOCUS:
		if (m_hwndItem!=NULL)
			::SetFocus(m_hwndItem);
		return 0;
	}

	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


void CPlugin::CPluginPanelItem::OnActivate()
{
	if (m_pItem!=NULL) {
		m_pItem->State|=TVTest::PANEL_ITEM_STATE_ACTIVE;

		if (m_pPlugin!=NULL) {
			TVTest::PanelItemEventInfo Info;
			Info.ID=m_pItem->ID;
			Info.Event=TVTest::PANEL_ITEM_EVENT_ACTIVATE;
			m_pPlugin->SendEvent(TVTest::EVENT_PANELITEM_NOTIFY,
								 reinterpret_cast<LPARAM>(&Info),0);
		}
	}
}


void CPlugin::CPluginPanelItem::OnDeactivate()
{
	if (m_pItem!=NULL) {
		m_pItem->State&=~TVTest::PANEL_ITEM_STATE_ACTIVE;

		if (m_pPlugin!=NULL) {
			TVTest::PanelItemEventInfo Info;
			Info.ID=m_pItem->ID;
			Info.Event=TVTest::PANEL_ITEM_EVENT_DEACTIVATE;
			m_pPlugin->SendEvent(TVTest::EVENT_PANELITEM_NOTIFY,
								 reinterpret_cast<LPARAM>(&Info),0);
		}
	}
}


void CPlugin::CPluginPanelItem::OnVisibilityChanged(bool fVisible)
{
	if (m_pItem!=NULL) {
		if (fVisible)
			m_pItem->State|=TVTest::PANEL_ITEM_STATE_ENABLED;
		else
			m_pItem->State&=~TVTest::PANEL_ITEM_STATE_ENABLED;

		if (m_pPlugin!=NULL) {
			TVTest::PanelItemEventInfo Info;
			Info.ID=m_pItem->ID;
			Info.Event=fVisible?
				TVTest::PANEL_ITEM_EVENT_ENABLE:
				TVTest::PANEL_ITEM_EVENT_DISABLE;
			m_pPlugin->SendEvent(TVTest::EVENT_PANELITEM_NOTIFY,
								 reinterpret_cast<LPARAM>(&Info),0);
		}
	}
}


void CPlugin::CPluginPanelItem::OnFormDelete()
{
	delete this;
}


bool CPlugin::CPluginPanelItem::DrawIcon(
	HDC hdc,int x,int y,int Width,int Height,const TVTest::Theme::ThemeColor &Color)
{
	if (m_pItem==NULL || !m_pItem->Icon.IsCreated())
		return false;

	return m_pItem->Icon.Draw(hdc,x,y,Width,Height,0,0,0,0,Color);
}


bool CPlugin::CPluginPanelItem::NeedKeyboardFocus() const
{
	return m_pItem!=NULL && (m_pItem->Style & TVTest::PANEL_ITEM_STYLE_NEEDFOCUS)!=0;
}




CPluginManager::CPluginManager()
{
}


CPluginManager::~CPluginManager()
{
	FreePlugins();
}


void CPluginManager::SortPluginsByName()
{
	if (m_PluginList.size()>1)
		std::sort(m_PluginList.begin(),m_PluginList.end(),CompareName);
}


bool CPluginManager::CompareName(const CPlugin *pPlugin1,const CPlugin *pPlugin2)
{
	int Cmp;

	Cmp=::lstrcmpi(pPlugin1->GetPluginName(),pPlugin2->GetPluginName());
	if (Cmp==0)
		Cmp=::lstrcmpi(pPlugin1->GetFileName(),pPlugin2->GetFileName());
	return Cmp<0;
}


bool CPluginManager::LoadPlugins(LPCTSTR pszDirectory,const std::vector<LPCTSTR> *pExcludePlugins)
{
	if (pszDirectory==NULL)
		return false;
	const int DirectoryLength=::lstrlen(pszDirectory);
	if (DirectoryLength+7>=MAX_PATH)	// +7 = "\\*.tvtp"
		return false;

	FreePlugins();

	CAppMain &App=GetAppClass();
	TCHAR szFileName[MAX_PATH];
	HANDLE hFind;
	WIN32_FIND_DATA wfd;

	::PathCombine(szFileName,pszDirectory,TEXT("*.tvtp"));
	hFind=::FindFirstFile(szFileName,&wfd);
	if (hFind!=INVALID_HANDLE_VALUE) {
		do {
			if (pExcludePlugins!=NULL) {
				bool fExclude=false;
				for (size_t i=0;i<pExcludePlugins->size();i++) {
					if (IsEqualFileName((*pExcludePlugins)[i],wfd.cFileName)) {
						fExclude=true;
						break;
					}
				}
				if (fExclude) {
					App.AddLog(TEXT("%s は除外指定されているため読み込まれません。"),
							   wfd.cFileName);
					continue;
				}
			}

			if (DirectoryLength+1+::lstrlen(wfd.cFileName)>=MAX_PATH)
				continue;

			CPlugin *pPlugin=new CPlugin;

			::PathCombine(szFileName,pszDirectory,wfd.cFileName);
			if (pPlugin->Load(szFileName)) {
				App.AddLog(TEXT("%s を読み込みました。"),wfd.cFileName);
				m_PluginList.push_back(pPlugin);
			} else {
				App.AddLog(
					CLogItem::TYPE_ERROR,
					TEXT("%s : %s"),
					wfd.cFileName,
					!IsStringEmpty(pPlugin->GetLastErrorText())?
						pPlugin->GetLastErrorText():
						TEXT("プラグインを読み込めません。"));
				if (!IsStringEmpty(pPlugin->GetLastErrorAdvise()))
					App.AddLog(CLogItem::TYPE_ERROR,TEXT("(%s)"),pPlugin->GetLastErrorAdvise());
				delete pPlugin;
			}
		} while (::FindNextFile(hFind,&wfd));
		::FindClose(hFind);
	}
	SortPluginsByName();
	for (size_t i=0;i<m_PluginList.size();i++)
		m_PluginList[i]->SetCommand(CM_PLUGIN_FIRST+(int)i);
	return true;
}


void CPluginManager::FreePlugins()
{
	for (std::vector<CPlugin*>::iterator i=m_PluginList.begin();i!=m_PluginList.end();) {
		delete *i;
		i=m_PluginList.erase(i);
	}
}


CPlugin *CPluginManager::GetPlugin(int Index)
{
	if (Index<0 || (size_t)Index>=m_PluginList.size())
		return NULL;
	return m_PluginList[Index];
}


const CPlugin *CPluginManager::GetPlugin(int Index) const
{
	if (Index<0 || (size_t)Index>=m_PluginList.size())
		return NULL;
	return m_PluginList[Index];
}


bool CPluginManager::EnablePlugins(bool fEnable)
{
	for (size_t i=0;i<m_PluginList.size();i++) {
		m_PluginList[i]->Enable(fEnable);
	}
	return true;
}


int CPluginManager::FindPlugin(const CPlugin *pPlugin) const
{
	for (size_t i=0;i<m_PluginList.size();i++) {
		if (m_PluginList[i]==pPlugin)
			return (int)i;
	}
	return -1;
}


int CPluginManager::FindPluginByFileName(LPCTSTR pszFileName) const
{
	if (pszFileName==NULL)
		return -1;
	for (size_t i=0;i<m_PluginList.size();i++) {
		if (IsEqualFileName(::PathFindFileName(m_PluginList[i]->GetFileName()),pszFileName))
			return (int)i;
	}
	return -1;
}


int CPluginManager::FindPluginByCommand(int Command) const
{
	for (size_t i=0;i<m_PluginList.size();i++) {
		if (m_PluginList[i]->GetCommand()==Command)
			return (int)i;
	}
	return -1;
}


CPlugin *CPluginManager::GetPluginByCommand(int Command)
{
	int Index=FindPluginByCommand(Command);
	if (Index<0)
		return NULL;
	return GetPlugin(Index);
}


CPlugin *CPluginManager::GetPluginByPluginCommand(LPCTSTR pszCommand,LPCTSTR *ppszCommandText)
{
	if (IsStringEmpty(pszCommand))
		return NULL;

	LPCTSTR pDelimiter=::StrChr(pszCommand,_T(':'));
	if (pDelimiter==NULL || (pDelimiter-pszCommand)>=MAX_PATH)
		return NULL;

	TCHAR szFileName[MAX_PATH];
	::lstrcpyn(szFileName,pszCommand,(int)((pDelimiter-pszCommand)+1));

	int PluginIndex=FindPluginByFileName(szFileName);
	if (PluginIndex<0)
		return NULL;

	if (ppszCommandText!=NULL)
		*ppszCommandText=pDelimiter+1;

	return m_PluginList[PluginIndex];
}


bool CPluginManager::DeletePlugin(int Index)
{
	if (Index<0 || (size_t)Index>=m_PluginList.size())
		return false;
	std::vector<CPlugin*>::iterator i=m_PluginList.begin();
	std::advance(i,Index);
	delete *i;
	m_PluginList.erase(i);
	return true;
}


bool CPluginManager::SetMenu(HMENU hmenu) const
{
	ClearMenu(hmenu);
	if (NumPlugins()>0) {
		for (size_t i=0;i<m_PluginList.size();i++) {
			const CPlugin *pPlugin=m_PluginList[i];

			if (!pPlugin->IsNoEnabledDisabled()) {
				::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED |
							 (pPlugin->IsEnabled()?MFS_CHECKED:MFS_UNCHECKED),
							 pPlugin->GetCommand(),pPlugin->GetPluginName());
			}
		}
	} else {
		::AppendMenu(hmenu,MFT_STRING | MFS_GRAYED,0,TEXT("なし"));
	}
	return true;
}


bool CPluginManager::OnPluginCommand(LPCTSTR pszCommand)
{
	LPCTSTR pszCommandText;
	CPlugin *pPlugin=GetPluginByPluginCommand(pszCommand,&pszCommandText);

	if (pPlugin==NULL)
		return false;

	return pPlugin->NotifyCommand(pszCommandText);
}


bool CPluginManager::OnProgramGuideCommand(LPCTSTR pszCommand,UINT Action,const CEventInfoData *pEvent,
										   const POINT *pCursorPos,const RECT *pItemRect)
{
	if (pszCommand==NULL)
		return false;

	LPCTSTR pDelimiter=::StrChr(pszCommand,_T(':'));
	if (pDelimiter==NULL || (pDelimiter-pszCommand)>=MAX_PATH)
		return false;

	TCHAR szFileName[MAX_PATH];
	::lstrcpyn(szFileName,pszCommand,(int)((pDelimiter-pszCommand)+1));

	int PluginIndex=FindPluginByFileName(szFileName);
	if (PluginIndex<0)
		return false;
	CPlugin *pPlugin=m_PluginList[PluginIndex];
	if (!pPlugin->IsEnabled()
			|| !pPlugin->IsProgramGuideEventEnabled(TVTest::PROGRAMGUIDE_EVENT_GENERAL))
		return false;
	return pPlugin->NotifyProgramGuideCommand(pDelimiter+1,Action,pEvent,pCursorPos,pItemRect);
}


bool CPluginManager::SendEvent(UINT Event,LPARAM lParam1,LPARAM lParam2)
{
	for (size_t i=0;i<m_PluginList.size();i++) {
		m_PluginList[i]->SendEvent(Event,lParam1,lParam2);
	}
	return true;
}


bool CPluginManager::SendProgramGuideEvent(UINT Event,LPARAM Param1,LPARAM Param2)
{
	bool fSent=false;

	for (size_t i=0;i<m_PluginList.size();i++) {
		CPlugin *pPlugin=m_PluginList[i];

		if (pPlugin->IsProgramGuideEventEnabled(TVTest::PROGRAMGUIDE_EVENT_GENERAL)
				&& pPlugin->SendEvent(Event,Param1,Param2))
			fSent=true;
	}
	return fSent;
}


bool CPluginManager::SendProgramGuideProgramEvent(UINT Event,const CEventInfoData &EventInfo,LPARAM Param)
{
	TVTest::ProgramGuideProgramInfo ProgramInfo;
	EventInfoToProgramGuideProgramInfo(EventInfo,&ProgramInfo);

	bool fSent=false;

	for (size_t i=0;i<m_PluginList.size();i++) {
		CPlugin *pPlugin=m_PluginList[i];

		if (pPlugin->IsProgramGuideEventEnabled(TVTest::PROGRAMGUIDE_EVENT_PROGRAM)
				&& pPlugin->SendEvent(Event,reinterpret_cast<LPARAM>(&ProgramInfo),Param))
			fSent=true;
	}
	return fSent;
}


bool CPluginManager::SendFilterGraphEvent(
	UINT Event,CMediaViewer *pMediaViewer,IGraphBuilder *pGraphBuilder)
{
	TVTest::FilterGraphInfo Info;

	Info.Flags=0;
	Info.VideoStreamType=pMediaViewer->GetVideoStreamType();
	::ZeroMemory(Info.Reserved,sizeof(Info.Reserved));
	Info.pGraphBuilder=pGraphBuilder;
	return SendEvent(Event,reinterpret_cast<LPARAM>(&Info));
}


void CPluginManager::OnTunerChanged()
{
	SendEvent(TVTest::EVENT_DRIVERCHANGE);
}


void CPluginManager::OnTunerShutDown()
{
	SendEvent(TVTest::EVENT_DRIVERCHANGE);
}


void CPluginManager::OnChannelChanged(unsigned int Status)
{
	SendEvent(TVTest::EVENT_CHANNELCHANGE);
}


void CPluginManager::OnServiceChanged()
{
	SendEvent(TVTest::EVENT_SERVICECHANGE);
}


void CPluginManager::OnServiceInfoUpdated()
{
	SendEvent(TVTest::EVENT_SERVICEUPDATE);
}


void CPluginManager::OnServiceListUpdated()
{
	SendEvent(TVTest::EVENT_SERVICEUPDATE);
}


void CPluginManager::OnRecordingStart(TVTest::AppEvent::RecordingStartInfo *pInfo)
{
	const CRecordManager *pRecordManager=pInfo->pRecordManager;
	TVTest::StartRecordInfo Info;

	Info.Size=sizeof(TVTest::StartRecordInfo);
	Info.Flags=0;
	Info.Modified=0;
	Info.Client=(DWORD)pRecordManager->GetClient();
	Info.pszFileName=pInfo->pszFileName;
	Info.MaxFileName=pInfo->MaxFileName;
	CRecordManager::TimeSpecInfo TimeSpec;
	pRecordManager->GetStartTimeSpec(&TimeSpec);
	switch (TimeSpec.Type) {
	case CRecordManager::TIME_NOTSPECIFIED:
		Info.StartTimeSpec=TVTest::RECORD_START_NOTSPECIFIED;
		break;
	case CRecordManager::TIME_DATETIME:
		Info.StartTimeSpec=TVTest::RECORD_START_TIME;
		break;
	case CRecordManager::TIME_DURATION:
		Info.StartTimeSpec=TVTest::RECORD_START_DELAY;
		break;
	}
	if (TimeSpec.Type!=CRecordManager::TIME_NOTSPECIFIED)
		pRecordManager->GetReservedStartTime(&Info.StartTime);
	pRecordManager->GetStopTimeSpec(&TimeSpec);
	switch (TimeSpec.Type) {
	case CRecordManager::TIME_NOTSPECIFIED:
		Info.StopTimeSpec=TVTest::RECORD_STOP_NOTSPECIFIED;
		break;
	case CRecordManager::TIME_DATETIME:
		Info.StopTimeSpec=TVTest::RECORD_STOP_TIME;
		Info.StopTime.Time=TimeSpec.Time.DateTime;
		break;
	case CRecordManager::TIME_DURATION:
		Info.StopTimeSpec=TVTest::RECORD_STOP_DURATION;
		Info.StopTime.Duration=TimeSpec.Time.Duration;
		break;
	}

	SendEvent(TVTest::EVENT_STARTRECORD,reinterpret_cast<LPARAM>(&Info));
}


void CPluginManager::OnRecordingStarted()
{
	OnRecordingStateChanged();
}


void CPluginManager::OnRecordingStopped()
{
	OnRecordingStateChanged();
}


void CPluginManager::OnRecordingPaused()
{
	OnRecordingStateChanged();
}


void CPluginManager::OnRecordingResumed()
{
	OnRecordingStateChanged();
}


void CPluginManager::OnRecordingStateChanged()
{
	const CRecordManager *pRecordManager=&GetAppClass().RecordManager;
	int Status;

	if (pRecordManager->IsRecording()) {
		if (pRecordManager->IsPaused())
			Status=TVTest::RECORD_STATUS_PAUSED;
		else
			Status=TVTest::RECORD_STATUS_RECORDING;
	} else {
		Status=TVTest::RECORD_STATUS_NOTRECORDING;
	}
	SendEvent(TVTest::EVENT_RECORDSTATUSCHANGE,Status);
}


void CPluginManager::OnRecordingFileChanged(LPCTSTR pszFileName)
{
	SendEvent(TVTest::EVENT_RELAYRECORD,reinterpret_cast<LPARAM>(pszFileName));
}


void CPluginManager::On1SegModeChanged(bool f1SegMode)
{
	SendEvent(TVTest::EVENT_1SEGMODECHANGED,f1SegMode);
}


void CPluginManager::OnFullscreenChanged(bool fFullscreen)
{
	SendEvent(TVTest::EVENT_FULLSCREENCHANGE,fFullscreen);
}


void CPluginManager::OnPlaybackStateChanged(bool fPlayback)
{
	SendEvent(TVTest::EVENT_PREVIEWCHANGE,fPlayback);
}


void CPluginManager::OnVolumeChanged(int Volume)
{
	SendEvent(TVTest::EVENT_VOLUMECHANGE,Volume,false);
}


void CPluginManager::OnMuteChanged(bool fMute)
{
	SendEvent(TVTest::EVENT_VOLUMECHANGE,GetAppClass().UICore.GetVolume(),fMute);
}


void CPluginManager::OnDualMonoModeChanged(CAudioDecFilter::DualMonoMode Mode)
{
	int StereoMode;

	switch (Mode) {
	case CAudioDecFilter::DUALMONO_MAIN:
		StereoMode=TVTest::STEREOMODE_LEFT;
		break;
	case CAudioDecFilter::DUALMONO_SUB:
		StereoMode=TVTest::STEREOMODE_RIGHT;
		break;
	case CAudioDecFilter::DUALMONO_BOTH:
		StereoMode=TVTest::STEREOMODE_STEREO;
		break;
	default:
		return;
	}

	SendEvent(TVTest::EVENT_STEREOMODECHANGE,StereoMode);
}


void CPluginManager::OnStereoModeChanged(CAudioDecFilter::StereoMode Mode)
{
	int StereoMode;

	switch (Mode) {
	case CAudioDecFilter::STEREOMODE_STEREO:
		StereoMode=TVTest::STEREOMODE_STEREO;
		break;
	case CAudioDecFilter::STEREOMODE_LEFT:
		StereoMode=TVTest::STEREOMODE_LEFT;
		break;
	case CAudioDecFilter::STEREOMODE_RIGHT:
		StereoMode=TVTest::STEREOMODE_RIGHT;
		break;
	default:
		return;
	}

	SendEvent(TVTest::EVENT_STEREOMODECHANGE,StereoMode);
}


void CPluginManager::OnAudioStreamChanged(int Stream)
{
	SendEvent(TVTest::EVENT_AUDIOSTREAMCHANGE,Stream);
}


void CPluginManager::OnColorSchemeChanged()
{
	SendEvent(TVTest::EVENT_COLORCHANGE);
}


void CPluginManager::OnStandbyChanged(bool fStandby)
{
	SendEvent(TVTest::EVENT_STANDBY,fStandby);
}


void CPluginManager::OnExecute(LPCTSTR pszCommandLine)
{
	SendEvent(TVTest::EVENT_EXECUTE,reinterpret_cast<LPARAM>(pszCommandLine));
}


void CPluginManager::OnEngineReset()
{
	SendEvent(TVTest::EVENT_RESET);
}


void CPluginManager::OnStatisticsReset()
{
	SendEvent(TVTest::EVENT_STATUSRESET);
}


void CPluginManager::OnSettingsChanged()
{
	SendEvent(TVTest::EVENT_SETTINGSCHANGE);
}


void CPluginManager::OnClose()
{
	SendEvent(TVTest::EVENT_CLOSE);
}


void CPluginManager::OnStartupDone()
{
	SendEvent(TVTest::EVENT_STARTUPDONE);
}


void CPluginManager::OnFavoritesChanged()
{
	SendEvent(TVTest::EVENT_FAVORITESCHANGED);
}


bool CPluginManager::SendProgramGuideInitializeEvent(HWND hwnd)
{
	return SendProgramGuideEvent(TVTest::EVENT_PROGRAMGUIDE_INITIALIZE,
								 reinterpret_cast<LPARAM>(hwnd));
}


bool CPluginManager::SendProgramGuideFinalizeEvent(HWND hwnd)
{
	return SendProgramGuideEvent(TVTest::EVENT_PROGRAMGUIDE_FINALIZE,
								 reinterpret_cast<LPARAM>(hwnd));
}


bool CPluginManager::SendProgramGuideInitializeMenuEvent(HMENU hmenu,UINT *pCommand)
{
	TVTest::ProgramGuideInitializeMenuInfo Info;
	Info.hmenu=hmenu;
	Info.Command=*pCommand;
	Info.Reserved=0;

	bool fSent=false;
	m_ProgramGuideMenuList.clear();

	for (size_t i=0;i<m_PluginList.size();i++) {
		CPlugin *pPlugin=m_PluginList[i];

		if (pPlugin->IsProgramGuideEventEnabled(TVTest::PROGRAMGUIDE_EVENT_GENERAL)) {
			MenuCommandInfo CommandInfo;

			int NumCommands=(int)
				pPlugin->SendEvent(TVTest::EVENT_PROGRAMGUIDE_INITIALIZEMENU,
								   reinterpret_cast<LPARAM>(&Info));
			if (NumCommands>0) {
				CommandInfo.pPlugin=pPlugin;
				CommandInfo.CommandFirst=Info.Command;
				CommandInfo.CommandEnd=Info.Command+NumCommands;
				m_ProgramGuideMenuList.push_back(CommandInfo);
				fSent=true;
				Info.Command+=NumCommands;
			}
		}
	}
	if (fSent)
		*pCommand=Info.Command;
	return fSent;
}


bool CPluginManager::SendProgramGuideMenuSelectedEvent(UINT Command)
{
	bool fResult=false;

	for (size_t i=0;i<m_ProgramGuideMenuList.size();i++) {
		const MenuCommandInfo &CommandInfo=m_ProgramGuideMenuList[i];

		if (CommandInfo.CommandFirst<=Command && CommandInfo.CommandEnd>Command) {
			if (FindPlugin(CommandInfo.pPlugin)>=0) {
				fResult=CommandInfo.pPlugin->SendEvent(
					TVTest::EVENT_PROGRAMGUIDE_MENUSELECTED,
					Command-CommandInfo.CommandFirst)!=0;
			}
			break;
		}
	}
	m_ProgramGuideMenuList.clear();
	return fResult;
}


bool CPluginManager::SendProgramGuideProgramDrawBackgroundEvent(const CEventInfoData &Event,HDC hdc,
	const RECT &ItemRect,const RECT &TitleRect,const RECT &ContentRect,COLORREF BackgroundColor)
{
	TVTest::ProgramGuideProgramDrawBackgroundInfo Info;

	Info.hdc=hdc;
	Info.ItemRect=ItemRect;
	Info.TitleRect=TitleRect;
	Info.ContentRect=ContentRect;
	Info.BackgroundColor=BackgroundColor;
	return SendProgramGuideProgramEvent(TVTest::EVENT_PROGRAMGUIDE_PROGRAM_DRAWBACKGROUND,
										Event,reinterpret_cast<LPARAM>(&Info));
}


bool CPluginManager::SendProgramGuideProgramInitializeMenuEvent(const CEventInfoData &Event,
	HMENU hmenu,UINT *pCommand,const POINT &CursorPos,const RECT &ItemRect)
{
	TVTest::ProgramGuideProgramInfo ProgramInfo;
	EventInfoToProgramGuideProgramInfo(Event,&ProgramInfo);

	TVTest::ProgramGuideProgramInitializeMenuInfo Info;
	Info.hmenu=hmenu;
	Info.Command=*pCommand;
	Info.Reserved=0;
	Info.CursorPos=CursorPos;
	Info.ItemRect=ItemRect;

	bool fSent=false;
	m_ProgramGuideMenuList.clear();

	for (size_t i=0;i<m_PluginList.size();i++) {
		CPlugin *pPlugin=m_PluginList[i];

		if (pPlugin->IsProgramGuideEventEnabled(TVTest::PROGRAMGUIDE_EVENT_PROGRAM)) {
			MenuCommandInfo CommandInfo;

			int NumCommands=(int)
				pPlugin->SendEvent(TVTest::EVENT_PROGRAMGUIDE_PROGRAM_INITIALIZEMENU,
								   reinterpret_cast<LPARAM>(&ProgramInfo),
								   reinterpret_cast<LPARAM>(&Info));
			if (NumCommands>0) {
				CommandInfo.pPlugin=pPlugin;
				CommandInfo.CommandFirst=Info.Command;
				CommandInfo.CommandEnd=Info.Command+NumCommands;
				m_ProgramGuideMenuList.push_back(CommandInfo);
				fSent=true;
				Info.Command+=NumCommands;
			}
		}
	}
	if (fSent)
		*pCommand=Info.Command;
	return fSent;
}


bool CPluginManager::SendProgramGuideProgramMenuSelectedEvent(const CEventInfoData &Event,UINT Command)
{
	bool fResult=false;

	for (size_t i=0;i<m_ProgramGuideMenuList.size();i++) {
		const MenuCommandInfo &CommandInfo=m_ProgramGuideMenuList[i];

		if (CommandInfo.CommandFirst<=Command && CommandInfo.CommandEnd>Command) {
			if (FindPlugin(CommandInfo.pPlugin)>=0) {
				TVTest::ProgramGuideProgramInfo ProgramInfo;

				EventInfoToProgramGuideProgramInfo(Event,&ProgramInfo);
				fResult=CommandInfo.pPlugin->SendEvent(
					TVTest::EVENT_PROGRAMGUIDE_PROGRAM_MENUSELECTED,
					reinterpret_cast<LPARAM>(&ProgramInfo),
					Command-CommandInfo.CommandFirst)!=0;
			}
			break;
		}
	}
	m_ProgramGuideMenuList.clear();
	return fResult;
}


bool CPluginManager::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,LRESULT *pResult)
{
	bool fNoDefault=false;

	for (size_t i=0;i<m_PluginList.size();i++) {
		if (m_PluginList[i]->OnMessage(hwnd,uMsg,wParam,lParam,pResult))
			fNoDefault=true;
	}
	return fNoDefault;
}


void CPluginManager::SendFilterGraphInitializeEvent(
	CMediaViewer *pMediaViewer,IGraphBuilder *pGraphBuilder)
{
	SendFilterGraphEvent(TVTest::EVENT_FILTERGRAPH_INITIALIZE,pMediaViewer,pGraphBuilder);
}


void CPluginManager::SendFilterGraphInitializedEvent(
	CMediaViewer *pMediaViewer,IGraphBuilder *pGraphBuilder)
{
	SendFilterGraphEvent(TVTest::EVENT_FILTERGRAPH_INITIALIZED,pMediaViewer,pGraphBuilder);
}


void CPluginManager::SendFilterGraphFinalizeEvent(
	CMediaViewer *pMediaViewer,IGraphBuilder *pGraphBuilder)
{
	SendFilterGraphEvent(TVTest::EVENT_FILTERGRAPH_FINALIZE,pMediaViewer,pGraphBuilder);
}


void CPluginManager::SendFilterGraphFinalizedEvent(
	CMediaViewer *pMediaViewer,IGraphBuilder *pGraphBuilder)
{
	SendFilterGraphEvent(TVTest::EVENT_FILTERGRAPH_FINALIZED,pMediaViewer,pGraphBuilder);
}


void CPluginManager::RegisterStatusItems()
{
	for (auto itr=m_PluginList.begin();itr!=m_PluginList.end();++itr)
		(*itr)->RegisterStatusItems();
}


void CPluginManager::SendStatusItemCreatedEvent()
{
	for (auto itr=m_PluginList.begin();itr!=m_PluginList.end();++itr) 
		(*itr)->SendStatusItemCreatedEvent();
}


void CPluginManager::SendStatusItemUpdateTimerEvent()
{
	for (auto itr=m_PluginList.begin();itr!=m_PluginList.end();++itr) 
		(*itr)->SendStatusItemUpdateTimerEvent();
}


void CPluginManager::RegisterPanelItems()
{
	for (auto itr=m_PluginList.begin();itr!=m_PluginList.end();++itr)
		(*itr)->RegisterPanelItems();
}




CPluginOptions::CPluginOptions(CPluginManager *pPluginManager)
	: m_pPluginManager(pPluginManager)
{
}


CPluginOptions::~CPluginOptions()
{
	Destroy();
	ClearList();
}


bool CPluginOptions::LoadSettings(CSettings &Settings)
{
	if (Settings.SetSection(TEXT("PluginList"))) {
		int Count;

		if (Settings.Read(TEXT("PluginCount"),&Count) && Count>0) {
			for (int i=0;i<Count;i++) {
				TCHAR szName[32],szFileName[MAX_PATH];

				::wsprintf(szName,TEXT("Plugin%d_Name"),i);
				if (!Settings.Read(szName,szFileName,lengthof(szFileName)))
					break;
				if (szFileName[0]!='\0') {
					bool fEnable;

					::wsprintf(szName,TEXT("Plugin%d_Enable"),i);
					if (Settings.Read(szName,&fEnable) && fEnable) {
						m_EnablePluginList.push_back(DuplicateString(szFileName));
					}
				}
			}
		}
	}

	return true;
}


bool CPluginOptions::SaveSettings(CSettings &Settings)
{
	if (!Settings.SetSection(TEXT("PluginList")))
		return false;

	Settings.Clear();
	Settings.Write(TEXT("PluginCount"),(unsigned int)m_EnablePluginList.size());
	for (size_t i=0;i<m_EnablePluginList.size();i++) {
		TCHAR szName[32];

		::wsprintf(szName,TEXT("Plugin%d_Name"),i);
		Settings.Write(szName,m_EnablePluginList[i]);
		::wsprintf(szName,TEXT("Plugin%d_Enable"),i);
		Settings.Write(szName,true);
	}

	return true;
}


bool CPluginOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_PLUGIN));
}


bool CPluginOptions::RestorePluginOptions()
{
	for (size_t i=0;i<m_EnablePluginList.size();i++) {
		for (int j=0;j<m_pPluginManager->NumPlugins();j++) {
			CPlugin *pPlugin=m_pPluginManager->GetPlugin(j);

			if (!pPlugin->IsDisableOnStart()
					&& IsEqualFileName(m_EnablePluginList[i],::PathFindFileName(pPlugin->GetFileName())))
				pPlugin->Enable(true);
		}
	}
	return true;
}


bool CPluginOptions::StorePluginOptions()
{
	ClearList();
	for (int i=0;i<m_pPluginManager->NumPlugins();i++) {
		const CPlugin *pPlugin=m_pPluginManager->GetPlugin(i);

		if (pPlugin->IsEnabled()) {
			m_EnablePluginList.push_back(DuplicateString(::PathFindFileName(pPlugin->GetFileName())));
		}
	}
	return true;
}


void CPluginOptions::ClearList()
{
	for (size_t i=0;i<m_EnablePluginList.size();i++) {
		delete [] m_EnablePluginList[i];
	}
	m_EnablePluginList.clear();
}


INT_PTR CPluginOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			HWND hwndList=GetDlgItem(hDlg,IDC_PLUGIN_LIST);
			LV_COLUMN lvc;
			int i;

			ListView_SetExtendedListViewStyle(hwndList,LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
			lvc.mask=LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			lvc.fmt=LVCFMT_LEFT;
			lvc.cx=120;
			lvc.pszText=TEXT("ファイル名");
			ListView_InsertColumn(hwndList,0,&lvc);
			lvc.pszText=TEXT("プラグイン名");
			ListView_InsertColumn(hwndList,1,&lvc);
			lvc.pszText=TEXT("説明");
			ListView_InsertColumn(hwndList,2,&lvc);
			lvc.pszText=TEXT("著作権");
			ListView_InsertColumn(hwndList,3,&lvc);
			for (i=0;i<m_pPluginManager->NumPlugins();i++) {
				const CPlugin *pPlugin=m_pPluginManager->GetPlugin(i);
				LV_ITEM lvi;

				lvi.mask=LVIF_TEXT | LVIF_PARAM;
				lvi.iItem=i;
				lvi.iSubItem=0;
				lvi.pszText=::PathFindFileName(pPlugin->GetFileName());
				lvi.lParam=reinterpret_cast<LPARAM>(pPlugin);
				ListView_InsertItem(hwndList,&lvi);
				lvi.mask=LVIF_TEXT;
				lvi.iSubItem=1;
				lvi.pszText=const_cast<LPTSTR>(pPlugin->GetPluginName());
				ListView_SetItem(hwndList,&lvi);
				lvi.iSubItem=2;
				lvi.pszText=const_cast<LPTSTR>(pPlugin->GetDescription());
				ListView_SetItem(hwndList,&lvi);
				lvi.iSubItem=3;
				lvi.pszText=const_cast<LPTSTR>(pPlugin->GetCopyright());
				ListView_SetItem(hwndList,&lvi);
			}
			for (i=0;i<4;i++)
				ListView_SetColumnWidth(hwndList,i,LVSCW_AUTOSIZE_USEHEADER);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_PLUGIN_SETTINGS:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_PLUGIN_LIST);
				int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

				if (Sel>=0) {
					LV_ITEM lvi;

					lvi.mask=LVIF_PARAM;
					lvi.iItem=Sel;
					lvi.iSubItem=0;
					if (ListView_GetItem(hwndList,&lvi))
						reinterpret_cast<CPlugin*>(lvi.lParam)->Settings(hDlg);
				}
			}
			return TRUE;

		case IDC_PLUGIN_UNLOAD:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_PLUGIN_LIST);
				int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

				if (Sel>=0) {
					LV_ITEM lvi;

					lvi.mask=LVIF_PARAM;
					lvi.iItem=Sel;
					lvi.iSubItem=0;
					if (ListView_GetItem(hwndList,&lvi)) {
						int Index=m_pPluginManager->FindPlugin(reinterpret_cast<CPlugin*>(lvi.lParam));

						if (m_pPluginManager->DeletePlugin(Index))
							ListView_DeleteItem(hwndList,Sel);
					}
				}
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case LVN_ITEMCHANGED:
			{
				LPNMLISTVIEW pnmlv=reinterpret_cast<LPNMLISTVIEW>(lParam);
				int Sel=ListView_GetNextItem(pnmlv->hdr.hwndFrom,-1,LVNI_SELECTED);

				if (Sel>=0) {
					LV_ITEM lvi;

					lvi.mask=LVIF_PARAM;
					lvi.iItem=Sel;
					lvi.iSubItem=0;
					if (ListView_GetItem(pnmlv->hdr.hwndFrom,&lvi)) {
						CPlugin *pPlugin=reinterpret_cast<CPlugin*>(lvi.lParam);
						EnableDlgItem(hDlg,IDC_PLUGIN_SETTINGS,pPlugin->HasSettings());
					}
				} else {
					EnableDlgItem(hDlg,IDC_PLUGIN_SETTINGS,false);
				}
			}
			return TRUE;

		case NM_DBLCLK:
			{
				LPNMITEMACTIVATE pnmia=reinterpret_cast<LPNMITEMACTIVATE>(lParam);

				if (pnmia->iItem>=0) {
					LV_ITEM lvi;

					lvi.mask=LVIF_PARAM;
					lvi.iItem=pnmia->iItem;
					lvi.iSubItem=0;
					if (ListView_GetItem(pnmia->hdr.hwndFrom,&lvi))
						reinterpret_cast<CPlugin*>(lvi.lParam)->Settings(hDlg);
				}
			}
			return TRUE;

		case NM_RCLICK:
			{
				LPNMHDR pnmh=reinterpret_cast<LPNMHDR>(lParam);
				int Sel=ListView_GetNextItem(pnmh->hwndFrom,-1,LVNI_SELECTED);

				if (Sel>=0) {
					LV_ITEM lvi;

					lvi.mask=LVIF_PARAM;
					lvi.iItem=Sel;
					lvi.iSubItem=0;
					if (ListView_GetItem(pnmh->hwndFrom,&lvi)) {
						HMENU hmenu=
							::LoadMenu(GetAppClass().GetResourceInstance(),
										MAKEINTRESOURCE(IDM_PLUGIN));
						CPlugin *pPlugin=reinterpret_cast<CPlugin*>(lvi.lParam);
						POINT pt;

						::EnableMenuItem(hmenu,IDC_PLUGIN_SETTINGS,
								pPlugin->HasSettings()?MFS_ENABLED:MFS_GRAYED);
						::EnableMenuItem(hmenu,IDC_PLUGIN_UNLOAD,
								pPlugin->CanUnload()?MFS_ENABLED:MFS_GRAYED);
						::GetCursorPos(&pt);
						::TrackPopupMenu(::GetSubMenu(hmenu,0),TPM_RIGHTBUTTON,
										 pt.x,pt.y,0,hDlg,NULL);
						::DestroyMenu(hmenu);
					}
				}
			}
			return TRUE;
		}
		break;
	}

	return FALSE;
}
