#ifndef APP_UTIL_H
#define APP_UTIL_H


#include <vector>


bool IsTVTestWindow(HWND hwnd);
bool EnumTVTestWindows(WNDENUMPROC pEnumFunc,LPARAM Param=0);

class CAppMutex {
	HANDLE m_hMutex;
	bool m_fAlreadyExists;
public:
	CAppMutex(bool fEnable);
	~CAppMutex();
	bool AlreadyExists() const { return m_fAlreadyExists; }
};

class CTVTestWindowFinder {
	TCHAR m_szModuleFileName[MAX_PATH];
	HWND m_hwndFirst;
	HWND m_hwndFound;
	static BOOL CALLBACK FindWindowCallback(HWND hwnd,LPARAM lParam);
public:
	HWND FindCommandLineTarget();
};

class CPortQuery {
	HWND m_hwndSelf;
	std::vector<WORD> m_UDPPortList;
#ifdef NETWORK_REMOCON_SUPPORT
	std::vector<WORD> m_RemoconPortList;
#endif
	static BOOL CALLBACK EnumProc(HWND hwnd,LPARAM lParam);
public:
	bool Query(HWND hwnd,WORD *pUDPPort,WORD MaxPort
#ifdef NETWORK_REMOCON_SUPPORT
			   ,WORD *pRemoconPort
#endif
			   );
};


#endif
