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
#include "StreamInfo.h"
#include "DialogUtil.h"
#include "LibISDB/LibISDB/TS/TSInformation.hpp"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace
{


class CStreamInfoPage
	: public CResizableDialog
{
public:
	~CStreamInfoPage();

	bool Create(HWND hwndOwner) override;

private:
	INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	void SetService();

	static void GetTreeViewText(HWND hwndTree, HTREEITEM hItem, bool fSiblings, String *pText, int Level = 0);
	static void ExpandTreeViewItem(HWND hwndTree, HTREEITEM hItem, unsigned int Flags);
};


CStreamInfoPage::~CStreamInfoPage()
{
	Destroy();
}


bool CStreamInfoPage::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner, GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_STREAMINFO));
}


INT_PTR CStreamInfoPage::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		SetService();

		AddControl(IDC_STREAMINFO_STREAM, AlignFlag::Horz);
		AddControl(IDC_STREAMINFO_NETWORK, AlignFlag::Horz);
		AddControl(IDC_STREAMINFO_SERVICE, AlignFlag::All);
		AddControl(IDC_STREAMINFO_UPDATE, AlignFlag::BottomRight);
		AddControl(IDC_STREAMINFO_COPY, AlignFlag::BottomRight);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_STREAMINFO_UPDATE:
			SetService();
			return TRUE;

		case IDC_STREAMINFO_COPY:
			{
				const HWND hwndTree = ::GetDlgItem(hDlg, IDC_STREAMINFO_SERVICE);
				String Text, Temp;

				GetDlgItemString(hDlg, IDC_STREAMINFO_STREAM, &Temp);
				Text += Temp;
				Text += TEXT("\r\n");
				GetDlgItemString(hDlg, IDC_STREAMINFO_NETWORK, &Temp);
				Text += Temp;
				Text += TEXT("\r\n");
				GetTreeViewText(
					hwndTree, TreeView_GetChild(hwndTree, TreeView_GetRoot(hwndTree)), true, &Text);
				CopyTextToClipboard(GetAppClass().UICore.GetMainWindow(), Text.c_str());
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<NMHDR*>(lParam)->code) {
		case NM_RCLICK:
			{
				const NMHDR *pnmhdr = reinterpret_cast<const NMHDR*>(lParam);
				//const HTREEITEM hItem = TreeView_GetSelection(pnmhdr->hwndFrom);
				const DWORD Pos = ::GetMessagePos();
				TVHITTESTINFO tvhti;
				tvhti.pt.x = static_cast<SHORT>(LOWORD(Pos));
				tvhti.pt.y = static_cast<SHORT>(HIWORD(Pos));
				::ScreenToClient(pnmhdr->hwndFrom, &tvhti.pt);
				const HTREEITEM hItem = TreeView_HitTest(pnmhdr->hwndFrom, &tvhti);
				if (hItem != nullptr) {
					enum {
						COMMAND_COPY = 1,
						COMMAND_EXPAND,
					};
					CPopupMenu Menu;
					Menu.Create();
					Menu.Append(COMMAND_COPY, TEXT("コピー(&C)"));
					Menu.Append(COMMAND_EXPAND, TEXT("ツリーを展開(&X)"));

					switch (Menu.Show(hDlg, nullptr, TPM_RIGHTBUTTON | TPM_RETURNCMD)) {
					case COMMAND_COPY:
						{
							String Text;

							GetTreeViewText(pnmhdr->hwndFrom, hItem, false, &Text);
							if (!Text.empty())
								CopyTextToClipboard(GetAppClass().UICore.GetMainWindow(), Text.c_str());
						}
						break;

					case COMMAND_EXPAND:
						ExpandTreeViewItem(pnmhdr->hwndFrom, hItem, TVE_EXPAND);
						break;
					}
				}
			}
			return TRUE;
		}
		break;
	}

	return FALSE;
}


static void FormatEsInfo(
	LPCTSTR pszName, int Index, const LibISDB::AnalyzerFilter::ESInfo &Es,
	LPTSTR pszText, int MaxText)
{
	const LPCTSTR pszStreamType = LibISDB::GetStreamTypeText(Es.StreamType);
	StringFormat(
		pszText, MaxText,
		TEXT("{}{} : PID {:#04x} ({}) / stream type {:#02x} ({}) / component tag {:#02x}"),
		pszName, Index + 1,
		Es.PID, Es.PID,
		Es.StreamType,
		pszStreamType != nullptr ? pszStreamType : TEXT("?"),
		Es.ComponentTag);
}

