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
	, m_pRenderingTester(nullptr)
{
}


bool CDirectWriteOptionsDialog::Show(HWND hwndOwner)
{
	return ShowDialog(hwndOwner,
					  GetAppClass().GetResourceInstance(),
					  MAKEINTRESOURCE(IDD_DIRECTWRITEOPTIONS))==IDOK;
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
				TEXT("����"),
				TEXT("�A���`�G�C���A�V���O�Ȃ�"),
				TEXT("GDI�݊�(2�l�݊�)"),
				TEXT("GDI�݊�"),
				TEXT("�����A���`�G�C���A�V���O"),
				TEXT("����/�����A���`�G�C���A�V���O"),
				TEXT("�A�E�g���C��"),
			};
			fEnable = (m_pParams->Mask & CDirectWriteRenderer::RenderingParams::PARAM_RENDERING_MODE) != 0;
			DlgCheckBox_Check(hDlg, IDC_DIRECTWRITEOPTIONS_RENDERINGMODE_ENABLE, fEnable);
			EnableDlgItem(hDlg, IDC_DIRECTWRITEOPTIONS_RENDERINGMODE, fEnable);
			SetComboBoxList(hDlg, IDC_DIRECTWRITEOPTIONS_RENDERINGMODE,
							RenderingModeList, lengthof(RenderingModeList));
			DlgComboBox_SetCurSel(hDlg, IDC_DIRECTWRITEOPTIONS_RENDERINGMODE,
								  m_pParams->RenderingMode);

			fEnable = (m_pParams->Mask & CDirectWriteRenderer::RenderingParams::PARAM_GAMMA) != 0;
			DlgCheckBox_Check(hDlg, IDC_DIRECTWRITEOPTIONS_GAMMA_ENABLE, fEnable);
			EnableDlgItems(hDlg,
						   IDC_DIRECTWRITEOPTIONS_GAMMA,
						   IDC_DIRECTWRITEOPTIONS_GAMMA_RANGE,
						   fEnable);
			SetItemFloatValue(IDC_DIRECTWRITEOPTIONS_GAMMA, m_pParams->Gamma);

			fEnable = (m_pParams->Mask & CDirectWriteRenderer::RenderingParams::PARAM_ENHANCED_CONTRAST) != 0;
			DlgCheckBox_Check(hDlg, IDC_DIRECTWRITEOPTIONS_ENHANCEDCONTRAST_ENABLE, fEnable);
			EnableDlgItems(hDlg,
						   IDC_DIRECTWRITEOPTIONS_ENHANCEDCONTRAST,
						   IDC_DIRECTWRITEOPTIONS_ENHANCEDCONTRAST_RANGE,
						   fEnable);
			SetItemFloatValue(IDC_DIRECTWRITEOPTIONS_ENHANCEDCONTRAST, m_pParams->EnhancedContrast);

			fEnable = (m_pParams->Mask & CDirectWriteRenderer::RenderingParams::PARAM_CLEARTYPE_LEVEL) != 0;
			DlgCheckBox_Check(hDlg, IDC_DIRECTWRITEOPTIONS_CLEARTYPELEVEL_ENABLE, fEnable);
			EnableDlgItems(hDlg,
						   IDC_DIRECTWRITEOPTIONS_CLEARTYPELEVEL,
						   IDC_DIRECTWRITEOPTIONS_CLEARTYPELEVEL_RANGE,
						   fEnable);
			SetItemFloatValue(IDC_DIRECTWRITEOPTIONS_CLEARTYPELEVEL, m_pParams->ClearTypeLevel);

			static const LPCTSTR PixelGeometryList[] = {
				TEXT("�t���b�g"),
				TEXT("RGB"),
				TEXT("BGR"),
			};
			fEnable = (m_pParams->Mask & CDirectWriteRenderer::RenderingParams::PARAM_PIXEL_GEOMETRY) != 0;
			DlgCheckBox_Check(hDlg, IDC_DIRECTWRITEOPTIONS_PIXELGEOMETRY_ENABLE,fEnable);
			EnableDlgItem(hDlg, IDC_DIRECTWRITEOPTIONS_PIXELGEOMETRY,fEnable);
			SetComboBoxList(hDlg,IDC_DIRECTWRITEOPTIONS_PIXELGEOMETRY,
							PixelGeometryList, lengthof(PixelGeometryList));
			DlgComboBox_SetCurSel(hDlg, IDC_DIRECTWRITEOPTIONS_PIXELGEOMETRY,
								  m_pParams->PixelGeometry);

			ShowDlgItem(hDlg, IDC_DIRECTWRITEOPTIONS_TEST, m_pRenderingTester != nullptr);

			m_TextDrawClient.Initialize(CTextDrawClient::ENGINE_DIRECTWRITE, hDlg);
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

			::FillRect(pdis->hDC, &pdis->rcItem,
					   static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)));

			m_TextDrawClient.InitializeTextDraw(&TextDraw);
			TextDraw.Begin(pdis->hDC, pdis->rcItem);
			TextDraw.SetFont(m_Font);
			TextDraw.SetTextColor(RGB(0, 0, 0));
			TextDraw.Draw(L"ABCabc123�����A�@�����T��\U0002a6a5",
						  pdis->rcItem,
						  pdis->rcItem.bottom - pdis->rcItem.top,
						  CTextDraw::DRAW_FLAG_ALIGN_HORZ_CENTER |
						  CTextDraw::DRAW_FLAG_ALIGN_VERT_CENTER);
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
	pParams->Mask = 0;
	if (DlgCheckBox_IsChecked(m_hDlg, IDC_DIRECTWRITEOPTIONS_RENDERINGMODE_ENABLE))
		pParams->Mask |= CDirectWriteRenderer::RenderingParams::PARAM_RENDERING_MODE;
	if (DlgCheckBox_IsChecked(m_hDlg, IDC_DIRECTWRITEOPTIONS_GAMMA_ENABLE))
		pParams->Mask |= CDirectWriteRenderer::RenderingParams::PARAM_GAMMA;
	if (DlgCheckBox_IsChecked(m_hDlg, IDC_DIRECTWRITEOPTIONS_ENHANCEDCONTRAST_ENABLE))
		pParams->Mask |= CDirectWriteRenderer::RenderingParams::PARAM_ENHANCED_CONTRAST;
	if (DlgCheckBox_IsChecked(m_hDlg, IDC_DIRECTWRITEOPTIONS_CLEARTYPELEVEL_ENABLE))
		pParams->Mask |= CDirectWriteRenderer::RenderingParams::PARAM_CLEARTYPE_LEVEL;
	if (DlgCheckBox_IsChecked(m_hDlg, IDC_DIRECTWRITEOPTIONS_PIXELGEOMETRY_ENABLE))
		pParams->Mask |= CDirectWriteRenderer::RenderingParams::PARAM_PIXEL_GEOMETRY;
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

	StdUtil::snprintf(szText, lengthof(szText), TEXT("%.2f"), Value);
	::SetDlgItemText(m_hDlg, ID, szText);
}


float CDirectWriteOptionsDialog::GetItemFloatValue(int ID)
{
	TCHAR szText[64];

	::GetDlgItemText(m_hDlg, ID, szText, lengthof(szText));
	return static_cast<float>(std::_tcstod(szText, NULL));
}


}	// namespace TVTest
