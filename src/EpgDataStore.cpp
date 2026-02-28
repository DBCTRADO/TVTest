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
#include "AppMain.h"
#include "EpgDataStore.h"
#include "StringUtility.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CEpgDataStore::CEpgDataStore()
{
	m_EPGDataFile.SetSourceID(1);
}


CEpgDataStore::~CEpgDataStore()
{
	Close();
}


bool CEpgDataStore::Open(LibISDB::EPGDatabase *pEPGDatabase, LPCTSTR pszFileName, OpenFlag Flags)
{
	Close();

	LibISDB::EPGDataFile::OpenFlag FileOpenFlags =
		LibISDB::EPGDataFile::OpenFlag::Read |
		LibISDB::EPGDataFile::OpenFlag::Write |
		LibISDB::EPGDataFile::OpenFlag::ShareRead |
		LibISDB::EPGDataFile::OpenFlag::DiscardOld;
	if (!!(Flags & OpenFlag::LoadBackground))
		FileOpenFlags |= LibISDB::EPGDataFile::OpenFlag::PriorityIdle;

	if (!m_EPGDataFile.Open(pEPGDatabase, pszFileName, FileOpenFlags))
		return false;

	m_OpenFlags = Flags;

	m_EPGDataFile.SetLogger(&GetAppClass().UICore);

	return true;
}


void CEpgDataStore::Close()
{
	if (m_hThread != nullptr) {
		if (::WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT) {
			GetAppClass().AddLog(TEXT("EPGデータ読み込みスレッドの終了を待っています..."));
			::CancelSynchronousIo(m_hThread);
			if (::WaitForSingleObject(m_hThread, 10000) == WAIT_TIMEOUT) {
				GetAppClass().AddLog(CLogItem::LogType::Warning, TEXT("EPGデータ読み込みスレッドを強制終了します。"));
				::TerminateThread(m_hThread, -1);
			}
		}

		::CloseHandle(m_hThread);
		m_hThread = nullptr;
	}

	m_EPGDataFile.Close();
}


bool CEpgDataStore::IsOpen() const
{
	return m_EPGDataFile.IsOpen();
}


bool CEpgDataStore::Load()
{
	if (!WaitThread(0))
		return false;
	if (!IsOpen())
		return false;

	return LoadMain();
}


bool CEpgDataStore::LoadAsync()
{
	if (!WaitThread(0))
		return false;
	if (!IsOpen())
		return false;

	m_hThread = reinterpret_cast<HANDLE>(::_beginthreadex(nullptr, 0, LoadThread, this, 0, nullptr));
	if (m_hThread == nullptr)
		return false;

	return true;
}


bool CEpgDataStore::IsLoading() const
{
	return (m_hThread != nullptr)
		&& (::WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT);
}


bool CEpgDataStore::Wait(DWORD Timeout)
{
	if (m_hThread != nullptr) {
		if (::WaitForSingleObject(m_hThread, Timeout) == WAIT_TIMEOUT)
			return false;
	}
	return true;
}


bool CEpgDataStore::Save()
{
	if (!WaitThread(0))
		return false;
	if (!IsOpen())
		return false;

	CAppMain &App = GetAppClass();

	CInterprocessReadWriteLock Lock;
	bool fLocked = false;
	if (CreateLock(&Lock)) {
		if (!Lock.LockWrite(m_LockTimeout)) {
			App.AddLog(CLogItem::LogType::Error, TEXT("EPGファイルがロックされているため保存できません。"));
			return false;
		}
		fLocked = true;
	}

	// ファイルが読み込んだ時から更新されている場合読み込み直す
	// (複数起動して他のプロセスが更新した可能性があるため)
	if (m_EPGDataFile.LoadHeader()) {
		if ((m_UpdateCount == 0) || (m_EPGDataFile.GetUpdateCount() > m_UpdateCount)) {
			TRACE(TEXT("Reload EPGData¥n"));
			m_EPGDataFile.LoadMerged();
		}
	}

	const bool fOK = m_EPGDataFile.Save();

	if (fOK) {
		m_UpdateCount = m_EPGDataFile.GetUpdateCount();
	}

	if (fLocked)
		Lock.UnlockWrite();

	return fOK;
}


void CEpgDataStore::SetEventHandler(CEventHandler *pEventHandler)
{
	m_pEventHandler = pEventHandler;
}


bool CEpgDataStore::LoadMain()
{
	CInterprocessReadWriteLock Lock;
	bool fLocked = false;
	if (CreateLock(&Lock)) {
		if (!Lock.LockRead(m_LockTimeout)) {
			GetAppClass().AddLog(CLogItem::LogType::Error, TEXT("EPGファイルがロックされているため読み込めません。"));
			return false;
		}
		fLocked = true;
	}

	if (m_pEventHandler != nullptr)
		m_pEventHandler->OnBeginLoading();

	const bool fOK = m_EPGDataFile.Load();

	if (fOK)
		m_UpdateCount = m_EPGDataFile.GetUpdateCount();

	if (fLocked)
		Lock.UnlockRead();

	if (m_pEventHandler != nullptr)
		m_pEventHandler->OnEndLoading(fOK);

	return fOK;
}


bool CEpgDataStore::WaitThread(DWORD Timeout)
{
	if (m_hThread != nullptr) {
		if (::WaitForSingleObject(m_hThread, Timeout) == WAIT_TIMEOUT)
			return false;
		::CloseHandle(m_hThread);
		m_hThread = nullptr;
	}

	return true;
}


bool CEpgDataStore::CreateLock(CInterprocessReadWriteLock *pLock) const
{
	String LockName = m_EPGDataFile.GetFileName();
	StringUtility::ToUpper(LockName);
	StringUtility::Replace(LockName, TEXT('\\'), TEXT(':'));
	String SemaphoreName = LockName;
	LockName += TEXT(":Lock");
	SemaphoreName += TEXT(":Semaphore");

	return pLock->Create(LockName.c_str(), SemaphoreName.c_str(), 10);
}


unsigned int __stdcall CEpgDataStore::LoadThread(void *pParameter)
{
	CEpgDataStore *pThis = static_cast<CEpgDataStore*>(pParameter);

	if (!!(pThis->m_OpenFlags & OpenFlag::LoadBackground))
		::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_LOWEST);

	pThis->LoadMain();

	return 0;
}


} // namespace TVTest
