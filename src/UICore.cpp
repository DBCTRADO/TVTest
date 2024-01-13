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
#include <algorithm>
#include "TVTest.h"
#include "AppMain.h"
#include "DPIUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CUICore::CUICore(CAppMain &App)
	: m_App(App)
{
}


CUICore::~CUICore()
{
	if (m_hPowerRequest != INVALID_HANDLE_VALUE) {
		SetPowerRequest(false);
		::CloseHandle(m_hPowerRequest);
	}
}


bool CUICore::SetSkin(CUISkin *pSkin)
{
	if (m_pSkin != nullptr) {
		m_App.AppEventManager.RemoveEventHandler(m_pSkin);
		m_pSkin->m_pCore = nullptr;
	}
	if (pSkin != nullptr) {
		pSkin->m_pCore = this;
		m_App.AppEventManager.AddEventHandler(pSkin);
	}
	m_pSkin = pSkin;
	return true;
}


HWND CUICore::GetMainWindow() const
{
	if (m_pSkin == nullptr)
		return nullptr;
	return m_pSkin->GetMainWindow();
}


HWND CUICore::GetDialogOwner() const
{
	if (m_pSkin == nullptr)
		return nullptr;
	return m_pSkin->GetVideoHostWindow();
}


int CUICore::GetNextChannel(bool fUp)
{
	int Channel = m_App.ChannelManager.GetNextChannel(
		m_App.OperationOptions.GetChannelUpDownOrder(), fUp);
	if (Channel < 0)
		return -1;

	if (m_App.OperationOptions.GetChannelUpDownSkipSubChannel()) {
		// 異なる番組を放送している場合以外はサブチャンネルをスキップする
		const CChannelList *pCurChannelList = m_App.ChannelManager.GetCurrentChannelList();
		const int CurChannel =
			m_App.ChannelManager.GetChangingChannel() >= 0 ?
			m_App.ChannelManager.GetChangingChannel() :
			m_App.ChannelManager.GetCurrentChannel();
		const CChannelInfo *pCurChannelInfo = pCurChannelList->GetChannelInfo(CurChannel);
		LibISDB::DateTime Time;
		LibISDB::EventInfo EventInfo;
		LibISDB::EventInfo::CommonEventInfo CurEvent;

		LibISDB::GetCurrentEPGTime(&Time);
		if (m_App.EPGDatabase.GetEventInfo(
					pCurChannelInfo->GetNetworkID(),
					pCurChannelInfo->GetTransportStreamID(),
					pCurChannelInfo->GetServiceID(),
					Time, &EventInfo)) {
			if (EventInfo.IsCommonEvent) {
				CurEvent = EventInfo.CommonEvent;
			} else {
				CurEvent.ServiceID = EventInfo.ServiceID;
				CurEvent.EventID = EventInfo.EventID;
			}
		}

		bool fDiffEvent = false;
		int i;

		for (i = pCurChannelList->NumEnableChannels() - 1; i > 0; i--) {
			const CChannelInfo *pNextChannelInfo = pCurChannelList->GetChannelInfo(Channel);

			if (pCurChannelInfo->GetNetworkID() != pNextChannelInfo->GetNetworkID()
					|| pCurChannelInfo->GetTransportStreamID() != pNextChannelInfo->GetTransportStreamID())
				break;

			if (CurEvent.ServiceID != LibISDB::SERVICE_ID_INVALID
					&& m_App.EPGDatabase.GetEventInfo(
						pNextChannelInfo->GetNetworkID(),
						pNextChannelInfo->GetTransportStreamID(),
						pNextChannelInfo->GetServiceID(),
						Time, &EventInfo)) {
				LibISDB::EventInfo::CommonEventInfo Event;
				if (EventInfo.IsCommonEvent) {
					Event = EventInfo.CommonEvent;
				} else {
					Event.ServiceID = EventInfo.ServiceID;
					Event.EventID = EventInfo.EventID;
				}
				if (CurEvent != Event) {
					fDiffEvent = true;
					break;
				}
			}

			Channel = m_App.ChannelManager.GetNextChannel(
				Channel, m_App.OperationOptions.GetChannelUpDownOrder(), fUp);
			if (Channel == CurChannel)
				return -1;
		}

		if (i == 0)
			return -1;

		if (!fUp && !fDiffEvent) {
			// 同じ番組の最初のサービスを探す
			const CChannelInfo *pNextChannelInfo = pCurChannelList->GetChannelInfo(Channel);
			LibISDB::EventInfo::CommonEventInfo NextEvent;

			if (m_App.EPGDatabase.GetEventInfo(
						pNextChannelInfo->GetNetworkID(),
						pNextChannelInfo->GetTransportStreamID(),
						pNextChannelInfo->GetServiceID(),
						Time, &EventInfo)) {
				if (EventInfo.IsCommonEvent) {
					NextEvent = EventInfo.CommonEvent;
				} else {
					NextEvent.ServiceID = EventInfo.ServiceID;
					NextEvent.EventID = EventInfo.EventID;
				}
			}

			for (int i = Channel - 1; i >= 0; i--) {
				const CChannelInfo *pChannelInfo = pCurChannelList->GetChannelInfo(i);

				if (!pChannelInfo->IsEnabled())
					continue;

				if (pNextChannelInfo->GetNetworkID() != pChannelInfo->GetNetworkID()
						|| pNextChannelInfo->GetTransportStreamID() != pChannelInfo->GetTransportStreamID())
					break;

				if (NextEvent.ServiceID != LibISDB::SERVICE_ID_INVALID
						&& m_App.EPGDatabase.GetEventInfo(
							pChannelInfo->GetNetworkID(),
							pChannelInfo->GetTransportStreamID(),
							pChannelInfo->GetServiceID(),
							Time, &EventInfo)) {
					LibISDB::EventInfo::CommonEventInfo Event;
					if (EventInfo.IsCommonEvent) {
						Event = EventInfo.CommonEvent;
					} else {
						Event.ServiceID = EventInfo.ServiceID;
						Event.EventID = EventInfo.EventID;
					}
					if (Event != NextEvent)
						break;
				}

				Channel = i;
			}
		}
	}

	return Channel;
}


bool CUICore::InitializeViewer(BYTE VideoStreamType)
{
	if (m_pSkin == nullptr)
		return false;
	const bool fOK = m_pSkin->InitializeViewer(VideoStreamType);
	m_fViewerInitializeError = !fOK;
	return fOK;
}


bool CUICore::FinalizeViewer()
{
	if (m_pSkin == nullptr)
		return false;
	return m_pSkin->FinalizeViewer();
}


bool CUICore::IsViewerEnabled() const
{
	if (m_pSkin == nullptr)
		return false;
	return m_pSkin->IsViewerEnabled();
}


bool CUICore::EnableViewer(bool fEnable)
{
	if (m_pSkin == nullptr)
		return false;
	return m_pSkin->EnableViewer(fEnable);
}


HWND CUICore::GetViewerWindow() const
{
	if (m_pSkin == nullptr)
		return nullptr;
	return m_pSkin->GetViewerWindow();
}


bool CUICore::SetZoomRate(int Rate, int Factor)
{
	if (m_pSkin == nullptr)
		return false;
	return m_pSkin->SetZoomRate(Rate, Factor);
}


bool CUICore::GetZoomRate(int *pRate, int *pFactor) const
{
	if (m_pSkin == nullptr)
		return false;
	if (!m_pSkin->GetZoomRate(pRate, pFactor)
			|| (pRate != nullptr && *pRate < 1) || (pFactor != nullptr && *pFactor < 1))
		return false;
	return true;
}


int CUICore::GetZoomPercentage() const
{
	int Rate, Factor;

	if (!GetZoomRate(&Rate, &Factor))
		return 0;
	return ::MulDiv(Rate, 100, Factor);
}


bool CUICore::SetPanAndScan(const CCoreEngine::PanAndScanInfo &Info)
{
	if (!m_App.CoreEngine.SetPanAndScan(Info))
		return false;
	m_App.AppEventManager.OnPanAndScanChanged();
	return true;
}


bool CUICore::GetPanAndScan(CCoreEngine::PanAndScanInfo *pInfo) const
{
	return m_App.CoreEngine.GetPanAndScan(pInfo);
}


