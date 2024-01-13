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
#include "AppCore.h"
#include "AppMain.h"
#include "AppUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CAppCore::CAppCore(CAppMain &App)
	: m_App(App)
{
}


void CAppCore::OnErrorV(StringView Format, FormatArgs Args)
{
	TCHAR szText[1024];

	StringVFormatArgs(szText, std::size(szText), Format, Args);
	m_App.AddLogRaw(CLogItem::LogType::Error, szText);
	if (!m_fSilent)
		m_App.UICore.GetSkin()->ShowErrorMessage(szText);
}


void CAppCore::OnError(const LibISDB::ErrorHandler *pErrorHandler, LPCTSTR pszTitle)
{
	if (pErrorHandler == nullptr)
		return;

	if (!IsStringEmpty(pErrorHandler->GetLastErrorText())) {
		m_App.AddLogRaw(CLogItem::LogType::Error, pErrorHandler->GetLastErrorText());
	} else if (pErrorHandler->GetLastErrorCode()) {
		std::string Message = pErrorHandler->GetLastErrorCode().message();
		if (!Message.empty()) {
			const int Length = ::MultiByteToWideChar(CP_ACP, 0, Message.data(), static_cast<int>(Message.length()), nullptr, 0);
			String Text(Length, TEXT('\0'));
			::MultiByteToWideChar(CP_ACP, 0, Message.data(), static_cast<int>(Message.length()), &Text[0], Length);
			m_App.AddLogRaw(CLogItem::LogType::Error, Text);
		}
	} else {
		m_App.AddLog(CLogItem::LogType::Error, TEXT("Unknown error"));
	}

	if (!m_fSilent)
		m_App.UICore.GetSkin()->ShowErrorMessage(pErrorHandler, pszTitle);
}


void CAppCore::SetSilent(bool fSilent)
{
	m_fSilent = fSilent;
#ifndef _DEBUG
	m_App.DebugHelper.SetExceptionFilterMode(
		fSilent ? CDebugHelper::ExceptionFilterMode::None : CDebugHelper::ExceptionFilterMode::Dialog);
#endif
}


bool CAppCore::InitializeChannel()
{
	const bool fNetworkDriver = m_App.CoreEngine.IsNetworkDriver();
	String ChannelFilePath;

	m_App.ChannelManager.Reset();
	m_App.ChannelManager.MakeDriverTuningSpaceList(m_App.CoreEngine.GetFilter<LibISDB::BonDriverSourceFilter>());

	if (!fNetworkDriver) {
		GetChannelFileName(m_App.CoreEngine.GetDriverFileName(), &ChannelFilePath);
	}

	if (!ChannelFilePath.empty()) {
		if (m_App.ChannelManager.LoadChannelList(ChannelFilePath.c_str())) {
			m_App.AddLog(
				TEXT("チャンネル設定を \"{}\" から読み込みました。"),
				ChannelFilePath);
			if (!m_App.ChannelManager.ChannelFileHasStreamIDs())
				m_App.AddLog(CLogItem::LogType::Warning, TEXT("チャンネルファイルが古いので再スキャンをお薦めします。"));
		}
	}

	CDriverOptions::ChannelInfo InitChInfo;
	if (m_App.DriverOptions.GetInitialChannel(m_App.CoreEngine.GetDriverFileName(), &InitChInfo)) {
		m_App.RestoreChannelInfo.Space = InitChInfo.Space;
		m_App.RestoreChannelInfo.Channel = InitChInfo.Channel;
		m_App.RestoreChannelInfo.ServiceID = InitChInfo.ServiceID;
		m_App.RestoreChannelInfo.TransportStreamID = InitChInfo.TransportStreamID;
		m_App.RestoreChannelInfo.fAllChannels = InitChInfo.fAllChannels;
	} else {
		m_App.RestoreChannelInfo.Space = -1;
		m_App.RestoreChannelInfo.Channel = -1;
		m_App.RestoreChannelInfo.ServiceID = -1;
		m_App.RestoreChannelInfo.TransportStreamID = -1;
		m_App.RestoreChannelInfo.fAllChannels = false;
	}

	m_App.ChannelManager.SetUseDriverChannelList(fNetworkDriver);
	m_App.ChannelManager.SetCurrentChannel(
		m_App.RestoreChannelInfo.fAllChannels ? CChannelManager::SPACE_ALL : std::max(m_App.RestoreChannelInfo.Space, 0),
		-1);
	m_App.ChannelManager.SetCurrentServiceID(0);
	m_App.AppEventManager.OnChannelListChanged();
	m_App.ChannelScan.SetTuningSpaceList(m_App.ChannelManager.GetTuningSpaceList());
	return true;
}


bool CAppCore::GetChannelFileName(LPCTSTR pszDriverFileName, String *pChannelFileName)
{
	if (pChannelFileName == nullptr)
		return false;
	pChannelFileName->clear();
	if (IsStringEmpty(pszDriverFileName))
		return false;

	const bool fRelative = ::PathIsRelative(pszDriverFileName) != FALSE;
	CFilePath Path, Path2, Dir;

	if (fRelative) {
		if (!m_App.CoreEngine.GetDriverDirectoryPath(&Dir))
			return false;
		Path = Dir;
		Path.Append(pszDriverFileName);
	} else {
		Path = pszDriverFileName;
	}
	Path.RenameExtension(CHANNEL_FILE_EXTENSION);

#ifdef DEFERRED_CHANNEL_FILE_EXTENSION
	if (!Path.IsFileExists()) {
		Path2 = Path;
		Path2.RenameExtension(DEFERRED_CHANNEL_FILE_EXTENSION);
		if (Path2.IsFileExists())
			Path = Path2;
	}
#endif

	if (fRelative && !Path.IsFileExists()) {
		m_App.GetAppDirectory(&Dir);
		Path2 = Dir;
		Path2.Append(pszDriverFileName);
		Path2.RenameExtension(CHANNEL_FILE_EXTENSION);
		if (Path2.IsFileExists()) {
			Path = Path2;
		}
#ifdef DEFERRED_CHANNEL_FILE_EXTENSION
		else {
			Path2.RenameExtension(DEFERRED_CHANNEL_FILE_EXTENSION);
			if (Path2.IsFileExists())
				Path = Path2;
#endif
		}
	}

	*pChannelFileName = Path;

	return true;
}


bool CAppCore::RestoreChannel()
{
	if (m_App.RestoreChannelInfo.Space >= 0 && m_App.RestoreChannelInfo.Channel >= 0) {
		const int Space = m_App.RestoreChannelInfo.fAllChannels ? CChannelManager::SPACE_ALL : m_App.RestoreChannelInfo.Space;
		const CChannelList *pList = m_App.ChannelManager.GetChannelList(Space);
		if (pList != nullptr) {
			int Index = pList->FindByIndex(
				m_App.RestoreChannelInfo.Space,
				m_App.RestoreChannelInfo.Channel,
				m_App.RestoreChannelInfo.ServiceID);
			if (Index < 0) {
				if (m_App.RestoreChannelInfo.TransportStreamID > 0 && m_App.RestoreChannelInfo.ServiceID > 0) {
					Index = pList->FindByIDs(
						0,
						static_cast<WORD>(m_App.RestoreChannelInfo.TransportStreamID),
						static_cast<WORD>(m_App.RestoreChannelInfo.ServiceID));
				}
				if (Index < 0 && m_App.RestoreChannelInfo.ServiceID > 0) {
					Index = pList->FindByIndex(
						m_App.RestoreChannelInfo.Space,
						m_App.RestoreChannelInfo.Channel);
				}
			}
			if (Index >= 0)
				return SetChannel(Space, Index);
		}
	}
	return false;
}


