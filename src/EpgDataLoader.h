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

		bool Load(LPCTSTR pszFolder, HANDLE hAbortEvent = nullptr);
		bool LoadAsync(LPCTSTR pszFolder, CEventHandler *pEventHandler = nullptr);
		bool IsLoading() const;
		bool Abort(DWORD Timeout = 10000);
		bool Wait(DWORD Timeout = INFINITE);

	private:
		LibISDB::EPGDatabase m_EPGDatabase;
		LibISDB::EPGDatabaseFilter m_EPGDatabaseFilter;
		HANDLE m_hThread;
		HANDLE m_hAbortEvent;
		String m_Folder;
		CEventHandler *m_pEventHandler;

		bool LoadFromFile(LPCTSTR pszFileName);
		static unsigned int __stdcall LoadThread(void *pParameter);
	};

}	// namespace TVTest


#endif	/* TVTEST_EPG_DATA_LOADER_H */
