/*
  TVTest
  Copyright(c) 2008-2017 DBCTRADO

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

			virtual void OnBeginLoad() {}
			virtual void OnEndLoad(bool fSuccess) {}
		};

		typedef CEpgDataLoader::CEventHandler CEDCBDataLoadEventHandler;

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

		bool LoadEpgFile(LibISDB::EPGDatabase *pEPGDatabase);
		bool AsyncLoadEpgFile(
			LibISDB::EPGDatabase *pEPGDatabase,
			CEpgDataStore::CEventHandler *pEventHandler = nullptr);
		bool IsEpgFileLoading() const;
		bool WaitEpgFileLoad(DWORD Timeout = INFINITE);
		bool SaveEpgFile(LibISDB::EPGDatabase *pEPGDatabase);

		bool LoadEDCBData();
		bool AsyncLoadEDCBData(CEDCBDataLoadEventHandler *pEventHandler = nullptr);
		bool IsEDCBDataLoading() const;
		bool WaitEDCBDataLoad(DWORD Timeout = INFINITE);

		EpgTimeMode GetEpgTimeMode() const { return m_EpgTimeMode; }

		bool LoadLogoFile();
		bool SaveLogoFile();

	private:
		bool m_fSaveEpgFile;
		CFilePath m_EpgFileName;
		bool m_fUpdateWhenStandby;
		bool m_fUpdateBSExtended;
		bool m_fUpdateCSExtended;
		bool m_fUseEDCBData;
		CFilePath m_EDCBDataFolder;
		EpgTimeMode m_EpgTimeMode;
		bool m_fSaveLogoFile;
		CFilePath m_LogoFileName;

		CEpgDataStore m_EpgDataStore;
		std::unique_ptr<CEpgDataLoader> m_EpgDataLoader;

		Style::Font m_EventInfoFont;
		Style::Font m_CurEventInfoFont;

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		bool GetEpgFileFullPath(LPTSTR pszFileName);
		static unsigned int __stdcall EpgFileLoadThread(void *pParameter);
	};

}	// namespace TVTest


#endif
