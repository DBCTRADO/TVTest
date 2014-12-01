#ifndef INITIAL_SETTINGS_H
#define INITIAL_SETTINGS_H


#include "DriverManager.h"
#include "CoreEngine.h"
#include "Dialog.h"
#include "Aero.h"
#include "Graphics.h"


class CInitialSettings : public CBasicDialog
{
public:
	enum { MAX_DECODER_NAME=128 };

	CInitialSettings(const CDriverManager *pDriverManager);
	~CInitialSettings();

// CBasicDialog
	bool Show(HWND hwndOwner) override;

	LPCTSTR GetDriverFileName() const { return m_szDriverFileName; }
	bool GetDriverFileName(LPTSTR pszFileName,int MaxLength) const;
	LPCTSTR GetMpeg2DecoderName() const { return m_Mpeg2DecoderName.c_str(); }
	LPCTSTR GetH264DecoderName() const { return m_H264DecoderName.c_str(); }
	LPCTSTR GetH265DecoderName() const { return m_H265DecoderName.c_str(); }
	CVideoRenderer::RendererType GetVideoRenderer() const { return m_VideoRenderer; }
	LPCTSTR GetRecordFolder() const { return m_szRecordFolder; }

private:
	const CDriverManager *m_pDriverManager;
	TCHAR m_szDriverFileName[MAX_PATH];
	TVTest::String m_Mpeg2DecoderName;
	TVTest::String m_H264DecoderName;
	TVTest::String m_H265DecoderName;
	CVideoRenderer::RendererType m_VideoRenderer;
	TCHAR m_szRecordFolder[MAX_PATH];
	CAeroGlass m_AeroGlass;
	TVTest::Graphics::CImage m_LogoImage;
	bool m_fDrawLogo;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	void InitDecoderList(int ID,const GUID &SubType,LPCTSTR pszDecoderName);
	void GetDecoderSetting(int ID,TVTest::String *pDecoderName) const;
};


#endif