void CStreamInfoPage::SetService()
{
	const LibISDB::AnalyzerFilter *pAnalyzer =
		GetAppClass().CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
	if (pAnalyzer == nullptr)
		return;

	TCHAR szText[1024];

	const WORD TSID = pAnalyzer->GetTransportStreamID();
	if (TSID != LibISDB::TRANSPORT_STREAM_ID_INVALID) {
		const size_t Length = StringFormat(szText, TEXT("TSID {0:#04x} ({0})"), TSID);
		String TSName;
		if (pAnalyzer->GetTSName(&TSName)) {
			StringFormat(szText + Length, lengthof(szText) - Length, TEXT(" {}"), TSName);
		}
	} else {
		szText[0] = '\0';
	}
	::SetDlgItemText(m_hDlg, IDC_STREAMINFO_STREAM, szText);

	const WORD NID = pAnalyzer->GetNetworkID();
	if (NID != LibISDB::NETWORK_ID_INVALID) {
		const size_t Length = StringFormat(szText, TEXT("NID {0:#04x} ({0})"), NID);
		String Name;
		if (pAnalyzer->GetNetworkName(&Name)) {
			StringFormat(szText + Length, lengthof(szText) - Length, TEXT(" {}"), Name);
		}
	} else {
		szText[0] = '\0';
	}
	::SetDlgItemText(m_hDlg, IDC_STREAMINFO_NETWORK, szText);

	LibISDB::AnalyzerFilter::ServiceList ServiceList;
	pAnalyzer->GetServiceList(&ServiceList);

	const HWND hwndTree = ::GetDlgItem(m_hDlg, IDC_STREAMINFO_SERVICE);
	TVINSERTSTRUCT tvis;
	HTREEITEM hItem;

	TreeView_DeleteAllItems(hwndTree);

	// サービス一覧
	tvis.hParent = TVI_ROOT;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask = TVIF_STATE | TVIF_TEXT | TVIF_CHILDREN;
	tvis.item.state = TVIS_EXPANDED;
	tvis.item.stateMask = ~0U;
	tvis.item.pszText = const_cast<LPTSTR>(TEXT("サービス"));
	tvis.item.cChildren = !ServiceList.empty() ? 1 : 0;
	hItem = TreeView_InsertItem(hwndTree, &tvis);
	if (hItem != nullptr) {
		for (int i = 0; i < static_cast<int>(ServiceList.size()); i++) {
			const LibISDB::AnalyzerFilter::ServiceInfo &ServiceInfo = ServiceList[i];
			WORD PID;

			tvis.hParent = hItem;
			tvis.item.state = 0;
			tvis.item.cChildren = 1;
			size_t Length = StringFormat(szText, TEXT("サービス{}"), i + 1);
			if (!ServiceInfo.ServiceName.empty())
				Length += StringFormat(
					szText + Length, lengthof(szText) - Length,
					TEXT(" ({})"), ServiceInfo.ServiceName);
			Length += StringFormat(
				szText + Length, lengthof(szText) - Length,
				TEXT(" : SID {0:#04x} ({0})"), ServiceInfo.ServiceID);
			if (ServiceInfo.ServiceType != LibISDB::SERVICE_TYPE_INVALID) {
				StringFormat(
					szText + Length, lengthof(szText) - Length,
					TEXT(" / Type {:#02x}"), ServiceInfo.ServiceType);
			}
			tvis.item.pszText = szText;
			tvis.hParent = TreeView_InsertItem(hwndTree, &tvis);

			tvis.item.cChildren = 0;
			PID = ServiceInfo.PMTPID;
			StringFormat(szText, TEXT("PMT : PID {0:#04x} ({0})"), PID);
			TreeView_InsertItem(hwndTree, &tvis);

			const int NumVideoStreams = static_cast<int>(ServiceInfo.VideoESList.size());
			for (int j = 0; j < NumVideoStreams; j++) {
				FormatEsInfo(
					TEXT("映像"), j, ServiceInfo.VideoESList[j],
					szText, lengthof(szText));
				TreeView_InsertItem(hwndTree, &tvis);
			}

			const int NumAudioStreams = static_cast<int>(ServiceInfo.AudioESList.size());
			for (int j = 0; j < NumAudioStreams; j++) {
				FormatEsInfo(
					TEXT("音声"), j, ServiceInfo.AudioESList[j],
					szText, lengthof(szText));
				TreeView_InsertItem(hwndTree, &tvis);
			}

			const int NumCaptionStreams = static_cast<int>(ServiceInfo.CaptionESList.size());
			for (int j = 0; j < NumCaptionStreams; j++) {
				PID = ServiceInfo.CaptionESList[j].PID;
				StringFormat(
					szText,
					TEXT("字幕{0} : PID {1:#04x} ({1}) / component tag {2:#02x}"),
					j + 1, PID, ServiceInfo.CaptionESList[j].ComponentTag);
				TreeView_InsertItem(hwndTree, &tvis);
			}

			const int NumDataStreams = static_cast<int>(ServiceInfo.DataCarrouselESList.size());
			for (int j = 0; j < NumDataStreams; j++) {
				PID = ServiceInfo.DataCarrouselESList[j].PID;
				StringFormat(
					szText,
					TEXT("データ{0} : PID {1:#04x} ({1}) / component tag {2:#02x}"),
					j + 1, PID, ServiceInfo.DataCarrouselESList[j].ComponentTag);
				TreeView_InsertItem(hwndTree, &tvis);
			}

			const int NumOtherStreams = static_cast<int>(ServiceInfo.OtherESList.size());
			for (int j = 0; j < NumOtherStreams; j++) {
				FormatEsInfo(
					TEXT("その他"), j, ServiceInfo.OtherESList[j],
					szText, lengthof(szText));
				TreeView_InsertItem(hwndTree, &tvis);
			}

			PID = ServiceInfo.PCRPID;
			if (PID != LibISDB::PID_INVALID) {
				StringFormat(szText, TEXT("PCR : PID {0:#04x} ({0})"), PID);
				TreeView_InsertItem(hwndTree, &tvis);
			}

			const int NumEcmStreams = static_cast<int>(ServiceInfo.ECMList.size());
			for (int j = 0; j < NumEcmStreams; j++) {
				PID = ServiceInfo.ECMList[j].PID;
				StringFormat(
					szText,
					TEXT("ECM{0} : PID {1:#04x} ({1}) / CA system ID {2:#02x}"),
					j + 1, PID, ServiceInfo.ECMList[j].CASystemID);
				TreeView_InsertItem(hwndTree, &tvis);
			}
		}
	}

	// チャンネルファイル用フォーマット一覧
	const CChannelInfo *pChannelInfo = GetAppClass().ChannelManager.GetCurrentChannelInfo();
	if (pChannelInfo != nullptr) {
		tvis.hParent = TVI_ROOT;
		tvis.hInsertAfter = TVI_LAST;
		tvis.item.mask = TVIF_STATE | TVIF_TEXT | TVIF_CHILDREN;
		tvis.item.state = TVIS_EXPANDED;
		tvis.item.stateMask = ~0U;
		tvis.item.pszText = const_cast<LPTSTR>(TEXT("チャンネルファイル用フォーマット"));
		tvis.item.cChildren = !ServiceList.empty() ? 1 : 0;;
		hItem = TreeView_InsertItem(hwndTree, &tvis);
		if (hItem != nullptr) {
			const int RemoteControlKeyID = pAnalyzer->GetRemoteControlKeyID();

			for (int i = 0; i < static_cast<int>(ServiceList.size()); i++) {
				const LibISDB::AnalyzerFilter::ServiceInfo &ServiceInfo = ServiceList[i];

				tvis.hParent = hItem;
				tvis.item.state = 0;
				tvis.item.cChildren = 0;
				size_t Length;
				if (!ServiceInfo.ServiceName.empty())
					Length = StringFormat(szText, TEXT("{}"), ServiceInfo.ServiceName);
				else
					Length = StringFormat(szText, TEXT("サービス{}"), i + 1);
				StringFormat(
					szText + Length, lengthof(szText) - Length,
					TEXT(",{},{},{},{},{},{},{}"),
					pChannelInfo->GetSpace(),
					pChannelInfo->GetChannelIndex(),
					RemoteControlKeyID,
					ServiceInfo.ServiceType,
					ServiceInfo.ServiceID, NID, TSID);
				tvis.item.pszText = szText;
				TreeView_InsertItem(hwndTree, &tvis);
			}
		}
	}

	// ネットワークTS一覧
	LibISDB::AnalyzerFilter::NetworkStreamList TsList;
	if (pAnalyzer->GetNetworkStreamList(&TsList) && !TsList.empty()) {
		tvis.hParent = TVI_ROOT;
		tvis.hInsertAfter = TVI_LAST;
		tvis.item.mask = TVIF_STATE | TVIF_TEXT | TVIF_CHILDREN;
		tvis.item.state = 0;
		tvis.item.stateMask = ~0U;
		tvis.item.pszText = const_cast<LPTSTR>(TEXT("ネットワークTS (NIT)"));
		tvis.item.cChildren = 1;
		hItem = TreeView_InsertItem(hwndTree, &tvis);
		if (hItem != nullptr) {
			for (size_t i = 0; i < TsList.size(); i++) {
				const LibISDB::AnalyzerFilter::NetworkStreamInfo &TsInfo = TsList[i];

				if (TsInfo.OriginalNetworkID == NID) {
					StringFormat(
						szText,
						TEXT("TS{0} : TSID {1:#04x} ({1})"),
						i + 1,
						TsInfo.TransportStreamID);
					tvis.hParent = hItem;
					tvis.item.state = 0;
					tvis.item.cChildren = 1;
					tvis.item.pszText = szText;
					tvis.hParent = TreeView_InsertItem(hwndTree, &tvis);
					tvis.item.cChildren = 0;
					for (size_t j = 0; j < TsInfo.ServiceList.size(); j++) {
						const LibISDB::AnalyzerFilter::NetworkServiceInfo &ServiceInfo = TsInfo.ServiceList[j];
						StringFormat(
							szText,
							TEXT("サービス{0} : SID {1:#04x} ({1}) / Type {2:#02x}"),
							j + 1,
							ServiceInfo.ServiceID,
							ServiceInfo.ServiceType);
						TreeView_InsertItem(hwndTree, &tvis);
					}
				}
			}
		}
	}

	// ネットワークサービス一覧
	LibISDB::AnalyzerFilter::SDTStreamList SdtList;
	if (pAnalyzer->GetSDTStreamList(&SdtList) && !SdtList.empty()) {
		tvis.hParent = TVI_ROOT;
		tvis.hInsertAfter = TVI_LAST;
		tvis.item.mask = TVIF_STATE | TVIF_TEXT | TVIF_CHILDREN;
		tvis.item.state = 0;
		tvis.item.stateMask = ~0U;
		tvis.item.pszText = const_cast<LPTSTR>(TEXT("ネットワークサービス (SDT)"));
		tvis.item.cChildren = 1;
		hItem = TreeView_InsertItem(hwndTree, &tvis);
		if (hItem != nullptr) {
			for (size_t i = 0; i < SdtList.size(); i++) {
				const LibISDB::AnalyzerFilter::SDTStreamInfo &TsInfo = SdtList[i];

				if (TsInfo.OriginalNetworkID == NID) {
					StringFormat(
						szText,
						TEXT("TS{0} : TSID {1:#04x} ({1})"),
						i + 1,
						TsInfo.TransportStreamID);
					tvis.hParent = hItem;
					tvis.item.state = 0;
					tvis.item.cChildren = 1;
					tvis.item.pszText = szText;
					tvis.hParent = TreeView_InsertItem(hwndTree, &tvis);
					tvis.item.cChildren = 0;
					for (size_t j = 0; j < TsInfo.ServiceList.size(); j++) {
						const LibISDB::AnalyzerFilter::SDTServiceInfo &ServiceInfo = TsInfo.ServiceList[j];
						StringFormat(
							szText,
							TEXT("サービス{0} ({1}) : SID {2:#04x} ({2}) / Type {3:#02x} / CA {4}"),
							j + 1,
							ServiceInfo.ServiceName,
							ServiceInfo.ServiceID,
							ServiceInfo.ServiceType,
							ServiceInfo.FreeCAMode);
						TreeView_InsertItem(hwndTree, &tvis);
					}
				}
			}
		}
	}

	// ブロードキャスタ情報
	LibISDB::AnalyzerFilter::BITNetworkList BITList;
	if (pAnalyzer->GetBITNetworkList(&BITList) && !BITList.empty()) {
		tvis.hParent = TVI_ROOT;
		tvis.hInsertAfter = TVI_LAST;
		tvis.item.mask = TVIF_STATE | TVIF_TEXT;
		tvis.item.state = 0;
		tvis.item.stateMask = ~0U;
		tvis.item.pszText = const_cast<LPTSTR>(TEXT("ブロードキャスタ (BIT)"));
		hItem = TreeView_InsertItem(hwndTree, &tvis);
		if (hItem != nullptr) {
			for (size_t i = 0; i < BITList.size(); i++) {
				const LibISDB::AnalyzerFilter::BITNetworkInfo &NetworkInfo = BITList[i];

				StringFormat(
					szText,
					TEXT("ネットワーク{0} : ONID {1:#04x} ({1})"),
					i + 1,
					NetworkInfo.OriginalNetworkID);
				tvis.hParent = hItem;
				tvis.item.pszText = szText;
				HTREEITEM hNetworkItem = TreeView_InsertItem(hwndTree, &tvis);

				tvis.hParent = hNetworkItem;
				tvis.item.pszText = const_cast<LPTSTR>(TEXT("SI伝送パラメータ"));
				HTREEITEM hSIParameterRoot = TreeView_InsertItem(hwndTree, &tvis);

				tvis.item.pszText = szText;

				for (size_t j = 0; j < NetworkInfo.SIParameterList.size(); j++) {
					const LibISDB::AnalyzerFilter::SIParameterInfo &SIParameter = NetworkInfo.SIParameterList[j];
					StringFormat(
						szText,
						TEXT("バージョン {} / 更新日 {}/{:02}/{:02}"),
						SIParameter.ParameterVersion,
						SIParameter.UpdateTime.Year, SIParameter.UpdateTime.Month, SIParameter.UpdateTime.Day);
					tvis.hParent = hSIParameterRoot;
					HTREEITEM hSIParameterItem = TreeView_InsertItem(hwndTree, &tvis);

					for (size_t k = 0; k < SIParameter.TableList.size(); k++) {
						const LibISDB::SIParameterDescriptor::TableInfo &Table = SIParameter.TableList[k];

						tvis.hParent = hSIParameterItem;

						if (Table.TableID == LibISDB::SIParameterDescriptor::TABLE_ID_EIT_SCHEDULE_ACTUAL
								|| Table.TableID == LibISDB::SIParameterDescriptor::TABLE_ID_EIT_SCHEDULE_EXTENDED
								|| Table.TableID == LibISDB::SIParameterDescriptor::TABLE_ID_EIT_SCHEDULE_OTHER) {
							switch(Table.TableID) {
							case LibISDB::SIParameterDescriptor::TABLE_ID_EIT_SCHEDULE_ACTUAL:
								StringCopy(szText, TEXT("EIT[schedule actual]"));
								break;
							case LibISDB::SIParameterDescriptor::TABLE_ID_EIT_SCHEDULE_EXTENDED:
								StringCopy(szText, TEXT("EIT[schedule extended]"));
								break;
							case LibISDB::SIParameterDescriptor::TABLE_ID_EIT_SCHEDULE_OTHER:
								StringCopy(szText, TEXT("EIT[schedule other]"));
								break;
							}
							HTREEITEM hEITItem = TreeView_InsertItem(hwndTree, &tvis);
							for (int MediaIndex = 0; MediaIndex < Table.EIT_Schedule.MediaTypeCount; MediaIndex++) {
								StringFormat(
									szText, TEXT("メディアタイプ {}({}) / 運用パターン {} / EIT[other]フラグ {} / 伝送範囲 {}日 / 周期 {}秒"),
									Table.EIT_Schedule.MediaTypeList[MediaIndex].MediaType,
									Table.EIT_Schedule.MediaTypeList[MediaIndex].MediaType == 1 ?
										TEXT("TV") :
									Table.EIT_Schedule.MediaTypeList[MediaIndex].MediaType == 2 ?
										TEXT("Audio") :
									Table.EIT_Schedule.MediaTypeList[MediaIndex].MediaType == 3 ?
										TEXT("Data") :
										TEXT("?"),
									Table.EIT_Schedule.MediaTypeList[MediaIndex].Pattern,
									NetworkInfo.BroadcasterList.size() == 1 && NetworkInfo.BroadcasterList[0].BroadcasterID == 0xFF ?
										TEXT("(n/a)") : // 地デジでは無効
										Table.EIT_Schedule.MediaTypeList[MediaIndex].EITOtherFlag ? TEXT("1") : TEXT("0"),
									Table.EIT_Schedule.MediaTypeList[MediaIndex].ScheduleRange,
									Table.EIT_Schedule.MediaTypeList[MediaIndex].BaseCycle);
								tvis.hParent = hEITItem;
								tvis.hParent = TreeView_InsertItem(hwndTree, &tvis);
								for (int CycleIndex = 0; CycleIndex < Table.EIT_Schedule.MediaTypeList[MediaIndex].CycleGroupCount; CycleIndex++) {
									StringFormat(
										szText,
										TEXT("セグメント数 {} / 周期 {}秒"),
										Table.EIT_Schedule.MediaTypeList[MediaIndex].CycleGroup[CycleIndex].NumOfSegment,
										Table.EIT_Schedule.MediaTypeList[MediaIndex].CycleGroup[CycleIndex].Cycle);
									TreeView_InsertItem(hwndTree, &tvis);
								}
							}
						} else {
							switch (Table.TableID) {
							case LibISDB::SIParameterDescriptor::TABLE_ID_NIT:
								StringFormat(szText, TEXT("NIT : 周期 {}秒"), Table.NIT.TableCycle);
								break;
							case LibISDB::SIParameterDescriptor::TABLE_ID_SDT_ACTUAL:
								StringFormat(szText, TEXT("SDT[actual] : 周期 {}秒"), Table.SDT.TableCycle);
								break;
							case LibISDB::SIParameterDescriptor::TABLE_ID_SDT_OTHER:
								StringFormat(szText, TEXT("SDT[other] : 周期 {}秒"), Table.SDT.TableCycle);
								break;
							case LibISDB::SIParameterDescriptor::TABLE_ID_EIT_PF_ACTUAL:
								if (Table.HMLEIT.Valid) {
									StringFormat(
										szText,
										TEXT("EIT[p/f actual] : H-EIT[p/f]周期 {}秒 / M-EIT周期 {}秒 / L-EIT周期 {}秒 / M-EIT番組数 {} / L-EIT番組数 {}"),
										Table.HMLEIT.HEITTableCycle,
										Table.HMLEIT.MEITTableCycle,
										Table.HMLEIT.LEITTableCycle,
										Table.HMLEIT.NumOfMEITEvent,
										Table.HMLEIT.NumOfLEITEvent);
								} else {
									StringFormat(szText, TEXT("EIT[p/f actual] : 周期 {}秒"), Table.EIT_PF.TableCycle);
								}
								break;
							case LibISDB::SIParameterDescriptor::TABLE_ID_EIT_PF_OTHER:
								StringFormat(szText, TEXT("EIT[p/f other] : 周期 {}秒"), Table.EIT_PF.TableCycle);
								break;
							case LibISDB::SIParameterDescriptor::TABLE_ID_SDTT:
								StringFormat(szText, TEXT("SDTT : 周期 {}秒"), Table.SDTT.TableCycle);
								break;
							case LibISDB::SIParameterDescriptor::TABLE_ID_BIT:
								StringFormat(szText, TEXT("BIT : 周期 {}秒"), Table.BIT.TableCycle);
								break;
							case LibISDB::SIParameterDescriptor::TABLE_ID_NBIT_MSG:
								StringFormat(szText, TEXT("NBIT[msg] : 周期 {}秒"), Table.NBIT.TableCycle);
								break;
							case LibISDB::SIParameterDescriptor::TABLE_ID_NBIT_REF:
								StringFormat(szText, TEXT("NBIT[ref] : 周期 {}秒"), Table.NBIT.TableCycle);
								break;
							case LibISDB::SIParameterDescriptor::TABLE_ID_LDT:
								StringFormat(szText, TEXT("LDT : 周期 {}秒"), Table.LDT.TableCycle);
								break;
							case LibISDB::SIParameterDescriptor::TABLE_ID_CDT:
								StringFormat(szText, TEXT("CDT : 周期 {}秒"), Table.CDT.TableCycle);
								break;
							default:
								StringFormat(szText, TEXT("Unknown table_id {}"), Table.TableID);
								break;
							}

							TreeView_InsertItem(hwndTree, &tvis);
						}
					}
				}

				tvis.hParent = hNetworkItem;
				tvis.item.pszText = const_cast<LPTSTR>(TEXT("ブロードキャスタ"));
				HTREEITEM hBroadcasterRoot = TreeView_InsertItem(hwndTree, &tvis);

				tvis.item.pszText = szText;

				for (size_t j = 0; j < NetworkInfo.BroadcasterList.size(); j++) {
					const LibISDB::AnalyzerFilter::BITBroadcasterInfo &Broadcaster = NetworkInfo.BroadcasterList[j];
					tvis.hParent = hBroadcasterRoot;

					if (Broadcaster.BroadcasterType == LibISDB::ExtendedBroadcasterDescriptor::BROADCASTER_TYPE_TERRESTRIAL
							|| Broadcaster.BroadcasterType == LibISDB::ExtendedBroadcasterDescriptor::BROADCASTER_TYPE_TERRESTRIAL_SOUND) {
						String AffiliationIDs;
						for (int k = 0; k < Broadcaster.TerrestrialBroadcasterInfo.NumberOfAffiliationIDLoop; k++) {
							if (!AffiliationIDs.empty())
								AffiliationIDs += TEXT(", ");
							TCHAR szID[4];
							StringFormat(szID, TEXT("{}"), Broadcaster.TerrestrialBroadcasterInfo.AffiliationIDList[k]);
							AffiliationIDs += szID;
						}
						StringFormat(
							szText,
							TEXT("地上ブロードキャスタID {} / ブロードキャスタ種別 {} / 系列ID [{}]"),
							Broadcaster.TerrestrialBroadcasterInfo.TerrestrialBroadcasterID,
							Broadcaster.BroadcasterType,
							AffiliationIDs);
						tvis.hParent = TreeView_InsertItem(hwndTree, &tvis);

						for (int k = 0; k < Broadcaster.TerrestrialBroadcasterInfo.NumberOfBroadcasterIDLoop; k++) {
							StringFormat(
								szText,
								TEXT("関連BS/CSブロードキャスタ{0} : ONID {1:#04x} ({1}) / ブロードキャスタID {2}"),
								k + 1,
								Broadcaster.TerrestrialBroadcasterInfo.BroadcasterIDList[k].OriginalNetworkID,
								Broadcaster.TerrestrialBroadcasterInfo.BroadcasterIDList[k].BroadcasterID);
							TreeView_InsertItem(hwndTree, &tvis);
						}
					} else {
						StringFormat(
							szText,
							TEXT("ブロードキャスタID {} / {}"),
							Broadcaster.BroadcasterID,
							Broadcaster.BroadcasterName);
						TreeView_InsertItem(hwndTree, &tvis);
					}
				}
			}
		}
	}

	// 地上/衛星分配システム
	LibISDB::AnalyzerFilter::TerrestrialDeliverySystemList TerrestrialList;
	if (pAnalyzer->GetTerrestrialDeliverySystemList(&TerrestrialList)) {
		tvis.hParent = TVI_ROOT;
		tvis.hInsertAfter = TVI_LAST;
		tvis.item.mask = TVIF_STATE | TVIF_TEXT | TVIF_CHILDREN;
		tvis.item.state = TVIS_EXPANDED;
		tvis.item.stateMask = ~0U;
		tvis.item.pszText = const_cast<LPTSTR>(TEXT("地上分配システム"));
		tvis.item.cChildren = !TerrestrialList.empty() ? 1 : 0;
		hItem = TreeView_InsertItem(hwndTree, &tvis);
		if (hItem != nullptr) {
			for (size_t i = 0; i < TerrestrialList.size(); i++) {
				const LibISDB::AnalyzerFilter::TerrestrialDeliverySystemInfo &Info = TerrestrialList[i];
				const LPCTSTR pszArea = LibISDB::GetAreaText_ja(Info.AreaCode);

				StringFormat(
					szText,
					TEXT("TSID {0:#04x} ({0}) / エリア {1} / ガードインターバル {2} / 伝送モード {3}"),
					Info.TransportStreamID,
					pszArea != nullptr ? pszArea : TEXT("?"),
					Info.GuardInterval == 0 ? TEXT("1/32") :
					Info.GuardInterval == 1 ? TEXT("1/16") :
					Info.GuardInterval == 2 ? TEXT("1/8") :
					Info.GuardInterval == 3 ? TEXT("1/4") : TEXT("?"),
					Info.TransmissionMode == 0 ? TEXT("Mode1") :
					Info.TransmissionMode == 1 ? TEXT("Mode2") :
					Info.TransmissionMode == 2 ? TEXT("Mode3") : TEXT("?"));
				tvis.hParent = hItem;
				tvis.item.state = 0;
				tvis.item.cChildren = 1;
				tvis.item.pszText = szText;
				tvis.hParent = TreeView_InsertItem(hwndTree, &tvis);
				tvis.item.cChildren = 0;
				for (size_t j = 0; j < TerrestrialList[i].Frequency.size(); j++) {
					StringFormat(
						szText, TEXT("周波数{} : {} MHz"),
						j + 1, TerrestrialList[i].Frequency[j] / 7);
					TreeView_InsertItem(hwndTree, &tvis);
				}
			}
		}
	} else {
		LibISDB::AnalyzerFilter::SatelliteDeliverySystemList SatelliteList;
		if (pAnalyzer->GetSatelliteDeliverySystemList(&SatelliteList)) {
			tvis.hParent = TVI_ROOT;
			tvis.hInsertAfter = TVI_LAST;
			tvis.item.mask = TVIF_STATE | TVIF_TEXT | TVIF_CHILDREN;
			tvis.item.state = 0;
			tvis.item.stateMask = ~0U;
			tvis.item.pszText = const_cast<LPTSTR>(TEXT("衛星分配システム"));
			tvis.item.cChildren = !SatelliteList.empty() ? 1 : 0;
			hItem = TreeView_InsertItem(hwndTree, &tvis);
			if (hItem != nullptr) {
				for (size_t i = 0; i < SatelliteList.size(); i++) {
					const LibISDB::AnalyzerFilter::SatelliteDeliverySystemInfo &Info = SatelliteList[i];

					StringFormat(
						szText,
						TEXT("TS{0} : TSID {1:#04x} ({1}) / 周波数 {2}.{3:05} GHz"),
						i + 1,
						Info.TransportStreamID,
						Info.Frequency / 100000,
						Info.Frequency % 100000);
					tvis.hParent = hItem;
					tvis.item.state = 0;
					tvis.item.cChildren = 0;
					tvis.item.pszText = szText;
					TreeView_InsertItem(hwndTree, &tvis);
				}
			}
		} else {
			LibISDB::AnalyzerFilter::CableDeliverySystemList CableList;
			if (pAnalyzer->GetCableDeliverySystemList(&CableList)) {
				tvis.hParent = TVI_ROOT;
				tvis.hInsertAfter = TVI_LAST;
				tvis.item.mask = TVIF_STATE | TVIF_TEXT | TVIF_CHILDREN;
				tvis.item.state = 0;
				tvis.item.stateMask = ~0U;
				tvis.item.pszText = const_cast<LPTSTR>(TEXT("有線分配システム"));
				tvis.item.cChildren = !CableList.empty() ? 1 : 0;
				hItem = TreeView_InsertItem(hwndTree, &tvis);
				if (hItem != nullptr) {
					for (size_t i = 0; i < CableList.size(); i++) {
						const LibISDB::AnalyzerFilter::CableDeliverySystemInfo& Info = CableList[i];

						StringFormat(
							szText,
							TEXT("TS{0} : TSID {1:#04x} ({1}) / 周波数 {2}.{3:04} MHz / {4} / 変調 {5}"),
							i + 1,
							Info.TransportStreamID,
							Info.Frequency / 10000,
							Info.Frequency % 10000,
							Info.FrameType == 0x01 ? TEXT("TSMF [53,15]") :
							Info.FrameType == 0x02 ? TEXT("TSMF [53,15] 複数TS不可") :
							Info.FrameType == 0x0f ? TEXT("単一TS") : TEXT("?"),
							Info.Modulation == 0x01 ? TEXT("16QAM") :
							Info.Modulation == 0x02 ? TEXT("32QAM") :
							Info.Modulation == 0x03 ? TEXT("64QAM") :
							Info.Modulation == 0x04 ? TEXT("128QAM") :
							Info.Modulation == 0x05 ? TEXT("256QAM") :
							Info.Modulation == 0x07 ? TEXT("1024QAM") : TEXT("?"));
						tvis.hParent = hItem;
						tvis.item.state = 0;
						tvis.item.cChildren = 0;
						tvis.item.pszText = szText;
						TreeView_InsertItem(hwndTree, &tvis);
					}
				}
			}
		}
	}
}


