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
#include "ChannelScan.h"
#include "DialogUtil.h"
#include "MessageDialog.h"
#include "resource.h"
#include "Help/HelpID.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace
{

// スキャンスレッドから送られるメッセージ
constexpr UINT WM_APP_BEGINSCAN    = WM_APP + 0;
constexpr UINT WM_APP_CHANNELFOUND = WM_APP + 1;
constexpr UINT WM_APP_ENDCHANNEL   = WM_APP + 2;
constexpr UINT WM_APP_ENDSCAN      = WM_APP + 3;

// スキャン結果
enum class ScanResult {
	Succeeded,
	Cancelled,
	SetChannelPartiallyFailed,
	SetChannelTimeout,
};

}




class CChannelPropDialog
	: public CBasicDialog
{
public:
	CChannelPropDialog(CChannelInfo *pChannelInfo);
	~CChannelPropDialog();
	bool Show(HWND hwndOwner) override;

private:
	INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	CChannelInfo *m_pChannelInfo;
};


CChannelPropDialog::CChannelPropDialog(CChannelInfo *pChannelInfo)
	: m_pChannelInfo(pChannelInfo)
{
}


CChannelPropDialog::~CChannelPropDialog()
{
	Destroy();
}


bool CChannelPropDialog::Show(HWND hwndOwner)
{
	return ShowDialog(
		hwndOwner,
		GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE(IDD_CHANNELPROPERTIES)) == IDOK;
}


INT_PTR CChannelPropDialog::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			TCHAR szText[64];

			::SetProp(hDlg, TEXT("ChannelInfo"), m_pChannelInfo);
			::SendDlgItemMessage(hDlg, IDC_CHANNELPROP_NAME, EM_LIMITTEXT, MAX_CHANNEL_NAME - 1, 0);
			::SetDlgItemText(hDlg, IDC_CHANNELPROP_NAME, m_pChannelInfo->GetName());
			for (int i = 1; i <= 12; i++) {
				StringFormat(szText, TEXT("{}"), i);
				DlgComboBox_AddString(hDlg, IDC_CHANNELPROP_CONTROLKEY, szText);
			}
			if (m_pChannelInfo->GetChannelNo() > 0)
				::SetDlgItemInt(hDlg, IDC_CHANNELPROP_CONTROLKEY, m_pChannelInfo->GetChannelNo(), TRUE);
			StringFormat(
				szText, TEXT("{0} ({0:#04x})"),
				m_pChannelInfo->GetNetworkID());
			::SetDlgItemText(hDlg, IDC_CHANNELPROP_NETWORKID, szText);
			StringFormat(
				szText, TEXT("{0} ({0:#04x})"),
				m_pChannelInfo->GetTransportStreamID());
			::SetDlgItemText(hDlg, IDC_CHANNELPROP_TSID, szText);
			StringFormat(
				szText, TEXT("{0} ({0:#04x})"),
				m_pChannelInfo->GetServiceID());
			::SetDlgItemText(hDlg, IDC_CHANNELPROP_SERVICEID, szText);
			::SetDlgItemInt(hDlg, IDC_CHANNELPROP_TUNINGSPACE, m_pChannelInfo->GetSpace(), TRUE);
			::SetDlgItemInt(hDlg, IDC_CHANNELPROP_CHANNELINDEX, m_pChannelInfo->GetChannelIndex(), TRUE);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			{
				bool fModified = false;
				TCHAR szName[MAX_CHANNEL_NAME];

				::GetDlgItemText(hDlg, IDC_CHANNELPROP_NAME, szName, lengthof(szName));
				if (szName[0] == '\0') {
					::MessageBox(hDlg, TEXT("名前を入力してください。"), TEXT("お願い"), MB_OK | MB_ICONINFORMATION);
					SetDlgItemFocus(hDlg, IDC_CHANNELPROP_NAME);
					return TRUE;
				}
				if (::lstrcmp(m_pChannelInfo->GetName(), szName) != 0) {
					m_pChannelInfo->SetName(szName);
					fModified = true;
				}
				const int ControlKey = ::GetDlgItemInt(hDlg, IDC_CHANNELPROP_CONTROLKEY, nullptr, TRUE);
				if (ControlKey != m_pChannelInfo->GetChannelNo()) {
					m_pChannelInfo->SetChannelNo(ControlKey);
					fModified = true;
				}
				if (!fModified)
					wParam = IDCANCEL;
			}
			[[fallthrough]];
		case IDCANCEL:
			::EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		return TRUE;
	}

	return FALSE;
}




CChannelScan::CChannelListSort::CChannelListSort(int Column, bool fDescending)
	: m_Column(Column)
	, m_fDescending(fDescending)
{
}


int CALLBACK CChannelScan::CChannelListSort::CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	const CChannelListSort *pThis = reinterpret_cast<const CChannelListSort*>(lParamSort);
	const CChannelInfo *pChInfo1 = reinterpret_cast<const CChannelInfo*>(lParam1);
	const CChannelInfo *pChInfo2 = reinterpret_cast<const CChannelInfo*>(lParam2);
	int Cmp;

	switch (pThis->m_Column) {
	case COLUMN_NAME:
		Cmp = ::lstrcmpi(pChInfo1->GetName(), pChInfo2->GetName());
		if (Cmp == 0)
			Cmp = ::lstrcmp(pChInfo1->GetName(), pChInfo2->GetName());
		break;

	case COLUMN_SERVICETYPE:
		Cmp = pChInfo1->GetServiceType() - pChInfo2->GetServiceType();
		break;

	case COLUMN_CHANNELNAME:
		Cmp = pChInfo1->GetChannelIndex() - pChInfo2->GetChannelIndex();
		break;

	case COLUMN_SERVICEID:
		Cmp = GetAppClass().NetworkDefinition.GetNetworkTypeOrder(
			pChInfo1->GetNetworkID(), pChInfo2->GetNetworkID());
		if (Cmp == 0)
			Cmp = pChInfo1->GetServiceID() - pChInfo2->GetServiceID();
		break;

	case COLUMN_REMOTECONTROLKEYID:
		Cmp = pChInfo1->GetChannelNo() - pChInfo2->GetChannelNo();
		break;

	case COLUMN_CHANNELINDEX:
		Cmp = pChInfo1->GetChannelIndex() - pChInfo2->GetChannelIndex();
		break;

	default:
		Cmp = 0;
	}

	return pThis->m_fDescending ? -Cmp : Cmp;
}


void CChannelScan::CChannelListSort::Sort(HWND hwndList)
{
	ListView_SortItems(hwndList, CompareFunc, reinterpret_cast<LPARAM>(this));
}


bool CChannelScan::CChannelListSort::UpdateChannelList(HWND hwndList, CChannelList *pList)
{
	if (pList == nullptr)
		return false;

	CChannelList ChannelList;
	const int Count = ListView_GetItemCount(hwndList);
	LVITEM lvi;

	lvi.mask = LVIF_PARAM;
	lvi.iSubItem = 0;

	for (int i = 0; i < Count; i++) {
		lvi.iItem = i;
		ListView_GetItem(hwndList, &lvi);
		ChannelList.AddChannel(*reinterpret_cast<const CChannelInfo*>(lvi.lParam));
	}

	*pList = ChannelList;

	for (int i = 0; i < Count; i++) {
		lvi.iItem = i;
		lvi.lParam = reinterpret_cast<LPARAM>(pList->GetChannelInfo(i));
		ListView_SetItem(hwndList, &lvi);
	}

	return true;
}




CChannelScan::~CChannelScan()
{
	Destroy();
}


bool CChannelScan::Apply(DWORD Flags)
{
	CAppMain &App = GetAppClass();

	if ((Flags & UPDATE_CHANNELLIST) != 0) {
		App.Core.UpdateCurrentChannelList(&m_TuningSpaceList);
	}

	if ((Flags & UPDATE_PREVIEW) != 0) {
		if (m_fRestorePreview)
			App.UICore.EnableViewer(true);
	}

	return true;
}


bool CChannelScan::ReadSettings(CSettings &Settings)
{
	Settings.Read(TEXT("ChannelScanIgnoreSignalLevel"), &m_fIgnoreSignalLevel);
	Settings.Read(TEXT("ChannelScanWait"), &m_ScanWait);
	Settings.Read(TEXT("ChannelScanRetry"), &m_RetryCount);
	int Value;
	if (Settings.Read(TEXT("ChannelScanSignalLevelThreshold"), &Value))
		m_SignalLevelThreshold = static_cast<float>(Value) / 100.0f;
	Settings.Read(TEXT("ChannelScanDetectDataService"), &m_fDetectDataService);
	Settings.Read(TEXT("ChannelScanDetect1SegService"), &m_fDetect1SegService);
	Settings.Read(TEXT("ChannelScanDetectAudioService"), &m_fDetectAudioService);
	return true;
}


bool CChannelScan::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("ChannelScanIgnoreSignalLevel"), m_fIgnoreSignalLevel);
	Settings.Write(TEXT("ChannelScanWait"), m_ScanWait);
	Settings.Write(TEXT("ChannelScanRetry"), m_RetryCount);
	Settings.Write(TEXT("ChannelScanSignalLevelThreshold"), static_cast<int>(m_SignalLevelThreshold * 100.0f + 0.5f));
	Settings.Write(TEXT("ChannelScanDetectDataService"), m_fDetectDataService);
	Settings.Write(TEXT("ChannelScanDetect1SegService"), m_fDetect1SegService);
	Settings.Write(TEXT("ChannelScanDetectAudioService"), m_fDetectAudioService);
	return true;
}


