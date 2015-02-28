#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "Accelerator.h"
#include "Menu.h"
#include "DialogUtil.h"
#include "MessageDialog.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define MAKE_ACCEL_PARAM(key,mod,global,appcommand) \
	(((key)<<16)|((mod)<<8)|((global)?0x80:0x00)|(appcommand))
#define GET_ACCEL_KEY(param)		((WORD)((param)>>16))
#define GET_ACCEL_MOD(param)		((BYTE)(((param)>>8)&0xFF))
#define GET_ACCEL_GLOBAL(param)		(((param)&0x80)!=0)
#define GET_ACCEL_APPCOMMAND(param)	((BYTE)((param)&0x7F))


static const struct {
	unsigned int KeyCode;
	LPCTSTR pszText;
} AccelKeyList[] = {
	{VK_BACK,		TEXT("BS")},
	{VK_TAB,		TEXT("Tab")},
	{VK_CLEAR,		TEXT("Clear")},
	{VK_RETURN,		TEXT("Enter")},
	{VK_PAUSE,		TEXT("Pause")},
	{VK_ESCAPE,		TEXT("Esc")},
	{VK_SPACE,		TEXT("Space")},
	{VK_PRIOR,		TEXT("PgUp")},
	{VK_NEXT,		TEXT("PgDown")},
	{VK_END,		TEXT("End")},
	{VK_HOME,		TEXT("Home")},
	{VK_LEFT,		TEXT("��")},
	{VK_UP,			TEXT("��")},
	{VK_RIGHT,		TEXT("��")},
	{VK_DOWN,		TEXT("��")},
	{VK_SELECT,		TEXT("Select")},
	{VK_PRINT,		TEXT("Print")},
	{VK_EXECUTE,	TEXT("Execute")},
	{VK_INSERT,		TEXT("Ins")},
	{VK_DELETE,		TEXT("Del")},
	{VK_HELP,		TEXT("Help")},
	{'0',			TEXT("0")},
	{'1',			TEXT("1")},
	{'2',			TEXT("2")},
	{'3',			TEXT("3")},
	{'4',			TEXT("4")},
	{'5',			TEXT("5")},
	{'6',			TEXT("6")},
	{'7',			TEXT("7")},
	{'8',			TEXT("8")},
	{'9',			TEXT("9")},
	{'A',			TEXT("A")},
	{'B',			TEXT("B")},
	{'C',			TEXT("C")},
	{'D',			TEXT("D")},
	{'E',			TEXT("E")},
	{'F',			TEXT("F")},
	{'G',			TEXT("G")},
	{'H',			TEXT("H")},
	{'I',			TEXT("I")},
	{'J',			TEXT("J")},
	{'K',			TEXT("K")},
	{'L',			TEXT("L")},
	{'M',			TEXT("M")},
	{'N',			TEXT("N")},
	{'O',			TEXT("O")},
	{'P',			TEXT("P")},
	{'Q',			TEXT("Q")},
	{'R',			TEXT("R")},
	{'S',			TEXT("S")},
	{'T',			TEXT("T")},
	{'U',			TEXT("U")},
	{'V',			TEXT("V")},
	{'W',			TEXT("W")},
	{'X',			TEXT("X")},
	{'Y',			TEXT("Y")},
	{'Z',			TEXT("Z")},
	{VK_OEM_MINUS,	TEXT("-")},
	{VK_OEM_7,		TEXT("^")},
	{VK_OEM_5,		TEXT("\\")},
	{VK_OEM_3,		TEXT("@")},
	{VK_OEM_4,		TEXT("[")},
	{VK_OEM_PLUS,	TEXT(";")},
	{VK_OEM_1,		TEXT(":")},
	{VK_OEM_6,		TEXT("]")},
	{VK_OEM_COMMA,	TEXT(",")},
	{VK_OEM_PERIOD,	TEXT(".")},
	{VK_OEM_2,		TEXT("/")},
	{VK_OEM_102,	TEXT("�_")},
	{VK_NUMPAD0,	TEXT("Num0")},
	{VK_NUMPAD1,	TEXT("Num1")},
	{VK_NUMPAD2,	TEXT("Num2")},
	{VK_NUMPAD3,	TEXT("Num3")},
	{VK_NUMPAD4,	TEXT("Num4")},
	{VK_NUMPAD5,	TEXT("Num5")},
	{VK_NUMPAD6,	TEXT("Num6")},
	{VK_NUMPAD7,	TEXT("Num7")},
	{VK_NUMPAD8,	TEXT("Num8")},
	{VK_NUMPAD9,	TEXT("Num9")},
	{VK_MULTIPLY,	TEXT("Num*")},
	{VK_ADD,		TEXT("Num+")},
	{VK_SUBTRACT,	TEXT("Num-")},
	{VK_DECIMAL,	TEXT("Num.")},
	{VK_DIVIDE,		TEXT("Num/")},
	{VK_F1,			TEXT("F1")},
	{VK_F2,			TEXT("F2")},
	{VK_F3,			TEXT("F3")},
	{VK_F4,			TEXT("F4")},
	{VK_F5,			TEXT("F5")},
	{VK_F6,			TEXT("F6")},
	{VK_F7,			TEXT("F7")},
	{VK_F8,			TEXT("F8")},
	{VK_F9,			TEXT("F9")},
	{VK_F10,		TEXT("F10")},
	{VK_F11,		TEXT("F11")},
	{VK_F12,		TEXT("F12")},
};

