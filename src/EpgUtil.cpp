#include "stdafx.h"
#include "TVTest.h"
#include "EpgUtil.h"
#include "AppMain.h"
#include "resource.h"
#include "Common/DebugDef.h"




namespace EpgUtil
{

	VideoType GetVideoType(BYTE ComponentType)
	{
		if ((ComponentType&0x0F)>=1 && (ComponentType&0x0F)<=4) {
			switch (ComponentType>>4) {
			case 0x0:
			case 0xA:
			case 0xD:
			case 0xF:
				return VIDEO_TYPE_SD;
			case 0x9:
			case 0xB:
			case 0xC:
			case 0xE:
				return VIDEO_TYPE_HD;
			}
		}
		return VIDEO_TYPE_UNKNOWN;
	}


	LPCTSTR GetComponentTypeText(BYTE StreamContent,BYTE ComponentType)
	{
		switch (StreamContent) {
		case 0x01:
		case 0x05:
			return GetVideoComponentTypeText(ComponentType);

		case 0x02:
			return GetAudioComponentTypeText(ComponentType);
		}

		return NULL;
	}


	LPCTSTR GetVideoComponentTypeText(BYTE ComponentType)
	{
		static const struct {
			BYTE ComponentType;
			LPCTSTR pszText;
		} VideoComponentTypeList[] = {
			{0x01,TEXT("480i[4:3]")},
			{0x02,TEXT("480i[16:9]")},	// パンベクトルあり
			{0x03,TEXT("480i[16:9]")},	// パンベクトルなし
			{0x04,TEXT("480i[>16:9]")},
			{0x91,TEXT("2160p[4:3]")},
			{0x92,TEXT("2160p[16:9]")},	// パンベクトルあり
			{0x93,TEXT("2160p[16:9]")},	// パンベクトルなし
			{0x94,TEXT("2160p[>16:9]")},
			{0xA1,TEXT("480p[4:3]")},
			{0xA2,TEXT("480p[16:9]")},	// パンベクトルあり
			{0xA3,TEXT("480p[16:9]")},	// パンベクトルなし
			{0xA4,TEXT("480p[>16:9]")},
			{0xB1,TEXT("1080i[4:3]")},
			{0xB2,TEXT("1080i[16:9]")},	// パンベクトルあり
			{0xB3,TEXT("1080i[16:9]")},	// パンベクトルなし
			{0xB4,TEXT("1080i[>16:9]")},
			{0xC1,TEXT("720p[4:3]")},
			{0xC2,TEXT("720p[16:9]")},	// パンベクトルあり
			{0xC3,TEXT("720p[16:9]")},	// パンベクトルなし
			{0xC4,TEXT("720p[>16:9]")},
			{0xD1,TEXT("240p[4:3]")},
			{0xD2,TEXT("240p[16:9]")},	// パンベクトルあり
			{0xD3,TEXT("240p[16:9]")},	// パンベクトルなし
			{0xD4,TEXT("240p[>16:9]")},
			{0xE1,TEXT("1080p[4:3]")},
			{0xE2,TEXT("1080p[16:9]")},	// パンベクトルあり
			{0xE3,TEXT("1080p[16:9]")},	// パンベクトルなし
			{0xE4,TEXT("1080p[>16:9]")},
			{0xF1,TEXT("180p[4:3]")},
			{0xF2,TEXT("180p[16:9]")},	// パンベクトルあり
			{0xF3,TEXT("180p[16:9]")},	// パンベクトルなし
			{0xF4,TEXT("180p[>16:9]")},
		};

		for (int i=0;i<lengthof(VideoComponentTypeList);i++) {
			if (VideoComponentTypeList[i].ComponentType==ComponentType)
				return VideoComponentTypeList[i].pszText;
		}

		return NULL;
	}