void CStreamInfoPage::GetTreeViewText(
	HWND hwndTree, HTREEITEM hItem, bool fSiblings, String *pText, int Level)
{
	TV_ITEM tvi;
	TCHAR szBuff[256];

	tvi.mask = TVIF_TEXT;
	tvi.hItem = hItem;
	while (tvi.hItem != nullptr) {
		if (Level > 0)
			pText->append(Level, TEXT('\t'));
		tvi.pszText = szBuff;
		tvi.cchTextMax = lengthof(szBuff);
		if (TreeView_GetItem(hwndTree, &tvi)) {
			*pText += szBuff;
			*pText += TEXT("\r\n");
		}
		const HTREEITEM hChild = TreeView_GetChild(hwndTree, tvi.hItem);
		if (hChild != nullptr) {
			GetTreeViewText(hwndTree, hChild, true, pText, Level + 1);
		}
		if (!fSiblings)
			break;
		tvi.hItem = TreeView_GetNextSibling(hwndTree, tvi.hItem);
	}
}


void CStreamInfoPage::ExpandTreeViewItem(HWND hwndTree, HTREEITEM hItem, unsigned int Flags)
{
	TreeView_Expand(hwndTree, hItem, Flags);

	for (HTREEITEM hChild = TreeView_GetChild(hwndTree, hItem);
			hChild != nullptr;
			hChild = TreeView_GetNextSibling(hwndTree, hChild)) {
		ExpandTreeViewItem(hwndTree, hChild, Flags);
	}
}




