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


#ifndef TVTEST_VIDEO_OPTIONS_H
#define TVTEST_VIDEO_OPTIONS_H


#include "Options.h"
#include "LibISDB/LibISDB/Windows/Viewer/ViewerFilter.hpp"
#include "LibISDB/LibISDB/Windows/Viewer/DirectShow/VideoRenderers/VideoRenderer.hpp"


namespace TVTest
{

	class CVideoOptions
		: public COptions
	{
	public:
		struct RendererInfo
		{
			LibISDB::DirectShow::VideoRenderer::RendererType Renderer;
			LPCTSTR pszName;
		};

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
		LibISDB::DirectShow::VideoRenderer::RendererType GetVideoRendererType() const;
		bool SetVideoRendererType(LibISDB::DirectShow::VideoRenderer::RendererType Renderer);
		bool GetResetPanScanEventChange() const { return m_fResetPanScanEventChange; }
		bool GetNoMaskSideCut() const { return m_fNoMaskSideCut; }
		bool SetStretchMode(LibISDB::ViewerFilter::ViewStretchMode Mode);
		LibISDB::ViewerFilter::ViewStretchMode GetStretchMode() const { return m_StretchMode; }
		LibISDB::ViewerFilter::ViewStretchMode GetFullscreenStretchMode() const { return m_FullscreenStretchMode; }
		LibISDB::ViewerFilter::ViewStretchMode GetMaximizeStretchMode() const { return m_MaximizeStretchMode; }
		bool GetIgnoreDisplayExtension() const { return m_fIgnoreDisplayExtension; }
		bool GetClipToDevice() const { return m_fClipToDevice; }

		static bool GetRendererInfo(int Index, RendererInfo *pInfo);

	private:
	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		void SetVideoDecoderList(
			int ID, const GUID &SubType, BYTE StreamType, const String &DecoderName);
		void GetVideoDecoderSetting(int ID, BYTE StreamType, String *pDecoderName);

		enum {
			UPDATE_DECODER                = 0x00000001UL,
			UPDATE_RENDERER               = 0x00000002UL,
			UPDATE_MASKCUTAREA            = 0x00000004UL,
			UPDATE_IGNOREDISPLAYEXTENSION = 0x00000008UL,
			UPDATE_CLIPTODEVICE           = 0x00000010UL
		};

		static constexpr size_t MAX_VIDEO_DECODER_NAME = 128;

		static const RendererInfo m_RendererList[];

		String m_Mpeg2DecoderName;
		String m_H264DecoderName;
		String m_H265DecoderName;
		LibISDB::DirectShow::VideoRenderer::RendererType m_VideoRendererType = LibISDB::DirectShow::VideoRenderer::RendererType::Default;
		bool m_fResetPanScanEventChange = true;
		bool m_fNoMaskSideCut = true;
		LibISDB::ViewerFilter::ViewStretchMode m_StretchMode = LibISDB::ViewerFilter::ViewStretchMode::KeepAspectRatio;
		LibISDB::ViewerFilter::ViewStretchMode m_FullscreenStretchMode = LibISDB::ViewerFilter::ViewStretchMode::KeepAspectRatio;
		LibISDB::ViewerFilter::ViewStretchMode m_MaximizeStretchMode = LibISDB::ViewerFilter::ViewStretchMode::KeepAspectRatio;
		bool m_fIgnoreDisplayExtension = false;
		bool m_fClipToDevice = true;
	};

}	// namespace TVTest


#endif
