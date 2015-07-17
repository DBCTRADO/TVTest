#include "stdafx.h"
#include "TVTest.h"
#include "TSProcessor.h"
#include <algorithm>
#include <utility>
#include "Common/DebugDef.h"


namespace TVTest
{


CTSPacketInterface::CTSPacketInterface()
	: m_pMediaData(nullptr)
	, m_fModified(false)
{
}


CTSPacketInterface::~CTSPacketInterface()
{
}


STDMETHODIMP CTSPacketInterface::QueryInterface(REFIID riid, void **ppvObject)
{
	if (ppvObject == nullptr)
		return E_POINTER;

	if (riid == IID_IUnknown || riid == __uuidof(ITSPacket)) {
		*ppvObject = static_cast<ITSPacket*>(this);
	} else if (riid == __uuidof(ITsMediaData)) {
		*ppvObject = static_cast<ITsMediaData*>(this);
	} else {
		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}

	AddRef();

	return S_OK;
}


STDMETHODIMP CTSPacketInterface::GetData(BYTE **ppData)
{
	if (ppData == nullptr)
		return E_POINTER;

	if (m_pMediaData == nullptr) {
		*ppData = nullptr;
		return E_FAIL;
	}

	*ppData = m_pMediaData->GetData();

	return S_OK;
}


STDMETHODIMP CTSPacketInterface::GetSize(ULONG *pSize)
{
	if (pSize == nullptr)
		return E_POINTER;

	if (m_pMediaData == nullptr)
		return E_UNEXPECTED;

	*pSize = m_pMediaData->GetSize();

	return S_OK;
}


STDMETHODIMP CTSPacketInterface::SetModified(BOOL fModified)
{
	m_fModified = fModified != FALSE;
	return S_OK;
}


STDMETHODIMP CTSPacketInterface::GetModified()
{
	return m_fModified ? S_OK : S_FALSE;
}


STDMETHODIMP CTSPacketInterface::SetMediaData(CMediaData *pMediaData)
{
	m_pMediaData = pMediaData;
	return S_OK;
}




CTSProcessor::CTSProcessor(Interface::ITSProcessor *pTSProcessor, IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1, 1)
	, m_pTSProcessor(pTSProcessor)
	, m_pFilterManager(nullptr)
	, m_pFilterModule(nullptr)
	, m_pFilter(nullptr)
	, m_pTSPacket(new CTSPacketInterface)
	, m_fSourceProcessor(false)
	, m_pEventHandler(nullptr)
	, m_CurDevice(-1)
{
	m_pTSProcessor->AddRef();
	m_pTSProcessor->QueryInterface(IID_PPV_ARGS(&m_pFilterManager));
	m_pTSProcessor->QueryInterface(IID_PPV_ARGS(&m_pFilterModule));
}


CTSProcessor::~CTSProcessor()
{
	UnloadModule();
	m_pTSPacket->Release();
	SafeRelease(&m_pFilterModule);
	SafeRelease(&m_pFilterManager);
	m_pTSProcessor->Release();
}


bool CTSProcessor::Initialize()
{
	if (FAILED(m_pTSProcessor->Initialize(this)))
		return false;
	m_pTSProcessor->StartStreaming(this);
	return S_OK;
}


void CTSProcessor::Finalize()
{
	m_pTSProcessor->StopStreaming();
	m_pTSProcessor->Finalize();

	if (m_pEventHandler != nullptr)
		m_pEventHandler->OnFinalize(this);
}


void CTSProcessor::Reset()
{
	m_pTSProcessor->Reset();
}


const bool CTSProcessor::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	m_pTSPacket->SetMediaData(pMediaData);

	return SUCCEEDED(m_pTSProcessor->InputPacket(m_pTSPacket));
}


bool CTSProcessor::SetActiveServiceID(WORD ServiceID)
{
	return SUCCEEDED(m_pTSProcessor->SetActiveServiceID(ServiceID));
}


WORD CTSProcessor::GetActiveServiceID() const
{
	WORD ServiceID = 0;
	if (FAILED(m_pTSProcessor->GetActiveServiceID(&ServiceID)))
		return 0;
	return ServiceID;
}


bool CTSProcessor::GetGuid(GUID *pGuid) const
{
	return SUCCEEDED(m_pTSProcessor->GetGuid(pGuid));
}


