#ifndef CARD_READER_DIALOG_H
#define CARD_READER_DIALOG_H


#include "Dialog.h"


class CCardReaderErrorDialog : public CBasicDialog
{
public:
	CCardReaderErrorDialog();
	~CCardReaderErrorDialog();
	bool Show(HWND hwndOwner) override;
	bool SetMessage(LPCTSTR pszMessage);
	int GetCasDevice() const { return m_CasDevice; }
	LPCTSTR GetReaderName() const { return m_ReaderName.Get(); }

private:
	CDynamicString m_Message;
	int m_CasDevice;
	CDynamicString m_ReaderName;

	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};


#endif
