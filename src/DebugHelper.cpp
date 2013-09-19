#include "stdafx.h"
#include "TVTest.h"
#include "DebugHelper.h"


#define MAX_MODULE_ENTRIES 128


#ifdef ENABLE_DEBUG_HELPER
HMODULE CDebugHelper::m_hDbgHelp=NULL;
CDebugHelper::SymInitializeFunc CDebugHelper::m_pSymInitialize=NULL;
CDebugHelper::SymCleanupFunc CDebugHelper::m_pSymCleanup=NULL;
CDebugHelper::SymFromAddrFunc CDebugHelper::m_pSymFromAddr=NULL;
CDebugHelper::SymSetOptionsFunc CDebugHelper::m_pSymSetOptions=NULL;
CDebugHelper::SymLoadModuleFunc CDebugHelper::m_pSymLoadModule=NULL;
CDebugHelper::StackWalkFunc CDebugHelper::m_pStackWalk=NULL;
//CDebugHelper::SymGetModuleInfoFunc CDebugHelper::m_pSymGetModuleInfo=NULL;
//CDebugHelper::SymGetModuleBaseFunc CDebugHelper::m_pSymGetModuleBase=NULL;
#endif
CDebugHelper::ExceptionFilterMode CDebugHelper::m_ExceptionFilterMode=EXCEPTION_FILTER_DEFAULT;


CDebugHelper::CDebugHelper()
{
#ifdef ENABLE_DEBUG_HELPER
	::SetUnhandledExceptionFilter(ExceptionFilter);
#endif
}


CDebugHelper::~CDebugHelper()
{
#ifdef ENABLE_DEBUG_HELPER
	if (m_hDbgHelp!=NULL) {
		::FreeLibrary(m_hDbgHelp);
		m_hDbgHelp=NULL;
	}
#endif
}


bool CDebugHelper::Initialize()
{
#ifdef ENABLE_DEBUG_HELPER
	if (m_hDbgHelp==NULL) {
		m_hDbgHelp=::LoadLibrary(TEXT("dbghelp.dll"));
		if (m_hDbgHelp==NULL)
			return false;

		m_pSymInitialize=reinterpret_cast<SymInitializeFunc>(::GetProcAddress(m_hDbgHelp,"SymInitialize"));
		m_pSymCleanup=reinterpret_cast<SymCleanupFunc>(::GetProcAddress(m_hDbgHelp,"SymCleanup"));
		m_pSymFromAddr=reinterpret_cast<SymFromAddrFunc>(::GetProcAddress(m_hDbgHelp,"SymFromAddr"));
		m_pSymSetOptions=reinterpret_cast<SymSetOptionsFunc>(::GetProcAddress(m_hDbgHelp,"SymSetOptions"));
		m_pSymLoadModule=reinterpret_cast<SymLoadModuleFunc>(::GetProcAddress(m_hDbgHelp,"SymLoadModule64"));
		m_pStackWalk=reinterpret_cast<StackWalkFunc>(::GetProcAddress(m_hDbgHelp,"StackWalk64"));
		//m_pSymGetModuleBase=reinterpret_cast<SymGetModuleBaseFunc>(::GetProcAddress(m_hDbgHelp,"SymGetModuleBase64"));
		//m_pSymGetModuleInfo=reinterpret_cast<SymGetModuleInfoFunc>(::GetProcAddress(m_hDbgHelp,"SymGetModuleInfo64"));
		if (m_pSymInitialize==NULL || m_pSymCleanup==NULL || m_pSymFromAddr==NULL
				|| m_pSymSetOptions==NULL || m_pStackWalk==NULL) {
			::FreeLibrary(m_hDbgHelp);
			m_hDbgHelp=NULL;
			return false;
		}
	}
#endif	// ENABLE_DEBUG_HELPER
	return true;
}


bool CDebugHelper::SetExceptionFilterMode(ExceptionFilterMode Mode)
{
	m_ExceptionFilterMode=Mode;
	return true;
}


