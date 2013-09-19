#include "stdafx.h"
#include <math.h>
#include "Util.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




int HexCharToInt(TCHAR Code)
{
	if (Code>=_T('0') && Code<=_T('9'))
		return Code-_T('0');
	if (Code>=_T('A') && Code<=_T('F'))
		return Code-_T('A')+10;
	if (Code>=_T('a') && Code<=_T('f'))
		return Code-_T('a')+10;
	return 0;
}


unsigned int HexStringToUInt(LPCTSTR pszString,int Length,LPCTSTR *ppszEnd)
{
	unsigned int Value=0;
	int i;
	for (i=0;i<Length;i++) {
		TCHAR Code=pszString[i];
		unsigned int v;
		if (Code>=_T('0') && Code<=_T('9'))
			v=Code-_T('0');
		else if (Code>=_T('A') && Code<=_T('F'))
			v=Code-_T('A')+10;
		else if (Code>=_T('a') && Code<=_T('f'))
			v=Code-_T('a')+10;
		else
			break;
		Value=(Value<<4) | v;
	}
	if (ppszEnd!=NULL)
		*ppszEnd=pszString+i;
	return Value;
}


bool IsRectIntersect(const RECT *pRect1,const RECT *pRect2)
{
	return pRect1->left<pRect2->right && pRect1->right>pRect2->left
		&& pRect1->top<pRect2->bottom && pRect1->bottom>pRect2->top;
}


float LevelToDeciBel(int Level)
{
	float Volume;

	if (Level<=0)
		Volume=-100.0f;
	else if (Level>=100)
		Volume=0.0f;
	else
		Volume=(float)(20.0*log10((double)Level/100.0));
	return Volume;
}


COLORREF MixColor(COLORREF Color1,COLORREF Color2,BYTE Ratio)
{
	return RGB((GetRValue(Color1)*Ratio+GetRValue(Color2)*(255-Ratio))/255,
			   (GetGValue(Color1)*Ratio+GetGValue(Color2)*(255-Ratio))/255,
			   (GetBValue(Color1)*Ratio+GetBValue(Color2)*(255-Ratio))/255);
}


COLORREF HSVToRGB(double Hue,double Saturation,double Value)
{
	double r,g,b;

	if (Saturation==0.0) {
		r=g=b=Value;
	} else {
		double h,s,v;
		double f,p,q,t;

		h=Hue*6.0;
		if (h>=6.0)
			h-=6.0;
		s=Saturation;
		v=Value;
		f=h-(int)h;
		p=v*(1.0-s);
		q=v*(1.0-s*f);
		t=v*(1.0-s*(1.0-f));
		switch ((int)h) {
		case 0: r=v; g=t; b=p; break;
		case 1: r=q; g=v; b=p; break;
		case 2: r=p; g=v; b=t; break;
		case 3: r=p; g=q; b=v; break;
		case 4: r=t; g=p; b=v; break;
		case 5: r=v; g=p; b=q; break;
		}
	}
	return RGB((BYTE)(r*255.0+0.5),(BYTE)(g*255.0+0.5),(BYTE)(b*255.0+0.5));
}


FILETIME &operator+=(FILETIME &ft,LONGLONG Offset)
{
	ULARGE_INTEGER Result;

	Result.LowPart=ft.dwLowDateTime;
	Result.HighPart=ft.dwHighDateTime;
	Result.QuadPart+=Offset;
	ft.dwLowDateTime=Result.LowPart;
	ft.dwHighDateTime=Result.HighPart;
	return ft;
}


LONGLONG operator-(const FILETIME &ft1,const FILETIME &ft2)
{
	LARGE_INTEGER Time1,Time2;

	Time1.LowPart=ft1.dwLowDateTime;
	Time1.HighPart=ft1.dwHighDateTime;
	Time2.LowPart=ft2.dwLowDateTime;
	Time2.HighPart=ft2.dwHighDateTime;
	return Time1.QuadPart-Time2.QuadPart;
}


int CompareSystemTime(const SYSTEMTIME *pTime1,const SYSTEMTIME *pTime2)
{
#if 0
	FILETIME ft1,ft2;

	SystemTimeToFileTime(pTime1,&ft1);
	SystemTimeToFileTime(pTime2,&ft2);
	return CompareFileTime(&ft1,&ft2);
#else
	DWORD Date1,Date2;

	Date1=((DWORD)pTime1->wYear<<16) | ((DWORD)pTime1->wMonth<<8) | pTime1->wDay;
	Date2=((DWORD)pTime2->wYear<<16) | ((DWORD)pTime2->wMonth<<8) | pTime2->wDay;
	if (Date1==Date2) {
		Date1=((DWORD)pTime1->wHour<<24) | ((DWORD)pTime1->wMinute<<16) |
			  ((DWORD)pTime1->wSecond<<10) | pTime1->wMilliseconds;
		Date2=((DWORD)pTime2->wHour<<24) | ((DWORD)pTime2->wMinute<<16) |
			  ((DWORD)pTime2->wSecond<<10) | pTime2->wMilliseconds;
	}
	if (Date1<Date2)
		return -1;
	if (Date1>Date2)
		return 1;
	return 0;
#endif
}


bool OffsetSystemTime(SYSTEMTIME *pTime,LONGLONG Offset)
{
	FILETIME ft;

	if (!::SystemTimeToFileTime(pTime,&ft))
		return false;
	ft+=Offset*FILETIME_MILLISECOND;
	return ::FileTimeToSystemTime(&ft,pTime)!=FALSE;
}


