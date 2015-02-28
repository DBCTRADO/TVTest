/*
	TVTest �v���O�C���T���v��

	�󂫃f�B�X�N�e�ʂ����Ȃ��Ȃ�����ʂ̃t�H���_�ɘ^�悷��
*/


#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <tchar.h>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"
#include "resource.h"

#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"shell32.lib")


// �\���̃t�H���_�̐�
#define NUM_SPARE_FOLDERS 3

// �E�B���h�E�N���X��
#define DISKRELAY_WINDOW_CLASS TEXT("TVTest DiskRelay Window")

// �󂫗e�ʂ��Ď�����^�C�}�[�̎��ʎq
#define WATCH_TIMER_ID 1
// �󂫗e�ʂ��Ď�����Ԋu(ms)
#define WATCH_INTERVAL 2000


// �v���O�C���N���X
class CDiskRelay : public TVTest::CTVTestPlugin
{
	bool m_fInitialized;				// �������ς݂�?
	TCHAR m_szIniFileName[MAX_PATH];	// INI�t�@�C���̃p�X
	TCHAR m_szSpareFolder[NUM_SPARE_FOLDERS][MAX_PATH];	// �\���̃t�H���_
	UINT m_LowFreeSpace;				// �󂫗e�ʂ����Ȃ��Ɣ��肷��臒l
	HWND m_hwnd;						// �E�B���h�E�n���h��
	bool m_fRecording;					// �^�撆��?
	int m_NextFolder;					// ���̘^���t�H���_

	bool InitializePlugin();
	bool CheckFreeSpace();
	bool SettingsDialog(HWND hwndOwner);
	void SaveSettings();
	static LRESULT CALLBACK EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData);
	static CDiskRelay *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static INT_PTR CALLBACK SettingsDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	CDiskRelay();
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();
};


CDiskRelay::CDiskRelay()
	: m_fInitialized(false)
	, m_LowFreeSpace(64)
	, m_hwnd(NULL)
	, m_fRecording(false)
{
	for (int i=0;i<NUM_SPARE_FOLDERS;i++)
		m_szSpareFolder[i][0]='\0';
}


bool CDiskRelay::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	// �v���O�C���̏���Ԃ�
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = TVTest::PLUGIN_FLAG_HASSETTINGS;
	pInfo->pszPluginName  = L"�\���̘^���";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"�󂫗e�ʂ����Ȃ��Ȃ������A�\���̘^���ɘ^�悵�܂��B";
	return true;
}


// ����������
bool CDiskRelay::Initialize()
{
	// �Ή��󋵂��`�F�b�N����
	TVTest::HostInfo HostInfo;
	if (!m_pApp->GetHostInfo(&HostInfo)
			|| HostInfo.SupportedPluginVersion<TVTEST_PLUGIN_VERSION_(0,0,10)
			|| !m_pApp->QueryMessage(TVTest::MESSAGE_RELAYRECORD)) {
		m_pApp->AddLog(L"���̃o�[�W�����ł͗��p�ł��܂���B",TVTest::LOG_TYPE_ERROR);
		return false;
	}

	// �C�x���g�R�[���o�b�N�֐���o�^
	m_pApp->SetEventCallback(EventCallback,this);

	return true;
}


// �v���O�C�����L���ɂ��ꂽ���̏���������
bool CDiskRelay::InitializePlugin()
{
	if (m_fInitialized)
		return true;

	// �ݒ�̓ǂݍ���
	::GetModuleFileName(g_hinstDLL,m_szIniFileName,MAX_PATH);
	::PathRenameExtension(m_szIniFileName,TEXT(".ini"));
	for (int i=0;i<NUM_SPARE_FOLDERS;i++) {
		TCHAR szKey[16];
		::wsprintf(szKey,TEXT("Folder%d"),i);
		::GetPrivateProfileString(TEXT("Settings"),szKey,NULL,
								  m_szSpareFolder[i],MAX_PATH,m_szIniFileName);
	}
	m_LowFreeSpace=::GetPrivateProfileInt(TEXT("Settings"),TEXT("LowFreeSpace"),
										  m_LowFreeSpace,m_szIniFileName);

	// �E�B���h�E�N���X�̓o�^
	WNDCLASS wc;
	wc.style=0;
	wc.lpfnWndProc=WndProc;
	wc.cbClsExtra=0;
	wc.cbWndExtra=0;
	wc.hInstance=g_hinstDLL;
	wc.hIcon=NULL;
	wc.hCursor=NULL;
	wc.hbrBackground=NULL;
	wc.lpszMenuName=NULL;
	wc.lpszClassName=DISKRELAY_WINDOW_CLASS;
	if (::RegisterClass(&wc)==0)
		return false;

	// �E�B���h�E�̍쐬
	m_hwnd=::CreateWindowEx(0,DISKRELAY_WINDOW_CLASS,NULL,WS_POPUP,
							0,0,0,0,HWND_MESSAGE,NULL,g_hinstDLL,this);
	if (m_hwnd==NULL)
		return false;

	m_fInitialized=true;
	return true;
}