bool CTSProcessor::GetName(String *pName) const
{
	if (pName == nullptr)
		return false;

	BSTR bstrName = nullptr;
	if (SUCCEEDED(m_pTSProcessor->GetName(&bstrName)) && bstrName != nullptr) {
		*pName = bstrName;
		::SysFreeString(bstrName);
		return true;
	}

	pName->clear();

	return false;
}


void CTSProcessor::SetSourceProcessor(bool fSource)
{
	m_fSourceProcessor=fSource;
}


bool CTSProcessor::IsPropertyBagSupported() const
{
	IPersistPropertyBag *pPersistPropBag;

	if (FAILED(m_pTSProcessor->QueryInterface(IID_PPV_ARGS(&pPersistPropBag))))
		return false;

	pPersistPropBag->Release();

	return true;
}


bool CTSProcessor::LoadProperties(IPropertyBag *pPropBag)
{
	IPersistPropertyBag *pPersistPropBag;

	if (FAILED(m_pTSProcessor->QueryInterface(IID_PPV_ARGS(&pPersistPropBag))))
		return false;

	HRESULT hr = pPersistPropBag->Load(pPropBag, nullptr);

	pPersistPropBag->Release();

	return SUCCEEDED(hr);
}


bool CTSProcessor::SaveProperties(IPropertyBag *pPropBag)
{
	IPersistPropertyBag *pPersistPropBag;

	if (FAILED(m_pTSProcessor->QueryInterface(IID_PPV_ARGS(&pPersistPropBag))))
		return false;

	HRESULT hr = pPersistPropBag->Save(pPropBag, FALSE, TRUE);

	pPersistPropBag->Release();

	return SUCCEEDED(hr);
}


bool CTSProcessor::IsPropertyPageSupported() const
{
	ISpecifyPropertyPages *pSpecifyPropPages;

	if (FAILED(m_pTSProcessor->QueryInterface(IID_PPV_ARGS(&pSpecifyPropPages))))
		return false;

	pSpecifyPropPages->Release();

	return true;
}


bool CTSProcessor::ShowPropertyPage(HWND hwndOwner, HINSTANCE hinst)
{
	ISpecifyPropertyPages *pSpecifyPropPages;

	if (FAILED(m_pTSProcessor->QueryInterface(IID_PPV_ARGS(&pSpecifyPropPages))))
		return false;

	CAUUID Pages;
	Pages.cElems = 0;
	Pages.pElems = nullptr;
	HRESULT hr = pSpecifyPropPages->GetPages(&Pages);
	if (SUCCEEDED(hr) && Pages.pElems != nullptr) {
		if (Pages.cElems > 0) {
			ISpecifyPropertyPages2 *pSpecifyPropPages2;
			if (SUCCEEDED(pSpecifyPropPages->QueryInterface(IID_PPV_ARGS(&pSpecifyPropPages2)))) {
				IPropertyPage **ppPropPages = new IPropertyPage*[Pages.cElems];
				ULONG PageCount = 0;
				for (ULONG i = 0; i < Pages.cElems; i++) {
					IPropertyPage *pPropPage;
					if (SUCCEEDED(pSpecifyPropPages2->CreatePage(Pages.pElems[i], &pPropPage)))
						ppPropPages[PageCount++] = pPropPage;
				}
				if (PageCount > 0) {
					hr = ShowPropertyPageFrame(ppPropPages, PageCount,
											   m_pTSProcessor, hwndOwner, hinst);
				}
				for (ULONG i = 0; i < PageCount; i++)
					ppPropPages[i]->Release();
				delete [] ppPropPages;
				pSpecifyPropPages2->Release();
			} else {
				IUnknown *pObject;
				hr = m_pTSProcessor->QueryInterface(IID_PPV_ARGS(&pObject));
				if (SUCCEEDED(hr)) {
					hr = ::OleCreatePropertyFrame(
						hwndOwner, 0, 0, L"プロパティ",
						1, &pObject, Pages.cElems, Pages.pElems,
						::GetUserDefaultLCID(),
						0, nullptr);
					pObject->Release();
				}
			}
		}
		::CoTaskMemFree(Pages.pElems);
	}

	pSpecifyPropPages->Release();

	return SUCCEEDED(hr);
}