bool CChannelScan::Create(HWND hwndOwner)
{
	return CreateDialogWindow(
		hwndOwner,
		GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_OPTIONS_CHANNELSCAN));
}


bool CChannelScan::SetTuningSpaceList(const CTuningSpaceList *pTuningSpaceList)
{
	m_pOriginalTuningSpaceList = pTuningSpaceList;
	return true;
}


bool CChannelScan::AutoUpdateChannelList(CTuningSpaceList *pTuningSpaceList, std::vector<String> *pMessageList)
{
	// スキャンされたチャンネルリストの自動更新(テスト)
	// CSはネットワークが複数に分かれているのでこのままじゃ駄目

	if (pTuningSpaceList == nullptr)
		return false;

	const LibISDB::AnalyzerFilter *pAnalyzer = GetAppClass().CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
	LibISDB::AnalyzerFilter::SDTStreamList TsList;

	if (!pAnalyzer->IsSDTComplete()
			|| !pAnalyzer->GetSDTStreamList(&TsList)
			|| TsList.empty())
		return false;

	bool fUpdated = false;
	String Message;

	// 現在のチャンネルリストに存在しないサービスを探す
	for (const auto &TsInfo : TsList) {
		for (const auto &ServiceInfo : TsInfo.ServiceList) {
			if (!IsScanService(ServiceInfo, false))
				continue;

			bool fFound = false;

			for (int i = 0; i < pTuningSpaceList->NumSpaces(); i++) {
				const CChannelList *pChannelList = pTuningSpaceList->GetChannelList(i);

				if (pChannelList != nullptr) {
					if (pChannelList->FindByIDs(TsInfo.OriginalNetworkID, 0, ServiceInfo.ServiceID, false) >= 0) {
						fFound = true;
						break;
					}
				}
			}

			if (!fFound) {
				// 新規サービス
				bool fInserted = false;

				for (int Space = 0; Space < pTuningSpaceList->NumSpaces(); Space++) {
					CChannelList *pChannelList = pTuningSpaceList->GetChannelList(Space);

					if (pChannelList != nullptr) {
						int Index = pChannelList->FindByIDs(TsInfo.OriginalNetworkID, TsInfo.TransportStreamID, 0, false);
						if (Index >= 0) {
							CChannelInfo ChannelInfo(*pChannelList->GetChannelInfo(Index));

							ChannelInfo.SetChannelNo(ServiceInfo.ServiceID);
							ChannelInfo.SetName(ServiceInfo.ServiceName.c_str());
							ChannelInfo.SetServiceID(ServiceInfo.ServiceID);
							ChannelInfo.Enable(true);

							for (; Index < pChannelList->NumChannels(); Index++) {
								const CChannelInfo *pChannelInfo = pChannelList->GetChannelInfo(Index);

								if (pChannelInfo->GetSpace() != ChannelInfo.GetSpace()
										|| pChannelInfo->GetChannelIndex() != ChannelInfo.GetChannelIndex()
										|| pChannelInfo->GetNetworkID() != TsInfo.OriginalNetworkID
										|| pChannelInfo->GetTransportStreamID() != TsInfo.TransportStreamID
										|| pChannelInfo->GetServiceID() > ServiceInfo.ServiceID)
									break;
							}

							pChannelList->InsertChannel(Index, ChannelInfo);

							if (pMessageList != nullptr) {
								StringFormat(
									&Message,
									TEXT("新しいサービス {} \"{}\" (NID {} TSID {:#04x}) を追加しました。"),
									ChannelInfo.GetServiceID(),
									ChannelInfo.GetName(),
									ChannelInfo.GetNetworkID(),
									ChannelInfo.GetTransportStreamID());
								pMessageList->push_back(Message);
							}

							fInserted = true;
							fUpdated = true;
							break;
						}
					}
				}

				if (!fInserted && pMessageList != nullptr) {
					StringFormat(
						&Message,
						TEXT("新しいサービス {} \"{}\" (NID {} TSID {:#04x}) が検出されましたが、当該 TS が見付かりません。"),
						ServiceInfo.ServiceID,
						ServiceInfo.ServiceName,
						TsInfo.OriginalNetworkID,
						TsInfo.TransportStreamID);
					pMessageList->push_back(Message);
				}
			}
		}
	}

	// チャンネルリストから既に存在しないサービスを探す
	for (int Space = 0; Space < pTuningSpaceList->NumSpaces(); Space++) {
		CChannelList *pChannelList = pTuningSpaceList->GetChannelList(Space);

		if (pChannelList != nullptr) {
			for (int Channel = 0; Channel < pChannelList->NumChannels();) {
				const CChannelInfo *pChannelInfo = pChannelList->GetChannelInfo(Channel);
				bool fNetworkFound = false, fServiceFound = false, fServiceMoved = false;

				for (const auto &TsInfo : TsList) {
					if (TsInfo.OriginalNetworkID == pChannelInfo->GetNetworkID()) {
						fNetworkFound = true;

						for (const auto &ServiceInfo : TsInfo.ServiceList) {
							if (ServiceInfo.ServiceID == pChannelInfo->GetServiceID()) {
								fServiceFound = true;

								if (TsInfo.TransportStreamID != pChannelInfo->GetTransportStreamID()) {
									// TS移動
									int Index = pChannelList->FindByIDs(TsInfo.OriginalNetworkID, TsInfo.TransportStreamID, 0, false);
									if (Index >= 0) {
										const CChannelInfo *pMoveChInfo = pChannelList->GetChannelInfo(Index);
										CChannelInfo ChannelInfo(*pChannelInfo);

										ChannelInfo.SetSpace(pMoveChInfo->GetSpace());
										ChannelInfo.SetChannelIndex(pMoveChInfo->GetChannelIndex());
										ChannelInfo.SetTransportStreamID(TsInfo.TransportStreamID);

										for (; Index < pChannelList->NumChannels(); Index++) {
											const CChannelInfo *pChInfo = pChannelList->GetChannelInfo(Index);

											if (pChInfo->GetSpace() != ChannelInfo.GetSpace()
													|| pChInfo->GetChannelIndex() != ChannelInfo.GetChannelIndex()
													|| pChInfo->GetNetworkID() != TsInfo.OriginalNetworkID
													|| pChInfo->GetTransportStreamID() != TsInfo.TransportStreamID
													|| pChInfo->GetServiceID() > ServiceInfo.ServiceID)
												break;
										}

										pChannelList->InsertChannel(Index, ChannelInfo);

										if (pMessageList != nullptr) {
											StringFormat(
												&Message,
												TEXT("サービス {} \"{}\" が TS {:#04x} から {:#04x} に移動しました。"),
												ChannelInfo.GetServiceID(),
												ChannelInfo.GetName(),
												pChannelInfo->GetTransportStreamID(),
												ChannelInfo.GetTransportStreamID());
											pMessageList->push_back(Message);
										}

										fServiceMoved = true;
										fUpdated = false;
									} else {
										if (pMessageList != nullptr) {
											StringFormat(
												&Message,
												TEXT("サービス {} \"{}\" の TS {:#04x} から {:#04x} への移動を検出しましたが、移動先 TS が見付かりません。"),
												ServiceInfo.ServiceID,
												pChannelInfo->GetName(),
												pChannelInfo->GetTransportStreamID(),
												TsInfo.TransportStreamID);
											pMessageList->push_back(Message);
										}
									}
								}
								break;
							}
						}
						if (fServiceFound)
							break;
					}
				}

				if (!fNetworkFound || (fServiceFound && !fServiceMoved)) {
					Channel++;
				} else {
					// サービス削除
					if (!fServiceMoved && pMessageList != nullptr) {
						StringFormat(
							&Message,
							TEXT("サービス {} \"{}\" は削除されました。"),
							pChannelInfo->GetServiceID(),
							pChannelInfo->GetName());
						pMessageList->push_back(Message);
					}

					pChannelList->DeleteChannel(Channel);

					fUpdated = true;
				}
			}
		}
	}

	return fUpdated;
}


void CChannelScan::InsertChannelInfo(int Index, const CChannelInfo *pChInfo, bool fServiceType)
{
	const HWND hwndList = ::GetDlgItem(m_hDlg, IDC_CHANNELSCAN_CHANNELLIST);
	LVITEM lvi;
	TCHAR szText[256];

	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iItem = Index;
	lvi.iSubItem = COLUMN_NAME;
	lvi.pszText = const_cast<LPTSTR>(pChInfo->GetName());
	lvi.lParam = reinterpret_cast<LPARAM>(pChInfo);
	lvi.iItem = ListView_InsertItem(hwndList, &lvi);

	lvi.mask = LVIF_TEXT;
	lvi.iSubItem = COLUMN_SERVICETYPE;
	lvi.pszText = szText;
	if (fServiceType) {
		switch (pChInfo->GetServiceType()) {
		case LibISDB::SERVICE_TYPE_DIGITAL_TV:      StringCopy(szText, TEXT("TV"));                 break;
		case LibISDB::SERVICE_TYPE_DIGITAL_AUDIO:   StringCopy(szText, TEXT("音声"));               break;
		case LibISDB::SERVICE_TYPE_PROMOTION_VIDEO: StringCopy(szText, TEXT("プロモーション映像")); break;
		case LibISDB::SERVICE_TYPE_DATA:            StringCopy(szText, TEXT("データ/ワンセグ"));    break;
		case LibISDB::SERVICE_TYPE_4K_TV:           StringCopy(szText, TEXT("4K TV"));              break;
		default:
			StringFormat(szText, TEXT("他({:02x})"), pChInfo->GetServiceType());
			break;
		}
	} else {
		szText[0] = _T('\0');
	}
	ListView_SetItem(hwndList, &lvi);

	lvi.iSubItem = COLUMN_CHANNELNAME;
	LPCTSTR pszChannelName =
		GetAppClass().CoreEngine.GetFilter<LibISDB::BonDriverSourceFilter>()->GetChannelName(
			pChInfo->GetSpace(), pChInfo->GetChannelIndex());
	StringCopy(szText, !IsStringEmpty(pszChannelName) ? pszChannelName : TEXT("\?\?\?"));
	ListView_SetItem(hwndList, &lvi);

	lvi.iSubItem = COLUMN_SERVICEID;
	StringFormat(szText, TEXT("{}"), pChInfo->GetServiceID());
	ListView_SetItem(hwndList, &lvi);

	if (pChInfo->GetChannelNo() > 0) {
		lvi.iSubItem = COLUMN_REMOTECONTROLKEYID;
		StringFormat(szText, TEXT("{}"), pChInfo->GetChannelNo());
		lvi.pszText = szText;
		ListView_SetItem(hwndList, &lvi);
	}

	lvi.iSubItem = COLUMN_CHANNELINDEX;
	StringFormat(szText, TEXT("{}"), pChInfo->GetChannelIndex());
	ListView_SetItem(hwndList, &lvi);

	ListView_SetCheckState(hwndList, lvi.iItem, pChInfo->IsEnabled());
}


