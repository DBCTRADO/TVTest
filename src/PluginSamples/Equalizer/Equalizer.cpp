/*
	TVTest プラグインサンプル

	簡易イコライザ
*/


#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <tchar.h>
#include <math.h>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT	// クラスとして実装
#include "TVTestPlugin.h"

#pragma comment(lib,"shlwapi.lib")


// ウィンドウクラス名
#define EQUALIZER_WINDOW_CLASS TEXT("TVTest Equalizer Window")

#ifndef M_PI
#define M_PI 3.14159265358979
#endif




// バンドパスフィルタクラス
class CBandPass
{
	enum {
		MAX_CHANNELS	= 6,	// 最大チャンネル数(5.1ch)
		MAX_FREQUENCY	= 16	// 最大周波数分割数
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
	//assert(Count < MAX_FREQUENCY);
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
	m_PreAmplifier=1.0;
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
	//assert(Index >=0 && Index <= m_EqualizerCount);
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

			short v;
			if (Value > 32767.0)
				v = 32767;
			else if (Value < -32768.0)
				v = - 32768;
			else
				v = (short)Value;
			*p++ = v;
		}
	}
	::LeaveCriticalSection(&m_Lock);
}




// プラグインクラス
class CEqualizer : public TVTest::CTVTestPlugin
{
	enum {
		NUM_FREQUENCY = 10,
		NUM_CUSTOM_PRESETS = 10
	};
	enum {
		SLIDER_WIDTH			= 16,
		SLIDER_HEIGHT			= 80,
		SLIDER_MARGIN			= 2,
		WINDOW_MARGIN			= 8,
		TEXT_HEIGHT				= 10,
		SLIDER_TEXT_MARGIN		= 2,
		SLIDER_BUTTON_MARGIN	= 4,
		BUTTON_WIDTH			= 48,
		BUTTON_HEIGHT			= TEXT_HEIGHT + 6,
		BUTTON_MARGIN			= 4,
		// ウィンドウのクライアント領域の大きさ
		WINDOW_WIDTH	= (NUM_FREQUENCY + 2) * (SLIDER_WIDTH + SLIDER_MARGIN) - SLIDER_MARGIN + WINDOW_MARGIN * 2,
		WINDOW_HEIGHT	= SLIDER_HEIGHT + SLIDER_TEXT_MARGIN + TEXT_HEIGHT + SLIDER_BUTTON_MARGIN + BUTTON_HEIGHT + WINDOW_MARGIN * 2
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

	TCHAR m_szIniFileName[MAX_PATH];
	bool m_fSettingsLoaded;
	bool m_fExecuted;
	HWND m_hwnd;
	COLORREF m_crBackColor;
	COLORREF m_crTextColor;
	HFONT m_hfont;
	POINT m_WindowPosition;
	CBandPass m_BandPass;
	bool m_fEnable;
	EqualizerSettings m_CurSettings;
	int m_CurSlider;
	EqualizerSettings m_CustomPresetList[NUM_CUSTOM_PRESETS];
	struct EqualizerPreset {
		LPCTSTR pszName;
		EqualizerSettings Setting;
	};
	static const EqualizerPreset m_PresetList[];
	static const double m_FreqTable[NUM_FREQUENCY-1];

	bool ReadPreset(LPCTSTR pszSection,LPCTSTR pszKeyName,EqualizerSettings *pSettings);
	bool WritePreset(LPCTSTR pszSection,LPCTSTR pszKeyName,const EqualizerSettings *pSettings) const;
	void LoadSettings();
	void SaveSettings() const;
	void EnableEqualizer(bool fEnable);
	void ResetSettings();
	void ApplySettings();
	void GetSliderRect(int Index,RECT *pRect,bool fBar=false) const;
	int MapSliderPos(int y) const;
	int CalcSliderPos(int Pos) const;
	void GetButtonRect(int Button,RECT *pRect) const;
	void GetColor();
	void OnButtonPush(int Button);
	static LRESULT CALLBACK EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData);
	static LRESULT CALLBACK AudioCallback(short *pData,DWORD Samples,int Channels,void *pClientData);
	static CEqualizer *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	CEqualizer();
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();
};


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
	, m_fExecuted(false)
	, m_hwnd(NULL)
	, m_fEnable(false)
{
	m_WindowPosition.x=0;
	m_WindowPosition.y=0;
	ResetSettings();
	for (int i=0;i<NUM_CUSTOM_PRESETS;i++) {
		m_CustomPresetList[i].PreAmplifier=0;
		for (int j=0;j<NUM_FREQUENCY;j++)
			m_CustomPresetList[i].Frequency[j]=0;
	}
}