	LPCTSTR GetAudioComponentTypeText(BYTE ComponentType)
	{
		static const struct {
			BYTE ComponentType;
			LPCTSTR pszText;
		} AudioComponentTypeList[] = {
			{0x01,TEXT("Mono")},					// 1/0
			{0x02,TEXT("Dual mono")},				// 1/0 + 1/0
			{0x03,TEXT("Stereo")},					// 2/0
			{0x04,TEXT("3ch[2/1]")},
			{0x05,TEXT("3ch[3/0]")},
			{0x06,TEXT("4ch[2/2]")},
			{0x07,TEXT("4ch[3/1]")},
			{0x08,TEXT("5ch")},						// 3/2
			{0x09,TEXT("5.1ch")},					// 3/2.1
			{0x0A,TEXT("6.1ch[3/3.1]")},
			{0x0B,TEXT("6.1ch[2/0/0-2/0/2-0.1]")},
			{0x0C,TEXT("7.1ch[5/2.1]")},
			{0x0D,TEXT("7.1ch[3/2/2.1]")},
			{0x0E,TEXT("7.1ch[2/0/0-3/0/2-0.1]")},
			{0x0F,TEXT("7.1ch[0/2/0-3/0/2-0.1]")},
			{0x10,TEXT("10.2ch")},					// 2/0/0-3/2/3-0.2
			{0x11,TEXT("22.2ch")},					// 3/3/3-5/2/3-3/0/0.2
			{0x40,TEXT("視覚障害者用音声解説")},
			{0x41,TEXT("聴覚障害者用音声")},
		};

		for (int i=0;i<lengthof(AudioComponentTypeList);i++) {
			if (AudioComponentTypeList[i].ComponentType==ComponentType)
				return AudioComponentTypeList[i].pszText;
		}

		return NULL;
	}


	int FormatEventTime(const CEventInfoData *pEventInfo,
						LPTSTR pszTime,int MaxLength,unsigned int Flags)
	{
		if (pszTime==NULL || MaxLength<1)
			return 0;

		if (pEventInfo==NULL || !pEventInfo->m_bValidStartTime) {
			pszTime[0]=_T('\0');
			return 0;
		}

		return FormatEventTime(pEventInfo->m_StartTime,pEventInfo->m_Duration,
							   pszTime,MaxLength,Flags);
	}


	int FormatEventTime(const SYSTEMTIME &StartTime,DWORD Duration,
						LPTSTR pszTime,int MaxLength,unsigned int Flags)
	{
		if (pszTime==NULL || MaxLength<1)
			return 0;

		SYSTEMTIME stStart;

		if ((Flags & EVENT_TIME_NO_CONVERT)!=0) {
			stStart=StartTime;
		} else {
			EpgTimeToDisplayTime(StartTime,&stStart);
		}

		TCHAR szDate[32];
		if ((Flags & EVENT_TIME_DATE)!=0) {
			int Length=0;
			if ((Flags & EVENT_TIME_YEAR)!=0) {
				Length=StdUtil::snprintf(szDate,lengthof(szDate),TEXT("%d/"),
										 stStart.wYear);
			}
			StdUtil::snprintf(szDate+Length,lengthof(szDate)-Length,
							  TEXT("%d/%d(%s) "),
							  stStart.wMonth,
							  stStart.wDay,
							  GetDayOfWeekText(stStart.wDayOfWeek));
		} else {
			szDate[0]=_T('\0');
		}

		LPCTSTR pszTimeFormat=
			(Flags & EVENT_TIME_HOUR_2DIGITS)!=0?TEXT("%02d:%02d"):TEXT("%d:%02d");
		TCHAR szStartTime[32],szEndTime[32];

		StdUtil::snprintf(szStartTime,lengthof(szStartTime),
						  pszTimeFormat,
						  stStart.wHour,
						  stStart.wMinute);

		szEndTime[0]=_T('\0');
		if ((Flags & EVENT_TIME_START_ONLY)==0) {
			if (Duration>0) {
				SYSTEMTIME EndTime=stStart;
				if (OffsetSystemTime(&EndTime,Duration*TimeConsts::SYSTEMTIME_SECOND)) {
					StdUtil::snprintf(szEndTime,lengthof(szEndTime),pszTimeFormat,
									  EndTime.wHour,EndTime.wMinute);
				}
			} else {
				if ((Flags & EVENT_TIME_UNDECIDED_TEXT)!=0)
					::lstrcpy(szEndTime,TEXT("(終了未定)"));
			}
		}

		return StdUtil::snprintf(pszTime,MaxLength,TEXT("%s%s%s%s"),
								 szDate,
								 szStartTime,
								 (Flags & EVENT_TIME_START_ONLY)==0?TEXT("～"):TEXT(""),
								 szEndTime);
	}


