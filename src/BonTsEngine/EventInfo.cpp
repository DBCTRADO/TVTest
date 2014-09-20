#include "stdafx.h"
#include "EventInfo.h"
#include "TsUtilClass.h"
#include "TsEncode.h"
#include "TsUtil.h"
#include <utility>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CEventInfo::CEventInfo()
	: m_bValidStartTime(false)
	, m_bCommonEvent(false)
	, m_Type(0)
	, m_UpdatedTime(0)
{
}


CEventInfo::CEventInfo(CEventInfo &&Src)
{
	*this = std::move(Src);
}


CEventInfo &CEventInfo::operator=(CEventInfo &&Op)
{
	if (&Op != this) {
		m_NetworkID = Op.m_NetworkID;
		m_TransportStreamID = Op.m_TransportStreamID;
		m_ServiceID = Op.m_ServiceID;
		m_EventID = Op.m_EventID;
		m_bValidStartTime = Op.m_bValidStartTime;
		m_StartTime = Op.m_StartTime;
		m_Duration = Op.m_Duration;
		m_RunningStatus = Op.m_RunningStatus;
		m_bFreeCaMode = Op.m_bFreeCaMode;
		m_EventName = std::move(Op.m_EventName);
		m_EventText = std::move(Op.m_EventText);
		m_EventExtendedText = std::move(Op.m_EventExtendedText);
		m_VideoList = std::move(Op.m_VideoList);
		m_AudioList = std::move(Op.m_AudioList);
		m_ContentNibble = Op.m_ContentNibble;
		m_EventGroupList = std::move(Op.m_EventGroupList);
		m_bCommonEvent = Op.m_bCommonEvent;
		m_CommonEventInfo = Op.m_CommonEventInfo;
		m_Type = Op.m_Type;
		m_UpdatedTime = Op.m_UpdatedTime;
	}

	return *this;
}


bool CEventInfo::operator==(const CEventInfo &Op) const
{
	return IsEqual(Op)
		&& m_Type == Op.m_Type
		&& m_UpdatedTime == Op.m_UpdatedTime;
}


bool CEventInfo::IsEqual(const CEventInfo &Op) const
{
	return m_NetworkID == Op.m_NetworkID
		&& m_TransportStreamID == Op.m_TransportStreamID
		&& m_ServiceID == Op.m_ServiceID
		&& m_EventID == Op.m_EventID
		&& m_bValidStartTime == Op.m_bValidStartTime
		&& (!m_bValidStartTime
			|| std::memcmp(&m_StartTime, &Op.m_StartTime, sizeof(SYSTEMTIME)) == 0)
		&& m_Duration == Op.m_Duration
		&& m_RunningStatus == Op.m_RunningStatus
		&& m_bFreeCaMode == Op.m_bFreeCaMode
		&& m_EventName == Op.m_EventName
		&& m_EventText == Op.m_EventText
		&& m_EventExtendedText == Op.m_EventExtendedText
		&& m_VideoList == Op.m_VideoList
		&& m_AudioList == Op.m_AudioList
		&& m_ContentNibble == Op.m_ContentNibble
		&& m_EventGroupList == Op.m_EventGroupList
		&& m_bCommonEvent == Op.m_bCommonEvent
		&& (!m_bCommonEvent
			|| m_CommonEventInfo == Op.m_CommonEventInfo);
}


bool CEventInfo::GetStartTime(SYSTEMTIME *pTime) const
{
	if (pTime == nullptr)
		return false;

	if (m_bValidStartTime) {
		*pTime = m_StartTime;
		return true;
	}

	::ZeroMemory(pTime, sizeof(SYSTEMTIME));

	return false;
}


bool CEventInfo::GetEndTime(SYSTEMTIME *pTime) const
{
	if (pTime == nullptr)
		return false;

	if (m_bValidStartTime) {
		CDateTime Time(m_StartTime);
		if (Time.Offset(CDateTime::SECONDS(m_Duration))) {
			Time.Get(pTime);
			return true;
		}
	}

	::ZeroMemory(pTime, sizeof(SYSTEMTIME));

	return false;
}