static const struct {
	int Command;
	LPCTSTR pszText;
} AppCommandList[] = {
	{APPCOMMAND_VOLUME_UP,				TEXT("����+")},
	{APPCOMMAND_VOLUME_DOWN,			TEXT("����-")},
	{APPCOMMAND_VOLUME_MUTE,			TEXT("����")},
	{APPCOMMAND_MEDIA_PLAY_PAUSE,		TEXT("�Đ�/�ꎞ��~")},
	{APPCOMMAND_MEDIA_PLAY,				TEXT("�Đ�")},
	{APPCOMMAND_MEDIA_PAUSE,			TEXT("�ꎞ��~")},
	{APPCOMMAND_MEDIA_STOP,				TEXT("��~")},
	{APPCOMMAND_MEDIA_RECORD,			TEXT("�^��/�^��")},
	{APPCOMMAND_MEDIA_CHANNEL_UP,		TEXT("�`�����l��+")},
	{APPCOMMAND_MEDIA_CHANNEL_DOWN,		TEXT("�`�����l��-")},
	{APPCOMMAND_MEDIA_PREVIOUSTRACK,	TEXT("�O�̃g���b�N")},
	{APPCOMMAND_MEDIA_NEXTTRACK,		TEXT("���̃g���b�N")},
	{APPCOMMAND_MEDIA_REWIND,			TEXT("�����߂�")},
	{APPCOMMAND_MEDIA_FAST_FORWARD,		TEXT("������")},
	{APPCOMMAND_BROWSER_BACKWARD,		TEXT("�߂�")},
	{APPCOMMAND_BROWSER_FORWARD,		TEXT("�i��")},
	{APPCOMMAND_BROWSER_HOME,			TEXT("�z�[��")},
	{APPCOMMAND_BROWSER_STOP,			TEXT("���~")},
	{APPCOMMAND_BROWSER_FAVORITES,		TEXT("���C�ɓ���")},
	{APPCOMMAND_BROWSER_REFRESH,		TEXT("�X�V")},
	{APPCOMMAND_BROWSER_SEARCH,			TEXT("Web����")},
	{APPCOMMAND_NEW,					TEXT("�V�K�쐬")},
	{APPCOMMAND_OPEN,					TEXT("�J��")},
	{APPCOMMAND_SAVE,					TEXT("�ۑ�")},
	{APPCOMMAND_PRINT,					TEXT("���")},
	{APPCOMMAND_CLOSE,					TEXT("����")},
	{APPCOMMAND_UNDO,					TEXT("���ɖ߂�")},
	{APPCOMMAND_REDO,					TEXT("��蒼��")},
	{APPCOMMAND_CUT,					TEXT("�؂���")},
	{APPCOMMAND_COPY,					TEXT("�R�s�[")},
	{APPCOMMAND_PASTE,					TEXT("�\��t��")},
	{APPCOMMAND_FIND,					TEXT("����")},
	{APPCOMMAND_HELP,					TEXT("�w���v")},
	{APPCOMMAND_CORRECTION_LIST,		TEXT("�������")},
	{APPCOMMAND_SPELL_CHECK,			TEXT("�X�y���`�F�b�N")},
	{APPCOMMAND_BASS_BOOST,				TEXT("Bass boost")},
	{APPCOMMAND_BASS_DOWN,				TEXT("Bass down")},
	{APPCOMMAND_BASS_UP,				TEXT("Bass up")},
	{APPCOMMAND_TREBLE_DOWN,			TEXT("Treble down")},
	{APPCOMMAND_TREBLE_UP,				TEXT("Treble up")},
	{APPCOMMAND_MIC_ON_OFF_TOGGLE,		TEXT("Mic on/off")},
	{APPCOMMAND_MICROPHONE_VOLUME_UP,	TEXT("Mic volume up")},
	{APPCOMMAND_MICROPHONE_VOLUME_DOWN,	TEXT("Mic volume down")},
	{APPCOMMAND_MICROPHONE_VOLUME_MUTE,	TEXT("Mic volume mute")},
	{APPCOMMAND_LAUNCH_MEDIA_SELECT,	TEXT("Media Select")},
	{APPCOMMAND_LAUNCH_APP1,			TEXT("Launch App1")},
	{APPCOMMAND_LAUNCH_APP2,			TEXT("Launch App2")},
	{APPCOMMAND_LAUNCH_MAIL,			TEXT("Launch mail")},
	{APPCOMMAND_FORWARD_MAIL,			TEXT("Forward mail")},
	{APPCOMMAND_SEND_MAIL,				TEXT("Send mail")},
	{APPCOMMAND_REPLY_TO_MAIL,			TEXT("Reply to mail")},
	{APPCOMMAND_DICTATE_OR_COMMAND_CONTROL_TOGGLE,	TEXT("Dictate or Command/Control")},
};


const CAccelerator::KeyInfo CAccelerator::m_DefaultAccelList[] = {
//	{CM_ZOOM_100,			VK_HOME,		0,			false},
	{CM_ASPECTRATIO,		'A',			0,			false},
	{CM_FULLSCREEN,			VK_RETURN,		MOD_ALT,	false},
	{CM_ALWAYSONTOP,		'T',			0,			false},
	{CM_VOLUME_MUTE,		'M',			0,			false},
	{CM_VOLUME_UP,			VK_UP,			0,			false},
	{CM_VOLUME_DOWN,		VK_DOWN,		0,			false},
	{CM_SWITCHAUDIO,		'S',			0,			false},
	{CM_CHANNEL_DOWN,		VK_LEFT,		0,			false},
	{CM_CHANNEL_UP,			VK_RIGHT,		0,			false},
	{CM_RECORD,				'R',			0,			false},
	{CM_COPY,				'C',			0,			false},
	{CM_SAVEIMAGE,			'V',			0,			false},
	{CM_PANEL,				'P',			0,			false},
	{CM_PROGRAMGUIDE,		'E',			0,			false},
	{CM_HOMEDISPLAY,		VK_HOME,		MOD_ALT,	false},
	{CM_CHANNELDISPLAY,		'Z',			0,			false},
};