bool CAppCore::UpdateCurrentChannelList(const CTuningSpaceList *pList)
{
	const bool fNetworkDriver = m_App.CoreEngine.IsNetworkDriver();

	m_App.ChannelManager.SetTuningSpaceList(pList);
	m_App.ChannelManager.SetUseDriverChannelList(fNetworkDriver);
	/*
	m_App.ChannelManager.SetCurrentChannel(
		(!fNetworkDriver && m_App.ChannelManager.GetAllChannelList()->NumChannels() > 0) ?
			CChannelManager::SPACE_ALL : 0,
		m_App.CoreEngine.IsUDPDriver() ? 0 : -1);
	*/
	int Space = -1;
	bool fAllChannels = false;
	for (int i = 0; i < pList->NumSpaces(); i++) {
		if (pList->GetTuningSpaceType(i) != CTuningSpaceInfo::TuningSpaceType::Terrestrial) {
			fAllChannels = false;
			break;
		}
		const CChannelList *pChannelList = pList->GetChannelList(i);
		if (pChannelList != nullptr && pChannelList->NumChannels() > 0) {
			if (Space >= 0)
				fAllChannels = true;
			else
				Space = i;
		}
	}
	m_App.ChannelManager.SetCurrentChannel(
		fAllChannels ? CChannelManager::SPACE_ALL : (Space >= 0 ? Space : 0),
		-1);
	m_App.ChannelManager.SetCurrentServiceID(0);
	const WORD ServiceID = m_App.CoreEngine.GetServiceID();
	if (ServiceID != LibISDB::SERVICE_ID_INVALID)
		FollowChannelChange(m_App.CoreEngine.GetTransportStreamID(), ServiceID);

	m_App.AppEventManager.OnChannelListChanged();

	UpdateChannelList(m_App.CoreEngine.GetDriverFileName(), pList);

	return true;
}


bool CAppCore::UpdateChannelList(LPCTSTR pszBonDriverName, const CTuningSpaceList *pList)
{
	if (IsStringEmpty(pszBonDriverName) || pList == nullptr)
		return false;

	const int Index = m_App.DriverManager.FindByFileName(::PathFindFileName(pszBonDriverName));
	if (Index >= 0) {
		CDriverInfo *pDriverInfo = m_App.DriverManager.GetDriverInfo(Index);
		if (pDriverInfo != nullptr) {
			pDriverInfo->ClearTuningSpaceList();
		}
	}

	// お気に入りチャンネルの更新
	class CFavoritesChannelUpdator
		: public CFavoriteItemEnumerator
	{
		LPCTSTR m_pszBonDriver;
		const CTuningSpaceList *m_pTuningSpaceList;
		bool m_fUpdated = false;

		bool ChannelItem(CFavoriteFolder &Folder, CFavoriteChannel &Channel) override
		{
			if (IsEqualFileName(Channel.GetBonDriverFileName(), m_pszBonDriver)) {
				const CChannelInfo &ChannelInfo = Channel.GetChannelInfo();
				const CChannelList *pChannelList = m_pTuningSpaceList->GetChannelList(ChannelInfo.GetSpace());

				if (pChannelList != nullptr) {
					if (pChannelList->FindByIDs(
								ChannelInfo.GetNetworkID(),
								ChannelInfo.GetTransportStreamID(),
								ChannelInfo.GetServiceID(),
								false) < 0) {
						const int ChannelCount = pChannelList->NumChannels();
						const CNetworkDefinition &NetworkDefinition = GetAppClass().NetworkDefinition;
						const CNetworkDefinition::NetworkType ChannelNetworkType =
							NetworkDefinition.GetNetworkType(ChannelInfo.GetNetworkID());

						for (int i = 0; i < ChannelCount; i++) {
							const CChannelInfo *pChInfo = pChannelList->GetChannelInfo(i);

							if (NetworkDefinition.GetNetworkType(pChInfo->GetNetworkID()) == ChannelNetworkType
									&& (pChInfo->GetServiceID() == ChannelInfo.GetServiceID()
										|| ::lstrcmp(pChInfo->GetName(), ChannelInfo.GetName()) == 0)) {
								TRACE(
									TEXT("お気に入りチャンネル更新 : {} -> {} / NID {} -> {} / TSID {:04x} -> {:04x} / SID {} -> {}\n"),
									ChannelInfo.GetName(), pChInfo->GetName(),
									ChannelInfo.GetNetworkID(), pChInfo->GetNetworkID(),
									ChannelInfo.GetTransportStreamID(), pChInfo->GetTransportStreamID(),
									ChannelInfo.GetServiceID(), pChInfo->GetServiceID());
								Channel.SetChannelInfo(*pChInfo);
								m_fUpdated = true;
								break;
							}
						}
					}
				}
			}

			return true;
		}

	public:
		CFavoritesChannelUpdator(LPCTSTR pszBonDriver, const CTuningSpaceList *pTuningSpaceList)
			: m_pszBonDriver(pszBonDriver)
			, m_pTuningSpaceList(pTuningSpaceList)
		{
		}

		bool IsUpdated() const { return m_fUpdated; }
	};

	CFavoritesChannelUpdator FavoritesUpdator(pszBonDriverName, pList);
	FavoritesUpdator.EnumItems(m_App.FavoritesManager.GetRootFolder());
	if (FavoritesUpdator.IsUpdated())
		m_App.FavoritesManager.SetModified(true);

	return true;
}


const CChannelInfo *CAppCore::GetCurrentChannelInfo() const
{
	return m_App.ChannelManager.GetCurrentChannelInfo();
}


