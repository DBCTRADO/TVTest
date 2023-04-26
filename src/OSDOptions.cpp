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


#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "OSDOptions.h"
#include "DialogUtil.h"
#include "DrawUtil.h"
#include "Util.h"
#include "Aero.h"
#include "StyleUtil.h"
#include "resource.h"
#include <dlgs.h>
#include "Common/DebugDef.h"


namespace TVTest
{

namespace
{

constexpr unsigned int OSD_FLAG(COSDOptions::OSDType type) { return 1U << static_cast<int>(type); }

}




COSDOptions::COSDOptions()
	: m_EnabledOSD(OSD_FLAG(OSDType::Channel) | OSD_FLAG(OSDType::Volume) | OSD_FLAG(OSDType::ChannelNoInput))
{
	CAeroGlass Aero;
	if (Aero.IsEnabled())
		m_fCompositionEnabled = true;

	DrawUtil::GetSystemFont(DrawUtil::FontType::Default, &m_OSDFont);

	LOGFONT lf;
	DrawUtil::GetSystemFont(DrawUtil::FontType::Message, &lf);

	m_NotificationBarFont.LogFont = lf;
#ifndef TVTEST_FOR_1SEG
	m_NotificationBarFont.LogFont.lfHeight = lf.lfHeight * 12 / 10;
#endif
	Style::CStyleManager::AssignFontSizeFromLogFont(&m_NotificationBarFont);

	m_DisplayFont.LogFont = lf;
	Style::CStyleManager::AssignFontSizeFromLogFont(&m_DisplayFont);
}


COSDOptions::~COSDOptions()
{
	Destroy();
}


bool COSDOptions::ReadSettings(CSettings &Settings)
{
	int Value;

	Settings.Read(TEXT("UseOSD"), &m_fShowOSD);
	Settings.Read(TEXT("PseudoOSD"), &m_fPseudoOSD);
	Settings.ReadColor(TEXT("OSDTextColor"), &m_TextColor);
	Settings.Read(TEXT("OSDOpacity"), &m_Opacity);
	Settings.Read(TEXT("OSDFont"), &m_OSDFont);
	Settings.Read(TEXT("OSDFadeTime"), &m_FadeTime);
	unsigned int EnabledOSD = m_EnabledOSD;
	if (Settings.Read(TEXT("EnabledOSD"), &EnabledOSD)) {
		// 項目が増えた時にデフォルト値が反映されるようにする
		unsigned int EnabledOSDMask;
		if (!Settings.Read(TEXT("EnabledOSDMask"), &EnabledOSDMask)
				|| EnabledOSDMask == 0)
			EnabledOSDMask = OSD_FLAG(OSDType::ChannelNoInput) - 1;
		if ((EnabledOSD & ~EnabledOSDMask) == 0)
			m_EnabledOSD = (EnabledOSD & EnabledOSDMask) | (m_EnabledOSD & ~EnabledOSDMask);
		else
			m_EnabledOSD = EnabledOSD;
	}
	if (Settings.Read(TEXT("ChannelOSDType"), &Value)
			&& CheckEnumRange(static_cast<ChannelChangeType>(Value)))
		m_ChannelChangeType = static_cast<ChannelChangeType>(Value);
	Settings.Read(TEXT("ChannelOSDText"), &m_ChannelChangeText);

	Settings.Read(TEXT("EnableNotificationBar"), &m_fEnableNotificationBar);
	Settings.Read(TEXT("NotificationBarDuration"), &m_NotificationBarDuration);
	bool f;
	if (Settings.Read(TEXT("NotifyEventName"), &f))
		EnableNotify(NOTIFY_EVENTNAME, f);
	if (Settings.Read(TEXT("NotifyTSProcessorError"), &f))
		EnableNotify(NOTIFY_TSPROCESSORERROR, f);

	if (StyleUtil::ReadFontSettings(
				Settings, TEXT("NotificationBarFont"), &m_NotificationBarFont, true, &f)) {
		if (!f)
			m_fChanged = true;
	}

	if (StyleUtil::ReadFontSettings(
				Settings, TEXT("DisplayMenuFont"), &m_DisplayFont, false, &f)) {
		if (!f)
			m_fChanged = true;
	}

	Settings.Read(TEXT("DisplayMenuFontAutoSize"), &m_fDisplayFontAutoSize);

	return true;
}


bool COSDOptions::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("UseOSD"), m_fShowOSD);
	Settings.Write(TEXT("PseudoOSD"), m_fPseudoOSD);
	Settings.WriteColor(TEXT("OSDTextColor"), m_TextColor);
	Settings.Write(TEXT("OSDOpacity"), m_Opacity);
	Settings.Write(TEXT("OSDFont"), &m_OSDFont);
	Settings.Write(TEXT("OSDFadeTime"), m_FadeTime);
	Settings.Write(TEXT("EnabledOSD"), m_EnabledOSD);
	Settings.Write(TEXT("EnabledOSDMask"), OSD_FLAG(OSDType::Trailer_) - 1);
	Settings.Write(TEXT("ChannelOSDType"), static_cast<int>(m_ChannelChangeType));
	Settings.Write(TEXT("ChannelOSDText"), m_ChannelChangeText);

	Settings.Write(TEXT("EnableNotificationBar"), m_fEnableNotificationBar);
	Settings.Write(TEXT("NotificationBarDuration"), m_NotificationBarDuration);
	Settings.Write(TEXT("NotifyEventName"), (m_NotificationBarFlags & NOTIFY_EVENTNAME) != 0);
	Settings.Write(TEXT("NotifyTSProcessorError"), (m_NotificationBarFlags & NOTIFY_TSPROCESSORERROR) != 0);

	StyleUtil::WriteFontSettings(Settings, TEXT("NotificationBarFont"), m_NotificationBarFont);
	StyleUtil::WriteFontSettings(Settings, TEXT("DisplayMenuFont"), m_DisplayFont);
	Settings.Write(TEXT("DisplayMenuFontAutoSize"), m_fDisplayFontAutoSize);

	return true;
}


