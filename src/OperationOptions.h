#ifndef OPERATION_OPTIONS_H
#define OPERATION_OPTIONS_H


#include "Options.h"
#include "Command.h"


class COperationOptions : public COptions
{
public:
	enum WheelMode {
		WHEEL_MODE_NONE,
		WHEEL_MODE_VOLUME,
		WHEEL_MODE_CHANNEL,
		WHEEL_MODE_AUDIO,
		WHEEL_MODE_ZOOM,
		WHEEL_MODE_ASPECTRATIO,
		WHEEL_MODE_FIRST=WHEEL_MODE_NONE,
		WHEEL_MODE_LAST=WHEEL_MODE_ASPECTRATIO
	};

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
	int GetVolumeStep() const { return m_VolumeStep; }
	WheelMode GetWheelMode() const { return m_WheelMode; }
	WheelMode GetWheelShiftMode() const { return m_WheelShiftMode; }
	WheelMode GetWheelCtrlMode() const { return m_WheelCtrlMode; }
	WheelMode GetWheelTiltMode() const { return m_WheelTiltMode; }
	bool IsStatusBarWheelEnabled() const { return m_fStatusBarWheel; }
	bool IsWheelModeReverse(WheelMode Mode) const;
	int GetWheelChannelDelay() const { return m_WheelChannelDelay; }
	int GetWheelZoomStep() const { return m_WheelZoomStep; }
	int GetLeftDoubleClickCommand() const { return m_LeftDoubleClickCommand; }
	int GetRightClickCommand() const { return m_RightClickCommand; }
	int GetMiddleClickCommand() const { return m_MiddleClickCommand; }

private:
	const CCommandList *m_pCommandList;
	bool m_fDisplayDragMove;
	int m_VolumeStep;
	WheelMode m_WheelMode;
	WheelMode m_WheelShiftMode;
	WheelMode m_WheelCtrlMode;
	WheelMode m_WheelTiltMode;
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

	void InitWheelSettings(int ID,WheelMode Mode) const;
};


#endif
