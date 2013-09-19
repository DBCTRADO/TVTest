#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "Record.h"
#include "DialogUtil.h"
#include "StdUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CRecordTime::CRecordTime()
{
	Clear();
}


bool CRecordTime::SetCurrentTime()
{
	SYSTEMTIME st;

	::GetLocalTime(&st);
	m_TickTime=::GetTickCount();
	::SystemTimeToFileTime(&st,&m_Time);
	return true;
}


bool CRecordTime::GetTime(FILETIME *pTime) const
{
	if (!IsValid())
		return false;
	*pTime=m_Time;
	return true;
}


void CRecordTime::Clear()
{
	::ZeroMemory(&m_Time,sizeof(m_Time));
	m_TickTime=0;
}


bool CRecordTime::IsValid() const
{
	if (m_Time.dwLowDateTime==0 && m_Time.dwHighDateTime==0)
		return false;
	return true;
}




CRecordTask::CRecordTask()
	: m_State(STATE_STOP)
	, m_pDtvEngine(NULL)
{
}


CRecordTask::~CRecordTask()
{
	Stop();
}


bool CRecordTask::Start(CDtvEngine *pDtvEngine,LPCTSTR pszFileName)
{
	if (m_State!=STATE_STOP)
		return false;
	if (!pDtvEngine->m_FileWriter.OpenFile(pszFileName,
										   CNCachedFile::CNF_SHAREREAD))
		return false;
	m_State=STATE_RECORDING;
	m_pDtvEngine=pDtvEngine;
	m_StartTime.SetCurrentTime();
	m_TotalPauseTime=0;
	return true;
}


bool CRecordTask::Stop()
{
	if (m_State==STATE_RECORDING || m_State==STATE_PAUSE) {
		m_pDtvEngine->m_FileWriter.CloseFile();
		m_State=STATE_STOP;
		m_pDtvEngine=NULL;
	}
	return true;
}


bool CRecordTask::Pause()
{
	if (m_State==STATE_RECORDING) {
		m_pDtvEngine->m_FileWriter.Pause();
		m_State=STATE_PAUSE;
		m_PauseStartTime=::GetTickCount();
	} else if (m_State==STATE_PAUSE) {
		m_pDtvEngine->m_FileWriter.ClearQueue();
		m_pDtvEngine->m_FileWriter.Resume();
		m_State=STATE_RECORDING;
		m_TotalPauseTime+=TickTimeSpan(m_PauseStartTime,::GetTickCount());
	} else
		return false;
	return true;
}


CRecordTask::State CRecordTask::GetState() const
{
	return m_State;
}


DWORD CRecordTask::GetStartTime() const
{
	if (m_State==STATE_STOP)
		return 0;
	return m_StartTime.GetTickTime();
}


bool CRecordTask::GetStartTime(FILETIME *pTime) const
{
	if (m_State==STATE_STOP)
		return false;
	return m_StartTime.GetTime(pTime);
}


bool CRecordTask::GetStartTime(CRecordTime *pTime) const
{
	if (m_State==STATE_STOP)
		return false;
	*pTime=m_StartTime;
	return true;
}


DWORD CRecordTask::GetRecordTime() const
{
	DWORD Time;

	if (m_State==STATE_RECORDING) {
		Time=TickTimeSpan(m_StartTime.GetTickTime(),::GetTickCount());
	} else if (m_State==STATE_PAUSE) {
		Time=TickTimeSpan(m_StartTime.GetTickTime(),m_PauseStartTime);
	} else
		return 0;
	return Time-m_TotalPauseTime;
}


DWORD CRecordTask::GetPauseTime() const
{
	if (m_State==STATE_RECORDING)
		return m_TotalPauseTime;
	if (m_State==STATE_PAUSE)
		return TickTimeSpan(m_PauseStartTime,::GetTickCount())+m_TotalPauseTime;
	return 0;
}


LONGLONG CRecordTask::GetWroteSize() const
{
	if (m_State==STATE_STOP)
		return 0;
	return m_pDtvEngine->m_FileWriter.GetWriteSize();
}


LPCTSTR CRecordTask::GetFileName() const
{
	if (m_State==STATE_STOP)
		return NULL;
	return m_pDtvEngine->m_FileWriter.GetFileName();
}


bool CRecordTask::RelayFile(LPCTSTR pszFileName)
{
	if (m_State==STATE_STOP)
		return false;
	return m_pDtvEngine->m_FileWriter.RelayFile(pszFileName,CNCachedFile::CNF_SHAREREAD);
}


LONGLONG CRecordTask::GetFreeSpace() const
{
	if (m_State==STATE_STOP)
		return -1;

	LPCTSTR pszFileName=m_pDtvEngine->m_FileWriter.GetFileName();
	if (pszFileName==NULL)
		return -1;
	TCHAR szPath[MAX_PATH];
	::lstrcpy(szPath,pszFileName);
	*::PathFindFileName(szPath)='\0';
	ULARGE_INTEGER FreeSpace;
	if (!::GetDiskFreeSpaceEx(szPath,&FreeSpace,NULL,NULL))
		return -1;
	return (LONGLONG)FreeSpace.QuadPart;
}




CRecordManager::CRecordManager()
	: m_fRecording(false)
	, m_fReserved(false)
	, m_fStopOnEventEnd(false)
	, m_Client(CLIENT_USER)
	, m_pDtvEngine(NULL)
	//, m_ExistsOperation(EXISTS_CONFIRM)
	, m_fCurServiceOnly(false)
	, m_SaveStream(CTsSelector::STREAM_ALL)
	, m_BufferSize(0x100000)
{
	m_StartTimeSpec.Type=TIME_NOTSPECIFIED;
	m_StopTimeSpec.Type=TIME_NOTSPECIFIED;
}


CRecordManager::~CRecordManager()
{
	StopRecord();
}


bool CRecordManager::SetFileName(LPCTSTR pszFileName)
{
	if (m_fRecording)
		return false;
	return m_FileName.Set(pszFileName);
}


/*
bool CRecordManager::SetFileExistsOperation(FileExistsOperation Operation)
{
	if (m_fRecording)
		return false;
	m_ExistsOperation=Operation;
	return true;
}
*/


bool CRecordManager::GetStartTime(FILETIME *pTime) const
{
	if (!m_fRecording)
		return false;
	return m_RecordTask.GetStartTime(pTime);
}


bool CRecordManager::GetReserveTime(FILETIME *pTime) const
{
	if (!m_fReserved)
		return false;
	return m_ReserveTime.GetTime(pTime);
}


bool CRecordManager::GetReservedStartTime(FILETIME *pTime) const
{
	if (!m_fReserved)
		return false;
	if (m_StartTimeSpec.Type==TIME_DATETIME) {
		*pTime=m_StartTimeSpec.Time.DateTime;
	} else if (m_StartTimeSpec.Type==TIME_DURATION) {
		if (!m_ReserveTime.GetTime(pTime))
			return false;
		*pTime+=m_StartTimeSpec.Time.Duration*FILETIME_MILLISECOND;
	} else
		return false;
	return true;
}


