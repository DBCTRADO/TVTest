#include "stdafx.h"


#ifdef _DEBUG
void DebugTrace(LPCTSTR szFormat, ...)
{
	TCHAR szTempStr[1024];
	int Length;

	SYSTEMTIME st;
	::GetLocalTime(&st);
	Length = ::_stprintf_s(szTempStr, _countof(szTempStr),
						   TEXT("%02d/%02d %02d:%02d:%02d > "),
						   st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	va_list Args;
	va_start(Args, szFormat);
	::_vstprintf_s(szTempStr + Length, _countof(szTempStr) - Length, szFormat, Args);
	va_end(Args);

	::OutputDebugString(szTempStr);
}
#endif
