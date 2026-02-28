/*
	TVTest プラグインサンプル

	簡易イコライザー

	このサンプルでは主に以下の機能を実装しています。

	・音声サンプルを取得・改変する
	・ウィンドウを表示する
	・配色を取得し、配色の変更に追従する
	・DPI に応じてスケーリングする
	・TVTest に合わせてウィンドウをダークモードにする
*/


#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <tchar.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <algorithm>
#include <crtdbg.h>
#if defined(_MSC_VER) && defined(_M_X64)
#include <emmintrin.h>
#endif

#define TVTEST_PLUGIN_CLASS_IMPLEMENT // クラスとして実装
#include "TVTestPlugin.h"
#include "resource.h"

#pragma comment(lib, "shlwapi.lib")


static inline int RoundToInt(double Value)
{
#if defined(_MSC_VER) && defined(_M_X64)
	return _mm_cvtsd_si32(_mm_load_sd(&Value));
#elif defined(_MSC_VER) && defined(_M_IX86)
	int i;
	__asm
	{
		fld Value;
		fistp i;
	}
	return i;
#elif !defined(_MSC_VER) || (_MSC_VER >= 1800)
	return std::lround(Value);
#else
	return (int)(value + (value >= 0.0 ? 0.5 : -0.5));
#endif
}


// バンドパスフィルタクラス
class CBandPass
{
	static constexpr int MAX_CHANNELS = 6;   // 最大チャンネル数(5.1ch)
	static constexpr int MAX_FREQUENCY = 16; // 最大周波数分割数

	double m_Coef[MAX_FREQUENCY * MAX_CHANNELS];
	double m_Ener[MAX_FREQUENCY * MAX_CHANNELS];
	double m_Volume[MAX_FREQUENCY * MAX_CHANNELS];
	int m_EqualizerCount = 0;
	double m_PreAmplifier;
	CRITICAL_SECTION m_Lock;

public:
	CBandPass();
	~CBandPass();
	void Initialize(const double *pFreqTable, int Count, int Frequency);
	void Reset();
	void SetVolume(int Index, double Volume);
	void SetPreAmplifier(double Volume);
	void ProcessSamples(short *pData, DWORD Samples, int Channels);
};


CBandPass::CBandPass()
{
	::InitializeCriticalSection(&m_Lock);
}


CBandPass::~CBandPass()
{
	::DeleteCriticalSection(&m_Lock);
}


void CBandPass::Initialize(const double *pFreqTable, int Count, int Frequency)
{
	_ASSERT(Count < MAX_FREQUENCY);
	double dt = 1.0 / (double)Frequency;

	m_EqualizerCount = Count;
	for (int i = 0; i < Count; i++) {
		double Coef = dt / (1.0 / (2.0 * M_PI * pFreqTable[i]));
		for (int j = 0; j < MAX_CHANNELS; j++)
			m_Coef[j * MAX_FREQUENCY + i] = Coef;
	}
	for (int i = 0; i < MAX_FREQUENCY * MAX_CHANNELS; i++) {
		m_Ener[i] = 0.0;
		m_Volume[i] = 1.0;
	}
	m_PreAmplifier = 1.0;
}


void CBandPass::Reset()
{
	::EnterCriticalSection(&m_Lock);
	for (int i = 0; i < MAX_FREQUENCY * MAX_CHANNELS; i++)
		m_Ener[i] = 0.0;
	::LeaveCriticalSection(&m_Lock);
}


void CBandPass::SetVolume(int Index, double Volume)
{
	_ASSERT(Index >= 0 && Index <= m_EqualizerCount);
	::EnterCriticalSection(&m_Lock);
	for (int i = 0; i < MAX_CHANNELS; i++)
		m_Volume[i * MAX_FREQUENCY + Index] = Volume;
	::LeaveCriticalSection(&m_Lock);
}


void CBandPass::SetPreAmplifier(double Volume)
{
	m_PreAmplifier = Volume;
}


void CBandPass::ProcessSamples(short *pData, DWORD Samples, int Channels)
{
	short *p = pData;

	::EnterCriticalSection(&m_Lock);
	for (DWORD i = 0; i < Samples; i++) {
		for (int j = 0; j < Channels; j++) {
			double Value = (double)*p * m_PreAmplifier;
			int eq;
			double u[MAX_FREQUENCY];

			eq = j * MAX_FREQUENCY;
			for (int k = 0; k < m_EqualizerCount; k++, eq++) {
				double e = m_Ener[eq];
				m_Ener[eq] += (Value - e) * m_Coef[eq];
				u[k] = e;
				Value -= e;
			}
			Value *= m_Volume[eq];
			eq = j * MAX_FREQUENCY;
			for (int k = 0; k < m_EqualizerCount; k++)
				Value += u[k] * m_Volume[eq++];

			int v = RoundToInt(Value);
			if (v > 32767)
				v = 32767;
			else if (v < -32768)
				v = -32768;
			*p++ = (short)v;
		}
	}
	::LeaveCriticalSection(&m_Lock);
}




// プラグインクラス
class CEqualizer : public TVTest::CTVTestPlugin
{
public:
	CEqualizer();
	bool GetPluginInfo(TVTest::PluginInfo *pInfo) override;
	bool Initialize() override;
	bool Finalize() override;

private:
	// コマンド
	enum {
		COMMAND_SHOW  = 1, // 表示/非表示
		COMMAND_ONOFF = 2  // On/Off
	};

