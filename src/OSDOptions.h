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


#ifndef TVTEST_OSD_OPTIONS_H
#define TVTEST_OSD_OPTIONS_H


#include "Options.h"
#include "Style.h"


namespace TVTest
{

	class COSDOptions
		: public COptions
	{
	public:
		enum class ChannelChangeType {
			LogoAndText,
			TextOnly,
			LogoOnly,
			TVTEST_ENUM_CLASS_TRAILER
		};

		enum class OSDType {
			Channel,
			Volume,
			Audio,
			Recording,
			ChannelNoInput,
			TVTEST_ENUM_CLASS_TRAILER
		};

		COSDOptions();
		~COSDOptions();

	// CSettingsBase
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	// CBasicDialog
		bool Create(HWND hwndOwner) override;

	// COSDOptions
		bool GetShowOSD() const { return m_fShowOSD; }
		bool GetPseudoOSD() const { return m_fPseudoOSD; }
		COLORREF GetTextColor() const { return m_TextColor; }
		int GetOpacity() const { return m_Opacity; }
		const LOGFONT *GetOSDFont() const { return &m_OSDFont; }
		int GetFadeTime() const { return m_FadeTime; }
		ChannelChangeType GetChannelChangeType() const { return m_ChannelChangeType; }
		LPCTSTR GetChannelChangeText() const { return m_ChannelChangeText.c_str(); }
		bool GetLayeredWindow() const;
		void OnDwmCompositionChanged();
		bool IsOSDEnabled(OSDType Type) const;

		const LOGFONT &GetEventInfoOSDFont() const { return m_EventInfoOSDFont; }
		unsigned int GetEventInfoOSDDuration() const { return m_EventInfoOSDDuration; }
		bool GetEventInfoOSDAutoShowChannelChange() const { return m_fEventInfoOSDAutoShowChannelChange; }
		bool GetEventInfoOSDAutoShowEventChange() const { return m_fEventInfoOSDAutoShowEventChange; }
		bool GetEventInfoOSDManualShowNoAutoHide() const { return m_fEventInfoOSDManualShowNoAutoHide; }
		int GetEventInfoOSDOpacity() const { return m_EventInfoOSDOpacity; }

		const Style::Font &GetDisplayFont() const { return m_DisplayFont; }
		bool IsDisplayFontAutoSize() const { return m_fDisplayFontAutoSize; }

	private:
	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		bool m_fShowOSD = true;
		bool m_fPseudoOSD = true;
		COLORREF m_TextColor = RGB(0, 255, 0);
		COLORREF m_CurTextColor;
		int m_Opacity = 80;
		LOGFONT m_OSDFont;
		LOGFONT m_CurOSDFont;
		int m_FadeTime = 3000;
		ChannelChangeType m_ChannelChangeType = ChannelChangeType::LogoAndText;
		String m_ChannelChangeText{TEXT("%channel-no% %channel-name%")};
		unsigned int m_EnabledOSD;

		bool m_fLayeredWindow = true;
		bool m_fCompositionEnabled = false;

		LOGFONT m_EventInfoOSDFont;
		LOGFONT m_EventInfoOSDFontCur;
		unsigned int m_EventInfoOSDDuration = 10;
		bool m_fEventInfoOSDAutoShowChannelChange = false;
		bool m_fEventInfoOSDAutoShowEventChange = false;
		bool m_fEventInfoOSDManualShowNoAutoHide = true;
		int m_EventInfoOSDOpacity = 60;

		Style::Font m_DisplayFont;
		Style::Font m_DisplayFontCur;
		bool m_fDisplayFontAutoSize = false;
	};

}	// namespace TVTest


#endif