class CPIDInfoPage
	: public CResizableDialog
{
public:
	enum {
		COLUMN_PID,
		COLUMN_INPUT_COUNT,
		COLUMN_OUTPUT_COUNT,
		COLUMN_DROPPED_COUNT,
		COLUMN_ERROR_COUNT,
		//COLUMN_SCRAMBLED_COUNT,
		COLUMN_DESCRIPTION,
		NUM_COLUMNS
	};

	CPIDInfoPage();
	~CPIDInfoPage();

	bool Create(HWND hwndOwner) override;

	int GetColumnWidth(int Column) const { return m_ColumnWidth[Column]; }
	void SetColumnWidth(int Column, int Width) { m_ColumnWidth[Column] = Width; }

private:
	INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	void UpdateInfo();
	String GetListText();
	void SortItems(int Column, bool fDescending);

	static int CALLBACK ItemCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	typedef LibISDB::TSPacketParserFilter::PacketCountInfo PacketCountInfo;

	struct PIDInfo
	{
		std::uint16_t PID;
		PacketCountInfo Count;
		String Description;
	};

	struct SortParams
	{
		std::vector<PIDInfo> *pPIDInfoList;
		int Column;
		bool fDescending;
	};

	struct ColumnInfo
	{
		LPCTSTR pszText;
		LPCTSTR pszFormatText;
		int Digits;
		int Format;
	};

	static constexpr UINT TIMER_ID_UPDATE = 1;
	static const ColumnInfo m_ColumnList[NUM_COLUMNS];

	int m_ColumnWidth[NUM_COLUMNS];
	int m_SortColumn = COLUMN_PID;
	bool m_fSortDescending = false;
	std::vector<PIDInfo> m_PIDInfoList;
};