bool CRecordManager::SetStartTimeSpec(const TimeSpecInfo *pInfo)
{
	if (m_fRecording)
		return false;
	if (pInfo!=NULL && pInfo->Type!=TIME_NOTSPECIFIED) {
		m_fReserved=true;
		m_ReserveTime.SetCurrentTime();
		m_StartTimeSpec=*pInfo;
	} else {
		m_fReserved=false;
		m_StartTimeSpec.Type=TIME_NOTSPECIFIED;
	}
	return true;
}


bool CRecordManager::GetStartTimeSpec(TimeSpecInfo *pInfo) const
{
	*pInfo=m_StartTimeSpec;
	return true;
}


bool CRecordManager::SetStopTimeSpec(const TimeSpecInfo *pInfo)
{
	if (pInfo!=NULL)
		m_StopTimeSpec=*pInfo;
	else
		m_StopTimeSpec.Type=TIME_NOTSPECIFIED;
	return true;
}


bool CRecordManager::GetStopTimeSpec(TimeSpecInfo *pInfo) const
{
	*pInfo=m_StopTimeSpec;
	return true;
}


bool CRecordManager::IsStopTimeSpecified() const
{
	return m_StopTimeSpec.Type!=TIME_NOTSPECIFIED;
}


bool CRecordManager::StartRecord(CDtvEngine *pDtvEngine,LPCTSTR pszFileName,bool fTimeShift)
{
	if (m_fRecording)
		return false;
	bool fOldWriteCurOnly=pDtvEngine->GetWriteCurServiceOnly();
	DWORD OldWriteStream;
	pDtvEngine->GetWriteStream(NULL,&OldWriteStream);
	pDtvEngine->SetWriteCurServiceOnly(m_fCurServiceOnly,m_SaveStream);
	pDtvEngine->m_FileWriter.SetBufferSize((DWORD)m_BufferSize);
	if (!fTimeShift)
		pDtvEngine->m_FileWriter.ClearQueue();
	if (!m_RecordTask.Start(pDtvEngine,pszFileName)) {
		pDtvEngine->SetWriteCurServiceOnly(fOldWriteCurOnly,OldWriteStream);
		SetError(pDtvEngine->m_FileWriter.GetLastErrorException());
		return false;
	}
	m_pDtvEngine=pDtvEngine;
	m_fRecording=true;
	m_fReserved=false;
	m_StartTimeSpec.Type=TIME_NOTSPECIFIED;
	ClearError();
	return true;
}


void CRecordManager::StopRecord()
{
	if (m_fRecording) {
		m_RecordTask.Stop();
		m_fRecording=false;
		if (m_fCurServiceOnly)
			m_pDtvEngine->SetWriteCurServiceOnly(false);
		//m_FileName.Clear();
		m_pDtvEngine=NULL;
	}
}


bool CRecordManager::PauseRecord()
{
	if (!m_fRecording)
		return false;
	return m_RecordTask.Pause();
}


bool CRecordManager::RelayFile(LPCTSTR pszFileName)
{
	if (!m_fRecording)
		return false;
	if (!m_RecordTask.RelayFile(pszFileName)) {
		SetError(m_pDtvEngine->m_FileWriter.GetLastErrorException());
		return false;
	}
	ClearError();
	return true;
}


bool CRecordManager::IsPaused() const
{
	return m_fRecording && m_RecordTask.IsPaused();
}


bool CRecordManager::CancelReserve()
{
	if (!m_fReserved)
		return false;
	m_fReserved=false;
	return true;
}


DWORD CRecordManager::GetRecordTime() const
{
	if (!m_fRecording)
		return 0;
	return m_RecordTask.GetRecordTime();
}


DWORD CRecordManager::GetPauseTime() const
{
	if (!m_fRecording)
		return 0;
	return m_RecordTask.GetPauseTime();
}


LONGLONG CRecordManager::GetRemainTime() const
{
	if (!m_fRecording)
		return -1;

	LONGLONG Remain;
	switch (m_StopTimeSpec.Type) {
	case TIME_DATETIME:
		{
			FILETIME ft;

			GetLocalTimeAsFileTime(&ft);
			Remain=(m_StopTimeSpec.Time.DateTime-ft)/FILETIME_MILLISECOND;
		}
		break;
	case TIME_DURATION:
		Remain=m_StopTimeSpec.Time.Duration-
			(LONGLONG)TickTimeSpan(m_RecordTask.GetStartTime(),::GetTickCount());
		break;
	default:
		Remain=-1;
	}
	return Remain;
}


bool CRecordManager::QueryStart(int Offset) const
{
	if (!m_fReserved)
		return false;
	switch (m_StartTimeSpec.Type) {
	case TIME_DATETIME:
		{
			FILETIME ft;

			GetLocalTimeAsFileTime(&ft);
			if (Offset!=0)
				ft+=(LONGLONG)Offset*(FILETIME_SECOND/1000);
			if (::CompareFileTime(&ft,&m_StartTimeSpec.Time.DateTime)>=0)
				return true;
		}
		break;
	case TIME_DURATION:
		{
			DWORD Span=TickTimeSpan(m_ReserveTime.GetTickTime(),::GetTickCount());

			if (Offset!=0) {
				if ((LONGLONG)Offset<=-(LONGLONG)Span)
					return true;
				Span+=Offset;
			}
			if ((ULONGLONG)Span>=m_StartTimeSpec.Time.Duration)
				return true;
		}
		break;
	}
	return false;
}


bool CRecordManager::QueryStop(int Offset) const
{
	if (!m_fRecording)
		return false;
	switch (m_StopTimeSpec.Type) {
	case TIME_DATETIME:
		{
			FILETIME ft;

			GetLocalTimeAsFileTime(&ft);
			if (Offset!=0)
				ft+=(LONGLONG)Offset*FILETIME_MILLISECOND;
			if (::CompareFileTime(&ft,&m_StopTimeSpec.Time.DateTime)>=0)
				return true;
		}
		break;
	case TIME_DURATION:
		{
			DWORD Span;

			Span=TickTimeSpan(m_RecordTask.GetStartTime(),::GetTickCount());
			if (Offset!=0) {
				if ((LONGLONG)Offset<=-(LONGLONG)Span)
					return true;
				Span+=Offset;
			}
			if ((ULONGLONG)Span>=m_StopTimeSpec.Time.Duration)
				return true;
		}
		break;
	}
	return false;
}


CRecordManager *CRecordManager::GetThis(HWND hDlg)
{
	return static_cast<CRecordManager*>(::GetProp(hDlg,TEXT("This")));
}


static void SetDateTimeFormat(HWND hDlg,UINT ID)
{
	int Length;
	TCHAR szText[256];

	GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_SSHORTDATE,szText,
														lengthof(szText)-1);
	Length=lstrlen(szText);
	szText[Length++]=' ';
	GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_STIMEFORMAT,
										szText+Length,lengthof(szText)-Length);
	DateTime_SetFormat(GetDlgItem(hDlg,ID),szText);
}


static BOOL DateTime_SetFiletime(HWND hwndDT,DWORD flag,FILETIME *pFileTime)
{
	SYSTEMTIME st;

	FileTimeToSystemTime(pFileTime,&st);
	return DateTime_SetSystemtime(hwndDT,flag,&st);
}


