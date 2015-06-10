#include "stdafx.h"
#include "TVTest.h"
#include "AppUtil.h"
#include "AppMain.h"
#include "MainWindow.h"
#include "Common/DebugDef.h"

#ifdef WIN_XP_SUPPORT
#include <psapi.h>	// for GetModuleFileNameEx
#pragma comment(lib,"psapi.lib")
#endif




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

		CBasicSecurityAttributes SecAttributes;
		SecAttributes.Initialize();

		m_hMutex=::CreateMutex(&SecAttributes,FALSE,szName);
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
		if (hProcess!=NULL) {
#ifdef WIN_XP_SUPPORT
			if (::GetModuleFileNameEx(hProcess,NULL,szFileName,lengthof(szFileName))>0
#else
			DWORD FileNameSize=lengthof(szFileName);
			if (::QueryFullProcessImageName(hProcess,0,szFileName,&FileNameSize)
#endif
					&& IsEqualFileName(szFileName,pThis->m_szModuleFileName)) {
				pThis->m_hwndFound=hwnd;
				::CloseHandle(hProcess);
				return FALSE;
			}
		}
		::CloseHandle(hProcess);
	}
	return TRUE;
}




bool CPortQuery::Query(HWND hwnd,WORD *pUDPPort,WORD MaxPort)
{
	size_t i;

	m_hwndSelf=hwnd;
	m_UDPPortList.clear();
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

			pThis->m_UDPPortList.push_back(UDPPort);
			GetAppClass().AddLog(TEXT("既に起動している") APP_NAME TEXT("が見付かりました。(UDPポート %d)"),UDPPort);
		}
	}
	return TRUE;
}