const CPIDInfoPage::ColumnInfo CPIDInfoPage::m_ColumnList[NUM_COLUMNS] = {
	{TEXT("PID"),      TEXT("PID"),         4, LVCFMT_LEFT},
	{TEXT("入力"),     TEXT("Input"),       9, LVCFMT_RIGHT},
	{TEXT("出力"),     TEXT("Output"),      9, LVCFMT_RIGHT},
	{TEXT("ドロップ"), TEXT("Dropped"),     9, LVCFMT_RIGHT},
	{TEXT("エラー"),   TEXT("Error"),       9, LVCFMT_RIGHT},
	//{TEXT("暗号化"),   TEXT("Scrambled"),   9, LVCFMT_RIGHT},
	{TEXT("内容"),     TEXT("Description"), 0, LVCFMT_LEFT},
};


CPIDInfoPage::CPIDInfoPage()
{
	for (int &Width : m_ColumnWidth)
		Width = -1;
}


CPIDInfoPage::~CPIDInfoPage()
{
	Destroy();
}


bool CPIDInfoPage::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner, GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_PIDINFO));
}


INT_PTR CPIDInfoPage::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			AddControl(IDC_PIDINFO_LIST, AlignFlag::All);
			AddControl(IDC_PIDINFO_COPY, AlignFlag::BottomRight);

			const HWND hwndList = ::GetDlgItem(hDlg, IDC_PIDINFO_LIST);

			ListView_SetExtendedListViewStyle(
				hwndList,
				LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);
			SetListViewTooltipsTopMost(hwndList);

			LVCOLUMN lvc;
			lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM;

			for (int i = 0; i < NUM_COLUMNS; i++) {
				lvc.pszText = const_cast<LPTSTR>(m_ColumnList[i].pszText);
				lvc.fmt = m_ColumnList[i].Format;
				lvc.iSubItem = i;
				ListView_InsertColumn(hwndList, i, &lvc);
			}

			UpdateInfo();

			for (int i = 0; i < NUM_COLUMNS; i++) {
				ListView_SetColumnWidth(
					hwndList, i, m_ColumnWidth[i] >= 0 ? m_ColumnWidth[i] : LVSCW_AUTOSIZE_USEHEADER);
			}
		}
		return TRUE;

	case WM_SHOWWINDOW:
		if (wParam) {
			UpdateInfo();
			::SetTimer(hDlg, TIMER_ID_UPDATE, 1000, nullptr);
		} else {
			::KillTimer(hDlg, TIMER_ID_UPDATE);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_PIDINFO_COPY:
			{
				String Text(GetListText());

				if (!Text.empty())
					CopyTextToClipboard(GetAppClass().UICore.GetMainWindow(), Text.c_str());
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<NMHDR*>(lParam)->code) {
		case LVN_COLUMNCLICK:
			{
				const NMLISTVIEW *pnmlv = reinterpret_cast<const NMLISTVIEW*>(lParam);
				const int Column = pnmlv->iSubItem;
				const bool fDescending =
					(Column == m_SortColumn) ?
						!m_fSortDescending :
						((Column != COLUMN_PID) && (Column != COLUMN_DESCRIPTION));

				SortItems(Column, fDescending);

				m_SortColumn = Column;
				m_fSortDescending = fDescending;
			}
			return TRUE;
		}
		break;

	case WM_TIMER:
		if (wParam == TIMER_ID_UPDATE)
			UpdateInfo();
		return TRUE;

	case WM_DESTROY:
		{
			const HWND hwndList = ::GetDlgItem(hDlg, IDC_PIDINFO_LIST);
			for (int i = 0; i < NUM_COLUMNS; i++) {
				m_ColumnWidth[i] = ListView_GetColumnWidth(hwndList, i);
			}
		}
		return TRUE;
	}

	return FALSE;
}


