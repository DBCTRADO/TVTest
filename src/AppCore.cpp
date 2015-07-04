#include "stdafx.h"
#include "TVTest.h"
#include "AppCore.h"
#include "AppMain.h"
#include "AppUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


using namespace TVTest;




CAppCore::CAppCore(CAppMain &App)
	: m_App(App)
	, m_fSilent(false)
	, m_fExitOnRecordingStop(false)
	, m_f1SegMode(false)
{
}


bool CAppCore::GetDriverDirectory(LPTSTR pszDirectory,int MaxLength) const
{
	return m_App.CoreEngine.GetDriverDirectory(pszDirectory,MaxLength);
}


void CAppCore::OnError(LPCTSTR pszText, ...)
{
	va_list Args;
	TCHAR szText[1024];

	va_start(Args,pszText);
	StdUtil::vsnprintf(szText,lengthof(szText),pszText,Args);
	va_end(Args);
	m_App.AddLog(CLogItem::TYPE_ERROR,TEXT("%s"),szText);
	if (!m_fSilent)
		m_App.UICore.GetSkin()->ShowErrorMessage(szText);
}


void CAppCore::OnError(const CBonErrorHandler *pErrorHandler,LPCTSTR pszTitle)
{
	if (pErrorHandler==nullptr)
		return;
	m_App.AddLog(CLogItem::TYPE_ERROR,TEXT("%s"),pErrorHandler->GetLastErrorText());
	if (!m_fSilent)
		m_App.UICore.GetSkin()->ShowErrorMessage(pErrorHandler,pszTitle);
}


void CAppCore::SetSilent(bool fSilent)
{
	m_fSilent=fSilent;
#ifndef _DEBUG
	m_App.DebugHelper.SetExceptionFilterMode(
		fSilent?CDebugHelper::EXCEPTION_FILTER_NONE:CDebugHelper::EXCEPTION_FILTER_DIALOG);
#endif
}


bool CAppCore::InitializeChannel()
{
	const bool fNetworkDriver=m_App.CoreEngine.IsNetworkDriver();
	CFilePath ChannelFilePath;

	m_App.ChannelManager.Reset();
	m_App.ChannelManager.MakeDriverTuningSpaceList(&m_App.CoreEngine.m_DtvEngine.m_BonSrcDecoder);

	if (!fNetworkDriver) {
		TCHAR szPath[MAX_PATH];
		GetChannelFileName(m_App.CoreEngine.GetDriverFileName(),szPath,MAX_PATH);
		ChannelFilePath.SetPath(szPath);
	}

	if (!ChannelFilePath.IsEmpty()) {
		if (m_App.ChannelManager.LoadChannelList(ChannelFilePath.GetPath())) {
			m_App.AddLog(TEXT("チャンネル設定を \"%s\" から読み込みました。"),
						 ChannelFilePath.GetPath());
			if (!m_App.ChannelManager.ChannelFileHasStreamIDs())
				m_App.AddLog(CLogItem::TYPE_WARNING,TEXT("チャンネルファイルが古いので再スキャンをお薦めします。"));
		}
	}

	CDriverOptions::ChannelInfo InitChInfo;
	if (m_App.DriverOptions.GetInitialChannel(m_App.CoreEngine.GetDriverFileName(),&InitChInfo)) {
		m_App.RestoreChannelInfo.Space=InitChInfo.Space;
		m_App.RestoreChannelInfo.Channel=InitChInfo.Channel;
		m_App.RestoreChannelInfo.ServiceID=InitChInfo.ServiceID;
		m_App.RestoreChannelInfo.TransportStreamID=InitChInfo.TransportStreamID;
		m_App.RestoreChannelInfo.fAllChannels=InitChInfo.fAllChannels;
	} else {
		m_App.RestoreChannelInfo.Space=-1;
		m_App.RestoreChannelInfo.Channel=-1;
		m_App.RestoreChannelInfo.ServiceID=-1;
		m_App.RestoreChannelInfo.TransportStreamID=-1;
		m_App.RestoreChannelInfo.fAllChannels=false;
	}

	m_App.ChannelManager.SetUseDriverChannelList(fNetworkDriver);
	m_App.ChannelManager.SetCurrentChannel(
		m_App.RestoreChannelInfo.fAllChannels?CChannelManager::SPACE_ALL:max(m_App.RestoreChannelInfo.Space,0),
		-1);
	m_App.ChannelManager.SetCurrentServiceID(0);
	m_App.AppEventManager.OnChannelListChanged();
	m_App.ChannelScan.SetTuningSpaceList(m_App.ChannelManager.GetTuningSpaceList());
	return true;
}


bool CAppCore::GetChannelFileName(LPCTSTR pszDriverFileName,
								  LPTSTR pszChannelFileName,int MaxChannelFileName)
{
	if (IsStringEmpty(pszDriverFileName) || pszChannelFileName==nullptr)
		return false;

	const bool fRelative=::PathIsRelative(pszDriverFileName)!=FALSE;
	TCHAR szPath[MAX_PATH],szPath2[MAX_PATH],szDir[MAX_PATH];
	if (fRelative) {
		if (!m_App.CoreEngine.GetDriverDirectory(szDir,lengthof(szDir)))
			return false;
		if (::PathCombine(szPath,szDir,pszDriverFileName)==nullptr)
			return false;
	} else {
		if (::lstrlen(pszDriverFileName)>=lengthof(szPath))
			return false;
		::lstrcpy(szPath,pszDriverFileName);
	}
	::PathRenameExtension(szPath,CHANNEL_FILE_EXTENSION);
#ifdef DEFERRED_CHANNEL_FILE_EXTENSION
	if (!::PathFileExists(szPath)) {
		::lstrcpy(szPath2,szPath);
		::PathRenameExtension(szPath2,DEFERRED_CHANNEL_FILE_EXTENSION);
		if (::PathFileExists(szPath2))
			::lstrcpy(szPath,szPath2);
	}
#endif
	if (fRelative && !::PathFileExists(szPath)) {
		m_App.GetAppDirectory(szDir);
		if (::PathCombine(szPath2,szDir,pszDriverFileName)!=nullptr) {
			::PathRenameExtension(szPath2,CHANNEL_FILE_EXTENSION);
			if (::PathFileExists(szPath2)) {
				::lstrcpy(szPath,szPath2);
			}
#ifdef DEFERRED_CHANNEL_FILE_EXTENSION
			else {
				::PathRenameExtension(szPath2,DEFERRED_CHANNEL_FILE_EXTENSION);
				if (::PathFileExists(szPath2))
					::lstrcpy(szPath,szPath2);
			}
#endif
		}
	}
	if (::lstrlen(szPath)>=MaxChannelFileName)
		return false;
	::lstrcpy(pszChannelFileName,szPath);
	return true;
}


bool CAppCore::RestoreChannel()
{
	if (m_App.RestoreChannelInfo.Space>=0 && m_App.RestoreChannelInfo.Channel>=0) {
		int Space=m_App.RestoreChannelInfo.fAllChannels?CChannelManager::SPACE_ALL:m_App.RestoreChannelInfo.Space;
		const CChannelList *pList=m_App.ChannelManager.GetChannelList(Space);
		if (pList!=nullptr) {
			int Index=pList->FindByIndex(m_App.RestoreChannelInfo.Space,
										 m_App.RestoreChannelInfo.Channel,
										 m_App.RestoreChannelInfo.ServiceID);
			if (Index<0) {
				if (m_App.RestoreChannelInfo.TransportStreamID>0 && m_App.RestoreChannelInfo.ServiceID>0) {
					Index=pList->FindByIDs(0,
										   (WORD)m_App.RestoreChannelInfo.TransportStreamID,
										   (WORD)m_App.RestoreChannelInfo.ServiceID);
				}
				if (Index<0 && m_App.RestoreChannelInfo.ServiceID>0) {
					Index=pList->FindByIndex(m_App.RestoreChannelInfo.Space,
											 m_App.RestoreChannelInfo.Channel);
				}
			}
			if (Index>=0)
				return SetChannel(Space,Index);
		}
	}
	return false;
}


