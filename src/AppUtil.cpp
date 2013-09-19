#include "stdafx.h"
#include <psapi.h>	// for GetModuleFileNameEx
#include "TVTest.h"
#include "AppUtil.h"
#include "AppMain.h"
#include "MainWindow.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#pragma comment(lib,"psapi.lib")




bool IsTVTestWindow(HWND hwnd)
{
	TCHAR szClass[64];

	return ::GetClassName(hwnd,szClass,lengthof(szClass))==lengthof(MAIN_WINDOW_CLASS)-1
			&& ::lstrcmpi(szClass,MAIN_WINDOW_CLASS)==0;
}


struct TVTestWindowEnumInfo {
	WNDENUMPROC pEnumFunc;
	LPARAM Param;
};

static BOOL CALLBACK TVTestWindowEnumProc(HWND hwnd,LPARAM Param)
{
	if (IsTVTestWindow(hwnd)) {
		TVTestWindowEnumInfo *pInfo=reinterpret_cast<TVTestWindowEnumInfo*>(Param);

		if (!(*pInfo->pEnumFunc)(hwnd,pInfo->Param))
			return FALSE;
	}
	return TRUE;
}

bool EnumTVTestWindows(WNDENUMPROC pEnumFunc,LPARAM Param)
{
	TVTestWindowEnumInfo Info;

	Info.pEnumFunc=pEnumFunc;
	Info.Param=Param;
	return EnumWindows(TVTestWindowEnumProc,reinterpret_cast<LPARAM>(&Info))!=FALSE;
}




CAppMutex::CAppMutex(bool fEnable)
{
	if (fEnable) {
		TCHAR szName[MAX_PATH];

		::GetModuleFileName(NULL,szName,lengthof(szName));
		::CharUpperBuff(szName,::lstrlen(szName));
		for (int i=0;szName[i]!='\0';i++) {
			if (szName[i]=='\\')
				szName[i]=':';
		}
		SECURITY_DESCRIPTOR sd;
		SECURITY_ATTRIBUTES sa;
		::ZeroMemory(&sd,sizeof(sd));
		::InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION);
		::SetSecurityDescriptorDacl(&sd,TRUE,NULL,FALSE);
		::ZeroMemory(&sa,sizeof(sa));
		sa.nLength=sizeof(sa);
		sa.lpSecurityDescriptor=&sd;
		m_hMutex=::CreateMutex(&sa,FALSE,szName);
		m_fAlreadyExists=m_hMutex!=NULL && ::GetLastError()==ERROR_ALREADY_EXISTS;
	} else {
		m_hMutex=NULL;
		m_fAlreadyExists=false;
	}
}


CAppMutex::~CAppMutex()
{
	if (m_hMutex!=NULL) {
		/*
		if (!m_fAlreadyExists)
			::ReleaseMutex(m_hMutex);
		*/
		::CloseHandle(m_hMutex);
	}
}




HWND CTVTestWindowFinder::FindCommandLineTarget()
{
	::GetModuleFileName(NULL,m_szModuleFileName,lengthof(m_szModuleFileName));
	m_hwndFirst=NULL;
	m_hwndFound=NULL;
	::EnumWindows(FindWindowCallback,reinterpret_cast<LPARAM>(this));
	return m_hwndFound!=NULL?m_hwndFound:m_hwndFirst;
}


BOOL CALLBACK CTVTestWindowFinder::FindWindowCallback(HWND hwnd,LPARAM lParam)
{
	CTVTestWindowFinder *pThis=reinterpret_cast<CTVTestWindowFinder*>(lParam);
	TCHAR szClassName[64],szFileName[MAX_PATH];

	if (::GetClassName(hwnd,szClassName,lengthof(szClassName))>0
			&& ::lstrcmpi(szClassName,MAIN_WINDOW_CLASS)==0) {
		if (pThis->m_hwndFirst==NULL)
			pThis->m_hwndFirst=hwnd;

		DWORD ProcessId;
		HANDLE hProcess;
		::GetWindowThreadProcessId(hwnd,&ProcessId);
		hProcess=::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,FALSE,ProcessId);
		if (hProcess!=NULL
				&& ::GetModuleFileNameEx(hProcess,NULL,szFileName,lengthof(szFileName))>0
				&& IsEqualFileName(szFileName,pThis->m_szModuleFileName)) {
			pThis->m_hwndFound=hwnd;
			::CloseHandle(hProcess);
			return FALSE;
		}
		::CloseHandle(hProcess);
	}
	return TRUE;
}




bool CPortQuery::Query(HWND hwnd,WORD *pUDPPort,WORD MaxPort
#ifdef NETWORK_REMOCON_SUPPORT
					   ,WORD *pRemoconPort
#endif
					   )
{
	size_t i;

	m_hwndSelf=hwnd;
	m_UDPPortList.clear();
#ifdef NETWORK_REMOCON_SUPPORT
	m_RemoconPortList.clear();
#endif
	::EnumWindows(EnumProc,reinterpret_cast<LPARAM>(this));
	if (m_UDPPortList.size()>0) {
		WORD UDPPort;

		for (UDPPort=*pUDPPort;UDPPort<=MaxPort;UDPPort++) {
			for (i=0;i<m_UDPPortList.size();i++) {
				if (m_UDPPortList[i]==UDPPort)
					break;
			}
			if (i==m_UDPPortList.size())
				break;
		}
		if (UDPPort>MaxPort)
			UDPPort=0;
		*pUDPPort=UDPPort;
	}
#ifdef NETWORK_REMOCON_SUPPORT
	if (m_RemoconPortList.size()>0) {
		WORD RemoconPort;

		for (RemoconPort=*pRemoconPort;;RemoconPort++) {
			for (i=0;i<m_RemoconPortList.size();i++) {
				if (m_RemoconPortList[i]==RemoconPort)
					break;
			}
			if (i==m_RemoconPortList.size())
				break;
		}
		*pRemoconPort=RemoconPort;
	}
#endif
	return true;
}


BOOL CALLBACK CPortQuery::EnumProc(HWND hwnd,LPARAM lParam)
{
	CPortQuery *pThis=reinterpret_cast<CPortQuery*>(lParam);

	if (hwnd==pThis->m_hwndSelf)
		return TRUE;
	if (IsTVTestWindow(hwnd)) {
		DWORD_PTR Result;

		if (::SendMessageTimeout(hwnd,WM_APP_QUERYPORT,0,0,
								 SMTO_NORMAL | SMTO_ABORTIFHUNG,1000,&Result)) {
			WORD UDPPort=LOWORD(Result);
#ifdef NETWORK_REMOCON_SUPPORT
			WORD RemoconPort=HIWORD(Result);
#endif

			TRACE(TEXT("CPortQuery::EnumProc %d %d\n"),UDPPort,RemoconPort);
			pThis->m_UDPPortList.push_back(UDPPort);
#ifdef NETWORK_REMOCON_SUPPORT
			if (RemoconPort>0)
				pThis->m_RemoconPortList.push_back(RemoconPort);
			GetAppClass().AddLog(TEXT("既に起動している") APP_NAME TEXT("が見付かりました。(UDPポート %d / リモコンポート %d)"),UDPPort,RemoconPort);
#else
			GetAppClass().AddLog(TEXT("既に起動している") APP_NAME TEXT("が見付かりました。(UDPポート %d)"),UDPPort);
#endif
		}
	}
	return TRUE;
}
