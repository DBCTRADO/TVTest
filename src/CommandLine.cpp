#include "stdafx.h"
#include "TVTest.h"
#include "CommandLine.h"
//#include "AppMain.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




class CArgsParser
{
	LPWSTR *m_ppszArgList;
	int m_Args;
	int m_CurPos;

public:
	CArgsParser(LPCWSTR pszCmdLine);
	~CArgsParser();
	bool IsSwitch() const;
	bool IsOption(LPCWSTR pszOption) const;
	bool GetOption(LPCWSTR pszOption,bool *pValue);
	bool GetOption(LPCWSTR pszOption,TVTest::String *pValue);
	bool GetOption(LPCWSTR pszOption,LPTSTR pszValue,int MaxLength);
	bool GetOption(LPCWSTR pszOption,int *pValue);
	bool GetOption(LPCWSTR pszOption,DWORD *pValue);
	bool GetOption(LPCWSTR pszOption,FILETIME *pValue);
	bool GetDurationOption(LPCWSTR pszOption,int *pValue);
	bool IsEnd() const { return m_CurPos>=m_Args; }
	bool Next();
	LPCWSTR GetText() const;
	bool GetText(LPWSTR pszText,int MaxLength) const;
	bool GetValue(int *pValue) const;
	bool GetValue(DWORD *pValue) const;
	bool GetValue(FILETIME *pValue) const;
	bool GetDurationValue(int *pValue) const;
};


CArgsParser::CArgsParser(LPCWSTR pszCmdLine)
{
	m_ppszArgList=::CommandLineToArgvW(pszCmdLine,&m_Args);
	if (m_ppszArgList==0)
		m_Args=0;
	m_CurPos=0;
}


CArgsParser::~CArgsParser()
{
	if (m_ppszArgList)
		::LocalFree(m_ppszArgList);
}


bool CArgsParser::IsSwitch() const
{
	if (IsEnd())
		return false;
	return m_ppszArgList[m_CurPos][0]==L'-'
		|| m_ppszArgList[m_CurPos][0]==L'/';
}


bool CArgsParser::IsOption(LPCWSTR pszOption) const
{
	if (IsEnd())
		return false;
	return ::lstrcmpi(m_ppszArgList[m_CurPos]+1,pszOption)==0;
}


bool CArgsParser::GetOption(LPCWSTR pszOption,bool *pValue)
{
	if (IsOption(pszOption)) {
		*pValue=true;
		return true;
	}
	return false;
}


bool CArgsParser::GetOption(LPCWSTR pszOption,TVTest::String *pValue)
{
	if (IsOption(pszOption)) {
		if (Next()) {
			TVTest::StringUtility::Assign(*pValue,GetText());
			return true;
		}
	}
	return false;
}


bool CArgsParser::GetOption(LPCWSTR pszOption,LPTSTR pszValue,int MaxLength)
{
	if (IsOption(pszOption)) {
		if (Next())
			return GetText(pszValue,MaxLength);
	}
	return false;
}


bool CArgsParser::GetOption(LPCWSTR pszOption,int *pValue)
{
	if (IsOption(pszOption)) {
		if (Next())
			return GetValue(pValue);
	}
	return false;
}


bool CArgsParser::GetOption(LPCWSTR pszOption,DWORD *pValue)
{
	if (IsOption(pszOption)) {
		if (Next())
			return GetValue(pValue);
	}
	return false;
}


bool CArgsParser::GetOption(LPCWSTR pszOption,FILETIME *pValue)
{
	if (IsOption(pszOption)) {
		if (Next())
			return GetValue(pValue);
	}
	return false;
}


bool CArgsParser::GetDurationOption(LPCWSTR pszOption,int *pValue)
{
	if (IsOption(pszOption)) {
		if (Next())
			return GetDurationValue(pValue);
	}
	return false;
}


bool CArgsParser::Next()
{
	m_CurPos++;
	return m_CurPos<m_Args;
}


LPCWSTR CArgsParser::GetText() const
{
	if (IsEnd())
		return L"";
	return m_ppszArgList[m_CurPos];
}


bool CArgsParser::GetText(LPWSTR pszText,int MaxLength) const
{
	if (IsEnd())
		return false;
	if (::lstrlen(m_ppszArgList[m_CurPos])>=MaxLength)
		return false;
	::lstrcpy(pszText,m_ppszArgList[m_CurPos]);
	return true;
}


bool CArgsParser::GetValue(int *pValue) const
{
	if (IsEnd())
		return false;
	*pValue=_wtoi(m_ppszArgList[m_CurPos]);
	return true;
}


