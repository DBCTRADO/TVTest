// TsUtilClass.cpp: TSユーティリティークラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TsUtilClass.h"
#include "StdUtil.h"
#include "../Common/DebugDef.h"


//////////////////////////////////////////////////////////////////////
// CDynamicReferenceable クラスの構築/消滅
//////////////////////////////////////////////////////////////////////

CDynamicReferenceable::CDynamicReferenceable()
	: m_RefCount(0)
{

}

CDynamicReferenceable::~CDynamicReferenceable()
{

}

void CDynamicReferenceable::AddRef(void)
{
	// 参照カウントインクリメント
	::InterlockedIncrement(&m_RefCount);
}

void CDynamicReferenceable::ReleaseRef(void)
{
	// 参照カウントデクリメント
#ifndef _DEBUG
	if (::InterlockedDecrement(&m_RefCount) == 0) {
		// インスタンス開放
		delete this;
	}
#else
	const LONG Count = ::InterlockedDecrement(&m_RefCount);
	if (Count == 0) {
		// インスタンス開放
		delete this;
	} else if (Count < 0) {
		::DebugBreak();
	}
#endif
}


//////////////////////////////////////////////////////////////////////
// CCriticalLock クラスの構築/消滅
//////////////////////////////////////////////////////////////////////

CCriticalLock::CCriticalLock()
{
	// クリティカルセクション初期化
	::InitializeCriticalSection(&m_CriticalSection);
}

CCriticalLock::~CCriticalLock()
{
	// クリティカルセクション削除
	::DeleteCriticalSection(&m_CriticalSection);
}

void CCriticalLock::Lock(void)
{
	// クリティカルセクション取得
	::EnterCriticalSection(&m_CriticalSection);
}

void CCriticalLock::Unlock(void)
{
	// クリティカルセクション開放
	::LeaveCriticalSection(&m_CriticalSection);
}

bool CCriticalLock::TryLock(DWORD TimeOut)
{
	bool bLocked = false;

	if (TimeOut == 0) {
		if (::TryEnterCriticalSection(&m_CriticalSection))
			bLocked = true;
	} else {
		// こういうのは無理に Critical section 使わない方がいいかも...
		const DWORD StartTime = ::GetTickCount();
		while (true) {
			if (::TryEnterCriticalSection(&m_CriticalSection)) {
				bLocked = true;
				break;
			}
			if (::GetTickCount() - StartTime >= TimeOut)
				break;
			::Sleep(1);
		};
	}
	return bLocked;
}


//////////////////////////////////////////////////////////////////////
// CBlockLock クラスの構築/消滅
//////////////////////////////////////////////////////////////////////

CBlockLock::CBlockLock(CCriticalLock *pCriticalLock)
	: m_pCriticalLock(pCriticalLock)
{
	// ロック取得
	m_pCriticalLock->Lock();
}

CBlockLock::~CBlockLock()
{
	// ロック開放
	m_pCriticalLock->Unlock();
}


CTryBlockLock::CTryBlockLock(CCriticalLock *pCriticalLock)
	: m_pCriticalLock(pCriticalLock)
	, m_bLocked(false)
{
}

CTryBlockLock::~CTryBlockLock()
{
	if (m_bLocked)
		m_pCriticalLock->Unlock();
}

bool CTryBlockLock::TryLock(DWORD TimeOut)
{
	if (m_pCriticalLock->TryLock(TimeOut))
		m_bLocked=true;
	return m_bLocked;
}


/////////////////////////////////////////////////////////////////////////////
// イベントクラス
/////////////////////////////////////////////////////////////////////////////

CLocalEvent::CLocalEvent()
	: m_hEvent(NULL)
{
}

CLocalEvent::~CLocalEvent()
{
	Close();
}

bool CLocalEvent::Create(bool bManual, bool bInitialState)
{
	if (m_hEvent)
		return false;
	m_hEvent = ::CreateEvent(NULL, bManual, bInitialState, NULL);
	return m_hEvent != NULL;
}

bool CLocalEvent::IsCreated() const
{
	return m_hEvent != NULL;
}

void CLocalEvent::Close()
{
	if (m_hEvent) {
		::CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}
}

bool CLocalEvent::Set()
{
	if (!m_hEvent)
		return false;
	return ::SetEvent(m_hEvent) != FALSE;
}

bool CLocalEvent::Reset()
{
	if (!m_hEvent)
		return false;
	return ::ResetEvent(m_hEvent) != FALSE;
}

DWORD CLocalEvent::Wait(DWORD Timeout)
{
	if (!m_hEvent)
		return WAIT_FAILED;
	return ::WaitForSingleObject(m_hEvent, Timeout);
}

