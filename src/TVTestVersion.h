#ifndef TVTEST_VERSION_H
#define TVTEST_VERSION_H


#define VERSION_MAJOR    0
#define VERSION_MINOR    10
#define VERSION_BUILD    0
#define VERSION_REVISION 0

#define VERSION_TEXT_A "0.10.0"

#define VERSION_STATUS_A "dev"

#ifndef RC_INVOKED
#if __has_include("TVTestVersionHash.h")
#include "TVTestVersionHash.h"
#define VERSION_HASH_W   LTEXT(VERSION_HASH_A)
#endif
#endif

#define VERSION_TEXT_W   LTEXT(VERSION_TEXT_A)
#ifdef VERSION_STATUS_A
#define VERSION_STATUS_W LTEXT(VERSION_STATUS_A)
#endif
#ifndef UNICODE
#define VERSION_TEXT     VERSION_TEXT_A
#ifdef VERSION_HASH_A
#define VERSION_HASH     VERSION_HASH_A
#endif
#ifdef VERSION_STATUS_A
#define VERSION_STATUS   VERSION_STATUS_A
#endif
#else
#define VERSION_TEXT     VERSION_TEXT_W
#ifdef VERSION_HASH_W
#define VERSION_HASH     VERSION_HASH_W
#endif
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


#endif