// プラグインの情報を返す
bool CEqualizer::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"Equalizer";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"簡易イコライザ";
	return true;
}


// 初期化処理
bool CEqualizer::Initialize()
{
	// INIファイル名の取得
	::GetModuleFileName(g_hinstDLL,m_szIniFileName,MAX_PATH);
	::PathRenameExtension(m_szIniFileName,TEXT(".ini"));

	// ウィンドウクラスの登録
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
	wc.lpszClassName=EQUALIZER_WINDOW_CLASS;
	if (::RegisterClass(&wc)==0)
		return false;

	// イコライザ初期化
	m_BandPass.Initialize(m_FreqTable,NUM_FREQUENCY-1,48000);

	// コマンドを登録
	m_pApp->RegisterCommand(1, L"Enable", L"イコライザOn/Off");

	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(EventCallback,this);

	bool fEnable=::GetPrivateProfileInt(TEXT("Settings"),TEXT("Enable"),0,m_szIniFileName)!=0;
	if (fEnable) {
		LoadSettings();
		EnableEqualizer(true);
	}

	return true;
}


// 終了処理
bool CEqualizer::Finalize()
{
	// ウィンドウの破棄
	if (m_hwnd!=NULL)
		::DestroyWindow(m_hwnd);

	if (m_fExecuted)
		SaveSettings();

	return true;
}


bool CEqualizer::ReadPreset(LPCTSTR pszSection,LPCTSTR pszKeyName,EqualizerSettings *pSettings)
{
	TCHAR szText[(NUM_FREQUENCY+1)*4+32];

	if (::GetPrivateProfileString(pszSection,pszKeyName,NULL,szText,
							sizeof(szText)/sizeof(WCHAR),m_szIniFileName)==0)
		return false;

	int i;
	LPCTSTR p=szText;

	for (i=-1;i<NUM_FREQUENCY && *p!='\0';i++) {
		while ((*p<'0' || *p>'9') && *p!='-' && *p!='\0')
			p++;
		if (*p=='\0')
			break;
		int Value=0;
		bool fNegative=false;
		if (*p=='-') {
			fNegative=true;
			p++;
		}
		while (*p>='0' && *p<='9') {
			Value=Value*10+(*p-'0');
			p++;
		}
		if (fNegative)
			Value=-Value;
		if (Value>10)
			Value=10;
		else if (Value<-10)
			Value=-10;
		if (i<0)
			pSettings->PreAmplifier=Value;
		else
			pSettings->Frequency[i]=Value;
	}
	if (i<0) {
		pSettings->PreAmplifier=0;
		i=0;
	}
	for (;i<NUM_FREQUENCY;i++)
		pSettings->Frequency[i]=0;
	return true;
}


bool CEqualizer::WritePreset(LPCTSTR pszSection,LPCTSTR pszKeyName,const EqualizerSettings *pSettings) const
{
	TCHAR szText[(NUM_FREQUENCY+1)*4+1];
	int Length;

	Length=::wsprintf(szText,TEXT("%d,"),pSettings->PreAmplifier);
	for (int i=0;i<NUM_FREQUENCY;i++)
		Length+=::wsprintf(szText+Length,TEXT("%d,"),pSettings->Frequency[i]);
	szText[Length-1]='\0';
	return ::WritePrivateProfileString(pszSection,pszKeyName,szText,m_szIniFileName)!=FALSE;
}


