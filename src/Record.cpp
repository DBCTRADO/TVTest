#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "Record.h"
#include "DialogUtil.h"
#include "StdUtil.h"
#include "LibISDB/LibISDB/Windows/Base/EDCBPluginWriter.hpp"
#include "resource.h"
#include <algorithm>
#include "Common/DebugDef.h"




CRecordingSettings::CRecordingSettings()
	: m_fCurServiceOnly(false)
	, m_SaveStream(LibISDB::StreamSelector::StreamFlag::All)
	, m_WriteCacheSize(WRITE_CACHE_SIZE_DEFAULT)
	, m_MaxPendingSize(MAX_PENDING_SIZE_DEFAULT)
	, m_PreAllocationUnit(0)
	, m_TimeShiftBufferSize(TIMESHIFT_BUFFER_SIZE_DEFAULT)
	, m_fEnableTimeShift(false)
{
}


bool CRecordingSettings::IsSaveCaption() const
{
	return TestSaveStreamFlag(LibISDB::StreamSelector::StreamFlag::Caption);
}


void CRecordingSettings::SetSaveCaption(bool fSave)
{
	SetSaveStreamFlag(LibISDB::StreamSelector::StreamFlag::Caption,fSave);
}


bool CRecordingSettings::IsSaveDataCarrousel() const
{
	return TestSaveStreamFlag(LibISDB::StreamSelector::StreamFlag::DataCarrousel);
}


void CRecordingSettings::SetSaveDataCarrousel(bool fSave)
{
	SetSaveStreamFlag(LibISDB::StreamSelector::StreamFlag::DataCarrousel,fSave);
}


bool CRecordingSettings::TestSaveStreamFlag(LibISDB::StreamSelector::StreamFlag Flag) const
{
	return (m_SaveStream & Flag)==Flag;
}


void CRecordingSettings::SetSaveStreamFlag(LibISDB::StreamSelector::StreamFlag Flag,bool fSet)
{
	if (fSet)
		m_SaveStream|=Flag;
	else
		m_SaveStream^=Flag;
}




CRecordTime::CRecordTime()
{
	Clear();
}


bool CRecordTime::SetCurrentTime()
{
	::GetSystemTime(&m_Time);
	m_TickTime=Util::GetTickCount();
	return true;
}


bool CRecordTime::GetTime(SYSTEMTIME *pTime) const
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
	return m_Time.wYear!=0;
}




CRecordTask::CRecordTask()
	: m_State(STATE_STOP)
	, m_pTSEngine(NULL)
	, m_pRecorderFilter(NULL)
{
}


CRecordTask::~CRecordTask()
{
	Stop();
}


void CRecordTask::SetRecorderFilter(
	LibISDB::TSEngine *pTSEngine,
	LibISDB::RecorderFilter *pRecorderFilter)
{
	m_pTSEngine=pTSEngine;
	m_pRecorderFilter=pRecorderFilter;
}


bool CRecordTask::UpdateRecordingSettings(const CRecordingSettings &Settings)
{
	if (!m_RecordingTask)
		return false;

	LibISDB::RecorderFilter::RecordingOptions Options=m_RecordingTask->GetOptions();

	Options.ServiceID=
		Settings.m_fCurServiceOnly?m_pTSEngine->GetServiceID():LibISDB::SERVICE_ID_INVALID;
	Options.FollowActiveService=Settings.m_fCurServiceOnly;
	Options.StreamFlags=Settings.m_SaveStream;
	Options.MaxPendingSize=
		m_State==STATE_STOP?Settings.m_TimeShiftBufferSize:Settings.m_MaxPendingSize;

	return m_RecordingTask->SetOptions(Options);
}


