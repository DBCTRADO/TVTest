#ifndef TVTEST_AUDIO_MANAGER_H
#define TVTEST_AUDIO_MANAGER_H


#include <vector>
#include <unordered_map>


namespace TVTest
{

	class CAudioManager
	{
	public:
		typedef WORD IDType;

		static const IDType ID_INVALID=0xFFFF;

		enum DualMonoMode {
			DUALMONO_INVALID,
			DUALMONO_MAIN,
			DUALMONO_SUB,
			DUALMONO_BOTH
		};

		struct AudioInfo
		{
			IDType ID;
			BYTE ComponentTag;
			BYTE ComponentType;
			DualMonoMode DualMono;
			bool fMultiLingual;
			DWORD Language;
			DWORD Language2;
			String Text;

			bool operator==(const AudioInfo &Op) const
			{
				return ID==Op.ID
					&& ComponentTag==Op.ComponentTag
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
			IDType ID;
			DualMonoMode DualMono;

			AudioSelectInfo()
				: ID(ID_INVALID)
				, DualMono(DUALMONO_INVALID)
			{
			}

			void Reset() { *this=AudioSelectInfo(); }
		};

		typedef std::vector<IDType> AudioComponentList;

		static inline IDType MakeID(int Index,BYTE ComponentTag)
		{
			return ComponentTag!=LibISDB::COMPONENT_TAG_INVALID ?
				ComponentTag :
				(IDType)(((BYTE)(INT8)Index<<8) | LibISDB::COMPONENT_TAG_INVALID);
		}
		static inline BYTE IDToComponentTag(IDType ID) { return (BYTE)(ID&0xFF); }
		static inline int IDToStreamIndex(IDType ID) { return (INT8)(BYTE)(ID>>8); }

		CAudioManager();
		int GetAudioCount() const;
		bool GetAudioInfo(int Index,AudioInfo *pInfo) const;
		bool GetAudioList(AudioList *pList) const;
		int FindAudioInfoByID(IDType ID) const;
		int GetDefaultAudio(AudioSelectInfo *pSelectInfo) const;
		bool GetAudioSelectInfoByID(IDType ID,AudioSelectInfo *pSelectInfo) const;
		void SetSelectedAudio(const AudioSelectInfo *pSelectInfo);
		bool GetSelectedAudio(AudioSelectInfo *pSelectInfo) const;
		int FindSelectedAudio() const;
		void SetSelectedID(IDType ID);
		IDType GetSelectedID() const;
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

		mutable MutexLock m_Lock;
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
