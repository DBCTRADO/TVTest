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
#include "OperationOptions.h"
#include "DialogUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


COperationOptions::COperationOptions()
	: m_WheelCommand(CM_WHEEL_VOLUME)
	, m_WheelShiftCommand(CM_WHEEL_CHANNEL)
	, m_WheelCtrlCommand(CM_WHEEL_AUDIO)
	, m_WheelTiltCommand(0)
	, m_LeftDoubleClickCommand(CM_FULLSCREEN)
	, m_RightClickCommand(CM_MENU)
	, m_MiddleClickCommand(0)
{
}


COperationOptions::~COperationOptions()
{
	Destroy();
}


bool COperationOptions::ReadSettings(CSettings &Settings)
{
	int Value;

	Settings.Read(TEXT("DisplayDragMove"), &m_fDisplayDragMove);
	Settings.Read(TEXT("VolumeStep"), &m_VolumeStep);
	Settings.Read(TEXT("AudioDelayStep"), &m_AudioDelayStep);
	if (Settings.Read(TEXT("ChannelUpDownOrder"), &Value))
		m_ChannelUpDownOrder = static_cast<CChannelManager::UpDownOrder>(Value);
	Settings.Read(TEXT("ChannelUpDownSkipSubChannel"), &m_fChannelUpDownSkipSubChannel);

	// ver.0.9.0 より前との互換用
	static const int WheelModeList[] = {
		0,
		CM_WHEEL_VOLUME,
		CM_WHEEL_CHANNEL,
		CM_WHEEL_AUDIO,
		CM_WHEEL_ZOOM,
		CM_WHEEL_ASPECTRATIO,
	};
	String Command;
	if (Settings.Read(TEXT("WheelCommand"), &Command)) {
		m_WheelCommand = m_WheelCommandManager.ParseCommand(Command.c_str());
	} else if (Settings.Read(TEXT("WheelMode"), &Value)
			&& Value >= 0 && Value < lengthof(WheelModeList)) {
		m_WheelCommand = WheelModeList[Value];
	}
	if (Settings.Read(TEXT("WheelShiftCommand"), &Command)) {
		m_WheelShiftCommand = m_WheelCommandManager.ParseCommand(Command.c_str());
	} else if (Settings.Read(TEXT("WheelShiftMode"), &Value)
			&& Value >= 0 && Value < lengthof(WheelModeList)) {
		m_WheelShiftCommand = WheelModeList[Value];
	}
	if (Settings.Read(TEXT("WheelCtrlCommand"), &Command)) {
		m_WheelCtrlCommand = m_WheelCommandManager.ParseCommand(Command.c_str());
	} else if (Settings.Read(TEXT("WheelCtrlMode"), &Value)
			&& Value >= 0 && Value < lengthof(WheelModeList)) {
		m_WheelCtrlCommand = WheelModeList[Value];
	}
	if (Settings.Read(TEXT("WheelTiltCommand"), &Command)) {
		m_WheelTiltCommand = m_WheelCommandManager.ParseCommand(Command.c_str());
	} else if (Settings.Read(TEXT("WheelTiltMode"), &Value)
			&& Value >= 0 && Value < lengthof(WheelModeList)) {
		m_WheelTiltCommand = WheelModeList[Value];
	}

	Settings.Read(TEXT("StatusBarWheel"), &m_fStatusBarWheel);
	Settings.Read(TEXT("ReverseWheelChannel"), &m_fWheelChannelReverse);
	Settings.Read(TEXT("ReverseWheelVolume"), &m_fWheelVolumeReverse);
	if (Settings.Read(TEXT("WheelChannelDelay"), &Value)) {
		if (Value < WHEEL_CHANNEL_DELAY_MIN)
			Value = WHEEL_CHANNEL_DELAY_MIN;
		m_WheelChannelDelay = Value;
	}
	Settings.Read(TEXT("WheelZoomStep"), &m_WheelZoomStep);

	if (m_pCommandManager != nullptr) {
		String Command;

		if (Settings.Read(TEXT("LeftDoubleClickCommand"), &Command)) {
			m_LeftDoubleClickCommand = m_pCommandManager->ParseIDText(Command);
		}
		if (Settings.Read(TEXT("RightClickCommand"), &Command)) {
			m_RightClickCommand = m_pCommandManager->ParseIDText(Command);
		}
		if (Settings.Read(TEXT("MiddleClickCommand"), &Command)) {
			m_MiddleClickCommand = m_pCommandManager->ParseIDText(Command);
		}
	}

	return true;
}


