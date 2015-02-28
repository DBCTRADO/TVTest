/*
	TVTest �v���O�C���T���v��

	HDUS�̃����R�����g��
*/


#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"
#include "HDUSRemocon_KeyHook.h"
#include "resource.h"

#pragma comment(lib,"shlwapi.lib")


// �R���g���[����
#define HDUS_REMOCON_NAME L"HDUS Remocon"

// �{�^���̃��X�g
static const struct {
	TVTest::ControllerButtonInfo Info;
	BYTE KeyCode;
	BYTE Modifiers;
} g_ButtonList[] = {
	{{L"��ʕ\��",		L"AspectRatio",			{ 8, 14,16,11}, {  0, 0}},	VK_F13,		MK_SHIFT},
	{{L"�d��",			L"Close",				{32, 11,16,16}, {  0,22}},	VK_F14,		MK_SHIFT},
	{{L"����",			L"Mute",				{56, 14,16,11}, { 16, 0}},	VK_F15,		MK_SHIFT},
	{{L"�`�����l��1",	L"Channel1",			{ 8, 28,16,11}, { 32, 0}},	VK_F17,		MK_SHIFT},
	{{L"�`�����l��2",	L"Channel2",			{24, 28,16,11}, { 48, 0}},	VK_F18,		MK_SHIFT},
	{{L"�`�����l��3",	L"Channel3",			{40, 28,16,11}, { 64, 0}},	VK_F19,		MK_SHIFT},
	{{L"�`�����l��4",	L"Channel4",			{ 8, 43,16,11}, { 80, 0}},	VK_F20,		MK_SHIFT},
	{{L"�`�����l��5",	L"Channel5",			{24, 43,16,11}, { 96, 0}},	VK_F21,		MK_SHIFT},
	{{L"�`�����l��6",	L"Channel6",			{40, 43,16,11}, {112, 0}},	VK_F22,		MK_SHIFT},
	{{L"�`�����l��7",	L"Channel7",			{ 8, 57,16,11}, {128, 0}},	VK_F23,		MK_SHIFT},
	{{L"�`�����l��8",	L"Channel8",			{24, 57,16,11}, {144, 0}},	VK_F24,		MK_SHIFT},
	{{L"�`�����l��9",	L"Channel9",			{40, 57,16,11}, {160, 0}},	VK_F13,		MK_CONTROL},
	{{L"�`�����l��10",	L"Channel10",			{ 8, 71,16,11}, {176, 0}},	VK_F16,		MK_SHIFT},
	{{L"�`�����l��11",	L"Channel11",			{24, 71,16,11}, {192, 0}},	VK_F14,		MK_CONTROL},
	{{L"�`�����l��12",	L"Channel12",			{40, 71,16,11}, {208, 0}},	VK_F15,		MK_CONTROL},
	{{L"���j���[",		L"Menu",				{11, 85,28,11}, { 32,11}},	VK_F16,		MK_CONTROL},
	{{L"�S��ʕ\��",	L"Fullscreen",			{41, 85,28,11}, { 60,11}},	VK_F17,		MK_CONTROL},
	{{L"����",			NULL,					{ 9,102,24,25}, {122,22}},	VK_F18,		MK_CONTROL},
	{{L"�����ؑ�",		L"SwitchAudio",			{47,102,24,25}, {146,22}},	VK_F19,		MK_CONTROL},
	{{L"EPG",			L"ProgramGuide",		{ 9,140,24,25}, {170,22}},	VK_F20,		MK_CONTROL},
	{{L"�߂�",			NULL,					{47,140,24,25}, {194,22}},	VK_F21,		MK_CONTROL},
	{{L"�^��",			L"RecordStart",			{20,170,16,11}, { 88,11}},	VK_F22,		MK_CONTROL},
	{{L"����",			L"SaveImage",			{44,170,16,11}, {104,11}},	VK_F23,		MK_CONTROL},
	{{L"��~",			L"RecordStop",			{ 8,186,16,11}, {120,11}},	VK_F24,		MK_CONTROL},
	{{L"�Đ�",			NULL,					{26,184,28,14}, { 16,22}},	VK_F13,		MK_CONTROL | MK_SHIFT},
	{{L"�ꎞ��~",		L"RecordPause",			{56,187,16,11}, {136,11}},	VK_F14,		MK_CONTROL | MK_SHIFT},
	{{L"|<<",			NULL,					{ 8,201,16,11}, {152,11}},	VK_F15,		MK_CONTROL | MK_SHIFT},
	{{L"<<",			NULL,					{24,201,16,11}, {168,11}},	VK_F16,		MK_CONTROL | MK_SHIFT},
	{{L">>",			NULL,					{40,201,16,11}, {184,11}},	VK_F17,		MK_CONTROL | MK_SHIFT},
	{{L">>|",			NULL,					{56,201,16,11}, {200,11}},	VK_F18,		MK_CONTROL | MK_SHIFT},
	{{L"������",		L"ChannelDisplayMenu",	{24,215,16,11}, {216,11}},	VK_F19,		MK_CONTROL | MK_SHIFT},
	{{L"�W�����v",		L"ChannelMenu",			{40,215,16,11}, {232,11}},	VK_F20,		MK_CONTROL | MK_SHIFT},
	{{L"A (��)",		NULL,					{ 8,230,16,11}, { 44,22}},	VK_F21,		MK_CONTROL | MK_SHIFT},
	{{L"B (��)",		NULL,					{24,230,16,11}, { 60,22}},	VK_F22,		MK_CONTROL | MK_SHIFT},
	{{L"C (��)",		NULL,					{40,230,16,11}, { 76,22}},	VK_F23,		MK_CONTROL | MK_SHIFT},
	{{L"D (��)",		NULL,					{56,230,16,11}, { 92,22}},	VK_F24,		MK_CONTROL | MK_SHIFT},
#if 0
	{{L"���� +",		L"VolumeUp",			{56, 28,16,11}, {  0,11}},	VK_UP,		MK_SHIFT},
	{{L"���� -",		L"VolumeDown",			{56, 43,16,11}, { 16,11}},	VK_DOWN,	MK_SHIFT},
	{{L"�`�����l�� +",	L"ChannelUp",			{57, 57,14,13}, {108,22}},	VK_UP,		MK_CONTROL},
	{{L"�`�����l�� -",	L"ChannelDown",			{57, 70,14,13}, {108,35}},	VK_DOWN,	MK_CONTROL},
#endif
};

