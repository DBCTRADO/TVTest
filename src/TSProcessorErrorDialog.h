#ifndef TVTEST_TS_PROCESSOR_ERROR_DIALOG_H
#define TVTEST_TS_PROCESSOR_ERROR_DIALOG_H


#include "TSProcessor.h"
#include "Dialog.h"


namespace TVTest
{

	class CTSProcessorErrorDialog
		: public CBasicDialog
	{
	public:
		CTSProcessorErrorDialog(CTSProcessor *pTSProcessor);
		~CTSProcessorErrorDialog();
		bool Show(HWND hwndOwner) override;
		void SetMessage(LPCTSTR pszMessage);
		void SetDevice(const String &Device);
		LPCTSTR GetDevice() const;
		void SetFilter(const String &Filter);
		LPCTSTR GetFilter() const;

	private:
		CTSProcessor *m_pTSProcessor;
		String m_Message;
		String m_Device;
		String m_Filter;

		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		bool SearchFilters();
	};

}	// namespace TVTest


#endif
