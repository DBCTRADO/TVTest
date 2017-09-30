#ifndef EPG_UTIL_H
#define EPG_UTIL_H


#include "DrawUtil.h"
#include "ThemeManager.h"
#include "ThemeDraw.h"
#include "LibISDB/LibISDB/EPG/EventInfo.hpp"
#include "LibISDB/LibISDB/TS/TSInformation.hpp"


namespace EpgUtil
{

	enum VideoType {
		VIDEO_TYPE_UNKNOWN,
		VIDEO_TYPE_HD,
		VIDEO_TYPE_SD
	};

	VideoType GetVideoType(BYTE ComponentType);

	enum {
		EVENT_TIME_HOUR_2DIGITS   = 0x0001U,
		EVENT_TIME_START_ONLY     = 0x0002U,
		EVENT_TIME_DATE           = 0x0004U,
		EVENT_TIME_YEAR           = 0x0008U,
		EVENT_TIME_UNDECIDED_TEXT = 0x0010U,
		EVENT_TIME_NO_CONVERT     = 0x0020U
	};
	enum {
		MAX_EVENT_TIME_LENGTH = 64
	};

	int FormatEventTime(const LibISDB::EventInfo &EventInfo,
						LPTSTR pszTime,int MaxLength,unsigned int Flags=0);
	int FormatEventTime(const LibISDB::DateTime &StartTime,DWORD Duration,
						LPTSTR pszTime,int MaxLength,unsigned int Flags=0);
	int FormatEventTime(const SYSTEMTIME &StartTime,DWORD Duration,
						LPTSTR pszTime,int MaxLength,unsigned int Flags=0);
	bool EpgTimeToDisplayTime(const SYSTEMTIME &EpgTime,SYSTEMTIME *pDisplayTime);
	bool EpgTimeToDisplayTime(const LibISDB::DateTime &EpgTime,LibISDB::DateTime *pDisplayTime);
	bool EpgTimeToDisplayTime(SYSTEMTIME *pTime);
	bool EpgTimeToDisplayTime(LibISDB::DateTime *pTime);
	bool DisplayTimeToEpgTime(const SYSTEMTIME &DisplayTime,SYSTEMTIME *pEpgTime);
	bool DisplayTimeToEpgTime(const LibISDB::DateTime &DisplayTime,LibISDB::DateTime *pEpgTime);
	bool DisplayTimeToEpgTime(SYSTEMTIME *pTime);
	bool DisplayTimeToEpgTime(LibISDB::DateTime *pTime);

	bool GetEventGenre(const LibISDB::EventInfo &EventInfo,
					   int *pLevel1,int *pLevel2=nullptr);
	bool GetEventGenre(const LibISDB::EventInfo::ContentNibbleInfo &ContentNibble,
					   int *pLevel1,int *pLevel2=nullptr);

	TVTest::String GetEventDisplayText(const LibISDB::EventInfo &EventInfo);

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
		GENRE_LAST=GENRE_WELFARE
	};
	enum {
		NUM_GENRE=16,
		NUM_SUB_GENRE=16
	};

	LPCTSTR GetText(int Level1,int Level2) const;
};

class CEpgIcons : public DrawUtil::CBitmap
{
public:
	enum {
		ICON_WIDTH  = 22,
		ICON_HEIGHT = 22,
		DEFAULT_ICON_WIDTH  = 11,
		DEFAULT_ICON_HEIGHT = 11
	};

	enum {
		ICON_HD,
		ICON_SD,
		ICON_5_1CH,
		ICON_MULTILINGUAL,
		ICON_SUB,
		ICON_FREE,
		ICON_PAY,
		ICON_LAST=ICON_PAY
	};

	static UINT IconFlag(int Icon) { return 1<<Icon; }

	CEpgIcons();
	~CEpgIcons();
	bool Load();
	bool BeginDraw(HDC hdc,int IconWidth=0,int IconHeight=0);
	void EndDraw();
	bool DrawIcon(
		HDC hdcDst,int DstX,int DstY,int Width,int Height,
		int Icon,BYTE Opacity=255,const RECT *pClipping=nullptr);
	bool DrawIcons(
		unsigned int IconFlags,
		HDC hdcDst,int DstX,int DstY,int Width,int Height,
		int IntervalX,int IntervalY,
		BYTE Opacity=255,const RECT *pClipping=nullptr);

	static unsigned int GetEventIcons(const LibISDB::EventInfo *pEventInfo);

protected:
	HDC m_hdc;
	HBITMAP m_hbmOld;
	int m_IconWidth;
	int m_IconHeight;
	DrawUtil::COffscreen m_StretchBuffer;
	unsigned int m_StretchedIcons;
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
		COLOR_CONTENT_FIRST=COLOR_CONTENT_NEWS,
		COLOR_CONTENT_LAST=COLOR_CONTENT_OTHER,
		COLOR_LAST=COLOR_CONTENT_LAST,
		NUM_COLORS=COLOR_LAST+1
	};

	enum {
		CONTENT_STYLE_CURRENT	=0x0001U,
		CONTENT_STYLE_NOBORDER	=0x0002U
	};
	enum {
		DRAW_CONTENT_BACKGROUND_CURRENT		=0x0001U,
		DRAW_CONTENT_BACKGROUND_SEPARATOR	=0x0002U,
		DRAW_CONTENT_BACKGROUND_NOBORDER	=0x0004U
	};

	CEpgTheme();
	void SetTheme(const TVTest::Theme::CThemeManager *pThemeManager);
	bool SetColor(int Type,const TVTest::Theme::ThemeColor &Color);
	TVTest::Theme::ThemeColor GetColor(int Type) const;
	TVTest::Theme::ThemeColor GetGenreColor(int Genre) const;
	TVTest::Theme::ThemeColor GetGenreColor(const LibISDB::EventInfo &EventInfo) const;
	TVTest::Theme::BackgroundStyle GetContentBackgroundStyle(
		int Genre,unsigned int Flags=0) const;
	TVTest::Theme::BackgroundStyle GetContentBackgroundStyle(
		const LibISDB::EventInfo &EventInfo,unsigned int Flags=0) const;
	bool DrawContentBackground(HDC hdc,TVTest::Theme::CThemeDraw &ThemeDraw,const RECT &Rect,
							   const LibISDB::EventInfo &EventInfo,unsigned int Flags=0) const;

private:
	TVTest::Theme::BackgroundStyle GetContentBackgroundStyle(
		const TVTest::Theme::ThemeColor &Color,unsigned int Flags) const;

	TVTest::Theme::ThemeColor m_ColorList[NUM_COLORS];
};


#endif
