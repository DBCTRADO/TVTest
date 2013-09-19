#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "RecordOptions.h"
#include "Command.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


static const UINT MEGA_BYTES=1024*1024;

// 書き出しバッファサイズの制限(バイト単位)
static const UINT WRITE_BUFFER_SIZE_MIN=1024;
static const UINT WRITE_BUFFER_SIZE_MAX=32*MEGA_BYTES;
static const UINT WRITE_BUFFER_SIZE_DEFAULT=MEGA_BYTES;

// さかのぼり録画バッファサイズの制限(MiB単位)
static const UINT TIMESHIFT_BUFFER_SIZE_MIN=1;
static const UINT TIMESHIFT_BUFFER_SIZE_MAX=1024;
static const UINT TIMESHIFT_BUFFER_SIZE_DEFAULT=32;

// 書き出し待ちバッファの制限(バイト単位)
static const UINT MAX_PENDING_SIZE_MIN=32*MEGA_BYTES;
static const UINT MAX_PENDING_SIZE_MAX=1024*MEGA_BYTES;
static const UINT MAX_PENDING_SIZE_DEFAULT=512*MEGA_BYTES;

// ステータスバーからの録画のコマンド
static const int StatusBarCommandList[] = {
	CM_RECORD_START,
	CM_RECORDOPTION,
	CM_TIMESHIFTRECORDING,
	0,
};




CRecordOptions::CRecordOptions()
	: m_fConfirmChannelChange(true)
	, m_fConfirmExit(true)
	, m_fConfirmStop(false)
	, m_fConfirmStopStatusBarOnly(false)
	, m_fCurServiceOnly(false)
	, m_fSaveSubtitle(true)
	, m_fSaveDataCarrousel(true)
	, m_fAlertLowFreeSpace(true)
	, m_LowFreeSpaceThreshold(2048)
	, m_BufferSize(WRITE_BUFFER_SIZE_DEFAULT)
	, m_TimeShiftBufferSize(TIMESHIFT_BUFFER_SIZE_DEFAULT)
	, m_fEnableTimeShiftRecording(false)
	, m_MaxPendingSize(MAX_PENDING_SIZE_DEFAULT)
	, m_fShowRemainTime(false)
	, m_StatusBarRecordCommand(CM_RECORD_START)
{
	m_szSaveFolder[0]=_T('\0');
	::lstrcpy(m_szFileName,TEXT("Record_%date%-%time%.ts"));
}


CRecordOptions::~CRecordOptions()
{
	Destroy();
}


bool CRecordOptions::Apply(DWORD Flags)
{
	CDtvEngine &DtvEngine=GetAppClass().GetCoreEngine()->m_DtvEngine;

	if ((Flags&UPDATE_RECORDSTREAM)!=0
			&& !GetAppClass().GetRecordManager()->IsRecording()) {
		DWORD Stream=CTsSelector::STREAM_ALL;
		if (!m_fSaveSubtitle)
			Stream^=CTsSelector::STREAM_CAPTION;
		if (!m_fSaveDataCarrousel)
			Stream^=CTsSelector::STREAM_DATACARROUSEL;
		DtvEngine.SetWriteCurServiceOnly(m_fCurServiceOnly,Stream);
		DtvEngine.m_FileWriter.ClearQueue();
	}

	if ((Flags&UPDATE_TIMESHIFTBUFFER)!=0)
		DtvEngine.m_FileWriter.SetQueueSize(MEGA_BYTES,m_TimeShiftBufferSize);

	if ((Flags&UPDATE_ENABLETIMESHIFT)!=0)
		DtvEngine.m_FileWriter.EnableQueueing(m_fEnableTimeShiftRecording);

	if ((Flags&UPDATE_MAXPENDINGSIZE)!=0)
		DtvEngine.m_FileWriter.SetMaxPendingSize(m_MaxPendingSize);

	return true;
}


