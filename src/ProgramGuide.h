/*
  TVTest
  Copyright(c) 2008-2019 DBCTRADO

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


#ifndef TVTEST_PROGRAM_GUIDE_H
#define TVTEST_PROGRAM_GUIDE_H


#include "View.h"
#include "EpgUtil.h"
#include "ChannelList.h"
#include "UIBase.h"
#include "Theme.h"
#include "DrawUtil.h"
#include "TextDrawClient.h"
#include "ProgramSearch.h"
#include "ProgramGuideFavorites.h"
#include "ProgramGuideTool.h"
#include "FeaturedEvents.h"
#include "EventInfoPopup.h"
#include "Tooltip.h"
#include "StatusView.h"
#include "Settings.h"
#include "WindowUtil.h"
#include "GUIUtil.h"
#include "LibISDB/LibISDB/EPG/EPGDatabase.hpp"


namespace TVTest
{

	namespace ProgramGuide
	{

		class CEventItem;
		class CEventLayout;
		class CServiceList;

		class CEventLayoutList
		{
			struct EventLayoutDeleter { void operator()(CEventLayout *p) const; };
			std::vector<std::unique_ptr<CEventLayout, EventLayoutDeleter>> m_LayoutList;

		public:
			void Clear();
			size_t Length() const { return m_LayoutList.size(); }
			void Add(CEventLayout *pLayout);
			CEventLayout *operator[](size_t Index);
			const CEventLayout *operator[](size_t Index) const;
		};

		class CServiceInfo
		{
			CTunerChannelInfo m_ChannelInfo;
			LibISDB::EPGDatabase::ServiceInfo m_ServiceInfo;
			HBITMAP m_hbmLogo;
			DrawUtil::CBitmap m_StretchedLogo;
			std::vector<std::unique_ptr<LibISDB::EventInfo>> m_EventList;
			typedef std::map<WORD, LibISDB::EventInfo*> EventIDMap;
			EventIDMap m_EventIDMap;

		public:
			CServiceInfo(const CChannelInfo &ChannelInfo, LPCTSTR pszBonDriver);

			CServiceInfo(const CServiceInfo &) = delete;
			CServiceInfo &operator=(const CServiceInfo &) = delete;

			const CTunerChannelInfo &GetChannelInfo() const { return m_ChannelInfo; }
			const LibISDB::EPGDatabase::ServiceInfo &GetServiceInfo() const { return m_ServiceInfo; }
			WORD GetNetworkID() const { return m_ServiceInfo.NetworkID; }
			WORD GetTSID() const { return m_ServiceInfo.TransportStreamID; }
			WORD GetServiceID() const { return m_ServiceInfo.ServiceID; }
			LPCTSTR GetServiceName() const { return m_ChannelInfo.GetName(); }
			void SetLogo(HBITMAP hbm) { m_hbmLogo = hbm; }
			HBITMAP GetLogo() const { return m_hbmLogo; }
			HBITMAP GetStretchedLogo(int Width, int Height);
			int NumEvents() const { return (int)m_EventList.size(); }
			LibISDB::EventInfo *GetEvent(int Index);
			const LibISDB::EventInfo *GetEvent(int Index) const;
			LibISDB::EventInfo *GetEventByEventID(WORD EventID);
			const LibISDB::EventInfo *GetEventByEventID(WORD EventID) const;
			bool AddEvent(LibISDB::EventInfo *pEvent);
			void ClearEvents();
			void CalcLayout(
				CEventLayout *pEventList, const CServiceList *pServiceList,
				const LibISDB::DateTime &FirstTime, const LibISDB::DateTime &LastTime, int LinesPerHour);
			bool SaveiEpgFile(const LibISDB::EventInfo *pEventInfo, LPCTSTR pszFileName, bool fVersion2) const;
		};

		class CServiceList
		{
			std::vector<std::unique_ptr<CServiceInfo>> m_ServiceList;

		public:
			size_t NumServices() const { return m_ServiceList.size(); }
			CServiceInfo *GetItem(size_t Index);
			const CServiceInfo *GetItem(size_t Index) const;
			CServiceInfo *GetItemByIDs(WORD TransportStreamID, WORD ServiceID);
			const CServiceInfo *GetItemByIDs(WORD TransportStreamID, WORD ServiceID) const;
			int FindItemByIDs(WORD TransportStreamID, WORD ServiceID) const;
			LibISDB::EventInfo *GetEventByIDs(WORD TransportStreamID, WORD ServiceID, WORD EventID);
			const LibISDB::EventInfo *GetEventByIDs(WORD TransportStreamID, WORD ServiceID, WORD EventID) const;
			void Add(CServiceInfo *pInfo);
			void Clear();
		};

	}

	// 番組表のチャンネル一覧取得クラス
	class ABSTRACT_CLASS(CProgramGuideChannelProvider)
	{
	public:
		virtual ~CProgramGuideChannelProvider() = default;

		virtual bool Update();
		virtual bool GetName(LPTSTR pszName, int MaxName) const = 0;
		virtual size_t GetGroupCount() const = 0;
		virtual bool GetGroupName(size_t Group, LPTSTR pszName, int MaxName) const = 0;
		virtual bool GetGroupID(size_t Group, String * pID) const = 0;
		virtual int ParseGroupID(LPCTSTR pszID) const = 0;
		virtual size_t GetChannelCount(size_t Group) const = 0;
		virtual const CChannelInfo *GetChannelInfo(size_t Group, size_t Channel) const = 0;
		virtual bool GetBonDriver(LPTSTR pszFileName, int MaxLength) const = 0;
		virtual bool GetBonDriverFileName(size_t Group, size_t Channel, LPTSTR pszFileName, int MaxLength) const = 0;
	};

	class CProgramGuideBaseChannelProvider
		: public CProgramGuideChannelProvider
	{
	public:
		CProgramGuideBaseChannelProvider();
		CProgramGuideBaseChannelProvider(const CTuningSpaceList *pSpaceList, LPCTSTR pszBonDriver);
		~CProgramGuideBaseChannelProvider();

	// CProgramGuideChannelProvider
		virtual bool GetName(LPTSTR pszName, int MaxName) const override;
		virtual size_t GetGroupCount() const override;
		virtual bool GetGroupName(size_t Group, LPTSTR pszName, int MaxName) const override;
		virtual bool GetGroupID(size_t Group, String *pID) const override;
		virtual int ParseGroupID(LPCTSTR pszID) const override;
		virtual size_t GetChannelCount(size_t Group) const override;
		virtual const CChannelInfo *GetChannelInfo(size_t Group, size_t Channel) const override;
		virtual bool GetBonDriver(LPTSTR pszFileName, int MaxLength) const override;
		virtual bool GetBonDriverFileName(size_t Group, size_t Channel, LPTSTR pszFileName, int MaxLength) const override;

	// CProgramGuideBaseChannelProvider
		bool SetTuningSpaceList(const CTuningSpaceList *pList);
		bool SetBonDriverFileName(LPCTSTR pszFileName);
		bool HasAllChannelGroup() const;

	protected:
		CTuningSpaceList m_TuningSpaceList;
		String m_BonDriverFileName;
	};

	// 番組表のチャンネル一覧取得管理クラス
	class ABSTRACT_CLASS(CProgramGuideChannelProviderManager)
	{
	public:
		virtual ~CProgramGuideChannelProviderManager() = default;

		virtual size_t GetChannelProviderCount() const = 0;
		virtual CProgramGuideChannelProvider *GetChannelProvider(size_t Index) const = 0;
	};

	class CProgramGuide
		: public CCustomWindow
		, public CUIBase
		, protected CDoubleBufferingDraw
		, protected CFeaturedEvents::CEventHandler
	{
	public:
		enum class ListMode {
			Services,
			Week,
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
			DAY_FIRST = DAY_TODAY,
			DAY_LAST = DAY_6DAYSAFTERTOMORROW
		};
		enum {
			COLOR_BACK,
			COLOR_CHANNELNAMETEXT,
			COLOR_CURCHANNELNAMETEXT,
			COLOR_TIMETEXT,
			COLOR_TIMELINE,
			COLOR_CURTIMELINE,
			COLOR_HIGHLIGHT_TITLE,
			COLOR_HIGHLIGHT_TEXT,
			COLOR_HIGHLIGHT_BACK,
			COLOR_HIGHLIGHT_BORDER,
			NUM_COLORS,
			COLOR_LAST = NUM_COLORS - 1
		};

		static constexpr int MIN_LINES_PER_HOUR = 8;
		static constexpr int MAX_LINES_PER_HOUR = 60;
		static constexpr int MIN_ITEM_WIDTH = 100;
		static constexpr int MAX_ITEM_WIDTH = 500;
		static constexpr int TIME_BAR_BACK_COLORS = 8;

		enum {
			FILTER_FREE         = 0x00000001U,
			FILTER_NEWPROGRAM   = 0x00000002U,
			FILTER_ORIGINAL     = 0x00000004U,
			FILTER_RERUN        = 0x00000008U,
			FILTER_NOT_SHOPPING = 0x00000010U,
			FILTER_GENRE_FIRST  = 0x00000100U,
			FILTER_GENRE_MASK   = 0x000FFF00U,
			FILTER_NEWS         = 0x00000100U,
			FILTER_SPORTS       = 0x00000200U,
			FILTER_INFORMATION  = 0x00000400U,
			FILTER_DRAMA        = 0x00000800U,
			FILTER_MUSIC        = 0x00001000U,
			FILTER_VARIETY      = 0x00002000U,
			FILTER_MOVIE        = 0x00004000U,
			FILTER_ANIME        = 0x00008000U,
			FILTER_DOCUMENTARY  = 0x00010000U,
			FILTER_THEATER      = 0x00020000U,
			FILTER_EDUCATION    = 0x00040000U,
			FILTER_WELFARE      = 0x00080000U
		};

		struct ServiceInfo
		{
			WORD NetworkID = 0;
			WORD TransportStreamID = 0;
			WORD ServiceID = 0;

			ServiceInfo() = default;
			ServiceInfo(WORD NID, WORD TSID, WORD SID)
				: NetworkID(NID), TransportStreamID(TSID), ServiceID(SID) {}
			void Clear() { *this = ServiceInfo(); }
		};

		typedef std::vector<ServiceInfo> ServiceInfoList;

		class ABSTRACT_CLASS(CFrame)
		{
		protected:
			CProgramGuide *m_pProgramGuide;

		public:
			CFrame();
			virtual ~CFrame();

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
			virtual void OnServiceTitleLButtonDown(LPCTSTR pszDriverFileName, const LibISDB::EPGDatabase::ServiceInfo * pServiceInfo) {}
			virtual bool OnRefresh() { return true; }
			virtual bool OnKeyDown(UINT KeyCode, UINT Flags) { return false; }
			virtual bool OnMenuInitialize(HMENU hmenu, UINT CommandBase) { return false; }
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
			virtual bool DrawBackground(
				const LibISDB::EventInfo & Event, HDC hdc,
				const RECT & ItemRect, const RECT & TitleRect, const RECT & ContentRect,
				COLORREF BackgroundColor) { return false; }
			virtual bool InitializeMenu(
				const LibISDB::EventInfo & Event, HMENU hmenu, UINT CommandBase,
				const POINT & CursorPos, const RECT & ItemRect) { return false; }
			virtual bool ProcessMenu(const LibISDB::EventInfo & Event, UINT Command) { return false; }
			virtual bool OnLButtonDoubleClick(
				const LibISDB::EventInfo & Event,
				const POINT & CursorPos, const RECT & ItemRect) { return false; }
			friend class CProgramGuide;
		};

		struct DateInfo
		{
			LibISDB::DateTime BeginningTime;
			LibISDB::DateTime EndTime;
			LPCTSTR pszRelativeDayText;
		};

		static bool Initialize(HINSTANCE hinst);

		CProgramGuide(CEventSearchOptions &EventSearchOptions);
		~CProgramGuide();

	// CBasicWindow
		bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;

	// CUIBase
		void SetStyle(const Style::CStyleManager *pStyleManager) override;
		void NormalizeStyle(
			const Style::CStyleManager *pStyleManager,
			const Style::CStyleScaling *pStyleScaling) override;
		void SetTheme(const Theme::CThemeManager *pThemeManager) override;

	// CProgramGuide
		bool SetEPGDatabase(LibISDB::EPGDatabase *pEPGDatabase);
		void Clear();
		bool Refresh();
		bool UpdateProgramGuide();
		bool SetChannelProviderManager(CProgramGuideChannelProviderManager *pManager);
		bool EnumChannelProvider(int Index, LPTSTR pszName, int MaxName) const;
		bool SetChannelProvider(int Provider, int Group);
		bool SetChannelProvider(int Provider, LPCTSTR pszGroupID);
		bool SetCurrentChannelProvider(int Provider, int Group);
		bool SetCurrentChannelProvider(int Provider, LPCTSTR pszGroupID);
		int GetCurrentChannelProvider() const { return m_CurrentChannelProvider; }
		int GetChannelGroupCount() const;
		bool GetChannelGroupName(int Group, LPTSTR pszName, int MaxName) const;
		int ParseChannelGroupID(LPCTSTR pszGroupID) const;
		bool SetCurrentChannelGroup(int Group);
		int GetCurrentChannelGroup() const { return m_CurrentChannelGroup; }
		bool GetChannelList(CChannelList *pList, bool fVisibleOnly) const;
		const ProgramGuide::CServiceList &GetServiceList() const { return m_ServiceList; }
		void SetCurrentService(WORD NetworkID, WORD TSID, WORD ServiceID);
		void ClearCurrentService() { SetCurrentService(0, 0, 0); }
		void SetCurrentEvent(WORD EventID);
		void ClearCurrentEvent() { SetCurrentEvent(0); }

		bool GetExcludeNoEventServices() const { return m_fExcludeNoEventServices; }
		bool SetExcludeNoEventServices(bool fExclude);
		bool SetExcludeServiceList(const ServiceInfoList &List);
		bool GetExcludeServiceList(ServiceInfoList *pList) const;
		bool IsExcludeService(WORD NetworkID, WORD TransportStreamID, WORD ServiceID) const;
		bool SetExcludeService(WORD NetworkID, WORD TransportStreamID, WORD ServiceID, bool fExclude);

		ListMode GetListMode() const { return m_ListMode; }
		bool SetServiceListMode();
		bool SetWeekListMode(int Service);
		int GetWeekListService() const { return m_WeekListService; }
		bool SetBeginHour(int Hour);
		bool SetTimeRange(const LibISDB::DateTime &FirstTime, const LibISDB::DateTime &LastTime);
		bool GetTimeRange(LibISDB::DateTime *pFirstTime, LibISDB::DateTime *pLastTime) const;
		bool GetCurrentTimeRange(LibISDB::DateTime *pFirstTime, LibISDB::DateTime *pLastTime) const;
		bool GetDayTimeRange(int Day, LibISDB::DateTime *pFirstTime, LibISDB::DateTime *pLastTime) const;
		bool GetCurrentDateInfo(DateInfo *pInfo) const;
		bool GetDateInfo(int Day, DateInfo *pInfo) const;
		bool ScrollToTime(const LibISDB::DateTime &Time, bool fHour = false);
		bool ScrollToCurrentTime();
		bool SetViewDay(int Day);
		int GetViewDay() const { return m_Day; }
		bool JumpEvent(WORD NetworkID, WORD TSID, WORD ServiceID, WORD EventID);
		bool JumpEvent(const LibISDB::EventInfo &EventInfo);
		bool ScrollToCurrentService();

		int GetLinesPerHour() const { return m_LinesPerHour; }
		int GetItemWidth() const { return m_ItemLogicalWidth; }
		bool SetUIOptions(int LinesPerHour, int ItemWidth);
		bool SetTextDrawEngine(CTextDrawClient::TextDrawEngine Engine);
		CTextDrawClient::TextDrawEngine GetTextDrawEngine() const { return m_TextDrawEngine; }
		bool SetDirectWriteRenderingParams(const CDirectWriteRenderer::RenderingParams &Params);
		bool SetFont(const Style::Font &Font);
		bool GetFont(Style::Font *pFont) const;
		bool SetEventInfoFont(const Style::Font &Font);
		bool SetShowToolTip(bool fShow);
		bool GetShowToolTip() const { return m_fShowToolTip; }
		void SetEventHandler(CEventHandler *pEventHandler);
		void SetFrame(CFrame *pFrame);
		void SetProgramCustomizer(CProgramCustomizer *pProgramCustomizer);
		int GetWheelScrollLines() const { return m_WheelScrollLines; }
		void SetWheelScrollLines(int Lines) { m_WheelScrollLines = Lines; }
		bool GetDragScroll() const { return m_fDragScroll; }
		bool SetDragScroll(bool fDragScroll);
		bool SetFilter(unsigned int Filter);
		unsigned int GetFilter() const { return m_Filter; }
		void SetVisibleEventIcons(UINT VisibleIcons);
		UINT GetVisibleEventIcons() const { return m_VisibleEventIcons; }
		bool GetKeepTimePos() const { return m_fKeepTimePos; }
		void SetKeepTimePos(bool fKeep);
		bool GetShowFeaturedMark() const { return m_fShowFeaturedMark; }
		void SetShowFeaturedMark(bool fShow);
		const Style::Margins &GetToolbarItemPadding() const;

		void GetInfoPopupSize(int *pWidth, int *pHeight) { m_EventInfoPopup.GetSize(pWidth, pHeight); }
		bool SetInfoPopupSize(int Width, int Height) { return m_EventInfoPopup.SetSize(Width, Height); }

		CProgramSearchDialog *GetProgramSearch() { return &m_ProgramSearch; }
		bool ShowProgramSearch(bool fShow);

		void SetMessage(LPCTSTR pszMessage, bool fUpdate = true);

		void OnEpgCaptureBegin();
		void OnEpgCaptureEnd();
		void SetEpgCaptureProgress(int Pos, int End, DWORD RemainingTime);

		CProgramGuideToolList *GetToolList() { return &m_ToolList; }
		CProgramGuideFavorites *GetFavorites() { return &m_Favorites; }

		bool OnCloseFrame();
		void OnShowFrame(bool fShow);

	private:
		struct ProgramGuideStyle
		{
			Style::IntValue ColumnMargin;
			Style::Margins HeaderPadding;
			Style::Margins HeaderChannelNameMargin;
			Style::Margins HeaderIconMargin;
			Style::Size HeaderChevronSize;
			Style::Margins HeaderChevronMargin;
			Style::IntValue HeaderShadowHeight;
			Style::IntValue EventLeading;
			Style::IntValue EventLineSpacing;
			bool fEventJustify;
			Style::Margins EventPadding;
			Style::Size EventIconSize;
			Style::Margins EventIconMargin;
			Style::Margins FeaturedMarkMargin;
			Style::Margins HighlightBorder;
			Style::Margins SelectedBorder;
			Style::Margins TimeBarPadding;
			Style::IntValue TimeBarShadowWidth;
			Style::IntValue CurTimeLineWidth;
			Style::Margins ToolbarItemPadding;

			ProgramGuideStyle();
			void SetStyle(const Style::CStyleManager *pStyleManager);
			void NormalizeStyle(
				const Style::CStyleManager *pStyleManager,
				const Style::CStyleScaling *pStyleScaling);
		};

		struct ProgramGuideTheme
		{
			Theme::ThemeColor ColorList[NUM_COLORS];
			Theme::FillStyle ChannelNameBackStyle;
			Theme::FillStyle CurChannelNameBackStyle;
			Theme::FillStyle TimeBarMarginStyle;
			Theme::FillStyle TimeBarBackStyle[TIME_BAR_BACK_COLORS];
			Theme::BackgroundStyle FeaturedMarkStyle;
		};

		LibISDB::EPGDatabase *m_pEPGDatabase;
		ProgramGuide::CServiceList m_ServiceList;
		ProgramGuide::CEventLayoutList m_EventLayoutList;
		ProgramGuideStyle m_Style;
		ListMode m_ListMode;
		int m_WeekListService;
		int m_LinesPerHour;
		Style::Font m_Font;
		DrawUtil::CFont m_ContentFont;
		DrawUtil::CFont m_TitleFont;
		DrawUtil::CFont m_TimeFont;
		CTextDrawClient::TextDrawEngine m_TextDrawEngine;
		CTextDrawClient m_TextDrawClient;
		int m_FontHeight;
		int m_GDIFontHeight;
		int m_LineMargin;
		int m_ItemLogicalWidth;
		int m_ItemWidth;
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
			bool fCursorMoved;
		} m_DragInfo;
		CMouseWheelHandler m_VertWheel;
		CMouseWheelHandler m_HorzWheel;
		Theme::IconList m_Chevron;
		CEpgIcons m_EpgIcons;
		UINT m_VisibleEventIcons;
		bool m_fBarShadow;
		CEventInfoPopup m_EventInfoPopup;
		CEventInfoPopupManager m_EventInfoPopupManager;
		class CEventInfoPopupHandler
			: public CEventInfoPopupManager::CEventHandler
			, public CEventInfoPopup::CEventHandler
		{
			CProgramGuide *m_pProgramGuide;

		// CEventInfoPopupManager::CEventHandler
			bool HitTest(int x, int y, LPARAM *pParam);
			bool ShowPopup(LPARAM Param, CEventInfoPopup *pPopup);

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
		WORD m_CurrentEventID;

		int m_BeginHour;
		LibISDB::DateTime m_FirstTime;
		LibISDB::DateTime m_LastTime;
		LibISDB::DateTime m_CurTime;
		int m_Day;
		int m_Hours;

		struct EventSelectInfo
		{
			bool fSelected;
			int ListIndex;
			int EventIndex;
		};
		EventSelectInfo m_CurEventItem;

		CEventHandler *m_pEventHandler;
		CFrame *m_pFrame;
		CProgramCustomizer *m_pProgramCustomizer;
		ProgramGuideTheme m_Theme;
		CEpgTheme m_EpgTheme;
		Theme::BackgroundStyle m_FeaturedMarkStyle;
		CProgramGuideToolList m_ToolList;
		int m_WheelScrollLines;
		unsigned int m_Filter;

		bool m_fEpgUpdating;
		struct {
			int Pos;
			int End;
			DWORD RemainingTime;
			void Clear() { Pos = 0; End = 0; RemainingTime = 0; }
		} m_EpgUpdateProgress;

		CProgramSearchDialog m_ProgramSearch;
		class CProgramSearchEventHandler
			: public CProgramSearchDialog::CEventHandler
		{
		public:
			CProgramSearchEventHandler(CProgramGuide *pProgramGuide);

			bool OnSearch() override;
			void OnEndSearch() override;
			bool OnClose() override;
			bool OnLDoubleClick(const CSearchEventInfo *pEventInfo) override;
			bool OnRButtonClick(const CSearchEventInfo *pEventInfo) override;
			void OnHighlightChange(bool fHighlight) override;

		private:
			void DoCommand(int Command, const CSearchEventInfo *pEventInfo);
			CProgramGuide *m_pProgramGuide;
		};
		CProgramSearchEventHandler m_ProgramSearchEventHandler;
		enum {
			SEARCH_TARGET_CURRENT,
			SEARCH_TARGET_ALL
		};

		String m_Message;

		CProgramGuideFavorites m_Favorites;

		bool m_fShowFeaturedMark;
		CFeaturedEventsMatcher m_FeaturedEventsMatcher;

		static const LPCTSTR m_pszWindowClass;
		static HINSTANCE m_hinst;

		static LPCTSTR GetRelativeDayText(int Day);

		bool UpdateList();
		bool UpdateService(ProgramGuide::CServiceInfo *pService);
		void UpdateServiceList();
		void CalcLayout();
		enum {
			EVENT_ITEM_STATUS_CURRENT     = 0x0001U,
			EVENT_ITEM_STATUS_HIGHLIGHTED = 0x0002U,
			EVENT_ITEM_STATUS_FILTERED    = 0x0004U,
			EVENT_ITEM_STATUS_COMMON      = 0x0008U
		};
		unsigned int GetEventItemStatus(const ProgramGuide::CEventItem *pItem, unsigned int Mask) const;
		void DrawEventBackground(
			ProgramGuide::CEventItem *pItem, HDC hdc, const RECT &Rect,
			Theme::CThemeDraw &ThemeDraw, CTextDraw &TextDraw, int LineHeight, int CurTimePos);
		void DrawEventText(
			ProgramGuide::CEventItem *pItem, HDC hdc, const RECT &Rect,
			Theme::CThemeDraw &ThemeDraw, CTextDraw &TextDraw, int LineHeight);
		void DrawEventList(
			ProgramGuide::CEventLayout *pLayout,
			HDC hdc, const RECT &Rect, const RECT &PaintRect,
			Theme::CThemeDraw &ThemeDraw, CTextDraw &TextDraw, bool fBackground);
		void DrawHeaderBackground(Theme::CThemeDraw &ThemeDraw, const RECT &Rect, bool fCur) const;
		void DrawServiceHeader(
			ProgramGuide::CServiceInfo *pServiceInfo,
			HDC hdc, const RECT &Rect, Theme::CThemeDraw &ThemeDraw,
			int Chevron, bool fLeftAlign = false);
		void DrawDayHeader(int Day, HDC hdc, const RECT &Rect, Theme::CThemeDraw &ThemeDraw) const;
		void DrawTimeBar(HDC hdc, const RECT &Rect, Theme::CThemeDraw &ThemeDraw, bool fRight);
		void Draw(HDC hdc, const RECT &PaintRect);
		void DrawMessage(HDC hdc, const RECT &ClientRect) const;
		bool CreateFonts();
		void CalcFontMetrics();
		int GetLineHeight() const;
		int CalcHeaderHeight() const;
		int GetCurTimeLinePos() const;
		void GetProgramGuideRect(RECT *pRect) const;
		void GetProgramGuideSize(SIZE *pSize) const;
		void GetPageSize(SIZE *pSize) const;
		void Scroll(int XOffset, int YOffset);
		void SetScrollPos(const POINT &Pos);
		void SetScrollBar();
		int GetTimePos() const;
		bool SetTimePos(int Pos);
		void StoreTimePos();
		void RestoreTimePos();
		void SetCaption();
		void SetTooltip();
		void ResetEventFont();
		void OnFontChanged();
		ProgramGuide::CEventItem *GetEventItem(int ListIndex, int EventIndex);
		const ProgramGuide::CEventItem *GetEventItem(int ListIndex, int EventIndex) const;
		bool GetEventRect(int ListIndex, int EventIndex, RECT *pRect) const;
		bool RedrawEvent(int ListIndex, int EventIndex);
		bool RedrawEventByIDs(WORD NetworkID, WORD TSID, WORD ServiceID, WORD EventID);
		bool EventHitTest(int x, int y, int *pListIndex, int *pEventIndex, RECT *pItemRect = nullptr) const;
		bool GetEventIndexByIDs(
			WORD NetworkID, WORD TSID, WORD ServiceID, WORD EventID,
			int *pListIndex, int *pEventIndex) const;
		bool SelectEvent(int ListIndex, int EventIndex);
		bool SelectEventByPosition(int x, int y);
		bool SelectEventByIDs(WORD NetworkID, WORD TSID, WORD ServiceID, WORD EventID);
		void OnCommand(int id);
		void ShowPopupMenu(int x, int y);
		void AppendToolMenu(HMENU hmenu) const;
		bool ExecuteiEpgAssociate(const ProgramGuide::CServiceInfo *pServiceInfo, const LibISDB::EventInfo *pEventInfo);
		bool ExecuteTool(int Tool, const ProgramGuide::CServiceInfo *pServiceInfo, const LibISDB::EventInfo *pEventInfo);

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void ApplyStyle() override;
		void RealizeStyle() override;

	// CFeaturedEvents::CEventHandler
		void OnFeaturedEventsSettingsChanged(CFeaturedEvents &FeaturedEvents) override;
	};

	namespace ProgramGuideBar
	{

		class ABSTRACT_CLASS(CProgramGuideBar)
		{
		public:
			struct ThemeInfo
			{
				CStatusView::StatusViewTheme StatusTheme;
				Theme::Style TimeStyle[CProgramGuide::TIME_BAR_BACK_COLORS];
			};

			CProgramGuideBar(CProgramGuide * pProgramGuide);
			virtual ~CProgramGuideBar();

			virtual bool CreateBar(HWND hwndParent, DWORD Style) = 0;
			virtual bool IsBarCreated() const = 0;
			virtual void DestroyBar() = 0;
			virtual bool SetBarVisible(bool fVisible) = 0;
			bool IsBarVisible() const { return m_fVisible; }
			void EnableBufferedPaint(bool fEnable) { m_fUseBufferedPaint = fEnable; }
			virtual void GetBarSize(SIZE * pSize) = 0;
			virtual void SetBarPosition(int x, int y, int Width, int Height) = 0;
			virtual void SetTheme(const ThemeInfo & Theme) {}
			virtual CUIBase *GetUIBase() = 0;

			virtual void OnDateChanged() {}
			virtual void OnSpaceChanged() {}
			virtual void OnTimeRangeChanged() {}
			virtual void OnFavoritesChanged() {}
			virtual bool OnSetCursor(HWND hwnd, int HitTestCode) { return false; }
			virtual bool OnNotify(LPARAM lParam, LRESULT * pResult) { return false; }

		protected:
			CProgramGuide *m_pProgramGuide;
			bool m_fVisible;
			bool m_fUseBufferedPaint;
		};

	}

	class CProgramGuideFrameSettings;

	class ABSTRACT_CLASS(CProgramGuideFrameBase)
		: public CProgramGuide::CFrame
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

		CProgramGuideFrameBase(CProgramGuide * pProgramGuide, CProgramGuideFrameSettings * pSettings);
		virtual ~CProgramGuideFrameBase();
		bool SetToolbarVisible(int Toolbar, bool fVisible);
		bool GetToolbarVisible(int Toolbar) const;
		void SetTheme(const Theme::CThemeManager * pThemeManager);

	protected:
		struct FrameStyle
		{
			Style::Margins ToolbarMargin;
			Style::IntValue ToolbarHorzGap;
			Style::IntValue ToolbarVertGap;
			bool fExtendFrame;

			FrameStyle();

			void SetStyle(const Style::CStyleManager * pStyleManager);
			void NormalizeStyle(
				const Style::CStyleManager * pStyleManager,
				const Style::CStyleScaling * pStyleScaling);
		};

		CProgramGuide * m_pProgramGuide;
		CProgramGuideFrameSettings * m_pSettings;
		ProgramGuideBar::CProgramGuideBar * m_ToolbarList[TOOLBAR_NUM];
		FrameStyle m_FrameStyle;
		int m_ToolbarRightMargin;
		bool m_fNoUpdateLayout;

	// CProgramGuide::CFrame
		void OnDateChanged() override;
		void OnSpaceChanged() override;
		void OnListModeChanged() override;
		void OnTimeRangeChanged() override;
		void OnFavoritesChanged() override;
		bool OnCommand(int Command) override;
		void OnMenuInitialize(HMENU hmenu) override;

		LRESULT DefaultMessageHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		void OnWindowCreate(
			HWND hwnd, Style::CStyleScaling * pStyleScaling, bool fBufferedPaint);
		void OnWindowDestroy();
		void OnSizeChanged(int Width, int Height);
		virtual void OnLayoutChange() {}
		virtual CUIBase *GetUIBase() = 0;
	};

	class CProgramGuideFrameSettings
		: public CSettingsBase
	{
	public:
		static constexpr int TOOLBAR_NUM = CProgramGuideFrameBase::TOOLBAR_NUM;
		static constexpr int DATEBAR_MAXBUTTONCOUNT = 8;
		static constexpr int DATEBAR_DEFAULTBUTTONCOUNT = 8;

		struct TimeBarSettings
		{
			enum class TimeType {
				Interval,
				Custom,
				TVTEST_ENUM_CLASS_TRAILER
			};

			static constexpr int INTERVAL_MIN    = 2;
			static constexpr int INTERVAL_MAX    = 12;
			static constexpr int BUTTONCOUNT_MIN = 1;
			static constexpr int BUTTONCOUNT_MAX = 20;

			TimeType Time;
			int Interval;
			String CustomTime;
			int MaxButtonCount;

			TimeBarSettings();
		};

		CProgramGuideFrameSettings();

	// CSettingsBase
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	// CProgramGuideFrameSettings
		LPCTSTR GetToolbarIDText(int Toolbar) const;
		LPCTSTR GetToolbarName(int Toolbar) const;
		bool SetToolbarVisible(int Toolbar, bool fVisible);
		bool GetToolbarVisible(int Toolbar) const;
		bool SetToolbarOrderList(const int *pOrder);
		bool GetToolbarOrderList(int *pOrder) const;
		bool SetDateBarButtonCount(int ButtonCount);
		int GetDateBarButtonCount() const { return m_DateBarButtonCount; }
		bool SetTimeBarSettings(const TimeBarSettings &Settings);
		const TimeBarSettings &GetTimeBarSettings() const { return m_TimeBarSettings; }

	private:
		struct ToolbarInfo
		{
			LPCTSTR pszIDText;
			LPCTSTR pszName;
		};

		struct ToolbarSettings
		{
			bool fVisible;
			int Order;
		};

		enum {
			TOOLBAR_STATUS_VISIBLE = 0x0001U
		};

		ToolbarSettings m_ToolbarSettingsList[TOOLBAR_NUM];

		int m_DateBarButtonCount;
		TimeBarSettings m_TimeBarSettings;

		static const ToolbarInfo m_ToolbarInfoList[TOOLBAR_NUM];

		int ParseIDText(LPCTSTR pszID) const;
	};

	class CProgramGuideFrame
		: public CProgramGuideFrameBase
		, public CCustomWindow
		, public CUIBase
	{
	public:
		static bool Initialize(HINSTANCE hinst);

		CProgramGuideFrame(CProgramGuide *pProgramGuide, CProgramGuideFrameSettings *pSettings);
		~CProgramGuideFrame();
		CProgramGuide *GetProgramGuide() { return m_pProgramGuide; }

	// CBasicWindow
		bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;

	// CProgramGuide::CFrame
		bool SetAlwaysOnTop(bool fTop) override;
		bool GetAlwaysOnTop() const override { return m_fAlwaysOnTop; }

	// CUIBase
		void SetTheme(const Theme::CThemeManager *pThemeManager) override;
		void SetStyle(const Style::CStyleManager *pStyleManager) override;
		void NormalizeStyle(
			const Style::CStyleManager *pStyleManager,
			const Style::CStyleScaling *pStyleScaling) override;

	// CProgramGuideFrame
		bool Show();

	private:
		Style::CStyleScaling m_StyleScaling;
		CAeroGlass m_AeroGlass;
		bool m_fAero;
		CUxTheme m_UxTheme;
		bool m_fAlwaysOnTop;
		bool m_fCreated;

	// CProgramGuideFrameBase
		void OnLayoutChange() override;
		CUIBase *GetUIBase() override { return this; }

	// CProgramGuide::CFrame
		void SetCaption(LPCTSTR pszCaption) override;

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void RealizeStyle() override;

	// CProgramGuideFrame
		void SetAeroGlass();
		void DrawBackground(HDC hdc, bool fActive);

		static const LPCTSTR m_pszWindowClass;
		static HINSTANCE m_hinst;
	};

	class CProgramGuideDisplay
		: public CProgramGuideFrameBase
		, public CDisplayView
	{
	public:
		class ABSTRACT_CLASS(CProgramGuideDisplayEventHandler)
			: public CDisplayView::CEventHandler
		{
		protected:
			class CProgramGuideDisplay * m_pProgramGuideDisplay;

		public:
			CProgramGuideDisplayEventHandler();

			virtual bool OnHide() { return true; }
			virtual bool SetAlwaysOnTop(bool fTop) = 0;
			virtual bool GetAlwaysOnTop() const = 0;
			friend class CProgramGuideDisplay;
		};

		static bool Initialize(HINSTANCE hinst);

		CProgramGuideDisplay(CProgramGuide *pProgramGuide, CProgramGuideFrameSettings *pSettings);
		~CProgramGuideDisplay();

	// CBasicWindow
		bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;

	// CUIBase
		void SetTheme(const Theme::CThemeManager *pThemeManager) override;
		void SetStyle(const Style::CStyleManager *pStyleManager) override;
		void NormalizeStyle(
			const Style::CStyleManager *pStyleManager,
			const Style::CStyleScaling *pStyleScaling) override;

	// CProgramGuideDisplay
		void SetEventHandler(CProgramGuideDisplayEventHandler *pHandler);

	private:
		CProgramGuideDisplayEventHandler *m_pProgramGuideDisplayEventHandler;

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
		CUIBase *GetUIBase() override { return this; }

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	};

}	// namespace TVTest


#endif
