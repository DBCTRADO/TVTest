#ifndef TVTEST_VIDEO_OPTIONS_H
#define TVTEST_VIDEO_OPTIONS_H


#include "Options.h"
#include "BonTsEngine/MediaViewer.h"
#include "DirectShowFilter/VideoRenderer.h"


class CVideoOptions : public COptions
{
public:
	struct RendererInfo
	{
		CVideoRenderer::RendererType Renderer;
		LPCTSTR pszName;
	};

	CVideoOptions();
	~CVideoOptions();

// COptions
	bool Apply(DWORD Flags) override;

// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;

// CBasicDialog
	bool Create(HWND hwndOwner) override;

// CVideoOptions
	LPCTSTR GetMpeg2DecoderName() const;
	void SetMpeg2DecoderName(LPCTSTR pszDecoderName);
	LPCTSTR GetH264DecoderName() const;
	void SetH264DecoderName(LPCTSTR pszDecoderName);
	LPCTSTR GetH265DecoderName() const;
	void SetH265DecoderName(LPCTSTR pszDecoderName);
	CVideoRenderer::RendererType GetVideoRendererType() const;
	bool SetVideoRendererType(CVideoRenderer::RendererType Renderer);
	bool GetResetPanScanEventChange() const { return m_fResetPanScanEventChange; }
	bool GetNoMaskSideCut() const { return m_fNoMaskSideCut; }
	CMediaViewer::ViewStretchMode GetFullscreenStretchMode() const { return m_FullscreenStretchMode; }
	CMediaViewer::ViewStretchMode GetMaximizeStretchMode() const { return m_MaximizeStretchMode; }
	bool GetIgnoreDisplayExtension() const { return m_fIgnoreDisplayExtension; }
	bool GetClipToDevice() const { return m_fClipToDevice; }

	static bool GetRendererInfo(int Index,RendererInfo *pInfo);

private:
// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	void SetVideoDecoderList(
		int ID,const GUID &SubType,BYTE StreamType,const TVTest::String &DecoderName);
	void GetVideoDecoderSetting(int ID,BYTE StreamType,TVTest::String *pDecoderName);

	enum {
		UPDATE_DECODER					= 0x00000001UL,
		UPDATE_RENDERER					= 0x00000002UL,
		UPDATE_MASKCUTAREA				= 0x00000004UL,
		UPDATE_IGNOREDISPLAYEXTENSION	= 0x00000008UL,
		UPDATE_CLIPTODEVICE				= 0x00000010UL
	};
	enum {
		MAX_VIDEO_DECODER_NAME=128
	};

	static const RendererInfo m_RendererList[];

	TVTest::String m_Mpeg2DecoderName;
	TVTest::String m_H264DecoderName;
	TVTest::String m_H265DecoderName;
	CVideoRenderer::RendererType m_VideoRendererType;
	bool m_fResetPanScanEventChange;
	bool m_fNoMaskSideCut;
	CMediaViewer::ViewStretchMode m_FullscreenStretchMode;
	CMediaViewer::ViewStretchMode m_MaximizeStretchMode;
	bool m_fIgnoreDisplayExtension;
	bool m_fClipToDevice;
};


#endif
