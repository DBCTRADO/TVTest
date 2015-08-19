#ifndef OPERATION_OPTIONS_H
#define OPERATION_OPTIONS_H


#include "Options.h"
#include "Command.h"
#include "WheelCommand.h"


class COperationOptions : public COptions
{
public:
	enum {
		WHEEL_CHANNEL_DELAY_MIN=100
	};

	COperationOptions();
	~COperationOptions();
// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;
// CBasicDialog
	bool Create(HWND hwndOwner) override;
// COperationOptions
	bool Initialize(CSettings &Settings,const CCommandList *pCommandList);
	bool GetDisplayDragMove() const { return m_fDisplayDragMove; }
	bool GetHideCursor() const { return m_fHideCursor; }
	int GetVolumeStep() const { return m_VolumeStep; }
	int GetAudioDelayStep() const { return m_AudioDelayStep; }
	int GetWheelCommand() const { return m_WheelCommand; }
	int GetWheelShiftCommand() const { return m_WheelShiftCommand; }
	int GetWheelCtrlCommand() const { return m_WheelCtrlCommand; }
	int GetWheelTiltCommand() const { return m_WheelTiltCommand; }
	bool IsStatusBarWheelEnabled() const { return m_fStatusBarWheel; }
	bool IsWheelCommandReverse(int Command) const;
	int GetWheelChannelDelay() const { return m_WheelChannelDelay; }
	int GetWheelZoomStep() const { return m_WheelZoomStep; }
	int GetLeftDoubleClickCommand() const { return m_LeftDoubleClickCommand; }
	int GetRightClickCommand() const { return m_RightClickCommand; }
	int GetMiddleClickCommand() const { return m_MiddleClickCommand; }

private:
	const CCommandList *m_pCommandList;
	TVTest::CWheelCommandManager m_WheelCommandManager;

	bool m_fDisplayDragMove;
	bool m_fHideCursor;
	int m_VolumeStep;
	int m_AudioDelayStep;
	int m_WheelCommand;
	int m_WheelShiftCommand;
	int m_WheelCtrlCommand;
	int m_WheelTiltCommand;
	bool m_fStatusBarWheel;
	bool m_fWheelVolumeReverse;
	bool m_fWheelChannelReverse;
	int m_WheelChannelDelay;
	int m_WheelZoomStep;
	int m_LeftDoubleClickCommand;
	int m_RightClickCommand;
	int m_MiddleClickCommand;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	void InitWheelSettings(int ID,int CurCommand) const;
};


#endif
