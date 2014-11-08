#ifndef MESSAGE_DIALOG_H
#define MESSAGE_DIALOG_H


#include "RichEditUtil.h"


class CMessageDialog
{
public:
	enum MessageType {
		TYPE_INFO,
		TYPE_WARNING,
		TYPE_ERROR
	};

	CMessageDialog();
	~CMessageDialog();
	bool Show(HWND hwndOwner,MessageType Type,LPCTSTR pszText,LPCTSTR pszTitle=NULL,LPCTSTR pszSystemMessage=NULL,LPCTSTR pszCaption=NULL);

private:
	CRichEditUtil m_RichEditUtil;
	TVTest::String m_Text;
	TVTest::String m_Title;
	TVTest::String m_SystemMessage;
	TVTest::String m_Caption;
	MessageType m_MessageType;
	HWND m_hDlg;

	void LogFontToCharFormat(const LOGFONT *plf,CHARFORMAT *pcf);
	static CMessageDialog *GetThis(HWND hDlg);
	static INT_PTR CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