bool CAppCore::UpdateCurrentChannelList(const CTuningSpaceList *pList)
{
	bool fNetworkDriver=m_App.CoreEngine.IsNetworkDriver();

	m_App.ChannelManager.SetTuningSpaceList(pList);
	m_App.ChannelManager.SetUseDriverChannelList(fNetworkDriver);
	/*
	m_App.ChannelManager.SetCurrentChannel(
		(!fNetworkDriver && m_App.ChannelManager.GetAllChannelList()->NumChannels()>0)?
			CChannelManager::SPACE_ALL:0,
		m_App.CoreEngine.IsUDPDriver()?0:-1);
	*/
	int Space=-1;
	bool fAllChannels=false;
	for (int i=0;i<pList->NumSpaces();i++) {
		if (pList->GetTuningSpaceType(i)!=CTuningSpaceInfo::SPACE_TERRESTRIAL) {
			fAllChannels=false;
			break;
		}
		if (pList->GetChannelList(i)->NumChannels()>0) {
			if (Space>=0)
				fAllChannels=true;
			else
				Space=i;
		}
	}
	m_App.ChannelManager.SetCurrentChannel(
		fAllChannels?CChannelManager::SPACE_ALL:(Space>=0?Space:0),
		-1);
	m_App.ChannelManager.SetCurrentServiceID(0);
	WORD ServiceID;
	if (m_App.CoreEngine.m_DtvEngine.GetServiceID(&ServiceID))
		FollowChannelChange(m_App.CoreEngine.m_DtvEngine.m_TsAnalyzer.GetTransportStreamID(),ServiceID);

	m_App.AppEventManager.OnChannelListChanged();

	UpdateChannelList(m_App.CoreEngine.GetDriverFileName(),pList);

	return true;
}


bool CAppCore::UpdateChannelList(LPCTSTR pszBonDriverName,const CTuningSpaceList *pList)
{
	if (IsStringEmpty(pszBonDriverName) || pList==nullptr)
		return false;

	int Index=m_App.DriverManager.FindByFileName(::PathFindFileName(pszBonDriverName));
	if (Index>=0) {
		CDriverInfo *pDriverInfo=m_App.DriverManager.GetDriverInfo(Index);
		if (pDriverInfo!=nullptr) {
			pDriverInfo->ClearTuningSpaceList();
		}
	}

	// お気に入りチャンネルの更新
	class CFavoritesChannelUpdator : public CFavoriteItemEnumerator
	{
		LPCTSTR m_pszBonDriver;
		const CTuningSpaceList *m_pTuningSpaceList;
		bool m_fUpdated;

		bool ChannelItem(CFavoriteFolder &Folder,CFavoriteChannel &Channel) override
		{
			if (IsEqualFileName(Channel.GetBonDriverFileName(),m_pszBonDriver)) {
				const CChannelInfo &ChannelInfo=Channel.GetChannelInfo();
				const CChannelList *pChannelList=m_pTuningSpaceList->GetChannelList(ChannelInfo.GetSpace());

				if (pChannelList!=nullptr) {
					if (pChannelList->FindByIDs(
							ChannelInfo.GetNetworkID(),
							ChannelInfo.GetTransportStreamID(),
							ChannelInfo.GetServiceID(),
							false)<0) {
						const int ChannelCount=pChannelList->NumChannels();
						const CNetworkDefinition &NetworkDefinition=GetAppClass().NetworkDefinition;
						const CNetworkDefinition::NetworkType ChannelNetworkType=
							NetworkDefinition.GetNetworkType(ChannelInfo.GetNetworkID());

						for (int i=0;i<ChannelCount;i++) {
							const CChannelInfo *pChInfo=pChannelList->GetChannelInfo(i);

							if (NetworkDefinition.GetNetworkType(pChInfo->GetNetworkID())==ChannelNetworkType
									&& (pChInfo->GetServiceID()==ChannelInfo.GetServiceID()
										|| ::lstrcmp(pChInfo->GetName(),ChannelInfo.GetName())==0)) {
								TRACE(TEXT("お気に入りチャンネル更新 : %s -> %s / NID %d -> %d / TSID %04x -> %04x / SID %d -> %d\n"),
									  ChannelInfo.GetName(),pChInfo->GetName(),
									  ChannelInfo.GetNetworkID(),pChInfo->GetNetworkID(),
									  ChannelInfo.GetTransportStreamID(),pChInfo->GetTransportStreamID(),
									  ChannelInfo.GetServiceID(),pChInfo->GetServiceID());
								Channel.SetChannelInfo(*pChInfo);
								m_fUpdated=true;
								break;
							}
						}
					}
				}
			}

			return true;
		}

	public:
		CFavoritesChannelUpdator(LPCTSTR pszBonDriver,const CTuningSpaceList *pTuningSpaceList)
			: m_pszBonDriver(pszBonDriver)
			, m_pTuningSpaceList(pTuningSpaceList)
			, m_fUpdated(false)
		{
		}

		bool IsUpdated() const { return m_fUpdated; }
	};

	CFavoritesChannelUpdator FavoritesUpdator(pszBonDriverName,pList);
	FavoritesUpdator.EnumItems(m_App.FavoritesManager.GetRootFolder());
	if (FavoritesUpdator.IsUpdated())
		m_App.FavoritesManager.SetModified(true);

	return true;
}


const CChannelInfo *CAppCore::GetCurrentChannelInfo() const
{
	return m_App.ChannelManager.GetCurrentChannelInfo();
}