DWORD CLocalEvent::SignalAndWait(HANDLE hHandle, DWORD Timeout, bool bAlertable)
{
	if (!m_hEvent)
		return WAIT_FAILED;
	return ::SignalObjectAndWait(m_hEvent, hHandle, Timeout, bAlertable);
}

DWORD CLocalEvent::SignalAndWait(CLocalEvent *pEvent, DWORD Timeout)
{
	if (!m_hEvent || !pEvent || !pEvent->m_hEvent)
		return WAIT_FAILED;
	return ::SignalObjectAndWait(m_hEvent, pEvent->m_hEvent, Timeout, FALSE);
}

bool CLocalEvent::IsSignaled()
{
	if (!m_hEvent)
		return false;
	return ::WaitForSingleObject(m_hEvent, 0) == WAIT_OBJECT_0;
}


/////////////////////////////////////////////////////////////////////////////
// 日時クラス
/////////////////////////////////////////////////////////////////////////////

CDateTime::CDateTime()
{
}

CDateTime::CDateTime(const SYSTEMTIME &Time)
{
	Set(Time);
}

CDateTime &CDateTime::operator=(const SYSTEMTIME &Time)
{
	Set(Time);
	return *this;
}

CDateTime &CDateTime::operator=(const FILETIME &Time)
{
	::FileTimeToSystemTime(&Time, &m_Time);
	return *this;
}

void CDateTime::LocalTime()
{
	::GetLocalTime(&m_Time);
}

void CDateTime::UTCTime()
{
	::GetSystemTime(&m_Time);
}

/*
bool CDateTime::LocalToUTC()
{
#ifdef WINDOWS2000_SUPPORT
	FILETIME ftLocal, ftUTC;

	return ::SystemTimeToFileTime(&m_Time, &ftLocal)
		&& ::LocalFileTimeToFileTime(&ftLocal, &ftUTC)
		&& ::FileTimeToSystemTime(&ftUTC, &m_Time);
#else
	TIME_ZONE_INFORMATION tzi;
	if (::GetTimeZoneInformation(&tzi) == TIME_ZONE_ID_INVALID)
		return false;
	tzi.StandardDate.wMonth = 0;
	tzi.DaylightDate.wMonth = 0;
	SYSTEMTIME st = m_Time;
	return ::TzSpecificLocalTimeToSystemTime(&tzi, &st, &m_Time) != FALSE;
#endif
}

bool CDateTime::UTCToLocal()
{
#if 0
	FILETIME ftUTC, ftLocal;

	return ::SystemTimeToFileTime(&m_Time, &ftUTC)
		&& ::FileTimeToLocalFileTime(&ftUTC, &ftLocal)
		&& ::FileTimeToSystemTime(&ftLocal, &m_Time);
#else
	TIME_ZONE_INFORMATION tzi;
	if (::GetTimeZoneInformation(&tzi) == TIME_ZONE_ID_INVALID)
		return false;
	tzi.StandardDate.wMonth = 0;
	tzi.DaylightDate.wMonth = 0;
	SYSTEMTIME st = m_Time;
	return ::SystemTimeToTzSpecificLocalTime(&tzi, &st, &m_Time) != FALSE;
#endif
}
*/

bool CDateTime::Offset(LONGLONG Milliseconds)
{
	FILETIME ft;
	ULARGE_INTEGER Time;

	if (!::SystemTimeToFileTime(&m_Time, &ft))
		return false;
	Time.LowPart = ft.dwLowDateTime;
	Time.HighPart = ft.dwHighDateTime;
	Time.QuadPart += Milliseconds * 10000LL;
	ft.dwLowDateTime = Time.LowPart;
	ft.dwHighDateTime = Time.HighPart;
	return ::FileTimeToSystemTime(&ft, &m_Time) != FALSE;
}

void CDateTime::Get(SYSTEMTIME *pTime) const
{
	if (pTime)
		*pTime = m_Time;
}


/////////////////////////////////////////////////////////////////////////////
// トレースクラス
/////////////////////////////////////////////////////////////////////////////

void CTracer::Trace(TraceType Type, LPCTSTR pszOutput, ...)
{
	va_list Args;

	va_start(Args,pszOutput);
	TraceV(Type,pszOutput,Args);
	va_end(Args);
}

void CTracer::TraceV(TraceType Type, LPCTSTR pszOutput, va_list Args)
{
	TCHAR szBuffer[512];

	StdUtil::vsnprintf(szBuffer,sizeof(szBuffer)/sizeof(TCHAR),pszOutput,Args);
	OnTrace(Type,szBuffer);
}


