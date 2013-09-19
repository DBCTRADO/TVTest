/*
	TVTest プラグインサンプル

	信号レベルとビットレートをグラフ表示する
*/


#include <windows.h>
#include <tchar.h>
#include <deque>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT	// クラスとして実装
#include "TVTestPlugin.h"


// ウィンドウクラス名
#define SIGNAL_GRAPH_WINDOW_CLASS TEXT("Signal Graph Window")

// ウィンドウのクライアント領域の大きさ
#define WINDOW_WIDTH	300
#define WINDOW_HEIGHT	200


// プラグインクラス
class CSignalGraph : public TVTest::CTVTestPlugin
{
	struct SignalInfo {
		DWORD SignalLevel;
		DWORD BitRate;
	};
	std::deque<SignalInfo> m_List;
	HWND m_hwnd;
	COLORREF m_crBackColor;
	COLORREF m_crSignalLevelColor;
	COLORREF m_crBitRateColor;
	COLORREF m_crGridColor;

	static LRESULT CALLBACK EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData);
	static CSignalGraph *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	CSignalGraph();
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();
};


CSignalGraph::CSignalGraph()
	: m_hwnd(NULL)
	, m_crBackColor(RGB(0,0,0))
	, m_crSignalLevelColor(RGB(0,255,0))
	, m_crBitRateColor(RGB(0,0,255))
	, m_crGridColor(RGB(128,128,128))
{
}


// プラグインの情報を返す
bool CSignalGraph::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"Signal Graph";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"信号レベルとビットレートをグラフ表示します。";
	return true;
}


// 初期化処理
bool CSignalGraph::Initialize()
{
	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(EventCallback,this);

	return true;
}


// 終了処理
bool CSignalGraph::Finalize()
{
	// ウィンドウの破棄
	if (m_hwnd!=NULL)
		::DestroyWindow(m_hwnd);

	return true;
}


// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CSignalGraph::EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
{
	CSignalGraph *pThis=static_cast<CSignalGraph*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// プラグインの有効状態が変化した
		{
			bool fEnable=lParam1!=0;

			if (fEnable) {
				// ウィンドウクラスの登録
				static bool fInitialized=false;
				if (!fInitialized) {
					WNDCLASS wc;

					wc.style=0;
					wc.lpfnWndProc=WndProc;
					wc.cbClsExtra=0;
					wc.cbWndExtra=0;
					wc.hInstance=g_hinstDLL;
					wc.hIcon=NULL;
					wc.hCursor=::LoadCursor(NULL,IDC_ARROW);
					wc.hbrBackground=NULL;
					wc.lpszMenuName=NULL;
					wc.lpszClassName=SIGNAL_GRAPH_WINDOW_CLASS;
					if (::RegisterClass(&wc)==0)
						return FALSE;
					fInitialized=true;
				}

				if (pThis->m_hwnd==NULL) {
					// ウィンドウの作成
					RECT rc={0,0,WINDOW_WIDTH,WINDOW_HEIGHT};
					::AdjustWindowRectEx(&rc,WS_POPUP | WS_CAPTION | WS_SYSMENU,
										 FALSE,WS_EX_TOOLWINDOW);
					if (::CreateWindowEx(WS_EX_TOOLWINDOW,SIGNAL_GRAPH_WINDOW_CLASS,
										 TEXT("Signal Graph"),
										 WS_POPUP | WS_CAPTION | WS_SYSMENU,
										 0,0,rc.right-rc.left,rc.bottom-rc.top,
										 pThis->m_pApp->GetAppWindow(),
										 NULL,g_hinstDLL,pThis)==NULL)
						return FALSE;
				}
			}

			::ShowWindow(pThis->m_hwnd,fEnable?SW_SHOW:SW_HIDE);
		}
		return TRUE;

	case TVTest::EVENT_STANDBY:
		// 待機状態が変化した
		if (pThis->m_pApp->IsPluginEnabled()) {
			// 待機状態の時はウィンドウを隠す
			::ShowWindow(pThis->m_hwnd,lParam1!=0?SW_HIDE:SW_SHOW);
		}
		return TRUE;
	}
	return 0;
}


