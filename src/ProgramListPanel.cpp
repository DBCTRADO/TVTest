#include "stdafx.h"
#include "TVTest.h"
#include "ProgramListPanel.h"
#include "AppMain.h"
#include "LogoManager.h"
#include "DrawUtil.h"
#include "TextDraw.h"
#include "resource.h"
#include "Common/DebugDef.h"




class CProgramItemInfo
{
public:
	CProgramItemInfo(const CEventInfoData &EventInfo);
	//~CProgramItemInfo();
	const CEventInfoData &GetEventInfo() const { return m_EventInfo; }
	WORD GetEventID() const { return m_EventID; }
	int GetTitleLines() const { return m_NameLines; }
	int GetTextLines() const { return m_TextLines; }
	int GetLines() const { return m_NameLines+m_TextLines; }
	int CalcTitleLines(TVTest::CTextDraw &DrawText,int Width);
	int CalcTextLines(TVTest::CTextDraw &DrawText,int Width);
	void DrawTitle(TVTest::CTextDraw &DrawText,const RECT &Rect,int LineHeight);
	void DrawText(TVTest::CTextDraw &DrawText,const RECT &Rect,int LineHeight);
	SIZE GetTimeSize(HDC hdc) const;
	bool IsChanged(const CProgramItemInfo *pItem) const;

private:
	enum {
		MAX_EVENT_TITLE = 256
	};

	CEventInfoData m_EventInfo;
	WORD m_EventID;
	int m_NameLines;
	int m_TextLines;

	LPCTSTR GetEventText() const;
	void GetEventTitleText(LPTSTR pszText,int MaxLength) const;
	void GetEventTimeText(LPTSTR pszText,int MaxLength) const;
};


CProgramItemInfo::CProgramItemInfo(const CEventInfoData &EventInfo)
{
	m_EventInfo=EventInfo;
	m_EventID=EventInfo.m_EventID;
	m_NameLines=0;
	m_TextLines=0;
}


int CProgramItemInfo::CalcTitleLines(TVTest::CTextDraw &DrawText,int Width)
{
	TCHAR szText[MAX_EVENT_TITLE];

	GetEventTitleText(szText,lengthof(szText));
	m_NameLines=DrawText.CalcLineCount(szText,Width);
	return m_NameLines;
}


int CProgramItemInfo::CalcTextLines(TVTest::CTextDraw &DrawText,int Width)
{
	LPCTSTR pszEventText=GetEventText();

	if (pszEventText!=NULL)
		m_TextLines=DrawText.CalcLineCount(pszEventText,Width);
	else
		m_TextLines=0;
	return m_TextLines;
}


void CProgramItemInfo::DrawTitle(TVTest::CTextDraw &DrawText,const RECT &Rect,int LineHeight)
{
	TCHAR szText[MAX_EVENT_TITLE];

	GetEventTitleText(szText,lengthof(szText));
	DrawText.Draw(szText,Rect,LineHeight);
}


void CProgramItemInfo::DrawText(TVTest::CTextDraw &DrawText,const RECT &Rect,int LineHeight)
{
	LPCTSTR pszEventText=GetEventText();
	if (pszEventText!=NULL) {
		DrawText.Draw(pszEventText,Rect,LineHeight);
	}
}


SIZE CProgramItemInfo::GetTimeSize(HDC hdc) const
{
	TCHAR szTime[EpgUtil::MAX_EVENT_TIME_LENGTH];
	SIZE sz;

	GetEventTimeText(szTime,lengthof(szTime));
	::GetTextExtentPoint32(hdc,szTime,::lstrlen(szTime),&sz);
	return sz;
}


bool CProgramItemInfo::IsChanged(const CProgramItemInfo *pItem) const
{
	return m_EventID!=pItem->m_EventID
		|| CompareSystemTime(&m_EventInfo.m_StartTime,&pItem->m_EventInfo.m_StartTime)!=0
		|| m_EventInfo.m_Duration!=pItem->m_EventInfo.m_Duration;
}


LPCTSTR CProgramItemInfo::GetEventText() const
{
	return EpgUtil::GetEventDisplayText(m_EventInfo);
}


void CProgramItemInfo::GetEventTitleText(LPTSTR pszText,int MaxLength) const
{
	TCHAR szTime[EpgUtil::MAX_EVENT_TIME_LENGTH];

	GetEventTimeText(szTime,lengthof(szTime));
	StdUtil::snprintf(pszText,MaxLength,TEXT("%s %s"),
					  szTime,m_EventInfo.m_EventName.c_str());
}


void CProgramItemInfo::GetEventTimeText(LPTSTR pszText,int MaxLength) const
{
	EpgUtil::FormatEventTime(&m_EventInfo,pszText,MaxLength,
							 EpgUtil::EVENT_TIME_HOUR_2DIGITS);
}




CProgramItemList::CProgramItemList()
	: m_NumItems(0)
	, m_ppItemList(NULL)
	, m_ItemListLength(0)
{
}


CProgramItemList::~CProgramItemList()
{
	Clear();
}


CProgramItemInfo *CProgramItemList::GetItem(int Index)
{
	if (Index<0 || Index>=m_NumItems)
		return NULL;
	return m_ppItemList[Index];
}


const CProgramItemInfo *CProgramItemList::GetItem(int Index) const
{
	if (Index<0 || Index>=m_NumItems)
		return NULL;
	return m_ppItemList[Index];
}


bool CProgramItemList::Add(CProgramItemInfo *pItem)
{
	if (m_NumItems==m_ItemListLength)
		return false;
	m_ppItemList[m_NumItems++]=pItem;
	return true;
}


void CProgramItemList::Clear()
{
	if (m_ppItemList!=NULL) {
		int i;

		for (i=0;i<m_NumItems;i++)
			delete m_ppItemList[i];
		delete [] m_ppItemList;
		m_ppItemList=NULL;
		m_NumItems=0;
		m_ItemListLength=0;
	}
}


void CProgramItemList::Reserve(int NumItems)
{
	Clear();
	m_ppItemList=new CProgramItemInfo*[NumItems];
	m_ItemListLength=NumItems;
}


void CProgramItemList::Attach(CProgramItemList *pList)
{
	Clear();
	m_NumItems=pList->m_NumItems;
	m_ppItemList=pList->m_ppItemList;
	m_ItemListLength=pList->m_ItemListLength;
	pList->m_NumItems=0;
	pList->m_ppItemList=NULL;
	pList->m_ItemListLength=0;
}




const LPCTSTR CProgramListPanel::m_pszClassName=APP_NAME TEXT(" Program List Panel");
HINSTANCE CProgramListPanel::m_hinst=NULL;