/////////////////////////////////////////////////////////////////////////////
// CRC計算クラス
/////////////////////////////////////////////////////////////////////////////

WORD CCrcCalculator::CalcCrc16(const BYTE *pData, SIZE_T DataSize, WORD wCurCrc)
{
	// CRC16計算(ISO/IEC 13818-1 準拠)
	static const WORD Crc16Table[256] = {
		0x0000U, 0x8005U, 0x800FU, 0x000AU, 0x801BU, 0x001EU, 0x0014U, 0x8011U, 0x8033U, 0x0036U, 0x003CU, 0x8039U, 0x0028U, 0x802DU, 0x8027U, 0x0022U,
		0x8063U, 0x0066U, 0x006CU, 0x8069U, 0x0078U, 0x807DU, 0x8077U, 0x0072U, 0x0050U, 0x8055U, 0x805FU, 0x005AU, 0x804BU, 0x004EU, 0x0044U, 0x8041U,
		0x80C3U, 0x00C6U, 0x00CCU, 0x80C9U, 0x00D8U, 0x80DDU, 0x80D7U, 0x00D2U, 0x00F0U, 0x80F5U, 0x80FFU, 0x00FAU, 0x80EBU, 0x00EEU, 0x00E4U, 0x80E1U,
		0x00A0U, 0x80A5U, 0x80AFU, 0x00AAU, 0x80BBU, 0x00BEU, 0x00B4U, 0x80B1U, 0x8093U, 0x0096U, 0x009CU, 0x8099U, 0x0088U, 0x808DU, 0x8087U, 0x0082U,
		0x8183U, 0x0186U, 0x018CU, 0x8189U, 0x0198U, 0x819DU, 0x8197U, 0x0192U, 0x01B0U, 0x81B5U, 0x81BFU, 0x01BAU, 0x81ABU, 0x01AEU, 0x01A4U, 0x81A1U,
		0x01E0U, 0x81E5U, 0x81EFU, 0x01EAU, 0x81FBU, 0x01FEU, 0x01F4U, 0x81F1U, 0x81D3U, 0x01D6U, 0x01DCU, 0x81D9U, 0x01C8U, 0x81CDU, 0x81C7U, 0x01C2U,
		0x0140U, 0x8145U, 0x814FU, 0x014AU, 0x815BU, 0x015EU, 0x0154U, 0x8151U, 0x8173U, 0x0176U, 0x017CU, 0x8179U, 0x0168U, 0x816DU, 0x8167U, 0x0162U,
		0x8123U, 0x0126U, 0x012CU, 0x8129U, 0x0138U, 0x813DU, 0x8137U, 0x0132U, 0x0110U, 0x8115U, 0x811FU, 0x011AU, 0x810BU, 0x010EU, 0x0104U, 0x8101U,
		0x8303U, 0x0306U, 0x030CU, 0x8309U, 0x0318U, 0x831DU, 0x8317U, 0x0312U, 0x0330U, 0x8335U, 0x833FU, 0x033AU, 0x832BU, 0x032EU, 0x0324U, 0x8321U,
		0x0360U, 0x8365U, 0x836FU, 0x036AU, 0x837BU, 0x037EU, 0x0374U, 0x8371U, 0x8353U, 0x0356U, 0x035CU, 0x8359U, 0x0348U, 0x834DU, 0x8347U, 0x0342U,
		0x03C0U, 0x83C5U, 0x83CFU, 0x03CAU, 0x83DBU, 0x03DEU, 0x03D4U, 0x83D1U, 0x83F3U, 0x03F6U, 0x03FCU, 0x83F9U, 0x03E8U, 0x83EDU, 0x83E7U, 0x03E2U,
		0x83A3U, 0x03A6U, 0x03ACU, 0x83A9U, 0x03B8U, 0x83BDU, 0x83B7U, 0x03B2U, 0x0390U, 0x8395U, 0x839FU, 0x039AU, 0x838BU, 0x038EU, 0x0384U, 0x8381U,
		0x0280U, 0x8285U, 0x828FU, 0x028AU, 0x829BU, 0x029EU, 0x0294U, 0x8291U, 0x82B3U, 0x02B6U, 0x02BCU, 0x82B9U, 0x02A8U, 0x82ADU, 0x82A7U, 0x02A2U,
		0x82E3U, 0x02E6U, 0x02ECU, 0x82E9U, 0x02F8U, 0x82FDU, 0x82F7U, 0x02F2U, 0x02D0U, 0x82D5U, 0x82DFU, 0x02DAU, 0x82CBU, 0x02CEU, 0x02C4U, 0x82C1U,
		0x8243U, 0x0246U, 0x024CU, 0x8249U, 0x0258U, 0x825DU, 0x8257U, 0x0252U, 0x0270U, 0x8275U, 0x827FU, 0x027AU, 0x826BU, 0x026EU, 0x0264U, 0x8261U,
		0x0220U, 0x8225U, 0x822FU, 0x022AU, 0x823BU, 0x023EU, 0x0234U, 0x8231U, 0x8213U, 0x0216U, 0x021CU, 0x8219U, 0x0208U, 0x820DU, 0x8207U, 0x0202U
	};

	for (SIZE_T i = 0 ; i < DataSize ; i++) {
		wCurCrc = (wCurCrc << 8) ^ Crc16Table[ (wCurCrc >> 8) ^ pData[i] ];
	}

	return wCurCrc;
}

