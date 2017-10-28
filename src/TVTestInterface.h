/*
	TVTest インターフェースヘッダ ver.0.0.14

	このファイルは再配布・改変など自由に行って構いません。
	ただし、改変した場合はオリジナルと違う旨を記載して頂けると、混乱がなくてい
	いと思います。
*/

/*
	プラグインから MESSAGE_REGISTERTSPROCESSOR で ITSProcessor を登録すると、
	TS に対する処理が行えるようになります。

	ITSProcessor は必要に応じて以下のインターフェースを実装し、
	QueryInterface() で取得できるようにします。

	・IFilterManager
	・IFilterModule
	・IPersistPropertyBag
	・ISpecifyPropertyPages
	・ISpecifyPropertyPages2

	ITSProcessor の処理の流れは以下のようになります。

	  ITSProcessor::Initialize()           初期化処理
	    ↓
	  ITSProcessor::StartStreaming() ←┐  ストリーミングの開始
	    ↓                             │
	  ITSProcessor::InputPacket()      │  ストリームの入力(繰り返し呼ばれる)
	    ↓                             │
	  ITSProcessor::StopStreaming()  ─┘  ストリーミングの終了
	    ↓
	  ITSProcessor::Finalize()             終了処理

	ITSProcessor::InputPacket() で ITSPacket を通じてTSデータが渡されるので、
	ITSProcessor::StartStreaming() で渡された ITSOutput の OutputPacket() を
	呼び出してTSデータを出力します。

	TSプロセッサを登録する際に、接続位置として TS_PROCESSOR_CONNECT_POSITION_SOURCE を
	指定した場合、ITSProcessor::InputPacket() に入力されるデータはチューナー等から
	入力されたストリームそのままであり、ITSPacket::GetSize() で取得されるサイズは不定です。
	それ以外の場合、ITSPacket は188バイトのパケットデータを表します。

	なお、ITSPacket がパケットデータを表す時、パケットのヘッダを書き換えた場合は
	ITSPacket::SetModified() を呼び出して、変更されたことが分かるようにする必要があります。

	MESSAGE_REGISTERTSPROCESSOR で TSProcessorInfo::ConnectPosition に指定する
	接続位置の関係は以下のようになります。

	  チューナー等
	    ↓(*1)
	  TS_PROCESSOR_CONNECT_POSITION_SOURCE
	    ↓(*1)
	  パケットの切り出し(ストリームがパケット単位に切り出されます)
	    ↓
	  TS_PROCESSOR_CONNECT_POSITION_PREPROCESSING
	    ↓
	  TSの解析(PAT/PMT/SDT/NIT/EIT 等の解析が行われます)
	    ↓
	  TS_PROCESSOR_CONNECT_POSITION_POSTPROCESSING
	    ├──────────────────┐
	    ↓                                    ↓
	  TS_PROCESSOR_CONNECT_POSITION_VIEWER  TS_PROCESSOR_CONNECT_POSITION_RECORDER
	    ↓                                    ↓
	  ビューア                              レコーダ

	(*1) はサイズが不定のストリームで、それ以外は188バイトのTSパケットです。
*/


#ifndef TVTEST_INTERFACE_H
#define TVTEST_INTERFACE_H


#include <pshpack1.h>


