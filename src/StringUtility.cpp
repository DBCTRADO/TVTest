#include "stdafx.h"
#include <cwctype>
#include "StringUtility.h"
#include "Util.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


LONGLONG StringToInt64(LPCTSTR pszString)
{
	return _tcstoi64(pszString,nullptr,0);
}


ULONGLONG StringToUInt64(LPCTSTR pszString)
{
	return _tcstoui64(pszString,nullptr,0);
}


bool Int64ToString(LONGLONG Value,LPTSTR pszString,int MaxLength,int Radix)
{
	return _i64tot_s(Value,pszString,MaxLength,Radix)==0;
}


bool UInt64ToString(ULONGLONG Value,LPTSTR pszString,int MaxLength,int Radix)
{
	return _ui64tot_s(Value,pszString,MaxLength,Radix)==0;
}


bool StringIsDigit(LPCTSTR pszString)
{
	if (IsStringEmpty(pszString))
		return false;

	LPCTSTR p=pszString;
	if (*p==_T('-') || *p==_T('+'))
		p++;
	if (*p<_T('0') || *p>_T('9'))
		return false;
	p++;
	while (*p!=_T('\0')) {
		if (*p<_T('0') || *p>_T('9'))
			return false;
		p++;
	}

	return true;
}


__declspec(restrict) LPSTR DuplicateString(LPCSTR pszString)
{
	if (pszString==NULL)
		return NULL;

	const size_t Length=lstrlenA(pszString)+1;
	LPSTR pszNewString=new char[Length];
	::CopyMemory(pszNewString,pszString,Length);
	return pszNewString;
}


__declspec(restrict) LPWSTR DuplicateString(LPCWSTR pszString)
{
	if (pszString==NULL)
		return NULL;

	const size_t Length=lstrlenW(pszString)+1;
	LPWSTR pszNewString=new WCHAR[Length];
	::CopyMemory(pszNewString,pszString,Length*sizeof(WCHAR));
	return pszNewString;
}


bool ReplaceString(LPSTR *ppszString,LPCSTR pszNewString)
{
	if (ppszString==NULL)
		return false;
	delete [] *ppszString;
	*ppszString=DuplicateString(pszNewString);
	return true;
}


bool ReplaceString(LPWSTR *ppszString,LPCWSTR pszNewString)
{
	if (ppszString==NULL)
		return false;
	delete [] *ppszString;
	*ppszString=DuplicateString(pszNewString);
	return true;
}


static inline bool IsWhitespace(TCHAR c)
{
	return c==_T(' ') || c==_T('\r') || c==_T('\n') || c==_T('\t');
}


int RemoveTrailingWhitespace(LPTSTR pszString)
{
	if (pszString==NULL)
		return 0;
	LPTSTR pSpace=NULL;
	LPTSTR p=pszString;
	while (*p!=_T('\0')) {
		if (IsWhitespace(*p)) {
			if (pSpace==NULL)
				pSpace=p;
		} else if (pSpace!=NULL) {
			pSpace=NULL;
		}
		p++;
	}
	if (pSpace==NULL)
		return 0;
	*pSpace=_T('\0');
	return (int)(p-pSpace);
}


LPTSTR SkipLeadingWhitespace(LPTSTR pszString)
{
	LPTSTR p=pszString;
	while (IsWhitespace(*p))
		p++;
	return p;
}


LPCTSTR SkipLeadingWhitespace(LPCTSTR pszString)
{
	LPCTSTR p=pszString;
	while (IsWhitespace(*p))
		p++;
	return p;
}


namespace TVTest
{

	namespace StringUtility
	{

		void Reserve(String &Str,size_t Size)
		{
			if (Size>size_t(Str.max_size()))
				return;

			if (Str.capacity()<Size)
				Str.reserve(Size);
		}

		void Assign(String &Str,const String::value_type *pszSrc)
		{
			if (IsStringEmpty(pszSrc))
				Str.clear();
			else
				Str=pszSrc;
		}

		const String::value_type *GetCStrOrNull(const String &Str)
		{
			if (Str.empty())
				return nullptr;
			return Str.c_str();
		}

		int Format(String &Str,LPCWSTR pszFormat, ...)
		{
			va_list Args;
			va_start(Args,pszFormat);
			int Length=FormatV(Str,pszFormat,Args);
			va_end(Args);

			return Length;
		}

		int FormatV(String &Str,LPCWSTR pszFormat,va_list Args)
		{
			if (pszFormat==nullptr) {
				Str.clear();
				return 0;
			}

			int Length=::_vscwprintf(pszFormat,Args);
			if (Length<=0) {
				Str.clear();
				return 0;
			}
			static const int BUFFER_LENGTH=256;
			if (Length<BUFFER_LENGTH) {
				WCHAR szBuffer[BUFFER_LENGTH];
				::_vsnwprintf_s(szBuffer,BUFFER_LENGTH,_TRUNCATE,pszFormat,Args);
				Str=szBuffer;
			} else {
				LPWSTR pszBuffer=new WCHAR[Length+1];
				::_vsnwprintf_s(pszBuffer,Length+1,_TRUNCATE,pszFormat,Args);
				Str=pszBuffer;
				delete [] pszBuffer;
			}

			return (int)Str.length();
		}