bool CRecordOptions::ReadSettings(CSettings &Settings)
{
	TCHAR szPath[MAX_PATH];
	unsigned int Value;

	// 古いバージョンの互換用
	if (Settings.Read(TEXT("RecordFile"),szPath,lengthof(szPath))
			&& szPath[0]!='\0') {
		LPTSTR pszFileName=::PathFindFileName(szPath);

		if (pszFileName!=szPath) {
			*(pszFileName-1)='\0';
			::lstrcpy(m_szSaveFolder,szPath);
			::lstrcpy(m_szFileName,pszFileName);
		}
	}
	if (Settings.Read(TEXT("RecordFolder"),szPath,lengthof(szPath))
			&& szPath[0]!='\0')
		::lstrcpy(m_szSaveFolder,szPath);
	if (Settings.Read(TEXT("RecordFileName"),szPath,lengthof(szPath))
			&& szPath[0]!='\0')
		::lstrcpy(m_szFileName,szPath);
#if 0
	// 古いバージョンの互換用
	bool fAddTime;
	if (Settings.Read(TEXT("AddRecordTime"),&fAddTime) && fAddTime) {
		TCHAR szFormat[] = TEXT("%date%_%time%");
		if (::lstrlen(m_szFileName)+lengthof(szFormat)<=lengthof(m_szFileName)) {
			LPTSTR pszExt=::PathFindExtension(m_szFileName);
			TCHAR szExtension[MAX_PATH];
			::lstrcpy(szExtension,pszExt);
			::wsprintf(pszExt,TEXT("%s%s"),szFormat,szExtension);
		}
	}
#endif
	Settings.Read(TEXT("ConfirmRecChChange"),&m_fConfirmChannelChange);
	Settings.Read(TEXT("ConfrimRecordingExit"),&m_fConfirmExit);
	Settings.Read(TEXT("ConfrimRecordStop"),&m_fConfirmStop);
	Settings.Read(TEXT("ConfrimRecordStopStatusBarOnly"),&m_fConfirmStopStatusBarOnly);
	Settings.Read(TEXT("RecordCurServiceOnly"),&m_fCurServiceOnly);
	Settings.Read(TEXT("RecordSubtitle"),&m_fSaveSubtitle);
	Settings.Read(TEXT("RecordDataCarrousel"),&m_fSaveDataCarrousel);
	if (Settings.Read(TEXT("RecordBufferSize"),&Value))
		m_BufferSize=CLAMP(Value,WRITE_BUFFER_SIZE_MIN,WRITE_BUFFER_SIZE_MAX);
	Settings.Read(TEXT("AlertLowFreeSpace"),&m_fAlertLowFreeSpace);
	Settings.Read(TEXT("LowFreeSpaceThreshold"),&m_LowFreeSpaceThreshold);
	if (Settings.Read(TEXT("TimeShiftRecBufferSize"),&Value))
		m_TimeShiftBufferSize=CLAMP(Value,TIMESHIFT_BUFFER_SIZE_MIN,TIMESHIFT_BUFFER_SIZE_MAX);
	Settings.Read(TEXT("TimeShiftRecording"),&m_fEnableTimeShiftRecording);
	if (Settings.Read(TEXT("RecMaxPendingSize"),&Value))
		m_MaxPendingSize=CLAMP(Value,MAX_PENDING_SIZE_MIN,MAX_PENDING_SIZE_MAX);
	Settings.Read(TEXT("ShowRecordRemainTime"),&m_fShowRemainTime);

	TCHAR szCommand[CCommandList::MAX_COMMAND_TEXT];
	if (Settings.Read(TEXT("StatusBarRecordCommand"),szCommand,lengthof(szCommand))) {
		if (szCommand[0]!=_T('\0')) {
			int Command=GetAppClass().GetCommandList()->ParseText(szCommand);
			if (Command!=0)
				m_StatusBarRecordCommand=Command;
		} else {
			m_StatusBarRecordCommand=0;
		}
	}

	return true;
}


