#ifndef TVTEST_UTIL_H
#define TVTEST_UTIL_H


#include "HelperClass/StdUtil.h"
#include "StringUtility.h"


int HexCharToInt(TCHAR Code);
unsigned int HexStringToUInt(LPCTSTR pszString,int Length,LPCTSTR *ppszEnd=NULL);

bool IsRectIntersect(const RECT *pRect1,const RECT *pRect2);

float LevelToDeciBel(int Level);

COLORREF MixColor(COLORREF Color1,COLORREF Color2,BYTE Ratio=128);
COLORREF HSVToRGB(double Hue,double Saturation,double Value);

inline DWORD TickTimeSpan(DWORD Start,DWORD End) { return End-Start; }
extern __declspec(selectany) const FILETIME FILETIME_NULL={0,0};
#define FILETIME_MILLISECOND	10000LL
#define FILETIME_SECOND			(1000LL*FILETIME_MILLISECOND)
#define FILETIME_MINUTE			(60LL*FILETIME_SECOND)
#define FILETIME_HOUR			(60LL*FILETIME_MINUTE)
FILETIME &operator+=(FILETIME &ft,LONGLONG Offset);
LONGLONG operator-(const FILETIME &ft1,const FILETIME &ft2);
int CompareSystemTime(const SYSTEMTIME *pTime1,const SYSTEMTIME *pTime2);
bool OffsetSystemTime(SYSTEMTIME *pTime,LONGLONG Offset);
LONGLONG DiffSystemTime(const SYSTEMTIME *pStartTime,const SYSTEMTIME *pEndTime);
bool SystemTimeToLocalTimeNoDST(const SYSTEMTIME *pUTCTime,SYSTEMTIME *pLocalTime);
void GetLocalTimeAsFileTime(FILETIME *pTime);
void GetLocalTimeNoDST(SYSTEMTIME *pTime);
void GetLocalTimeNoDST(FILETIME *pTime);
inline bool UTCToJST(SYSTEMTIME *pTime) { return OffsetSystemTime(pTime,9*60*60*1000); }
bool UTCToJST(const SYSTEMTIME *pUTCTime,SYSTEMTIME *pJST);
inline void UTCToJST(FILETIME *pTime) { *pTime+=9LL*FILETIME_HOUR; }
void GetCurrentJST(SYSTEMTIME *pTime);
void GetCurrentJST(FILETIME *pTime);
int CalcDayOfWeek(int Year,int Month,int Day);
LPCTSTR GetDayOfWeekText(int DayOfWeek);

bool CopyTextToClipboard(HWND hwndOwner,LPCTSTR pszText);

void ClearMenu(HMENU hmenu);
int CopyToMenuText(LPCTSTR pszSrcText,LPTSTR pszDstText,int MaxLength);

void InitOpenFileName(OPENFILENAME *pofn);

void ForegroundWindow(HWND hwnd);

bool ChooseColorDialog(HWND hwndOwner,COLORREF *pcrResult);
bool ChooseFontDialog(HWND hwndOwner,LOGFONT *plf);
bool BrowseFolderDialog(HWND hwndOwner,LPTSTR pszDirectory,LPCTSTR pszTitle);

bool CompareLogFont(const LOGFONT *pFont1,const LOGFONT *pFont2);
int PixelsToPoints(int Pixels);
int PointsToPixels(int Points);
int CalcFontPointHeight(HDC hdc,const LOGFONT *pFont);

int GetErrorText(DWORD ErrorCode,LPTSTR pszText,int MaxLength);

bool IsEqualFileName(LPCWSTR pszFileName1,LPCWSTR pszFileName2);
bool IsValidFileName(LPCTSTR pszFileName,bool fWildcard=false,LPTSTR pszMessage=NULL,int MaxMessage=0);
bool GetAbsolutePath(LPCTSTR pszFilePath,LPTSTR pszAbsolutePath,int MaxLength);

