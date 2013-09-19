#include "stdafx.h"
#include "TVTest.h"
#include "UISkin.h"
#include "MessageDialog.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


static CMessageDialog MessageDialog;




CUISkin::CUISkin()
	: m_pCore(NULL)
	, m_fWheelChannelChanging(false)
{
}


CUISkin::~CUISkin()
{
}


int CUISkin::ShowMessage(LPCTSTR pszText,LPCTSTR pszCaption,UINT Type) const
{
	return ::MessageBox(GetVideoHostWindow(),pszText,pszCaption,Type);
}


void CUISkin::ShowErrorMessage(LPCTSTR pszText) const
{
	MessageDialog.Show(GetVideoHostWindow(),CMessageDialog::TYPE_WARNING,pszText);
}


void CUISkin::ShowErrorMessage(const CBonErrorHandler *pErrorHandler,
							   LPCTSTR pszTitle) const
{
	TCHAR szText[1024];

	pErrorHandler->FormatLastErrorText(szText,lengthof(szText));
	MessageDialog.Show(GetVideoHostWindow(),CMessageDialog::TYPE_WARNING,szText,
					   pszTitle,pErrorHandler->GetLastErrorSystemMessage());
}


void CUISkin::SetWheelChannelChanging(bool fChanging,DWORD Delay)
{
	if (fChanging) {
		if (Delay>0)
			::SetTimer(GetMainWindow(),TIMER_ID_WHEELCHANNELCHANGE,Delay,NULL);
		m_fWheelChannelChanging=true;
	} else if (m_fWheelChannelChanging) {
		::KillTimer(GetMainWindow(),TIMER_ID_WHEELCHANNELCHANGE);
		m_fWheelChannelChanging=false;
	}
}