	bool EpgTimeToDisplayTime(const SYSTEMTIME &EpgTime,SYSTEMTIME *pDisplayTime)
	{
		if (pDisplayTime==NULL)
			return false;

		switch (GetAppClass().EpgOptions.GetEpgTimeMode()) {
		case CEpgOptions::EPGTIME_RAW:
			*pDisplayTime=EpgTime;
			return true;

		case CEpgOptions::EPGTIME_JST:
			{
				SYSTEMTIME st;
				TIME_ZONE_INFORMATION tzi;

				return EpgTimeToUtc(&EpgTime,&st)
					&& GetJSTTimeZoneInformation(&tzi)
					&& ::SystemTimeToTzSpecificLocalTime(&tzi,&st,pDisplayTime);
			}

		case CEpgOptions::EPGTIME_LOCAL:
			return EpgTimeToLocalTime(&EpgTime,pDisplayTime);

		case CEpgOptions::EPGTIME_UTC:
			return EpgTimeToUtc(&EpgTime,pDisplayTime);
		}

		return false;
	}


	bool EpgTimeToDisplayTime(SYSTEMTIME *pTime)
	{
		if (pTime==NULL)
			return false;

		SYSTEMTIME st;

		if (!EpgTimeToDisplayTime(*pTime,&st))
			return false;

		*pTime=st;

		return true;
	}


	bool DisplayTimeToEpgTime(const SYSTEMTIME &DisplayTime,SYSTEMTIME *pEpgTime)
	{
		if (pEpgTime==NULL)
			return false;

		switch (GetAppClass().EpgOptions.GetEpgTimeMode()) {
		case CEpgOptions::EPGTIME_RAW:
			*pEpgTime=DisplayTime;
			return true;

		case CEpgOptions::EPGTIME_JST:
			{
				SYSTEMTIME st;
				TIME_ZONE_INFORMATION tzi;

				return GetJSTTimeZoneInformation(&tzi)
					&& ::TzSpecificLocalTimeToSystemTime(&tzi,&DisplayTime,&st)
					&& UtcToEpgTime(&st,pEpgTime);
			}

		case CEpgOptions::EPGTIME_LOCAL:
			{
				SYSTEMTIME st;

				return ::TzSpecificLocalTimeToSystemTime(NULL,&DisplayTime,&st)
					&& UtcToEpgTime(&st,pEpgTime);
			}

		case CEpgOptions::EPGTIME_UTC:
			return UtcToEpgTime(&DisplayTime,pEpgTime);
		}

		return false;
	}


	bool DisplayTimeToEpgTime(SYSTEMTIME *pTime)
	{
		if (pTime==NULL)
			return false;

		SYSTEMTIME st;

		if (!DisplayTimeToEpgTime(*pTime,&st))
			return false;

		*pTime=st;

		return true;
	}