void CChannelScan::SetChannelList(int Space)
{
	const CChannelList *pChannelList = m_TuningSpaceList.GetChannelList(Space);

	ListView_DeleteAllItems(::GetDlgItem(m_hDlg, IDC_CHANNELSCAN_CHANNELLIST));
	if (pChannelList == nullptr)
		return;

	bool fServiceType = true;
	for (int i = 0; i < pChannelList->NumChannels(); i++) {
		if (pChannelList->GetChannelInfo(i)->GetServiceType() == 0) {
			fServiceType = false;
			break;
		}
	}

	m_fChanging = true;
	for (int i = 0; i < pChannelList->NumChannels(); i++)
		InsertChannelInfo(i, pChannelList->GetChannelInfo(i), fServiceType);
	m_fChanging = false;
}


CChannelInfo *CChannelScan::GetSelectedChannelInfo() const
{
	const HWND hwndList = ::GetDlgItem(m_hDlg, IDC_CHANNELSCAN_CHANNELLIST);
	LVITEM lvi;

	lvi.iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
	if (lvi.iItem >= 0) {
		lvi.mask = LVIF_PARAM;
		lvi.iSubItem = 0;
		if (ListView_GetItem(hwndList, &lvi))
			return reinterpret_cast<CChannelInfo*>(lvi.lParam);
	}
	return nullptr;
}


bool CChannelScan::LoadPreset(LPCTSTR pszFileName, CChannelList *pChannelList, int Space, bool *pfCorrupted)
{
	TCHAR szPresetFileName[MAX_PATH];
	CTuningSpaceList PresetList;

	GetAppClass().GetAppDirectory(szPresetFileName);
	::PathAppend(szPresetFileName, pszFileName);
	if (!PresetList.LoadFromFile(szPresetFileName))
		return false;

	const LibISDB::BonDriverSourceFilter *pSourceFilter =
		GetAppClass().CoreEngine.GetFilter<LibISDB::BonDriverSourceFilter>();
	if (pSourceFilter == nullptr)
		return false;
	std::vector<String> BonDriverChannelList;
	LPCTSTR pszName;
	for (int i = 0; (pszName = pSourceFilter->GetChannelName(Space, i)) != nullptr; i++) {
		BonDriverChannelList.emplace_back(pszName);
	}
	if (BonDriverChannelList.empty())
		return false;

	bool fCorrupted = false;
	CChannelList *pPresetChannelList = PresetList.GetAllChannelList();
	for (int i = 0; i < pPresetChannelList->NumChannels(); i++) {
		CChannelInfo ChannelInfo(*pPresetChannelList->GetChannelInfo(i));

		LPCTSTR pszChannelName = ChannelInfo.GetName(), pszDelimiter;
		if (pszChannelName[0] == _T('%')
				&& (pszDelimiter = ::StrChr(pszChannelName + 1, _T(' '))) != nullptr) {
			const int TSNameLength = static_cast<int>(pszDelimiter - pszChannelName) - 1;
			TCHAR szName[MAX_CHANNEL_NAME];
			StringCopy(szName, pszChannelName + 1, TSNameLength + 1);
			bool fFound = false;
			for (size_t j = 0; j < BonDriverChannelList.size(); j++) {
				pszName = BonDriverChannelList[j].c_str();
				LPCTSTR p = ::StrStrI(pszName, szName);
				if (p != nullptr && !::IsCharAlphaNumeric(p[TSNameLength])) {
					ChannelInfo.SetChannelIndex(static_cast<int>(j));
					fFound = true;
					break;
				}
			}
			if (!fFound) {
				if (ChannelInfo.GetChannelIndex() >= static_cast<int>(BonDriverChannelList.size()))
					continue;
				fCorrupted = true;
			}
			StringCopy(szName, pszDelimiter + 1);
			ChannelInfo.SetName(szName);
		} else {
			if (ChannelInfo.GetChannelIndex() >= static_cast<int>(BonDriverChannelList.size())) {
				fCorrupted = true;
				continue;
			}
		}

		ChannelInfo.SetSpace(Space);
		pChannelList->AddChannel(ChannelInfo);
	}

	*pfCorrupted = fCorrupted;

	return true;
}


bool CChannelScan::SetPreset(bool fAuto)
{
	const LibISDB::BonDriverSourceFilter *pSourceFilter =
		GetAppClass().CoreEngine.GetFilter<LibISDB::BonDriverSourceFilter>();
	if (pSourceFilter == nullptr)
		return false;
	LPCTSTR pszName = pSourceFilter->GetSpaceName(m_ScanSpace);
	if (pszName == nullptr)
		return false;

	const bool fBS = ::StrStrI(pszName, TEXT("BS")) != nullptr;
	const bool fCS = ::StrStrI(pszName, TEXT("CS")) != nullptr;
	if (fBS == fCS)
		return false;

	CTuningSpaceInfo *pTuningSpaceInfo = m_TuningSpaceList.GetTuningSpaceInfo(m_ScanSpace);
	if (pTuningSpaceInfo == nullptr)
		return false;

	CChannelList *pChannelList = pTuningSpaceInfo->GetChannelList();
	if (pChannelList == nullptr)
		return false;

	bool fResult, fCorrupted;
	CChannelList NewChannelList;
	if (fBS)
		fResult = LoadPreset(TEXT("Preset_BS.ch2"), &NewChannelList, m_ScanSpace, &fCorrupted);
	else
		fResult = LoadPreset(TEXT("Preset_CS.ch2"), &NewChannelList, m_ScanSpace, &fCorrupted);
	if (!fResult)
		return false;

	if (fCorrupted) {
		if (fAuto)
			return false;
		if (::MessageBox(
					m_hDlg,
					TEXT("この BonDriver はプリセットとチャンネルが異なっている可能性があります。\n")
					TEXT("プリセットを使用せずに、スキャンを行うことをお勧めします。\n")
					TEXT("プリセットを読み込みますか？"),
					TEXT("プリセット読み込み"),
					MB_YESNO | MB_DEFBUTTON2 | MB_ICONINFORMATION) != IDYES)
			return false;
	}

	*pChannelList = NewChannelList;
	pTuningSpaceInfo->SetName(pszName);
	m_fUpdated = true;
	SetChannelList(m_ScanSpace);
	m_SortColumn = -1;
	SetListViewSortMark(::GetDlgItem(m_hDlg, IDC_CHANNELSCAN_CHANNELLIST), -1);

	return true;
}


