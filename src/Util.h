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


#ifndef TVTEST_UTIL_H
#define TVTEST_UTIL_H


#include "StringFormat.h"
#include "StringUtility.h"
#include "PathUtil.h"


namespace TVTest
{

	inline bool operator==(const RECT &rc1, const RECT &rc2) {
		return rc1.left == rc2.left && rc1.top == rc2.top && rc1.right == rc2.right && rc1.bottom == rc2.bottom;
	}
	inline bool operator!=(const RECT &rc1, const RECT &rc2) { return !(rc1 == rc2); }
	inline bool operator==(const POINT &pt1, const POINT &pt2) { return pt1.x == pt2.x && pt1.y == pt2.y; }
	inline bool operator!=(const POINT &pt1, const POINT &pt2) { return !(pt1 == pt2); }

	int HexCharToInt(TCHAR Code);
	unsigned int HexStringToUInt(LPCTSTR pszString, int Length, LPCTSTR *ppszEnd = nullptr);

	bool IsRectIntersect(const RECT *pRect1, const RECT *pRect2);

	float LevelToDeciBel(int Level);

	COLORREF MixColor(COLORREF Color1, COLORREF Color2, BYTE Ratio = 128);
	COLORREF HSVToRGB(double Hue, double Saturation, double Value);
	void RGBToHSV(
		BYTE Red, BYTE Green, BYTE Blue,
		double *pHue, double *pSaturation, double *pValue);

	inline DWORD TickTimeSpan(DWORD Start, DWORD End) { return End - Start; }
	inline ULONGLONG TickTimeSpan(ULONGLONG Start, ULONGLONG End) { _ASSERT(Start <= End); return End - Start; }

	constexpr LONGLONG FILETIME_MILLISECOND = 10000LL;
	constexpr LONGLONG FILETIME_SECOND      = 1000LL * FILETIME_MILLISECOND;
	constexpr LONGLONG FILETIME_MINUTE      = 60LL * FILETIME_SECOND;
	constexpr LONGLONG FILETIME_HOUR        = 60LL*FILETIME_MINUTE;
	FILETIME &operator+=(FILETIME &ft, LONGLONG Offset);
	LONGLONG operator-(const FILETIME &ft1, const FILETIME &ft2);
	namespace TimeConsts {
		constexpr LONGLONG SYSTEMTIME_SECOND = 1000LL;
		constexpr LONGLONG SYSTEMTIME_MINUTE = 60LL * SYSTEMTIME_SECOND;
		constexpr LONGLONG SYSTEMTIME_HOUR   = 60LL * SYSTEMTIME_MINUTE;
		constexpr LONGLONG SYSTEMTIME_DAY    = 24LL * SYSTEMTIME_HOUR;
	}
	int CompareSystemTime(const SYSTEMTIME *pTime1, const SYSTEMTIME *pTime2);
	bool OffsetSystemTime(SYSTEMTIME *pTime, LONGLONG Offset);
	LONGLONG DiffSystemTime(const SYSTEMTIME *pStartTime, const SYSTEMTIME *pEndTime);
	void GetLocalTimeAsFileTime(FILETIME *pTime);
	void SystemTimeTruncateDay(SYSTEMTIME *pTime);
	void SystemTimeTruncateHour(SYSTEMTIME *pTime);
	void SystemTimeTruncateMinute(SYSTEMTIME *pTime);
	void SystemTimeTruncateSecond(SYSTEMTIME *pTime);
	bool GetJSTTimeZoneInformation(TIME_ZONE_INFORMATION *pInfo);
	int CalcDayOfWeek(int Year, int Month, int Day);
	LPCTSTR GetDayOfWeekText(int DayOfWeek);

	bool CopyTextToClipboard(HWND hwndOwner, LPCTSTR pszText);

	void ClearMenu(HMENU hmenu);
	int CopyToMenuText(LPCTSTR pszSrcText, LPTSTR pszDstText, int MaxLength);
	String FormatMenuString(const String &Str);
	String FormatMenuString(LPCWSTR pszText);

