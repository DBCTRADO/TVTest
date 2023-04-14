/**
@file TVTestInterface.h
@brief TVTest インターフェースヘッダ
@version ver.0.0.14
@author DBCTRADO
@details
	TVTest のプラグインで使用するインターフェースを定義したヘッダファイルです。

	このヘッダファイルは Creative Commons Zero (CC0) として、再配布・改変など自由に行って構いません。
	ただし、改変した場合はオリジナルと違う旨を記載して頂けると、混乱がなくていいと思います。
*/

/**
@page tsprocessor TS プロセッサ

@details
	プラグインから TVTest::MESSAGE_REGISTERTSPROCESSOR で TVTest::Interface::ITSProcessor を登録すると、
	TS に対する処理が行えるようになります。

	TVTest::Interface::ITSProcessor は必要に応じて以下のインターフェースを実装し、
	QueryInterface() で取得できるようにします。

	+ TVTest::Interface::IFilterManager
	+ TVTest::Interface::IFilterModule
	+ IPersistPropertyBag
	+ ISpecifyPropertyPages
	+ ISpecifyPropertyPages2

	TVTest::Interface::ITSProcessor の処理の流れは以下のようになります。
@code{.unparsed}
	  TVTest::Interface::ITSProcessor::Initialize()           初期化処理
	    ↓
	  TVTest::Interface::ITSProcessor::StartStreaming() ←┐  ストリーミングの開始
	    ↓                                                │
	  TVTest::Interface::ITSProcessor::InputPacket()      │  ストリームの入力(繰り返し呼ばれる)
	    ↓                                                │
	  TVTest::Interface::ITSProcessor::StopStreaming()  ─┘  ストリーミングの終了
	    ↓
	  TVTest::Interface::ITSProcessor::Finalize()             終了処理
@endcode
	TVTest::Interface::ITSProcessor::InputPacket() で TVTest::Interface::ITSPacket を通じてTSデータが渡されるので、
	TVTest::Interface::ITSProcessor::StartStreaming() で渡された TVTest::Interface::ITSOutput の
	TVTest::Interface::ITSOutput::OutputPacket() を呼び出して TS データを出力します。

	TS プロセッサを登録する際に、接続位置として TVTest::TS_PROCESSOR_CONNECT_POSITION_SOURCE を指定した場合、
	TVTest::Interface::ITSProcessor::InputPacket() に入力されるデータはチューナー等から入力されたストリームそのままであり、
	TVTest::Interface::ITSPacket::GetSize() で取得されるサイズは不定です。
	それ以外の場合、 TVTest::Interface::ITSPacket は 188 バイトのパケットデータを表します。

	なお、 TVTest::Interface::ITSPacket がパケットデータを表す時、パケットのヘッダを書き換えた場合は
	TVTest::Interface::ITSPacket::SetModified() を呼び出して、変更されたことが分かるようにする必要があります。

	TVTest::MESSAGE_REGISTERTSPROCESSOR で TVTest::TSProcessorInfo::ConnectPosition に指定する
	接続位置の関係は以下のようになります。
@code{.unparsed}
	  チューナー等
	    ↓(※1)
	  TVTest::TS_PROCESSOR_CONNECT_POSITION_SOURCE
	    ↓(※1)
	  パケットの切り出し(ストリームがパケット単位に切り出されます)
	    ↓
	  TVTest::TS_PROCESSOR_CONNECT_POSITION_PREPROCESSING
	    ↓
	  TS の解析(PAT/PMT/SDT/NIT/EIT 等の解析が行われます)
	    ↓
	  TVTest::TS_PROCESSOR_CONNECT_POSITION_POSTPROCESSING
	    ├──────────────────────┐
	    ↓                                            ↓
	  TVTest::TS_PROCESSOR_CONNECT_POSITION_VIEWER  TVTest::TS_PROCESSOR_CONNECT_POSITION_RECORDER
	    ↓                                            ↓
	  ビューア                                      レコーダ
@endcode
	(※1) はサイズが不定のストリームで、それ以外は 188 バイトの TS パケットです。
*/