bool CUICore::SetAspectRatioType(int Type)
{
	CCoreEngine::PanAndScanInfo Info;

	if ((Type >= ASPECTRATIO_DEFAULT) && (Type < ASPECTRATIO_CUSTOM_FIRST)) {
		static const CCoreEngine::PanAndScanInfo PanAndScanList[] = {
			{0, 0,  0,  0,  0,  0,  0,  0}, // デフォルト
			{0, 0,  1,  1,  1,  1, 16,  9}, // 16:9
			{0, 3,  1, 18,  1, 24, 16,  9}, // 16:9 レターボックス
			{2, 3, 12, 18, 16, 24, 16,  9}, // 16:9 超額縁
			{2, 0, 12,  1, 16,  1,  4,  3}, // 4:3 ピラーボックス
			{0, 0,  1,  1,  1,  1,  4,  3}, // 4:3
			{0, 0,  1,  1,  1,  1, 32,  9}, // 32:9
			{0, 0,  1,  1,  2,  1, 16,  9}, // 16:9 左
			{1, 0,  1,  1,  2,  1, 16,  9}, // 16:9 右
		};

		Info = PanAndScanList[Type];
	} else if (Type >= ASPECTRATIO_CUSTOM_FIRST) {
		CPanAndScanOptions::PanAndScanInfo PanScan;
		if (!m_App.PanAndScanOptions.GetPreset(Type - ASPECTRATIO_CUSTOM_FIRST, &PanScan))
			return false;
		Info = PanScan.Info;
	} else {
		return false;
	}

	SetPanAndScan(Info);

	m_AspectRatioType = Type;

	m_App.AppEventManager.OnAspectRatioTypeChanged(Type);

	return true;
}


void CUICore::ResetAspectRatioType()
{
	m_AspectRatioType = ASPECTRATIO_DEFAULT;
}


int CUICore::GetVolume() const
{
	return m_App.CoreEngine.GetVolume();
}


bool CUICore::SetVolume(int Volume, bool fOSD)
{
	if (!m_App.CoreEngine.SetVolume(Volume))
		return false;
	m_App.AppEventManager.OnVolumeChanged(Volume);
	if (fOSD && m_pSkin != nullptr)
		m_pSkin->ShowVolumeOSD();
	return true;
}


bool CUICore::GetMute() const
{
	return m_App.CoreEngine.GetMute();
}


bool CUICore::SetMute(bool fMute)
{
	if (fMute != GetMute()) {
		if (!m_App.CoreEngine.SetMute(fMute))
			return false;
		m_App.AppEventManager.OnMuteChanged(fMute);
	}
	return true;
}


bool CUICore::SetDualMonoMode(LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode Mode)
{
	return SelectDualMonoMode(Mode, true);
}


LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode CUICore::GetDualMonoMode() const
{
	return m_App.CoreEngine.GetDualMonoMode();
}


int CUICore::GetNumAudioStreams() const
{
	return m_App.CoreEngine.GetAudioStreamCount();
}


int CUICore::GetAudioStream() const
{
	return m_App.CoreEngine.GetAudioStream();
}


int CUICore::GetAudioChannelCount() const
{
	const LibISDB::ViewerFilter *pViewer = m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();
	if (pViewer == nullptr)
		return LibISDB::ViewerFilter::AudioChannelCount_Invalid;
	return pViewer->GetAudioChannelCount();
}


bool CUICore::SetAudioStream(int Stream)
{
	if (Stream < 0 || Stream >= GetNumAudioStreams())
		return false;

	uint8_t ComponentTag;
	const LibISDB::AnalyzerFilter *pAnalyzer = m_App.CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
	if (pAnalyzer != nullptr)
		ComponentTag = pAnalyzer->GetAudioComponentTag(m_App.CoreEngine.GetServiceIndex(), Stream);
	else
		ComponentTag = LibISDB::COMPONENT_TAG_INVALID;

	CAudioManager::AudioSelectInfo SelInfo;
	if (!m_App.AudioManager.GetAudioSelectInfoByID(
				CAudioManager::MakeID(Stream, ComponentTag),
				&SelInfo))
		return false;

	return SelectAudio(SelInfo);
}


bool CUICore::SelectAudio(int Index)
{
	CAudioManager::AudioInfo Info;

	if (!m_App.AudioManager.GetAudioInfo(Index, &Info))
		return false;

	CAudioManager::AudioSelectInfo SelInfo;

	SelInfo.ID = Info.ID;
	SelInfo.DualMono = Info.DualMono;

	return SelectAudio(SelInfo);
}


bool CUICore::AutoSelectAudio()
{
	const int Index = m_App.AudioManager.FindSelectedAudio();
	if (Index >= 0)
		return SelectAudio(Index);

	CAudioManager::AudioSelectInfo DefaultAudio;
	if (m_App.AudioManager.GetDefaultAudio(&DefaultAudio) < 0)
		return false;

	return SelectAudio(DefaultAudio, false);
}


bool CUICore::SelectAudio(const CAudioManager::AudioSelectInfo &Info, bool fUpdate)
{
	int AudioIndex;
	const BYTE ComponentTag = CAudioManager::IDToComponentTag(Info.ID);

	if (ComponentTag != LibISDB::COMPONENT_TAG_INVALID) {
		const LibISDB::AnalyzerFilter *pAnalyzer = m_App.CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
		if (pAnalyzer != nullptr) {
			AudioIndex = pAnalyzer->GetAudioIndexByComponentTag(
				m_App.CoreEngine.GetServiceIndex(), ComponentTag);
		} else {
			AudioIndex = -1;
		}
	} else {
		AudioIndex = CAudioManager::IDToStreamIndex(Info.ID);
	}

	if (AudioIndex >= 0) {
		LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode DualMonoMode =
			LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Invalid;

		switch (Info.DualMono) {
		case CAudioManager::DualMonoMode::Main:
			DualMonoMode = LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Main;
			break;
		case CAudioManager::DualMonoMode::Sub:
			DualMonoMode = LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Sub;
			break;
		case CAudioManager::DualMonoMode::Both:
			DualMonoMode = LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Both;
			break;
		}

		SelectAudioStream(AudioIndex);
		if (DualMonoMode != LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Invalid)
			SelectDualMonoMode(DualMonoMode, fUpdate);
	}

	if (fUpdate)
		m_App.AudioManager.SetSelectedID(Info.ID);

	return true;
}


bool CUICore::SelectAudioStream(int Stream)
{
	if (Stream != GetAudioStream()) {
		if (!m_App.CoreEngine.SetAudioStream(Stream))
			return false;
		m_App.AppEventManager.OnAudioStreamChanged(Stream);
	}

	return true;
}


bool CUICore::SelectDualMonoMode(LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode Mode, bool fUpdate)
{
	if (!m_App.CoreEngine.SetDualMonoMode(Mode))
		return false;
	if (fUpdate) {
		m_App.AudioManager.SetSelectedDualMonoMode(
			Mode == LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Main ?
				CAudioManager::DualMonoMode::Main :
			Mode == LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Sub ?
				CAudioManager::DualMonoMode::Sub :
			Mode == LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Both ?
				CAudioManager::DualMonoMode::Both :
				CAudioManager::DualMonoMode::Invalid);
	}
	m_App.AppEventManager.OnDualMonoModeChanged(Mode);
	return true;
}


bool CUICore::SwitchAudio()
{
	const int NumChannels = GetAudioChannelCount();
	bool fResult;

	if (NumChannels == LibISDB::ViewerFilter::AudioChannelCount_DualMono) {
		fResult = SwitchDualMonoMode();
	} else {
		const int NumStreams = GetNumAudioStreams();

		if (NumStreams > 1)
			fResult = SetAudioStream((GetAudioStream() + 1) % NumStreams);
		else
			fResult = false;
	}

	return fResult;
}


bool CUICore::SwitchDualMonoMode()
{
	LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode Mode;

	switch (GetDualMonoMode()) {
	case LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Main:
		Mode = LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Sub;
		break;
	case LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Sub:
		Mode = LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Both;
		break;
	case LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Both:
	default:
		Mode = LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Main;
		break;
	}

	return SetDualMonoMode(Mode);
}


