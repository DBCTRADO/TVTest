#include "stdafx.h"
#include <utility>
#include <algorithm>
#include "TVTest.h"
#include "HomeDisplay.h"
#include "Favorites.h"
#include "EpgProgramList.h"
#include "EpgUtil.h"
#include "LogoManager.h"
#include "ChannelHistory.h"
#include "FeaturedEvents.h"
#include "DriverManager.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


static const int CATEGORY_ICON_WIDTH=32,CATEGORY_ICON_HEIGHT=32;

enum {
	CATEGORY_ICON_FAVORITES,
	CATEGORY_ICON_RECENT_CHANNELS,
	CATEGORY_ICON_FEATURED_EVENTS
};

enum {
	CATEGORY_ID_FAVORITES,
	CATEGORY_ID_RECENT_CHANNELS,
	CATEGORY_ID_FEATURED_EVENTS
};




class ABSTRACT_CLASS(CChannelListCategoryBase) : public CHomeDisplay::CCategory
{
public:
	CChannelListCategoryBase(CHomeDisplay *pHomeDisplay);
	virtual ~CChannelListCategoryBase();

// CHomeDisplay::CCategory
	int GetHeight() const override { return m_Height; }
	void LayOut(const CHomeDisplay::StyleInfo &Style,HDC hdc,const RECT &ContentRect) override;
	void Draw(HDC hdc,const CHomeDisplay::StyleInfo &Style,const RECT &ContentRect,const RECT &PaintRect) const override;
	bool GetCurItemRect(RECT *pRect) const override;
	bool SetFocus(bool fFocus) override;
	bool IsFocused() const override { return m_HotItem>=0; }
	void OnCursorMove(int x,int y) override;
	void OnCursorLeave() override;
	bool OnClick(int x,int y) override;
	bool OnSetCursor() override;
	bool OnCursorKey(WPARAM KeyCode) override;

protected:
	class ABSTRACT_CLASS(CChannelItemBase) : public CChannelInfo
	{
	public:
		CChannelItemBase(const CChannelInfo &ChannelInfo);
		virtual ~CChannelItemBase() {}
		void SetSmallLogo(HBITMAP hbm) { m_hbmSmallLogo=hbm; }
		void SetBigLogo(HBITMAP hbm) { m_hbmBigLogo=hbm; }
		HBITMAP GetStretchedLogo(int Width,int Height) const;
		bool SetEvent(int Index,const CEventInfoData *pEvent);
		const CEventInfoData *GetEvent(int Index) const;

	protected:
		HBITMAP m_hbmSmallLogo;
		HBITMAP m_hbmBigLogo;
		mutable DrawUtil::CBitmap m_StretchedLogo;
		CEventInfoData m_Event[2];
	};

	typedef std::vector<CChannelItemBase*> ItemList;

	RECT m_Rect;
	int m_Height;
	int m_ItemHeight;
	int m_ChannelNameWidth;
	int m_LogoWidth;
	int m_LogoHeight;
	int m_HotItem;
	ItemList m_ItemList;

	void Clear();
	void UpdateChannelInfo();
	bool GetItemRect(size_t Item,RECT *pRect) const;
	int GetItemByPosition(int x,int y) const;
	void RedrawItem(int Item);
	bool SetHotItem(int Item);
};


CChannelListCategoryBase::CChannelListCategoryBase(CHomeDisplay *pHomeDisplay)
	: CCategory(pHomeDisplay)
	, m_Height(0)
	, m_HotItem(-1)
{
}


CChannelListCategoryBase::~CChannelListCategoryBase()
{
	Clear();
}


void CChannelListCategoryBase::LayOut(const CHomeDisplay::StyleInfo &Style,HDC hdc,const RECT &ContentRect)
{
	m_ItemHeight=Style.FontHeight*2+Style.ItemMargins.top+Style.ItemMargins.bottom;
	m_Height=(int)m_ItemList.size()*m_ItemHeight;
	m_Rect=ContentRect;

	m_LogoHeight=min(Style.FontHeight,36);
	m_LogoWidth=(m_LogoHeight*16+4)/9;

	m_ChannelNameWidth=Style.FontHeight*8;
	for (size_t i=0;i<m_ItemList.size();i++) {
		const CChannelItemBase *pItem=m_ItemList[i];
		LPCTSTR pszName=pItem->GetName();
		SIZE sz;

		if (::GetTextExtentPoint32(hdc,pszName,::lstrlen(pszName),&sz)
				&& sz.cx>m_ChannelNameWidth)
			m_ChannelNameWidth=sz.cx;
	}
}