bool CTSProcessor::IsFilterModuleSupported() const
{
	return m_pFilterModule != nullptr;
}


bool CTSProcessor::GetModuleList(std::vector<String> *pList) const
{
	if (pList == nullptr)
		return false;

	pList->clear();

	if (m_pFilterManager == nullptr)
		return false;

	Interface::IEnumFilterModule *pEnumModule = nullptr;
	if (FAILED(m_pFilterManager->EnumModules(&pEnumModule))
			|| pEnumModule == nullptr)
		return false;

	BSTR bstrName;
	while (pEnumModule->Next(&bstrName) == S_OK) {
		pList->push_back(String(bstrName));
		::SysFreeString(bstrName);
	}

	pEnumModule->Release();

	if (pList->size() > 1) {
		std::sort(pList->begin(), pList->end(),
				  [](const String &Lib1, const String &Lib2) {
				      return StringUtility::CompareNoCase(Lib1, Lib2) < 0;
				  });
	}

	return true;
}


bool CTSProcessor::LoadModule(LPCWSTR pszName)
{
	if (m_pFilterModule == nullptr) {
		SetError(E_NOTIMPL, TEXT("モジュールは利用できません。"));
		return false;
	}

	if (IsModuleLoaded()) {
		if (IsStringEmpty(pszName)) {
			if (m_ModuleName.empty())
				return true;
		} else {
			if (IsEqualFileName(m_ModuleName.c_str(), pszName))
				return true;
		}
	}

	UnloadModule();

	if (FAILED(m_pFilterModule->LoadModule(pszName)))
		return false;

	StringUtility::Assign(m_ModuleName, pszName);

	ClearError();

	return true;
}


void CTSProcessor::UnloadModule()
{
	CloseFilter();

	if (m_pFilterModule != nullptr)
		m_pFilterModule->UnloadModule();

	m_ModuleName.clear();
}


bool CTSProcessor::IsModuleLoaded() const
{
	return m_pFilterModule != nullptr
		&& m_pFilterModule->IsModuleLoaded() == S_OK;
}


bool CTSProcessor::GetModuleInfo(FilterModuleInfo *pInfo) const
{
	if (pInfo == nullptr)
		return false;

	if (m_pFilterModule != nullptr) {
		Interface::FilterModuleInfo Info;

		if (SUCCEEDED(m_pFilterModule->GetModuleInfo(&Info))) {
			pInfo->Name = Info.Name;
			pInfo->Version = Info.Version;
			::SysFreeString(Info.Name);
			::SysFreeString(Info.Version);
			return true;
		}
	}

	return false;
}


bool CTSProcessor::GetModuleInfo(LPCWSTR pszModule, ModuleInfo *pInfo) const
{
	if (pInfo == nullptr)
		return false;

	pInfo->DeviceList.clear();

	Interface::IFilterModule *pModule = nullptr;

	if (IsModuleLoaded()) {
		if (IsStringEmpty(pszModule)) {
			if (m_ModuleName.empty())
				pModule = m_pFilterModule;
		} else {
			if (IsEqualFileName(m_ModuleName.c_str(), pszModule))
				pModule = m_pFilterModule;
		}
	}

	if (pModule == nullptr) {
		if (m_pFilterManager == nullptr)
			return false;
		if (FAILED(m_pFilterManager->CreateModule(&pModule)))
			return false;
		if (FAILED(pModule->LoadModule(pszModule))) {
			pModule->Release();
			return false;
		}
	}

	HRESULT hr;
	int DeviceCount;
	hr = pModule->GetDeviceCount(&DeviceCount);
	if (SUCCEEDED(hr)) {
		for (int i = 0; i < DeviceCount; i++) {
			Interface::IFilterDevice *pDevice;
			hr = pModule->GetDevice(i, &pDevice);
			if (SUCCEEDED(hr)) {
				Interface::FilterDeviceInfo DeviceInfo;
				hr = pDevice->GetDeviceInfo(&DeviceInfo);
				if (SUCCEEDED(hr)) {
					ModuleDeviceInfo Info;

					Info.DeviceID = DeviceInfo.DeviceID;
					Info.Flags = DeviceInfo.Flags;
					Info.Name = DeviceInfo.Name;
					Info.Text = DeviceInfo.Text;
					::SysFreeString(DeviceInfo.Name);
					::SysFreeString(DeviceInfo.Text);

					int FilterCount;
					hr = pDevice->GetFilterCount(&FilterCount);
					if (SUCCEEDED(hr)) {
						for (int j = 0; j < FilterCount; j++) {
							BSTR Name;
							hr = pDevice->GetFilterName(j, &Name);
							if (FAILED(hr))
								break;
							Info.FilterList.push_back(String(Name));
							::SysFreeString(Name);
						}
					}

					pInfo->DeviceList.push_back(Info);
				}

				pDevice->Release();
			}

			if (FAILED(hr)) {
				pInfo->DeviceList.clear();
				break;
			}
		}
	}

	if (pModule != m_pFilterModule) {
		pModule->UnloadModule();
		pModule->Release();
	}

	return SUCCEEDED(hr);
}