int CUICore::FormatCurrentAudioText(LPTSTR pszText, int MaxLength) const
{
	if (pszText == nullptr || MaxLength < 1)
		return 0;

	CStaticStringFormatter Formatter(pszText, MaxLength);

	const LibISDB::AnalyzerFilter *pAnalyzer = m_App.CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
	if (pAnalyzer == nullptr)
		return 0;

	const int NumChannels = GetAudioChannelCount();
	if (NumChannels == LibISDB::ViewerFilter::AudioChannelCount_Invalid)
		return 0;

	const int NumAudio = GetNumAudioStreams();
	LibISDB::AnalyzerFilter::EventAudioInfo AudioInfo;

	if (NumAudio > 1)
		Formatter.AppendFormat(TEXT("#{}: "), GetAudioStream() + 1);

	if (NumChannels == LibISDB::ViewerFilter::AudioChannelCount_DualMono) {
		// Dual mono
		const LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode DualMonoMode = GetDualMonoMode();

		if (pAnalyzer->GetEventAudioInfo(m_App.CoreEngine.GetServiceIndex(), GetAudioStream(), &AudioInfo)
				&& AudioInfo.ComponentType == 0x02
				&& AudioInfo.ESMultiLingualFlag
				// ES multilingual flag が立っているのに両方日本語の場合がある
				&& AudioInfo.LanguageCode != AudioInfo.LanguageCode2) {
			// 二カ国語
			TCHAR szLang1[LibISDB::MAX_LANGUAGE_TEXT_LENGTH];
			TCHAR szLang2[LibISDB::MAX_LANGUAGE_TEXT_LENGTH];

			Formatter.Append(TEXT("[二] "));

			switch (DualMonoMode) {
			case LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Main:
				LibISDB::GetLanguageText_ja(
					AudioInfo.LanguageCode,
					szLang1, lengthof(szLang1),
					LibISDB::LanguageTextType::Simple);
				Formatter.Append(szLang1);
				break;
			case LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Sub:
				LibISDB::GetLanguageText_ja(
					AudioInfo.LanguageCode2,
					szLang2, lengthof(szLang2),
					LibISDB::LanguageTextType::Simple);
				Formatter.Append(szLang2);
				break;
			case LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Both:
				LibISDB::GetLanguageText_ja(
					AudioInfo.LanguageCode,
					szLang1, lengthof(szLang1),
					LibISDB::LanguageTextType::Short);
				LibISDB::GetLanguageText_ja(
					AudioInfo.LanguageCode2,
					szLang2, lengthof(szLang2),
					LibISDB::LanguageTextType::Short);
				Formatter.AppendFormat(TEXT("{}+{}"), szLang1, szLang2);
				break;
			}
		} else {
			Formatter.Append(
				DualMonoMode == LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Main ?
					TEXT("主音声") :
				DualMonoMode == LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Sub ?
					TEXT("副音声") :
					TEXT("主+副音声"));
		}
	} else if (NumAudio > 1
			&& pAnalyzer->GetEventAudioInfo(m_App.CoreEngine.GetServiceIndex(), GetAudioStream(), &AudioInfo)) {
		TCHAR szFormat[16];

		switch (NumChannels) {
		case 1:
			StringCopy(szFormat, TEXT("[M]"));
			break;

		case 2:
			StringCopy(szFormat, TEXT("[S]"));
			break;

		case 6:
			StringCopy(szFormat, TEXT("[5.1]"));
			break;

		default:
			StringFormat(szFormat, TEXT("[{}ch]"), NumChannels);
			break;
		}

		TCHAR szAudio[64];
		if (!AudioInfo.Text.empty()) {
			LibISDB::String::size_type Pos = AudioInfo.Text.find(TEXT('\r'));
			if (Pos != LibISDB::String::npos) {
				TCHAR szBuf[64];
				StringCopy(szBuf, AudioInfo.Text.c_str(), Pos);
				Pos++;
				if (Pos < AudioInfo.Text.length() && AudioInfo.Text[Pos] == TEXT('\n'))
					Pos++;
				StringFormat(szBuf + Pos, lengthof(szBuf) - Pos, TEXT("/{}"), AudioInfo.Text.c_str() + Pos);
				StringUtility::ToHalfWidthNoKatakana(
					szBuf, szAudio, lengthof(szAudio));
			} else {
				StringUtility::ToHalfWidthNoKatakana(
					AudioInfo.Text.c_str(), szAudio, lengthof(szAudio));
			}

			// [S] などがあれば除去する
			LPTSTR p = ::StrStrI(szAudio, szFormat);
			if (p != nullptr) {
				int Length = ::lstrlen(szFormat);
				if (p > szAudio && *(p - 1) == _T(' ')) {
					p--;
					Length++;
				}
				if (p[Length] == _T(' '))
					Length++;
				if (p == szAudio && ::lstrlen(szAudio) == Length) {
					LibISDB::GetLanguageText_ja(
						AudioInfo.LanguageCode,
						szAudio, lengthof(szAudio),
						LibISDB::LanguageTextType::Simple);
				} else {
					std::memmove(p, p + Length, (::lstrlen(p + Length) + 1) * sizeof(TCHAR));
				}
			}
		} else {
			LibISDB::GetLanguageText_ja(
				AudioInfo.LanguageCode,
				szAudio, lengthof(szAudio),
				LibISDB::LanguageTextType::Simple);
		}

		Formatter.AppendFormat(TEXT("{} {}"), szFormat, szAudio);
	} else {
		switch (NumChannels) {
		case 1:
			Formatter.Append(TEXT("Mono"));
			break;

		case 2:
			Formatter.Append(TEXT("Stereo"));
			break;

		case 6:
			Formatter.Append(TEXT("5.1ch"));
			break;

		default:
			Formatter.AppendFormat(TEXT("{}ch"), NumChannels);
			break;
		}
	}

	return static_cast<int>(Formatter.Length());
}


bool CUICore::GetSelectedAudioText(LPTSTR pszText, int MaxLength) const
{
	if (pszText == nullptr || MaxLength < 1)
		return false;

	const LibISDB::AnalyzerFilter *pAnalyzer = m_App.CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
	LibISDB::AnalyzerFilter::EventAudioInfo AudioInfo;

	if (pAnalyzer != nullptr
			&& pAnalyzer->GetEventAudioInfo(m_App.CoreEngine.GetServiceIndex(), GetAudioStream(), &AudioInfo)) {
		if (AudioInfo.ComponentType == 0x02) {
			// Dual mono
			TCHAR szAudio1[64], szAudio2[64];

			szAudio1[0] = _T('\0');
			szAudio2[0] = _T('\0');
			if (!AudioInfo.Text.empty()) {
				LibISDB::String::size_type Pos = AudioInfo.Text.find(TEXT('\r'));
				if (Pos != LibISDB::String::npos) {
					StringCopy(szAudio1, AudioInfo.Text.c_str(), std::min(lengthof(szAudio1), Pos));
					Pos++;
					if (Pos < AudioInfo.Text.length() && AudioInfo.Text[Pos] == TEXT('\n'))
						Pos++;
					StringCopy(szAudio2, AudioInfo.Text.c_str() + Pos, lengthof(szAudio2));
				}
			}
			if (AudioInfo.ESMultiLingualFlag
					&& AudioInfo.LanguageCode != AudioInfo.LanguageCode2) {
				// 二カ国語
				if (szAudio1[0] == _T('\0'))
					LibISDB::GetLanguageText_ja(AudioInfo.LanguageCode, szAudio1, lengthof(szAudio1));
				if (szAudio2[0] == _T('\0'))
					LibISDB::GetLanguageText_ja(AudioInfo.LanguageCode2, szAudio2, lengthof(szAudio2));
			} else {
				if (szAudio1[0] == _T('\0'))
					StringCopy(szAudio1, TEXT("主音声"));
				if (szAudio2[0] == _T('\0'))
					StringCopy(szAudio2, TEXT("副音声"));
			}
			switch (GetDualMonoMode()) {
			case LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Main:
				StringCopy(pszText, szAudio1, MaxLength);
				break;
			case LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Sub:
				StringCopy(pszText, szAudio2, MaxLength);
				break;
			case LibISDB::DirectShow::AudioDecoderFilter::DualMonoMode::Both:
				StringFormat(pszText, MaxLength, TEXT("{}+{}"), szAudio1, szAudio2);
				break;
			default:
				return false;
			}
		} else {
			TCHAR szText[LibISDB::MAX_LANGUAGE_TEXT_LENGTH];
			if (AudioInfo.Text.empty())
				LibISDB::GetLanguageText_ja(AudioInfo.LanguageCode, szText, lengthof(szText));
			StringFormat(
				pszText, MaxLength, TEXT("音声{}: {}"),
				GetAudioStream() + 1,
				AudioInfo.Text.empty() ? szText : AudioInfo.Text.c_str());
		}
	} else {
		StringFormat(pszText, MaxLength, TEXT("音声{}"), GetAudioStream() + 1);
	}

	return true;
}


void CUICore::ShowAudioOSD()
{
	if (m_App.OSDOptions.IsOSDEnabled(COSDOptions::OSDType::Audio)) {
		TCHAR szText[128];

		if (GetSelectedAudioText(szText, lengthof(szText)))
			m_App.OSDManager.ShowOSD(szText);
	}
}


