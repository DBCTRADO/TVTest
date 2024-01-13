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


#ifndef TVTEST_EPG_OPTIONS_H
#define TVTEST_EPG_OPTIONS_H


#include "CoreEngine.h"
#include "Options.h"
#include "LogoManager.h"
#include "EpgDataStore.h"
#include "EpgDataLoader.h"
#include "Style.h"


namespace TVTest
{

	class CEpgOptions
		: public COptions
	{
	public:
		class ABSTRACT_CLASS(CEpgFileLoadEventHandler)
		{
		public:
			virtual ~CEpgFileLoadEventHandler() = default;

			virtual void OnBeginEpgDataLoading() {}
			virtual void OnEndEpgDataLoading(bool fSuccess) {}
			virtual void OnBeginEdcbDataLoading() {}
			virtual void OnEndEdcbDataLoading(bool fSuccess, LibISDB::EPGDatabase *pEPGDatabase) {}
		};

		enum class EpgFileLoadFlag : unsigned int {
			None     = 0x0000U,
			EpgData  = 0x0001U,
			EdcbData = 0x0002U,
			TVTEST_ENUM_FLAGS_TRAILER,
			AllData  = EpgData | EdcbData,
		};

		enum class EpgTimeMode {
			Raw,
			JST,
			Local,
			UTC,
			TVTEST_ENUM_CLASS_TRAILER
		};

		CEpgOptions();
		~CEpgOptions();

	// CSettingsBase
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	// CBasicDialog
		bool Create(HWND hwndOwner) override;

	// CEpgOptions
		void Finalize();

		LPCTSTR GetEpgFileName() const { return m_EpgFileName.c_str(); }
		bool GetUpdateWhenStandby() const { return m_fUpdateWhenStandby; }
		bool GetUpdateBSExtended() const { return m_fUpdateBSExtended; }
		bool GetUpdateCSExtended() const { return m_fUpdateCSExtended; }

		const Style::Font &GetEventInfoFont() const { return m_EventInfoFont; }

		bool LoadEpgFile(
			LibISDB::EPGDatabase *pEPGDatabase,
			CEpgFileLoadEventHandler *pEventHandler = nullptr,
			EpgFileLoadFlag Flags = EpgFileLoadFlag::AllData);
		bool IsEpgFileLoading() const;
		bool IsEpgDataLoading() const;
		bool WaitEpgFileLoad(DWORD Timeout = INFINITE);
		bool SaveEpgFile(LibISDB::EPGDatabase *pEPGDatabase);

		EpgTimeMode GetEpgTimeMode() const { return m_EpgTimeMode; }

		bool LoadLogoFile();
		bool SaveLogoFile();

	private:
		class CEpgFileLoader
			: protected CEpgDataStore::CEventHandler
			, protected CEpgDataLoader::CEventHandler
		{
		public:
			~CEpgFileLoader();

			bool StartLoading(
				LibISDB::EPGDatabase *pEPGDatabase,
				CEpgDataStore *pEpgDataStore, const String &EpgDataPath,
				CEpgDataLoader *pEdcbDataLoader, const String &EdcbDataFolder,
				CEpgFileLoadEventHandler *pEventHandler);
			bool IsLoading();
			bool IsEpgDataLoading();
			bool WaitLoading(DWORD Timeout = INFINITE);

		private:
			enum {
				STATE_READY,
				STATE_ERROR,
				STATE_THREAD_START,
				STATE_THREAD_END,
				STATE_EPG_DATA_LOADING,
				STATE_EPG_DATA_LOADED,
				STATE_EDCB_DATA_LOADING,
				STATE_EDCB_DATA_LOADED,
			};

			LibISDB::EPGDatabase *m_pEPGDatabase = nullptr;
			CEpgDataStore *m_pEpgDataStore = nullptr;
			String m_EpgDataPath;
			CEpgDataLoader *m_pEdcbDataLoader = nullptr;
			String m_EdcbDataFolder;
			CEpgFileLoadEventHandler *m_pEventHandler = nullptr;
			HANDLE m_hThread = nullptr;
			HANDLE m_hAbortEvent = nullptr;
			std::atomic<int> m_State = STATE_READY;

		// CEpgDataStore::CEventHandler
			void OnBeginLoading() override;
			void OnEndLoading(bool fSuccess) override;

		// CEDCBDataLoadEventHandler
			void OnStart() override;
			void OnEnd(bool fSuccess, LibISDB::EPGDatabase *pEPGDatabase) override;

			void LoadMain();
			static unsigned int __stdcall LoadThread(void *pParameter);
		};

		bool m_fSaveEpgFile = true;
		CFilePath m_EpgFileName{TEXT("EpgData")};
		bool m_fUpdateWhenStandby = false;
		bool m_fUpdateBSExtended = false;
		bool m_fUpdateCSExtended = false;
		bool m_fUseEDCBData = false;
		CFilePath m_EDCBDataFolder;
		EpgTimeMode m_EpgTimeMode = EpgTimeMode::JST;
		bool m_fSaveLogoFile = true;
		CFilePath m_LogoFileName{TEXT("LogoData")};

		std::unique_ptr<CEpgFileLoader> m_EpgFileLoader;
		CEpgDataStore m_EpgDataStore;
		std::unique_ptr<CEpgDataLoader> m_EpgDataLoader;

		Style::Font m_EventInfoFont;
		Style::Font m_CurEventInfoFont;

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		bool GetEpgFileFullPath(LPTSTR pszFileName);
	};

} // namespace TVTest


#endif
