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


#ifndef TVTEST_RECORD_OPTIONS_H
#define TVTEST_RECORD_OPTIONS_H


#include "Options.h"
#include "Record.h"


namespace TVTest
{

	class CRecordOptions
		: public COptions
	{
		CFilePath m_SaveFolder;
		CFilePath m_FileName{TEXT("Record_%date%-%time%.ts")};
		bool m_fConfirmChannelChange = true;
		bool m_fConfirmExit = true;
		bool m_fConfirmStop = false;
		bool m_fConfirmStopStatusBarOnly = false;
		bool m_fAlertLowFreeSpace = true;
		unsigned int m_LowFreeSpaceThreshold = 2048;
		bool m_fShowRemainTime = false;
		int m_StatusBarRecordCommand;
		CRecordingSettings m_Settings;

		std::vector<String> m_WritePluginList;

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	public:
		enum {
			UPDATE_RECORDINGSETTINGS = 0x00000001UL,
		};

		CRecordOptions();
		~CRecordOptions();

	// COptions
		bool Apply(DWORD Flags) override;

	// CSettingsBase
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	// CBasicDialog
		bool Create(HWND hwndOwner) override;

	// CRecordOptions
		bool SetSaveFolder(LPCTSTR pszFolder);
		LPCTSTR GetSaveFolder() const { return m_SaveFolder.c_str(); }
		LPCTSTR GetFileName() const { return m_FileName.c_str(); }
		bool GetFilePath(LPTSTR pszFileName, int MaxLength) const;
		bool GenerateFilePath(CFilePath *pFilePath, LPCTSTR *ppszErrorMessage = nullptr) const;
		bool ConfirmChannelChange(HWND hwndOwner) const;
		bool ConfirmServiceChange(HWND hwndOwner, const CRecordManager *pRecordManager) const;
		bool ConfirmStop(HWND hwndOwner) const;
		bool ConfirmStatusBarStop(HWND hwndOwner) const;
		bool ConfirmExit(HWND hwndOwner, const CRecordManager *pRecordManager) const;
		const CRecordingSettings &GetRecordingSettings() const { return m_Settings; }
		bool GetAlertLowFreeSpace() const { return m_fAlertLowFreeSpace; }
		ULONGLONG GetLowFreeSpaceThresholdBytes() const
		{
			return static_cast<ULONGLONG>(m_LowFreeSpaceThreshold) * (1024 * 1024);
		}
		bool IsTimeShiftRecordingEnabled() const { return m_Settings.m_fEnableTimeShift; }
		bool EnableTimeShiftRecording(bool fEnable);
		void SetShowRemainTime(bool fShow) { m_fShowRemainTime = fShow; }
		bool GetShowRemainTime() const { return m_fShowRemainTime; }
		int GetStatusBarRecordCommand() const { return m_StatusBarRecordCommand; }
	};

}	// namespace TVTest


#endif
