/*
	TVTest プラグインサンプル

	コマンドを登録する
*/


#include <windows.h>
#include <tchar.h>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"


// プラグインクラス
class CCommandSample : public TVTest::CTVTestPlugin
{
	static LRESULT CALLBACK EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData);
public:
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();
};


bool CCommandSample::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	// プラグインの情報を返す
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"Command Sample";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"コマンドを登録するサンプル";
	return true;
}


bool CCommandSample::Initialize()
{
	// 初期化処理

	// コマンドを登録

	// 一つずつ登録する例
	m_pApp->RegisterCommand(1, L"Command1", L"コマンド1");
	m_pApp->RegisterCommand(2, L"Command2", L"コマンド2");

	// まとめて登録する例
	static const TVTest::CommandInfo CommandList[] = {
		{3,	L"Command3",	L"コマンド3"},
		{4,	L"Command4",	L"コマンド4"},
	};
	m_pApp->RegisterCommand(CommandList, sizeof(CommandList) / sizeof(TVTest::CommandInfo));

	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(EventCallback, this);

	return true;
}


bool CCommandSample::Finalize()
{
	// 終了処理

	return true;
}


// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CCommandSample::EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
{
	CCommandSample *pThis = static_cast<CCommandSample*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_COMMAND:
		{
			TCHAR szText[256];

			::wsprintf(szText, TEXT("コマンド%dが選択されました。"),(int)lParam1);
			::MessageBox(pThis->m_pApp->GetAppWindow(), szText, TEXT("コマンド"), MB_OK);
		}
		return TRUE;
	}
	return 0;
}




TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CCommandSample;
}
