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