// �I������
bool CDiskRelay::Finalize()
{
	// �E�B���h�E�̔j��
	if (m_hwnd)
		::DestroyWindow(m_hwnd);

	return true;
}


bool CDiskRelay::CheckFreeSpace()
{
	if (m_NextFolder>=NUM_SPARE_FOLDERS)
		return false;

	// �^��̏󋵂��擾����
	TVTest::RecordStatusInfo RecStat;
	TCHAR szFileName[MAX_PATH];
	RecStat.pszFileName=szFileName;
	RecStat.MaxFileName=MAX_PATH;
	if (m_pApp->GetRecordStatus(&RecStat)
			&& RecStat.Status==TVTest::RECORD_STATUS_RECORDING) {
		// �󂫗e�ʂ��擾
		TCHAR szPath[MAX_PATH+8];
		::lstrcpy(szPath,szFileName);
		*::PathFindFileName(szPath)='\0';
		ULARGE_INTEGER FreeSpace;
		if (::GetDiskFreeSpaceEx(szPath,&FreeSpace,NULL,NULL)
				&& FreeSpace.QuadPart<=(ULONGLONG)m_LowFreeSpace*0x100000) {
			m_pApp->AddLog(TEXT("�󂫗e�ʂ����Ȃ����ߑ�����\���̃t�H���_�ɘ^�悵�܂��B"),
						   TVTest::LOG_TYPE_WARNING);
			for (;m_NextFolder<NUM_SPARE_FOLDERS;m_NextFolder++) {
				if (m_szSpareFolder[m_NextFolder][0]!='\0'
						&& ::PathIsDirectory(m_szSpareFolder[m_NextFolder])) {
					::lstrcpy(szPath,m_szSpareFolder[m_NextFolder]);
					::PathAddBackslash(szPath);
					if (::GetDiskFreeSpaceEx(szPath,&FreeSpace,NULL,NULL)
							&& FreeSpace.QuadPart>(ULONGLONG)m_LowFreeSpace*0x100000) {
						::lstrcat(szPath,::PathFindFileName(szFileName));
						::wsprintf(::PathFindExtension(szPath),TEXT(".part%d%s"),
								   m_NextFolder+2,
								   ::PathFindExtension(szFileName));
						if (m_pApp->RelayRecord(szPath))
							return true;
					}
				}
			}
			m_pApp->AddLog(TEXT("�󂫗e�ʂ̂���t�H���_������܂���B"),
						   TVTest::LOG_TYPE_ERROR);
		}
	}
	return false;
}


// �C�x���g�R�[���o�b�N�֐�
// �����C�x���g���N����ƌĂ΂��
LRESULT CALLBACK CDiskRelay::EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
{
	CDiskRelay *pThis=static_cast<CDiskRelay*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// �v���O�C���̗L����Ԃ��ω�����
		if (lParam1!=0) {
			if (!pThis->InitializePlugin())
				return FALSE;

			// �܂��t�H���_���ݒ肳��Ă��Ȃ���ΐݒ�_�C�A���O��\������
			int i;
			for (i=0;i<NUM_SPARE_FOLDERS;i++) {
				if (pThis->m_szSpareFolder[i][0]!='\0')
					break;
			}
			if (i==NUM_SPARE_FOLDERS) {
				if (!pThis->SettingsDialog(pThis->m_pApp->GetAppWindow()))
					return FALSE;
			}

			if (pThis->m_fRecording)
				::SetTimer(pThis->m_hwnd,WATCH_TIMER_ID,WATCH_INTERVAL,NULL);
		} else {
			if (pThis->m_hwnd!=NULL && pThis->m_fRecording)
				::KillTimer(pThis->m_hwnd,WATCH_TIMER_ID);
		}
		return TRUE;

	case TVTest::EVENT_PLUGINSETTINGS:
		// �v���O�C���̐ݒ���s��
		pThis->InitializePlugin();
		return pThis->SettingsDialog(reinterpret_cast<HWND>(lParam1));

	case TVTest::EVENT_RECORDSTATUSCHANGE:
		// �^���Ԃ��ω�����
		if (lParam1!=TVTest::RECORD_STATUS_NOTRECORDING) {
			if (!pThis->m_fRecording) {
				pThis->m_fRecording=true;
				pThis->m_NextFolder=0;
				if (pThis->m_pApp->IsPluginEnabled()) {
					pThis->CheckFreeSpace();
					::SetTimer(pThis->m_hwnd,WATCH_TIMER_ID,WATCH_INTERVAL,NULL);
				}
			}
		} else {
			if (pThis->m_fRecording) {
				pThis->m_fRecording=false;
				if (pThis->m_hwnd!=NULL)
					::KillTimer(pThis->m_hwnd,WATCH_TIMER_ID);
			}
		}
		return TRUE;
	}

	return 0;
}


