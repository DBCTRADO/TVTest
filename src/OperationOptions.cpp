#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "OperationOptions.h"
#include "DialogUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"




COperationOptions::COperationOptions()
	: m_pCommandList(NULL)
	, m_fDisplayDragMove(true)
	, m_fHideCursor(false)
	, m_VolumeStep(5)
	, m_AudioDelayStep(50)
	, m_WheelCommand(CM_WHEEL_VOLUME)
	, m_WheelShiftCommand(CM_WHEEL_CHANNEL)
	, m_WheelCtrlCommand(CM_WHEEL_AUDIO)
	, m_WheelTiltCommand(0)
	, m_fStatusBarWheel(true)
	, m_fWheelVolumeReverse(false)
	, m_fWheelChannelReverse(false)
	, m_WheelChannelDelay(1000)
	, m_WheelZoomStep(5)
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

	Settings.Read(TEXT("DisplayDragMove"),&m_fDisplayDragMove);
	Settings.Read(TEXT("HideCursor"),&m_fHideCursor);
	Settings.Read(TEXT("VolumeStep"),&m_VolumeStep);
	Settings.Read(TEXT("AudioDelayStep"),&m_AudioDelayStep);

	// ver.0.9.0 ‚æ‚è‘O‚Æ‚ÌŒÝŠ·—p
	static const int WheelModeList[] = {
		0,
		CM_WHEEL_VOLUME,
		CM_WHEEL_CHANNEL,
		CM_WHEEL_AUDIO,
		CM_WHEEL_ZOOM,
		CM_WHEEL_ASPECTRATIO,
	};
	TVTest::String Command;
	if (Settings.Read(TEXT("WheelCommand"),&Command)) {
		m_WheelCommand=m_WheelCommandManager.ParseCommand(Command.c_str());
	} else if (Settings.Read(TEXT("WheelMode"),&Value)
			&& Value>=0 && Value<lengthof(WheelModeList)) {
		m_WheelCommand=WheelModeList[Value];
	}
	if (Settings.Read(TEXT("WheelShiftCommand"),&Command)) {
		m_WheelShiftCommand=m_WheelCommandManager.ParseCommand(Command.c_str());
	} else if (Settings.Read(TEXT("WheelShiftMode"),&Value)
			&& Value>=0 && Value<lengthof(WheelModeList)) {
		m_WheelShiftCommand=WheelModeList[Value];
	}
	if (Settings.Read(TEXT("WheelCtrlCommand"),&Command)) {
		m_WheelCtrlCommand=m_WheelCommandManager.ParseCommand(Command.c_str());
	} else if (Settings.Read(TEXT("WheelCtrlMode"),&Value)
			&& Value>=0 && Value<lengthof(WheelModeList)) {
		m_WheelCtrlCommand=WheelModeList[Value];
	}
	if (Settings.Read(TEXT("WheelTiltCommand"),&Command)) {
		m_WheelTiltCommand=m_WheelCommandManager.ParseCommand(Command.c_str());
	} else if (Settings.Read(TEXT("WheelTiltMode"),&Value)
			&& Value>=0 && Value<lengthof(WheelModeList)) {
		m_WheelTiltCommand=WheelModeList[Value];
	}

	Settings.Read(TEXT("StatusBarWheel"),&m_fStatusBarWheel);
	Settings.Read(TEXT("ReverseWheelChannel"),&m_fWheelChannelReverse);
	Settings.Read(TEXT("ReverseWheelVolume"),&m_fWheelVolumeReverse);
	if (Settings.Read(TEXT("WheelChannelDelay"),&Value)) {
		if (Value<WHEEL_CHANNEL_DELAY_MIN)
			Value=WHEEL_CHANNEL_DELAY_MIN;
		m_WheelChannelDelay=Value;
	}
	Settings.Read(TEXT("WheelZoomStep"),&m_WheelZoomStep);

	if (m_pCommandList!=NULL) {
		TCHAR szText[CCommandList::MAX_COMMAND_TEXT];

		if (Settings.Read(TEXT("LeftDoubleClickCommand"),szText,lengthof(szText))) {
			m_LeftDoubleClickCommand=m_pCommandList->ParseText(szText);
		}
		if (Settings.Read(TEXT("RightClickCommand"),szText,lengthof(szText))) {
			m_RightClickCommand=m_pCommandList->ParseText(szText);
		}
		if (Settings.Read(TEXT("MiddleClickCommand"),szText,lengthof(szText))) {
			m_MiddleClickCommand=m_pCommandList->ParseText(szText);
		}
	}

	return true;
}


