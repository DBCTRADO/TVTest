#include "stdafx.h"
#include "TVTest.h"
#include "UISkin.h"
#include "MessageDialog.h"
#include "Common/DebugDef.h"


static CMessageDialog MessageDialog;




CUISkin::CUISkin()
	: m_pCore(NULL)
	, m_fWheelChannelChanging(false)
{
}


int CUISkin::ShowMessage(LPCTSTR pszText, LPCTSTR pszCaption, UINT Type) const
{
	return ::MessageBox(GetVideoHostWindow(), pszText, pszCaption, Type);
}


void CUISkin::ShowErrorMessage(LPCTSTR pszText) const
{
	MessageDialog.Show(GetVideoHostWindow(), CMessageDialog::TYPE_WARNING, pszText);
}


void CUISkin::ShowErrorMessage(
	const LibISDB::ErrorHandler *pErrorHandler, LPCTSTR pszTitle) const
{
	TVTest::String Text;

	Text = pErrorHandler->GetLastErrorText();
	if (!IsStringEmpty(pErrorHandler->GetLastErrorAdvise())) {
		if (!Text.empty())
			Text += TEXT('\n');
		Text += pErrorHandler->GetLastErrorAdvise();
	}

	MessageDialog.Show(
		GetVideoHostWindow(), CMessageDialog::TYPE_WARNING, Text.c_str(),
		pszTitle, pErrorHandler->GetLastErrorSystemMessage());
}


void CUISkin::SetWheelChannelChanging(bool fChanging, DWORD Delay)
{
	if (fChanging) {
		if (Delay > 0)
			::SetTimer(GetMainWindow(), TIMER_ID_WHEELCHANNELCHANGE, Delay, NULL);
		m_fWheelChannelChanging = true;
	} else if (m_fWheelChannelChanging) {
		::KillTimer(GetMainWindow(), TIMER_ID_WHEELCHANNELCHANGE);
		m_fWheelChannelChanging = false;
	}
}
