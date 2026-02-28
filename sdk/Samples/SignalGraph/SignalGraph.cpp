/*
	TVTest プラグインサンプル

	信号レベルとビットレートをグラフ表示する

	このサンプルでは主に以下の機能を実装しています。

	・信号レベルとビットレートを取得する
	・ウィンドウを表示する
	・TVTest に合わせてウィンドウをダークモードにする
*/


#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <tchar.h>
#include <objbase.h>
#include <shlwapi.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <deque>
#include <strsafe.h>

// Windows SDK version 2104 (10.0.20348.0) より前は Gdiplus に min / max の宣言が必要
namespace Gdiplus {
	using std::min;
	using std::max;
}
#include <gdiplus.h>

#define TVTEST_PLUGIN_CLASS_IMPLEMENT // クラスとして実装
#include "TVTestPlugin.h"
#include "resource.h"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shlwapi.lib")


// プラグインクラス
class CSignalGraph : public TVTest::CTVTestPlugin
{
public:
	bool GetPluginInfo(TVTest::PluginInfo *pInfo) override;
	bool Initialize() override;
	bool Finalize() override;

private:
	struct Position
	{
		int Left = 0, Top = 0, Width = 0, Height = 0;
	};

	struct SignalInfo
	{
		float SignalLevel = 0.0f;
		DWORD BitRate = 0;

		SignalInfo() = default;
		SignalInfo(float level, DWORD rate) : SignalLevel(level), BitRate(rate) {}
	};

	bool m_fInitialized = false;
	std::deque<SignalInfo> m_List;
	HWND m_hwnd = nullptr;
	Position m_WindowPosition;
	int m_DPI;
	Gdiplus::Color m_BackColor{255, 0, 0, 0};
	Gdiplus::Color m_SignalLevelColor{255, 0, 255, 128};
	Gdiplus::Color m_BitRateColor{192, 0, 160, 255};
	Gdiplus::Color m_GridColor{255, 64, 64, 64};
	Gdiplus::Pen *m_pGridPen = nullptr;
	Gdiplus::Pen *m_pSignalLevelPen = nullptr;
	Gdiplus::Pen *m_pBitRatePen = nullptr;
	Gdiplus::SolidBrush *m_pBrush = nullptr;
	LOGFONT m_Font;
	Gdiplus::Font *m_pFont = nullptr;
	Gdiplus::Graphics *m_pOffscreen = nullptr;
	Gdiplus::Bitmap *m_pOffscreenImage = nullptr;
	float m_SignalLevelScale = 80.0f;
	float m_ActualSignalLevelScale;
	DWORD m_BitRateScale = 40 * 1000 * 1000;

	static const LPCTSTR WINDOW_CLASS_NAME;

	// グラフの大きさ
	static constexpr int GRAPH_WIDTH  = 300;
	static constexpr int GRAPH_HEIGHT = 200;

	bool EnablePlugin(bool fEnable);
	void DrawGraph(Gdiplus::Graphics &Graphics, int Width, int Height);
	void OnPaint(HWND hwnd);
	void FreeResources();
	void AdjustSignalLevelScale();
	void CreateDPIDependingResources();

	static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);
	static CSignalGraph * GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	template<typename T> static inline void SafeDelete(T *&p)
	{
		if (p != nullptr) {
			delete p;
			p = nullptr;
		}
	}
};


// ウィンドウクラス名
const LPCTSTR CSignalGraph::WINDOW_CLASS_NAME = TEXT("TVTest Signal Graph Window");


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
	// アイコンを登録
	m_pApp->RegisterPluginIconFromResource(g_hinstDLL, MAKEINTRESOURCE(IDB_ICON));

	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(EventCallback, this);

	return true;
}


// 終了処理
bool CSignalGraph::Finalize()
{
	// ウィンドウの破棄
	if (m_hwnd != nullptr)
		::DestroyWindow(m_hwnd);

	// 設定を保存
	if (m_fInitialized) {
		TCHAR szIniFileName[MAX_PATH];

		::GetModuleFileName(g_hinstDLL, szIniFileName, _countof(szIniFileName));
		::PathRenameExtension(szIniFileName, TEXT(".ini"));

		struct IntString {
			IntString(int Value) { ::StringCchPrintf(m_szBuffer, _countof(m_szBuffer), TEXT("%d"), Value); }
			operator LPCTSTR() const { return m_szBuffer; }
			TCHAR m_szBuffer[16];
		};

		::WritePrivateProfileString(
			TEXT("Settings"), TEXT("WindowLeft"),
			IntString(m_WindowPosition.Left), szIniFileName);
		::WritePrivateProfileString(
			TEXT("Settings"), TEXT("WindowTop"),
			IntString(m_WindowPosition.Top), szIniFileName);
		::WritePrivateProfileString(
			TEXT("Settings"), TEXT("WindowWidth"),
			IntString(m_WindowPosition.Width), szIniFileName);
		::WritePrivateProfileString(
			TEXT("Settings"), TEXT("WindowHeight"),
			IntString(m_WindowPosition.Height), szIniFileName);
	}

	return true;
}


