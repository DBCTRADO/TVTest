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
#include "EpgChannelSettings.h"
#include "ProgramGuide.h"
#include "AppMain.h"
#include "DialogUtil.h"
#include "DPIUtil.h"
#include "LogoManager.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CEpgChannelSettings::CEpgChannelSettings(CProgramGuide *pProgramGuide)
	: m_pProgramGuide(pProgramGuide)
{
}


bool CEpgChannelSettings::Show(HWND hwndOwner)
{
	return ShowDialog(
		hwndOwner,
		GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE(IDD_EPGCHANNELSETTINGS)) == IDOK;
}


INT_PTR CEpgChannelSettings::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			m_pProgramGuide->GetChannelList(&m_ChannelList, false);

			const HWND hwndList = ::GetDlgItem(hDlg, IDC_EPGCHANNELSETTINGS_CHANNELLIST);
			::SetWindowTheme(hwndList, L"explorer", nullptr);
			ListView_SetExtendedListViewStyle(
				hwndList,
				LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_LABELTIP);
			SetListViewTooltipsTopMost(hwndList);

			const int IconWidth = GetSystemMetricsWithDPI(SM_CXSMICON, m_CurrentDPI);
			const int IconHeight = GetSystemMetricsWithDPI(SM_CYSMICON, m_CurrentDPI);
			const HIMAGELIST himl = ::ImageList_Create(
				IconWidth, IconHeight, ILC_COLOR24 | ILC_MASK,
				m_ChannelList.NumChannels() + 1, 100);
			const HICON hico = CreateEmptyIcon(IconWidth, IconHeight);
			ImageList_AddIcon(himl, hico);
			::DestroyIcon(hico);
			ListView_SetImageList(hwndList, himl, LVSIL_SMALL);
			CLogoManager &LogoManager = GetAppClass().LogoManager;

			RECT rc;
			::GetClientRect(hwndList, &rc);
			LVCOLUMN lvc;
			lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
			lvc.fmt = LVCFMT_LEFT;
			lvc.cx = rc.right - GetScrollBarWidth(hwndList);
			lvc.pszText = const_cast<LPTSTR>(TEXT(""));
			lvc.iSubItem = 0;
			ListView_InsertColumn(hwndList, 0, &lvc);

			LVITEM lvi;
			lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
			lvi.iSubItem = 0;
			for (int i = 0; i < m_ChannelList.NumChannels(); i++) {
				const CChannelInfo *pChannelInfo = m_ChannelList.GetChannelInfo(i);
				lvi.iItem = i;
				lvi.pszText = const_cast<LPTSTR>(pChannelInfo->GetName());
				const HICON hico = LogoManager.CreateLogoIcon(
					pChannelInfo->GetNetworkID(),
					pChannelInfo->GetServiceID(),
					IconWidth, IconHeight);
				if (hico != nullptr) {
					lvi.iImage = ImageList_AddIcon(himl, hico);
					::DestroyIcon(hico);
				} else {
					lvi.iImage = 0;
				}
				lvi.lParam = reinterpret_cast<LPARAM>(pChannelInfo);
				lvi.iItem = ListView_InsertItem(hwndList, &lvi);
				ListView_SetCheckState(
					hwndList, lvi.iItem,
					!m_pProgramGuide->IsExcludeService(
						pChannelInfo->GetNetworkID(),
						pChannelInfo->GetTransportStreamID(),
						pChannelInfo->GetServiceID()));
			}

			DlgCheckBox_Check(
				hDlg, IDC_EPGCHANNELSETTINGS_EXCLUDENOEVENT,
				m_pProgramGuide->GetExcludeNoEventServices());

			AddControl(IDC_EPGCHANNELSETTINGS_CHANNELLIST, AlignFlag::All);
			AddControls(
				IDC_EPGCHANNELSETTINGS_CHECKALL,
				IDC_EPGCHANNELSETTINGS_INVERTCHECK,
				AlignFlag::Right);
			AddControl(IDC_EPGCHANNELSETTINGS_EXCLUDENOEVENT, AlignFlag::Bottom);
			AddControl(IDOK, AlignFlag::BottomRight);
			AddControl(IDCANCEL, AlignFlag::BottomRight);

			ApplyPosition();
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_EPGCHANNELSETTINGS_CHECKALL:
		case IDC_EPGCHANNELSETTINGS_UNCHECKALL:
			{
				const HWND hwndList = ::GetDlgItem(hDlg, IDC_EPGCHANNELSETTINGS_CHANNELLIST);
				const int ItemCount = ListView_GetItemCount(hwndList);
				const BOOL fCheck = LOWORD(wParam) == IDC_EPGCHANNELSETTINGS_CHECKALL;

				for (int i = 0; i < ItemCount; i++) {
					ListView_SetCheckState(hwndList, i, fCheck);
				}
			}
			return TRUE;

		case IDC_EPGCHANNELSETTINGS_INVERTCHECK:
			{
				const HWND hwndList = ::GetDlgItem(hDlg, IDC_EPGCHANNELSETTINGS_CHANNELLIST);
				const int ItemCount = ListView_GetItemCount(hwndList);

				for (int i = 0; i < ItemCount; i++) {
					ListView_SetCheckState(hwndList, i, !ListView_GetCheckState(hwndList, i));
				}
			}
			return TRUE;

		case IDOK:
			{
				const HWND hwndList = ::GetDlgItem(hDlg, IDC_EPGCHANNELSETTINGS_CHANNELLIST);
				const int ItemCount = ListView_GetItemCount(hwndList);

				LVITEM lvi;
				lvi.mask = LVIF_STATE | LVIF_PARAM;
				lvi.iSubItem = 0;
				lvi.stateMask = ~0U;
				for (int i = 0; i < ItemCount; i++) {
					lvi.iItem = i;
					if (ListView_GetItem(hwndList, &lvi)) {
						const CChannelInfo *pChannelInfo =
							reinterpret_cast<const CChannelInfo*>(lvi.lParam);
						m_pProgramGuide->SetExcludeService(
							pChannelInfo->GetNetworkID(),
							pChannelInfo->GetTransportStreamID(),
							pChannelInfo->GetServiceID(),
							(lvi.state & LVIS_STATEIMAGEMASK) != INDEXTOSTATEIMAGEMASK(2));
					}
				}

				m_pProgramGuide->SetExcludeNoEventServices(
					DlgCheckBox_IsChecked(hDlg, IDC_EPGCHANNELSETTINGS_EXCLUDENOEVENT));
			}
			[[fallthrough]];
		case IDCANCEL:
			::EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_DESTROY:
		m_ChannelList.Clear();
		return TRUE;
	}

	return FALSE;
}


}	// namespace TVTest