LONGLONG DiffSystemTime(const SYSTEMTIME *pStartTime,const SYSTEMTIME *pEndTime)
{
	FILETIME ftStart,ftEnd;

	::SystemTimeToFileTime(pStartTime,&ftStart);
	::SystemTimeToFileTime(pEndTime,&ftEnd);
	return (ftEnd-ftStart)/FILETIME_MILLISECOND;
}


bool SystemTimeToLocalTimeNoDST(const SYSTEMTIME *pUTCTime,SYSTEMTIME *pLocalTime)
{
	TIME_ZONE_INFORMATION tzi;

	if (GetTimeZoneInformation(&tzi)!=TIME_ZONE_ID_INVALID) {
		tzi.StandardDate.wMonth=0;
		tzi.DaylightDate.wMonth=0;
		if (SystemTimeToTzSpecificLocalTime(&tzi,pUTCTime,pLocalTime))
			return true;
	}
	return SystemTimeToTzSpecificLocalTime(NULL,pUTCTime,pLocalTime)!=FALSE;
}


void GetLocalTimeAsFileTime(FILETIME *pTime)
{
	SYSTEMTIME st;

	GetLocalTime(&st);
	SystemTimeToFileTime(&st,pTime);
}


void GetLocalTimeNoDST(SYSTEMTIME *pTime)
{
	TIME_ZONE_INFORMATION tzi;

	if (GetTimeZoneInformation(&tzi)!=TIME_ZONE_ID_INVALID) {
		SYSTEMTIME stUTC;

		tzi.StandardDate.wMonth=0;
		tzi.DaylightDate.wMonth=0;
		GetSystemTime(&stUTC);
		if (SystemTimeToTzSpecificLocalTime(&tzi,&stUTC,pTime))
			return;
	}
	GetLocalTime(pTime);
}


void GetLocalTimeNoDST(FILETIME *pTime)
{
	SYSTEMTIME st;

	GetLocalTimeNoDST(&st);
	SystemTimeToFileTime(&st,pTime);
}


bool UTCToJST(const SYSTEMTIME *pUTCTime,SYSTEMTIME *pJST)
{
	*pJST=*pUTCTime;
	return UTCToJST(pJST);
}


void GetCurrentJST(SYSTEMTIME *pTime)
{
	::GetSystemTime(pTime);
	if (!UTCToJST(pTime))
		::GetLocalTime(pTime);
}


void GetCurrentJST(FILETIME *pTime)
{
	::GetSystemTimeAsFileTime(pTime);
	UTCToJST(pTime);
}


int CalcDayOfWeek(int Year,int Month,int Day)
{
	if (Month<=2) {
		Year--;
		Month+=12;
	}
	return (Year*365+Year/4-Year/100+Year/400+306*(Month+1)/10+Day-428)%7;
}


LPCTSTR GetDayOfWeekText(int DayOfWeek)
{
	if (DayOfWeek<0 || DayOfWeek>6)
		return TEXT("？");
	return TEXT("日\0月\0火\0水\0木\0金\0土")+DayOfWeek*((3-sizeof(TCHAR))+1);
}


bool CopyTextToClipboard(HWND hwndOwner,LPCTSTR pszText)
{
	if (pszText==NULL)
		return false;

	bool fOK=false;
	SIZE_T Size=(::lstrlen(pszText)+1)*sizeof(TCHAR);
	HGLOBAL hData=::GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE,Size);
	if (hData!=NULL) {
		LPTSTR pBuffer=static_cast<LPTSTR>(::GlobalLock(hData));
		if (pBuffer!=NULL) {
			::CopyMemory(pBuffer,pszText,Size);
			::GlobalUnlock(hData);

			if (::OpenClipboard(hwndOwner)) {
				::EmptyClipboard();
				if (::SetClipboardData(
#ifdef UNICODE
						CF_UNICODETEXT,
#else
						CF_TEXT,
#endif
						hData)!=NULL)
					fOK=true;
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
	int Count,i;

	Count=GetMenuItemCount(hmenu);
	for (i=Count-1;i>=0;i--)
		DeleteMenu(hmenu,i,MF_BYPOSITION);
}


int CopyToMenuText(LPCTSTR pszSrcText,LPTSTR pszDstText,int MaxLength)
{
	int SrcPos,DstPos;

	SrcPos=DstPos=0;
	while (pszSrcText[SrcPos]!=_T('\0') && DstPos+1<MaxLength) {
		if (pszSrcText[SrcPos]==_T('&')) {
			if (DstPos+2>=MaxLength)
				break;
			pszDstText[DstPos++]=_T('&');
			pszDstText[DstPos++]=_T('&');
			SrcPos++;
		} else {
			int Length=StringCharLength(&pszSrcText[SrcPos]);
			if (Length==0 || DstPos+Length>=MaxLength)
				break;
			for (int i=0;i<Length;i++)
				pszDstText[DstPos++]=pszSrcText[SrcPos++];
		}
	}
	pszDstText[DstPos]=_T('\0');
	return DstPos;
}


void InitOpenFileName(OPENFILENAME *pofn)
{
#if _WIN32_WINNT>=0x0500
	OSVERSIONINFO osvi;

	osvi.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	if (GetVersionEx(&osvi)
			&& (osvi.dwMajorVersion>=5 || osvi.dwMinorVersion==90)) {
		/* Windows 2000/XP/Vista/7 or Me */
		pofn->lStructSize=sizeof(OPENFILENAME);
		pofn->pvReserved=NULL;
		pofn->dwReserved=0;
		pofn->FlagsEx=0;
	} else
		pofn->lStructSize=OPENFILENAME_SIZE_VERSION_400;
#else
	pofn->lStructSize=sizeof(OPENFILENAME);
#endif
	pofn->hwndOwner=NULL;
	pofn->hInstance=NULL;
	pofn->lpstrCustomFilter=NULL;
	pofn->nMaxCustFilter=0;
	pofn->nFilterIndex=1;
	pofn->lpstrFileTitle=NULL;
	pofn->lpstrInitialDir=NULL;
	pofn->lpstrTitle=NULL;
	pofn->Flags=0;
	pofn->lpstrDefExt=NULL;
}


void ForegroundWindow(HWND hwnd)
{
	int TargetID,ForegroundID;

	ForegroundID=GetWindowThreadProcessId(GetForegroundWindow(),NULL);
	TargetID=GetWindowThreadProcessId(hwnd,NULL);
	AttachThreadInput(TargetID,ForegroundID,TRUE);
	SetForegroundWindow(hwnd);
	AttachThreadInput(TargetID,ForegroundID,FALSE);
}


bool ChooseColorDialog(HWND hwndOwner,COLORREF *pcrResult)
{
	CHOOSECOLOR cc;
	static COLORREF crCustomColors[16] = {
		RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),
		RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),
		RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),
		RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),
		RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),
		RGB(0xFF,0xFF,0xFF)
	};

	cc.lStructSize=sizeof(CHOOSECOLOR);
	cc.hwndOwner=hwndOwner;
	cc.hInstance=NULL;
	cc.rgbResult=*pcrResult;
	cc.lpCustColors=crCustomColors;
	cc.Flags=CC_RGBINIT | CC_FULLOPEN;
	if (!ChooseColor(&cc))
		return false;
	*pcrResult=cc.rgbResult;
	return true;
}


