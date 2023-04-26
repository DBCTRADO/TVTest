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


#include "stdafx.h"
#include "TVTest.h"
#include "EpgUtil.h"
#include "AppMain.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace EpgUtil
{


VideoType GetVideoType(BYTE ComponentType)
{
	if ((ComponentType & 0x0F) >= 1 && (ComponentType & 0x0F) <= 4) {
		switch (ComponentType >> 4) {
		case 0x0:
		case 0xA:
		case 0xD:
		case 0xF:
			return VideoType::SD;
		case 0x9:
		case 0xB:
		case 0xC:
		case 0xE:
			return VideoType::HD;
		}
	}
	return VideoType::Unknown;
}


int FormatEventTime(
	const LibISDB::EventInfo &EventInfo,
	LPTSTR pszTime, int MaxLength, FormatEventTimeFlag Flags)
{
	if (pszTime == nullptr || MaxLength < 1)
		return 0;

	if (!EventInfo.StartTime.IsValid()) {
		pszTime[0] = _T('\0');
		return 0;
	}

	return FormatEventTime(
		EventInfo.StartTime, EventInfo.Duration,
		pszTime, MaxLength, Flags);
}


int FormatEventTime(
	const LibISDB::DateTime &StartTime, DWORD Duration,
	LPTSTR pszTime, int MaxLength, FormatEventTimeFlag Flags)
{
	return FormatEventTime(StartTime.ToSYSTEMTIME(), Duration, pszTime, MaxLength, Flags);
}


int FormatEventTime(
	const SYSTEMTIME &StartTime, DWORD Duration,
	LPTSTR pszTime, int MaxLength, FormatEventTimeFlag Flags)
{
	if (pszTime == nullptr || MaxLength < 1)
		return 0;

	SYSTEMTIME stStart;

	if (!!(Flags & FormatEventTimeFlag::NoConvert)) {
		stStart = StartTime;
	} else {
		EpgTimeToDisplayTime(StartTime, &stStart);
	}

	TCHAR szDate[32];
	if (!!(Flags & FormatEventTimeFlag::Date)) {
		size_t Length = 0;
		if (!!(Flags & FormatEventTimeFlag::Year)) {
			Length = StringFormat(
				szDate, TEXT("{}/"),
				stStart.wYear);
		}
		StringFormat(
			szDate + Length, lengthof(szDate) - Length,
			TEXT("{}/{}({}) "),
			stStart.wMonth,
			stStart.wDay,
			GetDayOfWeekText(stStart.wDayOfWeek));
	} else {
		szDate[0] = _T('\0');
	}

	const LPCTSTR pszTimeFormat =
		!!(Flags & FormatEventTimeFlag::Hour2Digits) ? TEXT("{:02}:{:02}") : TEXT("{}:{:02}");
	TCHAR szStartTime[32], szEndTime[32];

	StringFormat(
		szStartTime,
		pszTimeFormat,
		stStart.wHour,
		stStart.wMinute);

	szEndTime[0] = _T('\0');
	if (!(Flags & FormatEventTimeFlag::StartOnly)) {
		if (Duration > 0) {
			SYSTEMTIME EndTime = stStart;
			if (OffsetSystemTime(&EndTime, Duration * TimeConsts::SYSTEMTIME_SECOND)) {
				StringFormat(
					szEndTime, pszTimeFormat,
					EndTime.wHour, EndTime.wMinute);
			}
		} else {
			if (!!(Flags & FormatEventTimeFlag::UndecidedText))
				StringCopy(szEndTime, TEXT("(終了未定)"));
		}
	}

	return static_cast<int>(StringFormat(
		pszTime, MaxLength, TEXT("{}{}{}{}"),
		szDate,
		szStartTime,
		!(Flags & FormatEventTimeFlag::StartOnly) ? TEXT("～") : TEXT(""),
		szEndTime));
}


bool EpgTimeToDisplayTime(const SYSTEMTIME &EpgTime, SYSTEMTIME *pDisplayTime)
{
	if (pDisplayTime == nullptr)
		return false;

	switch (GetAppClass().EpgOptions.GetEpgTimeMode()) {
	case CEpgOptions::EpgTimeMode::Raw:
		*pDisplayTime = EpgTime;
		return true;

	case CEpgOptions::EpgTimeMode::JST:
		{
			LibISDB::DateTime From, To;
			SYSTEMTIME st;
			TIME_ZONE_INFORMATION tzi;

			From.FromSYSTEMTIME(EpgTime);
			if (!LibISDB::EPGTimeToUTCTime(From, &To))
				return false;
			st = To.ToSYSTEMTIME();
			return GetJSTTimeZoneInformation(&tzi)
				&& ::SystemTimeToTzSpecificLocalTime(&tzi, &st, pDisplayTime);
		}

	case CEpgOptions::EpgTimeMode::Local:
		{
			LibISDB::DateTime From, To;

			From.FromSYSTEMTIME(EpgTime);
			if (!LibISDB::EPGTimeToLocalTime(From, &To))
				return false;
			*pDisplayTime = To.ToSYSTEMTIME();
		}
		return true;

	case CEpgOptions::EpgTimeMode::UTC:
		{
			LibISDB::DateTime From, To;

			From.FromSYSTEMTIME(EpgTime);
			if (!LibISDB::EPGTimeToUTCTime(From, &To))
				return false;
			*pDisplayTime = To.ToSYSTEMTIME();
		}
		return true;
	}

	return false;
}


bool EpgTimeToDisplayTime(const LibISDB::DateTime &EpgTime, LibISDB::DateTime *pDisplayTime)
{
	if (pDisplayTime == nullptr)
		return false;

	SYSTEMTIME st;

	if (!EpgTimeToDisplayTime(EpgTime.ToSYSTEMTIME(), &st))
		return false;

	pDisplayTime->FromSYSTEMTIME(st);

	return true;
}


bool EpgTimeToDisplayTime(SYSTEMTIME *pTime)
{
	if (pTime == nullptr)
		return false;

	SYSTEMTIME st;

	if (!EpgTimeToDisplayTime(*pTime, &st))
		return false;

	*pTime = st;

	return true;
}


bool EpgTimeToDisplayTime(LibISDB::DateTime *pTime)
{
	if (pTime == nullptr)
		return false;

	LibISDB::DateTime Time;

	if (!EpgTimeToDisplayTime(*pTime, &Time))
		return false;

	*pTime = Time;

	return true;
}


bool DisplayTimeToEpgTime(const SYSTEMTIME &DisplayTime, SYSTEMTIME *pEpgTime)
{
	if (pEpgTime == nullptr)
		return false;

	switch (GetAppClass().EpgOptions.GetEpgTimeMode()) {
	case CEpgOptions::EpgTimeMode::Raw:
		*pEpgTime = DisplayTime;
		return true;

	case CEpgOptions::EpgTimeMode::JST:
		{
			SYSTEMTIME st;
			TIME_ZONE_INFORMATION tzi;
			LibISDB::DateTime From, To;

			if (!GetJSTTimeZoneInformation(&tzi)
					|| !::TzSpecificLocalTimeToSystemTime(&tzi, &DisplayTime, &st))
				return false;
			From.FromSYSTEMTIME(st);
			if (!LibISDB::UTCTimeToEPGTime(From, &To))
				return false;
			*pEpgTime = To.ToSYSTEMTIME();
		}
		return true;

	case CEpgOptions::EpgTimeMode::Local:
		{
			SYSTEMTIME st;
			LibISDB::DateTime From, To;

			if (!::TzSpecificLocalTimeToSystemTime(nullptr, &DisplayTime, &st))
				return false;
			From.FromSYSTEMTIME(st);
			if (!LibISDB::UTCTimeToEPGTime(From, &To))
				return false;
			*pEpgTime = To.ToSYSTEMTIME();
		}
		return true;

	case CEpgOptions::EpgTimeMode::UTC:
		{
			LibISDB::DateTime From, To;

			From.FromSYSTEMTIME(DisplayTime);
			if (!LibISDB::UTCTimeToEPGTime(From, &To))
				return false;
			*pEpgTime = To.ToSYSTEMTIME();
		}
		return true;
	}

	return false;
}


bool DisplayTimeToEpgTime(const LibISDB::DateTime &DisplayTime, LibISDB::DateTime *pEpgTime)
{
	if (pEpgTime == nullptr)
		return false;

	SYSTEMTIME st;

	if (!DisplayTimeToEpgTime(DisplayTime.ToSYSTEMTIME(), &st))
		return false;

	pEpgTime->FromSYSTEMTIME(st);

	return true;
}


bool DisplayTimeToEpgTime(SYSTEMTIME *pTime)
{
	if (pTime == nullptr)
		return false;

	SYSTEMTIME st;

	if (!DisplayTimeToEpgTime(*pTime, &st))
		return false;

	*pTime = st;

	return true;
}


bool DisplayTimeToEpgTime(LibISDB::DateTime *pTime)
{
	if (pTime == nullptr)
		return false;

	LibISDB::DateTime Time;

	if (!DisplayTimeToEpgTime(*pTime, &Time))
		return false;

	*pTime = Time;

	return true;
}


bool GetEventGenre(const LibISDB::EventInfo &EventInfo,
				   int *pLevel1, int *pLevel2)
{
	return GetEventGenre(EventInfo.ContentNibble, pLevel1, pLevel2);
}


bool GetEventGenre(
	const LibISDB::EventInfo::ContentNibbleInfo &ContentNibble,
	int *pLevel1, int *pLevel2)
{
	for (int i = 0; i < ContentNibble.NibbleCount; i++) {
		if (ContentNibble.NibbleList[i].ContentNibbleLevel1 != 0xE) {
			if (pLevel1 != nullptr)
				*pLevel1 = ContentNibble.NibbleList[i].ContentNibbleLevel1;
			if (pLevel2 != nullptr)
				*pLevel2 = ContentNibble.NibbleList[i].ContentNibbleLevel2;
			return true;
		}
	}

	if (pLevel1 != nullptr)
		*pLevel1 = -1;
	if (pLevel2 != nullptr)
		*pLevel2 = -1;

	return false;
}


String GetEventDisplayText(const LibISDB::EventInfo &EventInfo, bool fUseARIBSymbol)
{
	if (!EventInfo.EventText.empty()) {
		LPCTSTR p = EventInfo.EventText.c_str();
		while (*p != '\0') {
			if (*p <= 0x20) {
				p++;
				continue;
			}

			if (fUseARIBSymbol)
				return MapARIBSymbol(p);
			else
				return String(p);
		}
	}

	if (!EventInfo.ExtendedText.empty()) {
		String Text;

		for (auto it = EventInfo.ExtendedText.begin(); it != EventInfo.ExtendedText.end(); ++it) {
			if (!it->Description.empty()
					&& (it != EventInfo.ExtendedText.begin()
						|| it->Description.find(TEXT("番組内容")) == String::npos)) {
				Text += it->Description;
				Text += TEXT("\r\n");
			}

			LPCTSTR p = it->Text.c_str();
			while (*p != '\0' && *p <= 0x20)
				p++;
			Text += p;
			Text += TEXT("\r\n");
		}

		if (fUseARIBSymbol)
			return MapARIBSymbol(Text);
		else
			return Text;
	}

	return String();
}


/*
	[字] のような表記を ARIB 外字に変換する。とりあえず一部の文字のみ。
	LibISDB の ARIBString.cpp で実装すべきか？
*/
size_t MapARIBSymbol(LPCWSTR pszSource, LPWSTR pszDest, size_t DestLength)
{
	if (pszSource == nullptr || pszDest == nullptr || DestLength == 0)
		return 0;

	struct ARIBSymbolMap {
		LPCWSTR pszString;
		LPCWSTR pszSymbol;
	};

	static const ARIBSymbolMap MapList[] = {
		{L"[HV]",       L"\U0001f14a"},
		{L"[SD]",       L"\U0001f14c"},
		{L"[Ｐ]",       L"\U0001f13f"},
		{L"[Ｗ]",       L"\U0001f146"},
		{L"[MV]",       L"\U0001f14b"},
		{L"[手]",       L"\U0001f210"},
		{L"[字]",       L"\U0001f211"},
		{L"[双]",       L"\U0001f212"},
		{L"[デ]",       L"\U0001f213"},
		{L"[Ｓ]",       L"\U0001f142"},
		{L"[二]",       L"\U0001f214"},
		{L"[多]",       L"\U0001f215"},
		{L"[解]",       L"\U0001f216"},
		{L"[SS]",       L"\U0001f14d"},
		{L"[Ｂ]",       L"\U0001f131"},
		{L"[Ｎ]",       L"\U0001f13d"},
		{L"[天]",       L"\U0001f217"},
		{L"[交]",       L"\U0001f218"},
		{L"[映]",       L"\U0001f219"},
		{L"[無]",       L"\U0001f21a"},
		{L"[料]",       L"\U0001f21b"},
		{L"[年齢制限]", L"\u26bf"},
		{L"[前]",       L"\U0001f21c"},
		{L"[後]",       L"\U0001f21d"},
		{L"[再]",       L"\U0001f21e"},
		{L"[新]",       L"\U0001f21f"},
		{L"[初]",       L"\U0001f220"},
		{L"[終]",       L"\U0001f221"},
		{L"[生]",       L"\U0001f222"},
		{L"[販]",       L"\U0001f223"},
		{L"[声]",       L"\U0001f224"},
		{L"[吹]",       L"\U0001f225"},
		{L"[PPV]",      L"\U0001f14e"},
		{L"(秘)",       L"\u3299"},
	};

	LPCWSTR p = pszSource;
	size_t DestPos = 0;

	while (*p != L'\0' && DestPos + 1 < DestLength) {
		bool fMapped = false;

		for (const ARIBSymbolMap &Map : MapList) {
			size_t i;
			for (i = 0; p[i] != L'\0' && p[i] == Map.pszString[i]; i++);
			if (Map.pszString[i] == L'\0') {
				const size_t Length = Map.pszSymbol[1] != L'\0' ? 2 : 1;
				if (DestLength - DestPos <= Length)
					goto End;
				for (size_t j = 0; j < Length; j++)
					pszDest[DestPos++] = Map.pszSymbol[j];
				p += i;
				fMapped = true;
			}
		}

		if (!fMapped)
			pszDest[DestPos++] = *p++;
	}

End:
	pszDest[DestPos] = L'\0';

	return DestPos;
}


size_t MapARIBSymbol(LPCWSTR pszSource, String *pDest)
{
	if (pDest == nullptr)
		return 0;

	if (pszSource == nullptr) {
		pDest->clear();
		return 0;
	}

	pDest->resize(::lstrlenW(pszSource) + 1);
	const size_t Length = MapARIBSymbol(pszSource, pDest->data(), pDest->length());
	pDest->resize(Length);
	return Length;
}


String MapARIBSymbol(LPCWSTR pszSource)
{
	String Text;
	MapARIBSymbol(pszSource, &Text);
	return Text;
}


String MapARIBSymbol(const String &Source)
{
	String Dest;

	Dest.resize(Source.length() + 1);
	const size_t Length = MapARIBSymbol(Source.c_str(), Dest.data(), Dest.length());
	Dest.resize(Length);
	return Dest;
}

}