bool COperationOptions::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("DisplayDragMove"), m_fDisplayDragMove);
	Settings.Write(TEXT("VolumeStep"), m_VolumeStep);
	Settings.Write(TEXT("AudioDelayStep"), m_AudioDelayStep);
	Settings.Write(TEXT("ChannelUpDownOrder"), static_cast<int>(m_ChannelUpDownOrder));
	Settings.Write(TEXT("ChannelUpDownSkipSubChannel"), m_fChannelUpDownSkipSubChannel);

	TCHAR szCommand[CWheelCommandManager::MAX_COMMAND_PARSABLE_NAME];
	m_WheelCommandManager.GetCommandParsableName(m_WheelCommand, szCommand, lengthof(szCommand));
	Settings.Write(TEXT("WheelCommand"), szCommand);
	m_WheelCommandManager.GetCommandParsableName(m_WheelShiftCommand, szCommand, lengthof(szCommand));
	Settings.Write(TEXT("WheelShiftCommand"), szCommand);
	m_WheelCommandManager.GetCommandParsableName(m_WheelCtrlCommand, szCommand, lengthof(szCommand));
	Settings.Write(TEXT("WheelCtrlCommand"), szCommand);
	m_WheelCommandManager.GetCommandParsableName(m_WheelTiltCommand, szCommand, lengthof(szCommand));
	Settings.Write(TEXT("WheelTiltCommand"), szCommand);

	Settings.Write(TEXT("StatusBarWheel"), m_fStatusBarWheel);
	Settings.Write(TEXT("ReverseWheelChannel"), m_fWheelChannelReverse);
	Settings.Write(TEXT("ReverseWheelVolume"), m_fWheelVolumeReverse);
	Settings.Write(TEXT("WheelChannelDelay"), m_WheelChannelDelay);
	Settings.Write(TEXT("WheelZoomStep"), m_WheelZoomStep);
	if (m_pCommandManager != nullptr) {
		Settings.Write(
			TEXT("LeftDoubleClickCommand"),
			m_pCommandManager->GetCommandIDText(m_LeftDoubleClickCommand));
		Settings.Write(
			TEXT("RightClickCommand"),
			m_pCommandManager->GetCommandIDText(m_RightClickCommand));
		Settings.Write(
			TEXT("MiddleClickCommand"),
			m_pCommandManager->GetCommandIDText(m_MiddleClickCommand));
	}
	return true;
}


bool COperationOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(
		hwndOwner,
		GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_OPTIONS_OPERATION));
}


bool COperationOptions::Initialize(CSettings &Settings, const CCommandManager *pCommandManager)
{
	m_pCommandManager = pCommandManager;
	LoadSettings(Settings);
	return true;
}


bool COperationOptions::IsWheelCommandReverse(int Command) const
{
	switch (Command) {
	case CM_WHEEL_VOLUME:
		return m_fWheelVolumeReverse;
	case CM_WHEEL_CHANNEL:
		return m_fWheelChannelReverse;
	}
	return false;
}