		int CompareNoCase(const String &String1,const String &String2)
		{
			return ::lstrcmpiW(String1.c_str(),String2.c_str());
		}

		int CompareNoCase(const String &String1,LPCWSTR pszString2)
		{
			if (IsStringEmpty(pszString2)) {
				if (String1.empty())
					return 0;
				return 1;
			}

			return ::lstrcmpiW(String1.c_str(),pszString2);
		}

		int CompareNoCase(const String &String1,const String &String2,String::size_type Length)
		{
			return ::StrCmpNIW(String1.c_str(),String2.c_str(),(int)Length);
		}

		int CompareNoCase(const String &String1,LPCWSTR pszString2,String::size_type Length)
		{
			if (IsStringEmpty(pszString2)) {
				if (String1.empty())
					return 0;
				return 1;
			}

			return ::StrCmpNIW(String1.c_str(),pszString2,(int)Length);
		}

		bool Trim(String &Str,LPCWSTR pszSpaces)
		{
			if (IsStringEmpty(pszSpaces))
				return false;

			const String::size_type First=Str.find_first_not_of(pszSpaces);
			if (First==String::npos)
				return false;

			Str=Str.substr(First,Str.find_last_not_of(pszSpaces)-First+1);

			return true;
		}

		bool Replace(String &Str,LPCWSTR pszFrom,LPCWSTR pszTo)
		{
			if (IsStringEmpty(pszFrom))
				return false;

			const String::size_type FromLength=::lstrlenW(pszFrom);
			const String::size_type ToLength=IsStringEmpty(pszTo) ? 0 : ::lstrlenW(pszTo);

			String::size_type Next=0,Pos;
			while ((Pos=Str.find(pszFrom,Next))!=String::npos) {
				if (ToLength==0) {
					Str.erase(Pos,FromLength);
					Next=Pos;
				} else {
					Str.replace(Pos,FromLength,pszTo);
					Next=Pos+ToLength;
				}
			}

			return true;
		}

		bool Replace(String &Str,String::value_type From,String::value_type To)
		{
			String::size_type Next=0,Pos;
			while ((Pos=Str.find(From,Next))!=String::npos) {
				Str[Pos]=To;
				Next=Pos+1;
			}

			return true;
		}

		void ToUpper(String &Str)
		{
			::CharUpperBuff(&Str[0],static_cast<DWORD>(Str.length()));
		}

		void ToLower(String &Str)
		{
			::CharLowerBuff(&Str[0],static_cast<DWORD>(Str.length()));
		}

		bool ToHalfWidthNoKatakana(LPCWSTR pSrc,String::size_type SrcLength,String *pDst)
		{
			if (pSrc==nullptr || pDst==nullptr)
				return false;

			pDst->clear();

			for (String::size_type i=0;i<SrcLength;i++) {
				WORD Type;
				if (::GetStringTypeExW(LOCALE_USER_DEFAULT,CT_CTYPE3,&pSrc[i],1,&Type)
						&& (Type & (C3_FULLWIDTH | C3_KATAKANA))==C3_FULLWIDTH) {
					WCHAR Buff[4];
					int Length=::LCMapStringW(LOCALE_USER_DEFAULT,LCMAP_HALFWIDTH,
											  &pSrc[i],1,Buff,_countof(Buff));
					if (Length>0) {
						pDst->append(Buff,Length);
						continue;
					}
				}

				pDst->push_back(pSrc[i]);
			}

			return true;
		}

		bool ToHalfWidthNoKatakana(const String &Src,String *pDst)
		{
			return ToHalfWidthNoKatakana(Src.data(),Src.length(),pDst);
		}

		bool ToHalfWidthNoKatakana(LPCWSTR pszSrc,String *pDst)
		{
			if (pszSrc==nullptr)
				return false;
			return ToHalfWidthNoKatakana(pszSrc,::lstrlenW(pszSrc),pDst);
		}

		bool ToHalfWidthNoKatakana(LPCWSTR pszSrc,LPWSTR pszDst,String::size_type DstLength)
		{
			if (pszDst==nullptr || DstLength<1)
				return false;
			String Buf;
			if (!ToHalfWidthNoKatakana(pszSrc,&Buf)) {
				pszDst[0]=L'\0';
				return false;
			}
			StdUtil::strncpy(pszDst,DstLength,Buf.c_str());
			return true;
		}