#pragma warning(disable: 4700)	// 初期化されていないローカル変数 'ftStop' が使用されます

INT_PTR CALLBACK CRecordManager::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CRecordManager *pThis=reinterpret_cast<CRecordManager*>(lParam);
			SYSTEMTIME st;
			FILETIME ftTime;

			::SetProp(hDlg,TEXT("This"),pThis);
			::SendDlgItemMessage(hDlg,IDC_RECORD_FILENAME,EM_LIMITTEXT,MAX_PATH-1,0);
			if (!pThis->m_FileName.IsEmpty())
				::SetDlgItemText(hDlg,IDC_RECORD_FILENAME,pThis->m_FileName.Get());
			/*
			static const LPCTSTR pszExistsOperation[] = {
				TEXT("上書きする"),TEXT("確認を取る"),TEXT("連番を付加する")
			};
			for (i=0;i<3;i++)
				::SendDlgItemMessage(hDlg,IDC_RECORD_FILEEXISTS,CB_ADDSTRING,0,
							reinterpret_cast<LPARAM>(pszExistsOperation[i]));
			::SendDlgItemMessage(hDlg,IDC_RECORD_FILEEXISTS,CB_SETCURSEL,
										(WPARAM)pThis->m_ExistsOperation,0);
			::EnableDlgItems(hDlg,IDC_RECORD_FILENAME_LABEL,IDC_RECORD_FILEEXISTS,!pThis->m_fRecording);
			*/
			::EnableDlgItems(hDlg,IDC_RECORD_FILENAME_LABEL,IDC_RECORD_FILENAMEPREVIEW,!pThis->m_fRecording);

			// 開始時間
			DWORD Delay;
			::CheckRadioButton(hDlg,IDC_RECORD_START_NOW,IDC_RECORD_START_DELAY,
							   IDC_RECORD_START_NOW+pThis->m_StartTimeSpec.Type);
			switch (pThis->m_StartTimeSpec.Type) {
			case TIME_NOTSPECIFIED:
				::GetLocalTime(&st);
				st.wSecond=0;
				st.wMilliseconds=0;
				::SystemTimeToFileTime(&st,&ftTime);
				Delay=60*1000;
				break;
			case TIME_DATETIME:
				{
					FILETIME ft;
					ULARGE_INTEGER Time1,Time2;

					::GetLocalTime(&st);
					::SystemTimeToFileTime(&st,&ft);
					ftTime=pThis->m_StartTimeSpec.Time.DateTime;
					Time1.LowPart=ft.dwLowDateTime;
					Time1.HighPart=ft.dwHighDateTime;
					Time2.LowPart=ftTime.dwLowDateTime;
					Time2.HighPart=ftTime.dwHighDateTime;
					if (Time1.QuadPart<Time2.QuadPart) {
						Delay=
							(DWORD)((Time2.QuadPart-Time1.QuadPart)/FILETIME_MILLISECOND);
					} else {
						Delay=0;
					}
				}
				break;
			case TIME_DURATION:
				Delay=(DWORD)pThis->m_StartTimeSpec.Time.Duration;
				::GetLocalTime(&st);
				st.wSecond=0;
				st.wMilliseconds=0;
				::SystemTimeToFileTime(&st,&ftTime);
				ftTime+=(LONGLONG)Delay*FILETIME_MILLISECOND;
				break;
			}
			SetDateTimeFormat(hDlg,IDC_RECORD_STARTTIME_TIME);
			DateTime_SetFiletime(GetDlgItem(hDlg,IDC_RECORD_STARTTIME_TIME),
															GDT_VALID,&ftTime);
			Delay/=1000;
			::SetDlgItemInt(hDlg,IDC_RECORD_STARTTIME_HOUR,Delay/(60*60),FALSE);
			::SendDlgItemMessage(hDlg,IDC_RECORD_STARTTIME_HOUR_UD,UDM_SETRANGE,
														0,MAKELPARAM(100,0));
			::SetDlgItemInt(hDlg,IDC_RECORD_STARTTIME_MINUTE,Delay/60%60,FALSE);
			::SendDlgItemMessage(hDlg,IDC_RECORD_STARTTIME_MINUTE_UD,UDM_SETRANGE,
														0,MAKELPARAM(60,0));
			::SetDlgItemInt(hDlg,IDC_RECORD_STARTTIME_SECOND,Delay%60,FALSE);
			::SendDlgItemMessage(hDlg,IDC_RECORD_STARTTIME_SECOND_UD,UDM_SETRANGE,
														0,MAKELPARAM(60,0));
			if (pThis->m_fRecording) {
				::EnableDlgItems(hDlg,IDC_RECORD_STARTTIME,IDC_RECORD_STARTTIME_SECOND_LABEL,false);
			} else {
				EnableDlgItem(hDlg,IDC_RECORD_STARTTIME_TIME,
									pThis->m_StartTimeSpec.Type==TIME_DATETIME);
				EnableDlgItems(hDlg,IDC_RECORD_STARTTIME_HOUR,
									IDC_RECORD_STARTTIME_SECOND_LABEL,
									pThis->m_StartTimeSpec.Type==TIME_DURATION);
			}

			// 終了時間
			DWORD Duration;
			::CheckDlgButton(hDlg,IDC_RECORD_STOPSPECTIME,
				pThis->m_StopTimeSpec.Type!=TIME_NOTSPECIFIED?BST_CHECKED:BST_UNCHECKED);
			::CheckRadioButton(hDlg,IDC_RECORD_STOPDATETIME,
													IDC_RECORD_STOPREMAINTIME,
				pThis->m_StopTimeSpec.Type==TIME_DATETIME?
							IDC_RECORD_STOPDATETIME:IDC_RECORD_STOPREMAINTIME);
			switch (pThis->m_StopTimeSpec.Type) {
			case TIME_NOTSPECIFIED:
				::GetLocalTime(&st);
				st.wSecond=0;
				st.wMilliseconds=0;
				::SystemTimeToFileTime(&st,&ftTime);
				ftTime+=60*60*FILETIME_SECOND;
				Duration=60*60*1000;
				break;
			case TIME_DATETIME:
				{
					FILETIME ft;

					GetLocalTimeAsFileTime(&ft);
					ftTime=pThis->m_StopTimeSpec.Time.DateTime;
					if (::CompareFileTime(&ft,&ftTime)<0)
						Duration=(DWORD)((ftTime-ft)/FILETIME_MILLISECOND);
					else
						Duration=0;
				}
				break;
			case TIME_DURATION:
				Duration=(DWORD)pThis->m_StopTimeSpec.Time.Duration;
				::GetLocalTime(&st);
				st.wSecond=0;
				st.wMilliseconds=0;
				::SystemTimeToFileTime(&st,&ftTime);
				ftTime+=(LONGLONG)Duration*FILETIME_MILLISECOND;
				break;
			}
			SetDateTimeFormat(hDlg,IDC_RECORD_STOPTIME_TIME);
			DateTime_SetFiletime(GetDlgItem(hDlg,IDC_RECORD_STOPTIME_TIME),
															GDT_VALID,&ftTime);
			::SetDlgItemInt(hDlg,IDC_RECORD_STOPTIME_HOUR,
												Duration/(60*60*1000),FALSE);
			::SendDlgItemMessage(hDlg,IDC_RECORD_STOPTIME_HOUR_UD,UDM_SETRANGE,
														0,MAKELPARAM(100,0));
			::SetDlgItemInt(hDlg,IDC_RECORD_STOPTIME_MINUTE,
												Duration/(60*1000)%60,FALSE);
			::SendDlgItemMessage(hDlg,IDC_RECORD_STOPTIME_MINUTE_UD,UDM_SETRANGE,
														0,MAKELPARAM(60,0));
			::SetDlgItemInt(hDlg,IDC_RECORD_STOPTIME_SECOND,
												Duration/1000%60,FALSE);
			::SendDlgItemMessage(hDlg,IDC_RECORD_STOPTIME_SECOND_UD,UDM_SETRANGE,
														0,MAKELPARAM(60,0));
			if (pThis->m_StopTimeSpec.Type==TIME_NOTSPECIFIED) {
				EnableDlgItems(hDlg,IDC_RECORD_STOPDATETIME,
									IDC_RECORD_STOPTIME_SECOND_LABEL,false);
			} else {
				EnableDlgItem(hDlg,IDC_RECORD_STOPTIME_TIME,
									pThis->m_StopTimeSpec.Type==TIME_DATETIME);
				EnableDlgItems(hDlg,IDC_RECORD_STOPTIME_HOUR,
									IDC_RECORD_STOPTIME_SECOND_LABEL,
									pThis->m_StopTimeSpec.Type==TIME_DURATION);
#if 0
				if (pThis->m_fRecording
						&& pThis->m_StopTimeSpec.Type==TIME_DURATION)
					::SetTimer(hDlg,1,500,NULL);
#endif
			}

			DlgCheckBox_Check(hDlg,IDC_RECORD_CURSERVICEONLY,pThis->m_fCurServiceOnly);
			DlgCheckBox_Check(hDlg,IDC_RECORD_SAVESUBTITLE,
				(pThis->m_SaveStream&CTsSelector::STREAM_CAPTION)!=0);
			DlgCheckBox_Check(hDlg,IDC_RECORD_SAVEDATACARROUSEL,
				(pThis->m_SaveStream&CTsSelector::STREAM_DATACARROUSEL)!=0);
			if (pThis->m_fRecording) {
				EnableDlgItems(hDlg,IDC_RECORD_CURSERVICEONLY,IDC_RECORD_SAVEDATACARROUSEL,false);
			} else {
				/*
				EnableDlgItems(hDlg,IDC_RECORD_SAVESUBTITLE,IDC_RECORD_SAVEDATACARROUSEL,
							   pThis->m_fCurServiceOnly);
				*/
			}
			EnableDlgItem(hDlg,IDC_RECORD_CANCEL,pThis->m_fReserved);
		}
		return TRUE;