bool CAppCore::SetChannel(int Space,int Channel,int ServiceID/*=-1*/,bool fStrictService/*=false*/)
{
	const CChannelInfo *pPrevChInfo=m_App.ChannelManager.GetCurrentChannelInfo();
	int OldSpace=m_App.ChannelManager.GetCurrentSpace(),OldChannel=m_App.ChannelManager.GetCurrentChannel();

	if (!m_App.ChannelManager.SetCurrentChannel(Space,Channel))
		return false;
	const CChannelInfo *pChInfo=m_App.ChannelManager.GetCurrentChannelInfo();
	if (pChInfo==nullptr) {
		m_App.ChannelManager.SetCurrentChannel(OldSpace,OldChannel);
		return false;
	}
	if (pPrevChInfo==nullptr
			|| pChInfo->GetSpace()!=pPrevChInfo->GetSpace()
			|| pChInfo->GetChannelIndex()!=pPrevChInfo->GetChannelIndex()) {
		if (ServiceID>0) {
			const CChannelList *pChList=m_App.ChannelManager.GetCurrentChannelList();
			int Index=pChList->FindByIndex(pChInfo->GetSpace(),
										   pChInfo->GetChannelIndex(),
										   ServiceID);
			if (Index>=0) {
				m_App.ChannelManager.SetCurrentChannel(Space,Index);
				pChInfo=pChList->GetChannelInfo(Index);
			}
		} else {
			if (pChInfo->GetServiceID()>0)
				ServiceID=pChInfo->GetServiceID();
		}

		LPCTSTR pszTuningSpace=m_App.ChannelManager.GetDriverTuningSpaceList()->GetTuningSpaceName(pChInfo->GetSpace());
		m_App.AddLog(TEXT("BonDriverにチャンネル変更を要求します。(チューニング空間 %d[%s] / Ch %d[%s] / Sv %d)"),
					 pChInfo->GetSpace(),pszTuningSpace!=nullptr?pszTuningSpace:TEXT("\?\?\?"),
					 pChInfo->GetChannelIndex(),pChInfo->GetName(),ServiceID);

		CDtvEngine::ServiceSelectInfo ServiceSel;
		ServiceSel.ServiceID=ServiceID>0?ServiceID:CDtvEngine::SID_INVALID;
		ServiceSel.bFollowViewableService=!m_App.NetworkDefinition.IsCSNetworkID(pChInfo->GetNetworkID());

		if (!fStrictService && m_f1SegMode) {
			ServiceSel.OneSegSelect=CDtvEngine::ONESEG_SELECT_HIGHPRIORITY;

			// サブチャンネルの選択の場合、ワンセグもサブチャンネルを優先する
			if (ServiceSel.ServiceID!=CDtvEngine::SID_INVALID) {
				ServiceSel.PreferredServiceIndex=
					(WORD)GetCorresponding1SegService(
						pChInfo->GetSpace(),
						pChInfo->GetNetworkID(),
						pChInfo->GetTransportStreamID(),
						ServiceSel.ServiceID);
			}
		}

		if (!m_App.CoreEngine.m_DtvEngine.SetChannel(
				pChInfo->GetSpace(),pChInfo->GetChannelIndex(),&ServiceSel)) {
			m_App.AddLog(CLogItem::TYPE_ERROR,TEXT("%s"),m_App.CoreEngine.m_DtvEngine.GetLastErrorText());
			m_App.ChannelManager.SetCurrentChannel(OldSpace,OldChannel);
			return false;
		}

		m_App.ChannelManager.SetCurrentServiceID(ServiceID);
		m_App.AppEventManager.OnChannelChanged(
			Space!=OldSpace ? AppEvent::CHANNEL_CHANGED_STATUS_SPACE_CHANGED : 0);
	} else {
		if (ServiceID<=0) {
			if (pChInfo->GetServiceID()>0)
				ServiceID=pChInfo->GetServiceID();
			else
				return false;
		}
		if (!SetServiceByID(ServiceID,fStrictService?SET_SERVICE_STRICT_ID:0))
			return false;
	}

	return true;
}


bool CAppCore::SetChannelByIndex(int Space,int Channel,int ServiceID)
{
	const CChannelList *pChannelList=m_App.ChannelManager.GetChannelList(Space);
	if (pChannelList==nullptr)
		return false;

	int ListChannel=pChannelList->FindByIndex(Space,Channel,ServiceID);
	if (ListChannel<0) {
		if (ServiceID>0)
			ListChannel=pChannelList->FindByIndex(Space,Channel);
		if (ListChannel<0)
			return false;
	}

	return SetChannel(Space,ListChannel,ServiceID);
}


bool CAppCore::SelectChannel(LPCTSTR pszTunerName,const CChannelInfo &ChannelInfo,unsigned int Flags)
{
	if ((Flags & SELECT_CHANNEL_USE_CUR_TUNER)!=0
			&& m_App.CoreEngine.IsTunerOpen()) {
		int Space=m_App.ChannelManager.GetCurrentSpace();
		if (Space!=CChannelManager::SPACE_INVALID) {
			int Index=m_App.ChannelManager.FindChannelByIDs(Space,
				ChannelInfo.GetNetworkID(),
				ChannelInfo.GetTransportStreamID(),
				ChannelInfo.GetServiceID());
			if (Index<0 && Space!=CChannelManager::SPACE_ALL) {
				for (Space=0;Space<m_App.ChannelManager.NumSpaces();Space++) {
					Index=m_App.ChannelManager.FindChannelByIDs(Space,
						ChannelInfo.GetNetworkID(),
						ChannelInfo.GetTransportStreamID(),
						ChannelInfo.GetServiceID());
					if (Index>=0)
						break;
				}
			}
			if (Index>=0) {
				if (!m_App.UICore.ConfirmChannelChange())
					return false;
				return SetChannel(Space,Index,-1,(Flags & SELECT_CHANNEL_STRICT_SERVICE)!=0);
			}
		}
	}

	if (IsStringEmpty(pszTunerName))
		return false;

	if (!m_App.UICore.ConfirmChannelChange())
		return false;

	if (!OpenTuner(pszTunerName))
		return false;

	int Space=CChannelManager::SPACE_INVALID,Channel=-1;
	const CChannelList *pChannelList;

	pChannelList=m_App.ChannelManager.GetCurrentChannelList();
	if (pChannelList!=nullptr) {
		if (ChannelInfo.GetChannelIndex()>=0) {
			Channel=pChannelList->FindByIndex(
				ChannelInfo.GetSpace(),
				ChannelInfo.GetChannelIndex(),
				ChannelInfo.GetServiceID());
		} else if (ChannelInfo.GetServiceID()>0
				&& (ChannelInfo.GetNetworkID()>0
					|| ChannelInfo.GetTransportStreamID()>0)) {
			Channel=pChannelList->FindByIDs(
				ChannelInfo.GetNetworkID(),
				ChannelInfo.GetTransportStreamID(),
				ChannelInfo.GetServiceID());
		}
		if (Channel>=0)
			Space=m_App.ChannelManager.GetCurrentSpace();
	}

	if (Channel<0) {
		if (m_App.ChannelManager.GetCurrentSpace()==CChannelManager::SPACE_ALL
				|| ChannelInfo.GetSpace()==m_App.ChannelManager.GetCurrentSpace())
			return false;
		pChannelList=m_App.ChannelManager.GetChannelList(ChannelInfo.GetSpace());
		if (pChannelList==nullptr)
			return false;

		if (ChannelInfo.GetChannelIndex()>=0) {
			Channel=pChannelList->FindByIndex(
				ChannelInfo.GetSpace(),
				ChannelInfo.GetChannelIndex(),
				ChannelInfo.GetServiceID());
		}
		if (Channel<0) {
			if (ChannelInfo.GetServiceID()>0
					&& (ChannelInfo.GetNetworkID()>0
						|| ChannelInfo.GetTransportStreamID()>0)) {
				Channel=pChannelList->FindByIDs(
					ChannelInfo.GetNetworkID(),
					ChannelInfo.GetTransportStreamID(),
					ChannelInfo.GetServiceID());
			}
			if (Channel<0)
				return false;
		}
		Space=ChannelInfo.GetSpace();
	}

	return SetChannel(Space,Channel,-1,(Flags & SELECT_CHANNEL_STRICT_SERVICE)!=0);
}


bool CAppCore::SwitchChannel(int Channel)
{
	const CChannelList *pChList=m_App.ChannelManager.GetCurrentChannelList();
	if (pChList==nullptr)
		return false;
	const CChannelInfo *pChInfo=pChList->GetChannelInfo(Channel);
	if (pChInfo==nullptr)
		return false;

	if (!m_App.UICore.ConfirmChannelChange())
		return false;

	return SetChannel(m_App.ChannelManager.GetCurrentSpace(),Channel);
}


