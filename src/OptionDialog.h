#ifndef OPTION_DIALOG_H
#define OPTION_DIALOG_H


#include "Options.h"
#include "DrawUtil.h"


class COptionDialog : public COptionFrame, public CBasicDialog
{
public:
	enum {
		PAGE_GENERAL,
		PAGE_VIEW,
		PAGE_OSD,
		PAGE_STATUS,
		PAGE_SIDEBAR,
		PAGE_MENU,
		PAGE_PANEL,
		PAGE_COLORSCHEME,
		PAGE_OPERATION,
		PAGE_ACCELERATOR,
		PAGE_CONTROLLER,
		PAGE_DRIVER,
		PAGE_AUDIO,
		PAGE_RECORD,
		PAGE_CAPTURE,
		PAGE_CHANNELSCAN,
		PAGE_EPG,
		PAGE_PROGRAMGUIDE,
		PAGE_PLUGIN,
#ifdef NETWORK_REMOCON_SUPPORT
		PAGE_NETWORKREMOCON,
#endif
		PAGE_LOG,
		PAGE_LAST=PAGE_LOG
	};

	COptionDialog();
	~COptionDialog();
	bool Show(HWND hwndOwner,int StartPage=-1);
	int GetCurrentPage() const { return m_CurrentPage; }
	bool SetCurrentPage(int Page);

private:
	enum { NUM_PAGES=PAGE_LAST+1 };
	struct PageInfo {
		LPCTSTR pszTitle;
		COptions *pOptions;
		int HelpID;
	};
	static const PageInfo m_PageList[NUM_PAGES];
	int m_CurrentPage;
	int m_StartPage;
	//DrawUtil::CBitmap m_Icons;
	HIMAGELIST m_himlIcons;
	DrawUtil::CFont m_TitleFont;
	bool m_fSettingError;
	bool m_fApplied;

	void CreatePage(int Page);
	void SetPage(int Page);
	COLORREF GetTitleColor(int Page) const;

// COptionFrame
	void OnSettingError(COptions *pOptions) override;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};


#endif
