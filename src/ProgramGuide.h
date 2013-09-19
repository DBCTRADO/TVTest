#ifndef PROGRAM_GUIDE_H
#define PROGRAM_GUIDE_H


#include "View.h"
#include "EpgProgramList.h"
#include "EpgUtil.h"
#include "ChannelList.h"
#include "Theme.h"
#include "DrawUtil.h"
#include "ProgramSearch.h"
#include "ProgramGuideFavorites.h"
#include "EventInfoPopup.h"
#include "Tooltip.h"
#include "StatusView.h"
#include "Settings.h"


namespace ProgramGuide
{

	class CEventItem;
	class CEventLayout;
	class CServiceInfo;

	class CEventLayoutList
	{
		std::vector<CEventLayout*> m_LayoutList;

	public:
		~CEventLayoutList();
		void Clear();
		size_t Length() const { return m_LayoutList.size(); }
		void Add(CEventLayout *pLayout);
		CEventLayout *operator[](size_t Index);
		const CEventLayout *operator[](size_t Index) const;
	};

	class CServiceList
	{
		std::vector<CServiceInfo*> m_ServiceList;

	public:
		~CServiceList();
		size_t NumServices() const { return m_ServiceList.size(); }
		CServiceInfo *GetItem(size_t Index);
		const CServiceInfo *GetItem(size_t Index) const;
		CServiceInfo *GetItemByIDs(WORD TransportStreamID,WORD ServiceID);
		const CServiceInfo *GetItemByIDs(WORD TransportStreamID,WORD ServiceID) const;
		int FindItemByIDs(WORD TransportStreamID,WORD ServiceID) const;
		CEventInfoData *GetEventByIDs(WORD TransportStreamID,WORD ServiceID,WORD EventID);
		const CEventInfoData *GetEventByIDs(WORD TransportStreamID,WORD ServiceID,WORD EventID) const;
		void Add(CServiceInfo *pInfo);
		void Clear();
	};

}

// 番組表のチャンネル一覧取得クラス
class ABSTRACT_CLASS(CProgramGuideChannelProvider)
{
public:
	virtual ~CProgramGuideChannelProvider();
	virtual bool Update();
	virtual bool GetName(LPTSTR pszName,int MaxName) const = 0;
	virtual size_t GetGroupCount() const = 0;
	virtual bool GetGroupName(size_t Group,LPTSTR pszName,int MaxName) const = 0;
	virtual size_t GetChannelCount(size_t Group) const = 0;
	virtual const CChannelInfo *GetChannelInfo(size_t Group,size_t Channel) const = 0;
	virtual bool GetBonDriver(LPTSTR pszFileName,int MaxLength) const = 0;
	virtual bool GetBonDriverFileName(size_t Group,size_t Channel,LPTSTR pszFileName,int MaxLength) const = 0;
};

class CProgramGuideBaseChannelProvider : public CProgramGuideChannelProvider
{
public:
	CProgramGuideBaseChannelProvider();
	CProgramGuideBaseChannelProvider(const CTuningSpaceList *pSpaceList,LPCTSTR pszBonDriver);
	~CProgramGuideBaseChannelProvider();
// CProgramGuideChannelProvider
	virtual bool GetName(LPTSTR pszName,int MaxName) const override;
	virtual size_t GetGroupCount() const override;
	virtual bool GetGroupName(size_t Group,LPTSTR pszName,int MaxName) const override;
	virtual size_t GetChannelCount(size_t Group) const override;
	virtual const CChannelInfo *GetChannelInfo(size_t Group,size_t Channel) const override;
	virtual bool GetBonDriver(LPTSTR pszFileName,int MaxLength) const override;
	virtual bool GetBonDriverFileName(size_t Group,size_t Channel,LPTSTR pszFileName,int MaxLength) const override;
// CProgramGuideBaseChannelProvider
	bool SetTuningSpaceList(const CTuningSpaceList *pList);
	bool SetBonDriverFileName(LPCTSTR pszFileName);
	bool HasAllChannelGroup() const;

protected:
	CTuningSpaceList m_TuningSpaceList;
	TVTest::String m_BonDriverFileName;
};

