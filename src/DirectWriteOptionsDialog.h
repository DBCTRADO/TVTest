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


#ifndef TVTEST_DIRECTWRITE_OPTIONS_DIALOG_H
#define TVTEST_DIRECTWRITE_OPTIONS_DIALOG_H


#include "Dialog.h"
#include "TextDrawClient.h"


namespace TVTest
{

	class CDirectWriteOptionsDialog
		: public CBasicDialog
	{
	public:
		class CRenderingTester
		{
		public:
			virtual ~CRenderingTester() = default;
			virtual void Apply(const CDirectWriteRenderer::RenderingParams &Params) = 0;
			virtual void Reset() = 0;
		};

		CDirectWriteOptionsDialog(
			CDirectWriteRenderer::RenderingParams *pParams,
			const LOGFONT &Font);
		bool Show(HWND hwndOwner) override;
		void SetRenderingTester(CRenderingTester *pTester);

	private:
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		void GetRenderingParams(CDirectWriteRenderer::RenderingParams *pParams);
		void UpdatePreview();
		void SetItemFloatValue(int ID, float Value);
		float GetItemFloatValue(int ID);

		CDirectWriteRenderer::RenderingParams *m_pParams;
		LOGFONT m_Font;
		CTextDrawClient m_TextDrawClient;
		CRenderingTester *m_pRenderingTester = nullptr;
	};

}	// namespace TVTest


#endif
