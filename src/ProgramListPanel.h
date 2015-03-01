#ifndef PROGRAM_LIST_PANEL_H
#define PROGRAM_LIST_PANEL_H


#include "PanelForm.h"
#include "UIBase.h"
#include "EpgProgramList.h"
#include "EpgUtil.h"
#include "ChannelList.h"
#include "Theme.h"
#include "DrawUtil.h"
#include "Menu.h"
#include "EventInfoPopup.h"
#include "WindowUtil.h"
#include "FeaturedEvents.h"


class CProgramItemInfo;

class CProgramItemList
{
	int m_NumItems;
	CProgramItemInfo **m_ppItemList;
	int m_ItemListLength;

public:
	CProgramItemList();
	~CProgramItemList();
	int NumItems() const { return m_NumItems; }
	CProgramItemInfo *GetItem(int Index);
	const CProgramItemInfo *GetItem(int Index) const;
	bool Add(CProgramItemInfo *pItem);
	void Clear();
	void Reserve(int NumItems);
	void Attach(CProgramItemList *pList);
};

class CProgramListPanel
	: public CPanelForm::CPage
	, public TVTest::CUIBase
	, public CSettingsBase
	, protected CFeaturedEvents::CEventHandler
{
public:
	struct ProgramListPanelTheme {
		TVTest::Theme::Style ChannelNameStyle;
		TVTest::Theme::Style CurChannelNameStyle;
		TVTest::Theme::Style ChannelButtonStyle;
		TVTest::Theme::Style ChannelButtonHotStyle;
		TVTest::Theme::Style EventNameStyle;
		TVTest::Theme::Style CurEventNameStyle;
		TVTest::Theme::Style EventTextStyle;
		TVTest::Theme::Style CurEventTextStyle;
		TVTest::Theme::ThemeColor MarginColor;
		TVTest::Theme::BackgroundStyle FeaturedMarkStyle;
	};

	static bool Initialize(HINSTANCE hinst);

	CProgramListPanel();
	~CProgramListPanel();

// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;

// CUIBase
	void SetStyle(const TVTest::Style::CStyleManager *pStyleManager) override;
	void NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager) override;
	void SetTheme(const TVTest::Theme::CThemeManager *pThemeManager) override;

// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;

// CProgramListPanel
	void SetEpgProgramList(CEpgProgramList *pList) { m_pProgramList=pList; }
	bool UpdateProgramList(const CChannelInfo *pChannelInfo);
	void ClearProgramList();
	void SelectChannel(const CChannelInfo *pChannelInfo,bool fUpdate=true);
	void SetCurrentChannel(const CChannelInfo *pChannelInfo);
	void SetCurrentEventID(int EventID);
	bool SetProgramListPanelTheme(const ProgramListPanelTheme &Theme);
	bool GetProgramListPanelTheme(ProgramListPanelTheme *pTheme) const;
	bool SetFont(const LOGFONT *pFont);
	bool SetEventInfoFont(const LOGFONT *pFont);
	void ShowRetrievingMessage(bool fShow);
	void SetVisibleEventIcons(UINT VisibleIcons);
	void SetMouseOverEventInfo(bool fMouseOverEventInfo);
	bool GetMouseOverEventInfo() const { return m_fMouseOverEventInfo; }
	void SetUseEpgColorScheme(bool fUseEpgColorScheme);
	bool GetUseEpgColorScheme() const { return m_fUseEpgColorScheme; }
	void SetShowFeaturedMark(bool fShowFeaturedMark);
	bool GetShowFeaturedMark() const { return m_fShowFeaturedMark; }

private:
	struct ProgramListPanelStyle
	{
		TVTest::Style::Margins ChannelPadding;
		TVTest::Style::Margins ChannelLogoMargin;
		TVTest::Style::Margins ChannelNameMargin;
		TVTest::Style::Size ChannelButtonIconSize;
		TVTest::Style::Margins ChannelButtonPadding;
		TVTest::Style::IntValue ChannelButtonMargin;
		TVTest::Style::Margins TitlePadding;
		TVTest::Style::Size IconSize;
		TVTest::Style::Margins IconMargin;
		TVTest::Style::IntValue LineSpacing;
		TVTest::Style::Size FeaturedMarkSize;
		TVTest::Style::Margins FeaturedMarkMargin;

		ProgramListPanelStyle();
		void SetStyle(const TVTest::Style::CStyleManager *pStyleManager);
		void NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager);
	};

	enum {
		ITEM_CHANNEL,
		ITEM_CHANNELLISTBUTTON
	};

	CEpgProgramList *m_pProgramList;
	DrawUtil::CFont m_Font;
	DrawUtil::CFont m_TitleFont;
	DrawUtil::CFont m_IconFont;
	int m_FontHeight;
	ProgramListPanelStyle m_Style;
	ProgramListPanelTheme m_Theme;
	CEpgTheme m_EpgTheme;
	bool m_fMouseOverEventInfo;
	bool m_fUseEpgColorScheme;
	bool m_fShowFeaturedMark;
	CEpgIcons m_EpgIcons;
	UINT m_VisibleEventIcons;
	int m_ChannelHeight;
	int m_TotalLines;
	CProgramItemList m_ItemList;
	CChannelInfo m_SelectedChannel;
	CChannelInfo m_CurChannel;
	int m_CurEventID;
	int m_ScrollPos;
	int m_HotItem;
	CChannelMenu m_ChannelMenu;
	CMouseWheelHandler m_MouseWheel;
	//HWND m_hwndToolTip;
	CEventInfoPopup m_EventInfoPopup;
	CEventInfoPopupManager m_EventInfoPopupManager;
	class CEventInfoPopupHandler : public CEventInfoPopupManager::CEventHandler
	{
		CProgramListPanel *m_pPanel;
	public:
		CEventInfoPopupHandler(CProgramListPanel *pPanel);
		bool HitTest(int x,int y,LPARAM *pParam);
		bool ShowPopup(LPARAM Param,CEventInfoPopup *pPopup);
	};
	CEventInfoPopupHandler m_EventInfoPopupHandler;
	CFeaturedEventsMatcher m_FeaturedEventsMatcher;
	bool m_fShowRetrievingMessage;

	static const LPCTSTR m_pszClassName;
	static HINSTANCE m_hinst;

	void Draw(HDC hdc,const RECT *prcPaint);
	bool UpdateListInfo(const CChannelInfo *pChannelInfo);
	void GetHeaderRect(RECT *pRect) const;
	void GetChannelButtonRect(RECT *pRect) const;
	void GetProgramListRect(RECT *pRect) const;
	void CalcChannelHeight();
	void CalcDimensions();
	void SetScrollPos(int Pos);
	void SetScrollBar();
	void CalcFontHeight();
	int GetTextLeftMargin() const;
	int ItemHitTest(int x,int y) const;
	int ProgramHitTest(int x,int y) const;
	bool GetItemRect(int Item,RECT *pRect) const;
	void SetHotItem(int Item);
	void ShowChannelListMenu();
	//void SetToolTip();

// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

// CPanelForm::CPage
	bool NeedKeyboardFocus() const override { return true; }

// CFeaturedEvents::CEventHandler
	void OnFeaturedEventsSettingsChanged(CFeaturedEvents &FeaturedEvents) override;
};


#endif
