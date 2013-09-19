#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "NetworkRemocon.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#pragma comment(lib,"Ws2_32.lib")




class CSendStringInfo {
public:
	CNetworkRemocon *m_pRemocon;
	char *m_pBuffer;
	int m_Length;
	CNetworkRemoconReceiver *m_pReceiver;
	CSendStringInfo(CNetworkRemocon *pRemocon,const char *pBuffer,int Length,
					CNetworkRemoconReceiver *pReceiver) {
		m_pRemocon=pRemocon;
		m_pBuffer=new char[Length+1];
		CopyMemory(m_pBuffer,pBuffer,Length);
		m_pBuffer[Length]='\0';
		m_Length=Length;
		m_pReceiver=pReceiver;
	}
	~CSendStringInfo() {
		delete [] m_pBuffer;
	}
};




CNetworkRemocon::CNetworkRemocon()
{
	m_fInitialized=false;
	m_Address=INADDR_NONE;
	m_Port=0;
	m_hThread=NULL;
	m_Socket=INVALID_SOCKET;
	m_fConnected=false;
}


CNetworkRemocon::~CNetworkRemocon()
{
	if (m_fInitialized) {
		Shutdown();
		WSACleanup();
	}
}


bool CNetworkRemocon::Init(LPCSTR pszAddress,WORD Port)
{
	int Err;

	if (!m_fInitialized) {
		Err=WSAStartup(MAKEWORD(2,0),&m_WSAData);
		if (Err!=0)
			return false;
		m_fInitialized=true;
	} else {
		if (m_Address==inet_addr(pszAddress) && m_Port==Port)
			return true;
		Shutdown();
	}
	m_Address=inet_addr(pszAddress);
	if (m_Address==INADDR_NONE)
		return false;
	m_Port=Port;
	return true;
}


bool CNetworkRemocon::Shutdown()
{
	if (m_fInitialized) {
		if (m_hThread!=NULL) {
			if (WaitForSingleObject(m_hThread,5000)==WAIT_TIMEOUT)
				TerminateThread(m_hThread,FALSE);
			CloseHandle(m_hThread);
			m_hThread=NULL;
		}
		if (m_Socket!=INVALID_SOCKET) {
			shutdown(m_Socket,1);
			closesocket(m_Socket);
			m_Socket=INVALID_SOCKET;
		}
	}
	return true;
}


DWORD CNetworkRemocon::SendProc(LPVOID pParam)
{
	CSendStringInfo *pInfo=(CSendStringInfo*)pParam;
	int Result;
	char Buffer[4096];

	if (pInfo->m_pRemocon->m_Socket==INVALID_SOCKET) {
		pInfo->m_pRemocon->m_Socket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		if (pInfo->m_pRemocon->m_Socket==INVALID_SOCKET)
			return FALSE;
	}
	if (!pInfo->m_pRemocon->m_fConnected) {
		struct sockaddr_in sockaddri;

		sockaddri.sin_family=AF_INET;
		sockaddri.sin_port=htons(pInfo->m_pRemocon->m_Port);
		sockaddri.sin_addr.S_un.S_addr=pInfo->m_pRemocon->m_Address;
		ZeroMemory(sockaddri.sin_zero,sizeof(sockaddri.sin_zero));
		Result=connect(pInfo->m_pRemocon->m_Socket,
							(struct sockaddr*)&sockaddri,sizeof(sockaddr_in));
		if (Result!=0) {
			delete pInfo;
			return FALSE;
		}
		pInfo->m_pRemocon->m_fConnected=true;
	}
	Result=send(pInfo->m_pRemocon->m_Socket,pInfo->m_pBuffer,pInfo->m_Length,0);
	if (Result==SOCKET_ERROR) {
		closesocket(pInfo->m_pRemocon->m_Socket);
		pInfo->m_pRemocon->m_Socket=INVALID_SOCKET;
		pInfo->m_pRemocon->m_fConnected=false;
		return FALSE;
	}
	Result=recv(pInfo->m_pRemocon->m_Socket,Buffer,sizeof(Buffer)-1,0);
	if (Result!=SOCKET_ERROR && pInfo->m_pReceiver!=NULL) {
		Buffer[Result]='\0';
		pInfo->m_pReceiver->OnReceive(Buffer);
	}
	if (Result==SOCKET_ERROR || Result==0) {
		closesocket(pInfo->m_pRemocon->m_Socket);
		pInfo->m_pRemocon->m_Socket=INVALID_SOCKET;
		pInfo->m_pRemocon->m_fConnected=false;
		delete pInfo;
		return Result==SOCKET_ERROR;
	}
	delete pInfo;
	return TRUE;
}