LONG WINAPI CDebugHelper::ExceptionFilter(EXCEPTION_POINTERS *ExceptionInfo)
{
	if (m_ExceptionFilterMode==EXCEPTION_FILTER_NONE)
		return EXCEPTION_EXECUTE_HANDLER;

#ifdef ENABLE_DEBUG_HELPER
	if (m_hDbgHelp!=NULL) {
		static struct {
			HANDLE hProcess;
			HANDLE hThread;
			int i,j;
			char szText[16*1024];
			int Length;
			MODULEENTRY32 ModuleEntries[MAX_MODULE_ENTRIES];
			int NumModuleEntries;
			HANDLE hSnap;
			union {
				LPCSTR pszExceptionCode;
				MODULEENTRY32 ModuleEntry;
				struct {
					STACKFRAME64 StackFrame;
					CONTEXT Context;
					BYTE SymbolInfoBuffer[sizeof(SYMBOL_INFO)+256];
					PSYMBOL_INFO pSymbol;
				} Stack;
				struct {
					TCHAR szFileName[MAX_PATH];
					HANDLE hFile;
					DWORD WroteSize;
				} File;
			};
			bool fContinueExecution;
		} s;

		m_pSymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);

		s.hProcess=::GetCurrentProcess();
		if (!m_pSymInitialize(s.hProcess,NULL,FALSE))
			return EXCEPTION_CONTINUE_SEARCH;
		s.hThread=::GetCurrentThread();

		static const struct {
			DWORD Code;
			LPCSTR pszText;
		} ExceptionCodeList[] = {
			{EXCEPTION_ACCESS_VIOLATION,		"Access violation"},
			{EXCEPTION_STACK_OVERFLOW,			"Stack overflow"},
			{EXCEPTION_IN_PAGE_ERROR,			"Page error"},
			{EXCEPTION_ILLEGAL_INSTRUCTION,		"Illegal instruction"},
			{EXCEPTION_DATATYPE_MISALIGNMENT,	"Misalignment"},
			{EXCEPTION_INT_DIVIDE_BY_ZERO,		"Divide by zero"},
			{EXCEPTION_FLT_DIVIDE_BY_ZERO,		"Floating-point divide by zero"},
		};
		s.pszExceptionCode="Other";
		for (int i=0;i<lengthof(ExceptionCodeList);i++) {
			if (ExceptionCodeList[i].Code==ExceptionInfo->ExceptionRecord->ExceptionCode) {
				s.pszExceptionCode=ExceptionCodeList[i].pszText;
				break;
			}
		}

		s.Length=::wsprintfA(s.szText,
							 APP_NAME_A "で例外が発生しました。\r\n\r\n"
							 "Code %08x (%s) / Address %p\r\n"
#if defined(_M_IX86)
							 "EAX %08x / EBX %08x / ECX %08x / EDX %08x\r\n"
							 "ESI %08x / EDI %08x / EBP %08x / ESP %08x / EIP %08x\r\n"
#elif defined(_M_AMD64)
							 "RAX %p / RBX %p / RCX %p / RDX %p\r\n"
							 "RSI %p / RDI %p / RBP %p / RSP %p / RIP %p\r\n"
#endif
							 "\r\n",
							 ExceptionInfo->ExceptionRecord->ExceptionCode,
							 s.pszExceptionCode,
							 (void*)ExceptionInfo->ExceptionRecord->ExceptionAddress
#if defined(_M_IX86)
							 ,ExceptionInfo->ContextRecord->Eax,
							 ExceptionInfo->ContextRecord->Ebx,
							 ExceptionInfo->ContextRecord->Ecx,
							 ExceptionInfo->ContextRecord->Edx,
							 ExceptionInfo->ContextRecord->Esi,
							 ExceptionInfo->ContextRecord->Edi,
							 ExceptionInfo->ContextRecord->Ebp,
							 ExceptionInfo->ContextRecord->Esp,
							 ExceptionInfo->ContextRecord->Eip
#elif defined(_M_AMD64)
							 ,(void*)ExceptionInfo->ContextRecord->Rax,
							 (void*)ExceptionInfo->ContextRecord->Rbx,
							 (void*)ExceptionInfo->ContextRecord->Rcx,
							 (void*)ExceptionInfo->ContextRecord->Rdx,
							 (void*)ExceptionInfo->ContextRecord->Rsi,
							 (void*)ExceptionInfo->ContextRecord->Rdi,
							 (void*)ExceptionInfo->ContextRecord->Rbp,
							 (void*)ExceptionInfo->ContextRecord->Rsp,
							 (void*)ExceptionInfo->ContextRecord->Rip
#endif
							 );

		// モジュールの列挙
		s.NumModuleEntries=0;
		if (m_pSymLoadModule!=NULL) {
			s.hSnap=::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,::GetCurrentProcessId());
			if (s.hSnap!=INVALID_HANDLE_VALUE) {
				s.ModuleEntry.dwSize=sizeof(s.ModuleEntry);
				if (::Module32First(s.hSnap,&s.ModuleEntry)) {
					do {
						if ((m_pSymLoadModule(s.hProcess,NULL,
											  s.ModuleEntry.szExePath,
											  s.ModuleEntry.szModule,
											  reinterpret_cast<DWORD64>(s.ModuleEntry.modBaseAddr),
											  s.ModuleEntry.modBaseSize)!=0
									|| ::GetLastError()==ERROR_SUCCESS)
								&& s.NumModuleEntries<MAX_MODULE_ENTRIES) {
							::CopyMemory(&s.ModuleEntries[s.NumModuleEntries],
										 &s.ModuleEntry,sizeof(MODULEENTRY32));
							s.NumModuleEntries++;
						}
					} while (::Module32Next(s.hSnap,&s.ModuleEntry));
				}
				::CloseHandle(s.hSnap);
			}
		}

		::ZeroMemory(&s.Stack.StackFrame,sizeof(s.Stack.StackFrame));
