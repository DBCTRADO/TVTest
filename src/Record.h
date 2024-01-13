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


#ifndef TVTEST_RECORD_H
#define TVTEST_RECORD_H


#include <vector>
#include "LibISDB/LibISDB/Filters/RecorderFilter.hpp"
#include "VariableString.h"
#include "Dialog.h"


namespace TVTest
{

	class CRecordingSettings
	{
	public:
		static constexpr DWORD WRITE_CACHE_SIZE_DEFAULT = 0x100000;
		static constexpr DWORD MAX_PENDING_SIZE_DEFAULT = 0x10000000;
		static constexpr DWORD TIMESHIFT_BUFFER_SIZE_DEFAULT = 32 * 0x100000;

		bool m_fCurServiceOnly = false;
		LibISDB::StreamSelector::StreamFlag m_SaveStream = LibISDB::StreamSelector::StreamFlag::All;
		String m_WritePlugin;
		DWORD m_WriteCacheSize = WRITE_CACHE_SIZE_DEFAULT;
		DWORD m_MaxPendingSize = MAX_PENDING_SIZE_DEFAULT;
		ULONGLONG m_PreAllocationUnit = 0;
		DWORD m_TimeShiftBufferSize = TIMESHIFT_BUFFER_SIZE_DEFAULT;
		bool m_fEnableTimeShift = false;

		bool IsSaveCaption() const;
		void SetSaveCaption(bool fSave);
		bool IsSaveDataCarrousel() const;
		void SetSaveDataCarrousel(bool fSave);

	private:
		bool TestSaveStreamFlag(LibISDB::StreamSelector::StreamFlag Flag) const;
		void SetSaveStreamFlag(LibISDB::StreamSelector::StreamFlag Flag, bool fSet);
	};

	class CRecordTime
	{
		SYSTEMTIME m_Time;
		Util::TickCountType m_TickTime;

	public:
		CRecordTime();

		bool SetCurrentTime();
		bool GetTime(SYSTEMTIME *pTime) const;
		Util::TickCountType GetTickTime() const { return m_TickTime; }
		void Clear();
		bool IsValid() const;
	};

	class CRecordTask
		: public LibISDB::ErrorHandler
	{
	public:
		enum class State {
			Stop,
			Recording,
			Pause,
		};

		typedef Util::TickCountType DurationType;

	protected:
		State m_State = State::Stop;
		LibISDB::TSEngine *m_pTSEngine = nullptr;
		LibISDB::RecorderFilter *m_pRecorderFilter = nullptr;
		std::shared_ptr<LibISDB::RecorderFilter::RecordingTask> m_RecordingTask;
		CRecordTime m_StartTime;
		DurationType m_PauseStartTime;
		DurationType m_TotalPauseTime;
		LibISDB::RecorderFilter::RecordingStatistics m_Statistics;

	public:
		CRecordTask() = default;
		virtual ~CRecordTask();

		CRecordTask(const CRecordTask &) = delete;
		CRecordTask &operator=(const CRecordTask &) = delete;

		void SetRecorderFilter(
			LibISDB::TSEngine *pTSEngine,
			LibISDB::RecorderFilter *pRecorderFilter);
		bool UpdateRecordingSettings(const CRecordingSettings &Settings);
		bool Start(LPCTSTR pszFileName, const CRecordingSettings &Settings);
		bool Stop();
		bool Pause();
		State GetState() const;
		bool IsStopped() const { return m_State == State::Stop; }
		bool IsRecording() const { return m_State == State::Recording; }
		bool IsPaused() const { return m_State == State::Pause; }
		DurationType GetStartTime() const;
		bool GetStartTime(SYSTEMTIME *pTime) const;
		bool GetStartTime(CRecordTime *pTime) const;
		DurationType GetRecordTime() const;
		DurationType GetPauseTime() const;
		LONGLONG GetWroteSize() const;
		bool GetFileName(String *pFileName) const;
		bool GetStatistics(LibISDB::RecorderFilter::RecordingStatistics *pStats) const;
		bool RelayFile(LPCTSTR pszFileName);
#undef GetFreeSpace
		LONGLONG GetFreeSpace() const;
	};