void CPIDInfoPage::UpdateInfo()
{
	const CAppMain &App = GetAppClass();
	const LibISDB::TSPacketParserFilter *pParser =
		App.CoreEngine.GetFilter<LibISDB::TSPacketParserFilter>();
	const LibISDB::AnalyzerFilter *pAnalyzer =
		App.CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
	if (pParser == nullptr)
		return;

	LibISDB::AnalyzerFilter::ServiceList ServiceList;
	LibISDB::AnalyzerFilter::EMMPIDList EMMPIDList;
	if (pAnalyzer != nullptr) {
		pAnalyzer->GetServiceList(&ServiceList);
		pAnalyzer->GetEMMPIDList(&EMMPIDList);
	}

	const HWND hwndList = ::GetDlgItem(m_hDlg, IDC_PIDINFO_LIST);
	const int ItemCount = ListView_GetItemCount(hwndList);

	BYTE ItemState[LibISDB::PID_MAX + 1];
	::ZeroMemory(ItemState, sizeof(ItemState));

	LVITEM lvi;
	lvi.mask = LVIF_PARAM | LVIF_STATE;
	lvi.iSubItem = 0;
	lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	for (int i = 0; i < ItemCount; i++) {
		lvi.iItem = i;
		ListView_GetItem(hwndList, &lvi);
		ItemState[m_PIDInfoList[lvi.lParam].PID] = static_cast<BYTE>(lvi.state);
	}

	::SendMessage(hwndList, WM_SETREDRAW, FALSE, 0);

	int PIDCount = 0;

	for (std::uint16_t PID = 0; PID <= LibISDB::PID_MAX; PID++) {
		const PacketCountInfo PacketCount = pParser->GetPacketCount(PID);

		if (PacketCount.Input > 0) {
			if (m_PIDInfoList.size() <= static_cast<size_t>(PIDCount))
				m_PIDInfoList.emplace_back();
			PIDInfo &Info = m_PIDInfoList[PIDCount];
			Info.PID = PID;
			Info.Count = PacketCount;

			TCHAR szText[32];

			StringFormat(szText, TEXT("{:04x}"), PID);

			LVITEM lvi;

			lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
			lvi.iItem = PIDCount;
			lvi.iSubItem = COLUMN_PID;
			lvi.pszText = szText;
			lvi.lParam = PIDCount;
			lvi.state = ItemState[PID];
			lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

			if (PIDCount >= ItemCount)
				ListView_InsertItem(hwndList, &lvi);
			else
				ListView_SetItem(hwndList, &lvi);

			StringFormat(szText, TEXT("{}"), PacketCount.Input);
			ListView_SetItemText(hwndList, PIDCount, COLUMN_INPUT_COUNT, szText);

			StringFormat(szText, TEXT("{}"), PacketCount.Output);
			ListView_SetItemText(hwndList, PIDCount, COLUMN_OUTPUT_COUNT, szText);

			StringFormat(szText, TEXT("{}"), PacketCount.ContinuityError);
			ListView_SetItemText(hwndList, PIDCount, COLUMN_DROPPED_COUNT, szText);

			StringFormat(szText, TEXT("{}"), PacketCount.FormatError + PacketCount.TransportError);
			ListView_SetItemText(hwndList, PIDCount, COLUMN_ERROR_COUNT, szText);

			//StringFormat(szText, TEXT("{}"), PacketCount.Scrambled);
			//ListView_SetItemText(hwndList, PIDCount, COLUMN_SCRAMBLED_COUNT, szText);

			Info.Description.clear();

			const LibISDB::CharType *pText = LibISDB::GetPredefinedPIDText(PID);
			if (pText != nullptr)
				Info.Description += pText;

			if (std::ranges::find(EMMPIDList, PID) != EMMPIDList.end())
				Info.Description += TEXT("EMM");

			for (auto &Service : ServiceList) {
				TCHAR ServiceText[8];
				StringFormat(ServiceText, TEXT("[{:04x}]"), Service.ServiceID);

				if (Service.PMTPID == PID) {
					if (!Info.Description.empty())
						Info.Description += TEXT(' ');

					Info.Description += ServiceText;
					Info.Description += TEXT(" PMT");
				}

				if (Service.PCRPID == PID) {
					if (!Info.Description.empty())
						Info.Description += TEXT(' ');

					Info.Description += ServiceText;
					Info.Description += TEXT(" PCR");
				}

				for (auto const &ECM : Service.ECMList) {
					if (ECM.PID == PID) {
						if (!Info.Description.empty())
							Info.Description += TEXT(' ');

						Info.Description += ServiceText;
						Info.Description += TEXT(" ECM");
						break;
					}
				}

				for (auto &ES : Service.ESList) {
					if (ES.PID == PID) {
						if (!Info.Description.empty())
							Info.Description += TEXT(' ');

						Info.Description += ServiceText;

						if (ES.StreamType == LibISDB::STREAM_TYPE_CAPTION)
							pText = TEXT("字幕");
						else if (ES.StreamType == LibISDB::STREAM_TYPE_DATA_CARROUSEL)
							pText = TEXT("データ");
						else
							pText = LibISDB::GetStreamTypeText(ES.StreamType);
						if (pText != nullptr) {
							Info.Description += TEXT(' ');
							Info.Description += pText;
						}
					}
				}
			}

			ListView_SetItemText(hwndList, PIDCount, COLUMN_DESCRIPTION, const_cast<LPTSTR>(Info.Description.c_str()));

			PIDCount++;
		}
	}

	if (m_PIDInfoList.size() > static_cast<size_t>(PIDCount))
		m_PIDInfoList.resize(PIDCount);

	for (int i = ItemCount - 1; i >= PIDCount; i--)
		ListView_DeleteItem(hwndList, i);

	SortItems(m_SortColumn, m_fSortDescending);

	::SendMessage(hwndList, WM_SETREDRAW, TRUE, 0);
	::RedrawWindow(hwndList, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_UPDATENOW);
}


