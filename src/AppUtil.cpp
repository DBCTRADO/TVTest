/*
  TVTest
  Copyright(c) 2008-2020 DBCTRADO

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "stdafx.h"
#include "TVTest.h"
#include "AppUtil.h"
#include "AppMain.h"
#include "MainWindow.h"
#include "Common/DebugDef.h"


namespace TVTest
{


bool IsTVTestWindow(HWND hwnd)
{
	TCHAR szClass[64];

	return ::GetClassName(hwnd, szClass, lengthof(szClass)) > 0
		&& ::lstrcmpi(szClass, MAIN_WINDOW_CLASS) == 0;
}


struct TVTestWindowEnumInfo {
	WNDENUMPROC pEnumFunc;
	LPARAM Param;
};

static BOOL CALLBACK TVTestWindowEnumProc(HWND hwnd, LPARAM Param)
{
	if (IsTVTestWindow(hwnd)) {
		const TVTestWindowEnumInfo *pInfo = reinterpret_cast<const TVTestWindowEnumInfo*>(Param);

		if (!(*pInfo->pEnumFunc)(hwnd, pInfo->Param))
			return FALSE;
	}
	return TRUE;
}

bool EnumTVTestWindows(WNDENUMPROC pEnumFunc, LPARAM Param)
{
	TVTestWindowEnumInfo Info;

	Info.pEnumFunc = pEnumFunc;
	Info.Param = Param;
	return EnumWindows(TVTestWindowEnumProc, reinterpret_cast<LPARAM>(&Info)) != FALSE;
}




CAppMutex::CAppMutex(bool fEnable)
{
	if (fEnable) {
		TCHAR szName[MAX_PATH];

		const DWORD Length = ::GetModuleFileName(nullptr, szName, lengthof(szName));
		::CharUpperBuff(szName, Length);
		std::ranges::replace(szName, szName + Length, _T('\\'), _T(':'));

		CBasicSecurityAttributes SecAttributes;
		SecAttributes.Initialize();

		m_hMutex = ::CreateMutex(&SecAttributes, FALSE, szName);
		m_fAlreadyExists = m_hMutex != nullptr && ::GetLastError() == ERROR_ALREADY_EXISTS;
	} else {
		m_hMutex = nullptr;
		m_fAlreadyExists = false;
	}
}


CAppMutex::~CAppMutex()
{
	if (m_hMutex != nullptr) {
		/*
		if (!m_fAlreadyExists)
			::ReleaseMutex(m_hMutex);
		*/
		::CloseHandle(m_hMutex);
	}
}




HWND CTVTestWindowFinder::FindCommandLineTarget()
{
	::GetModuleFileName(nullptr, m_szModuleFileName, lengthof(m_szModuleFileName));
	m_hwndFirst = nullptr;
	m_hwndFound = nullptr;
	::EnumWindows(FindWindowCallback, reinterpret_cast<LPARAM>(this));
	return m_hwndFound != nullptr ? m_hwndFound : m_hwndFirst;
}


BOOL CALLBACK CTVTestWindowFinder::FindWindowCallback(HWND hwnd, LPARAM lParam)
{
	CTVTestWindowFinder *pThis = reinterpret_cast<CTVTestWindowFinder*>(lParam);
	TCHAR szClassName[64], szFileName[MAX_PATH];

	if (::GetClassName(hwnd, szClassName, lengthof(szClassName)) > 0
			&& ::lstrcmpi(szClassName, MAIN_WINDOW_CLASS) == 0) {
		if (pThis->m_hwndFirst == nullptr)
			pThis->m_hwndFirst = hwnd;

		DWORD ProcessId;
		HANDLE hProcess;
		::GetWindowThreadProcessId(hwnd, &ProcessId);
		hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessId);
		if (hProcess != nullptr) {
			DWORD FileNameSize = lengthof(szFileName);
			if (::QueryFullProcessImageName(hProcess, 0, szFileName, &FileNameSize)
					&& IsEqualFileName(szFileName, pThis->m_szModuleFileName)) {
				pThis->m_hwndFound = hwnd;
				::CloseHandle(hProcess);
				return FALSE;
			}
		}
		::CloseHandle(hProcess);
	}
	return TRUE;
}




bool CPortQuery::Query(HWND hwnd, WORD *pUDPPort, WORD MaxPort)
{
	m_hwndSelf = hwnd;
	m_UDPPortList.clear();
	::EnumWindows(EnumProc, reinterpret_cast<LPARAM>(this));
	if (m_UDPPortList.size() > 0) {
		WORD UDPPort;

		for (UDPPort = *pUDPPort; UDPPort <= MaxPort; UDPPort++) {
			if (std::ranges::find(m_UDPPortList, UDPPort) == m_UDPPortList.end())
				break;
		}
		if (UDPPort > MaxPort)
			UDPPort = 0;
		*pUDPPort = UDPPort;
	}
	return true;
}


BOOL CALLBACK CPortQuery::EnumProc(HWND hwnd, LPARAM lParam)
{
	CPortQuery *pThis = reinterpret_cast<CPortQuery*>(lParam);

	if (hwnd == pThis->m_hwndSelf)
		return TRUE;
	if (IsTVTestWindow(hwnd)) {
		DWORD_PTR Result = 0;

		if (::SendMessageTimeout(
					hwnd, WM_APP_QUERYPORT, 0, 0,
					SMTO_NORMAL | SMTO_ABORTIFHUNG, 1000, &Result)) {
			const WORD UDPPort = LOWORD(Result);

			pThis->m_UDPPortList.push_back(UDPPort);
			GetAppClass().AddLog(TEXT("既に起動している") APP_NAME TEXT("が見付かりました。(UDPポート {})"), UDPPort);
		}
	}
	return TRUE;
}


}	// namespace TVTest
