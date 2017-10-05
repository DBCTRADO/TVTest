#ifndef TVTEST_H
#define TVTEST_H


#define APP_NAME_A	"TVTest"

#define VERSION_MAJOR    0
#define VERSION_MINOR    10
#define VERSION_BUILD    0
#define VERSION_REVISION 0

#define VERSION_TEXT_A "0.10.0"

#define VERSION_STATUS_A "dev"

#define LTEXT_(text) L##text
#define LTEXT(text)  LTEXT_(text)
#define APP_NAME_W       LTEXT(APP_NAME_A)
#define VERSION_TEXT_W   LTEXT(VERSION_TEXT_A)
#ifdef VERSION_STATUS_A
#define VERSION_STATUS_W LTEXT(VERSION_STATUS_A)
#endif
#ifndef UNICODE
#define APP_NAME         APP_NAME_A
#define VERSION_TEXT     VERSION_TEXT_A
#ifdef VERSION_STATUS_A
#define VERSION_STATUS   VERSION_STATUS_A
#endif
#else
#define APP_NAME         APP_NAME_W
#define VERSION_TEXT     VERSION_TEXT_W
#ifdef VERSION_STATUS_W
#define VERSION_STATUS   VERSION_STATUS_W
#endif
#endif

#if defined(_M_IX86)
#define VERSION_PLATFORM TEXT("x86")
#elif defined(_M_X64)
#define VERSION_PLATFORM TEXT("x64")
#endif

#ifdef VERSION_STATUS
#define ABOUT_VERSION_TEXT APP_NAME TEXT(" ver.") VERSION_TEXT TEXT("-") VERSION_STATUS
#else
#define ABOUT_VERSION_TEXT APP_NAME TEXT(" ver.") VERSION_TEXT
#endif


#ifndef RC_INVOKED


#include "LibISDB/LibISDB/LibISDB.hpp"
#include "Util.h"


namespace TVTest
{
	using namespace LibISDB::Literals;
	using namespace LibISDB::EnumFlags;
}

#define lengthof _countof

#ifndef CLAMP
#define CLAMP(val, min, max) \
	(((val) > (max)) ? (max) : (((val) < (min)) ? (min) : (val)))
#endif

#ifndef SAFE_DELETE
//#define SAFE_DELETE(p)       if (p) { delete p; (p) = nullptr; }
//#define SAFE_DELETE_ARRAY(p) if (p) { delete [] p; (p) = nullptr; }
// delete nullptr でもいいので
#define SAFE_DELETE(p)       ((void)(delete p, (p) = nullptr))
#define SAFE_DELETE_ARRAY(p) ((void)(delete [] p, (p) = nullptr))
#endif

#define ABSTRACT_DECL        __declspec(novtable)
#define ABSTRACT_CLASS(name) ABSTRACT_DECL name abstract

#define CHANNEL_FILE_EXTENSION          TEXT(".ch2")
#define DEFERRED_CHANNEL_FILE_EXTENSION TEXT(".ch1")


#endif	// ndef RC_INVOKED


#endif	// ndef TVTEST_H
