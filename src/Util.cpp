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


#include "stdafx.h"
#include <math.h>
#include "TVTest.h"
#include "Util.h"
#include "DPIUtil.h"
#include "Common/DebugDef.h"


namespace TVTest
{


int HexCharToInt(TCHAR Code)
{
	if (Code >= _T('0') && Code <= _T('9'))
		return Code - _T('0');
	if (Code >= _T('A') && Code <= _T('F'))
		return Code - _T('A') + 10;
	if (Code >= _T('a') && Code <= _T('f'))
		return Code - _T('a') + 10;
	return 0;
}


unsigned int HexStringToUInt(LPCTSTR pszString, int Length, LPCTSTR *ppszEnd)
{
	unsigned int Value = 0;
	int i;
	for (i = 0; i < Length; i++) {
		TCHAR Code = pszString[i];
		unsigned int v;
		if (Code >= _T('0') && Code <= _T('9'))
			v = Code - _T('0');
		else if (Code >= _T('A') && Code <= _T('F'))
			v = Code - _T('A') + 10;
		else if (Code >= _T('a') && Code <= _T('f'))
			v = Code - _T('a') + 10;
		else
			break;
		Value = (Value << 4) | v;
	}
	if (ppszEnd != nullptr)
		*ppszEnd = pszString + i;
	return Value;
}


bool IsRectIntersect(const RECT *pRect1, const RECT *pRect2)
{
	return pRect1->left < pRect2->right && pRect1->right > pRect2->left
		&& pRect1->top < pRect2->bottom && pRect1->bottom > pRect2->top;
}


float LevelToDeciBel(int Level)
{
	float Volume;

	if (Level <= 0)
		Volume = -100.0f;
	else if (Level >= 100)
		Volume = 0.0f;
	else
		Volume = (float)(20.0 * log10((double)Level / 100.0));
	return Volume;
}


COLORREF MixColor(COLORREF Color1, COLORREF Color2, BYTE Ratio)
{
	return RGB(
		(GetRValue(Color1) * Ratio + GetRValue(Color2) * (255 - Ratio)) / 255,
		(GetGValue(Color1) * Ratio + GetGValue(Color2) * (255 - Ratio)) / 255,
		(GetBValue(Color1) * Ratio + GetBValue(Color2) * (255 - Ratio)) / 255);
}


COLORREF HSVToRGB(double Hue, double Saturation, double Value)
{
	double r, g, b;

	if (Saturation == 0.0) {
		r = g = b = Value;
	} else {
		double h, s, v;
		double f, p, q, t;

		h = Hue * 6.0;
		if (h >= 6.0)
			h -= 6.0;
		s = Saturation;
		v = Value;
		f = h - (int)h;
		p = v * (1.0 - s);
		q = v * (1.0 - s * f);
		t = v * (1.0 - s * (1.0 - f));
		switch ((int)h) {
		case 0: r = v; g = t; b = p; break;
		case 1: r = q; g = v; b = p; break;
		case 2: r = p; g = v; b = t; break;
		case 3: r = p; g = q; b = v; break;
		case 4: r = t; g = p; b = v; break;
		case 5: r = v; g = p; b = q; break;
		}
	}
	return RGB((BYTE)(r * 255.0 + 0.5), (BYTE)(g * 255.0 + 0.5), (BYTE)(b * 255.0 + 0.5));
}


void RGBToHSV(
	BYTE Red, BYTE Green, BYTE Blue,
	double *pHue, double *pSaturation, double *pValue)
{
	const double r = (double)Red / 255.0, g = (double)Green / 255.0, b = (double)Blue / 255.0;
	double h, s, v;
	double Max, Min;

	if (r > g) {
		Max = std::max(r, b);
		Min = std::min(g, b);
	} else {
		Max = std::max(g, b);
		Min = std::min(r, b);
	}
	v = Max;
	if (Max > Min) {
		s = (Max - Min) / Max;
		double Delta = Max - Min;
		if (r == Max)
			h = (g - b) / Delta;
		else if (g == Max)
			h = 2.0 + (b - r) / Delta;
		else
			h = 4.0 + (r - g) / Delta;
		h /= 6.0;
		if (h < 0.0)
			h += 1.0;
		else if (h >= 1.0)
			h -= 1.0;
	} else {
		s = 0.0;
		h = 0.0;
	}

	if (pHue != nullptr)
		*pHue = h;
	if (pSaturation != nullptr)
		*pSaturation = s;
	if (pValue != nullptr)
		*pValue = v;
}


FILETIME &operator+=(FILETIME &ft, LONGLONG Offset)
{
	ULARGE_INTEGER Result;

	Result.LowPart = ft.dwLowDateTime;
	Result.HighPart = ft.dwHighDateTime;
	Result.QuadPart += Offset;
	ft.dwLowDateTime = Result.LowPart;
	ft.dwHighDateTime = Result.HighPart;
	return ft;
}


LONGLONG operator-(const FILETIME &ft1, const FILETIME &ft2)
{
	LARGE_INTEGER Time1, Time2;

	Time1.LowPart = ft1.dwLowDateTime;
	Time1.HighPart = ft1.dwHighDateTime;
	Time2.LowPart = ft2.dwLowDateTime;
	Time2.HighPart = ft2.dwHighDateTime;
	return Time1.QuadPart - Time2.QuadPart;
}


int CompareSystemTime(const SYSTEMTIME *pTime1, const SYSTEMTIME *pTime2)
{
#if 0
	FILETIME ft1, ft2;

	SystemTimeToFileTime(pTime1, &ft1);
	SystemTimeToFileTime(pTime2, &ft2);
	return CompareFileTime(&ft1, &ft2);
#else
	DWORD Date1, Date2;

	Date1 = ((DWORD)pTime1->wYear << 16) | ((DWORD)pTime1->wMonth << 8) | pTime1->wDay;
	Date2 = ((DWORD)pTime2->wYear << 16) | ((DWORD)pTime2->wMonth << 8) | pTime2->wDay;
	if (Date1 == Date2) {
		Date1 =
			((DWORD)pTime1->wHour << 24) | ((DWORD)pTime1->wMinute << 16) |
			((DWORD)pTime1->wSecond << 10) | pTime1->wMilliseconds;
		Date2 =
			((DWORD)pTime2->wHour << 24) | ((DWORD)pTime2->wMinute << 16) |
			((DWORD)pTime2->wSecond << 10) | pTime2->wMilliseconds;
	}
	if (Date1 < Date2)
		return -1;
	if (Date1 > Date2)
		return 1;
	return 0;
#endif
}


bool OffsetSystemTime(SYSTEMTIME *pTime, LONGLONG Offset)
{
	FILETIME ft;

	if (!::SystemTimeToFileTime(pTime, &ft))
		return false;
	ft += Offset * FILETIME_MILLISECOND;
	return ::FileTimeToSystemTime(&ft, pTime) != FALSE;
}


LONGLONG DiffSystemTime(const SYSTEMTIME *pStartTime, const SYSTEMTIME *pEndTime)
{
	FILETIME ftStart, ftEnd;

	::SystemTimeToFileTime(pStartTime, &ftStart);
	::SystemTimeToFileTime(pEndTime, &ftEnd);
	return (ftEnd - ftStart) / FILETIME_MILLISECOND;
}


void GetLocalTimeAsFileTime(FILETIME *pTime)
{
	SYSTEMTIME st;

	GetLocalTime(&st);
	SystemTimeToFileTime(&st, pTime);
}


void SystemTimeTruncateDay(SYSTEMTIME *pTime)
{
	pTime->wHour = 0;
	pTime->wMinute = 0;
	pTime->wSecond = 0;
	pTime->wMilliseconds = 0;
}


void SystemTimeTruncateHour(SYSTEMTIME *pTime)
{
	pTime->wMinute = 0;
	pTime->wSecond = 0;
	pTime->wMilliseconds = 0;
}


void SystemTimeTruncateMinuite(SYSTEMTIME *pTime)
{
	pTime->wSecond = 0;
	pTime->wMilliseconds = 0;
}


void SystemTimeTruncateSecond(SYSTEMTIME *pTime)
{
	pTime->wMilliseconds = 0;
}


#include <pshpack1.h>
struct REG_TZI_FORMAT
{
	LONG Bias;
	LONG StandardBias;
	LONG DaylightBias;
	SYSTEMTIME StandardDate;
	SYSTEMTIME DaylightDate;
};
#include <poppack.h>

bool GetJSTTimeZoneInformation(TIME_ZONE_INFORMATION *pInfo)
{
	if (pInfo == nullptr)
		return false;

	*pInfo = TIME_ZONE_INFORMATION();

	bool fOK = false;
	HKEY hkey;

	if (::RegOpenKeyEx(
				HKEY_LOCAL_MACHINE,
				TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\Tokyo Standard Time"),
				0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS) {
		REG_TZI_FORMAT TimeZoneInfo;
		DWORD Type, Size = sizeof(REG_TZI_FORMAT);

		if (::RegQueryValueEx(
					hkey, TEXT("TZI"), nullptr, &Type,
					reinterpret_cast<BYTE*>(&TimeZoneInfo), &Size) == ERROR_SUCCESS
				&& Type == REG_BINARY
				&& Size == sizeof(REG_TZI_FORMAT)) {
			pInfo->Bias = TimeZoneInfo.Bias;
			pInfo->StandardDate = TimeZoneInfo.StandardDate;
			pInfo->StandardBias = TimeZoneInfo.StandardBias;
			pInfo->DaylightDate = TimeZoneInfo.DaylightDate;
			pInfo->DaylightBias = TimeZoneInfo.DaylightBias;
			fOK = true;
		}

		::RegCloseKey(hkey);
	}

	if (!fOK) {
		pInfo->Bias = -9 * 60;
		pInfo->DaylightBias = -60;
	}

	return true;
}


int CalcDayOfWeek(int Year, int Month, int Day)
{
	if (Month <= 2) {
		Year--;
		Month += 12;
	}
	return (Year * 365 + Year / 4 - Year / 100 + Year / 400 + 306 * (Month + 1) / 10 + Day - 428) % 7;
}


LPCTSTR GetDayOfWeekText(int DayOfWeek)
{
	if (DayOfWeek < 0 || DayOfWeek > 6)
		return TEXT("？");
	return TEXT("日\0月\0火\0水\0木\0金\0土") + DayOfWeek * ((3 - sizeof(TCHAR)) + 1);
}


bool CopyTextToClipboard(HWND hwndOwner, LPCTSTR pszText)
{
	if (pszText == nullptr)
		return false;

	bool fOK = false;
	SIZE_T Size = (::lstrlen(pszText) + 1) * sizeof(TCHAR);
	HGLOBAL hData = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, Size);
	if (hData != nullptr) {
		LPTSTR pBuffer = static_cast<LPTSTR>(::GlobalLock(hData));
		if (pBuffer != nullptr) {
			::CopyMemory(pBuffer, pszText, Size);
			::GlobalUnlock(hData);

			if (::OpenClipboard(hwndOwner)) {
				::EmptyClipboard();
				if (::SetClipboardData(
#ifdef UNICODE
							CF_UNICODETEXT,
#else
							CF_TEXT,
#endif
							hData) != nullptr)
					fOK = true;
				::CloseClipboard();
			}
		}
		if (!fOK)
			::GlobalFree(hData);
	}

	return fOK;
}


void ClearMenu(HMENU hmenu)
{
	int Count, i;

	Count = GetMenuItemCount(hmenu);
	for (i = Count - 1; i >= 0; i--)
		DeleteMenu(hmenu, i, MF_BYPOSITION);
}


int CopyToMenuText(LPCTSTR pszSrcText, LPTSTR pszDstText, int MaxLength)
{
	int SrcPos, DstPos;

	SrcPos = DstPos = 0;
	while (pszSrcText[SrcPos] != _T('\0') && DstPos + 1 < MaxLength) {
		if (pszSrcText[SrcPos] == _T('&')) {
			if (DstPos + 2 >= MaxLength)
				break;
			pszDstText[DstPos++] = _T('&');
			pszDstText[DstPos++] = _T('&');
			SrcPos++;
		} else {
			int Length = StringCharLength(&pszSrcText[SrcPos]);
			if (Length == 0 || DstPos + Length >= MaxLength)
				break;
			for (int i = 0; i < Length; i++)
				pszDstText[DstPos++] = pszSrcText[SrcPos++];
		}
	}
	pszDstText[DstPos] = _T('\0');
	return DstPos;
}


String FormatMenuString(const String &Str)
{
	String Temp(Str);
	StringUtility::Replace(Temp, L"&", L"&&");
	return Temp;
}


String FormatMenuString(LPCWSTR pszText)
{
	String Temp(pszText);
	StringUtility::Replace(Temp, L"&", L"&&");
	return Temp;
}


void InitOpenFileName(OPENFILENAME *pofn)
{
	pofn->lStructSize = sizeof(OPENFILENAME);
	pofn->hwndOwner = nullptr;
	pofn->hInstance = nullptr;
	pofn->lpstrCustomFilter = nullptr;
	pofn->nMaxCustFilter = 0;
	pofn->nFilterIndex = 1;
	pofn->lpstrFileTitle = nullptr;
	pofn->lpstrInitialDir = nullptr;
	pofn->lpstrTitle = nullptr;
	pofn->Flags = 0;
	pofn->lpstrDefExt = nullptr;
	pofn->pvReserved = nullptr;
	pofn->dwReserved = 0;
	pofn->FlagsEx = 0;
}


bool FileOpenDialog(OPENFILENAME *pofn)
{
	CommonDialogDPIBlock SystemDPI;

	return ::GetOpenFileName(pofn) != FALSE;
}


bool FileSaveDialog(OPENFILENAME *pofn)
{
	CommonDialogDPIBlock SystemDPI;

	return ::GetSaveFileName(pofn) != FALSE;
}


void ForegroundWindow(HWND hwnd)
{
	int TargetID, ForegroundID;

	ForegroundID = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
	TargetID = GetWindowThreadProcessId(hwnd, nullptr);
	AttachThreadInput(TargetID, ForegroundID, TRUE);
	SetForegroundWindow(hwnd);
	AttachThreadInput(TargetID, ForegroundID, FALSE);
}


bool ChooseColorDialog(HWND hwndOwner, COLORREF *pcrResult)
{
	CHOOSECOLOR cc;
	static COLORREF crCustomColors[16] = {
		RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),
		RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),
		RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),
		RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),
		RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),
		RGB(0xFF, 0xFF, 0xFF)
	};

	cc.lStructSize = sizeof(CHOOSECOLOR);
	cc.hwndOwner = hwndOwner;
	cc.hInstance = nullptr;
	cc.rgbResult = *pcrResult;
	cc.lpCustColors = crCustomColors;
	cc.Flags = CC_RGBINIT | CC_FULLOPEN;

	{
		CommonDialogDPIBlock SystemDPI;

		if (!ChooseColor(&cc))
			return false;
	}

	*pcrResult = cc.rgbResult;
	return true;
}