// 番組表のチャンネル一覧取得管理クラス
class ABSTRACT_CLASS(CProgramGuideChannelProviderManager)
{
public:
	virtual ~CProgramGuideChannelProviderManager();
	virtual size_t GetChannelProviderCount() const = 0;
	virtual CProgramGuideChannelProvider *GetChannelProvider(size_t Index) const = 0;
};

class CProgramGuideTool
{
public:
	enum { MAX_NAME=64, MAX_COMMAND=MAX_PATH*2 };

private:
	TCHAR m_szName[MAX_NAME];
	TCHAR m_szCommand[MAX_COMMAND];
	HICON m_hIcon;

	static bool GetCommandFileName(LPCTSTR *ppszCommand,LPTSTR pszFileName,int MaxFileName);
	static CProgramGuideTool *GetThis(HWND hDlg);
	static INT_PTR CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	CProgramGuideTool();
	CProgramGuideTool(const CProgramGuideTool &Tool);
	CProgramGuideTool(LPCTSTR pszName,LPCTSTR pszCommand);
	~CProgramGuideTool();
	CProgramGuideTool &operator=(const CProgramGuideTool &Tool);
	LPCTSTR GetName() const { return m_szName; }
	LPCTSTR GetCommand() const { return m_szCommand; }
	bool GetPath(LPTSTR pszPath,int MaxLength) const;
	HICON GetIcon();
	bool Execute(const ProgramGuide::CServiceInfo *pServiceInfo,
				 const CEventInfoData *pEventInfo,HWND hwnd);
	bool ShowDialog(HWND hwndOwner);
};

class CProgramGuideToolList
{
	std::vector<CProgramGuideTool*> m_ToolList;

public:
	CProgramGuideToolList();
	CProgramGuideToolList(const CProgramGuideToolList &Src);
	~CProgramGuideToolList();
	CProgramGuideToolList &operator=(const CProgramGuideToolList &Src);
	void Clear();
	bool Add(CProgramGuideTool *pTool);
	CProgramGuideTool *GetTool(size_t Index);
	const CProgramGuideTool *GetTool(size_t Index) const;
	size_t NumTools() const { return m_ToolList.size(); }
};

class CProgramGuide : public CCustomWindow, protected CDoubleBufferingDraw
{
public:
	enum ListMode {
		LIST_SERVICES,
		LIST_WEEK
	};
	enum {
		DAY_TODAY,
		DAY_TOMORROW,
		DAY_DAYAFTERTOMORROW,
		DAY_2DAYSAFTERTOMORROW,
		DAY_3DAYSAFTERTOMORROW,
		DAY_4DAYSAFTERTOMORROW,
		DAY_5DAYSAFTERTOMORROW,
		DAY_6DAYSAFTERTOMORROW,
		DAY_FIRST=DAY_TODAY,
		DAY_LAST=DAY_6DAYSAFTERTOMORROW
	};
	enum {
		COLOR_BACK,
		COLOR_EVENTTITLE,
		COLOR_EVENTTEXT,
		COLOR_CHANNELNAMETEXT,
		COLOR_CURCHANNELNAMETEXT,
		COLOR_TIMETEXT,
		COLOR_TIMELINE,
		COLOR_CURTIMELINE,
		COLOR_HIGHLIGHT_TITLE,
		COLOR_HIGHLIGHT_TEXT,
		COLOR_HIGHLIGHT_BACK,
		COLOR_HIGHLIGHT_BORDER,
		COLOR_CONTENT_NEWS,
		COLOR_CONTENT_SPORTS,
		COLOR_CONTENT_INFORMATION,
		COLOR_CONTENT_DRAMA,
		COLOR_CONTENT_MUSIC,
		COLOR_CONTENT_VARIETY,
		COLOR_CONTENT_MOVIE,
		COLOR_CONTENT_ANIME,
		COLOR_CONTENT_DOCUMENTARY,
		COLOR_CONTENT_THEATER,
		COLOR_CONTENT_EDUCATION,
		COLOR_CONTENT_WELFARE,
		COLOR_CONTENT_OTHER,
		COLOR_CONTENT_FIRST=COLOR_CONTENT_NEWS,
		COLOR_CONTENT_LAST=COLOR_CONTENT_OTHER,
		COLOR_LAST=COLOR_CONTENT_LAST,
		NUM_COLORS=COLOR_LAST+1
	};
	enum { MIN_LINES_PER_HOUR=8, MAX_LINES_PER_HOUR=60 };
	enum { MIN_ITEM_WIDTH=100, MAX_ITEM_WIDTH=500 };
	enum { TIME_BAR_BACK_COLORS=8 };

