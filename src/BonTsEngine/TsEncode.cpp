// TsEncode.cpp: TSエンコードクラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TsEncode.h"
#include "../Common/DebugDef.h"


#ifdef _UNICODE
#define MB_CHAR_LENGTH 1
#else
#define MB_CHAR_LENGTH 2
#endif


inline void CopyChar(LPTSTR pDst,LPCTSTR pSrc,int Index)
{
#ifdef _UNICODE
	pDst[0] = pSrc[Index];
#else
	pDst[0] = pSrc[Index * 2 + 0];
	pDst[1] = pSrc[Index * 2 + 1];
#endif
}


//////////////////////////////////////////////////////////////////////
// CAribString クラスの構築/消滅
//////////////////////////////////////////////////////////////////////

static const bool abCharSizeTable[] =
{
	false,	// CODE_UNKNOWN					不明なグラフィックセット(非対応)
	true,	// CODE_KANJI					Kanji
	false,	// CODE_ALPHANUMERIC			Alphanumeric
	false,	// CODE_HIRAGANA				Hiragana
	false,	// CODE_KATAKANA				Katakana
	false,	// CODE_MOSAIC_A				Mosaic A
	false,	// CODE_MOSAIC_B				Mosaic B
	false,	// CODE_MOSAIC_C				Mosaic C
	false,	// CODE_MOSAIC_D				Mosaic D
	false,	// CODE_PROP_ALPHANUMERIC		Proportional Alphanumeric
	false,	// CODE_PROP_HIRAGANA			Proportional Hiragana
	false,	// CODE_PROP_KATAKANA			Proportional Katakana
	false,	// CODE_JIS_X0201_KATAKANA		JIS X 0201 Katakana
	true,	// CODE_JIS_KANJI_PLANE_1		JIS compatible Kanji Plane 1
	true,	// CODE_JIS_KANJI_PLANE_2		JIS compatible Kanji Plane 2
	true,	// CODE_ADDITIONAL_SYMBOLS		Additional symbols
	true,	// CODE_DRCS_0					DRCS-0
	false,	// CODE_DRCS_1					DRCS-1
	false,	// CODE_DRCS_2					DRCS-2
	false,	// CODE_DRCS_3					DRCS-3
	false,	// CODE_DRCS_4					DRCS-4
	false,	// CODE_DRCS_5					DRCS-5
	false,	// CODE_DRCS_6					DRCS-6
	false,	// CODE_DRCS_7					DRCS-7
	false,	// CODE_DRCS_8					DRCS-8
	false,	// CODE_DRCS_9					DRCS-9
	false,	// CODE_DRCS_10					DRCS-10
	false,	// CODE_DRCS_11					DRCS-11
	false,	// CODE_DRCS_12					DRCS-12
	false,	// CODE_DRCS_13					DRCS-13
	false,	// CODE_DRCS_14					DRCS-14
	false,	// CODE_DRCS_15					DRCS-15
	false,	// CODE_MACRO					Macro
};

const DWORD CAribString::AribToString(
	TCHAR *lpszDst, const DWORD dwDstLen, const BYTE *pSrcData, const DWORD dwSrcLen, const unsigned int Flags)
{
	// ARIB STD-B24 Part1 → Shift-JIS / Unicode変換
	CAribString WorkObject;

	return WorkObject.AribToStringInternal(lpszDst, dwDstLen, pSrcData, dwSrcLen, Flags);
}

const DWORD CAribString::CaptionToString(
	TCHAR *lpszDst, const DWORD dwDstLen, const BYTE *pSrcData, const DWORD dwSrcLen,
	const bool b1Seg, FormatList *pFormatList, IDRCSMap *pDRCSMap)
{
	CAribString WorkObject;

	unsigned int Flags = FLAG_CAPTION;
	if (b1Seg)
		Flags|=FLAG_1SEG;

	return WorkObject.AribToStringInternal(lpszDst, dwDstLen, pSrcData, dwSrcLen, Flags, pFormatList, pDRCSMap);
}

const DWORD CAribString::AribToStringInternal(TCHAR *lpszDst, const DWORD dwDstLen, const BYTE *pSrcData, const DWORD dwSrcLen,
	const unsigned int Flags, FormatList *pFormatList, IDRCSMap *pDRCSMap)
{
	if (pSrcData == NULL || lpszDst == NULL || dwDstLen == 0)
		return 0UL;

	const bool bCaption = (Flags & FLAG_CAPTION) != 0;
	const bool b1Seg = (Flags & FLAG_1SEG) != 0;

	// 状態初期設定
	m_byEscSeqCount = 0U;
	m_pSingleGL = NULL;

	m_CodeG[0] = CODE_KANJI;
	m_CodeG[1] = CODE_ALPHANUMERIC;
	m_CodeG[2] = CODE_HIRAGANA;
	m_CodeG[3] = bCaption ? CODE_MACRO : CODE_KATAKANA;

	m_pLockingGL = &m_CodeG[0];
	m_pLockingGR = &m_CodeG[2];
	if (bCaption && b1Seg) {
		m_CodeG[1] = CODE_DRCS_1;
		m_pLockingGL = &m_CodeG[1];
		m_pLockingGR = &m_CodeG[0];
	}

	m_CharSize = SIZE_NORMAL;
	if (bCaption) {
		m_CharColorIndex = 7;
		m_BackColorIndex = 8;
		m_RasterColorIndex = 8;
	} else {
		m_CharColorIndex = 0;
		m_BackColorIndex = 0;
		m_RasterColorIndex = 0;
	}
	m_DefPalette = 0;
	m_RPC = 1;

	m_bCaption = bCaption;
	m_pFormatList = pFormatList;
	m_pDRCSMap = pDRCSMap;

	m_bUseCharSize = (Flags & FLAG_USE_CHAR_SIZE) != 0;
	m_bUnicodeSymbol = (Flags & FLAG_UNICODE_SYMBOL) != 0;

	return ProcessString(lpszDst, dwDstLen, pSrcData, dwSrcLen);
}

