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


#ifndef TVTEST_TS_PROCESSOR_ERROR_DIALOG_H
#define TVTEST_TS_PROCESSOR_ERROR_DIALOG_H


#include "TSProcessor.h"
#include "Dialog.h"


namespace TVTest
{

	class CTSProcessorErrorDialog
		: public CBasicDialog
	{
	public:
		CTSProcessorErrorDialog(CTSProcessor *pTSProcessor);

		bool Show(HWND hwndOwner) override;
		void SetMessage(LPCTSTR pszMessage);
		void SetDevice(const String &Device);
		LPCTSTR GetDevice() const;
		void SetFilter(const String &Filter);
		LPCTSTR GetFilter() const;

	private:
		CTSProcessor *m_pTSProcessor;
		String m_Message;
		String m_Device;
		String m_Filter;

		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		bool SearchFilters();
	};

}	// namespace TVTest


#endif