	bool GetLanguageText(DWORD LanguageCode,LPTSTR pszText,int MaxText,LanguageTextType Type)
	{
		static const struct {
			DWORD LanguageCode;
			LPCTSTR pszLongText;
			LPCTSTR pszSimpleText;
			LPCTSTR pszShortText;
		} LanguageList[] = {
			{LANGUAGE_CODE_JPN,	TEXT("日本語"),		TEXT("日本語"),	TEXT("日")},
			{LANGUAGE_CODE_ENG,	TEXT("英語"),		TEXT("英語"),	TEXT("英")},
			{LANGUAGE_CODE_DEU,	TEXT("ドイツ語"),	TEXT("独語"),	TEXT("独")},
			{LANGUAGE_CODE_FRA,	TEXT("フランス語"),	TEXT("仏語"),	TEXT("仏")},
			{LANGUAGE_CODE_ITA,	TEXT("イタリア語"),	TEXT("伊語"),	TEXT("伊")},
			{LANGUAGE_CODE_RUS,	TEXT("ロシア語"),	TEXT("露語"),	TEXT("露")},
			{LANGUAGE_CODE_ZHO,	TEXT("中国語"),		TEXT("中国語"),	TEXT("中")},
			{LANGUAGE_CODE_KOR,	TEXT("韓国語"),		TEXT("韓国語"),	TEXT("韓")},
			{LANGUAGE_CODE_SPA,	TEXT("スペイン語"),	TEXT("西語"),	TEXT("西")},
			{LANGUAGE_CODE_ETC,	TEXT("外国語"),		TEXT("外国語"),	TEXT("外")},
		};

		if (pszText==NULL || MaxText<1)
			return false;

		for (int i=0;i<lengthof(LanguageList);i++) {
			if (LanguageList[i].LanguageCode==LanguageCode) {
				LPCTSTR pszLang;

				switch (Type) {
				default:
				case LANGUAGE_TEXT_LONG:	pszLang=LanguageList[i].pszLongText;	break;
				case LANGUAGE_TEXT_SIMPLE:	pszLang=LanguageList[i].pszSimpleText;	break;
				case LANGUAGE_TEXT_SHORT:	pszLang=LanguageList[i].pszShortText;	break;
				}

				::lstrcpyn(pszText,pszLang,MaxText);

				return true;
			}
		}

		TCHAR szLang[4];
		szLang[0]=static_cast<TCHAR>((LanguageCode>>16)&0xFF);
		szLang[1]=static_cast<TCHAR>((LanguageCode>>8)&0xFF);
		szLang[2]=static_cast<TCHAR>(LanguageCode&0xFF);
		szLang[3]=_T('\0');
		::CharUpperBuff(szLang,3);
		::lstrcpyn(pszText,szLang,MaxText);

		return true;
	}


	bool GetEventGenre(const CEventInfoData &EventInfo,
					   int *pLevel1,int *pLevel2)
	{
		return GetEventGenre(EventInfo.m_ContentNibble,pLevel1,pLevel2);
	}


	bool GetEventGenre(const CEventInfoData::ContentNibble &ContentNibble,
					   int *pLevel1,int *pLevel2)
	{
		for (int i=0;i<ContentNibble.NibbleCount;i++) {
			if (ContentNibble.NibbleList[i].ContentNibbleLevel1!=0xE) {
				if (pLevel1!=nullptr)
					*pLevel1=ContentNibble.NibbleList[i].ContentNibbleLevel1;
				if (pLevel2!=nullptr)
					*pLevel2=ContentNibble.NibbleList[i].ContentNibbleLevel2;
				return true;
			}
		}

		if (pLevel1!=nullptr)
			*pLevel1=-1;
		if (pLevel2!=nullptr)
			*pLevel2=-1;

		return false;
	}


	LPCTSTR GetEventDisplayText(const CEventInfo &EventInfo)
	{
		LPCTSTR p;

		if (!EventInfo.m_EventText.empty()) {
			p=EventInfo.m_EventText.c_str();
			while (*p!='\0') {
				if (*p<=0x20) {
					p++;
					continue;
				}
				return p;
			}
		}

		if (!EventInfo.m_EventExtendedText.empty()) {
			p=EventInfo.m_EventExtendedText.c_str();
			TCHAR szContent[]=TEXT("番組内容");
			if (::StrCmpN(p,szContent,lengthof(szContent)-1)==0)
				p+=lengthof(szContent)-1;
			while (*p!='\0') {
				if (*p<=0x20) {
					p++;
					continue;
				}
				return p;
			}
		}

		return NULL;
	}

}