	enum {
		FILTER_FREE			=0x00000001U,
		FILTER_NEWPROGRAM	=0x00000002U,
		FILTER_ORIGINAL		=0x00000004U,
		FILTER_RERUN		=0x00000008U,
		FILTER_NOT_SHOPPING	=0x00000010U,
		FILTER_GENRE_FIRST	=0x00000100U,
		FILTER_GENRE_MASK	=0x000FFF00U,
		FILTER_NEWS			=0x00000100U,
		FILTER_SPORTS		=0x00000200U,
		FILTER_INFORMATION	=0x00000400U,
		FILTER_DRAMA		=0x00000800U,
		FILTER_MUSIC		=0x00001000U,
		FILTER_VARIETY		=0x00002000U,
		FILTER_MOVIE		=0x00004000U,
		FILTER_ANIME		=0x00008000U,
		FILTER_DOCUMENTARY	=0x00010000U,
		FILTER_THEATER		=0x00020000U,
		FILTER_EDUCATION	=0x00040000U,
		FILTER_WELFARE		=0x00080000U
	};

	struct ServiceInfo
	{
		WORD NetworkID;
		WORD TransportStreamID;
		WORD ServiceID;

		ServiceInfo() { Clear(); }
		ServiceInfo(WORD NID,WORD TSID,WORD SID)
			: NetworkID(NID), TransportStreamID(TSID), ServiceID(SID) {}
		void Clear() {
			NetworkID=0;
			TransportStreamID=0;
			ServiceID=0;
		}
	};

	typedef std::vector<ServiceInfo> ServiceInfoList;

	class ABSTRACT_CLASS(CFrame)
	{
	protected:
		CProgramGuide *m_pProgramGuide;
	public:
		CFrame();
		virtual ~CFrame() = 0;
		virtual void SetCaption(LPCTSTR pszCaption) {}
		virtual void OnDateChanged() {}
		virtual void OnSpaceChanged() {}
		virtual void OnListModeChanged() {}
		virtual void OnTimeRangeChanged() {}
		virtual void OnFavoritesChanged() {}
		virtual bool OnCommand(int Command) { return false; }
		virtual void OnMenuInitialize(HMENU hmenu) {}
		virtual bool SetAlwaysOnTop(bool fTop) { return false; }
		virtual bool GetAlwaysOnTop() const { return false; }
		friend class CProgramGuide;
	};

	class ABSTRACT_CLASS(CEventHandler)
	{
	protected:
		class CProgramGuide *m_pProgramGuide;
	public:
		CEventHandler();
		virtual ~CEventHandler();
		virtual bool OnClose() { return true; }
		virtual void OnDestroy() {}
		virtual void OnServiceTitleLButtonDown(LPCTSTR pszDriverFileName,const CServiceInfoData *pServiceInfo) {}
		virtual bool OnBeginUpdate(LPCTSTR pszBonDriver,const CChannelList *pChannelList) = 0;
		virtual void OnEndUpdate() {}
		virtual bool OnRefresh() { return true; }
		virtual bool OnKeyDown(UINT KeyCode,UINT Flags) { return false; }
		virtual bool OnMenuInitialize(HMENU hmenu,UINT CommandBase) { return false; }
		virtual bool OnMenuSelected(UINT Command) { return false; }
		friend class CProgramGuide;
	};

