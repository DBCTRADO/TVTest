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


CTSProcessorManager::CTSProcessorManager()
{
}


CTSProcessorManager::~CTSProcessorManager()
{
	for (auto it=m_SettingsList.begin();it!=m_SettingsList.end();++it)
		delete *it;
}


bool CTSProcessorManager::ReadSettings(CSettings &Settings)
{
	int Count;

	if (Settings.Read(TEXT("SettingsCount"),&Count) && Count>0) {
		String Buffer;

		for (int i=0;i<Count;i++) {
			TCHAR szKey[64];
			GUID guid;

			StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.GUID"),i);
			if (Settings.Read(szKey,&Buffer)
					&& ::IIDFromString(Buffer.c_str(),&guid)==S_OK) {
				CTSProcessorSettings *pTSProcessorSettings=new CTSProcessorSettings(guid);
				bool f;

				StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.EnableProcessing"),i);
				if (Settings.Read(szKey,&f))
					pTSProcessorSettings->m_EnableProcessing=f;
				StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.DefaultModule"),i);
				Settings.Read(szKey,&pTSProcessorSettings->m_DefaultFilter.Module);
				StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.DefaultDevice"),i);
				Settings.Read(szKey,&pTSProcessorSettings->m_DefaultFilter.Device);
				StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.DefaultFilter"),i);
				Settings.Read(szKey,&pTSProcessorSettings->m_DefaultFilter.Filter);

#if 1
				// 旧仕様の設定を読み込み
				for (int j=0;;j++) {
					TunerFilterInfo TunerDecInfo;
					int Value;

					StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.NetworkMap%d.NetworkID"),i,j);
					if (!Settings.Read(szKey,&Value))
						break;
					TunerDecInfo.NetworkID=(WORD)Value;
					StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.NetworkMap%d.TSID"),i,j);
					if (Settings.Read(szKey,&Value))
						TunerDecInfo.TransportStreamID=(WORD)Value;
					StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.NetworkMap%d.Enable"),i,j);
					Settings.Read(szKey,&TunerDecInfo.fEnable);
					StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.NetworkMap%d.EnableProcessing"),i,j);
					Settings.Read(szKey,&TunerDecInfo.fEnableProcessing);
					StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.NetworkMap%d.Module"),i,j);
					Settings.Read(szKey,&TunerDecInfo.Module);
					StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.NetworkMap%d.Device"),i,j);
					Settings.Read(szKey,&TunerDecInfo.Device);
					StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.NetworkMap%d.Filter"),i,j);
					Settings.Read(szKey,&TunerDecInfo.Filter);

					pTSProcessorSettings->m_TunerFilterMap.push_back(TunerDecInfo);
				}
#endif

				for (int j=0;;j++) {
					TunerFilterInfo TunerDecInfo;
					unsigned int Value;

					StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.TunerMap%d.Enable"),i,j);
					if (!Settings.IsValueExists(szKey))
						break;
					Settings.Read(szKey,&TunerDecInfo.fEnable);
					StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.TunerMap%d.EnableProcessing"),i,j);
					Settings.Read(szKey,&TunerDecInfo.fEnableProcessing);
					StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.TunerMap%d.Module"),i,j);
					Settings.Read(szKey,&TunerDecInfo.Module);
					StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.TunerMap%d.Device"),i,j);
					Settings.Read(szKey,&TunerDecInfo.Device);
					StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.TunerMap%d.Filter"),i,j);
					Settings.Read(szKey,&TunerDecInfo.Filter);
					StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.TunerMap%d.Tuner"),i,j);
					Settings.Read(szKey,&TunerDecInfo.Tuner);
					StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.TunerMap%d.NetworkID"),i,j);
					if (Settings.Read(szKey,&Value))
						TunerDecInfo.NetworkID=(WORD)Value;
					StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.TunerMap%d.TSID"),i,j);
					if (Settings.Read(szKey,&Value))
						TunerDecInfo.TransportStreamID=(WORD)Value;
					StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.TunerMap%d.ServiceID"),i,j);
					if (Settings.Read(szKey,&Value))
						TunerDecInfo.ServiceID=(WORD)Value;

					pTSProcessorSettings->m_TunerFilterMap.push_back(TunerDecInfo);
				}

				for (int j=0;;j++) {
					String Name,Value;

					StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.Property%d.Name"),i,j);
					if (!Settings.Read(szKey,&Name))
						break;
					StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.Property%d.Value"),i,j);
					if (Settings.Read(szKey,&Value)) {
						CVariant Var;
						if (SUCCEEDED(Var.FromString(StringUtility::Decode(Value))))
							pTSProcessorSettings->m_PropertyList.insert(std::pair<String,CVariant>(Name,Var));
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
	Settings.Write(TEXT("SettingsCount"),(int)m_SettingsList.size());

	for (int i=0;i<(int)m_SettingsList.size();i++) {
		const CTSProcessorSettings *pTSProcessorSettings=m_SettingsList[i];
		TCHAR szKey[64],szBuffer[256];

		StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.GUID"),i);
		::StringFromGUID2(pTSProcessorSettings->m_guid,szBuffer,lengthof(szBuffer));
		Settings.Write(szKey,szBuffer);
		if (pTSProcessorSettings->m_EnableProcessing) {
			StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.EnableProcessing"),i);
			Settings.Write(szKey,pTSProcessorSettings->m_EnableProcessing.value());
		}
		StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.DefaultModule"),i);
		Settings.Write(szKey,pTSProcessorSettings->m_DefaultFilter.Module);
		StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.DefaultDevice"),i);
		Settings.Write(szKey,pTSProcessorSettings->m_DefaultFilter.Device);
		StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.DefaultFilter"),i);
		Settings.Write(szKey,pTSProcessorSettings->m_DefaultFilter.Filter);

		for (int j=0;j<(int)pTSProcessorSettings->m_TunerFilterMap.size();j++) {
			const TunerFilterInfo &TunerDecInfo=pTSProcessorSettings->m_TunerFilterMap[j];

			StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.TunerMap%d.Enable"),i,j);
			Settings.Write(szKey,TunerDecInfo.fEnable);
			StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.TunerMap%d.EnableProcessing"),i,j);
			Settings.Write(szKey,TunerDecInfo.fEnableProcessing);
			StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.TunerMap%d.Module"),i,j);
			Settings.Write(szKey,TunerDecInfo.Module);
			StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.TunerMap%d.Device"),i,j);
			Settings.Write(szKey,TunerDecInfo.Device);
			StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.TunerMap%d.Filter"),i,j);
			Settings.Write(szKey,TunerDecInfo.Filter);
			StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.TunerMap%d.Tuner"),i,j);
			Settings.Write(szKey,TunerDecInfo.Tuner);
			if (TunerDecInfo.IsNetworkIDEnabled()) {
				StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.TunerMap%d.NetworkID"),i,j);
				Settings.Write(szKey,(unsigned int)TunerDecInfo.NetworkID);
			}
			if (TunerDecInfo.IsTransportStreamIDEnabled()) {
				StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.TunerMap%d.TSID"),i,j);
				Settings.Write(szKey,(unsigned int)TunerDecInfo.TransportStreamID);
			}
			if (TunerDecInfo.IsServiceIDEnabled()) {
				StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.TunerMap%d.ServiceID"),i,j);
				Settings.Write(szKey,(unsigned int)TunerDecInfo.ServiceID);
			}
		}

		const CTSProcessorSettings::PropertyList &PropertyList=pTSProcessorSettings->m_PropertyList;
		int j=0;
		for (auto it=PropertyList.begin();it!=PropertyList.end();++it) {
			String Value;
			if (SUCCEEDED(it->second.ToString(&Value))) {
				StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.Property%d.Name"),i,j);
				Settings.Write(szKey,it->first);
				StdUtil::snprintf(szKey,lengthof(szKey),TEXT("Processor%d.Property%d.Value"),i,j);
				Settings.Write(szKey,StringUtility::Encode(Value,TEXT("\"")));
				j++;
			}
		}
	}

	return true;
}


CTSProcessorManager::CTSProcessorSettings *CTSProcessorManager::GetTSProcessorSettings(const GUID &guid)
{
	for (auto it=m_SettingsList.begin();it!=m_SettingsList.end();++it) {
		if ((*it)->m_guid==guid)
			return *it;
	}
	return nullptr;
}


const CTSProcessorManager::CTSProcessorSettings *CTSProcessorManager::GetTSProcessorSettings(const GUID &guid) const
{
	for (auto it=m_SettingsList.begin();it!=m_SettingsList.end();++it) {
		if ((*it)->m_guid==guid)
			return *it;
	}
	return nullptr;
}


bool CTSProcessorManager::SetTSProcessorSettings(CTSProcessorSettings *pSettings)
{
	if (pSettings==nullptr)
		return false;

	CTSProcessorSettings *pCurSettings=GetTSProcessorSettings(pSettings->m_guid);
	if (pCurSettings!=nullptr) {
		*pCurSettings=std::move(*pSettings);
		delete pSettings;
	} else {
		m_SettingsList.push_back(pSettings);
	}

	return true;
}


bool CTSProcessorManager::ApplyTSProcessorSettings(const GUID &guid,bool fSetProperties)
{
	CTSProcessor *pTSProcessor=GetTSProcessor(guid);
	if (pTSProcessor==nullptr)
		return false;

	return ApplyTSProcessorSettings(pTSProcessor,guid,fSetProperties);
}


bool CTSProcessorManager::ApplyTSProcessorSettings(CTSProcessor *pTSProcessor,bool fSetProperties)
{
	if (pTSProcessor==nullptr)
		return false;

	GUID guid;
	if (!pTSProcessor->GetGuid(&guid))
		return false;

	return ApplyTSProcessorSettings(pTSProcessor,guid,fSetProperties);
}


bool CTSProcessorManager::ApplyTSProcessorSettings(
	CTSProcessor *pTSProcessor,const GUID &guid,bool fSetProperties)
{
	const CTSProcessorSettings *pSettings=GetTSProcessorSettings(guid);
	if (pSettings==nullptr)
		return false;

	if (pSettings->m_EnableProcessing)
		pTSProcessor->SetEnableProcessing(pSettings->m_EnableProcessing.value());

	if (fSetProperties
			&& !pSettings->m_PropertyList.empty()
			&& pTSProcessor->IsPropertyBagSupported()) {
		CPropertyBag *pPropBag=new CPropertyBag;
		for (auto it=pSettings->m_PropertyList.begin();it!=pSettings->m_PropertyList.end();++it) {
			CVariant var(it->second);
			pPropBag->Write(it->first.c_str(),&var);
		}
		pTSProcessor->LoadProperties(pPropBag);
		pPropBag->Release();
	}

	return true;
}


bool CTSProcessorManager::SaveTSProcessorProperties(CTSProcessor *pTSProcessor)
{
	if (pTSProcessor==nullptr)
		return false;

	bool fSaved=false;
	GUID guid;

	if (pTSProcessor->IsPropertyBagSupported()
			&& pTSProcessor->GetGuid(&guid)) {
		CTSProcessorSettings *pSettings=GetTSProcessorSettings(guid);

		if (pSettings!=nullptr) {
			CPropertyBag *pPropBag=new CPropertyBag;

			if (SUCCEEDED(pTSProcessor->SaveProperties(pPropBag))) {
				pSettings->m_PropertyList.clear();
				for (auto it=pPropBag->begin();it!=pPropBag->end();++it) {
					pSettings->m_PropertyList.insert(*it);
				}
				fSaved=true;
			}

			pPropBag->Release();
		}
	}

	return fSaved;
}


bool CTSProcessorManager::RegisterTSProcessor(
	CTSProcessor *pTSProcessor,CCoreEngine::TSProcessorConnectPosition ConnectPosition)
{
	CAppMain &App=GetAppClass();

	if (!App.CoreEngine.RegisterTSProcessor(pTSProcessor,ConnectPosition))
		return false;

	pTSProcessor->SetEventHandler(this);

	GUID guid;
	if (pTSProcessor->GetGuid(&guid)) {
		if (!ApplyTSProcessorSettings(pTSProcessor,guid)) {
			std::vector<String> List;
			if (pTSProcessor->GetModuleList(&List) && List.size()==1) {
				CTSProcessorSettings *pSettings=new CTSProcessorSettings(guid);
				pSettings->m_DefaultFilter.Module=std::move(List.front());
				m_SettingsList.push_back(pSettings);
			}
		}
	}

	return true;
}


CTSProcessor *CTSProcessorManager::GetTSProcessor(const GUID &guid) const
{
	CCoreEngine &CoreEngine=GetAppClass().CoreEngine;
	const size_t TSFilterCount=CoreEngine.GetTSProcessorCount();

	for (size_t i=0;i<TSFilterCount;i++) {
		CTSProcessor *pTSProcessor=CoreEngine.GetTSProcessorByIndex(i);
		GUID ProcessorGuid;

		if (pTSProcessor!=nullptr
				&& pTSProcessor->GetGuid(&ProcessorGuid)
				&& ProcessorGuid==guid)
			return pTSProcessor;
	}

	return nullptr;
}


bool CTSProcessorManager::GetTSProcessorList(std::vector<CTSProcessor*> *pList) const
{
	if (pList==nullptr)
		return false;

	pList->clear();

	CCoreEngine &CoreEngine=GetAppClass().CoreEngine;
	const size_t TSFilterCount=CoreEngine.GetTSProcessorCount();

	for (size_t i=0;i<TSFilterCount;i++) {
		CTSProcessor *pTSProcessor=CoreEngine.GetTSProcessorByIndex(i);
		if (pTSProcessor!=nullptr)
			pList->push_back(pTSProcessor);
	}

	return true;
}


void CTSProcessorManager::OpenDefaultFilters(unsigned int FilterOpenFlags)
{
	CCoreEngine &CoreEngine=GetAppClass().CoreEngine;
	const size_t TSFilterCount=CoreEngine.GetTSProcessorCount();

	for (size_t i=0;i<TSFilterCount;i++) {
		CTSProcessor *pTSProcessor=CoreEngine.GetTSProcessorByIndex(i);
		if (pTSProcessor==nullptr)
			continue;

		GUID guid;
		if (!pTSProcessor->GetGuid(&guid))
			continue;

		CTSProcessorSettings *pSettings=GetTSProcessorSettings(guid);
		if (pSettings!=nullptr
				&& !pSettings->IsTunerFilterMapEnabled()) {
			OpenFilter(pTSProcessor,pSettings,
					   pSettings->m_DefaultFilter,
					   FilterOpenFlags);
		}
	}
}


void CTSProcessorManager::CloseAllFilters()
{
	CCoreEngine &CoreEngine=GetAppClass().CoreEngine;
	const size_t TSFilterCount=CoreEngine.GetTSProcessorCount();

	for (size_t i=0;i<TSFilterCount;i++) {
		CTSProcessor *pTSProcessor=CoreEngine.GetTSProcessorByIndex(i);
		if (pTSProcessor!=nullptr)
			pTSProcessor->CloseFilter();
	}
}


void CTSProcessorManager::OnTunerChange(LPCTSTR pszOldTuner,LPCTSTR pszNewTuner)
{
	CCoreEngine &CoreEngine=GetAppClass().CoreEngine;
	const size_t TSFilterCount=CoreEngine.GetTSProcessorCount();

	for (size_t i=0;i<TSFilterCount;i++) {
		CTSProcessor *pTSProcessor=CoreEngine.GetTSProcessorByIndex(i);
		if (pTSProcessor==nullptr)
			continue;

		GUID guid;
		if (!pTSProcessor->GetGuid(&guid))
			continue;

		const CTSProcessorSettings *pSettings=GetTSProcessorSettings(guid);
		if (pSettings==nullptr)
			continue;

		const TunerFilterInfo *pOldTunerDecInfo=pSettings->GetTunerFilterInfo(pszOldTuner);
		const TunerFilterInfo *pNewTunerDecInfo=pSettings->GetTunerFilterInfo(pszNewTuner);
		const FilterInfo *pNewFilter,*pOldFilter;

		if (pNewTunerDecInfo!=nullptr && pNewTunerDecInfo->fEnable) {
			if (!pNewTunerDecInfo->fEnableProcessing) {
				pTSProcessor->CloseFilter();
				continue;
			}
			pNewFilter=pNewTunerDecInfo;
		} else {
			pNewFilter=&pSettings->m_DefaultFilter;
		}
		if (pOldTunerDecInfo!=nullptr && pOldTunerDecInfo->fEnable) {
			if (!pOldTunerDecInfo->fEnableProcessing)
				continue;
			pOldFilter=pOldTunerDecInfo;
		} else {
			pOldFilter=&pSettings->m_DefaultFilter;
		}
		if (StringUtility::CompareNoCase(pOldFilter->Device,pNewFilter->Device)!=0
				|| StringUtility::CompareNoCase(pOldFilter->Filter,pNewFilter->Filter)!=0)
			pTSProcessor->CloseFilter();
		if (!IsEqualFileName(pOldFilter->Module.c_str(),pNewFilter->Module.c_str()))
			pTSProcessor->UnloadModule();
	}
}


void CTSProcessorManager::OnTunerOpened(LPCTSTR pszTuner,unsigned int FilterOpenFlags)
{
	CAppMain &App=GetAppClass();
	CCoreEngine &CoreEngine=App.CoreEngine;
	const size_t TSFilterCount=CoreEngine.GetTSProcessorCount();

	for (size_t i=0;i<TSFilterCount;i++) {
		CTSProcessor *pTSProcessor=CoreEngine.GetTSProcessorByIndex(i);
		if (pTSProcessor==nullptr)
			continue;

		GUID guid;
		if (!pTSProcessor->GetGuid(&guid))
			continue;

		CTSProcessorSettings *pSettings=GetTSProcessorSettings(guid);
		if (pSettings==nullptr || !pSettings->IsTunerFilterMapEnabled())
			continue;

		const TunerFilterInfo *pTunerDecInfo=pSettings->GetTunerFilterInfo(pszTuner);
		const FilterInfo *pFilter;

		if (pTunerDecInfo!=nullptr && pTunerDecInfo->fEnable) {
			if (!pTunerDecInfo->fEnableProcessing) {
				pTSProcessor->CloseFilter();
				continue;
			}
			pFilter=pTunerDecInfo;
		} else {
			pFilter=&pSettings->m_DefaultFilter;
		}

		OpenFilter(pTSProcessor,pSettings,*pFilter,FilterOpenFlags);
	}
}


void CTSProcessorManager::OnNetworkChanged(
	LPCTSTR pszTuner,WORD NetworkID,WORD TransportStreamID,WORD ServiceID,unsigned int FilterOpenFlags)
{
	CAppMain &App=GetAppClass();
	CCoreEngine &CoreEngine=App.CoreEngine;
	const size_t TSFilterCount=CoreEngine.GetTSProcessorCount();

	for (size_t i=0;i<TSFilterCount;i++) {
		CTSProcessor *pTSProcessor=CoreEngine.GetTSProcessorByIndex(i);
		if (pTSProcessor==nullptr)
			continue;

		GUID guid;
		if (!pTSProcessor->GetGuid(&guid))
			continue;

		CTSProcessorSettings *pSettings=GetTSProcessorSettings(guid);
		if (pSettings==nullptr)
			continue;

		const TunerFilterInfo *pTunerDecInfo=
			pSettings->GetTunerFilterInfo(pszTuner,NetworkID,TransportStreamID,ServiceID);
		const FilterInfo *pFilter;

		if (pTunerDecInfo!=nullptr && pTunerDecInfo->fEnable) {
			if (!pTunerDecInfo->fEnableProcessing) {
				pTSProcessor->CloseFilter();
				continue;
			}
			pFilter=pTunerDecInfo;
		} else {
			pFilter=&pSettings->m_DefaultFilter;
		}

		OpenFilter(pTSProcessor,pSettings,*pFilter,FilterOpenFlags);
	}
}


void CTSProcessorManager::OpenFilter(
	CTSProcessor *pTSProcessor,CTSProcessorSettings *pSettings,
	const FilterInfo &Filter,unsigned int FilterOpenFlags)
{
	TRACE(TEXT("CTSProcessorManager::OpenFilter() : %s %s %s\n"),
		  Filter.Module.c_str(),Filter.Device.c_str(),Filter.Filter.c_str());

	// オープンに失敗したフィルタを繰り返しオープンしようとするのを避けるため、
	// 前回オープンを試みたフィルタを記憶しておく
	if (pSettings->m_fLastOpenFailed && pSettings->m_LastOpenFilter==Filter)
		return;
	pSettings->m_LastOpenFilter=Filter;

	CAppMain &App=GetAppClass();

	if (pTSProcessor->IsFilterModuleSupported()) {
		if (!Filter.Module.empty()) {
			if (!pTSProcessor->IsModuleLoaded()
					|| !IsEqualFileName(pTSProcessor->GetModuleName().c_str(),Filter.Module.c_str())) {
				if (!pTSProcessor->LoadModule(Filter.Module.c_str())) {
					pSettings->m_fLastOpenFailed=true;
					App.AddLog(CLogItem::TYPE_ERROR,TEXT("\"%s\" を読み込めません。"),Filter.Module.c_str());
					return;
				}

				CTSProcessor::FilterModuleInfo ModuleInfo;
				pTSProcessor->GetModuleInfo(&ModuleInfo);
				App.AddLog(TEXT("モジュール \"%s\" (%s %s) を読み込みました。"),
						   Filter.Module.c_str(),ModuleInfo.Name.c_str(),ModuleInfo.Version.c_str());
			}
		} else {
			if (!pTSProcessor->IsModuleLoaded())
				return;
		}
	}

	String DeviceName(Filter.Device);

	if (DeviceName.empty()) {
		int DeviceNo=pTSProcessor->GetDefaultDevice();
		if (DeviceNo<0) {
			App.AddLog(CLogItem::TYPE_ERROR,TEXT("TSフィルターのデフォルトデバイスがありません。"));
			return;
		}
		pTSProcessor->GetDeviceName(DeviceNo,&DeviceName);
	}

	bool fResult=pTSProcessor->OpenFilter(DeviceName.c_str(),Filter.Filter.c_str());
	pSettings->m_fLastOpenFailed=!fResult;
	if (!fResult) {
		if (!Filter.Filter.empty())
			App.AddLog(CLogItem::TYPE_ERROR,TEXT("TSフィルター \"%s\" : \"%s\" をオープンできません。"),DeviceName.c_str(),Filter.Filter.c_str());
		else
			App.AddLog(CLogItem::TYPE_ERROR,TEXT("TSフィルター \"%s\" をオープンできません。"),DeviceName.c_str());

		if ((FilterOpenFlags & FILTER_OPEN_RETRY_DIALOG)!=0) {
			String Message;
			CTSProcessorErrorDialog Dialog(pTSProcessor);

			if (!IsStringEmpty(pTSProcessor->GetLastErrorText())) {
				Message=pTSProcessor->GetLastErrorText();
				Message+=TEXT("\r\n");
			}
			if (!IsStringEmpty(pTSProcessor->GetLastErrorSystemMessage())) {
				Message+=TEXT("(");
				Message+=pTSProcessor->GetLastErrorSystemMessage();
				Message+=TEXT(")\r\n");
			}

			Dialog.SetMessage(
				!Message.empty() ? Message.c_str() : TEXT("TSフィルターをオープンできません。"));
			Dialog.SetDevice(DeviceName);
			Dialog.SetFilter(Filter.Filter);

			while (Dialog.Show(App.UICore.GetDialogOwner())) {
				if (IsStringEmpty(Dialog.GetDevice()))
					break;
				pSettings->m_LastOpenFilter.Device=Dialog.GetDevice();
				pSettings->m_LastOpenFilter.Filter=Dialog.GetFilter();
				if (pTSProcessor->OpenFilter(Dialog.GetDevice(),Dialog.GetFilter())) {
					pSettings->m_fLastOpenFailed=false;
					break;
				}
			}
		} else {
			if ((FilterOpenFlags & FILTER_OPEN_NOTIFY_ERROR)!=0) {
				App.UICore.GetSkin()->ShowNotificationBar(
					TEXT("TSフィルターをオープンできません。"),
					CNotificationBar::MESSAGE_ERROR,6000);
			}

			if ((FilterOpenFlags & FILTER_OPEN_NO_UI)==0) {
				App.UICore.GetSkin()->ShowErrorMessage(pTSProcessor);
			}
		}
	}
}


void CTSProcessorManager::OnFinalize(CTSProcessor *pTSProcessor)
{
	SaveTSProcessorProperties(pTSProcessor);
}


void CTSProcessorManager::OnNotify(CTSProcessor *pTSProcessor,
								   Interface::NotifyType Type, LPCWSTR pszMessage)
{
	GetAppClass().MainWindow.PostMessage(
		WM_APP_SHOWNOTIFICATIONBAR,
		MAKEWPARAM(static_cast<WORD>(Type),COSDOptions::NOTIFY_TSPROCESSORERROR),
		reinterpret_cast<LPARAM>(DuplicateString(pszMessage)));
}




CTSProcessorManager::CTSProcessorSettings::CTSProcessorSettings(const GUID &guid)
	: m_guid(guid)
	, m_fLastOpenFailed(false)
{
}


const CTSProcessorManager::TunerFilterInfo *
	CTSProcessorManager::CTSProcessorSettings::GetTunerFilterInfo(
		LPCTSTR pszTuner,WORD NetworkID,WORD TransportStreamID,WORD ServiceID) const
{
	if (IsStringEmpty(pszTuner))
		return nullptr;

	LPCTSTR pszName=::PathFindFileName(pszTuner);

	for (auto it=m_TunerFilterMap.begin();it!=m_TunerFilterMap.end();++it) {
		if ((it->Tuner.empty()
				|| (::PathMatchSpec(pszName,it->Tuner.c_str())
					|| (pszTuner!=pszName && ::PathMatchSpec(pszTuner,it->Tuner.c_str()))))
				&& (!it->IsNetworkIDEnabled()
					|| it->NetworkID==NetworkID)
				&& (!it->IsTransportStreamIDEnabled()
					|| it->TransportStreamID==TransportStreamID)
				&& (!it->IsServiceIDEnabled()
					|| it->ServiceID==ServiceID)) {
			return &*it;
		}
	}

	return nullptr;
}


bool CTSProcessorManager::CTSProcessorSettings::IsTunerFilterMapEnabled() const
{
	if (m_TunerFilterMap.empty())
		return false;

	for (auto it=m_TunerFilterMap.begin();it!=m_TunerFilterMap.end();++it) {
		if (it->fEnable
				&& (!it->Tuner.empty()
					|| it->IsNetworkIDEnabled()
					|| it->IsTransportStreamIDEnabled()
					|| it->IsServiceIDEnabled()))
			return true;
	}

	return false;
}


}	// namespace TVTest
