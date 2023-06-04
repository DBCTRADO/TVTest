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


#ifndef TVTEST_DPI_UTIL_H
#define TVTEST_DPI_UTIL_H


// from <windef.h> (SDK 10.0.16299.0)
#ifndef _DPI_AWARENESS_CONTEXTS_
DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
#define DPI_AWARENESS_CONTEXT_UNAWARE              ((DPI_AWARENESS_CONTEXT)-1)
#define DPI_AWARENESS_CONTEXT_SYSTEM_AWARE         ((DPI_AWARENESS_CONTEXT)-2)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE    ((DPI_AWARENESS_CONTEXT)-3)
#endif
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
#endif


namespace TVTest
{

	bool EnableNonClientDPIScaling(HWND hwnd);
	int GetSystemDPI();
	int GetMonitorDPI(HMONITOR hMonitor);
	int GetWindowDPI(HWND hwnd);
	int GetSystemMetricsWithDPI(int Index, int DPI, bool fFallbackScaling = true);
	bool SystemParametersInfoWithDPI(UINT Action, UINT Param, void *pParam, UINT Flags, int DPI);
	bool AdjustWindowRectWithDPI(RECT *pRect, DWORD Style, DWORD ExStyle, bool fMenu, int DPI);
	int GetScrollBarWidth(HWND hwnd);
	DPI_AWARENESS_CONTEXT GetWindowDPIAwareness(HWND hwnd);
	bool IsWindowPerMonitorDPIV1(HWND hwnd);
	bool IsWindowPerMonitorDPIV2(HWND hwnd);
	bool IsPerMonitorDPIV2Available();

	class DPIBlockBase
	{
	public:
		DPIBlockBase(DPI_AWARENESS_CONTEXT Context);
		~DPIBlockBase();

		DPIBlockBase(const DPIBlockBase &) = delete;
		DPIBlockBase &operator=(const DPIBlockBase &) = delete;

	private:
		DPI_AWARENESS_CONTEXT m_OldContext;
	};

	class SystemDPIBlock
		: public DPIBlockBase
	{
	public:
		SystemDPIBlock() : DPIBlockBase(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE) {}
	};

	class PerMonitorDPIBlock
		: public DPIBlockBase
	{
	public:
		PerMonitorDPIBlock() : DPIBlockBase(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE) {}
	};

	class CommonDialogDPIBlock
		: public DPIBlockBase
	{
	public:
		CommonDialogDPIBlock();
	};

}


#endif
