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


#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "AudioManager.h"
#include <algorithm>
#include <utility>
#include "Common/DebugDef.h"


namespace TVTest
{


int CAudioManager::GetAudioCount() const
{
	LibISDB::BlockLock Lock(m_Lock);

	return static_cast<int>(m_AudioList.size());
}


bool CAudioManager::GetAudioInfo(int Index, AudioInfo *pInfo) const
{
	LibISDB::BlockLock Lock(m_Lock);

	if (Index < 0 || static_cast<size_t>(Index) >= m_AudioList.size() || pInfo == nullptr)
		return false;

	*pInfo = m_AudioList[Index];

	return true;
}


bool CAudioManager::GetAudioList(AudioList *pList) const
{
	if (pList == nullptr)
		return false;

	LibISDB::BlockLock Lock(m_Lock);

	*pList = m_AudioList;

	return true;
}


int CAudioManager::FindAudioInfoByID(IDType ID) const
{
	LibISDB::BlockLock Lock(m_Lock);

	for (auto it = m_AudioList.begin(); it != m_AudioList.end(); ++it) {
		if (it->ID == ID)
			return static_cast<int>(std::distance(m_AudioList.begin(), it));
	}

	return -1;
}


int CAudioManager::GetDefaultAudio(AudioSelectInfo *pSelectInfo) const
{
	LibISDB::BlockLock Lock(m_Lock);

	if (m_AudioList.empty())
		return -1;

	const CAudioOptions &AudioOptions = GetAppClass().AudioOptions;

	if (AudioOptions.GetEnableLanguagePriority()) {
		const CAudioOptions::AudioLanguageList &PriorityList = AudioOptions.GetLanguagePriority();

		if (!PriorityList.empty()) {
			for (auto const &Priority : PriorityList) {
				for (auto itAudio = m_AudioList.begin(); itAudio != m_AudioList.end(); ++itAudio) {
					if (itAudio->Language == Priority.Language
							&& itAudio->DualMono != DualMonoMode::Both) {
						if (!Priority.fSub
								|| itAudio != m_AudioList.begin()
								|| itAudio->DualMono == CAudioManager::DualMonoMode::Sub) {
							if (pSelectInfo != nullptr) {
								pSelectInfo->ID = itAudio->ID;
								pSelectInfo->DualMono = itAudio->DualMono;
							}
							return static_cast<int>(std::distance(m_AudioList.begin(), itAudio));
						}
					}
				}
			}
		}
	}

	if (pSelectInfo != nullptr) {
		const AudioInfo &Info = m_AudioList.front();

		pSelectInfo->ID = Info.ID;
		if (Info.IsDualMono()) {
			if (m_SelectedAudio.DualMono != DualMonoMode::Invalid)
				pSelectInfo->DualMono = m_SelectedAudio.DualMono;
			else
				pSelectInfo->DualMono = DualMonoMode::Main;
		} else {
			pSelectInfo->DualMono = DualMonoMode::Invalid;
		}
	}

	return 0;
}


bool CAudioManager::GetAudioSelectInfoByID(
	IDType ID, AudioSelectInfo *pSelectInfo) const
{
	if (pSelectInfo == nullptr)
		return false;

	LibISDB::BlockLock Lock(m_Lock);

	const int Index = FindAudioInfoByID(ID);
	if (Index < 0)
		return false;

	const AudioInfo &Info = m_AudioList[Index];

	pSelectInfo->ID = ID;
	if (Info.IsDualMono()) {
		if (m_SelectedAudio.DualMono != DualMonoMode::Invalid)
			pSelectInfo->DualMono = m_SelectedAudio.DualMono;
		else
			pSelectInfo->DualMono = DualMonoMode::Main;
	} else {
		pSelectInfo->DualMono = DualMonoMode::Invalid;
	}

	return true;
}


void CAudioManager::SetSelectedAudio(const AudioSelectInfo *pSelectInfo)
{
	LibISDB::BlockLock Lock(m_Lock);

	if (pSelectInfo != nullptr) {
		m_SelectedAudio = *pSelectInfo;
	} else {
		m_SelectedAudio.Reset();
	}
}


bool CAudioManager::GetSelectedAudio(AudioSelectInfo *pSelectInfo) const
{
	BlockLock Lock(m_Lock);

	if (pSelectInfo != nullptr)
		*pSelectInfo = m_SelectedAudio;
	return m_SelectedAudio.ID != ID_INVALID;
}


int CAudioManager::FindSelectedAudio() const
{
	LibISDB::BlockLock Lock(m_Lock);

	if (m_SelectedAudio.ID == ID_INVALID)
		return -1;

	for (auto it = m_AudioList.begin(); it != m_AudioList.end(); ++it) {
		if (it->ID == m_SelectedAudio.ID
				&& it->DualMono == m_SelectedAudio.DualMono)
			return static_cast<int>(std::distance(m_AudioList.begin(), it));
	}

	return -1;
}


void CAudioManager::SetSelectedID(IDType ID)
{
	LibISDB::BlockLock Lock(m_Lock);

	m_SelectedAudio.ID = ID;
}


CAudioManager::IDType CAudioManager::GetSelectedID() const
{
	LibISDB::BlockLock Lock(m_Lock);

	return m_SelectedAudio.ID;
}


bool CAudioManager::SetSelectedDualMonoMode(DualMonoMode Mode)
{
	if (!CheckEnumRange(Mode))
		return false;

	LibISDB::BlockLock Lock(m_Lock);

	m_SelectedAudio.DualMono = Mode;

	return true;
}


CAudioManager::DualMonoMode CAudioManager::GetSelectedDualMonoMode() const
{
	LibISDB::BlockLock Lock(m_Lock);

	return m_SelectedAudio.DualMono;
}


bool CAudioManager::OnServiceUpdated()
{
	// PMT の情報から音声のリストを作成
	LibISDB::BlockLock Lock(m_Lock);
	const CCoreEngine &Engine = GetAppClass().CoreEngine;
	const LibISDB::AnalyzerFilter *pAnalyzer = Engine.GetFilter<LibISDB::AnalyzerFilter>();

	if (pAnalyzer == nullptr)
		return false;

	const int StreamCount = Engine.GetAudioStreamCount();
	const int ServiceIndex = Engine.GetServiceIndex();

	AudioComponentList ComponentList;

	ComponentList.resize(StreamCount);
	for (int i = 0; i < StreamCount; i++)
		ComponentList[i] = MakeID(i, pAnalyzer->GetAudioComponentTag(ServiceIndex, i));

	const WORD TransportStreamID = Engine.GetTransportStreamID();
	const WORD ServiceID = Engine.GetServiceID();
	const bool fServiceChanged =
		TransportStreamID != m_CurTransportStreamID || ServiceID != m_CurServiceID;

	if (m_AudioComponentList == ComponentList) {
		if (!fServiceChanged)
			return false;
	} else {
		m_AudioComponentList = std::move(ComponentList);
		MakeAudioList();
	}

	if (fServiceChanged) {
		if (m_CurTransportStreamID != 0 && m_CurServiceID != 0
				&& m_SelectedAudio.ID != ID_INVALID) {
			ServiceAudioSelectInfo Info;
			Info.SelectedAudio = m_SelectedAudio;
			Info.EventID = m_CurEventID;
			auto Result = m_ServiceAudioSelectMap.emplace(
				ServiceMapKey(m_CurTransportStreamID, m_CurServiceID), Info);
			if (!Result.second)
				Result.first->second = Info;
		}

		m_CurTransportStreamID = TransportStreamID;
		m_CurServiceID = ServiceID;
		m_CurEventID = LibISDB::EVENT_ID_INVALID;

		SetSelectedAudio(nullptr);

		if (TransportStreamID != 0 && ServiceID != 0) {
			auto it = m_ServiceAudioSelectMap.find(ServiceMapKey(TransportStreamID, ServiceID));
			if (it != m_ServiceAudioSelectMap.end()) {
				m_SelectedAudio = it->second.SelectedAudio;
				m_CurEventID = it->second.EventID;
			}
		}
	} else {
		// 選択されていたIDのストリームが無くなったらリセット
		if (m_SelectedAudio.ID != ID_INVALID) {
			if (std::ranges::find(
						m_AudioComponentList,
						m_SelectedAudio.ID) == m_AudioComponentList.end())
				SetSelectedAudio(nullptr);
		}
	}

	return true;
}


bool CAudioManager::OnEventUpdated()
{
	// EIT の情報から音声のリストを作成
	LibISDB::BlockLock Lock(m_Lock);
	const CCoreEngine &Engine = GetAppClass().CoreEngine;
	const LibISDB::AnalyzerFilter *pAnalyzer = Engine.GetFilter<LibISDB::AnalyzerFilter>();
	AudioList EventAudioList;

	if (pAnalyzer != nullptr) {
		LibISDB::AnalyzerFilter::EventAudioList EITAudioList;

		pAnalyzer->GetEventAudioList(Engine.GetServiceIndex(), &EITAudioList);

		for (auto const &EventAudio : EITAudioList) {
			AudioInfo Audio1;

			Audio1.ComponentType = EventAudio.ComponentType;
			Audio1.ComponentTag = EventAudio.ComponentTag;
			if (Audio1.IsDualMono()) {
				Audio1.DualMono = DualMonoMode::Main;
				Audio1.fMultiLingual =
					EventAudio.ESMultiLingualFlag &&
					EventAudio.LanguageCode != EventAudio.LanguageCode2;
			} else {
				Audio1.DualMono = DualMonoMode::Invalid;
				Audio1.fMultiLingual = false;
			}
			Audio1.Language = EventAudio.LanguageCode;
			Audio1.Language2 = 0;
			String::size_type DelimiterPos = EventAudio.Text.find(TEXT('\r'));
			if (DelimiterPos == String::npos)
				Audio1.Text = EventAudio.Text;
			else
				Audio1.Text = EventAudio.Text.substr(0, DelimiterPos);

			EventAudioList.push_back(Audio1);

			if (Audio1.IsDualMono()) {
				AudioInfo Audio2;

				Audio2.ComponentType = EventAudio.ComponentType;
				Audio2.ComponentTag = EventAudio.ComponentTag;
				Audio2.DualMono = DualMonoMode::Sub;
				Audio2.fMultiLingual = Audio1.fMultiLingual;
				Audio2.Language =
					EventAudio.ESMultiLingualFlag ?
					EventAudio.LanguageCode2 : EventAudio.LanguageCode;
				Audio2.Language2 = 0;
				if (DelimiterPos != String::npos) {
					DelimiterPos++;
					if (DelimiterPos < EventAudio.Text.length()
							&& EventAudio.Text[DelimiterPos] == TEXT('\n'))
						DelimiterPos++;
					Audio2.Text = EventAudio.Text.substr(DelimiterPos);
				}

				EventAudioList.push_back(Audio2);

				Audio1.DualMono = DualMonoMode::Both;
				Audio1.Language2 = Audio2.Language;
				if (DelimiterPos != String::npos) {
					Audio1.Text += TEXT('+');
					Audio1.Text += Audio2.Text;
				} else {
					Audio1.Text.clear();
				}

				EventAudioList.push_back(Audio1);
			}
		}
	}

	bool fChanged = false;

	const WORD EventID = Engine.GetEventID();
	if (m_CurEventID != EventID) {
		if (m_CurEventID != LibISDB::EVENT_ID_INVALID) {
			SetSelectedAudio(nullptr);
			fChanged = true;
		}
		m_CurEventID = EventID;
	}

	if (m_EventAudioList != EventAudioList) {
		m_EventAudioList = std::move(EventAudioList);
		MakeAudioList();
		fChanged = true;
	}

	return fChanged;
}


void CAudioManager::MakeAudioList()
{
	// PMT と EIT の情報を統合して音声のリストを作成
	m_AudioList.clear();

	for (size_t i = 0; i < m_AudioComponentList.size(); i++) {
		const IDType ID = m_AudioComponentList[i];
		const BYTE ComponentTag = IDToComponentTag(ID);
		bool fFound = false;

		if (ComponentTag != LibISDB::COMPONENT_TAG_INVALID) {
			for (size_t j = 0; j < m_EventAudioList.size(); j++) {
				const AudioInfo &Info = m_EventAudioList[j];

				if (Info.ComponentTag == ComponentTag) {
					m_AudioList.emplace_back(Info).ID = ID;
					if (Info.IsDualMono()) {
						m_AudioList.emplace_back(m_EventAudioList[j + 1]).ID = ID;
						m_AudioList.emplace_back(m_EventAudioList[j + 2]).ID = ID;
					}
					fFound = true;
					break;
				}
			}
		}

		if (!fFound) {
			AudioInfo Info;

			Info.ID = ID;
			Info.ComponentTag = ComponentTag;
			Info.ComponentType = LibISDB::COMPONENT_TYPE_INVALID;
			Info.DualMono = DualMonoMode::Invalid;
			Info.fMultiLingual = false;
			Info.Language = 0;
			Info.Language2 = 0;

			m_AudioList.push_back(Info);
		}
	}
}


} // namespace TVTest