// �ݒ�_�C�A���O�̕\��
bool CDiskRelay::SettingsDialog(HWND hwndOwner)
{
	return ::DialogBoxParam(g_hinstDLL,MAKEINTRESOURCE(IDD_SETTINGS),
							hwndOwner,SettingsDlgProc,
							reinterpret_cast<LPARAM>(this))==IDOK;
}


// �ݒ�̕ۑ�
void CDiskRelay::SaveSettings()
{
	for (int i=0;i<NUM_SPARE_FOLDERS;i++) {
		TCHAR szKey[16];
		::wsprintf(szKey,TEXT("Folder%d"),i);
		::WritePrivateProfileString(TEXT("Settings"),szKey,m_szSpareFolder[i],m_szIniFileName);
	}
	TCHAR szValue[16];
	::wsprintf(szValue,TEXT("%u"),m_LowFreeSpace);
	::WritePrivateProfileString(TEXT("Settings"),TEXT("LowFreeSpace"),szValue,m_szIniFileName);
}


// �E�B���h�E�n���h������this���擾����
CDiskRelay *CDiskRelay::GetThis(HWND hwnd)
{
	return reinterpret_cast<CDiskRelay*>(::GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


// �E�B���h�E�v���V�[�W��
// �P�Ƀ^�C�}�[���������邾��
LRESULT CALLBACK CDiskRelay::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs=reinterpret_cast<LPCREATESTRUCT>(lParam);
			CDiskRelay *pThis=static_cast<CDiskRelay*>(pcs->lpCreateParams);

			::SetWindowLongPtr(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(pThis));
		}
		return TRUE;

	case WM_TIMER:
		{
			CDiskRelay *pThis=GetThis(hwnd);

			pThis->CheckFreeSpace();
		}
		return 0;
	}

	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


int CALLBACK BrowseFolderCallback(HWND hwnd,UINT uMsg,LPARAM lpData,LPARAM lParam)
{
	switch (uMsg) {
	case BFFM_INITIALIZED:
		{
			// �t�H���_�Q�Ǝ��̏����t�H���_��ݒ肷��
			LPCTSTR pszDirectory=reinterpret_cast<LPCTSTR>(lParam);

			if (pszDirectory[0]!='\0') {
				TCHAR szFolder[MAX_PATH];

				lstrcpy(szFolder,pszDirectory);
				PathRemoveBackslash(szFolder);
				SendMessage(hwnd,BFFM_SETSELECTION,
							TRUE,reinterpret_cast<LPARAM>(szFolder));
			}
		}
		break;
	}

	return 0;
}

// �t�H���_�Q�ƃ_�C�A���O
bool BrowseFolderDialog(HWND hwndOwner,LPTSTR pszDirectory,LPCTSTR pszTitle)
{
	BROWSEINFO bi;
	PIDLIST_ABSOLUTE pidl;
	BOOL fRet;

	bi.hwndOwner=hwndOwner;
	bi.pidlRoot=NULL;
	bi.pszDisplayName=pszDirectory;
	bi.lpszTitle=pszTitle;
	// BIF_NEWDIALOGSTYLE ���g���Ă邪�ACoInitialize �� TVTest �{�̂ŌĂ�ł���̂ŕs�v
	bi.ulFlags=BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	bi.lpfn=BrowseFolderCallback;
	bi.lParam=reinterpret_cast<LPARAM>(pszDirectory);
	pidl=SHBrowseForFolder(&bi);
	if (pidl==NULL)
		return false;
	fRet=SHGetPathFromIDList(pidl,pszDirectory);
	CoTaskMemFree(pidl);
	return fRet==TRUE;
}


