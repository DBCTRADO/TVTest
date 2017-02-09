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


bool EventInfoContextMenu(HWND hwndParent,HWND hwndEdit)
{
	enum {
		COMMAND_COPY=1,
		COMMAND_SELECTALL,
		COMMAND_SEARCH
	};
	HMENU hmenu=::CreatePopupMenu();

	::AppendMenu(hmenu,MF_STRING | MF_ENABLED,COMMAND_COPY,TEXT("コピー(&C)"));
	::AppendMenu(hmenu,MF_STRING | MF_ENABLED,COMMAND_SELECTALL,TEXT("すべて選択(&A)"));

	if (CRichEditUtil::IsSelected(hwndEdit)) {
		const TVTest::CKeywordSearch &KeywordSearch=GetAppClass().KeywordSearch;
		if (KeywordSearch.GetSearchEngineCount()>0) {
			::AppendMenu(hmenu,MF_SEPARATOR,0,nullptr);
			KeywordSearch.InitializeMenu(hmenu,COMMAND_SEARCH);
		}
	}

	POINT pt;
	::GetCursorPos(&pt);
	int Command=::TrackPopupMenu(hmenu,TPM_RIGHTBUTTON | TPM_RETURNCMD,pt.x,pt.y,0,hwndParent,nullptr);
	::DestroyMenu(hmenu);

	switch (Command) {
	case COMMAND_COPY:
		if (CRichEditUtil::IsSelected(hwndEdit)) {
			::SendMessage(hwndEdit,WM_COPY,0,0);
		} else {
			CRichEditUtil::CopyAllText(hwndEdit);
		}
		break;

	case COMMAND_SELECTALL:
		CRichEditUtil::SelectAll(hwndEdit);
		break;

	default:
		if (Command>=COMMAND_SEARCH) {
			LPTSTR pszKeyword=CRichEditUtil::GetSelectedText(hwndEdit);
			if (pszKeyword!=nullptr) {
				GetAppClass().KeywordSearch.Search(Command-COMMAND_SEARCH,pszKeyword);
				delete [] pszKeyword;
			}
			break;
		}

		return false;
	}

	return true;
}


}	// namespace EventInfoUtil

}	// namespace TVTest