namespace TVTest
{

namespace Interface
{


typedef ULONG DeviceIDType;

enum LogType
{
	LOG_VERBOSE,
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR
};

enum NotifyType
{
	NOTIFY_INFO,
	NOTIFY_WARNING,
	NOTIFY_ERROR
};

struct ErrorInfo
{
	HRESULT hr;
	LPCWSTR pszText;
	LPCWSTR pszAdvise;
	LPCWSTR pszSystemMessage;
};

struct FilterModuleInfo
{
	BSTR Name;
	BSTR Version;
};

struct FilterDeviceInfo
{
	DeviceIDType DeviceID;
	ULONG Flags;
	BSTR Name;
	BSTR Text;
};

MIDL_INTERFACE("D513D10B-9438-4613-9758-2C17C434BE36") IStreamingClient : public IUnknown
{
	STDMETHOD(OnError)(const ErrorInfo *pInfo) = 0;
	STDMETHOD(OutLog)(LogType Type, LPCWSTR pszMessage) = 0;
	STDMETHOD(Notify)(NotifyType Type, LPCWSTR pszMessage) = 0;
};

MIDL_INTERFACE("558F0FBF-992E-490F-8BAE-6CB709801B1D") ITSPacket : public IUnknown
{
	STDMETHOD(GetData)(BYTE **ppData) = 0;
	STDMETHOD(GetSize)(ULONG *pSize) = 0;
	STDMETHOD(SetModified)(BOOL fModified) = 0;
	STDMETHOD(GetModified)() = 0;
};

MIDL_INTERFACE("F34436F9-35CD-4883-B4B4-E6F1CB33BE51") ITSOutput : public IUnknown
{
	STDMETHOD(OutputPacket)(ITSPacket *pPacket) = 0;
};

MIDL_INTERFACE("207D79AE-193B-4CD6-8228-456F032820F6") ITSProcessor : public IUnknown
{
	STDMETHOD(GetGuid)(GUID *pGuid) = 0;
	STDMETHOD(GetName)(BSTR *pName) = 0;
	STDMETHOD(Initialize)(IStreamingClient *pClient) = 0;
	STDMETHOD(Finalize)() = 0;
	STDMETHOD(StartStreaming)(ITSOutput *pOutput) = 0;
	STDMETHOD(StopStreaming)() = 0;
	STDMETHOD(InputPacket)(ITSPacket *pPacket) = 0;
	STDMETHOD(Reset)() = 0;
	STDMETHOD(SetEnableProcessing)(BOOL fEnable) = 0;
	STDMETHOD(GetEnableProcessing)(BOOL *pfEnable) = 0;
	STDMETHOD(SetActiveServiceID)(WORD ServiceID) = 0;
	STDMETHOD(GetActiveServiceID)(WORD *pServiceID) = 0;
};

MIDL_INTERFACE("57ECE161-0DC3-4309-8601-F2144B6D5A59") IFilterDevice : public IUnknown
{
	STDMETHOD(GetDeviceInfo)(FilterDeviceInfo *pInfo) = 0;
	STDMETHOD(GetFilterCount)(int *pCount) = 0;
	STDMETHOD(GetFilterName)(int Index, BSTR *pName) = 0;
	STDMETHOD(IsFilterAvailable)(LPCWSTR pszName, BOOL *pfAvailable) = 0;
};

MIDL_INTERFACE("83360AD2-B875-49d4-8EA6-3A4840AC2CC5") IFilter : public IUnknown
{
	STDMETHOD(GetDeviceID)(DeviceIDType *pDeviceID) = 0;
	STDMETHOD(GetName)(BSTR *pName) = 0;
	STDMETHOD(SendCommand)(const void *pSendData, DWORD SendSize, void *pRecvData, DWORD *pRecvSize) = 0;
};

MIDL_INTERFACE("F9D074A9-5730-489e-AC14-7A37D29E8B3E") IFilterModule : public IUnknown
{
	STDMETHOD(LoadModule)(LPCWSTR pszName) = 0;
	STDMETHOD(UnloadModule)() = 0;
	STDMETHOD(IsModuleLoaded)() = 0;
	STDMETHOD(GetModuleInfo)(FilterModuleInfo *pInfo) = 0;
	STDMETHOD(GetDeviceCount)(int *pCount) = 0;
	STDMETHOD(GetDevice)(int Device, IFilterDevice **ppDevice) = 0;
	STDMETHOD(IsDeviceAvailable)(int Device, BOOL *pfAvailable) = 0;
	STDMETHOD(CheckDeviceAvailability)(int Device, BOOL *pfAvailable, BSTR *pMessage) = 0;
	STDMETHOD(GetDefaultDevice)(int *pDevice) = 0;
	STDMETHOD(GetDeviceByID)(DeviceIDType DeviceID, int *pDevice) = 0;
	STDMETHOD(GetDeviceByName)(LPCWSTR pszName, int *pDevice) = 0;
	STDMETHOD(OpenFilter)(int Device, LPCWSTR pszName, IFilter **ppFilter) = 0;
};

MIDL_INTERFACE("ADABE110-5913-418d-8C31-BF6E39F67F28") IEnumFilterModule : public IUnknown
{
	STDMETHOD(Next)(BSTR *pName) = 0;
};

MIDL_INTERFACE("5CB26641-E1B3-4E8E-884A-8BEF5240B7EC") IFilterManager : public IUnknown
{
	STDMETHOD(EnumModules)(IEnumFilterModule **ppEnumModule) = 0;
	STDMETHOD(CreateModule)(IFilterModule **ppModule) = 0;
};


}	// namespace Interface

}	// namespace TVTest


#include <poppack.h>


#ifndef TVTEST_INTERFACE_NO_EXTERNAL

#include <ocidl.h>

MIDL_INTERFACE("03481710-D73E-4674-839F-03EDE2D60ED8") ISpecifyPropertyPages2 : public ISpecifyPropertyPages
{
	STDMETHOD(CreatePage)(const GUID &guid, IPropertyPage **ppPage) = 0;
};

#endif


#endif	// TVTEST_INTERFACE_H
