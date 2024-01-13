/*
	TVTest プラグインサンプル (TVTest ver.0.9.0 以降)

	映像ストリームを保存して画像をキャプチャする

	このサンプルでは主に以下の機能を実装しています。

	・ウィンドウを表示する
	・映像ストリームを取得する
	・TVTest DTV Video Decoder を使って映像をデコードする
	・TVTest_Image.dll を使って画像を保存する
	・TVTest の設定を取得する
	・変数文字列を展開する
	・テーマを使って項目を描画する
	・ウィンドウの DPI に応じてスケーリングする
	・設定ダイアログを表示する
	・TVTest に合わせてウィンドウをダークモードにする
*/


#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <commdlg.h>
#include <process.h>
#include <algorithm>
#include <vector>
#include <string>
#include <new>
#include <strsafe.h>

#define TVTEST_PLUGIN_CLASS_IMPLEMENT // クラスとして実装
#include "TVTestPlugin.h"
#include "VideoDecoder.h"
#include "ImageCodec.h"
#include "PreviewWindow.h"
#include "SeekBar.h"
#include "Toolbar.h"
#include "resource.h"

#pragma comment(lib, "shlwapi.lib")


#define TITLE_TEXT TEXT("メモリーキャプチャー")


// 排他制御用クラス
class CLocalLock
{
public:
	CLocalLock() { ::InitializeCriticalSection(&m_CriticalSection); }
	~CLocalLock() { ::DeleteCriticalSection(&m_CriticalSection); }
	void Lock() { ::EnterCriticalSection(&m_CriticalSection); }
	void Unlock() { ::LeaveCriticalSection(&m_CriticalSection); }

private:
	CRITICAL_SECTION m_CriticalSection;
};

class CBlockLock
{
public:
	CBlockLock(CLocalLock &Lock) : m_Lock(Lock) { m_Lock.Lock(); }
	~CBlockLock() { m_Lock.Unlock(); }

private:
	CLocalLock &m_Lock;
};


// プラグインクラス
class CMemoryCapture
	: public TVTest::CTVTestPlugin
	, protected CVideoDecoder::CFrameCapture
{
public:
	CMemoryCapture();

	bool GetPluginInfo(TVTest::PluginInfo *pInfo) override;
	bool Initialize() override;
	bool Finalize() override;

private:
	typedef std::basic_string<TCHAR> String;

	struct Position
	{
		int Left = 0, Top = 0, Width = 0, Height = 0;
	};

	enum class CaptureSizeType
	{
		Size,
		Rate,
	};

	struct CaptureSizeInfo
	{
		CaptureSizeType Type;
		union {
			struct {
				int Width;
				int Height;
			} Size;
			struct {
				int Num;
				int Denom;
			} Rate;
		};

		bool operator==(const CaptureSizeInfo &rhs) const
		{
			return Type == rhs.Type
				&& Size.Width == rhs.Size.Width
				&& Size.Height == rhs.Size.Height;
		}
	};

	struct FrameGroupInfo
	{
		std::size_t FirstFrame;
		std::size_t FrameCount;
		TVTest::VarStringContext *pVarContext;
	};

	enum {
		COMMAND_CAPTURE = 1,
		COMMAND_CAPTURE_ADD,
		COMMAND_SAVE,
		COMMAND_COPY
	};

	enum {
		IDC_SEEK_BAR = 900,
		IDC_TOOLBAR
	};

	enum {
		TIMER_SEEK_START = 1,
		TIMER_SEEK
	};

	enum {
		WM_APP_DECODE_START = WM_APP,
		WM_APP_DECODE_END,
		WM_APP_FRAME_DECODED
	};

	static constexpr unsigned int VideoMemorySizeLimitInMB = 40;

	bool m_fInitialized = false;
	HWND m_hwnd = nullptr;
	Position m_WindowPosition;
	bool m_fFitWindowToImage = true;
	bool m_fAccumulateAlways = false;
	int m_DPI;
	int m_SkipFrames = 10;
	int m_CurFrame = -1;
	int m_SeekCommand = 0;
	int m_WheelDelta = 0;
	DWORD m_WheelTime = 0;
	CImageCodec m_Codec;
	CVideoDecoder m_Decoder;
	CPreviewWindow m_Preview;
	CSeekBar m_SeekBar;
	CToolbar m_Toolbar;
	CaptureSizeInfo m_CaptureSize;
	CImage::ResampleType m_Resample = CImage::ResampleType::Lanczos3;
	CVideoDecoder::DeinterlaceMethod m_Deinterlace = CVideoDecoder::DeinterlaceMethod::Blend;
	CLocalLock m_WindowLock;

	unsigned int m_VideoMemorySizeInMB = 2;
	BYTE *m_pStreamBuffer = nullptr;
	std::size_t m_StreamSize = 0;
	std::size_t m_StreamAvail = 0;
	std::size_t m_StreamPos = 0;
	DWORD m_StreamFormat = 0;
	CLocalLock m_StreamLock;

	std::vector<CImage*> m_ImageList;
	CLocalLock m_ImageLock;
	std::vector<FrameGroupInfo> m_FrameGroupList;
	BYTE *m_pDecodeBuffer = nullptr;
	std::size_t m_DecodeSize = 0;
	HANDLE m_hDecodeThread = nullptr;

	CImageCodec::FormatType m_SaveFormat = CImageCodec::FormatType::JPEG;
	CImageCodec::FormatType m_LastSaveFormat = CImageCodec::FormatType::JPEG;
	String m_LastSaveFileName;
	String m_LastSaveFolder;

	static const LPCTSTR m_WindowClassName;
	static const CaptureSizeInfo m_CaptureSizeList[];
	static const CaptureSizeInfo m_ZoomRateList[];

	bool EnablePlugin(bool fEnable);
	void RegisterCommand(int ID, LPCWSTR pszText, LPCWSTR pszName, LPCWSTR pszDescription, int IconID = 0);
	void SaveSettings();
	void LoadSettings();
	void LoadAppSettings();
	bool AllocateStreamBuffer();
	void FreeStreamBuffer();
	void InputStream(DWORD Format, const void *pData, std::size_t Size);
	bool StartCapture(bool fAdd);
	void CloseDecodeThread();
	void FreeImages();
	const CImage *GetFrameImage(int Frame);
	const CImage *GetCurImage() { return GetFrameImage(m_CurFrame); }
	void SetCurFrame(int Frame);
	const FrameGroupInfo *GetFrameGroupInfo(int Frame) const;
	bool SaveImageToFile(const CImage *pImage, LPCWSTR pszFileName, CImageCodec::FormatType Format);
	bool CopyImageToClipboard(const CImage *pImage) const;
	void GetCaptureImageSize(const CImage *pImage, int *pWidth, int *pHeight) const;
	void SetCaption();
	void AdjustWindowSize();
	void PostNotifyMessage(UINT Message, WPARAM wParam = 0, LPARAM lParam = 0);
	void SetToolbarImage();
	void StartSeeking(int Command);
	void StopSeeking();
	bool GetSaveFolder(LPWSTR pszFolder);
	bool GetSaveFileName(
		LPWSTR pszFileName, LPCWSTR pszFolder,
		const CImage *pImage, const FrameGroupInfo *pGroup,
		int *pSequentialNumber = nullptr);
	bool SaveCurrent();
	bool SaveAs();
	bool SaveAll();
	void OnWindowCreate();
	void OnWindowDestroy();
	void OnCommand(int Command, int NotifyCode);
	bool OnPluginCommand(int Command);
	void ShowContextMenu(int x, int y);
	bool SettingsDialog(HWND hwndOwner);
	HWND GetOwnerWindow() const { return m_hwnd != nullptr ? m_hwnd : m_pApp->GetAppWindow(); }

	bool OnFrame(const CVideoDecoder::FrameInfo &Frame) override;

	static LRESULT CALLBACK EventCallback(
		UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);
	static LRESULT CALLBACK VideoStreamCallback(
		DWORD Format, const void *pData, SIZE_T Size, void *pClientData);
	static unsigned int __stdcall DecodeThread(void *pParameter);
	static CMemoryCapture * GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK SettingsDlgProc(
		HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, void *pClientData);
};


// ウィンドウクラス名
const LPCTSTR CMemoryCapture::m_WindowClassName = TEXT("TVTest Memory Capture Window");

// キャプチャサイズのリスト
const CMemoryCapture::CaptureSizeInfo CMemoryCapture::m_CaptureSizeList[] =
{
	// 倍率
	{CaptureSizeType::Rate, {1, 4}},
	{CaptureSizeType::Rate, {1, 3}},
	{CaptureSizeType::Rate, {1, 2}},
	{CaptureSizeType::Rate, {2, 3}},
	{CaptureSizeType::Rate, {3, 4}},
	{CaptureSizeType::Rate, {1, 1}},
	// 16:9
	{CaptureSizeType::Size, { 320,  180}},
	{CaptureSizeType::Size, { 640,  360}},
	{CaptureSizeType::Size, { 800,  450}},
	{CaptureSizeType::Size, { 960,  540}},
	{CaptureSizeType::Size, {1280,  720}},
	{CaptureSizeType::Size, {1440,  810}},
	{CaptureSizeType::Size, {1920, 1080}},
	// 4:3
	{CaptureSizeType::Size, { 320,  240}},
	{CaptureSizeType::Size, { 640,  480}},
	{CaptureSizeType::Size, { 720,  540}},
	{CaptureSizeType::Size, { 800,  600}},
	{CaptureSizeType::Size, {1024,  768}},
	{CaptureSizeType::Size, {1280,  960}},
	{CaptureSizeType::Size, {1440, 1080}},
};

