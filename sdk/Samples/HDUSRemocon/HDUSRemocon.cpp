/*
	TVTest プラグインサンプル

	HDUSのリモコンを使う

	このサンプルでは主に以下の機能を実装しています。

	・コントローラを登録する
	・メッセージフックを利用する
*/


#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>

#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"
#include "HDUSRemocon_KeyHook.h"
#include "resource.h"

#pragma comment(lib, "shlwapi.lib")


// ボタンのリスト
static const struct {
	TVTest::ControllerButtonInfo Info;
	BYTE KeyCode;
	BYTE Modifiers;
} g_ButtonList[] = {
	{{L"画面表示",     L"AspectRatio",        { 8,  14, 16, 11}, {  0,  0}}, VK_F13,  MK_SHIFT},
	{{L"電源",         L"Close",              {32,  11, 16, 16}, {  0, 22}}, VK_F14,  MK_SHIFT},
	{{L"消音",         L"Mute",               {56,  14, 16, 11}, { 16,  0}}, VK_F15,  MK_SHIFT},
	{{L"チャンネル1",  L"Channel1",           { 8,  28, 16, 11}, { 32,  0}}, VK_F17,  MK_SHIFT},
	{{L"チャンネル2",  L"Channel2",           {24,  28, 16, 11}, { 48,  0}}, VK_F18,  MK_SHIFT},
	{{L"チャンネル3",  L"Channel3",           {40,  28, 16, 11}, { 64,  0}}, VK_F19,  MK_SHIFT},
	{{L"チャンネル4",  L"Channel4",           { 8,  43, 16, 11}, { 80,  0}}, VK_F20,  MK_SHIFT},
	{{L"チャンネル5",  L"Channel5",           {24,  43, 16, 11}, { 96,  0}}, VK_F21,  MK_SHIFT},
	{{L"チャンネル6",  L"Channel6",           {40,  43, 16, 11}, {112,  0}}, VK_F22,  MK_SHIFT},
	{{L"チャンネル7",  L"Channel7",           { 8,  57, 16, 11}, {128,  0}}, VK_F23,  MK_SHIFT},
	{{L"チャンネル8",  L"Channel8",           {24,  57, 16, 11}, {144,  0}}, VK_F24,  MK_SHIFT},
	{{L"チャンネル9",  L"Channel9",           {40,  57, 16, 11}, {160,  0}}, VK_F13,  MK_CONTROL},
	{{L"チャンネル10", L"Channel10",          { 8,  71, 16, 11}, {176,  0}}, VK_F16,  MK_SHIFT},
	{{L"チャンネル11", L"Channel11",          {24,  71, 16, 11}, {192,  0}}, VK_F14,  MK_CONTROL},
	{{L"チャンネル12", L"Channel12",          {40,  71, 16, 11}, {208,  0}}, VK_F15,  MK_CONTROL},
	{{L"メニュー",     L"Menu",               {11,  85, 28, 11}, { 32, 11}}, VK_F16,  MK_CONTROL},
	{{L"全画面表示",   L"Fullscreen",         {41,  85, 28, 11}, { 60, 11}}, VK_F17,  MK_CONTROL},
	{{L"字幕",         nullptr,               { 9, 102, 24, 25}, {122, 22}}, VK_F18,  MK_CONTROL},
	{{L"音声切替",     L"SwitchAudio",        {47, 102, 24, 25}, {146, 22}}, VK_F19,  MK_CONTROL},
	{{L"EPG",          L"ProgramGuide",       { 9, 140, 24, 25}, {170, 22}}, VK_F20,  MK_CONTROL},
	{{L"戻る",         nullptr,               {47, 140, 24, 25}, {194, 22}}, VK_F21,  MK_CONTROL},
	{{L"録画",         L"RecordStart",        {20, 170, 16, 11}, { 88, 11}}, VK_F22,  MK_CONTROL},
	{{L"メモ",         L"SaveImage",          {44, 170, 16, 11}, {104, 11}}, VK_F23,  MK_CONTROL},
	{{L"停止",         L"RecordStop",         { 8, 186, 16, 11}, {120, 11}}, VK_F24,  MK_CONTROL},
	{{L"再生",         nullptr,               {26, 184, 28, 14}, { 16, 22}}, VK_F13,  MK_CONTROL | MK_SHIFT},
	{{L"一時停止",     L"RecordPause",        {56, 187, 16, 11}, {136, 11}}, VK_F14,  MK_CONTROL | MK_SHIFT},
	{{L"|<<",          nullptr,               { 8, 201, 16, 11}, {152, 11}}, VK_F15,  MK_CONTROL | MK_SHIFT},
	{{L"<<",           nullptr,               {24, 201, 16, 11}, {168, 11}}, VK_F16,  MK_CONTROL | MK_SHIFT},
	{{L">>",           nullptr,               {40, 201, 16, 11}, {184, 11}}, VK_F17,  MK_CONTROL | MK_SHIFT},
	{{L">>|",          nullptr,               {56, 201, 16, 11}, {200, 11}}, VK_F18,  MK_CONTROL | MK_SHIFT},
	{{L"しおり",       L"ChannelDisplayMenu", {24, 215, 16, 11}, {216, 11}}, VK_F19,  MK_CONTROL | MK_SHIFT},
	{{L"ジャンプ",     L"ChannelMenu",        {40, 215, 16, 11}, {232, 11}}, VK_F20,  MK_CONTROL | MK_SHIFT},
	{{L"A (青)",       nullptr,               { 8, 230, 16, 11}, { 44, 22}}, VK_F21,  MK_CONTROL | MK_SHIFT},
	{{L"B (赤)",       nullptr,               {24, 230, 16, 11}, { 60, 22}}, VK_F22,  MK_CONTROL | MK_SHIFT},
	{{L"C (緑)",       nullptr,               {40, 230, 16, 11}, { 76, 22}}, VK_F23,  MK_CONTROL | MK_SHIFT},
	{{L"D (黄)",       nullptr,               {56, 230, 16, 11}, { 92, 22}}, VK_F24,  MK_CONTROL | MK_SHIFT},
#if 0
	{{L"音量 +",       L"VolumeUp",           {56,  28, 16, 11}, {  0, 11}}, VK_UP,   MK_SHIFT},
	{{L"音量 -",       L"VolumeDown",         {56,  43, 16, 11}, { 16, 11}}, VK_DOWN, MK_SHIFT},
	{{L"チャンネル +", L"ChannelUp",          {57,  57, 14, 13}, {108, 22}}, VK_UP,   MK_CONTROL},
	{{L"チャンネル -", L"ChannelDown",        {57,  70, 14, 13}, {108, 35}}, VK_DOWN, MK_CONTROL},
#endif
};


