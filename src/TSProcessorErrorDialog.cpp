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
#include "TSProcessorErrorDialog.h"
#include "DialogUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CTSProcessorErrorDialog::CTSProcessorErrorDialog(CTSProcessor *pTSProcessor)
	: m_pTSProcessor(pTSProcessor)
{
}


bool CTSProcessorErrorDialog::Show(HWND hwndOwner)
{
	return ShowDialog(
		hwndOwner,
		GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE(IDD_TSPROCESSORERROR)) == IDOK;
}


void CTSProcessorErrorDialog::SetMessage(LPCTSTR pszMessage)
{
	StringUtility::Assign(m_Message, pszMessage);
}


void CTSProcessorErrorDialog::SetDevice(const String &Device)
{
	m_Device = Device;
}


LPCTSTR CTSProcessorErrorDialog::GetDevice() const
{
	return StringUtility::GetCStrOrNull(m_Device);
}


void CTSProcessorErrorDialog::SetFilter(const String &Filter)
{
	m_Filter = Filter;
}


LPCTSTR CTSProcessorErrorDialog::GetFilter() const
{
	return StringUtility::GetCStrOrNull(m_Filter);
}


INT_PTR CTSProcessorErrorDialog::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			::SendDlgItemMessage(
				hDlg, IDC_TSPROCESSORERROR_ICON, STM_SETICON,
				reinterpret_cast<WPARAM>(::LoadIcon(nullptr, IDI_WARNING)), 0);
			if (!m_Message.empty())
				::SetDlgItemText(hDlg, IDC_TSPROCESSORERROR_MESSAGE, m_Message.c_str());

			bool fFound = false;
			const int DeviceCount = m_pTSProcessor->GetDeviceCount();
			if (DeviceCount > 0) {
				int Sel = 0;
				for (int i = 0; i < DeviceCount; i++) {
					String Name;
					m_pTSProcessor->GetDeviceName(i, &Name);
					const LRESULT Index = DlgComboBox_AddString(hDlg, IDC_TSPROCESSORERROR_DEVICELIST, Name.c_str());
					DlgComboBox_SetItemData(hDlg, IDC_TSPROCESSORERROR_DEVICELIST, Index, i);
					if (StringUtility::IsEqualNoCase(m_Device, Name))
						Sel = static_cast<int>(Index);
				}
				DlgComboBox_SetCurSel(hDlg, IDC_TSPROCESSORERROR_DEVICELIST, Sel);
				fFound = SearchFilters();
				if (fFound)
					DlgListBox_SetCurSel(hDlg, IDC_TSPROCESSORERROR_FILTERLIST, 0);
			}
			EnableDlgItem(hDlg, IDC_TSPROCESSORERROR_RETRY, DeviceCount > 0);
			::CheckRadioButton(
				hDlg,
				IDC_TSPROCESSORERROR_RETRY,
				IDC_TSPROCESSORERROR_NOFILTER,
				fFound ? IDC_TSPROCESSORERROR_RETRY : IDC_TSPROCESSORERROR_NOFILTER);
			EnableDlgItems(
				hDlg,
				IDC_TSPROCESSORERROR_DEVICELIST,
				IDC_TSPROCESSORERROR_SEARCH,
				fFound);
			AdjustDialogPos(::GetParent(hDlg), hDlg);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_TSPROCESSORERROR_RETRY:
		case IDC_TSPROCESSORERROR_NOFILTER:
			EnableDlgItems(
				hDlg,
				IDC_TSPROCESSORERROR_DEVICELIST,
				IDC_TSPROCESSORERROR_SEARCH,
				DlgRadioButton_IsChecked(hDlg, IDC_TSPROCESSORERROR_RETRY));
			return TRUE;

		case IDC_TSPROCESSORERROR_DEVICELIST:
			if (HIWORD(wParam) != CBN_SELCHANGE)
				return TRUE;
			[[fallthrough]];
		case IDC_TSPROCESSORERROR_SEARCH:
			{
				const HCURSOR hcurOld = ::SetCursor(::LoadCursor(nullptr, IDC_WAIT));
				const bool fFound = SearchFilters();
				::SetCursor(hcurOld);
				if (fFound) {
					DlgListBox_SetCurSel(hDlg, IDC_TSPROCESSORERROR_FILTERLIST, 0);
				} else if (LOWORD(wParam) == IDC_TSPROCESSORERROR_SEARCH) {
					String Message, Text;

					Message = TEXT("フィルターが見付かりません。");
					bool fAvailable;
					if (m_pTSProcessor->CheckDeviceAvailability(0, &fAvailable, &Text)
							&& !fAvailable
							&& !Text.empty()) {
						Message += TEXT("\n");
						Message += Text;
					}
					::MessageBox(hDlg, Message.c_str(), TEXT("結果"), MB_OK | MB_ICONINFORMATION);
				}
			}
			return TRUE;

		case IDOK:
			{
				if (DlgRadioButton_IsChecked(hDlg, IDC_TSPROCESSORERROR_RETRY)) {
					const int DeviceSel = static_cast<int>(DlgComboBox_GetCurSel(hDlg, IDC_TSPROCESSORERROR_DEVICELIST));
					const int FilterSel = static_cast<int>(DlgListBox_GetCurSel(hDlg, IDC_TSPROCESSORERROR_FILTERLIST));

					if (DeviceSel < 0 || FilterSel < 0) {
						::MessageBox(hDlg, TEXT("フィルターを選択してください。"), TEXT("お願い"), MB_OK | MB_ICONINFORMATION);
						return TRUE;
					}

					const int Device = static_cast<int>(DlgComboBox_GetItemData(hDlg, IDC_TSPROCESSORERROR_DEVICELIST, DeviceSel));
					m_pTSProcessor->GetDeviceName(Device, &m_Device);

					const LRESULT Length = DlgListBox_GetStringLength(hDlg, IDC_TSPROCESSORERROR_FILTERLIST, FilterSel);
					if (Length > 0) {
						m_Filter.resize(Length + 1);
						DlgListBox_GetString(hDlg, IDC_TSPROCESSORERROR_FILTERLIST, FilterSel, m_Filter.data());
						m_Filter.pop_back();
					} else {
						m_Filter.clear();
					}
				} else {
					m_Device.clear();
					m_Filter.clear();
				}
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


bool CTSProcessorErrorDialog::SearchFilters()
{
	DlgListBox_Clear(m_hDlg, IDC_TSPROCESSORERROR_FILTERLIST);

	const int DeviceSel = static_cast<int>(DlgComboBox_GetCurSel(m_hDlg, IDC_TSPROCESSORERROR_DEVICELIST));
	if (DeviceSel < 0)
		return false;
	const int Device = static_cast<int>(DlgComboBox_GetItemData(m_hDlg, IDC_TSPROCESSORERROR_DEVICELIST, DeviceSel));

	bool fFound = false;

	if (m_pTSProcessor->IsDeviceAvailable(Device)) {
		std::vector<String> List;

		if (m_pTSProcessor->GetDeviceFilterList(Device, &List)) {
			for (const String &e : List) {
				DlgListBox_AddString(m_hDlg, IDC_TSPROCESSORERROR_FILTERLIST, e.c_str());
				fFound = true;
			}
		}
	}

	return fFound;
}


} // namespace TVTest