static LPCTSTR GetCommandText(const CCommandList *pCommandList,int Command)
{
	if (Command==0)
		return TEXT("");
	return pCommandList->GetCommandTextByID(Command);
}

bool COperationOptions::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("DisplayDragMove"),m_fDisplayDragMove);
	Settings.Write(TEXT("HideCursor"),m_fHideCursor);
	Settings.Write(TEXT("VolumeStep"),m_VolumeStep);
	Settings.Write(TEXT("AudioDelayStep"),m_AudioDelayStep);

	TCHAR szCommand[TVTest::CWheelCommandManager::MAX_COMMAND_PARSABLE_NAME];
	m_WheelCommandManager.GetCommandParsableName(m_WheelCommand,szCommand,lengthof(szCommand));
	Settings.Write(TEXT("WheelCommand"),szCommand);
	m_WheelCommandManager.GetCommandParsableName(m_WheelShiftCommand,szCommand,lengthof(szCommand));
	Settings.Write(TEXT("WheelShiftCommand"),szCommand);
	m_WheelCommandManager.GetCommandParsableName(m_WheelCtrlCommand,szCommand,lengthof(szCommand));
	Settings.Write(TEXT("WheelCtrlCommand"),szCommand);
	m_WheelCommandManager.GetCommandParsableName(m_WheelTiltCommand,szCommand,lengthof(szCommand));
	Settings.Write(TEXT("WheelTiltCommand"),szCommand);

	Settings.Write(TEXT("StatusBarWheel"),m_fStatusBarWheel);
	Settings.Write(TEXT("ReverseWheelChannel"),m_fWheelChannelReverse);
	Settings.Write(TEXT("ReverseWheelVolume"),m_fWheelVolumeReverse);
	Settings.Write(TEXT("WheelChannelDelay"),m_WheelChannelDelay);
	Settings.Write(TEXT("WheelZoomStep"),m_WheelZoomStep);
	if (m_pCommandList!=NULL) {
		Settings.Write(TEXT("LeftDoubleClickCommand"),
			GetCommandText(m_pCommandList,m_LeftDoubleClickCommand));
		Settings.Write(TEXT("RightClickCommand"),
			GetCommandText(m_pCommandList,m_RightClickCommand));
		Settings.Write(TEXT("MiddleClickCommand"),
			GetCommandText(m_pCommandList,m_MiddleClickCommand));
	}
	return true;
}


bool COperationOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_OPERATION));
}


