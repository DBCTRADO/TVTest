#include "stdafx.h"
#include "TVTest.h"
#include "EpgDataLoader.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CEpgDataLoader::CEpgDataLoader()
	: m_EventManager(NULL)
	, m_hThread(NULL)
	, m_hAbortEvent(NULL)
	, m_pEventHandler(NULL)
{
}


CEpgDataLoader::~CEpgDataLoader()
{
	if (m_hThread!=NULL) {
		if (::WaitForSingleObject(m_hThread,0)==WAIT_TIMEOUT) {
			if (m_hAbortEvent!=NULL)
				::SetEvent(m_hAbortEvent);

			auto pCancelSynchronousIo=
				GET_MODULE_FUNCTION(TEXT("kernel32.dll"),CancelSynchronousIo);
			if (pCancelSynchronousIo!=NULL)
				pCancelSynchronousIo(m_hThread);

			if (::WaitForSingleObject(m_hThread,10000)==WAIT_TIMEOUT) {
				TRACE(TEXT("Terminate EPG data loading thread\n"));
				::TerminateThread(m_hThread,-1);
			}
		}
		::CloseHandle(m_hThread);
	}

	if (m_hAbortEvent!=NULL)
		::CloseHandle(m_hAbortEvent);
}


bool CEpgDataLoader::LoadFromFile(LPCTSTR pszFileName)
{
	if (pszFileName==NULL)
		return false;

	static const DWORD MAX_READ_SIZE=0x1000000UL;	// 16MiB
	static const DWORD BUFFER_SIZE=188*4096;
	HANDLE hFile;
	LARGE_INTEGER FileSize;
	DWORD ReadSize,RemainSize;
	BYTE *pBuffer;
	DWORD Size,Read;
	CTsPacket Packet;

	hFile=::CreateFile(pszFileName,GENERIC_READ,FILE_SHARE_READ,NULL,
					   OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return false;
	if (!::GetFileSizeEx(hFile,&FileSize) || FileSize.QuadPart<188*10) {
		::CloseHandle(hFile);
		return false;
	}
	if (FileSize.QuadPart<=(LONGLONG)MAX_READ_SIZE) {
		ReadSize=FileSize.LowPart;
	} else {
		ReadSize=MAX_READ_SIZE;
		LARGE_INTEGER Offset;
		Offset.QuadPart=(FileSize.QuadPart-MAX_READ_SIZE)/188*188;
		::SetFilePointerEx(hFile,Offset,NULL,FILE_BEGIN);
	}
	try {
		pBuffer=new BYTE[min(BUFFER_SIZE,ReadSize)];
	} catch (...) {
		::CloseHandle(hFile);
		return false;
	}
	m_EventManager.Reset();
	for (RemainSize=ReadSize;RemainSize>=188;RemainSize-=Size) {
		Size=min(RemainSize,BUFFER_SIZE);
		if (!::ReadFile(hFile,pBuffer,Size,&Read,NULL))
			break;
		BYTE *p=pBuffer,*pEnd=pBuffer+Read/188*188;
		while (p<pEnd) {
			const WORD PID=((WORD)(p[1]&0x1F)<<8) | (WORD)p[2];
			// H-EIT / TOT / M-EIT / L-EIT
			if (PID==0x0012 || PID==0x0014 /*|| PID==0x0026*/ || PID==0x0027) {
				Packet.SetData(p,188);
				if (Packet.ParsePacket()==CTsPacket::EC_VALID)
					m_EventManager.InputMedia(&Packet);
			}
			p+=188;
		}
		if (Read<Size)
			break;
	}
	delete [] pBuffer;
	::CloseHandle(hFile);
	TRACE(TEXT("EPG data loaded %s\n"),pszFileName);
	return true;
}


bool CEpgDataLoader::Load(LPCTSTR pszFolder,HANDLE hAbortEvent)
{
	if (pszFolder==NULL)
		return false;

	FILETIME ftCurrent;
	TCHAR szFileMask[MAX_PATH];
	HANDLE hFind;
	WIN32_FIND_DATA fd;

	::GetSystemTimeAsFileTime(&ftCurrent);
	::PathCombine(szFileMask,pszFolder,TEXT("*_epg.dat"));
	hFind=::FindFirstFile(szFileMask,&fd);
	if (hFind==INVALID_HANDLE_VALUE)
		return false;
	do {
		if ((fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)==0
				&& ftCurrent-fd.ftLastWriteTime<FILETIME_HOUR*24*14
				&& ::lstrlen(pszFolder)+1+::lstrlen(fd.cFileName)<MAX_PATH) {
			TCHAR szFilePath[MAX_PATH];

			::PathCombine(szFilePath,pszFolder,fd.cFileName);
			LoadFromFile(szFilePath);
			if (hAbortEvent!=NULL
					&& ::WaitForSingleObject(hAbortEvent,0)==WAIT_OBJECT_0)
				break;
		}
	} while (::FindNextFile(hFind,&fd));
	::FindClose(hFind);
	return true;
}


bool CEpgDataLoader::LoadAsync(LPCTSTR pszFolder,CEventHandler *pEventHandler)
{
	if (IsStringEmpty(pszFolder))
		return false;

	if (m_hThread!=NULL) {
		if (::WaitForSingleObject(m_hThread,0)==WAIT_TIMEOUT)
			return false;
		::CloseHandle(m_hThread);
		m_hThread=NULL;
	}

	m_Folder.Set(pszFolder);
	m_pEventHandler=pEventHandler;
	if (m_hAbortEvent==NULL)
		m_hAbortEvent=::CreateEvent(NULL,FALSE,FALSE,NULL);
	else
		::ResetEvent(m_hAbortEvent);
	m_hThread=(HANDLE)::_beginthreadex(NULL,0,LoadThread,this,0,NULL);
	if (m_hThread==NULL)
		return false;

	return true;
}


bool CEpgDataLoader::IsLoading() const
{
	return m_hThread!=NULL
		&& ::WaitForSingleObject(m_hThread,0)==WAIT_TIMEOUT;
}


bool CEpgDataLoader::Abort(DWORD Timeout)
{
	if (m_hThread==NULL)
		return true;
	if (m_hAbortEvent!=NULL)
		::SetEvent(m_hAbortEvent);
	if (::WaitForSingleObject(m_hThread,Timeout)==WAIT_TIMEOUT)
		return false;
	return true;
}


bool CEpgDataLoader::Wait(DWORD Timeout)
{
	if (m_hThread!=NULL) {
		if (::WaitForSingleObject(m_hThread,Timeout)==WAIT_TIMEOUT)
			return false;
	}
	return true;
}


unsigned int __stdcall CEpgDataLoader::LoadThread(void *pParameter)
{
	CEpgDataLoader *pThis=static_cast<CEpgDataLoader*>(pParameter);

	if (pThis->m_pEventHandler!=NULL)
		pThis->m_pEventHandler->OnStart();
	::SetThreadPriority(::GetCurrentThread(),THREAD_PRIORITY_LOWEST);
	bool fSuccess=pThis->Load(pThis->m_Folder.Get(),pThis->m_hAbortEvent);
	if (pThis->m_pEventHandler!=NULL)
		pThis->m_pEventHandler->OnEnd(fSuccess,&pThis->m_EventManager);
	pThis->m_EventManager.Clear();
	pThis->m_Folder.Clear();
	return 0;
}




CEpgDataLoader::CEventHandler::~CEventHandler()
{
}
