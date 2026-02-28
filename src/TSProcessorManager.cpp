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


#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "TSProcessorManager.h"
#include "TSProcessorErrorDialog.h"
#include <algorithm>
#include <utility>
#include "Common/DebugDef.h"


namespace TVTest
{


bool CTSProcessorManager::ReadSettings(CSettings &Settings)
{
	int Count;

	if (Settings.Read(TEXT("SettingsCount"), &Count) && Count > 0) {
		String Buffer;

		for (int i = 0; i < Count; i++) {
			TCHAR szKey[64];
			GUID guid;

			StringFormat(szKey, TEXT("Processor{}.GUID"), i);
			if (Settings.Read(szKey, &Buffer)
					&& ::IIDFromString(Buffer.c_str(), &guid) == S_OK) {
				CTSProcessorSettings *pTSProcessorSettings = new CTSProcessorSettings(guid);
				bool f;

				StringFormat(szKey, TEXT("Processor{}.EnableProcessing"), i);
				if (Settings.Read(szKey, &f))
					pTSProcessorSettings->m_EnableProcessing = f;
				StringFormat(szKey, TEXT("Processor{}.DefaultModule"), i);
				Settings.Read(szKey, &pTSProcessorSettings->m_DefaultFilter.Module);
				StringFormat(szKey, TEXT("Processor{}.DefaultDevice"), i);
				Settings.Read(szKey, &pTSProcessorSettings->m_DefaultFilter.Device);
				StringFormat(szKey, TEXT("Processor{}.DefaultFilter"), i);
				Settings.Read(szKey, &pTSProcessorSettings->m_DefaultFilter.Filter);

#if 1
				// 旧仕様の設定を読み込み
				for (int j = 0;; j++) {
					TunerFilterInfo TunerDecInfo;
					int Value;

					StringFormat(szKey, TEXT("Processor{}.NetworkMap{}.NetworkID"), i, j);
					if (!Settings.Read(szKey, &Value))
						break;
					TunerDecInfo.NetworkID = static_cast<WORD>(Value);
					StringFormat(szKey, TEXT("Processor{}.NetworkMap{}.TSID"), i, j);
					if (Settings.Read(szKey, &Value))
						TunerDecInfo.TransportStreamID = static_cast<WORD>(Value);
					StringFormat(szKey, TEXT("Processor{}.NetworkMap{}.Enable"), i, j);
					Settings.Read(szKey, &TunerDecInfo.fEnable);
					StringFormat(szKey, TEXT("Processor{}.NetworkMap{}.EnableProcessing"), i, j);
					Settings.Read(szKey, &TunerDecInfo.fEnableProcessing);
					StringFormat(szKey, TEXT("Processor{}.NetworkMap{}.Module"), i, j);
					Settings.Read(szKey, &TunerDecInfo.Module);
					StringFormat(szKey, TEXT("Processor{}.NetworkMap{}.Device"), i, j);
					Settings.Read(szKey, &TunerDecInfo.Device);
					StringFormat(szKey, TEXT("Processor{}.NetworkMap{}.Filter"), i, j);
					Settings.Read(szKey, &TunerDecInfo.Filter);

					pTSProcessorSettings->m_TunerFilterMap.push_back(TunerDecInfo);
				}
#endif

				for (int j = 0;; j++) {
					TunerFilterInfo TunerDecInfo;
					unsigned int Value;

					StringFormat(szKey, TEXT("Processor{}.TunerMap{}.Enable"), i, j);
					if (!Settings.IsValueExists(szKey))
						break;
					Settings.Read(szKey, &TunerDecInfo.fEnable);
					StringFormat(szKey, TEXT("Processor{}.TunerMap{}.EnableProcessing"), i, j);
					Settings.Read(szKey, &TunerDecInfo.fEnableProcessing);
					StringFormat(szKey, TEXT("Processor{}.TunerMap{}.Module"), i, j);
					Settings.Read(szKey, &TunerDecInfo.Module);
					StringFormat(szKey, TEXT("Processor{}.TunerMap{}.Device"), i, j);
					Settings.Read(szKey, &TunerDecInfo.Device);
					StringFormat(szKey, TEXT("Processor{}.TunerMap{}.Filter"), i, j);
					Settings.Read(szKey, &TunerDecInfo.Filter);
					StringFormat(szKey, TEXT("Processor{}.TunerMap{}.Tuner"), i, j);
					Settings.Read(szKey, &TunerDecInfo.Tuner);
					StringFormat(szKey, TEXT("Processor{}.TunerMap{}.NetworkID"), i, j);
					if (Settings.Read(szKey, &Value))
						TunerDecInfo.NetworkID = static_cast<WORD>(Value);
					StringFormat(szKey, TEXT("Processor{}.TunerMap{}.TSID"), i, j);
					if (Settings.Read(szKey, &Value))
						TunerDecInfo.TransportStreamID = static_cast<WORD>(Value);
					StringFormat(szKey, TEXT("Processor{}.TunerMap{}.ServiceID"), i, j);
					if (Settings.Read(szKey, &Value))
						TunerDecInfo.ServiceID = static_cast<WORD>(Value);

					pTSProcessorSettings->m_TunerFilterMap.push_back(TunerDecInfo);
				}

				for (int j = 0;; j++) {
					String Name, Value;

					StringFormat(szKey, TEXT("Processor{}.Property{}.Name"), i, j);
					if (!Settings.Read(szKey, &Name))
						break;
					StringFormat(szKey, TEXT("Processor{}.Property{}.Value"), i, j);
					if (Settings.Read(szKey, &Value)) {
						CVariant Var;
						if (SUCCEEDED(Var.FromString(StringUtility::Decode(Value))))
							pTSProcessorSettings->m_PropertyList.emplace(Name, Var);
					}
				}

				SetTSProcessorSettings(pTSProcessorSettings);
			}
		}
	}

	return true;
}


bool CTSProcessorManager::WriteSettings(CSettings &Settings) const
{
	Settings.Clear();
	Settings.Write(TEXT("SettingsCount"), static_cast<int>(m_SettingsList.size()));

	for (int i = 0; i < static_cast<int>(m_SettingsList.size()); i++) {
		const CTSProcessorSettings *pTSProcessorSettings = m_SettingsList[i].get();
		TCHAR szKey[64], szBuffer[256];

		StringFormat(szKey, TEXT("Processor{}.GUID"), i);
		::StringFromGUID2(pTSProcessorSettings->m_guid, szBuffer, lengthof(szBuffer));
		Settings.Write(szKey, szBuffer);
		if (pTSProcessorSettings->m_EnableProcessing) {
			StringFormat(szKey, TEXT("Processor{}.EnableProcessing"), i);
			Settings.Write(szKey, pTSProcessorSettings->m_EnableProcessing.value());
		}
		StringFormat(szKey, TEXT("Processor{}.DefaultModule"), i);
		Settings.Write(szKey, pTSProcessorSettings->m_DefaultFilter.Module);
		StringFormat(szKey, TEXT("Processor{}.DefaultDevice"), i);
		Settings.Write(szKey, pTSProcessorSettings->m_DefaultFilter.Device);
		StringFormat(szKey, TEXT("Processor{}.DefaultFilter"), i);
		Settings.Write(szKey, pTSProcessorSettings->m_DefaultFilter.Filter);

		for (int j = 0; j < static_cast<int>(pTSProcessorSettings->m_TunerFilterMap.size()); j++) {
			const TunerFilterInfo &TunerDecInfo = pTSProcessorSettings->m_TunerFilterMap[j];

			StringFormat(szKey, TEXT("Processor{}.TunerMap{}.Enable"), i, j);
			Settings.Write(szKey, TunerDecInfo.fEnable);
			StringFormat(szKey, TEXT("Processor{}.TunerMap{}.EnableProcessing"), i, j);
			Settings.Write(szKey, TunerDecInfo.fEnableProcessing);
			StringFormat(szKey, TEXT("Processor{}.TunerMap{}.Module"), i, j);
			Settings.Write(szKey, TunerDecInfo.Module);
			StringFormat(szKey, TEXT("Processor{}.TunerMap{}.Device"), i, j);
			Settings.Write(szKey, TunerDecInfo.Device);
			StringFormat(szKey, TEXT("Processor{}.TunerMap{}.Filter"), i, j);
			Settings.Write(szKey, TunerDecInfo.Filter);
			StringFormat(szKey, TEXT("Processor{}.TunerMap{}.Tuner"), i, j);
			Settings.Write(szKey, TunerDecInfo.Tuner);
			if (TunerDecInfo.IsNetworkIDEnabled()) {
				StringFormat(szKey, TEXT("Processor{}.TunerMap{}.NetworkID"), i, j);
				Settings.Write(szKey, static_cast<unsigned int>(TunerDecInfo.NetworkID));
			}
			if (TunerDecInfo.IsTransportStreamIDEnabled()) {
				StringFormat(szKey, TEXT("Processor{}.TunerMap{}.TSID"), i, j);
				Settings.Write(szKey, static_cast<unsigned int>(TunerDecInfo.TransportStreamID));
			}
			if (TunerDecInfo.IsServiceIDEnabled()) {
				StringFormat(szKey, TEXT("Processor{}.TunerMap{}.ServiceID"), i, j);
				Settings.Write(szKey, static_cast<unsigned int>(TunerDecInfo.ServiceID));
			}
		}

		const CTSProcessorSettings::PropertyList &PropertyList = pTSProcessorSettings->m_PropertyList;
		int j = 0;
		for (auto it = PropertyList.begin(); it != PropertyList.end(); ++it) {
			String Value;
			if (SUCCEEDED(it->second.ToString(&Value))) {
				StringFormat(szKey, TEXT("Processor{}.Property{}.Name"), i, j);
				Settings.Write(szKey, it->first);
				StringFormat(szKey, TEXT("Processor{}.Property{}.Value"), i, j);
				Settings.Write(szKey, StringUtility::Encode(Value, TEXT("\"")));
				j++;
			}
		}
	}

	return true;
}


CTSProcessorManager::CTSProcessorSettings *CTSProcessorManager::GetTSProcessorSettings(const GUID &guid)
{
	for (auto &e : m_SettingsList) {
		if (e->m_guid == guid)
			return e.get();
	}
	return nullptr;
}


const CTSProcessorManager::CTSProcessorSettings *CTSProcessorManager::GetTSProcessorSettings(const GUID &guid) const
{
	for (auto &e : m_SettingsList) {
		if (e->m_guid == guid)
			return e.get();
	}
	return nullptr;
}


bool CTSProcessorManager::SetTSProcessorSettings(CTSProcessorSettings *pSettings)
{
	if (pSettings == nullptr)
		return false;

	CTSProcessorSettings *pCurSettings = GetTSProcessorSettings(pSettings->m_guid);
	if (pCurSettings != nullptr) {
		*pCurSettings = std::move(*pSettings);
		delete pSettings;
	} else {
		m_SettingsList.emplace_back(pSettings);
	}

	return true;
}


bool CTSProcessorManager::ApplyTSProcessorSettings(const GUID &guid, bool fSetProperties)
{
	CTSProcessor *pTSProcessor = GetTSProcessor(guid);
	if (pTSProcessor == nullptr)
		return false;

	return ApplyTSProcessorSettings(pTSProcessor, guid, fSetProperties);
}


bool CTSProcessorManager::ApplyTSProcessorSettings(CTSProcessor *pTSProcessor, bool fSetProperties)
{
	if (pTSProcessor == nullptr)
		return false;

	GUID guid;
	if (!pTSProcessor->GetGuid(&guid))
		return false;

	return ApplyTSProcessorSettings(pTSProcessor, guid, fSetProperties);
}


bool CTSProcessorManager::ApplyTSProcessorSettings(
	CTSProcessor *pTSProcessor, const GUID &guid, bool fSetProperties)
{
	const CTSProcessorSettings *pSettings = GetTSProcessorSettings(guid);
	if (pSettings == nullptr)
		return false;

	if (pSettings->m_EnableProcessing)
		pTSProcessor->SetEnableProcessing(pSettings->m_EnableProcessing.value());

	if (fSetProperties
			&& !pSettings->m_PropertyList.empty()
			&& pTSProcessor->IsPropertyBagSupported()) {
		CPropertyBag *pPropBag = new CPropertyBag;
		for (const auto &e : pSettings->m_PropertyList) {
			CVariant var(e.second);
			pPropBag->Write(e.first.c_str(), &var);
		}
		pTSProcessor->LoadProperties(pPropBag);
		pPropBag->Release();
	}

	return true;
}


bool CTSProcessorManager::SaveTSProcessorProperties(CTSProcessor *pTSProcessor)
{
	if (pTSProcessor == nullptr)
		return false;

	bool fSaved = false;
	GUID guid;

	if (pTSProcessor->IsPropertyBagSupported()
			&& pTSProcessor->GetGuid(&guid)) {
		CTSProcessorSettings *pSettings = GetTSProcessorSettings(guid);

		if (pSettings != nullptr) {
			CPropertyBag *pPropBag = new CPropertyBag;

			if (pTSProcessor->SaveProperties(pPropBag)) {
				pSettings->m_PropertyList.clear();
				for (const auto &e : *pPropBag) {
					pSettings->m_PropertyList.insert(e);
				}
				fSaved = true;
			}

			pPropBag->Release();
		}
	}

	return fSaved;
}


bool CTSProcessorManager::RegisterTSProcessor(
	CTSProcessor *pTSProcessor, CCoreEngine::TSProcessorConnectPosition ConnectPosition)
{
	CAppMain &App = GetAppClass();

	if (!App.CoreEngine.RegisterTSProcessor(pTSProcessor, ConnectPosition))
		return false;

	pTSProcessor->SetEventHandler(this);

	GUID guid;
	if (pTSProcessor->GetGuid(&guid)) {
		if (!ApplyTSProcessorSettings(pTSProcessor, guid)) {
			std::vector<String> List;
			if (pTSProcessor->GetModuleList(&List) && List.size() == 1) {
				CTSProcessorSettings *pSettings = new CTSProcessorSettings(guid);
				pSettings->m_DefaultFilter.Module = std::move(List.front());
				m_SettingsList.emplace_back(pSettings);
			}
		}
	}

	return true;
}


CTSProcessor *CTSProcessorManager::GetTSProcessor(const GUID &guid) const
{
	CCoreEngine &CoreEngine = GetAppClass().CoreEngine;
	const size_t TSFilterCount = CoreEngine.GetTSProcessorCount();

	for (size_t i = 0; i < TSFilterCount; i++) {
		CTSProcessor *pTSProcessor = CoreEngine.GetTSProcessorByIndex(i);
		GUID ProcessorGuid;

		if (pTSProcessor != nullptr
				&& pTSProcessor->GetGuid(&ProcessorGuid)
				&& ProcessorGuid == guid)
			return pTSProcessor;
	}

	return nullptr;
}


bool CTSProcessorManager::GetTSProcessorList(std::vector<CTSProcessor*> *pList) const
{
	if (pList == nullptr)
		return false;

	pList->clear();

	CCoreEngine &CoreEngine = GetAppClass().CoreEngine;
	const size_t TSFilterCount = CoreEngine.GetTSProcessorCount();

	for (size_t i = 0; i < TSFilterCount; i++) {
		CTSProcessor *pTSProcessor = CoreEngine.GetTSProcessorByIndex(i);
		if (pTSProcessor != nullptr)
			pList->push_back(pTSProcessor);
	}

	return true;
}


void CTSProcessorManager::OpenDefaultFilters(FilterOpenFlag FilterOpenFlags)
{
	CCoreEngine &CoreEngine = GetAppClass().CoreEngine;
	const size_t TSFilterCount = CoreEngine.GetTSProcessorCount();

	for (size_t i = 0; i < TSFilterCount; i++) {
		CTSProcessor *pTSProcessor = CoreEngine.GetTSProcessorByIndex(i);
		if (pTSProcessor == nullptr)
			continue;

		GUID guid;
		if (!pTSProcessor->GetGuid(&guid))
			continue;

		CTSProcessorSettings *pSettings = GetTSProcessorSettings(guid);
		if (pSettings != nullptr
				&& !pSettings->IsTunerFilterMapEnabled()) {
			OpenFilter(
				pTSProcessor, pSettings,
				pSettings->m_DefaultFilter,
				FilterOpenFlags);
		}
	}
}


void CTSProcessorManager::CloseAllFilters()
{
	CCoreEngine &CoreEngine = GetAppClass().CoreEngine;
	const size_t TSFilterCount = CoreEngine.GetTSProcessorCount();

	for (size_t i = 0; i < TSFilterCount; i++) {
		CTSProcessor *pTSProcessor = CoreEngine.GetTSProcessorByIndex(i);
		if (pTSProcessor != nullptr)
			pTSProcessor->CloseFilter();
	}
}


void CTSProcessorManager::OnTunerChange(LPCTSTR pszOldTuner, LPCTSTR pszNewTuner)
{
	CCoreEngine &CoreEngine = GetAppClass().CoreEngine;
	const size_t TSFilterCount = CoreEngine.GetTSProcessorCount();

	for (size_t i = 0; i < TSFilterCount; i++) {
		CTSProcessor *pTSProcessor = CoreEngine.GetTSProcessorByIndex(i);
		if (pTSProcessor == nullptr)
			continue;

		GUID guid;
		if (!pTSProcessor->GetGuid(&guid))
			continue;

		const CTSProcessorSettings *pSettings = GetTSProcessorSettings(guid);
		if (pSettings == nullptr)
			continue;

		const TunerFilterInfo *pOldTunerDecInfo = pSettings->GetTunerFilterInfo(pszOldTuner);
		const TunerFilterInfo *pNewTunerDecInfo = pSettings->GetTunerFilterInfo(pszNewTuner);
		const FilterInfo *pNewFilter, *pOldFilter;

		if (pNewTunerDecInfo != nullptr && pNewTunerDecInfo->fEnable) {
			if (!pNewTunerDecInfo->fEnableProcessing) {
				CloseFilter(pTSProcessor);
				continue;
			}
			pNewFilter = pNewTunerDecInfo;
		} else {
			pNewFilter = &pSettings->m_DefaultFilter;
		}
		if (pOldTunerDecInfo != nullptr && pOldTunerDecInfo->fEnable) {
			if (!pOldTunerDecInfo->fEnableProcessing)
				continue;
			pOldFilter = pOldTunerDecInfo;
		} else {
			pOldFilter = &pSettings->m_DefaultFilter;
		}
		if (!StringUtility::IsEqualNoCase(pOldFilter->Device, pNewFilter->Device)
				|| !StringUtility::IsEqualNoCase(pOldFilter->Filter, pNewFilter->Filter))
			pTSProcessor->CloseFilter();
		if (!IsEqualFileName(pOldFilter->Module.c_str(), pNewFilter->Module.c_str()))
			pTSProcessor->UnloadModule();
	}
}


void CTSProcessorManager::OnTunerOpened(LPCTSTR pszTuner, FilterOpenFlag FilterOpenFlags)
{
	CAppMain &App = GetAppClass();
	CCoreEngine &CoreEngine = App.CoreEngine;
	const size_t TSFilterCount = CoreEngine.GetTSProcessorCount();

	for (size_t i = 0; i < TSFilterCount; i++) {
		CTSProcessor *pTSProcessor = CoreEngine.GetTSProcessorByIndex(i);
		if (pTSProcessor == nullptr)
			continue;

		GUID guid;
		if (!pTSProcessor->GetGuid(&guid))
			continue;

		CTSProcessorSettings *pSettings = GetTSProcessorSettings(guid);
		if (pSettings == nullptr || !pSettings->IsTunerFilterMapEnabled())
			continue;

		const TunerFilterInfo *pTunerDecInfo = pSettings->GetTunerFilterInfo(pszTuner);
		const FilterInfo *pFilter;

		if (pTunerDecInfo != nullptr && pTunerDecInfo->fEnable) {
			if (!pTunerDecInfo->fEnableProcessing) {
				CloseFilter(pTSProcessor);
				continue;
			}
			pFilter = pTunerDecInfo;
		} else {
			pFilter = &pSettings->m_DefaultFilter;
		}

		OpenFilter(pTSProcessor, pSettings, *pFilter, FilterOpenFlags);
	}
}


void CTSProcessorManager::OnNetworkChanged(
	LPCTSTR pszTuner, WORD NetworkID, WORD TransportStreamID, WORD ServiceID, FilterOpenFlag FilterOpenFlags)
{
	CAppMain &App = GetAppClass();
	CCoreEngine &CoreEngine = App.CoreEngine;
	const size_t TSFilterCount = CoreEngine.GetTSProcessorCount();

	for (size_t i = 0; i < TSFilterCount; i++) {
		CTSProcessor *pTSProcessor = CoreEngine.GetTSProcessorByIndex(i);
		if (pTSProcessor == nullptr)
			continue;

		GUID guid;
		if (!pTSProcessor->GetGuid(&guid))
			continue;

		CTSProcessorSettings *pSettings = GetTSProcessorSettings(guid);
		if (pSettings == nullptr)
			continue;

		const TunerFilterInfo *pTunerDecInfo =
			pSettings->GetTunerFilterInfo(pszTuner, NetworkID, TransportStreamID, ServiceID);
		const FilterInfo *pFilter;

		if (pTunerDecInfo != nullptr && pTunerDecInfo->fEnable) {
			if (!pTunerDecInfo->fEnableProcessing) {
				CloseFilter(pTSProcessor);
				continue;
			}
			pFilter = pTunerDecInfo;
		} else {
			pFilter = &pSettings->m_DefaultFilter;
		}

		OpenFilter(pTSProcessor, pSettings, *pFilter, FilterOpenFlags);
	}
}


void CTSProcessorManager::OpenFilter(
	CTSProcessor *pTSProcessor, CTSProcessorSettings *pSettings,
	const FilterInfo &Filter, FilterOpenFlag FilterOpenFlags)
{
	TRACE(
		TEXT("CTSProcessorManager::OpenFilter() : {} {} {}\n"),
		Filter.Module, Filter.Device, Filter.Filter);

	// オープンに失敗したフィルタを繰り返しオープンしようとするのを避けるため、
	// 前回オープンを試みたフィルタを記憶しておく
	if (pSettings->m_fLastOpenFailed && pSettings->m_LastOpenFilter == Filter)
		return;
	pSettings->m_LastOpenFilter = Filter;

	CAppMain &App = GetAppClass();

	if (pTSProcessor->IsFilterModuleSupported()) {
		if (!Filter.Module.empty()) {
			if (!pTSProcessor->IsModuleLoaded()
					|| !IsEqualFileName(pTSProcessor->GetModuleName().c_str(), Filter.Module.c_str())) {
				if (!pTSProcessor->LoadModule(Filter.Module.c_str())) {
					pSettings->m_fLastOpenFailed = true;
					App.AddLog(CLogItem::LogType::Error, TEXT("\"{}\" を読み込めません。"), Filter.Module);
					return;
				}

				CTSProcessor::FilterModuleInfo ModuleInfo;
				pTSProcessor->GetModuleInfo(&ModuleInfo);
				App.AddLog(
					TEXT("モジュール \"{}\" ({} {}) を読み込みました。"),
					Filter.Module, ModuleInfo.Name, ModuleInfo.Version);
			}
		} else {
			if (!pTSProcessor->IsModuleLoaded())
				return;
		}
	}

	String DeviceName(Filter.Device);

	if (DeviceName.empty()) {
		const int DeviceNo = pTSProcessor->GetDefaultDevice();
		if (DeviceNo < 0) {
			App.AddLog(CLogItem::LogType::Error, TEXT("TSフィルターのデフォルトデバイスがありません。"));
			return;
		}
		pTSProcessor->GetDeviceName(DeviceNo, &DeviceName);
	}

	const bool fResult = pTSProcessor->OpenFilter(DeviceName.c_str(), Filter.Filter.c_str());
	pSettings->m_fLastOpenFailed = !fResult;
	if (!fResult) {
		if (!Filter.Filter.empty())
			App.AddLog(CLogItem::LogType::Error, TEXT("TSフィルター \"{}\" : \"{}\" をオープンできません。"), DeviceName, Filter.Filter);
		else
			App.AddLog(CLogItem::LogType::Error, TEXT("TSフィルター \"{}\" をオープンできません。"), DeviceName);

		if (!!(FilterOpenFlags & FilterOpenFlag::RetryDialog)) {
			String Message;
			CTSProcessorErrorDialog Dialog(pTSProcessor);

			if (!IsStringEmpty(pTSProcessor->GetLastErrorText())) {
				Message = pTSProcessor->GetLastErrorText();
				Message += TEXT("\r\n");
			}
			if (!IsStringEmpty(pTSProcessor->GetLastErrorSystemMessage())) {
				Message += TEXT("(");
				Message += pTSProcessor->GetLastErrorSystemMessage();
				Message += TEXT(")\r\n");
			}

			Dialog.SetMessage(
				!Message.empty() ? Message.c_str() : TEXT("TSフィルターをオープンできません。"));
			Dialog.SetDevice(DeviceName);
			Dialog.SetFilter(Filter.Filter);

			while (Dialog.Show(App.UICore.GetDialogOwner())) {
				if (IsStringEmpty(Dialog.GetDevice()))
					break;
				pSettings->m_LastOpenFilter.Device = Dialog.GetDevice();
				pSettings->m_LastOpenFilter.Filter = Dialog.GetFilter();
				if (pTSProcessor->OpenFilter(Dialog.GetDevice(), Dialog.GetFilter())) {
					pSettings->m_fLastOpenFailed = false;
					break;
				}
			}
		} else {
			if (!!(FilterOpenFlags & FilterOpenFlag::NotifyError)) {
				App.UICore.GetSkin()->ShowNotificationBar(
					TEXT("TSフィルターをオープンできません。"),
					CNotificationBar::MessageType::Error, 6000);
			}

			if (!(FilterOpenFlags & FilterOpenFlag::NoUI)) {
				App.UICore.GetSkin()->ShowErrorMessage(pTSProcessor);
			}
		}
	}
}


void CTSProcessorManager::CloseFilter(CTSProcessor *pTSProcessor)
{
#if 0
	pTSProcessor->CloseFilter();
#else
	if (pTSProcessor->IsModuleLoaded()) {
		String ModuleName = pTSProcessor->GetModuleName();
		pTSProcessor->UnloadModule();
		GetAppClass().AddLog(TEXT("モジュール \"{}\" を開放しました。"), ModuleName);
	}
#endif
}


void CTSProcessorManager::OnFinalize(CTSProcessor *pTSProcessor)
{
	SaveTSProcessorProperties(pTSProcessor);
}


void CTSProcessorManager::OnNotify(
	CTSProcessor *pTSProcessor, Interface::NotifyType Type, LPCWSTR pszMessage)
{
	GetAppClass().MainWindow.PostNotification(
		pszMessage, CNotificationBarOptions::NOTIFY_TSPROCESSORERROR,
		static_cast<CNotificationBar::MessageType>(Type));
}




CTSProcessorManager::CTSProcessorSettings::CTSProcessorSettings(const GUID &guid)
	: m_guid(guid)
{
}


const CTSProcessorManager::TunerFilterInfo *
CTSProcessorManager::CTSProcessorSettings::GetTunerFilterInfo(
	LPCTSTR pszTuner, WORD NetworkID, WORD TransportStreamID, WORD ServiceID) const
{
	if (IsStringEmpty(pszTuner))
		return nullptr;

	LPCTSTR pszName = ::PathFindFileName(pszTuner);

	for (const auto &e : m_TunerFilterMap) {
		if ((e.Tuner.empty()
				|| (::PathMatchSpec(pszName, e.Tuner.c_str())
					|| (pszTuner != pszName && ::PathMatchSpec(pszTuner, e.Tuner.c_str()))))
				&& (!e.IsNetworkIDEnabled()
					|| e.NetworkID == NetworkID)
				&& (!e.IsTransportStreamIDEnabled()
					|| e.TransportStreamID == TransportStreamID)
				&& (!e.IsServiceIDEnabled()
					|| e.ServiceID == ServiceID)) {
			return &e;
		}
	}

	return nullptr;
}


bool CTSProcessorManager::CTSProcessorSettings::IsTunerFilterMapEnabled() const
{
	if (m_TunerFilterMap.empty())
		return false;

	for (const auto &e : m_TunerFilterMap) {
		if (e.fEnable
				&& (!e.Tuner.empty()
					|| e.IsNetworkIDEnabled()
					|| e.IsTransportStreamIDEnabled()
					|| e.IsServiceIDEnabled()))
			return true;
	}

	return false;
}


} // namespace TVTest
