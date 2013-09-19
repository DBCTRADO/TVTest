#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "MiscDialog.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


static const size_t MAX_INFO_TEXT=256;




CAboutDialog::CAboutDialog()
{
}


CAboutDialog::~CAboutDialog()
{
	Destroy();

	m_LogoImage.Free();
	m_GdiPlus.Finalize();
}


bool CAboutDialog::Show(HWND hwndOwner)
{
	return ShowDialog(hwndOwner,GetAppClass().GetResourceInstance(),
					  MAKEINTRESOURCE(IDD_ABOUT))==IDOK;
}


INT_PTR CAboutDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static const struct {
		COLORREF Header1;
		COLORREF Header2;
		COLORREF HeaderText;
		COLORREF HeaderShadow;
		COLORREF Info1;
		COLORREF Info2;
		COLORREF InfoText;
	} Colors = {
		RGB(192,213,245),RGB(160,192,240),RGB(0,0,0),RGB(200,214,255),RGB(224,232,255),RGB(240,244,255),RGB(64,64,64)
	};

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			HWND hwndHeader=::GetDlgItem(hDlg,IDC_ABOUT_HEADER);
			HWND hwndInfo=::GetDlgItem(hDlg,IDC_ABOUT_INFO);
			HWND hwndLogo=::GetDlgItem(hDlg,IDC_ABOUT_LOGO);

			m_Font.Create(DrawUtil::FONT_MESSAGE);

			::SetWindowText(hwndHeader,
				ABOUT_VERSION_TEXT
#ifdef VERSION_PLATFORM
				TEXT(" (") VERSION_PLATFORM TEXT(")")
#endif
				);

			HDC hdc=::GetDC(hDlg);
			HFONT hfontOld=DrawUtil::SelectObject(hdc,m_Font);
			TCHAR szText[MAX_INFO_TEXT];
			::GetWindowText(hwndInfo,szText,lengthof(szText));
			RECT rcText={0,0,0,0};
			::DrawText(hdc,szText,-1,&rcText,DT_CALCRECT | DT_NOPREFIX);
			::SelectObject(hdc,hfontOld);
			::ReleaseDC(hDlg,hdc);
			RECT rcInfo;
			::GetWindowRect(hwndInfo,&rcInfo);
			::OffsetRect(&rcText,-rcText.left,-rcText.top);
			::OffsetRect(&rcInfo,-rcInfo.left,-rcInfo.top);
			RECT rcLogo;
			::GetWindowRect(hwndLogo,&rcLogo);
			if (rcInfo.bottom<rcText.bottom || rcInfo.right<rcText.right) {
				int Width=max(rcInfo.right,rcText.right);
				int Height=max(rcInfo.bottom,rcText.bottom);
				::SetWindowPos(hwndInfo,NULL,0,0,Width,Height,
							   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
				RECT rcHeader;
				::GetWindowRect(hwndHeader,&rcHeader);
				::SetWindowPos(hwndHeader,NULL,0,0,Width,rcHeader.bottom-rcHeader.top,
							   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
				RECT rcDialog;
				::GetWindowRect(hDlg,&rcDialog);
				::SetWindowPos(hDlg,NULL,0,0,
							   (rcDialog.right-rcDialog.left)+(Width-rcInfo.right),
							   (rcDialog.bottom-rcDialog.top)+(Height-rcInfo.bottom),
							   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
				RECT rcClient;
				::GetClientRect(hDlg,&rcClient);
				::SetWindowPos(hwndLogo,NULL,0,0,
							   rcLogo.right-rcLogo.left,rcClient.bottom,
							   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
			}

			::SetRect(&rcLogo,rcLogo.right-rcLogo.left,0,0,0);
			if (m_AeroGlass.ApplyAeroGlass(hDlg,&rcLogo)) {
				m_GdiPlus.Initialize();
				m_LogoImage.LoadFromResource(GetAppClass().GetResourceInstance(),
					MAKEINTRESOURCE(IDB_LOGO32),TEXT("PNG"));
				::ShowWindow(hwndLogo,SW_HIDE);
			} else {
				HBITMAP hbm=::LoadBitmap(GetAppClass().GetResourceInstance(),
										 MAKEINTRESOURCE(IDB_LOGO));
				::SendMessage(hwndLogo,STM_SETIMAGE,
							  IMAGE_BITMAP,reinterpret_cast<LPARAM>(hbm));
			}

			AdjustDialogPos(GetParent(hDlg),hDlg);
		}
		return TRUE;

	case WM_CTLCOLORSTATIC:
		if (reinterpret_cast<HWND>(lParam)==::GetDlgItem(hDlg,IDC_ABOUT_LOGO))
			return reinterpret_cast<INT_PTR>(::GetStockObject(WHITE_BRUSH));
		if (reinterpret_cast<HWND>(lParam)==::GetDlgItem(hDlg,IDC_ABOUT_HEADER)
				|| reinterpret_cast<HWND>(lParam)==::GetDlgItem(hDlg,IDC_ABOUT_INFO))
			return reinterpret_cast<INT_PTR>(::GetStockObject(NULL_BRUSH));
		break;

	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT pdis=reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
			int OldBkMode=::SetBkMode(pdis->hDC,TRANSPARENT);
			HFONT hfontOld=DrawUtil::SelectObject(pdis->hDC,m_Font);
			COLORREF OldTextColor;
			TCHAR szText[MAX_INFO_TEXT];

			if (pdis->CtlID==IDC_ABOUT_HEADER) {
				OldTextColor=::SetTextColor(pdis->hDC,Colors.HeaderText);
				::GetDlgItemText(hDlg,IDC_ABOUT_HEADER,szText,lengthof(szText));
				::DrawText(pdis->hDC,szText,-1,&pdis->rcItem,
						   DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
			} else if (pdis->CtlID==IDC_ABOUT_INFO) {
				OldTextColor=::SetTextColor(pdis->hDC,Colors.InfoText);
				::GetDlgItemText(hDlg,IDC_ABOUT_INFO,szText,lengthof(szText));
				::DrawText(pdis->hDC,szText,-1,&pdis->rcItem,DT_CENTER | DT_NOPREFIX);
			}

			::SetTextColor(pdis->hDC,OldTextColor);
			::SelectObject(pdis->hDC,hfontOld);
			::SetBkMode(pdis->hDC,OldBkMode);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		break;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rcClient,rcLogo,rcHeader,rc;

			::BeginPaint(hDlg,&ps);
			GetDlgItemRect(hDlg,IDC_ABOUT_LOGO,&rcLogo);
			GetDlgItemRect(hDlg,IDC_ABOUT_HEADER,&rcHeader);
			::GetClientRect(hDlg,&rcClient);
			rcClient.left=rcLogo.right;

			if (m_GdiPlus.IsInitialized()) {
				CGdiPlus::CCanvas Canvas(ps.hdc);

				Canvas.Clear(0,0,0,0);
				m_GdiPlus.DrawImage(&Canvas,&m_LogoImage,
									(rcLogo.right-m_LogoImage.GetWidth())/2,
									(rcLogo.bottom-m_LogoImage.GetHeight())/2);
				rc=rcClient;
				rc.bottom=rcHeader.bottom;
				m_GdiPlus.FillGradient(&Canvas,Colors.Header1,Colors.Header2,
									   rc,CGdiPlus::GRADIENT_DIRECTION_VERT);
				rc.top=rc.bottom;
				rc.bottom=rc.top+8;
				m_GdiPlus.FillGradient(&Canvas,Colors.HeaderShadow,Colors.Info1,
									   rc,CGdiPlus::GRADIENT_DIRECTION_VERT);
				rc.top=rc.bottom;
				rc.bottom=rcClient.bottom;
				m_GdiPlus.FillGradient(&Canvas,Colors.Info1,Colors.Info2,
									   rc,CGdiPlus::GRADIENT_DIRECTION_VERT);
			} else {
				rc=rcClient;
				rc.bottom=rcHeader.bottom;
				DrawUtil::FillGradient(ps.hdc,&rc,Colors.Header1,Colors.Header2,
									   DrawUtil::DIRECTION_VERT);
				rc.top=rc.bottom;
				rc.bottom=rc.top+8;
				DrawUtil::FillGradient(ps.hdc,&rc,Colors.HeaderShadow,Colors.Info1,
									   DrawUtil::DIRECTION_VERT);
				rc.top=rc.bottom;
				rc.bottom=rcClient.bottom;
				DrawUtil::FillGradient(ps.hdc,&rc,Colors.Info1,Colors.Info2,
									   DrawUtil::DIRECTION_VERT);
			}

			::EndPaint(hDlg,&ps);
		}
		return TRUE;

	case WM_DESTROY:
		{
			HBITMAP hbm=reinterpret_cast<HBITMAP>(::SendDlgItemMessage(hDlg,IDC_ABOUT_LOGO,
				STM_SETIMAGE,IMAGE_BITMAP,reinterpret_cast<LPARAM>((HBITMAP)NULL)));

			if (hbm!=NULL) {
				::DeleteObject(hbm);
			} else {
				m_LogoImage.Free();
				m_GdiPlus.Finalize();
			}

			m_Font.Destroy();
		}
		return TRUE;
	}
	return FALSE;
}
