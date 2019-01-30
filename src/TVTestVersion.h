/*
  TVTest
  Copyright(c) 2008-2019 DBCTRADO

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


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