INT_PTR CChannelScan::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			const CAppMain &App = GetAppClass();
			const CCoreEngine &CoreEngine = App.CoreEngine;
			const LibISDB::BonDriverSourceFilter *pSourceFilter =
				CoreEngine.GetFilter<LibISDB::BonDriverSourceFilter>();
			int NumSpaces;
			LPCTSTR pszName;

			m_TuningSpaceList = *m_pOriginalTuningSpaceList;
			for (NumSpaces = 0; (pszName = pSourceFilter->GetSpaceName(NumSpaces)) != nullptr; NumSpaces++) {
				DlgComboBox_AddString(hDlg, IDC_CHANNELSCAN_SPACE, pszName);
			}
			if (NumSpaces > 0) {
				const CChannelInfo *pCurChannel = App.ChannelManager.GetCurrentChannelInfo();
				if (pCurChannel != nullptr && pCurChannel->GetSpace() < NumSpaces)
					m_ScanSpace = pCurChannel->GetSpace();
				else
					m_ScanSpace = 0;
				DlgComboBox_SetCurSel(hDlg, IDC_CHANNELSCAN_SPACE, m_ScanSpace);
				m_TuningSpaceList.Reserve(NumSpaces);
			} else {
				m_ScanSpace = -1;
				EnableDlgItems(hDlg, IDC_CHANNELSCAN_SPACE, IDC_CHANNELSCAN_START, false);
			}

			m_fUpdated = false;
			m_fScaned = false;
			m_fRestorePreview = false;
			m_SortColumn = -1;

			const HWND hwndList = ::GetDlgItem(hDlg, IDC_CHANNELSCAN_CHANNELLIST);
			ListView_SetExtendedListViewStyle(
				hwndList,
				LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_LABELTIP | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
			SetListViewTooltipsTopMost(hwndList);

			const HDC hdc = ::GetDC(hwndList);
			const HFONT hfont = GetWindowFont(hwndList);
			const HFONT hfontOld = SelectFont(hdc, hfont);
			TEXTMETRIC tm;
			::GetTextMetrics(hdc, &tm);
			SelectFont(hdc, hfontOld);
			::ReleaseDC(hwndList, hdc);
			const int FontSize = tm.tmHeight - tm.tmInternalLeading;

			LVCOLUMN lvc;
			lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			lvc.fmt = LVCFMT_LEFT;
			lvc.cx = 10 * FontSize;
			lvc.pszText = const_cast<LPTSTR>(TEXT("名前"));
			ListView_InsertColumn(hwndList, COLUMN_NAME, &lvc);
			lvc.cx = 6 * FontSize;
			lvc.pszText = const_cast<LPTSTR>(TEXT("種別"));
			ListView_InsertColumn(hwndList, COLUMN_SERVICETYPE, &lvc);
			lvc.fmt = LVCFMT_RIGHT;
			lvc.cx = 6 * FontSize;
			lvc.pszText = const_cast<LPTSTR>(TEXT("チャンネル"));
			ListView_InsertColumn(hwndList, COLUMN_CHANNELNAME, &lvc);
			lvc.cx = 6 * FontSize;
			lvc.pszText = const_cast<LPTSTR>(TEXT("サービスID"));
			ListView_InsertColumn(hwndList, COLUMN_SERVICEID, &lvc);
			lvc.cx = 7 * FontSize;
			lvc.pszText = const_cast<LPTSTR>(TEXT("リモコンキー"));
			ListView_InsertColumn(hwndList, COLUMN_REMOTECONTROLKEYID, &lvc);
			lvc.cx = 4 * FontSize;
			lvc.pszText = const_cast<LPTSTR>(TEXT("インデックス"));
			ListView_InsertColumn(hwndList, COLUMN_CHANNELINDEX, &lvc);
			if (NumSpaces > 0) {
				//SetChannelList(hDlg, m_ScanSpace);
				::SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_CHANNELSCAN_SPACE, CBN_SELCHANGE), 0);
				/*
				for (int i = 0; i < NUM_COLUMNS; i++)
					ListView_SetColumnWidth(hwndList, i, LVSCW_AUTOSIZE_USEHEADER);
				*/
			}

			DlgCheckBox_Check(hDlg, IDC_CHANNELSCAN_SCANSERVICE, true);
			DlgCheckBox_Check(hDlg, IDC_CHANNELSCAN_IGNORESIGNALLEVEL, m_fIgnoreSignalLevel);

			if (CoreEngine.IsNetworkDriver())
				EnableDlgItems(hDlg, IDC_CHANNELSCAN_FIRST, IDC_CHANNELSCAN_LAST, false);

			AddControls({
				{IDC_CHANNELSCAN_CHANNELLIST,       AlignFlag::All},
				{IDC_CHANNELSCAN_SCANSERVICE,       AlignFlag::Bottom},
				{IDC_CHANNELSCAN_IGNORESIGNALLEVEL, AlignFlag::Bottom},
				{IDC_CHANNELSCAN_SETTINGS,          AlignFlag::Bottom},
				{IDC_CHANNELSCAN_LOADPRESET,        AlignFlag::BottomRight},
				{IDC_CHANNELSCAN_START,             AlignFlag::BottomRight},
			});
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CHANNELSCAN_SPACE:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				const int Space = static_cast<int>(DlgComboBox_GetCurSel(hDlg, IDC_CHANNELSCAN_SPACE));
				if (Space < 0)
					return TRUE;

				m_ScanSpace = Space;
				LPCTSTR pszName = GetAppClass().CoreEngine.GetFilter<LibISDB::BonDriverSourceFilter>()->GetSpaceName(Space);
				bool fBS = false, fCS = false;
				if (pszName != nullptr) {
					fBS = ::StrStrI(pszName, TEXT("BS")) != nullptr;
					fCS = ::StrStrI(pszName, TEXT("CS")) != nullptr;
				}
				CTuningSpaceInfo *pTuningSpaceInfo = m_TuningSpaceList.GetTuningSpaceInfo(Space);
				CChannelList *pChannelList = nullptr;
				if (pTuningSpaceInfo != nullptr)
					pChannelList = pTuningSpaceInfo->GetChannelList();
				if (fBS != fCS && pChannelList != nullptr && pChannelList->NumChannels() == 0) {
					SetPreset(true);
				} else {
					SetChannelList(Space);
				}
				if (fBS || fCS || (pChannelList != nullptr && pChannelList->HasMultiService()))
					DlgCheckBox_Check(hDlg, IDC_CHANNELSCAN_SCANSERVICE, true);
				EnableDlgItem(hDlg, IDC_CHANNELSCAN_LOADPRESET, fBS || fCS);
				m_SortColumn = -1;
				SetListViewSortMark(::GetDlgItem(hDlg, IDC_CHANNELSCAN_CHANNELLIST), -1);
			}
			return TRUE;

		case IDC_CHANNELSCAN_SETTINGS:
			{
				CScanSettingsDialog SettingsDlg(this);

				SettingsDlg.Show(::GetParent(hDlg));
			}
			return TRUE;

		case IDC_CHANNELSCAN_LOADPRESET:
			SetPreset(false);
			return TRUE;

		case IDC_CHANNELSCAN_START:
			{
				const int Space = static_cast<int>(DlgComboBox_GetCurSel(hDlg, IDC_CHANNELSCAN_SPACE));

				if (Space >= 0) {
					const LibISDB::BonDriverSourceFilter *pSourceFilter =
						GetAppClass().CoreEngine.GetFilter<LibISDB::BonDriverSourceFilter>();
					if (pSourceFilter->GetChannelName(Space, 0) == nullptr) {
						::MessageBox(
							hDlg,
							TEXT("このチューニング空間にはチャンネルがありません。\n")
							TEXT("正しいチューニング空間が選択されているか、\n")
							TEXT("チューナーが正しく設定されているか確認してください。"),
							TEXT("チャンネルスキャン"),
							MB_OK | MB_ICONEXCLAMATION);
						return TRUE;
					}

					const HWND hwndList = ::GetDlgItem(hDlg, IDC_CHANNELSCAN_CHANNELLIST);

					if (GetAppClass().UICore.IsViewerEnabled()) {
						GetAppClass().UICore.EnableViewer(false);
						m_fRestorePreview = true;
					}
					m_ScanSpace = Space;
					m_ScanChannel = 0;
					m_fScanService =
						DlgCheckBox_IsChecked(hDlg, IDC_CHANNELSCAN_SCANSERVICE);
					m_fIgnoreSignalLevel =
						DlgCheckBox_IsChecked(hDlg, IDC_CHANNELSCAN_IGNORESIGNALLEVEL);

					ListView_DeleteAllItems(hwndList);

					CScanDialog ScanDialog(this);

					if (ScanDialog.Show(::GetParent(hDlg))) {
						if (ListView_GetItemCount(hwndList) > 0) {
							// 元あったチャンネルで検出されなかったものがある場合、残すか問い合わせる
							CChannelList *pChannelList = m_TuningSpaceList.GetChannelList(Space);
							std::vector<const CChannelInfo*> MissingChannels;
							for (int i = 0; i < pChannelList->NumChannels(); i++) {
								const CChannelInfo *pOldChannel = pChannelList->GetChannelInfo(i);
								if (m_ScanningChannelList.FindByIDs(pOldChannel->GetNetworkID(), 0, pOldChannel->GetServiceID(), false) < 0
										&& m_ScanningChannelList.FindByName(pOldChannel->GetName()) < 0)
									MissingChannels.push_back(pOldChannel);
							}
							if (!MissingChannels.empty()) {
								const int Channels = static_cast<int>(MissingChannels.size());
								TCHAR szMessage[256 + (MAX_CHANNEL_NAME + 2) * 10];
								CStaticStringFormatter Formatter(szMessage, lengthof(szMessage));

								Formatter.AppendFormat(
									TEXT("元あったチャンネルのうち、以下の{}{}のチャンネルが検出されませんでした。\n\n"),
									Channels, Channels < 10 ? TEXT("つ") : TEXT(""));
								for (int i = 0; i < Channels; i++) {
									if (i == 10) {
										Formatter.Append(TEXT("...\n"));
										break;
									}
									Formatter.AppendFormat(TEXT("・{}\n"), MissingChannels[i]->GetName());
								}
								Formatter.Append(TEXT("\n検出されなかったチャンネルを残しますか？"));
								if (::MessageBox(
											hDlg, Formatter.GetString(), TEXT("問い合わせ"),
											MB_YESNO | MB_ICONINFORMATION) == IDYES) {
									bool fServiceType = true;
									for (int i = 0; i < pChannelList->NumChannels(); i++) {
										if (pChannelList->GetChannelInfo(i)->GetServiceType() == 0) {
											fServiceType = false;
											break;
										}
									}

									for (const CChannelInfo *e : MissingChannels) {
										CChannelInfo *pInfo = new CChannelInfo(*e);
										m_ScanningChannelList.AddChannel(pInfo);
										InsertChannelInfo(ListView_GetItemCount(hwndList), pInfo, fServiceType);
									}
								}
							}

							int SortColumn;
							if (m_ScanningChannelList.HasRemoteControlKeyID()
									&& GetAppClass().NetworkDefinition.IsTerrestrialNetworkID(
										m_ScanningChannelList.GetChannelInfo(0)->GetNetworkID()))
								SortColumn = COLUMN_REMOTECONTROLKEYID;
							else
								SortColumn = COLUMN_SERVICEID;
							CChannelListSort ListSort(SortColumn);
							ListSort.Sort(hwndList);
							ListSort.UpdateChannelList(hwndList, m_TuningSpaceList.GetChannelList(Space));
							if (IsStringEmpty(m_TuningSpaceList.GetTuningSpaceName(Space))) {
								m_TuningSpaceList.GetTuningSpaceInfo(Space)->SetName(
									GetAppClass().CoreEngine.GetFilter<LibISDB::BonDriverSourceFilter>()->GetSpaceName(Space));
							}
							m_SortColumn = SortColumn;
							m_fSortDescending = false;
							SetListViewSortMark(hwndList, m_SortColumn, !m_fSortDescending);
							ListView_EnsureVisible(hwndList, 0, FALSE);
						} else {
							// チャンネルが検出できなかった
							String Message;

							Message = TEXT("チャンネルが検出できませんでした。");
							if ((m_fIgnoreSignalLevel
										&& m_MaxSignalLevel < 1.0f)
									|| (!m_fIgnoreSignalLevel
										&& m_MaxSignalLevel < m_SignalLevelThreshold)) {
								Message += TEXT("\n信号レベルが低すぎるか、取得できません。");
								if (!m_fIgnoreSignalLevel
										&& m_MaxBitRate > 8000000)
									Message += TEXT("\n[信号レベルを無視する] をチェックしてスキャンしてみてください。");
							} else if (m_MaxBitRate < 8000000) {
								Message += TEXT("\nストリームを受信できません。");
							}
							::MessageBox(hDlg, Message.c_str(), TEXT("スキャン結果"), MB_OK | MB_ICONEXCLAMATION);
						}
						m_fUpdated = true;
					} else {
						SetChannelList(Space);
					}
				}
			}
			return TRUE;

		case IDC_CHANNELSCAN_PROPERTIES:
			{
				const HWND hwndList = ::GetDlgItem(hDlg, IDC_CHANNELSCAN_CHANNELLIST);
				LVITEM lvi;

				lvi.iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
				if (lvi.iItem >= 0) {
					lvi.mask = LVIF_PARAM;
					lvi.iSubItem = 0;
					ListView_GetItem(hwndList, &lvi);
					CChannelInfo *pChInfo = reinterpret_cast<CChannelInfo*>(lvi.lParam);
					CChannelPropDialog Dialog(pChInfo);
					if (Dialog.Show(hDlg)) {
						lvi.mask = LVIF_TEXT;
						lvi.pszText = const_cast<LPTSTR>(pChInfo->GetName());
						ListView_SetItem(hwndList, &lvi);
						lvi.iSubItem = 4;
						TCHAR szText[16];
						if (pChInfo->GetChannelNo() > 0)
							StringFormat(szText, TEXT("{}"), pChInfo->GetChannelNo());
						else
							szText[0] = '\0';
						lvi.pszText = szText;
						ListView_SetItem(hwndList, &lvi);
						m_fUpdated = true;
					}
				}
			}
			return TRUE;

		case IDC_CHANNELSCAN_DELETE:
			{
				const HWND hwndList = ::GetDlgItem(hDlg, IDC_CHANNELSCAN_CHANNELLIST);
				LVITEM lvi;

				lvi.iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
				if (lvi.iItem >= 0) {
					lvi.mask = LVIF_PARAM;
					lvi.iSubItem = 0;
					ListView_GetItem(hwndList, &lvi);
					const CChannelInfo *pChInfo = reinterpret_cast<CChannelInfo*>(lvi.lParam);
					CChannelList *pList = m_TuningSpaceList.GetChannelList(m_ScanSpace);
					if (pList != nullptr) {
						const int Index = pList->Find(pChInfo);
						if (Index >= 0) {
							ListView_DeleteItem(hwndList, lvi.iItem);
							pList->DeleteChannel(Index);
							m_fUpdated = true;
						}
					}
				}
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case LVN_COLUMNCLICK:
			{
				const NMLISTVIEW *pnmlv = reinterpret_cast<const NMLISTVIEW*>(lParam);

				if (m_SortColumn == pnmlv->iSubItem) {
					m_fSortDescending = !m_fSortDescending;
				} else {
					m_SortColumn = pnmlv->iSubItem;
					m_fSortDescending = false;
				}
				CChannelListSort ListSort(pnmlv->iSubItem, m_fSortDescending);
				ListSort.Sort(pnmlv->hdr.hwndFrom);
				ListSort.UpdateChannelList(pnmlv->hdr.hwndFrom, m_TuningSpaceList.GetChannelList(m_ScanSpace));
				SetListViewSortMark(pnmlv->hdr.hwndFrom, pnmlv->iSubItem, !m_fSortDescending);
			}
			return TRUE;

		case NM_RCLICK:
			if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == ::GetDlgItem(hDlg, IDC_CHANNELSCAN_CHANNELLIST)) {
				const NMITEMACTIVATE *pnmia = reinterpret_cast<const NMITEMACTIVATE*>(lParam);

				if (pnmia->iItem >= 0) {
					CPopupMenu Menu(GetAppClass().GetResourceInstance(), IDM_CHANNELSCAN);

					Menu.Show(hDlg);
				}
			}
			break;

		case NM_DBLCLK:
			if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == ::GetDlgItem(hDlg, IDC_CHANNELSCAN_CHANNELLIST)) {
				const NMITEMACTIVATE *pnmia = reinterpret_cast<const NMITEMACTIVATE*>(lParam);

				if (pnmia->iItem >= 0)
					::SendMessage(hDlg, WM_COMMAND, IDC_CHANNELSCAN_PROPERTIES, 0);
			}
			break;

		case LVN_ENDLABELEDIT:
			{
				NMLVDISPINFO *plvdi = reinterpret_cast<NMLVDISPINFO*>(lParam);
				BOOL fResult;

				if (plvdi->item.pszText != nullptr && plvdi->item.pszText[0] != '\0') {
					LVITEM lvi;

					lvi.mask = LVIF_PARAM;
					lvi.iItem = plvdi->item.iItem;
					lvi.iSubItem = 0;
					ListView_GetItem(plvdi->hdr.hwndFrom, &lvi);
					CChannelInfo *pChInfo = reinterpret_cast<CChannelInfo*>(lvi.lParam);
					pChInfo->SetName(plvdi->item.pszText);
					m_fUpdated = true;
					fResult = TRUE;
				} else {
					fResult = FALSE;
				}
				::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, fResult);
			}
			return TRUE;

		case LVN_ITEMCHANGED:
			if (!m_fChanging) {
				LPNMLISTVIEW pnmlv = reinterpret_cast<LPNMLISTVIEW>(lParam);

				if (pnmlv->iItem >= 0
						&& (pnmlv->uNewState & LVIS_STATEIMAGEMASK) != (pnmlv->uOldState & LVIS_STATEIMAGEMASK)) {
					LVITEM lvi;

					lvi.mask = LVIF_PARAM;
					lvi.iItem = pnmlv->iItem;
					lvi.iSubItem = 0;
					ListView_GetItem(pnmlv->hdr.hwndFrom, &lvi);
					CChannelInfo *pChInfo = reinterpret_cast<CChannelInfo*>(lvi.lParam);
					pChInfo->Enable((pnmlv->uNewState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK(2));
					m_fUpdated = true;
				}
			}
			return TRUE;

		case PSN_APPLY:
			{
				CAppMain &App = GetAppClass();

				if (m_fUpdated) {
					m_TuningSpaceList.MakeAllChannelList();
					TCHAR szFileName[MAX_PATH];
					if (!App.ChannelManager.GetChannelFileName(szFileName, lengthof(szFileName))
							|| ::lstrcmpi(::PathFindExtension(szFileName), CHANNEL_FILE_EXTENSION) != 0
							|| !::PathFileExists(szFileName)) {
						App.CoreEngine.GetDriverPath(szFileName, lengthof(szFileName));
						::PathRenameExtension(szFileName, CHANNEL_FILE_EXTENSION);
					}
					const bool fOK = m_TuningSpaceList.SaveToFile(szFileName);
					if (fOK) {
						App.AddLog(TEXT("チャンネルファイルを \"{}\" に保存しました。"), szFileName);
					} else {
						TCHAR szText[32 + MAX_PATH];
						StringFormat(
							szText,
							TEXT("チャンネルファイル \"{}\" を保存できません。"), szFileName);
						App.AddLogRaw(CLogItem::LogType::Error, szText);
						::MessageBox(hDlg, szText, nullptr, MB_OK | MB_ICONEXCLAMATION);
					}
					SetUpdateFlag(UPDATE_CHANNELLIST);

					if (fOK) {
						LPTSTR pSequence = ::PathFindExtension(szFileName);
						if (pSequence > szFileName && *(--pSequence) == _T('0')) {
							StringCopy(pSequence + 1, TEXT(".dll"));
							unsigned int Exists = 0;
							for (int i = 1; i <= 9; i++) {
								*pSequence = _T('0') + i;
								if (::PathFileExists(szFileName))
									Exists |= 1U << i;
							}
							if (Exists != 0) {
								LPCTSTR pszName = ::PathFindFileName(szFileName);
								TCHAR szText[256 + MAX_PATH * 10];
								CStaticStringFormatter Formatter(szText, lengthof(szText));

								*pSequence = _T('0');
								Formatter.AppendFormat(TEXT("{} のチャンネルスキャン結果を\n以下の BonDriver にも反映させますか？\n\n"), pszName);
								for (int i = 1; i <= 9; i++) {
									if ((Exists & (1U << i)) != 0) {
										*pSequence = _T('0') + i;
										Formatter.AppendFormat(TEXT("・{}\n"), pszName);
									}
								}
								if (::MessageBox(hDlg, Formatter.GetString(), TEXT("チャンネルスキャン"), MB_YESNO | MB_ICONINFORMATION) == IDYES) {
									for (int i = 1; i <= 9; i++) {
										if ((Exists & (1U << i)) != 0) {
											::PathRenameExtension(szFileName, CHANNEL_FILE_EXTENSION);
											*pSequence = _T('0') + i;
											if (m_TuningSpaceList.SaveToFile(szFileName)) {
												App.AddLog(TEXT("チャンネルファイルを \"{}\" に保存しました。"), szFileName);
												::PathRenameExtension(szFileName, TEXT(".dll"));
												App.Core.UpdateChannelList(szFileName, &m_TuningSpaceList);
											} else {
												StringFormat(
													szText,
													TEXT("チャンネルファイル \"{}\" を保存できません。"), szFileName);
												App.AddLogRaw(CLogItem::LogType::Error, szText);
												if (::MessageBox(hDlg, szText, nullptr, MB_OKCANCEL | MB_ICONEXCLAMATION) != IDOK)
													break;
											}
										}
									}
								}
							}
						}
					}
				}

				if (m_fRestorePreview)
					SetUpdateFlag(UPDATE_PREVIEW);

				m_fIgnoreSignalLevel =
					DlgCheckBox_IsChecked(hDlg, IDC_CHANNELSCAN_IGNORESIGNALLEVEL);

				m_fChanged = true;
			}
			return TRUE;

		case PSN_RESET:
			if (m_fRestorePreview)
				//SetUpdateFlag(UPDATE_PREVIEW);
				GetAppClass().UICore.EnableViewer(true);
			return TRUE;
		}
		break;

	case WM_APP_CHANNELFOUND:
		{
			CChannelInfo *pChInfo = reinterpret_cast<CChannelInfo*>(lParam);

			// 元の有効/無効状態を反映
			const CChannelList *pOldChList = m_TuningSpaceList.GetChannelList(m_ScanSpace);
			const int OldChIndex = pOldChList->FindByIDs(pChInfo->GetNetworkID(), 0, pChInfo->GetServiceID(), false);
			if (OldChIndex >= 0)
				pChInfo->Enable(pOldChList->IsEnabled(OldChIndex));

			const HWND hwndList = ::GetDlgItem(hDlg, IDC_CHANNELSCAN_CHANNELLIST);
			const int Index = ListView_GetItemCount(hwndList);

			m_fChanging = true;
			InsertChannelInfo(Index, pChInfo, true);
			ListView_EnsureVisible(hwndList, Index, FALSE);
			m_fChanging = false;
			::UpdateWindow(hwndList);
		}
		return TRUE;
	}

	return FALSE;
}


