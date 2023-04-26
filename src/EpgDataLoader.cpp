/*
  TVTest
  Copyright(c) 2008-2022 DBCTRADO

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
#include "EpgDataLoader.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CEpgDataLoader::CEpgDataLoader()
{
	m_EPGDatabaseFilter.SetEPGDatabase(&m_EPGDatabase);
	m_EPGDatabaseFilter.SetSourceID(2);
}


CEpgDataLoader::~CEpgDataLoader()
{
	if (m_hThread != nullptr) {
		if (::WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT) {
			if (m_hAbortEvent != nullptr)
				::SetEvent(m_hAbortEvent);

			::CancelSynchronousIo(m_hThread);

			if (::WaitForSingleObject(m_hThread, 10000) == WAIT_TIMEOUT) {
				TRACE(TEXT("Terminate EPG data loading thread\n"));
				::TerminateThread(m_hThread, -1);
			}
		}
		::CloseHandle(m_hThread);
	}

	if (m_hAbortEvent != nullptr)
		::CloseHandle(m_hAbortEvent);
}


bool CEpgDataLoader::LoadFromFile(LPCTSTR pszFileName)
{
	if (pszFileName == nullptr)
		return false;

	static constexpr DWORD MAX_READ_SIZE = 0x1000000UL;	// 16MiB
	static constexpr DWORD BUFFER_SIZE = 188 * 4096;
	LARGE_INTEGER FileSize;
	DWORD ReadSize, RemainSize;
	std::unique_ptr<BYTE[]> Buffer;
	DWORD Size, Read;
	LibISDB::TSPacket Packet;

	const HANDLE hFile = ::CreateFile(
		pszFileName, GENERIC_READ, FILE_SHARE_READ, nullptr,
		OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;
	if (!::GetFileSizeEx(hFile, &FileSize) || FileSize.QuadPart < 188 * 10) {
		::CloseHandle(hFile);
		return false;
	}
	if (FileSize.QuadPart <= static_cast<LONGLONG>(MAX_READ_SIZE)) {
		ReadSize = FileSize.LowPart;
	} else {
		ReadSize = MAX_READ_SIZE;
		LARGE_INTEGER Offset;
		Offset.QuadPart = (FileSize.QuadPart - MAX_READ_SIZE) / 188 * 188;
		::SetFilePointerEx(hFile, Offset, nullptr, FILE_BEGIN);
	}
	try {
		Buffer.reset(new BYTE[std::min(BUFFER_SIZE, ReadSize)]);
	} catch (...) {
		::CloseHandle(hFile);
		return false;
	}

	m_EPGDatabaseFilter.Reset();
	m_EPGDatabase.SetNoPastEvents(false);

	for (RemainSize = ReadSize; RemainSize >= 188; RemainSize -= Size) {
		Size = std::min(RemainSize, BUFFER_SIZE);
		if (!::ReadFile(hFile, Buffer.get(), Size, &Read, nullptr))
			break;

		const BYTE *pEnd = Buffer.get() + Read / 188 * 188;

		// 最初に TOT を渡すようにする
		if (RemainSize == ReadSize) {
			// TOT 取得フィルタ
			class TOTFilter
				: public LibISDB::SingleIOFilter
			{
			public:
				TOTFilter()
				{
					Reset();
				}

			// ObjectBase
				const LibISDB::CharType * GetObjectName() const noexcept override { return LIBISDB_STR("TOTFilter"); }

			// FilterBase
				void Reset() override
				{
					BlockLock Lock(m_FilterLock);

					m_PIDMapManager.UnmapAllTargets();
					m_PIDMapManager.MapTarget(LibISDB::PID_TOT, LibISDB::PSITableBase::CreateWithHandler<LibISDB::TOTTable>(&TOTFilter::OnTOTSection, this));
				}

			// SingleIOFilter
				bool ProcessData(LibISDB::DataStream *pData) override
				{
					if (pData->Is<LibISDB::TSPacket>())
						m_PIDMapManager.StorePacketStream(pData);
					return true;
				}

			// TOTFilter
				const LibISDB::DateTime & GetTOTTime() const noexcept { return m_DateTime; }

			private:
				void OnTOTSection(const LibISDB::PSITableBase *pTable, const LibISDB::PSISection *pSection)
				{
					const LibISDB::TOTTable *pTOTTable = dynamic_cast<const LibISDB::TOTTable *>(pTable);
					if (pTOTTable != nullptr)
						pTOTTable->GetDateTime(&m_DateTime);
				}

				LibISDB::PIDMapManager m_PIDMapManager;
				LibISDB::DateTime m_DateTime;
			};

			TOTFilter TOT;

			BYTE *p = Buffer.get();
			while (p < pEnd) {
				const WORD PID = (static_cast<WORD>(p[1] & 0x1F) << 8) | static_cast<WORD>(p[2]);
				if (PID == 0x0014) {
					Packet.SetData(p, 188);
					if (Packet.ParsePacket() == LibISDB::TSPacket::ParseResult::OK) {
						LibISDB::SingleDataStream<LibISDB::TSPacket> Stream(&Packet);
						TOT.ReceiveData(&Stream);
						const LibISDB::DateTime &Time = TOT.GetTOTTime();
						// 日付をまたいで遡らないようにする
						if (Time.IsValid() && (Time.Hour > 0 || Time.Minute > 0 || Time.Second >= 5))
							m_EPGDatabaseFilter.ReceiveData(&Stream);
						break;
					}
				}
				p += 188;
			}
		}

		BYTE *p = Buffer.get();
		while (p < pEnd) {
			const WORD PID = (static_cast<WORD>(p[1] & 0x1F) << 8) | static_cast<WORD>(p[2]);
			// H-EIT / TOT / M-EIT / L-EIT
			if (PID == 0x0012 || PID == 0x0014 /*|| PID==0x0026*/ || PID == 0x0027) {
				Packet.SetData(p, 188);
				if (Packet.ParsePacket() == LibISDB::TSPacket::ParseResult::OK) {
					LibISDB::SingleDataStream<LibISDB::TSPacket> Stream(&Packet);
					m_EPGDatabaseFilter.ReceiveData(&Stream);
				}
			}
			p += 188;
		}

		if (Read < Size)
			break;
	}

	::CloseHandle(hFile);
	TRACE(TEXT("EPG data loaded {}\n"), pszFileName);
	return true;
}


