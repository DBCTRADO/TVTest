/*
  TVTest
  Copyright(c) 2008-2020 DBCTRADO

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


#ifndef TVTEST_PROGRAM_SEARCH_H
#define TVTEST_PROGRAM_SEARCH_H


#include <vector>
#include <set>
#include <deque>
#include <map>
#include <memory>
#include "Dialog.h"
#include "ChannelList.h"
#include "RichEditUtil.h"
#include "RegExp.h"
#include "Settings.h"
#include "WindowUtil.h"
#include "LibISDB/LibISDB/EPG/EventInfo.hpp"


namespace TVTest
{

	class CEventSearchServiceList
	{
	public:
		typedef ULONGLONG ServiceKey;

		static ServiceKey GetServiceKey(WORD NetworkID, WORD TSID, WORD ServiceID)
		{
			return (static_cast<ULONGLONG>(NetworkID) << 32) | ((TSID << 16) | ServiceID);
		};

		static WORD ServiceKey_GetNetworkID(ServiceKey Key)
		{
			return static_cast<WORD>(Key >> 32);
		}

		static WORD ServiceKey_GetTransportStreamID(ServiceKey Key)
		{
			return static_cast<WORD>((Key >> 16) & 0xFFFF);
		}

		static WORD ServiceKey_GetServiceID(ServiceKey Key)
		{
			return static_cast<WORD>(Key & 0xFFFF);
		}

		typedef std::set<ServiceKey>::const_iterator Iterator;

		void Clear();
		bool IsEmpty() const;
		size_t GetServiceCount() const;
		void Add(ServiceKey Key);
		void Add(WORD NetworkID, WORD TSID, WORD ServiceID);
		bool IsExists(ServiceKey Key) const;
		bool IsExists(WORD NetworkID, WORD TSID, WORD ServiceID) const;
		void Combine(const CEventSearchServiceList &List);
		Iterator Begin() const;
		Iterator End() const;
		bool ToString(String *pString) const;
		bool FromString(LPCTSTR pszString);

	private:
		static int EncodeServiceKey(ServiceKey Key, LPTSTR pText);
		static bool DecodeServiceKey(LPCTSTR pText, size_t Length, ServiceKey *pKey);

		std::set<ServiceKey> m_ServiceList;
	};

	class CEventSearchSettings
	{
	public:
		static constexpr size_t MAX_NAME_LENGTH = 256;
		static constexpr size_t MAX_KEYWORD_LENGTH = 1024;

		struct TimeInfo {
			int Hour;
			int Minute;
		};

		enum class CAType {
			Free,
			Chargeable,
		};

		enum class VideoType {
			HD,
			SD,
		};

		bool fDisabled = false;
		String Name;
		String Keyword;
		bool fRegExp = false;
		bool fIgnoreCase = true;
		bool fIgnoreWidth = true;
		bool fEventName = true;
		bool fEventText = true;
		bool fGenre = false;
		WORD Genre1 = 0x0000;
		WORD Genre2[16] = {};
		bool fDayOfWeek = false;
		unsigned int DayOfWeekFlags = 0x00;
		bool fTime = false;
		TimeInfo StartTime = {0, 0};
		TimeInfo EndTime = {23, 59};
		bool fDuration = false;
		unsigned int DurationShortest = 10 * 60;
		unsigned int DurationLongest = 0;
		bool fCA = false;
		CAType CA = CAType::Free;
		bool fVideo = false;
		VideoType Video = VideoType::HD;
		bool fServiceList = false;
		CEventSearchServiceList ServiceList;

		void Clear();
		bool ToString(String *pString) const;
		bool FromString(LPCTSTR pszString);

	private:
		enum class ConditionFlag : unsigned int {
			None        = 0x00000000U,
			RegExp      = 0x00000001U,
			IgnoreCase  = 0x00000002U,
			IgnoreWidth = 0x00000004U,
			Genre       = 0x00000008U,
			DayOfWeek   = 0x00000010U,
			Time        = 0x00000020U,
			Duration    = 0x00000040U,
			CA          = 0x00000080U,
			Video       = 0x00000100U,
			ServiceList = 0x00000200U,
			Disabled    = 0x00000400U,
			EventName   = 0x00000800U,
			EventText   = 0x00001000U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		static void ParseTime(LPCWSTR pszString, TimeInfo *pTime);
	};

	class CEventSearchSettingsList
	{
	public:
		CEventSearchSettingsList() = default;
		CEventSearchSettingsList(const CEventSearchSettingsList &Src);
		CEventSearchSettingsList &operator=(const CEventSearchSettingsList &Src);

		void Clear();
		size_t GetCount() const;
		size_t GetEnabledCount() const;
		CEventSearchSettings *Get(size_t Index);
		const CEventSearchSettings *Get(size_t Index) const;
		CEventSearchSettings *GetByName(LPCTSTR pszName);
		const CEventSearchSettings *GetByName(LPCTSTR pszName) const;
		bool Add(const CEventSearchSettings &Settings);
		bool Erase(size_t Index);
		int FindByName(LPCTSTR pszName) const;
		bool Load(CSettings &Settings, LPCTSTR pszPrefix);
		bool Save(CSettings &Settings, LPCTSTR pszPrefix) const;

	private:
		std::vector<std::unique_ptr<CEventSearchSettings>> m_List;
	};

	class CEventSearcher
	{
	public:
		bool InitializeRegExp();
		void Finalize();
		bool BeginSearch(const CEventSearchSettings &Settings);
		bool Match(const LibISDB::EventInfo *pEventInfo);
		int FindKeyword(LPCTSTR pszText, LPCTSTR pKeyword, int KeywordLength, int *pFoundLength = nullptr) const;
		bool FindExtendedText(const LibISDB::EventInfo::ExtendedTextInfoList &ExtendedText, LPCTSTR pKeyword, int KeywordLength) const;
		const CEventSearchSettings &GetSearchSettings() const { return m_Settings; }
		CRegExp &GetRegExp() { return m_RegExp; }

	private:
		CEventSearchSettings m_Settings;
		CRegExp m_RegExp;

		bool MatchKeyword(const LibISDB::EventInfo *pEventInfo, LPCTSTR pszKeyword) const;
		bool MatchRegExp(const LibISDB::EventInfo *pEventInfo);
	};

	class CEventSearchOptions
	{
	public:
		bool SetKeywordHistory(const LPTSTR *pKeywordList, int NumKeywords);
		bool SetKeywordHistory(const String *pKeywordList, size_t NumKeywords);
		int GetKeywordHistoryCount() const;
		LPCTSTR GetKeywordHistory(int Index) const;
		bool AddKeywordHistory(LPCTSTR pszKeyword);
		bool DeleteKeywordHistory(int Index);
		void ClearKeywordHistory();
		int GetMaxKeywordHistory() const { return m_MaxKeywordHistory; }
		bool SetMaxKeywordHistory(int Max);

		size_t GetSearchSettingsCount() const;
		CEventSearchSettings *GetSearchSettings(size_t Index);
		const CEventSearchSettings *GetSearchSettings(size_t Index) const;
		CEventSearchSettings *GetSearchSettingsByName(LPCTSTR pszName);
		const CEventSearchSettings *GetSearchSettingsByName(LPCTSTR pszName) const;
		void ClearSearchSettings();
		bool AddSearchSettings(const CEventSearchSettings &Settings);
		bool DeleteSearchSettings(size_t Index);
		int FindSearchSettings(LPCTSTR pszName) const;
		bool LoadSearchSettings(CSettings &Settings, LPCTSTR pszPrefix);
		bool SaveSearchSettings(CSettings &Settings, LPCTSTR pszPrefix) const;

	private:
		std::deque<String> m_KeywordHistory;
		int m_MaxKeywordHistory = 40;
		CEventSearchSettingsList m_SettingsList;
	};

	class CEventSearchSettingsDialog
		: public CResizableDialog
	{
	public:
		class ABSTRACT_CLASS(CEventHandler)
		{
		public:
			virtual void OnSearch() {}
			virtual void OnHighlightResult(bool fHighlight) {}
		};

		CEventSearchSettingsDialog(CEventSearchOptions &Options);

	// CBasicDialog
		bool Create(HWND hwndOwner) override;

	// CEventSearchSettingsDialog
		bool GetSettings(CEventSearchSettings *pSettings) const;
		void SetSettings(const CEventSearchSettings &Settings);
		void SetEventHandler(CEventHandler *pEventHandler);
		bool BeginSearch();
		bool SetKeyword(LPCTSTR pszKeyword);
		bool AddToKeywordHistory(LPCTSTR pszKeyword);
		void ShowButton(int ID, bool fShow);
		void CheckButton(int ID, bool fCheck);
		void SetFocus(int ID);
		void SetSearchTargetList(const LPCTSTR *ppszList, int Count);
		bool SetSearchTarget(int Target);
		int GetSearchTarget() const { return m_SearchTarget; }

	private:
		class CKeywordEditSubclass
			: public CWindowSubclass
		{
			LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		};

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		void GetGenreSettings(CEventSearchSettings *pSettings) const;
		void SetGenreStatus();

		static LRESULT CALLBACK EditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		enum {
			KEYWORDTARGET_EVENTNAME_AND_EVENTTEXT,
			KEYWORDTARGET_EVENTNAME,
			KEYWORDTARGET_EVENTTEXT,
		};

		CEventSearchSettings m_SearchSettings;
		CEventHandler *m_pEventHandler = nullptr;
		CEventSearchOptions &m_Options;
		CKeywordEditSubclass m_KeywordEditSubclass;
		bool m_fGenreExpanded[16] = {};
		std::vector<String> m_SearchTargetList;
		int m_SearchTarget = 0;
	};

	class CSearchEventInfo
		: public LibISDB::EventInfo
	{
	public:
		CSearchEventInfo(
			const LibISDB::EventInfo &EventInfo,
			const CTunerChannelInfo &ChannelInfo);

		const CTunerChannelInfo &GetChannelInfo() const { return m_ChannelInfo; }

	protected:
		CTunerChannelInfo m_ChannelInfo;
	};

	class CProgramSearchDialog
		: public CResizableDialog
		, public CEventSearchSettingsDialog::CEventHandler
	{
	public:
		static constexpr int MAX_KEYWORD_HISTORY = 50;

		enum {
			COLUMN_CHANNEL,
			COLUMN_TIME,
			COLUMN_EVENTNAME,
			NUM_COLUMNS
		};

		class CEventHandler
		{
		public:
			virtual ~CEventHandler();
			bool Search(CEventSearcher *pSearcher);
			virtual bool OnSearch() = 0;
			virtual void OnEndSearch() {}
			virtual bool OnClose() { return true; }
			virtual bool OnLDoubleClick(const CSearchEventInfo *pEventInfo) { return false; }
			virtual bool OnRButtonClick(const CSearchEventInfo *pEventInfo) { return false; }
			virtual void OnHighlightChange(bool fHighlight) {}
			friend class CProgramSearchDialog;

		protected:
			class CProgramSearchDialog *m_pSearchDialog = nullptr;
			CEventSearcher *m_pSearcher = nullptr;

			bool AddSearchResult(CSearchEventInfo *pEventInfo);
			bool Match(const LibISDB::EventInfo *pEventInfo) const;
		};

		CProgramSearchDialog(CEventSearchOptions &Options);
		~CProgramSearchDialog();

		bool Create(HWND hwndOwner) override;
		bool SetEventHandler(CEventHandler *pHandler);
		int GetColumnWidth(int Index) const;
		bool SetColumnWidth(int Index, int Width);
		bool Search(LPCTSTR pszKeyword);
		bool SetHighlightResult(bool fHighlight);
		bool GetHighlightResult() const { return m_fHighlightResult; }
		bool IsHitEvent(const LibISDB::EventInfo *pEventInfo) const;
		const CEventSearchOptions &GetOptions() const { return m_Options; }
		CEventSearchOptions &GetOptions() { return m_Options; }
		void SetResultListHeight(int Height);
		int GetResultListHeight() const { return m_ResultListHeight; }
		void SetSearchTargetList(const LPCTSTR *ppszList, int Count);
		bool SetSearchTarget(int Target);
		int GetSearchTarget() const;

	private:
		CEventHandler *m_pEventHandler = nullptr;
		CEventSearchOptions &m_Options;
		CEventSearchSettings m_SearchSettings;
		CEventSearcher m_Searcher;
		CEventSearchSettingsDialog m_SearchSettingsDialog;
		std::map<ULONGLONG, CSearchEventInfo*> m_ResultMap;
		bool m_fHighlightResult = true;
		int m_SortColumn = -1;
		bool m_fSortDescending = false;
		int m_ColumnWidth[NUM_COLUMNS];
		int m_ResultListHeight = -1;
		bool m_fSplitterCursor = false;
		int m_SplitterDragPos;
		CRichEditUtil m_RichEditUtil;
		CRichEditLinkHandler m_RichEditLink;
		CHARFORMAT m_InfoTextFormat;

		static constexpr int MIN_PANE_HEIGHT = 16;

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		void OnDarkModeChanged(bool fDarkMode) override;

	// CUIBase
		void ApplyStyle() override;

		bool AddSearchResult(CSearchEventInfo *pEventInfo);
		void ClearSearchResult();
		void SortSearchResult();
		void UpdateEventInfoText();
		void SetEventInfoText(const LibISDB::EventInfo *pEventInfo);
		size_t FormatEventTimeText(const LibISDB::EventInfo *pEventInfo, LPTSTR pszText, size_t MaxLength) const;
		void FormatEventInfoText(const LibISDB::EventInfo *pEventInfo, String *pText) const;
		void HighlightKeyword();
		bool SearchNextKeyword(LPCTSTR *ppszText, LPCTSTR pKeyword, int KeywordLength, int *pLength) const;
		bool IsSplitterPos(int x, int y) const;
		void AdjustResultListHeight(int Height);

	// CEventSearchSettings::CEventHandler
		void OnSearch() override;
		void OnHighlightResult(bool fHighlight) override;

		static int CALLBACK ResultCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	};

}	// namespace TVTest


#endif
