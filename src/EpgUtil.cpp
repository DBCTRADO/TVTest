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
			{0x02,TEXT("480i[16:9]")},	// �p���x�N�g������
			{0x03,TEXT("480i[16:9]")},	// �p���x�N�g���Ȃ�
			{0x04,TEXT("480i[>16:9]")},
			{0x91,TEXT("2160p[4:3]")},
			{0x92,TEXT("2160p[16:9]")},	// �p���x�N�g������
			{0x93,TEXT("2160p[16:9]")},	// �p���x�N�g���Ȃ�
			{0x94,TEXT("2160p[>16:9]")},
			{0xA1,TEXT("480p[4:3]")},
			{0xA2,TEXT("480p[16:9]")},	// �p���x�N�g������
			{0xA3,TEXT("480p[16:9]")},	// �p���x�N�g���Ȃ�
			{0xA4,TEXT("480p[>16:9]")},
			{0xB1,TEXT("1080i[4:3]")},
			{0xB2,TEXT("1080i[16:9]")},	// �p���x�N�g������
			{0xB3,TEXT("1080i[16:9]")},	// �p���x�N�g���Ȃ�
			{0xB4,TEXT("1080i[>16:9]")},
			{0xC1,TEXT("720p[4:3]")},
			{0xC2,TEXT("720p[16:9]")},	// �p���x�N�g������
			{0xC3,TEXT("720p[16:9]")},	// �p���x�N�g���Ȃ�
			{0xC4,TEXT("720p[>16:9]")},
			{0xD1,TEXT("240p[4:3]")},
			{0xD2,TEXT("240p[16:9]")},	// �p���x�N�g������
			{0xD3,TEXT("240p[16:9]")},	// �p���x�N�g���Ȃ�
			{0xD4,TEXT("240p[>16:9]")},
			{0xE1,TEXT("1080p[4:3]")},
			{0xE2,TEXT("1080p[16:9]")},	// �p���x�N�g������
			{0xE3,TEXT("1080p[16:9]")},	// �p���x�N�g���Ȃ�
			{0xE4,TEXT("1080p[>16:9]")},
			{0xF1,TEXT("180p[4:3]")},
			{0xF2,TEXT("180p[16:9]")},	// �p���x�N�g������
			{0xF3,TEXT("180p[16:9]")},	// �p���x�N�g���Ȃ�
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
			{0x40,TEXT("���o��Q�җp�������")},
			{0x41,TEXT("���o��Q�җp����")},
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
					::lstrcpy(szEndTime,TEXT("(�I������)"));
			}
		}

		return StdUtil::snprintf(pszTime,MaxLength,TEXT("%s%s%s%s"),
								 szDate,
								 szStartTime,
								 (Flags & EVENT_TIME_START_ONLY)==0?TEXT("�`"):TEXT(""),
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
			{LANGUAGE_CODE_JPN,	TEXT("���{��"),		TEXT("���{��"),	TEXT("��")},
			{LANGUAGE_CODE_ENG,	TEXT("�p��"),		TEXT("�p��"),	TEXT("�p")},
			{LANGUAGE_CODE_DEU,	TEXT("�h�C�c��"),	TEXT("�ƌ�"),	TEXT("��")},
			{LANGUAGE_CODE_FRA,	TEXT("�t�����X��"),	TEXT("����"),	TEXT("��")},
			{LANGUAGE_CODE_ITA,	TEXT("�C�^���A��"),	TEXT("�Ɍ�"),	TEXT("��")},
			{LANGUAGE_CODE_RUS,	TEXT("���V�A��"),	TEXT("�I��"),	TEXT("�I")},
			{LANGUAGE_CODE_ZHO,	TEXT("������"),		TEXT("������"),	TEXT("��")},
			{LANGUAGE_CODE_KOR,	TEXT("�؍���"),		TEXT("�؍���"),	TEXT("��")},
			{LANGUAGE_CODE_SPA,	TEXT("�X�y�C����"),	TEXT("����"),	TEXT("��")},
			{LANGUAGE_CODE_ETC,	TEXT("�O����"),		TEXT("�O����"),	TEXT("�O")},
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
			TCHAR szContent[]=TEXT("�ԑg���e");
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
		{TEXT("�j���[�X�^��"),
			{
				TEXT("�莞�E����"),
				TEXT("�V�C"),
				TEXT("���W�E�h�L�������g"),
				TEXT("�����E����"),
				TEXT("�o�ρE�s��"),
				TEXT("�C�O�E����"),
				TEXT("���"),
				TEXT("���_�E��k"),
				TEXT("�񓹓���"),
				TEXT("���[�J���E�n��"),
				TEXT("���"),
				NULL,
				NULL,
				NULL,
				NULL,
				TEXT("���̑�")
			}
		},
		{TEXT("�X�|�[�c"),
			{
				TEXT("�X�|�[�c�j���[�X"),
				TEXT("�싅"),
				TEXT("�T�b�J�["),
				TEXT("�S���t"),
				TEXT("���̑��̋��Z"),
				TEXT("���o�E�i���Z"),
				TEXT("�I�����s�b�N�E���ۑ��"),
				TEXT("�}���\���E����E���j"),
				TEXT("���[�^�[�X�|�[�c"),
				TEXT("�}�����E�E�B���^�[�X�|�[�c"),
				TEXT("���n�E���c���Z"),
				NULL,
				NULL,
				NULL,
				NULL,
				TEXT("���̑�")
			}
		},
		{TEXT("���^���C�h�V���["),
			{
				TEXT("�|�\�E���C�h�V���["),
				TEXT("�t�@�b�V����"),
				TEXT("��炵�E�Z�܂�"),
				TEXT("���N�E���"),
				TEXT("�V���b�s���O�E�ʔ�"),
				TEXT("�O�����E����"),
				TEXT("�C�x���g"),
				TEXT("�ԑg�Љ�E���m�点"),
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				TEXT("���̑�")
			}
		},
		{TEXT("�h���}"),
			{
				TEXT("�����h���}"),
				TEXT("�C�O�h���}"),
				TEXT("���㌀"),
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
				TEXT("���̑�")
			}
		},
		{TEXT("���y"),
			{
				TEXT("�������b�N�E�|�b�v�X"),
				TEXT("�C�O���b�N�E�|�b�v�X"),
				TEXT("�N���V�b�N�E�I�y��"),
				TEXT("�W���Y�E�t���[�W����"),
				TEXT("�̗w�ȁE����"),
				TEXT("���C�u�E�R���T�[�g"),
				TEXT("�����L���O�E���N�G�X�g"),
				TEXT("�J���I�P�E�̂ǎ���"),
				TEXT("���w�E�M�y"),
				TEXT("���w�E�L�b�Y"),
				TEXT("�������y�E���[���h�~���[�W�b�N"),
				NULL,
				NULL,
				NULL,
				NULL,
				TEXT("���̑�")
			}
		},
		{TEXT("�o���G�e�B"),
			{
				TEXT("�N�C�Y"),
				TEXT("�Q�[��"),
				TEXT("�g�[�N�o���G�e�B"),
				TEXT("���΂��E�R���f�B"),
				TEXT("���y�o���G�e�B"),
				TEXT("���o���G�e�B"),
				TEXT("�����o���G�e�B"),
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				TEXT("���̑�")
			}
		},
		{TEXT("�f��"),
			{
				TEXT("�m��"),
				TEXT("�M��"),
				TEXT("�A�j��"),
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
				TEXT("���̑�")
			}
		},
		{TEXT("�A�j���^���B"),
			{
				TEXT("�����A�j��"),
				TEXT("�C�O�A�j��"),
				TEXT("���B"),
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
				TEXT("���̑�")
			}
		},
		{TEXT("�h�L�������^���[�^���{"),
			{
				TEXT("�Љ�E����"),
				TEXT("���j�E�I�s"),
				TEXT("���R�E�����E��"),
				TEXT("�F���E�Ȋw�E��w"),
				TEXT("�J���`���[�E�`������"),
				TEXT("���w�E���|"),
				TEXT("�X�|�[�c"),
				TEXT("�h�L�������^���[�S��"),
				TEXT("�C���^�r���[�E���_"),
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				TEXT("���̑�")
			}
		},
		{TEXT("����^����"),
			{
				TEXT("���㌀�E�V��"),
				TEXT("�~���[�W�J��"),
				TEXT("�_���X�E�o���G"),
				TEXT("����E���|"),
				TEXT("�̕���E�ÓT"),
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
				TEXT("���̑�")
			}
		},
		{TEXT("��^����"),
			{
				TEXT("���E�ނ�E�A�E�g�h�A"),
				TEXT("���|�E�y�b�g�E��|"),
				TEXT("���y�E���p�E�H�|"),
				TEXT("�͌�E����"),
				TEXT("�����E�p�`���R"),
				TEXT("�ԁE�I�[�g�o�C"),
				TEXT("�R���s���[�^�ETV�Q�[��"),
				TEXT("��b�E��w"),
				TEXT("�c���E���w��"),
				TEXT("���w���E���Z��"),
				TEXT("��w���E��"),
				TEXT("���U�w�K�E���i"),
				TEXT("������"),
				NULL,
				NULL,
				TEXT("���̑�")
			}
		},
		{TEXT("����"),
			{
				TEXT("�����"),
				TEXT("��Q��"),
				TEXT("�Љ��"),
				TEXT("�{�����e�B�A"),
				TEXT("��b"),
				TEXT("����(����)"),
				TEXT("�������"),
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				TEXT("���̑�")
			}
		},
	};

	if (Level2<0) {
		if (Level1>=0 && Level1<lengthof(GenreList))
			return GenreList[Level1].pszText;
		if (Level1==GENRE_OTHER)
			return TEXT("���̑�");
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


bool CEpgIcons::Draw(HDC hdcDst,int DstX,int DstY,int Width,int Height,
					 HDC hdcSrc,int Icon,BYTE Opacity,const RECT *pClipping)
{
	if (hdcDst==NULL || hdcSrc==NULL
			|| Width<=0 || Height<=0
			|| Icon<0 || Icon>ICON_LAST)
		return false;

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
	int SrcX=(ICON_WIDTH*(rcDraw.left-DstX)+Width/2)/Width;
	int SrcY=(ICON_HEIGHT*(rcDraw.top-DstY)+Height/2)/Height;
	int SrcWidth=(ICON_WIDTH*(rcDraw.right-DstX)+Width/2)/Width-SrcX;
	if (SrcWidth<1)
		SrcWidth=1;
	int SrcHeight=(ICON_HEIGHT*(rcDraw.bottom-DstY)+Height/2)/Height-SrcY;
	if (SrcHeight<1)
		SrcHeight=1;

	if (Opacity==255) {
		if (DstWidth==SrcWidth && DstHeight==SrcHeight) {
			::BitBlt(hdcDst,rcDraw.left,rcDraw.top,DstWidth,DstHeight,
					 hdcSrc,Icon*ICON_WIDTH+SrcX,SrcY,SRCCOPY);
		} else {
			int OldStretchMode=::SetStretchBltMode(hdcDst,STRETCH_HALFTONE);
			::StretchBlt(hdcDst,rcDraw.left,rcDraw.top,DstWidth,DstHeight,
						 hdcSrc,Icon*ICON_WIDTH+SrcX,SrcY,SrcWidth,SrcHeight,
						 SRCCOPY);
			::SetStretchBltMode(hdcDst,OldStretchMode);
		}
	} else {
		BLENDFUNCTION bf={AC_SRC_OVER,0,Opacity,0};
		::GdiAlphaBlend(hdcDst,rcDraw.left,rcDraw.top,DstWidth,DstHeight,
						hdcSrc,Icon*ICON_WIDTH+SrcX,SrcY,SrcWidth,SrcHeight,
						bf);
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


bool CEpgTheme::DrawContentBackground(HDC hdc,const RECT &Rect,
									  const CEventInfoData &EventInfo,unsigned int Flags) const
{
	if (hdc==nullptr)
		return false;

	unsigned int StyleFlags=0;
	if ((Flags & DRAW_CONTENT_BACKGROUND_CURRENT)!=0)
		StyleFlags|=CONTENT_STYLE_CURRENT;
	if ((Flags & DRAW_CONTENT_BACKGROUND_NOBORDER)!=0)
		StyleFlags|=CONTENT_STYLE_NOBORDER;
	TVTest::Theme::Draw(hdc,Rect,GetContentBackgroundStyle(EventInfo,StyleFlags));

	if ((Flags & DRAW_CONTENT_BACKGROUND_SEPARATOR)!=0) {
		HPEN hpen=::CreatePen(PS_SOLID,1,MixColor(GetGenreColor(EventInfo),RGB(0,0,0),224));
		HPEN hpenOld=static_cast<HPEN>(::SelectObject(hdc,hpen));
		::MoveToEx(hdc,Rect.left,Rect.top,NULL);
		::LineTo(hdc,Rect.right,Rect.top);
		::SelectObject(hdc,hpenOld);
		::DeleteObject(hpen);
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
