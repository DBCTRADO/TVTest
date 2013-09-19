#ifndef DEBUG_HELPER_H
#define DEBUG_HELPER_H


#if !defined(_DEBUG) && (defined(_M_IX86) || defined(_M_AMD64) || defined(_M_IA64))
#define ENABLE_DEBUG_HELPER
#endif

#ifdef ENABLE_DEBUG_HELPER
#include <dbghelp.h>
#include <tlhelp32.h>
#ifdef UNICODE
#undef Module32First
#undef Module32Next
#undef MODULEENTRY32
#endif
#endif


class CDebugHelper
{
public:
	enum ExceptionFilterMode {
		EXCEPTION_FILTER_DEFAULT,
		EXCEPTION_FILTER_NONE,
		EXCEPTION_FILTER_DIALOG
	};

	CDebugHelper();
	~CDebugHelper();
	static bool Initialize();
	static bool SetExceptionFilterMode(ExceptionFilterMode Mode);

private:
#ifdef ENABLE_DEBUG_HELPER
	typedef BOOL (WINAPI *SymInitializeFunc)(HANDLE hProcess,PCTSTR UserSearchPath,BOOL fInvadeProcess);
	typedef BOOL (WINAPI *SymCleanupFunc)(HANDLE hProcess);
	typedef BOOL (WINAPI *SymFromAddrFunc)(HANDLE hProcess,DWORD64 Address,PDWORD64 Displacement,PSYMBOL_INFO Symbol);
	typedef DWORD (WINAPI *SymSetOptionsFunc)(DWORD SymOptions);
	typedef DWORD (WINAPI *SymLoadModuleFunc)(HANDLE hProcess,HANDLE hFile,
		PSTR ImageName,PSTR ModuleName,DWORD64 BaseOfDll,DWORD SizeOfDll);
	typedef BOOL (WINAPI *StackWalkFunc)(DWORD MachineType,HANDLE hProcess,
		HANDLE hThread,LPSTACKFRAME64 StackFrame,PVOID ContextRecord,
		PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
		PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
		PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
		PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress);
	typedef DWORD64 (WINAPI *SymGetModuleBaseFunc)(HANDLE hProcess,DWORD64 dwAddr);
	typedef BOOL (WINAPI *SymGetModuleInfoFunc)(HANDLE hProcess,DWORD64 dwAddr,PIMAGEHLP_MODULE64 ModuleInfo);

	static HMODULE m_hDbgHelp;
	static SymInitializeFunc m_pSymInitialize;
	static SymCleanupFunc m_pSymCleanup;
	static SymFromAddrFunc m_pSymFromAddr;
	static SymSetOptionsFunc m_pSymSetOptions;
	static SymLoadModuleFunc m_pSymLoadModule;
	static StackWalkFunc m_pStackWalk;
	//static SymGetModuleBaseFunc m_pSymGetModuleBase;
	//static SymGetModuleInfoFunc m_pSymGetModuleInfo;
	static int FormatSymbolFromAddress(HANDLE hProcess,DWORD64 Address,
		const MODULEENTRY32 *pModuleEntries,int NumModuleEntries,
		PSYMBOL_INFO pSymbolInfo,char *pszText);
#endif
	static ExceptionFilterMode m_ExceptionFilterMode;
	static LONG WINAPI ExceptionFilter(EXCEPTION_POINTERS *ExceptionInfo);
};


#endif