int CTSProcessor::GetDeviceCount() const
{
	if (m_pFilterModule != nullptr) {
		int Count = 0;
		if (SUCCEEDED(m_pFilterModule->GetDeviceCount(&Count)))
			return Count;
	}
	return 0;
}


bool CTSProcessor::GetDeviceInfo(int Device, FilterDeviceInfo *pInfo) const
{
	if (pInfo == nullptr)
		return false;

	if (m_pFilterModule != nullptr) {
		Interface::IFilterDevice *pDevice;

		if (SUCCEEDED(m_pFilterModule->GetDevice(Device, &pDevice))) {
			Interface::FilterDeviceInfo Info;
			HRESULT hr = pDevice->GetDeviceInfo(&Info);
			pDevice->Release();
			if (SUCCEEDED(hr)) {
				pInfo->DeviceID = Info.DeviceID;
				pInfo->Flags = Info.Flags;
				pInfo->Name = Info.Name;
				pInfo->Text = Info.Text;
				::SysFreeString(Info.Name);
				::SysFreeString(Info.Text);
				return true;
			}
		}
	}

	return false;
}


bool CTSProcessor::GetDeviceName(int Device, String *pName) const
{
	if (pName == nullptr)
		return false;

	FilterDeviceInfo Info;

	if (!GetDeviceInfo(Device, &Info)) {
		pName->clear();
		return false;
	}

	*pName = std::move(Info.Name);

	return true;
}


bool CTSProcessor::GetDeviceList(std::vector<String> *pList) const
{
	if (pList == nullptr)
		return false;

	pList->clear();

	int Count = GetDeviceCount();

	for (int i = 0; i < Count; i++) {
		String Name;

		if (!GetDeviceName(i, &Name)) {
			pList->clear();
			return false;
		}

		pList->push_back(std::move(Name));
	}

	return true;
}


bool CTSProcessor::GetDeviceFilterList(int Device, std::vector<String> *pList) const
{
	if (pList == nullptr)
		return false;

	pList->clear();

	if (m_pFilterModule != nullptr) {
		Interface::IFilterDevice *pDevice;

		if (SUCCEEDED(m_pFilterModule->GetDevice(Device, &pDevice))) {
			HRESULT hr;
			int Count = 0;

			hr = pDevice->GetFilterCount(&Count);
			if (SUCCEEDED(hr) && Count > 0) {
				pList->reserve(Count);

				for (int i = 0; i < Count; i++) {
					BSTR bstrName = nullptr;
					hr = pDevice->GetFilterName(i, &bstrName);
					if (FAILED(hr))
						break;
					pList->push_back(String(bstrName));
					::SysFreeString(bstrName);
				}
			}

			pDevice->Release();
			if (SUCCEEDED(hr))
				return true;

			pList->clear();
		}
	}

	return false;
}


bool CTSProcessor::IsDeviceAvailable(int Device)
{
	if (m_pFilterModule != nullptr) {
		BOOL fAvailable = FALSE;
		if (SUCCEEDED(m_pFilterModule->IsDeviceAvailable(Device, &fAvailable)))
			return fAvailable != FALSE;
	}
	return false;
}