bool CEpgDataLoader::Load(LPCTSTR pszFolder, HANDLE hAbortEvent, CEventHandler *pEventHandler)
{
	if (pszFolder == nullptr)
		return false;

	FILETIME ftCurrent;
	TCHAR szFileMask[MAX_PATH];
	WIN32_FIND_DATA fd;

	::GetSystemTimeAsFileTime(&ftCurrent);
	::PathCombine(szFileMask, pszFolder, TEXT("*_epg.dat"));
	const HANDLE hFind = ::FindFirstFileEx(
		szFileMask, FindExInfoBasic, &fd,
		FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
	if (hFind == INVALID_HANDLE_VALUE)
		return false;

	if (pEventHandler != nullptr)
		pEventHandler->OnStart();

	do {
		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0
				&& ftCurrent - fd.ftLastWriteTime < FILETIME_HOUR * 24 * 14
				&& ::lstrlen(pszFolder) + 1 + ::lstrlen(fd.cFileName) < MAX_PATH) {
			TCHAR szFilePath[MAX_PATH];

			::PathCombine(szFilePath, pszFolder, fd.cFileName);
			LoadFromFile(szFilePath);
			if (hAbortEvent != nullptr
					&& ::WaitForSingleObject(hAbortEvent, 0) == WAIT_OBJECT_0)
				break;
		}
	} while (::FindNextFile(hFind, &fd));
	::FindClose(hFind);

	if (pEventHandler != nullptr)
		pEventHandler->OnEnd(true, &m_EPGDatabase);

	return true;
}


bool CEpgDataLoader::LoadAsync(LPCTSTR pszFolder, CEventHandler *pEventHandler)
{
	if (IsStringEmpty(pszFolder))
		return false;

	if (m_hThread != nullptr) {
		if (::WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT)
			return false;
		::CloseHandle(m_hThread);
		m_hThread = nullptr;
	}

	m_Folder = pszFolder;
	m_pEventHandler = pEventHandler;
	if (m_hAbortEvent == nullptr)
		m_hAbortEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
	else
		::ResetEvent(m_hAbortEvent);
	m_hThread = reinterpret_cast<HANDLE>(::_beginthreadex(nullptr, 0, LoadThread, this, 0, nullptr));
	if (m_hThread == nullptr)
		return false;

	return true;
}


bool CEpgDataLoader::IsLoading() const
{
	return m_hThread != nullptr
		&& ::WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
}


bool CEpgDataLoader::Abort(DWORD Timeout)
{
	if (m_hThread == nullptr)
		return true;
	if (m_hAbortEvent != nullptr)
		::SetEvent(m_hAbortEvent);
	if (::WaitForSingleObject(m_hThread, Timeout) == WAIT_TIMEOUT)
		return false;
	return true;
}


bool CEpgDataLoader::Wait(DWORD Timeout)
{
	if (m_hThread != nullptr) {
		if (::WaitForSingleObject(m_hThread, Timeout) == WAIT_TIMEOUT)
			return false;
	}
	return true;
}


unsigned int __stdcall CEpgDataLoader::LoadThread(void *pParameter)
{
	CEpgDataLoader *pThis = static_cast<CEpgDataLoader*>(pParameter);

	if (pThis->m_pEventHandler != nullptr)
		pThis->m_pEventHandler->OnStart();
	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_LOWEST);
	const bool fSuccess = pThis->Load(pThis->m_Folder.c_str(), pThis->m_hAbortEvent);
	if (pThis->m_pEventHandler != nullptr)
		pThis->m_pEventHandler->OnEnd(fSuccess, &pThis->m_EPGDatabase);
	pThis->m_EPGDatabase.Clear();
	pThis->m_Folder.clear();
	return 0;
}




CEpgDataLoader::CEventHandler::~CEventHandler() = default;


}	// namespace TVTest
