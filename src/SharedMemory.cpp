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


#include "stdafx.h"
#include "TVTest.h"
#include "SharedMemory.h"
#include "Common/DebugDef.h"

#ifndef _NTDEF_
typedef LONG NTSTATUS;
#endif

#include <pshpack1.h>

typedef enum _SECTION_INFORMATION_CLASS {
	SectionBasicInformation,
	SectionImageInformation
} SECTION_INFORMATION_CLASS;

typedef struct _SECTION_BASIC_INFORMATION {
	PVOID Base;
	ULONG Attributes;
	LARGE_INTEGER Size;
} SECTION_BASIC_INFORMATION;

NTSTATUS NTAPI NtQuerySection(
	HANDLE SectionHandle,
	SECTION_INFORMATION_CLASS InformationClass,
	PVOID InformationBuffer,
	ULONG InformationBufferSize,
	PULONG ResultLength);

#include <poppack.h>




namespace TVTest
{


CSharedMemory::~CSharedMemory()
{
	Close();
}


bool CSharedMemory::Create(LPCWSTR pszName, ULONGLONG Size, bool *pfExists)
{
	if (IsStringEmpty(pszName) || Size == 0)
		return false;
	if (m_hFileMapping != nullptr)
		return false;

	String LockName;
	GetLockName(pszName, &LockName);
	if (!m_Lock.Create(LockName.c_str()))
		return false;

	CBasicSecurityAttributes SecurityAttributes;

	if (!SecurityAttributes.Initialize()) {
		m_Lock.Close();
		return false;
	}

	if (!Lock()) {
		m_Lock.Close();
		return false;
	}

	m_hFileMapping = ::CreateFileMapping(
		INVALID_HANDLE_VALUE, &SecurityAttributes, PAGE_READWRITE,
		static_cast<DWORD>(Size >> 32), static_cast<DWORD>(Size & 0xFFFFFFFFULL),
		pszName);
	if (m_hFileMapping == nullptr) {
		TRACE(TEXT("CreateFileMapping \"{}\" Error {:x}\n"), pszName, ::GetLastError());
		m_Lock.Close();
		return false;
	}

	TRACE(TEXT("File mapping created \"{}\"\n"), pszName);

	if (pfExists != nullptr)
		*pfExists = ::GetLastError() == ERROR_ALREADY_EXISTS;

	return true;
}


bool CSharedMemory::Open(LPCWSTR pszName, DWORD DesiredAccess, bool fInheritHandle)
{
	if (IsStringEmpty(pszName))
		return false;
	if (m_hFileMapping != nullptr)
		return false;

	String LockName;
	GetLockName(pszName, &LockName);
	if (!m_Lock.Open(LockName.c_str()))
		return false;

	CBasicSecurityAttributes SecurityAttributes;

	if (!SecurityAttributes.Initialize())
		return false;

	m_hFileMapping = ::OpenFileMapping(DesiredAccess, fInheritHandle, pszName);
	if (m_hFileMapping == nullptr) {
		TRACE(TEXT("OpenFileMapping \"{}\" Error {:x}\n"), pszName, ::GetLastError());
		m_Lock.Close();
		return false;
	}

	TRACE(TEXT("File mapping opened \"{}\"\n"), pszName);

	return true;
}


void CSharedMemory::Close()
{
	if (m_hFileMapping != nullptr) {
		::CloseHandle(m_hFileMapping);
		m_hFileMapping = nullptr;
	}

	m_Lock.Close();
}


bool CSharedMemory::IsOpened() const
{
	return m_hFileMapping != nullptr;
}


void *CSharedMemory::Map(DWORD DesiredAccess, ULONGLONG Offset, size_t Size)
{
	if (m_hFileMapping == nullptr)
		return nullptr;

	return ::MapViewOfFile(
		m_hFileMapping, DesiredAccess,
		static_cast<DWORD>(Offset >> 32), static_cast<DWORD>(Offset & 0xFFFFFFFFULL),
		Size);
}


bool CSharedMemory::Unmap(void *pBuffer)
{
	if (m_hFileMapping == nullptr)
		return false;

	return ::UnmapViewOfFile(pBuffer) != FALSE;
}


bool CSharedMemory::Lock(DWORD Timeout)
{
	return m_Lock.Wait(Timeout);
}


bool CSharedMemory::Unlock()
{
	m_Lock.Release();
	return true;
}


ULONGLONG CSharedMemory::GetSize() const
{
	if (m_hFileMapping == nullptr)
		return 0;

	ULONGLONG Size = 0;
	const HMODULE hLib = Util::LoadSystemLibrary(TEXT("ntdll.dll"));

	if (hLib != nullptr) {
		const auto pNtQuerySection = GET_LIBRARY_FUNCTION(hLib, NtQuerySection);

		if (pNtQuerySection != nullptr) {
			SECTION_BASIC_INFORMATION Info;

			if (pNtQuerySection(m_hFileMapping, SectionBasicInformation, &Info, sizeof(Info), 0) == 0)
				Size = Info.Size.QuadPart;
		}

		::FreeLibrary(hLib);
	}

	return Size;
}


void CSharedMemory::GetLockName(LPCWSTR pszName, String *pLockName) const
{
	pLockName->assign(pszName);
	pLockName->append(L"_Lock");
}


}	// namespace TVTest