	class ABSTRACT_CLASS(CProgramCustomizer)
	{
	protected:
		class CProgramGuide *m_pProgramGuide;
	public:
		CProgramCustomizer();
		virtual ~CProgramCustomizer();
		virtual bool Initialize() { return true; }
		virtual void Finalize() {}
		virtual bool DrawBackground(const CEventInfoData &Event,HDC hdc,
			const RECT &ItemRect,const RECT &TitleRect,const RECT &ContentRect,
			COLORREF BackgroundColor) { return false; }
		virtual bool InitializeMenu(const CEventInfoData &Event,HMENU hmenu,UINT CommandBase,
									const POINT &CursorPos,const RECT &ItemRect) { return false; }
		virtual bool ProcessMenu(const CEventInfoData &Event,UINT Command) { return false; }
		virtual bool OnLButtonDoubleClick(const CEventInfoData &Event,
										  const POINT &CursorPos,const RECT &ItemRect) { return false; }
		friend class CProgramGuide;
	};

	static bool Initialize(HINSTANCE hinst);

	CProgramGuide(CEventSearchOptions &EventSearchOptions);
	~CProgramGuide();

// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;

// CProgramGuide
	bool SetEpgProgramList(CEpgProgramList *pList);
	void Clear();
	bool UpdateProgramGuide(bool fUpdateList=false);
	bool SetChannelProviderManager(CProgramGuideChannelProviderManager *pManager);
	bool EnumChannelProvider(int Index,LPTSTR pszName,int MaxName) const;
	bool SetCurrentChannelProvider(int Provider,int Group);
	int GetCurrentChannelProvider() const { return m_CurrentChannelProvider; }
	int GetChannelGroupCount() const;
	bool GetChannelGroupName(int Group,LPTSTR pszName,int MaxName) const;
	bool SetCurrentChannelGroup(int Group);
	int GetCurrentChannelGroup() const { return m_CurrentChannelGroup; }
	bool GetChannelList(CChannelList *pList,bool fVisibleOnly) const;
	const ProgramGuide::CServiceList &GetServiceList() const { return m_ServiceList; }
	void SetCurrentService(WORD NetworkID,WORD TSID,WORD ServiceID);
	void ClearCurrentService() { SetCurrentService(0,0,0); }

	bool GetExcludeNoEventServices() const { return m_fExcludeNoEventServices; }
	bool SetExcludeNoEventServices(bool fExclude);
	bool SetExcludeServiceList(const ServiceInfoList &List);
	bool GetExcludeServiceList(ServiceInfoList *pList) const;
	bool IsExcludeService(WORD NetworkID,WORD TransportStreamID,WORD ServiceID) const;
	bool SetExcludeService(WORD NetworkID,WORD TransportStreamID,WORD ServiceID,bool fExclude);

	ListMode GetListMode() const { return m_ListMode; }
	bool SetServiceListMode();
	bool SetWeekListMode(int Service);
	int GetWeekListService() const { return m_WeekListService; }
	bool SetBeginHour(int Hour);
	bool SetTimeRange(const SYSTEMTIME *pFirstTime,const SYSTEMTIME *pLastTime);
	bool GetTimeRange(SYSTEMTIME *pFirstTime,SYSTEMTIME *pLastTime) const;
	bool GetCurrentTimeRange(SYSTEMTIME *pFirstTime,SYSTEMTIME *pLastTime) const;
	bool GetDayTimeRange(int Day,SYSTEMTIME *pFirstTime,SYSTEMTIME *pLastTime) const;
	bool ScrollToTime(const SYSTEMTIME &Time,bool fHour=false);
	bool ScrollToCurrentTime();
	bool SetViewDay(int Day);
	int GetViewDay() const { return m_Day; }
	bool JumpEvent(WORD NetworkID,WORD TSID,WORD ServiceID,WORD EventID);
	bool JumpEvent(const CEventInfoData &EventInfo);
	bool ScrollToCurrentService();

