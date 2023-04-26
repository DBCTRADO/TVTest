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


#ifndef TVTEST_EPG_UTIL_H
#define TVTEST_EPG_UTIL_H


#include "DrawUtil.h"
#include "ThemeManager.h"
#include "ThemeDraw.h"
#include "LibISDB/LibISDB/EPG/EventInfo.hpp"
#include "LibISDB/LibISDB/TS/TSInformation.hpp"


namespace TVTest
{

	namespace EpgUtil
	{

		enum class VideoType {
			Unknown,
			HD,
			SD,
		};

		VideoType GetVideoType(BYTE ComponentType);

		enum class FormatEventTimeFlag : unsigned int {
			None          = 0x0000U,
			Hour2Digits   = 0x0001U,
			StartOnly     = 0x0002U,
			Date          = 0x0004U,
			Year          = 0x0008U,
			UndecidedText = 0x0010U,
			NoConvert     = 0x0020U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		static constexpr int MAX_EVENT_TIME_LENGTH = 64;

		int FormatEventTime(
			const LibISDB::EventInfo &EventInfo,
			LPTSTR pszTime, int MaxLength, FormatEventTimeFlag Flags = FormatEventTimeFlag::None);
		int FormatEventTime(
			const LibISDB::DateTime &StartTime, DWORD Duration,
			LPTSTR pszTime, int MaxLength, FormatEventTimeFlag Flags = FormatEventTimeFlag::None);
		int FormatEventTime(
			const SYSTEMTIME &StartTime, DWORD Duration,
			LPTSTR pszTime, int MaxLength, FormatEventTimeFlag Flags = FormatEventTimeFlag::None);
		bool EpgTimeToDisplayTime(const SYSTEMTIME &EpgTime, SYSTEMTIME *pDisplayTime);
		bool EpgTimeToDisplayTime(const LibISDB::DateTime &EpgTime, LibISDB::DateTime *pDisplayTime);
		bool EpgTimeToDisplayTime(SYSTEMTIME *pTime);
		bool EpgTimeToDisplayTime(LibISDB::DateTime *pTime);
		bool DisplayTimeToEpgTime(const SYSTEMTIME &DisplayTime, SYSTEMTIME *pEpgTime);
		bool DisplayTimeToEpgTime(const LibISDB::DateTime &DisplayTime, LibISDB::DateTime *pEpgTime);
		bool DisplayTimeToEpgTime(SYSTEMTIME *pTime);
		bool DisplayTimeToEpgTime(LibISDB::DateTime *pTime);

		bool GetEventGenre(
			const LibISDB::EventInfo &EventInfo,
			int *pLevel1, int *pLevel2 = nullptr);
		bool GetEventGenre(
			const LibISDB::EventInfo::ContentNibbleInfo &ContentNibble,
			int *pLevel1, int *pLevel2 = nullptr);

		String GetEventDisplayText(const LibISDB::EventInfo &EventInfo, bool fUseARIBSymbol = false);

		size_t MapARIBSymbol(LPCWSTR pszSource, LPWSTR pszDest, size_t DestLength);
		size_t MapARIBSymbol(LPCWSTR pszSource, String *pDest);
		String MapARIBSymbol(LPCWSTR pszSource);
		String MapARIBSymbol(const String &Source);

	}


	class CEpgGenre
	{
	public:
		enum {
			GENRE_NEWS,
			GENRE_SPORTS,
			GENRE_INFORMATION,
			GENRE_DRAMA,
			GENRE_MUSIC,
			GENRE_VARIETY,
			GENRE_MOVIE,
			GENRE_ANIME,
			GENRE_DOCUMENTARY,
			GENRE_THEATER,
			GENRE_EDUCATION,
			GENRE_WELFARE,
			GENRE_RESERVED1,
			GENRE_RESERVED2,
			GENRE_EXTEND,
			GENRE_OTHER,
			GENRE_LAST = GENRE_WELFARE
		};

		static constexpr int NUM_GENRE     = 16;
		static constexpr int NUM_SUB_GENRE = 16;

		LPCTSTR GetText(int Level1, int Level2) const;
	};