static const DWORD g_Crc32Table[256] = {
	0x00000000UL, 0x04C11DB7UL, 0x09823B6EUL, 0x0D4326D9UL, 0x130476DCUL, 0x17C56B6BUL, 0x1A864DB2UL, 0x1E475005UL,	0x2608EDB8UL, 0x22C9F00FUL, 0x2F8AD6D6UL, 0x2B4BCB61UL,	0x350C9B64UL, 0x31CD86D3UL, 0x3C8EA00AUL, 0x384FBDBDUL,
	0x4C11DB70UL, 0x48D0C6C7UL, 0x4593E01EUL, 0x4152FDA9UL, 0x5F15ADACUL, 0x5BD4B01BUL, 0x569796C2UL, 0x52568B75UL,	0x6A1936C8UL, 0x6ED82B7FUL, 0x639B0DA6UL, 0x675A1011UL,	0x791D4014UL, 0x7DDC5DA3UL, 0x709F7B7AUL, 0x745E66CDUL,
	0x9823B6E0UL, 0x9CE2AB57UL, 0x91A18D8EUL, 0x95609039UL, 0x8B27C03CUL, 0x8FE6DD8BUL, 0x82A5FB52UL, 0x8664E6E5UL,	0xBE2B5B58UL, 0xBAEA46EFUL, 0xB7A96036UL, 0xB3687D81UL,	0xAD2F2D84UL, 0xA9EE3033UL, 0xA4AD16EAUL, 0xA06C0B5DUL,
	0xD4326D90UL, 0xD0F37027UL, 0xDDB056FEUL, 0xD9714B49UL, 0xC7361B4CUL, 0xC3F706FBUL, 0xCEB42022UL, 0xCA753D95UL,	0xF23A8028UL, 0xF6FB9D9FUL, 0xFBB8BB46UL, 0xFF79A6F1UL,	0xE13EF6F4UL, 0xE5FFEB43UL, 0xE8BCCD9AUL, 0xEC7DD02DUL,
	0x34867077UL, 0x30476DC0UL, 0x3D044B19UL, 0x39C556AEUL, 0x278206ABUL, 0x23431B1CUL, 0x2E003DC5UL, 0x2AC12072UL,	0x128E9DCFUL, 0x164F8078UL, 0x1B0CA6A1UL, 0x1FCDBB16UL,	0x018AEB13UL, 0x054BF6A4UL, 0x0808D07DUL, 0x0CC9CDCAUL,
	0x7897AB07UL, 0x7C56B6B0UL, 0x71159069UL, 0x75D48DDEUL, 0x6B93DDDBUL, 0x6F52C06CUL, 0x6211E6B5UL, 0x66D0FB02UL,	0x5E9F46BFUL, 0x5A5E5B08UL, 0x571D7DD1UL, 0x53DC6066UL,	0x4D9B3063UL, 0x495A2DD4UL, 0x44190B0DUL, 0x40D816BAUL,
	0xACA5C697UL, 0xA864DB20UL, 0xA527FDF9UL, 0xA1E6E04EUL, 0xBFA1B04BUL, 0xBB60ADFCUL, 0xB6238B25UL, 0xB2E29692UL,	0x8AAD2B2FUL, 0x8E6C3698UL, 0x832F1041UL, 0x87EE0DF6UL,	0x99A95DF3UL, 0x9D684044UL, 0x902B669DUL, 0x94EA7B2AUL,
	0xE0B41DE7UL, 0xE4750050UL, 0xE9362689UL, 0xEDF73B3EUL, 0xF3B06B3BUL, 0xF771768CUL, 0xFA325055UL, 0xFEF34DE2UL,	0xC6BCF05FUL, 0xC27DEDE8UL, 0xCF3ECB31UL, 0xCBFFD686UL,	0xD5B88683UL, 0xD1799B34UL, 0xDC3ABDEDUL, 0xD8FBA05AUL,
	0x690CE0EEUL, 0x6DCDFD59UL, 0x608EDB80UL, 0x644FC637UL, 0x7A089632UL, 0x7EC98B85UL, 0x738AAD5CUL, 0x774BB0EBUL,	0x4F040D56UL, 0x4BC510E1UL, 0x46863638UL, 0x42472B8FUL,	0x5C007B8AUL, 0x58C1663DUL, 0x558240E4UL, 0x51435D53UL,
	0x251D3B9EUL, 0x21DC2629UL, 0x2C9F00F0UL, 0x285E1D47UL, 0x36194D42UL, 0x32D850F5UL, 0x3F9B762CUL, 0x3B5A6B9BUL,	0x0315D626UL, 0x07D4CB91UL, 0x0A97ED48UL, 0x0E56F0FFUL,	0x1011A0FAUL, 0x14D0BD4DUL, 0x19939B94UL, 0x1D528623UL,
	0xF12F560EUL, 0xF5EE4BB9UL, 0xF8AD6D60UL, 0xFC6C70D7UL, 0xE22B20D2UL, 0xE6EA3D65UL, 0xEBA91BBCUL, 0xEF68060BUL,	0xD727BBB6UL, 0xD3E6A601UL, 0xDEA580D8UL, 0xDA649D6FUL,	0xC423CD6AUL, 0xC0E2D0DDUL, 0xCDA1F604UL, 0xC960EBB3UL,
	0xBD3E8D7EUL, 0xB9FF90C9UL, 0xB4BCB610UL, 0xB07DABA7UL, 0xAE3AFBA2UL, 0xAAFBE615UL, 0xA7B8C0CCUL, 0xA379DD7BUL,	0x9B3660C6UL, 0x9FF77D71UL, 0x92B45BA8UL, 0x9675461FUL,	0x8832161AUL, 0x8CF30BADUL, 0x81B02D74UL, 0x857130C3UL,
	0x5D8A9099UL, 0x594B8D2EUL, 0x5408ABF7UL, 0x50C9B640UL, 0x4E8EE645UL, 0x4A4FFBF2UL, 0x470CDD2BUL, 0x43CDC09CUL,	0x7B827D21UL, 0x7F436096UL, 0x7200464FUL, 0x76C15BF8UL,	0x68860BFDUL, 0x6C47164AUL, 0x61043093UL, 0x65C52D24UL,
	0x119B4BE9UL, 0x155A565EUL, 0x18197087UL, 0x1CD86D30UL, 0x029F3D35UL, 0x065E2082UL, 0x0B1D065BUL, 0x0FDC1BECUL,	0x3793A651UL, 0x3352BBE6UL, 0x3E119D3FUL, 0x3AD08088UL,	0x2497D08DUL, 0x2056CD3AUL, 0x2D15EBE3UL, 0x29D4F654UL,
	0xC5A92679UL, 0xC1683BCEUL, 0xCC2B1D17UL, 0xC8EA00A0UL, 0xD6AD50A5UL, 0xD26C4D12UL, 0xDF2F6BCBUL, 0xDBEE767CUL,	0xE3A1CBC1UL, 0xE760D676UL, 0xEA23F0AFUL, 0xEEE2ED18UL,	0xF0A5BD1DUL, 0xF464A0AAUL, 0xF9278673UL, 0xFDE69BC4UL,
	0x89B8FD09UL, 0x8D79E0BEUL, 0x803AC667UL, 0x84FBDBD0UL, 0x9ABC8BD5UL, 0x9E7D9662UL, 0x933EB0BBUL, 0x97FFAD0CUL,	0xAFB010B1UL, 0xAB710D06UL, 0xA6322BDFUL, 0xA2F33668UL,	0xBCB4666DUL, 0xB8757BDAUL, 0xB5365D03UL, 0xB1F740B4UL
};