// 設定読み込み
void CEqualizer::LoadSettings()
{
	if (!m_fSettingsLoaded) {
		m_WindowPosition.x=::GetPrivateProfileInt(TEXT("Settings"),
						TEXT("WindowLeft"),m_WindowPosition.x,m_szIniFileName);
		m_WindowPosition.y=::GetPrivateProfileInt(TEXT("Settings"),
						TEXT("WindowTop"),m_WindowPosition.y,m_szIniFileName);
		ReadPreset(TEXT("Settings"),TEXT("CurSetting"),&m_CurSettings);

		int Count=::GetPrivateProfileInt(TEXT("Preset"),TEXT("Count"),0,m_szIniFileName);
		if (Count>NUM_CUSTOM_PRESETS)
			Count=NUM_CUSTOM_PRESETS;
		for (int i=0;i<Count;i++) {
			TCHAR szName[16];
			::wsprintf(szName,TEXT("Preset%d"),i);
			ReadPreset(TEXT("Preset"),szName,&m_CustomPresetList[i]);
		}

		m_fSettingsLoaded=true;
	}
}


BOOL WritePrivateProfileInt(LPCTSTR pszAppName,LPCTSTR pszKeyName,int Value,LPCTSTR pszFileName)
{
	TCHAR szValue[16];

	wsprintf(szValue,TEXT("%d"),Value);
	return WritePrivateProfileString(pszAppName,pszKeyName,szValue,pszFileName);
}

// 設定保存
void CEqualizer::SaveSettings() const
{
	::WritePrivateProfileInt(TEXT("Settings"),TEXT("Enable"),m_fEnable,m_szIniFileName);
	::WritePrivateProfileInt(TEXT("Settings"),TEXT("WindowLeft"),m_WindowPosition.x,m_szIniFileName);
	::WritePrivateProfileInt(TEXT("Settings"),TEXT("WindowTop"),m_WindowPosition.y,m_szIniFileName);
	WritePreset(TEXT("Settings"),TEXT("CurSetting"),&m_CurSettings);

	::WritePrivateProfileInt(TEXT("Preset"),TEXT("Count"),NUM_CUSTOM_PRESETS,m_szIniFileName);
	for (int i=0;i<NUM_CUSTOM_PRESETS;i++) {
		TCHAR szName[16];
		::wsprintf(szName,TEXT("Preset%d"),i);
		WritePreset(TEXT("Preset"),szName,&m_CustomPresetList[i]);
	}
}


// イコライザのOn/Off切り替え
void CEqualizer::EnableEqualizer(bool fEnable)
{
	if (m_fEnable!=fEnable) {
		if (fEnable) {
			m_BandPass.Reset();
			ApplySettings();
			// 音声コールバックを登録
			m_pApp->SetAudioCallback(AudioCallback,this);
		} else {
			// 音声コールバックを登録解除
			m_pApp->SetAudioCallback(NULL);
		}
		m_fEnable=fEnable;
		if (m_hwnd!=NULL)
			::InvalidateRect(m_hwnd,NULL,TRUE);
	}
}


// イコライザの設定を初期化
void CEqualizer::ResetSettings()
{
	m_CurSettings.PreAmplifier=0;
	for (int i=0;i<NUM_FREQUENCY;i++)
		m_CurSettings.Frequency[i]=0;
}


// イコライザの設定を適用
void CEqualizer::ApplySettings()
{
	m_BandPass.SetPreAmplifier((double)(m_CurSettings.PreAmplifier+10)*0.1);
	for (int i=0;i<NUM_FREQUENCY;i++)
		m_BandPass.SetVolume(i,(double)(m_CurSettings.Frequency[i]+10)*0.1);
}


// スライダの矩形を取得
void CEqualizer::GetSliderRect(int Index,RECT *pRect,bool fBar) const
{
	int x;

	if (Index<0)
		x=0;
	else
		x=Index+2;
	pRect->left=WINDOW_MARGIN+x*(SLIDER_WIDTH+SLIDER_MARGIN);
	pRect->right=pRect->left+SLIDER_WIDTH;
	pRect->top=WINDOW_MARGIN;
	pRect->bottom=WINDOW_MARGIN+SLIDER_HEIGHT;
	if (fBar) {
		pRect->left+=2;
		pRect->top+=2;
		pRect->right-=2;
		pRect->bottom-=2;
	}
}