#ifndef TVTEST_INTERFACE_H
#define TVTEST_INTERFACE_H


#include <pshpack1.h>


/**
@brief TVTest 名前空間
*/
namespace TVTest
{

/**
@brief Interface 名前空間
*/
namespace Interface
{


/**
@brief デバイスの識別子
*/
typedef ULONG DeviceIDType;

/**
@brief ログの種類
*/
enum LogType
{
	LOG_VERBOSE, /**< 付加的情報 */
	LOG_INFO,    /**< 情報 */
	LOG_WARNING, /**< 警告 */
	LOG_ERROR    /**< エラー */
};

/**
@brief 通知の種類
*/
enum NotifyType
{
	NOTIFY_INFO,    /**< 情報 */
	NOTIFY_WARNING, /**< 警告 */
	NOTIFY_ERROR    /**< エラー */
};

/**
@brief エラーの情報
*/
struct ErrorInfo
{
	HRESULT hr;               /**< エラーコード */
	LPCWSTR pszText;          /**< エラー説明 */
	LPCWSTR pszAdvise;        /**< アドバイス */
	LPCWSTR pszSystemMessage; /**< システムエラーメッセージ */
};

/**
@brief フィルタモジュールの情報
*/
struct FilterModuleInfo
{
	BSTR Name;    /**< 名前 */
	BSTR Version; /**< バージョン */
};

/**
@brief フィルタデバイスの情報
*/
struct FilterDeviceInfo
{
	DeviceIDType DeviceID; /**< デバイス識別子 */
	ULONG Flags;           /**< フラグ */
	BSTR Name;             /**< 名前 */
	BSTR Text;             /**< テキスト */
};

/**
@brief ストリーミングクライアントインターフェース
*/
MIDL_INTERFACE("D513D10B-9438-4613-9758-2C17C434BE36") IStreamingClient : public IUnknown
{
	/**
	@brief エラーの通知
	@param[in] pInfo エラーの情報
	@return エラーコード
	*/
	STDMETHOD(OnError)(const ErrorInfo *pInfo) = 0;

	/**
	@brief ログの記録
	@param[in] Type ログの種類
	@param[in] pszMessage メッセージ
	@return エラーコード
	*/
	STDMETHOD(OutLog)(LogType Type, LPCWSTR pszMessage) = 0;

	/**
	@brief 通知
	@param[in] Type 通知の種類
	@param[in] pszMessage メッセージ
	@return エラーコード
	*/
	STDMETHOD(Notify)(NotifyType Type, LPCWSTR pszMessage) = 0;
};

/**
@brief TS パケットインターフェース
*/
MIDL_INTERFACE("558F0FBF-992E-490F-8BAE-6CB709801B1D") ITSPacket : public IUnknown
{
	/**
	@brief データの取得
	@param[out] ppData データへのポインタを返す変数へのポインタ
	@return エラーコード
	*/
	STDMETHOD(GetData)(BYTE **ppData) = 0;

	/**
	@brief データのサイズの取得
	@param[out] pSize サイズを返す変数へのポインタ
	@return エラーコード
	*/
	STDMETHOD(GetSize)(ULONG *pSize) = 0;

	/**
	@brief データの変更フラグを設定
	@param[in] fModified 変更状態にする場合は TRUE、変更状態を解除する場合は FALSE
	@return エラーコード
	*/
	STDMETHOD(SetModified)(BOOL fModified) = 0;

	/**
	@brief データの変更状態を取得する
	@retval S_OK データが変更されている
	@retval S_FALSE データが変更されていない
	*/
	STDMETHOD(GetModified)() = 0;
};

/**
@brief TS 出力インターフェース
*/
MIDL_INTERFACE("F34436F9-35CD-4883-B4B4-E6F1CB33BE51") ITSOutput : public IUnknown
{
	/**
	@brief TS パケットを出力する
	@param[in] pPacket 出力するパケット
	@return エラーコード
	*/
	STDMETHOD(OutputPacket)(ITSPacket *pPacket) = 0;
};

/**
@brief TS プロセッサインターフェース
*/
MIDL_INTERFACE("207D79AE-193B-4CD6-8228-456F032820F6") ITSProcessor : public IUnknown
{
	/**
	@brief TS プロセッサの GUID を取得する
	@param[out] pGuid GUID を返す変数へのポインタ
	@return エラーコード
	*/
	STDMETHOD(GetGuid)(GUID *pGuid) = 0;

	/**
	@brief TS プロセッサの名前を取得する
	@param[out] pName 名前の文字列を返す変数へのポインタ。
	                  プラグイン側では SysAllocString() で文字列を確保して返す。
	                  呼び出し側では SysFreeString() で解放する。
	@return エラーコード
	*/
	STDMETHOD(GetName)(BSTR *pName) = 0;

	/**
	@brief TS プロセッサを初期化する
	@param[in] pClient IStreamClient インターフェース
	@return エラーコード
	*/
	STDMETHOD(Initialize)(IStreamingClient *pClient) = 0;

	/**
	@brief TS プロセッサの終了処理を行う
	@return エラーコード
	*/
	STDMETHOD(Finalize)() = 0;

	/**
	@brief ストリーミングを開始する
	@param[in] pOutput TS 出力インターフェース
	@return エラーコード
	*/
	STDMETHOD(StartStreaming)(ITSOutput *pOutput) = 0;

	/**
	@brief ストリーミングを停止する
	@return エラーコード
	*/
	STDMETHOD(StopStreaming)() = 0;

	/**
	@brief TS パケットを受け取る
	@param[in,out] pPacket TS パケット
	@return エラーコード
	*/
	STDMETHOD(InputPacket)(ITSPacket *pPacket) = 0;

	/**
	@brief TS プロセッサをリセットする
	@return エラーコード
	*/
	STDMETHOD(Reset)() = 0;

	/**
	@brief ストリーミングの有効状態を設定する
	@param[in] fEnable ストリーミングを有効にする場合は TRUE、無効にする場合は FALSE
	@return エラーコード
	*/
	STDMETHOD(SetEnableProcessing)(BOOL fEnable) = 0;

	/**
	@brief ストリーミングの有効状態を取得する
	@param[out] pfEnable ストリーミングの有効状態を受け取る変数へのポインタ
	@return エラーコード
	*/
	STDMETHOD(GetEnableProcessing)(BOOL *pfEnable) = 0;

	/**
	@brief アクティブなサービス ID を設定する
	@param[in] ServiceID サービス ID
	@return エラーコード
	*/
	STDMETHOD(SetActiveServiceID)(WORD ServiceID) = 0;

	/**
	@brief アクティブなサービス ID を取得する
	@param[out] pServiceID サービス ID を返す変数へのポインタ
	@return エラーコード
	*/
	STDMETHOD(GetActiveServiceID)(WORD *pServiceID) = 0;
};

/**
@brief フィルタデバイスインターフェース
*/
MIDL_INTERFACE("57ECE161-0DC3-4309-8601-F2144B6D5A59") IFilterDevice : public IUnknown
{
	/**
	@brief デバイスの情報を取得する
	@param[out] pInfo デバイスの情報を取得する変数へのポインタ
	@return エラーコード
	*/
	STDMETHOD(GetDeviceInfo)(FilterDeviceInfo *pInfo) = 0;

	/**
	@brief フィルタの数を取得する
	@param[out] pCount フィルタの数を取得する変数へのポインタ
	@return エラーコード
	*/
	STDMETHOD(GetFilterCount)(int *pCount) = 0;

	/**
	@brief フィルタ名を取得する
	@param[in] Index フィルタのインデックス
	@param[out] pName フィルタ名を返す変数へのポインタ。
	                  プラグイン側では SysAllocString() で文字列を確保して返す。
	                  呼び出し側では SysFreeString() で解放する。
	@return エラーコード
	*/
	STDMETHOD(GetFilterName)(int Index, BSTR *pName) = 0;

	/**
	@brief フィルタが利用可能かを取得する
	@param[in] pszName フィルタ名
	@param[out] pfAvailable フィルタが利用可能かの値を取得する変数へのポインタ
	@return エラーコード
	*/
	STDMETHOD(IsFilterAvailable)(LPCWSTR pszName, BOOL *pfAvailable) = 0;
};

/**
@brief フィルタインターフェース
*/
MIDL_INTERFACE("83360AD2-B875-49d4-8EA6-3A4840AC2CC5") IFilter : public IUnknown
{
	/**
	@brief デバイス識別子を取得する
	@param[out] pDeviceID デバイス識別子を返す変数へのポインタ
	@return エラーコード
	*/
	STDMETHOD(GetDeviceID)(DeviceIDType *pDeviceID) = 0;

	/**
	@brief フィルタ名を取得する
	@param[out] pName フィルタ名を返す変数へのポインタ。
	                  プラグイン側では SysAllocString() で文字列を確保して返す。
	                  呼び出し側では SysFreeString() で解放する。
	@return エラーコード
	*/
	STDMETHOD(GetName)(BSTR *pName) = 0;

	/**
	@brief コマンドを送信する
	@param[in] pSendData 送信するデータ
	@param[in] SendSize 送信するデータのサイズ(バイト単位)
	@param[out] pRecvData 受信したデータを格納するバッファへのポインタ
	@param[in,out] pRecvSize pRecvData のバッファのサイズ(バイト単位)を格納した変数へのポインタ。
	                         実際に受信したデータのサイズが返される。
	@return エラーコード
	*/
	STDMETHOD(SendCommand)(const void *pSendData, DWORD SendSize, void *pRecvData, DWORD *pRecvSize) = 0;
};

/**
@brief フィルタモジュールインターフェース
*/
MIDL_INTERFACE("F9D074A9-5730-489e-AC14-7A37D29E8B3E") IFilterModule : public IUnknown
{
	/**
	@brief モジュールを読み込む
	@param[in] pszName 読み込むモジュール名
	@return エラーコード
	*/
	STDMETHOD(LoadModule)(LPCWSTR pszName) = 0;

	/**
	@brief モジュールを解放する
	@return エラーコード
	*/
	STDMETHOD(UnloadModule)() = 0;

	/**
	@brief モジュールが読み込まれているか取得する
	@retval S_OK モジュールが読み込まれている
	@retval S_FALSE モジュールが読み込まれていない
	*/
	STDMETHOD(IsModuleLoaded)() = 0;

	/**
	@brief モジュールの情報を取得する
	@param[out] pInfo モジュールの情報を返す変数へのポインタ
	@return エラーコード
	*/
	STDMETHOD(GetModuleInfo)(FilterModuleInfo *pInfo) = 0;

	/**
	@brief デバイスの数を取得する
	@param[out] pCount デバイスの数を返す変数へのポインタ
	@return エラーコード
	*/
	STDMETHOD(GetDeviceCount)(int *pCount) = 0;

	/**
	@brief デバイスを取得する
	@param[in] Device デバイスのインデックス
	@param[out] ppDevice デバイスのインターフェースを受け取る変数へのポインタ
	@return エラーコード
	*/
	STDMETHOD(GetDevice)(int Device, IFilterDevice **ppDevice) = 0;

	/**
	@brief デバイスが利用可能かを取得する
	@param[in] Device デバイスのインデックス
	@param[out] pfAvailable デバイスが利用可能かを返す変数へのポインタ
	@return エラーコード
	*/
	STDMETHOD(IsDeviceAvailable)(int Device, BOOL *pfAvailable) = 0;

	/**
	@brief デバイスが利用可能かを試す
	@param[in] Device デバイスのインデックス
	@param[out] pfAvailable デバイスが利用可能かを返す変数へのポインタ
	@param[out] pMessage メッセージを返す変数へのポインタ。
	                     プラグイン側では SysAllocString() で文字列を確保して返す。
	                     呼び出し側では SysFreeString() で解放する。
	@return エラーコード
	*/
	STDMETHOD(CheckDeviceAvailability)(int Device, BOOL *pfAvailable, BSTR *pMessage) = 0;

	/**
	@brief デフォルトのデバイスを取得する
	@param[out] pDevice デフォルトのデバイスのインデックスを返す変数へのポインタ
	@return エラーコード
	*/
	STDMETHOD(GetDefaultDevice)(int *pDevice) = 0;

	/**
	@brief デバイスの識別子からインデックスを取得する
	@param[in] DeviceID デバイス識別子
	@param[out] pDevice デバイスのインデックスを返す変数へのポインタ
	@return エラーコード
	*/
	STDMETHOD(GetDeviceByID)(DeviceIDType DeviceID, int *pDevice) = 0;

	/**
	@brief デバイスの名前からインデックスを取得する
	@param[in] pszName デバイス名
	@param[out] pDevice デバイスのインデックスを返す変数へのポインタ
	@return エラーコード
	*/
	STDMETHOD(GetDeviceByName)(LPCWSTR pszName, int *pDevice) = 0;

	/**
	@brief フィルタを開く
	@param[in] Device デバイスのインデックス
	@param[in] pszName フィルタ名
	@param[out] ppFilter フィルタのインターフェースを返す変数へのポインタ
	@return エラーコード
	*/
	STDMETHOD(OpenFilter)(int Device, LPCWSTR pszName, IFilter **ppFilter) = 0;
};

/**
@brief フィルタモジュール列挙インターフェース
*/
MIDL_INTERFACE("ADABE110-5913-418d-8C31-BF6E39F67F28") IEnumFilterModule : public IUnknown
{
	/**
	@brief フィルタモジュールを列挙する
	@param[out] pName フィルタモジュール名を返す変数へのポインタ。
	                  プラグイン側では SysAllocString() で文字列を確保して返す。
	                  呼び出し側では SysFreeString() で解放する。
	@return エラーコード
	*/
	STDMETHOD(Next)(BSTR *pName) = 0;
};

/**
@brief フィルタ管理インターフェース
*/
MIDL_INTERFACE("5CB26641-E1B3-4E8E-884A-8BEF5240B7EC") IFilterManager : public IUnknown
{
	/**
	@brief モジュール列挙インターフェースを作成する
	@param[out] ppEnumModule 作成した IEnumFilterModule へのポインタを返す変数へのポインタ
	@return エラーコード
	*/
	STDMETHOD(EnumModules)(IEnumFilterModule **ppEnumModule) = 0;

	/**
	@brief フィルタモジュールを作成する
	@param[out] ppModule 作成した IFilterModule へのポインタを返す変数へのポインタ
	@return エラーコード
	*/
	STDMETHOD(CreateModule)(IFilterModule **ppModule) = 0;
};


}	// namespace Interface

}	// namespace TVTest


#include <poppack.h>


#ifndef TVTEST_INTERFACE_NO_EXTERNAL

#include <ocidl.h>

/**
@brief プロパティページ作成インターフェース
*/
MIDL_INTERFACE("03481710-D73E-4674-839F-03EDE2D60ED8") ISpecifyPropertyPages2 : public ISpecifyPropertyPages
{
	/**
	@brief プロパティページを作成する
	@param[in] guid 作成するページの GUID
	@param[out] ppPage 作成したページの IPropertyPage インターフェースへのポインタを返す変数へのポインタ
	@return エラーコード
	*/
	STDMETHOD(CreatePage)(const GUID &guid, IPropertyPage **ppPage) = 0;
};

#endif


#endif	// TVTEST_INTERFACE_H