#define NUM_BUTTONS (sizeof(g_ButtonList)/sizeof(g_ButtonList[0]))


// �v���O�C���N���X
class CHDUSRemocon : public TVTest::CTVTestPlugin
{
	bool m_fInitialized;	// �������ς݂�?
	HMODULE m_hLib;			// �t�b�NDLL�̃n���h��
	bool m_fHook;			// �t�b�N���Ă��邩?
	static UINT m_Message;	// �t�b�NDLL���瑗���郁�b�Z�[�W

	bool InitializePlugin();
	bool BeginHook();
	bool EndHook();
	bool SetWindow(HWND hwnd);
	void OnError(LPCWSTR pszMessage);

	static LRESULT CALLBACK EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData);
	static BOOL CALLBACK MessageCallback(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,
										 LRESULT *pResult,void *pClientData);
	static BOOL CALLBACK TranslateMessageCallback(HWND hwnd,MSG *pMessage,void *pClientData);

public:
	CHDUSRemocon();
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();
};


UINT CHDUSRemocon::m_Message=0;


CHDUSRemocon::CHDUSRemocon()
	: m_fInitialized(false)
	, m_hLib(NULL)
	, m_fHook(false)
{
}


bool CHDUSRemocon::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	// �v���O�C���̏���Ԃ�
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"HDUS�����R��";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"HDUS�̃����R���ɑΉ����܂��B";
	return true;
}