bool ChooseFontDialog(HWND hwndOwner,LOGFONT *plf)
{
	CHOOSEFONT cf;

	cf.lStructSize=sizeof(CHOOSEFONT);
	cf.hwndOwner=hwndOwner;
	cf.lpLogFont=plf;
	cf.Flags=CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;
	return ChooseFont(&cf)!=FALSE;
}


int CALLBACK BrowseFolderCallback(HWND hwnd,UINT uMsg,LPARAM lpData,
																LPARAM lParam)
{
	switch (uMsg) {
	case BFFM_INITIALIZED:
		if (((LPTSTR)lParam)[0]!=_T('\0')) {
			TCHAR szDirectory[MAX_PATH];

			lstrcpy(szDirectory,(LPTSTR)lParam);
			PathRemoveBackslash(szDirectory);
			SendMessage(hwnd,BFFM_SETSELECTION,TRUE,(LPARAM)szDirectory);
		}
		break;
	}
	return 0;
}


bool BrowseFolderDialog(HWND hwndOwner,LPTSTR pszDirectory,LPCTSTR pszTitle)
{
	BROWSEINFO bi;
	PIDLIST_ABSOLUTE pidl;
	BOOL fRet;

	bi.hwndOwner=hwndOwner;
	bi.pidlRoot=NULL;
	bi.pszDisplayName=pszDirectory;
	bi.lpszTitle=pszTitle;
	bi.ulFlags=BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	bi.lpfn=BrowseFolderCallback;
	bi.lParam=(LPARAM)pszDirectory;
	pidl=SHBrowseForFolder(&bi);
	if (pidl==NULL)
		return false;
	fRet=SHGetPathFromIDList(pidl,pszDirectory);
	CoTaskMemFree(pidl);
	return fRet==TRUE;
}


bool CompareLogFont(const LOGFONT *pFont1,const LOGFONT *pFont2)
{
	return memcmp(pFont1,pFont2,28/*offsetof(LOGFONT,lfFaceName)*/)==0
		&& lstrcmp(pFont1->lfFaceName,pFont2->lfFaceName)==0;
}


int PixelsToPoints(int Pixels)
{
	HDC hIC=::CreateIC(TEXT("DISPLAY"),NULL,NULL,NULL);
	if (hIC==NULL)
		return 0;
	int Resolution=::GetDeviceCaps(hIC,LOGPIXELSY);
	int Points;
	if (Resolution!=0)
		Points=::MulDiv(Pixels,72,Resolution);
	else
		Points=0;
	::DeleteDC(hIC);
	return Points;
}


int PointsToPixels(int Points)
{
	HDC hIC=::CreateIC(TEXT("DISPLAY"),NULL,NULL,NULL);
	if (hIC==NULL)
		return 0;
	int Resolution=::GetDeviceCaps(hIC,LOGPIXELSY);
	int Pixels;
	if (Resolution!=0)
		Pixels=::MulDiv(Points,Resolution,72);
	else
		Pixels=0;
	::DeleteDC(hIC);
	return Pixels;
}


int CalcFontPointHeight(HDC hdc,const LOGFONT *pFont)
{
	if (hdc==NULL || pFont==NULL)
		return 0;

	HFONT hfont=CreateFontIndirect(pFont),hfontOld;
	if (hfont==NULL)
		return 0;

	TEXTMETRIC tm;
	int PixelsPerInch;

	hfontOld=static_cast<HFONT>(SelectObject(hdc,hfont));
	GetTextMetrics(hdc,&tm);
	PixelsPerInch=GetDeviceCaps(hdc,LOGPIXELSY);
	SelectObject(hdc,hfontOld);
	DeleteObject(hfont);
	if (PixelsPerInch==0)
		return 0;
	return MulDiv(tm.tmHeight-tm.tmInternalLeading,72,PixelsPerInch);
}


int GetErrorText(DWORD ErrorCode,LPTSTR pszText,int MaxLength)
{
	if (pszText==NULL || MaxLength<1)
		return 0;

	int Length=::FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,NULL,
		ErrorCode,MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
		pszText,MaxLength,NULL);
	if (Length==0)
		pszText[0]=_T('\0');
	return Length;
}


