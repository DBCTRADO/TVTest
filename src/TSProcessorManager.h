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


#ifndef TVTEST_TS_PROCESSOR_MANAGER_H
#define TVTEST_TS_PROCESSOR_MANAGER_H


#include "TSProcessor.h"
#include "Settings.h"
#include <vector>
#include <memory>


namespace TVTest
{

	class CTSProcessorManager
		: protected CTSProcessor::CEventHandler
	{
	public:
		struct FilterInfo
		{
			String Module;
			String Device;
			String Filter;

			bool operator==(const FilterInfo &op) const noexcept = default;
		};

		struct TunerFilterInfo
			: public FilterInfo
		{
			static constexpr WORD NID_INVALID  = 0xFFFF;
			static constexpr WORD TSID_INVALID = 0xFFFF;
			static constexpr WORD SID_INVALID  = 0xFFFF;

			bool fEnable = true;
			bool fEnableProcessing = true;
			String Tuner;
			WORD NetworkID = NID_INVALID;
			WORD TransportStreamID = TSID_INVALID;
			WORD ServiceID = SID_INVALID;

			bool IsNetworkIDEnabled() const { return NetworkID != NID_INVALID; }
			bool IsTransportStreamIDEnabled() const { return TransportStreamID != TSID_INVALID; }
			bool IsServiceIDEnabled() const { return ServiceID != SID_INVALID; }
		};

		class CTSProcessorSettings
		{
		public:
			typedef CPropertyBag::PropertyListType PropertyList;

			GUID m_guid;
			std::optional<bool> m_EnableProcessing;
			FilterInfo m_DefaultFilter;
			std::vector<TunerFilterInfo> m_TunerFilterMap;
			PropertyList m_PropertyList;
			FilterInfo m_LastOpenFilter;
			bool m_fLastOpenFailed = false;

			CTSProcessorSettings(const GUID &guid);
			const TunerFilterInfo *GetTunerFilterInfo(
				LPCTSTR pszTuner,
				WORD NetworkID = TunerFilterInfo::NID_INVALID,
				WORD TransportStreamID = TunerFilterInfo::TSID_INVALID,
				WORD ServiceID = TunerFilterInfo::SID_INVALID) const;
			bool IsTunerFilterMapEnabled() const;
		};

		enum class FilterOpenFlag : unsigned int {
			None        = 0x0000U,
			RetryDialog = 0x0001U,
			NoUI        = 0x0002U,
			NotifyError = 0x0004U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		bool ReadSettings(CSettings &Settings);
		bool WriteSettings(CSettings &Settings) const;
		CTSProcessorSettings *GetTSProcessorSettings(const GUID &guid);
		const CTSProcessorSettings *GetTSProcessorSettings(const GUID &guid) const;
		bool SetTSProcessorSettings(CTSProcessorSettings *pSettings);
		bool ApplyTSProcessorSettings(const GUID &guid, bool fSetProperties = true);
		bool ApplyTSProcessorSettings(CTSProcessor *pTSProcessor, bool fSetProperties = true);
		bool SaveTSProcessorProperties(CTSProcessor *pTSProcessor);
		bool RegisterTSProcessor(
			CTSProcessor *pTSProcessor,
			CCoreEngine::TSProcessorConnectPosition ConnectPosition);
		CTSProcessor *GetTSProcessor(const GUID &guid) const;
		bool GetTSProcessorList(std::vector<CTSProcessor*> *pList) const;
		void OpenDefaultFilters(FilterOpenFlag FilterOpenFlags = FilterOpenFlag::None);
		void CloseAllFilters();
		void OnTunerChange(LPCTSTR pszOldTuner, LPCTSTR pszNewTuner);
		void OnTunerOpened(LPCTSTR pszTuner, FilterOpenFlag FilterOpenFlags = FilterOpenFlag::None);
		void OnNetworkChanged(
			LPCTSTR pszTuner,
			WORD NetworkID = TunerFilterInfo::NID_INVALID,
			WORD TransportStreamID = TunerFilterInfo::TSID_INVALID,
			WORD ServiceID = TunerFilterInfo::SID_INVALID,
			FilterOpenFlag FilterOpenFlags = FilterOpenFlag::None);

	private:
		std::vector<std::unique_ptr<CTSProcessorSettings>> m_SettingsList;

		bool ApplyTSProcessorSettings(CTSProcessor *pTSProcessor, const GUID &guid, bool fSetProperties = true);
		void OpenFilter(
			CTSProcessor *pTSProcessor, CTSProcessorSettings *pSettings,
			const FilterInfo &Filter, FilterOpenFlag FilterOpenFlags);
		void CloseFilter(CTSProcessor *pTSProcessor);

		// CTSProcessor::CEventHandler
		void OnFinalize(CTSProcessor *pTSProcessor) override;
		void OnNotify(
			CTSProcessor *pTSProcessor,
			Interface::NotifyType Type, LPCWSTR pszMessage) override;
	};

}	// namespace TVTest


#endif
