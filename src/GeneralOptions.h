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


#ifndef TVTEST_GENERAL_OPTIONS_H
#define TVTEST_GENERAL_OPTIONS_H


#include "Options.h"


namespace TVTest
{

	class CGeneralOptions
		: public COptions
	{
	public:
		enum class DefaultDriverType {
			None,
			Last,
			Custom,
			TVTEST_ENUM_CLASS_TRAILER
		};

		CGeneralOptions();
		~CGeneralOptions();

	// COptions
		bool Apply(DWORD Flags) override;

	// CSettingsBase
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	// CBasicDialog
		bool Create(HWND hwndOwner) override;

	// CGeneralOptions
		DefaultDriverType GetDefaultDriverType() const;
		LPCTSTR GetDefaultDriverName() const;
		bool SetDefaultDriverName(LPCTSTR pszDriverName);
		bool GetFirstDriverName(String *pDriverName) const;
		bool GetResident() const;
		bool GetKeepSingleTask() const;
		bool GetStandaloneProgramGuide() const { return m_fStandaloneProgramGuide; }

	private:
	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		enum {
			UPDATE_RESIDENT     = 0x00000001UL,
			UPDATE_1SEGFALLBACK = 0x00000002UL
		};

		String m_BonDriverDirectory;
		DefaultDriverType m_DefaultDriverType;
		String m_DefaultBonDriverName;
		String m_LastBonDriverName;
		bool m_fResident;
		bool m_fKeepSingleTask;
		bool m_fStandaloneProgramGuide;
		bool m_fEnable1SegFallback;
	};

}	// namespace TVTest


#endif