bool CRecordOptions::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("RecordFolder"),m_szSaveFolder);
	Settings.Write(TEXT("RecordFileName"),m_szFileName);
	Settings.Write(TEXT("ConfirmRecChChange"),m_fConfirmChannelChange);
	Settings.Write(TEXT("ConfrimRecordingExit"),m_fConfirmExit);
	Settings.Write(TEXT("ConfrimRecordStop"),m_fConfirmStop);
	Settings.Write(TEXT("ConfrimRecordStopStatusBarOnly"),m_fConfirmStopStatusBarOnly);
	Settings.Write(TEXT("RecordCurServiceOnly"),m_fCurServiceOnly);
	Settings.Write(TEXT("RecordSubtitle"),m_fSaveSubtitle);
	Settings.Write(TEXT("RecordDataCarrousel"),m_fSaveDataCarrousel);
	Settings.Write(TEXT("RecordBufferSize"),m_BufferSize);
	Settings.Write(TEXT("AlertLowFreeSpace"),m_fAlertLowFreeSpace);
	Settings.Write(TEXT("LowFreeSpaceThreshold"),m_LowFreeSpaceThreshold);
	Settings.Write(TEXT("TimeShiftRecBufferSize"),m_TimeShiftBufferSize);
	Settings.Write(TEXT("TimeShiftRecording"),m_fEnableTimeShiftRecording);
	Settings.Write(TEXT("RecMaxPendingSize"),m_MaxPendingSize);
	Settings.Write(TEXT("ShowRecordRemainTime"),m_fShowRemainTime);
	if (m_StatusBarRecordCommand!=0) {
		LPCTSTR pszCommand=GetAppClass().GetCommandList()->GetCommandTextByID(m_StatusBarRecordCommand);
		if (pszCommand!=NULL)
			Settings.Write(TEXT("StatusBarRecordCommand"),pszCommand);
	} else {
		Settings.Write(TEXT("StatusBarRecordCommand"),TEXT(""));
	}
	return true;
}


bool CRecordOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_RECORD));
}


bool CRecordOptions::SetSaveFolder(LPCTSTR pszFolder)
{
	::lstrcpy(m_szSaveFolder,pszFolder);
	return true;
}


bool CRecordOptions::GetFilePath(LPTSTR pszFileName,int MaxLength) const
{
	if (m_szSaveFolder[0]=='\0' || m_szFileName[0]=='\0')
		return false;
	if (::lstrlen(m_szSaveFolder)+1+::lstrlen(m_szFileName)>=MaxLength)
		return false;
	::PathCombine(pszFileName,m_szSaveFolder,m_szFileName);
	return true;
}


bool CRecordOptions::GenerateFilePath(LPTSTR pszFileName,int MaxLength,LPCTSTR *ppszErrorMessage) const
{
	if (m_szSaveFolder[0]=='\0') {
		if (ppszErrorMessage)
			*ppszErrorMessage=TEXT("設定で保存先フォルダを指定してください。");
		return false;
	}
	if (!::PathIsDirectory(m_szSaveFolder)) {
		int Result=::SHCreateDirectoryEx(NULL,m_szSaveFolder,NULL);
		if (Result!=ERROR_SUCCESS && Result!=ERROR_ALREADY_EXISTS) {
			if (ppszErrorMessage)
				*ppszErrorMessage=TEXT("保存先フォルダが作成できません。");
			return false;
		}
	}
	if (m_szFileName[0]=='\0') {
		if (ppszErrorMessage)
			*ppszErrorMessage=TEXT("設定でファイル名を指定してください。");
		return false;
	}
	if (::lstrlen(m_szSaveFolder)+1+::lstrlen(m_szFileName)>=MaxLength) {
		if (ppszErrorMessage)
			*ppszErrorMessage=TEXT("ファイルパスが長すぎます。");
		return false;
	}
	::lstrcpy(pszFileName,m_szSaveFolder);
	::PathAddBackslash(pszFileName);
	::lstrcat(pszFileName,m_szFileName);
	return true;
}


bool CRecordOptions::ConfirmChannelChange(HWND hwndOwner) const
{
	if (m_fConfirmChannelChange) {
		if (::MessageBox(hwndOwner,TEXT("録画中です。チャンネル変更しますか?"),
				TEXT("チャンネル変更の確認"),
				MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONQUESTION)!=IDOK)
			return false;
	}
	return true;
}