bool CAppCore::SetChannel(int Space, int Channel, int ServiceID/* = -1*/, bool fStrictService/* = false*/)
{
	const CChannelInfo *pPrevChInfo = m_App.ChannelManager.GetCurrentChannelInfo();
	const int OldSpace = m_App.ChannelManager.GetCurrentSpace(), OldChannel = m_App.ChannelManager.GetCurrentChannel();

	if (!m_App.ChannelManager.SetCurrentChannel(Space, Channel))
		return false;
	const CChannelInfo *pChInfo = m_App.ChannelManager.GetCurrentChannelInfo();
	if (pChInfo == nullptr) {
		m_App.ChannelManager.SetCurrentChannel(OldSpace, OldChannel);
		return false;
	}
	if (pPrevChInfo == nullptr
			|| pChInfo->GetSpace() != pPrevChInfo->GetSpace()
			|| pChInfo->GetChannelIndex() != pPrevChInfo->GetChannelIndex()) {
		if (ServiceID > 0) {
			const CChannelList *pChList = m_App.ChannelManager.GetCurrentChannelList();
			const int Index = pChList->FindByIndex(pChInfo->GetSpace(), pChInfo->GetChannelIndex(), ServiceID);
			if (Index >= 0) {
				m_App.ChannelManager.SetCurrentChannel(Space, Index);
				pChInfo = pChList->GetChannelInfo(Index);
			}
		} else {
			if (pChInfo->GetServiceID() > 0)
				ServiceID = pChInfo->GetServiceID();
		}

		LPCTSTR pszTuningSpace = m_App.ChannelManager.GetDriverTuningSpaceList()->GetTuningSpaceName(pChInfo->GetSpace());
		m_App.AddLog(
			TEXT("BonDriverにチャンネル変更を要求します。(チューニング空間 {}[{}] / Ch {}[{}] / Sv {})"),
			pChInfo->GetSpace(), pszTuningSpace != nullptr ? pszTuningSpace : TEXT("\?\?\?"),
			pChInfo->GetChannelIndex(), pChInfo->GetName(), ServiceID);

		LibISDB::TSEngine::ServiceSelectInfo ServiceSel;
		ServiceSel.ServiceID = ServiceID > 0 ? ServiceID : LibISDB::SERVICE_ID_INVALID;
		ServiceSel.FollowViewableService = !m_App.NetworkDefinition.IsCSNetworkID(pChInfo->GetNetworkID());

		if (!fStrictService && m_f1SegMode) {
			ServiceSel.OneSegSelect = LibISDB::TSEngine::OneSegSelectType::HighPriority;

			// サブチャンネルの選択の場合、ワンセグもサブチャンネルを優先する
			if (ServiceSel.ServiceID != LibISDB::SERVICE_ID_INVALID) {
				ServiceSel.PreferredServiceIndex =
					GetCorresponding1SegService(
						pChInfo->GetSpace(),
						pChInfo->GetNetworkID(),
						pChInfo->GetTransportStreamID(),
						ServiceSel.ServiceID);
			}
		}

		LibISDB::BonDriverSourceFilter *pSourceFilter =
			m_App.CoreEngine.GetFilter<LibISDB::BonDriverSourceFilter>();
		if (pSourceFilter == nullptr)
			return false;

		m_App.CoreEngine.SetServiceSelectInfo(&ServiceSel);

		if (!pSourceFilter->SetChannelAndPlay(pChInfo->GetSpace(), pChInfo->GetChannelIndex())) {
			m_App.AddLogRaw(CLogItem::LogType::Error, pSourceFilter->GetLastErrorText());
			m_App.ChannelManager.SetCurrentChannel(OldSpace, OldChannel);
			return false;
		}

		m_App.ChannelManager.SetCurrentServiceID(ServiceID);
		m_App.AppEventManager.OnChannelChanged(
			Space != OldSpace ?
				AppEvent::ChannelChangeStatus::SpaceChanged :
				AppEvent::ChannelChangeStatus::None);
	} else {
		if (ServiceID <= 0) {
			if (pChInfo->GetServiceID() > 0)
				ServiceID = pChInfo->GetServiceID();
			else
				return false;
		}
		if (!SetServiceByID(ServiceID, fStrictService ? SetServiceFlag::StrictID : SetServiceFlag::None))
			return false;
	}

	return true;
}


bool CAppCore::SetChannelByIndex(int Space, int Channel, int ServiceID)
{
	const CChannelList *pChannelList = m_App.ChannelManager.GetChannelList(Space);
	if (pChannelList == nullptr)
		return false;

	int ListChannel = pChannelList->FindByIndex(Space, Channel, ServiceID);
	if (ListChannel < 0) {
		if (ServiceID > 0)
			ListChannel = pChannelList->FindByIndex(Space, Channel);
		if (ListChannel < 0)
			return false;
	}

	return SetChannel(Space, ListChannel, ServiceID);
}


bool CAppCore::SelectChannel(LPCTSTR pszTunerName, const CChannelInfo &ChannelInfo, SelectChannelFlag Flags)
{
	const bool fEnabledOnly = !(Flags & SelectChannelFlag::AllowDisabled);

	if (!!(Flags & SelectChannelFlag::UseCurrentTuner)
			&& m_App.CoreEngine.IsTunerOpen()) {
		int Space = m_App.ChannelManager.GetCurrentSpace();
		if (Space != CChannelManager::SPACE_INVALID) {
			int Index = m_App.ChannelManager.FindChannelByIDs(
				Space,
				ChannelInfo.GetNetworkID(),
				ChannelInfo.GetTransportStreamID(),
				ChannelInfo.GetServiceID(),
				fEnabledOnly);
			if (Index < 0 && Space != CChannelManager::SPACE_ALL) {
				for (Space = 0; Space < m_App.ChannelManager.NumSpaces(); Space++) {
					Index = m_App.ChannelManager.FindChannelByIDs(
						Space,
						ChannelInfo.GetNetworkID(),
						ChannelInfo.GetTransportStreamID(),
						ChannelInfo.GetServiceID(),
						fEnabledOnly);
					if (Index >= 0)
						break;
				}
			}
			if (Index >= 0) {
				if (!m_App.UICore.ConfirmChannelChange())
					return false;
				return SetChannel(Space, Index, -1, !!(Flags & SelectChannelFlag::StrictService));
			}
		}
	}

	if (IsStringEmpty(pszTunerName))
		return false;

	if (!m_App.UICore.ConfirmChannelChange())
		return false;

	if (!OpenTuner(pszTunerName))
		return false;

	int Space = CChannelManager::SPACE_INVALID, Channel = -1;
	const CChannelList *pChannelList = m_App.ChannelManager.GetCurrentChannelList();

	if (pChannelList != nullptr) {
		if (ChannelInfo.GetChannelIndex() >= 0) {
			Channel = pChannelList->FindByIndex(
				ChannelInfo.GetSpace(),
				ChannelInfo.GetChannelIndex(),
				ChannelInfo.GetServiceID(),
				fEnabledOnly);
		} else if (ChannelInfo.GetServiceID() > 0
				&& (ChannelInfo.GetNetworkID() > 0
					|| ChannelInfo.GetTransportStreamID() > 0)) {
			Channel = pChannelList->FindByIDs(
				ChannelInfo.GetNetworkID(),
				ChannelInfo.GetTransportStreamID(),
				ChannelInfo.GetServiceID(),
				fEnabledOnly);
		}
		if (Channel >= 0)
			Space = m_App.ChannelManager.GetCurrentSpace();
	}

	if (Channel < 0) {
		if (m_App.ChannelManager.GetCurrentSpace() == CChannelManager::SPACE_ALL
				|| ChannelInfo.GetSpace() == m_App.ChannelManager.GetCurrentSpace())
			return false;
		pChannelList = m_App.ChannelManager.GetChannelList(ChannelInfo.GetSpace());
		if (pChannelList == nullptr)
			return false;

		if (ChannelInfo.GetChannelIndex() >= 0) {
			Channel = pChannelList->FindByIndex(
				ChannelInfo.GetSpace(),
				ChannelInfo.GetChannelIndex(),
				ChannelInfo.GetServiceID(),
				fEnabledOnly);
		}
		if (Channel < 0) {
			if (ChannelInfo.GetServiceID() > 0
					&& (ChannelInfo.GetNetworkID() > 0
						|| ChannelInfo.GetTransportStreamID() > 0)) {
				Channel = pChannelList->FindByIDs(
					ChannelInfo.GetNetworkID(),
					ChannelInfo.GetTransportStreamID(),
					ChannelInfo.GetServiceID(),
					fEnabledOnly);
			}
			if (Channel < 0)
				return false;
		}
		Space = ChannelInfo.GetSpace();
	}

	return SetChannel(Space, Channel, -1, !!(Flags & SelectChannelFlag::StrictService));
}


