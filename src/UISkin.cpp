/*
  TVTest
  Copyright(c) 2008-2020 DBCTRADO

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "stdafx.h"
#include "TVTest.h"
#include "UISkin.h"
#include "MessageDialog.h"
#include "Common/DebugDef.h"


namespace TVTest
{


static CMessageDialog MessageDialog;




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
	String Text = pErrorHandler->GetLastErrorText();

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