	static constexpr int NUM_FREQUENCY = 10;
	static constexpr int NUM_CUSTOM_PRESETS = 10;

	// 各部のサイズ(DIP単位)
	static constexpr int SLIDER_WIDTH         = 16;
	static constexpr int SLIDER_HEIGHT        = 80;
	static constexpr int SLIDER_MARGIN        = 4;
	static constexpr int SLIDER_PADDING       = 2;
	static constexpr int WINDOW_MARGIN        = 8;
	static constexpr int TEXT_HEIGHT          = 10;
	static constexpr int SLIDER_TEXT_MARGIN   = 3;
	static constexpr int SLIDER_BUTTON_MARGIN = 4;
	static constexpr int BUTTON_WIDTH         = 52;
	static constexpr int BUTTON_HEIGHT        = TEXT_HEIGHT + 8;
	static constexpr int BUTTON_MARGIN        = 4;
	static constexpr int LINE_WIDTH           = 1;

	enum {
		BUTTON_ENABLE,
		BUTTON_RESET,
		BUTTON_LOAD,
		BUTTON_SAVE,
		NUM_BUTTONS
	};

	struct EqualizerSettings
	{
		int PreAmplifier;
		int Frequency[NUM_FREQUENCY];
	};

	struct EqualizerPreset
	{
		LPCTSTR pszName;
		EqualizerSettings Setting;
	};

	TCHAR m_szIniFileName[MAX_PATH];
	bool m_fSettingsLoaded = false;
	bool m_fShowed = false;
	HWND m_hwnd = nullptr;
	COLORREF m_crBackColor;
	COLORREF m_crTextColor;
	int m_DPI;
	int m_SliderWidth;
	int m_SliderHeight;
	int m_SliderMargin;
	int m_SliderPadding;
	int m_WindowMargin;
	int m_TextHeight;
	int m_SliderTextMargin;
	int m_SliderButtonMargin;
	int m_ButtonWidth;
	int m_ButtonHeight;
	int m_ButtonMargin;
	int m_LineWidth;
	int m_ClientWidth;
	int m_ClientHeight;
	HFONT m_hfont = nullptr;
	POINT m_WindowPosition{};
	CBandPass m_BandPass;
	bool m_fEnabled = false;
	bool m_fEnableDefault = false;
	EqualizerSettings m_CurSettings;
	int m_CurSlider;
	EqualizerSettings m_CustomPresetList[NUM_CUSTOM_PRESETS];

	static const LPCTSTR WINDOW_CLASS_NAME;
	static const EqualizerPreset m_PresetList[];
	static const double m_FreqTable[NUM_FREQUENCY - 1];

	bool ReadPreset(LPCTSTR pszSection, LPCTSTR pszKeyName, EqualizerSettings *pSettings);
	bool WritePreset(LPCTSTR pszSection, LPCTSTR pszKeyName, const EqualizerSettings *pSettings) const;
	void LoadSettings();
	void SaveSettings() const;
	bool EnablePlugin(bool fEnable);
	void EnableEqualizer(bool fEnable);
	void ResetSettings();
	void ApplySettings();
	void CalcMetrics();
	void CreateDPIDependingResources();
	void GetSliderRect(int Index, RECT *pRect, bool fBar = false) const;
	int MapSliderPos(int y) const;
	int CalcSliderPos(int Pos) const;
	void GetButtonRect(int Button, RECT *pRect) const;
	void GetColor();
	void OnButtonPush(int Button);
	void Draw(HDC hdc, const RECT &rcPaint);
	void ScaleDPI(int *pValue) {
		int Value = ::MulDiv(*pValue, m_DPI, 96);
		if (Value < 1)
			Value = 1;
		*pValue = Value;
	}

	static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);
	static LRESULT CALLBACK AudioCallback(short *pData, DWORD Samples, int Channels, void *pClientData);
	static CEqualizer * GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};


const LPCTSTR CEqualizer::WINDOW_CLASS_NAME = TEXT("TVTest Equalizer Window");


const CEqualizer::EqualizerPreset CEqualizer::m_PresetList[] = {
	{TEXT("Accoustic"),         {0, { 4,  4,  3,  0,  1,  1,  3,  3,  3,  1}}},
	{TEXT("Bass Boost"),        {0, { 5,  4,  3,  2,  1,  0,  0,  0,  0,  0}}},
	{TEXT("Boost"),             {0, { 2,  5,  7,  5,  5,  4,  5,  7,  9,  6}}},
	{TEXT("Classical"),         {0, { 4,  3,  2,  2, -1, -1,  0,  1,  2,  3}}},
	{TEXT("Dance"),             {0, { 3,  5,  4,  0,  1,  3,  4,  3,  3,  0}}},
	{TEXT("Electronic"),        {0, { 3,  3,  1,  0, -1,  1,  0,  1,  3,  4}}},
	{TEXT("For Poor Speakers"), {0, { 4,  3,  3,  2,  1,  0, -1, -2, -2, -3}}},
	{TEXT("Hip-Hop"),           {0, { 4,  3,  1,  2,  0,  0,  1,  0,  1,  2}}},
	{TEXT("Jazz"),              {0, { 3,  2,  1,  1, -1, -1,  0,  1,  2,  3}}},
	{TEXT("Pop"),               {0, {-1,  0,  0,  1,  3,  3,  1,  0,  0, -1}}},
	{TEXT("R&&B"),              {0, { 2,  5,  4,  1, -1, -1,  2,  2,  2,  3}}},
	{TEXT("Rock"),              {0, { 4,  3,  2,  1,  0,  0,  0,  2,  3,  3}}},
	{TEXT("Treble Boost"),      {0, { 0,  0,  0,  0,  0,  1,  2,  3,  4,  5}}},
	{TEXT("Vocal"),             {0, {-1, -2, -2,  1,  3,  3,  2,  1,  0, -1}}},
	{TEXT("Voice"),             {0, {-2,  0,  0,  0,  2,  3,  4,  3,  2,  0}}},
};

