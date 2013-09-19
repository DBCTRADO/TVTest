#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "MessageDialog.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CMessageDialog::CMessageDialog()
{
}


CMessageDialog::~CMessageDialog()
{
}


void CMessageDialog::LogFontToCharFormat(const LOGFONT *plf,CHARFORMAT *pcf)
{
	HDC hdc=::GetDC(m_hDlg);
	CRichEditUtil::LogFontToCharFormat(hdc,plf,pcf);
	::ReleaseDC(m_hDlg,hdc);
}


CMessageDialog *CMessageDialog::GetThis(HWND hDlg)
{
	return static_cast<CMessageDialog*>(::GetProp(hDlg,TEXT("This")));
}


INT_PTR CALLBACK CMessageDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CMessageDialog *pThis=reinterpret_cast<CMessageDialog*>(lParam);

			::SetProp(hDlg,TEXT("This"),pThis);
			pThis->m_hDlg=hDlg;

			if (!pThis->m_Caption.IsEmpty())
				::SetWindowText(hDlg,pThis->m_Caption.Get());

			::SendDlgItemMessage(hDlg,IDC_ERROR_ICON,STM_SETICON,
				reinterpret_cast<WPARAM>(::LoadIcon(NULL,
					pThis->m_MessageType==TYPE_INFO?IDI_INFORMATION:
					pThis->m_MessageType==TYPE_WARNING?IDI_WARNING:IDI_ERROR)),0);
			::SendDlgItemMessage(hDlg,IDC_ERROR_MESSAGE,EM_SETBKGNDCOLOR,0,::GetSysColor(COLOR_WINDOW));

			const HWND hwndEdit=::GetDlgItem(hDlg,IDC_ERROR_MESSAGE);
			CHARFORMAT cf,cfBold;

			NONCLIENTMETRICS ncm;
#if WINVER<0x0600
			ncm.cbSize=sizeof(ncm);
#else
			ncm.cbSize=offsetof(NONCLIENTMETRICS,iPaddedBorderWidth);