// プラグインクラス
class CHDUSRemocon : public TVTest::CTVTestPlugin
{
	// コントローラ名
	static const LPCTSTR HDUS_REMOCON_NAME; 

	// ボタンの数
	static constexpr int NUM_BUTTONS = sizeof(g_ButtonList) / sizeof(g_ButtonList[0]);

	bool m_fInitialized = false; // 初期化済みか?
	HMODULE m_hLib = nullptr;    // フックDLLのハンドル
	bool m_fHook = false;        // フックしているか?
	static UINT m_Message;       // フックDLLから送られるメッセージ

	bool InitializePlugin();
	bool BeginHook();
	bool EndHook();
	bool SetWindow(HWND hwnd);
	void OnError(LPCWSTR pszMessage);

	static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);
	static BOOL CALLBACK MessageCallback(
		HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult, void *pClientData);
	static BOOL CALLBACK TranslateMessageCallback(HWND hwnd, MSG *pMessage, void *pClientData);

public:
	bool GetPluginInfo(TVTest::PluginInfo *pInfo) override;
	bool Initialize() override;
	bool Finalize() override;
};


const LPCTSTR CHDUSRemocon::HDUS_REMOCON_NAME = L"HDUS Remocon";

UINT CHDUSRemocon::m_Message = 0;