bool CUICore::SetStandby(bool fStandby, bool fTransient)
{
	if (m_fStandby != fStandby) {
		if (m_pSkin != nullptr) {
			if (!m_pSkin->SetStandby(fStandby))
				return false;
		}
		m_fStandby = fStandby;
		m_fTransientStandby = fStandby && fTransient;
		m_App.TaskTrayManager.SetStatus(
			fStandby ?
				CTaskTrayManager::StatusFlag::Standby :
				CTaskTrayManager::StatusFlag::None,
			CTaskTrayManager::StatusFlag::Standby);
		m_App.AppEventManager.OnStandbyChanged(fStandby);
	}
	return true;
}


bool CUICore::GetResident() const
{
	return m_fResident;
}


bool CUICore::SetResident(bool fResident)
{
	if (m_fResident != fResident) {
		if (!m_App.TaskTrayManager.SetResident(fResident))
			return false;
		m_fResident = fResident;
	}
	return true;
}


bool CUICore::SetFullscreen(bool fFullscreen)
{
	if (m_fFullscreen != fFullscreen) {
		if (m_pSkin == nullptr)
			return false;
		if (!m_pSkin->SetFullscreen(fFullscreen))
			return false;
		m_fFullscreen = fFullscreen;
		UpdateTitle();
		m_App.AppEventManager.OnFullscreenChanged(fFullscreen);
	}
	return true;
}


bool CUICore::ToggleFullscreen()
{
	return SetFullscreen(!m_fFullscreen);
}


bool CUICore::SetAlwaysOnTop(bool fTop)
{
	if (m_fAlwaysOnTop != fTop) {
		if (m_pSkin == nullptr)
			return false;
		if (!m_pSkin->SetAlwaysOnTop(fTop))
			return false;
		m_fAlwaysOnTop = fTop;
	}
	return true;
}


bool CUICore::PreventDisplaySave(bool fPrevent)
{
	if (fPrevent) {
		const bool fNoScreenSaver = m_App.GeneralOptions.GetNoScreenSaver();
		const bool fNoMonitorLowPower = m_App.GeneralOptions.GetNoMonitorLowPower();
		const bool fNoMonitorLowPowerActiveOnly = m_App.GeneralOptions.GetNoMonitorLowPowerActiveOnly();

		if (!fNoScreenSaver && m_fScreenSaverActiveOriginal) {
			SystemParametersInfo(
				SPI_SETSCREENSAVEACTIVE, TRUE, nullptr,
				SPIF_UPDATEINIFILE/* | SPIF_SENDWININICHANGE*/);
			m_fScreenSaverActiveOriginal = FALSE;
		}
		if (!fNoMonitorLowPower || fNoMonitorLowPowerActiveOnly)
			SetPowerRequest(false);

		if (fNoScreenSaver && !m_fScreenSaverActiveOriginal) {
			if (!SystemParametersInfo(
						SPI_GETSCREENSAVEACTIVE, 0,
						&m_fScreenSaverActiveOriginal, 0))
				m_fScreenSaverActiveOriginal = FALSE;
			if (m_fScreenSaverActiveOriginal) {
				SystemParametersInfo(
					SPI_SETSCREENSAVEACTIVE, FALSE, nullptr,
					0/*SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE*/);
			}
		}
		if (fNoMonitorLowPower && !fNoMonitorLowPowerActiveOnly)
			SetPowerRequest(true);
	} else {
		SetPowerRequest(false);

		if (m_fScreenSaverActiveOriginal) {
			::SystemParametersInfo(
				SPI_SETSCREENSAVEACTIVE, TRUE, nullptr,
				SPIF_UPDATEINIFILE/* | SPIF_SENDWININICHANGE*/);
			m_fScreenSaverActiveOriginal = FALSE;
		}
	}
	return true;
}


bool CUICore::SetPowerRequest(bool fSet)
{
	if (fSet == m_fPowerRequestSet)
		return true;

	if (fSet) {
		if (m_hPowerRequest == INVALID_HANDLE_VALUE) {
			REASON_CONTEXT ReasonContext;
			ReasonContext.Version = POWER_REQUEST_CONTEXT_VERSION;
			ReasonContext.Flags = POWER_REQUEST_CONTEXT_DETAILED_STRING;
			ReasonContext.Reason.Detailed.LocalizedReasonModule = m_App.GetResourceInstance();
			ReasonContext.Reason.Detailed.LocalizedReasonId = IDS_POWERREQUEST;
			ReasonContext.Reason.Detailed.ReasonStringCount = 0;
			ReasonContext.Reason.Detailed.ReasonStrings = nullptr;
			m_hPowerRequest = ::PowerCreateRequest(&ReasonContext);
			if (m_hPowerRequest == INVALID_HANDLE_VALUE)
				return false;
		}

		if (!::PowerSetRequest(m_hPowerRequest, PowerRequestDisplayRequired))
			return false;
		m_fPowerRequestSet = true;
	} else {
		::PowerClearRequest(m_hPowerRequest, PowerRequestDisplayRequired);
		m_fPowerRequestSet = false;
	}

	return true;
}


void CUICore::PopupMenu(const POINT *pPos, PopupMenuFlag Flags)
{
	POINT pt;

	if (pPos != nullptr)
		pt = *pPos;
	else
		::GetCursorPos(&pt);

	const bool fDefault = !!(Flags & PopupMenuFlag::Default);
	std::vector<int> ItemList;
	if (!fDefault)
		m_App.MenuOptions.GetMenuItemList(&ItemList);

	const int OldDPI = m_PopupMenuDPI;
	const HMONITOR hMonitor = ::MonitorFromPoint(pt, MONITOR_DEFAULTTONULL);
	if (hMonitor != nullptr)
		m_PopupMenuDPI = GetMonitorDPI(hMonitor);

	m_App.MainMenu.Show(
		TPM_RIGHTBUTTON, pt.x, pt.y, m_pSkin->GetMainWindow(), true,
		fDefault ? nullptr : &ItemList);

	m_PopupMenuDPI = OldDPI;
}


void CUICore::PopupSubMenu(int SubMenu, const POINT *pPos, UINT Flags, const RECT *pExcludeRect)
{
	POINT pt;

	if (pPos != nullptr)
		pt = *pPos;
	else
		::GetCursorPos(&pt);

	const int OldDPI = m_PopupMenuDPI;
	const HMONITOR hMonitor = ::MonitorFromPoint(pt, MONITOR_DEFAULTTONULL);
	if (hMonitor != nullptr)
		m_PopupMenuDPI = GetMonitorDPI(hMonitor);

	m_App.MainMenu.PopupSubMenu(SubMenu, Flags, m_pSkin->GetMainWindow(), &pt, true, pExcludeRect);

	m_PopupMenuDPI = OldDPI;
}


