#include "stdafx.h"
#include "TVTest.h"
#include "EpgUtil.h"
#include "AppMain.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




namespace EpgUtil
{

	VideoType GetVideoType(BYTE ComponentType)
	{
		if (ComponentType>=0xB1
				&& ComponentType<=0xB4)
			return VIDEO_TYPE_HD;
		if (ComponentType>=0x01
				&& ComponentType<=0x04)
			return VIDEO_TYPE_SD;
		return VIDEO_TYPE_UNKNOWN;
	}


	LPCTSTR GetVideoComponentTypeText(BYTE ComponentType)
	{
		static const struct {
			BYTE ComponentType;
			LPCTSTR pszText;
		} VideoComponentTypeList[] = {
			{0x01,TEXT("480i[4:3]")},
			{0x03,TEXT("480i[16:9]")},
			{0x04,TEXT("480i[>16:9]")},
			{0xA1,TEXT("480p[4:3]")},
			{0xA3,TEXT("480p[16:9]")},
			{0xA4,TEXT("480p[>16:9]")},
			{0xB1,TEXT("1080i[4:3]")},
			{0xB3,TEXT("1080i[16:9]")},
			{0xB4,TEXT("1080i[>16:9]")},
			{0xC1,TEXT("720p[4:3]")},
			{0xC3,TEXT("720p[16:9]")},
			{0xC4,TEXT("720p[>16:9]")},
			{0xD1,TEXT("240p[4:3]")},
			{0xD3,TEXT("240p[16:9]")},
			{0xD4,TEXT("240p[>16:9]")},
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
			{0x01,TEXT("Mono")},
			{0x02,TEXT("Dual mono")},
			{0x03,TEXT("Stereo")},
			{0x07,TEXT("3+1")},
			{0x08,TEXT("3+2")},
			{0x09,TEXT("5.1ch")},
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

		if (pEventInfo==NULL || !pEventInfo->m_fValidStartTime) {
			pszTime[0]=_T('\0');
			return 0;
		}

		return FormatEventTime(pEventInfo->m_stStartTime,pEventInfo->m_DurationSec,
							   pszTime,MaxLength,Flags);
	}


	int FormatEventTime(const SYSTEMTIME &StartTime,DWORD Duration,
						LPTSTR pszTime,int MaxLength,unsigned int Flags)
	{
		if (pszTime==NULL || MaxLength<1)
			return 0;

		TCHAR szDate[32];
		if ((Flags & EVENT_TIME_DATE)!=0) {
			int Length=0;
			if ((Flags & EVENT_TIME_YEAR)!=0) {
				Length=StdUtil::snprintf(szDate,lengthof(szDate),TEXT("%d/"),
										 StartTime.wYear);
			}
			StdUtil::snprintf(szDate+Length,lengthof(szDate)-Length,
							  TEXT("%d/%d(%s) "),
							  StartTime.wMonth,
							  StartTime.wDay,
							  GetDayOfWeekText(StartTime.wDayOfWeek));
		} else {
			szDate[0]=_T('\0');
		}

		LPCTSTR pszTimeFormat=
			(Flags & EVENT_TIME_HOUR_2DIGITS)!=0?TEXT("%02d:%02d"):TEXT("%d:%02d");
		TCHAR szStartTime[32],szEndTime[32];

		StdUtil::snprintf(szStartTime,lengthof(szStartTime),
						  pszTimeFormat,
						  StartTime.wHour,
						  StartTime.wMinute);

		szEndTime[0]=_T('\0');
		if ((Flags & EVENT_TIME_START_ONLY)==0) {
			if (Duration>0) {
				SYSTEMTIME EndTime=StartTime;
				if (OffsetSystemTime(&EndTime,Duration*1000)) {
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


	LPCTSTR GetLanguageText(DWORD LanguageCode,LanguageTextType Type)
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

		int i;
		for (i=0;i<lengthof(LanguageList)-1;i++) {
			if (LanguageList[i].LanguageCode==LanguageCode)
				break;
		}
		switch (Type) {
		case LANGUAGE_TEXT_LONG:	return LanguageList[i].pszLongText;
		case LANGUAGE_TEXT_SIMPLE:	return LanguageList[i].pszSimpleText;
		case LANGUAGE_TEXT_SHORT:	return LanguageList[i].pszShortText;
		}
		return TEXT("");
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




bool CEpgIcons::Load()
{
	return CBitmap::Load(GetAppClass().GetInstance(),IDB_PROGRAMGUIDEICONS,LR_DEFAULTCOLOR);
}


bool CEpgIcons::Draw(HDC hdcDst,int DstX,int DstY,
					 HDC hdcSrc,int Icon,int Width,int Height,BYTE Opacity)
{
	if (hdcDst==NULL || hdcSrc==NULL || Icon<0 || Icon>ICON_LAST
			|| Width<=0 || Height<=0)
		return false;
	if (Opacity==255) {
		::BitBlt(hdcDst,DstX,DstY,Width,Height,
				 hdcSrc,Icon*ICON_WIDTH,0,SRCCOPY);
	} else {
		BLENDFUNCTION bf={AC_SRC_OVER,0,Opacity,0};
		::GdiAlphaBlend(hdcDst,DstX,DstY,Width,Height,
						hdcSrc,Icon*ICON_WIDTH,0,Width,Height,
						bf);
	}
	return true;
}


unsigned int CEpgIcons::GetEventIcons(const CEventInfoData *pEventInfo)
{
	unsigned int ShowIcons=0;

	EpgUtil::VideoType Video=EpgUtil::GetVideoType(pEventInfo->m_VideoInfo.ComponentType);
	if (Video==EpgUtil::VIDEO_TYPE_HD)
		ShowIcons|=IconFlag(ICON_HD);
	else if (Video==EpgUtil::VIDEO_TYPE_SD)
		ShowIcons|=IconFlag(ICON_SD);

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

	if (pEventInfo->m_NetworkID>=4 && pEventInfo->m_NetworkID<=10) {
		if (pEventInfo->m_CaType==CEventInfoData::CA_TYPE_FREE)
			ShowIcons|=IconFlag(ICON_FREE);
		else if (pEventInfo->m_CaType==CEventInfoData::CA_TYPE_CHARGEABLE)
			ShowIcons|=IconFlag(ICON_PAY);
	}

	return ShowIcons;
}
