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


#ifndef TVTEST_EPG_DATA_LOADER_H
#define TVTEST_EPG_DATA_LOADER_H


#include "LibISDB/LibISDB/Filters/EPGDatabaseFilter.hpp"


namespace TVTest
{

	// EpgDataCap_BonのEPGデータを読み込むクラス
	class CEpgDataLoader
	{
	public:
		class ABSTRACT_CLASS(CEventHandler)
		{
		public:
			virtual ~CEventHandler() = 0;

			virtual void OnStart() {}
			virtual void OnEnd(bool fSuccess, LibISDB::EPGDatabase * pEPGDatabase) {}
		};

		CEpgDataLoader();
		~CEpgDataLoader();

		CEpgDataLoader(const CEpgDataLoader &) = delete;
		CEpgDataLoader &operator=(const CEpgDataLoader &) = delete;

		bool Load(LPCTSTR pszFolder, HANDLE hAbortEvent = nullptr, CEventHandler *pEventHandler = nullptr);
		bool LoadAsync(LPCTSTR pszFolder, CEventHandler *pEventHandler = nullptr);
		bool IsLoading() const;
		bool Abort(DWORD Timeout = 10000);
		bool Wait(DWORD Timeout = INFINITE);

	private:
		LibISDB::EPGDatabase m_EPGDatabase;
		LibISDB::EPGDatabaseFilter m_EPGDatabaseFilter;
		HANDLE m_hThread = nullptr;
		HANDLE m_hAbortEvent = nullptr;
		String m_Folder;
		CEventHandler *m_pEventHandler = nullptr;

		bool LoadFromFile(LPCTSTR pszFileName);
		static unsigned int __stdcall LoadThread(void *pParameter);
	};

} // namespace TVTest


#endif /* TVTEST_EPG_DATA_LOADER_H */