#if 0
	case WM_TIMER:
		{
			CRecordManager *pThis=GetThis(hDlg);
			const LONGLONG Remain=pThis->GetRemainTime();

			UpdateDlgItemInt(hDlg,IDC_RECORD_STOPTIME_HOUR,(int)(Remain/(60*60*1000)));
			UpdateDlgItemInt(hDlg,IDC_RECORD_STOPTIME_MINUTE,(int)(Remain/(60*1000))%60);
			UpdateDlgItemInt(hDlg,IDC_RECORD_STOPTIME_SECOND,(int)(Remain/1000)%60);
		}
		return TRUE;
#endif

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_RECORD_FILENAME_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH];

				GetDlgItemText(hDlg,IDC_RECORD_FILENAME,szFileName,MAX_PATH);
				ofn.lStructSize=sizeof(OPENFILENAME);
				ofn.hwndOwner=hDlg;
				ofn.lpstrFilter=
					TEXT("TSファイル(*.ts)\0*.ts\0すべてのファイル\0*.*\0");
				ofn.lpstrCustomFilter=NULL;
				ofn.nFilterIndex=1;
				ofn.lpstrFile=szFileName;
				ofn.nMaxFile=MAX_PATH;
				ofn.lpstrFileTitle=NULL;
				ofn.lpstrInitialDir=NULL;
				ofn.lpstrTitle=TEXT("保存ファイル名");
				ofn.Flags=OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
				ofn.lpstrDefExt=TEXT("ts");
#if _WIN32_WINNT>=0x500
				ofn.pvReserved=NULL;
				ofn.dwReserved=0;
				ofn.FlagsEx=0;
#endif
				if (GetSaveFileName(&ofn))
					SetDlgItemText(hDlg,IDC_RECORD_FILENAME,szFileName);
			}
			return TRUE;

		case IDC_RECORD_FILENAME:
			if (HIWORD(wParam)==EN_CHANGE) {
				CRecordManager *pThis=GetThis(hDlg);
				if (pThis==NULL)
					break;
				TCHAR szFormat[MAX_PATH],szFileName[MAX_PATH];

				::GetDlgItemText(hDlg,IDC_RECORD_FILENAME,szFormat,lengthof(szFormat));
				szFileName[0]='\0';
				if (szFormat[0]!='\0') {
					EventInfo Info;

					GetEventInfoSample(&Info);
					if (!pThis->GenerateFileName(szFileName,lengthof(szFileName),
												 &Info,szFormat))
						szFileName[0]='\0';
				}
				::SetDlgItemText(hDlg,IDC_RECORD_FILENAMEPREVIEW,szFileName);
			}
			return TRUE;

		case IDC_RECORD_FILENAMEFORMAT:
			{
				RECT rc;
				POINT pt;

				::GetWindowRect(::GetDlgItem(hDlg,IDC_RECORD_FILENAMEFORMAT),&rc);
				pt.x=rc.left;
				pt.y=rc.bottom;
				InsertFileNameParameter(hDlg,IDC_RECORD_FILENAME,&pt);
			}
			return TRUE;

		case IDC_RECORD_START_NOW:
		case IDC_RECORD_START_DATETIME:
		case IDC_RECORD_START_DELAY:
			EnableDlgItem(hDlg,IDC_RECORD_STARTTIME_TIME,
				DlgCheckBox_IsChecked(hDlg,IDC_RECORD_START_DATETIME));
			EnableDlgItems(hDlg,IDC_RECORD_STARTTIME_HOUR,
								IDC_RECORD_STARTTIME_SECOND_LABEL,
				DlgCheckBox_IsChecked(hDlg,IDC_RECORD_START_DELAY));
			return TRUE;

		case IDC_RECORD_STOPSPECTIME:
		case IDC_RECORD_STOPDATETIME:
		case IDC_RECORD_STOPREMAINTIME:
			if (DlgCheckBox_IsChecked(hDlg,IDC_RECORD_STOPSPECTIME)) {
				bool fDateTime=IsDlgButtonChecked(hDlg,
										IDC_RECORD_STOPDATETIME)==BST_CHECKED;

				EnableDlgItems(hDlg,IDC_RECORD_STOPDATETIME,
									IDC_RECORD_STOPREMAINTIME,TRUE);
				EnableDlgItem(hDlg,IDC_RECORD_STOPTIME_TIME,fDateTime);
				EnableDlgItems(hDlg,IDC_RECORD_STOPTIME_HOUR,
									IDC_RECORD_STOPTIME_SECOND_LABEL,!fDateTime);
			} else {
				EnableDlgItems(hDlg,IDC_RECORD_STOPDATETIME,
									IDC_RECORD_STOPTIME_SECOND_LABEL,false);
			}
			return TRUE;

