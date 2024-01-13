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


#ifndef TVTEST_EPG_DATA_STORE_H
#define TVTEST_EPG_DATA_STORE_H


#include "LibISDB/LibISDB/EPG/EPGDataFile.hpp"


namespace TVTest
{

	class CEpgDataStore
	{
	public:
		class ABSTRACT_CLASS(CEventHandler)
		{
		public:
			virtual ~CEventHandler() = default;

			virtual void OnBeginLoading() {}
			virtual void OnEndLoading(bool fSuccess) {}
		};

		enum class OpenFlag : unsigned int {
			None           = 0x0000U,
			LoadBackground = 0x0001U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		CEpgDataStore();
		~CEpgDataStore();

		CEpgDataStore(const CEpgDataStore &) = delete;
		CEpgDataStore &operator=(const CEpgDataStore &) = delete;

		bool Open(LibISDB::EPGDatabase *pEPGDatabase, LPCTSTR pszFileName, OpenFlag Flags = OpenFlag::None);
		void Close();
		bool IsOpen() const;
		bool Load();
		bool LoadAsync();
		bool IsLoading() const;
		bool Wait(DWORD Timeout = INFINITE);
		bool Save();
		void SetEventHandler(CEventHandler *pEventHandler);

	private:
		bool LoadMain();
		bool WaitThread(DWORD Timeout);
		bool CreateLock(CInterprocessReadWriteLock *pLock) const;

		static unsigned int __stdcall LoadThread(void *pParameter);

		LibISDB::EPGDataFile m_EPGDataFile;
		OpenFlag m_OpenFlags = OpenFlag::None;
		HANDLE m_hThread = nullptr;
		CEventHandler *m_pEventHandler = nullptr;
		uint64_t m_UpdateCount = 0;
		DWORD m_LockTimeout = 10000;
	};

} // namespace TVTest


#endif // TVTEST_EPG_DATA_STORE_H
