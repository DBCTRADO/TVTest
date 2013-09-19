#include "stdafx.h"
#include "TVTest.h"
#include "EpgChannelSettings.h"
#include "ProgramGuide.h"
#include "AppMain.h"
#include "DialogUtil.h"
#include "LogoManager.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CEpgChannelSettings::CEpgChannelSettings(CProgramGuide *pProgramGuide)
	: m_pProgramGuide(pProgramGuide)
{
}


CEpgChannelSettings::~CEpgChannelSettings()
{
}


bool CEpgChannelSettings::Show(HWND hwndOwner)
{
	return ShowDialog(hwndOwner,
					  GetAppClass().GetResourceInstance(),
					  MAKEINTRESOURCE(IDD_EPGCHANNELSETTINGS))==IDOK;
}


INT_PTR CEpgChannelSettings::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	INT_PTR Result=CResizableDialog::DlgProc(hDlg,uMsg,wParam,lParam);

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			m_pProgramGuide->GetChannelList(&m_ChannelList,false);

			HWND hwndList=::GetDlgItem(hDlg,IDC_EPGCHANNELSETTINGS_CHANNELLIST);
			if (Util::OS::IsWindowsVistaOrLater())
				::SetWindowTheme(hwndList,L"explorer",NULL);
			ListView_SetExtendedListViewStyle(hwndList,
				LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_LABELTIP);

			const int IconWidth=::GetSystemMetrics(SM_CXSMICON);
			const int IconHeight=::GetSystemMetrics(SM_CYSMICON);
			HIMAGELIST himl=::ImageList_Create(IconWidth,IconHeight,ILC_COLOR24 | ILC_MASK,
											   m_ChannelList.NumChannels()+1,100);
			ImageList_AddIcon(himl,CreateEmptyIcon(IconWidth,IconHeight));
			ListView_SetImageList(hwndList,himl,LVSIL_SMALL);
			CLogoManager *pLogoManager=GetAppClass().GetLogoManager();

			RECT rc;
			::GetClientRect(hwndList,&rc);
			LVCOLUMN lvc;
			lvc.mask=LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
			lvc.fmt=LVCFMT_LEFT;
			lvc.cx=rc.right-::GetSystemMetrics(SM_CXVSCROLL);
			lvc.pszText=TEXT("");
			lvc.iSubItem=0;
			ListView_InsertColumn(hwndList,0,&lvc);

			LVITEM lvi;
			lvi.mask=LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
			lvi.iSubItem=0;
			for (int i=0;i<m_ChannelList.NumChannels();i++) {
				const CChannelInfo *pChannelInfo=m_ChannelList.GetChannelInfo(i);
				lvi.iItem=i;
				lvi.pszText=const_cast<LPTSTR>(pChannelInfo->GetName());
				HICON hico=pLogoManager->CreateLogoIcon(
					pChannelInfo->GetNetworkID(),
					pChannelInfo->GetServiceID(),
					IconWidth,IconHeight);
				if (hico!=NULL) {
					lvi.iImage=ImageList_AddIcon(himl,hico);
					::DestroyIcon(hico);
				} else {
					lvi.iImage=0;
				}
				lvi.lParam=reinterpret_cast<LPARAM>(pChannelInfo);
				lvi.iItem=ListView_InsertItem(hwndList,&lvi);
				ListView_SetCheckState(hwndList,lvi.iItem,
					!m_pProgramGuide->IsExcludeService(
						pChannelInfo->GetNetworkID(),
						pChannelInfo->GetTransportStreamID(),
						pChannelInfo->GetServiceID()));
			}

			DlgCheckBox_Check(hDlg,IDC_EPGCHANNELSETTINGS_EXCLUDENOEVENT,
							  m_pProgramGuide->GetExcludeNoEventServices());

			AddControl(IDC_EPGCHANNELSETTINGS_CHANNELLIST,ALIGN_ALL);
			AddControls(IDC_EPGCHANNELSETTINGS_CHECKALL,
						IDC_EPGCHANNELSETTINGS_INVERTCHECK,
						ALIGN_RIGHT);
			AddControl(IDC_EPGCHANNELSETTINGS_EXCLUDENOEVENT,ALIGN_BOTTOM);
			AddControl(IDOK,ALIGN_BOTTOM_RIGHT);
			AddControl(IDCANCEL,ALIGN_BOTTOM_RIGHT);

			ApplyPosition();
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_EPGCHANNELSETTINGS_CHECKALL:
		case IDC_EPGCHANNELSETTINGS_UNCHECKALL:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_EPGCHANNELSETTINGS_CHANNELLIST);
				const int ItemCount=ListView_GetItemCount(hwndList);
				const BOOL fCheck=LOWORD(wParam)==IDC_EPGCHANNELSETTINGS_CHECKALL;

				for (int i=0;i<ItemCount;i++) {
					ListView_SetCheckState(hwndList,i,fCheck);
				}
			}
			return TRUE;

		case IDC_EPGCHANNELSETTINGS_INVERTCHECK:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_EPGCHANNELSETTINGS_CHANNELLIST);
				const int ItemCount=ListView_GetItemCount(hwndList);

				for (int i=0;i<ItemCount;i++) {
					ListView_SetCheckState(hwndList,i,!ListView_GetCheckState(hwndList,i));
				}
			}
			return TRUE;

		case IDOK:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_EPGCHANNELSETTINGS_CHANNELLIST);
				const int ItemCount=ListView_GetItemCount(hwndList);

				LVITEM lvi;
				lvi.mask=LVIF_STATE | LVIF_PARAM;
				lvi.iSubItem=0;
				lvi.stateMask=~0U;
				for (int i=0;i<ItemCount;i++) {
					lvi.iItem=i;
					if (ListView_GetItem(hwndList,&lvi)) {
						const CChannelInfo *pChannelInfo=
							reinterpret_cast<const CChannelInfo*>(lvi.lParam);
						m_pProgramGuide->SetExcludeService(
							pChannelInfo->GetNetworkID(),
							pChannelInfo->GetTransportStreamID(),
							pChannelInfo->GetServiceID(),
							(lvi.state & LVIS_STATEIMAGEMASK)!=INDEXTOSTATEIMAGEMASK(2));
					}
				}

				m_pProgramGuide->SetExcludeNoEventServices(
					DlgCheckBox_IsChecked(hDlg,IDC_EPGCHANNELSETTINGS_EXCLUDENOEVENT));
			}
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_DESTROY:
		m_ChannelList.Clear();
		return TRUE;
	}

	return Result;
}
