/*
  TVTest
  Copyright(c) 2008-2022 DBCTRADO

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
#include "Plugin.h"
#include "Image.h"
#include "DialogUtil.h"
#include "DPIUtil.h"
#include "Command.h"
#include "DriverManager.h"
#include "LogoManager.h"
#include "Controller.h"
#include "TSProcessor.h"
#include "TVTestVersion.h"
#include "DarkMode.h"
#include "LibISDB/LibISDB/Base/ARIBString.hpp"
#include "resource.h"
#include <algorithm>
#include "Common/DebugDef.h"


namespace TVTest
{


struct PluginMessageParam
{
	CPlugin *pPlugin;
	UINT Message;
	LPARAM lParam1;
	LPARAM lParam2;
};


static void EventInfoToProgramGuideProgramInfo(
	const LibISDB::EventInfo &EventInfo,
	ProgramGuideProgramInfo *pProgramInfo)
{
	pProgramInfo->NetworkID = EventInfo.NetworkID;
	pProgramInfo->TransportStreamID = EventInfo.TransportStreamID;
	pProgramInfo->ServiceID = EventInfo.ServiceID;
	pProgramInfo->EventID = EventInfo.EventID;
	pProgramInfo->StartTime = EventInfo.StartTime.ToSYSTEMTIME();
	pProgramInfo->Duration = EventInfo.Duration;
}


static bool SystemTimeToLocalFileTime(const SYSTEMTIME *pSystemTime, FILETIME *pFileTime)
{
	SYSTEMTIME stLocal;

	return ::SystemTimeToTzSpecificLocalTime(nullptr, pSystemTime, &stLocal)
		&& ::SystemTimeToFileTime(&stLocal, pFileTime);
}


static bool LocalFileTimeToSystemTime(const FILETIME *pFileTime, SYSTEMTIME *pSystemTime)
{
	SYSTEMTIME stLocal;

	return ::FileTimeToSystemTime(pFileTime, &stLocal)
		&& ::TzSpecificLocalTimeToSystemTime(nullptr, &stLocal, pSystemTime);
}




class CControllerPlugin
	: public CController
{
	CPlugin *m_pPlugin;
	DWORD m_Flags;
	String m_Name;
	String m_Text;
	int m_NumButtons;
	std::unique_ptr<CController::ButtonInfo[]> m_ButtonList;
	std::unique_ptr<TCHAR[]> m_ButtonNameList;
	String m_IniFileName;
	String m_SectionName;
	UINT m_ControllerImageID;
	UINT m_SelButtonsImageID;
	ControllerInfo::TranslateMessageCallback m_pTranslateMessage;
	void *m_pClientData;

public:
	CControllerPlugin(CPlugin *pPlugin, const ControllerInfo *pInfo);
	LPCTSTR GetName() const override { return m_Name.c_str(); }
	LPCTSTR GetText() const override { return m_Text.c_str(); }
	int NumButtons() const override { return m_NumButtons; }
	bool GetButtonInfo(int Index, ButtonInfo *pInfo) const override;
	bool Enable(bool fEnable) override { return m_pPlugin->Enable(fEnable); }
	bool IsEnabled() const override { return m_pPlugin->IsEnabled(); }
	bool IsActiveOnly() const override { return (m_Flags & CONTROLLER_FLAG_ACTIVEONLY) != 0; }
	bool SetTargetWindow(HWND hwnd) override {
		return m_pPlugin->SendEvent(EVENT_CONTROLLERFOCUS, reinterpret_cast<LPARAM>(hwnd)) != 0;
	}
	bool TranslateMessage(HWND hwnd, MSG *pMessage) override;
	bool GetIniFileName(LPTSTR pszFileName, int MaxLength) const override;
	LPCTSTR GetIniFileSection() const override;
	HBITMAP GetImage(ImageType Type) const override;
};


CControllerPlugin::CControllerPlugin(CPlugin *pPlugin, const ControllerInfo *pInfo)
	: m_pPlugin(pPlugin)
	, m_Flags(pInfo->Flags)
	, m_Name(pInfo->pszName)
	, m_Text(pInfo->pszText)
	, m_NumButtons(pInfo->NumButtons)
	, m_IniFileName(StringFromCStr(pInfo->pszIniFileName))
	, m_SectionName(StringFromCStr(pInfo->pszSectionName))
	, m_ControllerImageID(pInfo->ControllerImageID)
	, m_SelButtonsImageID(pInfo->SelButtonsImageID)
	, m_pTranslateMessage(pInfo->pTranslateMessage)
	, m_pClientData(pInfo->pClientData)
{
	const CCommandManager &CommandManager = GetAppClass().CommandManager;

	int Length = m_NumButtons + 1;
	for (int i = 0; i < m_NumButtons; i++)
		Length += ::lstrlen(pInfo->pButtonList[i].pszName);
	m_ButtonNameList = std::make_unique<TCHAR[]>(Length);
	LPTSTR pszName = m_ButtonNameList.get();

	m_ButtonList = std::make_unique<CController::ButtonInfo[]>(m_NumButtons);
	for (int i = 0; i < m_NumButtons; i++) {
		const ControllerButtonInfo &ButtonInfo = pInfo->pButtonList[i];

		StringCopy(pszName, ButtonInfo.pszName);
		m_ButtonList[i].pszName = pszName;
		m_ButtonList[i].DefaultCommand =
			ButtonInfo.pszDefaultCommand != nullptr ?
				CommandManager.ParseIDText(ButtonInfo.pszDefaultCommand) : 0;
		m_ButtonList[i].ImageButtonRect.Left = ButtonInfo.ButtonRect.Left;
		m_ButtonList[i].ImageButtonRect.Top = ButtonInfo.ButtonRect.Top;
		m_ButtonList[i].ImageButtonRect.Width = ButtonInfo.ButtonRect.Width;
		m_ButtonList[i].ImageButtonRect.Height = ButtonInfo.ButtonRect.Height;
		m_ButtonList[i].ImageSelButtonPos.Left = ButtonInfo.SelButtonPos.Left;
		m_ButtonList[i].ImageSelButtonPos.Top = ButtonInfo.SelButtonPos.Top;
		pszName += ::lstrlen(pszName) + 1;
	}
}


bool CControllerPlugin::GetButtonInfo(int Index, ButtonInfo *pInfo) const
{
	if (Index < 0 || Index >= m_NumButtons)
		return false;
	*pInfo = m_ButtonList[Index];
	return true;
}


bool CControllerPlugin::TranslateMessage(HWND hwnd, MSG *pMessage)
{
	if (m_pTranslateMessage == nullptr)
		return false;
	return m_pTranslateMessage(hwnd, pMessage, m_pClientData) != FALSE;
}


bool CControllerPlugin::GetIniFileName(LPTSTR pszFileName, int MaxLength) const
{
	if (m_IniFileName.empty())
		return CController::GetIniFileName(pszFileName, MaxLength);
	if (m_IniFileName.length() >= static_cast<String::size_type>(MaxLength))
		return false;
	StringCopy(pszFileName, m_IniFileName.c_str());
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
	case ImageType::Controller: ID = m_ControllerImageID; break;
	case ImageType::SelButtons: ID = m_SelButtonsImageID; break;
	default:	return nullptr;
	}
	if (ID == 0)
		return nullptr;
	return static_cast<HBITMAP>(
		::LoadImage(
			m_pPlugin->GetModuleHandle(),
			MAKEINTRESOURCE(ID),
			IMAGE_BITMAP, 0, 0,
			LR_CREATEDIBSECTION));
}



static void ConvertChannelInfo(const CChannelInfo *pChInfo, ChannelInfo *pChannelInfo)
{
	pChannelInfo->Space = pChInfo->GetSpace();
	pChannelInfo->Channel = pChInfo->GetChannelIndex();
	pChannelInfo->RemoteControlKeyID = pChInfo->GetChannelNo();
	pChannelInfo->NetworkID = pChInfo->GetNetworkID();
	pChannelInfo->TransportStreamID = pChInfo->GetTransportStreamID();
	pChannelInfo->szNetworkName[0] = '\0';
	pChannelInfo->szTransportStreamName[0] = '\0';
	StringCopy(pChannelInfo->szChannelName, pChInfo->GetName());
	if (pChannelInfo->Size >= CHANNELINFO_SIZE_V2) {
		pChannelInfo->PhysicalChannel = pChInfo->GetPhysicalChannel();
		pChannelInfo->Reserved = 0;
		pChannelInfo->ServiceType = pChInfo->GetServiceType();
		pChannelInfo->ServiceID = pChInfo->GetServiceID();
		if (pChannelInfo->Size == sizeof(ChannelInfo)) {
			pChannelInfo->Flags = CHANNEL_FLAG_NONE;
			if (!pChInfo->IsEnabled())
				pChannelInfo->Flags |= CHANNEL_FLAG_DISABLED;
		}
	}
}




class CEpgDataConverter
{
	static void GetEventInfoSize(
		const LibISDB::EventInfo &EventInfo, size_t *pInfoSize, size_t *pStringSize);
	static void ConvertEventInfo(
		const LibISDB::EventInfo &EventData,
		EpgEventInfo **ppEventInfo, LPWSTR *ppStringBuffer);
	static LPWSTR CopyString(LPWSTR pDstString, LPCWSTR pSrcString)
	{
		LPCWSTR p = pSrcString;
		LPWSTR q = pDstString;

		while ((*q = *p) != L'\0') {
			p++;
			q++;
		}
		return q + 1;
	}

public:
	EpgEventInfo *Convert(const LibISDB::EventInfo &EventData) const;
	EpgEventInfo **Convert(const LibISDB::EPGDatabase::EventList &EventList) const;
	static void FreeEventInfo(EpgEventInfo *pEventInfo);
	static void FreeEventList(EpgEventInfo **ppEventList);
};


void CEpgDataConverter::GetEventInfoSize(const LibISDB::EventInfo &EventInfo,
		size_t *pInfoSize, size_t *pStringSize)
{
	size_t InfoSize = sizeof(EpgEventInfo);
	size_t StringSize = 0;

	if (!EventInfo.EventName.empty())
		StringSize += EventInfo.EventName.length() + 1;
	if (!EventInfo.EventText.empty())
		StringSize += EventInfo.EventText.length() + 1;
	if (!EventInfo.ExtendedText.empty())
		StringSize += EventInfo.GetConcatenatedExtendedTextLength() + 1;
	InfoSize += (sizeof(EpgEventVideoInfo) + sizeof(EpgEventVideoInfo*)) * EventInfo.VideoList.size();
	for (const auto &e : EventInfo.VideoList) {
		if (!e.Text.empty())
			StringSize += e.Text.length() + 1;
	}
	InfoSize += (sizeof(EpgEventAudioInfo) + sizeof(EpgEventAudioInfo*)) * EventInfo.AudioList.size();
	for (const auto &e : EventInfo.AudioList) {
		if (!e.Text.empty())
			StringSize += e.Text.length() + 1;
	}
	InfoSize += sizeof(EpgEventContentInfo) * EventInfo.ContentNibble.NibbleCount;
	InfoSize += (sizeof(EpgEventGroupInfo) + sizeof(EpgEventGroupInfo*)) * EventInfo.EventGroupList.size();
	for (const auto &e : EventInfo.EventGroupList)
		InfoSize += sizeof(EpgGroupEventInfo) * e.EventList.size();

	*pInfoSize = InfoSize;
	*pStringSize = StringSize * sizeof(WCHAR);
}


void CEpgDataConverter::ConvertEventInfo(
	const LibISDB::EventInfo &EventData,
	EpgEventInfo **ppEventInfo, LPWSTR *ppStringBuffer)
{
	LPWSTR pString = *ppStringBuffer;

	EpgEventInfo *pEventInfo = *ppEventInfo;
	pEventInfo->EventID = EventData.EventID;
	pEventInfo->RunningStatus = EventData.RunningStatus;
	pEventInfo->FreeCaMode = EventData.FreeCAMode;
	pEventInfo->Reserved = 0;
	pEventInfo->StartTime = EventData.StartTime.ToSYSTEMTIME();
	pEventInfo->Duration = EventData.Duration;
	pEventInfo->VideoListLength = static_cast<BYTE>(EventData.VideoList.size());
	pEventInfo->AudioListLength = static_cast<BYTE>(EventData.AudioList.size());
	pEventInfo->ContentListLength = static_cast<BYTE>(EventData.ContentNibble.NibbleCount);
	pEventInfo->EventGroupListLength = static_cast<BYTE>(EventData.EventGroupList.size());
	if (!EventData.EventName.empty()) {
		pEventInfo->pszEventName = pString;
		pString = CopyString(pString, EventData.EventName.c_str());
	} else {
		pEventInfo->pszEventName = nullptr;
	}
	if (!EventData.EventText.empty()) {
		pEventInfo->pszEventText = pString;
		pString = CopyString(pString, EventData.EventText.c_str());
	} else {
		pEventInfo->pszEventText = nullptr;
	}
	if (!EventData.ExtendedText.empty()) {
		String ExtendedText;
		EventData.GetConcatenatedExtendedText(&ExtendedText);
		pEventInfo->pszEventExtendedText = pString;
		pString = CopyString(pString, ExtendedText.c_str());
	} else {
		pEventInfo->pszEventExtendedText = nullptr;
	}

	BYTE *p = reinterpret_cast<BYTE*>(pEventInfo + 1);

	if (!EventData.VideoList.empty()) {
		pEventInfo->VideoList = reinterpret_cast<EpgEventVideoInfo**>(p);
		p += sizeof(EpgEventVideoInfo*) * EventData.VideoList.size();
		for (size_t i = 0; i < EventData.VideoList.size(); i++) {
			const LibISDB::EventInfo::VideoInfo &Video = EventData.VideoList[i];
			pEventInfo->VideoList[i] = reinterpret_cast<EpgEventVideoInfo*>(p);
			pEventInfo->VideoList[i]->StreamContent = Video.StreamContent;
			pEventInfo->VideoList[i]->ComponentType = Video.ComponentType;
			pEventInfo->VideoList[i]->ComponentTag = Video.ComponentTag;
			pEventInfo->VideoList[i]->Reserved = 0;
			pEventInfo->VideoList[i]->LanguageCode = Video.LanguageCode;
			p += sizeof(EpgEventVideoInfo);
			if (!Video.Text.empty()) {
				pEventInfo->VideoList[i]->pszText = pString;
				pString = CopyString(pString, Video.Text.c_str());
			} else {
				pEventInfo->VideoList[i]->pszText = nullptr;
			}
		}
	} else {
		pEventInfo->VideoList = nullptr;
	}

	if (EventData.AudioList.size() > 0) {
		pEventInfo->AudioList = reinterpret_cast<EpgEventAudioInfo**>(p);
		p += sizeof(EpgEventAudioInfo*) * EventData.AudioList.size();
		for (size_t i = 0; i < EventData.AudioList.size(); i++) {
			const LibISDB::EventInfo::AudioInfo &Audio = EventData.AudioList[i];
			EpgEventAudioInfo *pAudioInfo = reinterpret_cast<EpgEventAudioInfo*>(p);
			pEventInfo->AudioList[i] = pAudioInfo;
			pAudioInfo->Flags = EPG_EVENT_AUDIO_FLAG_NONE;
			if (Audio.ESMultiLingualFlag)
				pAudioInfo->Flags |= EPG_EVENT_AUDIO_FLAG_MULTILINGUAL;
			if (Audio.MainComponentFlag)
				pAudioInfo->Flags |= EPG_EVENT_AUDIO_FLAG_MAINCOMPONENT;
			pAudioInfo->StreamContent = Audio.StreamContent;
			pAudioInfo->ComponentType = Audio.ComponentType;
			pAudioInfo->ComponentTag = Audio.ComponentTag;
			pAudioInfo->SimulcastGroupTag = Audio.SimulcastGroupTag;
			pAudioInfo->QualityIndicator = Audio.QualityIndicator;
			pAudioInfo->SamplingRate = Audio.SamplingRate;
			pAudioInfo->Reserved = 0;
			pAudioInfo->LanguageCode = Audio.LanguageCode;
			pAudioInfo->LanguageCode2 = Audio.LanguageCode2;
			p += sizeof(EpgEventAudioInfo);
			if (!Audio.Text.empty()) {
				pAudioInfo->pszText = pString;
				pString = CopyString(pString, Audio.Text.c_str());
			} else {
				pAudioInfo->pszText = nullptr;
			}
		}
	} else {
		pEventInfo->AudioList = nullptr;
	}

	if (EventData.ContentNibble.NibbleCount > 0) {
		pEventInfo->ContentList = reinterpret_cast<EpgEventContentInfo*>(p);
		p += sizeof(EpgEventContentInfo) * EventData.ContentNibble.NibbleCount;
		for (int i = 0; i < EventData.ContentNibble.NibbleCount; i++) {
			pEventInfo->ContentList[i].ContentNibbleLevel1 =
				EventData.ContentNibble.NibbleList[i].ContentNibbleLevel1;
			pEventInfo->ContentList[i].ContentNibbleLevel2 =
				EventData.ContentNibble.NibbleList[i].ContentNibbleLevel2;
			pEventInfo->ContentList[i].UserNibble1 =
				EventData.ContentNibble.NibbleList[i].UserNibble1;
			pEventInfo->ContentList[i].UserNibble2 =
				EventData.ContentNibble.NibbleList[i].UserNibble2;
		}
	} else {
		pEventInfo->ContentList = nullptr;
	}

	if (EventData.EventGroupList.size() > 0) {
		pEventInfo->EventGroupList = reinterpret_cast<EpgEventGroupInfo**>(p);
		p += sizeof(EpgEventGroupInfo*) * EventData.EventGroupList.size();
		for (size_t i = 0; i < EventData.EventGroupList.size(); i++) {
			const LibISDB::EventInfo::EventGroupInfo &Group = EventData.EventGroupList[i];
			EpgEventGroupInfo *pGroupInfo = reinterpret_cast<EpgEventGroupInfo*>(p);
			p += sizeof(EpgEventGroupInfo);
			pEventInfo->EventGroupList[i] = pGroupInfo;
			pGroupInfo->GroupType = Group.GroupType;
			pGroupInfo->EventListLength = static_cast<BYTE>(Group.EventList.size());
			::ZeroMemory(pGroupInfo->Reserved, sizeof(pGroupInfo->Reserved));
			pGroupInfo->EventList = reinterpret_cast<EpgGroupEventInfo*>(p);
			for (size_t j = 0; j < Group.EventList.size(); j++) {
				pGroupInfo->EventList[j].NetworkID = Group.EventList[j].NetworkID;
				pGroupInfo->EventList[j].TransportStreamID = Group.EventList[j].TransportStreamID;
				pGroupInfo->EventList[j].ServiceID = Group.EventList[j].ServiceID;
				pGroupInfo->EventList[j].EventID = Group.EventList[j].EventID;
			}
			p += sizeof(EpgGroupEventInfo) * Group.EventList.size();
		}
	} else {
		pEventInfo->EventGroupList = nullptr;
	}

	*ppEventInfo = reinterpret_cast<EpgEventInfo*>(p);
	*ppStringBuffer = pString;
}


EpgEventInfo *CEpgDataConverter::Convert(const LibISDB::EventInfo &EventData) const
{
	size_t InfoSize, StringSize;

	GetEventInfoSize(EventData, &InfoSize, &StringSize);
	BYTE *pBuffer = static_cast<BYTE*>(std::malloc(InfoSize + StringSize));
	if (pBuffer == nullptr)
		return nullptr;
	EpgEventInfo *pEventInfo = reinterpret_cast<EpgEventInfo*>(pBuffer);
	LPWSTR pString = reinterpret_cast<LPWSTR>(pBuffer + InfoSize);
	ConvertEventInfo(EventData, &pEventInfo, &pString);
#ifdef _DEBUG
	if (reinterpret_cast<BYTE*>(pEventInfo) - pBuffer != InfoSize
			|| reinterpret_cast<BYTE*>(pString) - (pBuffer + InfoSize) != StringSize)
		::DebugBreak();
#endif
	return reinterpret_cast<EpgEventInfo*>(pBuffer);
}


EpgEventInfo **CEpgDataConverter::Convert(const LibISDB::EPGDatabase::EventList &EventList) const
{
	const size_t ListSize = EventList.size() * sizeof(EpgEventInfo*);
	size_t InfoSize = 0, StringSize = 0;

	for (auto &Event : EventList) {
		size_t Info, String;

		GetEventInfoSize(Event, &Info, &String);
		InfoSize += Info;
		StringSize += String;
	}
	BYTE *pBuffer = static_cast<BYTE*>(std::malloc(ListSize + InfoSize + StringSize));
	if (pBuffer == nullptr)
		return nullptr;
	EpgEventInfo **ppEventList = reinterpret_cast<EpgEventInfo**>(pBuffer);
	EpgEventInfo *pEventInfo = reinterpret_cast<EpgEventInfo*>(pBuffer + ListSize);
	LPWSTR pString = reinterpret_cast<LPWSTR>(pBuffer + ListSize + InfoSize);
	int i = 0;
	for (auto &Event : EventList) {
		ppEventList[i++] = pEventInfo;
		ConvertEventInfo(Event, &pEventInfo, &pString);
	}
#ifdef _DEBUG
	if (reinterpret_cast<BYTE*>(pEventInfo) - (pBuffer + ListSize) != InfoSize
			|| reinterpret_cast<BYTE*>(pString) - (pBuffer + ListSize + InfoSize) != StringSize)
		::DebugBreak();
#endif
	return ppEventList;
}


void CEpgDataConverter::FreeEventInfo(EpgEventInfo *pEventInfo)
{
	std::free(pEventInfo);
}


void CEpgDataConverter::FreeEventList(EpgEventInfo **ppEventList)
{
	std::free(ppEventList);
}




static void GetFavoriteItemSize(
	const CFavoriteItem *pItem,
	size_t *pStructSize, size_t *pStringSize)
{
	*pStructSize += sizeof(FavoriteItemInfo);
	*pStringSize += ::lstrlen(pItem->GetName()) + 1;

	if (pItem->GetType() == CFavoriteItem::ItemType::Folder) {
		const CFavoriteFolder *pFolder =
			static_cast<const CFavoriteFolder*>(pItem);

		for (size_t i = 0; i < pFolder->GetItemCount(); i++)
			GetFavoriteItemSize(pFolder->GetItem(i), pStructSize, pStringSize);
	} else if (pItem->GetType() == CFavoriteItem::ItemType::Channel) {
		const CFavoriteChannel *pChannel =
			static_cast<const CFavoriteChannel*>(pItem);

		*pStringSize += ::lstrlen(pChannel->GetBonDriverFileName()) + 1;
	}
}


static void GetFavoriteItemInfo(
	const CFavoriteItem *pItem,
	FavoriteItemInfo **ppItemInfo,
	LPWSTR *ppStringBuffer)
{
	FavoriteItemInfo *pItemInfo = *ppItemInfo;
	LPWSTR pStringBuffer = *ppStringBuffer;
	size_t Length;

	*pItemInfo = FavoriteItemInfo();
	pItemInfo->pszName = pStringBuffer;
	Length = ::lstrlen(pItem->GetName()) + 1;
	std::memcpy(pStringBuffer, pItem->GetName(), Length * sizeof(WCHAR));
	pStringBuffer += Length;

	++*ppItemInfo;

	if (pItem->GetType() == CFavoriteItem::ItemType::Folder) {
		const CFavoriteFolder *pFolder =
			static_cast<const CFavoriteFolder*>(pItem);

		pItemInfo->Type = FAVORITE_ITEM_TYPE_FOLDER;
		pItemInfo->Folder.ItemCount = static_cast<DWORD>(pFolder->GetItemCount());
		if (pFolder->GetItemCount() > 0) {
			pItemInfo->Folder.ItemList = *ppItemInfo;
			for (size_t i = 0; i < pFolder->GetItemCount(); i++)
				GetFavoriteItemInfo(pFolder->GetItem(i), ppItemInfo, &pStringBuffer);
		}
	} else if (pItem->GetType() == CFavoriteItem::ItemType::Channel) {
		const CFavoriteChannel *pChannel =
			static_cast<const CFavoriteChannel*>(pItem);
		const CChannelInfo &ChannelInfo = pChannel->GetChannelInfo();

		pItemInfo->Type = FAVORITE_ITEM_TYPE_CHANNEL;
		if (pChannel->GetForceBonDriverChange())
			pItemInfo->Channel.Flags |= FAVORITE_CHANNEL_FLAG_FORCETUNERCHANGE;
		pItemInfo->Channel.Space = ChannelInfo.GetSpace();
		pItemInfo->Channel.Channel = ChannelInfo.GetChannelIndex();
		pItemInfo->Channel.ChannelNo = ChannelInfo.GetChannelNo();
		pItemInfo->Channel.NetworkID = ChannelInfo.GetNetworkID();
		pItemInfo->Channel.TransportStreamID = ChannelInfo.GetTransportStreamID();
		pItemInfo->Channel.ServiceID = ChannelInfo.GetServiceID();
		pItemInfo->Channel.pszTuner = pStringBuffer;
		Length = ::lstrlen(pChannel->GetBonDriverFileName()) + 1;
		std::memcpy(pStringBuffer, pChannel->GetBonDriverFileName(), Length * sizeof(WCHAR));
		pStringBuffer += Length;
	}

	*ppStringBuffer = pStringBuffer;
}


static bool GetFavoriteList(const CFavoriteFolder &Folder, FavoriteList *pList)
{
	pList->ItemCount = 0;
	pList->ItemList = nullptr;

	if (Folder.GetItemCount() == 0)
		return true;

	size_t StructSize = 0, StringSize = 0;

	for (size_t i = 0; i < Folder.GetItemCount(); i++)
		GetFavoriteItemSize(Folder.GetItem(i), &StructSize, &StringSize);

	StringSize *= sizeof(WCHAR);
	BYTE *pBuffer = static_cast<BYTE*>(std::malloc(StructSize + StringSize));
	if (pBuffer == nullptr)
		return false;
	pList->ItemList = reinterpret_cast<FavoriteItemInfo*>(pBuffer);
	pList->ItemCount = static_cast<DWORD>(Folder.GetItemCount());

	FavoriteItemInfo *pItemInfo = pList->ItemList;
	LPWSTR pStringBuffer = reinterpret_cast<LPWSTR>(pBuffer + StructSize);

	for (size_t i = 0; i < Folder.GetItemCount(); i++)
		GetFavoriteItemInfo(Folder.GetItem(i), &pItemInfo, &pStringBuffer);

	TVTEST_ASSERT(
		(reinterpret_cast<BYTE*>(pItemInfo) - reinterpret_cast<BYTE*>(pList->ItemList)) == StructSize &&
		(reinterpret_cast<BYTE*>(pStringBuffer) - reinterpret_cast<BYTE*>(pItemInfo)) == StringSize);

	return true;
}


static void FreeFavoriteList(FavoriteList *pList)
{
	pList->ItemCount = 0;
	if (pList->ItemList != nullptr) {
		std::free(pList->ItemList);
		pList->ItemList = nullptr;
	}
}


static bool CopyESList(const LibISDB::AnalyzerFilter::ESInfoList &SrcList, ElementaryStreamInfoList *pList)
{
	pList->ESCount = static_cast<WORD>(SrcList.size());
	pList->ESList = static_cast<ElementaryStreamInfo *>(std::malloc(sizeof(ElementaryStreamInfo) * SrcList.size()));
	if (pList->ESList == nullptr)
		return false;

	for (size_t i = 0; i < SrcList.size(); i++) {
		const LibISDB::AnalyzerFilter::ESInfo &SrcInfo = SrcList[i];
		ElementaryStreamInfo &Info = pList->ESList[i];
		Info.PID = SrcInfo.PID;
		Info.HierarchicalReferencePID = SrcInfo.HierarchicalReferencePID;
		Info.StreamType = SrcInfo.StreamType;
		Info.ComponentTag = SrcInfo.ComponentTag;
		Info.QualityLevel = SrcInfo.QualityLevel;
		Info.Reserved = 0;
	}

	return true;
}


static void AnalyzerServiceInfoToServiceInfo2(
	const LibISDB::AnalyzerFilter::ServiceInfo &Info, ServiceInfo2 *pServiceInfo)
{
	pServiceInfo->Status = SERVICE_INFO2_STATUS_NONE;
	if (Info.FreeCAMode)
		pServiceInfo->Status |= SERVICE_INFO2_STATUS_FREE_CA_MODE;
	pServiceInfo->ServiceID = Info.ServiceID;
	pServiceInfo->ServiceType = Info.ServiceType;
	pServiceInfo->Reserved = 0;
	pServiceInfo->PMT_PID = Info.PMTPID;
	pServiceInfo->PCR_PID = Info.PCRPID;
	StringCopy(pServiceInfo->szServiceName, Info.ServiceName.c_str());
	StringCopy(pServiceInfo->szProviderName, Info.ProviderName.c_str());
}



struct VarStringContext
{
	CEventVariableStringMap::EventInfo Event;
};

class CPluginVarStringMap
	: public CEventVariableStringMap
{
public:
	CPluginVarStringMap(VarStringFormatInfo *pInfo, const VarStringContext *pContext)
		: CEventVariableStringMap(pContext->Event)
		, m_pInfo(pInfo)
		, m_pContext(pContext)
	{
		m_Flags = (pInfo->Flags & VAR_STRING_FORMAT_FLAG_FILENAME) == 0 ? Flag::NoNormalize : Flag::None;
	}

	bool GetLocalString(LPCWSTR pszKeyword, String *pString) override
	{
		if (m_pInfo->pMapFunc != nullptr) {
			LPWSTR pszString = nullptr;
			if (m_pInfo->pMapFunc(pszKeyword, &pszString, m_pInfo->pClientData)) {
				if (pszString != nullptr) {
					*pString = pszString;
					std::free(pszString);
				}
				return true;
			}
		}
		return CEventVariableStringMap::GetLocalString(pszKeyword, pString);
	}

private:
	VarStringFormatInfo *m_pInfo;
	const VarStringContext *m_pContext;
};




HWND CPlugin::m_hwndMessage = nullptr;
UINT CPlugin::m_MessageCode = 0;

std::vector<CPlugin::CAudioStreamCallbackInfo> CPlugin::m_AudioStreamCallbackList;
MutexLock CPlugin::m_AudioStreamLock;
CPlugin::CAudioSampleCallback CPlugin::m_AudioSampleCallback;
std::vector<CPlugin::CVideoStreamCallbackInfo> CPlugin::m_VideoStreamCallbackList;
CPlugin::CVideoStreamCallback CPlugin::m_VideoStreamCallback;
MutexLock CPlugin::m_VideoStreamLock;


CPlugin::~CPlugin()
{
	Free();
}


bool CPlugin::Load(LPCTSTR pszFileName)
{
	if (m_hLib != nullptr)
		Free();

	const HMODULE hLib = ::LoadLibrary(pszFileName);
	if (hLib == nullptr) {
		const int ErrorCode = ::GetLastError();
		TCHAR szText[256];

		StringFormat(
			szText,
			TEXT("DLLがロードできません。(エラーコード {:#x})"), ErrorCode);
		SetWin32Error(ErrorCode, szText);
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
		case ERROR_MOD_NOT_FOUND:
		case ERROR_SXS_CANT_GEN_ACTCTX:
			SetErrorAdvise(TEXT("必要なランタイムがインストールされていない可能性があります。"));
			break;
		}
		return false;
	}
	const GetVersionFunc pGetVersion =
		reinterpret_cast<GetVersionFunc>(::GetProcAddress(hLib, "TVTGetVersion"));
	if (pGetVersion == nullptr) {
		::FreeLibrary(hLib);
		SetErrorText(TEXT("TVTGetVersion()関数のアドレスを取得できません。"));
		return false;
	}
	m_Version = pGetVersion();
	if (GetMajorVersion(m_Version) != GetMajorVersion(TVTEST_PLUGIN_VERSION)
			|| GetMinorVersion(m_Version) != GetMinorVersion(TVTEST_PLUGIN_VERSION)) {
		::FreeLibrary(hLib);
		SetErrorText(TEXT("対応していないバージョンです。"));
		return false;
	}
	const GetPluginInfoFunc pGetPluginInfo =
		reinterpret_cast<GetPluginInfoFunc>(::GetProcAddress(hLib, "TVTGetPluginInfo"));
	if (pGetPluginInfo == nullptr) {
		::FreeLibrary(hLib);
		SetErrorText(TEXT("TVTGetPluginInfo()関数のアドレスを取得できません。"));
		return false;
	}
	PluginInfo PluginInfo = {};
	if (!pGetPluginInfo(&PluginInfo)
			|| IsStringEmpty(PluginInfo.pszPluginName)) {
		::FreeLibrary(hLib);
		SetErrorText(TEXT("プラグインの情報を取得できません。"));
		return false;
	}
	const InitializeFunc pInitialize =
		reinterpret_cast<InitializeFunc>(::GetProcAddress(hLib, "TVTInitialize"));
	if (pInitialize == nullptr) {
		::FreeLibrary(hLib);
		SetErrorText(TEXT("TVTInitialize()関数のアドレスを取得できません。"));
		return false;
	}
	m_FileName = pszFileName;
	m_PluginName = PluginInfo.pszPluginName;
	StringUtility::Assign(m_Copyright, PluginInfo.pszCopyright);
	StringUtility::Assign(m_Description, PluginInfo.pszDescription);
	m_Type = PluginInfo.Type;
	m_Flags = PluginInfo.Flags;
	m_fEnabled = (m_Flags & PLUGIN_FLAG_ENABLEDEFAULT) != 0;
	m_PluginParam.Callback = Callback;
	m_PluginParam.hwndApp = GetAppClass().UICore.GetMainWindow();
	m_PluginParam.pClientData = nullptr;
	m_PluginParam.pInternalData = this;
	if (!pInitialize(&m_PluginParam)) {
		Free();
		::FreeLibrary(hLib);
		SetErrorText(TEXT("プラグインの初期化でエラー発生しました。"));
		return false;
	}
	m_hLib = hLib;
	ResetError();
	return true;
}


void CPlugin::Free()
{
	CAppMain &App = GetAppClass();

	m_pEventCallback = nullptr;
	m_ProgramGuideEventFlags = 0;
	m_pMessageCallback = nullptr;

	m_GrabberLock.Lock();
	if (!m_StreamGrabberList.empty()) {
		LibISDB::GrabberFilter *pGrabberFilter =
			App.CoreEngine.GetFilter<LibISDB::GrabberFilter>();
		for (auto &e : m_StreamGrabberList) {
			if (pGrabberFilter != nullptr)
				pGrabberFilter->RemoveGrabber(e.get());
		}
		m_StreamGrabberList.clear();
	}
	m_GrabberLock.Unlock();

	m_AudioStreamLock.Lock();
	for (std::vector<CAudioStreamCallbackInfo>::iterator i = m_AudioStreamCallbackList.begin();
			i != m_AudioStreamCallbackList.end(); i++) {
		if (i->m_pPlugin == this) {
			m_AudioStreamCallbackList.erase(i);
			break;
		}
	}
	m_AudioStreamLock.Unlock();

	m_VideoStreamLock.Lock();
	for (auto it = m_VideoStreamCallbackList.begin(); it != m_VideoStreamCallbackList.end(); ++it) {
		if (it->m_pPlugin == this) {
			m_VideoStreamCallbackList.erase(it);
			break;
		}
	}
	m_VideoStreamLock.Unlock();

	m_CommandList.clear();
	m_ProgramGuideCommandList.clear();

	for (const auto &e : m_ControllerList)
		App.ControllerManager.DeleteController(e.c_str());
	m_ControllerList.clear();

	if (m_hLib != nullptr) {
		const LPCTSTR pszFileName = ::PathFindFileName(m_FileName.c_str());

		App.AddLog(TEXT("{} の終了処理を行っています..."), pszFileName);

		const FinalizeFunc pFinalize =
			reinterpret_cast<FinalizeFunc>(::GetProcAddress(m_hLib, "TVTFinalize"));
		if (pFinalize == nullptr) {
			App.AddLog(
				CLogItem::LogType::Error,
				TEXT("{} のTVTFinalize()関数のアドレスを取得できません。"),
				pszFileName);
		} else {
			pFinalize();
		}
		::FreeLibrary(m_hLib);
		m_hLib = nullptr;
		App.AddLog(TEXT("{} を解放しました。"), pszFileName);
	}

	for (auto &Item : m_StatusItemList) {
		if (Item->pItem != nullptr)
			Item->pItem->DetachItem();
	}
	m_StatusItemList.clear();

	for (auto &Item : m_PanelItemList) {
		if (Item->pItem != nullptr)
			Item->pItem->DetachItem();
	}
	m_PanelItemList.clear();

	m_FileName.clear();
	m_PluginName.clear();
	m_Copyright.clear();
	m_Description.clear();
	m_fEnabled = false;
	m_fSetting = false;
	m_PluginParam.pInternalData = nullptr;
}


bool CPlugin::Enable(bool fEnable)
{
	if ((m_Flags & PLUGIN_FLAG_NOENABLEDDISABLED) != 0)
		return false;

	if (m_fEnabled != fEnable) {
		if (m_fSetting)
			return false;
		m_fSetting = true;
		const bool fResult = SendEvent(EVENT_PLUGINENABLE, static_cast<LPARAM>(fEnable)) != 0;
		m_fSetting = false;
		if (!fResult)
			return false;
		m_fEnabled = fEnable;

		if (m_Command > 0) {
			GetAppClass().CommandManager.SetCommandState(
				m_Command,
				CCommandManager::CommandState::Checked,
				fEnable ? CCommandManager::CommandState::Checked : CCommandManager::CommandState::None);
		}

		if (fEnable) {
			for (const auto &e : m_ControllerList)
				GetAppClass().ControllerManager.LoadControllerSettings(e.c_str());
		}
	}
	return true;
}


bool CPlugin::SetCommand(int Command)
{
	if (Command < CM_PLUGIN_FIRST || Command > CM_PLUGIN_LAST)
		return false;
	m_Command = Command;
	return true;
}


int CPlugin::NumPluginCommands() const
{
	return static_cast<int>(m_CommandList.size());
}


int CPlugin::ParsePluginCommand(LPCWSTR pszCommand) const
{
	if (IsStringEmpty(pszCommand))
		return -1;

	for (size_t i = 0; i < m_CommandList.size(); i++) {
		if (::lstrcmpi(m_CommandList[i].GetText(), pszCommand) == 0)
			return static_cast<int>(i);
	}

	return -1;
}


CPlugin::CPluginCommandInfo *CPlugin::GetPluginCommandInfo(int Index)
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_CommandList.size())
		return nullptr;
	return &m_CommandList[Index];
}


const CPlugin::CPluginCommandInfo *CPlugin::GetPluginCommandInfo(int Index) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_CommandList.size())
		return nullptr;
	return &m_CommandList[Index];
}


CPlugin::CPluginCommandInfo *CPlugin::GetPluginCommandInfo(LPCWSTR pszCommand)
{
	const int Index = ParsePluginCommand(pszCommand);
	if (Index < 0)
		return nullptr;
	return &m_CommandList[Index];
}


const CPlugin::CPluginCommandInfo *CPlugin::GetPluginCommandInfo(LPCWSTR pszCommand) const
{
	const int Index = ParsePluginCommand(pszCommand);
	if (Index < 0)
		return nullptr;
	return &m_CommandList[Index];
}


bool CPlugin::GetPluginCommandInfo(int Index, CommandInfo *pInfo) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_CommandList.size() || pInfo == nullptr)
		return false;
	const CPluginCommandInfo &Command = m_CommandList[Index];
	pInfo->ID = Command.GetID();
	pInfo->pszText = Command.GetText();
	pInfo->pszName = Command.GetName();
	return true;
}


bool CPlugin::NotifyCommand(LPCWSTR pszCommand)
{
	const int i = ParsePluginCommand(pszCommand);
	if (i < 0)
		return false;
	return SendEvent(EVENT_COMMAND, m_CommandList[i].GetID(), 0) != FALSE;
}


bool CPlugin::DrawPluginCommandIcon(const DrawCommandIconInfo *pInfo)
{
	if (pInfo == nullptr)
		return false;
	return SendEvent(EVENT_DRAWCOMMANDICON, reinterpret_cast<LPARAM>(pInfo), 0) != FALSE;
}


int CPlugin::NumProgramGuideCommands() const
{
	return static_cast<int>(m_ProgramGuideCommandList.size());
}


bool CPlugin::GetProgramGuideCommandInfo(int Index, ProgramGuideCommandInfo *pInfo) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_ProgramGuideCommandList.size())
		return false;
	const CProgramGuideCommand &Command = m_ProgramGuideCommandList[Index];
	pInfo->Type = Command.GetType();
	pInfo->Flags = PROGRAMGUIDE_COMMAND_FLAG_NONE;
	pInfo->ID = Command.GetID();
	pInfo->pszText = Command.GetText();
	pInfo->pszName = Command.GetName();
	return true;
}


bool CPlugin::NotifyProgramGuideCommand(
	LPCTSTR pszCommand, ProgramGuideCommandAction Action, const LibISDB::EventInfo *pEvent,
	const POINT *pCursorPos, const RECT *pItemRect)
{
	for (const auto &e : m_ProgramGuideCommandList) {
		if (::lstrcmpi(e.GetText(), pszCommand) == 0) {
			ProgramGuideCommandParam Param;

			Param.ID = e.GetID();
			Param.Action = Action;
			if (pEvent != nullptr)
				EventInfoToProgramGuideProgramInfo(*pEvent, &Param.Program);
			else
				::ZeroMemory(&Param.Program, sizeof(Param.Program));
			if (pCursorPos != nullptr)
				Param.CursorPos = *pCursorPos;
			else
				::GetCursorPos(&Param.CursorPos);
			if (pItemRect != nullptr)
				Param.ItemRect = *pItemRect;
			else
				::SetRectEmpty(&Param.ItemRect);
			return SendEvent(
				EVENT_PROGRAMGUIDE_COMMAND,
				Param.ID, reinterpret_cast<LPARAM>(&Param)) != 0;
		}
	}
	return false;
}


bool CPlugin::IsDisableOnStart() const
{
	return (m_Flags & PLUGIN_FLAG_DISABLEONSTART) != 0;
}


void CPlugin::RegisterStatusItems()
{
	CAppMain &App = GetAppClass();

	for (auto &Item : m_StatusItemList) {
		String IDText;

		IDText = ::PathFindFileName(GetFileName());
		IDText += _T(':');
		IDText += Item->IDText;

		const int ItemID = App.StatusOptions.RegisterItem(IDText.c_str());
		if (ItemID >= 0) {
			Item->ItemID = ItemID;
			App.StatusView.AddItem(new CPluginStatusItem(this, Item.get()));
		}
	}
}


void CPlugin::SendStatusItemCreatedEvent()
{
	for (const auto &Item : m_StatusItemList) {
		if (Item->pItem != nullptr) {
			StatusItemEventInfo Info;

			Info.ID = Item->ID;
			Info.Event = STATUS_ITEM_EVENT_CREATED;
			Info.Param = 0;

			SendEvent(EVENT_STATUSITEM_NOTIFY, reinterpret_cast<LPARAM>(&Info), 0);
		}
	}
}


void CPlugin::SendStatusItemUpdateTimerEvent()
{
	for (const auto &Item : m_StatusItemList) {
		if ((Item->Flags & STATUS_ITEM_FLAG_TIMERUPDATE) != 0
				&& Item->pItem != nullptr) {
			StatusItemEventInfo Info;

			Info.ID = Item->ID;
			Info.Event = STATUS_ITEM_EVENT_UPDATETIMER;
			Info.Param = 0;

			if (SendEvent(
						EVENT_STATUSITEM_NOTIFY,
						reinterpret_cast<LPARAM>(&Info), 0) != FALSE) {
				Item->pItem->Redraw();
			}
		}
	}
}


void CPlugin::RegisterPanelItems()
{
	CAppMain &App = GetAppClass();

	for (auto &Item : m_PanelItemList) {
		String IDText;

		IDText = ::PathFindFileName(GetFileName());
		IDText += _T(':');
		IDText += Item->IDText;

		const int ItemID = App.PanelOptions.RegisterPanelItem(IDText.c_str(), Item->Title.c_str());
		if (ItemID >= 0) {
			Item->ItemID = ItemID;
			if ((Item->StateMask & PANEL_ITEM_STATE_ENABLED) != 0) {
				App.PanelOptions.SetPanelItemVisibility(
					ItemID, (Item->State & PANEL_ITEM_STATE_ENABLED) != 0);
			} else {
				if (App.PanelOptions.GetPanelItemVisibility(ItemID))
					Item->State |= PANEL_ITEM_STATE_ENABLED;
				else
					Item->State &= ~PANEL_ITEM_STATE_ENABLED;
			}
			CPluginPanelItem *pPanelItem = new CPluginPanelItem(this, Item.get());
			pPanelItem->Create(App.Panel.Form.GetHandle(), WS_CHILD | WS_CLIPCHILDREN);
			CPanelForm::PageInfo PageInfo;
			PageInfo.pPage = pPanelItem;
			PageInfo.pszTitle = Item->Title.c_str();
			PageInfo.ID = ItemID;
			PageInfo.Icon = -1;
			PageInfo.fVisible = (Item->State & PANEL_ITEM_STATE_ENABLED) != 0;
			App.Panel.Form.AddPage(PageInfo);
		}
	}
}


LRESULT CPlugin::SendEvent(EventCode Event, LPARAM lParam1, LPARAM lParam2)
{
	if (m_pEventCallback != nullptr)
		return m_pEventCallback(Event, lParam1, lParam2, m_pEventCallbackClientData);
	return 0;
}


bool CPlugin::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
	if (m_pMessageCallback != nullptr)
		return m_pMessageCallback(hwnd, uMsg, wParam, lParam, pResult, m_pMessageCallbackClientData) != FALSE;
	return false;
}


bool CPlugin::Settings(HWND hwndOwner)
{
	if ((m_Flags & PLUGIN_FLAG_HASSETTINGS) == 0)
		return false;
	if (m_fSetting)
		return true;
	m_fSetting = true;
	const bool fResult = SendEvent(EVENT_PLUGINSETTINGS, reinterpret_cast<LPARAM>(hwndOwner)) != 0;
	m_fSetting = false;
	return fResult;
}


LRESULT CALLBACK CPlugin::Callback(PluginParam *pParam, MessageCode Message, LPARAM lParam1, LPARAM lParam2)
{
	if (pParam == nullptr)
		return 0;

	CPlugin *pPlugin = static_cast<CPlugin*>(pParam->pInternalData);
	if (pPlugin == nullptr)
		return 0;

	return pPlugin->OnCallback(pParam, Message, lParam1, lParam2);
}


LRESULT CPlugin::SendPluginMessage(
	PluginParam *pParam, MessageCode Message, LPARAM lParam1, LPARAM lParam2,
	LRESULT FailedResult)
{
	PluginMessageParam MessageParam;
	DWORD_PTR Result;

	MessageParam.pPlugin = this;
	if (MessageParam.pPlugin == nullptr)
		return FailedResult;
	MessageParam.Message = Message;
	MessageParam.lParam1 = lParam1;
	MessageParam.lParam2 = lParam2;
	if (::SendMessageTimeout(
				m_hwndMessage, m_MessageCode,
				Message, reinterpret_cast<LPARAM>(&MessageParam),
				SMTO_NORMAL, 10000, &Result))
		return Result;
	GetAppClass().AddLog(
		CLogItem::LogType::Error,
		TEXT("応答が無いためプラグインからのメッセージを処理できません。({} : {})"),
		::PathFindFileName(MessageParam.pPlugin->m_FileName.c_str()), static_cast<UINT>(Message));
	return FailedResult;
}


LRESULT CPlugin::OnCallback(PluginParam *pParam, MessageCode Message, LPARAM lParam1, LPARAM lParam2)
{
	switch (Message) {
	case MESSAGE_GETVERSION:
		return MakeVersion(VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD);

	case MESSAGE_QUERYMESSAGE:
		if (lParam1 < 0 || lParam1 >= MESSAGE_TRAILER)
			return FALSE;
		if (lParam1 == MESSAGE_REMOVED1
				|| lParam1 == MESSAGE_REMOVED2)
			return FALSE;
		return TRUE;

	case MESSAGE_MEMORYALLOC:
		{
			void *pData = reinterpret_cast<void*>(lParam1);
			const size_t Size = lParam2;

			if (Size > 0) {
				return reinterpret_cast<LRESULT>(std::realloc(pData, Size));
			} else if (pData != nullptr) {
				std::free(pData);
			}
		}
		return reinterpret_cast<LRESULT>(nullptr);

	case MESSAGE_SETEVENTCALLBACK:
		m_pEventCallback = reinterpret_cast<EventCallbackFunc>(lParam1);
		m_pEventCallbackClientData = reinterpret_cast<void*>(lParam2);
		return TRUE;

	case MESSAGE_GETCURRENTCHANNELINFO:
	case MESSAGE_SETCHANNEL:
		return SendPluginMessage(pParam, Message, lParam1, lParam2);

	case MESSAGE_GETSERVICE:
		{
			const CCoreEngine &CoreEngine = GetAppClass().CoreEngine;
			int *pNumServices = reinterpret_cast<int*>(lParam1);

			if (pNumServices)
				*pNumServices = CoreEngine.GetSelectableServiceCount();
			const uint16_t ServiceID = CoreEngine.GetServiceID();
			if (ServiceID == LibISDB::SERVICE_ID_INVALID)
				return -1;
			return CoreEngine.GetSelectableServiceIndexByID(ServiceID);;
		}

	case MESSAGE_SETSERVICE:
	case MESSAGE_GETTUNINGSPACENAME:
	case MESSAGE_GETCHANNELINFO:
		return SendPluginMessage(pParam, Message, lParam1, lParam2);

	case MESSAGE_GETSERVICEINFO:
		{
			const int Index = static_cast<int>(lParam1);
			ServiceInfo *pServiceInfo = reinterpret_cast<ServiceInfo*>(lParam2);

			if (Index < 0 || pServiceInfo == nullptr
					|| (pServiceInfo->Size != sizeof(ServiceInfo)
						&& pServiceInfo->Size != SERVICEINFO_SIZE_V1))
				return FALSE;

			const CCoreEngine &CoreEngine = GetAppClass().CoreEngine;
			const LibISDB::AnalyzerFilter *pAnalyzer = CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
			if (pAnalyzer == nullptr)
				return FALSE;

			const WORD ServiceID = CoreEngine.GetSelectableServiceID(Index);
			if (ServiceID == LibISDB::SERVICE_ID_INVALID)
				return FALSE;
			LibISDB::AnalyzerFilter::ServiceInfo Info;
			if (!pAnalyzer->GetServiceInfo(pAnalyzer->GetServiceIndexByID(ServiceID), &Info))
				return FALSE;
			pServiceInfo->ServiceID = ServiceID;
			pServiceInfo->VideoPID =
				Info.VideoESList.empty() ? LibISDB::PID_INVALID : Info.VideoESList[0].PID;
			const int NumAudioPIDs = std::min(static_cast<int>(Info.AudioESList.size()), 4);
			pServiceInfo->NumAudioPIDs = NumAudioPIDs;
			for (int i = 0; i < NumAudioPIDs; i++)
				pServiceInfo->AudioPID[i] = Info.AudioESList[i].PID;
			StringCopy(pServiceInfo->szServiceName, Info.ServiceName.c_str());
			if (pServiceInfo->Size == sizeof(ServiceInfo)) {
				const int ServiceIndex = pAnalyzer->GetServiceIndexByID(ServiceID);
				for (int i = 0; i < NumAudioPIDs; i++) {
					pServiceInfo->AudioComponentType[i] =
						pAnalyzer->GetAudioComponentType(ServiceIndex, i);
				}
				if (Info.CaptionESList.size() > 0)
					pServiceInfo->SubtitlePID = Info.CaptionESList[0].PID;
				else
					pServiceInfo->SubtitlePID = 0;
				pServiceInfo->Reserved = 0;
			}
		}
		return TRUE;

	case MESSAGE_GETDRIVERNAME:
	case MESSAGE_SETDRIVERNAME:

	case MESSAGE_STARTRECORD:
	case MESSAGE_STOPRECORD:
	case MESSAGE_PAUSERECORD:
	case MESSAGE_GETRECORD:
	case MESSAGE_MODIFYRECORD:

	case MESSAGE_GETZOOM:
	case MESSAGE_SETZOOM:
		return SendPluginMessage(pParam, Message, lParam1, lParam2);

	case MESSAGE_GETPANSCAN:
		{
			PanScanInfo *pInfo = reinterpret_cast<PanScanInfo*>(lParam1);

			if (pInfo == nullptr || pInfo->Size != sizeof(PanScanInfo))
				return FALSE;

			CCoreEngine::PanAndScanInfo PanScan;
			if (!GetAppClass().UICore.GetPanAndScan(&PanScan))
				return FALSE;

			pInfo->Type = PANSCAN_NONE;
			if (PanScan.Width < PanScan.XFactor) {
				if (PanScan.Height < PanScan.YFactor)
					pInfo->Type = PANSCAN_WINDOWBOX;
				else
					pInfo->Type = PANSCAN_PILLARBOX;
			} else if (PanScan.Height < PanScan.YFactor) {
				pInfo->Type = PANSCAN_LETTERBOX;
			}
		}
		return TRUE;

	case MESSAGE_SETPANSCAN:
		{
			const PanScanInfo *pInfo = reinterpret_cast<const PanScanInfo*>(lParam1);

			if (pInfo == nullptr || pInfo->Size != sizeof(PanScanInfo))
				return FALSE;
			int Command;
			switch (pInfo->Type) {
			case PANSCAN_NONE:
				if (pInfo->XAspect == 0 && pInfo->YAspect == 0)
					Command = CM_ASPECTRATIO_DEFAULT;
				else if (pInfo->XAspect == 16 && pInfo->YAspect == 9)
					Command = CM_ASPECTRATIO_16x9;
				else if (pInfo->XAspect == 4 && pInfo->YAspect == 3)
					Command = CM_ASPECTRATIO_4x3;
				else
					return FALSE;
				break;
			case PANSCAN_LETTERBOX:
				if (pInfo->XAspect == 16 && pInfo->YAspect == 9)
					Command = CM_ASPECTRATIO_LETTERBOX;
				else
					return FALSE;
				break;
			case PANSCAN_PILLARBOX:
				if (pInfo->XAspect == 4 && pInfo->YAspect == 3)
					Command = CM_ASPECTRATIO_PILLARBOX;
				else
					return FALSE;
				break;
			case PANSCAN_WINDOWBOX:
				if (pInfo->XAspect == 16 && pInfo->YAspect == 9)
					Command = CM_ASPECTRATIO_WINDOWBOX;
				else
					return FALSE;
				break;
			default:
				return FALSE;
			}
			GetAppClass().UICore.DoCommand(Command);
		}
		return TRUE;

	case MESSAGE_GETSTATUS:
		{
			StatusInfo *pInfo = reinterpret_cast<StatusInfo*>(lParam1);

			if (pInfo == nullptr
					|| (pInfo->Size != sizeof(StatusInfo)
						&& pInfo->Size != STATUSINFO_SIZE_V1))
				return FALSE;
			const CCoreEngine *pCoreEngine = &GetAppClass().CoreEngine;
			const ULONGLONG DropCount = pCoreEngine->GetContinuityErrorPacketCount();
			pInfo->SignalLevel = pCoreEngine->GetSignalLevel();
			pInfo->BitRate = pCoreEngine->GetBitRate();
			pInfo->ErrorPacketCount = static_cast<DWORD>(pCoreEngine->GetErrorPacketCount() + DropCount);
			pInfo->ScramblePacketCount = static_cast<DWORD>(pCoreEngine->GetScramblePacketCount());
			if (pInfo->Size == sizeof(StatusInfo)) {
				pInfo->DropPacketCount = static_cast<DWORD>(DropCount);
				pInfo->Reserved = 0;
			}
		}
		return TRUE;

	case MESSAGE_GETRECORDSTATUS:
		return SendPluginMessage(pParam, Message, lParam1, lParam2);

	case MESSAGE_GETVIDEOINFO:
		{
			VideoInfo *pVideoInfo = reinterpret_cast<VideoInfo*>(lParam1);

			if (pVideoInfo == nullptr || pVideoInfo->Size != sizeof(VideoInfo))
				return FALSE;

			const LibISDB::ViewerFilter *pViewer =
				GetAppClass().CoreEngine.GetFilter<LibISDB::ViewerFilter>();
			if (pViewer == nullptr)
				return FALSE;

			int VideoWidth, VideoHeight;
			int XAspect, YAspect;

			if (pViewer->GetOriginalVideoSize(&VideoWidth, &VideoHeight)
					&& pViewer->GetVideoAspectRatio(&XAspect, &YAspect)
					&& pViewer->GetSourceRect(&pVideoInfo->SourceRect)) {
				pVideoInfo->Width = VideoWidth;
				pVideoInfo->Height = VideoHeight;
				pVideoInfo->XAspect = XAspect;
				pVideoInfo->YAspect = YAspect;
				return TRUE;
			}
		}
		return FALSE;

	case MESSAGE_GETVOLUME:
		{
			const CUICore &UICore = GetAppClass().UICore;
			const int Volume = UICore.GetVolume();
			const bool fMute = UICore.GetMute();

			return MAKELONG(Volume, fMute);
		}

	case MESSAGE_SETVOLUME:
		return SendPluginMessage(pParam, Message, lParam1, lParam2);

	case MESSAGE_GETSTEREOMODE:
	case MESSAGE_SETSTEREOMODE:
		return SendPluginMessage(pParam, Message, lParam1, lParam2);

	case MESSAGE_GETFULLSCREEN:
		return GetAppClass().UICore.GetFullscreen();

	case MESSAGE_SETFULLSCREEN:
		return SendPluginMessage(pParam, Message, lParam1, lParam2);

	case MESSAGE_GETPREVIEW:
		return GetAppClass().UICore.IsViewerEnabled();

	case MESSAGE_SETPREVIEW:
		return SendPluginMessage(pParam, Message, lParam1, lParam2);

	case MESSAGE_GETSTANDBY:
		return GetAppClass().UICore.GetStandby();

	case MESSAGE_SETSTANDBY:
		return SendPluginMessage(pParam, Message, lParam1, lParam2);

	case MESSAGE_GETALWAYSONTOP:
		return GetAppClass().UICore.GetAlwaysOnTop();

	case MESSAGE_SETALWAYSONTOP:
		return SendPluginMessage(pParam, Message, lParam1, lParam2);

	case MESSAGE_CAPTUREIMAGE:
		{
			LibISDB::COMMemoryPointer<> Image(GetAppClass().CoreEngine.GetCurrentImage());

			if (Image) {
				const size_t Size = CalcDIBSize(reinterpret_cast<BITMAPINFOHEADER*>(Image.get()));
				void *pDib;

				pDib = std::malloc(Size);
				if (pDib != nullptr)
					std::memcpy(pDib, Image.get(), Size);
				return reinterpret_cast<LRESULT>(pDib);
			}
		}
		return reinterpret_cast<LRESULT>(nullptr);

	case MESSAGE_SAVEIMAGE:
		GetAppClass().UICore.DoCommand(CM_SAVEIMAGE);
		return TRUE;

	case MESSAGE_RESET:
		{
			const DWORD Flags = static_cast<DWORD>(lParam1);

			if (Flags == RESET_ALL)
				GetAppClass().UICore.DoCommand(CM_RESET);
			else if (Flags == RESET_VIEWER)
				GetAppClass().UICore.DoCommand(CM_RESETVIEWER);
			else
				return FALSE;
		}
		return TRUE;

	case MESSAGE_CLOSE:
		::PostMessage(
			GetAppClass().UICore.GetMainWindow(),
			WM_COMMAND,
			(lParam1 & CLOSE_EXIT) != 0 ? CM_EXIT : CM_CLOSE, 0);
		return TRUE;

	case MESSAGE_SETSTREAMCALLBACK:
		{
			StreamCallbackInfo *pInfo = reinterpret_cast<StreamCallbackInfo*>(lParam1);

			if (pInfo == nullptr
					|| pInfo->Size != sizeof(StreamCallbackInfo)
					|| pInfo->Callback == nullptr)
				return FALSE;

			LibISDB::GrabberFilter *pGrabberFilter =
				GetAppClass().CoreEngine.GetFilter<LibISDB::GrabberFilter>();
			if (pGrabberFilter == nullptr)
				return false;

			BlockLock Lock(m_GrabberLock);

			if ((pInfo->Flags & STREAM_CALLBACK_REMOVE) == 0) {
				// コールバック登録
				if (!m_StreamGrabberList.empty()) {
					for (auto &Grabber : m_StreamGrabberList) {
						if (Grabber->GetCallbackFunc() == pInfo->Callback) {
							Grabber->SetClientData(pInfo->pClientData);
							return TRUE;
						}
					}
				}
				CStreamGrabber *pGrabber = new CStreamGrabber(pInfo->Callback, pInfo->pClientData);
				m_StreamGrabberList.emplace_back(pGrabber);
				pGrabberFilter->AddGrabber(pGrabber);
			} else {
				// コールバック削除
				for (auto it = m_StreamGrabberList.begin(); it != m_StreamGrabberList.end(); ++it) {
					CStreamGrabber *pGrabber = it->get();
					if (pGrabber->GetCallbackFunc() == pInfo->Callback) {
						pGrabberFilter->RemoveGrabber(pGrabber);
						m_StreamGrabberList.erase(it);
						return TRUE;
					}
				}
				return FALSE;
			}
		}
		return TRUE;

	case MESSAGE_ENABLEPLUGIN:
		return Enable(lParam1 != 0);

	case MESSAGE_GETCOLOR:
		{
			const LPCWSTR pszName = reinterpret_cast<LPCWSTR>(lParam1);

			if (pszName == nullptr)
				return CLR_INVALID;
			return GetAppClass().UICore.GetColor(pszName);
		}

	case MESSAGE_DECODEARIBSTRING:
		{
			ARIBStringDecodeInfo *pInfo = reinterpret_cast<ARIBStringDecodeInfo*>(lParam1);

			if (pInfo == nullptr || pInfo->Size != sizeof(ARIBStringDecodeInfo)
					|| pInfo->pSrcData == nullptr
					|| pInfo->pszDest == nullptr || pInfo->DestLength == 0)
				return FALSE;

			LibISDB::ARIBStringDecoder Decoder;
			LibISDB::String Str;
			if (Decoder.Decode(
						static_cast<const uint8_t *>(pInfo->pSrcData), pInfo->SrcLength, &Str)) {
				StringCopy(pInfo->pszDest, Str.c_str(), pInfo->DestLength);
			} else {
				pInfo->pszDest[0] = '\0';
			}
		}
		return TRUE;

	case MESSAGE_GETCURRENTPROGRAMINFO:
		{
			ProgramInfo *pProgramInfo = reinterpret_cast<ProgramInfo*>(lParam1);

			if (pProgramInfo == nullptr
					|| pProgramInfo->Size != sizeof(ProgramInfo))
				return FALSE;

			LibISDB::EventInfo EventInfo;
			if (!GetAppClass().CoreEngine.GetCurrentEventInfo(&EventInfo, lParam2 != 0))
				return FALSE;

			pProgramInfo->ServiceID = EventInfo.ServiceID;
			pProgramInfo->EventID = EventInfo.EventID;
			if (pProgramInfo->pszEventName != nullptr && pProgramInfo->MaxEventName > 0) {
				StringCopy(pProgramInfo->pszEventName, EventInfo.EventName.c_str(), pProgramInfo->MaxEventName);
			}
			if (pProgramInfo->pszEventText != nullptr && pProgramInfo->MaxEventText > 0) {
				StringCopy(pProgramInfo->pszEventText, EventInfo.EventText.c_str(), pProgramInfo->MaxEventText);
			}
			if (pProgramInfo->pszEventExtText != nullptr && pProgramInfo->MaxEventExtText > 0) {
				LibISDB::String ExtendedText;
				EventInfo.GetConcatenatedExtendedText(&ExtendedText);
				StringCopy(pProgramInfo->pszEventExtText, ExtendedText.c_str(), pProgramInfo->MaxEventExtText);
			}
			pProgramInfo->StartTime = EventInfo.StartTime.ToSYSTEMTIME();
			pProgramInfo->Duration = EventInfo.Duration;
		}
		return TRUE;

	case MESSAGE_QUERYEVENT:
		return lParam1 >= 0 && lParam1 < EVENT_TRAILER;

	case MESSAGE_GETTUNINGSPACE:
		{
			int *pNumSpaces = reinterpret_cast<int*>(lParam1);
			if (pNumSpaces != nullptr)
				*pNumSpaces = 0;
		}
		return SendPluginMessage(pParam, Message, lParam1, lParam2, -1);

	case MESSAGE_GETTUNINGSPACEINFO:
		return SendPluginMessage(pParam, Message, lParam1, lParam2, -1);

	case MESSAGE_SETNEXTCHANNEL:
		GetAppClass().UICore.DoCommand((lParam1 & 1) != 0 ? CM_CHANNEL_UP : CM_CHANNEL_DOWN);
		return TRUE;

	case MESSAGE_GETAUDIOSTREAM:
		return GetAppClass().UICore.GetAudioStream();

	case MESSAGE_SETAUDIOSTREAM:
		return SendPluginMessage(pParam, Message, lParam1, lParam2);

	case MESSAGE_ISPLUGINENABLED:
		return IsEnabled();

	case MESSAGE_REGISTERCOMMAND:
		{
			const CommandInfo *pCommandList = reinterpret_cast<CommandInfo*>(lParam1);
			const int NumCommands = static_cast<int>(lParam2);

			if (pCommandList == nullptr || NumCommands <= 0)
				return FALSE;
			for (int i = 0; i < NumCommands; i++) {
				m_CommandList.emplace_back(pCommandList[i]);
			}
		}
		return TRUE;

	case MESSAGE_ADDLOG:
		{
			const LPCWSTR pszText = reinterpret_cast<LPCWSTR>(lParam1);
			if (pszText == nullptr)
				return FALSE;

			const LPCTSTR pszFileName = ::PathFindFileName(m_FileName.c_str());
			GetAppClass().AddLog(static_cast<CLogItem::LogType>(lParam2), TEXT("{} : {}"), pszFileName, pszText);
		}
		return TRUE;

	case MESSAGE_RESETSTATUS:
		GetAppClass().UICore.DoCommand(CM_RESETERRORCOUNT);
		return TRUE;

	case MESSAGE_SETAUDIOCALLBACK:
		{
			const AudioCallbackFunc pCallback = reinterpret_cast<AudioCallbackFunc>(lParam1);
			LibISDB::ViewerFilter *pViewer =
				GetAppClass().CoreEngine.GetFilter<LibISDB::ViewerFilter>();
			if (pViewer == nullptr)
				return FALSE;

			m_AudioStreamLock.Lock();
			if (pCallback != nullptr) {
				if (m_AudioStreamCallbackList.empty()) {
					pViewer->SetAudioSampleCallback(&m_AudioSampleCallback);
				} else {
					for (std::vector<CAudioStreamCallbackInfo>::iterator i = m_AudioStreamCallbackList.begin();
							i != m_AudioStreamCallbackList.end(); i++) {
						if (i->m_pPlugin == this) {
							m_AudioStreamCallbackList.erase(i);
							break;
						}
					}
				}
				m_AudioStreamCallbackList.emplace_back(this, pCallback, reinterpret_cast<void*>(lParam2));
				m_AudioStreamLock.Unlock();
			} else {
				bool fFound = false;
				for (std::vector<CAudioStreamCallbackInfo>::iterator i = m_AudioStreamCallbackList.begin();
						i != m_AudioStreamCallbackList.end(); i++) {
					if (i->m_pPlugin == this) {
						m_AudioStreamCallbackList.erase(i);
						fFound = true;
						break;
					}
				}
				m_AudioStreamLock.Unlock();
				if (!fFound)
					return FALSE;
				if (m_AudioStreamCallbackList.empty())
					pViewer->SetAudioSampleCallback(nullptr);
			}
		}
		return TRUE;

	case MESSAGE_DOCOMMAND:
		{
			const LPCWSTR pszCommand = reinterpret_cast<LPCWSTR>(lParam1);

			if (pszCommand == nullptr || pszCommand[0] == L'\0')
				return FALSE;
			return GetAppClass().UICore.DoCommand(pszCommand);
		}

	case MESSAGE_GETHOSTINFO:
		{
			HostInfo *pHostInfo = reinterpret_cast<HostInfo*>(lParam1);

			if (pHostInfo == nullptr || pHostInfo->Size != sizeof(HostInfo))
				return FALSE;
			pHostInfo->pszAppName = APP_NAME_W;
			pHostInfo->Version.Major = VERSION_MAJOR;
			pHostInfo->Version.Minor = VERSION_MINOR;
			pHostInfo->Version.Build = VERSION_BUILD;
			pHostInfo->pszVersionText = VERSION_TEXT_W;
			pHostInfo->SupportedPluginVersion = TVTEST_PLUGIN_VERSION;
		}
		return TRUE;

	case MESSAGE_GETSETTING:
		return OnGetSetting(reinterpret_cast<SettingInfo*>(lParam1));

	case MESSAGE_GETDRIVERFULLPATHNAME:
		return SendPluginMessage(pParam, Message, lParam1, lParam2);

	case MESSAGE_GETLOGO:
		{
			const WORD NetworkID = LOWORD(lParam1), ServiceID = HIWORD(lParam1);
			const BYTE LogoType = static_cast<BYTE>(lParam2 & 0xFF);
			const HBITMAP hbm = GetAppClass().LogoManager.GetAssociatedLogoBitmap(NetworkID, ServiceID, LogoType);
			if (hbm != nullptr) {
				return reinterpret_cast<LRESULT>(::CopyImage(hbm, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
			}
		}
		return reinterpret_cast<LRESULT>(nullptr);

	case MESSAGE_GETAVAILABLELOGOTYPE:
		{
			const WORD NetworkID = LOWORD(lParam1), ServiceID = HIWORD(lParam1);

			return GetAppClass().LogoManager.GetAvailableLogoType(NetworkID, ServiceID);
		}

	case MESSAGE_RELAYRECORD:
		return SendPluginMessage(pParam, Message, lParam1, lParam2);

	case MESSAGE_SILENTMODE:
		if (lParam1 == SILENTMODE_GET) {
			return GetAppClass().Core.IsSilent();
		} else if (lParam1 == SILENTMODE_SET) {
			GetAppClass().Core.SetSilent(lParam2 != 0);
			return TRUE;
		}
		return FALSE;

	case MESSAGE_SETWINDOWMESSAGECALLBACK:
		return SendPluginMessage(pParam, Message, lParam1, lParam2);

	case MESSAGE_REGISTERCONTROLLER:
		{
			const ControllerInfo *pInfo = reinterpret_cast<const ControllerInfo*>(lParam1);

			if (pInfo == nullptr
					|| pInfo->Size != sizeof(ControllerInfo)
					|| pInfo->pszName == nullptr || pInfo->pszName[0] == '\0'
					|| pInfo->pszText == nullptr || pInfo->pszText[0] == '\0'
					|| pInfo->NumButtons < 1
					|| pInfo->pButtonList == nullptr)
				return FALSE;
			CControllerPlugin *pController = new CControllerPlugin(this, pInfo);
			CControllerManager *pControllerManager = &GetAppClass().ControllerManager;
			if (!pControllerManager->AddController(pController)) {
				delete pController;
				return FALSE;
			}
			m_ControllerList.emplace_back(pInfo->pszName);
			if (m_fEnabled)
				pControllerManager->LoadControllerSettings(pInfo->pszName);
		}
		return TRUE;

	case MESSAGE_ONCONTROLLERBUTTONDOWN:
		return GetAppClass().ControllerManager.OnButtonDown(
			reinterpret_cast<LPCWSTR>(lParam1), static_cast<int>(lParam2));

	case MESSAGE_GETCONTROLLERSETTINGS:
		{
			static constexpr DWORD ValidMask = CONTROLLER_SETTINGS_MASK_FLAGS;
			const LPCWSTR pszName = reinterpret_cast<LPCWSTR>(lParam1);
			ControllerSettings *pSettings = reinterpret_cast<ControllerSettings*>(lParam2);

			if (pSettings == nullptr
					|| (pSettings->Mask | ValidMask) != ValidMask)
				return FALSE;
			const CControllerManager::ControllerSettings *pControllerSettings =
				GetAppClass().ControllerManager.GetControllerSettings(pszName);
			if (pControllerSettings == nullptr)
				return FALSE;
			if ((pSettings->Mask & CONTROLLER_SETTINGS_MASK_FLAGS) != 0) {
				pSettings->Flags = CONTROLLER_SETTINGS_FLAG_NONE;
				if (pControllerSettings->fActiveOnly)
					pSettings->Flags |= CONTROLLER_SETTINGS_FLAG_ACTIVEONLY;
			}
		}
		return TRUE;

	case MESSAGE_GETEPGEVENTINFO:
		{
			const EpgEventQueryInfo *pQueryInfo =
				reinterpret_cast<const EpgEventQueryInfo*>(lParam1);
			if (pQueryInfo == nullptr)
				return reinterpret_cast<LRESULT>(nullptr);

			const LibISDB::EPGDatabase &EPGDatabase = GetAppClass().EPGDatabase;
			LibISDB::EventInfo EventData;
			if (pQueryInfo->Type == EPG_EVENT_QUERY_EVENTID) {
				if (!EPGDatabase.GetEventInfo(
							pQueryInfo->NetworkID,
							pQueryInfo->TransportStreamID,
							pQueryInfo->ServiceID,
							pQueryInfo->EventID,
							&EventData))
					return reinterpret_cast<LRESULT>(nullptr);
			} else if (pQueryInfo->Type == EPG_EVENT_QUERY_TIME) {
				SYSTEMTIME stUTC;
				LibISDB::DateTime UTCTime, EPGTime;

				if (!::FileTimeToSystemTime(&pQueryInfo->Time, &stUTC))
					return reinterpret_cast<LRESULT>(nullptr);
				UTCTime.FromSYSTEMTIME(stUTC);
				if (!LibISDB::UTCTimeToEPGTime(UTCTime, &EPGTime))
					return reinterpret_cast<LRESULT>(nullptr);
				if (!EPGDatabase.GetEventInfo(
							pQueryInfo->NetworkID,
							pQueryInfo->TransportStreamID,
							pQueryInfo->ServiceID,
							EPGTime,
							&EventData))
					return reinterpret_cast<LRESULT>(nullptr);
			} else {
				return reinterpret_cast<LRESULT>(nullptr);
			}

			CEpgDataConverter Converter;
			return reinterpret_cast<LRESULT>(Converter.Convert(EventData));
		}

	case MESSAGE_FREEEPGEVENTINFO:
		{
			EpgEventInfo *pEventInfo = reinterpret_cast<EpgEventInfo*>(lParam1);

			if (pEventInfo != nullptr)
				CEpgDataConverter::FreeEventInfo(pEventInfo);
		}
		return TRUE;

	case MESSAGE_GETEPGEVENTLIST:
		{
			EpgEventList *pEventList = reinterpret_cast<EpgEventList*>(lParam1);
			if (pEventList == nullptr)
				return FALSE;

			pEventList->NumEvents = 0;
			pEventList->EventList = nullptr;

			LibISDB::EPGDatabase::EventList EventList;

			if (!GetAppClass().EPGDatabase.GetEventListSortedByTime(
						pEventList->NetworkID,
						pEventList->TransportStreamID,
						pEventList->ServiceID,
						&EventList)
					|| EventList.empty())
				return FALSE;

			CEpgDataConverter Converter;
			pEventList->EventList = Converter.Convert(EventList);
			if (pEventList->EventList == nullptr)
				return FALSE;
			pEventList->NumEvents = static_cast<WORD>(EventList.size());
		}
		return TRUE;

	case MESSAGE_FREEEPGEVENTLIST:
		{
			EpgEventList *pEventList = reinterpret_cast<EpgEventList*>(lParam1);

			if (pEventList != nullptr) {
				CEpgDataConverter::FreeEventList(pEventList->EventList);
				pEventList->NumEvents = 0;
				pEventList->EventList = nullptr;
			}
		}
		return TRUE;

	case MESSAGE_ENUMDRIVER:
		{
			const LPWSTR pszFileName = reinterpret_cast<LPWSTR>(lParam1);
			const int Index = LOWORD(lParam2);
			const int MaxLength = HIWORD(lParam2);

			const CDriverManager *pDriverManager = &GetAppClass().DriverManager;
			const CDriverInfo *pDriverInfo = pDriverManager->GetDriverInfo(Index);
			if (pDriverInfo != nullptr) {
				if (pszFileName != nullptr)
					StringCopy(pszFileName, pDriverInfo->GetFileName(), MaxLength);
				return ::lstrlen(pDriverInfo->GetFileName());
			}
		}
		return 0;

	case MESSAGE_GETDRIVERTUNINGSPACELIST:
		{
			const LPCWSTR pszDriverName = reinterpret_cast<LPCWSTR>(lParam1);
			DriverTuningSpaceList *pList =
				reinterpret_cast<DriverTuningSpaceList*>(lParam2);

			if (pszDriverName == nullptr || pList == nullptr)
				return FALSE;

			pList->NumSpaces = 0;
			pList->SpaceList = nullptr;

			CDriverInfo DriverInfo(pszDriverName);
			if (!DriverInfo.LoadTuningSpaceList())
				return FALSE;

			const CTuningSpaceList *pTuningSpaceList = DriverInfo.GetTuningSpaceList();
			if (pTuningSpaceList->NumSpaces() == 0)
				return FALSE;

			const int NumSpaces = pTuningSpaceList->NumSpaces();
			size_t BufferSize =
				NumSpaces *
					(sizeof(DriverTuningSpaceInfo) +
					 sizeof(DriverTuningSpaceInfo*) +
					 sizeof(TuningSpaceInfo));
			for (int i = 0; i < NumSpaces; i++) {
				const CChannelList *pChannelList = pTuningSpaceList->GetChannelList(i);
				BufferSize +=
					pChannelList->NumChannels() *
						(sizeof(ChannelInfo) + sizeof(ChannelInfo*));
			}
			BYTE *pBuffer = static_cast<BYTE*>(std::malloc(BufferSize));
			if (pBuffer == nullptr)
				return FALSE;
			BYTE *p = pBuffer;
			pList->NumSpaces = NumSpaces;
			pList->SpaceList = reinterpret_cast<DriverTuningSpaceInfo**>(p);
			p += NumSpaces * sizeof(DriverTuningSpaceInfo*);
			for (int i = 0; i < NumSpaces; i++) {
				const CTuningSpaceInfo *pSpaceInfo = pTuningSpaceList->GetTuningSpaceInfo(i);
				const CChannelList *pChannelList = pSpaceInfo->GetChannelList();
				const int NumChannels = pChannelList->NumChannels();
				DriverTuningSpaceInfo *pDriverSpaceInfo = reinterpret_cast<DriverTuningSpaceInfo*>(p);

				p += sizeof(DriverTuningSpaceInfo);
				pList->SpaceList[i] = pDriverSpaceInfo;
				pDriverSpaceInfo->Flags = DRIVER_TUNING_SPACE_INFO_FLAG_NONE;
				pDriverSpaceInfo->NumChannels = NumChannels;
				pDriverSpaceInfo->pInfo = reinterpret_cast<TuningSpaceInfo*>(p);
				pDriverSpaceInfo->pInfo->Size = sizeof(TuningSpaceInfo);
				pDriverSpaceInfo->pInfo->Space = static_cast<TuningSpaceType>(pSpaceInfo->GetType());
				if (pSpaceInfo->GetName() != nullptr) {
					StringCopy(pDriverSpaceInfo->pInfo->szName, pSpaceInfo->GetName());
				} else {
					pDriverSpaceInfo->pInfo->szName[0] = '\0';
				}
				p += sizeof(TuningSpaceInfo);
				pDriverSpaceInfo->ChannelList = reinterpret_cast<ChannelInfo**>(p);
				p += NumChannels * sizeof(ChannelInfo*);
				for (int j = 0; j < NumChannels; j++) {
					ChannelInfo *pChannelInfo = reinterpret_cast<ChannelInfo*>(p);
					p += sizeof(ChannelInfo);
					pDriverSpaceInfo->ChannelList[j] = pChannelInfo;
					pChannelInfo->Size = sizeof(ChannelInfo);
					ConvertChannelInfo(pChannelList->GetChannelInfo(j), pChannelInfo);
				}
			}
#ifdef _DEBUG
			if (p - pBuffer != BufferSize)
				::DebugBreak();
#endif
		}
		return TRUE;

	case MESSAGE_FREEDRIVERTUNINGSPACELIST:
		{
			DriverTuningSpaceList *pList =
				reinterpret_cast<DriverTuningSpaceList*>(lParam1);

			if (pList != nullptr) {
				std::free(pList->SpaceList);
				pList->NumSpaces = 0;
				pList->SpaceList = nullptr;
			}
		}
		return TRUE;

	case MESSAGE_ENABLEPROGRAMGUIDEEVENT:
		m_ProgramGuideEventFlags = static_cast<UINT>(lParam1);
		return TRUE;

	case MESSAGE_REGISTERPROGRAMGUIDECOMMAND:
		{
			const ProgramGuideCommandInfo *pCommandList =
				reinterpret_cast<ProgramGuideCommandInfo*>(lParam1);
			const int NumCommands = static_cast<int>(lParam2);

			if (pCommandList == nullptr || NumCommands < 1)
				return FALSE;
			for (int i = 0; i < NumCommands; i++) {
				m_ProgramGuideCommandList.emplace_back(pCommandList[i]);
			}
		}
		return TRUE;

	case MESSAGE_GETSTYLEVALUE:
		{
			StyleValueInfo *pInfo = reinterpret_cast<StyleValueInfo*>(lParam1);

			if (pInfo == nullptr
					|| pInfo->Size != sizeof(StyleValueInfo)
					|| pInfo->Flags != 0
					|| IsStringEmpty(pInfo->pszName))
				return FALSE;

			const Style::CStyleManager &StyleManager = GetAppClass().StyleManager;
			Style::StyleInfo Style;

			if (!StyleManager.Get(pInfo->pszName, &Style))
				return FALSE;
			if (Style.Type == Style::ValueType::Int) {
				if (pInfo->Unit == STYLE_UNIT_UNDEFINED) {
					pInfo->Value = Style.Value.Int;
					switch (Style.Unit) {
					case Style::UnitType::LogicalPixel:
						pInfo->Unit = STYLE_UNIT_LOGICAL_PIXEL;
						break;
					case Style::UnitType::PhysicalPixel:
						pInfo->Unit = STYLE_UNIT_PHYSICAL_PIXEL;
						break;
					case Style::UnitType::Point:
						pInfo->Unit = STYLE_UNIT_POINT;
						break;
					case Style::UnitType::DIP:
						pInfo->Unit = STYLE_UNIT_DIP;
						break;
					}
				} else {
					Style::UnitType Unit;
					switch (pInfo->Unit) {
					case STYLE_UNIT_LOGICAL_PIXEL:
						Unit = Style::UnitType::LogicalPixel;
						break;
					case STYLE_UNIT_PHYSICAL_PIXEL:
						Unit = Style::UnitType::PhysicalPixel;
						break;
					case STYLE_UNIT_POINT:
						Unit = Style::UnitType::Point;
						break;
					case STYLE_UNIT_DIP:
						Unit = Style::UnitType::DIP;
						break;
					default:
						return FALSE;
					}
					Style::CStyleScaling StyleScaling;
					const int DPI = pInfo->DPI != 0 ? pInfo->DPI : 96;
					StyleScaling.SetDPI(DPI);
					pInfo->Value = StyleScaling.ConvertUnit(Style.Value.Int, Style.Unit, Unit);
				}
			} else if (Style.Type == Style::ValueType::Bool) {
				pInfo->Value = Style.Value.Bool;
			} else {
				return FALSE;
			}
		}
		return TRUE;

	case MESSAGE_THEMEDRAWBACKGROUND:
		{
			ThemeDrawBackgroundInfo *pInfo =
				reinterpret_cast<ThemeDrawBackgroundInfo*>(lParam1);

			if (pInfo == nullptr
					|| pInfo->Size != sizeof(ThemeDrawBackgroundInfo)
					|| IsStringEmpty(pInfo->pszStyle)
					|| pInfo->hdc == nullptr)
				return FALSE;

			CAppMain &App = GetAppClass();
			const Theme::CThemeManager ThemeManager(App.UICore.GetCurrentColorScheme());
			const int Type = ThemeManager.ParseStyleName(pInfo->pszStyle);
			if (Type < 0)
				return FALSE;
			Theme::BackgroundStyle Style;
			ThemeManager.GetBackgroundStyle(Type, &Style);
			const Style::CStyleScaling *pStyleScaling;
			Style::CStyleScaling StyleScaling;
			if (pInfo->DPI == 0) {
				pStyleScaling = App.MainWindow.GetStyleScaling();
			} else {
				App.StyleManager.InitStyleScaling(&StyleScaling);
				StyleScaling.SetDPI(pInfo->DPI);
				pStyleScaling = &StyleScaling;
			}
			pStyleScaling->ToPixels(&Style.Border.Width.Left);
			pStyleScaling->ToPixels(&Style.Border.Width.Top);
			pStyleScaling->ToPixels(&Style.Border.Width.Right);
			pStyleScaling->ToPixels(&Style.Border.Width.Bottom);
			Theme::Draw(pInfo->hdc, pInfo->DrawRect, Style);
			if ((pInfo->Flags & THEME_DRAW_BACKGROUND_FLAG_ADJUSTRECT) != 0)
				Theme::SubtractBorderRect(Style.Border, &pInfo->DrawRect);
		}
		return TRUE;

	case MESSAGE_THEMEDRAWTEXT:
		{
			ThemeDrawTextInfo *pInfo =
				reinterpret_cast<ThemeDrawTextInfo*>(lParam1);

			if (pInfo == nullptr
					|| pInfo->Size != sizeof(ThemeDrawTextInfo)
					|| IsStringEmpty(pInfo->pszStyle)
					|| pInfo->hdc == nullptr
					|| pInfo->pszText == nullptr)
				return FALSE;

			const Theme::CThemeManager ThemeManager(GetAppClass().UICore.GetCurrentColorScheme());
			const int Type = ThemeManager.ParseStyleName(pInfo->pszStyle);
			if (Type < 0)
				return FALSE;
			Theme::ForegroundStyle Style;
			ThemeManager.GetForegroundStyle(Type, &Style);
			if (pInfo->Color != CLR_INVALID) {
				Style.Fill.Type = Theme::FillType::Solid;
				Style.Fill.Solid.Color = Theme::ThemeColor(pInfo->Color);
			}
			Theme::Draw(pInfo->hdc, pInfo->DrawRect, Style, pInfo->pszText, pInfo->DrawFlags);
		}
		return TRUE;

	case MESSAGE_THEMEDRAWICON:
		{
			ThemeDrawIconInfo *pInfo =
				reinterpret_cast<ThemeDrawIconInfo*>(lParam1);

			if (pInfo == nullptr
					|| pInfo->Size != sizeof(ThemeDrawIconInfo)
					|| IsStringEmpty(pInfo->pszStyle)
					|| pInfo->hdc == nullptr
					|| pInfo->hbm == nullptr)
				return FALSE;

			const Theme::CThemeManager ThemeManager(GetAppClass().UICore.GetCurrentColorScheme());
			const int Type = ThemeManager.ParseStyleName(pInfo->pszStyle);
			if (Type < 0)
				return FALSE;
			Theme::ForegroundStyle Style;
			ThemeManager.GetForegroundStyle(Type, &Style);
			DrawUtil::CMonoColorBitmap Bitmap;
			if (!Bitmap.Create(pInfo->hbm))
				return FALSE;
			Bitmap.Draw(
				pInfo->hdc,
				pInfo->DstRect.left, pInfo->DstRect.top,
				pInfo->DstRect.right - pInfo->DstRect.left,
				pInfo->DstRect.bottom - pInfo->DstRect.top,
				pInfo->SrcRect.left, pInfo->SrcRect.top,
				pInfo->SrcRect.right - pInfo->SrcRect.left,
				pInfo->SrcRect.bottom - pInfo->SrcRect.top,
				pInfo->Color != CLR_INVALID ? pInfo->Color : Style.Fill.GetSolidColor().GetCOLORREF(),
				pInfo->Opacity);
		}
		return TRUE;

	case MESSAGE_GETEPGCAPTURESTATUS:
		{
			EpgCaptureStatusInfo *pInfo =
				reinterpret_cast<EpgCaptureStatusInfo*>(lParam1);

			if (pInfo == nullptr
					|| pInfo->Size != sizeof(EpgCaptureStatusInfo)
					|| pInfo->Flags != EPG_CAPTURE_STATUS_FLAG_NONE)
				return FALSE;

			const LibISDB::EPGDatabase &EPGDatabase = GetAppClass().EPGDatabase;
			EpgCaptureStatus Status = EPG_CAPTURE_STATUS_NONE;

			if ((pInfo->Status & EPG_CAPTURE_STATUS_SCHEDULEBASICCOMPLETED) != 0) {
				if (EPGDatabase.IsScheduleComplete(pInfo->NetworkID, pInfo->TransportStreamID, pInfo->ServiceID, false))
					Status |= EPG_CAPTURE_STATUS_SCHEDULEBASICCOMPLETED;
			}
			if ((pInfo->Status & EPG_CAPTURE_STATUS_SCHEDULEEXTENDEDCOMPLETED) != 0) {
				if (EPGDatabase.IsScheduleComplete(pInfo->NetworkID, pInfo->TransportStreamID, pInfo->ServiceID, true))
					Status |= EPG_CAPTURE_STATUS_SCHEDULEEXTENDEDCOMPLETED;
			}
			if ((pInfo->Status & EPG_CAPTURE_STATUS_HASSCHEDULEBASIC) != 0) {
				if (EPGDatabase.HasSchedule(pInfo->NetworkID, pInfo->TransportStreamID, pInfo->ServiceID, false))
					Status |= EPG_CAPTURE_STATUS_HASSCHEDULEBASIC;
			}
			if ((pInfo->Status & EPG_CAPTURE_STATUS_HASSCHEDULEEXTENDED) != 0) {
				if (EPGDatabase.HasSchedule(pInfo->NetworkID, pInfo->TransportStreamID, pInfo->ServiceID, true))
					Status |= EPG_CAPTURE_STATUS_HASSCHEDULEEXTENDED;
			}

			pInfo->Status = Status;
		}
		return TRUE;

	case MESSAGE_GETAPPCOMMANDINFO:
		{
			AppCommandInfo *pInfo = reinterpret_cast<AppCommandInfo*>(lParam1);

			if (pInfo == nullptr || pInfo->Size != sizeof(AppCommandInfo))
				return FALSE;

			CreateAppCommandList();

			if (pInfo->Index >= m_AppCommandList.size())
				return FALSE;

			const AppCommand &Command = m_AppCommandList[pInfo->Index];

			if (pInfo->pszText != nullptr) {
				if (pInfo->MaxText < 1)
					return FALSE;
				StringCopy(pInfo->pszText, Command.IDText.c_str(), pInfo->MaxText);
			} else {
				pInfo->MaxText = static_cast<int>(Command.IDText.length()) + 1;
			}
			if (pInfo->pszName != nullptr) {
				if (pInfo->MaxName < 1)
					return FALSE;
				StringCopy(pInfo->pszName, Command.Text.c_str(), pInfo->MaxName);
			} else {
				pInfo->MaxName = static_cast<int>(Command.Text.length()) + 1;
			}
		}
		return TRUE;

	case MESSAGE_GETAPPCOMMANDCOUNT:
		CreateAppCommandList();
		return m_AppCommandList.size();

	case MESSAGE_GETVIDEOSTREAMCOUNT:
		return GetAppClass().CoreEngine.GetVideoStreamCount();

	case MESSAGE_GETVIDEOSTREAM:
		return GetAppClass().CoreEngine.GetVideoStream();

	case MESSAGE_SETVIDEOSTREAM:
		return SendPluginMessage(pParam, Message, lParam1, lParam2);

	case MESSAGE_GETLOG:
		{
			GetLogInfo *pInfo = reinterpret_cast<GetLogInfo*>(lParam1);

			if (pInfo == nullptr || pInfo->Size != sizeof(GetLogInfo))
				return FALSE;

			const CLogger &Logger = GetAppClass().Logger;
			CLogItem Log;

			if ((pInfo->Flags & GET_LOG_FLAG_BYSERIAL) == 0) {
				if (!Logger.GetLog(pInfo->Index, &Log))
					return FALSE;
				pInfo->Serial = Log.GetSerialNumber();
			} else {
				if (!Logger.GetLogBySerialNumber(pInfo->Serial, &Log))
					return FALSE;
			}
			LPCTSTR pszText = Log.GetText();
			if (pInfo->pszText != nullptr) {
				StringCopy(pInfo->pszText, pszText, pInfo->MaxText);
			} else {
				pInfo->MaxText = ::lstrlen(pszText) + 1;
			}
			pInfo->Type = static_cast<int>(Log.GetType());
		}
		return TRUE;

	case MESSAGE_GETLOGCOUNT:
		return GetAppClass().Logger.GetLogCount();

	case MESSAGE_REGISTERPLUGINCOMMAND:
		{
			const PluginCommandInfo *pInfo =
				reinterpret_cast<const PluginCommandInfo*>(lParam1);

			if (pInfo == nullptr || pInfo->Size != sizeof(PluginCommandInfo))
				return FALSE;

			m_CommandList.emplace_back(*pInfo);
		}
		return TRUE;

	case MESSAGE_SETPLUGINCOMMANDSTATE:
		{
			const int ID = static_cast<int>(lParam1);

			for (auto &e : m_CommandList) {
				if (e.GetID() == ID) {
					const PluginCommandState State = static_cast<PluginCommandState>(lParam2);

					e.SetState(State);

					CCommandManager::CommandState CommandState = CCommandManager::CommandState::None;
					if ((State & PLUGIN_COMMAND_STATE_DISABLED) != 0)
						CommandState |= CCommandManager::CommandState::Disabled;
					if ((State & PLUGIN_COMMAND_STATE_CHECKED) != 0)
						CommandState |= CCommandManager::CommandState::Checked;
					GetAppClass().CommandManager.SetCommandState(
						e.GetCommand(),
						CCommandManager::CommandState::Disabled |
						CCommandManager::CommandState::Checked,
						CommandState);

					return TRUE;
				}
			}
		}
		return FALSE;

	case MESSAGE_PLUGINCOMMANDNOTIFY:
		{
			const int ID = static_cast<int>(lParam1);

			for (const auto &e : m_CommandList) {
				if (e.GetID() == ID) {
					const unsigned int Type = static_cast<unsigned int>(lParam2);

					if ((Type & PLUGIN_COMMAND_NOTIFY_CHANGEICON))
						GetAppClass().SideBar.RedrawItem(e.GetCommand());

					return TRUE;
				}
			}
		}
		return FALSE;

	case MESSAGE_REGISTERPLUGINICON:
		{
			const PluginIconInfo *pInfo =
				reinterpret_cast<const PluginIconInfo*>(lParam1);

			if (pInfo == nullptr
					|| pInfo->Size != sizeof(PluginIconInfo)
					|| pInfo->Flags != 0
					|| pInfo->hbmIcon == nullptr)
				return FALSE;

			if (!m_PluginIcon.Create(pInfo->hbmIcon))
				return FALSE;
		}
		return TRUE;

	case MESSAGE_REGISTERSTATUSITEM:
		{
			const StatusItemInfo *pInfo =
				reinterpret_cast<const StatusItemInfo*>(lParam1);

			if (pInfo == nullptr
					|| pInfo->Size != sizeof(StatusItemInfo)
					|| IsStringEmpty(pInfo->pszIDText)
					|| IsStringEmpty(pInfo->pszName))
				return FALSE;

			StatusItem *pItem = new StatusItem;

			pItem->Flags = pInfo->Flags;
			pItem->ID = pInfo->ID;
			pItem->IDText = pInfo->pszIDText;
			pItem->Name = pInfo->pszName;
			pItem->MinWidth = pInfo->MinWidth;
			pItem->MaxWidth = pInfo->MaxWidth;
			pItem->DefaultWidth = pInfo->DefaultWidth;
			pItem->MinHeight = pInfo->MinHeight;
			pItem->ItemID = -1;
			pItem->Style = pInfo->Style;
			pItem->State = STATUS_ITEM_STATE_NONE;
			pItem->pItem = nullptr;

			m_StatusItemList.emplace_back(pItem);
		}
		return TRUE;

	case MESSAGE_SETSTATUSITEM:
		{
			const StatusItemSetInfo *pInfo =
				reinterpret_cast<const StatusItemSetInfo*>(lParam1);

			if (pInfo == nullptr || pInfo->Size != sizeof(StatusItemSetInfo))
				return FALSE;

			for (auto &Item : m_StatusItemList) {
				if (Item->ID == pInfo->ID) {
					if ((pInfo->Mask & STATUS_ITEM_SET_INFO_MASK_STATE) != 0) {
						const StatusItemState OldState = Item->State;
						const StatusItemState NewState = (Item->State & ~pInfo->StateMask) | (pInfo->State & pInfo->StateMask);
						Item->State = NewState;
						if (((NewState ^ OldState) & STATUS_ITEM_STATE_VISIBLE) != 0) {
							const bool fVisible = (NewState & STATUS_ITEM_STATE_VISIBLE) != 0;
							GetAppClass().StatusOptions.SetItemVisibility(Item->ItemID, fVisible);
							if (Item->pItem != nullptr) {
								GetAppClass().MainWindow.ShowStatusBarItem(Item->ItemID, fVisible);
							}
						}
					}

					if ((pInfo->Mask & STATUS_ITEM_SET_INFO_MASK_STYLE) != 0) {
						const StatusItemStyle OldStyle = Item->Style;
						const StatusItemStyle NewStyle = (OldStyle & ~pInfo->StyleMask) | (pInfo->Style & pInfo->StyleMask);
						if (NewStyle != OldStyle) {
							Item->Style = NewStyle;
							if (Item->pItem != nullptr) {
								Item->pItem->ApplyItemStyle();
								GetAppClass().StatusView.AdjustSize();
							}
						}
					}

					return TRUE;
				}
			}
		}
		return FALSE;

	case MESSAGE_GETSTATUSITEMINFO:
		{
			StatusItemGetInfo *pInfo =
				reinterpret_cast<StatusItemGetInfo*>(lParam1);

			if (pInfo == nullptr || pInfo->Size != sizeof(StatusItemGetInfo))
				return FALSE;

			for (const auto &Item : m_StatusItemList) {
				if (Item->ID == pInfo->ID) {
					if ((pInfo->Mask & STATUS_ITEM_GET_INFO_MASK_STATE) != 0)
						pInfo->State = Item->State;

					if ((pInfo->Mask & STATUS_ITEM_GET_INFO_MASK_HWND) != 0) {
						if (Item->pItem != nullptr)
							pInfo->hwnd = Item->pItem->GetWindowHandle();
						else
							pInfo->hwnd = nullptr;
					}

					if ((pInfo->Mask & STATUS_ITEM_GET_INFO_MASK_ITEMRECT) != 0) {
						if (Item->pItem == nullptr || !Item->pItem->GetRect(&pInfo->ItemRect))
							::SetRectEmpty(&pInfo->ItemRect);
					}

					if ((pInfo->Mask & STATUS_ITEM_GET_INFO_MASK_CONTENTRECT) != 0) {
						if (Item->pItem == nullptr || !Item->pItem->GetClientRect(&pInfo->ContentRect))
							::SetRectEmpty(&pInfo->ContentRect);
					}

					if ((pInfo->Mask & STATUS_ITEM_GET_INFO_MASK_STYLE) != 0)
						pInfo->Style = Item->Style;

					return TRUE;
				}
			}
		}
		return FALSE;

	case MESSAGE_STATUSITEMNOTIFY:
		for (const auto &Item : m_StatusItemList) {
			if (Item->ID == lParam1) {
				switch (lParam2) {
				case STATUS_ITEM_NOTIFY_REDRAW:
					if (Item->pItem != nullptr)
						Item->pItem->Redraw();
					return TRUE;
				}
			}
		}
		return FALSE;

	case MESSAGE_REGISTERTSPROCESSOR:
		{
			const TSProcessorInfo *pInfo =
				reinterpret_cast<const TSProcessorInfo*>(lParam1);

			if (pInfo == nullptr
					|| pInfo->Size != sizeof(TSProcessorInfo)
					|| pInfo->Flags != 0
					|| pInfo->pTSProcessor == nullptr)
				return FALSE;

			CTSProcessor *pTSProcessor =
				new CTSProcessor(pInfo->pTSProcessor);
			if (!GetAppClass().TSProcessorManager.RegisterTSProcessor(
						pTSProcessor,
						static_cast<CCoreEngine::TSProcessorConnectPosition>(pInfo->ConnectPosition))) {
				pTSProcessor->Release();
				return FALSE;
			}

			m_Flags |= PLUGIN_FLAG_NOUNLOAD;
		}
		return TRUE;

	case MESSAGE_REGISTERPANELITEM:
		{
			const PanelItemInfo *pInfo =
				reinterpret_cast<const PanelItemInfo*>(lParam1);

			if (pInfo == nullptr
					|| pInfo->Size != sizeof(PanelItemInfo)
					|| pInfo->Flags != PANEL_ITEM_FLAG_NONE
					|| IsStringEmpty(pInfo->pszIDText)
					|| IsStringEmpty(pInfo->pszTitle))
				return FALSE;

			PanelItem *pItem = new PanelItem;

			pItem->ID = pInfo->ID;
			pItem->IDText = pInfo->pszIDText;
			pItem->Title = pInfo->pszTitle;
			pItem->StateMask = PANEL_ITEM_STATE_NONE;
			pItem->State = PANEL_ITEM_STATE_NONE;
			pItem->Style = pInfo->Style;
			pItem->ItemID = -1;
			pItem->pItem = nullptr;
			if (pInfo->hbmIcon != nullptr)
				pItem->Icon.Create(pInfo->hbmIcon);

			m_PanelItemList.emplace_back(pItem);
		}
		return TRUE;

	case MESSAGE_SETPANELITEM:
		{
			const PanelItemSetInfo *pInfo =
				reinterpret_cast<const PanelItemSetInfo*>(lParam1);

			if (pInfo == nullptr || pInfo->Size != sizeof(PanelItemSetInfo))
				return FALSE;

			for (auto &Item : m_PanelItemList) {
				if (Item->ID == pInfo->ID) {
					if ((pInfo->Mask & PANEL_ITEM_SET_INFO_MASK_STATE) != 0) {
						if (Item->pItem == nullptr || Item->pItem->GetItemHandle() == nullptr) {
							Item->StateMask |= pInfo->StateMask;
							Item->State = (Item->State & ~pInfo->StateMask) | (pInfo->State & pInfo->StateMask);
						} else {
							if ((pInfo->StateMask & PANEL_ITEM_STATE_ENABLED) != 0) {
								GetAppClass().PanelOptions.SetPanelItemVisibility(
									Item->ItemID,
									(pInfo->State & PANEL_ITEM_STATE_ENABLED) != 0);
							}

							if ((pInfo->StateMask & PANEL_ITEM_STATE_ACTIVE) != 0) {
								if ((pInfo->State & PANEL_ITEM_STATE_ACTIVE) != 0) {
									GetAppClass().Panel.Form.SetCurPageByID(Item->ItemID);
								}
							}
						}
					}

					if ((pInfo->Mask & PANEL_ITEM_SET_INFO_MASK_STYLE) != 0) {
						Item->Style = (Item->Style & ~pInfo->StyleMask) | (pInfo->Style & pInfo->StyleMask);
					}

					return TRUE;
				}
			}
		}
		return FALSE;

	case MESSAGE_GETPANELITEMINFO:
		{
			PanelItemGetInfo *pInfo =
				reinterpret_cast<PanelItemGetInfo*>(lParam1);

			if (pInfo == nullptr || pInfo->Size != sizeof(PanelItemGetInfo))
				return FALSE;

			auto it = std::ranges::find_if(
				m_PanelItemList,
				[&](const std::unique_ptr<PanelItem> &Item) -> bool { return Item->ID == pInfo->ID; });
			if (it == m_PanelItemList.end())
				return FALSE;

			const PanelItem *pItem = it->get();

			if ((pInfo->Mask & PANEL_ITEM_GET_INFO_MASK_STATE) != 0) {
				pInfo->State = pItem->State;
			}

			if ((pInfo->Mask & PANEL_ITEM_GET_INFO_MASK_HWNDPARENT) != 0) {
				if (pItem->pItem != nullptr)
					pInfo->hwndParent = pItem->pItem->GetHandle();
				else
					pInfo->hwndParent = nullptr;
			}

			if ((pInfo->Mask & PANEL_ITEM_GET_INFO_MASK_HWNDITEM) != 0) {
				if (pItem->pItem != nullptr)
					pInfo->hwndItem = pItem->pItem->GetItemHandle();
				else
					pInfo->hwndItem = nullptr;
			}

			if ((pInfo->Mask & PANEL_ITEM_GET_INFO_MASK_STYLE) != 0) {
				pInfo->Style = pItem->Style;
			}
		}
		return TRUE;

	case MESSAGE_SELECTCHANNEL:
		return SendPluginMessage(pParam, Message, lParam1, lParam2);

	case MESSAGE_GETFAVORITELIST:
		{
			FavoriteList *pList = reinterpret_cast<FavoriteList*>(lParam1);

			if (pList == nullptr || pList->Size != sizeof(FavoriteList))
				return FALSE;

			return GetFavoriteList(GetAppClass().FavoritesManager.GetRootFolder(), pList);
		}

	case MESSAGE_FREEFAVORITELIST:
		{
			FavoriteList *pList = reinterpret_cast<FavoriteList*>(lParam1);

			if (pList != nullptr && pList->Size == sizeof(FavoriteList))
				FreeFavoriteList(pList);
		}
		return 0;

	case MESSAGE_GET1SEGMODE:
		return GetAppClass().Core.Is1SegMode();

	case MESSAGE_SET1SEGMODE:
		return SendPluginMessage(pParam, Message, lParam1, lParam2);

	case MESSAGE_GETDPI:
	case MESSAGE_GETFONT:
	case MESSAGE_SHOWDIALOG:
		return SendPluginMessage(pParam, Message, lParam1, lParam2);

	case MESSAGE_CONVERTTIME:
		{
			ConvertTimeInfo *pInfo = reinterpret_cast<ConvertTimeInfo*>(lParam1);

			if (pInfo == nullptr || pInfo->Size != sizeof(ConvertTimeInfo))
				return FALSE;

			const bool fFromFileTime =
				(pInfo->Flags & CONVERT_TIME_FLAG_FROM_FILETIME) != 0;
			const bool fToFileTime =
				(pInfo->Flags & CONVERT_TIME_FLAG_TO_FILETIME) != 0;

			if (pInfo->TypeFrom == pInfo->TypeTo) {
				if (!fFromFileTime && fToFileTime) {
					if (!::SystemTimeToFileTime(&pInfo->From.SystemTime, &pInfo->To.FileTime))
						return FALSE;
				} else if (fFromFileTime && !fToFileTime) {
					if (!::FileTimeToSystemTime(&pInfo->From.FileTime, &pInfo->To.SystemTime))
						return FALSE;
				} else {
					pInfo->To = pInfo->From;
				}
				return TRUE;
			}

			::ZeroMemory(&pInfo->To, sizeof(pInfo->To));

			SYSTEMTIME stFrom, stTo;

			if (fFromFileTime) {
				if (!::FileTimeToSystemTime(&pInfo->From.FileTime, &stFrom))
					return FALSE;
			} else {
				stFrom = pInfo->From.SystemTime;
			}

			switch (pInfo->TypeFrom) {
			case CONVERT_TIME_TYPE_UTC:
				break;

			case CONVERT_TIME_TYPE_LOCAL:
				if (!::TzSpecificLocalTimeToSystemTime(nullptr, &stFrom, &stTo))
					return FALSE;
				stFrom = stTo;
				break;

			case CONVERT_TIME_TYPE_EPG:
				{
					LibISDB::DateTime From, To;
					From.FromSYSTEMTIME(stFrom);
					if (!LibISDB::EPGTimeToUTCTime(From, &To))
						return FALSE;
					stFrom = To.ToSYSTEMTIME();
				}
				break;

			default:
				return FALSE;
			}

			switch (pInfo->TypeTo) {
			case CONVERT_TIME_TYPE_UTC:
				stTo = stFrom;
				break;

			case CONVERT_TIME_TYPE_LOCAL:
				if (!::SystemTimeToTzSpecificLocalTime(nullptr, &stFrom, &stTo))
					return FALSE;
				break;

			case CONVERT_TIME_TYPE_EPG:
				{
					LibISDB::DateTime From, To;
					From.FromSYSTEMTIME(stFrom);
					if (!LibISDB::UTCTimeToEPGTime(From, &To))
						return FALSE;
					stTo = To.ToSYSTEMTIME();
				}
				break;

			case CONVERT_TIME_TYPE_EPG_DISPLAY:
				{
					const CEpgOptions &EpgOptions = GetAppClass().EpgOptions;

					switch (EpgOptions.GetEpgTimeMode()) {
					case CEpgOptions::EpgTimeMode::Raw:
						{
							LibISDB::DateTime From, To;
							From.FromSYSTEMTIME(stFrom);
							if (!LibISDB::UTCTimeToEPGTime(From, &To))
								return FALSE;
							stTo = To.ToSYSTEMTIME();
						}
						break;

					case CEpgOptions::EpgTimeMode::JST:
						{
							TIME_ZONE_INFORMATION tzi;
							if (!GetJSTTimeZoneInformation(&tzi)
									|| !::SystemTimeToTzSpecificLocalTime(&tzi, &stFrom, &stTo))
								return FALSE;
						}
						break;

					case CEpgOptions::EpgTimeMode::Local:
						{
							LibISDB::DateTime From, To;
							From.FromSYSTEMTIME(stFrom);
							if (!LibISDB::EPGTimeToLocalTime(From, &To))
								return FALSE;
							stTo = To.ToSYSTEMTIME();
						}
						break;

					case CEpgOptions::EpgTimeMode::UTC:
						stTo = stFrom;
						break;

					default:
						return FALSE;
					}
				}
				break;

			default:
				return FALSE;
			}

			if ((pInfo->Flags & CONVERT_TIME_FLAG_OFFSET) != 0
					&& pInfo->Offset != 0) {
				if (!OffsetSystemTime(&stTo, pInfo->Offset))
					return FALSE;
			}

			if (fToFileTime) {
				if (!::SystemTimeToFileTime(&stTo, &pInfo->To.FileTime))
					return FALSE;
			} else {
				pInfo->To.SystemTime = stTo;
			}
		}
		return TRUE;

	case MESSAGE_SETVIDEOSTREAMCALLBACK:
		{
			VideoStreamCallbackFunc pCallback =
				reinterpret_cast<VideoStreamCallbackFunc>(lParam1);
			LibISDB::ViewerFilter *pViewer =
				GetAppClass().CoreEngine.GetFilter<LibISDB::ViewerFilter>();
			if (pViewer == nullptr)
				return FALSE;
			BlockLock Lock(m_VideoStreamLock);

			if (pCallback != nullptr) {
				if (m_VideoStreamCallbackList.empty()) {
					pViewer->SetVideoStreamCallback(&m_VideoStreamCallback);
				} else {
					for (auto it = m_VideoStreamCallbackList.begin(); it != m_VideoStreamCallbackList.end(); ++it) {
						if (it->m_pPlugin == this) {
							m_VideoStreamCallbackList.erase(it);
							break;
						}
					}
				}
				m_VideoStreamCallbackList.emplace_back(this, pCallback, reinterpret_cast<void*>(lParam2));
			} else {
				bool fFound = false;
				for (auto it = m_VideoStreamCallbackList.begin(); it != m_VideoStreamCallbackList.end(); ++it) {
					if (it->m_pPlugin == this) {
						m_VideoStreamCallbackList.erase(it);
						fFound = true;
						break;
					}
				}
				if (!fFound)
					return FALSE;
				if (m_VideoStreamCallbackList.empty())
					pViewer->SetVideoStreamCallback(nullptr);
			}
		}
		return TRUE;

	case MESSAGE_GETVARSTRINGCONTEXT:
		{
			VarStringContext *pContext = new VarStringContext;

			if (!GetAppClass().Core.GetVariableStringEventInfo(&pContext->Event)) {
				delete pContext;
				return reinterpret_cast<LRESULT>(nullptr);
			}

			return reinterpret_cast<LRESULT>(pContext);
		}

	case MESSAGE_FREEVARSTRINGCONTEXT:
		{
			VarStringContext *pContext = reinterpret_cast<VarStringContext*>(lParam1);

			if (pContext == nullptr)
				return FALSE;

			delete pContext;
		}
		return TRUE;

	case MESSAGE_FORMATVARSTRING:
		{
			VarStringFormatInfo *pInfo =
				reinterpret_cast<VarStringFormatInfo*>(lParam1);

			if (pInfo == nullptr
					|| pInfo->Size != sizeof(VarStringFormatInfo))
				return FALSE;

			pInfo->pszResult = nullptr;

			if (pInfo->pszFormat == nullptr)
				return FALSE;

			VarStringContext Context;
			if (pInfo->pContext == nullptr) {
				if (!GetAppClass().Core.GetVariableStringEventInfo(&Context.Event))
					return FALSE;
			}
			CPluginVarStringMap VarStrMap(
				pInfo,
				pInfo->pContext != nullptr ? pInfo->pContext : &Context);
			String Result;

			if (!FormatVariableString(&VarStrMap, pInfo->pszFormat, &Result))
				return FALSE;

			pInfo->pszResult = static_cast<LPWSTR>(std::malloc((Result.length() + 1) * sizeof(WCHAR)));
			if (pInfo->pszResult == nullptr)
				return FALSE;
			std::memcpy(pInfo->pszResult, Result.c_str(), (Result.length() + 1) * sizeof(WCHAR));
		}
		return TRUE;

	case MESSAGE_REGISTERVARIABLE:
		{
			const RegisterVariableInfo *pInfo =
				reinterpret_cast<const RegisterVariableInfo*>(lParam1);

			if (pInfo == nullptr
					|| pInfo->Size != sizeof(RegisterVariableInfo)
					|| IsStringEmpty(pInfo->pszKeyword))
				return FALSE;

			CAppMain &App = GetAppClass();
			CVariableManager::VariableFlag Flags = CVariableManager::VariableFlag::None;
			if ((pInfo->Flags & REGISTER_VARIABLE_FLAG_OVERRIDE) != 0)
				Flags |= CVariableManager::VariableFlag::Override;

			if (!App.VariableManager.RegisterVariable(
						pInfo->pszKeyword, pInfo->pszValue,
						pInfo->pszValue == nullptr ? &m_GetVariable : nullptr,
						pInfo->pszDescription,
						Flags))
				return FALSE;
			App.AppEventManager.OnVariableChanged();
		}
		return TRUE;

	case MESSAGE_GETDARKMODESTATUS:
		{
			const CAppMain &App = GetAppClass();
			DWORD Status = 0;
			if (IsDarkAppModeSupported()) {
				Status |= DARK_MODE_STATUS_APP_SUPPORTED;
				if (App.StyleManager.IsUseDarkMenu())
					Status |= DARK_MODE_STATUS_MENU_DARK;
			}
			if (IsDarkThemeSupported())
				Status |= DARK_MODE_STATUS_PANEL_SUPPORTED;
			if (App.MainWindow.IsDarkMode())
				Status |= DARK_MODE_STATUS_MAINWINDOW_DARK;
			if (App.Epg.ProgramGuideFrame.IsDarkMode())
				Status |= DARK_MODE_STATUS_PROGRAMGUIDE_DARK;
			if (App.StyleManager.IsDarkDialog() && IsDarkThemeSupported() && IsDarkMode())
				Status |= DARK_MODE_STATUS_DIALOG_DARK;
			return Status;
		}

	case MESSAGE_ISDARKMODECOLOR:
		return IsDarkThemeColor(static_cast<COLORREF>(lParam1));

	case MESSAGE_SETWINDOWDARKMODE:
		{
			const HWND hwnd = reinterpret_cast<HWND>(lParam1);
			if (hwnd == nullptr)
				return FALSE;
			const bool fDark = lParam2 != 0;

			if (!(::GetWindowStyle(hwnd) & WS_CHILD)) {
				if (!SetWindowFrameDarkMode(hwnd, fDark))
					return FALSE;
			}

			return SetWindowDarkTheme(hwnd, fDark);
		}

	case MESSAGE_GETELEMENTARYSTREAMINFOLIST:
		{
			ElementaryStreamInfoList *pList = reinterpret_cast<ElementaryStreamInfoList *>(lParam1);

			if (pList == nullptr
					|| pList->Size != sizeof(ElementaryStreamInfoList))
				return FALSE;

			pList->ESCount = 0;
			pList->ESList = nullptr;

			if (pList->Flags != 0)
				return FALSE;

			const CCoreEngine &CoreEngine = GetAppClass().CoreEngine;
			const LibISDB::AnalyzerFilter *pAnalyzer = CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
			if (pAnalyzer == nullptr)
				return FALSE;

			WORD ServiceID = pList->ServiceID;
			if (ServiceID == 0) {
				ServiceID = CoreEngine.GetServiceID();
				if (ServiceID == LibISDB::SERVICE_ID_INVALID)
					return FALSE;
			}
			LibISDB::AnalyzerFilter::ServiceInfo Info;
			if (!pAnalyzer->GetServiceInfoByID(ServiceID, &Info))
				return FALSE;

			switch (pList->Media) {
			case ES_MEDIA_ALL:
				if (!CopyESList(Info.ESList, pList))
					return FALSE;
				break;

			case ES_MEDIA_VIDEO:
				if (!CopyESList(Info.VideoESList, pList))
					return FALSE;
				break;

			case ES_MEDIA_AUDIO:
				if (!CopyESList(Info.AudioESList, pList))
					return FALSE;
				break;

			case ES_MEDIA_CAPTION:
				if (!CopyESList(Info.CaptionESList, pList))
					return FALSE;
				break;

			case ES_MEDIA_DATA_CARROUSEL:
				if (!CopyESList(Info.DataCarrouselESList, pList))
					return FALSE;
				break;

			default:
				return FALSE;
			}
		}
		return TRUE;

	case MESSAGE_GETSERVICECOUNT:
		{
			const CCoreEngine &CoreEngine = GetAppClass().CoreEngine;
			const LibISDB::AnalyzerFilter *pAnalyzer = CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
			if (pAnalyzer != nullptr)
				return pAnalyzer->GetServiceCount();
		}
		return 0;

	case MESSAGE_GETSERVICEINFO2:
		{
			const int Service = static_cast<int>(lParam1);
			ServiceInfo2 *pServiceInfo = reinterpret_cast<ServiceInfo2*>(lParam2);

			if (pServiceInfo == nullptr
					|| pServiceInfo->Size != sizeof(ServiceInfo2)
					|| (pServiceInfo->Flags & ~(
							SERVICE_INFO2_FLAG_BY_ID |
							SERVICE_INFO2_FLAG_BY_SELECTABLE_INDEX)) != 0)
				return FALSE;

			const CCoreEngine &CoreEngine = GetAppClass().CoreEngine;
			const LibISDB::AnalyzerFilter *pAnalyzer = CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
			if (pAnalyzer == nullptr)
				return FALSE;

			LibISDB::AnalyzerFilter::ServiceInfo Info;

			if (Service == -1) {
				const uint16_t ServiceID = CoreEngine.GetServiceID();
				if (ServiceID == LibISDB::SERVICE_ID_INVALID)
					return FALSE;
				if (!pAnalyzer->GetServiceInfoByID(ServiceID, &Info))
					return FALSE;
			} else if ((pServiceInfo->Flags & SERVICE_INFO2_FLAG_BY_ID) != 0) {
				if ((pServiceInfo->Flags & SERVICE_INFO2_FLAG_BY_SELECTABLE_INDEX) != 0)
					return FALSE;
				if (!pAnalyzer->GetServiceInfoByID(static_cast<uint16_t>(Service), &Info))
					return FALSE;
			} else if ((pServiceInfo->Flags & SERVICE_INFO2_FLAG_BY_SELECTABLE_INDEX) != 0) {
				const uint16_t ServiceID = CoreEngine.GetSelectableServiceID(Service);
				if (ServiceID == LibISDB::SERVICE_ID_INVALID)
					return FALSE;
				if (!pAnalyzer->GetServiceInfoByID(ServiceID, &Info))
					return FALSE;
			} else {
				if (!pAnalyzer->GetServiceInfo(Service, &Info))
					return FALSE;
			}

			AnalyzerServiceInfoToServiceInfo2(Info, pServiceInfo);
			// 厳密にいえば GetServiceInfo() と同期が必要
			pServiceInfo->NetworkID = pAnalyzer->GetNetworkID();
			pServiceInfo->TransportStreamID = pAnalyzer->GetTransportStreamID();
		}
		return TRUE;

	case MESSAGE_GETSERVICEINFOLIST:
		{
			ServiceInfoList *pList = reinterpret_cast<ServiceInfoList*>(lParam1);

			if (pList == nullptr
					|| pList->Size != sizeof(ServiceInfoList)
					|| (pList->Flags & ~(
							SERVICE_INFO_LIST_FLAG_SELECTABLE_ONLY |
							SERVICE_INFO_LIST_FLAG_SDT_ACTUAL)) != 0)
				return FALSE;

			pList->Reserved = 0;
			pList->ServiceCount = 0;
			pList->ServiceList = nullptr;

			const CCoreEngine &CoreEngine = GetAppClass().CoreEngine;
			const LibISDB::AnalyzerFilter *pAnalyzer = CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
			if (pAnalyzer == nullptr)
				return FALSE;

			// 厳密にいえば GetServiceList() 等と同期が必要
			const uint16_t NetworkID = pAnalyzer->GetNetworkID();
			const uint16_t TransportStreamID = pAnalyzer->GetTransportStreamID();

			if ((pList->Flags & SERVICE_INFO_LIST_FLAG_SDT_ACTUAL) != 0) {
				// 同時に指定できないフラグ
				if ((pList->Flags & SERVICE_INFO_LIST_FLAG_SELECTABLE_ONLY) != 0)
					return FALSE;

				LibISDB::AnalyzerFilter::SDTServiceList SDTList;
				if (!pAnalyzer->GetSDTServiceList(&SDTList) || SDTList.empty())
					return FALSE;
				pList->ServiceList = static_cast<ServiceInfo2*>(std::malloc(sizeof(ServiceInfo2) * SDTList.size()));
				if (pList->ServiceList == nullptr)
					return FALSE;
				pList->ServiceCount = static_cast<DWORD>(SDTList.size());

				for (size_t i = 0; i < SDTList.size(); i++) {
					ServiceInfo2 &Info = pList->ServiceList[i];
					const LibISDB::AnalyzerFilter::SDTServiceInfo &SDTInfo = SDTList[i];

					Info.Size = sizeof(ServiceInfo2);
					Info.Flags = SERVICE_INFO2_FLAG_NONE;
					Info.Status = SERVICE_INFO2_STATUS_NONE;
					if (SDTInfo.FreeCAMode)
						Info.Status |= SERVICE_INFO2_STATUS_FREE_CA_MODE;
					Info.NetworkID = NetworkID;
					Info.TransportStreamID = TransportStreamID;
					Info.ServiceID = SDTInfo.ServiceID;
					Info.ServiceType = SDTInfo.ServiceType;
					Info.Reserved = 0;
					Info.PMT_PID = 0xFFFF;
					Info.PCR_PID = 0xFFFF;
					StringCopy(Info.szServiceName, SDTInfo.ServiceName.c_str());
					StringCopy(Info.szProviderName, SDTInfo.ProviderName.c_str());
				}
			} else {
				LibISDB::AnalyzerFilter::ServiceList List;

				if ((pList->Flags & SERVICE_INFO_LIST_FLAG_SELECTABLE_ONLY) != 0) {
					if (!CoreEngine.GetSelectableServiceList(&List) || List.empty())
						return FALSE;
				} else {
					if (!pAnalyzer->GetServiceList(&List) || List.empty())
						return FALSE;
				}

				pList->ServiceList = static_cast<ServiceInfo2*>(std::malloc(sizeof(ServiceInfo2) * List.size()));
				if (pList->ServiceList == nullptr)
					return FALSE;
				pList->ServiceCount = static_cast<DWORD>(List.size());

				for (size_t i = 0; i < List.size(); i++) {
					ServiceInfo2 &Info = pList->ServiceList[i];
					Info.Size = sizeof(ServiceInfo2);
					Info.Flags = SERVICE_INFO2_FLAG_NONE;
					AnalyzerServiceInfoToServiceInfo2(List[i], &Info);
					Info.NetworkID = NetworkID;
					Info.TransportStreamID = TransportStreamID;
				}
			}
		}
		return TRUE;

	case MESSAGE_GETAUDIOINFO:
		{
			AudioInfo *pInfo = reinterpret_cast<AudioInfo*>(lParam1);

			if (pInfo == nullptr
					|| pInfo->Size != sizeof(AudioInfo))
				return FALSE;

			const LibISDB::ViewerFilter *pViewer =
				GetAppClass().CoreEngine.GetFilter<LibISDB::ViewerFilter>();
			if (pViewer == nullptr)
				return FALSE;
			LibISDB::ViewerFilter::AudioInfo Info;
			if (!pViewer->GetAudioInfo(&Info))
				return FALSE;

			pInfo->Status = AUDIO_INFO_STATUS_NONE;
			if (Info.DualMono)
				pInfo->Status |= AUDIO_INFO_STATUS_DUAL_MONO;
			if (pViewer->IsSPDIFPassthrough())
				pInfo->Status |= AUDIO_INFO_STATUS_SPDIF;

			if (Info.OriginalChannelCount == 2) {
				switch (pViewer->GetStereoMode()) {
				case LibISDB::DirectShow::AudioDecoderFilter::StereoMode::Left:
					pInfo->Status |= AUDIO_INFO_STATUS_LEFT_ONLY;
					break;
				case LibISDB::DirectShow::AudioDecoderFilter::StereoMode::Right:
					pInfo->Status |= AUDIO_INFO_STATUS_RIGHT_ONLY;
					break;
				}
			}

			pInfo->Frequency = Info.Frequency;
			pInfo->OriginalChannelCount = Info.OriginalChannelCount;
			pInfo->OutputChannelCount = pViewer->GetAudioOutputChannelCount();
			if (pInfo->OutputChannelCount == LibISDB::ViewerFilter::AudioChannelCount_Invalid)
				pInfo->OutputChannelCount = 0;
		}
		return TRUE;

	case MESSAGE_GETELEMENTARYSTREAMCOUNT:
		{
			const ElementaryStreamMediaType Media = static_cast<ElementaryStreamMediaType>(lParam1);
			WORD ServiceID = static_cast<WORD>(lParam2);

			const CCoreEngine &CoreEngine = GetAppClass().CoreEngine;
			const LibISDB::AnalyzerFilter *pAnalyzer = CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
			if (pAnalyzer == nullptr)
				return 0;

			if (ServiceID == 0) {
				ServiceID = CoreEngine.GetServiceID();
				if (ServiceID == LibISDB::SERVICE_ID_INVALID)
					return 0;
			}
			const int ServiceIndex = pAnalyzer->GetServiceIndexByID(ServiceID);
			if (ServiceIndex < 0)
				return 0;

			switch (Media) {
			case ES_MEDIA_ALL:
				return pAnalyzer->GetESCount(ServiceIndex);

			case ES_MEDIA_VIDEO:
				return pAnalyzer->GetVideoESCount(ServiceIndex);

			case ES_MEDIA_AUDIO:
				return pAnalyzer->GetAudioESCount(ServiceIndex);

			case ES_MEDIA_CAPTION:
				return pAnalyzer->GetCaptionESCount(ServiceIndex);

			case ES_MEDIA_DATA_CARROUSEL:
				return pAnalyzer->GetDataCarrouselESCount(ServiceIndex);
			}
		}
		return 0;

	case MESSAGE_SELECTAUDIO:
	case MESSAGE_GETSELECTEDAUDIO:
		return SendPluginMessage(pParam, Message, lParam1, lParam2);

	case MESSAGE_GETCURRENTEPGEVENTINFO:
		{
			const WORD ServiceID = LOWORD(lParam1);
			const bool fNext = (lParam2 & 1) != 0;
			LibISDB::EventInfo EventInfo;

			if (!GetAppClass().CoreEngine.GetCurrentEventInfo(&EventInfo, ServiceID, fNext))
				return reinterpret_cast<LRESULT>(nullptr);

			CEpgDataConverter Converter;
			return reinterpret_cast<LRESULT>(Converter.Convert(EventInfo));
		}

#ifdef _DEBUG
	default:
		TRACE(TEXT("CPluign::OnCallback() : Unknown message {}\n"), static_cast<UINT>(Message));
		break;
#endif
	}

	return 0;
}


void CPlugin::SetMessageWindow(HWND hwnd, UINT Message)
{
	m_hwndMessage = hwnd;
	m_MessageCode = Message;
}


LRESULT CPlugin::OnPluginMessage(WPARAM wParam, LPARAM lParam)
{
	PluginMessageParam *pParam = reinterpret_cast<PluginMessageParam*>(lParam);

	if (pParam == nullptr || wParam != pParam->Message)
		return 0;

	switch (static_cast<UINT>(wParam)) {
	case MESSAGE_GETCURRENTCHANNELINFO:
		{
			ChannelInfo *pChannelInfo = reinterpret_cast<ChannelInfo*>(pParam->lParam1);

			if (pChannelInfo == nullptr
					|| (pChannelInfo->Size != sizeof(ChannelInfo)
						&& pChannelInfo->Size != CHANNELINFO_SIZE_V1
						&& pChannelInfo->Size != CHANNELINFO_SIZE_V2))
				return FALSE;
			const CChannelInfo *pChInfo = GetAppClass().Core.GetCurrentChannelInfo();
			if (pChInfo == nullptr)
				return FALSE;
			ConvertChannelInfo(pChInfo, pChannelInfo);
			const LibISDB::AnalyzerFilter *pAnalyzer =
				GetAppClass().CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
			LibISDB::String Name;
			if (pAnalyzer->GetNetworkName(&Name)) {
				StringCopy(pChannelInfo->szNetworkName, Name.c_str());
			} else {
				pChannelInfo->szNetworkName[0] = '\0';
			}
			if (!pAnalyzer->GetTSName(&Name)) {
				StringCopy(pChannelInfo->szTransportStreamName, Name.c_str());
			} else {
				pChannelInfo->szTransportStreamName[0] = '\0';
			}
		}
		return TRUE;

	case MESSAGE_SETCHANNEL:
		{
			CAppMain &App = GetAppClass();

			App.Core.OpenTuner();

			const int Space = static_cast<int>(pParam->lParam1);

			if (pParam->pPlugin->m_Version < TVTEST_PLUGIN_VERSION_(0, 0, 8))
				return App.Core.SetChannel(Space, static_cast<int>(pParam->lParam2), -1);

			const WORD ServiceID = HIWORD(pParam->lParam2);
			return App.Core.SetChannel(
				Space, static_cast<SHORT>(LOWORD(pParam->lParam2)),
				ServiceID != 0 ? static_cast<int>(ServiceID) : -1);
		}

	case MESSAGE_SETSERVICE:
		{
			if (pParam->lParam2 == 0)
				return GetAppClass().Core.SetServiceByIndex(static_cast<int>(pParam->lParam1), CAppCore::SetServiceFlag::StrictID);
			return GetAppClass().Core.SetServiceByID(static_cast<WORD>(pParam->lParam1), CAppCore::SetServiceFlag::StrictID);
		}

	case MESSAGE_GETTUNINGSPACENAME:
		{
			const LPWSTR pszName = reinterpret_cast<LPWSTR>(pParam->lParam1);
			const int Index = LOWORD(pParam->lParam2);
			const int MaxLength = HIWORD(pParam->lParam2);
			const CTuningSpaceList *pTuningSpaceList = GetAppClass().ChannelManager.GetDriverTuningSpaceList();
			const LPCTSTR pszTuningSpaceName = pTuningSpaceList->GetTuningSpaceName(Index);

			if (pszTuningSpaceName == nullptr)
				return 0;
			if (pszName != nullptr)
				StringCopy(pszName, pszTuningSpaceName, MaxLength);
			return ::lstrlen(pszTuningSpaceName);
		}

	case MESSAGE_GETCHANNELINFO:
		{
			ChannelInfo *pChannelInfo = reinterpret_cast<ChannelInfo*>(pParam->lParam1);
			const int Space = LOWORD(pParam->lParam2);
			const int Channel = HIWORD(pParam->lParam2);

			if (pChannelInfo == nullptr
					|| (pChannelInfo->Size != sizeof(ChannelInfo)
						&& pChannelInfo->Size != CHANNELINFO_SIZE_V1
						&& pChannelInfo->Size != CHANNELINFO_SIZE_V2)
					|| Space < 0 || Channel < 0)
				return FALSE;

			const CChannelManager *pChannelManager = &GetAppClass().ChannelManager;
			const CChannelList *pChannelList = pChannelManager->GetChannelList(Space);
			if (pChannelList == nullptr)
				return FALSE;
			const CChannelInfo *pChInfo = pChannelList->GetChannelInfo(Channel);
			if (pChInfo == nullptr)
				return FALSE;
			ConvertChannelInfo(pChInfo, pChannelInfo);
		}
		return TRUE;

	case MESSAGE_GETDRIVERNAME:
		{
			const LPWSTR pszName = reinterpret_cast<LPWSTR>(pParam->lParam1);
			const int MaxLength = static_cast<int>(pParam->lParam2);
			const LPCTSTR pszDriverName = GetAppClass().CoreEngine.GetDriverFileName();

			if (pszName != nullptr && MaxLength > 0)
				StringCopy(pszName, pszDriverName, MaxLength);
			return ::lstrlen(pszDriverName);
		}

	case MESSAGE_SETDRIVERNAME:
		{
			const LPCWSTR pszDriverName = reinterpret_cast<LPCWSTR>(pParam->lParam1);

			if (pszDriverName == nullptr) {
				GetAppClass().Core.ShutDownTuner();
				return TRUE;
			}

			return GetAppClass().Core.OpenTuner(pszDriverName);
		}

	case MESSAGE_STARTRECORD:
		{
			CAppMain &App = GetAppClass();
			RecordInfo *pRecInfo = reinterpret_cast<RecordInfo*>(pParam->lParam1);
			CRecordManager::TimeSpecInfo StartTime, StopTime;

			if (pRecInfo == nullptr)
				return App.Core.StartRecord(nullptr, nullptr, nullptr, CRecordManager::RecordClient::Plugin);
			if (pRecInfo->Size != sizeof(RecordInfo))
				return FALSE;
			StartTime.Type = CRecordManager::TimeSpecType::NotSpecified;
			if ((pRecInfo->Mask & RECORD_MASK_STARTTIME) != 0) {
				switch (pRecInfo->StartTimeSpec) {
				case RECORD_START_NOTSPECIFIED:
					break;
				case RECORD_START_TIME:
					StartTime.Type = CRecordManager::TimeSpecType::DateTime;
					if ((pRecInfo->Mask & RECORD_MASK_FLAGS) != 0
							&& (pRecInfo->Flags & RECORD_FLAG_UTC) != 0) {
						if (!::FileTimeToSystemTime(&pRecInfo->StartTime.Time, &StartTime.Time.DateTime))
							return FALSE;
					} else {
						if (!LocalFileTimeToSystemTime(&pRecInfo->StartTime.Time, &StartTime.Time.DateTime))
							return FALSE;
					}
					break;
				case RECORD_START_DELAY:
					StartTime.Type = CRecordManager::TimeSpecType::Duration;
					StartTime.Time.Duration = pRecInfo->StartTime.Delay;
					break;
				default:
					return FALSE;
				}
			}
			StopTime.Type = CRecordManager::TimeSpecType::NotSpecified;
			if ((pRecInfo->Mask & RECORD_MASK_STOPTIME) != 0) {
				switch (pRecInfo->StopTimeSpec) {
				case RECORD_STOP_NOTSPECIFIED:
					break;
				case RECORD_STOP_TIME:
					StopTime.Type = CRecordManager::TimeSpecType::DateTime;
					if ((pRecInfo->Mask & RECORD_MASK_FLAGS) != 0
							&& (pRecInfo->Flags & RECORD_FLAG_UTC) != 0) {
						if (!::FileTimeToSystemTime(&pRecInfo->StopTime.Time, &StopTime.Time.DateTime))
							return FALSE;
					} else {
						if (!LocalFileTimeToSystemTime(&pRecInfo->StopTime.Time, &StopTime.Time.DateTime))
							return FALSE;
					}
					break;
				case RECORD_STOP_DURATION:
					StopTime.Type = CRecordManager::TimeSpecType::Duration;
					StopTime.Time.Duration = pRecInfo->StopTime.Duration;
					break;
				default:
					return FALSE;
				}
			}
			return App.Core.StartRecord(
				(pRecInfo->Mask & RECORD_MASK_FILENAME) != 0 ? pRecInfo->pszFileName : nullptr,
				&StartTime, &StopTime,
				CRecordManager::RecordClient::Plugin);
		}

	case MESSAGE_STOPRECORD:
		return GetAppClass().Core.StopRecord();

	case MESSAGE_PAUSERECORD:
		{
			CAppMain &App = GetAppClass();
			const CRecordManager *pRecordManager = &App.RecordManager;
			const bool fPause = pParam->lParam1 != 0;

			if (!pRecordManager->IsRecording())
				return FALSE;
			if (fPause == pRecordManager->IsPaused())
				return FALSE;
			App.UICore.DoCommand(CM_RECORD_PAUSE);
		}
		return TRUE;

	case MESSAGE_GETRECORD:
		{
			RecordInfo *pRecInfo = reinterpret_cast<RecordInfo*>(pParam->lParam1);

			if (pRecInfo == nullptr || pRecInfo->Size != sizeof(RecordInfo))
				return FALSE;

			const CRecordManager *pRecordManager = &GetAppClass().RecordManager;

			if ((pRecInfo->Mask & RECORD_MASK_FILENAME) != 0
					&& pRecInfo->pszFileName != nullptr && pRecInfo->MaxFileName > 0) {
				if (pRecordManager->GetFileName() != nullptr) {
					StringCopy(
						pRecInfo->pszFileName, pRecordManager->GetFileName(),
						pRecInfo->MaxFileName);
				} else {
					pRecInfo->pszFileName[0] = '\0';
				}
			}

			SYSTEMTIME st;
			if (pRecordManager->GetReserveTime(&st)) {
				if ((pRecInfo->Mask & RECORD_MASK_FLAGS) != 0
						&& (pRecInfo->Flags & RECORD_FLAG_UTC) != 0) {
					::SystemTimeToFileTime(&st, &pRecInfo->ReserveTime);
				} else {
					SystemTimeToLocalFileTime(&st, &pRecInfo->ReserveTime);
				}
			} else {
				pRecInfo->ReserveTime = FILETIME();
			}

			if ((pRecInfo->Mask & RECORD_MASK_STARTTIME) != 0) {
				CRecordManager::TimeSpecInfo StartTime;

				if (!pRecordManager->GetStartTimeSpec(&StartTime))
					StartTime.Type = CRecordManager::TimeSpecType::NotSpecified;
				switch (StartTime.Type) {
				case CRecordManager::TimeSpecType::NotSpecified:
					pRecInfo->StartTimeSpec = RECORD_START_NOTSPECIFIED;
					break;
				case CRecordManager::TimeSpecType::DateTime:
					pRecInfo->StartTimeSpec = RECORD_START_TIME;
					if ((pRecInfo->Mask & RECORD_MASK_FLAGS) != 0
							&& (pRecInfo->Flags & RECORD_FLAG_UTC) != 0) {
						::SystemTimeToFileTime(&StartTime.Time.DateTime, &pRecInfo->StartTime.Time);
					} else {
						SystemTimeToLocalFileTime(&StartTime.Time.DateTime, &pRecInfo->StartTime.Time);
					}
					break;
				case CRecordManager::TimeSpecType::Duration:
					pRecInfo->StartTimeSpec = RECORD_START_DELAY;
					pRecInfo->StartTime.Delay = StartTime.Time.Duration;
					break;
				}
			}

			if ((pRecInfo->Mask & RECORD_MASK_STOPTIME) != 0) {
				CRecordManager::TimeSpecInfo StopTime;

				if (!pRecordManager->GetStopTimeSpec(&StopTime))
					StopTime.Type = CRecordManager::TimeSpecType::NotSpecified;
				switch (StopTime.Type) {
				case CRecordManager::TimeSpecType::NotSpecified:
					pRecInfo->StopTimeSpec = RECORD_STOP_NOTSPECIFIED;
					break;
				case CRecordManager::TimeSpecType::DateTime:
					pRecInfo->StopTimeSpec = RECORD_STOP_TIME;
					if ((pRecInfo->Mask & RECORD_MASK_FLAGS) != 0
							&& (pRecInfo->Flags & RECORD_FLAG_UTC) != 0) {
						::SystemTimeToFileTime(&StopTime.Time.DateTime, &pRecInfo->StopTime.Time);
					} else {
						SystemTimeToLocalFileTime(&StopTime.Time.DateTime, &pRecInfo->StopTime.Time);
					}
					break;
				case CRecordManager::TimeSpecType::Duration:
					pRecInfo->StopTimeSpec = RECORD_STOP_DURATION;
					pRecInfo->StopTime.Duration = StopTime.Time.Duration;
					break;
				}
			}
		}
		return TRUE;

	case MESSAGE_MODIFYRECORD:
		{
			CAppMain &App = GetAppClass();
			RecordInfo *pRecInfo = reinterpret_cast<RecordInfo*>(pParam->lParam1);
			CRecordManager::TimeSpecInfo StartTime, StopTime;

			if (pRecInfo == nullptr || pRecInfo->Size != sizeof(RecordInfo))
				return false;
			if ((pRecInfo->Mask & RECORD_MASK_FLAGS) != 0) {
				if ((pRecInfo->Flags & RECORD_FLAG_CANCEL) != 0)
					return App.Core.CancelReservedRecord();
			}
			if ((pRecInfo->Mask & RECORD_MASK_STARTTIME) != 0) {
				switch (pRecInfo->StartTimeSpec) {
				case RECORD_START_NOTSPECIFIED:
					StartTime.Type = CRecordManager::TimeSpecType::NotSpecified;
					break;
				case RECORD_START_TIME:
					StartTime.Type = CRecordManager::TimeSpecType::DateTime;
					if ((pRecInfo->Mask & RECORD_MASK_FLAGS) != 0
							&& (pRecInfo->Flags & RECORD_FLAG_UTC) != 0) {
						if (!::FileTimeToSystemTime(&pRecInfo->StartTime.Time, &StartTime.Time.DateTime))
							return FALSE;
					} else {
						if (!LocalFileTimeToSystemTime(&pRecInfo->StartTime.Time, &StartTime.Time.DateTime))
							return FALSE;
					}
					break;
				case RECORD_START_DELAY:
					StartTime.Type = CRecordManager::TimeSpecType::Duration;
					StartTime.Time.Duration = pRecInfo->StartTime.Delay;
					break;
				default:
					return FALSE;
				}
			}
			if ((pRecInfo->Mask & RECORD_MASK_STOPTIME) != 0) {
				switch (pRecInfo->StopTimeSpec) {
				case RECORD_STOP_NOTSPECIFIED:
					StopTime.Type = CRecordManager::TimeSpecType::NotSpecified;
					break;
				case RECORD_STOP_TIME:
					StopTime.Type = CRecordManager::TimeSpecType::DateTime;
					if ((pRecInfo->Mask & RECORD_MASK_FLAGS) != 0
							&& (pRecInfo->Flags & RECORD_FLAG_UTC) != 0) {
						if (!::FileTimeToSystemTime(&pRecInfo->StopTime.Time, &StopTime.Time.DateTime))
							return FALSE;
					} else {
						if (!LocalFileTimeToSystemTime(&pRecInfo->StopTime.Time, &StopTime.Time.DateTime))
							return FALSE;
					}
					break;
				case RECORD_STOP_DURATION:
					StopTime.Type = CRecordManager::TimeSpecType::Duration;
					StopTime.Time.Duration = pRecInfo->StopTime.Duration;
					break;
				default:
					return FALSE;
				}
			}
			return App.Core.ModifyRecord(
				(pRecInfo->Mask & RECORD_MASK_FILENAME) != 0 ? pRecInfo->pszFileName : nullptr,
				(pRecInfo->Mask & RECORD_MASK_STARTTIME) != 0 ? &StartTime : nullptr,
				(pRecInfo->Mask & RECORD_MASK_STOPTIME) != 0 ? &StopTime : nullptr,
				CRecordManager::RecordClient::Plugin);
		}

	case MESSAGE_GETZOOM:
		return GetAppClass().UICore.GetZoomPercentage();

	case MESSAGE_SETZOOM:
		return GetAppClass().UICore.SetZoomRate(static_cast<int>(pParam->lParam1), static_cast<int>(pParam->lParam2));

	case MESSAGE_GETRECORDSTATUS:
		{
			RecordStatusInfo *pInfo = reinterpret_cast<RecordStatusInfo*>(pParam->lParam1);
			const CRecordManager *pRecordManager = &GetAppClass().RecordManager;

			if (pInfo == nullptr
					|| (pInfo->Size != sizeof(RecordStatusInfo)
						&& pInfo->Size != RECORDSTATUSINFO_SIZE_V1))
				return FALSE;

			const DWORD Flags = static_cast<DWORD>(pParam->lParam2);

			pInfo->Status =
				pRecordManager->IsRecording() ?
					(pRecordManager->IsPaused() ?
						RECORD_STATUS_PAUSED :
						RECORD_STATUS_RECORDING) :
					RECORD_STATUS_NOTRECORDING;
			if (pInfo->Status != RECORD_STATUS_NOTRECORDING) {
				SYSTEMTIME st;
				pRecordManager->GetStartTime(&st);
				if ((Flags & RECORD_STATUS_FLAG_UTC) != 0) {
					::SystemTimeToFileTime(&st, &pInfo->StartTime);
				} else {
					SystemTimeToLocalFileTime(&st, &pInfo->StartTime);
				}
				CRecordManager::TimeSpecInfo StopTimeInfo;
				pRecordManager->GetStopTimeSpec(&StopTimeInfo);
				pInfo->StopTimeSpec = static_cast<RecordStopTime>(StopTimeInfo.Type);
				if (StopTimeInfo.Type == CRecordManager::TimeSpecType::DateTime) {
					if ((Flags & RECORD_STATUS_FLAG_UTC) != 0) {
						::SystemTimeToFileTime(&StopTimeInfo.Time.DateTime, &pInfo->StopTime.Time);
					} else {
						SystemTimeToLocalFileTime(&StopTimeInfo.Time.DateTime, &pInfo->StopTime.Time);
					}
				} else {
					pInfo->StopTime.Duration = StopTimeInfo.Time.Duration;
				}
			} else {
				pInfo->StopTimeSpec = RECORD_STOP_NOTSPECIFIED;
			}
			pInfo->RecordTime = static_cast<DWORD>(pRecordManager->GetRecordTime());
			pInfo->PauseTime = static_cast<DWORD>(pRecordManager->GetPauseTime());
			if (pInfo->Size > RECORDSTATUSINFO_SIZE_V1) {
				if (pInfo->pszFileName != nullptr && pInfo->MaxFileName > 0) {
					if (pRecordManager->IsRecording()) {
						String FileName;
						pRecordManager->GetRecordTask()->GetFileName(&FileName);
						StringCopy(pInfo->pszFileName, FileName.c_str(), pInfo->MaxFileName);
					} else {
						pInfo->pszFileName[0] = '\0';
					}
				}
			}
		}
		return TRUE;

	case MESSAGE_SETVOLUME:
		{
			CUICore *pUICore = &GetAppClass().UICore;
			const int Volume = static_cast<int>(pParam->lParam1);

			if (Volume < 0)
				return pUICore->SetMute(pParam->lParam2 != 0);
			return pUICore->SetVolume(Volume, true);
		}

	case MESSAGE_GETSTEREOMODE:
		{
			int StereoMode;

			switch (GetAppClass().UICore.GetDualMonoMode()) {
			case LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Main:
				StereoMode = STEREOMODE_LEFT;
				break;
			case LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Sub:
				StereoMode = STEREOMODE_RIGHT;
				break;
			case LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Both:
			default:
				StereoMode = STEREOMODE_STEREO;
				break;
			}

			return StereoMode;
		}

	case MESSAGE_SETSTEREOMODE:
		{
			const int StereoMode = static_cast<int>(pParam->lParam1);
			LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode DecoderMode;

			switch (StereoMode) {
			case STEREOMODE_LEFT:
				DecoderMode = LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Main;
				break;
			case STEREOMODE_RIGHT:
				DecoderMode = LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Sub;
				break;
			case STEREOMODE_STEREO:
				DecoderMode = LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Both;
				break;
			default:
				return FALSE;
			}

			return GetAppClass().UICore.SetDualMonoMode(DecoderMode);
		}

	case MESSAGE_SETFULLSCREEN:
		return GetAppClass().UICore.SetFullscreen(pParam->lParam1 != 0);

	case MESSAGE_SETPREVIEW:
		return GetAppClass().UICore.EnableViewer(pParam->lParam1 != 0);

	case MESSAGE_SETSTANDBY:
		return GetAppClass().UICore.SetStandby(pParam->lParam1 != 0);

	case MESSAGE_SETALWAYSONTOP:
		GetAppClass().UICore.SetAlwaysOnTop(pParam->lParam1 != 0);
		return TRUE;

	case MESSAGE_GETTUNINGSPACE:
		{
			const CChannelManager *pChannelManager = &GetAppClass().ChannelManager;
			int *pNumSpaces = reinterpret_cast<int*>(pParam->lParam1);
			int CurSpace;

			if (pNumSpaces != nullptr)
				*pNumSpaces = pChannelManager->GetDriverTuningSpaceList()->NumSpaces();
			CurSpace = pChannelManager->GetCurrentSpace();
			if (CurSpace < 0) {
				const CChannelInfo *pChannelInfo;

				if (CurSpace == CChannelManager::SPACE_ALL
						&& (pChannelInfo = pChannelManager->GetCurrentChannelInfo()) != nullptr) {
					CurSpace = pChannelInfo->GetSpace();
				} else {
					CurSpace = -1;
				}
			}
			return CurSpace;
		}

	case MESSAGE_GETTUNINGSPACEINFO:
		{
			const int Index = static_cast<int>(pParam->lParam1);
			TuningSpaceInfo *pInfo = reinterpret_cast<TuningSpaceInfo*>(pParam->lParam2);

			if (pInfo == nullptr || pInfo->Size != sizeof(TuningSpaceInfo))
				return FALSE;

			const CTuningSpaceList *pTuningSpaceList = GetAppClass().ChannelManager.GetDriverTuningSpaceList();
			const LPCTSTR pszTuningSpaceName = pTuningSpaceList->GetTuningSpaceName(Index);

			if (pszTuningSpaceName == nullptr)
				return FALSE;
			StringCopy(pInfo->szName, pszTuningSpaceName);
			pInfo->Space = static_cast<TuningSpaceType>(pTuningSpaceList->GetTuningSpaceType(Index));
		}
		return TRUE;

	case MESSAGE_SETAUDIOSTREAM:
		{
			CUICore *pUICore = &GetAppClass().UICore;
			const int Index = static_cast<int>(pParam->lParam1);

			if (Index < 0 || Index >= pUICore->GetNumAudioStreams()
					|| !pUICore->SetAudioStream(Index))
				return FALSE;
		}
		return TRUE;

	case MESSAGE_GETDRIVERFULLPATHNAME:
		{
			const LPWSTR pszPath = reinterpret_cast<LPWSTR>(pParam->lParam1);
			const int MaxLength = static_cast<int>(pParam->lParam2);
			TCHAR szFileName[MAX_PATH];

			if (!GetAppClass().CoreEngine.GetDriverPath(szFileName, lengthof(szFileName)))
				return 0;
			if (pszPath != nullptr && MaxLength > 0)
				StringCopy(pszPath, szFileName, MaxLength);
			return ::lstrlen(szFileName);
		}

	case MESSAGE_RELAYRECORD:
		{
			const LPCWSTR pszFileName = reinterpret_cast<LPCWSTR>(pParam->lParam1);

			return GetAppClass().Core.RelayRecord(pszFileName);
		}

	case MESSAGE_SETWINDOWMESSAGECALLBACK:
		pParam->pPlugin->m_pMessageCallback = reinterpret_cast<WindowMessageCallbackFunc>(pParam->lParam1);
		pParam->pPlugin->m_pMessageCallbackClientData = reinterpret_cast<void*>(pParam->lParam2);
		return TRUE;

	case MESSAGE_SETVIDEOSTREAM:
		return GetAppClass().CoreEngine.SetVideoStream(static_cast<int>(pParam->lParam1));

	case MESSAGE_SELECTCHANNEL:
		{
			const ChannelSelectInfo *pInfo =
				reinterpret_cast<const ChannelSelectInfo*>(pParam->lParam1);

			if (pInfo == nullptr || pInfo->Size != sizeof(ChannelSelectInfo))
				return FALSE;

			CChannelInfo Channel;
			CAppCore::SelectChannelFlag Flags = CAppCore::SelectChannelFlag::None;

			Channel.SetSpace(pInfo->Space);
			Channel.SetChannelIndex(pInfo->Channel);
			Channel.SetNetworkID(pInfo->NetworkID);
			Channel.SetTransportStreamID(pInfo->TransportStreamID);
			Channel.SetServiceID(pInfo->ServiceID);
			if (pInfo->pszTuner == nullptr)
				Flags |= CAppCore::SelectChannelFlag::UseCurrentTuner;
			if ((pInfo->Flags & CHANNEL_SELECT_FLAG_STRICTSERVICE) != 0)
				Flags |= CAppCore::SelectChannelFlag::StrictService;
			if ((pInfo->Flags & CHANNEL_SELECT_FLAG_ALLOWDISABLED) != 0)
				Flags |= CAppCore::SelectChannelFlag::AllowDisabled;

			return GetAppClass().Core.SelectChannel(pInfo->pszTuner, Channel, Flags);
		}

	case MESSAGE_SET1SEGMODE:
		return GetAppClass().Core.Set1SegMode(pParam->lParam1 != 0, true);

	case MESSAGE_GETDPI:
		{
			GetDPIInfo *pInfo = reinterpret_cast<GetDPIInfo*>(pParam->lParam1);

			if (pInfo == nullptr)
				return 0;

			CAppMain &App = GetAppClass();

			switch (pInfo->Type) {
			case DPI_TYPE_SYSTEM:
				return GetSystemDPI();

			case DPI_TYPE_WINDOW:
				{
					HWND hwndRoot;
					const Style::CStyleScaling *pStyleScaling = nullptr;

					if (pInfo->hwnd == nullptr || (hwndRoot = ::GetAncestor(pInfo->hwnd, GA_ROOT)) == App.MainWindow.GetHandle())
						pStyleScaling = App.UICore.GetSkin()->GetUIBase()->GetStyleScaling();
					else if (hwndRoot == App.UICore.GetSkin()->GetFullscreenWindow())
						pStyleScaling = App.UICore.GetSkin()->GetUIBase()->GetStyleScaling();
					else if (hwndRoot == App.Panel.Frame.GetHandle())
						pStyleScaling = App.Panel.Frame.GetStyleScaling();
					else if (hwndRoot == App.Epg.ProgramGuide.GetHandle())
						pStyleScaling = App.Epg.ProgramGuide.GetStyleScaling();

					if (pStyleScaling != nullptr)
						return pStyleScaling->GetDPI();

					return GetWindowDPI(hwndRoot);
				}

			case DPI_TYPE_RECT:
			case DPI_TYPE_POINT:
			case DPI_TYPE_MONITOR:
				if ((pInfo->Flags & DPI_FLAG_FORCED) != 0
						&& App.StyleManager.GetForcedDPI() > 0)
					return App.StyleManager.GetForcedDPI();

				if (Util::OS::IsWindows8_1OrLater()) {
					HMONITOR hMonitor;

					switch (pInfo->Type) {
					case DPI_TYPE_RECT:
						hMonitor = ::MonitorFromRect(&pInfo->Rect, MONITOR_DEFAULTTONULL);
						break;
					case DPI_TYPE_POINT:
						hMonitor = ::MonitorFromPoint(pInfo->Point, MONITOR_DEFAULTTONULL);
						break;
					case DPI_TYPE_MONITOR:
						hMonitor = pInfo->hMonitor;
						break;
					}

					if (hMonitor != nullptr) {
						const int DPI = GetMonitorDPI(hMonitor);
						if (DPI != 0)
							return DPI;
					}
				}
				return GetSystemDPI();
			}

			return 0;
		}

	case MESSAGE_GETFONT:
		{
			GetFontInfo *pInfo = reinterpret_cast<GetFontInfo*>(pParam->lParam1);

			if (pInfo == nullptr || pInfo->Size != sizeof(GetFontInfo))
				return FALSE;

			const CAppMain &App = GetAppClass();
			Style::Font Font;

			if (::lstrcmpiW(pInfo->pszName, L"OSDFont") == 0) {
				Font.LogFont = *App.OSDOptions.GetOSDFont();
			} else if (::lstrcmpiW(pInfo->pszName, L"PanelFont") == 0) {
				Font = App.PanelOptions.GetFont();
			} else if (::lstrcmpiW(pInfo->pszName, L"ProgramGuideFont") == 0) {
				Font = App.ProgramGuideOptions.GetFont();
			} else if (::lstrcmpiW(pInfo->pszName, L"StatusBarFont") == 0) {
				Font = App.StatusOptions.GetFont();
			} else {
				pInfo->LogFont = LOGFONT();
				return FALSE;
			}

			pInfo->LogFont = Font.LogFont;

			if (pInfo->DPI != 0 && Font.Size.Unit == Style::UnitType::Point) {
				const LONG Height = ::MulDiv(Font.Size.Value, pInfo->DPI, 72);
				pInfo->LogFont.lfHeight =
					Font.LogFont.lfHeight >= 0 ? Height : -Height;
			}
		}
		return TRUE;

	case MESSAGE_SHOWDIALOG:
		{
			ShowDialogInfo *pInfo =
				reinterpret_cast<ShowDialogInfo*>(pParam->lParam1);

			if (pInfo == nullptr || pInfo->Size != sizeof(ShowDialogInfo))
				return 0;

			class CPluginDialog
				: public CBasicDialog
			{
				ShowDialogInfo m_Info;
				INT_PTR m_Result;

			public:
				CPluginDialog(ShowDialogInfo *pInfo)
					: m_Info(*pInfo)
				{
					if (!!(m_Info.Flags & SHOW_DIALOG_FLAG_DISABLE_DARK_MODE))
						m_fDisableDarkMode = true;
				}

				bool Show(HWND hwndOwner) override
				{
					m_Result = ShowDialog(m_Info.hwndOwner, m_Info.hinst, m_Info.pszTemplate);
					return m_Result == IDOK;
				}

				bool Create(HWND hwndOwner) override
				{
					return CreateDialogWindow(m_Info.hwndOwner, m_Info.hinst, m_Info.pszTemplate);
				}

				INT_PTR GetResult() const { return m_Result; }
				HWND GetHandle() const { return m_hDlg; }

			private:
				INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override
				{
					if (m_Info.pMessageFunc != nullptr) {
						return m_Info.pMessageFunc(
							hDlg, uMsg, wParam,
							uMsg == WM_INITDIALOG ? reinterpret_cast<LPARAM>(m_Info.pClientData) : lParam,
							m_Info.pClientData);
					}
					if (uMsg == WM_INITDIALOG)
						return TRUE;
					return FALSE;
				}

				void OnDestroyed() override
				{
					if (m_fModeless)
						delete this;
				}
			};

			INT_PTR Result;

			if ((pInfo->Flags & SHOW_DIALOG_FLAG_MODELESS) != 0) {
				CPluginDialog *pDialog = new CPluginDialog(pInfo);
				if ((pInfo->Flags & SHOW_DIALOG_FLAG_POSITION) != 0)
					pDialog->SetPosition(pInfo->Position.x, pInfo->Position.y);
				if (!pDialog->Create(pInfo->hwndOwner)) {
					delete pDialog;
					return 0;
				}
				Result = reinterpret_cast<INT_PTR>(pDialog->GetHandle());
			} else {
				CPluginDialog Dialog(pInfo);
				if ((pInfo->Flags & SHOW_DIALOG_FLAG_POSITION) != 0)
					Dialog.SetPosition(pInfo->Position.x, pInfo->Position.y);
				Dialog.Show(pInfo->hwndOwner);
				Result = Dialog.GetResult();
			}

			return Result;
		}

	case MESSAGE_SELECTAUDIO:
		{
			const AudioSelectInfo *pInfo = reinterpret_cast<const AudioSelectInfo *>(pParam->lParam1);

			if (pInfo == nullptr
					|| pInfo->Size != sizeof(AudioSelectInfo)
					|| (pInfo->Flags & ~(
						AUDIO_SELECT_FLAG_COMPONENT_TAG |
						AUDIO_SELECT_FLAG_DUAL_MONO)) != 0)
				return FALSE;

			CAppMain &App = GetAppClass();
			CAudioManager::AudioSelectInfo SelectInfo;

			if ((pInfo->Flags & AUDIO_SELECT_FLAG_COMPONENT_TAG) != 0) {
				SelectInfo.ID = CAudioManager::MakeID(0, pInfo->ComponentTag);
			} else {
				const LibISDB::AnalyzerFilter *pAnalyzer = App.CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
				const uint8_t ComponentTag = pAnalyzer->GetAudioComponentTag(App.CoreEngine.GetServiceIndex(), pInfo->Index);
				if (ComponentTag == LibISDB::COMPONENT_TAG_INVALID)
					return FALSE;
				SelectInfo.ID = CAudioManager::MakeID(0, ComponentTag);
			}

			if ((pInfo->Flags & AUDIO_SELECT_FLAG_DUAL_MONO) != 0) {
				switch (pInfo->DualMono) {
				case DUAL_MONO_CHANNEL_INVALID:
					SelectInfo.DualMono = CAudioManager::DualMonoMode::Invalid;
					break;
				case DUAL_MONO_CHANNEL_MAIN:
					SelectInfo.DualMono = CAudioManager::DualMonoMode::Main;
					break;
				case DUAL_MONO_CHANNEL_SUB:
					SelectInfo.DualMono = CAudioManager::DualMonoMode::Sub;
					break;
				case DUAL_MONO_CHANNEL_BOTH:
					SelectInfo.DualMono = CAudioManager::DualMonoMode::Both;
					break;
				default:
					return FALSE;
				}
			} else {
				SelectInfo.DualMono = App.AudioManager.GetSelectedDualMonoMode();
			}

			App.AudioManager.SetSelectedAudio(&SelectInfo);
			const int Index = App.AudioManager.FindSelectedAudio();
			if (Index < 0)
				return FALSE;
			return App.UICore.SelectAudio(Index);
		}

	case MESSAGE_GETSELECTEDAUDIO:
		{
			AudioSelectInfo *pInfo = reinterpret_cast<AudioSelectInfo *>(pParam->lParam1);

			if (pInfo == nullptr
					|| pInfo->Size != sizeof(AudioSelectInfo)
					|| pInfo->Flags != 0)
				return FALSE;

			const CAppMain &App = GetAppClass();
			const LibISDB::AnalyzerFilter *pAnalyzer = App.CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();

			pInfo->Index = App.UICore.GetAudioStream();
			pInfo->ComponentTag = pAnalyzer->GetAudioComponentTag(App.CoreEngine.GetServiceIndex(), pInfo->Index);
			std::memset(pInfo->Reserved, 0, sizeof(pInfo->Reserved));
			pInfo->DualMono = static_cast<DualMonoChannel>(App.UICore.GetDualMonoMode());
		}
		return TRUE;

#ifdef _DEBUG
	default:
		TRACE(TEXT("CPlugin::OnPluginMessage() : Unknown message {}\n"), pParam->Message);
		break;
#endif
	}
	return 0;
}


static bool GetSettingString(SettingInfo *pSetting, LPCWSTR pszString)
{
	if (pSetting->Type != SETTING_TYPE_STRING)
		return false;
	if (pSetting->Value.pszString != nullptr) {
		if (pSetting->ValueSize == 0)
			return false;
		StringCopy(pSetting->Value.pszString, pszString, pSetting->ValueSize / sizeof(WCHAR));
		pSetting->ValueSize = (::lstrlenW(pSetting->Value.pszString) + 1) * sizeof(WCHAR);
	} else {
		pSetting->ValueSize = (::lstrlenW(pszString) + 1) * sizeof(WCHAR);
	}
	return true;
}

static bool GetSettingFont(SettingInfo *pSetting, const LOGFONT *pFont)
{
	if (pSetting->Type != SETTING_TYPE_DATA)
		return false;
	if (pSetting->Value.pData != nullptr) {
		if (pSetting->ValueSize != sizeof(LOGFONT))
			return false;
		std::memcpy(pSetting->Value.pData, pFont, sizeof(LOGFONT));
	} else {
		pSetting->ValueSize = sizeof(LOGFONT);
	}
	return true;
}

static bool GetSettingFont(SettingInfo *pSetting, const Style::Font &Font)
{
	Style::Font f = Font;
	GetAppClass().MainWindow.GetStyleScaling()->RealizeFontSize(&f);
	return GetSettingFont(pSetting, &f.LogFont);
}

static bool GetSettingInt(SettingInfo *pSetting, int Value)
{
	if (pSetting->Type != SETTING_TYPE_INT)
		return false;
	if (pSetting->ValueSize != sizeof(int))
		return false;
	pSetting->Value.Int = Value;
	return true;
}

bool CPlugin::OnGetSetting(SettingInfo *pSetting) const
{
	if (pSetting == nullptr || pSetting->pszName == nullptr)
		return false;

	const CAppMain &App = GetAppClass();

	if (::lstrcmpiW(pSetting->pszName, L"DriverDirectory") == 0) {
		String Directory;
		App.CoreEngine.GetDriverDirectoryPath(&Directory);
		return GetSettingString(pSetting, Directory.c_str());
	} else if (::lstrcmpiW(pSetting->pszName, L"IniFilePath") == 0) {
		return GetSettingString(pSetting, App.GetIniFileName());
	} else if (::lstrcmpiW(pSetting->pszName, L"RecordFolder") == 0) {
		return GetSettingString(pSetting, App.RecordOptions.GetSaveFolder());
	} else if (::lstrcmpiW(pSetting->pszName, L"RecordFileName") == 0) {
		return GetSettingString(pSetting, App.RecordOptions.GetFileName());
	} else if (::lstrcmpiW(pSetting->pszName, L"CaptureFolder") == 0) {
		return GetSettingString(pSetting, App.CaptureOptions.GetSaveFolder());
	} else if (::lstrcmpiW(pSetting->pszName, L"CaptureFileName") == 0) {
		return GetSettingString(pSetting, App.CaptureOptions.GetFileName());
	} else if (::lstrcmpiW(pSetting->pszName, L"OSDFont") == 0) {
		return GetSettingFont(pSetting, App.OSDOptions.GetOSDFont());
	} else if (::lstrcmpiW(pSetting->pszName, L"PanelFont") == 0) {
		return GetSettingFont(pSetting, App.PanelOptions.GetFont());
	} else if (::lstrcmpiW(pSetting->pszName, L"ProgramGuideFont") == 0) {
		return GetSettingFont(pSetting, App.ProgramGuideOptions.GetFont());
	} else if (::lstrcmpiW(pSetting->pszName, L"StatusBarFont") == 0) {
		return GetSettingFont(pSetting, App.StatusOptions.GetFont());
	}
#ifdef _DEBUG
	else {
		TRACE(TEXT("CPlugin::OnGetSettings() : Unknown setting \"{}\"\n"), pSetting->pszName);
	}
#endif

	return false;
}


void CPlugin::CreateAppCommandList()
{
	if (m_AppCommandList.empty()) {
		const CCommandManager &CommandManager = GetAppClass().CommandManager;
		CCommandManager::CCommandLister CommandLister(CommandManager);
		int ID;

		while ((ID = CommandLister.Next()) != 0) {
			AppCommand &Command = m_AppCommandList.emplace_back();
			Command.ID = ID;
			Command.IDText = CommandManager.GetCommandIDText(ID);
			TCHAR szText[CCommandManager::MAX_COMMAND_TEXT];
			CommandManager.GetCommandText(ID, szText, lengthof(szText));
			Command.Text = szText;
		}
	}
}




CPlugin::CPluginCommandInfo::CPluginCommandInfo(int ID, LPCWSTR pszText, LPCWSTR pszName)
	: m_ID(ID)
	, m_Text(pszText)
	, m_Name(pszName)
{
}


CPlugin::CPluginCommandInfo::CPluginCommandInfo(const CommandInfo &Info)
	: m_ID(Info.ID)
	, m_Text(Info.pszText)
	, m_Name(Info.pszName)
{
}


CPlugin::CPluginCommandInfo::CPluginCommandInfo(const PluginCommandInfo &Info)
	: m_ID(Info.ID)
	, m_Flags(Info.Flags)
	, m_State(Info.State)
	, m_Text(Info.pszText)
	, m_Name(Info.pszName)
{
	m_Icon.Create(Info.hbmIcon);
}


CPlugin::CProgramGuideCommand::CProgramGuideCommand(const ProgramGuideCommandInfo &Info)
	: CPluginCommandInfo(Info.ID, Info.pszText, Info.pszName)
	, m_Type(Info.Type)
{
}




CPlugin::CStreamGrabber::CStreamGrabber(StreamCallbackFunc Callback, void *pClientData)
	: m_Callback(Callback)
	, m_pClientData(pClientData)
{
}


void CPlugin::CStreamGrabber::SetClientData(void *pClientData)
{
	m_pClientData = pClientData;
}


bool CPlugin::CStreamGrabber::ReceiveData(LibISDB::DataBuffer *pData)
{
	if (pData->GetSize() != LibISDB::TS_PACKET_SIZE)
		return true;
	return m_Callback(pData->GetData(), m_pClientData) != FALSE;
}




CPlugin::CPluginStatusItem::CPluginStatusItem(CPlugin *pPlugin, StatusItem *pItem)
	: CStatusItem(
		pItem->ItemID,
		SizeValue(std::abs(pItem->DefaultWidth), pItem->DefaultWidth >= 0 ? SizeUnit::Pixel : SizeUnit::EM))
	, m_pPlugin(pPlugin)
	, m_pItem(pItem)
{
	m_MinWidth = pItem->MinWidth;
	m_MaxWidth = pItem->MaxWidth;
	m_MinHeight = pItem->MinHeight;
	m_fVisible = (pItem->State & STATUS_ITEM_STATE_VISIBLE) != 0;
	ApplyItemStyle();

	m_IDText = ::PathFindFileName(m_pPlugin->GetFileName());
	m_IDText += _T(':');
	m_IDText += pItem->IDText;

	pItem->pItem = this;
}


CPlugin::CPluginStatusItem::~CPluginStatusItem()
{
	if (m_pItem != nullptr)
		m_pItem->pItem = nullptr;
}


void CPlugin::CPluginStatusItem::Draw(
	HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags)
{
	NotifyDraw(hdc, ItemRect, DrawRect, Flags);
}


void CPlugin::CPluginStatusItem::OnLButtonDown(int x, int y)
{
	NotifyMouseEvent(STATUS_ITEM_MOUSE_ACTION_LDOWN, x, y);
}


void CPlugin::CPluginStatusItem::OnLButtonUp(int x, int y)
{
	NotifyMouseEvent(STATUS_ITEM_MOUSE_ACTION_LUP, x, y);
}


void CPlugin::CPluginStatusItem::OnLButtonDoubleClick(int x, int y)
{
	NotifyMouseEvent(STATUS_ITEM_MOUSE_ACTION_LDOUBLECLICK, x, y);
}


void CPlugin::CPluginStatusItem::OnRButtonDown(int x, int y)
{
	NotifyMouseEvent(STATUS_ITEM_MOUSE_ACTION_RDOWN, x, y);
}


void CPlugin::CPluginStatusItem::OnRButtonUp(int x, int y)
{
	NotifyMouseEvent(STATUS_ITEM_MOUSE_ACTION_RUP, x, y);
}


void CPlugin::CPluginStatusItem::OnRButtonDoubleClick(int x, int y)
{
	NotifyMouseEvent(STATUS_ITEM_MOUSE_ACTION_RDOUBLECLICK, x, y);
}


void CPlugin::CPluginStatusItem::OnMButtonDown(int x, int y)
{
	NotifyMouseEvent(STATUS_ITEM_MOUSE_ACTION_MDOWN, x, y);
}


void CPlugin::CPluginStatusItem::OnMButtonUp(int x, int y)
{
	NotifyMouseEvent(STATUS_ITEM_MOUSE_ACTION_MUP, x, y);
}


void CPlugin::CPluginStatusItem::OnMButtonDoubleClick(int x, int y)
{
	NotifyMouseEvent(STATUS_ITEM_MOUSE_ACTION_MDOUBLECLICK, x, y);
}


void CPlugin::CPluginStatusItem::OnMouseMove(int x, int y)
{
	NotifyMouseEvent(STATUS_ITEM_MOUSE_ACTION_MOVE, x, y);
}


bool CPlugin::CPluginStatusItem::OnMouseWheel(int x, int y, bool fHorz, int Delta, int *pCommand)
{
	return NotifyMouseEvent(
		fHorz ?
			STATUS_ITEM_MOUSE_ACTION_HORZWHEEL :
			STATUS_ITEM_MOUSE_ACTION_WHEEL,
		x, y, Delta) != 0;
}


void CPlugin::CPluginStatusItem::OnVisibilityChanged()
{
	if (m_pItem != nullptr) {
		if (m_fVisible)
			m_pItem->State |= STATUS_ITEM_STATE_VISIBLE;
		else
			m_pItem->State &= ~STATUS_ITEM_STATE_VISIBLE;

		if (m_pPlugin != nullptr) {
			StatusItemEventInfo Info;
			Info.ID = m_pItem->ID;
			Info.Event = STATUS_ITEM_EVENT_VISIBILITYCHANGED;
			Info.Param = m_fVisible;
			m_pPlugin->SendEvent(
				EVENT_STATUSITEM_NOTIFY, reinterpret_cast<LPARAM>(&Info), 0);
		}
	}
}


void CPlugin::CPluginStatusItem::OnFocus(bool fFocus)
{
	if (m_pItem != nullptr) {
		if (fFocus)
			m_pItem->State |= STATUS_ITEM_STATE_HOT;
		else
			m_pItem->State &= ~STATUS_ITEM_STATE_HOT;

		if (m_pPlugin != nullptr) {
			StatusItemEventInfo Info;
			Info.ID = m_pItem->ID;
			Info.Event =
				fFocus ?
					STATUS_ITEM_EVENT_ENTER :
					STATUS_ITEM_EVENT_LEAVE;
			Info.Param = 0;
			m_pPlugin->SendEvent(
				EVENT_STATUSITEM_NOTIFY, reinterpret_cast<LPARAM>(&Info), 0);
		}
	}
}


void CPlugin::CPluginStatusItem::OnSizeChanged()
{
	if (m_pPlugin != nullptr && m_pItem != nullptr) {
		StatusItemEventInfo Info;
		Info.ID = m_pItem->ID;
		Info.Event = STATUS_ITEM_EVENT_SIZECHANGED;
		Info.Param = 0;
		m_pPlugin->SendEvent(
			EVENT_STATUSITEM_NOTIFY, reinterpret_cast<LPARAM>(&Info), 0);
	}
}


void CPlugin::CPluginStatusItem::OnCaptureReleased()
{
	NotifyMouseEvent(STATUS_ITEM_MOUSE_ACTION_CAPTURERELEASE, 0, 0);
}


void CPlugin::CPluginStatusItem::OnFontChanged()
{
	if (m_pPlugin != nullptr && m_pItem != nullptr) {
		StatusItemEventInfo Info;
		Info.ID = m_pItem->ID;
		Info.Event = STATUS_ITEM_EVENT_FONTCHANGED;
		Info.Param = 0;
		m_pPlugin->SendEvent(
			EVENT_STATUSITEM_NOTIFY, reinterpret_cast<LPARAM>(&Info), 0);
	}
}


void CPlugin::CPluginStatusItem::DetachItem()
{
	m_pPlugin = nullptr;
	m_pItem = nullptr;
}


HWND CPlugin::CPluginStatusItem::GetWindowHandle() const
{
	if (m_pStatus == nullptr)
		return nullptr;
	return m_pStatus->GetHandle();
}


void CPlugin::CPluginStatusItem::RealizeStyle()
{
	if (m_pPlugin != nullptr && m_pItem != nullptr) {
		StatusItemEventInfo Info;
		Info.ID = m_pItem->ID;
		Info.Event = STATUS_ITEM_EVENT_STYLECHANGED;
		Info.Param = 0;
		m_pPlugin->SendEvent(
			EVENT_STATUSITEM_NOTIFY, reinterpret_cast<LPARAM>(&Info), 0);
	}
}


void CPlugin::CPluginStatusItem::ApplyItemStyle()
{
	if (m_pItem != nullptr) {
		m_Style = StyleFlag::None;
		if ((m_pItem->Style & STATUS_ITEM_STYLE_VARIABLEWIDTH) != 0)
			m_Style |= StyleFlag::VariableWidth;
		if ((m_pItem->Style & STATUS_ITEM_STYLE_FULLROW) != 0)
			m_Style |= StyleFlag::FullRow;
		if ((m_pItem->Style & STATUS_ITEM_STYLE_FORCEFULLROW) != 0)
			m_Style |= StyleFlag::FullRow | StyleFlag::ForceFullRow;
	}
}


void CPlugin::CPluginStatusItem::NotifyDraw(
	HDC hdc, const RECT &ItemRect, const RECT &DrawRect, DrawFlag Flags)
{
	if (m_pPlugin != nullptr && m_pItem != nullptr) {
		StatusItemDrawInfo Info;

		Info.ID = m_pItem->ID;
		Info.Flags = STATUS_ITEM_DRAW_FLAG_NONE;
		if (!!(Flags & DrawFlag::Preview))
			Info.Flags |= STATUS_ITEM_DRAW_FLAG_PREVIEW;
		Info.State = STATUS_ITEM_DRAW_STATE_NONE;
		if (!!(Flags & DrawFlag::Highlight)) {
			Info.State |= STATUS_ITEM_DRAW_STATE_HOT;
			Info.pszStyle = L"status-bar.item.hot";
		} else if (!!(Flags & DrawFlag::Bottom)) {
			Info.pszStyle = L"status-bar.item.bottom";
		} else {
			Info.pszStyle = L"status-bar.item";
		}
		Info.hdc = hdc;
		Info.ItemRect = ItemRect;
		Info.DrawRect = DrawRect;
		Info.Color = ::GetTextColor(hdc);

		m_pPlugin->SendEvent(
			EVENT_STATUSITEM_DRAW, reinterpret_cast<LPARAM>(&Info), 0);
	}
}


LRESULT CPlugin::CPluginStatusItem::NotifyMouseEvent(StatusItemMouseAction Action, int x, int y, int WheelDelta)
{
	if (m_pPlugin != nullptr && m_pItem != nullptr) {
		StatusItemMouseEventInfo Info;

		Info.ID = m_pItem->ID;
		Info.Action = Action;
		Info.hwnd = m_pStatus->GetHandle();
		Info.CursorPos.x = x;
		Info.CursorPos.y = y;
		GetRect(&Info.ItemRect);
		Info.CursorPos.x += Info.ItemRect.left;
		Info.CursorPos.y += Info.ItemRect.top;
		GetClientRect(&Info.ContentRect);
		Info.WheelDelta = WheelDelta;

		return m_pPlugin->SendEvent(
			EVENT_STATUSITEM_MOUSE, reinterpret_cast<LPARAM>(&Info), 0);
	}

	return 0;
}




bool CPlugin::CPluginPanelItem::m_fInitialized = false;


CPlugin::CPluginPanelItem::CPluginPanelItem(CPlugin *pPlugin, PanelItem *pItem)
	: m_pPlugin(pPlugin)
	, m_pItem(pItem)
{
	pItem->pItem = this;
}


CPlugin::CPluginPanelItem::~CPluginPanelItem()
{
	if (m_pItem != nullptr)
		m_pItem->pItem = nullptr;
}


bool CPlugin::CPluginPanelItem::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	static const LPCTSTR CLASS_NAME = TEXT("TVTest Plugin Panel");

	if (m_pPlugin == nullptr || m_pItem == nullptr)
		return false;

	const HINSTANCE hinst = GetAppClass().GetInstance();

	if (!m_fInitialized) {
		WNDCLASS wc;

		wc.style = 0;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hinst;
		wc.hIcon = nullptr;
		wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = nullptr;
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = CLASS_NAME;
		if (::RegisterClass(&wc) == 0)
			return false;
		m_fInitialized = true;
	}

	return CreateBasicWindow(
		hwndParent, Style, ExStyle, ID, CLASS_NAME, m_pItem->Title.c_str(), hinst);
}


void CPlugin::CPluginPanelItem::DetachItem()
{
	m_pPlugin = nullptr;
	m_pItem = nullptr;
	m_hwndItem = nullptr;
}


LRESULT CPlugin::CPluginPanelItem::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			PanelItemCreateEventInfo CreateEvent;

			CreateEvent.EventInfo.ID = m_pItem->ID;
			CreateEvent.EventInfo.Event = PANEL_ITEM_EVENT_CREATE;
			GetAppClass().Panel.Form.GetPageClientRect(&CreateEvent.ItemRect);
			CreateEvent.hwndParent = hwnd;
			CreateEvent.hwndItem = nullptr;

			if (m_pPlugin->SendEvent(
						EVENT_PANELITEM_NOTIFY,
						reinterpret_cast<LPARAM>(&CreateEvent), 0) != 0)
				m_hwndItem = CreateEvent.hwndItem;
		}
		return 0;

	case WM_SIZE:
		if (m_hwndItem != nullptr)
			::MoveWindow(m_hwndItem, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd, &ps);
			DrawUtil::Fill(
				ps.hdc, &ps.rcPaint,
				GetAppClass().ColorSchemeOptions.GetColor(CColorScheme::COLOR_PANELBACK));
			::EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_SETFOCUS:
		if (m_hwndItem != nullptr)
			::SetFocus(m_hwndItem);
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


void CPlugin::CPluginPanelItem::RealizeStyle()
{
	if (m_pItem != nullptr && m_pPlugin != nullptr) {
		PanelItemEventInfo Info;
		Info.ID = m_pItem->ID;
		Info.Event = PANEL_ITEM_EVENT_STYLECHANGED;
		m_pPlugin->SendEvent(
			EVENT_PANELITEM_NOTIFY, reinterpret_cast<LPARAM>(&Info), 0);
	}
}


bool CPlugin::CPluginPanelItem::SetFont(const Style::Font &Font)
{
	if (m_pItem != nullptr && m_pPlugin != nullptr) {
		PanelItemEventInfo Info;
		Info.ID = m_pItem->ID;
		Info.Event = PANEL_ITEM_EVENT_FONTCHANGED;
		m_pPlugin->SendEvent(
			EVENT_PANELITEM_NOTIFY, reinterpret_cast<LPARAM>(&Info), 0);
	}
	return true;
}


void CPlugin::CPluginPanelItem::OnActivate()
{
	if (m_pItem != nullptr) {
		m_pItem->State |= PANEL_ITEM_STATE_ACTIVE;

		if (m_pPlugin != nullptr) {
			PanelItemEventInfo Info;
			Info.ID = m_pItem->ID;
			Info.Event = PANEL_ITEM_EVENT_ACTIVATE;
			m_pPlugin->SendEvent(
				EVENT_PANELITEM_NOTIFY, reinterpret_cast<LPARAM>(&Info), 0);
		}
	}
}


void CPlugin::CPluginPanelItem::OnDeactivate()
{
	if (m_pItem != nullptr) {
		m_pItem->State &= ~PANEL_ITEM_STATE_ACTIVE;

		if (m_pPlugin != nullptr) {
			PanelItemEventInfo Info;
			Info.ID = m_pItem->ID;
			Info.Event = PANEL_ITEM_EVENT_DEACTIVATE;
			m_pPlugin->SendEvent(
				EVENT_PANELITEM_NOTIFY, reinterpret_cast<LPARAM>(&Info), 0);
		}
	}
}


void CPlugin::CPluginPanelItem::OnVisibilityChanged(bool fVisible)
{
	if (m_pItem != nullptr) {
		if (fVisible)
			m_pItem->State |= PANEL_ITEM_STATE_ENABLED;
		else
			m_pItem->State &= ~PANEL_ITEM_STATE_ENABLED;

		if (m_pPlugin != nullptr) {
			PanelItemEventInfo Info;
			Info.ID = m_pItem->ID;
			Info.Event =
				fVisible ?
					PANEL_ITEM_EVENT_ENABLE :
					PANEL_ITEM_EVENT_DISABLE;
			m_pPlugin->SendEvent(
				EVENT_PANELITEM_NOTIFY, reinterpret_cast<LPARAM>(&Info), 0);
		}
	}
}


void CPlugin::CPluginPanelItem::OnFormDelete()
{
	delete this;
}


bool CPlugin::CPluginPanelItem::DrawIcon(
	HDC hdc, int x, int y, int Width, int Height, const Theme::ThemeColor &Color)
{
	if (m_pItem == nullptr || !m_pItem->Icon.IsCreated())
		return false;

	return m_pItem->Icon.Draw(hdc, x, y, Width, Height, 0, 0, 0, 0, Color);
}


bool CPlugin::CPluginPanelItem::NeedKeyboardFocus() const
{
	return m_pItem != nullptr && (m_pItem->Style & PANEL_ITEM_STYLE_NEEDFOCUS) != 0;
}




void CPlugin::CVideoStreamCallback::OnStream(DWORD Format, const void *pData, size_t Size)
{
	BlockLock Lock(CPlugin::m_VideoStreamLock);

	for (auto &e : CPlugin::m_VideoStreamCallbackList) {
		e.m_pCallback(Format, pData, Size, e.m_pClientData);
	}
}




void CPlugin::CAudioSampleCallback::OnSamples(short *pData, size_t Length, int Channels)
{
	BlockLock Lock(CPlugin::m_AudioStreamLock);

	for (auto &e : CPlugin::m_AudioStreamCallbackList) {
		(e.m_pCallback)(pData, static_cast<DWORD>(Length), Channels, e.m_pClientData);
	}
}




CPlugin::CGetVariable::CGetVariable(CPlugin *pPlugin)
	: m_pPlugin(pPlugin)
{
}


bool CPlugin::CGetVariable::GetVariable(LPCWSTR pszKeyword, String *pValue)
{
	GetVariableInfo Info;

	Info.pszKeyword = pszKeyword;
	Info.pszValue = nullptr;

	if (!m_pPlugin->SendEvent(EVENT_GETVARIABLE, reinterpret_cast<LPARAM>(&Info)))
		return false;

	if (Info.pszValue != nullptr) {
		*pValue = Info.pszValue;
		std::free(Info.pszValue);
	}

	return true;
}




CPluginManager::~CPluginManager()
{
	FreePlugins();
}


void CPluginManager::SortPluginsByName()
{
	if (m_PluginList.size() > 1)
		std::ranges::sort(m_PluginList, CompareName);
}


bool CPluginManager::CompareName(
	const std::unique_ptr<CPlugin> &Plugin1,
	const std::unique_ptr<CPlugin> &Plugin2)
{
	int Cmp;

	Cmp = ::lstrcmpi(Plugin1->GetPluginName(), Plugin2->GetPluginName());
	if (Cmp == 0)
		Cmp = ::lstrcmpi(Plugin1->GetFileName(), Plugin2->GetFileName());
	return Cmp < 0;
}


bool CPluginManager::LoadPlugins(LPCTSTR pszDirectory, const std::vector<LPCTSTR> *pExcludePlugins)
{
	if (pszDirectory == nullptr)
		return false;
	const int DirectoryLength = ::lstrlen(pszDirectory);
	if (DirectoryLength + 7 >= MAX_PATH)	// +7 = "\\*.tvtp"
		return false;

	FreePlugins();

	CAppMain &App = GetAppClass();
	TCHAR szFileName[MAX_PATH];
	WIN32_FIND_DATA wfd;

	::PathCombine(szFileName, pszDirectory, TEXT("*.tvtp"));
	const HANDLE hFind = ::FindFirstFileEx(szFileName, FindExInfoBasic, &wfd, FindExSearchNameMatch, nullptr, 0);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
				continue;

			if (pExcludePlugins != nullptr) {
				bool fExclude = false;
				for (const LPCTSTR pszName : *pExcludePlugins) {
					if (IsEqualFileName(pszName, wfd.cFileName)) {
						fExclude = true;
						break;
					}
				}
				if (fExclude) {
					App.AddLog(
						TEXT("{} は除外指定されているため読み込まれません。"),
						wfd.cFileName);
					continue;
				}
			}

			if (DirectoryLength + 1 + ::lstrlen(wfd.cFileName) >= MAX_PATH)
				continue;

			CPlugin *pPlugin = new CPlugin;

			::PathCombine(szFileName, pszDirectory, wfd.cFileName);
			if (pPlugin->Load(szFileName)) {
				App.AddLog(TEXT("{} を読み込みました。"), wfd.cFileName);
				m_PluginList.emplace_back(pPlugin);
			} else {
				App.AddLog(
					CLogItem::LogType::Error,
					TEXT("{} : {}"),
					wfd.cFileName,
					!IsStringEmpty(pPlugin->GetLastErrorText()) ?
					pPlugin->GetLastErrorText() :
					TEXT("プラグインを読み込めません。"));
				if (!IsStringEmpty(pPlugin->GetLastErrorAdvise()))
					App.AddLog(CLogItem::LogType::Error, TEXT("({})"), pPlugin->GetLastErrorAdvise());
				delete pPlugin;
			}
		} while (::FindNextFile(hFind, &wfd));
		::FindClose(hFind);
	}
	SortPluginsByName();
	for (size_t i = 0; i < m_PluginList.size(); i++)
		m_PluginList[i]->SetCommand(CM_PLUGIN_FIRST + static_cast<int>(i));
	return true;
}


void CPluginManager::FreePlugins()
{
	while (!m_PluginList.empty()) {
		m_PluginList.pop_back();
	}
}


CPlugin *CPluginManager::GetPlugin(int Index)
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_PluginList.size())
		return nullptr;
	return m_PluginList[Index].get();
}


const CPlugin *CPluginManager::GetPlugin(int Index) const
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_PluginList.size())
		return nullptr;
	return m_PluginList[Index].get();
}


bool CPluginManager::EnablePlugins(bool fEnable)
{
	for (auto &e : m_PluginList) {
		e->Enable(fEnable);
	}
	return true;
}


int CPluginManager::FindPlugin(const CPlugin *pPlugin) const
{
	for (size_t i = 0; i < m_PluginList.size(); i++) {
		if (m_PluginList[i].get() == pPlugin)
			return static_cast<int>(i);
	}
	return -1;
}


int CPluginManager::FindPluginByFileName(LPCTSTR pszFileName) const
{
	if (pszFileName == nullptr)
		return -1;
	for (size_t i = 0; i < m_PluginList.size(); i++) {
		if (IsEqualFileName(::PathFindFileName(m_PluginList[i]->GetFileName()), pszFileName))
			return static_cast<int>(i);
	}
	return -1;
}


int CPluginManager::FindPluginByCommand(int Command) const
{
	for (size_t i = 0; i < m_PluginList.size(); i++) {
		if (m_PluginList[i]->GetCommand() == Command)
			return static_cast<int>(i);
	}
	return -1;
}


CPlugin *CPluginManager::GetPluginByCommand(int Command)
{
	const int Index = FindPluginByCommand(Command);
	if (Index < 0)
		return nullptr;
	return GetPlugin(Index);
}


CPlugin *CPluginManager::GetPluginByPluginCommand(LPCTSTR pszCommand, LPCTSTR *ppszCommandText)
{
	if (IsStringEmpty(pszCommand))
		return nullptr;

	const LPCTSTR pDelimiter = ::StrChr(pszCommand, _T(':'));
	if (pDelimiter == nullptr || (pDelimiter - pszCommand) >= MAX_PATH)
		return nullptr;

	TCHAR szFileName[MAX_PATH];
	StringCopy(szFileName, pszCommand, (pDelimiter - pszCommand) + 1);

	const int PluginIndex = FindPluginByFileName(szFileName);
	if (PluginIndex < 0)
		return nullptr;

	if (ppszCommandText != nullptr)
		*ppszCommandText = pDelimiter + 1;

	return m_PluginList[PluginIndex].get();
}


bool CPluginManager::DeletePlugin(int Index)
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_PluginList.size())
		return false;
	auto it = m_PluginList.begin();
	std::advance(it, Index);
	m_PluginList.erase(it);
	return true;
}


bool CPluginManager::SetMenu(HMENU hmenu) const
{
	ClearMenu(hmenu);
	if (NumPlugins() > 0) {
		for (const auto &Plugin : m_PluginList) {
			if (!Plugin->IsNoEnabledDisabled()) {
				::AppendMenu(
					hmenu, MF_STRING | MF_ENABLED |
					(Plugin->IsEnabled() ? MF_CHECKED : MF_UNCHECKED),
					Plugin->GetCommand(), Plugin->GetPluginName());
			}
		}
	} else {
		::AppendMenu(hmenu, MF_STRING | MF_GRAYED, 0, TEXT("なし"));
	}
	return true;
}


bool CPluginManager::OnPluginCommand(LPCTSTR pszCommand)
{
	LPCTSTR pszCommandText;
	CPlugin *pPlugin = GetPluginByPluginCommand(pszCommand, &pszCommandText);

	if (pPlugin == nullptr)
		return false;

	return pPlugin->NotifyCommand(pszCommandText);
}


bool CPluginManager::OnProgramGuideCommand(
	LPCTSTR pszCommand, ProgramGuideCommandAction Action, const LibISDB::EventInfo *pEvent,
	const POINT *pCursorPos, const RECT *pItemRect)
{
	if (pszCommand == nullptr)
		return false;

	const LPCTSTR pDelimiter = ::StrChr(pszCommand, _T(':'));
	if (pDelimiter == nullptr || (pDelimiter - pszCommand) >= MAX_PATH)
		return false;

	TCHAR szFileName[MAX_PATH];
	StringCopy(szFileName, pszCommand, (pDelimiter - pszCommand) + 1);

	const int PluginIndex = FindPluginByFileName(szFileName);
	if (PluginIndex < 0)
		return false;
	CPlugin *pPlugin = m_PluginList[PluginIndex].get();
	if (!pPlugin->IsProgramGuideEventEnabled(PROGRAMGUIDE_EVENT_COMMAND_ALWAYS)) {
		if (!pPlugin->IsEnabled()
				|| !pPlugin->IsProgramGuideEventEnabled(PROGRAMGUIDE_EVENT_GENERAL))
			return false;
	}
	return pPlugin->NotifyProgramGuideCommand(pDelimiter + 1, Action, pEvent, pCursorPos, pItemRect);
}


bool CPluginManager::SendEvent(EventCode Event, LPARAM lParam1, LPARAM lParam2)
{
	for (const auto &e : m_PluginList) {
		e->SendEvent(Event, lParam1, lParam2);
	}
	return true;
}


bool CPluginManager::SendProgramGuideEvent(EventCode Event, LPARAM Param1, LPARAM Param2)
{
	bool fSent = false;

	for (const auto &Plugin : m_PluginList) {
		if (Plugin->IsProgramGuideEventEnabled(PROGRAMGUIDE_EVENT_GENERAL)
				&& Plugin->SendEvent(Event, Param1, Param2))
			fSent = true;
	}
	return fSent;
}


bool CPluginManager::SendProgramGuideProgramEvent(EventCode Event, const LibISDB::EventInfo &EventInfo, LPARAM Param)
{
	ProgramGuideProgramInfo ProgramInfo;
	EventInfoToProgramGuideProgramInfo(EventInfo, &ProgramInfo);

	bool fSent = false;

	for (const auto &Plugin : m_PluginList) {
		if (Plugin->IsProgramGuideEventEnabled(PROGRAMGUIDE_EVENT_PROGRAM)
				&& Plugin->SendEvent(Event, reinterpret_cast<LPARAM>(&ProgramInfo), Param))
			fSent = true;
	}
	return fSent;
}


bool CPluginManager::SendFilterGraphEvent(
	EventCode Event, LibISDB::ViewerFilter *pMediaViewer, IGraphBuilder *pGraphBuilder)
{
	FilterGraphInfo Info;

	Info.Flags = FILTER_GRAPH_INFO_FLAG_NONE;
	Info.VideoStreamType = pMediaViewer->GetVideoStreamType();
	::ZeroMemory(Info.Reserved, sizeof(Info.Reserved));
	Info.pGraphBuilder = pGraphBuilder;
	return SendEvent(Event, reinterpret_cast<LPARAM>(&Info));
}


void CPluginManager::OnTunerChanged()
{
	SendEvent(EVENT_DRIVERCHANGE);
}


void CPluginManager::OnTunerShutDown()
{
	SendEvent(EVENT_DRIVERCHANGE);
}


void CPluginManager::OnChannelChanged(AppEvent::ChannelChangeStatus Status)
{
	SendEvent(EVENT_CHANNELCHANGE);
}


void CPluginManager::OnServiceChanged()
{
	SendEvent(EVENT_SERVICECHANGE);
}


void CPluginManager::OnServiceInfoUpdated()
{
	SendEvent(EVENT_SERVICEUPDATE);
}


void CPluginManager::OnServiceListUpdated()
{
	SendEvent(EVENT_SERVICEUPDATE);
}


void CPluginManager::OnRecordingStart(AppEvent::RecordingStartInfo *pInfo)
{
	const CRecordManager *pRecordManager = pInfo->pRecordManager;
	StartRecordInfo Info;

	Info.Size = sizeof(StartRecordInfo);
	Info.Flags = STARTRECORD_FLAG_NONE;
	Info.Modified = STARTRECORD_MODIFIED_NONE;
	Info.Client = static_cast<RecordClient>(pRecordManager->GetClient());
	Info.pszFileName = pInfo->pszFileName;
	Info.MaxFileName = pInfo->MaxFileName;
	CRecordManager::TimeSpecInfo TimeSpec;
	pRecordManager->GetStartTimeSpec(&TimeSpec);
	switch (TimeSpec.Type) {
	case CRecordManager::TimeSpecType::NotSpecified:
		Info.StartTimeSpec = RECORD_START_NOTSPECIFIED;
		break;
	case CRecordManager::TimeSpecType::DateTime:
		Info.StartTimeSpec = RECORD_START_TIME;
		break;
	case CRecordManager::TimeSpecType::Duration:
		Info.StartTimeSpec = RECORD_START_DELAY;
		break;
	}
	if (TimeSpec.Type != CRecordManager::TimeSpecType::NotSpecified) {
		SYSTEMTIME st;
		pRecordManager->GetReservedStartTime(&st);
		SystemTimeToLocalFileTime(&st, &Info.StartTime);
	}
	pRecordManager->GetStopTimeSpec(&TimeSpec);
	switch (TimeSpec.Type) {
	case CRecordManager::TimeSpecType::NotSpecified:
		Info.StopTimeSpec = RECORD_STOP_NOTSPECIFIED;
		break;
	case CRecordManager::TimeSpecType::DateTime:
		Info.StopTimeSpec = RECORD_STOP_TIME;
		SystemTimeToLocalFileTime(&TimeSpec.Time.DateTime, &Info.StopTime.Time);
		break;
	case CRecordManager::TimeSpecType::Duration:
		Info.StopTimeSpec = RECORD_STOP_DURATION;
		Info.StopTime.Duration = TimeSpec.Time.Duration;
		break;
	}

	SendEvent(EVENT_STARTRECORD, reinterpret_cast<LPARAM>(&Info));
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
	const CRecordManager *pRecordManager = &GetAppClass().RecordManager;
	int Status;

	if (pRecordManager->IsRecording()) {
		if (pRecordManager->IsPaused())
			Status = RECORD_STATUS_PAUSED;
		else
			Status = RECORD_STATUS_RECORDING;
	} else {
		Status = RECORD_STATUS_NOTRECORDING;
	}
	SendEvent(EVENT_RECORDSTATUSCHANGE, Status);
}


void CPluginManager::OnRecordingFileChanged(LPCTSTR pszFileName)
{
	SendEvent(EVENT_RELAYRECORD, reinterpret_cast<LPARAM>(pszFileName));
}


void CPluginManager::On1SegModeChanged(bool f1SegMode)
{
	SendEvent(EVENT_1SEGMODECHANGED, f1SegMode);
}


void CPluginManager::OnFullscreenChanged(bool fFullscreen)
{
	SendEvent(EVENT_FULLSCREENCHANGE, fFullscreen);
}


void CPluginManager::OnPlaybackStateChanged(bool fPlayback)
{
	SendEvent(EVENT_PREVIEWCHANGE, fPlayback);
}


void CPluginManager::OnVideoFormatChanged()
{
	SendEvent(EVENT_VIDEOFORMATCHANGE);
}


void CPluginManager::OnVolumeChanged(int Volume)
{
	SendEvent(EVENT_VOLUMECHANGE, Volume, false);
}


void CPluginManager::OnMuteChanged(bool fMute)
{
	SendEvent(EVENT_VOLUMECHANGE, GetAppClass().UICore.GetVolume(), fMute);
}


void CPluginManager::OnDualMonoModeChanged(LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode Mode)
{
	int StereoMode;

	switch (Mode) {
	case LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Main:
		StereoMode = STEREOMODE_LEFT;
		break;
	case LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Sub:
		StereoMode = STEREOMODE_RIGHT;
		break;
	case LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Both:
		StereoMode = STEREOMODE_STEREO;
		break;
	default:
		return;
	}

	SendEvent(EVENT_STEREOMODECHANGE, StereoMode);
}


void CPluginManager::OnAudioStreamChanged(int Stream)
{
	SendEvent(EVENT_AUDIOSTREAMCHANGE, Stream);
}


void CPluginManager::OnAudioFormatChanged()
{
	SendEvent(EVENT_AUDIOFORMATCHANGE);
}


void CPluginManager::OnColorSchemeChanged()
{
	SendEvent(EVENT_COLORCHANGE);
}


void CPluginManager::OnStandbyChanged(bool fStandby)
{
	SendEvent(EVENT_STANDBY, fStandby);
}


void CPluginManager::OnExecute(LPCTSTR pszCommandLine)
{
	SendEvent(EVENT_EXECUTE, reinterpret_cast<LPARAM>(pszCommandLine));
}


void CPluginManager::OnEngineReset()
{
	SendEvent(EVENT_RESET);
}


void CPluginManager::OnStatisticsReset()
{
	SendEvent(EVENT_STATUSRESET);
}


void CPluginManager::OnSettingsChanged()
{
	SendEvent(EVENT_SETTINGSCHANGE);
}


void CPluginManager::OnClose()
{
	SendEvent(EVENT_CLOSE);
}


void CPluginManager::OnStartupDone()
{
	SendEvent(EVENT_STARTUPDONE);
}


void CPluginManager::OnFavoritesChanged()
{
	SendEvent(EVENT_FAVORITESCHANGED);
}


void CPluginManager::OnDarkModeChanged(bool fDarkMode)
{
	SendEvent(EVENT_DARKMODECHANGED, fDarkMode);
}


void CPluginManager::OnMainWindowDarkModeChanged(bool fDarkMode)
{
	SendEvent(EVENT_MAINWINDOWDARKMODECHANGED, fDarkMode);
}


void CPluginManager::OnProgramGuideDarkModeChanged(bool fDarkMode)
{
	SendEvent(EVENT_PROGRAMGUIDEDARKMODECHANGED, fDarkMode);
}


void CPluginManager::OnEventChanged()
{
	SendEvent(EVENT_EVENTCHANGED);
}


void CPluginManager::OnEventInfoChanged()
{
	SendEvent(EVENT_EVENTINFOCHANGED);
}


bool CPluginManager::SendProgramGuideInitializeEvent(HWND hwnd)
{
	return SendProgramGuideEvent(
		EVENT_PROGRAMGUIDE_INITIALIZE, reinterpret_cast<LPARAM>(hwnd));
}


bool CPluginManager::SendProgramGuideFinalizeEvent(HWND hwnd)
{
	return SendProgramGuideEvent(
		EVENT_PROGRAMGUIDE_FINALIZE, reinterpret_cast<LPARAM>(hwnd));
}


bool CPluginManager::SendProgramGuideInitializeMenuEvent(HMENU hmenu, UINT *pCommand)
{
	ProgramGuideInitializeMenuInfo Info;
	Info.hmenu = hmenu;
	Info.Command = *pCommand;
	Info.Reserved = 0;

	bool fSent = false;
	m_ProgramGuideMenuList.clear();

	for (auto &Plugin : m_PluginList) {
		if (Plugin->IsProgramGuideEventEnabled(PROGRAMGUIDE_EVENT_GENERAL)) {
			MenuCommandInfo CommandInfo;

			const int NumCommands =
				static_cast<int>(Plugin->SendEvent(
					EVENT_PROGRAMGUIDE_INITIALIZEMENU,
					reinterpret_cast<LPARAM>(&Info)));
			if (NumCommands > 0) {
				CommandInfo.pPlugin = Plugin.get();
				CommandInfo.CommandFirst = Info.Command;
				CommandInfo.CommandEnd = Info.Command + NumCommands;
				m_ProgramGuideMenuList.push_back(CommandInfo);
				fSent = true;
				Info.Command += NumCommands;
			}
		}
	}
	if (fSent)
		*pCommand = Info.Command;
	return fSent;
}


bool CPluginManager::SendProgramGuideMenuSelectedEvent(UINT Command)
{
	bool fResult = false;

	for (const MenuCommandInfo &CommandInfo : m_ProgramGuideMenuList) {
		if (CommandInfo.CommandFirst <= Command && CommandInfo.CommandEnd > Command) {
			if (FindPlugin(CommandInfo.pPlugin) >= 0) {
				fResult = CommandInfo.pPlugin->SendEvent(
					EVENT_PROGRAMGUIDE_MENUSELECTED,
					Command - CommandInfo.CommandFirst) != 0;
			}
			break;
		}
	}
	m_ProgramGuideMenuList.clear();
	return fResult;
}


bool CPluginManager::SendProgramGuideProgramDrawBackgroundEvent(
	const LibISDB::EventInfo &Event, HDC hdc,
	const RECT &ItemRect, const RECT &TitleRect, const RECT &ContentRect, COLORREF BackgroundColor)
{
	ProgramGuideProgramDrawBackgroundInfo Info;

	Info.hdc = hdc;
	Info.ItemRect = ItemRect;
	Info.TitleRect = TitleRect;
	Info.ContentRect = ContentRect;
	Info.BackgroundColor = BackgroundColor;
	return SendProgramGuideProgramEvent(
		EVENT_PROGRAMGUIDE_PROGRAM_DRAWBACKGROUND,
		Event, reinterpret_cast<LPARAM>(&Info));
}


bool CPluginManager::SendProgramGuideProgramInitializeMenuEvent(
	const LibISDB::EventInfo &Event,
	HMENU hmenu, UINT *pCommand, const POINT &CursorPos, const RECT &ItemRect)
{
	ProgramGuideProgramInfo ProgramInfo;
	EventInfoToProgramGuideProgramInfo(Event, &ProgramInfo);

	ProgramGuideProgramInitializeMenuInfo Info;
	Info.hmenu = hmenu;
	Info.Command = *pCommand;
	Info.Reserved = 0;
	Info.CursorPos = CursorPos;
	Info.ItemRect = ItemRect;

	bool fSent = false;
	m_ProgramGuideMenuList.clear();

	for (auto &Plugin : m_PluginList) {
		if (Plugin->IsProgramGuideEventEnabled(PROGRAMGUIDE_EVENT_PROGRAM)) {
			MenuCommandInfo CommandInfo;

			const int NumCommands =
				static_cast<int>(Plugin->SendEvent(
					EVENT_PROGRAMGUIDE_PROGRAM_INITIALIZEMENU,
					reinterpret_cast<LPARAM>(&ProgramInfo),
					reinterpret_cast<LPARAM>(&Info)));
			if (NumCommands > 0) {
				CommandInfo.pPlugin = Plugin.get();
				CommandInfo.CommandFirst = Info.Command;
				CommandInfo.CommandEnd = Info.Command + NumCommands;
				m_ProgramGuideMenuList.push_back(CommandInfo);
				fSent = true;
				Info.Command += NumCommands;
			}
		}
	}
	if (fSent)
		*pCommand = Info.Command;
	return fSent;
}


bool CPluginManager::SendProgramGuideProgramMenuSelectedEvent(const LibISDB::EventInfo &Event, UINT Command)
{
	bool fResult = false;

	for (const MenuCommandInfo &CommandInfo : m_ProgramGuideMenuList) {
		if (CommandInfo.CommandFirst <= Command && CommandInfo.CommandEnd > Command) {
			if (FindPlugin(CommandInfo.pPlugin) >= 0) {
				ProgramGuideProgramInfo ProgramInfo;

				EventInfoToProgramGuideProgramInfo(Event, &ProgramInfo);
				fResult = CommandInfo.pPlugin->SendEvent(
					EVENT_PROGRAMGUIDE_PROGRAM_MENUSELECTED,
					reinterpret_cast<LPARAM>(&ProgramInfo),
					Command - CommandInfo.CommandFirst) != 0;
			}
			break;
		}
	}
	m_ProgramGuideMenuList.clear();
	return fResult;
}


bool CPluginManager::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
	bool fNoDefault = false;

	for (const auto &e : m_PluginList) {
		if (e->OnMessage(hwnd, uMsg, wParam, lParam, pResult))
			fNoDefault = true;
	}
	return fNoDefault;
}


void CPluginManager::SendFilterGraphInitializeEvent(
	LibISDB::ViewerFilter *pMediaViewer, IGraphBuilder *pGraphBuilder)
{
	SendFilterGraphEvent(EVENT_FILTERGRAPH_INITIALIZE, pMediaViewer, pGraphBuilder);
}


void CPluginManager::SendFilterGraphInitializedEvent(
	LibISDB::ViewerFilter *pMediaViewer, IGraphBuilder *pGraphBuilder)
{
	SendFilterGraphEvent(EVENT_FILTERGRAPH_INITIALIZED, pMediaViewer, pGraphBuilder);
}


void CPluginManager::SendFilterGraphFinalizeEvent(
	LibISDB::ViewerFilter *pMediaViewer, IGraphBuilder *pGraphBuilder)
{
	SendFilterGraphEvent(EVENT_FILTERGRAPH_FINALIZE, pMediaViewer, pGraphBuilder);
}


void CPluginManager::SendFilterGraphFinalizedEvent(
	LibISDB::ViewerFilter *pMediaViewer, IGraphBuilder *pGraphBuilder)
{
	SendFilterGraphEvent(EVENT_FILTERGRAPH_FINALIZED, pMediaViewer, pGraphBuilder);
}


void CPluginManager::RegisterStatusItems()
{
	for (const auto &e : m_PluginList)
		e->RegisterStatusItems();
}


void CPluginManager::SendStatusItemCreatedEvent()
{
	for (const auto &e : m_PluginList)
		e->SendStatusItemCreatedEvent();
}


void CPluginManager::SendStatusItemUpdateTimerEvent()
{
	for (const auto &e : m_PluginList)
		e->SendStatusItemUpdateTimerEvent();
}


void CPluginManager::RegisterPanelItems()
{
	for (const auto &e : m_PluginList)
		e->RegisterPanelItems();
}




CPluginOptions::CPluginOptions(CPluginManager *pPluginManager)
	: m_pPluginManager(pPluginManager)
{
}


CPluginOptions::~CPluginOptions()
{
	Destroy();
}


bool CPluginOptions::LoadSettings(CSettings &Settings)
{
	if (Settings.SetSection(TEXT("PluginList"))) {
		int Count;

		if (Settings.Read(TEXT("PluginCount"), &Count) && Count > 0) {
			for (int i = 0; i < Count; i++) {
				TCHAR szName[32], szFileName[MAX_PATH];

				StringFormat(szName, TEXT("Plugin{}_Name"), i);
				if (!Settings.Read(szName, szFileName, lengthof(szFileName)))
					break;
				if (szFileName[0] != '\0') {
					bool fEnable;

					StringFormat(szName, TEXT("Plugin{}_Enable"), i);
					if (Settings.Read(szName, &fEnable) && fEnable) {
						m_EnablePluginList.emplace_back(szFileName);
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
	Settings.Write(TEXT("PluginCount"), static_cast<unsigned int>(m_EnablePluginList.size()));
	for (size_t i = 0; i < m_EnablePluginList.size(); i++) {
		TCHAR szName[32];

		StringFormat(szName, TEXT("Plugin{}_Name"), i);
		Settings.Write(szName, m_EnablePluginList[i]);
		StringFormat(szName, TEXT("Plugin{}_Enable"), i);
		Settings.Write(szName, true);
	}

	return true;
}


bool CPluginOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(
		hwndOwner,
		GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_OPTIONS_PLUGIN));
}


bool CPluginOptions::RestorePluginOptions()
{
	for (const String &Name : m_EnablePluginList) {
		for (int j = 0; j < m_pPluginManager->NumPlugins(); j++) {
			CPlugin *pPlugin = m_pPluginManager->GetPlugin(j);

			if (!pPlugin->IsDisableOnStart()
					&& IsEqualFileName(Name.c_str(), ::PathFindFileName(pPlugin->GetFileName())))
				pPlugin->Enable(true);
		}
	}
	return true;
}


bool CPluginOptions::StorePluginOptions()
{
	ClearList();
	for (int i = 0; i < m_pPluginManager->NumPlugins(); i++) {
		const CPlugin *pPlugin = m_pPluginManager->GetPlugin(i);

		if (pPlugin->IsEnabled()) {
			m_EnablePluginList.emplace_back(::PathFindFileName(pPlugin->GetFileName()));
		}
	}
	return true;
}


void CPluginOptions::ClearList()
{
	m_EnablePluginList.clear();
}


INT_PTR CPluginOptions::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			const HWND hwndList = GetDlgItem(hDlg, IDC_PLUGIN_LIST);
			LVCOLUMN lvc;

			ListView_SetExtendedListViewStyle(
				hwndList, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_GRIDLINES);
			lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			lvc.fmt = LVCFMT_LEFT;
			lvc.cx = 120;
			lvc.pszText = const_cast<LPTSTR>(TEXT("ファイル名"));
			ListView_InsertColumn(hwndList, COLUMN_FILENAME, &lvc);
			lvc.pszText = const_cast<LPTSTR>(TEXT("プラグイン名"));
			ListView_InsertColumn(hwndList, COLUMN_PLUGINNAME, &lvc);
			lvc.pszText = const_cast<LPTSTR>(TEXT("説明"));
			ListView_InsertColumn(hwndList, COLUMN_DESCRIPTION, &lvc);
			lvc.pszText = const_cast<LPTSTR>(TEXT("著作権"));
			ListView_InsertColumn(hwndList, COLUMN_COPYRIGHT, &lvc);

			for (int i = 0; i < m_pPluginManager->NumPlugins(); i++) {
				const CPlugin *pPlugin = m_pPluginManager->GetPlugin(i);
				LVITEM lvi;

				lvi.mask = LVIF_TEXT | LVIF_PARAM;
				lvi.iItem = i;
				lvi.iSubItem = 0;
				lvi.pszText = ::PathFindFileName(pPlugin->GetFileName());
				lvi.lParam = reinterpret_cast<LPARAM>(pPlugin);
				ListView_InsertItem(hwndList, &lvi);
				lvi.mask = LVIF_TEXT;
				lvi.iSubItem = 1;
				lvi.pszText = const_cast<LPTSTR>(pPlugin->GetPluginName());
				ListView_SetItem(hwndList, &lvi);
				lvi.iSubItem = 2;
				lvi.pszText = const_cast<LPTSTR>(pPlugin->GetDescription());
				ListView_SetItem(hwndList, &lvi);
				lvi.iSubItem = 3;
				lvi.pszText = const_cast<LPTSTR>(pPlugin->GetCopyright());
				ListView_SetItem(hwndList, &lvi);
			}

			for (int i = 0; i < NUM_COLUMNS; i++)
				ListView_SetColumnWidth(hwndList, i, LVSCW_AUTOSIZE_USEHEADER);

			SetListViewTooltipsTopMost(hwndList);

			AddControl(IDC_PLUGIN_LIST, AlignFlag::All);
			AddControl(IDC_PLUGIN_SETTINGS, AlignFlag::Bottom);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_PLUGIN_SETTINGS:
			{
				const HWND hwndList = ::GetDlgItem(hDlg, IDC_PLUGIN_LIST);
				const int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

				if (Sel >= 0) {
					LVITEM lvi;

					lvi.mask = LVIF_PARAM;
					lvi.iItem = Sel;
					lvi.iSubItem = 0;
					if (ListView_GetItem(hwndList, &lvi))
						reinterpret_cast<CPlugin*>(lvi.lParam)->Settings(hDlg);
				}
			}
			return TRUE;

		case IDC_PLUGIN_UNLOAD:
			{
				const HWND hwndList = ::GetDlgItem(hDlg, IDC_PLUGIN_LIST);
				const int Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

				if (Sel >= 0) {
					LVITEM lvi;

					lvi.mask = LVIF_PARAM;
					lvi.iItem = Sel;
					lvi.iSubItem = 0;
					if (ListView_GetItem(hwndList, &lvi)) {
						const int Index = m_pPluginManager->FindPlugin(reinterpret_cast<CPlugin*>(lvi.lParam));

						if (m_pPluginManager->DeletePlugin(Index))
							ListView_DeleteItem(hwndList, Sel);
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
				const NMLISTVIEW *pnmlv = reinterpret_cast<const NMLISTVIEW*>(lParam);
				const int Sel = ListView_GetNextItem(pnmlv->hdr.hwndFrom, -1, LVNI_SELECTED);

				if (Sel >= 0) {
					LVITEM lvi;

					lvi.mask = LVIF_PARAM;
					lvi.iItem = Sel;
					lvi.iSubItem = 0;
					if (ListView_GetItem(pnmlv->hdr.hwndFrom, &lvi)) {
						const CPlugin *pPlugin = reinterpret_cast<const CPlugin*>(lvi.lParam);
						EnableDlgItem(hDlg, IDC_PLUGIN_SETTINGS, pPlugin->HasSettings());
					}
				} else {
					EnableDlgItem(hDlg, IDC_PLUGIN_SETTINGS, false);
				}
			}
			return TRUE;

		case NM_DBLCLK:
			{
				const NMITEMACTIVATE *pnmia = reinterpret_cast<const NMITEMACTIVATE*>(lParam);

				if (pnmia->iItem >= 0) {
					LVITEM lvi;

					lvi.mask = LVIF_PARAM;
					lvi.iItem = pnmia->iItem;
					lvi.iSubItem = 0;
					if (ListView_GetItem(pnmia->hdr.hwndFrom, &lvi))
						reinterpret_cast<CPlugin*>(lvi.lParam)->Settings(hDlg);
				}
			}
			return TRUE;

		case NM_RCLICK:
			{
				const NMHDR *pnmh = reinterpret_cast<const NMHDR*>(lParam);
				const int Sel = ListView_GetNextItem(pnmh->hwndFrom, -1, LVNI_SELECTED);

				if (Sel >= 0) {
					LVITEM lvi;

					lvi.mask = LVIF_PARAM;
					lvi.iItem = Sel;
					lvi.iSubItem = 0;
					if (ListView_GetItem(pnmh->hwndFrom, &lvi)) {
						const HMENU hmenu =
							::LoadMenu(
								GetAppClass().GetResourceInstance(),
								MAKEINTRESOURCE(IDM_PLUGIN));
						const CPlugin *pPlugin = reinterpret_cast<const CPlugin*>(lvi.lParam);
						POINT pt;

						::EnableMenuItem(
							hmenu, IDC_PLUGIN_SETTINGS,
							pPlugin->HasSettings() ? MFS_ENABLED : MFS_GRAYED);
						::EnableMenuItem(
							hmenu, IDC_PLUGIN_UNLOAD,
							pPlugin->CanUnload() ? MFS_ENABLED : MFS_GRAYED);
						::GetCursorPos(&pt);
						::TrackPopupMenu(
							::GetSubMenu(hmenu, 0), TPM_RIGHTBUTTON,
							pt.x, pt.y, 0, hDlg, nullptr);
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


void CPluginOptions::RealizeStyle()
{
	CBasicDialog::RealizeStyle();

	if (m_hDlg != nullptr) {
		const HWND hwndList = ::GetDlgItem(m_hDlg, IDC_PLUGIN_LIST);

		for (int i = 0; i < NUM_COLUMNS; i++)
			ListView_SetColumnWidth(hwndList, i, LVSCW_AUTOSIZE_USEHEADER);
	}
}


}	// namespace TVTest