bool CTSProcessor::CheckDeviceAvailability(int Device, bool *pfAvailable, String *pMessage)
{
	if (pfAvailable != nullptr)
		*pfAvailable = false;
	if (pMessage != nullptr)
		pMessage->clear();

	if (m_pFilterModule != nullptr) {
		BOOL fAvailable = FALSE;
		BSTR bstrMessage = nullptr;
		if (SUCCEEDED(m_pFilterModule->CheckDeviceAvailability(Device, &fAvailable, &bstrMessage))) {
			if (pfAvailable != nullptr)
				*pfAvailable = fAvailable != FALSE;
			if (bstrMessage != nullptr) {
				if (pMessage != nullptr)
					*pMessage = bstrMessage;
				::SysFreeString(bstrMessage);
			}
			return true;
		}
	}

	return false;
}


int CTSProcessor::GetDefaultDevice() const
{
	if (m_pFilterModule != nullptr) {
		int Device = -1;
		if (SUCCEEDED(m_pFilterModule->GetDefaultDevice(&Device)))
			return Device;
	}
	return -1;
}


int CTSProcessor::GetDeviceByID(DWORD DeviceID) const
{
	if (m_pFilterModule != nullptr) {
		int Device = -1;
		if (SUCCEEDED(m_pFilterModule->GetDeviceByID(DeviceID, &Device)))
			return Device;
	}
	return -1;
}


int CTSProcessor::GetDeviceByName(LPCWSTR pszName) const
{
	if (m_pFilterModule != nullptr) {
		int Device = -1;
		if (SUCCEEDED(m_pFilterModule->GetDeviceByName(pszName, &Device)))
			return Device;
	}
	return -1;
}


bool CTSProcessor::OpenFilter(int Device, LPCWSTR pszName)
{
	if (m_pFilter != nullptr) {
		if (m_CurDevice == Device) {
			if (IsStringEmpty(pszName)) {
				if (m_CurFilter.empty())
					return true;
			} else {
				if (StringUtility::CompareNoCase(m_CurFilter, pszName) == 0)
					return true;
			}
		}
	}

	if (m_pFilterModule == nullptr) {
		SetError(E_NOTIMPL, TEXT("フィルターは利用できません。"));
		return false;
	}

	CloseFilter();

	Interface::IFilter *pFilter = nullptr;
	if (FAILED(m_pFilterModule->OpenFilter(Device, pszName, &pFilter)))
		return false;

	m_pFilter = pFilter;
	m_CurDevice = Device;
	StringUtility::Assign(m_CurFilter, pszName);

	ClearError();

	return true;
}


bool CTSProcessor::OpenFilter(LPCWSTR pszDevice, LPCWSTR pszName)
{
	int Device = GetDeviceByName(pszDevice);
	if (Device < 0)
		return false;

	return OpenFilter(Device, pszName);
}


void CTSProcessor::CloseFilter()
{
	SafeRelease(&m_pFilter);
	m_CurDevice = -1;
	m_CurFilter.clear();
}


bool CTSProcessor::IsFilterOpened() const
{
	return m_pFilter != nullptr;
}


int CTSProcessor::GetDevice() const
{
	if (m_pFilter != nullptr) {
		DWORD DeviceID;
		if (SUCCEEDED(m_pFilter->GetDeviceID(&DeviceID))) {
			int Device = -1;
			if (SUCCEEDED(m_pFilterModule->GetDeviceByID(DeviceID, &Device)))
				return Device;
		}
	}
	return -1;
}


bool CTSProcessor::GetFilterName(String *pName) const
{
	if (pName == nullptr)
		return false;

	if (m_pFilter != nullptr) {
		BSTR bstrName = nullptr;
		if (SUCCEEDED(m_pFilter->GetName(&bstrName)) && bstrName != nullptr) {
			*pName = bstrName;
			::SysFreeString(bstrName);
			return true;
		}
	}

	pName->clear();

	return false;
}


bool CTSProcessor::SendFilterCommand(const void *pSendData, DWORD SendSize, void *pRecvData, DWORD *pRecvSize)
{
	if (m_pFilter == nullptr)
		return false;

	return SUCCEEDED(m_pFilter->SendCommand(pSendData, SendSize, pRecvData, pRecvSize));
}


bool CTSProcessor::SetEnableProcessing(bool fEnable)
{
	return SUCCEEDED(m_pTSProcessor->SetEnableProcessing(fEnable));
}