String CPIDInfoPage::GetListText()
{
	const HWND hwndList = ::GetDlgItem(m_hDlg, IDC_PIDINFO_LIST);
	const int ItemCount = ListView_GetItemCount(hwndList);
	int ColumnDigits[NUM_COLUMNS];
	String Text;

	for (int i = 0; i < NUM_COLUMNS; i++) {
		const int Length = static_cast<int>(StringLength(m_ColumnList[i].pszFormatText));
		ColumnDigits[i] = std::max(Length, m_ColumnList[i].Digits);
		const int Filler = ColumnDigits[i] - Length;
		if (m_ColumnList[i].Format == LVCFMT_RIGHT)
			Text.append(Filler, TEXT(' '));
		Text += m_ColumnList[i].pszFormatText;
		if (i < NUM_COLUMNS - 1) {
			if (m_ColumnList[i].Format == LVCFMT_LEFT)
				Text.append(Filler, TEXT(' '));
			Text += TEXT(' ');
		}
	}

	Text += TEXT("\r\n");

	for (int i = 0; i < ItemCount; i++) {
		for (int j = 0; j < NUM_COLUMNS; j++) {
			TCHAR szText[256];

			szText[0] = TEXT('\0');
			ListView_GetItemText(hwndList, i, j, szText, lengthof(szText));
			const int Filler = std::max(ColumnDigits[j] - static_cast<int>(StringLength(szText)), 0);
			if (m_ColumnList[j].Format == LVCFMT_RIGHT)
				Text.append(Filler, TEXT(' '));
			Text += szText;
			if (j < NUM_COLUMNS - 1) {
				if (m_ColumnList[j].Format == LVCFMT_LEFT)
					Text.append(Filler, TEXT(' '));
				Text += TEXT(' ');
			}
		}

		Text += TEXT("\r\n");
	}

	return Text;
}


void CPIDInfoPage::SortItems(int Column, bool fDescending)
{
	const HWND hwndList = ::GetDlgItem(m_hDlg, IDC_PIDINFO_LIST);
	SortParams Params;

	Params.pPIDInfoList = &m_PIDInfoList;
	Params.Column = Column;
	Params.fDescending = fDescending;

	ListView_SortItems(hwndList, ItemCompare, reinterpret_cast<LPARAM>(&Params));
	SetListViewSortMark(hwndList, Column, !fDescending);
}


int CALLBACK CPIDInfoPage::ItemCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	const SortParams *pParams = reinterpret_cast<const SortParams*>(lParamSort);
	const PIDInfo &Info1 = (*pParams->pPIDInfoList)[lParam1];
	const PIDInfo &Info2 = (*pParams->pPIDInfoList)[lParam2];
	const auto CompareValue = [](auto v1, auto v2) -> int { return (v1 < v2) ? -1 : (v1 > v2) ? 1 : 0; };
	int Cmp;

	switch (pParams->Column) {
	case COLUMN_PID:
		Cmp = CompareValue(Info1.PID, Info2.PID);
		break;

	case COLUMN_INPUT_COUNT:
		Cmp = CompareValue(Info1.Count.Input, Info2.Count.Input);
		break;

	case COLUMN_OUTPUT_COUNT:
		Cmp = CompareValue(Info1.Count.Output, Info2.Count.Output);
		break;

	case COLUMN_DROPPED_COUNT:
		Cmp = CompareValue(Info1.Count.ContinuityError, Info2.Count.ContinuityError);
		break;

	case COLUMN_ERROR_COUNT:
		Cmp = CompareValue(
			Info1.Count.FormatError + Info1.Count.TransportError,
			Info2.Count.FormatError + Info2.Count.TransportError);
		break;

	/*
	case COLUMN_SCRAMBLED_COUNT:
		Cmp = CompareValue(Info1.Count.Scrambled, Info2.Count.Scrambled);
		break;
	*/

	case COLUMN_DESCRIPTION:
		if (Info1.Description.empty()) {
			if (Info2.Description.empty())
				Cmp = 0;
			else
				Cmp = 1;
		} else if (Info2.Description.empty()) {
			Cmp = -1;
		} else {
			Cmp = Info1.Description.compare(Info2.Description);
		}
		break;
	}

	if (Cmp == 0)
		Cmp = CompareValue(Info1.PID, Info2.PID);

	return pParams->fDescending ? -Cmp : Cmp;
}


} // namespace