const CAccelerator::AppCommandInfo CAccelerator::m_DefaultAppCommandList[] = {
	{CM_VOLUME_UP,			MEDIAKEY_APPCOMMAND,	APPCOMMAND_VOLUME_UP},
	{CM_VOLUME_DOWN,		MEDIAKEY_APPCOMMAND,	APPCOMMAND_VOLUME_DOWN},
	{CM_VOLUME_MUTE,		MEDIAKEY_APPCOMMAND,	APPCOMMAND_VOLUME_MUTE},
	{CM_RECORD_STOP,		MEDIAKEY_APPCOMMAND,	APPCOMMAND_MEDIA_STOP},
	{CM_RECORD_PAUSE,		MEDIAKEY_APPCOMMAND,	APPCOMMAND_MEDIA_PAUSE},
	{CM_RECORD_START,		MEDIAKEY_APPCOMMAND,	APPCOMMAND_MEDIA_RECORD},
	{CM_CHANNEL_UP,			MEDIAKEY_APPCOMMAND,	APPCOMMAND_MEDIA_CHANNEL_UP},
	{CM_CHANNEL_DOWN,		MEDIAKEY_APPCOMMAND,	APPCOMMAND_MEDIA_CHANNEL_DOWN},
	/*
	{CM_CHANNEL_UP,			MEDIAKEY_APPCOMMAND,	APPCOMMAND_MEDIA_NEXTTRACK},
	{CM_CHANNEL_DOWN,		MEDIAKEY_APPCOMMAND,	APPCOMMAND_MEDIA_PREVIOUSTRACK},
	*/
	{CM_CHANNEL_BACKWARD,	MEDIAKEY_APPCOMMAND,	APPCOMMAND_BROWSER_BACKWARD},
	{CM_CHANNEL_FORWARD,	MEDIAKEY_APPCOMMAND,	APPCOMMAND_BROWSER_FORWARD},
	{CM_HOMEDISPLAY,		MEDIAKEY_APPCOMMAND,	APPCOMMAND_BROWSER_HOME},
	{CM_CLOSE,				MEDIAKEY_APPCOMMAND,	APPCOMMAND_CLOSE},
	{CM_SAVEIMAGE,			MEDIAKEY_APPCOMMAND,	APPCOMMAND_SAVE},
	{CM_COPY,				MEDIAKEY_APPCOMMAND,	APPCOMMAND_COPY},
};


CAccelerator::CAccelerator()
{
	m_hAccel=NULL;
	m_KeyList.resize(lengthof(m_DefaultAccelList));
	for (size_t i=0;i<lengthof(m_DefaultAccelList);i++)
		m_KeyList[i]=m_DefaultAccelList[i];
	m_MediaKeyList.resize(lengthof(AppCommandList)+m_RawInput.NumKeyTypes());
	MediaKeyInfo Info;
	Info.Type=MEDIAKEY_APPCOMMAND;
	for (size_t i=0;i<lengthof(AppCommandList);i++) {
		Info.Command=AppCommandList[i].Command;
		Info.pszText=AppCommandList[i].pszText;
		m_MediaKeyList[i]=Info;
	}
	Info.Type=MEDIAKEY_RAWINPUT;
	for (int i=0;i<m_RawInput.NumKeyTypes();i++) {
		Info.Command=m_RawInput.GetKeyData(i);
		Info.pszText=m_RawInput.GetKeyText(i);
		m_MediaKeyList[lengthof(AppCommandList)+i]=Info;
	}
	m_AppCommandList.resize(lengthof(m_DefaultAppCommandList));
	for (size_t i=0;i<lengthof(m_DefaultAppCommandList);i++)
		m_AppCommandList[i]=m_DefaultAppCommandList[i];
	m_hwndHotKey=NULL;
	m_pMainMenu=NULL;
	m_pCommandList=NULL;
	m_fRegisterHotKey=false;
}


CAccelerator::~CAccelerator()
{
	Destroy();
	Finalize();
}


void CAccelerator::FormatAccelText(LPTSTR pszText,int Key,int Modifiers,bool fGlobal)
{
	pszText[0]='\0';
	if ((Modifiers&MOD_SHIFT)!=0)
		::lstrcat(pszText,TEXT("Shift+"));
	if ((Modifiers&MOD_CONTROL)!=0)
		::lstrcat(pszText,TEXT("Ctrl+"));
	if ((Modifiers&MOD_ALT)!=0)
		::lstrcat(pszText,TEXT("Alt+"));
	for (int i=0;i<lengthof(AccelKeyList);i++) {
		if (Key==AccelKeyList[i].KeyCode) {
			::lstrcat(pszText,AccelKeyList[i].pszText);
			break;
		}
	}
	if (fGlobal)
		::lstrcat(pszText,TEXT(" (G)"));
}


void CAccelerator::SetMenuAccelText(HMENU hmenu,int Command)
{
	TCHAR szText[128],*p;
	size_t i;
	const KeyInfo *pKey=NULL;

	GetMenuString(hmenu,Command,szText,lengthof(szText),MF_BYCOMMAND);
	for (i=0;i<m_KeyList.size();i++) {
		if (m_KeyList[i].Command==Command) {
			pKey=&m_KeyList[i];
			break;
		}
	}
	p=szText;
	while (*p!='\t') {
		if (*p=='\0') {
			if (pKey==NULL)
				return;
			break;
		}
		p++;
	}
	if (pKey!=NULL) {
		p[0]='\t';
		FormatAccelText(p+1,pKey->KeyCode,pKey->Modifiers);
	} else
		*p='\0';
	ModifyMenu(hmenu,Command,
		MF_BYCOMMAND | MFT_STRING | GetMenuState(hmenu,Command,MF_BYCOMMAND),
		Command,szText);
}


HACCEL CAccelerator::CreateAccel()
{
	if (m_KeyList.size()==0)
		return NULL;

	LPACCEL paccl;
	int j;

	paccl=new ACCEL[m_KeyList.size()];
	j=0;
	for (size_t i=0;i<m_KeyList.size();i++) {
		if (!m_KeyList[i].fGlobal) {
			paccl[j].fVirt=FVIRTKEY;
			if ((m_KeyList[i].Modifiers&MOD_SHIFT)!=0)
				paccl[j].fVirt|=FSHIFT;
			if ((m_KeyList[i].Modifiers&MOD_CONTROL)!=0)
				paccl[j].fVirt|=FCONTROL;
			if ((m_KeyList[i].Modifiers&MOD_ALT)!=0)
				paccl[j].fVirt|=FALT;
			paccl[j].key=m_KeyList[i].KeyCode;
			paccl[j].cmd=m_KeyList[i].Command;
			j++;
		}
	}
	HACCEL hAccel=NULL;
	if (j>0)
		hAccel=::CreateAcceleratorTable(paccl,j);
	delete [] paccl;
	return hAccel;
}


bool CAccelerator::RegisterHotKey()
{
	if (m_fRegisterHotKey)
		return false;

	bool fOK=true;

	for (size_t i=0;i<m_KeyList.size();i++) {
		if (m_KeyList[i].fGlobal) {
			if (!::RegisterHotKey(m_hwndHotKey,
					(m_KeyList[i].Modifiers<<8)|m_KeyList[i].KeyCode,
					m_KeyList[i].Modifiers,m_KeyList[i].KeyCode)) {
				fOK=false;
			}
		}
	}
	m_fRegisterHotKey=true;
	return fOK;
}