#if 0
		case IDC_RECORD_STOPTIME_HOUR:
		case IDC_RECORD_STOPTIME_MINUTE:
		case IDC_RECORD_STOPTIME_SECOND:
			if (HIWORD(wParam)==EN_CHANGE) {
				::KillTimer(hDlg,1);
			}
			return TRUE;
#endif

		/*
		case IDC_RECORD_CURSERVICEONLY:
			EnableDlgItems(hDlg,IDC_RECORD_SAVESUBTITLE,IDC_RECORD_SAVEDATACARROUSEL,
				DlgCheckBox_IsChecked(hDlg,IDC_RECORD_CURSERVICEONLY));
			return TRUE;
		*/

		case IDOK:
			{
				CRecordManager *pThis=GetThis(hDlg);
				int StartTimeChecked;
				SYSTEMTIME st;
				FILETIME ftCur,ftStart,ftStop;

				GetLocalTimeAsFileTime(&ftCur);
				if (!pThis->m_fRecording) {
					StartTimeChecked=GetCheckedRadioButton(hDlg,IDC_RECORD_START_NOW,IDC_RECORD_START_DELAY);
					if (StartTimeChecked==IDC_RECORD_START_DATETIME) {
						DateTime_GetSystemtime(
							GetDlgItem(hDlg,IDC_RECORD_STARTTIME_TIME),&st);
						::SystemTimeToFileTime(&st,&ftStart);
						if (::CompareFileTime(&ftStart,&ftCur)<=0) {
							::MessageBox(hDlg,
								TEXT("指定された開始時刻を既に過ぎています。"),
								NULL,MB_OK | MB_ICONEXCLAMATION);
							SetDlgItemFocus(hDlg,IDC_RECORD_STARTTIME_TIME);
							return TRUE;
						}
					}
				}
				if (DlgCheckBox_IsChecked(hDlg,IDC_RECORD_STOPSPECTIME)
						&& DlgCheckBox_IsChecked(hDlg,IDC_RECORD_STOPDATETIME)) {
					DateTime_GetSystemtime(
						::GetDlgItem(hDlg,IDC_RECORD_STOPTIME_TIME),&st);
					::SystemTimeToFileTime(&st,&ftStop);
					if (::CompareFileTime(&ftStop,&ftCur)<=0) {
						::MessageBox(hDlg,
							TEXT("指定された停止時刻を既に過ぎています。"),
							NULL,MB_OK | MB_ICONEXCLAMATION);
						SetDlgItemFocus(hDlg,IDC_RECORD_STOPTIME_TIME);
						return TRUE;
					}
				}
				if (!pThis->m_fRecording) {
					TCHAR szFileName[MAX_PATH];

					GetDlgItemText(hDlg,IDC_RECORD_FILENAME,szFileName,MAX_PATH);
					CFilePath FilePath(szFileName);
					if (szFileName[0]=='\0' || *FilePath.GetFileName()=='\0') {
						MessageBox(hDlg,TEXT("ファイル名を入力してください。"),
											NULL,MB_OK | MB_ICONEXCLAMATION);
						SetDlgItemFocus(hDlg,IDC_RECORD_FILENAME);
						return TRUE;
					}
#if 0
					if (!FilePath.IsValid()) {
						MessageBox(hDlg,
							TEXT("ファイル名に使用できない文字が含まれています。"),
							NULL,MB_OK | MB_ICONEXCLAMATION);
						SetDlgItemFocus(hDlg,IDC_RECORD_FILENAME);
						return TRUE;
					}
#else
					TCHAR szMessage[256];
					if (!IsValidFileName(FilePath.GetFileName(),false,
										 szMessage,lengthof(szMessage))) {
						MessageBox(hDlg,szMessage,NULL,MB_OK | MB_ICONEXCLAMATION);
						SetDlgItemFocus(hDlg,IDC_RECORD_FILENAME);
						return TRUE;
					}
#endif
					if (!FilePath.HasDirectory()) {
						MessageBox(hDlg,TEXT("保存先フォルダを入力してください。"),
											NULL,MB_OK | MB_ICONEXCLAMATION);
						SetDlgItemFocus(hDlg,IDC_RECORD_FILENAME);
						return TRUE;
					}
					FilePath.GetDirectory(szFileName);
					if (!::PathIsDirectory(szFileName)) {
						TCHAR szMessage[MAX_PATH+64];

						StdUtil::snprintf(szMessage,lengthof(szMessage),
							TEXT("録画ファイルの保存先フォルダ \"%s\" がありません。\n")
							TEXT("作成しますか?"),szFileName);
						if (::MessageBox(hDlg,szMessage,TEXT("フォルダ作成の確認"),
										 MB_YESNO | MB_ICONQUESTION)==IDYES) {
							int Result;

							Result=::SHCreateDirectoryEx(hDlg,szFileName,NULL);
							if (Result!=ERROR_SUCCESS
									&& Result!=ERROR_ALREADY_EXISTS) {
								::MessageBox(hDlg,TEXT("フォルダが作成できません。"),
											 NULL,MB_OK | MB_ICONEXCLAMATION);
								SetDlgItemFocus(hDlg,IDC_RECORD_FILENAME);
								return TRUE;
							}
						}
					}
					pThis->SetFileName(FilePath.GetPath());
					/*
					pThis->m_ExistsOperation=(FileExistsOperation)
						SendDlgItemMessage(hDlg,IDC_RECORD_FILEEXISTS,CB_GETCURSEL,0,0);
					*/
					switch (StartTimeChecked) {
					case IDC_RECORD_START_NOW:
					default:
						pThis->SetStartTimeSpec(NULL);
						break;
					case IDC_RECORD_START_DATETIME:
						{
							TimeSpecInfo TimeSpec;

							TimeSpec.Type=TIME_DATETIME;
							TimeSpec.Time.DateTime=ftStart;
							pThis->SetStartTimeSpec(&TimeSpec);
						}
						break;
					case IDC_RECORD_START_DELAY:
						{
							TimeSpecInfo TimeSpec;
							unsigned int Hour,Minute,Second;

							TimeSpec.Type=TIME_DURATION;
							Hour=GetDlgItemInt(hDlg,IDC_RECORD_STARTTIME_HOUR,NULL,FALSE);
							Minute=GetDlgItemInt(hDlg,IDC_RECORD_STARTTIME_MINUTE,NULL,FALSE);
						Second=GetDlgItemInt(hDlg,IDC_RECORD_STARTTIME_SECOND,NULL,FALSE);
							TimeSpec.Time.Duration=(Hour*(60*60)+Minute*60+Second)*1000;
							pThis->SetStartTimeSpec(&TimeSpec);
						}
						break;
					}

					pThis->m_fCurServiceOnly=DlgCheckBox_IsChecked(hDlg,IDC_RECORD_CURSERVICEONLY);
					pThis->m_SaveStream=CTsSelector::STREAM_ALL;
					if (!DlgCheckBox_IsChecked(hDlg,IDC_RECORD_SAVESUBTITLE))
						pThis->m_SaveStream^=CTsSelector::STREAM_CAPTION;
					if (!DlgCheckBox_IsChecked(hDlg,IDC_RECORD_SAVEDATACARROUSEL))
						pThis->m_SaveStream^=CTsSelector::STREAM_DATACARROUSEL;
				}

				if (DlgCheckBox_IsChecked(hDlg,IDC_RECORD_STOPSPECTIME)) {
					TimeSpecInfo TimeSpec;

					if (DlgCheckBox_IsChecked(hDlg,IDC_RECORD_STOPDATETIME)) {
						TimeSpec.Type=TIME_DATETIME;
						TimeSpec.Time.DateTime=ftStop;
					} else {
						unsigned int Hour,Minute,Second;

						TimeSpec.Type=TIME_DURATION;
						Hour=GetDlgItemInt(hDlg,IDC_RECORD_STOPTIME_HOUR,NULL,FALSE);
						Minute=GetDlgItemInt(hDlg,IDC_RECORD_STOPTIME_MINUTE,NULL,FALSE);
						Second=GetDlgItemInt(hDlg,IDC_RECORD_STOPTIME_SECOND,NULL,FALSE);
						TimeSpec.Time.Duration=(Hour*(60*60)+Minute*60+Second)*1000;
					}
					pThis->SetStopTimeSpec(&TimeSpec);
				} else {
					pThis->SetStopTimeSpec(NULL);
				}
			}
		case IDCANCEL:
			EndDialog(hDlg,LOWORD(wParam));
			return TRUE;

		case IDC_RECORD_CANCEL:
			{
				CRecordManager *pThis=GetThis(hDlg);

				pThis->CancelReserve();
				EndDialog(hDlg,IDCANCEL);
			}
			return TRUE;
		}
		return TRUE;

	case WM_DESTROY:
		::RemoveProp(hDlg,TEXT("This"));
		return TRUE;
	}
	return FALSE;
}