	int GetLinesPerHour() const { return m_LinesPerHour; }
	int GetItemWidth() const { return m_ItemWidth; }
	bool SetUIOptions(int LinesPerHour,int ItemWidth,int LineMargin);
	bool SetColor(int Type,COLORREF Color);
	void SetBackColors(const Theme::GradientInfo *pChannelBackGradient,
					   const Theme::GradientInfo *pCurChannelBackGradient,
					   const Theme::GradientInfo *pTimeBarMarginGradient,
					   const Theme::GradientInfo *pTimeBarBackGradient);
	bool SetFont(const LOGFONT *pFont);
	bool GetFont(LOGFONT *pFont) const;
	bool SetEventInfoFont(const LOGFONT *pFont);
	bool SetShowToolTip(bool fShow);
	bool GetShowToolTip() const { return m_fShowToolTip; }
	void SetEventHandler(CEventHandler *pEventHandler);
	void SetFrame(CFrame *pFrame);
	void SetProgramCustomizer(CProgramCustomizer *pProgramCustomizer);
	int GetWheelScrollLines() const { return m_WheelScrollLines; }
	void SetWheelScrollLines(int Lines) { m_WheelScrollLines=Lines; }
	bool GetDragScroll() const { return m_fDragScroll; }
	bool SetDragScroll(bool fDragScroll);
	bool SetFilter(unsigned int Filter);
	unsigned int GetFilter() const { return m_Filter; }
	void SetVisibleEventIcons(UINT VisibleIcons);
	UINT GetVisibleEventIcons() const { return m_VisibleEventIcons; }
	bool GetKeepTimePos() const { return m_fKeepTimePos; }
	void SetKeepTimePos(bool fKeep);

	void GetInfoPopupSize(int *pWidth,int *pHeight) { m_EventInfoPopup.GetSize(pWidth,pHeight); }
	bool SetInfoPopupSize(int Width,int Height) { return m_EventInfoPopup.SetSize(Width,Height); }

	CProgramSearchDialog *GetProgramSearch() { return &m_ProgramSearch; }
	bool ShowProgramSearch(bool fShow);

	void SetMessage(LPCTSTR pszMessage,bool fUpdate=true);
	void SetEpgUpdateProgress(int Pos,int End,DWORD RemainingTime);

	CProgramGuideToolList *GetToolList() { return &m_ToolList; }
	CProgramGuideFavorites *GetFavorites() { return &m_Favorites; }

	bool OnCloseFrame();
	void OnShowFrame(bool fShow);

private:
	CEpgProgramList *m_pProgramList;
	ProgramGuide::CServiceList m_ServiceList;
	ProgramGuide::CEventLayoutList m_EventLayoutList;
	ListMode m_ListMode;
	int m_WeekListService;
	int m_LinesPerHour;
	DrawUtil::CFont m_Font;
	DrawUtil::CFont m_TitleFont;
	DrawUtil::CFont m_TimeFont;
	int m_FontHeight;
	int m_LineMargin;
	int m_ItemWidth;
	int m_ItemMargin;
	int m_TextLeftMargin;
	int m_HeaderHeight;
	int m_TimeBarWidth;
	POINT m_ScrollPos;
	POINT m_OldScrollPos;
	bool m_fDragScroll;
	bool m_fScrolling;
	HCURSOR m_hDragCursor1;
	HCURSOR m_hDragCursor2;
	struct {
		POINT StartCursorPos;
		POINT StartScrollPos;
	} m_DragInfo;
	DrawUtil::CMonoColorBitmap m_Chevron;
	CEpgIcons m_EpgIcons;
	UINT m_VisibleEventIcons;
	bool m_fBarShadow;
	CEventInfoPopup m_EventInfoPopup;
	CEventInfoPopupManager m_EventInfoPopupManager;
	class CEventInfoPopupHandler : public CEventInfoPopupManager::CEventHandler
								 , public CEventInfoPopup::CEventHandler
	{
		CProgramGuide *m_pProgramGuide;
	// CEventInfoPopupManager::CEventHandler
		bool HitTest(int x,int y,LPARAM *pParam);
		bool GetEventInfo(LPARAM Param,const CEventInfoData **ppInfo);
		bool OnShow(const CEventInfoData *pInfo);
	// CEventInfoPopup::CEventHandler
		bool OnMenuPopup(HMENU hmenu);
		void OnMenuSelected(int Command);
	public:
		CEventInfoPopupHandler(CProgramGuide *pProgramGuide);
	};
	CEventInfoPopupHandler m_EventInfoPopupHandler;
	bool m_fShowToolTip;
	CTooltip m_Tooltip;
	bool m_fKeepTimePos;
	int m_CurTimePos;