// 表示倍率のリスト
const CMemoryCapture::CaptureSizeInfo CMemoryCapture::m_ZoomRateList[] =
{
	{CaptureSizeType::Rate, {1, 4}},
	{CaptureSizeType::Rate, {1, 3}},
	{CaptureSizeType::Rate, {1, 2}},
	{CaptureSizeType::Rate, {2, 3}},
	{CaptureSizeType::Rate, {3, 4}},
	{CaptureSizeType::Rate, {1, 1}},
	{CaptureSizeType::Rate, {3, 2}},
	{CaptureSizeType::Rate, {2, 1}},
};


CMemoryCapture::CMemoryCapture()
{
	m_CaptureSize.Type = CaptureSizeType::Rate;
	m_CaptureSize.Rate.Num = 1;
	m_CaptureSize.Rate.Denom = 1;
}


// プラグインの情報を返す
bool CMemoryCapture::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = TVTest::PLUGIN_FLAG_HASSETTINGS; // 設定ダイアログあり
	pInfo->pszPluginName  = L"メモリーキャプチャー";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"画像をキャプチャします。";
	return true;
}


// 初期化処理
bool CMemoryCapture::Initialize()
{
	// アイコンを登録
	m_pApp->RegisterPluginIconFromResource(g_hinstDLL, MAKEINTRESOURCE(IDB_ICON));

	// コマンドを登録
	RegisterCommand(COMMAND_CAPTURE, L"Capture", L"取り込み", L"画像の取り込みを行います。", IDB_CAPTURE);
	RegisterCommand(COMMAND_CAPTURE_ADD, L"CaptureAdd", L"追加取り込み", L"画像を追加で取り込みます。", IDB_CAPTURE_ADD);
	RegisterCommand(COMMAND_SAVE, L"Save", L"保存", L"現在の画像を保存します。");
	RegisterCommand(COMMAND_COPY, L"Copy", L"コピー", L"現在の画像をコピーします。");

	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(EventCallback, this);

	// 設定を読み込む
	LoadSettings();
	LoadAppSettings();

	m_Decoder.SetFrameCapture(this);

	if (m_fAccumulateAlways) {
		AllocateStreamBuffer();
		m_pApp->SetVideoStreamCallback(VideoStreamCallback, this);
	}

	return true;
}


// 終了処理
bool CMemoryCapture::Finalize()
{
	// ウィンドウの破棄
	if (m_hwnd != nullptr)
		::DestroyWindow(m_hwnd);

	FreeStreamBuffer();

	m_Decoder.Finalize();

	// 設定を保存
	if (m_fInitialized)
		SaveSettings();

	return true;
}


// プラグインの有効/無効の切り替え
bool CMemoryCapture::EnablePlugin(bool fEnable)
{
	if (fEnable) {
		if (!m_fInitialized) {
			// ウィンドウクラスの登録
			WNDCLASSEX wc = {};

			wc.cbSize = sizeof(WNDCLASSEX);
			wc.lpfnWndProc = WndProc;
			wc.hInstance = g_hinstDLL;
			wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
			wc.lpszClassName = m_WindowClassName;
			wc.hIcon = static_cast<HICON>(
				::LoadImage(
					g_hinstDLL, MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON,
					::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON),
					LR_SHARED));
			wc.hIconSm = static_cast<HICON>(
				::LoadImage(
					g_hinstDLL, MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON,
					::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON),
					LR_SHARED));
			if (::RegisterClassEx(&wc) == 0)
				return false;

			CPreviewWindow::Initialize(g_hinstDLL);
			CSeekBar::Initialize(g_hinstDLL);
			CToolbar::Initialize(g_hinstDLL);

			// ツールバーのボタンを設定
			static const CToolbar::ItemInfo ToolbarItemList[] =
			{
				{CM_CAPTURE,             0, 0},
				{CM_CAPTURE_ADD,         1, 0},
				{CM_PREV_FRAME,          2, CToolbar::ItemFlag_NotifyPress},
				{CM_NEXT_FRAME,          3, CToolbar::ItemFlag_NotifyPress},
				{CM_SKIP_BACKWARD_FRAME, 4, CToolbar::ItemFlag_NotifyPress},
				{CM_SKIP_FORWARD_FRAME,  5, CToolbar::ItemFlag_NotifyPress},
				{CM_FIRST_FRAME,         6, 0},
				{CM_LAST_FRAME,          7, 0},
				{CM_SAVE,                8, 0},
				{CM_COPY,                9, 0},
			};
			for (int i = 0; i < _countof(ToolbarItemList); i++)
				m_Toolbar.AddItem(ToolbarItemList[i]);

			m_fInitialized = true;
		}

		if (m_hwnd == nullptr) {
			constexpr DWORD Style = WS_OVERLAPPEDWINDOW;
			constexpr DWORD ExStyle = 0;

			// プライマリモニタの DPI を取得
			m_DPI = m_pApp->GetDPIFromPoint(0, 0);
			if (m_DPI == 0)
				m_DPI = 96;

			// デフォルトのウィンドウサイズを取得
			if (m_WindowPosition.Width <= 0 || m_WindowPosition.Height <= 0) {
				int Width = 640, Height = 360;
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
					ExStyle, m_WindowClassName, TITLE_TEXT, Style,
					0, 0, m_WindowPosition.Width, m_WindowPosition.Height,
					/*m_pApp->GetAppWindow()*/nullptr, nullptr, g_hinstDLL, this) == nullptr)
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
			::ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
		}
		::UpdateWindow(m_hwnd);
	} else {
		if (m_hwnd != nullptr)
			::SendMessage(m_hwnd, WM_CLOSE, 0, 0);
	}

	return true;
}


