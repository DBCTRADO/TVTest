/*
	TVTest プラグインヘッダ ver.0.0.14-pre

	※ ver.0.0.14 はまだ開発途中です。今後変更される可能性があります。

	このファイルは再配布・改変など自由に行って構いません。
	ただし、改変した場合はオリジナルと違う旨を記載して頂けると、混乱がなくてい
	いと思います。

	特にコンパイラに依存する記述はないはずなので、Borland などでも大丈夫です。
	また、ヘッダを移植すれば C++ 以外の言語でプラグインを作成することも可能な
	はずです(凄く面倒なので誰もやらないと思いますが)。

	実際にプラグインを作成する場合、このヘッダだけを見ても恐らく意味不明だと思
	いますので、サンプルを参考にしてください。
*/


/*
	TVTest プラグインの概要

	プラグインは32/64ビット DLL の形式です。拡張子は .tvtp とします。
	プラグインでは、以下の関数をエクスポートします。

	DWORD WINAPI TVTGetVersion()
	BOOL WINAPI TVTGetPluginInfo(PluginInfo *pInfo)
	BOOL WINAPI TVTInitialize(PluginParam *pParam)
	BOOL WINAPI TVTFinalize()

	各関数については、このヘッダの最後の方にあるプラグインクラスでのエクスポート
	関数の実装(#ifdef TVTEST_PLUGIN_CLASS_IMPLEMENT の部分)を参照してください。

	プラグインからは、コールバック関数を通じてメッセージを送信することにより、
	TVTest の機能を利用することができます。
	このような方法になっているのは、将来的な拡張が容易であるためです。

	また、イベントコールバック関数を登録することにより、TVTest からイベントが通
	知されます。

	メッセージの送信はスレッドセーフではありませんので、別スレッドからメッセー
	ジコールバック関数を呼び出さないでください。

	TVTEST_PLUGIN_CLASS_IMPLEMENT シンボルが #define されていると、エクスポート
	関数を直接記述しなくても、クラスとしてプラグインを記述することができます。
	その場合、TVTestPlugin クラスからプラグインクラスを派生させます。

	以下は最低限の実装を行ったサンプルです。

	#include <windows.h>
	#define TVTEST_PLUGIN_CLASS_IMPLEMENT	// プラグインをクラスとして実装
	#include "TVTestPlugin.h"

	// プラグインクラス。CTVTestPlugin から派生させる
	class CMyPlugin : public TVTest::CTVTestPlugin
	{
	public:
		bool GetPluginInfo(TVTest::PluginInfo *pInfo) override
		{
			// プラグインの情報を返す
			pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
			pInfo->Flags          = 0;
			pInfo->pszPluginName  = L"サンプル";
			pInfo->pszCopyright   = L"Copyright(c) 2010 Taro Yamada";
			pInfo->pszDescription = L"何もしないプラグイン";
			return true;	// false を返すとプラグインのロードが失敗になる
		}

		bool Initialize() override
		{
			// ここで初期化を行う
			// 何もしないのであればオーバーライドしなくても良い
			return true;	// false を返すとプラグインのロードが失敗になる
		}

		bool Finalize() override
		{
			// ここでクリーンアップを行う
			// 何もしないのであればオーバーライドしなくても良い
			return true;
		}
	};

	// CreatePluginClass 関数で、プラグインクラスのインスタンスを生成して返す
	TVTest::CTVTestPlugin *CreatePluginClass()
	{
		return new CMyPlugin;
	}
*/


/*
	更新履歴

	ver.0.0.14 (TVTest ver.0.9.0 or later)
	・以下のメッセージを追加した
	  ・MESSAGE_GETSTYLEVALUE
	  ・MESSAGE_THEMEDRAWBACKGROUND
	  ・MESSAGE_THEMEDRAWTEXT
	  ・MESSAGE_THEMEDRAWICON
	  ・MESSAGE_GETEPGCAPTURESTATUS
	  ・MESSAGE_GETAPPCOMMANDINFO
	  ・MESSAGE_GETAPPCOMMANDCOUNT
	  ・MESSAGE_GETVIDEOSTREAMCOUNT
	  ・MESSAGE_GETVIDEOSTREAM
	  ・MESSAGE_SETVIDEOSTREAM
	  ・MESSAGE_GETLOG
	  ・MESSAGE_GETLOGCOUNT
	  ・MESSAGE_REGISTERPLUGINCOMMAND
	  ・MESSAGE_SETPLUGINCOMMANDSTATE
	  ・MESSAGE_PLUGINCOMMANDNOTIFY
	  ・MESSAGE_REGISTERPLUGINICON
	  ・MESSAGE_REGISTERSTATUSITEM
	  ・MESSAGE_SETSTATUSITEM
	  ・MESSAGE_GETSTATUSITEMINFO
	  ・MESSAGE_STATUSITEMNOTIFY
	  ・MESSAGE_REGISTERTSPROCESSOR
	  ・MESSAGE_REGISTERPANELITEM
	  ・MESSAGE_SETPANELITEM
	  ・MESSAGE_GETPANELITEMINFO
	  ・MESSAGE_SELECTCHANNEL
	  ・MESSAGE_GETFAVORITELIST
	  ・MESSAGE_FREEFAVORITELIST
	  ・MESSAGE_GET1SEGMODE
	  ・MESSAGE_SET1SEGMODE
	・以下のイベントを追加した
	  ・EVENT_FILTERGRAPH_INITIALIZE
	  ・EVENT_FILTERGRAPH_INITIALIZED
	  ・EVENT_FILTERGRAPH_FINALIZE
	  ・EVENT_FILTERGRAPH_FINALIZED
	  ・EVENT_DRAWCOMMANDICON
	  ・EVENT_STATUSITEM_DRAW
	  ・EVENT_STATUSITEM_NOTIFY
	  ・EVENT_STATUSITEM_MOUSE
	  ・EVENT_PANELITEM_NOTIFY
	  ・EVENT_FAVORITESCHANGED
	  ・EVENT_1SEGMODECHANGED
	・MESSAGE_GETSETTING で取得できる設定に以下を追加した
	  ・OSDFont
	  ・PanelFont
	  ・ProgramGuideFont
	  ・StatusBarFont
	  ・DPI
	・プラグインのフラグに PLUGIN_FLAG_NOENABLEDDISABLED を追加した

	ver.0.0.13 (TVTest ver.0.7.16 or later)
	・以下のメッセージを追加した
	  ・MESSAGE_ENABLEPROGRAMGUIDEEVENT
	  ・MESSAGE_REGISTERPROGRAMGUIDECOMMAND
	・以下のイベントを追加した
	  ・EVENT_STARTUPDONE
	  ・EVENT_PROGRAMGUIDE_INITIALIZE
	  ・EVENT_PROGRAMGUIDE_FINALIZE
	  ・EVENT_PROGRAMGUIDE_COMMAND
	  ・EVENT_PROGRAMGUIDE_INITIALIZEMENU
	  ・EVENT_PROGRAMGUIDE_MENUSELECTED
	  ・EVENT_PROGRAMGUIDE_PROGRAM_DRAWBACKGROUND
	  ・EVENT_PROGRAMGUIDE_PROGRAM_INITIALIZEMENU
	  ・EVENT_PROGRAMGUIDE_PROGRAM_MENUSELECTED

	ver.0.0.12 (TVTest ver.0.7.14 or later)
	・以下のメッセージを追加した
	  ・MESSAGE_GETEPGEVENTINFO
	  ・MESSAGE_FREEEPGEVENTINFO
	  ・MESSAGE_GETEPGEVENTLIST
	  ・MESSAGE_FREEEPGEVENTLIST
	  ・MESSAGE_ENUMDRIVER
	  ・MESSAGE_GETDRIVERTUNINGSPACELIST
	  ・MESSAGE_FREEDRIVERTUNINGSPACELIST
	・ChannelInfo 構造体に Flags メンバを追加した

	ver.0.0.11 (TVTest ver.0.7.6 or later)
	・以下のメッセージを追加した
	  ・MESSAGE_SETWINDOWMESSAGECALLBACK
	  ・MESSAGE_REGISTERCONTROLLER
	  ・MESSAGE_ONCOTROLLERBUTTONDOWN
	  ・MESSAGE_GETCONTROLLERSETTINGS
	・EVENT_CONTROLLERFOCUS を追加した
	・プラグインのフラグに PLUGIN_FLAG_NOUNLOAD を追加した

	ver.0.0.10 (TVTest ver.0.7.0 or later)
	・以下のメッセージを追加した
	  ・MESSAGE_GETLOGO
	  ・MESSAGE_GETAVAILABLELOGOTYPE
	  ・MESSAGE_RELAYRECORD
	  ・MESSAGE_SILENTMODE
	・以下のイベントを追加した
	  ・EVENT_CLOSE
	  ・EVENT_STARTRECORD
	  ・EVENT_RELAYRECORD
	・プラグインのフラグに PLUGIN_FLAG_DISABLEONSTART を追加した
	・RecordStatusInfo 構造体に pszFileName と MaxFileName メンバを追加した

	ver.0.0.9.1
	・64ビットで警告が出ないようにした

	ver.0.0.9 (TVTest ver.0.6.2 or later)
	・MESSAGE_GETSETTING と MESSAGE_GETDRIVERFULLPATHNAME を追加した
	・EVENT_SETTINGSCHANGE を追加した
	・MESSAGE_RESET にパラメータを追加した

	ver.0.0.8 (TVTest ver.0.6.0 or later)
	・以下のメッセージを追加した
	  ・MESSAGE_GETBCASINFO
	  ・MESSAGE_SENDBCASCOMMAND
	  ・MESSAGE_GETHOSTINFO
	・MESSAGE_SETCHANNEL のパラメータにサービスIDを追加した

	ver.0.0.7 (TVTest ver.0.5.45 or later)
	・MESSAGE_DOCOMMAND を追加した

	ver.0.0.6 (TVTest ver.0.5.42 or later)
	・MESSAGE_SETAUDIOCALLBACK を追加した
	・EVENT_DSHOWINITIALIZED を追加した

	ver.0.0.5 (TVTest ver.0.5.41 or later)
	・以下のイベントを追加した
	  ・EVENT_RESET
	  ・EVENT_STATUSRESET
	  ・EVENT_AUDIOSTREAMCHANGE
	・MESSAGE_RESETSTATUS を追加した

	ver.0.0.4 (TVTest ver.0.5.33 or later)
	・EVENT_EXECUTE を追加した

	ver.0.0.3 (TVTest ver.0.5.27 or later)
	・以下のメッセージを追加した
	  ・MESSAGE_ISPLUGINENABLED
	  ・MESSAGE_REGISTERCOMMAND
	  ・MESSAGE_ADDLOG
	・EVENT_STANDBY と EVENT_COMMAND を追加した

	ver.0.0.2
	・MESSAGE_GETAUDIOSTREAM と MESSAGE_SETAUDIOSTREAM を追加した
	・ServiceInfo 構造体に AudioComponentType と SubtitlePID メンバを追加した
	・StatusInfo 構造体に DropPacketCount と BcasCardStatus メンバを追加した

	ver.0.0.1
	・以下のメッセージを追加した
	  ・MESSAGE_QUERYEVENT
	  ・MESSAGE_GETTUNINGSPACE
	  ・MESSAGE_GETTUNINGSPACEINFO
	  ・MESSAGE_SETNEXTCHANNEL
	・ChannelInfo 構造体にいくつかメンバを追加した

	ver.0.0.0
	・最初のバージョン
*/


#ifndef TVTEST_PLUGIN_H
#define TVTEST_PLUGIN_H


#include <pshpack1.h>


namespace TVTest {


// プラグインのバージョン
#define TVTEST_PLUGIN_VERSION_(major,minor,rev) \
	(((major)<<24) | ((minor)<<12) | (rev))
#ifndef TVTEST_PLUGIN_VERSION
#define TVTEST_PLUGIN_VERSION TVTEST_PLUGIN_VERSION_(0,0,14)
#endif

// エクスポート関数定義用
#define TVTEST_EXPORT(type) extern "C" __declspec(dllexport) type WINAPI

// offsetof
#define TVTEST_OFFSETOF(type,member) \
	((size_t)((BYTE*)&((type*)0)->member-(BYTE*)(type*)0))

// プラグインの種類
enum {
	PLUGIN_TYPE_NORMAL	// 普通
};

// プラグインのフラグ
enum {
	PLUGIN_FLAG_HASSETTINGS			=0x00000001UL,	// 設定ダイアログがある
	PLUGIN_FLAG_ENABLEDEFAULT		=0x00000002UL	// デフォルトで有効
													// 特別な理由が無い限り使わない
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,10)
	,PLUGIN_FLAG_DISABLEONSTART		=0x00000004UL	// 起動時は必ず無効
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,11)
	,PLUGIN_FLAG_NOUNLOAD			=0x00000008UL	// 終了時以外アンロード不可
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,14)
	,PLUGIN_FLAG_NOENABLEDDISABLED	=0x00000010UL	// 有効/無効の区別がない
													// メニューに表示されず、EVENT_PLUGINENABLE が送られません
#endif
};

// プラグインの情報
struct PluginInfo {
	DWORD Type;				// 種類(PLUGIN_TYPE_???)
	DWORD Flags;			// フラグ(PLUGIN_FLAG_???)
	LPCWSTR pszPluginName;	// プラグイン名
	LPCWSTR pszCopyright;	// 著作権情報
	LPCWSTR pszDescription;	// 説明文
};

// メッセージ送信用コールバック関数
typedef LRESULT (CALLBACK *MessageCallbackFunc)(struct PluginParam *pParam,UINT Message,LPARAM lParam1,LPARAM lParam2);

// プラグインパラメータ
struct PluginParam {
	MessageCallbackFunc Callback;	// コールバック関数
	HWND hwndApp;					// メインウィンドウのハンドル
	void *pClientData;				// プラグイン側で好きに使えるデータ
	void *pInternalData;			// TVTest側で使用するデータ。アクセス禁止
};

// エクスポート関数
typedef DWORD (WINAPI *GetVersionFunc)();
typedef BOOL (WINAPI *GetPluginInfoFunc)(PluginInfo *pInfo);
typedef BOOL (WINAPI *InitializeFunc)(PluginParam *pParam);
typedef BOOL (WINAPI *FinalizeFunc)();

// メッセージ
enum {
	MESSAGE_GETVERSION,					// プログラムのバージョンを取得
	MESSAGE_QUERYMESSAGE,				// メッセージに対応しているか問い合わせる
	MESSAGE_MEMORYALLOC,				// メモリ確保
	MESSAGE_SETEVENTCALLBACK,			// イベントハンドル用コールバックの設定
	MESSAGE_GETCURRENTCHANNELINFO,		// 現在のチャンネルの情報を取得
	MESSAGE_SETCHANNEL,					// チャンネルを設定
	MESSAGE_GETSERVICE,					// サービスを取得
	MESSAGE_SETSERVICE,					// サービスを設定
	MESSAGE_GETTUNINGSPACENAME,			// チューニング空間名を取得
	MESSAGE_GETCHANNELINFO,				// チャンネルの情報を取得
	MESSAGE_GETSERVICEINFO,				// サービスの情報を取得
	MESSAGE_GETDRIVERNAME,				// BonDriverのファイル名を取得
	MESSAGE_SETDRIVERNAME,				// BonDriverを設定
	MESSAGE_STARTRECORD,				// 録画の開始
	MESSAGE_STOPRECORD,					// 録画の停止
	MESSAGE_PAUSERECORD,				// 録画の一時停止/再開
	MESSAGE_GETRECORD,					// 録画設定の取得
	MESSAGE_MODIFYRECORD,				// 録画設定の変更
	MESSAGE_GETZOOM,					// 表示倍率の取得
	MESSAGE_SETZOOM,					// 表示倍率の設定
	MESSAGE_GETPANSCAN,					// パンスキャンの設定を取得
	MESSAGE_SETPANSCAN,					// パンスキャンを設定
	MESSAGE_GETSTATUS,					// ステータスを取得
	MESSAGE_GETRECORDSTATUS,			// 録画ステータスを取得
	MESSAGE_GETVIDEOINFO,				// 映像の情報を取得
	MESSAGE_GETVOLUME,					// 音量を取得
	MESSAGE_SETVOLUME,					// 音量を設定
	MESSAGE_GETSTEREOMODE,				// ステレオモードを取得
	MESSAGE_SETSTEREOMODE,				// ステレオモードを設定
	MESSAGE_GETFULLSCREEN,				// 全画面表示の状態を取得
	MESSAGE_SETFULLSCREEN,				// 全画面表示の状態を設定
	MESSAGE_GETPREVIEW,					// 再生が有効か取得
	MESSAGE_SETPREVIEW,					// 再生の有効状態を設定
	MESSAGE_GETSTANDBY,					// 待機状態であるか取得
	MESSAGE_SETSTANDBY,					// 待機状態を設定
	MESSAGE_GETALWAYSONTOP,				// 常に最前面表示であるか取得
	MESSAGE_SETALWAYSONTOP,				// 常に最前面表示を設定
	MESSAGE_CAPTUREIMAGE,				// 画像をキャプチャする
	MESSAGE_SAVEIMAGE,					// 画像を保存する
	MESSAGE_RESET,						// リセットを行う
	MESSAGE_CLOSE,						// ウィンドウを閉じる
	MESSAGE_SETSTREAMCALLBACK,			// ストリームコールバックを設定
	MESSAGE_ENABLEPLUGIN,				// プラグインの有効状態を設定
	MESSAGE_GETCOLOR,					// 色の設定を取得
	MESSAGE_DECODEARIBSTRING,			// ARIB文字列のデコード
	MESSAGE_GETCURRENTPROGRAMINFO,		// 現在の番組の情報を取得
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,1)
	MESSAGE_QUERYEVENT,					// イベントに対応しているか取得
	MESSAGE_GETTUNINGSPACE,				// 現在のチューニング空間を取得
	MESSAGE_GETTUNINGSPACEINFO,			// チューニング空間の情報を取得
	MESSAGE_SETNEXTCHANNEL,				// チャンネルを次に設定する
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,2)
	MESSAGE_GETAUDIOSTREAM,				// 音声ストリームを取得
	MESSAGE_SETAUDIOSTREAM,				// 音声ストリームを設定
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,3)
	MESSAGE_ISPLUGINENABLED,			// プラグインの有効状態を取得
	MESSAGE_REGISTERCOMMAND,			// コマンドの登録
	MESSAGE_ADDLOG,						// ログを記録
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,5)
	MESSAGE_RESETSTATUS,				// ステータスを初期化
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,6)
	MESSAGE_SETAUDIOCALLBACK,			// 音声のコールバック関数を設定
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,7)
	MESSAGE_DOCOMMAND,					// コマンドの実行
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,8)
	MESSAGE_GETBCASINFO,				// B-CAS カードの情報を取得
	MESSAGE_SENDBCASCOMMAND,			// B-CAS カードにコマンドを送信
	MESSAGE_GETHOSTINFO,				// ホストプログラムの情報を取得
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,9)
	MESSAGE_GETSETTING,					// 設定の取得
	MESSAGE_GETDRIVERFULLPATHNAME,		// BonDriverのフルパスを取得
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,10)
	MESSAGE_GETLOGO,					// ロゴの取得
	MESSAGE_GETAVAILABLELOGOTYPE,		// 利用可能なロゴの取得
	MESSAGE_RELAYRECORD,				// 録画ファイルの切り替え
	MESSAGE_SILENTMODE,					// サイレントモードの取得/設定
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,11)
	MESSAGE_SETWINDOWMESSAGECALLBACK,	// ウィンドウメッセージコールバックの設定
	MESSAGE_REGISTERCONTROLLER,			// コントローラの登録
	MESSAGE_ONCONTROLLERBUTTONDOWN,		// コントローラのボタンが押されたのを通知
	MESSAGE_GETCONTROLLERSETTINGS,		// コントローラの設定を取得
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,12)
	MESSAGE_GETEPGEVENTINFO,			// 番組情報を取得
	MESSAGE_FREEEPGEVENTINFO,			// 番組情報を解放
	MESSAGE_GETEPGEVENTLIST,			// 番組のリストを取得
	MESSAGE_FREEEPGEVENTLIST,			// 番組のリストを解放
	MESSAGE_ENUMDRIVER,					// BonDriverの列挙
	MESSAGE_GETDRIVERTUNINGSPACELIST,	// BonDriverのチューニング空間のリストの取得
	MESSAGE_FREEDRIVERTUNINGSPACELIST,	// BonDriverのチューニング空間のリストの解放
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,13)
	MESSAGE_ENABLEPROGRAMGUIDEEVENT,	// 番組表のイベントの有効/無効を設定する
	MESSAGE_REGISTERPROGRAMGUIDECOMMAND,	// 番組表のコマンドを登録
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,14)
	MESSAGE_GETSTYLEVALUE,				// スタイル値を取得
	MESSAGE_THEMEDRAWBACKGROUND,		// テーマの背景を描画
	MESSAGE_THEMEDRAWTEXT,				// テーマの文字列を描画
	MESSAGE_THEMEDRAWICON,				// テーマのアイコンを描画
	MESSAGE_GETEPGCAPTURESTATUS,		// EPG取得状況を取得
	MESSAGE_GETAPPCOMMANDINFO,			// コマンドの情報を取得
	MESSAGE_GETAPPCOMMANDCOUNT,			// コマンドの数を取得
	MESSAGE_GETVIDEOSTREAMCOUNT,		// 映像ストリームの数を取得
	MESSAGE_GETVIDEOSTREAM,				// 映像ストリームを取得
	MESSAGE_SETVIDEOSTREAM,				// 映像ストリームを設定
	MESSAGE_GETLOG,						// ログを取得
	MESSAGE_GETLOGCOUNT,				// ログの数を取得
	MESSAGE_REGISTERPLUGINCOMMAND,		// プラグインのコマンドを登録
	MESSAGE_SETPLUGINCOMMANDSTATE,		// プラグインのコマンドの状態を設定
	MESSAGE_PLUGINCOMMANDNOTIFY,		// プラグインのコマンドの通知
	MESSAGE_REGISTERPLUGINICON,			// プラグインのアイコンを登録
	MESSAGE_REGISTERSTATUSITEM,			// ステータス項目を登録
	MESSAGE_SETSTATUSITEM,				// ステータス項目の設定
	MESSAGE_GETSTATUSITEMINFO,			// ステータス項目の情報を取得
	MESSAGE_STATUSITEMNOTIFY,			// ステータス項目の通知
	MESSAGE_REGISTERTSPROCESSOR,		// TSプロセッサの登録
	MESSAGE_REGISTERPANELITEM,			// パネル項目を登録
	MESSAGE_SETPANELITEM,				// パネル項目の設定
	MESSAGE_GETPANELITEMINFO,			// パネル項目の情報を取得
	MESSAGE_SELECTCHANNEL,				// チャンネルを選択する
	MESSAGE_GETFAVORITELIST,			// お気に入りチャンネルを取得
	MESSAGE_FREEFAVORITELIST,			// お気に入りチャンネルを解放
	MESSAGE_GET1SEGMODE,				// ワンセグモードを取得
	MESSAGE_SET1SEGMODE,				// ワンセグモードを設定