LPCTSTR CEpgGenre::GetText(int Level1,int Level2) const
{
	static const struct {
		LPCTSTR pszText;
		LPCTSTR pszSubText[16];
	} GenreList[] = {
		{TEXT("ニュース／報道"),
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
				NULL,
				NULL,
				NULL,
				NULL,
				TEXT("その他")
			}
		},
		{TEXT("スポーツ"),
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
				NULL,
				NULL,
				NULL,
				NULL,
				TEXT("その他")
			}
		},
		{TEXT("情報／ワイドショー"),
			{
				TEXT("芸能・ワイドショー"),
				TEXT("ファッション"),
				TEXT("暮らし・住まい"),
				TEXT("健康・医療"),
				TEXT("ショッピング・通販"),
				TEXT("グルメ・料理"),
				TEXT("イベント"),
				TEXT("番組紹介・お知らせ"),
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				TEXT("その他")
			}
		},
		{TEXT("ドラマ"),
			{
				TEXT("国内ドラマ"),
				TEXT("海外ドラマ"),
				TEXT("時代劇"),
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				TEXT("その他")
			}
		},
		{TEXT("音楽"),
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
				NULL,
				NULL,
				NULL,
				NULL,
				TEXT("その他")
			}
		},
		{TEXT("バラエティ"),
			{
				TEXT("クイズ"),
				TEXT("ゲーム"),
				TEXT("トークバラエティ"),
				TEXT("お笑い・コメディ"),
				TEXT("音楽バラエティ"),
				TEXT("旅バラエティ"),
				TEXT("料理バラエティ"),
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				TEXT("その他")
			}
		},
		{TEXT("映画"),
			{
				TEXT("洋画"),
				TEXT("邦画"),
				TEXT("アニメ"),
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				TEXT("その他")
			}
		},
		{TEXT("アニメ／特撮"),
			{
				TEXT("国内アニメ"),
				TEXT("海外アニメ"),
				TEXT("特撮"),
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				TEXT("その他")
			}
		},
		{TEXT("ドキュメンタリー／教養"),
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
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				TEXT("その他")
			}
		},
		{TEXT("劇場／公演"),
			{
				TEXT("現代劇・新劇"),
				TEXT("ミュージカル"),
				TEXT("ダンス・バレエ"),
				TEXT("落語・演芸"),
				TEXT("歌舞伎・古典"),
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				TEXT("その他")
			}
		},
		{TEXT("趣味／教育"),
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
				NULL,
				NULL,
				TEXT("その他")
			}
		},
		{TEXT("福祉"),
			{
				TEXT("高齢者"),
				TEXT("障害者"),
				TEXT("社会福祉"),
				TEXT("ボランティア"),
				TEXT("手話"),
				TEXT("文字(字幕)"),
				TEXT("音声解説"),
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				TEXT("その他")
			}
		},
	};

	if (Level2<0) {
		if (Level1>=0 && Level1<lengthof(GenreList))
			return GenreList[Level1].pszText;
		if (Level1==GENRE_OTHER)
			return TEXT("その他");
		return NULL;
	}
	if (Level1>=0 && Level1<lengthof(GenreList)
			&& Level2>=0 && Level2<16)
		return GenreList[Level1].pszSubText[Level2];
	return NULL;
}




CEpgIcons::CEpgIcons()
	: m_hdc(NULL)
	, m_hbmOld(NULL)
{
}


CEpgIcons::~CEpgIcons()
{
	EndDraw();
}


bool CEpgIcons::Load()
{
	return CBitmap::Load(GetAppClass().GetInstance(),IDB_PROGRAMGUIDEICONS,LR_DEFAULTCOLOR);
}


bool CEpgIcons::BeginDraw(HDC hdc,int IconWidth,int IconHeight)
{
	if (!IsCreated())
		return false;

	EndDraw();

	m_hdc=::CreateCompatibleDC(hdc);
	if (m_hdc==NULL)
		return false;
	m_hbmOld=DrawUtil::SelectObject(m_hdc,*this);

	if (IconWidth>0 && IconHeight>0
			&& (IconWidth!=ICON_WIDTH || IconHeight!=ICON_HEIGHT)) {
		m_StretchBuffer.Create((ICON_LAST+1)*IconWidth,IconHeight,hdc);
		m_StretchedIcons=0;
	}

	m_IconWidth=IconWidth;
	m_IconHeight=IconHeight;

	return true;
}


void CEpgIcons::EndDraw()
{
	if (m_hdc!=NULL) {
		::SelectObject(m_hdc,m_hbmOld);
		::DeleteDC(m_hdc);
		m_hdc=NULL;
		m_hbmOld=NULL;
	}
	m_StretchBuffer.Destroy();
}