	class CEpgIcons
		: public DrawUtil::CBitmap
	{
	public:
		static constexpr int ICON_WIDTH  = 22;
		static constexpr int ICON_HEIGHT = 22;
		static constexpr int DEFAULT_ICON_WIDTH  = 11;
		static constexpr int DEFAULT_ICON_HEIGHT = 11;

		enum {
			ICON_HD,
			ICON_SD,
			ICON_5_1CH,
			ICON_MULTILINGUAL,
			ICON_SUB,
			ICON_FREE,
			ICON_PAY,
			ICON_LAST = ICON_PAY
		};

		static UINT IconFlag(int Icon) { return 1 << Icon; }

		CEpgIcons() = default;
		~CEpgIcons();

		CEpgIcons(const CEpgIcons &) = delete;
		CEpgIcons &operator=(const CEpgIcons &) = delete;

		bool Load();
		bool BeginDraw(HDC hdc, int IconWidth = 0, int IconHeight = 0);
		void EndDraw();
		bool DrawIcon(
			HDC hdcDst, int DstX, int DstY, int Width, int Height,
			int Icon, BYTE Opacity = 255, const RECT *pClipping = nullptr);
		bool DrawIcons(
			unsigned int IconFlags,
			HDC hdcDst, int DstX, int DstY, int Width, int Height,
			int IntervalX, int IntervalY,
			BYTE Opacity = 255, const RECT *pClipping = nullptr);

		static unsigned int GetEventIcons(const LibISDB::EventInfo *pEventInfo);

	protected:
		HDC m_hdc = nullptr;
		HBITMAP m_hbmOld = nullptr;
		int m_IconWidth = 0;
		int m_IconHeight = 0;
		DrawUtil::COffscreen m_StretchBuffer;
		unsigned int m_StretchedIcons = 0;
	};

	class CEpgTheme
	{
	public:
		enum {
			COLOR_EVENTNAME,
			COLOR_EVENTTEXT,
			COLOR_CONTENT_NEWS,
			COLOR_CONTENT_SPORTS,
			COLOR_CONTENT_INFORMATION,
			COLOR_CONTENT_DRAMA,
			COLOR_CONTENT_MUSIC,
			COLOR_CONTENT_VARIETY,
			COLOR_CONTENT_MOVIE,
			COLOR_CONTENT_ANIME,
			COLOR_CONTENT_DOCUMENTARY,
			COLOR_CONTENT_THEATER,
			COLOR_CONTENT_EDUCATION,
			COLOR_CONTENT_WELFARE,
			COLOR_CONTENT_OTHER,
			COLOR_CONTENT_FIRST = COLOR_CONTENT_NEWS,
			COLOR_CONTENT_LAST = COLOR_CONTENT_OTHER,
			COLOR_LAST = COLOR_CONTENT_LAST,
			NUM_COLORS = COLOR_LAST + 1
		};

		enum class ContentStyleFlag : unsigned int {
			None     = 0x0000U,
			Current  = 0x0001U,
			NoBorder = 0x0002U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		enum class DrawContentBackgroundFlag : unsigned int {
			None      = 0x0000U,
			Current   = 0x0001U,
			Separator = 0x0002U,
			NoBorder  = 0x0004U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		CEpgTheme();

		void SetTheme(const Theme::CThemeManager *pThemeManager);
		bool SetColor(int Type, const Theme::ThemeColor &Color);
		Theme::ThemeColor GetColor(int Type) const;
		Theme::ThemeColor GetGenreColor(int Genre) const;
		Theme::ThemeColor GetGenreColor(const LibISDB::EventInfo &EventInfo) const;
		Theme::BackgroundStyle GetContentBackgroundStyle(
			int Genre, ContentStyleFlag Flags = ContentStyleFlag::None) const;
		Theme::BackgroundStyle GetContentBackgroundStyle(
			const LibISDB::EventInfo &EventInfo, ContentStyleFlag Flags = ContentStyleFlag::None) const;
		bool DrawContentBackground(
			HDC hdc, Theme::CThemeDraw &ThemeDraw, const RECT &Rect,
			const LibISDB::EventInfo &EventInfo,
			DrawContentBackgroundFlag Flags = DrawContentBackgroundFlag::None) const;

	private:
		Theme::BackgroundStyle GetContentBackgroundStyle(
			const Theme::ThemeColor &Color, ContentStyleFlag Flags) const;

		Theme::ThemeColor m_ColorList[NUM_COLORS];
	};

}	// namespace TVTest


#endif
