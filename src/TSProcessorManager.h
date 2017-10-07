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

			bool operator==(const FilterInfo &op) const {
				return Module == op.Module && Device == op.Device && Filter == op.Filter;
			}
			bool operator!=(const FilterInfo &op) const { return !(*this == op); }
		};

		struct TunerFilterInfo
			: public FilterInfo
		{
			bool fEnable;
			bool fEnableProcessing;
			String Tuner;
			WORD NetworkID;
			WORD TransportStreamID;
			WORD ServiceID;

			static const WORD NID_INVALID = 0xFFFF;
			static const WORD TSID_INVALID = 0xFFFF;
			static const WORD SID_INVALID = 0xFFFF;

			TunerFilterInfo()
				: fEnable(true)
				, fEnableProcessing(true)
				, NetworkID(NID_INVALID)
				, TransportStreamID(TSID_INVALID)
				, ServiceID(SID_INVALID)
			{
			}

			bool IsNetworkIDEnabled() const { return NetworkID != NID_INVALID; }
			bool IsTransportStreamIDEnabled() const { return TransportStreamID != TSID_INVALID; }
			bool IsServiceIDEnabled() const { return ServiceID != SID_INVALID; }
		};

		template<typename T> struct Optional
		{
			Optional() : m_fValid(false), m_Value(T()) {}
			Optional(const T &Value) : m_fValid(true), m_Value(Value) {}
			Optional<T> &operator=(const T &Value) { m_Value = Value; m_fValid = true; return *this; }
			operator bool() const { return m_fValid; }
			T &value() { return m_Value; }
			const T &value() const { return m_Value; }

		private:
			bool m_fValid;
			T m_Value;
		};

		class CTSProcessorSettings
		{
		public:
			typedef CPropertyBag::PropertyListType PropertyList;

			GUID m_guid;
			Optional<bool> m_EnableProcessing;
			FilterInfo m_DefaultFilter;
			std::vector<TunerFilterInfo> m_TunerFilterMap;
			PropertyList m_PropertyList;
			FilterInfo m_LastOpenFilter;
			bool m_fLastOpenFailed;

			CTSProcessorSettings(const GUID &guid);
			const TunerFilterInfo *GetTunerFilterInfo(
				LPCTSTR pszTuner,
				WORD NetworkID = TunerFilterInfo::NID_INVALID,
				WORD TransportStreamID = TunerFilterInfo::TSID_INVALID,
				WORD ServiceID = TunerFilterInfo::SID_INVALID) const;
			bool IsTunerFilterMapEnabled() const;
		};

		enum {
			FILTER_OPEN_RETRY_DIALOG = 0x0001U,
			FILTER_OPEN_NO_UI        = 0x0002U,
			FILTER_OPEN_NOTIFY_ERROR = 0x0004U
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
		void OpenDefaultFilters(unsigned int FilterOpenFlags = 0);
		void CloseAllFilters();
		void OnTunerChange(LPCTSTR pszOldTuner, LPCTSTR pszNewTuner);
		void OnTunerOpened(LPCTSTR pszTuner, unsigned int FilterOpenFlags = 0);
		void OnNetworkChanged(
			LPCTSTR pszTuner,
			WORD NetworkID = TunerFilterInfo::NID_INVALID,
			WORD TransportStreamID = TunerFilterInfo::TSID_INVALID,
			WORD ServiceID = TunerFilterInfo::SID_INVALID,
			unsigned int FilterOpenFlags = 0);

	private:
		std::vector<std::unique_ptr<CTSProcessorSettings>> m_SettingsList;

		bool ApplyTSProcessorSettings(CTSProcessor *pTSProcessor, const GUID &guid, bool fSetProperties = true);
		void OpenFilter(
			CTSProcessor *pTSProcessor, CTSProcessorSettings *pSettings,
			const FilterInfo &Filter, unsigned int FilterOpenFlags);
		void CloseFilter(CTSProcessor *pTSProcessor);

		// CTSProcessor::CEventHandler
		void OnFinalize(CTSProcessor *pTSProcessor) override;
		void OnNotify(
			CTSProcessor *pTSProcessor,
			Interface::NotifyType Type, LPCWSTR pszMessage) override;
	};

}	// namespace TVTest


#endif