INT_PTR CChannelScan::ScanDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static constexpr UINT TIMER_ID_STATISTICS = 1;

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			const LibISDB::BonDriverSourceFilter *pSourceFilter =
				GetAppClass().CoreEngine.GetFilter<LibISDB::BonDriverSourceFilter>();
			m_BonDriverChannelList.clear();
			if (m_ScanChannel > 0)
				m_BonDriverChannelList.resize(m_ScanChannel);
			LPCTSTR pszName;
			for (int i = m_ScanChannel; (pszName = pSourceFilter->GetChannelName(m_ScanSpace, i)) != nullptr; i++) {
				m_BonDriverChannelList.emplace_back(pszName);
			}

			m_ChannelSignalLevel.clear();
			m_ChannelSignalLevel.reserve(m_BonDriverChannelList.size());

			::SendDlgItemMessage(
				hDlg, IDC_CHANNELSCAN_PROGRESS, PBM_SETRANGE32,
				m_ScanChannel, m_BonDriverChannelList.size());
			::SendDlgItemMessage(
				hDlg, IDC_CHANNELSCAN_PROGRESS, PBM_SETPOS,
				m_ScanChannel, 0);

			GetAppClass().Core.BeginChannelScan(m_ScanSpace);

			m_fCancelled = false;
			m_hCancelEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
			m_hScanThread = reinterpret_cast<HANDLE>(::_beginthreadex(nullptr, 0, ScanProc, this, 0, nullptr));
			m_fScaned = true;

			::SetTimer(hDlg, TIMER_ID_STATISTICS, 1000, nullptr);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL:
			{
				m_fCancelled = LOWORD(wParam) == IDCANCEL;
				::SetEvent(m_hCancelEvent);
				EnableDlgItem(hDlg, IDOK, false);
				EnableDlgItem(hDlg, IDCANCEL, false);
			}
			return TRUE;
		}
		return TRUE;

	case WM_TIMER:
		if (wParam == TIMER_ID_STATISTICS) {
			const CCoreEngine &CoreEngine = GetAppClass().CoreEngine;
			TCHAR szText[64], szSignalLevel[32], szBitRate[32];

			CoreEngine.GetSignalLevelText(szSignalLevel, lengthof(szSignalLevel));
			CoreEngine.GetBitRateText(szBitRate, lengthof(szBitRate));
			StringFormat(szText, TEXT("{} / {}"), szSignalLevel, szBitRate);
			::SetDlgItemText(hDlg, IDC_CHANNELSCAN_LEVEL, szText);
		}
		return TRUE;

	case WM_DRAWITEM:
		if (wParam == IDC_CHANNELSCAN_GRAPH) {
			const DRAWITEMSTRUCT *pdis = reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);

			::FillRect(pdis->hDC, &pdis->rcItem, static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
			if (!m_BonDriverChannelList.empty()) {
				const int ChannelCount = static_cast<int>(m_BonDriverChannelList.size());
				const int CellWidth = (pdis->rcItem.right - pdis->rcItem.left - 5) / ChannelCount;
				const float SignalScale = std::max(m_MaxSignalLevel, 30.0f);
				RECT rcGraph;
				rcGraph.left = ((pdis->rcItem.right - pdis->rcItem.left) - ChannelCount * CellWidth) / 2;
				rcGraph.right = rcGraph.left + ChannelCount * CellWidth;
				rcGraph.top = pdis->rcItem.top + 2;
				rcGraph.bottom = pdis->rcItem.bottom - 2;

				const HPEN hpenGrid = ::CreatePen(PS_SOLID, 1, RGB(96, 96, 96));
				const HGDIOBJ hOldPen = ::SelectObject(pdis->hDC, hpenGrid);
				const HGDIOBJ hOldBrush = ::SelectObject(pdis->hDC, ::GetStockObject(NULL_BRUSH));

				for (int i = 0; i < ChannelCount; i++) {
					const int x = rcGraph.left + i * CellWidth;
					::MoveToEx(pdis->hDC, x, rcGraph.top, nullptr);
					::LineTo(pdis->hDC, x, rcGraph.bottom);

					if (i < static_cast<int>(m_ChannelSignalLevel.size())) {
						const float SignalLevel = m_ChannelSignalLevel[i];
						if (SignalLevel >= 0.0f) {
							RECT rcFill;
							rcFill.left = x + 1;
							rcFill.right = x + CellWidth;
							rcFill.top = rcGraph.bottom - 3 - static_cast<int>(SignalLevel * static_cast<float>(rcGraph.bottom - rcGraph.top - 4) / SignalScale);
							rcFill.bottom = rcGraph.bottom - 1;
							DrawUtil::Fill(pdis->hDC, &rcFill, HSVToRGB(0.6 - SignalLevel / SignalScale * 0.6, 0.8, 1.0));
						}
					}
				}

				::Rectangle(pdis->hDC, rcGraph.left, rcGraph.top, rcGraph.right + 1, rcGraph.bottom);

				::SelectObject(pdis->hDC, hOldBrush);
				::SelectObject(pdis->hDC, hOldPen);
				::DeleteObject(hpenGrid);
			}
		}
		return TRUE;

	case WM_APP_BEGINSCAN:
		{
			const int CurChannel = static_cast<int>(wParam);
			const int NumChannels = static_cast<int>(m_BonDriverChannelList.size());
			unsigned int EstimateRemain = (NumChannels - CurChannel) * m_ScanWait;
			TCHAR szText[80];

			if (m_fIgnoreSignalLevel)
				EstimateRemain += (NumChannels - CurChannel) * m_RetryCount * m_RetryInterval;
			EstimateRemain = (EstimateRemain + 500) / 1000;
			StringFormat(
				szText,
				TEXT("チャンネル {}/{} をスキャンしています... (残り時間 {}:{:02})"),
				CurChannel + 1, NumChannels,
				EstimateRemain / 60, EstimateRemain % 60);
			::SetDlgItemText(hDlg, IDC_CHANNELSCAN_INFO, szText);
			::SetDlgItemText(hDlg, IDC_CHANNELSCAN_CHANNEL, m_BonDriverChannelList[CurChannel].c_str());
			::SendDlgItemMessage(hDlg, IDC_CHANNELSCAN_PROGRESS, PBM_SETPOS, wParam, 0);
			GetAppClass().UICore.SetProgress(CurChannel, NumChannels);
		}
		return TRUE;

	case WM_APP_CHANNELFOUND:
		{
			//const int ScanChannel = LOWORD(wParam), Service = HIWORD(wParam);
			//CChannelInfo *pChannelInfo = reinterpret_cast<CChannelInfo*>(lParam);

			::SendMessage(m_hDlg, WM_APP_CHANNELFOUND, 0, lParam);
		}
		return TRUE;

	case WM_APP_ENDCHANNEL:
		{
			while (static_cast<int>(m_ChannelSignalLevel.size()) < static_cast<int>(wParam))
				m_ChannelSignalLevel.push_back(-1.0f);
			m_ChannelSignalLevel.push_back(static_cast<float>(lParam) / 100.0f);
			InvalidateDlgItem(hDlg, IDC_CHANNELSCAN_GRAPH, false);
		}
		return TRUE;

	case WM_APP_ENDSCAN:
		{
			const ScanResult Result = static_cast<ScanResult>(wParam);

			::WaitForSingleObject(m_hScanThread, INFINITE);

			//GetAppClass().Core.EndChannelScan();
			GetAppClass().UICore.EndProgress();

			if (m_fCancelled) {
				::EndDialog(hDlg, IDCANCEL);
			} else {
				CMessageDialog Dialog;

				if (Result == ScanResult::SetChannelPartiallyFailed) {
					const int ChannelCount = LOWORD(lParam);
					const int ErrorCount = HIWORD(lParam);
					String Message;
					if (ErrorCount < ChannelCount) {
						StringFormat(
							&Message,
							TEXT("{}チャンネルのうち、{}回のチャンネル変更が BonDriver に受け付けられませんでした。\n")
							TEXT("(受信できるチャンネルが全てスキャンできていれば問題はありません)"),
							ChannelCount, ErrorCount);
						Dialog.Show(hDlg, CMessageDialog::MessageType::Info, Message.c_str(), nullptr, nullptr, TEXT("お知らせ"));
					} else {
						Dialog.Show(
							hDlg, CMessageDialog::MessageType::Warning,
							TEXT("チャンネル変更の要求が BonDriver に受け付けられないため、スキャンを行えませんでした。"));
					}
				} else if (Result == ScanResult::SetChannelTimeout) {
					Dialog.Show(
						hDlg, CMessageDialog::MessageType::Warning,
						TEXT("タイムアウトのためチャンネル変更ができません。"));
				}

				::EndDialog(hDlg, IDOK);
			}
		}
		return TRUE;

	case WM_DESTROY:
		{
			if (m_hScanThread != nullptr) {
				::SetEvent(m_hCancelEvent);
				if (::WaitForSingleObject(m_hScanThread, 30000) == WAIT_TIMEOUT) {
					GetAppClass().AddLog(CLogItem::LogType::Warning, TEXT("チャンネルスキャンスレッドを強制終了します。"));
					::TerminateThread(m_hScanThread, -1);
				}
				::CloseHandle(m_hScanThread);
				m_hScanThread = nullptr;
			}
			if (m_hCancelEvent != nullptr) {
				::CloseHandle(m_hCancelEvent);
				m_hCancelEvent = nullptr;
			}
		}
		return TRUE;
	}

	return FALSE;
}