	CProgramGuideChannelProviderManager *m_pChannelProviderManager;
	CProgramGuideChannelProvider *m_pChannelProvider;
	int m_CurrentChannelProvider;
	int m_CurrentChannelGroup;
	ServiceInfo m_CurrentChannel;
	bool m_fExcludeNoEventServices;
	ServiceInfoList m_ExcludeServiceList;

	int m_BeginHour;
	SYSTEMTIME m_stFirstTime;
	SYSTEMTIME m_stLastTime;
	SYSTEMTIME m_stCurTime;
	int m_Day;
	int m_Hours;

	struct EventSelectInfo {
		bool fSelected;
		int ListIndex;
		int EventIndex;
	};
	EventSelectInfo m_CurEventItem;

	CEventHandler *m_pEventHandler;
	CFrame *m_pFrame;
	CProgramCustomizer *m_pProgramCustomizer;
	COLORREF m_ColorList[NUM_COLORS];
	Theme::GradientInfo m_ChannelNameBackGradient;
	Theme::GradientInfo m_CurChannelNameBackGradient;
	Theme::GradientInfo m_TimeBarMarginGradient;
	Theme::GradientInfo m_TimeBarBackGradient[TIME_BAR_BACK_COLORS];
	CProgramGuideToolList m_ToolList;
	int m_WheelScrollLines;
	unsigned int m_Filter;

	bool m_fEpgUpdating;
	struct {
		int Pos;
		int End;
		DWORD RemainingTime;
		void Clear() { Pos=0; End=0; RemainingTime=0; }
	} m_EpgUpdateProgress;

	CProgramSearchDialog m_ProgramSearch;
	class CProgramSearchEventHandler : public CProgramSearchDialog::CEventHandler
	{
	public:
		CProgramSearchEventHandler(CProgramGuide *pProgramGuide);
		bool OnSearch() override;
		void OnEndSearch() override;
		bool OnClose() override;
		bool OnLDoubleClick(const CEventInfoData *pEventInfo,LPARAM Param) override;
		bool OnRButtonClick(const CEventInfoData *pEventInfo,LPARAM Param) override;
		void OnHighlightChange(bool fHighlight) override;
	private:
		void DoCommand(int Command,const CEventInfoData *pEventInfo);
		CProgramGuide *m_pProgramGuide;
	};
	CProgramSearchEventHandler m_ProgramSearchEventHandler;

	TVTest::String m_Message;

	CProgramGuideFavorites m_Favorites;

	static const LPCTSTR m_pszWindowClass;
	static HINSTANCE m_hinst;