#endif
			::SystemParametersInfo(SPI_GETNONCLIENTMETRICS,ncm.cbSize,&ncm,0);
			pThis->LogFontToCharFormat(&ncm.lfMessageFont,&cf);

			cfBold=cf;
			cfBold.dwMask|=CFM_BOLD;
			cfBold.dwEffects|=CFE_BOLD;
			if (!pThis->m_Title.IsEmpty()) {
				CRichEditUtil::AppendText(hwndEdit,pThis->m_Title.Get(),&cfBold);
				CRichEditUtil::AppendText(hwndEdit,TEXT("\n"),&cf);
			}
			if (!pThis->m_Text.IsEmpty()) {
				CRichEditUtil::AppendText(hwndEdit,pThis->m_Text.Get(),&cf);
			}
			if (!pThis->m_SystemMessage.IsEmpty()) {
				CRichEditUtil::AppendText(hwndEdit,TEXT("\n\nWindowsのエラーメッセージ :\n"),&cfBold);
				CRichEditUtil::AppendText(hwndEdit,pThis->m_SystemMessage.Get(),&cf);
			}
			const int MaxWidth=CRichEditUtil::GetMaxLineWidth(hwndEdit)+8;
			RECT rcEdit,rcIcon,rcDlg,rcClient,rcOK;
			::GetWindowRect(hwndEdit,&rcEdit);
			::OffsetRect(&rcEdit,-rcEdit.left,-rcEdit.top);
			::GetWindowRect(::GetDlgItem(hDlg,IDC_ERROR_ICON),&rcIcon);
			rcIcon.bottom-=rcIcon.top;
			if (rcEdit.bottom<rcIcon.bottom)
				rcEdit.bottom=rcIcon.bottom;
			::SetWindowPos(hwndEdit,NULL,0,0,MaxWidth,rcEdit.bottom,
						   SWP_NOMOVE | SWP_NOZORDER);
			::GetWindowRect(hDlg,&rcDlg);
			::GetClientRect(hDlg,&rcClient);
			GetDlgItemRect(hDlg,IDOK,&rcOK);
			const int Offset=MaxWidth-rcEdit.right;
			::SetWindowPos(::GetDlgItem(hDlg,IDOK),NULL,
						   rcOK.left+Offset,rcOK.top,0,0,
						   SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
			::SetWindowPos(hDlg,NULL,0,0,(rcDlg.right-rcDlg.left)+Offset,rcDlg.bottom-rcDlg.top,
						   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
			::SendMessage(hwndEdit,EM_SETEVENTMASK,0,ENM_REQUESTRESIZE | ENM_MOUSEEVENTS);
			::SendDlgItemMessage(hDlg,IDC_ERROR_MESSAGE,EM_REQUESTRESIZE,0,0);

			AdjustDialogPos(::GetParent(hDlg),hDlg);
		}
		return TRUE;

	/*
	case WM_SIZE:
		::SendDlgItemMessage(hDlg,IDC_ERROR_MESSAGE,EM_REQUESTRESIZE,0,0);
		return TRUE;
	*/

	case WM_PAINT:
		{
			CMessageDialog *pThis=GetThis(hDlg);
			PAINTSTRUCT ps;
			RECT rcEdit,rc;

			::BeginPaint(hDlg,&ps);
			::GetWindowRect(::GetDlgItem(hDlg,IDC_ERROR_MESSAGE),&rcEdit);
			MapWindowRect(NULL,hDlg,&rcEdit);
			::GetClientRect(hDlg,&rc);
			rc.bottom=rcEdit.bottom;
			::FillRect(ps.hdc,&rc,::GetSysColorBrush(COLOR_WINDOW));
			::EndPaint(hDlg,&ps);
		}
		return TRUE;

	case WM_CTLCOLORSTATIC:
		if (reinterpret_cast<HWND>(lParam)==::GetDlgItem(hDlg,IDC_ERROR_ICON))
			return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_WINDOW));
		break;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case EN_REQUESTRESIZE:
			{
				REQRESIZE *prr=reinterpret_cast<REQRESIZE*>(lParam);
				RECT rcEdit,rcDialog,rcClient,rcOK,rcIcon;
				int Width,Height,MinWidth;
				int XOffset,YOffset;

				::GetWindowRect(hDlg,&rcDialog);
				::GetClientRect(hDlg,&rcClient);
				::GetWindowRect(prr->nmhdr.hwndFrom,&rcEdit);
				GetDlgItemRect(hDlg,IDOK,&rcOK);
				MinWidth=(rcOK.right-rcOK.left)+(rcClient.right-rcOK.right)*2;
				Width=prr->rc.right-prr->rc.left;
				if (Width<MinWidth)
					Width=MinWidth;
				Height=prr->rc.bottom-prr->rc.top;
				::GetWindowRect(::GetDlgItem(hDlg,IDC_ERROR_ICON),&rcIcon);
				rcIcon.bottom-=rcIcon.top;
				if (Height<rcIcon.bottom)
					Height=rcIcon.bottom;
				if (Width==rcEdit.right-rcEdit.left
						&& Height==rcEdit.bottom-rcEdit.top)
					break;
				XOffset=Width-(rcEdit.right-rcEdit.left);
				YOffset=Height-(rcEdit.bottom-rcEdit.top);
				::SetWindowPos(prr->nmhdr.hwndFrom,NULL,0,0,Width,Height,
							   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
				::SetRect(&rcEdit,0,0,Width,Height);
				::SendDlgItemMessage(hDlg,IDC_ERROR_MESSAGE,EM_SETRECT,0,reinterpret_cast<LPARAM>(&rcEdit));
				rcDialog.right+=XOffset;
				rcDialog.bottom+=YOffset;
				::MoveWindow(hDlg,rcDialog.left,rcDialog.top,
							 rcDialog.right-rcDialog.left,
							 rcDialog.bottom-rcDialog.top,TRUE);
				::MoveWindow(::GetDlgItem(hDlg,IDOK),rcOK.left+XOffset,rcOK.top+YOffset,
							 rcOK.right-rcOK.left,rcOK.bottom-rcOK.top,TRUE);
			}
			return TRUE;

		case EN_MSGFILTER:
			if (reinterpret_cast<MSGFILTER*>(lParam)->msg==WM_RBUTTONDOWN) {
				HMENU hmenu;
				POINT pt;

				hmenu=::CreatePopupMenu();
				::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,IDC_ERROR_COPY,TEXT("コピー(&C)"));
				::GetCursorPos(&pt);
				::TrackPopupMenu(hmenu,TPM_RIGHTBUTTON,pt.x,pt.y,0,hDlg,NULL);
				::DestroyMenu(hmenu);
			}
			return TRUE;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ERROR_COPY:
			{
				HWND hwndEdit=::GetDlgItem(hDlg,IDC_ERROR_MESSAGE);

				if (::SendMessage(hwndEdit,EM_SELECTIONTYPE,0,0)==SEL_EMPTY) {
					CRichEditUtil::CopyAllText(hwndEdit);
				} else {
					::SendMessage(hwndEdit,WM_COPY,0,0);
				}
			}
			return TRUE;

		case IDOK:
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			CMessageDialog *pThis=GetThis(hDlg);

			pThis->m_Text.Clear();
			pThis->m_Title.Clear();
			pThis->m_SystemMessage.Clear();
			pThis->m_Caption.Clear();
			pThis->m_hDlg=NULL;
			::RemoveProp(hDlg,TEXT("This"));
		}
		return TRUE;
	}
	return FALSE;
}


bool CMessageDialog::Show(HWND hwndOwner,MessageType Type,LPCTSTR pszText,LPCTSTR pszTitle,LPCTSTR pszSystemMessage,LPCTSTR pszCaption)
{
	if (pszText==NULL && pszTitle==NULL && pszSystemMessage==NULL)
		return false;

	if (!m_RichEditUtil.LoadRichEditLib()) {
		TCHAR szMessage[1024];
		CStaticStringFormatter Formatter(szMessage,lengthof(szMessage));

		if (pszTitle!=NULL)
			Formatter.Append(pszTitle);
		if (pszText!=NULL) {
			if (!Formatter.IsEmpty())
				Formatter.Append(TEXT("\n"));
			Formatter.Append(pszText);
		}
		if (pszSystemMessage!=NULL) {
			if (!Formatter.IsEmpty())
				Formatter.Append(TEXT("\n\n"));
			Formatter.Append(TEXT("Windowsのエラーメッセージ:\n"));
			Formatter.Append(pszSystemMessage);
		}
		return ::MessageBox(hwndOwner,Formatter.GetString(),pszCaption,
							MB_OK |
							(Type==TYPE_INFO?MB_ICONINFORMATION:
							 Type==TYPE_WARNING?MB_ICONEXCLAMATION:
							 Type==TYPE_ERROR?MB_ICONSTOP:0))==IDOK;
	}

	m_Text.Set(pszText);
	m_Title.Set(pszTitle);
	m_SystemMessage.Set(pszSystemMessage);
	m_Caption.Set(pszCaption);
	m_MessageType=Type;
	return ::DialogBoxParam(GetAppClass().GetResourceInstance(),
							MAKEINTRESOURCE(IDD_ERROR),hwndOwner,DlgProc,
							reinterpret_cast<LPARAM>(this))==IDOK;
}