class CFileNameCompare
{
public:
	CFileNameCompare()
		: m_pCompareStringOrdinal(GET_MODULE_FUNCTION(TEXT("kernel32.dll"),CompareStringOrdinal))
	{
	}

	bool IsEqual(LPCWSTR pString1,int Count1,LPCWSTR pString2,int Count2)
	{
		if (m_pCompareStringOrdinal!=NULL)
			return m_pCompareStringOrdinal(pString1,Count1,pString2,Count2,TRUE)==CSTR_EQUAL;

		if (Count1<0)
			Count1=::lstrlenW(pString1);
		if (Count2<0)
			Count2=::lstrlenW(pString2);
		if (Count1!=Count2)
			return false;
		bool fResult;
		if (Count1<=MAX_PATH) {
			WCHAR Buff1[MAX_PATH],Buff2[MAX_PATH];
			::CopyMemory(Buff1,pString1,Count1*sizeof(WCHAR));
			::CharUpperBuffW(Buff1,Count1);
			::CopyMemory(Buff2,pString2,Count2*sizeof(WCHAR));
			::CharUpperBuffW(Buff2,Count2);
			fResult=std::wmemcmp(Buff1,Buff2,Count1)==0;
		} else {
			TVTest::String Buff1(pString1,Count1);
			TVTest::String Buff2(pString2,Count2);
			TVTest::StringUtility::ToUpper(Buff1);
			TVTest::StringUtility::ToUpper(Buff2);
			fResult=Buff1==Buff2;
		}

		return fResult;
	}

	bool IsEqual(LPCWSTR pszString1,LPCWSTR pszString2)
	{
		return IsEqual(pszString1,-1,pszString2,-1);
	}

private:
	decltype(CompareStringOrdinal) *m_pCompareStringOrdinal;
};

static CFileNameCompare g_FileNameCompare;

bool IsEqualFileName(LPCWSTR pszFileName1,LPCWSTR pszFileName2)
{
	return g_FileNameCompare.IsEqual(pszFileName1,pszFileName2);
}


bool IsValidFileName(LPCTSTR pszFileName,bool fWildcard,LPTSTR pszMessage,int MaxMessage)
{
	if (pszFileName==NULL || pszFileName[0]==_T('\0')) {
		if (pszMessage!=NULL)
			lstrcpyn(pszMessage,TEXT("ファイル名が指定されていません。"),MaxMessage);
		return false;
	}
	int Length=lstrlen(pszFileName);
	if (Length>=MAX_PATH) {
		if (pszMessage!=NULL)
			lstrcpyn(pszMessage,TEXT("ファイル名が長すぎます。"),MaxMessage);
		return false;
	}
	if (Length==3) {
		static const LPCTSTR pszNGList[] = {
			TEXT("CON"), TEXT("PRN"), TEXT("AUX"), TEXT("NUL")
		};
		for (int i=0;i<_countof(pszNGList);i++) {
			if (lstrcmpi(pszNGList[i],pszFileName)==0) {
				if (pszMessage!=NULL)
					lstrcpyn(pszMessage,TEXT("仮想デバイス名はファイル名に使用できません。"),MaxMessage);
				return false;
			}
		}
	} else if (Length==4) {
		TCHAR szName[5];

		for (int i=1;i<=9;i++) {
			wsprintf(szName,TEXT("COM%d"),i);
			if (lstrcmpi(szName,pszFileName)==0) {
				if (pszMessage!=NULL)
					lstrcpyn(pszMessage,TEXT("仮想デバイス名はファイル名に使用できません。"),MaxMessage);
				return false;
			}
		}
		for (int i=1;i<=9;i++) {
			wsprintf(szName,TEXT("LPT%d"),i);
			if (lstrcmpi(szName,pszFileName)==0) {
				if (pszMessage!=NULL)
					lstrcpyn(pszMessage,TEXT("仮想デバイス名はファイル名に使用できません。"),MaxMessage);
				return false;
			}
		}
	}
	LPCTSTR p=pszFileName;
	while (*p!=_T('\0')) {
		if (*p<=31 || *p==_T('<') || *p==_T('>') || *p==_T(':') || *p==_T('"')
				|| *p==_T('/') || *p==_T('\\') || *p==_T('|')
				|| (!fWildcard && (*p==_T('*') || *p==_T('?')))) {
			if (pszMessage!=NULL)
				lstrcpyn(pszMessage,TEXT("ファイル名に使用できない文字が含まれています。"),MaxMessage);
			return false;
		}
		if ((*p==_T(' ') || *p==_T('.')) && *(p+1)==_T('\0')) {
			if (pszMessage!=NULL)
				lstrcpyn(pszMessage,TEXT("ファイル名の末尾に半角空白及び . は使用できません。"),MaxMessage);
			return false;
		}
#ifndef UNICODE
		if (IsDBCSLeadByteEx(CP_ACP,*p)) {
			if (*(p+1)==_T('\0')) {
				lstrcpyn(pszMessage,TEXT("2バイト文字の2バイト目が欠けています。"),MaxMessage);
				return false;
			}
			p++;
		}
#endif
		p++;
	}
	return true;
}


bool GetAbsolutePath(LPCTSTR pszFilePath,LPTSTR pszAbsolutePath,int MaxLength)
{
	if (pszAbsolutePath==NULL || MaxLength<1)
		return false;
	pszAbsolutePath[0]=_T('\0');
	if (pszFilePath==NULL || pszFilePath[0]==_T('\0'))
		return false;
	if (::PathIsRelative(pszFilePath)) {
		TCHAR szTemp[MAX_PATH],*p;

		::GetModuleFileName(NULL,szTemp,_countof(szTemp));
		p=::PathFindFileName(szTemp);
		if ((p-szTemp)+::lstrlen(pszFilePath)>=MaxLength)
			return false;
		::lstrcpy(p,pszFilePath);
		::PathCanonicalize(pszAbsolutePath,szTemp);
	} else {
		if (::lstrlen(pszFilePath)>=MaxLength)
			return false;
		::lstrcpy(pszAbsolutePath,pszFilePath);
	}
	return true;
}


