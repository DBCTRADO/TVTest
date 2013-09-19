#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "OperationOptions.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




COperationOptions::COperationOptions()
	: m_pCommandList(NULL)
	, m_fDisplayDragMove(true)
	, m_VolumeStep(5)
	, m_WheelMode(WHEEL_MODE_VOLUME)
	, m_WheelShiftMode(WHEEL_MODE_CHANNEL)
	, m_WheelCtrlMode(WHEEL_MODE_AUDIO)
	, m_WheelTiltMode(WHEEL_MODE_NONE)
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
	Settings.Read(TEXT("VolumeStep"),&m_VolumeStep);

	if (Settings.Read(TEXT("WheelMode"),&Value)
			&& Value>=WHEEL_MODE_FIRST && Value<=WHEEL_MODE_LAST)
		m_WheelMode=(WheelMode)Value;
	if (Settings.Read(TEXT("WheelShiftMode"),&Value)
			&& Value>=WHEEL_MODE_FIRST && Value<=WHEEL_MODE_LAST)
		m_WheelShiftMode=(WheelMode)Value;
	if (Settings.Read(TEXT("WheelCtrlMode"),&Value)
			&& Value>=WHEEL_MODE_FIRST && Value<=WHEEL_MODE_LAST)
		m_WheelCtrlMode=(WheelMode)Value;
	if (Settings.Read(TEXT("WheelTiltMode"),&Value)
			&& Value>=WHEEL_MODE_FIRST && Value<=WHEEL_MODE_LAST)
		m_WheelTiltMode=(WheelMode)Value;

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
	Settings.Write(TEXT("VolumeStep"),m_VolumeStep);
	Settings.Write(TEXT("WheelMode"),(int)m_WheelMode);
	Settings.Write(TEXT("WheelShiftMode"),(int)m_WheelShiftMode);
	Settings.Write(TEXT("WheelCtrlMode"),(int)m_WheelCtrlMode);
	Settings.Write(TEXT("WheelTiltMode"),(int)m_WheelTiltMode);
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


bool COperationOptions::IsWheelModeReverse(WheelMode Mode) const
{
	switch (Mode) {
	case WHEEL_MODE_VOLUME:
		return m_fWheelVolumeReverse;
	case WHEEL_MODE_CHANNEL:
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

			InitWheelSettings(IDC_OPTIONS_WHEELMODE,m_WheelMode);
			InitWheelSettings(IDC_OPTIONS_WHEELSHIFTMODE,m_WheelShiftMode);
			InitWheelSettings(IDC_OPTIONS_WHEELCTRLMODE,m_WheelCtrlMode);
			InitWheelSettings(IDC_OPTIONS_WHEELTILTMODE,m_WheelTiltMode);

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
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				m_fDisplayDragMove=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_DISPLAYDRAGMOVE);

				m_WheelMode=(WheelMode)DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_WHEELMODE);
				m_WheelShiftMode=(WheelMode)DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_WHEELSHIFTMODE);
				m_WheelCtrlMode=(WheelMode)DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_WHEELCTRLMODE);
				m_WheelTiltMode=(WheelMode)DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_WHEELTILTMODE);

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

				m_fChanged=true;
			}
			break;
		}
		break;
	}

	return FALSE;
}


void COperationOptions::InitWheelSettings(int ID,WheelMode Mode) const
{
	static const LPCTSTR pszWheelMode[] = {
		TEXT("‚È‚µ"),
		TEXT("‰¹—Ê"),
		TEXT("ƒ`ƒƒƒ“ƒlƒ‹"),
		TEXT("‰¹º"),
		TEXT("•\Ž¦”{—¦"),
		TEXT("”ä—¦"),
	};

	for (int i=0;i<lengthof(pszWheelMode);i++)
		DlgComboBox_AddString(m_hDlg,ID,pszWheelMode[i]);
	DlgComboBox_SetCurSel(m_hDlg,ID,Mode);
}
