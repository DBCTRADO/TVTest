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
#include "AppMain.h"
#include "TVTestVersion.h"
#include "LibISDB/LibISDB/LibISDBVersion.hpp"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CAppMain &GetAppClass()
{
	static CAppMain App;
	return App;
}




bool CTotTimeAdjuster::BeginAdjust(DWORD TimeOut)
{
	m_TimeOut = TimeOut;
	m_StartTime = ::GetTickCount();
	m_PrevTime.wYear = 0;
	m_fEnable = true;
	return true;
}


bool CTotTimeAdjuster::AdjustTime()
{
	if (!m_fEnable)
		return false;
	if (TickTimeSpan(m_StartTime, ::GetTickCount()) >= m_TimeOut) {
		m_fEnable = false;
		return false;
	}

	const LibISDB::AnalyzerFilter *pAnalyzer =
		GetAppClass().CoreEngine.GetFilter<LibISDB::AnalyzerFilter>();
	if (pAnalyzer == nullptr)
		return false;
	LibISDB::DateTime TOTTime;
	if (!pAnalyzer->GetTOTTime(&TOTTime))
		return false;

	SYSTEMTIME st = TOTTime.ToSYSTEMTIME();
	if (m_PrevTime.wYear == 0) {
		m_PrevTime = st;
		return false;
	} else if (CompareSystemTime(&m_PrevTime, &st) == 0) {
		return false;
	}

	bool fOK = false;
	HANDLE hToken;
	if (::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		LUID luid;
		if (::LookupPrivilegeValue(nullptr, SE_SYSTEMTIME_NAME, &luid)) {
			TOKEN_PRIVILEGES tkp;
			tkp.PrivilegeCount = 1;
			tkp.Privileges[0].Luid = luid;
			tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			if (::AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(TOKEN_PRIVILEGES), nullptr, 0)
					&& ::GetLastError() == ERROR_SUCCESS) {
				// バッファがあるので少し時刻を戻す
				OffsetSystemTime(&st, -2 * TimeConsts::SYSTEMTIME_SECOND);
				if (::SetLocalTime(&st)) {
					GetAppClass().AddLog(
						TEXT("TOTで時刻合わせを行いました。({}/{}/{} {}:{:02}:{:02})"),
						st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
					fOK = true;
				}
			}
		}
		::CloseHandle(hToken);
	}
	m_fEnable = false;
	return fOK;
}




void CEpgLoadEventHandler::OnBeginEpgDataLoading()
{
	TRACE(TEXT("Start EPG file loading ...\n"));
}


void CEpgLoadEventHandler::OnEndEpgDataLoading(bool fSuccess)
{
	CAppMain &App = GetAppClass();

	if (fSuccess) {
		App.AddLog(TEXT("EPG データを読み込みました。"));

		App.MainWindow.PostMessage(WM_APP_EPGLOADED, 0, 0);
	} else {
		App.AddLog(CLogItem::LogType::Error, TEXT("EPG データを読み込めませんでした。"));
	}
}


void CEpgLoadEventHandler::OnBeginEdcbDataLoading()
{
	TRACE(TEXT("Start EDCB data loading ...\n"));
}


void CEpgLoadEventHandler::OnEndEdcbDataLoading(bool fSuccess, LibISDB::EPGDatabase *pEPGDatabase)
{
	CAppMain &App = GetAppClass();

	if (fSuccess) {
		GetAppClass().AddLog(TEXT("EDCB の EPG データを読み込みました。"));

		if (App.EPGDatabase.Merge(pEPGDatabase, LibISDB::EPGDatabase::MergeFlag::Database)) {
			App.MainWindow.PostMessage(WM_APP_EPGLOADED, 0, 0);
		}
	} else {
		App.AddLog(CLogItem::LogType::Error, TEXT("EDCB の EPG データを読み込めませんでした。"));
	}
}




CServiceUpdateInfo::CServiceUpdateInfo(LibISDB::TSEngine *pEngine, LibISDB::AnalyzerFilter *pAnalyzer)
{
	pEngine->GetSelectableServiceList(&m_ServiceList);
	m_CurService = -1;
	if (!m_ServiceList.empty()) {
		const WORD ServiceID = pEngine->GetServiceID();
		if (ServiceID != LibISDB::SERVICE_ID_INVALID) {
			for (size_t i = 0; i < m_ServiceList.size(); i++) {
				if (m_ServiceList[i].ServiceID == ServiceID) {
					m_CurService = static_cast<int>(i);
					break;
				}
			}
		}
	}
	m_NetworkID = pAnalyzer->GetNetworkID();
	m_TransportStreamID = pAnalyzer->GetTransportStreamID();
	m_fServiceListEmpty = pAnalyzer->GetServiceCount() == 0;
}


}	// namespace TVTest




// エントリポイント
int APIENTRY _tWinMain(
	HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/,
	LPTSTR pszCmdLine, int nCmdShow)
{
	// DLLハイジャック対策
	SetDllDirectory(TEXT(""));

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF/* | _CRTDBG_CHECK_ALWAYS_DF*/);
#else
	TVTest::CDebugHelper::Initialize();
	TVTest::CDebugHelper::SetExceptionFilterMode(TVTest::CDebugHelper::ExceptionFilterMode::Dialog);
#endif

	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

	TVTest::CAppMain &App = TVTest::GetAppClass();

	App.AddLog(
		TEXT("******** ") ABOUT_VERSION_TEXT
#ifdef VERSION_HASH
		TEXT(" ") VERSION_HASH
#endif
		TEXT(" (")
#ifdef _DEBUG
		TEXT("Debug")
#else
		TEXT("Release")
#endif
#ifdef VERSION_PLATFORM
		TEXT(" ") VERSION_PLATFORM
#endif
		TEXT(") 起動 ********"));
	App.AddLog(
		TEXT("Work with LibISDB ver.") LIBISDB_VERSION_STRING
#ifdef LIBISDB_VERSION_HASH
		TEXT(" ") LIBISDB_VERSION_HASH
#endif
		);
#ifdef _MSC_FULL_VER
	App.AddLog(
		TEXT("Compiled with MSVC {}.{}.{}.{}"),
		_MSC_FULL_VER / 10000000, (_MSC_FULL_VER / 100000) % 100, _MSC_FULL_VER % 100000, _MSC_BUILD);
#endif

	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE | COINIT_SPEED_OVER_MEMORY);

	const int Result = App.Main(hInstance, pszCmdLine, nCmdShow);

	CoUninitialize();

	App.AddLog(TEXT("******** 終了 ********"));
	if (App.CmdLineOptions.m_fSaveLog && !App.Logger.GetOutputToFile()) {
		App.Logger.SaveToFile(nullptr, true);
	}

	return Result;
}
