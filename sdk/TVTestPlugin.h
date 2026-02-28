/**
@file TVTestPlugin.h
@brief TVTest プラグインヘッダ
@version ver.0.0.15-pre
@author DBCTRADO
@details
	TVTest のプラグインを作成するためのヘッダファイルです。

	このヘッダは C++11 の規格に沿って記述しています。
	windows.h に依存していますがインクルードは行っていないので、
	利用する側でこのヘッダをインクルードする前に \#include <windows.h> を行ってください。

	実際にプラグインを作成する場合、このヘッダだけを見ても恐らく意味不明だと思いますので、サンプルを参考にしてください。

	このヘッダファイルは Creative Commons Zero (CC0) として、再配布・改変など自由に行って構いません。
	ただし、改変した場合はオリジナルと違う旨を記載して頂けると、混乱がなくていいと思います。
*/

/*
	このファイルは Doxygen 形式でコメントを記述しています。
	添付の Doxyfile を使って HTML のドキュメントを生成できます。
*/

/**
@mainpage TVTest プラグインの概要

@details
	プラグインは 32 / 64 ビット DLL の形式です。拡張子は .tvtp とします。
	プラグインでは、以下の4つの関数をエクスポートします。
@code{.cpp}
	DWORD WINAPI TVTGetVersion();
	BOOL WINAPI TVTGetPluginInfo(TVTest::PluginInfo *pInfo);
	BOOL WINAPI TVTInitialize(TVTest::PluginParam *pParam);
	BOOL WINAPI TVTFinalize();
@endcode
	各関数については、それぞれの関数のドキュメントを参照してください。

	プラグインからは、コールバック関数を通じてメッセージを送信することにより、TVTest の機能を利用することができます。
	このような方法になっているのは、将来的な拡張が容易であるためです。

	また、イベントコールバック関数を登録することにより、TVTest からイベントが通知されます。

	TVTEST_PLUGIN_CLASS_IMPLEMENT シンボルが \#define されていると、エクスポート関数を直接記述しなくても、
	クラスとしてプラグインを記述することができます。

	その場合、 TVTest::CTVTestPlugin クラスからプラグインクラスを派生させます。
	また、 CreatePluginClass() 関数を実装して、プラグインクラスのインスタンスを new で生成して返します。

	以下は最低限の実装を行ったサンプルです。
@code{.cpp}
	#include <windows.h>
	#define TVTEST_PLUGIN_CLASS_IMPLEMENT // プラグインをクラスとして実装
	#include "TVTestPlugin.h"

	// プラグインクラス
	// CTVTestPlugin を基底クラスとする
	class CMyPlugin : public TVTest::CTVTestPlugin
	{
	public:
		bool GetPluginInfo(TVTest::PluginInfo *pInfo) override
		{
			// プラグインの情報を返す
			pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
			pInfo->Flags          = TVTest::PLUGIN_FLAG_NONE;
			pInfo->pszPluginName  = L"サンプル";
			pInfo->pszCopyright   = L"Copyright(c) 2010 Taro Yamada";
			pInfo->pszDescription = L"何もしないプラグイン";
			return true; // false を返すとプラグインのロードが失敗になる
		}

		bool Initialize() override
		{
			// ここで初期化を行う
			// 何もしないのであればオーバーライドしなくても良い
			return true; // false を返すとプラグインのロードが失敗になる
		}

		bool Finalize() override
		{
			// ここでクリーンアップを行う
			// 何もしないのであればオーバーライドしなくても良い
			return true;
		}
	};

	// CreatePluginClass 関数で、プラグインクラスのインスタンスを生成して返す
	TVTest::CTVTestPlugin * CreatePluginClass()
	{
		return new CMyPlugin;
	}
@endcode
	なお、プラグインが複数のソースファイルからなっている場合、
	TVTEST_PLUGIN_CLASS_IMPLEMENT を定義するのは一つのファイルのみとします。

	TVTest::CTVTestPlugin::m_pApp メンバには TVTest::CTVTestApp クラスのインスタンスが保持されるので、
	それを使って TVTest の機能を呼び出すことができます。
*/


/**
@page history 更新履歴

+ ver.0.0.15 (TVTest ver.0.10.0)
  + 以下のメッセージを追加した
    + MESSAGE_GETDARKMODESTATUS
    + MESSAGE_ISDARKMODECOLOR
    + MESSAGE_SETWINDOWDARKMODE
    + MESSAGE_GETELEMENTARYSTREAMINFOLIST
    + MESSAGE_GETSERVICECOUNT
    + MESSAGE_GETSERVICEINFO2
    + MESSAGE_GETSERVICEINFOLIST
    + MESSAGE_GETAUDIOINFO
    + MESSAGE_GETELEMENTARYSTREAMCOUNT
    + MESSAGE_SELECTAUDIO
    + MESSAGE_GETSELECTEDAUDIO
    + MESSAGE_GETCURRENTEPGEVENTINFO
  + 以下のイベントを追加した
    + EVENT_DARKMODECHANGED
    + EVENT_MAINWINDOWDARKMODECHANGED
    + EVENT_PROGRAMGUIDEDARKMODECHANGED
    + EVENT_VIDEOFORMATCHANGE
    + EVENT_AUDIOFORMATCHANGE
    + EVENT_EVENTCHANGED
    + EVENT_EVENTINFOCHANGED
  + チャンネル選択のフラグに CHANNEL_SELECT_FLAG_ALLOWDISABLED を追加した
  + ダイアログ表示のフラグに SHOW_DIALOG_FLAG_DISABLE_DARK_MODE を追加した
  + 番組表のイベントのフラグに PROGRAMGUIDE_EVENT_COMMAND_ALWAYS を追加した

+ ver.0.0.14 (TVTest ver.0.9.0)
  + 以下のメッセージを追加した
    + MESSAGE_GETSTYLEVALUE
    + MESSAGE_THEMEDRAWBACKGROUND
    + MESSAGE_THEMEDRAWTEXT
    + MESSAGE_THEMEDRAWICON
    + MESSAGE_GETEPGCAPTURESTATUS
    + MESSAGE_GETAPPCOMMANDINFO
    + MESSAGE_GETAPPCOMMANDCOUNT
    + MESSAGE_GETVIDEOSTREAMCOUNT
    + MESSAGE_GETVIDEOSTREAM
    + MESSAGE_SETVIDEOSTREAM
    + MESSAGE_GETLOG
    + MESSAGE_GETLOGCOUNT
    + MESSAGE_REGISTERPLUGINCOMMAND
    + MESSAGE_SETPLUGINCOMMANDSTATE
    + MESSAGE_PLUGINCOMMANDNOTIFY
    + MESSAGE_REGISTERPLUGINICON
    + MESSAGE_REGISTERSTATUSITEM
    + MESSAGE_SETSTATUSITEM
    + MESSAGE_GETSTATUSITEMINFO
    + MESSAGE_STATUSITEMNOTIFY
    + MESSAGE_REGISTERTSPROCESSOR
    + MESSAGE_REGISTERPANELITEM
    + MESSAGE_SETPANELITEM
    + MESSAGE_GETPANELITEMINFO
    + MESSAGE_SELECTCHANNEL
    + MESSAGE_GETFAVORITELIST
    + MESSAGE_FREEFAVORITELIST
    + MESSAGE_GET1SEGMODE
    + MESSAGE_SET1SEGMODE
    + MESSAGE_GETDPI
    + MESSAGE_GETFONT
    + MESSAGE_SHOWDIALOG
    + MESSAGE_CONVERTTIME
    + MESSAGE_SETVIDEOSTREAMCALLBACK
    + MESSAGE_GETVARSTRINGCONTEXT
    + MESSAGE_FREEVARSTRINGCONTEXT
    + MESSAGE_FORMATVARSTRING
    + MESSAGE_REGISTERVARIABLE
  + 以下のイベントを追加した
    + EVENT_FILTERGRAPH_INITIALIZE
    + EVENT_FILTERGRAPH_INITIALIZED
    + EVENT_FILTERGRAPH_FINALIZE
    + EVENT_FILTERGRAPH_FINALIZED
    + EVENT_DRAWCOMMANDICON
    + EVENT_STATUSITEM_DRAW
    + EVENT_STATUSITEM_NOTIFY
    + EVENT_STATUSITEM_MOUSE
    + EVENT_PANELITEM_NOTIFY
    + EVENT_FAVORITESCHANGED
    + EVENT_1SEGMODECHANGED
  + プラグインのフラグに PLUGIN_FLAG_NOENABLEDDISABLED を追加した
  + 録画情報のフラグに RECORD_FLAG_UTC を追加した。
  + MESSAGE_GETRECORDSTATUS にフラグの指定を追加した。
  + MESSAGE_GETSETTING で取得できる設定に以下を追加した。
    + RecordFileName
    + CaptureFolder
    + CaptureFileName

+ ver.0.0.13 (TVTest ver.0.7.16)
  + 以下のメッセージを追加した
    + MESSAGE_ENABLEPROGRAMGUIDEEVENT
    + MESSAGE_REGISTERPROGRAMGUIDECOMMAND
  + 以下のイベントを追加した
    + EVENT_STARTUPDONE
    + EVENT_PROGRAMGUIDE_INITIALIZE
    + EVENT_PROGRAMGUIDE_FINALIZE
    + EVENT_PROGRAMGUIDE_COMMAND
    + EVENT_PROGRAMGUIDE_INITIALIZEMENU
    + EVENT_PROGRAMGUIDE_MENUSELECTED
    + EVENT_PROGRAMGUIDE_PROGRAM_DRAWBACKGROUND
    + EVENT_PROGRAMGUIDE_PROGRAM_INITIALIZEMENU
    + EVENT_PROGRAMGUIDE_PROGRAM_MENUSELECTED

+ ver.0.0.12 (TVTest ver.0.7.14)
  + 以下のメッセージを追加した
    + MESSAGE_GETEPGEVENTINFO
    + MESSAGE_FREEEPGEVENTINFO
    + MESSAGE_GETEPGEVENTLIST
    + MESSAGE_FREEEPGEVENTLIST
    + MESSAGE_ENUMDRIVER
    + MESSAGE_GETDRIVERTUNINGSPACELIST
    + MESSAGE_FREEDRIVERTUNINGSPACELIST
  + ChannelInfo 構造体に Flags メンバを追加した

+ ver.0.0.11 (TVTest ver.0.7.6)
  + 以下のメッセージを追加した
    + MESSAGE_SETWINDOWMESSAGECALLBACK
    + MESSAGE_REGISTERCONTROLLER
    + MESSAGE_ONCOTROLLERBUTTONDOWN
    + MESSAGE_GETCONTROLLERSETTINGS
  + EVENT_CONTROLLERFOCUS を追加した
  + プラグインのフラグに PLUGIN_FLAG_NOUNLOAD を追加した

+ ver.0.0.10 (TVTest ver.0.7.0)
  + 以下のメッセージを追加した
    + MESSAGE_GETLOGO
    + MESSAGE_GETAVAILABLELOGOTYPE
    + MESSAGE_RELAYRECORD
    + MESSAGE_SILENTMODE
  + 以下のイベントを追加した
    + EVENT_CLOSE
    + EVENT_STARTRECORD
    + EVENT_RELAYRECORD
  + プラグインのフラグに PLUGIN_FLAG_DISABLEONSTART を追加した
  + RecordStatusInfo 構造体に pszFileName と MaxFileName メンバを追加した

+ ver.0.0.9.1
  + 64ビットで警告が出ないようにした

+ ver.0.0.9 (TVTest ver.0.6.2)
  + MESSAGE_GETSETTING と MESSAGE_GETDRIVERFULLPATHNAME を追加した
  + EVENT_SETTINGSCHANGE を追加した
  + MESSAGE_RESET にパラメータを追加した

+ ver.0.0.8 (TVTest ver.0.6.0)
  + MESSAGE_GETHOSTINFO を追加した
  + MESSAGE_SETCHANNEL のパラメータにサービスIDを追加した

+ ver.0.0.7 (TVTest ver.0.5.45)
  + MESSAGE_DOCOMMAND を追加した

+ ver.0.0.6 (TVTest ver.0.5.42)
  + MESSAGE_SETAUDIOCALLBACK を追加した
  + EVENT_DSHOWINITIALIZED を追加した

+ ver.0.0.5 (TVTest ver.0.5.41)
  + 以下のイベントを追加した
    + EVENT_RESET
    + EVENT_STATUSRESET
    + EVENT_AUDIOSTREAMCHANGE
  + MESSAGE_RESETSTATUS を追加した

+ ver.0.0.4 (TVTest ver.0.5.33)
  + EVENT_EXECUTE を追加した

+ ver.0.0.3 (TVTest ver.0.5.27)
  + 以下のメッセージを追加した
    + MESSAGE_ISPLUGINENABLED
    + MESSAGE_REGISTERCOMMAND
    + MESSAGE_ADDLOG
  + EVENT_STANDBY と EVENT_COMMAND を追加した

+ ver.0.0.2 (TVTest ver.0.5.13)
  + MESSAGE_GETAUDIOSTREAM と MESSAGE_SETAUDIOSTREAM を追加した
  + ServiceInfo 構造体に AudioComponentType と SubtitlePID メンバを追加した
  + StatusInfo 構造体に DropPacketCount と Reserved メンバを追加した

+ ver.0.0.1 (TVTest ver.0.5.5)
  + 以下のメッセージを追加した
    + MESSAGE_QUERYEVENT
    + MESSAGE_GETTUNINGSPACE
    + MESSAGE_GETTUNINGSPACEINFO
    + MESSAGE_SETNEXTCHANNEL
  + ChannelInfo 構造体にいくつかメンバを追加した

+ ver.0.0.0 (TVTest ver.0.5.4)
  + 最初のバージョン
*/


#ifndef TVTEST_PLUGIN_H
#define TVTEST_PLUGIN_H


#ifndef offsetof
#include <cstddef>
#endif

#include <pshpack1.h>


/**
@brief TVTest 名前空間
*/
namespace TVTest {


/**
@brief プラグイン仕様のバージョンを定義

@param[in] major メジャーバージョン
@param[in] minor マイナーバージョン
@param[in] rev リビジョンナンバー

@return バージョン番号を表す値
*/
#define TVTEST_PLUGIN_VERSION_(major, minor, rev) \
	(((major) << 24) | ((minor) << 12) | (rev))

#ifndef TVTEST_PLUGIN_VERSION
/**
@brief プラグイン仕様のバージョン

プラグイン作成者が定義しなければ、最新のバージョンで定義されます。
*/
#define TVTEST_PLUGIN_VERSION TVTEST_PLUGIN_VERSION_(0, 0, 15)
#endif

/**
@brief エクスポート関数の定義
*/
#define TVTEST_EXPORT(type) extern "C" __declspec(dllexport) type WINAPI

#ifdef interface
/**
@brief COM インターフェースの前方宣言用
*/
#define TVTEST_COM_INTERFACE interface
#else
/**
@brief COM インターフェースの前方宣言用
*/
#define TVTEST_COM_INTERFACE struct
#endif

#ifdef TVTEST_PLUGIN_USE_NAMED_ENUM
/**
@brief 列挙型を名前付きで定義する

TVTEST_PLUGIN_USE_NAMED_ENUM が定義されている場合、列挙型が名前付きで定義されます。
コンパイラが C++11 規格に対応している必要があります。

TVTEST_PLUGIN_USE_NAMED_ENUM が定義されていない場合、列挙型は Anonymous enum で定義され、整数型の別名として型名が定義されます。

@param name 列挙型の型名
@param type 基底型の型名
*/
#define TVTEST_DEFINE_ENUM(name, type) enum name : type
/**
@brief 列挙型のビット操作演算子を定義する
@param name 列挙型の型名
*/
#define TVTEST_DEFINE_ENUM_FLAG_OPERATORS(name) TVTEST_DEFINE_ENUM_FLAG_OPERATORS_(name)
// DEFINE_ENUM_FLAG_OPERATORS はグローバル名前空間で使用しないと他の演算子オーバーロードに干渉する
#define TVTEST_DEFINE_ENUM_FLAG_OPERATORS_(name) } DEFINE_ENUM_FLAG_OPERATORS(TVTest::name) namespace TVTest {
#else
#define TVTEST_DEFINE_ENUM(name, type) typedef type name; enum
#define TVTEST_DEFINE_ENUM_FLAG_OPERATORS(name)
#endif

/**
@brief プラグインの種類

PluginInfo::Type メンバに設定します。
*/
TVTEST_DEFINE_ENUM(PluginType, DWORD) {
	PLUGIN_TYPE_NORMAL /**< 普通 */
};

/**
@brief プラグインのフラグ

PluginInfo::Flags メンバに設定します。
*/
TVTEST_DEFINE_ENUM(PluginFlag, DWORD) {
	PLUGIN_FLAG_NONE                = 0x00000000UL, /**< なし(0) */
	PLUGIN_FLAG_HASSETTINGS         = 0x00000001UL, /**< 設定ダイアログがある */
	PLUGIN_FLAG_ENABLEDEFAULT       = 0x00000002UL  /**< デフォルトで有効 */
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 10)
	/**
	@brief 起動時は必ず無効
	@since ver.0.0.10
	*/
	, PLUGIN_FLAG_DISABLEONSTART    = 0x00000004UL
#endif
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 11)
	/**
	@brief 終了時以外アンロード不可
	@since ver.0.0.11
	*/
	, PLUGIN_FLAG_NOUNLOAD          = 0x00000008UL
#endif
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 14)
	/**
	@brief 有効/無効の区別がない
	@details メニューに表示されず、 TVTest::EVENT_PLUGINENABLE が送られません
	@since ver.0.0.14
	*/
	, PLUGIN_FLAG_NOENABLEDDISABLED = 0x00000010UL
#endif
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(PluginFlag)

/**
@brief プラグインの情報

TVTGetPluginInfo() 関数に渡されます。
*/
struct PluginInfo
{
	PluginType Type;        /**< 種類(PLUGIN_TYPE_*) */
	PluginFlag Flags;       /**< フラグ(PLUGIN_FLAG_* の組み合わせ) */
	LPCWSTR pszPluginName;  /**< プラグイン名 */
	LPCWSTR pszCopyright;   /**< 著作権情報 */
	LPCWSTR pszDescription; /**< 説明文 */
};

/**
@brief メッセージコード

メッセージコールバック関数(MessageCallbackFunc)に渡すことのできるメッセージコードです。
メッセージを使って TVTest の機能を呼び出すことができます。

通常はメッセージコードを直接は扱わず、Msg で始まるユーティリティ関数を利用するか、 CTVTestApp クラスを利用します。
*/
TVTEST_DEFINE_ENUM(MessageCode, UINT) {
	/**
	@brief プログラムのバージョンを取得
	@since ver.0.0.0
	@sa MsgGetVersion()
	@sa CTVTestApp::GetVersion()
	*/
	MESSAGE_GETVERSION,

	/**
	@brief メッセージに対応しているか問い合わせる
	@since ver.0.0.0
	@sa MsgQueryMessage()
	@sa CTVTestApp::QueryMessage()
	*/
	MESSAGE_QUERYMESSAGE,

	/**
	@brief メモリ確保
	@since ver.0.0.0
	@sa MsgMemoryAlloc()
	@sa MsgMemoryFree()
	@sa MsgMemoryReAlloc()
	@sa CTVTestApp::MemoryAlloc()
	@sa CTVTestApp::MemoryFree()
	@sa CTVTestApp::MemoryReAlloc()
	*/
	MESSAGE_MEMORYALLOC,

	/**
	@brief イベントハンドル用コールバックを設定
	@since ver.0.0.0
	@sa MsgSetEventCallback()
	@sa CTVTestApp::SetEventCallback()
	*/
	MESSAGE_SETEVENTCALLBACK,

	/**
	@brief 現在のチャンネルの情報を取得
	@since ver.0.0.0
	@sa MsgGetCurrentChannelInfo()
	@sa CTVTestApp::GetCurrentChannelInfo()
	*/
	MESSAGE_GETCURRENTCHANNELINFO,

	/**
	@brief チャンネルを設定
	@since ver.0.0.0
	@sa MsgSetChannel()
	@sa CTVTestApp::SetChannel()
	*/
	MESSAGE_SETCHANNEL,

	/**
	@brief サービスを取得
	@since ver.0.0.0
	@sa MsgGetService()
	@sa CTVTestApp::GetService()
	*/
	MESSAGE_GETSERVICE,

	/**
	@brief サービスを設定
	@since ver.0.0.0
	@sa MsgSetService()
	@sa CTVTestApp::SetService()
	*/
	MESSAGE_SETSERVICE,

	/**
	@brief チューニング空間名を取得
	@since ver.0.0.0
	@sa MsgGetTuningSpaceName()
	@sa CTVTestApp::GetTuningSpaceName()
	*/
	MESSAGE_GETTUNINGSPACENAME,

	/**
	@brief チャンネルの情報を取得
	@since ver.0.0.0
	@sa MsgGetChannelInfo()
	@sa CTVTestApp::GetChannelInfo()
	*/
	MESSAGE_GETCHANNELINFO,

	/**
	@brief サービスの情報を取得
	@since ver.0.0.0
	@sa MsgGetServiceInfo()
	@sa CTVTestApp::GetServiceInfo()
	*/
	MESSAGE_GETSERVICEINFO,

	/**
	@brief BonDriver のファイル名を取得
	@since ver.0.0.0
	@sa MsgGetDriverName()
	@sa CTVTestApp::GetDriverName()
	*/
	MESSAGE_GETDRIVERNAME,

	/**
	@brief BonDriver を設定
	@since ver.0.0.0
	@sa MsgSetDriverName()
	@sa CTVTestApp::SetDriverName()
	*/
	MESSAGE_SETDRIVERNAME,

	/**
	@brief 録画の開始
	@since ver.0.0.0
	@sa MsgStartRecord()
	@sa CTVTestApp::StartRecord()
	*/
	MESSAGE_STARTRECORD,

	/**
	@brief 録画の停止
	@since ver.0.0.0
	@sa MsgStopRecord()
	@sa CTVTestApp::StopRecord()
	*/
	MESSAGE_STOPRECORD,

	/**
	@brief 録画の一時停止/再開
	@since ver.0.0.0
	@sa MsgPauseRecord()
	@sa CTVTestApp::PauseRecord()
	*/
	MESSAGE_PAUSERECORD,

	/**
	@brief 録画設定を取得
	@since ver.0.0.0
	@sa MsgGetRecord()
	@sa CTVTestApp::GetRecord()
	*/
	MESSAGE_GETRECORD,

	/**
	@brief 録画設定を変更
	@since ver.0.0.0
	@sa MsgModifyRecord()
	@sa CTVTestApp::ModifyRecord()
	*/
	MESSAGE_MODIFYRECORD,

	/**
	@brief 表示倍率を取得
	@since ver.0.0.0
	@sa MsgGetZoom()
	@sa CTVTestApp::GetZoom()
	*/
	MESSAGE_GETZOOM,

	/**
	@brief 表示倍率を設定
	@since ver.0.0.0
	@sa MsgSetZoom()
	@sa CTVTestApp::SetZoom()
	*/
	MESSAGE_SETZOOM,

	/**
	@brief パンスキャンの設定を取得
	@since ver.0.0.0
	@sa MsgGetPanScan()
	@sa CTVTestApp::GetPanScan()
	*/
	MESSAGE_GETPANSCAN,

	/**
	@brief パンスキャンを設定
	@since ver.0.0.0
	@sa MsgSetPanScan()
	@sa CTVTestApp::SetPanScan()
	*/
	MESSAGE_SETPANSCAN,

	/**
	@brief ステータスを取得
	@since ver.0.0.0
	@sa MsgGetStatus()
	@sa CTVTestApp::GetStatus()
	*/
	MESSAGE_GETSTATUS,

	/**
	@brief 録画ステータスを取得
	@since ver.0.0.0
	@sa MsgGetRecordStatus()
	@sa CTVTestApp::GetRecordStatus()
	*/
	MESSAGE_GETRECORDSTATUS,

	/**
	@brief 映像の情報を取得
	@since ver.0.0.0
	@sa MsgGetVideoInfo()
	@sa CTVTestApp::GetVideoInfo()
	*/
	MESSAGE_GETVIDEOINFO,

	/**
	@brief 音量を取得
	@since ver.0.0.0
	@sa MsgGetVolume()
	@sa CTVTestApp::GetVolume()
	*/
	MESSAGE_GETVOLUME,

	/**
	@brief 音量を設定
	@since ver.0.0.0
	@sa MsgSetVolume()
	@sa CTVTestApp::SetVolume()
	*/
	MESSAGE_SETVOLUME,

	/**
	@brief ステレオモードを取得
	@since ver.0.0.0
	@sa MsgGetStereoMode()
	@sa CTVTestApp::GetStereoMode()
	*/
	MESSAGE_GETSTEREOMODE,

	/**
	@brief ステレオモードを設定
	@since ver.0.0.0
	@sa MsgSetStereoMode()
	@sa CTVTestApp::SetStereoMode()
	*/
	MESSAGE_SETSTEREOMODE,

	/**
	@brief 全画面表示の状態を取得
	@since ver.0.0.0
	@sa MsgGetFullscreen()
	@sa CTVTestApp::GetFullscreen()
	*/
	MESSAGE_GETFULLSCREEN,

	/**
	@brief 全画面表示の状態を設定
	@since ver.0.0.0
	@sa MsgSetFullscreen()
	@sa CTVTestApp::SetFullscreen()
	*/
	MESSAGE_SETFULLSCREEN,

	/**
	@brief 再生が有効か取得
	@since ver.0.0.0
	@sa MsgGetPreview()
	@sa CTVTestApp::GetPreview()
	*/
	MESSAGE_GETPREVIEW,

	/**
	@brief 再生の有効状態を設定
	@since ver.0.0.0
	@sa MsgSetPreview()
	@sa CTVTestApp::SetPreview()
	*/
	MESSAGE_SETPREVIEW,

	/**
	@brief 待機状態であるか取得
	@since ver.0.0.0
	@sa MsgGetStandby()
	@sa CTVTestApp::GetStandby()
	*/
	MESSAGE_GETSTANDBY,

	/**
	@brief 待機状態を設定
	@since ver.0.0.0
	@sa MsgSetStandby()
	@sa CTVTestApp::SetStandby()
	*/
	MESSAGE_SETSTANDBY,

	/**
	@brief 常に最前面表示であるか取得
	@since ver.0.0.0
	@sa MsgGetAlwaysOnTop()
	@sa CTVTestApp::GetAlwaysOnTop()
	*/
	MESSAGE_GETALWAYSONTOP,

	/**
	@brief 常に最前面表示を設定
	@since ver.0.0.0
	@sa MsgSetAlwaysOnTop()
	@sa CTVTestApp::SetAlwaysOnTop()
	*/
	MESSAGE_SETALWAYSONTOP,

	/**
	@brief 画像をキャプチャする
	@since ver.0.0.0
	@sa MsgCaptureImage()
	@sa CTVTestApp::CaptureImage()
	*/
	MESSAGE_CAPTUREIMAGE,

	/**
	@brief 画像を保存する
	@since ver.0.0.0
	@sa MsgSaveImage()
	@sa CTVTestApp::SaveImage()
	*/
	MESSAGE_SAVEIMAGE,

	/**
	@brief リセットを行う
	@since ver.0.0.0
	@sa MsgReset()
	@sa CTVTestApp::Reset()
	*/
	MESSAGE_RESET,

	/**
	@brief ウィンドウを閉じる
	@since ver.0.0.0
	@sa MsgClose()
	@sa CTVTestApp::Close()
	*/
	MESSAGE_CLOSE,

	/**
	@brief ストリームコールバックを設定
	@since ver.0.0.0
	@sa MsgSetStreamCallback()
	@sa CTVTestApp::SetStreamCallback()
	*/
	MESSAGE_SETSTREAMCALLBACK,

	/**
	@brief プラグインの有効状態を設定
	@since ver.0.0.0
	@sa MsgEnablePlugin()
	@sa CTVTestApp::EnablePlugin()
	*/
	MESSAGE_ENABLEPLUGIN,

	/**
	@brief 色の設定を取得
	@since ver.0.0.0
	@sa MsgGetColor()
	@sa CTVTestApp::GetColor()
	*/
	MESSAGE_GETCOLOR,

	/**
	@brief ARIB 文字列をデコード
	@since ver.0.0.0
	@sa MsgDecodeARIBString()
	@sa CTVTestApp::DecodeARIBString()
	*/
	MESSAGE_DECODEARIBSTRING,

	/**
	@brief 現在の番組の情報を取得
	@since ver.0.0.0
	@sa MsgGetCurrentProgramInfo()
	@sa CTVTestApp::GetCurrentProgramInfo()
	*/
	MESSAGE_GETCURRENTPROGRAMINFO,

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 1)
	/**
	@brief イベントに対応しているか取得
	@since ver.0.0.1
	@sa MsgQueryEvent()
	@sa CTVTestApp::QueryEvent()
	*/
	MESSAGE_QUERYEVENT,

	/**
	@brief 現在のチューニング空間を取得
	@since ver.0.0.1
	@sa MsgGetTuningSpace()
	@sa CTVTestApp::GetTuningSpace()
	*/
	MESSAGE_GETTUNINGSPACE,

	/**
	@brief チューニング空間の情報を取得
	@since ver.0.0.1
	@sa MsgGetTuningSpaceInfo()
	@sa CTVTestApp::GetTuningSpaceInfo()
	*/
	MESSAGE_GETTUNINGSPACEINFO,

	/**
	@brief チャンネルを次に送る
	@since ver.0.0.1
	@sa MsgSetNextChannel()
	@sa CTVTestApp::SetNextChannel()
	*/
	MESSAGE_SETNEXTCHANNEL,
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 2)
	/**
	@brief 音声ストリームを取得
	@since ver.0.0.2
	@sa MsgGetAudioStream()
	@sa CTVTestApp::GetAudioStream()
	*/
	MESSAGE_GETAUDIOSTREAM,

	/**
	@brief 音声ストリームを設定
	@since ver.0.0.2
	@sa MsgSetAudioStream()
	@sa CTVTestApp::SetAudioStream()
	*/
	MESSAGE_SETAUDIOSTREAM,
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 3)
	/**
	@brief プラグインの有効状態を取得
	@since ver.0.0.3
	@sa MsgIsPluginEnabled()
	@sa CTVTestApp::IsPluginEnabled()
	*/
	MESSAGE_ISPLUGINENABLED,

	/**
	@brief コマンドを登録
	@since ver.0.0.3
	@sa MsgRegisterCommand()
	@sa CTVTestApp::RegisterCommand()
	*/
	MESSAGE_REGISTERCOMMAND,

	/**
	@brief ログを記録
	@since ver.0.0.3
	@sa MsgAddLog()
	@sa CTVTestApp::AddLog()
	*/
	MESSAGE_ADDLOG,
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 5)
	/**
	@brief ステータスを初期化
	@since ver.0.0.5
	@sa MsgResetStatus()
	@sa CTVTestApp::ResetStatus()
	*/
	MESSAGE_RESETSTATUS,
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 6)
	/**
	@brief 音声のコールバック関数を設定
	@since ver.0.0.6
	@sa MsgSetAudioCallback()
	@sa CTVTestApp::SetAudioCallback()
	*/
	MESSAGE_SETAUDIOCALLBACK,
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 7)
	/**
	@brief コマンドの実行
	@since ver.0.0.7
	@sa MsgDoCommand()
	@sa CTVTestApp::DoCommand()
	*/
	MESSAGE_DOCOMMAND,
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 8)
	MESSAGE_REMOVED1, /**< (機能削除) */
	MESSAGE_REMOVED2, /**< (機能削除) */

	/**
	@brief ホストプログラムの情報を取得
	@since ver.0.0.8
	@sa MsgGetHostInfo()
	@sa CTVTestApp::GetHostInfo()
	*/
	MESSAGE_GETHOSTINFO,
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 9)
	/**
	@brief 設定を取得
	@since ver.0.0.9
	@sa MsgGetSetting()
	@sa CTVTestApp::GetSetting()
	*/
	MESSAGE_GETSETTING,

	/**
	@brief BonDriver のフルパスを取得
	@since ver.0.0.9
	@sa MsgGetDriverFullPathName()
	@sa CTVTestApp::GetDriverFullPathName()
	*/
	MESSAGE_GETDRIVERFULLPATHNAME,
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 10)
	/**
	@brief ロゴを取得
	@since ver.0.0.10
	@sa MsgGetLogo()
	@sa CTVTestApp::GetLogo()
	*/
	MESSAGE_GETLOGO,

	/**
	@brief 利用可能なロゴを取得
	@since ver.0.0.10
	@sa MsgGetAvailableLogoType()
	@sa CTVTestApp::GetAvailableLogoType()
	*/
	MESSAGE_GETAVAILABLELOGOTYPE,

	/**
	@brief 録画ファイルの切り替え
	@since ver.0.0.10
	@sa MsgRelayRecord()
	@sa CTVTestApp::RelayRecord()
	*/
	MESSAGE_RELAYRECORD,

	/**
	@brief サイレントモードを取得/設定
	@since ver.0.0.10
	@sa MsgGetSilentMode()
	@sa MsgSetSilentMode()
	@sa CTVTestApp::GetSilentMode()
	@sa CTVTestApp::SetSilentMode()
	*/
	MESSAGE_SILENTMODE,
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 11)
	/**
	@brief ウィンドウメッセージコールバックを設定
	@since ver.0.0.11
	@sa MsgSetWindowMessageCallback()
	@sa CTVTestApp::SetWindowMessageCallback()
	*/
	MESSAGE_SETWINDOWMESSAGECALLBACK,

	/**
	@brief コントローラを登録
	@since ver.0.0.11
	@sa MsgRegisterController()
	@sa CTVTestApp::RegisterController()
	*/
	MESSAGE_REGISTERCONTROLLER,

	/**
	@brief コントローラのボタンが押されたのを通知
	@since ver.0.0.11
	@sa MsgOnControllerButtonDown()
	@sa CTVTestApp::OnControllerButtonDown()
	*/
	MESSAGE_ONCONTROLLERBUTTONDOWN,

	/**
	@brief コントローラの設定を取得
	@since ver.0.0.11
	@sa MsgGetControllerSettings()
	@sa CTVTestApp::GetControllerSettings()
	*/
	MESSAGE_GETCONTROLLERSETTINGS,
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 12)
	/**
	@brief 番組情報を取得
	@since ver.0.0.12
	@sa MsgGetEpgEventInfo()
	@sa CTVTestApp::GetEpgEventInfo()
	*/
	MESSAGE_GETEPGEVENTINFO,

	/**
	@brief 番組情報を解放
	@since ver.0.0.12
	@sa MsgFreeEpgEventInfo()
	@sa CTVTestApp::FreeEpgEventInfo()
	*/
	MESSAGE_FREEEPGEVENTINFO,

	/**
	@brief 番組のリストを取得
	@since ver.0.0.12
	@sa MsgGetEpgEventList()
	@sa CTVTestApp::GetEpgEventList()
	*/
	MESSAGE_GETEPGEVENTLIST,

	/**
	@brief 番組のリストを解放
	@since ver.0.0.12
	@sa MsgFreeEpgEventList()
	@sa CTVTestApp::FreeEpgEventList()
	*/
	MESSAGE_FREEEPGEVENTLIST,

	/**
	@brief BonDriver の列挙
	@since ver.0.0.12
	@sa MsgEnumDriver()
	@sa CTVTestApp::EnumDriver()
	*/
	MESSAGE_ENUMDRIVER,

	/**
	@brief BonDriver のチューニング空間のリストを取得
	@since ver.0.0.12
	@sa MsgGetDriverTuningSpaceList()
	@sa CTVTestApp::GetDriverTuningSpaceList()
	*/
	MESSAGE_GETDRIVERTUNINGSPACELIST,

	/**
	@brief BonDriver のチューニング空間のリストを解放
	@since ver.0.0.12
	@sa MsgFreeDriverTuningSpaceList()
	@sa CTVTestApp::FreeDriverTuningSpaceList()
	*/
	MESSAGE_FREEDRIVERTUNINGSPACELIST,
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 13)
	/**
	@brief 番組表のイベントの有効/無効を設定
	@since ver.0.0.13
	@sa MsgEnableProgramGuideEvent()
	@sa CTVTestApp::EnableProgramGuideEvent()
	*/
	MESSAGE_ENABLEPROGRAMGUIDEEVENT,

	/**
	@brief 番組表のコマンドを登録
	@since ver.0.0.13
	@sa MsgRegisterProgramGuideCommand()
	@sa CTVTestApp::RegisterProgramGuideCommand()
	*/
	MESSAGE_REGISTERPROGRAMGUIDECOMMAND,
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 14)
	/**
	@brief スタイル値を取得
	@since ver.0.0.14
	@sa MsgGetStyleValue()
	@sa CTVTestApp::GetStyleValue()
	*/
	MESSAGE_GETSTYLEVALUE,

	/**
	@brief テーマの背景を描画
	@since ver.0.0.14
	@sa MsgThemeDrawBackground()
	@sa CTVTestApp::ThemeDrawBackground()
	*/
	MESSAGE_THEMEDRAWBACKGROUND,

	/**
	@brief テーマの文字列を描画
	@since ver.0.0.14
	@sa MsgThemeDrawText()
	@sa CTVTestApp::ThemeDrawText()
	*/
	MESSAGE_THEMEDRAWTEXT,

	/**
	@brief テーマのアイコンを描画
	@since ver.0.0.14
	@sa MsgThemeDrawIcon()
	@sa CTVTestApp::ThemeDrawIcon()
	*/
	MESSAGE_THEMEDRAWICON,

	/**
	@brief EPG 取得状況を取得
	@since ver.0.0.14
	@sa MsgGetEpgCaptureStatus()
	@sa CTVTestApp::GetEpgCaptureStatus()
	*/
	MESSAGE_GETEPGCAPTURESTATUS,

	/**
	@brief コマンドの情報を取得
	@since ver.0.0.14
	@sa MsgGetAppCommandInfo()
	@sa CTVTestApp::GetAppCommandInfo()
	*/
	MESSAGE_GETAPPCOMMANDINFO,

	/**
	@brief コマンドの数を取得
	@since ver.0.0.14
	@sa MsgGetAppCommandCount()
	@sa CTVTestApp::GetAppCommandCount()
	*/
	MESSAGE_GETAPPCOMMANDCOUNT,

	/**
	@brief 映像ストリームの数を取得
	@since ver.0.0.14
	@sa MsgGetVideoStreamCount()
	@sa CTVTestApp::GetVideoStreamCount()
	*/
	MESSAGE_GETVIDEOSTREAMCOUNT,

	/**
	@brief 映像ストリームを取得
	@since ver.0.0.14
	@sa MsgGetVideoStream()
	@sa CTVTestApp::GetVideoStream()
	*/
	MESSAGE_GETVIDEOSTREAM,

	/**
	@brief 映像ストリームを設定
	@since ver.0.0.14
	@sa MsgSetVideoStream()
	@sa CTVTestApp::SetVideoStream()
	*/
	MESSAGE_SETVIDEOSTREAM,

	/**
	@brief ログを取得
	@since ver.0.0.14
	@sa MsgGetLog()
	@sa CTVTestApp::GetLog()
	*/
	MESSAGE_GETLOG,

	/**
	@brief ログの数を取得
	@since ver.0.0.14
	@sa MsgGetLogCount()
	@sa CTVTestApp::GetLogCount()
	*/
	MESSAGE_GETLOGCOUNT,

	/**
	@brief プラグインのコマンドを登録
	@since ver.0.0.14
	@sa MsgRegisterPluginCommand()
	@sa CTVTestApp::RegisterPluginCommand()
	*/
	MESSAGE_REGISTERPLUGINCOMMAND,

	/**
	@brief プラグインのコマンドの状態を設定
	@since ver.0.0.14
	@sa MsgSetPluginCommandState()
	@sa CTVTestApp::SetPluginCommandState()
	*/
	MESSAGE_SETPLUGINCOMMANDSTATE,

	/**
	@brief プラグインのコマンドの通知
	@since ver.0.0.14
	@sa MsgPluginCommandNotify()
	@sa CTVTestApp::PluginCommandNotify()
	*/
	MESSAGE_PLUGINCOMMANDNOTIFY,

	/**
	@brief プラグインのアイコンを登録
	@since ver.0.0.14
	@sa MsgRegisterPluginIcon()
	@sa CTVTestApp::RegisterPluginIcon()
	*/
	MESSAGE_REGISTERPLUGINICON,

	/**
	@brief ステータス項目を登録
	@since ver.0.0.14
	@sa MsgRegisterStatusItem()
	@sa CTVTestApp::RegisterStatusItem()
	*/
	MESSAGE_REGISTERSTATUSITEM,

	/**
	@brief ステータス項目を設定
	@since ver.0.0.14
	@sa MsgSetStatusItem()
	@sa CTVTestApp::SetStatusItem()
	*/
	MESSAGE_SETSTATUSITEM,

	/**
	@brief ステータス項目の情報を取得
	@since ver.0.0.14
	@sa MsgGetStatusItemInfo()
	@sa CTVTestApp::GetStatusItemInfo()
	*/
	MESSAGE_GETSTATUSITEMINFO,

	/**
	@brief ステータス項目の通知
	@since ver.0.0.14
	@sa MsgStatusItemNotify()
	@sa CTVTestApp::StatusItemNotify()
	*/
	MESSAGE_STATUSITEMNOTIFY,

	/**
	@brief TS プロセッサを登録
	@since ver.0.0.14
	@sa MsgRegisterTSProcessor()
	@sa CTVTestApp::RegisterTSProcessor()
	*/
	MESSAGE_REGISTERTSPROCESSOR,

	/**
	@brief パネル項目を登録
	@since ver.0.0.14
	@sa MsgRegisterPanelItem()
	@sa CTVTestApp::RegisterPanelItem()
	*/
	MESSAGE_REGISTERPANELITEM,

	/**
	@brief パネル項目を設定
	@since ver.0.0.14
	@sa MsgSetPanelItem()
	@sa CTVTestApp::SetPanelItem()
	*/
	MESSAGE_SETPANELITEM,

	/**
	@brief パネル項目の情報を取得
	@since ver.0.0.14
	@sa MsgGetPanelItemInfo()
	@sa CTVTestApp::GetPanelItemInfo()
	*/
	MESSAGE_GETPANELITEMINFO,

	/**
	@brief チャンネルを選択
	@since ver.0.0.14
	@sa MsgSelectChannel()
	@sa CTVTestApp::SelectChannel()
	*/
	MESSAGE_SELECTCHANNEL,

	/**
	@brief お気に入りチャンネルを取得
	@since ver.0.0.14
	@sa MsgGetFavoriteList()
	@sa CTVTestApp::GetFavoriteList()
	*/
	MESSAGE_GETFAVORITELIST,

	/**
	@brief お気に入りチャンネルを解放
	@since ver.0.0.14
	@sa MsgFreeFavoriteList()
	@sa CTVTestApp::FreeFavoriteList()
	*/
	MESSAGE_FREEFAVORITELIST,

	/**
	@brief ワンセグモードを取得
	@since ver.0.0.14
	@sa MsgGet1SegMode()
	@sa CTVTestApp::Get1SegMode()
	*/
	MESSAGE_GET1SEGMODE,

	/**
	@brief ワンセグモードを設定
	@since ver.0.0.14
	@sa MsgSet1SegMode()
	@sa CTVTestApp::Set1SegMode()
	*/
	MESSAGE_SET1SEGMODE,

	/**
	@brief DPI を取得
	@since ver.0.0.14
	@sa MsgGetDPI()
	@sa CTVTestApp::GetDPI()
	*/
	MESSAGE_GETDPI,

	/**
	@brief フォントを取得
	@since ver.0.0.14
	@sa MsgGetFont()
	@sa CTVTestApp::GetFont()
	*/
	MESSAGE_GETFONT,

	/**
	@brief ダイアログを表示
	@since ver.0.0.14
	@sa MsgShowDialog()
	@sa CTVTestApp::ShowDialog()
	*/
	MESSAGE_SHOWDIALOG,

	/**
	@brief 日時を変換
	@since ver.0.0.14
	@sa MsgConvertTime()
	@sa CTVTestApp::ConvertTime()
	*/
	MESSAGE_CONVERTTIME,

	/**
	@brief 映像ストリームのコールバック関数を設定
	@since ver.0.0.14
	@sa MsgSetVideoStreamCallback()
	@sa CTVTestApp::SetVideoStreamCallback()
	*/
	MESSAGE_SETVIDEOSTREAMCALLBACK,

	/**
	@brief 変数文字列のコンテキストを取得
	@since ver.0.0.14
	@sa MsgGetVarStringContext()
	@sa CTVTestApp::GetVarStringContext()
	*/
	MESSAGE_GETVARSTRINGCONTEXT,

	/**
	@brief 変数文字列のコンテキストを解放
	@since ver.0.0.14
	@sa MsgFreeVarStringContext()
	@sa CTVTestApp::FreeVarStringContext()
	*/
	MESSAGE_FREEVARSTRINGCONTEXT,

	/**
	@brief 変数文字列を使って文字列をフォーマット
	@since ver.0.0.14
	@sa MsgFormatVarString()
	@sa CTVTestApp::FormatVarString()
	*/
	MESSAGE_FORMATVARSTRING,

	/**
	@brief 変数を登録
	@since ver.0.0.14
	@sa MsgRegisterVariable()
	@sa CTVTestApp::RegisterVariable()
	*/
	MESSAGE_REGISTERVARIABLE,
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 15)
	/**
	@brief ダークモードの状態を取得
	@since ver.0.0.15
	@sa MsgGetDarkModeStatus()
	@sa CTVTestApp::GetDarkModeStatus()
	*/
	MESSAGE_GETDARKMODESTATUS,

	/**
	@brief ダークモードの色かを取得
	@since ver.0.0.15
	@sa MsgIsDarkModeColor()
	@sa CTVTestApp::IsDarkModeColor()
	*/
	MESSAGE_ISDARKMODECOLOR,

	/**
	@brief ウィンドウをダークモードにする
	@since ver.0.0.15
	@sa MsgSetWindowDarkMode()
	@sa CTVTestApp::SetWindowDarkMode()
	*/
	MESSAGE_SETWINDOWDARKMODE,

	/**
	@brief Elementary Stream (ES) の情報のリストを取得
	@since ver.0.0.15
	@sa MsgGetElementaryStreamInfoList()
	@sa CTVTestApp::GetElementaryStreamInfoList()
	*/
	MESSAGE_GETELEMENTARYSTREAMINFOLIST,

	/**
	@brief 全てのサービス数を取得
	@since ver.0.0.15
	@sa MsgGetServiceCount()
	@sa CTVTestApp::GetServiceCount()
	*/
	MESSAGE_GETSERVICECOUNT,

	/**
	@brief サービスの情報を取得
	@since ver.0.0.15
	@sa MsgGetServiceInfo2()
	@sa CTVTestApp::GetServiceInfo2()
	*/
	MESSAGE_GETSERVICEINFO2,

	/**
	@brief サービスの情報のリストを取得
	@since ver.0.0.15
	@sa MsgGetServiceInfoList()
	@sa CTVTestApp::GetServiceInfoList()
	*/
	MESSAGE_GETSERVICEINFOLIST,

	/**
	@brief 音声の情報を取得
	@since ver.0.0.15
	@sa MsgGetAudioInfo()
	@sa CTVTestApp::GetAudioInfo()
	*/
	MESSAGE_GETAUDIOINFO,

	/**
	@brief Elementary Stream (ES) の数を取得
	@since ver.0.0.15
	@sa MsgGetElementaryStreamCount()
	@sa CTVTestApp::GetElementaryStreamCount()
	*/
	MESSAGE_GETELEMENTARYSTREAMCOUNT,

	/**
	@brief 音声を選択
	@since ver.0.0.15
	@sa MsgSelectAudio()
	@sa CTVTestApp::SelectAudio()
	*/
	MESSAGE_SELECTAUDIO,

	/**
	@brief 選択された音声を取得
	@since ver.0.0.15
	@sa MsgGetSelectedAudio()
	@sa CTVTestApp::GetSelectedAudio()
	*/
	MESSAGE_GETSELECTEDAUDIO,

	/**
	@brief 現在の番組情報を取得
	@since ver.0.0.15
	@sa MsgGetCurrentEpgEventInfo()
	@sa CTVTestApp::GetCurrentEpgEventInfo()
	*/
	MESSAGE_GETCURRENTEPGEVENTINFO,
#endif

	MESSAGE_TRAILER
};

/**
@brief メッセージ送信用コールバック関数
*/
typedef LRESULT (CALLBACK *MessageCallbackFunc)(struct PluginParam *pParam, MessageCode Message, LPARAM lParam1, LPARAM lParam2);

/**
@brief プラグインパラメータ

TVTInitialize() 関数に PluginParam 構造体へのポインタが渡されるので、プラグイン側では渡されたポインタ値を保持して利用します。
*/
struct PluginParam
{
	MessageCallbackFunc Callback; /**< メッセージコールバック関数 */
	HWND hwndApp;                 /**< メインウィンドウのハンドル */
	void *pClientData;            /**< プラグイン側で好きに使えるデータ */
	void *pInternalData;          /**< TVTest 側で使用するデータ。アクセス禁止 */
};

/**
@brief イベントコード

イベントコールバック関数(EventCallbackFunc)によって通知されるイベントのコードです。
lParam1 と lParam2 の2つの引数で、各イベントに固有のパラメータが渡されます。

番組表関係のイベント(EVENT_PROGRAMGUIDE_*)は、 TVTest::MESSAGE_ENABLEPROGRAMGUIDEEVENT を呼んで有効にしないと通知されません。
*/
TVTEST_DEFINE_ENUM(EventCode, UINT) {
	/**
	@brief 有効状態が変化する
	@param[in] lParam1 プラグインが有効化される場合は 1、無効化される場合は 0
	@return 変化を受け入れる場合は TRUE、変化を拒否する場合は FALSE
	@since ver.0.0.0
	@sa CTVTestEventHandler::OnPluginEnable()
	*/
	EVENT_PLUGINENABLE,

	/**
	@brief 設定を行う
	@param[in] lParam1 HWND 設定ダイアログのオーナーとするウィンドウハンドル
	@return 設定が OK された場合は TRUE、設定がキャンセルされたかエラーが発生した場合は FALSE
	@since ver.0.0.0
	@sa CTVTestEventHandler::OnPluginSettings()
	*/
	EVENT_PLUGINSETTINGS,

	/**
	@brief チャンネルが変更された
	@since ver.0.0.0
	@sa CTVTestEventHandler::OnChannelChange()
	*/
	EVENT_CHANNELCHANGE,

	/**
	@brief サービスが変更された
	@since ver.0.0.0
	@sa CTVTestEventHandler::OnServiceChange()
	*/
	EVENT_SERVICECHANGE,

	/**
	@brief ドライバが変更された
	@since ver.0.0.0
	@sa CTVTestEventHandler::OnDriverChange()
	*/
	EVENT_DRIVERCHANGE,

	/**
	@brief サービスの構成が変化した
	@since ver.0.0.0
	@sa CTVTestEventHandler::OnServiceUpdate()
	*/
	EVENT_SERVICEUPDATE,

	/**
	@brief 録画状態が変化した
	@param[in] lParam1 新しい状態(RECORD_STATUS_*)
	@since ver.0.0.0
	@sa CTVTestEventHandler::OnRecordStatusChange()
	*/
	EVENT_RECORDSTATUSCHANGE,

	/**
	@brief 全画面表示状態が変化した
	@param[in] lParam1 全画面表示になった場合は 1、全画面表示が終了した場合は 0
	@since ver.0.0.0
	@sa CTVTestEventHandler::OnFullscreenChange()
	*/
	EVENT_FULLSCREENCHANGE,

	/**
	@brief 再生状態が変化した
	@param[in] lParam1 再生状態になった場合は 1、再生オフになった場合は 0
	@since ver.0.0.0
	@sa CTVTestEventHandler::OnPreviewChange()
	*/
	EVENT_PREVIEWCHANGE,

	/**
	@brief 音量が変化した
	@param[in] lParam1 音量(0-100)
	@param[in] lParam2 消音状態の場合は 1、消音状態でない場合は 0
	@since ver.0.0.0
	@sa CTVTestEventHandler::OnVolumeChange()
	*/
	EVENT_VOLUMECHANGE,

	/**
	@brief ステレオモードが変化した
	@param[in] lParam1 新しいステレオモード(STEREOMODE_*)
	@since ver.0.0.0
	@sa CTVTestEventHandler::OnStereoModeChange()
	*/
	EVENT_STEREOMODECHANGE,

	/**
	@brief 色の設定が変化した
	@since ver.0.0.0
	@sa CTVTestEventHandler::OnColorChange()
	*/
	EVENT_COLORCHANGE,

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 3)
	/**
	@brief 待機状態が変化した
	@param[in] lParam1 待機状態になった場合は 1、待機状態が解除された場合は 0
	@since ver.0.0.3
	@sa CTVTestEventHandler::OnStandby()
	*/
	EVENT_STANDBY,

	/**
	@brief コマンドが選択された
	@param[in] lParam1 選択されたコマンドの識別子
	@return コマンドを処理した場合は 1、コマンドを処理しなかった場合は 0
	@since ver.0.0.3
	@sa CTVTestEventHandler::OnCommand()
	*/
	EVENT_COMMAND,
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 4)
	/**
	@brief 複数起動禁止時に複数起動された
	@param[in] lParam1 LPCWSTR 新しく起動されたプロセスのコマンドライン引数
	@since ver.0.0.4
	@sa CTVTestEventHandler::OnExecute()
	*/
	EVENT_EXECUTE,
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 5)
	/**
	@brief リセットされた
	@since ver.0.0.5
	@sa CTVTestEventHandler::OnReset()
	*/
	EVENT_RESET,

	/**
	@brief ステータスがリセットされた
	@since ver.0.0.5
	@sa CTVTestEventHandler::OnStatusReset()
	*/
	EVENT_STATUSRESET,

	/**
	@brief 音声ストリームが変更された
	@param[in] lParam1 音声ストリームのインデックス
	@since ver.0.0.5
	@sa CTVTestEventHandler::OnAudioStreamChange()
	*/
	EVENT_AUDIOSTREAMCHANGE,
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 9)
	/**
	@brief TVTest の設定が変更された
	@since ver.0.0.9
	@sa CTVTestEventHandler::OnSettingsChange()
	*/
	EVENT_SETTINGSCHANGE,
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 10)
	/**
	@brief TVTest のウィンドウが閉じられる
	@since ver.0.0.10
	@sa CTVTestEventHandler::OnClose()
	*/
	EVENT_CLOSE,

	/**
	@brief 録画が開始される
	@param[in,out] lParam1 StartRecordInfo * 録画開始の情報
	@since ver.0.0.10
	@sa CTVTestEventHandler::OnStartRecord()
	*/
	EVENT_STARTRECORD,

	/**
	@brief 録画ファイルが切り替えられた
	@param[in] lParam1 LPCWSTR 新しいファイルのパス
	@since ver.0.0.10
	@sa CTVTestEventHandler::OnRelayRecord()
	*/
	EVENT_RELAYRECORD,
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 11)
	/**
	@brief コントローラの対象を設定
	@param[in] lParam1 HWND コントローラの操作対象となるウィンドウのハンドル
	@since ver.0.0.11
	@sa CTVTestEventHandler::OnControllerFocus()
	*/
	EVENT_CONTROLLERFOCUS,
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 13)
	/**
	@brief 起動時の処理が終了した
	@since ver.0.0.13
	@sa CTVTestEventHandler::OnStartupDone()
	*/
	EVENT_STARTUPDONE,

	/**
	@brief 番組表の初期化
	@param[in] lParam1 HWND 番組表のウィンドウハンドル
	@since ver.0.0.13
	@sa CTVTestEventHandler::OnProgramGuideInitialize()
	*/
	EVENT_PROGRAMGUIDE_INITIALIZE,

	/**
	@brief 番組表の終了
	@param[in] lParam1 HWND 番組表のウィンドウハンドル
	@since ver.0.0.13
	@sa CTVTestEventHandler::OnProgramGuideFinalize()
	*/
	EVENT_PROGRAMGUIDE_FINALIZE,

	/**
	@brief 番組表のコマンド実行
	@param[in] lParam1 実行するコマンドの識別子
	@param[in] lParam2 const ProgramGuideCommandParam * 実行するコマンドのパラメータ
	@return コマンドを実行した場合は TRUE、実行しなかった場合は FALSE
	@since ver.0.0.13
	@sa CTVTestEventHandler::OnProgramGuideCommand()
	*/
	EVENT_PROGRAMGUIDE_COMMAND,

	/**
	@brief 番組表のメニューの初期化
	@param[in] lParam1 const ProgramGuideInitializeMenuInfo * メニューの情報
	@return メニューに追加したコマンドの数
	@since ver.0.0.13
	@sa CTVTestEventHandler::OnProgramGuideInitializeMenu()
	*/
	EVENT_PROGRAMGUIDE_INITIALIZEMENU,

	/**
	@brief 番組表のメニューが選択された
	@param[in] lParam1 選択されたコマンドのインデックス
	@return コマンドを実行した場合は TRUE、実行しなかった場合は FALSE
	@since ver.0.0.13
	@sa CTVTestEventHandler::OnProgramGuideMenuSelected()
	*/
	EVENT_PROGRAMGUIDE_MENUSELECTED,

	/**
	@brief 番組表の番組の背景を描画
	@param[in] lParam1 const ProgramGuideProgramInfo * 番組の情報
	@param[in] lParam2 const ProgramGuideProgramDrawBackgroundInfo * 背景描画の情報
	@return 背景を描画した場合は TRUE、描画しなかった場合は FALSE
	@since ver.0.0.13
	@sa CTVTestEventHandler::OnProgramGuideProgramDrawBackground()
	*/
	EVENT_PROGRAMGUIDE_PROGRAM_DRAWBACKGROUND,

	/**
	@brief 番組表の番組のメニューの設定
	@param[in] lParam1 const ProgramGuideProgramInfo * 番組の情報
	@param[in] lParam2 const ProgramGuideProgramInitializeMenuInfo * メニューの情報
	@return メニューに追加したコマンドの数
	@since ver.0.0.13
	@sa CTVTestEventHandler::OnProgramGuideProgramInitializeMenu()
	*/
	EVENT_PROGRAMGUIDE_PROGRAM_INITIALIZEMENU,

	/**
	@brief 番組表の番組のメニューが選択された
	@param[in] lParam1 const ProgramGuideProgramInfo * 番組の情報
	@param[in] lParam2 選択されたコマンドのインデックス
	@return コマンドを実行した場合は TRUE、実行しなかった場合は FALSE
	@since ver.0.0.13
	@sa CTVTestEventHandler::OnProgramGuideProgramMenuSelected()
	*/
	EVENT_PROGRAMGUIDE_PROGRAM_MENUSELECTED,
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 14)
	/**
	@brief フィルタグラフの初期化開始
	@param[in] lParam1 FilterGraphInfo * フィルタグラフの情報
	@since ver.0.0.14
	@sa CTVTestEventHandler::OnFilterGraphInitialize()
	*/
	EVENT_FILTERGRAPH_INITIALIZE,

	/**
	@brief フィルタグラフの初期化終了
	@param[in] lParam1 FilterGraphInfo * フィルタグラフの情報
	@since ver.0.0.14
	@sa CTVTestEventHandler::OnFilterGraphInitialized()
	*/
	EVENT_FILTERGRAPH_INITIALIZED,

	/**
	@brief フィルタグラフの終了処理開始
	@param[in] lParam1 FilterGraphInfo * フィルタグラフの情報
	@since ver.0.0.14
	@sa CTVTestEventHandler::OnFilterGraphFinalize()
	*/
	EVENT_FILTERGRAPH_FINALIZE,

	/**
	@brief フィルタグラフの終了処理終了
	@param[in] lParam1 FilterGraphInfo * フィルタグラフの情報
	@since ver.0.0.14
	@sa CTVTestEventHandler::OnFilterGraphFinalized()
	*/
	EVENT_FILTERGRAPH_FINALIZED,

	/**
	@brief コマンドアイコンの描画
	@param[in] lParam1 DrawCommandIconInfo * コマンドアイコン描画の情報
	@return 描画を行った場合は TRUE、行わなかった場合は FALSE
	@since ver.0.0.14
	@sa CTVTestEventHandler::OnDrawCommandIcon()
	*/
	EVENT_DRAWCOMMANDICON,

	/**
	@brief ステータス項目を描画
	@param[in] lParam1 StatusItemDrawInfo * ステータス項目描画の情報
	@return 描画を行った場合は TRUE、行わなかった場合は FALSE
	@since ver.0.0.14
	@sa CTVTestEventHandler::OnStatusItemDraw()
	*/
	EVENT_STATUSITEM_DRAW,

	/**
	@brief ステータス項目の通知
	@param[in] lParam1 StatusItemEventInfo * ステータス項目通知の情報
	@since ver.0.0.14
	@sa CTVTestEventHandler::OnStatusItemNotify()
	*/
	EVENT_STATUSITEM_NOTIFY,

	/**
	@brief ステータス項目のマウス操作
	@param[in] lParam1 StatusItemMouseEventInfo * ステータス項目のマウスイベントの情報
	@since ver.0.0.14
	@sa CTVTestEventHandler::OnStatusItemMouse()
	*/
	EVENT_STATUSITEM_MOUSE,

	/**
	@brief パネル項目の通知
	@param[in] lParam1 PanelItemEventInfo * パネル項目通知の情報
	@since ver.0.0.14
	@sa CTVTestEventHandler::OnPanelItemNotify()
	*/
	EVENT_PANELITEM_NOTIFY,

	/**
	@brief お気に入りチャンネルが変更された
	@since ver.0.0.14
	@sa CTVTestEventHandler::OnFavoritesChanged()
	*/
	EVENT_FAVORITESCHANGED,

	/**
	@brief ワンセグモードが変わった
	@param[in] lParam1 ワンセグモードが有効になった場合は 1、無効になった場合は 0
	@since ver.0.0.14
	@sa CTVTestEventHandler::On1SegModeChanged()
	*/
	EVENT_1SEGMODECHANGED,

	/**
	@brief 変数の取得
	@param[in,out] lParam1 GetVariableInfo * 変数取得の情報
	@return 変数の値が取得された場合は TRUE、取得されなかった場合は FALSE
	@since ver.0.0.14
	@sa CTVTestEventHandler::OnGetVariable()
	*/
	EVENT_GETVARIABLE,
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 15)
	/**
	@brief ダークモード状態が変わった
	@param[in] lParam1 ダークモードになった場合は 1、ダークモードが解除された場合は 0
	@since ver.0.0.15
	@sa CTVTestEventHandler::OnDarkModeChanged()
	*/
	EVENT_DARKMODECHANGED,

	/**
	@brief メインウィンドウのダークモード状態が変わった
	@param[in] lParam1 ダークモードになった場合は 1、ダークモードが解除された場合は 0
	@since ver.0.0.15
	@sa CTVTestEventHandler::OnMainWindowDarkModeChanged()
	*/
	EVENT_MAINWINDOWDARKMODECHANGED,

	/**
	@brief 番組表のダークモード状態が変わった
	@param[in] lParam1 ダークモードになった場合は 1、ダークモードが解除された場合は 0
	@since ver.0.0.15
	@sa CTVTestEventHandler::OnProgramGuideDarkModeChanged()
	*/
	EVENT_PROGRAMGUIDEDARKMODECHANGED,

	/**
	@brief 映像の形式が変わった
	@since ver.0.0.15
	@sa CTVTestEventHandler::OnVidoeFormatChange()
	*/
	EVENT_VIDEOFORMATCHANGE,

	/**
	@brief 音声の形式が変わった
	@since ver.0.0.15
	@sa CTVTestEventHandler::OnAudioFormatChange()
	*/
	EVENT_AUDIOFORMATCHANGE,

	/**
	@brief 番組が変わった
	@since ver.0.0.15
	@sa CTVTestEventHandler::OnEventChanged()
	*/
	EVENT_EVENTCHANGED,

	/**
	@brief 番組情報が変化した
	@since ver.0.0.15
	@sa CTVTestEventHandler::OnEventInfoChanged()
	*/
	EVENT_EVENTINFOCHANGED,
#endif

	EVENT_TRAILER
};

/**
@brief イベント用コールバック関数
*/
typedef LRESULT (CALLBACK *EventCallbackFunc)(EventCode Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);

/**
@brief プラグインのバージョン取得関数 TVTGetVersion() へのポインタ型
*/
typedef DWORD (WINAPI *GetVersionFunc)();

/**
@brief プラグインの情報取得関数 TVTGetPluginInfo() へのポインタ型
*/
typedef BOOL (WINAPI *GetPluginInfoFunc)(PluginInfo *pInfo);

/**
@brief プラグインの初期化処理関数 TVTInitialize() へのポインタ型
*/
typedef BOOL (WINAPI *InitializeFunc)(PluginParam *pParam);

/**
@brief プラグインの終了処理関数 TVTFinalize() へのポインタ型
*/
typedef BOOL (WINAPI *FinalizeFunc)();

/**
@brief バージョン番号を DWORD にまとめる

@param[in] Major メジャーバージョン
@param[in] Minor マイナーバージョン
@param[in] Build ビルドバージョン

@return バージョン番号を表す DWORD 値

@sa GetMajorVersion()
@sa GetMinorVersion()
@sa GetBuildVersion()
*/
inline DWORD MakeVersion(BYTE Major, WORD Minor, WORD Build)
{
	return (static_cast<DWORD>(Major) << 24) | (static_cast<DWORD>(Minor) << 12) | Build;
}

/**
@brief DWORD にまとめたバージョン番号からメジャーバージョンを取得

@param[in] Version バージョン番号

@return メジャーバージョン

@sa MakeVersion()
*/
inline DWORD GetMajorVersion(DWORD Version) { return Version >> 24; }

/**
@brief DWORD にまとめたバージョン番号からマイナーバージョンを取得

@param[in] Version バージョン番号

@return マイナーバージョン

@sa MakeVersion()
*/
inline DWORD GetMinorVersion(DWORD Version) { return (Version & 0x00FFF000UL) >> 12; }

/**
@brief DWORD にまとめたバージョン番号からビルドバージョンを取得

@param[in] Version バージョン番号

@return ビルドバージョン

@sa MakeVersion()
*/
inline DWORD GetBuildVersion(DWORD Version) { return Version & 0x00000FFFUL; }

/**
@brief プログラム(TVTest)のバージョンを取得する

@param[in] pParam プラグインパラメータ

@return TVTest のバージョン。
        上位8ビットがメジャーバージョン、次の12ビットがマイナーバージョン、下位12ビットがビルドナンバー。
        それぞれの値は GetMajorVersion() / GetMinorVersion() / GetBuildVersion() を使って取得できます。

@remark より詳しいプログラムの情報を MsgGetHostInfo() で取得できます。

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_GETVERSION

@sa MakeVersion()
@sa GetMajorVersion()
@sa GetMinorVersion()
@sa GetBuildVersion()
*/
inline DWORD MsgGetVersion(PluginParam *pParam)
{
	return static_cast<DWORD>((*pParam->Callback)(pParam, MESSAGE_GETVERSION, 0, 0));
}

/**
@brief 指定されたメッセージに対応しているか問い合わせる

TVTest が指定されたメッセージコードに対応しているかを問い合わせます。

@param[in] pParam プラグインパラメータ
@param[in] Message 問い合わせるメッセージのコード(MESSAGE_*)

@retval true メッセージに対応している
@retval false メッセージに対応していない

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_QUERYMESSAGE
*/
inline bool MsgQueryMessage(PluginParam *pParam, UINT Message)
{
	return (*pParam->Callback)(pParam, MESSAGE_QUERYMESSAGE, Message, 0) != 0;
}

/**
@brief メモリ再確保

@param[in] pParam プラグインパラメータ
@param[in] pData 再確保するデータのポインタ。nullptr で新しい領域を確保。
@param[in] Size 確保するサイズ(バイト単位)。0 で領域を解放。

@return 確保したメモリ領域へのポインタ。
        Size 引数が 0 か、メモリが確保できなかった場合は nullptr。

@note 確保したメモリは MsgMemoryFree() で解放します。
      または MsgMemoryReAlloc() の Size 引数を 0 で呼び出すことでも解放できます。

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_MEMORYALLOC

@sa MsgMemoryAlloc()
@sa MsgMemoryFree()
*/
inline void *MsgMemoryReAlloc(PluginParam *pParam, void *pData, DWORD Size)
{
	return reinterpret_cast<void*>((*pParam->Callback)(pParam, MESSAGE_MEMORYALLOC, reinterpret_cast<LPARAM>(pData), Size));
}

/**
@brief メモリ確保

@param[in] pParam プラグインパラメータ
@param[in] Size 確保するサイズ(バイト単位)

@return 確保したメモリ領域へのポインタ。
        Size 引数が 0 か、メモリが確保できなかった場合は nullptr。

@note 確保したメモリは MsgMemoryFree() で解放します。

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_MEMORYALLOC
*/
inline void *MsgMemoryAlloc(PluginParam *pParam, DWORD Size)
{
	return reinterpret_cast<void*>((*pParam->Callback)(pParam, MESSAGE_MEMORYALLOC, reinterpret_cast<LPARAM>(nullptr), Size));
}

/**
@brief メモリ解放

MsgMemoryAlloc() や MsgMemoryReAlloc() などで確保されたメモリを解放します。

@param[in] pParam プラグインパラメータ
@param[in] pData 解放するメモリ領域へのポインタ

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_MEMORYALLOC
*/
inline void MsgMemoryFree(PluginParam *pParam, void *pData)
{
	(*pParam->Callback)(pParam, MESSAGE_MEMORYALLOC, reinterpret_cast<LPARAM>(pData), 0);
}

/**
@brief 文字列を複製

@param[in] pParam プラグインパラメータ
@param[in] pszString 複製する文字列へのポインタ

@return 複製した文字列へのポインタ。
        pszString が nullptr であるか、メモリが確保できなかった場合は nullptr。

@note この関数は MsgMemoryAlloc() を使用してメモリを確保します。
      複製した文字列は MsgMemoryFree() で解放します。

@since ver.0.0.0
*/
inline LPWSTR MsgStringDuplicate(PluginParam *pParam, LPCWSTR pszString)
{
	if (pszString == nullptr)
		return nullptr;
	const DWORD Size = (::lstrlenW(pszString) + 1) * sizeof(WCHAR);
	LPWSTR pszDup = static_cast<LPWSTR>(MsgMemoryAlloc(pParam, Size));
	if (pszDup != nullptr)
		::CopyMemory(pszDup, pszString, Size);
	return pszDup;
}

/**
@brief イベントハンドル用コールバックを設定

@param[in] pParam プラグインパラメータ
@param[in] Callback コールバック関数。nullptr を渡すと設定が解除される。
@param[in] pClientData コールバック関数に渡される値

@retval true 正常に設定された
@retval false エラーが発生した

@note 一つのプラグインで設定できるコールバック関数は一つだけです。
      2回目以降の呼び出しでは、前回の設定が上書きされます。

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_SETEVENTCALLBACK
*/
inline bool MsgSetEventCallback(PluginParam *pParam, EventCallbackFunc Callback, void *pClientData = nullptr)
{
	return (*pParam->Callback)(pParam, MESSAGE_SETEVENTCALLBACK, reinterpret_cast<LPARAM>(Callback), reinterpret_cast<LPARAM>(pClientData)) != 0;
}

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 12)
/**
@briefチャンネルの情報のフラグ
*/
TVTEST_DEFINE_ENUM(ChannelFlag, DWORD) {
	CHANNEL_FLAG_NONE     = 0x00000000UL, /**< なし(0) */
	CHANNEL_FLAG_DISABLED = 0x00000001UL  /**< 無効にされている */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(ChannelFlag)
#endif

/**
@brief チャンネルの情報

主にチャンネルファイル(*.ch2,*.ch1)の情報を取得するのに利用します。

ChannelInfo::ServiceType と ChannelInfo::ServiceID はチャンネルファイルで設定されているものが取得されます。
サービスはユーザーが切り替えられるので、実際に視聴中のサービスがこれであるとは限りません。
実際に視聴中のサービスは MsgGetServiceInfo() または MsgGetServiceInfo2() で取得できます。
*/
struct ChannelInfo
{
	DWORD Size;                      /**< 構造体のサイズ */
	int Space;                       /**< チューニング空間(BonDriver のインデックス) */
	int Channel;                     /**< チャンネル(BonDriver のインデックス) */
	int RemoteControlKeyID;          /**< リモコン ID */
	WORD NetworkID;                  /**< ネットワーク ID */
	WORD TransportStreamID;          /**< トランスポートストリーム ID */
	WCHAR szNetworkName[32];         /**< ネットワーク名 */
	WCHAR szTransportStreamName[32]; /**< トランスポートストリーム名 */
	WCHAR szChannelName[64];         /**< チャンネル名 */
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 1)
	/**
	@brief 物理チャンネル番号
	@details BonDriver のチャンネル名から推測したもの。不明の場合は 0。
	@since ver.0.0.1
	*/
	int PhysicalChannel;
#if TVTEST_PLUGIN_VERSION < TVTEST_PLUGIN_VERSION_(0, 0, 15)
	/**
	@brief サービスのインデックス(現在は意味を無くしているので使わない)
	@since ver.0.0.1
	@note ver.0.0.15 で Reserved と ServiceType に置き換え。
	*/
	WORD ServiceIndex;
#else
	/**
	@brief 予約領域(現在は常に 0)
	@since ver.0.0.15
	@note ver.0.0.1 から ver.0.0.14 までは WORD ServiceIndex の下位バイトとして存在。
	*/
	BYTE Reserved;

	/**
	@brief サービス形式種別
	@since ver.0.0.15
	@note ver.0.0.1 から ver.0.0.14 までは WORD ServiceIndex の上位バイトとして存在。
	*/
	BYTE ServiceType;
#endif
	/**
	@brief サービス ID
	@since ver.0.0.1
	*/
	WORD ServiceID;
#endif
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 12)
	/**
	@brief 各種フラグ(CHANNEL_FLAG_* の組み合わせ)
	@since ver.0.0.12
	*/
	ChannelFlag Flags;
#endif
};

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 1)
/**
@brief ChannelInfo 構造体の旧バージョンのサイズ
*/
enum {
	CHANNELINFO_SIZE_V1 = offsetof(ChannelInfo, PhysicalChannel) /**< プラグイン仕様 ver.0.0.0 の ChannelInfo 構造体のサイズ */
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 12)
	, CHANNELINFO_SIZE_V2 = offsetof(ChannelInfo, Flags)         /**< プラグイン仕様 ver.0.0.1 から ver.0.0.11 までの ChannelInfo 構造体のサイズ */
#endif
};
#endif

/**
@brief 現在のチャンネルの情報を取得する

@param[in] pParam プラグインパラメータ
@param[in,out] pInfo 情報を取得する ChannelInfo 構造体へのポインタ。
               事前に ChannelInfo::Size メンバを設定して呼び出します。

@retval true 正常に取得された
@retval false エラーが発生した

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_GETCURRENTCHANNELINFO
*/
inline bool MsgGetCurrentChannelInfo(PluginParam *pParam, ChannelInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETCURRENTCHANNELINFO, reinterpret_cast<LPARAM>(pInfo), 0) != 0;
}

#if TVTEST_PLUGIN_VERSION < TVTEST_PLUGIN_VERSION_(0, 0, 8)
/**
@brief チャンネルを設定する

@param[in] pParam プラグインパラメータ
@param[in] Space チューニング空間のインデックス
@param[in] Channel チャンネルのインデックス

@retval true 正常に設定された
@retval false エラーが発生した

@remark 機能が追加された MsgSelectChannel() もあります。

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_SETCHANNEL
*/
inline bool MsgSetChannel(PluginParam *pParam, int Space, int Channel)
{
	return (*pParam->Callback)(pParam, MESSAGE_SETCHANNEL, Space, Channel) != 0;
}
#else
/**
@brief チャンネルを設定する

@param[in] pParam プラグインパラメータ
@param[in] Space チューニング空間のインデックス
@param[in] Channel チャンネルのインデックス
@param[in] ServiceID サービス ID(ver.0.0.8 以降)。
                     引数を省略するか 0 を指定した場合はデフォルトのサービス。

@retval true 正常に設定された
@retval false エラーが発生した

@remark 機能が追加された MsgSelectChannel() もあります。

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_SETCHANNEL
*/
inline bool MsgSetChannel(PluginParam *pParam, int Space, int Channel, WORD ServiceID = 0)
{
	return (*pParam->Callback)(pParam, MESSAGE_SETCHANNEL, Space, MAKELPARAM(static_cast<SHORT>(Channel), ServiceID)) != 0;
}
#endif

/**
@brief 現在のサービス及びサービス数を取得する

@param[in] pParam プラグインパラメータ
@param[out] pNumServices 試聴可能なサービスの数を返す変数へのポインタ。不要な場合は nullptr を指定する。

@return 現在のサービスのインデックス。エラー時は -1。

@note 返されるインデックスは試聴可能なサービスの中での順序です。
      全てのサービスの数を取得するには MsgGetServiceCount() を使用します。

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_GETSERVICE
*/
inline int MsgGetService(PluginParam *pParam, int *pNumServices = nullptr)
{
	return static_cast<int>((*pParam->Callback)(pParam, MESSAGE_GETSERVICE, reinterpret_cast<LPARAM>(pNumServices), 0));
}

/**
@brief サービスを設定する

@param[in] pParam プラグインパラメータ
@param[in] Service fByID = false の場合はインデックス、fByID = true の場合はサービス ID。
                   インデックスは試聴可能なサービスの中での順序。
@param[in] fByID true の場合 Serivce をサービス ID とみなし、false の場合インデックスとみなす

@retval true 正常に設定された
@retval false エラーが発生した

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_SETSERVICE
*/
inline bool MsgSetService(PluginParam *pParam, int Service, bool fByID = false)
{
	return (*pParam->Callback)(pParam, MESSAGE_SETSERVICE, Service, fByID) != 0;
}

/**
@brief チューニング空間名を取得する

@param[in] pParam プラグインパラメータ
@param[in] Index チューニング空間のインデックス
@param[out] pszName チューニング空間名を取得するバッファ。nullptr で長さの取得のみ行う。
@param[in] MaxLength pszName の先に格納できる最大の要素数(終端の空文字を含む)

@return チューニング空間名の長さ(終端の空文字は含まない)。
        Index が範囲外の場合は 0。

@note チューニング空間名の長さが MaxLength 以上である場合、pszName に格納される文字列は MaxLength - 1 の長さに切り詰められ、終端に空文字が付加されます。
      その際も戻り値は切り捨てられる前の本来の長さです。

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_GETTUNINGSPACENAME
*/
inline int MsgGetTuningSpaceName(PluginParam *pParam, int Index, LPWSTR pszName, int MaxLength)
{
	return static_cast<int>((*pParam->Callback)(pParam, MESSAGE_GETTUNINGSPACENAME, reinterpret_cast<LPARAM>(pszName), MAKELPARAM(Index, MaxLength > 0xFFFF ? 0xFFFF : MaxLength)));
}

/**
@brief チャンネルの情報を取得する

@param[in] pParam プラグインパラメータ
@param[in] Space チューニング空間のインデックス
@param[in] Index チャンネルのインデックス
@param[in,out] pInfo 情報を取得する ChannelInfo 構造体へのポインタ。
                     事前に ChannelInfo::Size メンバを設定して呼び出します。

@retval true 正常に取得された
@retval false エラーが発生した

@note ChannelInfo::szNetworkName, ChannelInfo::szTransportStreamName は MsgGetCurrentChannelInfo() でしか取得できません。
      ChannelInfo::NetworkID, ChannelInfo::TransportStreamID はチャンネルスキャンしていないと取得できません。
      取得できなかった場合は 0 になります。

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_GETCHANNELINFO
*/
inline bool MsgGetChannelInfo(PluginParam *pParam, int Space, int Index, ChannelInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETCHANNELINFO, reinterpret_cast<LPARAM>(pInfo), MAKELPARAM(Space, Index)) != 0;
}

/**
@brief サービスの情報

@sa ServiceInfo2
*/
struct ServiceInfo
{
	DWORD Size;              /**< 構造体のサイズ */
	WORD ServiceID;          /**< サービス ID */
	WORD VideoPID;           /**< ビデオストリームの PID */
	int NumAudioPIDs;        /**< 音声 PID の数(最大4まで) */
	WORD AudioPID[4];        /**< 音声ストリームの PID */
	WCHAR szServiceName[32]; /**< サービス名 */
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 2)
	/**
	@brief 音声コンポーネントタイプ
	@since ver.0.0.2
	*/
	BYTE AudioComponentType[4];

	/**
	@brief 字幕ストリームの PID(無い場合は 0)
	@since ver.0.0.2
	*/
	WORD SubtitlePID;

	/**
	@brief 予約
	@since ver.0.0.2
	*/
	WORD Reserved;
#endif
};

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 2)
/**
@brief ServiceInfo 構造体の旧バージョンのサイズ
*/
enum {
	SERVICEINFO_SIZE_V1 = offsetof(ServiceInfo, AudioComponentType) /**< プラグイン仕様 ver.0.0.1 までの ServiceInfo 構造体のサイズ */
};
#endif

/**
@brief サービスの情報を取得する

現在のチャンネルのサービスの情報を取得します。
引数 Index には試聴可能なサービスの中でのインデックスを指定します。
試聴できないサービスも含めた全てのサービスの情報を取得するには MsgGetServiceInfo2() を使用してください。
このメッセージでは取得できる映像/音声/字幕ストリームの数に制限がありますので、
全てのストリームの情報を取得するには MsgGetElementaryStreamInfoList() を使用してください。

@param[in] pParam プラグインパラメータ
@param[in] Index サービスのインデックス
@param[in,out] pInfo サービスの情報を取得する ServiceInfo 構造体へのポインタ。
                     事前に ServiceInfo::Size メンバを設定して呼び出します。

@retval true 正常に取得された
@retval false エラーが発生した

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_GETSERVICEINFO

@sa MsgGetServiceInfo2()
*/
inline bool MsgGetServiceInfo(PluginParam *pParam, int Index, ServiceInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETSERVICEINFO, Index, reinterpret_cast<LPARAM>(pInfo)) != 0;
}

/**
@brief BonDriver のファイル名を取得する

@param[in] pParam プラグインパラメータ
@param[out] pszName ファイル名を取得するバッファ。nullptr で長さの取得のみ行う。
@param[in] MaxLength pszName の先に格納できる最大の要素数(終端の空文字を含む)

@return ファイル名の長さ(終端の空文字を含まない)。
        BonDriver が読み込まれていない場合は 0。

@note ファイル名の長さが MaxLength 以上である場合、pszName に格納される文字列は MaxLength - 1 の長さに切り詰められ、終端に空文字が付加されます。
      その際も戻り値は切り捨てられる前の本来の長さです。
@note 取得されるのは、ディレクトリを含まないファイル名のみか、相対パスの場合もあります。
      フルパスを取得したい場合は MsgGetDriverFullPathName() を使用してください。

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_GETDRIVERNAME
*/
inline int MsgGetDriverName(PluginParam *pParam, LPWSTR pszName, int MaxLength)
{
	return static_cast<int>((*pParam->Callback)(pParam, MESSAGE_GETDRIVERNAME, reinterpret_cast<LPARAM>(pszName), MaxLength));
}

/**
@brief BonDriver を設定する

@param[in] pParam プラグインパラメータ
@param[in] pszName BonDriver のファイル名。
                   nullptr を指定すると、現在の BonDriver が解放されます(ver.0.0.14 以降)。

@retval true 正常に設定された
@retval false エラーが発生した

@note ファイル名のみか相対パスを指定すると、BonDriver 検索フォルダの設定が使用されます。

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_SETDRIVERNAME
*/
inline bool MsgSetDriverName(PluginParam *pParam, LPCWSTR pszName)
{
	return (*pParam->Callback)(pParam, MESSAGE_SETDRIVERNAME, reinterpret_cast<LPARAM>(pszName), 0) != 0;
}

/**
@brief 録画情報のマスク
*/
TVTEST_DEFINE_ENUM(RecordInfoMask, DWORD) {
	RECORD_MASK_NONE      = 0x00000000UL, /**< なし(0) */
	RECORD_MASK_FLAGS     = 0x00000001UL, /**< Flags が有効 */
	RECORD_MASK_FILENAME  = 0x00000002UL, /**< pszFileName が有効 */
	RECORD_MASK_STARTTIME = 0x00000004UL, /**< StartTime が有効 */
	RECORD_MASK_STOPTIME  = 0x00000008UL  /**< StopTime が有効 */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(RecordInfoMask)

/**
@brief 録画フラグ
*/
TVTEST_DEFINE_ENUM(RecordFlag, DWORD) {
	RECORD_FLAG_NONE   = 0x00000000UL, /**< なし(0) */
	RECORD_FLAG_CANCEL = 0x10000000UL  /**< キャンセル */
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 14)
	, RECORD_FLAG_UTC  = 0x00000001UL  /**< UTC 日時 */
#endif
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(RecordFlag)

/**
@brief 録画開始時間の指定方法
*/
TVTEST_DEFINE_ENUM(RecordStartTime, DWORD) {
	RECORD_START_NOTSPECIFIED, /**< 未指定 */
	RECORD_START_TIME,         /**< 時刻指定 */
	RECORD_START_DELAY         /**< 長さ指定 */
};

/**
@brief 録画停止時間の指定方法
*/
TVTEST_DEFINE_ENUM(RecordStopTime, DWORD) {
	RECORD_STOP_NOTSPECIFIED, /**< 未指定 */
	RECORD_STOP_TIME,         /**< 時刻指定 */
	RECORD_STOP_DURATION      /**< 長さ指定 */
};

/**
@brief 録画情報
*/
struct RecordInfo
{
	DWORD Size;                    /**< 構造体のサイズ */
	RecordInfoMask Mask;           /**< マスク(RECORD_MASK_*) */
	RecordFlag Flags;              /**< フラグ(RECORD_FLAG_*) */
	LPWSTR pszFileName;            /**< ファイル名(nullptr でデフォルト)。 */
	                               /**< "%～%" で囲まれた置換キーワードを使用できます */
	int MaxFileName;               /**< ファイル名の最大長(TVTest::MESSAGE_GETRECORD のみで使用) */
	FILETIME ReserveTime;          /**< 録画予約された時刻(TVTest::MESSAGE_GETRECORD のみで使用)。 */
	                               /**< ローカル時刻(RecordInfo::Flags に TVTest::RECORD_FLAG_UTC を指定した場合 UTC) */
	RecordStartTime StartTimeSpec; /**< 録画開始時間の指定方法(RECORD_START_*) */
	union {
	    FILETIME Time;             /**< 録画開始時刻(RecordInfo::StartTimeSpec == TVTest::RECORD_START_TIME)。 */
	                               /**< ローカル時刻(RecordInfo::Flags に TVTest::RECORD_FLAG_UTC を指定した場合 UTC) */
	    ULONGLONG Delay;           /**< 録画開始時間(RecordInfo::StartTimeSpec == TVTest::RECORD_START_DELAY)。 */
	                               /**< 録画を開始するまでの時間(ms) */
	} StartTime;                   /**< 開始時間 */
	RecordStopTime StopTimeSpec;   /**< 録画停止時間の指定方法(RECORD_STOP_*) */
	union {
		FILETIME Time;             /**< 録画停止時刻(RecordInfo::StopTimeSpec == TVTest::RECORD_STOP_TIME)。 */
		                           /**< ローカル時刻(RecordInfo::Flags に TVTest::RECORD_FLAG_UTC を指定した場合 UTC) */
		ULONGLONG Duration;        /**< 録画停止時間(RecordInfo::StopTimeSpec == TVTest::RECORD_STOP_DURATION)。 */
		                           /**< 開始時間からのミリ秒 */
	} StopTime;                    /**< 終了時間 */
};

/**
@brief 録画を開始する

@param[in] pParam プラグインパラメータ
@param[in] pInfo 録画の情報。nullptr で即時録画開始。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_STARTRECORD
*/
inline bool MsgStartRecord(PluginParam *pParam, const RecordInfo *pInfo = nullptr)
{
	return (*pParam->Callback)(pParam, MESSAGE_STARTRECORD, reinterpret_cast<LPARAM>(pInfo), 0) != 0;
}

/**
@brief 録画を停止する

@param[in] pParam プラグインパラメータ

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_STOPRECORD
*/
inline bool MsgStopRecord(PluginParam *pParam)
{
	return (*pParam->Callback)(pParam, MESSAGE_STOPRECORD, 0, 0) != 0;
}

/**
@brief 録画を一時停止/再開する

@param[in] pParam プラグインパラメータ
@param[in] fPause 一時停止する場合は true、再開する場合は false

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_PAUSERECORD
*/
inline bool MsgPauseRecord(PluginParam *pParam, bool fPause = true)
{
	return (*pParam->Callback)(pParam, MESSAGE_PAUSERECORD, fPause, 0) != 0;
}

/**
@brief 録画設定を取得する

@param[in] pParam プラグインパラメータ
@param[in,out] pInfo 録画設定の情報を取得する RecordInfo 構造体へのポインタ。
                     事前に RecordInfo::Size メンバを設定して呼び出します。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_GETRECORD
*/
inline bool MsgGetRecord(PluginParam *pParam, RecordInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETRECORD, reinterpret_cast<LPARAM>(pInfo), 0) != 0;
}

/**
@brief 録画設定を変更する

@param[in] pParam プラグインパラメータ
@param[in] pInfo 録画の情報

@retval true 正常終了
@retval false エラー発生

@note 既に録画中である場合は、ファイル名と開始時間の指定は無視されます。

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_MODIFYRECORD
*/
inline bool MsgModifyRecord(PluginParam *pParam, const RecordInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_MODIFYRECORD, reinterpret_cast<LPARAM>(pInfo), 0) != 0;
}

/**
@brief 表示倍率を取得する(% 単位)

@param[in] pParam プラグインパラメータ

@return 表示倍率(% 単位)。再生が行われていない場合は 0。

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_GETZOOM
*/
inline int MsgGetZoom(PluginParam *pParam)
{
	return static_cast<int>((*pParam->Callback)(pParam, MESSAGE_GETZOOM, 0, 0));
}

/**
@brief 表示倍率を設定する

@param[in] pParam プラグインパラメータ
@param[in] Num 表示倍率の分子。
@param[in] Denom 表示倍率の分母。省略すると 100。

@retval true 正常終了
@retval false エラー発生

@note % 単位だけではなく、 MsgSetZoom(pParam, 1, 3) などとして、割り切れない倍率を設定することもできます。

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_SETZOOM
*/
inline bool MsgSetZoom(PluginParam *pParam, int Num, int Denom = 100)
{
	return (*pParam->Callback)(pParam, MESSAGE_SETZOOM, Num, Denom) != 0;
}

/**
@brief パンスキャンの種類
*/
TVTEST_DEFINE_ENUM(PanScanType, int) {
	PANSCAN_NONE,      /**< なし */
	PANSCAN_LETTERBOX, /**< レターボックス */
	PANSCAN_PILLARBOX, /**< ピラーボックス */
	PANSCAN_WINDOWBOX  /**< 超額縁 */
};

/**
@brief パンスキャンの情報
*/
struct PanScanInfo
{
	DWORD Size;       /**< 構造体のサイズ */
	PanScanType Type; /**< 種類(PANSCAN_*) */
	int XAspect;      /**< 水平アスペクト比 */
	int YAspect;      /**< 垂直アスペクト比 */
};

/**
@brief パンスキャンの設定を取得する

@param[in] pParam プラグインパラメータ
@param[in,out] pInfo 情報を取得する PanScanInfo 構造体へのポインタ。
                     事前に PanScanInfo::Size メンバに構造体のサイズを設定して呼び出します。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_GETPANSCAN
*/
inline bool MsgGetPanScan(PluginParam *pParam, PanScanInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETPANSCAN, reinterpret_cast<LPARAM>(pInfo), 0) != 0;
}

/**
@brief パンスキャンを設定する

@param[in] pParam プラグインパラメータ
@param[in] pInfo 設定する情報を格納した PanScanInfo 構造体へのポインタ

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_SETPANSCAN
*/
inline bool MsgSetPanScan(PluginParam *pParam, const PanScanInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_SETPANSCAN, reinterpret_cast<LPARAM>(pInfo), 0) != 0;
}

/**
@brief ステータス情報
*/
struct StatusInfo
{
	DWORD Size;                /**< 構造体のサイズ */
	float SignalLevel;         /**< 信号レベル(dB) */
	DWORD BitRate;             /**< ビットレート(Bits/Sec) */
	DWORD ErrorPacketCount;    /**< エラーパケット数。DropPacketCount も含まれる */
	DWORD ScramblePacketCount; /**< 復号漏れパケット数 */
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 2)
	/**
	@brief ドロップパケット数
	@since ver.0.0.2
	*/
	DWORD DropPacketCount;

	/**
	@brief 予約(現在は常に 0)
	@since ver.0.0.2
	*/
	DWORD Reserved;
#endif
};

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 2)
/**
@brief StatusInfo 構造体の旧バージョンのサイズ
*/
enum {
	STATUSINFO_SIZE_V1 = offsetof(StatusInfo, DropPacketCount) /**< プラグイン仕様 ver.0.0.1 までの StatusInfo 構造体のサイズ */
};
#endif

/**
@brief ステータスを取得する

@param[in] pParam プラグインパラメータ
@param[in,out] pInfo 情報を取得する StatusInfo 構造体へのポインタ。
                     事前に StatusInfo::Size メンバを設定して呼び出します。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_GETSTATUS
*/
inline bool MsgGetStatus(PluginParam *pParam, StatusInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETSTATUS, reinterpret_cast<LPARAM>(pInfo), 0) != 0;
}

/**
@brief 録画の状態
*/
TVTEST_DEFINE_ENUM(RecordStatus, DWORD) {
	RECORD_STATUS_NOTRECORDING, /**< 録画していない */
	RECORD_STATUS_RECORDING,    /**< 録画中 */
	RECORD_STATUS_PAUSED        /**< 録画一時停止中 */
};

/**
@brief 録画ステータス情報
*/
struct RecordStatusInfo
{
	DWORD Size;                  /**< 構造体のサイズ */
	RecordStatus Status;         /**< 状態(RECORD_STATUS_*) */
	FILETIME StartTime;          /**< 録画開始時刻。 */
	                             /**< ローカル時刻(TVTest::RECORD_STATUS_FLAG_UTC が指定されていれば UTC)。 */
	DWORD RecordTime;            /**< 録画時間(ms) 一時停止中を含まない */
	DWORD PauseTime;             /**< 一時停止時間(ms) */
	RecordStopTime StopTimeSpec; /**< 録画停止時間の指定方法(RECORD_STOP_*) */
	union {
		FILETIME Time;           /**< 録画停止予定時刻(RecordStatusInfo::StopTimeSpec == TVTest::RECORD_STOP_TIME)。 */
		                         /**< ローカル時刻(TVTest::RECORD_STATUS_FLAG_UTC が指定されていれば UTC) */
		ULONGLONG Duration;      /**< 録画停止までの時間(RecordStatusInfo::StopTimeSpec == TVTest::RECORD_STOP_DURATION)。 */
		                         /**< 開始時刻(RecordStatusInfo::StartTime)からミリ秒単位 */
	} StopTime;                  /**< 開始時間 */
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 10)
	/**
	@brief ファイルパス
	@since ver.0.0.10
	*/
	LPWSTR pszFileName;

	/**
	@brief ファイルパスの最大長
	@since ver.0.0.10
	*/
	int MaxFileName;

	/**
	@brief コンストラクタ
	@details ver.0.0.10 で追加された RecordStatusInfo::pszFileName と RecordStatusInfo::MaxFileName メンバを初期化します。
	         それ以外のメンバは初期化しません。
	@since ver.0.0.10
	*/
	RecordStatusInfo() : pszFileName(nullptr), MaxFileName(0) {}
#endif
};

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 10)
/**
@brief RecordStatusInfo 構造体の旧バージョンのサイズ
*/
enum {
	RECORDSTATUSINFO_SIZE_V1 = offsetof(RecordStatusInfo, pszFileName) /**< プラグイン仕様 ver.0.0.9 までの RecordStatusInfo 構造体のサイズ */
};
#endif

/**
@brief 録画ステータスを取得する

@param[in] pParam プラグインパラメータ
@param[in,out] pInfo 情報を取得する RecordStatusInfo 構造体へのポインタ。
                     事前に RecordStatusInfo::Size メンバを設定して呼び出します。
                     ファイル名を取得する場合は  RecordStatusInfo::pszFileName と  RecordStatusInfo::MaxFileName メンバを設定しておきます。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_GETRECORDSTATUS
*/
inline bool MsgGetRecordStatus(PluginParam *pParam, RecordStatusInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETRECORDSTATUS, reinterpret_cast<LPARAM>(pInfo), 0) != 0;
}

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 14)
/**
@brief 録画ステータス取得フラグ
*/
TVTEST_DEFINE_ENUM(RecordStatusFlag, DWORD) {
	RECORD_STATUS_FLAG__NONE = 0x00000000U, /**< なし(0) */
	RECORD_STATUS_FLAG_UTC   = 0x00000001U  /**< UTC の時刻を取得 */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(RecordStatusFlag)

/**
@brief 録画ステータスを取得する

@param[in] pParam プラグインパラメータ
@param[in,out] pInfo 情報を取得する RecordStatusInfo 構造体へのポインタ。
                     事前に RecordStatusInfo::Size メンバを設定して呼び出します。
                     ファイル名を取得する場合は RecordStatusInfo::pszFileName と RecordStatusInfo::MaxFileName メンバを設定しておきます。
@param[in] Flags 取得する情報のフラグ(RECORD_STATUS_FLAG_*) (ver.0.0.14 以降)

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_GETRECORDSTATUS
*/
inline bool MsgGetRecordStatus(PluginParam *pParam, RecordStatusInfo *pInfo, RecordStatusFlag Flags)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETRECORDSTATUS, reinterpret_cast<LPARAM>(pInfo), static_cast<LPARAM>(Flags)) != 0;
}
#endif

/**
@brief 映像の情報
*/
struct VideoInfo
{
	DWORD Size;      /**< 構造体のサイズ */
	int Width;       /**< 幅(ピクセル単位) */
	int Height;      /**< 高さ(ピクセル単位) */
	int XAspect;     /**< 水平アスペクト比 */
	int YAspect;     /**< 垂直アスペクト比 */
	RECT SourceRect; /**< ソースの表示範囲 */
};

/**
@brief 映像の情報を取得する

@param[in] pParam プラグインパラメータ
@param[in,out] pInfo 情報を取得する VideoInfo 構造体へのポインタ。
                     事前に VideoInfo::Size メンバを設定して呼び出します。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_GETVIDEOINFO
*/
inline bool MsgGetVideoInfo(PluginParam *pParam, VideoInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETVIDEOINFO, reinterpret_cast<LPARAM>(pInfo), 0) != 0;
}

/**
@brief 音量を取得する

@param[in] pParam プラグインパラメータ

@return 音量(0-100)

@note 現在消音状態である場合、消音状態になる前の音量が返ります。

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_GETVOLUME

@sa MsgGetMute()
*/
inline int MsgGetVolume(PluginParam *pParam)
{
	return LOWORD((*pParam->Callback)(pParam, MESSAGE_GETVOLUME, 0, 0));
}

/**
@brief 音量を設定する

@param[in] pParam プラグインパラメータ
@param[in] Volume 音量(0-100)

@retval true 正常終了
@retval false エラー発生

@note 現在消音状態である場合、消音状態が解除されます。

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_SETVOLUME
*/
inline bool MsgSetVolume(PluginParam *pParam, int Volume)
{
	return (*pParam->Callback)(pParam, MESSAGE_SETVOLUME, Volume, 0) != 0;
}

/**
@brief 消音状態であるか取得する

@param[in] pParam プラグインパラメータ

@retval true 消音状態
@retval false 消音状態ではない

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_GETVOLUME
*/
inline bool MsgGetMute(PluginParam *pParam)
{
	return HIWORD((*pParam->Callback)(pParam, MESSAGE_GETVOLUME, 0, 0)) != 0;
}

/**
@brief 消音状態を設定する

@param[in] pParam プラグインパラメータ
@param[in] fMute 消音状態にする場合は true、消音状態を解除する場合は false

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_SETVOLUME
*/
inline bool MsgSetMute(PluginParam *pParam, bool fMute)
{
	return (*pParam->Callback)(pParam, MESSAGE_SETVOLUME, -1, fMute) != 0;
}

/**
@brief ステレオモード
*/
TVTEST_DEFINE_ENUM(StereoModeType, int) {
	STEREOMODE_STEREO, /**< ステレオ */
	STEREOMODE_LEFT,   /**< 左(主音声) */
	STEREOMODE_RIGHT   /**< 右(副音声) */
};

/**
@brief ステレオモードを取得する

@param[in] pParam プラグインパラメータ

@return TVTest ver.0.9.0 以降は、デュアルモノラル時に選択される音声。
        ver.0.9.0 より前は、ステレオもしくはデュアルモノラル時に現在選択されている音声。

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_GETSTEREOMODE
*/
inline StereoModeType MsgGetStereoMode(PluginParam *pParam)
{
	return static_cast<StereoModeType>((*pParam->Callback)(pParam, MESSAGE_GETSTEREOMODE, 0, 0));
}

/**
@brief ステレオモードを設定する

@param[in] pParam プラグインパラメータ
@param[in] StereoMode TVTest ver.0.10.0 以降は、デュアルモノラル時に選択される音声。
                      ver.0.10.0 より前は、ステレオもしくはデュアルモノラル時に再生される音声。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_SETSTEREOMODE
*/
inline bool MsgSetStereoMode(PluginParam *pParam, StereoModeType StereoMode)
{
	return (*pParam->Callback)(pParam, MESSAGE_SETSTEREOMODE, static_cast<LPARAM>(StereoMode), 0) != 0;
}

/**
@brief 全画面表示の状態を取得する

@param[in] pParam プラグインパラメータ

@retval true 全画面表示
@retval false 全画面表示ではない

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_GETFULLSCREEN
*/
inline bool MsgGetFullscreen(PluginParam *pParam)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETFULLSCREEN, 0, 0) != 0;
}

/**
@brief 全画面表示の状態を設定する

@param[in] pParam プラグインパラメータ
@param[in] fFullscreen 全画面表示にする場合は true、全画面表示を解除する場合は false

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_SETFULLSCREEN
*/
inline bool MsgSetFullscreen(PluginParam *pParam, bool fFullscreen)
{
	return (*pParam->Callback)(pParam, MESSAGE_SETFULLSCREEN, fFullscreen, 0) != 0;
}

/**
@brief 再生が有効であるか取得する

@param[in] pParam プラグインパラメータ

@retval true 再生中
@retval false 再生オフ

@par Corresponding message
TVTest::MESSAGE_GETPREVIEW
*/
inline bool MsgGetPreview(PluginParam *pParam)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETPREVIEW, 0, 0) != 0;
}

/**
@brief 再生の有効状態を設定する

@param[in] pParam プラグインパラメータ
@param[in] fPreview 再生を有効にする場合は true、再生オフにする場合は false

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_SETPREVIEW
*/
inline bool MsgSetPreview(PluginParam *pParam, bool fPreview)
{
	return (*pParam->Callback)(pParam, MESSAGE_SETPREVIEW, fPreview, 0) != 0;
}

/**
@brief 待機状態であるか取得する

@param[in] pParam プラグインパラメータ

@retval true 待機状態
@retval false 待機状態ではない

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_GETSTANDBY
*/
inline bool MsgGetStandby(PluginParam *pParam)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETSTANDBY, 0, 0) != 0;
}

/**
@brief 待機状態を設定する

@param[in] pParam プラグインパラメータ
@param[in] fStandby 待機状態にする場合は true、待機状態を解除する場合は false

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_SETSTANDBY
*/
inline bool MsgSetStandby(PluginParam *pParam, bool fStandby)
{
	return (*pParam->Callback)(pParam, MESSAGE_SETSTANDBY, fStandby, 0) != 0;
}

/**
@brief 常に最前面表示の状態を取得する

@param[in] pParam プラグインパラメータ

@retval true 常に最前面表示
@retval false 常に最前面表示ではない

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_GETALWAYSONTOP
*/
inline bool MsgGetAlwaysOnTop(PluginParam *pParam)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETALWAYSONTOP, 0, 0) != 0;
}

/**
@brief 常に最前面表示の状態を設定する

@param[in] pParam プラグインパラメータ
@param[in] fAlwaysOnTop 常に最前面表示にする場合は true、常に最前面表示を解除する場合は false

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_SETALWAYSONTOP
*/
inline bool MsgSetAlwaysOnTop(PluginParam *pParam, bool fAlwaysOnTop)
{
	return (*pParam->Callback)(pParam, MESSAGE_SETALWAYSONTOP, fAlwaysOnTop, 0) != 0;
}

/**
@brief 画像キャプチャのフラグ
*/
TVTEST_DEFINE_ENUM(CaptureImageFlag, DWORD) {
	CAPTURE_IMAGE_FLAG_NONE = 0x00000000U /**< なし(0) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(CaptureImageFlag)

/**
@brief 画像をキャプチャする

@param[in] pParam プラグインパラメータ
@param[in] Flags フラグ。現在は常に TVTest::CAPTURE_IMAGE_FLAG_NONE 。

@return パック DIB データ(BITMAPINFOHEADER + ピクセルデータ)へのポインタ。
        不要になった場合は MsgMemoryFree() で解放します。
        キャプチャできなかった場合は nullptr が返ります。

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_CAPTUREIMAGE
*/
inline void *MsgCaptureImage(PluginParam *pParam, CaptureImageFlag Flags = CAPTURE_IMAGE_FLAG_NONE)
{
	return reinterpret_cast<void*>((*pParam->Callback)(pParam, MESSAGE_CAPTUREIMAGE, static_cast<LPARAM>(Flags), 0));
}

/**
@brief 画像を保存する

@param[in] pParam プラグインパラメータ

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_SAVEIMAGE
*/
inline bool MsgSaveImage(PluginParam *pParam)
{
	return (*pParam->Callback)(pParam, MESSAGE_SAVEIMAGE, 0, 0) != 0;
}

#if TVTEST_PLUGIN_VERSION < TVTEST_PLUGIN_VERSION_(0, 0, 9)
/**
@brief リセットを行う

@param[in] pParam プラグインパラメータ

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_RESET
*/
inline bool MsgReset(PluginParam *pParam)
{
	return (*pParam->Callback)(pParam, MESSAGE_RESET, 0, 0) != 0;
}
#else
/**
@brief リセットのフラグ
*/
TVTEST_DEFINE_ENUM(ResetFlag, DWORD) {
	RESET_ALL    = 0x00000000UL, /**< 全て */
	RESET_VIEWER = 0x00000001UL  /**< ビューアのみ */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(ResetFlag)

/**
@brief リセットを行う

@param[in] pParam プラグインパラメータ
@param[in] Flags リセットのフラグ(RESET_*) (ver.0.0.9 以降)

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_RESET
*/
inline bool MsgReset(PluginParam *pParam, ResetFlag Flags = RESET_ALL)
{
	return (*pParam->Callback)(pParam, MESSAGE_RESET, static_cast<LPARAM>(Flags), 0) != 0;
}
#endif

/**
@brief ウィンドウクローズのフラグ
*/
TVTEST_DEFINE_ENUM(CloseFlag, DWORD) {
	CLOSE_NONE = 0x00000000UL, /**< なし(0) */
	CLOSE_EXIT = 0x00000001UL  /**< 必ず終了させる */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(CloseFlag)

/**
@brief ウィンドウを閉じる

@param[in] pParam プラグインパラメータ
@param[in] Flags フラグ(CLOSE_*)

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_CLOSE
*/
inline bool MsgClose(PluginParam *pParam, CloseFlag Flags = CLOSE_NONE)
{
	return (*pParam->Callback)(pParam, MESSAGE_CLOSE, static_cast<LPARAM>(Flags), 0) != 0;
}

/**
@brief ストリームコールバック関数

pData は 188 バイトの TS パケットが渡されます。
FALSE を返すとパケットが破棄されます。
*/
typedef BOOL (CALLBACK *StreamCallbackFunc)(BYTE *pData, void *pClientData);

/**
@brief ストリームコールバックフラグ
*/
TVTEST_DEFINE_ENUM(StreamCallbackFlag, DWORD) {
	STREAM_CALLBACK_NONE   = 0x00000000UL, /**< なし(0) */
	STREAM_CALLBACK_REMOVE = 0x00000001UL  /**< コールバックの削除 */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(StreamCallbackFlag)

/**
@brief ストリームコールバックの情報
*/
struct StreamCallbackInfo
{
	DWORD Size;                  /**< 構造体のサイズ */
	StreamCallbackFlag Flags;    /**< フラグ(STREAM_CALLBACK_*) */
	StreamCallbackFunc Callback; /**< コールバック関数 */
	void *pClientData;           /**< コールバック関数に渡されるデータ */
};

/**
@brief ストリームコールバックを設定する

ストリームコールバックを登録すると、TS データを受け取ることができます。
コールバック関数は一つのプラグインで複数登録できます。
ストリームコールバック関数で処理が遅延すると全体が遅延するので、
時間が掛かる処理は別スレッドで行うなどしてください。

@param[in] pParam プラグインパラメータ
@param[in] Flags フラグ(STREAM_CALLBACK_*)
@param[in] Callback コールバック関数
@param[in] pClientData コールバック関数に渡すデータ

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_SETSTREAMCALLBACK
*/
inline bool MsgSetStreamCallback(
	PluginParam *pParam, StreamCallbackFlag Flags,
	StreamCallbackFunc Callback, void *pClientData = nullptr)
{
	StreamCallbackInfo Info;
	Info.Size = sizeof(StreamCallbackInfo);
	Info.Flags = Flags;
	Info.Callback = Callback;
	Info.pClientData = pClientData;
	return (*pParam->Callback)(pParam, MESSAGE_SETSTREAMCALLBACK, reinterpret_cast<LPARAM>(&Info), 0) != 0;
}

/**
@brief プラグインの有効状態を設定する

@param[in] pParam プラグインパラメータ
@param[in] fEnable 有効にする場合は true、無効にする場合は false

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_ENABLEPLUGIN
*/
inline bool MsgEnablePlugin(PluginParam *pParam, bool fEnable)
{
	return (*pParam->Callback)(pParam, MESSAGE_ENABLEPLUGIN, fEnable, 0) != 0;
}

/**
@brief 色の設定を取得する

@param[in] pParam プラグインパラメータ
@param[in] pszColor 取得したい色の名前。
                    名前は配色設定ファイル(*.httheme)の項目名("StatusBack" など)と同じです。

@return 色を表す COLORREF 値。
        pszColor が無効な場合は CLR_INVALID。

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_GETCOLOR
*/
inline COLORREF MsgGetColor(PluginParam *pParam, LPCWSTR pszColor)
{
	return static_cast<COLORREF>((*pParam->Callback)(pParam, MESSAGE_GETCOLOR, reinterpret_cast<LPARAM>(pszColor), 0));
}

/**
@brief ARIB 文字列デコードフラグ
*/
TVTEST_DEFINE_ENUM(ARIBStringDecodeFlag, DWORD) {
	ARIB_STRING_DECODE_FLAG_NONE = 0x00000000U /**< なし(0) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(ARIBStringDecodeFlag)

/**
@brief ARIB 文字列のデコード情報
*/
struct ARIBStringDecodeInfo
{
	DWORD Size;                 /**< 構造体のサイズ */
	ARIBStringDecodeFlag Flags; /**< フラグ(現在は常に ARIB_STRING_DECODE_FLAG_NONE) */
	const void *pSrcData;       /**< 変換元データ */
	DWORD SrcLength;            /**< 変換元サイズ(バイト単位) */
	LPWSTR pszDest;             /**< 変換先バッファ */
	DWORD DestLength;           /**< 変換先バッファのサイズ(文字単位) */
};

/**
@brief ARIB 文字列をデコードする

@param[in] pParam プラグインパラメータ
@param[in] pSrcData デコードする ARIB 文字列データへのポインタ
@param[in] SrcLength デコードする ARIB 文字列データの長さ(バイト単位)
@param[out] pszDest デコードした文字列を格納するバッファへのポインタ
@param[in] DestLength pszDest の差すバッファの長さ

@retval true 正常終了
@retval false エラー発生

@note デコードした文字列の長さが DestLength 以上の場合、DestLength - 1 の長さまで切り詰められ、終端に空文字が付加されます。

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_DECODEARIBSTRING
*/
inline bool MsgDecodeARIBString(
	PluginParam *pParam, const void *pSrcData,
	DWORD SrcLength, LPWSTR pszDest, DWORD DestLength)
{
	ARIBStringDecodeInfo Info;
	Info.Size = sizeof(ARIBStringDecodeInfo);
	Info.Flags = ARIB_STRING_DECODE_FLAG_NONE;
	Info.pSrcData = pSrcData;
	Info.SrcLength = SrcLength;
	Info.pszDest = pszDest;
	Info.DestLength = DestLength;
	return (*pParam->Callback)(pParam, MESSAGE_DECODEARIBSTRING, reinterpret_cast<LPARAM>(&Info), 0) != 0;
}

/**
@brief 番組の情報
*/
struct ProgramInfo
{
	DWORD Size;             /**< 構造体のサイズ */
	WORD ServiceID;         /**< サービス ID */
	WORD EventID;           /**< イベント ID */
	LPWSTR pszEventName;    /**< イベント名 */
	int MaxEventName;       /**< イベント名の最大長 */
	LPWSTR pszEventText;    /**< イベントテキスト */
	int MaxEventText;       /**< イベントテキストの最大長 */
	LPWSTR pszEventExtText; /**< 追加イベントテキスト */
	int MaxEventExtText;    /**< 追加イベントテキストの最大長 */
	SYSTEMTIME StartTime;   /**< 開始日時(EPG 日時 : UTC+9) */
	DWORD Duration;         /**< 長さ(秒単位) */
};

/**
@brief 現在の番組の情報を取得する

@param[in] pParam プラグインパラメータ
@param[in,out] pInfo 情報を取得する ProgramInfo 構造体へのポインタ。
                     事前に ProgramInfo::Size メンバに構造体のサイズを設定します。
                     また、ProgramInfo::pszEventName / ProgramInfo::pszEventText / ProgramInfo::pszEventExtText メンバに取得先のバッファへのポインタを、
                     ProgramInfo::MaxEventName / ProgramInfo::MaxEventText / ProgramInfo::MaxEventExtText メンバにバッファの長さ(要素数)を設定します。
                     必要のない情報は、ポインタを nullptr にすると取得されません。
@param[in] fNext 次の番組の情報を取得する場合は true、現在の番組の情報を取得する場合は false

@retval true 正常終了
@retval false エラー発生

@remark MsgGetEpgEventInfo() または MsgGetCurrentEpgEventInfo() で、より詳しい番組情報を取得することもできます。

@since ver.0.0.0

@par Corresponding message
TVTest::MESSAGE_GETCURRENTPROGRAMINFO

@sa MsgGetCurrentEpgEventInfo()
*/
inline bool MsgGetCurrentProgramInfo(PluginParam *pParam, ProgramInfo *pInfo, bool fNext = false)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETCURRENTPROGRAMINFO, reinterpret_cast<LPARAM>(pInfo), fNext) != 0;
}

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 1)

/**
@brief 指定されたイベントの通知に対応しているか取得する

@param[in] pParam プラグインパラメータ
@param[in] Event イベントのコード(EVENT_*)

@retval true イベントの通知に対応している
@retval false イベントの通知に対応していない

@since ver.0.0.1

@par Corresponding message
TVTest::MESSAGE_QUERYEVENT
*/
inline bool MsgQueryEvent(PluginParam *pParam, UINT Event)
{
	return (*pParam->Callback)(pParam, MESSAGE_QUERYEVENT, Event, 0) != 0;
}

/**
@brief 現在のチューニング空間及びチューニング空間数を取得する

@param[in] pParam プラグインパラメータ
@param[out] pNumSpaces チューニング空間の数を取得する変数へのポインタ。取得する必要がない場合は nullptr。

@return 現在のチューニング空間のインデックス

@since ver.0.0.1

@par Corresponding message
TVTest::MESSAGE_GETTUNINGSPACE
*/
inline int MsgGetTuningSpace(PluginParam *pParam, int *pNumSpaces = nullptr)
{
	return static_cast<int>((*pParam->Callback)(pParam, MESSAGE_GETTUNINGSPACE, reinterpret_cast<LPARAM>(pNumSpaces), 0));
}

/**
@brief チューニング空間の種類
*/
TVTEST_DEFINE_ENUM(TuningSpaceType, int) {
	TUNINGSPACE_UNKNOWN,     /**< 不明 */
	TUNINGSPACE_TERRESTRIAL, /**< 地上デジタル */
	TUNINGSPACE_BS,          /**< BS */
	TUNINGSPACE_110CS        /**< 110度CS */
};

/**
@brief チューニング空間の情報
*/
struct TuningSpaceInfo
{
	DWORD Size;            /**< 構造体のサイズ */
	TuningSpaceType Space; /**< チューニング空間の種類(TUNINGSPACE_*)。チューニング空間名から推測したもの。 */
	WCHAR szName[64];      /**< チューニング空間名 */
};

/**
@brief チューニング空間の情報を取得する

@param[in] pParam プラグインパラメータ
@param[in] Index 情報を取得するチューニング空間のインデックス
@param[in,out] pInfo 情報を取得する TuningSpaceInfo 構造体へのポインタ。
                     事前に TuningSpaceInfo::Size メンバに構造体のサイズを設定します。

@return 現在のチューニング空間のインデックス

@since ver.0.0.1

@par Corresponding message
TVTest::MESSAGE_GETTUNINGSPACEINFO
*/
inline bool MsgGetTuningSpaceInfo(PluginParam *pParam, int Index, TuningSpaceInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETTUNINGSPACEINFO, Index, reinterpret_cast<LPARAM>(pInfo)) != 0;
}

/**
@brief チャンネルを次に送る

@param[in] pParam プラグインパラメータ
@param[in] fNext 次のチャンネルにする場合は true、前のチャンネルにする場合は false

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.1

@par Corresponding message
TVTest::MESSAGE_SETNEXTCHANNEL
*/
inline bool MsgSetNextChannel(PluginParam *pParam, bool fNext = true)
{
	return (*pParam->Callback)(pParam, MESSAGE_SETNEXTCHANNEL, fNext, 0) != 0;
}

#endif // TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 1)

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 2)
/**
@brief 現在の音声ストリームを取得する

@param[in] pParam プラグインパラメータ

@return 現在の音声ストリームのインデックス

@remark 音声ストリームの数は MsgGetAudioStreamCount() または MsgGetServiceInfo() で取得できます。

@since ver.0.0.2

@par Corresponding message
TVTest::MESSAGE_GETAUDIOSTREAM
*/
inline int MsgGetAudioStream(PluginParam *pParam)
{
	return static_cast<int>((*pParam->Callback)(pParam, MESSAGE_GETAUDIOSTREAM, 0, 0));
}

/**
@brief 音声ストリームを設定する

@param[in] pParam プラグインパラメータ
@param[in] Index 音声ストリームのインデックス

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.2

@par Corresponding message
TVTest::MESSAGE_SETAUDIOSTREAM
*/
inline bool MsgSetAudioStream(PluginParam *pParam, int Index)
{
	return (*pParam->Callback)(pParam, MESSAGE_SETAUDIOSTREAM, Index, 0) != 0;
}

#endif // TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 2)

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 3)
/**
@brief プラグインの有効状態を取得する

@param[in] pParam プラグインパラメータ

@retval true プラグインが有効
@retval false プラグインが無効

@since ver.0.0.3

@par Corresponding message
TVTest::MESSAGE_ISPLUGINENABLED
*/
inline bool MsgIsPluginEnabled(PluginParam *pParam)
{
	return (*pParam->Callback)(pParam, MESSAGE_ISPLUGINENABLED, 0, 0) != 0;
}

/**
@brief コマンドの情報
*/
struct CommandInfo
{
	int ID;          /**< 識別子 */
	LPCWSTR pszText; /**< コマンドの文字列 */
	LPCWSTR pszName; /**< コマンドの名前 */
};

/**
@brief コマンドを登録する

TVTInitialize() 内で呼びます。
コマンドを登録すると、ショートカットキーやリモコンに機能を割り当てられるようになります。

コマンドが実行されると TVTest::EVENT_COMMAND イベントが送られます。
その際、パラメータとして識別子が渡されます。

@param[in] pParam プラグインパラメータ
@param[in] ID コマンドの識別子
@param[in] pszText コマンドの文字列
@param[in] pszName コマンドの名前

@retval true 正常終了
@retval false エラー発生

@remark MsgRegisterCommand() から機能が追加されたバージョンの MsgRegisterPluginCommand() もあります。

@since ver.0.0.3

@par Corresponding message
TVTest::MESSAGE_REGISTERCOMMAND

@par Example
@code{.cpp}
constexpr int ID_MYCOMMAND = 1;

TVTest::MsgRegisterCommand(pParam, ID_MYCOMMAND, L"MyCommand", L"私のコマンド");
@endcode
*/
inline bool MsgRegisterCommand(PluginParam *pParam, int ID, LPCWSTR pszText, LPCWSTR pszName)
{
	CommandInfo Info;
	Info.ID = ID;
	Info.pszText = pszText;
	Info.pszName = pszName;
	return (*pParam->Callback)(pParam, MESSAGE_REGISTERCOMMAND, reinterpret_cast<LPARAM>(&Info), 1) != 0;
}

/**
@brief コマンドを登録する

TVTInitialize 内で呼びます。

@param[in] pParam プラグインパラメータ
@param[in] pCommandList コマンドの情報を格納した CommandInfo 構造体の配列へのポインタ
@param[in] NumCommands pCommandList の差す配列の要素数

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.3

@par Corresponding message
TVTest::MESSAGE_REGISTERCOMMAND
*/
inline bool MsgRegisterCommand(PluginParam *pParam, const CommandInfo *pCommandList, int NumCommands)
{
	return (*pParam->Callback)(pParam, MESSAGE_REGISTERCOMMAND, reinterpret_cast<LPARAM>(pCommandList), NumCommands) != 0;
}

/**
@brief ログを記録する

設定のログの項目に表示されます。

@param[in] pParam プラグインパラメータ
@param[in] pszText ログの文字列

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.3

@par Corresponding message
TVTest::MESSAGE_ADDLOG
*/
inline bool MsgAddLog(PluginParam *pParam, LPCWSTR pszText)
{
	return (*pParam->Callback)(pParam, MESSAGE_ADDLOG, reinterpret_cast<LPARAM>(pszText), 0) != 0;
}

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 14)

/**
@brief ログの種類
*/
TVTEST_DEFINE_ENUM(LogType, int) {
	LOG_TYPE_INFORMATION, /**< 情報 */
	LOG_TYPE_WARNING,     /**< 警告 */
	LOG_TYPE_ERROR        /**< エラー */
};

/**
@brief ログを記録する

設定のログの項目に表示されます。

@param[in] pParam プラグインパラメータ
@param[in] pszText ログの文字列
@param[in] Type ログの種類(プラグイン仕様 0.0.14 以降)

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.3

@par Corresponding message
TVTest::MESSAGE_ADDLOG
*/
inline bool MsgAddLog(PluginParam *pParam, LPCWSTR pszText, LogType Type)
{
	return (*pParam->Callback)(pParam, MESSAGE_ADDLOG, reinterpret_cast<LPARAM>(pszText), static_cast<LPARAM>(Type)) != 0;
}
#endif

#endif // TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 3)

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 5)

/**
@brief ステータス(TVTest::MESSAGE_GETSTATUS で取得できる内容)をリセットする

@param[in] pParam プラグインパラメータ

@retval true 正常終了
@retval false エラー発生

@remark リセットが行われると TVTest::EVENT_STATUSRESET が送られます。

@since ver.0.0.5

@par Corresponding message
TVTest::MESSAGE_RESETSTATUS
*/
inline bool MsgResetStatus(PluginParam *pParam)
{
	return (*pParam->Callback)(pParam, MESSAGE_RESETSTATUS, 0, 0) != 0;
}

#endif // TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 5)

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 6)

/**
@brief 音声サンプルのコールバック関数

渡されるサンプルは 48kHz / 16ビット固定です。
pData の先には Samples * Channels 分のデータが入っています。
5.1ch をダウンミックスする設定になっている場合、ダウンミックスされたデータが渡されます。
戻り値は今のところ常に 0 を返します。
*/
typedef LRESULT (CALLBACK *AudioCallbackFunc)(short *pData, DWORD Samples, int Channels, void *pClientData);

/**
@brief 音声のサンプルを取得するコールバック関数を設定する

@param[in] pParam プラグインパラメータ
@param[in] pCallback コールバック関数。nullptr を指定すると、設定が解除されます。
@param[in] pClientData コールバック関数に渡されるデータ

@retval true 正常終了
@retval false エラー発生

@note 一つのプラグインで設定できるコールバック関数は一つだけです。
      2回目以降の呼び出しでは、前回の設定が上書きされます。

@since ver.0.0.6

@par Corresponding message
TVTest::MESSAGE_SETAUDIOCALLBACK
*/
inline bool MsgSetAudioCallback(PluginParam *pParam, AudioCallbackFunc pCallback, void *pClientData = nullptr)
{
	return (*pParam->Callback)(pParam, MESSAGE_SETAUDIOCALLBACK, reinterpret_cast<LPARAM>(pCallback), reinterpret_cast<LPARAM>(pClientData)) != 0;
}

#endif // TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 6)

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 7)

/**
@brief コマンドを実行する

文字列を指定してコマンドを実行します。
コマンドとは TVTest の各機能を実行するためのものです。
コマンドの情報は MsgGetAppCommandInfo() で取得できます。

@param[in] pParam プラグインパラメータ
@param[in] pszCommand 実行するコマンドの文字列

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.7

@par Corresponding message
TVTest::MESSAGE_DOCOMMAND

@par Example
@code{.cpp}
// 設定ダイアログを表示
TVTest::MsgDoCommand(pParam, L"Options");
@endcode
*/
inline bool MsgDoCommand(PluginParam *pParam, LPCWSTR pszCommand)
{
	return (*pParam->Callback)(pParam, MESSAGE_DOCOMMAND, reinterpret_cast<LPARAM>(pszCommand), 0) != 0;
}

#endif // TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 7)

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 8)

/**
@brief ホストプログラムの情報
*/
struct HostInfo
{
	DWORD Size;                   /**< 構造体のサイズ */
	LPCWSTR pszAppName;           /**< プログラム名 ("TVTest"、"TVH264" など) */
	struct {
		int Major;                /**< メジャーバージョン */
		int Minor;                /**< マイナーバージョン */
		int Build;                /**< ビルドナンバー */
	} Version;                    /**< バージョン番号 */
	LPCWSTR pszVersionText;       /**< バージョン文字列 ("1.2.0" など) */
	DWORD SupportedPluginVersion; /**< 対応しているプラグインのバージョン。 #TVTEST_PLUGIN_VERSION_(?,?,?) で表される値 */
};

/**
@brief ホストプログラムの情報を取得する

@param[in] pParam プラグインパラメータ
@param[in,out] pInfo 情報を取得する HostInfo 構造体へのポインタ。
                     事前に HostInfo::Size メンバに構造体のサイズを設定して呼び出します。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.8

@par Corresponding message
TVTest::MESSAGE_GETHOSTINFO
*/
inline bool MsgGetHostInfo(PluginParam *pParam, HostInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETHOSTINFO, reinterpret_cast<LPARAM>(pInfo), 0) != 0;
}

#endif // TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 8)

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 9)

/**
@brief 設定の情報
*/
struct SettingInfo
{
	LPCWSTR pszName;       /**< 設定名 */
	DWORD Type;            /**< 値の型 (SETTING_TYPE_*) */
	union {
		int Int;           /**< int */
		unsigned int UInt; /**< unsigned int */
		LPWSTR pszString;  /**< 文字列 */
		void *pData;       /**< データ */
	} Value;               /**< 値 */
	DWORD ValueSize;       /**< 値のサイズ (バイト単位) */
};

/**
@brief 設定の値の型
*/
enum SettingType {
	SETTING_TYPE_UNDEFINED, /**< 未定義 */
	SETTING_TYPE_INT,       /**< int */
	SETTING_TYPE_UINT,      /**< unsigned int */
	SETTING_TYPE_STRING,    /**< 文字列 */
	SETTING_TYPE_DATA       /**< データ */
};

/**
@brief 設定を取得する

以下の設定が取得できます。
設定名の大文字と小文字は区別されません。

| 設定名          | 内容                            | 型     | 対応バージョン |
|-----------------|---------------------------------|--------|----------------|
| DriverDirectory | BonDriver の検索ディレクトリ    | 文字列 | 0.0.9          |
| IniFilePath     | Ini ファイルのパス              | 文字列 | 0.0.9          |
| RecordFolder    | 録画時の保存先フォルダ          | 文字列 | 0.0.9          |
| RecordFileName  | 録画のファイル名(※1)           | 文字列 | 0.0.14         |
| CaptureFolder   | キャプチャの保存先フォルダ(※2) | 文字列 | 0.0.14         |
| CaptureFileName | キャプチャのファイル名(※1)     | 文字列 | 0.0.14         |

+ ※1 "%event-name%" などの変数が含まれている可能性があります。
      MsgFormatVarString() を使って変数を展開できます。
+ ※2 相対パスの可能性があります。その場合実行ファイルの場所が基準です。

@param[in] pParam プラグインパラメータ
@param[in,out] pInfo 設定を取得する SettingInfo 構造体へのポインタ。
                     呼び出す前に各メンバを設定しておきます。
                     文字列とデータの場合、 SettingInfo::ValueSize に設定の格納に必要なバイト数が返されます。

@retval true 正常終了
@retval false エラー発生

@note 通常は型ごとにオーバーロードされた関数を使用した方が便利です。

@since ver.0.0.9

@par Corresponding message
TVTest::MESSAGE_GETSETTING
*/
inline bool MsgGetSetting(PluginParam *pParam, SettingInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETSETTING, reinterpret_cast<LPARAM>(pInfo), 0) != 0;
}

/**
@brief int の設定を取得する

@param[in] pParam プラグインパラメータ
@param[in] pszName 取得する設定名
@param[out] pValue 値を取得する変数へのポインタ

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.9

@par Corresponding message
TVTest::MESSAGE_GETSETTING
*/
inline bool MsgGetSetting(PluginParam *pParam, LPCWSTR pszName, int *pValue)
{
	SettingInfo Info;
	Info.pszName = pszName;
	Info.Type = SETTING_TYPE_INT;
	Info.ValueSize = sizeof(int);
	if (!(*pParam->Callback)(pParam, MESSAGE_GETSETTING, reinterpret_cast<LPARAM>(&Info), 0))
		return false;
	*pValue = Info.Value.Int;
	return true;
}

/**
@brief unsigned int の設定を取得する

@param[in] pParam プラグインパラメータ
@param[in] pszName 取得する設定名
@param[out] pValue 値を取得する変数へのポインタ

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.9

@par Corresponding message
TVTest::MESSAGE_GETSETTING
*/
inline bool MsgGetSetting(PluginParam *pParam, LPCWSTR pszName, unsigned int *pValue)
{
	SettingInfo Info;
	Info.pszName = pszName;
	Info.Type = SETTING_TYPE_UINT;
	Info.ValueSize = sizeof(unsigned int);
	if (!(*pParam->Callback)(pParam, MESSAGE_GETSETTING, reinterpret_cast<LPARAM>(&Info), 0))
		return false;
	*pValue = Info.Value.UInt;
	return true;
}

/**
@brief 文字列の設定を取得する

@param[in] pParam プラグインパラメータ
@param[in] pszName 取得する設定名
@param[out] pszString 文字列を取得するバッファへのポインタ。
                      nullptr にすると、必要なバッファの長さ(終端の空文字を含む)が返ります。
@param[in] MaxLength pszString の差す先に格納できる最大長(終端の空文字を含む)

@return 取得した文字列の長さ(終端の空文字を含む)。
        設定が取得できなかった場合は 0。

@note 文字列が MaxLength の長さに収まらない場合、途中で切り詰められて終端に空文字が付加されます。

@since ver.0.0.9

@par Corresponding message
TVTest::MESSAGE_GETSETTING

@par Example
@code{.cpp}
	WCHAR szIniPath[MAX_PATH];
	if (TVTest::MsgGetSetting(pParam, L"IniFilePath", szIniPath, MAX_PATH) > 0) {
		// 呼び出しが成功した場合は、szIniPath に Ini ファイルのパスが格納されています
	}
@endcode
*/
inline DWORD MsgGetSetting(PluginParam *pParam, LPCWSTR pszName, LPWSTR pszString, DWORD MaxLength)
{
	SettingInfo Info;
	Info.pszName = pszName;
	Info.Type = SETTING_TYPE_STRING;
	Info.Value.pszString = pszString;
	Info.ValueSize = MaxLength * sizeof(WCHAR);
	if (!(*pParam->Callback)(pParam, MESSAGE_GETSETTING, reinterpret_cast<LPARAM>(&Info), 0))
		return 0;
	return Info.ValueSize / sizeof(WCHAR);
}

/**
@brief BonDriver のフルパス名を取得する

@param[in] pParam プラグインパラメータ
@param[out] pszPath パスを取得するバッファへのポインタ。nullptr で長さの取得のみ行う。
@param[in] MaxLength pszPath の差す先に格納できる最大長(終端の空文字を含む)

@return パスの長さ(終端の空文字を除く)。
        BonDriver が読み込まれていない場合は 0。

@note パスの長さが MaxLength 以上である場合、pszPath に格納される文字列は MaxLength - 1 の長さに切り詰められ、終端に空文字が付加されます。
      その際も戻り値は切り捨てられる前の本来の長さです。

@since ver.0.0.9

@par Corresponding message
TVTest::MESSAGE_GETDRIVERFULLPATHNAME
*/
inline int MsgGetDriverFullPathName(PluginParam *pParam, LPWSTR pszPath, int MaxLength)
{
	return static_cast<int>((*pParam->Callback)(pParam, MESSAGE_GETDRIVERFULLPATHNAME, reinterpret_cast<LPARAM>(pszPath), MaxLength));
}

#endif // TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 9)

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 10)

/**
@brief ロゴの画像を取得する

@param[in] pParam プラグインパラメータ
@param[in] NetworkID ネットワーク ID
@param[in] ServiceID サービス ID
@param[in] LogoType ロゴの種類。0 から 5 までで指定します。以下のサイズのロゴが取得されます。
                    | LogoType | サイズ |
                    |----------|--------|
                    | 0        | 48x24  |
                    | 1        | 36x24  |
                    | 2        | 48x27  |
                    | 3        | 72x36  |
                    | 4        | 54x36  |
                    | 5        | 64x36  |
                    いずれのロゴも16:9で表示すると本来の比率になります。

@return ビットマップ(DIB セクション)のハンドル。
        画像が取得できない場合は nullptr。

@note ビットマップは不要になった時に DeleteObject() で破棄してください。

@since ver.0.0.10

@par Corresponding message
TVTest::MESSAGE_GETLOGO
*/
inline HBITMAP MsgGetLogo(PluginParam *pParam, WORD NetworkID, WORD ServiceID, BYTE LogoType)
{
	return reinterpret_cast<HBITMAP>((*pParam->Callback)(pParam, MESSAGE_GETLOGO, MAKELONG(NetworkID, ServiceID), LogoType));
}

/**
@brief 利用可能なロゴの種類を取得する

@param[in] pParam プラグインパラメータ
@param[in] NetworkID ネットワーク ID
@param[in] ServiceID サービス ID

@return 利用可能なロゴを表すフラグ。
        下位から 1 ビットごとに LogoType の 0 から 5 までを表し、ビットが 1 であればその種類のロゴが利用できます。
        LogoType については MsgGetLogo() を参照してください。

@since ver.0.0.10

@par Corresponding message
TVTest::MESSAGE_GETAVAILABLELOGOTYPE

@par Example
@code{.cpp}
	if (TVTest::MsgGetAvailableLogoType(pParam, NetworkID, ServiceID) & 1) {
		// タイプ0のロゴが利用できる
	}
@endcode
*/
inline UINT MsgGetAvailableLogoType(PluginParam *pParam, WORD NetworkID, WORD ServiceID)
{
	return static_cast<UINT>((*pParam->Callback)(pParam, MESSAGE_GETAVAILABLELOGOTYPE, MAKELONG(NetworkID, ServiceID), 0));
}

/**
@brief 録画ファイルを切り替える

現在録画中のファイルを閉じて、指定されたパスにファイルを作成して続きを録画するようにします。
新しいファイルが開けなかった場合は、今までのファイルで録画が継続されます。

@param[in] pParam プラグインパラメータ
@param[in] pszFileName 新しいファイルのパス

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.10

@par Corresponding message
TVTest::MESSAGE_RELAYRECORD
*/
inline bool MsgRelayRecord(PluginParam *pParam, LPCWSTR pszFileName)
{
	return (*pParam->Callback)(pParam, MESSAGE_RELAYRECORD, reinterpret_cast<LPARAM>(pszFileName), 0) != 0;
}

/**
@brief サイレントモードのパラメータ
*/
enum {
	SILENTMODE_GET, /**< 取得 */
	SILENTMODE_SET  /**< 設定 */
};

/**
@brief サイレントモードを取得する

サイレントモードではエラー時などにダイアログが出なくなります。
コマンドラインで /silent を指定するか、 MsgSetSilentMode() で設定すればサイレントモードになります。

@param[in] pParam プラグインパラメータ

@retval true サイレントモードである
@retval false サイレントモードではない

@since ver.0.0.10

@par Corresponding message
TVTest::MESSAGE_SILENTMODE
*/
inline bool MsgGetSilentMode(PluginParam *pParam)
{
	return (*pParam->Callback)(pParam, MESSAGE_SILENTMODE, SILENTMODE_GET, 0) != 0;
}

/**
@brief サイレントモードを設定する

@param[in] pParam プラグインパラメータ
@param[in] fSilent サイレントモードにする場合は true、サイレントモードを解除する場合は false

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.10

@par Corresponding message
TVTest::MESSAGE_SILENTMODE
*/
inline bool MsgSetSilentMode(PluginParam *pParam, bool fSilent)
{
	return (*pParam->Callback)(pParam, MESSAGE_SILENTMODE, SILENTMODE_SET, static_cast<LPARAM>(fSilent)) != 0;
}

/**
@brief 録画のクライアント
*/
TVTEST_DEFINE_ENUM(RecordClient, DWORD) {
	RECORD_CLIENT_USER,        /**< ユーザーの操作 */
	RECORD_CLIENT_COMMANDLINE, /**< コマンドラインでの指定 */
	RECORD_CLIENT_PLUGIN       /**< プラグインからの指定 */
};

/**
@brief 録画開始情報で変更した項目
*/
TVTEST_DEFINE_ENUM(StartRecordModifiedFlag, DWORD) {
	STARTRECORD_MODIFIED_NONE     = 0x00000000UL, /**< なし(0) */
	STARTRECORD_MODIFIED_FILENAME = 0x00000001UL  /**< ファイル名 */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(StartRecordModifiedFlag)

/**
@brief 録画開始情報で変更した項目
*/
TVTEST_DEFINE_ENUM(StartRecordFlag, DWORD) {
	STARTRECORD_FLAG_NONE = 0x00000000UL, /**< なし(0) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(StartRecordFlag)

/**
@brief 録画開始情報

TVTest::EVENT_STARTRECORD で渡されます。
*/
struct StartRecordInfo
{
	DWORD Size;                       /**< 構造体のサイズ */
	StartRecordFlag Flags;            /**< フラグ(現在未使用) */
	StartRecordModifiedFlag Modified; /**< 変更した項目(STARTRECORD_MODIFIED_*) */
	RecordClient Client;              /**< 録画のクライアント(RECORD_CLIENT_*) */
	LPWSTR pszFileName;               /**< ファイル名 */
	int MaxFileName;                  /**< ファイル名の最大長 */
	RecordStartTime StartTimeSpec;    /**< 開始時間の指定方法(RECORD_START_*) */
	FILETIME StartTime;               /**< 指定された開始時刻(ローカル時刻) */
	                                  /**< StartRecordInfo::StartTimeSpec != TVTest::RECORD_START_NOTSPECIFIED の場合のみ有効 */
	RecordStopTime StopTimeSpec;      /**< 停止時間の指定方法(RECORD_STOP_*) */
	union {
		FILETIME Time;                /**< 停止時刻(ローカル時刻) (StartRecordInfo::StopTimeSpec == TVTest::RECORD_STOP_TIME) */
		ULONGLONG Duration;           /**< 録画停止までの時間(開始時刻からのミリ秒) (StartRecordInfo::StopTimeSpec == TVTest::RECORD_STOP_DURATION) */
	} StopTime;                       /**< 停止時間 */
};

#endif // TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 10)

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 11)

/**
@brief ウィンドウメッセージコールバック関数
*/
typedef BOOL (CALLBACK *WindowMessageCallbackFunc)(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult, void *pUserData);

/**
@brief ウィンドウメッセージコールバックを設定

メッセージコールバック関数を登録すると、TVTest のメインウィンドウにメッセージが送られた時に呼び出されます。
コールバック関数では、TVTest にメッセージの処理をさせない時は TRUE を返します。
その際、pResult に書き込んだ値が返されます。

@param[in] pParam プラグインパラメータ
@param[in] Callback コールバック関数。nullptr を渡すと設定が解除されます。
@param[in] pClientData コールバック関数の呼び出し時に渡されるデータ。

@retval true 正常終了
@retval false エラー発生

@note 一つのプラグインで設定できるコールバック関数は一つだけです。
      2回目以降の呼び出しでは、前回の設定が上書きされます。

@since ver.0.0.11

@par Corresponding message
TVTest::MESSAGE_SETWINDOWMESSAGECALLBACK
*/
inline bool MsgSetWindowMessageCallback(PluginParam *pParam, WindowMessageCallbackFunc Callback, void *pClientData = nullptr)
{
	return (*pParam->Callback)(pParam, MESSAGE_SETWINDOWMESSAGECALLBACK, reinterpret_cast<LPARAM>(Callback), reinterpret_cast<LPARAM>(pClientData)) != 0;
}

/**
@brief コントローラのフラグ
*/
TVTEST_DEFINE_ENUM(ControllerFlag, DWORD) {
	CONTROLLER_FLAG_NONE       = 0x00000000UL, /**< なし(0) */
	CONTROLLER_FLAG_ACTIVEONLY = 0x00000001UL  /**< アクティブ時のみ使用できる */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(ControllerFlag)

/**
@brief コントローラのボタンの情報
*/
struct ControllerButtonInfo
{
	LPCWSTR pszName;           /**< ボタンの名称("音声切替" など) */
	LPCWSTR pszDefaultCommand; /**< デフォルトのコマンド(MsgDoCommand と同じもの)。指定しない場合は nullptr */
	struct {
		WORD Left;             /**< 画像のボタンの左位置(画像が無い場合は無視される) */
		WORD Top;              /**< 画像のボタンの上位置(画像が無い場合は無視される) */
		WORD Width;            /**< 画像のボタンの幅(画像が無い場合は無視される) */
		WORD Height;           /**< 画像のボタンの高さ(画像が無い場合は無視される) */
	} ButtonRect;              /**< 画像のボタンの位置 */
	struct {
		WORD Left;             /**< 画像の選択ボタンの左位置(画像が無い場合は無視される) */
		WORD Top;              /**< 画像の選択ボタンの上位置(画像が無い場合は無視される) */
	} SelButtonPos;            /**< 画像の選択ボタンの位置 */
	DWORD Reserved;            /**< 予約領域(0にしてください) */
};

/**
@brief コントローラの情報
*/
struct ControllerInfo
{
	DWORD Size;                                 /**< 構造体のサイズ */
	ControllerFlag Flags;                       /**< 各種フラグ(CONTROLLER_FLAG_*) */
	LPCWSTR pszName;                            /**< コントローラ識別名 */
	LPCWSTR pszText;                            /**< コントローラの名称("○○リモコン" など) */
	int NumButtons;                             /**< ボタンの数 */
	const ControllerButtonInfo *pButtonList;    /**< ボタンのリスト */
	LPCWSTR pszIniFileName;                     /**< 設定ファイル名(nullptr にすると TVTest の Ini ファイル) */
	LPCWSTR pszSectionName;                     /**< 設定のセクション名 */
	UINT ControllerImageID;                     /**< コントローラの画像の識別子(無い場合は 0) */
	UINT SelButtonsImageID;                     /**< 選択ボタン画像の識別子(無い場合は 0) */
	typedef BOOL (CALLBACK *TranslateMessageCallback)(HWND hwnd, MSG *pMessage, void *pClientData); /**< メッセージ変換コールバック関数 */
	TranslateMessageCallback pTranslateMessage; /**< メッセージの変換コールバック(必要無ければ nullptr) */
	void *pClientData;                          /**< コールバックに渡すパラメータ */
};

/**
@brief コントローラを登録する

コントローラの登録を行うと、設定ダイアログのリモコンのページで割り当てが設定できるようになります。
コントローラの画像を用意すると、設定の右側に表示されます。
ボタンが押された時に MsgOnControllerButtonDown() を呼び出して通知すると、
割り当ての設定に従って機能が実行されます。

@param[in] pParam プラグインパラメータ
@param[in] pInfo 登録するコントローラの情報を格納した ControllerInfo 構造体へのポインタ

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.11

@par Corresponding message
TVTest::MESSAGE_REGISTERCONTROLLER
*/
inline bool MsgRegisterController(PluginParam *pParam, const ControllerInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_REGISTERCONTROLLER, reinterpret_cast<LPARAM>(pInfo), 0) != 0;
}

/**
@brief コントローラのボタンが押されたことを通知する

@param[in] pParam プラグインパラメータ
@param[in] pszName コントローラ識別名
@param[in] Button 押されたボタンのインデックス

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.11

@par Corresponding message
TVTest::MESSAGE_ONCONTROLLERBUTTONDOWN
*/
inline bool MsgOnControllerButtonDown(PluginParam *pParam, LPCWSTR pszName, int Button)
{
	return (*pParam->Callback)(pParam, MESSAGE_ONCONTROLLERBUTTONDOWN, reinterpret_cast<LPARAM>(pszName), Button) != 0;
}

/**
@brief コントローラの設定マスク
*/
TVTEST_DEFINE_ENUM(ControllerSettingsMask, DWORD) {
	CONTROLLER_SETTINGS_MASK_NONE  = 0x00000000UL, /**< なし(0) */
	CONTROLLER_SETTINGS_MASK_FLAGS = 0x00000001UL  /**< ControllerSettings::Flags が有効 */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(ControllerSettingsMask)

/**
@brief コントローラの設定フラグ
*/
TVTEST_DEFINE_ENUM(ControllerSettingsFlag, DWORD) {
	CONTROLLER_SETTINGS_FLAG_NONE       = 0x00000000UL, /**< なし(0) */
	CONTROLLER_SETTINGS_FLAG_ACTIVEONLY = 0x00000001UL  /**< アクティブ時のみ */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(ControllerSettingsFlag)

/**
@brief コントローラの設定
*/
struct ControllerSettings
{
	ControllerSettingsMask Mask;  /**< 取得する項目(CONTROLLER_SETTINGS_MASK_*) */
	ControllerSettingsFlag Flags; /**< 各種フラグ(CONTROLLER_SETTINGS_FLAG_*) */
};

/**
@brief コントローラの設定を取得する

@param[in] pParam プラグインパラメータ
@param[in] pszName コントローラ識別名
@param[in,out] pSettings 設定を取得する ControllerSettings 構造体へのポインタ。
                         事前に ControllerSettings::Mask メンバに取得する項目のフラグを設定して呼び出します。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.11

@par Corresponding message
TVTest::MESSAGE_GETCONTROLLERSETTINGS
*/
inline bool MsgGetControllerSettings(PluginParam *pParam, LPCWSTR pszName, ControllerSettings *pSettings)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETCONTROLLERSETTINGS, reinterpret_cast<LPARAM>(pszName), reinterpret_cast<LPARAM>(pSettings)) != 0;
}

/**
@brief コントローラがアクティブ時のみに設定されているか取得する

@param[in] pParam プラグインパラメータ
@param[in] pszName コントローラ識別名

@retval true アクティブ時のみ
@retval false アクティブ時のみではない

@note この関数は MsgGetControllerSettings() を呼び出します。

@since ver.0.0.11
*/
inline bool MsgIsControllerActiveOnly(PluginParam *pParam, LPCWSTR pszName)
{
	ControllerSettings Settings;
	Settings.Mask = CONTROLLER_SETTINGS_MASK_FLAGS;
	if (!MsgGetControllerSettings(pParam, pszName, &Settings))
		return false;
	return (Settings.Flags & CONTROLLER_SETTINGS_FLAG_ACTIVEONLY) != 0;
}

#endif // TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 11)

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 12)

/**
@brief 番組情報の取得方法
*/
TVTEST_DEFINE_ENUM(EpgEventQueryType, BYTE) {
	EPG_EVENT_QUERY_EVENTID, /**< イベントID */
	EPG_EVENT_QUERY_TIME     /**< 日時 */
};

/**
@brief 番組情報の取得フラグ
*/
TVTEST_DEFINE_ENUM(EpgEventQueryFlag, BYTE) {
	EPG_EVENT_QUERY_FLAG_NONE = 0x00 /**< なし */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(EpgEventQueryFlag)

/**
@brief 番組情報の取得のための情報
*/
struct EpgEventQueryInfo
{
	WORD NetworkID;          /**< ネットワークID */
	WORD TransportStreamID;  /**< ストリームID */
	WORD ServiceID;          /**< サービスID */
	EpgEventQueryType Type;  /**< 取得方法(EPG_EVENT_QUERY_*) */
	EpgEventQueryFlag Flags; /**< フラグ(現在は常に TVTest::EPG_EVENT_QUERY_FLAG_NONE) */
	union {
		WORD EventID;        /**< イベントID */
		FILETIME Time;       /**< 日時(UTC) */
	};
};

/**
@brief EPG の映像の情報
*/
struct EpgEventVideoInfo
{
	BYTE StreamContent; /**< stream_content */
	BYTE ComponentType; /**< component_type */
	                    /**< (0x01 = 480i[4:3] / 0x03 = 480i[16:9] / 0xB1 = 1080i[4:3] / 0xB3 = 1080i[16:9]) */
	BYTE ComponentTag;  /**< component_tag */
	BYTE Reserved;      /**< 予約 */
	DWORD LanguageCode; /**< 言語コード */
	LPCWSTR pszText;    /**< テキスト(無い場合は nullptr) */
};

/**
@brief EPG の音声のフラグ
*/
TVTEST_DEFINE_ENUM(EpgEventAudioFlag, BYTE) {
	EPG_EVENT_AUDIO_FLAG_NONE          = 0x00, /**< なし(0) */
	EPG_EVENT_AUDIO_FLAG_MULTILINGUAL  = 0x01, /**< 二ヶ国語 */
	EPG_EVENT_AUDIO_FLAG_MAINCOMPONENT = 0x02  /**< 主音声 */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(EpgEventAudioFlag)

/**
@brief EPG の音声の情報
*/
struct EpgEventAudioInfo
{
	EpgEventAudioFlag Flags; /**< フラグ(EPG_EVENT_AUDIO_FLAG_*) */
	BYTE StreamContent;      /**< stream_content */
	BYTE ComponentType;      /**< component_type (1 = Mono / 2 = Dual Mono / 3 = Stereo / 9 = 5.1ch) */
	BYTE ComponentTag;       /**< component_tag */
	BYTE SimulcastGroupTag;  /**< simulcast_group_tag */
	BYTE QualityIndicator;   /**< quality_indicator */
	BYTE SamplingRate;       /**< サンプリング周波数の種類 */
	BYTE Reserved;           /**< 予約 */
	DWORD LanguageCode;      /**< 言語コード(主音声) */
	DWORD LanguageCode2;     /**< 言語コード(副音声) */
	LPCWSTR pszText;         /**< テキスト(無い場合は nullptr) */
};

/**
@brief ジャンルの情報

(意味は STD-B10 第2部 付録H 等参照)
*/
struct EpgEventContentInfo
{
	BYTE ContentNibbleLevel1; /**< 大分類 */
	BYTE ContentNibbleLevel2; /**< 中分類 */
	BYTE UserNibble1;         /**< ユーザージャンル1 */
	BYTE UserNibble2;         /**< ユーザージャンル2 */
};

/**
@brief イベントグループのイベントの情報
*/
struct EpgGroupEventInfo
{
	WORD NetworkID;         /**< ネットワーク ID */
	WORD TransportStreamID; /**< ストリーム ID */
	WORD ServiceID;         /**< サービス ID */
	WORD EventID;           /**< イベント ID */
};

/**
@brief イベントグループの情報
*/
struct EpgEventGroupInfo
{
	BYTE GroupType;               /**< 種類 */
	BYTE EventListLength;         /**< イベントのリストの要素数 */
	BYTE Reserved[6];             /**< 予約 */
	EpgGroupEventInfo *EventList; /**< イベントのリスト */
};

/**
@brief 番組情報
*/
struct EpgEventInfo
{
	WORD EventID;                       /**< イベント ID */
	BYTE RunningStatus;                 /**< running_status */
	BYTE FreeCaMode;                    /**< free_CA_mode */
	DWORD Reserved;                     /**< 予約 */
	SYSTEMTIME StartTime;               /**< 開始日時(EPG 日時 : UTC+9) */
	DWORD Duration;                     /**< 長さ(秒単位) */
	BYTE VideoListLength;               /**< 映像の情報の数(VideoList の要素数) */
	BYTE AudioListLength;               /**< 音声の情報の数(AudioList の要素数) */
	BYTE ContentListLength;             /**< ジャンルの情報の数(ContentList の要素数) */
	BYTE EventGroupListLength;          /**< イベントグループの情報の数(EventGroupList の要素数) */
	LPCWSTR pszEventName;               /**< イベント名(無い場合は nullptr) */
	LPCWSTR pszEventText;               /**< テキスト(無い場合は nullptr) */
	LPCWSTR pszEventExtendedText;       /**< 拡張テキスト(無い場合は nullptr) */
	EpgEventVideoInfo **VideoList;      /**< 映像の情報のリスト(無い場合は nullptr) */
	EpgEventAudioInfo **AudioList;      /**< 音声の情報のリスト(無い場合は nullptr) */
	EpgEventContentInfo *ContentList;   /**< ジャンルの情報(無い場合は nullptr) */
	EpgEventGroupInfo **EventGroupList; /**< イベントグループ情報(無い場合は nullptr) */
};

/**
@brief 番組情報のリスト
*/
struct EpgEventList
{
	WORD NetworkID;           /**< ネットワークID */
	WORD TransportStreamID;   /**< ストリームID */
	WORD ServiceID;           /**< サービスID */
	WORD NumEvents;           /**< 番組情報の数 */
	EpgEventInfo **EventList; /**< リスト */
};

/**
@brief 番組情報を取得

@param[in] pParam プラグインパラメータ
@param[in] pInfo 取得したい番組情報を指定する EpgEventQueryInfo 構造体へのポインタ

@return 番組情報を表す EpgEventInfo 構造体のポインタ。
        情報が取得できなかった場合は nullptr。
        取得した情報が不要になった場合、 MsgFreeEpgEventInfo() で解放します。

@since ver.0.0.12

@par Corresponding message
TVTest::MESSAGE_GETEPGEVENTINFO

@par Example
@code{.cpp}
	// 現在のチャンネルの現在の番組を取得する例
	TVTest::ChannelInfo ChInfo;
	ChInfo.Size = sizeof(ChInfo);
	if (TVTest::MsgGetCurrentChannelInfo(pParam, &ChInfo)) {
		TVTest::EpgEventQueryInfo QueryInfo;
		QueryInfo.NetworkID         = ChInfo.NetworkID;
		QueryInfo.TransportStreamID = ChInfo.TransportStreamID;
		QueryInfo.ServiceID         = ChInfo.ServiceID;
		QueryInfo.Type              = TVTest::EPG_EVENT_QUERY_TIME;
		QueryInfo.Flags             = TVTest::EPG_EVENT_QUERY_FLAG_NONE;
		GetSystemTimeAsFileTime(&QueryInfo.Time);
		TVTest::EpgEventInfo *pEvent = TVTest::MsgGetEpgEventInfo(pParam, &QueryInfo);
		if (pEvent != nullptr) {
			...
			TVTest::MsgFreeEpgEventInfo(pParam, pEvent);
		}
	}
@endcode
*/
inline EpgEventInfo *MsgGetEpgEventInfo(PluginParam *pParam, const EpgEventQueryInfo *pInfo)
{
	return reinterpret_cast<EpgEventInfo*>((*pParam->Callback)(pParam, MESSAGE_GETEPGEVENTINFO, reinterpret_cast<LPARAM>(pInfo), 0));
}

/**
@brief 番組情報を解放

MsgGetEpgEventInfo() で取得した情報のメモリを解放します。

@param[in] pParam プラグインパラメータ
@param[in] pEventInfo 解放する EpgEventInfo 構造体へのポインタ

@since ver.0.0.12

@par Corresponding message
TVTest::MESSAGE_FREEEPGEVENTINFO
*/
inline void MsgFreeEpgEventInfo(PluginParam *pParam, EpgEventInfo *pEventInfo)
{
	(*pParam->Callback)(pParam, MESSAGE_FREEEPGEVENTINFO, reinterpret_cast<LPARAM>(pEventInfo), 0);
}

/**
@brief 番組情報のリストを取得

@param[in] pParam プラグインパラメータ
@param[in,out] pList 番組情報のリストを取得する EpgEventList 構造体へのポインタ。
                     EpgEventList::NetworkID / EpgEventList::TransportStreamID / EpgEventList::ServiceID メンバを設定して呼び出します。

@retval true 正常終了
@retval false エラー発生

@note 取得された番組は開始日時順にソートされています。
      リストが不要になった場合、 MsgFreeEpgEventList() で解放します。

@since ver.0.0.12

@par Corresponding message
TVTest::MESSAGE_GETEPGEVENTLIST

@par Example
@code{.cpp}
	// 現在のチャンネルの番組表を取得する
	TVTest::ChannelInfo ChInfo;
	ChInfo.Size = sizeof(ChInfo);
	if (TVTest::MsgGetCurrentChannelInfo(pParam, &ChInfo)) {
		TVTest::EpgEventList EventList;
		EventList.NetworkID         = ChInfo.NetworkID;
		EventList.TransportStreamID = ChInfo.TransportStreamID;
		EventList.ServiceID         = ChInfo.ServiceID;
		if (TVTest::MsgGetEpgEventList(pParam, &EventList)) {
			...
			TVTest::MsgFreeEpgEventList(pParam, &EventList);
		}
	}
@endcode
*/
inline bool MsgGetEpgEventList(PluginParam *pParam, EpgEventList *pList)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETEPGEVENTLIST, reinterpret_cast<LPARAM>(pList), 0) != 0;
}

/**
@brief 番組情報のリストを解放

MsgGetEpgEventList() で取得したリストのメモリを解放します。

@param[in] pParam プラグインパラメータ
@param[in,out] pList 解放する番組情報のリストを格納した EpgEventList 構造体へのポインタ

@since ver.0.0.12

@par Corresponding message
TVTest::MESSAGE_FREEEPGEVENTLIST
*/
inline void MsgFreeEpgEventList(PluginParam *pParam, EpgEventList *pList)
{
	(*pParam->Callback)(pParam, MESSAGE_FREEEPGEVENTLIST, reinterpret_cast<LPARAM>(pList), 0);
}

/**
@brief BonDriver を列挙する

BonDriver のフォルダとして設定されているフォルダ内の BonDriver を列挙します。

@param[in] pParam プラグインパラメータ
@param[in] Index インデックス
@param[out] pszFileName ファイル名を取得するバッファへのポインタ
@param[in] MaxLength pszFileName が差すバッファの長さ

@return ファイル名の長さ。Index が範囲外の場合は 0。

@note ファイル名の長さが MaxLength 以上である場合、pszFileName に格納される文字列は MaxLength - 1 の長さに切り詰められ、終端に空文字が付加されます。
      その際も戻り値は切り捨てられる前の本来の長さです。

@since ver.0.0.12

@par Corresponding message
TVTest::MESSAGE_ENUMDRIVER
*/
inline int MsgEnumDriver(PluginParam *pParam, int Index, LPWSTR pszFileName, int MaxLength)
{
	return static_cast<int>((*pParam->Callback)(pParam, MESSAGE_ENUMDRIVER, reinterpret_cast<LPARAM>(pszFileName), MAKELPARAM(Index, MaxLength > 0xFFFF ? 0xFFFF : MaxLength)));
}

/**
@brief チューニング空間の情報フラグ
*/
TVTEST_DEFINE_ENUM(DriverTuningSpaceInfoFlag, DWORD) {
	DRIVER_TUNING_SPACE_INFO_FLAG_NONE = 0x00000000U /**< なし(0) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(DriverTuningSpaceInfoFlag)

/**
@brief チューニング空間の情報
*/
struct DriverTuningSpaceInfo
{
	DriverTuningSpaceInfoFlag Flags; /**< フラグ(現在は常に TVTest::DRIVER_TUNING_SPACE_INFO_FLAG_NONE) */
	DWORD NumChannels;               /**< チャンネル数 */
	TuningSpaceInfo *pInfo;          /**< チューニング空間の情報 */
	ChannelInfo **ChannelList;       /**< チャンネルのリスト */
};

/**
@brief チューニング空間のリストフラグ
*/
TVTEST_DEFINE_ENUM(DriverTuningSpaceListFlag, DWORD) {
	DRIVER_TUNING_SPACE_LIST_FLAG_NONE = 0x00000000U /**< なし(0) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(DriverTuningSpaceListFlag)

/**
@brief チューニング空間のリスト
*/
struct DriverTuningSpaceList
{
	DriverTuningSpaceListFlag Flags;   /**< フラグ(現在は常に TVTest::DRIVER_TUNING_SPACE_LIST_FLAG_NONE) */
	DWORD NumSpaces;                   /**< チューニング空間の数 */
	DriverTuningSpaceInfo **SpaceList; /**< チューニング空間のリスト */
};

/**
@brief BonDriver のチャンネルのリストを取得する

@param[in] pParam プラグインパラメータ
@param[in] pszDriverName 情報を取得する BonDriver のファイル名。
@param[in,out] pList 情報を取得する DriverTuningSpaceList 構造体へのポインタ。
                     DriverTuningSpaceList::Flags メンバを設定して呼び出します。

@retval true 正常終了
@retval false エラー発生

@note リストが不要になった場合、 MsgFreeDriverTuningSpaceList() で解放します。

@since ver.0.0.12

@par Corresponding message
TVTest::MESSAGE_GETDRIVERTUNINGSPACELIST
*/
inline bool MsgGetDriverTuningSpaceList(PluginParam *pParam, LPCWSTR pszDriverName, DriverTuningSpaceList *pList)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETDRIVERTUNINGSPACELIST, reinterpret_cast<LPARAM>(pszDriverName), reinterpret_cast<LPARAM>(pList)) != 0;
}

/**
@brief BonDriver のチャンネルのリストを解放する

MsgGetDriverTuningSpaceList() で取得した情報を解放します。

@param[in] pParam プラグインパラメータ
@param[in,out] pList 解放する DriverTuningSpaceList 構造体へのポインタ。

@since ver.0.0.12

@par Corresponding message
TVTest::MESSAGE_FREEDRIVERTUNINGSPACELIST
*/
inline void MsgFreeDriverTuningSpaceList(PluginParam *pParam, DriverTuningSpaceList *pList)
{
	(*pParam->Callback)(pParam, MESSAGE_FREEDRIVERTUNINGSPACELIST, reinterpret_cast<LPARAM>(pList), 0);
}

#endif // TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 12)

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 13)

/**
@brief 番組表の番組の情報
*/
struct ProgramGuideProgramInfo
{
	WORD NetworkID;         /**< ネットワーク ID */
	WORD TransportStreamID; /**< ストリーム ID */
	WORD ServiceID;         /**< サービス ID */
	WORD EventID;           /**< イベント ID */
	SYSTEMTIME StartTime;   /**< 開始日時(EPG 日時 : UTC+9) */
	DWORD Duration;         /**< 長さ(秒単位) */
};

/**
@brief 番組表の番組の背景描画の情報
*/
struct ProgramGuideProgramDrawBackgroundInfo
{
	HDC hdc;                  /**< 描画先 DC ハンドル */
	RECT ItemRect;            /**< 項目全体の位置 */
	RECT TitleRect;           /**< タイトルの位置 */
	RECT ContentRect;         /**< 番組内容の位置 */
	COLORREF BackgroundColor; /**< 背景色 */
};

/**
@brief 番組表のメニューの情報
*/
struct ProgramGuideInitializeMenuInfo
{
	HMENU hmenu;   /**< メニューのハンドル */
	UINT Command;  /**< 項目の ID */
	UINT Reserved; /**< 予約 */
};

/**
@brief 番組表の番組のメニューの情報
*/
struct ProgramGuideProgramInitializeMenuInfo
{
	HMENU hmenu;     /**< メニューのハンドル */
	UINT Command;    /**< 項目の ID */
	UINT Reserved;   /**< 予約 */
	POINT CursorPos; /**< カーソル位置 */
	RECT ItemRect;   /**< 番組の位置 */
};

/**
@brief 番組表のイベントのフラグ
*/
TVTEST_DEFINE_ENUM(ProgramGuideEventFlag, UINT) {
	PROGRAMGUIDE_EVENT_NONE    = 0x0000, /**< なし(0) */
	PROGRAMGUIDE_EVENT_GENERAL = 0x0001, /**< 全体のイベント(EVENT_PROGRAMGUIDE_*)を有効にする */
	PROGRAMGUIDE_EVENT_PROGRAM = 0x0002  /**< 各番組のイベント(EVENT_PROGRAMGUIDE_PROGRAM_*)を有効にする */
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 15)
	/**
	@brief コマンドのイベント(TVTest::EVENT_PROGRAMGUIDE_COMMAND)を常に有効にする
	@details TVTest::EVENT_PROGRAMGUIDE_COMMAND は PROGRAMGUIDE_EVENT_GENERAL を指定した場合にも送られますが、
	         このフラグを指定しなければプラグインが無効状態の時は送られません。
	@since ver.0.0.15
	*/
	, PROGRAMGUIDE_EVENT_COMMAND_ALWAYS = 0x0004
#endif
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(ProgramGuideEventFlag)

/**
@brief 番組表のイベントの有効/無効を設定する

番組表のイベント(EVENT_PROGRAMGUIDE_*)の通知が必要な場合、通知を有効に設定します。

@param[in] pParam プラグインパラメータ
@param[in] EventFlags 有効にしたい通知を表すフラグを指定します。
                      PROGRAMGUIDE_EVENT_* の組み合わせです。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.13

@par Corresponding message
TVTest::MESSAGE_ENABLEPROGRAMGUIDEEVENT
*/
inline bool MsgEnableProgramGuideEvent(PluginParam *pParam, ProgramGuideEventFlag EventFlags)
{
	return (*pParam->Callback)(pParam, MESSAGE_ENABLEPROGRAMGUIDEEVENT, static_cast<LPARAM>(EventFlags), 0) != 0;
}

/**
@brief 番組表のコマンドの種類
*/
TVTEST_DEFINE_ENUM(ProgramGuideCommandType, WORD) {
	PROGRAMGUIDE_COMMAND_TYPE_PROGRAM = 1  /**< 各番組 */
};

/**
@brief 番組表コマンドのフラグ
*/
TVTEST_DEFINE_ENUM(ProgramGuideCommandFlag, WORD) {
	PROGRAMGUIDE_COMMAND_FLAG_NONE = 0x0000 /**< なし(0) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(ProgramGuideCommandFlag)

/**
@brief 番組表のコマンドの情報
*/
struct ProgramGuideCommandInfo
{
	ProgramGuideCommandType Type;  /**< 種類(PROGRAMGUiDE_COMMAND_TYPE_*) */
	ProgramGuideCommandFlag Flags; /**< 各種フラグ(現在は常に TVTest::PROGRAMGUIDE_COMMAND_FLAG_NONE) */
	UINT ID;                       /**< 識別子 */
	LPCWSTR pszText;               /**< コマンドの文字列 */
	LPCWSTR pszName;               /**< コマンドの名前 */
};

/**
@brief 番組表のコマンド実行の操作の種類
*/
TVTEST_DEFINE_ENUM(ProgramGuideCommandAction, UINT) {
	PROGRAMGUIDE_COMMAND_ACTION_MOUSE, /**< マウスなど */
	PROGRAMGUIDE_COMMAND_ACTION_KEY    /**< キーボード */
};

/**
@brief 番組表のコマンド実行時の情報
*/
struct ProgramGuideCommandParam
{
	UINT ID;                          /**< 識別子 */
	ProgramGuideCommandAction Action; /**< 操作の種類(PROGRAMGUIDE_COMMAND_ACTION_*) */
	ProgramGuideProgramInfo Program;  /**< 番組の情報 */
	POINT CursorPos;                  /**< カーソル位置 */
	RECT ItemRect;                    /**< 項目の位置 */
};

/**
@brief 番組表のコマンドを登録する

コマンドを登録すると、番組表のダブルクリックなどに機能を割り当てることができるようになります。
コマンドが実行されると TVTest::EVENT_PROGRAMGUIDE_COMMAND イベントが送られます。

@param[in] pParam プラグインパラメータ
@param[in] pCommandList コマンドの情報を表す ProgramGuideCommandInfo 構造体の配列へのポインタ
@param[in] NumCommands pCommandList の差す配列の要素数

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.13

@par Corresponding message
TVTest::MESSAGE_REGISTERPROGRAMGUIDECOMMAND
*/
inline bool MsgRegisterProgramGuideCommand(
	PluginParam *pParam,
	const ProgramGuideCommandInfo *pCommandList, int NumCommands = 1)
{
	return (*pParam->Callback)(pParam, MESSAGE_REGISTERPROGRAMGUIDECOMMAND, reinterpret_cast<LPARAM>(pCommandList), NumCommands) != 0;
}

#endif // TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 13)

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 14)

/**
@brief フィルタグラフの情報フラグ
*/
TVTEST_DEFINE_ENUM(FilterGraphInfoFlag, DWORD) {
	FILTER_GRAPH_INFO_FLAG_NONE = 0x00000000U /**< なし(0) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(FilterGraphInfoFlag)

/**
@brief フィルタグラフの情報

フィルタグラフ関係のイベント(EVENT_FILTERGRAPH_*)で渡されます。
*/
struct FilterGraphInfo
{
	FilterGraphInfoFlag Flags;                         /**< 各種フラグ(現在は常に TVTest::FILTER_GRAPH_INFO_FLAG_NONE) */
	BYTE VideoStreamType;                              /**< 映像 stream_type */
	BYTE Reserved[3];                                  /**< 予約 */
	TVTEST_COM_INTERFACE IGraphBuilder *pGraphBuilder; /**< IGraphBuilder */
};

/**
@brief スタイル値の単位
*/
TVTEST_DEFINE_ENUM(StyleUnit, int) {
	STYLE_UNIT_UNDEFINED,      /**< 未定義 */
	STYLE_UNIT_LOGICAL_PIXEL,  /**< 論理ピクセル(96 DPI におけるピクセル単位) */
	STYLE_UNIT_PHYSICAL_PIXEL, /**< 物理ピクセル */
	STYLE_UNIT_POINT,          /**< ポイント(1/72 インチ) */
	STYLE_UNIT_DIP             /**< dip(1/160 インチ) */
};

/**
@brief スタイル値のフラグ
*/
TVTEST_DEFINE_ENUM(StyleFlag, DWORD) {
	STYLE_FLAG_NONE = 0x00000000U /**< なし(0) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(StyleFlag)

/**
@brief スタイル値の情報
*/
struct StyleValueInfo
{
	DWORD Size;      /**< 構造体のサイズ */
	StyleFlag Flags; /**< 各種フラグ(現在は常に TVTest::STYLE_FLAG_NONE) */
	LPCWSTR pszName; /**< スタイル名 */
	StyleUnit Unit;  /**< 取得する値の単位(STYLE_UNIT_*) */
	int DPI;         /**< DPI の指定 */
	int Value;       /**< 取得された値 */
};

/**
@brief スタイル値を取得する

TVTest.style.ini で設定されたスタイル値を取得します。
通常は単位ごとにオーバーロードされた関数を利用する方が簡単です。

@param[in] pParam プラグインパラメータ
@param[in,out] pInfo 情報を取得する StyleValueInfo 構造体へのポインタ。
                     StyleValueInfo::Value 以外のメンバを設定して呼び出し、 StyleValueInfo::Value に値が返ります。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GETSTYLEVALUE
*/
inline bool MsgGetStyleValue(PluginParam *pParam, StyleValueInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETSTYLEVALUE, reinterpret_cast<LPARAM>(pInfo), 0) != FALSE;
}

/**
@brief 指定された単位のスタイル値を取得する

@param[in] pParam プラグインパラメータ
@param[in] pszName スタイル名
@param[in] Unit 取得する値の単位(STYLE_UNIT_*)
@param[in] DPI DPI の指定
@param[out] pValue 取得した値を返す変数へのポインタ

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GETSTYLEVALUE
*/
inline bool MsgGetStyleValue(PluginParam *pParam, LPCWSTR pszName, StyleUnit Unit, int DPI, int *pValue)
{
	StyleValueInfo Info;
	Info.Size = sizeof(Info);
	Info.Flags = STYLE_FLAG_NONE;
	Info.pszName = pszName;
	Info.Unit = Unit;
	Info.DPI = DPI;
	if (!MsgGetStyleValue(pParam, &Info))
		return false;
	*pValue = Info.Value;
	return true;
}

/**
@brief オリジナルの単位のスタイル値を取得する

@param[in] pParam プラグインパラメータ
@param[in] pszName スタイル名
@param[out] pValue 取得した値を返す変数へのポインタ

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GETSTYLEVALUE
*/
inline bool MsgGetStyleValue(PluginParam *pParam, LPCWSTR pszName, int *pValue)
{
	return MsgGetStyleValue(pParam, pszName, STYLE_UNIT_UNDEFINED, 0, pValue);
}

/**
@brief 物理ピクセル単位のスタイル値を取得する

@param[in] pParam プラグインパラメータ
@param[in] pszName スタイル名
@param[in] DPI DPI の指定
@param[out] pValue 取得した値を返す変数へのポインタ

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GETSTYLEVALUE
*/
inline bool MsgGetStyleValuePixels(PluginParam *pParam, LPCWSTR pszName, int DPI, int *pValue)
{
	return MsgGetStyleValue(pParam, pszName, STYLE_UNIT_PHYSICAL_PIXEL, DPI, pValue);
}

/**
@brief テーマ描画フラグ
*/
TVTEST_DEFINE_ENUM(ThemeDrawBackgroundFlag, DWORD) {
	THEME_DRAW_BACKGROUND_FLAG_NONE       = 0x00000000U, /**< なし(0) */
	THEME_DRAW_BACKGROUND_FLAG_ADJUSTRECT = 0x00000001U  /**< クライアント領域を取得 */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(ThemeDrawBackgroundFlag)

/**
@brief テーマの背景描画情報
*/
struct ThemeDrawBackgroundInfo
{
	DWORD Size;                    /**< 構造体のサイズ */
	ThemeDrawBackgroundFlag Flags; /**< 各種フラグ(THEME_DRAW_BACKGROUND_FLAG_*) */
	LPCWSTR pszStyle;              /**< スタイル名 */
	HDC hdc;                       /**< 描画先 DC */
	RECT DrawRect;                 /**< 描画領域 */
	int DPI;                       /**< DPI の指定(0 でメインウィンドウと同じ) */
};

/**
@brief テーマの背景を描画する

@param[in] pParam プラグインパラメータ
@param[in,out] pInfo テーマの背景描画情報を格納した ThemeDrawBackgroundInfo 構造体へのポインタ

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_THEMEDRAWBACKGROUND
*/
inline bool MsgThemeDrawBackground(PluginParam *pParam, ThemeDrawBackgroundInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_THEMEDRAWBACKGROUND, reinterpret_cast<LPARAM>(pInfo), 0) != FALSE;
}

/**
@brief テーマの背景を描画する

@param[in] pParam プラグインパラメータ
@param[in] pszStyle スタイル名
@param[in] hdc 描画先 DC
@param[in] DrawRect 描画範囲
@param[in] DPI DPI の指定(0 でメインウィンドウと同じ)

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_THEMEDRAWBACKGROUND
*/
inline bool MsgThemeDrawBackground(PluginParam *pParam, LPCWSTR pszStyle, HDC hdc, const RECT &DrawRect, int DPI = 0)
{
	ThemeDrawBackgroundInfo Info;
	Info.Size = sizeof(Info);
	Info.Flags = THEME_DRAW_BACKGROUND_FLAG_NONE;
	Info.pszStyle = pszStyle;
	Info.hdc = hdc;
	Info.DrawRect = DrawRect;
	Info.DPI = DPI;
	return MsgThemeDrawBackground(pParam, &Info);
}

/**
@brief テーマ文字列描画のフラグ
*/
TVTEST_DEFINE_ENUM(ThemeDrawTextFlag, DWORD) {
	THEME_DRAW_TEXT_FLAG_NONE = 0x00000000U /**< なし(0) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(ThemeDrawTextFlag)

/**
@brief テーマの文字列描画情報
*/
struct ThemeDrawTextInfo
{
	DWORD Size;              /**< 構造体のサイズ */
	ThemeDrawTextFlag Flags; /**< 各種フラグ(現在は常に TVTest::THEME_DRAW_TEXT_FLAG_NONE) */
	LPCWSTR pszStyle;        /**< スタイル名 */
	HDC hdc;                 /**< 描画先 DC */
	LPCWSTR pszText;         /**< 描画する文字列 */
	RECT DrawRect;           /**< 描画先の領域 */
	UINT DrawFlags;          /**< 描画フラグ(DrawText() API の DT_*) */
	COLORREF Color;          /**< 描画する色(CLR_INVALID でデフォルトの色) */
};

/**
@brief テーマの文字列を描画する

@param[in] pParam プラグインパラメータ
@param[in] pInfo テーマの文字列描画情報を格納した ThemeDrawTextInfo 構造体へのポインタ

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_THEMEDRAWTEXT
*/
inline bool MsgThemeDrawText(PluginParam *pParam, ThemeDrawTextInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_THEMEDRAWTEXT, reinterpret_cast<LPARAM>(pInfo), 0) != FALSE;
}

/**
@brief テーマの文字列を描画する

@param[in] pParam プラグインパラメータ
@param[in] pszStyle スタイル名
@param[in] hdc 描画先 DC
@param[in] pszText 描画する文字列
@param[in] DrawRect 描画範囲
@param[in] DrawFlags 描画フラグ(DrawText() API の DT_*)
@param[in] Color 描画する色(CLR_INVALID でデフォルトの色)

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_THEMEDRAWTEXT
*/
inline bool MsgThemeDrawText(
	PluginParam *pParam,
	LPCWSTR pszStyle, HDC hdc, LPCWSTR pszText, const RECT &DrawRect,
	UINT DrawFlags, COLORREF Color = CLR_INVALID)
{
	ThemeDrawTextInfo Info;
	Info.Size = sizeof(Info);
	Info.Flags = THEME_DRAW_TEXT_FLAG_NONE;
	Info.pszStyle = pszStyle;
	Info.hdc = hdc;
	Info.pszText = pszText;
	Info.DrawRect = DrawRect;
	Info.DrawFlags = DrawFlags;
	Info.Color = Color;
	return MsgThemeDrawText(pParam, &Info);
}

/**
@brief テーマのアイコン描画のフラグ
*/
TVTEST_DEFINE_ENUM(ThemeDrawIconFlag, DWORD) {
	THEME_DRAW_ICON_FLAG_NONE = 0x00000000U /**< なし(0) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(ThemeDrawIconFlag)

/**
@brief テーマのアイコン描画情報
*/
struct ThemeDrawIconInfo
{
	DWORD Size;              /**< 構造体のサイズ */
	ThemeDrawIconFlag Flags; /**< 各種フラグ(現在は常に TVTest::THEME_DRAW_ICON_FLAG_NONE) */
	LPCWSTR pszStyle;        /**< スタイル名 */
	HDC hdc;                 /**< 描画先 DC */
	HBITMAP hbm;             /**< 描画するビットマップ */
	RECT DstRect;            /**< 描画先の領域 */
	RECT SrcRect;            /**< 描画元の領域 */
	COLORREF Color;          /**< 描画する色(CLR_INVALID でデフォルトの色) */
	BYTE Opacity;            /**< 不透明度(1-255) */
	BYTE Reserved[3];        /**< 予約領域 */
};

/**
@brief テーマのアイコンを描画する

@param[in] pParam プラグインパラメータ
@param[in] pInfo テーマのアイコン描画情報を格納した ThemeDrawIconInfo 構造体へのポインタ

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_THEMEDRAWICON
*/
inline bool MsgThemeDrawIcon(PluginParam *pParam, ThemeDrawIconInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_THEMEDRAWICON, reinterpret_cast<LPARAM>(pInfo), 0) != FALSE;
}

/**
@brief テーマのアイコンを描画する

@param[in] pParam プラグインパラメータ
@param[in] pszStyle スタイル名
@param[in] hdc 描画先 DC
@param[in] DstX 描画先の左位置
@param[in] DstY 描画先の上位置
@param[in] DstWidth 描画先の幅
@param[in] DstHeight 描画先の高さ
@param[in] hbm 描画するビットマップ
@param[in] SrcX 描画元の左位置
@param[in] SrcY 描画元の上位置
@param[in] SrcWidth 描画元の幅
@param[in] SrcHeight 描画元の高さ
@param[in] Color 描画する色(CLR_INVALID でデフォルトの色)
@param[in] Opacity 不透明度(1-255)。デフォルトは 255

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_THEMEDRAWICON
*/
inline bool MsgThemeDrawIcon(
	PluginParam *pParam, LPCWSTR pszStyle,
	HDC hdc, int DstX, int DstY, int DstWidth, int DstHeight,
	HBITMAP hbm, int SrcX, int SrcY, int SrcWidth, int SrcHeight,
	COLORREF Color = CLR_INVALID, BYTE Opacity = 255)
{
	ThemeDrawIconInfo Info;
	Info.Size = sizeof(Info);
	Info.Flags = THEME_DRAW_ICON_FLAG_NONE;
	Info.pszStyle = pszStyle;
	Info.hdc = hdc;
	Info.hbm = hbm;
	Info.DstRect.left = DstX;
	Info.DstRect.top = DstY;
	Info.DstRect.right = DstX + DstWidth;
	Info.DstRect.bottom = DstY + DstHeight;
	Info.SrcRect.left = SrcX;
	Info.SrcRect.top = SrcY;
	Info.SrcRect.right = SrcX + SrcWidth;
	Info.SrcRect.bottom = SrcY + SrcHeight;
	Info.Color = Color;
	Info.Opacity = Opacity;
	return MsgThemeDrawIcon(pParam, &Info);
}

/**
@brief EPG 取得状況のステータス
*/
TVTEST_DEFINE_ENUM(EpgCaptureStatus, DWORD) {
	EPG_CAPTURE_STATUS_NONE                      = 0x00000000U, /**< なし(0) */
	EPG_CAPTURE_STATUS_SCHEDULEBASICCOMPLETED    = 0x00000001U, /**< schedule basic が揃っている */
	EPG_CAPTURE_STATUS_SCHEDULEEXTENDEDCOMPLETED = 0x00000002U, /**< schedule extended が揃っている */
	EPG_CAPTURE_STATUS_HASSCHEDULEBASIC          = 0x00000004U, /**< schedule basic が存在する */
	EPG_CAPTURE_STATUS_HASSCHEDULEEXTENDED       = 0x00000008U  /**< schedule extended が存在する */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(EpgCaptureStatus)

/**
@brief EPG 取得状況のフラグ
*/
TVTEST_DEFINE_ENUM(EpgCaptureStatusFlag, WORD) {
	EPG_CAPTURE_STATUS_FLAG_NONE = 0x0000 /**< なし(0) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(EpgCaptureStatusFlag)

/**
@brief EPG 取得状況の情報
*/
struct EpgCaptureStatusInfo
{
	DWORD Size;                 /**< 構造体のサイズ */
	EpgCaptureStatusFlag Flags; /**< 各種フラグ(現在は常に TVTest::EPG_CAPTURE_STATUS_FLAG_NONE) */
	WORD NetworkID;             /**< ネットワーク ID */
	WORD TransportStreamID;     /**< ストリーム ID */
	WORD ServiceID;             /**< サービス ID */
	EpgCaptureStatus Status;    /**< ステータス(EPG_CAPTURE_STATUS_*) */
};

/**
@brief EPG 取得状況を取得する

@param[in] pParam プラグインパラメータ
@param[in,out] pInfo EPG 取得状態の情報を格納する EpgCaptureStatusInfo 構造体へのポインタ。
                     事前に各メンバを設定して呼び出します。
                     EpgCaptureStatusInfo::Status メンバには取得したい情報のフラグ(EPG_CAPCTURE_STATUS_*)を指定して呼び出し、
                     そのフラグの中で現在の取得状況が返されます。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GETEPGCAPTURESTATUS

@par Example
@code{.cpp}
	TVTest::EpgCaptureStatusInfo Info;
	Info.Size = sizeof(Info);
	Info.Flags = TVTest::EPG_CAPTURE_STATUS_FLAG_NONE;
	// 取得したいサービスを指定する
	Info.NetworkID = NetworkID;
	Info.TransportStreamID = TransportStreamID;
	Info.ServiceID = ServiceID;
	// Info.Status に取得したいステータスを指定する
	Info.Status = TVTest::EPG_CAPTURE_STATUS_SCHEDULEBASICCOMPLETED |
	              TVTest::EPG_CAPTURE_STATUS_SCHEDULEEXTENDEDCOMPLETED;
	if (TVTest::MsgGetEpgCaptureStatus(pParam, &Info)) {
		if (Info.Status & TVTest::EPG_CAPTURE_STATUS_SCHEDULEBASICCOMPLETED)
			std::printf("schedule basic 揃った\n");
		if (Info.Status & TVTest::EPG_CAPTURE_STATUS_SCHEDULEEXTENDEDCOMPLETED)
			std::printf("schedule extended 揃った\n");
	}
@endcode
*/
inline bool MsgGetEpgCaptureStatus(PluginParam *pParam, EpgCaptureStatusInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETEPGCAPTURESTATUS, reinterpret_cast<LPARAM>(pInfo), 0) != FALSE;
}

/**
@brief コマンドの情報
*/
struct AppCommandInfo
{
	DWORD Size;     /**< 構造体のサイズ */
	DWORD Index;    /**< 取得するコマンドのインデックス */
	LPWSTR pszText; /**< コマンドの文字列 */
	int MaxText;    /**< コマンドの文字列のバッファ長 */
	LPWSTR pszName; /**< コマンドの名前 */
	int MaxName;    /**< コマンドの名前のバッファ長 */
};

/**
@brief コマンドの情報を取得する

@param[in] pParam プラグインパラメータ
@param[in,out] pInfo コマンドの情報を取得する AppCommandInfo 構造体へのポインタ。
                     事前に各メンバを設定して呼び出します。
                     AppCommandInfo::pszText に nullptr を指定すると、必要なバッファ長(終端の空文字を含む)が AppCommandInfo::MaxText に返ります。
                     AppCommandInfo::pszName に nullptr を指定すると、必要なバッファ長(終端の空文字を含む)が AppCommandInfo::MaxName に返ります。

@retval true 正常終了
@retval false エラー発生

@note TVTInitialize() 内で呼ぶと、プラグインのコマンドなどが取得できませんので注意してください。
@note AppCommandInfo::pszText に取得されたコマンド文字列を MsgDoCommand() に指定して実行できます。

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GETAPPCOMMANDINFO

@par Example
@code{.cpp}
	const DWORD Count = TVTest::MsgGetAppCommandCount(pParam);
	for (DWORD i = 0; i < Count; i++) {
		TVTest::AppCommandInfo Info;
		WCHAR szText[256], szName[256];
		Info.Size = sizeof(Info);
		Info.Index = i;
		Info.pszText = szText;
		Info.MaxText = _countof(szText);
		Info.pszName = szName;
		Info.MaxName = _countof(szName);
		if (TVTest::MsgGetAppCommandInfo(pParam, &Info)) {
			std::wprintf(L"Command %u %ls %ls\n", i, szText, szName);
		}
	}
@endcode
*/
inline bool MsgGetAppCommandInfo(PluginParam *pParam, AppCommandInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETAPPCOMMANDINFO, reinterpret_cast<LPARAM>(pInfo), 0) != FALSE;
}

/**
@brief コマンドの数を取得する

@param[in] pParam プラグインパラメータ

@return コマンドの数

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GETAPPCOMMANDCOUNT
*/
inline DWORD MsgGetAppCommandCount(PluginParam *pParam)
{
	return static_cast<DWORD>((*pParam->Callback)(pParam, MESSAGE_GETAPPCOMMANDCOUNT, 0, 0));
}

/**
@brief 映像ストリームの数を取得する

@param[in] pParam プラグインパラメータ

@return 映像ストリームの数

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GETVIDEOSTREAMCOUNT
*/
inline int MsgGetVideoStreamCount(PluginParam *pParam)
{
	return static_cast<int>((*pParam->Callback)(pParam, MESSAGE_GETVIDEOSTREAMCOUNT, 0, 0));
}

/**
@brief 現在の映像ストリームを取得する

@param[in] pParam プラグインパラメータ

@return 現在の映像ストリームのインデックス

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GETVIDEOSTREAM
*/
inline int MsgGetVideoStream(PluginParam *pParam)
{
	return static_cast<int>((*pParam->Callback)(pParam, MESSAGE_GETVIDEOSTREAM, 0, 0));
}

/**
@brief 映像ストリームを設定する

@param[in] pParam プラグインパラメータ
@param[in] Stream 設定する映像ストリームのインデックス

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_SETVIDEOSTREAM
*/
inline bool MsgSetVideoStream(PluginParam *pParam, int Stream)
{
	return (*pParam->Callback)(pParam, MESSAGE_SETVIDEOSTREAM, Stream, 0) != FALSE;
}

/**
@brief ログ取得のフラグ
*/
TVTEST_DEFINE_ENUM(GetLogFlag, DWORD) {
	GET_LOG_FLAG_NONE     = 0x00000000U, /**< なし(0) */
	GET_LOG_FLAG_BYSERIAL = 0x00000001U  /**< シリアルナンバーから取得 */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(GetLogFlag)

/**
@brief ログ取得の情報
*/
struct GetLogInfo
{
	DWORD Size;       /**< 構造体のサイズ */
	GetLogFlag Flags; /**< 各種フラグ(GET_LOG_FLAG_*) */
	DWORD Index;      /**< ログのインデックス(現在保持されているログの中でのインデックス) */
	DWORD Serial;     /**< ログのシリアルナンバー(起動時からの連番) */
	LPWSTR pszText;   /**< 取得する文字列 */
	DWORD MaxText;    /**< 文字列の最大長 */
	int Type;         /**< ログの種類(LOG_TYPE_*) */
};

/**
@brief ログを取得する

@param[in] pParam プラグインパラメータ
@param[in,out] pInfo ログの情報を取得する GetLogInfo 構造体へのポインタ。
                     GetLogInfo::Size メンバに構造体のサイズを、GetLogInfo::Flags メンバにフラグを指定します。
                     GetLogInfo::Flags に TVTest::GET_LOG_FLAG_BYSERIAL を指定した場合、GetLogInfo::Serial に取得したいログのシリアルナンバーを指定します。
                     TVTest::GET_LOG_FLAG_BYSERIAL を指定しない場合、GetLogInfo::Index に取得したいログのインデックスを指定します。
                     GetLogInfo::pszText メンバに取得する文字列を格納するバッファへのポインタを、GetLogInfo::MaxText メンバにバッファに格納できる最大長(終端の空文字を含む)を指定します。
                     GetLogInfo::pszText に nullptr を指定すると、必要なバッファ長(終端の空文字を含む)が GetLogInfo::MaxText に返ります。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GETLOG

@par Example
@code{.cpp}
	// 全てのログを列挙する
	TVTest::GetLogInfo Info;
	// まず最初のログのシリアルを取得
	Info.Size = sizeof(Info);
	Info.Flags = TVTest::GET_LOG_FLAG_NONE;
	Info.Index = 0;
	Info.pszText = nullptr;
	if (TVTest::MsgGetLog(pParam, &Info)) {
		// シリアルから順番にログを取得
		WCHAR szText[256];
		Info.Flags = TVTest::GET_LOG_FLAG_BYSERIAL;
		Info.pszText = szText;
		for (;;) {
			Info.MaxText = _countof(szText);
			if (!TVTest::MsgGetLog(pParam, &Info))
				break;
			std::wprintf(L"Log %u : %ls\n", Info.Serial, Info.pszText);
			Info.Serial++;
		}
	}
@endcode
*/
inline bool MsgGetLog(PluginParam *pParam, GetLogInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETLOG, reinterpret_cast<LPARAM>(pInfo), 0) != FALSE;
}

/**
@brief ログの数を取得する

@param[in] pParam プラグインパラメータ

@return ログの数

@note ユーザーがログをクリアするなどして、数が減ることもあり得ます。

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GETLOGCOUNT
*/
inline DWORD MsgGetLogCount(PluginParam *pParam)
{
	return static_cast<DWORD>((*pParam->Callback)(pParam, MESSAGE_GETLOGCOUNT, 0, 0));
}

/**
@brief プラグインのコマンドのフラグ
*/
TVTEST_DEFINE_ENUM(PluginCommandFlag, DWORD) {
	PLUGIN_COMMAND_FLAG_NONE           = 0x00000000U, /**< なし(0) */
	PLUGIN_COMMAND_FLAG_ICONIZE        = 0x00000001U, /**< アイコン表示(サイドバーなどに表示される) */
	PLUGIN_COMMAND_FLAG_NOTIFYDRAWICON = 0x00000002U  /**< アイコン描画の通知(TVTest::EVENT_DRAWCOMMANDICON で描画を行う) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(PluginCommandFlag)

/**
@brief プラグインコマンドの状態フラグ
*/
TVTEST_DEFINE_ENUM(PluginCommandState, DWORD) {
	PLUGIN_COMMAND_STATE_NONE     = 0x00000000U, /**< なし(0) */
	PLUGIN_COMMAND_STATE_DISABLED = 0x00000001U, /**< 無効 */
	PLUGIN_COMMAND_STATE_CHECKED  = 0x00000002U  /**< チェック */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(PluginCommandState)

/**
@brief プラグインのコマンドの情報

CommandInfo が拡張されたものです。
*/
struct PluginCommandInfo
{
	DWORD Size;               /**< 構造体のサイズ */
	PluginCommandFlag Flags;  /**< 各種フラグ(PLUGIN_COMMAND_FLAG_*) */
	PluginCommandState State; /**< 状態フラグ(PLUGIN_COMMAND_STATE_*) */
	int ID;                   /**< 識別子 */
	LPCWSTR pszText;          /**< コマンドの文字列 */
	LPCWSTR pszName;          /**< コマンドの名前 */
	LPCWSTR pszDescription;   /**< コマンドの説明 */
	HBITMAP hbmIcon;          /**< アイコン */
};

/**
@brief プラグインのコマンドを登録する

基本的に MsgRegisterCommand() と同じですが、メンバが追加されています。

@param[in] pParam プラグインパラメータ
@param[in] pInfo 登録するコマンドの情報を格納した PluginCommandInfo 構造体へのポインタ

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_REGISTERPLUGINCOMMAND
*/
inline bool MsgRegisterPluginCommand(PluginParam *pParam, const PluginCommandInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_REGISTERPLUGINCOMMAND, reinterpret_cast<LPARAM>(pInfo), 0) != FALSE;
}

/**
@brief プラグインのコマンドの状態を設定する

@param[in] pParam プラグインパラメータ
@param[in] ID 状態を設定するコマンドの識別子
@param[in] State 状態(PLUGIN_COMMAND_STATE_* の組み合わせ)

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_SETPLUGINCOMMANDSTATE
*/
inline bool MsgSetPluginCommandState(PluginParam *pParam, int ID, DWORD State)
{
	return (*pParam->Callback)(pParam, MESSAGE_SETPLUGINCOMMANDSTATE, ID, State) != FALSE;
}

/**
@brief プラグインコマンドの通知の種類
*/
TVTEST_DEFINE_ENUM(PluginCommandNotify, unsigned int) {
	PLUGIN_COMMAND_NOTIFY_NONE       = 0x00000000U, /**< なし(0) */
	PLUGIN_COMMAND_NOTIFY_CHANGEICON = 0x00000001U  /**< アイコンを再描画する */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(PluginCommandNotify)

/**
@brief プラグインコマンドの通知を行う

@param[in] pParam プラグインパラメータ
@param[in] ID 通知するコマンドの識別子
@param[in] Type 通知の種類(PLUGIN_COMMAND_NOTIFY_*)

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_PLUGINCOMMANDNOTIFY
*/
inline bool MsgPluginCommandNotify(PluginParam *pParam, int ID, PluginCommandNotify Type)
{
	return (*pParam->Callback)(pParam, MESSAGE_PLUGINCOMMANDNOTIFY, ID, static_cast<LPARAM>(Type)) != FALSE;
}

/**
@brief プラグインのアイコンフラグ
*/
TVTEST_DEFINE_ENUM(PluginIconFlag, DWORD) {
	PLUGIN_ICON_FLAG_NONE = 0x00000000U /**< なし(0) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(PluginIconFlag)

/**
@brief プラグインのアイコンの情報
*/
struct PluginIconInfo
{
	DWORD Size;           /**< 構造体のサイズ */
	PluginIconFlag Flags; /**< 各種フラグ(現在は常に TVTest::PLUGIN_ICON_FLAG_NONE) */
	HBITMAP hbmIcon;      /**< アイコン */
};

/**
@brief プラグインのアイコンを登録する

アイコンを登録すると、プラグインの有効/無効をサイドバーで切り替えられるようになります。

@param[in] pParam プラグインパラメータ
@param[in] pInfo 登録するアイコンの情報を格納した PluginIconInfo 構造体へのポインタ

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_REGISTERPLUGINICON
*/
inline bool MsgRegisterPluginIcon(PluginParam *pParam, const PluginIconInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_REGISTERPLUGINICON, reinterpret_cast<LPARAM>(pInfo), 0) != FALSE;
}

/**
@brief プラグインのアイコンを登録する

@param[in] pParam プラグインパラメータ
@param[in] hbmIcon 登録するアイコン画像のビットマップ

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_REGISTERPLUGINICON
*/
inline bool MsgRegisterPluginIcon(PluginParam *pParam, HBITMAP hbmIcon)
{
	PluginIconInfo Info;
	Info.Size = sizeof(Info);
	Info.Flags = PLUGIN_ICON_FLAG_NONE;
	Info.hbmIcon = hbmIcon;
	return MsgRegisterPluginIcon(pParam, &Info);
}

/**
@brief プラグインのアイコンをリソースから読み込んで登録する

@param[in] pParam プラグインパラメータ
@param[in] hinst 読み込み元のリソースのインスタンスハンドル
@param[in] pszName 読み込むリソース名。BITMAP リソースでなければなりません。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_REGISTERPLUGINICON
*/
inline bool MsgRegisterPluginIconFromResource(PluginParam *pParam, HINSTANCE hinst, LPCTSTR pszName)
{
	HBITMAP hbmIcon = static_cast<HBITMAP>(::LoadImage(hinst, pszName, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
	const bool fResult = MsgRegisterPluginIcon(pParam, hbmIcon);
	::DeleteObject(hbmIcon);
	return fResult;
}

/**
@brief コマンドアイコンのフラグ
*/
TVTEST_DEFINE_ENUM(CommandIconFlag, WORD) {
	COMMAND_ICON_FLAG_NONE = 0x0000U /**< なし(0) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(CommandIconFlag)

/**
@brief コマンドアイコンの状態フラグ
*/
TVTEST_DEFINE_ENUM(CommandIconState, WORD) {
	COMMAND_ICON_STATE_NONE     = 0x0000U, /**< なし(0) */
	COMMAND_ICON_STATE_DISABLED = 0x0001U, /**< 無効状態 */
	COMMAND_ICON_STATE_CHECKED  = 0x0002U, /**< チェック状態 */
	COMMAND_ICON_STATE_HOT      = 0x0004U  /**< フォーカスが当たっている */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(CommandIconState)

/**
@brief コマンドアイコンの描画情報

TVTest::EVENT_DRAWCOMMANDICON で渡されます。
*/
struct DrawCommandIconInfo
{
	int ID;                 /**< コマンドの識別子 */
	CommandIconFlag Flags;  /**< 各種フラグ(現在は常に TVTest::COMMAND_ICON_FLAG_NONE) */
	CommandIconState State; /**< 状態フラグ(COMMAND_ICON_STATE_*) */
	LPCWSTR pszStyle;       /**< スタイル名 */
	HDC hdc;                /**< 描画先DC */
	RECT DrawRect;          /**< 描画先領域 */
	COLORREF Color;         /**< 色 */
	BYTE Opacity;           /**< 不透明度 */
	BYTE Reserved[3];       /**< 予約領域 */
};

/**
@brief ステータス項目のフラグ
*/
TVTEST_DEFINE_ENUM(StatusItemFlag, DWORD) {
	STATUS_ITEM_FLAG_NONE        = 0x00000000U, /**< なし(0) */
	STATUS_ITEM_FLAG_TIMERUPDATE = 0x00000001U  /**< 定期的に更新する(TVTest::STATUS_ITEM_EVENT_UPDATETIMER が呼ばれる) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(StatusItemFlag)

/**
@brief ステータス項目のスタイルフラグ
*/
TVTEST_DEFINE_ENUM(StatusItemStyle, DWORD) {
	STATUS_ITEM_STYLE_NONE          = 0x00000000U, /**< なし(0) */
	STATUS_ITEM_STYLE_VARIABLEWIDTH = 0x00000001U, /**< 可変幅 */
	STATUS_ITEM_STYLE_FULLROW       = 0x00000002U, /**< 一行表示(表示領域が足りなければ一行表示にならないこともある) */
	STATUS_ITEM_STYLE_FORCEFULLROW  = 0x00000004U  /**< 強制一行表示(常に一行表示になり、常に表示される) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(StatusItemStyle)

/**
@brief ステータス項目の情報
*/
struct StatusItemInfo
{
	DWORD Size;            /**< 構造体のサイズ */
	StatusItemFlag Flags;  /**< 各種フラグ(STATUS_ITEM_FLAG_*) */
	StatusItemStyle Style; /**< スタイルフラグ(STATUS_ITEM_STYLE_*) */
	int ID;                /**< 識別子 */
	LPCWSTR pszIDText;     /**< 識別子文字列 */
	LPCWSTR pszName;       /**< 名前 */
	int MinWidth;          /**< 最小の幅 */
	int MaxWidth;          /**< 最大の幅(-1 で制限なし) */
	int DefaultWidth;      /**< デフォルト幅(正数ではピクセル単位、負数ではフォントの高さの -1/1000 単位) */
	int MinHeight;         /**< 最小の高さ */
};

/**
@brief ステータス項目の幅をフォントサイズから求める

@param[in] Size フォントサイズ(ピクセル単位)

@return 幅の値
*/
inline int StatusItemWidthByFontSize(int Size) { return Size * -1000; }

/**
@brief ステータス項目を登録する

@param[in] pParam プラグインパラメータ
@param[in] pInfo 登録するステータス項目の情報を格納した StatusItemInfo 構造体へのポインタ

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_REGISTERSTATUSITEM
*/
inline bool MsgRegisterStatusItem(PluginParam *pParam, const StatusItemInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_REGISTERSTATUSITEM, reinterpret_cast<LPARAM>(pInfo), 0) != FALSE;
}

/**
@brief ステータス項目の状態フラグ
*/
TVTEST_DEFINE_ENUM(StatusItemState, DWORD) {
	STATUS_ITEM_STATE_NONE    = 0x00000000U, /**< なし(0) */
	STATUS_ITEM_STATE_VISIBLE = 0x00000001U, /**< 可視 */
	STATUS_ITEM_STATE_HOT     = 0x00000002U  /**< フォーカスが当たっている */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(StatusItemState)

/**
@brief ステータス項目の設定
*/
TVTEST_DEFINE_ENUM(StatusItemSetInfoMask, DWORD) {
	STATUS_ITEM_SET_INFO_MASK_NONE  = 0x00000000U, /**< なし(0) */
	STATUS_ITEM_SET_INFO_MASK_STATE = 0x00000001U, /**< StatusItemSetInfo::StateMask / StatusItemSetInfo::State を設定 */
	STATUS_ITEM_SET_INFO_MASK_STYLE = 0x00000002U  /**< StatusItemSetInfo::StyleMask / StatusItemSetInfo::Style を設定 */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(StatusItemSetInfoMask)

/**
@brief ステータス項目の設定の情報
*/
struct StatusItemSetInfo
{
	DWORD Size;                 /**< 構造体のサイズ */
	StatusItemSetInfoMask Mask; /**< 設定する情報のマスク(STATUS_ITEM_SET_INFO_MASK_*) */
	int ID;                     /**< 項目の識別子 */
	StatusItemState StateMask;  /**< 状態フラグのマスク(STATUS_ITEM_STATE_*) */
	StatusItemState State;      /**< 状態フラグ(STATUS_ITEM_STATE_*) */
	StatusItemStyle StyleMask;  /**< スタイルフラグのマスク(STATUS_ITEM_STYLE_*) */
	StatusItemStyle Style;      /**< スタイルフラグ(STATUS_ITEM_STYLE_*) */
};

/**
@brief ステータス項目を設定する

@param[in] pParam プラグインパラメータ
@param[in] pInfo 設定する情報を格納した StatusItemSetInfo 構造体へのポインタ。
                 StatusItemSetInfo::Size に構造体のサイズを、 StatusItemSetInfo::Mask に設定したい情報を、 StatusItemSetInfo::ID に設定したい項目の識別子を指定します。
                 StatusItemSetInfo::Mask に TVTest::STATUS_ITEM_SET_INFO_MASK_STATE を指定した場合、 StatusItemSetInfo::StateMask に設定したい状態のフラグを、 StatusItemSetInfo::State に有効にしたい状態のフラグを指定します。
                 StatusItemSetInfo::Mask に TVTest::STATUS_ITEM_SET_INFO_MASK_STYLE を指定した場合、 StatusItemSetInfo::StyleMask に設定したい状態のフラグを、 StatusItemSetInfo::Style に有効にしたい状態のフラグを指定します。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_SETSTATUSITEM

@par Example
@code{.cpp}
	// 項目の表示状態を設定する
	void ShowStatusItem(PluginParam *pParam, int ID, bool fVisible)
	{
		TVTest::StatusItemSetInfo Info;
		Info.Size = sizeof(Info);
		Info.Mask = TVTest::STATUS_ITEM_SET_INFO_MASK_STATE;
		Info.ID = ID;
		Info.StateMask = TVTest::STATUS_ITEM_STATE_VISIBLE;
		Info.State = fVisible ? TVTest::STATUS_ITEM_STATE_VISIBLE : TVTest::STATUS_ITEM_STATE_NONE;
		TVTest::MsgSetStatusItem(pParam, &Info);
	}
@endcode
*/
inline bool MsgSetStatusItem(PluginParam *pParam, const StatusItemSetInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_SETSTATUSITEM, reinterpret_cast<LPARAM>(pInfo), 0) != FALSE;
}

/**
@brief ステータス項目の情報取得
*/
TVTEST_DEFINE_ENUM(StatusItemGetInfoMask, DWORD) {
	STATUS_ITEM_GET_INFO_MASK_NONE        = 0x00000000U, /**< なし(0) */
	STATUS_ITEM_GET_INFO_MASK_STATE       = 0x00000001U, /**< StatusItemGetInfo::State を取得 */
	STATUS_ITEM_GET_INFO_MASK_HWND        = 0x00000002U, /**< StatusItemGetInfo::hwnd を取得 */
	STATUS_ITEM_GET_INFO_MASK_ITEMRECT    = 0x00000004U, /**< StatusItemGetInfo::ItemRect を取得 */
	STATUS_ITEM_GET_INFO_MASK_CONTENTRECT = 0x00000008U, /**< StatusItemGetInfo::ContentRect を取得 */
	STATUS_ITEM_GET_INFO_MASK_STYLE       = 0x00000010U  /**< StatusItemGetInfo::Style を取得 */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(StatusItemGetInfoMask)

/**
@brief ステータス項目の情報取得
*/
struct StatusItemGetInfo
{
	DWORD Size;                 /**< 構造体のサイズ */
	StatusItemGetInfoMask Mask; /**< 取得する情報のマスク(STATUS_ITEM_GET_INFO_MASK_*) */
	int ID;                     /**< 項目の識別子 */
	StatusItemState State;      /**< 状態フラグ(STATUS_ITEM_STATE_*) */
	HWND hwnd;                  /**< ウィンドウハンドル */
	RECT ItemRect;              /**< 項目の領域 */
	RECT ContentRect;           /**< 項目の余白を除いた領域 */
	StatusItemStyle Style;      /**< スタイルフラグ(STATUS_ITEM_STYLE_*) */
};

/**
@brief ステータス項目の情報を取得する

@param[in] pParam プラグインパラメータ
@param[in,out] pInfo 情報を取得する StatusItemGetInfo 構造体へのポインタ。
                     StatusItemGetInfo::Size に構造体のサイズを、 StatusItemGetInfo::Mask に取得したい情報を、 StatusItemGetInfo::ID に取得したい項目の識別子を指定して呼び出します。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GETSTATUSITEMINFO

@par Example
@code{.cpp}
	// 項目の表示状態を取得する
	bool IsStatusItemVisible(PluginParam *pParam, int ID)
	{
		TVTest::StatusItemGetInfo Info;
		Info.Size = sizeof(Info);
		Info.Mask = TVTest::STATUS_ITEM_GET_INFO_MASK_STATE;
		Info.ID = ID;
		return TVTest::MsgGetStatusItemInfo(pParam, &Info)
			&& (Info.State & TVTest::STATUS_ITEM_STATE_VISIBLE) != 0;
	}
@endcode
*/
inline bool MsgGetStatusItemInfo(PluginParam *pParam, StatusItemGetInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETSTATUSITEMINFO, reinterpret_cast<LPARAM>(pInfo), 0) != FALSE;
}

/**
@brief ステータス項目の通知
*/
TVTEST_DEFINE_ENUM(StatusItemNotifyType, UINT) {
	STATUS_ITEM_NOTIFY_REDRAW  /**< 再描画する */
};

/**
@brief ステータス項目の通知を行う

@param[in] pParam プラグインパラメータ
@param[in] ID 通知を行う項目の識別子
@param[in] Type 通知の種類(STATUS_ITEM_NOTIFY_* のいずれか)

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_STATUSITEMNOTIFY
*/
inline bool MsgStatusItemNotify(PluginParam *pParam, int ID, StatusItemNotifyType Type)
{
	return (*pParam->Callback)(pParam, MESSAGE_STATUSITEMNOTIFY, ID, static_cast<LPARAM>(Type)) != FALSE;
}

/**
@brief ステータス項目描画フラグ
*/
TVTEST_DEFINE_ENUM(StatusItemDrawFlag, WORD) {
	STATUS_ITEM_DRAW_FLAG_NONE    = 0x0000U, /**< なし(0) */
	STATUS_ITEM_DRAW_FLAG_PREVIEW = 0x0001U  /**< プレビュー(設定ダイアログでの表示) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(StatusItemDrawFlag)

/**
@brief ステータス項目描画状態フラグ
*/
TVTEST_DEFINE_ENUM(StatusItemDrawState, WORD) {
	STATUS_ITEM_DRAW_STATE_NONE = 0x0000U, /**< なし(0) */
	STATUS_ITEM_DRAW_STATE_HOT  = 0x0001U  /**< フォーカスが当たっている */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(StatusItemDrawState)

/**
@brief ステータス項目の描画情報

TVTest::EVENT_STATUSITEM_DRAW で渡されます。
描画はダブルバッファリングによって行われるため、 StatusItemDrawInfo::ItemRect / StatusItemDrawInfo::DrawRect で渡される位置は表示上の位置とは異なっています。
*/
struct StatusItemDrawInfo
{
	int ID;                    /**< 項目の識別子 */
	StatusItemDrawFlag Flags;  /**< 各種フラグ(STATUS_ITEM_DRAW_FLAG_*) */
	StatusItemDrawState State; /**< 状態フラグ(STATUS_ITEM_DRAW_STATE_*) */
	LPCWSTR pszStyle;          /**< スタイル */
	HDC hdc;                   /**< 描画先 DC */
	RECT ItemRect;             /**< 項目の領域 */
	RECT DrawRect;             /**< 描画する領域 */
	COLORREF Color;            /**< 色 */
};

/**
@brief ステータス項目のイベント
*/
TVTEST_DEFINE_ENUM(StatusItemEvent, UINT) {
	STATUS_ITEM_EVENT_CREATED = 1,       /**< 項目が作成された */
	STATUS_ITEM_EVENT_VISIBILITYCHANGED, /**< 項目の表示状態が変わった */
	STATUS_ITEM_EVENT_ENTER,             /**< フォーカスが当たった */
	STATUS_ITEM_EVENT_LEAVE,             /**< フォーカスが離れた */
	STATUS_ITEM_EVENT_SIZECHANGED,       /**< 項目の大きさが変わった */
	STATUS_ITEM_EVENT_UPDATETIMER,       /**< 更新タイマー */
	STATUS_ITEM_EVENT_STYLECHANGED,      /**< スタイルが変わった(DPI の変更など) */
	STATUS_ITEM_EVENT_FONTCHANGED        /**< フォントが変わった */
};

/**
@brief ステータス項目の通知情報

TVTest::EVENT_STATUSITEM_NOTIFY で渡されます。
*/
struct StatusItemEventInfo
{
	int ID;                /**< 項目の識別子 */
	StatusItemEvent Event; /**< イベントの種類(STATUS_ITEM_EVENT_*) */
	LPARAM Param;          /**< パラメータ */
};

/**
@brief ステータス項目のマウス操作の種類

STATUS_ITEM_MOUSE_ACTION_*DOWN か STATUS_ITEM_MOUSE_ACTION_*DOUBLECLICK が送られた際に SetCapture() でマウスキャプチャが行われると、
キャプチャが解除された時に STATUS_ITEM_MOUSE_ACTION_CAPTURERELEASE が送られます。
*/
TVTEST_DEFINE_ENUM(StatusItemMouseAction, UINT) {
	STATUS_ITEM_MOUSE_ACTION_LDOWN = 1,     /**< 左ボタンが押された */
	STATUS_ITEM_MOUSE_ACTION_LUP,           /**< 左ボタンが離された */
	STATUS_ITEM_MOUSE_ACTION_LDOUBLECLICK,  /**< 左ダブルクリック */
	STATUS_ITEM_MOUSE_ACTION_RDOWN,         /**< 右ボタンが押された */
	STATUS_ITEM_MOUSE_ACTION_RUP,           /**< 右ボタンが離された */
	STATUS_ITEM_MOUSE_ACTION_RDOUBLECLICK,  /**< 右ダブルクリック */
	STATUS_ITEM_MOUSE_ACTION_MDOWN,         /**< 中央ボタンが押された */
	STATUS_ITEM_MOUSE_ACTION_MUP,           /**< 中央ボタンが離された */
	STATUS_ITEM_MOUSE_ACTION_MDOUBLECLICK,  /**< 中央ダブルクリック */
	STATUS_ITEM_MOUSE_ACTION_MOVE,          /**< カーソル移動 */
	STATUS_ITEM_MOUSE_ACTION_WHEEL,         /**< ホイール */
	STATUS_ITEM_MOUSE_ACTION_HORZWHEEL,     /**< 横ホイール */
	STATUS_ITEM_MOUSE_ACTION_CAPTURERELEASE /**< キャプチャが解除された */
};

/**
@brief ステータス項目のマウスイベント情報

TVTest::EVENT_STATUSITEM_MOUSE で渡されます。
*/
struct StatusItemMouseEventInfo
{
	int ID;                       /**< 項目の識別子 */
	StatusItemMouseAction Action; /**< マウス操作の種類(STATUS_ITEM_MOUSE_ACTION_*) */
	HWND hwnd;                    /**< ウィンドウハンドル */
	POINT CursorPos;              /**< カーソル位置(クライアント座標) */
	RECT ItemRect;                /**< 項目の領域 */
	RECT ContentRect;             /**< 項目の余白を除いた領域 */
	int WheelDelta;               /**< ホイール移動量 */
};

#ifndef TVTEST_INTERFACE_H
/**
@brief Interface 名前空間

TS プロセッサのインターフェースは TVTestInterface.h で宣言されています。
*/
namespace Interface {
	/**
	@brief TS プロセッサのインターフェース

	TVTestInterface.h で宣言されています。
	*/
	struct ITSProcessor;
}
#endif

/**
@brief TS プロセッサの接続位置

詳細は TVTestInterface.h を参照してください。
*/
TVTEST_DEFINE_ENUM(TSProcessorConnectPosition, DWORD) {
	TS_PROCESSOR_CONNECT_POSITION_SOURCE,         /**< ソース(チューナー等からストリームが入力された後) */
	TS_PROCESSOR_CONNECT_POSITION_PREPROCESSING,  /**< 前処理(TS を解析する前) */
	TS_PROCESSOR_CONNECT_POSITION_POSTPROCESSING, /**< 後処理(TS を解析した後) */
	TS_PROCESSOR_CONNECT_POSITION_VIEWER,         /**< ビューア(再生の前) */
	TS_PROCESSOR_CONNECT_POSITION_RECORDER        /**< レコーダ(ストリーム書き出しの前) */
};

/**
@brief TS プロセッサのフラグ
*/
TVTEST_DEFINE_ENUM(TSProcessorFlag, DWORD) {
	TS_PROCESSOR_FLAG_NONE = 0x00000000U /**< なし(0) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(TSProcessorFlag)

/**
@brief TSプロセッサの情報
*/
struct TSProcessorInfo
{
	DWORD Size;                                 /**< 構造体のサイズ */
	TSProcessorFlag Flags;                      /**< 各種フラグ(現在は常に TVTest::TS_PROCESSOR_FLAG_NONE) */
	Interface::ITSProcessor *pTSProcessor;      /**< 接続する ITSProcessor */
	TSProcessorConnectPosition ConnectPosition; /**< 接続位置(TS_PROCESSOR_CONNECT_POSITION_*) */
};

/**
@brief TSプロセッサを登録する

@param[in] pParam プラグインパラメータ
@param[in] pInfo 登録する TS プロセッサの情報を格納した TSProcessorInfo 構造体へのポインタ

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_REGISTERTSPROCESSOR
*/
inline bool MsgRegisterTSProcessor(PluginParam *pParam, const TSProcessorInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_REGISTERTSPROCESSOR, reinterpret_cast<LPARAM>(pInfo), 0) != FALSE;
}

/**
@brief パネル項目のスタイル
*/
TVTEST_DEFINE_ENUM(PanelItemStyle, DWORD){
	PANEL_ITEM_STYLE_NONE      = 0x0000U, /**< なし(0) */
	PANEL_ITEM_STYLE_NEEDFOCUS = 0x0001U  /**< キーボードフォーカスを受け取る */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(PanelItemStyle)

/**
@brief パネル項目の状態
*/
TVTEST_DEFINE_ENUM(PanelItemState, DWORD){
	PANEL_ITEM_STATE_NONE    = 0x0000U, /**< なし(0) */
	PANEL_ITEM_STATE_ENABLED = 0x0001U, /**< 有効(タブに表示されている) */
	PANEL_ITEM_STATE_ACTIVE  = 0x0002U  /**< アクティブ */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(PanelItemState)

/**
@brief パネル項目のフラグ
*/
TVTEST_DEFINE_ENUM(PanelItemFlag, DWORD) {
	PANEL_ITEM_FLAG_NONE = 0x00000000U /**< なし(0) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(PanelItemFlag)

/**
@brief パネル項目の情報
*/
struct PanelItemInfo
{
	DWORD Size;           /**< 構造体のサイズ */
	PanelItemFlag Flags;  /**< 各種フラグ(現在は常に TVTest::PANEL_ITEM_FLAG_NONE) */
	PanelItemStyle Style; /**< スタイルフラグ(PANEL_ITEM_STYLE_* の組み合わせ) */
	int ID;               /**< 識別子 */
	LPCWSTR pszIDText;    /**< 識別子文字列 */
	LPCWSTR pszTitle;     /**< タイトル */
	HBITMAP hbmIcon;      /**< アイコンのビットマップ */
};

/**
@brief パネル項目を登録する

@param[in] pParam プラグインパラメータ
@param[in] pInfo 登録するパネル項目の情報を格納した PanelItemInfo 構造体へのポインタ

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_REGISTERPANELITEM
*/
inline bool MsgRegisterPanelItem(PluginParam *pParam, const PanelItemInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_REGISTERPANELITEM, reinterpret_cast<LPARAM>(pInfo), 0) != FALSE;
}

/**
@brief パネル項目の設定
*/
TVTEST_DEFINE_ENUM(PanelItemSetInfoMask, DWORD) {
	PANEL_ITEM_SET_INFO_MASK_NONE  = 0x00000000U, /**< なし(0) */
	PANEL_ITEM_SET_INFO_MASK_STATE = 0x00000001U, /**< PanelItemSetInfo::StateMask / PanelItemSetInfo::State を設定 */
	PANEL_ITEM_SET_INFO_MASK_STYLE = 0x00000002U  /**< PanelItemSetInfo::StyleMask / PanelItemSetInfo::Style を設定 */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(PanelItemSetInfoMask)

/**
@brief パネル項目の設定の情報
*/
struct PanelItemSetInfo
{
	DWORD Size;                /**< 構造体のサイズ */
	PanelItemSetInfoMask Mask; /**< 設定する情報のマスク(PANEL_ITEM_SET_INFO_MASK_* の組み合わせ) */
	int ID;                    /**< 項目の識別子 */
	PanelItemState StateMask;  /**< 状態フラグのマスク(PANEL_ITEM_STATE_* の組み合わせ) */
	PanelItemState State;      /**< 状態フラグ(PANEL_ITEM_STATE_* の組み合わせ) */
	PanelItemStyle StyleMask;  /**< スタイルフラグのマスク(PANEL_ITEM_STYLE_* の組み合わせ) */
	PanelItemStyle Style;      /**< スタイルフラグ(PANEL_ITEM_STYLE_* の組み合わせ) */
};

/**
@brief パネル項目を設定する

@param[in] pParam プラグインパラメータ
@param[in] pInfo 設定するパネル項目の情報を格納した PanelItemSetInfo 構造体へのポインタ。
                 PanelItemSetInfo::Size に構造体のサイズを、 PanelItemSetInfo::Mask に設定したい情報を、 PanelItemSetInfo::ID に設定したい項目の識別子を指定して呼び出します。
                 PanelItemSetInfo::Mask に TVTest::PANEL_ITEM_SET_INFO_MASK_STATE を指定した場合、 PanelItemSetInfo::StateMask に設定したい状態のフラグを、 PanelItemSetInfo::State に有効にしたい状態のフラグを指定します。
                 PanelItemSetInfo::Mask に TVTest::PANEL_ITEM_SET_INFO_MASK_STYLE を指定した場合、 PanelItemSetInfo::StyleMask に設定したい状態のフラグを、 PanelItemSetInfo::Style に有効にしたい状態のフラグを指定します。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_SETPANELITEM

@par Example
@code{.cpp}
	// 項目の有効状態を設定する
	void EnablePanelItem(PluginParam *pParam, int ID, bool fEnable)
	{
		TVTest::PanelItemSetInfo Info;
		Info.Size = sizeof(Info);
		Info.Mask = TVTest::PANEL_ITEM_SET_INFO_MASK_STATE;
		Info.ID = ID;
		Info.StateMask = TVTest::PANEL_ITEM_STATE_ENABLED;
		Info.State = fEnable ? TVTest::PANEL_ITEM_STATE_ENABLED : TVTest::PANEL_ITEM_STATE_NONE;
		TVTest::MsgSetPanelItem(pParam, &Info);
	}
@endcode
*/
inline bool MsgSetPanelItem(PluginParam *pParam, const PanelItemSetInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_SETPANELITEM, reinterpret_cast<LPARAM>(pInfo), 0) != FALSE;
}

/**
@brief パネル項目の情報取得マスク
*/
TVTEST_DEFINE_ENUM(PanelItemGetInfoMask, DWORD) {
	PANEL_ITEM_GET_INFO_MASK_NONE       = 0x0000U, /**< なし(0) */
	PANEL_ITEM_GET_INFO_MASK_STATE      = 0x0001U, /**< PanelItemGetInfo::State を取得 */
	PANEL_ITEM_GET_INFO_MASK_HWNDPARENT = 0x0002U, /**< PanelItemGetInfo::hwndParent を取得 */
	PANEL_ITEM_GET_INFO_MASK_HWNDITEM   = 0x0004U, /**< PanelItemGetInfo::hwndItem を取得 */
	PANEL_ITEM_GET_INFO_MASK_STYLE      = 0x0008U  /**< PanelItemGetInfo::Style を取得 */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(PanelItemGetInfoMask)

/**
@brief パネル項目の情報取得
*/
struct PanelItemGetInfo
{
	DWORD Size;                /**< 構造体のサイズ */
	PanelItemGetInfoMask Mask; /**< 取得する情報のマスク(PANEL_ITEM_GET_INFO_MASK_* の組み合わせ) */
	int ID;                    /**< 項目の識別子 */
	PanelItemState State;      /**< 項目の状態フラグ(PANEL_ITEM_STATE_* の組み合わせ) */
	HWND hwndParent;           /**< 親ウィンドウのハンドル */
	HWND hwndItem;             /**< 項目のウィンドウハンドル */
	PanelItemStyle Style;      /**< スタイルフラグ(PANEL_ITEM_STYLE_* の組み合わせ) */
};

/**
@brief パネル項目の情報を取得する

@param[in] pParam プラグインパラメータ
@param[in,out] pInfo 情報を取得する PanelItemGetInfo 構造体へのポインタ。
                     PanelItemGetInfo::Size に構造体のサイズを、 PanelItemGetInfo::Mask に取得したい情報を、 PanelItemGetInfo::ID に取得したい項目の識別子を指定して呼び出します。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GETPANELITEMINFO
*/
inline bool MsgGetPanelItemInfo(PluginParam *pParam, PanelItemGetInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETPANELITEMINFO, reinterpret_cast<LPARAM>(pInfo), 0) != FALSE;
}

/**
@brief パネル項目のイベント
*/
TVTEST_DEFINE_ENUM(PanelItemEvent, UINT) {
	PANEL_ITEM_EVENT_CREATE = 1,   /**< 項目を作成する */
	PANEL_ITEM_EVENT_ACTIVATE,     /**< 項目がアクティブになる */
	PANEL_ITEM_EVENT_DEACTIVATE,   /**< 項目が非アクティブになる */
	PANEL_ITEM_EVENT_ENABLE,       /**< 項目が有効になる */
	PANEL_ITEM_EVENT_DISABLE,      /**< 項目が無効になる */
	PANEL_ITEM_EVENT_STYLECHANGED, /**< スタイルが変わった(DPI の変更など) */
	PANEL_ITEM_EVENT_FONTCHANGED   /**< フォントが変わった */
};

/**
@brief パネル項目の通知情報

TVTest::EVENT_PANELITEM_NOTIFY で渡されます
*/
struct PanelItemEventInfo
{
	int ID;               /**< 項目の識別子 */
	PanelItemEvent Event; /**< イベントの種類(PANEL_ITEM_EVENT_* のいずれか) */
};

/**
@brief パネル項目作成イベントの情報

TVTest::PANEL_ITEM_EVENT_CREATE で渡されます。
hwndItem に作成したウィンドウのハンドルを返します。
*/
struct PanelItemCreateEventInfo
{
	PanelItemEventInfo EventInfo; /**< イベントの通知情報 */
	RECT ItemRect;                /**< 項目の位置 */
	HWND hwndParent;              /**< 親ウィンドウのハンドル */
	HWND hwndItem;                /**< 項目のウィンドウハンドル */
};

/**
@brief チャンネル選択のフラグ
*/
TVTEST_DEFINE_ENUM(ChannelSelectFlag, DWORD) {
	CHANNEL_SELECT_FLAG_NONE            = 0x0000U, /**< なし(0) */
	/**
	@brief ServiceID の指定を厳密に扱う
	@details このフラグを指定しない場合、 ChannelSelectInfo::ServiceID で指定されたサービスが存在しない時は他の視聴可能なサービスを選択します。
	*/
	CHANNEL_SELECT_FLAG_STRICTSERVICE   = 0x0001U
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 15)
	/**
	@brief 無効に設定されているチャンネルも選択する
	@since ver.0.0.15
	*/
	, CHANNEL_SELECT_FLAG_ALLOWDISABLED = 0x0002U
#endif
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(ChannelSelectFlag)

/**
@brief チャンネル選択の情報
*/
struct ChannelSelectInfo
{
	DWORD Size;              /**< 構造体のサイズ */
	ChannelSelectFlag Flags; /**< 各種フラグ(CHANNEL_SELECT_FLAG_* の組み合わせ) */
	LPCWSTR pszTuner;        /**< チューナー名(nullptr で指定なし) */
	int Space;               /**< チューニング空間(-1 で指定なし) */
	int Channel;             /**< チャンネル(-1 で指定なし) */
	WORD NetworkID;          /**< ネットワーク ID(0 で指定なし) */
	WORD TransportStreamID;  /**< トランスポートストリーム ID(0 で指定なし) */
	WORD ServiceID;          /**< サービス ID(0 で指定なし) */
};

/**
@brief チャンネルを選択する

@param[in] pParam プラグインパラメータ
@param[in] pInfo 選択するチャンネルの情報を格納した ChannelSelectInfo 構造体へのポインタ

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_SELECTCHANNEL
*/
inline bool MsgSelectChannel(PluginParam *pParam, const ChannelSelectInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_SELECTCHANNEL, reinterpret_cast<LPARAM>(pInfo), 0) != FALSE;
}

/**
@brief お気に入り項目の種類
*/
TVTEST_DEFINE_ENUM(FavoriteItemType, DWORD) {
	FAVORITE_ITEM_TYPE_FOLDER,  /**< フォルダ */
	FAVORITE_ITEM_TYPE_CHANNEL  /**< チャンネル */
};

/**
@brief お気に入り項目のフラグ
*/
TVTEST_DEFINE_ENUM(FavoriteItemFlag, DWORD) {
	FAVORITE_ITEM_FLAG_NONE = 0x00000000U, /**< なし(0) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(FavoriteItemFlag)

/**
@brief お気に入りフォルダのフラグ
*/
TVTEST_DEFINE_ENUM(FavoriteFolderFlag, DWORD) {
	FAVORITE_FOLDER_FLAG_NONE = 0x00000000U, /**< なし(0) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(FavoriteFolderFlag)

/**
@brief お気に入りチャンネルのフラグ
*/
TVTEST_DEFINE_ENUM(FavoriteChannelFlag, DWORD) {
	FAVORITE_CHANNEL_FLAG_NONE             = 0x0000U, /**< なし(0) */
	FAVORITE_CHANNEL_FLAG_FORCETUNERCHANGE = 0x0001U  /**< チューナー指定を強制 */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(FavoriteChannelFlag)

/**
@brief お気に入り項目の情報
*/
struct FavoriteItemInfo
{
	FavoriteItemType Type;                           /**< 種類(FAVORITE_ITEM_TYPE_* のいずれか) */
	FavoriteItemFlag Flags;                          /**< 各種フラグ(現在は常に TVTest::FAVORITE_ITEM_FLAG_NONE) */
	LPCWSTR pszName;                                 /**< 名前 */
	union {
		struct {
			FavoriteFolderFlag Flags;                /**< 各種フラグ(現在は常に TVTest::FAVORITE_FOLDER_FLAG_NONE) */
			DWORD ItemCount;                         /**< 子項目数 */
			const struct FavoriteItemInfo *ItemList; /**< 子項目のリスト */
		} Folder;                                    /**< フォルダの情報(FavoriteItemInfo::Type == TVTest::FAVORITE_ITEM_TYPE_FOLDER の場合) */
		struct {
			FavoriteChannelFlag Flags; /**< 各種フラグ(FAVORITE_CHANNEL_FLAG_*) */
			int Space;                               /**< チューニング空間 */
			int Channel;                             /**< チャンネルインデックス */
			int ChannelNo;                           /**< リモコン番号 */
			WORD NetworkID;                          /**< ネットワーク ID */
			WORD TransportStreamID;                  /**< トランスポートストリーム ID */
			WORD ServiceID;                          /**< サービス ID */
			WORD Reserved;                           /**< 予約(現在は常に 0) */
			LPCWSTR pszTuner;                        /**< チューナー名 */
		} Channel;                                   /**< チャンネルの情報(FavoriteItemInfo::Type == TVTest::FAVORITE_ITEM_TYPE_CHANNEL の場合) */
	};
};

/**
@brief お気に入りリスト
*/
struct FavoriteList
{
	DWORD Size;                 /**< 構造体のサイズ */
	DWORD ItemCount;            /**< お気に入り項目数 */
	FavoriteItemInfo *ItemList; /**< お気に入り項目のリスト */
};

/**
@brief お気に入りリストを取得する

@param[in] pParam プラグインパラメータ
@param[in,out] pList お気に入りリストを取得する FavoriteList 構造体へのポインタ。
                     FavoriteList::Size メンバに構造体のサイズを設定して呼び出します。

@retval true 正常終了
@retval false エラー発生

@note 取得したリストは MsgFreeFavoriteList() で解放します。

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GETFAVORITELIST

@par Example
@code{.cpp}
	TVTest::FavoriteList List;
	List.Size = sizeof(List);
	if (TVTest::MsgGetFavoriteList(pParam, &List)) {
		for (DWORD i = 0; i < List.ItemCount; i++) {
			const TVTest::FavoriteItemInfo &Item = List.ItemList[i];
			if (Item.Type == TVTest::FAVORITE_ITEM_TYPE_FOLDER) {
				// Item.Folder にフォルダの情報が格納されている
				for (DWORD j = 0; j < Item.Folder.ItemCount; j++) {
					const TVTest::FavoriteItemInfo &SubItem = Item.Folder.ItemList[j];
					...
				}
			} else if (Item.Type == TVTest::FAVORITE_ITEM_TYPE_CHANNEL) {
				// Item.Channel にチャンネルの情報が格納されている
				...
			}
		}

		TVTest::MsgFreeFavoriteList(pParam, &List);
	}
@endcode
*/
inline bool MsgGetFavoriteList(PluginParam *pParam, FavoriteList *pList)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETFAVORITELIST, reinterpret_cast<LPARAM>(pList), 0) != FALSE;
}

/**
@brief お気に入りリストを解放する

MsgGetFavoriteList() で取得したリストを解放します。

@param[in] pParam プラグインパラメータ
@param[in,out] pList 解放するお気に入りリストの FavoriteList 構造体へのポインタ。

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_FREEFAVORITELIST
*/
inline void MsgFreeFavoriteList(PluginParam *pParam, FavoriteList *pList)
{
	(*pParam->Callback)(pParam, MESSAGE_FREEFAVORITELIST, reinterpret_cast<LPARAM>(pList), 0);
}

/**
@brief ワンセグモードを取得する

@param[in] pParam プラグインパラメータ

@retval true ワンセグモード
@retval false ワンセグモードではない

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GET1SEGMODE
*/
inline bool MsgGet1SegMode(PluginParam *pParam)
{
	return (*pParam->Callback)(pParam, MESSAGE_GET1SEGMODE, 0, 0) != FALSE;
}

/**
@brief ワンセグモードを設定する

@param[in] pParam プラグインパラメータ
@param[in] f1SegMode ワンセグモードに設定する場合は true、ワンセグモードを解除する場合は false

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_SET1SEGMODE
*/
inline bool MsgSet1SegMode(PluginParam *pParam, bool f1SegMode)
{
	return (*pParam->Callback)(pParam, MESSAGE_SET1SEGMODE, f1SegMode, 0) != FALSE;
}

/**
@brief 取得する DPI の種類
*/
enum DPIType {
	DPI_TYPE_SYSTEM,  /**< システム */
	DPI_TYPE_WINDOW,  /**< ウィンドウ */
	DPI_TYPE_RECT,    /**< 矩形 */
	DPI_TYPE_POINT,   /**< 位置 */
	DPI_TYPE_MONITOR  /**< モニタ */
};

/**
@brief DPI 取得のフラグ
*/
TVTEST_DEFINE_ENUM(DPIFlag, DWORD) {
	DPI_FLAG_NONE   = 0x00000000U, /**< なし(0) */
	DPI_FLAG_FORCED = 0x00000001U  /**< 強制指定された DPI を取得 */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(DPIFlag)

/**
@brief DPI 取得の情報
*/
struct GetDPIInfo
 {
	DPIType Type;          /**< 取得する DPI の種類(DPI_TYPE_*) */
	DPIFlag Flags;         /**< フラグ(DPI_FLAG_*) */
	union {
		HWND hwnd;         /**< ウィンドウハンドル(GetDPIInfo::Type == TVTest::DPI_TYPE_WINDOW) */
		RECT Rect;         /**< 矩形(GetDPIInfo::Type == TVTest::DPI_TYPE_RECT) */
		POINT Point;       /**< 位置(GetDPIInfo::Type == TVTest::DPI_TYPE_POINT) */
		HMONITOR hMonitor; /**< モニタ(GetDPIInfo::Type == TVTest::DPI_TYPE_MONITOR) */
	};
};

/**
@brief DPI を取得する

@param[in] pParam プラグインパラメータ
@param[in] pInfo 取得する DPI の種類を指定する GetDPIInfo 構造体へのポインタ。
                 GetDPIInfo::Type に取得する種類を、 GetDPIInfo::Flags にフラグを指定します。
                 GetDPIInfo::Type が TVTest::DPI_TYPE_WINDOW の場合、 GetDPIInfo::hwnd メンバにウィンドウハンドルを指定します。
                 GetDPIInfo::Type が TVTest::DPI_TYPE_RECT の場合、 GetDPIInfo::Rect メンバに矩形を指定します。
                 GetDPIInfo::Type が TVTest::DPI_TYPE_POINT の場合、 GetDPIInfo::Point メンバに位置を指定します。
                 GetDPIInfo::Type が TVTest::DPI_TYPE_MONITOR の場合、 GetDPIInfo::hMonitor メンバにモニタのハンドルを指定します。

@return DPI の値。エラー時は 0

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GETDPI
*/
inline int MsgGetDPI(PluginParam *pParam, GetDPIInfo *pInfo)
{
	return static_cast<int>((*pParam->Callback)(pParam, MESSAGE_GETDPI, reinterpret_cast<LPARAM>(pInfo), 0));
}

/**
@brief システムの DPI を取得する

@param[in] pParam プラグインパラメータ

@return DPI の値

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GETDPI
*/
inline int MsgGetSystemDPI(PluginParam *pParam)
{
	GetDPIInfo Info;
	Info.Type = DPI_TYPE_SYSTEM;
	Info.Flags = DPI_FLAG_NONE;
	return static_cast<int>((*pParam->Callback)(pParam, MESSAGE_GETDPI, reinterpret_cast<LPARAM>(&Info), 0));
}

/**
@brief ウィンドウの DPI を取得する

@param[in] pParam プラグインパラメータ
@param[in] hwnd ウィンドウのハンドル
@param[in] Flags フラグ(DPI_FLAG_*)

@return DPI の値。エラー時は 0

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GETDPI
*/
inline int MsgGetDPIFromWindow(PluginParam *pParam, HWND hwnd, DPIFlag Flags = DPI_FLAG_NONE)
{
	GetDPIInfo Info;
	Info.Type = DPI_TYPE_WINDOW;
	Info.Flags = Flags;
	Info.hwnd = hwnd;
	return static_cast<int>((*pParam->Callback)(pParam, MESSAGE_GETDPI, reinterpret_cast<LPARAM>(&Info), 0));
}

/**
@brief 矩形位置の DPI を取得する

@param[in] pParam プラグインパラメータ
@param[in] Rect 矩形位置
@param[in] Flags フラグ(DPI_FLAG_*)

@return DPI の値。エラー時は 0

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GETDPI
*/
inline int MsgGetDPIFromRect(PluginParam *pParam, const RECT &Rect, DPIFlag Flags = DPI_FLAG_NONE)
{
	GetDPIInfo Info;
	Info.Type = DPI_TYPE_RECT;
	Info.Flags = Flags;
	Info.Rect = Rect;
	return static_cast<int>((*pParam->Callback)(pParam, MESSAGE_GETDPI, reinterpret_cast<LPARAM>(&Info), 0));
}

/**
@brief 指定位置の DPI を取得する

@param[in] pParam プラグインパラメータ
@param[in] Point 位置
@param[in] Flags フラグ(DPI_FLAG_*)

@return DPI の値。エラー時は 0

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GETDPI
*/
inline int MsgGetDPIFromPoint(PluginParam *pParam, const POINT &Point, DPIFlag Flags = DPI_FLAG_NONE)
{
	GetDPIInfo Info;
	Info.Type = DPI_TYPE_POINT;
	Info.Flags = Flags;
	Info.Point = Point;
	return static_cast<int>((*pParam->Callback)(pParam, MESSAGE_GETDPI, reinterpret_cast<LPARAM>(&Info), 0));
}

/**
@brief 指定位置の DPI を取得する

@param[in] pParam プラグインパラメータ
@param[in] x 左位置
@param[in] y 上位置
@param[in] Flags フラグ(DPI_FLAG_*)

@return DPI の値。エラー時は 0

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GETDPI
*/
inline int MsgGetDPIFromPoint(PluginParam *pParam, LONG x, LONG y, DPIFlag Flags = DPI_FLAG_NONE)
{
	POINT pt;
	pt.x = x;
	pt.y = y;
	return MsgGetDPIFromPoint(pParam, pt, Flags);
}

/**
@brief モニタの DPI を取得する

@param[in] pParam プラグインパラメータ
@param[in] hMonitor モニタのハンドル
@param[in] Flags フラグ(DPI_FLAG_*)

@return DPI の値。エラー時は 0

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GETDPI
*/
inline int MsgGetDPIFromMonitor(PluginParam *pParam, HMONITOR hMonitor, DPIFlag Flags = DPI_FLAG_NONE)
{
	GetDPIInfo Info;
	Info.Type = DPI_TYPE_MONITOR;
	Info.Flags = Flags;
	Info.hMonitor = hMonitor;
	return static_cast<int>((*pParam->Callback)(pParam, MESSAGE_GETDPI, reinterpret_cast<LPARAM>(&Info), 0));
}

/**
@brief フォント取得のフラグ
*/
TVTEST_DEFINE_ENUM(GetFontFlag, DWORD) {
	GET_FONT_FLAG_NONE = 0x00000000U /**< なし(0) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(GetFontFlag)

/**
@brief フォント取得の情報
*/
struct GetFontInfo
{
	DWORD Size;        /**< 構造体のサイズ */
	GetFontFlag Flags; /**< フラグ(現在は常に TVTest::GET_FONT_FLAG_NONE) */
	LPCWSTR pszName;   /**< 取得するフォントの指定 */
	LOGFONTW LogFont;  /**< 取得したフォント */
	int DPI;           /**< DPI の指定(0 で指定なし) */
};

/**
@brief フォントを取得する

以下のフォントが取得できます。

| 識別子           | 意味                     |
|------------------|--------------------------|
| OSDFont          | OSD のフォント           |
| PanelFont        | パネルのフォント         |
| ProgramGuideFont | 番組表のフォント         |
| StatusBarFont    | ステータスバーのフォント |

@param[in] pParam プラグインパラメータ
@param[in,out] pInfo フォントの情報を取得する GetFontInfo 構造体へのポインタ。
                     GetFontInfo::Size メンバに構造体のサイズを、 GetFontInfo::Flags メンバにフラグを、 GetFontInfo::pszName メンバに取得したいフォントの名前を指定します。
                     GetFontInfo::DPI メンバにフォントの大きさをスケーリングするための GetFontInfo::DPI 値を指定します。
                     GetFontInfo::LogFont メンバにフォントの情報が返されます。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GETFONT
*/
inline bool MsgGetFont(PluginParam *pParam, GetFontInfo *pInfo)
{
	return static_cast<int>((*pParam->Callback)(pParam, MESSAGE_GETFONT, reinterpret_cast<LPARAM>(pInfo), 0)) != FALSE;
}

/**
@brief フォントを取得する

@param[in] pParam プラグインパラメータ
@param[in] pszName 取得したいフォントの名前。
                   指定できる名前は MsgGetFont(PluginParam *, GetFontInfo *) を参照してください。
@param[out] pLogFont フォントの情報を取得する LOGFONTW 構造体へのポインタ
@param[in] DPI フォントの大きさをスケーリングするための DPI 値。スケーリングを行わない場合は 0

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GETFONT
*/
inline bool MsgGetFont(PluginParam *pParam, LPCWSTR pszName, LOGFONTW *pLogFont, int DPI = 0)
{
	GetFontInfo Info;
	Info.Size = sizeof(GetFontInfo);
	Info.Flags = GET_FONT_FLAG_NONE;
	Info.pszName = pszName;
	Info.DPI = DPI;
	if (!MsgGetFont(pParam, &Info))
		return false;
	*pLogFont = Info.LogFont;
	return true;
}

/**
@brief ダイアログのメッセージ処理関数
*/
typedef INT_PTR (CALLBACK *DialogMessageFunc)(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, void *pClientData);

/**
@brief ダイアログ表示のフラグ
*/
TVTEST_DEFINE_ENUM(ShowDialogFlag, DWORD) {
	SHOW_DIALOG_FLAG_NONE                = 0x00000000U, /**< なし(0) */
	SHOW_DIALOG_FLAG_MODELESS            = 0x00000001U, /**< モードレス */
	SHOW_DIALOG_FLAG_POSITION            = 0x00000002U  /**< 位置指定が有効 */
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 15)
	/**
	@brief ダークモードにしない
	@since ver.0.0.15
	*/
	, SHOW_DIALOG_FLAG_DISABLE_DARK_MODE = 0x00000004U
#endif
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(ShowDialogFlag)

/**
@brief ダイアログ表示の情報
*/
struct ShowDialogInfo
{
	DWORD Size;                     /**< 構造体のサイズ */
	ShowDialogFlag Flags;           /**< フラグ(SHOW_DIALOG_FLAG_*) */
	HINSTANCE hinst;                /**< ダイアログテンプレートのインスタンス */
	LPCWSTR pszTemplate;            /**< ダイアログテンプレート */
	DialogMessageFunc pMessageFunc; /**< メッセージ処理関数 */
	void *pClientData;              /**< メッセージ処理関数に渡すパラメータ */
	HWND hwndOwner;                 /**< オーナーウィンドウのハンドル */
	POINT Position;                 /**< ダイアログの位置(ShowDialogInfo::Flags に TVTest::SHOW_DIALOG_FLAG_POSITION が指定されている場合に有効) */
};

/**
@brief ダイアログを表示する

DialogBox() / CreateDialog() などの代わりに MsgShowDialog() を利用すると、
DPI に応じたスケーリングやダークモードへの対応などが自動的に行われます。

@param[in] pParam プラグインパラメータ
@param[in] pInfo ダイアログ表示の情報を格納した ShowDialogInfo 構造体へのポインタ。
                 ShowDialogInfo::Flags に TVTest::SHOW_DIALOG_FLAG_MODELESS を指定するとモードレスダイアログが作成され、
                 指定しない場合はモーダルダイアログが作成されます。

@return モーダルダイアログの場合、EndDialog() で指定された戻り値。
        モードレスダイアログの場合、ダイアログのウィンドウハンドル。

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_SHOWDIALOG
*/
inline INT_PTR MsgShowDialog(PluginParam *pParam, ShowDialogInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_SHOWDIALOG, reinterpret_cast<LPARAM>(pInfo), 0);
}

/**
@brief 各種日時
*/
union TimeUnion {
	SYSTEMTIME SystemTime; /**< システム日時 */
	FILETIME FileTime;     /**< ファイル日時 */
};

/**
@brief 日時変換のフラグ
*/
TVTEST_DEFINE_ENUM(ConvertTimeFlag, DWORD) {
	CONVERT_TIME_FLAG_NONE          = 0x00000000U, /**< なし(0) */
	CONVERT_TIME_FLAG_FROM_FILETIME = 0x00000001U, /**< FILETIME から変換 */
	CONVERT_TIME_FLAG_TO_FILETIME   = 0x00000002U, /**< FILETIME へ変換 */
	CONVERT_TIME_FLAG_FILETIME      = CONVERT_TIME_FLAG_FROM_FILETIME | CONVERT_TIME_FLAG_TO_FILETIME, /**< FILETIME から FILETIME へ変換 */
	CONVERT_TIME_FLAG_OFFSET        = 0x00000004U  /**< オフセットの指定 */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(ConvertTimeFlag)

/**
@brief 日時変換の種類
*/
TVTEST_DEFINE_ENUM(ConvertTimeType, DWORD) {
	CONVERT_TIME_TYPE_UTC,         /**< UTC */
	CONVERT_TIME_TYPE_LOCAL,       /**< ローカル */
	CONVERT_TIME_TYPE_EPG,         /**< EPG 日時(UTC+9) */
	CONVERT_TIME_TYPE_EPG_DISPLAY  /**< EPG の表示用(変換先としてのみ指定可能) */
};

/**
@brief 日時変換の情報
*/
struct ConvertTimeInfo
{
	DWORD Size;               /**< 構造体のサイズ */
	ConvertTimeFlag Flags;    /**< 各種フラグ(CONVERT_TIME_FLAG_*) */
	ConvertTimeType TypeFrom; /**< 変換元の種類(CONVERT_TIME_TYPE_*) */
	ConvertTimeType TypeTo;   /**< 変換先の種類(CONVERT_TIME_TYPE_*) */
	TimeUnion From;           /**< 変換元の日時 */
	TimeUnion To;             /**< 変換先の日時 */
	LONGLONG Offset;          /**< オフセットms(ConvertTimeInfo::Flags に TVTest::CONVERT_TIME_FLAG_OFFSET が指定されている場合のみ) */
};

/**
@brief 日時を変換する

@param[in] pParam プラグインパラメータ
@param[in,out] pInfo 日時変換の情報を格納した ConvertTimeInfo 構造体へのポインタ。
                     ConvertTimeInfo::Flags に TVTest::CONVERT_TIME_FLAG_OFFSET を指定すると、 ConvertTimeInfo::Offset の時間が変換結果に加算されます。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_CONVERTTIME
*/
inline bool MsgConvertTime(PluginParam *pParam, ConvertTimeInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_CONVERTTIME, reinterpret_cast<LPARAM>(pInfo), 0) != FALSE;
}

/**
@brief EPG 日時を変換する

@param[in] pParam プラグインパラメータ
@param[in] EpgTime 変換元の日時
@param[in] Type 変換先の種類(CONVERT_TIME_TYPE_*)
@param[out] pDstTime 変換した日時の情報を格納する SYSTEMTIME 構造体へのポインタ

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_CONVERTTIME
*/
inline bool MsgConvertEpgTimeTo(
	PluginParam *pParam, const SYSTEMTIME &EpgTime, ConvertTimeType Type, SYSTEMTIME *pDstTime)
{
	ConvertTimeInfo Info;
	Info.Size = sizeof(ConvertTimeInfo);
	Info.Flags = CONVERT_TIME_FLAG_NONE;
	Info.TypeFrom = CONVERT_TIME_TYPE_EPG;
	Info.TypeTo = Type;
	Info.From.SystemTime = EpgTime;
	if (!MsgConvertTime(pParam, &Info))
		return false;
	*pDstTime = Info.To.SystemTime;
	return true;
}

/**
@brief 映像ストリームのコールバック関数

Format はストリームのコーデックの FourCC で、現在以下のいずれかです。

+ FCC('mp2v') MPEG-2 Video
+ FCC('H264') H.264
+ FCC('H265') H.265

渡されたデータを加工することはできません。
戻り値は今のところ常に 0 を返します。
*/
typedef LRESULT (CALLBACK *VideoStreamCallbackFunc)(
	DWORD Format, const void *pData, SIZE_T Size, void *pClientData);

/**
@brief 映像ストリームを取得するコールバック関数を設定する

@param[in] pParam プラグインパラメータ
@param[in] pCallback コールバック関数。nullptr を指定すると、設定が解除されます。
@param[in] pClientData コールバック関数に渡すデータ

@retval true 正常終了
@retval false エラー発生

@note 一つのプラグインで設定できるコールバック関数は一つだけです。
      2回目以降の呼び出しでは、前回の設定が上書きされます。

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_SETVIDEOSTREAMCALLBACK
*/
inline bool MsgSetVideoStreamCallback(PluginParam *pParam, VideoStreamCallbackFunc pCallback, void *pClientData = nullptr)
{
	return (*pParam->Callback)(pParam, MESSAGE_SETVIDEOSTREAMCALLBACK, reinterpret_cast<LPARAM>(pCallback), reinterpret_cast<LPARAM>(pClientData)) != 0;
}

/**
@brief 変数文字列のコンテキスト

MsgFormatVarString() の説明を参照してください。
*/
struct VarStringContext;

/**
@brief 変数文字列のコンテキストを取得

@param[in] pParam プラグインパラメータ

@return コンテキストを表す VarStringContext 構造体へのポインタ

@note 取得したコンテキストは MsgFreeVarStringContext() で解放します。

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_GETVARSTRINGCONTEXT
*/
inline VarStringContext *MsgGetVarStringContext(PluginParam *pParam)
{
	return reinterpret_cast<VarStringContext*>((*pParam->Callback)(pParam, MESSAGE_GETVARSTRINGCONTEXT, 0, 0));
}

/**
@brief 変数文字列のコンテキストを解放

MsgGetVarStringContext() で取得したコンテキストを解放します。

@param[in] pParam プラグインパラメータ
@param[in] pContext 解放する VarStringContext 構造体へのポインタ

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_FREEVARSTRINGCONTEXT
*/
inline void MsgFreeVarStringContext(PluginParam *pParam, VarStringContext *pContext)
{
	(*pParam->Callback)(pParam, MESSAGE_FREEVARSTRINGCONTEXT, reinterpret_cast<LPARAM>(pContext), 0);
}

/**
@brief 変数文字列のマップ関数
*/
typedef BOOL (CALLBACK *VarStringMapFunc)(LPCWSTR pszVar, LPWSTR *ppszString, void *pClientData);

/**
@brief 変数文字列のフォーマットフラグ
*/
TVTEST_DEFINE_ENUM(VarStringFormatFlag, DWORD) {
	VAR_STRING_FORMAT_FLAG_NONE     = 0x00000000U, /**< なし(0) */
	VAR_STRING_FORMAT_FLAG_FILENAME = 0x00000001U  /**< ファイル名用(ファイル名に使えない文字が全角になる) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(VarStringFormatFlag)

/**
@brief 変数文字列のフォーマット情報
*/
struct VarStringFormatInfo
{
	DWORD Size;                       /**< 構造体のサイズ */
	VarStringFormatFlag Flags;        /**< 各種フラグ(VAR_STRING_FORMAT_FLAG_*) */
	LPCWSTR pszFormat;                /**< フォーマット文字列 */
	const VarStringContext *pContext; /**< コンテキスト(nullptr で現在のコンテキスト) */
	VarStringMapFunc pMapFunc;        /**< マップ関数(必要なければ nullptr) */
	void *pClientData;                /**< マップ関数に渡す任意データ */
	LPWSTR pszResult;                 /**< 変換結果の文字列 */
};

/**
@brief 変数文字列を使って文字列をフォーマット

変数文字列は、"%event-name%" などの変数が含まれた文字列です。
このような文字列の変数を展開した文字列を取得できます。
VarStringFormatInfo::pszResult に結果の文字列が返されるので、
不要になったら MsgMemoryFree() で解放します。
変数の展開に必要な、現在の番組や日時などの情報をコンテキストと呼びます。
MsgGetVarStringContext() で、その時点のコンテキストを取得できます。

@param[in] pParam プラグインパラメータ
@param[in,out] pInfo 変数文字列のフォーマットの情報を格納した VarStringFormatInfo 構造体へのポインタ。
                     VarStringFormatInfo::pszResult に結果の文字列が返されるので、不要になったら MsgMemoryFree() で解放します。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_FORMATVARSTRING

@par Example
@code{.cpp}
	// 現在のコンテキストを取得
	// (ここでは例のために取得していますが、現在の情報を使うのであれば
	//  VarStringFormatInfo.pContext を nullptr にすればよいです)
	TVTest::VarStringContext *pContext = TVTest::MsgGetVarStringContext(pParam);

	// 文字列をフォーマットする
	TVTest::VarStringFormatInfo Info;
	Info.Size = sizeof(Info);
	Info.Flags = TVTest::VAR_STRING_FORMAT_FLAG_NONE;
	Info.pszFormat = L"%event-name% %tot-date% %tot-time%";
	Info.pContext = pContext;
	Info.pMapFunc = nullptr;
	Info.pClientData = nullptr;

	if (TVTest::MsgFormatVarString(pParam, &Info)) {
		// Info.pszResult に結果の文字列が返される
		...
		// 不要になったらメモリを解放する
		TVTest::MsgMemoryFree(pParam, Info.pszResult);
	}

	// 不要になったらコンテキストを解放する
	TVTest::MsgFreeVarStringContext(pParam, pContext);
@endcode
@par
マップ関数を指定すると、任意の変数を文字列に変換できます。
上記の例でマップ関数を使う場合は以下のようになります。
@par
@code{.cpp}
	// マップ関数の定義
	BOOL CALLBACK VarStringMap(LPCWSTR pszVar, LPWSTR *ppszString, void *pClientData)
	{
		TVTest::PluginParam *pParam = static_cast<TVTest::PluginParam*>(pClientData);
		if (::lstrcmpiW(pszVar, L"my-var") == 0) {
			LPCWSTR pszMapString = L"replaced string"; // 置き換える文字列
			*ppszString = TVTest::MsgStringDuplicate(pParam, pszMapString);
			return TRUE;
		}
		return FALSE;
	}
@endcode
@code{.cpp}
	// VarStringFormatInfo にマップ関数と、マップ関数の pClientData 引数に渡すデータを指定する
	Info.pMapFunc = VarStringMap;
	Info.pClientData = pParam;
@endcode
*/
inline bool MsgFormatVarString(PluginParam *pParam, VarStringFormatInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_FORMATVARSTRING, reinterpret_cast<LPARAM>(pInfo), 0) != FALSE;
}

/**
@brief 変数登録のフラグ
*/
TVTEST_DEFINE_ENUM(RegisterVariableFlag, DWORD) {
	REGISTER_VARIABLE_FLAG_NONE     = 0x00000000U, /**< なし(0) */
	REGISTER_VARIABLE_FLAG_OVERRIDE = 0x00000001U  /**< デフォルトの変数を上書き */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(RegisterVariableFlag)

/**
@brief 変数登録の情報
*/
struct RegisterVariableInfo
{
	DWORD Size;                 /**< 構造体のサイズ */
	RegisterVariableFlag Flags; /**< フラグ(REGISTER_VARIABLE_FLAG_*) */
	LPCWSTR pszKeyword;         /**< 識別子 */
	LPCWSTR pszDescription;     /**< 説明文 */
	LPCWSTR pszValue;           /**< 変数の値(nullptr で動的に取得) */
};

/**
@brief 変数取得の情報

TVTest::EVENT_GETVARIABLE で渡されます。
*/
struct GetVariableInfo
 {
	LPCWSTR pszKeyword; /**< 識別子 */
	LPWSTR pszValue;    /**< 値 */
};

/**
@brief 変数を登録

変数を登録すると、TVTest の各所の変数文字列で利用できるようになります。
変数の識別子は半角のアルファベットと数字、-記号のみ使用してください。
TVTest で既に定義されている識別子と同じものを指定した場合、
RegisterVariableInfo::Flags に TVTest::REGISTER_VARIABLE_FLAG_OVERRIDE が設定されている場合は
プラグインで登録されたものが優先され、そうでない場合は TVTest での定義が優先されます。
同じ識別子の変数を再登録すると、値が更新されます。

@param[in] pParam プラグインパラメータ
@param[in] pInfo 変数登録の情報を格納した RegisterVariableInfo 構造体へのポインタ

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.14

@par Corresponding message
TVTest::MESSAGE_REGISTERVARIABLE

@par Example
@code{.cpp}
	// 変数 lucky-number を登録します。
	// 変数文字列の中で "%lucky-number%" を使うと、"777" に置き換えられます。
	TVTest::RegisterVariableInfo Info;
	Info.Size           = sizeof(Info);
	Info.Flags          = TVTest::REGISTER_VARIABLE_FLAG_NONE;
	Info.pszKeyword     = L"lucky-number";
	Info.pszDescription = L"幸運の番号";
	Info.pszValue       = L"777";
	TVTest::MsgRegisterVariable(pParam, &Info);
@endcode
@par
変数の値が頻繁に変わるような場合は、動的に取得されるようにします。
RegisterVariableInfo::pszValue に nullptr を指定すると、変数が必要になった段階で
TVTest::EVENT_GETVARIABLE が呼ばれるので、そこで値を返します。
@par
@code{.cpp}
	// 変数の値が動的に取得されるようにする例
	TVTest::RegisterVariableInfo Info;
	Info.Size           = sizeof(Info);
	Info.Flags          = TVTest::REGISTER_VARIABLE_FLAG_NONE;
	Info.pszKeyword     = L"tick-count";
	Info.pszDescription = L"Tick count";
	Info.pszValue       = nullptr;
	TVTest::MsgRegisterVariable(pParam, &Info);
@endcode
@code{.cpp}
	// EVENT_GETVARIABLE で値を返します。
	bool OnGetVariable(TVTest::GetVariableInfo *pInfo)
	{
		if (lstrcmpiW(pInfo->pszKeyword, L"tick-count") == 0) {
			// GetTickCount() の値を返す
			WCHAR szValue[16];
			wsprintf(szValue, L"%u", GetTickCount());
			pInfo->pszValue = TVTest::MsgStringDuplicate(pParam, szValue);
			return true;
		}
		return false;
	}
@endcode
*/
inline bool MsgRegisterVariable(PluginParam *pParam, const RegisterVariableInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_REGISTERVARIABLE, reinterpret_cast<LPARAM>(pInfo), 0) != FALSE;
}

#endif // TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 14)

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 15)

/**
@brief ダークモードの状態フラグ
*/
TVTEST_DEFINE_ENUM(DarkModeStatus, DWORD) {
	DARK_MODE_STATUS_NONE              = 0x00000000U, /**< なし(0) */
	DARK_MODE_STATUS_APP_SUPPORTED     = 0x00000001U, /**< アプリケーションがダークモードに対応している */
	DARK_MODE_STATUS_MENU_DARK         = 0x00000002U, /**< メニューがダークモード */
	DARK_MODE_STATUS_PANEL_SUPPORTED   = 0x00000004U, /**< パネルがダークモードに対応している */
	DARK_MODE_STATUS_MAINWINDOW_DARK   = 0x00000008U, /**< メインウィンドウがダークモード */
	DARK_MODE_STATUS_PROGRAMGUIDE_DARK = 0x00000010U, /**< 番組表がダークモード */
	DARK_MODE_STATUS_DIALOG_DARK       = 0x00000020U  /**< ダイアログがダークモード */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(DarkModeStatus)

/**
@brief ダークモードの状態を取得

@param[in] pParam プラグインパラメータ

@return ダークモードの状態フラグ DARK_MODE_STATUS_* の組み合わせ

@since ver.0.0.15

@par Corresponding message
TVTest::MESSAGE_GETDARKMODESTATUS

@par Example
MsgSetWindowDarkMode() の例を参照してください。
*/
inline DarkModeStatus MsgGetDarkModeStatus(PluginParam *pParam)
{
	return static_cast<DarkModeStatus>((*pParam->Callback)(pParam, MESSAGE_GETDARKMODESTATUS, 0, 0));
}

/**
@brief ダークモードの色かを取得

指定された色がダークモードの色であるかを取得します。
一般に MsgGetColor() で取得した色に対して使用します。

@param[in] pParam プラグインパラメータ
@param[in] Color 色

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.15

@par Corresponding message
TVTest::MESSAGE_ISDARKMODECOLOR

@par Example
MsgSetWindowDarkMode() の例を参照してください。
*/
inline bool MsgIsDarkModeColor(PluginParam *pParam, COLORREF Color)
{
	return (*pParam->Callback)(pParam, MESSAGE_ISDARKMODECOLOR, Color, 0) != FALSE;
}

/**
@brief ウィンドウをダークモードにする

ウィンドウのスクロールバーなどがダークモードになります。
トップレベルウィンドウであれば、タイトルバーがダークモードになります。

@param[in] pParam プラグインパラメータ
@param[in] hwnd ウィンドウのハンドル
@param[in] fDark ダークモードにする場合は true、ダークモードを解除する場合は false

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.15

@par Corresponding message
TVTest::MESSAGE_SETWINDOWDARKMODE

@par Example
@code{.cpp}
	// パネルのウィンドウをダークモードにする
	void SetPanelWindowDarkMode(PluginParam *pParam, HWND hwnd)
	{
		if (TVTest::MsgGetDarkModeStatus(pParam) & TVTest::DARK_MODE_STATUS_PANEL_SUPPORTED)
			TVTest::MsgSetWindowDarkMode(pParam, hwnd, TVTest::MsgIsDarkModeColor(pParam, TVTest::MsgGetColor(pParam, L"PanelBack")));
	}
@endcode
*/
inline bool MsgSetWindowDarkMode(PluginParam *pParam, HWND hwnd, bool fDark)
{
	return (*pParam->Callback)(pParam, MESSAGE_SETWINDOWDARKMODE, reinterpret_cast<LPARAM>(hwnd), fDark) != FALSE;
}

/**
@brief Elementary Stream (ES) の情報
*/
struct ElementaryStreamInfo
{
	WORD PID;                      /**< PID */
	WORD HierarchicalReferencePID; /**< 階層変調の参照先 PID (参照先が無い場合は 0x1FFF、階層伝送記述子が無い場合は 0xFFFF) */
	BYTE StreamType;               /**< ストリームの種類(stream_type) */
	BYTE ComponentTag;             /**< コンポーネントタグ(component_tag) */
	BYTE QualityLevel;             /**< 階層(0 = 低階層 / 1 = 高階層 / 0xFF = 階層伝送記述子が無い) */
	BYTE Reserved;                 /**< 予約領域(現在は常に 0) */
};

/**
@brief Elementary Stream (ES) のメディアの種類
*/
enum ElementaryStreamMediaType {
	ES_MEDIA_ALL,           /**< 全て */
	ES_MEDIA_VIDEO,         /**< 映像 */
	ES_MEDIA_AUDIO,         /**< 音声 */
	ES_MEDIA_CAPTION,       /**< 字幕 */
	ES_MEDIA_DATA_CARROUSEL /**< データ放送 */
};

/**
@brief Elementary Stream (ES) の情報フラグ
*/
TVTEST_DEFINE_ENUM(ElementaryStreamInfoFlag, DWORD) {
	ES_INFO_FLAG_NONE = 0x000U /**< なし(0) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(ElementaryStreamInfoFlag)

/**
@brief Elementary Stream (ES) の情報のリスト
*/
struct ElementaryStreamInfoList
{
	DWORD Size;                      /**< 構造体のサイズ */
	ElementaryStreamMediaType Media; /**< メディアの種類 */
	ElementaryStreamInfoFlag Flags;  /**< フラグ(現在は常に TVTest::ES_INFO_FLAG_NONE) */
	WORD ServiceID;                  /**< サービス ID(0 で現在のサービス) */
	WORD ESCount;                    /**< ストリームの数 */
	ElementaryStreamInfo *ESList;    /**< ストリームの情報(ElementaryStreamInfoList::ESCount 分の情報が入る) */
};

/**
@brief Elementary Stream (ES) の情報のリストを取得

@param[in] pParam プラグインパラメータ
@param[in,out] pList ES の情報のリストを取得する ElementaryStreamInfoList 構造体へのポインタ。
                     ElementaryStreamInfoList::Size / ElementaryStreamInfoList::Media / ElementaryStreamInfoList::Flags / ElementaryStreamInfoList::ServiceID メンバを設定して呼び出します。
                     ElementaryStreamInfoList::ESCount メンバにストリームの数が、 ElementaryStreamInfoList::ESList メンバに ElementaryStreamInfoList::ESCount の数分だけストリームの情報が返されます。
                     不要になったら ElementaryStreamInfoList::ESList メンバのメモリを MsgMemoryFree() で解放します。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.15

@par Corresponding message
TVTest::MESSAGE_GETELEMENTARYSTREAMINFOLIST

@par Example
@code{.cpp}
	// 音声 ES の情報を取得する
	TVTest::ElementaryStreamInfoList List;
	List.Size = sizeof(List);
	List.Media = TVTest::ES_MEDIA_AUDIO;
	List.Flags = TVTest::ES_INFO_FLAG_NONE;
	List.ServiceID = 0;
	if (TVTest::MsgGetElementaryStreamInfoList(pParam, &List)) {
		// ESList に ESCount の要素数分の情報が取得される
		for (WORD i = 0; i < List.ESCount; i++) {
			const TVTest::ElementaryStreamInfo &ESInfo = List.ESList[i];
			...
		}

		// 不要になったら ESList のメモリを解放する
		TVTest::MsgMemoryFree(pParam, List.ESList);
	}
@endcode
*/
inline bool MsgGetElementaryStreamInfoList(PluginParam *pParam, ElementaryStreamInfoList *pList)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETELEMENTARYSTREAMINFOLIST, reinterpret_cast<LPARAM>(pList), 0) != FALSE;
}

/**
@brief 全てのサービス数を取得

現在のストリームのサービス数を取得します。
この数は PAT に含まれる全てのサービスの数で、試聴できないサービスも含まれます。
試聴できるサービスの数を取得する場合は MsgGetService() を使用します。

@param[in] pParam プラグインパラメータ

@return サービスの数

@since ver.0.0.15

@par Corresponding message
TVTest::MESSAGE_GETSERVICECOUNT
*/
inline int MsgGetServiceCount(PluginParam *pParam)
{
	return static_cast<int>((*pParam->Callback)(pParam, MESSAGE_GETSERVICECOUNT, 0, 0));
}

/**
@brief サービス情報のフラグ
*/
TVTEST_DEFINE_ENUM(ServiceInfo2Flag, DWORD) {
	SERVICE_INFO2_FLAG_NONE                = 0x00000000U, /**< なし(0) */
	SERVICE_INFO2_FLAG_BY_ID               = 0x00000001U, /**< サービス ID から情報を取得 */
	SERVICE_INFO2_FLAG_BY_SELECTABLE_INDEX = 0x00000002U  /**< 試聴可能なサービスの中でのインデックスから情報を取得 */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(ServiceInfo2Flag)

/**
@brief サービス情報のステータスフラグ
*/
TVTEST_DEFINE_ENUM(ServiceInfo2Status, DWORD) {
	SERVICE_INFO2_STATUS_NONE         = 0x00000000U, /**< なし(0) */
	SERVICE_INFO2_STATUS_FREE_CA_MODE = 0x00000001U  /**< 有料(free_CA_mode) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(ServiceInfo2Status)

/**
@brief サービス情報

@sa ServiceInfo
*/
struct ServiceInfo2
{
	DWORD Size;                /**< 構造体のサイズ */
	ServiceInfo2Flag Flags;    /**< フラグ(SERVICE_INFO2_FLAG_*) */
	ServiceInfo2Status Status; /**< ステータス(SERVICE_INFO2_STATUS_*) */
	WORD NetworkID;            /**< ネットワーク ID */
	WORD TransportStreamID;    /**< トランスポートストリーム ID */
	WORD ServiceID;            /**< サービス ID */
	BYTE ServiceType;          /**< サービス形式種別 */
	BYTE Reserved;             /**< 予約(現在は常に 0) */
	WORD PMT_PID;              /**< PMT の PID (無効な場合 0xFFFF) */
	WORD PCR_PID;              /**< PCR の PID (無効な場合 0xFFFF) */
	WCHAR szServiceName[32];   /**< サービス名 */
	WCHAR szProviderName[32];  /**< 事業者名 */
};

/**
@brief サービスの情報を取得する

現在のストリームのサービスの情報を取得します。

@param[in] pParam プラグインパラメータ
@param[in] Service サービスの指定。
                   ServiceInfo2::Flags に TVTest::SERVICE_INFO2_FLAG_BY_ID が設定されている場合、サービス ID を指定します。
                   それ以外の場合、インデックスを指定します。
                   -1 を指定した場合、現在選択されているサービスの情報が取得されます。
@param[in,out] pInfo サービスの情報を取得する ServiceInfo2 構造体へのポインタ。
                     事前に ServiceInfo2::Size と ServiceInfo2::Flags メンバを設定しておきます。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.15

@par Corresponding message
TVTest::MESSAGE_GETSERVICEINFO2

@sa MsgGetServiceInfo()
*/
inline bool MsgGetServiceInfo2(PluginParam *pParam, int Service, ServiceInfo2 *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETSERVICEINFO2, Service, reinterpret_cast<LPARAM>(pInfo)) != 0;
}

/**
@brief サービス情報のリストのフラグ
*/
TVTEST_DEFINE_ENUM(ServiceInfoListFlag, DWORD) {
	SERVICE_INFO_LIST_FLAG_NONE            = 0x00000000U, /**< なし(0) */
	SERVICE_INFO_LIST_FLAG_SELECTABLE_ONLY = 0x00000001U, /**< 試聴可能なサービスのみ取得 */
	SERVICE_INFO_LIST_FLAG_SDT_ACTUAL      = 0x00000002U  /**< SDT(actual) のサービス情報を取得 */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(ServiceInfoListFlag)

/**
@brief サービス情報のリスト
*/
struct ServiceInfoList
{
	DWORD Size;                /**< 構造体のサイズ */
	ServiceInfoListFlag Flags; /**< フラグ(SERVICE_INFO_LIST_FLAG_*) */
	DWORD Reserved;            /**< 予約(現在は常に 0) */
	DWORD ServiceCount;        /**< サービスの数 */
	ServiceInfo2 *ServiceList; /**< サービスの情報(ServiceInfoList::ServiceCount 分の情報が入る) */
};

/**
@brief サービスの情報のリストを取得

@param[in] pParam プラグインパラメータ
@param[in,out] pList サービス情報のリストを取得する ServiceInfoList 構造体へのポインタ。
                     ServiceInfoList::Size と ServiceInfoList::Flags メンバを設定して呼び出します。
                     ServiceInfoList::ServiceCount メンバにサービスの数が、 ServiceInfoList::ServiceList メンバに ServiceInfoList::ServiceCount の数分だけサービスの情報が返されます。
                     不要になったら ServiceInfoList::ServiceList メンバのメモリを MsgMemoryFree() で解放します。
                     ServiceInfoList::Flags に TVTest::SERVICE_INFO_LIST_FLAG_SDT_ACTUAL が指定されている場合、 ServiceInfo2::PMT_PID と ServiceInfo2::PCR_PID は取得されません。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.15

@par Corresponding message
TVTest::MESSAGE_GETSERVICEINFOLIST

@par Example
@code{.cpp}
	// 全てのサービスの情報を取得する
	TVTest::ServiceInfoList List;
	List.Size = sizeof(List);
	List.Flags = TVTest::SERVICE_INFO_LIST_FLAG_NONE;
	if (TVTest::MsgGetServiceInfoList(pParam, &List)) {
		// SrviceList に ServiceCount の数分の情報が取得される
		for (DWORD i = 0; i < List.ServiceCount; i++) {
			const TVTest::ServiceInfo2 &Info = List.ServiceList[i];
			...
		}

		// 不要になったら ServiceList のメモリを解放する
		TVTest::MsgMemoryFree(pParam, List.ServiceList);
	}
@endcode
*/
inline bool MsgGetServiceInfoList(PluginParam *pParam, ServiceInfoList *pList)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETSERVICEINFOLIST, reinterpret_cast<LPARAM>(pList), 0) != FALSE;
}

/**
@brief 音声情報の状態フラグ
*/
TVTEST_DEFINE_ENUM(AudioInfoStatus, DWORD) {
	AUDIO_INFO_STATUS_NONE       = 0x00000000U, /**< なし(0) */
	AUDIO_INFO_STATUS_DUAL_MONO  = 0x00000001U, /**< デュアルモノラル */
	AUDIO_INFO_STATUS_SPDIF      = 0x00000002U, /**< S/PDIF パススルー */
	AUDIO_INFO_STATUS_LEFT_ONLY  = 0x00000004U, /**< 左チャンネルのみ出力(主にデュアルモノラル時) */
	AUDIO_INFO_STATUS_RIGHT_ONLY = 0x00000008U  /**< 右チャンネルのみ出力(主にデュアルモノラル時) */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(AudioInfoStatus)

/**
@brief 音声の情報
*/
struct AudioInfo
{
	DWORD Size;                /**< 構造体のサイズ */
	AudioInfoStatus Status;    /**< 状態フラグ(AUDIO_INFO_STATUS_*) */
	DWORD Frequency;           /**< サンプリング周波数(Hz) */
	BYTE OriginalChannelCount; /**< 元のチャンネル数 */
	BYTE OutputChannelCount;   /**< 出力チャンネル数(出力されていなければ 0) */
	WORD Reserved;             /**< 予約(現在は常に 0) */
};

/**
@brief 音声の情報を取得

@param[in] pParam プラグインパラメータ
@param[in,out] pInfo 音声の情報を取得する AudioInfo 構造体へのポインタ。
                     事前に AudioInfo::Size メンバを設定して呼び出します。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.15

@par Corresponding message
TVTest::MESSAGE_GETAUDIOINFO
*/
inline bool MsgGetAudioInfo(PluginParam *pParam, AudioInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETAUDIOINFO, reinterpret_cast<LPARAM>(pInfo), 0) != FALSE;
}

/**
@brief Elementary Stream (ES) の数を取得

@param[in] pParam プラグインパラメータ
@param[in] Media 数を取得する ES の種類
@param[in] ServiceID サービス ID。0 の場合、現在のサービス。

@return ES の数

@since ver.0.0.15

@par Corresponding message
TVTest::MESSAGE_GETELEMENTARYSTREAMCOUNT
*/
inline int MsgGetElementaryStreamCount(PluginParam *pParam, ElementaryStreamMediaType Media, WORD ServiceID = 0)
{
	return static_cast<int>((*pParam->Callback)(pParam, MESSAGE_GETELEMENTARYSTREAMCOUNT, static_cast<LPARAM>(Media), ServiceID));
}

/**
@brief 音声ストリームの数を取得

@param[in] pParam プラグインパラメータ

@return 音声ストリームの数

@since ver.0.0.15

@par Corresponding message
TVTest::MESSAGE_GETELEMENTARYSTREAMCOUNT
*/
inline int MsgGetAudioStreamCount(PluginParam *pParam)
{
	return MsgGetElementaryStreamCount(pParam, ES_MEDIA_AUDIO);
}

/**
@brief デュアルモノラルのチャンネル
*/
enum DualMonoChannel {
	DUAL_MONO_CHANNEL_INVALID, /**< 無効 */
	DUAL_MONO_CHANNEL_MAIN,    /**< 主音声(左) */
	DUAL_MONO_CHANNEL_SUB,     /**< 副音声(右) */
	DUAL_MONO_CHANNEL_BOTH     /**< 主+副音声(左右) */
};

/**
@brief 音声選択のフラグ
*/
TVTEST_DEFINE_ENUM(AudioSelectFlag, DWORD) {
	AUDIO_SELECT_FLAG_NONE          = 0x00000000U, /**< なし(0) */
	AUDIO_SELECT_FLAG_COMPONENT_TAG = 0x00000001U, /**< コンポーネントタグ(component_tag)で選択する */
	AUDIO_SELECT_FLAG_DUAL_MONO     = 0x00000002U  /**< DualMono メンバが有効 */
};
TVTEST_DEFINE_ENUM_FLAG_OPERATORS(AudioSelectFlag)

/**
@brief 音声選択の情報
*/
struct AudioSelectInfo
{
	DWORD Size;               /**< 構造体のサイズ */
	AudioSelectFlag Flags;    /**< フラグ(AUDIO_SELECT_FLAG_*) */
	int Index;                /**< インデックス */
	BYTE ComponentTag;        /**< コンポーネントタグ(component_tag) */
	BYTE Reserved[3];         /**< 予約 */
	DualMonoChannel DualMono; /**< デュアルモノラルのチャンネル */
};

/**
@brief 音声を選択する

@param[in] pParam プラグインパラメータ
@param[in] pInfo 音声選択の情報を格納した AudioSelectInfo 構造体へのポインタ。
                 AudioSelectInfo::Flags に TVTest::AUDIO_SELECT_FLAG_COMPONENT_TAG が指定されている場合、 AudioSelectInfo::ComponentTag メンバでコンポーネントタグを指定します。
                 それ以外の場合、AudioSelectInfo::Index メンバに音声ストリームのインデックスを指定します。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.15

@par Corresponding message
TVTest::MESSAGE_SELECTAUDIO
*/
inline bool MsgSelectAudio(PluginParam *pParam, const AudioSelectInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_SELECTAUDIO, reinterpret_cast<LPARAM>(pInfo), 0) != FALSE;
}

/**
@brief 選択された音声を取得する

@param[in] pParam プラグインパラメータ
@param[in,out] pInfo 音声選択の情報を取得する AudioSelectInfo 構造体へのポインタ。
                     事前に AudioSelectInfo::Size と AudioSelectInfo::Flags メンバを設定して呼び出します。
                     AudioSelectInfo::Flags は今のところ常に TVTest::AUDIO_SELECT_FLAG_NONE に設定します。

@retval true 正常終了
@retval false エラー発生

@since ver.0.0.15

@par Corresponding message
TVTest::MESSAGE_GETSELECTEDAUDIO
*/
inline bool MsgGetSelectedAudio(PluginParam *pParam, AudioSelectInfo *pInfo)
{
	return (*pParam->Callback)(pParam, MESSAGE_GETSELECTEDAUDIO, reinterpret_cast<LPARAM>(pInfo), 0) != FALSE;
}

/**
@brief 現在の番組情報を取得する

@param[in] pParam プラグインパラメータ
@param[in] ServiceID サービス ID。0 を指定すると、現在のサービスの情報が取得されます。
@param[in] fNext 現在の番組の情報を取得する場合は true、次の番組の情報を取得する場合は false を指定します。

@return 現在の番組情報を格納した EpgEventInfo 構造体へのポインタ。
        情報が取得できなかった場合は nullptr が返ります。
        取得した情報が不要になった場合、 MsgFreeEpgEventInfo() で解放します。

@since ver.0.0.15

@par Corresponding message
TVTest::MESSAGE_GETCURRENTEPGEVENTINFO

@sa MsgGetCurrentProgramInfo()
*/
inline EpgEventInfo *MsgGetCurrentEpgEventInfo(PluginParam *pParam, WORD ServiceID = 0, bool fNext = false)
{
	return reinterpret_cast<EpgEventInfo*>((*pParam->Callback)(pParam, MESSAGE_GETCURRENTEPGEVENTINFO, ServiceID, fNext));
}

#endif // TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 15)

/**
@brief TVTest アプリケーションクラス

TVTest の各種機能を呼び出すためのクラスです。
単なるラッパーなので必ずしも使う必要はありません。

TVTInitialize() 関数が呼ばれた時に、引数の PluginParam 構造体へのポインタをコンストラクタに渡してインスタンスを生成します。

TVTEST_PLUGIN_CLASS_IMPLEMENT シンボルが \#define されている場合は、 CTVTestPlugin::m_pApp メンバにインスタンスが生成されます。
*/
class CTVTestApp
{
protected:
	PluginParam *m_pParam; /**< プラグインパラメータ */

public:
	/**
	@brief コンストラクタ

	@param[in] pParam プラグインパラメータ
	*/
	CTVTestApp(PluginParam *pParam) : m_pParam(pParam) {}

	/**
	@brief デストラクタ
	*/
	virtual ~CTVTestApp() {}

	/**
	@brief メインウィンドウのハンドルを取得

	@return メインウィンドウのハンドル
	*/
	HWND GetAppWindow()
	{
		return m_pParam->hwndApp;
	}

	/**
	@brief プログラム(TVTest)のバージョンを取得する

	@return TVTest のバージョン。
	        上位8ビットがメジャーバージョン、次の12ビットがマイナーバージョン、下位12ビットがビルドナンバー。
	        それぞれの値は GetMajorVersion() / GetMinorVersion() / GetBuildVersion() を使って取得できます。

	@remark より詳しいプログラムの情報を GetHostInfo() で取得できます。

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_GETVERSION

	@sa MakeVersion()
	@sa GetMajorVersion()
	@sa GetMinorVersion()
	@sa GetBuildVersion()
	*/
	DWORD GetVersion()
	{
		return MsgGetVersion(m_pParam);
	}

	/**
	@brief 指定されたメッセージに対応しているか問い合わせる

	TVTest が指定されたメッセージコードに対応しているかを問い合わせます。

	@param[in] Message 問い合わせるメッセージのコード(MESSAGE_*)

	@retval true メッセージに対応している
	@retval false メッセージに対応していない

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_QUERYMESSAGE
	*/
	bool QueryMessage(MessageCode Message)
	{
		return MsgQueryMessage(m_pParam, Message);
	}

	/**
	@brief メモリ再確保

	@param[in] pData 再確保するデータのポインタ。nullptr で新しい領域を確保。
	@param[in] Size 確保するサイズ(バイト単位)。0 で領域を解放。

	@return 確保したメモリ領域へのポインタ。
	        Size 引数が 0 か、メモリが確保できなかった場合は nullptr。

	@note 確保したメモリは MemoryFree() で解放します。
	      または MemoryReAlloc() の Size 引数を 0 で呼び出すことでも解放できます。

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_MEMORYALLOC

	@sa MemoryAlloc()
	@sa MemoryFree()
	*/
	void *MemoryReAlloc(void *pData, DWORD Size)
	{
		return MsgMemoryReAlloc(m_pParam, pData, Size);
	}

	/**
	@brief メモリ確保

	@param[in] Size 確保するサイズ(バイト単位)

	@return 確保したメモリ領域へのポインタ。
	        Size 引数が 0 か、メモリが確保できなかった場合は nullptr。

	@note 確保したメモリは MemoryFree() で解放します。

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_MEMORYALLOC
	*/
	void *MemoryAlloc(DWORD Size)
	{
		return MsgMemoryAlloc(m_pParam, Size);
	}

	/**
	@brief メモリ解放

	MemoryAlloc() や MemoryReAlloc() などで確保されたメモリを解放します。

	@param[in] pData 解放するメモリ領域へのポインタ

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_MEMORYALLOC
	*/
	void MemoryFree(void *pData)
	{
		MsgMemoryFree(m_pParam, pData);
	}

	/**
	@brief 文字列を複製

	@param[in] pszString 複製する文字列へのポインタ

	@return 複製した文字列へのポインタ。
	        pszString が nullptr であるか、メモリが確保できなかった場合は nullptr。

	@note この関数は MsgMemoryAlloc() を使用してメモリを確保します。
	      複製した文字列は MemoryFree() で解放します。

	@since ver.0.0.0
	*/
	LPWSTR StringDuplicate(LPCWSTR pszString)
	{
		return MsgStringDuplicate(m_pParam, pszString);
	}

	/**
	@brief イベントハンドル用コールバックを設定

	@param[in] Callback コールバック関数。nullptr を渡すと設定が解除される。
	@param[in] pClientData コールバック関数に渡される値

	@retval true 正常に設定された
	@retval false エラーが発生した

	@note 一つのプラグインで設定できるコールバック関数は一つだけです。
	      2回目以降の呼び出しでは、前回の設定が上書きされます。

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_SETEVENTCALLBACK
	*/
	bool SetEventCallback(EventCallbackFunc Callback, void *pClientData = nullptr)
	{
		return MsgSetEventCallback(m_pParam, Callback, pClientData);
	}

	/**
	@brief 現在のチャンネルの情報を取得する

	@param[out] pInfo 情報を取得する ChannelInfo 構造体へのポインタ。

	@retval true 正常に取得された
	@retval false エラーが発生した

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_GETCURRENTCHANNELINFO
	*/
	bool GetCurrentChannelInfo(ChannelInfo *pInfo)
	{
		pInfo->Size = sizeof(ChannelInfo);
		return MsgGetCurrentChannelInfo(m_pParam, pInfo);
	}

#if TVTEST_PLUGIN_VERSION < TVTEST_PLUGIN_VERSION_(0, 0, 8)
	/**
	@brief チャンネルを設定する

	@param[in] Space チューニング空間のインデックス
	@param[in] Channel チャンネルのインデックス

	@retval true 正常に設定された
	@retval false エラーが発生した

	@remark 機能が追加された SelectChannel() もあります。

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_SETCHANNEL
	*/
	bool SetChannel(int Space, int Channel)
	{
		return MsgSetChannel(m_pParam, Space, Channel);
	}
#else
	/**
	@brief チャンネルを設定する

	@param[in] Space チューニング空間のインデックス
	@param[in] Channel チャンネルのインデックス
	@param[in] ServiceID サービス ID(ver.0.0.8 以降)。
	                     引数を省略するか 0 を指定した場合はデフォルトのサービス。

	@retval true 正常に設定された
	@retval false エラーが発生した

	@remark 機能が追加された SelectChannel() もあります。

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_SETCHANNEL
	*/
	bool SetChannel(int Space, int Channel, WORD ServiceID = 0)
	{
		return MsgSetChannel(m_pParam, Space, Channel, ServiceID);
	}
#endif

	/**
	@brief 現在のサービス及びサービス数を取得する

	@param[out] pNumServices 試聴可能なサービスの数を返す変数へのポインタ。不要な場合は nullptr を指定する。

	@return 現在のサービスのインデックス。エラー時は -1。

	@note 返されるインデックスは試聴可能なサービスの中での順序です。
	      全てのサービスの数を取得するには GetServiceCount() を使用します。

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_GETSERVICE
	*/
	int GetService(int *pNumServices = nullptr)
	{
		return MsgGetService(m_pParam, pNumServices);
	}

	/**
	@brief サービスを設定する

	@param[in] Service fByID = false の場合はインデックス、fByID = true の場合はサービス ID。
	                   インデックスは試聴可能なサービスの中での順序。
	@param[in] fByID true の場合 Serivce をサービス ID とみなし、false の場合インデックスとみなす

	@retval true 正常に設定された
	@retval false エラーが発生した

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_SETSERVICE
	*/
	bool SetService(int Service, bool fByID = false)
	{
		return MsgSetService(m_pParam, Service, fByID);
	}

	/**
	@brief チューニング空間名を取得する

	@param[in] Index チューニング空間のインデックス
	@param[out] pszName チューニング空間名を取得するバッファ。nullptr で長さの取得のみ行う。
	@param[in] MaxLength pszName の先に格納できる最大の要素数(終端の空文字を含む)

	@return チューニング空間名の長さ(終端の空文字は含まない)。
	        Index が範囲外の場合は 0。

	@note チューニング空間名の長さが MaxLength 以上である場合、pszName に格納される文字列は MaxLength - 1 の長さに切り詰められ、終端に空文字が付加されます。
	      その際も戻り値は切り捨てられる前の本来の長さです。

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_GETTUNINGSPACENAME
	*/
	int GetTuningSpaceName(int Index, LPWSTR pszName, int MaxLength)
	{
		return MsgGetTuningSpaceName(m_pParam, Index, pszName, MaxLength);
	}

	/**
	@brief チャンネルの情報を取得する

	@param[in] Space チューニング空間のインデックス
	@param[in] Index チャンネルのインデックス
	@param[out] pInfo 情報を取得する ChannelInfo 構造体へのポインタ。

	@retval true 正常に取得された
	@retval false エラーが発生した

	@note ChannelInfo::szNetworkName, ChannelInfo::szTransportStreamName は GetCurrentChannelInfo() でしか取得できません。
	      ChannelInfo::NetworkID, ChannelInfo::TransportStreamID はチャンネルスキャンしていないと取得できません。
	      取得できなかった場合は 0 になります。

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_GETCHANNELINFO
	*/
	bool GetChannelInfo(int Space, int Index, ChannelInfo *pInfo)
	{
		pInfo->Size = sizeof(ChannelInfo);
		return MsgGetChannelInfo(m_pParam, Space, Index, pInfo);
	}

	/**
	@brief サービスの情報を取得する

	現在のチャンネルのサービスの情報を取得します。
	引数 Index には試聴可能なサービスの中でのインデックスを指定します。
	試聴できないサービスも含めた全てのサービスの情報を取得するには GetServiceInfo2() を使用してください。
	このメッセージでは取得できる映像/音声/字幕ストリームの数に制限がありますので、
	全てのストリームの情報を取得するには GetElementaryStreamInfoList() を使用してください。

	@param[in] Index サービスのインデックス
	@param[out] pInfo サービスの情報を取得する ServiceInfo 構造体へのポインタ。

	@retval true 正常に取得された
	@retval false エラーが発生した

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_GETSERVICEINFO

	@sa GetServiceInfo2()
	*/
	bool GetServiceInfo(int Index, ServiceInfo *pInfo)
	{
		pInfo->Size = sizeof(ServiceInfo);
		return MsgGetServiceInfo(m_pParam, Index, pInfo);
	}

	/**
	@brief BonDriver のファイル名を取得する

	@param[out] pszName ファイル名を取得するバッファ。nullptr で長さの取得のみ行う。
	@param[in] MaxLength pszName の先に格納できる最大の要素数(終端の空文字を含む)

	@return ファイル名の長さ(終端の空文字を含まない)。
            BonDriver が読み込まれていない場合は 0。

	@note ファイル名の長さが MaxLength 以上である場合、pszName に格納される文字列は MaxLength - 1 の長さに切り詰められ、終端に空文字が付加されます。
	      その際も戻り値は切り捨てられる前の本来の長さです。
	@note 取得されるのは、ディレクトリを含まないファイル名のみか、相対パスの場合もあります。
	      フルパスを取得したい場合は GetDriverFullPathName() を使用してください。

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_GETDRIVERNAME
	*/
	int GetDriverName(LPWSTR pszName, int MaxLength)
	{
		return MsgGetDriverName(m_pParam, pszName, MaxLength);
	}

	/**
	@brief BonDriver を設定する

	@param[in] pszName BonDriver のファイル名。
	                   nullptr を指定すると、現在の BonDriver が解放されます(ver.0.0.14 以降)。

	@retval true 正常に設定された
	@retval false エラーが発生した

	@note ファイル名のみか相対パスを指定すると、BonDriver 検索フォルダの設定が使用されます。

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_SETDRIVERNAME
	*/
	bool SetDriverName(LPCWSTR pszName)
	{
		return MsgSetDriverName(m_pParam, pszName);
	}

	/**
	@brief 録画を開始する

	@param[in] pInfo 録画の情報。nullptr で即時録画開始。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_STARTRECORD
	*/
	bool StartRecord(RecordInfo *pInfo = nullptr)
	{
		if (pInfo != nullptr)
			pInfo->Size = sizeof(RecordInfo);
		return MsgStartRecord(m_pParam, pInfo);
	}

	/**
	@brief 録画を停止する

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_STOPRECORD
	*/
	bool StopRecord()
	{
		return MsgStopRecord(m_pParam);
	}

	/**
	@brief 録画を一時停止/再開する

	@param[in] fPause 一時停止する場合は true、再開する場合は false

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_PAUSERECORD
	*/
	bool PauseRecord(bool fPause = true)
	{
		return MsgPauseRecord(m_pParam, fPause);
	}

	/**
	@brief 録画設定を取得する

	@param[out] pInfo 録画設定の情報を取得する RecordInfo 構造体へのポインタ。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_GETRECORD
	*/
	bool GetRecord(RecordInfo *pInfo)
	{
		pInfo->Size = sizeof(RecordInfo);
		return MsgGetRecord(m_pParam, pInfo);
	}

	/**
	@brief 録画設定を変更する

	@param[in] pInfo 録画の情報

	@retval true 正常終了
	@retval false エラー発生

	@note 既に録画中である場合は、ファイル名と開始時間の指定は無視されます。

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_MODIFYRECORD
	*/
	bool ModifyRecord(RecordInfo *pInfo)
	{
		pInfo->Size = sizeof(RecordInfo);
		return MsgModifyRecord(m_pParam, pInfo);
	}

	/**
	@brief 表示倍率を取得する(% 単位)

	@return 表示倍率(% 単位)。再生が行われていない場合は 0。

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_GETZOOM
	*/
	int GetZoom()
	{
		return MsgGetZoom(m_pParam);
	}

	/**
	@brief 表示倍率を設定する

	@param[in] Num 表示倍率の分子。
	@param[in] Denom 表示倍率の分母。省略すると 100。

	@retval true 正常終了
	@retval false エラー発生

	@note % 単位だけではなく、 SetZoom(1, 3) などとして、割り切れない倍率を設定することもできます。

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_SETZOOM
	*/
	int SetZoom(int Num, int Denom = 100)
	{
		return MsgSetZoom(m_pParam, Num, Denom);
	}

	/**
	@brief パンスキャンの設定を取得する

	@param[out] pInfo 情報を取得する PanScanInfo 構造体へのポインタ。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_GETPANSCAN
	*/
	bool GetPanScan(PanScanInfo *pInfo)
	{
		pInfo->Size = sizeof(PanScanInfo);
		return MsgGetPanScan(m_pParam, pInfo);
	}

	/**
	@brief パンスキャンを設定する

	@param[in] pInfo 設定する情報を格納した PanScanInfo 構造体へのポインタ

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_SETPANSCAN
	*/
	bool SetPanScan(PanScanInfo *pInfo)
	{
		pInfo->Size = sizeof(PanScanInfo);
		return MsgSetPanScan(m_pParam, pInfo);
	}

	/**
	@brief ステータスを取得する

	@param[out] pInfo 情報を取得する StatusInfo 構造体へのポインタ。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_GETSTATUS
	*/
	bool GetStatus(StatusInfo *pInfo)
	{
		pInfo->Size = sizeof(StatusInfo);
		return MsgGetStatus(m_pParam, pInfo);
	}

	/**
	@brief 録画ステータスを取得する

	@param[in,out] pInfo 情報を取得する RecordStatusInfo 構造体へのポインタ。
	                     ファイル名を取得する場合は  RecordStatusInfo::pszFileName と  RecordStatusInfo::MaxFileName メンバを設定しておきます。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_GETRECORDSTATUS
	*/
	bool GetRecordStatus(RecordStatusInfo *pInfo)
	{
#if TVTEST_PLUGIN_VERSION < TVTEST_PLUGIN_VERSION_(0, 0, 10)
		pInfo->Size = sizeof(RecordStatusInfo);
#else
		if (pInfo->pszFileName != nullptr)
			pInfo->Size = sizeof(RecordStatusInfo);
		else
			pInfo->Size = RECORDSTATUSINFO_SIZE_V1;
#endif
		return MsgGetRecordStatus(m_pParam, pInfo);
	}

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 14)
	/**
	@brief 録画ステータスを取得する

	@param[in,out] pInfo 情報を取得する RecordStatusInfo 構造体へのポインタ。
	                     ファイル名を取得する場合は RecordStatusInfo::pszFileName と RecordStatusInfo::MaxFileName メンバを設定しておきます。
	@param[in] Flags 取得する情報のフラグ(RECORD_STATUS_FLAG_*) (ver.0.0.14 以降)

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_GETRECORDSTATUS
	*/
	bool GetRecordStatus(RecordStatusInfo *pInfo, RecordStatusFlag Flags)
	{
		pInfo->Size = sizeof(RecordStatusInfo);
		return MsgGetRecordStatus(m_pParam, pInfo, Flags);
	}
#endif

	/**
	@brief 映像の情報を取得する

	@param[out] pInfo 情報を取得する VideoInfo 構造体へのポインタ。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_GETVIDEOINFO
	*/
	bool GetVideoInfo(VideoInfo *pInfo)
	{
		pInfo->Size = sizeof(VideoInfo);
		return MsgGetVideoInfo(m_pParam, pInfo);
	}

	/**
	@brief 音量を取得する

	@return 音量(0-100)

	@note 現在消音状態である場合、消音状態になる前の音量が返ります。

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_GETVOLUME

	@sa MsgGetMute()
	*/
	int GetVolume()
	{
		return MsgGetVolume(m_pParam);
	}

	/**
	@brief 音量を設定する

	@param[in] Volume 音量(0-100)

	@retval true 正常終了
	@retval false エラー発生

	@note 現在消音状態である場合、消音状態が解除されます。

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_SETVOLUME
	*/
	bool SetVolume(int Volume)
	{
		return MsgSetVolume(m_pParam, Volume);
	}

	/**
	@brief 消音状態であるか取得する

	@retval true 消音状態
	@retval false 消音状態ではない

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_GETVOLUME
	*/
	bool GetMute()
	{
		return MsgGetMute(m_pParam);
	}

	/**
	@brief 消音状態を設定する

	@param[in] fMute 消音状態にする場合は true、消音状態を解除する場合は false

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_SETVOLUME
	*/
	bool SetMute(bool fMute)
	{
		return MsgSetMute(m_pParam, fMute);
	}

	/**
	@brief ステレオモードを取得する

	@return TVTest ver.0.9.0 以降は、デュアルモノラル時に選択される音声。
	        ver.0.9.0 より前は、ステレオもしくはデュアルモノラル時に現在選択されている音声。

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_GETSTEREOMODE
	*/
	StereoModeType GetStereoMode()
	{
		return MsgGetStereoMode(m_pParam);
	}

	/**
	@brief ステレオモードを設定する

	@param[in] StereoMode TVTest ver.0.10.0 以降は、デュアルモノラル時に選択される音声。
	                      ver.0.10.0 より前は、ステレオもしくはデュアルモノラル時に再生される音声。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_SETSTEREOMODE
	*/
	bool SetStereoMode(StereoModeType StereoMode)
	{
		return MsgSetStereoMode(m_pParam, StereoMode);
	}

	/**
	@brief 全画面表示の状態を取得する

	@retval true 全画面表示
	@retval false 全画面表示ではない

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_GETFULLSCREEN
	*/
	bool GetFullscreen()
	{
		return MsgGetFullscreen(m_pParam);
	}

	/**
	@brief 全画面表示の状態を設定する

	@param[in] fFullscreen 全画面表示にする場合は true、全画面表示を解除する場合は false

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_SETFULLSCREEN
	*/
	bool SetFullscreen(bool fFullscreen)
	{
		return MsgSetFullscreen(m_pParam, fFullscreen);
	}

	/**
	@brief 再生が有効であるか取得する

	@retval true 再生中
	@retval false 再生オフ

	@par Corresponding message
	TVTest::MESSAGE_GETPREVIEW
	*/
	bool GetPreview()
	{
		return MsgGetPreview(m_pParam);
	}

	/**
	@brief 再生の有効状態を設定する

	@param[in] fPreview 再生を有効にする場合は true、再生オフにする場合は false

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_SETPREVIEW
	*/
	bool SetPreview(bool fPreview)
	{
		return MsgSetPreview(m_pParam, fPreview);
	}

	/**
	@brief 待機状態であるか取得する

	@retval true 待機状態
	@retval false 待機状態ではない

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_GETSTANDBY
	*/
	bool GetStandby()
	{
		return MsgGetStandby(m_pParam);
	}

	/**
	@brief 待機状態を設定する

	@param[in] fStandby 待機状態にする場合は true、待機状態を解除する場合は false

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_SETSTANDBY
	*/
	bool SetStandby(bool fStandby)
	{
		return MsgSetStandby(m_pParam, fStandby);
	}

	/**
	@brief 常に最前面表示の状態を取得する

	@retval true 常に最前面表示
	@retval false 常に最前面表示ではない

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_GETALWAYSONTOP
	*/
	bool GetAlwaysOnTop()
	{
		return MsgGetAlwaysOnTop(m_pParam);
	}

	/**
	@brief 常に最前面表示の状態を設定する

	@param[in] fAlwaysOnTop 常に最前面表示にする場合は true、常に最前面表示を解除する場合は false

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_SETALWAYSONTOP
	*/
	bool SetAlwaysOnTop(bool fAlwaysOnTop)
	{
		return MsgSetAlwaysOnTop(m_pParam, fAlwaysOnTop);
	}

	/**
	@brief 画像をキャプチャする

	@param[in] Flags フラグ。現在は常に CAPTURE_IMAGE_FLAG_NONE 。

	@return パック DIB データ(BITMAPINFOHEADER + ピクセルデータ)へのポインタ。
	        不要になった場合は MemoryFree() で解放します。
	        キャプチャできなかった場合は nullptr が返ります。

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_CAPTUREIMAGE
	*/
	void *CaptureImage(CaptureImageFlag Flags = CAPTURE_IMAGE_FLAG_NONE)
	{
		return MsgCaptureImage(m_pParam, Flags);
	}

	/**
	@brief 画像を保存する

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_SAVEIMAGE
	*/
	bool SaveImage()
	{
		return MsgSaveImage(m_pParam);
	}

#if TVTEST_PLUGIN_VERSION < TVTEST_PLUGIN_VERSION_(0, 0, 9)
	/**
	@brief リセットを行う

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_RESET
	*/
	bool Reset()
	{
		return MsgReset(m_pParam);
	}
#else
	/**
	@brief リセットを行う

	@param[in] Flags リセットのフラグ(RESET_*) (ver.0.0.9 以降)

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_RESET
	*/
	bool Reset(ResetFlag Flags = RESET_ALL)
	{
		return MsgReset(m_pParam, Flags);
	}
#endif

	/**
	@brief ウィンドウを閉じる

	@param[in] Flags フラグ(CLOSE_*)

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_CLOSE
	*/
	bool Close(CloseFlag Flags = CLOSE_NONE)
	{
		return MsgClose(m_pParam, Flags);
	}

	/**
	@brief ストリームコールバックを設定する

	ストリームコールバックを登録すると、TS データを受け取ることができます。
	コールバック関数は一つのプラグインで複数登録できます。
	ストリームコールバック関数で処理が遅延すると全体が遅延するので、
	時間が掛かる処理は別スレッドで行うなどしてください。

	@param[in] Flags フラグ(STREAM_CALLBACK_*)
	@param[in] Callback コールバック関数
	@param[in] pClientData コールバック関数に渡すデータ

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_SETSTREAMCALLBACK
	*/
	bool SetStreamCallback(StreamCallbackFlag Flags, StreamCallbackFunc Callback, void *pClientData = nullptr)
	{
		return MsgSetStreamCallback(m_pParam, Flags, Callback, pClientData);
	}

	/**
	@brief プラグインの有効状態を設定する

	@param[in] fEnable 有効にする場合は true、無効にする場合は false

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_ENABLEPLUGIN
	*/
	bool EnablePlugin(bool fEnable)
	{
		return MsgEnablePlugin(m_pParam, fEnable);
	}

	/**
	@brief 色の設定を取得する

	@param[in] pszColor 取得したい色の名前。
	                    名前は配色設定ファイル(*.httheme)の項目名("StatusBack" など)と同じです。

	@return 色を表す COLORREF 値。
	        pszColor が無効な場合は CLR_INVALID。

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_GETCOLOR
	*/
	COLORREF GetColor(LPCWSTR pszColor)
	{
		return MsgGetColor(m_pParam, pszColor);
	}

	/**
	@brief ARIB 文字列をデコードする

	@param[in] pSrcData デコードする ARIB 文字列データへのポインタ
	@param[in] SrcLength デコードする ARIB 文字列データの長さ(バイト単位)
	@param[out] pszDest デコードした文字列を格納するバッファへのポインタ
	@param[in] DestLength pszDest の差すバッファの長さ

	@retval true 正常終了
	@retval false エラー発生

	@note デコードした文字列の長さが DestLength 以上の場合、DestLength - 1 の長さまで切り詰められ、終端に空文字が付加されます。

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_DECODEARIBSTRING
	*/
	bool DecodeARIBString(
			const void *pSrcData, DWORD SrcLength,
			LPWSTR pszDest, DWORD DestLength)
	{
		return MsgDecodeARIBString(m_pParam, pSrcData, SrcLength, pszDest, DestLength);
	}

	/**
	@brief 現在の番組の情報を取得する

	@param[in,out] pInfo 情報を取得する ProgramInfo 構造体へのポインタ。
	                     ProgramInfo::pszEventName / ProgramInfo::pszEventText / ProgramInfo::pszEventExtText メンバに取得先のバッファへのポインタを、
	                     ProgramInfo::MaxEventName / ProgramInfo::MaxEventText / ProgramInfo::MaxEventExtText メンバにバッファの長さ(要素数)を設定します。
	                     必要のない情報は、ポインタを nullptr にすると取得されません。
	@param[in] fNext 次の番組の情報を取得する場合は true、現在の番組の情報を取得する場合は false

	@retval true 正常終了
	@retval false エラー発生

	@remark GetEpgEventInfo() または GetCurrentEpgEventInfo() で、より詳しい番組情報を取得することもできます。

	@since ver.0.0.0

	@par Corresponding message
	TVTest::MESSAGE_GETCURRENTPROGRAMINFO

	@sa GetCurrentEpgEventInfo()
	*/
	bool GetCurrentProgramInfo(ProgramInfo *pInfo, bool fNext = false)
	{
		pInfo->Size = sizeof(ProgramInfo);
		return MsgGetCurrentProgramInfo(m_pParam, pInfo, fNext);
	}

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 1)
	/**
	@brief 指定されたイベントの通知に対応しているか取得する

	@param[in] Event イベントのコード(EVENT_*)

	@retval true イベントの通知に対応している
	@retval false イベントの通知に対応していない

	@since ver.0.0.1

	@par Corresponding message
	TVTest::MESSAGE_QUERYEVENT
	*/
	bool QueryEvent(EventCode Event)
	{
		return MsgQueryEvent(m_pParam, Event);
	}

	/**
	@brief 現在のチューニング空間及びチューニング空間数を取得する

	@param[out] pNumSpaces チューニング空間の数を取得する変数へのポインタ。取得する必要がない場合は nullptr。

	@return 現在のチューニング空間のインデックス

	@since ver.0.0.1

	@par Corresponding message
	TVTest::MESSAGE_GETTUNINGSPACE
	*/
	int GetTuningSpace(int *pNumSpaces = nullptr)
	{
		return MsgGetTuningSpace(m_pParam, pNumSpaces);
	}

	/**
	@brief チューニング空間の情報を取得する

	@param[in] Index 情報を取得するチューニング空間のインデックス
	@param[out] pInfo 情報を取得する TuningSpaceInfo 構造体へのポインタ。

	@return 現在のチューニング空間のインデックス

	@since ver.0.0.1

	@par Corresponding message
	TVTest::MESSAGE_GETTUNINGSPACEINFO
	*/
	bool GetTuningSpaceInfo(int Index, TuningSpaceInfo *pInfo)
	{
		pInfo->Size = sizeof(TuningSpaceInfo);
		return MsgGetTuningSpaceInfo(m_pParam, Index, pInfo);
	}

	/**
	@brief チャンネルを次に送る

	@param[in] fNext 次のチャンネルにする場合は true、前のチャンネルにする場合は false

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.1

	@par Corresponding message
	TVTest::MESSAGE_SETNEXTCHANNEL
	*/
	bool SetNextChannel(bool fNext = true)
	{
		return MsgSetNextChannel(m_pParam, fNext);
	}
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 2)
	/**
	@brief 現在の音声ストリームを取得する

	@return 現在の音声ストリームのインデックス

	@remark 音声ストリームの数は GetAudioStreamCount() または GetServiceInfo() で取得できます。

	@since ver.0.0.2

	@par Corresponding message
	TVTest::MESSAGE_GETAUDIOSTREAM
	*/
	int GetAudioStream()
	{
		return MsgGetAudioStream(m_pParam);
	}

	/**
	@brief 音声ストリームを設定する

	@param[in] Index 音声ストリームのインデックス

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.2

	@par Corresponding message
	TVTest::MESSAGE_SETAUDIOSTREAM
	*/
	bool SetAudioStream(int Index)
	{
		return MsgSetAudioStream(m_pParam, Index);
	}
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 3)
	/**
	@brief プラグインの有効状態を取得する

	@retval true プラグインが有効
	@retval false プラグインが無効

	@since ver.0.0.3

	@par Corresponding message
	TVTest::MESSAGE_ISPLUGINENABLED
	*/
	bool IsPluginEnabled()
	{
		return MsgIsPluginEnabled(m_pParam);
	}

	/**
	@brief コマンドを登録する

	TVTInitialize() 内で呼びます。
	コマンドを登録すると、ショートカットキーやリモコンに機能を割り当てられるようになります。

	コマンドが実行されると TVTest::EVENT_COMMAND イベントが送られます。
	その際、パラメータとして識別子が渡されます。

	@param[in] ID コマンドの識別子
	@param[in] pszText コマンドの文字列
	@param[in] pszName コマンドの名前

	@retval true 正常終了
	@retval false エラー発生

	@remark RegisterCommand() から機能が追加されたバージョンの RegisterPluginCommand() もあります。

	@since ver.0.0.3

	@par Corresponding message
	TVTest::MESSAGE_REGISTERCOMMAND

	@par Example
	@code{.cpp}
		constexpr int ID_MYCOMMAND = 1;

		m_pApp->RegisterCommand(ID_MYCOMMAND, L"MyCommand", L"私のコマンド");
	@endcode
	*/
	bool RegisterCommand(int ID, LPCWSTR pszText, LPCWSTR pszName)
	{
		return MsgRegisterCommand(m_pParam, ID, pszText, pszName);
	}

	/**
	@brief コマンドを登録する

	TVTInitialize 内で呼びます。

	@param[in] pCommandList コマンドの情報を格納した CommandInfo 構造体の配列へのポインタ
	@param[in] NumCommands pCommandList の差す配列の要素数

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.3

	@par Corresponding message
	TVTest::MESSAGE_REGISTERCOMMAND
	*/
	bool RegisterCommand(const CommandInfo *pCommandList, int NumCommands)
	{
		return MsgRegisterCommand(m_pParam, pCommandList, NumCommands);
	}

	/**
	@brief ログを記録する

	設定のログの項目に表示されます。

	@param[in] pszText ログの文字列

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.3

	@par Corresponding message
	TVTest::MESSAGE_ADDLOG
	*/
	bool AddLog(LPCWSTR pszText)
	{
		return MsgAddLog(m_pParam, pszText);
	}

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 14)
	/**
	@brief ログを記録する

	設定のログの項目に表示されます。

	@param[in] pszText ログの文字列
	@param[in] Type ログの種類(ver.0.0.14 以降)

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.3

	@par Corresponding message
	TVTest::MESSAGE_ADDLOG
	*/
	bool AddLog(LPCWSTR pszText, LogType Type)
	{
		return MsgAddLog(m_pParam, pszText, Type);
	}
#endif
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 5)
	/**
	@brief ステータス(TVTest::MESSAGE_GETSTATUS で取得できる内容)をリセットする

	@retval true 正常終了
	@retval false エラー発生

	@remark リセットが行われると TVTest::EVENT_STATUSRESET が送られます。

	@since ver.0.0.5

	@par Corresponding message
	TVTest::MESSAGE_RESETSTATUS
	*/
	bool ResetStatus()
	{
		return MsgResetStatus(m_pParam);
	}
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 6)
	/**
	@brief 音声のサンプルを取得するコールバック関数を設定する

	@param[in] pCallback コールバック関数。nullptr を指定すると、設定が解除されます。
	@param[in] pClientData コールバック関数に渡されるデータ

	@retval true 正常終了
	@retval false エラー発生

	@note 一つのプラグインで設定できるコールバック関数は一つだけです。
	      2回目以降の呼び出しでは、前回の設定が上書きされます。

	@since ver.0.0.6

	@par Corresponding message
	TVTest::MESSAGE_SETAUDIOCALLBACK
	*/
	bool SetAudioCallback(AudioCallbackFunc pCallback, void *pClientData = nullptr)
	{
		return MsgSetAudioCallback(m_pParam, pCallback, pClientData);
	}
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 7)
	/**
	@brief コマンドを実行する

	文字列を指定してコマンドを実行します。
	コマンドとは TVTest の各機能を実行するためのものです。
	コマンドの情報は GetAppCommandInfo() で取得できます。

	@param[in] pszCommand 実行するコマンドの文字列

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.7

	@par Corresponding message
	TVTest::MESSAGE_DOCOMMAND

	@par Example
	@code{.cpp}
		// 設定ダイアログを表示
		m_pApp->DoCommand(L"Options");
	@endcode
	*/
	bool DoCommand(LPCWSTR pszCommand)
	{
		return MsgDoCommand(m_pParam, pszCommand);
	}
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 8)
	/**
	@brief ホストプログラムの情報を取得する

	@param[out] pInfo 情報を取得する HostInfo 構造体へのポインタ。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.8

	@par Corresponding message
	TVTest::MESSAGE_GETHOSTINFO
	*/
	bool GetHostInfo(HostInfo *pInfo)
	{
		pInfo->Size = sizeof(HostInfo);
		return MsgGetHostInfo(m_pParam, pInfo);
	}
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 9)
	/**
	@brief 設定を取得する

	以下の設定が取得できます。
	設定名の大文字と小文字は区別されません。

	| 設定名          | 内容                            | 型     | 対応バージョン |
	|-----------------|---------------------------------|--------|----------------|
	| DriverDirectory | BonDriver の検索ディレクトリ    | 文字列 | 0.0.9          |
	| IniFilePath     | Ini ファイルのパス              | 文字列 | 0.0.9          |
	| RecordFolder    | 録画時の保存先フォルダ          | 文字列 | 0.0.9          |
	| RecordFileName  | 録画のファイル名(※1)           | 文字列 | 0.0.14         |
	| CaptureFolder   | キャプチャの保存先フォルダ(※2) | 文字列 | 0.0.14         |
	| CaptureFileName | キャプチャのファイル名(※1)     | 文字列 | 0.0.14         |

	+ ※1 "%event-name%" などの変数が含まれている可能性があります。
	      FormatVarString() を使って変数を展開できます。
	+ ※2 相対パスの可能性があります。その場合実行ファイルの場所が基準です。

	@param[in,out] pInfo 設定を取得する SettingInfo 構造体へのポインタ。
	                     呼び出す前に各メンバを設定しておきます。
	                     文字列とデータの場合、 SettingInfo::ValueSize に設定の格納に必要なバイト数が返されます。

	@retval true 正常終了
	@retval false エラー発生

	@note 通常は型ごとにオーバーロードされた関数を使用した方が便利です。

	@since ver.0.0.9

	@par Corresponding message
	TVTest::MESSAGE_GETSETTING
	*/
	bool GetSetting(SettingInfo *pInfo)
	{
		return MsgGetSetting(m_pParam, pInfo);
	}

	/**
	@brief int の設定を取得する

	@param[in] pszName 取得する設定名
	@param[out] pValue 値を取得する変数へのポインタ

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.9

	@par Corresponding message
	TVTest::MESSAGE_GETSETTING
	*/
	bool GetSetting(LPCWSTR pszName, int *pValue)
	{
		return MsgGetSetting(m_pParam, pszName, pValue);
	}

	/**
	@brief unsigned int の設定を取得する

	@param[in] pszName 取得する設定名
	@param[out] pValue 値を取得する変数へのポインタ

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.9

	@par Corresponding message
	TVTest::MESSAGE_GETSETTING
	*/
	bool GetSetting(LPCWSTR pszName, unsigned int *pValue)
	{
		return MsgGetSetting(m_pParam, pszName, pValue);
	}

	/**
	@brief 文字列の設定を取得する

	@param[in] pszName 取得する設定名
	@param[out] pszString 文字列を取得するバッファへのポインタ。
	                      nullptr にすると、必要なバッファの長さ(終端の空文字を含む)が返ります。
	@param[in] MaxLength pszString の差す先に格納できる最大長(終端の空文字を含む)

	@return 取得した文字列の長さ(終端の空文字を含む)。
	        設定が取得できなかった場合は 0。

	@note 文字列が MaxLength の長さに収まらない場合、途中で切り詰められて終端に空文字が付加されます。

	@since ver.0.0.9

	@par Corresponding message
	TVTest::MESSAGE_GETSETTING

	@par Example
	@code{.cpp}
		WCHAR szIniPath[MAX_PATH];
		if (m_pApp->GetSetting(L"IniFilePath", szIniPath, MAX_PATH) > 0) {
			// 呼び出しが成功した場合は、szIniPath に Ini ファイルのパスが格納されています
		}
	@endcode
	*/
	DWORD GetSetting(LPCWSTR pszName, LPWSTR pszString, DWORD MaxLength)
	{
		return MsgGetSetting(m_pParam, pszName, pszString, MaxLength);
	}

	/**
	@brief BonDriver のフルパス名を取得する

	@param[out] pszPath パスを取得するバッファへのポインタ。nullptr で長さの取得のみ行う。
	@param[in] MaxLength pszPath の差す先に格納できる最大長(終端の空文字を含む)

	@return パスの長さ(終端の空文字を除く)。
            BonDriver が読み込まれていない場合は 0。

	@note パスの長さが MaxLength 以上である場合、pszPath に格納される文字列は MaxLength - 1 の長さに切り詰められ、終端に空文字が付加されます。
	      その際も戻り値は切り捨てられる前の本来の長さです。

	@since ver.0.0.9

	@par Corresponding message
	TVTest::MESSAGE_GETDRIVERFULLPATHNAME
	*/
	int GetDriverFullPathName(LPWSTR pszPath, int MaxLength)
	{
		return MsgGetDriverFullPathName(m_pParam, pszPath, MaxLength);
	}
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 10)
	/**
	@brief ロゴの画像を取得する

	@param[in] NetworkID ネットワーク ID
	@param[in] ServiceID サービス ID
	@param[in] LogoType ロゴの種類。0 から 5 までで指定します。以下のサイズのロゴが取得されます。
	                    | LogoType | サイズ |
	                    |----------|--------|
	                    | 0        | 48x24  |
	                    | 1        | 36x24  |
	                    | 2        | 48x27  |
	                    | 3        | 72x36  |
	                    | 4        | 54x36  |
	                    | 5        | 64x36  |
	                    いずれのロゴも16:9で表示すると本来の比率になります。

	@return ビットマップ(DIB セクション)のハンドル。
	        画像が取得できない場合は nullptr。

	@note ビットマップは不要になった時に DeleteObject() で破棄してください。

	@since ver.0.0.10

	@par Corresponding message
	TVTest::MESSAGE_GETLOGO
	*/
	HBITMAP GetLogo(WORD NetworkID, WORD ServiceID, BYTE LogoType)
	{
		return MsgGetLogo(m_pParam, NetworkID, ServiceID, LogoType);
	}

	/**
	@brief 利用可能なロゴの種類を取得する

	@param[in] NetworkID ネットワーク ID
	@param[in] ServiceID サービス ID

	@return 利用可能なロゴを表すフラグ。
	        下位から 1 ビットごとに LogoType の 0 から 5 までを表し、ビットが 1 であればその種類のロゴが利用できます。
	        LogoType については GetLogo() を参照してください。

	@since ver.0.0.10

	@par Corresponding message
	TVTest::MESSAGE_GETAVAILABLELOGOTYPE

	@par Example
	@code{.cpp}
		if (m_pApp->GetAvailableLogoType(NetworkID, ServiceID) & 1) {
			// タイプ0のロゴが利用できる
		}
	@endcode
	*/
	UINT GetAvailableLogoType(WORD NetworkID, WORD ServiceID)
	{
		return MsgGetAvailableLogoType(m_pParam, NetworkID, ServiceID);
	}

	/**
	@brief 録画ファイルを切り替える

	現在録画中のファイルを閉じて、指定されたパスにファイルを作成して続きを録画するようにします。
	新しいファイルが開けなかった場合は、今までのファイルで録画が継続されます。

	@param[in] pszFileName 新しいファイルのパス

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.10

	@par Corresponding message
	TVTest::MESSAGE_RELAYRECORD
	*/
	bool RelayRecord(LPCWSTR pszFileName)
	{
		return MsgRelayRecord(m_pParam, pszFileName);
	}

	/**
	@brief サイレントモードを取得する

	サイレントモードではエラー時などにダイアログが出なくなります。
	コマンドラインで /silent を指定するか、 SetSilentMode() で設定すればサイレントモードになります。

	@retval true サイレントモードである
	@retval false サイレントモードではない

	@since ver.0.0.10

	@par Corresponding message
	TVTest::MESSAGE_SILENTMODE
	*/
	bool GetSilentMode()
	{
		return MsgGetSilentMode(m_pParam);
	}

	/**
	@brief サイレントモードを設定する

	@param[in] fSilent サイレントモードにする場合は true、サイレントモードを解除する場合は false

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.10

	@par Corresponding message
	TVTest::MESSAGE_SILENTMODE
	*/
	bool SetSilentMode(bool fSilent)
	{
		return MsgSetSilentMode(m_pParam, fSilent);
	}
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 11)
	/**
	@brief ウィンドウメッセージコールバックを設定

	メッセージコールバック関数を登録すると、TVTest のメインウィンドウにメッセージが送られた時に呼び出されます。
	コールバック関数では、TVTest にメッセージの処理をさせない時は TRUE を返します。
	その際、pResult に書き込んだ値が返されます。

	@param[in] Callback コールバック関数。nullptr を渡すと設定が解除されます。
	@param[in] pClientData コールバック関数の呼び出し時に渡されるデータ。

	@retval true 正常終了
	@retval false エラー発生

	@note 一つのプラグインで設定できるコールバック関数は一つだけです。
	      2回目以降の呼び出しでは、前回の設定が上書きされます。

	@since ver.0.0.11

	@par Corresponding message
	TVTest::MESSAGE_SETWINDOWMESSAGECALLBACK
	*/
	bool SetWindowMessageCallback(WindowMessageCallbackFunc Callback, void *pClientData = nullptr)
	{
		return MsgSetWindowMessageCallback(m_pParam, Callback, pClientData);
	}

	/**
	@brief コントローラを登録する

	コントローラの登録を行うと、設定ダイアログのリモコンのページで割り当てが設定できるようになります。
	コントローラの画像を用意すると、設定の右側に表示されます。
	ボタンが押された時に OnControllerButtonDown() を呼び出して通知すると、
	割り当ての設定に従って機能が実行されます。

	@param[in] pInfo 登録するコントローラの情報を格納した ControllerInfo 構造体へのポインタ

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.11

	@par Corresponding message
	TVTest::MESSAGE_REGISTERCONTROLLER
	*/
	bool RegisterController(ControllerInfo *pInfo)
	{
		pInfo->Size = sizeof(ControllerInfo);
		return MsgRegisterController(m_pParam, pInfo);
	}

	/**
	@brief コントローラのボタンが押されたことを通知する

	@param[in] pszName コントローラ識別名
	@param[in] Button 押されたボタンのインデックス

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.11

	@par Corresponding message
	TVTest::MESSAGE_ONCONTROLLERBUTTONDOWN
	*/
	bool OnControllerButtonDown(LPCWSTR pszName, int Button)
	{
		return MsgOnControllerButtonDown(m_pParam, pszName, Button);
	}

	/**
	@brief コントローラの設定を取得する

	@param[in] pszName コントローラ識別名
	@param[in,out] pSettings 設定を取得する ControllerSettings 構造体へのポインタ。
	                         事前に ControllerSettings::Mask メンバに取得する項目のフラグを設定して呼び出します。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.11

	@par Corresponding message
	TVTest::MESSAGE_GETCONTROLLERSETTINGS
	*/
	bool GetControllerSettings(LPCWSTR pszName, ControllerSettings *pSettings)
	{
		return MsgGetControllerSettings(m_pParam, pszName, pSettings);
	}

	/**
	@brief コントローラがアクティブ時のみに設定されているか取得する

	@param[in] pszName コントローラ識別名

	@retval true アクティブ時のみ
	@retval false アクティブ時のみではない

	@note この関数は MsgGetControllerSettings() を呼び出します。

	@since ver.0.0.11
	*/
	bool IsControllerActiveOnly(LPCWSTR pszName)
	{
		return MsgIsControllerActiveOnly(m_pParam, pszName);
	}
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 12)
	/**
	@brief 番組情報を取得

	@param[in] pInfo 取得したい番組情報を指定する EpgEventQueryInfo 構造体へのポインタ

	@return 番組情報を表す EpgEventInfo 構造体のポインタ。
	        情報が取得できなかった場合は nullptr。
	        取得した情報が不要になった場合、 FreeEpgEventInfo() で解放します。

	@since ver.0.0.12

	@par Corresponding message
	TVTest::MESSAGE_GETEPGEVENTINFO

	@par Example
	@code{.cpp}
		// 現在のチャンネルの現在の番組を取得する例
		TVTest::ChannelInfo ChInfo;
		if (m_pApp->GetCurrentChannelInfo(&ChInfo)) {
			TVTest::EpgEventQueryInfo QueryInfo;
			QueryInfo.NetworkID         = ChInfo.NetworkID;
			QueryInfo.TransportStreamID = ChInfo.TransportStreamID;
			QueryInfo.ServiceID         = ChInfo.ServiceID;
			QueryInfo.Type              = TVTest::EPG_EVENT_QUERY_TIME;
			QueryInfo.Flags             = TVTest::EPG_EVENT_QUERY_FLAG_NONE;
			GetSystemTimeAsFileTime(&QueryInfo.Time);
			TVTest::EpgEventInfo *pEvent = m_pApp->GetEpgEventInfo(&QueryInfo);
			if (pEvent != nullptr) {
				...
				m_pApp->FreeEpgEventInfo(pEvent);
			}
		}
	@endcode
	*/
	EpgEventInfo *GetEpgEventInfo(const EpgEventQueryInfo *pInfo)
	{
		return MsgGetEpgEventInfo(m_pParam, pInfo);
	}

	/**
	@brief 番組情報を解放

	GetEpgEventInfo() で取得した情報のメモリを解放します。

	@param[in] pEventInfo 解放する EpgEventInfo 構造体へのポインタ

	@since ver.0.0.12

	@par Corresponding message
	TVTest::MESSAGE_FREEEPGEVENTINFO
	*/
	void FreeEpgEventInfo(EpgEventInfo *pEventInfo)
	{
		MsgFreeEpgEventInfo(m_pParam, pEventInfo);
	}

	/**
	@brief 番組情報のリストを取得

	@param[in,out] pList 番組情報のリストを取得する EpgEventList 構造体へのポインタ。
	                     EpgEventList::NetworkID / EpgEventList::TransportStreamID / EpgEventList::ServiceID メンバを設定して呼び出します。

	@retval true 正常終了
	@retval false エラー発生

	@note 取得された番組は開始日時順にソートされています。
	      リストが不要になった場合、 FreeEpgEventList() で解放します。

	@since ver.0.0.12

	@par Corresponding message
	TVTest::MESSAGE_GETEPGEVENTLIST

	@par Example
	@code{.cpp}
		// 現在のチャンネルの番組表を取得する
		TVTest::ChannelInfo ChInfo;
		if (m_pApp->GetCurrentChannelInfo(&ChInfo)) {
			TVTest::EpgEventList EventList;
			EventList.NetworkID         = ChInfo.NetworkID;
			EventList.TransportStreamID = ChInfo.TransportStreamID;
			EventList.ServiceID         = ChInfo.ServiceID;
			if (m_pApp->GetEpgEventList(&EventList)) {
				...
				m_pApp->FreeEpgEventList(&EventList);
			}
		}
	@endcode
	*/
	bool GetEpgEventList(EpgEventList *pList)
	{
		return MsgGetEpgEventList(m_pParam, pList);
	}

	/**
	@brief 番組情報のリストを解放

	GetEpgEventList() で取得したリストのメモリを解放します。

	@param[in,out] pList 解放する番組情報のリストを格納した EpgEventList 構造体へのポインタ

	@since ver.0.0.12

	@par Corresponding message
	TVTest::MESSAGE_FREEEPGEVENTLIST
	*/
	void FreeEpgEventList(EpgEventList *pList)
	{
		MsgFreeEpgEventList(m_pParam, pList);
	}

	/**
	@brief BonDriver を列挙する

	BonDriver のフォルダとして設定されているフォルダ内の BonDriver を列挙します。

	@param[in] Index インデックス
	@param[out] pszFileName ファイル名を取得するバッファへのポインタ
	@param[in] MaxLength pszFileName が差すバッファの長さ

	@return ファイル名の長さ。Index が範囲外の場合は 0。

	@note ファイル名の長さが MaxLength 以上である場合、pszFileName に格納される文字列は MaxLength - 1 の長さに切り詰められ、終端に空文字が付加されます。
	      その際も戻り値は切り捨てられる前の本来の長さです。

	@since ver.0.0.12

	@par Corresponding message
	TVTest::MESSAGE_ENUMDRIVER
	*/
	int EnumDriver(int Index, LPWSTR pszFileName, int MaxLength)
	{
		return MsgEnumDriver(m_pParam, Index, pszFileName, MaxLength);
	}

	/**
	@brief BonDriver のチャンネルのリストを取得する

	@param[in] pszDriverName 情報を取得する BonDriver のファイル名。
	@param[in,out] pList 情報を取得する DriverTuningSpaceList 構造体へのポインタ。
	                     DriverTuningSpaceList::Flags メンバを設定して呼び出します。

	@retval true 正常終了
	@retval false エラー発生

	@note リストが不要になった場合、 FreeDriverTuningSpaceList() で解放します。

	@since ver.0.0.12

	@par Corresponding message
	TVTest::MESSAGE_GETDRIVERTUNINGSPACELIST
	*/
	bool GetDriverTuningSpaceList(LPCWSTR pszDriverName, DriverTuningSpaceList *pList)
	{
		return MsgGetDriverTuningSpaceList(m_pParam, pszDriverName, pList);
	}

	/**
	@brief BonDriver のチャンネルのリストを解放する

	GetDriverTuningSpaceList() で取得した情報を解放します。

	@param[in,out] pList 解放する DriverTuningSpaceList 構造体へのポインタ。

	@since ver.0.0.12

	@par Corresponding message
	TVTest::MESSAGE_FREEDRIVERTUNINGSPACELIST
	*/
	void FreeDriverTuningSpaceList(DriverTuningSpaceList *pList)
	{
		MsgFreeDriverTuningSpaceList(m_pParam, pList);
	}
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 13)
	/**
	@brief 番組表のイベントの有効/無効を設定する

	番組表のイベント(EVENT_PROGRAMGUIDE_*)の通知が必要な場合、通知を有効に設定します。

	@param[in] EventFlags 有効にしたい通知を表すフラグを指定します。
	                      PROGRAMGUIDE_EVENT_* の組み合わせです。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.13

	@par Corresponding message
	TVTest::MESSAGE_ENABLEPROGRAMGUIDEEVENT
	*/
	bool EnableProgramGuideEvent(ProgramGuideEventFlag EventFlags)
	{
		return MsgEnableProgramGuideEvent(m_pParam, EventFlags);
	}

	/**
	@brief 番組表のコマンドを登録する

	コマンドを登録すると、番組表のダブルクリックなどに機能を割り当てることができるようになります。
	コマンドが実行されると TVTest::EVENT_PROGRAMGUIDE_COMMAND イベントが送られます。

	@param[in] pCommandList コマンドの情報を表す ProgramGuideCommandInfo 構造体の配列へのポインタ
	@param[in] NumCommands pCommandList の差す配列の要素数

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.13

	@par Corresponding message
	TVTest::MESSAGE_REGISTERPROGRAMGUIDECOMMAND
	*/
	bool RegisterProgramGuideCommand(const ProgramGuideCommandInfo *pCommandList, int NumCommands = 1)
	{
		return MsgRegisterProgramGuideCommand(m_pParam, pCommandList, NumCommands);
	}
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 14)
	/**
	@brief スタイル値を取得する

	TVTest.style.ini で設定されたスタイル値を取得します。
	通常は単位ごとにオーバーロードされた関数を利用する方が簡単です。

	@param[in,out] pInfo 情報を取得する StyleValueInfo 構造体へのポインタ。
	                     StyleValueInfo::Value 以外のメンバを設定して呼び出し、 StyleValueInfo::Value に値が返ります。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GETSTYLEVALUE
	*/
	bool GetStyleValue(StyleValueInfo *pInfo)
	{
		pInfo->Size = sizeof(StyleValueInfo);
		return MsgGetStyleValue(m_pParam, pInfo);
	}

	/**
	@brief 指定された単位のスタイル値を取得する

	@param[in] pszName スタイル名
	@param[in] Unit 取得する値の単位(STYLE_UNIT_*)
	@param[in] DPI DPI の指定
	@param[out] pValue 取得した値を返す変数へのポインタ

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GETSTYLEVALUE
	*/
	bool GetStyleValue(LPCWSTR pszName, StyleUnit Unit, int DPI, int *pValue)
	{
		return MsgGetStyleValue(m_pParam, pszName, Unit, DPI, pValue);
	}

	/**
	@brief オリジナルの単位のスタイル値を取得する

	@param[in] pszName スタイル名
	@param[out] pValue 取得した値を返す変数へのポインタ

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GETSTYLEVALUE
	*/
	bool GetStyleValue(LPCWSTR pszName, int *pValue)
	{
		return MsgGetStyleValue(m_pParam, pszName, pValue);
	}

	/**
	@brief 物理ピクセル単位のスタイル値を取得する

	@param[in] pszName スタイル名
	@param[in] DPI DPI の指定
	@param[out] pValue 取得した値を返す変数へのポインタ

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GETSTYLEVALUE
	*/
	bool GetStyleValuePixels(LPCWSTR pszName, int DPI, int *pValue)
	{
		return MsgGetStyleValuePixels(m_pParam, pszName, DPI, pValue);
	}

	/**
	@brief テーマの背景を描画する

	@param[in,out] pInfo テーマの背景描画情報を格納した ThemeDrawBackgroundInfo 構造体へのポインタ

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_THEMEDRAWBACKGROUND
	*/
	bool ThemeDrawBackground(ThemeDrawBackgroundInfo *pInfo)
	{
		pInfo->Size = sizeof(ThemeDrawBackgroundInfo);
		return MsgThemeDrawBackground(m_pParam, pInfo);
	}

	/**
	@brief テーマの背景を描画する

	@param[in] pszStyle スタイル名
	@param[in] hdc 描画先 DC
	@param[in] DrawRect 描画範囲
	@param[in] DPI DPI の指定(0 でメインウィンドウと同じ)

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_THEMEDRAWBACKGROUND
	*/
	bool ThemeDrawBackground(LPCWSTR pszStyle, HDC hdc, const RECT &DrawRect, int DPI = 0)
	{
		return MsgThemeDrawBackground(m_pParam, pszStyle, hdc, DrawRect, DPI);
	}

	/**
	@brief テーマの文字列を描画する

	@param[in] pInfo テーマの文字列描画情報を格納した ThemeDrawTextInfo 構造体へのポインタ

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_THEMEDRAWTEXT
	*/
	bool ThemeDrawText(ThemeDrawTextInfo *pInfo)
	{
		pInfo->Size = sizeof(ThemeDrawTextInfo);
		return MsgThemeDrawText(m_pParam, pInfo);
	}

	/**
	@brief テーマの文字列を描画する

	@param[in] pszStyle スタイル名
	@param[in] hdc 描画先 DC
	@param[in] pszText 描画する文字列
	@param[in] DrawRect 描画範囲
	@param[in] DrawFlags 描画フラグ(DrawText() API の DT_*)
	@param[in] Color 描画する色(CLR_INVALID でデフォルトの色)

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_THEMEDRAWTEXT
	*/
	bool ThemeDrawText(
		LPCWSTR pszStyle, HDC hdc, LPCWSTR pszText, const RECT &DrawRect,
		UINT DrawFlags, COLORREF Color = CLR_INVALID)
	{
		return MsgThemeDrawText(m_pParam, pszStyle, hdc, pszText, DrawRect, DrawFlags, Color);
	}

	/**
	@brief テーマのアイコンを描画する

	@param[in] pInfo テーマのアイコン描画情報を格納した ThemeDrawIconInfo 構造体へのポインタ

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_THEMEDRAWICON
	*/
	bool ThemeDrawIcon(ThemeDrawIconInfo *pInfo)
	{
		pInfo->Size = sizeof(ThemeDrawIconInfo);
		return MsgThemeDrawIcon(m_pParam, pInfo);
	}

	/**
	@brief テーマのアイコンを描画する

	@param[in] pszStyle スタイル名
	@param[in] hdc 描画先 DC
	@param[in] DstX 描画先の左位置
	@param[in] DstY 描画先の上位置
	@param[in] DstWidth 描画先の幅
	@param[in] DstHeight 描画先の高さ
	@param[in] hbm 描画するビットマップ
	@param[in] SrcX 描画元の左位置
	@param[in] SrcY 描画元の上位置
	@param[in] SrcWidth 描画元の幅
	@param[in] SrcHeight 描画元の高さ
	@param[in] Color 描画する色(CLR_INVALID でデフォルトの色)
	@param[in] Opacity 不透明度(1-255)。デフォルトは 255

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_THEMEDRAWICON
	*/
	bool ThemeDrawIcon(
		LPCWSTR pszStyle,
		HDC hdc, int DstX, int DstY, int DstWidth, int DstHeight,
		HBITMAP hbm, int SrcX, int SrcY, int SrcWidth, int SrcHeight,
		COLORREF Color = CLR_INVALID, BYTE Opacity = 255)
	{
		return MsgThemeDrawIcon(
			m_pParam, pszStyle, hdc, DstX, DstY, DstWidth, DstHeight,
			hbm, SrcX, SrcY, SrcWidth, SrcHeight, Color, Opacity);
	}

	/**
	@brief EPG 取得状況を取得する

	@param[in,out] pInfo EPG 取得状態の情報を格納する EpgCaptureStatusInfo 構造体へのポインタ。
	                     事前に各メンバを設定して呼び出します。
	                     EpgCaptureStatusInfo::Status メンバには取得したい情報のフラグ(EPG_CAPCTURE_STATUS_*)を指定して呼び出し、
	                     そのフラグの中で現在の取得状況が返されます。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GETEPGCAPTURESTATUS

	@par Example
	@code{.cpp}
		TVTest::EpgCaptureStatusInfo Info;
		Info.Flags = TVTest::EPG_CAPTURE_STATUS_FLAG_NONE;
		// 取得したいサービスを指定する
		Info.NetworkID = NetworkID;
		Info.TransportStreamID = TransportStreamID;
		Info.ServiceID = ServiceID;
		// Info.Status に取得したいステータスを指定する
		Info.Status = TVTest::EPG_CAPTURE_STATUS_SCHEDULEBASICCOMPLETED |
		              TVTest::EPG_CAPTURE_STATUS_SCHEDULEEXTENDEDCOMPLETED;
		if (m_pApp->GetEpgCaptureStatus(&Info)) {
			if (Info.Status & TVTest::EPG_CAPTURE_STATUS_SCHEDULEBASICCOMPLETED)
				std::printf("schedule basic 揃った\n");
			if (Info.Status & TVTest::EPG_CAPTURE_STATUS_SCHEDULEEXTENDEDCOMPLETED)
				std::printf("schedule extended 揃った\n");
		}
	@endcode
	*/
	bool GetEpgCaptureStatus(EpgCaptureStatusInfo *pInfo)
	{
		pInfo->Size = sizeof(EpgCaptureStatusInfo);
		return MsgGetEpgCaptureStatus(m_pParam, pInfo);
	}

	/**
	@brief コマンドの情報を取得する

	@param[in,out] pInfo コマンドの情報を取得する AppCommandInfo 構造体へのポインタ。
	                     事前に各メンバを設定して呼び出します。
	                     AppCommandInfo::pszText に nullptr を指定すると、必要なバッファ長(終端の空文字を含む)が AppCommandInfo::MaxText に返ります。
	                     AppCommandInfo::pszName に nullptr を指定すると、必要なバッファ長(終端の空文字を含む)が AppCommandInfo::MaxName に返ります。

	@retval true 正常終了
	@retval false エラー発生

	@note TVTInitialize() 内で呼ぶと、プラグインのコマンドなどが取得できませんので注意してください。
	@note AppCommandInfo::pszText に取得されたコマンド文字列を DoCommand() に指定して実行できます。

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GETAPPCOMMANDINFO

	@par Example
	@code{.cpp}
		const DWORD Count = m_pApp->GetAppCommandCount();
		for (DWORD i = 0; i < Count; i++) {
			TVTest::AppCommandInfo Info;
			WCHAR szText[256], szName[256];
			Info.Size = sizeof(Info);
			Info.Index = i;
			Info.pszText = szText;
			Info.MaxText = _countof(szText);
			Info.pszName = szName;
			Info.MaxName = _countof(szName);
			if (m_pApp->GetAppCommandInfo(&Info)) {
				std::wprintf(L"Command %u %ls %ls\n", i, szText, szName);
			}
		}
	@endcode
	*/
	bool GetAppCommandInfo(AppCommandInfo *pInfo)
	{
		pInfo->Size = sizeof(AppCommandInfo);
		return MsgGetAppCommandInfo(m_pParam, pInfo);
	}

	/**
	@brief コマンドの数を取得する

	@return コマンドの数

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GETAPPCOMMANDCOUNT
	*/
	DWORD GetAppCommandCount()
	{
		return MsgGetAppCommandCount(m_pParam);
	}

	/**
	@brief 映像ストリームの数を取得する

	@return 映像ストリームの数

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GETVIDEOSTREAMCOUNT
	*/
	int GetVideoStreamCount()
	{
		return MsgGetVideoStreamCount(m_pParam);
	}

	/**
	@brief 現在の映像ストリームを取得する

	@return 現在の映像ストリームのインデックス

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GETVIDEOSTREAM
	*/
	int GetVideoStream()
	{
		return MsgGetVideoStream(m_pParam);
	}

	/**
	@brief 映像ストリームを設定する

	@param[in] Stream 設定する映像ストリームのインデックス

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_SETVIDEOSTREAM
	*/
	bool SetVideoStream(int Stream)
	{
		return MsgSetVideoStream(m_pParam, Stream);
	}

	/**
	@brief ログを取得する

	@param[in,out] pInfo ログの情報を取得する GetLogInfo 構造体へのポインタ。
	                     GetLogInfo::Flags メンバにフラグを指定します。
	                     GetLogInfo::Flags に TVTest::GET_LOG_FLAG_BYSERIAL を指定した場合、GetLogInfo::Serial に取得したいログのシリアルナンバーを指定します。
	                     TVTest::GET_LOG_FLAG_BYSERIAL を指定しない場合、GetLogInfo::Index に取得したいログのインデックスを指定します。
	                     GetLogInfo::pszText メンバに取得する文字列を格納するバッファへのポインタを、GetLogInfo::MaxText メンバにバッファに格納できる最大長(終端の空文字を含む)を指定します。
	                     GetLogInfo::pszText に nullptr を指定すると、必要なバッファ長(終端の空文字を含む)が GetLogInfo::MaxText に返ります。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GETLOG

	@par Example
	@code{.cpp}
		// 全てのログを列挙する
		TVTest::GetLogInfo Info;
		// まず最初のログのシリアルを取得
		Info.Size = sizeof(Info);
		Info.Flags = TVTest::GET_LOG_FLAG_NONE;
		Info.Index = 0;
		Info.pszText = nullptr;
		if (m_pApp->GetLog(&Info)) {
			// シリアルから順番にログを取得
			WCHAR szText[256];
			Info.Flags = TVTest::GET_LOG_FLAG_BYSERIAL;
			Info.pszText = szText;
			for (;;) {
				Info.MaxText = _countof(szText);
				if (!m_pApp->GetLog(&Info))
					break;
				std::wprintf(L"Log %u : %ls\n", Info.Serial, Info.pszText);
				Info.Serial++;
			}
		}
	@endcode
	*/
	bool GetLog(GetLogInfo *pInfo)
	{
		pInfo->Size = sizeof(GetLogInfo);
		return MsgGetLog(m_pParam, pInfo);
	}

	/**
	@brief ログの数を取得する

	@return ログの数

	@note ユーザーがログをクリアするなどして、数が減ることもあり得ます。

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GETLOGCOUNT
	*/
	DWORD GetLogCount()
	{
		return MsgGetLogCount(m_pParam);
	}

	/**
	@brief プラグインのコマンドを登録する

	基本的に RegisterCommand() と同じですが、メンバが追加されています。

	@param[in] pInfo 登録するコマンドの情報を格納した PluginCommandInfo 構造体へのポインタ

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_REGISTERPLUGINCOMMAND
	*/
	bool RegisterPluginCommand(const PluginCommandInfo *pInfo)
	{
		return MsgRegisterPluginCommand(m_pParam, pInfo);
	}

	/**
	@brief プラグインのコマンドの状態を設定する

	@param[in] ID 状態を設定するコマンドの識別子
	@param[in] State 状態(PLUGIN_COMMAND_STATE_* の組み合わせ)

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_SETPLUGINCOMMANDSTATE
	*/
	bool SetPluginCommandState(int ID, PluginCommandState State)
	{
		return MsgSetPluginCommandState(m_pParam, ID, State);
	}

	/**
	@brief プラグインコマンドの通知を行う

	@param[in] ID 通知するコマンドの識別子
	@param[in] Type 通知の種類(PLUGIN_COMMAND_NOTIFY_*)

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_PLUGINCOMMANDNOTIFY
	*/
	bool PluginCommandNotify(int ID, PluginCommandNotify Type)
	{
		return MsgPluginCommandNotify(m_pParam, ID, Type);
	}

	/**
	@brief プラグインのアイコンを登録する

	アイコンを登録すると、プラグインの有効/無効をサイドバーで切り替えられるようになります。

	@param[in] pInfo 登録するアイコンの情報を格納した PluginIconInfo 構造体へのポインタ

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_REGISTERPLUGINICON
	*/
	bool RegisterPluginIcon(const PluginIconInfo *pInfo)
	{
		return MsgRegisterPluginIcon(m_pParam, pInfo);
	}

	/**
	@brief プラグインのアイコンを登録する

	@param[in] hbmIcon 登録するアイコン画像のビットマップ

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_REGISTERPLUGINICON
	*/
	bool RegisterPluginIcon(HBITMAP hbmIcon)
	{
		return MsgRegisterPluginIcon(m_pParam, hbmIcon);
	}

	/**
	@brief プラグインのアイコンをリソースから読み込んで登録する

	@param[in] hinst 読み込み元のリソースのインスタンスハンドル
	@param[in] pszName 読み込むリソース名。BITMAP リソースでなければなりません。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_REGISTERPLUGINICON
	*/
	bool RegisterPluginIconFromResource(HINSTANCE hinst, LPCTSTR pszName)
	{
		return MsgRegisterPluginIconFromResource(m_pParam, hinst, pszName);
	}

	/**
	@brief ステータス項目を登録する

	@param[in] pInfo 登録するステータス項目の情報を格納した StatusItemInfo 構造体へのポインタ

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_REGISTERSTATUSITEM
	*/
	bool RegisterStatusItem(const StatusItemInfo *pInfo)
	{
		return MsgRegisterStatusItem(m_pParam, pInfo);
	}

	/**
	@brief ステータス項目を設定する

	@param[in] pInfo 設定する情報を格納した StatusItemSetInfo 構造体へのポインタ。
	                 StatusItemSetInfo::Size に構造体のサイズを、 StatusItemSetInfo::Mask に設定したい情報を、 StatusItemSetInfo::ID に設定したい項目の識別子を指定します。
	                 StatusItemSetInfo::Mask に TVTest::STATUS_ITEM_SET_INFO_MASK_STATE を指定した場合、 StatusItemSetInfo::StateMask に設定したい状態のフラグを、 StatusItemSetInfo::State に有効にしたい状態のフラグを指定します。
	                 StatusItemSetInfo::Mask に TVTest::STATUS_ITEM_SET_INFO_MASK_STYLE を指定した場合、 StatusItemSetInfo::StyleMask に設定したい状態のフラグを、 StatusItemSetInfo::Style に有効にしたい状態のフラグを指定します。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_SETSTATUSITEM

	@par Example
	@code{.cpp}
		// 項目の表示状態を設定する
		void ShowStatusItem(int ID, bool fVisible)
		{
			TVTest::StatusItemSetInfo Info;
			Info.Size = sizeof(Info);
			Info.Mask = TVTest::STATUS_ITEM_SET_INFO_MASK_STATE;
			Info.ID = ID;
			Info.StateMask = TVTest::STATUS_ITEM_STATE_VISIBLE;
			Info.State = fVisible ? TVTest::STATUS_ITEM_STATE_VISIBLE : TVTest::STATUS_ITEM_STATE_NONE;
			m_pApp->SetStatusItem(&Info);
		}
	@endcode
	*/
	bool SetStatusItem(const StatusItemSetInfo *pInfo)
	{
		return MsgSetStatusItem(m_pParam, pInfo);
	}

	/**
	@brief ステータス項目の情報を取得する

	@param[in,out] pInfo 取得した情報を格納する StatusItemGetInfo 構造体へのポインタ。
	                     StatusItemGetInfo::Mask に取得したい情報を、 StatusItemGetInfo::ID に取得したい項目の識別子を指定して呼び出します。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GETSTATUSITEMINFO

	@par Example
	@code{.cpp}
		// 項目の表示状態を取得する
		bool IsStatusItemVisible(int ID)
		{
			TVTest::StatusItemGetInfo Info;
			Info.Mask = TVTest::STATUS_ITEM_GET_INFO_MASK_STATE;
			Info.ID = ID;
			return m_pApp->GetStatusItemInfo(&Info)
				&& (Info.State & TVTest::STATUS_ITEM_STATE_VISIBLE) != 0;
		}
	@endcode
	*/
	bool GetStatusItemInfo(StatusItemGetInfo *pInfo)
	{
		pInfo->Size = sizeof(StatusItemGetInfo);
		return MsgGetStatusItemInfo(m_pParam, pInfo);
	}

	/**
	@brief ステータス項目の通知を行う

	@param[in] ID 通知を行う項目の識別子
	@param[in] Type 通知の種類(STATUS_ITEM_NOTIFY_* のいずれか)

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_STATUSITEMNOTIFY
	*/
	bool StatusItemNotify(int ID, StatusItemNotifyType Type)
	{
		return MsgStatusItemNotify(m_pParam, ID, Type);
	}

	/**
	@brief TSプロセッサを登録する

	@param[in] pInfo 登録する TS プロセッサの情報を格納した TSProcessorInfo 構造体へのポインタ

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_REGISTERTSPROCESSOR
	*/
	bool RegisterTSProcessor(const TSProcessorInfo *pInfo)
	{
		return MsgRegisterTSProcessor(m_pParam, pInfo);
	}

	/**
	@brief パネル項目を登録する

	@param[in] pInfo 登録するパネル項目の情報を格納した PanelItemInfo 構造体へのポインタ

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_REGISTERPANELITEM
	*/
	bool RegisterPanelItem(const PanelItemInfo *pInfo)
	{
		return MsgRegisterPanelItem(m_pParam, pInfo);
	}

	/**
	@brief パネル項目を設定する

	@param[in] pInfo 設定するパネル項目の情報を格納した PanelItemSetInfo 構造体へのポインタ。
	                 PanelItemSetInfo::Size に構造体のサイズを、 PanelItemSetInfo::Mask に設定したい情報を、 PanelItemSetInfo::ID に設定したい項目の識別子を指定して呼び出します。
	                 PanelItemSetInfo::Mask に TVTest::PANEL_ITEM_SET_INFO_MASK_STATE を指定した場合、 PanelItemSetInfo::StateMask に設定したい状態のフラグを、 PanelItemSetInfo::State に有効にしたい状態のフラグを指定します。
	                 PanelItemSetInfo::Mask に TVTest::PANEL_ITEM_SET_INFO_MASK_STYLE を指定した場合、 PanelItemSetInfo::StyleMask に設定したい状態のフラグを、 PanelItemSetInfo::Style に有効にしたい状態のフラグを指定します。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_SETPANELITEM

	@par Example
	@code{.cpp}
		// 項目の有効状態を設定する
		void EnablePanelItem(int ID, bool fEnable)
		{
			TVTest::PanelItemSetInfo Info;
			Info.Size = sizeof(Info);
			Info.Mask = TVTest::PANEL_ITEM_SET_INFO_MASK_STATE;
			Info.ID = ID;
			Info.StateMask = TVTest::PANEL_ITEM_STATE_ENABLED;
			Info.State = fEnable ? TVTest::PANEL_ITEM_STATE_ENABLED : TVTest::PANEL_ITEM_STATE_NONE;
			m_pApp->SetPanelItem(&Info);
		}
	@endcode
	*/
	bool SetPanelItem(const PanelItemSetInfo *pInfo)
	{
		return MsgSetPanelItem(m_pParam, pInfo);
	}

	/**
	@brief パネル項目の情報を取得する

	@param[in,out] pInfo 情報を取得する PanelItemGetInfo 構造体へのポインタ。
	                     PanelItemGetInfo::Mask に取得したい情報を、 PanelItemGetInfo::ID に取得したい項目の識別子を指定して呼び出します。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GETPANELITEMINFO
	*/
	bool GetPanelItemInfo(PanelItemGetInfo *pInfo)
	{
		pInfo->Size = sizeof(PanelItemGetInfo);
		return MsgGetPanelItemInfo(m_pParam, pInfo);
	}

	/**
	@brief チャンネルを選択する

	@param[in] pInfo 選択するチャンネルの情報を格納した ChannelSelectInfo 構造体へのポインタ

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_SELECTCHANNEL
	*/
	bool SelectChannel(const ChannelSelectInfo *pInfo)
	{
		return MsgSelectChannel(m_pParam, pInfo);
	}

	/**
	@brief お気に入りリストを取得する

	@param[out] pList お気に入りリストを取得する FavoriteList 構造体へのポインタ。

	@retval true 正常終了
	@retval false エラー発生

	@note 取得したリストは FreeFavoriteList() で解放します。

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GETFAVORITELIST

	@par Example
	@code{.cpp}
		TVTest::FavoriteList List;
		if (m_pApp->GetFavoriteList(&List)) {
			for (DWORD i = 0; i < List.ItemCount; i++) {
				const TVTest::FavoriteItemInfo &Item = List.ItemList[i];
				if (Item.Type == TVTest::FAVORITE_ITEM_TYPE_FOLDER) {
					// Item.Folder にフォルダの情報が格納されている
					for (DWORD j = 0; j < Item.Folder.ItemCount; j++) {
						const TVTest::FavoriteItemInfo &SubItem = Item.Folder.ItemList[j];
						...
					}
				} else if (Item.Type == TVTest::FAVORITE_ITEM_TYPE_CHANNEL) {
					// Item.Channel にチャンネルの情報が格納されている
					...
				}
			}

			m_pApp->FreeFavoriteList(&List);
		}
	@endcode
	*/
	bool GetFavoriteList(FavoriteList *pList)
	{
		pList->Size = sizeof(FavoriteList);
		return MsgGetFavoriteList(m_pParam, pList);
	}

	/**
	@brief お気に入りリストを解放する

	GetFavoriteList() で取得したリストを解放します。

	@param[in,out] pList 解放するお気に入りリストの FavoriteList 構造体へのポインタ。

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_FREEFAVORITELIST
	*/
	void FreeFavoriteList(FavoriteList *pList)
	{
		MsgFreeFavoriteList(m_pParam, pList);
	}

	/**
	@brief ワンセグモードを取得する

	@retval true ワンセグモード
	@retval false ワンセグモードではない

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GET1SEGMODE
	*/
	bool Get1SegMode()
	{
		return MsgGet1SegMode(m_pParam);
	}

	/**
	@brief ワンセグモードを設定する

	@param[in] f1SegMode ワンセグモードに設定する場合は true、ワンセグモードを解除する場合は false

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_SET1SEGMODE
	*/
	bool Set1SegMode(bool f1SegMode)
	{
		return MsgSet1SegMode(m_pParam, f1SegMode);
	}

	/**
	@brief DPI を取得する

	@param[in] pInfo 取得する DPI の種類を指定する GetDPIInfo 構造体へのポインタ。
	                 GetDPIInfo::Type に取得する種類を、 GetDPIInfo::Flags にフラグを指定します。
	                 GetDPIInfo::Type が TVTest::DPI_TYPE_WINDOW の場合、 GetDPIInfo::hwnd メンバにウィンドウハンドルを指定します。
	                 GetDPIInfo::Type が TVTest::DPI_TYPE_RECT の場合、 GetDPIInfo::Rect メンバに矩形を指定します。
	                 GetDPIInfo::Type が TVTest::DPI_TYPE_POINT の場合、 GetDPIInfo::Point メンバに位置を指定します。
	                 GetDPIInfo::Type が TVTest::DPI_TYPE_MONITOR の場合、 GetDPIInfo::hMonitor メンバにモニタのハンドルを指定します。

	@return DPI の値。エラー時は 0

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GETDPI
	*/
	int GetDPI(GetDPIInfo *pInfo)
	{
		return MsgGetDPI(m_pParam, pInfo);
	}

	/**
	@brief システムの DPI を取得する

	@return DPI の値

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GETDPI
	*/
	int GetSystemDPI()
	{
		return MsgGetSystemDPI(m_pParam);
	}

	/**
	@brief ウィンドウの DPI を取得する

	@param[in] hwnd ウィンドウのハンドル
	@param[in] Flags フラグ(DPI_FLAG_*)

	@return DPI の値。エラー時は 0

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GETDPI
	*/
	int GetDPIFromWindow(HWND hwnd, DPIFlag Flags = DPI_FLAG_NONE)
	{
		return MsgGetDPIFromWindow(m_pParam, hwnd, Flags);
	}

	/**
	@brief 矩形位置の DPI を取得する

	@param[in] Rect 矩形位置
	@param[in] Flags フラグ(DPI_FLAG_*)

	@return DPI の値。エラー時は 0

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GETDPI
	*/
	int GetDPIFromRect(const RECT &Rect, DPIFlag Flags = DPI_FLAG_NONE)
	{
		return MsgGetDPIFromRect(m_pParam, Rect, Flags);
	}

	/**
	@brief 指定位置の DPI を取得する

	@param[in] Point 位置
	@param[in] Flags フラグ(DPI_FLAG_*)

	@return DPI の値。エラー時は 0

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GETDPI
	*/
	int GetDPIFromPoint(const POINT &Point, DPIFlag Flags = DPI_FLAG_NONE)
	{
		return MsgGetDPIFromPoint(m_pParam, Point, Flags);
	}

	/**
	@brief 指定位置の DPI を取得する

	@param[in] x 左位置
	@param[in] y 上位置
	@param[in] Flags フラグ(DPI_FLAG_*)

	@return DPI の値。エラー時は 0

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GETDPI
	*/
	int GetDPIFromPoint(LONG x, LONG y, DPIFlag Flags = DPI_FLAG_NONE)
	{
		return MsgGetDPIFromPoint(m_pParam, x, y, Flags);
	}

	/**
	@brief モニタの DPI を取得する

	@param[in] hMonitor モニタのハンドル
	@param[in] Flags フラグ(DPI_FLAG_*)

	@return DPI の値。エラー時は 0

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GETDPI
	*/
	int GetDPIFromMonitor(HMONITOR hMonitor, DPIFlag Flags = DPI_FLAG_NONE)
	{
		return MsgGetDPIFromMonitor(m_pParam, hMonitor, Flags);
	}

	/**
	@brief フォントを取得する

	以下のフォントが取得できます。

	| 識別子           | 意味                     |
	|------------------|--------------------------|
	| OSDFont          | OSD のフォント           |
	| PanelFont        | パネルのフォント         |
	| ProgramGuideFont | 番組表のフォント         |
	| StatusBarFont    | ステータスバーのフォント |

	@param[in,out] pInfo フォントの情報を取得する GetFontInfo 構造体へのポインタ。
	                     GetFontInfo::Flags メンバにフラグを、 GetFontInfo::pszName メンバに取得したいフォントの名前を指定します。
	                     GetFontInfo::DPI メンバにフォントの大きさをスケーリングするための GetFontInfo::DPI 値を指定します。
	                     GetFontInfo::LogFont メンバにフォントの情報が返されます。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GETFONT
	*/
	bool GetFont(GetFontInfo *pInfo)
	{
		pInfo->Size = sizeof(GetFontInfo);
		return MsgGetFont(m_pParam, pInfo);
	}

	/**
	@brief フォントを取得する

	@param[in] pszName 取得したいフォントの名前。
	                   指定できる名前は GetFont(GetFontInfo *) を参照してください。
	@param[out] pLogFont フォントの情報を取得する LOGFONTW 構造体へのポインタ
	@param[in] DPI フォントの大きさをスケーリングするための DPI 値。スケーリングを行わない場合は 0

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GETFONT
	*/
	bool GetFont(LPCWSTR pszName, LOGFONTW *pLogFont, int DPI = 0)
	{
		return MsgGetFont(m_pParam, pszName, pLogFont, DPI);
	}

	/**
	@brief ダイアログを表示する

	DialogBox() / CreateDialog() などの代わりに MsgShowDialog() を利用すると、
	DPI に応じたスケーリングやダークモードへの対応などが自動的に行われます。

	@param[in,out] pInfo ダイアログ表示の情報を格納した ShowDialogInfo 構造体へのポインタ。
	                     ShowDialogInfo::Flags に TVTest::SHOW_DIALOG_FLAG_MODELESS を指定するとモードレスダイアログが作成され、
	                     指定しない場合はモーダルダイアログが作成されます。

	@return モーダルダイアログの場合、EndDialog() で指定された戻り値。
	        モードレスダイアログの場合、ダイアログのウィンドウハンドル。

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_SHOWDIALOG
	*/
	INT_PTR ShowDialog(ShowDialogInfo *pInfo)
	{
		pInfo->Size = sizeof(ShowDialogInfo);
		return MsgShowDialog(m_pParam, pInfo);
	}

	/**
	@brief 日時を変換する

	@param[in,out] pInfo 日時変換の情報を格納した ConvertTimeInfo 構造体へのポインタ。
	                     ConvertTimeInfo::Flags に TVTest::CONVERT_TIME_FLAG_OFFSET を指定すると、 ConvertTimeInfo::Offset の時間が変換結果に加算されます。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_CONVERTTIME
	*/
	bool ConvertTime(ConvertTimeInfo *pInfo)
	{
		pInfo->Size = sizeof(ConvertTimeInfo);
		return MsgConvertTime(m_pParam, pInfo);
	}

	/**
	@brief EPG 日時を変換する

	@param[in] EpgTime 変換元の日時
	@param[in] Type 変換先の種類(CONVERT_TIME_TYPE_*)
	@param[out] pDstTime 変換した日時の情報を格納する SYSTEMTIME 構造体へのポインタ

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_CONVERTTIME
	*/
	bool ConvertEpgTimeTo(const SYSTEMTIME &EpgTime, ConvertTimeType Type, SYSTEMTIME *pDstTime)
	{
		return MsgConvertEpgTimeTo(m_pParam, EpgTime, Type, pDstTime);
	}

	/**
	@brief 映像ストリームを取得するコールバック関数を設定する

	@param[in] pCallback コールバック関数。nullptr を指定すると、設定が解除されます。
	@param[in] pClientData コールバック関数に渡すデータ

	@retval true 正常終了
	@retval false エラー発生

	@note 一つのプラグインで設定できるコールバック関数は一つだけです。
	      2回目以降の呼び出しでは、前回の設定が上書きされます。

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_SETVIDEOSTREAMCALLBACK
	*/
	bool SetVideoStreamCallback(VideoStreamCallbackFunc pCallback, void *pClientData = nullptr)
	{
		return MsgSetVideoStreamCallback(m_pParam, pCallback, pClientData);
	}

	/**
	@brief 変数文字列のコンテキストを取得

	@return コンテキストを表す VarStringContext 構造体へのポインタ

	@note 取得したコンテキストは MsgFreeVarStringContext() で解放します。

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_GETVARSTRINGCONTEXT
	*/
	VarStringContext *GetVarStringContext()
	{
		return MsgGetVarStringContext(m_pParam);
	}

	/**
	@brief 変数文字列のコンテキストを解放

	MsgGetVarStringContext() で取得したコンテキストを解放します。

	@param[in] pContext 解放する VarStringContext 構造体へのポインタ

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_FREEVARSTRINGCONTEXT
	*/
	void FreeVarStringContext(VarStringContext *pContext)
	{
		MsgFreeVarStringContext(m_pParam, pContext);
	}

	/**
	@brief 変数文字列を使って文字列をフォーマット

	変数文字列は、"%event-name%" などの変数が含まれた文字列です。
	このような文字列の変数を展開した文字列を取得できます。
	VarStringFormatInfo::pszResult に結果の文字列が返されるので、
	不要になったら MemoryFree() で解放します。
	変数の展開に必要な、現在の番組や日時などの情報をコンテキストと呼びます。
	GetVarStringContext() で、その時点のコンテキストを取得できます。

	@param[in,out] pInfo 変数文字列のフォーマットの情報を格納した VarStringFormatInfo 構造体へのポインタ。
	                     VarStringFormatInfo::pszResult に結果の文字列が返されるので、不要になったら MsgMemoryFree() で解放します。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_FORMATVARSTRING

	@par Example
	@code{.cpp}
		// 現在のコンテキストを取得
		// (ここでは例のために取得していますが、現在の情報を使うのであれば
		//  VarStringFormatInfo.pContext を nullptr にすればよいです)
		TVTest::VarStringContext *pContext = m_pApp->GetVarStringContext();

		// 文字列をフォーマットする
		TVTest::VarStringFormatInfo Info;
		Info.Size = sizeof(Info);
		Info.Flags = TVTest::VAR_STRING_FORMAT_FLAG_NONE;
		Info.pszFormat = L"%event-name% %tot-date% %tot-time%";
		Info.pContext = pContext;
		Info.pMapFunc = nullptr;
		Info.pClientData = nullptr;

		if (m_pApp->FormatVarString(&Info)) {
			// Info.pszResult に結果の文字列が返される
			...
			// 不要になったらメモリを解放する
			m_pApp->MemoryFree(Info.pszResult);
		}

		// 不要になったらコンテキストを解放する
		m_pApp->FreeVarStringContext(pContext);
	@endcode
	@par
	マップ関数を指定すると、任意の変数を文字列に変換できます。
	上記の例でマップ関数を使う場合は以下のようになります。
	@par
	@code{.cpp}
		// マップ関数の定義
		BOOL CALLBACK VarStringMap(LPCWSTR pszVar, LPWSTR *ppszString, void *pClientData)
		{
			TVTest::CTVTestApp *pApp = static_cast<TVTest::CTVTestApp*>(pClientData);
			if (::lstrcmpiW(pszVar, L"my-var") == 0) {
				LPCWSTR pszMapString = L"replaced string"; // 置き換える文字列
				*ppszString = pApp->StringDuplicate(pszMapString);
				return TRUE;
			}
			return FALSE;
		}
	@endcode
	@code{.cpp}
		// VarStringFormatInfo にマップ関数と、マップ関数の pClientData 引数に渡すデータを指定する
		Info.pMapFunc = VarStringMap;
		Info.pClientData = m_pApp;
	@endcode
	*/
	bool FormatVarString(VarStringFormatInfo *pInfo)
	{
		pInfo->Size = sizeof(VarStringFormatInfo);
		return MsgFormatVarString(m_pParam, pInfo);
	}

	/**
	@brief 変数を登録

	変数を登録すると、TVTest の各所の変数文字列で利用できるようになります。
	変数の識別子は半角のアルファベットと数字、-記号のみ使用してください。
	TVTest で既に定義されている識別子と同じものを指定した場合、
	RegisterVariableInfo::Flags に TVTest::REGISTER_VARIABLE_FLAG_OVERRIDE が設定されている場合は
	プラグインで登録されたものが優先され、そうでない場合は TVTest での定義が優先されます。
	同じ識別子の変数を再登録すると、値が更新されます。

	@param[in] pInfo 変数登録の情報を格納した RegisterVariableInfo 構造体へのポインタ

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.14

	@par Corresponding message
	TVTest::MESSAGE_REGISTERVARIABLE

	@par Example
	@code{.cpp}
		// 変数 lucky-number を登録します。
		// 変数文字列の中で "%lucky-number%" を使うと、"777" に置き換えられます。
		TVTest::RegisterVariableInfo Info;
		Info.Flags          = TVTest::REGISTER_VARIABLE_FLAG_NONE;
		Info.pszKeyword     = L"lucky-number";
		Info.pszDescription = L"幸運の番号";
		Info.pszValue       = L"777";
		m_pRegisterVariable(&Info);
	@endcode
	@par
	変数の値が頻繁に変わるような場合は、動的に取得されるようにします。
	RegisterVariableInfo::pszValue に nullptr を指定すると、変数が必要になった段階で
	TVTest::EVENT_GETVARIABLE が呼ばれるので、そこで値を返します。
	@par
	@code{.cpp}
		// 変数の値が動的に取得されるようにする例
		TVTest::RegisterVariableInfo Info;
		Info.Flags          = TVTest::REGISTER_VARIABLE_FLAG_NONE;
		Info.pszKeyword     = L"tick-count";
		Info.pszDescription = L"Tick count";
		Info.pszValue       = nullptr;
		m_pApp->RegisterVariable(&Info);
	@endcode
	@code{.cpp}
		// EVENT_GETVARIABLE で値を返します。
		bool OnGetVariable(TVTest::GetVariableInfo *pInfo)
		{
			if (lstrcmpiW(pInfo->pszKeyword, L"tick-count") == 0) {
				// GetTickCount() の値を返す
				WCHAR szValue[16];
				wsprintf(szValue, L"%u", GetTickCount());
				pInfo->pszValue = m_pApp->StringDuplicate(szValue);
				return true;
			}
			return false;
		}
	@endcode
	*/
	bool RegisterVariable(RegisterVariableInfo *pInfo)
	{
		pInfo->Size = sizeof(RegisterVariableInfo);
		return MsgRegisterVariable(m_pParam, pInfo);
	}
#endif

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 15)
	/**
	@brief ダークモードの状態を取得

	@return ダークモードの状態フラグ DARK_MODE_STATUS_* の組み合わせ

	@since ver.0.0.15

	@par Corresponding message
	TVTest::MESSAGE_GETDARKMODESTATUS

	@par Example
	SetWindowDarkMode() の例を参照してください。
	*/
	DarkModeStatus GetDarkModeStatus()
	{
		return MsgGetDarkModeStatus(m_pParam);
	}

	/**
	@brief ダークモードの色かを取得

	指定された色がダークモードの色であるかを取得します。
	一般に GetColor() で取得した色に対して使用します。

	@param[in] Color 色

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.15

	@par Corresponding message
	TVTest::MESSAGE_ISDARKMODECOLOR

	@par Example
	SetWindowDarkMode() の例を参照してください。
	*/
	bool IsDarkModeColor(COLORREF Color)
	{
		return MsgIsDarkModeColor(m_pParam, Color);
	}

	/**
	@brief ウィンドウをダークモードにする

	ウィンドウのスクロールバーなどがダークモードになります。
	トップレベルウィンドウであれば、タイトルバーがダークモードになります。

	@param[in] hwnd ウィンドウのハンドル
	@param[in] fDark ダークモードにする場合は true、ダークモードを解除する場合は false

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.15

	@par Corresponding message
	TVTest::MESSAGE_SETWINDOWDARKMODE

	@par Example
	@code{.cpp}
		// パネルのウィンドウをダークモードにする
		void SetPanelWindowDarkMode(HWND hwnd)
		{
			if (m_pApp->GetDarkModeStatus() & TVTest::DARK_MODE_STATUS_PANEL_SUPPORTED)
				m_pApp->SetWindowDarkMode(hwnd, m_pApp->IsDarkModeColor(m_pApp->GetColor(L"PanelBack")));
		}
	@endcode
	*/
	bool SetWindowDarkMode(HWND hwnd, bool fDark)
	{
		return MsgSetWindowDarkMode(m_pParam, hwnd, fDark);
	}

	/**
	@brief Elementary Stream (ES) の情報のリストを取得

	@param[in,out] pList ES の情報のリストを取得する ElementaryStreamInfoList 構造体へのポインタ。
	                     ElementaryStreamInfoList::Media / ElementaryStreamInfoList::Flags / ElementaryStreamInfoList::ServiceID メンバを設定して呼び出します。
	                     ElementaryStreamInfoList::ESCount メンバにストリームの数が、 ElementaryStreamInfoList::ESList メンバに ElementaryStreamInfoList::ESCount の数分だけストリームの情報が返されます。
	                     不要になったら ElementaryStreamInfoList::ESList メンバのメモリを MsgMemoryFree() で解放します。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.15

	@par Corresponding message
	TVTest::MESSAGE_GETELEMENTARYSTREAMINFOLIST

	@par Example
	@code{.cpp}
		// 音声 ES の情報を取得する
		TVTest::ElementaryStreamInfoList List;
		List.Media = TVTest::ES_MEDIA_AUDIO;
		List.Flags = TVTest::ES_INFO_FLAG_NONE;
		List.ServiceID = 0;
		if (m_pApp->GetElementaryStreamInfoList(&List)) {
			// ESList に ESCount の要素数分の情報が取得される
			for (WORD i = 0; i < List.ESCount; i++) {
				const TVTest::ElementaryStreamInfo &ESInfo = List.ESList[i];
				...
			}

			// 不要になったら ESList のメモリを解放する
			m_pApp->MemoryFree(List.ESList);
		}
	@endcode
	*/
	bool GetElementaryStreamInfoList(ElementaryStreamInfoList *pList)
	{
		pList->Size = sizeof(ElementaryStreamInfoList);
		return MsgGetElementaryStreamInfoList(m_pParam, pList);
	}

	/**
	@brief Elementary Stream (ES) の情報のリストを取得

	@param[out] pList ES の情報のリストを取得する ElementaryStreamInfoList 構造体へのポインタ。
	                  ElementaryStreamInfoList::ESCount メンバにストリームの数が、 ElementaryStreamInfoList::ESList メンバに ElementaryStreamInfoList::ESCount の数分だけストリームの情報が返されます。
	                  不要になったら ElementaryStreamInfoList::ESList メンバのメモリを MemoryFree() で解放します。
	@param[in] Media メディアの種類
	@param[in] ServiceID サービス ID(0 で現在のサービス)
	@param[in] Flags フラグ(現在は常に TVTest::ES_INFO_FLAG_NONE)

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.15

	@par Corresponding message
	TVTest::MESSAGE_GETELEMENTARYSTREAMINFOLIST
	*/
	bool GetElementaryStreamInfoList(
		ElementaryStreamInfoList *pList, ElementaryStreamMediaType Media,
		WORD ServiceID = 0, ElementaryStreamInfoFlag Flags = ES_INFO_FLAG_NONE)
	{
		pList->Size = sizeof(ElementaryStreamInfoList);
		pList->Media = Media;
		pList->Flags = Flags;
		pList->ServiceID = ServiceID;
		return MsgGetElementaryStreamInfoList(m_pParam, pList);
	}

	/**
	@brief 全てのサービス数を取得

	現在のストリームのサービス数を取得します。
	この数は PAT に含まれる全てのサービスの数で、試聴できないサービスも含まれます。
	試聴できるサービスの数を取得する場合は GetService() を使用します。

	@return サービスの数

	@since ver.0.0.15

	@par Corresponding message
	TVTest::MESSAGE_GETSERVICECOUNT
	*/
	int GetServiceCount()
	{
		return MsgGetServiceCount(m_pParam);
	}

	/**
	@brief サービスの情報を取得する

	現在のストリームのサービスの情報を取得します。

	@param[in] Service サービスの指定。
	                   ServiceInfo2::Flags に TVTest::SERVICE_INFO2_FLAG_BY_ID が設定されている場合、サービス ID を指定します。
	                   それ以外の場合、インデックスを指定します。
	                   -1 を指定した場合、現在選択されているサービスの情報が取得されます。
	@param[in,out] pInfo サービスの情報を取得する ServiceInfo2 構造体へのポインタ。
	                     事前に ServiceInfo2::Flags メンバを設定しておきます。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.15

	@par Corresponding message
	TVTest::MESSAGE_GETSERVICEINFO2

	@sa GetServiceInfo()
	*/
	bool GetServiceInfo2(int Service, ServiceInfo2 *pInfo)
	{
		pInfo->Size = sizeof(ServiceInfo2);
		return MsgGetServiceInfo2(m_pParam, Service, pInfo);
	}

	/**
	@brief サービスの情報を取得する

	現在のストリームのサービスの情報を取得します。

	@param[in] Service サービスの指定。
	                   Flags 引数に TVTest::SERVICE_INFO2_FLAG_BY_ID が設定されている場合、サービス ID を指定します。
	                   それ以外の場合、インデックスを指定します。
	                   -1 を指定した場合、現在選択されているサービスの情報が取得されます。
	@param[out] pInfo サービスの情報を取得する ServiceInfo2 構造体へのポインタ
	@param[in] Flags フラグ(SERVICE_INFO2_FLAG_*)

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.15

	@par Corresponding message
	TVTest::MESSAGE_GETSERVICEINFO2

	@sa GetServiceInfo()
	*/
	bool GetServiceInfo2(int Service, ServiceInfo2 *pInfo, ServiceInfo2Flag Flags)
	{
		pInfo->Size = sizeof(ServiceInfo2);
		pInfo->Flags = Flags;
		return MsgGetServiceInfo2(m_pParam, Service, pInfo);
	}

	/**
	@brief サービスの情報のリストを取得

	@param[in,out] pList サービス情報のリストを取得する ServiceInfoList 構造体へのポインタ。
	                     ServiceInfoList::Flags メンバを設定して呼び出します。
	                     ServiceInfoList::ServiceCount メンバにサービスの数が、 ServiceInfoList::ServiceList メンバに ServiceInfoList::ServiceCount の数分だけサービスの情報が返されます。
	                     不要になったら ServiceInfoList::ServiceList メンバのメモリを MsgMemoryFree() で解放します。
	                     ServiceInfoList::Flags に TVTest::SERVICE_INFO_LIST_FLAG_SDT_ACTUAL が指定されている場合、 ServiceInfo2::PMT_PID と ServiceInfo2::PCR_PID は取得されません。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.15

	@par Corresponding message
	TVTest::MESSAGE_GETSERVICEINFOLIST

	@par Example
	@code{.cpp}
		// 全てのサービスの情報を取得する
		TVTest::ServiceInfoList List;
		List.Flags = TVTest::SERVICE_INFO_LIST_FLAG_NONE;
		if (m_pApp->GetServiceInfoList(&List)) {
			// SrviceList に ServiceCount の数分の情報が取得される
			for (DWORD i = 0; i < List.ServiceCount; i++) {
				const TVTest::ServiceInfo2 &Info = List.ServiceList[i];
				...
			}

			// 不要になったら ServiceList のメモリを解放する
			m_pApp->MemoryFree(List.ServiceList);
		}
	@endcode
	*/
	bool GetServiceInfoList(ServiceInfoList *pList)
	{
		pList->Size = sizeof(ServiceInfoList);
		return MsgGetServiceInfoList(m_pParam, pList);
	}

	/**
	@brief サービスの情報のリストを取得

	@param[out] pList サービス情報のリストを取得する ServiceInfoList 構造体へのポインタ。
	                  ServiceInfoList::ServiceCount メンバにサービスの数が、 ServiceInfoList::ServiceList メンバに ServiceInfoList::ServiceCount の数分だけサービスの情報が返されます。
	                  不要になったら ServiceInfoList::ServiceList メンバのメモリを MemoryFree() で解放します。
	                  Flags 引数に TVTest::SERVICE_INFO_LIST_FLAG_SDT_ACTUAL が指定されている場合、 ServiceInfo2::PMT_PID と ServiceInfo2::PCR_PID は取得されません。
	@param[in] Flags フラグ(SERVICE_INFO_LIST_FLAG_*)

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.15

	@par Corresponding message
	TVTest::MESSAGE_GETSERVICEINFOLIST
	*/
	bool GetServiceInfoList(ServiceInfoList *pList, ServiceInfoListFlag Flags)
	{
		pList->Size = sizeof(ServiceInfoList);
		pList->Flags = Flags;
		return MsgGetServiceInfoList(m_pParam, pList);
	}

	/**
	@brief 音声の情報を取得

	@param[out] pInfo 音声の情報を取得する AudioInfo 構造体へのポインタ。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.15

	@par Corresponding message
	TVTest::MESSAGE_GETAUDIOINFO
	*/
	bool GetAudioInfo(AudioInfo *pInfo)
	{
		pInfo->Size = sizeof(AudioInfo);
		return MsgGetAudioInfo(m_pParam, pInfo);
	}

	/**
	@brief Elementary Stream (ES) の数を取得

	@param[in] Media 数を取得する ES の種類
	@param[in] ServiceID サービス ID。0 の場合、現在のサービス。

	@return ES の数

	@since ver.0.0.15

	@par Corresponding message
	TVTest::MESSAGE_GETELEMENTARYSTREAMCOUNT
	*/
	int GetElementaryStreamCount(ElementaryStreamMediaType Media, WORD ServiceID = 0)
	{
		return MsgGetElementaryStreamCount(m_pParam, Media, ServiceID);
	}

	/**
	@brief 音声ストリームの数を取得

	@return 音声ストリームの数

	@since ver.0.0.15

	@par Corresponding message
	TVTest::MESSAGE_GETELEMENTARYSTREAMCOUNT
	*/
	int GetAudioStreamCount()
	{
		return MsgGetAudioStreamCount(m_pParam);
	}

	/**
	@brief 音声を選択する

	@param[in] pInfo 音声選択の情報を格納した AudioSelectInfo 構造体へのポインタ。
	                 AudioSelectInfo::Flags に TVTest::AUDIO_SELECT_FLAG_COMPONENT_TAG が指定されている場合、 AudioSelectInfo::ComponentTag メンバでコンポーネントタグを指定します。
	                 それ以外の場合、AudioSelectInfo::Index メンバに音声ストリームのインデックスを指定します。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.15

	@par Corresponding message
	TVTest::MESSAGE_SELECTAUDIO
	*/
	bool SelectAudio(const AudioSelectInfo *pInfo)
	{
		return MsgSelectAudio(m_pParam, pInfo);
	}

	/**
	@brief 選択された音声を取得する

	@param[in,out] pInfo 音声選択の情報を取得する AudioSelectInfo 構造体へのポインタ。
	                     事前に AudioSelectInfo::Flags メンバを設定して呼び出します。
	                     AudioSelectInfo::Flags は今のところ常に TVTest::SELECT_AUDIO_FLAG_NONE に設定します。

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.15

	@par Corresponding message
	TVTest::MESSAGE_GETSELECTEDAUDIO
	*/
	bool GetSelectedAudio(AudioSelectInfo *pInfo)
	{
		pInfo->Size = sizeof(AudioSelectInfo);
		return MsgGetSelectedAudio(m_pParam, pInfo);
	}

	/**
	@brief 現在の番組情報を取得する

	@param[in] ServiceID サービス ID。0 を指定すると、現在のサービスの情報が取得されます。
	@param[in] fNext 現在の番組の情報を取得する場合は true、次の番組の情報を取得する場合は false を指定します。

	@return 現在の番組情報を格納した EpgEventInfo 構造体へのポインタ。
	        情報が取得できなかった場合は nullptr が返ります。
	        取得した情報が不要になった場合、 FreeEpgEventInfo() で解放します。

	@since ver.0.0.15

	@par Corresponding message
	TVTest::MESSAGE_GETCURRENTEPGEVENTINFO

	@sa GetCurrentProgramInfo()
	*/
	EpgEventInfo *GetCurrentEpgEventInfo(WORD ServiceID = 0, bool fNext = false)
	{
		return MsgGetCurrentEpgEventInfo(m_pParam, ServiceID, fNext);
	}
#endif
};

/**
@brief TVTest プラグインクラス

プラグインをクラスとして記述する場合に使用する抽象クラスです。
このクラスを各プラグインで派生させて、プラグインの内容を記述します。
*/
class CTVTestPlugin
{
protected:
	PluginParam *m_pPluginParam; /**< プラグインパラメータ */
	CTVTestApp *m_pApp;          /**< アプリケーションクラス */

public:
	/**
	@brief コンストラクタ
	*/
	CTVTestPlugin() : m_pPluginParam(nullptr), m_pApp(nullptr) {}

	/**
	@brief デストラクタ
	*/
	virtual ~CTVTestPlugin() { delete m_pApp; }

	/**
	@brief プラグインパラメータを設定する

	TVTInitialize() 内で呼び出されます。

	@param[in] pParam 設定する PluginParam 構造体へのポインタ
	*/
	void SetPluginParam(PluginParam *pParam)
	{
		m_pPluginParam = pParam;
		m_pApp = new CTVTestApp(pParam);
	}

	/**
	@brief 準拠するプラグイン仕様のバージョンを取得する

	TVTGetVersion() 内で呼び出されます。

	@return プラグイン仕様のバージョン
	*/
	virtual DWORD GetVersion() { return TVTEST_PLUGIN_VERSION; }

	/**
	@brief プラグインの情報を取得する

	TVTGetPluginInfo() 内で呼び出されます。

	@param[out] pInfo プラグインの情報を取得する PluginInfo 構造体へのポインタ

	@retval true 正常終了
	@retval false エラー発生
	*/
	virtual bool GetPluginInfo(PluginInfo *pInfo) = 0;

	/**
	@brief プラグインの初期化処理を行う

	TVTInitialize() 内で呼び出されます。

	@retval true 正常終了
	@retval false エラー発生
	*/
	virtual bool Initialize() { return true; }

	/**
	@brief プラグインの終了処理を行う

	TVTFinalize() 内で呼び出します。

	@retval true 正常終了
	@retval false エラー発生
	*/
	virtual bool Finalize() { return true; }
};

/**
@brief イベントハンドルクラス

イベント処理用クラスです。
このクラスを派生させてイベント処理を行うことができます。
このクラスを使わずに直接イベントコードを扱うこともできます。
イベントコールバック関数として登録した関数内で HandleEvent() を呼びます。

イベントハンドラは、特に記述の無いものは何か処理したら true を返します

以下は実装例です。

@code{.cpp}
	class CMyEventHandler : public TVTest::CTVTestEventHandler
	{
	public:
		virtual bool OnPluginEnable(bool fEnable)
		{
			if (fEnable) {
				if (MessageBox(nullptr, TEXT("有効にするよ"), TEXT("イベント"), MB_OKCANCEL) != IDOK) {
					return false;
				}
			}
			return true;
		}
	};

	CMyEventHandler Handler;

	// この関数がイベントコールバック関数として登録されているものとします
	LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
	{
		return Handler.HandleEvent(Event, lParam1, lParam2, pClientData);
	}
@endcode
*/
class CTVTestEventHandler
{
protected:
	void *m_pClientData; /**< イベントコールバック関数に渡されたデータ */

	/**
	@brief 有効状態が変化する

	@param[in] fEnable プラグインが有効化される場合は true、無効化される場合は false

	@retval true 変化を受け入れる
	@retval false 変化を拒否する

	@since ver.0.0.0

	@sa TVTest::EVENT_PLUGINENABLE
	*/
	virtual bool OnPluginEnable(bool fEnable) { return false; }

	/**
	@brief 設定を行う

	プラグインの設定ダイアログを表示します。
	プラグインのフラグに TVTest::PLUGIN_FLAG_HASSETTINGS が設定されている場合に呼ばれます。

	@param[in] hwndOwner 設定ダイアログのオーナーウィンドウのハンドル

	@retval true 設定が OK された
	@retval false 設定がキャンセルされたかエラーが発生した

	@since ver.0.0.0

	@sa TVTest::EVENT_PLUGINSETTINGS
	*/
	virtual bool OnPluginSettings(HWND hwndOwner) { return false; }

	/**
	@brief チャンネルが変更された

	@retval true 何か処理した
	@retval false 何もしなかった

	@since ver.0.0.0

	@sa TVTest::EVENT_CHANNELCHANGE
	*/
	virtual bool OnChannelChange() { return false; }

	/**
	@brief サービスが変更された

	@retval true 何か処理した
	@retval false 何もしなかった

	@since ver.0.0.0

	@sa TVTest::EVENT_SERVICECHANGE
	*/
	virtual bool OnServiceChange() { return false; }

	/**
	@brief ドライバが変更された

	@retval true 何か処理した
	@retval false 何もしなかった

	@since ver.0.0.0

	@sa TVTest::EVENT_DRIVERCHANGE
	*/
	virtual bool OnDriverChange() { return false; }

	/**
	@brief サービスの構成が変化した

	@retval true 何か処理した
	@retval false 何もしなかった

	@since ver.0.0.0

	@sa TVTest::EVENT_SERVICEUPDATE
	*/
	virtual bool OnServiceUpdate() { return false; }

	/**
	@brief 録画状態が変化した

	@param[in] Status 新しい録画状態を表す RECORD_STATUS_* の値

	@retval true 何か処理した
	@retval false 何もしなかった

	@since ver.0.0.0

	@sa TVTest::EVENT_RECORDSTATUSCHANGE
	*/
	virtual bool OnRecordStatusChange(int Status) { return false; }

	/**
	@brief 全画面表示状態が変化した

	@param[in] fFullscreen 全画面表示になった場合は true、全画面表示が終了した場合は false

	@retval true 何か処理した
	@retval false 何もしなかった

	@since ver.0.0.0

	@sa TVTest::EVENT_FULLSCREENCHANGE
	*/
	virtual bool OnFullscreenChange(bool fFullscreen) { return false; }

	/**
	@brief 再生状態が変化した

	@param[in] fPreview 再生状態になった場合は true、再生オフになった場合は false

	@retval true 何か処理した
	@retval false 何もしなかった

	@since ver.0.0.0

	@sa TVTest::EVENT_PREVIEWCHANGE
	*/
	virtual bool OnPreviewChange(bool fPreview) { return false; }

	/**
	@brief 音量が変化した

	@param[in] Volume 音量(0-100)
	@param[in] fMute 消音状態の場合は true、消音状態でない場合は false

	@retval true 何か処理した
	@retval false 何もしなかった

	@since ver.0.0.0

	@sa TVTest::EVENT_VOLUMECHANGE
	*/
	virtual bool OnVolumeChange(int Volume, bool fMute) { return false; }

	/**
	@brief ステレオモードが変化した

	@param[in] StereoMode 新しいステレオモードを表す STEREOMODE_* の値

	@retval true 何か処理した
	@retval false 何もしなかった

	@since ver.0.0.0

	@sa TVTest::EVENT_STEREOMODECHANGE
	*/
	virtual bool OnStereoModeChange(int StereoMode) { return false; }

	/**
	@brief 色の設定が変化した

	@retval true 何か処理した
	@retval false 何もしなかった

	@since ver.0.0.0

	@sa TVTest::EVENT_COLORCHANGE
	*/
	virtual bool OnColorChange() { return false; }

#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 3)
	/**
	@brief 待機状態が変化した

	@param[in] fStandby 待機状態になった場合は true、待機状態が解除された場合は false

	@retval true 何か処理した
	@retval false 何もしなかった

	@since ver.0.0.3

	@sa TVTest::EVENT_STANDBY
	*/
	virtual bool OnStandby(bool fStandby) { return false; }

	/**
	@brief コマンドが選択された

	@param[in] ID 選択されたコマンドの識別子

	@retval true コマンドを処理した
	@retval false コマンドを処理しなかった

	@since ver.0.0.3

	@sa TVTest::EVENT_COMMAND
	*/
	virtual bool OnCommand(int ID) { return false; }
#endif
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 4)
	/**
	@brief 複数起動禁止時に複数起動された

	@param[in] pszCommandLine 新しく起動されたプロセスのコマンドライン引数

	@retval true 何か処理した
	@retval false 何もしなかった

	@since ver.0.0.4

	@sa TVTest::EVENT_EXECUTE
	*/
	virtual bool OnExecute(LPCWSTR pszCommandLine) { return false; }
#endif
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 5)
	/**
	@brief リセットされた

	@retval true 何か処理した
	@retval false 何もしなかった

	@since ver.0.0.5

	@sa TVTest::EVENT_RESET
	*/
	virtual bool OnReset() { return false; }

	/**
	@brief ステータス(MsgGetStatus() で取得できる内容)がリセットされた

	@retval true 何か処理した
	@retval false 何もしなかった

	@since ver.0.0.5

	@sa TVTest::EVENT_STATUSRESET
	*/
	virtual bool OnStatusReset() { return false; }

	/**
	@brief 音声ストリームが変更された

	@param[in] Stream 音声ストリームのインデックス

	@retval true 何か処理した
	@retval false 何もしなかった

	@since ver.0.0.5

	@sa TVTest::EVENT_AUDIOSTREAMCHANGE
	*/
	virtual bool OnAudioStreamChange(int Stream) { return false; }
#endif
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 9)
	/**
	@brief TVTest の設定が変更された

	@retval true 何か処理した
	@retval false 何もしなかった

	@since ver.0.0.9

	@sa TVTest::EVENT_SETTINGSCHANGE
	*/
	virtual bool OnSettingsChange() { return false; }
#endif
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 10)
	/**
	@brief TVTestのウィンドウが閉じられる

	@retval true 何か処理した
	@retval false 何もしなかった

	@since ver.0.0.10

	@sa TVTest::EVENT_CLOSE
	*/
	virtual bool OnClose() { return false; }

	/**
	@brief 録画が開始される

	@param[in,out] pInfo 録画の情報が格納された StartRecordInfo 構造体へのポインタ

	@retval true 何か処理した
	@retval false 何もしなかった

	@since ver.0.0.10

	@sa TVTest::EVENT_STARTRECORD
	*/
	virtual bool OnStartRecord(StartRecordInfo *pInfo) { return false; }

	/**
	@brief 録画ファイルの切り替えが行われた

	@param[in] pszFileName 新しいファイルのパス

	@retval true 何か処理した
	@retval false 何もしなかった

	@since ver.0.0.10

	@sa TVTest::EVENT_RELAYRECORD
	*/
	virtual bool OnRelayRecord(LPCWSTR pszFileName) { return false; }
#endif
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 11)
	/**
	@brief コントローラの対象の設定

	@param[in] hwnd コントローラの操作対象となるウィンドウのハンドル

	@retval true 何か処理した
	@retval false 何もしなかった

	@since ver.0.0.11

	@sa TVTest::EVENT_CONTROLLERFOCUS
	*/
	virtual bool OnControllerFocus(HWND hwnd) { return false; }
#endif
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 13)
	/**
	@brief 起動処理が終了した

	@since ver.0.0.13

	@sa TVTest::EVENT_STARTUPDONE
	*/
	virtual void OnStartupDone() {}

	/**
	@brief 番組表の初期化

	@param[in] hwnd 番組表のウィンドウハンドル

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.13

	@sa TVTest::EVENT_PROGRAMGUIDE_INITIALIZE
	*/
	virtual bool OnProgramGuideInitialize(HWND hwnd) { return true; }

	/**
	@brief 番組表の終了

	@param[in] hwnd 番組表のウィンドウハンドル

	@retval true 正常終了
	@retval false エラー発生

	@since ver.0.0.13

	@sa TVTest::EVENT_PROGRAMGUIDE_FINALIZE
	*/
	virtual bool OnProgramGuideFinalize(HWND hwnd) { return true; }

	/**
	@brief 番組表のコマンドの実行

	@param[in] Command 実行するコマンドの識別子
	@param[in] pParam 実行するコマンドのパラメータ

	@retval true コマンドを実行した
	@retval false コマンドを実行しなかった

	@since ver.0.0.13

	@sa TVTest::EVENT_PROGRAMGUIDE_COMMAND
	*/
	virtual bool OnProgramGuideCommand(
		UINT Command, const ProgramGuideCommandParam *pParam) { return false; }

	/**
	@brief 番組表のメニューの初期化

	@param[in] pInfo メニューの情報

	@return メニューに追加したコマンドの数

	@since ver.0.0.13

	@sa TVTest::EVENT_PROGRAMGUIDE_INITIALIZEMENU
	*/
	virtual int OnProgramGuideInitializeMenu(
		const ProgramGuideInitializeMenuInfo *pInfo) { return 0; }

	/**
	@brief 番組表のメニューが選択された

	@param[in] Command 選択されたコマンドのインデックス

	@retval true コマンドを実行した
	@retval false コマンドを実行しなかった

	@since ver.0.0.13

	@sa TVTest::EVENT_PROGRAMGUIDEMENUSELECTED
	*/
	virtual bool OnProgramGuideMenuSelected(UINT Command) { return false; }

	/**
	@brief 番組表の番組の背景描画

	@param[in] pProgramInfo 番組の情報
	@param[in] pInfo 背景描画の情報

	@retval true 背景を描画した
	@retval false 描画しなかった

	@since ver.0.0.13

	@sa TVTest::EVENT_PROGRAMGUIDE_PROGRAM_DRAWBACKGROUND
	*/
	virtual bool OnProgramGuideProgramDrawBackground(
		const ProgramGuideProgramInfo *pProgramInfo,
		const ProgramGuideProgramDrawBackgroundInfo *pInfo) { return false; }

	/**
	@brief 番組表の番組のメニュー初期化

	@param[in] pProgramInfo 番組の情報
	@param[in] pInfo メニューの情報

	@return メニューに追加したコマンドの数

	@since ver.0.0.13

	@sa TVTest::EVENT_PROGRAMGUIDE_PROGRAM_INITIALIZEMENU
	*/
	virtual int OnProgramGuideProgramInitializeMenu(
		const ProgramGuideProgramInfo *pProgramInfo,
		const ProgramGuideProgramInitializeMenuInfo *pInfo) { return 0; }

	/**
	@brief 番組表の番組のメニューが選択された

	@param[in] pProgramInfo 番組の情報
	@param[in] Command 選択されたコマンドのインデックス

	@retval true コマンドを実行した
	@retval false コマンドを実行しなかった

	@since ver.0.0.13

	@sa TVTest::EVENT_PROGRAMGUIDE_PROGRAM_MENUSELECTED
	*/
	virtual bool OnProgramGuideProgramMenuSelected(
		const ProgramGuideProgramInfo *pProgramInfo, UINT Command) { return false; }
#endif
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 14)
	/**
	@brief フィルタグラフの初期化開始

	@param[in] pInfo フィルタグラフの情報

	@since ver.0.0.14

	@sa TVTest::EVENT_FILTERGRAPH_INITIALIZE
	*/
	virtual void OnFilterGraphInitialize(FilterGraphInfo *pInfo) {}

	/**
	@brief フィルタグラフが初期化終了

	@param[in] pInfo フィルタグラフの情報

	@since ver.0.0.14

	@sa TVTest::EVENT_FILTERGRAPH_INITIALIZED
	*/
	virtual void OnFilterGraphInitialized(FilterGraphInfo *pInfo) {}

	/**
	@brief フィルタグラフの終了処理開始

	@param[in] pInfo フィルタグラフの情報

	@since ver.0.0.14

	@sa TVTest::EVENT_FILTERGRAPH_FINALIZE
	*/
	virtual void OnFilterGraphFinalize(FilterGraphInfo *pInfo) {}

	/**
	@brief フィルタグラフが終了処理終了

	@param[in] pInfo フィルタグラフの情報

	@since ver.0.0.14

	@sa TVTest::EVENT_FILTERGRAPH_FINALIZED
	*/
	virtual void OnFilterGraphFinalized(FilterGraphInfo *pInfo) {}

	/**
	@brief コマンドアイコンの描画

	@param[in] pInfo コマンドアイコン描画の情報

	@retval true 描画を行った
	@retval false 描画を行わなかった

	@since ver.0.0.14

	@sa TVTest::EVENT_DRAWCOMMANDICON
	*/
	virtual bool OnDrawCommandIcon(DrawCommandIconInfo *pInfo) { return false; }

	/**
	@brief ステータス項目の描画

	@param[in] pInfo ステータス項目描画の情報

	@retval true 描画を行った
	@retval false 描画を行わなかった

	@since ver.0.0.14

	@sa TVTest::EVENT_STATUSITEM_DRAW
	*/
	virtual bool OnStatusItemDraw(StatusItemDrawInfo *pInfo) { return false; }

	/**
	@brief ステータス項目の通知

	@param[in] pInfo ステータス項目通知の情報

	@retval true 何か処理した
	@retval false 何もしなかった

	@since ver.0.0.14

	@sa TVTest::EVENT_STATUSITEM_NOTIFY
	*/
	virtual bool OnStatusItemNotify(StatusItemEventInfo *pInfo) { return false; }

	/**
	@brief ステータス項目のマウスイベント

	@param[in] pInfo ステータス項目のマウスイベントの情報

	@retval true 何か処理した
	@retval false 何もしなかった

	@since ver.0.0.14

	@sa TVTest::EVENT_STATUSITEM_MOUSE
	*/
	virtual bool OnStatusItemMouseEvent(StatusItemMouseEventInfo *pInfo) { return false; }

	/**
	@brief パネル項目の通知

	@param[in] pInfo パネル項目通知の情報

	@retval true 何か処理した
	@retval false 何もしなかった

	@since ver.0.0.14

	@sa TVTest::EVENT_PANELITEM_NOTIFY
	*/
	virtual bool OnPanelItemNotify(PanelItemEventInfo *pInfo) { return false; }

	/**
	@brief お気に入りチャンネルが変更された

	@since ver.0.0.14

	@sa TVTest::EVENT_FAVORITESCHANGED
	*/
	virtual void OnFavoritesChanged() {}

	/**
	@brief ワンセグモードが変わった

	@param[in] f1SegMode ワンセグモードが有効になった場合は true、無効になった場合は false

	@since ver.0.0.14

	@sa TVTest::EVENT_1SEGMODECHANGED
	*/
	virtual void On1SegModeChanged(bool f1SegMode) {}

	/**
	@brief 変数を取得

	@param[in,out] pInfo 変数取得の情報

	@retval true 変数の値が取得された
	@retval false 変数の値が取得されなかった

	@since ver.0.0.14

	@sa TVTest::EVENT_GETVARIABLE
	*/
	virtual bool OnGetVariable(GetVariableInfo *pInfo) { return false; }
#endif
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 15)
	/**
	@brief ダークモードの状態が変わった

	@param[in] fDarkMode ダークモードになった場合は true、ダークモードが解除された場合は false

	@since ver.0.0.15

	@sa TVTest::EVENT_DARKMODECHANGED
	*/
	virtual void OnDarkModeChanged(bool fDarkMode) {}

	/**
	@brief メインウィンドウのダークモードの状態が変わった

	@param[in] fDarkMode ダークモードになった場合は true、ダークモードが解除された場合は false

	@since ver.0.0.15

	@sa TVTest::EVENT_MAINWINDOWDARKMODECHANGED
	*/
	virtual void OnMainWindowDarkModeChanged(bool fDarkMode) {}

	/**
	@brief 番組表のダークモードの状態が変わった

	@param[in] fDarkMode ダークモードになった場合は true、ダークモードが解除された場合は false

	@since ver.0.0.15

	@sa TVTest::EVENT_PROGRAMGUIDEDARKMODECHANGED
	*/
	virtual void OnProgramGuideDarkModeChanged(bool fDarkMode) {}

	/**
	@brief 映像の形式が変わった

	@since ver.0.0.15

	@sa TVTest::EVENT_VIDEOFORMATCHANGE
	*/
	virtual void OnVideoFormatChange() {}

	/**
	@brief 音声の形式が変わった

	@since ver.0.0.15

	@sa TVTest::EVENT_AUDIOFORMATCHANGE
	*/
	virtual void OnAudioFormatChange() {}

	/**
	@brief 番組が変わった

	@since ver.0.0.15

	@sa TVTest::EVENT_EVENTCHANGED
	*/
	virtual void OnEventChanged() {}

	/**
	@brief 番組情報が変化した

	@since ver.0.0.15

	@sa TVTest::EVENT_EVENTINFOCHANGED
	*/
	virtual void OnEventInfoChanged() {}
#endif

public:
	/**
	@brief デストラクタ
	*/
	virtual ~CTVTestEventHandler() {}

	/**
	@brief イベントに応じてハンドラを呼び出す

	通常はイベントコールバック関数として登録した関数から呼び出します。

	@param[in] Event イベントコード(EVENT_*)
	@param[in] lParam1 イベント毎のパラメータ
	@param[in] lParam2 イベント毎のパラメータ
	@param[in] pClientData イベントコールバック関数の登録時に渡したデータ

	@return 各イベント毎に規定された値。
	        イベントを処理しなかった場合は 0。
	*/
	LRESULT HandleEvent(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
	{
		m_pClientData = pClientData;
		switch (Event) {
		case EVENT_PLUGINENABLE:        return OnPluginEnable(lParam1 != 0);
		case EVENT_PLUGINSETTINGS:      return OnPluginSettings(reinterpret_cast<HWND>(lParam1));
		case EVENT_CHANNELCHANGE:       return OnChannelChange();
		case EVENT_SERVICECHANGE:       return OnServiceChange();
		case EVENT_DRIVERCHANGE:        return OnDriverChange();
		case EVENT_SERVICEUPDATE:       return OnServiceUpdate();
		case EVENT_RECORDSTATUSCHANGE:  return OnRecordStatusChange(static_cast<int>(lParam1));
		case EVENT_FULLSCREENCHANGE:    return OnFullscreenChange(lParam1 != 0);
		case EVENT_PREVIEWCHANGE:       return OnPreviewChange(lParam1 != 0);
		case EVENT_VOLUMECHANGE:        return OnVolumeChange(static_cast<int>(lParam1), lParam2 != 0);
		case EVENT_STEREOMODECHANGE:    return OnStereoModeChange(static_cast<int>(lParam1));
		case EVENT_COLORCHANGE:         return OnColorChange();
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 3)
		case EVENT_STANDBY:             return OnStandby(lParam1 != 0);
		case EVENT_COMMAND:             return OnCommand(static_cast<int>(lParam1));
#endif
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 4)
		case EVENT_EXECUTE:             return OnExecute(reinterpret_cast<LPCWSTR>(lParam1));
#endif
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 5)
		case EVENT_RESET:               return OnReset();
		case EVENT_STATUSRESET:         return OnStatusReset();
		case EVENT_AUDIOSTREAMCHANGE:   return OnAudioStreamChange(static_cast<int>(lParam1));
#endif
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 9)
		case EVENT_SETTINGSCHANGE:      return OnSettingsChange();
#endif
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 10)
		case EVENT_CLOSE:               return OnClose();
		case EVENT_STARTRECORD:         return OnStartRecord(reinterpret_cast<StartRecordInfo*>(lParam1));
		case EVENT_RELAYRECORD:         return OnRelayRecord(reinterpret_cast<LPCWSTR>(lParam1));
#endif
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 11)
		case EVENT_CONTROLLERFOCUS:     return OnControllerFocus(reinterpret_cast<HWND>(lParam1));
#endif
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 13)
		case EVENT_STARTUPDONE:         OnStartupDone(); return 0;
		case EVENT_PROGRAMGUIDE_INITIALIZE: return OnProgramGuideInitialize(reinterpret_cast<HWND>(lParam1));
		case EVENT_PROGRAMGUIDE_FINALIZE:   return OnProgramGuideFinalize(reinterpret_cast<HWND>(lParam1));
		case EVENT_PROGRAMGUIDE_COMMAND:
			return OnProgramGuideCommand(
				static_cast<UINT>(lParam1),
				reinterpret_cast<const ProgramGuideCommandParam*>(lParam2));
		case EVENT_PROGRAMGUIDE_INITIALIZEMENU:
			return OnProgramGuideInitializeMenu(
				reinterpret_cast<const ProgramGuideInitializeMenuInfo*>(lParam1));
		case EVENT_PROGRAMGUIDE_MENUSELECTED:
			return OnProgramGuideMenuSelected(static_cast<UINT>(lParam1));
		case EVENT_PROGRAMGUIDE_PROGRAM_DRAWBACKGROUND:
			return OnProgramGuideProgramDrawBackground(
				reinterpret_cast<const ProgramGuideProgramInfo*>(lParam1),
				reinterpret_cast<const ProgramGuideProgramDrawBackgroundInfo*>(lParam2));
		case EVENT_PROGRAMGUIDE_PROGRAM_INITIALIZEMENU:
			return OnProgramGuideProgramInitializeMenu(
				reinterpret_cast<const ProgramGuideProgramInfo*>(lParam1),
				reinterpret_cast<const ProgramGuideProgramInitializeMenuInfo*>(lParam2));
		case EVENT_PROGRAMGUIDE_PROGRAM_MENUSELECTED:
			return OnProgramGuideProgramMenuSelected(
				reinterpret_cast<const ProgramGuideProgramInfo*>(lParam1), static_cast<UINT>(lParam2));
#endif
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 14)
		case EVENT_FILTERGRAPH_INITIALIZE:
			OnFilterGraphInitialize(reinterpret_cast<FilterGraphInfo*>(lParam1));
			return 0;
		case EVENT_FILTERGRAPH_INITIALIZED:
			OnFilterGraphInitialized(reinterpret_cast<FilterGraphInfo*>(lParam1));
			return 0;
		case EVENT_FILTERGRAPH_FINALIZE:
			OnFilterGraphFinalize(reinterpret_cast<FilterGraphInfo*>(lParam1));
			return 0;
		case EVENT_FILTERGRAPH_FINALIZED:
			OnFilterGraphFinalized(reinterpret_cast<FilterGraphInfo*>(lParam1));
			return 0;
		case EVENT_DRAWCOMMANDICON:
			return OnDrawCommandIcon(reinterpret_cast<DrawCommandIconInfo*>(lParam1));
		case EVENT_STATUSITEM_DRAW:
			return OnStatusItemDraw(reinterpret_cast<StatusItemDrawInfo*>(lParam1));
		case EVENT_STATUSITEM_NOTIFY:
			return OnStatusItemNotify(reinterpret_cast<StatusItemEventInfo*>(lParam1));
		case EVENT_STATUSITEM_MOUSE:
			return OnStatusItemMouseEvent(reinterpret_cast<StatusItemMouseEventInfo*>(lParam1));
		case EVENT_PANELITEM_NOTIFY:
			return OnPanelItemNotify(reinterpret_cast<PanelItemEventInfo*>(lParam1));
		case EVENT_FAVORITESCHANGED:
			OnFavoritesChanged();
			return 0;
		case EVENT_1SEGMODECHANGED:
			On1SegModeChanged(lParam1 != 0);
			return 0;
		case EVENT_GETVARIABLE:
			return OnGetVariable(reinterpret_cast<GetVariableInfo*>(lParam1));
#endif
#if TVTEST_PLUGIN_VERSION >= TVTEST_PLUGIN_VERSION_(0, 0, 15)
		case EVENT_DARKMODECHANGED:
			OnDarkModeChanged(lParam1 != 0);
			return 0;
		case EVENT_MAINWINDOWDARKMODECHANGED:
			OnMainWindowDarkModeChanged(lParam1 != 0);
			return 0;
		case EVENT_PROGRAMGUIDEDARKMODECHANGED:
			OnProgramGuideDarkModeChanged(lParam1 != 0);
			return 0;
		case EVENT_VIDEOFORMATCHANGE:
			OnVideoFormatChange();
			return 0;
		case EVENT_AUDIOFORMATCHANGE:
			OnAudioFormatChange();
			return 0;
		case EVENT_EVENTCHANGED:
			OnEventChanged();
			return 0;
		case EVENT_EVENTINFOCHANGED:
			OnEventInfoChanged();
			return 0;
#endif
		}
		return 0;
	}
};


} // namespace TVTest


#include <poppack.h>


/**
@brief プラグインの準拠するプラグイン仕様のバージョンを返す

プラグインがロードされると最初にこの関数が呼ばれます。
TVTest が対応していないバージョンが返された場合は、すぐにアンロードされます。

この関数はプラグインの作成者が実装します。
TVTEST_PLUGIN_CLASS_IMPLEMENT が \#define されている場合、この関数は自動的に定義されます。

@return 準拠するプラグイン仕様のバージョン。
        通常は #TVTEST_PLUGIN_VERSION を返します。
*/
TVTEST_EXPORT(DWORD) TVTGetVersion();

/**
@brief プラグインの情報を取得する

TVTGetVersion() の次に呼ばれるので、プラグインの情報を TVTest::PluginInfo 構造体に設定します。

この関数はプラグインの作成者が実装します。
TVTEST_PLUGIN_CLASS_IMPLEMENT が \#define されている場合、この関数は自動的に定義されます。

@param[out] pInfo プラグインの情報を取得する TVTest::PluginInfo 構造体へのポインタ。

@retval TRUE 正常終了
@retval FALSE エラー発生。すぐにアンロードされます。
*/
TVTEST_EXPORT(BOOL) TVTGetPluginInfo(TVTest::PluginInfo *pInfo);

/**
@brief プラグインの初期化を行う

TVTGetPluginInfo() の次に呼ばれるので、初期化処理を行います。

この関数はプラグインの作成者が実装します。
TVTEST_PLUGIN_CLASS_IMPLEMENT が \#define されている場合、この関数は自動的に定義されます。

@param[in,out] pParam プラグインのパラメータを格納した TVTest::PluginParam 構造体へのポインタ。

@retval TRUE 正常終了
@retval FALSE エラー発生。すぐにアンロードされます。
*/
TVTEST_EXPORT(BOOL) TVTInitialize(TVTest::PluginParam *pParam);

/**
@brief プラグインの終了処理を行う

プラグインがアンロードされる前に呼ばれるので、終了処理を行います。
この関数が呼ばれるのは TVTInitialize() 関数が TRUE を返した場合だけです。

この関数はプラグインの作成者が実装します。
TVTEST_PLUGIN_CLASS_IMPLEMENT が \#define されている場合、この関数は自動的に定義されます。

@retval TRUE 正常終了
@retval FALSE エラー発生
*/
TVTEST_EXPORT(BOOL) TVTFinalize();


#ifdef TVTEST_PLUGIN_CLASS_IMPLEMENT
/*
	プラグインをクラスとして記述できるようにするための、エクスポート関数の実装です。
	これを使えば、エクスポート関数を自分で実装する必要がなくなります。
*/


/**
@brief プラグインクラスのインスタンスを生成する

この関数はプラグイン作成者が実装します。

@return TVTest::CTVTestPlugin クラスのインスタンス
*/
TVTest::CTVTestPlugin *CreatePluginClass();

HINSTANCE g_hinstDLL;             /**< DLL のインスタンスハンドル */
TVTest::CTVTestPlugin *g_pPlugin; /**< プラグインクラスへのポインタ */


// エントリポイント
// プラグインクラスのインスタンスの生成と破棄を行っています
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		g_hinstDLL = hinstDLL;
		g_pPlugin = CreatePluginClass();
		if (g_pPlugin == nullptr)
			return FALSE;
		break;
	case DLL_PROCESS_DETACH:
		if (g_pPlugin) {
			delete g_pPlugin;
			g_pPlugin = nullptr;
		}
		break;
	}
	return TRUE;
}

// プラグインの準拠するプラグイン仕様のバージョンを返す
TVTEST_EXPORT(DWORD) TVTGetVersion()
{
	return g_pPlugin->GetVersion();
}

// プラグインの情報を取得する
TVTEST_EXPORT(BOOL) TVTGetPluginInfo(TVTest::PluginInfo *pInfo)
{
	return g_pPlugin->GetPluginInfo(pInfo);
}

// 初期化を行う
TVTEST_EXPORT(BOOL) TVTInitialize(TVTest::PluginParam *pParam)
{
	g_pPlugin->SetPluginParam(pParam);
	return g_pPlugin->Initialize();
}

// 終了処理を行う
TVTEST_EXPORT(BOOL) TVTFinalize()
{
	bool fOK = g_pPlugin->Finalize();
	delete g_pPlugin;
	g_pPlugin = nullptr;
	return fOK;
}


#endif // TVTEST_PLUGIN_CLASS_IMPLEMENT


#endif // TVTEST_PLUGIN_H
