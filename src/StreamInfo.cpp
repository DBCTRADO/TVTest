#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "StreamInfo.h"
#include "DialogUtil.h"
#include "BonTsEngine/TsInformation.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CStreamInfo::CStreamInfo()
	: m_pEventHandler(NULL)
	, m_fCreateFirst(true)
{
}


CStreamInfo::~CStreamInfo()
{
}


bool CStreamInfo::Create(HWND hwndOwner)
{
	if (m_fCreateFirst) {
		if (m_pEventHandler!=NULL)
			m_pEventHandler->OnRestoreSettings();
		m_fCreateFirst=false;
	}

	return CreateDialogWindow(hwndOwner,GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_STREAMINFO));
}


void CStreamInfo::SetEventHandler(CEventHandler *pHandler)
{
	m_pEventHandler=pHandler;
}


INT_PTR CStreamInfo::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	INT_PTR Result=CResizableDialog::DlgProc(hDlg,uMsg,wParam,lParam);

	switch (uMsg) {
	case WM_INITDIALOG:
		SetService();

		AddControl(IDC_STREAMINFO_STREAM,ALIGN_HORZ);
		AddControl(IDC_STREAMINFO_NETWORK,ALIGN_HORZ);
		AddControl(IDC_STREAMINFO_SERVICE,ALIGN_ALL);
		AddControl(IDC_STREAMINFO_UPDATE,ALIGN_BOTTOM_RIGHT);
		AddControl(IDC_STREAMINFO_COPY,ALIGN_BOTTOM_RIGHT);

		ApplyPosition();
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_STREAMINFO_UPDATE:
			SetService();
			return TRUE;

		case IDC_STREAMINFO_COPY:
			{
				int Length;
				LPTSTR pszText,p;
				HWND hwndTree=::GetDlgItem(hDlg,IDC_STREAMINFO_SERVICE);

				Length=0x8000;
				pszText=new TCHAR[Length];
				p=pszText;
				::GetDlgItemText(hDlg,IDC_STREAMINFO_STREAM,p,Length);
				p+=::lstrlen(p);
				*p++=_T('\r');
				*p++=_T('\n');
				::GetDlgItemText(hDlg,IDC_STREAMINFO_NETWORK,p,Length-(int)(p-pszText));
				p+=::lstrlen(p);
				*p++=_T('\r');
				*p++=_T('\n');
				GetTreeViewText(hwndTree,TreeView_GetChild(hwndTree,TreeView_GetRoot(hwndTree)),true,
								p,Length-(int)(p-pszText));
				CopyTextToClipboard(GetAppClass().UICore.GetMainWindow(),pszText);
				delete [] pszText;
			}
			return TRUE;

		case IDOK:
		case IDCANCEL:
			if (m_pEventHandler==NULL || m_pEventHandler->OnClose())
				::DestroyWindow(hDlg);
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<NMHDR*>(lParam)->code) {
		case NM_RCLICK:
			{
				NMHDR *pnmhdr=reinterpret_cast<NMHDR*>(lParam);
				//HTREEITEM hItem=TreeView_GetSelection(pnmhdr->hwndFrom);
				DWORD Pos=::GetMessagePos();
				TVHITTESTINFO tvhti;
				tvhti.pt.x=(SHORT)LOWORD(Pos);
				tvhti.pt.y=(SHORT)HIWORD(Pos);
				::ScreenToClient(pnmhdr->hwndFrom,&tvhti.pt);
				HTREEITEM hItem=TreeView_HitTest(pnmhdr->hwndFrom,&tvhti);
				if (hItem!=NULL) {
					HMENU hmenu=::CreatePopupMenu();
					::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,1,TEXT("コピー(&C)"));
					POINT pt;
					::GetCursorPos(&pt);
					switch (::TrackPopupMenu(hmenu,TPM_RIGHTBUTTON | TPM_RETURNCMD,pt.x,pt.y,0,hDlg,NULL)) {
					case 1:
						{
							int Length=0x8000;
							LPTSTR pszText=new TCHAR[Length];

							if (GetTreeViewText(pnmhdr->hwndFrom,hItem,false,pszText,Length)>0)
								CopyTextToClipboard(GetAppClass().UICore.GetMainWindow(),pszText);
							delete [] pszText;
						}
						break;
					}
				}
			}
			return TRUE;
		}
		break;
	}
	return Result;
}