bool CTSProcessor::GetEnableProcessing(bool *pfEnable) const
{
	if (pfEnable == nullptr)
		return false;

	BOOL fEnable = FALSE;
	if (FAILED(m_pTSProcessor->GetEnableProcessing(&fEnable))) {
		*pfEnable = false;
		return false;
	}

	*pfEnable = fEnable != FALSE;

	return true;
}


void CTSProcessor::SetEventHandler(CEventHandler *pEventHandler)
{
	m_pEventHandler = pEventHandler;
}


STDMETHODIMP CTSProcessor::QueryInterface(REFIID riid, void **ppvObject)
{
	if (ppvObject == nullptr)
		return E_POINTER;

	if (riid == IID_IUnknown || riid == __uuidof(IStreamingClient)) {
		*ppvObject = static_cast<IStreamingClient*>(this);
	} else if (riid == __uuidof(ITSOutput)) {
		*ppvObject = static_cast<ITSOutput*>(this);
	} else {
		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}

	AddRef();

	return S_OK;
}


STDMETHODIMP CTSProcessor::OnError(const Interface::ErrorInfo *pInfo)
{
	if (pInfo == nullptr)
		return E_POINTER;

	SetError(pInfo->hr, pInfo->pszText, pInfo->pszAdvise, pInfo->pszSystemMessage);

	return S_OK;
}


STDMETHODIMP CTSProcessor::OutLog(Interface::LogType Type, LPCWSTR pszMessage)
{
	if (pszMessage == nullptr)
		return E_POINTER;

	if (Type == Interface::LOG_VERBOSE) {
		TRACE(L"%s\n", pszMessage);
		return S_OK;
	}

	CTracer::TraceType TraceType;

	switch (Type) {
	case Interface::LOG_INFO:
	default:
		TraceType = CTracer::TYPE_INFORMATION;
		break;
	case Interface::LOG_WARNING:
		TraceType = CTracer::TYPE_WARNING;
		break;
	case Interface::LOG_ERROR:
		TraceType = CTracer::TYPE_ERROR;
		break;
	}

	Trace(TraceType, TEXT("%s"), pszMessage);

	return S_OK;
}


STDMETHODIMP CTSProcessor::Notify(Interface::NotifyType Type, LPCWSTR pszMessage)
{
	if (pszMessage == nullptr)
		return E_POINTER;

	if (m_pEventHandler != nullptr)
		m_pEventHandler->OnNotify(this, Type, pszMessage);

	return S_OK;
}


STDMETHODIMP CTSProcessor::OutputPacket(Interface::ITSPacket *pPacket)
{
	if (pPacket == nullptr)
		return E_POINTER;

	ITsMediaData *pTsMediaData;

	if (SUCCEEDED(pPacket->QueryInterface(IID_PPV_ARGS(&pTsMediaData)))) {
		CMediaData *pMediaData = pTsMediaData->GetMediaData();

		if (!m_fSourceProcessor && pPacket->GetModified() == S_OK) {
#ifndef _DEBUG
			CTsPacket *pTsPacket = static_cast<CTsPacket*>(pMediaData);
#else
			CTsPacket *pTsPacket = dynamic_cast<CTsPacket*>(pMediaData);
			_ASSERT(pTsPacket != nullptr);
#endif
			if (pTsPacket->ParsePacket() != CTsPacket::EC_VALID) {
				pTsMediaData->Release();
				return E_FAIL;
			}
		}

		OutputMedia(pMediaData);

		pTsMediaData->Release();
	} else {
		HRESULT hr;
		ULONG Size = 0;
		hr = pPacket->GetSize(&Size);
		if (FAILED(hr))
			return hr;
		BYTE *pData = nullptr;
		hr = pPacket->GetData(&pData);
		if (FAILED(hr))
			return hr;
		if (Size == 0 || pData == nullptr)
			return E_FAIL;

		if (m_fSourceProcessor) {
			if (m_OutputPacket.SetData(pData, Size) != Size)
				return E_OUTOFMEMORY;
		} else {
			if (Size != TS_PACKETSIZE)
				return E_FAIL;
			if (m_OutputPacket.SetData(pData, TS_PACKETSIZE) != TS_PACKETSIZE)
				return E_OUTOFMEMORY;
			if (m_OutputPacket.ParsePacket() != CTsPacket::EC_VALID)
				return E_FAIL;
		}

		OutputMedia(&m_OutputPacket);
	}

	return S_OK;
}


}	// namespace TVTest