static HBITMAP CreateIconMaskBitmap(int IconWidth,int IconHeight,
									int ImageWidth,int ImageHeight)
{
	SIZE_T BytesPerLine,BitsBytes;
	BYTE *pBits;
	int Top;
	HBITMAP hbm;

	BytesPerLine=(IconWidth+15)/16*2;
	BitsBytes=BytesPerLine*IconHeight;
	pBits=new BYTE[BitsBytes];
	::FillMemory(pBits,BitsBytes,0xFF);
	Top=(IconHeight-ImageHeight)/2;
	if (ImageWidth==IconWidth) {
		::ZeroMemory(pBits+Top*BytesPerLine,ImageHeight*BytesPerLine);
	} else {
		int Left,x,y;
		BYTE *p;

		Left=(IconWidth-ImageWidth)/2;
		p=pBits+Top*BytesPerLine;
		for (y=0;y<ImageHeight;y++) {
			for (x=Left;x<Left+ImageWidth;x++)
				//p[x/8]&=~(0x80>>(x%8));
				p[x>>3]&=~(0x80>>(x&7));
			p+=BytesPerLine;
		}
	}
	hbm=::CreateBitmap(IconWidth,IconHeight,1,1,pBits);
	delete [] pBits;
	return hbm;
}

static HBITMAP CreateIconColorBitmap(HBITMAP hbm,int IconWidth,int IconHeight,
									 int ImageWidth,int ImageHeight)
{
	HDC hdc;
	HBITMAP hbmIcon;

	hdc=::GetDC(NULL);
	hbmIcon=::CreateCompatibleBitmap(hdc,IconWidth,IconHeight);
	if (hbmIcon!=NULL) {
		BITMAP bm;
		::GetObject(hbm,sizeof(bm),&bm);
		HDC hdcSrc=::CreateCompatibleDC(hdc);
		HBITMAP hbmSrcOld=static_cast<HBITMAP>(::SelectObject(hdcSrc,hbm));
		HDC hdcDest=::CreateCompatibleDC(hdc);
		HBITMAP hbmDestOld=static_cast<HBITMAP>(::SelectObject(hdcDest,hbmIcon));

		if (ImageWidth<IconWidth || ImageHeight<IconHeight) {
			RECT rc={0,0,IconWidth,IconHeight};

			::FillRect(hdcDest,&rc,static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
		}
		int OldStretchMode=::SetStretchBltMode(hdcDest,STRETCH_HALFTONE);
		::StretchBlt(hdcDest,
					 (IconWidth-ImageWidth)/2,(IconHeight-ImageHeight)/2,
					 ImageWidth,ImageHeight,
					 hdcSrc,0,0,bm.bmWidth,bm.bmHeight,SRCCOPY);
		::SetStretchBltMode(hdcDest,OldStretchMode);
		::SelectObject(hdcDest,hbmDestOld);
		::DeleteDC(hdcDest);
		::SelectObject(hdcSrc,hbmSrcOld);
		::DeleteDC(hdcSrc);
	}
	::ReleaseDC(NULL,hdc);
	return hbmIcon;
}

HICON CreateIconFromBitmap(HBITMAP hbm,int IconWidth,int IconHeight,int ImageWidth,int ImageHeight)
{
	if (hbm==NULL || IconWidth<=0 || IconHeight<=0
			|| ImageWidth<0 || ImageWidth>IconWidth
			|| ImageHeight<0 || ImageHeight>IconHeight)
		return NULL;

	if (ImageWidth==0 || ImageHeight==0) {
		BITMAP bm;

		if (::GetObject(hbm,sizeof(bm),&bm)!=sizeof(bm))
			return NULL;
		if (bm.bmWidth<=IconWidth && bm.bmHeight<=IconHeight) {
			ImageWidth=bm.bmWidth;
			ImageHeight=bm.bmHeight;
		} else {
			ImageWidth=min(bm.bmWidth*IconHeight/bm.bmHeight,IconWidth);
			if (ImageWidth<1)
				ImageWidth=1;
			ImageHeight=min(bm.bmHeight*IconWidth/bm.bmWidth,IconHeight);
			if (ImageHeight<1)
				ImageHeight=1;
		}
	}

	ICONINFO ii;
	ii.hbmMask=CreateIconMaskBitmap(IconWidth,IconHeight,ImageWidth,ImageHeight);
	if (ii.hbmMask==NULL)
		return NULL;
	ii.hbmColor=CreateIconColorBitmap(hbm,IconWidth,IconHeight,ImageWidth,ImageHeight);
	if (ii.hbmColor==NULL) {
		::DeleteObject(ii.hbmMask);
		return NULL;
	}
	ii.fIcon=TRUE;
	ii.xHotspot=0;
	ii.yHotspot=0;
	HICON hico=::CreateIconIndirect(&ii);
	::DeleteObject(ii.hbmMask);
	::DeleteObject(ii.hbmColor);
	return hico;
}


HICON CreateEmptyIcon(int Width,int Height)
{
	size_t Size=(Width+7)/8*Height;
	BYTE *pAndBits=new BYTE[Size*2];
	BYTE *pXorBits=pAndBits+Size;
	::FillMemory(pAndBits,Size,0xFF);
	::FillMemory(pXorBits,Size,0x00);
	HICON hico=::CreateIcon(::GetModuleHandle(NULL),Width,Height,1,1,pAndBits,pXorBits);
	delete [] pAndBits;
	return hico;
}




CDynamicString::CDynamicString()
	: m_pszString(NULL)
{
}


CDynamicString::CDynamicString(const CDynamicString &String)
	: m_pszString(NULL)
{
	*this=String;
}


#ifdef MOVE_SEMANTICS_SUPPORTED
CDynamicString::CDynamicString(CDynamicString &&String)
	: m_pszString(String.m_pszString)
{
	String.m_pszString=NULL;
}
#endif


CDynamicString::CDynamicString(LPCTSTR pszString)
	: m_pszString(NULL)
{
	Set(pszString);
}


CDynamicString::~CDynamicString()
{
	Clear();
}


CDynamicString &CDynamicString::operator=(const CDynamicString &String)
{
	if (&String!=this) {
		ReplaceString(&m_pszString,String.m_pszString);
	}
	return *this;
}


#ifdef MOVE_SEMANTICS_SUPPORTED
CDynamicString &CDynamicString::operator=(CDynamicString &&String)
{
	if (&String!=this) {
		Clear();
		m_pszString=String.m_pszString;
		String.m_pszString=NULL;
	}
	return *this;
}
#endif


CDynamicString &CDynamicString::operator+=(const CDynamicString &String)
{
	return *this+=String.m_pszString;
}


CDynamicString &CDynamicString::operator=(LPCTSTR pszString)
{
	ReplaceString(&m_pszString,pszString);
	return *this;
}


CDynamicString &CDynamicString::operator+=(LPCTSTR pszString)
{
	int Length=0;
	if (m_pszString!=NULL)
		Length+=::lstrlen(m_pszString);
	if (pszString!=NULL)
		Length+=::lstrlen(pszString);
	if (Length>0) {
		LPTSTR pszOldString=m_pszString;

		m_pszString=new TCHAR[Length+1];
		m_pszString[0]='\0';
		if (pszOldString!=NULL) {
			::lstrcpy(m_pszString,pszOldString);
			delete [] pszOldString;
		}
		if (pszString!=NULL)
			::lstrcat(m_pszString,pszString);
	}
	return *this;
}


bool CDynamicString::operator==(const CDynamicString &String) const
{
	return Compare(String.m_pszString)==0;
}


bool CDynamicString::operator!=(const CDynamicString &String) const
{
	return Compare(String.m_pszString)!=0;
}


bool CDynamicString::Set(LPCTSTR pszString)
{
	return ReplaceString(&m_pszString,pszString);
}


bool CDynamicString::Set(LPCTSTR pszString,size_t Length)
{
	Clear();
	if (pszString!=NULL && Length>0) {
		Length=StdUtil::strnlen(pszString,Length);
		m_pszString=new TCHAR[Length+1];
		::CopyMemory(m_pszString,pszString,Length*sizeof(TCHAR));
		m_pszString[Length]=_T('\0');
	}
	return true;
}


bool CDynamicString::Attach(LPTSTR pszString)
{
	Clear();
	m_pszString=pszString;
	return true;
}


int CDynamicString::Length() const
{
	if (m_pszString==NULL)
		return 0;
	return ::lstrlen(m_pszString);
}


void CDynamicString::Clear()
{
	if (m_pszString!=NULL) {
		delete [] m_pszString;
		m_pszString=NULL;
	}
}


bool CDynamicString::IsEmpty() const
{
	return IsStringEmpty(m_pszString);
}


int CDynamicString::Compare(LPCTSTR pszString) const
{
	if (IsEmpty()) {
		if (IsStringEmpty(pszString))
			return 0;
		return -1;
	}
	if (IsStringEmpty(pszString))
		return 1;
	return ::lstrcmp(m_pszString,pszString);
}


int CDynamicString::CompareIgnoreCase(LPCTSTR pszString) const
{
	if (IsEmpty()) {
		if (IsStringEmpty(pszString))
			return 0;
		return -1;
	}
	if (IsStringEmpty(pszString))
		return 1;
	return ::lstrcmpi(m_pszString,pszString);
}




CStaticStringFormatter::CStaticStringFormatter(LPTSTR pBuffer,size_t BufferLength)
	: m_pBuffer(pBuffer)
	, m_BufferLength(BufferLength)
	, m_Length(0)
{
	m_pBuffer[0]=_T('\0');
}

void CStaticStringFormatter::Clear()
{
	m_Length=0;
}

void CStaticStringFormatter::Append(LPCTSTR pszString)
{
	if (pszString!=NULL && m_Length+1<m_BufferLength) {
		size_t Length=StdUtil::strnlen(pszString,m_BufferLength-m_Length-1);
		StdUtil::strncpy(m_pBuffer+m_Length,Length+1,pszString);
		m_Length+=Length;
	}
}

void CStaticStringFormatter::AppendFormat(LPCTSTR pszFormat, ...)
{
	if (pszFormat!=NULL && m_Length+1<m_BufferLength) {
		va_list Args;

		va_start(Args,pszFormat);
		AppendFormatV(pszFormat,Args);
		va_end(Args);
	}
}

void CStaticStringFormatter::AppendFormatV(LPCTSTR pszFormat,va_list Args)
{
	if (pszFormat!=NULL && m_Length+1<m_BufferLength) {
		int Length=StdUtil::vsnprintf(m_pBuffer+m_Length,m_BufferLength-m_Length,pszFormat,Args);
		if (Length>=0)
			m_Length+=Length;
	}
}

void CStaticStringFormatter::RemoveTrailingWhitespace()
{
	if (m_Length>0) {
		m_Length-=::RemoveTrailingWhitespace(m_pBuffer);
	}
}




CFilePath::CFilePath()
{
	m_szPath[0]='\0';
}


CFilePath::CFilePath(const CFilePath &Path)
{
	::lstrcpy(m_szPath,Path.m_szPath);
}


CFilePath::CFilePath(LPCTSTR pszPath)
{
	if (pszPath)
		::lstrcpy(m_szPath,pszPath);
	else
		m_szPath[0]='\0';
}


CFilePath::~CFilePath()
{
}


bool CFilePath::SetPath(LPCTSTR pszPath)
{
	if (::lstrlen(pszPath)>=MAX_PATH)
		return false;
	::lstrcpy(m_szPath,pszPath);
	return true;
}


void CFilePath::GetPath(LPTSTR pszPath) const
{
	::lstrcpy(pszPath,m_szPath);
}


LPCTSTR CFilePath::GetFileName() const
{
	if (m_szPath[0]=='\0')
		return m_szPath;
	return ::PathFindFileName(m_szPath);
}


bool CFilePath::SetFileName(LPCTSTR pszFileName)
{
	::lstrcpy(::PathFindFileName(m_szPath),pszFileName);
	return true;
}


bool CFilePath::RemoveFileName()
{
	LPTSTR pszFileName=::PathFindFileName(m_szPath);

	if (pszFileName>m_szPath) {
		*pszFileName='\0';
		::PathRemoveBackslash(m_szPath);
	}
	return true;
}


LPCTSTR CFilePath::GetExtension() const
{
	return ::PathFindExtension(m_szPath);
}


bool CFilePath::SetExtension(LPCTSTR pszExtension)
{
	return ::PathRenameExtension(m_szPath,pszExtension)!=FALSE;
}


bool CFilePath::RemoveExtension()
{
	if (m_szPath[0]!='\0')
		::PathRemoveExtension(m_szPath);
	return true;
}


bool CFilePath::AppendExtension(LPCTSTR pszExtension)
{
	if (::lstrlen(m_szPath)+::lstrlen(pszExtension)>=MAX_PATH)
		return false;
	::lstrcat(m_szPath,pszExtension);
	return true;
}


bool CFilePath::Make(LPCTSTR pszDirectory,LPCTSTR pszFileName)
{
	::PathCombine(m_szPath,pszDirectory,pszFileName);
	return true;
}


bool CFilePath::Append(LPCTSTR pszMore)
{
	return ::PathAppend(m_szPath,pszMore)!=FALSE;
}


bool CFilePath::GetDirectory(LPTSTR pszDirectory) const
{
	LPCTSTR pszFileName=GetFileName();

	if (pszFileName==m_szPath) {
		pszDirectory[0]='\0';
		return false;
	}
	::lstrcpyn(pszDirectory,m_szPath,(int)(pszFileName-m_szPath));
	::PathRemoveBackslash(pszDirectory);
	return true;
}


bool CFilePath::SetDirectory(LPCTSTR pszDirectory)
{
	if (IsStringEmpty(pszDirectory)) {
		RemoveDirectory();
	} else {
		TCHAR szPath[MAX_PATH];

		if (IsRelative()) {
			if (::PathCombine(szPath,pszDirectory,m_szPath)==NULL
					|| !::PathCanonicalize(m_szPath,szPath))
				return false;
		} else {
			if (::PathCombine(szPath,pszDirectory,GetFileName())==NULL)
				return false;
			::lstrcpy(m_szPath,szPath);
		}
	}
	return true;
}


bool CFilePath::RemoveDirectory()
{
	LPTSTR pszFileName=::PathFindFileName(m_szPath);

	if (pszFileName>m_szPath)
		::MoveMemory(m_szPath,pszFileName,(::lstrlen(pszFileName)+1)*sizeof(TCHAR));
	return true;
}


bool CFilePath::HasDirectory() const
{
	if (m_szPath[0]=='\0')
		return false;
	return !::PathIsFileSpec(m_szPath);
}


bool CFilePath::IsRelative() const
{
	if (m_szPath[0]=='\0')
		return false;
	return ::PathIsRelative(m_szPath)!=0;
}


bool CFilePath::IsExists() const
{
	if (m_szPath[0]=='\0')
		return false;
	return ::PathFileExists(m_szPath)!=FALSE;
}


bool CFilePath::IsValid(bool fWildcard) const
{
	UINT Mask;
	LPCTSTR p;

	if (m_szPath[0]=='\0')
		return false;
	Mask=GCT_INVALID;
	if (!fWildcard)
		Mask|=GCT_WILD;
	p=m_szPath;
	while (*p!='\0') {
#ifndef UNICODE
		if (::IsDBCSLeadByteEx(CP_ACP,*p)) {
			if (*(p+1)!='\0')
				p++;
			p++;
			continue;
		}
#endif
		if ((::PathGetCharType(*p)&Mask)!=0)
			return false;
		p++;
	}
	return true;
}




CLocalTime::CLocalTime()
{
	SetCurrentTime();
}


CLocalTime::CLocalTime(const FILETIME &Time)
{
	m_Time=Time;
}


CLocalTime::CLocalTime(const SYSTEMTIME &Time)
{
	::SystemTimeToFileTime(&Time,&m_Time);
}


CLocalTime::~CLocalTime()
{
}


bool CLocalTime::operator==(const CLocalTime &Time) const
{
	return m_Time.dwLowDateTime==Time.m_Time.dwLowDateTime
		&& m_Time.dwHighDateTime==Time.m_Time.dwHighDateTime;
}


bool CLocalTime::operator<(const CLocalTime &Time) const
{
	return m_Time.dwHighDateTime<Time.m_Time.dwHighDateTime
		|| (m_Time.dwHighDateTime==Time.m_Time.dwHighDateTime
			&& m_Time.dwLowDateTime<Time.m_Time.dwLowDateTime);
}


bool CLocalTime::operator>(const CLocalTime &Time) const
{
	return m_Time.dwHighDateTime>Time.m_Time.dwHighDateTime
		|| (m_Time.dwHighDateTime==Time.m_Time.dwHighDateTime
			&& m_Time.dwLowDateTime>Time.m_Time.dwLowDateTime);
}


bool CLocalTime::operator<=(const CLocalTime &Time) const
{
	return m_Time.dwHighDateTime<Time.m_Time.dwHighDateTime
		|| (m_Time.dwHighDateTime==Time.m_Time.dwHighDateTime
			&& m_Time.dwLowDateTime<=Time.m_Time.dwLowDateTime);
}


bool CLocalTime::operator>=(const CLocalTime &Time) const
{
	return m_Time.dwHighDateTime>Time.m_Time.dwHighDateTime
		|| (m_Time.dwHighDateTime==Time.m_Time.dwHighDateTime
			&& m_Time.dwLowDateTime>=Time.m_Time.dwLowDateTime);
}


CLocalTime &CLocalTime::operator+=(LONGLONG Offset)
{
	m_Time+=Offset*FILETIME_MILLISECOND;
	return *this;
}


LONGLONG CLocalTime::operator-(const CLocalTime &Time) const
{
	return (((LONGLONG)m_Time.dwHighDateTime<<32)|(LONGLONG)m_Time.dwLowDateTime)-
		(((LONGLONG)Time.m_Time.dwHighDateTime<<32)|(LONGLONG)Time.m_Time.dwLowDateTime);
}


void CLocalTime::SetCurrentTime()
{
	SYSTEMTIME st;

	::GetLocalTime(&st);
	::SystemTimeToFileTime(&st,&m_Time);
}


bool CLocalTime::GetTime(FILETIME *pTime) const
{
	*pTime=m_Time;
	return true;
}


bool CLocalTime::GetTime(SYSTEMTIME *pTime) const
{
	return ::FileTimeToSystemTime(&m_Time,pTime)!=0;
}


CGlobalLock::CGlobalLock()
	: m_hMutex(NULL)
	, m_fOwner(false)
{
}

CGlobalLock::~CGlobalLock()
{
	Close();
}

bool CGlobalLock::Create(LPCTSTR pszName)
{
	if (m_hMutex!=NULL)
		return false;
	SECURITY_DESCRIPTOR sd;
	SECURITY_ATTRIBUTES sa;
	::ZeroMemory(&sd,sizeof(sd));
	::InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION);
	::SetSecurityDescriptorDacl(&sd,TRUE,NULL,FALSE);
	::ZeroMemory(&sa,sizeof(sa));
	sa.nLength=sizeof(sa);
	sa.lpSecurityDescriptor=&sd;
	m_hMutex=::CreateMutex(&sa,FALSE,pszName);
	m_fOwner=false;
	return m_hMutex!=NULL;
}