bool CRecordTask::Start(LPCTSTR pszFileName,const CRecordingSettings &Settings)
{
	if (m_State!=STATE_STOP) {
		SetError(std::errc::operation_not_permitted,TEXT("呼び出しが不正です。"));
		return false;
	}
	if (m_pTSEngine==NULL || m_pRecorderFilter==NULL) {
		SetError(std::errc::operation_not_permitted,TEXT("呼び出しが不正です。"));
		return false;
	}

	LibISDB::RecorderFilter::RecordingOptions RecOptions;

	RecOptions.ServiceID=
		Settings.m_fCurServiceOnly?m_pTSEngine->GetServiceID():LibISDB::SERVICE_ID_INVALID;
	RecOptions.FollowActiveService=Settings.m_fCurServiceOnly;
	RecOptions.StreamFlags=Settings.m_SaveStream;
	RecOptions.WriteCacheSize=Settings.m_WriteCacheSize;

	LibISDB::StreamWriter *pWriter=NULL;

	if (pszFileName!=NULL) {
		// ファイルへの保存を開始

		RecOptions.MaxPendingSize=Settings.m_MaxPendingSize;
		if (Settings.m_fEnableTimeShift && m_RecordingTask)
			RecOptions.MaxPendingSize+=Settings.m_TimeShiftBufferSize;

		if (!Settings.m_WritePlugin.empty()) {
			TVTest::String WritePlugin;
			if (::PathIsFileSpec(Settings.m_WritePlugin.c_str())) {
				TCHAR szPath[MAX_PATH];
				GetAppClass().GetAppDirectory(szPath);
				if (::PathAppend(szPath,Settings.m_WritePlugin.c_str()))
					WritePlugin=szPath;
			} else {
				WritePlugin=Settings.m_WritePlugin;
			}
			if (!WritePlugin.empty()) {
				LibISDB::EDCBPluginWriter *pPluginWriter=new LibISDB::EDCBPluginWriter;
				// 出力プラグインがロードできなければプラグインを使わないようにする
				if (pPluginWriter->Load(WritePlugin)) {
					pWriter=pPluginWriter;
				} else {
					GetAppClass().AddLog(
						CLogItem::TYPE_WARNING,
						TEXT("出力プラグイン \"%s\" がロードできないため、TS出力を行います。"),
						WritePlugin.c_str());
					delete pPluginWriter;
				}
			}
		}

		if (pWriter==NULL) {
			pWriter=new LibISDB::FileStreamWriter;
		}

		pWriter->SetPreallocationUnit(Settings.m_PreAllocationUnit);

		if (!pWriter->Open(pszFileName)) {
			SetError(pWriter->GetLastErrorDescription());
			delete pWriter;
			return false;
		}
	} else {
		// さかのぼり用のバッファリングを開始

		if (m_RecordingTask || !Settings.m_fEnableTimeShift) {
			SetError(std::errc::operation_not_permitted,TEXT("呼び出しが不正です。"));
			return false;
		}

		RecOptions.MaxPendingSize=Settings.m_TimeShiftBufferSize;
	}

	if (m_RecordingTask) {
		if (!Settings.m_fEnableTimeShift)
			m_RecordingTask->ClearBuffer();
		m_RecordingTask->SetOptions(RecOptions);
		m_RecordingTask->SetWriter(pWriter);
	} else {
		m_RecordingTask=m_pRecorderFilter->CreateTask(pWriter,&RecOptions);
		if (!m_RecordingTask) {
			SetError(m_pRecorderFilter->GetLastErrorDescription());
			delete pWriter;
			return false;
		}
	}

	if (pWriter!=NULL)
		m_State=STATE_RECORDING;
	m_StartTime.SetCurrentTime();
	m_TotalPauseTime=0;

	ResetError();

	return true;
}


bool CRecordTask::Stop()
{
	if (m_RecordingTask) {
		m_RecordingTask->Stop();
		m_RecordingTask->GetStatistics(&m_Statistics);
		m_pRecorderFilter->DeleteTask(m_RecordingTask);
	}

	m_State=STATE_STOP;

	return true;
}


bool CRecordTask::Pause()
{
	if (m_State==STATE_RECORDING) {
		m_RecordingTask->Pause();
		m_State=STATE_PAUSE;
		m_PauseStartTime=Util::GetTickCount();
	} else if (m_State==STATE_PAUSE) {
		m_RecordingTask->Resume();
		m_State=STATE_RECORDING;
		m_TotalPauseTime+=TickTimeSpan(m_PauseStartTime,Util::GetTickCount());
	} else
		return false;
	return true;
}


CRecordTask::State CRecordTask::GetState() const
{
	return m_State;
}


CRecordTask::DurationType CRecordTask::GetStartTime() const
{
	if (m_State==STATE_STOP)
		return 0;
	return m_StartTime.GetTickTime();
}


bool CRecordTask::GetStartTime(SYSTEMTIME *pTime) const
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


CRecordTask::DurationType CRecordTask::GetRecordTime() const
{
	DurationType Time;

	if (m_State==STATE_RECORDING) {
		Time=TickTimeSpan(m_StartTime.GetTickTime(),Util::GetTickCount());
	} else if (m_State==STATE_PAUSE) {
		Time=TickTimeSpan(m_StartTime.GetTickTime(),m_PauseStartTime);
	} else
		return 0;
	return Time-m_TotalPauseTime;
}


CRecordTask::DurationType CRecordTask::GetPauseTime() const
{
	if (m_State==STATE_RECORDING)
		return m_TotalPauseTime;
	if (m_State==STATE_PAUSE)
		return TickTimeSpan(m_PauseStartTime,Util::GetTickCount())+m_TotalPauseTime;
	return 0;
}


LONGLONG CRecordTask::GetWroteSize() const
{
	LibISDB::RecorderFilter::RecordingStatistics Stats;

	if (!m_RecordingTask
			|| !m_RecordingTask->GetStatistics(&Stats)
			|| Stats.WriteBytes==LibISDB::RecorderFilter::RecordingStatistics::INVALID_SIZE)
		return -1;
	return Stats.WriteBytes;
}


bool CRecordTask::GetFileName(TVTest::String *pFileName) const
{
	if (!m_RecordingTask) {
		if (pFileName!=NULL)
			pFileName->clear();
		return false;
	}
	return m_RecordingTask->GetFileName(pFileName);
}


bool CRecordTask::GetStatistics(LibISDB::RecorderFilter::RecordingStatistics *pStats) const
{
	if (pStats==NULL)
		return false;
	if (!m_RecordingTask) {
		*pStats=m_Statistics;
		return true;
	}
	return m_RecordingTask->GetStatistics(pStats);
}


bool CRecordTask::RelayFile(LPCTSTR pszFileName)
{
	if (!m_RecordingTask)
		return false;
	return m_RecordingTask->Reopen(pszFileName);
}


LONGLONG CRecordTask::GetFreeSpace() const
{
	if (m_State==STATE_STOP)
		return -1;

	TVTest::String FileName;
	if (!GetFileName(&FileName))
		return -1;
	TVTest::String::size_type Pos=FileName.find_last_of(TEXT("\\/"));
	if (Pos==TVTest::String::npos)
		return -1;
	FileName.resize(Pos+1);
	ULARGE_INTEGER FreeSpace;
	if (!::GetDiskFreeSpaceEx(FileName.c_str(),&FreeSpace,NULL,NULL))
		return -1;
	return (LONGLONG)FreeSpace.QuadPart;
}




