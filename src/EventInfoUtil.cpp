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
#include "AppMain.h"
#include "EventInfoUtil.h"
#include "RichEditUtil.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace EventInfoUtil
{


bool EventInfoContextMenu(HWND hwndParent, HWND hwndEdit)
{
	enum {
		COMMAND_COPY = 1,
		COMMAND_SELECTALL,
		COMMAND_SEARCH
	};
	CPopupMenu Menu;

	Menu.Create();
	Menu.Append(COMMAND_COPY, TEXT("コピー(&C)"));
	Menu.Append(COMMAND_SELECTALL, TEXT("すべて選択(&A)"));

	if (CRichEditUtil::IsSelected(hwndEdit)) {
		const CKeywordSearch &KeywordSearch = GetAppClass().KeywordSearch;
		if (KeywordSearch.GetSearchEngineCount() > 0) {
			Menu.AppendSeparator();
			KeywordSearch.InitializeMenu(Menu.GetPopupHandle(), COMMAND_SEARCH);
		}
	}

	const int Command = Menu.Show(hwndParent, nullptr, TPM_RIGHTBUTTON | TPM_RETURNCMD);

	switch (Command) {
	case COMMAND_COPY:
		if (CRichEditUtil::IsSelected(hwndEdit)) {
			::SendMessage(hwndEdit, WM_COPY, 0, 0);
		} else {
			CRichEditUtil::CopyAllText(hwndEdit);
		}
		break;

	case COMMAND_SELECTALL:
		CRichEditUtil::SelectAll(hwndEdit);
		break;

	default:
		if (Command >= COMMAND_SEARCH) {
			String Keyword(CRichEditUtil::GetSelectedText(hwndEdit));
			if (!Keyword.empty()) {
				GetAppClass().KeywordSearch.Search(Command - COMMAND_SEARCH, Keyword.c_str());
			}
			break;
		}

		return false;
	}

	return true;
}


}	// namespace EventInfoUtil

}	// namespace TVTest