#if defined(_M_IX86)
		s.Stack.StackFrame.AddrPC.Offset=ExceptionInfo->ContextRecord->Eip;
		s.Stack.StackFrame.AddrPC.Mode=AddrModeFlat;
		s.Stack.StackFrame.AddrFrame.Offset=ExceptionInfo->ContextRecord->Ebp;
		s.Stack.StackFrame.AddrFrame.Mode=AddrModeFlat;
		s.Stack.StackFrame.AddrStack.Offset=ExceptionInfo->ContextRecord->Esp;
		s.Stack.StackFrame.AddrStack.Mode=AddrModeFlat;
#elif defined(_M_AMD64)
		s.Stack.StackFrame.AddrPC.Offset=ExceptionInfo->ContextRecord->Rip;
		s.Stack.StackFrame.AddrPC.Mode=AddrModeFlat;
		s.Stack.StackFrame.AddrFrame.Offset=ExceptionInfo->ContextRecord->Rbp;
		s.Stack.StackFrame.AddrFrame.Mode=AddrModeFlat;
		s.Stack.StackFrame.AddrStack.Offset=ExceptionInfo->ContextRecord->Rsp;
		s.Stack.StackFrame.AddrStack.Mode=AddrModeFlat;
#elif defined(_M_IA64)
		s.Stack.StackFrame.AddrPC.Offset=ExceptionInfo->ContextRecord->StIIP;
		s.Stack.StackFrame.AddrPC.Mode=AddrModeFlat;
		s.Stack.StackFrame.AddrStack.Offset=ExceptionInfo->ContextRecord->IntSp;
		s.Stack.StackFrame.AddrStack.Mode=AddrModeFlat;
		s.Stack.StackFrame.AddrBStore.Offset=ExceptionInfo->ContextRecord->RsBSP;
		s.Stack.StackFrame.AddrBStore.Mode=AddrModeFlat;