LPCTSTR CEpgGenre::GetText(int Level1, int Level2) const
{
	static const struct {
		LPCTSTR pszText;
		LPCTSTR pszSubText[16];
	} GenreList[] = {
		{	TEXT("ニュース／報道"),
			{
				TEXT("定時・総合"),
				TEXT("天気"),
				TEXT("特集・ドキュメント"),
				TEXT("政治・国会"),
				TEXT("経済・市況"),
				TEXT("海外・国際"),
				TEXT("解説"),
				TEXT("討論・会談"),
				TEXT("報道特番"),
				TEXT("ローカル・地域"),
				TEXT("交通"),
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				TEXT("その他")
			}
		},
		{	TEXT("スポーツ"),
			{
				TEXT("スポーツニュース"),
				TEXT("野球"),
				TEXT("サッカー"),
				TEXT("ゴルフ"),
				TEXT("その他の球技"),
				TEXT("相撲・格闘技"),
				TEXT("オリンピック・国際大会"),
				TEXT("マラソン・陸上・水泳"),
				TEXT("モータースポーツ"),
				TEXT("マリン・ウィンタースポーツ"),
				TEXT("競馬・公営競技"),
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				TEXT("その他")
			}
		},
		{	TEXT("情報／ワイドショー"),
			{
				TEXT("芸能・ワイドショー"),
				TEXT("ファッション"),
				TEXT("暮らし・住まい"),
				TEXT("健康・医療"),
				TEXT("ショッピング・通販"),
				TEXT("グルメ・料理"),
				TEXT("イベント"),
				TEXT("番組紹介・お知らせ"),
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				TEXT("その他")
			}
		},
		{	TEXT("ドラマ"),
			{
				TEXT("国内ドラマ"),
				TEXT("海外ドラマ"),
				TEXT("時代劇"),
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				TEXT("その他")
			}
		},
		{	TEXT("音楽"),
			{
				TEXT("国内ロック・ポップス"),
				TEXT("海外ロック・ポップス"),
				TEXT("クラシック・オペラ"),
				TEXT("ジャズ・フュージョン"),
				TEXT("歌謡曲・演歌"),
				TEXT("ライブ・コンサート"),
				TEXT("ランキング・リクエスト"),
				TEXT("カラオケ・のど自慢"),
				TEXT("民謡・邦楽"),
				TEXT("童謡・キッズ"),
				TEXT("民族音楽・ワールドミュージック"),
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				TEXT("その他")
			}
		},
		{	TEXT("バラエティ"),
			{
				TEXT("クイズ"),
				TEXT("ゲーム"),
				TEXT("トークバラエティ"),
				TEXT("お笑い・コメディ"),
				TEXT("音楽バラエティ"),
				TEXT("旅バラエティ"),
				TEXT("料理バラエティ"),
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				TEXT("その他")
			}
		},
		{	TEXT("映画"),
			{
				TEXT("洋画"),
				TEXT("邦画"),
				TEXT("アニメ"),
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				TEXT("その他")
			}
		},
		{	TEXT("アニメ／特撮"),
			{
				TEXT("国内アニメ"),
				TEXT("海外アニメ"),
				TEXT("特撮"),
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				TEXT("その他")
			}
		},
		{	TEXT("ドキュメンタリー／教養"),
			{
				TEXT("社会・時事"),
				TEXT("歴史・紀行"),
				TEXT("自然・動物・環境"),
				TEXT("宇宙・科学・医学"),
				TEXT("カルチャー・伝統文化"),
				TEXT("文学・文芸"),
				TEXT("スポーツ"),
				TEXT("ドキュメンタリー全般"),
				TEXT("インタビュー・討論"),
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				TEXT("その他")
			}
		},
		{	TEXT("劇場／公演"),
			{
				TEXT("現代劇・新劇"),
				TEXT("ミュージカル"),
				TEXT("ダンス・バレエ"),
				TEXT("落語・演芸"),
				TEXT("歌舞伎・古典"),
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				TEXT("その他")
			}
		},
		{	TEXT("趣味／教育"),
			{
				TEXT("旅・釣り・アウトドア"),
				TEXT("園芸・ペット・手芸"),
				TEXT("音楽・美術・工芸"),
				TEXT("囲碁・将棋"),
				TEXT("麻雀・パチンコ"),
				TEXT("車・オートバイ"),
				TEXT("コンピュータ・TVゲーム"),
				TEXT("会話・語学"),
				TEXT("幼児・小学生"),
				TEXT("中学生・高校生"),
				TEXT("大学生・受験"),
				TEXT("生涯学習・資格"),
				TEXT("教育問題"),
				nullptr,
				nullptr,
				TEXT("その他")
			}
		},
		{	TEXT("福祉"),
			{
				TEXT("高齢者"),
				TEXT("障害者"),
				TEXT("社会福祉"),
				TEXT("ボランティア"),
				TEXT("手話"),
				TEXT("文字(字幕)"),
				TEXT("音声解説"),
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				TEXT("その他")
			}
		},
	};

	if (Level2 < 0) {
		if (Level1 >= 0 && Level1 < lengthof(GenreList))
			return GenreList[Level1].pszText;
		if (Level1 == GENRE_OTHER)
			return TEXT("その他");
		return nullptr;
	}
	if (Level1 >= 0 && Level1 < lengthof(GenreList)
			&& Level2 >= 0 && Level2 < 16)
		return GenreList[Level1].pszSubText[Level2];
	return nullptr;
}




