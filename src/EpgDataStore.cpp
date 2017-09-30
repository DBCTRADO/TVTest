#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "EpgDataStore.h"
#include "StringUtility.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CEpgDataStore::CEpgDataStore()
	: m_OpenFlags(OpenFlag::None)
	, m_hThread(nullptr)
	, m_pEventHandler(nullptr)
	, m_UpdateCount(0)
{
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
				GetAppClass().AddLog(CLogItem::TYPE_WARNING, TEXT("EPGデータ読み込みスレッドを強制終了します。"));
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

	TVTest::String LockName;
	LockName = m_EPGDataFile.GetFileName();
	StringUtility::ToUpper(LockName);
	StringUtility::Replace(LockName, TEXT('\\'), TEXT(':'));
	LockName += TEXT(":Lock");

	CGlobalLock GlobalLock;
	if (GlobalLock.Create(LockName.c_str())) {
		if (!GlobalLock.Wait(10000)) {
			App.AddLog(CLogItem::TYPE_ERROR, TEXT("EPGファイルがロックされているため保存できません。"));
			return false;
		}
	}

	// ファイルが読み込んだ時から更新されている場合読み込み直す
	// (複数起動して他のプロセスが更新した可能性があるため)
	if (m_EPGDataFile.LoadHeader()) {
		if ((m_UpdateCount == 0) || (m_EPGDataFile.GetUpdateCount() > m_UpdateCount)) {
			TRACE(TEXT("Reload EPGData¥n"));
			m_EPGDataFile.LoadMerged();
		}
	}

	bool fOK = m_EPGDataFile.Save();

	if (fOK) {
		m_UpdateCount = m_EPGDataFile.GetUpdateCount();
	}

	GlobalLock.Release();

	return fOK;
}


void CEpgDataStore::SetEventHandler(CEventHandler *pEventHandler)
{
	m_pEventHandler = pEventHandler;
}


bool CEpgDataStore::LoadMain()
{
	if (!m_EPGDataFile.Load())
		return false;

	m_UpdateCount = m_EPGDataFile.GetUpdateCount();

	return true;
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


unsigned int __stdcall CEpgDataStore::LoadThread(void *pParameter)
{
	CEpgDataStore *pThis = static_cast<CEpgDataStore*>(pParameter);

	if (!!(pThis->m_OpenFlags & OpenFlag::LoadBackground))
		::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_LOWEST);

	if (pThis->m_pEventHandler != nullptr)
		pThis->m_pEventHandler->OnBeginLoading();

	bool fOK = pThis->LoadMain();

	if (pThis->m_pEventHandler != nullptr)
		pThis->m_pEventHandler->OnEndLoading(fOK);

	return 0;
}


}	// namespace TVTest