#if 0

// Dilip V. Sarwate のアルゴリズム

DWORD CCrcCalculator::CalcCrc32(const BYTE *pData, SIZE_T DataSize, DWORD dwCurCrc)
{
	for (SIZE_T i = 0 ; i < DataSize ; i++) {
		dwCurCrc = (dwCurCrc << 8) ^ g_Crc32Table[ (dwCurCrc >> 24) ^ pData[i] ];
	}

	return dwCurCrc;
}

#else

// Slicing-by-4/8 アルゴリズム

#define CRC_SLICING_BY_4	// Slicing-by-4

#ifndef CRC_SLICING_BY_4
#define CRC_SLICING_COUNT 8
#else
#define CRC_SLICING_COUNT 4
#endif

static DWORD g_Crc32SlicingTable[CRC_SLICING_COUNT][256];

class CCrcSlicingTableInitializer
{
public:
	CCrcSlicingTableInitializer()
	{
		// Slicing-by-4/8 用テーブルの初期化
		for (size_t i = 0; i < 256; i++) {
			DWORD c = g_Crc32Table[i];
			g_Crc32SlicingTable[0][i] = c;
			for (size_t j = 1; j < CRC_SLICING_COUNT; j++) {
				c = (c << 8) ^ g_Crc32Table[c >> 24];
				g_Crc32SlicingTable[j][i] = c;
			}
		}
	}
};

