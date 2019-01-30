/*
  TVTest
  Copyright(c) 2008-2019 DBCTRADO

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


const LPCTSTR STREAM_INFO_WINDOW_CLASS = APP_NAME TEXT(" Stream Info Window");
const LPCTSTR STREAM_INFO_TITLE_TEXT = TEXT("ストリームの情報");


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
				HWND hwndTree = ::GetDlgItem(hDlg, IDC_STREAMINFO_SERVICE);
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
				NMHDR *pnmhdr = reinterpret_cast<NMHDR*>(lParam);
				//HTREEITEM hItem = TreeView_GetSelection(pnmhdr->hwndFrom);
				DWORD Pos = ::GetMessagePos();
				TVHITTESTINFO tvhti;
				tvhti.pt.x = (SHORT)LOWORD(Pos);
				tvhti.pt.y = (SHORT)HIWORD(Pos);
				::ScreenToClient(pnmhdr->hwndFrom, &tvhti.pt);
				HTREEITEM hItem = TreeView_HitTest(pnmhdr->hwndFrom, &tvhti);
				if (hItem != nullptr) {
					HMENU hmenu = ::CreatePopupMenu();
					::AppendMenu(hmenu, MFT_STRING | MFS_ENABLED, 1, TEXT("コピー(&C)"));
					POINT pt;
					::GetCursorPos(&pt);
					switch (::TrackPopupMenu(hmenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hDlg, nullptr)) {
					case 1:
						{
							String Text;

							GetTreeViewText(pnmhdr->hwndFrom, hItem, false, &Text);
							if (!Text.empty())
								CopyTextToClipboard(GetAppClass().UICore.GetMainWindow(), Text.c_str());
						}
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
	LPCTSTR pszStreamType = LibISDB::GetStreamTypeText(Es.StreamType);
	StringPrintf(
		pszText, MaxText,
		TEXT("%s%d : PID 0x%04x (%d) / stream type 0x%02x (%s) / component tag 0x%02x"),
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
	int Length;

	const WORD TSID = pAnalyzer->GetTransportStreamID();
	if (TSID != LibISDB::TRANSPORT_STREAM_ID_INVALID) {
		Length = StringPrintf(szText, TEXT("TSID 0x%04x (%d)"), TSID, TSID);
		String TSName;
		if (pAnalyzer->GetTSName(&TSName)) {
			StringPrintf(szText + Length, lengthof(szText) - Length, TEXT(" %s"), TSName.c_str());
		}
	} else {
		szText[0] = '\0';
	}
	::SetDlgItemText(m_hDlg, IDC_STREAMINFO_STREAM, szText);

	const WORD NID = pAnalyzer->GetNetworkID();
	if (NID != LibISDB::NETWORK_ID_INVALID) {
		Length = StringPrintf(szText, TEXT("NID 0x%04x (%d)"), NID, NID);
		String Name;
		if (pAnalyzer->GetNetworkName(&Name)) {
			StringPrintf(szText + Length, lengthof(szText) - Length, TEXT(" %s"), Name.c_str());
		}
	} else {
		szText[0] = '\0';
	}
	::SetDlgItemText(m_hDlg, IDC_STREAMINFO_NETWORK, szText);

	LibISDB::AnalyzerFilter::ServiceList ServiceList;
	pAnalyzer->GetServiceList(&ServiceList);

	HWND hwndTree = ::GetDlgItem(m_hDlg, IDC_STREAMINFO_SERVICE);
	TVINSERTSTRUCT tvis;
	HTREEITEM hItem;

	TreeView_DeleteAllItems(hwndTree);

	// サービス一覧
	tvis.hParent = TVI_ROOT;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask = TVIF_STATE | TVIF_TEXT | TVIF_CHILDREN;
	tvis.item.state = TVIS_EXPANDED;
	tvis.item.stateMask = (UINT) - 1;
	tvis.item.pszText = const_cast<LPTSTR>(TEXT("サービス"));
	tvis.item.cChildren = !ServiceList.empty() ? 1 : 0;
	hItem = TreeView_InsertItem(hwndTree, &tvis);
	if (hItem != nullptr) {
		for (int i = 0; i < (int)ServiceList.size(); i++) {
			const LibISDB::AnalyzerFilter::ServiceInfo &ServiceInfo = ServiceList[i];
			WORD ServiceID, PID;

			tvis.hParent = hItem;
			tvis.item.state = 0;
			tvis.item.cChildren = 1;
			Length = StringPrintf(szText, TEXT("サービス%d"), i + 1);
			if (!ServiceInfo.ServiceName.empty())
				Length += StringPrintf(
					szText + Length, lengthof(szText) - Length,
					TEXT(" (%s)"), ServiceInfo.ServiceName.c_str());
			ServiceID = ServiceInfo.ServiceID;
			Length += StringPrintf(
				szText + Length, lengthof(szText) - Length,
				TEXT(" : SID 0x%04x (%d)"), ServiceID, ServiceID);
			if (ServiceInfo.ServiceType != LibISDB::SERVICE_TYPE_INVALID) {
				StringPrintf(
					szText + Length, lengthof(szText) - Length,
					TEXT(" / Type 0x%02x"), ServiceInfo.ServiceType);
			}
			tvis.item.pszText = szText;
			tvis.hParent = TreeView_InsertItem(hwndTree, &tvis);

			tvis.item.cChildren = 0;
			PID = ServiceInfo.PMTPID;
			StringPrintf(szText, TEXT("PMT : PID 0x%04x (%d)"), PID, PID);
			TreeView_InsertItem(hwndTree, &tvis);

			int NumVideoStreams = (int)ServiceInfo.VideoESList.size();
			for (int j = 0; j < NumVideoStreams; j++) {
				FormatEsInfo(
					TEXT("映像"), j, ServiceInfo.VideoESList[j],
					szText, lengthof(szText));
				TreeView_InsertItem(hwndTree, &tvis);
			}

			int NumAudioStreams = (int)ServiceInfo.AudioESList.size();
			for (int j = 0; j < NumAudioStreams; j++) {
				FormatEsInfo(
					TEXT("音声"), j, ServiceInfo.AudioESList[j],
					szText, lengthof(szText));
				TreeView_InsertItem(hwndTree, &tvis);
			}

			int NumCaptionStreams = (int)ServiceInfo.CaptionESList.size();
			for (int j = 0; j < NumCaptionStreams; j++) {
				PID = ServiceInfo.CaptionESList[j].PID;
				StringPrintf(
					szText,
					TEXT("字幕%d : PID 0x%04x (%d) / component tag 0x%02x"),
					j + 1, PID, PID, ServiceInfo.CaptionESList[j].ComponentTag);
				TreeView_InsertItem(hwndTree, &tvis);
			}

			int NumDataStreams = (int)ServiceInfo.DataCarrouselESList.size();
			for (int j = 0; j < NumDataStreams; j++) {
				PID = ServiceInfo.DataCarrouselESList[j].PID;
				StringPrintf(
					szText,
					TEXT("データ%d : PID 0x%04x (%d) / component tag 0x%02x"),
					j + 1, PID, PID, ServiceInfo.DataCarrouselESList[j].ComponentTag);
				TreeView_InsertItem(hwndTree, &tvis);
			}

			int NumOtherStreams = (int)ServiceInfo.OtherESList.size();
			for (int j = 0; j < NumOtherStreams; j++) {
				FormatEsInfo(
					TEXT("その他"), j, ServiceInfo.OtherESList[j],
					szText, lengthof(szText));
				TreeView_InsertItem(hwndTree, &tvis);
			}

			PID = ServiceInfo.PCRPID;
			if (PID != LibISDB::PID_INVALID) {
				StringPrintf(szText, TEXT("PCR : PID 0x%04x (%d)"), PID, PID);
				TreeView_InsertItem(hwndTree, &tvis);
			}

			int NumEcmStreams = (int)ServiceInfo.ECMList.size();
			for (int j = 0; j < NumEcmStreams; j++) {
				PID = ServiceInfo.ECMList[j].PID;
				StringPrintf(
					szText,
					TEXT("ECM%d : PID 0x%04x (%d) / CA system ID 0x%02x"),
					j + 1, PID, PID, ServiceInfo.ECMList[j].CASystemID);
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

			for (int i = 0; i < (int)ServiceList.size(); i++) {
				const LibISDB::AnalyzerFilter::ServiceInfo &ServiceInfo = ServiceList[i];

				tvis.hParent = hItem;
				tvis.item.state = 0;
				tvis.item.cChildren = 0;
				if (!ServiceInfo.ServiceName.empty())
					Length = StringPrintf(szText, TEXT("%s"), ServiceInfo.ServiceName.c_str());
				else
					Length = StringPrintf(szText, TEXT("サービス%d"), i + 1);
				StringPrintf(
					szText + Length, lengthof(szText) - Length,
					TEXT(",%d,%d,%d,%d,%d,%d,%d"),
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
					StringPrintf(
						szText,
						TEXT("TS%d : TSID 0x%04x (%d)"),
						(int)i + 1,
						TsInfo.TransportStreamID, TsInfo.TransportStreamID);
					tvis.hParent = hItem;
					tvis.item.state = 0;
					tvis.item.cChildren = 1;
					tvis.item.pszText = szText;
					tvis.hParent = TreeView_InsertItem(hwndTree, &tvis);
					tvis.item.cChildren = 0;
					for (size_t j = 0; j < TsInfo.ServiceList.size(); j++) {
						const LibISDB::AnalyzerFilter::NetworkServiceInfo &ServiceInfo = TsInfo.ServiceList[j];
						StringPrintf(
							szText,
							TEXT("サービス%d : SID 0x%04x (%d) / Type 0x%02x"),
							(int)j + 1,
							ServiceInfo.ServiceID, ServiceInfo.ServiceID,
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
					StringPrintf(
						szText,
						TEXT("TS%d : TSID 0x%04x (%d)"),
						(int)i + 1,
						TsInfo.TransportStreamID, TsInfo.TransportStreamID);
					tvis.hParent = hItem;
					tvis.item.state = 0;
					tvis.item.cChildren = 1;
					tvis.item.pszText = szText;
					tvis.hParent = TreeView_InsertItem(hwndTree, &tvis);
					tvis.item.cChildren = 0;
					for (size_t j = 0; j < TsInfo.ServiceList.size(); j++) {
						const LibISDB::AnalyzerFilter::SDTServiceInfo &ServiceInfo = TsInfo.ServiceList[j];
						StringPrintf(
							szText,
							TEXT("サービス%d (%s) : SID 0x%04x (%d) / Type 0x%02x / CA %d"),
							(int)j + 1,
							ServiceInfo.ServiceName.c_str(),
							ServiceInfo.ServiceID, ServiceInfo.ServiceID,
							ServiceInfo.ServiceType,
							ServiceInfo.FreeCAMode);
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
			for (int i = 0; i < (int)TerrestrialList.size(); i++) {
				const LibISDB::AnalyzerFilter::TerrestrialDeliverySystemInfo &Info = TerrestrialList[i];
				LPCTSTR pszArea = LibISDB::GetAreaText_ja(Info.AreaCode);

				StringPrintf(
					szText,
					TEXT("TSID 0x%04x (%d) / エリア %s / ガードインターバル %s / 伝送モード %s"),
					Info.TransportStreamID,
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
					StringPrintf(
						szText, TEXT("周波数%d : %d MHz"),
						(int)j + 1, TerrestrialList[i].Frequency[j] / 7);
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
				for (int i = 0; i < (int)SatelliteList.size(); i++) {
					const LibISDB::AnalyzerFilter::SatelliteDeliverySystemInfo &Info = SatelliteList[i];

					StringPrintf(
						szText,
						TEXT("TS%d : TSID 0x%04x (%d) / 周波数 %ld.%05ld GHz"),
						i + 1,
						Info.TransportStreamID,
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
		HTREEITEM hChild = TreeView_GetChild(hwndTree, tvi.hItem);
		if (hChild != nullptr) {
			GetTreeViewText(hwndTree, hChild, true, pText, Level + 1);
		}
		if (!fSiblings)
			break;
		tvi.hItem = TreeView_GetNextSibling(hwndTree, tvi.hItem);
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
	int m_SortColumn;
	bool m_fSortDescending;
	std::vector<PIDInfo> m_PIDInfoList;
};


const CPIDInfoPage::ColumnInfo CPIDInfoPage::m_ColumnList[NUM_COLUMNS] = {
	{TEXT("PID"),      TEXT("PID"),         4, LVCFMT_LEFT},
	{TEXT("入力"),     TEXT("Input"),       9, LVCFMT_RIGHT},
	{TEXT("出力"),     TEXT("Output"),      9, LVCFMT_RIGHT},
	{TEXT("ドロップ"), TEXT("Dropped"),     9, LVCFMT_RIGHT},
	{TEXT("エラー"),   TEXT("Error"),       9, LVCFMT_RIGHT},
//	{TEXT("暗号化"),   TEXT("Scrambled"),   9, LVCFMT_RIGHT},
	{TEXT("内容"),     TEXT("Description"), 0, LVCFMT_LEFT},
};


CPIDInfoPage::CPIDInfoPage()
	: m_SortColumn(COLUMN_PID)
	, m_fSortDescending(false)
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

			HWND hwndList = ::GetDlgItem(hDlg, IDC_PIDINFO_LIST);

			ListView_SetExtendedListViewStyle(
				hwndList,
				LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);

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
				NMLISTVIEW *pnmlv = reinterpret_cast<NMLISTVIEW*>(lParam);
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
			HWND hwndList = ::GetDlgItem(hDlg, IDC_PIDINFO_LIST);
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
	CAppMain &App = GetAppClass();
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

	HWND hwndList = ::GetDlgItem(m_hDlg, IDC_PIDINFO_LIST);
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
		PacketCountInfo PacketCount = pParser->GetPacketCount(PID);

		if (PacketCount.Input > 0) {
			if (m_PIDInfoList.size() <= static_cast<size_t>(PIDCount))
				m_PIDInfoList.emplace_back();
			PIDInfo &Info = m_PIDInfoList[PIDCount];
			Info.PID = PID;
			Info.Count = PacketCount;

			TCHAR szText[32];

			StringPrintf(szText, TEXT("%04x"), PID);

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

			StringPrintf(szText, TEXT("%llu"), PacketCount.Input);
			ListView_SetItemText(hwndList, PIDCount, COLUMN_INPUT_COUNT, szText);

			StringPrintf(szText, TEXT("%llu"), PacketCount.Output);
			ListView_SetItemText(hwndList, PIDCount, COLUMN_OUTPUT_COUNT, szText);

			StringPrintf(szText, TEXT("%llu"), PacketCount.ContinuityError);
			ListView_SetItemText(hwndList, PIDCount, COLUMN_DROPPED_COUNT, szText);

			StringPrintf(szText, TEXT("%llu"), PacketCount.FormatError + PacketCount.TransportError);
			ListView_SetItemText(hwndList, PIDCount, COLUMN_ERROR_COUNT, szText);

			//StringPrintf(szText, TEXT("%llu"), PacketCount.Scrambled);
			//ListView_SetItemText(hwndList, PIDCount, COLUMN_SCRAMBLED_COUNT, szText);

			Info.Description.clear();

			const LibISDB::CharType *pText = LibISDB::GetPredefinedPIDText(PID);
			if (pText != nullptr)
				Info.Description += pText;

			if (auto it = std::find(EMMPIDList.begin(), EMMPIDList.end(), PID); it != EMMPIDList.end())
				Info.Description += TEXT("EMM");

			for (auto &Service : ServiceList) {
				TCHAR ServiceText[8];
				StringPrintf(ServiceText, TEXT("[%04x]"), Service.ServiceID);

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
	HWND hwndList = ::GetDlgItem(m_hDlg, IDC_PIDINFO_LIST);
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
	HWND hwndList = ::GetDlgItem(m_hDlg, IDC_PIDINFO_LIST);
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
	auto CompareValue = [](auto v1, auto v2) -> int { return (v1 < v2) ? -1 : (v1 > v2) ? 1 : 0; };
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


}	// namespace




HINSTANCE CStreamInfo::m_hinst = nullptr;


bool CStreamInfo::Initialize(HINSTANCE hinst)
{
	if (m_hinst == nullptr) {
		WNDCLASS wc;

		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hinst;
		wc.hIcon = nullptr;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1);
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = STREAM_INFO_WINDOW_CLASS;

		if (::RegisterClass(&wc) == 0)
			return false;

		m_hinst = hinst;
	}

	return true;
}


CStreamInfo::CStreamInfo()
	: m_CurrentPage(0)
	, m_hwndTab(nullptr)
	, m_pEventHandler(nullptr)
	, m_fCreateFirst(true)
	, m_DefaultPageSize()
{
	m_PageList[PAGE_STREAMINFO].pszTitle = TEXT("情報");
	m_PageList[PAGE_STREAMINFO].Dialog = std::make_unique<CStreamInfoPage>();
	m_PageList[PAGE_PIDINFO   ].pszTitle = TEXT("PID");
	m_PageList[PAGE_PIDINFO   ].Dialog = std::make_unique<CPIDInfoPage>();

	for (auto &Page : m_PageList)
		RegisterUIChild(Page.Dialog.get());

	SetStyleScaling(&m_StyleScaling);
}


CStreamInfo::~CStreamInfo()
{
	Destroy();
}


bool CStreamInfo::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	if (m_fCreateFirst) {
		if (m_pEventHandler != nullptr)
			m_pEventHandler->OnRestoreSettings();
		m_fCreateFirst = false;
	}

	return CreateBasicWindow(
		hwndParent, Style, ExStyle, ID,
		STREAM_INFO_WINDOW_CLASS, STREAM_INFO_TITLE_TEXT, m_hinst);
}


void CStreamInfo::SetEventHandler(CEventHandler *pHandler)
{
	m_pEventHandler = pHandler;
}


bool CStreamInfo::IsPositionSet() const
{
	return m_WindowPosition.Width > 0;
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
		StringPrintf(szKey, TEXT("PIDInfo.Column%d.Width"), i);
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
		Settings.Write(TEXT("StreamInfoLeft"), rc.left);
		Settings.Write(TEXT("StreamInfoTop"), rc.top);
		Settings.Write(TEXT("StreamInfoWidth"), rc.right - rc.left);
		Settings.Write(TEXT("StreamInfoHeight"), rc.bottom - rc.top);
	}

	Settings.Write(TEXT("StreamInfo.ActivePage"), m_CurrentPage);

	const CPIDInfoPage *pPIDInfo = static_cast<const CPIDInfoPage*>(m_PageList[PAGE_PIDINFO].Dialog.get());
	for (int i = 0; i < CPIDInfoPage::NUM_COLUMNS; i++) {
		const int Width = pPIDInfo->GetColumnWidth(i);
		if (Width >= 0) {
			TCHAR szKey[32];
			StringPrintf(szKey, TEXT("PIDInfo.Column%d.Width"), i);
			Settings.Write(szKey, Width);
		}
	}
}


LRESULT CStreamInfo::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			InitializeUI();

			RECT rc;
			::GetClientRect(hwnd, &rc);
			m_hwndTab = ::CreateWindowEx(
				0, WC_TABCONTROL, TEXT(""),
				WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
				0, 0, rc.right, rc.bottom,
				hwnd, nullptr, m_hinst, nullptr);
			::SendMessage(m_hwndTab, WM_SETFONT, reinterpret_cast<WPARAM>(m_TabFont.GetHandle()), FALSE);

			TCITEM tci;
			tci.mask = TCIF_TEXT;
			for (int i = 0; i < NUM_PAGES; i++) {
				tci.pszText = const_cast<LPTSTR>(m_PageList[i].pszTitle);
				TabCtrl_InsertItem(m_hwndTab, i, &tci);
			}

			SetPage(m_CurrentPage);

			if (!IsPositionSet()) {
				// デフォルトのウィンドウサイズ
				RECT rc = {0, 0, m_DefaultPageSize.cx, m_DefaultPageSize.cy};
				TabCtrl_AdjustRect(m_hwndTab, TRUE, &rc);
				m_pStyleScaling->AdjustWindowRect(hwnd, &rc);
				::SetWindowPos(
					hwnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
					SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
			}
		}
		return 0;

	case WM_NCCREATE:
		InitStyleScaling(hwnd, true);
		break;

	case WM_SIZE:
		OnSizeChanged(LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_NOTIFY:
		switch (reinterpret_cast<NMHDR*>(lParam)->code) {
		case TCN_SELCHANGE:
			SetPage(TabCtrl_GetCurSel(m_hwndTab));
			return TRUE;
		}
		break;

	case WM_CLOSE:
		if ((m_pEventHandler != nullptr) && !m_pEventHandler->OnClose())
			return 0;
		break;

	case WM_DESTROY:
		{
			for (auto &Page : m_PageList)
				Page.Dialog->Destroy();

			m_hwndTab = nullptr;
		}
		break;
	}

	return CCustomWindow::OnMessage(hwnd, uMsg, wParam, lParam);
}


void CStreamInfo::ApplyStyle()
{
	if (m_hwnd != nullptr) {
		CreateDefaultFont(&m_TabFont);
	}
}


void CStreamInfo::RealizeStyle()
{
	if (m_hwnd != nullptr) {
		if (m_hwndTab != nullptr) {
			::SendMessage(m_hwndTab, WM_SETFONT, reinterpret_cast<WPARAM>(m_TabFont.GetHandle()), FALSE);
			SendSizeMessage();
		}
	}
}


void CStreamInfo::OnSizeChanged(int Width, int Height)
{
	if (m_hwndTab != nullptr) {
		::MoveWindow(m_hwndTab, 0, 0, Width, Height, TRUE);

		RECT rc;
		GetPagePosition(&rc);
		m_PageList[m_CurrentPage].Dialog->SetPosition(&rc);
	}
}


bool CStreamInfo::CreatePage(int Page)
{
	if (Page < 0 || Page >= NUM_PAGES)
		return false;

	PageInfo &Info = m_PageList[Page];

	if (Info.Dialog->IsCreated())
		return true;

	if (!Info.Dialog->Create(m_hwndTab))
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

	m_CurrentPage = Page;

	TabCtrl_SetCurSel(m_hwndTab, m_CurrentPage);

	return true;
}


void CStreamInfo::GetPagePosition(RECT *pPosition) const
{
	GetClientRect(pPosition);
	TabCtrl_AdjustRect(m_hwndTab, FALSE, pPosition);
}


}	// namespace TVTest
