#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "ChannelInput.h"
#include "DialogUtil.h"
#include "resource.h"
#include "Help/HelpID.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CChannelInputOptions::CChannelInputOptions()
	: KeyTimeout(2000)
	, fKeyTimeoutCancel(false)
{
	for (int i=0;i<lengthof(KeyInputMode);i++)
		KeyInputMode[i]=KEYINPUTMODE_SINGLEKEY;
}




CChannelInput::CChannelInput(const CChannelInputOptions &Options)
	: m_Options(Options)
	, m_fInputting(false)
	, m_MaxDigits(0)
	, m_CurDigits(0)
	, m_Number(0)
{
}


bool CChannelInput::BeginInput(int MaxDigits)
{
	m_fInputting=true;
	m_MaxDigits=MaxDigits;
	m_CurDigits=0;
	m_Number=0;
	return true;
}


void CChannelInput::EndInput()
{
	m_fInputting=false;
}


CChannelInput::KeyDownResult CChannelInput::OnKeyDown(WPARAM wParam)
{
	CChannelInputOptions::KeyType KeyType;
	int Number=-1;

	if (wParam>='0' && wParam<='9') {
		KeyType=CChannelInputOptions::KEY_DIGIT;
		Number=(int)wParam-'0';
	} else if (wParam>=VK_NUMPAD0 && wParam<=VK_NUMPAD9) {
		KeyType=CChannelInputOptions::KEY_NUMPAD;
		Number=(int)wParam-VK_NUMPAD0;
	} else if (wParam>=VK_F1 && wParam<=VK_F12) {
		KeyType=CChannelInputOptions::KEY_FUNCTION;
		Number=((int)wParam-VK_F1)+1;
	}

	if (Number>=0) {
		if (m_Options.KeyInputMode[KeyType]==CChannelInputOptions::KEYINPUTMODE_DISABLED)
			return KEYDOWN_NOTPROCESSED;

		if (!m_fInputting) {
			if (m_Options.KeyInputMode[KeyType]==CChannelInputOptions::KEYINPUTMODE_SINGLEKEY) {
				m_fInputting=true;
				m_Number=Number==0?10:Number;
				m_MaxDigits=m_Number<=9?1:2;
				m_CurDigits=m_MaxDigits;
				return KEYDOWN_COMPLETED;
			}

			m_fInputting=true;
			m_MaxDigits=0;
			if (Number==0)
				m_Number=10;
			else
				m_Number=Number;
			m_CurDigits=m_Number<=9?1:2;
			return KEYDOWN_BEGIN;
		}

		m_Number=m_Number*10+Number;
		m_CurDigits++;
		if (m_MaxDigits>0 && m_CurDigits>=m_MaxDigits)
			return KEYDOWN_COMPLETED;
		return KEYDOWN_CONTINUE;
	} else if (m_fInputting) {
		switch (wParam) {
		case VK_ESCAPE:
			return KEYDOWN_CANCELLED;

		case VK_RETURN:
			if (m_CurDigits<1)
				return KEYDOWN_CANCELLED;
			return KEYDOWN_COMPLETED;

		case VK_BACK:
			if (m_CurDigits<2)
				return KEYDOWN_CANCELLED;
			m_CurDigits--;
			m_Number/=10;
			return KEYDOWN_CONTINUE;
		}
	}

	return KEYDOWN_NOTPROCESSED;
}


bool CChannelInput::IsKeyNeeded(WPARAM wParam) const
{
	if (m_fInputting) {
		switch (wParam) {
		case VK_RETURN:
		case VK_ESCAPE:
		case VK_BACK:
			return true;
		}
	}
	return false;
}




CChannelInputOptionsDialog::CChannelInputOptionsDialog(CChannelInputOptions &Options)
	: m_Options(Options)
{
}


bool CChannelInputOptionsDialog::Show(HWND hwndOwner)
{
	return ShowDialog(hwndOwner,
					  GetAppClass().GetResourceInstance(),
					  MAKEINTRESOURCE(IDD_CHANNELINPUTOPTIONS))==IDOK;
}


INT_PTR CChannelInputOptionsDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			static const LPCTSTR KeyModeList[] = {
				TEXT("無効"),
				TEXT("単キー"),
				TEXT("連続入力"),
			};

			for (int i=0;i<=CChannelInputOptions::KEY_LAST;i++) {
				int ID=IDC_CHANNELINPUT_DIGITKEYMODE+i;
				for (int j=0;j<lengthof(KeyModeList);j++)
					DlgComboBox_AddString(hDlg,ID,KeyModeList[j]);
				DlgComboBox_SetCurSel(hDlg,ID,m_Options.KeyInputMode[i]);
			}

			DlgEdit_SetUInt(hDlg,IDC_CHANNELINPUT_TIMEOUT,m_Options.KeyTimeout);

			DlgComboBox_AddString(hDlg,IDC_CHANNELINPUT_TIMEOUTMODE,TEXT("確定"));
			DlgComboBox_AddString(hDlg,IDC_CHANNELINPUT_TIMEOUTMODE,TEXT("キャンセル"));
			DlgComboBox_SetCurSel(hDlg,IDC_CHANNELINPUT_TIMEOUTMODE,
								  m_Options.fKeyTimeoutCancel?1:0);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CHANNELINPUT_HELP:
			GetAppClass().UICore.ShowHelpContent(HELP_ID_CHANNELINPUTOPTIONS);
			return TRUE;

		case IDOK:
			{
				for (int i=0;i<=CChannelInputOptions::KEY_LAST;i++) {
					int Sel=(int)DlgComboBox_GetCurSel(hDlg,IDC_CHANNELINPUT_DIGITKEYMODE+i);
					if (Sel>=0) {
						m_Options.KeyInputMode[i]=
							static_cast<CChannelInputOptions::KeyInputModeType>(Sel);
					}
				}

				m_Options.KeyTimeout=
					DlgEdit_GetUInt(hDlg,IDC_CHANNELINPUT_TIMEOUT);
				m_Options.fKeyTimeoutCancel=
					DlgComboBox_GetCurSel(hDlg,IDC_CHANNELINPUT_TIMEOUTMODE)==1;
			}
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;
	}

	return FALSE;
}


}	// namespace TVTest