static CCrcSlicingTableInitializer g_CrcSlicingTableInitializer;

#pragma intrinsic(_byteswap_ulong)
DWORD CCrcCalculator::CalcCrc32(const BYTE *pData, SIZE_T DataSize, DWORD dwCurCrc)
{
	const BYTE *p = pData;
	const BYTE *pEnd = p;

	if (DataSize >= 8) {
		const SIZE_T Align = (sizeof(DWORD) - reinterpret_cast<SIZE_T>(p)) & 3;
		DataSize -= Align;
		pEnd += Align;
		while (p < pEnd)
			dwCurCrc = (dwCurCrc << 8) ^ g_Crc32SlicingTable[0][(dwCurCrc >> 24) ^ *p++];

#ifndef CRC_SLICING_BY_4
		// Slicing-by-8
		pEnd += DataSize & ~(SIZE_T)7;
		while (p < pEnd) {
			dwCurCrc ^= _byteswap_ulong(*reinterpret_cast<const DWORD*>(p));
			p += sizeof(DWORD);
			const DWORD dwNext = *reinterpret_cast<const DWORD*>(p);
			p += sizeof(DWORD);
			dwCurCrc =
				g_Crc32SlicingTable[7][(dwCurCrc >> 24)] ^
				g_Crc32SlicingTable[6][(dwCurCrc >> 16) & 0xFF] ^
				g_Crc32SlicingTable[5][(dwCurCrc >>  8) & 0xFF] ^
				g_Crc32SlicingTable[4][(dwCurCrc      ) & 0xFF] ^
				g_Crc32SlicingTable[3][(dwNext        ) & 0xFF] ^
				g_Crc32SlicingTable[2][(dwNext   >>  8) & 0xFF] ^
				g_Crc32SlicingTable[1][(dwNext   >> 16) & 0xFF] ^
				g_Crc32SlicingTable[0][(dwNext   >> 24)];
		}

		DataSize &= 7;
#else
		// Slicing-by-4
		pEnd += DataSize & ~(SIZE_T)3;
		while (p < pEnd) {
			dwCurCrc ^= _byteswap_ulong(*reinterpret_cast<const DWORD*>(p));
			p += sizeof(DWORD);
			dwCurCrc =
				g_Crc32SlicingTable[0][(dwCurCrc      ) & 0xFF] ^
				g_Crc32SlicingTable[1][(dwCurCrc >>  8) & 0xFF] ^
				g_Crc32SlicingTable[2][(dwCurCrc >> 16) & 0xFF] ^
				g_Crc32SlicingTable[3][(dwCurCrc >> 24)];
		}

		DataSize &= 3;
#endif
	}

	pEnd += DataSize;
	while (p < pEnd)
		dwCurCrc = (dwCurCrc << 8) ^ g_Crc32SlicingTable[0][(dwCurCrc >> 24) ^ *p++];

	return dwCurCrc;
}

#endif


CCrc32::CCrc32()
	: m_Crc(0xFFFFFFFFUL)
{
}

DWORD CCrc32::GetCrc() const
{
	return m_Crc;
}

void CCrc32::Calc(const void *pData, SIZE_T DataSize)
{
	m_Crc = CCrcCalculator::CalcCrc32(static_cast<const BYTE*>(pData), DataSize, m_Crc);
}

void CCrc32::Reset()
{
	m_Crc = 0xFFFFFFFFUL;
}


/////////////////////////////////////////////////////////////////////////////
// MD5計算クラス
/////////////////////////////////////////////////////////////////////////////

#define F1(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) ((x) ^ (y) ^ (z))
#define F4(x, y, z) ((y) ^ ((x) | ~(z)))

#define MD5STEP(f, w, x, y, z, data, s) \
	((w) += f(x, y, z) + (data), (w) = (w) << (s) | (w) >> (32 - (s)), (w) += (x))