bool CProgramListPanel::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=NULL;
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=m_pszClassName;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CProgramListPanel::CProgramListPanel()
	: m_EventInfoPopupManager(&m_EventInfoPopup)
	, m_EventInfoPopupHandler(this)
	, m_pProgramList(NULL)
	, m_FontHeight(0)
	, m_fMouseOverEventInfo(true)
	, m_fUseEpgColorScheme(false)
	, m_fShowFeaturedMark(true)
	, m_VisibleEventIcons(((1<<(CEpgIcons::ICON_LAST+1))-1)^CEpgIcons::IconFlag(CEpgIcons::ICON_PAY))
	, m_ChannelHeight(0)
	, m_CurEventID(-1)
	, m_ScrollPos(0)
	//, m_hwndToolTip(NULL)
	, m_fShowRetrievingMessage(false)
{
}


CProgramListPanel::~CProgramListPanel()
{
	Destroy();
}


bool CProgramListPanel::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 m_pszClassName,TEXT("番組表"),m_hinst);
}


void CProgramListPanel::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	m_Style.SetStyle(pStyleManager);
}


void CProgramListPanel::NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	m_Style.NormalizeStyle(pStyleManager);
}


void CProgramListPanel::SetTheme(const TVTest::Theme::CThemeManager *pThemeManager)
{
	ProgramListPanelTheme Theme;

	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_PROGRAMLISTPANEL_CHANNEL,
							&Theme.ChannelNameStyle);
	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_PROGRAMLISTPANEL_CURCHANNEL,
							&Theme.CurChannelNameStyle);
	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_PROGRAMLISTPANEL_CHANNELBUTTON,
							&Theme.ChannelButtonStyle);
	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_PROGRAMLISTPANEL_CHANNELBUTTON_HOT,
							&Theme.ChannelButtonHotStyle);
	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_PROGRAMLISTPANEL_EVENT,
							&Theme.EventTextStyle);
	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_PROGRAMLISTPANEL_CUREVENT,
							&Theme.CurEventTextStyle);
	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_PROGRAMLISTPANEL_TITLE,
							&Theme.EventNameStyle);
	pThemeManager->GetStyle(TVTest::Theme::CThemeManager::STYLE_PROGRAMLISTPANEL_CURTITLE,
							&Theme.CurEventNameStyle);
	Theme.MarginColor=
		pThemeManager->GetColor(CColorScheme::COLOR_PANELBACK);
	pThemeManager->GetBackgroundStyle(TVTest::Theme::CThemeManager::STYLE_PROGRAMGUIDE_FEATUREDMARK,
									  &Theme.FeaturedMarkStyle);

	SetProgramListPanelTheme(Theme);

	m_EpgTheme.SetTheme(pThemeManager);
}


bool CProgramListPanel::ReadSettings(CSettings &Settings)
{
	Settings.Read(TEXT("ProgramListPanel.MouseOverEventInfo"),&m_fMouseOverEventInfo);
	Settings.Read(TEXT("ProgramListPanel.UseEpgColorScheme"),&m_fUseEpgColorScheme);
	Settings.Read(TEXT("ProgramListPanel.ShowFeaturedMark"),&m_fShowFeaturedMark);

	return true;
}


bool CProgramListPanel::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("ProgramListPanel.MouseOverEventInfo"),m_fMouseOverEventInfo);
	Settings.Write(TEXT("ProgramListPanel.UseEpgColorScheme"),m_fUseEpgColorScheme);
	Settings.Write(TEXT("ProgramListPanel.ShowFeaturedMark"),m_fShowFeaturedMark);

	return true;
}


bool CProgramListPanel::UpdateProgramList(const CChannelInfo *pChannelInfo)
{
	if (m_pProgramList==NULL || pChannelInfo==NULL)
		return false;
	if (m_hwnd!=NULL
			&& m_SelectedChannel.GetNetworkID()==pChannelInfo->GetNetworkID()
			&& m_SelectedChannel.GetTransportStreamID()==pChannelInfo->GetTransportStreamID()
			&& m_SelectedChannel.GetServiceID()==pChannelInfo->GetServiceID()) {
		const bool fRetrieving=m_fShowRetrievingMessage;

		m_fShowRetrievingMessage=false;
		if (UpdateListInfo(pChannelInfo)) {
			CalcDimensions();
			SetScrollBar();
			//SetToolTip();
			RECT rc;
			GetProgramListRect(&rc);
			Invalidate(&rc);
		} else if (fRetrieving) {
			Invalidate();
		}
	}
	return true;
}


bool CProgramListPanel::UpdateListInfo(const CChannelInfo *pChannelInfo)
{
	if (m_pProgramList==NULL || pChannelInfo==NULL)
		return false;

	const CEpgServiceInfo *pServiceInfo=m_pProgramList->GetServiceInfo(
		pChannelInfo->GetNetworkID(),
		pChannelInfo->GetTransportStreamID(),
		pChannelInfo->GetServiceID());
	int NumEvents=0;
	if (pServiceInfo!=NULL)
		NumEvents=(int)pServiceInfo->m_EventList.EventDataMap.size();
	if (NumEvents==0) {
		if (m_ItemList.NumItems()>0) {
			m_ItemList.Clear();
			return true;
		}
		return false;
	}
	CProgramItemList NewItemList;
	NewItemList.Reserve(NumEvents);

	const CEventInfoList &EventList=pServiceInfo->m_EventList;
	SYSTEMTIME stFirst;
	GetCurrentEpgTime(&stFirst);
	stFirst.wSecond=0;
	stFirst.wMilliseconds=0;
	CEventManager::TimeEventInfo Key(stFirst);
	auto itrTime=EventList.EventTimeMap.lower_bound(Key);
	if (itrTime!=EventList.EventTimeMap.begin()) {
		--itrTime;
		if (itrTime->StartTime+itrTime->Duration>Key.StartTime) {
			auto itrEvent=EventList.EventDataMap.find(itrTime->EventID);
			if (itrEvent!=EventList.EventDataMap.end())
				NewItemList.Add(new CProgramItemInfo(itrEvent->second));
		}
		++itrTime;
	}
	Key.StartTime+=24*60*60;
	for (;itrTime!=EventList.EventTimeMap.end();++itrTime) {
		if (itrTime->StartTime>=Key.StartTime)
			break;
		auto itrEvent=EventList.EventDataMap.find(itrTime->EventID);
		if (itrEvent!=EventList.EventDataMap.end())
			NewItemList.Add(new CProgramItemInfo(itrEvent->second));
	}

	bool fChanged;
	if (NewItemList.NumItems()!=m_ItemList.NumItems()) {
		fChanged=true;
	} else {
		fChanged=false;
		for (int i=0;i<m_ItemList.NumItems();i++) {
			if (m_ItemList.GetItem(i)->IsChanged(NewItemList.GetItem(i))) {
				fChanged=true;
				break;
			}
		}
	}
	if (!fChanged)
		return false;
	m_ItemList.Attach(&NewItemList);

	return true;
}