bool CAppCore::SwitchChannelByNo(int ChannelNo,bool fSwitchService)
{
	if (ChannelNo<1)
		return false;

	const CChannelList *pList=m_App.ChannelManager.GetCurrentChannelList();
	if (pList==nullptr)
		return false;

	int Index;

	if (pList->HasRemoteControlKeyID()) {
		Index=pList->FindChannelNo(ChannelNo);

		if (fSwitchService) {
			const CChannelInfo *pCurChInfo=m_App.ChannelManager.GetCurrentChannelInfo();

			if (pCurChInfo!=nullptr && pCurChInfo->GetChannelNo()==ChannelNo) {
				const int NumChannels=pList->NumChannels();

				for (int i=m_App.ChannelManager.GetCurrentChannel()+1;i<NumChannels;i++) {
					const CChannelInfo *pChInfo=pList->GetChannelInfo(i);

					if (pChInfo->IsEnabled() && pChInfo->GetChannelNo()==ChannelNo) {
						Index=i;
						break;
					}
				}
			}
		}

		if (Index<0)
			return false;
	} else {
		Index=ChannelNo-1;
	}

	return SwitchChannel(Index);
}


bool CAppCore::SetCommandLineChannel(const CCommandLineOptions *pCmdLine)
{
	const CChannelList *pChannelList;

	for (int i=0;(pChannelList=m_App.ChannelManager.GetChannelList(i))!=nullptr;i++) {
		if (pCmdLine->m_TuningSpace<0 || i==pCmdLine->m_TuningSpace) {
			for (int j=0;j<pChannelList->NumChannels();j++) {
				const CChannelInfo *pChannelInfo=pChannelList->GetChannelInfo(j);

				if ((pCmdLine->m_Channel==0
						|| pCmdLine->m_Channel==pChannelInfo->GetPhysicalChannel())
					&& (pCmdLine->m_ChannelIndex<0
						|| pCmdLine->m_ChannelIndex==pChannelInfo->GetChannelIndex())
					&& (pCmdLine->m_ControllerChannel==0
						|| pCmdLine->m_ControllerChannel==pChannelInfo->GetChannelNo())
					&& (pCmdLine->m_ServiceID==0
						|| pCmdLine->m_ServiceID==pChannelInfo->GetServiceID())
					&& (pCmdLine->m_NetworkID==0
						|| pCmdLine->m_NetworkID==pChannelInfo->GetNetworkID())
					&& (pCmdLine->m_TransportStreamID==0
						|| pCmdLine->m_TransportStreamID==pChannelInfo->GetTransportStreamID())) {
					return SetChannel(i,j);
				}
			}
		}
	}

	if (pCmdLine->m_ServiceID>0
			&& (pCmdLine->m_Channel>0 || pCmdLine->m_ChannelIndex>=0
				|| pCmdLine->m_ControllerChannel>0
				|| pCmdLine->m_NetworkID>0 || pCmdLine->m_TransportStreamID>0)) {
		for (int i=0;(pChannelList=m_App.ChannelManager.GetChannelList(i))!=nullptr;i++) {
			if (pCmdLine->m_TuningSpace<0 || i==pCmdLine->m_TuningSpace) {
				for (int j=0;j<pChannelList->NumChannels();j++) {
					const CChannelInfo *pChannelInfo=pChannelList->GetChannelInfo(j);

					if ((pCmdLine->m_Channel==0
							|| pCmdLine->m_Channel==pChannelInfo->GetPhysicalChannel())
						&& (pCmdLine->m_ChannelIndex<0
							|| pCmdLine->m_ChannelIndex==pChannelInfo->GetChannelIndex())
						&& (pCmdLine->m_ControllerChannel==0
							|| pCmdLine->m_ControllerChannel==pChannelInfo->GetChannelNo())
						&& (pCmdLine->m_NetworkID==0
							|| pCmdLine->m_NetworkID==pChannelInfo->GetNetworkID())
						&& (pCmdLine->m_TransportStreamID==0
							|| pCmdLine->m_TransportStreamID==pChannelInfo->GetTransportStreamID())) {
						return SetChannel(i,j,pCmdLine->m_ServiceID);
					}
				}
			}
		}
	}

	m_App.AddLog(CLogItem::TYPE_ERROR,TEXT("コマンドラインで指定されたチャンネルが見付かりません。"));

	return false;
}


bool CAppCore::FollowChannelChange(WORD TransportStreamID,WORD ServiceID)
{
	const CChannelList *pChannelList;
	const CChannelInfo *pChannelInfo=nullptr;
	int Space,Channel;

	pChannelList=m_App.ChannelManager.GetCurrentChannelList();
	if (pChannelList!=nullptr) {
		Channel=pChannelList->FindByIDs(0,TransportStreamID,ServiceID);
		if (Channel>=0) {
			pChannelInfo=pChannelList->GetChannelInfo(Channel);
			Space=m_App.ChannelManager.GetCurrentSpace();
		}
	} else {
		for (int i=0;i<m_App.ChannelManager.NumSpaces();i++) {
			pChannelList=m_App.ChannelManager.GetChannelList(i);
			Channel=pChannelList->FindByIDs(0,TransportStreamID,ServiceID);
			if (Channel>=0) {
				pChannelInfo=pChannelList->GetChannelInfo(Channel);
				Space=i;
				break;
			}
		}
	}
	if (pChannelInfo==nullptr)
		return false;

	const CChannelInfo *pCurChInfo=m_App.ChannelManager.GetCurrentChannelInfo();
	if (pCurChInfo==nullptr
			|| pCurChInfo->GetTransportStreamID()!=TransportStreamID) {
		m_App.AddLog(TEXT("ストリームの変化を検知しました。(TSID %d / SID %d)"),
					 TransportStreamID,ServiceID);
	}
	const bool fSpaceChanged=Space!=m_App.ChannelManager.GetCurrentSpace();
	if (!m_App.ChannelManager.SetCurrentChannel(Space,Channel))
		return false;
	m_App.ChannelManager.SetCurrentServiceID(0);
	m_App.AppEventManager.OnChannelChanged(
		AppEvent::CHANNEL_CHANGED_STATUS_DETECTED
		| (fSpaceChanged ? AppEvent::CHANNEL_CHANGED_STATUS_SPACE_CHANGED : 0));
	return true;
}


