#ifndef GENERAL_OPTIONS_H
#define GENERAL_OPTIONS_H


#include "Options.h"
#include "CoreEngine.h"
#include "DirectShowFilter/VideoRenderer.h"


class CGeneralOptions : public COptions
{
public:
	enum DefaultDriverType {
		DEFAULT_DRIVER_NONE,
		DEFAULT_DRIVER_LAST,
		DEFAULT_DRIVER_CUSTOM
	};
	enum {
		MAX_VIDEO_DECODER_NAME=128
	};

	CGeneralOptions();
	~CGeneralOptions();

// COptions
	bool Apply(DWORD Flags) override;

// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;

// CBasicDialog
	bool Create(HWND hwndOwner) override;

// CGeneralOptions
	DefaultDriverType GetDefaultDriverType() const;
	LPCTSTR GetDefaultDriverName() const;
	bool SetDefaultDriverName(LPCTSTR pszDriverName);
	bool GetFirstDriverName(LPTSTR pszDriverName) const;
	LPCTSTR GetMpeg2DecoderName() const;
	bool SetMpeg2DecoderName(LPCTSTR pszDecoderName);
	LPCTSTR GetH264DecoderName() const;
	bool SetH264DecoderName(LPCTSTR pszDecoderName);
	LPCTSTR GetH265DecoderName() const;
	bool SetH265DecoderName(LPCTSTR pszDecoderName);
	CVideoRenderer::RendererType GetVideoRendererType() const;
	bool SetVideoRendererType(CVideoRenderer::RendererType Renderer);
	bool GetResident() const;
	bool GetKeepSingleTask() const;
	bool GetStandaloneProgramGuide() const { return m_fStandaloneProgramGuide; }

private:
// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	void SetVideoDecoderList(
		int ID,const GUID &SubType,BYTE StreamType,const TVTest::String &DecoderName);
	void GetVideoDecoderSetting(int ID,BYTE StreamType,TVTest::String *pDecoderName);

	enum {
		UPDATE_DECODER				= 0x00000001UL,
		UPDATE_RENDERER				= 0x00000002UL,
		UPDATE_RESIDENT				= 0x00000004UL,
		UPDATE_1SEGFALLBACK			= 0x00000008UL
	};

	TVTest::String m_BonDriverDirectory;
	DefaultDriverType m_DefaultDriverType;
	TVTest::String m_DefaultBonDriverName;
	TVTest::String m_LastBonDriverName;
	TVTest::String m_Mpeg2DecoderName;
	TVTest::String m_H264DecoderName;
	TVTest::String m_H265DecoderName;
	CVideoRenderer::RendererType m_VideoRendererType;
	bool m_fResident;
	bool m_fKeepSingleTask;
	bool m_fStandaloneProgramGuide;
	bool m_fEnable1SegFallback;
};


#endif
