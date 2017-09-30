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
		};

		CEpgDataStore();
		~CEpgDataStore();

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

		static unsigned int __stdcall LoadThread(void *pParameter);

		LibISDB::EPGDataFile m_EPGDataFile;
		OpenFlag m_OpenFlags;
		HANDLE m_hThread;
		CEventHandler *m_pEventHandler;
		uint64_t m_UpdateCount;
	};

	LIBISDB_ENUM_FLAGS(CEpgDataStore::OpenFlag)

}	// namespace TVTest


#endif	// TVTEST_EPG_DATA_STORE_H