bool CAppCore::SwitchChannel(int Channel)
{
	const CChannelList *pChList = m_App.ChannelManager.GetCurrentChannelList();
	if (pChList == nullptr)
		return false;
	const CChannelInfo *pChInfo = pChList->GetChannelInfo(Channel);
	if (pChInfo == nullptr)
		return false;

	if (!m_App.UICore.ConfirmChannelChange())
		return false;

	return SetChannel(m_App.ChannelManager.GetCurrentSpace(), Channel);
}


bool CAppCore::SwitchChannelByNo(int ChannelNo, bool fSwitchService)
{
	if (ChannelNo < 1)
		return false;

	const CChannelList *pList = m_App.ChannelManager.GetCurrentChannelList();
	if (pList == nullptr)
		return false;

	int Index;

	if (pList->HasRemoteControlKeyID()) {
		Index = pList->FindChannelNo(ChannelNo);

		if (fSwitchService) {
			const CChannelInfo *pCurChInfo = m_App.ChannelManager.GetCurrentChannelInfo();

			if (pCurChInfo != nullptr && pCurChInfo->GetChannelNo() == ChannelNo) {
				const int NumChannels = pList->NumChannels();

				for (int i = m_App.ChannelManager.GetCurrentChannel() + 1; i < NumChannels; i++) {
					const CChannelInfo *pChInfo = pList->GetChannelInfo(i);

					if (pChInfo->IsEnabled() && pChInfo->GetChannelNo() == ChannelNo) {
						Index = i;
						break;
					}
				}
			}
		}

		if (Index < 0)
			return false;
	} else {
		Index = ChannelNo - 1;
	}

	return SwitchChannel(Index);
}


bool CAppCore::SetCommandLineChannel(const CCommandLineOptions *pCmdLine)
{
	CChannelInfo FindChannel;

	if (pCmdLine->m_Channel > 0)
		FindChannel.SetPhysicalChannel(pCmdLine->m_Channel);
	if (pCmdLine->m_ControllerChannel > 0)
		FindChannel.SetChannelNo(pCmdLine->m_ControllerChannel);
	if (pCmdLine->m_ChannelIndex >= 0)
		FindChannel.SetChannelIndex(pCmdLine->m_ChannelIndex);
	if (pCmdLine->m_TuningSpace >= 0)
		FindChannel.SetSpace(pCmdLine->m_TuningSpace);
	if (pCmdLine->m_ServiceID > 0)
		FindChannel.SetServiceID(static_cast<WORD>(pCmdLine->m_ServiceID));
	if (pCmdLine->m_NetworkID > 0)
		FindChannel.SetNetworkID(static_cast<WORD>(pCmdLine->m_NetworkID));
	if (pCmdLine->m_TransportStreamID > 0)
		FindChannel.SetTransportStreamID(static_cast<WORD>(pCmdLine->m_TransportStreamID));

	// まず有効なチャンネルから探し、無ければ全てのチャンネルから探す
	for (int i = 0; i < 2; i++) {
		for (int Space = 0; Space < m_App.ChannelManager.NumSpaces(); Space++) {
			const CChannelList *pChannelList = m_App.ChannelManager.GetChannelList(Space);
			if (pChannelList != nullptr
					&& (pCmdLine->m_TuningSpace < 0 || Space == pCmdLine->m_TuningSpace)) {
				const int Channel = pChannelList->Find(FindChannel, i == 0);
				if (Channel >= 0) {
					return SetChannel(Space, Channel);
				}
			}
		}
	}

	// 指定と完全に一致するチャンネルが無い場合、サービスIDを無視して探し
	// チャンネル設定時にサービスIDを指定する
	if (pCmdLine->m_ServiceID > 0
			&& (pCmdLine->m_Channel > 0 || pCmdLine->m_ChannelIndex >= 0
				|| pCmdLine->m_ControllerChannel > 0
				|| pCmdLine->m_NetworkID > 0 || pCmdLine->m_TransportStreamID > 0)) {
		FindChannel.SetServiceID(0);
		for (int i = 0; i < 2; i++) {
			for (int Space = 0; Space < m_App.ChannelManager.NumSpaces(); Space++) {
				const CChannelList *pChannelList = m_App.ChannelManager.GetChannelList(Space);
				if (pChannelList != nullptr
						|| (pCmdLine->m_TuningSpace < 0 || Space == pCmdLine->m_TuningSpace)) {
					const int Channel = pChannelList->Find(FindChannel, i == 0);
					if (Channel >= 0) {
						return SetChannel(Space, Channel, pCmdLine->m_ServiceID);
					}
				}
			}
		}
	}

	m_App.AddLog(CLogItem::LogType::Error, TEXT("コマンドラインで指定されたチャンネルが見付かりません。"));

	return false;
}


bool CAppCore::FollowChannelChange(WORD TransportStreamID, WORD ServiceID)
{
	const CChannelList *pChannelList = m_App.ChannelManager.GetCurrentChannelList();
	const CChannelInfo *pChannelInfo = nullptr;
	int Space, Channel;

	if (pChannelList != nullptr) {
		Channel = pChannelList->FindByIDs(0, TransportStreamID, ServiceID);
		if (Channel >= 0) {
			pChannelInfo = pChannelList->GetChannelInfo(Channel);
			Space = m_App.ChannelManager.GetCurrentSpace();
		}
	} else {
		for (int i = 0; i < m_App.ChannelManager.NumSpaces(); i++) {
			pChannelList = m_App.ChannelManager.GetChannelList(i);
			if (pChannelList != nullptr) {
				Channel = pChannelList->FindByIDs(0, TransportStreamID, ServiceID);
				if (Channel >= 0) {
					pChannelInfo = pChannelList->GetChannelInfo(Channel);
					Space = i;
					break;
				}
			}
		}
	}
	if (pChannelInfo == nullptr)
		return false;

	const CChannelInfo *pCurChInfo = m_App.ChannelManager.GetCurrentChannelInfo();
	if (pCurChInfo == nullptr
			|| pCurChInfo->GetTransportStreamID() != TransportStreamID) {
		m_App.AddLog(TEXT("ストリームの変化を検知しました。(TSID {} / SID {})"), TransportStreamID, ServiceID);
	}
	const bool fSpaceChanged = Space != m_App.ChannelManager.GetCurrentSpace();
	if (!m_App.ChannelManager.SetCurrentChannel(Space, Channel))
		return false;
	m_App.ChannelManager.SetCurrentServiceID(0);

	AppEvent::ChannelChangeStatus Status = AppEvent::ChannelChangeStatus::Detected;
	if (fSpaceChanged)
		Status |= AppEvent::ChannelChangeStatus::SpaceChanged;
	m_App.AppEventManager.OnChannelChanged(Status);
	return true;
}


