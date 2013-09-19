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

class CFeaturedEvents
{
public:
	CFeaturedEvents(const CFeaturedEventsSettings &Settings);
	~CFeaturedEvents();
	void Clear();
	bool Update();
	size_t GetEventCount() const;
	const CEventInfoData *GetEventInfo(size_t Index) const;

private:
	const CFeaturedEventsSettings &m_Settings;
	std::vector<CEventInfoData*> m_EventList;
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


#endif