bool CEpgIcons::DrawIcon(
	HDC hdcDst,int DstX,int DstY,int Width,int Height,
	int Icon,BYTE Opacity,const RECT *pClipping)
{
	if (m_hdc==NULL
			|| hdcDst==NULL
			|| Width<=0 || Height<=0
			|| Icon<0 || Icon>ICON_LAST)
		return false;

	HDC hdcSrc;
	int IconWidth,IconHeight;

	if ((Width!=ICON_WIDTH || Height!=ICON_HEIGHT)
			&& Opacity<255
			&& m_StretchBuffer.IsCreated()) {
		hdcSrc=m_StretchBuffer.GetDC();
		IconWidth=m_IconWidth;
		IconHeight=m_IconHeight;
		if ((m_StretchedIcons & IconFlag(Icon))==0) {
			int OldStretchMode=::SetStretchBltMode(hdcSrc,STRETCH_HALFTONE);
			::StretchBlt(hdcSrc,Icon*IconWidth,0,IconWidth,IconHeight,
						 m_hdc,Icon*ICON_WIDTH,0,ICON_WIDTH,ICON_HEIGHT,
						 SRCCOPY);
			::SetStretchBltMode(hdcSrc,OldStretchMode);
			m_StretchedIcons|=IconFlag(Icon);
		}
	} else {
		hdcSrc=m_hdc;
		IconWidth=ICON_WIDTH;
		IconHeight=ICON_HEIGHT;
	}

	RECT rcDraw,rcDst;

	if (pClipping!=NULL) {
		::SetRect(&rcDst,DstX,DstY,DstX+Width,DstY+Height);
		if (!::IntersectRect(&rcDraw,&rcDst,pClipping))
			return true;
	} else {
		::SetRect(&rcDraw,DstX,DstY,DstX+Width,DstY+Height);
	}

	int DstWidth=rcDraw.right-rcDraw.left;
	int DstHeight=rcDraw.bottom-rcDraw.top;
	int SrcX=(IconWidth*(rcDraw.left-DstX)+Width/2)/Width;
	int SrcY=(IconHeight*(rcDraw.top-DstY)+Height/2)/Height;
	int SrcWidth=(IconWidth*(rcDraw.right-DstX)+Width/2)/Width-SrcX;
	if (SrcWidth<1)
		SrcWidth=1;
	int SrcHeight=(IconHeight*(rcDraw.bottom-DstY)+Height/2)/Height-SrcY;
	if (SrcHeight<1)
		SrcHeight=1;

	if (Opacity==255) {
		if (DstWidth==SrcWidth && DstHeight==SrcHeight) {
			::BitBlt(hdcDst,rcDraw.left,rcDraw.top,DstWidth,DstHeight,
					 hdcSrc,Icon*IconWidth+SrcX,SrcY,SRCCOPY);
		} else {
			int OldStretchMode=::SetStretchBltMode(hdcDst,STRETCH_HALFTONE);
			::StretchBlt(hdcDst,rcDraw.left,rcDraw.top,DstWidth,DstHeight,
						 hdcSrc,Icon*IconWidth+SrcX,SrcY,SrcWidth,SrcHeight,
						 SRCCOPY);
			::SetStretchBltMode(hdcDst,OldStretchMode);
		}
	} else {
		BLENDFUNCTION bf={AC_SRC_OVER,0,Opacity,0};
		::GdiAlphaBlend(hdcDst,rcDraw.left,rcDraw.top,DstWidth,DstHeight,
						hdcSrc,Icon*IconWidth+SrcX,SrcY,SrcWidth,SrcHeight,
						bf);
	}

	return true;
}


bool CEpgIcons::DrawIcons(
	unsigned int IconFlags,
	HDC hdcDst,int DstX,int DstY,int Width,int Height,
	int IntervalX,int IntervalY,
	BYTE Opacity,const RECT *pClipping)
{
	if (IconFlags==0)
		return false;

	int x=DstX,y=DstY;
	int Icon=0;

	for (unsigned int Flag=IconFlags;Flag!=0;Flag>>=1) {
		if (pClipping!=nullptr
				&& (x>=pClipping->right || y>=pClipping->bottom))
			break;
		if ((Flag&1)!=0) {
			DrawIcon(hdcDst,x,y,Width,Height,Icon,Opacity,pClipping);
			x+=IntervalX;
			y+=IntervalY;
		}
		Icon++;
	}

	return true;
}


