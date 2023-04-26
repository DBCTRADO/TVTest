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

		enum {
			NOTIFY_EVENTNAME        = 0x00000001,
			NOTIFY_TSPROCESSORERROR = 0x00000002
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
		bool IsNotificationBarEnabled() const { return m_fEnableNotificationBar; }
		int GetNotificationBarDuration() const { return m_NotificationBarDuration; }
		const Style::Font &GetNotificationBarFont() const { return m_NotificationBarFont; }
		bool IsNotifyEnabled(unsigned int Type) const;
		const Style::Font &GetDisplayFont() const { return m_DisplayFont; }
		bool IsDisplayFontAutoSize() const { return m_fDisplayFontAutoSize; }

	private:
	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		static UINT_PTR CALLBACK ChooseFontHookProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

		void EnableNotify(unsigned int Type, bool fEnabled);

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

		bool m_fEnableNotificationBar = true;
		int m_NotificationBarDuration = 3000;
		unsigned int m_NotificationBarFlags = NOTIFY_EVENTNAME | NOTIFY_TSPROCESSORERROR;
		Style::Font m_NotificationBarFont;
		Style::Font m_CurNotificationBarFont;

		Style::Font m_DisplayFont;
		Style::Font m_DisplayFontCur;
		bool m_fDisplayFontAutoSize = false;
	};

}	// namespace TVTest


#endif
