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


#ifndef TVTEST_TS_PROCESSOR_H
#define TVTEST_TS_PROCESSOR_H


#include "TVTestInterface.h"
#include "ComUtility.h"
#include "LibISDB/LibISDB/Filters/FilterBase.hpp"
#include "LibISDB/LibISDB/TS/TSPacket.hpp"
#include <vector>


namespace TVTest
{

	MIDL_INTERFACE("9E0B0063-94EA-45ed-A736-6BC51A4AB5EF") ITSDataBuffer
		: public IUnknown
	{
		STDMETHOD(SetDataBuffer)(LibISDB::DataBuffer *pData) = 0;
		STDMETHOD_(LibISDB::DataBuffer*, GetDataBuffer)() = 0;
	};

	class CTSPacketInterface
		: public Interface::ITSPacket
		, public ITSDataBuffer
		, protected CIUnknownImpl
	{
	public:
	// IUnknown
		STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject) override;
		STDMETHODIMP_(ULONG) AddRef() override { return AddRefImpl(); }
		STDMETHODIMP_(ULONG) Release() override { return ReleaseImpl(); }

	// ITSPacket
		STDMETHODIMP GetData(BYTE **ppData) override;
		STDMETHODIMP GetSize(ULONG *pSize) override;
		STDMETHODIMP SetModified(BOOL fModified) override;
		STDMETHODIMP GetModified() override;

	// ITSDataBuffer
		STDMETHODIMP SetDataBuffer(LibISDB::DataBuffer *pData) override;
		STDMETHODIMP_(LibISDB::DataBuffer*) GetDataBuffer() override { return m_pData; }

	protected:
		LibISDB::DataBuffer *m_pData = nullptr;
		bool m_fModified = false;

		~CTSPacketInterface() = default;
	};

	class CTSProcessor
		: public LibISDB::SingleIOFilter
		, protected Interface::IStreamingClient
		, protected Interface::ITSOutput
		, protected CIUnknownImpl
	{
	public:
		class CEventHandler
		{
		public:
			virtual void OnFinalize(CTSProcessor *pTSProcessor) {}
			virtual void OnNotify(
				CTSProcessor *pTSProcessor,
				Interface::NotifyType Type, LPCWSTR pszMessage) {}
		};

		struct FilterModuleInfo
		{
			String Name;
			String Version;
		};

		struct FilterDeviceInfo
		{
			Interface::DeviceIDType DeviceID;
			ULONG Flags;
			String Name;
			String Text;
		};

		struct ModuleDeviceInfo
			: public FilterDeviceInfo
		{
			std::vector<String> FilterList;
		};

		struct ModuleInfo
		{
			std::vector<ModuleDeviceInfo> DeviceList;
		};

		CTSProcessor(Interface::ITSProcessor *pTSProcessor);

	// LibISDB::ObjectBase
		const LibISDB::CharType * GetObjectName() const noexcept override { return LIBISDB_STR("TSProcessor"); }

	// LibISDB::FilterBase
		bool Initialize() override;
		void Finalize() override;
		void Reset() override;
		void SetActiveServiceID(uint16_t ServiceID) override;

	// LibISDB::FilterSink
		bool ReceiveData(LibISDB::DataStream *pData) override;

	// IUnknown
		STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject) override;
		STDMETHODIMP_(ULONG) AddRef() override { return AddRefImpl(); }
		STDMETHODIMP_(ULONG) Release() override {return ReleaseImpl(); }

	// CTSProcessor
		bool GetGuid(GUID *pGuid) const;
		bool GetName(String *pName) const;

		void SetSourceProcessor(bool fSource);
		bool GetSourceProcessor() const { return m_fSourceProcessor; }

		bool IsPropertyBagSupported() const;
		bool LoadProperties(IPropertyBag *pPropBag);
		bool SaveProperties(IPropertyBag *pPropBag);
		bool IsPropertyPageSupported() const;
		bool ShowPropertyPage(HWND hwndOwner, HINSTANCE hinst);

		bool IsFilterModuleSupported() const;
		bool GetModuleList(std::vector<String> *pList) const;
		bool LoadModule(LPCWSTR pszName);
		void UnloadModule();
		bool IsModuleLoaded() const;
		bool GetModuleInfo(FilterModuleInfo *pInfo) const;
		bool GetModuleInfo(LPCWSTR pszModule, ModuleInfo *pInfo) const;
		const String &GetModuleName() const { return m_ModuleName; }

		int GetDeviceCount() const;
		bool GetDeviceInfo(int Device, FilterDeviceInfo *pInfo) const;
		bool GetDeviceName(int Device, String *pName) const;
		bool GetDeviceList(std::vector<String> *pList) const;
		bool GetDeviceFilterList(int Device, std::vector<String> *pList) const;
		bool IsDeviceAvailable(int Device);
		bool CheckDeviceAvailability(int Device, bool *pfAvailable, String *pMessage);
		int GetDefaultDevice() const;
		int GetDeviceByID(DWORD DeviceID) const;
		int GetDeviceByName(LPCWSTR pszName) const;

		bool OpenFilter(int Device, LPCWSTR pszName);
		bool OpenFilter(LPCWSTR pszDevice, LPCWSTR pszName);
		void CloseFilter();
		bool IsFilterOpened() const;
		int GetDevice() const;
		bool GetFilterName(String *pName) const;
		bool SendFilterCommand(const void *pSendData, DWORD SendSize, void *pRecvData, DWORD *pRecvSize);

		bool SetEnableProcessing(bool fEnable);
		bool GetEnableProcessing(bool *pfEnable) const;

		void SetEventHandler(CEventHandler *pEventHandler);

	protected:
		Interface::ITSProcessor *m_pTSProcessor;
		Interface::IFilterManager *m_pFilterManager = nullptr;
		Interface::IFilterModule *m_pFilterModule = nullptr;
		Interface::IFilter *m_pFilter = nullptr;
		CTSPacketInterface *m_pTSPacket;
		LibISDB::DataBuffer m_OutputData;
		std::vector<LibISDB::TSPacket> m_OutputPacket;
		LibISDB::DataStreamSequence<LibISDB::DataBuffer *> m_OutputSequence;
		bool m_fSourceProcessor = false;
		CEventHandler *m_pEventHandler = nullptr;
		String m_ModuleName;
		int m_CurDevice = -1;
		String m_CurFilter;

		~CTSProcessor();

	// IStreamingClient
		STDMETHODIMP OnError(const Interface::ErrorInfo *pInfo) override;
		STDMETHODIMP OutLog(Interface::LogType Type, LPCWSTR pszMessage) override;
		STDMETHODIMP Notify(Interface::NotifyType Type, LPCWSTR pszMessage) override;

	// ITSOutput
		STDMETHODIMP OutputPacket(Interface::ITSPacket *pPacket) override;
	};

} // namespace TVTest


#endif