bool CGlobalLock::Wait(DWORD Timeout)
{
	if (m_hMutex==NULL)
		return false;
	if (::WaitForSingleObject(m_hMutex,Timeout)==WAIT_TIMEOUT)
		return false;
	m_fOwner=true;
	return true;
}

void CGlobalLock::Close()
{
	if (m_hMutex!=NULL) {
		Release();
		::CloseHandle(m_hMutex);
		m_hMutex=NULL;
	}
}

void CGlobalLock::Release()
{
	if (m_hMutex!=NULL && m_fOwner) {
		::ReleaseMutex(m_hMutex);
		m_fOwner=false;
	}
}


namespace Util
{

	namespace OS
	{

		bool GetVersion(DWORD *pMajor,DWORD *pMinor)
		{
			OSVERSIONINFO osvi;

			osvi.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
			if (!::GetVersionEx(&osvi))
				return false;
			if (pMajor)
				*pMajor=osvi.dwMajorVersion;
			if (pMinor)
				*pMinor=osvi.dwMinorVersion;
			return true;
		}

		static bool CheckOSVersion(DWORD Major,DWORD Minor)
		{
			DWORD MajorVersion,MinorVersion;
			if (!GetVersion(&MajorVersion,&MinorVersion))
				return false;
			return MajorVersion==Major
				&& MinorVersion==Minor;
		}