bool CAppCore::SetServiceByID(WORD ServiceID, SetServiceFlag Flags)
{
	TRACE(
		TEXT("CAppCore::SetServiceByID({:04x}, {:x})\n"),
		ServiceID, static_cast<std::underlying_type_t<SetServiceFlag>>(Flags));

	const bool fStrict = !!(Flags & SetServiceFlag::StrictID);
	const CChannelInfo *pCurChInfo = m_App.ChannelManager.GetCurrentChannelInfo();
	const LibISDB::AnalyzerFilter *pAnalyzer =
		m_App.CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
	if (pAnalyzer == nullptr)
		return false;
	WORD NetworkID = pAnalyzer->GetNetworkID();
	if (NetworkID == LibISDB::NETWORK_ID_INVALID
			&& pCurChInfo != nullptr && pCurChInfo->GetNetworkID() > 0)
		NetworkID = pCurChInfo->GetNetworkID();

	LibISDB::TSEngine::ServiceSelectInfo ServiceSel;

	ServiceSel.ServiceID = ServiceID;
	ServiceSel.FollowViewableService = !m_App.NetworkDefinition.IsCSNetworkID(NetworkID);

	if (!fStrict && m_f1SegMode) {
		ServiceSel.OneSegSelect = LibISDB::TSEngine::OneSegSelectType::HighPriority;

		if (pCurChInfo != nullptr) {
			// サブチャンネルの選択の場合、ワンセグもサブチャンネルを優先する
			if (ServiceSel.ServiceID != LibISDB::SERVICE_ID_INVALID) {
				ServiceSel.PreferredServiceIndex =
					GetCorresponding1SegService(
						pCurChInfo->GetSpace(),
						pCurChInfo->GetNetworkID(),
						pCurChInfo->GetTransportStreamID(),
						ServiceSel.ServiceID);
			}
		}
	}

	bool fResult;

	if (ServiceSel.ServiceID == LibISDB::SERVICE_ID_INVALID) {
		m_App.AddLog(TEXT("デフォルトのサービスを選択します..."));
		fResult = m_App.CoreEngine.SetService(ServiceSel);
		if (fResult) {
			ServiceID = m_App.CoreEngine.GetServiceID();
		}
	} else {
		if (ServiceSel.OneSegSelect == LibISDB::TSEngine::OneSegSelectType::HighPriority)
			m_App.AddLog(TEXT("サービスを選択します..."));
		else
			m_App.AddLog(TEXT("サービスを選択します(SID {})..."), ServiceSel.ServiceID);
		fResult = m_App.CoreEngine.SetService(ServiceSel);
	}
	if (!fResult) {
		m_App.AddLog(CLogItem::LogType::Error, TEXT("サービスを選択できません。"));
		return false;
	}

	if (!(Flags & SetServiceFlag::NoChangeCurrentServiceID))
		m_App.ChannelManager.SetCurrentServiceID(ServiceID);

	if (ServiceID != LibISDB::SERVICE_ID_INVALID) {
		const int ServiceIndex = pAnalyzer->GetServiceIndexByID(ServiceID);
		if (ServiceIndex >= 0) {
			//m_App.AddLog(TEXT("サービスを変更しました。(SID {})"), ServiceID);

			if (fStrict && m_f1SegMode
					&& !pAnalyzer->Is1SegService(ServiceIndex)) {
				Set1SegMode(false, false);
			}
		}
	}

	if (m_f1SegMode && ServiceSel.OneSegSelect != LibISDB::TSEngine::OneSegSelectType::HighPriority) {
		Set1SegMode(false, false);
	}

	bool fChannelChanged = false;

	if (/*!m_f1SegMode && */ServiceID != LibISDB::SERVICE_ID_INVALID && pCurChInfo != nullptr) {
		const CChannelList *pChList = m_App.ChannelManager.GetCurrentChannelList();
		const int Index = pChList->FindByIndex(pCurChInfo->GetSpace(), pCurChInfo->GetChannelIndex(), ServiceID);
		if (Index >= 0) {
			m_App.ChannelManager.SetCurrentChannel(m_App.ChannelManager.GetCurrentSpace(), Index);
			m_App.AppEventManager.OnChannelChanged(AppEvent::ChannelChangeStatus::None);
			fChannelChanged = true;
		}
	}

	if (!fChannelChanged)
		m_App.AppEventManager.OnServiceChanged();

	return true;
}


bool CAppCore::SetServiceByIndex(int Service, SetServiceFlag Flags)
{
	if (Service < 0)
		return false;

	const WORD ServiceID = m_App.CoreEngine.GetSelectableServiceID(Service);
	if (ServiceID == LibISDB::SERVICE_ID_INVALID)
		return false;

	return SetServiceByID(ServiceID, Flags);
}


bool CAppCore::GetCurrentStreamIDInfo(StreamIDInfo *pInfo) const
{
	if (pInfo == nullptr)
		return false;

	const LibISDB::AnalyzerFilter *pAnalyzer =
		m_App.CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
	if (pAnalyzer == nullptr)
		return false;

	pInfo->NetworkID = pAnalyzer->GetNetworkID();
	pInfo->TransportStreamID = pAnalyzer->GetTransportStreamID();
	WORD ServiceID = m_App.CoreEngine.GetServiceID();
	if (ServiceID == LibISDB::SERVICE_ID_INVALID) {
		const int CurServiceID = m_App.ChannelManager.GetCurrentServiceID();
		if (CurServiceID > 0)
			ServiceID = static_cast<WORD>(CurServiceID);
	}
	pInfo->ServiceID = ServiceID;

	return true;
}


bool CAppCore::GetCurrentStreamChannelInfo(CChannelInfo *pInfo, bool fName) const
{
	if (pInfo == nullptr)
		return false;

	StreamIDInfo IDInfo;
	if (!GetCurrentStreamIDInfo(&IDInfo))
		return false;

	const CChannelInfo *pCurChInfo = m_App.ChannelManager.GetCurrentChannelInfo();
	if (pCurChInfo != nullptr
			&& pCurChInfo->GetNetworkID() == IDInfo.NetworkID
			&& pCurChInfo->GetTransportStreamID() == IDInfo.TransportStreamID
			&& pCurChInfo->GetServiceID() == IDInfo.ServiceID) {
		*pInfo = *pCurChInfo;
	} else {
		CChannelInfo ChInfo;
		ChInfo.SetNetworkID(IDInfo.NetworkID);
		ChInfo.SetTransportStreamID(IDInfo.TransportStreamID);
		ChInfo.SetServiceID(IDInfo.ServiceID);
		*pInfo = ChInfo;
		if (fName) {
			TCHAR szService[MAX_CHANNEL_NAME];
			if (GetCurrentServiceName(szService, lengthof(szService), false))
				pInfo->SetName(szService);
		}
	}

	return true;
}