void CMD5Calculator::MD5Transform(DWORD pBuffer[4], const void *pData)
{
	const DWORD *p = static_cast<const DWORD*>(pData);
	DWORD a, b, c, d;

	a = pBuffer[0];
	b = pBuffer[1];
	c = pBuffer[2];
	d = pBuffer[3];

	MD5STEP(F1, a, b, c, d, p[ 0] + 0xD76AA478,  7);
	MD5STEP(F1, d, a, b, c, p[ 1] + 0xE8C7B756, 12);
	MD5STEP(F1, c, d, a, b, p[ 2] + 0x242070DB, 17);
	MD5STEP(F1, b, c, d, a, p[ 3] + 0xC1BDCEEE, 22);
	MD5STEP(F1, a, b, c, d, p[ 4] + 0xF57C0FAF,  7);
	MD5STEP(F1, d, a, b, c, p[ 5] + 0x4787C62A, 12);
	MD5STEP(F1, c, d, a, b, p[ 6] + 0xA8304613, 17);
	MD5STEP(F1, b, c, d, a, p[ 7] + 0xFD469501, 22);
	MD5STEP(F1, a, b, c, d, p[ 8] + 0x698098D8,  7);
	MD5STEP(F1, d, a, b, c, p[ 9] + 0x8B44F7AF, 12);
	MD5STEP(F1, c, d, a, b, p[10] + 0xFFFF5BB1, 17);
	MD5STEP(F1, b, c, d, a, p[11] + 0x895CD7BE, 22);
	MD5STEP(F1, a, b, c, d, p[12] + 0x6B901122,  7);
	MD5STEP(F1, d, a, b, c, p[13] + 0xFD987193, 12);
	MD5STEP(F1, c, d, a, b, p[14] + 0xA679438E, 17);
	MD5STEP(F1, b, c, d, a, p[15] + 0x49B40821, 22);

	MD5STEP(F2, a, b, c, d, p[ 1] + 0xF61E2562,  5);
	MD5STEP(F2, d, a, b, c, p[ 6] + 0xC040B340,  9);
	MD5STEP(F2, c, d, a, b, p[11] + 0x265E5A51, 14);
	MD5STEP(F2, b, c, d, a, p[ 0] + 0xE9B6C7AA, 20);
	MD5STEP(F2, a, b, c, d, p[ 5] + 0xD62F105D,  5);
	MD5STEP(F2, d, a, b, c, p[10] + 0x02441453,  9);
	MD5STEP(F2, c, d, a, b, p[15] + 0xD8A1E681, 14);
	MD5STEP(F2, b, c, d, a, p[ 4] + 0xE7D3FBC8, 20);
	MD5STEP(F2, a, b, c, d, p[ 9] + 0x21E1CDE6,  5);
	MD5STEP(F2, d, a, b, c, p[14] + 0xC33707D6,  9);
	MD5STEP(F2, c, d, a, b, p[ 3] + 0xF4D50D87, 14);
	MD5STEP(F2, b, c, d, a, p[ 8] + 0x455A14ED, 20);
	MD5STEP(F2, a, b, c, d, p[13] + 0xA9E3E905,  5);
	MD5STEP(F2, d, a, b, c, p[ 2] + 0xFCEFA3F8,  9);
	MD5STEP(F2, c, d, a, b, p[ 7] + 0x676F02D9, 14);
	MD5STEP(F2, b, c, d, a, p[12] + 0x8D2A4C8A, 20);

	MD5STEP(F3, a, b, c, d, p[ 5] + 0xFFFA3942,  4);
	MD5STEP(F3, d, a, b, c, p[ 8] + 0x8771F681, 11);
	MD5STEP(F3, c, d, a, b, p[11] + 0x6D9D6122, 16);
	MD5STEP(F3, b, c, d, a, p[14] + 0xFDE5380C, 23);
	MD5STEP(F3, a, b, c, d, p[ 1] + 0xA4BEEA44,  4);
	MD5STEP(F3, d, a, b, c, p[ 4] + 0x4BDECFA9, 11);
	MD5STEP(F3, c, d, a, b, p[ 7] + 0xF6BB4B60, 16);
	MD5STEP(F3, b, c, d, a, p[10] + 0xBEBFBC70, 23);
	MD5STEP(F3, a, b, c, d, p[13] + 0x289B7EC6,  4);
	MD5STEP(F3, d, a, b, c, p[ 0] + 0xEAA127FA, 11);
	MD5STEP(F3, c, d, a, b, p[ 3] + 0xD4EF3085, 16);
	MD5STEP(F3, b, c, d, a, p[ 6] + 0x04881D05, 23);
	MD5STEP(F3, a, b, c, d, p[ 9] + 0xD9D4D039,  4);
	MD5STEP(F3, d, a, b, c, p[12] + 0xE6DB99E5, 11);
	MD5STEP(F3, c, d, a, b, p[15] + 0x1FA27CF8, 16);
	MD5STEP(F3, b, c, d, a, p[ 2] + 0xC4AC5665, 23);

	MD5STEP(F4, a, b, c, d, p[ 0] + 0xF4292244,  6);
	MD5STEP(F4, d, a, b, c, p[ 7] + 0x432AFF97, 10);
	MD5STEP(F4, c, d, a, b, p[14] + 0xAB9423A7, 15);
	MD5STEP(F4, b, c, d, a, p[ 5] + 0xFC93A039, 21);
	MD5STEP(F4, a, b, c, d, p[12] + 0x655B59C3,  6);
	MD5STEP(F4, d, a, b, c, p[ 3] + 0x8F0CCC92, 10);
	MD5STEP(F4, c, d, a, b, p[10] + 0xFFEFF47D, 15);
	MD5STEP(F4, b, c, d, a, p[ 1] + 0x85845DD1, 21);
	MD5STEP(F4, a, b, c, d, p[ 8] + 0x6FA87E4F,  6);
	MD5STEP(F4, d, a, b, c, p[15] + 0xFE2CE6E0, 10);
	MD5STEP(F4, c, d, a, b, p[ 6] + 0xA3014314, 15);
	MD5STEP(F4, b, c, d, a, p[13] + 0x4E0811A1, 21);
	MD5STEP(F4, a, b, c, d, p[ 4] + 0xF7537E82,  6);
	MD5STEP(F4, d, a, b, c, p[11] + 0xBD3AF235, 10);
	MD5STEP(F4, c, d, a, b, p[ 2] + 0x2AD7D2BB, 15);
	MD5STEP(F4, b, c, d, a, p[ 9] + 0xEB86D391, 21);

	pBuffer[0] += a;
	pBuffer[1] += b;
	pBuffer[2] += c;
	pBuffer[3] += d;
}

