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


#ifndef TVTEST_EPG_CHANNEL_SETTINGS_H
#define TVTEST_EPG_CHANNEL_SETTINGS_H


#include "Dialog.h"
#include "ChannelList.h"


namespace TVTest
{

	class CProgramGuide;

	class CEpgChannelSettings
		: public CResizableDialog
	{
	public:
		CEpgChannelSettings(CProgramGuide *pProgramGuide);

		bool Show(HWND hwndOwner) override;

	private:
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		CProgramGuide *m_pProgramGuide;
		CChannelList m_ChannelList;
	};

}	// namespace TVTest


#endif
