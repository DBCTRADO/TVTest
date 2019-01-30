/*
  TVTest
  Copyright(c) 2008-2019 DBCTRADO

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


#ifndef TVTEST_OPERATION_OPTIONS_H
#define TVTEST_OPERATION_OPTIONS_H


#include "Options.h"
#include "Command.h"
#include "WheelCommand.h"
#include "ChannelManager.h"


namespace TVTest
{

	class COperationOptions
		: public COptions
	{
	public:
		static constexpr int WHEEL_CHANNEL_DELAY_MIN = 100;

		COperationOptions();
		~COperationOptions();

	// CSettingsBase
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	// CBasicDialog
		bool Create(HWND hwndOwner) override;

	// COperationOptions
		bool Initialize(CSettings &Settings, const CCommandManager *pCommandManager);
		bool GetDisplayDragMove() const { return m_fDisplayDragMove; }
		int GetVolumeStep() const { return m_VolumeStep; }
		int GetAudioDelayStep() const { return m_AudioDelayStep; }
		CChannelManager::UpDownOrder GetChannelUpDownOrder() const { return m_ChannelUpDownOrder; }
		bool GetChannelUpDownSkipSubChannel() const { return m_fChannelUpDownSkipSubChannel; }
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
		const CCommandManager *m_pCommandManager;
		CWheelCommandManager m_WheelCommandManager;

		bool m_fDisplayDragMove;
		int m_VolumeStep;
		int m_AudioDelayStep;
		CChannelManager::UpDownOrder m_ChannelUpDownOrder;
		bool m_fChannelUpDownSkipSubChannel;
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
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		void InitWheelSettings(int ID, int CurCommand) const;
	};

}	// namespace TVTest


#endif