CStreamInfo::CStreamInfo()
{
	m_PageList[PAGE_STREAMINFO].pszTitle = TEXT("情報");
	m_PageList[PAGE_STREAMINFO].Dialog = std::make_unique<CStreamInfoPage>();
	m_PageList[PAGE_PIDINFO   ].pszTitle = TEXT("PID");
	m_PageList[PAGE_PIDINFO   ].Dialog = std::make_unique<CPIDInfoPage>();

	for (const auto &Page : m_PageList)
		RegisterUIChild(Page.Dialog.get());

	SetStyleScaling(&m_StyleScaling);
}


CStreamInfo::~CStreamInfo()
{
	Destroy();
}


bool CStreamInfo::Create(HWND hwndOwner)
{
	if (m_fCreateFirst) {
		if (m_pEventHandler != nullptr)
			m_pEventHandler->OnRestoreSettings();
		m_fCreateFirst = false;
	}

	return CreateDialogWindow(hwndOwner, GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_STREAMPROPERTIES));
}


void CStreamInfo::SetEventHandler(CEventHandler *pHandler)
{
	m_pEventHandler = pHandler;
}


void CStreamInfo::LoadSettings(CSettings &Settings)
{
	int Left, Top, Width, Height;
	GetPosition(&Left, &Top, &Width, &Height);
	Settings.Read(TEXT("StreamInfoLeft"), &Left);
	Settings.Read(TEXT("StreamInfoTop"), &Top);
	Settings.Read(TEXT("StreamInfoWidth"), &Width);
	Settings.Read(TEXT("StreamInfoHeight"), &Height);
	SetPosition(Left, Top, Width, Height);

	int ActivePage;
	if (Settings.Read(TEXT("StreamInfo.ActivePage"), &ActivePage)
			&& (ActivePage >= 0) && (ActivePage < NUM_PAGES))
		m_CurrentPage = ActivePage;

	CPIDInfoPage *pPIDInfo = static_cast<CPIDInfoPage*>(m_PageList[PAGE_PIDINFO].Dialog.get());
	for (int i = 0; i < CPIDInfoPage::NUM_COLUMNS; i++) {
		TCHAR szKey[32];
		StringFormat(szKey, TEXT("PIDInfo.Column{}.Width"), i);
		int Width;
		if (Settings.Read(szKey, &Width))
			pPIDInfo->SetColumnWidth(i, Width);
	}
}


void CStreamInfo::SaveSettings(CSettings &Settings) const
{
	if (IsPositionSet()) {
		RECT rc;
		GetPosition(&rc);
		Settings.Write(TEXT("StreamInfoLeft"), static_cast<int>(rc.left));
		Settings.Write(TEXT("StreamInfoTop"), static_cast<int>(rc.top));
		Settings.Write(TEXT("StreamInfoWidth"), static_cast<int>(rc.right - rc.left));
		Settings.Write(TEXT("StreamInfoHeight"), static_cast<int>(rc.bottom - rc.top));
	}

	Settings.Write(TEXT("StreamInfo.ActivePage"), m_CurrentPage);

	const CPIDInfoPage *pPIDInfo = static_cast<const CPIDInfoPage*>(m_PageList[PAGE_PIDINFO].Dialog.get());
	for (int i = 0; i < CPIDInfoPage::NUM_COLUMNS; i++) {
		const int Width = pPIDInfo->GetColumnWidth(i);
		if (Width >= 0) {
			TCHAR szKey[32];
			StringFormat(szKey, TEXT("PIDInfo.Column{}.Width"), i);
			Settings.Write(szKey, Width);
		}
	}
}


INT_PTR CStreamInfo::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			const HWND hwndTab = ::GetDlgItem(hDlg, IDC_STREAMPROPERTIES_TAB);

			TCITEM tci;
			tci.mask = TCIF_TEXT;
			for (int i = 0; i < NUM_PAGES; i++) {
				tci.pszText = const_cast<LPTSTR>(m_PageList[i].pszTitle);
				TabCtrl_InsertItem(hwndTab, i, &tci);
			}

			SetPage(m_CurrentPage);

			if (!IsPositionSet()) {
				// デフォルトのウィンドウサイズ
				RECT rc = {0, 0, m_DefaultPageSize.cx, m_DefaultPageSize.cy};
				TabCtrl_AdjustRect(hwndTab, TRUE, &rc);
				m_pStyleScaling->AdjustWindowRect(hDlg, &rc);
				::SetWindowPos(
					hDlg, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
					SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
			} else {
				ApplyPosition();
			}

			GetAppClass().UICore.RegisterModelessDialog(this);
		}
		return TRUE;

	case WM_SIZE:
		OnSizeChanged(LOWORD(lParam), HIWORD(lParam));
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL:
			if (m_pEventHandler != nullptr)
				m_pEventHandler->OnClose();
			::DestroyWindow(hDlg);
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<NMHDR*>(lParam)->code) {
		case TCN_SELCHANGE:
			SetPage(TabCtrl_GetCurSel(::GetDlgItem(hDlg, IDC_STREAMPROPERTIES_TAB)));
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			for (const auto &Page : m_PageList)
				Page.Dialog->Destroy();

			GetAppClass().UICore.UnregisterModelessDialog(this);
		}
		return TRUE;
	}

	return FALSE;
}


void CStreamInfo::OnSizeChanged(int Width, int Height)
{
	const HWND hwndTab = ::GetDlgItem(m_hDlg, IDC_STREAMPROPERTIES_TAB);

	::MoveWindow(hwndTab, 0, 0, Width, Height, TRUE);

	RECT rc;
	GetPagePosition(&rc);
	m_PageList[m_CurrentPage].Dialog->SetPosition(&rc);
}


bool CStreamInfo::CreatePage(int Page)
{
	if (Page < 0 || Page >= NUM_PAGES)
		return false;

	const PageInfo &Info = m_PageList[Page];

	if (Info.Dialog->IsCreated())
		return true;

	if (!Info.Dialog->Create(m_hDlg))
		return false;

	RECT rc;
	Info.Dialog->GetPosition(&rc);
	m_DefaultPageSize.cx = rc.right - rc.left;
	m_DefaultPageSize.cy = rc.bottom - rc.top;

	return true;
}


bool CStreamInfo::SetPage(int Page)
{
	if (!CreatePage(Page))
		return false;

	if (m_PageList[m_CurrentPage].Dialog->IsCreated())
		m_PageList[m_CurrentPage].Dialog->SetVisible(false);

	RECT rc;
	GetPagePosition(&rc);
	m_PageList[Page].Dialog->SetPosition(&rc);
	m_PageList[Page].Dialog->SetVisible(true);
	::BringWindowToTop(m_PageList[Page].Dialog->GetHandle());

	m_CurrentPage = Page;

	TabCtrl_SetCurSel(::GetDlgItem(m_hDlg, IDC_STREAMPROPERTIES_TAB), m_CurrentPage);

	return true;
}


void CStreamInfo::GetPagePosition(RECT *pPosition) const
{
	GetClientRect(m_hDlg, pPosition);
	TabCtrl_AdjustRect(::GetDlgItem(m_hDlg, IDC_STREAMPROPERTIES_TAB), FALSE, pPosition);
}


} // namespace TVTest