	void InitOpenFileName(OPENFILENAME *pofn);
	bool FileOpenDialog(OPENFILENAME *pofn);
	bool FileSaveDialog(OPENFILENAME *pofn);

	void ForegroundWindow(HWND hwnd);

	bool ChooseColorDialog(HWND hwndOwner, COLORREF *pcrResult);
	bool ChooseFontDialog(HWND hwndOwner, LOGFONT *plf, int *pPointSize = nullptr);
	bool BrowseFolderDialog(HWND hwndOwner, LPTSTR pszDirectory, LPCTSTR pszTitle);

	bool CompareLogFont(const LOGFONT *pFont1, const LOGFONT *pFont2);
	int PixelsToPoints(int Pixels);
	int PointsToPixels(int Points);
	int CalcFontPointHeight(HDC hdc, const LOGFONT *pFont);

	int GetErrorText(DWORD ErrorCode, LPTSTR pszText, int MaxLength);

	bool IsEqualFileName(LPCWSTR pszFileName1, LPCWSTR pszFileName2);
	enum class FileNameValidateFlag : unsigned int {
		None           = 0x0000U,
		Wildcard       = 0x0001U,
		AllowDelimiter = 0x0002U,
		TVTEST_ENUM_FLAGS_TRAILER
	};
	bool IsValidFileName(LPCTSTR pszFileName, FileNameValidateFlag Flags = FileNameValidateFlag::None, String *pMessage = nullptr);
	bool MakeUniqueFileName(
		String *pFileName, size_t MaxLength = MAX_PATH - 1,
		LPCTSTR pszNumberFormat = nullptr);
	bool GetAbsolutePath(LPCTSTR pszFilePath, LPTSTR pszAbsolutePath, int MaxLength);
	bool GetAbsolutePath(const String &FilePath, String *pAbsolutePath);

	HICON CreateIconFromBitmap(HBITMAP hbm, int IconWidth, int IconHeight, int ImageWidth = 0, int ImageHeight = 0);
	bool SaveIconFromBitmap(
		LPCTSTR pszFileName, HBITMAP hbm,
		int IconWidth, int IconHeight, int ImageWidth = 0, int ImageHeight = 0);
	HICON CreateEmptyIcon(int Width, int Height, int BitsPerPixel = 1);
	enum class IconSizeType {
		Small,
		Normal,
	};
	bool GetStandardIconSize(IconSizeType Size, int *pWidth, int *pHeight);
	HICON LoadIconStandardSize(HINSTANCE hinst, LPCTSTR pszName, IconSizeType Size);
	HICON LoadIconSpecificSize(HINSTANCE hinst, LPCTSTR pszName, int Width, int Height);
	HICON LoadSystemIcon(LPCTSTR pszName, IconSizeType Size);
	HICON LoadSystemIcon(LPCTSTR pszName, int Width, int Height);

	class CStaticStringFormatter
	{
	public:
		CStaticStringFormatter(LPTSTR pBuffer, size_t BufferLength);
		size_t Length() const { return m_Length; }
		bool IsEmpty() const { return m_Length == 0; }
		LPCTSTR GetString() const { return m_pBuffer; }
		void Clear();
		void Append(LPCTSTR pszString);
		template<typename... TArgs> void AppendFormat(StringView Format, TArgs&&... Args)
		{
			AppendFormatV(Format, MakeFormatArgs(Args...));
		}
		void AppendFormatV(StringView Format, FormatArgs Args);
		void RemoveTrailingWhitespace();

	private:
		const LPTSTR m_pBuffer;
		const size_t m_BufferLength;
		size_t m_Length = 0;
	};

	class CGlobalLock
	{
		HANDLE m_hMutex = nullptr;
		bool m_fOwner = false;

	public:
		CGlobalLock() = default;
		~CGlobalLock();