void CChannelListCategoryBase::Draw(HDC hdc,const CHomeDisplay::StyleInfo &Style,const RECT &ContentRect,const RECT &PaintRect) const
{
	for (size_t i=0;i<m_ItemList.size();i++) {
		RECT rcItem,rc;

		GetItemRect(i,&rcItem);
		if (!IsRectIntersect(&rcItem,&PaintRect))
			continue;
		const CChannelItemBase *pItem=m_ItemList[i];
		const Theme::Style *pItemStyle;
		if (i==m_HotItem) {
			pItemStyle=&Style.ItemHotStyle;
		} else {
			pItemStyle=&Style.ItemStyle[i%2];
		}
		Theme::DrawStyleBackground(hdc,&rcItem,pItemStyle);
		::SetTextColor(hdc,pItemStyle->TextColor);
		rc=rcItem;
		rc.left+=Style.ItemMargins.left;
		rc.top+=Style.ItemMargins.top;
		rc.right=rc.left+m_ChannelNameWidth;
		rc.bottom-=Style.ItemMargins.bottom;
		HBITMAP hbmLogo=pItem->GetStretchedLogo(m_LogoWidth,m_LogoHeight);
		if (hbmLogo!=NULL) {
			DrawUtil::DrawBitmap(hdc,rc.left,rc.top+(Style.FontHeight-m_LogoHeight)/2,
								 m_LogoWidth,m_LogoHeight,hbmLogo,NULL,
								 i==m_HotItem?255:224);
			rc.top+=Style.FontHeight;
		}
		::DrawText(hdc,pItem->GetName(),-1,&rc,
			DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);

		rc.left=rc.right+8;
		rc.top=rcItem.top+Style.ItemMargins.top;
		rc.right=rcItem.right-Style.ItemMargins.right;
		rc.bottom=rc.top+Style.FontHeight;

		for (int j=0;j<2;j++) {
			const CEventInfoData *pEventInfo=pItem->GetEvent(j);

			if (pEventInfo!=NULL) {
				TCHAR szText[1024];
				int Length;

				Length=EpgUtil::FormatEventTime(pEventInfo,szText,lengthof(szText),
					EpgUtil::EVENT_TIME_HOUR_2DIGITS | EpgUtil::EVENT_TIME_START_ONLY);
				if (!IsStringEmpty(pEventInfo->GetEventName())) {
					Length+=StdUtil::snprintf(
						szText+Length,lengthof(szText)-Length,
						TEXT("%s%s"),
						Length>0?TEXT(" "):TEXT(""),
						pEventInfo->GetEventName());
				}
				if (Length>0) {
					::DrawText(hdc,szText,Length,&rc,
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
				}
			}

			rc.top=rc.bottom;
			rc.bottom=rc.top+Style.FontHeight;
		}
	}
}


bool CChannelListCategoryBase::GetCurItemRect(RECT *pRect) const
{
	if (m_HotItem<0)
		return false;
	return GetItemRect(m_HotItem,pRect);
}


bool CChannelListCategoryBase::SetFocus(bool fFocus)
{
	if (fFocus) {
		if (m_ItemList.empty())
			return false;
		SetHotItem(0);
	} else {
		SetHotItem(-1);
	}
	return true;
}


void CChannelListCategoryBase::OnCursorMove(int x,int y)
{
	SetHotItem(GetItemByPosition(x,y));
}


void CChannelListCategoryBase::OnCursorLeave()
{
	SetHotItem(-1);
}


bool CChannelListCategoryBase::OnClick(int x,int y)
{
	SetHotItem(GetItemByPosition(x,y));
	return OnDecide();
}


bool CChannelListCategoryBase::OnSetCursor()
{
	if (m_HotItem>=0) {
		::SetCursor(::LoadCursor(NULL,IDC_HAND));
		return true;
	}
	return false;
}


bool CChannelListCategoryBase::OnCursorKey(WPARAM KeyCode)
{
	int HotItem;

	if (KeyCode==VK_UP) {
		if (m_HotItem==0 || m_ItemList.empty())
			return false;
		if (m_HotItem<0)
			HotItem=0;
		else
			HotItem=m_HotItem-1;
	} else if (KeyCode==VK_DOWN) {
		if (m_ItemList.empty() || m_HotItem+1==(int)m_ItemList.size())
			return false;
		if (m_HotItem<0)
			HotItem=0;
		else
			HotItem=m_HotItem+1;
	} else {
		return false;
	}

	SetHotItem(HotItem);

	return true;
}


void CChannelListCategoryBase::Clear()
{
	for (auto it=m_ItemList.begin();it!=m_ItemList.end();++it)
		delete *it;
	m_ItemList.clear();

	m_Height=0;
	m_HotItem=-1;
}


void CChannelListCategoryBase::UpdateChannelInfo()
{
	CEpgProgramList *pEpgProgramList=GetAppClass().GetEpgProgramList();
	CLogoManager *pLogoManager=GetAppClass().GetLogoManager();
	SYSTEMTIME stCurTime;

	GetCurrentJST(&stCurTime);

	for (size_t i=0;i<m_ItemList.size();i++) {
		CChannelItemBase *pItem=m_ItemList[i];
		CEventInfoData EventInfo;

		if (pEpgProgramList->GetEventInfo(
				pItem->GetNetworkID(),
				pItem->GetTransportStreamID(),
				pItem->GetServiceID(),
				&stCurTime,&EventInfo)) {
			pItem->SetEvent(0,&EventInfo);
			SYSTEMTIME st;
			if (EventInfo.m_fValidStartTime
					&& EventInfo.m_DurationSec>0
					&& EventInfo.GetEndTime(&st)
					&& pEpgProgramList->GetEventInfo(
						pItem->GetNetworkID(),
						pItem->GetTransportStreamID(),
						pItem->GetServiceID(),
						&st,&EventInfo)) {
				pItem->SetEvent(1,&EventInfo);
			} else {
				pItem->SetEvent(1,NULL);
			}
		} else {
			pItem->SetEvent(0,NULL);
			if (pEpgProgramList->GetNextEventInfo(
						pItem->GetNetworkID(),
						pItem->GetTransportStreamID(),
						pItem->GetServiceID(),
						&stCurTime,&EventInfo)
					&& DiffSystemTime(&stCurTime,&EventInfo.m_stStartTime)<8*60*60*1000) {
				pItem->SetEvent(1,&EventInfo);
			} else {
				pItem->SetEvent(1,NULL);
			}
		}

		HBITMAP hbmLogo=pLogoManager->GetAssociatedLogoBitmap(
			pItem->GetNetworkID(),pItem->GetServiceID(),CLogoManager::LOGOTYPE_SMALL);
		if (hbmLogo!=NULL)
			pItem->SetSmallLogo(hbmLogo);
		hbmLogo=pLogoManager->GetAssociatedLogoBitmap(
			pItem->GetNetworkID(),pItem->GetServiceID(),CLogoManager::LOGOTYPE_BIG);
		if (hbmLogo!=NULL)
			pItem->SetBigLogo(hbmLogo);
	}
}


bool CChannelListCategoryBase::GetItemRect(size_t Item,RECT *pRect) const
{
	if (Item>=m_ItemList.size())
		return false;

	pRect->left=m_Rect.left;
	pRect->top=m_Rect.top+(int)Item*m_ItemHeight-m_pHomeDisplay->GetScrollPos();
	pRect->right=m_Rect.right;
	pRect->bottom=pRect->top+m_ItemHeight;

	return true;
}


int CChannelListCategoryBase::GetItemByPosition(int x,int y) const
{
	POINT pt={x,y};

	if (!::PtInRect(&m_Rect,pt))
		return -1;

	int Item=(y-m_Rect.top+m_pHomeDisplay->GetScrollPos())/m_ItemHeight;
	if ((size_t)Item>=m_ItemList.size())
		return -1;
	return Item;
}


void CChannelListCategoryBase::RedrawItem(int Item)
{
	RECT rc;

	if (GetItemRect(Item,&rc))
		m_pHomeDisplay->Invalidate(&rc);
}


bool CChannelListCategoryBase::SetHotItem(int Item)
{
	int HotItem;

	if (Item>=0) {
		if ((size_t)Item>=m_ItemList.size())
			return false;
		HotItem=Item;
	} else {
		HotItem=-1;
	}

	if (m_HotItem!=HotItem) {
		if (m_HotItem>=0)
			RedrawItem(m_HotItem);
		m_HotItem=HotItem;
		if (HotItem>=0)
			RedrawItem(HotItem);
	}

	return true;
}


CChannelListCategoryBase::CChannelItemBase::CChannelItemBase(const CChannelInfo &ChannelInfo)
	: CChannelInfo(ChannelInfo)
	, m_hbmSmallLogo(NULL)
	, m_hbmBigLogo(NULL)
{
}


HBITMAP CChannelListCategoryBase::CChannelItemBase::GetStretchedLogo(int Width,int Height) const
{
	HBITMAP hbmLogo=
		(Height<=14 || m_hbmBigLogo==NULL)?m_hbmSmallLogo:m_hbmBigLogo;
	if (hbmLogo==NULL)
		return NULL;

	// AlphaBlendでリサイズすると汚いので、予めリサイズした画像を作成しておく
	if (m_StretchedLogo.IsCreated()) {
		if (m_StretchedLogo.GetWidth()!=Width || m_StretchedLogo.GetHeight()!=Height)
			m_StretchedLogo.Destroy();
	}
	if (!m_StretchedLogo.IsCreated()) {
		HBITMAP hbm=DrawUtil::ResizeBitmap(hbmLogo,Width,Height);
		if (hbm!=NULL)
			m_StretchedLogo.Attach(hbm);
	}
	return m_StretchedLogo.GetHandle();
}


bool CChannelListCategoryBase::CChannelItemBase::SetEvent(int Index,const CEventInfoData *pEvent)
{
	if (Index<0 || Index>1)
		return false;
	if (pEvent!=NULL)
		m_Event[Index]=*pEvent;
	else
		m_Event[Index].SetEventName(NULL);
	return true;
}


const CEventInfoData *CChannelListCategoryBase::CChannelItemBase::GetEvent(int Index) const
{
	if (Index<0 || Index>1)
		return NULL;
	if (m_Event[Index].GetEventName()==NULL)
		return NULL;
	return &m_Event[Index];
}




class CFavoritesCategory : public CChannelListCategoryBase
{
public:
	CFavoritesCategory(CHomeDisplay *pHomeDisplay);

// CHomeDisplay::CCategory
	int GetID() const override { return CATEGORY_ID_FAVORITES; }
	LPCTSTR GetTitle() const override { return TEXT("お気に入り"); }
	int GetIconIndex() const override { return CATEGORY_ICON_FAVORITES; }
	bool Create() override;
	bool OnDecide() override;

private:
	class CChannelItem
		: public CChannelItemBase
	{
		TVTest::String m_BonDriverFileName;
		bool m_fForceBonDriverChange;

	public:
		CChannelItem(const TVTest::CFavoriteChannel &Channel)
			: CChannelItemBase(Channel.GetChannelInfo())
			, m_BonDriverFileName(Channel.GetBonDriverFileName())
			, m_fForceBonDriverChange(Channel.GetForceBonDriverChange())
		{
		}

		LPCTSTR GetBonDriverFileName() const { return m_BonDriverFileName.c_str(); }
		bool GetForceBonDriverChange() const { return m_fForceBonDriverChange; }
	};
};


CFavoritesCategory::CFavoritesCategory(CHomeDisplay *pHomeDisplay)
	: CChannelListCategoryBase(pHomeDisplay)
{
}


bool CFavoritesCategory::Create()
{
	class CItemEnumerator : public TVTest::CFavoriteItemEnumerator
	{
		ItemList &m_ItemList;

		bool ChannelItem(TVTest::CFavoriteFolder &Folder,TVTest::CFavoriteChannel &Channel) override
		{
			m_ItemList.push_back(new CChannelItem(Channel));
			return true;
		}

	public:
		CItemEnumerator(ItemList &ItemList)
			: m_ItemList(ItemList)
		{
		}
	};

	TVTest::CFavoritesManager *pFavoritesManager=GetAppClass().GetFavoritesManager();
	CItemEnumerator ItemEnumerator(m_ItemList);

	Clear();
	ItemEnumerator.EnumItems(pFavoritesManager->GetRootFolder());

	UpdateChannelInfo();

	return true;
}


bool CFavoritesCategory::OnDecide()
{
	if (m_HotItem>=0) {
		const CChannelItem *pItem=static_cast<CChannelItem*>(m_ItemList[m_HotItem]);
		CAppMain::ChannelSelectInfo ChSelInfo;

		ChSelInfo.Channel=*static_cast<const CChannelInfo*>(pItem);
		ChSelInfo.TunerName=pItem->GetBonDriverFileName();
		ChSelInfo.fUseCurTuner=!pItem->GetForceBonDriverChange();

		return GetAppClass().SelectChannel(ChSelInfo);
	}

	return false;
}




class CRecentChannelsCategory : public CChannelListCategoryBase
{
public:
	CRecentChannelsCategory(CHomeDisplay *pHomeDisplay);

// CHomeDisplay::CCategory
	int GetID() const override { return CATEGORY_ID_RECENT_CHANNELS; }
	LPCTSTR GetTitle() const override { return TEXT("最近見たチャンネル"); }
	int GetIconIndex() const override { return CATEGORY_ICON_RECENT_CHANNELS; }
	bool Create() override;
	void ReadSettings(CSettings &Settings) override;
	bool OnDecide() override;

private:
	class CChannelItem
		: public CChannelItemBase
	{
		TVTest::String m_BonDriverFileName;

	public:
		CChannelItem(const CRecentChannelList::CChannel *pChannel)
			: CChannelItemBase(*pChannel)
			, m_BonDriverFileName(pChannel->GetDriverFileName())
		{
		}

		LPCTSTR GetBonDriverFileName() const { return m_BonDriverFileName.c_str(); }
	};

	int m_MaxChannels;
};


CRecentChannelsCategory::CRecentChannelsCategory(CHomeDisplay *pHomeDisplay)
	: CChannelListCategoryBase(pHomeDisplay)
	, m_MaxChannels(20)
{
}


bool CRecentChannelsCategory::Create()
{
	Clear();

	CRecentChannelList *pRecentChannelList=GetAppClass().GetRecentChannelList();
	int NumChannels=pRecentChannelList->NumChannels();
	if (NumChannels>m_MaxChannels)
		NumChannels=m_MaxChannels;
	for (int i=0;i<NumChannels;i++) {
		m_ItemList.push_back(new CChannelItem(pRecentChannelList->GetChannelInfo(i)));
	}

	UpdateChannelInfo();

	return true;
}


void CRecentChannelsCategory::ReadSettings(CSettings &Settings)
{
	if (Settings.SetSection(TEXT("HomeDisplay"))) {
		int MaxChannels;
		if (Settings.Read(TEXT("RecentChannels_MaxChannels"),&MaxChannels) && MaxChannels>0)
			m_MaxChannels=MaxChannels;
	}
}


bool CRecentChannelsCategory::OnDecide()
{
	if (m_HotItem>=0) {
		const CChannelItem *pItem=static_cast<CChannelItem*>(m_ItemList[m_HotItem]);
		CAppMain::ChannelSelectInfo ChSelInfo;

		ChSelInfo.Channel=*static_cast<const CChannelInfo*>(pItem);
		ChSelInfo.TunerName=pItem->GetBonDriverFileName();
		ChSelInfo.fUseCurTuner=false;

		return GetAppClass().SelectChannel(ChSelInfo);
	}

	return false;
}




class CFeaturedEventsCategory : public CHomeDisplay::CCategory
{
public:
	CFeaturedEventsCategory(CHomeDisplay *pHomeDisplay,CEventSearchOptions &EventSearchOptions);
	~CFeaturedEventsCategory();

// CHomeDisplay::CCategory
	int GetID() const override { return CATEGORY_ID_FEATURED_EVENTS; }
	LPCTSTR GetTitle() const override { return TEXT("注目の番組"); }
	int GetIconIndex() const override { return CATEGORY_ICON_FEATURED_EVENTS; }
	int GetHeight() const override { return m_Height; }
	bool Create() override;
	void ReadSettings(CSettings &Settings) override;
	void WriteSettings(CSettings &Settings) override;
	void LayOut(const CHomeDisplay::StyleInfo &Style,HDC hdc,const RECT &ContentRect) override;
	void Draw(HDC hdc,const CHomeDisplay::StyleInfo &Style,const RECT &ContentRect,const RECT &PaintRect) const override;
	bool GetCurItemRect(RECT *pRect) const override;
	bool SetFocus(bool fFocus) override;
	bool IsFocused() const override { return m_HotItem>=0; }
	bool OnDecide() override;
	void OnCursorMove(int x,int y) override;
	void OnCursorLeave() override;
	bool OnClick(int x,int y) override;
	bool OnRButtonDown(int x,int y) override;
	bool OnSetCursor() override { return false; }
	bool OnCursorKey(WPARAM KeyCode) override;

private:
	class CEventItem
	{
	public:
		CEventItem(const CChannelInfo &ChannelInfo,const CEventInfoData &EventInfo);
		const CChannelInfo &GetChannelInfo() const { return m_ChannelInfo; }
		const CEventInfoData &GetEventInfo() const { return m_EventInfo; }
		int GetExpandedHeight() const { return m_ExpandedHeight; }
		void SetExpandedHeight(int Height) { m_ExpandedHeight=Height; }
		bool IsExpanded() const { return m_fExpanded; }
		void SetExpanded(bool fExpanded) { m_fExpanded=fExpanded; }
		void SetLogo(HBITMAP hbm);
		HBITMAP GetStretchedLogo(int Width,int Height) const;
		bool GetEventText(TVTest::String *pText) const;

	private:
		CChannelInfo m_ChannelInfo;
		CEventInfoData m_EventInfo;
		int m_ExpandedHeight;
		bool m_fExpanded;
		HBITMAP m_hbmLogo;
		mutable DrawUtil::CBitmap m_StretchedLogo;

		static void AppendEventText(TVTest::String *pString,LPCWSTR pszText);
	};

	CFeaturedEventsSettings m_Settings;
	CEventSearchOptions &m_EventSearchOptions;
	CFeaturedEventsDialog m_Dialog;
	RECT m_Rect;
	int m_Height;
	int m_ItemHeight;
	int m_ChannelNameWidth;
	int m_LogoWidth;
	int m_LogoHeight;
	int m_HotItem;
	std::vector<CEventItem*> m_ItemList;

	void Clear();
	void SortItems(CFeaturedEventsSettings::SortType SortType);
	void ExpandItem(int Item);
	bool GetItemRect(size_t Item,RECT *pRect) const;
	int GetItemByPosition(int x,int y) const;
	void RedrawItem(int Item);
	bool SetHotItem(int Item);
};


CFeaturedEventsCategory::CFeaturedEventsCategory(CHomeDisplay *pHomeDisplay,CEventSearchOptions &EventSearchOptions)
	: CCategory(pHomeDisplay)
	, m_EventSearchOptions(EventSearchOptions)
	, m_Dialog(m_Settings,m_EventSearchOptions)
	, m_Height(0)
	, m_HotItem(-1)
{
}


CFeaturedEventsCategory::~CFeaturedEventsCategory()
{
	Clear();
}


bool CFeaturedEventsCategory::Create()
{
	Clear();

	CChannelList ServiceList;
	GetAppClass().GetDriverManager()->GetAllServiceList(&ServiceList);

	CFeaturedEvents FeaturedEvents(m_Settings);
	FeaturedEvents.Update();

	const size_t EventCount=FeaturedEvents.GetEventCount();
	for (size_t i=0;i<EventCount;i++) {
		const CEventInfoData *pEventInfo=FeaturedEvents.GetEventInfo(i);
		int Index=ServiceList.FindByIDs(
			pEventInfo->m_NetworkID,pEventInfo->m_TSID,pEventInfo->m_ServiceID);

		if (Index>=0)
			m_ItemList.push_back(new CEventItem(*ServiceList.GetChannelInfo(Index),*pEventInfo));
	}

	SortItems(m_Settings.GetSortType());

	return true;
}


void CFeaturedEventsCategory::ReadSettings(CSettings &Settings)
{
	m_Settings.LoadSettings(Settings);
}


void CFeaturedEventsCategory::WriteSettings(CSettings &Settings)
{
	m_Settings.SaveSettings(Settings);
}


void CFeaturedEventsCategory::LayOut(const CHomeDisplay::StyleInfo &Style,HDC hdc,const RECT &ContentRect)
{
	int ItemBaseHeight=2*Style.FontHeight+Style.ItemMargins.top+Style.ItemMargins.bottom;
	m_ItemHeight=ItemBaseHeight;
	if (m_Settings.GetShowEventText())
		m_ItemHeight+=m_Settings.GetEventTextLines()*Style.FontHeight;
	m_Height=(int)m_ItemList.size()*m_ItemHeight;
	m_Rect=ContentRect;

	m_LogoHeight=min(Style.FontHeight,36);
	m_LogoWidth=(m_LogoHeight*16+4)/9;
	CLogoManager *pLogoManager=GetAppClass().GetLogoManager();

	const int EventTextWidth=(ContentRect.right-ContentRect.left)-
		(Style.ItemMargins.left+Style.ItemMargins.top)-Style.FontHeight;
	TVTest::String Text;
	m_ChannelNameWidth=0;
	for (size_t i=0;i<m_ItemList.size();i++) {
		CEventItem *pItem=m_ItemList[i];

		LPCTSTR pszName=pItem->GetChannelInfo().GetName();
		SIZE sz;
		if (::GetTextExtentPoint32(hdc,pszName,::lstrlen(pszName),&sz)
				&& sz.cx>m_ChannelNameWidth)
			m_ChannelNameWidth=sz.cx;

		pItem->SetLogo(pLogoManager->GetAssociatedLogoBitmap(
			pItem->GetChannelInfo().GetNetworkID(),
			pItem->GetChannelInfo().GetServiceID(),
			m_LogoHeight<=14?CLogoManager::LOGOTYPE_SMALL:CLogoManager::LOGOTYPE_BIG));

		int Height=m_ItemHeight;
		if (pItem->GetEventText(&Text)) {
			int Lines=DrawUtil::CalcWrapTextLines(hdc,Text.c_str(),EventTextWidth);
			if (!m_Settings.GetShowEventText()
					|| Lines>m_Settings.GetEventTextLines()) {
				Height=ItemBaseHeight+Lines*Style.FontHeight;
				if (pItem->IsExpanded()) {
					m_Height+=Height-m_ItemHeight;
				}
			}
		}
		pItem->SetExpandedHeight(Height);
	}
}


void CFeaturedEventsCategory::Draw(HDC hdc,const CHomeDisplay::StyleInfo &Style,const RECT &ContentRect,const RECT &PaintRect) const
{
	if (m_ItemList.empty()) {
		LPCTSTR pszText;
		if (m_Settings.GetSearchSettingsList().GetCount()==0) {
			pszText=TEXT("注目の番組を表示するには、右クリックメニューの [注目の番組の設定] から検索条件を登録します。");
		} else {
			pszText=TEXT("検索された番組はありません。");
		}
		RECT rc=ContentRect;
		rc.left+=Style.ItemMargins.left;
		rc.top+=Style.ItemMargins.top;
		rc.right-=Style.ItemMargins.right;
		rc.bottom-=Style.ItemMargins.bottom;
		::SetTextColor(hdc,Style.BannerTextColor);
		::DrawText(hdc,pszText,-1,&rc,DT_LEFT | DT_NOPREFIX | DT_WORDBREAK);
		return;
	}

	HFONT hfontText=static_cast<HFONT>(::GetCurrentObject(hdc,OBJ_FONT));
	LOGFONT lf;
	::GetObject(hfontText,sizeof(lf),&lf);
	lf.lfWeight=FW_BOLD;
	HFONT hfontTitle=::CreateFontIndirect(&lf);

	for (size_t i=0;i<m_ItemList.size();i++) {
		RECT rcItem,rc;

		GetItemRect(i,&rcItem);
		if (!IsRectIntersect(&rcItem,&PaintRect))
			continue;
		const CEventItem *pItem=m_ItemList[i];
		const CChannelInfo &ChannelInfo=pItem->GetChannelInfo();
		const Theme::Style *pItemStyle;
		if (i==m_HotItem) {
			pItemStyle=&Style.ItemHotStyle;
		} else {
			pItemStyle=&Style.ItemStyle[i%2];
		}
		Theme::DrawStyleBackground(hdc,&rcItem,pItemStyle);
		::SetTextColor(hdc,pItemStyle->TextColor);
		rc=rcItem;
		rc.left+=Style.ItemMargins.left;
		rc.top+=Style.ItemMargins.top;
		rc.bottom=rc.top+Style.FontHeight;
		HBITMAP hbmLogo=pItem->GetStretchedLogo(m_LogoWidth,m_LogoHeight);
		if (hbmLogo!=NULL) {
			DrawUtil::DrawBitmap(hdc,rc.left,rc.top+(Style.FontHeight-m_LogoHeight)/2,
								 m_LogoWidth,m_LogoHeight,hbmLogo,NULL,
								 i==m_HotItem?255:224);
		}
		rc.left+=m_LogoWidth+Style.ItemMargins.left;
		rc.right=rc.left+m_ChannelNameWidth;
		::DrawText(hdc,ChannelInfo.GetName(),-1,&rc,
			DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);

		rc.left=rc.right+Style.FontHeight;
		rc.right=rcItem.right-Style.ItemMargins.right;

		const CEventInfoData &EventInfo=pItem->GetEventInfo();
		TCHAR szText[1024];
		int Length=EpgUtil::FormatEventTime(&EventInfo,szText,lengthof(szText),
											EpgUtil::EVENT_TIME_DATE);
		if (Length>0) {
			::DrawText(hdc,szText,Length,&rc,
				DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
		}
		rc.left=rcItem.left+Style.ItemMargins.left;
		rc.top=rc.bottom;
		rc.bottom=rc.top+Style.FontHeight;
		if (!IsStringEmpty(EventInfo.GetEventName())) {
			::SelectObject(hdc,hfontTitle);
			::DrawText(hdc,EventInfo.GetEventName(),-1,&rc,
				DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
			::SelectObject(hdc,hfontText);
		}

		if (m_Settings.GetShowEventText() || pItem->IsExpanded()) {
			TVTest::String Text;

			if (pItem->GetEventText(&Text)) {
				rc.left=rcItem.left+Style.ItemMargins.left+Style.FontHeight;
				rc.top=rc.bottom;
				rc.bottom=rcItem.bottom-Style.ItemMargins.bottom;
				DrawUtil::DrawWrapText(hdc,Text.c_str(),&rc,Style.FontHeight,
									   DrawUtil::DRAW_TEXT_ELLIPSIS);
			}
		}
	}

	::DeleteObject(hfontTitle);
}


bool CFeaturedEventsCategory::GetCurItemRect(RECT *pRect) const
{
	if (m_HotItem<0)
		return false;
	return GetItemRect(m_HotItem,pRect);
}


bool CFeaturedEventsCategory::SetFocus(bool fFocus)
{
	if (fFocus) {
		if (m_ItemList.empty())
			return false;
		SetHotItem(0);
	} else {
		SetHotItem(-1);
	}
	return true;
}


bool CFeaturedEventsCategory::OnDecide()
{
	if (m_HotItem>=0) {
		ExpandItem(m_HotItem);
	}
	return false;
}


void CFeaturedEventsCategory::OnCursorMove(int x,int y)
{
	SetHotItem(GetItemByPosition(x,y));
}


void CFeaturedEventsCategory::OnCursorLeave()
{
	SetHotItem(-1);
}


bool CFeaturedEventsCategory::OnClick(int x,int y)
{
	SetHotItem(GetItemByPosition(x,y));
	return OnDecide();
}


bool CFeaturedEventsCategory::OnRButtonDown(int x,int y)
{
	enum {
		ID_SETTINGS=1
	};

	HMENU hmenu=::LoadMenu(GetAppClass().GetResourceInstance(),
						   MAKEINTRESOURCE(IDM_FEATUREDEVENTS));

	::CheckMenuItem(hmenu,CM_FEATUREDEVENTS_SHOWEVENTTEXT,
					MF_BYCOMMAND | (m_Settings.GetShowEventText()?MF_CHECKED:MF_UNCHECKED));
	::CheckMenuRadioItem(hmenu,
						 CM_FEATUREDEVENTS_SORT_FIRST,
						 CM_FEATUREDEVENTS_SORT_LAST,
						 CM_FEATUREDEVENTS_SORT_FIRST+(int)m_Settings.GetSortType(),
						 MF_BYCOMMAND);

	POINT pt;
	::GetCursorPos(&pt);
	int Result=::TrackPopupMenu(::GetSubMenu(hmenu,0),TPM_RIGHTBUTTON | TPM_RETURNCMD,
								pt.x,pt.y,0,m_pHomeDisplay->GetHandle(),NULL);
	::DestroyMenu(hmenu);

	switch (Result) {
	case CM_FEATUREDEVENTS_SETTINGS:
		if (m_Dialog.Show(m_pHomeDisplay->GetHandle())) {
			m_pHomeDisplay->UpdateCurContent();
		}
		break;

	case CM_FEATUREDEVENTS_SHOWEVENTTEXT:
		m_Settings.SetShowEventText(!m_Settings.GetShowEventText());
		m_pHomeDisplay->UpdateCurContent();
		break;

	default:
		if (Result>=CM_FEATUREDEVENTS_SORT_FIRST
				&& Result<=CM_FEATUREDEVENTS_SORT_LAST) {
			CFeaturedEventsSettings::SortType SortType=
				(CFeaturedEventsSettings::SortType)(Result-CM_FEATUREDEVENTS_SORT_FIRST);
			if (SortType!=m_Settings.GetSortType()) {
				m_Settings.SetSortType(SortType);
				SortItems(SortType);
				m_pHomeDisplay->SetScrollPos(0,false);
				m_pHomeDisplay->Invalidate();
			}
			break;
		}
	}

	return true;
}


bool CFeaturedEventsCategory::OnCursorKey(WPARAM KeyCode)
{
	int HotItem;

	if (KeyCode==VK_UP) {
		if (m_HotItem==0 || m_ItemList.empty())
			return false;
		if (m_HotItem<0)
			HotItem=0;
		else
			HotItem=m_HotItem-1;
	} else if (KeyCode==VK_DOWN) {
		if (m_ItemList.empty() || m_HotItem+1==(int)m_ItemList.size())
			return false;
		if (m_HotItem<0)
			HotItem=0;
		else
			HotItem=m_HotItem+1;
	} else {
		return false;
	}

	SetHotItem(HotItem);

	return true;
}


void CFeaturedEventsCategory::Clear()
{
	for (auto it=m_ItemList.begin();it!=m_ItemList.end();++it)
		delete *it;
	m_ItemList.clear();

	m_Height=0;
	m_HotItem=-1;
}


void CFeaturedEventsCategory::SortItems(CFeaturedEventsSettings::SortType SortType)
{
	class CItemCompare
	{
	public:
		CItemCompare(CFeaturedEventsSettings::SortType SortType) : m_SortType(SortType) {}

		bool operator()(const CEventItem *pItem1,const CEventItem *pItem2) const
		{
			int Cmp=0;

			switch (m_SortType) {
			case CFeaturedEventsSettings::SORT_TIME:
				Cmp=CompareTime(pItem1,pItem2);
				if (Cmp==0)
					Cmp=CompareService(pItem1,pItem2);
				break;

			case CFeaturedEventsSettings::SORT_SERVICE:
				Cmp=CompareService(pItem1,pItem2);
				if (Cmp==0)
					Cmp=CompareTime(pItem1,pItem2);
				break;
			}

			return Cmp<0;
		}

	private:
		CFeaturedEventsSettings::SortType m_SortType;

		int CompareTime(const CEventItem *pItem1,const CEventItem *pItem2) const
		{
			int Cmp;

			Cmp=CompareSystemTime(&pItem1->GetEventInfo().m_stStartTime,
								  &pItem2->GetEventInfo().m_stStartTime);
			if (Cmp==0) {
				Cmp=(int)pItem1->GetEventInfo().m_DurationSec-
					(int)pItem2->GetEventInfo().m_DurationSec;
			}
			return Cmp;
		}

		int CompareService(const CEventItem *pItem1,const CEventItem *pItem2) const
		{
			const CChannelInfo &ChannelInfo1=pItem1->GetChannelInfo();
			const CChannelInfo &ChannelInfo2=pItem2->GetChannelInfo();
			NetworkType Network1=GetNetworkType(ChannelInfo1.GetNetworkID());
			NetworkType Network2=GetNetworkType(ChannelInfo2.GetNetworkID());
			int Cmp;
			if (Network1!=Network2) {
				Cmp=(int)Network1-(int)Network2;
			} else {
				int Channel1=ChannelInfo1.GetChannelNo();
				if (Channel1<=0)
					Channel1=ChannelInfo1.GetServiceID();
				int Channel2=ChannelInfo2.GetChannelNo();
				if (Channel2<=0)
					Channel2=ChannelInfo1.GetServiceID();
				Cmp=Channel1-Channel2;
			}
			return Cmp;
		}
	};

	std::sort(m_ItemList.begin(),m_ItemList.end(),CItemCompare(SortType));
}


void CFeaturedEventsCategory::ExpandItem(int Item)
{
	CEventItem *pCurItem=NULL;

	if (Item>=0 && (size_t)Item<m_ItemList.size())
		pCurItem=m_ItemList[Item];

	int ScrollPos=m_pHomeDisplay->GetScrollPos();

	if (pCurItem!=NULL && pCurItem->IsExpanded()) {
		pCurItem->SetExpanded(false);
		m_Height-=pCurItem->GetExpandedHeight()-m_ItemHeight;
	} else {
		for (size_t i=0;i<m_ItemList.size();i++) {
			CEventItem *pItem=m_ItemList[i];
			if (pItem->IsExpanded()) {
				pItem->SetExpanded(false);
				int ExtendedHeight=pItem->GetExpandedHeight()-m_ItemHeight;
				if (pCurItem!=NULL && i<(size_t)Item) {
					ScrollPos-=ExtendedHeight;
				}
				m_Height-=ExtendedHeight;
			}
		}
		if (pCurItem!=NULL) {
			pCurItem->SetExpanded(true);
			m_Height+=pCurItem->GetExpandedHeight()-m_ItemHeight;
		}
	}
	m_pHomeDisplay->OnContentChanged();
	m_pHomeDisplay->SetScrollPos(ScrollPos,false);
}


bool CFeaturedEventsCategory::GetItemRect(size_t Item,RECT *pRect) const
{
	if (Item>=m_ItemList.size())
		return false;

	RECT rc;
	rc.left=m_Rect.left;
	rc.bottom=m_Rect.top-m_pHomeDisplay->GetScrollPos();
	rc.right=m_Rect.right;
	for (size_t i=0;i<=Item;i++) {
		const CEventItem *pItem=m_ItemList[i];
		rc.top=rc.bottom;
		if (pItem->IsExpanded()) {
			rc.bottom+=pItem->GetExpandedHeight();
		} else {
			rc.bottom+=m_ItemHeight;
		}
	}

	*pRect=rc;

	return true;
}


int CFeaturedEventsCategory::GetItemByPosition(int x,int y) const
{
	POINT pt={x,y};

	if (!::PtInRect(&m_Rect,pt))
		return -1;

	int Top,Bottom;
	Bottom=m_Rect.top-m_pHomeDisplay->GetScrollPos();
	for (size_t i=0;i<m_ItemList.size();i++) {
		const CEventItem *pItem=m_ItemList[i];
		Top=Bottom;
		if (pItem->IsExpanded()) {
			Bottom=Top+pItem->GetExpandedHeight();
		} else {
			Bottom=Top+m_ItemHeight;
		}
		if (y>=Top && y<Bottom)
			return (int)i;
	}

	return -1;
}


void CFeaturedEventsCategory::RedrawItem(int Item)
{
	RECT rc;

	if (GetItemRect(Item,&rc))
		m_pHomeDisplay->Invalidate(&rc);
}


bool CFeaturedEventsCategory::SetHotItem(int Item)
{
	int HotItem;

	if (Item>=0) {
		if ((size_t)Item>=m_ItemList.size())
			return false;
		HotItem=Item;
	} else {
		HotItem=-1;
	}

	if (m_HotItem!=HotItem) {
		if (m_HotItem>=0)
			RedrawItem(m_HotItem);
		m_HotItem=HotItem;
		if (HotItem>=0)
			RedrawItem(HotItem);
	}

	return true;
}


CFeaturedEventsCategory::CEventItem::CEventItem(const CChannelInfo &ChannelInfo,const CEventInfoData &EventInfo)
	: m_ChannelInfo(ChannelInfo)
	, m_EventInfo(EventInfo)
	, m_ExpandedHeight(0)
	, m_fExpanded(false)
	, m_hbmLogo(NULL)
{
}


void CFeaturedEventsCategory::CEventItem::SetLogo(HBITMAP hbm)
{
	if (m_hbmLogo!=hbm) {
		m_hbmLogo=hbm;
		m_StretchedLogo.Destroy();
	}
}


HBITMAP CFeaturedEventsCategory::CEventItem::GetStretchedLogo(int Width,int Height) const
{
	if (m_hbmLogo==NULL)
		return NULL;
	// AlphaBlendでリサイズすると汚いので、予めリサイズした画像を作成しておく
	if (m_StretchedLogo.IsCreated()) {
		if (m_StretchedLogo.GetWidth()!=Width || m_StretchedLogo.GetHeight()!=Height)
			m_StretchedLogo.Destroy();
	}
	if (!m_StretchedLogo.IsCreated()) {
		HBITMAP hbm=DrawUtil::ResizeBitmap(m_hbmLogo,Width,Height);
		if (hbm!=NULL)
			m_StretchedLogo.Attach(hbm);
	}
	return m_StretchedLogo.GetHandle();
}


bool CFeaturedEventsCategory::CEventItem::GetEventText(TVTest::String *pText) const
{
	pText->clear();
	if (!IsStringEmpty(m_EventInfo.GetEventText()))
		AppendEventText(pText,m_EventInfo.GetEventText());
	if (!IsStringEmpty(m_EventInfo.GetEventExtText()))
		AppendEventText(pText,m_EventInfo.GetEventExtText());
	return !pText->empty();
}


void CFeaturedEventsCategory::CEventItem::AppendEventText(TVTest::String *pString,LPCWSTR pszText)
{
	bool fFirst=true;
	LPCWSTR p=pszText;

	while (*p!=L'\0') {
		if (*p==L'\r' || *p==L'\n') {
			const WCHAR c=*p++;
			int i=0;
			while (*p==L'\r' || *p==L'\n') {
				if (*p==c)
					i++;
				p++;
			}
			if (*p==L'\0')
				break;
			if (!fFirst) {
				if (i>0)
					pString->append(L"\r\n");
				else
					pString->push_back(L'　');
			}
		} else {
			if (fFirst) {
				if (!pString->empty())
					pString->append(L"\r\n");
				fFirst=false;
			}
			pString->push_back(*p);
			p++;
		}
	}
}




const LPCTSTR CHomeDisplay::m_pszWindowClass=APP_NAME TEXT(" Home Display");
HINSTANCE CHomeDisplay::m_hinst=NULL;


bool CHomeDisplay::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=NULL;
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=m_pszWindowClass;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CHomeDisplay::CHomeDisplay(CEventSearchOptions &EventSearchOptions)
	: m_fAutoFontSize(true)
	, m_ContentHeight(0)
	, m_pEventHandler(NULL)
	, m_CurCategory(0)
	, m_hwndScroll(NULL)
	, m_ScrollPos(0)
	, m_himlIcons(NULL)
{
	GetBackgroundStyle(BACKGROUND_STYLE_CATEGORIES,&m_Style.CategoriesBackGradient);
	GetBackgroundStyle(BACKGROUND_STYLE_CONTENT,&m_Style.ContentBackGradient);
	GetItemStyle(ITEM_STYLE_NORMAL,&m_Style.CategoryItemStyle);
	GetItemStyle(ITEM_STYLE_SELECTED,&m_Style.CategoryItemSelStyle);
	GetItemStyle(ITEM_STYLE_CURRENT,&m_Style.CategoryItemCurStyle);
	GetItemStyle(ITEM_STYLE_NORMAL_1,&m_Style.ItemStyle[0]);
	GetItemStyle(ITEM_STYLE_NORMAL_2,&m_Style.ItemStyle[1]);
	GetItemStyle(ITEM_STYLE_HOT,&m_Style.ItemHotStyle);
	m_Style.BannerTextColor=RGB(255,255,255);

	m_Style.ContentMargins.left=16;
	m_Style.ContentMargins.top=16;
	m_Style.ContentMargins.right=32;
	m_Style.ContentMargins.bottom=16;
	m_Style.ItemMargins.left=4;
	m_Style.ItemMargins.top=2;
	m_Style.ItemMargins.right=4;
	m_Style.ItemMargins.bottom=2;
	m_Style.CategoryItemMargins.left=4;
	m_Style.CategoryItemMargins.top=4;
	m_Style.CategoryItemMargins.right=6;
	m_Style.CategoryItemMargins.bottom=4;
	m_Style.CategoryIconMargin=6;
	m_Style.CategoriesMargins.left=8;
	m_Style.CategoriesMargins.top=16;
	m_Style.CategoriesMargins.right=8;
	m_Style.CategoriesMargins.bottom=16;

	m_CategoryList.reserve(3);
	m_CategoryList.push_back(new CFavoritesCategory(this));
	m_CategoryList.push_back(new CRecentChannelsCategory(this));
	m_CategoryList.push_back(new CFeaturedEventsCategory(this,EventSearchOptions));
}


CHomeDisplay::~CHomeDisplay()
{
	Destroy();
	Clear();
}


bool CHomeDisplay::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,m_pszWindowClass,NULL,m_hinst);
}


bool CHomeDisplay::Close()
{
	if (m_pEventHandler!=NULL) {
		m_pEventHandler->OnClose();
		return true;
	}
	return false;
}


bool CHomeDisplay::IsMessageNeed(const MSG *pMsg) const
{
	if (pMsg->message==WM_KEYDOWN || pMsg->message==WM_KEYUP) {
		switch (pMsg->wParam) {
		case VK_LEFT:
		case VK_RIGHT:
		case VK_UP:
		case VK_DOWN:
		case VK_RETURN:
		case VK_SPACE:
		case VK_PRIOR:
		case VK_NEXT:
		case VK_HOME:
		case VK_END:
		case VK_TAB:
		case VK_ESCAPE:
			return true;
		}
	}
	return false;
}


bool CHomeDisplay::OnMouseWheel(UINT Msg,WPARAM wParam,LPARAM lParam)
{
	if (Msg==WM_MOUSEWHEEL && m_hwnd!=NULL) {
		int Delta=GET_WHEEL_DELTA_WPARAM(wParam)<=0?1:-1;

		SetScrollPos(m_ScrollPos+(Delta*m_Style.FontHeight),true);
		return true;
	}

	return false;
}


bool CHomeDisplay::LoadSettings(CSettings &Settings)
{
	if (Settings.SetSection(TEXT("HomeDisplay"))) {
		int CategoryCount;
		if (Settings.Read(TEXT("CategoryCount"),&CategoryCount)
				&& CategoryCount>0 && (size_t)CategoryCount<=m_CategoryList.size()) {
			decltype(m_CategoryList) Categories;

			Categories.reserve(CategoryCount);
			for (int i=0;i<CategoryCount;i++) {
				TCHAR szKey[32];
				int ID;

				::wsprintf(szKey,TEXT("Category%d_ID"),i);
				if (Settings.Read(szKey,&ID)) {
					size_t j;
					for (j=0;j<Categories.size();j++) {
						if (Categories[j]->GetID()==ID)
							break;
					}
					if (j==Categories.size()) {
						for (j=0;j<m_CategoryList.size();j++) {
							CCategory *pCategory=m_CategoryList[j];

							if (pCategory->GetID()==ID) {
								Categories.push_back(pCategory);
								break;
							}
						}
					}
				}
			}
			if (Categories.size()<m_CategoryList.size()) {
				for (size_t i=0;i<m_CategoryList.size();i++) {
					CCategory *pCategory=m_CategoryList[i];
					size_t j;
					for (j=0;j<Categories.size();j++) {
						if (Categories[j]->GetID()==pCategory->GetID())
							break;
					}
					if (j==Categories.size())
						Categories.push_back(pCategory);
				}
			}
			m_CategoryList=std::move(Categories);
		}

		int CurCategory;
		if (Settings.Read(TEXT("CurCategory"),&CurCategory)
				&& CurCategory>=0 && (size_t)CurCategory<m_CategoryList.size())
			m_CurCategory=CurCategory;
	}

	for (auto itr=m_CategoryList.begin();itr!=m_CategoryList.end();++itr)
		(*itr)->ReadSettings(Settings);

	return true;
}


bool CHomeDisplay::SaveSettings(CSettings &Settings)
{
	if (Settings.SetSection(TEXT("HomeDisplay"))) {
		if (Settings.Write(TEXT("CategoryCount"),(int)m_CategoryList.size())) {
			for (int i=0;i<(int)m_CategoryList.size();i++) {
				const CCategory *pCategory=m_CategoryList[i];
				TCHAR szKey[32];

				::wsprintf(szKey,TEXT("Category%d_ID"),i);
				Settings.Write(szKey,pCategory->GetID());
			}
		}

		Settings.Write(TEXT("CurCategory"),m_CurCategory);
	}

	for (auto itr=m_CategoryList.begin();itr!=m_CategoryList.end();++itr)
		(*itr)->WriteSettings(Settings);

	return true;
}


void CHomeDisplay::Clear()
{
	for (size_t i=0;i<m_CategoryList.size();i++)
		delete m_CategoryList[i];
	m_CategoryList.clear();
}


bool CHomeDisplay::UpdateContents()
{
	for (size_t i=0;i<m_CategoryList.size();i++)
		m_CategoryList[i]->Create();

	if (m_hwnd!=NULL) {
		LayOut();
		Invalidate();
	}

	return true;
}


void CHomeDisplay::SetEventHandler(CEventHandler *pEventHandler)
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pHomeDisplay=NULL;
	if (pEventHandler!=NULL)
		pEventHandler->m_pHomeDisplay=this;
	m_pEventHandler=pEventHandler;
}