CEpgIcons::~CEpgIcons()
{
	EndDraw();
}


bool CEpgIcons::Load()
{
	return CBitmap::Load(GetAppClass().GetInstance(), IDB_PROGRAMGUIDEICONS, LR_DEFAULTCOLOR);
}


bool CEpgIcons::BeginDraw(HDC hdc, int IconWidth, int IconHeight)
{
	if (!IsCreated())
		return false;

	EndDraw();

	m_hdc = ::CreateCompatibleDC(hdc);
	if (m_hdc == nullptr)
		return false;
	m_hbmOld = DrawUtil::SelectObject(m_hdc, *this);

	if (IconWidth > 0 && IconHeight > 0
			&& (IconWidth != ICON_WIDTH || IconHeight != ICON_HEIGHT)) {
		m_StretchBuffer.Create((ICON_LAST + 1) * IconWidth, IconHeight, hdc);
		m_StretchedIcons = 0;
	}

	m_IconWidth = IconWidth;
	m_IconHeight = IconHeight;

	return true;
}


void CEpgIcons::EndDraw()
{
	if (m_hdc != nullptr) {
		::SelectObject(m_hdc, m_hbmOld);
		::DeleteDC(m_hdc);
		m_hdc = nullptr;
		m_hbmOld = nullptr;
	}
	m_StretchBuffer.Destroy();
}


