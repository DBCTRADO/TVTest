#ifndef TVTEST_STREAM_INFO_H
#define TVTEST_STREAM_INFO_H


#include "Dialog.h"


namespace TVTest
{

	class CStreamInfo
		: public CResizableDialog
	{
	public:
		class CEventHandler
		{
		public:
			virtual ~CEventHandler() = default;

			virtual void OnRestoreSettings() {}
			virtual bool OnClose() { return true; }
		};

		CStreamInfo();
		~CStreamInfo();

		bool Create(HWND hwndOwner);
		void SetEventHandler(CEventHandler *pHandler);

	private:
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		void SetService();
		static int GetTreeViewText(HWND hwndTree, HTREEITEM hItem, bool fSiblings, LPTSTR pszText, int MaxText, int Level = 0);

		CEventHandler *m_pEventHandler;
		bool m_fCreateFirst;
	};

}	// namespace TVTest


#endif
