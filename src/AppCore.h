#ifndef APP_CORE_H
#define APP_CORE_H


#include "CoreEngine.h"
#include "ChannelManager.h"
#include "Record.h"
#include "Settings.h"
#include "CommandLine.h"
#include "TSProcessorManager.h"


class CAppMain;
class CCommandList;
class CDriverManager;
class CLogoManager;
class CControllerManager;
class CRecentChannelList;

namespace TVTest
{
	class CFavoritesManager;
}

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
	};

	enum class SelectChannelFlag : unsigned int {
		None            = 0x0000U,
		UseCurrentTuner = 0x0001U,
		StrictService   = 0x0002U,
	};

	enum class SetServiceFlag : unsigned int {
		None                     = 0x0000U,
		StrictID                 = 0x0001U,
		NoChangeCurrentServiceID = 0x0002U,
	};

	CAppCore(CAppMain &App);

	CAppCore(const CAppCore &) = delete;
	CAppCore &operator=(const CAppCore &) = delete;

	void OnError(LPCTSTR pszText, ...);
	void OnError(const LibISDB::ErrorHandler *pErrorHandler, LPCTSTR pszTitle = nullptr);
	void SetSilent(bool fSilent);
	bool IsSilent() const { return m_fSilent; }
	bool SaveCurrentChannel();

	bool InitializeChannel();
	bool GetChannelFileName(LPCTSTR pszDriverFileName, TVTest::String *pChannelFileName);
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
	bool GetCurrentStreamChannelInfo(CChannelInfo *pInfo) const;
	bool GetCurrentServiceName(LPTSTR pszName, int MaxLength, bool fUseChannelName = true);

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

	void NotifyTSProcessorNetworkChanged(TVTest::CTSProcessorManager::FilterOpenFlag FilterOpenFlags);

	bool GetVariableStringEventInfo(
		TVTest::CEventVariableStringMap::EventInfo *pInfo,
		DWORD NextEventMargin = 0) const;

private:
	CAppMain &m_App;
	bool m_fSilent;
	bool m_fExitOnRecordingStop;
	bool m_f1SegMode;

	int GetCorresponding1SegService(int Space, WORD NetworkID, WORD TSID, WORD ServiceID) const;
	bool GenerateRecordFileName(LPTSTR pszFileName, int MaxFileName);
};

TVTEST_ENUM_FLAGS(CAppCore::OpenTunerFlag)
TVTEST_ENUM_FLAGS(CAppCore::SelectChannelFlag)
TVTEST_ENUM_FLAGS(CAppCore::SetServiceFlag)


#endif
