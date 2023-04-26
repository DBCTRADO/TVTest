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


#ifndef TVTEST_SHARED_MEMORY_H
#define TVTEST_SHARED_MEMORY_H


#include "Util.h"


namespace TVTest
{

	// 共有メモリクラス
	class CSharedMemory
	{
	public:
		CSharedMemory() = default;
		~CSharedMemory();

		CSharedMemory(const CSharedMemory &) = delete;
		CSharedMemory &operator=(const CSharedMemory &) = delete;

		bool Create(LPCWSTR pszName, ULONGLONG Size, bool *pfExists = nullptr);
		bool Open(LPCWSTR pszName, DWORD DesiredAccess = FILE_MAP_ALL_ACCESS, bool fInheritHandle = false);
		void Close();
		bool IsOpened() const;
		void *Map(DWORD DesiredAccess = FILE_MAP_ALL_ACCESS, ULONGLONG Offset = 0, size_t Size = 0);
		bool Unmap(void *pBuffer);
		bool Lock(DWORD Timeout = INFINITE);
		bool Unlock();
		ULONGLONG GetSize() const;

	private:
		void GetLockName(LPCWSTR pszName, String *pLockName) const;

		HANDLE m_hFileMapping = nullptr;
		CGlobalLock m_Lock;
	};

}


#endif
