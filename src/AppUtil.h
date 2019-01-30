/*
  TVTest
  Copyright(c) 2008-2019 DBCTRADO

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


#ifndef TVTEST_APP_UTIL_H
#define TVTEST_APP_UTIL_H


#include <vector>


namespace TVTest
{

	bool IsTVTestWindow(HWND hwnd);
	bool EnumTVTestWindows(WNDENUMPROC pEnumFunc, LPARAM Param = 0);

	class CAppMutex
	{
		HANDLE m_hMutex;
		bool m_fAlreadyExists;

	public:
		CAppMutex(bool fEnable);
		~CAppMutex();

		CAppMutex(const CAppMutex &) = delete;
		CAppMutex &operator=(const CAppMutex &) = delete;

		bool AlreadyExists() const { return m_fAlreadyExists; }
	};

	class CTVTestWindowFinder
	{
		TCHAR m_szModuleFileName[MAX_PATH];
		HWND m_hwndFirst;
		HWND m_hwndFound;
		static BOOL CALLBACK FindWindowCallback(HWND hwnd, LPARAM lParam);

	public:
		HWND FindCommandLineTarget();
	};

	class CPortQuery
	{
		HWND m_hwndSelf;
		std::vector<WORD> m_UDPPortList;
		static BOOL CALLBACK EnumProc(HWND hwnd, LPARAM lParam);

	public:
		bool Query(HWND hwnd, WORD *pUDPPort, WORD MaxPort);
	};

}


#endif