const DWORD CAribString::ProcessString(TCHAR *lpszDst, const DWORD dwDstLen, const BYTE *pSrcData, const DWORD dwSrcLen)
{
	DWORD dwSrcPos = 0UL, dwDstPos = 0UL;
	int Length;

	while (dwSrcPos < dwSrcLen && dwDstPos < dwDstLen - 1) {
		if (!m_byEscSeqCount) {
			// GL/GR領域
			if ((pSrcData[dwSrcPos] >= 0x21U) && (pSrcData[dwSrcPos] <= 0x7EU)) {
				// GL領域
				const CODE_SET CurCodeSet = (m_pSingleGL)? *m_pSingleGL : *m_pLockingGL;
				m_pSingleGL = NULL;

				if (abCharSizeTable[CurCodeSet]) {
					// 2バイトコード
					if ((dwSrcLen - dwSrcPos) < 2UL)
						break;

					Length = ProcessCharCode(&lpszDst[dwDstPos], dwDstLen - dwDstPos - 1, ((WORD)pSrcData[dwSrcPos + 0] << 8) | (WORD)pSrcData[dwSrcPos + 1], CurCodeSet);
					if (Length < 0)
						break;
					dwDstPos += Length;
					dwSrcPos++;
				} else {
					// 1バイトコード
					Length = ProcessCharCode(&lpszDst[dwDstPos], dwDstLen - dwDstPos - 1, (WORD)pSrcData[dwSrcPos], CurCodeSet);
					if (Length < 0)
						break;
					dwDstPos += Length;
				}
			} else if ((pSrcData[dwSrcPos] >= 0xA1U) && (pSrcData[dwSrcPos] <= 0xFEU)) {
				// GR領域
				const CODE_SET CurCodeSet = *m_pLockingGR;

				if (abCharSizeTable[CurCodeSet]) {
					// 2バイトコード
					if ((dwSrcLen - dwSrcPos) < 2UL) break;

					Length = ProcessCharCode(&lpszDst[dwDstPos], dwDstLen - dwDstPos -1, ((WORD)(pSrcData[dwSrcPos + 0] & 0x7FU) << 8) | (WORD)(pSrcData[dwSrcPos + 1] & 0x7FU), CurCodeSet);
					if (Length < 0)
						break;
					dwDstPos += Length;
					dwSrcPos++;
				} else {
					// 1バイトコード
					Length = ProcessCharCode(&lpszDst[dwDstPos], dwDstLen - dwDstPos - 1, (WORD)(pSrcData[dwSrcPos] & 0x7FU), CurCodeSet);
					if (Length < 0)
						break;
					dwDstPos += Length;
				}
			} else {
				// 制御コード
				switch (pSrcData[dwSrcPos]) {
				case 0x0D:	// APR
							lpszDst[dwDstPos++] = '\r';
							if (dwDstPos + 1 < dwDstLen)
								lpszDst[dwDstPos++] = '\n';
							break;
				case 0x0F:	LockingShiftGL(0U);					break;	// LS0
				case 0x0E:	LockingShiftGL(1U);					break;	// LS1
				case 0x19:	SingleShiftGL(2U);					break;	// SS2
				case 0x1D:	SingleShiftGL(3U);					break;	// SS3
				case 0x1B:	m_byEscSeqCount = 1U;				break;	// ESC
				case 0x20:	// SP
					if (IsSmallCharMode()) {
						lpszDst[dwDstPos++] = TEXT(' ');
					} else {
#ifdef _UNICODE
						lpszDst[dwDstPos++] = L'　';
#else
						if (dwDstPos + 2 < dwDstLen) {
							lpszDst[dwDstPos++] = "　"[0];
							lpszDst[dwDstPos++] = "　"[1];
						} else {
							lpszDst[dwDstPos++] = ' ';
						}
#endif
					}
					break;
				case 0xA0:	lpszDst[dwDstPos++] = TEXT(' ');	break;	// SP

				case 0x80:
				case 0x81:
				case 0x82:
				case 0x83:
				case 0x84:
				case 0x85:
				case 0x86:
				case 0x87:
					m_CharColorIndex = (m_DefPalette << 4) | (pSrcData[dwSrcPos] & 0x0F);
					SetFormat(dwDstPos);
					break;

				case 0x88:	// SSZ 小型
					m_CharSize = SIZE_SMALL;
					SetFormat(dwDstPos);
					break;
				case 0x89:	// MSZ 中型
					m_CharSize = SIZE_MEDIUM;
					SetFormat(dwDstPos);
					break;
				case 0x8A:	// NSZ 標準
					m_CharSize = SIZE_NORMAL;
					SetFormat(dwDstPos);
					break;
				case 0x8B:	// SZX 指定サイズ
					if (++dwSrcPos >= dwSrcLen)
						break;
					switch (pSrcData[dwSrcPos]) {
					case 0x60:	m_CharSize = SIZE_MICRO;		break;	// 超小型
					case 0x41:	m_CharSize = SIZE_HIGH_W;		break;	// 縦倍
					case 0x44:	m_CharSize = SIZE_WIDTH_W;		break;	// 横倍
					case 0x45:	m_CharSize = SIZE_W;			break;	// 縦横倍
					case 0x6B:	m_CharSize = SIZE_SPECIAL_1;	break;	// 特殊1
					case 0x64:	m_CharSize = SIZE_SPECIAL_2;	break;	// 特殊2
					}
					SetFormat(dwDstPos);
					break;

				case 0x0C:	// CS
					lpszDst[dwDstPos++] = '\f';
					break;
				case 0x16:	dwSrcPos++;		break;	// PAPF
				case 0x1C:	dwSrcPos+=2;	break;	// APS
				case 0x90:	// COL
					if (++dwSrcPos >= dwSrcLen)
						break;
					if (pSrcData[dwSrcPos] == 0x20) {
						m_DefPalette = pSrcData[++dwSrcPos] & 0x0F;
					} else {
						switch (pSrcData[dwSrcPos] & 0xF0) {
						case 0x40:
							m_CharColorIndex = pSrcData[dwSrcPos] & 0x0F;
							break;
						case 0x50:
							m_BackColorIndex = pSrcData[dwSrcPos] & 0x0F;
							break;
						}
						SetFormat(dwDstPos);
					}
					break;
				case 0x91:	dwSrcPos++;		break;	// FLC
				case 0x93:	dwSrcPos++;		break;	// POL
				case 0x94:	dwSrcPos++;		break;	// WMM
				case 0x95:	// MACRO
					do {
						if (++dwSrcPos >= dwSrcLen)
							break;
					} while (pSrcData[dwSrcPos] != 0x4F);
					break;
				case 0x97:	dwSrcPos++;		break;	// HLC
				case 0x98:	// RPC
					if (++dwSrcPos >= dwSrcLen)
						break;
					m_RPC = pSrcData[dwSrcPos] & 0x3F;
					break;
				case 0x9B:	//CSI
					for (Length = 0; ++dwSrcPos < dwSrcLen && pSrcData[dwSrcPos] <= 0x3B; Length++);
					if (dwSrcPos < dwSrcLen) {
						if (pSrcData[dwSrcPos] == 0x69) {	// ACS
							if (Length != 2)
								goto End;
							if (pSrcData[dwSrcPos - 2] >= 0x32) {
								while (++dwSrcPos < dwSrcLen && pSrcData[dwSrcPos] != 0x9B);
								dwSrcPos+=3;
							}
						}
					}
					break;
				case 0x9D:	// TIME
					if (++dwSrcPos >= dwSrcLen)
						break;
					if (pSrcData[dwSrcPos] == 0x20) {
						dwSrcPos++;
					} else {
						while (pSrcData[dwSrcPos] < 0x40 || pSrcData[dwSrcPos] > 0x43)
							dwSrcPos++;
					}
					break;
				default:	break;	// 非対応
				}
			}
		} else {
			// エスケープシーケンス処理
			ProcessEscapeSeq(pSrcData[dwSrcPos]);
		}

		dwSrcPos++;
	}

End:
	// 終端文字
	lpszDst[dwDstPos] = TEXT('\0');

	return dwDstPos;
}

inline const int CAribString::ProcessCharCode(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode, const CODE_SET CodeSet)
{
	int Length;

	switch (CodeSet) {
	case CODE_KANJI	:
	case CODE_JIS_KANJI_PLANE_1 :
	case CODE_JIS_KANJI_PLANE_2 :
		// 漢字コード出力
		Length = PutKanjiChar(lpszDst, dwDstLen, wCode);
		break;

	case CODE_ALPHANUMERIC :
	case CODE_PROP_ALPHANUMERIC :
		// 英数字コード出力
		Length = PutAlphanumericChar(lpszDst, dwDstLen, wCode);
		break;

	case CODE_HIRAGANA :
	case CODE_PROP_HIRAGANA :
		// ひらがなコード出力
		Length = PutHiraganaChar(lpszDst, dwDstLen, wCode);
		break;

	case CODE_PROP_KATAKANA :
	case CODE_KATAKANA :
		// カタカナコード出力
		Length = PutKatakanaChar(lpszDst, dwDstLen, wCode);
		break;

	case CODE_JIS_X0201_KATAKANA :
		// JISカタカナコード出力
		Length = PutJisKatakanaChar(lpszDst, dwDstLen, wCode);
		break;

	case CODE_ADDITIONAL_SYMBOLS :
		// 追加シンボルコード出力
		Length = PutSymbolsChar(lpszDst, dwDstLen, wCode);
		break;

	case CODE_MACRO:
		Length = PutMacroChar(lpszDst, dwDstLen, wCode);
		break;

	case CODE_DRCS_0:
		Length = PutDRCSChar(lpszDst, dwDstLen, wCode);
		break;

	case CODE_DRCS_1:
	case CODE_DRCS_2:
	case CODE_DRCS_3:
	case CODE_DRCS_4:
	case CODE_DRCS_5:
	case CODE_DRCS_6:
	case CODE_DRCS_7:
	case CODE_DRCS_8:
	case CODE_DRCS_9:
	case CODE_DRCS_10:
	case CODE_DRCS_11:
	case CODE_DRCS_12:
	case CODE_DRCS_13:
	case CODE_DRCS_14:
	case CODE_DRCS_15:
		Length = PutDRCSChar(lpszDst, dwDstLen, ((CodeSet - CODE_DRCS_0 + 0x40) << 8) | wCode);
		break;

	default:
#ifdef _UNICODE
		lpszDst[0] = L'□';
#else
		if (dwDstLen < 2)
			return -1;
		lpszDst[0] = "□"[0];
		lpszDst[1] = "□"[1];
#endif
		Length = MB_CHAR_LENGTH;
	}

	if (m_RPC > 1 && Length == 1 && dwDstLen > 1) {
		DWORD Count = min((DWORD)m_RPC - 1, dwDstLen - Length);
		while (Count-- > 0)
			lpszDst[Length++] = lpszDst[0];
	}
	m_RPC = 1;

	return Length;
}

