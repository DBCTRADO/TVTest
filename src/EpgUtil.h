#ifndef EPG_UTIL_H
#define EPG_UTIL_H


#include "EpgProgramList.h"
#include "DrawUtil.h"


namespace EpgUtil
{

	enum VideoType {
		VIDEO_TYPE_UNKNOWN,
		VIDEO_TYPE_HD,
		VIDEO_TYPE_SD
	};

	VideoType GetVideoType(BYTE ComponentType);
	LPCTSTR GetVideoComponentTypeText(BYTE ComponentType);
	LPCTSTR GetAudioComponentTypeText(BYTE ComponentType);

	enum {
		EVENT_TIME_HOUR_2DIGITS   = 0x0001U,
		EVENT_TIME_START_ONLY     = 0x0002U,
		EVENT_TIME_DATE           = 0x0004U,
		EVENT_TIME_YEAR           = 0x0008U,
		EVENT_TIME_UNDECIDED_TEXT = 0x0010U
	};
	enum {
		MAX_EVENT_TIME_LENGTH = 64
	};

	int FormatEventTime(const CEventInfoData *pEventInfo,
						LPTSTR pszTime,int MaxLength,unsigned int Flags=0);
	int FormatEventTime(const SYSTEMTIME &StartTime,DWORD Duration,
						LPTSTR pszTime,int MaxLength,unsigned int Flags=0);

	enum LanguageTextType {
		LANGUAGE_TEXT_LONG,
		LANGUAGE_TEXT_SIMPLE,
		LANGUAGE_TEXT_SHORT
	};

	LPCTSTR GetLanguageText(DWORD LanguageCode,LanguageTextType Type=LANGUAGE_TEXT_LONG);

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
		GENRE_OTHER
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
		ICON_WIDTH	=11,
		ICON_HEIGHT	=11
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

	bool Load();
	static bool Draw(HDC hdcDst,int DstX,int DstY,
					 HDC hdcSrc,int Icon,int Width,int Height,BYTE Opacity=255);
	static unsigned int GetEventIcons(const CEventInfoData *pEventInfo);
};


#endif