bool CHDUSRemocon::Initialize()
{
	// ����������

	// �{�^���̃��X�g���쐬
	TVTest::ControllerButtonInfo ButtonList[NUM_BUTTONS];
	for (int i=0;i<NUM_BUTTONS;i++)
		ButtonList[i]=g_ButtonList[i].Info;

	// �R���g���[���̓o�^
	TVTest::ControllerInfo Info;
	Info.Flags             = 0;
	Info.pszName           = HDUS_REMOCON_NAME;
	Info.pszText           = L"HDUS�����R��";
	Info.NumButtons        = NUM_BUTTONS;
	Info.pButtonList       = ButtonList;
	Info.pszIniFileName    = NULL;
	Info.pszSectionName    = L"HDUSController";
	Info.ControllerImageID = IDB_CONTROLLER;
	Info.SelButtonsImageID = IDB_SELBUTTONS;
	Info.pTranslateMessage = TranslateMessageCallback;
	Info.pClientData       = this;
	if (!m_pApp->RegisterController(&Info)) {
		m_pApp->AddLog(L"�R���g���[����o�^�ł��܂���B",TVTest::LOG_TYPE_ERROR);
		return false;
	}

	// �C�x���g�R�[���o�b�N�֐���o�^
	m_pApp->SetEventCallback(EventCallback,this);

	return true;
}


// �v���O�C�����L���ɂ��ꂽ���̏���������
bool CHDUSRemocon::InitializePlugin()
{
	if (!m_fInitialized) {
		// ���b�Z�[�W�̓o�^
		m_Message=::RegisterWindowMessage(KEYHOOK_MESSAGE);
		if (m_Message==0)
			return false;

		// Vista/7 �ŊǗ��Ҍ����Ŏ��s���ꂽ���p�̑΍�
#ifndef MSGFLT_ADD
#define MSGFLT_ADD 1
#endif
		typedef BOOL (WINAPI *ChangeWindowMessageFilterFunc)(UINT,DWORD);
		HMODULE hLib=::LoadLibrary(TEXT("user32.dll"));
		ChangeWindowMessageFilterFunc pChangeFilter=
			(ChangeWindowMessageFilterFunc)::GetProcAddress(hLib,"ChangeWindowMessageFilter");
		if (pChangeFilter!=NULL)
			pChangeFilter(m_Message,MSGFLT_ADD);
		::FreeLibrary(hLib);

		m_fInitialized=true;
	}

	// ���b�Z�[�W�R�[���o�b�N�֐���o�^
	m_pApp->SetWindowMessageCallback(MessageCallback,this);

	return true;
}


bool CHDUSRemocon::Finalize()
{
	// �I������

	// �t�b�N�I���
	if (m_hLib!=NULL) {
		EndHook();
		::FreeLibrary(m_hLib);
		m_hLib=NULL;
	}

	return true;
}


bool CHDUSRemocon::BeginHook()
{
	// �t�b�N�J�n

	if (m_hLib==NULL) {
		TCHAR szFileName[MAX_PATH];

		::GetModuleFileName(g_hinstDLL,szFileName,MAX_PATH);
		::lstrcpy(::PathFindExtension(szFileName),TEXT("_KeyHook.dll"));
		m_hLib=::LoadLibrary(szFileName);
		if (m_hLib==NULL)
			return false;
	}

	BeginHookFunc pBeginHook;
	pBeginHook=(BeginHookFunc)::GetProcAddress(m_hLib,"BeginHook");
	if (pBeginHook==NULL
			|| !pBeginHook(m_pApp->GetAppWindow(),
						   m_pApp->IsControllerActiveOnly(HDUS_REMOCON_NAME))) {
		::FreeLibrary(m_hLib);
		m_hLib=NULL;
		return false;
	}
	m_fHook=true;

	return true;
}


bool CHDUSRemocon::EndHook()
{
	// �t�b�N�I���

	if (m_fHook) {
		EndHookFunc pEndHook;

		pEndHook=(EndHookFunc)::GetProcAddress(m_hLib,"EndHook");
		if (pEndHook==NULL)
			return false;
		pEndHook();
		m_fHook=false;
	}
	return true;
}


bool CHDUSRemocon::SetWindow(HWND hwnd)
{
	// �E�B���h�E��ݒ�

	if (!m_fHook)
		return false;
	SetWindowFunc pSetWindow=(SetWindowFunc)::GetProcAddress(m_hLib,"SetWindow");
	if (pSetWindow==NULL)
		return false;
	return pSetWindow(hwnd)!=FALSE;
}


