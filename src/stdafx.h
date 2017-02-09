// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、または
// 参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。

#pragma once

#ifndef WINVER
#define WINVER 0x0601		// Windows 7
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601	// Windows 7
#endif

#ifndef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN7
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x0600	// Internet Explorer 6.0
#endif

#define WIN32_LEAN_AND_MEAN

#define _WIN32_DCOM	// for CoInitializeEx()

#if defined(_MSC_VER) && (_MSC_VER <= 1800)
#ifndef _ALLOW_KEYWORD_MACROS
#define _ALLOW_KEYWORD_MACROS
#endif
#endif

#ifdef WIN_XP_SUPPORT
#define PSAPI_VERSION 1
#endif

// Windows のヘッダで出る警告を抑止
#pragma warning(push)
#pragma warning(disable: 4091)

#include <stdio.h>
#include <process.h>
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <streams.h>	// DirectShow BaseClasses

#pragma warning(pop)

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif
#include <crtdbg.h>
#ifdef _DEBUG
#undef strdup
#define strdup strdup
#define DEBUG_NEW new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif // _DEBUG

#include "Common/Common.h"


// 警告の無効設定
#pragma warning(disable: 4355) // warning C4355: "'this' : ベース メンバ初期化リストで使用されました。"
#pragma warning(disable: 4995) // warning C4995: 名前が避けられた #pragma として記述されています。
#pragma warning(disable: 4996) // warning C4996: "This function or variable may be unsafe."


// ライブラリのリンク
#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "AdvApi32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "ComCtl32.lib")
#pragma comment(lib, "ComDlg32.lib")
#pragma comment(lib, "Strmiids.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "ShLwApi.lib")

#ifdef _DEBUG
#pragma comment(lib, "StrmBasd.lib")
#else
#pragma comment(lib, "StrmBase.lib")
#endif

// トレース出力
#ifdef _DEBUG
	#undef TRACE
	#define TRACE ::DebugTrace
	void DebugTrace(LPCTSTR szFormat, ...);
#else
	#define TRACE __noop
#endif

#ifndef _DEBUG
	#define _SECURE_SCL 0
#endif

#ifndef NO_NULLPTR
#undef NULL
#define NULL nullptr
#endif

// アラインメント指定
#if defined(_MSC_VER) && (_MSC_VER <= 1800)
#define alignas(n) __declspec(align(n))
#define alignof(t) __alignof(t)
#endif


// BonTsEngine の設定
#define BONTSENGINE_SSE2				// SSE2 を利用
#define BONTSENGINE_MPEG2_SUPPORT		// MPEG-2 対応
#define BONTSENGINE_H264_SUPPORT		// H.264 対応
#define BONTSENGINE_H265_SUPPORT		// H.265 対応
#define BONTSENGINE_MPEG_AUDIO_SUPPORT	// MPEG-1/2 Audio 対応
#define BONTSENGINE_AC3_SUPPORT			// AC-3 対応
