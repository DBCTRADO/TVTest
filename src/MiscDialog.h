#ifndef MISC_DIALOG_H
#define MISC_DIALOG_H


#include "Dialog.h"
#include "Aero.h"
#include "DrawUtil.h"
#include "Graphics.h"


class CAboutDialog : public CBasicDialog
{
public:
	CAboutDialog();
	~CAboutDialog();
	bool Show(HWND hwndOwner) override;

private:
	CAeroGlass m_AeroGlass;
	TVTest::Graphics::CImage m_LogoImage;
	bool m_fDrawLogo;
	DrawUtil::CFont m_Font;
	DrawUtil::CFont m_LinkFont;

	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};


#endif