void CHDUSRemocon::OnError(LPCWSTR pszMessage)
{
	// �G���[�������̃��b�Z�[�W�\��
	m_pApp->AddLog(pszMessage,TVTest::LOG_TYPE_ERROR);
	if (!m_pApp->GetSilentMode()) {
		::MessageBoxW(m_pApp->GetAppWindow(),pszMessage,L"HDUS�����R��",
					  MB_OK | MB_ICONEXCLAMATION);
	}
}


// �C�x���g�R�[���o�b�N�֐�
// �����C�x���g���N����ƌĂ΂��
LRESULT CALLBACK CHDUSRemocon::EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
{
	CHDUSRemocon *pThis=static_cast<CHDUSRemocon*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// �v���O�C���̗L����Ԃ��ω�����
		if (lParam1!=0) {
			if (!pThis->InitializePlugin()) {
				pThis->OnError(L"�������ŃG���[���������܂����B");
				return FALSE;
			}
			if (!pThis->BeginHook()) {
				pThis->OnError(L"�t�b�N�ł��܂���B");
				return FALSE;
			}
		} else {
			pThis->EndHook();
			pThis->m_pApp->SetWindowMessageCallback(NULL);
		}
		return TRUE;

	case TVTest::EVENT_CONTROLLERFOCUS:
		// ���̃v���Z�X������̑ΏۂɂȂ���
		pThis->SetWindow(reinterpret_cast<HWND>(lParam1));
		return TRUE;
	}

	return 0;
}


// ���b�Z�[�W�R�[���o�b�N�֐�
BOOL CALLBACK CHDUSRemocon::MessageCallback(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,
											LRESULT *pResult,void *pClientData)
{
	if (uMsg==m_Message) {
		// �t�b�NDLL���瑗���郁�b�Z�[�W

		if (KEYHOOK_GET_REPEATCOUNT(lParam)>1)
			return 0;

		BYTE Modifiers=0;
		if (KEYHOOK_GET_CONTROL(lParam))
			Modifiers|=MK_CONTROL;
		if (KEYHOOK_GET_SHIFT(lParam))
			Modifiers|=MK_SHIFT;
		WORD KeyCode=(WORD)wParam;

		// �����ꂽ�{�^����T��
		for (int i=0;i<NUM_BUTTONS;i++) {
			if (g_ButtonList[i].KeyCode==KeyCode
					&& g_ButtonList[i].Modifiers==Modifiers) {
				CHDUSRemocon *pThis=static_cast<CHDUSRemocon*>(pClientData);

				// �{�^���������ꂽ���Ƃ�ʒm����
				pThis->m_pApp->OnControllerButtonDown(HDUS_REMOCON_NAME,i);
				break;
			}
		}
		return TRUE;
	}

	return FALSE;
}


// ���b�Z�[�W�̕ϊ��R�[���o�b�N
// Shift+�� �Ȃǂ̓t�b�N����Ɩ�肠��̂ł����ŏ������Ă��܂�
BOOL CALLBACK CHDUSRemocon::TranslateMessageCallback(HWND hwnd,MSG *pMessage,void *pClientData)
{
	if (pMessage->message==WM_KEYDOWN
			&& (pMessage->wParam==VK_UP || pMessage->wParam==VK_DOWN)) {
		bool fShift=::GetKeyState(VK_SHIFT)<0;
		bool fCtrl=::GetKeyState(VK_CONTROL)<0;

		// �ǂ��炩�����������Ă���ꍇ�̂ݏ�������
		if (fShift!=fCtrl) {
			CHDUSRemocon *pThis=static_cast<CHDUSRemocon*>(pClientData);

			pThis->m_pApp->DoCommand(
				pMessage->wParam==VK_UP?(fShift?L"VolumeUp":L"ChannelUp"):
										(fShift?L"VolumeDown":L"ChannelDown"));
			return TRUE;
		}
	}

	return FALSE;
}




// �v���O�C���N���X�̃C���X�^���X�𐶐�����
TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CHDUSRemocon;
}
