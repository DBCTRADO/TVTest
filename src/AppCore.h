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


#ifndef TVTEST_APP_CORE_H
#define TVTEST_APP_CORE_H


#include "CoreEngine.h"
#include "ChannelManager.h"
#include "Record.h"
#include "Settings.h"
#include "CommandLine.h"
#include "TSProcessorManager.h"


namespace TVTest
{

	class CAppMain;
	class CCommandManager;
	class CDriverManager;
	class CLogoManager;
	class CControllerManager;
	class CRecentChannelList;
	class CFavoritesManager;

	class CAppCore
	{
	public:
		struct StreamIDInfo {
			WORD NetworkID;
			WORD TransportStreamID;
			WORD ServiceID;
		};

		enum class OpenTunerFlag : unsigned int {
			None        = 0x0000U,
			NoUI        = 0x0001U,
			NoNotify    = 0x0002U,
			RetryDialog = 0x0004U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		enum class SelectChannelFlag : unsigned int {
			None            = 0x0000U,
			UseCurrentTuner = 0x0001U,
			StrictService   = 0x0002U,
			AllowDisabled   = 0x0004U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		enum class SetServiceFlag : unsigned int {
			None                     = 0x0000U,
			StrictID                 = 0x0001U,
			NoChangeCurrentServiceID = 0x0002U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		CAppCore(CAppMain &App);

		CAppCore(const CAppCore &) = delete;
		CAppCore &operator=(const CAppCore &) = delete;

		template<typename... TArgs> void OnError(StringView Format, const TArgs&... Args)
		{
			return OnErrorV(Format, MakeFormatArgs(Args...));
		}
		void OnErrorV(StringView Format, FormatArgs Args);
		void OnError(const LibISDB::ErrorHandler *pErrorHandler, LPCTSTR pszTitle = nullptr);
		void SetSilent(bool fSilent);
		bool IsSilent() const { return m_fSilent; }
		bool SaveCurrentChannel();

		bool InitializeChannel();
		bool GetChannelFileName(LPCTSTR pszDriverFileName, String *pChannelFileName);
		bool RestoreChannel();
		bool UpdateCurrentChannelList(const CTuningSpaceList *pList);
		bool UpdateChannelList(LPCTSTR pszBonDriverName, const CTuningSpaceList *pList);
		const CChannelInfo *GetCurrentChannelInfo() const;
		bool SetChannel(int Space, int Channel, int ServiceID = -1, bool fStrictService = false);
		bool SetChannelByIndex(int Space, int Channel, int ServiceID = -1);
		bool SelectChannel(
			LPCTSTR pszTunerName, const CChannelInfo &ChannelInfo,
			SelectChannelFlag Flags = SelectChannelFlag::None);
		bool SwitchChannel(int Channel);
		bool SwitchChannelByNo(int ChannelNo, bool fSwitchService);
		bool SetCommandLineChannel(const CCommandLineOptions *pCmdLine);
		bool FollowChannelChange(WORD TransportStreamID, WORD ServiceID);
		bool SetServiceByID(WORD ServiceID, SetServiceFlag Flags = SetServiceFlag::None);
		bool SetServiceByIndex(int Service, SetServiceFlag Flags = SetServiceFlag::None);
		bool GetCurrentStreamIDInfo(StreamIDInfo *pInfo) const;
		bool GetCurrentStreamChannelInfo(CChannelInfo *pInfo, bool fName = false) const;
		bool GetCurrentServiceName(LPTSTR pszName, int MaxLength, bool fUseChannelName = true) const;

		bool OpenTuner(LPCTSTR pszFileName);
		bool OpenTuner();
		bool OpenAndInitializeTuner(OpenTunerFlag OpenFlags = OpenTunerFlag::None);
		bool CloseTuner();
		void ShutDownTuner();
		void ResetEngine();

		bool Set1SegMode(bool f1Seg, bool fServiceChange);
		bool Is1SegMode() const { return m_f1SegMode; }

		void ApplyBonDriverOptions();

		bool StartRecord(
			LPCTSTR pszFileName = nullptr,
			const CRecordManager::TimeSpecInfo *pStartTime = nullptr,
			const CRecordManager::TimeSpecInfo *pStopTime = nullptr,
			CRecordManager::RecordClient Client = CRecordManager::RecordClient::User,
			bool fTimeShift = false);
		bool ModifyRecord(
			LPCTSTR pszFileName = nullptr,
			const CRecordManager::TimeSpecInfo *pStartTime = nullptr,
			const CRecordManager::TimeSpecInfo *pStopTime = nullptr,
			CRecordManager::RecordClient Client = CRecordManager::RecordClient::User);
		bool StartReservedRecord();
		bool CancelReservedRecord();
		bool StopRecord();
		bool PauseResumeRecording();
		bool RelayRecord(LPCTSTR pszFileName);
		bool CommandLineRecord(const CCommandLineOptions *pCmdLine);
		bool CommandLineRecord(LPCTSTR pszFileName, const SYSTEMTIME *pStartTime, int Delay, int Duration);
		LPCTSTR GetDefaultRecordFolder() const;
		bool GetExitOnRecordingStop() const { return m_fExitOnRecordingStop; }
		void SetExitOnRecordingStop(bool fExit) { m_fExitOnRecordingStop = fExit; }

		void BeginChannelScan(int Space);
		bool IsChannelScanning() const;
		bool IsDriverNoSignalLevel(LPCTSTR pszFileName) const;

		void NotifyTSProcessorNetworkChanged(CTSProcessorManager::FilterOpenFlag FilterOpenFlags);

		bool GetVariableStringEventInfo(
			CEventVariableStringMap::EventInfo *pInfo,
			DWORD NextEventMargin = 0) const;

	private:
		CAppMain &m_App;
		bool m_fSilent = false;
		bool m_fExitOnRecordingStop = false;
		bool m_f1SegMode = false;

		int GetCorresponding1SegService(int Space, WORD NetworkID, WORD TSID, WORD ServiceID) const;
		bool GenerateRecordFileName(LPTSTR pszFileName, int MaxFileName);
	};

}


#endif