#else
#error Unsupported processor.
#endif

		s.Stack.Context=*ExceptionInfo->ContextRecord;

		s.Stack.pSymbol=(PSYMBOL_INFO)s.Stack.SymbolInfoBuffer;
		s.Stack.pSymbol->SizeOfStruct=sizeof(SYMBOL_INFO);
		s.Stack.pSymbol->MaxNameLen=sizeof(s.Stack.SymbolInfoBuffer)-sizeof(SYMBOL_INFO);

		s.szText[s.Length++]='>';
		s.Length+=FormatSymbolFromAddress(s.hProcess,
										  (DWORD64)ExceptionInfo->ExceptionRecord->ExceptionAddress,
										  s.ModuleEntries,s.NumModuleEntries,
										  s.Stack.pSymbol,
										  s.szText+s.Length);

		// スタックトレース
		for (s.i=0;s.i<10;s.i++) {
			if (!m_pStackWalk(
#if defined(_M_IX86)
								IMAGE_FILE_MACHINE_I386,
#elif defined(_M_AMD64)
								IMAGE_FILE_MACHINE_AMD64,
#elif defined(_M_IA64)
								IMAGE_FILE_MACHINE_IA64,
#else
#error Unsupported processor.
#endif
								s.hProcess,s.hThread,
								&s.Stack.StackFrame,
								&s.Stack.Context,
								NULL,NULL,NULL,NULL)
					|| s.Stack.StackFrame.AddrPC.Offset==0
					|| s.Stack.StackFrame.AddrReturn.Offset==0
					|| s.Stack.StackFrame.AddrPC.Offset==s.Stack.StackFrame.AddrReturn.Offset)
				break;

			s.Length+=FormatSymbolFromAddress(s.hProcess,
											  s.Stack.StackFrame.AddrReturn.Offset,
											  s.ModuleEntries,s.NumModuleEntries,
											  s.Stack.pSymbol,
											  s.szText+s.Length);
		}

		m_pSymCleanup(s.hProcess);

		// メッセージ表示
		s.fContinueExecution=false;
		if (m_ExceptionFilterMode==EXCEPTION_FILTER_DIALOG) {
			if (ExceptionInfo->ExceptionRecord->ExceptionFlags==EXCEPTION_NONCONTINUABLE) {
				::MessageBoxA(NULL,s.szText,NULL,MB_OK | MB_ICONSTOP);
			} else {
				::lstrcpyA(s.szText+s.Length,
						   "\r\n再試行しますか？\r\n"
						   "[いいえ] を選択するとプログラムが終了します。");
				if (::MessageBoxA(NULL,s.szText,NULL,MB_YESNO | MB_ICONSTOP | MB_DEFBUTTON2)==IDYES)
					s.fContinueExecution=true;
			}
		}

		// ログ保存
		::GetModuleFileName(NULL,s.File.szFileName,lengthof(s.File.szFileName));
		::lstrcat(s.File.szFileName,TEXT(".exception.log"));
		s.File.hFile=::CreateFile(s.File.szFileName,GENERIC_WRITE,0,NULL,
								  CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		if (s.File.hFile!=INVALID_HANDLE_VALUE) {
			static const char szHeader[]=APP_NAME_A " ver." VERSION_TEXT_A
#if defined(_M_IX86)
				" (x86)"
#elif defined(_M_AMD64)
				" (x64)"
#elif defined(_M_IA64)
				" (IA64)"
#endif
				"\r\n\r\n";
			::WriteFile(s.File.hFile,szHeader,(DWORD)(sizeof(szHeader)-1),&s.File.WroteSize,NULL);
			::WriteFile(s.File.hFile,s.szText,s.Length,&s.File.WroteSize,NULL);
			if (s.NumModuleEntries>0) {
				s.Length=::wsprintfA(s.szText,"\r\nModules (%d Modules)\r\n",s.NumModuleEntries);
				for (s.i=0;s.i<s.NumModuleEntries;s.i++) {
					s.Length+=::wsprintfA(s.szText+s.Length,"%s %p + %08x\r\n",
										  s.ModuleEntries[s.i].szModule,
										  s.ModuleEntries[s.i].modBaseAddr,
										  s.ModuleEntries[s.i].modBaseSize);
				}
				::WriteFile(s.File.hFile,s.szText,s.Length,&s.File.WroteSize,NULL);
			}
			::CloseHandle(s.File.hFile);
		}

		if (s.fContinueExecution)
			return EXCEPTION_CONTINUE_EXECUTION;

		//return EXCEPTION_EXECUTE_HANDLER;
	}
#endif	// ENABLE_DEBUG_HELPER

	return EXCEPTION_CONTINUE_SEARCH;
}


#ifdef ENABLE_DEBUG_HELPER

int CDebugHelper::FormatSymbolFromAddress(HANDLE hProcess,DWORD64 Address,
	const MODULEENTRY32 *pModuleEntries,int NumModuleEntries,
	PSYMBOL_INFO pSymbolInfo,char *pszText)
{
	const char *pszModule;

	pszModule="\?\?\?";
#if 0
	if (m_pSymGetModuleBase!=NULL && m_pSymGetModuleInfo!=NULL) {
		DWORD64 ModuleBase=m_pSymGetModuleBase(s.hProcess,s.Stack.StackFrame.AddrReturn.Offset);
		if (ModuleBase!=0) {
			IMAGEHLP_MODULE64 ModuleInfo;

			ModuleInfo.SizeOfStruct=CCSIZEOF_STRUCT(IMAGEHLP_MODULE64,LoadedImageName);
			if (m_pSymGetModuleInfo(hProcess,ModuleBase,&ModuleInfo)) {
				pszModule=::PathFindFileNameA(ModuleInfo.ImageName);
			}
		}
	}
#else
	int i;
	for (i=0;i<NumModuleEntries;i++) {
		if (Address>=(DWORD64)pModuleEntries[i].modBaseAddr
				&& Address<(DWORD64)pModuleEntries[i].modBaseAddr+pModuleEntries[i].modBaseSize) {
			pszModule=pModuleEntries[i].szModule;
			break;
		}
	}
#endif

	DWORD64 Displacement;
	int Length;
	if (m_pSymFromAddr(hProcess,Address,&Displacement,pSymbolInfo)) {
		Length=::wsprintfA(pszText,"%p %s : %s + %08x\r\n",
						   (void*)Address,pszModule,
						   pSymbolInfo->Name,(DWORD)Displacement);
	} else if (i<NumModuleEntries) {
		Length=::wsprintfA(pszText,"%p %s + %08x\r\n",
						   (void*)Address,pszModule,
						   (DWORD)(Address-(DWORD64)pModuleEntries[i].modBaseAddr));
	} else {
		Length=::wsprintfA(pszText,"%p %s\r\n",
						   (void*)Address,pszModule);
	}
	return Length;
}

#endif	// ENABLE_DEBUG_HELPER