bool CEpgIcons::DrawIcon(
	HDC hdcDst, int DstX, int DstY, int Width, int Height,
	int Icon, BYTE Opacity, const RECT *pClipping)
{
	if (m_hdc == nullptr
			|| hdcDst == nullptr
			|| Width <= 0 || Height <= 0
			|| Icon < 0 || Icon > ICON_LAST)
		return false;

	HDC hdcSrc;
	int IconWidth, IconHeight;

	if ((Width != ICON_WIDTH || Height != ICON_HEIGHT)
			&& Opacity < 255
			&& m_StretchBuffer.IsCreated()) {
		hdcSrc = m_StretchBuffer.GetDC();
		IconWidth = m_IconWidth;
		IconHeight = m_IconHeight;
		if ((m_StretchedIcons & IconFlag(Icon)) == 0) {
			const int OldStretchMode = ::SetStretchBltMode(hdcSrc, STRETCH_HALFTONE);
			::StretchBlt(
				hdcSrc, Icon * IconWidth, 0, IconWidth, IconHeight,
				m_hdc, Icon * ICON_WIDTH, 0, ICON_WIDTH, ICON_HEIGHT,
				SRCCOPY);
			::SetStretchBltMode(hdcSrc, OldStretchMode);
			m_StretchedIcons |= IconFlag(Icon);
		}
	} else {
		hdcSrc = m_hdc;
		IconWidth = ICON_WIDTH;
		IconHeight = ICON_HEIGHT;
	}

	RECT rcDraw, rcDst;

	if (pClipping != nullptr) {
		::SetRect(&rcDst, DstX, DstY, DstX + Width, DstY + Height);
		if (!::IntersectRect(&rcDraw, &rcDst, pClipping))
			return true;
	} else {
		::SetRect(&rcDraw, DstX, DstY, DstX + Width, DstY + Height);
	}

	const int DstWidth = rcDraw.right - rcDraw.left;
	const int DstHeight = rcDraw.bottom - rcDraw.top;
	const int SrcX = (IconWidth * (rcDraw.left - DstX) + Width / 2) / Width;
	const int SrcY = (IconHeight * (rcDraw.top - DstY) + Height / 2) / Height;
	int SrcWidth = (IconWidth * (rcDraw.right - DstX) + Width / 2) / Width - SrcX;
	if (SrcWidth < 1)
		SrcWidth = 1;
	int SrcHeight = (IconHeight * (rcDraw.bottom - DstY) + Height / 2) / Height - SrcY;
	if (SrcHeight < 1)
		SrcHeight = 1;

	if (Opacity == 255) {
		if (DstWidth == SrcWidth && DstHeight == SrcHeight) {
			::BitBlt(
				hdcDst, rcDraw.left, rcDraw.top, DstWidth, DstHeight,
				hdcSrc, Icon * IconWidth + SrcX, SrcY, SRCCOPY);
		} else {
			const int OldStretchMode = ::SetStretchBltMode(hdcDst, STRETCH_HALFTONE);
			::StretchBlt(
				hdcDst, rcDraw.left, rcDraw.top, DstWidth, DstHeight,
				hdcSrc, Icon * IconWidth + SrcX, SrcY, SrcWidth, SrcHeight,
				SRCCOPY);
			::SetStretchBltMode(hdcDst, OldStretchMode);
		}
	} else {
		const BLENDFUNCTION bf = {AC_SRC_OVER, 0, Opacity, 0};
		::GdiAlphaBlend(
			hdcDst, rcDraw.left, rcDraw.top, DstWidth, DstHeight,
			hdcSrc, Icon * IconWidth + SrcX, SrcY, SrcWidth, SrcHeight,
			bf);
	}

	return true;
}


