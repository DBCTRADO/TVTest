#ifndef PROGRAM_SEARCH_H
#define PROGRAM_SEARCH_H


#include <vector>
#include <set>
#include <deque>
#include "Dialog.h"
#include "EpgProgramList.h"
#include "RichEditUtil.h"
#include "RegExp.h"
#include "Settings.h"


class CEventSearchServiceList
{
public:
	typedef ULONGLONG ServiceKey;

	static ServiceKey GetServiceKey(WORD NetworkID,WORD TSID,WORD ServiceID)
	{
		return ((ULONGLONG)NetworkID<<32) | ((TSID<<16) | ServiceID);
	};

	static WORD ServiceKey_GetNetworkID(ServiceKey Key)
	{
		return (WORD)(Key>>32);
	}

	static WORD ServiceKey_GetTransportStreamID(ServiceKey Key)
	{
		return (WORD)((Key>>16)&0xFFFF);
	}

	static WORD ServiceKey_GetServiceID(ServiceKey Key)
	{
		return (WORD)(Key&0xFFFF);
	}

	typedef std::set<ServiceKey>::const_iterator Iterator;

	void Clear();
	bool IsEmpty() const;
	size_t GetServiceCount() const;
	void Add(ServiceKey Key);
	void Add(WORD NetworkID,WORD TSID,WORD ServiceID);
	bool IsExists(ServiceKey Key) const;
	bool IsExists(WORD NetworkID,WORD TSID,WORD ServiceID) const;
	void Combine(const CEventSearchServiceList &List);
	Iterator Begin() const;
	Iterator End() const;
	bool ToString(TVTest::String *pString) const;
	bool FromString(LPCTSTR pszString);

private:
	static int EncodeServiceKey(ServiceKey Key,LPTSTR pText);
	static bool DecodeServiceKey(LPCTSTR pText,size_t Length,ServiceKey *pKey);

	std::set<ServiceKey> m_ServiceList;
};

class CEventSearchSettings
{
public:
	enum {
		MAX_NAME_LENGTH=256,
		MAX_KEYWORD_LENGTH=1024
	};

	struct TimeInfo {
		int Hour;
		int Minute;
	};

	enum CAType {
		CA_FREE,
		CA_CHARGEABLE
	};

	enum VideoType {
		VIDEO_HD,
		VIDEO_SD
	};

	bool fDisabled;
	TVTest::String Name;
	TVTest::String Keyword;
	bool fRegExp;
	bool fIgnoreCase;
	bool fIgnoreWidth;
	bool fGenre;
	WORD Genre1;
	WORD Genre2[16];
	bool fDayOfWeek;
	unsigned int DayOfWeekFlags;
	bool fTime;
	TimeInfo StartTime;
	TimeInfo EndTime;
	bool fDuration;
	unsigned int DurationShortest;
	unsigned int DurationLongest;
	bool fCA;
	CAType CA;
	bool fVideo;
	VideoType Video;
	bool fServiceList;
	CEventSearchServiceList ServiceList;

	CEventSearchSettings();
	void Clear();
	bool ToString(TVTest::String *pString) const;
	bool FromString(LPCTSTR pszString);

private:
	enum {
		FLAG_REG_EXP      = 0x00000001U,
		FLAG_IGNORE_CASE  = 0x00000002U,
		FLAG_IGNORE_WIDTH = 0x00000004U,
		FLAG_GENRE        = 0x00000008U,
		FLAG_DAY_OF_WEEK  = 0x00000010U,
		FLAG_TIME         = 0x00000020U,
		FLAG_DURATION     = 0x00000040U,
		FLAG_CA           = 0x00000080U,
		FLAG_VIDEO        = 0x00000100U,
		FLAG_SERVICE_LIST = 0x00000200U,
		FLAG_DISABLED     = 0x00000400U
	};

	static void ParseTime(LPCWSTR pszString,TimeInfo *pTime);
};

class CEventSearchSettingsList
{
public:
	CEventSearchSettingsList();
	~CEventSearchSettingsList();
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
	bool Load(CSettings &Settings,LPCTSTR pszPrefix);
	bool Save(CSettings &Settings,LPCTSTR pszPrefix) const;

private:
	std::vector<CEventSearchSettings*> m_List;
};

class CEventSearcher
{
public:
	CEventSearcher();
	bool InitializeRegExp();
	void Finalize();
	bool BeginSearch(const CEventSearchSettings &Settings);
	bool Match(const CEventInfoData *pEventInfo);
	int FindKeyword(LPCTSTR pszText,LPCTSTR pKeyword,int KeywordLength,int *pFoundLength=NULL) const;
	const CEventSearchSettings &GetSearchSettings() const { return m_Settings; }
	TVTest::CRegExp &GetRegExp() { return m_RegExp; }

private:
	CEventSearchSettings m_Settings;
	TVTest::CRegExp m_RegExp;
	decltype(FindNLSString) *m_pFindNLSString;

	bool MatchKeyword(const CEventInfoData *pEventInfo,LPCTSTR pszKeyword) const;
	bool MatchRegExp(const CEventInfoData *pEventInfo);
};

class CEventSearchOptions
{
public:
	CEventSearchOptions();