// プラグインの有効/無効の切り替え
bool CSignalGraph::EnablePlugin(bool fEnable)
{
	if (fEnable) {
		// ウィンドウクラスの登録
		if (!m_fInitialized) {
			WNDCLASS wc;

			wc.style = CS_HREDRAW | CS_VREDRAW;
			wc.lpfnWndProc = WndProc;
			wc.cbClsExtra = 0;
			wc.cbWndExtra = 0;
			wc.hInstance = g_hinstDLL;
			wc.hIcon = nullptr;
			wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
			wc.hbrBackground = nullptr;
			wc.lpszMenuName = nullptr;
			wc.lpszClassName = WINDOW_CLASS_NAME;
			if (::RegisterClass(&wc) == 0)
				return false;

			// 設定の読み込み
			TCHAR szIniFileName[MAX_PATH];
			::GetModuleFileName(g_hinstDLL, szIniFileName, _countof(szIniFileName));
			::PathRenameExtension(szIniFileName, TEXT(".ini"));
			m_WindowPosition.Left =
				::GetPrivateProfileInt(TEXT("Settings"), TEXT("WindowLeft"), m_WindowPosition.Left, szIniFileName);
			m_WindowPosition.Top =
				::GetPrivateProfileInt(TEXT("Settings"), TEXT("WindowTop"), m_WindowPosition.Top, szIniFileName);
			m_WindowPosition.Width =
				::GetPrivateProfileInt(TEXT("Settings"), TEXT("WindowWidth"), m_WindowPosition.Width, szIniFileName);
			m_WindowPosition.Height =
				::GetPrivateProfileInt(TEXT("Settings"), TEXT("WindowHeight"), m_WindowPosition.Height, szIniFileName);

			m_fInitialized = true;
		}

		if (m_hwnd == nullptr) {
			constexpr DWORD Style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
			constexpr DWORD ExStyle = WS_EX_TOOLWINDOW;

			// プライマリモニタの DPI を取得
			m_DPI = m_pApp->GetDPIFromPoint(0, 0);
			if (m_DPI == 0)
				m_DPI = 96;

			// デフォルトのウィンドウサイズを取得
			if (m_WindowPosition.Width <= 0 || m_WindowPosition.Height <= 0) {
				int Width = GRAPH_WIDTH, Height = GRAPH_HEIGHT;
				// DPI設定に合わせてスケーリング
				if (m_DPI != 96) {
					Width = ::MulDiv(Width, m_DPI, 96);
					Height = ::MulDiv(Height, m_DPI, 96);
				}
				RECT rc = {0, 0, Width, Height};
				::AdjustWindowRectEx(&rc, Style, FALSE, ExStyle);
				if (m_WindowPosition.Width <= 0)
					m_WindowPosition.Width = rc.right - rc.left;
				if (m_WindowPosition.Height <= 0)
					m_WindowPosition.Height = rc.bottom - rc.top;
			}

			// ウィンドウの作成
			if (::CreateWindowEx(
					ExStyle, WINDOW_CLASS_NAME, TEXT("Signal Graph"), Style,
					0, 0, m_WindowPosition.Width, m_WindowPosition.Height,
					m_pApp->GetAppWindow(), nullptr, g_hinstDLL, this) == nullptr)
				return false;

			// ウィンドウ位置の復元
			WINDOWPLACEMENT wp;
			wp.length = sizeof(WINDOWPLACEMENT);
			::GetWindowPlacement(m_hwnd, &wp);
			wp.flags = 0;
			wp.showCmd = SW_SHOWNOACTIVATE;
			wp.rcNormalPosition.left = m_WindowPosition.Left;
			wp.rcNormalPosition.top = m_WindowPosition.Top;
			wp.rcNormalPosition.right = m_WindowPosition.Left + m_WindowPosition.Width;
			wp.rcNormalPosition.bottom = m_WindowPosition.Top + m_WindowPosition.Height;
			::SetWindowPlacement(m_hwnd, &wp);
		} else {
			AdjustSignalLevelScale();
			::ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
		}
		::UpdateWindow(m_hwnd);
	} else {
		if (m_hwnd != nullptr)
			::ShowWindow(m_hwnd, SW_HIDE);
	}

	return true;
}