bool ChooseFontDialog(HWND hwndOwner, LOGFONT *plf, int *pPointSize)
{
	CHOOSEFONT cf;

	cf.lStructSize = sizeof(CHOOSEFONT);
	cf.hwndOwner = hwndOwner;
	cf.lpLogFont = plf;
	cf.Flags = CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;

	{
		CommonDialogDPIBlock SystemDPI;

		if (!ChooseFont(&cf))
			return false;
	}

	if (pPointSize != nullptr)
		*pPointSize = cf.iPointSize;

	return true;
}


int CALLBACK BrowseFolderCallback(HWND hwnd, UINT uMsg, LPARAM lpData, LPARAM lParam)
{
	switch (uMsg) {
	case BFFM_INITIALIZED:
		if (((LPTSTR)lParam)[0] != _T('\0')) {
			TCHAR szDirectory[MAX_PATH];

			StringCopy(szDirectory, (LPTSTR)lParam);
			PathRemoveBackslash(szDirectory);
			SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)szDirectory);
		}
		break;
	}
	return 0;
}


bool BrowseFolderDialog(HWND hwndOwner, LPTSTR pszDirectory, LPCTSTR pszTitle)
{
	BROWSEINFO bi;
	PIDLIST_ABSOLUTE pidl;
	BOOL fRet;

	bi.hwndOwner = hwndOwner;
	bi.pidlRoot = nullptr;
	bi.pszDisplayName = pszDirectory;
	bi.lpszTitle = pszTitle;
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	bi.lpfn = BrowseFolderCallback;
	bi.lParam = (LPARAM)pszDirectory;

	{
		CommonDialogDPIBlock SystemDPI;

		pidl = SHBrowseForFolder(&bi);
		if (pidl == nullptr)
			return false;
	}

	fRet = SHGetPathFromIDList(pidl, pszDirectory);
	CoTaskMemFree(pidl);
	return fRet == TRUE;
}