bool CNetworkRemocon::Send(const char *pBuffer,int Length,
											CNetworkRemoconReceiver *pReceiver)
{
	DWORD ThreadID;
	CSendStringInfo *pInfo;

	if (!m_fInitialized)
		return false;
	if (m_hThread!=NULL) {
		if (WaitForSingleObject(m_hThread,0)==WAIT_TIMEOUT)
			return false;
		CloseHandle(m_hThread);
	}
	pInfo=new CSendStringInfo(this,pBuffer,Length,pReceiver);
	m_hThread=CreateThread(NULL,0,SendProc,pInfo,0,&ThreadID);
	if (m_hThread==NULL) {
		delete pInfo;
		return false;
	}
	return true;
}


bool CNetworkRemocon::SetChannel(int ChannelNo)
{
	char szText[16];
	int Length;

	if (m_ChannelList.NumChannels()>0) {
		int i;

		for (i=0;i<m_ChannelList.NumChannels();i++) {
			if (m_ChannelList.GetChannelNo(i)==ChannelNo+1) {
				Length=wsprintfA(szText,"SetCh:%d",i);
				return Send(szText,Length);
			}
		}
	} else {
		Length=wsprintfA(szText,"SetCh:%d",ChannelNo);
		return Send(szText,Length);
	}
	return false;
}


class CGetChannelReceiver : public CNetworkRemoconReceiver {
public:
	CNetworkRemoconReceiver *m_pReceiver;
	void OnReceive(LPCSTR pszText);
};

void CGetChannelReceiver::OnReceive(LPCSTR pszText)
{
	LPCSTR p;
	char szChannel[16];
	int i;

	p=pszText;
	while (*p!='\t') {
		if (*p=='\0')
			return;
		p++;
	}
	p++;
	for (i=0;*p>='0' && *p<='9';i++) {
		if (i==sizeof(szChannel))
			return;
		szChannel[i]=*p++;
	}
	if (i==0)
		return;
	szChannel[i]='\0';
	m_pReceiver->OnReceive(szChannel);
}


bool CNetworkRemocon::GetChannel(CNetworkRemoconReceiver *pReceiver)
{
	static CGetChannelReceiver Receiver;

	Receiver.m_pReceiver=pReceiver;
	return Send("GetChList",9,&Receiver);
}


bool CNetworkRemocon::SetService(int Service)
{
	char szText[16];
	int Length;

	Length=wsprintfA(szText,"SetService:%d",Service);
	return Send(szText,Length);
}


bool CNetworkRemocon::GetDriverList(CNetworkRemoconReceiver *pReceiver)
{
	return Send("GetBonList",10,pReceiver);
}