// グラフを描画
void CSignalGraph::DrawGraph(Gdiplus::Graphics &Graphics, int Width, int Height)
{
	Graphics.Clear(m_BackColor);

	const float PenScale = (float)Height / (float)GRAPH_HEIGHT;

	// グリッドを描画
	if (m_pGridPen == nullptr)
		m_pGridPen = new Gdiplus::Pen(m_GridColor, PenScale);
	else
		m_pGridPen->SetWidth(PenScale);
	for (int i = 0; i < 10; i++) {
		float y = (float)(Height * (i + 1)) / 10.0f - 1.0f;
		Graphics.DrawLine(m_pGridPen, 0.0f, y, (float)Width, y);
	}

	const int ListSize = (int)m_List.size();
	const int NumPoints = std::min(ListSize, GRAPH_WIDTH);
	Gdiplus::PointF *pPoints = new Gdiplus::PointF[NumPoints + 3];
	const float XScale = (float)Width / (float)GRAPH_WIDTH;
	const float YScale = (float)(Height - 1);
	int i, x;

	// ビットレートを描画
	if (GRAPH_WIDTH > ListSize) {
		x = GRAPH_WIDTH - ListSize;
		i = 0;
	} else {
		x = 0;
		i = ListSize - GRAPH_WIDTH;
	}
	const float BitRateScale = YScale / (float)m_BitRateScale;
	for (int j = 0; i < ListSize; i++, j++) {
		float y = (float)std::min(m_List[i].BitRate, m_BitRateScale) * BitRateScale;
		pPoints[j].X = (float)x * XScale;
		if (j == 0) {
			pPoints[0].Y = (float)Height;
			pPoints[1].X = pPoints[0].X;
			j++;
		}
		pPoints[j].Y = YScale - y;
		x++;
	}
	pPoints[NumPoints + 1] = pPoints[NumPoints];
	pPoints[NumPoints + 1].X += XScale;
	pPoints[NumPoints + 2].X = pPoints[NumPoints + 1].X;
	pPoints[NumPoints + 2].Y = (float)Height;
	Gdiplus::GraphicsPath Path(Gdiplus::FillModeAlternate);
	Path.AddLines(pPoints, NumPoints + 3);
	if (m_pBrush == nullptr)
		m_pBrush = new Gdiplus::SolidBrush(m_BitRateColor);
	else
		m_pBrush->SetColor(m_BitRateColor);
	Graphics.FillPath(m_pBrush, &Path);
	if (m_pBitRatePen == nullptr) {
		m_pBitRatePen = new Gdiplus::Pen(m_BitRateColor, PenScale);
		m_pBitRatePen->SetStartCap(Gdiplus::LineCapRound);
		m_pBitRatePen->SetEndCap(Gdiplus::LineCapRound);
		m_pBitRatePen->SetLineJoin(Gdiplus::LineJoinMiter);
	} else {
		m_pBitRatePen->SetWidth(PenScale);
	}
	Graphics.DrawLines(m_pBitRatePen, &pPoints[1], NumPoints + 1);

	// 信号レベルを描画
	if (GRAPH_WIDTH > ListSize) {
		x = GRAPH_WIDTH - ListSize;
		i = 0;
	} else {
		x = 0;
		i = ListSize - GRAPH_WIDTH;
	}
	const float SignalLevelScale = YScale / m_ActualSignalLevelScale;
	for (int j = 0; i < ListSize; i++, j++) {
		float y = std::min(m_List[i].SignalLevel, m_ActualSignalLevelScale) * SignalLevelScale;
		pPoints[j].X = (float)x * XScale;
		pPoints[j].Y = YScale - y;
		x++;
	}
	pPoints[NumPoints] = pPoints[NumPoints - 1];
	pPoints[NumPoints].X += XScale;
	if (m_pSignalLevelPen == nullptr) {
		m_pSignalLevelPen = new Gdiplus::Pen(m_SignalLevelColor, PenScale);
		m_pSignalLevelPen->SetStartCap(Gdiplus::LineCapRound);
		m_pSignalLevelPen->SetEndCap(Gdiplus::LineCapRound);
		m_pSignalLevelPen->SetLineJoin(Gdiplus::LineJoinMiter);
	} else {
		m_pSignalLevelPen->SetWidth(PenScale);
	}
	Graphics.DrawLines(m_pSignalLevelPen, pPoints, NumPoints + 1);

	delete [] pPoints;

	// ビットレートと信号レベルの文字列を描画
	const Gdiplus::REAL FontHeight = m_pFont->GetHeight(96.0f);
	const Gdiplus::REAL TextMargin = (Gdiplus::REAL)std::floor(FontHeight / 8.0f);
	Gdiplus::PointF Pos(TextMargin, TextMargin);
	const SignalInfo &Info = m_List.back();
	WCHAR szText[64];
	::StringCchPrintf(szText, _countof(szText), L"%.2f dB", Info.SignalLevel);
	m_pBrush->SetColor(Gdiplus::Color(m_SignalLevelColor.GetValue() | 0xFF000000U));
	Graphics.DrawString(szText, -1, m_pFont, Pos, m_pBrush);
	::StringCchPrintf(szText, _countof(szText), L"%.2f Mbps", (double)Info.BitRate / (double)(1000 * 1000));
	Pos.Y += FontHeight;
	m_pBrush->SetColor(Gdiplus::Color(m_BitRateColor.GetValue() | 0xFF000000U));
	Graphics.DrawString(szText, -1, m_pFont, Pos, m_pBrush);
}


