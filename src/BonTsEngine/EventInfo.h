#pragma once


#include <string>
#include "TsDescriptor.h"


class CEventInfo
{
public:
	typedef std::basic_string<TCHAR> String;

	struct VideoInfo
	{
		enum { MAX_TEXT = 64 };

		BYTE StreamContent;
		BYTE ComponentType;
		BYTE ComponentTag;
		DWORD LanguageCode;
		TCHAR szText[MAX_TEXT];

		VideoInfo()
			: StreamContent(0)
			, ComponentType(0)
			, ComponentTag(0)
			, LanguageCode(0)
		{
			szText[0] = '\0';
		}

		bool operator==(const VideoInfo &Op) const
		{
			return StreamContent == Op.StreamContent
				&& ComponentType == Op.ComponentType
				&& ComponentTag == Op.ComponentTag
				&& LanguageCode == Op.LanguageCode
				&& ::lstrcmp(szText, Op.szText) == 0;
		}

		bool operator!=(const VideoInfo &Op) const
		{
			return !(*this==Op);
		}
	};

	struct AudioInfo
	{
		enum { MAX_TEXT = 64 };

		BYTE StreamContent;
		BYTE ComponentType;
		BYTE ComponentTag;
		BYTE SimulcastGroupTag;
		bool bESMultiLingualFlag;
		bool bMainComponentFlag;
		BYTE QualityIndicator;
		BYTE SamplingRate;
		DWORD LanguageCode;
		DWORD LanguageCode2;
		TCHAR szText[MAX_TEXT];

		AudioInfo() { szText[0] = '\0'; }

		bool operator==(const AudioInfo &Op) const
		{
			return StreamContent == Op.StreamContent
				&& ComponentType == Op.ComponentType
				&& ComponentTag == Op.ComponentTag
				&& SimulcastGroupTag == Op.SimulcastGroupTag
				&& bESMultiLingualFlag == Op.bESMultiLingualFlag
				&& bMainComponentFlag == Op.bMainComponentFlag
				&& QualityIndicator == Op.QualityIndicator
				&& SamplingRate == Op.SamplingRate
				&& LanguageCode == Op.LanguageCode
				&& LanguageCode2 == Op.LanguageCode2
				&& ::lstrcmp(szText, Op.szText) == 0;
		}

		bool operator!=(const AudioInfo &Op) const
		{
			return !(*this == Op);
		}
	};

	struct ContentNibble
	{
		int NibbleCount;
		CContentDesc::Nibble NibbleList[7];

		ContentNibble() : NibbleCount(0) {}

		bool operator==(const ContentNibble &Operand) const
		{
			if (NibbleCount == Operand.NibbleCount) {
				for (int i = 0; i < NibbleCount; i++) {
					if (NibbleList[i] != Operand.NibbleList[i])
						return false;
				}
				return true;
			}
			return false;
		}

		bool operator!=(const ContentNibble &Operand) const
		{
			return !(*this == Operand);
		}
	};

	struct EventGroupInfo
	{
		BYTE GroupType;
		std::vector<CEventGroupDesc::EventInfo> EventList;

		bool operator==(const EventGroupInfo &Operand) const
		{
			return GroupType == Operand.GroupType
				&& EventList == Operand.EventList;
		}

		bool operator!=(const EventGroupInfo &Operand) const
		{
			return !(*this == Operand);
		}
	};

	struct CommonEventInfo
	{
		WORD ServiceID;
		WORD EventID;

		bool operator==(const CommonEventInfo &Op) const
		{
			return ServiceID==Op.ServiceID
				&& EventID==Op.EventID;
		}

		bool operator!=(const CommonEventInfo &Op) const
		{
			return !(*this==Op);
		}
	};

	struct SeriesInfo
	{
		WORD SeriesID;
		BYTE RepeatLabel;
		BYTE ProgramPattern;
		bool bIsExpireDateValid;
		SYSTEMTIME ExpireDate;
		WORD EpisodeNumber;
		WORD LastEpisodeNumber;
		TCHAR szSeriesName[CSeriesDesc::MAX_SERIES_NAME];
	};

	typedef std::vector<VideoInfo> VideoList;
	typedef std::vector<AudioInfo> AudioList;
	typedef std::vector<EventGroupInfo> EventGroupList;

	enum {
		TYPE_BASIC     = 0x0001U,
		TYPE_EXTENDED  = 0x0002U,
		TYPE_PRESENT   = 0x0004U,
		TYPE_FOLLOWING = 0x0008U
	};

	WORD m_NetworkID;
	WORD m_TransportStreamID;
	WORD m_ServiceID;
	WORD m_EventID;
	bool m_bValidStartTime;
	SYSTEMTIME m_StartTime;
	DWORD m_Duration;
	BYTE m_RunningStatus;
	bool m_bFreeCaMode;
	String m_EventName;
	String m_EventText;
	String m_EventExtendedText;
	VideoList m_VideoList;
	AudioList m_AudioList;
	ContentNibble m_ContentNibble;
	EventGroupList m_EventGroupList;
	bool m_bCommonEvent;
	CommonEventInfo m_CommonEventInfo;
	unsigned int m_Type;
	ULONGLONG m_UpdatedTime;

	CEventInfo();
	CEventInfo(CEventInfo &&Src);
	CEventInfo &operator=(CEventInfo &&Op);
#if defined(_MSC_VER) && (_MSC_VER >= 1800)
	CEventInfo(const CEventInfo &) = default;
	CEventInfo &operator=(const CEventInfo &) = default;
#endif
	bool operator==(const CEventInfo &Op) const;
	bool operator!=(const CEventInfo &Op) const { return !(*this == Op); }
	bool IsEqual(const CEventInfo &Op) const;
	bool HasBasic() const { return (m_Type & TYPE_BASIC) != 0; }
	bool HasExtended() const { return (m_Type & TYPE_EXTENDED) != 0; }
	bool IsPresent() const { return (m_Type & TYPE_PRESENT) != 0; }
	bool IsFollowing() const { return (m_Type & TYPE_FOLLOWING) != 0; }
	bool IsPresentFollowing() const { return (m_Type & (TYPE_PRESENT | TYPE_FOLLOWING)) != 0; }
	bool GetStartTime(SYSTEMTIME *pTime) const;
	bool GetEndTime(SYSTEMTIME *pTime) const;
	bool GetStartTimeUtc(SYSTEMTIME *pTime) const;
	bool GetEndTimeUtc(SYSTEMTIME *pTime) const;
	bool GetStartTimeLocal(SYSTEMTIME *pTime) const;
	bool GetEndTimeLocal(SYSTEMTIME *pTime) const;
};


bool EpgTimeToUtc(const SYSTEMTIME *pEpgTime, SYSTEMTIME *pUtc);
bool UtcToEpgTime(const SYSTEMTIME *pUtc, SYSTEMTIME *pEpgTime);
bool EpgTimeToLocalTime(const SYSTEMTIME *pEpgTime, SYSTEMTIME *pLocalTime);
bool GetCurrentEpgTime(SYSTEMTIME *pTime);
int GetEventExtendedText(const CDescBlock *pDescBlock, LPTSTR pszText, int MaxLength);
