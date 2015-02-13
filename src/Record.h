#ifndef RECORD_H
#define RECORD_H


#include <vector>
#include "DtvEngine.h"


class CRecordingSettings
{
public:
	static const DWORD DEFAULT_BUFFER_SIZE=0x100000;

	bool m_fCurServiceOnly;
	DWORD m_SaveStream;
	TVTest::String m_WritePlugin;
	DWORD m_BufferSize;
	ULONGLONG m_PreAllocationUnit;
	bool m_fTimeShift;

	CRecordingSettings();
	bool IsSaveCaption() const;
	void SetSaveCaption(bool fSave);
	bool IsSaveDataCarrousel() const;
	void SetSaveDataCarrousel(bool fSave);

private:
	bool TestSaveStreamFlag(DWORD Flag) const;
	void SetSaveStreamFlag(DWORD Flag,bool fSet);
};

class CRecordTime {
	FILETIME m_Time;
	Util::TickCountType m_TickTime;

public:
	CRecordTime();
	bool SetCurrentTime();
	bool GetTime(FILETIME *pTime) const;
	Util::TickCountType GetTickTime() const { return m_TickTime; }
	void Clear();
	bool IsValid() const;
};

class CRecordTask {
public:
	enum State {
		STATE_STOP,
		STATE_RECORDING,
		STATE_PAUSE
	};

	typedef Util::TickCountType DurationType;

protected:
	State m_State;
	CDtvEngine *m_pDtvEngine;
	CRecordTime m_StartTime;
	DurationType m_PauseStartTime;
	DurationType m_TotalPauseTime;

public:
	CRecordTask();
	virtual ~CRecordTask();
	bool Start(CDtvEngine *pDtvEngine,LPCTSTR pszFileName,const CRecordingSettings &Settings);
	bool Stop();
	bool Pause();
	State GetState() const;
	bool IsStopped() const { return m_State==STATE_STOP; }
	bool IsRecording() const { return m_State==STATE_RECORDING; }
	bool IsPaused() const { return m_State==STATE_PAUSE; }
	DurationType GetStartTime() const;
	bool GetStartTime(FILETIME *pTime) const;
	bool GetStartTime(CRecordTime *pTime) const;
	DurationType GetRecordTime() const;
	DurationType GetPauseTime() const;
	LONGLONG GetWroteSize() const;
	int GetFileName(LPTSTR pszFileName,int MaxFileName) const;
	bool RelayFile(LPCTSTR pszFileName);
#undef GetFreeSpace
	LONGLONG GetFreeSpace() const;
};

class CRecordManager : public CBonErrorHandler {
public:
	enum TimeSpecType {
		TIME_NOTSPECIFIED,
		TIME_DATETIME,
		TIME_DURATION
	};
	struct TimeSpecInfo {
		TimeSpecType Type;
		union {
			FILETIME DateTime;
			ULONGLONG Duration;
		} Time;
	};
	enum RecordClient {
		CLIENT_USER,
		CLIENT_COMMANDLINE,
		CLIENT_PLUGIN
	};
	/*
	enum FileExistsOperation {
		EXISTS_OVERWRITE,
		EXISTS_CONFIRM,
		EXISTS_SEQUENCIALNUMBER
	};
	*/

	struct EventInfo {
		LPCTSTR pszChannelName;
		int ChannelNo;
		LPCTSTR pszServiceName;
		WORD ServiceID;
		LPCTSTR pszEventName;
		WORD EventID;
		SYSTEMTIME stTotTime;
	};

private:
	bool m_fRecording;
	bool m_fReserved;
	TVTest::String m_FileName;
	CRecordTime m_ReserveTime;
	TimeSpecInfo m_StartTimeSpec;
	TimeSpecInfo m_StopTimeSpec;
	bool m_fStopOnEventEnd;
	RecordClient m_Client;
	CRecordTask m_RecordTask;
	CDtvEngine *m_pDtvEngine;
	//FileExistsOperation m_ExistsOperation;
	CRecordingSettings m_Settings;

	std::vector<TVTest::String> m_WritePluginList;

	static CRecordManager *GetThis(HWND hDlg);
	static INT_PTR CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	int FormatFileName(LPTSTR pszFileName,int MaxFileName,const EventInfo *pEventInfo,LPCTSTR pszFormat) const;
	static int MapFileNameCopy(LPWSTR pszFileName,int MaxFileName,LPCWSTR pszText);

public:
	CRecordManager();
	~CRecordManager();
	bool SetFileName(LPCTSTR pszFileName);
	LPCTSTR GetFileName() const { return TVTest::StringUtility::GetCStrOrNull(m_FileName); }
	/*
	bool SetFileExistsOperation(FileExistsOperation Operation);
	FileExistsOperation GetFileExistsOperation() const { return m_ExistsOperation; }
	*/
	bool GetStartTime(FILETIME *pTime) const;
	bool GetReserveTime(FILETIME *pTime) const;
	bool GetReservedStartTime(FILETIME *pTime) const;
	bool SetStartTimeSpec(const TimeSpecInfo *pInfo);
	bool GetStartTimeSpec(TimeSpecInfo *pInfo) const;
	bool SetStopTimeSpec(const TimeSpecInfo *pInfo);
	bool GetStopTimeSpec(TimeSpecInfo *pInfo) const;
	bool IsStopTimeSpecified() const;
	void SetStopOnEventEnd(bool fStop) { m_fStopOnEventEnd=fStop; }
	bool GetStopOnEventEnd() const { return m_fStopOnEventEnd; }
	RecordClient GetClient() const { return m_Client; }
	void SetClient(RecordClient Client) { m_Client=Client; }
	bool StartRecord(CDtvEngine *pDtvEngine,LPCTSTR pszFileName,bool fTimeShift=false);
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
	bool QueryStart(int Offset=0) const;
	bool QueryStop(int Offset=0) const;
	bool RecordDialog(HWND hwndOwner);
	bool GenerateFileName(LPTSTR pszFileName,int MaxLength,const EventInfo *pEventInfo,LPCTSTR pszFormat=NULL) const;
	//bool DoFileExistsOperation(HWND hwndOwner,LPTSTR pszFileName);

	CRecordingSettings &GetRecordingSettings() { return m_Settings; }
	const CRecordingSettings &GetRecordingSettings() const { return m_Settings; }
	bool SetCurServiceOnly(bool fOnly);
	bool GetCurServiceOnly() const { return m_Settings.m_fCurServiceOnly; }
	bool SetSaveStream(DWORD Stream);
	bool SetWritePlugin(LPCTSTR pszPlugin);
	LPCTSTR GetWritePlugin() const;
	bool SetBufferSize(DWORD BufferSize);

	static bool InsertFileNameParameter(HWND hDlg,int ID,const POINT *pMenuPos);
	static void GetEventInfoSample(EventInfo *pEventInfo);
	static bool GetWritePluginList(std::vector<TVTest::String> *pList);
	static bool ShowWritePluginSetting(HWND hwndOwner,LPCTSTR pszPlugin);
};


#endif
