#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "ChannelScan.h"
#include "DialogUtil.h"
#include "MessageDialog.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// 信号レベルを考慮する場合の閾値
#define SIGNAL_LEVEL_THRESHOLD	7.0f

// スキャンスレッドから送られるメッセージ
#define WM_APP_BEGINSCAN	(WM_APP+0)
#define WM_APP_CHANNELFOUND	(WM_APP+1)
#define WM_APP_ENDCHANNEL	(WM_APP+2)
#define WM_APP_ENDSCAN		(WM_APP+3)

// スキャン結果
enum ScanResult
{
	SCAN_RESULT_SUCCEEDED,
	SCAN_RESULT_CANCELLED,
	SCAN_RESULT_SET_CHANNEL_PARTIALLY_FAILED,
	SCAN_RESULT_SET_CHANNEL_TIMEOUT
};




class CChannelPropDialog : public CBasicDialog
{
public:
	CChannelPropDialog(CChannelInfo *pChannelInfo);
	~CChannelPropDialog();
	bool Show(HWND hwndOwner) override;

private:
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

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
	return ShowDialog(hwndOwner,
					  GetAppClass().GetResourceInstance(),
					  MAKEINTRESOURCE(IDD_CHANNELPROPERTIES))==IDOK;
}


INT_PTR CChannelPropDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			TCHAR szText[64];

			::SetProp(hDlg,TEXT("ChannelInfo"),m_pChannelInfo);
			::SendDlgItemMessage(hDlg,IDC_CHANNELPROP_NAME,EM_LIMITTEXT,MAX_CHANNEL_NAME-1,0);
			::SetDlgItemText(hDlg,IDC_CHANNELPROP_NAME,m_pChannelInfo->GetName());
			for (int i=1;i<=12;i++) {
				::wsprintf(szText,TEXT("%d"),i);
				DlgComboBox_AddString(hDlg,IDC_CHANNELPROP_CONTROLKEY,szText);
			}
			if (m_pChannelInfo->GetChannelNo()>0)
				::SetDlgItemInt(hDlg,IDC_CHANNELPROP_CONTROLKEY,m_pChannelInfo->GetChannelNo(),TRUE);
			::wsprintf(szText,TEXT("%d (%#04x)"),
					   m_pChannelInfo->GetNetworkID(),m_pChannelInfo->GetNetworkID());
			::SetDlgItemText(hDlg,IDC_CHANNELPROP_NETWORKID,szText);
			::wsprintf(szText,TEXT("%d (%#04x)"),
					   m_pChannelInfo->GetTransportStreamID(),m_pChannelInfo->GetTransportStreamID());
			::SetDlgItemText(hDlg,IDC_CHANNELPROP_TSID,szText);
			::wsprintf(szText,TEXT("%d (%#04x)"),
					   m_pChannelInfo->GetServiceID(),m_pChannelInfo->GetServiceID());
			::SetDlgItemText(hDlg,IDC_CHANNELPROP_SERVICEID,szText);
			::SetDlgItemInt(hDlg,IDC_CHANNELPROP_TUNINGSPACE,m_pChannelInfo->GetSpace(),TRUE);
			::SetDlgItemInt(hDlg,IDC_CHANNELPROP_CHANNELINDEX,m_pChannelInfo->GetChannelIndex(),TRUE);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			{
				bool fModified=false;
				TCHAR szName[MAX_CHANNEL_NAME];
				int ControlKey;

				::GetDlgItemText(hDlg,IDC_CHANNELPROP_NAME,szName,lengthof(szName));
				if (szName[0]=='\0') {
					::MessageBox(hDlg,TEXT("名前を入力してください。"),TEXT("お願い"),MB_OK | MB_ICONINFORMATION);
					::SetFocus(::GetDlgItem(hDlg,IDC_CHANNELPROP_NAME));
					return TRUE;
				}
				if (::lstrcmp(m_pChannelInfo->GetName(),szName)!=0) {
					m_pChannelInfo->SetName(szName);
					fModified=true;
				}
				ControlKey=::GetDlgItemInt(hDlg,IDC_CHANNELPROP_CONTROLKEY,NULL,TRUE);
				if (ControlKey!=m_pChannelInfo->GetChannelNo()) {
					m_pChannelInfo->SetChannelNo(ControlKey);
					fModified=true;
				}
				if (!fModified)
					wParam=IDCANCEL;
			}
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;
	}

	return FALSE;
}




// チャンネルリストのソートクラス
class CChannelListSort
{
	static int CALLBACK CompareFunc(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);

public:
	enum SortType {
		SORT_NAME,
		SORT_CHANNEL,
		SORT_CHANNELNAME,
		SORT_SERVICEID,
		SORT_REMOTECONTROLKEYID
	};

	CChannelListSort();
	CChannelListSort(SortType Type,bool fDescending=false);
	void Sort(HWND hwndList);
	bool UpdateChannelList(HWND hwndList,CChannelList *pList);

	SortType m_Type;
	bool m_fDescending;
};


CChannelListSort::CChannelListSort()
	: m_Type(SORT_CHANNEL)
	, m_fDescending(false)
{
}


CChannelListSort::CChannelListSort(SortType Type,bool fDescending)
	: m_Type(Type)
	, m_fDescending(fDescending)
{
}


int CALLBACK CChannelListSort::CompareFunc(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CChannelListSort *pThis=reinterpret_cast<CChannelListSort*>(lParamSort);
	CChannelInfo *pChInfo1=reinterpret_cast<CChannelInfo*>(lParam1);
	CChannelInfo *pChInfo2=reinterpret_cast<CChannelInfo*>(lParam2);
	int Cmp;

	switch (pThis->m_Type) {
	case SORT_NAME:
		Cmp=::lstrcmpi(pChInfo1->GetName(),pChInfo2->GetName());
		if (Cmp==0)
			Cmp=::lstrcmp(pChInfo1->GetName(),pChInfo2->GetName());
		break;

	case SORT_CHANNEL:
		Cmp=pChInfo1->GetChannelIndex()-pChInfo2->GetChannelIndex();
		break;

	case SORT_CHANNELNAME:
		Cmp=pChInfo1->GetChannelIndex()-pChInfo2->GetChannelIndex();
		break;

	case SORT_SERVICEID:
		{
			NetworkType Ch1Network=GetNetworkType(pChInfo1->GetNetworkID());
			NetworkType Ch2Network=GetNetworkType(pChInfo2->GetNetworkID());
			if (Ch1Network!=Ch2Network) {
				Cmp=Ch1Network-Ch2Network;
			} else {
				Cmp=pChInfo1->GetServiceID()-pChInfo2->GetServiceID();
			}
		}
		break;

	case SORT_REMOTECONTROLKEYID:
		Cmp=pChInfo1->GetChannelNo()-pChInfo2->GetChannelNo();
		break;

	default:
		Cmp=0;
	}

	return pThis->m_fDescending?-Cmp:Cmp;
}


void CChannelListSort::Sort(HWND hwndList)
{
	ListView_SortItems(hwndList,CompareFunc,reinterpret_cast<LPARAM>(this));
}


bool CChannelListSort::UpdateChannelList(HWND hwndList,CChannelList *pList)
{
	if (pList==NULL)
		return false;

	CChannelList ChannelList;
	int Count=ListView_GetItemCount(hwndList);
	LVITEM lvi;

	lvi.mask=LVIF_PARAM;
	lvi.iSubItem=0;

	for (int i=0;i<Count;i++) {
		lvi.iItem=i;
		ListView_GetItem(hwndList,&lvi);
		ChannelList.AddChannel(*reinterpret_cast<const CChannelInfo*>(lvi.lParam));
	}

	*pList=ChannelList;

	for (int i=0;i<Count;i++) {
		lvi.iItem=i;
		lvi.lParam=reinterpret_cast<LPARAM>(pList->GetChannelInfo(i));
		ListView_SetItem(hwndList,&lvi);
	}

	return true;
}




CChannelScan::CChannelScan()
	: m_pOriginalTuningSpaceList(NULL)
	, m_fScanService(true)
	, m_fIgnoreSignalLevel(false)	// 信号レベルを無視
	, m_ScanWait(5000)				// チャンネル切り替え後の待ち時間(ms)
	, m_RetryCount(4)				// 情報取得の再試行回数
	, m_RetryInterval(1000)			// 再試行の間隔(ms)
	, m_hScanDlg(NULL)
	, m_hScanThread(NULL)
	, m_hCancelEvent(NULL)
	, m_fChanging(false)
{
}


CChannelScan::~CChannelScan()
{
	Destroy();
}


bool CChannelScan::Apply(DWORD Flags)
{
	CAppMain &AppMain=GetAppClass();

	if ((Flags&UPDATE_CHANNELLIST)!=0) {
		AppMain.UpdateCurrentChannelList(&m_TuningSpaceList);
	}

	if ((Flags&UPDATE_PREVIEW)!=0) {
		if (m_fRestorePreview)
			AppMain.GetUICore()->EnableViewer(true);
	}

	return true;
}


