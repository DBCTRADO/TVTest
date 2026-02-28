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
#include "ChannelInput.h"
#include "DialogUtil.h"
#include "resource.h"
#include "Help/HelpID.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CChannelInputOptions::CChannelInputOptions()
{
	for (auto &e : KeyInputMode)
		e = KeyInputModeType::SingleKey;
}




CChannelInput::CChannelInput(const CChannelInputOptions &Options)
	: m_Options(Options)
{
}


bool CChannelInput::BeginInput(int MaxDigits)
{
	m_fInputting = true;
	m_MaxDigits = MaxDigits;
	m_CurDigits = 0;
	m_Number = 0;
	return true;
}


void CChannelInput::EndInput()
{
	m_fInputting = false;
}


CChannelInput::KeyDownResult CChannelInput::OnKeyDown(WPARAM wParam)
{
	CChannelInputOptions::KeyType KeyType;
	int Number = -1;

	if (wParam >= '0' && wParam <= '9') {
		KeyType = CChannelInputOptions::KeyType::Digit;
		Number = static_cast<int>(wParam) - '0';
	} else if (wParam >= VK_NUMPAD0 && wParam <= VK_NUMPAD9) {
		KeyType = CChannelInputOptions::KeyType::NumPad;
		Number = static_cast<int>(wParam) - VK_NUMPAD0;
	} else if (wParam >= VK_F1 && wParam <= VK_F12) {
		KeyType = CChannelInputOptions::KeyType::Function;
		Number = (static_cast<int>(wParam) - VK_F1) + 1;
	}

	if (Number >= 0) {
		if (m_Options.KeyInputMode[static_cast<int>(KeyType)] == CChannelInputOptions::KeyInputModeType::Disabled)
			return KeyDownResult::NotProcessed;

		if (!m_fInputting) {
			if (m_Options.KeyInputMode[static_cast<int>(KeyType)] == CChannelInputOptions::KeyInputModeType::SingleKey) {
				m_fInputting = true;
				m_Number = Number == 0 ? 10 : Number;
				m_MaxDigits = m_Number <= 9 ? 1 : 2;
				m_CurDigits = m_MaxDigits;
				return KeyDownResult::Completed;
			}

			m_fInputting = true;
			m_MaxDigits = 0;
			if (Number == 0)
				m_Number = 10;
			else
				m_Number = Number;
			m_CurDigits = m_Number <= 9 ? 1 : 2;
			return KeyDownResult::Begin;
		}

		m_Number = m_Number * 10 + Number;
		m_CurDigits++;
		if (m_MaxDigits > 0 && m_CurDigits >= m_MaxDigits)
			return KeyDownResult::Completed;
		return KeyDownResult::Continue;
	} else if (m_fInputting) {
		switch (wParam) {
		case VK_ESCAPE:
			return KeyDownResult::Cancelled;

		case VK_RETURN:
			if (m_CurDigits < 1)
				return KeyDownResult::Cancelled;
			return KeyDownResult::Completed;

		case VK_BACK:
			if (m_CurDigits < 2)
				return KeyDownResult::Cancelled;
			m_CurDigits--;
			m_Number /= 10;
			return KeyDownResult::Continue;
		}
	}

	return KeyDownResult::NotProcessed;
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
	return ShowDialog(
		hwndOwner,
		GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE(IDD_CHANNELINPUTOPTIONS)) == IDOK;
}


INT_PTR CChannelInputOptionsDialog::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			static const LPCTSTR KeyModeList[] = {
				TEXT("無効"),
				TEXT("単キー"),
				TEXT("連続入力"),
			};

			for (int i = 0; i <= static_cast<int>(CChannelInputOptions::KeyType::Last_); i++) {
				const int ID = IDC_CHANNELINPUT_DIGITKEYMODE + i;
				for (const LPCTSTR pszMode : KeyModeList)
					DlgComboBox_AddString(hDlg, ID, pszMode);
				DlgComboBox_SetCurSel(hDlg, ID, static_cast<int>(m_Options.KeyInputMode[i]));
			}

			DlgEdit_SetUInt(hDlg, IDC_CHANNELINPUT_TIMEOUT, m_Options.KeyTimeout);

			DlgComboBox_AddString(hDlg, IDC_CHANNELINPUT_TIMEOUTMODE, TEXT("確定"));
			DlgComboBox_AddString(hDlg, IDC_CHANNELINPUT_TIMEOUTMODE, TEXT("キャンセル"));
			DlgComboBox_SetCurSel(hDlg, IDC_CHANNELINPUT_TIMEOUTMODE, m_Options.fKeyTimeoutCancel ? 1 : 0);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CHANNELINPUT_HELP:
			GetAppClass().UICore.ShowHelpContent(HELP_ID_CHANNELINPUTOPTIONS);
			return TRUE;

		case IDOK:
			{
				for (int i = 0; i <= static_cast<int>(CChannelInputOptions::KeyType::Last_); i++) {
					const int Sel = static_cast<int>(DlgComboBox_GetCurSel(hDlg, IDC_CHANNELINPUT_DIGITKEYMODE + i));
					if (Sel >= 0) {
						m_Options.KeyInputMode[i] =
							static_cast<CChannelInputOptions::KeyInputModeType>(Sel);
					}
				}

				m_Options.KeyTimeout =
					DlgEdit_GetUInt(hDlg, IDC_CHANNELINPUT_TIMEOUT);
				m_Options.fKeyTimeoutCancel =
					DlgComboBox_GetCurSel(hDlg, IDC_CHANNELINPUT_TIMEOUTMODE) == 1;
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


} // namespace TVTest
