#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "MiscDialog.h"
#include "DialogUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


static const size_t MAX_INFO_TEXT=256;




CAboutDialog::CAboutDialog()
	: m_fDrawLogo(false)
{
}


CAboutDialog::~CAboutDialog()
{
	Destroy();
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
		COLORREF LinkText;
	} Colors = {
		RGB(192,213,245),RGB(160,192,240),
		RGB(0,0,0),
		RGB(200,214,255),
		RGB(224,232,255),RGB(240,244,255),
		RGB(64,64,64),
		RGB(0,0,255)
	};

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			HWND hwndHeader=::GetDlgItem(hDlg,IDC_ABOUT_HEADER);
			HWND hwndInfo=::GetDlgItem(hDlg,IDC_ABOUT_INFO);
			HWND hwndLogo=::GetDlgItem(hDlg,IDC_ABOUT_LOGO);
			HWND hwndLink=::GetDlgItem(hDlg,IDC_ABOUT_LINK);

			LOGFONT lf;
			DrawUtil::GetSystemFont(DrawUtil::FONT_MESSAGE,&lf);
			m_Font.Create(&lf);
			lf.lfUnderline=1;
			m_LinkFont.Create(&lf);

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
			DrawUtil::SelectObject(hdc,m_LinkFont);
			::GetWindowText(hwndLink,szText,lengthof(szText));
			RECT rcLinkText={0,0,0,0};
			::DrawText(hdc,szText,-1,&rcLinkText,DT_CALCRECT | DT_NOPREFIX);
			::SelectObject(hdc,hfontOld);
			::ReleaseDC(hDlg,hdc);
			if (rcText.right<rcLinkText.right)
				rcText.right=rcLinkText.right;

			RECT rcInfo;
			::GetWindowRect(hwndInfo,&rcInfo);
			::OffsetRect(&rcInfo,-rcInfo.left,-rcInfo.top);
			RECT rcLink;
			GetDlgItemRect(hDlg,IDC_ABOUT_LINK,&rcLink);
			RECT rcLogo;
			::GetWindowRect(hwndLogo,&rcLogo);

			if (rcInfo.bottom<rcText.bottom || rcInfo.right<rcText.right
					|| rcLink.bottom-rcLink.top<rcLinkText.bottom) {
				int Width=max(rcInfo.right,rcText.right);
				int Height=max(rcInfo.bottom,rcText.bottom);
				int XDiff=Width-rcInfo.right;
				int YDiff=Height-rcInfo.bottom;

				::SetWindowPos(hwndInfo,NULL,0,0,Width,Height,
							   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
				::OffsetRect(&rcLink,0,YDiff);
				rcLink.right+=XDiff;
				YDiff+=rcLinkText.bottom-(rcLink.bottom-rcLink.top);
				RECT rcHeader;
				::GetWindowRect(hwndHeader,&rcHeader);
				::SetWindowPos(hwndHeader,NULL,0,0,Width,rcHeader.bottom-rcHeader.top,
							   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
				RECT rcDialog;
				::GetWindowRect(hDlg,&rcDialog);
				::SetWindowPos(hDlg,NULL,0,0,
							   (rcDialog.right-rcDialog.left)+XDiff,
							   (rcDialog.bottom-rcDialog.top)+YDiff,
							   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
				RECT rcClient;
				::GetClientRect(hDlg,&rcClient);
				::SetWindowPos(hwndLogo,NULL,0,0,
							   rcLogo.right-rcLogo.left,rcClient.bottom,
							   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
			}

			::SetWindowPos(hwndLink,NULL,
						   rcLink.left+((rcLink.right-rcLink.left)-rcLinkText.right)/2,
						   rcLink.top,
						   rcLinkText.right,
						   rcLinkText.bottom,
						   SWP_NOZORDER | SWP_NOACTIVATE);

			::SetRect(&rcLogo,rcLogo.right-rcLogo.left,0,0,0);
			if (!Util::OS::IsWindows8OrLater()
					&& m_AeroGlass.ApplyAeroGlass(hDlg,&rcLogo)) {
				m_fDrawLogo=true;
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
				|| reinterpret_cast<HWND>(lParam)==::GetDlgItem(hDlg,IDC_ABOUT_INFO)
				|| reinterpret_cast<HWND>(lParam)==::GetDlgItem(hDlg,IDC_ABOUT_LINK))
			return reinterpret_cast<INT_PTR>(::GetStockObject(NULL_BRUSH));
		break;

	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT pdis=reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
			int OldBkMode=::SetBkMode(pdis->hDC,TRANSPARENT);
			COLORREF OldTextColor=::GetTextColor(pdis->hDC);;
			HFONT hfontOld=(HFONT)::GetCurrentObject(pdis->hDC,OBJ_FONT);
			TCHAR szText[MAX_INFO_TEXT];

			if (pdis->CtlID==IDC_ABOUT_HEADER) {
				DrawUtil::SelectObject(pdis->hDC,m_Font);
				::SetTextColor(pdis->hDC,Colors.HeaderText);
				::GetDlgItemText(hDlg,IDC_ABOUT_HEADER,szText,lengthof(szText));
				::DrawText(pdis->hDC,szText,-1,&pdis->rcItem,
						   DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
			} else if (pdis->CtlID==IDC_ABOUT_INFO) {
				DrawUtil::SelectObject(pdis->hDC,m_Font);
				::SetTextColor(pdis->hDC,Colors.InfoText);
				::GetDlgItemText(hDlg,IDC_ABOUT_INFO,szText,lengthof(szText));
				::DrawText(pdis->hDC,szText,-1,&pdis->rcItem,DT_CENTER | DT_NOPREFIX);
			} else if (pdis->CtlID==IDC_ABOUT_LINK) {
				DrawUtil::SelectObject(pdis->hDC,m_LinkFont);
				::SetTextColor(pdis->hDC,Colors.LinkText);
				::GetDlgItemText(hDlg,IDC_ABOUT_LINK,szText,lengthof(szText));
				::DrawText(pdis->hDC,szText,-1,&pdis->rcItem,DT_CENTER | DT_NOPREFIX);
			}

			::SelectObject(pdis->hDC,hfontOld);
			::SetTextColor(pdis->hDC,OldTextColor);
			::SetBkMode(pdis->hDC,OldBkMode);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ABOUT_LINK:
			if (HIWORD(wParam)==STN_CLICKED) {
				TCHAR szText[MAX_INFO_TEXT];

				if (::GetDlgItemText(hDlg,IDC_ABOUT_LINK,szText,lengthof(szText))>0)
					::ShellExecute(NULL,TEXT("open"),szText,NULL,NULL,SW_SHOW);
			}
			return TRUE;

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

			if (m_fDrawLogo) {
				TVTest::Graphics::CCanvas Canvas(ps.hdc);

				Canvas.Clear(0,0,0,0);
				Canvas.DrawImage(&m_LogoImage,
								 (rcLogo.right-m_LogoImage.GetWidth())/2,
								 (rcLogo.bottom-m_LogoImage.GetHeight())/2);
				rc=rcClient;
				rc.bottom=rcHeader.bottom;
				Canvas.FillGradient(Colors.Header1,Colors.Header2,rc,
									TVTest::Graphics::GRADIENT_DIRECTION_VERT);
				rc.top=rc.bottom;
				rc.bottom=rc.top+8;
				Canvas.FillGradient(Colors.HeaderShadow,Colors.Info1,rc,
									TVTest::Graphics::GRADIENT_DIRECTION_VERT);
				rc.top=rc.bottom;
				rc.bottom=rcClient.bottom;
				Canvas.FillGradient(Colors.Info1,Colors.Info2,rc,
									TVTest::Graphics::GRADIENT_DIRECTION_VERT);
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

	case WM_SETCURSOR:
		if ((HWND)wParam==::GetDlgItem(hDlg,IDC_ABOUT_LINK)) {
			::SetCursor(GetAppClass().UICore.GetLinkCursor());
			::SetWindowLongPtr(hDlg,DWLP_MSGRESULT,TRUE);
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			HBITMAP hbm=reinterpret_cast<HBITMAP>(::SendDlgItemMessage(hDlg,IDC_ABOUT_LOGO,
				STM_SETIMAGE,IMAGE_BITMAP,reinterpret_cast<LPARAM>((HBITMAP)NULL)));

			if (hbm!=NULL) {
				::DeleteObject(hbm);
			} else {
				m_LogoImage.Free();
			}

			m_Font.Destroy();
			m_LinkFont.Destroy();
		}
		return TRUE;
	}

	return FALSE;
}