bool CAppCore::GetCurrentServiceName(LPTSTR pszName, int MaxLength, bool fUseChannelName) const
{
	if (pszName == nullptr || MaxLength < 1)
		return false;

	const WORD ServiceID = m_App.CoreEngine.GetServiceID();

	const CChannelInfo *pChannelInfo = nullptr;
	if (fUseChannelName) {
		pChannelInfo = m_App.ChannelManager.GetCurrentChannelInfo();
		if (pChannelInfo != nullptr) {
			if (ServiceID == LibISDB::SERVICE_ID_INVALID
					|| pChannelInfo->GetServiceID() <= 0
					|| pChannelInfo->GetServiceID() == ServiceID) {
				StringCopy(pszName, pChannelInfo->GetName(), MaxLength);
				return true;
			}
		}
	}

	pszName[0] = _T('\0');

	if (ServiceID == LibISDB::SERVICE_ID_INVALID)
		return false;

	const LibISDB::AnalyzerFilter *pAnalyzer =
		m_App.CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
	if (pAnalyzer == nullptr)
		return false;

	const int Index = pAnalyzer->GetServiceIndexByID(ServiceID);
	if (Index < 0)
		return false;
#if 0
	if (pChannelInfo != nullptr) {
		const size_t Length = StringFormat(pszName, MaxLength, TEXT("#{} "), Index + 1);
		pszName += Length;
		MaxLength -= static_cast<int>(Length);
	}
#endif
	LibISDB::String Name;
	if (!pAnalyzer->GetServiceName(Index, &Name))
		return false;
	StringCopy(pszName, Name.c_str(), MaxLength);

	return true;
}