bool CompareLogFont(const LOGFONT *pFont1, const LOGFONT *pFont2)
{
	return memcmp(pFont1, pFont2, 28/*offsetof(LOGFONT, lfFaceName)*/) == 0
		&& lstrcmp(pFont1->lfFaceName, pFont2->lfFaceName) == 0;
}


int PixelsToPoints(int Pixels)
{
	HDC hIC = ::CreateIC(TEXT("DISPLAY"), nullptr, nullptr, nullptr);
	if (hIC == nullptr)
		return 0;
	int Resolution = ::GetDeviceCaps(hIC, LOGPIXELSY);
	int Points;
	if (Resolution != 0)
		Points = ::MulDiv(Pixels, 72, Resolution);
	else
		Points = 0;
	::DeleteDC(hIC);
	return Points;
}


int PointsToPixels(int Points)
{
	HDC hIC = ::CreateIC(TEXT("DISPLAY"), nullptr, nullptr, nullptr);
	if (hIC == nullptr)
		return 0;
	int Resolution = ::GetDeviceCaps(hIC, LOGPIXELSY);
	int Pixels;
	if (Resolution != 0)
		Pixels = ::MulDiv(Points, Resolution, 72);
	else
		Pixels = 0;
	::DeleteDC(hIC);
	return Pixels;
}


int CalcFontPointHeight(HDC hdc, const LOGFONT *pFont)
{
	if (hdc == nullptr || pFont == nullptr)
		return 0;

	HFONT hfont = CreateFontIndirect(pFont), hfontOld;
	if (hfont == nullptr)
		return 0;

	TEXTMETRIC tm;
	int PixelsPerInch;

	hfontOld = static_cast<HFONT>(SelectObject(hdc, hfont));
	GetTextMetrics(hdc, &tm);
	PixelsPerInch = GetDeviceCaps(hdc, LOGPIXELSY);
	SelectObject(hdc, hfontOld);
	DeleteObject(hfont);
	if (PixelsPerInch == 0)
		return 0;
	return MulDiv(tm.tmHeight - tm.tmInternalLeading, 72, PixelsPerInch);
}


int GetErrorText(DWORD ErrorCode, LPTSTR pszText, int MaxLength)
{
	if (pszText == nullptr || MaxLength < 1)
		return 0;

	int Length = ::FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
		ErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		pszText, MaxLength, nullptr);
	if (Length == 0)
		pszText[0] = _T('\0');
	return Length;
}


bool IsEqualFileName(LPCWSTR pszFileName1, LPCWSTR pszFileName2)
{
	return ::CompareStringOrdinal(pszFileName1, -1, pszFileName2, -1, TRUE) == CSTR_EQUAL;
}