#endif
	MESSAGE_TRAILER
};

// イベント用コールバック関数
typedef LRESULT (CALLBACK *EventCallbackFunc)(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData);

// イベント
// 各イベント発生時のパラメータは CTVTestEventHadler を参照してください。
enum {
	EVENT_PLUGINENABLE,			// 有効状態が変化した
	EVENT_PLUGINSETTINGS,		// 設定を行う
	EVENT_CHANNELCHANGE,		// チャンネルが変更された
	EVENT_SERVICECHANGE,		// サービスが変更された
	EVENT_DRIVERCHANGE,			// ドライバが変更された
	EVENT_SERVICEUPDATE,		// サービスの構成が変化した
	EVENT_RECORDSTATUSCHANGE,	// 録画状態が変化した
	EVENT_FULLSCREENCHANGE,		// 全画面表示状態が変化した
	EVENT_PREVIEWCHANGE,		// プレビュー表示状態が変化した
	EVENT_VOLUMECHANGE,			// 音量が変化した
	EVENT_STEREOMODECHANGE,		// ステレオモードが変化した
	EVENT_COLORCHANGE,			// 色の設定が変化した
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,3)
	EVENT_STANDBY,				// 待機状態が変化した
	EVENT_COMMAND,				// コマンドが選択された
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,4)
	EVENT_EXECUTE,				// 複数起動禁止時に複数起動された
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,5)
	EVENT_RESET,				// リセットされた
	EVENT_STATUSRESET,			// ステータスがリセットされた
	EVENT_AUDIOSTREAMCHANGE,	// 音声ストリームが変更された
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,9)
	EVENT_SETTINGSCHANGE,		// 設定が変更された
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,10)
	EVENT_CLOSE,				// TVTestのウィンドウが閉じられる
	EVENT_STARTRECORD,			// 録画が開始される
	EVENT_RELAYRECORD,			// 録画ファイルが切り替えられた
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,11)
	EVENT_CONTROLLERFOCUS,		// コントローラの対象を設定
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,13)
	EVENT_STARTUPDONE,			// 起動時の処理が終わった
	// 番組表関係のイベントは、MESSAGE_ENABLEPROGRAMGUIDEEVENT を呼んで有効にしないと通知されません
	EVENT_PROGRAMGUIDE_INITIALIZE,				// 番組表の初期化
	EVENT_PROGRAMGUIDE_FINALIZE,				// 番組表の終了
	EVENT_PROGRAMGUIDE_COMMAND,					// 番組表のコマンド実行
	EVENT_PROGRAMGUIDE_INITIALIZEMENU,			// 番組表のメニューの設定
	EVENT_PROGRAMGUIDE_MENUSELECTED,			// 番組表のメニューが選択された
	EVENT_PROGRAMGUIDE_PROGRAM_DRAWBACKGROUND,	// 番組表の番組の背景を描画
	EVENT_PROGRAMGUIDE_PROGRAM_INITIALIZEMENU,	// 番組表の番組のメニューの設定
	EVENT_PROGRAMGUIDE_PROGRAM_MENUSELECTED,	// 番組表の番組のメニューが選択された
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,14)
	EVENT_FILTERGRAPH_INITIALIZE,				// フィルタグラフの初期化開始
	EVENT_FILTERGRAPH_INITIALIZED,				// フィルタグラフの初期化終了
	EVENT_FILTERGRAPH_FINALIZE,					// フィルタグラフの終了処理開始
	EVENT_FILTERGRAPH_FINALIZED,				// フィルタグラフの終了処理終了
	EVENT_DRAWCOMMANDICON,						// コマンドアイコンの描画
	EVENT_STATUSITEM_DRAW,						// ステータス項目を描画
	EVENT_STATUSITEM_NOTIFY,					// ステータス項目の通知
	EVENT_STATUSITEM_MOUSE,						// ステータス項目のマウス操作
	EVENT_PANELITEM_NOTIFY,						// パネル項目の通知
	EVENT_FAVORITESCHANGED,						// お気に入りチャンネルが変更された
	EVENT_1SEGMODECHANGED,						// ワンセグモードが変わった
#endif
	EVENT_TRAILER
};

// バージョン番号を DWORD にまとめる
inline DWORD MakeVersion(BYTE Major,WORD Minor,WORD Build) {
	return ((DWORD)Major<<24) | ((DWORD)Minor<<12) | Build;
}
inline DWORD GetMajorVersion(DWORD Version) { return Version>>24; }
inline DWORD GetMinorVersion(DWORD Version) { return (Version&0x00FFF000UL)>>12; }
inline DWORD GetBuildVersion(DWORD Version) { return Version&0x00000FFFUL; }

// プログラム(TVTest)のバージョンを取得する
// 上位8ビットがメジャーバージョン、次の12ビットがマイナーバージョン、
// 下位12ビットがビルドナンバー
// GetMajorVersion/GetMinorVersion/GetBuildVersionを使って取得できる
inline DWORD MsgGetVersion(PluginParam *pParam) {
	return (DWORD)(*pParam->Callback)(pParam,MESSAGE_GETVERSION,0,0);
}

// 指定されたメッセージに対応しているか問い合わせる
inline bool MsgQueryMessage(PluginParam *pParam,UINT Message) {
	return (*pParam->Callback)(pParam,MESSAGE_QUERYMESSAGE,Message,0)!=0;
}

// メモリ再確保
// 仕様はreallocと同じ
inline void *MsgMemoryReAlloc(PluginParam *pParam,void *pData,DWORD Size) {
	return (void*)(*pParam->Callback)(pParam,MESSAGE_MEMORYALLOC,(LPARAM)pData,Size);
}

// メモリ確保
// 仕様はrealloc(NULL,Size)と同じ
inline void *MsgMemoryAlloc(PluginParam *pParam,DWORD Size) {
	return (void*)(*pParam->Callback)(pParam,MESSAGE_MEMORYALLOC,(LPARAM)(void*)NULL,Size);
}

// メモリ開放
// 仕様はrealloc(pData,0)と同じ
// (実際にreallocでメモリ開放しているコードは見たこと無いけど...)
inline void MsgMemoryFree(PluginParam *pParam,void *pData) {
	(*pParam->Callback)(pParam,MESSAGE_MEMORYALLOC,(LPARAM)pData,0);
}

// イベントハンドル用コールバックの設定
// pClientData はコールバックの呼び出し時に渡されます。
// 一つのプラグインで設定できるコールバック関数は一つだけです。
// Callback に NULL を渡すと設定が解除されます。
inline bool MsgSetEventCallback(PluginParam *pParam,EventCallbackFunc Callback,void *pClientData=NULL) {
	return (*pParam->Callback)(pParam,MESSAGE_SETEVENTCALLBACK,(LPARAM)Callback,(LPARAM)pClientData)!=0;
}

// チャンネルの情報
struct ChannelInfo {
	DWORD Size;							// 構造体のサイズ
	int Space;							// チューニング空間(BonDriverのインデックス)
	int Channel;						// チャンネル(BonDriverのインデックス)
	int RemoteControlKeyID;				// リモコンID
	WORD NetworkID;						// ネットワークID
	WORD TransportStreamID;				// トランスポートストリームID
	WCHAR szNetworkName[32];			// ネットワーク名
	WCHAR szTransportStreamName[32];	// トランスポートストリーム名
	WCHAR szChannelName[64];			// チャンネル名
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,1)
	int PhysicalChannel;				// 物理チャンネル番号(あまり信用できない)
										// 不明の場合は0
	WORD ServiceIndex;					// サービスのインデックス
										// (現在は意味を無くしているので使わない)
	WORD ServiceID;						// サービスID
	// サービスはチャンネルファイルで設定されているものが取得される
	// サービスはユーザーが切り替えられるので、実際に視聴中のサービスがこれであるとは限らない
	// 実際に視聴中のサービスは MESSAGE_GETSERVICE で取得できる
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,12)
	DWORD Flags;						// 各種フラグ(CHANNEL_FLAG_*)
#endif
};

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,12)
// チャンネルの情報のフラグ
enum {
	CHANNEL_FLAG_DISABLED	=0x00000001UL	// 無効にされている
};
#endif

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,1)
enum {
	CHANNELINFO_SIZE_V1=TVTEST_OFFSETOF(ChannelInfo,PhysicalChannel)
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,12)
	, CHANNELINFO_SIZE_V2=TVTEST_OFFSETOF(ChannelInfo,Flags)
#endif
};
#endif

// 現在のチャンネルの情報を取得する
// 事前に ChannelInfo の Size メンバを設定しておきます。
inline bool MsgGetCurrentChannelInfo(PluginParam *pParam,ChannelInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_GETCURRENTCHANNELINFO,(LPARAM)pInfo,0)!=0;
}

// チャンネルを設定する
// 機能が追加された MESSAGE_SELECTCHANNEL もあります。
#if TVTEST_PLUGIN_VERSION<TVTEST_PLUGIN_VERSION_(0,0,8)
inline bool MsgSetChannel(PluginParam *pParam,int Space,int Channel) {
	return (*pParam->Callback)(pParam,MESSAGE_SETCHANNEL,Space,Channel)!=0;
}
#else
inline bool MsgSetChannel(PluginParam *pParam,int Space,int Channel,WORD ServiceID=0) {
	return (*pParam->Callback)(pParam,MESSAGE_SETCHANNEL,Space,MAKELPARAM((SHORT)Channel,ServiceID))!=0;
}
#endif

// 現在のサービス及びサービス数を取得する
// サービスのインデックスが返る。エラー時は-1が返ります。
// pNumServices が NULL でない場合は、サービスの数が返されます。
inline int MsgGetService(PluginParam *pParam,int *pNumServices=NULL) {
	return (int)(*pParam->Callback)(pParam,MESSAGE_GETSERVICE,(LPARAM)pNumServices,0);
}

// サービスを設定する
// fByID=false の場合はインデックス、fByID=trueの場合はサービスID
inline bool MsgSetService(PluginParam *pParam,int Service,bool fByID=false) {
	return (*pParam->Callback)(pParam,MESSAGE_SETSERVICE,Service,fByID)!=0;
}

// チューニング空間名を取得する
// チューニング空間名の長さが返ります。Indexが範囲外の場合は0が返ります。
// pszName を NULL で呼べば長さだけを取得できます。
// MaxLength には pszName の先に格納できる最大の要素数(終端の空文字を含む)を指定します。
inline int MsgGetTuningSpaceName(PluginParam *pParam,int Index,LPWSTR pszName,int MaxLength) {
	return (int)(*pParam->Callback)(pParam,MESSAGE_GETTUNINGSPACENAME,(LPARAM)pszName,MAKELPARAM(Index,min(MaxLength,0xFFFF)));
}

// チャンネルの情報を取得する
// 事前に ChannelInfo の Size メンバを設定しておきます。
// szNetworkName,szTransportStreamName は MESSAGE_GETCURRENTCHANNEL でしか取得できません。
// NetworkID,TransportStreamID はチャンネルスキャンしていないと取得できません。
// 取得できなかった場合は0になります。
inline bool MsgGetChannelInfo(PluginParam *pParam,int Space,int Index,ChannelInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_GETCHANNELINFO,(LPARAM)pInfo,MAKELPARAM(Space,Index))!=0;
}

// サービスの情報
struct ServiceInfo {
	DWORD Size;					// 構造体のサイズ
	WORD ServiceID;				// サービスID
	WORD VideoPID;				// ビデオストリームのPID
	int NumAudioPIDs;			// 音声PIDの数
	WORD AudioPID[4];			// 音声ストリームのPID
	WCHAR szServiceName[32];	// サービス名
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,2)
	BYTE AudioComponentType[4];	// 音声コンポーネントタイプ
	WORD SubtitlePID;			// 字幕ストリームのPID(無い場合は0)
	WORD Reserved;				// 予約
#endif
};

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,2)
enum { SERVICEINFO_SIZE_V1=TVTEST_OFFSETOF(ServiceInfo,AudioComponentType) };
#endif

// サービスの情報を取得する
// 現在のチャンネルのサービスの情報を取得します。
// 事前に ServiceInfo の Size メンバを設定しておきます。
inline bool MsgGetServiceInfo(PluginParam *pParam,int Index,ServiceInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_GETSERVICEINFO,Index,(LPARAM)pInfo)!=0;
}

// BonDriverのファイル名を取得する
// 戻り値はファイル名の長さ(終端のNullを除く)が返ります。
// pszName を NULL で呼べば長さだけを取得できます。
// 取得されるのは、ディレクトリを含まないファイル名のみか、相対パスの場合もあります。
// フルパスを取得したい場合は MsgGetDriverFullPathName を使用してください。
inline int MsgGetDriverName(PluginParam *pParam,LPWSTR pszName,int MaxLength) {
	return (int)(*pParam->Callback)(pParam,MESSAGE_GETDRIVERNAME,(LPARAM)pszName,MaxLength);
}

// BonDriverを設定する
// ファイル名のみか相対パスを指定すると、BonDriver 検索フォルダの設定が使用されます。
// NULL を指定すると現在の BonDriver が解放されます(ver.0.0.14 以降)。
inline bool MsgSetDriverName(PluginParam *pParam,LPCWSTR pszName) {
	return (*pParam->Callback)(pParam,MESSAGE_SETDRIVERNAME,(LPARAM)pszName,0)!=0;
}

// 録画情報のマスク
enum {
	RECORD_MASK_FLAGS		=0x00000001UL,	// Flags が有効
	RECORD_MASK_FILENAME	=0x00000002UL,	// pszFileName が有効
	RECORD_MASK_STARTTIME	=0x00000004UL,	// StartTime が有効
	RECORD_MASK_STOPTIME	=0x00000008UL	// StopTime が有効
};

// 録画フラグ
enum {
	RECORD_FLAG_CANCEL		=0x10000000UL	// キャンセル
};

// 録画開始時間の指定方法
enum {
	RECORD_START_NOTSPECIFIED,	// 未指定
	RECORD_START_TIME,			// 時刻指定
	RECORD_START_DELAY			// 長さ指定
};

// 録画停止時間の指定方法
enum {
	RECORD_STOP_NOTSPECIFIED,	// 未指定
	RECORD_STOP_TIME,			// 時刻指定
	RECORD_STOP_DURATION		// 長さ指定
};

// 録画情報
struct RecordInfo {
	DWORD Size;				// 構造体のサイズ
	DWORD Mask;				// マスク(RECORD_MASK_???)
	DWORD Flags;			// フラグ(RECORD_FLAG_???)
	LPWSTR pszFileName;		// ファイル名(NULLでデフォルト)
							// %～% で囲まれた置換キーワードを使用できます
	int MaxFileName;		// ファイル名の最大長(MESSAGE_GETRECORDのみで使用)
	FILETIME ReserveTime;	// 録画予約された時刻(MESSAGE_GETRECORDのみで使用)
	DWORD StartTimeSpec;	// 録画開始時間の指定方法(RECORD_START_???)
	union {
		FILETIME Time;		// 録画開始時刻(StartTimeSpec==RECORD_START_TIME)
							// ローカル時刻
		ULONGLONG Delay;	// 録画開始時間(StartTimeSpec==RECORD_START_DELAY)
							// 録画を開始するまでの時間(ms)
	} StartTime;
	DWORD StopTimeSpec;		// 録画停止時間の指定方法(RECORD_STOP_???)
	union {
		FILETIME Time;		// 録画停止時刻(StopTimeSpec==RECORD_STOP_TIME)
							// ローカル時刻
		ULONGLONG Duration;	// 録画停止時間(StopTimeSpec==RECORD_STOP_DURATION)
							// 開始時間からのミリ秒
	} StopTime;
};

// 録画を開始する
// pInfo が NULL で即時録画開始します。
inline bool MsgStartRecord(PluginParam *pParam,const RecordInfo *pInfo=NULL) {
	return (*pParam->Callback)(pParam,MESSAGE_STARTRECORD,(LPARAM)pInfo,0)!=0;
}

// 録画を停止する
inline bool MsgStopRecord(PluginParam *pParam) {
	return (*pParam->Callback)(pParam,MESSAGE_STOPRECORD,0,0)!=0;
}

// 録画を一時停止/再開する
inline bool MsgPauseRecord(PluginParam *pParam,bool fPause=true) {
	return (*pParam->Callback)(pParam,MESSAGE_PAUSERECORD,fPause,0)!=0;
}

// 録画設定を取得する
// 事前に RecordInfo の Size メンバを設定しておきます。
inline bool MsgGetRecord(PluginParam *pParam,RecordInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_GETRECORD,(LPARAM)pInfo,0)!=0;
}

// 録画設定を変更する
// 既に録画中である場合は、ファイル名と開始時間の指定は無視されます。
inline bool MsgModifyRecord(PluginParam *pParam,const RecordInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_MODIFYRECORD,(LPARAM)pInfo,0)!=0;
}

// 表示倍率を取得する(%単位)
inline int MsgGetZoom(PluginParam *pParam) {
	return (int)(*pParam->Callback)(pParam,MESSAGE_GETZOOM,0,0);
}

// 表示倍率を設定する
// %単位だけではなく、Num=1/Denom=3などとして割り切れない倍率を設定することもできます。
inline bool MsgSetZoom(PluginParam *pParam,int Num,int Denom=100) {
	return (*pParam->Callback)(pParam,MESSAGE_SETZOOM,Num,Denom)!=0;
}

// パンスキャンの種類
enum {
	PANSCAN_NONE,		// なし
	PANSCAN_LETTERBOX,	// レターボックス
	PANSCAN_SIDECUT,	// サイドカット
	PANSCAN_SUPERFRAME	// 超額縁
};

// パンスキャンの情報
struct PanScanInfo {
	DWORD Size;		// 構造体のサイズ
	int Type;		// 種類(PANSCAN_???)
	int XAspect;	// 水平アスペクト比
	int YAspect;	// 垂直アスペクト比
};

// パンスキャンの設定を取得する
inline bool MsgGetPanScan(PluginParam *pParam,PanScanInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_GETPANSCAN,(LPARAM)pInfo,0)!=0;
}

// パンスキャンを設定する
inline bool MsgSetPanScan(PluginParam *pParam,const PanScanInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_SETPANSCAN,(LPARAM)pInfo,0)!=0;
}

// B-CAS カードの状態
enum {
	BCAS_STATUS_OK,				// エラーなし
	BCAS_STATUS_NOTOPEN,		// 開かれていない(スクランブル解除なし)
	BCAS_STATUS_NOCARDREADER,	// カードリーダが無い
	BCAS_STATUS_NOCARD,			// カードがない
	BCAS_STATUS_OPENERROR,		// オープンエラー
	BCAS_STATUS_TRANSMITERROR,	// 通信エラー
	BCAS_STATUS_ESTABLISHERROR	// コンテキスト確立失敗
};

// ステータス情報
struct StatusInfo {
	DWORD Size;							// 構造体のサイズ
	float SignalLevel;					// 信号レベル(dB)
	DWORD BitRate;						// ビットレート(Bits/Sec)
	DWORD ErrorPacketCount;				// エラーパケット数
										// DropPacketCount も含まれる
	DWORD ScramblePacketCount;			// 復号漏れパケット数
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,2)
	DWORD DropPacketCount;				// ドロップパケット数
	DWORD BcasCardStatus;				// B-CAS カードの状態(BCAS_STATUS_???)
										// ※ B-CAS 関連の機能は削除されました。現在は利用できません。
