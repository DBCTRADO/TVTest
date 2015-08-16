/*
	TVTest �v���O�C���T���v��

	�ȈՃC�R���C�U�[
*/


#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <tchar.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstddef>
#include <crtdbg.h>
#if defined(_MSC_VER) && defined(_M_X64)
#include <emmintrin.h>
#endif
#define TVTEST_PLUGIN_CLASS_IMPLEMENT	// �N���X�Ƃ��Ď���
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


// �o���h�p�X�t�B���^�N���X
class CBandPass
{
	enum {
		MAX_CHANNELS	= 6,	// �ő�`�����l����(5.1ch)
		MAX_FREQUENCY	= 16	// �ő���g��������
	};

	double m_Coef[MAX_FREQUENCY * MAX_CHANNELS];
	double m_Ener[MAX_FREQUENCY * MAX_CHANNELS];
	double m_Volume[MAX_FREQUENCY * MAX_CHANNELS];
	int m_EqualizerCount;
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
	: m_EqualizerCount(0)
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
	_ASSERT(Index >=0 && Index <= m_EqualizerCount);
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




// �v���O�C���N���X
class CEqualizer : public TVTest::CTVTestPlugin
{
public:
	CEqualizer();
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();

private:
	// �R�}���h
	enum {
		COMMAND_SHOW	= 1,	// �\��/��\��
		COMMAND_ONOFF	= 2		// On/Off
	};

	enum {
		NUM_FREQUENCY = 10,
		NUM_CUSTOM_PRESETS = 10
	};

	// �e���̃T�C�Y(DIP�P��)
	enum {
		SLIDER_WIDTH			= 16,
		SLIDER_HEIGHT			= 80,
		SLIDER_MARGIN			= 4,
		SLIDER_PADDING			= 2,
		WINDOW_MARGIN			= 8,
		TEXT_HEIGHT				= 10,
		SLIDER_TEXT_MARGIN		= 3,
		SLIDER_BUTTON_MARGIN	= 4,
		BUTTON_WIDTH			= 52,
		BUTTON_HEIGHT			= TEXT_HEIGHT + 8,
		BUTTON_MARGIN			= 4,
		LINE_WIDTH				= 1
	};

	enum {
		BUTTON_ENABLE,
		BUTTON_RESET,
		BUTTON_LOAD,
		BUTTON_SAVE,
		NUM_BUTTONS
	};

	struct EqualizerSettings {
		int PreAmplifier;
		int Frequency[NUM_FREQUENCY];
	};

	struct EqualizerPreset {
		LPCTSTR pszName;
		EqualizerSettings Setting;
	};

	TCHAR m_szIniFileName[MAX_PATH];
	bool m_fSettingsLoaded;
	bool m_fShowed;
	HWND m_hwnd;
	COLORREF m_crBackColor;
	COLORREF m_crTextColor;
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
	HFONT m_hfont;
	POINT m_WindowPosition;
	CBandPass m_BandPass;
	bool m_fEnabled;
	bool m_fEnableDefault;
	EqualizerSettings m_CurSettings;
	int m_CurSlider;
	EqualizerSettings m_CustomPresetList[NUM_CUSTOM_PRESETS];

	static const LPCTSTR WINDOW_CLASS_NAME;
	static const EqualizerPreset m_PresetList[];
	static const double m_FreqTable[NUM_FREQUENCY - 1];

	bool ReadPreset(LPCTSTR pszSection,LPCTSTR pszKeyName, EqualizerSettings *pSettings);
	bool WritePreset(LPCTSTR pszSection,LPCTSTR pszKeyName, const EqualizerSettings *pSettings) const;
	void LoadSettings();
	void SaveSettings() const;
	bool EnablePlugin(bool fEnable);
	void EnableEqualizer(bool fEnable);
	void ResetSettings();
	void ApplySettings();
	void GetSliderRect(int Index, RECT *pRect, bool fBar = false) const;
	int MapSliderPos(int y) const;
	int CalcSliderPos(int Pos) const;
	void GetButtonRect(int Button, RECT *pRect) const;
	void GetColor();
	void OnButtonPush(int Button);
	void Draw(HDC hdc,const RECT &rcPaint);

	static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);
	static LRESULT CALLBACK AudioCallback(short *pData, DWORD Samples, int Channels, void *pClientData);
	static CEqualizer *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void ScaleDPI(int *pValue,int DPI) {
		int Value = ::MulDiv(*pValue, DPI, 96);
		if (Value < 1)
			Value = 1;
		*pValue = Value;
	}
};


