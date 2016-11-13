#ifndef TVTEST_DPI_UTIL_H
#define TVTEST_DPI_UTIL_H


// from <windef.h> (SDK 10.0.14393.0)
#ifndef _DPI_AWARENESS_CONTEXTS_
DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
#define DPI_AWARENESS_CONTEXT_UNAWARE           ((DPI_AWARENESS_CONTEXT)-1)
#define DPI_AWARENESS_CONTEXT_SYSTEM_AWARE      ((DPI_AWARENESS_CONTEXT)-2)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE ((DPI_AWARENESS_CONTEXT)-3)
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

	class DPIBlockBase
	{
	public:
		DPIBlockBase(DPI_AWARENESS_CONTEXT Context);
		~DPIBlockBase();

	private:
		DPI_AWARENESS_CONTEXT m_OldContext;
	};

	class SystemDPIBlock : public DPIBlockBase
	{
	public:
		SystemDPIBlock() : DPIBlockBase(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE) {}
	};

	class PerMonitorDPIBlock : public DPIBlockBase
	{
	public:
		PerMonitorDPIBlock() : DPIBlockBase(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE) {}
	};

}


#endif