unsigned int __stdcall CChannelScan::ScanProc(LPVOID lpParameter)
{
	CChannelScan *pThis = static_cast<CChannelScan*>(lpParameter);

	pThis->Scan();

	return 0;
}


void CChannelScan::Scan()
{
	CAppMain &App = GetAppClass();
	const LibISDB::AnalyzerFilter *pAnalyzer =
		App.CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
	LibISDB::BonDriverSourceFilter *pSource =
		App.CoreEngine.GetFilter<LibISDB::BonDriverSourceFilter>();

	ScanResult Result = ScanResult::Cancelled;
	int SetChannelCount = 0, SetChannelErrorCount = 0;

	m_ScanningChannelList.Clear();
	m_MaxSignalLevel = 0.0f;
	m_ChannelMaxSignalLevel = 0.0f;
	m_MaxBitRate = 0;

	const bool fPurgeStream = pSource->IsPurgeStreamOnChannelChange();
	if (!fPurgeStream)
		pSource->SetPurgeStreamOnChannelChange(true);

	for (;; m_ScanChannel++) {
		if (m_ScanChannel >= static_cast<int>(m_BonDriverChannelList.size())) {
			if (Result == ScanResult::Cancelled)
				Result = ScanResult::Succeeded;
			break;
		}

		::PostMessage(m_hScanDlg, WM_APP_BEGINSCAN, m_ScanChannel, 0);

		SetChannelCount++;
		App.CoreEngine.SetServiceSelectInfo(nullptr);
		if (!pSource->SetChannelAndPlay(m_ScanSpace, m_ScanChannel)) {
			SetChannelErrorCount++;
			if (pSource->GetLastErrorCode().category() == std::generic_category()
					&& static_cast<std::errc>(pSource->GetLastErrorCode().value()) == std::errc::timed_out) {
				Result = ScanResult::SetChannelTimeout;
				break;
			} else {
				Result = ScanResult::SetChannelPartiallyFailed;
			}
			if (::WaitForSingleObject(m_hCancelEvent, 0) != WAIT_TIMEOUT)
				break;
			continue;
		}

		if (::WaitForSingleObject(m_hCancelEvent, std::min(m_ScanWait, 2000U)) != WAIT_TIMEOUT)
			break;
		if (m_ScanWait > 2000U) {
			DWORD Wait = m_ScanWait - 2000;
			while (true) {
				// SDTが来たら待ち時間終了
				if (pAnalyzer->IsSDTUpdated())
					break;
				if (::WaitForSingleObject(m_hCancelEvent, std::min<DWORD>(Wait, 1000)) != WAIT_TIMEOUT)
					goto End;
				if (Wait <= 1000)
					break;
				Wait -= 1000;
			}
		}

		bool fScanService = m_fScanService;
		bool fFound = false;
		LibISDB::AnalyzerFilter::SDTServiceList ServiceList;
		LibISDB::String Name;

		m_ChannelMaxSignalLevel = 0.0f;
		const float SignalLevel = GetSignalLevel();

		if (pAnalyzer->IsSDTUpdated()
				|| m_fIgnoreSignalLevel
				|| SignalLevel >= m_SignalLevelThreshold) {
			for (int i = 0; i <= m_RetryCount; i++) {
				if (i > 0) {
					if (::WaitForSingleObject(m_hCancelEvent, m_RetryInterval) != WAIT_TIMEOUT)
						goto End;
					GetSignalLevel();
				}
				if (pAnalyzer->IsSDTUpdated()
						&& pAnalyzer->GetSDTServiceList(&ServiceList)
						&& !ServiceList.empty()) {
					if (fScanService) {
						if (pAnalyzer->GetServiceCount() > 0) {
							// サービスのPMTが揃ったら完了
							// (サービスが視聴可能かどうかPMTの情報で判定するため)
							size_t i;
							for (i = 0; i < ServiceList.size(); i++) {
								if (IsScanService(ServiceList[i])) {
									const int ServiceIndex = pAnalyzer->GetServiceIndexByID(ServiceList[i].ServiceID);
									if (ServiceIndex >= 0 && !pAnalyzer->IsServicePMTAcquired(ServiceIndex))
										break;
								}
							}
							if (i == ServiceList.size()) {
								fFound = true;
								break;
							}
						}
						// 時間内にPMTが来ない場合、PMTの情報なしで継続する
						if (!fFound && i == m_RetryCount) {
							fFound = true;
							break;
						}
					} else {
						if (pAnalyzer->IsNITUpdated()) {
							if (pAnalyzer->GetTSName(&Name)) {
								fFound = true;
								break;
							} else {
								// BS/CS の場合はサービスの検索を有効にする
								const WORD NetworkID = pAnalyzer->GetNetworkID();
								if (App.NetworkDefinition.IsSatelliteNetworkID(NetworkID))
									fScanService = true;
							}
						}
					}
				}
			}
		}
		if (fFound) {
			const WORD TransportStreamID = pAnalyzer->GetTransportStreamID();
			const WORD NetworkID = pAnalyzer->GetNetworkID();

			// 見付かったチャンネルを追加する
			if (fScanService) {
				int ServiceCount = 0;
				for (size_t i = 0; i < ServiceList.size(); i++) {
					const LibISDB::AnalyzerFilter::SDTServiceInfo &ServiceInfo = ServiceList[i];

					if (!IsScanService(ServiceInfo))
						continue;

					const WORD ServiceID = ServiceInfo.ServiceID;

					// 視聴不可のサービスを除外
					const int ServiceIndex = pAnalyzer->GetServiceIndexByID(ServiceID);
					if (ServiceIndex >= 0
							&& pAnalyzer->IsServicePMTAcquired(ServiceIndex)) {
						if (!App.CoreEngine.IsSelectableService(ServiceIndex))
							continue;
					} else {
						if (ServiceInfo.ServiceType == LibISDB::SERVICE_TYPE_PROMOTION_VIDEO)
							continue;
					}
					if (ServiceInfo.ServiceType == LibISDB::SERVICE_TYPE_DATA) {
						if (pAnalyzer->Is1SegService(ServiceIndex)) {
							if (!m_fDetect1SegService)
								continue;
						} else {
							if (!m_fDetectDataService)
								continue;
						}
					}

					int RemoteControlKeyID = pAnalyzer->GetRemoteControlKeyID();
					if (RemoteControlKeyID == 0) {
						RemoteControlKeyID =
							App.NetworkDefinition.GetRemoteControlKeyID(NetworkID, ServiceID);
					}

					String Name = ServiceInfo.ServiceName;
					// ハイフンの代わりにマイナス記号(U+2212)が使われている局がある
					StringUtility::Replace(Name, L'\u2212', L'\uff0d');

					CChannelInfo *pChInfo =
						new CChannelInfo(
							m_ScanSpace, m_ScanChannel,
							RemoteControlKeyID,
							Name.c_str());
					pChInfo->SetNetworkID(NetworkID);
					pChInfo->SetTransportStreamID(TransportStreamID);
					pChInfo->SetServiceID(ServiceID);
					pChInfo->SetServiceType(ServiceInfo.ServiceType);

					// 一部サービスはデフォルトで無効にする
					if ((ServiceInfo.ServiceType != LibISDB::SERVICE_TYPE_DIGITAL_TV
								&& ServiceInfo.ServiceType != LibISDB::SERVICE_TYPE_PROMOTION_VIDEO
								&& ServiceInfo.ServiceType != LibISDB::SERVICE_TYPE_4K_TV
#ifdef TVTEST_FOR_1SEG
								&& ServiceInfo.ServiceType != LibISDB::SERVICE_TYPE_DATA
#endif
								&& ServiceInfo.ServiceType != LibISDB::SERVICE_TYPE_DIGITAL_AUDIO
							)
							// BSのサブチャンネル
							|| (App.NetworkDefinition.IsBSNetworkID(NetworkID) && ServiceID < 190 && ServiceCount > 0)
							// 地デジのサブチャンネル
							|| (NetworkID == TransportStreamID && ServiceCount > 0)
						)
						pChInfo->Enable(false);

					TRACE(
						TEXT("Channel found [{:2}][{:2}] : {} NID {:#04x} TSID {:#04x} SID {:#04x}\n"),
						m_ScanChannel, i, Name,
						NetworkID, TransportStreamID, ServiceID);

					m_ScanningChannelList.AddChannel(pChInfo);
					ServiceCount++;

					::PostMessage(m_hScanDlg, WM_APP_CHANNELFOUND, m_ScanChannel, reinterpret_cast<LPARAM>(pChInfo));
				}
			} else {
				CChannelInfo *pChInfo =
					new CChannelInfo(
						m_ScanSpace, m_ScanChannel,
						pAnalyzer->GetRemoteControlKeyID(), Name.c_str());
				pChInfo->SetNetworkID(NetworkID);
				pChInfo->SetTransportStreamID(TransportStreamID);

				for (auto it = ServiceList.begin(); it != ServiceList.end(); ++it) {
					if (IsScanService(*it)
							&& (
#ifdef TVTEST_FOR_1SEG
								it->ServiceType == LibISDB::SERVICE_TYPE_DATA
#else
								it->ServiceType == LibISDB::SERVICE_TYPE_DIGITAL_TV
								|| it->ServiceType == LibISDB::SERVICE_TYPE_4K_TV
#endif
								|| it->ServiceType == LibISDB::SERVICE_TYPE_DIGITAL_AUDIO)) {
						pChInfo->SetServiceID(it->ServiceID);
						pChInfo->SetServiceType(it->ServiceType);
						break;
					}
				}

				TRACE(
					TEXT("Channel found [{:2}] : {} NID {:#04x} TSID {:#04x} SID {:#04x}\n"),
					m_ScanChannel, Name.c_str(), TransportStreamID, NetworkID, pChInfo->GetServiceID());

				m_ScanningChannelList.AddChannel(pChInfo);

				::PostMessage(
					m_hScanDlg, WM_APP_CHANNELFOUND,
					m_ScanChannel, reinterpret_cast<LPARAM>(pChInfo));
			}
		}
#ifdef _DEBUG
		else {
			TRACE(
				TEXT("Channel scan [{:2}] Service not found.\n"),
				m_ScanChannel);
		}
#endif

		::PostMessage(m_hScanDlg, WM_APP_ENDCHANNEL, m_ScanChannel, static_cast<LPARAM>(m_ChannelMaxSignalLevel * 100.0f));
	}

End:
	if (!fPurgeStream)
		pSource->SetPurgeStreamOnChannelChange(false);

	::PostMessage(
		m_hScanDlg, WM_APP_ENDSCAN,
		static_cast<WPARAM>(Result), MAKELPARAM(SetChannelCount, SetChannelErrorCount));
}