const LPCTSTR CEqualizer::WINDOW_CLASS_NAME = TEXT("TVTest Equalizer Window");


const CEqualizer::EqualizerPreset CEqualizer::m_PresetList[] = {
	{TEXT("Accoustic"),			{0,	{ 4, 4, 3, 0, 1, 1, 3, 3, 3, 1}}},
	{TEXT("Bass Boost"),		{0,	{ 5, 4, 3, 2, 1, 0, 0, 0, 0, 0}}},
	{TEXT("Boost"),				{0,	{ 2, 5, 7, 5, 5, 4, 5, 7, 9, 6}}},
	{TEXT("Classical"),			{0,	{ 4, 3, 2, 2,-1,-1, 0, 1, 2, 3}}},
	{TEXT("Dance"),				{0,	{ 3, 5, 4, 0, 1, 3, 4, 3, 3, 0}}},
	{TEXT("Electronic"),		{0,	{ 3, 3, 1, 0,-1, 1, 0, 1, 3, 4}}},
	{TEXT("For Poor Speakers"),	{0,	{ 4, 3, 3, 2, 1, 0,-1,-2,-2,-3}}},
	{TEXT("Hip-Hop"),			{0,	{ 4, 3, 1, 2, 0, 0, 1, 0, 1, 2}}},
	{TEXT("Jazz"),				{0,	{ 3, 2, 1, 1,-1,-1, 0, 1, 2, 3}}},
	{TEXT("Pop"),				{0,	{-1, 0, 0, 1, 3, 3, 1, 0, 0,-1}}},
	{TEXT("R&&B"),				{0,	{ 2, 5, 4, 1,-1,-1, 2, 2, 2, 3}}},
	{TEXT("Rock"),				{0,	{ 4, 3, 2, 1, 0, 0, 0, 2, 3, 3}}},
	{TEXT("Treble Boost"),		{0,	{ 0, 0, 0, 0, 0, 1, 2, 3, 4, 5}}},
	{TEXT("Vocal"),				{0,	{-1,-2,-2, 1, 3, 3, 2, 1, 0,-1}}},
	{TEXT("Voice"),				{0,	{-2, 0, 0, 0, 2, 3, 4, 3, 2, 0}}},
};

const double CEqualizer::m_FreqTable[NUM_FREQUENCY-1] = {
	44.2, 88.4, 176.8, 353.6, 707.1, 1414.2, 2828.4, 5656.8, 11313.7
};


CEqualizer::CEqualizer()
	: m_fSettingsLoaded(false)
	, m_fShowed(false)
	, m_hwnd(nullptr)
	, m_fEnabled(false)
	, m_fEnableDefault(false)
{
	m_WindowPosition.x = 0;
	m_WindowPosition.y = 0;

	ResetSettings();

	for (int i = 0; i < NUM_CUSTOM_PRESETS; i++) {
		m_CustomPresetList[i].PreAmplifier = 0;
		for (int j = 0; j < NUM_FREQUENCY; j++)
			m_CustomPresetList[i].Frequency[j] = 0;
	}
}


// �v���O�C���̏���Ԃ�
bool CEqualizer::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"�C�R���C�U�[";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"�ȈՃC�R���C�U�[";
	return true;
}