inline const int CAribString::PutKanjiChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode)
{
	if (wCode >= 0x7521)
		return PutSymbolsChar(lpszDst, dwDstLen, wCode);

	// JIS → Shift_JIS漢字コード変換
	BYTE First = (BYTE)(wCode >> 8), Second = (BYTE)(wCode & 0x00FF);
	First -= 0x21;
	if ((First & 0x01) == 0) {
		Second += 0x1F;
		if (Second >= 0x7F)
			Second++;
	} else {
		Second += 0x7E;
	}
	First >>= 1;
	if (First >= 0x1F)
		First += 0xC1;
	else
		First += 0x81;

#ifdef _UNICODE
	// Shift_JIS → UNICODE
	if (dwDstLen < 1)
		return -1;
	char cShiftJIS[2];
	cShiftJIS[0] = (char)First;
	cShiftJIS[1] = (char)Second;
	// Shift_JIS = Code page 932
	int Length = ::MultiByteToWideChar(932, MB_PRECOMPOSED, cShiftJIS, 2, lpszDst, dwDstLen);
	if (Length == 0) {
		lpszDst[0] = L'□';
		return 1;
	}
	return Length;
#else
	// Shift_JIS → Shift_JIS
	if (dwDstLen < 2)
		return -1;
	lpszDst[0] = (char)First;
	lpszDst[1] = (char)Second;
	return 2;
#endif
}

inline const int CAribString::PutAlphanumericChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode)
{
	// 英数字文字コード変換
	static const LPCTSTR acAlphanumericTable =
		TEXT("　　　　　　　　　　　　　　　　")
		TEXT("　　　　　　　　　　　　　　　　")
		TEXT("　！”＃＄％＆’（）＊＋，－．／")
		TEXT("０１２３４５６７８９：；＜＝＞？")
		TEXT("＠ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯ")
		TEXT("ＰＱＲＳＴＵＶＷＸＹＺ［￥］＾＿")
		TEXT("｀ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏ")
		TEXT("ｐｑｒｓｔｕｖｗｘｙｚ｛｜｝￣　");
	static const LPCWSTR acAlphanumericHalfWidthTable =
		L"                "
		L"                "
		L" !\"#$%&'()*+,-./"
		L"0123456789:;<=>?"
		L"@ABCDEFGHIJKLMNO"
		L"PQRSTUVWXYZ[\u00a5]^_"
		L"`abcdefghijklmno"
		L"pqrstuvwxyz{|}\u203e ";

	if (m_bUseCharSize && m_CharSize == SIZE_MEDIUM) {
		if (dwDstLen < 1)
			return -1;
		*lpszDst = acAlphanumericHalfWidthTable[wCode];
		return 1;
	}

	if (dwDstLen < MB_CHAR_LENGTH)
		return -1;
	CopyChar(lpszDst, acAlphanumericTable, wCode);

	return MB_CHAR_LENGTH;
}

inline const int CAribString::PutHiraganaChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode)
{
	// ひらがな文字コード変換
	static const LPCTSTR acHiraganaTable =
		TEXT("　　　　　　　　　　　　　　　　")
		TEXT("　　　　　　　　　　　　　　　　")
		TEXT("　ぁあぃいぅうぇえぉおかがきぎく")
		TEXT("ぐけげこごさざしじすずせぜそぞた")
		TEXT("だちぢっつづてでとどなにぬねのは")
		TEXT("ばぱひびぴふぶぷへべぺほぼぽまみ")
		TEXT("むめもゃやゅゆょよらりるれろゎわ")
		TEXT("ゐゑをん　　　ゝゞー。「」、・　");

	if (dwDstLen < MB_CHAR_LENGTH)
		return -1;
	CopyChar(lpszDst, acHiraganaTable, wCode);

	return MB_CHAR_LENGTH;
}

inline const int CAribString::PutKatakanaChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode)
{
	// カタカナ文字コード変換
	static const LPCTSTR acKatakanaTable =
		TEXT("　　　　　　　　　　　　　　　　")
		TEXT("　　　　　　　　　　　　　　　　")
		TEXT("　ァアィイゥウェエォオカガキギク")
		TEXT("グケゲコゴサザシジスズセゼソゾタ")
		TEXT("ダチヂッツヅテデトドナニヌネノハ")
		TEXT("バパヒビピフブプヘベペホボポマミ")
		TEXT("ムメモャヤュユョヨラリルレロヮワ")
		TEXT("ヰヱヲンヴヵヶヽヾー。「」、・　");

	if (dwDstLen < MB_CHAR_LENGTH)
		return -1;
	CopyChar(lpszDst, acKatakanaTable, wCode);

	return MB_CHAR_LENGTH;
}

inline const int CAribString::PutJisKatakanaChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode)
{
	// JISカタカナ文字コード変換
	static const LPCTSTR acJisKatakanaTable =
		TEXT("　　　　　　　　　　　　　　　　")
		TEXT("　　　　　　　　　　　　　　　　")
		TEXT("　。「」、・ヲァィゥェォャュョッ")
		TEXT("ーアイウエオカキクケコサシスセソ")
		TEXT("タチツテトナニヌネノハヒフヘホマ")
		TEXT("ミムメモヤユヨラリルレロワン゛゜")
		TEXT("　　　　　　　　　　　　　　　　")
		TEXT("　　　　　　　　　　　　　　　　");

	if (dwDstLen < MB_CHAR_LENGTH)
		return -1;
	CopyChar(lpszDst, acJisKatakanaTable, wCode);

	return MB_CHAR_LENGTH;
}

