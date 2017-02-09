#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "EpgCapture.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CEpgCaptureManager::CEpgCaptureManager()
	: m_fCapturing(false)
	, m_pEventHandler(nullptr)
	, m_fChannelChanging(false)
{
}


CEpgCaptureManager::~CEpgCaptureManager()
{
}


bool CEpgCaptureManager::BeginCapture(
	LPCTSTR pszTuner,const CChannelList *pChannelList,unsigned int Flags)
{
	if (m_fCapturing)
		return false;

	CAppMain &App=GetAppClass();
	const bool fNoUI=(Flags & BEGIN_NO_UI)!=0;

	if (App.CmdLineOptions.m_fNoEpg) {
		if (!fNoUI) {
			App.UICore.GetSkin()->ShowMessage(
				TEXT("コマンドラインオプションでEPG情報を取得しないように指定されているため、\n番組表の取得ができません。"),
				TEXT("お知らせ"),MB_OK | MB_ICONINFORMATION);
		}
		return false;
	}
	if (App.RecordManager.IsRecording()) {
		if (!fNoUI) {
			App.UICore.GetSkin()->ShowMessage(
				TEXT("録画中は番組表の取得を行えません。"),
				TEXT("お知らせ"),MB_OK | MB_ICONINFORMATION);
		}
		return false;
	}
	if (!IsStringEmpty(pszTuner)) {
		CDriverManager::TunerSpec Spec;
		if (App.DriverManager.GetTunerSpec(pszTuner,&Spec)
				&& (Spec.Flags &
					(CDriverManager::TunerSpec::FLAG_NETWORK |
					 CDriverManager::TunerSpec::FLAG_FILE))!=0) {
			if (!fNoUI) {
				App.UICore.GetSkin()->ShowMessage(
					TEXT("ネットワーク再生及びファイル再生では番組表の取得はできません。"),
					TEXT("お知らせ"),MB_OK | MB_ICONINFORMATION);
			}
			return false;
		}
	}

	const bool fTunerAlreadyOpened=App.CoreEngine.IsTunerOpen();

	if (!IsStringEmpty(pszTuner)) {
		if (!App.Core.OpenTuner(pszTuner))
			return false;
	}

	if (pChannelList==nullptr) {
		pChannelList=App.ChannelManager.GetCurrentChannelList();
		if (pChannelList==nullptr) {
			if (!fTunerAlreadyOpened)
				App.Core.CloseTuner();
			return false;
		}
	}

	m_ChannelList.clear();

	for (int i=0;i<pChannelList->NumChannels();i++) {
		const CChannelInfo *pChInfo=pChannelList->GetChannelInfo(i);

		if (pChInfo->IsEnabled()) {
			const CNetworkDefinition::NetworkType Network=
				App.NetworkDefinition.GetNetworkType(pChInfo->GetNetworkID());
			std::vector<ChannelGroup>::iterator itr;

			for (itr=m_ChannelList.begin();itr!=m_ChannelList.end();++itr) {
				if (pChInfo->GetSpace()==itr->Space && pChInfo->GetChannelIndex()==itr->Channel)
					break;
				if (pChInfo->GetNetworkID()==itr->ChannelList.GetChannelInfo(0)->GetNetworkID()
						&& ((Network==CNetworkDefinition::NETWORK_BS && !App.EpgOptions.GetUpdateBSExtended())
						 || (Network==CNetworkDefinition::NETWORK_CS && !App.EpgOptions.GetUpdateCSExtended())))
					break;
			}
			if (itr==m_ChannelList.end()) {
				m_ChannelList.push_back(ChannelGroup());
				itr=m_ChannelList.end();
				--itr;
				itr->Space=pChInfo->GetSpace();
				itr->Channel=pChInfo->GetChannelIndex();
				itr->Time=0;
			}
			itr->ChannelList.AddChannel(*pChInfo);
		}
	}
	if (m_ChannelList.empty()) {
		if (!fTunerAlreadyOpened)
			App.Core.CloseTuner();
		return false;
	}

	unsigned int Status=0;
	if (fTunerAlreadyOpened)
		Status|=BEGIN_STATUS_TUNER_ALREADY_OPENED;

	if (App.UICore.GetStandby()) {
		if (!App.Core.OpenTuner())
			return false;
		Status|=BEGIN_STATUS_STANDBY;
	} else {
		if (!App.CoreEngine.IsTunerOpen())
			return false;
		if ((Flags & BEGIN_STANDBY)!=0)
			Status|=BEGIN_STATUS_STANDBY;
	}

	m_fCapturing=true;
	m_CurChannel=-1;

	App.AddLog(TEXT("番組表の取得を開始します。"));

	if (m_pEventHandler!=nullptr)
		m_pEventHandler->OnBeginCapture(Flags,Status);

	NextChannel();

	return m_fCapturing;
}


