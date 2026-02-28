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


#ifndef TVTEST_VIEW_OPTIONS_H
#define TVTEST_VIEW_OPTIONS_H


#include "Options.h"
#include "Style.h"
#include "LibISDB/LibISDB/Windows/Viewer/ViewerFilter.hpp"


namespace TVTest
{

	class CViewOptions
		: public COptions
	{
	public:
		enum {
			UPDATE_MASKCUTAREA            = 0x00000001UL,
			UPDATE_IGNOREDISPLAYEXTENSION = 0x00000002UL,
			UPDATE_LOGO                   = 0x00000004UL
		};

		enum class AdjustWindowMode {
			None,
			Fit,
			Width,
			TVTEST_ENUM_CLASS_TRAILER
		};

		CViewOptions();
		~CViewOptions();

	// COptions
		bool Apply(DWORD Flags) override;
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	// CBasicDialog
		bool Create(HWND hwndOwner) override;

	// CViewOptions
		bool GetAdjustAspectResizing() const { return m_fAdjustAspectResizing; }
		bool GetSnapAtWindowEdge() const { return m_fSnapAtWindowEdge; }
		int GetSnapAtWindowEdgeMargin() const { return m_SnapAtWindowEdgeMargin; }
		bool GetSupportAeroSnap() const { return m_fSupportAeroSnap; }
		bool GetNearCornerResizeOrigin() const { return m_fNearCornerResizeOrigin; }
		bool GetZoomKeepAspectRatio() const { return m_fZoomKeepAspectRatio; }
		AdjustWindowMode GetPanScanAdjustWindowMode() const { return m_PanScanAdjustWindowMode; }
		bool GetRemember1SegWindowSize() const { return m_fRemember1SegWindowSize; }
		bool GetMinimizeToTray() const { return m_fMinimizeToTray; }
		bool GetDisablePreviewWhenMinimized() const { return m_fDisablePreviewWhenMinimized; }
		bool GetHideCursor() const { return m_fHideCursor; }
		bool GetUseLogoIcon() const { return m_fUseLogoIcon; }
		LPCTSTR GetTitleTextFormat() const { return m_TitleTextFormat.c_str(); }
		LPCTSTR GetMinimizedTitleTextFormat() const { return m_MinimizedTitleTextFormat.c_str(); }
		LPCTSTR GetMaximizedTitleTextFormat() const { return m_MaximizedTitleTextFormat.c_str(); }
		LPCTSTR GetTaskbarTitleTextFormat() const { return m_TaskbarTitleTextFormat.c_str(); }
		bool GetTitleBarFontEnabled() const { return m_fEnableTitleBarFont; }
		const Style::Font &GetTitleBarFont() const { return m_TitleBarFont; }
		bool GetShowLogo() const { return m_fShowLogo; }
		LPCTSTR GetLogoFileName() const { return m_LogoFileName.c_str(); }

	private:
	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		bool m_fAdjustAspectResizing = false;
		bool m_fSnapAtWindowEdge = false;
		int m_SnapAtWindowEdgeMargin = 8;
		bool m_fSupportAeroSnap = true;
		bool m_fNearCornerResizeOrigin = false;
		bool m_fZoomKeepAspectRatio = false;
		AdjustWindowMode m_PanScanAdjustWindowMode = AdjustWindowMode::Width;
		bool m_fRemember1SegWindowSize = true;
		bool m_fMinimizeToTray = false;
		bool m_fDisablePreviewWhenMinimized = false;
		bool m_fHideCursor = false;
		bool m_fUseLogoIcon = false;
		String m_TitleTextFormat;
		String m_MinimizedTitleTextFormat;
		String m_MaximizedTitleTextFormat;
		String m_TaskbarTitleTextFormat;
		bool m_fEnableTitleBarFont = false;
		Style::Font m_TitleBarFont;
		bool m_fShowLogo = true;
		String m_LogoFileName{APP_NAME TEXT("_Logo.bmp")};

		Style::Font m_CurTitleBarFont;
	};

} // namespace TVTest


#endif
