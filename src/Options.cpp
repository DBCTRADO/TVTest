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


#include "stdafx.h"
#include "TVTest.h"
#include "Options.h"
#include "Common/DebugDef.h"


namespace TVTest
{


COptionFrame *COptions::m_pFrame = nullptr;
DWORD COptions::m_GeneralUpdateFlags = 0;


COptions::COptions(LPCTSTR pszSection)
	: CSettingsBase(pszSection)
{
}


DWORD COptions::SetUpdateFlag(DWORD Flag)
{
	m_UpdateFlags |= Flag;
	return m_UpdateFlags;
}


DWORD COptions::SetGeneralUpdateFlag(DWORD Flag)
{
	m_GeneralUpdateFlags |= Flag;
	return m_GeneralUpdateFlags;
}


void COptions::ActivatePage()
{
	if (m_pFrame != nullptr)
		m_pFrame->ActivatePage(this);
}


void COptions::SettingError()
{
	if (m_pFrame != nullptr)
		m_pFrame->OnSettingError(this);
}


}	// namespace TVTest