bool CAppCore::SetServiceByID(WORD ServiceID,unsigned int Flags)
{
	TRACE(TEXT("CAppCore::SetServiceByID(%04x,%x)\n"),ServiceID,Flags);

	const bool fStrict=(Flags & SET_SERVICE_STRICT_ID)!=0;
	const CChannelInfo *pCurChInfo=m_App.ChannelManager.GetCurrentChannelInfo();
	WORD NetworkID=m_App.CoreEngine.m_DtvEngine.m_TsAnalyzer.GetNetworkID();
	if (NetworkID==0 && pCurChInfo!=nullptr && pCurChInfo->GetNetworkID()>0)
		NetworkID=pCurChInfo->GetNetworkID();

	CDtvEngine::ServiceSelectInfo ServiceSel;

	ServiceSel.ServiceID=ServiceID!=0?ServiceID:CDtvEngine::SID_INVALID;
	ServiceSel.bFollowViewableService=!m_App.NetworkDefinition.IsCSNetworkID(NetworkID);

	if (!fStrict && m_f1SegMode) {
		ServiceSel.OneSegSelect=CDtvEngine::ONESEG_SELECT_HIGHPRIORITY;

		if (pCurChInfo!=nullptr) {
			// サブチャンネルの選択の場合、ワンセグもサブチャンネルを優先する
			if (ServiceSel.ServiceID!=CDtvEngine::SID_INVALID) {
				ServiceSel.PreferredServiceIndex=
					(WORD)GetCorresponding1SegService(
						pCurChInfo->GetSpace(),
						pCurChInfo->GetNetworkID(),
						pCurChInfo->GetTransportStreamID(),
						ServiceSel.ServiceID);
			}
		}
	}

	bool fResult;

	if (ServiceSel.ServiceID==CDtvEngine::SID_DEFAULT) {
		m_App.AddLog(TEXT("デフォルトのサービスを選択します..."));
		fResult=m_App.CoreEngine.m_DtvEngine.SetService(&ServiceSel);
		if (fResult) {
			if (!m_App.CoreEngine.m_DtvEngine.GetServiceID(&ServiceID))
				ServiceID=0;
		}
	} else {
		if (ServiceSel.OneSegSelect==CDtvEngine::ONESEG_SELECT_HIGHPRIORITY)
			m_App.AddLog(TEXT("サービスを選択します..."));
		else
			m_App.AddLog(TEXT("サービスを選択します(SID %d)..."),ServiceSel.ServiceID);
		fResult=m_App.CoreEngine.m_DtvEngine.SetService(&ServiceSel);
	}
	if (!fResult) {
		m_App.AddLog(CLogItem::TYPE_ERROR,TEXT("サービスを選択できません。"));
		return false;
	}

	if ((Flags & SET_SERVICE_NO_CHANGE_CUR_SERVICE_ID)==0)
		m_App.ChannelManager.SetCurrentServiceID(ServiceID);

	if (ServiceID!=0) {
		int ServiceIndex=m_App.CoreEngine.m_DtvEngine.m_TsAnalyzer.GetServiceIndexByID(ServiceID);
		if (ServiceIndex>=0) {
			//m_App.AddLog(TEXT("サービスを変更しました。(SID %d)"),ServiceID);

			if (fStrict && m_f1SegMode
					&& !m_App.CoreEngine.m_DtvEngine.m_TsAnalyzer.Is1SegService(ServiceIndex)) {
				Set1SegMode(false,false);
			}
		}
	}

	if (m_f1SegMode && ServiceSel.OneSegSelect!=CDtvEngine::ONESEG_SELECT_HIGHPRIORITY) {
		Set1SegMode(false,false);
	}

	bool fChannelChanged=false;

	if (!m_f1SegMode && ServiceID!=0 && pCurChInfo!=nullptr) {
		const CChannelList *pChList=m_App.ChannelManager.GetCurrentChannelList();
		int Index=pChList->FindByIndex(pCurChInfo->GetSpace(),
									   pCurChInfo->GetChannelIndex(),
									   ServiceID);
		if (Index>=0) {
			m_App.ChannelManager.SetCurrentChannel(m_App.ChannelManager.GetCurrentSpace(),Index);
			m_App.AppEventManager.OnChannelChanged(0);
			fChannelChanged=true;
		}
	}

	if (!fChannelChanged)
		m_App.AppEventManager.OnServiceChanged();

	return true;
}


bool CAppCore::SetServiceByIndex(int Service,unsigned int Flags)
{
	if (Service < 0)
		return false;

	WORD ServiceID;

	if (!m_App.CoreEngine.m_DtvEngine.m_TsAnalyzer.GetViewableServiceID(Service,&ServiceID))
		return false;

	return SetServiceByID(ServiceID,Flags);
}


bool CAppCore::GetCurrentStreamIDInfo(StreamIDInfo *pInfo) const
{
	if (pInfo==nullptr)
		return false;

	pInfo->NetworkID=
		m_App.CoreEngine.m_DtvEngine.m_TsAnalyzer.GetNetworkID();
	pInfo->TransportStreamID=
		m_App.CoreEngine.m_DtvEngine.m_TsAnalyzer.GetTransportStreamID();
	WORD ServiceID;
	if (!m_App.CoreEngine.m_DtvEngine.GetServiceID(&ServiceID)) {
		int CurServiceID=m_App.ChannelManager.GetCurrentServiceID();
		if (CurServiceID>0)
			ServiceID=(WORD)CurServiceID;
		else
			ServiceID=0;
	}
	pInfo->ServiceID=ServiceID;

	return true;
}


bool CAppCore::GetCurrentStreamChannelInfo(CChannelInfo *pInfo) const
{
	if (pInfo==nullptr)
		return false;

	StreamIDInfo IDInfo;
	if (!GetCurrentStreamIDInfo(&IDInfo))
		return false;

	const CChannelInfo *pCurChInfo=m_App.ChannelManager.GetCurrentChannelInfo();
	if (pCurChInfo!=nullptr
			&& pCurChInfo->GetNetworkID()==IDInfo.NetworkID
			&& pCurChInfo->GetTransportStreamID()==IDInfo.TransportStreamID
			&& pCurChInfo->GetServiceID()==IDInfo.ServiceID) {
		*pInfo=*pCurChInfo;
	} else {
		CChannelInfo ChInfo;
		ChInfo.SetNetworkID(IDInfo.NetworkID);
		ChInfo.SetTransportStreamID(IDInfo.TransportStreamID);
		ChInfo.SetServiceID(IDInfo.ServiceID);
		*pInfo=ChInfo;
	}

	return true;
}


bool CAppCore::GetCurrentServiceName(LPTSTR pszName,int MaxLength,bool fUseChannelName)
{
	if (pszName==nullptr || MaxLength<1)
		return false;

	WORD ServiceID;
	if (!m_App.CoreEngine.m_DtvEngine.GetServiceID(&ServiceID))
		ServiceID=0;

	const CChannelInfo *pChannelInfo=nullptr;
	if (fUseChannelName) {
		pChannelInfo=m_App.ChannelManager.GetCurrentChannelInfo();
		if (pChannelInfo!=nullptr) {
			if (ServiceID==0 || pChannelInfo->GetServiceID()<=0
					|| pChannelInfo->GetServiceID()==ServiceID) {
				::lstrcpyn(pszName,pChannelInfo->GetName(),MaxLength);
				return true;
			}
		}
	}

	pszName[0]=_T('\0');

	if (ServiceID==0)
		return false;

	int Index=m_App.CoreEngine.m_DtvEngine.m_TsAnalyzer.GetServiceIndexByID(ServiceID);
	if (Index<0)
		return false;
#if 0
	if (pChannelInfo!=nullptr) {
		int Length=StdUtil::snprintf(pszName,MaxLength,TEXT("#%d "),Index+1);
		pszName+=Length;
		MaxLength-=Length;
	}
#endif
	if (m_App.CoreEngine.m_DtvEngine.m_TsAnalyzer.GetServiceName(Index,pszName,MaxLength)<1)
		return false;

	return true;
}