bool CAccelerator::UnregisterHotKey()
{
	if (!m_fRegisterHotKey)
		return true;

	bool fOK=true;

	for (size_t i=0;i<m_KeyList.size();i++) {
		if (m_KeyList[i].fGlobal) {
			if (!::UnregisterHotKey(m_hwndHotKey,
					(m_KeyList[i].Modifiers<<8)|m_KeyList[i].KeyCode)) {
				fOK=false;
			}
		}
	}
	m_fRegisterHotKey=false;
	return fOK;
}


bool CAccelerator::LoadSettings(CSettings &Settings)
{
	if (Settings.SetSection(TEXT("Settings"))) {
		for (int i=0;i<=TVTest::CChannelInputOptions::KEY_LAST;i++) {
			TCHAR szKey[32];
			int Value;

			StdUtil::snprintf(szKey,lengthof(szKey),TEXT("ChInputKeyMode%d"),i);
			if (Settings.Read(szKey,&Value)) {
				m_ChannelInputOptions.KeyInputMode[i]=
					static_cast<TVTest::CChannelInputOptions::KeyInputModeType>(Value);
			} else {
				// ver.0.9.0 ���O�Ƃ̌݊��p
				static const LPCTSTR KeyList[] = {
					TEXT("DigitKeyChangeChannel"),
					TEXT("NumPadChangeChannel"),
					TEXT("FuncKeyChangeChannel")
				};
				bool f;

				if (Settings.Read(KeyList[i],&f)) {
					m_ChannelInputOptions.KeyInputMode[i]=
						f?TVTest::CChannelInputOptions::KEYINPUTMODE_SINGLEKEY:
						  TVTest::CChannelInputOptions::KEYINPUTMODE_DISABLED;
				}
			}
		}

		Settings.Read(TEXT("ChInputKeyTimeout"),&m_ChannelInputOptions.KeyTimeout);
		Settings.Read(TEXT("ChInputKeyTimeoutCancel"),&m_ChannelInputOptions.fKeyTimeoutCancel);
	}

	if (m_pCommandList==NULL)
		return true;

	if (Settings.SetSection(TEXT("Accelerator"))) {
		int NumAccel;

		if (Settings.Read(TEXT("AccelCount"),&NumAccel) && NumAccel>=0) {
			if (NumAccel>m_pCommandList->NumCommands())
				NumAccel=m_pCommandList->NumCommands();
			m_KeyList.clear();
			for (int i=0;i<NumAccel;i++) {
				TCHAR szName[64],szCommand[CCommandList::MAX_COMMAND_TEXT];
				int Command;

				::wsprintf(szName,TEXT("Accel%d_Command"),i);
				if (Settings.Read(szName,szCommand,lengthof(szCommand))
						&& szCommand[0]!='\0'
						&& (Command=m_pCommandList->ParseText(szCommand))!=0) {
					unsigned int Key,Modifiers;

					::wsprintf(szName,TEXT("Accel%d_Key"),i);
					if (Settings.Read(szName,&Key) && Key!=0) {
						KeyInfo Info;

						Info.Command=Command;
						Info.KeyCode=Key;
						Modifiers=0;
						::wsprintf(szName,TEXT("Accel%d_Mod"),i);
						Settings.Read(szName,&Modifiers);
						Info.Modifiers=Modifiers&0x7F;
						Info.fGlobal=(Modifiers&0x80)!=0;
						m_KeyList.push_back(Info);
					}
				}
			}
		}
	}

	if (Settings.SetSection(TEXT("AppCommand"))) {
		int NumCommands;

		if (Settings.Read(TEXT("NumCommands"),&NumCommands) && NumCommands>=0) {
			if (NumCommands>lengthof(AppCommandList))
				NumCommands=lengthof(AppCommandList);
			m_AppCommandList.clear();
			for (int i=0;i<NumCommands;i++) {
				TCHAR szName[64],szCommand[CCommandList::MAX_COMMAND_TEXT];

				::wsprintf(szName,TEXT("Button%d_Command"),i);
				if (Settings.Read(szName,szCommand,lengthof(szCommand))
						&& szCommand[0]!='\0') {
					int Type;
					unsigned int AppCommand;

					::wsprintf(szName,TEXT("Button%d_Type"),i);
					if (!Settings.Read(szName,&Type) || (Type!=0 && Type!=1))
						continue;
					::wsprintf(szName,TEXT("Button%d_AppCommand"),i);
					if (Settings.Read(szName,&AppCommand) && AppCommand!=0) {
						AppCommandInfo Info;

						Info.Command=m_pCommandList->ParseText(szCommand);
						if (Info.Command!=0) {
							Info.Type=(MediaKeyType)Type;
							Info.AppCommand=AppCommand;
							m_AppCommandList.push_back(Info);
						}
					}
				}
			}
		}
	}

	return true;
}


