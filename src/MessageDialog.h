/*
  TVTest
  Copyright(c) 2008-2019 DBCTRADO

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


#ifndef TVTEST_MESSAGE_DIALOG_H
#define TVTEST_MESSAGE_DIALOG_H


#include "RichEditUtil.h"


namespace TVTest
{

	class CMessageDialog
	{
	public:
		enum class MessageType {
			Info,
			Warning,
			Error,
		};

		CMessageDialog();
		~CMessageDialog();

		bool Show(HWND hwndOwner, MessageType Type, LPCTSTR pszText, LPCTSTR pszTitle = nullptr, LPCTSTR pszSystemMessage = nullptr, LPCTSTR pszCaption = nullptr);

	private:
		CRichEditUtil m_RichEditUtil;
		String m_Text;
		String m_Title;
		String m_SystemMessage;
		String m_Caption;
		MessageType m_MessageType;
		HWND m_hDlg;

		void LogFontToCharFormat(const LOGFONT *plf, CHARFORMAT *pcf);
		static CMessageDialog *GetThis(HWND hDlg);
		static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	};

}	// namespace TVTest


#endif
