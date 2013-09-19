#include "stdafx.h"
#include "StdUtil.h"
#include <new>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




int StdUtil::snprintf(char *s,size_t n,const char *format, ...)
{
	va_list args;
	int Length;

	va_start(args,format);
#if defined(__STDC_VERSION__) && __STDC_VERSION__>=199901L
	// 実質的にVCでしかコンパイルできないので無意味だが...
	Length=::vsnprintf(s,n,format,args);
#else
	if (n>0) {
		Length=::vsprintf_s(s,n,format,args);
	} else {
		Length=0;
	}
#endif
	va_end(args);
	return Length;
}


int StdUtil::snprintf(wchar_t *s,size_t n,const wchar_t *format, ...)
{
	va_list args;
	int Length;

	va_start(args,format);
	if (n>0) {
		Length=::vswprintf_s(s,n,format,args);
	} else {
		Length=0;
	}
	va_end(args);
	return Length;
}


int StdUtil::vsnprintf(char *s,size_t n,const char *format,va_list args)
{
	int Length;

#if defined(__STDC_VERSION__) && __STDC_VERSION__>=199901L
	Length=::vsnprintf(s,n,format,args);
#else
	if (n>0) {
		Length=::vsprintf_s(s,n,format,args);
	} else {
		Length=0;
	}
#endif
	return Length;
}


int StdUtil::vsnprintf(wchar_t *s,size_t n,const wchar_t *format,va_list args)
{
	int Length;

	if (n>0) {
		Length=::vswprintf_s(s,n,format,args);
	} else {
		Length=0;
	}
	return Length;
}


char *StdUtil::strncpy(char *dest,size_t n,const char *src)
{
#if 0
	size_t length=::strlen(src);

	if (n-1<length) {
		::memcpy(dest,src,n-1);
		dest[n-1]='\0';
	} else {
		::strcpy(dest,src);
	}
#else
	::strncpy_s(dest,n,src,_TRUNCATE);
#endif
	return dest;
}


wchar_t *StdUtil::strncpy(wchar_t *dest,size_t n,const wchar_t *src)
{
#if 0
	size_t length=::wcslen(src);

	if (n-1<length) {
		::memcpy(dest,src,(n-1)*sizeof(wchar_t));
		dest[n-1]=L'\0';
	} else {
		::wcscpy(dest,src);
	}
#else
	::wcsncpy_s(dest,n,src,_TRUNCATE);
#endif
	return dest;
}


char *StdUtil::strdup(const char *s)
{
	if (!s)
		return NULL;
	size_t length=::strlen(s)+1;
	char *dup;
	try {
		dup=new char[length];
	} catch (std::bad_alloc&) {
		return NULL;
	}
	::strncpy_s(dup,length,s,_TRUNCATE);
	return dup;
}


wchar_t *StdUtil::strdup(const wchar_t *s)
{
	if (!s)
		return NULL;
	size_t length=::wcslen(s)+1;
	wchar_t *dup;
	try {
		dup=new wchar_t[length];
	} catch (std::bad_alloc&) {
		return NULL;
	}
	::wcsncpy_s(dup,length,s,_TRUNCATE);
	return dup;
}
