#ifndef TVTEST_TS_PROCESSOR_OPTIONS_H
#define TVTEST_TS_PROCESSOR_OPTIONS_H


#include "TSProcessorManager.h"
#include "Options.h"
#include "ListView.h"
#include <vector>
#include <map>


namespace TVTest
{

	class CTSProcessorOptions : public COptions
	{
	public:
		CTSProcessorOptions(CTSProcessorManager &TSProcessorManager);
		~CTSProcessorOptions();

	// CSettingsBase
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	// CBasicDialog
		bool Create(HWND hwndOwner) override;

	private:
		typedef CTSProcessor::ModuleInfo ModuleInfo;

		class CTunerMapDialog : public CBasicDialog
		{
		public:
			CTunerMapDialog(CTSProcessorOptions *pOptions,
							CTSProcessorManager::TunerFilterInfo *pInfo);
			bool Show(HWND hwndOwner) override;

		private:
			CTSProcessorOptions *m_pOptions;
			CTSProcessorManager::TunerFilterInfo *m_pInfo;

			INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
		};

		class CNetworkMapDialog : public CBasicDialog
		{
		public:
			CNetworkMapDialog(CTSProcessorOptions *pOptions,
							  CTSProcessorManager::NetworkFilterInfo *pInfo);
			bool Show(HWND hwndOwner) override;

		private:
			CTSProcessorOptions *m_pOptions;
			CTSProcessorManager::NetworkFilterInfo *m_pInfo;

			INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
		};

		static const UINT WM_APP_UPDATEDEVICEFILTERLIST=WM_APP;

		CTSProcessorManager &m_TSProcessorManager;
		std::vector<CTSProcessorManager::CTSProcessorSettings*> m_SettingsList;
		CTSProcessorManager::CTSProcessorSettings *m_pCurSettings;
		CListView m_TunerMapListView;
		CListView m_NetworkMapListView;
		std::vector<String> m_ModuleList;
		std::map<String,ModuleInfo,StringFunctional::LessNoCase> m_ModuleInfoMap;

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

		const ModuleInfo *GetModuleInfo(const String &Name);
		void UpdateCurSettings();
		void UpdateItemsState();
		void UpdateDeviceFilterList(HWND hDlg,int ModuleID,int DeviceID,int FilterID);
		void UpdateTunerMapItem(int Index);
		void UpdateNetworkMapItem(int Index);
		bool TunerMapDialog(CTSProcessorManager::TunerFilterInfo *pInfo);
		bool NetworkMapDialog(CTSProcessorManager::NetworkFilterInfo *pInfo);
	};

}	// namespace TVTest


#endif