bool IsValidFileName(LPCTSTR pszFileName, FileNameValidateFlag Flags, String *pMessage)
{
	if (pszFileName == nullptr || pszFileName[0] == _T('\0')) {
		if (pMessage != nullptr)
			*pMessage = TEXT("ファイル名が指定されていません。");
		return false;
	}
	int Length = lstrlen(pszFileName);
	if (Length >= MAX_PATH) {
		if (pMessage != nullptr)
			*pMessage = TEXT("ファイル名が長すぎます。");
		return false;
	}
	if (Length == 3) {
		static const LPCTSTR pszNGList[] = {
			TEXT("CON"), TEXT("PRN"), TEXT("AUX"), TEXT("NUL")
		};
		for (LPCTSTR e : pszNGList) {
			if (lstrcmpi(e, pszFileName) == 0) {
				if (pMessage != nullptr)
					*pMessage = TEXT("仮想デバイス名はファイル名に使用できません。");
				return false;
			}
		}
	} else if (Length == 4) {
		TCHAR szName[5];

		for (int i = 1; i <= 9; i++) {
			StringPrintf(szName, TEXT("COM%d"), i);
			if (lstrcmpi(szName, pszFileName) == 0) {
				if (pMessage != nullptr)
					*pMessage = TEXT("仮想デバイス名はファイル名に使用できません。");
				return false;
			}
		}
		for (int i = 1; i <= 9; i++) {
			StringPrintf(szName, TEXT("LPT%d"), i);
			if (lstrcmpi(szName, pszFileName) == 0) {
				if (pMessage != nullptr)
					*pMessage = TEXT("仮想デバイス名はファイル名に使用できません。");
				return false;
			}
		}
	}

	const bool fWildcard = !!(Flags & FileNameValidateFlag::Wildcard);
	const bool fAllowDelimiter = !!(Flags & FileNameValidateFlag::AllowDelimiter);
	LPCTSTR p = pszFileName;

	while (*p != _T('\0')) {
		if (*p <= 31 || *p == _T('<') || *p == _T('>') || *p == _T(':') || *p == _T('"')
				|| *p == _T('/') || *p == _T('|')
				|| (!fWildcard && (*p == _T('*') || *p == _T('?')))
				|| (!fAllowDelimiter && *p == _T('\\'))) {
			if (pMessage != nullptr) {
				if (*p <= 31) {
					StringUtility::Format(
						*pMessage,
						TEXT("ファイル名に使用できない文字 0x%02x が含まれています。"), *p);
				} else {
					StringUtility::Format(
						*pMessage,
						TEXT("ファイル名に使用できない文字 %c が含まれています。"), *p);
				}
			}
			return false;
		}
		if ((*p == _T(' ') || *p == _T('.')) && *(p + 1) == _T('\0')) {
			if (pMessage != nullptr)
				*pMessage = TEXT("ファイル名の末尾に半角空白及び . は使用できません。");
			return false;
		}
#ifndef UNICODE
		if (IsDBCSLeadByteEx(CP_ACP, *p)) {
			if (*(p + 1) == _T('\0')) {
				if (pMessage != nullptr)
					*pMessage = TEXT("2バイト文字の2バイト目が欠けています。");
				return false;
			}
			p++;
		}
#endif
		p++;
	}

	return true;
}


bool MakeUniqueFileName(String *pFileName, size_t MaxLength, LPCTSTR pszNumberFormat)
{
	if (pFileName == nullptr || pFileName->empty())
		return false;

	LPCTSTR pszFileName = pFileName->c_str();
	size_t DirLength = ::PathFindFileName(pszFileName) - pszFileName;
	if (DirLength == 0 || DirLength > MaxLength - 1)
		return false;

	String Path(*pFileName);

	LPCTSTR pszExtension = ::PathFindExtension(pszFileName);
	size_t ExtensionLength = ::lstrlen(pszExtension);
	size_t MaxFileName = MaxLength - DirLength;
	if (Path.length() > MaxLength) {
		if (ExtensionLength < MaxFileName) {
			Path.resize(MaxLength - ExtensionLength);
			Path += pszExtension;
		} else {
			Path.resize(MaxLength);
		}
	}

	if (::PathFileExists(Path.c_str())) {
		static const int MAX_NUMBER = 1000;
		String BaseName(
			pFileName->substr(DirLength, pFileName->length() - DirLength - ExtensionLength));
		String Name;

		if (IsStringEmpty(pszNumberFormat))
			pszNumberFormat = TEXT("-%d");

		for (int i = 2;; i++) {
			if (i == MAX_NUMBER)
				return false;

			TCHAR szNumber[16];

			StringPrintf(szNumber, pszNumberFormat, i);
			Name = BaseName;
			Name += szNumber;
			Name += pszExtension;
			Path = pFileName->substr(0, DirLength);
			if (Name.length() <= MaxFileName)
				Path += Name;
			else
				Path += Name.substr(Name.length() - MaxFileName);
			if (!::PathFileExists(Path.c_str()))
				break;
		}
	}

	*pFileName = Path;

	return true;
}


bool GetAbsolutePath(LPCTSTR pszFilePath, LPTSTR pszAbsolutePath, int MaxLength)
{
	if (pszAbsolutePath == nullptr || MaxLength < 1)
		return false;
	pszAbsolutePath[0] = _T('\0');
	if (pszFilePath == nullptr || pszFilePath[0] == _T('\0'))
		return false;
	if (::PathIsRelative(pszFilePath)) {
		TCHAR szTemp[MAX_PATH], *p;

		::GetModuleFileName(nullptr, szTemp, _countof(szTemp));
		p = ::PathFindFileName(szTemp);
		if ((p - szTemp) + ::lstrlen(pszFilePath) >= MaxLength)
			return false;
		StringCopy(p, pszFilePath);
		::PathCanonicalize(pszAbsolutePath, szTemp);
	} else {
		if (::lstrlen(pszFilePath) >= MaxLength)
			return false;
		StringCopy(pszAbsolutePath, pszFilePath);
	}
	return true;
}


bool GetAbsolutePath(const String &FilePath, String *pAbsolutePath)
{
	if (pAbsolutePath == nullptr)
		return false;

	pAbsolutePath->clear();

	if (FilePath.empty())
		return false;

	if (PathUtil::IsRelative(FilePath)) {
		TCHAR szDir[MAX_PATH];
		::GetModuleFileName(nullptr, szDir, _countof(szDir));
		::PathRemoveFileSpec(szDir);

		if (!PathUtil::RelativeToAbsolute(pAbsolutePath, String(szDir), FilePath))
			return false;
	} else {
		pAbsolutePath->assign(FilePath);
	}

	return true;
}