#endif
};

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,2)
enum { STATUSINFO_SIZE_V1=TVTEST_OFFSETOF(StatusInfo,DropPacketCount) };
#endif

// ステータスを取得する
// 事前にStatusInfoのSizeメンバを設定しておきます。
inline bool MsgGetStatus(PluginParam *pParam,StatusInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_GETSTATUS,(LPARAM)pInfo,0)!=0;
}

// 録画の状態
enum {
	RECORD_STATUS_NOTRECORDING,	// 録画していない
	RECORD_STATUS_RECORDING,	// 録画中
	RECORD_STATUS_PAUSED		// 録画一時停止中
};

// 録画ステータス情報
struct RecordStatusInfo {
	DWORD Size;				// 構造体のサイズ
	DWORD Status;			// 状態(RECORD_STATUS_???)
	FILETIME StartTime;		// 録画開始時刻(ローカル時刻)
	DWORD RecordTime;		// 録画時間(ms) 一時停止中を含まない
	DWORD PauseTime;		// 一時停止時間(ms)
	DWORD StopTimeSpec;		// 録画停止時間の指定方法(RECORD_STOP_???)
	union {
		FILETIME Time;		// 録画停止予定時刻(StopTimeSpec==RECORD_STOP_TIME)
							// (ローカル時刻)
		ULONGLONG Duration;	// 録画停止までの時間(StopTimeSpec==RECORD_STOP_DURATION)
							// 開始時刻(StartTime)からミリ秒単位
	} StopTime;
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,10)
	LPWSTR pszFileName;		// ファイルパス
	int MaxFileName;		// ファイルパスの最大長
	RecordStatusInfo() : pszFileName(NULL), MaxFileName(0) {}
#endif
};

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,10)
enum { RECORDSTATUSINFO_SIZE_V1=TVTEST_OFFSETOF(RecordStatusInfo,pszFileName) };
#endif

// 録画ステータスを取得する
// 事前に RecordStatusInfo の Size メンバを設定しておきます。
// ファイル名を取得する場合は pszFileName と MaxFileName メンバを設定しておきます。
inline bool MsgGetRecordStatus(PluginParam *pParam,RecordStatusInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_GETRECORDSTATUS,(LPARAM)pInfo,0)!=0;
}

// 映像の情報
struct VideoInfo {
	DWORD Size;			// 構造体のサイズ
	int Width;			// 幅(ピクセル単位)
	int Height;			// 高さ(ピクセル単位)
	int XAspect;		// 水平アスペクト比
	int YAspect;		// 垂直アスペクト比
	RECT SourceRect;	// ソースの表示範囲
};

// 映像の情報を取得する
// 事前にVideoInfoのSizeメンバを設定しておきます。
inline bool MsgGetVideoInfo(PluginParam *pParam,VideoInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_GETVIDEOINFO,(LPARAM)pInfo,0)!=0;
}

// 音量を取得する(0-100)
inline int MsgGetVolume(PluginParam *pParam) {
	return LOWORD((*pParam->Callback)(pParam,MESSAGE_GETVOLUME,0,0));
}

// 音量を設定する(0-100)
inline bool MsgSetVolume(PluginParam *pParam,int Volume) {
	return (*pParam->Callback)(pParam,MESSAGE_SETVOLUME,Volume,0)!=0;
}

// 消音状態であるか取得する
inline bool MsgGetMute(PluginParam *pParam) {
	return HIWORD((*pParam->Callback)(pParam,MESSAGE_GETVOLUME,0,0))!=0;
}

// 消音状態を設定する
inline bool MsgSetMute(PluginParam *pParam,bool fMute) {
	return (*pParam->Callback)(pParam,MESSAGE_SETVOLUME,-1,fMute)!=0;
}

// ステレオモード
enum {
	STEREOMODE_STEREO,	// ステレオ
	STEREOMODE_LEFT,	// 左(主音声)
	STEREOMODE_RIGHT	// 右(副音声)
};

// ステレオモードを取得する
inline int MsgGetStereoMode(PluginParam *pParam) {
	return (int)(*pParam->Callback)(pParam,MESSAGE_GETSTEREOMODE,0,0);
}

// ステレオモードを設定する
inline bool MsgSetStereoMode(PluginParam *pParam,int StereoMode) {
	return (*pParam->Callback)(pParam,MESSAGE_SETSTEREOMODE,StereoMode,0)!=0;
}

// 全画面表示の状態を取得する
inline bool MsgGetFullscreen(PluginParam *pParam) {
	return (*pParam->Callback)(pParam,MESSAGE_GETFULLSCREEN,0,0)!=0;
}

// 全画面表示の状態を設定する
inline bool MsgSetFullscreen(PluginParam *pParam,bool fFullscreen) {
	return (*pParam->Callback)(pParam,MESSAGE_SETFULLSCREEN,fFullscreen,0)!=0;
}

// 再生が有効であるか取得する
inline bool MsgGetPreview(PluginParam *pParam) {
	return (*pParam->Callback)(pParam,MESSAGE_GETPREVIEW,0,0)!=0;
}

// 再生の有効状態を設定する
inline bool MsgSetPreview(PluginParam *pParam,bool fPreview) {
	return (*pParam->Callback)(pParam,MESSAGE_SETPREVIEW,fPreview,0)!=0;
}

// 待機状態であるか取得する
inline bool MsgGetStandby(PluginParam *pParam) {
	return (*pParam->Callback)(pParam,MESSAGE_GETSTANDBY,0,0)!=0;
}

// 待機状態を設定する
inline bool MsgSetStandby(PluginParam *pParam,bool fStandby) {
	return (*pParam->Callback)(pParam,MESSAGE_SETSTANDBY,fStandby,0)!=0;
}

// 常に最前面表示の状態を取得する
inline bool MsgGetAlwaysOnTop(PluginParam *pParam) {
	return (*pParam->Callback)(pParam,MESSAGE_GETALWAYSONTOP,0,0)!=0;
}

// 常に最前面表示の状態を設定する
inline bool MsgSetAlwaysOnTop(PluginParam *pParam,bool fAlwaysOnTop) {
	return (*pParam->Callback)(pParam,MESSAGE_SETALWAYSONTOP,fAlwaysOnTop,0)!=0;
}

// 画像をキャプチャする
// 戻り値はパックDIBデータ(BITMAPINFOHEADER + ピクセルデータ)へのポインタです。
// 不要になった場合はMsgMemoryFreeで開放します。
// キャプチャできなかった場合はNULLが返ります。
inline void *MsgCaptureImage(PluginParam *pParam,DWORD Flags=0) {
	return (void*)(*pParam->Callback)(pParam,MESSAGE_CAPTUREIMAGE,Flags,0);
}

// 画像を保存する
inline bool MsgSaveImage(PluginParam *pParam) {
	return (*pParam->Callback)(pParam,MESSAGE_SAVEIMAGE,0,0)!=0;
}

// リセットを行う
#if TVTEST_PLUGIN_VERSION<TVTEST_PLUGIN_VERSION_(0,0,9)
inline bool MsgReset(PluginParam *pParam) {
	return (*pParam->Callback)(pParam,MESSAGE_RESET,0,0)!=0;
}
#else
// リセットのフラグ
enum {
	RESET_ALL		= 0x00000000UL,	// 全て
	RESET_VIEWER	= 0x00000001UL	// ビューアのみ
};

inline bool MsgReset(PluginParam *pParam,DWORD Flags=RESET_ALL) {
	return (*pParam->Callback)(pParam,MESSAGE_RESET,Flags,0)!=0;
}
#endif

// ウィンドウクローズのフラグ
enum {
	CLOSE_EXIT	=0x00000001UL	// 必ず終了させる
};

// ウィンドウを閉じる
inline bool MsgClose(PluginParam *pParam,DWORD Flags=0) {
	return (*pParam->Callback)(pParam,MESSAGE_CLOSE,Flags,0)!=0;
}

// ストリームコールバック関数
// pData は 188 バイトの TS パケットが渡されます。
// FALSE を返すとパケットが破棄されます。
typedef BOOL (CALLBACK *StreamCallbackFunc)(BYTE *pData,void *pClientData);

// ストリームコールバックフラグ
enum {
	STREAM_CALLBACK_REMOVE	=0x00000001UL	// コールバックの削除
};

// ストリームコールバックの情報
struct StreamCallbackInfo {
	DWORD Size;						// 構造体のサイズ
	DWORD Flags;					// フラグ(STREAM_CALLBACK_???)
	StreamCallbackFunc Callback;	// コールバック関数
	void *pClientData;				// コールバック関数に渡されるデータ
};

// ストリームコールバックを設定する
// ストリームコールバックを登録すると、TS データを受け取ることができます。
// コールバック関数は一つのプラグインで複数設定できます。
// ストリームコールバック関数で処理が遅延すると全体が遅延するので、
// 時間が掛かる処理は別スレッドで行うなどしてください。
inline bool MsgSetStreamCallback(PluginParam *pParam,DWORD Flags,
					StreamCallbackFunc Callback,void *pClientData=NULL) {
	StreamCallbackInfo Info;
	Info.Size=sizeof(StreamCallbackInfo);
	Info.Flags=Flags;
	Info.Callback=Callback;
	Info.pClientData=pClientData;
	return (*pParam->Callback)(pParam,MESSAGE_SETSTREAMCALLBACK,(LPARAM)&Info,0)!=0;
}

// プラグインの有効状態を設定する
inline bool MsgEnablePlugin(PluginParam *pParam,bool fEnable) {
	return (*pParam->Callback)(pParam,MESSAGE_ENABLEPLUGIN,fEnable,0)!=0;
}

// 色の設定を取得する
// pszColor に取得したい色の名前を指定します。
// 名前は配色設定ファイル(*.httheme)の項目名("StatusBack" など)と同じです。
inline COLORREF MsgGetColor(PluginParam *pParam,LPCWSTR pszColor) {
	return (COLORREF)(*pParam->Callback)(pParam,MESSAGE_GETCOLOR,(LPARAM)pszColor,0);
}

// ARIB文字列のデコード情報
struct ARIBStringDecodeInfo {
	DWORD Size;				// 構造体のサイズ
	DWORD Flags;			// フラグ(現在は常に0)
	const void *pSrcData;	// 変換元データ
	DWORD SrcLength;		// 変換元サイズ(バイト単位)
	LPWSTR pszDest;			// 変換先バッファ
	DWORD DestLength;		// 変換先バッファのサイズ(文字単位)
};

// ARIB文字列をデコードする
inline bool MsgDecodeARIBString(PluginParam *pParam,const void *pSrcData,
							DWORD SrcLength,LPWSTR pszDest,DWORD DestLength) {
	ARIBStringDecodeInfo Info;
	Info.Size=sizeof(ARIBStringDecodeInfo);
	Info.Flags=0;
	Info.pSrcData=pSrcData;
	Info.SrcLength=SrcLength;
	Info.pszDest=pszDest;
	Info.DestLength=DestLength;
	return (*pParam->Callback)(pParam,MESSAGE_DECODEARIBSTRING,(LPARAM)&Info,0)!=0;
}

// 番組の情報
struct ProgramInfo {
	DWORD Size;				// 構造体のサイズ
	WORD ServiceID;			// サービスID
	WORD EventID;			// イベントID
	LPWSTR pszEventName;	// イベント名
	int MaxEventName;		// イベント名の最大長
	LPWSTR pszEventText;	// イベントテキスト
	int MaxEventText;		// イベントテキストの最大長
	LPWSTR pszEventExtText;	// 追加イベントテキスト
	int MaxEventExtText;	// 追加イベントテキストの最大長
	SYSTEMTIME StartTime;	// 開始日時(JST)
	DWORD Duration;			// 長さ(秒単位)
};

// 現在の番組の情報を取得する
// 事前に Size メンバに構造体のサイズを設定します。
// また、pszEventName / pszEventText / pszEventExtText メンバに取得先のバッファへのポインタを、
// MaxEventName / MaxEventText / MaxEventExtText メンバにバッファの長さ(要素数)を設定します。
// 必要のない情報は、ポインタを NULL にすると取得されません。
// MsgGetEpgEventInfo で、より詳しい番組情報を取得することもできます。
inline bool MsgGetCurrentProgramInfo(PluginParam *pParam,ProgramInfo *pInfo,bool fNext=false) {
	return (*pParam->Callback)(pParam,MESSAGE_GETCURRENTPROGRAMINFO,(LPARAM)pInfo,fNext)!=0;
}

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,1)

// 指定されたイベントの通知に対応しているか取得する
inline bool MsgQueryEvent(PluginParam *pParam,UINT Event) {
	return (*pParam->Callback)(pParam,MESSAGE_QUERYEVENT,Event,0)!=0;
}

// 現在のチューニング空間及びチューニング空間数を取得する
inline int MsgGetTuningSpace(PluginParam *pParam,int *pNumSpaces=NULL) {
	return (int)(*pParam->Callback)(pParam,MESSAGE_GETTUNINGSPACE,(LPARAM)pNumSpaces,0);
}

// チューニング空間の種類
enum {
	TUNINGSPACE_UNKNOWN,		// 不明
	TUNINGSPACE_TERRESTRIAL,	// 地上デジタル
	TUNINGSPACE_BS,				// BS
	TUNINGSPACE_110CS			// 110度CS
};

// チューニング空間の情報
struct TuningSpaceInfo {
	DWORD Size;			// 構造体のサイズ
	int Space;			// チューニング空間の種類(TUNINGSPACE_???)
						// 場合によっては信用できない
	WCHAR szName[64];	// チューニング空間名
};

// チューニング空間の情報を取得する
// 事前に TuningSpaceInfo の Size メンバを設定しておきます。
inline bool MsgGetTuningSpaceInfo(PluginParam *pParam,int Index,TuningSpaceInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_GETTUNINGSPACEINFO,Index,(LPARAM)pInfo)!=0;
}

// チャンネルを次に設定する
inline bool MsgSetNextChannel(PluginParam *pParam,bool fNext=true)
{
	return (*pParam->Callback)(pParam,MESSAGE_SETNEXTCHANNEL,fNext,0)!=0;
}

#endif	// TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,1)

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,2)

// 現在の音声ストリームを取得する
// 音声ストリームの数は MESSAGE_GETSERVICEINFO で取得できます。
inline int MsgGetAudioStream(PluginParam *pParam)
{
	return (int)(*pParam->Callback)(pParam,MESSAGE_GETAUDIOSTREAM,0,0);
}

// 音声ストリームを設定する
inline bool MsgSetAudioStream(PluginParam *pParam,int Index)
{
	return (*pParam->Callback)(pParam,MESSAGE_SETAUDIOSTREAM,Index,0)!=0;
}

#endif	// TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,2)

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,3)

// プラグインの有効状態を取得する
inline bool MsgIsPluginEnabled(PluginParam *pParam)
{
	return (*pParam->Callback)(pParam,MESSAGE_ISPLUGINENABLED,0,0)!=0;
}

// コマンドの情報
struct CommandInfo {
	int ID;				// 識別子
	LPCWSTR pszText;	// コマンドの文字列
	LPCWSTR pszName;	// コマンドの名前
};

// コマンドを登録する
// TVTInitialize 内で呼びます。
// コマンドを登録すると、ショートカットキーやリモコンに機能を割り当てられるようになります。
// 以下のように使用します。
// MsgRegisterCommand(pParam, ID_MYCOMMAND, L"MyCommand", L"私のコマンド");
// コマンドが実行されると EVENT_COMMAND イベントが送られます。
// その際、パラメータとして識別子が渡されます。
// MsgRegisterCommand から機能が追加されたバージョンの MsgRegisterPluginCommand もあります。
inline bool MsgRegisterCommand(PluginParam *pParam,int ID,LPCWSTR pszText,LPCWSTR pszName)
{
	CommandInfo Info;
	Info.ID=ID;
	Info.pszText=pszText;
	Info.pszName=pszName;
	return (*pParam->Callback)(pParam,MESSAGE_REGISTERCOMMAND,(LPARAM)&Info,1)!=0;
}

// コマンドを登録する
// TVTInitialize 内で呼びます。
inline bool MsgRegisterCommand(PluginParam *pParam,const CommandInfo *pCommandList,int NumCommands)
{
	return (*pParam->Callback)(pParam,MESSAGE_REGISTERCOMMAND,(LPARAM)pCommandList,NumCommands)!=0;
}

// ログを記録する
// 設定のログの項目に表示されます。
inline bool MsgAddLog(PluginParam *pParam,LPCWSTR pszText)
{
	return (*pParam->Callback)(pParam,MESSAGE_ADDLOG,(LPARAM)pszText,0)!=0;
}

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,14)
// ログの種類
enum {
	LOG_TYPE_INFORMATION,	// 情報
	LOG_TYPE_WARNING,		// 警告
	LOG_TYPE_ERROR			// エラー
};

// ログを記録する
inline bool MsgAddLog(PluginParam *pParam,LPCWSTR pszText,int Type)
{
	return (*pParam->Callback)(pParam,MESSAGE_ADDLOG,(LPARAM)pszText,Type)!=0;
}
#endif

#endif	// TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,3)

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,5)

// ステータス(MESSAGE_GETSTATUS で取得できる内容)をリセットする
// リセットが行われると EVENT_STATUSRESET が送られます。
inline bool MsgResetStatus(PluginParam *pParam)
{
	return (*pParam->Callback)(pParam,MESSAGE_RESETSTATUS,0,0)!=0;
}

#endif	// TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,5)

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,6)

// 音声サンプルのコールバック関数
// 渡されるサンプルは 48kHz / 16ビット固定です。
// pData の先には Samples * Channels 分のデータが入っています。
// 今のところ、5.1chをダウンミックスする設定になっている場合
// ダウンミックスされたデータが渡されますが、そのうち仕様を変えるかも知れません。
// 戻り値は今のところ常に0を返します。
typedef LRESULT (CALLBACK *AudioCallbackFunc)(short *pData,DWORD Samples,int Channels,void *pClientData);

// 音声のサンプルを取得するコールバック関数を設定する
// 一つのプラグインで設定できるコールバック関数は一つだけです。
// pClinetData はコールバック関数に渡されます。
// pCallback に NULL を指定すると、設定が解除されます。
inline bool MsgSetAudioCallback(PluginParam *pParam,AudioCallbackFunc pCallback,void *pClientData=NULL)
{
	return (*pParam->Callback)(pParam,MESSAGE_SETAUDIOCALLBACK,(LPARAM)pCallback,(LPARAM)pClientData)!=0;
}

#endif	// TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,6)

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,7)

// コマンドを実行する
// 文字列を指定してコマンドを実行します。
// コマンドとは TVTest の各機能を実行するためのものです。
// コマンドの情報は MsgGetAppCommandInfo で取得できます。
/*
	// 例
	MsgDoCommand(pParam, L"Options");	// 設定ダイアログを表示
*/
inline bool MsgDoCommand(PluginParam *pParam,LPCWSTR pszCommand)
{
	return (*pParam->Callback)(pParam,MESSAGE_DOCOMMAND,(LPARAM)pszCommand,0)!=0;
}

#endif	// TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,7)

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,8)

// B-CAS の情報
struct BCasInfo {
	DWORD Size;						// 構造体のサイズ
	WORD CASystemID;				// CA_system_id
	BYTE CardID[6];					// カードID
	BYTE CardType;					// カード種別
	BYTE MessagePartitionLength;	// メッセージ分割長
	BYTE SystemKey[32];				// システム鍵
	BYTE InitialCBC[8];				// CBC初期値
	BYTE CardManufacturerID;		// メーカ識別
	BYTE CardVersion;				// バージョン
	WORD CheckCode;					// チェックコード
	char szFormatCardID[25];		// 可読形式のカードID (4桁の数字x5)
};

// B-CAS カードの情報を取得する
// ※ B-CAS 関連の機能は削除されました。現在は利用できません。
// カードが開かれていない場合は false が返ります。
inline bool MsgGetBCasInfo(PluginParam *pParam,BCasInfo *pInfo)
{
	return (*pParam->Callback)(pParam,MESSAGE_GETBCASINFO,(LPARAM)pInfo,0)!=0;
}

// B-CAS コマンドの情報
struct BCasCommandInfo {
	const BYTE *pSendData;	// 送信データ
	DWORD SendSize;			// 送信サイズ (バイト単位)
	BYTE *pReceiveData;		// 受信データ
	DWORD ReceiveSize;		// 受信サイズ (バイト単位)
};

// B-CAS カードにコマンドを送信する
// ※ B-CAS 関連の機能は削除されました。現在は利用できません。
// BCasCommandInfo の pSendData に送信データへのポインタを指定して、
// SendSize に送信データのバイト数を設定します。
// また、pReceiveData に受信データを格納するバッファへのポインタを指定して、
// ReceiveSize にバッファに格納できる最大サイズを設定します。
// 送信が成功すると、ReceiveSize に受信したデータのサイズが返されます。
inline bool MsgSendBCasCommand(PluginParam *pParam,BCasCommandInfo *pInfo)
{
	return (*pParam->Callback)(pParam,MESSAGE_SENDBCASCOMMAND,(LPARAM)pInfo,0)!=0;
}