bool CNetworkRemocon::LoadChannelText(LPCTSTR pszFileName,
											const CChannelList *pChannelList)
{
	HANDLE hFile;
	DWORD FileSize,Read;
	char *pszText,*p;

	hFile=CreateFile(pszFileName,GENERIC_READ,FILE_SHARE_READ,NULL,
														OPEN_EXISTING,0,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return false;
	FileSize=GetFileSize(hFile,NULL);
	if (FileSize==INVALID_FILE_SIZE || FileSize==0) {
		CloseHandle(hFile);
		return false;
	}
	pszText=new char[FileSize+1];
	if (!ReadFile(hFile,pszText,FileSize,&Read,NULL) || Read!=FileSize) {
		CloseHandle(hFile);
		return false;
	}
	pszText[FileSize]='\0';
	CloseHandle(hFile);
	m_ChannelList.Clear();
	p=pszText;
	while (*p!='\0') {
		char *pszName;
		int Space,Channel;

		while (*p=='\r' || *p=='\n')
			p++;
		if (*p==';') {	// Comment
			while (*p!='\r' && *p!='\n' && *p!='\0')
				p++;
			continue;
		}
		pszName=p;
		while (*p!='\t' && *p!='\0')
			p++;
		if (*p=='\0')
			break;
		*p++='\0';
		Space=0;
		while (*p>='0' && *p<='9') {
			Space=Space*10+(*p-'0');
			p++;
		}
		if (*p!='\t')
			break;
		p++;
		Channel=0;
		while (*p>='0' && *p<='9') {
			Channel=Channel*10+(*p-'0');
			p++;
		}
		int i;
		for (i=0;i<pChannelList->NumChannels();i++) {
			const CChannelInfo *pChInfo=pChannelList->GetChannelInfo(i);
			if (pChInfo->GetSpace()==Space && pChInfo->GetChannelIndex()==Channel) {
				m_ChannelList.AddChannel(*pChInfo);
				break;
			}
		}
		if (i==pChannelList->NumChannels()) {
			TCHAR szName[MAX_CHANNEL_NAME];

			MultiByteToWideChar(CP_ACP,0,pszName,-1,szName,MAX_CHANNEL_NAME);
			m_ChannelList.AddChannel(
				CChannelInfo(Space,Channel,
							 pChannelList->NumChannels()==0?m_ChannelList.NumChannels()+1:Channel+1,
							 szName));
		}
		while (*p!='\r' && *p!='\n' && *p!='\0')
			p++;
	}
	delete [] pszText;
	return true;
}




CNetworkRemoconOptions::CNetworkRemoconOptions()
{
	m_fUseNetworkRemocon=false;
	::lstrcpyA(m_szAddress,"127.0.0.1");
	m_Port=1334;
	m_szChannelFileName[0]='\0';
	m_szDefaultChannelFileName[0]='\0';
	m_fTempEnable=false;
	m_TempPort=0;
}


CNetworkRemoconOptions::~CNetworkRemoconOptions()
{
	Destroy();
}


bool CNetworkRemoconOptions::ReadSettings(CSettings &Settings)
{
	TCHAR szText[16];

	Settings.Read(TEXT("UseNetworkRemocon"),&m_fUseNetworkRemocon);
	if (Settings.Read(TEXT("NetworkRemoconAddress"),szText,lengthof(szText)))
		::WideCharToMultiByte(CP_ACP,0,szText,-1,
							  m_szAddress,lengthof(m_szAddress),NULL,NULL);
	Settings.Read(TEXT("NetworkRemoconPort"),&m_Port);
	Settings.Read(TEXT("NetworkRemoconChFile"),
				  m_szChannelFileName,lengthof(m_szChannelFileName));
	return true;
}


bool CNetworkRemoconOptions::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("UseNetworkRemocon"),m_fUseNetworkRemocon);
	WCHAR szAddress[16];
	::MultiByteToWideChar(CP_ACP,0,m_szAddress,-1,szAddress,lengthof(szAddress));
	Settings.Write(TEXT("NetworkRemoconAddress"),szAddress);
	Settings.Write(TEXT("NetworkRemoconPort"),m_Port);
	Settings.Write(TEXT("NetworkRemoconChFile"),m_szChannelFileName);
	return true;
}


bool CNetworkRemoconOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_NETWORKREMOCON));
}


bool CNetworkRemoconOptions::SetTempEnable(bool fEnable)
{
	m_fTempEnable=fEnable;
	return true;
}


bool CNetworkRemoconOptions::SetTempPort(unsigned int Port)
{
	if (m_fUseNetworkRemocon || m_fTempEnable)
		m_TempPort=Port;
	return true;
}


