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
#include "AppMain.h"
#include "DirectWriteOptionsDialog.h"
#include "DialogUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CDirectWriteOptionsDialog::CDirectWriteOptionsDialog(
	CDirectWriteRenderer::RenderingParams *pParams,
	const LOGFONT &Font)
	: m_pParams(pParams)
	, m_Font(Font)
{
}


bool CDirectWriteOptionsDialog::Show(HWND hwndOwner)
{
	return ShowDialog(
		hwndOwner,
		GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE(IDD_DIRECTWRITEOPTIONS)) == IDOK;
}


void CDirectWriteOptionsDialog::SetRenderingTester(CRenderingTester *pTester)
{
	m_pRenderingTester = pTester;
}


INT_PTR CDirectWriteOptionsDialog::DlgProc(
	HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			bool fEnable;

			static const LPCTSTR RenderingModeList[] = {
				TEXT("自動"),
				TEXT("アンチエイリアシングなし"),
				TEXT("GDI互換(2値互換)"),
				TEXT("GDI互換"),
				TEXT("水平アンチエイリアシング"),
				TEXT("水平/垂直アンチエイリアシング"),
				TEXT("アウトライン"),
			};
			fEnable = !!(m_pParams->Mask & CDirectWriteRenderer::RenderingParams::ParamFlag::RenderingMode);
			DlgCheckBox_Check(hDlg, IDC_DIRECTWRITEOPTIONS_RENDERINGMODE_ENABLE, fEnable);
			EnableDlgItem(hDlg, IDC_DIRECTWRITEOPTIONS_RENDERINGMODE, fEnable);
			SetComboBoxList(
				hDlg, IDC_DIRECTWRITEOPTIONS_RENDERINGMODE,
				RenderingModeList, lengthof(RenderingModeList));
			DlgComboBox_SetCurSel(
				hDlg, IDC_DIRECTWRITEOPTIONS_RENDERINGMODE,
				m_pParams->RenderingMode);

			fEnable = !!(m_pParams->Mask & CDirectWriteRenderer::RenderingParams::ParamFlag::Gamma);
			DlgCheckBox_Check(hDlg, IDC_DIRECTWRITEOPTIONS_GAMMA_ENABLE, fEnable);
			EnableDlgItems(
				hDlg,
				IDC_DIRECTWRITEOPTIONS_GAMMA,
				IDC_DIRECTWRITEOPTIONS_GAMMA_RANGE,
				fEnable);
			SetItemFloatValue(IDC_DIRECTWRITEOPTIONS_GAMMA, m_pParams->Gamma);

			fEnable = !!(m_pParams->Mask & CDirectWriteRenderer::RenderingParams::ParamFlag::EnhancedContrast);
			DlgCheckBox_Check(hDlg, IDC_DIRECTWRITEOPTIONS_ENHANCEDCONTRAST_ENABLE, fEnable);
			EnableDlgItems(
				hDlg,
				IDC_DIRECTWRITEOPTIONS_ENHANCEDCONTRAST,
				IDC_DIRECTWRITEOPTIONS_ENHANCEDCONTRAST_RANGE,
				fEnable);
			SetItemFloatValue(IDC_DIRECTWRITEOPTIONS_ENHANCEDCONTRAST, m_pParams->EnhancedContrast);

			fEnable = !!(m_pParams->Mask & CDirectWriteRenderer::RenderingParams::ParamFlag::ClearTypeLevel);
			DlgCheckBox_Check(hDlg, IDC_DIRECTWRITEOPTIONS_CLEARTYPELEVEL_ENABLE, fEnable);
			EnableDlgItems(
				hDlg,
				IDC_DIRECTWRITEOPTIONS_CLEARTYPELEVEL,
				IDC_DIRECTWRITEOPTIONS_CLEARTYPELEVEL_RANGE,
				fEnable);
			SetItemFloatValue(IDC_DIRECTWRITEOPTIONS_CLEARTYPELEVEL, m_pParams->ClearTypeLevel);

			static const LPCTSTR PixelGeometryList[] = {
				TEXT("フラット"),
				TEXT("RGB"),
				TEXT("BGR"),
			};
			fEnable = !!(m_pParams->Mask & CDirectWriteRenderer::RenderingParams::ParamFlag::PixelGeometry);
			DlgCheckBox_Check(hDlg, IDC_DIRECTWRITEOPTIONS_PIXELGEOMETRY_ENABLE, fEnable);
			EnableDlgItem(hDlg, IDC_DIRECTWRITEOPTIONS_PIXELGEOMETRY, fEnable);
			SetComboBoxList(
				hDlg, IDC_DIRECTWRITEOPTIONS_PIXELGEOMETRY,
				PixelGeometryList, lengthof(PixelGeometryList));
			DlgComboBox_SetCurSel(
				hDlg, IDC_DIRECTWRITEOPTIONS_PIXELGEOMETRY,
				m_pParams->PixelGeometry);

			ShowDlgItem(hDlg, IDC_DIRECTWRITEOPTIONS_TEST, m_pRenderingTester != nullptr);

			m_TextDrawClient.Initialize(CTextDrawClient::TextDrawEngine::DirectWrite, hDlg);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_DIRECTWRITEOPTIONS_RENDERINGMODE_ENABLE:
			EnableDlgItemSyncCheckBox(
				hDlg,
				IDC_DIRECTWRITEOPTIONS_RENDERINGMODE,
				IDC_DIRECTWRITEOPTIONS_RENDERINGMODE_ENABLE);
			UpdatePreview();
			return TRUE;

		case IDC_DIRECTWRITEOPTIONS_RENDERINGMODE:
			if (HIWORD(wParam) == CBN_SELCHANGE)
				UpdatePreview();
			return TRUE;

		case IDC_DIRECTWRITEOPTIONS_GAMMA_ENABLE:
			EnableDlgItemsSyncCheckBox(
				hDlg,
				IDC_DIRECTWRITEOPTIONS_GAMMA,
				IDC_DIRECTWRITEOPTIONS_GAMMA_RANGE,
				IDC_DIRECTWRITEOPTIONS_GAMMA_ENABLE);
			UpdatePreview();
			return TRUE;

		case IDC_DIRECTWRITEOPTIONS_GAMMA:
			if (HIWORD(wParam) == EN_CHANGE)
				UpdatePreview();
			return TRUE;

		case IDC_DIRECTWRITEOPTIONS_ENHANCEDCONTRAST_ENABLE:
			EnableDlgItemsSyncCheckBox(
				hDlg,
				IDC_DIRECTWRITEOPTIONS_ENHANCEDCONTRAST,
				IDC_DIRECTWRITEOPTIONS_ENHANCEDCONTRAST_RANGE,
				IDC_DIRECTWRITEOPTIONS_ENHANCEDCONTRAST_ENABLE);
			UpdatePreview();
			return TRUE;

		case IDC_DIRECTWRITEOPTIONS_ENHANCEDCONTRAST:
			if (HIWORD(wParam) == EN_CHANGE)
				UpdatePreview();
			return TRUE;

		case IDC_DIRECTWRITEOPTIONS_CLEARTYPELEVEL_ENABLE:
			EnableDlgItemsSyncCheckBox(
				hDlg,
				IDC_DIRECTWRITEOPTIONS_CLEARTYPELEVEL,
				IDC_DIRECTWRITEOPTIONS_CLEARTYPELEVEL_RANGE,
				IDC_DIRECTWRITEOPTIONS_CLEARTYPELEVEL_ENABLE);
			UpdatePreview();
			return TRUE;

		case IDC_DIRECTWRITEOPTIONS_CLEARTYPELEVEL:
			if (HIWORD(wParam) == EN_CHANGE)
				UpdatePreview();
			return TRUE;

		case IDC_DIRECTWRITEOPTIONS_PIXELGEOMETRY_ENABLE:
			EnableDlgItemSyncCheckBox(
				hDlg,
				IDC_DIRECTWRITEOPTIONS_PIXELGEOMETRY,
				IDC_DIRECTWRITEOPTIONS_PIXELGEOMETRY_ENABLE);
			UpdatePreview();
			return TRUE;

		case IDC_DIRECTWRITEOPTIONS_PIXELGEOMETRY:
			if (HIWORD(wParam) == CBN_SELCHANGE)
				UpdatePreview();
			return TRUE;

		case IDC_DIRECTWRITEOPTIONS_TEST:
			if (m_pRenderingTester != nullptr) {
				CDirectWriteRenderer::RenderingParams Params;

				GetRenderingParams(&Params);
				m_pRenderingTester->Apply(Params);
			}
			return TRUE;

		case IDOK:
			GetRenderingParams(m_pParams);
			::EndDialog(hDlg, IDOK);
			return TRUE;

		case IDCANCEL:
			if (m_pRenderingTester != nullptr)
				m_pRenderingTester->Reset();
			::EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		return TRUE;

	case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *pdis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
			CDirectWriteRenderer::RenderingParams Params;
			CTextDraw TextDraw;

			GetRenderingParams(&Params);
			m_TextDrawClient.SetDirectWriteRenderingParams(Params);

			::FillRect(
				pdis->hDC, &pdis->rcItem,
				static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)));

			m_TextDrawClient.InitializeTextDraw(&TextDraw);
			TextDraw.Begin(pdis->hDC, pdis->rcItem);
			TextDraw.SetFont(m_Font);
			TextDraw.SetTextColor(RGB(0, 0, 0));
			TextDraw.Draw(
				L"ABCabc123あぁアァ漢字鬱贔\U0002a6a5\U0001f211\U0001f216",
				pdis->rcItem,
				pdis->rcItem.bottom - pdis->rcItem.top,
				CTextDraw::DrawFlag::Align_HorzCenter |
				CTextDraw::DrawFlag::Align_VertCenter);
			TextDraw.End();
		}
		return TRUE;

	case WM_DESTROY:
		m_TextDrawClient.Finalize();
		return TRUE;
	}

	return FALSE;
}


