// TsUtilClass.h: TSユーティリティークラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


/////////////////////////////////////////////////////////////////////////////
// ダイナミックリファレンス管理ベースクラス
/////////////////////////////////////////////////////////////////////////////

class CDynamicReferenceable
{
public:
	CDynamicReferenceable();
	virtual ~CDynamicReferenceable();

	void AddRef(void);
	void ReleaseRef(void);

private:
	LONG m_RefCount;
};


/////////////////////////////////////////////////////////////////////////////
// クリティカルセクションラッパークラス
/////////////////////////////////////////////////////////////////////////////

class CCriticalLock
{
public:
	CCriticalLock();
	virtual ~CCriticalLock();

	void Lock(void);
	void Unlock(void);
	bool TryLock(DWORD TimeOut=0);
private:
	CRITICAL_SECTION m_CriticalSection;
};


/////////////////////////////////////////////////////////////////////////////
// ブロックスコープロッククラス
/////////////////////////////////////////////////////////////////////////////

class CBlockLock
{
public:
	CBlockLock(CCriticalLock *pCriticalLock);
	virtual ~CBlockLock();
		
private:
	CCriticalLock *m_pCriticalLock;
};

class CTryBlockLock
{
public:
	CTryBlockLock(CCriticalLock *pCriticalLock);
	bool TryLock(DWORD TimeOut=0);
	~CTryBlockLock();
private:
	CCriticalLock *m_pCriticalLock;
	bool m_bLocked;
};


/////////////////////////////////////////////////////////////////////////////
// イベントクラス
/////////////////////////////////////////////////////////////////////////////

class CLocalEvent
{
	HANDLE m_hEvent;
public:
	CLocalEvent();
	~CLocalEvent();
	bool Create(bool bManual = false, bool bInitialState = false);
	bool IsCreated() const;
	void Close();
	bool Set();
	bool Reset();
	DWORD Wait(DWORD Timeout = INFINITE);
	DWORD SignalAndWait(HANDLE hHandle, DWORD Timeout = INFINITE, bool bAlertable = false);
	DWORD SignalAndWait(CLocalEvent *pEvent, DWORD Timeout = INFINITE);
	bool IsSignaled();
};


/////////////////////////////////////////////////////////////////////////////
// 日時クラス
/////////////////////////////////////////////////////////////////////////////

class CDateTime
{
	SYSTEMTIME m_Time;
public:
	CDateTime();
	CDateTime(const SYSTEMTIME &Time);
	CDateTime &operator=(const SYSTEMTIME &Time);
	CDateTime &operator=(const FILETIME &Time);
	void LocalTime();
	void UTCTime();
	//bool LocalToUTC();
	//bool UTCToLocal();
	bool Offset(LONGLONG Milliseconds);
	void Set(const SYSTEMTIME &Time) { m_Time = Time; }
	const SYSTEMTIME &Get() const { return m_Time; }
	void Get(SYSTEMTIME *pTime) const;
	int GetYear() const { return m_Time.wYear; }
	int GetMonth() const { return m_Time.wMonth; }
	int GetDay() const { return m_Time.wDay; }
	int GetDayOfWeek() const { return m_Time.wDayOfWeek; }
	int GetHour() const { return m_Time.wHour; }
	int GetMinute() const { return m_Time.wMinute; }
	int GetSecond() const { return m_Time.wSecond; }
	DWORD GetMilliseconds() const { return m_Time.wMilliseconds; }
	static LONGLONG SECONDS(int Sec) { return Sec * 1000LL; }
	static LONGLONG MINUTES(int Min) { return Min * (1000LL * 60LL); }
	static LONGLONG HOURS(int Hours) { return Hours * (1000LL * 60LL * 60LL); }
};


/////////////////////////////////////////////////////////////////////////////
// トレースクラス
/////////////////////////////////////////////////////////////////////////////

class CTracer
{
public:
	enum TraceType {
		TYPE_INFORMATION,
		TYPE_WARNING,
		TYPE_ERROR
	};

	virtual ~CTracer() {}
	void Trace(TraceType Type, LPCTSTR pszOutput, ...);
	void TraceV(TraceType Type, LPCTSTR pszOutput, va_list Args);

protected:
	virtual void OnTrace(TraceType Type, LPCTSTR pszOutput) = 0;
};


/////////////////////////////////////////////////////////////////////////////
// CRC計算クラス
/////////////////////////////////////////////////////////////////////////////

class CCrcCalculator
{
public:
	static WORD CalcCrc16(const BYTE *pData, SIZE_T DataSize, WORD wCurCrc = 0xFFFF);
	static DWORD CalcCrc32(const BYTE *pData, SIZE_T DataSize, DWORD dwCurCrc = 0xFFFFFFFFUL);
};

class CCrc32
{
	DWORD m_Crc;
public:
	CCrc32();
	DWORD GetCrc() const;
	void Calc(const void *pData, SIZE_T DataSize);
	void Reset();
};


/////////////////////////////////////////////////////////////////////////////
// MD5計算クラス
/////////////////////////////////////////////////////////////////////////////

class CMD5Calculator
{
	static void MD5Transform(DWORD pBuffer[4], const void *pData);
public:
	static void CalcMD5(const void *pData, SIZE_T DataSize, BYTE pMD5[16]);
};


/////////////////////////////////////////////////////////////////////////////
// ビットレート計算クラス
/////////////////////////////////////////////////////////////////////////////

class CBitRateCalculator
{
	DWORD m_Time;
	SIZE_T m_Size;
	DWORD m_BitRate;
public:
	CBitRateCalculator();
	void Initialize();
	void Reset();
	bool Update(SIZE_T Size);
	DWORD GetBitRate() const { return m_BitRate; }
};


/////////////////////////////////////////////////////////////////////////////
// カウンタークラス
/////////////////////////////////////////////////////////////////////////////

template<typename T> class CCounter32
{
public:
	CCounter32() : m_Counter(0) {}
	void Reset() { ::InterlockedExchange((LONG*)&m_Counter, 0); }
	T Increment() { return ::InterlockedIncrement((LONG*)&m_Counter); }
	T Decrement() { return ::InterlockedDecrement((LONG*)&m_Counter); }
	T Get() const { return m_Counter; }

private:
	T m_Counter;
};

template<typename T> class CCounter64
{
public:
	CCounter64() : m_Counter(0) {}

#ifdef _WIN64
	void Reset() { ::_InterlockedExchange64((__int64*)&m_Counter, 0); }
	T Increment() { return ::_InterlockedIncrement64((__int64*)&m_Counter); }
	T Decrement() { return ::_InterlockedDecrement64((__int64*)&m_Counter); }
	T Get() const { return m_Counter; }
#else
	void Reset() {
		m_Lock.Lock();
		m_Counter = 0;
		m_Lock.Unlock();
	}
	T Increment() {
		m_Lock.Lock();
		T Count = ++m_Counter;
		m_Lock.Unlock();
		return Count;
	}
	T Decrement() {
		m_Lock.Lock();
		T Count = --m_Counter;
		m_Lock.Unlock();
		return Count;
	}
	T Get() const {
		m_Lock.Lock();
		UINT64 Count = m_Counter;
		m_Lock.Unlock();
		return Count;
	}
#endif

private:
	T m_Counter;
#ifndef _WIN64
	mutable CCriticalLock m_Lock;
#endif
};

typedef CCounter32<INT32> CInt32Counter;
typedef CCounter32<UINT32> CUInt32Counter;
typedef CCounter64<INT64> CInt64Counter;
typedef CCounter64<UINT64> CUInt64Counter;
