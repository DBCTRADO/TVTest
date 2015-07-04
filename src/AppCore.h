#ifndef APP_CORE_H
#define APP_CORE_H


#include "CoreEngine.h"
#include "ChannelManager.h"
#include "Record.h"
#include "Settings.h"
#include "CommandLine.h"


class CAppMain;
class CCommandList;
class CDriverManager;
class CLogoManager;
class CControllerManager;
class CEpgProgramList;
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

	enum {
		OPENTUNER_NO_UI        = 0x0001U,
		OPENTUNER_NO_NOTIFY    = 0x0002U,
		OPENTUNER_RETRY_DIALOG = 0x0004U
	};

	CAppCore(CAppMain &App);
	bool GetDriverDirectory(LPTSTR pszDirectory,int MaxLength) const;
	void OnError(LPCTSTR pszText, ...);
	void OnError(const CBonErrorHandler *pErrorHandler,LPCTSTR pszTitle=NULL);
	void SetSilent(bool fSilent);
	bool IsSilent() const { return m_fSilent; }
	bool SaveCurrentChannel();

	bool InitializeChannel();
	bool GetChannelFileName(LPCTSTR pszDriverFileName,
							LPTSTR pszChannelFileName,int MaxChannelFileName);
	bool RestoreChannel();
	bool UpdateCurrentChannelList(const CTuningSpaceList *pList);
	bool UpdateChannelList(LPCTSTR pszBonDriverName,const CTuningSpaceList *pList);
	const CChannelInfo *GetCurrentChannelInfo() const;
	bool SetChannel(int Space,int Channel,int ServiceID=-1,bool fStrictService=false);
	bool SetChannelByIndex(int Space,int Channel,int ServiceID=-1);
	enum {
		SELECT_CHANNEL_USE_CUR_TUNER  = 0x0001U,
		SELECT_CHANNEL_STRICT_SERVICE = 0x0002U
	};
	bool SelectChannel(LPCTSTR pszTunerName,const CChannelInfo &ChannelInfo,unsigned int Flags=0);
	bool SwitchChannel(int Channel);
	bool SwitchChannelByNo(int ChannelNo,bool fSwitchService);
	bool SetCommandLineChannel(const CCommandLineOptions *pCmdLine);
	bool FollowChannelChange(WORD TransportStreamID,WORD ServiceID);
	enum {
		SET_SERVICE_STRICT_ID					= 0x0001U,
		SET_SERVICE_NO_CHANGE_CUR_SERVICE_ID	= 0x0002U
	};
	bool SetServiceByID(WORD ServiceID,unsigned int Flags=0);
	bool SetServiceByIndex(int Service,unsigned int Flags=0);
	bool GetCurrentStreamIDInfo(StreamIDInfo *pInfo) const;
	bool GetCurrentStreamChannelInfo(CChannelInfo *pInfo) const;
	bool GetCurrentServiceName(LPTSTR pszName,int MaxLength,bool fUseChannelName=true);

	bool OpenTuner(LPCTSTR pszFileName);
	bool OpenTuner();
	bool OpenAndInitializeTuner(unsigned int OpenFlags=0);
	bool CloseTuner();
	void ShutDownTuner();
	void ResetEngine();

	bool Set1SegMode(bool f1Seg,bool fServiceChange);
	bool Is1SegMode() const { return m_f1SegMode; }

	void ApplyBonDriverOptions();

	bool StartRecord(LPCTSTR pszFileName=NULL,
					 const CRecordManager::TimeSpecInfo *pStartTime=NULL,
					 const CRecordManager::TimeSpecInfo *pStopTime=NULL,
					 CRecordManager::RecordClient Client=CRecordManager::CLIENT_USER,
					 bool fTimeShift=false);
	bool ModifyRecord(LPCTSTR pszFileName=NULL,
					  const CRecordManager::TimeSpecInfo *pStartTime=NULL,
					  const CRecordManager::TimeSpecInfo *pStopTime=NULL,
					  CRecordManager::RecordClient Client=CRecordManager::CLIENT_USER);
	bool StartReservedRecord();
	bool CancelReservedRecord();
	bool StopRecord();
	bool PauseResumeRecording();
	bool RelayRecord(LPCTSTR pszFileName);
	bool CommandLineRecord(const CCommandLineOptions *pCmdLine);
	bool CommandLineRecord(LPCTSTR pszFileName,const FILETIME *pStartTime,int Delay,int Duration);
	LPCTSTR GetDefaultRecordFolder() const;
	bool GetExitOnRecordingStop() const { return m_fExitOnRecordingStop; }
	void SetExitOnRecordingStop(bool fExit) { m_fExitOnRecordingStop=fExit; }

	void BeginChannelScan(int Space);
	bool IsChannelScanning() const;
	bool IsDriverNoSignalLevel(LPCTSTR pszFileName) const;

	bool GetVariableStringEventInfo(TVTest::CEventVariableStringMap::EventInfo *pInfo,
									DWORD NextEventMargin=0) const;

private:
	CAppMain &m_App;
	bool m_fSilent;
	bool m_fExitOnRecordingStop;
	bool m_f1SegMode;

	int GetCorresponding1SegService(int Space,WORD NetworkID,WORD TSID,WORD ServiceID) const;
	bool GenerateRecordFileName(LPTSTR pszFileName,int MaxFileName);

	// ÉRÉsÅ[ã÷é~
	CAppCore(const CAppCore &) /* = delete */;
	CAppCore &operator=(const CAppCore &) /* = delete */;
};


#endif