HICON CreateIconFromBitmap(HBITMAP hbm,int IconWidth,int IconHeight,int ImageWidth=0,int ImageHeight=0);
HICON CreateEmptyIcon(int Width,int Height);

class CDynamicString {
protected:
	LPTSTR m_pszString;
public:
	CDynamicString();
	CDynamicString(const CDynamicString &String);
#ifdef MOVE_SEMANTICS_SUPPORTED
	CDynamicString(CDynamicString &&String);
#endif
	explicit CDynamicString(LPCTSTR pszString);
	virtual ~CDynamicString();
	CDynamicString &operator=(const CDynamicString &String);
#ifdef MOVE_SEMANTICS_SUPPORTED
	CDynamicString &operator=(CDynamicString &&String);
#endif
	CDynamicString &operator+=(const CDynamicString &String);
	CDynamicString &operator=(LPCTSTR pszString);
	CDynamicString &operator+=(LPCTSTR pszString);
	bool operator==(const CDynamicString &String) const;
	bool operator!=(const CDynamicString &String) const;
	LPCTSTR Get() const { return m_pszString; }
	LPCTSTR GetSafe() const { return NullToEmptyString(m_pszString); }
	bool Set(LPCTSTR pszString);
	bool Set(LPCTSTR pszString,size_t Length);
	bool Attach(LPTSTR pszString);
	int Length() const;
	void Clear();
	bool IsEmpty() const;
	int Compare(LPCTSTR pszString) const;
	int CompareIgnoreCase(LPCTSTR pszString) const;
};

class CStaticStringFormatter
{
public:
	CStaticStringFormatter(LPTSTR pBuffer,size_t BufferLength);
	size_t Length() const { return m_Length; }
	bool IsEmpty() const { return m_Length==0; }
	LPCTSTR GetString() const { return m_pBuffer; }
	void Clear();
	void Append(LPCTSTR pszString);
	void AppendFormat(LPCTSTR pszFormat, ...);
	void AppendFormatV(LPCTSTR pszFormat,va_list Args);
	void RemoveTrailingWhitespace();
private:
	const LPTSTR m_pBuffer;
	const size_t m_BufferLength;
	size_t m_Length;
};

class CFilePath {
	TCHAR m_szPath[MAX_PATH];
public:
	CFilePath();
	CFilePath(const CFilePath &Path);
	explicit CFilePath(LPCTSTR pszPath);
	~CFilePath();
	bool IsEmpty() const { return m_szPath[0]==_T('\0'); }
	bool SetPath(LPCTSTR pszPath);
	LPCTSTR GetPath() const { return m_szPath; }
	void GetPath(LPTSTR pszPath) const;
	int GetLength() const { return ::lstrlen(m_szPath); }
	LPCTSTR GetFileName() const;
	bool SetFileName(LPCTSTR pszFileName);
	bool RemoveFileName();
	LPCTSTR GetExtension() const;
	bool SetExtension(LPCTSTR pszExtension);
	bool RemoveExtension();
	bool AppendExtension(LPCTSTR pszExtension);
	bool Make(LPCTSTR pszDirectory,LPCTSTR pszFileName);
	bool Append(LPCTSTR pszMore);
	bool GetDirectory(LPTSTR pszDirectory) const;
	bool SetDirectory(LPCTSTR pszDirectory);
	bool RemoveDirectory();
	bool HasDirectory() const;
	bool IsRelative() const;
	bool IsExists() const;
	bool IsValid(bool fWildcard=false) const;
};

class CLocalTime {
protected:
	FILETIME m_Time;
public:
	CLocalTime();
	CLocalTime(const FILETIME &Time);
	CLocalTime(const SYSTEMTIME &Time);
	virtual ~CLocalTime();
	bool operator==(const CLocalTime &Time) const;
	bool operator!=(const CLocalTime &Time) const { return !(*this==Time); }
	bool operator<(const CLocalTime &Time) const;
	bool operator>(const CLocalTime &Time) const;
	bool operator<=(const CLocalTime &Time) const;
	bool operator>=(const CLocalTime &Time) const;
	CLocalTime &operator+=(LONGLONG Offset);
	CLocalTime &operator-=(LONGLONG Offset) { return *this+=-Offset; }
	LONGLONG operator-(const CLocalTime &Time) const;
	void SetCurrentTime();
	bool GetTime(FILETIME *pTime) const;
	bool GetTime(SYSTEMTIME *pTime) const;
};