// 拡張テキストを取得する
int GetEventExtendedText(const CDescBlock *pDescBlock, LPTSTR pszText, int MaxLength)
{
	pszText[0] = '\0';

	std::vector<const CExtendedEventDesc *> DescList;
	for (int i = 0; i < pDescBlock->GetDescNum(); i++) {
		const CBaseDesc *pDesc = pDescBlock->GetDescByIndex(i);
		if (pDesc != NULL && pDesc->GetTag() == CExtendedEventDesc::DESC_TAG) {
			const CExtendedEventDesc *pExtendedEvent = dynamic_cast<const CExtendedEventDesc *>(pDesc);
			if (pExtendedEvent != NULL) {
				DescList.push_back(pExtendedEvent);
			}
		}
	}
	if (DescList.empty())
		return 0;

	// descriptor_number 順にソートする
	TsEngine::InsertionSort(DescList,
		[](const CExtendedEventDesc *pDesc1, const CExtendedEventDesc *pDesc2) {
			return pDesc1->GetDescriptorNumber() < pDesc2->GetDescriptorNumber();
		});

	struct ItemInfo {
		BYTE DescriptorNumber;
		LPCTSTR pszDescription;
		int Data1Length;
		const BYTE *pData1;
		int Data2Length;
		const BYTE *pData2;
	};
	std::vector<ItemInfo> ItemList;
	for (size_t i = 0; i < DescList.size(); i++) {
		const CExtendedEventDesc *pExtendedEvent = DescList[i];
		for (int j = 0; j < pExtendedEvent->GetItemCount(); j++) {
			const CExtendedEventDesc::ItemInfo *pItem = pExtendedEvent->GetItem(j);
			if (pItem == NULL)
				continue;
			if (pItem->szDescription[0] != '\0') {
				// 新規項目
				ItemInfo Item;
				Item.DescriptorNumber = pExtendedEvent->GetDescriptorNumber();
				Item.pszDescription = pItem->szDescription;
				Item.Data1Length = pItem->ItemLength;
				Item.pData1 = pItem->ItemChar;
				Item.Data2Length = 0;
				Item.pData2 = NULL;
				ItemList.push_back(Item);
			} else if (!ItemList.empty()) {
				// 前の項目の続き
				ItemInfo &Item = ItemList[ItemList.size() - 1];
				if (Item.DescriptorNumber == pExtendedEvent->GetDescriptorNumber() - 1
						&& Item.pData2 == NULL) {
					Item.Data2Length = pItem->ItemLength;
					Item.pData2 = pItem->ItemChar;
				}
			}
		}
	}

	int Pos = 0;
	for (size_t i = 0; i < ItemList.size(); i++) {
		ItemInfo &Item = ItemList[i];
		int Length = ::lstrlen(Item.pszDescription);
		if (Length + 2 >= MaxLength - Pos)
			break;
		::lstrcpy(&pszText[Pos], Item.pszDescription);
		Pos += Length;
		pszText[Pos++] = '\r';
		pszText[Pos++] = '\n';
		TCHAR szText[1024];
		if (Item.pData2 == NULL) {
			CAribString::AribToString(szText, _countof(szText), Item.pData1, Item.Data1Length);
		} else {
			BYTE Buffer[220 * 2];
			::CopyMemory(Buffer, Item.pData1, Item.Data1Length);
			::CopyMemory(Buffer + Item.Data1Length, Item.pData2, Item.Data2Length);
			CAribString::AribToString(szText, _countof(szText), Buffer, Item.Data1Length + Item.Data2Length);
		}
		LPTSTR p = szText;
		while (*p != '\0') {
			if (Pos >= MaxLength - 1)
				break;
			pszText[Pos++] = *p;
			if (*p == '\r') {
				if (*(p + 1) != '\n') {
					if (Pos == MaxLength - 1)
						break;
					pszText[Pos++] = '\n';
				}
			}
			p++;
		}
		if (Pos + 2 >= MaxLength)
			break;
		pszText[Pos++] = '\r';
		pszText[Pos++] = '\n';
	}
	pszText[Pos] = '\0';

	return Pos;
}
