#ifndef STD_UTIL_H
#define STD_UTIL_H


namespace StdUtil {

int snprintf(char *s,size_t n,const char *format, ...);
int snprintf(wchar_t *s,size_t n,const wchar_t *format, ...);
int vsnprintf(char *s,size_t n,const char *format,va_list args);
int vsnprintf(wchar_t *s,size_t n,const wchar_t *format,va_list args);
inline size_t strlen(const char *s) { return ::strlen(s); }
inline size_t strlen(const wchar_t *s) { return ::wcslen(s); }
inline size_t strnlen(const char *s,size_t n) { return ::strnlen(s,n); }
inline size_t strnlen(const wchar_t *s,size_t n) { return ::wcsnlen(s,n); }
inline char *strcpy(char *dest,const char *src) { return ::strcpy(dest,src); }
inline wchar_t *strcpy(wchar_t *dest,const wchar_t *src) { return ::wcscpy(dest,src); }
char *strncpy(char *dest,size_t n,const char *src);
wchar_t *strncpy(wchar_t *dest,size_t n,const wchar_t *src);
char *strdup(const char *s);
wchar_t *strdup(const wchar_t *s);

}


#endif