bool CArgsParser::GetValue(DWORD *pValue) const
{
	if (IsEnd())
		return false;
	*pValue=wcstoul(m_ppszArgList[m_CurPos],NULL,0);
	return true;
}


bool CArgsParser::GetValue(FILETIME *pValue) const
{
	if (IsEnd())
		return false;

	/*
		���t�Ǝ����̃p�[�X���s��
		Y-M-DTh:m:s �� Y/M/D-h:m:s �Ȃǂ��A�o�E�g�Ɏ󂯕t����
		Y�AM�As�͏ȗ��\
		���t�݂̂��w�肵���ꍇ�A���̓���0��0��0�b����Ƃ���
		�����݂̂��w�肵���ꍇ�A���ɂ��̎��������鎞�Ƃ���
	*/
	SYSTEMTIME CurTime,Time;
	::GetLocalTime(&CurTime);
	::ZeroMemory(&Time,sizeof(Time));

	WORD Date[3],TimeValue[3];
	int DateCount=0,TimeCount=0;
	UINT Value=0;
	for (LPCWSTR p=m_ppszArgList[m_CurPos];;p++) {
		if (*p>=L'0' && *p<=L'9') {
			Value=Value*10+(*p-L'0');
			if (Value>0xFFFF)
				return false;
		} else {
			if (*p==L'/' || *p==L'-' || *p==L'T') {
				if (DateCount==3)
					return false;
				Date[DateCount++]=(WORD)Value;
			} else if (*p==L':' || *p==L'\0') {
				if (TimeCount==3)
					return false;
				TimeValue[TimeCount++]=(WORD)Value;
				if (*p==L'\0')
					break;
			}
			Value=0;
		}
	}

	if ((DateCount==0 && TimeCount<2) || TimeCount==1)
		return false;

	int i=0;
	if (DateCount>2) {
		Time.wYear=Date[i++];
		if (Time.wYear<100)
			Time.wYear+=CurTime.wYear/100*100;
	}
	if (DateCount>1) {
		Time.wMonth=Date[i++];
		if (Time.wMonth<1 || Time.wMonth>12)
			return false;
	}
	if (DateCount>0) {
		Time.wDay=Date[i];
		if (Time.wDay<1 || Time.wDay>31)
			return false;
	}
	if (Time.wYear==0) {
		Time.wYear=CurTime.wYear;
		if (Time.wMonth==0) {
			Time.wMonth=CurTime.wMonth;
			if (Time.wDay==0) {
				Time.wDay=CurTime.wDay;
			} else if (Time.wDay<CurTime.wDay) {
				Time.wMonth++;
				if (Time.wMonth>12) {
					Time.wMonth=1;
					Time.wYear++;
				}
			}
		} else if (Time.wMonth<CurTime.wMonth) {
			Time.wYear++;
		}
	}

	if (TimeCount>0) {
		Time.wHour=TimeValue[0];
		if (TimeCount>1) {
			Time.wMinute=TimeValue[1];
			if (Time.wMinute>59)
				return false;
			if (TimeCount>2) {
				Time.wSecond=TimeValue[2];
				if (Time.wSecond>59)	// Windows�ɉ[�b�͖����炵��
					return false;
			}
		}
	}
	if (DateCount==0) {
		if (Time.wHour<CurTime.wHour)
			Time.wHour+=24;
	}

	SYSTEMTIME st;
	FILETIME ft;
	::ZeroMemory(&st,sizeof(st));
	st.wYear=Time.wYear;
	st.wMonth=Time.wMonth;
	st.wDay=Time.wDay;
	if (!::SystemTimeToFileTime(&st,&ft))
		return false;

	ft+=(LONGLONG)Time.wHour*FILETIME_HOUR+
		(LONGLONG)Time.wMinute*FILETIME_MINUTE+
		(LONGLONG)Time.wSecond*FILETIME_SECOND;

	*pValue=ft;

	return true;
}


bool CArgsParser::GetDurationValue(int *pValue) const
{
	if (IsEnd())
		return false;

	// ?h?m?s �`���̎��Ԏw����p�[�X����
	// �P�ʂ̎w�肪�����ꍇ�͕b�P�ʂƉ��߂���
	LPCWSTR p=m_ppszArgList[m_CurPos];
	int DurationSec=0,Duration=0;

	while (*p!=L'\0') {
		if (*p==L'-' || (*p>=L'0' && *p<=L'9')) {
			Duration=wcstol(p,(wchar_t**)&p,10);
			if (Duration==LONG_MAX || Duration==LONG_MIN)
				return false;
		} else {
			switch (*p) {
			case L'h': case L'H':
				DurationSec+=Duration*(60*60);
				break;
			case L'm': case L'M':
				DurationSec+=Duration*60;
				break;
			case L's': case L'S':
				DurationSec+=Duration;
				break;
			}
			Duration=0;
			p++;
		}
	}
	DurationSec+=Duration;
	*pValue=DurationSec;

	return true;
}


