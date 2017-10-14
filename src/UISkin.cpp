#include "stdafx.h"
#include "TVTest.h"
#include "UISkin.h"
#include "MessageDialog.h"
#include "Common/DebugDef.h"


namespace TVTest
{


static CMessageDialog MessageDialog;




CUISkin::CUISkin()
	: m_pCore(nullptr)
	, m_fWheelChannelChanging(false)
{
}


int CUISkin::ShowMessage(LPCTSTR pszText, LPCTSTR pszCaption, UINT Type) const
{
	return ::MessageBox(GetVideoHostWindow(), pszText, pszCaption, Type);
}


void CUISkin::ShowErrorMessage(LPCTSTR pszText) const
{
	MessageDialog.Show(GetVideoHostWindow(), CMessageDialog::MessageType::Warning, pszText);
}


void CUISkin::ShowErrorMessage(
	const LibISDB::ErrorHandler *pErrorHandler, LPCTSTR pszTitle) const
{
	String Text;

	Text = pErrorHandler->GetLastErrorText();
	if (!IsStringEmpty(pErrorHandler->GetLastErrorAdvise())) {
		if (!Text.empty())
			Text += TEXT('\n');
		Text += pErrorHandler->GetLastErrorAdvise();
	}

	MessageDialog.Show(
		GetVideoHostWindow(), CMessageDialog::MessageType::Warning, Text.c_str(),
		pszTitle, pErrorHandler->GetLastErrorSystemMessage());
}


void CUISkin::SetWheelChannelChanging(bool fChanging, DWORD Delay)
{
	if (fChanging) {
		BeginWheelChannelSelect(Delay);
		m_fWheelChannelChanging = true;
	} else if (m_fWheelChannelChanging) {
		EndWheelChannelSelect();
		m_fWheelChannelChanging = false;
	}
}


}	// namespace TVTest
