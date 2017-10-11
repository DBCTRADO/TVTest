#ifndef TVTEST_INITIAL_SETTINGS_H
#define TVTEST_INITIAL_SETTINGS_H


#include "DriverManager.h"
#include "CoreEngine.h"
#include "Dialog.h"
#include "Aero.h"
#include "Graphics.h"


namespace TVTest
{

	class CInitialSettings
		: public CBasicDialog
	{
	public:
		static constexpr size_t MAX_DECODER_NAME = 128;

		CInitialSettings(const CDriverManager *pDriverManager);
		~CInitialSettings();

	// CBasicDialog
		bool Show(HWND hwndOwner) override;

		LPCTSTR GetDriverFileName() const { return m_DriverFileName.c_str(); }
		LPCTSTR GetMpeg2DecoderName() const { return m_Mpeg2DecoderName.c_str(); }
		LPCTSTR GetH264DecoderName() const { return m_H264DecoderName.c_str(); }
		LPCTSTR GetH265DecoderName() const { return m_H265DecoderName.c_str(); }
		LibISDB::DirectShow::VideoRenderer::RendererType GetVideoRenderer() const { return m_VideoRenderer; }
		LPCTSTR GetRecordFolder() const { return m_RecordFolder.c_str(); }

	private:
		const CDriverManager *m_pDriverManager;
		CFilePath m_DriverFileName;
		String m_Mpeg2DecoderName;
		String m_H264DecoderName;
		String m_H265DecoderName;
		LibISDB::DirectShow::VideoRenderer::RendererType m_VideoRenderer;
		String m_RecordFolder;
		CAeroGlass m_AeroGlass;
		Graphics::CImage m_LogoImage;
		bool m_fDrawLogo;

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		void InitDecoderList(int ID, const GUID &SubType, LPCTSTR pszDecoderName);
		void GetDecoderSetting(int ID, String *pDecoderName) const;
	};

}	// namespace TVTest


#endif
