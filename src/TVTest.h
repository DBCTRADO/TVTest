/*
  TVTest
  Copyright(c) 2008-2020 DBCTRADO

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


#ifndef TVTEST_H
#define TVTEST_H


#define LTEXT_(text) L##text
#define LTEXT(text)  LTEXT_(text)

#define APP_NAME_A "TVTest"
#define APP_NAME_W LTEXT(APP_NAME_A)
#ifndef UNICODE
#define APP_NAME APP_NAME_A
#else
#define APP_NAME APP_NAME_W
#endif


#ifndef RC_INVOKED


#include "LibISDB/LibISDB/LibISDB.hpp"
#include "LibISDB/LibISDB/Base/Debug.hpp"


#define TRACE LIBISDB_TRACE
#define TVTEST_ASSERT LIBISDB_ASSERT


namespace TVTest
{
	using namespace LibISDB::Literals;
	using namespace LibISDB::EnumFlags;

	namespace Concept
	{
		using namespace LibISDB::Concept;
	}

#define TVTEST_ENUM_FLAGS_TRAILER_ LIBISDB_ENUM_FLAGS_TRAILER_
#define TVTEST_ENUM_FLAGS_TRAILER LIBISDB_ENUM_FLAGS_TRAILER

#define TVTEST_ENUM_CLASS_TRAILER_(first) \
	Trailer_, \
	First_ = first, \
	Last_ = Trailer_ - 1
#define TVTEST_ENUM_CLASS_TRAILER \
	TVTEST_ENUM_CLASS_TRAILER_(0)

	template<typename T> constexpr bool CheckEnumRange(T val) {
		return (val >= T::First_) && (val <= T::Last_);
	}
}


#include "Util.h"


#define lengthof _countof

#define ABSTRACT_DECL        __declspec(novtable)
#define ABSTRACT_CLASS(name) ABSTRACT_DECL name abstract

#define CHANNEL_FILE_EXTENSION          TEXT(".ch2")
#define DEFERRED_CHANNEL_FILE_EXTENSION TEXT(".ch1")


#endif // ndef RC_INVOKED


#endif // ndef TVTEST_H