inline const int CAribString::PutSymbolsChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode)
{
	// 追加シンボル文字コード変換

	static const LPCWSTR SymbolsTable_90_01[] =
	{
		L"\u26cc",		L"\u26cd",		L"\u2757",		L"\u26cf",		L"\u26d0",		L"\u26d1",		nullptr,		L"\u26d2",			// 0x7A21 - 0x7A28	90/01 - 90/08
		L"\u26d5",		L"\u26d3",		L"\u26d4",		nullptr,		nullptr,		nullptr,		nullptr,		L"\U0001f17f",		// 0x7A29 - 0x7A30	90/09 - 90/16
		L"\U0001f18a",	nullptr,		nullptr,		L"\u26d6",		L"\u26d7",		L"\u26d8",		L"\u26d9",		L"\u26da",			// 0x7A31 - 0x7A38	90/17 - 90/24
		L"\u26db",		L"\u26dc",		L"\u26dd",		L"\u26de",		L"\u26df",		L"\u26e0",		L"\u26e1",		L"\u2b55",			// 0x7A39 - 0x7A40	90/18 - 90/32
		L"\u3248",		L"\u3249",		L"\u324a",		L"\u324b",		L"\u324c",		L"\u324d",		L"\u324e",		L"\u324f"			// 0x7A41 - 0x7A48	90/33 - 90/40
	};

	static const LPCTSTR SymbolsTable_90_45[] =
	{
		_T("10."),		_T("11."),		_T("12."),			_T("[HV]"),		_T("[SD]"),		_T("[Ｐ]"),		_T("[Ｗ]"),		_T("[MV]"),		// 0x7A4D - 0x7A54	90/45 - 90/52
		_T("[手]"),		_T("[字]"),		_T("[双]"),			_T("[デ]"),		_T("[Ｓ]"),		_T("[二]"),		_T("[多]"),		_T("[解]"),		// 0x7A55 - 0x7A5C	90/53 - 90/60
		_T("[SS]"),		_T("[Ｂ]"),		_T("[Ｎ]"),			_T("■"),		_T("●"),		_T("[天]"),		_T("[交]"),		_T("[映]"),		// 0x7A5D - 0x7A64	90/61 - 90/68
		_T("[無]"),		_T("[料]"),		_T("[年齢制限]"),	_T("[前]"),		_T("[後]"),		_T("[再]"),		_T("[新]"),		_T("[初]"),		// 0x7A65 - 0x7A6C	90/69 - 90/76
		_T("[終]"),		_T("[生]"),		_T("[販]"),			_T("[声]"),		_T("[吹]"),		_T("[PPV]"),	_T("(秘)"),		_T("ほか")		// 0x7A6D - 0x7A74	90/77 - 90/84
	};
	static const LPCWSTR SymbolsTable_90_45_U[] =
	{
		L"\u2491",		L"\u2492",		L"\u2493",		L"\U0001f14a",	L"\U0001f14c",	L"\U0001f13f",	L"\U0001f146",	L"\U0001f14b",		// 0x7A4D - 0x7A54	90/45 - 90/52
		L"\U0001f210",	L"\U0001f211",	L"\U0001f212",	L"\U0001f213",	L"\U0001f142",	L"\U0001f214",	L"\U0001f215",	L"\U0001f216",		// 0x7A55 - 0x7A5C	90/53 - 90/60
		L"\U0001f14d",	L"\U0001f131",	L"\U0001f13d",	L"\u2b1b",		L"\u2b24",		L"\U0001f217",	L"\U0001f218",	L"\U0001f219",		// 0x7A5D - 0x7A64	90/61 - 90/68
		L"\U0001f21a",	L"\U0001f21b",	L"\u26bf",		L"\U0001f21c",	L"\U0001f21d",	L"\U0001f21e",	L"\U0001f21f",	L"\U0001f220",		// 0x7A65 - 0x7A6C	90/69 - 90/76
		L"\U0001f221",	L"\U0001f222",	L"\U0001f223",	L"\U0001f224",	L"\U0001f225",	L"\U0001f14e",	L"\u3299",		L"\U0001f200"		// 0x7A6D - 0x7A74	90/77 - 90/84
	};

	static const LPCWSTR SymbolsTable_91[] =
	{
		L"\u26e3",		L"\u2b56",		L"\u2b57",		L"\u2b58",		L"\u2b59",		L"\u2613",		L"\u328b",		L"\u3012",			// 0x7B21 - 0x7B28	91/01 - 91/08
		L"\u26e8",		L"\u3246",		L"\u3245",		L"\u26e9",		L"\u0fd6",		L"\u26ea",		L"\u26eb",		L"\u26ec",			// 0x7B29 - 0x7B30	91/09 - 91/16
		L"\u2668",		L"\u26ed",		L"\u26ee",		L"\u26ef",		L"\u2693",		L"\u2708",		L"\u26f0",		L"\u26f1",			// 0x7B31 - 0x7B38	91/17 - 91/24
		L"\u26f2",		L"\u26f3",		L"\u26f4",		L"\u26f5",		L"\U0001f157",	L"\u24b9",		L"\u24c8",		L"\u26f6",			// 0x7B39 - 0x7B40	91/25 - 91/32
		L"\U0001f15f",	L"\U0001f18b",	L"\U0001f18d",	L"\U0001f18c",	L"\U0001f179",	L"\u26f7",		L"\u26f8",		L"\u26f9",			// 0x7B41 - 0x7B48	91/33 - 91/40
		L"\u26fa",		L"\U0001f17b",	L"\u260e",		L"\u26fb",		L"\u26fc",		L"\u26fd",		L"\u26fe",		L"\U0001f17c",		// 0x7B49 - 0x7B50	91/41 - 91/48
		L"\u26ff"																															// 0x7B51 - 0x7B51	91/49 - 91/49
	};

	static const LPCTSTR SymbolsTable_92[] =
	{
		_T("→"),		_T("←"),		_T("↑"),		_T("↓"),		_T("○"),		_T("●"),		_T("年"),		_T("月"),			// 0x7C21 - 0x7C28	92/01 - 92/08
		_T("日"),		_T("円"),		_T("㎡"),		_T("立方ｍ"),	_T("㎝"),		_T("平方㎝"),	_T("立方㎝"),	_T("０."),			// 0x7C29 - 0x7C30	92/09 - 92/16
		_T("１."),		_T("２."),		_T("３."),		_T("４."),		_T("５."),		_T("６."),		_T("７."),		_T("８."),			// 0x7C31 - 0x7C38	92/17 - 92/24
		_T("９."),		_T("氏"),		_T("副"),		_T("元"),		_T("故"),		_T("前"),		_T("新"),		_T("０,"),			// 0x7C39 - 0x7C40	92/25 - 92/32
		_T("１,"),		_T("２,"),		_T("３,"),		_T("４,"),		_T("５,"),		_T("６,"),		_T("７,"),		_T("８,"),			// 0x7C41 - 0x7C48	92/33 - 92/40
		_T("９,"),		_T("(社)"),		_T("(財)"),		_T("(有)"),		_T("(株)"),		_T("(代)"),		_T("(問)"),		_T("＞"),			// 0x7C49 - 0x7C50	92/41 - 92/48
		_T("＜"),		_T("【"),		_T("】"),		_T("◇"),		_T("^2"),		_T("^3"),		_T("(CD)"),		_T("(vn)"),			// 0x7C51 - 0x7C58	92/49 - 92/56
		_T("(ob)"),		_T("(cb)"),		_T("(ce"),		_T("mb)"),		_T("(hp)"),		_T("(br)"),		_T("(p)"),		_T("(s)"),			// 0x7C59 - 0x7C60	92/57 - 92/64
		_T("(ms)"),		_T("(t)"),		_T("(bs)"),		_T("(b)"),		_T("(tb)"),		_T("(tp)"),		_T("(ds)"),		_T("(ag)"),			// 0x7C61 - 0x7C68	92/65 - 92/72
		_T("(eg)"),		_T("(vo)"),		_T("(fl)"),		_T("(ke"),		_T("y)"),		_T("(sa"),		_T("x)"),		_T("(sy"),			// 0x7C69 - 0x7C70	92/73 - 92/80
		_T("n)"),		_T("(or"),		_T("g)"),		_T("(pe"),		_T("r)"),		_T("(R)"),		_T("(C)"),		_T("(箏)"),			// 0x7C71 - 0x7C78	92/81 - 92/88
		_T("DJ"),		_T("[演]"),		_T("Fax")																							// 0x7C79 - 0x7C7B	92/89 - 92/91
	};
	static const LPCWSTR SymbolsTable_92_U[] =
	{
		L"\u27a1",		L"\u2b05",		L"\u2b06",		L"\u2b07",		L"\u2b2f",		L"\u2b2e",		L"年",			L"月",				// 0x7C21 - 0x7C28	92/01 - 92/08
		L"日",			L"円",			L"㎡",			L"\u33a5",		L"㎝",			L"\u33a0",		L"\u33a4",		L"\U0001f100",		// 0x7C29 - 0x7C30	92/09 - 92/16
		L"\u2488",		L"\u2489",		L"\u248a",		L"\u248b",		L"\u248c",		L"\u248d",		L"\u248e",		L"\u248f",			// 0x7C31 - 0x7C38	92/17 - 92/24
		L"\u2490",		L"氏",			L"副",			L"元",			L"故",			L"前",			L"新",			L"\U0001f101",		// 0x7C39 - 0x7C40	92/25 - 92/32
		L"\U0001f102",	L"\U0001f103",	L"\U0001f104",	L"\U0001f105",	L"\U0001f106",	L"\U0001f107",	L"\U0001f108",	L"\U0001f109",		// 0x7C41 - 0x7C48	92/33 - 92/40
		L"\U0001f10a",	L"\u3233",		L"\u3236",		L"\u3232",		L"\u3231",		L"\u3239",		L"\u3244",		L"\u25b6",			// 0x7C49 - 0x7C50	92/41 - 92/48
		L"\u25c0",		L"\u3016",		L"\u3017",		L"\u27d0",		L"\u00b2",		L"\u00b3",		L"\U0001f12d",	L"(vn)",			// 0x7C51 - 0x7C58	92/49 - 92/56
		L"(ob)",		L"(cb)",		L"(ce",			L"mb)",			L"(hp)",		L"(br)",		L"(p)",			L"(s)",				// 0x7C59 - 0x7C60	92/57 - 92/64
		L"(ms)",		L"(t)",			L"(bs)",		L"(b)",			L"(tb)",		L"(tp)",		L"(ds)",		L"(ag)",			// 0x7C61 - 0x7C68	92/65 - 92/72
		L"(eg)",		L"(vo)",		L"(fl)",		L"(ke",			L"y)",			L"(sa",			L"x)",			L"(sy",				// 0x7C69 - 0x7C70	92/73 - 92/80
		L"n)",			L"(or",			L"g)",			L"(pe",			L"r)",			L"\U0001f12c",	L"\U0001f12b",	L"\u3247",			// 0x7C71 - 0x7C78	92/81 - 92/88
		L"\U0001f190",	L"\U0001f226",	L"\u213b"																							// 0x7C79 - 0x7C7B	92/89 - 92/91
	};

	static const LPCTSTR SymbolsTable_93[] =
	{
		_T("(月)"),		_T("(火)"),		_T("(水)"),		_T("(木)"),		_T("(金)"),		_T("(土)"),		_T("(日)"),		_T("(祝)"),			// 0x7D21 - 0x7D28	93/01 - 93/08
		_T("㍾"),		_T("㍽"),		_T("㍼"),		_T("㍻"),		_T("№"),		_T("℡"),		_T("(〒)"),		_T("○"),			// 0x7D29 - 0x7D30	93/09 - 93/16
		_T("〔本〕"),	_T("〔三〕"),	_T("〔二〕"),	_T("〔安〕"),	_T("〔点〕"),	_T("〔打〕"),	_T("〔盗〕"),	_T("〔勝〕"),		// 0x7D31 - 0x7D38	93/17 - 93/24
		_T("〔敗〕"),	_T("〔Ｓ〕"),	_T("［投］"),	_T("［捕］"),	_T("［一］"),	_T("［二］"),	_T("［三］"),	_T("［遊］"),		// 0x7D39 - 0x7D40	93/25 - 93/32
		_T("［左］"),	_T("［中］"),	_T("［右］"),	_T("［指］"),	_T("［走］"),	_T("［打］"),	_T("㍑"),		_T("㎏"),			// 0x7D41 - 0x7D48	93/33 - 93/40
		_T("Hz"),		_T("ha"),		_T("km"),		_T("平方km"),	_T("hPa"),		nullptr,		nullptr,		_T("1/2"),			// 0x7D49 - 0x7D50	93/41 - 93/48
		_T("0/3"),		_T("1/3"),		_T("2/3"),		_T("1/4"),		_T("3/4"),		_T("1/5"),		_T("2/5"),		_T("3/5"),			// 0x7D51 - 0x7D58	93/49 - 93/56
		_T("4/5"),		_T("1/6"),		_T("5/6"),		_T("1/7"),		_T("1/8"),		_T("1/9"),		_T("1/10"),		_T("晴れ"),			// 0x7D59 - 0x7D60	93/57 - 93/64
		_T("曇り"),		_T("雨"),		_T("雪"),		_T("△"),		_T("▲"),		_T("▽"),		_T("▼"),		_T("◆"),			// 0x7D61 - 0x7D68	93/65 - 93/72
		_T("・"),		_T("・"),		_T("・"),		_T("◇"),		_T("◎"),		_T("!!"),		_T("!?"),		_T("曇/晴"),		// 0x7D69 - 0x7D70	93/73 - 93/80
		_T("雨"),		_T("雨"),		_T("雪"),		_T("大雪"),		_T("雷"),		_T("雷雨"),		_T("　"),		_T("・"),			// 0x7D71 - 0x7D78	93/81 - 93/88
		_T("・"),		_T("♪"),		_T("℡")																							// 0x7D79 - 0x7D7B	93/89 - 93/91
	};
	static const LPCWSTR SymbolsTable_93_U[] =
	{
		L"\u322a",		L"\u322b",		L"\u322c",		L"\u322d",		L"\u322e",		L"\u322f",		L"\u3230",		L"\u3237",			// 0x7D21 - 0x7D28	93/01 - 93/08
		L"㍾",			L"㍽",			L"㍼",			L"㍻",			L"№",			L"℡",			L"\u3036",		L"\u26be",			// 0x7D29 - 0x7D30	93/09 - 93/16
		L"\U0001f240",	L"\U0001f241",	L"\U0001f242",	L"\U0001f243",	L"\U0001f244",	L"\U0001f245",	L"\U0001f246",	L"\U0001f247",		// 0x7D31 - 0x7D38	93/17 - 93/24
		L"\U0001f248",	L"\U0001f12a",	L"\U0001f227",	L"\U0001f228",	L"\U0001f229",	L"\U0001f214",	L"\U0001f22a",	L"\U0001f22b",		// 0x7D39 - 0x7D40	93/25 - 93/32
		L"\U0001f22c",	L"\U0001f22d",	L"\U0001f22e",	L"\U0001f22f",	L"\U0001f230",	L"\U0001f231",	L"\u2113",		L"㎏",				// 0x7D41 - 0x7D48	93/33 - 93/40
		L"\u3390",		L"\u33ca",		L"\u339e",		L"\u33a2",		L"\u3371",		nullptr,		nullptr,		L"\u00bd",			// 0x7D49 - 0x7D50	93/41 - 93/48
		L"\u2189",		L"\u2153",		L"\u2154",		L"\u00bc",		L"\u00be",		L"\u2155",		L"\u2156",		L"\u2157",			// 0x7D51 - 0x7D58	93/49 - 93/56
		L"\u2158",		L"\u2159",		L"\u215a",		L"\u2150",		L"\u215b",		L"\u2151",		L"\u2152",		L"\u2600",			// 0x7D59 - 0x7D60	93/57 - 93/64
		L"\u2601",		L"\u2602",		L"\u26c4",		L"\u2616",		L"\u2617",		L"\u26c9",		L"\u26ca",		L"\u2666",			// 0x7D61 - 0x7D68	93/65 - 93/72
		L"\u2665",		L"\u2663",		L"\u2660",		L"\u26cb",		L"\u2a00",		L"\u203c",		L"\u2049",		L"\u26c5",			// 0x7D69 - 0x7D70	93/73 - 93/80
		L"\u2614",		L"\u26c6",		L"\u2603",		L"\u26c7",		L"\u26a1",		L"\u26c8",		L"　",			L"\u269e",			// 0x7D71 - 0x7D78	93/81 - 93/88
		L"\u269f",		L"\u266c",		L"\u260e"																							// 0x7D79 - 0x7D7B	93/89 - 93/91
	};

	static const LPCTSTR SymbolsTable_94[] =
	{
		_T("Ⅰ"),		_T("Ⅱ"),		_T("Ⅲ"),		_T("Ⅳ"),		_T("Ⅴ"),		_T("Ⅵ"),		_T("Ⅶ"),		_T("Ⅷ"),			// 0x7E21 - 0x7E28	94/01 - 94/08
		_T("Ⅸ"),		_T("Ⅹ"),		_T("XI"),		_T("XⅡ"),		_T("⑰"),		_T("⑱"),		_T("⑲"),		_T("⑳"),			// 0x7E29 - 0x7E30	94/09 - 94/16
		_T("(1)"),		_T("(2)"),		_T("(3)"),		_T("(4)"),		_T("(5)"),		_T("(6)"),		_T("(7)"),		_T("(8)"),			// 0x7E31 - 0x7E38	94/17 - 94/24
		_T("(9)"),		_T("(10)"),		_T("(11)"),		_T("(12)"),		_T("(21)"),		_T("(22)"),		_T("(23)"),		_T("(24)"),			// 0x7E39 - 0x7E40	94/25 - 94/32
		_T("(A)"),		_T("(B)"),		_T("(C)"),		_T("(D)"),		_T("(E)"),		_T("(F)"),		_T("(G)"),		_T("(H)"),			// 0x7E41 - 0x7E48	94/33 - 94/40
		_T("(I)"),		_T("(J)"),		_T("(K)"),		_T("(L)"),		_T("(M)"),		_T("(N)"),		_T("(O)"),		_T("(P)"),			// 0x7E49 - 0x7E50	94/41 - 94/48
		_T("(Q)"),		_T("(R)"),		_T("(S)"),		_T("(T)"),		_T("(U)"),		_T("(V)"),		_T("(W)"),		_T("(X)"),			// 0x7E51 - 0x7E58	94/49 - 94/56
		_T("(Y)"),		_T("(Z)"),		_T("(25)"),		_T("(26)"),		_T("(27)"),		_T("(28)"),		_T("(29)"),		_T("(30)"),			// 0x7E59 - 0x7E60	94/57 - 94/64
		_T("①"),		_T("②"),		_T("③"),		_T("④"),		_T("⑤"),		_T("⑥"),		_T("⑦"),		_T("⑧"),			// 0x7E61 - 0x7E68	94/65 - 94/72
		_T("⑨"),		_T("⑩"),		_T("⑪"),		_T("⑫"),		_T("⑬"),		_T("⑭"),		_T("⑮"),		_T("⑯"),			// 0x7E69 - 0x7E70	94/73 - 94/80
		_T("①"),		_T("②"),		_T("③"),		_T("④"),		_T("⑤"),		_T("⑥"),		_T("⑦"),		_T("⑧"),			// 0x7E71 - 0x7E78	94/81 - 94/88
		_T("⑨"),		_T("⑩"),		_T("⑪"),		_T("⑫"),		_T("(31)")															// 0x7E79 - 0x7E7D	94/89 - 94/93
	};
	static const LPCWSTR SymbolsTable_94_U[] =
	{
		L"Ⅰ",			L"Ⅱ",			L"Ⅲ",			L"Ⅳ",			L"Ⅴ",			L"Ⅵ",			L"Ⅶ",			L"Ⅷ",				// 0x7E21 - 0x7E28	94/01 - 94/08
		L"Ⅸ",			L"Ⅹ",			L"\u216a",		L"\u216b",		L"⑰",			L"⑱",			L"⑲",			L"⑳",				// 0x7E29 - 0x7E30	94/09 - 94/16
		L"\u2474",		L"\u2475",		L"\u2476",		L"\u2477",		L"\u2478",		L"\u2479",		L"\u247a",		L"\u247b",			// 0x7E31 - 0x7E38	94/17 - 94/24
		L"\u247c",		L"\u247d",		L"\u247e",		L"\u247f",		L"\u3251",		L"\u3252",		L"\u3253",		L"\u3254",			// 0x7E39 - 0x7E40	94/25 - 94/32
		L"\U0001f1110",	L"\U0001f111",	L"\U0001f112",	L"\U0001f113",	L"\U0001f114",	L"\U0001f115",	L"\U0001f116",	L"\U0001f117",		// 0x7E41 - 0x7E48	94/33 - 94/40
		L"\U0001f1118",	L"\U0001f119",	L"\U0001f11a",	L"\U0001f11b",	L"\U0001f11c",	L"\U0001f11d",	L"\U0001f11e",	L"\U0001f11f",		// 0x7E49 - 0x7E50	94/41 - 94/48
		L"\U0001f120",	L"\U0001f121",	L"\U0001f122",	L"\U0001f123",	L"\U0001f124",	L"\U0001f125",	L"\U0001f126",	L"\U0001f127",		// 0x7E51 - 0x7E58	94/49 - 94/56
		L"\U0001f128",	L"\U0001f129",	L"\u3255",		L"\u3256",		L"\u3257",		L"\u3258",		L"\u3259",		L"\u325a",			// 0x7E59 - 0x7E60	94/57 - 94/64
		L"①",			L"②",			L"③",			L"④",			L"⑤",			L"⑥",			L"⑦",			L"⑧",				// 0x7E61 - 0x7E68	94/65 - 94/72
		L"⑨",			L"⑩",			L"⑪",			L"⑫",			L"⑬",			L"⑭",			L"⑮",			L"⑯",				// 0x7E69 - 0x7E70	94/73 - 94/80
		L"\u2776",		L"\u2777",		L"\u2778",		L"\u2779",		L"\u277a",		L"\u277b",		L"\u277c",		L"\u277d",			// 0x7E71 - 0x7E78	94/81 - 94/88
		L"\u277e",		L"\u277f",		L"\u24eb",		L"\u24ec",		L"\u325b"															// 0x7E79 - 0x7E7D	94/89 - 94/93
	};

	static const LPCWSTR KanjiTable1[] = {
		L"\u3402",		L"\U00020158",	L"\u4efd",		L"\u4eff",		L"\u4f9a",		L"\u4fc9",		L"\u509c",		L"\u511e",			// 0x7521 - 0x7528
		L"\u51bc",		L"\u351f",		L"\u5307",		L"\u5361",		L"\u536c",		L"\u8a79",		L"\U00020bb7",	L"\u544d",			// 0x7529 - 0x7530
		L"\u5496",		L"\u549c",		L"\u54a9",		L"\u550e",		L"\u554a",		L"\u5672",		L"\u56e4",		L"\u5733",			// 0x7531 - 0x7538
		L"\u5734",		L"\ufa10",		L"\u5880",		L"\u59e4",		L"\u5a23",		L"\u5a55",		L"\u5bec",		L"\ufa11",			// 0x7539 - 0x7540
		L"\u37e2",		L"\u5eac",		L"\u5f34",		L"\u5f45",		L"\u5fb7",		L"\u6017",		L"\ufa6b",		L"\u6130",			// 0x7541 - 0x7548
		L"\u6624",		L"\u66c8",		L"\u66d9",		L"\u66fa",		L"\u66fb",		L"\u6852",		L"\u9fc4",		L"\u6911",			// 0x7549 - 0x7551
		L"\u693b",		L"\u6a45",		L"\u6a91",		L"\u6adb",		L"\U000233cc",	L"\U000233fe",	L"\U000235c4",	L"\u6bf1",			// 0x7550 - 0x7558
		L"\u6ce0",		L"\u6d2e",		L"\ufa45",		L"\u6dbf",		L"\u6dca",		L"\u6df8",		L"\ufa46",		L"\u6f5e",			// 0x7559 - 0x7560
		L"\u6ff9",		L"\u7064",		L"\ufa6c",		L"\U000242ee",	L"\u7147",		L"\u71c1",		L"\u7200",		L"\u739f",			// 0x7561 - 0x7568
		L"\u73a8",		L"\u73c9",		L"\u73d6",		L"\u741b",		L"\u7421",		L"\ufa4a",		L"\u7426",		L"\u742a",			// 0x7569 - 0x7570
		L"\u742c",		L"\u7439",		L"\u744b",		L"\u3eda",		L"\u7575",		L"\u7581",		L"\u7772",		L"\u4093",			// 0x7571 - 0x7578
		L"\u78c8",		L"\u78e0",		L"\u7947",		L"\u79ae",		L"\u9fc6",		L"\u4103",											// 0x7579 - 0x757E
	};
	static const LPCWSTR KanjiTable2[] = {
		L"\u9fc5",		L"\u79da",		L"\u7a1e",		L"\u7b7f",		L"\u7c31",		L"\u4264",		L"\u7d8b",		L"\u7fa1",			// 0x7621 - 0x7628
		L"\u8118",		L"\u813a",		L"\ufa6d",		L"\u82ae",		L"\u845b",		L"\u84dc",		L"\u84ec",		L"\u8559",			// 0x7629 - 0x7630
		L"\u85ce",		L"\u8755",		L"\u87ec",		L"\u880b",		L"\u88f5",		L"\u89d2",		L"\u8af6",		L"\u8dce",			// 0x7631 - 0x7638
		L"\u8fbb",		L"\u8ff6",		L"\u90dd",		L"\u9127",		L"\u912d",		L"\u91b2",		L"\u9233",		L"\u9288",			// 0x7639 - 0x7640
		L"\u9321",		L"\u9348",		L"\u9592",		L"\u96de",		L"\u9903",		L"\u9940",		L"\u9ad9",		L"\u9bd6",			// 0x7641 - 0x7648
		L"\u9dd7",		L"\u9eb4",		L"\u9eb5",																							// 0x7649 - 0x764B
	};

	static const struct {
		WORD First;
		WORD Last;
		const LPCTSTR *Table;
		const LPCWSTR *TableU;
	} SymbolTable[] =
	{
		{0x7521,	0x757E,	nullptr,			KanjiTable1},
		{0x7621,	0x764B,	nullptr,			KanjiTable2},
		{0x7A21,	0x7A48,	nullptr,			SymbolsTable_90_01},
		{0x7A4D,	0x7A74,	SymbolsTable_90_45,	SymbolsTable_90_45_U},
		{0x7B21,	0x7B51,	nullptr,			SymbolsTable_91},
		{0x7C21,	0x7C7B,	SymbolsTable_92,	SymbolsTable_92_U},
		{0x7D21,	0x7D7B,	SymbolsTable_93,	SymbolsTable_93_U},
		{0x7E21,	0x7E7D,	SymbolsTable_94,	SymbolsTable_94_U},
	};

	// シンボルを変換する
	LPCTSTR pszSrc = nullptr;
	for (int i = 0; i < _countof(SymbolTable); i++) {
		if ((wCode >= SymbolTable[i].First) && (wCode <= SymbolTable[i].Last)) {
#ifdef _UNICODE
			if (!SymbolTable[i].Table || m_bUnicodeSymbol)
				pszSrc = SymbolTable[i].TableU[wCode - SymbolTable[i].First];
			else
				pszSrc = SymbolTable[i].Table[wCode - SymbolTable[i].First];
#else
			if (SymbolTable[i].Table)
				pszSrc = SymbolTable[i].Table[wCode - SymbolTable[i].First];
#endif
			break;
		}
	}
	if (!pszSrc)
		pszSrc = TEXT("□");
	DWORD Length = ::lstrlen(pszSrc);
	if (dwDstLen < Length)
		return -1;
	::CopyMemory(lpszDst, pszSrc, Length * sizeof(TCHAR));

	return Length;
}