bool CEpgIcons::DrawIcons(
	unsigned int IconFlags,
	HDC hdcDst, int DstX, int DstY, int Width, int Height,
	int IntervalX, int IntervalY,
	BYTE Opacity, const RECT *pClipping)
{
	if (IconFlags == 0)
		return false;

	int x = DstX, y = DstY;
	int Icon = 0;

	for (unsigned int Flag = IconFlags; Flag != 0; Flag >>= 1) {
		if (pClipping != nullptr
				&& (x >= pClipping->right || y >= pClipping->bottom))
			break;
		if ((Flag & 1) != 0) {
			DrawIcon(hdcDst, x, y, Width, Height, Icon, Opacity, pClipping);
			x += IntervalX;
			y += IntervalY;
		}
		Icon++;
	}

	return true;
}


unsigned int CEpgIcons::GetEventIcons(const LibISDB::EventInfo *pEventInfo)
{
	unsigned int ShowIcons = 0;

	if (!pEventInfo->VideoList.empty()) {
		const EpgUtil::VideoType Video = EpgUtil::GetVideoType(pEventInfo->VideoList[0].ComponentType);
		if (Video == EpgUtil::VideoType::HD)
			ShowIcons |= IconFlag(ICON_HD);
		else if (Video == EpgUtil::VideoType::SD)
			ShowIcons |= IconFlag(ICON_SD);
	}

	if (!pEventInfo->AudioList.empty()) {
		const LibISDB::EventInfo::AudioInfo *pAudioInfo = pEventInfo->GetMainAudioInfo();

		if (pAudioInfo->ComponentType == 0x02) {
			if (pAudioInfo->ESMultiLingualFlag
					&& pAudioInfo->LanguageCode != pAudioInfo->LanguageCode2)
				ShowIcons |= IconFlag(ICON_MULTILINGUAL);
			else
				ShowIcons |= IconFlag(ICON_SUB);
		} else {
			if (pAudioInfo->ComponentType == 0x09)
				ShowIcons |= IconFlag(ICON_5_1CH);
			if (pEventInfo->AudioList.size() >= 2
					&& pEventInfo->AudioList[0].LanguageCode != 0
					&& pEventInfo->AudioList[1].LanguageCode != 0) {
				if (pEventInfo->AudioList[0].LanguageCode !=
						pEventInfo->AudioList[1].LanguageCode)
					ShowIcons |= IconFlag(ICON_MULTILINGUAL);
				else
					ShowIcons |= IconFlag(ICON_SUB);
			}
		}
	}

	if (GetAppClass().NetworkDefinition.IsSatelliteNetworkID(pEventInfo->NetworkID)) {
		if (pEventInfo->FreeCAMode)
			ShowIcons |= IconFlag(ICON_PAY);
		else
			ShowIcons |= IconFlag(ICON_FREE);
	}

	return ShowIcons;
}




