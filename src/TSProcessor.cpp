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
#include "TSProcessor.h"
#include <OleCtl.h>
#include <algorithm>
#include <utility>
#include "Common/DebugDef.h"


namespace TVTest
{


STDMETHODIMP CTSPacketInterface::QueryInterface(REFIID riid, void **ppvObject)
{
	if (ppvObject == nullptr)
		return E_POINTER;

	if (riid == IID_IUnknown || riid == __uuidof(ITSPacket)) {
		*ppvObject = static_cast<ITSPacket*>(this);
	} else if (riid == __uuidof(ITSDataBuffer)) {
		*ppvObject = static_cast<ITSDataBuffer*>(this);
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

	if (m_pData == nullptr) {
		*ppData = nullptr;
		return E_FAIL;
	}

	*ppData = m_pData->GetData();

	return S_OK;
}


STDMETHODIMP CTSPacketInterface::GetSize(ULONG *pSize)
{
	if (pSize == nullptr)
		return E_POINTER;

	if (m_pData == nullptr)
		return E_UNEXPECTED;

	*pSize = static_cast<ULONG>(m_pData->GetSize());

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


STDMETHODIMP CTSPacketInterface::SetDataBuffer(LibISDB::DataBuffer *pData)
{
	m_pData = pData;
	return S_OK;
}




CTSProcessor::CTSProcessor(Interface::ITSProcessor *pTSProcessor)
	: m_pTSProcessor(pTSProcessor)
	, m_pTSPacket(new CTSPacketInterface)
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
	return true;
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


void CTSProcessor::SetActiveServiceID(uint16_t ServiceID)
{
	m_pTSProcessor->SetActiveServiceID(ServiceID);
}


bool CTSProcessor::ReceiveData(LibISDB::DataStream *pData)
{
	do {
		m_pTSPacket->SetDataBuffer(pData->GetData());
		m_pTSProcessor->InputPacket(m_pTSPacket);
	} while (pData->Next());

	if (m_OutputSequence.GetDataCount() > 0) {
		OutputData(m_OutputSequence);
		m_OutputSequence.SetDataCount(0);
		m_OutputPacket.clear();
	}

	return true;
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

	const HRESULT hr = pPersistPropBag->Load(pPropBag, nullptr);

	pPersistPropBag->Release();

	return SUCCEEDED(hr);
}


bool CTSProcessor::SaveProperties(IPropertyBag *pPropBag)
{
	IPersistPropertyBag *pPersistPropBag;

	if (FAILED(m_pTSProcessor->QueryInterface(IID_PPV_ARGS(&pPersistPropBag))))
		return false;

	const HRESULT hr = pPersistPropBag->Save(pPropBag, FALSE, TRUE);

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
				std::vector<IPropertyPage *> PropPages(Pages.cElems);
				ULONG PageCount = 0;
				for (ULONG i = 0; i < Pages.cElems; i++) {
					IPropertyPage *pPropPage;
					if (SUCCEEDED(pSpecifyPropPages2->CreatePage(Pages.pElems[i], &pPropPage)))
						PropPages[PageCount++] = pPropPage;
				}
				if (PageCount > 0) {
					hr = ShowPropertyPageFrame(PropPages.data(), PageCount, m_pTSProcessor, hwndOwner, hinst);
				}
				for (ULONG i = 0; i < PageCount; i++)
					PropPages[i]->Release();
				pSpecifyPropPages2->Release();
			} else {
				IUnknown *pObject;
				hr = m_pTSProcessor->QueryInterface(IID_IUnknown, reinterpret_cast<void**>(&pObject));
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
		pList->emplace_back(bstrName);
		::SysFreeString(bstrName);
	}

	pEnumModule->Release();

	if (pList->size() > 1) {
		std::ranges::sort(
			*pList,
			[](const String &Lib1, const String &Lib2) {
				return StringUtility::CompareNoCase(Lib1, Lib2) < 0;
			});
	}

	return true;
}


bool CTSProcessor::LoadModule(LPCWSTR pszName)
{
	if (m_pFilterModule == nullptr) {
		SetHRESULTError(E_NOTIMPL, TEXT("モジュールは利用できません。"));
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

	ResetError();

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
							Info.FilterList.emplace_back(Name);
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
			const HRESULT hr = pDevice->GetDeviceInfo(&Info);
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

	const int Count = GetDeviceCount();

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
					pList->emplace_back(bstrName);
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
		SetHRESULTError(E_NOTIMPL, TEXT("フィルターは利用できません。"));
		return false;
	}

	CloseFilter();

	Interface::IFilter *pFilter = nullptr;
	if (FAILED(m_pFilterModule->OpenFilter(Device, pszName, &pFilter)))
		return false;

	m_pFilter = pFilter;
	m_CurDevice = Device;
	StringUtility::Assign(m_CurFilter, pszName);

	ResetError();

	return true;
}


bool CTSProcessor::OpenFilter(LPCWSTR pszDevice, LPCWSTR pszName)
{
	const int Device = GetDeviceByName(pszDevice);
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

	SetHRESULTError(pInfo->hr, pInfo->pszText);
	SetErrorAdvise(pInfo->pszAdvise);
	SetErrorSystemMessage(pInfo->pszSystemMessage);

	return S_OK;
}


STDMETHODIMP CTSProcessor::OutLog(Interface::LogType Type, LPCWSTR pszMessage)
{
	if (pszMessage == nullptr)
		return E_POINTER;

	LibISDB::Logger::LogType LogType;

	switch (Type) {
	case Interface::LOG_VERBOSE:
		LogType = LibISDB::Logger::LogType::Verbose;
		break;
	case Interface::LOG_INFO:
	default:
		LogType = LibISDB::Logger::LogType::Information;
		break;
	case Interface::LOG_WARNING:
		LogType = LibISDB::Logger::LogType::Warning;
		break;
	case Interface::LOG_ERROR:
		LogType = LibISDB::Logger::LogType::Error;
		break;
	}

	LogRaw(LogType, pszMessage);

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

	ITSDataBuffer *pTsDataBuffer;

	if (SUCCEEDED(pPacket->QueryInterface(IID_PPV_ARGS(&pTsDataBuffer)))) {
		LibISDB::DataBuffer *pData = pTsDataBuffer->GetDataBuffer();

		if (m_fSourceProcessor) {
			OutputData(pData);
		} else {
#ifndef _DEBUG
			LibISDB::TSPacket *pPacketData = static_cast<LibISDB::TSPacket *>(pData);
#else
			LibISDB::TSPacket *pPacketData = dynamic_cast<LibISDB::TSPacket *>(pData);
			TVTEST_ASSERT(pPacketData != nullptr);
#endif

			if (pPacket->GetModified() == S_OK) {
#ifndef _DEBUG
				pPacketData->ReparsePacket();
#else
				if (pPacketData->ParsePacket() != LibISDB::TSPacket::ParseResult::OK) {
					pTsDataBuffer->Release();
					return E_FAIL;
				}
#endif
			}

			m_OutputSequence.AddData(pPacketData);
		}

		pTsDataBuffer->Release();
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
			if (m_OutputData.SetData(pData, Size) != Size)
				return E_OUTOFMEMORY;
			OutputData(&m_OutputData);
		} else {
			if (Size != LibISDB::TS_PACKET_SIZE)
				return E_FAIL;
			LibISDB::TSPacket &Packet = m_OutputPacket.emplace_back();
			if (Packet.SetData(pData, LibISDB::TS_PACKET_SIZE) != LibISDB::TS_PACKET_SIZE) {
				m_OutputPacket.pop_back();
				return E_OUTOFMEMORY;
			}
			if (Packet.ParsePacket() != LibISDB::TSPacket::ParseResult::OK) {
				m_OutputPacket.pop_back();
				return E_FAIL;
			}
			m_OutputSequence.AddData(&Packet);
		}
	}

	return S_OK;
}


}	// namespace TVTest
