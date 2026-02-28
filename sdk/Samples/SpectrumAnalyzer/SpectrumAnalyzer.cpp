/*
	TVTest プラグインサンプル

	スペクトラム・アナライザー

	fft4g.c は http://www.kurims.kyoto-u.ac.jp/~ooura/fft-j.html で配布されて
	いるもので、大浦拓哉氏に著作権があります。
	Copyright Takuya OOURA, 1996-2001

	このサンプルでは主に以下の機能を実装しています。

	・音声サンプルを取得する
	・ウィンドウを表示する
	・TVTest に合わせてウィンドウをダークモードにする
*/


#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <objbase.h>
#include <shlwapi.h>
#include <algorithm>
#define _USE_MATH_DEFINES
#include <cmath>

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


// fft4g.c
extern "C" void rdft(int n, int isgn, double *a, int *ip, double *w);


// プラグインクラス
class CSpectrumAnalyzer : public TVTest::CTVTestPlugin
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

	class CCriticalLock
	{
	public:
		CCriticalLock() {
			::InitializeCriticalSection(&m_CriticalSection);
		}
		virtual ~CCriticalLock() {
			::DeleteCriticalSection(&m_CriticalSection);
		}
		void Lock() {
			::EnterCriticalSection(&m_CriticalSection);
		}
		void Unlock() {
			::LeaveCriticalSection(&m_CriticalSection);
		}

	private:
		CRITICAL_SECTION m_CriticalSection;
	};

	// FFT 処理を行うサンプル数
	static constexpr int FFT_SIZE_BITS = 10;
	static constexpr int FFT_SIZE = 1 << FFT_SIZE_BITS;
	// 帯域分割数
	static constexpr int NUM_BANDS = 20;
	// レベル分割数
	static constexpr int NUM_LEVELS = 30;

	bool m_fInitialized = false;
	HWND m_hwnd = nullptr;
	Position m_WindowPosition;
	int m_DPI;
	Gdiplus::Color m_BackColor{255, 0, 0, 0};
	Gdiplus::Color m_SpectrumColor1{255, 0, 224, 0};
	Gdiplus::Color m_SpectrumColor2{255, 255, 128, 0};
	Gdiplus::SolidBrush *m_pBrush = nullptr;
	Gdiplus::Graphics *m_pOffscreen = nullptr;
	Gdiplus::Bitmap *m_pOffscreenImage = nullptr;

	short *m_pSampleBuffer = nullptr;
	double *m_pFFTBuffer = nullptr;
	int *m_pFFTWorkBuffer = nullptr;
	double *m_pFFTSinTable = nullptr;
	DWORD m_BufferUsed = 0;
	DWORD m_BufferPos = 0;
	double *m_pPower = nullptr;
	struct {
		double Real;
		double Delayed;
	} m_PeakList[NUM_BANDS];
	CCriticalLock m_FFTLock;

	static const LPCTSTR WINDOW_CLASS_NAME;

	bool EnablePlugin(bool fEnable);
	void DrawSpectrum(Gdiplus::Graphics &Graphics, int Width, int Height);
	void OnPaint(HWND hwnd);
	void UpdateSpectrum();

	static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);
	static LRESULT CALLBACK AudioCallback(short *pData, DWORD Samples, int Channels, void *pClientData);
	static CSpectrumAnalyzer * GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	template<typename T> static inline void SafeDelete(T *&p)
	{
		if (p != nullptr) {
			delete p;
			p = nullptr;
		}
	}

	template<typename T> static inline void SafeDeleteArray(T *&p)
	{
		if (p != nullptr) {
			delete [] p;
			p = nullptr;
		}
	}

	static inline double Pow2(double v) { return std::pow(v, 2.0); }
};


// ウィンドウクラス名
const LPCTSTR CSpectrumAnalyzer::WINDOW_CLASS_NAME = TEXT("TVTest Spectrum Analyzer Window");


// プラグインの情報を返す
bool CSpectrumAnalyzer::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"Spectrum Analyzer";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"スペクトラム・アナライザー";
	return true;
}