void CProgramListPanel::ClearProgramList()
{
	if (m_ItemList.NumItems()>0) {
		m_ItemList.Clear();
		m_ScrollPos=0;
		m_TotalLines=0;
		if (m_hwnd!=NULL) {
			SetScrollBar();
			//SetToolTip();
			Invalidate();
		}
	}
}


void CProgramListPanel::SelectChannel(const CChannelInfo *pChannelInfo,bool fUpdate)
{
	ClearProgramList();
	if (pChannelInfo!=NULL) {
		m_SelectedChannel=*pChannelInfo;
		if (fUpdate)
			UpdateProgramList(pChannelInfo);
	} else {
		m_SelectedChannel=CChannelInfo();
	}
}


void CProgramListPanel::SetCurrentChannel(const CChannelInfo *pChannelInfo)
{
	if (pChannelInfo!=NULL)
		m_CurChannel=*pChannelInfo;
	else
		m_CurChannel=CChannelInfo();
	m_CurEventID=-1;
	Invalidate();
}


void CProgramListPanel::SetCurrentEventID(int EventID)
{
	m_CurEventID=EventID;
	if (m_hwnd!=NULL)
		Invalidate();
}


void CProgramListPanel::GetHeaderRect(RECT *pRect) const
{
	GetClientRect(pRect);
	pRect->bottom=m_ChannelHeight;
}


void CProgramListPanel::GetChannelButtonRect(RECT *pRect) const
{
	GetHeaderRect(pRect);
	TVTest::Style::Subtract(pRect,m_Style.ChannelPadding);
	int Width=m_Style.ChannelButtonIconSize.Width+m_Style.ChannelButtonPadding.Horz();
	int Height=m_Style.ChannelButtonIconSize.Height+m_Style.ChannelButtonPadding.Vert();
	pRect->left=pRect->right-Width;
	pRect->top=pRect->top+((pRect->bottom-pRect->top)-Height)/2;
	pRect->bottom=pRect->top+Height;
}


void CProgramListPanel::GetProgramListRect(RECT *pRect) const
{
	GetClientRect(pRect);
	pRect->top=m_ChannelHeight;
	if (pRect->bottom<pRect->top)
		pRect->bottom=pRect->top;
}


void CProgramListPanel::CalcChannelHeight()
{
	int LabelHeight=m_FontHeight+m_Style.ChannelNameMargin.Vert();
	int ButtonHeight=m_Style.ChannelButtonIconSize.Height+m_Style.ChannelButtonPadding.Vert();
	m_ChannelHeight=max(LabelHeight,ButtonHeight)+m_Style.ChannelPadding.Vert();
}


void CProgramListPanel::CalcDimensions()
{
	HDC hdc=::GetDC(m_hwnd);
	TVTest::CTextDraw DrawText;
	RECT rc;
	GetClientRect(&rc);
	DrawText.Begin(hdc,rc,TVTest::CTextDraw::FLAG_JAPANESE_HYPHNATION);
	GetProgramListRect(&rc);
	HFONT hfontOld=static_cast<HFONT>(::GetCurrentObject(hdc,OBJ_FONT));
	m_TotalLines=0;
	for (int i=0;i<m_ItemList.NumItems();i++) {
		CProgramItemInfo *pItem=m_ItemList.GetItem(i);

		DrawUtil::SelectObject(hdc,m_TitleFont);
		m_TotalLines+=pItem->CalcTitleLines(DrawText,rc.right);
		DrawUtil::SelectObject(hdc,m_Font);
		m_TotalLines+=pItem->CalcTextLines(DrawText,rc.right-GetTextLeftMargin());
	}
	::SelectObject(hdc,hfontOld);
	DrawText.End();
	::ReleaseDC(m_hwnd,hdc);
}


void CProgramListPanel::SetScrollPos(int Pos)
{
	RECT rc;

	GetProgramListRect(&rc);
	const int Page=rc.bottom-rc.top;
	if (Pos<0) {
		Pos=0;
	} else {
		int Max=m_TotalLines*(m_FontHeight+m_Style.LineSpacing)+
				m_ItemList.NumItems()*(m_Style.TitlePadding.Top+m_Style.TitlePadding.Bottom-m_Style.LineSpacing)-Page;
		if (Max<0)
			Max=0;
		if (Pos>Max)
			Pos=Max;
	}
	if (Pos!=m_ScrollPos) {
		int Offset=Pos-m_ScrollPos;
		SCROLLINFO si;

		m_ScrollPos=Pos;
		si.cbSize=sizeof(SCROLLINFO);
		si.fMask=SIF_POS;
		si.nPos=Pos;
		::SetScrollInfo(m_hwnd,SB_VERT,&si,TRUE);
		if (abs(Offset)<Page) {
			::ScrollWindowEx(m_hwnd,0,-Offset,
							 &rc,&rc,NULL,NULL,SW_ERASE | SW_INVALIDATE);
		} else {
			Invalidate(&rc);
		}
		//SetToolTip();
	}
}


void CProgramListPanel::SetScrollBar()
{
	SCROLLINFO si;
	RECT rc;

	si.cbSize=sizeof(SCROLLINFO);
	si.fMask=SIF_PAGE | SIF_RANGE | SIF_POS | SIF_DISABLENOSCROLL;
	si.nMin=0;
	si.nMax=m_TotalLines<1?0:
		m_TotalLines*(m_FontHeight+m_Style.LineSpacing)+
			m_ItemList.NumItems()*(m_Style.TitlePadding.Top+m_Style.TitlePadding.Bottom-m_Style.LineSpacing);
	GetProgramListRect(&rc);
	si.nPage=rc.bottom-rc.top;
	si.nPos=m_ScrollPos;
	::SetScrollInfo(m_hwnd,SB_VERT,&si,TRUE);
}


bool CProgramListPanel::SetProgramListPanelTheme(const ProgramListPanelTheme &Theme)
{
	m_Theme=Theme;
	if (m_hwnd!=NULL)
		Invalidate();
	return true;
}


bool CProgramListPanel::GetProgramListPanelTheme(ProgramListPanelTheme *pTheme) const
{
	if (pTheme==NULL)
		return false;
	*pTheme=m_Theme;
	return true;
}


bool CProgramListPanel::SetFont(const LOGFONT *pFont)
{
	if (!m_Font.Create(pFont))
		return false;
	LOGFONT lf=*pFont;
	lf.lfWeight=FW_BOLD;
	m_TitleFont.Create(&lf);
	m_ScrollPos=0;
	if (m_hwnd!=NULL) {
		CalcFontHeight();
		CalcDimensions();
		SetScrollBar();
		//SetToolTip();
		Invalidate();
	}
	return true;
}