bool COSDOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(
		hwndOwner,
		GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_OPTIONS_OSD));
}


bool COSDOptions::GetLayeredWindow() const
{
	return m_fLayeredWindow && m_fCompositionEnabled;
}


void COSDOptions::OnDwmCompositionChanged()
{
	CAeroGlass Aero;

	m_fCompositionEnabled = Aero.IsEnabled();
}


bool COSDOptions::IsOSDEnabled(OSDType Type) const
{
	return m_fShowOSD && (m_EnabledOSD & OSD_FLAG(Type)) != 0;
}


bool COSDOptions::IsNotifyEnabled(unsigned int Type) const
{
	return m_fEnableNotificationBar && (m_NotificationBarFlags & Type) != 0;
}


void COSDOptions::EnableNotify(unsigned int Type, bool fEnabled)
{
	if (fEnabled)
		m_NotificationBarFlags |= Type;
	else
		m_NotificationBarFlags &= ~Type;
}


INT_PTR COSDOptions::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			DlgCheckBox_Check(hDlg, IDC_OSDOPTIONS_SHOWOSD, m_fShowOSD);
			DlgCheckBox_Check(hDlg, IDC_OSDOPTIONS_COMPOSITE, !m_fPseudoOSD);
			m_CurTextColor = m_TextColor;
			m_CurOSDFont = m_OSDFont;
			::SetDlgItemText(hDlg, IDC_OSDOPTIONS_OSDFONT_INFO, m_OSDFont.lfFaceName);
			::SetDlgItemInt(hDlg, IDC_OSDOPTIONS_FADETIME, m_FadeTime / 1000, TRUE);
			DlgUpDown_SetRange(hDlg, IDC_OSDOPTIONS_FADETIME_UD, 1, UD_MAXVAL);
			DlgCheckBox_Check(hDlg, IDC_OSDOPTIONS_SHOW_CHANNEL, (m_EnabledOSD & OSD_FLAG(OSDType::Channel)) != 0);
			DlgCheckBox_Check(hDlg, IDC_OSDOPTIONS_SHOW_VOLUME, (m_EnabledOSD & OSD_FLAG(OSDType::Volume)) != 0);
			DlgCheckBox_Check(hDlg, IDC_OSDOPTIONS_SHOW_AUDIO, (m_EnabledOSD & OSD_FLAG(OSDType::Audio)) != 0);
			DlgCheckBox_Check(hDlg, IDC_OSDOPTIONS_SHOW_RECORDING, (m_EnabledOSD & OSD_FLAG(OSDType::Recording)) != 0);
			DlgCheckBox_Check(hDlg, IDC_OSDOPTIONS_SHOW_CHANNELNOINPUT, (m_EnabledOSD & OSD_FLAG(OSDType::ChannelNoInput)) != 0);
			static const LPCTSTR ChannelChangeModeText[] = {
				TEXT("ロゴとテキスト"),
				TEXT("テキストのみ"),
				TEXT("ロゴのみ"),
			};
			SetComboBoxList(
				hDlg, IDC_OSDOPTIONS_CHANNELCHANGE_TYPE,
				ChannelChangeModeText, lengthof(ChannelChangeModeText));
			DlgComboBox_SetCurSel(hDlg, IDC_OSDOPTIONS_CHANNELCHANGE_TYPE, static_cast<int>(m_ChannelChangeType));
			DlgEdit_SetText(hDlg, IDC_OSDOPTIONS_CHANNELCHANGE_TEXT, m_ChannelChangeText.c_str());
			InitDropDownButton(hDlg, IDC_OSDOPTIONS_CHANNELCHANGE_TEXT_PARAMS);
			EnableDlgItems(hDlg, IDC_OSDOPTIONS_FIRST, IDC_OSDOPTIONS_LAST, m_fShowOSD);

			DlgCheckBox_Check(hDlg, IDC_NOTIFICATIONBAR_ENABLE, m_fEnableNotificationBar);
			DlgCheckBox_Check(hDlg, IDC_NOTIFICATIONBAR_NOTIFYEVENTNAME, (m_NotificationBarFlags & NOTIFY_EVENTNAME) != 0);
			DlgCheckBox_Check(hDlg, IDC_NOTIFICATIONBAR_NOTIFYTSPROCESSORERROR, (m_NotificationBarFlags & NOTIFY_TSPROCESSORERROR) != 0);
			::SetDlgItemInt(hDlg, IDC_NOTIFICATIONBAR_DURATION, m_NotificationBarDuration / 1000, FALSE);
			DlgUpDown_SetRange(hDlg, IDC_NOTIFICATIONBAR_DURATION_UPDOWN, 1, 60);
			m_CurNotificationBarFont = m_NotificationBarFont;
			StyleUtil::SetFontInfoItem(hDlg, IDC_NOTIFICATIONBAR_FONT_INFO, m_CurNotificationBarFont);
			EnableDlgItems(hDlg, IDC_NOTIFICATIONBAR_FIRST, IDC_NOTIFICATIONBAR_LAST, m_fEnableNotificationBar);

			m_DisplayFontCur = m_DisplayFont;
			StyleUtil::SetFontInfoItem(hDlg, IDC_DISPLAYMENU_FONT_INFO, m_DisplayFont);
			DlgCheckBox_Check(hDlg, IDC_DISPLAYMENU_AUTOFONTSIZE, m_fDisplayFontAutoSize);

			AddControls({
				{IDC_OSDOPTIONS_GROUP,                     AlignFlag::Horz},
				{IDC_OSDOPTIONS_CHANNELCHANGE_TEXT,        AlignFlag::Horz},
				{IDC_OSDOPTIONS_CHANNELCHANGE_TEXT_PARAMS, AlignFlag::Right},
			});
		}
		return TRUE;