		CGlobalLock(const CGlobalLock &) = delete;
		CGlobalLock &operator=(const CGlobalLock &) = delete;

		bool Create(LPCTSTR pszName, bool fInitialOwner = false);
		bool Open(
			LPCTSTR pszName,
			DWORD DesiredAccess = MUTEX_ALL_ACCESS,
			bool fInheritHandle = false);
		bool Wait(DWORD Timeout = INFINITE);
		void Close();
		void Release();
	};

	class CSemaphore
	{
	public:
		CSemaphore() = default;
		~CSemaphore();

		CSemaphore(const CSemaphore &) = delete;
		CSemaphore &operator=(const CSemaphore &) = delete;

		bool Create(LONG InitialCount, LONG MaxCount, LPCTSTR pszName);
		bool Open(
			LPCTSTR pszName,
			DWORD DesiredAccess = SEMAPHORE_ALL_ACCESS,
			bool fInheritHandle = false);
		bool Wait(DWORD Timeout = INFINITE);
		void Close();
		LONG Release(LONG Count = 1);
		LONG GetMaxCount() const { return m_MaxCount; }

	private:
		HANDLE m_hSemaphore = nullptr;
		LONG m_MaxCount = 0;
	};

	class CInterprocessReadWriteLock
	{
	public:
		CInterprocessReadWriteLock() = default;
		~CInterprocessReadWriteLock() = default;

		CInterprocessReadWriteLock(const CInterprocessReadWriteLock &) = delete;
		CInterprocessReadWriteLock &operator=(const CInterprocessReadWriteLock &) = delete;

		bool Create(LPCTSTR pszLockName, LPCTSTR pszSemaphoreName, LONG MaxCount);
		bool LockRead(DWORD Timeout = INFINITE);
		void UnlockRead();
		bool LockWrite(DWORD Timeout = INFINITE);
		void UnlockWrite();

	private:
		CGlobalLock m_Lock;
		CSemaphore m_Semaphore;
	};

	class CBasicSecurityAttributes
		: public SECURITY_ATTRIBUTES
	{
	public:
		CBasicSecurityAttributes();

		CBasicSecurityAttributes(const CBasicSecurityAttributes &) = delete;
		CBasicSecurityAttributes &operator=(const CBasicSecurityAttributes &) = delete;

		bool Initialize();

	private:
		SECURITY_DESCRIPTOR m_SecurityDescriptor;
	};

	namespace Util
	{

		template<typename T> T *GetLibraryFunction(HMODULE hLib, LPCSTR pszFunc)
		{
			return reinterpret_cast<T*>(::GetProcAddress(hLib, pszFunc));
		}

		template<typename T> T *GetLibraryFunction(HMODULE hLib, int Oridnal)
		{
			return reinterpret_cast<T*>(::GetProcAddress(hLib, MAKEINTRESOURCEA(Oridnal)));
		}

		template<typename T> T *GetLibraryFunction(HMODULE hLib, LPCSTR pszFunc, int Ordinal)
		{
			T *pFunc = GetLibraryFunction<T>(hLib, pszFunc);
			if (pFunc == nullptr)
				pFunc = GetLibraryFunction<T>(hLib, Ordinal);
			return pFunc;
		}

		template<typename T> T *GetModuleFunction(LPCTSTR pszModule, LPCSTR pszFunc)
		{
			return GetLibraryFunction<T>(::GetModuleHandle(pszModule), pszFunc);
		}

		template<typename T> T *GetModuleFunction(LPCTSTR pszModule, int Ordinal)
		{
			return GetLibraryFunction<T>(::GetModuleHandle(pszModule), Ordinal);
		}

