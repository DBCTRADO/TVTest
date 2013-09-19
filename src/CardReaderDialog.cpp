#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "CardReaderDialog.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CCardReaderErrorDialog::CCardReaderErrorDialog()
	: m_CasDevice(0)
{
}


CCardReaderErrorDialog::~CCardReaderErrorDialog()
{
}


bool CCardReaderErrorDialog::Show(HWND hwndOwner)
{
	return ShowDialog(hwndOwner,
					  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_CARDREADER))==IDOK;
}


bool CCardReaderErrorDialog::SetMessage(LPCTSTR pszMessage)
{
	return m_Message.Set(pszMessage);
}


static bool SearchReaders(HWND hDlg)
{
	CCasProcessor &CasProcessor=GetAppClass().GetCoreEngine()->m_DtvEngine.m_CasProcessor;
	const int DeviceCount=CasProcessor.GetCasDeviceCount();
	bool fFound=false;

	for (int i=0;i<DeviceCount;i++) {
		if (CasProcessor.IsCasDeviceAvailable(i)) {
			CCasProcessor::StringList List;

			if (CasProcessor.GetCasDeviceCardList(i,&List)) {
				for (size_t j=0;j<List.size();j++) {
					LRESULT Index=DlgListBox_AddString(hDlg,IDC_CARDREADER_READERLIST,List[j].c_str());
					DlgListBox_SetItemData(hDlg,IDC_CARDREADER_READERLIST,Index,i);
					fFound=true;
				}
			}
		}
	}

	return fFound;
}


INT_PTR CCardReaderErrorDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			::SendDlgItemMessage(hDlg,IDC_CARDREADER_ICON,STM_SETICON,
				reinterpret_cast<WPARAM>(::LoadIcon(NULL,IDI_WARNING)),0);
			if (!m_Message.IsEmpty())
				::SetDlgItemText(hDlg,IDC_CARDREADER_MESSAGE,m_Message.Get());
			bool fFound=SearchReaders(hDlg);
			EnableDlgItem(hDlg,IDC_CARDREADER_RETRY,fFound);
			::CheckRadioButton(hDlg,IDC_CARDREADER_RETRY,IDC_CARDREADER_NOREADER,
							   fFound && m_CasDevice>=0?
							   IDC_CARDREADER_RETRY:IDC_CARDREADER_NOREADER);
			EnableDlgItem(hDlg,IDC_CARDREADER_READERLIST,
						  fFound && m_CasDevice>=0);
			if (fFound)
				DlgListBox_SetCurSel(hDlg,IDC_CARDREADER_READERLIST,0);
			AdjustDialogPos(::GetParent(hDlg),hDlg);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CARDREADER_RETRY:
		case IDC_CARDREADER_NOREADER:
			EnableDlgItem(hDlg,IDC_CARDREADER_READERLIST,
						  DlgRadioButton_IsChecked(hDlg,IDC_CARDREADER_RETRY));
			return TRUE;

		case IDC_CARDREADER_SEARCH:
			{
				DlgListBox_Clear(hDlg,IDC_CARDREADER_READERLIST);
				HCURSOR hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));
				bool fFound=SearchReaders(hDlg);
				::SetCursor(hcurOld);
				EnableDlgItem(hDlg,IDC_CARDREADER_RETRY,fFound);
				::CheckRadioButton(hDlg,IDC_CARDREADER_RETRY,IDC_CARDREADER_NOREADER,
								   fFound?IDC_CARDREADER_RETRY:IDC_CARDREADER_NOREADER);
				EnableDlgItem(hDlg,IDC_CARDREADER_READERLIST,
							  fFound && DlgRadioButton_IsChecked(hDlg,IDC_CARDREADER_RETRY));
				if (fFound) {
					DlgListBox_SetCurSel(hDlg,IDC_CARDREADER_READERLIST,0);
				} else {
					CCasProcessor &CasProcessor=GetAppClass().GetCoreEngine()->m_DtvEngine.m_CasProcessor;
					TCHAR szMessage[1024],szText[256];

					::lstrcpy(szMessage,TEXT("カードリーダが見付かりません。"));
					bool fAvailable;
					szText[0]=_T('\0');
					if (CasProcessor.CheckCasDeviceAvailability(0,&fAvailable,szText,lengthof(szText))
							&& !fAvailable
							&& szText[0]!=_T('\0')) {
						::lstrcat(szMessage,TEXT("\n"));
						::lstrcat(szMessage,szText);
					}
					::MessageBox(hDlg,szMessage,TEXT("結果"),MB_OK | MB_ICONINFORMATION);
				}
			}
			return TRUE;

		case IDOK:
			{
				if (DlgRadioButton_IsChecked(hDlg,IDC_CARDREADER_RETRY)) {
					LRESULT Sel=DlgListBox_GetCurSel(hDlg,IDC_CARDREADER_READERLIST);

					if (Sel<0) {
						::MessageBox(hDlg,TEXT("カードリーダを選択してください。"),TEXT("お願い"),MB_OK | MB_ICONINFORMATION);
						return TRUE;
					}
					m_CasDevice=(int)DlgListBox_GetItemData(hDlg,IDC_CARDREADER_READERLIST,Sel);
					LRESULT Length=DlgListBox_GetStringLength(hDlg,IDC_CARDREADER_READERLIST,Sel);
					if (Length>0) {
						LPTSTR pszName=new TCHAR[Length+1];
						DlgListBox_GetString(hDlg,IDC_CARDREADER_READERLIST,Sel,pszName);
						m_ReaderName.Set(pszName);
						delete [] pszName;
					} else {
						m_ReaderName.Clear();
					}
				} else {
					m_CasDevice=-1;
					m_ReaderName.Clear();
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