// B-CAS カードにコマンドを送信する
// ※ B-CAS 関連の機能は削除されました。現在は利用できません。
inline bool MsgSendBCasCommand(PluginParam *pParam,const BYTE *pSendData,DWORD SendSize,BYTE *pReceiveData,DWORD *pReceiveSize)
{
	BCasCommandInfo Info;
	Info.pSendData=pSendData;
	Info.SendSize=SendSize;
	Info.pReceiveData=pReceiveData;
	Info.ReceiveSize=*pReceiveSize;
	if (!MsgSendBCasCommand(pParam,&Info))
		return false;
	*pReceiveSize=Info.ReceiveSize;
	return true;
}

// ホストプログラムの情報
struct HostInfo {
	DWORD Size;						// 構造体のサイズ
	LPCWSTR pszAppName;				// プログラム名 ("TVTest"、"TVH264" など)
	struct {
		int Major;					// メジャーバージョン
		int Minor;					// マイナーバージョン
		int Build;					// ビルドナンバー
	} Version;
	LPCWSTR pszVersionText;			// バージョン文字列 ("1.2.0" など)
	DWORD SupportedPluginVersion;	// 対応しているプラグインのバージョン
									// TVTEST_PLUGIN_VERSION_(?,?,?)
};

// ホストプログラムの情報を取得する
// 事前に HostInfo の Size メンバに構造体のサイズを設定して呼び出します。
inline bool MsgGetHostInfo(PluginParam *pParam,HostInfo *pInfo)
{
	return (*pParam->Callback)(pParam,MESSAGE_GETHOSTINFO,(LPARAM)pInfo,0)!=0;
}

#endif	// TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,8)

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,9)

// 設定の情報
struct SettingInfo {
	LPCWSTR pszName;		// 設定名
	DWORD Type;				// 値の型 (SETTING_TYPE_???)
	union {
		int Int;			// int
		unsigned int UInt;	// unsigned int
		LPWSTR pszString;	// 文字列
		void *pData;		// データ
	} Value;
	DWORD ValueSize;		// 値のサイズ (バイト単位)
};

// 設定の値の型
enum SettingType {
	SETTING_TYPE_UNDEFINED,	// 未定義
	SETTING_TYPE_INT,		// int
	SETTING_TYPE_UINT,		// unsigned int
	SETTING_TYPE_STRING,	// 文字列
	SETTING_TYPE_DATA		// データ
};

/*
	今のところ以下の設定が取得できます。
	設定名の大文字と小文字は区別されません。

	設定名                内容                                型

	DriverDirectory       BonDriver の検索ディレクトリ        文字列
	IniFilePath           Ini ファイルのパス                  文字列
	RecordFolder          録画時の保存先フォルダ              文字列

	* ver.0.0.14 以降
	OSDFont               OSD のフォント                      データ(LOGFONT)
	PanelFont             パネルのフォント                    データ(LOGFONT)
	ProgramGuideFont      番組表のフォント                    データ(LOGFONT)
	StatusBarFont         ステータスバーのフォント            データ(LOGFONT)
	DPI                   UIのDPI                             int
*/

// 設定を取得する
// 呼び出す前に SettingInfo のメンバを設定しておきます。
// 文字列とデータの場合、ValueSize に設定の格納に必要なバイト数が返されます。
// 通常は下にある型ごとにオーバーロードされた関数を使用した方が便利です。
inline bool MsgGetSetting(PluginParam *pParam,SettingInfo *pInfo)
{
	return (*pParam->Callback)(pParam,MESSAGE_GETSETTING,(LPARAM)pInfo,0)!=0;
}

// intの設定を取得する
inline bool MsgGetSetting(PluginParam *pParam,LPCWSTR pszName,int *pValue)
{
	SettingInfo Info;
	Info.pszName=pszName;
	Info.Type=SETTING_TYPE_INT;
	Info.ValueSize=sizeof(int);
	if (!(*pParam->Callback)(pParam,MESSAGE_GETSETTING,(LPARAM)&Info,0))
		return false;
	*pValue=Info.Value.Int;
	return true;
}

// unsigned intの設定を取得する
inline bool MsgGetSetting(PluginParam *pParam,LPCWSTR pszName,unsigned int *pValue)
{
	SettingInfo Info;
	Info.pszName=pszName;
	Info.Type=SETTING_TYPE_UINT;
	Info.ValueSize=sizeof(unsigned int);
	if (!(*pParam->Callback)(pParam,MESSAGE_GETSETTING,(LPARAM)&Info,0))
		return false;
	*pValue=Info.Value.UInt;
	return true;
}

// 文字列の設定を取得する
// 戻り値は取得した文字列の長さ(終端のNullを含む)です。
// pszString を NULL にすると、必要なバッファの長さ(終端のNullを含む)が返ります。
// 設定が取得できなかった場合は 0 が返ります。
/*
	// 例
	WCHAR szIniPath[MAX_PATH];
	if (MsgGetSetting(pParam, L"IniFilePath", szIniPath, MAX_PATH) > 0) {
		// 呼び出しが成功した場合は、szIniPath に Ini ファイルのパスが格納されています
	}
*/
inline DWORD MsgGetSetting(PluginParam *pParam,LPCWSTR pszName,LPWSTR pszString,DWORD MaxLength)
{
	SettingInfo Info;
	Info.pszName=pszName;
	Info.Type=SETTING_TYPE_STRING;
	Info.Value.pszString=pszString;
	Info.ValueSize=MaxLength*sizeof(WCHAR);
	if (!(*pParam->Callback)(pParam,MESSAGE_GETSETTING,(LPARAM)&Info,0))
		return 0;
	return Info.ValueSize/sizeof(WCHAR);
}

// フォントの設定を取得する
inline bool MsgGetSetting(PluginParam *pParam,LPCWSTR pszName,LOGFONTW *pFont)
{
	SettingInfo Info;
	Info.pszName=pszName;
	Info.Type=SETTING_TYPE_DATA;
	Info.ValueSize=sizeof(LOGFONTW);
	Info.Value.pData=pFont;
	return (*pParam->Callback)(pParam,MESSAGE_GETSETTING,(LPARAM)&Info,0)!=FALSE;
}

// BonDriverのフルパス名を取得する
// 戻り値はパスの長さ(終端のNullを除く)が返ります。
// pszPathをNULLで呼べば長さだけを取得できます。
inline int MsgGetDriverFullPathName(PluginParam *pParam,LPWSTR pszPath,int MaxLength)
{
	return (int)(*pParam->Callback)(pParam,MESSAGE_GETDRIVERFULLPATHNAME,(LPARAM)pszPath,MaxLength);
}

#endif	// TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,9)

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,10)

// ロゴの画像を取得する
// LogoTypeは0から5までで指定します。以下のサイズのロゴが取得されます。
// 0 = 48x24 / 1 = 36x24 / 2 = 48x27 / 3 = 72x36 / 4 = 54x36 / 5 = 64x36
// いずれのロゴも16:9で表示すると本来の比率になります。
// 画像が取得できた場合はビットマップ(DIBセクション)のハンドルが返ります。
// 画像が取得できない場合はNULLが返ります。
// ビットマップは不要になった時にDeleteObject()で破棄してください。
inline HBITMAP MsgGetLogo(PluginParam *pParam,WORD NetworkID,WORD ServiceID,BYTE LogoType)
{
	return (HBITMAP)(*pParam->Callback)(pParam,MESSAGE_GETLOGO,MAKELONG(NetworkID,ServiceID),LogoType);
}

// 利用可能なロゴの種類を取得する
// 利用可能なロゴを表すフラグが返ります。
// 下位から1ビットごとにLogoTypeの0から5までを表し、ビットが1であればその種類のロゴが利用できます。
/*
	// 例
	if (MsgGetAvailableLogoType(pParam, NetworkID, ServiceID) & 1) {
		// タイプ0のロゴが利用できる
	}
*/
inline UINT MsgGetAvailableLogoType(PluginParam *pParam,WORD NetworkID,WORD ServiceID)
{
	return (UINT)(*pParam->Callback)(pParam,MESSAGE_GETAVAILABLELOGOTYPE,MAKELONG(NetworkID,ServiceID),0);
}

// 録画ファイルを切り替える
// 現在録画中のファイルを閉じて、指定されたパスにファイルを作成して続きを録画するようにします。
// 新しいファイルが開けなかった場合は、今までのファイルで録画が継続されます。
inline bool MsgRelayRecord(PluginParam *pParam,LPCWSTR pszFileName)
{
	return (*pParam->Callback)(pParam,MESSAGE_RELAYRECORD,(LPARAM)pszFileName,0)!=0;
}

// サイレントモードのパラメータ
enum {
	SILENTMODE_GET,	// 取得
	SILENTMODE_SET	// 設定
};

// サイレントモードの取得
// サイレントモードであるか取得します。
// サイレントモードではエラー時などにダイアログが出なくなります。
// コマンドラインで /silent を指定するか、MsgSetSilentMode で設定すればサイレントモードになります。
inline bool MsgGetSilentMode(PluginParam *pParam)
{
	return (*pParam->Callback)(pParam,MESSAGE_SILENTMODE,SILENTMODE_GET,0)!=0;
}

// サイレントモードの設定
// サイレントモードの有効/無効を設定します。
inline bool MsgSetSilentMode(PluginParam *pParam,bool fSilent)
{
	return (*pParam->Callback)(pParam,MESSAGE_SILENTMODE,SILENTMODE_SET,(LPARAM)fSilent)!=0;
}

// 録画のクライアント
enum {
	RECORD_CLIENT_USER,			// ユーザーの操作
	RECORD_CLIENT_COMMANDLINE,	// コマンドラインでの指定
	RECORD_CLIENT_PLUGIN		// プラグインからの指定
};

// 録画開始情報で変更した項目
enum {
	STARTRECORD_MODIFIED_FILENAME	=0x00000001UL	// ファイル名
};

// 録画開始情報
struct StartRecordInfo {
	DWORD Size;				// 構造体のサイズ
	DWORD Flags;			// フラグ(現在未使用)
	DWORD Modified;			// 変更した項目(STARTRECORD_MODIFIED_???)
	DWORD Client;			// 録画のクライアント(RECORD_CLIENT_???)
	LPWSTR pszFileName;		// ファイル名
	int MaxFileName;		// ファイル名の最大長
	DWORD StartTimeSpec;	// 開始時間の指定方法(RECORD_START_???)
	FILETIME StartTime;		// 指定された開始時刻(ローカル時刻)
							// StartTimeSpec!=RECORD_START_NOTSPECIFIED の場合のみ有効
	DWORD StopTimeSpec;		// 停止時間の指定方法(RECORD_STOP_???)
	union {
		FILETIME Time;		// 停止時刻(StopTimeSpec==RECORD_STOP_TIME)
							// ローカル時刻
		ULONGLONG Duration;	// 録画停止までの時間(StopTimeSpec==RECORD_STOP_DURATION)
							// 開始時刻からのミリ秒
	} StopTime;
};

#endif	// TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,10)

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,11)

/*
メッセージコールバック関数を登録すると、TVTest のメインウィンドウにメッセージが
送られた時に呼び出されます。
コールバック関数では、TVTest にメッセージの処理をさせない時は TRUE を返します。
その際、pResult に書き込んだ値が返されます。
*/

// ウィンドウメッセージコールバック関数
typedef BOOL (CALLBACK *WindowMessageCallbackFunc)(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,LRESULT *pResult,void *pUserData);

// ウィンドウメッセージコールバックの設定
// pClientData はコールバックの呼び出し時に渡されます。
// 一つのプラグインで設定できるコールバック関数は一つだけです。
// Callback に NULL を渡すと設定が解除されます。
inline bool MsgSetWindowMessageCallback(PluginParam *pParam,WindowMessageCallbackFunc Callback,void *pClientData=NULL) {
	return (*pParam->Callback)(pParam,MESSAGE_SETWINDOWMESSAGECALLBACK,(LPARAM)Callback,(LPARAM)pClientData)!=0;
}

/*
コントローラの登録を行うと、設定ダイアログのリモコンのページで割り当てが設定できるようになります。
コントローラの画像を用意すると、設定の右側に表示されます。
ボタンが押された時に MsgOnControllerButtonDown を呼び出して通知すると、
割り当ての設定に従って機能が実行されます。
*/

// コントローラのボタンの情報
struct ControllerButtonInfo {
	LPCWSTR pszName;				// ボタンの名称("音声切替" など)
	LPCWSTR pszDefaultCommand;		// デフォルトのコマンド(MsgDoCommand と同じもの)
									// 指定しない場合はNULL
	struct {
		WORD Left,Top,Width,Height;	// 画像のボタンの位置(画像が無い場合は無視される)
	} ButtonRect;
	struct {
		WORD Left,Top;				// 画像の選択ボタンの位置(画像が無い場合は無視される)
	} SelButtonPos;
	DWORD Reserved;					// 予約領域(0にしてください)
};

// コントローラの情報
struct ControllerInfo {
	DWORD Size;									// 構造体のサイズ
	DWORD Flags;								// 各種フラグ(CONTROLLER_FLAG_???)
	LPCWSTR pszName;							// コントローラ識別名
	LPCWSTR pszText;							// コントローラの名称("HDUSリモコン" など)
	int NumButtons;								// ボタンの数
	const ControllerButtonInfo *pButtonList;	// ボタンのリスト
	LPCWSTR pszIniFileName;						// 設定ファイル名(NULL にすると TVTest の Ini ファイル)
	LPCWSTR pszSectionName;						// 設定のセクション名
	UINT ControllerImageID;						// コントローラの画像の識別子(無い場合は0)
	UINT SelButtonsImageID;						// 選択ボタン画像の識別子(無い場合は0)
	typedef BOOL (CALLBACK *TranslateMessageCallback)(HWND hwnd,MSG *pMessage,void *pClientData);
	TranslateMessageCallback pTranslateMessage;	// メッセージの変換コールバック(必要無ければ NULL)
	void *pClientData;							// コールバックに渡すパラメータ
};

// コントローラのフラグ
enum {
	CONTROLLER_FLAG_ACTIVEONLY	=0x00000001UL	// アクティブ時のみ使用できる
};

// コントローラを登録する
inline bool MsgRegisterController(PluginParam *pParam,const ControllerInfo *pInfo)
{
	return (*pParam->Callback)(pParam,MESSAGE_REGISTERCONTROLLER,(LPARAM)pInfo,0)!=0;
}

// コントローラのボタンが押されたことを通知する
inline bool MsgOnControllerButtonDown(PluginParam *pParam,LPCWSTR pszName,int Button)
{
	return (*pParam->Callback)(pParam,MESSAGE_ONCONTROLLERBUTTONDOWN,(LPARAM)pszName,Button)!=0;
}

// コントローラの設定
struct ControllerSettings {
	DWORD Mask;		// 取得する項目(CONTROLLER_SETTINGS_MASK_*)
	DWORD Flags;	// 各種フラグ(CONTROLLER_SETTINGS_FLAG_*)
};

// コントローラの設定マスク
enum {
	CONTROLLER_SETTINGS_MASK_FLAGS		=0x00000001UL	// Flags が有効
};

// コントローラの設定フラグ
enum {
	CONTROLLER_SETTINGS_FLAG_ACTIVEONLY	=0x00000001UL	// アクティブ時のみ
};

// コントローラの設定を取得する
inline bool MsgGetControllerSettings(PluginParam *pParam,LPCWSTR pszName,ControllerSettings *pSettings)
{
	return (*pParam->Callback)(pParam,MESSAGE_GETCONTROLLERSETTINGS,(LPARAM)pszName,(LPARAM)pSettings)!=0;
}

// コントローラがアクティブ時のみに設定されているか取得する
inline bool MsgIsControllerActiveOnly(PluginParam *pParam,LPCWSTR pszName)
{
	ControllerSettings Settings;
	Settings.Mask=CONTROLLER_SETTINGS_MASK_FLAGS;
	if (!MsgGetControllerSettings(pParam,pszName,&Settings))
		return false;
	return (Settings.Flags&CONTROLLER_SETTINGS_FLAG_ACTIVEONLY)!=0;
}

#endif	// TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,11)

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,12)

// イベントの取得方法
enum {
	EPG_EVENT_QUERY_EVENTID,	// イベントID
	EPG_EVENT_QUERY_TIME		// 日時
};

// イベントの取得のための情報
struct EpgEventQueryInfo {
	WORD NetworkID;			// ネットワークID
	WORD TransportStreamID;	// ストリームID
	WORD ServiceID;			// サービスID
	BYTE Type;				// 取得方法(EPG_EVENT_QUERY_*)
	BYTE Flags;				// フラグ(現在は常に0)
	union {
		WORD EventID;		// イベントID
		FILETIME Time;		// 日時(UTC)
	};
};

// 映像の情報
struct EpgEventVideoInfo {
	BYTE StreamContent;	// stream_content
	BYTE ComponentType;	// component_type
						// (0x01 = 480i[4:3] / 0x03 = 480i[16:9] / 0xB1 = 1080i[4:3] / 0xB3 = 1080i[16:9])
	BYTE ComponentTag;	// component_tag
	BYTE Reserved;		// 予約
	DWORD LanguageCode;	// 言語コード
	LPCWSTR pszText;	// テキスト(無い場合はNULL)
};

// 音声の情報
struct EpgEventAudioInfo {
	BYTE Flags;				// フラグ(EPG_EVENT_AUDIO_FLAG_*)
	BYTE StreamContent;		// stream_content
	BYTE ComponentType;		// component_type (1 = Mono / 2 = Dual Mono / 3 = Stereo / 9 = 5.1ch)
	BYTE ComponentTag;		// component_tag
	BYTE SimulcastGroupTag;	// simulcast_group_tag
	BYTE QualityIndicator;	// quality_indicator
	BYTE SamplingRate;		// サンプリング周波数の種類
	BYTE Reserved;			// 予約
	DWORD LanguageCode;		// 言語コード(主音声)
	DWORD LanguageCode2;	// 言語コード(副音声)
	LPCWSTR pszText;		// テキスト(無い場合はNULL)
};

// 音声のフラグ
enum {
	EPG_EVENT_AUDIO_FLAG_MULTILINGUAL	=0x01,	// 二ヶ国語
	EPG_EVENT_AUDIO_FLAG_MAINCOMPONENT	=0x02	// 主音声
};

// ジャンルの情報
// (意味は STD-B10 第2部 付録H 等参照)
struct EpgEventContentInfo {
	BYTE ContentNibbleLevel1;	// 大分類
	BYTE ContentNibbleLevel2;	// 中分類
	BYTE UserNibble1;
	BYTE UserNibble2;
};

// イベントグループのイベントの情報
struct EpgGroupEventInfo {
	WORD NetworkID;			// ネットワークID
	WORD TransportStreamID;	// ストリームID
	WORD ServiceID;			// サービスID
	WORD EventID;			// イベントID
};

// イベントグループの情報
struct EpgEventGroupInfo {
	BYTE GroupType;					// 種類
	BYTE EventListLength;			// イベントのリストの要素数
	BYTE Reserved[6];				// 予約
	EpgGroupEventInfo *EventList;	// イベントのリスト
};

// イベントの情報
struct EpgEventInfo {
	WORD EventID;						// イベントID
	BYTE RunningStatus;					// running_status
	BYTE FreeCaMode;					// free_CA_mode
	DWORD Reserved;						// 予約
	SYSTEMTIME StartTime;				// 開始日時(ローカル時刻)
	DWORD Duration;						// 長さ(秒単位)
	BYTE VideoListLength;				// 映像の情報の数
	BYTE AudioListLength;				// 音声の情報の数
	BYTE ContentListLength;				// ジャンルの情報の数
	BYTE EventGroupListLength;			// イベントグループの情報の数
	LPCWSTR pszEventName;				// イベント名(無い場合はNULL)
	LPCWSTR pszEventText;				// テキスト(無い場合はNULL)
	LPCWSTR pszEventExtendedText;		// 拡張テキスト(無い場合はNULL)
	EpgEventVideoInfo **VideoList;		// 映像の情報のリスト(無い場合はNULL)
	EpgEventAudioInfo **AudioList;		// 音声の情報のリスト(無い場合はNULL)
	EpgEventContentInfo *ContentList;	// ジャンルの情報(無い場合はNULL)
	EpgEventGroupInfo **EventGroupList;	// イベントグループ情報(無い場合はNULL)
};

// イベントのリスト
struct EpgEventList {
	WORD NetworkID;				// ネットワークID
	WORD TransportStreamID;		// ストリームID
	WORD ServiceID;				// サービスID
	WORD NumEvents;				// イベントの数
	EpgEventInfo **EventList;	// リスト
};

// イベントの情報の取得
// EpgEventQueryInfo で、取得したいイベントを指定します。
// 取得した情報が不要になった場合、MsgFreeEventInfo で解放します。
// 情報が取得できなかった場合は NULL が返ります。
/*
	// 現在のチャンネルの現在の番組を取得する例
	ChannelInfo ChInfo;
	ChInfo.Size = sizeof(ChInfo);
	if (MsgGetCurrentChannelInfo(pParam, &ChInfo)) {
		EpgEventQueryInfo QueryInfo;
		QueryInfo.NetworkID         = ChInfo.NetworkID;
		QueryInfo.TransportStreamID = ChInfo.TransportStreamID;
		QueryInfo.ServiceID         = ChInfo.ServiceID;
		QueryInfo.Type              = EPG_EVENT_QUERY_TIME;
		QueryInfo.Flags             = 0;
		GetSystemTimeAsFileTime(&QueryInfo.Time);
		EpgEventInfo *pEvent = MsgGetEpgEventInfo(pParam, &QueryInfo);
		if (pEvent != NULL) {
			...
			MsgFreeEpgEventInfo(pParam, pEvent);
		}
	}
*/
inline EpgEventInfo *MsgGetEpgEventInfo(PluginParam *pParam,const EpgEventQueryInfo *pInfo)
{
	return (EpgEventInfo*)(*pParam->Callback)(pParam,MESSAGE_GETEPGEVENTINFO,(LPARAM)pInfo,0);
}

