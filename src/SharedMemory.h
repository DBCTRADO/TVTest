#ifndef TVTEST_SHARED_MEMORY_H
#define TVTEST_SHARED_MEMORY_H


#include "Util.h"


namespace TVTest
{

	// ã§óLÉÅÉÇÉäÉNÉâÉX
	class CSharedMemory
	{
	public:
		CSharedMemory();
		~CSharedMemory();
		bool Create(LPCWSTR pszName,ULONGLONG Size,bool *pfExists=nullptr);
		bool Open(LPCWSTR pszName,DWORD DesiredAccess=FILE_MAP_ALL_ACCESS,bool fInheritHandle=false);
		void Close();
		bool IsOpened() const;
		void *Map(DWORD DesiredAccess=FILE_MAP_ALL_ACCESS,ULONGLONG Offset=0,size_t Size=0);
		bool Unmap(void *pBuffer);
		bool Lock(DWORD Timeout=INFINITE);
		bool Unlock();
		ULONGLONG GetSize() const;

	private:
		void GetLockName(LPCWSTR pszName,String *pLockName) const;

		CSharedMemory(const CSharedMemory &) /* = delete */;
		CSharedMemory &operator=(const CSharedMemory &) /* = delete */;

		HANDLE m_hFileMapping;
		CGlobalLock m_Lock;
	};

}


#endif