bool COperationOptions::Initialize(CSettings &Settings,const CCommandList *pCommandList)
{
	m_pCommandList=pCommandList;
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


INT_PTR COperationOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_DISPLAYDRAGMOVE,m_fDisplayDragMove);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_HIDECURSOR,m_fHideCursor);

			InitWheelSettings(IDC_OPTIONS_WHEELMODE,m_WheelCommand);
			InitWheelSettings(IDC_OPTIONS_WHEELSHIFTMODE,m_WheelShiftCommand);
			InitWheelSettings(IDC_OPTIONS_WHEELCTRLMODE,m_WheelCtrlCommand);
			InitWheelSettings(IDC_OPTIONS_WHEELTILTMODE,m_WheelTiltCommand);

			DlgCheckBox_Check(hDlg,IDC_OPTIONS_WHEELVOLUMEREVERSE,m_fWheelVolumeReverse);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_WHEELCHANNELREVERSE,m_fWheelChannelReverse);
			DlgEdit_SetUInt(hDlg,IDC_OPTIONS_WHEELCHANNELDELAY,m_WheelChannelDelay);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_STATUSBARWHEEL,m_fStatusBarWheel);

			int LeftDoubleClick=0,RightClick=0,MiddleClick=0;
			for (int i=IDC_OPTIONS_MOUSECOMMAND_FIRST;i<=IDC_OPTIONS_MOUSECOMMAND_LAST;i++) {
				DlgComboBox_AddString(hDlg,i,TEXT("‚È‚µ"));
				DlgComboBox_SetItemData(hDlg,i,0,0);
			}
			int NumCommands=m_pCommandList->NumCommands();
			for (int i=0;i<NumCommands;i++) {
				TCHAR szText[CCommandList::MAX_COMMAND_NAME];
				int Command=m_pCommandList->GetCommandID(i);

				m_pCommandList->GetCommandName(i,szText,lengthof(szText));
				for (int j=IDC_OPTIONS_MOUSECOMMAND_FIRST;j<=IDC_OPTIONS_MOUSECOMMAND_LAST;j++) {
					int Index=(int)DlgComboBox_AddString(hDlg,j,szText);
					DlgComboBox_SetItemData(hDlg,j,Index,Command);
				}
				if (Command==m_LeftDoubleClickCommand)
					LeftDoubleClick=i+1;
				if (Command==m_RightClickCommand)
					RightClick=i+1;
				if (Command==m_MiddleClickCommand)
					MiddleClick=i+1;
			}
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_LEFTDOUBLECLICKCOMMAND,LeftDoubleClick);
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_RIGHTCLICKCOMMAND,RightClick);
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_MIDDLECLICKCOMMAND,MiddleClick);

			DlgEdit_SetInt(hDlg,IDC_OPTIONS_VOLUMESTEP,m_VolumeStep);
			DlgUpDown_SetRange(hDlg,IDC_OPTIONS_VOLUMESTEP_UD,1,100);

			DlgEdit_SetInt(hDlg,IDC_OPTIONS_AUDIODELAYSTEP,m_AudioDelayStep);
			DlgUpDown_SetRange(hDlg,IDC_OPTIONS_AUDIODELAYSTEP_UD,1,1000);
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				m_fDisplayDragMove=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_DISPLAYDRAGMOVE);
				m_fHideCursor=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_HIDECURSOR);

				m_WheelCommand=
					(int)DlgComboBox_GetItemData(hDlg,IDC_OPTIONS_WHEELMODE,
						DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_WHEELMODE));
				m_WheelShiftCommand=
					(int)DlgComboBox_GetItemData(hDlg,IDC_OPTIONS_WHEELSHIFTMODE,
						DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_WHEELSHIFTMODE));
				m_WheelCtrlCommand=
					(int)DlgComboBox_GetItemData(hDlg,IDC_OPTIONS_WHEELCTRLMODE,
						DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_WHEELCTRLMODE));
				m_WheelTiltCommand=
					(int)DlgComboBox_GetItemData(hDlg,IDC_OPTIONS_WHEELTILTMODE,
						DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_WHEELTILTMODE));

				m_fWheelVolumeReverse=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_WHEELVOLUMEREVERSE);
				m_fWheelChannelReverse=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_WHEELCHANNELREVERSE);
				m_WheelChannelDelay=DlgEdit_GetUInt(hDlg,IDC_OPTIONS_WHEELCHANNELDELAY);
				if (m_WheelChannelDelay<WHEEL_CHANNEL_DELAY_MIN)
					m_WheelChannelDelay=WHEEL_CHANNEL_DELAY_MIN;
				m_fStatusBarWheel=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_STATUSBARWHEEL);

				m_LeftDoubleClickCommand=
					(int)DlgComboBox_GetItemData(hDlg,IDC_OPTIONS_LEFTDOUBLECLICKCOMMAND,
						DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_LEFTDOUBLECLICKCOMMAND));
				m_RightClickCommand=
					(int)DlgComboBox_GetItemData(hDlg,IDC_OPTIONS_RIGHTCLICKCOMMAND,
						DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_RIGHTCLICKCOMMAND));
				m_MiddleClickCommand=
					(int)DlgComboBox_GetItemData(hDlg,IDC_OPTIONS_MIDDLECLICKCOMMAND,
						DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_MIDDLECLICKCOMMAND));

				m_VolumeStep=DlgEdit_GetInt(hDlg,IDC_OPTIONS_VOLUMESTEP);
				m_AudioDelayStep=DlgEdit_GetInt(hDlg,IDC_OPTIONS_AUDIODELAYSTEP);

				m_fChanged=true;
			}
			break;
		}
		break;
	}

	return FALSE;
}


void COperationOptions::InitWheelSettings(int ID,int CurCommand) const
{
	const int CommandCount=m_WheelCommandManager.GetCommandCount();
	int Sel=0;

	DlgComboBox_AddString(m_hDlg,ID,TEXT("‰½‚à‚µ‚È‚¢"));
	DlgComboBox_SetItemData(m_hDlg,ID,0,0);

	for (int i=0;i<CommandCount;i++) {
		int Command=m_WheelCommandManager.GetCommandID(i);
		TCHAR szText[TVTest::CWheelCommandManager::MAX_COMMAND_TEXT];
		m_WheelCommandManager.GetCommandText(Command,szText,lengthof(szText));
		LRESULT Index=DlgComboBox_AddString(m_hDlg,ID,szText);
		DlgComboBox_SetItemData(m_hDlg,ID,Index,Command);
		if (Command==CurCommand)
			Sel=i+1;
	}

	DlgComboBox_SetCurSel(m_hDlg,ID,Sel);
}