static HBITMAP CreateIconMaskBitmap(
	int IconWidth, int IconHeight, int ImageWidth, int ImageHeight)
{
	SIZE_T BytesPerLine, BitsBytes;
	BYTE *pBits;
	int Top;
	HBITMAP hbm;

	BytesPerLine = (IconWidth + 15) / 16 * 2;
	BitsBytes = BytesPerLine * IconHeight;
	pBits = new BYTE[BitsBytes];
	::FillMemory(pBits, BitsBytes, 0xFF);
	Top = (IconHeight - ImageHeight) / 2;
	if (ImageWidth == IconWidth) {
		::ZeroMemory(pBits + Top * BytesPerLine, ImageHeight * BytesPerLine);
	} else {
		int Left, x, y;
		BYTE *p;

		Left = (IconWidth - ImageWidth) / 2;
		p = pBits + Top * BytesPerLine;
		for (y = 0; y < ImageHeight; y++) {
			for (x = Left; x < Left + ImageWidth; x++)
				//p[x / 8] &= ~(0x80 >> (x % 8));
				p[x >> 3] &= ~(0x80 >> (x & 7));
			p += BytesPerLine;
		}
	}
	hbm = ::CreateBitmap(IconWidth, IconHeight, 1, 1, pBits);
	delete [] pBits;
	return hbm;
}