static void FormatEsInfo(LPCTSTR pszName,int Index,const CTsAnalyzer::EsInfo &Es,
						 LPTSTR pszText,int MaxText)
{
	LPCTSTR pszStreamType=TsEngine::GetStreamTypeText(Es.StreamType);
	StdUtil::snprintf(pszText,MaxText,
		TEXT("%s%d : PID 0x%04x (%d) / stream type 0x%02x (%s) / component tag 0x%02x"),
		pszName,Index+1,
		Es.PID,Es.PID,
		Es.StreamType,
		pszStreamType!=NULL?pszStreamType:TEXT("?"),
		Es.ComponentTag);
}

void CStreamInfo::SetService()
{
	CDtvEngine &DtvEngine=GetAppClass().CoreEngine.m_DtvEngine;
	CTsAnalyzer *pAnalyzer=&DtvEngine.m_TsAnalyzer;
	TCHAR szText[1024];
	int Length;

	const WORD TSID=pAnalyzer->GetTransportStreamID();
	if (TSID!=0) {
		Length=StdUtil::snprintf(szText,lengthof(szText),TEXT("TSID 0x%04x (%d)"),TSID,TSID);
		TCHAR szTsName[64];
		if (pAnalyzer->GetTsName(szTsName,lengthof(szTsName))>0) {
			StdUtil::snprintf(szText+Length,lengthof(szText)-Length,TEXT(" %s"),szTsName);
		}
	} else {
		szText[0]='\0';
	}
	::SetDlgItemText(m_hDlg,IDC_STREAMINFO_STREAM,szText);

	const WORD NID=pAnalyzer->GetNetworkID();
	if (NID!=0) {
		Length=StdUtil::snprintf(szText,lengthof(szText),TEXT("NID 0x%04x (%d)"),NID,NID);
		TCHAR szName[64];
		if (pAnalyzer->GetNetworkName(szName,lengthof(szName))>0) {
			StdUtil::snprintf(szText+Length,lengthof(szText)-Length,TEXT(" %s"),szName);
		}
	} else {
		szText[0]='\0';
	}
	::SetDlgItemText(m_hDlg,IDC_STREAMINFO_NETWORK,szText);

	CTsAnalyzer::ServiceList ServiceList;
	pAnalyzer->GetServiceList(&ServiceList);

	HWND hwndTree=::GetDlgItem(m_hDlg,IDC_STREAMINFO_SERVICE);
	TVINSERTSTRUCT tvis;
	HTREEITEM hItem;

	TreeView_DeleteAllItems(hwndTree);

	// サービス一覧
	tvis.hParent=TVI_ROOT;
	tvis.hInsertAfter=TVI_LAST;
	tvis.item.mask=TVIF_STATE | TVIF_TEXT | TVIF_CHILDREN;
	tvis.item.state=TVIS_EXPANDED;
	tvis.item.stateMask=(UINT)-1;
	tvis.item.pszText=TEXT("サービス");
	tvis.item.cChildren=!ServiceList.empty()?1:0;
	hItem=TreeView_InsertItem(hwndTree,&tvis);
	if (hItem!=NULL) {
		for (int i=0;i<(int)ServiceList.size();i++) {
			const CTsAnalyzer::ServiceInfo &ServiceInfo=ServiceList[i];
			WORD ServiceID,PID;

			tvis.hParent=hItem;
			tvis.item.state=0;
			tvis.item.cChildren=1;
			Length=StdUtil::snprintf(szText,lengthof(szText),TEXT("サービス%d"),i+1);
			if (ServiceInfo.szServiceName[0]!='\0')
				Length+=StdUtil::snprintf(szText+Length,lengthof(szText)-Length,
										  TEXT(" (%s)"),ServiceInfo.szServiceName);
			ServiceID=ServiceInfo.ServiceID;
			Length+=StdUtil::snprintf(szText+Length,lengthof(szText)-Length,
									  TEXT(" : SID 0x%04x (%d)"),ServiceID,ServiceID);
			if (ServiceInfo.ServiceType!=SERVICE_TYPE_INVALID) {
				StdUtil::snprintf(szText+Length,lengthof(szText)-Length,
								  TEXT(" / Type 0x%02x"),ServiceInfo.ServiceType);
			}
			tvis.item.pszText=szText;
			tvis.hParent=TreeView_InsertItem(hwndTree,&tvis);

			tvis.item.cChildren=0;
			PID=ServiceInfo.PmtPID;
			StdUtil::snprintf(szText,lengthof(szText),TEXT("PMT : PID 0x%04x (%d)"),PID,PID);
			TreeView_InsertItem(hwndTree,&tvis);

			int NumVideoStreams=(int)ServiceInfo.VideoEsList.size();
			for (int j=0;j<NumVideoStreams;j++) {
				FormatEsInfo(TEXT("映像"),j,ServiceInfo.VideoEsList[j],
							 szText,lengthof(szText));
				TreeView_InsertItem(hwndTree,&tvis);
			}

			int NumAudioStreams=(int)ServiceInfo.AudioEsList.size();
			for (int j=0;j<NumAudioStreams;j++) {
				FormatEsInfo(TEXT("音声"),j,ServiceInfo.AudioEsList[j],
							 szText,lengthof(szText));
				TreeView_InsertItem(hwndTree,&tvis);
			}

			int NumCaptionStreams=(int)ServiceInfo.CaptionEsList.size();
			for (int j=0;j<NumCaptionStreams;j++) {
				PID=ServiceInfo.CaptionEsList[j].PID;
				StdUtil::snprintf(szText,lengthof(szText),
								  TEXT("字幕%d : PID 0x%04x (%d) / component tag 0x%02x"),
								  j+1,PID,PID,ServiceInfo.CaptionEsList[j].ComponentTag);
				TreeView_InsertItem(hwndTree,&tvis);
			}

			int NumDataStreams=(int)ServiceInfo.DataCarrouselEsList.size();
			for (int j=0;j<NumDataStreams;j++) {
				PID=ServiceInfo.DataCarrouselEsList[j].PID;
				StdUtil::snprintf(szText,lengthof(szText),
								  TEXT("データ%d : PID 0x%04x (%d) / component tag 0x%02x"),
								  j+1,PID,PID,ServiceInfo.DataCarrouselEsList[j].ComponentTag);
				TreeView_InsertItem(hwndTree,&tvis);
			}

			int NumOtherStreams=(int)ServiceInfo.OtherEsList.size();
			for (int j=0;j<NumOtherStreams;j++) {
				FormatEsInfo(TEXT("その他"),j,ServiceInfo.OtherEsList[j],
							 szText,lengthof(szText));
				TreeView_InsertItem(hwndTree,&tvis);
			}

			PID=ServiceInfo.PcrPID;
			if (PID!=CTsAnalyzer::PID_INVALID) {
				StdUtil::snprintf(szText,lengthof(szText),TEXT("PCR : PID 0x%04x (%d)"),PID,PID);
				TreeView_InsertItem(hwndTree,&tvis);
			}

			int NumEcmStreams=(int)ServiceInfo.EcmList.size();
			for (int j=0;j<NumEcmStreams;j++) {
				PID=ServiceInfo.EcmList[j].PID;
				StdUtil::snprintf(szText,lengthof(szText),
								  TEXT("ECM%d : PID 0x%04x (%d) / CA system ID 0x%02x"),
								  j+1,PID,PID,ServiceInfo.EcmList[j].CaSystemID);
				TreeView_InsertItem(hwndTree,&tvis);
			}
		}
	}

	// チャンネルファイル用フォーマット一覧
	const CChannelInfo *pChannelInfo=GetAppClass().ChannelManager.GetCurrentChannelInfo();
	if (pChannelInfo!=NULL) {
		tvis.hParent=TVI_ROOT;
		tvis.hInsertAfter=TVI_LAST;
		tvis.item.mask=TVIF_STATE | TVIF_TEXT | TVIF_CHILDREN;
		tvis.item.state=TVIS_EXPANDED;
		tvis.item.stateMask=~0U;
		tvis.item.pszText=TEXT("チャンネルファイル用フォーマット");
		tvis.item.cChildren=!ServiceList.empty()?1:0;;
		hItem=TreeView_InsertItem(hwndTree,&tvis);
		if (hItem!=NULL) {
			const int RemoteControlKeyID=pAnalyzer->GetRemoteControlKeyID();

			for (int i=0;i<(int)ServiceList.size();i++) {
				const CTsAnalyzer::ServiceInfo &ServiceInfo=ServiceList[i];

				tvis.hParent=hItem;
				tvis.item.state=0;
				tvis.item.cChildren=0;
				if (ServiceInfo.szServiceName[0]!='\0')
					Length=StdUtil::snprintf(szText,lengthof(szText),TEXT("%s"),ServiceInfo.szServiceName);
				else
					Length=StdUtil::snprintf(szText,lengthof(szText),TEXT("サービス%d"),i+1);
				StdUtil::snprintf(szText+Length,lengthof(szText)-Length,
								  TEXT(",%d,%d,%d,%d,%d,%d,%d"),
								  pChannelInfo->GetSpace(),
								  pChannelInfo->GetChannelIndex(),
								  RemoteControlKeyID,
								  ServiceInfo.ServiceType,
								  ServiceInfo.ServiceID,NID,TSID);
				tvis.item.pszText=szText;
				TreeView_InsertItem(hwndTree,&tvis);
			}
		}
	}

	// ネットワークTS一覧
	CTsAnalyzer::NetworkTsList TsList;
	if (pAnalyzer->GetNetworkTsList(&TsList) && !TsList.empty()) {
		tvis.hParent=TVI_ROOT;
		tvis.hInsertAfter=TVI_LAST;
		tvis.item.mask=TVIF_STATE | TVIF_TEXT | TVIF_CHILDREN;
		tvis.item.state=0;
		tvis.item.stateMask=~0U;
		tvis.item.pszText=TEXT("ネットワークTS (NIT)");
		tvis.item.cChildren=1;
		hItem=TreeView_InsertItem(hwndTree,&tvis);
		if (hItem!=NULL) {
			for (size_t i=0;i<TsList.size();i++) {
				const CTsAnalyzer::NetworkTsInfo &TsInfo=TsList[i];

				if (TsInfo.OriginalNetworkID==NID) {
					StdUtil::snprintf(szText,lengthof(szText),
						TEXT("TS%d : TSID 0x%04x (%d)"),
						(int)i+1,
						TsInfo.TransportStreamID,TsInfo.TransportStreamID);
					tvis.hParent=hItem;
					tvis.item.state=0;
					tvis.item.cChildren=1;
					tvis.item.pszText=szText;
					tvis.hParent=TreeView_InsertItem(hwndTree,&tvis);
					tvis.item.cChildren=0;
					for (size_t j=0;j<TsInfo.ServiceList.size();j++) {
						const CTsAnalyzer::NetworkServiceInfo &ServiceInfo=TsInfo.ServiceList[j];
						StdUtil::snprintf(szText,lengthof(szText),
							TEXT("サービス%d : SID 0x%04x (%d) / Type 0x%02x"),
							(int)j+1,
							ServiceInfo.ServiceID,ServiceInfo.ServiceID,
							ServiceInfo.ServiceType);
						TreeView_InsertItem(hwndTree,&tvis);
					}
				}
			}
		}
	}

	// ネットワークサービス一覧
	CTsAnalyzer::SdtTsList SdtList;
	if (pAnalyzer->GetSdtTsList(&SdtList) && !SdtList.empty()) {
		tvis.hParent=TVI_ROOT;
		tvis.hInsertAfter=TVI_LAST;
		tvis.item.mask=TVIF_STATE | TVIF_TEXT | TVIF_CHILDREN;
		tvis.item.state=0;
		tvis.item.stateMask=~0U;
		tvis.item.pszText=TEXT("ネットワークサービス (SDT)");
		tvis.item.cChildren=1;
		hItem=TreeView_InsertItem(hwndTree,&tvis);
		if (hItem!=NULL) {
			for (size_t i=0;i<SdtList.size();i++) {
				const CTsAnalyzer::SdtTsInfo &TsInfo=SdtList[i];

				if (TsInfo.OriginalNetworkID==NID) {
					StdUtil::snprintf(szText,lengthof(szText),
						TEXT("TS%d : TSID 0x%04x (%d)"),
						(int)i+1,
						TsInfo.TransportStreamID,TsInfo.TransportStreamID);
					tvis.hParent=hItem;
					tvis.item.state=0;
					tvis.item.cChildren=1;
					tvis.item.pszText=szText;
					tvis.hParent=TreeView_InsertItem(hwndTree,&tvis);
					tvis.item.cChildren=0;
					for (size_t j=0;j<TsInfo.ServiceList.size();j++) {
						const CTsAnalyzer::SdtServiceInfo &ServiceInfo=TsInfo.ServiceList[j];
						StdUtil::snprintf(szText,lengthof(szText),
							TEXT("サービス%d (%s) : SID 0x%04x (%d) / Type 0x%02x / CA %d"),
							(int)j+1,
							ServiceInfo.szServiceName,
							ServiceInfo.ServiceID,ServiceInfo.ServiceID,
							ServiceInfo.ServiceType,
							ServiceInfo.bFreeCaMode);
						TreeView_InsertItem(hwndTree,&tvis);
					}
				}
			}
		}
	}

	// 地上/衛星分配システム
	CTsAnalyzer::TerrestrialDeliverySystemList TerrestrialList;
	if (pAnalyzer->GetTerrestrialDeliverySystemList(&TerrestrialList)) {
		tvis.hParent=TVI_ROOT;
		tvis.hInsertAfter=TVI_LAST;
		tvis.item.mask=TVIF_STATE | TVIF_TEXT | TVIF_CHILDREN;
		tvis.item.state=TVIS_EXPANDED;
		tvis.item.stateMask=~0U;
		tvis.item.pszText=TEXT("地上分配システム");
		tvis.item.cChildren=!TerrestrialList.empty()?1:0;
		hItem=TreeView_InsertItem(hwndTree,&tvis);
		if (hItem!=NULL) {
			for (int i=0;i<(int)TerrestrialList.size();i++) {
				const CTsAnalyzer::TerrestrialDeliverySystemInfo &Info=TerrestrialList[i];
				LPCTSTR pszArea=TsEngine::GetAreaText(Info.AreaCode);

				StdUtil::snprintf(szText,lengthof(szText),
					TEXT("TSID 0x%04x (%d) / エリア %s / ガードインターバル %s / 伝送モード %s"),
					Info.TransportStreamID,
					Info.TransportStreamID,
					pszArea!=NULL?pszArea:TEXT("?"),
					Info.GuardInterval==0?TEXT("1/32"):
					Info.GuardInterval==1?TEXT("1/16"):
					Info.GuardInterval==2?TEXT("1/8"):
					Info.GuardInterval==3?TEXT("1/4"):TEXT("?"),
					Info.TransmissionMode==0?TEXT("Mode1"):
					Info.TransmissionMode==1?TEXT("Mode2"):
					Info.TransmissionMode==2?TEXT("Mode3"):TEXT("?"));
				tvis.hParent=hItem;
				tvis.item.state=0;
				tvis.item.cChildren=1;
				tvis.item.pszText=szText;
				tvis.hParent=TreeView_InsertItem(hwndTree,&tvis);
				tvis.item.cChildren=0;
				for (size_t j=0;j<TerrestrialList[i].Frequency.size();j++) {
					StdUtil::snprintf(szText,lengthof(szText),TEXT("周波数%d : %d MHz"),
									  (int)j+1,TerrestrialList[i].Frequency[j]/7);
					TreeView_InsertItem(hwndTree,&tvis);
				}
			}
		}
	} else {
		CTsAnalyzer::SatelliteDeliverySystemList SatelliteList;
		if (pAnalyzer->GetSatelliteDeliverySystemList(&SatelliteList)) {
			tvis.hParent=TVI_ROOT;
			tvis.hInsertAfter=TVI_LAST;
			tvis.item.mask=TVIF_STATE | TVIF_TEXT | TVIF_CHILDREN;
			tvis.item.state=0;
			tvis.item.stateMask=~0U;
			tvis.item.pszText=TEXT("衛星分配システム");
			tvis.item.cChildren=!SatelliteList.empty()?1:0;
			hItem=TreeView_InsertItem(hwndTree,&tvis);
			if (hItem!=NULL) {
				for (int i=0;i<(int)SatelliteList.size();i++) {
					const CTsAnalyzer::SatelliteDeliverySystemInfo &Info=SatelliteList[i];

					StdUtil::snprintf(szText,lengthof(szText),
									  TEXT("TS%d : TSID 0x%04x (%d) / 周波数 %ld.%05ld GHz"),
									  i+1,
									  Info.TransportStreamID,
									  Info.TransportStreamID,
									  Info.Frequency/100000,
									  Info.Frequency%100000);
					tvis.hParent=hItem;
					tvis.item.state=0;
					tvis.item.cChildren=0;
					tvis.item.pszText=szText;
					TreeView_InsertItem(hwndTree,&tvis);
				}
			}
		}
	}
}