		template<typename T> T *GetModuleFunction(LPCTSTR pszModule, LPCSTR pszFunc, int Ordinal)
		{
			return GetLibraryFunction<T>(::GetModuleHandle(pszModule), pszFunc, Ordinal);
		}

#define GET_LIBRARY_FUNCTION(hLib,Func) \
	Util::GetLibraryFunction<decltype(Func)>(hLib,#Func)
#define GET_LIBRARY_FUNCTION_ORDINAL(hLib,Func,Ordinal) \
	Util::GetLibraryFunction<decltype(Func)>(hLib,#Func,Ordinal)
#define GET_MODULE_FUNCTION(pszModule,Func) \
	Util::GetModuleFunction<decltype(Func)>(pszModule,#Func)
#define GET_MODULE_FUNCTION_ORDINAL(pszModule,Func,Ordinal) \
	Util::GetModuleFunction<decltype(Func)>(pszModule,#Func,Ordinal)

		HMODULE LoadSystemLibrary(LPCTSTR pszName);

		typedef ULONGLONG TickCountType;
		inline TickCountType GetTickCount() { return ::GetTickCount64(); }

		namespace OS
		{

			bool IsWindowsXP();
			bool IsWindowsVista();
			bool IsWindows7();
			bool IsWindows8();
			bool IsWindows8_1();
			bool IsWindows10();
			bool IsWindowsXPOrLater();
			bool IsWindowsVistaOrLater();
			bool IsWindows7OrLater();
			bool IsWindows8OrLater();
			bool IsWindows8_1OrLater();
			bool IsWindows10OrLater();
			bool IsWindows10AnniversaryUpdateOrLater();
			bool IsWindows10CreatorsUpdateOrLater();
			bool IsWindows10RS5OrLater();
			bool IsWindows10_19H1OrLater();
			bool IsWindows10_20H1OrLater();
			bool IsWindows11();
			bool IsWindows11OrLater();

		}	// namespace OS

		class CClock
		{
		public:
			void Start() { m_Clock = ::GetTickCount(); }
			DWORD GetSpan() const { return TickTimeSpan(m_Clock, ::GetTickCount()); }

		private:
			DWORD m_Clock;
		};

		class CWaitCursor
		{
		public:
			CWaitCursor()
				: m_hcurOld(::SetCursor(::LoadCursor(nullptr, IDC_WAIT)))
			{
			}

			~CWaitCursor()
			{
				::SetCursor(m_hcurOld);
			}

		private:
			HCURSOR m_hcurOld;
		};

		class CTimer
		{
		public:
			virtual ~CTimer();
			bool Begin(DWORD DueTime, DWORD Period);
			void End();

		protected:
			virtual void OnTimer() = 0;

		private:
			static void CALLBACK TimerCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired);

			HANDLE m_hTimer = nullptr;
		};

		template<typename T, std::size_t N> class CTempBuffer
		{
		public:
			CTempBuffer() : m_pBuffer(m_FixedBuffer) {}
			CTempBuffer(std::size_t Elements) { Allocate_(Elements); }
			~CTempBuffer() { Free(); }
			T &operator[](std::size_t i) { return m_pBuffer[i]; }
			const T &operator[](std::size_t i) const { return m_pBuffer[i]; }
			void Allocate(std::size_t Elements)
			{
				Free();
				Allocate_(Elements);
			}
			void Free()
			{
				if (m_pBuffer != m_FixedBuffer) {
					delete [] m_pBuffer;
					m_pBuffer = m_FixedBuffer;
				}
			}
			T *GetBuffer() { return m_pBuffer; }
			const T *GetBuffer() const { return m_pBuffer; }

		private:
			void Allocate_(std::size_t Elements)
			{
				if (Elements <= N) {
					m_pBuffer = m_FixedBuffer;
				} else {
					m_pBuffer = new T[Elements];
				}
			}

			T *m_pBuffer;
			T m_FixedBuffer[N];
		};

	}	// namespace Util

}	// namespace TVTest


#include "LibISDB/LibISDB/Utilities/Lock.hpp"

namespace TVTest
{

	using LibISDB::MutexLock;
	using LibISDB::BlockLock;
	using LibISDB::TryBlockLock;

}	// namespace TVTest


#endif