// �ݒ�_�C�A���O�v���V�[�W��
INT_PTR CALLBACK CDiskRelay::SettingsDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static const LPCTSTR PROP_NAME=TEXT("ABDBEFF3-CB03-459F-9D44-CE65377C7792");

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CDiskRelay *pThis=reinterpret_cast<CDiskRelay*>(lParam);

			::SetProp(hDlg,PROP_NAME,pThis);

			// �f�t�H���g�̘^��t�H���_���擾����
			TCHAR szDefaultFolder[MAX_PATH];
			if (pThis->m_pApp->GetSetting(L"RecordFolder",szDefaultFolder,MAX_PATH)>0)
				::SetDlgItemText(hDlg,IDC_SETTINGS_DEFAULTFOLDER,szDefaultFolder);

			for (int i=0;i<NUM_SPARE_FOLDERS;i++) {
				::SendDlgItemMessage(hDlg,IDC_SETTINGS_FOLDER1+i*3,EM_LIMITTEXT,MAX_PATH-1,0);
				::SetDlgItemText(hDlg,IDC_SETTINGS_FOLDER1+i*3,pThis->m_szSpareFolder[i]);
			}

			::SetDlgItemInt(hDlg,IDC_SETTINGS_LOWSPACE,pThis->m_LowFreeSpace,FALSE);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SETTINGS_FOLDER1_BROWSE:
		case IDC_SETTINGS_FOLDER2_BROWSE:
		case IDC_SETTINGS_FOLDER3_BROWSE:
			{
				int EditID=LOWORD(wParam)-1;
				TCHAR szFolder[MAX_PATH];

				::GetDlgItemText(hDlg,EditID,szFolder,MAX_PATH);
				if (BrowseFolderDialog(hDlg,szFolder,TEXT("�\���̃t�H���_��I�����Ă�������"))) {
					::SetDlgItemText(hDlg,EditID,szFolder);
				}
			}
			return TRUE;

		case IDOK:
			{
				CDiskRelay *pThis=static_cast<CDiskRelay*>(::GetProp(hDlg,PROP_NAME));

				// �t�H���_�����邩�`�F�b�N����
				for (int i=0;i<NUM_SPARE_FOLDERS;i++) {
					TCHAR szFolder[MAX_PATH];
					::GetDlgItemText(hDlg,IDC_SETTINGS_FOLDER1+i*3,szFolder,MAX_PATH);
					if (szFolder[0]!='\0' && !::PathIsDirectory(szFolder)) {
						TCHAR szMessage[MAX_PATH+80];
						::wsprintf(szMessage,
								   TEXT("�t�H���_ \"%s\" ������܂���B\n�쐬���܂���?"),
								   szFolder);
						switch (::MessageBox(hDlg,szMessage,
											 TEXT("�t�H���_�쐬�̊m�F"),
											 MB_YESNOCANCEL | MB_ICONQUESTION)) {
						case IDYES:
							{
								int Result=::SHCreateDirectoryEx(hDlg,szFolder,NULL);
								if (Result!=ERROR_SUCCESS
										&& Result!=ERROR_ALREADY_EXISTS) {
									::wsprintf(szMessage,
											   TEXT("�t�H���_ \"%s\" ���쐬�ł��܂���B"),
											   szFolder);
									::MessageBox(hDlg,szMessage,NULL,
												 MB_OK | MB_ICONEXCLAMATION);
									return TRUE;
								}
							}
							break;
						case IDNO:
							break;
						default:
							return TRUE;
						}
					}
				}

				for (int i=0;i<NUM_SPARE_FOLDERS;i++) {
					::GetDlgItemText(hDlg,IDC_SETTINGS_FOLDER1+i*3,
									 pThis->m_szSpareFolder[i],MAX_PATH);
				}

				pThis->m_LowFreeSpace=
					::GetDlgItemInt(hDlg,IDC_SETTINGS_LOWSPACE,NULL,FALSE);

				pThis->SaveSettings();
			}
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_NCDESTROY:
		::RemoveProp(hDlg,PROP_NAME);
		return TRUE;
	}

	return FALSE;
}




// �v���O�C���N���X�̃C���X�^���X�𐶐�����
TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CDiskRelay;
}