// ����������
bool CEqualizer::Initialize()
{
	// INI�t�@�C�����̎擾
	::GetModuleFileName(g_hinstDLL, m_szIniFileName, MAX_PATH);
	::PathRenameExtension(m_szIniFileName, TEXT(".ini"));

	// �C�R���C�U�[������
	m_BandPass.Initialize(m_FreqTable, NUM_FREQUENCY - 1, 48000);

	// �R�}���h��o�^
	TVTest::HostInfo Host;
	if (m_pApp->GetHostInfo(&Host)
			&& Host.SupportedPluginVersion >= TVTEST_PLUGIN_VERSION_(0,0,14)) {
		// �A�C�R���t���R�}���h�o�^
		TVTest::PluginCommandInfo CommandInfo;
		CommandInfo.Size           = sizeof(CommandInfo);
		CommandInfo.Flags          = TVTest::PLUGIN_COMMAND_FLAG_ICONIZE;
		CommandInfo.State          = 0;
		CommandInfo.ID             = COMMAND_SHOW;
		CommandInfo.pszText        = L"Show";
		CommandInfo.pszName        = L"�C�R���C�U�[ �\��/��\��";
		CommandInfo.pszDescription = L"�C�R���C�U�[�̕\��/��\����؂�ւ��܂��B";
		CommandInfo.hbmIcon        =
			(HBITMAP)::LoadImage(g_hinstDLL, MAKEINTRESOURCE(IDB_SHOW),
								 IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
		m_pApp->RegisterPluginCommand(&CommandInfo);
		::DeleteObject(CommandInfo.hbmIcon);
		CommandInfo.ID             = COMMAND_ONOFF;
		CommandInfo.pszText        = L"Enable";
		CommandInfo.pszName        = L"�C�R���C�U�[ ��/��";
		CommandInfo.pszDescription = L"�C�R���C�U�[�̗L��/������؂�ւ��܂��B";
		CommandInfo.hbmIcon        =
			(HBITMAP)::LoadImage(g_hinstDLL, MAKEINTRESOURCE(IDB_ONOFF),
								 IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
		m_pApp->RegisterPluginCommand(&CommandInfo);
		::DeleteObject(CommandInfo.hbmIcon);
	} else {
		// ���o�[�W�����p�R�}���h�o�^
		m_pApp->RegisterCommand(COMMAND_ONOFF, L"Enable", L"�C�R���C�U�[ ��/��");
	}

	// �C�x���g�R�[���o�b�N�֐���o�^
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


// �I������
bool CEqualizer::Finalize()
{
	// �E�B���h�E�̔j��
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
		int Value = (int)::_tcstol(p, &pEnd, 10);
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


// �ݒ�ǂݍ���
void CEqualizer::LoadSettings()
{
	if (!m_fSettingsLoaded) {
		m_WindowPosition.x =
			::GetPrivateProfileInt(TEXT("Settings"), TEXT("WindowLeft"),
								   m_WindowPosition.x, m_szIniFileName);
		m_WindowPosition.y =
			::GetPrivateProfileInt(TEXT("Settings"), TEXT("WindowTop"),
								   m_WindowPosition.y, m_szIniFileName);
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

// �ݒ�ۑ�
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


// �v���O�C���̗L����Ԃ�؂�ւ�
bool CEqualizer::EnablePlugin(bool fEnable)
{
	if (fEnable) {
		// �v���O�C�����L���ɂ��ꂽ�̂ŃE�B���h�E���쐬����
		if (m_hwnd == nullptr) {
			if (!m_fShowed) {
				// �E�B���h�E�N���X�̓o�^
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

			// DPI�ɉ����ăX�P�[�����O
			int DPI;
			if (m_pApp->GetSetting(L"DPI", &DPI) && DPI != 96) {
				ScaleDPI(&m_SliderWidth, DPI);
				ScaleDPI(&m_SliderHeight, DPI);
				ScaleDPI(&m_SliderMargin, DPI);
				ScaleDPI(&m_SliderPadding, DPI);
				ScaleDPI(&m_WindowMargin, DPI);
				ScaleDPI(&m_TextHeight, DPI);
				ScaleDPI(&m_SliderTextMargin, DPI);
				ScaleDPI(&m_SliderButtonMargin, DPI);
				ScaleDPI(&m_ButtonWidth, DPI);
				ScaleDPI(&m_ButtonHeight, DPI);
				ScaleDPI(&m_ButtonMargin, DPI);
				ScaleDPI(&m_LineWidth, DPI);
			}

			static const DWORD Style = WS_POPUP | WS_CAPTION | WS_SYSMENU;
			static const DWORD ExStyle = WS_EX_TOOLWINDOW;
			RECT rc;
			::SetRect(&rc, 0, 0,
				(NUM_FREQUENCY + 2) * (m_SliderWidth + m_SliderMargin) - m_SliderMargin + m_WindowMargin * 2,
				m_SliderHeight + m_SliderTextMargin + m_TextHeight + m_SliderButtonMargin + m_ButtonHeight + m_WindowMargin * 2);
			::AdjustWindowRectEx(&rc, Style, FALSE, ExStyle);
			if (::CreateWindowEx(
					ExStyle, WINDOW_CLASS_NAME, TEXT("Equalizer"), Style,
					m_WindowPosition.x, m_WindowPosition.y,
					rc.right - rc.left, rc.bottom - rc.top,
					m_pApp->GetAppWindow(), nullptr, g_hinstDLL, this) == nullptr)
				return false;

			m_fShowed = true;
		}

		::ShowWindow(m_hwnd, SW_SHOWNORMAL);
		::UpdateWindow(m_hwnd);
	} else {
		// �v���O�C���������ɂ��ꂽ�̂ŃE�B���h�E��j������
		if (m_hwnd != nullptr)
			::DestroyWindow(m_hwnd);
	}

	m_pApp->SetPluginCommandState(COMMAND_SHOW,
								  fEnable ? TVTest::PLUGIN_COMMAND_STATE_CHECKED : 0);

	return true;
}


// �C�R���C�U�[��On/Off�؂�ւ�
void CEqualizer::EnableEqualizer(bool fEnable)
{
	if (m_fEnabled != fEnable) {
		if (fEnable) {
			m_BandPass.Reset();
			ApplySettings();
			// �����R�[���o�b�N��o�^
			m_pApp->SetAudioCallback(AudioCallback, this);
		} else {
			// �����R�[���o�b�N��o�^����
			m_pApp->SetAudioCallback(nullptr);
		}

		m_fEnabled = fEnable;

		if (m_hwnd != nullptr)
			::InvalidateRect(m_hwnd, nullptr, FALSE);

		m_pApp->SetPluginCommandState(COMMAND_ONOFF,
									  fEnable ? TVTest::PLUGIN_COMMAND_STATE_CHECKED : 0);
	}
}


// �C�R���C�U�[�̐ݒ��������
void CEqualizer::ResetSettings()
{
	m_CurSettings.PreAmplifier = 0;
	for (int i = 0; i < NUM_FREQUENCY; i++)
		m_CurSettings.Frequency[i] = 0;
}


// �C�R���C�U�[�̐ݒ��K�p
void CEqualizer::ApplySettings()
{
	m_BandPass.SetPreAmplifier((double)(m_CurSettings.PreAmplifier + 10) * 0.1);
	for (int i = 0; i < NUM_FREQUENCY; i++)
		m_BandPass.SetVolume(i, (double)(m_CurSettings.Frequency[i] + 10) * 0.1);
}


// �X���C�_�̋�`���擾
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


// �J�[�\���ʒu����X���C�_�̈ʒu�����߂�
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


// �X���C�_�̈ʒu����`��ʒu�����߂�
int CEqualizer::CalcSliderPos(int Pos) const
{
	return m_WindowMargin + m_SliderHeight - m_SliderPadding -
		::MulDiv(Pos + 10, m_SliderHeight - m_SliderPadding * 2, 20);
}


// �{�^���̈ʒu���擾
void CEqualizer::GetButtonRect(int Button, RECT *pRect) const
{
	pRect->left = m_WindowMargin + (m_ButtonWidth + m_ButtonMargin) * Button;
	pRect->right = pRect->left + m_ButtonWidth;
	pRect->top = m_WindowMargin + m_SliderHeight + m_SliderTextMargin + m_TextHeight + m_SliderButtonMargin;
	pRect->bottom = pRect->top + m_ButtonHeight;
}


// �z�F���擾
void CEqualizer::GetColor()
{
	m_crBackColor = m_pApp->GetColor(L"PanelBack");
	m_crTextColor = m_pApp->GetColor(L"PanelText");
}


// �{�^���������ꂽ
void CEqualizer::OnButtonPush(int Button)
{
	switch (Button) {
	case BUTTON_ENABLE:
		// On/Off�؂�ւ�
		EnableEqualizer(!m_fEnabled);
		::UpdateWindow(m_hwnd);
		break;

	case BUTTON_RESET:
		// ���Z�b�g
		ResetSettings();
		ApplySettings();
		::InvalidateRect(m_hwnd, nullptr, FALSE);
		::UpdateWindow(m_hwnd);
		break;

	case BUTTON_LOAD:
	case BUTTON_SAVE:
		// �v���Z�b�g�̓ǂݍ���/�ۑ�
		{
			HMENU hmenu = ::CreatePopupMenu();

			for (int i = 0; i < NUM_CUSTOM_PRESETS; i++) {
				TCHAR szText[16];
				::wsprintf(szText, TEXT("Preset %d"), i + 1);
				::AppendMenu(hmenu, MF_STRING | MF_ENABLED, i + 1, szText);
			}
			if (Button == BUTTON_LOAD) {
				for (int i = 0; i < sizeof(m_PresetList) / sizeof(EqualizerPreset); i++) {
					UINT Flags = MF_STRING | MF_ENABLED;
					if (i == 0)
						Flags |= MF_MENUBREAK;
					::AppendMenu(hmenu, Flags, NUM_CUSTOM_PRESETS + 1 + i, m_PresetList[i].pszName);
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


// ��ʕ`��
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

	// �v���A���v
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
	rcText.top -= min(tm.tmInternalLeading, m_SliderTextMargin);
	::DrawText(hdc, TEXT("Pre"), -1, &rcText, DT_CENTER | DT_SINGLELINE);

	// �ڐ�
	rc.left = rc.right + m_SliderMargin;
	rc.right = rc.left + m_SliderWidth;
	for (int i = -10; i <= 10; i += 5) {
		int y = CalcSliderPos(i);
		::MoveToEx(hdc, rc.left + m_SliderPadding, y, nullptr);
		::LineTo(hdc, rc.right - m_SliderPadding, y);
	}

	// �e���g��
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

	// �{�^���̕`��
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


// �C�x���g�R�[���o�b�N�֐�
// �����C�x���g���N����ƌĂ΂��
LRESULT CALLBACK CEqualizer::EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
{
	CEqualizer *pThis = static_cast<CEqualizer*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// �v���O�C���̗L����Ԃ��ω�����
		return pThis->EnablePlugin(lParam1 != 0);

	case TVTest::EVENT_STANDBY:
		// �ҋ@��Ԃ��ω�����
		if (pThis->m_pApp->IsPluginEnabled()) {
			// �ҋ@��Ԃ̎��̓E�B���h�E���B��
			::ShowWindow(pThis->m_hwnd, lParam1 != 0 ? SW_HIDE : SW_SHOW);
		}
		return TRUE;

	case TVTest::EVENT_COMMAND:
		// �R�}���h���I�����ꂽ
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
		// �F�̐ݒ肪�ω�����
		if (pThis->m_hwnd != nullptr) {
			// �V�����z�F��K�p����
			pThis->GetColor();
			::RedrawWindow(pThis->m_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
		}
		return TRUE;
	}

	return 0;
}


// �����R�[���o�b�N�֐�
LRESULT CALLBACK CEqualizer::AudioCallback(short *pData, DWORD Samples, int Channels, void *pClientData)
{
	CEqualizer *pThis = static_cast<CEqualizer*>(pClientData);

	// �C�R���C�U�[����
	pThis->m_BandPass.ProcessSamples(pData, Samples, Channels);

	return 0;
}


// �E�B���h�E�n���h������this�|�C���^���擾����
CEqualizer *CEqualizer::GetThis(HWND hwnd)
{
	return reinterpret_cast<CEqualizer*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
}


// �E�B���h�E�v���V�[�W��
LRESULT CALLBACK CEqualizer::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
			CEqualizer *pThis = static_cast<CEqualizer*>(pcs->lpCreateParams);

			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
			pThis->m_hwnd = hwnd;
			pThis->GetColor();

			// �t�H���g���擾
			LOGFONT lf;
			if (!pThis->m_pApp->GetSetting(L"StatusBarFont", &lf)) {
				NONCLIENTMETRICS ncm;
#if WINVER >= 0x0600
				ncm.cbSize = offsetof(NONCLIENTMETRICS, iPaddedBorderWidth);
#else
				ncm.cbSize = sizeof(NONCLIENTMETRICS);
#endif
				::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
				lf = ncm.lfStatusFont;
			}
			lf.lfHeight = -pThis->m_TextHeight;
			lf.lfWidth = 0;
			pThis->m_hfont = ::CreateFontIndirect(&lf);

			// �E�B���h�E���ŏ��ɕ\���������ɃC�R���C�U�[��L��������
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
						// �X���C�_
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
						// �{�^���������ꂽ
						pThis->OnButtonPush(i);
						break;
					}
				}
			}
		}
		return 0;

	case WM_LBUTTONUP:
		if (::GetCapture() == hwnd)
			// �X���C�_�̃h���b�O�I��
			::ReleaseCapture();
		return 0;

	case WM_MOUSEMOVE:
		if (::GetCapture() == hwnd) {
			// �X���C�_�̃h���b�O
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
			// �J�[�\���ݒ�
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
			// ���鎞�̓v���O�C���𖳌��ɂ���
			// (����ɕ����TVTest�Ƃ̐����������Ȃ��Ȃ邽��)
			CEqualizer *pThis = GetThis(hwnd);

			pThis->m_pApp->EnablePlugin(false);
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			CEqualizer *pThis = GetThis(hwnd);

			// ��n��
			::DeleteObject(pThis->m_hfont);

			// �E�B���h�E�ʒu�ۑ�
			RECT rc;
			::GetWindowRect(hwnd, &rc);
			pThis->m_WindowPosition.x = rc.left;
			pThis->m_WindowPosition.y = rc.top;

			pThis->m_hwnd = nullptr;
		}
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}




// �v���O�C���N���X�̃C���X�^���X�𐶐�����
TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CEqualizer;
}
