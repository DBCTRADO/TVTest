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


CTSProcessorErrorDialog::~CTSProcessorErrorDialog()
{
}


bool CTSProcessorErrorDialog::Show(HWND hwndOwner)
{
	return ShowDialog(hwndOwner,
					  GetAppClass().GetResourceInstance(),
					  MAKEINTRESOURCE(IDD_TSPROCESSORERROR))==IDOK;
}


void CTSProcessorErrorDialog::SetMessage(LPCTSTR pszMessage)
{
	StringUtility::Assign(m_Message,pszMessage);
}


void CTSProcessorErrorDialog::SetDevice(const String &Device)
{
	m_Device=Device;
}


LPCTSTR CTSProcessorErrorDialog::GetDevice() const
{
	return StringUtility::GetCStrOrNull(m_Device);
}


void CTSProcessorErrorDialog::SetFilter(const String &Filter)
{
	m_Filter=Filter;
}


LPCTSTR CTSProcessorErrorDialog::GetFilter() const
{
	return StringUtility::GetCStrOrNull(m_Filter);
}


INT_PTR CTSProcessorErrorDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			::SendDlgItemMessage(hDlg,IDC_TSPROCESSORERROR_ICON,STM_SETICON,
				reinterpret_cast<WPARAM>(::LoadIcon(NULL,IDI_WARNING)),0);
			if (!m_Message.empty())
				::SetDlgItemText(hDlg,IDC_TSPROCESSORERROR_MESSAGE,m_Message.c_str());

			bool fFound=false;
			const int DeviceCount=m_pTSProcessor->GetDeviceCount();
			if (DeviceCount>0) {
				int Sel=0;
				for (int i=0;i<DeviceCount;i++) {
					String Name;
					m_pTSProcessor->GetDeviceName(i,&Name);
					LRESULT Index=DlgComboBox_AddString(hDlg,IDC_TSPROCESSORERROR_DEVICELIST,Name.c_str());
					DlgComboBox_SetItemData(hDlg,IDC_TSPROCESSORERROR_DEVICELIST,Index,i);
					if (StringUtility::CompareNoCase(m_Device,Name)==0)
						Sel=(int)Index;
				}
				DlgComboBox_SetCurSel(hDlg,IDC_TSPROCESSORERROR_DEVICELIST,Sel);
				fFound=SearchFilters();
				if (fFound)
					DlgListBox_SetCurSel(hDlg,IDC_TSPROCESSORERROR_FILTERLIST,0);
			}
			EnableDlgItem(hDlg,IDC_TSPROCESSORERROR_RETRY,DeviceCount>0);
			::CheckRadioButton(hDlg,
							   IDC_TSPROCESSORERROR_RETRY,
							   IDC_TSPROCESSORERROR_NOFILTER,
							   fFound?IDC_TSPROCESSORERROR_RETRY:IDC_TSPROCESSORERROR_NOFILTER);
			EnableDlgItems(hDlg,
						   IDC_TSPROCESSORERROR_DEVICELIST,
						   IDC_TSPROCESSORERROR_SEARCH,
						   fFound);
			AdjustDialogPos(::GetParent(hDlg),hDlg);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_TSPROCESSORERROR_RETRY:
		case IDC_TSPROCESSORERROR_NOFILTER:
			EnableDlgItems(hDlg,
						   IDC_TSPROCESSORERROR_DEVICELIST,
						   IDC_TSPROCESSORERROR_SEARCH,
						   DlgRadioButton_IsChecked(hDlg,IDC_TSPROCESSORERROR_RETRY));
			return TRUE;

		case IDC_TSPROCESSORERROR_DEVICELIST:
			if (HIWORD(wParam)!=CBN_SELCHANGE)
				return TRUE;
		case IDC_TSPROCESSORERROR_SEARCH:
			{
				HCURSOR hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));
				bool fFound=SearchFilters();
				::SetCursor(hcurOld);
				if (fFound) {
					DlgListBox_SetCurSel(hDlg,IDC_TSPROCESSORERROR_FILTERLIST,0);
				} else if (LOWORD(wParam)==IDC_TSPROCESSORERROR_SEARCH) {
					String Message,Text;

					Message=TEXT("�t�B���^�[�����t����܂���B");
					bool fAvailable;
					if (m_pTSProcessor->CheckDeviceAvailability(0,&fAvailable,&Text)
							&& !fAvailable
							&& !Text.empty()) {
						Message+=TEXT("\n");
						Message+=Text;
					}
					::MessageBox(hDlg,Message.c_str(),TEXT("����"),MB_OK | MB_ICONINFORMATION);
				}
			}
			return TRUE;

		case IDOK:
			{
				if (DlgRadioButton_IsChecked(hDlg,IDC_TSPROCESSORERROR_RETRY)) {
					int DeviceSel=(int)DlgComboBox_GetCurSel(hDlg,IDC_TSPROCESSORERROR_DEVICELIST);
					int FilterSel=(int)DlgListBox_GetCurSel(hDlg,IDC_TSPROCESSORERROR_FILTERLIST);

					if (DeviceSel<0 || FilterSel<0) {
						::MessageBox(hDlg,TEXT("�t�B���^�[��I�����Ă��������B"),TEXT("���肢"),MB_OK | MB_ICONINFORMATION);
						return TRUE;
					}

					int Device=(int)DlgComboBox_GetItemData(hDlg,IDC_TSPROCESSORERROR_DEVICELIST,DeviceSel);
					m_pTSProcessor->GetDeviceName(Device,&m_Device);

					LRESULT Length=DlgListBox_GetStringLength(hDlg,IDC_TSPROCESSORERROR_FILTERLIST,FilterSel);
					if (Length>0) {
						LPTSTR pszName=new TCHAR[Length+1];
						DlgListBox_GetString(hDlg,IDC_TSPROCESSORERROR_FILTERLIST,FilterSel,pszName);
						m_Filter=pszName;
						delete [] pszName;
					} else {
						m_Filter.clear();
					}
				} else {
					m_Device.clear();
					m_Filter.clear();
				}
			}
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;
	}

	return FALSE;
}


bool CTSProcessorErrorDialog::SearchFilters()
{
	DlgListBox_Clear(m_hDlg,IDC_TSPROCESSORERROR_FILTERLIST);

	int DeviceSel=(int)DlgComboBox_GetCurSel(m_hDlg,IDC_TSPROCESSORERROR_DEVICELIST);
	if (DeviceSel<0)
		return false;
	int Device=(int)DlgComboBox_GetItemData(m_hDlg,IDC_TSPROCESSORERROR_DEVICELIST,DeviceSel);

	bool fFound=false;

	if (m_pTSProcessor->IsDeviceAvailable(Device)) {
		std::vector<String> List;

		if (m_pTSProcessor->GetDeviceFilterList(Device,&List)) {
			for (size_t j=0;j<List.size();j++) {
				DlgListBox_AddString(m_hDlg,IDC_TSPROCESSORERROR_FILTERLIST,List[j].c_str());
				fFound=true;
			}
		}
	}

	return fFound;
}


}	// namespace TVTest
