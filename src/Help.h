#ifndef TVTEST_HELP_H
#define TVTEST_HELP_H


#include "Help/HelpID.h"


class CHtmlHelp
{
	typedef HWND (WINAPI *HtmlHelpFunc)(HWND hwndCaller,LPCTSTR pszFile,
										UINT uCommand,DWORD_PTR dwData);
	TCHAR m_szFileName[MAX_PATH];
	HMODULE m_hLib;
	HtmlHelpFunc m_pHtmlHelp;
	DWORD m_Cookie;

public:
	CHtmlHelp();
	~CHtmlHelp();
	bool Initialize();
	void Finalize();
	bool ShowIndex();
	bool ShowContent(int ID);
	bool PreTranslateMessage(MSG *pmsg);
};


#endif