static bool GetIniEntry(LPCWSTR pszText,CCommandLineOptions::IniEntry *pEntry)
{
	LPCWSTR p=pszText;

	if (*p==L'[') {
		p++;
		LPCWSTR pEnd=::StrChrW(p,L']');
		if (pEnd==NULL)
			return false;
		pEntry->Section.assign(p,pEnd-p);
		p=pEnd+1;
	}

	LPCWSTR pEnd=::StrChrW(p,L'=');
	if (pEnd==NULL || pEnd-p<1)
		return false;
	pEntry->Name.assign(p,pEnd-p);
	p=pEnd+1;
	pEntry->Value.assign(p);

	return true;
}




CCommandLineOptions::CCommandLineOptions()
	: m_fNoDriver(false)
	, m_fNoTSProcessor(false)
	, m_fSingleTask(false)
	, m_fStandby(false)
	, m_fNoView(false)
	, m_fNoDirectShow(false)
	, m_fMpeg2(false)
	, m_fH264(false)
	, m_fH265(false)
	, m_fSilent(false)
	, m_fInitialSettings(false)
	, m_fSaveLog(false)
	, m_fNoEpg(false)
	, m_f1Seg(false)
	, m_TvRockDID(-1)

	, m_Channel(0)
	, m_ControllerChannel(0)
	, m_TuningSpace(-1)
	, m_ServiceID(0)
	, m_NetworkID(0)
	, m_TransportStreamID(0)

	, m_fUseNetworkRemocon(false)
	, m_UDPPort(1234)

	, m_fRecord(false)
	, m_fRecordStop(false)
	, m_RecordStartTime(FILETIME_NULL)
	, m_RecordDelay(0)
	, m_RecordDuration(0)
	, m_fRecordCurServiceOnly(false)
	, m_fExitOnRecordEnd(false)
	, m_fRecordOnly(false)

	, m_fFullscreen(false)
	, m_fMinimize(false)
	, m_fMaximize(false)
	, m_fTray(false)
	, m_WindowLeft(INVALID_WINDOW_POS)
	, m_WindowTop(INVALID_WINDOW_POS)
	, m_WindowWidth(0)
	, m_WindowHeight(0)

	, m_Volume(-1)
	, m_fMute(false)

	, m_fNoPlugin(false)

	, m_fShowProgramGuide(false)
	, m_fProgramGuideOnly(false)

	, m_fHomeDisplay(false)
	, m_fChannelDisplay(false)
{
}