bool CAccelerator::SaveSettings(CSettings &Settings)
{
	if (Settings.SetSection(TEXT("Settings"))) {
		for (int i=0;i<=TVTest::CChannelInputOptions::KEY_LAST;i++) {
			TCHAR szKey[32];
			StdUtil::snprintf(szKey,lengthof(szKey),TEXT("ChInputKeyMode%d"),i);
			Settings.Write(szKey,(int)m_ChannelInputOptions.KeyInputMode[i]);
		}
		Settings.Write(TEXT("ChInputKeyTimeout"),m_ChannelInputOptions.KeyTimeout);
		Settings.Write(TEXT("ChInputKeyTimeoutCancel"),m_ChannelInputOptions.fKeyTimeoutCancel);
	}

	if (m_pCommandList==NULL)
		return true;

	if (Settings.SetSection(TEXT("Accelerator"))) {
		Settings.Clear();
#if 1
		/* �f�t�H���g�Ɠ����ꍇ�͕ۑ����Ȃ� */
		bool fDefault=false;
		if (m_KeyList.size()==lengthof(m_DefaultAccelList)) {
			int i,j;

			for (i=0;i<lengthof(m_DefaultAccelList);i++) {
				for (j=0;j<lengthof(m_DefaultAccelList);j++) {
					if (m_DefaultAccelList[i]==m_KeyList[j])
						break;
				}
				if (j==lengthof(m_DefaultAccelList))
					break;
			}
			if (i==lengthof(m_DefaultAccelList))
				fDefault=true;
		}
		if (fDefault) {
			Settings.Write(TEXT("AccelCount"),-1);
		} else
#endif
		{
			Settings.Write(TEXT("AccelCount"),(int)m_KeyList.size());
			for (size_t i=0;i<m_KeyList.size();i++) {
				TCHAR szName[64];

				::wsprintf(szName,TEXT("Accel%d_Command"),i);
				Settings.Write(szName,
					m_pCommandList->GetCommandText(m_pCommandList->IDToIndex(m_KeyList[i].Command)));
				::wsprintf(szName,TEXT("Accel%d_Key"),i);
				Settings.Write(szName,(int)m_KeyList[i].KeyCode);
				::wsprintf(szName,TEXT("Accel%d_Mod"),i);
				Settings.Write(szName,(int)(m_KeyList[i].Modifiers | (m_KeyList[i].fGlobal?0x80:0x00)));
			}
		}
	}

	if (Settings.SetSection(TEXT("AppCommand"))) {
		Settings.Clear();
#if 1
		/* �f�t�H���g�Ɠ����ꍇ�͕ۑ����Ȃ� */
		bool fDefault=false;
		if (m_AppCommandList.size()==lengthof(m_DefaultAppCommandList)) {
			int i,j;

			for (i=0;i<lengthof(m_DefaultAppCommandList);i++) {
				for (j=0;j<lengthof(m_DefaultAppCommandList);j++) {
					if (m_DefaultAppCommandList[i]==m_AppCommandList[j])
						break;
				}
				if (j==lengthof(m_DefaultAppCommandList))
					break;
			}
			if (i==lengthof(m_DefaultAppCommandList))
				fDefault=true;
		}
		if (fDefault) {
			Settings.Write(TEXT("NumCommands"),-1);
		} else
#endif
		{
			Settings.Write(TEXT("NumCommands"),(int)m_AppCommandList.size());
			for (size_t i=0;i<m_AppCommandList.size();i++) {
				TCHAR szName[64];

				::wsprintf(szName,TEXT("Button%d_Command"),i);
				Settings.Write(szName,
					m_pCommandList->GetCommandText(m_pCommandList->IDToIndex(m_AppCommandList[i].Command)));
				::wsprintf(szName,TEXT("Button%d_Type"),i);
				Settings.Write(szName,(int)m_AppCommandList[i].Type);
				::wsprintf(szName,TEXT("Button%d_AppCommand"),i);
				Settings.Write(szName,m_AppCommandList[i].AppCommand);
			}
		}
	}

	return true;
}


bool CAccelerator::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_ACCELERATOR));
}


bool CAccelerator::Initialize(HWND hwndHotKey,CMainMenu *pMainMenu,
							  CSettings &Settings,const CCommandList *pCommandList)
{
	m_pCommandList=pCommandList;
	LoadSettings(Settings);
	if (m_hAccel==NULL)
		m_hAccel=CreateAccel();
	m_pMainMenu=pMainMenu;
	m_pMainMenu->SetAccelerator(this);
	m_hwndHotKey=hwndHotKey;
	RegisterHotKey();
	m_RawInput.Initialize(hwndHotKey);
	m_RawInput.SetEventHandler(this);
	return true;
}


void CAccelerator::Finalize()
{
	if (m_hAccel) {
		::DestroyAcceleratorTable(m_hAccel);
		m_hAccel=NULL;
	}
	if (m_fRegisterHotKey)
		UnregisterHotKey();
}


bool CAccelerator::TranslateMessage(HWND hwnd,LPMSG pmsg)
{
	if (m_hAccel!=NULL)
		return ::TranslateAccelerator(hwnd,m_hAccel,pmsg)!=FALSE;
	return false;
}


int CAccelerator::TranslateHotKey(WPARAM wParam,LPARAM lParam) const
{
	for (size_t i=0;i<m_KeyList.size();i++) {
		if (((m_KeyList[i].Modifiers<<8)|m_KeyList[i].KeyCode)==wParam) {
			return m_KeyList[i].Command;
		}
	}
	return -1;
}


int CAccelerator::TranslateAppCommand(WPARAM wParam,LPARAM lParam) const
{
	const WORD Command=GET_APPCOMMAND_LPARAM(lParam);

	for (size_t i=0;i<m_AppCommandList.size();i++) {
		if (m_AppCommandList[i].Type==MEDIAKEY_APPCOMMAND
				&& m_AppCommandList[i].AppCommand==Command)
			return m_AppCommandList[i].Command;
	}
	return 0;
}


void CAccelerator::SetMenuAccel(HMENU hmenu)
{
	int Count,i;
	unsigned int State;

	Count=::GetMenuItemCount(hmenu);
	for (i=0;i<Count;i++) {
		State=::GetMenuState(hmenu,i,MF_BYPOSITION);
		if ((State&MF_POPUP)!=0) {
			SetMenuAccel(::GetSubMenu(hmenu,i));
		} else if ((State&MF_SEPARATOR)==0) {
			SetMenuAccelText(hmenu,(int)::GetMenuItemID(hmenu,i));
		}
	}
}


int CAccelerator::CheckAccelKey(BYTE Mod,WORD Key)
{
	const int Count=m_ListView.GetItemCount();

	for (int i=0;i<Count;i++) {
		LPARAM Param=m_ListView.GetItemParam(i);
		if (GET_ACCEL_KEY(Param)==Key && GET_ACCEL_MOD(Param)==Mod)
			return i;
	}

	return -1;
}


int CAccelerator::CheckAppCommand(int AppCommand)
{
	const int Count=m_ListView.GetItemCount();

	for (int i=0;i<Count;i++) {
		LPARAM Param=m_ListView.GetItemParam(i);
		if (GET_ACCEL_APPCOMMAND(Param)==AppCommand)
			return i;
	}

	return -1;
}


void CAccelerator::SetAccelItem(int Index,BYTE Mod,WORD Key,bool fGlobal,BYTE AppCommand)
{
	TCHAR szText[64];

	m_ListView.SetItemParam(Index,MAKE_ACCEL_PARAM(Key,Mod,fGlobal,AppCommand));
	if (Key!=0) {
		FormatAccelText(szText,Key,Mod,fGlobal);
	} else {
		szText[0]=_T('\0');
	}
	m_ListView.SetItemText(Index,COLUMN_KEY,szText);
	if (AppCommand!=0) {
		::lstrcpy(szText,m_MediaKeyList[AppCommand-1].pszText);
	} else {
		szText[0]=_T('\0');
	}
	m_ListView.SetItemText(Index,COLUMN_APPCOMMAND,szText);
}