bool CAppCore::OpenTuner(LPCTSTR pszFileName)
{
	if (IsStringEmpty(pszFileName))
		return false;
	if (m_App.CoreEngine.IsTunerOpen()
			&& IsEqualFileName(m_App.CoreEngine.GetDriverFileName(),pszFileName))
		return true;

	TRACE(TEXT("CAppCore::OpenTuner(%s)\n"),pszFileName);

	HCURSOR hcurOld=::SetCursor(::LoadCursor(nullptr,IDC_WAIT));
	bool fOK;

	SaveCurrentChannel();

	m_App.UICore.SetStatusBarTrace(true);

	if (m_App.CoreEngine.IsTunerOpen()) {
		m_App.CoreEngine.CloseTuner();
	}

	m_App.TSProcessorManager.OnTunerChange(m_App.CoreEngine.GetDriverFileName(),pszFileName);

	m_App.CoreEngine.SetDriverFileName(pszFileName);
	fOK=OpenAndInitializeTuner();
	if (fOK) {
		InitializeChannel();
		::SetCursor(hcurOld);
	} else {
		::SetCursor(hcurOld);
		OnError(&m_App.CoreEngine,TEXT("BonDriverの初期化ができません。"));
	}

	m_App.UICore.SetStatusBarTrace(false);
	m_App.AppEventManager.OnTunerChanged();

	return fOK;
}


bool CAppCore::OpenTuner()
{
	TRACE(TEXT("CAppCore::OpenTuner()\n"));

	if (!m_App.CoreEngine.IsDriverSpecified())
		return false;

	m_App.UICore.SetStatusBarTrace(true);
	bool fOK=true;

	if (!m_App.CoreEngine.IsTunerOpen()) {
		if (OpenAndInitializeTuner(OPENTUNER_NO_UI)) {
			m_App.AppEventManager.OnTunerOpened();
		} else {
			OnError(&m_App.CoreEngine,TEXT("BonDriverの初期化ができません。"));
			fOK=false;
		}
	}

	m_App.UICore.SetStatusBarTrace(false);

	return fOK;
}


bool CAppCore::OpenAndInitializeTuner(unsigned int OpenFlags)
{
	CDriverOptions::BonDriverOptions Options(m_App.CoreEngine.GetDriverFileName());
	m_App.DriverOptions.GetBonDriverOptions(m_App.CoreEngine.GetDriverFileName(),&Options);

	m_App.CoreEngine.m_DtvEngine.SetStartStreamingOnDriverOpen(!Options.fIgnoreInitialStream);

	if (!m_App.CoreEngine.OpenTuner())
		return false;

	m_App.AddLog(TEXT("%s を読み込みました。"),m_App.CoreEngine.GetDriverFileName());

	ApplyBonDriverOptions();

	unsigned int DecoderOpenFlags=0;
	if ((OpenFlags & OPENTUNER_NO_NOTIFY)==0)
		DecoderOpenFlags|=CTSProcessorManager::FILTER_OPEN_NOTIFY_ERROR;
	if (m_fSilent || (OpenFlags & OPENTUNER_NO_UI)!=0)
		DecoderOpenFlags|=CTSProcessorManager::FILTER_OPEN_NO_UI;
	else if ((OpenFlags & OPENTUNER_RETRY_DIALOG)!=0)
		DecoderOpenFlags|=CTSProcessorManager::FILTER_OPEN_RETRY_DIALOG;
	m_App.TSProcessorManager.OnTunerOpened(m_App.CoreEngine.GetDriverFileName(),DecoderOpenFlags);

	return true;
}


bool CAppCore::CloseTuner()
{
	TRACE(TEXT("CAppCore::CloseTuner()\n"));

	m_App.TSProcessorManager.CloseAllFilters();

	if (m_App.CoreEngine.IsTunerOpen()) {
		m_App.CoreEngine.CloseTuner();
		SaveCurrentChannel();
		m_App.ChannelManager.SetCurrentChannel(m_App.ChannelManager.GetCurrentSpace(),-1);
		m_App.AppEventManager.OnTunerClosed();
	}

	return true;
}


void CAppCore::ShutDownTuner()
{
	CloseTuner();
	ResetEngine();

	m_App.ChannelManager.Reset();

	if (m_App.CoreEngine.IsDriverSpecified()) {
		m_App.CoreEngine.SetDriverFileName(NULL);
		m_App.AppEventManager.OnTunerShutDown();
	}
}


void CAppCore::ResetEngine()
{
	m_App.CoreEngine.m_DtvEngine.ResetEngine();
	m_App.AppEventManager.OnEngineReset();
}


bool CAppCore::Set1SegMode(bool f1Seg,bool fServiceChange)
{
	if (m_f1SegMode != f1Seg) {
		m_f1SegMode = f1Seg;

		m_App.AddLog(TEXT("ワンセグモードを%sにします。"),f1Seg?TEXT("オン"):TEXT("オフ"));

		if (fServiceChange) {
			if (m_f1SegMode) {
				CTsAnalyzer &TsAnalyzer=m_App.CoreEngine.m_DtvEngine.m_TsAnalyzer;

				if (TsAnalyzer.Has1SegService()) {
					WORD ServiceID=CDtvEngine::SID_DEFAULT;
					CChannelInfo ChInfo;

					if (GetCurrentStreamChannelInfo(&ChInfo)) {
						int Index=GetCorresponding1SegService(
							ChInfo.GetSpace(),
							ChInfo.GetNetworkID(),
							ChInfo.GetTransportStreamID(),
							ChInfo.GetServiceID());
						WORD SID;
						if (TsAnalyzer.Get1SegServiceIDByIndex(Index,&SID))
							ServiceID=SID;
					}

					SetServiceByID(ServiceID,SET_SERVICE_NO_CHANGE_CUR_SERVICE_ID);
				} else {
					m_App.CoreEngine.m_DtvEngine.SetOneSegSelectType(
						CDtvEngine::ONESEG_SELECT_HIGHPRIORITY);
				}
			} else {
				SetServiceByID(
					m_App.ChannelManager.GetCurrentServiceID()>0?
						m_App.ChannelManager.GetCurrentServiceID():CDtvEngine::SID_DEFAULT,
					SET_SERVICE_NO_CHANGE_CUR_SERVICE_ID);
			}
		}

		m_App.AppEventManager.On1SegModeChanged(m_f1SegMode);
	}

	return true;
}


int CAppCore::GetCorresponding1SegService(
	int Space,WORD NetworkID,WORD TSID,WORD ServiceID) const
{
	if (ServiceID==0)
		return 0;

	int ServiceIndex=0;

	for (WORD SID=ServiceID-1;SID>0;SID--) {
		if (m_App.ChannelManager.FindChannelByIDs(Space,NetworkID,TSID,SID,false)<0)
			break;
		ServiceIndex=ServiceID-SID;
	}

	return ServiceIndex;
}


void CAppCore::ApplyBonDriverOptions()
{
	if (!m_App.CoreEngine.IsTunerOpen())
		return;

	CDriverOptions::BonDriverOptions Options(m_App.CoreEngine.GetDriverFileName());
	m_App.DriverOptions.GetBonDriverOptions(m_App.CoreEngine.GetDriverFileName(),&Options);

	//m_App.CoreEngine.m_DtvEngine.SetStartStreamingOnDriverOpen(!Options.fIgnoreInitialStream);

	CBonSrcDecoder &BonSrcDecoder=m_App.CoreEngine.m_DtvEngine.m_BonSrcDecoder;

	BonSrcDecoder.SetPurgeStreamOnChannelChange(Options.fPurgeStreamOnChannelChange);
	BonSrcDecoder.SetFirstChannelSetDelay(Options.FirstChannelSetDelay);
	BonSrcDecoder.SetMinChannelChangeInterval(Options.MinChannelChangeInterval);

	m_App.CoreEngine.m_DtvEngine.m_MediaViewer.SetPacketInputWait(
		Options.fPumpStreamSyncPlayback?3000:0);
}


