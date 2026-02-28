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


#ifndef TVTEST_OPTIONS_H
#define TVTEST_OPTIONS_H


#include "Settings.h"
#include "Dialog.h"


namespace TVTest
{

	class ABSTRACT_CLASS(COptionFrame)
	{
	public:
		virtual ~COptionFrame() = default;

		virtual void ActivatePage(class COptions * pOptions) = 0;
		virtual void OnSettingError(class COptions * pOptions) = 0;
	};

	class COptions
		: public CResizableDialog
		, public CSettingsBase
	{
	public:
		enum {
			UPDATE_GENERAL_BUILDMEDIAVIEWER = 0x00000001UL,
			UPDATE_GENERAL_EVENTINFOFONT    = 0x00000002UL,
			UPDATE_ALL                      = 0xFFFFFFFFUL
		};

		COptions() = default;
		COptions(LPCTSTR pszSection);
		virtual ~COptions() = default;

		DWORD GetUpdateFlags() const { return m_UpdateFlags; }
		DWORD SetUpdateFlag(DWORD Flag);
		void ClearUpdateFlags() { m_UpdateFlags = 0; }
		virtual bool Apply(DWORD Flags) { return true; }

		static void SetFrame(COptionFrame *pFrame) { m_pFrame = pFrame; }
		static void ClearGeneralUpdateFlags() { m_GeneralUpdateFlags = 0; }
		static DWORD GetGeneralUpdateFlags() { return m_GeneralUpdateFlags; }
		static DWORD SetGeneralUpdateFlag(DWORD Flag);

	protected:
		void ActivatePage();
		void SettingError();

		DWORD m_UpdateFlags = 0;

	private:
		static COptionFrame *m_pFrame;
		static DWORD m_GeneralUpdateFlags;
	};

} // namespace TVTest


#endif