	bool SetKeywordHistory(const LPTSTR *pKeywordList,int NumKeywords);
	int GetKeywordHistoryCount() const;
	LPCTSTR GetKeywordHistory(int Index) const;
	bool AddKeywordHistory(LPCTSTR pszKeyword);
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
	bool LoadSearchSettings(CSettings &Settings,LPCTSTR pszPrefix);
	bool SaveSearchSettings(CSettings &Settings,LPCTSTR pszPrefix) const;

private:
	std::deque<TVTest::String> m_KeywordHistory;
	int m_MaxKeywordHistory;
	CEventSearchSettingsList m_SettingsList;
};

class CEventSearchSettingsDialog : public CResizableDialog
{
public:
	class ABSTRACT_CLASS(CEventHandler)
	{
	public:
		virtual void OnSearch() {}
		virtual void OnHighlightResult(bool fHighlight) {}
	};

	CEventSearchSettingsDialog(CEventSearchOptions &Options);
	~CEventSearchSettingsDialog();

// CBasicDialog
	bool Create(HWND hwndOwner) override;

// CEventSearchSettingsDialog
	bool GetSettings(CEventSearchSettings *pSettings) const;
	void SetSettings(const CEventSearchSettings &Settings);
	void SetEventHandler(CEventHandler *pEventHandler);
	bool BeginSearch();
	bool SetKeyword(LPCTSTR pszKeyword);
	bool AddToKeywordHistory(LPCTSTR pszKeyword);
	void ShowButton(int ID,bool fShow);
	void CheckButton(int ID,bool fCheck);
	void SetFocus(int ID);

private:
// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	void GetGenreSettings(CEventSearchSettings *pSettings) const;
	void SetGenreStatus();

	static LRESULT CALLBACK EditProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static const LPCTSTR m_pszPropName;

	CEventSearchSettings m_SearchSettings;
	CEventHandler *m_pEventHandler;
	CEventSearchOptions &m_Options;
	WNDPROC m_pOldEditProc;
	bool m_fGenreExpanded[16];
};

class CSearchEventInfo;

class CProgramSearchDialog
	: public CResizableDialog
	, public CEventSearchSettingsDialog::CEventHandler
{
public:
	enum {
		MAX_KEYWORD_HISTORY=50,
		NUM_COLUMNS=3
	};

	class CEventHandler
	{
	public:
		CEventHandler();
		virtual ~CEventHandler();
		bool Search(CEventSearcher *pSearcher);
		virtual bool OnSearch() = 0;
		virtual void OnEndSearch() {}
		virtual bool OnClose() { return true; }
		virtual bool OnLDoubleClick(const CEventInfoData *pEventInfo,LPARAM Param) { return false; }
		virtual bool OnRButtonClick(const CEventInfoData *pEventInfo,LPARAM Param) { return false; }
		virtual void OnHighlightChange(bool fHighlight) {}
		friend class CProgramSearchDialog;

	protected:
		class CProgramSearchDialog *m_pSearchDialog;
		CEventSearcher *m_pSearcher;

		bool AddSearchResult(const CEventInfoData *pEventInfo,LPCTSTR pszChannelName,LPARAM Param=0);
		bool Match(const CEventInfoData *pEventInfo) const;
	};

	CProgramSearchDialog(CEventSearchOptions &Options);
	~CProgramSearchDialog();
	bool Create(HWND hwndOwner);
	bool SetEventHandler(CEventHandler *pHandler);
	int GetColumnWidth(int Index) const;
	bool SetColumnWidth(int Index,int Width);
	bool Search(LPTSTR pszKeyword);
	bool SetHighlightResult(bool fHighlight);
	bool GetHighlightResult() const { return m_fHighlightResult; }
	bool IsHitEvent(const CEventInfoData *pEventInfo) const;
	const CEventSearchOptions &GetOptions() const { return m_Options; }
	CEventSearchOptions &GetOptions() { return m_Options; }

private:
	CEventHandler *m_pEventHandler;
	CEventSearchOptions &m_Options;
	CEventSearchSettings m_SearchSettings;
	CEventSearcher m_Searcher;
	CEventSearchSettingsDialog m_SearchSettingsDialog;
	std::map<ULONGLONG,CSearchEventInfo*> m_ResultMap;
	bool m_fHighlightResult;
	int m_SortColumn;
	bool m_fSortDescending;
	int m_ColumnWidth[NUM_COLUMNS];
	CRichEditUtil m_RichEditUtil;
	CHARFORMAT m_InfoTextFormat;

	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	bool AddSearchResult(const CEventInfoData *pEventInfo,LPCTSTR pszChannelName,LPARAM Param=0);
	void ClearSearchResult();
	void SortSearchResult();
	int FormatEventTimeText(const CEventInfoData *pEventInfo,LPTSTR pszText,int MaxLength) const;
	int FormatEventInfoText(const CEventInfoData *pEventInfo,LPTSTR pszText,int MaxLength) const;
	void HighlightKeyword();
	bool SearchNextKeyword(LPCTSTR *ppszText,LPCTSTR pKeyword,int KeywordLength,int *pLength) const;

// CEventSearchSettings::CEventHandler
	void OnSearch() override;
	void OnHighlightResult(bool fHighlight) override;

	static int CALLBACK ResultCompareFunc(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
};


#endif