unsigned int CEpgIcons::GetEventIcons(const CEventInfoData *pEventInfo)
{
	unsigned int ShowIcons=0;

	if (!pEventInfo->m_VideoList.empty()) {
		EpgUtil::VideoType Video=EpgUtil::GetVideoType(pEventInfo->m_VideoList[0].ComponentType);
		if (Video==EpgUtil::VIDEO_TYPE_HD)
			ShowIcons|=IconFlag(ICON_HD);
		else if (Video==EpgUtil::VIDEO_TYPE_SD)
			ShowIcons|=IconFlag(ICON_SD);
	}

	if (!pEventInfo->m_AudioList.empty()) {
		const CEventInfoData::AudioInfo *pAudioInfo=pEventInfo->GetMainAudioInfo();

		if (pAudioInfo->ComponentType==0x02) {
			if (pAudioInfo->bESMultiLingualFlag
					&& pAudioInfo->LanguageCode!=pAudioInfo->LanguageCode2)
				ShowIcons|=IconFlag(ICON_MULTILINGUAL);
			else
				ShowIcons|=IconFlag(ICON_SUB);
		} else {
			if (pAudioInfo->ComponentType==0x09)
				ShowIcons|=IconFlag(ICON_5_1CH);
			if (pEventInfo->m_AudioList.size()>=2
					&& pEventInfo->m_AudioList[0].LanguageCode!=0
					&& pEventInfo->m_AudioList[1].LanguageCode!=0) {
				if (pEventInfo->m_AudioList[0].LanguageCode!=
						pEventInfo->m_AudioList[1].LanguageCode)
					ShowIcons|=IconFlag(ICON_MULTILINGUAL);
				else
					ShowIcons|=IconFlag(ICON_SUB);
			}
		}
	}

	if (GetAppClass().NetworkDefinition.IsSatelliteNetworkID(pEventInfo->m_NetworkID)) {
		if (pEventInfo->m_bFreeCaMode)
			ShowIcons|=IconFlag(ICON_PAY);
		else
			ShowIcons|=IconFlag(ICON_FREE);
	}

	return ShowIcons;
}




CEpgTheme::CEpgTheme()
{
	m_ColorList[COLOR_EVENTNAME].Set(0,0,0);
	m_ColorList[COLOR_EVENTTEXT].Set(0,0,0);
	for (int i=COLOR_CONTENT_FIRST;i<=COLOR_CONTENT_LAST;i++)
		m_ColorList[i].Set(240,240,240);
	m_ColorList[COLOR_CONTENT_NEWS       ].Set(255,255,224);
	m_ColorList[COLOR_CONTENT_SPORTS     ].Set(255,255,224);
//	m_ColorList[COLOR_CONTENT_INFORMATION].Set(255,255,224);
	m_ColorList[COLOR_CONTENT_DRAMA      ].Set(255,224,224);
	m_ColorList[COLOR_CONTENT_MUSIC      ].Set(224,255,224);
	m_ColorList[COLOR_CONTENT_VARIETY    ].Set(224,224,255);
	m_ColorList[COLOR_CONTENT_MOVIE      ].Set(224,255,255);
	m_ColorList[COLOR_CONTENT_ANIME      ].Set(255,224,255);
	m_ColorList[COLOR_CONTENT_DOCUMENTARY].Set(255,255,224);
	m_ColorList[COLOR_CONTENT_THEATER    ].Set(224,255,255);
}


void CEpgTheme::SetTheme(const TVTest::Theme::CThemeManager *pThemeManager)
{
	m_ColorList[COLOR_EVENTNAME]=
		pThemeManager->GetColor(CColorScheme::COLOR_PROGRAMGUIDE_EVENTTITLE);
	m_ColorList[COLOR_EVENTTEXT]=
		pThemeManager->GetColor(CColorScheme::COLOR_PROGRAMGUIDE_TEXT);

	for (int i=COLOR_CONTENT_FIRST,j=0;i<=COLOR_CONTENT_LAST;i++,j++) {
		m_ColorList[i]=
			pThemeManager->GetColor(CColorScheme::COLOR_PROGRAMGUIDE_CONTENT_FIRST+j);
	}
}


bool CEpgTheme::SetColor(int Type,const TVTest::Theme::ThemeColor &Color)
{
	if (Type<0 || Type>=NUM_COLORS)
		return false;
	m_ColorList[Type]=Color;
	return true;
}


TVTest::Theme::ThemeColor CEpgTheme::GetColor(int Type) const
{
	if (Type<0 || Type>=NUM_COLORS)
		return TVTest::Theme::ThemeColor();
	return m_ColorList[Type];
}