void CMD5Calculator::CalcMD5(const void *pData, SIZE_T DataSize, BYTE pMD5[16])
{
	const BYTE *pSrc = static_cast<const BYTE*>(pData);
	DWORD *pdwMD5 = reinterpret_cast<DWORD*>(pMD5);
	const ULONGLONG BitsSize = (ULONGLONG)DataSize << 3;

	pdwMD5[0] = 0x67452301UL;
	pdwMD5[1] = 0xEFCDAB89UL;
	pdwMD5[2] = 0x98BADCFEUL;
	pdwMD5[3] = 0x10325476UL;

	while (DataSize >= 64) {
		MD5Transform(pdwMD5, pSrc);
		pSrc += 64;
		DataSize -= 64;
	}

	SIZE_T PaddingSize;
	BYTE PaddingData[64], *p;

	PaddingSize = DataSize & 0x3F;
	::CopyMemory(PaddingData, pSrc, DataSize);
	p = PaddingData + PaddingSize;
	*p++ = 0x80;
	PaddingSize = 64 - 1 - PaddingSize;
	if (PaddingSize < 8) {
		::ZeroMemory(p, PaddingSize);
		MD5Transform(pdwMD5, PaddingData);
		::ZeroMemory(PaddingData, 56);
	} else {
		::ZeroMemory(p, PaddingSize - 8);
	}
	((ULONGLONG*)PaddingData)[7] = BitsSize;
	MD5Transform(pdwMD5, PaddingData);
}


/////////////////////////////////////////////////////////////////////////////
// ビットレート計算クラス
/////////////////////////////////////////////////////////////////////////////

CBitRateCalculator::CBitRateCalculator()
{
	Reset();
}

void CBitRateCalculator::Initialize()
{
	m_Time=::GetTickCount();
	m_Size=0;
	m_BitRate=0;
}

void CBitRateCalculator::Reset()
{
	m_Time=0;
	m_Size=0;
	m_BitRate=0;
}

bool CBitRateCalculator::Update(SIZE_T Size)
{
	DWORD Now=::GetTickCount();
	bool bUpdated=false;

	if (Now>=m_Time) {
		m_Size+=Size;
		if (Now-m_Time>=1000) {
			m_BitRate=(DWORD)(((ULONGLONG)m_Size*8*1000)/(ULONGLONG)(Now-m_Time));
			m_Time=Now;
			m_Size=0;
			bUpdated=true;
		}
	} else {
		m_Time=Now;
		m_Size=0;
	}
	return bUpdated;
}