// カーソル位置からスライダの位置を求める
int CEqualizer::MapSliderPos(int y) const
{
	y-=WINDOW_MARGIN+2;
	if (y<0)
		y=0;
	else if (y>=SLIDER_HEIGHT-4)
		y=SLIDER_HEIGHT-4-1;
	return 10-(y*20)/(SLIDER_HEIGHT-4-1);
}


// スライダの位置から描画位置を求める
int CEqualizer::CalcSliderPos(int Pos) const
{
	return WINDOW_MARGIN+SLIDER_HEIGHT-2-(Pos+10)*(SLIDER_HEIGHT-4)/20;
}


// ボタンの位置を取得
void CEqualizer::GetButtonRect(int Button,RECT *pRect) const
{
	pRect->left=WINDOW_MARGIN+(BUTTON_WIDTH+BUTTON_MARGIN)*Button;
	pRect->right=pRect->left+BUTTON_WIDTH;
	pRect->top=WINDOW_MARGIN+SLIDER_HEIGHT+SLIDER_TEXT_MARGIN+TEXT_HEIGHT+SLIDER_BUTTON_MARGIN;
	pRect->bottom=pRect->top+BUTTON_HEIGHT;
}


// 配色を取得
void CEqualizer::GetColor()
{
	m_crBackColor=m_pApp->GetColor(L"PanelBack");
	m_crTextColor=m_pApp->GetColor(L"PanelText");
}


// ボタンが押された
void CEqualizer::OnButtonPush(int Button)
{
	switch (Button) {
	case BUTTON_ENABLE:
		// On/Off切り替え
		EnableEqualizer(!m_fEnable);
		::UpdateWindow(m_hwnd);
		break;

	case BUTTON_RESET:
		// リセット
		ResetSettings();
		ApplySettings();
		::InvalidateRect(m_hwnd,NULL,TRUE);
		::UpdateWindow(m_hwnd);
		break;

	case BUTTON_LOAD:
	case BUTTON_SAVE:
		// プリセットの読み込み/保存
		{
			HMENU hmenu=::CreatePopupMenu();

			for (int i=0;i<NUM_CUSTOM_PRESETS;i++) {
				TCHAR szText[16];
				::wsprintf(szText,TEXT("Preset %d"),i+1);
				::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,i+1,szText);
			}
			if (Button==BUTTON_LOAD) {
				for (int i=0;i<sizeof(m_PresetList)/sizeof(EqualizerPreset);i++) {
					UINT Flags=MFT_STRING | MFS_ENABLED;
					if (i==0)
						Flags|=MF_MENUBREAK;
					::AppendMenu(hmenu,Flags,NUM_CUSTOM_PRESETS+1+i,m_PresetList[i].pszName);
				}
			}
			RECT rc;
			POINT pt;
			GetButtonRect(Button,&rc);
			pt.x=rc.left;
			pt.y=rc.bottom;
			::ClientToScreen(m_hwnd,&pt);
			int Result=::TrackPopupMenu(hmenu,TPM_RIGHTBUTTON | TPM_RETURNCMD,pt.x,pt.y,0,m_hwnd,NULL);
			::DestroyMenu(hmenu);
			if (Result>0) {
				Result--;
				if (Button==BUTTON_LOAD) {
					if (Result<NUM_CUSTOM_PRESETS)
						m_CurSettings=m_CustomPresetList[Result];
					else
						m_CurSettings=m_PresetList[Result-NUM_CUSTOM_PRESETS].Setting;
					if (m_fEnable) {
						ApplySettings();
						::InvalidateRect(m_hwnd,NULL,TRUE);
					} else {
						EnableEqualizer(true);
					}
					::UpdateWindow(m_hwnd);
				} else {
					m_CustomPresetList[Result]=m_CurSettings;
				}
			}
		}
		break;
	}
}


// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CEqualizer::EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
{
	CEqualizer *pThis=static_cast<CEqualizer*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// プラグインの有効状態が変化した
		if (lParam1!=0) {
			// プラグインが有効にされたのでウィンドウを作成する
			if (pThis->m_hwnd==NULL) {
				RECT rc;

				pThis->LoadSettings();
				::SetRect(&rc,0,0,WINDOW_WIDTH,WINDOW_HEIGHT);
				::AdjustWindowRectEx(&rc,WS_POPUP | WS_CAPTION | WS_SYSMENU,
									 FALSE,WS_EX_TOOLWINDOW);
				if (::CreateWindowEx(WS_EX_TOOLWINDOW,EQUALIZER_WINDOW_CLASS,
						TEXT("Equalizer"),
						WS_POPUP | WS_CAPTION | WS_SYSMENU,
						pThis->m_WindowPosition.x,pThis->m_WindowPosition.y,
						rc.right-rc.left,rc.bottom-rc.top,
						pThis->m_pApp->GetAppWindow(),NULL,g_hinstDLL,pThis)==NULL)
					return FALSE;
				pThis->m_fExecuted=true;
			}
			::ShowWindow(pThis->m_hwnd,SW_SHOWNORMAL);
			::UpdateWindow(pThis->m_hwnd);
		} else {
			// プラグインが無効にされたのでウィンドウを破棄する
			if (pThis->m_hwnd!=NULL)
				::DestroyWindow(pThis->m_hwnd);
		}
		return TRUE;

	case TVTest::EVENT_STANDBY:
		// 待機状態が変化した
		if (pThis->m_pApp->IsPluginEnabled()) {
			// 待機状態の時はウィンドウを隠す
			::ShowWindow(pThis->m_hwnd,lParam1!=0?SW_HIDE:SW_SHOW);
		}
		return TRUE;

	case TVTest::EVENT_COMMAND:
		// コマンドが選択された
		pThis->EnableEqualizer(!pThis->m_fEnable);
		return TRUE;

	case TVTest::EVENT_COLORCHANGE:
		// 色の設定が変化した
		if (pThis->m_hwnd!=NULL) {
			// 新しい配色を適用する
			pThis->GetColor();
			::RedrawWindow(pThis->m_hwnd,NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW);
		}
		return TRUE;
	}
	return 0;
}


// 音声コールバック関数
LRESULT CALLBACK CEqualizer::AudioCallback(short *pData,DWORD Samples,int Channels,void *pClientData)
{
	CEqualizer *pThis=static_cast<CEqualizer*>(pClientData);

	// イコライザー処理
	pThis->m_BandPass.ProcessSamples(pData,Samples,Channels);

	return 0;
}