	bool UpdateList(bool fUpdateList=false);
	bool UpdateService(ProgramGuide::CServiceInfo *pService,bool fUpdateEpg);
	void UpdateServiceList();
	void CalcLayout();
	void DrawEventList(const ProgramGuide::CEventLayout *pLayout,
					   HDC hdc,const RECT &Rect,const RECT &PaintRect) const;
	void DrawHeaderBackground(HDC hdc,const RECT &Rect,bool fCur) const;
	void DrawServiceHeader(ProgramGuide::CServiceInfo *pServiceInfo,
						   HDC hdc,const RECT &Rect,int Chevron,
						   bool fLeftAlign=false);
	void DrawDayHeader(int Day,HDC hdc,const RECT &Rect) const;
	void DrawTimeBar(HDC hdc,const RECT &Rect,bool fRight);
	void Draw(HDC hdc,const RECT &PaintRect);
	void DrawMessage(HDC hdc,const RECT &ClientRect) const;
	int GetCurTimeLinePos() const;
	void GetProgramGuideRect(RECT *pRect) const;
	void GetProgramGuideSize(SIZE *pSize) const;
	void GetPageSize(SIZE *pSize) const;
	void Scroll(int XOffset,int YOffset);
	void SetScrollPos(const POINT &Pos);
	void SetScrollBar();
	int GetTimePos() const;
	bool SetTimePos(int Pos);
	void StoreTimePos();
	void RestoreTimePos();
	void SetCaption();
	void SetTooltip();
	ProgramGuide::CEventItem *GetEventItem(int ListIndex,int EventIndex);
	const ProgramGuide::CEventItem *GetEventItem(int ListIndex,int EventIndex) const;
	bool GetEventRect(int ListIndex,int EventIndex,RECT *pRect) const;
	bool RedrawEvent(int ListIndex,int EventIndex);
	bool EventHitTest(int x,int y,int *pListIndex,int *pEventIndex,RECT *pItemRect=NULL) const;
	bool SelectEvent(int ListIndex,int EventIndex);
	bool SelectEventByPosition(int x,int y);
	bool SelectEventByIDs(WORD NetworkID,WORD TSID,WORD ServiceID,WORD EventID);
	void OnCommand(int id);
	void ShowPopupMenu(int x,int y);
	void AppendToolMenu(HMENU hmenu) const;
	bool ExecuteiEpgAssociate(const ProgramGuide::CServiceInfo *pServiceInfo,const CEventInfoData *pEventInfo);
	bool ExecuteTool(int Tool,const ProgramGuide::CServiceInfo *pServiceInfo,const CEventInfoData *pEventInfo);

// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};

namespace ProgramGuideBar
{

	class ABSTRACT_CLASS(CProgramGuideBar)
	{
	public:
		struct ThemeInfo {
			const CStatusView::ThemeInfo *pStatusTheme;
		};

		CProgramGuideBar(CProgramGuide *pProgramGuide);
		virtual ~CProgramGuideBar();
		virtual bool CreateBar(HWND hwndParent,DWORD Style) = 0;
		virtual bool IsBarCreated() const = 0;
		virtual void DestroyBar() = 0;
		virtual bool SetBarVisible(bool fVisible) = 0;
		bool IsBarVisible() const { return m_fVisible; }
		void EnableBufferedPaint(bool fEnable) { m_fUseBufferedPaint=fEnable; }
		virtual void GetBarSize(SIZE *pSize) = 0;
		virtual void SetBarPosition(int x,int y,int Width,int Height) = 0;
		virtual void SetTheme(const ThemeInfo &Theme) {}

		virtual void OnDateChanged() {}
		virtual void OnSpaceChanged() {}
		virtual void OnTimeRangeChanged() {}
		virtual void OnFavoritesChanged() {}
		virtual bool OnSetCursor(HWND hwnd,int HitTestCode) { return false; }
		virtual bool OnNotify(LPARAM lParam,LRESULT *pResult) { return false; }

	protected:
		CProgramGuide *m_pProgramGuide;
		bool m_fVisible;
		bool m_fUseBufferedPaint;
	};

}

class CProgramGuideFrameSettings;

class ABSTRACT_CLASS(CProgramGuideFrameBase) : public CProgramGuide::CFrame
{
public:
	enum {
		TOOLBAR_TUNER_MENU,
		TOOLBAR_DATE_MENU,
		TOOLBAR_FAVORITES,
		TOOLBAR_DATE,
		TOOLBAR_TIME,
		TOOLBAR_NUM
	};