CEpgTheme::CEpgTheme()
{
	m_ColorList[COLOR_EVENTNAME].Set(0, 0, 0);
	m_ColorList[COLOR_EVENTTEXT].Set(0, 0, 0);
	for (int i = COLOR_CONTENT_FIRST; i <= COLOR_CONTENT_LAST; i++)
		m_ColorList[i].Set(240, 240, 240);
	m_ColorList[COLOR_CONTENT_NEWS       ].Set(255, 255, 224);
	m_ColorList[COLOR_CONTENT_SPORTS     ].Set(255, 255, 224);
//	m_ColorList[COLOR_CONTENT_INFORMATION].Set(255, 255, 224);
	m_ColorList[COLOR_CONTENT_DRAMA      ].Set(255, 224, 224);
	m_ColorList[COLOR_CONTENT_MUSIC      ].Set(224, 255, 224);
	m_ColorList[COLOR_CONTENT_VARIETY    ].Set(224, 224, 255);
	m_ColorList[COLOR_CONTENT_MOVIE      ].Set(224, 255, 255);
	m_ColorList[COLOR_CONTENT_ANIME      ].Set(255, 224, 255);
	m_ColorList[COLOR_CONTENT_DOCUMENTARY].Set(255, 255, 224);
	m_ColorList[COLOR_CONTENT_THEATER    ].Set(224, 255, 255);
}