// ウィンドウハンドルからthisを取得する
CSignalGraph *CSignalGraph::GetThis(HWND hwnd)
{
	return reinterpret_cast<CSignalGraph*>(::GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


// ウィンドウプロシージャ
LRESULT CALLBACK CSignalGraph::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs=reinterpret_cast<LPCREATESTRUCT>(lParam);
			CSignalGraph *pThis=static_cast<CSignalGraph*>(pcs->lpCreateParams);

			::SetWindowLongPtr(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(pThis));
			pThis->m_hwnd=hwnd;

			// 更新用タイマの設定
			::SetTimer(hwnd,1,1000,NULL);
		}
		return 0;

	case WM_PAINT:
		{
			CSignalGraph *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;
			HBRUSH hbr;
			HPEN hpen,hpenOld;
			const int ListSize=(int)pThis->m_List.size();
			int x,y,i;

			::BeginPaint(hwnd,&ps);
			hbr=::CreateSolidBrush(pThis->m_crBackColor);
			::FillRect(ps.hdc,&ps.rcPaint,hbr);
			::DeleteObject(hbr);
			hpenOld=static_cast<HPEN>(::GetCurrentObject(ps.hdc,OBJ_PEN));

			// グリッドを描く
			hpen=::CreatePen(PS_SOLID,1,pThis->m_crGridColor);
			::SelectObject(ps.hdc,hpen);
			for (y=ps.rcPaint.top/10*10;y<ps.rcPaint.bottom;y+=10) {
				::MoveToEx(ps.hdc,ps.rcPaint.left,y,NULL);
				::LineTo(ps.hdc,ps.rcPaint.right,y);
			}

			// ビットレートを描く
			hpen=::CreatePen(PS_SOLID,1,pThis->m_crBitRateColor);
			::DeleteObject(::SelectObject(ps.hdc,hpen));
			if (ps.rcPaint.left<WINDOW_WIDTH-ListSize) {
				x=WINDOW_WIDTH-ListSize;
				i=0;
			} else {
				x=ps.rcPaint.left;
				i=ps.rcPaint.left-(WINDOW_WIDTH-ListSize);
			}
			for (;x<min(ps.rcPaint.right,WINDOW_WIDTH);x++) {
				y=min(pThis->m_List[i].BitRate,40*1024*1024)*WINDOW_HEIGHT/(40*1024*1024);
				if (y>0) {
					::MoveToEx(ps.hdc,x,WINDOW_HEIGHT-y,NULL);
					::LineTo(ps.hdc,x,WINDOW_HEIGHT);
				}
				i++;
			}

			// 信号レベルを描く
			hpen=::CreatePen(PS_SOLID,1,pThis->m_crSignalLevelColor);
			::DeleteObject(::SelectObject(ps.hdc,hpen));
			if (max(ps.rcPaint.left-1,0)<WINDOW_WIDTH-ListSize) {
				x=WINDOW_WIDTH-ListSize;
				i=0;
			} else {
				x=max(ps.rcPaint.left-1,0);
				i=x-(WINDOW_WIDTH-ListSize);
			}
			for (;x<min(ps.rcPaint.right,WINDOW_WIDTH);x++) {
				y=min(pThis->m_List[i].SignalLevel,800)*(WINDOW_HEIGHT-1)/800;
				::MoveToEx(ps.hdc,x,WINDOW_HEIGHT-1-y,NULL);
				i++;
				if (i<ListSize) {
					y=min(pThis->m_List[i].SignalLevel,800)*(WINDOW_HEIGHT-1)/800;
				}
				::LineTo(ps.hdc,x+1,WINDOW_HEIGHT-1-y);
			}
			::DeleteObject(::SelectObject(ps.hdc,hpenOld));
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_TIMER:
		{
			CSignalGraph *pThis=GetThis(hwnd);
			TVTest::StatusInfo Status;
			SignalInfo Info;
			RECT rc;

			// 情報取得&表示更新
			pThis->m_pApp->GetStatus(&Status);
			Info.SignalLevel=(DWORD)(max(Status.SignalLevel,0.0f)*10.0f);
			Info.BitRate=Status.BitRate;
			if (pThis->m_List.size()==WINDOW_WIDTH)
				pThis->m_List.pop_front();
			pThis->m_List.push_back(Info);
			::ScrollWindowEx(hwnd,-1,0,NULL,NULL,NULL,NULL,0/*SW_INVALIDATE*/);
			::GetClientRect(hwnd,&rc);
			rc.left=rc.right-2;
			::InvalidateRect(hwnd,&rc,TRUE);
		}
		return 0;

	case WM_SYSCOMMAND:
		if ((wParam&0xFFF0)==SC_CLOSE) {
			// 閉じる時はプラグインを無効にする
			CSignalGraph *pThis=GetThis(hwnd);

			pThis->m_pApp->EnablePlugin(false);
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			CSignalGraph *pThis=GetThis(hwnd);

			::KillTimer(hwnd,1);	// 別にしなくてもいいけど...
			pThis->m_hwnd=NULL;
		}
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}




// プラグインクラスのインスタンスを生成する
TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CSignalGraph;
}