int CStreamInfo::GetTreeViewText(HWND hwndTree,HTREEITEM hItem,bool fSiblings,LPTSTR pszText,int MaxText,int Level)
{
	if (MaxText<=0)
		return 0;

	TV_ITEM tvi;
	LPTSTR p=pszText;

	tvi.mask=TVIF_TEXT;
	tvi.hItem=hItem;
	while (tvi.hItem!=NULL && MaxText>2) {
		if (Level>0) {
			if (MaxText<=Level)
				break;
			for (int i=0;i<Level;i++)
				*p++=_T('\t');
			MaxText-=Level;
		}
		tvi.pszText=p;
		tvi.cchTextMax=MaxText;
		if (TreeView_GetItem(hwndTree,&tvi)) {
			int Len=::lstrlen(p);
			p+=Len;
			*p++=_T('\r');
			*p++=_T('\n');
			MaxText-=Len+2;
		}
		HTREEITEM hChild=TreeView_GetChild(hwndTree,tvi.hItem);
		if (hChild!=NULL) {
			int Length=GetTreeViewText(hwndTree,hChild,true,p,MaxText,Level+1);
			p+=Length;
			MaxText-=Length;
		}
		if (!fSiblings)
			break;
		tvi.hItem=TreeView_GetNextSibling(hwndTree,tvi.hItem);
	}
	*p=_T('\0');
	return (int)(p-pszText);
}