void CEpgTheme::SetTheme(const Theme::CThemeManager *pThemeManager)
{
	m_ColorList[COLOR_EVENTNAME] =
		pThemeManager->GetColor(CColorScheme::COLOR_PROGRAMGUIDE_EVENTTITLE);
	m_ColorList[COLOR_EVENTTEXT] =
		pThemeManager->GetColor(CColorScheme::COLOR_PROGRAMGUIDE_TEXT);

	for (int i = COLOR_CONTENT_FIRST, j = 0; i <= COLOR_CONTENT_LAST; i++, j++) {
		m_ColorList[i] =
			pThemeManager->GetColor(CColorScheme::COLOR_PROGRAMGUIDE_CONTENT_FIRST + j);
	}
}


bool CEpgTheme::SetColor(int Type, const Theme::ThemeColor &Color)
{
	if (Type < 0 || Type >= NUM_COLORS)
		return false;
	m_ColorList[Type] = Color;
	return true;
}


Theme::ThemeColor CEpgTheme::GetColor(int Type) const
{
	if (Type < 0 || Type >= NUM_COLORS)
		return Theme::ThemeColor();
	return m_ColorList[Type];
}


Theme::ThemeColor CEpgTheme::GetGenreColor(int Genre) const
{
	return m_ColorList[
		Genre >= 0 && Genre <= CEpgGenre::GENRE_LAST ?
			COLOR_CONTENT_FIRST + Genre :
			COLOR_CONTENT_OTHER];
}