float CChannelScan::GetSignalLevel()
{
	const LibISDB::BonDriverSourceFilter *pSource =
		GetAppClass().CoreEngine.GetFilter<LibISDB::BonDriverSourceFilter>();

	const float SignalLevel = pSource->GetSignalLevel();
	if (SignalLevel > m_MaxSignalLevel)
		m_MaxSignalLevel = SignalLevel;
	if (SignalLevel > m_ChannelMaxSignalLevel)
		m_ChannelMaxSignalLevel = SignalLevel;

	const DWORD BitRate = pSource->GetBitRate();
	if (BitRate > m_MaxBitRate)
		m_MaxBitRate = BitRate;

	return SignalLevel;
}


bool CChannelScan::IsScanService(const LibISDB::AnalyzerFilter::SDTServiceInfo &ServiceInfo, bool fData) const
{
	return !ServiceInfo.ServiceName.empty()
		&& ServiceInfo.ServiceName.compare(TEXT(" ")) != 0
		&& ServiceInfo.ServiceName.compare(TEXT("　")) != 0
		&& IsScanServiceType(ServiceInfo.ServiceType, fData);
}


bool CChannelScan::IsScanServiceType(BYTE ServiceType, bool fData) const
{
	return ServiceType == LibISDB::SERVICE_TYPE_DIGITAL_TV
		|| ServiceType == LibISDB::SERVICE_TYPE_PROMOTION_VIDEO
		|| ServiceType == LibISDB::SERVICE_TYPE_4K_TV
		|| (m_fDetectAudioService && ServiceType == LibISDB::SERVICE_TYPE_DIGITAL_AUDIO)
		|| (fData && ServiceType == LibISDB::SERVICE_TYPE_DATA);
}