// イベントの情報の解放
// MsgGetEpgEventInfo で取得した情報のメモリを解放します。
inline void MsgFreeEpgEventInfo(PluginParam *pParam,EpgEventInfo *pEventInfo)
{
	(*pParam->Callback)(pParam,MESSAGE_FREEEPGEVENTINFO,(LPARAM)pEventInfo,0);
}

// イベントのリストの取得
// EpgEventList の NetworkID / TransportStreamID / ServiceID を設定して呼び出します。
// 取得された番組は開始日時順にソートされています。
// リストが不要になった場合、MsgFreeEpgEventList で解放します。
/*
	// 現在のチャンネルの番組表を取得する例
	ChannelInfo ChInfo;
	ChInfo.Size = sizeof(ChInfo);
	if (MsgGetCurrentChannelInfo(pParam, &ChInfo)) {
		EpgEventList EventList;
		EventList.NetworkID         = ChInfo.NetworkID;
		EventList.TransportStreamID = ChInfo.TransportStreamID;
		EventList.ServiceID         = ChInfo.ServiceID;
		if (MsgGetEpgEventList(pParam, &EventList)) {
			...
			MsgFreeEpgEventList(pParam, &EventList);
		}
	}
*/
inline bool MsgGetEpgEventList(PluginParam *pParam,EpgEventList *pList)
{
	return (*pParam->Callback)(pParam,MESSAGE_GETEPGEVENTLIST,(LPARAM)pList,0)!=0;
}

// イベントのリストの解放
// MsgGetEpgEventList で取得したリストのメモリを解放します。
inline void MsgFreeEpgEventList(PluginParam *pParam,EpgEventList *pList)
{
	(*pParam->Callback)(pParam,MESSAGE_FREEEPGEVENTLIST,(LPARAM)pList,0);
}

// BonDriver を列挙する
// BonDriver のフォルダとして設定されているフォルダ内の BonDriver を列挙します。
// 戻り値としてファイル名の長さが返ります。Index が範囲外の場合は0が返ります。
inline int MsgEnumDriver(PluginParam *pParam,int Index,LPWSTR pszFileName,int MaxLength)
{
	return (int)(*pParam->Callback)(pParam,MESSAGE_ENUMDRIVER,(LPARAM)pszFileName,MAKELPARAM(Index,min(MaxLength,0xFFFF)));
}

// チューニング空間の情報
struct DriverTuningSpaceInfo {
	DWORD Flags;				// フラグ(現在は常に0)
	DWORD NumChannels;			// チャンネル数
	TuningSpaceInfo *pInfo;		// チューニング空間の情報
	ChannelInfo **ChannelList;	// チャンネルのリスト
};

// チューニング空間のリスト
struct DriverTuningSpaceList {
	DWORD Flags;						// フラグ(現在は常に0)
	DWORD NumSpaces;					// チューニング空間の数
	DriverTuningSpaceInfo **SpaceList;	// チューニング空間のリスト
};

// BonDriver のチャンネルのリストを取得する
// DriverTuningSpaceList の Flags を設定して呼び出します。
// リストが不要になった場合、MsgFreeDriverTuningSpaceList で解放します。
inline bool MsgGetDriverTuningSpaceList(PluginParam *pParam,LPCWSTR pszDriverName,DriverTuningSpaceList *pList)
{
	return (*pParam->Callback)(pParam,MESSAGE_GETDRIVERTUNINGSPACELIST,(LPARAM)pszDriverName,(LPARAM)pList)!=0;
}

// BonDriver のチャンネルのリストを解放する
inline void MsgFreeDriverTuningSpaceList(PluginParam *pParam,DriverTuningSpaceList *pList)
{
	(*pParam->Callback)(pParam,MESSAGE_FREEDRIVERTUNINGSPACELIST,(LPARAM)pList,0);
}

#endif	// TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,12)

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,13)

// 番組表の番組の情報
struct ProgramGuideProgramInfo {
	WORD NetworkID;			// ネットワークID
	WORD TransportStreamID;	// ストリームID
	WORD ServiceID;			// サービスID
	WORD EventID;			// イベントID
	SYSTEMTIME StartTime;	// 開始日時
	DWORD Duration;			// 長さ(秒単位)
};

// 番組表の番組の背景描画の情報
struct ProgramGuideProgramDrawBackgroundInfo {
	HDC hdc;					// 描画先DCハンドル
	RECT ItemRect;				// 項目全体の位置
	RECT TitleRect;				// タイトルの位置
	RECT ContentRect;			// 番組内容の位置
	COLORREF BackgroundColor;	// 背景色
};

// 番組表のメニューの情報
struct ProgramGuideInitializeMenuInfo {
	HMENU hmenu;		// メニューのハンドル
	UINT Command;		// 項目のID
	UINT Reserved;		// 予約
};

// 番組表の番組のメニューの情報
struct ProgramGuideProgramInitializeMenuInfo {
	HMENU hmenu;		// メニューのハンドル
	UINT Command;		// 項目のID
	UINT Reserved;		// 予約
	POINT CursorPos;	// カーソル位置
	RECT ItemRect;		// 番組の位置
};

// 番組表のイベントのフラグ
enum {
	PROGRAMGUIDE_EVENT_GENERAL	=0x0001,	// 全体のイベント(EVENT_PROGRAMGUIDE_*)
	PROGRAMGUIDE_EVENT_PROGRAM	=0x0002		// 各番組のイベント(EVENT_PROGRAMGUIDE_PROGRAM_*)
};

// 番組表のイベントの有効/無効を設定する
// 番組表のイベント(EVENT_PROGRAMGUIDE_*)の通知が必要な場合、通知を有効に設定します。
// EventFlags で指定されたイベント(PROGRAMGUIDE_EVENT_*)の通知が有効になります。
inline bool MsgEnableProgramGuideEvent(PluginParam *pParam,UINT EventFlags)
{
	return (*pParam->Callback)(pParam,MESSAGE_ENABLEPROGRAMGUIDEEVENT,EventFlags,0)!=0;
}

// 番組表のコマンドの情報
struct ProgramGuideCommandInfo {
	WORD Type;			// 種類(PROGRAMGUiDE_COMMAND_TYPE_*)
	WORD Flags;			// 各種フラグ(現在は常に0)
	UINT ID;			// 識別子
	LPCWSTR pszText;	// コマンドの文字列
	LPCWSTR pszName;	// コマンドの名前
};

// 番組表のコマンドの種類
enum {
	PROGRAMGUIDE_COMMAND_TYPE_PROGRAM	=1	// 各番組
};

// 番組表のコマンド実行時の情報
struct ProgramGuideCommandParam {
	UINT ID;							// 識別子
	UINT Action;						// 操作の種類(PROGRAMGUIDE_COMMAND_ACTION_*)
	ProgramGuideProgramInfo Program;	// 番組の情報
	POINT CursorPos;					// カーソル位置
	RECT ItemRect;						// 項目の位置
};

// 番組表のコマンド実行の操作の種類
enum {
	PROGRAMGUIDE_COMMAND_ACTION_MOUSE,	// マウスなど
	PROGRAMGUIDE_COMMAND_ACTION_KEY		// キーボード
};

// 番組表のコマンドを登録する
// コマンドを登録すると、番組表のダブルクリックなどに機能を割り当てることができるようになります。
// コマンドが実行されると EVENT_PROGRAMGUIDE_COMMAND イベントが送られます。
inline bool MsgRegisterProgramGuideCommand(PluginParam *pParam,
										   const ProgramGuideCommandInfo *pCommandList,int NumCommands=1)
{
	return (*pParam->Callback)(pParam,MESSAGE_REGISTERPROGRAMGUIDECOMMAND,(LPARAM)pCommandList,NumCommands)!=0;
}

#endif	// TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,13)

#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,14)

// フィルタグラフの情報
// フィルタグラフ関係のイベント(EVENT_FILTERGRAPH_*)で渡されます。
struct FilterGraphInfo {
	DWORD Flags;							// 各種フラグ(現在は常に0)
	BYTE VideoStreamType;					// 映像stream_type
	BYTE Reserved[3];						// 予約
	interface IGraphBuilder *pGraphBuilder;	// IGraphBuilder
};

// スタイル値の単位
enum {
	STYLE_UNIT_UNDEFINED,	// 未定義
	STYLE_UNIT_PIXEL,		// ピクセル
	STYLE_UNIT_POINT,		// ポイント
	STYLE_UNIT_DIP			// dip
};

// スタイル値の情報
struct StyleValueInfo {
	DWORD Size;			// 構造体のサイズ
	DWORD Flags;		// 各種フラグ(現在は常に0)
	LPCWSTR pszName;	// スタイル名
	int Unit;			// 取得する値の単位(STYLE_UNIT_*)
	int Value;			// 取得された値
};

// スタイル値を取得する
// 通常は下のオーバーロードされた関数を利用する方が簡単です。
inline bool MsgGetStyleValue(PluginParam *pParam,StyleValueInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_GETSTYLEVALUE,(LPARAM)pInfo,0)!=FALSE;
}

// 指定された単位のスタイル値を取得する
inline bool MsgGetStyleValue(PluginParam *pParam,LPCWSTR pszName,int Unit,int *pValue) {
	StyleValueInfo Info;
	Info.Size=sizeof(Info);
	Info.Flags=0;
	Info.pszName=pszName;
	Info.Unit=Unit;
	if (!MsgGetStyleValue(pParam,&Info))
		return false;
	*pValue=Info.Value;
	return true;
}

// オリジナルの単位のスタイル値を取得する
inline bool MsgGetStyleValue(PluginParam *pParam,LPCWSTR pszName,int *pValue) {
	return MsgGetStyleValue(pParam,pszName,STYLE_UNIT_UNDEFINED,pValue);
}

// ピクセル単位のスタイル値を取得する
inline bool MsgGetStyleValuePixels(PluginParam *pParam,LPCWSTR pszName,int *pValue) {
	return MsgGetStyleValue(pParam,pszName,STYLE_UNIT_PIXEL,pValue);
}

// テーマの背景描画情報
struct ThemeDrawBackgroundInfo {
	DWORD Size;			// 構造体のサイズ
	DWORD Flags;		// 各種フラグ(THEME_DRAW_BACKGROUND_FLAG_*)
	LPCWSTR pszStyle;	// スタイル名
	HDC hdc;			// 描画先DC
	RECT DrawRect;		// 描画領域
};

// テーマ描画フラグ
enum {
	THEME_DRAW_BACKGROUND_FLAG_ADJUSTRECT	=0x00000001U	// クライアント領域を取得
};

// テーマの背景を描画する
// 通常は下のオーバーロードされた関数を利用する方が簡単です。
inline bool MsgThemeDrawBackground(PluginParam *pParam,ThemeDrawBackgroundInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_THEMEDRAWBACKGROUND,(LPARAM)pInfo,0)!=FALSE;
}

// テーマの背景を描画する
inline bool MsgThemeDrawBackground(PluginParam *pParam,LPCWSTR pszStyle,HDC hdc,const RECT &DrawRect) {
	ThemeDrawBackgroundInfo Info;
	Info.Size=sizeof(Info);
	Info.Flags=0;
	Info.pszStyle=pszStyle;
	Info.hdc=hdc;
	Info.DrawRect=DrawRect;
	return MsgThemeDrawBackground(pParam,&Info);
}

// テーマの文字列描画情報
struct ThemeDrawTextInfo {
	DWORD Size;			// 構造体のサイズ
	DWORD Flags;		// 各種フラグ(現在は常に0)
	LPCWSTR pszStyle;	// スタイル名
	HDC hdc;			// 描画先DC
	LPCWSTR pszText;	// 描画する文字列
	RECT DrawRect;		// 描画先の領域
	UINT DrawFlags;		// 描画フラグ(DrawText API の DT_*)
	COLORREF Color;		// 描画する色(CLR_INVALID でデフォルトの色)
};

// テーマの文字列を描画する
// 通常は下のオーバーロードされた関数を利用する方が簡単です。
inline bool MsgThemeDrawText(PluginParam *pParam,ThemeDrawTextInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_THEMEDRAWTEXT,(LPARAM)pInfo,0)!=FALSE;
}

// テーマの文字列を描画する
inline bool MsgThemeDrawText(PluginParam *pParam,
		LPCWSTR pszStyle,HDC hdc,LPCWSTR pszText,const RECT &DrawRect,
		UINT DrawFlags,COLORREF Color=CLR_INVALID) {
	ThemeDrawTextInfo Info;
	Info.Size=sizeof(Info);
	Info.Flags=0;
	Info.pszStyle=pszStyle;
	Info.hdc=hdc;
	Info.pszText=pszText;
	Info.DrawRect=DrawRect;
	Info.DrawFlags=DrawFlags;
	Info.Color=Color;
	return MsgThemeDrawText(pParam,&Info);
}

// テーマのアイコン描画情報
struct ThemeDrawIconInfo {
	DWORD Size;			// 構造体のサイズ
	DWORD Flags;		// 各種フラグ(現在は常に0)
	LPCWSTR pszStyle;	// スタイル名
	HDC hdc;			// 描画先DC
	HBITMAP hbm;		// 描画するビットマップ
	RECT DstRect;		// 描画先の領域
	RECT SrcRect;		// 描画元の領域
	COLORREF Color;		// 描画する色(CLR_INVALID でデフォルトの色)
	BYTE Opacity;		// 不透明度(1-255)
	BYTE Reserved[3];	// 予約領域
};

// テーマのアイコンを描画する
// 通常は下のオーバーロードされた関数を利用する方が簡単です。
inline bool MsgThemeDrawIcon(PluginParam *pParam,ThemeDrawIconInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_THEMEDRAWICON,(LPARAM)pInfo,0)!=FALSE;
}

// テーマのアイコンを描画する
inline bool MsgThemeDrawIcon(PluginParam *pParam,LPCWSTR pszStyle,
		HDC hdc,int DstX,int DstY,int DstWidth,int DstHeight,
		HBITMAP hbm,int SrcX,int SrcY,int SrcWidth,int SrcHeight,
		COLORREF Color=CLR_INVALID,BYTE Opacity=255) {
	ThemeDrawIconInfo Info;
	Info.Size=sizeof(Info);
	Info.Flags=0;
	Info.pszStyle=pszStyle;
	Info.hdc=hdc;
	Info.hbm=hbm;
	Info.DstRect.left=DstX;
	Info.DstRect.top=DstY;
	Info.DstRect.right=DstX+DstWidth;
	Info.DstRect.bottom=DstY+DstHeight;
	Info.SrcRect.left=SrcX;
	Info.SrcRect.top=SrcY;
	Info.SrcRect.right=SrcX+SrcWidth;
	Info.SrcRect.bottom=SrcY+SrcHeight;
	Info.Color=Color;
	Info.Opacity=Opacity;
	return MsgThemeDrawIcon(pParam,&Info);
}

// EPG 取得状況の情報
struct EpgCaptureStatusInfo {
	DWORD Size;				// 構造体のサイズ
	WORD Flags;				// 各種フラグ(現在は常に0)
	WORD NetworkID;			// ネットワークID
	WORD TransportStreamID;	// ストリームID
	WORD ServiceID;			// サービスID
	DWORD Status;			// ステータス(EPG_CAPTURE_STATUS_*)
};

// EPG 取得状況のステータス
enum {
	EPG_CAPTURE_STATUS_SCHEDULEBASICCOMPLETED		=0x00000001U,	// schedule basic が揃っている
	EPG_CAPTURE_STATUS_SCHEDULEEXTENDEDCOMPLETED	=0x00000002U,	// schedule extended が揃っている
	EPG_CAPTURE_STATUS_HASSCHEDULEBASIC				=0x00000004U,	// schedule basic が存在する
	EPG_CAPTURE_STATUS_HASSCHEDULEEXTENDED			=0x00000008U	// schedule extended が存在する
};

// EPG 取得状況を取得する
/*
	// 例
	EpgCaptureStatusInfo Info;
	Info.Size = sizeof(Info);
	Info.Flags = 0;
	// 取得したいサービスを指定する
	Info.NetworkID = NetworkID;
	Info.TransportStreamID = TransportStreamID;
	Info.ServiceID = ServiceID;
	// Info.Status に取得したいステータスを指定する
	Info.Status = EPG_CAPTURE_STATUS_SCHEDULEBASICCOMPLETED |
	              EPG_CAPTURE_STATUS_SCHEDULEEXTENDEDCOMPLETED;
	if (MsgGetEpgCaptureStatus(pParam, &Info)) {
		if (Info.Status & EPG_CAPTURE_STATUS_SCHEDULEBASICCOMPLETED)
			std::printf("schedule basic 揃った\n");
		if (Info.Status & EPG_CAPTURE_STATUS_SCHEDULEEXTENDEDCOMPLETED)
			std::printf("schedule extended 揃った\n");
	}
*/
inline bool MsgGetEpgCaptureStatus(PluginParam *pParam,EpgCaptureStatusInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_GETEPGCAPTURESTATUS,(LPARAM)pInfo,0)!=FALSE;
}

// コマンドの情報
struct AppCommandInfo {
	DWORD Size;			// 構造体のサイズ
	DWORD Index;		// 取得するコマンドのインデックス
	LPWSTR pszText;		// コマンドの文字列
	int MaxText;		// コマンドの文字列のバッファ長
	LPWSTR pszName;		// コマンドの名前
	int MaxName;		// コマンドの名前のバッファ長
};

// コマンドの情報を取得する
// TVTInitialize 内で呼ぶと、プラグインのコマンドなどが取得できませんので注意してください。
// pszText に NULL を指定すると、必要なバッファ長(終端のNull文字を含む)が MaxText に返ります。
// pszName に NULL を指定すると、必要なバッファ長(終端のNull文字を含む)が MaxName に返ります。
// pszText に取得されたコマンド文字列を MsgDoCommand に指定して実行できます。
/*
	// 例
	DWORD Count = MsgGetAppCommandCount(pParam);
	for (DWORD i = 0; i < Count; i++) {
		AppCommandInfo Info;
		WCHAR szText[256],szName[256];
		Info.Size = sizeof(Info);
		Info.Index = i;
		Info.pszText = szText;
		Info.MaxText = _countof(szText);
		Info.pszName = szName;
		Info.MaxName = _countof(szName);
		if (MsgGetAppCommandInfo(pParam,&Info)) {
			std::wprintf(L"Command %u %s %s\n", i, szText, szName);
		}
	}
*/
inline bool MsgGetAppCommandInfo(PluginParam *pParam,AppCommandInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_GETAPPCOMMANDINFO,(LPARAM)pInfo,0)!=FALSE;
}

// コマンドの数を取得する
inline DWORD MsgGetAppCommandCount(PluginParam *pParam) {
	return (DWORD)(*pParam->Callback)(pParam,MESSAGE_GETAPPCOMMANDCOUNT,0,0);
}

// 映像ストリームの数を取得する
inline int MsgGetVideoStreamCount(PluginParam *pParam) {
	return (int)(*pParam->Callback)(pParam,MESSAGE_GETVIDEOSTREAMCOUNT,0,0);
}

// 現在の映像ストリームを取得する
inline int MsgGetVideoStream(PluginParam *pParam) {
	return (int)(*pParam->Callback)(pParam,MESSAGE_GETVIDEOSTREAM,0,0);
}

// 映像ストリームを設定する
inline bool MsgSetVideoStream(PluginParam *pParam,int Stream) {
	return (*pParam->Callback)(pParam,MESSAGE_SETVIDEOSTREAM,Stream,0)!=FALSE;
}

// ログ取得の情報
// Index は現在保持されているログの中でのインデックスを、
// Serial は起動時からの連番を表します。
struct GetLogInfo {
	DWORD Size;		// 構造体のサイズ
	DWORD Flags;	// 各種フラグ(GET_LOG_FLAG_*)
	DWORD Index;	// ログのインデックス
	DWORD Serial;	// ログのシリアルナンバー
	LPWSTR pszText;	// 取得する文字列
	DWORD MaxText;	// 文字列の最大長
	int Type;		// ログの種類(LOG_TYPE_*)
};

// ログ取得のフラグ
enum {
	GET_LOG_FLAG_BYSERIAL	=0x00000001U	// シリアルナンバーから取得
};

// ログを取得する
// GetLogInfo の pszText に NULL を指定すると、
// 必要なバッファ長(終端のNull文字を含む)が MaxText に返ります。
/*
	// 例
	// 全てのログを列挙する
	GetLogInfo Info;
	// まず最初のログのシリアルを取得
	Info.Size = sizeof(Info);
	Info.Flags = 0;
	Info.Index = 0;
	Info.pszText = NULL;
	if (MsgGetLog(pParam, &Info)) {
		// シリアルから順番にログを取得
		WCHAR szText[256];
		Info.Flags = GET_LOG_FLAG_BYSERIAL;
		Info.pszText = szText;
		for (;;) {
			Info.MaxText = _countof(szText);
			if (!MsgGetLog(pParam, &Info))
				break;
			std::wprintf(L"Log %u : %s\n", Info.Serial, Info.pszText);
			Info.Serial++;
		}
	}
*/
inline bool MsgGetLog(PluginParam *pParam,GetLogInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_GETLOG,(LPARAM)pInfo,0)!=FALSE;
}