Theme::ThemeColor CEpgTheme::GetGenreColor(const LibISDB::EventInfo &EventInfo) const
{
	int Genre;

	if (!EpgUtil::GetEventGenre(EventInfo, &Genre))
		return m_ColorList[COLOR_CONTENT_OTHER];

	return GetGenreColor(Genre);
}


Theme::BackgroundStyle CEpgTheme::GetContentBackgroundStyle(
	int Genre, ContentStyleFlag Flags) const
{
	return GetContentBackgroundStyle(GetGenreColor(Genre), Flags);
}


Theme::BackgroundStyle CEpgTheme::GetContentBackgroundStyle(
	const LibISDB::EventInfo &EventInfo, ContentStyleFlag Flags) const
{
	return GetContentBackgroundStyle(GetGenreColor(EventInfo), Flags);
}


bool CEpgTheme::DrawContentBackground(
	HDC hdc, Theme::CThemeDraw &ThemeDraw, const RECT &Rect,
	const LibISDB::EventInfo &EventInfo, DrawContentBackgroundFlag Flags) const
{
	if (hdc == nullptr)
		return false;

	ContentStyleFlag StyleFlags = ContentStyleFlag::None;
	if (!!(Flags & DrawContentBackgroundFlag::Current))
		StyleFlags |= ContentStyleFlag::Current;
	if (!!(Flags & DrawContentBackgroundFlag::NoBorder))
		StyleFlags |= ContentStyleFlag::NoBorder;
	ThemeDraw.Draw(GetContentBackgroundStyle(EventInfo, StyleFlags), Rect);

	if (!!(Flags & DrawContentBackgroundFlag::Separator)) {
		RECT rc = Rect;
		rc.bottom = rc.top + ThemeDraw.GetStyleScaling()->ToPixels(1, Style::UnitType::LogicalPixel);
		DrawUtil::Fill(hdc, &rc, MixColor(GetGenreColor(EventInfo), RGB(0, 0, 0), 224));
	}

	return true;
}


Theme::BackgroundStyle CEpgTheme::GetContentBackgroundStyle(
	const Theme::ThemeColor &Color, ContentStyleFlag Flags) const
{
	Theme::BackgroundStyle BackStyle;

	if (!(Flags & ContentStyleFlag::Current)) {
		BackStyle.Fill.Type = Theme::FillType::Solid;
		BackStyle.Fill.Solid = Theme::SolidStyle(Color);
	} else {
		double h, s, v, s1, v1;
		RGBToHSV(Color.Red, Color.Green, Color.Blue, &h, &s, &v);
		s1 = s;
		v1 = v;
		if (s1 < 0.1 || v1 <= 0.95) {
			v1 += 0.05;
			if (v1 > 1.0)
				v1 = 1.0;
		} else {
			s1 -= s * 0.5;
			if (s1 < 0.0)
				s1 = 0.0;
		}
		const Theme::ThemeColor Color1 = HSVToRGB(h, s1, v1);
		s1 = s;
		v1 = v;
		if (s1 >= 0.1 && s1 <= 0.9) {
			s1 += s * 0.5;
			if (s1 > 1.0)
				s1 = 1.0;
		} else {
			v1 -= 0.05;
			if (v1 < 0.0)
				v1 = 0.0;
		}
		const Theme::ThemeColor Color2 = HSVToRGB(h, s1, v1);
		BackStyle.Fill.Type = Theme::FillType::Gradient;
		BackStyle.Fill.Gradient = Theme::GradientStyle(
			Theme::GradientType::Normal,
			Theme::GradientDirection::Vert,
			Color1, Color2);
	}

#if 0
	if (!!(Flags & ContentStyleFlag::NoBorder))
		BackStyle.Border.Type = Theme::BorderType::None;
#endif

	return BackStyle;
}


}	// namespace TVTest