bool CRecordOptions::ConfirmServiceChange(HWND hwndOwner,const CRecordManager *pRecordManager) const
{
	if (pRecordManager->GetCurServiceOnly()) {
		if (::MessageBox(hwndOwner,
				TEXT("現在のサービスのみ録画中です。\r\n")
				TEXT("サービスの変更をすると正常に再生できなくなるかも知れません。\r\n")
				TEXT("サービスを変更しますか?"),
				TEXT("変更の確認"),
				MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONQUESTION)!=IDOK)
			return false;
	}
	return true;
}


bool CRecordOptions::ConfirmStop(HWND hwndOwner) const
{
	if (m_fConfirmStop && !m_fConfirmStopStatusBarOnly) {
		if (::MessageBox(hwndOwner,
				TEXT("録画を停止しますか?"),TEXT("停止の確認"),
				MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONQUESTION)!=IDOK)
			return false;
	}
	return true;
}


bool CRecordOptions::ConfirmStatusBarStop(HWND hwndOwner) const
{
	if (m_fConfirmStop && m_fConfirmStopStatusBarOnly) {
		if (::MessageBox(hwndOwner,
				TEXT("録画を停止しますか?"),TEXT("停止の確認"),
				MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONQUESTION)!=IDOK)
			return false;
	}
	return true;
}


bool CRecordOptions::ConfirmExit(HWND hwndOwner,const CRecordManager *pRecordManager) const
{
	if (m_fConfirmExit && pRecordManager->IsRecording()) {
		if (::MessageBox(hwndOwner,
				TEXT("現在録画中です。\r\n終了してもいいですか?"),
				TEXT("終了の確認"),
				MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONQUESTION)!=IDOK)
			return false;
	}
	if (pRecordManager->IsReserved()) {
		if (::MessageBox(hwndOwner,
				TEXT("録画の設定がされています。\r\n")
				TEXT("終了すると録画は行われません。\r\n")
				TEXT("終了してもいいですか?"),
				TEXT("終了の確認"),
				MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONQUESTION)!=IDOK)
			return false;
	}
	return true;
}


bool CRecordOptions::EnableTimeShiftRecording(bool fEnable)
{
	if (m_fEnableTimeShiftRecording!=fEnable) {
		if (!GetAppClass().GetCoreEngine()->m_DtvEngine.m_FileWriter.EnableQueueing(fEnable))
			return false;
		m_fEnableTimeShiftRecording=fEnable;
	}
	return true;
}


bool CRecordOptions::ApplyOptions(CRecordManager *pManager)
{
	pManager->SetCurServiceOnly(m_fCurServiceOnly);
	DWORD Stream=CTsSelector::STREAM_ALL;
	if (!m_fSaveSubtitle)
		Stream^=CTsSelector::STREAM_CAPTION;
	if (!m_fSaveDataCarrousel)
		Stream^=CTsSelector::STREAM_DATACARROUSEL;
	pManager->SetSaveStream(Stream);
	pManager->SetBufferSize(m_BufferSize);
	return true;
}