// ログの数を取得する
// ユーザーがログをクリアするなどして、数が減ることもあり得ます。
inline DWORD MsgGetLogCount(PluginParam *pParam) {
	return (DWORD)(*pParam->Callback)(pParam,MESSAGE_GETLOGCOUNT,0,0);
}

// プラグインのコマンドの情報
// CommandInfo が拡張されたものです。
struct PluginCommandInfo {
	DWORD Size;				// 構造体のサイズ
	DWORD Flags;			// 各種フラグ(PLUGIN_COMMAND_FLAG_*)
	DWORD State;			// 状態フラグ(PLUGIN_COMMAND_STATE_*)
	int ID;					// 識別子
	LPCWSTR pszText;		// コマンドの文字列
	LPCWSTR pszName;		// コマンドの名前
	LPCWSTR pszDescription;	// コマンドの説明
	HBITMAP hbmIcon;		// アイコン
};

// プラグインのコマンドのフラグ
enum {
	PLUGIN_COMMAND_FLAG_ICONIZE			=0x00000001U,	// アイコン表示(サイドバーなどに表示される)
	PLUGIN_COMMAND_FLAG_NOTIFYDRAWICON	=0x00000002U	// アイコン描画の通知(EVENT_DRAWCOMMANDICON で描画を行う)
};

// プラグインコマンドの状態フラグ
enum {
	PLUGIN_COMMAND_STATE_DISABLED		=0x00000001U,	// 無効
	PLUGIN_COMMAND_STATE_CHECKED		=0x00000002U	// チェック
};

// プラグインのコマンドを登録する
// 基本的に MsgRegisterCommand と同じですが、メンバが追加されています。
inline bool MsgRegisterPluginCommand(PluginParam *pParam,const PluginCommandInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_REGISTERPLUGINCOMMAND,(LPARAM)pInfo,0)!=FALSE;
}

// プラグインのコマンドの状態を設定する
inline bool MsgSetPluginCommandState(PluginParam *pParam,int ID,DWORD State) {
	return (*pParam->Callback)(pParam,MESSAGE_SETPLUGINCOMMANDSTATE,ID,State)!=FALSE;
}

// プラグインコマンドの通知の種類
enum {
	PLUGIN_COMMAND_NOTIFY_CHANGEICON	=0x00000001U	// アイコンを再描画する
};

// プラグインコマンドの通知を行う
inline bool MsgPluginCommandNotify(PluginParam *pParam,int ID,unsigned int Type) {
	return (*pParam->Callback)(pParam,MESSAGE_PLUGINCOMMANDNOTIFY,ID,Type)!=FALSE;
}

// プラグインのアイコンの情報
struct PluginIconInfo {
	DWORD Size;			// 構造体のサイズ
	DWORD Flags;		// 各種フラグ(現在は常に0)
	HBITMAP hbmIcon;	// アイコン
};

// プラグインのアイコンを登録する
// アイコンを登録すると、プラグインの有効/無効をサイドバーで切り替えられるようになります。
inline bool MsgRegisterPluginIcon(PluginParam *pParam,const PluginIconInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_REGISTERPLUGINICON,(LPARAM)pInfo,0)!=FALSE;
}

// プラグインのアイコンを登録する
inline bool MsgRegisterPluginIcon(PluginParam *pParam,HBITMAP hbmIcon) {
	PluginIconInfo Info;
	Info.Size=sizeof(Info);
	Info.Flags=0;
	Info.hbmIcon=hbmIcon;
	return MsgRegisterPluginIcon(pParam,&Info);
}

// プラグインのアイコンを登録する
inline bool MsgRegisterPluginIconFromResource(PluginParam *pParam,HINSTANCE hinst,LPCTSTR pszName) {
	HBITMAP hbmIcon=(HBITMAP)::LoadImage(hinst,pszName,IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
	bool fResult=MsgRegisterPluginIcon(pParam,hbmIcon);
	::DeleteObject(hbmIcon);
	return fResult;
}

// コマンドアイコンの描画情報
// EVENT_DRAWCOMMANDICON で渡されます。
struct DrawCommandIconInfo {
	int ID;				// コマンドの識別子
	WORD Flags;			// 各種フラグ(現在は常に0)
	WORD State;			// 状態フラグ(COMMAND_ICON_STATE_*)
	LPCWSTR pszStyle;	// スタイル名
	HDC hdc;			// 描画先DC
	RECT DrawRect;		// 描画先領域
	COLORREF Color;		// 色
	BYTE Opacity;		// 不透明度
	BYTE Reserved[3];	// 予約領域
};

// コマンドアイコンの状態フラグ
enum {
	COMMAND_ICON_STATE_DISABLED	=0x0001U,	// 無効状態
	COMMAND_ICON_STATE_CHECKED	=0x0002U,	// チェック状態
	COMMAND_ICON_STATE_HOT		=0x0004U	// フォーカスが当たっている
};

// ステータス項目の情報
struct StatusItemInfo {
	DWORD Size;			// 構造体のサイズ
	DWORD Flags;		// 各種フラグ(STATUS_ITEM_FLAG_*)
	DWORD Style;		// スタイルフラグ(STATUS_ITEM_STYLE_*)
	int ID;				// 識別子
	LPCWSTR pszIDText;	// 識別子文字列
	LPCWSTR pszName;	// 名前
	int MinWidth;		// 最小の幅
	int MaxWidth;		// 最大の幅(-1で制限なし)
	int DefaultWidth;	// デフォルト幅(正数ではピクセル単位、負数ではフォントの高さの-1/1000単位)
	int MinHeight;		// 最小の高さ
};

// ステータス項目のフラグ
enum {
	STATUS_ITEM_FLAG_TIMERUPDATE	=0x00000001U	// 定期的に更新する(STATUS_ITEM_EVENT_UPDATETIMER が呼ばれる)
};

// ステータス項目のスタイルフラグ
enum {
	STATUS_ITEM_STYLE_VARIABLEWIDTH	=0x00000001U,	// 可変幅
	STATUS_ITEM_STYLE_FULLROW		=0x00000002U,	// 一行表示(表示領域が足りなければ一行表示にならないこともある)
	STATUS_ITEM_STYLE_FORCEFULLROW	=0x00000004U	// 強制一行表示(常に一行表示になり、常に表示される)
};

// ステータス項目の幅をフォントサイズから求める
inline int StatusItemWidthByFontSize(int Size) { return Size*-1000; }

// ステータス項目を登録する
inline bool MsgRegisterStatusItem(PluginParam *pParam,const StatusItemInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_REGISTERSTATUSITEM,(LPARAM)pInfo,0)!=FALSE;
}

// ステータス項目の状態フラグ
enum {
	STATUS_ITEM_STATE_VISIBLE		=0x00000001U,	// 可視
	STATUS_ITEM_STATE_HOT			=0x00000002U	// フォーカスが当たっている
};

// ステータス項目の設定の情報
struct StatusItemSetInfo {
	DWORD Size;			// 構造体のサイズ
	DWORD Mask;			// 設定する情報のマスク(STATUS_ITEM_SET_INFO_MASK_*)
	int ID;				// 項目の識別子
	DWORD StateMask;	// 状態フラグのマスク(STATUS_ITEM_STATE_*)
	DWORD State;		// 状態フラグ(STATUS_ITEM_STATE_*)
	DWORD StyleMask;	// スタイルフラグのマスク(STATUS_ITEM_STYLE_*)
	DWORD Style;		// スタイルフラグ(STATUS_ITEM_STYLE_*)
};

// ステータス項目の設定
enum {
	STATUS_ITEM_SET_INFO_MASK_STATE	=0x00000001U,	// StateMask / State を設定
	STATUS_ITEM_SET_INFO_MASK_STYLE	=0x00000002U	// StyleMask / Style を設定
};

// ステータス項目を設定する
// StatusItemSetInfo の Size に構造体のサイズを、Mask に設定したい情報を、
// ID に設定したい項目の識別子を指定して呼び出します。
/*
	// 例
	// 項目の表示状態を設定する
	void ShowStatusItem(PluginParam *pParam, int ID, bool fVisible)
	{
		StatusItemSetInfo Info;
		Info.Size = sizeof(Info);
		Info.Mask = STATUS_ITEM_SET_INFO_MASK_STATE;
		Info.ID = ID;
		Info.StateMask = STATUS_ITEM_STATE_VISIBLE;
		Info.State = fVisible ? STATUS_ITEM_STATE_VISIBLE : 0;
		MsgSetStatusItem(pParam, &Info);
	}
*/
inline bool MsgSetStatusItem(PluginParam *pParam,const StatusItemSetInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_SETSTATUSITEM,(LPARAM)pInfo,0)!=FALSE;
}

// ステータス項目の情報取得
struct StatusItemGetInfo {
	DWORD Size;			// 構造体のサイズ
	DWORD Mask;			// 取得する情報のマスク(STATUS_ITEM_GET_INFO_MASK_*)
	int ID;				// 項目の識別子
	DWORD State;		// 状態フラグ(STATUS_ITEM_STATE_*)
	HWND hwnd;			// ウィンドウハンドル
	RECT ItemRect;		// 項目の領域
	RECT ContentRect;	// 項目の余白を除いた領域
	DWORD Style;		// スタイルフラグ(STATUS_ITEM_STYLE_*)
};

// ステータス項目の情報取得
enum {
	STATUS_ITEM_GET_INFO_MASK_STATE			=0x00000001U,	// State を取得
	STATUS_ITEM_GET_INFO_MASK_HWND			=0x00000002U,	// hwnd を取得
	STATUS_ITEM_GET_INFO_MASK_ITEMRECT		=0x00000004U,	// ItemRect を取得
	STATUS_ITEM_GET_INFO_MASK_CONTENTRECT	=0x00000008U,	// ContentRect を取得
	STATUS_ITEM_GET_INFO_MASK_STYLE			=0x00000010U	// Style を取得
};

// ステータス項目の情報を取得する
// StatusItemGetInfo の Size に構造体のサイズを、Mask に取得したい情報を、
// ID に取得したい項目の識別子を指定して呼び出します。
/*
	// 例
	// 項目の表示状態を取得する
	bool IsStatusItemVisible(PluginParam *pParam, int ID)
	{
		StatusItemGetInfo Info;
		Info.Size = sizeof(Info);
		Info.Mask = STATUS_ITEM_GET_INFO_MASK_STATE;
		Info.ID = ID;
		return MsgGetStatusItem(pParam, &Info)
			&& (Info.State & STATUS_ITEM_STATE_VISIBLE) != 0;
	}
*/
inline bool MsgGetStatusItemInfo(PluginParam *pParam,StatusItemGetInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_GETSTATUSITEMINFO,(LPARAM)pInfo,0)!=FALSE;
}

// ステータス項目の通知
enum {
	STATUS_ITEM_NOTIFY_REDRAW	// 再描画する
};

// ステータス項目の通知を行う
inline bool MsgStatusItemNotify(PluginParam *pParam,int ID,UINT Type) {
	return (*pParam->Callback)(pParam,MESSAGE_STATUSITEMNOTIFY,ID,Type)!=FALSE;
}

// ステータス項目の描画情報
// EVENT_STATUSITEM_DRAW で渡されます。
// 描画はダブルバッファリングによって行われるため、
// ItemRect / DrawRect で渡される位置は表示上の位置とは異なっています。
struct StatusItemDrawInfo {
	int ID;				// 項目の識別子
	WORD Flags;			// 各種フラグ(STATUS_ITEM_DRAW_FLAG_*)
	WORD State;			// 状態フラグ(STATUS_ITEM_DRAW_STATE_*)
	LPCWSTR pszStyle;	// スタイル
	HDC hdc;			// 描画先DC
	RECT ItemRect;		// 項目の領域
	RECT DrawRect;		// 描画する領域
	COLORREF Color;		// 色
};

// ステータス項目描画フラグ
enum {
	STATUS_ITEM_DRAW_FLAG_PREVIEW	=0x0001U	// プレビュー(設定ダイアログでの表示)
};

// ステータス項目描画状態フラグ
enum {
	STATUS_ITEM_DRAW_STATE_HOT		=0x0001U	// フォーカスが当たっている
};

// ステータス項目の通知情報
// EVENT_STATUSITEM_NOTIFY で渡されます。
struct StatusItemEventInfo {
	int ID;				// 項目の識別子
	UINT Event;			// イベントの種類(STATUS_ITEM_EVENT_*)
	LPARAM Param;		// パラメータ
};

// ステータス項目のイベント
enum {
	STATUS_ITEM_EVENT_CREATED=1,			// 項目が作成された
	STATUS_ITEM_EVENT_VISIBILITYCHANGED,	// 項目の表示状態が変わった
	STATUS_ITEM_EVENT_ENTER,				// フォーカスが当たった
	STATUS_ITEM_EVENT_LEAVE,				// フォーカスが離れた
	STATUS_ITEM_EVENT_SIZECHANGED,			// 項目の大きさが変わった
	STATUS_ITEM_EVENT_UPDATETIMER			// 更新タイマー
};

// ステータス項目のマウスイベント情報
// EVENT_STATUSITEM_MOUSE で渡されます。
struct StatusItemMouseEventInfo {
	int ID;				// 項目の識別子
	UINT Action;		// マウス操作の種類(STATUS_ITEM_MOUSE_ACTION_*)
	HWND hwnd;			// ウィンドウハンドル
	POINT CursorPos;	// カーソル位置(クライアント座標)
	RECT ItemRect;		// 項目の領域
	RECT ContentRect;	// 項目の余白を除いた領域
	int WheelDelta;		// ホイール移動量
};

// ステータス項目のマウス操作の種類
// DOWN か DOUBLECLICK が送られた際に SetCapture() でマウスキャプチャが行われると、
// キャプチャが解除された時に CAPTURERELEASE が送られます。
enum {
	STATUS_ITEM_MOUSE_ACTION_LDOWN=1,		// 左ボタンが押された
	STATUS_ITEM_MOUSE_ACTION_LUP,			// 左ボタンが離された
	STATUS_ITEM_MOUSE_ACTION_LDOUBLECLICK,	// 左ダブルクリック
	STATUS_ITEM_MOUSE_ACTION_RDOWN,			// 右ボタンが押された
	STATUS_ITEM_MOUSE_ACTION_RUP,			// 右ボタンが離された
	STATUS_ITEM_MOUSE_ACTION_RDOUBLECLICK,	// 右ダブルクリック
	STATUS_ITEM_MOUSE_ACTION_MDOWN,			// 中央ボタンが押された
	STATUS_ITEM_MOUSE_ACTION_MUP,			// 中央ボタンが離された
	STATUS_ITEM_MOUSE_ACTION_MDOUBLECLICK,	// 中央ダブルクリック
	STATUS_ITEM_MOUSE_ACTION_MOVE,			// カーソル移動
	STATUS_ITEM_MOUSE_ACTION_WHEEL,			// ホイール
	STATUS_ITEM_MOUSE_ACTION_HORZWHEEL,		// 横ホイール
	STATUS_ITEM_MOUSE_ACTION_CAPTURERELEASE	// キャプチャが解除された
};

// TSプロセッサのインターフェースは TVTestInterface.h で宣言されています。
#ifndef TVTEST_INTERFACE_H
namespace Interface {
	struct ITSProcessor;
}
#endif

// TSプロセッサの接続位置
// 詳細は TVTestInterface.h を参照してください。
enum {
	TS_PROCESSOR_CONNECT_POSITION_SOURCE,			// ソース(チューナー等からストリームが入力された後)
	TS_PROCESSOR_CONNECT_POSITION_PREPROCESSING,	// 前処理(TSを解析する前)
	TS_PROCESSOR_CONNECT_POSITION_POSTPROCESSING,	// 後処理(TSを解析した後)
	TS_PROCESSOR_CONNECT_POSITION_VIEWER,			// ビューア(再生の前)
	TS_PROCESSOR_CONNECT_POSITION_RECORDER			// レコーダ(ストリーム書き出しの前)
};

// TSプロセッサの情報
struct TSProcessorInfo {
	DWORD Size;								// 構造体のサイズ
	DWORD Flags;							// 各種フラグ(現在は常に0)
	Interface::ITSProcessor *pTSProcessor;	// 接続する ITSProcessor
	DWORD ConnectPosition;					// 接続位置(TS_PROCESSOR_CONNECT_POSITION_*)
};

// TSプロセッサを登録する
inline bool MsgRegisterTSProcessor(PluginParam *pParam,const TSProcessorInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_REGISTERTSPROCESSOR,(LPARAM)pInfo,0)!=FALSE;
}

// パネル項目のスタイル
enum {
	PANEL_ITEM_STYLE_NEEDFOCUS	=0x0001U	// キーボードフォーカスを受け取る
};

// パネル項目の状態
enum {
	PANEL_ITEM_STATE_ENABLED	=0x0001U,	// 有効(タブに表示されている)
	PANEL_ITEM_STATE_ACTIVE		=0x0002U	// アクティブ
};

// パネル項目の情報
struct PanelItemInfo {
	DWORD Size;			// 構造体のサイズ
	DWORD Flags;		// 各種フラグ(現在は常に0)
	DWORD Style;		// スタイルフラグ(PANEL_ITEM_STYLE_* の組み合わせ)
	int ID;				// 識別子
	LPCWSTR pszIDText;	// 識別子文字列
	LPCWSTR pszTitle;	// タイトル
	HBITMAP hbmIcon;	// アイコンのビットマップ
};

// パネル項目を登録する
inline bool MsgRegisterPanelItem(PluginParam *pParam,const PanelItemInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_REGISTERPANELITEM,(LPARAM)pInfo,0)!=FALSE;
}

// パネル項目の設定の情報
struct PanelItemSetInfo {
	DWORD Size;			// 構造体のサイズ
	DWORD Mask;			// 設定する情報のマスク(PANEL_ITEM_SET_INFO_MASK_* の組み合わせ)
	int ID;				// 項目の識別子
	DWORD StateMask;	// 状態フラグのマスク(PANEL_ITEM_STATE_* の組み合わせ)
	DWORD State;		// 状態フラグ(PANEL_ITEM_STATE_* の組み合わせ)
	DWORD StyleMask;	// スタイルフラグのマスク(PANEL_ITEM_STYLE_* の組み合わせ)
	DWORD Style;		// スタイルフラグ(PANEL_ITEM_STYLE_* の組み合わせ)
};

// パネル項目の設定
enum {
	PANEL_ITEM_SET_INFO_MASK_STATE	=0x00000001U,	// StateMask / State を設定
	PANEL_ITEM_SET_INFO_MASK_STYLE	=0x00000002U	// StyleMask / Style を設定
};

// ステータス項目を設定する
// PanelItemSetInfo の Size に構造体のサイズを、Mask に設定したい情報を、
// ID に設定したい項目の識別子を指定して呼び出します。
/*
	// 例
	// 項目の有効状態を設定する
	void EnablePanelItem(PluginParam *pParam, int ID, bool fEnable)
	{
		PanelItemSetInfo Info;
		Info.Size = sizeof(PanelItemSetInfo);
		Info.Mask = PANEL_ITEM_SET_INFO_MASK_STATE;
		Info.ID = ID;
		Info.StateMask = PANEL_ITEM_STATE_ENABLED;
		Info.State = fEnable ? PANEL_ITEM_STATE_ENABLED : 0;
		MsgSetPanelItem(pParam, &Info);
	}
*/
inline bool MsgSetPanelItem(PluginParam *pParam,const PanelItemSetInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_SETPANELITEM,(LPARAM)pInfo,0)!=FALSE;
}

// パネル項目の情報取得
struct PanelItemGetInfo {
	DWORD Size;			// 構造体のサイズ
	DWORD Mask;			// 取得する情報のマスク(PANEL_ITEM_GET_INFO_MASK_* の組み合わせ)
	int ID;				// 項目の識別子
	DWORD State;		// 項目の状態フラグ(PANEL_ITEM_STATE_* の組み合わせ)
	HWND hwndParent;	// 親ウィンドウのハンドル
	HWND hwndItem;		// 項目のウィンドウハンドル
	DWORD Style;		// スタイルフラグ(PANEL_ITEM_STYLE_* の組み合わせ)
};

// パネル項目の情報取得マスク
enum {
	PANEL_ITEM_GET_INFO_MASK_STATE		=0x0001U,	// State を取得
	PANEL_ITEM_GET_INFO_MASK_HWNDPARENT	=0x0002U,	// hwndParent を取得
	PANEL_ITEM_GET_INFO_MASK_HWNDITEM	=0x0004U,	// hwndItem を取得
	PANEL_ITEM_GET_INFO_MASK_STYLE		=0x0008U	// Style を取得
};

// パネル項目の情報を取得する
// PanelItemGetInfo の Size に構造体のサイズを、Mask に取得したい情報を、
// ID に取得したい項目の識別子を指定して呼び出します。
inline bool MsgGetPanelItemInfo(PluginParam *pParam,PanelItemGetInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_GETPANELITEMINFO,(LPARAM)pInfo,0)!=FALSE;
}