bool CProgramListPanel::SetEventInfoFont(const LOGFONT *pFont)
{
	return m_EventInfoPopup.SetFont(pFont);
}


void CProgramListPanel::CalcFontHeight()
{
	HDC hdc;

	hdc=::GetDC(m_hwnd);
	if (hdc==NULL)
		return;
	m_FontHeight=m_Font.GetHeight();
	::ReleaseDC(m_hwnd,hdc);
}


int CProgramListPanel::GetTextLeftMargin() const
{
	return m_Style.IconSize.Width+m_Style.IconMargin.Left+m_Style.IconMargin.Right;
}


void CProgramListPanel::ShowRetrievingMessage(bool fShow)
{
	if (m_fShowRetrievingMessage!=fShow) {
		m_fShowRetrievingMessage=fShow;
		if (m_hwnd!=NULL)
			Invalidate();
	}
}


void CProgramListPanel::SetVisibleEventIcons(UINT VisibleIcons)
{
	if (m_VisibleEventIcons!=VisibleIcons) {
		m_VisibleEventIcons=VisibleIcons;
		if (m_hwnd!=NULL)
			Invalidate();
	}
}


void CProgramListPanel::SetMouseOverEventInfo(bool fMouseOverEventInfo)
{
	if (m_fMouseOverEventInfo!=fMouseOverEventInfo) {
		m_fMouseOverEventInfo=fMouseOverEventInfo;
		m_EventInfoPopupManager.SetEnable(fMouseOverEventInfo);
	}
}


void CProgramListPanel::SetUseEpgColorScheme(bool fUseEpgColorScheme)
{
	if (m_fUseEpgColorScheme!=fUseEpgColorScheme) {
		m_fUseEpgColorScheme=fUseEpgColorScheme;
		if (m_hwnd!=NULL)
			Invalidate();
	}
}


void CProgramListPanel::SetShowFeaturedMark(bool fShowFeaturedMark)
{
	if (m_fShowFeaturedMark!=fShowFeaturedMark) {
		m_fShowFeaturedMark=fShowFeaturedMark;
		if (m_hwnd!=NULL) {
			if (m_fShowFeaturedMark)
				m_FeaturedEventsMatcher.BeginMatching(GetAppClass().FeaturedEvents.GetSettings());
			Invalidate();
		}
	}
}


int CProgramListPanel::ItemHitTest(int x,int y) const
{
	POINT pt={x,y};
	RECT rcHeader;
	int HotItem=-1;

	GetHeaderRect(&rcHeader);
	if (::PtInRect(&rcHeader,pt)) {
		RECT rc;
		GetChannelButtonRect(&rc);
		if (::PtInRect(&rc,pt)) {
			HotItem=ITEM_CHANNELLISTBUTTON;
		} else if (pt.x<rc.left-m_Style.ChannelButtonMargin) {
			HotItem=ITEM_CHANNEL;
		}
	}

	return HotItem;
}


int CProgramListPanel::ProgramHitTest(int x,int y) const
{
	POINT pt;
	RECT rc;

	pt.x=x;
	pt.y=y;
	GetProgramListRect(&rc);
	if (!::PtInRect(&rc,pt))
		return -1;
	rc.top-=m_ScrollPos;
	for (int i=0;i<m_ItemList.NumItems();i++) {
		const CProgramItemInfo *pItem=m_ItemList.GetItem(i);

		rc.bottom=rc.top+(pItem->GetTitleLines()+pItem->GetTextLines())*(m_FontHeight+m_Style.LineSpacing)+
			(m_Style.TitlePadding.Top+m_Style.TitlePadding.Bottom-m_Style.LineSpacing);
		if (::PtInRect(&rc,pt))
			return i;
		rc.top=rc.bottom;
	}
	return -1;
}


bool CProgramListPanel::GetItemRect(int Item,RECT *pRect) const
{
	if (Item<0 || Item>=m_ItemList.NumItems())
		return false;

	RECT rc;

	GetProgramListRect(&rc);
	rc.top-=m_ScrollPos;
	for (int i=0;;i++) {
		const CProgramItemInfo *pItem=m_ItemList.GetItem(i);

		rc.bottom=rc.top+(pItem->GetTitleLines()+pItem->GetTextLines())*(m_FontHeight+m_Style.LineSpacing)+
			(m_Style.TitlePadding.Top+m_Style.TitlePadding.Bottom-m_Style.LineSpacing);
		if (i==Item)
			break;
		rc.top=rc.bottom;
	}

	*pRect=rc;

	return true;
}


void CProgramListPanel::SetHotItem(int Item)
{
	if (m_HotItem!=Item) {
		m_HotItem=Item;
		RECT rc;
		GetHeaderRect(&rc);
		Invalidate(&rc);
	}
}


void CProgramListPanel::ShowChannelListMenu()
{
	const CChannelList *pChannelList=
		GetAppClass().ChannelManager.GetCurrentChannelList();
	if (pChannelList==NULL)
		return;

	CChannelList ChannelList;

	int ItemCount=0,CurChannel=-1,SelectedChannel=-1;

	for (int i=0;i<pChannelList->NumChannels();i++) {
		const CChannelInfo *pChannelInfo=pChannelList->GetChannelInfo(i);

		if (pChannelInfo->IsEnabled()) {
			ChannelList.AddChannel(*pChannelInfo);

			if (CurChannel<0
					&& m_CurChannel.GetServiceID()>0
					&& m_CurChannel.GetNetworkID()==pChannelInfo->GetNetworkID()
					&& m_CurChannel.GetTransportStreamID()==pChannelInfo->GetTransportStreamID()
					&& m_CurChannel.GetServiceID()==pChannelInfo->GetServiceID())
				CurChannel=ItemCount;
			if (SelectedChannel<0
					&& m_SelectedChannel.GetNetworkID()==pChannelInfo->GetNetworkID()
					&& m_SelectedChannel.GetTransportStreamID()==pChannelInfo->GetTransportStreamID()
					&& m_SelectedChannel.GetServiceID()==pChannelInfo->GetServiceID())
				SelectedChannel=ItemCount;
			ItemCount++;
		}
	}

	if (ItemCount==0)
		return;

	m_ChannelMenu.Create(&ChannelList,CurChannel,1,NULL,m_hwnd,
						 CChannelMenu::FLAG_SHOWLOGO | CChannelMenu::FLAG_SPACEBREAK,
						 GetAppClass().MenuOptions.GetMaxChannelMenuRows());
	if (SelectedChannel>=0)
		m_ChannelMenu.SetHighlightedItem(SelectedChannel);

	RECT rc;

	GetHeaderRect(&rc);
	MapWindowRect(m_hwnd,NULL,&rc);

	int Result=m_ChannelMenu.Show(TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_VERTICAL,
								  rc.left,rc.bottom,&rc);
	m_ChannelMenu.Destroy();

	if (Result>0) {
		const CChannelInfo *pChannelInfo=ChannelList.GetChannelInfo(Result-1);

		if (pChannelInfo!=NULL) {
			SelectChannel(pChannelInfo);
		}
	}
}