INT_PTR COperationOptions::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			DlgCheckBox_Check(hDlg, IDC_OPTIONS_DISPLAYDRAGMOVE, m_fDisplayDragMove);

			InitWheelSettings(IDC_OPTIONS_WHEELMODE, m_WheelCommand);
			InitWheelSettings(IDC_OPTIONS_WHEELSHIFTMODE, m_WheelShiftCommand);
			InitWheelSettings(IDC_OPTIONS_WHEELCTRLMODE, m_WheelCtrlCommand);
			InitWheelSettings(IDC_OPTIONS_WHEELTILTMODE, m_WheelTiltCommand);

			DlgCheckBox_Check(hDlg, IDC_OPTIONS_WHEELVOLUMEREVERSE, m_fWheelVolumeReverse);
			DlgCheckBox_Check(hDlg, IDC_OPTIONS_WHEELCHANNELREVERSE, m_fWheelChannelReverse);
			DlgEdit_SetUInt(hDlg, IDC_OPTIONS_WHEELCHANNELDELAY, m_WheelChannelDelay);
			DlgCheckBox_Check(hDlg, IDC_OPTIONS_STATUSBARWHEEL, m_fStatusBarWheel);

			int LeftDoubleClick = 0, RightClick = 0, MiddleClick = 0;
			for (int i = IDC_OPTIONS_MOUSECOMMAND_FIRST; i <= IDC_OPTIONS_MOUSECOMMAND_LAST; i++) {
				DlgComboBox_AddString(hDlg, i, TEXT("なし"));
				DlgComboBox_SetItemData(hDlg, i, 0, 0);
			}
			CCommandManager::CCommandLister CommandLister(*m_pCommandManager);
			int Command;
			for (int i = 0; (Command = CommandLister.Next()) != 0; i++) {
				TCHAR szText[CCommandManager::MAX_COMMAND_TEXT];

				m_pCommandManager->GetCommandText(Command, szText, lengthof(szText));
				for (int j = IDC_OPTIONS_MOUSECOMMAND_FIRST; j <= IDC_OPTIONS_MOUSECOMMAND_LAST; j++) {
					const int Index = static_cast<int>(DlgComboBox_AddString(hDlg, j, szText));
					DlgComboBox_SetItemData(hDlg, j, Index, Command);
				}
				if (Command == m_LeftDoubleClickCommand)
					LeftDoubleClick = i + 1;
				if (Command == m_RightClickCommand)
					RightClick = i + 1;
				if (Command == m_MiddleClickCommand)
					MiddleClick = i + 1;
			}
			DlgComboBox_SetCurSel(hDlg, IDC_OPTIONS_LEFTDOUBLECLICKCOMMAND, LeftDoubleClick);
			DlgComboBox_SetCurSel(hDlg, IDC_OPTIONS_RIGHTCLICKCOMMAND, RightClick);
			DlgComboBox_SetCurSel(hDlg, IDC_OPTIONS_MIDDLECLICKCOMMAND, MiddleClick);

			DlgEdit_SetInt(hDlg, IDC_OPTIONS_VOLUMESTEP, m_VolumeStep);
			DlgUpDown_SetRange(hDlg, IDC_OPTIONS_VOLUMESTEP_UD, 1, 100);

			DlgEdit_SetInt(hDlg, IDC_OPTIONS_AUDIODELAYSTEP, m_AudioDelayStep);
			DlgUpDown_SetRange(hDlg, IDC_OPTIONS_AUDIODELAYSTEP_UD, 1, 1000);

			static const LPCTSTR ChannelUpDownOrderList[] = {
				TEXT("リストの並び順"),
				TEXT("チャンネル番号順"),
			};
			for (LPCTSTR pszText : ChannelUpDownOrderList) {
				DlgComboBox_AddString(hDlg, IDC_OPTIONS_CHANNELUPDOWNORDER, pszText);
			}
			DlgComboBox_SetCurSel(hDlg, IDC_OPTIONS_CHANNELUPDOWNORDER, static_cast<int>(m_ChannelUpDownOrder));

			DlgCheckBox_Check(hDlg, IDC_OPTIONS_SKIPSUBCHANNEL, m_fChannelUpDownSkipSubChannel);
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case PSN_APPLY:
			{
				m_fDisplayDragMove =
					DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_DISPLAYDRAGMOVE);

				m_WheelCommand =
					static_cast<int>(DlgComboBox_GetItemData(
						hDlg, IDC_OPTIONS_WHEELMODE,
						DlgComboBox_GetCurSel(hDlg, IDC_OPTIONS_WHEELMODE)));
				m_WheelShiftCommand =
					static_cast<int>(DlgComboBox_GetItemData(
						hDlg, IDC_OPTIONS_WHEELSHIFTMODE,
						DlgComboBox_GetCurSel(hDlg, IDC_OPTIONS_WHEELSHIFTMODE)));
				m_WheelCtrlCommand =
					static_cast<int>(DlgComboBox_GetItemData(
						hDlg, IDC_OPTIONS_WHEELCTRLMODE,
						DlgComboBox_GetCurSel(hDlg, IDC_OPTIONS_WHEELCTRLMODE)));
				m_WheelTiltCommand =
					static_cast<int>(DlgComboBox_GetItemData(
						hDlg, IDC_OPTIONS_WHEELTILTMODE,
						DlgComboBox_GetCurSel(hDlg, IDC_OPTIONS_WHEELTILTMODE)));

				m_fWheelVolumeReverse = DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_WHEELVOLUMEREVERSE);
				m_fWheelChannelReverse = DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_WHEELCHANNELREVERSE);
				m_WheelChannelDelay = DlgEdit_GetUInt(hDlg, IDC_OPTIONS_WHEELCHANNELDELAY);
				if (m_WheelChannelDelay < WHEEL_CHANNEL_DELAY_MIN)
					m_WheelChannelDelay = WHEEL_CHANNEL_DELAY_MIN;
				m_fStatusBarWheel = DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_STATUSBARWHEEL);

				m_LeftDoubleClickCommand =
					static_cast<int>(DlgComboBox_GetItemData(
						hDlg, IDC_OPTIONS_LEFTDOUBLECLICKCOMMAND,
						DlgComboBox_GetCurSel(hDlg, IDC_OPTIONS_LEFTDOUBLECLICKCOMMAND)));
				m_RightClickCommand =
					static_cast<int>(DlgComboBox_GetItemData(
						hDlg, IDC_OPTIONS_RIGHTCLICKCOMMAND,
						DlgComboBox_GetCurSel(hDlg, IDC_OPTIONS_RIGHTCLICKCOMMAND)));
				m_MiddleClickCommand =
					static_cast<int>(DlgComboBox_GetItemData(
						hDlg, IDC_OPTIONS_MIDDLECLICKCOMMAND,
						DlgComboBox_GetCurSel(hDlg, IDC_OPTIONS_MIDDLECLICKCOMMAND)));

				m_VolumeStep = DlgEdit_GetInt(hDlg, IDC_OPTIONS_VOLUMESTEP);
				m_AudioDelayStep = DlgEdit_GetInt(hDlg, IDC_OPTIONS_AUDIODELAYSTEP);

				m_ChannelUpDownOrder =
					static_cast<CChannelManager::UpDownOrder>(
						DlgComboBox_GetCurSel(hDlg, IDC_OPTIONS_CHANNELUPDOWNORDER));
				m_fChannelUpDownSkipSubChannel =
					DlgCheckBox_IsChecked(hDlg, IDC_OPTIONS_SKIPSUBCHANNEL);

				m_fChanged = true;
			}
			break;
		}
		break;
	}

	return FALSE;
}


void COperationOptions::InitWheelSettings(int ID, int CurCommand) const
{
	const int CommandCount = m_WheelCommandManager.GetCommandCount();
	int Sel = 0;

	DlgComboBox_AddString(m_hDlg, ID, TEXT("何もしない"));
	DlgComboBox_SetItemData(m_hDlg, ID, 0, 0);

	for (int i = 0; i < CommandCount; i++) {
		const int Command = m_WheelCommandManager.GetCommandID(i);
		TCHAR szText[CWheelCommandManager::MAX_COMMAND_TEXT];
		m_WheelCommandManager.GetCommandText(Command, szText, lengthof(szText));
		const LRESULT Index = DlgComboBox_AddString(m_hDlg, ID, szText);
		DlgComboBox_SetItemData(m_hDlg, ID, Index, Command);
		if (Command == CurCommand)
			Sel = i + 1;
	}

	DlgComboBox_SetCurSel(m_hDlg, ID, Sel);
}


} // namespace TVTest