#pragma warning(default: 4700)


bool CRecordManager::RecordDialog(HWND hwndOwner)
{
	return DialogBoxParam(GetAppClass().GetResourceInstance(),
						  MAKEINTRESOURCE(IDD_RECORDOPTION),hwndOwner,
						  DlgProc,reinterpret_cast<LPARAM>(this))==IDOK;
}


#if 0

INT_PTR CALLBACK CRecordManager::StopTimeDlgProc(HWND hDlg,UINT uMsg,
												WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CRecordManager *pThis=reinterpret_cast<CRecordManager*>(lParam);
			FILETIME ft;;

			SetProp(hDlg,TEXT("This"),pThis);
			CheckDlgButton(hDlg,IDC_RECORDSTOPTIME_ENABLE,
				pThis->m_StopTimeSpec.Type!=TIME_NOTSPECIFIED?BST_CHECKED:BST_UNCHECKED);
			EnableDlgItem(hDlg,IDC_RECORDSTOPTIME_TIME,
								pThis->m_StopTimeSpec.Type!=TIME_NOTSPECIFIED);
			SetDateTimeFormat(hDlg,IDC_RECORDSTOPTIME_TIME);
			if (pThis->m_StopTimeSpec.Type==TIME_DATETIME) {
				ft=pThis->m_StopTimeSpec.Time.DateTime;
			} else if (pThis->m_StopTimeSpec.Type==TIME_DURATION) {
				pThis->GetStartTime(&ft);
				ft+=(LONGLONG)pThis->m_StopTimeSpec.Time.Duration*FILETIME_MILLISECOND;
			}
			DateTime_SetFiletime(GetDlgItem(hDlg,IDC_RECORDSTOPTIME_TIME),
																GDT_VALID,&ft);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_RECORDSTOPTIME_ENABLE:
			EnableDlgItem(hDlg,IDC_RECORDSTOPTIME_TIME,
				IsDlgButtonChecked(hDlg,IDC_RECORDSTOPTIME_ENABLE)==BST_CHECKED);
			return TRUE;

		case IDOK:
			{
				CRecordManager *pThis=GetThis(hDlg);
				bool fStopTimeSpec;
				SYSTEMTIME st;

				fStopTimeSpec=IsDlgButtonChecked(hDlg,
									IDC_RECORDSTOPTIME_ENABLE)==BST_CHECKED;
				if (fStopTimeSpec) {
					FILETIME ft,ftCur;
					TimeSpecInfo TimeSpec;

					if (DateTime_GetSystemtime(
							GetDlgItem(hDlg,IDC_RECORDSTOPTIME_TIME),&st)!=
																GDT_VALID) {
						MessageBox(hDlg,TEXT("時間の取得エラー。"),NULL,
												MB_OK | MB_ICONEXCLAMATION);
						return TRUE;
					}
					SystemTimeToFileTime(&st,&ft);
					GetLocalTime(&st);
					SystemTimeToFileTime(&st,&ftCur);
					if (CompareFileTime(&ft,&ftCur)<=0) {
						MessageBox(hDlg,
								TEXT("指定された停止時間を既に過ぎています。"),
											NULL,MB_OK | MB_ICONEXCLAMATION);
						SetFocus(GetDlgItem(hDlg,IDC_RECORDSTOPTIME_TIME));
						return TRUE;
					}
					TimeSpec.Type=TIME_DATETIME;
					TimeSpec.Time.DateTime=ft;
					pThis->SetStopTimeSpec(&TimeSpec);
				} else {
					pThis->SetStopTimeSpec(NULL);
				}
			}
		case IDCANCEL:
			EndDialog(hDlg,LOWORD(wParam));
		}
		return TRUE;

	case WM_DESTROY:
		::RemoveProp(hDlg,TEXT("This"));
		return TRUE;
	}
	return FALSE;
}


bool CRecordManager::ChangeStopTimeDialog(HWND hwndOwner)
{
	return DialogBoxParam(GetAppClass().GetResourceInstance(),
						  MAKEINTRESOURCE(IDD_RECORDSTOPTIME),hwndOwner,
						  StopTimeDlgProc,reinterpret_cast<LPARAM>(this))==IDOK;
}

#endif


