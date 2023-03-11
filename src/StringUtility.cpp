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


#include "stdafx.h"
#include <cwctype>
#include "TVTest.h"
#include "StringUtility.h"
#include "Util.h"
#include "Common/DebugDef.h"


namespace TVTest
{


LONGLONG StringToInt64(LPCTSTR pszString)
{
	return std::_tcstoll(pszString, nullptr, 0);
}


ULONGLONG StringToUInt64(LPCTSTR pszString)
{
	return std::_tcstoull(pszString, nullptr, 0);
}


bool Int64ToString(LONGLONG Value, LPTSTR pszString, int MaxLength, int Radix)
{
	return _i64tot_s(Value, pszString, MaxLength, Radix) == 0;
}


bool UInt64ToString(ULONGLONG Value, LPTSTR pszString, int MaxLength, int Radix)
{
	return _ui64tot_s(Value, pszString, MaxLength, Radix) == 0;
}


bool StringIsDigit(LPCTSTR pszString)
{
	if (IsStringEmpty(pszString))
		return false;

	LPCTSTR p = pszString;
	if (*p == _T('-') || *p == _T('+'))
		p++;
	if (*p < _T('0') || *p > _T('9'))
		return false;
	p++;
	while (*p != _T('\0')) {
		if (*p < _T('0') || *p > _T('9'))
			return false;
		p++;
	}

	return true;
}


[[nodiscard]] LPSTR DuplicateString(LPCSTR pszString)
{
	if (pszString == nullptr)
		return nullptr;

	const size_t Length = lstrlenA(pszString) + 1;
	const LPSTR pszNewString = new char[Length];
	std::memcpy(pszNewString, pszString, Length);
	return pszNewString;
}


[[nodiscard]] LPWSTR DuplicateString(LPCWSTR pszString)
{
	if (pszString == nullptr)
		return nullptr;

	const size_t Length = lstrlenW(pszString) + 1;
	const LPWSTR pszNewString = new WCHAR[Length];
	std::memcpy(pszNewString, pszString, Length * sizeof(WCHAR));
	return pszNewString;
}


static inline bool IsWhitespace(TCHAR c)
{
	return c == _T(' ') || c == _T('\r') || c == _T('\n') || c == _T('\t');
}


int RemoveTrailingWhitespace(LPTSTR pszString)
{
	if (pszString == nullptr)
		return 0;
	LPTSTR pSpace = nullptr;
	LPTSTR p = pszString;
	while (*p != _T('\0')) {
		if (IsWhitespace(*p)) {
			if (pSpace == nullptr)
				pSpace = p;
		} else if (pSpace != nullptr) {
			pSpace = nullptr;
		}
		p++;
	}
	if (pSpace == nullptr)
		return 0;
	*pSpace = _T('\0');
	return static_cast<int>(p - pSpace);
}


LPTSTR SkipLeadingWhitespace(LPTSTR pszString)
{
	LPTSTR p = pszString;
	while (IsWhitespace(*p))
		p++;
	return p;
}


LPCTSTR SkipLeadingWhitespace(LPCTSTR pszString)
{
	LPCTSTR p = pszString;
	while (IsWhitespace(*p))
		p++;
	return p;
}


namespace StringUtility
{


void Reserve(String &Str, size_t Size)
{
	if (Size > Str.max_size())
		return;

	if (Str.capacity() < Size)
		Str.reserve(Size);
}

void Assign(String &Str, const String::value_type *pszSrc)
{
	if (IsStringEmpty(pszSrc))
		Str.clear();
	else
		Str = pszSrc;
}

const String::value_type *GetCStrOrNull(const String &Str)
{
	if (Str.empty())
		return nullptr;
	return Str.c_str();
}

int Format(String &Str, LPCWSTR pszFormat, ...)
{
	va_list Args;
	va_start(Args, pszFormat);
	const int Length = FormatV(Str, pszFormat, Args);
	va_end(Args);

	return Length;
}

int FormatV(String &Str, LPCWSTR pszFormat, va_list Args)
{
	if (pszFormat == nullptr) {
		Str.clear();
		return 0;
	}

	va_list CopyArgs;
	va_copy(CopyArgs, Args);
	const int Length = ::_vscwprintf(pszFormat, Args);
	if (Length <= 0) {
		Str.clear();
	} else {
		Str.resize(Length + 1);
		::_vsnwprintf_s(Str.data(), Str.length(), _TRUNCATE, pszFormat, CopyArgs);
		Str.pop_back();
	}
	va_end(CopyArgs);

	return static_cast<int>(Str.length());
}

int Compare(const String &String1, LPCWSTR pszString2)
{
	if (IsStringEmpty(pszString2)) {
		if (String1.empty())
			return 0;
		return 1;
	}

	return String1.compare(pszString2);
}

int CompareNoCase(const String &String1, const String &String2)
{
	return ::lstrcmpiW(String1.c_str(), String2.c_str());
}

int CompareNoCase(const String &String1, LPCWSTR pszString2)
{
	if (IsStringEmpty(pszString2)) {
		if (String1.empty())
			return 0;
		return 1;
	}

	return ::lstrcmpiW(String1.c_str(), pszString2);
}

int CompareNoCase(const String &String1, const String &String2, String::size_type Length)
{
	return ::StrCmpNIW(String1.c_str(), String2.c_str(), static_cast<int>(Length));
}

int CompareNoCase(const String &String1, LPCWSTR pszString2, String::size_type Length)
{
	if (IsStringEmpty(pszString2)) {
		if (String1.empty())
			return 0;
		return 1;
	}

	return ::StrCmpNIW(String1.c_str(), pszString2, static_cast<int>(Length));
}

bool Trim(String &Str, LPCWSTR pszSpaces)
{
	if (IsStringEmpty(pszSpaces))
		return false;

	const String::size_type First = Str.find_first_not_of(pszSpaces);
	if (First == String::npos)
		return false;

	Str = Str.substr(First, Str.find_last_not_of(pszSpaces) - First + 1);

	return true;
}

bool Replace(String &Str, LPCWSTR pszFrom, LPCWSTR pszTo)
{
	if (IsStringEmpty(pszFrom))
		return false;

	const String::size_type FromLength = ::lstrlenW(pszFrom);
	const String::size_type ToLength = IsStringEmpty(pszTo) ? 0 : ::lstrlenW(pszTo);

	String::size_type Next = 0, Pos;
	while ((Pos = Str.find(pszFrom, Next)) != String::npos) {
		if (ToLength == 0) {
			Str.erase(Pos, FromLength);
			Next = Pos;
		} else {
			Str.replace(Pos, FromLength, pszTo);
			Next = Pos + ToLength;
		}
	}

	return true;
}

bool Replace(String &Str, String::value_type From, String::value_type To)
{
	String::size_type Next = 0, Pos;
	while ((Pos = Str.find(From, Next)) != String::npos) {
		Str[Pos] = To;
		Next = Pos + 1;
	}

	return true;
}

void ToUpper(String &Str)
{
	::CharUpperBuff(&Str[0], static_cast<DWORD>(Str.length()));
}

void ToLower(String &Str)
{
	::CharLowerBuff(&Str[0], static_cast<DWORD>(Str.length()));
}

bool ToHalfWidthNoKatakana(LPCWSTR pSrc, String::size_type SrcLength, String *pDst)
{
	if (pSrc == nullptr || pDst == nullptr)
		return false;

	pDst->clear();

	for (String::size_type i = 0; i < SrcLength; i++) {
		WORD Type;
		if (::GetStringTypeExW(LOCALE_USER_DEFAULT, CT_CTYPE3, &pSrc[i], 1, &Type)
				&& (Type & (C3_FULLWIDTH | C3_KATAKANA)) == C3_FULLWIDTH) {
			WCHAR Buff[4];
			const int Length = ::LCMapStringW(
				LOCALE_USER_DEFAULT, LCMAP_HALFWIDTH,
				&pSrc[i], 1, Buff, _countof(Buff));
			if (Length > 0) {
				pDst->append(Buff, Length);
				continue;
			}
		}

		pDst->push_back(pSrc[i]);
	}

	return true;
}

bool ToHalfWidthNoKatakana(const String &Src, String *pDst)
{
	return ToHalfWidthNoKatakana(Src.data(), Src.length(), pDst);
}

bool ToHalfWidthNoKatakana(String &Str)
{
	String Temp;

	if (!ToHalfWidthNoKatakana(Str.data(), Str.length(), &Temp))
		return false;
	Str = Temp;
	return true;
}

bool ToHalfWidthNoKatakana(LPCWSTR pszSrc, String *pDst)
{
	if (pszSrc == nullptr)
		return false;
	return ToHalfWidthNoKatakana(pszSrc, ::lstrlenW(pszSrc), pDst);
}

bool ToHalfWidthNoKatakana(LPCWSTR pszSrc, LPWSTR pszDst, String::size_type DstLength)
{
	if (pszDst == nullptr || DstLength < 1)
		return false;
	String Buf;
	if (!ToHalfWidthNoKatakana(pszSrc, &Buf)) {
		pszDst[0] = L'\0';
		return false;
	}
	StringCopy(pszDst, Buf.c_str(), DstLength);
	return true;
}

bool ToAnsi(const String &Src, AnsiString *pDst)
{
	if (pDst == nullptr)
		return false;

	pDst->clear();

	if (!Src.empty()) {
		const int Length = ::WideCharToMultiByte(CP_ACP, 0, Src.data(), static_cast<int>(Src.length()), nullptr, 0, nullptr, nullptr);
		if (Length < 1)
			return false;
		pDst->resize(Length);
		::WideCharToMultiByte(CP_ACP, 0, Src.data(), static_cast<int>(Src.length()), pDst->data(), Length, nullptr, nullptr);
	}

	return true;
}

bool Split(const String &Src, LPCWSTR pszDelimiter, std::vector<String> *pList)
{
	if (pList == nullptr)
		return false;

	pList->clear();

	if (IsStringEmpty(pszDelimiter))
		return false;

	const int DelimiterLength = ::lstrlenW(pszDelimiter);
	String::const_iterator SrcBegin = Src.begin();
	String::size_type Pos, Next = 0;
	while ((Pos = Src.find(pszDelimiter, Next)) != String::npos) {
		pList->emplace_back(SrcBegin + Next, SrcBegin + Pos);
		Next = Pos + DelimiterLength;
	}
	pList->emplace_back(SrcBegin + Next, Src.end());

	return true;
}

bool Combine(const std::vector<String> &List, LPCWSTR pszDelimiter, String *pDst)
{
	if (pDst == nullptr)
		return false;

	pDst->clear();

	if (!List.empty()) {
		for (auto i = List.begin();;) {
			pDst->append(*i);
			i++;
			if (i == List.end())
				break;
			if (pszDelimiter != nullptr)
				pDst->append(pszDelimiter);
		}
	}

	return true;
}

bool Encode(LPCWSTR pszSrc, String *pDst, LPCWSTR pszEncodeChars)
{
	if (pszSrc == nullptr || pDst == nullptr)
		return false;

	pDst->clear();

	LPCWSTR p = pszSrc;
	while (*p != L'\0') {
		bool fEncode = false;

		if (*p <= 0x19 || *p == L'%')
			fEncode = true;
		else if (pszEncodeChars != nullptr)
			fEncode = ::StrChr(pszEncodeChars, *p) != nullptr;
		if (fEncode) {
			WCHAR szCode[8];
			StringFormat(szCode, L"%{:04X}", *p);
			*pDst += szCode;
		} else {
			pDst->push_back(*p);
		}
		p++;
	}

	return true;
}

String Encode(const String &Src, LPCWSTR pszEncodeChars)
{
	String Dst;

	Encode(Src.c_str(), &Dst, pszEncodeChars);

	return Dst;
}

bool Decode(LPCWSTR pszSrc, String *pDst)
{
	if (pszSrc == nullptr || pDst == nullptr)
		return false;

	pDst->clear();

	LPCWSTR p = pszSrc;
	while (*p != L'\0') {
		if (*p == L'%') {
			p++;
			const WCHAR Code = static_cast<WCHAR>(HexStringToUInt(p, 4, &p));
			pDst->push_back(Code);
		} else {
			pDst->push_back(*p);
			p++;
		}
	}

	return true;
}

String Decode(const String &Src)
{
	String Dst;

	Decode(Src.c_str(), &Dst);

	return Dst;
}


// FNV hash parameters
static constexpr UINT32 FNV_PRIME_32 = 16777619UL;
static constexpr UINT32 FNV_OFFSET_BASIS_32 = 2166136261UL;
static constexpr UINT64 FNV_PRIME_64 = 1099511628211ULL;
static constexpr UINT64 FNV_OFFSET_BASIS_64 = 14695981039346656037ULL;

template<typename TIterator, typename THash> THash FNVHash(
	const TIterator &begin, const TIterator &end,
	const THash OffsetBasis, const THash Prime)
{
	THash Hash = OffsetBasis;

	for (auto i = begin; i != end; ++i)
		Hash = (Prime * Hash) ^ (*i);
	return Hash;
}

template<typename TIterator, typename TTransform, typename THash> THash FNVHash(
	const TIterator &begin, const TIterator &end, TTransform Transform,
	const THash OffsetBasis, const THash Prime)
{
	THash Hash = OffsetBasis;

	for (auto i = begin; i != end; ++i)
		Hash = (Prime * Hash) ^ Transform(*i);
	return Hash;
}

UINT32 Hash32(const String &Str)
{
	return FNVHash(Str.begin(), Str.end(), FNV_OFFSET_BASIS_32, FNV_PRIME_32);
}

UINT64 Hash64(const String &Str)
{
	return FNVHash(Str.begin(), Str.end(), FNV_OFFSET_BASIS_64, FNV_PRIME_64);
}

UINT32 HashNoCase32(const String &Str)
{
	return FNVHash(Str.begin(), Str.end(), std::towlower, FNV_OFFSET_BASIS_32, FNV_PRIME_32);
}

UINT64 HashNoCase64(const String &Str)
{
	return FNVHash(Str.begin(), Str.end(), std::towlower, FNV_OFFSET_BASIS_64, FNV_PRIME_64);
}


}	// namespace StringUtility

}	// namespace TVTest