// ウィンドウハンドルからthisポインタを取得する
CEqualizer *CEqualizer::GetThis(HWND hwnd)
{
	return reinterpret_cast<CEqualizer*>(::GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


// ウィンドウプロシージャ
LRESULT CALLBACK CEqualizer::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs=reinterpret_cast<LPCREATESTRUCT>(lParam);
			CEqualizer *pThis=static_cast<CEqualizer*>(pcs->lpCreateParams);

			::SetWindowLongPtr(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(pThis));
			pThis->m_hwnd=hwnd;
			pThis->GetColor();

			// フォント作成
			LOGFONT lf;
			::GetObject(::GetStockObject(DEFAULT_GUI_FONT),sizeof(LOGFONT),&lf);
			lf.lfHeight=-TEXT_HEIGHT;
			pThis->m_hfont=::CreateFontIndirect(&lf);

			// ウィンドウを最初に表示した時にイコライザーを有効化する
			if (!pThis->m_fEnable && !pThis->m_fExecuted)
				pThis->EnableEqualizer(true);
		}
		return 0;

	case WM_PAINT:
		{
			CEqualizer *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;
			HBRUSH hbr,hbrOld;
			HPEN hpen,hpenOld;
			HFONT hfontOld;
			COLORREF crOldTextColor;
			int OldBkMode;
			RECT rc,rcSlider,rcText;
			int i;
			TCHAR szText[16];

			::BeginPaint(hwnd,&ps);
			hbr=::CreateSolidBrush(pThis->m_crBackColor);
			::FillRect(ps.hdc,&ps.rcPaint,hbr);
			::DeleteObject(hbr);

			hbr=::CreateSolidBrush(
				pThis->m_fEnable?pThis->m_crTextColor:
				RGB((GetRValue(pThis->m_crTextColor)+GetRValue(pThis->m_crBackColor))/2,
					(GetGValue(pThis->m_crTextColor)+GetGValue(pThis->m_crBackColor))/2,
					(GetBValue(pThis->m_crTextColor)+GetBValue(pThis->m_crBackColor))/2));
			hpen=::CreatePen(PS_SOLID,1,pThis->m_crTextColor);
			hpenOld=static_cast<HPEN>(::SelectObject(ps.hdc,hpen));
			hbrOld=static_cast<HBRUSH>(::SelectObject(ps.hdc,::GetStockObject(NULL_BRUSH)));
			hfontOld=static_cast<HFONT>(::SelectObject(ps.hdc,pThis->m_hfont));
			crOldTextColor=::SetTextColor(ps.hdc,pThis->m_crTextColor);
			OldBkMode=::SetBkMode(ps.hdc,TRANSPARENT);

			// プリアンプ
			rc.left=WINDOW_MARGIN;
			rc.top=WINDOW_MARGIN;
			rc.right=rc.left+SLIDER_WIDTH;
			rc.bottom=rc.top+SLIDER_HEIGHT;
			::Rectangle(ps.hdc,rc.left,rc.top,rc.right,rc.bottom);
			rcSlider.left=rc.left+2;
			rcSlider.right=rc.right-2;
			rcSlider.bottom=rc.bottom-2;
			rcSlider.top=pThis->CalcSliderPos(pThis->m_CurSettings.PreAmplifier);
			if (rcSlider.top<rcSlider.bottom)
				::FillRect(ps.hdc,&rcSlider,hbr);
			rcText.left=rc.left-SLIDER_MARGIN/2;
			rcText.right=rc.right+SLIDER_MARGIN/2;
			rcText.top=rc.bottom+SLIDER_TEXT_MARGIN;
			rcText.bottom=rcText.top+TEXT_HEIGHT;
			::DrawText(ps.hdc,TEXT("Pre"),-1,&rcText,DT_CENTER | DT_SINGLELINE);

			// 目盛
			rc.left=rc.right+SLIDER_MARGIN;
			rc.right=rc.left+SLIDER_WIDTH;
			for (i=-10;i<=10;i+=5) {
				int y=pThis->CalcSliderPos(i);
				::MoveToEx(ps.hdc,rc.left+2,y,NULL);
				::LineTo(ps.hdc,rc.right-2,y);
			}

			// 各周波数
			int Freq=3125;
			for (i=0;i<NUM_FREQUENCY;i++) {
				rc.left=rc.right+SLIDER_MARGIN;
				rc.right=rc.left+SLIDER_WIDTH;
				::Rectangle(ps.hdc,rc.left,rc.top,rc.right,rc.bottom);
				rcSlider.left=rc.left+2;
				rcSlider.right=rc.right-2;
				rcSlider.top=pThis->CalcSliderPos(pThis->m_CurSettings.Frequency[i]);
				if (rcSlider.top<rcSlider.bottom)
					::FillRect(ps.hdc,&rcSlider,hbr);
				rcText.left=rc.left-SLIDER_MARGIN/2;
				rcText.right=rc.right+SLIDER_MARGIN/2;
				if (Freq<100000)
					::wsprintf(szText,TEXT("%d"),Freq/100);
				else
					::wsprintf(szText,TEXT("%dk"),Freq/(1000*100));
				::DrawText(ps.hdc,szText,-1,&rcText,DT_CENTER | DT_SINGLELINE);
				Freq*=2;
			}

			// ボタンの描画
			static const LPCTSTR pszButtonText[NUM_BUTTONS] = {
				TEXT("On/Off"), TEXT("Flat"), TEXT("Load"), TEXT("Save")
			};
			for (i=0;i<NUM_BUTTONS;i++) {
				pThis->GetButtonRect(i,&rc);
				::Rectangle(ps.hdc,rc.left,rc.top,rc.right,rc.bottom);
				::DrawText(ps.hdc,pszButtonText[i],-1,&rc,DT_CENTER | DT_SINGLELINE | DT_VCENTER);
			}

			::DeleteObject(hbr);
			::SelectObject(ps.hdc,hpenOld);
			::DeleteObject(hpen);
			::SelectObject(ps.hdc,hfontOld);
			::SetTextColor(ps.hdc,crOldTextColor);
			::SetBkMode(ps.hdc,OldBkMode);
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			CEqualizer *pThis=GetThis(hwnd);
			POINT pt;
			RECT rc;

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			if (pt.y>=WINDOW_MARGIN && pt.y<WINDOW_MARGIN+SLIDER_HEIGHT) {
				for (int i=-1;i<NUM_FREQUENCY;i++) {
					pThis->GetSliderRect(i,&rc);
					if (::PtInRect(&rc,pt)) {
						// スライダ
						int Pos=pThis->MapSliderPos(pt.y);

						if (i<0)
							pThis->m_CurSettings.PreAmplifier=Pos;
						else
							pThis->m_CurSettings.Frequency[i]=Pos;
						if (pThis->m_fEnable) {
							pThis->ApplySettings();
							::InvalidateRect(hwnd,&rc,TRUE);
						} else {
							pThis->EnableEqualizer(true);
						}
						::UpdateWindow(hwnd);
						pThis->m_CurSlider=i;
						::SetCapture(hwnd);
						break;
					}
				}
			} else {
				for (int i=0;i<NUM_BUTTONS;i++) {
					pThis->GetButtonRect(i,&rc);
					if (::PtInRect(&rc,pt)) {
						// ボタンが押された
						pThis->OnButtonPush(i);
						break;
					}
				}
			}
		}
		return 0;

	case WM_LBUTTONUP:
		if (::GetCapture()==hwnd)
			// スライダのドラッグ終了
			::ReleaseCapture();
		return 0;

	case WM_MOUSEMOVE:
		if (::GetCapture()==hwnd) {
			// スライダのドラッグ
			CEqualizer *pThis=GetThis(hwnd);
			int Pos=pThis->MapSliderPos(GET_Y_LPARAM(lParam));

			if (pThis->m_CurSlider<0) {
				if (pThis->m_CurSettings.PreAmplifier==Pos)
					return 0;
				pThis->m_CurSettings.PreAmplifier=Pos;
			} else {
				if (pThis->m_CurSettings.Frequency[pThis->m_CurSlider]==Pos)
					return 0;
				pThis->m_CurSettings.Frequency[pThis->m_CurSlider]=Pos;
			}
			RECT rc;
			pThis->GetSliderRect(pThis->m_CurSlider,&rc,true);
			::InvalidateRect(hwnd,&rc,TRUE);
			::UpdateWindow(hwnd);
			pThis->ApplySettings();
		}
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT) {
			// カーソル設定
			CEqualizer *pThis=GetThis(hwnd);
			DWORD Pos=::GetMessagePos();
			POINT pt;
			RECT rc;

			pt.x=GET_X_LPARAM(Pos);
			pt.y=GET_Y_LPARAM(Pos);
			::ScreenToClient(hwnd,&pt);
			for (int i=-1;i<NUM_FREQUENCY;i++) {
				pThis->GetSliderRect(i,&rc);
				if (::PtInRect(&rc,pt)) {
					::SetCursor(::LoadCursor(NULL,IDC_HAND));
					return TRUE;
				}
			}
			for (int i=0;i<NUM_BUTTONS;i++) {
				pThis->GetButtonRect(i,&rc);
				if (::PtInRect(&rc,pt)) {
					::SetCursor(::LoadCursor(NULL,IDC_HAND));
					return TRUE;
				}
			}
		}
		break;

	case WM_SYSCOMMAND:
		if ((wParam&0xFFF0)==SC_CLOSE) {
			// 閉じる時はプラグインを無効にする
			// (勝手に閉じるとTVTestとの整合性が取れなくなるため)
			CEqualizer *pThis=GetThis(hwnd);

			pThis->m_pApp->EnablePlugin(false);
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			CEqualizer *pThis=GetThis(hwnd);

			// 後始末
			::DeleteObject(pThis->m_hfont);

			// ウィンドウ位置保存
			RECT rc;
			::GetWindowRect(hwnd,&rc);
			pThis->m_WindowPosition.x=rc.left;
			pThis->m_WindowPosition.y=rc.top;

			pThis->m_hwnd=NULL;
		}
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}




// プラグインクラスのインスタンスを生成する
TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CEqualizer;
}