// プラグインのコマンドを登録する
void CMemoryCapture::RegisterCommand(
	int ID, LPCWSTR pszText, LPCWSTR pszName, LPCWSTR pszDescription, int IconID)
{
	TVTest::PluginCommandInfo CommandInfo;

	CommandInfo.Size = sizeof(CommandInfo);
	CommandInfo.Flags = 0;
	CommandInfo.State = 0;
	CommandInfo.ID = ID;
	CommandInfo.pszText = pszText;
	CommandInfo.pszName = pszName;
	CommandInfo.pszDescription = pszDescription;
	if (IconID != 0) {
		CommandInfo.Flags |= TVTest::PLUGIN_COMMAND_FLAG_ICONIZE;
		CommandInfo.hbmIcon =
			static_cast<HBITMAP>(
				::LoadImage(
					g_hinstDLL, MAKEINTRESOURCE(IconID),
					IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
	} else {
		CommandInfo.hbmIcon = nullptr;
	}

	m_pApp->RegisterPluginCommand(&CommandInfo);

	::DeleteObject(CommandInfo.hbmIcon);
}


// 設定を保存する
void CMemoryCapture::SaveSettings()
{
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
	::WritePrivateProfileString(
		TEXT("Settings"), TEXT("Resample"),
		IntString((int)m_Resample), szIniFileName);
	::WritePrivateProfileString(
		TEXT("Settings"), TEXT("Deinterlace"),
		IntString((int)m_Deinterlace), szIniFileName);
	::WritePrivateProfileString(
		TEXT("Settings"), TEXT("FitWindowToImage"),
		IntString(m_fFitWindowToImage), szIniFileName);
	::WritePrivateProfileString(
		TEXT("Settings"), TEXT("AccumulateAlways"),
		IntString(m_fAccumulateAlways), szIniFileName);
	::WritePrivateProfileString(
		TEXT("Settings"), TEXT("VideoMemorySizeInMB"),
		IntString(m_VideoMemorySizeInMB), szIniFileName);

	::WritePrivateProfileString(
		TEXT("Settings"), TEXT("CaptureSizeType"),
		IntString((int)m_CaptureSize.Type), szIniFileName);
	if (m_CaptureSize.Type == CaptureSizeType::Size) {
		::WritePrivateProfileString(
			TEXT("Settings"), TEXT("CaptureWidth"),
			IntString(m_CaptureSize.Size.Width), szIniFileName);
		::WritePrivateProfileString(
			TEXT("Settings"), TEXT("CaptureHeight"),
			IntString(m_CaptureSize.Size.Height), szIniFileName);
	} else {
		::WritePrivateProfileString(
			TEXT("Settings"), TEXT("CaptureRateNum"),
			IntString(m_CaptureSize.Rate.Num), szIniFileName);
		::WritePrivateProfileString(
			TEXT("Settings"), TEXT("CaptureRateDenom"),
			IntString(m_CaptureSize.Rate.Denom), szIniFileName);
	}

	int ZoomNum, ZoomDenom;
	m_Preview.GetZoomRate(&ZoomNum, &ZoomDenom);
	::WritePrivateProfileString(
		TEXT("Settings"), TEXT("ZoomNum"),
		IntString(ZoomNum), szIniFileName);
	::WritePrivateProfileString(
		TEXT("Settings"), TEXT("ZoomDenom"),
		IntString(ZoomDenom), szIniFileName);
	::WritePrivateProfileString(
		TEXT("Settings"), TEXT("FitImageToWindow"),
		IntString(m_Preview.GetFitImageToWindow()), szIniFileName);

	::WritePrivateProfileString(
		TEXT("Settings"), TEXT("LastSaveFileName"),
		m_LastSaveFileName.c_str(), szIniFileName);
	::WritePrivateProfileString(
		TEXT("Settings"), TEXT("LastSaveFolder"),
		m_LastSaveFolder.c_str(), szIniFileName);
}


// 設定を読み込む
void CMemoryCapture::LoadSettings()
{
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

	m_SkipFrames =
		::GetPrivateProfileInt(TEXT("Settings"), TEXT("SkipFrames"), m_SkipFrames, szIniFileName);
	m_Resample = (CImage::ResampleType)
		::GetPrivateProfileInt(TEXT("Settings"), TEXT("Resample"), (int)m_Resample, szIniFileName);
	m_Deinterlace = (CVideoDecoder::DeinterlaceMethod)
		::GetPrivateProfileInt(TEXT("Settings"), TEXT("Deinterlace"), (int)m_Deinterlace, szIniFileName);
	m_fFitWindowToImage =
		::GetPrivateProfileInt(TEXT("Settings"), TEXT("FitWindowToImage"), m_fFitWindowToImage, szIniFileName) != 0;
	m_fAccumulateAlways =
		::GetPrivateProfileInt(TEXT("Settings"), TEXT("AccumulateAlways"), m_fAccumulateAlways, szIniFileName) != 0;
	m_VideoMemorySizeInMB =
		::GetPrivateProfileInt(TEXT("Settings"), TEXT("VideoMemorySizeInMB"), m_VideoMemorySizeInMB, szIniFileName);
	if (m_VideoMemorySizeInMB < 1)
		m_VideoMemorySizeInMB = 1;
	else if (m_VideoMemorySizeInMB > VideoMemorySizeLimitInMB)
		m_VideoMemorySizeInMB = VideoMemorySizeLimitInMB;

	CaptureSizeInfo CaptureSize;
	CaptureSize.Type = (CaptureSizeType)
		::GetPrivateProfileInt(TEXT("Settings"), TEXT("CaptureSizeType"), -1, szIniFileName);
	if (CaptureSize.Type == CaptureSizeType::Size) {
		CaptureSize.Size.Width =
			::GetPrivateProfileInt(TEXT("Settings"), TEXT("CaptureWidth"), 0, szIniFileName);
		CaptureSize.Size.Height =
			::GetPrivateProfileInt(TEXT("Settings"), TEXT("CaptureHeight"), 0, szIniFileName);
		if (CaptureSize.Size.Width > 0 && CaptureSize.Size.Height > 0)
			m_CaptureSize = CaptureSize;
	} else if (CaptureSize.Type == CaptureSizeType::Rate) {
		CaptureSize.Rate.Num =
			::GetPrivateProfileInt(TEXT("Settings"), TEXT("CaptureRateNum"), 0, szIniFileName);
		CaptureSize.Rate.Denom =
			::GetPrivateProfileInt(TEXT("Settings"), TEXT("CaptureRateDenom"), 0, szIniFileName);
		if (CaptureSize.Rate.Num > 0 && CaptureSize.Rate.Denom > 0)
			m_CaptureSize = CaptureSize;
	}

	int ZoomNum, ZoomDenom;
	ZoomNum =
		::GetPrivateProfileInt(TEXT("Settings"), TEXT("ZoomNum"), 0, szIniFileName);
	ZoomDenom =
		::GetPrivateProfileInt(TEXT("Settings"), TEXT("ZoomDenom"), 0, szIniFileName);
	if (ZoomNum > 0 && ZoomDenom > 0)
		m_Preview.SetZoomRate(ZoomNum, ZoomDenom);
	m_Preview.SetFitImageToWindow(
		::GetPrivateProfileInt(TEXT("Settings"), TEXT("FitImageToWindow"), m_Preview.GetFitImageToWindow(), szIniFileName) != 0);

	TCHAR szPath[MAX_PATH];
	if (::GetPrivateProfileString(TEXT("Settings"), TEXT("LastSaveFileName"), TEXT(""), szPath, _countof(szPath), szIniFileName) > 0)
		m_LastSaveFileName = szPath;
	if (::GetPrivateProfileString(TEXT("Settings"), TEXT("LastSaveFolder"), TEXT(""), szPath, _countof(szPath), szIniFileName) > 0)
		m_LastSaveFolder = szPath;
}


// TVTest の設定を取得する
void CMemoryCapture::LoadAppSettings()
{
	WCHAR szIniPath[MAX_PATH];

	if (m_pApp->GetSetting(L"IniFilePath", szIniPath, _countof(szIniPath)) > 0) {
		// 保存形式
		WCHAR szFormat[8];
		if (::GetPrivateProfileStringW(L"Settings", L"CaptureSaveFormat", L"", szFormat, _countof(szFormat), szIniPath) > 0) {
			CImageCodec::FormatType Format = m_Codec.ParseFormatName(szFormat);
			if (Format != CImageCodec::FormatType::Invalid)
				m_SaveFormat = Format;
		}

		// JPEG の品質
		int Value = ::GetPrivateProfileIntW(L"Settings", L"JpegQuality", 0, szIniPath);
		if (Value > 0 && Value <= 100)
			m_Codec.SetJpegQuality(Value);

		// PNG の圧縮レベル
		Value = ::GetPrivateProfileIntW(L"Settings", L"PngCompressionLevel", 0, szIniPath);
		if (Value >= 0 && Value <= 9)
			m_Codec.SetPngCompressionLevel(Value);
	}
}


// ストリーム保存用バッファを確保する
bool CMemoryCapture::AllocateStreamBuffer()
{
	CBlockLock Lock(m_StreamLock);

	FreeStreamBuffer();

	std::size_t Size = m_VideoMemorySizeInMB * (1024 * 1024);
	m_pStreamBuffer = new(std::nothrow) BYTE[Size];
	if (m_pStreamBuffer == nullptr) {
		m_StreamSize = 0;
		return false;
	}

	m_StreamSize = Size;

	return true;
}


// ストリーム保存用バッファを解放する
void CMemoryCapture::FreeStreamBuffer()
{
	CBlockLock Lock(m_StreamLock);

	if (m_pStreamBuffer != nullptr) {
		delete [] m_pStreamBuffer;
		m_pStreamBuffer = nullptr;
	}

	m_StreamSize = 0;
	m_StreamAvail = 0;
	m_StreamPos = 0;
}


// ストリームの受け取り
void CMemoryCapture::InputStream(DWORD Format, const void *pData, std::size_t Size)
{
	CBlockLock Lock(m_StreamLock);

	if (m_pStreamBuffer != nullptr) {
		// フォーマットが変わったらバッファを初期化する
		if (m_StreamFormat != Format) {
			m_StreamFormat = Format;
			m_StreamPos = 0;
			m_StreamAvail = 0;
		}

		// リングバッファに保存する
		if (Size >= m_StreamSize) {
			std::memcpy(m_pStreamBuffer, static_cast<const BYTE*>(pData) + (Size - m_StreamSize), Size);
			m_StreamPos = 0;
			m_StreamAvail = Size;
		} else {
			std::size_t EndPos = (m_StreamPos + m_StreamAvail) % m_StreamSize;
			std::size_t Remain = std::min(m_StreamSize - EndPos, Size);

			if (Remain > 0)
				std::memcpy(m_pStreamBuffer + EndPos, pData, Remain);
			if (Remain < Size)
				std::memcpy(m_pStreamBuffer, static_cast<const BYTE*>(pData) + Remain, Size - Remain);

			if (m_StreamAvail + Size <= m_StreamSize) {
				m_StreamAvail += Size;
			} else {
				m_StreamPos += m_StreamAvail + Size - m_StreamSize;
				m_StreamPos %= m_StreamSize;
				m_StreamAvail = m_StreamSize;
			}
		}
	}
}


// 画像取り込みの開始
bool CMemoryCapture::StartCapture(bool fAdd)
{
	CloseDecodeThread();

	if (!m_Decoder.Initialize()) {
		::MessageBox(
			GetOwnerWindow(),
			TEXT("デコーダーを初期化できません。"),
			nullptr,
			MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	// 追加でない場合は現在の画像を破棄
	if (!fAdd) {
		m_Preview.SetImage(nullptr);
		FreeImages();
		m_CurFrame = -1;
		SetCaption();
	}

	// デコード用バッファの用意
	{
		CBlockLock Lock(m_StreamLock);

		if (m_pStreamBuffer == nullptr || m_StreamAvail == 0)
			return false;

		m_pDecodeBuffer = new(std::nothrow) BYTE[m_StreamAvail];
		if (m_pDecodeBuffer == nullptr) {
			::MessageBox(
				GetOwnerWindow(),
				TEXT("メモリーを確保できません。"),
				nullptr,
				MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
		m_DecodeSize = m_StreamAvail;

		std::size_t Size = std::min(m_StreamSize - m_StreamPos, m_StreamAvail);
		std::memcpy(m_pDecodeBuffer, m_pStreamBuffer + m_StreamPos, Size);
		if (Size < m_StreamAvail)
			std::memcpy(m_pDecodeBuffer + Size, m_pStreamBuffer, m_StreamAvail - Size);
	}

	// 前回デコードできなかった場合
	if (!m_FrameGroupList.empty() && m_FrameGroupList.back().FrameCount == 0) {
		m_pApp->FreeVarStringContext(m_FrameGroupList.back().pVarContext);
		m_FrameGroupList.pop_back();
	}

	// ファイル名の生成に使う変数の情報を取得
	FrameGroupInfo Group;
	Group.FirstFrame = m_ImageList.size();
	Group.FrameCount = 0;
	Group.pVarContext = m_pApp->GetVarStringContext();
	m_FrameGroupList.push_back(Group);

	// デコードスレッドの開始
	m_hDecodeThread = reinterpret_cast<HANDLE>(
		::_beginthreadex(nullptr, 0, DecodeThread, this, 0, nullptr));

	return m_hDecodeThread != nullptr;
}


// デコードスレッドを閉じる
void CMemoryCapture::CloseDecodeThread()
{
	if (m_hDecodeThread != nullptr) {
		::WaitForSingleObject(m_hDecodeThread, INFINITE);
		::CloseHandle(m_hDecodeThread);
		m_hDecodeThread = nullptr;
	}

	if (m_pDecodeBuffer != nullptr) {
		delete [] m_pDecodeBuffer;
		m_pDecodeBuffer = nullptr;
	}
	m_DecodeSize = 0;
}


// 画像を解放する
void CMemoryCapture::FreeImages()
{
	for (auto it = m_ImageList.begin(); it != m_ImageList.end(); ++it)
		delete *it;
	m_ImageList.clear();

	for (auto it = m_FrameGroupList.begin(); it != m_FrameGroupList.end(); ++it)
		m_pApp->FreeVarStringContext(it->pVarContext);
	m_FrameGroupList.clear();
}


// 指定フレームの画像を取得する
const CImage *CMemoryCapture::GetFrameImage(int Frame)
{
	CBlockLock Lock(m_ImageLock);

	if (Frame < 0 || Frame >= (int)m_ImageList.size())
		return nullptr;
	return m_ImageList[Frame];
}


// 現在のフレームを設定する
void CMemoryCapture::SetCurFrame(int Frame)
{
	CBlockLock Lock(m_ImageLock);

	if (Frame >= 0 && Frame < (int)m_ImageList.size()) {
		m_CurFrame = Frame;
		m_Preview.SetImage(GetCurImage());
		m_SeekBar.SetBarPos(Frame);
		SetCaption();
	}
}


// フレームのグループの情報を取得する
const CMemoryCapture::FrameGroupInfo *CMemoryCapture::GetFrameGroupInfo(int Frame) const
{
	for (auto it = m_FrameGroupList.begin(); it != m_FrameGroupList.end(); ++it) {
		if (Frame >= (int)it->FirstFrame && Frame < (int)(it->FirstFrame + it->FrameCount))
			return &*it;
	}
	return nullptr;
}


// 画像を保存する
bool CMemoryCapture::SaveImageToFile(
	const CImage *pImage, LPCWSTR pszFileName, CImageCodec::FormatType Format)
{
	if (pImage == nullptr)
		return false;

	const CImage *pSaveImage;
	CImage *pTempImage = nullptr;
	int Width, Height;

	GetCaptureImageSize(pImage, &Width, &Height);
	if (Width == pImage->GetWidth() && Height == pImage->GetHeight()) {
		pSaveImage = pImage;
	} else {
		pTempImage = pImage->Resize(Width, Height, m_Resample);
		if (pTempImage == nullptr)
			return false;
		pSaveImage = pTempImage;
	}

	bool fResult = m_Codec.SaveImageToFile(pSaveImage, pszFileName, Format);

	delete pTempImage;

	return fResult;
}


// 画像をクリップボードにコピーする
bool CMemoryCapture::CopyImageToClipboard(const CImage *pImage) const
{
	if (pImage == nullptr)
		return false;

	const CImage *pCopyImage;
	CImage *pTempImage = nullptr;
	int Width, Height;

	GetCaptureImageSize(pImage, &Width, &Height);
	if (Width == pImage->GetWidth() && Height == pImage->GetHeight()) {
		pCopyImage = pImage;
	} else {
		pTempImage = pImage->Resize(Width, Height, m_Resample);
		if (pTempImage == nullptr)
			return false;
		pCopyImage = pTempImage;
	}

	const SIZE_T RowBytes = (Width * 3 + 3) / 4 * 4;
	HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, sizeof(BITMAPINFOHEADER) + (Height * RowBytes));
	if (hGlobal == nullptr) {
		delete pTempImage;
		return false;
	}

	BITMAPINFOHEADER *pbmih = static_cast<BITMAPINFOHEADER*>(::GlobalLock(hGlobal));

	pbmih->biSize = sizeof(BITMAPINFOHEADER);
	pbmih->biWidth = Width;
	pbmih->biHeight = Height;
	pbmih->biPlanes = 1;
	pbmih->biBitCount = 24;
	pbmih->biCompression = BI_RGB;
	pbmih->biSizeImage = 0;
	pbmih->biXPelsPerMeter = 0;
	pbmih->biYPelsPerMeter = 0;
	pbmih->biClrUsed = 0;
	pbmih->biClrImportant = 0;

	BYTE *pPixels = reinterpret_cast<BYTE*>(pbmih + 1);

	for (int y = 0; y < Height; y++) {
		pCopyImage->ExtractRow24(Height - 1 - y, pPixels);
		pPixels += RowBytes;
	}

	::GlobalUnlock(hGlobal);

	bool fOK = false;

	if (::OpenClipboard(m_hwnd)) {
		::EmptyClipboard();
		if (::SetClipboardData(CF_DIB, hGlobal) != nullptr)
			fOK = true;
		::CloseClipboard();
	}
	if (!fOK)
		::GlobalFree(hGlobal);

	delete pTempImage;

	return fOK;
}


// キャプチャ画像のサイズを取得する
void CMemoryCapture::GetCaptureImageSize(const CImage *pImage, int *pWidth, int *pHeight) const
{
	int Width, Height;

	if (m_CaptureSize.Type == CaptureSizeType::Rate) {
		Width = ::MulDiv(pImage->GetDisplayWidth(), m_CaptureSize.Rate.Num, m_CaptureSize.Rate.Denom);
		Height = ::MulDiv(pImage->GetDisplayHeight(), m_CaptureSize.Rate.Num, m_CaptureSize.Rate.Denom);
	} else {
		Width = m_CaptureSize.Size.Width;
		Height = m_CaptureSize.Size.Height;
	}

	if (pWidth != nullptr)
		*pWidth = Width;
	if (pHeight != nullptr)
		*pHeight = Height;
}


// キャプションを設定する
void CMemoryCapture::SetCaption()
{
	if (m_hwnd != nullptr) {
		CBlockLock Lock(m_ImageLock);
		const CImage *pImage = GetCurImage();

		if (pImage != nullptr) {
			TCHAR szText[256];
			const unsigned int FrameFlags = pImage->GetFrameFlags();

			::StringCchPrintf(
				szText, _countof(szText),
				TITLE_TEXT TEXT(" - %d/%d [%dx%d (%d:%d)] %s"),
				m_CurFrame + 1, (int)m_ImageList.size(),
				pImage->GetWidth(), pImage->GetHeight(),
				pImage->GetAspectRatioX(), pImage->GetAspectRatioY(),
				(FrameFlags & CVideoDecoder::FrameFlag_IFrame) ? TEXT("I") :
				(FrameFlags & CVideoDecoder::FrameFlag_PFrame) ? TEXT("P") :
				(FrameFlags & CVideoDecoder::FrameFlag_BFrame) ? TEXT("B") : TEXT(""));
			::SetWindowText(m_hwnd, szText);
		} else {
			::SetWindowText(m_hwnd, TITLE_TEXT);
		}
	}
}


// ウィンドウサイズを調整する
void CMemoryCapture::AdjustWindowSize()
{
	if (m_hwnd == nullptr || ::IsZoomed(m_hwnd))
		return;

	int Width, Height;

	if (!m_Preview.GetDisplaySize(&Width, &Height))
		return;

	RECT rc;
	rc.left = 0;
	rc.top = 0;
	rc.right = Width;
	rc.bottom = Height + m_SeekBar.GetHeight() + m_Toolbar.GetHeight();
	::AdjustWindowRectEx(&rc, GetWindowStyle(m_hwnd), FALSE, GetWindowExStyle(m_hwnd));

	Width = rc.right - rc.left;
	Height = rc.bottom - rc.top;

	// モニタからはみ出さないようにする
	HMONITOR hMonitor = ::MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	::GetMonitorInfo(hMonitor, &mi);
	::GetWindowRect(m_hwnd, &rc);
	rc.right = rc.left + Width;
	rc.bottom = rc.top + Height;
	if (rc.right > mi.rcWork.right) {
		rc.right = mi.rcWork.right;
		rc.left = std::max(rc.right - Width, mi.rcWork.left);
	}
	if (rc.bottom > mi.rcWork.bottom) {
		rc.bottom = mi.rcWork.bottom;
		rc.top = std::max(rc.bottom - Height, mi.rcWork.top);
	}

	::SetWindowPos(
		m_hwnd, nullptr,
		rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
		SWP_NOZORDER | SWP_NOACTIVATE);
}


// ウィンドウに通知メッセージを送る
void CMemoryCapture::PostNotifyMessage(UINT Message, WPARAM wParam, LPARAM lParam)
{
	CBlockLock Lock(m_WindowLock);

	if (m_hwnd != nullptr)
		::PostMessage(m_hwnd, Message, wParam, lParam);
}


// ツールバーの画像を設定する
void CMemoryCapture::SetToolbarImage()
{
	int ID, Width, Height;

	if (m_DPI <= 128) {
		ID = IDB_TOOLBAR_16;
		Width = Height = 16;
	} else {
		ID = IDB_TOOLBAR_32;
		Width = Height = 32;
	}

	m_Toolbar.SetIconImage(
		static_cast<HBITMAP>(::LoadImage(
			g_hinstDLL, MAKEINTRESOURCE(ID), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION)),
		Width, Height);
}


// フレーム送り開始
void CMemoryCapture::StartSeeking(int Command)
{
	StopSeeking();

	m_SeekCommand = Command;
	::SendMessage(m_hwnd, WM_COMMAND, Command, 0);

	int Delay;
	::SystemParametersInfo(SPI_GETKEYBOARDDELAY, 0, &Delay, 0);

	::SetTimer(m_hwnd, TIMER_SEEK_START, (Delay + 1) * 250, nullptr);
}


// フレーム送り終了
void CMemoryCapture::StopSeeking()
{
	m_SeekCommand = 0;
	::KillTimer(m_hwnd, TIMER_SEEK);
	::KillTimer(m_hwnd, TIMER_SEEK_START);
}


// 保存先フォルダを取得する
bool CMemoryCapture::GetSaveFolder(LPWSTR pszFolder)
{
	// 設定を取得する
	if (m_pApp->GetSetting(L"CaptureFolder", pszFolder, MAX_PATH) < 1) {
		::MessageBox(GetOwnerWindow(), TEXT("保存先フォルダを取得できません。"), nullptr, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	// 相対パスの場合絶対パスに変換する
	if (::PathIsRelativeW(pszFolder)) {
		WCHAR szBaseFolder[MAX_PATH];
		DWORD Length = ::GetModuleFileNameW(nullptr, szBaseFolder, _countof(szBaseFolder));
		if (Length < 1 || Length >= _countof(szBaseFolder)
				|| !::PathRemoveFileSpecW(szBaseFolder)
				|| !::PathAppendW(szBaseFolder, pszFolder)
				|| !::PathCanonicalizeW(pszFolder, szBaseFolder)) {
			::MessageBox(GetOwnerWindow(), TEXT("相対パスを変換できません。"), nullptr, MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
	}

	return true;
}


// 保存ファイル名を取得する
bool CMemoryCapture::GetSaveFileName(
	LPWSTR pszFileName, LPCWSTR pszFolder,
	const CImage *pImage, const FrameGroupInfo *pGroup, int *pSequentialNumber)
{
	// 設定を取得する
	WCHAR szFileName[1024];
	if (m_pApp->GetSetting(L"CaptureFileName", szFileName, _countof(szFileName)) < 1) {
		::MessageBox(GetOwnerWindow(), TEXT("保存ファイル名を取得できません。"), nullptr, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	// ファイル名中の変数を展開する

	struct VarMapParam
	{
		TVTest::CTVTestApp *pApp;
		const CImage *pImage;
	};

	struct CustomVarMap
	{
		static BOOL CALLBACK Func(LPCWSTR pszVar, LPWSTR *ppszString, void *pClientData)
		{
			VarMapParam *pParam = static_cast<VarMapParam*>(pClientData);

			if (::lstrcmpiW(pszVar, L"width") == 0 || ::lstrcmpiW(pszVar, L"height") == 0) {
				WCHAR szTemp[16];
				::StringCchPrintfW(
					szTemp, _countof(szTemp), L"%d",
					::lstrcmpiW(pszVar, L"width") == 0 ? pParam->pImage->GetWidth() : pParam->pImage->GetHeight());
				DWORD Size = (::lstrlenW(szTemp) + 1) * sizeof(WCHAR);
				*ppszString = static_cast<LPWSTR>(pParam->pApp->MemoryAlloc(Size));
				if (*ppszString != nullptr)
					std::memcpy(*ppszString, szTemp, Size);
				return TRUE;
			}

			return FALSE;
		}
	};

	VarMapParam MapParam;
	MapParam.pApp = m_pApp;
	MapParam.pImage = pImage;

	TVTest::VarStringFormatInfo Info;
	Info.Flags = TVTest::VAR_STRING_FORMAT_FLAG_FILENAME;
	Info.pszFormat = szFileName;
	Info.pContext = pGroup != nullptr ? pGroup->pVarContext : nullptr;
	Info.pMapFunc = CustomVarMap::Func;
	Info.pClientData = &MapParam;
	Info.pszResult = nullptr;

	if (!m_pApp->FormatVarString(&Info)) {
		::MessageBox(GetOwnerWindow(), TEXT("保存ファイル名を生成できません。"), nullptr, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	if (::lstrlenW(pszFolder) + 1 + ::lstrlenW(Info.pszResult) + 4 >= MAX_PATH) {
		::MessageBox(GetOwnerWindow(), TEXT("パスが長すぎます。"), nullptr, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	WCHAR szPath[MAX_PATH];
	::PathCombineW(szPath, pszFolder, Info.pszResult);
	::StringCchCatW(szPath, _countof(szPath), m_Codec.GetFormatExtensions(m_SaveFormat));

	// フォルダが存在しなければ作成する
	// ファイル名にフォルダ階層が含まれている場合もある("%event-name%\\%date%-%time%" など)
	WCHAR szFolder[MAX_PATH];
	::StringCchCopyW(szFolder, _countof(szFolder), szPath);
	::PathRemoveFileSpecW(szFolder);
	if (!::PathIsDirectoryW(szFolder)) {
		int Result = ::SHCreateDirectory(nullptr, szFolder);
		if (Result != ERROR_SUCCESS && Result != ERROR_ALREADY_EXISTS) {
			::MessageBox(GetOwnerWindow(), TEXT("フォルダを作成できません。"), nullptr, MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
	}

	// ファイルが既に存在する場合は連番を付加する
	if (pSequentialNumber != nullptr || ::PathFileExistsW(szPath)) {
		bool fOK = false;
		for (int i = ((pSequentialNumber != nullptr) ? (*pSequentialNumber + 1) : 1); i < 1000; i++) {
			WCHAR szNum[16];
			::StringCchPrintfW(szNum, _countof(szNum), L"-%d", i);
			::PathCombineW(szPath, pszFolder, Info.pszResult);
			if (FAILED(::StringCchCatW(szPath, _countof(szPath), szNum))
					|| FAILED(::StringCchCatW(szPath, _countof(szPath), m_Codec.GetFormatExtensions(m_SaveFormat)))) {
				::MessageBox(GetOwnerWindow(), TEXT("パスが長すぎます。"), nullptr, MB_OK | MB_ICONEXCLAMATION);
				return false;
			}
			if (!::PathFileExistsW(szPath)) {
				if (pSequentialNumber != nullptr)
					*pSequentialNumber = i;
				fOK = true;
				break;
			}
		}
		if (!fOK) {
			::MessageBox(GetOwnerWindow(), TEXT("ユニークなファイル名を生成できません。"), nullptr, MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
	}

	::StringCchCopyW(pszFileName, MAX_PATH, szPath);

	return true;
}


// 現在の画像を保存する
bool CMemoryCapture::SaveCurrent()
{
	const CImage *pImage = GetCurImage();

	if (pImage == nullptr)
		return false;

	WCHAR szFolder[MAX_PATH];
	if (!GetSaveFolder(szFolder))
		return false;

	WCHAR szFileName[MAX_PATH];
	if (!GetSaveFileName(szFileName, szFolder, pImage, GetFrameGroupInfo(m_CurFrame)))
		return false;

	if (!SaveImageToFile(pImage, szFileName, m_SaveFormat)) {
		::MessageBox(GetOwnerWindow(), TEXT("保存ができません。"), nullptr, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	return true;
}


// 名前を付けて保存
bool CMemoryCapture::SaveAs()
{
	if (GetCurImage() == nullptr)
		return false;

	OPENFILENAME ofn = {};
	TCHAR szFileName[MAX_PATH];

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = m_hwnd;
	ofn.lpstrFilter =
		TEXT("BMP ファイル (*.bmp)\0*.bmp\0")
		TEXT("JPEG ファイル (*.jpg;*.jpeg;*.jpe)\0*.jpg;*.jpeg;*.jpe\0")
		TEXT("PNG ファイル (*.png)\0*.png\0");
	ofn.nFilterIndex = static_cast<int>(m_LastSaveFormat) + 1;
	if (!m_LastSaveFileName.empty())
		::lstrcpyn(szFileName, m_LastSaveFileName.c_str(), _countof(szFileName));
	else
		szFileName[0] = TEXT('\0');
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = _countof(szFileName);
	if (!m_LastSaveFolder.empty())
		ofn.lpstrInitialDir = m_LastSaveFolder.c_str();
	ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR;

	if (!::GetSaveFileName(&ofn))
		return false;

	CImageCodec::FormatType Format = (CImageCodec::FormatType)(ofn.nFilterIndex - 1);

	m_LastSaveFormat = Format;
	m_LastSaveFileName = ::PathFindFileName(szFileName);

	// 拡張子を付加する
	LPCTSTR pExtensions = m_Codec.GetFormatExtensions(Format);
	if (pExtensions != nullptr) {
		LPCTSTR pszExtension = ::PathFindExtension(szFileName);
		bool fAddExtension = true;

		if (pszExtension[0] == TEXT('.')) {
			for (LPCTSTR p = pExtensions; *p != TEXT('\0'); p += ::lstrlen(p) + 1) {
				if (::lstrcmpi(p, pszExtension) == 0) {
					fAddExtension = false;
					break;
				}
			}
		}
		if (fAddExtension) {
			if (FAILED(::StringCchCat(szFileName, _countof(szFileName), pExtensions)))
				return false;
		}
	}

	bool fResult = SaveImageToFile(GetCurImage(), szFileName, Format);
	if (!fResult)
		::MessageBox(GetOwnerWindow(), TEXT("保存ができません。"), nullptr, MB_OK | MB_ICONEXCLAMATION);

	::PathRemoveFileSpec(szFileName);
	m_LastSaveFolder = szFileName;

	return fResult;
}


// すべて保存
bool CMemoryCapture::SaveAll()
{
	WCHAR szFolder[MAX_PATH];
	if (!GetSaveFolder(szFolder))
		return false;

	m_ImageLock.Lock();
	const int FrameCount = (int)m_ImageList.size();
	m_ImageLock.Unlock();

	const FrameGroupInfo *pPrevGroup = nullptr;
	int SequentialNumber = 0;

	for (int i = 0; i < FrameCount; i++) {
		const CImage *pImage = GetFrameImage(i);
		const FrameGroupInfo *pCurGroup = GetFrameGroupInfo(i);

		if (pPrevGroup != pCurGroup)
			SequentialNumber = 0;

		WCHAR szFileName[MAX_PATH];
		if (!GetSaveFileName(szFileName, szFolder, pImage, pCurGroup, &SequentialNumber))
			return false;

		if (!SaveImageToFile(pImage, szFileName, m_SaveFormat)) {
			::MessageBox(GetOwnerWindow(), TEXT("保存ができません。"), nullptr, MB_OK | MB_ICONEXCLAMATION);
			return false;
		}

		pPrevGroup = pCurGroup;
	}

	return true;
}


// ウィンドウ作成時の処理
void CMemoryCapture::OnWindowCreate()
{
	m_ImageLock.Lock();
	const int FrameCount = (int)m_ImageList.size();
	if (!m_ImageList.empty()) {
		if (m_CurFrame < 0 || m_CurFrame >= (int)m_ImageList.size())
			m_CurFrame = 0;
	} else {
		m_CurFrame = -1;
	}
	m_ImageLock.Unlock();

	m_WheelDelta = 0;
	m_WheelTime = 0;

	m_Preview.SetImage(GetCurImage());
	m_Preview.Create(m_hwnd);

	m_SeekBar.SetDPI(m_DPI);
	m_SeekBar.SetBarRange(0, FrameCount - 1);
	m_SeekBar.SetBarPos(m_CurFrame);
	m_SeekBar.Create(m_hwnd, IDC_SEEK_BAR, m_pApp);

	SetToolbarImage();
	m_Toolbar.SetDPI(m_DPI);
	m_Toolbar.Create(m_hwnd, IDC_TOOLBAR, m_pApp);

	SetCaption();

	if (!m_fAccumulateAlways) {
		AllocateStreamBuffer();
		m_pApp->SetVideoStreamCallback(VideoStreamCallback, this);
	}

	// メインウィンドウがダークモードであればそれに合わせてダークモードにする
	if (m_pApp->GetDarkModeStatus() & TVTest::DARK_MODE_STATUS_MAINWINDOW_DARK)
		m_pApp->SetWindowDarkMode(m_hwnd, true);
}


// ウィンドウ破棄時の処理
void CMemoryCapture::OnWindowDestroy()
{
	CloseDecodeThread();
	FreeImages();

	if (!m_fAccumulateAlways)
		FreeStreamBuffer();

	// ウィンドウ位置保存
	WINDOWPLACEMENT wp;
	wp.length = sizeof(WINDOWPLACEMENT);
	if (::GetWindowPlacement(m_hwnd, &wp)) {
		m_WindowPosition.Left = wp.rcNormalPosition.left;
		m_WindowPosition.Top = wp.rcNormalPosition.top;
		m_WindowPosition.Width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
		m_WindowPosition.Height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
	}
}


// コマンドの処理
void CMemoryCapture::OnCommand(int Command, int NotifyCode)
{
	switch (Command) {
	case CM_CAPTURE:
		// 取り込み
		StartCapture(false);
		return;

	case CM_CAPTURE_ADD:
		// 追加取り込み
		StartCapture(true);
		return;

	case CM_SAVE:
		// 保存
		SaveCurrent();
		return;

	case CM_SAVE_AS:
		// 名前を付けて保存
		SaveAs();
		return;

	case CM_SAVE_ALL:
		// すべて保存
		{
			HCURSOR hcurOld = ::SetCursor(::LoadCursor(nullptr, IDC_WAIT));
			SaveAll();
			::SetCursor(hcurOld);
		}
		return;

	case CM_COPY:
		// クリップボードにコピー
		CopyImageToClipboard(GetCurImage());
		return;

	case CM_PREV_FRAME:
		// 前のフレーム
		SetCurFrame(m_CurFrame - 1);
		return;

	case CM_NEXT_FRAME:
		// 次のフレーム
		SetCurFrame(m_CurFrame + 1);
		return;

	case CM_FIRST_FRAME:
		// 最初のフレーム
		SetCurFrame(0);
		return;

	case CM_LAST_FRAME:
		// 最後のフレーム
		{
			CBlockLock Lock(m_ImageLock);
			SetCurFrame((int)m_ImageList.size() - 1);
		}
		return;

	case CM_SKIP_BACKWARD_FRAME:
		// フレームを後方にスキップ
		SetCurFrame(std::max(m_CurFrame - m_SkipFrames, 0));
		return;

	case CM_SKIP_FORWARD_FRAME:
		// フレームを前方にスキップ
		{
			CBlockLock Lock(m_ImageLock);
			SetCurFrame(std::min(m_CurFrame + m_SkipFrames, (int)m_ImageList.size() - 1));
		}
		return;

	case CM_FIT_IMAGE_TO_WINDOW:
		// 表示倍率をウィンドウに合わせる
		m_Preview.SetFitImageToWindow(!m_Preview.GetFitImageToWindow());
		return;

	case CM_FIT_WINDOW_TO_IMAGE:
		// ウィンドウサイズを画像に合わせる
		m_fFitWindowToImage = !m_fFitWindowToImage;
		if (m_fFitWindowToImage && !m_Preview.GetFitImageToWindow())
			AdjustWindowSize();
		return;

	case CM_SETTINGS:
		SettingsDialog(m_hwnd);
		return;

	case IDC_SEEK_BAR:
		if (NotifyCode == CSeekBar::Notify_PosChanged)
			SetCurFrame(m_SeekBar.GetBarPos());
		return;

	case IDC_TOOLBAR:
		if (NotifyCode == CToolbar::Notify_ItemPressed)
			StartSeeking(m_Toolbar.GetPressingItem());
		else if (NotifyCode == CToolbar::Notify_ItemReleased)
			StopSeeking();
		return;

	default:
		// キャプチャーサイズ
		if (Command >= CM_CAPTURE_SIZE_FIRST && Command <= CM_CAPTURE_SIZE_LAST) {
			std::size_t Index = Command - CM_CAPTURE_SIZE_FIRST;

			if (Index < _countof(m_CaptureSizeList))
				m_CaptureSize = m_CaptureSizeList[Index];
			return;
		}

		// 再サンプリング
		if (Command >= CM_RESAMPLE_FIRST && Command <= CM_RESAMPLE_LAST) {
			m_Resample = (CImage::ResampleType)(Command - CM_RESAMPLE_FIRST);
			return;
		}

		// インターレース解除
		if (Command >= CM_DEINTERLACE_FIRST && Command <= CM_DEINTERLACE_LAST) {
			m_Deinterlace = (CVideoDecoder::DeinterlaceMethod)(Command - CM_DEINTERLACE_FIRST);
			return;
		}

		// 表示倍率
		if (Command >= CM_ZOOM_FIRST && Command <= CM_ZOOM_LAST) {
			int Index = Command - CM_ZOOM_FIRST;

			if (Index < _countof(m_ZoomRateList)) {
				m_Preview.SetZoomRate(m_ZoomRateList[Index].Rate.Num, m_ZoomRateList[Index].Rate.Denom);
				if (m_fFitWindowToImage)
					AdjustWindowSize();
			}
			return;
		}
	}
}


// プラグインのコマンドを実行する
bool CMemoryCapture::OnPluginCommand(int Command)
{
	switch (Command) {
	case COMMAND_CAPTURE:
	case COMMAND_CAPTURE_ADD:
		if (m_hwnd != nullptr) {
			::SendMessage(m_hwnd, WM_COMMAND, Command == COMMAND_CAPTURE ? CM_CAPTURE : CM_CAPTURE_ADD, 0);
		} else if (m_fAccumulateAlways) {
			m_StreamLock.Lock();
			const bool fAvail = m_StreamAvail > 0;
			m_StreamLock.Unlock();
			if (fAvail) {
				if (StartCapture(Command == COMMAND_CAPTURE_ADD))
					m_pApp->EnablePlugin(true);
			}
		}
		return true;

	case COMMAND_SAVE:
		if (m_hwnd != nullptr)
			::SendMessage(m_hwnd, WM_COMMAND, CM_SAVE, 0);
		return true;

	case COMMAND_COPY:
		if (m_hwnd != nullptr)
			::SendMessage(m_hwnd, WM_COMMAND, CM_COPY, 0);
		return true;
	}

	return false;
}


// コンテキストメニューを表示する
void CMemoryCapture::ShowContextMenu(int x, int y)
{
	HMENU hmenu = ::LoadMenu(g_hinstDLL, MAKEINTRESOURCE(IDM_CONTEXT_MENU));
	HMENU hmenuPopup = ::GetSubMenu(hmenu, 0);
	bool fImage = GetCurImage() != nullptr;

	::EnableMenuItem(hmenuPopup, CM_SAVE, MF_BYCOMMAND | (fImage ? MF_ENABLED : MF_GRAYED));
	::EnableMenuItem(hmenuPopup, CM_SAVE_AS, MF_BYCOMMAND | (fImage ? MF_ENABLED : MF_GRAYED));
	::EnableMenuItem(hmenuPopup, CM_SAVE_ALL, MF_BYCOMMAND | (fImage ? MF_ENABLED : MF_GRAYED));
	::EnableMenuItem(hmenuPopup, CM_COPY, MF_BYCOMMAND | (fImage ? MF_ENABLED : MF_GRAYED));
	::CheckMenuItem(hmenuPopup, CM_FIT_WINDOW_TO_IMAGE, MF_BYCOMMAND | (m_fFitWindowToImage ? MF_CHECKED : MF_UNCHECKED));

	// 表示倍率
	int Zoom = 0;
	if (m_Preview.GetFitImageToWindow()) {
		Zoom = CM_FIT_IMAGE_TO_WINDOW;
	} else {
		int Num, Denom;
		m_Preview.GetZoomRate(&Num, &Denom);
		for (int i = 0; i < _countof(m_ZoomRateList); i++) {
			if (m_ZoomRateList[i].Rate.Num == Num && m_ZoomRateList[i].Rate.Denom == Denom) {
				Zoom = CM_ZOOM_FIRST + i;
				break;
			}
		}
	}
	if (Zoom != 0)
		::CheckMenuRadioItem(hmenuPopup, CM_FIT_IMAGE_TO_WINDOW, CM_ZOOM_LAST, Zoom, MF_BYCOMMAND);

	// キャプチャーサイズ
	HMENU hmenuSize = ::GetSubMenu(hmenuPopup, 9);
	::DeleteMenu(hmenuSize, 0, MF_BYPOSITION); // ダミーを削除
	int CurSize = -1;
	for (std::size_t i = 0; i < _countof(m_CaptureSizeList); i++) {
		TCHAR szText[64];
		if (m_CaptureSizeList[i].Type == CaptureSizeType::Size) {
			::StringCchPrintf(
				szText, _countof(szText), TEXT("%d x %d"),
				m_CaptureSizeList[i].Size.Width, m_CaptureSizeList[i].Size.Height);
		} else {
			::StringCchPrintf(
				szText, _countof(szText), TEXT("%d%%"),
				m_CaptureSizeList[i].Rate.Num * 100 / m_CaptureSizeList[i].Rate.Denom);
		}
		::AppendMenu(hmenuSize, MF_STRING | MF_ENABLED, CM_CAPTURE_SIZE_FIRST + i, szText);
		if (m_CaptureSizeList[i] == m_CaptureSize)
			CurSize = (int)i;
	}
	if (CurSize >= 0)
		::CheckMenuRadioItem(hmenuSize, 0, _countof(m_CaptureSizeList) - 1, CurSize, MF_BYPOSITION);

	::CheckMenuRadioItem(
		hmenuPopup, CM_RESAMPLE_FIRST, CM_RESAMPLE_LAST,
		CM_RESAMPLE_FIRST + (int)m_Resample, MF_BYCOMMAND);
	::CheckMenuRadioItem(
		hmenuPopup, CM_DEINTERLACE_FIRST, CM_DEINTERLACE_LAST,
		CM_DEINTERLACE_FIRST + (int)m_Deinterlace, MF_BYCOMMAND);

	::TrackPopupMenu(hmenuPopup, TPM_RIGHTBUTTON, x, y, 0, m_hwnd, nullptr);
	::DestroyMenu(hmenu);
}


// 設定ダイアログを表示する
bool CMemoryCapture::SettingsDialog(HWND hwndOwner)
{
	TVTest::ShowDialogInfo Info;

	Info.Flags = 0;
	Info.hinst = g_hinstDLL;
	Info.pszTemplate = MAKEINTRESOURCE(IDD_SETTINGS);
	Info.pMessageFunc = SettingsDlgProc;
	Info.pClientData = this;
	Info.hwndOwner = hwndOwner;

	return m_pApp->ShowDialog(&Info) == IDOK;
}


// フレームがデコードされた
bool CMemoryCapture::OnFrame(const CVideoDecoder::FrameInfo &Frame)
{
	CImage *pImage = new CImage;

	if (!pImage->Create(
			Frame.Width, Frame.Height, Frame.BitsPerPixel,
			Frame.AspectRatioX, Frame.AspectRatioY)) {
		delete pImage;
		return false;
	}

	pImage->SetFrameFlags(Frame.Flags);

	std::size_t RowBytes = (Frame.Width * Frame.BitsPerPixel + 7) / 8;

	for (int y = 0; y < Frame.Height; y++) {
		std::memcpy(pImage->GetRowPixels(y), Frame.Buffer + y * Frame.Pitch, RowBytes);
	}

	std::size_t FrameIndex;
	{
		CBlockLock Lock(m_ImageLock);

		FrameIndex = m_ImageList.size();
		m_ImageList.push_back(pImage);
		m_FrameGroupList.back().FrameCount++;
	}

	PostNotifyMessage(WM_APP_FRAME_DECODED, FrameIndex);

	return true;
}


// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CMemoryCapture::EventCallback(
	UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
{
	CMemoryCapture *pThis = static_cast<CMemoryCapture *>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// プラグインの有効状態が変化した
		return pThis->EnablePlugin(lParam1 != 0);

	case TVTest::EVENT_PLUGINSETTINGS:
		// プラグインの設定を行う
		return pThis->SettingsDialog(reinterpret_cast<HWND>(lParam1));

	case TVTest::EVENT_STANDBY:
		// 待機状態が変化した
		if (pThis->m_pApp->IsPluginEnabled()) {
			// 待機状態の時はウィンドウを隠す
			::ShowWindow(pThis->m_hwnd, lParam1 != 0 ? SW_HIDE : SW_SHOW);
		}
		return TRUE;

	case TVTest::EVENT_COMMAND:
		// コマンドが選択された
		return pThis->OnPluginCommand((int)lParam1);

	case TVTest::EVENT_COLORCHANGE:
		// 配色が変わった
		if (pThis->m_hwnd != nullptr)
			::RedrawWindow(pThis->m_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
		return TRUE;

	case TVTest::EVENT_SETTINGSCHANGE:
		// 設定が変更された
		pThis->LoadAppSettings();
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


// 映像ストリームのコールバック関数
LRESULT CALLBACK CMemoryCapture::VideoStreamCallback(
	DWORD Format, const void *pData, SIZE_T Size, void *pClientData)
{
	static_cast<CMemoryCapture *>(pClientData)->InputStream(Format, pData, Size);

	return 0;
}


// 映像デコードスレッド
unsigned int __stdcall CMemoryCapture::DecodeThread(void *pParameter)
{
	CMemoryCapture *pThis = static_cast<CMemoryCapture *>(pParameter);

	::CoInitialize(nullptr);

	pThis->m_Decoder.SetDeinterlaceMethod(pThis->m_Deinterlace);

	if (pThis->m_Decoder.Open(pThis->m_StreamFormat)) {
		pThis->PostNotifyMessage(WM_APP_DECODE_START);

		pThis->m_Decoder.InputStream(pThis->m_pDecodeBuffer, pThis->m_DecodeSize);
		pThis->m_Decoder.Close();

		delete [] pThis->m_pDecodeBuffer;
		pThis->m_pDecodeBuffer = nullptr;

		int FrameOffset, FrameCount;
		{
			CBlockLock Lock(pThis->m_ImageLock);
			const FrameGroupInfo &Group = pThis->m_FrameGroupList.back();

			FrameOffset = (int)Group.FirstFrame;
			FrameCount = (int)Group.FrameCount;
		}

		pThis->PostNotifyMessage(WM_APP_DECODE_END, FrameOffset, FrameCount);
	}

	::CoUninitialize();

	return 0;
}


// ウィンドウハンドルからthisを取得する
CMemoryCapture * CMemoryCapture::GetThis(HWND hwnd)
{
	return reinterpret_cast<CMemoryCapture *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
}


// ウィンドウプロシージャ
LRESULT CALLBACK CMemoryCapture::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
			CMemoryCapture *pThis = static_cast<CMemoryCapture *>(pcs->lpCreateParams);

			pThis->m_WindowLock.Lock();
			pThis->m_hwnd = hwnd;
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
			pThis->m_WindowLock.Unlock();

			pThis->OnWindowCreate();
		}
		return 0;

	case WM_SIZE:
		{
			CMemoryCapture *pThis = GetThis(hwnd);
			int Width = LOWORD(lParam), Height = HIWORD(lParam);
			int SeekBarHeight = pThis->m_SeekBar.GetHeight();
			int ToolbarHeight = pThis->m_Toolbar.GetHeight();
			int y = Height;

			y -= ToolbarHeight;
			pThis->m_Toolbar.SetPosition(0, y, Width, ToolbarHeight);
			y -= SeekBarHeight;
			pThis->m_SeekBar.SetPosition(0, y, Width, SeekBarHeight);
			pThis->m_Preview.SetPosition(0, 0, Width, y);
		}
		return 0;

	case WM_COMMAND:
		{
			CMemoryCapture *pThis = GetThis(hwnd);

			pThis->OnCommand(LOWORD(wParam), HIWORD(wParam));
		}
		return 0;

	case WM_SYSCOMMAND:
		if ((wParam & 0xFFF0) == SC_CLOSE) {
			// 閉じる時はプラグインを無効にする
			CMemoryCapture *pThis = GetThis(hwnd);

			pThis->m_pApp->EnablePlugin(false);
			return TRUE;
		}
		break;

	case WM_KEYDOWN:
		// キー操作
		{
			CMemoryCapture *pThis = GetThis(hwnd);

			switch (wParam) {
			case VK_LEFT:
				if (pThis->m_SeekCommand != CM_PREV_FRAME)
					pThis->StartSeeking(CM_PREV_FRAME);
				break;
			case VK_RIGHT:
				if (pThis->m_SeekCommand != CM_NEXT_FRAME)
					pThis->StartSeeking(CM_NEXT_FRAME);
				break;
			case VK_BACK:
				::SendMessage(hwnd, WM_COMMAND, CM_PREV_FRAME, 0);
				break;
			case VK_SPACE:
				::SendMessage(hwnd, WM_COMMAND, CM_NEXT_FRAME, 0);
				break;
			case VK_HOME:
				::SendMessage(hwnd, WM_COMMAND, CM_FIRST_FRAME, 0);
				break;
			case VK_END:
				::SendMessage(hwnd, WM_COMMAND, CM_LAST_FRAME, 0);
				break;
			case VK_PRIOR:
				if (pThis->m_SeekCommand != CM_SKIP_BACKWARD_FRAME)
					pThis->StartSeeking(CM_SKIP_BACKWARD_FRAME);
				break;
			case VK_NEXT:
				if (pThis->m_SeekCommand != CM_SKIP_FORWARD_FRAME)
					pThis->StartSeeking(CM_SKIP_FORWARD_FRAME);
				break;
			}
		}
		return 0;

	case WM_KEYUP:
		{
			CMemoryCapture *pThis = GetThis(hwnd);

			switch (wParam) {
			case VK_LEFT:
			case VK_RIGHT:
			case VK_PRIOR:
			case VK_NEXT:
				pThis->StopSeeking();
				break;
			}
		}
		return 0;

	case WM_MOUSEWHEEL:
		// マウスホイール
		{
			CMemoryCapture *pThis = GetThis(hwnd);
			const int Delta = GET_WHEEL_DELTA_WPARAM(wParam);
			const DWORD CurTime = ::GetTickCount();

			if (CurTime - pThis->m_WheelTime >= 500)
				pThis->m_WheelDelta = Delta;
			else
				pThis->m_WheelDelta += Delta;

			if (std::abs(pThis->m_WheelDelta) >= WHEEL_DELTA) {
				CBlockLock Lock(pThis->m_ImageLock);
				int Delta = pThis->m_WheelDelta / WHEEL_DELTA;
				int Frame = pThis->m_CurFrame - Delta;
				if (Frame < 0)
					Frame = 0;
				else if (Frame > (int)pThis->m_ImageList.size() - 1)
					Frame = (int)pThis->m_ImageList.size() - 1;
				pThis->SetCurFrame(Frame);
				pThis->m_WheelDelta -= Delta * WHEEL_DELTA;
			}

			pThis->m_WheelTime = CurTime;
		}
		return 0;

	case WM_CONTEXTMENU:
		// コンテキストメニュー
		{
			CMemoryCapture *pThis = GetThis(hwnd);
			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

			if (pt.x == -1 && pt.y == -1) {
				// キーボード操作
				pt.x = 0;
				pt.y = 0;
				::ClientToScreen(hwnd, &pt);
			} else {
				RECT rc;

				::GetClientRect(hwnd, &rc);
				MapWindowRect(hwnd, nullptr, &rc);
				if (!::PtInRect(&rc, pt))
					break;
			}

			pThis->ShowContextMenu(pt.x, pt.y);
		}
		return 0;

	case WM_TIMER:
		{
			CMemoryCapture *pThis = GetThis(hwnd);

			if (wParam == TIMER_SEEK_START) {
				::KillTimer(hwnd, TIMER_SEEK_START);

				UINT Interval;
				if (pThis->m_SeekCommand == CM_PREV_FRAME || pThis->m_SeekCommand == CM_NEXT_FRAME) {
					Interval = 1000 / 30;
				} else {
					::SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &Interval, 0);
					Interval = 400 - (Interval * (400 - 33) / 31);
				}
				::SetTimer(hwnd, TIMER_SEEK, Interval, nullptr);
			} else if (wParam == TIMER_SEEK) {
				::SendMessage(hwnd, WM_COMMAND, pThis->m_SeekCommand, 0);
			}
		}
		return 0;

	case WM_GETDLGCODE:
		// カーソルキーを利用する
		return DLGC_WANTARROWS;

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif
	case WM_DPICHANGED:
		// DPI が変わった
		{
			CMemoryCapture *pThis = GetThis(hwnd);
			const RECT *prc = reinterpret_cast<const RECT*>(lParam);

			pThis->m_DPI = HIWORD(wParam);

			pThis->m_SeekBar.SetDPI(pThis->m_DPI);
			pThis->SetToolbarImage();
			pThis->m_Toolbar.SetDPI(pThis->m_DPI);

			::SetWindowPos(
				hwnd, nullptr,
				prc->left, prc->top,
				prc->right - prc->left, prc->bottom - prc->top,
				SWP_NOZORDER | SWP_NOACTIVATE);

			RECT rc;
			::GetClientRect(hwnd, &rc);
			::SendMessage(hwnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
		}
		break;

	case WM_APP_DECODE_START:
		// デコード開始の通知
		::SetWindowText(hwnd, TITLE_TEXT TEXT(" - デコード中..."));
		return 0;

	case WM_APP_DECODE_END:
		// デコード終了の通知
		{
			CMemoryCapture *pThis = GetThis(hwnd);
			const int FrameOffset = (int)wParam, FrameCount = (int)lParam;

			pThis->m_SeekBar.SetBarRange(0, (FrameOffset + FrameCount) - 1);
			if (FrameCount > 0) {
				if (pThis->m_CurFrame < 0 || pThis->m_CurFrame >= FrameOffset + FrameCount)
					pThis->m_CurFrame = FrameOffset;
				pThis->m_SeekBar.SetBarPos(pThis->m_CurFrame);
				pThis->m_Preview.SetImage(pThis->GetCurImage());
			}

			pThis->SetCaption();
		}
		return 0;

	case WM_APP_FRAME_DECODED:
		// フレームがデコードされた
		{
			CMemoryCapture *pThis = GetThis(hwnd);
			const int Frame = (int)wParam;

			if (pThis->m_CurFrame < 0 || pThis->m_CurFrame >= Frame)
				pThis->m_CurFrame = Frame;
			pThis->m_SeekBar.SetBarRange(0, Frame);
			pThis->m_SeekBar.SetBarPos(pThis->m_CurFrame);
			pThis->m_Preview.SetImage(pThis->GetCurImage());
		}
		return 0;

	case WM_DESTROY:
		{
			CMemoryCapture *pThis = GetThis(hwnd);

			if (pThis != nullptr) {
				pThis->OnWindowDestroy();

				pThis->m_WindowLock.Lock();
				pThis->m_hwnd = nullptr;
				::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(nullptr));
				pThis->m_WindowLock.Unlock();
			}
		}
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


// 設定ダイアログプロシージャ
INT_PTR CALLBACK CMemoryCapture::SettingsDlgProc(
	HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, void *pClientData)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CMemoryCapture *pThis = static_cast<CMemoryCapture*>(pClientData);

			::SetDlgItemInt(hDlg, IDC_SETTINGS_MEMORY_SIZE, pThis->m_VideoMemorySizeInMB, FALSE);
			::CheckDlgButton(hDlg, IDC_SETTINGS_ACCUMULATE_ALWAYS, pThis->m_fAccumulateAlways ? BST_CHECKED : BST_UNCHECKED);
			::SetDlgItemInt(hDlg, IDC_SETTINGS_SKIP_FRAMES, pThis->m_SkipFrames, TRUE);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SETTINGS_MEMORY_SIZE:
			if (HIWORD(wParam) == EN_CHANGE) {
				unsigned int Size = ::GetDlgItemInt(hDlg, IDC_SETTINGS_MEMORY_SIZE, nullptr, FALSE);
				TCHAR szText[16];

				// 1MBあたり15フレームとしてメモリ使用量を計算
				::StringCchPrintf(
					szText, _countof(szText), TEXT("%u MB"),
					(Size * (15 * ((1440 * 1080) * 3))) / (1024 * 1024));
				::SetDlgItemText(hDlg, IDC_SETTINGS_ESTIMATE_MEMORY_USAGE, szText);
			}
			return TRUE;

		case IDOK:
			{
				CMemoryCapture *pThis = static_cast<CMemoryCapture *>(pClientData);

				unsigned int VideoMemorySizeInMB =
					::GetDlgItemInt(hDlg, IDC_SETTINGS_MEMORY_SIZE, nullptr, FALSE);
				if (VideoMemorySizeInMB < 1)
					VideoMemorySizeInMB = 1;
				else if (VideoMemorySizeInMB > VideoMemorySizeLimitInMB)
					VideoMemorySizeInMB = VideoMemorySizeLimitInMB;

				bool fAccumulateAlways =
					::IsDlgButtonChecked(hDlg, IDC_SETTINGS_ACCUMULATE_ALWAYS) == BST_CHECKED;

				if (pThis->m_VideoMemorySizeInMB != VideoMemorySizeInMB) {
					pThis->m_VideoMemorySizeInMB = VideoMemorySizeInMB;
					if (fAccumulateAlways || pThis->m_hwnd != nullptr)
						pThis->AllocateStreamBuffer();
				}

				if (pThis->m_fAccumulateAlways != fAccumulateAlways) {
					pThis->m_fAccumulateAlways = fAccumulateAlways;
					if (fAccumulateAlways && pThis->m_hwnd == nullptr)
						pThis->m_pApp->SetVideoStreamCallback(VideoStreamCallback, pThis);
				}

				pThis->m_SkipFrames =
					::GetDlgItemInt(hDlg, IDC_SETTINGS_SKIP_FRAMES, nullptr, TRUE);
			}
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;
	}

	return FALSE;
}




// プラグインクラスのインスタンスを生成する
TVTest::CTVTestPlugin * CreatePluginClass()
{
	return new CMemoryCapture;
}