/*
void CProgramListPanel::SetToolTip()
{
	if (m_hwndToolTip!=NULL) {
		int NumTools=::SendMessage(m_hwndToolTip,TTM_GETTOOLCOUNT,0,0);
		int NumItems=m_ItemList.NumItems();
		TOOLINFO ti;

		ti.cbSize=TTTOOLINFOA_V2_SIZE;
		ti.hwnd=m_hwnd;
		if (NumTools<NumItems) {
			ti.uFlags=TTF_SUBCLASS;
			ti.hinst=NULL;
			ti.lpszText=LPSTR_TEXTCALLBACK;
			::SetRect(&ti.rect,0,0,0,0);
			for (int i=NumTools;i<NumItems;i++) {
				ti.uId=i;
				ti.lParam=i;
				::SendMessage(m_hwndToolTip,TTM_ADDTOOL,0,(LPARAM)&ti);
			}
		} else if (NumTools>NumItems) {
			for (int i=NumItems;i<NumTools;i++) {
				ti.uId=i;
				::SendMessage(m_hwndToolTip,TTM_DELTOOL,0,(LPARAM)&ti);
			}
		}
		GetClientRect(&ti.rect);
		ti.rect.top=-m_ScrollPos;
		ti.uId=0;
		for (int i=0;i<NumItems;i++) {
			const CProgramItemInfo *pItem=m_ItemList.GetItem(i);

			ti.rect.bottom=ti.rect.top+(pItem->GetTitleLines()+pItem->GetTextLines())*(m_FontHeight+m_Style.LineSpacing)+
				(m_Style.TitlePadding.Top+m_Style.TitlePadding.Bottom-m_Style.LineSpacing);
			::SendMessage(m_hwndToolTip,TTM_NEWTOOLRECT,0,(LPARAM)&ti);
			ti.uId++;
			ti.rect.top=ti.rect.bottom;
		}
	}
}
*/


LRESULT CProgramListPanel::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			InitializeUI();

			if (!m_Font.IsCreated()) {
				LOGFONT lf;
				GetDefaultFont(&lf);
				m_Font.Create(&lf);
				lf.lfWeight=FW_BOLD;
				m_TitleFont.Create(&lf);
			}

			LOGFONT lf;
			::ZeroMemory(&lf,sizeof(lf));
			lf.lfHeight=-m_Style.ChannelButtonIconSize.Height;
			lf.lfCharSet=SYMBOL_CHARSET;
			::lstrcpy(lf.lfFaceName,TEXT("Marlett"));
			m_IconFont.Create(&lf);

			CalcFontHeight();
			CalcChannelHeight();

			m_EpgIcons.Load();
			/*
			m_hwndToolTip=::CreateWindowEx(WS_EX_TOPMOST,TOOLTIPS_CLASS,NULL,
				WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,0,0,0,0,
				hwnd,NULL,m_hinst,NULL);
			::SendMessage(m_hwndToolTip,TTM_SETMAXTIPWIDTH,0,320);
			::SendMessage(m_hwndToolTip,TTM_SETDELAYTIME,TTDT_AUTOPOP,30000);
			*/
			m_EventInfoPopupManager.Initialize(hwnd,&m_EventInfoPopupHandler);
			m_EventInfoPopupManager.SetEnable(m_fMouseOverEventInfo);

			CFeaturedEvents &FeaturedEvents=GetAppClass().FeaturedEvents;
			FeaturedEvents.AddEventHandler(this);
			if (m_fShowFeaturedMark)
				m_FeaturedEventsMatcher.BeginMatching(FeaturedEvents.GetSettings());

			m_HotItem=-1;
		}
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			BeginPaint(hwnd,&ps);
			Draw(ps.hdc,&ps.rcPaint);
			EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_SIZE:
		CalcDimensions();
		SetScrollBar();
		//SetToolTip();
		return 0;

	case WM_MOUSEWHEEL:
		{
			int Delta=m_MouseWheel.OnMouseWheel(wParam,
				(m_FontHeight+m_Style.LineSpacing)*m_MouseWheel.GetDefaultScrollLines());

			if (Delta!=0)
				SetScrollPos(m_ScrollPos-Delta);
		}
		return 0;

	case WM_VSCROLL:
		{
			const int LineHeight=m_FontHeight+m_Style.LineSpacing;
			int Pos,Page,Max;
			RECT rc;

			Pos=m_ScrollPos;
			GetProgramListRect(&rc);
			Page=rc.bottom-rc.top;
			Max=m_TotalLines*LineHeight+
				m_ItemList.NumItems()*(m_Style.TitlePadding.Top+m_Style.TitlePadding.Bottom-m_Style.LineSpacing)-Page;
			if (Max<0)
				Max=0;
			switch (LOWORD(wParam)) {
			case SB_LINEUP:		Pos-=LineHeight;	break;
			case SB_LINEDOWN:	Pos+=LineHeight;	break;
			case SB_PAGEUP:		Pos-=Page;			break;
			case SB_PAGEDOWN:	Pos+=Page;			break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:	Pos=HIWORD(wParam);	break;
			case SB_TOP:		Pos=0;				break;
			case SB_BOTTOM:		Pos=Max;			break;
			default:	return 0;
			}
			SetScrollPos(Pos);
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			int HotItem=ItemHitTest(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));

			if (HotItem!=m_HotItem) {
				SetHotItem(HotItem);

				if (m_HotItem>=0) {
					TRACKMOUSEEVENT tme;

					tme.cbSize=sizeof(tme);
					tme.dwFlags=TME_LEAVE;
					tme.hwndTrack=hwnd;
					::TrackMouseEvent(&tme);
				}
			}
		}
		return 0;

	case WM_MOUSELEAVE:
		if (m_HotItem>=0)
			SetHotItem(-1);
		return 0;

	case WM_LBUTTONDOWN:
		{
			::SetFocus(hwnd);

			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			int HotItem=ItemHitTest(x,y);
			if (HotItem==m_HotItem) {
				switch (HotItem) {
				case ITEM_CHANNEL:
					if (!IsStringEmpty(m_SelectedChannel.GetName())) {
						GetAppClass().Core.SelectChannel(
							nullptr,m_SelectedChannel,CAppCore::SELECT_CHANNEL_USE_CUR_TUNER);
					}
					break;

				case ITEM_CHANNELLISTBUTTON:
					ShowChannelListMenu();
					SetHotItem(-1);
					break;
				}
			} else {
				SetHotItem(HotItem);
			}

			if (HotItem<0 && !m_fMouseOverEventInfo) {
				m_EventInfoPopupManager.Popup(x,y);
			}
		}
		return 0;

	case WM_RBUTTONUP:
		{
			::SetFocus(hwnd);

			CPopupMenu Menu(GetAppClass().GetResourceInstance(),IDM_PROGRAMLISTPANEL);

			Menu.CheckItem(CM_PROGRAMLISTPANEL_MOUSEOVEREVENTINFO,m_fMouseOverEventInfo);
			Menu.CheckItem(CM_PROGRAMLISTPANEL_USEEPGCOLORSCHEME,m_fUseEpgColorScheme);
			Menu.CheckItem(CM_PROGRAMLISTPANEL_SHOWFEATUREDMARK,m_fShowFeaturedMark);
			Menu.Show(hwnd);
		}
		return 0;

	case WM_SETCURSOR:
		if ((HWND)wParam==hwnd) {
			if (LOWORD(lParam)==HTCLIENT && m_HotItem>=0)
				::SetCursor(GetActionCursor());
			else
				::SetCursor(::LoadCursor(NULL,IDC_ARROW));
			return TRUE;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case CM_PROGRAMLISTPANEL_MOUSEOVEREVENTINFO:
			SetMouseOverEventInfo(!m_fMouseOverEventInfo);
			return 0;

		case CM_PROGRAMLISTPANEL_USEEPGCOLORSCHEME:
			SetUseEpgColorScheme(!m_fUseEpgColorScheme);
			return 0;

		case CM_PROGRAMLISTPANEL_SHOWFEATUREDMARK:
			SetShowFeaturedMark(!m_fShowFeaturedMark);
			return 0;
		}
		return 0;

