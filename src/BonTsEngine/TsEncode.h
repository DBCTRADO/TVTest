// TsEncode.h: TSエンコードクラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>


/////////////////////////////////////////////////////////////////////////////
// ARIB STD-B24 Part1文字列処理クラス
/////////////////////////////////////////////////////////////////////////////

class CAribString
{
public:
	enum {
		FLAG_CAPTION		= 0x0001U,	// 字幕
		FLAG_1SEG			= 0x0002U,	// ワンセグ
		FLAG_USE_CHAR_SIZE	= 0x0004U,	// 文字サイズを反映
		FLAG_UNICODE_SYMBOL	= 0x0008U	// Unicodeの記号を利用(Unicode 5.2以降)
	};

	enum CHAR_SIZE {
		SIZE_SMALL,		// 小型
		SIZE_MEDIUM,	// 中型
		SIZE_NORMAL,	// 標準
		SIZE_MICRO,		// 超小型
		SIZE_HIGH_W,	// 縦倍
		SIZE_WIDTH_W,	// 横倍
		SIZE_W,			// 縦横倍
		SIZE_SPECIAL_1,	// 特殊1
		SIZE_SPECIAL_2	// 特殊2
	};

	struct FormatInfo {
		DWORD Pos;
		CHAR_SIZE Size;
		BYTE CharColorIndex;
		BYTE BackColorIndex;
		BYTE RasterColorIndex;
		bool operator==(const FormatInfo &o) {
			return Pos == o.Pos
				&& Size == o.Size
				&& CharColorIndex == o.CharColorIndex
				&& BackColorIndex == o.BackColorIndex
				&& RasterColorIndex == o.RasterColorIndex;
		}
		bool operator!=(const FormatInfo &o) { return !(*this == o); }
	};
	typedef std::vector<FormatInfo> FormatList;

	class __declspec(novtable) IDRCSMap {
	public:
		virtual ~IDRCSMap() {}
		virtual LPCTSTR GetString(WORD Code) = 0;
	};

	static const DWORD AribToString(TCHAR *lpszDst, const DWORD dwDstLen,
									const BYTE *pSrcData, const DWORD dwSrcLen,
									const unsigned int Flags = FLAG_USE_CHAR_SIZE);
	static const DWORD CaptionToString(TCHAR *lpszDst, const DWORD dwDstLen, const BYTE *pSrcData, const DWORD dwSrcLen,
									   const bool b1Seg = false, FormatList *pFormatList = NULL, IDRCSMap *pDRCSMap = NULL);

private:
	enum CODE_SET
	{
		CODE_UNKNOWN,				// 不明なグラフィックセット(非対応)
		CODE_KANJI,					// Kanji
		CODE_ALPHANUMERIC,			// Alphanumeric
		CODE_HIRAGANA,				// Hiragana
		CODE_KATAKANA,				// Katakana
		CODE_MOSAIC_A,				// Mosaic A
		CODE_MOSAIC_B,				// Mosaic B
		CODE_MOSAIC_C,				// Mosaic C
		CODE_MOSAIC_D,				// Mosaic D
		CODE_PROP_ALPHANUMERIC,		// Proportional Alphanumeric
		CODE_PROP_HIRAGANA,			// Proportional Hiragana
		CODE_PROP_KATAKANA,			// Proportional Katakana
		CODE_JIS_X0201_KATAKANA,	// JIS X 0201 Katakana
		CODE_JIS_KANJI_PLANE_1,		// JIS compatible Kanji Plane 1
		CODE_JIS_KANJI_PLANE_2,		// JIS compatible Kanji Plane 2
		CODE_ADDITIONAL_SYMBOLS,	// Additional symbols
		CODE_DRCS_0,				// DRCS-0
		CODE_DRCS_1,				// DRCS-1
		CODE_DRCS_2,				// DRCS-2
		CODE_DRCS_3,				// DRCS-3
		CODE_DRCS_4,				// DRCS-4
		CODE_DRCS_5,				// DRCS-5
		CODE_DRCS_6,				// DRCS-6
		CODE_DRCS_7,				// DRCS-7
		CODE_DRCS_8,				// DRCS-8
		CODE_DRCS_9,				// DRCS-9
		CODE_DRCS_10,				// DRCS-10
		CODE_DRCS_11,				// DRCS-11
		CODE_DRCS_12,				// DRCS-12
		CODE_DRCS_13,				// DRCS-13
		CODE_DRCS_14,				// DRCS-14
		CODE_DRCS_15,				// DRCS-15
		CODE_MACRO					// Macro
	};

