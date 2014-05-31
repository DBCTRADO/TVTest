#ifndef APP_MAIN_H
#define APP_MAIN_H


#include "CoreEngine.h"
#include "UICore.h"
#include "ChannelManager.h"
#include "Record.h"
#include "Settings.h"


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

class CAppMain
{
	struct CasLibraryNetworkInfo
	{
		WORD NetworkID;
		WORD TSID;
		TVTest::String FileName;
	};

	CUICore m_UICore;
	bool m_fSilent;
	TCHAR m_szIniFileName[MAX_PATH];
	TCHAR m_szChannelSettingFileName[MAX_PATH];
	TCHAR m_szFavoritesFileName[MAX_PATH];
	CSettings m_Settings;
	bool m_fFirstExecute;

	TVTest::String m_DefaultCasLibrary;
	std::vector<CasLibraryNetworkInfo> m_CasLibraryNetworkMap;
	bool m_fCasCardOpenError;

	bool m_f1SegMode;

	int GetCorresponding1SegService(int Space,WORD NetworkID,WORD TSID,WORD ServiceID) const;
	bool GenerateRecordFileName(LPTSTR pszFileName,int MaxFileName) const;
	const TVTest::String &GetCasLibraryFileName(WORD NetworkID,WORD TSID) const;
	void OutCasCardInfo();

	// ÉRÉsÅ[ã÷é~
	CAppMain(const CAppMain &);
	CAppMain &operator=(const CAppMain &);

public:
	struct ChannelSelectInfo {
		CChannelInfo Channel;
		TVTest::String TunerName;
		bool fUseCurTuner;
	};

	struct StreamIDInfo {
		WORD NetworkID;
		WORD TransportStreamID;
		WORD ServiceID;
	};

	CAppMain();
	bool Initialize();
	bool Finalize();
	HINSTANCE GetInstance() const;
	HINSTANCE GetResourceInstance() const;
	bool GetAppDirectory(LPTSTR pszDirectory) const;
	bool GetDriverDirectory(LPTSTR pszDirectory,int MaxLength) const;
	LPCTSTR GetIniFileName() const { return m_szIniFileName; }
	LPCTSTR GetFavoritesFileName() const { return m_szFavoritesFileName; }
	void AddLog(LPCTSTR pszText, ...);
	void OnError(const CBonErrorHandler *pErrorHandler,LPCTSTR pszTitle=NULL);
	void SetSilent(bool fSilent);
	bool IsSilent() const { return m_fSilent; }
	bool LoadSettings();
	enum {
		SETTINGS_SAVE_STATUS  = 0x0001U,
		SETTINGS_SAVE_OPTIONS = 0x0002U,
		SETTINGS_SAVE_ALL     = SETTINGS_SAVE_STATUS | SETTINGS_SAVE_OPTIONS
	};
	bool SaveSettings(unsigned int Flags);
	void InitializeCommandSettings();
	bool SaveCurrentChannel();
	bool IsFirstExecute() const;

	bool SaveChannelSettings();
	bool InitializeChannel();
	bool GetChannelFileName(LPCTSTR pszDriverFileName,
							LPTSTR pszChannelFileName,int MaxChannelFileName);
	bool RestoreChannel();
	bool UpdateCurrentChannelList(const CTuningSpaceList *pList);
	bool UpdateChannelList(LPCTSTR pszBonDriverName,const CTuningSpaceList *pList);
	const CChannelInfo *GetCurrentChannelInfo() const;
	bool SetChannel(int Space,int Channel,int ServiceID=-1,bool fStrictService=false);
	bool SetChannelByIndex(int Space,int Channel,int ServiceID=-1);
	bool SelectChannel(const ChannelSelectInfo &SelInfo);
	bool SwitchChannel(int Channel);
	bool SwitchChannelByNo(int ChannelNo,bool fSwitchService);
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
	bool OpenTunerAndSetChannel(LPCTSTR pszDriverFileName,const CChannelInfo *pChannelInfo);
	bool OpenTuner();
	bool OpenAndInitializeTuner(unsigned int OpenCasCardFlags=0);
	bool CloseTuner();

	bool Set1SegMode(bool f1Seg,bool fServiceChange);
	bool Is1SegMode() const { return m_f1SegMode; }

	enum {
		OPEN_CAS_CARD_RETRY        = 0x0001U,
		OPEN_CAS_CARD_NO_UI        = 0x0002U,
		OPEN_CAS_CARD_NOTIFY_ERROR = 0x0004U
	};
	bool OpenCasCard(unsigned int Flags=0);
	bool ChangeCasCard(int Device,LPCTSTR pszName=NULL);
	bool LoadCasLibrary(LPCTSTR pszFileName);
	bool LoadCasLibrary();
	void SetDefaultCasLibrary(LPCTSTR pszFileName);
	LPCTSTR GetDefaultCasLibrary() const { return m_DefaultCasLibrary.c_str(); }
	bool HasDefaultCasLibrary() const { return !m_DefaultCasLibrary.empty(); }
	bool HasCasLibraryNetworkMap() const { return !m_CasLibraryNetworkMap.empty(); }

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
	bool RelayRecord(LPCTSTR pszFileName);
	LPCTSTR GetDefaultRecordFolder() const;

	bool ShowHelpContent(int ID);
	void BeginChannelScan(int Space);
	bool IsChannelScanning() const;
	bool IsDriverNoSignalLevel(LPCTSTR pszFileName) const;
	void SetProgress(int Pos,int Max);
	void EndProgress();
	COLORREF GetColor(LPCTSTR pszText) const;

	CCoreEngine *GetCoreEngine();
	const CCoreEngine *GetCoreEngine() const;
	CUICore *GetUICore();
	const CCommandList *GetCommandList() const;
	const CChannelManager *GetChannelManager() const;
	const CRecordManager *GetRecordManager() const;
	const CDriverManager *GetDriverManager() const;
	CEpgProgramList *GetEpgProgramList() const;
	CLogoManager *GetLogoManager() const;
	CControllerManager *GetControllerManager() const;
	TVTest::CFavoritesManager *GetFavoritesManager() const;
	CRecentChannelList *GetRecentChannelList() const;
};


CAppMain &GetAppClass();


#endif
