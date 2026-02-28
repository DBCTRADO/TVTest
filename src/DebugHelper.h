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


#ifndef DEBUG_HELPER_H
#define DEBUG_HELPER_H


#if !defined(_DEBUG) && (defined(_M_IX86) || defined(_M_AMD64) || defined(_M_IA64))
#define ENABLE_DEBUG_HELPER
#endif

#ifdef ENABLE_DEBUG_HELPER
#pragma warning(push)
#pragma warning(disable:4091)
#include <dbghelp.h>
#include <tlhelp32.h>
#pragma warning(pop)
#ifdef UNICODE
#undef Module32First
#undef Module32Next
#undef MODULEENTRY32
#endif
#endif


namespace TVTest
{

	class CDebugHelper
	{
	public:
		enum class ExceptionFilterMode {
			Default,
			None,
			Dialog,
		};

		CDebugHelper();
		~CDebugHelper();

		static bool Initialize();
		static bool SetExceptionFilterMode(ExceptionFilterMode Mode);

	private:
#ifdef ENABLE_DEBUG_HELPER
		typedef BOOL (WINAPI *SymInitializeFunc)(HANDLE hProcess, PCTSTR UserSearchPath, BOOL fInvadeProcess);
		typedef BOOL (WINAPI *SymCleanupFunc)(HANDLE hProcess);
		typedef BOOL (WINAPI *SymFromAddrFunc)(HANDLE hProcess, DWORD64 Address, PDWORD64 Displacement, PSYMBOL_INFO Symbol);
		typedef DWORD (WINAPI *SymSetOptionsFunc)(DWORD SymOptions);
		typedef DWORD (WINAPI *SymLoadModuleFunc)(HANDLE hProcess, HANDLE hFile,
				PSTR ImageName, PSTR ModuleName, DWORD64 BaseOfDll, DWORD SizeOfDll);
		typedef BOOL (WINAPI *StackWalkFunc)(
			DWORD MachineType, HANDLE hProcess,
			HANDLE hThread, LPSTACKFRAME64 StackFrame, PVOID ContextRecord,
			PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
			PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
			PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
			PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress);
		typedef DWORD64 (WINAPI *SymGetModuleBaseFunc)(HANDLE hProcess, DWORD64 dwAddr);
		typedef BOOL (WINAPI *SymGetModuleInfoFunc)(HANDLE hProcess, DWORD64 dwAddr, PIMAGEHLP_MODULE64 ModuleInfo);

		static HMODULE m_hDbgHelp;
		static SymInitializeFunc m_pSymInitialize;
		static SymCleanupFunc m_pSymCleanup;
		static SymFromAddrFunc m_pSymFromAddr;
		static SymSetOptionsFunc m_pSymSetOptions;
		static SymLoadModuleFunc m_pSymLoadModule;
		static StackWalkFunc m_pStackWalk;
		//static SymGetModuleBaseFunc m_pSymGetModuleBase;
		//static SymGetModuleInfoFunc m_pSymGetModuleInfo;

		static int FormatSymbolFromAddress(
			HANDLE hProcess, DWORD64 Address,
			const MODULEENTRY32 *pModuleEntries, int NumModuleEntries,
			PSYMBOL_INFO pSymbolInfo, char *pszText);
#endif

		static ExceptionFilterMode m_ExceptionFilterMode;
		static LONG WINAPI ExceptionFilter(EXCEPTION_POINTERS *ExceptionInfo);
	};

} // namespace TVTest


#endif