bool CHomeDisplay::SetFont(const LOGFONT *pFont,bool fAutoSize)
{
	if (!m_Font.Create(pFont))
		return false;
	m_fAutoFontSize=fAutoSize;
	if (m_hwnd!=NULL) {
		LayOut();
		Invalidate();
	}
	return true;
}


bool CHomeDisplay::SetScrollPos(int Pos,bool fScroll)
{
	if (m_hwnd==NULL)
		return false;

	RECT rcContent;
	GetContentAreaRect(&rcContent);

	if (Pos<0 || m_ContentHeight<=rcContent.bottom-rcContent.top) {
		Pos=0;
	} else if (Pos>m_ContentHeight-(rcContent.bottom-rcContent.top)) {
		Pos=m_ContentHeight-(rcContent.bottom-rcContent.top);
	}

	if (Pos!=m_ScrollPos) {
		SCROLLINFO si;

		si.cbSize=sizeof(si);
		si.fMask=SIF_POS;
		si.nPos=Pos;
		::SetScrollInfo(m_hwndScroll,SB_CTL,&si,TRUE);
		if (fScroll) {
			Update();
			::ScrollWindowEx(m_hwnd,0,m_ScrollPos-Pos,
							 &rcContent,&rcContent,NULL,NULL,SW_INVALIDATE);
		}
		m_ScrollPos=Pos;
	}

	return true;
}