TVTest::Theme::ThemeColor CEpgTheme::GetGenreColor(int Genre) const
{
	return m_ColorList[Genre>=0 && Genre<=CEventInfoData::CONTENT_LAST?
					   COLOR_CONTENT_FIRST+Genre:
					   COLOR_CONTENT_OTHER];
}


TVTest::Theme::ThemeColor CEpgTheme::GetGenreColor(const CEventInfoData &EventInfo) const
{
	int Genre;

	if (!EpgUtil::GetEventGenre(EventInfo,&Genre))
		return m_ColorList[COLOR_CONTENT_OTHER];

	return GetGenreColor(Genre);
}


TVTest::Theme::BackgroundStyle CEpgTheme::GetContentBackgroundStyle(
	int Genre,unsigned int Flags) const
{
	return GetContentBackgroundStyle(GetGenreColor(Genre),Flags);
}


TVTest::Theme::BackgroundStyle CEpgTheme::GetContentBackgroundStyle(
	const CEventInfoData &EventInfo,unsigned int Flags) const
{
	return GetContentBackgroundStyle(GetGenreColor(EventInfo),Flags);
}


bool CEpgTheme::DrawContentBackground(
	HDC hdc,TVTest::Theme::CThemeDraw &ThemeDraw,const RECT &Rect,
	const CEventInfoData &EventInfo,unsigned int Flags) const
{
	if (hdc==nullptr)
		return false;

	unsigned int StyleFlags=0;
	if ((Flags & DRAW_CONTENT_BACKGROUND_CURRENT)!=0)
		StyleFlags|=CONTENT_STYLE_CURRENT;
	if ((Flags & DRAW_CONTENT_BACKGROUND_NOBORDER)!=0)
		StyleFlags|=CONTENT_STYLE_NOBORDER;
	ThemeDraw.Draw(GetContentBackgroundStyle(EventInfo,StyleFlags),Rect);

	if ((Flags & DRAW_CONTENT_BACKGROUND_SEPARATOR)!=0) {
		RECT rc=Rect;
		rc.bottom=rc.top+ThemeDraw.GetStyleScaling()->ToPixels(1,TVTest::Style::UNIT_LOGICAL_PIXEL);
		DrawUtil::Fill(hdc,&rc,MixColor(GetGenreColor(EventInfo),RGB(0,0,0),224));
	}

	return true;
}


TVTest::Theme::BackgroundStyle CEpgTheme::GetContentBackgroundStyle(
	const TVTest::Theme::ThemeColor &Color,unsigned int Flags) const
{
	TVTest::Theme::BackgroundStyle BackStyle;

	if ((Flags & CONTENT_STYLE_CURRENT)==0) {
		BackStyle.Fill.Type=TVTest::Theme::FILL_SOLID;
		BackStyle.Fill.Solid=TVTest::Theme::SolidStyle(Color);
	} else {
		double h,s,v,s1,v1;
		TVTest::Theme::ThemeColor Color1,Color2;
		RGBToHSV(Color.Red,Color.Green,Color.Blue,&h,&s,&v);
		s1=s;
		v1=v;
		if (s1<0.1 || v1<=0.95) {
			v1+=0.05;
			if (v1>1.0)
				v1=1.0;
		} else {
			s1-=s*0.5;
			if (s1<0.0)
				s1=0.0;
		}
		Color1=HSVToRGB(h,s1,v1);
		s1=s;
		v1=v;
		if (s1>=0.1 && s1<=0.9) {
			s1+=s*0.5;
			if (s1>1.0)
				s1=1.0;
		} else {
			v1-=0.05;
			if (v1<0.0)
				v1=0.0;
		}
		Color2=HSVToRGB(h,s1,v1);
		BackStyle.Fill.Type=TVTest::Theme::FILL_GRADIENT;
		BackStyle.Fill.Gradient=TVTest::Theme::GradientStyle(
			TVTest::Theme::GRADIENT_NORMAL,
			TVTest::Theme::DIRECTION_VERT,
			Color1,Color2);
	}

#if 0
	if ((Flags & CONTENT_STYLE_NOBORDER)!=0)
		BackStyle.Border.Type=TVTest::Theme::BORDER_NONE;
#endif

	return BackStyle;
}