int CRecordManager::FormatFileName(LPTSTR pszFileName,int MaxFileName,const EventInfo *pEventInfo,LPCTSTR pszFormat) const
{
	SYSTEMTIME stStart;
	LPCTSTR p;
	int i;

	if (m_fReserved && m_StartTimeSpec.Type==TIME_DATETIME) {
		::FileTimeToSystemTime(&m_StartTimeSpec.Time.DateTime,&stStart);
	} else {
		::GetLocalTime(&stStart);
	}
	p=pszFormat;
	for (i=0;i<MaxFileName-1 && *p!=_T('\0');) {
		if (*p==_T('%')) {
			p++;
			if (*p==_T('%')) {
				pszFileName[i++]=_T('%');
				p++;
			} else {
				TCHAR szKeyword[32];
				size_t j;

				for (j=0;p[j]!=_T('%') && p[j]!=_T('\0');j++) {
					if (j<lengthof(szKeyword)-1)
						szKeyword[j]=p[j];
				}
				if (j<=lengthof(szKeyword)-1 && p[j]==_T('%')) {
					const int Remain=MaxFileName-i;

					p+=j+1;
					szKeyword[j]=_T('\0');
					if (::lstrcmpi(szKeyword,TEXT("date"))==0) {
						i+=StdUtil::snprintf(&pszFileName[i],Remain,TEXT("%d%02d%02d"),
											 stStart.wYear,stStart.wMonth,stStart.wDay);
					} else if (::lstrcmpi(szKeyword,TEXT("year"))==0) {
						i+=StdUtil::snprintf(&pszFileName[i],Remain,TEXT("%d"),stStart.wYear);
					} else if (::lstrcmpi(szKeyword,TEXT("year2"))==0) {
						i+=StdUtil::snprintf(&pszFileName[i],Remain,TEXT("%02d"),stStart.wYear%100);
					} else if (::lstrcmpi(szKeyword,TEXT("month"))==0) {
						i+=StdUtil::snprintf(&pszFileName[i],Remain,TEXT("%d"),stStart.wMonth);
					} else if (::lstrcmpi(szKeyword,TEXT("month2"))==0) {
						i+=StdUtil::snprintf(&pszFileName[i],Remain,TEXT("%02d"),stStart.wMonth);
					} else if (::lstrcmpi(szKeyword,TEXT("day"))==0) {
						i+=StdUtil::snprintf(&pszFileName[i],Remain,TEXT("%d"),stStart.wDay);
					} else if (::lstrcmpi(szKeyword,TEXT("day2"))==0) {
						i+=StdUtil::snprintf(&pszFileName[i],Remain,TEXT("%02d"),stStart.wDay);
					} else if (::lstrcmpi(szKeyword,TEXT("time"))==0) {
						i+=StdUtil::snprintf(&pszFileName[i],Remain,TEXT("%02d%02d%02d"),
											 stStart.wHour,stStart.wMinute,stStart.wSecond);
					} else if (::lstrcmpi(szKeyword,TEXT("hour"))==0) {
						i+=StdUtil::snprintf(&pszFileName[i],Remain,TEXT("%d"),stStart.wHour);
					} else if (::lstrcmpi(szKeyword,TEXT("hour2"))==0) {
						i+=StdUtil::snprintf(&pszFileName[i],Remain,TEXT("%02d"),stStart.wHour);
					} else if (::lstrcmpi(szKeyword,TEXT("minute"))==0) {
						i+=StdUtil::snprintf(&pszFileName[i],Remain,TEXT("%d"),stStart.wMinute);
					} else if (::lstrcmpi(szKeyword,TEXT("minute2"))==0) {
						i+=StdUtil::snprintf(&pszFileName[i],Remain,TEXT("%02d"),stStart.wMinute);
					} else if (::lstrcmpi(szKeyword,TEXT("second"))==0) {
						i+=StdUtil::snprintf(&pszFileName[i],Remain,TEXT("%d"),stStart.wSecond);
					} else if (::lstrcmpi(szKeyword,TEXT("second2"))==0) {
						i+=StdUtil::snprintf(&pszFileName[i],Remain,TEXT("%02d"),stStart.wSecond);
					} else if (::lstrcmpi(szKeyword,TEXT("day-of-week"))==0) {
						i+=StdUtil::snprintf(&pszFileName[i],Remain,TEXT("%s"),
											 GetDayOfWeekText(stStart.wDayOfWeek));
					} else if (::lstrcmpi(szKeyword,TEXT("channel-name"))==0) {
						if (pEventInfo->pszChannelName!=NULL)
							i+=MapFileNameCopy(&pszFileName[i],Remain,pEventInfo->pszChannelName);
					} else if (::lstrcmpi(szKeyword,TEXT("channel-no"))==0) {
						if (pEventInfo->ChannelNo!=0)
							i+=StdUtil::snprintf(&pszFileName[i],Remain,TEXT("%d"),pEventInfo->ChannelNo);
					} else if (::lstrcmpi(szKeyword,TEXT("channel-no2"))==0) {
						if (pEventInfo->ChannelNo!=0)
							i+=StdUtil::snprintf(&pszFileName[i],Remain,TEXT("%02d"),pEventInfo->ChannelNo);
					} else if (::lstrcmpi(szKeyword,TEXT("channel-no3"))==0) {
						if (pEventInfo->ChannelNo!=0)
							i+=StdUtil::snprintf(&pszFileName[i],Remain,TEXT("%03d"),pEventInfo->ChannelNo);
					} else if (::lstrcmpi(szKeyword,TEXT("event-name"))==0) {
						if (pEventInfo->pszEventName!=NULL)
							i+=MapFileNameCopy(&pszFileName[i],Remain,pEventInfo->pszEventName);
					} else if (::lstrcmpi(szKeyword,TEXT("event-id"))==0) {
						i+=StdUtil::snprintf(&pszFileName[i],Remain,TEXT("%04X"),pEventInfo->EventID);
					} else if (::lstrcmpi(szKeyword,TEXT("service-name"))==0) {
						if (pEventInfo->pszServiceName!=NULL)
							i+=MapFileNameCopy(&pszFileName[i],Remain,pEventInfo->pszServiceName);
					} else if (::lstrcmpi(szKeyword,TEXT("service-id"))==0) {
						if (pEventInfo->ServiceID!=0)
							i+=StdUtil::snprintf(&pszFileName[i],Remain,TEXT("%04X"),pEventInfo->ServiceID);
					} else if (::lstrcmpi(szKeyword,TEXT("tot-date"))==0) {
						i+=StdUtil::snprintf(&pszFileName[i],Remain,TEXT("%d%02d%02d"),
							pEventInfo->stTotTime.wYear,pEventInfo->stTotTime.wMonth,pEventInfo->stTotTime.wDay);
					} else if (::lstrcmpi(szKeyword,TEXT("tot-time"))==0) {
						i+=StdUtil::snprintf(&pszFileName[i],Remain,TEXT("%02d%02d%02d"),
							pEventInfo->stTotTime.wHour,pEventInfo->stTotTime.wMinute,pEventInfo->stTotTime.wSecond);
					} else {
						TRACE(TEXT("Unknown keyword %%%s%%\n"),szKeyword);
						i+=StdUtil::snprintf(&pszFileName[i],Remain,TEXT("%%%s%%"),szKeyword);
					}
				} else {
					pszFileName[i++]=_T('%');
				}
			}
		} else {
#ifndef UNICODE
			if (::IsDBCSLeadByteEx(CP_ACP,*p)) {
				if (i+1==MaxFileName || *(p+1)==_T('\0'))
					break;
				pszFileName[i++]=*p++;
			}
#endif
			pszFileName[i++]=*p++;
		}
	}
	pszFileName[i]=_T('\0');
	return i;
}