bool CUICore::ShowSpecialMenu(MenuType Menu, const POINT *pPos, UINT Flags, const RECT *pExcludeRect)
{
	POINT pt;

	if (pPos != nullptr)
		pt = *pPos;
	else
		::GetCursorPos(&pt);

	switch (Menu) {
	case MenuType::TunerSelect:
		m_TunerSelectMenu.Create(GetMainWindow());
		m_TunerSelectMenu.Show(Flags, pt.x, pt.y, pExcludeRect);
		m_TunerSelectMenu.Destroy();
		break;

	case MenuType::Record:
		{
			CPopupMenu Menu(m_App.GetResourceInstance(), IDM_RECORD);

			Menu.CheckItem(CM_RECORDEVENT, m_App.RecordManager.GetStopOnEventEnd());
			Menu.EnableItem(CM_RECORD_PAUSE, m_App.RecordManager.IsRecording());
			Menu.CheckItem(CM_RECORD_PAUSE, m_App.RecordManager.IsPaused());
			const bool fTimeShift = m_App.RecordOptions.IsTimeShiftRecordingEnabled();
			Menu.EnableItem(CM_TIMESHIFTRECORDING, fTimeShift && !m_App.RecordManager.IsRecording());
			Menu.CheckItem(CM_ENABLETIMESHIFTRECORDING, fTimeShift);
			Menu.EnableItem(
				CM_SHOWRECORDREMAINTIME,
				m_App.RecordManager.IsRecording() && m_App.RecordManager.IsStopTimeSpecified());
			Menu.CheckItem(CM_SHOWRECORDREMAINTIME, m_App.RecordOptions.GetShowRemainTime());
			Menu.CheckItem(CM_EXITONRECORDINGSTOP, m_App.Core.GetExitOnRecordingStop());
			m_App.Accelerator.SetMenuAccel(Menu.GetPopupHandle());
			Menu.Show(GetMainWindow(), &pt, Flags, pExcludeRect);
		}
		break;

	case MenuType::Capture:
		{
			CPopupMenu Menu(m_App.GetResourceInstance(), IDM_CAPTURE);

			Menu.CheckRadioItem(
				CM_CAPTURESIZE_FIRST, CM_CAPTURESIZE_LAST,
				CM_CAPTURESIZE_FIRST + m_App.CaptureOptions.GetPresetCaptureSize());
			Menu.CheckItem(CM_CAPTUREPREVIEW, m_App.CaptureWindow.GetVisible());
			m_App.Accelerator.SetMenuAccel(Menu.GetPopupHandle());
			Menu.Show(GetMainWindow(), &pt, Flags, pExcludeRect);
		}
		break;

	case MenuType::Buffering:
		{
			CPopupMenu Menu(m_App.GetResourceInstance(), IDM_BUFFERING);

			Menu.CheckItem(CM_ENABLEBUFFERING, m_App.CoreEngine.GetPacketBuffering());
			Menu.Show(GetMainWindow(), &pt, Flags, pExcludeRect);
		}
		break;

	case MenuType::StreamError:
		{
			CPopupMenu Menu(m_App.GetResourceInstance(), IDM_ERROR);

			Menu.Show(GetMainWindow(), &pt, Flags, pExcludeRect);
		}
		break;

	case MenuType::Clock:
		{
			CPopupMenu Menu(m_App.GetResourceInstance(), IDM_TIME);

			Menu.CheckItem(CM_SHOWTOTTIME, m_App.StatusOptions.GetShowTOTTime());
			Menu.CheckItem(CM_INTERPOLATETOTTIME, m_App.StatusOptions.GetInterpolateTOTTime());
			Menu.EnableItem(CM_INTERPOLATETOTTIME, m_App.StatusOptions.GetShowTOTTime());
			Menu.Show(GetMainWindow(), &pt, Flags, pExcludeRect);
		}
		break;

	case MenuType::ProgramInfo:
		{
			CPopupMenu Menu(m_App.GetResourceInstance(), IDM_PROGRAMINFOSTATUS);

			Menu.CheckItem(
				CM_PROGRAMINFOSTATUS_POPUPINFO,
				m_App.StatusOptions.IsPopupProgramInfoEnabled());
			Menu.CheckItem(
				CM_PROGRAMINFOSTATUS_SHOWPROGRESS,
				m_App.StatusOptions.GetShowEventProgress());
			Menu.Show(GetMainWindow(), &pt, Flags, pExcludeRect);
		}
		break;

	default:
		return false;
	}

	return true;
}


void CUICore::InitChannelMenu(HMENU hmenu)
{
	const CChannelList *pList = m_App.ChannelManager.GetCurrentChannelList();

	m_App.ChannelMenu.Destroy();
	ClearMenu(hmenu);
	if (pList == nullptr)
		return;

	if (!m_App.CoreEngine.IsNetworkDriver()) {
		CreateChannelMenu(
			pList, m_App.ChannelManager.GetCurrentChannel(),
			CM_CHANNEL_FIRST, CM_CHANNEL_LAST, hmenu, GetMainWindow());
	} else {
		const bool fControlKeyID = pList->HasRemoteControlKeyID();
		for (int i = 0, j = 0; i < pList->NumChannels(); i++) {
			const CChannelInfo *pChInfo = pList->GetChannelInfo(i);
			TCHAR szText[MAX_CHANNEL_NAME + 4];

			if (pChInfo->IsEnabled()) {
				StringFormat(
					szText, TEXT("{}: {}"),
					fControlKeyID ? pChInfo->GetChannelNo() : i + 1, pChInfo->GetName());
				::AppendMenu(
					hmenu,
					MF_STRING | MF_ENABLED
						| (j != 0 && j % m_App.MenuOptions.GetMaxChannelMenuRows() == 0 ? MF_MENUBREAK : 0),
					CM_CHANNEL_FIRST + i <= CM_CHANNEL_LAST ? CM_CHANNEL_FIRST + i : 0,
					szText);
				j++;
			}
		}
		if (m_App.ChannelManager.GetCurrentChannel() >= 0
				&& pList->IsEnabled(m_App.ChannelManager.GetCurrentChannel())) {
			::CheckMenuRadioItem(
				hmenu, CM_CHANNEL_FIRST,
				CM_CHANNEL_FIRST + pList->NumChannels() - 1,
				CM_CHANNEL_FIRST + m_App.ChannelManager.GetCurrentChannel(),
				MF_BYCOMMAND);
		}
	}
}


void CUICore::InitTunerMenu(HMENU hmenu)
{
	m_App.ChannelMenu.Destroy();

	CPopupMenu Menu(hmenu, false);
	Menu.Clear();

	const bool fIsTunerOpen = m_App.CoreEngine.IsTunerOpen();
	TCHAR szText[256];

	// 各チューニング空間のメニューを追加する
	// 実際のメニューの設定は WM_INITMENUPOPUP で行っている
	if (fIsTunerOpen && m_App.ChannelManager.NumSpaces() > 0) {
		if (m_App.ChannelManager.NumSpaces() > 1) {
			HMENU hmenuSpace = ::CreatePopupMenu();
			Menu.Append(hmenuSpace, TEXT("&A: すべて"));
		}

		for (int i = 0; i < m_App.ChannelManager.NumSpaces(); i++) {
			HMENU hmenuSpace = ::CreatePopupMenu();
			const size_t Length = StringFormat(szText, TEXT("&{}: "), i);
			LPCTSTR pszName = m_App.ChannelManager.GetTuningSpaceName(i);
			if (pszName != nullptr) {
				CopyToMenuText(pszName, szText + Length, static_cast<int>(lengthof(szText) - Length));
			} else {
				StringFormat(
					szText + Length, lengthof(szText) - Length,
					TEXT("チューニング空間{}"), i);
			}
			const CChannelList *pChannelList = m_App.ChannelManager.GetChannelList(i);
			Menu.Append(
				hmenuSpace, szText,
				pChannelList != nullptr && pChannelList->NumEnableChannels() > 0 ? MF_ENABLED : MF_GRAYED);
		}

		Menu.AppendSeparator();
	}

	::LoadString(m_App.GetResourceInstance(), CM_CHANNELDISPLAY, szText, lengthof(szText));
	Menu.Append(
		CM_CHANNELDISPLAY, szText,
		MF_ENABLED | (m_App.ChannelDisplay.GetVisible() ? MF_CHECKED : MF_UNCHECKED));
	::AppendMenu(hmenu, MF_SEPARATOR, 0, nullptr);
	int CurDriver = -1;
	int i;
	for (i = 0; i < m_App.DriverManager.NumDrivers() && CM_DRIVER_FIRST + i < CM_DRIVER_LAST; i++) {
		const CDriverInfo *pDriverInfo = m_App.DriverManager.GetDriverInfo(i);
		StringCopy(szText, pDriverInfo->GetFileName());
		::PathRemoveExtension(szText);
		Menu.AppendUnformatted(CM_DRIVER_FIRST + i, szText);
		if (fIsTunerOpen
				&& IsEqualFileName(pDriverInfo->GetFileName(), m_App.CoreEngine.GetDriverFileName()))
			CurDriver = i;
	}
	if (CurDriver < 0 && fIsTunerOpen) {
		Menu.AppendUnformatted(CM_DRIVER_FIRST + i, m_App.CoreEngine.GetDriverFileName());
		CurDriver = i++;
	}

	if (CurDriver >= 0) {
		Menu.CheckRadioItem(
			CM_DRIVER_FIRST, CM_DRIVER_FIRST + i - 1,
			CM_DRIVER_FIRST + CurDriver);
	}

	Menu.Append(CM_DRIVER_BROWSE, TEXT("参照..."));
	Menu.AppendSeparator();
	m_App.CommandManager.GetCommandText(CM_CLOSETUNER, szText, lengthof(szText));
	Menu.Append(CM_CLOSETUNER, szText);
	Menu.EnableItem(CM_CLOSETUNER, fIsTunerOpen);

	m_App.Accelerator.SetMenuAccel(hmenu);
}


