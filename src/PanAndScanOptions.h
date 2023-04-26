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


#ifndef TVTEST_PAN_AND_SCAN_OPTIONS_H
#define TVTEST_PAN_AND_SCAN_OPTIONS_H


#include <vector>
#include "Settings.h"
#include "Dialog.h"
#include "CoreEngine.h"
#include "Command.h"


namespace TVTest
{

	class CPanAndScanOptions
		: public CBasicDialog
		, public CSettingsBase
		, public CCommandManager::CCommandCustomizer
	{
	public:
		static constexpr size_t MAX_NAME = 64;

		struct PanAndScanInfo
		{
			CCoreEngine::PanAndScanInfo Info;
			TCHAR szName[MAX_NAME];
			UINT ID;
		};

		CPanAndScanOptions();

	// CBasicDialog
		bool Show(HWND hwndOwner) override;

	// CSettingsBase
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	// CPanAndScanOptions
		size_t GetPresetCount() const;
		bool GetPreset(size_t Index, PanAndScanInfo *pInfo) const;
		bool GetPresetByID(UINT ID, PanAndScanInfo *pInfo) const;
		UINT GetPresetID(size_t Index) const;
		int FindPresetByID(UINT ID) const;

	private:
	//CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		void SetItemStatus() const;
		bool GetSettings(PanAndScanInfo *pInfo) const;
		bool GetPanAndScanSettings(CCoreEngine::PanAndScanInfo *pInfo) const;
		bool IsSettingsValid() const;
		bool Import(LPCTSTR pszFileName);
		bool Export(LPCTSTR pszFileName) const;

	//CCommandCustomizer
		bool GetCommandText(int Command, LPTSTR pszText, size_t MaxLength) override;

		std::vector<PanAndScanInfo> m_PresetList;
		UINT m_PresetID;
		bool m_fStateChanging = false;
		bool m_fTested = false;
		CCoreEngine::PanAndScanInfo m_OldPanAndScanInfo;
	};

}	// namespace TVTest


#endif