bool CAppCore::OpenTuner(LPCTSTR pszFileName)
{
	if (IsStringEmpty(pszFileName))
		return false;
	if (m_App.CoreEngine.IsTunerOpen()
			&& IsEqualFileName(m_App.CoreEngine.GetDriverFileName(), pszFileName))
		return true;

	TRACE(TEXT("CAppCore::OpenTuner({})\n"), pszFileName);

	const HCURSOR hcurOld = ::SetCursor(::LoadCursor(nullptr, IDC_WAIT));
	bool fOK;

	SaveCurrentChannel();

	m_App.UICore.SetStatusBarTrace(true);

	if (m_App.CoreEngine.IsTunerOpen()) {
		m_App.CoreEngine.CloseTuner();
	}

	m_App.TSProcessorManager.OnTunerChange(m_App.CoreEngine.GetDriverFileName(), pszFileName);

	m_App.CoreEngine.SetDriverFileName(pszFileName);
	fOK = OpenAndInitializeTuner();
	if (fOK) {
		InitializeChannel();
		::SetCursor(hcurOld);
	} else {
		::SetCursor(hcurOld);
		OnError(&m_App.CoreEngine, TEXT("BonDriverの初期化ができません。"));
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
	bool fOK = true;

	if (!m_App.CoreEngine.IsTunerOpen()) {
		if (OpenAndInitializeTuner(OpenTunerFlag::NoUI)) {
			m_App.AppEventManager.OnTunerOpened();
		} else {
			OnError(&m_App.CoreEngine, TEXT("BonDriverの初期化ができません。"));
			fOK = false;
		}
	}

	m_App.UICore.SetStatusBarTrace(false);

	return fOK;
}


bool CAppCore::OpenAndInitializeTuner(OpenTunerFlag OpenFlags)
{
	CDriverOptions::BonDriverOptions Options(m_App.CoreEngine.GetDriverFileName());
	m_App.DriverOptions.GetBonDriverOptions(m_App.CoreEngine.GetDriverFileName(), &Options);

	m_App.CoreEngine.SetStartStreamingOnSourceOpen(!Options.fIgnoreInitialStream);

	if (!m_App.CoreEngine.OpenTuner())
		return false;

	m_App.AddLog(TEXT("{} を読み込みました。"), m_App.CoreEngine.GetDriverFileName());

	ApplyBonDriverOptions();

	CTSProcessorManager::FilterOpenFlag DecoderOpenFlags = CTSProcessorManager::FilterOpenFlag::None;
	if (!(OpenFlags & OpenTunerFlag::NoNotify))
		DecoderOpenFlags |= CTSProcessorManager::FilterOpenFlag::NotifyError;
	if (m_fSilent || !!(OpenFlags & OpenTunerFlag::NoUI))
		DecoderOpenFlags |= CTSProcessorManager::FilterOpenFlag::NoUI;
	else if (!!(OpenFlags & OpenTunerFlag::RetryDialog))
		DecoderOpenFlags |= CTSProcessorManager::FilterOpenFlag::RetryDialog;
	m_App.TSProcessorManager.OnTunerOpened(m_App.CoreEngine.GetDriverFileName(), DecoderOpenFlags);

	return true;
}


bool CAppCore::CloseTuner()
{
	TRACE(TEXT("CAppCore::CloseTuner()\n"));

	m_App.TSProcessorManager.CloseAllFilters();

	if (m_App.CoreEngine.IsTunerOpen()) {
		m_App.CoreEngine.CloseTuner();
		SaveCurrentChannel();
		m_App.ChannelManager.SetCurrentChannel(m_App.ChannelManager.GetCurrentSpace(), -1);
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
		m_App.CoreEngine.SetDriverFileName(nullptr);
		m_App.AppEventManager.OnTunerShutDown();
	}
}


void CAppCore::ResetEngine()
{
	m_App.CoreEngine.ResetEngine();
	m_App.AppEventManager.OnEngineReset();
}


bool CAppCore::Set1SegMode(bool f1Seg, bool fServiceChange)
{
	if (m_f1SegMode != f1Seg) {
		m_f1SegMode = f1Seg;

		m_App.AddLog(TEXT("ワンセグモードを{}にします。"), f1Seg ? TEXT("オン") : TEXT("オフ"));

		if (fServiceChange) {
			if (m_f1SegMode) {
				const LibISDB::AnalyzerFilter *pAnalyzer =
					m_App.CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();

				if (pAnalyzer->Has1SegService()) {
					WORD ServiceID = LibISDB::SERVICE_ID_INVALID;
					CChannelInfo ChInfo;

					if (GetCurrentStreamChannelInfo(&ChInfo)) {
						const int Index = GetCorresponding1SegService(
							ChInfo.GetSpace(),
							ChInfo.GetNetworkID(),
							ChInfo.GetTransportStreamID(),
							ChInfo.GetServiceID());
						ServiceID = pAnalyzer->Get1SegServiceIDByIndex(Index);
					}

					SetServiceByID(ServiceID, SetServiceFlag::NoChangeCurrentServiceID);
				} else {
					m_App.CoreEngine.SetOneSegSelectType(
						LibISDB::TSEngine::OneSegSelectType::HighPriority);
				}
			} else {
				SetServiceByID(
					m_App.ChannelManager.GetCurrentServiceID() > 0 ?
					m_App.ChannelManager.GetCurrentServiceID() : LibISDB::SERVICE_ID_INVALID,
					SetServiceFlag::NoChangeCurrentServiceID);
			}
		}

		m_App.AppEventManager.On1SegModeChanged(m_f1SegMode);
	}

	return true;
}


int CAppCore::GetCorresponding1SegService(
	int Space, WORD NetworkID, WORD TSID, WORD ServiceID) const
{
	if (ServiceID == 0)
		return 0;

	int ServiceIndex = 0;

	for (WORD SID = ServiceID - 1; SID > 0; SID--) {
		if (m_App.ChannelManager.FindChannelByIDs(Space, NetworkID, TSID, SID, false) < 0)
			break;
		ServiceIndex = ServiceID - SID;
	}

	// フルセグのマルチ放送のサービスIDが101/103のように飛んでいる場合があるが、
	// ワンセグのサービスは連続しているため、サブチャンネルは1に固定する
	return std::min(ServiceIndex, 1);
}


void CAppCore::ApplyBonDriverOptions()
{
	if (!m_App.CoreEngine.IsTunerOpen())
		return;

	CDriverOptions::BonDriverOptions Options(m_App.CoreEngine.GetDriverFileName());
	m_App.DriverOptions.GetBonDriverOptions(m_App.CoreEngine.GetDriverFileName(), &Options);

	//m_App.CoreEngine.SetStartStreamingOnSourceOpen(!Options.fIgnoreInitialStream);

	LibISDB::BonDriverSourceFilter *pSourceFilter =
		m_App.CoreEngine.GetFilter<LibISDB::BonDriverSourceFilter>();
	if (pSourceFilter != nullptr) {
		pSourceFilter->SetPurgeStreamOnChannelChange(Options.fPurgeStreamOnChannelChange);
		pSourceFilter->SetFirstChannelSetDelay(Options.FirstChannelSetDelay);
		pSourceFilter->SetMinChannelChangeInterval(Options.MinChannelChangeInterval);
	}

	LibISDB::ViewerFilter *pViewerFilter =
		m_App.CoreEngine.GetFilter<LibISDB::ViewerFilter>();
	if (pViewerFilter != nullptr) {
		pViewerFilter->SetPacketInputWait(Options.fPumpStreamSyncPlayback ? 3000 : 0);
	}
}


bool CAppCore::SaveCurrentChannel()
{
	if (!IsStringEmpty(m_App.CoreEngine.GetDriverFileName())) {
		const CChannelInfo *pInfo = m_App.ChannelManager.GetCurrentChannelInfo();

		if (pInfo != nullptr) {
			CDriverOptions::ChannelInfo ChInfo;

			ChInfo.Space = pInfo->GetSpace();
			ChInfo.Channel = pInfo->GetChannelIndex();
			ChInfo.ServiceID = pInfo->GetServiceID();
			ChInfo.TransportStreamID = pInfo->GetTransportStreamID();
			ChInfo.fAllChannels = m_App.ChannelManager.GetCurrentSpace() == CChannelManager::SPACE_ALL;
			m_App.DriverOptions.SetLastChannel(m_App.CoreEngine.GetDriverFileName(), &ChInfo);
		}
	}
	return true;
}


bool CAppCore::GenerateRecordFileName(LPTSTR pszFileName, int MaxFileName)
{
	CRecordManager::FileNameFormatInfo FormatInfo;

	GetVariableStringEventInfo(&FormatInfo, 60 * 1000);

	String Path;
	if (!m_App.RecordManager.GenerateFilePath(FormatInfo, nullptr, &Path)) {
		OnError(TEXT("録画ファイルのパスを作成できません。"));
		return false;
	}
	if (!GetAbsolutePath(Path.c_str(), pszFileName, MaxFileName)) {
		OnError(TEXT("録画ファイルの保存先フォルダの指定が正しくないか、パスが長過ぎます。"));
		return false;
	}

	TCHAR szDir[MAX_PATH];
	StringCopy(szDir, pszFileName);
	::PathRemoveFileSpec(szDir);
	if (!::PathIsDirectory(szDir)) {
		const int Result = ::SHCreateDirectoryEx(nullptr, szDir, nullptr);
		if (Result != ERROR_SUCCESS && Result != ERROR_ALREADY_EXISTS) {
			OnError(TEXT("録画ファイルの保存先フォルダ \"{}\" を作成できません。"), szDir);
			return false;
		}
	}

	return true;
}


bool CAppCore::StartRecord(
	LPCTSTR pszFileName,
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
	if (m_App.CmdLineOptions.m_fRecordCurServiceOnly)
		m_App.RecordManager.SetCurServiceOnly(true);
	if (m_App.RecordManager.IsReserved()) {
		m_App.StatusView.UpdateItem(STATUS_ITEM_RECORD);
		return true;
	}

	OpenTuner();

	if (IsStringEmpty(pszFileName)) {
		CFilePath Path;
		LPCTSTR pszErrorMessage;

		if (!m_App.RecordOptions.GenerateFilePath(&Path, &pszErrorMessage)) {
			m_App.UICore.GetSkin()->ShowErrorMessage(pszErrorMessage);
			return false;
		}
		m_App.RecordManager.SetFileName(Path.c_str());
	}
	TCHAR szFileName[MAX_PATH * 2];
	if (!GenerateRecordFileName(szFileName, lengthof(szFileName)))
		return false;
	AppEvent::RecordingStartInfo RecStartInfo;
	RecStartInfo.pRecordManager = &m_App.RecordManager;
	RecStartInfo.pszFileName = szFileName;
	RecStartInfo.MaxFileName = lengthof(szFileName);
	m_App.AppEventManager.OnRecordingStart(&RecStartInfo);
	m_App.CoreEngine.ResetErrorCount();
	if (!m_App.RecordManager.StartRecord(szFileName, fTimeShift)) {
		OnError(&m_App.RecordManager, TEXT("録画を開始できません。"));
		return false;
	}
	m_App.TaskTrayManager.SetStatus(
		CTaskTrayManager::StatusFlag::Recording,
		CTaskTrayManager::StatusFlag::Recording);
	m_App.AddLog(TEXT("録画開始 {}"), szFileName);
	m_App.AppEventManager.OnRecordingStarted();
	return true;
}


bool CAppCore::ModifyRecord(
	LPCTSTR pszFileName,
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
		if (!GenerateRecordFileName(szFileName, MAX_PATH))
			return false;
		/*
		if (!m_App.RecordManager.DoFileExistsOperation(m_App.UICore.GetDialogOwner(), szFileName))
			return false;
		*/
	} else {
		CFilePath Path;
		LPCTSTR pszErrorMessage;

		if (!m_App.RecordOptions.GenerateFilePath(&Path, &pszErrorMessage)) {
			m_App.UICore.GetSkin()->ShowErrorMessage(pszErrorMessage);
			return false;
		}
		m_App.RecordManager.SetFileName(Path.c_str());
		if (!GenerateRecordFileName(szFileName, MAX_PATH))
			return false;
	}
	OpenTuner();
	AppEvent::RecordingStartInfo RecStartInfo;
	RecStartInfo.pRecordManager = &m_App.RecordManager;
	RecStartInfo.pszFileName = szFileName;
	RecStartInfo.MaxFileName = lengthof(szFileName);
	m_App.AppEventManager.OnRecordingStart(&RecStartInfo);
	m_App.CoreEngine.ResetErrorCount();
	if (!m_App.RecordManager.StartRecord(szFileName, false, true)) {
		m_App.RecordManager.CancelReserve();
		OnError(&m_App.RecordManager, TEXT("録画を開始できません。"));
		return false;
	}
	String ActualFileName;
	m_App.RecordManager.GetRecordTask()->GetFileName(&ActualFileName);
	m_App.AddLog(TEXT("録画開始 {}"), ActualFileName);

	m_App.TaskTrayManager.SetStatus(
		CTaskTrayManager::StatusFlag::Recording,
		CTaskTrayManager::StatusFlag::Recording);
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

	const CRecordTask *pTask = m_App.RecordManager.GetRecordTask();
	String FileName;
	pTask->GetFileName(&FileName);

	m_App.RecordManager.StopRecord();

	LibISDB::RecorderFilter::RecordingStatistics Stats;
	pTask->GetStatistics(&Stats);
	m_App.AddLog(
		TEXT("録画停止 {} (出力TSサイズ {} Bytes / 書き出しエラー回数 {})"),
		FileName, Stats.OutputBytes, Stats.WriteErrorCount);

	m_App.TaskTrayManager.SetStatus(CTaskTrayManager::StatusFlag::None, CTaskTrayManager::StatusFlag::Recording);
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
		OnError(&m_App.RecordManager, TEXT("録画ファイルを切り替えできません。"));
		return false;
	}
	m_App.AddLog(TEXT("録画ファイルを切り替えました {}"), pszFileName);
	m_App.AppEventManager.OnRecordingFileChanged(pszFileName);
	return true;
}