bool CUICore::ProcessTunerMenu(int Command)
{
	if (Command < CM_SPACE_CHANNEL_FIRST || Command > CM_SPACE_CHANNEL_LAST)
		return false;

	const bool fIsTunerOpen = m_App.CoreEngine.IsTunerOpen();
	int CommandBase = CM_SPACE_CHANNEL_FIRST;
	const CChannelList *pChannelList;

	if (fIsTunerOpen) {
		pChannelList = m_App.ChannelManager.GetAllChannelList();
		if (pChannelList->NumChannels() > 0) {
			if (Command - CommandBase < pChannelList->NumChannels())
				return m_App.Core.SetChannel(-1, Command - CommandBase);
			CommandBase += pChannelList->NumChannels();
		}
		for (int i = 0; i < m_App.ChannelManager.NumSpaces(); i++) {
			pChannelList = m_App.ChannelManager.GetChannelList(i);
			if (pChannelList != nullptr) {
				if (Command - CommandBase < pChannelList->NumChannels())
					return m_App.Core.SetChannel(i, Command - CommandBase);
				CommandBase += pChannelList->NumChannels();
			}
		}
	}

	for (int i = 0; i < m_App.DriverManager.NumDrivers(); i++) {
		const CDriverInfo *pDriverInfo = m_App.DriverManager.GetDriverInfo(i);

		if (fIsTunerOpen
				&& IsEqualFileName(pDriverInfo->GetFileName(), m_App.CoreEngine.GetDriverFileName()))
			continue;

		if (pDriverInfo->IsTuningSpaceListLoaded()) {
			const CTuningSpaceList *pTuningSpaceList = pDriverInfo->GetAvailableTuningSpaceList();

			for (int j = 0; j < pTuningSpaceList->NumSpaces(); j++) {
				pChannelList = pTuningSpaceList->GetChannelList(j);
				if (pChannelList != nullptr) {
					if (Command - CommandBase < pChannelList->NumChannels()) {
						if (!m_App.Core.OpenTuner(pDriverInfo->GetFileName()))
							return false;
						return m_App.Core.SetChannel(j, Command - CommandBase);
					}
					CommandBase += pChannelList->NumChannels();
				}
			}
		}
	}

	return false;
}


bool CUICore::HandleInitMenuPopup(HMENU hmenu)
{
	if (InitChannelMenuPopup(m_App.MainMenu.GetSubMenu(CMainMenu::SUBMENU_SPACE), hmenu))
		return true;

	if (m_TunerSelectMenu.OnInitMenuPopup(hmenu))
		return true;

	return false;
}


void CUICore::SetPopupMenuDPI(int DPI)
{
	m_PopupMenuDPI = DPI;
}


bool CUICore::ShowEventInfoOSD(COSDManager::EventInfoOSDFlag Flags)
{
	LibISDB::EventInfo EventInfo;

	if (!m_App.CoreEngine.GetCurrentEventInfo(&EventInfo, !!(Flags & COSDManager::EventInfoOSDFlag::Next)))
		return false;

	return m_App.OSDManager.ShowEventInfoOSD(EventInfo, Flags);
}


bool CUICore::DoCommand(int Command)
{
	if (m_pSkin == nullptr || Command <= 0 || Command > 0xFFFF)
		return false;
	::SendMessage(m_pSkin->GetMainWindow(), WM_COMMAND, MAKEWPARAM(Command, 0), 0);
	return true;
}


bool CUICore::DoCommand(LPCTSTR pszCommand)
{
	if (pszCommand == nullptr)
		return false;
	const int Command = m_App.CommandManager.ParseIDText(pszCommand);
	if (Command == 0)
		return false;
	return DoCommand(Command);
}


bool CUICore::DoCommandAsync(int Command)
{
	if (m_pSkin == nullptr || Command <= 0 || Command > 0xFFFF)
		return false;
	::PostMessage(m_pSkin->GetMainWindow(), WM_COMMAND, MAKEWPARAM(Command, 0), 0);
	return true;
}


bool CUICore::DoCommandAsync(LPCTSTR pszCommand)
{
	if (pszCommand == nullptr)
		return false;
	const int Command = m_App.CommandManager.ParseIDText(pszCommand);
	if (Command == 0)
		return false;
	return DoCommandAsync(Command);
}


bool CUICore::SetCommandEnabledState(int Command, bool fEnabled)
{
	return m_App.CommandManager.SetCommandState(
		Command,
		CCommandManager::CommandState::Disabled,
		fEnabled ? CCommandManager::CommandState::None : CCommandManager::CommandState::Disabled);
}


bool CUICore::GetCommandEnabledState(int Command) const
{
	return !(m_App.CommandManager.GetCommandState(Command) & CCommandManager::CommandState::Disabled);
}


bool CUICore::SetCommandCheckedState(int Command, bool fChecked)
{
	return m_App.CommandManager.SetCommandState(
		Command,
		CCommandManager::CommandState::Checked,
		fChecked ? CCommandManager::CommandState::Checked : CCommandManager::CommandState::None);
}


bool CUICore::GetCommandCheckedState(int Command) const
{
	return !!(m_App.CommandManager.GetCommandState(Command) & CCommandManager::CommandState::Checked);
}


bool CUICore::SetCommandRadioCheckedState(int FirstCommand, int LastCommand, int CheckedCommand)
{
	return m_App.CommandManager.SetCommandRadioCheckedState(FirstCommand, LastCommand, CheckedCommand);
}


bool CUICore::ConfirmChannelChange()
{
	if (m_App.RecordManager.IsRecording()) {
		if (!m_App.RecordOptions.ConfirmChannelChange(GetDialogOwner()))
			return false;
	}
	return true;
}


bool CUICore::ConfirmStopRecording()
{
	return m_App.RecordOptions.ConfirmStatusBarStop(GetDialogOwner());
}


bool CUICore::UpdateIcon()
{
	HICON hicoBig = nullptr, hicoSmall = nullptr;

	if (m_App.ViewOptions.GetUseLogoIcon() && m_App.CoreEngine.IsTunerOpen()) {
		const CChannelInfo *pCurChannel = m_App.ChannelManager.GetCurrentChannelInfo();

		if (pCurChannel != nullptr) {
			hicoBig = m_App.LogoManager.CreateLogoIcon(
				pCurChannel->GetNetworkID(),
				pCurChannel->GetServiceID(),
				::GetSystemMetrics(SM_CXICON),
				::GetSystemMetrics(SM_CYICON));
			hicoSmall = m_App.LogoManager.CreateLogoIcon(
				pCurChannel->GetNetworkID(),
				pCurChannel->GetServiceID(),
				::GetSystemMetrics(SM_CXSMICON),
				::GetSystemMetrics(SM_CYSMICON));
		}
	}

	const HWND hwnd = GetMainWindow();
	if (hwnd != nullptr) {
		::SendMessage(
			hwnd, WM_SETICON, ICON_BIG,
			reinterpret_cast<LPARAM>(hicoBig != nullptr ? hicoBig : m_App.GetAppIcon()));
		::SendMessage(
			hwnd, WM_SETICON, ICON_SMALL,
			reinterpret_cast<LPARAM>(hicoSmall != nullptr ? hicoSmall : m_App.GetAppIconSmall()));
	}

	m_LogoIconBig.Attach(hicoBig);
	m_LogoIconSmall.Attach(hicoSmall);

	return true;
}


static void RemoveMultipleSpaces(String &Str)
{
	// 連続する空白を除去する
	String::size_type i, j;
	for (i = 0; i < Str.length() && Str[i] == L' '; i++);
	WCHAR LastChar = L'\0';
	for (j = 0; i < Str.length(); i++) {
		if (Str[i] == L' ' && LastChar == L' ')
			continue;
		LastChar = Str[i];
		Str[j++] = LastChar;
	}
	if (LastChar == L' ')
		j--;
	Str.resize(j);
}

bool CUICore::UpdateTitle()
{
	const HWND hwnd = GetMainWindow();

	if (hwnd == nullptr)
		return false;

	LPCTSTR pszTitleTextFormat, pszWindowTextFormat = nullptr;

	pszTitleTextFormat = m_App.ViewOptions.GetTitleTextFormat();
	if (::IsIconic(hwnd)) {
		if (!IsStringEmpty(m_App.ViewOptions.GetMinimizedTitleTextFormat()))
			pszTitleTextFormat = m_App.ViewOptions.GetMinimizedTitleTextFormat();
	} else {
		if (::IsZoomed(hwnd) || m_fFullscreen) {
			if (!IsStringEmpty(m_App.ViewOptions.GetMaximizedTitleTextFormat()))
				pszTitleTextFormat = m_App.ViewOptions.GetMaximizedTitleTextFormat();
		}
		if (!IsStringEmpty(m_App.ViewOptions.GetTaskbarTitleTextFormat())) {
			pszWindowTextFormat = m_App.ViewOptions.GetTaskbarTitleTextFormat();
		}
	}

	CTitleStringMap::EventInfo EventInfo;
	String TitleText, WindowText;

	m_App.Core.GetVariableStringEventInfo(&EventInfo);
	CTitleStringMap Map(m_App, &EventInfo);
	FormatVariableString(&Map, pszTitleTextFormat, &TitleText);
	RemoveMultipleSpaces(TitleText);
	if (pszWindowTextFormat != nullptr) {
		FormatVariableString(&Map, pszWindowTextFormat, &WindowText);
		RemoveMultipleSpaces(WindowText);
	}

	m_pSkin->SetTitleText(
		TitleText.c_str(),
		!WindowText.empty() ? WindowText.c_str() : TitleText.c_str());

	return true;
}