bool CHomeDisplay::SetCurCategory(int Category)
{
	if (Category!=m_CurCategory) {
		if (Category<0 || (size_t)Category>=m_CategoryList.size())
			return false;

		if (m_CurCategory>=0)
			m_CategoryList[m_CurCategory]->SetFocus(false);

		m_CurCategory=Category;

		m_ScrollPos=0;
		if (m_hwnd!=NULL) {
			LayOut();
			Invalidate();
		}
	}

	return true;
}


bool CHomeDisplay::UpdateCurContent()
{
	if (m_CurCategory<0)
		return false;

	m_CategoryList[m_CurCategory]->Create();

	if (m_hwnd!=NULL) {
		m_ScrollPos=0;
		LayOut();
		RECT rc;
		GetContentAreaRect(&rc);
		Invalidate(&rc);
	}

	return true;
}


bool CHomeDisplay::OnContentChanged()
{
	if (m_CurCategory<0)
		return false;

	m_ContentHeight=m_CategoryList[m_CurCategory]->GetHeight();

	if (m_hwnd!=NULL) {
		SetScrollBar();
		RECT rc;
		GetContentAreaRect(&rc);
		Invalidate(&rc);
	}

	return true;
}


LRESULT CHomeDisplay::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		if (!m_Font.IsCreated())
			m_Font.Create(DrawUtil::FONT_MESSAGE);
		m_hwndScroll=::CreateWindowEx(0,TEXT("SCROLLBAR"),TEXT(""),
			WS_CHILD | SBS_VERT,0,0,0,0,hwnd,NULL,m_hinst,NULL);
		m_ScrollPos=0;
		m_LButtonDownPos.x=-1;
		m_LButtonDownPos.y=-1;
		m_fHitCloseButton=false;
		m_CursorPart=PART_MARGIN;
		m_himlIcons=::ImageList_LoadImage(m_hinst,MAKEINTRESOURCE(IDB_HOME),
										  CATEGORY_ICON_WIDTH,1,0,IMAGE_BITMAP,LR_CREATEDIBSECTION);
		return 0;

	case WM_SIZE:
		LayOut();
		return 0;

	case WM_PAINT:
#if 0
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd,&ps);
			Draw(ps.hdc,ps.rcPaint);
			::EndPaint(hwnd,&ps);
		}
#else
		OnPaint(hwnd);
#endif
		return 0;

	case WM_LBUTTONDOWN:
		{
			POINT pt={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};

			::SetFocus(hwnd);
			m_LButtonDownPos=pt;
			m_fHitCloseButton=CloseButtonHitTest(pt.x,pt.y);
		}
		// Fall through
	case WM_MOUSEMOVE:
		{
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			int CategoryIndex=-1;
			PartType Part=HitTest(x,y,&CategoryIndex);

			m_CursorPart=Part;
			if (Part==PART_CONTENT) {
				if (m_CurCategory>=0) {
					CCategory *pCategory=m_CategoryList[m_CurCategory];
					bool fFocused=pCategory->IsFocused();
					pCategory->OnCursorMove(x,y);
					if (pCategory->IsFocused()!=fFocused)
						RedrawCategoryItem(m_CurCategory);
				}
			} else {
				/*
				if (m_CurCategory>=0) {
					CCategory *pCategory=m_CategoryList[m_CurCategory];
					bool fFocused=pCategory->IsFocused();
					pCategory->OnCursorLeave();
					if (pCategory->IsFocused()!=fFocused)
						RedrawCategoryItem(m_CurCategory);
				}
				*/
				if (Part==PART_CATEGORY) {
					if (m_CurCategory!=CategoryIndex) {
						SetCurCategory(CategoryIndex);
					}
				}
			}
		}
		return 0;

	case WM_LBUTTONUP:
		{
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);

			if (m_fHitCloseButton) {
				if (CloseButtonHitTest(x,y)) {
					Close();
				}
			} else if (m_LButtonDownPos.x==x
					&& m_LButtonDownPos.y==y) {
				int Category;
				PartType Part=HitTest(x,y,&Category);

				switch (Part) {
				case PART_CATEGORY:
					SetCurCategory(Category);
					break;

				case PART_CONTENT:
					if (m_CategoryList[m_CurCategory]->OnClick(x,y)) {
						Close();
					}
					break;
				}
			}
		}
		return 0;

	case WM_RBUTTONDOWN:
		{
			POINT pt={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
			RECT rc;

			GetContentAreaRect(&rc);
			if (m_CurCategory<0
					|| !::PtInRect(&rc,pt)
					|| !m_CategoryList[m_CurCategory]->OnRButtonDown(pt.x,pt.y)) {
				if (m_pEventHandler!=NULL) {
					m_pEventHandler->OnRButtonDown(pt.x,pt.y);
				}
			}
		}
		return 0;

	case WM_LBUTTONDBLCLK:
		if (m_pEventHandler!=NULL) {
			m_pEventHandler->OnLButtonDoubleClick(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		}
		return 0;

	case WM_SETCURSOR:
		if (reinterpret_cast<HWND>(wParam)==hwnd
				&& LOWORD(lParam)==HTCLIENT) {
			if (m_CursorPart==PART_CONTENT && m_CurCategory>=0) {
				if (m_CategoryList[m_CurCategory]->OnSetCursor())
					return TRUE;
			}
			::SetCursor(::LoadCursor(NULL,IDC_ARROW));
			return TRUE;
		}
		break;

	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		if (OnMouseWheel(uMsg,wParam,lParam))
			return 0;
		break;

	case WM_VSCROLL:
		{
			int Pos=m_ScrollPos;
			RECT rcContent;
			GetContentAreaRect(&rcContent);
			int PageSize=rcContent.bottom-rcContent.top;

			switch (LOWORD(wParam)) {
			case SB_LINEUP:		Pos--;									break;
			case SB_LINEDOWN:	Pos++;									break;
			case SB_PAGEUP:		Pos-=PageSize;							break;
			case SB_PAGEDOWN:	Pos+=PageSize;							break;
			case SB_TOP:		Pos=0;									break;
			case SB_BOTTOM:		Pos=max(m_ContentHeight-PageSize,0);	break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:	Pos=HIWORD(wParam);						break;
			default:	return 0;
			}
			SetScrollPos(Pos,true);
		}
		return 0;

	case WM_KEYDOWN:
		switch (wParam) {
		case VK_LEFT:
			if (m_CurCategory>=0 && m_CategoryList[m_CurCategory]->IsFocused()) {
				m_CategoryList[m_CurCategory]->SetFocus(false);
				RedrawCategoryItem(m_CurCategory);
			}
			break;

		case VK_RIGHT:
			if (m_CurCategory>=0 && !m_CategoryList[m_CurCategory]->IsFocused()) {
				if (m_CategoryList[m_CurCategory]->SetFocus(true))
					RedrawCategoryItem(m_CurCategory);
			}
			break;

		case VK_UP:
		case VK_DOWN:
			{
				CCategory *pCategory=m_CategoryList[m_CurCategory];

				if (pCategory->IsFocused()) {
					if (pCategory->OnCursorKey(wParam)) {
						ScrollToCurItem();
					}
				} else {
					int Category=m_CurCategory;

					if (wParam==VK_UP) {
						if (Category<1)
							break;
						Category--;
					} else {
						Category++;
					}
					SetCurCategory(Category);
				}
			}
			break;

		case VK_RETURN:
		case VK_SPACE:
			{
				CCategory *pCategory=m_CategoryList[m_CurCategory];

				if (pCategory->IsFocused()) {
					if (pCategory->OnDecide()) {
						Close();
					}
				}
			}
			break;

		case VK_PRIOR:
		case VK_NEXT:
		case VK_HOME:
		case VK_END:
			{
				static const struct {
					WORD KeyCode;
					WORD Scroll;
				} KeyMap[] = {
					{VK_PRIOR,	SB_PAGEUP},
					{VK_NEXT,	SB_PAGEDOWN},
					{VK_HOME,	SB_TOP},
					{VK_END,	SB_BOTTOM},
				};
				int i;
				for (i=0;KeyMap[i].KeyCode!=wParam;i++);
				::SendMessage(hwnd,WM_VSCROLL,KeyMap[i].Scroll,reinterpret_cast<LPARAM>(m_hwndScroll));
			}
			break;

		case VK_TAB:
			{
				int Category=m_CurCategory;

				if (::GetKeyState(VK_SHIFT)<0) {
					if (Category<1)
						break;
					Category--;
				} else {
					Category++;
				}
				SetCurCategory(Category);
			}
			break;

		case VK_ESCAPE:
			Close();
			break;
		}
		return 0;

	case WM_DESTROY:
		if (m_himlIcons!=NULL) {
			::ImageList_Destroy(m_himlIcons);
			m_himlIcons=NULL;
		}
		return 0;
	}

	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


void CHomeDisplay::Draw(HDC hdc,const RECT &PaintRect)
{
	RECT rcClient;
	GetClientRect(&rcClient);

	int OldBkMode=::SetBkMode(hdc,TRANSPARENT);
	COLORREF OldTextColor=::GetTextColor(hdc);
	HFONT hfontOld=DrawUtil::SelectObject(hdc,m_Font);

	const CCategory *pCategory=NULL;
	if (m_CurCategory>=0)
		pCategory=m_CategoryList[m_CurCategory];

	if (PaintRect.left<m_CategoriesAreaWidth) {
		RECT rcCategories=rcClient;
		rcCategories.right=m_CategoriesAreaWidth;
		Theme::FillGradient(hdc,&rcCategories,&m_Style.CategoriesBackGradient);

		RECT rcItem;
		rcItem.left=rcCategories.left+m_Style.CategoriesMargins.left;
		rcItem.right=rcItem.left+m_CategoryItemWidth;
		rcItem.top=rcCategories.top+m_Style.CategoriesMargins.top;

		for (size_t i=0;i<m_CategoryList.size();i++) {
			const CCategory *pCategory=m_CategoryList[i];
			const Theme::Style *pStyle=
				((int)i==m_CurCategory)?
					(pCategory->IsFocused()?&m_Style.CategoryItemSelStyle:&m_Style.CategoryItemCurStyle):
					(&m_Style.CategoryItemStyle);

			rcItem.bottom=rcItem.top+m_CategoryItemHeight;
			Theme::DrawStyleBackground(hdc,&rcItem,pStyle);
			RECT rc=rcItem;
			rc.left+=m_Style.CategoryItemMargins.left;
			rc.top+=m_Style.CategoryItemMargins.top;
			rc.right-=m_Style.CategoryItemMargins.right;
			rc.bottom-=m_Style.CategoryItemMargins.bottom;
			::ImageList_Draw(m_himlIcons,pCategory->GetIconIndex(),
							 hdc,rc.left,rc.top+((rc.bottom-rc.top)-CATEGORY_ICON_HEIGHT)/2,ILD_TRANSPARENT);
			rc.left+=CATEGORY_ICON_WIDTH+m_Style.CategoryIconMargin;
			::SetTextColor(hdc,pStyle->TextColor);
			::DrawText(hdc,pCategory->GetTitle(),-1,&rc,DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
			rcItem.top=rcItem.bottom;
		}
	}

	if (PaintRect.right>m_CategoriesAreaWidth) {
		RECT rcContent=rcClient;
		rcContent.left=m_CategoriesAreaWidth;
		Theme::FillGradient(hdc,&rcContent,&m_Style.ContentBackGradient);

		if (pCategory!=NULL) {
			rcContent.left+=m_Style.ContentMargins.left;
			rcContent.top+=m_Style.ContentMargins.top;
			rcContent.right-=m_Style.ContentMargins.right;
			rcContent.bottom-=m_Style.ContentMargins.bottom;

			if (rcContent.left<rcContent.right) {
				HRGN hrgn=::CreateRectRgnIndirect(&rcContent);
				::SelectClipRgn(hdc,hrgn);

				::OffsetRect(&rcContent,0,-m_ScrollPos);
				pCategory->Draw(hdc,m_Style,rcContent,PaintRect);

				::SelectClipRgn(hdc,NULL);
				::DeleteObject(hrgn);
			}
		}
	}

	::SelectObject(hdc,hfontOld);
	::SetTextColor(hdc,OldTextColor);
	::SetBkMode(hdc,OldBkMode);

	DrawCloseButton(hdc);
}


void CHomeDisplay::LayOut()
{
	RECT rcClient;
	GetClientRect(&rcClient);

	if (m_fAutoFontSize) {
		LOGFONT lf;
		m_Font.GetLogFont(&lf);
		lf.lfHeight=GetDefaultFontSize(rcClient.right,rcClient.bottom);
		lf.lfWidth=0;
		m_Font.Create(&lf);
	}

	HDC hdc=::GetDC(m_hwnd);
	HFONT hfontOld=DrawUtil::SelectObject(hdc,m_Font);

	m_Style.FontHeight=m_Font.GetHeight(hdc);

	int CategoryTextWidth=0;
	for (size_t i=0;i<m_CategoryList.size();i++) {
		RECT rc={0,0,0,0};
		::DrawText(hdc,m_CategoryList[i]->GetTitle(),-1,&rc,DT_CALCRECT);
		if (rc.right>CategoryTextWidth)
			CategoryTextWidth=rc.right;
	}
	m_CategoryItemWidth=CategoryTextWidth+CATEGORY_ICON_WIDTH+m_Style.CategoryIconMargin+
		m_Style.CategoryItemMargins.left+m_Style.CategoryItemMargins.right;
	m_CategoryItemHeight=max(m_Style.FontHeight,CATEGORY_ICON_HEIGHT)+
		m_Style.CategoryItemMargins.top+m_Style.CategoryItemMargins.bottom;
	m_CategoriesAreaWidth=m_CategoryItemWidth+
		m_Style.CategoriesMargins.left+m_Style.CategoriesMargins.right;

	RECT rcContent=rcClient;
	rcContent.left+=m_CategoriesAreaWidth+m_Style.ContentMargins.left;
	rcContent.top+=m_Style.ContentMargins.top;
	rcContent.right-=m_Style.ContentMargins.right;
	rcContent.bottom-=m_Style.ContentMargins.bottom;

	CCategory *pCategory=m_CategoryList[m_CurCategory];
	pCategory->LayOut(m_Style,hdc,rcContent);
	m_ContentHeight=pCategory->GetHeight();

	::SelectObject(hdc,hfontOld);
	::ReleaseDC(m_hwnd,hdc);

	SetScrollBar();
}


void CHomeDisplay::SetScrollBar()
{
	RECT rcContent;
	GetContentAreaRect(&rcContent);
	int ContentAreaHeight=rcContent.bottom-rcContent.top;

	if (ContentAreaHeight<m_ContentHeight) {
		SCROLLINFO si;

		if (m_ScrollPos>m_ContentHeight-ContentAreaHeight)
			m_ScrollPos=m_ContentHeight-ContentAreaHeight;
		si.cbSize=sizeof(si);
		si.fMask=SIF_PAGE | SIF_POS | SIF_RANGE;
		si.nMin=0;
		si.nMax=m_ContentHeight-1;
		si.nPage=ContentAreaHeight;
		si.nPos=m_ScrollPos;
		::SetScrollInfo(m_hwndScroll,SB_CTL,&si,TRUE);
		int ScrollWidth=::GetSystemMetrics(SM_CXVSCROLL);
		::MoveWindow(m_hwndScroll,
					 rcContent.right,rcContent.top,
					 ScrollWidth,rcContent.bottom-rcContent.top,TRUE);
		::ShowWindow(m_hwndScroll,SW_SHOW);
	} else {
		m_ScrollPos=0;
		::ShowWindow(m_hwndScroll,SW_HIDE);
	}
}


void CHomeDisplay::GetContentAreaRect(RECT *pRect) const
{
	GetClientRect(pRect);
	pRect->left+=m_CategoriesAreaWidth+m_Style.ContentMargins.left;
	pRect->top+=m_Style.ContentMargins.top;
	pRect->right-=m_Style.ContentMargins.right;
	pRect->bottom-=m_Style.ContentMargins.bottom;
	if (pRect->right<pRect->left)
		pRect->right=pRect->left;
	if (pRect->bottom<pRect->top)
		pRect->bottom=pRect->top;
}


CHomeDisplay::PartType CHomeDisplay::HitTest(int x,int y,int *pCategoryIndex) const
{
	POINT pt={x,y};

	RECT rcCategories;
	rcCategories.left=m_Style.CategoriesMargins.left;
	rcCategories.top=m_Style.CategoriesMargins.top;
	rcCategories.right=rcCategories.left+m_CategoriesAreaWidth;
	rcCategories.bottom=rcCategories.top+m_CategoryItemHeight*(int)m_CategoryList.size();
	if (::PtInRect(&rcCategories,pt)) {
		if (pCategoryIndex!=NULL)
			*pCategoryIndex=(pt.y-rcCategories.top)/m_CategoryItemHeight;
		return PART_CATEGORY;
	}

	RECT rcContent;
	GetContentAreaRect(&rcContent);
	if (rcContent.bottom-rcContent.top>m_ContentHeight)
		rcContent.bottom=rcContent.top+m_ContentHeight;
	if (::PtInRect(&rcContent,pt)) {
		return PART_CONTENT;
	}

	return PART_MARGIN;
}


void CHomeDisplay::ScrollToCurItem()
{
	if (m_CurCategory>=0) {
		RECT rcItem;

		if (m_CategoryList[m_CurCategory]->GetCurItemRect(&rcItem)) {
			RECT rcContent;
			int Pos;

			GetContentAreaRect(&rcContent);
			if (rcItem.top<rcContent.top) {
				Pos=rcItem.top-rcContent.top+m_ScrollPos;
			} else if (rcItem.bottom>rcContent.bottom) {
				Pos=(rcItem.bottom-rcContent.top+m_ScrollPos)-
					(rcContent.bottom-rcContent.top);
			} else {
				return;
			}
			SetScrollPos(Pos,true);
		}
	}
}


bool CHomeDisplay::GetCategoryItemRect(int Category,RECT *pRect) const
{
	if (Category<0 || (size_t)Category>=m_CategoryList.size())
		return false;

	pRect->left=m_Style.CategoriesMargins.left;
	pRect->top=m_Style.CategoriesMargins.top+(Category*m_CategoryItemHeight);
	pRect->right=pRect->left+m_CategoryItemWidth;
	pRect->bottom=pRect->top+m_CategoryItemHeight;

	return true;
}


bool CHomeDisplay::RedrawCategoryItem(int Category)
{
	RECT rc;

	if (!GetCategoryItemRect(Category,&rc))
		return false;
	Invalidate(&rc);
	return true;
}




CHomeDisplay::CEventHandler::CEventHandler()
	: m_pHomeDisplay(NULL)
{
}


CHomeDisplay::CEventHandler::~CEventHandler()
{
}




CHomeDisplay::CCategory::CCategory(CHomeDisplay *pHomeDisplay)
	: m_pHomeDisplay(pHomeDisplay)
{
}
