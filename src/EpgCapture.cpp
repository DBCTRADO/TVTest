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
#include "EpgCapture.h"
#include "Common/DebugDef.h"


namespace TVTest
{


bool CEpgCaptureManager::BeginCapture(
	LPCTSTR pszTuner, const CChannelList *pChannelList, BeginFlag Flags)
{
	if (m_fCapturing)
		return false;

	CAppMain &App = GetAppClass();
	const bool fNoUI = !!(Flags & BeginFlag::NoUI);

	if (App.CmdLineOptions.m_fNoEpg) {
		if (!fNoUI) {
			App.UICore.GetSkin()->ShowMessage(
				TEXT("コマンドラインオプションでEPG情報を取得しないように指定されているため、\n番組表の取得ができません。"),
				TEXT("お知らせ"), MB_OK | MB_ICONINFORMATION);
		}
		return false;
	}
	if (App.RecordManager.IsRecording()) {
		if (!fNoUI) {
			App.UICore.GetSkin()->ShowMessage(
				TEXT("録画中は番組表の取得を行えません。"),
				TEXT("お知らせ"), MB_OK | MB_ICONINFORMATION);
		}
		return false;
	}
	if (!IsStringEmpty(pszTuner)) {
		CDriverManager::TunerSpec Spec;
		if (App.DriverManager.GetTunerSpec(pszTuner, &Spec)
				&& !!(Spec.Flags &
					(CDriverManager::TunerSpec::Flag::Network |
					 CDriverManager::TunerSpec::Flag::File))) {
			if (!fNoUI) {
				App.UICore.GetSkin()->ShowMessage(
					TEXT("ネットワーク再生及びファイル再生では番組表の取得はできません。"),
					TEXT("お知らせ"), MB_OK | MB_ICONINFORMATION);
			}
			return false;
		}
	}

	const bool fTunerAlreadyOpened = App.CoreEngine.IsTunerOpen();

	if (!IsStringEmpty(pszTuner)) {
		if (!App.Core.OpenTuner(pszTuner))
			return false;
	}

	if (pChannelList == nullptr) {
		pChannelList = App.ChannelManager.GetCurrentChannelList();
		if (pChannelList == nullptr) {
			if (!fTunerAlreadyOpened)
				App.Core.CloseTuner();
			return false;
		}
	}

	m_ChannelList.clear();

	for (int i = 0; i < pChannelList->NumChannels(); i++) {
		const CChannelInfo *pChInfo = pChannelList->GetChannelInfo(i);

		if (pChInfo->IsEnabled()) {
			const CNetworkDefinition::NetworkType Network =
				App.NetworkDefinition.GetNetworkType(pChInfo->GetNetworkID());
			std::vector<ChannelGroup>::iterator itr;

			for (itr = m_ChannelList.begin(); itr != m_ChannelList.end(); ++itr) {
				if (pChInfo->GetSpace() == itr->Space && pChInfo->GetChannelIndex() == itr->Channel)
					break;
				if (pChInfo->GetNetworkID() == itr->ChannelList.GetChannelInfo(0)->GetNetworkID()
						&& ((Network == CNetworkDefinition::NetworkType::BS && !App.EpgOptions.GetUpdateBSExtended())
							|| (Network == CNetworkDefinition::NetworkType::CS && !App.EpgOptions.GetUpdateCSExtended())))
					break;
			}
			if (itr == m_ChannelList.end()) {
				m_ChannelList.emplace_back();
				itr = m_ChannelList.end();
				--itr;
				itr->Space = pChInfo->GetSpace();
				itr->Channel = pChInfo->GetChannelIndex();
				itr->Time = 0;
			}
			itr->ChannelList.AddChannel(*pChInfo);
		}
	}
	if (m_ChannelList.empty()) {
		if (!fTunerAlreadyOpened)
			App.Core.CloseTuner();
		return false;
	}

	BeginStatus Status = BeginStatus::None;
	if (fTunerAlreadyOpened)
		Status |= BeginStatus::TunerAlreadyOpened;

	if (App.UICore.GetStandby()) {
		if (!App.Core.OpenTuner())
			return false;
		Status |= BeginStatus::Standby;
	} else {
		if (!App.CoreEngine.IsTunerOpen())
			return false;
		if (!!(Flags & BeginFlag::Standby))
			Status |= BeginStatus::Standby;
	}

	m_fCapturing = true;
	m_CurChannel = -1;

	App.AddLog(TEXT("番組表の取得を開始します。"));

	if (m_pEventHandler != nullptr)
		m_pEventHandler->OnBeginCapture(Flags, Status);

	NextChannel();

	return m_fCapturing;
}


void CEpgCaptureManager::EndCapture(EndFlag Flags)
{
	if (!m_fCapturing)
		return;

	CAppMain &App = GetAppClass();

	App.AddLog(TEXT("番組表の取得を終了します。"));

	m_fCapturing = false;
	m_CurChannel = -1;
	m_ChannelList.clear();

	if (m_pEventHandler != nullptr)
		m_pEventHandler->OnEndCapture(Flags);
}


bool CEpgCaptureManager::ProcessCapture()
{
	if (!m_fCapturing)
		return true;

	const CAppMain &App = GetAppClass();
	const LibISDB::EPGDatabase &EPGDatabase = App.EPGDatabase;

	const CChannelList *pChannelList = App.ChannelManager.GetCurrentChannelList();
	const CChannelInfo *pCurChannelInfo = App.ChannelManager.GetCurrentChannelInfo();
	if (pChannelList == nullptr || pCurChannelInfo == nullptr) {
		EndCapture();
		return true;
	}

	bool fComplete = true, fBasic = false, fNoBasic = false;
	ChannelGroup &CurChGroup = m_ChannelList[m_CurChannel];
	for (int i = 0; i < CurChGroup.ChannelList.NumChannels(); i++) {
		const CChannelInfo *pChannelInfo = CurChGroup.ChannelList.GetChannelInfo(i);
		const WORD NetworkID = pChannelInfo->GetNetworkID();
		const WORD TSID = pChannelInfo->GetTransportStreamID();
		const WORD ServiceID = pChannelInfo->GetServiceID();
		const CNetworkDefinition::NetworkType Network =
			App.NetworkDefinition.GetNetworkType(NetworkID);

		if (EPGDatabase.HasSchedule(NetworkID, TSID, ServiceID, false)) {
			fBasic = true;
			if (!EPGDatabase.IsScheduleComplete(NetworkID, TSID, ServiceID, false)) {
				fComplete = false;
				break;
			}
			if ((Network != CNetworkDefinition::NetworkType::BS && Network != CNetworkDefinition::NetworkType::CS)
					|| (Network == CNetworkDefinition::NetworkType::BS && App.EpgOptions.GetUpdateBSExtended())
					|| (Network == CNetworkDefinition::NetworkType::CS && App.EpgOptions.GetUpdateCSExtended())) {
				if (EPGDatabase.HasSchedule(NetworkID, TSID, ServiceID, true)
						&& !EPGDatabase.IsScheduleComplete(NetworkID, TSID, ServiceID, true)) {
					fComplete = false;
					break;
				}
			}
		} else {
			fNoBasic = true;
		}
	}

	if (fComplete && fBasic && fNoBasic
			&& m_AccumulateClock.GetSpan() < 60000)
		fComplete = false;

	if (fComplete) {
		TRACE(TEXT("EPG schedule complete\n"));
	} else {
		const WORD NetworkID = App.CoreEngine.GetNetworkID();
		DWORD Timeout;

		// 真面目に判定する場合BITから周期を取ってくる必要がある
		if (App.NetworkDefinition.IsSatelliteNetworkID(NetworkID))
			Timeout = 360000;
		else
			Timeout = 120000;
		if (m_AccumulateClock.GetSpan() < Timeout)
			return false;
		TRACE(TEXT("EPG schedule timeout\n"));
	}

	if (m_pEventHandler != nullptr)
		m_pEventHandler->OnChannelEnd(fComplete);

	NextChannel();

	return true;
}


void CEpgCaptureManager::SetEventHandler(CEventHandler *pEventHandler)
{
	m_pEventHandler = pEventHandler;
}


DWORD CEpgCaptureManager::GetRemainingTime() const
{
	// TODO: 残り時間をちゃんと算出する
	const CAppMain &App = GetAppClass();
	DWORD Time = 0;

	for (size_t i = m_CurChannel; i < m_ChannelList.size(); i++) {
		const WORD NetworkID = m_ChannelList[i].ChannelList.GetChannelInfo(0)->GetNetworkID();
		if (App.NetworkDefinition.IsSatelliteNetworkID(NetworkID))
			Time += 180000;
		else
			Time += 60000;
	}

	return Time;
}


bool CEpgCaptureManager::NextChannel()
{
	CAppMain &App = GetAppClass();

	for (size_t i = m_CurChannel + 1; i < m_ChannelList.size(); i++) {
		const ChannelGroup &ChGroup = m_ChannelList[i];

		m_fChannelChanging = true;
		const bool fOK = App.Core.SetChannelByIndex(ChGroup.Space, ChGroup.Channel);
		m_fChannelChanging = false;
		if (fOK) {
			m_CurChannel = static_cast<int>(i);
			m_AccumulateClock.Start();
			if (m_pEventHandler != nullptr)
				m_pEventHandler->OnChannelChanged();
			return true;
		}
	}

	EndCapture();

	return false;
}


}	// namespace TVTest