// WM_PAINT の処理
void CSignalGraph::OnPaint(HWND hwnd)
{
	PAINTSTRUCT ps;
	RECT rcClient;

	::BeginPaint(hwnd, &ps);
	::GetClientRect(hwnd, &rcClient);

	if (m_pOffscreenImage == nullptr
			|| m_pOffscreenImage->GetWidth() < (UINT)rcClient.right
			|| m_pOffscreenImage->GetHeight() < (UINT)rcClient.bottom) {
		SafeDelete(m_pOffscreen);
		delete m_pOffscreenImage;
		m_pOffscreenImage = new Gdiplus::Bitmap(rcClient.right, rcClient.bottom, PixelFormat32bppPARGB);
		if (m_pOffscreenImage != nullptr)
			m_pOffscreen = new Gdiplus::Graphics(m_pOffscreenImage);
	}

	if (m_pFont == nullptr)
		m_pFont = new Gdiplus::Font(ps.hdc, &m_Font);

	const Gdiplus::Rect PaintRect(
		ps.rcPaint.left, ps.rcPaint.top,
		ps.rcPaint.right - ps.rcPaint.left,
		ps.rcPaint.bottom - ps.rcPaint.top);

	if (m_pOffscreen != nullptr) {
		m_pOffscreen->SetPageUnit(Gdiplus::UnitPixel);
		m_pOffscreen->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
		m_pOffscreen->SetTextRenderingHint(Gdiplus::TextRenderingHintSystemDefault);
		m_pOffscreen->SetClip(PaintRect);

		DrawGraph(*m_pOffscreen, rcClient.right, rcClient.bottom);

		Gdiplus::Graphics Graphics(ps.hdc);

		Graphics.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
		Graphics.DrawImage(
			m_pOffscreenImage,
			PaintRect.X, PaintRect.Y,
			PaintRect.X, PaintRect.Y,
			PaintRect.Width, PaintRect.Height,
			Gdiplus::UnitPixel);
	} else {
		Gdiplus::Graphics Graphics(ps.hdc);

		Graphics.SetClip(PaintRect);
		DrawGraph(Graphics, rcClient.right, rcClient.bottom);
	}

	::EndPaint(hwnd, &ps);
}


// リソースを解放
void CSignalGraph::FreeResources()
{
	SafeDelete(m_pGridPen);
	SafeDelete(m_pSignalLevelPen);
	SafeDelete(m_pBitRatePen);
	SafeDelete(m_pBrush);
	SafeDelete(m_pFont);
	SafeDelete(m_pOffscreen);
	SafeDelete(m_pOffscreenImage);
}


// 信号レベルのスケールを調整
void CSignalGraph::AdjustSignalLevelScale()
{
	double MaxLevel = 0.0;

	for (auto it = m_List.begin(); it != m_List.end(); ++it) {
		if (it->SignalLevel > MaxLevel)
			MaxLevel = it->SignalLevel;
	}

	if (m_SignalLevelScale > MaxLevel)
		m_ActualSignalLevelScale = m_SignalLevelScale;
	else
		m_ActualSignalLevelScale = (float)std::ceil(MaxLevel * 0.1f) * 10.0f;
}


// DPI に依存したリソースを作成する
void CSignalGraph::CreateDPIDependingResources()
{
	// フォントを取得
	m_pApp->GetFont(L"StatusBarFont", &m_Font, m_DPI);

	SafeDelete(m_pFont);
}


// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CSignalGraph::EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
{
	CSignalGraph *pThis = static_cast<CSignalGraph *>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// プラグインの有効状態が変化した
		return pThis->EnablePlugin(lParam1 != 0);

	case TVTest::EVENT_STANDBY:
		// 待機状態が変化した
		if (pThis->m_pApp->IsPluginEnabled()) {
			// 待機状態の時はウィンドウを隠す
			::ShowWindow(pThis->m_hwnd, lParam1 != 0 ? SW_HIDE : SW_SHOW);
		}
		return TRUE;

	case TVTest::EVENT_MAINWINDOWDARKMODECHANGED:
		// メインウィンドウのダークモード状態が変わった
		if (pThis->m_hwnd != nullptr) {
			// メインウィンドウに合わせてダークモード状態を変更する
			pThis->m_pApp->SetWindowDarkMode(
				pThis->m_hwnd,
				(pThis->m_pApp->GetDarkModeStatus() & TVTest::DARK_MODE_STATUS_MAINWINDOW_DARK) != 0);
		}
		return TRUE;
	}

	return 0;
}


// ウィンドウハンドルからthisを取得する
CSignalGraph * CSignalGraph::GetThis(HWND hwnd)
{
	return reinterpret_cast<CSignalGraph *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
}


// ウィンドウプロシージャ
LRESULT CALLBACK CSignalGraph::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
			CSignalGraph *pThis = static_cast<CSignalGraph *>(pcs->lpCreateParams);

			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
			pThis->m_hwnd = hwnd;

			pThis->CreateDPIDependingResources();

			pThis->m_ActualSignalLevelScale = pThis->m_SignalLevelScale;

			pThis->m_List.clear();
			pThis->m_List.push_back(SignalInfo());

			// メインウィンドウがダークモードであればそれに合わせてダークモードにする
			if (pThis->m_pApp->GetDarkModeStatus() & TVTest::DARK_MODE_STATUS_MAINWINDOW_DARK)
				pThis->m_pApp->SetWindowDarkMode(hwnd, true);

			// 更新用タイマの設定
			::SetTimer(hwnd, 1, 1000, nullptr);
		}
		return 0;

	case WM_PAINT:
		{
			CSignalGraph *pThis = GetThis(hwnd);

			pThis->OnPaint(hwnd);
		}
		return 0;

	case WM_TIMER:
		{
			CSignalGraph *pThis = GetThis(hwnd);

			// 情報取得&表示更新
			TVTest::StatusInfo Status;
			pThis->m_pApp->GetStatus(&Status);
			if (Status.SignalLevel < 0.0f)
				Status.SignalLevel = 0.0f;
			if (pThis->m_List.size() == GRAPH_WIDTH)
				pThis->m_List.pop_front();
			pThis->m_List.push_back(SignalInfo(Status.SignalLevel, Status.BitRate));

			if (pThis->m_ActualSignalLevelScale < Status.SignalLevel)
				pThis->m_ActualSignalLevelScale = (float)std::ceil(Status.SignalLevel * 0.1f) * 10.0f;

			::InvalidateRect(hwnd, nullptr, FALSE);
		}
		return 0;

	case WM_SYSCOMMAND:
		if ((wParam & 0xFFF0) == SC_CLOSE) {
			// 閉じる時はプラグインを無効にする
			CSignalGraph *pThis = GetThis(hwnd);

			pThis->m_pApp->EnablePlugin(false);
			return TRUE;
		}
		break;

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif
	case WM_DPICHANGED:
		// DPI が変わった
		{
			CSignalGraph *pThis = GetThis(hwnd);
			const RECT *prc = reinterpret_cast<const RECT *>(lParam);

			pThis->m_DPI = HIWORD(wParam);

			pThis->CreateDPIDependingResources();

			::SetWindowPos(
				hwnd, nullptr,
				prc->left, prc->top,
				prc->right - prc->left, prc->bottom - prc->top,
				SWP_NOZORDER | SWP_NOACTIVATE);
			::InvalidateRect(hwnd, nullptr, FALSE);
		}
		break;

	case WM_DESTROY:
		{
			CSignalGraph *pThis = GetThis(hwnd);

			pThis->FreeResources();

			// ウィンドウ位置保存
			WINDOWPLACEMENT wp;
			wp.length = sizeof (WINDOWPLACEMENT);
			if (::GetWindowPlacement(hwnd, &wp)) {
				pThis->m_WindowPosition.Left = wp.rcNormalPosition.left;
				pThis->m_WindowPosition.Top = wp.rcNormalPosition.top;
				pThis->m_WindowPosition.Width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
				pThis->m_WindowPosition.Height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
			}

			pThis->m_hwnd = nullptr;
		}
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}




// プラグインクラスのインスタンスを生成する
TVTest::CTVTestPlugin * CreatePluginClass()
{
	return new CSignalGraph;
}
