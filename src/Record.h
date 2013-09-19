#ifndef RECORD_H
#define RECORD_H


#include "DtvEngine.h"


class CRecordTime {
	FILETIME m_Time;
	DWORD m_TickTime;

public:
	CRecordTime();
	bool SetCurrentTime();
	bool GetTime(FILETIME *pTime) const;
	DWORD GetTickTime() const { return m_TickTime; }
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

protected:
	State m_State;
	CDtvEngine *m_pDtvEngine;
	CRecordTime m_StartTime;
	DWORD m_PauseStartTime;
	DWORD m_TotalPauseTime;

public:
	CRecordTask();
	virtual ~CRecordTask();
	bool Start(CDtvEngine *pDtvEngine,LPCTSTR pszFileName);
	bool Stop();
	bool Pause();
	State GetState() const;
	bool IsStopped() const { return m_State==STATE_STOP; }
	bool IsRecording() const { return m_State==STATE_RECORDING; }
	bool IsPaused() const { return m_State==STATE_PAUSE; }
	DWORD GetStartTime() const;
	bool GetStartTime(FILETIME *pTime) const;
	bool GetStartTime(CRecordTime *pTime) const;
	DWORD GetRecordTime() const;
	DWORD GetPauseTime() const;
	LONGLONG GetWroteSize() const;
	LPCTSTR GetFileName() const;
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
	CDynamicString m_FileName;
	CRecordTime m_ReserveTime;
	TimeSpecInfo m_StartTimeSpec;
	TimeSpecInfo m_StopTimeSpec;
	bool m_fStopOnEventEnd;
	RecordClient m_Client;
	CRecordTask m_RecordTask;
	CDtvEngine *m_pDtvEngine;
	//FileExistsOperation m_ExistsOperation;
	bool m_fCurServiceOnly;
	DWORD m_SaveStream;
	SIZE_T m_BufferSize;

	static CRecordManager *GetThis(HWND hDlg);
	static INT_PTR CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	int FormatFileName(LPTSTR pszFileName,int MaxFileName,const EventInfo *pEventInfo,LPCTSTR pszFormat) const;
	static int MapFileNameCopy(LPWSTR pszFileName,int MaxFileName,LPCWSTR pszText);

public:
	CRecordManager();
	~CRecordManager();
	bool SetFileName(LPCTSTR pszFileName);
	LPCTSTR GetFileName() const { return m_FileName.Get(); }
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
	DWORD GetRecordTime() const;
	DWORD GetPauseTime() const;
	LONGLONG GetRemainTime() const;
	const CRecordTask *GetRecordTask() const { return &m_RecordTask; }
	bool QueryStart(int Offset=0) const;
	bool QueryStop(int Offset=0) const;
	bool RecordDialog(HWND hwndOwner);
	bool GenerateFileName(LPTSTR pszFileName,int MaxLength,const EventInfo *pEventInfo,LPCTSTR pszFormat=NULL) const;
	//bool DoFileExistsOperation(HWND hwndOwner,LPTSTR pszFileName);
	bool SetCurServiceOnly(bool fOnly);
	bool GetCurServiceOnly() const { return m_fCurServiceOnly; }
	bool SetSaveStream(DWORD Stream);
	DWORD GetSaveStream() const { return m_SaveStream; }
	bool SetBufferSize(SIZE_T BufferSize);
	static bool InsertFileNameParameter(HWND hDlg,int ID,const POINT *pMenuPos);
	static void GetEventInfoSample(EventInfo *pEventInfo);
};


#endif