static HBITMAP CreateIconColorBitmap(
	HBITMAP hbm, int IconWidth, int IconHeight, int ImageWidth, int ImageHeight)
{
	HDC hdc;
	HBITMAP hbmIcon;

	hdc = ::GetDC(nullptr);
	hbmIcon = ::CreateCompatibleBitmap(hdc, IconWidth, IconHeight);
	if (hbmIcon != nullptr) {
		BITMAP bm;
		::GetObject(hbm, sizeof(bm), &bm);
		HDC hdcSrc = ::CreateCompatibleDC(hdc);
		HBITMAP hbmSrcOld = static_cast<HBITMAP>(::SelectObject(hdcSrc, hbm));
		HDC hdcDest = ::CreateCompatibleDC(hdc);
		HBITMAP hbmDestOld = static_cast<HBITMAP>(::SelectObject(hdcDest, hbmIcon));

		if (ImageWidth < IconWidth || ImageHeight < IconHeight) {
			RECT rc = {0, 0, IconWidth, IconHeight};

			::FillRect(hdcDest, &rc, static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
		}
		int OldStretchMode = ::SetStretchBltMode(hdcDest, STRETCH_HALFTONE);
		::StretchBlt(
			hdcDest,
			(IconWidth - ImageWidth) / 2, (IconHeight - ImageHeight) / 2,
			ImageWidth, ImageHeight,
			hdcSrc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
		::SetStretchBltMode(hdcDest, OldStretchMode);
		::SelectObject(hdcDest, hbmDestOld);
		::DeleteDC(hdcDest);
		::SelectObject(hdcSrc, hbmSrcOld);
		::DeleteDC(hdcSrc);
	}
	::ReleaseDC(nullptr, hdc);
	return hbmIcon;
}

HICON CreateIconFromBitmap(HBITMAP hbm, int IconWidth, int IconHeight, int ImageWidth, int ImageHeight)
{
	if (hbm == nullptr || IconWidth <= 0 || IconHeight <= 0
			|| ImageWidth < 0 || ImageWidth > IconWidth
			|| ImageHeight < 0 || ImageHeight > IconHeight)
		return nullptr;

	if (ImageWidth == 0 || ImageHeight == 0) {
		BITMAP bm;

		if (::GetObject(hbm, sizeof(bm), &bm) != sizeof(bm))
			return nullptr;
		if (bm.bmWidth <= IconWidth && bm.bmHeight <= IconHeight) {
			ImageWidth = bm.bmWidth;
			ImageHeight = bm.bmHeight;
		} else {
			ImageWidth = std::min(bm.bmWidth * IconHeight / bm.bmHeight, (long)IconWidth);
			if (ImageWidth < 1)
				ImageWidth = 1;
			ImageHeight = std::min(bm.bmHeight * IconWidth / bm.bmWidth, (long)IconHeight);
			if (ImageHeight < 1)
				ImageHeight = 1;
		}
	}

	ICONINFO ii;
	ii.hbmMask = CreateIconMaskBitmap(IconWidth, IconHeight, ImageWidth, ImageHeight);
	if (ii.hbmMask == nullptr)
		return nullptr;
	ii.hbmColor = CreateIconColorBitmap(hbm, IconWidth, IconHeight, ImageWidth, ImageHeight);
	if (ii.hbmColor == nullptr) {
		::DeleteObject(ii.hbmMask);
		return nullptr;
	}
	ii.fIcon = TRUE;
	ii.xHotspot = 0;
	ii.yHotspot = 0;
	HICON hico = ::CreateIconIndirect(&ii);
	::DeleteObject(ii.hbmMask);
	::DeleteObject(ii.hbmColor);
	return hico;
}


#include <pshpack1.h>

struct ICONDIRENTRY
{
	BYTE bWidth;
	BYTE bHeight;
	BYTE bColorCount;
	BYTE bReserved;
	WORD wPlanes;
	WORD wBitCount;
	DWORD dwBytesInRes;
	DWORD dwImageOffset;
};

struct ICONDIR
{
	WORD idReserved;
	WORD idType;
	WORD idCount;
	ICONDIRENTRY idEntries[1];
};

#include <poppack.h>

/*
	アイコンをファイルに保存する
	保存されるアイコンは24ビット固定
*/
bool SaveIconFromBitmap(
	LPCTSTR pszFileName, HBITMAP hbm,
	int IconWidth, int IconHeight, int ImageWidth, int ImageHeight)
{
	if (IsStringEmpty(pszFileName) || hbm == nullptr
			|| IconWidth <= 0 || IconHeight <= 0
			|| ImageWidth < 0 || ImageWidth > IconWidth
			|| ImageHeight < 0 || ImageHeight > IconHeight)
		return false;

	BITMAP bm;
	if (::GetObject(hbm, sizeof(bm), &bm) != sizeof(bm))
		return false;

	if (ImageWidth == 0 || ImageHeight == 0) {
		if (bm.bmWidth <= IconWidth && bm.bmHeight <= IconHeight) {
			ImageWidth = bm.bmWidth;
			ImageHeight = bm.bmHeight;
		} else {
			ImageWidth = std::min(bm.bmWidth * IconHeight / bm.bmHeight, (long)IconWidth);
			if (ImageWidth < 1)
				ImageWidth = 1;
			ImageHeight = std::min(bm.bmHeight * IconWidth / bm.bmWidth, (long)IconHeight);
			if (ImageHeight < 1)
				ImageHeight = 1;
		}
	}

	int BitCount = 24;
	DWORD PixelRowBytes = (IconWidth * BitCount + 31) / 32 * 4;
	DWORD PixelBytes = PixelRowBytes * IconHeight;
	DWORD MaskRowBytes = (IconWidth + 31) / 32 * 4;
	DWORD MaskBytes = MaskRowBytes * IconHeight;

	ICONDIR id;
	id.idReserved = 0;
	id.idType = 1;
	id.idCount = 1;
	id.idEntries[0].bWidth = IconWidth;
	id.idEntries[0].bHeight = IconHeight;
	id.idEntries[0].bColorCount = 0;
	id.idEntries[0].bReserved = 0;
	id.idEntries[0].wPlanes = 1;
	id.idEntries[0].wBitCount = BitCount;
	id.idEntries[0].dwBytesInRes = sizeof(BITMAPINFOHEADER) + PixelBytes + MaskBytes;
	id.idEntries[0].dwImageOffset = sizeof(ICONDIR);

	BITMAPINFOHEADER bmih;
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biWidth = IconWidth;
	bmih.biHeight = IconHeight;
	bmih.biPlanes = 1;
	bmih.biBitCount = BitCount;
	bmih.biCompression = BI_RGB;
	bmih.biSizeImage = 0;
	bmih.biXPelsPerMeter = 0;
	bmih.biYPelsPerMeter = 0;
	bmih.biClrUsed = 0;
	bmih.biClrImportant = 0;

	bool fOK = false;
	void *pColorBits;
	HBITMAP hbmColor = ::CreateDIBSection(
		nullptr, reinterpret_cast<BITMAPINFO*>(&bmih), DIB_RGB_COLORS, &pColorBits, nullptr, 0);
	if (hbmColor != nullptr) {
		HDC hdcSrc = ::CreateCompatibleDC(nullptr);
		HBITMAP hbmSrcOld = static_cast<HBITMAP>(::SelectObject(hdcSrc, hbm));
		HDC hdcDst = ::CreateCompatibleDC(nullptr);
		HBITMAP hbmDstOld = static_cast<HBITMAP>(::SelectObject(hdcDst, hbmColor));

		if (ImageWidth < IconWidth || ImageHeight < IconHeight) {
			RECT rc = {0, 0, IconWidth, IconHeight};
			::FillRect(hdcDst, &rc, static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
		}
		int OldStretchMode = ::SetStretchBltMode(hdcDst, STRETCH_HALFTONE);
		::StretchBlt(
			hdcDst,
			(IconWidth - ImageWidth) / 2, (IconHeight - ImageHeight) / 2,
			ImageWidth, ImageHeight,
			hdcSrc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
		::SetStretchBltMode(hdcDst, OldStretchMode);
		::SelectObject(hdcDst, hbmDstOld);
		::SelectObject(hdcSrc, hbmSrcOld);

		HBITMAP hbmMask = CreateIconMaskBitmap(IconWidth, IconHeight, ImageWidth, ImageHeight);
		if (hbmMask != nullptr) {
			BYTE MaskInfoBuff[sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 2];
			BITMAPINFO *pbmiMask = reinterpret_cast<BITMAPINFO*>(MaskInfoBuff);
			::CopyMemory(pbmiMask, &bmih, sizeof(BITMAPINFOHEADER));
			pbmiMask->bmiHeader.biBitCount = 1;
			static const RGBQUAD Palette[2] = {{0, 0, 0, 0}, {255, 255, 255, 0}};
			pbmiMask->bmiColors[0] = Palette[0];
			pbmiMask->bmiColors[1] = Palette[1];
			BYTE *pMaskBits = new BYTE[MaskBytes];
			::GetDIBits(hdcSrc, hbmMask, 0, IconHeight, pMaskBits, pbmiMask, DIB_RGB_COLORS);

			HANDLE hFile = ::CreateFile(
				pszFileName, GENERIC_WRITE, 0, nullptr,
				CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hFile != INVALID_HANDLE_VALUE) {
				DWORD Write;

				bmih.biHeight *= 2;
				if (::WriteFile(hFile, &id, sizeof(ICONDIR), &Write, nullptr)
						&& Write == sizeof(ICONDIR)
						&& ::WriteFile(hFile, &bmih, sizeof(BITMAPINFOHEADER), &Write, nullptr)
						&& Write == sizeof(BITMAPINFOHEADER)
						&& ::WriteFile(hFile, pColorBits, PixelBytes, &Write, nullptr)
						&& Write == PixelBytes
						&& ::WriteFile(hFile, pMaskBits, MaskBytes, &Write, nullptr)
						&& Write == MaskBytes)
					fOK = true;
				::CloseHandle(hFile);
			}

			delete [] pMaskBits;
			::DeleteObject(hbmMask);
		}

		::DeleteDC(hdcDst);
		::DeleteDC(hdcSrc);
		::DeleteObject(hbmColor);
	}

	return fOK;
}


HICON CreateEmptyIcon(int Width, int Height, int BitsPerPixel)
{
	if (Width <= 0 || Height <= 0)
		return nullptr;

	ICONINFO ii = {TRUE, 0, 0, nullptr, nullptr};
	HICON hicon = nullptr;

	const int Planes = BitsPerPixel == 1 ? 2 : 1;
	const size_t Size = (Width + 15) / 16 * 2 * Height;
	BYTE *pMaskBits = new BYTE[Size * Planes];
	::FillMemory(pMaskBits, Size, 0xFF);
	if (BitsPerPixel == 1)
		::FillMemory(pMaskBits + Size, Size, 0x00);
	ii.hbmMask = ::CreateBitmap(Width, Height * Planes, 1, 1, pMaskBits);
	delete [] pMaskBits;

	if (BitsPerPixel != 1) {
		const DWORD HeaderSize = BitsPerPixel == 32 ? sizeof(BITMAPV5HEADER) : sizeof(BITMAPINFOHEADER);
		size_t PaletteSize = 0;
		if (BitsPerPixel <= 8)
			PaletteSize = ((size_t)1 << BitsPerPixel) * sizeof(RGBQUAD);
		BITMAPINFO *pbmi = (BITMAPINFO*)std::malloc(HeaderSize + PaletteSize);
		if (pbmi != nullptr) {
			::ZeroMemory(pbmi, HeaderSize + PaletteSize);
			pbmi->bmiHeader.biSize = HeaderSize;
			pbmi->bmiHeader.biWidth = Width;
			pbmi->bmiHeader.biHeight = Height;
			pbmi->bmiHeader.biPlanes = 1;
			pbmi->bmiHeader.biBitCount = BitsPerPixel;
			if (BitsPerPixel == 32) {
				BITMAPV5HEADER *pbV5 = (BITMAPV5HEADER*)&pbmi->bmiHeader;
				pbV5->bV5Compression = BI_BITFIELDS;
				pbV5->bV5RedMask  = 0x00FF0000;
				pbV5->bV5GreenMask = 0x0000FF00;
				pbV5->bV5BlueMask = 0x000000FF;
				pbV5->bV5AlphaMask = 0xFF000000;
			}
			void *pBits;
			ii.hbmColor = ::CreateDIBSection(nullptr, pbmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
			std::free(pbmi);
			if (ii.hbmColor != nullptr)
				::ZeroMemory(pBits, (Width * BitsPerPixel + 31) / 32 * 4 * Height);
		}
	}

	hicon = ::CreateIconIndirect(&ii);

	if (ii.hbmMask != nullptr)
		::DeleteObject(ii.hbmMask);
	if (ii.hbmColor != nullptr)
		::DeleteObject(ii.hbmColor);

	return hicon;
}


bool GetStandardIconSize(IconSizeType Size, int *pWidth, int *pHeight)
{
	int Width, Height;

	switch (Size) {
	case IconSizeType::Small:
		Width = ::GetSystemMetrics(SM_CXSMICON);
		Height = ::GetSystemMetrics(SM_CYSMICON);
		break;

	case IconSizeType::Normal:
		Width = ::GetSystemMetrics(SM_CXICON);
		Height = ::GetSystemMetrics(SM_CYICON);
		break;

	default:
		return false;
	}

	if (pWidth != nullptr)
		*pWidth = Width;
	if (pHeight != nullptr)
		*pHeight = Height;

	return true;
}


HICON LoadIconStandardSize(HINSTANCE hinst, LPCTSTR pszName, IconSizeType Size)
{
	int Metric;
	HICON hico;

	switch (Size) {
	case IconSizeType::Small:  Metric = LIM_SMALL; break;
	case IconSizeType::Normal: Metric = LIM_LARGE; break;
	default:
		return nullptr;
	}

	if (SUCCEEDED(::LoadIconMetric(hinst, pszName, Metric, &hico)))
		return hico;

	int Width, Height;

	GetStandardIconSize(Size, &Width, &Height);

	return (HICON)::LoadImage(hinst, pszName, IMAGE_ICON, Width, Height, LR_DEFAULTCOLOR);
}


HICON LoadIconSpecificSize(HINSTANCE hinst, LPCTSTR pszName, int Width, int Height)
{
	if (Width <= 0 || Height <= 0)
		return nullptr;

	HICON hico;

	if (SUCCEEDED(::LoadIconWithScaleDown(hinst, pszName, Width, Height, &hico)))
		return hico;

	return (HICON)::LoadImage(hinst, pszName, IMAGE_ICON, Width, Height, LR_DEFAULTCOLOR);
}


HICON LoadSystemIcon(LPCTSTR pszName, IconSizeType Size)
{
	int Metric;
	HICON hico;

	switch (Size) {
	case IconSizeType::Small:  Metric = LIM_SMALL; break;
	case IconSizeType::Normal: Metric = LIM_LARGE; break;
	default:
		return nullptr;
	}

	if (SUCCEEDED(::LoadIconMetric(nullptr, pszName, Metric, &hico)))
		return hico;

	int Width, Height;

	GetStandardIconSize(Size, &Width, &Height);

	hico = ::LoadIcon(nullptr, pszName);
	if (hico != nullptr)
		hico = (HICON)::CopyImage(hico, IMAGE_ICON, Width, Height, 0);
	return hico;
}


HICON LoadSystemIcon(LPCTSTR pszName, int Width, int Height)
{
	if (Width <= 0 || Height <= 0)
		return nullptr;

	HICON hico;

	if (SUCCEEDED(::LoadIconWithScaleDown(nullptr, pszName, Width, Height, &hico)))
		return hico;

	if (Width == ::GetSystemMetrics(SM_CXICON) && Height == ::GetSystemMetrics(SM_CYICON))
		return LoadSystemIcon(pszName, IconSizeType::Normal);
	if (Width == ::GetSystemMetrics(SM_CXSMICON) && Height == ::GetSystemMetrics(SM_CYSMICON))
		return LoadSystemIcon(pszName, IconSizeType::Small);

	hico = ::LoadIcon(nullptr, pszName);
	if (hico != nullptr)
		hico = (HICON)::CopyImage(hico, IMAGE_ICON, Width, Height, 0);
	return hico;
}




CStaticStringFormatter::CStaticStringFormatter(LPTSTR pBuffer, size_t BufferLength)
	: m_pBuffer(pBuffer)
	, m_BufferLength(BufferLength)
	, m_Length(0)
{
	m_pBuffer[0] = _T('\0');
}

void CStaticStringFormatter::Clear()
{
	m_Length = 0;
}

void CStaticStringFormatter::Append(LPCTSTR pszString)
{
	if (pszString != nullptr && m_Length + 1 < m_BufferLength) {
		size_t Length = StringLength(pszString, m_BufferLength - m_Length - 1);
		StringCopy(m_pBuffer + m_Length, pszString, Length + 1);
		m_Length += Length;
	}
}

void CStaticStringFormatter::AppendFormat(LPCTSTR pszFormat, ...)
{
	if (pszFormat != nullptr && m_Length + 1 < m_BufferLength) {
		va_list Args;

		va_start(Args, pszFormat);
		AppendFormatV(pszFormat, Args);
		va_end(Args);
	}
}

void CStaticStringFormatter::AppendFormatV(LPCTSTR pszFormat, va_list Args)
{
	if (pszFormat != nullptr && m_Length + 1 < m_BufferLength) {
		int Length = StringPrintfV(m_pBuffer + m_Length, m_BufferLength - m_Length, pszFormat, Args);
		if (Length >= 0)
			m_Length += Length;
	}
}

void CStaticStringFormatter::RemoveTrailingWhitespace()
{
	if (m_Length > 0) {
		m_Length -= TVTest::RemoveTrailingWhitespace(m_pBuffer);
	}
}




CGlobalLock::CGlobalLock()
	: m_hMutex(nullptr)
	, m_fOwner(false)
{
}

CGlobalLock::~CGlobalLock()
{
	Close();
}

bool CGlobalLock::Create(LPCTSTR pszName, bool fInheritHandle)
{
	if (m_hMutex != nullptr)
		return false;

	CBasicSecurityAttributes SecAttributes;

	if (!SecAttributes.Initialize())
		return false;

	m_hMutex = ::CreateMutex(&SecAttributes, fInheritHandle, pszName);
	m_fOwner = false;

	return m_hMutex != nullptr;
}

bool CGlobalLock::Open(LPCTSTR pszName, DWORD DesiredAccess, bool fInheritHandle)
{
	if (m_hMutex != nullptr)
		return false;

	m_hMutex = ::OpenMutex(DesiredAccess, fInheritHandle, pszName);
	m_fOwner = false;

	return m_hMutex != nullptr;
}

bool CGlobalLock::Wait(DWORD Timeout)
{
	if (m_hMutex == nullptr)
		return false;
	if (::WaitForSingleObject(m_hMutex, Timeout) == WAIT_TIMEOUT)
		return false;
	m_fOwner = true;
	return true;
}

void CGlobalLock::Close()
{
	if (m_hMutex != nullptr) {
		Release();
		::CloseHandle(m_hMutex);
		m_hMutex = nullptr;
	}
}

void CGlobalLock::Release()
{
	if (m_hMutex != nullptr && m_fOwner) {
		::ReleaseMutex(m_hMutex);
		m_fOwner = false;
	}
}


CBasicSecurityAttributes::CBasicSecurityAttributes()
{
	nLength = sizeof(SECURITY_ATTRIBUTES);
	lpSecurityDescriptor = nullptr;
	bInheritHandle = FALSE;
}

bool CBasicSecurityAttributes::Initialize()
{
	if (!::InitializeSecurityDescriptor(&m_SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION))
		return false;
	if (!::SetSecurityDescriptorDacl(&m_SecurityDescriptor, TRUE, nullptr, FALSE))
		return false;
	lpSecurityDescriptor = &m_SecurityDescriptor;
	return true;
}


namespace Util
{


HMODULE LoadSystemLibrary(LPCTSTR pszName)
{
	TCHAR szPath[MAX_PATH];
	UINT Length = ::GetSystemDirectory(szPath, _countof(szPath));
	if (Length < 1 || Length + 1 + ::lstrlen(pszName) >= _countof(szPath))
		return nullptr;
	::PathAppend(szPath, pszName);
	return ::LoadLibrary(szPath);
}


namespace OS
{


static bool VerifyOSVersion(
	DWORD Major, BYTE MajorOperator,
	DWORD Minor, BYTE MinorOperator)
{
	OSVERSIONINFOEX osvi = {sizeof(osvi)};

	osvi.dwMajorVersion = Major;
	osvi.dwMinorVersion = Minor;

	return ::VerifyVersionInfo(
		&osvi, VER_MAJORVERSION | VER_MINORVERSION,
		::VerSetConditionMask(
			::VerSetConditionMask(0, VER_MAJORVERSION, MajorOperator),
			VER_MINORVERSION, MinorOperator)) != FALSE;
}

static bool VerifyOSVersion(
	DWORD Major, BYTE MajorOperator,
	DWORD Minor, BYTE MinorOperator,
	DWORD Build, BYTE BuildOperator)
{
	OSVERSIONINFOEX osvi = {sizeof(osvi)};

	osvi.dwMajorVersion = Major;
	osvi.dwMinorVersion = Minor;
	osvi.dwBuildNumber = Build;

	ULONGLONG ConditionMask = 0;
	VER_SET_CONDITION(ConditionMask, VER_MAJORVERSION, MajorOperator);
	VER_SET_CONDITION(ConditionMask, VER_MINORVERSION, MinorOperator);
	VER_SET_CONDITION(ConditionMask, VER_BUILDNUMBER, BuildOperator);

	return ::VerifyVersionInfo(
		&osvi,
		VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER,
		ConditionMask) != FALSE;
}

static bool CheckOSVersion(DWORD Major, DWORD Minor)
{
	return VerifyOSVersion(Major, VER_EQUAL, Minor, VER_EQUAL);
}

static bool CheckOSVersion(DWORD Major, DWORD Minor, DWORD Build)
{
	return VerifyOSVersion(Major, VER_EQUAL, Minor, VER_EQUAL, Build, VER_EQUAL);
}

static bool CheckOSVersionLater(DWORD Major, DWORD Minor)
{
	return VerifyOSVersion(Major, VER_GREATER_EQUAL, Minor, VER_GREATER_EQUAL);
}

static bool CheckOSVersionLater(DWORD Major, DWORD Minor, DWORD Build)
{
	return VerifyOSVersion(Major, VER_GREATER_EQUAL, Minor, VER_GREATER_EQUAL, Build, VER_GREATER_EQUAL);
}

bool IsWindowsXP()
{
	return VerifyOSVersion(5, VER_EQUAL, 1, VER_GREATER_EQUAL);
}

bool IsWindowsVista()
{
	return CheckOSVersion(6, 0);
}

bool IsWindows7()
{
	return CheckOSVersion(6, 1);
}

bool IsWindows8()
{
	return CheckOSVersion(6, 2);
}

bool IsWindows8_1()
{
	return CheckOSVersion(6, 3);
}

bool IsWindows10()
{
	return CheckOSVersion(10, 0);
}

bool IsWindowsXPOrLater()
{
	return CheckOSVersionLater(5, 1);
}

bool IsWindowsVistaOrLater()
{
	return CheckOSVersionLater(6, 0);
}

bool IsWindows7OrLater()
{
	return CheckOSVersionLater(6, 1);
}

bool IsWindows8OrLater()
{
	return CheckOSVersionLater(6, 2);
}

bool IsWindows8_1OrLater()
{
	return CheckOSVersionLater(6, 3);
}

bool IsWindows10OrLater()
{
	return CheckOSVersionLater(10, 0);
}

bool IsWindows10AnniversaryUpdateOrLater()
{
	return CheckOSVersionLater(10, 0, 14393);
}

bool IsWindows10CreatorsUpdateOrLater()
{
	return CheckOSVersionLater(10, 0, 15063);
}


}	// namespace OS


CTimer::CTimer()
	: m_hTimer(nullptr)
{
}

CTimer::~CTimer()
{
	End();
}

bool CTimer::Begin(DWORD DueTime, DWORD Period)
{
	End();

	HANDLE hTimer;
	if (!::CreateTimerQueueTimer(&hTimer, nullptr, TimerCallback, this, DueTime, Period, WT_EXECUTEDEFAULT))
		return false;

	m_hTimer = hTimer;

	return true;
}

void CTimer::End()
{
	if (m_hTimer != nullptr) {
		::DeleteTimerQueueTimer(nullptr, m_hTimer, INVALID_HANDLE_VALUE);
		m_hTimer = nullptr;
	}
}

void CALLBACK CTimer::TimerCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	static_cast<CTimer*>(lpParameter)->OnTimer();
}


}	// namespace Util

}	// namespace TVTest