// パネル項目の通知情報
// EVENT_PANELITEM_NOTIFY で渡されます
struct PanelItemEventInfo {
	int ID;				// 項目の識別子
	UINT Event;			// イベントの種類(PANEL_ITEM_EVENT_* のいずれか)
};

// パネル項目のイベント
enum {
	PANEL_ITEM_EVENT_CREATE=1,		// 項目を作成する
	PANEL_ITEM_EVENT_ACTIVATE,		// 項目がアクティブになる
	PANEL_ITEM_EVENT_DEACTIVATE,	// 項目が非アクティブになる
	PANEL_ITEM_EVENT_ENABLE,		// 項目が有効になる
	PANEL_ITEM_EVENT_DISABLE		// 項目が無効になる
};

// パネル項目作成イベントの情報
// PANEL_ITEM_EVENT_CREATE で渡されます。
// hwndItem に作成したウィンドウのハンドルを返します。
struct PanelItemCreateEventInfo {
	PanelItemEventInfo EventInfo;
	RECT ItemRect;
	HWND hwndParent;
	HWND hwndItem;
};

// チャンネル選択の情報
struct ChannelSelectInfo {
	DWORD Size;				// 構造体のサイズ
	DWORD Flags;			// 各種フラグ(CHANNEL_SELECT_FLAG_* の組み合わせ)
	LPCWSTR pszTuner;		// チューナー名(NULL で指定なし)
	int Space;				// チューニング空間(-1 で指定なし)
	int Channel;			// チャンネル(-1 で指定なし)
	WORD NetworkID;			// ネットワークID(0 で指定なし)
	WORD TransportStreamID;	// トランスポートストリームID(0 で指定なし)
	WORD ServiceID;			// サービスID(0 で指定なし)
};

// チャンネル選択のフラグ
enum {
	CHANNEL_SELECT_FLAG_STRICTSERVICE	=0x0001U	// ServiceID の指定を厳密に扱う
};

// チャンネルを選択する
inline bool MsgSelectChannel(PluginParam *pParam,const ChannelSelectInfo *pInfo) {
	return (*pParam->Callback)(pParam,MESSAGE_SELECTCHANNEL,(LPARAM)pInfo,0)!=FALSE;
}

// お気に入り項目の種類
enum {
	FAVORITE_ITEM_TYPE_FOLDER,	// フォルダ
	FAVORITE_ITEM_TYPE_CHANNEL	// チャンネル
};

// お気に入り項目の情報
struct FavoriteItemInfo {
	DWORD Type;						// 種類(FAVORITE_ITEM_TYPE_* のいずれか)
	DWORD Flags;					// 各種フラグ(現在は常に0)
	LPCWSTR pszName;				// 名前
	union {
		// Type == FAVORITE_ITEM_TYPE_FOLDER の場合
		struct {
			DWORD Flags;								// 各種フラグ(現在は常に0)
			DWORD ItemCount;							// 子項目数
			const struct FavoriteItemInfo *ItemList;	// 子項目のリスト
		} Folder;
		// Type == FAVORITE_ITEM_TYPE_CHANNEL の場合
		struct {
			DWORD Flags;			// 各種フラグ(FAVORITE_CHANNEL_FLAG_*)
			int Space;				// チューニング空間
			int Channel;			// チャンネルインデックス
			int ChannelNo;			// リモコン番号
			WORD NetworkID;			// ネットワークID
			WORD TransportStreamID;	// トランスポートストリームID
			WORD ServiceID;			// サービスID
			WORD Reserved;			// 予約(現在は常に0)
			LPCWSTR pszTuner;		// チューナー名
		} Channel;
	};
};

// お気に入りチャンネルのフラグ
enum {
	FAVORITE_CHANNEL_FLAG_FORCETUNERCHANGE	=0x0001U	// チューナー指定を強制
};

// お気に入りリスト
struct FavoriteList {
	DWORD Size;					// 構造体のサイズ
	DWORD ItemCount;			// お気に入り項目数
	FavoriteItemInfo *ItemList;	// お気に入り項目のリスト
};

// お気に入りリストを取得する
// FavoriteList の Size メンバに構造体のサイズを設定して呼び出します。
// 取得したリストは MsgFreeFavoriteList で解放します。
inline bool MsgGetFavoriteList(PluginParam *pParam,FavoriteList *pList) {
	return (*pParam->Callback)(pParam,MESSAGE_GETFAVORITELIST,(LPARAM)pList,0)!=FALSE;
}

// お気に入りリストを解放する
// MsgGetFavoriteList で取得したリストを解放します。
inline void MsgFreeFavoriteList(PluginParam *pParam,FavoriteList *pList) {
	(*pParam->Callback)(pParam,MESSAGE_FREEFAVORITELIST,(LPARAM)pList,0);
}

// ワンセグモードを取得する
inline bool MsgGet1SegMode(PluginParam *pParam) {
	return (*pParam->Callback)(pParam,MESSAGE_GET1SEGMODE,0,0)!=FALSE;
}

// ワンセグモードを設定する
inline bool MsgSet1SegMode(PluginParam *pParam,bool f1SegMode) {
	return (*pParam->Callback)(pParam,MESSAGE_SET1SEGMODE,f1SegMode,0)!=FALSE;
}

#endif	// TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,14)


/*
	TVTest アプリケーションクラス

	TVTest の各種機能を呼び出すためのクラスです。
	ただのラッパーなので使わなくてもいいです。
	TVTInitialize 関数が呼ばれた時に、引数の PluginParam 構造体へのポインタを
	コンストラクタに渡してインスタンスを生成します。
*/
class CTVTestApp
{
protected:
	PluginParam *m_pParam;
public:
	CTVTestApp(PluginParam *pParam) : m_pParam(pParam) {}
	virtual ~CTVTestApp() {}
	HWND GetAppWindow() {
		return m_pParam->hwndApp;
	}
	DWORD GetVersion() {
		return MsgGetVersion(m_pParam);
	}
	bool QueryMessage(UINT Message) {
		return MsgQueryMessage(m_pParam,Message);
	}
	void *MemoryReAlloc(void *pData,DWORD Size) {
		return MsgMemoryReAlloc(m_pParam,pData,Size);
	}
	void *MemoryAlloc(DWORD Size) {
		return MsgMemoryAlloc(m_pParam,Size);
	}
	void MemoryFree(void *pData) {
		MsgMemoryFree(m_pParam,pData);
	}
	bool SetEventCallback(EventCallbackFunc Callback,void *pClientData=NULL) {
		return MsgSetEventCallback(m_pParam,Callback,pClientData);
	}
	bool GetCurrentChannelInfo(ChannelInfo *pInfo) {
		pInfo->Size=sizeof(ChannelInfo);
		return MsgGetCurrentChannelInfo(m_pParam,pInfo);
	}
#if TVTEST_PLUGIN_VERSION<TVTEST_PLUGIN_VERSION_(0,0,8)
	bool SetChannel(int Space,int Channel) {
		return MsgSetChannel(m_pParam,Space,Channel);
	}
#else
	bool SetChannel(int Space,int Channel,WORD ServiceID=0) {
		return MsgSetChannel(m_pParam,Space,Channel,ServiceID);
	}
#endif
	int GetService(int *pNumServices=NULL) {
		return MsgGetService(m_pParam,pNumServices);
	}
	bool SetService(int Service,bool fByID=false) {
		return MsgSetService(m_pParam,Service,fByID);
	}
	int GetTuningSpaceName(int Index,LPWSTR pszName,int MaxLength) {
		return MsgGetTuningSpaceName(m_pParam,Index,pszName,MaxLength);
	}
	bool GetChannelInfo(int Space,int Index,ChannelInfo *pInfo) {
		pInfo->Size=sizeof(ChannelInfo);
		return MsgGetChannelInfo(m_pParam,Space,Index,pInfo);
	}
	bool GetServiceInfo(int Index,ServiceInfo *pInfo) {
		pInfo->Size=sizeof(ServiceInfo);
		return MsgGetServiceInfo(m_pParam,Index,pInfo);
	}
	int GetDriverName(LPWSTR pszName,int MaxLength) {
		return MsgGetDriverName(m_pParam,pszName,MaxLength);
	}
	bool SetDriverName(LPCWSTR pszName) {
		return MsgSetDriverName(m_pParam,pszName);
	}
	bool StartRecord(RecordInfo *pInfo=NULL) {
		if (pInfo!=NULL)
			pInfo->Size=sizeof(RecordInfo);
		return MsgStartRecord(m_pParam,pInfo);
	}
	bool StopRecord() {
		return MsgStopRecord(m_pParam);
	}
	bool PauseRecord(bool fPause=true) {
		return MsgPauseRecord(m_pParam,fPause);
	}
	bool GetRecord(RecordInfo *pInfo) {
		pInfo->Size=sizeof(RecordInfo);
		return MsgGetRecord(m_pParam,pInfo);
	}
	bool ModifyRecord(RecordInfo *pInfo) {
		pInfo->Size=sizeof(RecordInfo);
		return MsgModifyRecord(m_pParam,pInfo);
	}
	int GetZoom() {
		return MsgGetZoom(m_pParam);
	}
	int SetZoom(int Num,int Denom=100) {
		return MsgSetZoom(m_pParam,Num,Denom);
	}
	bool GetPanScan(PanScanInfo *pInfo) {
		pInfo->Size=sizeof(PanScanInfo);
		return MsgGetPanScan(m_pParam,pInfo);
	}
	bool SetPanScan(PanScanInfo *pInfo) {
		pInfo->Size=sizeof(PanScanInfo);
		return MsgSetPanScan(m_pParam,pInfo);
	}
	bool GetStatus(StatusInfo *pInfo) {
		pInfo->Size=sizeof(StatusInfo);
		return MsgGetStatus(m_pParam,pInfo);
	}
	bool GetRecordStatus(RecordStatusInfo *pInfo) {
#if TVTEST_PLUGIN_VERSION<TVTEST_PLUGIN_VERSION_(0,0,10)
		pInfo->Size=sizeof(RecordStatusInfo);
#else
		if (pInfo->pszFileName!=NULL)
			pInfo->Size=sizeof(RecordStatusInfo);
		else
			pInfo->Size=RECORDSTATUSINFO_SIZE_V1;
#endif
		return MsgGetRecordStatus(m_pParam,pInfo);
	}
	bool GetVideoInfo(VideoInfo *pInfo) {
		pInfo->Size=sizeof(VideoInfo);
		return MsgGetVideoInfo(m_pParam,pInfo);
	}
	int GetVolume() {
		return MsgGetVolume(m_pParam);
	}
	bool SetVolume(int Volume) {
		return MsgSetVolume(m_pParam,Volume);
	}
	bool GetMute() {
		return MsgGetMute(m_pParam);
	}
	bool SetMute(bool fMute) {
		return MsgSetMute(m_pParam,fMute);
	}
	int GetStereoMode() {
		return MsgGetStereoMode(m_pParam);
	}
	bool SetStereoMode(int StereoMode) {
		return MsgSetStereoMode(m_pParam,StereoMode);
	}
	bool GetFullscreen() {
		return MsgGetFullscreen(m_pParam);
	}
	bool SetFullscreen(bool fFullscreen) {
		return MsgSetFullscreen(m_pParam,fFullscreen);
	}
	bool GetPreview() {
		return MsgGetPreview(m_pParam);
	}
	bool SetPreview(bool fPreview) {
		return MsgSetPreview(m_pParam,fPreview);
	}
	bool GetStandby() {
		return MsgGetStandby(m_pParam);
	}
	bool SetStandby(bool fStandby) {
		return MsgSetStandby(m_pParam,fStandby);
	}
	bool GetAlwaysOnTop() {
		return MsgGetAlwaysOnTop(m_pParam);
	}
	bool SetAlwaysOnTop(bool fAlwaysOnTop) {
		return MsgSetAlwaysOnTop(m_pParam,fAlwaysOnTop);
	}
	void *CaptureImage(DWORD Flags=0) {
		return MsgCaptureImage(m_pParam,Flags);
	}
	bool SaveImage() {
		return MsgSaveImage(m_pParam);
	}
#if TVTEST_PLUGIN_VERSION<TVTEST_PLUGIN_VERSION_(0,0,9)
	bool Reset() {
		return MsgReset(m_pParam);
	}
#else
	bool Reset(DWORD Flags=RESET_ALL) {
		return MsgReset(m_pParam,Flags);
	}
#endif
	bool Close(DWORD Flags=0) {
		return MsgClose(m_pParam,Flags);
	}
	bool SetStreamCallback(DWORD Flags,StreamCallbackFunc Callback,void *pClientData=NULL) {
		return MsgSetStreamCallback(m_pParam,Flags,Callback,pClientData);
	}
	bool EnablePlugin(bool fEnable) {
		return MsgEnablePlugin(m_pParam,fEnable);
	}
	COLORREF GetColor(LPCWSTR pszColor) {
		return MsgGetColor(m_pParam,pszColor);
	}
	bool DecodeARIBString(const void *pSrcData,DWORD SrcLength,
						  LPWSTR pszDest,DWORD DestLength) {
		return MsgDecodeARIBString(m_pParam,pSrcData,SrcLength,pszDest,DestLength);
	}
	bool GetCurrentProgramInfo(ProgramInfo *pInfo,bool fNext=false) {
		pInfo->Size=sizeof(ProgramInfo);
		return MsgGetCurrentProgramInfo(m_pParam,pInfo,fNext);
	}
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,1)
	bool QueryEvent(UINT Event) {
		return MsgQueryEvent(m_pParam,Event);
	}
	int GetTuningSpace(int *pNumSpaces=NULL) {
		return MsgGetTuningSpace(m_pParam,pNumSpaces);
	}
	bool GetTuningSpaceInfo(int Index,TuningSpaceInfo *pInfo) {
		pInfo->Size=sizeof(TuningSpaceInfo);
		return MsgGetTuningSpaceInfo(m_pParam,Index,pInfo);
	}
	bool SetNextChannel(bool fNext=true) {
		return MsgSetNextChannel(m_pParam,fNext);
	}
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,2)
	int GetAudioStream() {
		return MsgGetAudioStream(m_pParam);
	}
	bool SetAudioStream(int Index) {
		return MsgSetAudioStream(m_pParam,Index);
	}
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,3)
	bool IsPluginEnabled() {
		return MsgIsPluginEnabled(m_pParam);
	}
	bool RegisterCommand(int ID,LPCWSTR pszText,LPCWSTR pszName) {
		return MsgRegisterCommand(m_pParam,ID,pszText,pszName);
	}
	bool RegisterCommand(const CommandInfo *pCommandList,int NumCommands) {
		return MsgRegisterCommand(m_pParam,pCommandList,NumCommands);
	}
	bool AddLog(LPCWSTR pszText) {
		return MsgAddLog(m_pParam,pszText);
	}
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,14)
	bool AddLog(LPCWSTR pszText,int Type) {
		return MsgAddLog(m_pParam,pszText,Type);
	}
#endif
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,5)
	bool ResetStatus() {
		return MsgResetStatus(m_pParam);
	}
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,6)
	bool SetAudioCallback(AudioCallbackFunc pCallback,void *pClientData=NULL) {
		return MsgSetAudioCallback(m_pParam,pCallback,pClientData);
	}
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,7)
	bool DoCommand(LPCWSTR pszCommand) {
		return MsgDoCommand(m_pParam,pszCommand);
	}
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,8)
	bool GetBCasInfo(BCasInfo *pInfo) {
		pInfo->Size=sizeof(BCasInfo);
		return MsgGetBCasInfo(m_pParam,pInfo);
	}
	bool SendBCasCommand(BCasCommandInfo *pInfo) {
		return MsgSendBCasCommand(m_pParam,pInfo);
	}
	bool SendBCasCommand(const BYTE *pSendData,DWORD SendSize,BYTE *pReceiveData,DWORD *pReceiveSize) {
		return MsgSendBCasCommand(m_pParam,pSendData,SendSize,pReceiveData,pReceiveSize);
	}
	bool GetHostInfo(HostInfo *pInfo) {
		pInfo->Size=sizeof(HostInfo);
		return MsgGetHostInfo(m_pParam,pInfo);
	}
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,9)
	bool GetSetting(SettingInfo *pInfo) {
		return MsgGetSetting(m_pParam,pInfo);
	}
	bool GetSetting(LPCWSTR pszName,int *pValue) {
		return MsgGetSetting(m_pParam,pszName,pValue);
	}
	bool GetSetting(LPCWSTR pszName,unsigned int *pValue) {
		return MsgGetSetting(m_pParam,pszName,pValue);
	}
	DWORD GetSetting(LPCWSTR pszName,LPWSTR pszString,DWORD MaxLength) {
		return MsgGetSetting(m_pParam,pszName,pszString,MaxLength);
	}
	bool GetSetting(LPCWSTR pszName,LOGFONTW *pFont) {
		return MsgGetSetting(m_pParam,pszName,pFont);
	}
	int GetDriverFullPathName(LPWSTR pszPath,int MaxLength) {
		return MsgGetDriverFullPathName(m_pParam,pszPath,MaxLength);
	}
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,10)
	HBITMAP GetLogo(WORD NetworkID,WORD ServiceID,BYTE LogoType) {
		return MsgGetLogo(m_pParam,NetworkID,ServiceID,LogoType);
	}
	UINT GetAvailableLogoType(WORD NetworkID,WORD ServiceID) {
		return MsgGetAvailableLogoType(m_pParam,NetworkID,ServiceID);
	}
	bool RelayRecord(LPCWSTR pszFileName) {
		return MsgRelayRecord(m_pParam,pszFileName);
	}
	bool GetSilentMode() {
		return MsgGetSilentMode(m_pParam);
	}
	bool SetSilentMode(bool fSilent) {
		return MsgSetSilentMode(m_pParam,fSilent);
	}
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,11)
	bool SetWindowMessageCallback(WindowMessageCallbackFunc Callback,void *pClientData=NULL) {
		return MsgSetWindowMessageCallback(m_pParam,Callback,pClientData);
	}
	bool RegisterController(ControllerInfo *pInfo) {
		pInfo->Size=sizeof(ControllerInfo);
		return MsgRegisterController(m_pParam,pInfo);
	}
	bool OnControllerButtonDown(LPCWSTR pszName,int Button) {
		return MsgOnControllerButtonDown(m_pParam,pszName,Button);
	}
	bool GetControllerSettings(LPCWSTR pszName,ControllerSettings *pSettings) {
		return MsgGetControllerSettings(m_pParam,pszName,pSettings);
	}
	bool IsControllerActiveOnly(LPCWSTR pszName) {
		return MsgIsControllerActiveOnly(m_pParam,pszName);
	}
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,12)
	EpgEventInfo *GetEpgEventInfo(const EpgEventQueryInfo *pInfo) {
		return MsgGetEpgEventInfo(m_pParam,pInfo);
	}
	void FreeEpgEventInfo(EpgEventInfo *pEventInfo) {
		MsgFreeEpgEventInfo(m_pParam,pEventInfo);
	}
	bool GetEpgEventList(EpgEventList *pList) {
		return MsgGetEpgEventList(m_pParam,pList);
	}
	void FreeEpgEventList(EpgEventList *pList) {
		MsgFreeEpgEventList(m_pParam,pList);
	}
	int EnumDriver(int Index,LPWSTR pszFileName,int MaxLength) {
		return MsgEnumDriver(m_pParam,Index,pszFileName,MaxLength);
	}
	bool GetDriverTuningSpaceList(LPCWSTR pszDriverName,DriverTuningSpaceList *pList) {
		return MsgGetDriverTuningSpaceList(m_pParam,pszDriverName,pList);
	}
	void FreeDriverTuningSpaceList(DriverTuningSpaceList *pList) {
		MsgFreeDriverTuningSpaceList(m_pParam,pList);
	}
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,13)
	bool EnableProgramGuideEvent(UINT EventFlags) {
		return MsgEnableProgramGuideEvent(m_pParam,EventFlags);
	}
	bool RegisterProgramGuideCommand(const ProgramGuideCommandInfo *pCommandList,int NumCommands=1) {
		return MsgRegisterProgramGuideCommand(m_pParam,pCommandList,NumCommands);
	}
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,14)
	bool GetStyleValue(StyleValueInfo *pInfo) {
		pInfo->Size=sizeof(StyleValueInfo);
		return MsgGetStyleValue(m_pParam,pInfo);
	}
	bool GetStyleValue(LPCWSTR pszName,int Unit,int *pValue) {
		return MsgGetStyleValue(m_pParam,pszName,Unit,pValue);
	}
	bool GetStyleValue(LPCWSTR pszName,int *pValue) {
		return MsgGetStyleValue(m_pParam,pszName,pValue);
	}
	bool GetStyleValuePixels(LPCWSTR pszName,int *pValue) {
		return MsgGetStyleValuePixels(m_pParam,pszName,pValue);
	}
	bool ThemeDrawBackground(ThemeDrawBackgroundInfo *pInfo) {
		pInfo->Size=sizeof(ThemeDrawBackgroundInfo);
		return MsgThemeDrawBackground(m_pParam,pInfo);
	}
	bool ThemeDrawBackground(LPCWSTR pszStyle,HDC hdc,const RECT &DrawRect) {
		return MsgThemeDrawBackground(m_pParam,pszStyle,hdc,DrawRect);
	}
	bool ThemeDrawText(ThemeDrawTextInfo *pInfo) {
		pInfo->Size=sizeof(ThemeDrawTextInfo);
		return MsgThemeDrawText(m_pParam,pInfo);
	}
	bool ThemeDrawText(LPCWSTR pszStyle,HDC hdc,LPCWSTR pszText,const RECT &DrawRect,
					   UINT DrawFlags,COLORREF Color=CLR_INVALID) {
		return MsgThemeDrawText(m_pParam,pszStyle,hdc,pszText,DrawRect,DrawFlags,Color);
	}
	bool ThemeDrawIcon(ThemeDrawIconInfo *pInfo) {
		pInfo->Size=sizeof(ThemeDrawIconInfo);
		return MsgThemeDrawIcon(m_pParam,pInfo);
	}
	bool ThemeDrawIcon(LPCWSTR pszStyle,
			HDC hdc,int DstX,int DstY,int DstWidth,int DstHeight,
			HBITMAP hbm,int SrcX,int SrcY,int SrcWidth,int SrcHeight,
			COLORREF Color=CLR_INVALID,BYTE Opacity=255) {
		return MsgThemeDrawIcon(m_pParam,pszStyle,hdc,DstX,DstY,DstWidth,DstHeight,
								hbm,SrcX,SrcY,SrcWidth,SrcHeight,Color,Opacity);
	}
	bool GetEpgCaptureStatus(EpgCaptureStatusInfo *pInfo) {
		pInfo->Size=sizeof(EpgCaptureStatusInfo);
		return MsgGetEpgCaptureStatus(m_pParam,pInfo);
	}
	bool GetAppCommandInfo(AppCommandInfo *pInfo) {
		pInfo->Size=sizeof(AppCommandInfo);
		return MsgGetAppCommandInfo(m_pParam,pInfo);
	}
	DWORD GetAppCommandCount() {
		return MsgGetAppCommandCount(m_pParam);
	}
	int GetVideoStreamCount() {
		return MsgGetVideoStreamCount(m_pParam);
	}
	int GetVideoStream() {
		return MsgGetVideoStream(m_pParam);
	}
	bool SetVideoStream(int Stream) {
		return MsgSetVideoStream(m_pParam,Stream);
	}
	bool GetLog(GetLogInfo *pInfo) {
		pInfo->Size=sizeof(GetLogInfo);
		return MsgGetLog(m_pParam,pInfo);
	}
	DWORD GetLogCount() {
		return MsgGetLogCount(m_pParam);
	}
	bool RegisterPluginCommand(const PluginCommandInfo *pInfo) {
		return MsgRegisterPluginCommand(m_pParam,pInfo);
	}
	bool SetPluginCommandState(int ID,DWORD State) {
		return MsgSetPluginCommandState(m_pParam,ID,State);
	}
	bool PluginCommandNotify(int ID,unsigned int Type) {
		return MsgPluginCommandNotify(m_pParam,ID,Type);
	}
	bool RegisterPluginIcon(const PluginIconInfo *pInfo) {
		return MsgRegisterPluginIcon(m_pParam,pInfo);
	}
	bool RegisterPluginIcon(HBITMAP hbmIcon) {
		return MsgRegisterPluginIcon(m_pParam,hbmIcon);
	}
	bool RegisterPluginIconFromResource(HINSTANCE hinst,LPCTSTR pszName) {
		return MsgRegisterPluginIconFromResource(m_pParam,hinst,pszName);
	}
	bool RegisterStatusItem(const StatusItemInfo *pInfo) {
		return MsgRegisterStatusItem(m_pParam,pInfo);
	}
	bool SetStatusItem(const StatusItemSetInfo *pInfo) {
		return MsgSetStatusItem(m_pParam,pInfo);
	}
	bool GetStatusItemInfo(StatusItemGetInfo *pInfo) {
		pInfo->Size=sizeof(StatusItemGetInfo);
		return MsgGetStatusItemInfo(m_pParam,pInfo);
	}
	bool StatusItemNotify(int ID,UINT Type) {
		return MsgStatusItemNotify(m_pParam,ID,Type);
	}
	bool RegisterTSProcessor(const TSProcessorInfo *pInfo) {
		return MsgRegisterTSProcessor(m_pParam,pInfo);
	}
	bool RegisterPanelItem(const PanelItemInfo *pInfo) {
		return MsgRegisterPanelItem(m_pParam,pInfo);
	}
	bool SetPanelItem(const PanelItemSetInfo *pInfo) {
		return MsgSetPanelItem(m_pParam,pInfo);
	}
	bool GetPanelItemInfo(PanelItemGetInfo *pInfo) {
		pInfo->Size=sizeof(PanelItemGetInfo);
		return MsgGetPanelItemInfo(m_pParam,pInfo);
	}
	bool SelectChannel(const ChannelSelectInfo *pInfo) {
		return MsgSelectChannel(m_pParam,pInfo);
	}
	bool GetFavoriteList(FavoriteList *pList) {
		pList->Size=sizeof(FavoriteList);
		return MsgGetFavoriteList(m_pParam,pList);
	}
	void FreeFavoriteList(FavoriteList *pList) {
		MsgFreeFavoriteList(m_pParam,pList);
	}
	bool Get1SegMode() {
		return MsgGet1SegMode(m_pParam);
	}
	bool Set1SegMode(bool f1SegMode) {
		return MsgSet1SegMode(m_pParam,f1SegMode);
	}