#if 0	// テキストが長過ぎてツールチップを使うと問題がある
	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case TTN_NEEDTEXT:
			{
				LPNMTTDISPINFO pnmtdi=reinterpret_cast<LPNMTTDISPINFO>(lParam);
				const CProgramItemInfo *pItem=m_ItemList.GetItem((int)pnmtdi->lParam);

				if (pItem!=NULL) {
					static TCHAR szText[1024];
					const CEventInfoData &EventInfo=pItem->GetEventInfo();
					TCHAR szEndTime[16];
					SYSTEMTIME stEnd;
					if (EventInfo.m_Duration>0 && EventInfo.GetEndTime(&stEnd))
						StdUtil::snprintf(szEndTime,lengthof(szEndTime),
										  TEXT("～%d:%02d"),stEnd.wHour,stEnd.wMinute);
					else
						szEndTime[0]='\0';
					StdUtil::snprintf(szText,lengthof(szText),
						TEXT("%d/%d(%s) %d:%02d%s\n%s\n\n%s%s%s%s"),
						EventInfo.m_StartTime.wMonth,
						EventInfo.m_StartTime.wDay,
						GetDayOfWeekText(EventInfo.m_StartTime.wDayOfWeek),
						EventInfo.m_StartTime.wHour,
						EventInfo.m_StartTime.wMinute,
						szEndTime,
						EventInfo.m_EventName.c_str(),
						EventInfo.m_EventText.c_str(),
						!EventInfo.m_EventText.empty()?TEXT("\n\n"):TEXT(""),
						EventInfo.m_EventExtendedText.c_str(),
						!EventInfo.m_EventExtendedText.empty()?TEXT("\n\n"):TEXT(""));
					pnmtdi->lpszText=szText;
				} else {
					pnmtdi->lpszText=TEXT("");
				}
				pnmtdi->szText[0]='\0';
				pnmtdi->hinst=NULL;
			}
			return 0;

		case TTN_SHOW:
			{
				// ツールチップの位置がカーソルと重なっていると
				// 出たり消えたりを繰り返しておかしくなるのでずらす
				LPNMHDR pnmh=reinterpret_cast<LPNMHDR>(lParam);
				RECT rcTip;
				POINT pt;

				::GetWindowRect(pnmh->hwndFrom,&rcTip);
				::GetCursorPos(&pt);
				if (::PtInRect(&rcTip,pt)) {
					HMONITOR hMonitor=::MonitorFromRect(&rcTip,MONITOR_DEFAULTTONEAREST);
					if (hMonitor!=NULL) {
						MONITORINFO mi;

						mi.cbSize=sizeof(mi);
						if (::GetMonitorInfo(hMonitor,&mi)) {
							if (rcTip.left<=mi.rcMonitor.left+16)
								rcTip.left=pt.x+16;
							else if (rcTip.right>=mi.rcMonitor.right-16)
								rcTip.left=pt.x-(rcTip.right-rcTip.left)-8;
							else
								break;
							::SetWindowPos(pnmh->hwndFrom,HWND_TOPMOST,
										   rcTip.left,rcTip.top,0,0,
										   SWP_NOSIZE | SWP_NOACTIVATE);
							return TRUE;
						}
					}
				}
			}
			break;
		}
		break;
#endif

	case WM_DESTROY:
		m_EpgIcons.Destroy();
		//m_hwndToolTip=NULL;
		return 0;

	default:
		{
			LRESULT Result;

			if (m_ChannelMenu.HandleMessage(hwnd,uMsg,wParam,lParam,&Result))
				return Result;
		}
		break;
	}

	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