class CGlobalLock
{
	HANDLE m_hMutex;
	bool m_fOwner;

	// delete
	CGlobalLock(const CGlobalLock &);
	CGlobalLock &operator=(const CGlobalLock &);

public:
	CGlobalLock();
	~CGlobalLock();
	bool Create(LPCTSTR pszName);
	bool Wait(DWORD Timeout=INFINITE);
	void Close();
	void Release();
};

namespace Util
{

	template<typename T> T *GetLibraryFunction(HMODULE hLib,LPCSTR pszFunc)
	{
		return reinterpret_cast<T*>(::GetProcAddress(hLib,pszFunc));
	}

	template<typename T> T *GetModuleFunction(LPCTSTR pszModule,LPCSTR pszFunc)
	{
		return reinterpret_cast<T*>(::GetProcAddress(::GetModuleHandle(pszModule),pszFunc));
	}

#define GET_LIBRARY_FUNCTION(hLib,Func) \
	Util::GetLibraryFunction<decltype(Func)>(hLib,#Func)
#define GET_MODULE_FUNCTION(pszModule,Func) \
	Util::GetModuleFunction<decltype(Func)>(pszModule,#Func)

	namespace OS
	{

		bool GetVersion(DWORD *pMajor,DWORD *pMinor);
		DWORD GetMajorVersion();
		bool IsWindowsXP();
		bool IsWindowsVista();
		bool IsWindows7();
		bool IsWindows8();
		bool IsWindowsXPOrLater();
		bool IsWindowsVistaOrLater();
		bool IsWindows7OrLater();

	}	// namespace OS

	class CRect : public ::RECT
	{
	public:
		CRect() { Empty(); }

		CRect(int Left,int Top,int Right,int Bottom)
		{
			Set(Left,Top,Right,Bottom);
		}

		CRect &operator=(const RECT &Op)
		{
			left=Op.left;
			top=Op.top;
			right=Op.right;
			bottom=Op.bottom;
			return *this;
		}

		bool operator==(const CRect &Op)
		{
			return left==Op.left
				&& top==Op.top
				&& right==Op.right
				&& bottom==Op.bottom;
		}

		bool operator!=(const CRect &Op) { return !(*this==Op); }

		void Set(int Left,int Top,int Right,int Bottom)
		{
			left=Left; top=Top; right=Right; bottom=Bottom;
		}

		void Empty() { left=0; top=0; right=0; bottom=0; }

		bool IsEmpty() const { return left==right && top==bottom; }

		int GetWidth() const { return right-left; }

		int GetHeight() const { return bottom-top; }

		bool Intersect(const RECT &Rect)
		{
			RECT rc;
			bool fResult=::IntersectRect(&rc,this,&Rect)!=FALSE;
			*this=rc;
			return fResult;
		}

		bool Union(const RECT &Rect)
		{
			RECT rc;
			bool fResult=::UnionRect(&rc,this,&Rect)!=FALSE;
			*this=rc;
			return fResult;
		}
	};

	class CClock
	{
	public:
		void Start() { m_Clock=::GetTickCount(); }
		DWORD GetSpan() const { return TickTimeSpan(m_Clock,::GetTickCount()); }

	private:
		DWORD m_Clock;
	};

	class CWaitCursor
	{
	public:
		CWaitCursor()
			: m_hcurOld(::SetCursor(::LoadCursor(NULL,IDC_WAIT)))
		{
		}

		~CWaitCursor()
		{
			::SetCursor(m_hcurOld);
		}

	private:
		HCURSOR m_hcurOld;
	};

}	// namespace Util


#endif