INT_PTR CRecordOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			/*
			if (m_szSaveFolder[0]=='\0') {
				if (!::SHGetSpecialFolderPath(hDlg,m_szSaveFolder,
											  CSIDL_MYVIDEO,FALSE)
						&& !::SHGetSpecialFolderPath(hDlg,m_szSaveFolder,
													 CSIDL_PERSONAL,FALSE))
					m_szSaveFolder[0]='\0';
			}
			*/
			::SendDlgItemMessage(hDlg,IDC_RECORDOPTIONS_SAVEFOLDER,EM_LIMITTEXT,MAX_PATH-1,0);
			::SetDlgItemText(hDlg,IDC_RECORDOPTIONS_SAVEFOLDER,m_szSaveFolder);
			::SendDlgItemMessage(hDlg,IDC_RECORDOPTIONS_FILENAME,EM_LIMITTEXT,MAX_PATH-1,0);
			::SetDlgItemText(hDlg,IDC_RECORDOPTIONS_FILENAME,m_szFileName);
			::CheckDlgButton(hDlg,IDC_RECORDOPTIONS_CONFIRMCHANNELCHANGE,
				m_fConfirmChannelChange?BST_CHECKED:BST_UNCHECKED);
			::CheckDlgButton(hDlg,IDC_RECORDOPTIONS_CONFIRMEXIT,
				m_fConfirmExit?BST_CHECKED:BST_UNCHECKED);
			::CheckDlgButton(hDlg,IDC_RECORDOPTIONS_CONFIRMSTOP,
				m_fConfirmStop?BST_CHECKED:BST_UNCHECKED);
			::CheckDlgButton(hDlg,IDC_RECORDOPTIONS_CONFIRMSTATUSBARSTOP,
				m_fConfirmStopStatusBarOnly?BST_CHECKED:BST_UNCHECKED);
			EnableDlgItem(hDlg,IDC_RECORDOPTIONS_CONFIRMSTATUSBARSTOP,m_fConfirmStop);
			::CheckDlgButton(hDlg,IDC_RECORDOPTIONS_CURSERVICEONLY,
				m_fCurServiceOnly?BST_CHECKED:BST_UNCHECKED);
			::CheckDlgButton(hDlg,IDC_RECORDOPTIONS_SAVESUBTITLE,
				m_fSaveSubtitle?BST_CHECKED:BST_UNCHECKED);
			::CheckDlgButton(hDlg,IDC_RECORDOPTIONS_SAVEDATACARROUSEL,
				m_fSaveDataCarrousel?BST_CHECKED:BST_UNCHECKED);

			DlgCheckBox_Check(hDlg,IDC_RECORDOPTIONS_ALERTLOWFREESPACE,m_fAlertLowFreeSpace);
			::SetDlgItemInt(hDlg,IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD,m_LowFreeSpaceThreshold,FALSE);
			DlgUpDown_SetRange(hDlg,IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD_SPIN,
							   1,0x7FFFFFFF);
			EnableDlgItems(hDlg,IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD_LABEL,
								IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD_UNIT,
						   m_fAlertLowFreeSpace);

			const CCommandList *pCommandList=GetAppClass().GetCommandList();
			for (int i=0;i<lengthof(StatusBarCommandList);i++) {
				const int Command=StatusBarCommandList[i];
				TCHAR szText[CCommandList::MAX_COMMAND_NAME];

				if (Command!=0)
					pCommandList->GetCommandNameByID(Command,szText,lengthof(szText));
				else
					::lstrcpy(szText,TEXT("何もしない"));
				DlgComboBox_AddString(hDlg,IDC_RECORDOPTIONS_STATUSBARCOMMAND,szText);
				if (Command==m_StatusBarRecordCommand)
					DlgComboBox_SetCurSel(hDlg,IDC_RECORDOPTIONS_STATUSBARCOMMAND,i);
			}

			::SetDlgItemInt(hDlg,IDC_RECORDOPTIONS_BUFFERSIZE,
							m_BufferSize/1024,FALSE);
			DlgUpDown_SetRange(hDlg,IDC_RECORDOPTIONS_BUFFERSIZE_UD,
							   WRITE_BUFFER_SIZE_MIN/1024,WRITE_BUFFER_SIZE_MAX/1024);

			::SetDlgItemInt(hDlg,IDC_RECORDOPTIONS_TIMESHIFTBUFFERSIZE,m_TimeShiftBufferSize,FALSE);
			DlgUpDown_SetRange(hDlg,IDC_RECORDOPTIONS_TIMESHIFTBUFFERSIZE_SPIN,
							   TIMESHIFT_BUFFER_SIZE_MIN,TIMESHIFT_BUFFER_SIZE_MAX);

			::SetDlgItemInt(hDlg,IDC_RECORDOPTIONS_MAXPENDINGSIZE,
							m_MaxPendingSize/MEGA_BYTES,FALSE);
			DlgUpDown_SetRange(hDlg,IDC_RECORDOPTIONS_MAXPENDINGSIZE_SPIN,
							   MAX_PENDING_SIZE_MIN/MEGA_BYTES,MAX_PENDING_SIZE_MAX/MEGA_BYTES);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_RECORDOPTIONS_SAVEFOLDER_BROWSE:
			{
				TCHAR szFolder[MAX_PATH];

				::GetDlgItemText(hDlg,IDC_RECORDOPTIONS_SAVEFOLDER,szFolder,MAX_PATH);
				if (BrowseFolderDialog(hDlg,szFolder,
										TEXT("録画ファイルの保存先フォルダ:")))
					::SetDlgItemText(hDlg,IDC_RECORDOPTIONS_SAVEFOLDER,szFolder);
			}
			return TRUE;

		case IDC_RECORDOPTIONS_FILENAME:
			if (HIWORD(wParam)==EN_CHANGE) {
				TCHAR szFormat[MAX_PATH],szFileName[MAX_PATH];

				::GetDlgItemText(hDlg,IDC_RECORDOPTIONS_FILENAME,szFormat,lengthof(szFormat));
				szFileName[0]='\0';
				if (szFormat[0]!='\0') {
					CRecordManager::EventInfo EventInfo;

					CRecordManager::GetEventInfoSample(&EventInfo);
					if (!GetAppClass().GetRecordManager()->GenerateFileName(
							szFileName,lengthof(szFileName),&EventInfo,szFormat))
						szFileName[0]='\0';
				}
				::SetDlgItemText(hDlg,IDC_RECORDOPTIONS_FILENAMEPREVIEW,szFileName);
			}
			return TRUE;

		case IDC_RECORDOPTIONS_FILENAMEFORMAT:
			{
				RECT rc;
				POINT pt;

				::GetWindowRect(::GetDlgItem(hDlg,IDC_RECORDOPTIONS_FILENAMEFORMAT),&rc);
				pt.x=rc.left;
				pt.y=rc.bottom;
				CRecordManager::InsertFileNameParameter(hDlg,IDC_RECORDOPTIONS_FILENAME,&pt);
			}
			return TRUE;

		case IDC_RECORDOPTIONS_CONFIRMSTOP:
			EnableDlgItem(hDlg,IDC_RECORDOPTIONS_CONFIRMSTATUSBARSTOP,
				DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_CONFIRMSTOP));
			return TRUE;

		case IDC_RECORDOPTIONS_ALERTLOWFREESPACE:
			EnableDlgItems(hDlg,IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD_LABEL,
								IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD_UNIT,
						   DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_ALERTLOWFREESPACE));
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				TCHAR szSaveFolder[MAX_PATH],szFileName[MAX_PATH];

				::GetDlgItemText(hDlg,IDC_RECORDOPTIONS_SAVEFOLDER,szSaveFolder,MAX_PATH);
				if (szSaveFolder[0]!='\0' && !::PathIsDirectory(szSaveFolder)) {
					TCHAR szMessage[MAX_PATH+80];

					StdUtil::snprintf(szMessage,lengthof(szMessage),
						TEXT("録画ファイルの保存先フォルダ \"%s\" がありません。\n")
						TEXT("作成しますか?"),szSaveFolder);
					if (::MessageBox(hDlg,szMessage,TEXT("フォルダ作成の確認"),
										MB_YESNO | MB_ICONQUESTION)==IDYES) {
						int Result;

						Result=::SHCreateDirectoryEx(hDlg,szSaveFolder,NULL);
						if (Result!=ERROR_SUCCESS
								&& Result!=ERROR_ALREADY_EXISTS) {
							SettingError();
							::MessageBox(hDlg,TEXT("フォルダが作成できません。"),
											NULL,MB_OK | MB_ICONEXCLAMATION);
							SetDlgItemFocus(hDlg,IDC_RECORDOPTIONS_SAVEFOLDER);
							return TRUE;
						}
					}
				}
				::GetDlgItemText(hDlg,IDC_RECORDOPTIONS_FILENAME,szFileName,lengthof(szFileName));
				if (szFileName[0]!='\0') {
#if 0
					CFilePath FilePath(szFileName);
					if (!FilePath.IsValid()) {
						::MessageBox(hDlg,
							TEXT("録画ファイル名に、ファイル名に使用できない文字が含まれています。"),
							NULL,MB_OK | MB_ICONEXCLAMATION);
					}
#else
					TCHAR szMessage[256];
					if (!IsValidFileName(szFileName,false,szMessage,lengthof(szMessage))) {
						SettingError();
						SetDlgItemFocus(hDlg,IDC_RECORDOPTIONS_FILENAME);
						SendDlgItemMessage(hDlg,IDC_RECORDOPTIONS_FILENAME,EM_SETSEL,0,-1);
						::MessageBox(hDlg,szMessage,NULL,MB_OK | MB_ICONEXCLAMATION);
						return TRUE;
					}
#endif
				}
				::lstrcpy(m_szSaveFolder,szSaveFolder);
				::lstrcpy(m_szFileName,szFileName);

				m_fConfirmChannelChange=
					DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_CONFIRMCHANNELCHANGE);
				m_fConfirmExit=
					DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_CONFIRMEXIT);
				m_fConfirmStop=
					DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_CONFIRMSTOP);
				m_fConfirmStopStatusBarOnly=
					DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_CONFIRMSTATUSBARSTOP);

				bool fOptionChanged=false;
				bool f=DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_CURSERVICEONLY);
				if (m_fCurServiceOnly!=f) {
					m_fCurServiceOnly=f;
					fOptionChanged=true;
				}
				f=DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_SAVESUBTITLE);
				if (m_fSaveSubtitle!=f) {
					m_fSaveSubtitle=f;
					fOptionChanged=true;
				}
				f=DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_SAVEDATACARROUSEL);
				if (m_fSaveDataCarrousel!=f) {
					m_fSaveDataCarrousel=f;
					fOptionChanged=true;
				}
				if (fOptionChanged)
					SetUpdateFlag(UPDATE_RECORDSTREAM);

				m_fAlertLowFreeSpace=
					DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_ALERTLOWFREESPACE);
				m_LowFreeSpaceThreshold=
					::GetDlgItemInt(hDlg,IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD,NULL,FALSE);

				int Sel=(int)DlgComboBox_GetCurSel(hDlg,IDC_RECORDOPTIONS_STATUSBARCOMMAND);
				if (Sel>=0 && Sel<lengthof(StatusBarCommandList))
					m_StatusBarRecordCommand=StatusBarCommandList[Sel];

				unsigned int BufferSize=
					::GetDlgItemInt(hDlg,IDC_RECORDOPTIONS_BUFFERSIZE,NULL,FALSE);
				if (BufferSize!=0)
					m_BufferSize=CLAMP(BufferSize*1024,WRITE_BUFFER_SIZE_MIN,WRITE_BUFFER_SIZE_MAX);

				BufferSize=::GetDlgItemInt(hDlg,IDC_RECORDOPTIONS_TIMESHIFTBUFFERSIZE,NULL,FALSE);
				if (BufferSize!=0) {
					BufferSize=CLAMP(BufferSize,TIMESHIFT_BUFFER_SIZE_MIN,TIMESHIFT_BUFFER_SIZE_MAX);
					if (BufferSize!=m_TimeShiftBufferSize) {
						m_TimeShiftBufferSize=BufferSize;
						SetUpdateFlag(UPDATE_TIMESHIFTBUFFER);
					}
				}

				BufferSize=::GetDlgItemInt(hDlg,IDC_RECORDOPTIONS_MAXPENDINGSIZE,NULL,FALSE);
				if (BufferSize!=0) {
					BufferSize=CLAMP(BufferSize*MEGA_BYTES,MAX_PENDING_SIZE_MIN,MAX_PENDING_SIZE_MAX);
					if (BufferSize!=m_MaxPendingSize) {
						m_MaxPendingSize=BufferSize;
						SetUpdateFlag(UPDATE_MAXPENDINGSIZE);
					}
				}

				m_fChanged=true;
			}
			break;
		}
		break;
	}

	return FALSE;
}