void CProgramListPanel::Draw(HDC hdc,const RECT *prcPaint)
{
	RECT rc,rcMargin;
	TVTest::CTextDraw DrawText;
	GetClientRect(&rc);
	DrawText.Begin(hdc,rc,TVTest::CTextDraw::FLAG_JAPANESE_HYPHNATION);

	const int LineHeight=m_FontHeight+m_Style.LineSpacing;

	HFONT hfontOld=static_cast<HFONT>(::GetCurrentObject(hdc,OBJ_FONT));
	COLORREF crOldTextColor=::GetTextColor(hdc);
	int OldBkMode=::SetBkMode(hdc,TRANSPARENT);

	const bool fCurChannel=
		m_CurChannel.GetServiceID()>0
		&& m_SelectedChannel.GetNetworkID()==m_CurChannel.GetNetworkID()
		&& m_SelectedChannel.GetTransportStreamID()==m_CurChannel.GetTransportStreamID()
		&& m_SelectedChannel.GetServiceID()==m_CurChannel.GetServiceID();

	GetHeaderRect(&rc);
	if (IsRectIntersect(&rc,prcPaint)) {
		const TVTest::Theme::Style &ChannelStyle=
			fCurChannel?m_Theme.CurChannelNameStyle:m_Theme.ChannelNameStyle;

		TVTest::Theme::Draw(hdc,rc,ChannelStyle.Back);

		if (!IsStringEmpty(m_SelectedChannel.GetName())) {
			TVTest::Style::Subtract(&rc,m_Style.ChannelPadding);

			HBITMAP hbmLogo=GetAppClass().LogoManager.GetAssociatedLogoBitmap(
				m_SelectedChannel.GetNetworkID(),m_SelectedChannel.GetServiceID(),
				CLogoManager::LOGOTYPE_SMALL);
			if (hbmLogo!=NULL) {
				int LogoHeight=(rc.bottom-rc.top)-m_Style.ChannelLogoMargin.Vert();
				int LogoWidth=LogoHeight*16/9;
				rc.left+=m_Style.ChannelLogoMargin.Left;
				DrawUtil::DrawBitmap(hdc,
									 rc.left,rc.top+m_Style.ChannelLogoMargin.Top,
									 LogoWidth,LogoHeight,
									 hbmLogo);
				rc.left+=LogoWidth+m_Style.ChannelLogoMargin.Right;
			}

			rc.right-=m_Style.ChannelButtonMargin+
				m_Style.ChannelButtonIconSize.Width+
				m_Style.ChannelButtonPadding.Horz();
			TVTest::Style::Subtract(&rc,m_Style.ChannelNameMargin);
			DrawUtil::SelectObject(hdc,m_TitleFont);
			TVTest::Theme::Draw(hdc,rc,ChannelStyle.Fore,m_SelectedChannel.GetName(),
								DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
		}

		GetChannelButtonRect(&rc);
		const TVTest::Theme::Style &ButtonStyle=
			m_HotItem==ITEM_CHANNELLISTBUTTON?
				m_Theme.ChannelButtonHotStyle:m_Theme.ChannelButtonStyle;
		if (ButtonStyle.Back.Border.Type!=TVTest::Theme::BORDER_NONE
				|| ButtonStyle.Back.Fill!=ChannelStyle.Back.Fill)
			TVTest::Theme::Draw(hdc,rc,ButtonStyle.Back);
		TVTest::Style::Subtract(&rc,m_Style.ChannelButtonPadding);
		DrawUtil::SelectObject(hdc,m_IconFont);
		TVTest::Theme::Draw(hdc,rc,ButtonStyle.Fore,TEXT("6"),
							DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}

	HBRUSH hbr=::CreateSolidBrush(m_Theme.MarginColor);

	GetProgramListRect(&rc);

	if (m_fShowRetrievingMessage && m_ItemList.NumItems()==0) {
		::FillRect(hdc,&rc,hbr);
		DrawUtil::SelectObject(hdc,m_Font);
		::SetTextColor(hdc,m_Theme.EventTextStyle.Fore.Fill.GetSolidColor());
		TVTest::Style::Subtract(&rc,m_Style.TitlePadding);
		DrawText.Draw(TEXT("番組表の取得中です..."),rc,LineHeight);
	} else {
		HRGN hrgn=::CreateRectRgnIndirect(&rc);
		::SelectClipRgn(hdc,hrgn);

		m_EpgIcons.BeginDraw(hdc,m_Style.IconSize.Width,m_Style.IconSize.Height);

		rc.top-=m_ScrollPos;
		for (int i=0;i<m_ItemList.NumItems();i++) {
			CProgramItemInfo *pItem=m_ItemList.GetItem(i);
			const bool fCur=fCurChannel && pItem->GetEventID()==m_CurEventID;
			const int EventTextHeight=pItem->GetTextLines()*LineHeight;

			rc.bottom=rc.top+pItem->GetTitleLines()*LineHeight+
				(m_Style.TitlePadding.Top+m_Style.TitlePadding.Bottom-m_Style.LineSpacing);
			if (m_fUseEpgColorScheme) {
				RECT rcContent;
				rcContent.left=0;
				rcContent.top=rc.top;
				rcContent.right=rc.right;
				rcContent.bottom=rc.bottom+EventTextHeight;
				if (rcContent.bottom>prcPaint->top) {
					unsigned int Flags=CEpgTheme::DRAW_CONTENT_BACKGROUND_SEPARATOR;
					if (fCur)
						Flags|=CEpgTheme::DRAW_CONTENT_BACKGROUND_CURRENT;
					m_EpgTheme.DrawContentBackground(hdc,rcContent,pItem->GetEventInfo(),Flags);
				}
			}
			if (rc.bottom>prcPaint->top) {
				rc.left=0;
				if (m_fUseEpgColorScheme) {
					::SetTextColor(hdc,m_EpgTheme.GetColor(CEpgTheme::COLOR_EVENTNAME));
				} else {
					const TVTest::Theme::Style &Style=
						fCur?m_Theme.CurEventNameStyle:m_Theme.EventNameStyle;
					::SetTextColor(hdc,Style.Fore.Fill.GetSolidColor());
					TVTest::Theme::Draw(hdc,rc,Style.Back);
				}

				RECT rcTitle=rc;
				TVTest::Style::Subtract(&rcTitle,m_Style.TitlePadding);
				DrawUtil::SelectObject(hdc,m_TitleFont);

				if (m_fShowFeaturedMark
						&& m_FeaturedEventsMatcher.IsMatch(pItem->GetEventInfo())) {
					RECT rcMark;
					SIZE sz=pItem->GetTimeSize(hdc);
					if (m_fUseEpgColorScheme) {
						rcMark.left=rcTitle.left;
						rcMark.top=rcTitle.top;
						rcMark.right=rcMark.left+sz.cx;
						rcMark.bottom=rcMark.top+sz.cy;
						TVTest::Style::Subtract(&rcMark,m_Style.FeaturedMarkMargin);
					} else {
						rcMark.left=rc.left+1;
						rcMark.top=rc.top+1;
						rcMark.right=rcMark.left+m_Style.FeaturedMarkSize.Width;
						rcMark.bottom=rcMark.top+m_Style.FeaturedMarkSize.Height;
					}
					TVTest::Theme::Draw(hdc,rcMark,m_Theme.FeaturedMarkStyle);
				}

				pItem->DrawTitle(DrawText,rcTitle,LineHeight);
			}

			rc.top=rc.bottom;
			rc.bottom=rc.top+EventTextHeight;
			if (rc.bottom>prcPaint->top) {
				rc.left=0;
				if (m_fUseEpgColorScheme) {
					::SetTextColor(hdc,m_EpgTheme.GetColor(CEpgTheme::COLOR_EVENTTEXT));
				} else {
					const TVTest::Theme::Style &Style=
						fCur?m_Theme.CurEventTextStyle:m_Theme.EventTextStyle;
					::SetTextColor(hdc,Style.Fore.Fill.GetSolidColor());
					TVTest::Theme::Draw(hdc,rc,Style.Back);
				}
				DrawUtil::SelectObject(hdc,m_Font);
				rc.left=GetTextLeftMargin();
				pItem->DrawText(DrawText,rc,LineHeight);

				const unsigned int ShowIcons=
					CEpgIcons::GetEventIcons(&pItem->GetEventInfo()) & m_VisibleEventIcons;
				if (ShowIcons!=0) {
					rc.left=0;
					m_EpgIcons.DrawIcons(
						ShowIcons,hdc,
						m_Style.IconMargin.Left,rc.top+m_Style.IconMargin.Top,
						m_Style.IconSize.Width,m_Style.IconSize.Height,
						0,m_Style.IconSize.Height+m_Style.IconMargin.Bottom,
						m_fUseEpgColorScheme?255:192,&rc);
				}
			}

			rc.top=rc.bottom;
			if (rc.top>=prcPaint->bottom)
				break;
		}

		if (rc.top<prcPaint->bottom) {
			rcMargin.left=prcPaint->left;
			rcMargin.top=max(rc.top,prcPaint->top);
			rcMargin.right=prcPaint->right;
			rcMargin.bottom=prcPaint->bottom;
			::FillRect(hdc,&rcMargin,hbr);
		}

		m_EpgIcons.EndDraw();

		::SelectClipRgn(hdc,NULL);
		::DeleteObject(hrgn);
	}

	::SetTextColor(hdc,crOldTextColor);
	::SetBkMode(hdc,OldBkMode);
	::SelectObject(hdc,hfontOld);
	::DeleteObject(hbr);
	DrawText.End();
}


void CProgramListPanel::OnFeaturedEventsSettingsChanged(CFeaturedEvents &FeaturedEvents)
{
	if (m_fShowFeaturedMark) {
		m_FeaturedEventsMatcher.BeginMatching(FeaturedEvents.GetSettings());
		Invalidate();
	}
}


CProgramListPanel::CEventInfoPopupHandler::CEventInfoPopupHandler(CProgramListPanel *pPanel)
	: m_pPanel(pPanel)
{
}


bool CProgramListPanel::CEventInfoPopupHandler::HitTest(int x,int y,LPARAM *pParam)
{
	int Program=m_pPanel->ProgramHitTest(x,y);

	if (Program>=0) {
		*pParam=Program;
		return true;
	}
	return false;
}


bool CProgramListPanel::CEventInfoPopupHandler::ShowPopup(LPARAM Param,CEventInfoPopup *pPopup)
{
	const int ItemIndex=static_cast<int>(Param);
	const CProgramItemInfo *pItem=m_pPanel->m_ItemList.GetItem(ItemIndex);
	if (pItem==NULL)
		return false;

	pPopup->SetTitleColor(m_pPanel->m_EpgTheme.GetGenreColor(pItem->GetEventInfo()),
						  m_pPanel->m_EpgTheme.GetColor(CEpgTheme::COLOR_EVENTNAME));

	int IconWidth,IconHeight;
	pPopup->GetPreferredIconSize(&IconWidth,&IconHeight);
	HICON hIcon=GetAppClass().LogoManager.CreateLogoIcon(
		m_pPanel->m_SelectedChannel.GetNetworkID(),
		m_pPanel->m_SelectedChannel.GetServiceID(),
		IconWidth,IconHeight);

	RECT rc;
	POINT pt;
	m_pPanel->GetItemRect(ItemIndex,&rc);
	pt.x=rc.left;
	pt.y=rc.bottom;
	::ClientToScreen(m_pPanel->m_hwnd,&pt);
	pPopup->GetDefaultPopupPosition(&rc);
	if (rc.top>pt.y) {
		rc.bottom=pt.y+(rc.bottom-rc.top);
		rc.top=pt.y;
	}

	if (!pPopup->Show(&pItem->GetEventInfo(),&rc,
					  hIcon,m_pPanel->m_SelectedChannel.GetName())) {
		if (hIcon!=NULL)
			::DestroyIcon(hIcon);
		return false;
	}

	return true;
}




CProgramListPanel::ProgramListPanelStyle::ProgramListPanelStyle()
	: ChannelPadding(3,3,3,3)
	, ChannelLogoMargin(0,0,3,0)
	, ChannelNameMargin(0,2,0,2)
	, ChannelButtonIconSize(12,12)
	, ChannelButtonPadding(2)
	, ChannelButtonMargin(12)
	, TitlePadding(2)
	, IconSize(CEpgIcons::DEFAULT_ICON_WIDTH,CEpgIcons::DEFAULT_ICON_HEIGHT)
	, IconMargin(1)
	, LineSpacing(1)
	, FeaturedMarkSize(5,5)
	, FeaturedMarkMargin(0)
{
}


void CProgramListPanel::ProgramListPanelStyle::SetStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	pStyleManager->Get(TEXT("program-list-panel.channel.padding"),&ChannelPadding);
	pStyleManager->Get(TEXT("program-list-panel.channel.logo.margin"),&ChannelLogoMargin);
	pStyleManager->Get(TEXT("program-list-panel.channel.channel-name.margin"),&ChannelNameMargin);
	pStyleManager->Get(TEXT("program-list-panel.channel.button.icon"),&ChannelButtonIconSize);
	pStyleManager->Get(TEXT("program-list-panel.channel.button.padding"),&ChannelButtonPadding);
	pStyleManager->Get(TEXT("program-list-panel.channel.button.margin"),&ChannelButtonMargin);
	pStyleManager->Get(TEXT("program-list-panel.title.padding"),&TitlePadding);
	pStyleManager->Get(TEXT("program-list-panel.icon"),&IconSize);
	pStyleManager->Get(TEXT("program-list-panel.icon.margin"),&IconMargin);
	pStyleManager->Get(TEXT("program-list-panel.line-spacing"),&LineSpacing);
	pStyleManager->Get(TEXT("program-list-panel.featured-mark"),&FeaturedMarkSize);
	pStyleManager->Get(TEXT("program-guide.event.featured-mark.margin"),&FeaturedMarkMargin);
}


void CProgramListPanel::ProgramListPanelStyle::NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager)
{
	pStyleManager->ToPixels(&ChannelPadding);
	pStyleManager->ToPixels(&ChannelLogoMargin);
	pStyleManager->ToPixels(&ChannelNameMargin);
	pStyleManager->ToPixels(&ChannelButtonIconSize);
	pStyleManager->ToPixels(&ChannelButtonPadding);
	pStyleManager->ToPixels(&ChannelButtonMargin);
	pStyleManager->ToPixels(&TitlePadding);
	pStyleManager->ToPixels(&IconSize);
	pStyleManager->ToPixels(&IconMargin);
	pStyleManager->ToPixels(&LineSpacing);
	pStyleManager->ToPixels(&FeaturedMarkSize);
	pStyleManager->ToPixels(&FeaturedMarkMargin);
}
