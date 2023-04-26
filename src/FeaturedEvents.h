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


#ifndef TVTEST_FEATURED_EVENTS_H
#define TVTEST_FEATURED_EVENTS_H


#include <vector>
#include <memory>
#include "ChannelList.h"
#include "ProgramSearch.h"
#include "Settings.h"
#include "Dialog.h"


namespace TVTest
{

	class CFeaturedEventsSettings
		: public CSettingsBase
	{
	public:
		enum class SortType {
			Time,
			Service,
			TVTEST_ENUM_CLASS_TRAILER
		};

		static constexpr int MAX_EVENT_TEXT_LINES = 10;

		CFeaturedEventsSettings();

	// CSettingsBase
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	// CFeaturedEventsSettings
		CEventSearchServiceList &GetDefaultServiceList() { return m_DefaultServiceList; }
		const CEventSearchServiceList &GetDefaultServiceList() const { return m_DefaultServiceList; }
		CEventSearchSettingsList &GetSearchSettingsList() { return m_SearchSettingsList; }
		const CEventSearchSettingsList &GetSearchSettingsList() const { return m_SearchSettingsList; }
		SortType GetSortType() const { return m_SortType; }
		void SetSortType(SortType Type) { m_SortType = Type; }
		int GetPeriodSeconds() const { return m_PeriodSeconds; }
		void SetPeriodSeconds(int Period) { m_PeriodSeconds = Period; }
		bool GetShowEventText() const { return m_fShowEventText; }
		void SetShowEventText(bool fShow) { m_fShowEventText = fShow; }
		int GetEventTextLines() const { return m_EventTextLines; }
		void SetEventTextLines(int Lines) { m_EventTextLines = Lines; }

	private:
		CEventSearchServiceList m_DefaultServiceList;
		CEventSearchSettingsList m_SearchSettingsList;
		SortType m_SortType = SortType::Time;
		int m_PeriodSeconds = 24 * 60 * 60;
		bool m_fShowEventText = true;
		int m_EventTextLines = 2;
	};

	class CFeaturedEventsSearcher
	{
	public:
		CFeaturedEventsSearcher(const CFeaturedEventsSettings &Settings);

		void Clear();
		bool Update();
		size_t GetEventCount() const;
		const LibISDB::EventInfo *GetEventInfo(size_t Index) const;

	private:
		const CFeaturedEventsSettings &m_Settings;
		std::vector<std::unique_ptr<LibISDB::EventInfo>> m_EventList;
	};

	class CFeaturedEventsMatcher
	{
	public:
		bool BeginMatching(const CFeaturedEventsSettings &Settings);
		void EndMatching();
		bool IsMatch(const LibISDB::EventInfo &EventInfo);

	private:
		CEventSearchServiceList m_DefaultServiceList;
		std::vector<std::unique_ptr<CEventSearcher>> m_SearcherList;
	};

	class CFeaturedEventsDialog
		: public CResizableDialog
	{
	public:
		CFeaturedEventsDialog(
			CFeaturedEventsSettings &Settings,
			CEventSearchOptions &Options);

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
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		void SetSearchSettingsListItemStatus(HWND hDlg);
		int AddSearchSettingsItem(HWND hDlg, const CEventSearchSettings &Settings);
		void UpdateSearchSettingsItem(HWND hDlg, int Item);

		CFeaturedEventsSettings &m_Settings;
		CEventSearchOptions &m_Options;
		CChannelList m_ServiceList;
		int m_SettingsColumnWidth[NUM_SETTINGS_COLUMNS];
	};

	class CFeaturedEvents
		: public CSettingsBase
	{
	public:
		class ABSTRACT_CLASS(CEventHandler)
		{
		public:
			virtual void OnFeaturedEventsSettingsChanged(CFeaturedEvents & FeaturedEvents) {}
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

}	// namespace TVTest


#endif