		bool ToAnsi(const String &Src,AnsiString *pDst)
		{
			if (pDst==nullptr)
				return false;

			pDst->clear();

			if (!Src.empty()) {
				int Length=::WideCharToMultiByte(CP_ACP,0,Src.data(),(int)Src.length(),NULL,0,NULL,NULL);
				if (Length<1)
					return false;
				char *pszBuffer=new char[Length+1];
				::WideCharToMultiByte(CP_ACP,0,Src.data(),(int)Src.length(),pszBuffer,Length,NULL,NULL);
				pszBuffer[Length]='\0';
				pDst->assign(pszBuffer);
				delete [] pszBuffer;
			}

			return true;
		}

		bool Split(const String &Src,LPCWSTR pszDelimiter,std::vector<String> *pList)
		{
			if (pList==nullptr)
				return false;

			pList->clear();

			if (IsStringEmpty(pszDelimiter))
				return false;

			const int DelimiterLength=::lstrlenW(pszDelimiter);
			String::const_iterator SrcBegin=Src.begin();
			String::size_type Pos,Next=0;
			while ((Pos=Src.find(pszDelimiter,Next))!=String::npos) {
				pList->push_back(String(SrcBegin+Next,SrcBegin+Pos));
				Next=Pos+DelimiterLength;
			}
			pList->push_back(String(SrcBegin+Next,Src.end()));

			return true;
		}

		bool Combine(const std::vector<String> &List,LPCWSTR pszDelimiter,String *pDst)
		{
			if (pDst==nullptr)
				return false;

			pDst->clear();

			if (!List.empty()) {
				for (auto i=List.begin();;) {
					pDst->append(*i);
					i++;
					if (i==List.end())
						break;
					if (pszDelimiter!=nullptr)
						pDst->append(pszDelimiter);
				}
			}

			return true;
		}

		bool Encode(LPCWSTR pszSrc,String *pDst,LPCWSTR pszEncodeChars)
		{
			if (pszSrc==nullptr || pDst==nullptr)
				return false;

			pDst->clear();

			LPCWSTR p=pszSrc;
			while (*p!=L'\0') {
				bool fEncode=false;

				if (*p<=0x19 || *p==L'%')
					fEncode=true;
				else if (pszEncodeChars!=nullptr)
					fEncode=::StrChr(pszEncodeChars,*p)!=nullptr;
				if (fEncode) {
					WCHAR szCode[8];
					StdUtil::snprintf(szCode,_countof(szCode),L"%%%04X",*p);
					*pDst+=szCode;
				} else {
					pDst->push_back(*p);
				}
				p++;
			}

			return true;
		}

		String Encode(const String &Src,LPCWSTR pszEncodeChars)
		{
			String Dst;

			Encode(Src.c_str(),&Dst,pszEncodeChars);

			return Dst;
		}

		bool Decode(LPCWSTR pszSrc,String *pDst)
		{
			if (pszSrc==nullptr || pDst==nullptr)
				return false;

			pDst->clear();

			LPCWSTR p=pszSrc;
			while (*p!=L'\0') {
				if (*p==L'%') {
					p++;
					WCHAR Code=(WCHAR)HexStringToUInt(p,4,&p);
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

			Decode(Src.c_str(),&Dst);

			return Dst;
		}


		// FNV hash parameters
		static const UINT32 FNV_PRIME_32=16777619UL;
		static const UINT32 FNV_OFFSET_BASIS_32=2166136261UL;
		static const UINT64 FNV_PRIME_64=1099511628211ULL;
		static const UINT64 FNV_OFFSET_BASIS_64=14695981039346656037ULL;

		template<typename TIterator,typename THash> THash FNVHash(
			const TIterator &begin,const TIterator &end,
			const THash OffsetBasis,const THash Prime)
		{
			THash Hash=OffsetBasis;

			for (auto i=begin;i!=end;++i)
				Hash=(Prime*Hash)^(*i);
			return Hash;
		}

		template<typename TIterator,typename TTransform,typename THash> THash FNVHash(
			const TIterator &begin,const TIterator &end,TTransform Transform,
			const THash OffsetBasis,const THash Prime)
		{
			THash Hash=OffsetBasis;

			for (auto i=begin;i!=end;++i)
				Hash=(Prime*Hash)^Transform(*i);
			return Hash;
		}

		UINT32 Hash32(const String &Str)
		{
			return FNVHash(Str.begin(),Str.end(),FNV_OFFSET_BASIS_32,FNV_PRIME_32);
		}

		UINT64 Hash64(const String &Str)
		{
			return FNVHash(Str.begin(),Str.end(),FNV_OFFSET_BASIS_64,FNV_PRIME_64);
		}

		UINT32 HashNoCase32(const String &Str)
		{
			return FNVHash(Str.begin(),Str.end(),std::towlower,FNV_OFFSET_BASIS_32,FNV_PRIME_32);
		}

		UINT64 HashNoCase64(const String &Str)
		{
			return FNVHash(Str.begin(),Str.end(),std::towlower,FNV_OFFSET_BASIS_64,FNV_PRIME_64);
		}

	}

}