/*
	���p�\�ȃR�}���h���C���I�v�V����

	/ch				�`�����l�� (e.g. /ch 13)
	/chspace		�`���[�j���O��� (e.g. /chspace 1)
	/d				�h���C�o�̎w�� (e.g. /d BonDriver.dll)
	/f /fullscreen	�t���X�N���[��
	/ini			INI�t�@�C����
	/init			�����ݒ�_�C�A���O��\������
	/inikey			INI�t�@�C���̒l��ݒ�
	/log			�I�����Ƀ��O��ۑ�����
	/max			�ő剻��ԂŋN��
	/min			�ŏ�����ԂŋN��
	/tray			�N�����Ƀ^�X�N�g���C�Ɋi�[
	/posx			�E�B���h�E�̍��ʒu�̎w��
	/posy			�E�B���h�E�̏�ʒu�̎w��
	/width			�E�B���h�E�̕��̎w��
	/height			�E�B���h�E�̍����̎w��
	/mute			����
	/nd				TS�v���Z�b�T�[�𖳌��ɂ���
	/nid			�l�b�g���[�NID
	/nodriver		BonDriver��ǂݍ��܂Ȃ�
	/nodshow		DirectShow�̏����������Ȃ�
	/noplugin		�v���O�C����ǂݍ��܂Ȃ�
	/noview			�v���r���[����
	/mpeg2			MPEG-2��L��
	/h264			H.264��L��
	/h265			H.265��L��
	/1seg			�����Z�O���[�h
	/nr				�l�b�g���[�N�����R�����g�p����
	/p /port		UDP �̃|�[�g�ԍ� (e.g. /p 1234)
	/plugin-		�w�肳�ꂽ�v���O�C����ǂݍ��܂Ȃ�
	/plugindir		�v���O�C���̃t�H���_
	/rch			�����R���`�����l��
	/rec			�^��
	/reccurservice	���݂̃T�[�r�X�̂ݘ^��
	/recstarttime	�^��J�n����
	/recdelay		�^��܂ł̎���
	/recduration	�^�掞��
	/recexit		�^��I�����Ƀv���O�������I��
	/recfile		�^��t�@�C����
	/reconly		�^���p���[�h
	/recstop		�^���~
	/s				�����N�����Ȃ�
	/sid			�T�[�r�XID
	/silent			�G���[���Ƀ_�C�A���O��\�����Ȃ�
	/standby		�ҋ@��ԂŋN��
	/tsid			�g�����X�|�[�g�X�g���[��ID
	/volume			����
	/noepg			EPG���̎擾���s��Ȃ�
	/epg			EPG�ԑg�\��\������
	/epgonly		EPG�ԑg�\�̂ݕ\������
	/epgtuner		EPG�ԑg�\�̃f�t�H���g�`���[�i�[
	/epgspace		EPG�ԑg�\�̃f�t�H���g�`���[�j���O���
	/home			�z�[����ʕ\��
	/chdisplay		�`�����l���I����ʕ\��
	/style			�X�^�C���t�@�C����
*/
void CCommandLineOptions::Parse(LPCWSTR pszCmdLine)
{
	CArgsParser Args(pszCmdLine);

	if (Args.IsEnd())
		return;
	do {
		if (Args.IsSwitch()) {
			if (!Args.GetOption(TEXT("1seg"),&m_f1Seg)
					&& !Args.GetOption(TEXT("ch"),&m_Channel)
					&& !Args.GetOption(TEXT("chdisplay"),&m_fChannelDisplay)
					&& !Args.GetOption(TEXT("chspace"),&m_TuningSpace)
					&& !Args.GetOption(TEXT("d"),&m_DriverName)
					&& !Args.GetOption(TEXT("epg"),&m_fShowProgramGuide)
					&& !Args.GetOption(TEXT("epgonly"),&m_fProgramGuideOnly)
					&& !Args.GetOption(TEXT("epgspace"),&m_ProgramGuideSpace)
					&& !Args.GetOption(TEXT("epgtuner"),&m_ProgramGuideTuner)
					&& !Args.GetOption(TEXT("f"),&m_fFullscreen)
					&& !Args.GetOption(TEXT("fullscreen"),&m_fFullscreen)
					&& !Args.GetOption(TEXT("h264"),&m_fH264)
					&& !Args.GetOption(TEXT("h265"),&m_fH265)
					&& !Args.GetOption(TEXT("height"),&m_WindowHeight)
					&& !Args.GetOption(TEXT("home"),&m_fHomeDisplay)
					&& !Args.GetOption(TEXT("ini"),&m_IniFileName)
					&& !Args.GetOption(TEXT("init"),&m_fInitialSettings)
					&& !Args.GetOption(TEXT("log"),&m_fSaveLog)
					&& !Args.GetOption(TEXT("max"),&m_fMaximize)
					&& !Args.GetOption(TEXT("min"),&m_fMinimize)
					&& !Args.GetOption(TEXT("mpeg2"),&m_fMpeg2)
					&& !Args.GetOption(TEXT("mute"),&m_fMute)
					&& !Args.GetOption(TEXT("nd"),&m_fNoTSProcessor)
					&& !Args.GetOption(TEXT("nodriver"),&m_fNoDriver)
					&& !Args.GetOption(TEXT("nodshow"),&m_fNoDirectShow)
					&& !Args.GetOption(TEXT("noepg"),&m_fNoEpg)
					&& !Args.GetOption(TEXT("noplugin"),&m_fNoPlugin)
					&& !Args.GetOption(TEXT("noview"),&m_fNoView)
					&& !Args.GetOption(TEXT("nr"),&m_fUseNetworkRemocon)
					&& !Args.GetOption(TEXT("nid"),&m_NetworkID)
					&& !Args.GetOption(TEXT("p"),&m_UDPPort)
					&& !Args.GetOption(TEXT("port"),&m_UDPPort)
					&& !Args.GetOption(TEXT("posx"),&m_WindowLeft)
					&& !Args.GetOption(TEXT("posy"),&m_WindowTop)
					&& !Args.GetOption(TEXT("plugindir"),&m_PluginsDirectory)
					&& !Args.GetOption(TEXT("pluginsdir"),&m_PluginsDirectory)
					&& !Args.GetOption(TEXT("rec"),&m_fRecord)
					&& !Args.GetOption(TEXT("reccurservice"),&m_fRecordCurServiceOnly)
					&& !Args.GetOption(TEXT("recstarttime"),&m_RecordStartTime)
					&& !Args.GetDurationOption(TEXT("recdelay"),&m_RecordDelay)
					&& !Args.GetDurationOption(TEXT("recduration"),&m_RecordDuration)
					&& !Args.GetOption(TEXT("recexit"),&m_fExitOnRecordEnd)
					&& !Args.GetOption(TEXT("recfile"),&m_RecordFileName)
					&& !Args.GetOption(TEXT("reconly"),&m_fRecordOnly)
					&& !Args.GetOption(TEXT("recstop"),&m_fRecordStop)
					&& !Args.GetOption(TEXT("rch"),&m_ControllerChannel)
					&& !Args.GetOption(TEXT("s"),&m_fSingleTask)
					&& !Args.GetOption(TEXT("sid"),&m_ServiceID)
					&& !Args.GetOption(TEXT("silent"),&m_fSilent)
					&& !Args.GetOption(TEXT("standby"),&m_fStandby)
					&& !Args.GetOption(TEXT("style"),&m_StyleFileName)
					&& !Args.GetOption(TEXT("tray"),&m_fTray)
					&& !Args.GetOption(TEXT("tsid"),&m_TransportStreamID)
					&& !Args.GetOption(TEXT("volume"),&m_Volume)
					&& !Args.GetOption(TEXT("width"),&m_WindowWidth)) {
				if (Args.IsOption(TEXT("inikey"))) {
					if (Args.Next()) {
						IniEntry Entry;
						if (GetIniEntry(Args.GetText(),&Entry))
							m_IniValueList.push_back(Entry);
					}
				} else if (Args.IsOption(TEXT("plugin-"))) {
					if (Args.Next()) {
						TCHAR szPlugin[MAX_PATH];
						if (Args.GetText(szPlugin,MAX_PATH))
							m_NoLoadPlugins.push_back(TVTest::String(szPlugin));
					}
				} else if (Args.IsOption(TEXT("did"))) {
					if (Args.Next()) {
						const WCHAR DID=Args.GetText()[0];

						if (DID>=L'A' && DID<=L'Z')
							m_TvRockDID=DID-L'A';
						else if (DID>=L'a' && DID<=L'z')
							m_TvRockDID=DID-L'a';
					}
				}
#ifdef _DEBUG
				else {
					TRACE(TEXT("Unknown command line option %s\n"),Args.GetText());
					// �v���O�C���ŉ��߂���I�v�V����������̂Łc
					//GetAppClass().AddLong(TEXT("�s���ȃR�}���h���C���I�v�V���� %s �𖳎����܂��B"),Args.GetText());
				}
#endif
			}
		} else {
			// �Ȃ���udp://@:1234�̂悤�Ƀ|�[�g���w��ł���Ǝv���Ă���l�������̂ŁA�Ή����Ă���
			if (::wcsncmp(Args.GetText(),L"udp://@:",8)==0)
				m_UDPPort=::_wtoi(Args.GetText()+8);
		}
	} while (Args.Next());
	if (m_fRecordOnly) {
		m_fNoDirectShow=true;
	}

/*
#ifdef _DEBUG
	// �R�}���h���C���̉�͂̃e�X�g
	{
		CArgsParser Args(L"/d 120 /d -45 /d 1h30m5s /t 2011/12/25-1:30:25 /t 1:30 /t 12/25 /t 12/25-1:30 /t 12/25-26:25");
		do {
			if (Args.IsSwitch()) {
				int Duration;
				FILETIME Time;
				if (Args.GetDurationOption(L"d",&Duration)) {
					TRACE(L"Commandline parse test : \"%s\" %d\n",
						  Args.GetText(),Duration);
				} else if (Args.GetOption(L"t",&Time)) {
					SYSTEMTIME st;
					::FileTimeToSystemTime(&Time,&st);
					TRACE(L"Commandline parse test : \"%s\" %d/%d/%d %d:%d:%d\n",
						  Args.GetText(),
						  st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
				}
			}
		} while (Args.Next());
	}
#endif
*/
}


bool CCommandLineOptions::IsChannelSpecified() const
{
	return m_Channel>0 || m_ControllerChannel>0 || m_ServiceID>0
		|| m_NetworkID>0 || m_TransportStreamID>0;
}