void CAccelerator::SetDlgItemStatus(HWND hDlg)
{
	int Sel=m_ListView.GetSelectedItem();

	if (Sel>=0) {
		LPARAM Param=m_ListView.GetItemParam(Sel);
		WORD Key=GET_ACCEL_KEY(Param);
		BYTE Mod=GET_ACCEL_MOD(Param);
		for (int i=0;i<lengthof(AccelKeyList);i++) {
			if (AccelKeyList[i].KeyCode==(unsigned)Key) {
				::SendDlgItemMessage(hDlg,IDC_ACCELERATOR_KEY,CB_SETCURSEL,i+1,0);
				break;
			}
		}
		DlgCheckBox_Check(hDlg,IDC_ACCELERATOR_SHIFT,(Mod&MOD_SHIFT)!=0);
		DlgCheckBox_Check(hDlg,IDC_ACCELERATOR_CONTROL,(Mod&MOD_CONTROL)!=0);
		DlgCheckBox_Check(hDlg,IDC_ACCELERATOR_ALT,(Mod&MOD_ALT)!=0);
		DlgCheckBox_Check(hDlg,IDC_ACCELERATOR_GLOBAL,GET_ACCEL_GLOBAL(Param));
	} else {
		::SendDlgItemMessage(hDlg,IDC_ACCELERATOR_KEY,CB_SETCURSEL,0,0);
		for (int i=IDC_ACCELERATOR_SHIFT;i<=IDC_ACCELERATOR_ALT;i++)
			DlgCheckBox_Check(hDlg,i,false);
		DlgCheckBox_Check(hDlg,IDC_ACCELERATOR_GLOBAL,false);
		ShowDlgItem(hDlg,IDC_ACCELERATOR_APPCOMMAND,false);
	}
	EnableDlgItems(hDlg,IDC_ACCELERATOR_KEY,IDC_ACCELERATOR_GLOBAL,Sel>=0);
}


