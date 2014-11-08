#ifndef CARD_READER_DIALOG_H
#define CARD_READER_DIALOG_H


#include "Dialog.h"


class CCardReaderErrorDialog : public CBasicDialog
{
public:
	CCardReaderErrorDialog();
	~CCardReaderErrorDialog();
	bool Show(HWND hwndOwner) override;
	void SetMessage(LPCTSTR pszMessage);
	int GetCasDevice() const { return m_CasDevice; }
	LPCTSTR GetReaderName() const;

private:
	TVTest::String m_Message;
	int m_CasDevice;
	TVTest::String m_ReaderName;

	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};


#endif
