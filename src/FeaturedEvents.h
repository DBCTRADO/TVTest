#ifndef FEATURED_EVENTS_H
#define FEATURED_EVENTS_H


#include <vector>
#include "ChannelList.h"
#include "ProgramSearch.h"
#include "Settings.h"
#include "Dialog.h"


class CFeaturedEventsSettings : public CSettingsBase
{
public:
	enum SortType {
		SORT_TIME,
		SORT_SERVICE,
		SORT_FIRST=SORT_TIME,
		SORT_LAST =SORT_SERVICE
	};

	enum {
		MAX_EVENT_TEXT_LINES=10
	};

	CFeaturedEventsSettings();
	~CFeaturedEventsSettings();

// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;

// CFeaturedEventsSettings
	CEventSearchServiceList &GetDefaultServiceList() { return m_DefaultServiceList; }
	const CEventSearchServiceList &GetDefaultServiceList() const { return m_DefaultServiceList; }
	CEventSearchSettingsList &GetSearchSettingsList() { return m_SearchSettingsList; }
	const CEventSearchSettingsList &GetSearchSettingsList() const { return m_SearchSettingsList; }
	SortType GetSortType() const { return m_SortType; }
	void SetSortType(SortType Type) { m_SortType=Type; }
	int GetPeriodSeconds() const { return m_PeriodSeconds; }
	void SetPeriodSeconds(int Period) { m_PeriodSeconds=Period; }
	bool GetShowEventText() const { return m_fShowEventText; }
	void SetShowEventText(bool fShow) { m_fShowEventText=fShow; }
	int GetEventTextLines() const { return m_EventTextLines; }
	void SetEventTextLines(int Lines) { m_EventTextLines=Lines; }

private:
	CEventSearchServiceList m_DefaultServiceList;
	CEventSearchSettingsList m_SearchSettingsList;
	SortType m_SortType;
	int m_PeriodSeconds;
	bool m_fShowEventText;
	int m_EventTextLines;
};

class CFeaturedEventsSearcher
{
public:
	CFeaturedEventsSearcher(const CFeaturedEventsSettings &Settings);
	~CFeaturedEventsSearcher();
	void Clear();
	bool Update();
	size_t GetEventCount() const;
	const CEventInfoData *GetEventInfo(size_t Index) const;

private:
	const CFeaturedEventsSettings &m_Settings;
	std::vector<CEventInfoData*> m_EventList;
};

class CFeaturedEventsMatcher
{
public:
	bool BeginMatching(const CFeaturedEventsSettings &Settings);
	void EndMatching();
	bool IsMatch(const CEventInfoData &EventInfo);

private:
	CEventSearchServiceList m_DefaultServiceList;
	std::vector<CEventSearcher> m_SearcherList;
};

class CFeaturedEventsDialog : public CResizableDialog
{
public:
	CFeaturedEventsDialog(
		CFeaturedEventsSettings &Settings,
		CEventSearchOptions &Options);
	~CFeaturedEventsDialog();

// CBasicDialog
	bool Show(HWND hwndOwner) override;

private:
	enum {
		SETTINGS_COLUMN_NAME,
		SETTINGS_COLUMN_KEYWORD,
		SETTINGS_COLUMN_GENRE,
		NUM_SETTINGS_COLUMNS
	};

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	void SetSearchSettingsListItemStatus(HWND hDlg);
	int AddSearchSettingsItem(HWND hDlg,const CEventSearchSettings &Settings);
	void UpdateSearchSettingsItem(HWND hDlg,int Item);

	CFeaturedEventsSettings &m_Settings;
	CEventSearchOptions &m_Options;
	CChannelList m_ServiceList;
	int m_SettingsColumnWidth[NUM_SETTINGS_COLUMNS];
};

class CFeaturedEvents : public CSettingsBase
{
public:
	class ABSTRACT_CLASS(CEventHandler)
	{
	public:
		virtual void OnFeaturedEventsSettingsChanged(CFeaturedEvents &FeaturedEvents) {}
	};

	CFeaturedEvents(CEventSearchOptions &EventSearchOptions);

// CSettingsBase
	bool LoadSettings(CSettings &Settings) override;
	bool SaveSettings(CSettings &Settings) override;

// CFeaturedEvents
	CFeaturedEventsSettings &GetSettings() { return m_Settings; }
	bool AddEventHandler(CEventHandler *pEventHandler);
	bool RemoveEventHandler(CEventHandler *pEventHandler);
	bool ShowDialog(HWND hwndOwner);

private:
	CFeaturedEventsSettings m_Settings;
	CFeaturedEventsDialog m_Dialog;
	std::vector<CEventHandler*> m_EventHandlerList;
};


#endif