bool CHDUSRemocon::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	// プラグインの情報を返す
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"HDUSリモコン";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"HDUSのリモコンに対応します。";
	return true;
}


bool CHDUSRemocon::Initialize()
{
	// 初期化処理

	// ボタンのリストを作成
	TVTest::ControllerButtonInfo ButtonList[NUM_BUTTONS];
	for (int i = 0; i < NUM_BUTTONS; i++)
		ButtonList[i] = g_ButtonList[i].Info;

	// コントローラの登録
	TVTest::ControllerInfo Info;
	Info.Flags             = 0;
	Info.pszName           = HDUS_REMOCON_NAME;
	Info.pszText           = L"HDUSリモコン";
	Info.NumButtons        = NUM_BUTTONS;
	Info.pButtonList       = ButtonList;
	Info.pszIniFileName    = nullptr;
	Info.pszSectionName    = L"HDUSController";
	Info.ControllerImageID = IDB_CONTROLLER;
	Info.SelButtonsImageID = IDB_SELBUTTONS;
	Info.pTranslateMessage = TranslateMessageCallback;
	Info.pClientData       = this;
	if (!m_pApp->RegisterController(&Info)) {
		m_pApp->AddLog(L"コントローラを登録できません。", TVTest::LOG_TYPE_ERROR);
		return false;
	}

	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(EventCallback, this);

	return true;
}


// プラグインが有効にされた時の初期化処理
bool CHDUSRemocon::InitializePlugin()
{
	if (!m_fInitialized) {
		// メッセージの登録
		m_Message = ::RegisterWindowMessage(KEYHOOK_MESSAGE);
		if (m_Message == 0)
			return false;

		// 管理者権限で実行された時用の対策
		::ChangeWindowMessageFilterEx(m_pApp->GetAppWindow(), m_Message, MSGFLT_ALLOW, nullptr);

		m_fInitialized = true;
	}

	// メッセージコールバック関数を登録
	m_pApp->SetWindowMessageCallback(MessageCallback, this);

	return true;
}


bool CHDUSRemocon::Finalize()
{
	// 終了処理

	// フック終わり
	if (m_hLib != nullptr) {
		EndHook();
		::FreeLibrary(m_hLib);
		m_hLib = nullptr;
	}

	return true;
}


bool CHDUSRemocon::BeginHook()
{
	// フック開始

	if (m_hLib == nullptr) {
		TCHAR szFileName[MAX_PATH];

		::GetModuleFileName(g_hinstDLL, szFileName, MAX_PATH);
		::lstrcpy(::PathFindExtension(szFileName), TEXT("_KeyHook.dll"));
		m_hLib = ::LoadLibrary(szFileName);
		if (m_hLib == nullptr)
			return false;
	}

	BeginHookFunc pBeginHook = (BeginHookFunc)::GetProcAddress(m_hLib, "BeginHook");
	if (pBeginHook == nullptr
			|| !pBeginHook(m_pApp->GetAppWindow(), m_pApp->IsControllerActiveOnly(HDUS_REMOCON_NAME))) {
		::FreeLibrary(m_hLib);
		m_hLib = nullptr;
		return false;
	}
	m_fHook = true;

	return true;
}


bool CHDUSRemocon::EndHook()
{
	// フック終わり

	if (m_fHook) {
		EndHookFunc pEndHook = (EndHookFunc)::GetProcAddress(m_hLib, "EndHook");
		if (pEndHook == nullptr)
			return false;
		pEndHook();
		m_fHook = false;
	}
	return true;
}