CRecordManager::CRecordManager()
	: m_fRecording(false)
	, m_fReserved(false)
	, m_fStopOnEventEnd(false)
	, m_Client(CLIENT_USER)
	, m_pTSEngine(NULL)
	, m_pRecorderFilter(NULL)
	//, m_ExistsOperation(EXISTS_CONFIRM)
{
	m_StartTimeSpec.Type=TIME_NOTSPECIFIED;
	m_StopTimeSpec.Type=TIME_NOTSPECIFIED;
}


CRecordManager::~CRecordManager()
{
	Terminate();
}


void CRecordManager::Terminate()
{
	m_RecordTask.Stop();
	m_fRecording=false;
	m_fReserved=false;
}


bool CRecordManager::SetFileName(LPCTSTR pszFileName)
{
	if (m_fRecording)
		return false;
	TVTest::StringUtility::Assign(m_FileName,pszFileName);
	return true;
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


bool CRecordManager::GetStartTime(SYSTEMTIME *pTime) const
{
	if (!m_fRecording)
		return false;
	return m_RecordTask.GetStartTime(pTime);
}


bool CRecordManager::GetReserveTime(SYSTEMTIME *pTime) const
{
	if (!m_fReserved)
		return false;
	return m_ReserveTime.GetTime(pTime);
}


bool CRecordManager::GetReservedStartTime(SYSTEMTIME *pTime) const
{
	if (!m_fReserved)
		return false;
	if (m_StartTimeSpec.Type==TIME_DATETIME) {
		*pTime=m_StartTimeSpec.Time.DateTime;
	} else if (m_StartTimeSpec.Type==TIME_DURATION) {
		if (!m_ReserveTime.GetTime(pTime))
			return false;
		OffsetSystemTime(pTime,m_StartTimeSpec.Time.Duration);
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


void CRecordManager::SetRecorderFilter(
	LibISDB::TSEngine *pTSEngine,
	LibISDB::RecorderFilter *pRecorderFilter)
{
	m_pTSEngine=pTSEngine;
	m_pRecorderFilter=pRecorderFilter;

	m_RecordTask.SetRecorderFilter(pTSEngine,pRecorderFilter);
}


bool CRecordManager::SetRecordingSettings(const CRecordingSettings &Settings)
{
	if (!m_fRecording) {
		if (Settings.m_fEnableTimeShift!=m_Settings.m_fEnableTimeShift) {
			if (Settings.m_fEnableTimeShift) {
				if (!m_RecordTask.Start(NULL,Settings))
					return false;
			} else {
				m_RecordTask.Stop();
			}
		} else if (m_Settings.m_fEnableTimeShift) {
			m_RecordTask.UpdateRecordingSettings(Settings);
		}
	}

	m_Settings=Settings;

	return true;
}


bool CRecordManager::StartRecord(LPCTSTR pszFileName,bool fTimeShift,bool fReserved)
{
	if (m_fRecording) {
		SetError(std::errc::operation_in_progress,TEXT("既に録画中です。"));
		return false;
	}

	if (m_pTSEngine==NULL || m_pRecorderFilter==NULL) {
		SetError(std::errc::operation_not_permitted,TEXT("呼び出しが不正です。"));
		return false;
	}

	CRecordingSettings Settings=fReserved?m_ReserveSettings:m_Settings;

	if (Settings.m_fEnableTimeShift && !fTimeShift)
		Settings.m_fEnableTimeShift=false;

	if (!m_RecordTask.Start(pszFileName,Settings)) {
		SetError(m_RecordTask.GetLastErrorDescription());
		return false;
	}

	m_fRecording=true;
	m_fReserved=false;
	m_StartTimeSpec.Type=TIME_NOTSPECIFIED;
	ResetError();

	return true;
}


void CRecordManager::StopRecord()
{
	if (m_fRecording) {
		m_RecordTask.Stop();
		m_fRecording=false;
		//m_FileName.clear();

		if (m_Settings.m_fEnableTimeShift) {
			m_RecordTask.Start(NULL,m_Settings);
		}
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
		SetError(m_pRecorderFilter->GetLastErrorDescription());
		return false;
	}
	ResetError();
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


CRecordTask::DurationType CRecordManager::GetRecordTime() const
{
	if (!m_fRecording)
		return 0;
	return m_RecordTask.GetRecordTime();
}


CRecordTask::DurationType CRecordManager::GetPauseTime() const
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
			SYSTEMTIME st;

			::GetSystemTime(&st);
			Remain=DiffSystemTime(&st,&m_StopTimeSpec.Time.DateTime);
		}
		break;
	case TIME_DURATION:
		Remain=m_StopTimeSpec.Time.Duration-
			(LONGLONG)TickTimeSpan(m_RecordTask.GetStartTime(),Util::GetTickCount());
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
			SYSTEMTIME st;

			::GetSystemTime(&st);
			if (Offset!=0)
				OffsetSystemTime(&st,Offset);
			if (CompareSystemTime(&st,&m_StartTimeSpec.Time.DateTime)>=0)
				return true;
		}
		break;
	case TIME_DURATION:
		{
			ULONGLONG Span=TickTimeSpan(m_ReserveTime.GetTickTime(),Util::GetTickCount());

			if (Offset!=0) {
				if ((LONGLONG)Offset<=-(LONGLONG)Span)
					return true;
				Span+=Offset;
			}
			if (Span>=m_StartTimeSpec.Time.Duration)
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
			SYSTEMTIME st;

			::GetSystemTime(&st);
			if (Offset!=0)
				OffsetSystemTime(&st,Offset);
			if (CompareSystemTime(&st,&m_StopTimeSpec.Time.DateTime)>=0)
				return true;
		}
		break;
	case TIME_DURATION:
		{
			ULONGLONG Span;

			Span=TickTimeSpan(m_RecordTask.GetStartTime(),Util::GetTickCount());
			if (Offset!=0) {
				if ((LONGLONG)Offset<=-(LONGLONG)Span)
					return true;
				Span+=Offset;
			}
			if (Span>=m_StopTimeSpec.Time.Duration)
				return true;
		}
		break;
	}
	return false;
}