const double CEqualizer::m_FreqTable[NUM_FREQUENCY-1] = {
	44.2, 88.4, 176.8, 353.6, 707.1, 1414.2, 2828.4, 5656.8, 11313.7
};


CEqualizer::CEqualizer()
{
	ResetSettings();

	for (int i = 0; i < NUM_CUSTOM_PRESETS; i++) {
		m_CustomPresetList[i].PreAmplifier = 0;
		for (int j = 0; j < NUM_FREQUENCY; j++)
			m_CustomPresetList[i].Frequency[j] = 0;
	}
}


// プラグインの情報を返す
bool CEqualizer::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"イコライザー";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"簡易イコライザー";
	return true;
}


// 初期化処理
bool CEqualizer::Initialize()
{
	// INIファイル名の取得
	::GetModuleFileName(g_hinstDLL, m_szIniFileName, MAX_PATH);
	::PathRenameExtension(m_szIniFileName, TEXT(".ini"));

	// イコライザー初期化
	m_BandPass.Initialize(m_FreqTable, NUM_FREQUENCY - 1, 48000);

	// コマンドを登録
	TVTest::HostInfo Host;
	if (m_pApp->GetHostInfo(&Host)
			&& Host.SupportedPluginVersion >= TVTEST_PLUGIN_VERSION_(0, 0, 14)) {
		// アイコン付きコマンド登録
		TVTest::PluginCommandInfo CommandInfo;
		CommandInfo.Size           = sizeof(CommandInfo);
		CommandInfo.Flags          = TVTest::PLUGIN_COMMAND_FLAG_ICONIZE;
		CommandInfo.State          = 0;
		CommandInfo.ID             = COMMAND_SHOW;
		CommandInfo.pszText        = L"Show";
		CommandInfo.pszName        = L"イコライザー 表示/非表示";
		CommandInfo.pszDescription = L"イコライザーの表示/非表示を切り替えます。";
		CommandInfo.hbmIcon        =
			(HBITMAP)::LoadImage(
				g_hinstDLL, MAKEINTRESOURCE(IDB_SHOW),
				IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
		m_pApp->RegisterPluginCommand(&CommandInfo);
		::DeleteObject(CommandInfo.hbmIcon);
		CommandInfo.ID             = COMMAND_ONOFF;
		CommandInfo.pszText        = L"Enable";
		CommandInfo.pszName        = L"イコライザー 入/切";
		CommandInfo.pszDescription = L"イコライザーの有効/無効を切り替えます。";
		CommandInfo.hbmIcon        =
			(HBITMAP)::LoadImage(
				g_hinstDLL, MAKEINTRESOURCE(IDB_ONOFF),
				IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
		m_pApp->RegisterPluginCommand(&CommandInfo);
		::DeleteObject(CommandInfo.hbmIcon);
	} else {
		// 旧バージョン用コマンド登録
		m_pApp->RegisterCommand(COMMAND_ONOFF, L"Enable", L"イコライザー 入/切");
	}

	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(EventCallback, this);

	int Enabled = ::GetPrivateProfileInt(TEXT("Settings"), TEXT("Enable"), -1, m_szIniFileName);
	if (Enabled == 1) {
		LoadSettings();
		EnableEqualizer(true);
	} else if (Enabled < 0) {
		m_fEnableDefault = true;
	}

	return true;
}


// 終了処理
bool CEqualizer::Finalize()
{
	// ウィンドウの破棄
	if (m_hwnd != nullptr)
		::DestroyWindow(m_hwnd);

	if (m_fShowed)
		SaveSettings();

	return true;
}


bool CEqualizer::ReadPreset(LPCTSTR pszSection, LPCTSTR pszKeyName, EqualizerSettings *pSettings)
{
	TCHAR szText[(NUM_FREQUENCY + 1) * 4 + 32];

	if (::GetPrivateProfileString(
			pszSection, pszKeyName, nullptr,
			szText, sizeof(szText) / sizeof(WCHAR), m_szIniFileName) == 0)
		return false;

	int i;
	LPCTSTR p = szText;

	for (i = -1; i < NUM_FREQUENCY && *p != _T('\0'); i++) {
		while (*p != _T('\0') && *p != _T('-') && (*p < _T('0') || *p > _T('9')))
			p++;
		if (*p == _T('\0'))
			break;
		LPTSTR pEnd;
		int Value = (int)std::_tcstol(p, &pEnd, 10);
		if (pEnd == p)
			break;
		p = pEnd;
		if (Value > 10)
			Value = 10;
		else if (Value < -10)
			Value = -10;
		if (i < 0)
			pSettings->PreAmplifier = Value;
		else
			pSettings->Frequency[i] = Value;
	}
	if (i < 0) {
		pSettings->PreAmplifier = 0;
		i = 0;
	}
	for (; i < NUM_FREQUENCY; i++)
		pSettings->Frequency[i] = 0;

	return true;
}


bool CEqualizer::WritePreset(LPCTSTR pszSection, LPCTSTR pszKeyName, const EqualizerSettings *pSettings) const
{
	TCHAR szText[(NUM_FREQUENCY + 1) * 4 + 1];
	int Length;

	Length = ::wsprintf(szText, TEXT("%d,"), pSettings->PreAmplifier);
	for (int i = 0; i < NUM_FREQUENCY; i++)
		Length += ::wsprintf(szText + Length, TEXT("%d,"), pSettings->Frequency[i]);
	szText[Length-1] = '\0';
	return ::WritePrivateProfileString(pszSection, pszKeyName, szText, m_szIniFileName) != FALSE;
}


// 設定読み込み
void CEqualizer::LoadSettings()
{
	if (!m_fSettingsLoaded) {
		m_WindowPosition.x =
			::GetPrivateProfileInt(TEXT("Settings"), TEXT("WindowLeft"), m_WindowPosition.x, m_szIniFileName);
		m_WindowPosition.y =
			::GetPrivateProfileInt(TEXT("Settings"), TEXT("WindowTop"), m_WindowPosition.y, m_szIniFileName);
		ReadPreset(TEXT("Settings"), TEXT("CurSetting"), &m_CurSettings);

		int Count = ::GetPrivateProfileInt(TEXT("Preset"), TEXT("Count"), 0, m_szIniFileName);
		if (Count > NUM_CUSTOM_PRESETS)
			Count = NUM_CUSTOM_PRESETS;
		for (int i = 0; i < Count; i++) {
			TCHAR szName[16];
			::wsprintf(szName, TEXT("Preset%d"), i);
			ReadPreset(TEXT("Preset"), szName, &m_CustomPresetList[i]);
		}

		m_fSettingsLoaded = true;
	}
}


BOOL WritePrivateProfileInt(LPCTSTR pszAppName, LPCTSTR pszKeyName, int Value, LPCTSTR pszFileName)
{
	TCHAR szValue[16];

	wsprintf(szValue, TEXT("%d"), Value);
	return WritePrivateProfileString(pszAppName, pszKeyName, szValue, pszFileName);
}

// 設定保存
void CEqualizer::SaveSettings() const
{
	::WritePrivateProfileInt(TEXT("Settings"), TEXT("Enable"), m_fEnabled, m_szIniFileName);
	::WritePrivateProfileInt(TEXT("Settings"), TEXT("WindowLeft"), m_WindowPosition.x, m_szIniFileName);
	::WritePrivateProfileInt(TEXT("Settings"), TEXT("WindowTop"), m_WindowPosition.y, m_szIniFileName);
	WritePreset(TEXT("Settings"), TEXT("CurSetting"), &m_CurSettings);

	::WritePrivateProfileInt(TEXT("Preset"), TEXT("Count"), NUM_CUSTOM_PRESETS, m_szIniFileName);
	for (int i = 0; i < NUM_CUSTOM_PRESETS; i++) {
		TCHAR szName[16];
		::wsprintf(szName, TEXT("Preset%d"), i);
		WritePreset(TEXT("Preset"), szName, &m_CustomPresetList[i]);
	}
}


// プラグインの有効状態を切り替え
bool CEqualizer::EnablePlugin(bool fEnable)
{
	if (fEnable) {
		// プラグインが有効にされたのでウィンドウを作成する
		if (m_hwnd == nullptr) {
			if (!m_fShowed) {
				// ウィンドウクラスの登録
				WNDCLASS wc;
				wc.style = 0;
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
			}

			LoadSettings();

			// プライマリモニタの DPI を取得
			m_DPI = m_pApp->GetDPIFromPoint(0, 0);
			if (m_DPI == 0)
				m_DPI = 96;

			CalcMetrics();

			constexpr DWORD Style = WS_POPUP | WS_CAPTION | WS_SYSMENU;
			constexpr DWORD ExStyle = WS_EX_TOOLWINDOW;
			RECT rc;
			::SetRect(&rc, 0, 0, m_ClientWidth, m_ClientHeight);
			::AdjustWindowRectEx(&rc, Style, FALSE, ExStyle);
			if (::CreateWindowEx(
					ExStyle, WINDOW_CLASS_NAME, TEXT("Equalizer"), Style,
					0, 0, rc.right - rc.left, rc.bottom - rc.top,
					m_pApp->GetAppWindow(), nullptr, g_hinstDLL, this) == nullptr)
				return false;

			m_fShowed = true;

			// ウィンドウ位置の復元
			WINDOWPLACEMENT wp;
			wp.length = sizeof(WINDOWPLACEMENT);
			::GetWindowPlacement(m_hwnd, &wp);
			wp.flags = 0;
			wp.showCmd = SW_SHOWNOACTIVATE;
			wp.rcNormalPosition.left = m_WindowPosition.x;
			wp.rcNormalPosition.top = m_WindowPosition.y;
			wp.rcNormalPosition.right = m_WindowPosition.x + (rc.right - rc.left);
			wp.rcNormalPosition.bottom = m_WindowPosition.y + (rc.bottom - rc.top);
			::SetWindowPlacement(m_hwnd, &wp);
		} else {
			::ShowWindow(m_hwnd, SW_SHOWNORMAL);
		}
		::UpdateWindow(m_hwnd);
	} else {
		// プラグインが無効にされたのでウィンドウを破棄する
		if (m_hwnd != nullptr)
			::DestroyWindow(m_hwnd);
	}

	m_pApp->SetPluginCommandState(COMMAND_SHOW, fEnable ? TVTest::PLUGIN_COMMAND_STATE_CHECKED : 0);

	return true;
}


// イコライザーのOn/Off切り替え
void CEqualizer::EnableEqualizer(bool fEnable)
{
	if (m_fEnabled != fEnable) {
		if (fEnable) {
			m_BandPass.Reset();
			ApplySettings();
			// 音声コールバックを登録
			m_pApp->SetAudioCallback(AudioCallback, this);
		} else {
			// 音声コールバックを登録解除
			m_pApp->SetAudioCallback(nullptr);
		}

		m_fEnabled = fEnable;

		if (m_hwnd != nullptr)
			::InvalidateRect(m_hwnd, nullptr, FALSE);

		m_pApp->SetPluginCommandState(COMMAND_ONOFF, fEnable ? TVTest::PLUGIN_COMMAND_STATE_CHECKED : 0);
	}
}


// イコライザーの設定を初期化
void CEqualizer::ResetSettings()
{
	m_CurSettings.PreAmplifier = 0;
	for (int i = 0; i < NUM_FREQUENCY; i++)
		m_CurSettings.Frequency[i] = 0;
}


// イコライザーの設定を適用
void CEqualizer::ApplySettings()
{
	m_BandPass.SetPreAmplifier((double)(m_CurSettings.PreAmplifier + 10) * 0.1);
	for (int i = 0; i < NUM_FREQUENCY; i++)
		m_BandPass.SetVolume(i, (double)(m_CurSettings.Frequency[i] + 10) * 0.1);
}


// 各部のサイズを計算する
void CEqualizer::CalcMetrics()
{
	m_SliderWidth = SLIDER_WIDTH;
	m_SliderHeight = SLIDER_HEIGHT;
	m_SliderMargin = SLIDER_MARGIN;
	m_SliderPadding = SLIDER_PADDING;
	m_WindowMargin = WINDOW_MARGIN;
	m_TextHeight = TEXT_HEIGHT;
	m_SliderTextMargin = SLIDER_TEXT_MARGIN;
	m_SliderButtonMargin = SLIDER_BUTTON_MARGIN;
	m_ButtonWidth = BUTTON_WIDTH;
	m_ButtonHeight = BUTTON_HEIGHT;
	m_ButtonMargin = BUTTON_MARGIN;
	m_LineWidth = LINE_WIDTH;

	// DPIに応じてスケーリング
	if (m_DPI != 96) {
		ScaleDPI(&m_SliderWidth);
		ScaleDPI(&m_SliderHeight);
		ScaleDPI(&m_SliderMargin);
		ScaleDPI(&m_SliderPadding);
		ScaleDPI(&m_WindowMargin);
		ScaleDPI(&m_TextHeight);
		ScaleDPI(&m_SliderTextMargin);
		ScaleDPI(&m_SliderButtonMargin);
		ScaleDPI(&m_ButtonWidth);
		ScaleDPI(&m_ButtonHeight);
		ScaleDPI(&m_ButtonMargin);
		ScaleDPI(&m_LineWidth);
	}

	m_ClientWidth = (NUM_FREQUENCY + 2) * (m_SliderWidth + m_SliderMargin) - m_SliderMargin + m_WindowMargin * 2;
	m_ClientHeight = m_SliderHeight + m_SliderTextMargin + m_TextHeight + m_SliderButtonMargin + m_ButtonHeight + m_WindowMargin * 2;
}


// DPI 依存のリソースを作成する
void CEqualizer::CreateDPIDependingResources()
{
	// フォントを取得
	LOGFONT lf;
	m_pApp->GetFont(L"StatusBarFont", &lf, m_DPI);
	lf.lfHeight = -m_TextHeight;
	lf.lfWidth = 0;
	if (m_hfont != nullptr)
		::DeleteObject(m_hfont);
	m_hfont = ::CreateFontIndirect(&lf);
}


// スライダの矩形を取得
void CEqualizer::GetSliderRect(int Index, RECT *pRect, bool fBar) const
{
	int x;

	if (Index < 0)
		x = 0;
	else
		x = Index + 2;
	pRect->left = m_WindowMargin + x * (m_SliderWidth + m_SliderMargin);
	pRect->right = pRect->left + m_SliderWidth;
	pRect->top = m_WindowMargin;
	pRect->bottom = m_WindowMargin + m_SliderHeight;
	if (fBar)
		::InflateRect(pRect, -m_SliderPadding, -m_SliderPadding);
}


// カーソル位置からスライダの位置を求める
int CEqualizer::MapSliderPos(int y) const
{
	const int BarHeight = m_SliderHeight - m_SliderPadding * 2;
	y -= m_WindowMargin + m_SliderMargin;
	if (y < 0)
		y = 0;
	else if (y >= BarHeight)
		y = BarHeight - 1;
	return 10 - ::MulDiv(y, 20, BarHeight - 1);
}


// スライダの位置から描画位置を求める
int CEqualizer::CalcSliderPos(int Pos) const
{
	return m_WindowMargin + m_SliderHeight - m_SliderPadding -
		::MulDiv(Pos + 10, m_SliderHeight - m_SliderPadding * 2, 20);
}


// ボタンの位置を取得
void CEqualizer::GetButtonRect(int Button, RECT *pRect) const
{
	pRect->left = m_WindowMargin + (m_ButtonWidth + m_ButtonMargin) * Button;
	pRect->right = pRect->left + m_ButtonWidth;
	pRect->top = m_WindowMargin + m_SliderHeight + m_SliderTextMargin + m_TextHeight + m_SliderButtonMargin;
	pRect->bottom = pRect->top + m_ButtonHeight;
}


// 配色を取得
void CEqualizer::GetColor()
{
	m_crBackColor = m_pApp->GetColor(L"PanelBack");
	m_crTextColor = m_pApp->GetColor(L"PanelText");
}


// ボタンが押された
void CEqualizer::OnButtonPush(int Button)
{
	switch (Button) {
	case BUTTON_ENABLE:
		// On/Off切り替え
		EnableEqualizer(!m_fEnabled);
		::UpdateWindow(m_hwnd);
		break;

	case BUTTON_RESET:
		// リセット
		ResetSettings();
		ApplySettings();
		::InvalidateRect(m_hwnd, nullptr, FALSE);
		::UpdateWindow(m_hwnd);
		break;

	case BUTTON_LOAD:
	case BUTTON_SAVE:
		// プリセットの読み込み/保存
		{
			HMENU hmenu = ::CreatePopupMenu();

			for (int i = 0; i < NUM_CUSTOM_PRESETS; i++) {
				TCHAR szText[16];
				::wsprintf(szText, TEXT("Slot %d"), i + 1);
				::AppendMenu(hmenu, MF_STRING | MF_ENABLED, i + 1, szText);
			}
			if (Button == BUTTON_LOAD) {
				::AppendMenu(hmenu, MF_SEPARATOR, 0, nullptr);
				HMENU hmenuPresets = ::CreatePopupMenu();
				::AppendMenu(hmenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hmenuPresets), TEXT("Presets"));
				for (int i = 0; i < sizeof(m_PresetList) / sizeof(EqualizerPreset); i++) {
					::AppendMenu(hmenuPresets, MF_STRING | MF_ENABLED, NUM_CUSTOM_PRESETS + 1 + i, m_PresetList[i].pszName);
				}
			}
			RECT rc;
			GetButtonRect(Button, &rc);
			MapWindowRect(m_hwnd, nullptr, &rc);
			TPMPARAMS tpm;
			tpm.cbSize = sizeof(TPMPARAMS);
			tpm.rcExclude = rc;
			int Result = ::TrackPopupMenuEx(
				hmenu, TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_VERTICAL, rc.left, rc.bottom, m_hwnd, &tpm);
			::DestroyMenu(hmenu);
			if (Result > 0) {
				Result--;
				if (Button == BUTTON_LOAD) {
					if (Result < NUM_CUSTOM_PRESETS)
						m_CurSettings = m_CustomPresetList[Result];
					else
						m_CurSettings = m_PresetList[Result - NUM_CUSTOM_PRESETS].Setting;
					if (m_fEnabled) {
						ApplySettings();
						::InvalidateRect(m_hwnd, nullptr, FALSE);
					} else {
						EnableEqualizer(true);
					}
					::UpdateWindow(m_hwnd);
				} else {
					m_CustomPresetList[Result] = m_CurSettings;
				}
			}
		}
		break;
	}
}


// 画面描画
void CEqualizer::Draw(HDC hdc, const RECT &rcPaint)
{
	HBRUSH hbr, hbrOld;
	HPEN hpen, hpenOld;
	HFONT hfontOld;
	COLORREF crOldTextColor;
	int OldBkMode;
	RECT rc, rcSlider, rcText;
	TCHAR szText[16];

	hbr = ::CreateSolidBrush(m_crBackColor);
	::FillRect(hdc, &rcPaint, hbr);
	::DeleteObject(hbr);

	hbr = ::CreateSolidBrush(
		m_fEnabled ? m_crTextColor:
		RGB((GetRValue(m_crTextColor) + GetRValue(m_crBackColor)) / 2,
			(GetGValue(m_crTextColor) + GetGValue(m_crBackColor)) / 2,
			(GetBValue(m_crTextColor) + GetBValue(m_crBackColor)) / 2));
	LOGBRUSH lb;
	lb.lbStyle = BS_SOLID;
	lb.lbColor = m_crTextColor;
	lb.lbHatch = 0;
	hpen = ::ExtCreatePen(
		PS_GEOMETRIC | PS_INSIDEFRAME | PS_ENDCAP_FLAT | PS_JOIN_MITER,
		m_LineWidth, &lb, 0, nullptr);
	hpenOld = static_cast<HPEN>(::SelectObject(hdc, hpen));
	hbrOld = static_cast<HBRUSH>(::SelectObject(hdc, ::GetStockObject(NULL_BRUSH)));
	hfontOld = static_cast<HFONT>(::SelectObject(hdc, m_hfont));
	crOldTextColor = ::SetTextColor(hdc, m_crTextColor);
	OldBkMode = ::SetBkMode(hdc, TRANSPARENT);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);

	// プリアンプ
	rc.left = m_WindowMargin;
	rc.top = m_WindowMargin;
	rc.right = rc.left + m_SliderWidth;
	rc.bottom = rc.top + m_SliderHeight;
	::Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
	rcSlider.left = rc.left + m_SliderPadding;
	rcSlider.right = rc.right - m_SliderPadding;
	rcSlider.bottom = rc.bottom - m_SliderPadding;
	rcSlider.top = CalcSliderPos(m_CurSettings.PreAmplifier);
	if (rcSlider.top < rcSlider.bottom)
		::FillRect(hdc, &rcSlider, hbr);
	rcText.left = rc.left - m_SliderMargin;
	rcText.right = rc.right + m_SliderMargin;
	rcText.top = rc.bottom + m_SliderTextMargin;
	rcText.bottom = rcText.top + m_TextHeight;
	rcText.top -= std::min<LONG>(tm.tmInternalLeading, m_SliderTextMargin);
	::DrawText(hdc, TEXT("Pre"), -1, &rcText, DT_CENTER | DT_SINGLELINE);

	// 目盛
	rc.left = rc.right + m_SliderMargin;
	rc.right = rc.left + m_SliderWidth;
	for (int i = -10; i <= 10; i += 5) {
		int y = CalcSliderPos(i);
		::MoveToEx(hdc, rc.left + m_SliderPadding, y, nullptr);
		::LineTo(hdc, rc.right - m_SliderPadding, y);
	}

	// 各周波数
	int Freq = 3125;
	for (int i = 0; i < NUM_FREQUENCY; i++) {
		rc.left = rc.right + m_SliderMargin;
		rc.right = rc.left + m_SliderWidth;
		::Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
		rcSlider.left = rc.left + m_SliderPadding;
		rcSlider.right = rc.right - m_SliderPadding;
		rcSlider.top = CalcSliderPos(m_CurSettings.Frequency[i]);
		if (rcSlider.top < rcSlider.bottom)
			::FillRect(hdc, &rcSlider, hbr);
		rcText.left = rc.left - m_SliderMargin / 2;
		rcText.right = rc.right + m_SliderMargin / 2;
		if (Freq < 100000)
			::wsprintf(szText, TEXT("%d"), Freq / 100);
		else
			::wsprintf(szText, TEXT("%dk"), Freq / (1000 * 100));
		::DrawText(hdc, szText, -1, &rcText, DT_CENTER | DT_SINGLELINE);
		Freq *= 2;
	}

	// ボタンの描画
	static const LPCTSTR pszButtonText[NUM_BUTTONS] = {
		TEXT("On/Off"), TEXT("Flat"), TEXT("Load"), TEXT("Save")
	};
	for (int i = 0; i < NUM_BUTTONS; i++) {
		GetButtonRect(i, &rc);
		if (i == BUTTON_ENABLE && m_fEnabled) {
			::FillRect(hdc, &rc, hbr);
			::SetTextColor(hdc, m_crBackColor);
		} else {
			::Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
			::SetTextColor(hdc, m_crTextColor);
		}
		::DrawText(hdc, pszButtonText[i], -1, &rc, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
	}

	::DeleteObject(hbr);
	::SelectObject(hdc, hpenOld);
	::DeleteObject(hpen);
	::SelectObject(hdc, hfontOld);
	::SetTextColor(hdc, crOldTextColor);
	::SetBkMode(hdc, OldBkMode);
}


// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CEqualizer::EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
{
	CEqualizer *pThis = static_cast<CEqualizer *>(pClientData);

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

	case TVTest::EVENT_COMMAND:
		// コマンドが選択された
		switch ((int)lParam1) {
		case COMMAND_SHOW:
			pThis->m_pApp->EnablePlugin(!pThis->m_pApp->IsPluginEnabled());
			return TRUE;

		case  COMMAND_ONOFF:
			pThis->EnableEqualizer(!pThis->m_fEnabled);
			return TRUE;
		}
		return FALSE;

	case TVTest::EVENT_COLORCHANGE:
		// 色の設定が変化した
		if (pThis->m_hwnd != nullptr) {
			// 新しい配色を適用する
			pThis->GetColor();
			::RedrawWindow(pThis->m_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
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
LRESULT CALLBACK CEqualizer::AudioCallback(short *pData, DWORD Samples, int Channels, void *pClientData)
{
	CEqualizer *pThis = static_cast<CEqualizer *>(pClientData);

	// イコライザー処理
	pThis->m_BandPass.ProcessSamples(pData, Samples, Channels);

	return 0;
}


// ウィンドウハンドルからthisポインタを取得する
CEqualizer * CEqualizer::GetThis(HWND hwnd)
{
	return reinterpret_cast<CEqualizer *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
}


// ウィンドウプロシージャ
LRESULT CALLBACK CEqualizer::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
			CEqualizer *pThis = static_cast<CEqualizer *>(pcs->lpCreateParams);

			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
			pThis->m_hwnd = hwnd;
			pThis->GetColor();

			pThis->CreateDPIDependingResources();

			// メインウィンドウがダークモードであればそれに合わせてダークモードにする
			if (pThis->m_pApp->GetDarkModeStatus() & TVTest::DARK_MODE_STATUS_MAINWINDOW_DARK)
				pThis->m_pApp->SetWindowDarkMode(hwnd, true);

			// ウィンドウを最初に表示した時にイコライザーを有効化する
			if (!pThis->m_fEnabled && pThis->m_fEnableDefault)
				pThis->EnableEqualizer(true);
		}
		return 0;

	case WM_PAINT:
		{
			CEqualizer *pThis = GetThis(hwnd);
			PAINTSTRUCT ps;

			::BeginPaint(hwnd, &ps);
			pThis->Draw(ps.hdc, ps.rcPaint);
			::EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			CEqualizer *pThis = GetThis(hwnd);
			POINT pt;
			RECT rc;

			pt.x = GET_X_LPARAM(lParam);
			pt.y = GET_Y_LPARAM(lParam);
			if (pt.y >= pThis->m_WindowMargin && pt.y < pThis->m_WindowMargin + pThis->m_SliderHeight) {
				for (int i = -1; i < NUM_FREQUENCY; i++) {
					pThis->GetSliderRect(i, &rc);
					if (::PtInRect(&rc, pt)) {
						// スライダ
						int Pos = pThis->MapSliderPos(pt.y);

						if (i < 0)
							pThis->m_CurSettings.PreAmplifier = Pos;
						else
							pThis->m_CurSettings.Frequency[i] = Pos;
						if (pThis->m_fEnabled) {
							pThis->ApplySettings();
							::InvalidateRect(hwnd, &rc, FALSE);
						} else {
							pThis->EnableEqualizer(true);
						}
						::UpdateWindow(hwnd);
						pThis->m_CurSlider = i;
						::SetCapture(hwnd);
						break;
					}
				}
			} else {
				for (int i = 0; i < NUM_BUTTONS; i++) {
					pThis->GetButtonRect(i, &rc);
					if (::PtInRect(&rc, pt)) {
						// ボタンが押された
						pThis->OnButtonPush(i);
						break;
					}
				}
			}
		}
		return 0;

	case WM_LBUTTONUP:
		if (::GetCapture() == hwnd)
			// スライダのドラッグ終了
			::ReleaseCapture();
		return 0;

	case WM_MOUSEMOVE:
		if (::GetCapture() == hwnd) {
			// スライダのドラッグ
			CEqualizer *pThis = GetThis(hwnd);
			int Pos = pThis->MapSliderPos(GET_Y_LPARAM(lParam));

			if (pThis->m_CurSlider < 0) {
				if (pThis->m_CurSettings.PreAmplifier == Pos)
					return 0;
				pThis->m_CurSettings.PreAmplifier = Pos;
			} else {
				if (pThis->m_CurSettings.Frequency[pThis->m_CurSlider] == Pos)
					return 0;
				pThis->m_CurSettings.Frequency[pThis->m_CurSlider] = Pos;
			}
			RECT rc;
			pThis->GetSliderRect(pThis->m_CurSlider, &rc, true);
			::InvalidateRect(hwnd, &rc, FALSE);
			::UpdateWindow(hwnd);
			pThis->ApplySettings();
		}
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT) {
			// カーソル設定
			CEqualizer *pThis = GetThis(hwnd);
			DWORD Pos = ::GetMessagePos();
			POINT pt;
			RECT rc;

			pt.x = GET_X_LPARAM(Pos);
			pt.y = GET_Y_LPARAM(Pos);
			::ScreenToClient(hwnd, &pt);
			for (int i = -1; i < NUM_FREQUENCY; i++) {
				pThis->GetSliderRect(i, &rc);
				if (::PtInRect(&rc, pt)) {
					::SetCursor(::LoadCursor(nullptr, IDC_HAND));
					return TRUE;
				}
			}
			for (int i = 0; i < NUM_BUTTONS; i++) {
				pThis->GetButtonRect(i, &rc);
				if (::PtInRect(&rc, pt)) {
					::SetCursor(::LoadCursor(nullptr, IDC_HAND));
					return TRUE;
				}
			}
		}
		break;

	case WM_SYSCOMMAND:
		if ((wParam & 0xFFF0) == SC_CLOSE) {
			// 閉じる時はプラグインを無効にする
			// (勝手に閉じるとTVTestとの整合性が取れなくなるため)
			CEqualizer *pThis = GetThis(hwnd);

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
			CEqualizer *pThis = GetThis(hwnd);
			const RECT *prc = reinterpret_cast<const RECT *>(lParam);

			pThis->m_DPI = HIWORD(wParam);

			pThis->CalcMetrics();
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
			CEqualizer *pThis = GetThis(hwnd);

			// 後始末
			::DeleteObject(pThis->m_hfont);

			// ウィンドウ位置保存
			WINDOWPLACEMENT wp;
			wp.length = sizeof (WINDOWPLACEMENT);
			if (::GetWindowPlacement(hwnd, &wp)) {
				pThis->m_WindowPosition.x = wp.rcNormalPosition.left;
				pThis->m_WindowPosition.y = wp.rcNormalPosition.top;
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
	return new CEqualizer;
}
