#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "AudioManager.h"
#include <algorithm>
#include <utility>
#include "Common/DebugDef.h"


namespace TVTest
{


CAudioManager::CAudioManager()
	: m_CurTransportStreamID(0)
	, m_CurServiceID(0)
	, m_CurEventID(CTsAnalyzer::EVENTID_INVALID)
{
}


int CAudioManager::GetAudioCount() const
{
	CBlockLock Lock(&m_Lock);

	return (int)m_AudioList.size();
}


bool CAudioManager::GetAudioInfo(int Index,AudioInfo *pInfo) const
{
	CBlockLock Lock(&m_Lock);

	if (Index<0 || (size_t)Index>=m_AudioList.size() || pInfo==nullptr)
		return false;

	*pInfo=m_AudioList[Index];

	return true;
}


bool CAudioManager::GetAudioList(AudioList *pList) const
{
	if (pList==nullptr)
		return false;

	CBlockLock Lock(&m_Lock);

	*pList=m_AudioList;

	return true;
}


int CAudioManager::FindAudioInfoByID(IDType ID) const
{
	CBlockLock Lock(&m_Lock);

	for (auto it=m_AudioList.begin();it!=m_AudioList.end();++it) {
		if (it->ID==ID)
			return (int)std::distance(m_AudioList.begin(),it);
	}

	return -1;
}


int CAudioManager::GetDefaultAudio(AudioSelectInfo *pSelectInfo) const
{
	CBlockLock Lock(&m_Lock);

	if (m_AudioList.empty())
		return -1;

	const CAudioOptions &AudioOptions=GetAppClass().AudioOptions;
	bool fDefault=true;

	if (AudioOptions.GetEnableLanguagePriority()) {
		const CAudioOptions::AudioLanguageList &PriorityList=AudioOptions.GetLanguagePriority();

		if (!PriorityList.empty()) {
			for (auto itPriority=PriorityList.begin();itPriority!=PriorityList.end();++itPriority) {
				for (auto itAudio=m_AudioList.begin();itAudio!=m_AudioList.end();++itAudio) {
					if (itAudio->Language==itPriority->Language
							&& itAudio->DualMono!=DUALMONO_BOTH) {
						if (!itPriority->fSub
								|| itAudio!=m_AudioList.begin()
								|| itAudio->DualMono==CAudioManager::DUALMONO_SUB) {
							if (pSelectInfo!=nullptr) {
								pSelectInfo->ID=itAudio->ID;
								pSelectInfo->DualMono=itAudio->DualMono;
							}
							return (int)std::distance(m_AudioList.begin(),itAudio);
						}
					}
				}
			}
		}
	}

	if (pSelectInfo!=nullptr) {
		const AudioInfo &Info=m_AudioList.front();

		pSelectInfo->ID=Info.ID;
		if (Info.IsDualMono()) {
			if (m_SelectedAudio.DualMono!=DUALMONO_INVALID)
				pSelectInfo->DualMono=m_SelectedAudio.DualMono;
			else
				pSelectInfo->DualMono=DUALMONO_MAIN;
		} else {
			pSelectInfo->DualMono=DUALMONO_INVALID;
		}
	}

	return 0;
}


bool CAudioManager::GetAudioSelectInfoByID(
	IDType ID,AudioSelectInfo *pSelectInfo) const
{
	if (pSelectInfo==nullptr)
		return false;

	CBlockLock Lock(&m_Lock);

	const int Index=FindAudioInfoByID(ID);
	if (Index<0)
		return false;

	const AudioInfo &Info=m_AudioList[Index];

	pSelectInfo->ID=ID;
	if (Info.IsDualMono()) {
		if (m_SelectedAudio.DualMono!=DUALMONO_INVALID)
			pSelectInfo->DualMono=m_SelectedAudio.DualMono;
		else
			pSelectInfo->DualMono=DUALMONO_MAIN;
	} else {
		pSelectInfo->DualMono=DUALMONO_INVALID;
	}

	return true;
}


void CAudioManager::SetSelectedAudio(const AudioSelectInfo *pSelectInfo)
{
	CBlockLock Lock(&m_Lock);

	if (pSelectInfo!=nullptr) {
		m_SelectedAudio=*pSelectInfo;
	} else {
		m_SelectedAudio.Reset();
	}
}


bool CAudioManager::GetSelectedAudio(AudioSelectInfo *pSelectInfo) const
{
	CBlockLock Lock(&m_Lock);

	if (pSelectInfo!=nullptr)
		*pSelectInfo=m_SelectedAudio;
	return m_SelectedAudio.ID!=ID_INVALID;
}


int CAudioManager::FindSelectedAudio() const
{
	CBlockLock Lock(&m_Lock);

	if (m_SelectedAudio.ID==ID_INVALID)
		return -1;

	for (auto it=m_AudioList.begin();it!=m_AudioList.end();++it) {
		if (it->ID==m_SelectedAudio.ID
				&& it->DualMono==m_SelectedAudio.DualMono)
			return (int)std::distance(m_AudioList.begin(),it);
	}

	return -1;
}


void CAudioManager::SetSelectedID(IDType ID)
{
	CBlockLock Lock(&m_Lock);

	m_SelectedAudio.ID=ID;
}


CAudioManager::IDType CAudioManager::GetSelectedID() const
{
	CBlockLock Lock(&m_Lock);

	return m_SelectedAudio.ID;
}


bool CAudioManager::SetSelectedDualMonoMode(DualMonoMode Mode)
{
	if (Mode<DUALMONO_INVALID || Mode>DUALMONO_BOTH)
		return false;

	CBlockLock Lock(&m_Lock);

	m_SelectedAudio.DualMono=Mode;

	return true;
}


CAudioManager::DualMonoMode CAudioManager::GetSelectedDualMonoMode() const
{
	CBlockLock Lock(&m_Lock);

	return m_SelectedAudio.DualMono;
}


bool CAudioManager::OnServiceUpdated()
{
	// PMT の情報から音声のリストを作成
	CBlockLock Lock(&m_Lock);
	CDtvEngine &DtvEngine=GetAppClass().CoreEngine.m_DtvEngine;

	const int StreamCount=DtvEngine.GetAudioStreamNum();
	const int ServiceIndex=DtvEngine.GetServiceIndex();

	AudioComponentList ComponentList;

	ComponentList.resize(StreamCount);
	for (int i=0;i<StreamCount;i++)
		ComponentList[i]=MakeID(i,DtvEngine.m_TsAnalyzer.GetAudioComponentTag(ServiceIndex,i));

	WORD TransportStreamID,ServiceID;
	TransportStreamID=DtvEngine.m_TsAnalyzer.GetTransportStreamID();
	if (!DtvEngine.GetServiceID(&ServiceID))
		ServiceID=0;
	const bool fServiceChanged=
		TransportStreamID!=m_CurTransportStreamID || ServiceID!=m_CurServiceID;

	if (m_AudioComponentList==ComponentList) {
		if (!fServiceChanged)
			return false;
	} else {
		m_AudioComponentList=std::move(ComponentList);
		MakeAudioList();
	}

	if (fServiceChanged) {
		if (m_CurTransportStreamID!=0 && m_CurServiceID!=0
				&& m_SelectedAudio.ID!=ID_INVALID) {
			ServiceAudioSelectInfo Info;
			Info.SelectedAudio=m_SelectedAudio;
			Info.EventID=m_CurEventID;
			auto Result=m_ServiceAudioSelectMap.insert(
				std::make_pair(ServiceMapKey(m_CurTransportStreamID,m_CurServiceID),Info));
			if (!Result.second)
				Result.first->second=Info;
		}

		m_CurTransportStreamID=TransportStreamID;
		m_CurServiceID=ServiceID;
		m_CurEventID=CTsAnalyzer::EVENTID_INVALID;

		SetSelectedAudio(nullptr);

		if (TransportStreamID!=0 && ServiceID!=0) {
			auto it=m_ServiceAudioSelectMap.find(ServiceMapKey(TransportStreamID,ServiceID));
			if (it!=m_ServiceAudioSelectMap.end()) {
				m_SelectedAudio=it->second.SelectedAudio;
				m_CurEventID=it->second.EventID;
			}
		}
	} else {
		// 選択されていたIDのストリームが無くなったらリセット
		if (m_SelectedAudio.ID!=ID_INVALID) {
			if (std::find(m_AudioComponentList.begin(),
						  m_AudioComponentList.end(),
						  m_SelectedAudio.ID)==m_AudioComponentList.end())
				SetSelectedAudio(nullptr);
		}
	}

	return true;
}


bool CAudioManager::OnEventUpdated()
{
	// EIT の情報から音声のリストを作成
	CBlockLock Lock(&m_Lock);
	CDtvEngine &DtvEngine=GetAppClass().CoreEngine.m_DtvEngine;

	AudioList EventAudioList;

	for (int i=0;;i++) {
		CTsAnalyzer::EventAudioInfo EventAudio;

		if (!DtvEngine.GetEventAudioInfo(&EventAudio,i))
			break;

		AudioInfo Audio1;

		Audio1.ComponentType=EventAudio.ComponentType;
		Audio1.ComponentTag=EventAudio.ComponentTag;
		if (Audio1.IsDualMono()) {
			Audio1.DualMono=DUALMONO_MAIN;
			Audio1.fMultiLingual=
				EventAudio.bESMultiLingualFlag &&
				EventAudio.LanguageCode!=EventAudio.LanguageCode2;
		} else {
			Audio1.DualMono=DUALMONO_INVALID;
			Audio1.fMultiLingual=false;
		}
		Audio1.Language=EventAudio.LanguageCode;
		Audio1.Language2=0;
		LPCTSTR pszDelimiter=::StrChr(EventAudio.szText,TEXT('\r'));
		if (pszDelimiter==nullptr)
			Audio1.Text=EventAudio.szText;
		else
			Audio1.Text.assign(EventAudio.szText,pszDelimiter-EventAudio.szText);

		EventAudioList.push_back(Audio1);

		if (Audio1.IsDualMono()) {
			AudioInfo Audio2;

			Audio2.ComponentType=EventAudio.ComponentType;
			Audio2.ComponentTag=EventAudio.ComponentTag;
			Audio2.DualMono=DUALMONO_SUB;
			Audio2.fMultiLingual=Audio1.fMultiLingual;
			Audio2.Language=
				EventAudio.bESMultiLingualFlag?
					EventAudio.LanguageCode2 : EventAudio.LanguageCode;
			Audio2.Language2=0;
			if (pszDelimiter!=nullptr) {
				pszDelimiter++;
				if (*pszDelimiter==TEXT('\n'))
					pszDelimiter++;
				Audio2.Text.assign(pszDelimiter);
			}

			EventAudioList.push_back(Audio2);

			Audio1.DualMono=DUALMONO_BOTH;
			Audio1.Language2=Audio2.Language;
			if (pszDelimiter!=nullptr) {
				Audio1.Text+=TEXT('+');
				Audio1.Text+=Audio2.Text;
			} else {
				Audio1.Text.clear();
			}

			EventAudioList.push_back(Audio1);
		}
	}

	bool fChanged=false;

	const WORD EventID=DtvEngine.GetEventID();
	if (m_CurEventID!=EventID) {
		if (m_CurEventID!=CTsAnalyzer::EVENTID_INVALID) {
			SetSelectedAudio(nullptr);
			fChanged=true;
		}
		m_CurEventID=EventID;
	}

	if (m_EventAudioList!=EventAudioList) {
		m_EventAudioList=std::move(EventAudioList);
		MakeAudioList();
		fChanged=true;
	}

	return fChanged;
}


void CAudioManager::MakeAudioList()
{
	// PMT と EIT の情報を統合して音声のリストを作成
	m_AudioList.clear();

	for (size_t i=0;i<m_AudioComponentList.size();i++) {
		const IDType ID=m_AudioComponentList[i];
		const BYTE ComponentTag=IDToComponentTag(ID);
		bool fFound=false;

		if (ComponentTag!=COMPONENT_TAG_INVALID) {
			for (size_t j=0;j<m_EventAudioList.size();j++) {
				const AudioInfo &Info=m_EventAudioList[j];

				if (Info.ComponentTag==ComponentTag) {
					m_AudioList.push_back(Info);
					m_AudioList.back().ID=ID;
					if (Info.IsDualMono()) {
						m_AudioList.push_back(m_EventAudioList[j+1]);
						m_AudioList.back().ID=ID;
						m_AudioList.push_back(m_EventAudioList[j+2]);
						m_AudioList.back().ID=ID;
					}
					fFound=true;
					break;
				}
			}
		}

		if (!fFound) {
			AudioInfo Info;

			Info.ID=ID;
			Info.ComponentTag=ComponentTag;
			Info.ComponentType=COMPONENT_TYPE_INVALID;
			Info.DualMono=DUALMONO_INVALID;
			Info.fMultiLingual=false;
			Info.Language=0;
			Info.Language2=0;

			m_AudioList.push_back(Info);
		}
	}
}


}	// namespace TVTest