#endif
};

/*
	TVTest プラグインクラス

	プラグインをクラスとして記述するための抽象クラスです。
	このクラスを各プラグインで派生させて、プラグインの内容を記述します。
	このクラスを使わずに直接エクスポート関数を書いてもいいです。
*/
class CTVTestPlugin
{
protected:
	PluginParam *m_pPluginParam;
	CTVTestApp *m_pApp;
public:
	CTVTestPlugin() : m_pPluginParam(NULL),m_pApp(NULL) {}
	void SetPluginParam(PluginParam *pParam) {
		m_pPluginParam=pParam;
		m_pApp=new CTVTestApp(pParam);
	}
	virtual ~CTVTestPlugin() { delete m_pApp; }
	virtual DWORD GetVersion() { return TVTEST_PLUGIN_VERSION; }
	virtual bool GetPluginInfo(PluginInfo *pInfo)=0;
	virtual bool Initialize() { return true; }
	virtual bool Finalize() { return true; }
};

/*
	イベントハンドルクラス

	イベント処理用クラスです。
	このクラスを派生させてイベント処理を行うことができます。
	イベントコールバック関数として登録した関数内で HandleEvent を呼びます。
	もちろん使わなくてもいいです。

	以下は実装例です。

	class CMyEventHandler : public TVTest::CTVTestEventHandler
	{
	public:
		virtual bool OnPluginEnable(bool fEnable) {
			if (fEnable) {
				if (MessageBox(NULL, TEXT("有効にするよ"), TEXT("イベント"),
							   MB_OKCANCEL) != IDOK) {
					return false;
				}
			}
			return true;
		}
	};

	CMyEventHandler Handler;

	// この関数がイベントコールバック関数として登録されているものとします
	LRESULT CALLBACK EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
	{
		return Handler.HandleEvent(Event,lParam1,lParam2,pClientData);
	}
*/
class CTVTestEventHandler
{
protected:
	void *m_pClientData;

	// イベントハンドラは、特に記述の無いものは何か処理したら true を返します

	// 有効状態が変化した
	// 変化を拒否する場合 false を返します
	virtual bool OnPluginEnable(bool fEnable) { return false; }
	// 設定を行う
	// プラグインのフラグに PLUGIN_FLAG_HASSETTINGS が設定されている場合に呼ばれます
	// 設定が OK されたら true を返します
	virtual bool OnPluginSettings(HWND hwndOwner) { return false; }
	// チャンネルが変更された
	virtual bool OnChannelChange() { return false; }
	// サービスが変更された
	virtual bool OnServiceChange() { return false; }
	// ドライバが変更された
	virtual bool OnDriverChange() { return false; }
	// サービスの構成が変化した
	virtual bool OnServiceUpdate() { return false; }
	// 録画状態が変化した
	virtual bool OnRecordStatusChange(int Status) { return false; }
	// 全画面表示状態が変化した
	virtual bool OnFullscreenChange(bool fFullscreen) { return false; }
	// プレビュー表示状態が変化した
	virtual bool OnPreviewChange(bool fPreview) { return false; }
	// 音量が変化した
	virtual bool OnVolumeChange(int Volume,bool fMute) { return false; }
	// ステレオモードが変化した
	virtual bool OnStereoModeChange(int StereoMode) { return false; }
	// 色の設定が変化した
	virtual bool OnColorChange() { return false; }
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,3)
	// 待機状態が変化した
	virtual bool OnStandby(bool fStandby) { return false; }
	// コマンドが選択された
	virtual bool OnCommand(int ID) { return false; }
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,4)
	// 複数起動禁止時に複数起動された
	virtual bool OnExecute(LPCWSTR pszCommandLine) { return false; }
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,5)
	// リセットされた
	virtual bool OnReset() { return false; }
	// ステータス(MESSAGE_GETSTUATUSで取得できる内容)がリセットされた
	virtual bool OnStatusReset() { return false; }
	// 音声ストリームが変更された
	virtual bool OnAudioStreamChange(int Stream) { return false; }
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,9)
	// 設定が変更された
	virtual bool OnSettingsChange() { return false; }
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,10)
	// TVTestのウィンドウが閉じられる
	virtual bool OnClose() { return false; }
	// 録画が開始される
	virtual bool OnStartRecord(StartRecordInfo *pInfo) { return false; }
	// 録画ファイルの切り替えが行われた
	virtual bool OnRelayRecord(LPCWSTR pszFileName) { return false; }
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,11)
	// コントローラの対象の設定
	virtual bool OnControllerFocus(HWND hwnd) { return false; }
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,13)
	// 起動処理が終了した
	virtual void OnStartupDone() {}
	// 番組表の初期化
	virtual bool OnProgramGuideInitialize(HWND hwnd) { return true; }
	// 番組表の終了
	virtual bool OnProgramGuideFinalize(HWND hwnd) { return true; }
	// 番組表のコマンドの実行
	virtual bool OnProgramGuideCommand(
		UINT Command,const ProgramGuideCommandParam *pParam) { return false; }
	// 番組表のメニューの初期化
	virtual int OnProgramGuideInitializeMenu(
		const ProgramGuideInitializeMenuInfo *pInfo) { return 0; }
	// 番組表のメニューが選択された
	virtual bool OnProgramGuideMenuSelected(UINT Command) { return false; }
	// 番組表の番組の背景描画
	virtual bool OnProgramGuideProgramDrawBackground(
		const ProgramGuideProgramInfo *pProgramInfo,
		const ProgramGuideProgramDrawBackgroundInfo *pInfo) { return false; }
	// 番組表の番組のメニュー初期化
	virtual int OnProgramGuideProgramInitializeMenu(
		const ProgramGuideProgramInfo *pProgramInfo,
		const ProgramGuideProgramInitializeMenuInfo *pInfo) { return 0; }
	// 番組表の番組のメニューが選択された
	virtual bool OnProgramGuideProgramMenuSelected(
		const ProgramGuideProgramInfo *pProgramInfo,UINT Command) { return false; }
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,14)
	// フィルタグラフの初期化
	virtual void OnFilterGraphInitialize(FilterGraphInfo *pInfo) {}
	// フィルタグラフが初期化された
	virtual void OnFilterGraphInitialized(FilterGraphInfo *pInfo) {}
	// フィルタグラフの終了処理
	virtual void OnFilterGraphFinalize(FilterGraphInfo *pInfo) {}
	// フィルタグラフが終了処理された
	virtual void OnFilterGraphFinalized(FilterGraphInfo *pInfo) {}
	// コマンドアイコンの描画
	virtual bool OnDrawCommandIcon(DrawCommandIconInfo *pInfo) { return false; }
	// ステータス項目の描画
	virtual bool OnStatusItemDraw(StatusItemDrawInfo *pInfo) { return false; }
	// ステータス項目の通知
	virtual bool OnStatusItemNotify(StatusItemEventInfo *pInfo) { return false; }
	// ステータス項目のマウスイベント
	virtual bool OnStatusItemMouseEvent(StatusItemMouseEventInfo *pInfo) { return false; }
	// パネル項目の通知
	virtual bool OnPanelItemNotify(PanelItemEventInfo *pInfo) { return false; }
	// お気に入りチャンネルが変更された
	virtual void OnFavoritesChanged() {}
	// ワンセグモードが変わった
	virtual void On1SegModeChanged(bool f1SegMode) {}
#endif

public:
	virtual ~CTVTestEventHandler() {}
	LRESULT HandleEvent(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
	{
		m_pClientData=pClientData;
		switch (Event) {
		case EVENT_PLUGINENABLE:		return OnPluginEnable(lParam1!=0);
		case EVENT_PLUGINSETTINGS:		return OnPluginSettings((HWND)lParam1);
		case EVENT_CHANNELCHANGE:		return OnChannelChange();
		case EVENT_SERVICECHANGE:		return OnServiceChange();
		case EVENT_DRIVERCHANGE:		return OnDriverChange();
		case EVENT_SERVICEUPDATE:		return OnServiceUpdate();
		case EVENT_RECORDSTATUSCHANGE:	return OnRecordStatusChange((int)lParam1);
		case EVENT_FULLSCREENCHANGE:	return OnFullscreenChange(lParam1!=0);
		case EVENT_PREVIEWCHANGE:		return OnPreviewChange(lParam1!=0);
		case EVENT_VOLUMECHANGE:		return OnVolumeChange((int)lParam1,lParam2!=0);
		case EVENT_STEREOMODECHANGE:	return OnStereoModeChange((int)lParam1);
		case EVENT_COLORCHANGE:			return OnColorChange();
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,3)
		case EVENT_STANDBY:				return OnStandby(lParam1!=0);
		case EVENT_COMMAND:				return OnCommand((int)lParam1);
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,4)
		case EVENT_EXECUTE:				return OnExecute((LPCWSTR)lParam1);
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,5)
		case EVENT_RESET:				return OnReset();
		case EVENT_STATUSRESET:			return OnStatusReset();
		case EVENT_AUDIOSTREAMCHANGE:	return OnAudioStreamChange((int)lParam1);
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,9)
		case EVENT_SETTINGSCHANGE:		return OnSettingsChange();
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,10)
		case EVENT_CLOSE:				return OnClose();
		case EVENT_STARTRECORD:			return OnStartRecord((StartRecordInfo*)lParam1);
		case EVENT_RELAYRECORD:			return OnRelayRecord((LPCWSTR)lParam1);
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,11)
		case EVENT_CONTROLLERFOCUS:		return OnControllerFocus((HWND)lParam1);
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,13)
		case EVENT_STARTUPDONE:			OnStartupDone(); return 0;
		case EVENT_PROGRAMGUIDE_INITIALIZE:	return OnProgramGuideInitialize((HWND)lParam1);
		case EVENT_PROGRAMGUIDE_FINALIZE:	return OnProgramGuideFinalize((HWND)lParam1);
		case EVENT_PROGRAMGUIDE_COMMAND:
			return OnProgramGuideCommand(
				(UINT)lParam1,
				(const ProgramGuideCommandParam*)lParam2);
		case EVENT_PROGRAMGUIDE_INITIALIZEMENU:
			return OnProgramGuideInitializeMenu(
				(const ProgramGuideInitializeMenuInfo*)lParam1);
		case EVENT_PROGRAMGUIDE_MENUSELECTED:
			return OnProgramGuideMenuSelected((UINT)lParam1);
		case EVENT_PROGRAMGUIDE_PROGRAM_DRAWBACKGROUND:
			return OnProgramGuideProgramDrawBackground(
				(const ProgramGuideProgramInfo*)lParam1,
				(const ProgramGuideProgramDrawBackgroundInfo*)lParam2);
		case EVENT_PROGRAMGUIDE_PROGRAM_INITIALIZEMENU:
			return OnProgramGuideProgramInitializeMenu(
				(const ProgramGuideProgramInfo*)lParam1,
				(const ProgramGuideProgramInitializeMenuInfo*)lParam2);
		case EVENT_PROGRAMGUIDE_PROGRAM_MENUSELECTED:
			return OnProgramGuideProgramMenuSelected(
				(const ProgramGuideProgramInfo*)lParam1,(UINT)lParam2);
#endif
#if TVTEST_PLUGIN_VERSION>=TVTEST_PLUGIN_VERSION_(0,0,14)
		case EVENT_FILTERGRAPH_INITIALIZE:
			OnFilterGraphInitialize((FilterGraphInfo*)lParam1);
			return 0;
		case EVENT_FILTERGRAPH_INITIALIZED:
			OnFilterGraphInitialized((FilterGraphInfo*)lParam1);
			return 0;
		case EVENT_FILTERGRAPH_FINALIZE:
			OnFilterGraphFinalize((FilterGraphInfo*)lParam1);
			return 0;
		case EVENT_FILTERGRAPH_FINALIZED:
			OnFilterGraphFinalized((FilterGraphInfo*)lParam1);
			return 0;
		case EVENT_DRAWCOMMANDICON:
			return OnDrawCommandIcon((DrawCommandIconInfo*)lParam1);
		case EVENT_STATUSITEM_DRAW:
			return OnStatusItemDraw((StatusItemDrawInfo*)lParam1);
		case EVENT_STATUSITEM_NOTIFY:
			return OnStatusItemNotify((StatusItemEventInfo*)lParam1);
		case EVENT_STATUSITEM_MOUSE:
			return OnStatusItemMouseEvent((StatusItemMouseEventInfo*)lParam1);
		case EVENT_PANELITEM_NOTIFY:
			return OnPanelItemNotify((PanelItemEventInfo*)lParam1);
		case EVENT_FAVORITESCHANGED:
			OnFavoritesChanged();
			return 0;
		case EVENT_1SEGMODECHANGED:
			On1SegModeChanged(lParam1!=0);
			return 0;
#endif
		}
		return 0;
	}
};


}	// namespace TVTest


#include <poppack.h>


#ifdef TVTEST_PLUGIN_CLASS_IMPLEMENT
/*
	プラグインをクラスとして記述できるようにするための、エクスポート関数の実装
	です。
	これを使えば、エクスポート関数を自分で実装する必要がなくなります。

	プラグイン側では CreatePluginClass 関数を実装して、CTVTestPlugin クラスから
	派生させたプラグインクラスのインスタンスを new で生成して返します。例えば、
	以下のように書きます。

	#include <windows.h>
	#define TVTEST_PLUGIN_CLASS_IMPLEMENT
	#include "TVTestPlugin.h"

	// プラグインクラスの宣言
	class CMyPluginClass : public TVTest::CTVTestPlugin {
		...
	};

	// クラスのインスタンスを生成する
	TVTest::CTVTestPlugin *CreatePluginClass()
	{
		return new CMyPluginClass;
	}
*/


TVTest::CTVTestPlugin *CreatePluginClass();

HINSTANCE g_hinstDLL;				// DLL のインスタンスハンドル
TVTest::CTVTestPlugin *g_pPlugin;	// プラグインクラスへのポインタ


// エントリポイント
// プラグインクラスのインスタンスの生成と破棄を行っています
BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		g_hinstDLL=hinstDLL;
		g_pPlugin=CreatePluginClass();
		if (g_pPlugin==NULL)
			return FALSE;
		break;
	case DLL_PROCESS_DETACH:
		if (g_pPlugin) {
			delete g_pPlugin;
			g_pPlugin=NULL;
		}
		break;
	}
	return TRUE;
}

// プラグインの準拠するプラグイン仕様のバージョンを返す
// プラグインがロードされると最初にこの関数が呼ばれ、
// 対応していないバージョンが返された場合はすぐにアンロードされます。
TVTEST_EXPORT(DWORD) TVTGetVersion()
{
	return g_pPlugin->GetVersion();
}

// プラグインの情報を取得する
// TVTGetVersion の次に呼ばれるので、プラグインの情報を PluginInfo 構造体に設定します。
// FALSE が返された場合、すぐにアンロードされます。
TVTEST_EXPORT(BOOL) TVTGetPluginInfo(TVTest::PluginInfo *pInfo)
{
	return g_pPlugin->GetPluginInfo(pInfo);
}

// 初期化を行う
// TVTGetPluginInfo の次に呼ばれるので、初期化処理を行います。
// FALSE が返された場合、すぐにアンロードされます。
TVTEST_EXPORT(BOOL) TVTInitialize(TVTest::PluginParam *pParam)
{
	g_pPlugin->SetPluginParam(pParam);
	return g_pPlugin->Initialize();
}

// 終了処理を行う
// プラグインがアンロードされる前に呼ばれるので、終了処理を行います。
// この関数が呼ばれるのは TVTInitialize 関数が TRUE を返した場合だけです。
TVTEST_EXPORT(BOOL) TVTFinalize()
{
	bool fOK=g_pPlugin->Finalize();
	delete g_pPlugin;
	g_pPlugin=NULL;
	return fOK;
}


#endif	// TVTEST_PLUGIN_CLASS_IMPLEMENT


#endif	// TVTEST_PLUGIN_H