bool CChannelScan::ReadSettings(CSettings &Settings)
{
	Settings.Read(TEXT("ChannelScanIgnoreSignalLevel"),&m_fIgnoreSignalLevel);
	Settings.Read(TEXT("ChannelScanWait"),&m_ScanWait);
	Settings.Read(TEXT("ChannelScanRetry"),&m_RetryCount);
	return true;
}


bool CChannelScan::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("ChannelScanIgnoreSignalLevel"),m_fIgnoreSignalLevel);
	Settings.Write(TEXT("ChannelScanWait"),m_ScanWait);
	Settings.Write(TEXT("ChannelScanRetry"),m_RetryCount);
	return true;
}


bool CChannelScan::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_CHANNELSCAN));
}


bool CChannelScan::SetTuningSpaceList(const CTuningSpaceList *pTuningSpaceList)
{
	m_pOriginalTuningSpaceList=pTuningSpaceList;
	return true;
}


bool CChannelScan::AutoUpdateChannelList(CTuningSpaceList *pTuningSpaceList,std::vector<TVTest::String> *pMessageList)
{
	// スキャンされたチャンネルリストの自動更新(テスト)
	// CSはネットワークが複数に分かれているのでこのままじゃ駄目

	if (pTuningSpaceList==NULL)
		return false;

	CTsAnalyzer *pTsAnalyzer=&GetAppClass().GetCoreEngine()->m_DtvEngine.m_TsAnalyzer;
	CTsAnalyzer::SdtTsList TsList;

	if (!pTsAnalyzer->IsSdtComplete()
			|| !pTsAnalyzer->GetSdtTsList(&TsList)
			|| TsList.empty())
		return false;

	bool fUpdated=false;
	TVTest::String Message;

	// 現在のチャンネルリストに存在しないサービスを探す
	for (size_t TsIndex=0;TsIndex<TsList.size();TsIndex++) {
		const CTsAnalyzer::SdtTsInfo &TsInfo=TsList[TsIndex];

		for (size_t ServiceIndex=0;ServiceIndex<TsInfo.ServiceList.size();ServiceIndex++) {
			const CTsAnalyzer::SdtServiceInfo &ServiceInfo=TsInfo.ServiceList[ServiceIndex];

			if (!IsScanService(ServiceInfo,false))
				continue;

			bool fFound=false;

			for (int i=0;i<pTuningSpaceList->NumSpaces();i++) {
				CChannelList *pChannelList=pTuningSpaceList->GetChannelList(i);

				if (pChannelList!=NULL) {
					if (pChannelList->FindByIDs(TsInfo.OriginalNetworkID,0,ServiceInfo.ServiceID)>=0) {
						fFound=true;
						break;
					}
				}
			}

			if (!fFound) {
				// 新規サービス
				bool fInserted=false;

				for (int Space=0;Space<pTuningSpaceList->NumSpaces();Space++) {
					CChannelList *pChannelList=pTuningSpaceList->GetChannelList(Space);

					if (pChannelList!=NULL) {
						int Index=pChannelList->FindByIDs(TsInfo.OriginalNetworkID,TsInfo.TransportStreamID,0);
						if (Index>=0) {
							CChannelInfo ChannelInfo(*pChannelList->GetChannelInfo(Index));

							ChannelInfo.SetChannelNo(ServiceInfo.ServiceID);
							ChannelInfo.SetName(ServiceInfo.szServiceName);
							ChannelInfo.SetServiceID(ServiceInfo.ServiceID);
							ChannelInfo.Enable(true);

							for (;Index<pChannelList->NumChannels();Index++) {
								const CChannelInfo *pChannelInfo=pChannelList->GetChannelInfo(Index);

								if (pChannelInfo->GetSpace()!=ChannelInfo.GetSpace()
										|| pChannelInfo->GetChannelIndex()!=ChannelInfo.GetChannelIndex()
										|| pChannelInfo->GetNetworkID()!=TsInfo.OriginalNetworkID
										|| pChannelInfo->GetTransportStreamID()!=TsInfo.TransportStreamID
										|| pChannelInfo->GetServiceID()>ServiceInfo.ServiceID)
									break;
							}

							pChannelList->InsertChannel(Index,ChannelInfo);

							if (pMessageList!=NULL) {
								TVTest::StringUtility::Format(Message,
									TEXT("新しいサービス %d \"%s\" (NID %d TSID 0x%04x) を追加しました。"),
									ChannelInfo.GetServiceID(),
									ChannelInfo.GetName(),
									ChannelInfo.GetNetworkID(),
									ChannelInfo.GetTransportStreamID());
								pMessageList->push_back(Message);
							}

							fInserted=true;
							fUpdated=true;
							break;
						}
					}
				}

				if (!fInserted && pMessageList!=NULL) {
					TVTest::StringUtility::Format(Message,
						TEXT("新しいサービス %d \"%s\" (NID %d TSID 0x%04x) が検出されましたが、当該 TS が見付かりません。"),
						ServiceInfo.ServiceID,
						ServiceInfo.szServiceName,
						TsInfo.OriginalNetworkID,
						TsInfo.TransportStreamID);
					pMessageList->push_back(Message);
				}
			}
		}
	}

	// チャンネルリストから既に存在しないサービスを探す
	for (int Space=0;Space<pTuningSpaceList->NumSpaces();Space++) {
		CChannelList *pChannelList=pTuningSpaceList->GetChannelList(Space);

		if (pChannelList!=NULL) {
			for (int Channel=0;Channel<pChannelList->NumChannels();) {
				const CChannelInfo *pChannelInfo=pChannelList->GetChannelInfo(Channel);
				bool fNetworkFound=false,fServiceFound=false,fServiceMoved=false;

				for (size_t TsIndex=0;TsIndex<TsList.size();TsIndex++) {
					const CTsAnalyzer::SdtTsInfo &TsInfo=TsList[TsIndex];

					if (TsInfo.OriginalNetworkID==pChannelInfo->GetNetworkID()) {
						fNetworkFound=true;

						for (size_t ServiceIndex=0;ServiceIndex<TsInfo.ServiceList.size();ServiceIndex++) {
							const CTsAnalyzer::SdtServiceInfo &ServiceInfo=TsInfo.ServiceList[ServiceIndex];

							if (ServiceInfo.ServiceID==pChannelInfo->GetServiceID()) {
								fServiceFound=true;

								if (TsInfo.TransportStreamID!=pChannelInfo->GetTransportStreamID()) {
									// TS移動
									int Index=pChannelList->FindByIDs(TsInfo.OriginalNetworkID,TsInfo.TransportStreamID,0);
									if (Index>=0) {
										const CChannelInfo *pMoveChInfo=pChannelList->GetChannelInfo(Index);
										CChannelInfo ChannelInfo(*pChannelInfo);

										ChannelInfo.SetSpace(pMoveChInfo->GetSpace());
										ChannelInfo.SetChannelIndex(pMoveChInfo->GetChannelIndex());
										ChannelInfo.SetTransportStreamID(TsInfo.TransportStreamID);

										for (;Index<pChannelList->NumChannels();Index++) {
											const CChannelInfo *pChInfo=pChannelList->GetChannelInfo(Index);

											if (pChInfo->GetSpace()!=ChannelInfo.GetSpace()
													|| pChInfo->GetChannelIndex()!=ChannelInfo.GetChannelIndex()
													|| pChInfo->GetNetworkID()!=TsInfo.OriginalNetworkID
													|| pChInfo->GetTransportStreamID()!=TsInfo.TransportStreamID
													|| pChInfo->GetServiceID()>ServiceInfo.ServiceID)
												break;
										}

										pChannelList->InsertChannel(Index,ChannelInfo);

										if (pMessageList!=NULL) {
											TVTest::StringUtility::Format(Message,
												TEXT("サービス %d \"%s\" が TS 0x%04x から 0x%04x に移動しました。"),
												ChannelInfo.GetServiceID(),
												ChannelInfo.GetName(),
												pChannelInfo->GetTransportStreamID(),
												ChannelInfo.GetTransportStreamID());
											pMessageList->push_back(Message);
										}

										fServiceMoved=true;
										fUpdated=false;
									} else {
										if (pMessageList!=NULL) {
											TVTest::StringUtility::Format(Message,
												TEXT("サービス %d \"%s\" の TS 0x%04x から 0x%04x への移動を検出しましたが、移動先 TS が見付かりません。"),
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
					if (!fServiceMoved && pMessageList!=NULL) {
						TVTest::StringUtility::Format(Message,
							TEXT("サービス %d \"%s\" は削除されました。"),
							pChannelInfo->GetServiceID(),
							pChannelInfo->GetName());
						pMessageList->push_back(Message);
					}

					pChannelList->DeleteChannel(Channel);

					fUpdated=true;
				}
			}
		}
	}

	return fUpdated;
}


void CChannelScan::InsertChannelInfo(int Index,const CChannelInfo *pChInfo)
{
	HWND hwndList=::GetDlgItem(m_hDlg,IDC_CHANNELSCAN_CHANNELLIST);
	LV_ITEM lvi;
	TCHAR szText[256];

	lvi.mask=LVIF_TEXT | LVIF_PARAM;
	lvi.iItem=Index;
	lvi.iSubItem=0;
	lvi.pszText=const_cast<LPTSTR>(pChInfo->GetName());
	lvi.lParam=reinterpret_cast<LPARAM>(pChInfo);
	lvi.iItem=ListView_InsertItem(hwndList,&lvi);
	lvi.mask=LVIF_TEXT;
	lvi.iSubItem=1;
	::wsprintf(szText,TEXT("%d"),pChInfo->GetChannelIndex());
	lvi.pszText=szText;
	ListView_SetItem(hwndList,&lvi);
	lvi.iSubItem=2;
	LPCTSTR pszChannelName=GetAppClass().GetCoreEngine()->m_DtvEngine.m_BonSrcDecoder.GetChannelName(pChInfo->GetSpace(),pChInfo->GetChannelIndex());
	::lstrcpyn(szText,!IsStringEmpty(pszChannelName)?pszChannelName:TEXT("\?\?\?"),lengthof(szText));
	ListView_SetItem(hwndList,&lvi);
	lvi.iSubItem=3;
	::wsprintf(szText,TEXT("%d"),pChInfo->GetServiceID());
	ListView_SetItem(hwndList,&lvi);
	if (pChInfo->GetChannelNo()>0) {
		lvi.iSubItem=4;
		::wsprintf(szText,TEXT("%d"),pChInfo->GetChannelNo());
		lvi.pszText=szText;
		ListView_SetItem(hwndList,&lvi);
	}
	ListView_SetCheckState(hwndList,lvi.iItem,pChInfo->IsEnabled());
}


void CChannelScan::SetChannelList(int Space)
{
	const CChannelList *pChannelList=m_TuningSpaceList.GetChannelList(Space);

	ListView_DeleteAllItems(::GetDlgItem(m_hDlg,IDC_CHANNELSCAN_CHANNELLIST));
	if (pChannelList==NULL)
		return;
	m_fChanging=true;
	for (int i=0;i<pChannelList->NumChannels();i++)
		InsertChannelInfo(i,pChannelList->GetChannelInfo(i));
	m_fChanging=false;
}


CChannelInfo *CChannelScan::GetSelectedChannelInfo() const
{
	HWND hwndList=::GetDlgItem(m_hDlg,IDC_CHANNELSCAN_CHANNELLIST);
	LV_ITEM lvi;

	lvi.iItem=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
	if (lvi.iItem>=0) {
		lvi.mask=LVIF_PARAM;
		lvi.iSubItem=0;
		if (ListView_GetItem(hwndList,&lvi))
			return reinterpret_cast<CChannelInfo*>(lvi.lParam);
	}
	return NULL;
}


bool CChannelScan::LoadPreset(LPCTSTR pszFileName,CChannelList *pChannelList,int Space,bool *pfCorrupted)
{
	TCHAR szPresetFileName[MAX_PATH];
	CTuningSpaceList PresetList;

	GetAppClass().GetAppDirectory(szPresetFileName);
	::PathAppend(szPresetFileName,pszFileName);
	if (!PresetList.LoadFromFile(szPresetFileName))
		return false;

	CBonSrcDecoder &BonSrcDecoder=GetAppClass().GetCoreEngine()->m_DtvEngine.m_BonSrcDecoder;
	std::vector<CDynamicString> BonDriverChannelList;
	LPCTSTR pszName;
	for (int i=0;(pszName=BonSrcDecoder.GetChannelName(Space,i))!=NULL;i++) {
		BonDriverChannelList.push_back(CDynamicString(pszName));
	}
	if (BonDriverChannelList.empty())
		return false;

	bool fCorrupted=false;
	CChannelList *pPresetChannelList=PresetList.GetAllChannelList();
	for (int i=0;i<pPresetChannelList->NumChannels();i++) {
		CChannelInfo ChannelInfo(*pPresetChannelList->GetChannelInfo(i));

		LPCTSTR pszChannelName=ChannelInfo.GetName(),pszDelimiter;
		if (pszChannelName[0]==_T('%')
				&& (pszDelimiter=::StrChr(pszChannelName+1,_T(' ')))!=NULL) {
			int TSNameLength=(int)(pszDelimiter-pszChannelName)-1;
			TCHAR szName[MAX_CHANNEL_NAME];
			::lstrcpyn(szName,pszChannelName+1,TSNameLength+1);
			bool fFound=false;
			for (size_t j=0;j<BonDriverChannelList.size();j++) {
				pszName=BonDriverChannelList[j].Get();
				LPCTSTR p=::StrStrI(pszName,szName);
				if (p!=NULL && !::IsCharAlphaNumeric(p[TSNameLength])) {
					ChannelInfo.SetChannelIndex((int)j);
					fFound=true;
					break;
				}
			}
			if (!fFound) {
				if (ChannelInfo.GetChannelIndex()>=(int)BonDriverChannelList.size())
					continue;
				fCorrupted=true;
			}
			::lstrcpy(szName,pszDelimiter+1);
			ChannelInfo.SetName(szName);
		} else {
			if (ChannelInfo.GetChannelIndex()>=(int)BonDriverChannelList.size()) {
				fCorrupted=true;
				continue;
			}
		}

		ChannelInfo.SetSpace(Space);
		pChannelList->AddChannel(ChannelInfo);
	}

	*pfCorrupted=fCorrupted;

	return true;
}


bool CChannelScan::SetPreset(bool fAuto)
{
	LPCTSTR pszName=GetAppClass().GetCoreEngine()->m_DtvEngine.m_BonSrcDecoder.GetSpaceName(m_ScanSpace);
	if (pszName==NULL)
		return false;

	bool fBS=::StrStrI(pszName,TEXT("BS"))!=NULL;
	bool fCS=::StrStrI(pszName,TEXT("CS"))!=NULL;
	if (fBS==fCS)
		return false;

	CTuningSpaceInfo *pTuningSpaceInfo=m_TuningSpaceList.GetTuningSpaceInfo(m_ScanSpace);
	if (pTuningSpaceInfo==NULL)
		return false;

	CChannelList *pChannelList=pTuningSpaceInfo->GetChannelList();
	if (pChannelList==NULL)
		return false;

	bool fResult,fCorrupted;
	CChannelList NewChannelList;
	if (fBS)
		fResult=LoadPreset(TEXT("Preset_BS.ch2"),&NewChannelList,m_ScanSpace,&fCorrupted);
	else
		fResult=LoadPreset(TEXT("Preset_CS.ch2"),&NewChannelList,m_ScanSpace,&fCorrupted);
	if (!fResult)
		return false;

	if (fCorrupted) {
		if (fAuto)
			return false;
		if (::MessageBox(m_hDlg,
						 TEXT("この BonDriver はプリセットとチャンネルが異なっている可能性があります。\n")
						 TEXT("プリセットを使用せずに、スキャンを行うことをお勧めします。\n")
						 TEXT("プリセットを読み込みますか？"),
						 TEXT("プリセット読み込み"),
						 MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION)!=IDYES)
			return false;
	}

	*pChannelList=NewChannelList;
	pTuningSpaceInfo->SetName(pszName);
	m_fUpdated=true;
	SetChannelList(m_ScanSpace);
	m_SortColumn=-1;
	SetListViewSortMark(::GetDlgItem(m_hDlg,IDC_CHANNELSCAN_CHANNELLIST),-1);

	return true;
}


INT_PTR CChannelScan::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CAppMain &AppMain=GetAppClass();
			CCoreEngine *pCoreEngine=AppMain.GetCoreEngine();
			int NumSpaces;
			LPCTSTR pszName;

			m_TuningSpaceList=*m_pOriginalTuningSpaceList;
			for (NumSpaces=0;(pszName=pCoreEngine->m_DtvEngine.m_BonSrcDecoder.GetSpaceName(NumSpaces))!=NULL;NumSpaces++) {
				DlgComboBox_AddString(hDlg,IDC_CHANNELSCAN_SPACE,pszName);
			}
			if (NumSpaces>0) {
				const CChannelInfo *pCurChannel=AppMain.GetChannelManager()->GetCurrentRealChannelInfo();
				if (pCurChannel!=NULL && pCurChannel->GetSpace()<NumSpaces)
					m_ScanSpace=pCurChannel->GetSpace();
				else
					m_ScanSpace=0;
				DlgComboBox_SetCurSel(hDlg,IDC_CHANNELSCAN_SPACE,m_ScanSpace);
				m_TuningSpaceList.Reserve(NumSpaces);
			} else {
				m_ScanSpace=-1;
				EnableDlgItems(hDlg,IDC_CHANNELSCAN_SPACE,IDC_CHANNELSCAN_START,false);
			}

			m_fUpdated=false;
			m_fScaned=false;
			m_fRestorePreview=false;
			m_SortColumn=-1;

			HWND hwndList=::GetDlgItem(hDlg,IDC_CHANNELSCAN_CHANNELLIST);
			ListView_SetExtendedListViewStyle(hwndList,
				LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_LABELTIP);

			LV_COLUMN lvc;
			lvc.mask=LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			lvc.fmt=LVCFMT_LEFT;
			lvc.cx=128;
			lvc.pszText=TEXT("名前");
			ListView_InsertColumn(hwndList,0,&lvc);
			lvc.fmt=LVCFMT_RIGHT;
			lvc.cx=40;
			lvc.pszText=TEXT("番号");
			ListView_InsertColumn(hwndList,1,&lvc);
			lvc.cx=72;
			lvc.pszText=TEXT("チャンネル");
			ListView_InsertColumn(hwndList,2,&lvc);
			lvc.cx=72;
			lvc.pszText=TEXT("サービスID");
			ListView_InsertColumn(hwndList,3,&lvc);
			lvc.cx=80;
			lvc.pszText=TEXT("リモコンキー");
			ListView_InsertColumn(hwndList,4,&lvc);
			if (NumSpaces>0) {
				//SetChannelList(hDlg,m_ScanSpace);
				::SendMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_CHANNELSCAN_SPACE,CBN_SELCHANGE),0);
				/*
				for (i=0;i<5;i++)
					ListView_SetColumnWidth(hwndList,i,LVSCW_AUTOSIZE_USEHEADER);
				*/
			}

			DlgCheckBox_Check(hDlg,IDC_CHANNELSCAN_IGNORESIGNALLEVEL,m_fIgnoreSignalLevel);

			TCHAR szText[8];
			for (int i=1;i<=10;i++) {
				wsprintf(szText,TEXT("%d 秒"),i);
				DlgComboBox_AddString(hDlg,IDC_CHANNELSCAN_SCANWAIT,szText);
			}
			DlgComboBox_SetCurSel(hDlg,IDC_CHANNELSCAN_SCANWAIT,m_ScanWait/1000-1);
			for (int i=0;i<=10;i++) {
				wsprintf(szText,TEXT("%d 秒"),i);
				DlgComboBox_AddString(hDlg,IDC_CHANNELSCAN_RETRYCOUNT,szText);
			}
			DlgComboBox_SetCurSel(hDlg,IDC_CHANNELSCAN_RETRYCOUNT,m_RetryCount);

			if (pCoreEngine->IsNetworkDriver())
				EnableDlgItems(hDlg,IDC_CHANNELSCAN_FIRST,IDC_CHANNELSCAN_LAST,false);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CHANNELSCAN_SPACE:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				const int Space=(int)DlgComboBox_GetCurSel(hDlg,IDC_CHANNELSCAN_SPACE);
				if (Space<0)
					return TRUE;

				m_ScanSpace=Space;
				LPCTSTR pszName=GetAppClass().GetCoreEngine()->m_DtvEngine.m_BonSrcDecoder.GetSpaceName(Space);
				bool fBS=false,fCS=false;
				if (pszName!=NULL) {
					fBS=::StrStrI(pszName,TEXT("BS"))!=NULL;
					fCS=::StrStrI(pszName,TEXT("CS"))!=NULL;
				}
				CTuningSpaceInfo *pTuningSpaceInfo=m_TuningSpaceList.GetTuningSpaceInfo(Space);
				CChannelList *pChannelList=NULL;
				if (pTuningSpaceInfo!=NULL)
					pChannelList=pTuningSpaceInfo->GetChannelList();
				if (fBS!=fCS && pChannelList!=NULL && pChannelList->NumChannels()==0) {
					SetPreset(true);
				} else {
					SetChannelList(Space);
				}
				DlgCheckBox_Check(hDlg,IDC_CHANNELSCAN_SCANSERVICE,
								  fBS || fCS || (pChannelList!=NULL && pChannelList->HasMultiService()));
				EnableDlgItem(hDlg,IDC_CHANNELSCAN_LOADPRESET,fBS || fCS);
				m_SortColumn=-1;
				SetListViewSortMark(::GetDlgItem(hDlg,IDC_CHANNELSCAN_CHANNELLIST),-1);
			}
			return TRUE;

		case IDC_CHANNELSCAN_LOADPRESET:
			SetPreset(false);
			return TRUE;

		case IDC_CHANNELSCAN_START:
			{
				int Space=(int)DlgComboBox_GetCurSel(hDlg,IDC_CHANNELSCAN_SPACE);

				if (Space>=0) {
					HWND hwndList=::GetDlgItem(hDlg,IDC_CHANNELSCAN_CHANNELLIST);

					if (GetAppClass().GetUICore()->IsViewerEnabled()) {
						GetAppClass().GetUICore()->EnableViewer(false);
						m_fRestorePreview=true;
					}
					m_ScanSpace=Space;
					m_ScanChannel=0;
					m_fScanService=
						DlgCheckBox_IsChecked(hDlg,IDC_CHANNELSCAN_SCANSERVICE);
					m_fIgnoreSignalLevel=
						DlgCheckBox_IsChecked(hDlg,IDC_CHANNELSCAN_IGNORESIGNALLEVEL);
					m_ScanWait=((unsigned int)DlgComboBox_GetCurSel(hDlg,IDC_CHANNELSCAN_SCANWAIT)+1)*1000;
					m_RetryCount=(int)DlgComboBox_GetCurSel(hDlg,IDC_CHANNELSCAN_RETRYCOUNT);

					ListView_DeleteAllItems(hwndList);

					if (::DialogBoxParam(GetAppClass().GetResourceInstance(),
							MAKEINTRESOURCE(IDD_CHANNELSCAN),::GetParent(hDlg),
							ScanDialogProc,reinterpret_cast<LPARAM>(this))==IDOK) {
						if (ListView_GetItemCount(hwndList)>0) {
							// 元あったチャンネルで検出されなかったものがある場合、残すか問い合わせる
							CChannelList *pChannelList=m_TuningSpaceList.GetChannelList(Space);
							std::vector<const CChannelInfo*> MissingChannels;
							for (int i=0;i<pChannelList->NumChannels();i++) {
								const CChannelInfo *pOldChannel=pChannelList->GetChannelInfo(i);
								if (m_ScanningChannelList.FindByIDs(pOldChannel->GetNetworkID(),0,pOldChannel->GetServiceID())<0
										&& m_ScanningChannelList.FindByName(pOldChannel->GetName())<0)
									MissingChannels.push_back(pOldChannel);
							}
							if (!MissingChannels.empty()) {
								const int Channels=(int)MissingChannels.size();
								TCHAR szMessage[256+(MAX_CHANNEL_NAME+2)*10];
								CStaticStringFormatter Formatter(szMessage,lengthof(szMessage));

								Formatter.AppendFormat(
									TEXT("元あったチャンネルのうち、以下の%d%sのチャンネルが検出されませんでした。\n\n"),
									Channels,Channels<10?TEXT("つ"):TEXT(""));
								for (int i=0;i<Channels;i++) {
									if (i==10) {
										Formatter.Append(TEXT("...\n"));
										break;
									}
									Formatter.AppendFormat(TEXT("・%s\n"),MissingChannels[i]->GetName());
								}
								Formatter.Append(TEXT("\n検出されなかったチャンネルを残しますか？"));
								if (::MessageBox(hDlg,Formatter.GetString(),TEXT("問い合わせ"),
												 MB_YESNO | MB_ICONQUESTION)==IDYES) {
									for (size_t i=0;i<MissingChannels.size();i++) {
										CChannelInfo *pInfo=new CChannelInfo(*MissingChannels[i]);
										m_ScanningChannelList.AddChannel(pInfo);
										InsertChannelInfo(ListView_GetItemCount(hwndList),pInfo);
									}
								}
							}

							CChannelListSort::SortType SortType;
							if (m_ScanningChannelList.HasRemoteControlKeyID()
									&& GetNetworkType(m_ScanningChannelList.GetChannelInfo(0)->GetNetworkID())==NETWORK_TERRESTRIAL)
								SortType=CChannelListSort::SORT_REMOTECONTROLKEYID;
							else
								SortType=CChannelListSort::SORT_SERVICEID;
							CChannelListSort ListSort(SortType);
							ListSort.Sort(hwndList);
							ListSort.UpdateChannelList(hwndList,m_TuningSpaceList.GetChannelList(Space));
							if (IsStringEmpty(m_TuningSpaceList.GetTuningSpaceName(Space))) {
								m_TuningSpaceList.GetTuningSpaceInfo(Space)->SetName(
									GetAppClass().GetCoreEngine()->m_DtvEngine.m_BonSrcDecoder.GetSpaceName(Space));
							}
							m_SortColumn=(int)SortType;
							m_fSortDescending=false;
							SetListViewSortMark(hwndList,m_SortColumn,!m_fSortDescending);
							ListView_EnsureVisible(hwndList,0,FALSE);
						} else {
							// チャンネルが検出できなかった
							TCHAR szText[1024];

							::lstrcpy(szText,TEXT("チャンネルが検出できませんでした。"));
							if ((m_fIgnoreSignalLevel
										&& m_MaxSignalLevel<1.0f)
									|| (!m_fIgnoreSignalLevel
										&& m_MaxSignalLevel<SIGNAL_LEVEL_THRESHOLD)) {
								::lstrcat(szText,TEXT("\n信号レベルが低すぎるか、取得できません。"));
								if (!m_fIgnoreSignalLevel
										&& m_MaxBitRate>8000000)
									::lstrcat(szText,TEXT("\n[信号レベルを無視する] をチェックしてスキャンしてみてください。"));
							} else if (m_MaxBitRate<8000000) {
								::lstrcat(szText,TEXT("\nストリームを受信できません。"));
							}
							::MessageBox(hDlg,szText,TEXT("スキャン結果"),MB_OK | MB_ICONEXCLAMATION);
						}
						m_fUpdated=true;
					} else {
						SetChannelList(Space);
					}
				}
			}
			return TRUE;

		case IDC_CHANNELSCAN_PROPERTIES:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_CHANNELSCAN_CHANNELLIST);
				LV_ITEM lvi;

				lvi.iItem=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
				if (lvi.iItem>=0) {
					lvi.mask=LVIF_PARAM;
					lvi.iSubItem=0;
					ListView_GetItem(hwndList,&lvi);
					CChannelInfo *pChInfo=reinterpret_cast<CChannelInfo*>(lvi.lParam);
					CChannelPropDialog Dialog(pChInfo);
					if (Dialog.Show(hDlg)) {
						lvi.mask=LVIF_TEXT;
						lvi.pszText=const_cast<LPTSTR>(pChInfo->GetName());
						ListView_SetItem(hwndList,&lvi);
						lvi.iSubItem=4;
						TCHAR szText[16];
						if (pChInfo->GetChannelNo()>0)
							::wsprintf(szText,TEXT("%d"),pChInfo->GetChannelNo());
						else
							szText[0]='\0';
						lvi.pszText=szText;
						ListView_SetItem(hwndList,&lvi);
						m_fUpdated=true;
					}
				}
			}
			return TRUE;

		case IDC_CHANNELSCAN_DELETE:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_CHANNELSCAN_CHANNELLIST);
				LV_ITEM lvi;

				lvi.iItem=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
				if (lvi.iItem>=0) {
					lvi.mask=LVIF_PARAM;
					lvi.iSubItem=0;
					ListView_GetItem(hwndList,&lvi);
					CChannelInfo *pChInfo=reinterpret_cast<CChannelInfo*>(lvi.lParam);
					CChannelList *pList=m_TuningSpaceList.GetChannelList(m_ScanSpace);
					if (pList!=NULL) {
						int Index=pList->Find(pChInfo);
						if (Index>=0) {
							ListView_DeleteItem(hwndList,lvi.iItem);
							pList->DeleteChannel(Index);
							m_fUpdated=true;
						}
					}
				}
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case LVN_COLUMNCLICK:
			{
				LPNMLISTVIEW pnmlv=reinterpret_cast<LPNMLISTVIEW>(lParam);

				if (m_SortColumn==pnmlv->iSubItem) {
					m_fSortDescending=!m_fSortDescending;
				} else {
					m_SortColumn=pnmlv->iSubItem;
					m_fSortDescending=false;
				}
				CChannelListSort ListSort(
					(CChannelListSort::SortType)pnmlv->iSubItem,
					m_fSortDescending);
				ListSort.Sort(pnmlv->hdr.hwndFrom);
				ListSort.UpdateChannelList(pnmlv->hdr.hwndFrom,
					m_TuningSpaceList.GetChannelList(m_ScanSpace));
				SetListViewSortMark(pnmlv->hdr.hwndFrom,pnmlv->iSubItem,!m_fSortDescending);
			}
			return TRUE;

		case NM_RCLICK:
			if (((LPNMHDR)lParam)->hwndFrom==::GetDlgItem(hDlg,IDC_CHANNELSCAN_CHANNELLIST)) {
				LPNMITEMACTIVATE pnmia=reinterpret_cast<LPNMITEMACTIVATE>(lParam);

				if (pnmia->iItem>=0) {
					HMENU hmenu=::LoadMenu(GetAppClass().GetResourceInstance(),
										   MAKEINTRESOURCE(IDM_CHANNELSCAN));
					POINT pt;

					::GetCursorPos(&pt);
					::TrackPopupMenu(GetSubMenu(hmenu,0),TPM_RIGHTBUTTON,
									 pt.x,pt.y,0,hDlg,NULL);
					::DestroyMenu(hmenu);
				}
			}
			break;

		case NM_DBLCLK:
			if (((LPNMHDR)lParam)->hwndFrom==::GetDlgItem(hDlg,IDC_CHANNELSCAN_CHANNELLIST)) {
				LPNMITEMACTIVATE pnmia=reinterpret_cast<LPNMITEMACTIVATE>(lParam);

				if (pnmia->iItem>=0)
					::SendMessage(hDlg,WM_COMMAND,IDC_CHANNELSCAN_PROPERTIES,0);
			}
			break;

		case LVN_ENDLABELEDIT:
			{
				NMLVDISPINFO *plvdi=reinterpret_cast<NMLVDISPINFO*>(lParam);
				BOOL fResult;

				if (plvdi->item.pszText!=NULL && plvdi->item.pszText[0]!='\0') {
					LV_ITEM lvi;

					lvi.mask=LVIF_PARAM;
					lvi.iItem=plvdi->item.iItem;
					lvi.iSubItem=0;
					ListView_GetItem(plvdi->hdr.hwndFrom,&lvi);
					CChannelInfo *pChInfo=reinterpret_cast<CChannelInfo*>(lvi.lParam);
					pChInfo->SetName(plvdi->item.pszText);
					m_fUpdated=true;
					fResult=TRUE;
				} else {
					fResult=FALSE;
				}
				::SetWindowLongPtr(hDlg,DWLP_MSGRESULT,fResult);
			}
			return TRUE;

		case LVN_ITEMCHANGED:
			if (!m_fChanging) {
				LPNMLISTVIEW pnmlv=reinterpret_cast<LPNMLISTVIEW>(lParam);

				if (pnmlv->iItem>=0
						&& (pnmlv->uNewState&LVIS_STATEIMAGEMASK)!=
						   (pnmlv->uOldState&LVIS_STATEIMAGEMASK)) {
					LV_ITEM lvi;

					lvi.mask=LVIF_PARAM;
					lvi.iItem=pnmlv->iItem;
					lvi.iSubItem=0;
					ListView_GetItem(pnmlv->hdr.hwndFrom,&lvi);
					CChannelInfo *pChInfo=reinterpret_cast<CChannelInfo*>(lvi.lParam);
					pChInfo->Enable((pnmlv->uNewState&LVIS_STATEIMAGEMASK)==INDEXTOSTATEIMAGEMASK(2));
					m_fUpdated=true;
				}
			}
			return TRUE;

		case PSN_APPLY:
			{
				CAppMain &AppMain=GetAppClass();

				if (m_fUpdated) {
					m_TuningSpaceList.MakeAllChannelList();
					TCHAR szFileName[MAX_PATH];
					if (!GetAppClass().GetChannelManager()->GetChannelFileName(szFileName,lengthof(szFileName))
							|| ::lstrcmpi(::PathFindExtension(szFileName),CHANNEL_FILE_EXTENSION)!=0
							|| !::PathFileExists(szFileName)) {
						GetAppClass().GetCoreEngine()->GetDriverPath(szFileName,lengthof(szFileName));
						::PathRenameExtension(szFileName,CHANNEL_FILE_EXTENSION);
					}
					bool fOK=m_TuningSpaceList.SaveToFile(szFileName);
					if (fOK) {
						AppMain.AddLog(TEXT("チャンネルファイルを \"%s\" に保存しました。"),szFileName);
					} else {
						TCHAR szText[32+MAX_PATH];
						StdUtil::snprintf(szText,lengthof(szText),
										  TEXT("チャンネルファイル \"%s\" を保存できません。"),szFileName);
						AppMain.AddLog(TEXT("%s"),szText);
						::MessageBox(hDlg,szText,NULL,MB_OK | MB_ICONEXCLAMATION);
					}
					SetUpdateFlag(UPDATE_CHANNELLIST);

					if (fOK) {
						LPTSTR pSequence=::PathFindExtension(szFileName);
						if (pSequence>szFileName && *(--pSequence)==_T('0')) {
							::lstrcpy(pSequence+1,TEXT(".dll"));
							unsigned int Exists=0;
							for (int i=1;i<=9;i++) {
								*pSequence=_T('0')+i;
								if (::PathFileExists(szFileName))
									Exists|=1U<<i;
							}
							if (Exists!=0) {
								LPCTSTR pszName=::PathFindFileName(szFileName);
								TCHAR szText[256+MAX_PATH*10];
								CStaticStringFormatter Formatter(szText,lengthof(szText));

								*pSequence=_T('0');
								Formatter.AppendFormat(TEXT("%s のチャンネルスキャン結果を\n以下の BonDriver にも反映させますか？\n\n"),pszName);
								for (int i=1;i<=9;i++) {
									if ((Exists&(1U<<i))!=0) {
										*pSequence=_T('0')+i;
										Formatter.AppendFormat(TEXT("・%s\n"),pszName);
									}
								}
								if (::MessageBox(hDlg,Formatter.GetString(),TEXT("チャンネルスキャン"),MB_YESNO | MB_ICONQUESTION)==IDYES) {
									for (int i=1;i<=9;i++) {
										if ((Exists&(1U<<i))!=0) {
											::PathRenameExtension(szFileName,CHANNEL_FILE_EXTENSION);
											*pSequence=_T('0')+i;
											if (m_TuningSpaceList.SaveToFile(szFileName)) {
												AppMain.AddLog(TEXT("チャンネルファイルを \"%s\" に保存しました。"),szFileName);
												::PathRenameExtension(szFileName,TEXT(".dll"));
												AppMain.UpdateChannelList(szFileName,&m_TuningSpaceList);
											} else {
												StdUtil::snprintf(szText,lengthof(szText),
																  TEXT("チャンネルファイル \"%s\" を保存できません。"),szFileName);
												AppMain.AddLog(TEXT("%s"),szText);
												if (::MessageBox(hDlg,szText,NULL,MB_OKCANCEL | MB_ICONEXCLAMATION)!=IDOK)
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

				m_fIgnoreSignalLevel=
					DlgCheckBox_IsChecked(hDlg,IDC_CHANNELSCAN_IGNORESIGNALLEVEL);
				m_ScanWait=((unsigned int)DlgComboBox_GetCurSel(hDlg,IDC_CHANNELSCAN_SCANWAIT)+1)*1000;
				m_RetryCount=(int)DlgComboBox_GetCurSel(hDlg,IDC_CHANNELSCAN_RETRYCOUNT);

				m_fChanged=true;
			}
			return TRUE;

		case PSN_RESET:
			if (m_fRestorePreview)
				//SetUpdateFlag(UPDATE_PREVIEW);
				GetAppClass().GetUICore()->EnableViewer(true);
			return TRUE;
		}
		break;

	case WM_APP_CHANNELFOUND:
		{
			CChannelInfo *pChInfo=reinterpret_cast<CChannelInfo*>(lParam);

			// 元の有効/無効状態を反映
			const CChannelList *pOldChList=m_TuningSpaceList.GetChannelList(m_ScanSpace);
			int OldChIndex=pOldChList->FindByIDs(pChInfo->GetNetworkID(),0,pChInfo->GetServiceID());
			if (OldChIndex>=0)
				pChInfo->Enable(pOldChList->IsEnabled(OldChIndex));

			HWND hwndList=::GetDlgItem(hDlg,IDC_CHANNELSCAN_CHANNELLIST);
			int Index=ListView_GetItemCount(hwndList);

			m_fChanging=true;
			InsertChannelInfo(Index,pChInfo);
			ListView_EnsureVisible(hwndList,Index,FALSE);
			m_fChanging=false;
			::UpdateWindow(hwndList);
		}
		return TRUE;
	}

	return FALSE;
}


INT_PTR CALLBACK CChannelScan::ScanDialogProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	CChannelScan *pThis;

	if (uMsg==WM_INITDIALOG) {
		pThis=reinterpret_cast<CChannelScan*>(lParam);
		pThis->m_hScanDlg=hDlg;
		::SetProp(hDlg,TEXT("This"),pThis);
	} else {
		pThis=static_cast<CChannelScan*>(::GetProp(hDlg,TEXT("This")));
		if (pThis==NULL)
			return FALSE;
		if (uMsg==WM_NCDESTROY) {
			pThis->m_hScanDlg=NULL;
			::RemoveProp(hDlg,TEXT("This"));
			return TRUE;
		}
	}

	return pThis->ScanDlgProc(hDlg,uMsg,wParam,lParam);
}


INT_PTR CChannelScan::ScanDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static const UINT TIMER_ID_STATISTICS=1;

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CBonSrcDecoder &BonSrcDecoder=GetAppClass().GetCoreEngine()->m_DtvEngine.m_BonSrcDecoder;
			m_BonDriverChannelList.clear();
			if (m_ScanChannel>0)
				m_BonDriverChannelList.resize(m_ScanChannel);
			LPCTSTR pszName;
			for (int i=m_ScanChannel;(pszName=BonSrcDecoder.GetChannelName(m_ScanSpace,i))!=NULL;i++) {
				m_BonDriverChannelList.push_back(TVTest::String(pszName));
			}

			m_ChannelSignalLevel.clear();
			m_ChannelSignalLevel.reserve(m_BonDriverChannelList.size());

			::SendDlgItemMessage(hDlg,IDC_CHANNELSCAN_PROGRESS,PBM_SETRANGE32,
								 m_ScanChannel,m_BonDriverChannelList.size());
			::SendDlgItemMessage(hDlg,IDC_CHANNELSCAN_PROGRESS,PBM_SETPOS,
								 m_ScanChannel,0);

			GetAppClass().BeginChannelScan(m_ScanSpace);

			m_fCancelled=false;
			m_hCancelEvent=::CreateEvent(NULL,FALSE,FALSE,NULL);
			m_hScanThread=reinterpret_cast<HANDLE>(::_beginthreadex(NULL,0,ScanProc,this,0,NULL));
			m_fScaned=true;

			::SetTimer(hDlg,TIMER_ID_STATISTICS,1000,NULL);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL:
			{
				m_fCancelled=LOWORD(wParam)==IDCANCEL;
				::SetEvent(m_hCancelEvent);
				::EnableDlgItem(hDlg,IDOK,false);
				::EnableDlgItem(hDlg,IDCANCEL,false);
			}
			return TRUE;
		}
		return TRUE;

	case WM_TIMER:
		if (wParam==TIMER_ID_STATISTICS) {
			const CCoreEngine *pCoreEngine=GetAppClass().GetCoreEngine();
			TCHAR szText[64],szSignalLevel[32],szBitRate[32];

			pCoreEngine->GetSignalLevelText(szSignalLevel,lengthof(szSignalLevel));
			pCoreEngine->GetBitRateText(szBitRate,lengthof(szBitRate));
			StdUtil::snprintf(szText,lengthof(szText),
							  TEXT("%s / %s"),szSignalLevel,szBitRate);
			::SetDlgItemText(hDlg,IDC_CHANNELSCAN_LEVEL,szText);
		}
		return TRUE;

	case WM_DRAWITEM:
		if (wParam==IDC_CHANNELSCAN_GRAPH) {
			const DRAWITEMSTRUCT *pdis=reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);

			::FillRect(pdis->hDC,&pdis->rcItem,static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
			if (!m_BonDriverChannelList.empty()) {
				const int ChannelCount=(int)m_BonDriverChannelList.size();
				const int CellWidth=(pdis->rcItem.right-pdis->rcItem.left-5)/ChannelCount;
				const float SignalScale=max(m_MaxSignalLevel,30.0f);
				RECT rcGraph;
				rcGraph.left=((pdis->rcItem.right-pdis->rcItem.left)-ChannelCount*CellWidth)/2;
				rcGraph.right=rcGraph.left+ChannelCount*CellWidth;
				rcGraph.top=pdis->rcItem.top+2;
				rcGraph.bottom=pdis->rcItem.bottom-2;

				HPEN hpenGrid=::CreatePen(PS_SOLID,1,RGB(96,96,96));
				HGDIOBJ hOldPen=::SelectObject(pdis->hDC,hpenGrid);
				HGDIOBJ hOldBrush=::SelectObject(pdis->hDC,::GetStockObject(NULL_BRUSH));

				for (int i=0;i<ChannelCount;i++) {
					int x=rcGraph.left+i*CellWidth;
					::MoveToEx(pdis->hDC,x,rcGraph.top,NULL);
					::LineTo(pdis->hDC,x,rcGraph.bottom);

					if (i<(int)m_ChannelSignalLevel.size()) {
						float SignalLevel=m_ChannelSignalLevel[i];
						if (SignalLevel>=0.0f) {
							RECT rcFill;
							rcFill.left=x+1;
							rcFill.right=x+CellWidth;
							rcFill.top=rcGraph.bottom-3-(int)(SignalLevel*(float)(rcGraph.bottom-rcGraph.top-4)/SignalScale);
							rcFill.bottom=rcGraph.bottom-1;
							HBRUSH hbr=::CreateSolidBrush(HSVToRGB(0.6-SignalLevel/SignalScale*0.6,0.8,1.0));
							::FillRect(pdis->hDC,&rcFill,hbr);
							::DeleteObject(hbr);
						}
					}
				}

				::Rectangle(pdis->hDC,rcGraph.left,rcGraph.top,rcGraph.right+1,rcGraph.bottom);

				::SelectObject(pdis->hDC,hOldBrush);
				::SelectObject(pdis->hDC,hOldPen);
				::DeleteObject(hpenGrid);
			}
		}
		return TRUE;

	case WM_APP_BEGINSCAN:
		{
			const int CurChannel=(int)wParam;
			const int NumChannels=(int)m_BonDriverChannelList.size();
			unsigned int EstimateRemain=(NumChannels-CurChannel)*m_ScanWait;
			TCHAR szText[80];

			if (m_fIgnoreSignalLevel)
				EstimateRemain+=(NumChannels-CurChannel)*m_RetryCount*m_RetryInterval;
			EstimateRemain=(EstimateRemain+500)/1000;
			StdUtil::snprintf(szText,lengthof(szText),
				TEXT("チャンネル %d/%d をスキャンしています... (残り時間 %u:%02u)"),
				CurChannel+1,NumChannels,
				EstimateRemain/60,EstimateRemain%60);
			::SetDlgItemText(hDlg,IDC_CHANNELSCAN_INFO,szText);
			::SetDlgItemText(hDlg,IDC_CHANNELSCAN_CHANNEL,m_BonDriverChannelList[CurChannel].c_str());
			::SendDlgItemMessage(hDlg,IDC_CHANNELSCAN_PROGRESS,PBM_SETPOS,wParam,0);
			GetAppClass().SetProgress(CurChannel,NumChannels);
		}
		return TRUE;

	case WM_APP_CHANNELFOUND:
		{
			//int ScanChannel=LOWORD(wParam),Service=HIWORD(wParam);
			//CChannelInfo *pChannelInfo=reinterpret_cast<CChannelInfo*>(lParam);

			::SendMessage(m_hDlg,WM_APP_CHANNELFOUND,0,lParam);
		}
		return TRUE;

	case WM_APP_ENDCHANNEL:
		{
			while ((int)m_ChannelSignalLevel.size()<(int)wParam)
				m_ChannelSignalLevel.push_back(-1.0f);
			m_ChannelSignalLevel.push_back((float)lParam/100.0f);
			InvalidateDlgItem(hDlg,IDC_CHANNELSCAN_GRAPH,false);
		}
		return TRUE;

	case WM_APP_ENDSCAN:
		{
			ScanResult Result=(ScanResult)wParam;

			::WaitForSingleObject(m_hScanThread,INFINITE);

			//GetAppClass().EndChannelScan();
			GetAppClass().EndProgress();

			if (m_fCancelled) {
				::EndDialog(hDlg,IDCANCEL);
			} else {
				CMessageDialog Dialog;

				if (Result==SCAN_RESULT_SET_CHANNEL_PARTIALLY_FAILED) {
					const int ChannelCount=LOWORD(lParam);
					const int ErrorCount=HIWORD(lParam);
					TVTest::String Message;
					if (ErrorCount<ChannelCount) {
						TVTest::StringUtility::Format(Message,
							TEXT("%dチャンネルのうち、%d回のチャンネル変更が BonDriver に受け付けられませんでした。\n")
							TEXT("(受信できるチャンネルが全てスキャンできていれば問題はありません)"),
							ChannelCount,ErrorCount);
						Dialog.Show(hDlg,CMessageDialog::TYPE_INFO,Message.c_str(),NULL,NULL,TEXT("お知らせ"));
					} else {
						Dialog.Show(hDlg,CMessageDialog::TYPE_WARNING,
									TEXT("チャンネル変更の要求が BonDriver に受け付けられないため、スキャンを行えませんでした。"));
					}
				} else if (Result==SCAN_RESULT_SET_CHANNEL_TIMEOUT) {
					Dialog.Show(hDlg,CMessageDialog::TYPE_WARNING,
								TEXT("タイムアウトのためチャンネル変更ができません。"));
				}

				::EndDialog(hDlg,IDOK);
			}
		}
		return TRUE;

	case WM_DESTROY:
		{
			if (m_hScanThread!=NULL) {
				::SetEvent(m_hCancelEvent);
				if (::WaitForSingleObject(m_hScanThread,30000)==WAIT_TIMEOUT) {
					GetAppClass().AddLog(TEXT("チャンネルスキャンスレッドを強制終了します。"));
					::TerminateThread(m_hScanThread,-1);
				}
				::CloseHandle(m_hScanThread);
				m_hScanThread=NULL;
			}
			if (m_hCancelEvent!=NULL) {
				::CloseHandle(m_hCancelEvent);
				m_hCancelEvent=NULL;
			}
		}
		return TRUE;
	}

	return FALSE;
}


unsigned int __stdcall CChannelScan::ScanProc(LPVOID lpParameter)
{
	CChannelScan *pThis=static_cast<CChannelScan*>(lpParameter);

	pThis->Scan();

	return 0;
}


void CChannelScan::Scan()
{
	CDtvEngine *pDtvEngine=&GetAppClass().GetCoreEngine()->m_DtvEngine;
	CTsAnalyzer *pTsAnalyzer=&pDtvEngine->m_TsAnalyzer;

	ScanResult Result=SCAN_RESULT_CANCELLED;
	int SetChannelCount=0,SetChannelErrorCount=0;

	m_ScanningChannelList.Clear();
	m_MaxSignalLevel=0.0f;
	m_ChannelMaxSignalLevel=0.0f;
	m_MaxBitRate=0;

	bool fPurgeStream=pDtvEngine->m_BonSrcDecoder.IsPurgeStreamOnChannelChange();
	if (!fPurgeStream)
		pDtvEngine->m_BonSrcDecoder.SetPurgeStreamOnChannelChange(true);

	for (;;m_ScanChannel++) {
		if (m_ScanChannel>=(int)m_BonDriverChannelList.size()) {
			if (Result==SCAN_RESULT_CANCELLED)
				Result=SCAN_RESULT_SUCCEEDED;
			break;
		}

		::PostMessage(m_hScanDlg,WM_APP_BEGINSCAN,m_ScanChannel,0);

		SetChannelCount++;
		if (!pDtvEngine->SetChannel(m_ScanSpace,m_ScanChannel)) {
			SetChannelErrorCount++;
			if (pDtvEngine->GetLastErrorCode()==CBonSrcDecoder::ERR_TIMEOUT) {
				Result=SCAN_RESULT_SET_CHANNEL_TIMEOUT;
				break;
			} else {
				Result=SCAN_RESULT_SET_CHANNEL_PARTIALLY_FAILED;
			}
			if (::WaitForSingleObject(m_hCancelEvent,0)!=WAIT_TIMEOUT)
				break;
			continue;
		}

		if (::WaitForSingleObject(m_hCancelEvent,
								  min(m_ScanWait,2000))!=WAIT_TIMEOUT)
			break;
		if (m_ScanWait>2000) {
			DWORD Wait=m_ScanWait-2000;
			while (true) {
				// SDTが来たら待ち時間終了
				if (pTsAnalyzer->IsSdtUpdated())
					break;
				if (::WaitForSingleObject(m_hCancelEvent,min(Wait,1000))!=WAIT_TIMEOUT)
					goto End;
				if (Wait<=1000)
					break;
				Wait-=1000;
			}
		}

		bool fScanService=m_fScanService;
		bool fFound=false;
		CTsAnalyzer::SdtServiceList ServiceList;
		TCHAR szName[256];

		m_ChannelMaxSignalLevel=0.0f;
		float SignalLevel=GetSignalLevel();

		if (pTsAnalyzer->IsSdtUpdated()
				|| m_fIgnoreSignalLevel
				|| SignalLevel>=SIGNAL_LEVEL_THRESHOLD) {
			for (int i=0;i<=m_RetryCount;i++) {
				if (i>0) {
					if (::WaitForSingleObject(m_hCancelEvent,
											  m_RetryInterval)!=WAIT_TIMEOUT)
						goto End;
					GetSignalLevel();
				}
				if (pTsAnalyzer->IsSdtUpdated()
						&& pTsAnalyzer->GetSdtServiceList(&ServiceList)
						&& !ServiceList.empty()) {
					if (fScanService) {
						if (pTsAnalyzer->GetServiceNum()>0) {
							// サービスのPMTが揃ったら完了
							// (サービスが視聴可能かどうかPMTの情報で判定するため)
							size_t i;
							for (i=0;i<ServiceList.size();i++) {
								if (IsScanService(ServiceList[i])) {
									int ServiceIndex=pTsAnalyzer->GetServiceIndexByID(ServiceList[i].ServiceID);
									if (ServiceIndex>=0 && !pTsAnalyzer->IsServiceUpdated(ServiceIndex))
										break;
								}
							}
							if (i==ServiceList.size()) {
								fFound=true;
								break;
							}
						}
						// 時間内にPMTが来ない場合、PMTの情報なしで継続する
						if (!fFound && i==m_RetryCount) {
							fFound=true;
							break;
						}
					} else {
						if (pTsAnalyzer->IsNitUpdated()) {
							if (pTsAnalyzer->GetTsName(szName,lengthof(szName))>0) {
								fFound=true;
								break;
							} else {
								// BS/CS の場合はサービスの検索を有効にする
								WORD NetworkID=pTsAnalyzer->GetNetworkID();
								if (IsBSNetworkID(NetworkID) || IsCSNetworkID(NetworkID))
									fScanService=true;
							}
						}
					}
				}
			}
		}
		if (fFound) {
			const WORD TransportStreamID=pTsAnalyzer->GetTransportStreamID();
			const WORD NetworkID=pTsAnalyzer->GetNetworkID();

			// 見付かったチャンネルを追加する
			if (fScanService) {
				int ServiceCount=0;
				for (size_t i=0;i<ServiceList.size();i++) {
					const CTsAnalyzer::SdtServiceInfo &ServiceInfo=ServiceList[i];

					if (!IsScanService(ServiceInfo))
						continue;

					const WORD ServiceID=ServiceInfo.ServiceID;

					// 視聴不可のサービスを除外
					int ServiceIndex=pTsAnalyzer->GetServiceIndexByID(ServiceID);
					if (ServiceIndex>=0
							&& pTsAnalyzer->IsServiceUpdated(ServiceIndex)) {
						if (!pTsAnalyzer->IsViewableService(ServiceIndex))
							continue;
					} else {
						if (ServiceInfo.ServiceType==SERVICE_TYPE_PROMOTIONVIDEO)
							continue;
#ifndef TVH246_FOR_1SEG
						if (ServiceInfo.ServiceType==SERVICE_TYPE_DATA)
							continue;
#endif
					}

					int RemoteControlKeyID=pTsAnalyzer->GetRemoteControlKeyID();
					if (RemoteControlKeyID==0) {
						// BSのリモコン番号を割り当てる
						if (IsBSNetworkID(NetworkID) && ServiceID<230) {
							if (ServiceID<=109)
								RemoteControlKeyID=ServiceID%10;
							else
								RemoteControlKeyID=ServiceID/10-10;
						} else if (ServiceID<1000) {
							RemoteControlKeyID=ServiceID;
						}
					}

					CChannelInfo *pChInfo=
						new CChannelInfo(m_ScanSpace,m_ScanChannel,
										 RemoteControlKeyID,
										 ServiceInfo.szServiceName);
					pChInfo->SetNetworkID(NetworkID);
					pChInfo->SetTransportStreamID(TransportStreamID);
					pChInfo->SetServiceID(ServiceID);
					pChInfo->SetServiceType(ServiceInfo.ServiceType);

					// 一部サービスはデフォルトで無効にする
					if ((ServiceInfo.ServiceType!=SERVICE_TYPE_DIGITALTV
							&& ServiceInfo.ServiceType!=SERVICE_TYPE_PROMOTIONVIDEO
#ifdef TVH264_FOR_1SEG
							&& ServiceInfo.ServiceType!=SERVICE_TYPE_DATA
#endif
#ifdef TVTEST_RADIO_SUPPORT
							&& ServiceInfo.ServiceType!=SERVICE_TYPE_DIGITALAUDIO
#endif
							)
							// BSのサブチャンネル
							|| (IsBSNetworkID(NetworkID) && ServiceID<190 && ServiceCount>0)
#ifndef TVH264
							// 地デジのサブチャンネル
							|| (NetworkID==TransportStreamID && ServiceCount>0)
#endif
						)
						pChInfo->Enable(false);

					TRACE(TEXT("Channel found [%2d][%2d] : %s NID 0x%04x TSID 0x%04x SID 0x%04x\n"),
						  m_ScanChannel,(int)i,ServiceInfo.szServiceName,
						  NetworkID,TransportStreamID,ServiceID);

					m_ScanningChannelList.AddChannel(pChInfo);
					ServiceCount++;

					::PostMessage(m_hScanDlg,WM_APP_CHANNELFOUND,
								  m_ScanChannel,reinterpret_cast<LPARAM>(pChInfo));
				}
			} else {
				CChannelInfo *pChInfo=
					new CChannelInfo(m_ScanSpace,m_ScanChannel,
									 pTsAnalyzer->GetRemoteControlKeyID(),szName);
				pChInfo->SetNetworkID(NetworkID);
				pChInfo->SetTransportStreamID(TransportStreamID);

				for (auto it=ServiceList.begin();it!=ServiceList.end();++it) {
					if (IsScanService(*it)
							&& (
#ifdef TVH264_FOR_1SEG
							it->ServiceType==SERVICE_TYPE_DATA
#else
							it->ServiceType==SERVICE_TYPE_DIGITALTV
#endif
#ifdef TVTEST_RADIO_SUPPORT
							|| it->ServiceType==SERVICE_TYPE_DIGITALAUDIO
#endif
							)) {
						pChInfo->SetServiceID(it->ServiceID);
						pChInfo->SetServiceType(it->ServiceType);
						break;
					}
				}

				TRACE(TEXT("Channel found [%2d] : %s NID 0x%04x TSID 0x%04x SID 0x%04x\n"),
					  m_ScanChannel,szName,TransportStreamID,NetworkID,pChInfo->GetServiceID());

				m_ScanningChannelList.AddChannel(pChInfo);

				::PostMessage(m_hScanDlg,WM_APP_CHANNELFOUND,
							  m_ScanChannel,reinterpret_cast<LPARAM>(pChInfo));
			}
		}
#ifdef _DEBUG
		else {
			TRACE(TEXT("Channel scan [%2d] Service not found.\n"),
				  m_ScanChannel);
		}
#endif

		::PostMessage(m_hScanDlg,WM_APP_ENDCHANNEL,m_ScanChannel,(LPARAM)(m_ChannelMaxSignalLevel*100.0f));
	}

End:
	if (!fPurgeStream)
		pDtvEngine->m_BonSrcDecoder.SetPurgeStreamOnChannelChange(false);

	::PostMessage(m_hScanDlg,WM_APP_ENDSCAN,
				  (WPARAM)Result,MAKELPARAM(SetChannelCount,SetChannelErrorCount));
}


float CChannelScan::GetSignalLevel()
{
	CBonSrcDecoder *pBonSrcDecoder=&GetAppClass().GetCoreEngine()->m_DtvEngine.m_BonSrcDecoder;

	float SignalLevel=pBonSrcDecoder->GetSignalLevel();
	if (SignalLevel>m_MaxSignalLevel)
		m_MaxSignalLevel=SignalLevel;
	if (SignalLevel>m_ChannelMaxSignalLevel)
		m_ChannelMaxSignalLevel=SignalLevel;

	DWORD BitRate=pBonSrcDecoder->GetBitRate();
	if (BitRate>m_MaxBitRate)
		m_MaxBitRate=BitRate;

	return SignalLevel;
}


bool CChannelScan::IsScanService(const CTsAnalyzer::SdtServiceInfo &ServiceInfo,bool fData)
{
	return ServiceInfo.szServiceName[0]!='\0'
		&& IsScanServiceType(ServiceInfo.ServiceType,fData);
}


bool CChannelScan::IsScanServiceType(BYTE ServiceType,bool fData)
{
	return ServiceType==SERVICE_TYPE_DIGITALTV
#ifdef TVTEST_RADIO_SUPPORT
		|| ServiceType==SERVICE_TYPE_DIGITALAUDIO
#endif
		|| ServiceType==SERVICE_TYPE_PROMOTIONVIDEO
		|| (fData && ServiceType==SERVICE_TYPE_DATA);
}
