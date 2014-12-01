#ifndef TVTEST_TS_PROCESSOR_H
#define TVTEST_TS_PROCESSOR_H


#include "TVTestInterface.h"
#include "ComUtility.h"
#include "BonTsEngine/MediaDecoder.h"
#include "BonTsEngine/TsStream.h"
#include <vector>


namespace TVTest
{

	MIDL_INTERFACE("9E0B0063-94EA-45ed-A736-6BC51A4AB5EF") ITsEnginePacket : public IUnknown
	{
		STDMETHOD(SetTsPacket)(CTsPacket *pPacket) = 0;
		STDMETHOD_(CTsPacket*, GetTsPacket)() = 0;
	};

	class CTSPacketInterface
		: public Interface::ITSPacket
		, public ITsEnginePacket
		, protected CIUnknownImpl
	{
	public:
		CTSPacketInterface();

	// IUnknown
		STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject) override;
		STDMETHODIMP_(ULONG) AddRef() override { return AddRefImpl(); }
		STDMETHODIMP_(ULONG) Release() override { return ReleaseImpl(); }

	// ITSPacket
		STDMETHODIMP GetData(BYTE **ppData) override;
		STDMETHODIMP GetSize(ULONG *pSize) override;
		STDMETHODIMP SetModified(BOOL fModified) override;
		STDMETHODIMP GetModified() override;

	// ITsEnginePacket
		STDMETHODIMP SetTsPacket(CTsPacket *pPacket) override;
		STDMETHODIMP_(CTsPacket*) GetTsPacket() override { return m_pPacket; }

	protected:
		CTsPacket *m_pPacket;
		bool m_fModified;

		~CTSPacketInterface();
	};

	class CTSProcessor
		: public CMediaDecoder
		, protected Interface::IStreamingClient
		, protected Interface::ITSOutput
		, protected CIUnknownImpl
	{
	public:
		class CEventHandler
		{
		public:
			virtual void OnFinalize(CTSProcessor *pTSProcessor) {}
			virtual void OnNotify(CTSProcessor *pTSProcessor,
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

		struct ModuleDeviceInfo : public FilterDeviceInfo
		{
			std::vector<String> FilterList;
		};

		struct ModuleInfo
		{
			std::vector<ModuleDeviceInfo> DeviceList;
		};

		CTSProcessor(Interface::ITSProcessor *pTSProcessor,
					 IEventHandler *pEventHandler = nullptr);

	// CMediaDecoder
		bool Initialize() override;
		void Finalize() override;
		void Reset() override;
		const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex) override;
		bool SetActiveServiceID(WORD ServiceID) override;
		WORD GetActiveServiceID() const override;

	// IUnknown
		STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject) override;
		STDMETHODIMP_(ULONG) AddRef() override { return AddRefImpl(); }
		STDMETHODIMP_(ULONG) Release() override {return ReleaseImpl(); }

	// CTSProcessor
		bool GetGuid(GUID *pGuid) const;
		bool GetName(String *pName) const;

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
		const TVTest::String &GetModuleName() const { return m_ModuleName; }

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
		Interface::IFilterManager *m_pFilterManager;
		Interface::IFilterModule *m_pFilterModule;
		Interface::IFilter *m_pFilter;
		CTSPacketInterface *m_pTSPacket;
		CTsPacket m_OutputPacket;
		CEventHandler *m_pEventHandler;
		String m_ModuleName;
		int m_CurDevice;
		String m_CurFilter;

		~CTSProcessor();

	// IStreamingClient
		STDMETHODIMP OnError(const Interface::ErrorInfo *pInfo) override;
		STDMETHODIMP OutLog(Interface::LogType Type, LPCWSTR pszMessage) override;
		STDMETHODIMP Notify(Interface::NotifyType Type, LPCWSTR pszMessage) override;

	// ITSOutput
		STDMETHODIMP OutputPacket(Interface::ITSPacket *pPacket) override;
	};

}	// namespace TVTest


#endif
