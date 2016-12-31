#ifndef TVTEST_DIRECTWRITE_OPTIONS_DIALOG_H
#define TVTEST_DIRECTWRITE_OPTIONS_DIALOG_H


#include "Dialog.h"
#include "TextDrawClient.h"


namespace TVTest
{

	class CDirectWriteOptionsDialog : public CBasicDialog
	{
	public:
		class CRenderingTester
		{
		public:
			virtual ~CRenderingTester() {}
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
		CRenderingTester *m_pRenderingTester;
	};

}	// namespace TVTest


#endif
