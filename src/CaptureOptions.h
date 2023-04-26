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


#ifndef TVTEST_CAPTURE_OPTIONS_H
#define TVTEST_CAPTURE_OPTIONS_H


#include "Image.h"
#include "Options.h"
#include "Capture.h"


namespace TVTest
{

	class CCaptureOptions
		: public COptions
	{
	public:
		enum {
			SIZE_TYPE_ORIGINAL,
			SIZE_TYPE_VIEW,
			SIZE_TYPE_RAW,
			SIZE_TYPE_CUSTOM,
			SIZE_TYPE_PERCENTAGE,
			SIZE_TYPE_LAST = SIZE_TYPE_PERCENTAGE
		};
		enum {
			SIZE_1920x1080,
			SIZE_1440x810,
			SIZE_1280x720,
			SIZE_1024x576,
			SIZE_960x540,
			SIZE_800x450,
			SIZE_640x360,
			SIZE_320x180,
			SIZE_1440x1080,
			SIZE_1280x960,
			SIZE_1024x768,
			SIZE_800x600,
			SIZE_720x540,
			SIZE_640x480,
			SIZE_320x240,
			SIZE_LAST = SIZE_320x240
		};
		enum {
			PERCENTAGE_75,
			PERCENTAGE_66,
			PERCENTAGE_50,
			PERCENTAGE_33,
			PERCENTAGE_25,
			PERCENTAGE_LAST = PERCENTAGE_25
		};

		~CCaptureOptions();

	// CSettingsBase
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	// CBasicDialog
		bool Create(HWND hwndOwner) override;

	// CCaptureOptions
		LPCTSTR GetSaveFolder() const { return m_SaveFolder.c_str(); }
		LPCTSTR GetFileName() const { return m_FileName.c_str(); }
		int GetSaveFormat() const { return m_SaveFormat; }
		bool GetWriteComment() const { return m_fSetComment; }
		bool SetPresetCaptureSize(int Size);
		int GetPresetCaptureSize() const;
		int GetCaptureSizeType() const { return m_CaptureSizeType; }
		bool GetSizePercentage(int *pNum, int *pDenom) const;
		bool GetCustomSize(int *pWidth, int *pHeight) const;
		bool GenerateFileName(String *pFileName, const CCaptureImage *pImage) const;
		bool GetOptionText(LPTSTR pszOption, int MaxLength) const;
		bool GetCommentText(String *pComment, const CCaptureImage *pImage) const;
		bool SaveImage(CCaptureImage *pImage);
		int TranslateCommand(int Command);
		bool OpenSaveFolder() const;

	private:
		String m_SaveFolder;
		String m_FileName{TEXT("Capture_%date%-%time%")};
		int m_SaveFormat = 0;
		int m_JPEGQuality = 90;
		int m_PNGCompressionLevel = 6;
		bool m_fCaptureSaveToFile = true;
		bool m_fSetComment = false;
		String m_CommentFormat{TEXT("%year%/%month%/%day% %hour%:%minute%:%second% %channel-name%\r\n%event-title%")};
		int m_CaptureSizeType = SIZE_TYPE_ORIGINAL;
		int m_CaptureSize =
#ifndef TVTEST_FOR_1SEG
			SIZE_1920x1080
#else
			SIZE_320x180
#endif
			;
		int m_CapturePercentage = PERCENTAGE_50;
		CImageCodec m_ImageCodec;

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		void UpdateFileNamePreview();

		static const SIZE m_SizeList[SIZE_LAST + 1];
		struct PercentageType {
			BYTE Num, Denom;
		};
		static const PercentageType m_PercentageList[PERCENTAGE_LAST + 1];
	};

}	// namespace TVTest


#endif