bool CHDUSRemocon::SetWindow(HWND hwnd)
{
	// ウィンドウを設定

	if (!m_fHook)
		return false;
	SetWindowFunc pSetWindow = (SetWindowFunc)::GetProcAddress(m_hLib, "SetWindow");
	if (pSetWindow == nullptr)
		return false;
	return pSetWindow(hwnd) != FALSE;
}


void CHDUSRemocon::OnError(LPCWSTR pszMessage)
{
	// エラー発生時のメッセージ表示
	m_pApp->AddLog(pszMessage, TVTest::LOG_TYPE_ERROR);
	if (!m_pApp->GetSilentMode()) {
		::MessageBoxW(m_pApp->GetAppWindow(), pszMessage, L"HDUSリモコン", MB_OK | MB_ICONEXCLAMATION);
	}
}


// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CHDUSRemocon::EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
{
	CHDUSRemocon *pThis = static_cast<CHDUSRemocon *>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// プラグインの有効状態が変化した
		if (lParam1 != 0) {
			if (!pThis->InitializePlugin()) {
				pThis->OnError(L"初期化でエラーが発生しました。");
				return FALSE;
			}
			if (!pThis->BeginHook()) {
				pThis->OnError(L"フックできません。");
				return FALSE;
			}
		} else {
			pThis->EndHook();
			pThis->m_pApp->SetWindowMessageCallback(nullptr);
		}
		return TRUE;

	case TVTest::EVENT_CONTROLLERFOCUS:
		// このプロセスが操作の対象になった
		pThis->SetWindow(reinterpret_cast<HWND>(lParam1));
		return TRUE;
	}

	return 0;
}


// メッセージコールバック関数
BOOL CALLBACK CHDUSRemocon::MessageCallback(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult, void *pClientData)
{
	if (uMsg == m_Message) {
		// フックDLLから送られるメッセージ

		if (KEYHOOK_GET_REPEATCOUNT(lParam) > 1)
			return 0;

		BYTE Modifiers = 0;
		if (KEYHOOK_GET_CONTROL(lParam))
			Modifiers |= MK_CONTROL;
		if (KEYHOOK_GET_SHIFT(lParam))
			Modifiers |= MK_SHIFT;
		WORD KeyCode = (WORD)wParam;

		// 押されたボタンを探す
		for (int i = 0; i < NUM_BUTTONS; i++) {
			if (g_ButtonList[i].KeyCode == KeyCode
					&& g_ButtonList[i].Modifiers == Modifiers) {
				CHDUSRemocon *pThis = static_cast<CHDUSRemocon *>(pClientData);

				// ボタンが押されたことを通知する
				pThis->m_pApp->OnControllerButtonDown(HDUS_REMOCON_NAME, i);
				break;
			}
		}
		return TRUE;
	}

	return FALSE;
}


// メッセージの変換コールバック
// Shift+↑ などはフックすると問題あるのでここで処理しています
BOOL CALLBACK CHDUSRemocon::TranslateMessageCallback(HWND hwnd, MSG *pMessage, void *pClientData)
{
	if (pMessage->message == WM_KEYDOWN
			&& (pMessage->wParam == VK_UP || pMessage->wParam == VK_DOWN)) {
		bool fShift = ::GetKeyState(VK_SHIFT) < 0;
		bool fCtrl = ::GetKeyState(VK_CONTROL) < 0;

		// どちらか一方が押されている場合のみ処理する
		if (fShift != fCtrl) {
			CHDUSRemocon *pThis = static_cast<CHDUSRemocon *>(pClientData);

			pThis->m_pApp->DoCommand(
				pMessage->wParam == VK_UP ?
					(fShift ? L"VolumeUp" : L"ChannelUp"):
					(fShift ? L"VolumeDown" : L"ChannelDown"));
			return TRUE;
		}
	}

	return FALSE;
}




// プラグインクラスのインスタンスを生成する
TVTest::CTVTestPlugin * CreatePluginClass()
{
	return new CHDUSRemocon;
}