int CRecordManager::MapFileNameCopy(LPWSTR pszFileName,int MaxFileName,LPCWSTR pszText)
{
	int i;
	LPCWSTR p=pszText;

	for (i=0;i<MaxFileName-1 && *p!='\0';i++) {
		static const struct {
			WCHAR From;
			WCHAR To;
		} CharMap[] = {
			{L'\\',	L'￥'},
			{L'/',	L'／'},
			{L':',	L'：'},
			{L'*',	L'＊'},
			{L'?',	L'？'},
			{L'"',	L'”'},
			{L'<',	L'＜'},
			{L'>',	L'＞'},
			{L'|',	L'｜'},
		};

		for (int j=0;j<lengthof(CharMap);j++) {
			if (CharMap[j].From==*p) {
				pszFileName[i]=CharMap[j].To;
				goto Next;
			}
		}
		pszFileName[i]=*p;
	Next:
		p++;
	}
	pszFileName[i]='\0';
	return i;
}


bool CRecordManager::GenerateFileName(LPTSTR pszFileName,int MaxLength,const EventInfo *pEventInfo,LPCTSTR pszFormat) const
{
	if (pszFormat==NULL) {
		if (m_FileName.IsEmpty())
			return false;
	}
	if (FormatFileName(pszFileName,MaxLength-5,pEventInfo,
					   pszFormat!=NULL?pszFormat:m_FileName.Get())==0)
		return false;
	if (pszFormat==NULL && ::PathFileExists(pszFileName)) {
		LPTSTR pszSeqNumber=::PathFindExtension(pszFileName);
		LPCTSTR pszExtension=::PathFindExtension(m_FileName.Get());

		for (int i=2;i<1000;i++) {
			::wsprintf(pszSeqNumber,TEXT("-%d%s"),i,pszExtension);
			if (!::PathFileExists(pszFileName))
				break;
			if (i==999)
				return false;
		}
	}
	return true;
}


/*
bool CRecordManager::DoFileExistsOperation(HWND hwndOwner,LPTSTR pszFileName)
{
	lstrcpy(pszFileName,m_FileName.Get());
	switch (m_ExistsOperation) {
	case EXISTS_CONFIRM:
		if (PathFileExists(m_FileName.Get())
				&& MessageBox(hwndOwner,
					TEXT("ファイルが既に存在します。\n上書きしますか?"),
					TEXT("上書きの確認"),MB_OKCANCEL | MB_ICONQUESTION)!=IDOK)
			return false;
		break;
	case EXISTS_SEQUENCIALNUMBER:
		if (PathFileExists(m_FileName.Get())) {
			int i;
			TCHAR szFileName[MAX_PATH];
			LPTSTR pszExtension,p;

			pszExtension=PathFindExtension(m_FileName.Get());
			lstrcpy(szFileName,m_FileName.Get());
			p=PathFindExtension(szFileName);
			for (i=0;;i++) {
				wsprintf(p,TEXT("%d%s"),i+1,pszExtension);
				if (!PathFileExists(szFileName))
					break;
			}
			lstrcpy(pszFileName,szFileName);
		}
		break;
	}
	return true;
}
*/


bool CRecordManager::SetCurServiceOnly(bool fOnly)
{
	m_fCurServiceOnly=fOnly;
	return true;
}


bool CRecordManager::SetSaveStream(DWORD Stream)
{
	m_SaveStream=Stream;
	return true;
}


bool CRecordManager::SetBufferSize(SIZE_T BufferSize)
{
	m_BufferSize=BufferSize;
	return true;
}


bool CRecordManager::InsertFileNameParameter(HWND hDlg,int ID,const POINT *pMenuPos)
{
	static const struct {
		LPCTSTR pszParameter;
		LPCTSTR pszText;
	} ParameterList[] = {
		{TEXT("%date%"),			TEXT("開始年月日")},
		{TEXT("%year%"),			TEXT("開始年")},
		{TEXT("%year2%"),			TEXT("開始年(下2桁)")},
		{TEXT("%month%"),			TEXT("開始月")},
		{TEXT("%month2%"),			TEXT("開始月(2桁)")},
		{TEXT("%day%"),				TEXT("開始日")},
		{TEXT("%day2%"),			TEXT("開始日(2桁)")},
		{TEXT("%time%"),			TEXT("開始時刻(時+分+秒)")},
		{TEXT("%hour%"),			TEXT("開始時")},
		{TEXT("%hour2%"),			TEXT("開始時(2桁)")},
		{TEXT("%minute%"),			TEXT("開始分")},
		{TEXT("%minute2%"),			TEXT("開始分(2桁)")},
		{TEXT("%second%"),			TEXT("開始秒")},
		{TEXT("%second2%"),			TEXT("開始秒(2桁)")},
		{TEXT("%day-of-week%"),		TEXT("開始曜日(漢字)")},
		{TEXT("%channel-name%"),	TEXT("チャンネル名")},
		{TEXT("%channel-no%"),		TEXT("チャンネル番号")},
		{TEXT("%channel-no2%"),		TEXT("チャンネル番号(2桁)")},
		{TEXT("%channel-no3%"),		TEXT("チャンネル番号(3桁)")},
		{TEXT("%event-name%"),		TEXT("イベント名")},
		{TEXT("%event-id%"),		TEXT("イベントID")},
		{TEXT("%service-name%"),	TEXT("サービス名")},
		{TEXT("%service-id%"),		TEXT("サービスID")},
		{TEXT("%tot-date%"),		TEXT("TOT年月日")},
		{TEXT("%tot-time%"),		TEXT("TOT時刻(時+分+秒)")},
	};
	HMENU hmenu=::CreatePopupMenu();
	int Command;

	for (int i=0;i<lengthof(ParameterList);i++) {
		TCHAR szText[128];

		StdUtil::snprintf(szText,lengthof(szText),TEXT("%s (%s)"),
						  ParameterList[i].pszParameter,ParameterList[i].pszText);
		::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,i+1,szText);
	}
	Command=::TrackPopupMenu(hmenu,TPM_RETURNCMD,pMenuPos->x,pMenuPos->y,0,hDlg,NULL);
	::DestroyMenu(hmenu);
	if (Command<=0)
		return false;

	DWORD Start,End;
	::SendDlgItemMessage(hDlg,ID,EM_GETSEL,
		reinterpret_cast<WPARAM>(&Start),reinterpret_cast<LPARAM>(&End));
	::SendDlgItemMessage(hDlg,ID,EM_REPLACESEL,
		TRUE,reinterpret_cast<LPARAM>(ParameterList[Command-1].pszParameter));
	if (End<Start)
		Start=End;
	::SendDlgItemMessage(hDlg,ID,EM_SETSEL,
		Start,Start+::lstrlen(ParameterList[Command-1].pszParameter));
	return true;
}


void CRecordManager::GetEventInfoSample(EventInfo *pEventInfo)
{
	pEventInfo->pszChannelName=TEXT("アフリカ中央テレビ");
	pEventInfo->ChannelNo=13;
	pEventInfo->pszServiceName=TEXT("アフテレ1");
	pEventInfo->ServiceID=0x1234;
	pEventInfo->pszEventName=TEXT("今日のニュース");
	pEventInfo->EventID=0xABCD;
	::GetLocalTime(&pEventInfo->stTotTime);
	pEventInfo->stTotTime.wSecond=pEventInfo->stTotTime.wSecond/5*5;
	pEventInfo->stTotTime.wMilliseconds=0;
}
