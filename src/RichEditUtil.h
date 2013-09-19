#ifndef RICH_EDIT_UTIL_H
#define RICH_EDIT_UTIL_H


#include <richedit.h>
#include <vector>


class CRichEditUtil
{
public:
	typedef std::vector<CHARRANGE> CharRangeList;

	CRichEditUtil();
	~CRichEditUtil();
	bool LoadRichEditLib();
	void UnloadRichEditLib();
	bool IsRichEditLibLoaded() const { return m_hLib!=NULL; }
	LPCTSTR GetWindowClassName() const { return TEXT("RichEdit20W"); }
	static bool LogFontToCharFormat(HDC hdc,const LOGFONT *plf,CHARFORMAT *pcf);
	static bool LogFontToCharFormat2(HDC hdc,const LOGFONT *plf,CHARFORMAT2 *pcf);
	static void CharFormatToCharFormat2(const CHARFORMAT *pcf,CHARFORMAT2 *pcf2);
	static bool AppendText(HWND hwndEdit,LPCTSTR pszText,const CHARFORMAT *pcf);
	static bool AppendText(HWND hwndEdit,LPCTSTR pszText,const CHARFORMAT2 *pcf);
	static bool CopyAllText(HWND hwndEdit);
	static void SelectAll(HWND hwndEdit);
	static bool IsSelected(HWND hwndEdit);
	static LPTSTR GetSelectedText(HWND hwndEdit);
	static int GetMaxLineWidth(HWND hwndEdit);
	enum {
		URL_NO_LINK = 0x0001
	};
	static bool DetectURL(HWND hwndEdit,const CHARFORMAT *pcf,int FirstLine=0,int LastLine=-1,
						  unsigned int Flags=0,CharRangeList *pCharRangeList=NULL);
	static bool HandleLinkClick(const ENLINK *penl);
	static bool HandleLinkClick(HWND hwndEdit,const POINT &Pos,const CharRangeList &LinkList);
	static int LinkHitTest(HWND hwndEdit,const POINT &Pos,const CharRangeList &LinkList);

private:
	HMODULE m_hLib;

	static bool SearchNextURL(LPCTSTR *ppszText,int *pLength);
	static bool OpenLink(HWND hwndEdit,const CHARRANGE &Range);
};


#endif