bool CAppCore::SaveCurrentChannel()
{
	if (!IsStringEmpty(m_App.CoreEngine.GetDriverFileName())) {
		const CChannelInfo *pInfo=m_App.ChannelManager.GetCurrentChannelInfo();

		if (pInfo!=nullptr) {
			CDriverOptions::ChannelInfo ChInfo;

			ChInfo.Space=pInfo->GetSpace();
			ChInfo.Channel=pInfo->GetChannelIndex();
			ChInfo.ServiceID=pInfo->GetServiceID();
			ChInfo.TransportStreamID=pInfo->GetTransportStreamID();
			ChInfo.fAllChannels=m_App.ChannelManager.GetCurrentSpace()==CChannelManager::SPACE_ALL;
			m_App.DriverOptions.SetLastChannel(m_App.CoreEngine.GetDriverFileName(),&ChInfo);
		}
	}
	return true;
}


bool CAppCore::GenerateRecordFileName(LPTSTR pszFileName,int MaxFileName)
{
	CRecordManager::FileNameFormatInfo FormatInfo;

	GetVariableStringEventInfo(&FormatInfo,60*1000);

	TVTest::String Path;
	if (!m_App.RecordManager.GenerateFilePath(FormatInfo,NULL,&Path)) {
		OnError(TEXT("録画ファイルのパスを作成できません。"));
		return false;
	}
	if (!GetAbsolutePath(Path.c_str(),pszFileName,MaxFileName)) {
		OnError(TEXT("録画ファイルの保存先フォルダの指定が正しくないか、パスが長過ぎます。"));
		return false;
	}

	TCHAR szDir[MAX_PATH];
	::lstrcpy(szDir,pszFileName);
	::PathRemoveFileSpec(szDir);
	if (!::PathIsDirectory(szDir)) {
		int Result=::SHCreateDirectoryEx(NULL,szDir,NULL);
		if (Result!=ERROR_SUCCESS && Result!=ERROR_ALREADY_EXISTS) {
			OnError(TEXT("録画ファイルの保存先フォルダ \"%s\" を作成できません。"),szDir);
			return false;
		}
	}

	return true;
}


bool CAppCore::StartRecord(LPCTSTR pszFileName,
						   const CRecordManager::TimeSpecInfo *pStartTime,
						   const CRecordManager::TimeSpecInfo *pStopTime,
						   CRecordManager::RecordClient Client,
						   bool fTimeShift)
{
	if (m_App.RecordManager.IsRecording())
		return false;
	m_App.RecordManager.SetFileName(pszFileName);
	m_App.RecordManager.SetStartTimeSpec(pStartTime);
	m_App.RecordManager.SetStopTimeSpec(pStopTime);
	m_App.RecordManager.SetStopOnEventEnd(false);
	m_App.RecordManager.SetClient(Client);
	m_App.RecordOptions.GetRecordingSettings(&m_App.RecordManager.GetRecordingSettings());
	if (m_App.CmdLineOptions.m_fRecordCurServiceOnly)
		m_App.RecordManager.SetCurServiceOnly(true);
	if (m_App.RecordManager.IsReserved()) {
		m_App.StatusView.UpdateItem(STATUS_ITEM_RECORD);
		return true;
	}

	OpenTuner();

	TCHAR szFileName[MAX_PATH*2];
	if (IsStringEmpty(pszFileName)) {
		LPCTSTR pszErrorMessage;

		if (!m_App.RecordOptions.GenerateFilePath(szFileName,lengthof(szFileName),
												  &pszErrorMessage)) {
			m_App.UICore.GetSkin()->ShowErrorMessage(pszErrorMessage);
			return false;
		}
		m_App.RecordManager.SetFileName(szFileName);
	}
	if (!GenerateRecordFileName(szFileName,lengthof(szFileName)))
		return false;
	AppEvent::RecordingStartInfo RecStartInfo;
	RecStartInfo.pRecordManager=&m_App.RecordManager;
	RecStartInfo.pszFileName=szFileName;
	RecStartInfo.MaxFileName=lengthof(szFileName);
	m_App.AppEventManager.OnRecordingStart(&RecStartInfo);
	m_App.CoreEngine.ResetErrorCount();
	if (!m_App.RecordManager.StartRecord(&m_App.CoreEngine.m_DtvEngine,szFileName,fTimeShift)) {
		OnError(&m_App.RecordManager,TEXT("録画を開始できません。"));
		return false;
	}
	m_App.TaskTrayManager.SetStatus(CTaskTrayManager::STATUS_RECORDING,
									CTaskTrayManager::STATUS_RECORDING);
	m_App.AddLog(TEXT("録画開始 %s"),szFileName);
	m_App.AppEventManager.OnRecordingStarted();
	return true;
}


bool CAppCore::ModifyRecord(LPCTSTR pszFileName,
							const CRecordManager::TimeSpecInfo *pStartTime,
							const CRecordManager::TimeSpecInfo *pStopTime,
							CRecordManager::RecordClient Client)
{
	m_App.RecordManager.SetFileName(pszFileName);
	m_App.RecordManager.SetStartTimeSpec(pStartTime);
	m_App.RecordManager.SetStopTimeSpec(pStopTime);
	m_App.RecordManager.SetClient(Client);
	m_App.StatusView.UpdateItem(STATUS_ITEM_RECORD);
	return true;
}


bool CAppCore::StartReservedRecord()
{
	TCHAR szFileName[MAX_PATH];

	/*
	if (!m_App.RecordManager.IsReserved())
		return false;
	*/
	if (!IsStringEmpty(m_App.RecordManager.GetFileName())) {
		if (!GenerateRecordFileName(szFileName,MAX_PATH))
			return false;
		/*
		if (!m_App.RecordManager.DoFileExistsOperation(m_App.UICore.GetDialogOwner(),szFileName))
			return false;
		*/
	} else {
		LPCTSTR pszErrorMessage;

		if (!m_App.RecordOptions.GenerateFilePath(szFileName,lengthof(szFileName),
												  &pszErrorMessage)) {
			m_App.UICore.GetSkin()->ShowErrorMessage(pszErrorMessage);
			return false;
		}
		m_App.RecordManager.SetFileName(szFileName);
		if (!GenerateRecordFileName(szFileName,MAX_PATH))
			return false;
	}
	OpenTuner();
	AppEvent::RecordingStartInfo RecStartInfo;
	RecStartInfo.pRecordManager=&m_App.RecordManager;
	RecStartInfo.pszFileName=szFileName;
	RecStartInfo.MaxFileName=lengthof(szFileName);
	m_App.AppEventManager.OnRecordingStart(&RecStartInfo);
	m_App.CoreEngine.ResetErrorCount();
	if (!m_App.RecordManager.StartRecord(&m_App.CoreEngine.m_DtvEngine,szFileName)) {
		m_App.RecordManager.CancelReserve();
		OnError(&m_App.RecordManager,TEXT("録画を開始できません。"));
		return false;
	}
	m_App.RecordManager.GetRecordTask()->GetFileName(szFileName,lengthof(szFileName));
	m_App.AddLog(TEXT("録画開始 %s"),szFileName);

	m_App.TaskTrayManager.SetStatus(CTaskTrayManager::STATUS_RECORDING,
									CTaskTrayManager::STATUS_RECORDING);
	m_App.AppEventManager.OnRecordingStarted();

	return true;
}


