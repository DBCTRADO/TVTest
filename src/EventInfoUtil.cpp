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
	const HMENU hmenu = ::CreatePopupMenu();

	::AppendMenu(hmenu, MF_STRING | MF_ENABLED, COMMAND_COPY, TEXT("コピー(&C)"));
	::AppendMenu(hmenu, MF_STRING | MF_ENABLED, COMMAND_SELECTALL, TEXT("すべて選択(&A)"));

	if (CRichEditUtil::IsSelected(hwndEdit)) {
		const CKeywordSearch &KeywordSearch = GetAppClass().KeywordSearch;
		if (KeywordSearch.GetSearchEngineCount() > 0) {
			::AppendMenu(hmenu, MF_SEPARATOR, 0, nullptr);
			KeywordSearch.InitializeMenu(hmenu, COMMAND_SEARCH);
		}
	}

	POINT pt;
	::GetCursorPos(&pt);
	const int Command = ::TrackPopupMenu(hmenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hwndParent, nullptr);
	::DestroyMenu(hmenu);

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