bool CNetworkRemoconOptions::SetDefaultChannelFileName(LPCTSTR pszFileName)
{
	::lstrcpy(m_szDefaultChannelFileName,pszFileName);
	return true;
}


bool CNetworkRemoconOptions::IsEnable() const
{
	return m_fUseNetworkRemocon || m_fTempEnable;
}


bool CNetworkRemoconOptions::IsSettingValid() const
{
	return m_Port>0;
}


bool CNetworkRemoconOptions::CreateNetworkRemocon(CNetworkRemocon **ppNetworkRemocon)
{
	*ppNetworkRemocon=new CNetworkRemocon;
	if (!(*ppNetworkRemocon)->Init(m_szAddress,m_TempPort>0?m_TempPort:m_Port)) {
		delete *ppNetworkRemocon;
		*ppNetworkRemocon=NULL;
	}
	return true;
}


bool CNetworkRemoconOptions::InitNetworkRemocon(CNetworkRemocon **ppNetworkRemocon,
		const CCoreEngine *pCoreEngine,CChannelManager *pChannelManager) const
{
	if ((m_fUseNetworkRemocon || m_fTempEnable)
			&& pCoreEngine->IsNetworkDriver()) {
		TCHAR szChannelFile[MAX_PATH];

		if (!GetChannelFilePath(szChannelFile))
			return false;
		if (*ppNetworkRemocon==NULL)
			*ppNetworkRemocon=new CNetworkRemocon;
		if ((*ppNetworkRemocon)->LoadChannelText(szChannelFile,
								pChannelManager->GetFileAllChannelList())) {
			pChannelManager->SetNetworkRemoconMode(true,
									&(*ppNetworkRemocon)->GetChannelList());
			pChannelManager->SetNetworkRemoconCurrentChannel(-1);
		}
		(*ppNetworkRemocon)->Init(m_szAddress,m_TempPort>0?m_TempPort:m_Port);
	} else {
		if (*ppNetworkRemocon!=NULL) {
			delete *ppNetworkRemocon;
			*ppNetworkRemocon=NULL;
		}
	}
	return true;
}


bool CNetworkRemoconOptions::FindChannelFile(LPCTSTR pszDriverName,LPTSTR pszFileName) const
{
	TCHAR szMask[MAX_PATH];
	HANDLE hFind;
	WIN32_FIND_DATA wfd;

	::SHGetSpecialFolderPath(NULL,szMask,CSIDL_PERSONAL,FALSE);
	::PathAppend(szMask,TEXT("EpgTimerBon\\*(*).ChSet.txt"));
	hFind=::FindFirstFile(szMask,&wfd);
	if (hFind==INVALID_HANDLE_VALUE)
		return false;
	bool fFound=false;
	do {
		LPCTSTR p;
		TCHAR szName[MAX_PATH];
		int i;

		p=wfd.cFileName;
		while (*p!='(')
			p++;
		p++;
		for (i=0;*p!=')';i++)
			szName[i]=*p++;
		szName[i]='\0';
		if (IsEqualFileName(szName,pszDriverName)) {
			::lstrcpy(pszFileName,wfd.cFileName);
			fFound=true;
			break;
		}
	} while (::FindNextFile(hFind,&wfd));
	::FindClose(hFind);
	return fFound;
}


bool CNetworkRemoconOptions::GetChannelFilePath(LPTSTR pszPath) const
{
	if (m_szChannelFileName[0]=='\0' || ::PathIsFileSpec(m_szChannelFileName)) {
		::SHGetSpecialFolderPath(NULL,pszPath,CSIDL_PERSONAL,FALSE);
		::PathAppend(pszPath,TEXT("EpgTimerBon"));
		if (m_szChannelFileName[0]!='\0') {
			::PathAppend(pszPath,m_szChannelFileName);
		} else if (m_szDefaultChannelFileName[0]!='\0') {
			::PathAppend(pszPath,m_szDefaultChannelFileName);
		} else {
			TCHAR szMask[MAX_PATH];
			HANDLE hFind;
			WIN32_FIND_DATA fd;

			::lstrcpy(szMask,pszPath);
			::PathAppend(szMask,TEXT("BonDriver_*(*).ChSet.txt"));
			hFind=::FindFirstFile(szMask,&fd);
			if (hFind==INVALID_HANDLE_VALUE)
				return false;
			::FindClose(hFind);
			::PathAppend(pszPath,fd.cFileName);
		}
	} else {
		::lstrcpy(pszPath,m_szChannelFileName);
	}
	return true;
}