inline const int CAribString::PutMacroChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode)
{
	static const BYTE Macro[16][20] = {
		{ 0x1B, 0x24, 0x39, 0x1B, 0x29, 0x4A, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
		{ 0x1B, 0x24, 0x39, 0x1B, 0x29, 0x31, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
		{ 0x1B, 0x24, 0x39, 0x1B, 0x29, 0x20, 0x41, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
		{ 0x1B, 0x28, 0x32, 0x1B, 0x29, 0x34, 0x1B, 0x2A, 0x35, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
		{ 0x1B, 0x28, 0x32, 0x1B, 0x29, 0x33, 0x1B, 0x2A, 0x35, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
		{ 0x1B, 0x28, 0x32, 0x1B, 0x29, 0x20, 0x41, 0x1B, 0x2A, 0x35, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
		{ 0x1B, 0x28, 0x20, 0x41, 0x1B, 0x29, 0x20, 0x42, 0x1B, 0x2A, 0x20, 0x43, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
		{ 0x1B, 0x28, 0x20, 0x44, 0x1B, 0x29, 0x20, 0x45, 0x1B, 0x2A, 0x20, 0x46, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
		{ 0x1B, 0x28, 0x20, 0x47, 0x1B, 0x29, 0x20, 0x48, 0x1B, 0x2A, 0x20, 0x49, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
		{ 0x1B, 0x28, 0x20, 0x4A, 0x1B, 0x29, 0x20, 0x4B, 0x1B, 0x2A, 0x20, 0x4C, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
		{ 0x1B, 0x28, 0x20, 0x4D, 0x1B, 0x29, 0x20, 0x4E, 0x1B, 0x2A, 0x20, 0x4F, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
		{ 0x1B, 0x24, 0x39, 0x1B, 0x29, 0x20, 0x42, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
		{ 0x1B, 0x24, 0x39, 0x1B, 0x29, 0x20, 0x43, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
		{ 0x1B, 0x24, 0x39, 0x1B, 0x29, 0x20, 0x44, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
		{ 0x1B, 0x28, 0x31, 0x1B, 0x29, 0x30, 0x1B, 0x2A, 0x4A, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
		{ 0x1B, 0x28, 0x4A, 0x1B, 0x29, 0x32, 0x1B, 0x2A, 0x20, 0x41, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D },
	};

	if ((wCode & 0xF0) == 0x60)
		return ProcessString(lpszDst, dwDstLen, Macro[wCode & 0x0F], ::lstrlenA((LPCSTR)Macro[wCode & 0x0F]));
	return 0;
}

inline const int CAribString::PutDRCSChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode)
{
	if (m_pDRCSMap) {
		LPCTSTR pszSrc = m_pDRCSMap->GetString(wCode);
		if (pszSrc) {
			int Length = ::lstrlen(pszSrc);
			if (dwDstLen < (DWORD)Length)
				return -1;
			::CopyMemory(lpszDst, pszSrc, Length * sizeof(TCHAR));
			return Length;
		}
	}
#ifdef _UNICODE
	if (dwDstLen < 1)
		return -1;
	lpszDst[0] = L'□';
	return 1;
#else
	if (dwDstLen < 2)
		return -1;
	lpszDst[0] = "□"[0];
	lpszDst[1] = "□"[1];
	return 2;
#endif
}

inline void CAribString::ProcessEscapeSeq(const BYTE byCode)
{
	// エスケープシーケンス処理
	switch(m_byEscSeqCount){
		// 1バイト目
		case 1U	:
			switch(byCode){
				// Invocation of code elements
				case 0x6EU	: LockingShiftGL(2U);	m_byEscSeqCount = 0U;	return;		// LS2
				case 0x6FU	: LockingShiftGL(3U);	m_byEscSeqCount = 0U;	return;		// LS3
				case 0x7EU	: LockingShiftGR(1U);	m_byEscSeqCount = 0U;	return;		// LS1R
				case 0x7DU	: LockingShiftGR(2U);	m_byEscSeqCount = 0U;	return;		// LS2R
				case 0x7CU	: LockingShiftGR(3U);	m_byEscSeqCount = 0U;	return;		// LS3R

				// Designation of graphic sets
				case 0x24U	:
				case 0x28U	: m_byEscSeqIndex = 0U;		break;
				case 0x29U	: m_byEscSeqIndex = 1U;		break;
				case 0x2AU	: m_byEscSeqIndex = 2U;		break;
				case 0x2BU	: m_byEscSeqIndex = 3U;		break;
				default		: m_byEscSeqCount = 0U;		return;		// エラー
				}
			break;

		// 2バイト目
		case 2U	:
			if(DesignationGSET(m_byEscSeqIndex, byCode)){
				m_byEscSeqCount = 0U;
				return;
				}

			switch(byCode){
				case 0x20	: m_bIsEscSeqDrcs = true;	break;
				case 0x28	: m_bIsEscSeqDrcs = true;	m_byEscSeqIndex = 0U;	break;
				case 0x29	: m_bIsEscSeqDrcs = false;	m_byEscSeqIndex = 1U;	break;
				case 0x2A	: m_bIsEscSeqDrcs = false;	m_byEscSeqIndex = 2U;	break;
				case 0x2B	: m_bIsEscSeqDrcs = false;	m_byEscSeqIndex = 3U;	break;
				default		: m_byEscSeqCount = 0U;		return;		// エラー
				}
			break;

		// 3バイト目
		case 3U	:
			if(!m_bIsEscSeqDrcs){
				if(DesignationGSET(m_byEscSeqIndex, byCode)){
					m_byEscSeqCount = 0U;
					return;
					}
				}
			else{
				if(DesignationDRCS(m_byEscSeqIndex, byCode)){
					m_byEscSeqCount = 0U;
					return;
					}
				}

			if(byCode == 0x20U){
				m_bIsEscSeqDrcs = true;
				}
			else{
				// エラー
				m_byEscSeqCount = 0U;
				return;
				}
			break;

		// 4バイト目
		case 4U	:
			DesignationDRCS(m_byEscSeqIndex, byCode);
			m_byEscSeqCount = 0U;
			return;
		}

	m_byEscSeqCount++;
}

inline void CAribString::LockingShiftGL(const BYTE byIndexG)
{
	// LSx
	m_pLockingGL = &m_CodeG[byIndexG];
}

inline void CAribString::LockingShiftGR(const BYTE byIndexG)
{
	// LSxR
	m_pLockingGR = &m_CodeG[byIndexG];
}

inline void CAribString::SingleShiftGL(const BYTE byIndexG)
{
	// SSx
	m_pSingleGL  = &m_CodeG[byIndexG];
}

inline const bool CAribString::DesignationGSET(const BYTE byIndexG, const BYTE byCode)
{
	// Gのグラフィックセットを割り当てる
	switch(byCode){
		case 0x42U	: m_CodeG[byIndexG] = CODE_KANJI;				return true;	// Kanji
		case 0x4AU	: m_CodeG[byIndexG] = CODE_ALPHANUMERIC;		return true;	// Alphanumeric
		case 0x30U	: m_CodeG[byIndexG] = CODE_HIRAGANA;			return true;	// Hiragana
		case 0x31U	: m_CodeG[byIndexG] = CODE_KATAKANA;			return true;	// Katakana
		case 0x32U	: m_CodeG[byIndexG] = CODE_MOSAIC_A;			return true;	// Mosaic A
		case 0x33U	: m_CodeG[byIndexG] = CODE_MOSAIC_B;			return true;	// Mosaic B
		case 0x34U	: m_CodeG[byIndexG] = CODE_MOSAIC_C;			return true;	// Mosaic C
		case 0x35U	: m_CodeG[byIndexG] = CODE_MOSAIC_D;			return true;	// Mosaic D
		case 0x36U	: m_CodeG[byIndexG] = CODE_PROP_ALPHANUMERIC;	return true;	// Proportional Alphanumeric
		case 0x37U	: m_CodeG[byIndexG] = CODE_PROP_HIRAGANA;		return true;	// Proportional Hiragana
		case 0x38U	: m_CodeG[byIndexG] = CODE_PROP_KATAKANA;		return true;	// Proportional Katakana
		case 0x49U	: m_CodeG[byIndexG] = CODE_JIS_X0201_KATAKANA;	return true;	// JIS X 0201 Katakana
		case 0x39U	: m_CodeG[byIndexG] = CODE_JIS_KANJI_PLANE_1;	return true;	// JIS compatible Kanji Plane 1
		case 0x3AU	: m_CodeG[byIndexG] = CODE_JIS_KANJI_PLANE_2;	return true;	// JIS compatible Kanji Plane 2
		case 0x3BU	: m_CodeG[byIndexG] = CODE_ADDITIONAL_SYMBOLS;	return true;	// Additional symbols
		default		: return false;		// 不明なグラフィックセット
		}
}

inline const bool CAribString::DesignationDRCS(const BYTE byIndexG, const BYTE byCode)
{
	// DRCSのグラフィックセットを割り当てる
	if (byCode >= 0x40 && byCode <= 0x4F) {		// DRCS
		m_CodeG[byIndexG] = (CODE_SET)(CODE_DRCS_0 + (byCode - 0x40));
	} else if (byCode == 0x70) {				// Macro
		m_CodeG[byIndexG] = CODE_MACRO;
	} else {
		return false;
	}
	return true;
}

bool CAribString::SetFormat(DWORD Pos)
{
	if (!m_pFormatList)
		return false;

	FormatInfo Format;
	Format.Pos = Pos;
	Format.Size = m_CharSize;
	Format.CharColorIndex = m_CharColorIndex;
	Format.BackColorIndex = m_BackColorIndex;
	Format.RasterColorIndex = m_RasterColorIndex;

	if (!m_pFormatList->empty()) {
		if (m_pFormatList->back().Pos == Pos) {
			m_pFormatList->back() = Format;
			return true;
		}
	}
	m_pFormatList->push_back(Format);
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// ARIB STD-B10 Part2 Annex C MJD+JTC 処理クラス
/////////////////////////////////////////////////////////////////////////////

const bool CAribTime::AribToSystemTime(const BYTE *pHexData, SYSTEMTIME *pSysTime)
{
	// 全ビットが1のときは未定義
	if((*((DWORD *)pHexData) == 0xFFFFFFFFUL) && (pHexData[4] == 0xFFU))return false;

	// MJD形式の日付を解析
	SplitAribMjd(((WORD)pHexData[0] << 8) | (WORD)pHexData[1], &pSysTime->wYear, &pSysTime->wMonth, &pSysTime->wDay, &pSysTime->wDayOfWeek);

	// BCD形式の時刻を解析
	SplitAribBcd(&pHexData[2], &pSysTime->wHour, &pSysTime->wMinute, &pSysTime->wSecond);

	// ミリ秒は常に0
	pSysTime->wMilliseconds = 0U;

	return true;
}

void CAribTime::SplitAribMjd(const WORD wAribMjd, WORD *pwYear, WORD *pwMonth, WORD *pwDay, WORD *pwDayOfWeek)
{
	// MJD形式の日付を解析する
	const DWORD dwYd = (DWORD)(((double)wAribMjd - 15078.2) / 365.25);
	const DWORD dwMd = (DWORD)(((double)wAribMjd - 14956.1 - (double)((int)((double)dwYd * 365.25))) / 30.6001);
	const DWORD dwK = ((dwMd == 14UL) || (dwMd == 15UL))? 1U : 0U;

	if(pwDay)*pwDay = wAribMjd - 14956U - (WORD)((double)dwYd * 365.25) - (WORD)((double)dwMd * 30.6001);
	if(pwYear)*pwYear = (WORD)(dwYd + dwK) + 1900U;
	if(pwMonth)*pwMonth	= (WORD)(dwMd - 1UL - dwK * 12UL);
	if(pwDayOfWeek)*pwDayOfWeek = (wAribMjd + 3U) % 7U;
}

bool CAribTime::BuildAribMjd(const WORD Year, const WORD Month, const WORD Day, WORD *pMjd)
{
	// 日付をMJD形式に変換する
	if (!pMjd)
		return false;

	int Y = Year, M = Month;
	if (M <= 2) {
		M += 12;
		Y--;
	}
	*pMjd = (WORD)((int)((double)Y * 365.25) + (Y / 400) - (Y / 100) + (int)((double)(M - 2) * 30.59) + Day - 678912);

	return true;
}

void CAribTime::MjdToSystemTime(const WORD wAribMjd, SYSTEMTIME *pSysTime)
{
	::ZeroMemory(pSysTime, sizeof(SYSTEMTIME));
	SplitAribMjd(wAribMjd, &pSysTime->wYear, &pSysTime->wMonth, &pSysTime->wDay, &pSysTime->wDayOfWeek);
}

const bool CAribTime::SystemTimeToMjd(const SYSTEMTIME *pSysTime, WORD *pMjd)
{
	if (!pSysTime)
		return false;

	return BuildAribMjd(pSysTime->wYear, pSysTime->wMonth, pSysTime->wDay, pMjd);
}

void CAribTime::SplitAribBcd(const BYTE *pAribBcd, WORD *pwHour, WORD *pwMinute, WORD *pwSecond)
{
	// BCD形式の時刻を解析する
	if(pwHour)*pwHour		= (WORD)(pAribBcd[0] >> 4) * 10U + (WORD)(pAribBcd[0] & 0x0FU);
	if(pwMinute)*pwMinute	= (WORD)(pAribBcd[1] >> 4) * 10U + (WORD)(pAribBcd[1] & 0x0FU);
	if(pwSecond)*pwSecond	= (WORD)(pAribBcd[2] >> 4) * 10U + (WORD)(pAribBcd[2] & 0x0FU);
}

const DWORD CAribTime::AribBcdToSecond(const BYTE *pAribBcd)
{
	// 全ビットが1のときは未定義
	if (pAribBcd[0] == 0xFF && pAribBcd[1] == 0xFF && pAribBcd[2] == 0xFF)
		return 0;

	// BCD形式の時刻を秒に変換する
	const DWORD dwSecond = (((DWORD)(pAribBcd[0] >> 4) * 10U + (DWORD)(pAribBcd[0] & 0x0FU)) * 3600UL)
						 + (((DWORD)(pAribBcd[1] >> 4) * 10U + (DWORD)(pAribBcd[1] & 0x0FU)) * 60UL)
						 + ((DWORD)(pAribBcd[2] >> 4) * 10U + (DWORD)(pAribBcd[2] & 0x0FU));

	return dwSecond;
}

const WORD CAribTime::BcdHMToMinute(const WORD Bcd)
{
	return ((((Bcd >> 12) * 10) + ((Bcd >> 8) & 0x0F)) * 60) +
			((((Bcd >> 4) & 0x0F) * 10) + (Bcd & 0x0F));
}
