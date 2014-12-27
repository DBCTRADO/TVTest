#ifndef TVTEST_AUDIO_MANAGER_H
#define TVTEST_AUDIO_MANAGER_H


#include <vector>
#include <unordered_map>


namespace TVTest
{

	class CAudioManager
	{
	public:
		static const BYTE COMPONENT_TAG_INVALID=0xFF;
		static const BYTE COMPONENT_TYPE_INVALID=0xFF;

		enum DualMonoMode {
			DUALMONO_INVALID,
			DUALMONO_MAIN,
			DUALMONO_SUB,
			DUALMONO_BOTH
		};

		struct AudioInfo
		{
			BYTE ComponentTag;
			BYTE ComponentType;
			DualMonoMode DualMono;
			bool fMultiLingual;
			DWORD Language;
			DWORD Language2;
			String Text;

			bool operator==(const AudioInfo &Op) const
			{
				return ComponentTag==Op.ComponentTag
					&& ComponentType==Op.ComponentType
					&& DualMono==Op.DualMono
					&& fMultiLingual==Op.fMultiLingual
					&& Language==Op.Language
					&& Language2==Op.Language2
					&& Text==Op.Text;
			}
			bool operator!=(const AudioInfo &Op) const { return !(*this==Op); }
			bool IsDualMono() const { return ComponentType==0x02; }
		};

		typedef std::vector<AudioInfo> AudioList;

		struct AudioSelectInfo
		{
			BYTE ComponentTag;
			DualMonoMode DualMono;
		};

		typedef std::vector<BYTE> AudioComponentList;

		CAudioManager();
		int GetAudioCount() const;
		bool GetAudioInfo(int Index,AudioInfo *pInfo) const;
		bool GetAudioList(AudioList *pList) const;
		int FindAudioInfoByComponentTag(BYTE ComponentTag) const;
		bool GetAudioComponentList(AudioComponentList *pList) const;
		int GetDefaultAudio(AudioSelectInfo *pSelectInfo) const;
		bool GetAudioSelectInfoByComponentTag(BYTE ComponentTag,AudioSelectInfo *pSelectInfo) const;
		void SetSelectedAudio(const AudioSelectInfo *pSelectInfo);
		bool GetSelectedAudio(AudioSelectInfo *pSelectInfo) const;
		int FindSelectedAudio() const;
		void SetSelectedComponentTag(BYTE ComponentTag);
		BYTE GetSelectedComponentTag() const;
		bool SetSelectedDualMonoMode(DualMonoMode Mode);
		DualMonoMode GetSelectedDualMonoMode() const;
		bool OnServiceUpdated();
		bool OnEventUpdated();

	private:
		struct ServiceAudioSelectInfo
		{
			AudioSelectInfo SelectedAudio;
			WORD EventID;
		};

		mutable CCriticalLock m_Lock;
		AudioComponentList m_AudioComponentList;
		AudioList m_EventAudioList;
		AudioList m_AudioList;
		AudioSelectInfo m_SelectedAudio;
		WORD m_CurTransportStreamID;
		WORD m_CurServiceID;
		WORD m_CurEventID;
		std::unordered_map<DWORD,ServiceAudioSelectInfo> m_ServiceAudioSelectMap;

		static DWORD ServiceMapKey(WORD TSID,WORD ServiceID) {
			return ((DWORD)TSID<<16) | ServiceID;
		}

		void MakeAudioList();
	};

}	// namespace TVTest


#endif