	CODE_SET m_CodeG[4];
	CODE_SET *m_pLockingGL;
	CODE_SET *m_pLockingGR;
	CODE_SET *m_pSingleGL;

	BYTE m_byEscSeqCount;
	BYTE m_byEscSeqIndex;
	bool m_bIsEscSeqDrcs;

	CHAR_SIZE m_CharSize;
	BYTE m_CharColorIndex;
	BYTE m_BackColorIndex;
	BYTE m_RasterColorIndex;
	BYTE m_DefPalette;
	BYTE m_RPC;
	FormatList *m_pFormatList;
	IDRCSMap *m_pDRCSMap;

	bool m_bCaption;
	bool m_bUseCharSize;
	bool m_bUnicodeSymbol;

	const DWORD AribToStringInternal(TCHAR *lpszDst, const DWORD dwDstLen, const BYTE *pSrcData, const DWORD dwSrcLen,
		const unsigned int Flags, FormatList *pFormatList = NULL, IDRCSMap *pDRCSMap = NULL);
	const DWORD ProcessString(TCHAR *lpszDst, const DWORD dwDstLen, const BYTE *pSrcData, const DWORD dwSrcLen);
	inline const int ProcessCharCode(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode, const CODE_SET CodeSet);

	inline const int PutKanjiChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode);
	inline const int PutAlphanumericChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode);
	inline const int PutHiraganaChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode);
	inline const int PutKatakanaChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode);
	inline const int PutJisKatakanaChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode);
	inline const int PutSymbolsChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode);
	inline const int PutMacroChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode);
	inline const int PutDRCSChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode);

	inline void ProcessEscapeSeq(const BYTE byCode);

	inline void LockingShiftGL(const BYTE byIndexG);
	inline void LockingShiftGR(const BYTE byIndexG);
	inline void SingleShiftGL(const BYTE byIndexG);

	inline const bool DesignationGSET(const BYTE byIndexG, const BYTE byCode);
	inline const bool DesignationDRCS(const BYTE byIndexG, const BYTE byCode);

	inline const bool IsSmallCharMode() const {
		return m_CharSize == SIZE_SMALL || m_CharSize == SIZE_MEDIUM || m_CharSize == SIZE_MICRO;
	}

	bool SetFormat(DWORD Pos);
};


/////////////////////////////////////////////////////////////////////////////
// ARIB STD-B10 Part2 Annex C MJD+JTC 処理クラス
/////////////////////////////////////////////////////////////////////////////

class CAribTime
{
public:
	static const bool AribToSystemTime(const BYTE *pHexData, SYSTEMTIME *pSysTime);
	static void SplitAribMjd(const WORD wAribMjd, WORD *pwYear, WORD *pwMonth, WORD *pwDay, WORD *pwDayOfWeek = NULL);
	static bool BuildAribMjd(const WORD Year, const WORD Month, const WORD Day, WORD *pMjd);
	static void MjdToSystemTime(const WORD wAribMjd, SYSTEMTIME *pSysTime);
	static const bool SystemTimeToMjd(const SYSTEMTIME *pSysTime, WORD *pMjd);
	static void SplitAribBcd(const BYTE *pAribBcd, WORD *pwHour, WORD *pwMinute, WORD *pwSecond = NULL);
	static const DWORD AribBcdToSecond(const BYTE *pAribBcd);
	static const WORD BcdHMToMinute(const WORD Bcd);
};
