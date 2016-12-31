#ifndef TVTEST_PROGRAM_GUIDE_TOOLBAR_OPTIONS_H
#define TVTEST_PROGRAM_GUIDE_TOOLBAR_OPTIONS_H


#include "ProgramGuide.h"
#include "Dialog.h"
#include "ListView.h"


class CProgramGuideToolbarOptions
	: public CBasicDialog
{
public:
	CProgramGuideToolbarOptions(CProgramGuideFrameSettings &FrameSettings);
	~CProgramGuideToolbarOptions();

// CBasicDialog
	bool Show(HWND hwndOwner) override;

private:
	CProgramGuideFrameSettings &m_FrameSettings;
	TVTest::CListView m_ItemListView;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

// CProgramGuideToolbarOptions
	void UpdateItemState();
};


#endif