	CProgramGuideFrameBase(CProgramGuide *pProgramGuide,CProgramGuideFrameSettings *pSettings);
	virtual ~CProgramGuideFrameBase() = 0;
	void SetStatusTheme(const CStatusView::ThemeInfo *pTheme);
	bool SetToolbarVisible(int Toolbar,bool fVisible);
	bool GetToolbarVisible(int Toolbar) const;

protected:
	CProgramGuide *m_pProgramGuide;
	CProgramGuideFrameSettings *m_pSettings;
	ProgramGuideBar::CProgramGuideBar *m_ToolbarList[TOOLBAR_NUM];
	RECT m_ToolbarMargin;
	POINT m_ToolbarGap;

// CProgramGuide::CFrame
	void OnDateChanged() override;
	void OnSpaceChanged() override;
	void OnListModeChanged() override;
	void OnTimeRangeChanged() override;
	void OnFavoritesChanged() override;
	bool OnCommand(int Command) override;
	void OnMenuInitialize(HMENU hmenu) override;

	LRESULT DefaultMessageHandler(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	void OnWindowCreate(HWND hwnd,bool fBufferedPaint);
	void OnWindowDestroy();
	void OnSizeChanged(int Width,int Height);
	virtual void OnLayoutChange() {}
};

class CProgramGuideFrameSettings : public CSettingsBase
{
public:
	CProgramGuideFrameSettings();

// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;

// CProgramGuideFrameSettings
	bool SetToolbarVisible(int Toolbar,bool fVisible);
	bool GetToolbarVisible(int Toolbar) const;

private:
	struct ToolbarInfo {
		bool fVisible;
	};

	enum {
		TOOLBAR_STATUS_VISIBLE = 0x0001U
	};

	ToolbarInfo m_ToolbarList[CProgramGuideFrameBase::TOOLBAR_NUM];
};

class CProgramGuideFrame : public CProgramGuideFrameBase, public CCustomWindow
{
public:
	static bool Initialize(HINSTANCE hinst);

	CProgramGuideFrame(CProgramGuide *pProgramGuide,CProgramGuideFrameSettings *pSettings);
	~CProgramGuideFrame();
	CProgramGuide *GetProgramGuide() { return m_pProgramGuide; }
// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;
// CProgramGuide::CFrame
	bool SetAlwaysOnTop(bool fTop) override;
	bool GetAlwaysOnTop() const override { return m_fAlwaysOnTop; }
// CProgramGuideFrame
	bool Show();

private:
	CAeroGlass m_AeroGlass;
	bool m_fAlwaysOnTop;

// CProgramGuideFrameBase
	void OnLayoutChange() override;
// CProgramGuide::CFrame
	void SetCaption(LPCTSTR pszCaption) override;
// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
// CProgramGuideFrame
	void SetAeroGlass();

	static const LPCTSTR m_pszWindowClass;
	static HINSTANCE m_hinst;
};

class CProgramGuideDisplay : public CProgramGuideFrameBase, public CDisplayView
{
public:
	class ABSTRACT_CLASS(CEventHandler) {
	protected:
		class CProgramGuideDisplay *m_pProgramGuideDisplay;
	public:
		CEventHandler();
		virtual ~CEventHandler() = 0;
		virtual bool OnHide() { return true; }
		virtual bool SetAlwaysOnTop(bool fTop) = 0;
		virtual bool GetAlwaysOnTop() const = 0;
		virtual void OnRButtonDown(int x,int y) {}
		virtual void OnLButtonDoubleClick(int x,int y) {}
		friend class CProgramGuideDisplay;
	};

	static bool Initialize(HINSTANCE hinst);

	CProgramGuideDisplay(CProgramGuide *pProgramGuide,CProgramGuideFrameSettings *pSettings);
	~CProgramGuideDisplay();
// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;
// CProgramGuideDisplay
	void SetEventHandler(CEventHandler *pHandler);

private:
	CEventHandler *m_pEventHandler;

	static const LPCTSTR m_pszWindowClass;
	static HINSTANCE m_hinst;

// CDisplayView
	bool Close() override;
	bool OnVisibleChange(bool fVisible) override;
// CProgramGuide::CFrame
	bool SetAlwaysOnTop(bool fTop) override;
	bool GetAlwaysOnTop() const override;
// CProgramGuideFrameBase
	void OnLayoutChange() override;
// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};


#endif