bool CAppCore::CancelReservedRecord()
{
	if (!m_App.RecordManager.CancelReserve())
		return false;
	m_App.StatusView.UpdateItem(STATUS_ITEM_RECORD);
	return true;
}


bool CAppCore::StopRecord()
{
	if (!m_App.RecordManager.IsRecording())
		return false;

	TCHAR szFileName[MAX_PATH];
	m_App.RecordManager.GetRecordTask()->GetFileName(szFileName,lengthof(szFileName));

	m_App.RecordManager.StopRecord();
	m_App.RecordOptions.Apply(CRecordOptions::UPDATE_RECORDSTREAM);

	CTsRecorder::WriteStatistics Stats;
	m_App.CoreEngine.m_DtvEngine.m_TsRecorder.GetWriteStatistics(&Stats);
	m_App.AddLog(TEXT("録画停止 %s (出力TSサイズ %llu Bytes / 書き出しエラー回数 %u)"),
				 szFileName,Stats.OutputSize,Stats.WriteErrorCount);

	m_App.TaskTrayManager.SetStatus(0,CTaskTrayManager::STATUS_RECORDING);
	m_App.AppEventManager.OnRecordingStopped();
	if (m_fExitOnRecordingStop)
		m_App.Exit();

	return true;
}


bool CAppCore::PauseResumeRecording()
{
	if (!m_App.RecordManager.IsRecording())
		return false;
	if (!m_App.RecordManager.PauseRecord())
		return false;

	if (m_App.RecordManager.IsPaused()) {
		m_App.AddLog(TEXT("録画一時停止"));
		m_App.AppEventManager.OnRecordingPaused();
	} else {
		m_App.AddLog(TEXT("録画再開"));
		m_App.AppEventManager.OnRecordingResumed();
	}

	return true;
}


bool CAppCore::RelayRecord(LPCTSTR pszFileName)
{
	if (IsStringEmpty(pszFileName) || !m_App.RecordManager.IsRecording())
		return false;
	if (!m_App.RecordManager.RelayFile(pszFileName)) {
		OnError(&m_App.RecordManager,TEXT("録画ファイルを切り替えできません。"));
		return false;
	}
	m_App.AddLog(TEXT("録画ファイルを切り替えました %s"),pszFileName);
	m_App.AppEventManager.OnRecordingFileChanged(pszFileName);
	return true;
}


bool CAppCore::CommandLineRecord(const CCommandLineOptions *pCmdLine)
{
	if (pCmdLine==nullptr)
		return false;
	return CommandLineRecord(pCmdLine->m_RecordFileName.c_str(),
							 &pCmdLine->m_RecordStartTime,
							 pCmdLine->m_RecordDelay,
							 pCmdLine->m_RecordDuration);

}


bool CAppCore::CommandLineRecord(LPCTSTR pszFileName,const FILETIME *pStartTime,int Delay,int Duration)
{
	CRecordManager::TimeSpecInfo StartTime,StopTime;

	if (pStartTime!=nullptr && (pStartTime->dwLowDateTime!=0 || pStartTime->dwHighDateTime!=0)) {
		StartTime.Type=CRecordManager::TIME_DATETIME;
		StartTime.Time.DateTime=*pStartTime;
		if (Delay!=0)
			StartTime.Time.DateTime+=(LONGLONG)Delay*FILETIME_SECOND;
		SYSTEMTIME st;
		::FileTimeToSystemTime(pStartTime,&st);
		m_App.AddLog(TEXT("コマンドラインから録画指定されました。(%d/%d/%d %d:%02d:%02d 開始)"),
					 st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
	} else if (Delay>0) {
		StartTime.Type=CRecordManager::TIME_DURATION;
		StartTime.Time.Duration=Delay*1000;
		m_App.AddLog(TEXT("コマンドラインから録画指定されました。(%d 秒後開始)"),Delay);
	} else {
		StartTime.Type=CRecordManager::TIME_NOTSPECIFIED;
		m_App.AddLog(TEXT("コマンドラインから録画指定されました。"));
	}
	if (Duration>0) {
		StopTime.Type=CRecordManager::TIME_DURATION;
		StopTime.Time.Duration=Duration*1000;
	} else {
		StopTime.Type=CRecordManager::TIME_NOTSPECIFIED;
	}

	return StartRecord(
		IsStringEmpty(pszFileName)?nullptr:pszFileName,
		&StartTime,&StopTime,
		CRecordManager::CLIENT_COMMANDLINE);
}


LPCTSTR CAppCore::GetDefaultRecordFolder() const
{
	return m_App.RecordOptions.GetSaveFolder();
}


void CAppCore::BeginChannelScan(int Space)
{
	m_App.ChannelManager.SetCurrentChannel(Space,-1);
	m_App.AppEventManager.OnChannelChanged(AppEvent::CHANNEL_CHANGED_STATUS_SPACE_CHANGED);
}


bool CAppCore::IsChannelScanning() const
{
	return m_App.ChannelScan.IsScanning();
}


bool CAppCore::IsDriverNoSignalLevel(LPCTSTR pszFileName) const
{
	return m_App.DriverOptions.IsNoSignalLevel(pszFileName);
}


bool CAppCore::GetVariableStringEventInfo(
	TVTest::CEventVariableStringMap::EventInfo *pInfo,DWORD NextEventMargin) const
{
	if (pInfo==nullptr)
		return false;

	const CChannelInfo *pChannelInfo=m_App.ChannelManager.GetCurrentChannelInfo();
	bool fEventInfoValid=false;

	if (pChannelInfo!=nullptr) {
		pInfo->Channel=*pChannelInfo;
		if (pInfo->Channel.GetChannelNo()==0
				&& pInfo->Channel.GetServiceID()>0)
			pInfo->Channel.SetChannelNo(pInfo->Channel.GetServiceID());
	}

	SYSTEMTIME stCur;
	if (!m_App.CoreEngine.m_DtvEngine.m_TsAnalyzer.GetTotTime(&stCur))
		GetCurrentEpgTime(&stCur);
	pInfo->TotTime=stCur;

	WORD ServiceID;
	if (m_App.CoreEngine.m_DtvEngine.GetServiceID(&ServiceID)) {
		int Index=m_App.CoreEngine.m_DtvEngine.m_TsAnalyzer.GetServiceIndexByID(ServiceID);
		TCHAR szServiceName[32];
		if (m_App.CoreEngine.m_DtvEngine.m_TsAnalyzer.GetServiceName(Index,szServiceName,lengthof(szServiceName)))
			pInfo->ServiceName=szServiceName;
		bool fNext=false;
		if (NextEventMargin>0) {
			SYSTEMTIME stStart;
			if (m_App.CoreEngine.m_DtvEngine.GetEventTime(&stStart,nullptr,true)) {
				LONGLONG Diff=DiffSystemTime(&stCur,&stStart);
				if (Diff>=0 && Diff<(LONGLONG)NextEventMargin)
					fNext=true;
			}
		}
		if (m_App.CoreEngine.m_DtvEngine.GetEventInfo(&pInfo->Event,fNext))
			fEventInfoValid=true;
		pInfo->Event.m_ServiceID=ServiceID;
	} else {
		pInfo->Event.m_ServiceID=0;
	}
	if (!fEventInfoValid) {
		pInfo->Event.m_NetworkID=0;
		pInfo->Event.m_TransportStreamID=0;
		pInfo->Event.m_EventID=0;
		pInfo->Event.m_bValidStartTime=false;
		pInfo->Event.m_Duration=0;
	}

	return true;
}
