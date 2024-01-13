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
		bool m_fDrawLogo = false;

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		void InitDecoderList(int ID, const GUID &SubType, LPCTSTR pszDecoderName);
		void GetDecoderSetting(int ID, String *pDecoderName) const;
	};

} // namespace TVTest


#endif