INT_PTR CAccelerator::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			m_ListView.Attach(::GetDlgItem(hDlg,IDC_ACCELERATOR_LIST));
			m_ListView.SetExtendedStyle(
				LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP);

			m_ListView.InsertColumn(COLUMN_COMMAND,TEXT("�@�\"));
			m_ListView.InsertColumn(COLUMN_KEY,TEXT("�L�["));
			m_ListView.InsertColumn(COLUMN_APPCOMMAND,TEXT("�}���`���f�B�A�L�["));

			for (int i=0;i<m_pCommandList->NumCommands();i++) {
				int Command=m_pCommandList->GetCommandID(i);
				const KeyInfo *pKey=NULL;
				int AppCommand=0;
				TCHAR szText[CCommandList::MAX_COMMAND_NAME];

				for (size_t j=0;j<m_KeyList.size();j++) {
					if (m_KeyList[j].Command==Command) {
						pKey=&m_KeyList[j];
						break;
					}
				}
				for (size_t j=0;j<m_AppCommandList.size();j++) {
					if (m_AppCommandList[j].Command==Command) {
						for (size_t k=0;k<m_MediaKeyList.size();k++) {
							if (m_MediaKeyList[k].Type==m_AppCommandList[j].Type
									&& m_MediaKeyList[k].Command==m_AppCommandList[j].AppCommand) {
								AppCommand=(int)(k+1);
								break;
							}
						}
						break;
					}
				}
				m_pCommandList->GetCommandName(i,szText,lengthof(szText));
				LPARAM Param;
				if (pKey!=NULL)
					Param=MAKE_ACCEL_PARAM(pKey->KeyCode,pKey->Modifiers,pKey->fGlobal,AppCommand);
				else
					Param=MAKE_ACCEL_PARAM(0,0,false,AppCommand);
				int Index=m_ListView.InsertItem(i,szText,Param);
				if (pKey!=NULL) {
					FormatAccelText(szText,pKey->KeyCode,pKey->Modifiers,pKey->fGlobal);
					m_ListView.SetItemText(Index,COLUMN_KEY,szText);
				}
				if (AppCommand!=0) {
					::lstrcpy(szText,m_MediaKeyList[AppCommand-1].pszText);
					m_ListView.SetItemText(Index,COLUMN_APPCOMMAND,szText);
				}
			}

			m_ListView.AdjustColumnWidth(true);

			DlgComboBox_AddString(hDlg,IDC_ACCELERATOR_KEY,TEXT("�Ȃ�"));
			for (int i=0;i<lengthof(AccelKeyList);i++)
				DlgComboBox_AddString(hDlg,IDC_ACCELERATOR_KEY,AccelKeyList[i].pszText);
			DlgComboBox_AddString(hDlg,IDC_ACCELERATOR_APPCOMMAND,TEXT("�Ȃ�"));
			for (size_t i=0;i<m_MediaKeyList.size();i++)
				DlgComboBox_AddString(hDlg,IDC_ACCELERATOR_APPCOMMAND,m_MediaKeyList[i].pszText);

			SetDlgItemStatus(hDlg);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ACCELERATOR_KEY:
			if (HIWORD(wParam)!=CBN_SELCHANGE)
				break;
		case IDC_ACCELERATOR_SHIFT:
		case IDC_ACCELERATOR_CONTROL:
		case IDC_ACCELERATOR_ALT:
		case IDC_ACCELERATOR_GLOBAL:
			{
				int Sel=m_ListView.GetSelectedItem();
				if (Sel<0)
					return TRUE;

				LPARAM Param=m_ListView.GetItemParam(Sel);
				int Key=(int)SendDlgItemMessage(hDlg,IDC_ACCELERATOR_KEY,CB_GETCURSEL,0,0);
				if (Key>0) {
					BYTE Mod;
					int i;

					Mod=0;
					if (DlgCheckBox_IsChecked(hDlg,IDC_ACCELERATOR_SHIFT))
						Mod|=MOD_SHIFT;
					if (DlgCheckBox_IsChecked(hDlg,IDC_ACCELERATOR_CONTROL))
						Mod|=MOD_CONTROL;
					if (DlgCheckBox_IsChecked(hDlg,IDC_ACCELERATOR_ALT))
						Mod|=MOD_ALT;
					i=CheckAccelKey(Mod,AccelKeyList[Key-1].KeyCode);
					if (i>=0 && i!=Sel) {
						TCHAR szCommand[CCommandList::MAX_COMMAND_NAME],szText[CCommandList::MAX_COMMAND_NAME+128];

						m_pCommandList->GetCommandName(i,szCommand,lengthof(szCommand));
						StdUtil::snprintf(szText,lengthof(szText),
										  TEXT("���� [%s] �Ɋ��蓖�Ă��Ă��܂��B\n���蓖�Ē����܂���?"),
										  szCommand);
						if (::MessageBox(hDlg,szText,TEXT("�m�F"),
										 MB_YESNO | MB_ICONQUESTION)!=IDYES)
							return TRUE;
						SetAccelItem(i,0,0,false,
									 GET_ACCEL_APPCOMMAND(m_ListView.GetItemParam(i)));
					}
					Param=MAKE_ACCEL_PARAM(AccelKeyList[Key-1].KeyCode,Mod,
						DlgCheckBox_IsChecked(hDlg,IDC_ACCELERATOR_GLOBAL),
						GET_ACCEL_APPCOMMAND(Param));
				} else {
					Param=MAKE_ACCEL_PARAM(0,0,false,GET_ACCEL_APPCOMMAND(Param));
				}
				m_ListView.SetItemParam(Sel,Param);
				TCHAR szText[64];
				if (Key>0) {
					FormatAccelText(szText,
						GET_ACCEL_KEY(Param),GET_ACCEL_MOD(Param),GET_ACCEL_GLOBAL(Param));
				} else {
					szText[0]=_T('\0');
				}
				m_ListView.SetItemText(Sel,COLUMN_KEY,szText);
			}
			return TRUE;

		case IDC_ACCELERATOR_APPCOMMAND:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				int Sel=m_ListView.GetSelectedItem();
				if (Sel<0)
					return TRUE;

				int AppCommand=(int)DlgComboBox_GetCurSel(hDlg,IDC_ACCELERATOR_APPCOMMAND);
				if (AppCommand>0) {
					int i;

					i=CheckAppCommand(AppCommand);
					if (i>=0 && i!=Sel) {
						TCHAR szCommand[CCommandList::MAX_COMMAND_NAME],szText[CCommandList::MAX_COMMAND_NAME+128];

						m_pCommandList->GetCommandName(i,szCommand,lengthof(szCommand));
						StdUtil::snprintf(szText,lengthof(szText),
										  TEXT("���� [%s] �Ɋ��蓖�Ă��Ă��܂��B\n���蓖�Ē����܂���?"),
										  szCommand);
						if (::MessageBox(hDlg,szText,TEXT("�m�F"),
										 MB_YESNO | MB_ICONQUESTION)!=IDYES)
							return TRUE;
						LPARAM Param=m_ListView.GetItemParam(i);
						SetAccelItem(i,GET_ACCEL_MOD(Param),
									 GET_ACCEL_KEY(Param),GET_ACCEL_GLOBAL(Param),0);
					}
				}
				LPARAM Param=m_ListView.GetItemParam(Sel);
				Param=MAKE_ACCEL_PARAM(GET_ACCEL_KEY(Param),
					GET_ACCEL_MOD(Param),GET_ACCEL_GLOBAL(Param),AppCommand);
				m_ListView.SetItemParam(Sel,Param);
				TCHAR szText[64];
				if (AppCommand>0) {
					::lstrcpy(szText,m_MediaKeyList[AppCommand-1].pszText);
				} else {
					szText[0]=_T('\0');
				}
				m_ListView.SetItemText(Sel,COLUMN_APPCOMMAND,szText);
			}
			return TRUE;

		case IDC_ACCELERATOR_DEFAULT:
			{
				int NumCommands,i,j;

				NumCommands=m_pCommandList->NumCommands();
				for (i=0;i<NumCommands;i++) {
					int Command=m_pCommandList->GetCommandID(i);
					WORD Key=0;
					BYTE Mod=0;
					int AppCommand=0;

					for (j=0;j<lengthof(m_DefaultAccelList);j++) {
						if (m_DefaultAccelList[j].Command==Command) {
							Key=m_DefaultAccelList[j].KeyCode;
							Mod=m_DefaultAccelList[j].Modifiers;
							break;
						}
					}
					for (j=0;j<lengthof(m_DefaultAppCommandList);j++) {
						if (m_DefaultAppCommandList[j].Command==Command) {
							for (size_t k=0;k<m_MediaKeyList.size();k++) {
								if (m_MediaKeyList[k].Type==MEDIAKEY_APPCOMMAND
										&& m_MediaKeyList[k].Command==m_DefaultAppCommandList[j].AppCommand) {
									AppCommand=(int)(k+1);
									break;
								}
							}
							break;
						}
					}
					LPARAM Param=m_ListView.GetItemParam(i);
					if (GET_ACCEL_KEY(Param)!=Key
							|| GET_ACCEL_MOD(Param)!=Mod
							|| GET_ACCEL_APPCOMMAND(Param)!=AppCommand)
						SetAccelItem(i,Mod,Key,false,AppCommand);
				}
				SetDlgItemStatus(hDlg);
			}
			return TRUE;

		case IDC_ACCELERATOR_CHANNELINPUTOPTIONS:
			{
				TVTest::CChannelInputOptionsDialog Dialog(m_ChannelInputOptions);

				if (Dialog.Show(hDlg))
					m_fChanged=true;
			}
			return TRUE;
		}
		return TRUE;

	case WM_APPCOMMAND:
		{
			int Sel=m_ListView.GetSelectedItem();

			if (Sel>=0) {
				const WORD Command=GET_APPCOMMAND_LPARAM(lParam);
				int i;

				for (i=0;i<lengthof(AppCommandList);i++) {
					if (AppCommandList[i].Command==Command)
						break;
				}
				if (i<lengthof(AppCommandList)) {
					i++;
					if (GET_ACCEL_APPCOMMAND(m_ListView.GetItemParam(Sel))!=i) {
						DlgComboBox_SetCurSel(hDlg,IDC_ACCELERATOR_APPCOMMAND,i);
						::SendMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_ACCELERATOR_APPCOMMAND,CBN_SELCHANGE),0);
					}
				}
			}
		}
		break;

	case WM_APP:
		{
			int Sel=m_ListView.GetSelectedItem();

			if (Sel>=0) {
				int Index=m_RawInput.KeyDataToIndex((int)wParam);

				if (Index>=0) {
					Index+=1+lengthof(AppCommandList);
					if (GET_ACCEL_APPCOMMAND(m_ListView.GetItemParam(Sel))!=Index) {
						DlgComboBox_SetCurSel(hDlg,IDC_ACCELERATOR_APPCOMMAND,Index);
						::SendMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_ACCELERATOR_APPCOMMAND,CBN_SELCHANGE),0);
					}
				}
			}
		}
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case LVN_ITEMCHANGED:
			SetDlgItemStatus(hDlg);
			break;

		case NM_CLICK:
			{
				LPNMITEMACTIVATE pnmia=reinterpret_cast<LPNMITEMACTIVATE>(lParam);
				HWND hwndList=pnmia->hdr.hwndFrom;
				HWND hwndAppCommand=::GetDlgItem(hDlg,IDC_ACCELERATOR_APPCOMMAND);
				LVHITTESTINFO lvhi;
				DWORD Pos=::GetMessagePos();

				lvhi.pt.x=GET_X_LPARAM(Pos);
				lvhi.pt.y=GET_Y_LPARAM(Pos);
				::ScreenToClient(hwndList,&lvhi.pt);
				if (ListView_SubItemHitTest(hwndList,&lvhi)>=0
						&& (lvhi.flags&LVHT_ONITEMLABEL)!=0 && lvhi.iSubItem==2) {
					RECT rc;

					ListView_GetSubItemRect(hwndList,lvhi.iItem,lvhi.iSubItem,LVIR_BOUNDS,&rc);
					::MapWindowPoints(hwndList,hDlg,(LPPOINT)&rc,2);
					::MoveWindow(hwndAppCommand,rc.left,rc.top,rc.right-rc.left,240,TRUE);
					ComboBox_SetCurSel(hwndAppCommand,GET_ACCEL_APPCOMMAND(m_ListView.GetItemParam(lvhi.iItem)));
					::ShowWindow(hwndAppCommand,SW_SHOW);
					::BringWindowToTop(hwndAppCommand);
				} else {
					::ShowWindow(hwndAppCommand,SW_HIDE);
				}
			}
			break;

		case LVN_BEGINSCROLL:
			ShowDlgItem(hDlg,IDC_ACCELERATOR_APPCOMMAND,false);
			break;

		case LVN_KEYDOWN:
			{
				LPNMLVKEYDOWN pnmlvk=reinterpret_cast<LPNMLVKEYDOWN>(lParam);

				if (pnmlvk->wVKey==VK_BACK || pnmlvk->wVKey==VK_DELETE) {
					int Sel=m_ListView.GetSelectedItem();

					if (Sel>=0)
						SetAccelItem(Sel,0,0,false,0);
				}
			}
			break;

		case PSN_APPLY:
			{
				UnregisterHotKey();
				m_KeyList.clear();
				m_AppCommandList.clear();
				const int Count=m_pCommandList->NumCommands();
				for (int i=0;i<Count;i++) {
					LPARAM Param=m_ListView.GetItemParam(i);
					if (GET_ACCEL_KEY(Param)!=0) {
						KeyInfo Info;

						Info.Command=m_pCommandList->GetCommandID(i);
						Info.KeyCode=GET_ACCEL_KEY(Param);
						Info.Modifiers=GET_ACCEL_MOD(Param);
						Info.fGlobal=GET_ACCEL_GLOBAL(Param);
						m_KeyList.push_back(Info);
					}
					int AppCommand=GET_ACCEL_APPCOMMAND(Param);
					if (AppCommand!=0) {
						AppCommandInfo Info;

						Info.Command=m_pCommandList->GetCommandID(i);
						Info.Type=m_MediaKeyList[AppCommand-1].Type;
						Info.AppCommand=m_MediaKeyList[AppCommand-1].Command;
						m_AppCommandList.push_back(Info);
					}
				}
				HACCEL hAccel=CreateAccel();
				if (m_hAccel!=NULL)
					::DestroyAcceleratorTable(m_hAccel);
				m_hAccel=hAccel;
				m_pMainMenu->SetAccelerator(this);
				RegisterHotKey();

				m_fChanged=true;
			}
			break;
		}
		break;

	case WM_DESTROY:
		m_ListView.Detach();
		return TRUE;
	}

	return FALSE;
}