void CDirectWriteOptionsDialog::GetRenderingParams(CDirectWriteRenderer::RenderingParams *pParams)
{
	pParams->Mask = CDirectWriteRenderer::RenderingParams::ParamFlag::None;
	if (DlgCheckBox_IsChecked(m_hDlg, IDC_DIRECTWRITEOPTIONS_RENDERINGMODE_ENABLE))
		pParams->Mask |= CDirectWriteRenderer::RenderingParams::ParamFlag::RenderingMode;
	if (DlgCheckBox_IsChecked(m_hDlg, IDC_DIRECTWRITEOPTIONS_GAMMA_ENABLE))
		pParams->Mask |= CDirectWriteRenderer::RenderingParams::ParamFlag::Gamma;
	if (DlgCheckBox_IsChecked(m_hDlg, IDC_DIRECTWRITEOPTIONS_ENHANCEDCONTRAST_ENABLE))
		pParams->Mask |= CDirectWriteRenderer::RenderingParams::ParamFlag::EnhancedContrast;
	if (DlgCheckBox_IsChecked(m_hDlg, IDC_DIRECTWRITEOPTIONS_CLEARTYPELEVEL_ENABLE))
		pParams->Mask |= CDirectWriteRenderer::RenderingParams::ParamFlag::ClearTypeLevel;
	if (DlgCheckBox_IsChecked(m_hDlg, IDC_DIRECTWRITEOPTIONS_PIXELGEOMETRY_ENABLE))
		pParams->Mask |= CDirectWriteRenderer::RenderingParams::ParamFlag::PixelGeometry;
	pParams->RenderingMode =
		static_cast<DWRITE_RENDERING_MODE>(
			DlgComboBox_GetCurSel(m_hDlg, IDC_DIRECTWRITEOPTIONS_RENDERINGMODE));
	pParams->Gamma =
		GetItemFloatValue(IDC_DIRECTWRITEOPTIONS_GAMMA);
	pParams->EnhancedContrast =
		GetItemFloatValue(IDC_DIRECTWRITEOPTIONS_ENHANCEDCONTRAST);
	pParams->ClearTypeLevel =
		GetItemFloatValue(IDC_DIRECTWRITEOPTIONS_CLEARTYPELEVEL);
	pParams->PixelGeometry =
		static_cast<DWRITE_PIXEL_GEOMETRY>(
			DlgComboBox_GetCurSel(m_hDlg, IDC_DIRECTWRITEOPTIONS_PIXELGEOMETRY));
}


void CDirectWriteOptionsDialog::UpdatePreview()
{
	InvalidateDlgItem(m_hDlg, IDC_DIRECTWRITEOPTIONS_PREVIEW);
}


void CDirectWriteOptionsDialog::SetItemFloatValue(int ID, float Value)
{
	TCHAR szText[64];

	StringFormat(szText, TEXT("{:.2f}"), Value);
	::SetDlgItemText(m_hDlg, ID, szText);
}


float CDirectWriteOptionsDialog::GetItemFloatValue(int ID)
{
	TCHAR szText[64];

	::GetDlgItemText(m_hDlg, ID, szText, lengthof(szText));
	return std::_tcstof(szText, nullptr);
}


}	// namespace TVTest
