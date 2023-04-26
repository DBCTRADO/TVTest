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


#ifndef TVTEST_TS_PROCESSOR_OPTIONS_H
#define TVTEST_TS_PROCESSOR_OPTIONS_H


#include "TSProcessorManager.h"
#include "Options.h"
#include "ListView.h"
#include <vector>
#include <map>
#include <memory>


namespace TVTest
{

	class CTSProcessorOptions
		: public COptions
	{
	public:
		CTSProcessorOptions(CTSProcessorManager &TSProcessorManager);

	// CSettingsBase
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	// CBasicDialog
		bool Create(HWND hwndOwner) override;

	private:
		typedef CTSProcessor::ModuleInfo ModuleInfo;

		class CTunerMapDialog
			: public CBasicDialog
		{
		public:
			CTunerMapDialog(
				CTSProcessorOptions *pOptions,
				CTSProcessorManager::TunerFilterInfo *pInfo);

			bool Show(HWND hwndOwner) override;

		private:
			CTSProcessorOptions *m_pOptions;
			CTSProcessorManager::TunerFilterInfo *m_pInfo;

			INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		};

		static constexpr UINT WM_APP_UPDATEDEVICEFILTERLIST = WM_APP;

		CTSProcessorManager &m_TSProcessorManager;
		std::vector<std::unique_ptr<CTSProcessorManager::CTSProcessorSettings>> m_SettingsList;
		CTSProcessorManager::CTSProcessorSettings *m_pCurSettings = nullptr;
		CListView m_TunerMapListView;
		std::vector<String> m_ModuleList;
		std::map<String, ModuleInfo, StringFunctional::LessNoCase> m_ModuleInfoMap;

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		const ModuleInfo *GetModuleInfo(const String &Name);
		void UpdateCurSettings();
		void UpdateItemsState();
		void UpdateDeviceFilterList(HWND hDlg, int ModuleID, int DeviceID, int FilterID);
		void UpdateTunerMapItem(int Index);
		bool TunerMapDialog(CTSProcessorManager::TunerFilterInfo *pInfo);
	};

}	// namespace TVTest


#endif
