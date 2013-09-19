#ifndef TVTEST_H
#define TVTEST_H


#ifndef TVH264
#define APP_NAME_A	"TVTest"
#else
#define APP_NAME_A	"TVH264"
#endif

#define VERSION_MAJOR		0
#define VERSION_MINOR		8
#define VERSION_BUILD		1
#define VERSION_REVISION	0

#define VERSION_TEXT_A	"0.8.1"

#ifdef TVH264
// ƒƒ“ƒZƒOŒü‚¯
#if !defined(TVH264_FOR_1SEG) && defined(BONTSENGINE_1SEG_SUPPORT)
#define TVH264_FOR_1SEG
#endif
#endif

#define LTEXT_(text)	L##text
#define LTEXT(text)		LTEXT_(text)
#define APP_NAME_W		LTEXT(APP_NAME_A)
#define VERSION_TEXT_W	LTEXT(VERSION_TEXT_A)
#ifndef UNICODE
#define APP_NAME		APP_NAME_A
#define VERSION_TEXT	VERSION_TEXT_A
#else
#define APP_NAME		APP_NAME_W
#define VERSION_TEXT	VERSION_TEXT_W
#endif

#if defined(_M_IX86)
#define VERSION_PLATFORM	TEXT("x86")
#elif defined(_M_X64)
#define VERSION_PLATFORM	TEXT("x64")
#endif

#define ABOUT_VERSION_TEXT	APP_NAME TEXT(" ver.") VERSION_TEXT

#ifndef NO_NETWORK_REMOCON
#define NETWORK_REMOCON_SUPPORT
#endif

#ifdef BONTSENGINE_RADIO_SUPPORT
#define TVTEST_RADIO_SUPPORT
#endif


#ifndef RC_INVOKED


#include "Util.h"

#define lengthof _countof

#ifndef CLAMP
#define CLAMP(val,min,max) \
	(((val)>(max))?(max):(((val)<(min))?(min):(val)))
#endif

#ifndef SAFE_DELETE
//#define SAFE_DELETE(p)		if (p) { delete p; (p)=NULL; }
//#define SAFE_DELETE_ARRAY(p)	if (p) { delete [] p; (p)=NULL; }
// delete NULL ‚Å‚à‚¢‚¢‚Ì‚Å
#define SAFE_DELETE(p)			((void)(delete p,(p)=NULL))
#define SAFE_DELETE_ARRAY(p)	((void)(delete [] p,(p)=NULL))
#endif

#define ABSTRACT_DECL			__declspec(novtable)
#define ABSTRACT_CLASS(name)	ABSTRACT_DECL name abstract

#ifndef TVH264
#define CHANNEL_FILE_EXTENSION			TEXT(".ch2")
#else
#define CHANNEL_FILE_EXTENSION			TEXT(".ch1")
#define DEFERRED_CHANNEL_FILE_EXTENSION	TEXT(".ch2")
#endif


#endif	// ndef RC_INVOKED


#endif	// ndef TVTEST_H