bool CUICore::SetTitleFont(const Style::Font &Font)
{
	if (m_pSkin == nullptr)
		return false;
	return m_pSkin->SetTitleFont(Font);
}


bool CUICore::SetLogo(LPCTSTR pszFileName)
{
	if (m_pSkin == nullptr)
		return false;

	if (IsStringEmpty(pszFileName))
		return m_pSkin->SetLogo(nullptr);

	TCHAR szFileName[MAX_PATH];

	if (::PathIsRelative(pszFileName)) {
		TCHAR szTemp[MAX_PATH];
		m_App.GetAppDirectory(szTemp);
		::PathAppend(szTemp, pszFileName);
		::PathCanonicalize(szFileName, szTemp);
	} else {
		StringCopy(szFileName, pszFileName);
	}

	const HBITMAP hbm = static_cast<HBITMAP>(
		::LoadImage(
			nullptr, szFileName, IMAGE_BITMAP,
			0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION));
	if (hbm == nullptr)
		return false;

	if (!m_pSkin->SetLogo(hbm)) {
		::DeleteObject(hbm);
		return false;
	}

	return true;
}


bool CUICore::RegisterModelessDialog(CBasicDialog *pDialog)
{
	if (pDialog == nullptr)
		return false;
	if (std::ranges::find(m_ModelessDialogList, pDialog) != m_ModelessDialogList.end())
		return false;
	m_ModelessDialogList.push_back(pDialog);
	return true;
}


bool CUICore::UnregisterModelessDialog(CBasicDialog *pDialog)
{
	auto itr = std::ranges::find(m_ModelessDialogList, pDialog);
	if (itr == m_ModelessDialogList.end())
		return false;
	m_ModelessDialogList.erase(itr);
	return true;
}


bool CUICore::ProcessDialogMessage(MSG *pMessage)
{
	for (CBasicDialog *e : m_ModelessDialogList) {
		if (e->ProcessMessage(pMessage))
			return true;
	}
	return false;
}


COLORREF CUICore::GetColor(LPCTSTR pszText) const
{
	return m_App.ColorSchemeOptions.GetColor(pszText);
}


const CColorScheme *CUICore::GetCurrentColorScheme() const
{
	if (m_pColorScheme == nullptr)
		return m_App.ColorSchemeOptions.GetColorScheme();
	return m_pColorScheme;
}


// 配色を適用する
bool CUICore::ApplyColorScheme(const CColorScheme *pColorScheme)
{
	const Theme::CThemeManager ThemeManager(pColorScheme);

	m_pColorScheme = pColorScheme;

	m_App.MainWindow.SetTheme(&ThemeManager);
	m_App.StatusView.SetTheme(&ThemeManager);
	m_App.SideBar.SetTheme(&ThemeManager);
	m_App.Panel.SetTheme(&ThemeManager);
	m_App.CaptureWindow.SetTheme(&ThemeManager);
	m_App.Epg.ProgramGuide.SetTheme(&ThemeManager);
	m_App.Epg.ProgramGuideFrame.SetTheme(&ThemeManager);
	m_App.Epg.ProgramGuideDisplay.SetTheme(&ThemeManager);

	m_App.AppEventManager.OnColorSchemeChanged();

	return true;
}


HCURSOR CUICore::GetActionCursor() const
{
	return ::LoadCursor(nullptr, IDC_HAND);
}


HCURSOR CUICore::GetLinkCursor() const
{
	return ::LoadCursor(nullptr, IDC_HAND);
}


bool CUICore::ShowHelpContent(int ID)
{
	return m_App.HtmlHelpClass.ShowContent(ID);
}


void CUICore::SetProgress(int Pos, int Max)
{
	m_App.TaskbarManager.SetProgress(Pos, Max);
}


void CUICore::EndProgress()
{
	m_App.TaskbarManager.EndProgress();
}


void CUICore::SetStatusBarTrace(bool fStatusBarTrace)
{
	if (m_fStatusBarTrace != fStatusBarTrace) {
		m_fStatusBarTrace = fStatusBarTrace;
		if (!m_fStatusBarTrace)
			m_App.StatusView.SetSingleText(nullptr);
	}
}


bool CUICore::CreateChannelMenu(
	const CChannelList *pChannelList, int CurChannel,
	UINT Command, UINT LastCommand, HMENU hmenu, HWND hwnd,
	CChannelMenu::CreateFlag Flags)
{
	if (pChannelList == nullptr)
		return false;
	const bool fEventInfo =
		!!(Flags & CChannelMenu::CreateFlag::ShowEventInfo)
		|| pChannelList->NumEnableChannels() <= m_App.MenuOptions.GetMaxChannelMenuEventInfo();
	CChannelMenu::CreateFlag MenuFlags = CChannelMenu::CreateFlag::ShowLogo | Flags;
	if (fEventInfo)
		MenuFlags |= CChannelMenu::CreateFlag::ShowEventInfo;
	else
		MenuFlags |= CChannelMenu::CreateFlag::ShowToolTip;
	return m_App.ChannelMenu.Create(
		pChannelList, CurChannel,
		Command, LastCommand, hmenu, hwnd, MenuFlags,
		fEventInfo ? 0 : m_App.MenuOptions.GetMaxChannelMenuRows(),
		m_PopupMenuDPI);
}


bool CUICore::InitChannelMenuPopup(HMENU hmenuParent, HMENU hmenu)
{
	bool fChannelMenu = false;
	const int Count = ::GetMenuItemCount(hmenuParent);
	int i;
	for (i = 0; i < Count; i++) {
		if (::GetSubMenu(hmenuParent, i) == hmenu) {
			fChannelMenu = true;
			break;
		}
		if ((::GetMenuState(hmenuParent, i, MF_BYPOSITION) & MF_POPUP) == 0)
			break;
	}

	if (!fChannelMenu)
		return false;

	const CChannelManager &ChannelManager = m_App.ChannelManager;
	const CChannelList *pChannelList = ChannelManager.GetAllChannelList();
	int Command = CM_SPACE_CHANNEL_FIRST;

	if (ChannelManager.NumSpaces() > 1) {
		if (i == 0) {
			CreateChannelMenu(
				pChannelList,
				ChannelManager.GetCurrentSpace() == CChannelManager::SPACE_ALL ?
				ChannelManager.GetCurrentChannel() : -1,
				Command, CM_SPACE_CHANNEL_LAST, hmenu, GetMainWindow(),
				CChannelMenu::CreateFlag::SpaceBreak);
			return true;
		}
		i--;
	}
	if (i >= ChannelManager.NumSpaces()) {
		TRACE(TEXT("CUICore::InitChannelMenuPopup() : Invalid space {}\n"), i);
		ClearMenu(hmenu);
		return true;
	}
	Command += pChannelList->NumChannels();
	for (int j = 0; j < i; j++) {
		pChannelList = ChannelManager.GetChannelList(j);
		if (pChannelList != nullptr)
			Command += pChannelList->NumChannels();
	}
	CreateChannelMenu(
		ChannelManager.GetChannelList(i),
		ChannelManager.GetCurrentSpace() == i ?
		ChannelManager.GetCurrentChannel() : -1,
		Command, CM_SPACE_CHANNEL_LAST, hmenu, GetMainWindow());

	return true;
}


void CUICore::OnLog(LibISDB::Logger::LogType Type, LPCTSTR pszOutput)
{
	CLogItem::LogType LogType;

	switch (Type) {
	case LibISDB::Logger::LogType::Verbose:
	case LibISDB::Logger::LogType::Information:
		LogType = CLogItem::LogType::Information;
		break;
	case LibISDB::Logger::LogType::Warning:
		LogType = CLogItem::LogType::Warning;
		break;
	case LibISDB::Logger::LogType::Error:
		LogType = CLogItem::LogType::Error;
		break;
	default:
		return;
	}

	if (m_fStatusBarTrace && LogType == CLogItem::LogType::Information)
		m_App.StatusView.SetSingleText(pszOutput);
	else
		m_App.Logger.AddLogRaw(LogType, pszOutput);
}




CUICore::CTitleStringMap::CTitleStringMap(CAppMain &App, const EventInfo *pInfo)
	: m_App(App)
{
	m_Flags = Flag::NoNormalize | Flag::NoCurrentTime | Flag::NoTOTTime;

	if (pInfo)
		m_EventInfo = *pInfo;
}