	class CRecordManager
		: public LibISDB::ErrorHandler
	{
	public:
		enum class TimeSpecType {
			NotSpecified,
			DateTime,
			Duration,
		};

		struct TimeSpecInfo
		{
			TimeSpecType Type = TimeSpecType::NotSpecified;
			union {
				SYSTEMTIME DateTime;
				ULONGLONG Duration;
			} Time;
		};

		enum class RecordClient {
			User,
			CommandLine,
			Plugin,
		};

		typedef CEventVariableStringMap::EventInfo FileNameFormatInfo;

	private:
		class CRecordSettingsDialog
			: public CBasicDialog
		{
		public:
			CRecordSettingsDialog(CRecordManager *pRecManager, CRecordingSettings *pSettings);

			bool Show(HWND hwndOwner) override;

		private:
			CRecordManager *m_pRecManager;
			CRecordingSettings *m_pSettings;

			INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		};

		bool m_fRecording = false;
		bool m_fReserved = false;
		String m_FileName;
		CRecordTime m_ReserveTime;
		TimeSpecInfo m_StartTimeSpec;
		TimeSpecInfo m_StopTimeSpec;
		bool m_fStopOnEventEnd = false;
		RecordClient m_Client = RecordClient::User;
		CRecordTask m_RecordTask;
		LibISDB::TSEngine *m_pTSEngine = nullptr;
		LibISDB::RecorderFilter *m_pRecorderFilter = nullptr;
		CRecordingSettings m_Settings;
		CRecordingSettings m_ReserveSettings;

		std::vector<String> m_WritePluginList;

	public:
		CRecordManager() = default;
		~CRecordManager();

		CRecordManager(const CRecordManager &) = delete;
		CRecordManager &operator=(const CRecordManager &) = delete;

		void Terminate();
		bool SetFileName(LPCTSTR pszFileName);
		LPCTSTR GetFileName() const { return StringUtility::GetCStrOrNull(m_FileName); }
		bool GetStartTime(SYSTEMTIME *pTime) const;
		bool GetReserveTime(SYSTEMTIME *pTime) const;
		bool GetReservedStartTime(SYSTEMTIME *pTime) const;
		bool SetStartTimeSpec(const TimeSpecInfo *pInfo);
		bool GetStartTimeSpec(TimeSpecInfo *pInfo) const;
		bool SetStopTimeSpec(const TimeSpecInfo *pInfo);
		bool GetStopTimeSpec(TimeSpecInfo *pInfo) const;
		bool IsStopTimeSpecified() const;
		void SetStopOnEventEnd(bool fStop) { m_fStopOnEventEnd = fStop; }
		bool GetStopOnEventEnd() const { return m_fStopOnEventEnd; }
		RecordClient GetClient() const { return m_Client; }
		void SetClient(RecordClient Client) { m_Client = Client; }
		void SetRecorderFilter(
			LibISDB::TSEngine *pTSEngine,
			LibISDB::RecorderFilter *pRecorderFilter);
		bool SetRecordingSettings(const CRecordingSettings &Settings);
		bool StartRecord(LPCTSTR pszFileName, bool fTimeShift = false, bool fReserved = false);
		void StopRecord();
		bool PauseRecord();
		bool RelayFile(LPCTSTR pszFileName);
		bool IsRecording() const { return m_fRecording; }
		bool IsPaused() const;
		bool IsReserved() const { return m_fReserved; }
		bool CancelReserve();
		CRecordTask::DurationType GetRecordTime() const;
		CRecordTask::DurationType GetPauseTime() const;
		LONGLONG GetRemainTime() const;
		const CRecordTask *GetRecordTask() const { return &m_RecordTask; }
		bool QueryStart(int Offset = 0) const;
		bool QueryStop(int Offset = 0) const;
		bool RecordDialog(HWND hwndOwner);
		bool GenerateFilePath(
			const FileNameFormatInfo &FormatInfo, LPCWSTR pszFormat,
			String *pFilePath) const;

		CRecordingSettings &GetRecordingSettings() { return m_Settings; }
		const CRecordingSettings &GetRecordingSettings() const { return m_Settings; }
		bool SetCurServiceOnly(bool fOnly);
		bool GetCurServiceOnly() const { return m_Settings.m_fCurServiceOnly; }

		static bool GetWritePluginList(std::vector<String> *pList);
		static bool ShowWritePluginSetting(HWND hwndOwner, LPCTSTR pszPlugin);
	};

} // namespace TVTest


#endif