// 初期化処理
bool CSpectrumAnalyzer::Initialize()
{
	// アイコンを登録
	m_pApp->RegisterPluginIconFromResource(g_hinstDLL, MAKEINTRESOURCE(IDB_ICON));

	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(EventCallback, this);

	return true;
}


// 終了処理
bool CSpectrumAnalyzer::Finalize()
{
	// ウィンドウが作成されていたら破棄する
	if (m_hwnd != nullptr)
		::DestroyWindow(m_hwnd);

	// 設定を保存
	if (m_fInitialized) {
		TCHAR szIniFileName[MAX_PATH];

		::GetModuleFileName(g_hinstDLL, szIniFileName, _countof(szIniFileName));
		::PathRenameExtension(szIniFileName, TEXT(".ini"));

		struct IntString {
			IntString(int Value) { ::wsprintf(m_szBuffer, TEXT("%d"), Value); }
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
bool CSpectrumAnalyzer::EnablePlugin(bool fEnable)
{
	if (fEnable) {
		if (!m_fInitialized) {
			// ウィンドウクラス登録
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

		// ウィンドウを作成する
		if (m_hwnd == nullptr) {
			constexpr DWORD Style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
			constexpr DWORD ExStyle = WS_EX_TOOLWINDOW;

			// プライマリモニタの DPI を取得
			m_DPI = m_pApp->GetDPIFromPoint(0, 0);
			if (m_DPI == 0)
				m_DPI = 96;

			// デフォルトのウィンドウサイズを取得
			if (m_WindowPosition.Width <= 0 || m_WindowPosition.Height <= 0) {
				int Width = NUM_BANDS * 10, Height = NUM_LEVELS * 4;
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
					ExStyle, WINDOW_CLASS_NAME, TEXT("Spectrum Analyzer"), Style,
					0, 0, m_WindowPosition.Width, m_WindowPosition.Height,
					m_pApp->GetAppWindow(), nullptr, g_hinstDLL, this) == nullptr)
				return false;
		}

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
		::UpdateWindow(m_hwnd);
	} else {
		if (m_hwnd != nullptr)
			::DestroyWindow(m_hwnd);
	}

	return true;
}


// スペクトラムを描画
void CSpectrumAnalyzer::DrawSpectrum(Gdiplus::Graphics &Graphics, int Width, int Height)
{
	Graphics.Clear(m_BackColor);

	const int BarWidth = std::max(Width / NUM_BANDS, 4);
	const int CellHeight = std::max(Height / NUM_LEVELS, 3);
	const int BaseX = (Width - BarWidth * NUM_BANDS) / 2;
	const int BaseY = (Height - CellHeight * NUM_LEVELS) / 2;

	if (m_pBrush == nullptr)
		m_pBrush = new Gdiplus::SolidBrush(m_SpectrumColor1);

	for (int i = 0; i < NUM_BANDS; i++) {
		int Level = (int)(m_PeakList[i].Delayed * ((double)NUM_LEVELS / 40.0) + 0.5);
		if (Level > 0) {
			if (Level > NUM_LEVELS)
				Level = NUM_LEVELS;

			Gdiplus::Rect Rect;
			Rect.X = BaseX + i * BarWidth + 1;
			Rect.Y = BaseY + Height - CellHeight + 1;
			Rect.Width = BarWidth - 2;
			Rect.Height = CellHeight - 2;

			for (int j = 0; j < Level; j++) {
				const int Ratio = NUM_LEVELS - j;
				int A = ((m_SpectrumColor1.GetA() * Ratio) + (m_SpectrumColor2.GetA() * j)) / NUM_LEVELS;
				m_pBrush->SetColor(
					Gdiplus::Color(
						A,
						((m_SpectrumColor1.GetR() * Ratio) + (m_SpectrumColor2.GetR() * j)) / NUM_LEVELS,
						((m_SpectrumColor1.GetG() * Ratio) + (m_SpectrumColor2.GetG() * j)) / NUM_LEVELS,
						((m_SpectrumColor1.GetB() * Ratio) + (m_SpectrumColor2.GetB() * j)) / NUM_LEVELS));
				Graphics.FillRectangle(m_pBrush, Rect);
				Rect.Y -= CellHeight;
			}
		}
	}
}


// WM_PAINT の処理
void CSpectrumAnalyzer::OnPaint(HWND hwnd)
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

	const Gdiplus::Rect PaintRect(
		ps.rcPaint.left, ps.rcPaint.top,
		ps.rcPaint.right - ps.rcPaint.left,
		ps.rcPaint.bottom - ps.rcPaint.top);

	if (m_pOffscreen != nullptr) {
		m_pOffscreen->SetPageUnit(Gdiplus::UnitPixel);
		m_pOffscreen->SetClip(PaintRect);

		DrawSpectrum(*m_pOffscreen, rcClient.right, rcClient.bottom);

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
		DrawSpectrum(Graphics, rcClient.right, rcClient.bottom);
	}

	::EndPaint(hwnd, &ps);
}


// スペクトラム更新
void CSpectrumAnalyzer::UpdateSpectrum()
{
	m_FFTLock.Lock();
	m_pPower[0] = Pow2(m_pFFTBuffer[0]);
	for (int i = 1; i < FFT_SIZE / 2; i++) {
		m_pPower[i] = Pow2(m_pFFTBuffer[i * 2]) +
		              Pow2(m_pFFTBuffer[i * 2 + 1]);
	}
	m_pPower[FFT_SIZE / 2] = Pow2(m_pFFTBuffer[1]);
	m_FFTLock.Unlock();

	int j1 = 0;
	for (int i = 0; i < NUM_BANDS; i++) {
		double Peak = 0.0;
		int j2 = (int)std::pow(2.0, (double)i * (double)(FFT_SIZE_BITS - 2) / (double)(NUM_BANDS - 1));
		/*
		if (j2 > FFT_SIZE / 2)
			j2 = FFT_SIZE / 2;
		else
		*/
		if (j2 <= j1)
			j2 = j1 + 1;
		for (; j1 < j2; j1++) {
			if (m_pPower[j1] > Peak)
				Peak = m_pPower[j1];
		}
		Peak = std::log10(Peak) * 10.0;
		m_PeakList[i].Real = Peak;
		if (Peak < m_PeakList[i].Delayed)
			Peak = std::max(m_PeakList[i].Delayed - 2.5, Peak);
		m_PeakList[i].Delayed = Peak;
	}
}


// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CSpectrumAnalyzer::EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
{
	CSpectrumAnalyzer *pThis = static_cast<CSpectrumAnalyzer *>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// プラグインの有効状態が変化した
		pThis->EnablePlugin(lParam1 != 0);
		return TRUE;

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


// 音声コールバック関数
LRESULT CALLBACK CSpectrumAnalyzer::AudioCallback(short *pData, DWORD Samples, int Channels, void *pClientData)
{
	CSpectrumAnalyzer *pThis = static_cast<CSpectrumAnalyzer *>(pClientData);

	DWORD BufferUsed = pThis->m_BufferUsed;
	DWORD j = pThis->m_BufferPos;
	for (DWORD i = 0; i < Samples; i++) {
		if (j == FFT_SIZE)
			j = 0;
		switch (Channels) {
		case 2:
		case 6:
			pThis->m_pSampleBuffer[j] = (short)((pData[i * Channels] + pData[i * Channels + 1]) / 2);
			break;
		default:
			pThis->m_pSampleBuffer[j] = pData[i * Channels];
			break;
		}
		j++;
		BufferUsed++;

		if (BufferUsed == FFT_SIZE) {
			pThis->m_FFTLock.Lock();
			// doubleに変換
			for (int k = 0; k < FFT_SIZE; k++) {
				pThis->m_pFFTBuffer[k]=
					(double)pThis->m_pSampleBuffer[(j + k) % FFT_SIZE] / 32768.0;
			}

			// FFT処理実行
			rdft(FFT_SIZE, 1, pThis->m_pFFTBuffer, pThis->m_pFFTWorkBuffer, pThis->m_pFFTSinTable);
			pThis->m_FFTLock.Unlock();

			BufferUsed = 0;
		}
	}

	pThis->m_BufferUsed = BufferUsed;
	pThis->m_BufferPos = j;

	return 0;
}


CSpectrumAnalyzer * CSpectrumAnalyzer::GetThis(HWND hwnd)
{
	// ウィンドウハンドルからthisを取得
	return reinterpret_cast<CSpectrumAnalyzer *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
}


LRESULT CALLBACK CSpectrumAnalyzer::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
			CSpectrumAnalyzer *pThis = static_cast<CSpectrumAnalyzer *>(pcs->lpCreateParams);

			// ウィンドウハンドルからthisを取得できるようにする
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
			pThis->m_hwnd = hwnd;

			// メモリ確保
			pThis->m_pSampleBuffer = new short[FFT_SIZE];
			pThis->m_pFFTBuffer = new double[FFT_SIZE];
			for (int i = 0; i < FFT_SIZE; i++)
				pThis->m_pFFTBuffer[i] = 0.0;
			pThis->m_pFFTWorkBuffer = new int[2 + (int)std::sqrt((double)(FFT_SIZE / 2)) + 10];
			pThis->m_pFFTWorkBuffer[0] = 0;
			pThis->m_pFFTSinTable = new double[FFT_SIZE / 2];
			pThis->m_pPower = new double[FFT_SIZE / 2 + 1];

			for (int i = 0; i < NUM_BANDS; i++) {
				pThis->m_PeakList[i].Real = 0.0;
				pThis->m_PeakList[i].Delayed = 0.0;
			}

			pThis->m_BufferUsed = 0;
			pThis->m_BufferPos = 0;

			// メインウィンドウがダークモードであればそれに合わせてダークモードにする
			if (pThis->m_pApp->GetDarkModeStatus() & TVTest::DARK_MODE_STATUS_MAINWINDOW_DARK)
				pThis->m_pApp->SetWindowDarkMode(hwnd, true);

			// 音声コールバックを登録
			pThis->m_pApp->SetAudioCallback(AudioCallback, pThis);

			// 表示更新用タイマの設定
			::SetTimer(hwnd, 1, 200, nullptr);
		}
		return TRUE;

	case WM_PAINT:
		{
			CSpectrumAnalyzer *pThis = GetThis(hwnd);

			pThis->OnPaint(hwnd);
		}
		return 0;

	case WM_TIMER:
		{
			CSpectrumAnalyzer *pThis = GetThis(hwnd);

			pThis->UpdateSpectrum();
			::InvalidateRect(hwnd, nullptr, FALSE);
		}
		return 0;

	case WM_SYSCOMMAND:
		if ((wParam & 0xFFF0) == SC_CLOSE) {
			// 閉じる時はプラグインを無効にする
			CSpectrumAnalyzer *pThis = GetThis(hwnd);

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
			CSpectrumAnalyzer *pThis = GetThis(hwnd);
			const RECT *prc = reinterpret_cast<const RECT *>(lParam);

			pThis->m_DPI = HIWORD(wParam);

			::SetWindowPos(
				hwnd, nullptr,
				prc->left, prc->top,
				prc->right - prc->left, prc->bottom - prc->top,
				SWP_NOZORDER | SWP_NOACTIVATE);
		}
		break;

	case WM_DESTROY:
		{
			CSpectrumAnalyzer *pThis = GetThis(hwnd);

			// 音声コールバックを登録解除
			pThis->m_pApp->SetAudioCallback(nullptr);

			// リソース解放
			SafeDelete(pThis->m_pBrush);
			SafeDelete(pThis->m_pOffscreen);
			SafeDelete(pThis->m_pOffscreenImage);

			// メモリ開放
			SafeDeleteArray(pThis->m_pSampleBuffer);
			SafeDeleteArray(pThis->m_pFFTBuffer);
			SafeDeleteArray(pThis->m_pFFTWorkBuffer);
			SafeDeleteArray(pThis->m_pFFTSinTable);
			SafeDeleteArray(pThis->m_pPower);

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
	return new CSpectrumAnalyzer;
}