// CRawInput::CEventHandler
void CAccelerator::OnInput(int Type)
{
	int Data=m_RawInput.GetKeyData(Type);

	if (m_hDlg==NULL || !::IsWindowVisible(m_hDlg)) {
		for (size_t i=0;i<m_AppCommandList.size();i++) {
			if (m_AppCommandList[i].Type==MEDIAKEY_RAWINPUT
					&& m_AppCommandList[i].AppCommand==Data) {
				::PostMessage(m_hwndHotKey,WM_COMMAND,m_AppCommandList[i].Command,0);
				return;
			}
		}
	} else if (::IsWindowEnabled(m_hDlg)) {
		::PostMessage(m_hDlg,WM_APP,Data,0);
	}
}


void CAccelerator::OnUnknownInput(const BYTE *pData,int Size)
{
	/*
	if (m_hDlg!=NULL && ::IsWindowVisible(m_hDlg) && ::IsWindowEnabled(m_hDlg)) {
		static const LPCTSTR pszHex=TEXT("0123456789ABCDEF");
		TCHAR szText[256];

		::lstrcpy(szText,TEXT("�f�[�^ : "));
		i=::lstrlen(szText);
		for (int j=0;j<min(Size,16);j++) {
			szText[i++]=pszHex[pData[j]>>4];
			szText[i++]=pszHex[pData[j]&0x0F];
		}
		if (Size>16) {
			::lstrcpy(szText+i,TEXT(" ..."));
			i+=4;
		}
		::lstrcpy(szText+i,TEXT("\n(�������L�[�ƃf�[�^�������Ă��炦��ΑΉ��ł��邩���m��܂���B)"));
		CMessageDialog MessageDialog;
		MessageDialog.Show(m_hDlg,CMessageDialog::TYPE_INFO,szText,
						   TEXT("�Ή����Ă��Ȃ��L�[��������܂����B"),NULL,TEXT("���߂�"));
	}
	*/
}
