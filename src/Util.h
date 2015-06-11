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
void RGBToHSV(BYTE Red,BYTE Green,BYTE Blue,
			  double *pHue,double *pSaturation,double *pValue);

inline DWORD TickTimeSpan(DWORD Start,DWORD End) { return End-Start; }
inline ULONGLONG TickTimeSpan(ULONGLONG Start,ULONGLONG End) { _ASSERT(Start<=End); return End-Start; }

extern __declspec(selectany) const FILETIME FILETIME_NULL={0,0};
#define FILETIME_MILLISECOND	10000LL
#define FILETIME_SECOND			(1000LL*FILETIME_MILLISECOND)
#define FILETIME_MINUTE			(60LL*FILETIME_SECOND)
#define FILETIME_HOUR			(60LL*FILETIME_MINUTE)
FILETIME &operator+=(FILETIME &ft,LONGLONG Offset);
LONGLONG operator-(const FILETIME &ft1,const FILETIME &ft2);
struct TimeConsts {
	static const LONGLONG SYSTEMTIME_SECOND=1000LL;
	static const LONGLONG SYSTEMTIME_MINUTE=60LL*SYSTEMTIME_SECOND;
	static const LONGLONG SYSTEMTIME_HOUR  =60LL*SYSTEMTIME_MINUTE;
	static const LONGLONG SYSTEMTIME_DAY   =24LL*SYSTEMTIME_HOUR;
};
int CompareSystemTime(const SYSTEMTIME *pTime1,const SYSTEMTIME *pTime2);
bool OffsetSystemTime(SYSTEMTIME *pTime,LONGLONG Offset);
LONGLONG DiffSystemTime(const SYSTEMTIME *pStartTime,const SYSTEMTIME *pEndTime);
void GetLocalTimeAsFileTime(FILETIME *pTime);
void SystemTimeTruncateDay(SYSTEMTIME *pTime);
void SystemTimeTruncateHour(SYSTEMTIME *pTime);
void SystemTimeTruncateMinute(SYSTEMTIME *pTime);
void SystemTimeTruncateSecond(SYSTEMTIME *pTime);
bool GetJSTTimeZoneInformation(TIME_ZONE_INFORMATION *pInfo);
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
enum {
	FILENAME_VALIDATE_WILDCARD       = 0x0001U,
	FILENAME_VALIDATE_ALLOWDELIMITER = 0x0002U
};
bool IsValidFileName(LPCTSTR pszFileName,unsigned int Flags=0,TVTest::String *pMessage=NULL);
bool MakeUniqueFileName(TVTest::String *pFileName,size_t MaxLength=MAX_PATH-1,
						LPCTSTR pszNumberFormat=NULL);
bool GetAbsolutePath(LPCTSTR pszFilePath,LPTSTR pszAbsolutePath,int MaxLength);

HICON CreateIconFromBitmap(HBITMAP hbm,int IconWidth,int IconHeight,int ImageWidth=0,int ImageHeight=0);
bool SaveIconFromBitmap(LPCTSTR pszFileName,HBITMAP hbm,
						int IconWidth,int IconHeight,int ImageWidth=0,int ImageHeight=0);
HICON CreateEmptyIcon(int Width,int Height);
enum IconSizeType {
	ICON_SIZE_SMALL,
	ICON_SIZE_NORMAL
};
HICON LoadIconStandardSize(HINSTANCE hinst,LPCTSTR pszName,IconSizeType Size);
HICON LoadSystemIcon(LPCTSTR pszName,IconSizeType Size);
HICON LoadSystemIcon(LPCTSTR pszName,int Width,int Height);

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
	bool Create(LPCTSTR pszName,bool fInheritHandle=false);
	bool Open(LPCTSTR pszName,
			  DWORD DesiredAccess=MUTEX_ALL_ACCESS,
			  bool fInheritHandle=false);
	bool Wait(DWORD Timeout=INFINITE);
	void Close();
	void Release();
};

class CBasicSecurityAttributes : public SECURITY_ATTRIBUTES
{
public:
	CBasicSecurityAttributes();
	bool Initialize();

private:
	SECURITY_DESCRIPTOR m_SecurityDescriptor;

	CBasicSecurityAttributes(const CBasicSecurityAttributes &) /* = delete */;
	CBasicSecurityAttributes &operator=(const CBasicSecurityAttributes &) /* = delete */;
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

#ifdef WIN_XP_SUPPORT
	typedef DWORD TickCountType;
	inline TickCountType GetTickCount() { return ::GetTickCount(); }
#else
	typedef ULONGLONG TickCountType;
	inline TickCountType GetTickCount() { return ::GetTickCount64(); }
#endif

	namespace OS
	{

		bool IsWindowsXP();
		bool IsWindowsVista();
		bool IsWindows7();
		bool IsWindows8();
		bool IsWindowsXPOrLater();
		bool IsWindowsVistaOrLater();
		bool IsWindows7OrLater();
		bool IsWindows8OrLater();

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
