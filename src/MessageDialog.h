#ifndef MESSAGE_DIALOG_H
#define MESSAGE_DIALOG_H


#include "RichEditUtil.h"


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
	TVTest::String m_Text;
	TVTest::String m_Title;
	TVTest::String m_SystemMessage;
	TVTest::String m_Caption;
	MessageType m_MessageType;
	HWND m_hDlg;

	void LogFontToCharFormat(const LOGFONT *plf, CHARFORMAT *pcf);
	static CMessageDialog *GetThis(HWND hDlg);
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};


#endif