void CEpgCaptureManager::EndCapture(unsigned int Flags)
{
	if (!m_fCapturing)
		return;

	CAppMain &App=GetAppClass();

	App.AddLog(TEXT("番組表の取得を終了します。"));

	m_fCapturing=false;
	m_CurChannel=-1;
	m_ChannelList.clear();

	if (m_pEventHandler!=nullptr)
		m_pEventHandler->OnEndCapture(Flags);
}


bool CEpgCaptureManager::ProcessCapture()
{
	if (!m_fCapturing)
		return true;

	CAppMain &App=GetAppClass();
	CEventManager &EventManager=App.CoreEngine.m_DtvEngine.m_EventManager;
	const CChannelList *pChannelList=App.ChannelManager.GetCurrentChannelList();
	const CChannelInfo *pCurChannelInfo=App.ChannelManager.GetCurrentChannelInfo();
	if (pChannelList==nullptr || pCurChannelInfo==nullptr) {
		EndCapture();
		return true;
	}

	bool fComplete=true,fBasic=false,fNoBasic=false;
	ChannelGroup &CurChGroup=m_ChannelList[m_CurChannel];
	for (int i=0;i<CurChGroup.ChannelList.NumChannels();i++) {
		const CChannelInfo *pChannelInfo=CurChGroup.ChannelList.GetChannelInfo(i);
		const WORD NetworkID=pChannelInfo->GetNetworkID();
		const WORD TSID=pChannelInfo->GetTransportStreamID();
		const WORD ServiceID=pChannelInfo->GetServiceID();
		const CNetworkDefinition::NetworkType Network=
			App.NetworkDefinition.GetNetworkType(NetworkID);

		if (EventManager.HasSchedule(NetworkID,TSID,ServiceID,false)) {
			fBasic=true;
			if (!EventManager.IsScheduleComplete(NetworkID,TSID,ServiceID,false)) {
				fComplete=false;
				break;
			}
			if ((Network!=CNetworkDefinition::NETWORK_BS && Network!=CNetworkDefinition::NETWORK_CS)
					|| (Network==CNetworkDefinition::NETWORK_BS && App.EpgOptions.GetUpdateBSExtended())
					|| (Network==CNetworkDefinition::NETWORK_CS && App.EpgOptions.GetUpdateCSExtended())) {
				if (EventManager.HasSchedule(NetworkID,TSID,ServiceID,true)
						&& !EventManager.IsScheduleComplete(NetworkID,TSID,ServiceID,true)) {
					fComplete=false;
					break;
				}
			}
		} else {
			fNoBasic=true;
		}
	}

	if (fComplete && fBasic && fNoBasic
			&& m_AccumulateClock.GetSpan()<60000)
		fComplete=false;

	if (fComplete) {
		TRACE(TEXT("EPG schedule complete\n"));
	} else {
		WORD NetworkID=App.CoreEngine.m_DtvEngine.m_TsAnalyzer.GetNetworkID();
		DWORD Timeout;

		// 真面目に判定する場合BITから周期を取ってくる必要がある
		if (App.NetworkDefinition.IsSatelliteNetworkID(NetworkID))
			Timeout=360000;
		else
			Timeout=120000;
		if (m_AccumulateClock.GetSpan()<Timeout)
			return false;
		TRACE(TEXT("EPG schedule timeout\n"));
	}

	if (m_pEventHandler!=nullptr)
		m_pEventHandler->OnChannelEnd(fComplete);

	NextChannel();

	return true;
}


void CEpgCaptureManager::SetEventHandler(CEventHandler *pEventHandler)
{
	m_pEventHandler=pEventHandler;
}


DWORD CEpgCaptureManager::GetRemainingTime() const
{
	// TODO: 残り時間をちゃんと算出する
	CAppMain &App=GetAppClass();
	DWORD Time=0;

	for (size_t i=m_CurChannel;i<m_ChannelList.size();i++) {
		WORD NetworkID=m_ChannelList[i].ChannelList.GetChannelInfo(0)->GetNetworkID();
		if (App.NetworkDefinition.IsSatelliteNetworkID(NetworkID))
			Time+=180000;
		else
			Time+=60000;
	}

	return Time;
}


bool CEpgCaptureManager::NextChannel()
{
	CAppMain &App=GetAppClass();

	for (size_t i=m_CurChannel+1;i<m_ChannelList.size();i++) {
		const ChannelGroup &ChGroup=m_ChannelList[i];

		m_fChannelChanging=true;
		bool fOK=App.Core.SetChannelByIndex(ChGroup.Space,ChGroup.Channel);
		m_fChannelChanging=false;
		if (fOK) {
			m_CurChannel=(int)i;
			m_AccumulateClock.Start();
			if (m_pEventHandler!=nullptr)
				m_pEventHandler->OnChannelChanged();
			return true;
		}
	}

	EndCapture();

	return false;
}


}	// namespace TVTest