bool CRecordManager::RecordDialog(HWND hwndOwner)
{
	if (!m_fReserved)
		m_ReserveSettings=m_Settings;
	CRecordSettingsDialog Dialog(this,&m_ReserveSettings);
	return Dialog.Show(hwndOwner);
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


bool CRecordManager::GenerateFilePath(
	const FileNameFormatInfo &FormatInfo,LPCWSTR pszFormat,
	TVTest::String *pFilePath) const
{
	if (pFilePath==NULL)
		return false;

	pFilePath->clear();

	if (pszFormat==NULL) {
		if (m_FileName.empty())
			return false;
	}

	TVTest::CEventVariableStringMap VarStrMap(FormatInfo);
	TVTest::String FileName;

	if (m_fReserved && m_StartTimeSpec.Type==TIME_DATETIME) {
		SYSTEMTIME st;
		::SystemTimeToTzSpecificLocalTime(NULL,&m_StartTimeSpec.Time.DateTime,&st);
		LibISDB::DateTime Time;
		Time.FromSYSTEMTIME(st);
		VarStrMap.SetCurrentTime(&Time);
	}

	if (!TVTest::FormatVariableString(
				&VarStrMap,pszFormat!=NULL?pszFormat:m_FileName.c_str(),
				&FileName)
			|| FileName.empty())
		return false;

	if (!::PathIsRelativeW(FileName.c_str())) {
		if (!MakeUniqueFileName(&FileName))
			return false;
	}

	*pFilePath=FileName;

	return true;
}


/*
bool CRecordManager::DoFileExistsOperation(HWND hwndOwner,LPTSTR pszFileName)
{
	lstrcpy(pszFileName,m_FileName.c_str());
	switch (m_ExistsOperation) {
	case EXISTS_CONFIRM:
		if (PathFileExists(m_FileName.c_str())
				&& MessageBox(hwndOwner,
					TEXT("ファイルが既に存在します。\n上書きしますか?"),
					TEXT("上書きの確認"),MB_OKCANCEL | MB_ICONQUESTION)!=IDOK)
			return false;
		break;
	case EXISTS_SEQUENCIALNUMBER:
		if (PathFileExists(m_FileName.c_str())) {
			int i;
			TCHAR szFileName[MAX_PATH];
			LPTSTR pszExtension,p;

			pszExtension=PathFindExtension(m_FileName.c_str());
			lstrcpy(szFileName,m_FileName.c_str());
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
	m_Settings.m_fCurServiceOnly=fOnly;
	return true;
}


bool CRecordManager::GetWritePluginList(std::vector<TVTest::String> *pList)
{
	if (pList==nullptr)
		return false;

	pList->clear();

	TCHAR szDir[MAX_PATH];
	HANDLE hFind;
	WIN32_FIND_DATA fd;

	GetAppClass().GetAppDirectory(szDir);
	::PathAppend(szDir,TEXT("Write_*.dll"));
	hFind=::FindFirstFile(szDir,&fd);
	if (hFind==INVALID_HANDLE_VALUE)
		return false;
	do {
		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0)
			pList->push_back(TVTest::String(fd.cFileName));
	} while (::FindNextFile(hFind,&fd));
	::FindClose(hFind);

	if (pList->size()>1) {
		std::sort(pList->begin(),pList->end(),
				  [](const TVTest::String &Lib1,const TVTest::String &Lib2) {
				      return TVTest::StringUtility::CompareNoCase(Lib1,Lib2)<0;
				  });
	}

	return true;
}


bool CRecordManager::ShowWritePluginSetting(HWND hwndOwner,LPCTSTR pszPlugin)
{
	if (IsStringEmpty(pszPlugin))
		return false;

	bool fOK=false;
	HMODULE hLib=::LoadLibrary(pszPlugin);
	if (hLib!=NULL) {
		typedef void (WINAPI *Setting)(HWND parentWnd);
		Setting pSetting=(Setting)::GetProcAddress(hLib,"Setting");
		if (pSetting!=NULL) {
			pSetting(hwndOwner);
			fOK=true;
		}
		::FreeLibrary(hLib);
	}

	return fOK;
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


CRecordManager::CRecordSettingsDialog::CRecordSettingsDialog(
	CRecordManager *pRecManager,CRecordingSettings *pSettings)
	: m_pRecManager(pRecManager)
	, m_pSettings(pSettings)
{
}


bool CRecordManager::CRecordSettingsDialog::Show(HWND hwndOwner)
{
	return ShowDialog(hwndOwner,
					  GetAppClass().GetResourceInstance(),
					  MAKEINTRESOURCE(IDD_RECORDOPTION))==IDOK;
}


INT_PTR CRecordManager::CRecordSettingsDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			DlgEdit_LimitText(hDlg,IDC_RECORD_FILENAME,MAX_PATH-1);
			if (!m_pRecManager->m_FileName.empty())
				DlgEdit_SetText(hDlg,IDC_RECORD_FILENAME,m_pRecManager->m_FileName.c_str());
			InitDropDownButton(hDlg,IDC_RECORD_FILENAMEFORMAT);
			EnableDlgItems(hDlg,IDC_RECORD_FILENAME_LABEL,IDC_RECORD_FILENAMEPREVIEW,!m_pRecManager->m_fRecording);

			SYSTEMTIME stLocal;
			::GetLocalTime(&stLocal);

			// 開始時間
			SYSTEMTIME stStart;
			DWORD Delay;
			::CheckRadioButton(hDlg,IDC_RECORD_START_NOW,IDC_RECORD_START_DELAY,
							   IDC_RECORD_START_NOW+m_pRecManager->m_StartTimeSpec.Type);
			switch (m_pRecManager->m_StartTimeSpec.Type) {
			case TIME_NOTSPECIFIED:
				stStart=stLocal;
				stStart.wSecond=0;
				stStart.wMilliseconds=0;
				Delay=60*1000;
				break;
			case TIME_DATETIME:
				{
					SYSTEMTIME st;

					::SystemTimeToTzSpecificLocalTime(
						NULL,&m_pRecManager->m_StartTimeSpec.Time.DateTime,&stStart);
					::GetSystemTime(&st);
					LONGLONG Diff=DiffSystemTime(&m_pRecManager->m_StartTimeSpec.Time.DateTime,&st);
					if (Diff>0) {
						Delay=(DWORD)Diff;
					} else {
						Delay=0;
					}
				}
				break;
			case TIME_DURATION:
				Delay=(DWORD)m_pRecManager->m_StartTimeSpec.Time.Duration;
				stStart=stLocal;
				stStart.wSecond=0;
				stStart.wMilliseconds=0;
				OffsetSystemTime(&stStart,Delay);
				break;
			}
			SetDateTimeFormat(hDlg,IDC_RECORD_STARTTIME_TIME);
			DateTime_SetSystemtime(::GetDlgItem(hDlg,IDC_RECORD_STARTTIME_TIME),GDT_VALID,&stStart);
			Delay/=1000;
			::SetDlgItemInt(hDlg,IDC_RECORD_STARTTIME_HOUR,Delay/(60*60),FALSE);
			DlgUpDown_SetRange(hDlg,IDC_RECORD_STARTTIME_HOUR_UD,0,100);
			::SetDlgItemInt(hDlg,IDC_RECORD_STARTTIME_MINUTE,Delay/60%60,FALSE);
			DlgUpDown_SetRange(hDlg,IDC_RECORD_STARTTIME_MINUTE_UD,0,59);
			::SetDlgItemInt(hDlg,IDC_RECORD_STARTTIME_SECOND,Delay%60,FALSE);
			DlgUpDown_SetRange(hDlg,IDC_RECORD_STARTTIME_SECOND_UD,0,59);
			if (m_pRecManager->m_fRecording) {
				EnableDlgItems(hDlg,IDC_RECORD_STARTTIME,IDC_RECORD_STARTTIME_SECOND_LABEL,false);
			} else {
				EnableDlgItem(hDlg,IDC_RECORD_STARTTIME_TIME,
							  m_pRecManager->m_StartTimeSpec.Type==TIME_DATETIME);
				EnableDlgItems(hDlg,
							   IDC_RECORD_STARTTIME_HOUR,
							   IDC_RECORD_STARTTIME_SECOND_LABEL,
							   m_pRecManager->m_StartTimeSpec.Type==TIME_DURATION);
			}

			// 終了時間
			SYSTEMTIME stStop;
			DWORD Duration;
			::CheckDlgButton(hDlg,IDC_RECORD_STOPSPECTIME,
				m_pRecManager->m_StopTimeSpec.Type!=TIME_NOTSPECIFIED?BST_CHECKED:BST_UNCHECKED);
			::CheckRadioButton(hDlg,IDC_RECORD_STOPDATETIME,IDC_RECORD_STOPREMAINTIME,
				m_pRecManager->m_StopTimeSpec.Type==TIME_DATETIME?
					IDC_RECORD_STOPDATETIME:IDC_RECORD_STOPREMAINTIME);
			switch (m_pRecManager->m_StopTimeSpec.Type) {
			case TIME_NOTSPECIFIED:
				stStop=stLocal;
				stStop.wSecond=0;
				stStop.wMilliseconds=0;
				Duration=60*60*1000;
				OffsetSystemTime(&stStop,Duration);
				break;
			case TIME_DATETIME:
				{
					SYSTEMTIME st;

					::SystemTimeToTzSpecificLocalTime(
						NULL,&m_pRecManager->m_StopTimeSpec.Time.DateTime,&stStop);
					::GetSystemTime(&st);
					LONGLONG Diff=DiffSystemTime(&st,&m_pRecManager->m_StopTimeSpec.Time.DateTime);
					if (Diff>0)
						Duration=(DWORD)Diff;
					else
						Duration=0;
				}
				break;
			case TIME_DURATION:
				Duration=(DWORD)m_pRecManager->m_StopTimeSpec.Time.Duration;
				stStop=stLocal;
				stStop.wSecond=0;
				stStop.wMilliseconds=0;
				OffsetSystemTime(&stStop,Duration);
				break;
			}
			SetDateTimeFormat(hDlg,IDC_RECORD_STOPTIME_TIME);
			DateTime_SetSystemtime(::GetDlgItem(hDlg,IDC_RECORD_STOPTIME_TIME),GDT_VALID,&stStop);
			::SetDlgItemInt(hDlg,IDC_RECORD_STOPTIME_HOUR,Duration/(60*60*1000),FALSE);
			DlgUpDown_SetRange(hDlg,IDC_RECORD_STOPTIME_HOUR_UD,0,100);
			::SetDlgItemInt(hDlg,IDC_RECORD_STOPTIME_MINUTE,Duration/(60*1000)%60,FALSE);
			DlgUpDown_SetRange(hDlg,IDC_RECORD_STOPTIME_MINUTE_UD,0,59);
			::SetDlgItemInt(hDlg,IDC_RECORD_STOPTIME_SECOND,Duration/1000%60,FALSE);
			DlgUpDown_SetRange(hDlg,IDC_RECORD_STOPTIME_SECOND_UD,0,59);
			if (m_pRecManager->m_StopTimeSpec.Type==TIME_NOTSPECIFIED) {
				EnableDlgItems(hDlg,
							   IDC_RECORD_STOPDATETIME,
							   IDC_RECORD_STOPTIME_SECOND_LABEL,false);
			} else {
				EnableDlgItem(hDlg,IDC_RECORD_STOPTIME_TIME,
							  m_pRecManager->m_StopTimeSpec.Type==TIME_DATETIME);
				EnableDlgItems(hDlg,
							   IDC_RECORD_STOPTIME_HOUR,
							   IDC_RECORD_STOPTIME_SECOND_LABEL,
							   m_pRecManager->m_StopTimeSpec.Type==TIME_DURATION);
#if 0
				if (m_pRecManager->m_fRecording
						&& m_pRecManager->m_StopTimeSpec.Type==TIME_DURATION)
					::SetTimer(hDlg,1,500,NULL);
#endif
			}

			DlgCheckBox_Check(hDlg,IDC_RECORD_CURSERVICEONLY,
							  m_pSettings->m_fCurServiceOnly);
			DlgCheckBox_Check(hDlg,IDC_RECORD_SAVESUBTITLE,
							  m_pSettings->IsSaveCaption());
			DlgCheckBox_Check(hDlg,IDC_RECORD_SAVEDATACARROUSEL,
							  m_pSettings->IsSaveDataCarrousel());
			if (m_pRecManager->m_fRecording) {
				EnableDlgItems(hDlg,IDC_RECORD_CURSERVICEONLY,IDC_RECORD_SAVEDATACARROUSEL,false);
			} else {
				/*
				EnableDlgItems(hDlg,IDC_RECORD_SAVESUBTITLE,IDC_RECORD_SAVEDATACARROUSEL,
							   m_pSettings->m_fCurServiceOnly);
				*/
			}

			// 保存プラグイン
			DlgComboBox_AddString(hDlg,IDC_RECORD_WRITEPLUGIN,TEXT("使用しない (TS出力)"));
			if (!m_pRecManager->m_fRecording) {
				GetWritePluginList(&m_pRecManager->m_WritePluginList);
				int Sel=-1;
				for (size_t i=0;i<m_pRecManager->m_WritePluginList.size();i++) {
					LPCTSTR pszFileName=m_pRecManager->m_WritePluginList[i].c_str();
					DlgComboBox_AddString(hDlg,IDC_RECORD_WRITEPLUGIN,pszFileName);
					if (!m_pSettings->m_WritePlugin.empty()
							&& IsEqualFileName(pszFileName,m_pSettings->m_WritePlugin.c_str()))
						Sel=(int)i;
				}
				DlgComboBox_SetCurSel(hDlg,IDC_RECORD_WRITEPLUGIN,Sel+1);
				EnableDlgItem(hDlg,IDC_RECORD_WRITEPLUGINSETTING,Sel>=0);
			} else {
				int Sel=0;
				if (!m_pSettings->m_WritePlugin.empty()) {
					DlgComboBox_AddString(hDlg,IDC_RECORD_WRITEPLUGIN,
										  ::PathFindFileName(m_pSettings->m_WritePlugin.c_str()));
					Sel=1;
				}
				DlgComboBox_SetCurSel(hDlg,IDC_RECORD_WRITEPLUGIN,Sel);
				EnableDlgItems(hDlg,IDC_RECORD_WRITEPLUGIN_LABEL,IDC_RECORD_WRITEPLUGINSETTING,false);
			}

			EnableDlgItem(hDlg,IDC_RECORD_CANCEL,m_pRecManager->m_fReserved);
		}
		return TRUE;