INT_PTR CNetworkRemoconOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			::CheckDlgButton(hDlg,IDC_NETWORKREMOCON_USE,
						m_fUseNetworkRemocon?BST_CHECKED:
						m_fTempEnable?BST_INDETERMINATE:BST_UNCHECKED);
			::SetDlgItemTextA(hDlg,IDC_NETWORKREMOCON_ADDRESS,m_szAddress);
			::SetDlgItemInt(hDlg,IDC_NETWORKREMOCON_PORT,m_Port,FALSE);
			{
				TCHAR szFileMask[MAX_PATH];
				HANDLE hFind;
				WIN32_FIND_DATA wfd;

				::SendDlgItemMessage(hDlg,IDC_NETWORKREMOCON_CHANNELFILE,CB_LIMITTEXT,MAX_PATH-1,0);
				::SHGetSpecialFolderPath(NULL,szFileMask,CSIDL_PERSONAL,FALSE);
				::PathAppend(szFileMask,TEXT("EpgTimerBon\\*(*).ChSet.txt"));
				hFind=::FindFirstFile(szFileMask,&wfd);
				if (hFind!=INVALID_HANDLE_VALUE) {
					do {
						::SendDlgItemMessage(hDlg,IDC_NETWORKREMOCON_CHANNELFILE,
							CB_ADDSTRING,0,
							reinterpret_cast<LPARAM>(wfd.cFileName));
					} while (::FindNextFile(hFind,&wfd));
					::FindClose(hFind);
				}
				::SetDlgItemText(hDlg,IDC_NETWORKREMOCON_CHANNELFILE,m_szChannelFileName);
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_NETWORKREMOCON_USE:
			::CheckDlgButton(hDlg,IDC_NETWORKREMOCON_USE,
				::IsDlgButtonChecked(hDlg,IDC_NETWORKREMOCON_USE)==BST_CHECKED?
													BST_UNCHECKED:BST_CHECKED);
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				int State;
				char szAddress[16];
				unsigned int Port;
				TCHAR szChannelFile[MAX_PATH];

				State=::IsDlgButtonChecked(hDlg,IDC_NETWORKREMOCON_USE);
				::GetDlgItemTextA(hDlg,IDC_NETWORKREMOCON_ADDRESS,szAddress,
														lengthof(szAddress));
				Port=::GetDlgItemInt(hDlg,IDC_NETWORKREMOCON_PORT,NULL,FALSE);
				::GetDlgItemText(hDlg,IDC_NETWORKREMOCON_CHANNELFILE,
										szChannelFile,lengthof(szChannelFile));
				bool fUpdate=false;
				if (State!=BST_INDETERMINATE
						&& (State==BST_CHECKED)!=(m_fUseNetworkRemocon || m_fTempEnable)) {
					m_fUseNetworkRemocon=State==BST_CHECKED;
					m_fTempEnable=false;
					fUpdate=true;
				} else if (m_fUseNetworkRemocon || m_fTempEnable) {
					if (m_Port!=Port
							|| ::lstrcmpiA(m_szAddress,szAddress)!=0
							|| !IsEqualFileName(m_szChannelFileName,szChannelFile)) {
						fUpdate=true;
					}
				}
				m_Port=Port;
				::lstrcpyA(m_szAddress,szAddress);
				::lstrcpy(m_szChannelFileName,szChannelFile);
				if (fUpdate)
					SetUpdateFlag(UPDATE_NETWORKREMOCON);

				m_fChanged=true;
			}
			return TRUE;
		}
		break;
	}

	return FALSE;
}