CChannelScan::CScanSettingsDialog::CScanSettingsDialog(CChannelScan *pChannelScan)
	: m_pChannelScan(pChannelScan)
{
}


bool CChannelScan::CScanSettingsDialog::Show(HWND hwndOwner)
{
	return ShowDialog(
		hwndOwner,
		GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE(IDD_CHANNELSCANSETTINGS)) == IDOK;
}


INT_PTR CChannelScan::CScanSettingsDialog::DlgProc(
	HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			TCHAR szText[16];

			StringFormat(
				szText, TEXT("{:.2f}"),
				m_pChannelScan->m_SignalLevelThreshold);
			::SetDlgItemText(hDlg, IDC_CHANNELSCANSETTINGS_SIGNALLEVELTHRESHOLD, szText);

			for (int i = 1; i <= 10; i++) {
				StringFormat(szText, TEXT("{} 秒"), i);
				DlgComboBox_AddString(hDlg, IDC_CHANNELSCANSETTINGS_SCANWAIT, szText);
			}
			DlgComboBox_SetCurSel(
				hDlg, IDC_CHANNELSCANSETTINGS_SCANWAIT,
				m_pChannelScan->m_ScanWait / 1000 - 1);
			for (int i = 0; i <= 10; i++) {
				StringFormat(szText, TEXT("{} 秒"), i);
				DlgComboBox_AddString(hDlg, IDC_CHANNELSCANSETTINGS_RETRYCOUNT, szText);
			}
			DlgComboBox_SetCurSel(
				hDlg, IDC_CHANNELSCANSETTINGS_RETRYCOUNT,
				m_pChannelScan->m_RetryCount);

			DlgCheckBox_Check(
				hDlg, IDC_CHANNELSCANSETTINGS_DETECTDATASERVICE,
				m_pChannelScan->m_fDetectDataService);
			DlgCheckBox_Check(
				hDlg, IDC_CHANNELSCANSETTINGS_DETECT1SEGSERVICE,
				m_pChannelScan->m_fDetect1SegService);
			DlgCheckBox_Check(
				hDlg, IDC_CHANNELSCANSETTINGS_DETECTAUDIOSERVICE,
				m_pChannelScan->m_fDetectAudioService);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CHANNELSCANSETTINGS_HELP:
			GetAppClass().UICore.ShowHelpContent(HELP_ID_CHANNELSCANDETAILS);
			return TRUE;

		case IDOK:
			{
				TCHAR szText[16];
				if (::GetDlgItemText(
							hDlg, IDC_CHANNELSCANSETTINGS_SIGNALLEVELTHRESHOLD,
							szText, lengthof(szText)) > 0) {
					m_pChannelScan->m_SignalLevelThreshold = std::_tcstof(szText, nullptr);
				}

				m_pChannelScan->m_ScanWait =
					(static_cast<unsigned int>(DlgComboBox_GetCurSel(hDlg, IDC_CHANNELSCANSETTINGS_SCANWAIT)) + 1) * 1000;
				m_pChannelScan->m_RetryCount =
					static_cast<int>(DlgComboBox_GetCurSel(hDlg, IDC_CHANNELSCANSETTINGS_RETRYCOUNT));

				m_pChannelScan->m_fDetectDataService =
					DlgCheckBox_IsChecked(hDlg, IDC_CHANNELSCANSETTINGS_DETECTDATASERVICE);
				m_pChannelScan->m_fDetect1SegService =
					DlgCheckBox_IsChecked(hDlg, IDC_CHANNELSCANSETTINGS_DETECT1SEGSERVICE);
				m_pChannelScan->m_fDetectAudioService =
					DlgCheckBox_IsChecked(hDlg, IDC_CHANNELSCANSETTINGS_DETECTAUDIOSERVICE);
			}
			[[fallthrough]];
		case IDCANCEL:
			::EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		return TRUE;
	}

	return FALSE;
}




CChannelScan::CScanDialog::CScanDialog(CChannelScan *pChannelScan)
	: m_pChannelScan(pChannelScan)
{
}


bool CChannelScan::CScanDialog::Show(HWND hwndOwner)
{
	return ShowDialog(
		hwndOwner,
		GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE(IDD_CHANNELSCAN)) == IDOK;
}


INT_PTR CChannelScan::CScanDialog::DlgProc(
	HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		m_pChannelScan->m_hScanDlg = hDlg;
		break;

	case WM_NCDESTROY:
		m_pChannelScan->m_hScanDlg = nullptr;
		break;
	}

	return m_pChannelScan->ScanDlgProc(hDlg, uMsg, wParam, lParam);
}


} // namespace TVTest