#if 0
	case WM_TIMER:
		{
			const LONGLONG Remain=m_pRecManager->GetRemainTime();

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

				DlgEdit_GetText(hDlg,IDC_RECORD_FILENAME,szFileName,MAX_PATH);
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
				if (FileSaveDialog(&ofn))
					DlgEdit_SetText(hDlg,IDC_RECORD_FILENAME,szFileName);
			}
			return TRUE;

		case IDC_RECORD_FILENAME:
			if (HIWORD(wParam)==EN_CHANGE) {
				TCHAR szFormat[MAX_PATH];
				TVTest::String FileName;

				DlgEdit_GetText(hDlg,IDC_RECORD_FILENAME,szFormat,lengthof(szFormat));
				if (szFormat[0]!='\0') {
					TVTest::CEventVariableStringMap EventVarStrMap;
					EventVarStrMap.SetSampleEventInfo();
					TVTest::FormatVariableString(&EventVarStrMap,szFormat,&FileName);
				}
				DlgEdit_SetText(hDlg,IDC_RECORD_FILENAMEPREVIEW,FileName.c_str());
			}
			return TRUE;

		case IDC_RECORD_FILENAMEFORMAT:
			{
				RECT rc;
				POINT pt;

				::GetWindowRect(::GetDlgItem(hDlg,IDC_RECORD_FILENAMEFORMAT),&rc);
				pt.x=rc.left;
				pt.y=rc.bottom;
				TVTest::CEventVariableStringMap EventVarStrMap;
				EventVarStrMap.InputParameter(hDlg,IDC_RECORD_FILENAME,pt);
			}
			return TRUE;

		case IDC_RECORD_START_NOW:
		case IDC_RECORD_START_DATETIME:
		case IDC_RECORD_START_DELAY:
			EnableDlgItem(hDlg,IDC_RECORD_STARTTIME_TIME,
				DlgCheckBox_IsChecked(hDlg,IDC_RECORD_START_DATETIME));
			EnableDlgItems(hDlg,
				IDC_RECORD_STARTTIME_HOUR,
				IDC_RECORD_STARTTIME_SECOND_LABEL,
				DlgCheckBox_IsChecked(hDlg,IDC_RECORD_START_DELAY));
			return TRUE;

		case IDC_RECORD_STOPSPECTIME:
		case IDC_RECORD_STOPDATETIME:
		case IDC_RECORD_STOPREMAINTIME:
			if (DlgCheckBox_IsChecked(hDlg,IDC_RECORD_STOPSPECTIME)) {
				bool fDateTime=DlgRadioButton_IsChecked(hDlg,IDC_RECORD_STOPDATETIME);

				EnableDlgItems(hDlg,
					IDC_RECORD_STOPDATETIME,
					IDC_RECORD_STOPREMAINTIME,
					true);
				EnableDlgItem(hDlg,IDC_RECORD_STOPTIME_TIME,fDateTime);
				EnableDlgItems(hDlg,
					IDC_RECORD_STOPTIME_HOUR,
					IDC_RECORD_STOPTIME_SECOND_LABEL,
					!fDateTime);
			} else {
				EnableDlgItems(hDlg,
					IDC_RECORD_STOPDATETIME,
					IDC_RECORD_STOPTIME_SECOND_LABEL,
					false);
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

		case IDC_RECORD_WRITEPLUGIN:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				EnableDlgItem(hDlg,IDC_RECORD_WRITEPLUGINSETTING,
							  DlgComboBox_GetCurSel(hDlg,IDC_RECORD_WRITEPLUGIN)>0);
			}
			return TRUE;

		case IDC_RECORD_WRITEPLUGINSETTING:
			{
				LRESULT Sel=DlgComboBox_GetCurSel(hDlg,IDC_RECORD_WRITEPLUGIN)-1;

				if (Sel>=0 && (size_t)Sel<m_pRecManager->m_WritePluginList.size()) {
					ShowWritePluginSetting(hDlg,m_pRecManager->m_WritePluginList[Sel].c_str());
				}
			}
			return TRUE;

		case IDOK:
			{
				int StartTimeChecked;
				SYSTEMTIME stCur,stStart,stStop;

				::GetSystemTime(&stCur);

				if (!m_pRecManager->m_fRecording) {
					StartTimeChecked=GetCheckedRadioButton(hDlg,IDC_RECORD_START_NOW,IDC_RECORD_START_DELAY);
					if (StartTimeChecked==IDC_RECORD_START_DATETIME) {
						SYSTEMTIME st;
						DateTime_GetSystemtime(
							::GetDlgItem(hDlg,IDC_RECORD_STARTTIME_TIME),&st);
						::TzSpecificLocalTimeToSystemTime(NULL,&st,&stStart);
						if (CompareSystemTime(&stStart,&stCur)<=0) {
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
					SYSTEMTIME st;
					DateTime_GetSystemtime(
						::GetDlgItem(hDlg,IDC_RECORD_STOPTIME_TIME),&st);
					::TzSpecificLocalTimeToSystemTime(NULL,&st,&stStop);
					if (CompareSystemTime(&stStop,&stCur)<=0) {
						::MessageBox(hDlg,
							TEXT("指定された停止時刻を既に過ぎています。"),
							NULL,MB_OK | MB_ICONEXCLAMATION);
						SetDlgItemFocus(hDlg,IDC_RECORD_STOPTIME_TIME);
						return TRUE;
					}
				}

				if (!m_pRecManager->m_fRecording) {
					TCHAR szFileName[MAX_PATH];

					GetDlgItemText(hDlg,IDC_RECORD_FILENAME,szFileName,MAX_PATH);
					CFilePath FilePath(szFileName);
					if (szFileName[0]=='\0' || *FilePath.GetFileName()=='\0') {
						::MessageBox(hDlg,
							TEXT("ファイル名を入力してください。"),
							NULL,MB_OK | MB_ICONEXCLAMATION);
						SetDlgItemFocus(hDlg,IDC_RECORD_FILENAME);
						return TRUE;
					}
#if 0
					if (!FilePath.IsValid()) {
						::MessageBox(hDlg,
							TEXT("ファイル名に使用できない文字が含まれています。"),
							NULL,MB_OK | MB_ICONEXCLAMATION);
						SetDlgItemFocus(hDlg,IDC_RECORD_FILENAME);
						return TRUE;
					}
#else
					TVTest::String Message;
					if (!IsValidFileName(FilePath.GetFileName(),0,&Message)) {
						::MessageBox(hDlg,Message.c_str(),NULL,MB_OK | MB_ICONEXCLAMATION);
						SetDlgItemFocus(hDlg,IDC_RECORD_FILENAME);
						return TRUE;
					}
#endif
					if (!FilePath.HasDirectory()) {
						::MessageBox(hDlg,
							TEXT("保存先フォルダを入力してください。"),
							NULL,MB_OK | MB_ICONEXCLAMATION);
						SetDlgItemFocus(hDlg,IDC_RECORD_FILENAME);
						return TRUE;
					}
					if (StartTimeChecked!=IDC_RECORD_START_NOW) {
						FilePath.GetDirectory(szFileName);
						CAppMain::CreateDirectoryResult CreateDirResult=
							GetAppClass().CreateDirectory(
								hDlg,szFileName,
								TEXT("録画ファイルの保存先フォルダ \"%s\" がありません。\n")
								TEXT("作成しますか?"));
						if (CreateDirResult==CAppMain::CREATEDIRECTORY_RESULT_ERROR) {
							SetDlgItemFocus(hDlg,IDC_RECORD_FILENAME);
							return TRUE;
						}
					}
					m_pRecManager->SetFileName(FilePath.GetPath());

					switch (StartTimeChecked) {
					case IDC_RECORD_START_NOW:
					default:
						m_pRecManager->SetStartTimeSpec(NULL);
						break;
					case IDC_RECORD_START_DATETIME:
						{
							TimeSpecInfo TimeSpec;

							TimeSpec.Type=TIME_DATETIME;
							TimeSpec.Time.DateTime=stStart;
							m_pRecManager->SetStartTimeSpec(&TimeSpec);
						}
						break;
					case IDC_RECORD_START_DELAY:
						{
							TimeSpecInfo TimeSpec;
							unsigned int Hour,Minute,Second;

							TimeSpec.Type=TIME_DURATION;
							Hour=::GetDlgItemInt(hDlg,IDC_RECORD_STARTTIME_HOUR,NULL,FALSE);
							Minute=::GetDlgItemInt(hDlg,IDC_RECORD_STARTTIME_MINUTE,NULL,FALSE);
							Second=::GetDlgItemInt(hDlg,IDC_RECORD_STARTTIME_SECOND,NULL,FALSE);
							TimeSpec.Time.Duration=(Hour*(60*60)+Minute*60+Second)*1000;
							m_pRecManager->SetStartTimeSpec(&TimeSpec);
						}
						break;
					}

					m_pSettings->m_fCurServiceOnly=
						DlgCheckBox_IsChecked(hDlg,IDC_RECORD_CURSERVICEONLY);
					m_pSettings->SetSaveCaption(
						DlgCheckBox_IsChecked(hDlg,IDC_RECORD_SAVESUBTITLE));
					m_pSettings->SetSaveDataCarrousel(
						DlgCheckBox_IsChecked(hDlg,IDC_RECORD_SAVEDATACARROUSEL));

					int Sel=(int)DlgComboBox_GetCurSel(hDlg,IDC_RECORDOPTIONS_WRITEPLUGIN)-1;
					if (Sel>=0 && (size_t)Sel<m_pRecManager->m_WritePluginList.size())
						m_pSettings->m_WritePlugin=m_pRecManager->m_WritePluginList[Sel];
					else
						m_pSettings->m_WritePlugin.clear();
				}

				if (DlgCheckBox_IsChecked(hDlg,IDC_RECORD_STOPSPECTIME)) {
					TimeSpecInfo TimeSpec;

					if (DlgCheckBox_IsChecked(hDlg,IDC_RECORD_STOPDATETIME)) {
						TimeSpec.Type=TIME_DATETIME;
						TimeSpec.Time.DateTime=stStop;
					} else {
						unsigned int Hour,Minute,Second;

						TimeSpec.Type=TIME_DURATION;
						Hour=::GetDlgItemInt(hDlg,IDC_RECORD_STOPTIME_HOUR,NULL,FALSE);
						Minute=::GetDlgItemInt(hDlg,IDC_RECORD_STOPTIME_MINUTE,NULL,FALSE);
						Second=::GetDlgItemInt(hDlg,IDC_RECORD_STOPTIME_SECOND,NULL,FALSE);
						TimeSpec.Time.Duration=(Hour*(60*60)+Minute*60+Second)*1000;
					}
					m_pRecManager->SetStopTimeSpec(&TimeSpec);
				} else {
					m_pRecManager->SetStopTimeSpec(NULL);
				}
			}
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;

		case IDC_RECORD_CANCEL:
			m_pRecManager->CancelReserve();
			::EndDialog(hDlg,IDCANCEL);
			return TRUE;
		}
		return TRUE;

	case WM_DESTROY:
		m_pRecManager->m_WritePluginList.clear();
		return TRUE;
	}

	return FALSE;
}
