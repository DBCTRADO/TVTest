/*
	TVTest プラグインサンプル

	パケットカウンター

	ステータスバーにパケットを数える項目を追加します。
	このサンプルでは、プラグインの有効/無効の状態と
	ステータスバーの項目の表示/非表示の状態の同期をとっています。

	このサンプルでは主に以下の機能を実装しています。

	・TS のパケットを取得する
	・ステータスバー項目を追加する
*/


#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <tchar.h>

#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"


// プラグインクラス
class CPacketCounter : public TVTest::CTVTestPlugin
{
	// ステータス項目の識別子
	static constexpr int STATUS_ITEM_ID = 1;

	LONG m_PacketCount = 0;

	static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);
	static BOOL CALLBACK StreamCallback(BYTE *pData, void *pClientData);
	void ShowItem(bool fShow);

public:
	bool GetPluginInfo(TVTest::PluginInfo *pInfo) override;
	bool Initialize() override;
	bool Finalize() override;
};


bool CPacketCounter::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	// プラグインの情報を返す
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"パケットカウンター";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"ステータスバーにパケットを数える項目を追加します。";
	return true;
}


bool CPacketCounter::Initialize()
{
	// 初期化処理

	// ステータス項目を登録
	TVTest::StatusItemInfo Item;
	Item.Size         = sizeof(Item);
	Item.Flags        = TVTest::STATUS_ITEM_FLAG_TIMERUPDATE;
	Item.Style        = 0;
	Item.ID           = STATUS_ITEM_ID;
	Item.pszIDText    = L"PacketCounter";
	Item.pszName      = L"パケットカウンター";
	Item.MinWidth     = 0;
	Item.MaxWidth     = -1;
	Item.DefaultWidth = TVTest::StatusItemWidthByFontSize(6);
	Item.MinHeight    = 0;
	if (!m_pApp->RegisterStatusItem(&Item)) {
		m_pApp->AddLog(L"ステータス項目を登録できません。", TVTest::LOG_TYPE_ERROR);
		return false;
	}

	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(EventCallback, this);

	// ストリームコールバック関数を登録
	m_pApp->SetStreamCallback(0, StreamCallback, this);

	return true;
}


bool CPacketCounter::Finalize()
{
	// 終了処理

	return true;
}


// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CPacketCounter::EventCallback(
	UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
{
	CPacketCounter *pThis = static_cast<CPacketCounter *>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// プラグインの有効状態が変化した
		// プラグインの有効状態と項目の表示状態を同期する
		pThis->ShowItem(lParam1 != 0);
		return TRUE;

	case TVTest::EVENT_STATUSITEM_DRAW:
		// ステータス項目の描画
		{
			const TVTest::StatusItemDrawInfo *pInfo =
				reinterpret_cast<const TVTest::StatusItemDrawInfo *>(lParam1);
			WCHAR szText[32];

			if ((pInfo->Flags & TVTest::STATUS_ITEM_DRAW_FLAG_PREVIEW) == 0) {
				// 通常の項目の描画
				::_itow_s(pThis->m_PacketCount, szText, 10);
			} else {
				// プレビュー(設定ダイアログ)の項目の描画
				::lstrcpyW(szText, L"123456");
			}
			pThis->m_pApp->ThemeDrawText(
				pInfo->pszStyle, pInfo->hdc, szText,
				pInfo->DrawRect,
				DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS,
				pInfo->Color);
		}
		return TRUE;

	case TVTest::EVENT_STATUSITEM_NOTIFY:
		// ステータス項目の通知
		{
			const TVTest::StatusItemEventInfo *pInfo =
				reinterpret_cast<const TVTest::StatusItemEventInfo *>(lParam1);

			switch (pInfo->Event) {
			case TVTest::STATUS_ITEM_EVENT_CREATED:
				// 項目が作成された
				// プラグインが有効であれば項目を表示状態にする
				pThis->ShowItem(pThis->m_pApp->IsPluginEnabled());
				return TRUE;

			case TVTest::STATUS_ITEM_EVENT_VISIBILITYCHANGED:
				// 項目の表示状態が変わった
				// 項目の表示状態とプラグインの有効状態を同期する
				pThis->m_pApp->EnablePlugin(pInfo->Param != 0);
				return TRUE;

			case TVTest::STATUS_ITEM_EVENT_UPDATETIMER:
				// 更新タイマー
				return TRUE; // TRUE を返すと再描画される
			}
		}
		return FALSE;

	case TVTest::EVENT_STATUSITEM_MOUSE:
		// ステータス項目のマウス操作
		{
			const TVTest::StatusItemMouseEventInfo *pInfo =
				reinterpret_cast<const TVTest::StatusItemMouseEventInfo *>(lParam1);

			// 左ボタンが押されたらリセットする
			if (pInfo->Action == TVTest::STATUS_ITEM_MOUSE_ACTION_LDOWN) {
				::InterlockedExchange(&pThis->m_PacketCount, 0);
				// 項目を再描画
				pThis->m_pApp->StatusItemNotify(STATUS_ITEM_ID, TVTest::STATUS_ITEM_NOTIFY_REDRAW);
				return TRUE;
			}
		}
		return FALSE;
	}

	return 0;
}


// ストリームコールバック関数
// 188バイトのパケットデータが渡される
BOOL CALLBACK CPacketCounter::StreamCallback(BYTE *pData, void *pClientData)
{
	CPacketCounter *pThis = static_cast<CPacketCounter *>(pClientData);

	::InterlockedIncrement(&pThis->m_PacketCount);

	return TRUE; // FALSEを返すとパケットが破棄される
}


// ステータス項目の表示/非表示を切り替える
void CPacketCounter::ShowItem(bool fShow)
{
	TVTest::StatusItemSetInfo Info;

	Info.Size      = sizeof(Info);
	Info.Mask      = TVTest::STATUS_ITEM_SET_INFO_MASK_STATE;
	Info.ID        = STATUS_ITEM_ID;
	Info.StateMask = TVTest::STATUS_ITEM_STATE_VISIBLE;
	Info.State     = fShow ? TVTest::STATUS_ITEM_STATE_VISIBLE : 0;

	m_pApp->SetStatusItem(&Info);
}




TVTest::CTVTestPlugin * CreatePluginClass()
{
	return new CPacketCounter;
}