#if 0
	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT pdis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
			RECT rc = pdis->rcItem;

			::DrawEdge(pdis->hDC, &rc, BDR_SUNKENOUTER, BF_RECT | BF_ADJUST);
			DrawUtil::Fill(pdis->hDC, &rc, m_CurTextColor);
		}
		return TRUE;
#endif

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OSDOPTIONS_SHOWOSD:
			EnableDlgItemsSyncCheckBox(
				hDlg, IDC_OSDOPTIONS_FIRST, IDC_OSDOPTIONS_LAST, IDC_OSDOPTIONS_SHOWOSD);
			return TRUE;

		case IDC_OSDOPTIONS_TEXTCOLOR:
			if (ChooseColorDialog(hDlg, &m_CurTextColor))
				InvalidateDlgItem(hDlg, IDC_OSDOPTIONS_TEXTCOLOR);
			return TRUE;

		case IDC_OSDOPTIONS_OSDFONT_CHOOSE:
			{
				CHOOSEFONT cf;
				LOGFONT lf = m_CurOSDFont;

				lf.lfWidth = 0;
				lf.lfHeight = -PointsToPixels(11);
				cf.lStructSize = sizeof(CHOOSEFONT);
				cf.hwndOwner = hDlg;
				cf.lpLogFont = &lf;
				cf.Flags = CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS | CF_ENABLEHOOK;
				cf.lpfnHook = ChooseFontHookProc;
				if (::ChooseFont(&cf)) {
					m_CurOSDFont = lf;
					::SetDlgItemText(hDlg, IDC_OSDOPTIONS_OSDFONT_INFO, lf.lfFaceName);
				}
			}
			return TRUE;

		case IDC_OSDOPTIONS_CHANNELCHANGE_TEXT_PARAMS:
			{
				RECT rc;

				::GetWindowRect(::GetDlgItem(hDlg, IDC_OSDOPTIONS_CHANNELCHANGE_TEXT_PARAMS), &rc);
				const POINT pt = {rc.left, rc.bottom};
				CUICore::CTitleStringMap StrMap(GetAppClass());
				StrMap.InputParameter(hDlg, IDC_OSDOPTIONS_CHANNELCHANGE_TEXT, pt);
			}
			return TRUE;

		case IDC_NOTIFICATIONBAR_ENABLE:
			EnableDlgItemsSyncCheckBox(
				hDlg, IDC_NOTIFICATIONBAR_FIRST, IDC_NOTIFICATIONBAR_LAST, IDC_NOTIFICATIONBAR_ENABLE);
			return TRUE;

		case IDC_NOTIFICATIONBAR_FONT_CHOOSE:
			if (StyleUtil::ChooseStyleFont(hDlg, &m_CurNotificationBarFont))
				StyleUtil::SetFontInfoItem(hDlg, IDC_NOTIFICATIONBAR_FONT_INFO, m_CurNotificationBarFont);
			return TRUE;

		case IDC_DISPLAYMENU_FONT_CHOOSE:
			if (StyleUtil::ChooseStyleFont(hDlg, &m_DisplayFontCur))
				StyleUtil::SetFontInfoItem(hDlg, IDC_DISPLAYMENU_FONT_INFO, m_DisplayFontCur);
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case PSN_APPLY:
			{
				m_fShowOSD = DlgCheckBox_IsChecked(hDlg, IDC_OSDOPTIONS_SHOWOSD);
				m_fPseudoOSD = !DlgCheckBox_IsChecked(hDlg, IDC_OSDOPTIONS_COMPOSITE);
				m_TextColor = m_CurTextColor;
				if (!CompareLogFont(&m_OSDFont, &m_CurOSDFont)) {
					m_OSDFont = m_CurOSDFont;
					GetAppClass().OSDManager.OnOSDFontChanged();
				}
				m_FadeTime = ::GetDlgItemInt(hDlg, IDC_OSDOPTIONS_FADETIME, nullptr, FALSE) * 1000;
				unsigned int EnabledOSD = 0;
				if (DlgCheckBox_IsChecked(hDlg, IDC_OSDOPTIONS_SHOW_CHANNEL))
					EnabledOSD |= OSD_FLAG(OSDType::Channel);
				if (DlgCheckBox_IsChecked(hDlg, IDC_OSDOPTIONS_SHOW_VOLUME))
					EnabledOSD |= OSD_FLAG(OSDType::Volume);
				if (DlgCheckBox_IsChecked(hDlg, IDC_OSDOPTIONS_SHOW_AUDIO))
					EnabledOSD |= OSD_FLAG(OSDType::Audio);
				if (DlgCheckBox_IsChecked(hDlg, IDC_OSDOPTIONS_SHOW_RECORDING))
					EnabledOSD |= OSD_FLAG(OSDType::Recording);
				if (DlgCheckBox_IsChecked(hDlg, IDC_OSDOPTIONS_SHOW_CHANNELNOINPUT))
					EnabledOSD |= OSD_FLAG(OSDType::ChannelNoInput);
				m_EnabledOSD = EnabledOSD;
				m_ChannelChangeType = static_cast<ChannelChangeType>(DlgComboBox_GetCurSel(hDlg, IDC_OSDOPTIONS_CHANNELCHANGE_TYPE));
				GetDlgItemString(hDlg, IDC_OSDOPTIONS_CHANNELCHANGE_TEXT, &m_ChannelChangeText);

				m_fEnableNotificationBar =
					DlgCheckBox_IsChecked(hDlg, IDC_NOTIFICATIONBAR_ENABLE);
				EnableNotify(
					NOTIFY_EVENTNAME,
					DlgCheckBox_IsChecked(hDlg, IDC_NOTIFICATIONBAR_NOTIFYEVENTNAME));
				EnableNotify(
					NOTIFY_TSPROCESSORERROR,
					DlgCheckBox_IsChecked(hDlg, IDC_NOTIFICATIONBAR_NOTIFYTSPROCESSORERROR));
				m_NotificationBarDuration =
					::GetDlgItemInt(hDlg, IDC_NOTIFICATIONBAR_DURATION, nullptr, FALSE) * 1000;
				m_NotificationBarFont = m_CurNotificationBarFont;

				m_DisplayFont = m_DisplayFontCur;
				m_fDisplayFontAutoSize = DlgCheckBox_IsChecked(hDlg, IDC_DISPLAYMENU_AUTOFONTSIZE);

				m_fChanged = true;
			}
			return TRUE;

		case NM_CUSTOMDRAW:
			{
				LPNMCUSTOMDRAW pnmcd = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);

				if (pnmcd->hdr.idFrom == IDC_OSDOPTIONS_TEXTCOLOR) {
					switch (pnmcd->dwDrawStage) {
					case CDDS_PREPAINT:
						::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_NOTIFYPOSTPAINT);
						return TRUE;

					case CDDS_POSTPAINT:
						{
							const HDC hdc = pnmcd->hdc;
							RECT rc = pnmcd->rc;

							bool fMargins = false;
							CUxTheme Theme;
							if (Theme.IsActive()) {
								if (Theme.Open(pnmcd->hdr.hwndFrom, L"BUTTON", m_CurrentDPI)) {
									MARGINS margins;
									if (Theme.GetMargins(BP_PUSHBUTTON, PBS_NORMAL, TMT_CONTENTMARGINS, &margins)) {
										rc.left += margins.cxLeftWidth + 1;
										rc.top += margins.cyTopHeight + 1;
										rc.right -= margins.cxRightWidth + 1;
										rc.bottom -= margins.cyBottomHeight + 1;
										fMargins = true;
									}
									Theme.Close();
								}
							}
							if (!fMargins)
								::InflateRect(&rc, -6, -4);
							const HGDIOBJ hOldPen = ::SelectObject(hdc, ::GetStockObject(BLACK_PEN));
							::Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
							::InflateRect(&rc, -1, -1);
							::SelectObject(hdc, ::GetStockObject(WHITE_PEN));
							::Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
							::SelectObject(hdc, hOldPen);
							::InflateRect(&rc, -1, -1);
							DrawUtil::Fill(hdc, &rc, m_CurTextColor);
						}
						return 0;
					}
				}
			}
			break;
		}
		break;
	}

	return FALSE;
}


UINT_PTR CALLBACK COSDOptions::ChooseFontHookProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		// サイズの項目を非表示にする
		ShowDlgItem(hDlg, stc3, false);
		ShowDlgItem(hDlg, cmb3, false);
		break;
	}

	return FALSE;
}


}	// namespace TVTest