bool CAppCore::CommandLineRecord(const CCommandLineOptions *pCmdLine)
{
	if (pCmdLine == nullptr)
		return false;
	return CommandLineRecord(
		pCmdLine->m_RecordFileName.c_str(),
		&pCmdLine->m_RecordStartTime,
		pCmdLine->m_RecordDelay,
		pCmdLine->m_RecordDuration);

}


bool CAppCore::CommandLineRecord(LPCTSTR pszFileName, const SYSTEMTIME *pStartTime, int Delay, int Duration)
{
	CRecordManager::TimeSpecInfo StartTime, StopTime;

	if (pStartTime != nullptr && pStartTime->wYear != 0) {
		StartTime.Type = CRecordManager::TimeSpecType::DateTime;
		::TzSpecificLocalTimeToSystemTime(nullptr, pStartTime, &StartTime.Time.DateTime);
		if (Delay != 0)
			OffsetSystemTime(&StartTime.Time.DateTime, Delay * TimeConsts::SYSTEMTIME_SECOND);
		SYSTEMTIME st;
		::SystemTimeToTzSpecificLocalTime(nullptr, &StartTime.Time.DateTime, &st);
		m_App.AddLog(
			TEXT("コマンドラインから録画指定されました。({}/{}/{} {}:{:02}:{:02} 開始)"),
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	} else if (Delay > 0) {
		StartTime.Type = CRecordManager::TimeSpecType::Duration;
		StartTime.Time.Duration = Delay * 1000;
		m_App.AddLog(TEXT("コマンドラインから録画指定されました。({} 秒後開始)"), Delay);
	} else {
		StartTime.Type = CRecordManager::TimeSpecType::NotSpecified;
		m_App.AddLog(TEXT("コマンドラインから録画指定されました。"));
	}
	if (Duration > 0) {
		StopTime.Type = CRecordManager::TimeSpecType::Duration;
		StopTime.Time.Duration = Duration * 1000;
	} else {
		StopTime.Type = CRecordManager::TimeSpecType::NotSpecified;
	}

	return StartRecord(
		IsStringEmpty(pszFileName) ? nullptr : pszFileName,
		&StartTime, &StopTime,
		CRecordManager::RecordClient::CommandLine);
}


LPCTSTR CAppCore::GetDefaultRecordFolder() const
{
	return m_App.RecordOptions.GetSaveFolder();
}


void CAppCore::BeginChannelScan(int Space)
{
	m_App.ChannelManager.SetCurrentChannel(Space, -1);
	m_App.AppEventManager.OnChannelChanged(AppEvent::ChannelChangeStatus::SpaceChanged);
}


bool CAppCore::IsChannelScanning() const
{
	return m_App.ChannelScan.IsScanning();
}


bool CAppCore::IsDriverNoSignalLevel(LPCTSTR pszFileName) const
{
	return m_App.DriverOptions.IsNoSignalLevel(pszFileName);
}


void CAppCore::NotifyTSProcessorNetworkChanged(CTSProcessorManager::FilterOpenFlag FilterOpenFlags)
{
	StreamIDInfo StreamID;

	GetCurrentStreamIDInfo(&StreamID);
	m_App.TSProcessorManager.OnNetworkChanged(
		m_App.CoreEngine.GetDriverFileName(),
		StreamID.NetworkID,
		StreamID.TransportStreamID,
		StreamID.ServiceID,
		FilterOpenFlags);
}


bool CAppCore::GetVariableStringEventInfo(
	CEventVariableStringMap::EventInfo *pInfo, DWORD NextEventMargin) const
{
	if (pInfo == nullptr)
		return false;

	const CChannelInfo *pChannelInfo = m_App.ChannelManager.GetCurrentChannelInfo();
	bool fEventInfoValid = false;

	if (pChannelInfo != nullptr) {
		pInfo->Channel = *pChannelInfo;
		if (pInfo->Channel.GetChannelNo() == 0
				&& pInfo->Channel.GetServiceID() > 0)
			pInfo->Channel.SetChannelNo(pInfo->Channel.GetServiceID());
		pInfo->Channel.SetTunerName(m_App.CoreEngine.GetDriverFileName());
	}

	const LibISDB::AnalyzerFilter *pAnalyzer = m_App.CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();

	LibISDB::DateTime CurTime;
	if (pAnalyzer != nullptr && !pAnalyzer->GetInterpolatedTOTTime(&CurTime))
		LibISDB::GetCurrentEPGTime(&CurTime);
	pInfo->TOTTime = CurTime;

	if (WORD ServiceID;
			pAnalyzer != nullptr
			&& (ServiceID = m_App.CoreEngine.GetServiceID()) != LibISDB::SERVICE_ID_INVALID) {
		const int Index = pAnalyzer->GetServiceIndexByID(ServiceID);
		LibISDB::String ServiceName;
		if (pAnalyzer->GetServiceName(Index, &ServiceName))
			pInfo->ServiceName = ServiceName;
		bool fNext = false;
		if (NextEventMargin > 0) {
			LibISDB::DateTime StartTime;
			if (pAnalyzer->GetEventTime(Index, &StartTime, nullptr, true)) {
				const long long Diff = StartTime.DiffMilliseconds(CurTime);
				if (Diff >= 0 && Diff < static_cast<long long>(NextEventMargin))
					fNext = true;
			}
		}
		if (pAnalyzer->GetEventInfo(Index, &pInfo->Event, fNext))
			fEventInfoValid = true;
		pInfo->Event.ServiceID = ServiceID;
	} else {
		pInfo->Event.ServiceID = LibISDB::SERVICE_ID_INVALID;
	}
	if (!fEventInfoValid) {
		pInfo->Event.NetworkID = LibISDB::NETWORK_ID_INVALID;
		pInfo->Event.TransportStreamID = LibISDB::TRANSPORT_STREAM_ID_INVALID;
		pInfo->Event.EventID = LibISDB::EVENT_ID_INVALID;
		pInfo->Event.StartTime.Reset();
		pInfo->Event.Duration = 0;
	}

	return true;
}


} // namespace TVTest
