#ifndef TVTEST_STRING_UTILITY_H
#define TVTEST_STRING_UTILITY_H


#include <string>
#include <vector>


LONGLONG StringToInt64(LPCTSTR pszString);
ULONGLONG StringToUInt64(LPCTSTR pszString);
bool Int64ToString(LONGLONG Value,LPTSTR pszString,int MaxLength,int Radix=10);
bool UInt64ToString(ULONGLONG Value,LPTSTR pszString,int MaxLength,int Radix=10);
bool StringIsDigit(LPCTSTR pszString);

__declspec(restrict) LPSTR DuplicateString(LPCSTR pszString);
__declspec(restrict) LPWSTR DuplicateString(LPCWSTR pszString);
bool ReplaceString(LPSTR *ppszString,LPCSTR pszNewString);
bool ReplaceString(LPWSTR *ppszString,LPCWSTR pszNewString);
int RemoveTrailingWhitespace(LPTSTR pszString);
LPTSTR SkipLeadingWhitespace(LPTSTR pszString);
LPCTSTR SkipLeadingWhitespace(LPCTSTR pszString);

inline bool IsStringEmpty(LPCSTR pszString) {
	return pszString==NULL || pszString[0]=='\0';
}
inline bool IsStringEmpty(LPCWSTR pszString) {
	return pszString==NULL || pszString[0]==L'\0';
}
inline LPCSTR NullToEmptyString(LPCSTR pszString) {
	return pszString!=NULL?pszString:"";
}
inline LPCWSTR NullToEmptyString(LPCWSTR pszString) {
	return pszString!=NULL?pszString:L"";
}

inline LPSTR StringNextChar(LPCSTR pszCur)
{
	return ::CharNextA(pszCur);
}

inline LPWSTR StringNextChar(LPCWSTR pszCur)
{
	if (*pszCur!=L'\0') {
		if (IS_SURROGATE_PAIR(*pszCur,*(pszCur+1)))
			return const_cast<LPWSTR>(pszCur+2);
	}
	return ::CharNextW(pszCur);
}

inline LPSTR StringPrevChar(LPCSTR pszString,LPCSTR pszCur)
{
	return ::CharPrevA(pszString,pszCur);
}

inline LPWSTR StringPrevChar(LPCWSTR pszString,LPCWSTR pszCur)
{
	LPWSTR pPrev=::CharPrevW(pszString,pszCur);
	if (pPrev>pszString && IS_SURROGATE_PAIR(*(pPrev-1),*pPrev))
		pPrev--;
	return pPrev;
}

template<typename T> int StringCharLength(T pszString)
{
	return (int)(StringNextChar(pszString)-pszString);
}


namespace TVTest
{

	typedef std::wstring String;
	typedef std::string AnsiString;

	namespace StringUtility
	{

		void Reserve(String &Str,size_t Size);
		void Assign(String &Str,const String::value_type *pszSrc);
		const String::value_type *GetCStrOrNull(const String &Str);
		int Format(String &Str,LPCWSTR pszFormat, ...);
		int FormatV(String &Str,LPCWSTR pszFormat,va_list Args);
		int Compare(const String &String1,LPCWSTR pszString2);
		int CompareNoCase(const String &String1,const String &String2);
		int CompareNoCase(const String &String1,LPCWSTR pszString2);
		int CompareNoCase(const String &String1,const String &String2,String::size_type Length);
		int CompareNoCase(const String &String1,LPCWSTR pszString2,String::size_type Length);
		bool Trim(String &Str,LPCWSTR pszSpaces=L" \t");
		bool Replace(String &Str,LPCWSTR pszFrom,LPCWSTR pszTo);
		bool Replace(String &Str,String::value_type From,String::value_type To);
		void ToUpper(String &Str);
		void ToLower(String &Str);
		bool ToHalfWidthNoKatakana(LPCWSTR pSrc,String::size_type SrcLength,String *pDst);
		bool ToHalfWidthNoKatakana(const String &Src,String *pDst);
		bool ToHalfWidthNoKatakana(LPCWSTR pszSrc,String *pDst);
		bool ToHalfWidthNoKatakana(LPCWSTR pszSrc,LPWSTR pszDst,String::size_type DstLength);
		bool ToAnsi(const String &Src,AnsiString *pDst);
		bool Split(const String &Src,LPCWSTR pszDelimiter,std::vector<String> *pList);
		bool Combine(const std::vector<String> &List,LPCWSTR pszDelimiter,String *pDst);
		bool Encode(LPCWSTR pszSrc,String *pDst,LPCWSTR pszEncodeChars=L"\\\"\',/");
		String Encode(const String &Src,LPCWSTR pszEncodeChars=L"\\\"\',/");
		bool Decode(LPCWSTR pszSrc,String *pDst);
		String Decode(const String &Src);

		UINT32 Hash32(const String &Str);
		UINT64 Hash64(const String &Str);
		UINT32 HashNoCase32(const String &Str);
		UINT64 HashNoCase64(const String &Str);
		inline std::size_t Hash(const String &Str) {
#ifndef _WIN64
			return Hash32(Str);
#else
			return Hash64(Str);
#endif
		}
		inline std::size_t HashNoCase(const String &Str) {
#ifndef _WIN64
			return HashNoCase32(Str);
#else
			return HashNoCase64(Str);
#endif
		}

	}

	namespace StringFunctional
	{

		struct Equal {
			bool operator()(const String &Str1,const String &Str2) const {
				return Str1==Str2;
			}
		};

		struct EqualNoCase {
			bool operator()(const String &Str1,const String &Str2) const {
				return StringUtility::CompareNoCase(Str1,Str2)==0;
			}
		};

		struct LessNoCase {
			bool operator()(const String &Str1,const String &Str2) const {
				return StringUtility::CompareNoCase(Str1,Str2)<0;
			}
		};

		struct Hash {
			std::size_t operator()(const String &Str) const {
				return StringUtility::Hash(Str);
			}
		};

		struct HashNoCase {
			std::size_t operator()(const String &Str) const {
				return StringUtility::HashNoCase(Str);
			}
		};

	}

	inline String StringFromCStr(const String::value_type *pszSrc) {
		String Str;
		StringUtility::Assign(Str,pszSrc);
		return Str;
	}

}


#endif