		static bool CheckOSVersionLater(DWORD Major,DWORD Minor)
		{
			DWORD MajorVersion,MinorVersion;
			if (!GetVersion(&MajorVersion,&MinorVersion))
				return false;
			return MajorVersion>Major
				|| (MajorVersion==Major && MinorVersion>=Minor);
		}

		DWORD GetMajorVersion()
		{
			DWORD MajorVersion;
			if (!GetVersion(&MajorVersion,nullptr))
				return 0;
			return MajorVersion;
		}

		bool IsWindowsXP()
		{
			DWORD MajorVersion,MinorVersion;
			if (!GetVersion(&MajorVersion,&MinorVersion))
				return false;
			return MajorVersion==5 && MinorVersion>=1;
		}

		bool IsWindowsVista()
		{
			return CheckOSVersion(6,0);
		}

		bool IsWindows7()
		{
			return CheckOSVersion(6,1);
		}

		bool IsWindows8()
		{
			return CheckOSVersion(6,2);
		}

		bool IsWindowsXPOrLater()
		{
			return CheckOSVersionLater(5,1);
		}

		bool IsWindowsVistaOrLater()
		{
			return CheckOSVersionLater(6,0);
		}

		bool IsWindows7OrLater()
		{
			return CheckOSVersionLater(6,1);
		}

	}	// namespace OS

}	// namespace Util