bool CUICore::CTitleStringMap::GetLocalString(LPCWSTR pszKeyword, String *pString)
{
	if (::lstrcmpi(pszKeyword, TEXT("event-time")) == 0) {
		TCHAR szTime[EpgUtil::MAX_EVENT_TIME_LENGTH + 1];
		if (m_EventInfo.Event.StartTime.IsValid()
				&& EpgUtil::FormatEventTime(
					m_EventInfo.Event,
					szTime, lengthof(szTime)) > 0)
			*pString = szTime;
	} else if (::lstrcmpi(pszKeyword, TEXT("rec-circle")) == 0) {
		if (m_App.RecordManager.IsRecording())
			*pString = TEXT("●");
	} else {
		return CEventVariableStringMap::GetLocalString(pszKeyword, pString);
	}
	return true;
}


bool CUICore::CTitleStringMap::GetParameterList(ParameterGroupList *pList) const
{
	if (!CEventVariableStringMap::GetParameterList(pList))
		return false;

	static const ParameterInfo ParameterList[] = {
		{TEXT("event-time"), TEXT("番組開始～終了時間")},
		{TEXT("rec-circle"), TEXT("録画●")},
	};

	ParameterGroup &Group = pList->emplace_back();
	Group.ParameterList.insert(
		Group.ParameterList.end(),
		ParameterList,
		ParameterList + lengthof(ParameterList));

	return true;
}




CUICore::CTunerSelectMenu::CTunerSelectMenu(CUICore &UICore)
	: m_UICore(UICore)
{
}


CUICore::CTunerSelectMenu::~CTunerSelectMenu()
{
	Destroy();
}


bool CUICore::CTunerSelectMenu::Create(HWND hwnd)
{
	Destroy();

	m_Menu.Create();
	m_hwnd = hwnd;

	const CChannelManager &ChannelManager = m_UICore.m_App.ChannelManager;
	const bool fIsTunerOpen = m_UICore.m_App.CoreEngine.IsTunerOpen();
	const CChannelList *pChannelList;
	int Command = CM_SPACE_CHANNEL_FIRST;
	TCHAR szText[MAX_PATH * 2];

	if (fIsTunerOpen) {
		pChannelList = ChannelManager.GetAllChannelList();
		if (ChannelManager.NumSpaces() > 1) {
			HMENU hmenuSpace = ::CreatePopupMenu();
			m_Menu.Append(hmenuSpace, TEXT("&A: すべて"));
		}
		Command += pChannelList->NumChannels();
		for (int i = 0; i < ChannelManager.NumSpaces(); i++) {
			pChannelList = ChannelManager.GetChannelList(i);
			HMENU hmenuSpace = ::CreatePopupMenu();
			const size_t Length = StringFormat(szText, TEXT("&{}: "), i);
			LPCTSTR pszName = ChannelManager.GetTuningSpaceName(i);
			if (!IsStringEmpty(pszName))
				CopyToMenuText(pszName, szText + Length, static_cast<int>(lengthof(szText) - Length));
			else
				StringFormat(szText + Length, lengthof(szText) - Length, TEXT("チューニング空間{}"), i);
			m_Menu.Append(
				hmenuSpace, szText,
				pChannelList != nullptr && pChannelList->NumEnableChannels() > 0 ? MF_ENABLED : MF_GRAYED);
			Command += pChannelList->NumChannels();
		}

		if (Command > CM_SPACE_CHANNEL_FIRST)
			m_Menu.AppendSeparator();
	}

	CDriverManager &DriverManager = m_UICore.m_App.DriverManager;

	for (int i = 0; i < DriverManager.NumDrivers(); i++) {
		CDriverInfo *pDriverInfo = DriverManager.GetDriverInfo(i);

		if (fIsTunerOpen
				&& IsEqualFileName(
						pDriverInfo->GetFileName(),
						m_UICore.m_App.CoreEngine.GetDriverFileName())) {
			continue;
		}
		TCHAR szFileName[MAX_PATH];
		StringCopy(szFileName, pDriverInfo->GetFileName());
		::PathRemoveExtension(szFileName);

		const CTuningSpaceList *pTuningSpaceList;
		if (pDriverInfo->LoadTuningSpaceList(CDriverInfo::LoadTuningSpaceListMode::NoLoadDriver)
				&& (pTuningSpaceList = pDriverInfo->GetAvailableTuningSpaceList()) != nullptr) {
			const HMENU hmenuDriver = ::CreatePopupMenu();

			for (int j = 0; j < pTuningSpaceList->NumSpaces(); j++) {
				pChannelList = pTuningSpaceList->GetChannelList(j);
				if (pChannelList == nullptr)
					continue;
				if (pChannelList->NumEnableChannels() == 0) {
					Command += pChannelList->NumChannels();
					continue;
				}
				HMENU hmenuSpace;
				if (pTuningSpaceList->NumSpaces() > 1)
					hmenuSpace = ::CreatePopupMenu();
				else
					hmenuSpace = hmenuDriver;
				m_PopupList.emplace_back(pChannelList, Command);
				MENUINFO mi;
				mi.cbSize = sizeof(mi);
				mi.fMask = MIM_MENUDATA;
				mi.dwMenuData = m_PopupList.size() - 1;
				::SetMenuInfo(hmenuSpace, &mi);
				Command += pChannelList->NumChannels();
				if (hmenuSpace != hmenuDriver) {
					LPCTSTR pszName = pTuningSpaceList->GetTuningSpaceName(j);
					const size_t Length = StringFormat(szText, TEXT("&{}: "), j);
					if (!IsStringEmpty(pszName)) {
						CopyToMenuText(pszName, szText + Length, static_cast<int>(lengthof(szText) - Length));
					} else {
						StringFormat(
							szText + Length, lengthof(szText) - Length,
							TEXT("チューニング空間{}"), j);
					}
					::AppendMenu(
						hmenuDriver, MF_POPUP | MF_ENABLED,
						reinterpret_cast<UINT_PTR>(hmenuSpace), szText);
				}
			}
			if (!IsStringEmpty(pDriverInfo->GetTunerName())) {
				TCHAR szTemp[lengthof(szText)];

				StringFormat(
					szTemp, TEXT("{} [{}]"),
					pDriverInfo->GetTunerName(),
					szFileName);
				CopyToMenuText(szTemp, szText, lengthof(szText));
			} else {
				CopyToMenuText(szFileName, szText, lengthof(szText));
			}
			m_Menu.Append(hmenuDriver, szText);
		} else {
			if (CM_DRIVER_FIRST + i <= CM_DRIVER_LAST)
				m_Menu.AppendUnformatted(CM_DRIVER_FIRST + i, szFileName);
		}
	}

	return true;
}


void CUICore::CTunerSelectMenu::Destroy()
{
	m_Menu.Destroy();
	m_hwnd = nullptr;
	m_PopupList.clear();
}


int CUICore::CTunerSelectMenu::Show(UINT Flags, int x, int y, const RECT *pExcludeRect)
{
	POINT pt = {x, y};
	return m_Menu.Show(m_hwnd, &pt, Flags, pExcludeRect);
}


bool CUICore::CTunerSelectMenu::OnInitMenuPopup(HMENU hmenu)
{
	if (!m_Menu.IsCreated())
		return false;

	if (m_UICore.InitChannelMenuPopup(m_Menu.GetPopupHandle(), hmenu))
		return true;

	bool fChannelMenu = false;
	const int Count = m_Menu.GetItemCount();
	int i = 0;
	if (m_UICore.m_App.CoreEngine.IsTunerOpen()) {
		i = m_UICore.m_App.ChannelManager.NumSpaces();
		if (i > 1)
			i++;
	}
	for (i++; i < Count; i++) {
		const HMENU hmenuChannel = m_Menu.GetSubMenu(i);
		const int Items = ::GetMenuItemCount(hmenuChannel);

		if (hmenuChannel == hmenu) {
			if (Items > 0)
				return true;
			fChannelMenu = true;
			break;
		}
		if (Items > 0) {
			int j;
			for (j = 0; j < Items; j++) {
				if (::GetSubMenu(hmenuChannel, j) == hmenu)
					break;
			}
			if (j < Items) {
				fChannelMenu = true;
				break;
			}
		}
	}

	if (fChannelMenu) {
		MENUINFO mi;

		mi.cbSize = sizeof(mi);
		mi.fMask = MIM_MENUDATA;
		if (!::GetMenuInfo(hmenu, &mi) || mi.dwMenuData >= m_PopupList.size())
			return false;
		const PopupInfo &Info = m_PopupList[mi.dwMenuData];
		m_UICore.CreateChannelMenu(Info.pChannelList, -1, Info.Command, CM_SPACE_CHANNEL_LAST, hmenu, m_hwnd);
		return true;
	}

	return false;
}


} // namespace TVTest
