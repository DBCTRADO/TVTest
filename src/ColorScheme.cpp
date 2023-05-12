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
#include <algorithm>
#include "TVTest.h"
#include "AppMain.h"
#include "ColorScheme.h"
#include "Settings.h"
#include "ThemeManager.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace
{

static const LPCTSTR GradientDirectionList[] = {
	TEXT("horizontal"),
	TEXT("vertical"),
	TEXT("horizontal-mirror"),
	TEXT("vertical-mirror"),
};

static const LPCTSTR GradientTypeNameList[] = {
	TEXT("normal"),
	TEXT("glossy"),
	TEXT("interlaced"),
};

static const LPCTSTR FillTypeNameList[] = {
	TEXT("none"),
	TEXT("solid"),
	TEXT("gradient"),
};

static const LPCTSTR BorderTypeNameList[] = {
	TEXT("none"),
	TEXT("solid"),
	TEXT("sunken"),
	TEXT("raised"),
};


constexpr COLORREF HEXRGB(DWORD hex) { return RGB(hex >> 16, (hex >> 8) & 0xFF, hex & 0xFF); }

}


const CColorScheme::ColorInfo CColorScheme::m_ColorInfoList[NUM_COLORS] = {
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("StatusBack"),                         TEXT("ステータスバー 背景1")},
	{HEXRGB(0x111111), CLR_INVALID,      TEXT("StatusBack2"),                        TEXT("ステータスバー 背景2")},
	{HEXRGB(0x999999), CLR_INVALID,      TEXT("StatusText"),                         TEXT("ステータスバー 文字")},
	{HEXRGB(0x777777), CLR_INVALID,      TEXT("StatusItemBorder"),                   TEXT("ステータスバー 項目外枠")},
	{HEXRGB(0x111111), CLR_INVALID,      TEXT("StatusBottomItemBack"),               TEXT("ステータスバー 下段背景1")},
	{HEXRGB(0x111111), CLR_INVALID,      TEXT("StatusBottomItemBack2"),              TEXT("ステータスバー 下段背景2")},
	{HEXRGB(0x999999), CLR_INVALID,      TEXT("StatusBottomItemText"),               TEXT("ステータスバー 下段文字")},
	{HEXRGB(0x111111), CLR_INVALID,      TEXT("StatusBottomItemBorder"),             TEXT("ステータスバー 下段外枠")},
	{HEXRGB(0x4486E8), CLR_INVALID,      TEXT("StatusHighlightBack"),                TEXT("ステータスバー 選択背景1")},
	{HEXRGB(0x3C76CC), CLR_INVALID,      TEXT("StatusHighlightBack2"),               TEXT("ステータスバー 選択背景2")},
	{HEXRGB(0xDDDDDD), CLR_INVALID,      TEXT("StatusHighlightText"),                TEXT("ステータスバー 選択文字")},
	{HEXRGB(0x3C76CC), CLR_INVALID,      TEXT("StatusHighlightBorder"),              TEXT("ステータスバー 選択外枠")},
	{HEXRGB(0x111111), CLR_INVALID,      TEXT("StatusBorder"),                       TEXT("ステータスバー 外枠")},
	{HEXRGB(0xDF3F00), CLR_INVALID,      TEXT("StatusRecordingCircle"),              TEXT("ステータスバー 録画●")},
	{HEXRGB(0x444444), CLR_INVALID,      TEXT("StatusEventProgressBack"),            TEXT("ステータスバー 番組経過時間背景1")},
	{HEXRGB(0x444444), CLR_INVALID,      TEXT("StatusEventProgressBack2"),           TEXT("ステータスバー 番組経過時間背景2")},
	{HEXRGB(0x444444), CLR_INVALID,      TEXT("StatusEventProgressBorder"),          TEXT("ステータスバー 番組経過時間外枠")},
	{HEXRGB(0x3465B0), CLR_INVALID,      TEXT("StatusEventProgressElapsed"),         TEXT("ステータスバー 番組経過時間バー1")},
	{HEXRGB(0x3465B0), CLR_INVALID,      TEXT("StatusEventProgressElapsed2"),        TEXT("ステータスバー 番組経過時間バー2")},
	{HEXRGB(0x3465B0), CLR_INVALID,      TEXT("StatusEventProgressElapsedBorder"),   TEXT("ステータスバー 番組経過時間バー外枠")},
	{HEXRGB(0x222222), CLR_INVALID,      TEXT("Splitter"),                           TEXT("分割線")},
	{HEXRGB(0x000000), CLR_INVALID,      TEXT("ScreenBorder"),                       TEXT("画面の外枠")},
	{HEXRGB(0x555555), CLR_INVALID,      TEXT("WindowFrame"),                        TEXT("ウィンドウ 細枠")},
	{HEXRGB(0x555555), CLR_INVALID,      TEXT("WindowFrameBorder"),                  TEXT("ウィンドウ 細枠の境界")},
	{HEXRGB(0x666666), CLR_INVALID,      TEXT("WindowActiveFrame"),                  TEXT("ウィンドウ アクティブ細枠")},
	{HEXRGB(0x666666), CLR_INVALID,      TEXT("WindowActiveFrameBorder"),            TEXT("ウィンドウ アクティブ細枠の境界")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("PanelBack"),                          TEXT("パネル 背景")},
	{HEXRGB(0x999999), CLR_INVALID,      TEXT("PanelText"),                          TEXT("パネル 文字")},
	{HEXRGB(0x000000), CLR_INVALID,      TEXT("PanelTabBack"),                       TEXT("パネル タブ背景1")},
	{HEXRGB(0x222222), CLR_INVALID,      TEXT("PanelTabBack2"),                      TEXT("パネル タブ背景2")},
	{HEXRGB(0x888888), CLR_INVALID,      TEXT("PanelTabText"),                       TEXT("パネル タブ文字")},
	{HEXRGB(0x000000), CLR_INVALID,      TEXT("PanelTabBorder"),                     TEXT("パネル タブ外枠")},
	{HEXRGB(0x555555), CLR_INVALID,      TEXT("PanelCurTabBack"),                    TEXT("パネル 選択タブ背景1")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("PanelCurTabBack2"),                   TEXT("パネル 選択タブ背景2")},
	{HEXRGB(0xAAAAAA), CLR_INVALID,      TEXT("PanelCurTabText"),                    TEXT("パネル 選択タブ文字")},
	{HEXRGB(0x444444), CLR_INVALID,      TEXT("PanelCurTabBorder"),                  TEXT("パネル 選択タブ外枠")},
	{HEXRGB(0x000000), CLR_INVALID,      TEXT("PanelTabMargin"),                     TEXT("パネル タブ余白1")},
	{HEXRGB(0x222222), CLR_INVALID,      TEXT("PanelTabMargin2"),                    TEXT("パネル タブ余白2")},
	{HEXRGB(0x888888), CLR_INVALID,      TEXT("PanelTabMarginBorder"),               TEXT("パネル タブ余白外枠")},
	{HEXRGB(0x444444), CLR_INVALID,      TEXT("PanelTabLine"),                       TEXT("パネル タブ線")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("PanelTitleBack"),                     TEXT("パネル タイトル背景1")},
	{HEXRGB(0x111111), CLR_INVALID,      TEXT("PanelTitleBack2"),                    TEXT("パネル タイトル背景2")},
	{HEXRGB(0xAAAAAA), CLR_INVALID,      TEXT("PanelTitleText"),                     TEXT("パネル タイトル文字")},
	{HEXRGB(0x111111), CLR_INVALID,      TEXT("PanelTitleBorder"),                   TEXT("パネル タイトル外枠")},
	{HEXRGB(0x111111), CLR_INVALID,      TEXT("ProgramInfoBack"),                    TEXT("情報パネル 番組情報背景")},
	{HEXRGB(0xAAAAAA), CLR_INVALID,      TEXT("ProgramInfoText"),                    TEXT("情報パネル 番組情報文字")},
	{HEXRGB(0x111111), CLR_INVALID,      TEXT("ProgramInfoBorder"),                  TEXT("情報パネル 番組情報外枠")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("InformationPanelButtonBack"),         TEXT("情報パネル ボタン背景1")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("InformationPanelButtonBack2"),        TEXT("情報パネル ボタン背景2")},
	{HEXRGB(0x999999), CLR_INVALID,      TEXT("InformationPanelButtonText"),         TEXT("情報パネル ボタン文字")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("InformationPanelButtonBorder"),       TEXT("情報パネル ボタン境界")},
	{HEXRGB(0x4486E8), CLR_INVALID,      TEXT("InformationPanelHotButtonBack"),      TEXT("情報パネル 選択ボタン背景1")},
	{HEXRGB(0x3C76CC), CLR_INVALID,      TEXT("InformationPanelHotButtonBack2"),     TEXT("情報パネル 選択ボタン背景2")},
	{HEXRGB(0xDDDDDD), CLR_INVALID,      TEXT("InformationPanelHotButtonText"),      TEXT("情報パネル 選択ボタン文字")},
	{HEXRGB(0x3C76CC), CLR_INVALID,      TEXT("InformationPanelHotButtonBorder"),    TEXT("情報パネル 選択ボタン境界")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("ProgramListChannelBack"),             TEXT("番組表パネル 局名背景1")},
	{HEXRGB(0x000000), CLR_INVALID,      TEXT("ProgramListChannelBack2"),            TEXT("番組表パネル 局名背景2")},
	{HEXRGB(0xAAAAAA), CLR_INVALID,      TEXT("ProgramListChannelText"),             TEXT("番組表パネル 局名文字")},
	{HEXRGB(0x000000), CLR_INVALID,      TEXT("ProgramListChannelBorder"),           TEXT("番組表パネル 局名外枠")},
	{HEXRGB(0x4486E8), CLR_INVALID,      TEXT("ProgramListCurChannelBack"),          TEXT("番組表パネル 現在局名背景1")},
	{HEXRGB(0x3C76CC), CLR_INVALID,      TEXT("ProgramListCurChannelBack2"),         TEXT("番組表パネル 現在局名背景2")},
	{HEXRGB(0xDDDDDD), CLR_INVALID,      TEXT("ProgramListCurChannelText"),          TEXT("番組表パネル 現在局名文字")},
	{HEXRGB(0x000000), CLR_INVALID,      TEXT("ProgramListCurChannelBorder"),        TEXT("番組表パネル 現在局名外枠")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("ProgramListChannelButtonBack"),       TEXT("番組表パネル ボタン背景1")},
	{HEXRGB(0x000000), CLR_INVALID,      TEXT("ProgramListChannelButtonBack2"),      TEXT("番組表パネル ボタン背景2")},
	{HEXRGB(0xAAAAAA), CLR_INVALID,      TEXT("ProgramListChannelButtonText"),       TEXT("番組表パネル ボタン文字")},
	{HEXRGB(0x000000), CLR_INVALID,      TEXT("ProgramListChannelButtonBorder"),     TEXT("番組表パネル ボタン外枠")},
	{HEXRGB(0x4486E8), CLR_INVALID,      TEXT("ProgramListChannelButtonHotBack"),    TEXT("番組表パネル 選択ボタン背景1")},
	{HEXRGB(0x3C76CC), CLR_INVALID,      TEXT("ProgramListChannelButtonHotBack2"),   TEXT("番組表パネル 選択ボタン背景2")},
	{HEXRGB(0xDDDDDD), CLR_INVALID,      TEXT("ProgramListChannelButtonHotText"),    TEXT("番組表パネル 選択ボタン文字")},
	{HEXRGB(0x3C76CC), CLR_INVALID,      TEXT("ProgramListChannelButtonHotBorder"),  TEXT("番組表パネル 選択ボタン外枠")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("ProgramListBack"),                    TEXT("番組表パネル 番組内容背景1")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("ProgramListBack2"),                   TEXT("番組表パネル 番組内容背景2")},
	{HEXRGB(0x999999), CLR_INVALID,      TEXT("ProgramListText"),                    TEXT("番組表パネル 番組内容文字")},
	{HEXRGB(0x444444), CLR_INVALID,      TEXT("ProgramListBorder"),                  TEXT("番組表パネル 番組内容外枠")},
	{HEXRGB(0x222222), CLR_INVALID,      TEXT("ProgramListCurBack"),                 TEXT("番組表パネル 現在番組内容背景1")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("ProgramListCurBack2"),                TEXT("番組表パネル 現在番組内容背景2")},
	{HEXRGB(0xAAAAAA), CLR_INVALID,      TEXT("ProgramListCurText"),                 TEXT("番組表パネル 現在番組内容文字")},
	{HEXRGB(0x555555), CLR_INVALID,      TEXT("ProgramListCurBorder"),               TEXT("番組表パネル 現在番組内容外枠")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("ProgramListTitleBack"),               TEXT("番組表パネル 番組名背景1")},
	{HEXRGB(0x000000), CLR_INVALID,      TEXT("ProgramListTitleBack2"),              TEXT("番組表パネル 番組名背景2")},
	{HEXRGB(0xAAAAAA), CLR_INVALID,      TEXT("ProgramListTitleText"),               TEXT("番組表パネル 番組名文字")},
	{HEXRGB(0x000000), CLR_INVALID,      TEXT("ProgramListTitleBorder"),             TEXT("番組表パネル 番組名外枠")},
	{HEXRGB(0x4486E8), CLR_INVALID,      TEXT("ProgramListCurTitleBack"),            TEXT("番組表パネル 現在番組名背景1")},
	{HEXRGB(0x3C76CC), CLR_INVALID,      TEXT("ProgramListCurTitleBack2"),           TEXT("番組表パネル 現在番組名背景2")},
	{HEXRGB(0xDDDDDD), CLR_INVALID,      TEXT("ProgramListCurTitleText"),            TEXT("番組表パネル 現在番組名文字")},
	{HEXRGB(0x000000), CLR_INVALID,      TEXT("ProgramListCurTitleBorder"),          TEXT("番組表パネル 現在番組名外枠")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("ChannelPanelChannelNameBack"),        TEXT("チャンネルパネル 局名背景1")},
	{HEXRGB(0x000000), CLR_INVALID,      TEXT("ChannelPanelChannelNameBack2"),       TEXT("チャンネルパネル 局名背景2")},
	{HEXRGB(0xAAAAAA), CLR_INVALID,      TEXT("ChannelPanelChannelNameText"),        TEXT("チャンネルパネル 局名文字")},
	{HEXRGB(0x000000), CLR_INVALID,      TEXT("ChannelPanelChannelNameBorder"),      TEXT("チャンネルパネル 局名外枠")},
	{HEXRGB(0x4486E8), CLR_INVALID,      TEXT("ChannelPanelCurChannelNameBack"),     TEXT("チャンネルパネル 現在局名背景1")},
	{HEXRGB(0x3C76CC), CLR_INVALID,      TEXT("ChannelPanelCurChannelNameBack2"),    TEXT("チャンネルパネル 現在局名背景2")},
	{HEXRGB(0xDDDDDD), CLR_INVALID,      TEXT("ChannelPanelCurChannelNameText"),     TEXT("チャンネルパネル 現在局名文字")},
	{HEXRGB(0x000000), CLR_INVALID,      TEXT("ChannelPanelCurChannelNameBorder"),   TEXT("チャンネルパネル 現在局名外枠")},
	{HEXRGB(0x444444), CLR_INVALID,      TEXT("ChannelPanelEventNameBack"),          TEXT("チャンネルパネル 番組名1背景1")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("ChannelPanelEventNameBack2"),         TEXT("チャンネルパネル 番組名1背景2")},
	{HEXRGB(0x999999), CLR_INVALID,      TEXT("ChannelPanelEventNameText"),          TEXT("チャンネルパネル 番組名1文字")},
	{HEXRGB(0x444444), CLR_INVALID,      TEXT("ChannelPanelEventNameBorder"),        TEXT("チャンネルパネル 番組名1外枠")},
	{HEXRGB(0x222222), CLR_INVALID,      TEXT("ChannelPanelEventName2Back"),         TEXT("チャンネルパネル 番組名2背景1")},
	{HEXRGB(0x222222), CLR_INVALID,      TEXT("ChannelPanelEventName2Back2"),        TEXT("チャンネルパネル 番組名2背景2")},
	{HEXRGB(0x999999), CLR_INVALID,      TEXT("ChannelPanelEventName2Text"),         TEXT("チャンネルパネル 番組名2文字")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("ChannelPanelEventName2Border"),       TEXT("チャンネルパネル 番組名2外枠")},
	{HEXRGB(0x444444), CLR_INVALID,      TEXT("ChannelPanelCurEventNameBack"),       TEXT("チャンネルパネル 選択番組名1背景1")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("ChannelPanelCurEventNameBack2"),      TEXT("チャンネルパネル 選択番組名1背景2")},
	{HEXRGB(0xAAAAAA), CLR_INVALID,      TEXT("ChannelPanelCurEventNameText"),       TEXT("チャンネルパネル 選択番組名1文字")},
	{HEXRGB(0x444444), CLR_INVALID,      TEXT("ChannelPanelCurEventNameBorder"),     TEXT("チャンネルパネル 選択番組名1外枠")},
	{HEXRGB(0x222222), CLR_INVALID,      TEXT("ChannelPanelCurEventName2Back"),      TEXT("チャンネルパネル 選択番組名2背景1")},
	{HEXRGB(0x222222), CLR_INVALID,      TEXT("ChannelPanelCurEventName2Back2"),     TEXT("チャンネルパネル 選択番組名2背景2")},
	{HEXRGB(0xAAAAAA), CLR_INVALID,      TEXT("ChannelPanelCurEventName2Text"),      TEXT("チャンネルパネル 選択番組名2文字")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("ChannelPanelCurEventName2Border"),    TEXT("チャンネルパネル 選択番組名2外枠")},
	{HEXRGB(0x00FF00), CLR_INVALID,      TEXT("ChannelPanelFeaturedMark"),           TEXT("チャンネルパネル 注目マーク背景1")},
	{HEXRGB(0x00FF00), CLR_INVALID,      TEXT("ChannelPanelFeaturedMark2"),          TEXT("チャンネルパネル 注目マーク背景2")},
	{HEXRGB(0x00BF00), CLR_INVALID,      TEXT("ChannelPanelFeaturedMarkBorder"),     TEXT("チャンネルパネル 注目マーク外枠")},
	{HEXRGB(0x2D5899), CLR_INVALID,      TEXT("ChannelPanelProgress"),               TEXT("チャンネルパネル 番組経過時間バー1")},
	{HEXRGB(0x2D5899), CLR_INVALID,      TEXT("ChannelPanelProgress2"),              TEXT("チャンネルパネル 番組経過時間バー2")},
	{HEXRGB(0x2D5899), CLR_INVALID,      TEXT("ChannelPanelProgressBorder"),         TEXT("チャンネルパネル 番組経過時間バー外枠")},
	{HEXRGB(0x3465B0), CLR_INVALID,      TEXT("ChannelPanelCurProgress"),            TEXT("チャンネルパネル 選択番組経過時間バー1")},
	{HEXRGB(0x3465B0), CLR_INVALID,      TEXT("ChannelPanelCurProgress2"),           TEXT("チャンネルパネル 選択番組経過時間バー2")},
	{HEXRGB(0x3465B0), CLR_INVALID,      TEXT("ChannelPanelCurProgressBorder"),      TEXT("チャンネルパネル 選択番組経過時間バー外枠")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("ControlPanelBack"),                   TEXT("操作パネル 背景1")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("ControlPanelBack2"),                  TEXT("操作パネル 背景2")},
	{HEXRGB(0x999999), CLR_INVALID,      TEXT("ControlPanelText"),                   TEXT("操作パネル 文字")},
	{HEXRGB(0x666666), CLR_INVALID,      TEXT("ControlPanelItemBorder"),             TEXT("操作パネル 項目外枠")},
	{HEXRGB(0x4486E8), CLR_INVALID,      TEXT("ControlPanelHighlightBack"),          TEXT("操作パネル 選択背景1")},
	{HEXRGB(0x3C76CC), CLR_INVALID,      TEXT("ControlPanelHighlightBack2"),         TEXT("操作パネル 選択背景2")},
	{HEXRGB(0xDDDDDD), CLR_INVALID,      TEXT("ControlPanelHighlightText"),          TEXT("操作パネル 選択文字")},
	{HEXRGB(0x3C76CC), CLR_INVALID,      TEXT("ControlPanelHighlightBorder"),        TEXT("操作パネル 選択項目外枠")},
	{HEXRGB(0x444444), CLR_INVALID,      TEXT("ControlPanelCheckedBack"),            TEXT("操作パネル チェック背景1")},
	{HEXRGB(0x555555), CLR_INVALID,      TEXT("ControlPanelCheckedBack2"),           TEXT("操作パネル チェック背景2")},
	{HEXRGB(0xDDDDDD), CLR_INVALID,      TEXT("ControlPanelCheckedText"),            TEXT("操作パネル チェック文字")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("ControlPanelCheckedBorder"),          TEXT("操作パネル チェック項目外枠")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("ControlPanelMargin"),                 TEXT("操作パネル 余白")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("CaptionPanelBack"),                   TEXT("字幕パネル 背景")},
	{HEXRGB(0x999999), CLR_INVALID,      TEXT("CaptionPanelText"),                   TEXT("字幕パネル 文字")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("TitleBarBack"),                       TEXT("タイトルバー 背景1")},
	{HEXRGB(0x111111), CLR_INVALID,      TEXT("TitleBarBack2"),                      TEXT("タイトルバー 背景2")},
	{HEXRGB(0xAAAAAA), CLR_INVALID,      TEXT("TitleBarText"),                       TEXT("タイトルバー 文字")},
	{HEXRGB(0x777777), CLR_INVALID,      TEXT("TitleBarTextBorder"),                 TEXT("タイトルバー 文字外枠")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("TitleBarIconBack"),                   TEXT("タイトルバー アイコン背景1")},
	{HEXRGB(0x111111), CLR_INVALID,      TEXT("TitleBarIconBack2"),                  TEXT("タイトルバー アイコン背景2")},
	{HEXRGB(0x999999), CLR_INVALID,      TEXT("TitleBarIcon"),                       TEXT("タイトルバー アイコン")},
	{HEXRGB(0x777777), CLR_INVALID,      TEXT("TitleBarIconBorder"),                 TEXT("タイトルバー アイコン外枠")},
	{HEXRGB(0x4486E8), CLR_INVALID,      TEXT("TitleBarHighlightBack"),              TEXT("タイトルバー 選択背景1")},
	{HEXRGB(0x3C76CC), CLR_INVALID,      TEXT("TitleBarHighlightBack2"),             TEXT("タイトルバー 選択背景2")},
	{HEXRGB(0xDDDDDD), CLR_INVALID,      TEXT("TitleBarHighlightIcon"),              TEXT("タイトルバー 選択アイコン")},
	{HEXRGB(0x3C76CC), CLR_INVALID,      TEXT("TitleBarHighlightIconBorder"),        TEXT("タイトルバー 選択アイコン外枠")},
	{HEXRGB(0x111111), CLR_INVALID,      TEXT("TitleBarBorder"),                     TEXT("タイトルバー 外枠")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("SideBarBack"),                        TEXT("サイドバー 背景1")},
	{HEXRGB(0x111111), CLR_INVALID,      TEXT("SideBarBack2"),                       TEXT("サイドバー 背景2")},
	{HEXRGB(0xAAAAAA), CLR_INVALID,      TEXT("SideBarIcon"),                        TEXT("サイドバー アイコン")},
	{HEXRGB(0x777777), CLR_INVALID,      TEXT("SideBarItemBorder"),                  TEXT("サイドバー 項目外枠")},
	{HEXRGB(0x4486E8), CLR_INVALID,      TEXT("SideBarHighlightBack"),               TEXT("サイドバー 選択背景1")},
	{HEXRGB(0x3C76CC), CLR_INVALID,      TEXT("SideBarHighlightBack2"),              TEXT("サイドバー 選択背景2")},
	{HEXRGB(0xDDDDDD), CLR_INVALID,      TEXT("SideBarHighlightIcon"),               TEXT("サイドバー 選択アイコン")},
	{HEXRGB(0x3C76CC), CLR_INVALID,      TEXT("SideBarHighlightBorder"),             TEXT("サイドバー 選択外枠")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("SideBarCheckBack"),                   TEXT("サイドバー チェック背景1")},
	{HEXRGB(0x444444), CLR_INVALID,      TEXT("SideBarCheckBack2"),                  TEXT("サイドバー チェック背景2")},
	{HEXRGB(0xAAAAAA), CLR_INVALID,      TEXT("SideBarCheckIcon"),                   TEXT("サイドバー チェックアイコン")},
	{HEXRGB(0x222222), CLR_INVALID,      TEXT("SideBarCheckBorder"),                 TEXT("サイドバー チェック外枠")},
	{HEXRGB(0x111111), CLR_INVALID,      TEXT("SideBarBorder"),                      TEXT("サイドバー 外枠")},
	{HEXRGB(0x222222), CLR_INVALID,      TEXT("NotificationBarBack"),                TEXT("通知バー 背景1")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("NotificationBarBack2"),               TEXT("通知バー 背景2")},
	{HEXRGB(0xBBBBBB), CLR_INVALID,      TEXT("NotificationBarText"),                TEXT("通知バー 文字")},
	{HEXRGB(0xFF9F44), CLR_INVALID,      TEXT("NotificationBarWarningText"),         TEXT("通知バー 警告文字")},
	{HEXRGB(0xFF4444), CLR_INVALID,      TEXT("NotificationBarErrorText"),           TEXT("通知バー エラー文字")},
	{HEXRGB(0x111111), HEXRGB(0xFFFFFF), TEXT("EventInfoPopupBack"),                 TEXT("番組情報ポップアップ 背景")},
	{HEXRGB(0xBBBBBB), HEXRGB(0x333333), TEXT("EventInfoPopupText"),                 TEXT("番組情報ポップアップ 文字")},
	{HEXRGB(0xDDDDDD), HEXRGB(0x000000), TEXT("EventInfoPopupEventTitle"),           TEXT("番組情報ポップアップ 番組名")},
	{HEXRGB(0x333333), HEXRGB(0xFFFFFF), TEXT("ProgramGuideBack"),                   TEXT("EPG番組表 背景")},
	{HEXRGB(0x222222), HEXRGB(0x000000), TEXT("ProgramGuideText"),                   TEXT("EPG番組表 番組内容")},
	{HEXRGB(0x000000), CLR_INVALID,      TEXT("ProgramGuideEventTitle"),             TEXT("EPG番組表 番組名")},
	{HEXRGB(0x0000BF), CLR_INVALID,      TEXT("ProgramGuideHighlightText"),          TEXT("EPG番組表 検索番組内容")},
	{HEXRGB(0x0000FF), CLR_INVALID,      TEXT("ProgramGuideHighlightTitle"),         TEXT("EPG番組表 検索番組名")},
	{HEXRGB(0x9999FF), CLR_INVALID,      TEXT("ProgramGuideHighlightBack"),          TEXT("EPG番組表 検索番組背景")},
	{HEXRGB(0x6666FF), CLR_INVALID,      TEXT("ProgramGuideHighlightBorder"),        TEXT("EPG番組表 検索番組枠")},
	{HEXRGB(0xCCFFCC), CLR_INVALID,      TEXT("ProgramGuideFeaturedMark"),           TEXT("EPG番組表 注目マーク背景1")},
	{HEXRGB(0x99FF99), CLR_INVALID,      TEXT("ProgramGuideFeaturedMark2"),          TEXT("EPG番組表 注目マーク背景2")},
	{HEXRGB(0x00EF00), CLR_INVALID,      TEXT("ProgramGuideFeaturedMarkBorder"),     TEXT("EPG番組表 注目マーク外枠")},
	{HEXRGB(0x333333), CLR_INVALID,      TEXT("ProgramGuideChannelBack"),            TEXT("EPG番組表 チャンネル名背景1")},
	{HEXRGB(0x111111), CLR_INVALID,      TEXT("ProgramGuideChannelBack2"),           TEXT("EPG番組表 チャンネル名背景2")},
	{HEXRGB(0x999999), CLR_INVALID,      TEXT("ProgramGuideChannelText"),            TEXT("EPG番組表 チャンネル名文字")},
	{HEXRGB(0xDDDDDD), CLR_INVALID,      TEXT("ProgramGuideChannelHighlightText"),   TEXT("EPG番組表 チャンネル名強調文字")},
	{HEXRGB(0x4486E8), CLR_INVALID,      TEXT("ProgramGuideCurChannelBack"),         TEXT("EPG番組表 チャンネル名選択背景1")},
	{HEXRGB(0x3C76CC), CLR_INVALID,      TEXT("ProgramGuideCurChannelBack2"),        TEXT("EPG番組表 チャンネル名選択背景2")},
	{HEXRGB(0xDDDDDD), CLR_INVALID,      TEXT("ProgramGuideCurChannelText"),         TEXT("EPG番組表 チャンネル名選択文字")},
	{HEXRGB(0x333333), HEXRGB(0xFFFFFF), TEXT("ProgramGuideTimeBack"),               TEXT("EPG番組表 日時背景1")},
	{HEXRGB(0x111111), HEXRGB(0xDDDDDD), TEXT("ProgramGuideTimeBack2"),              TEXT("EPG番組表 日時背景2")},
	{HEXRGB(0x00337F), HEXRGB(0x004CBF), TEXT("ProgramGuideTime0To2Back"),           TEXT("EPG番組表 0～2時背景1")},
	{HEXRGB(0x00193F), HEXRGB(0x00337F), TEXT("ProgramGuideTime0To2Back2"),          TEXT("EPG番組表 0～2時背景2")},
	{HEXRGB(0x00667F), HEXRGB(0x0099BF), TEXT("ProgramGuideTime3To5Back"),           TEXT("EPG番組表 3～5時背景1")},
	{HEXRGB(0x00333F), HEXRGB(0x00667F), TEXT("ProgramGuideTime3To5Back2"),          TEXT("EPG番組表 3～5時背景2")},
	{HEXRGB(0x007F66), HEXRGB(0x00BF99), TEXT("ProgramGuideTime6To8Back"),           TEXT("EPG番組表 6～8時背景1")},
	{HEXRGB(0x003F33), HEXRGB(0x007F66), TEXT("ProgramGuideTime6To8Back2"),          TEXT("EPG番組表 6～8時背景2")},
	{HEXRGB(0x667F00), HEXRGB(0x99BF00), TEXT("ProgramGuideTime9To11Back"),          TEXT("EPG番組表 9～11時背景1")},
	{HEXRGB(0x333F00), HEXRGB(0x667F00), TEXT("ProgramGuideTime9To11Back2"),         TEXT("EPG番組表 9～11時背景2")},
	{HEXRGB(0x7F6600), HEXRGB(0xBF9900), TEXT("ProgramGuideTime12To14Back"),         TEXT("EPG番組表 12～14時背景1")},
	{HEXRGB(0x3F3300), HEXRGB(0x7F6600), TEXT("ProgramGuideTime12To14Back2"),        TEXT("EPG番組表 12～14時背景2")},
	{HEXRGB(0x7F3300), HEXRGB(0xBF4C00), TEXT("ProgramGuideTime15To17Back"),         TEXT("EPG番組表 15～17時背景1")},
	{HEXRGB(0x3F1900), HEXRGB(0x7F3300), TEXT("ProgramGuideTime15To17Back2"),        TEXT("EPG番組表 15～17間背景2")},
	{HEXRGB(0x7F0066), HEXRGB(0xBF0099), TEXT("ProgramGuideTime18To20Back"),         TEXT("EPG番組表 18～20時背景1")},
	{HEXRGB(0x3F0033), HEXRGB(0x7F0066), TEXT("ProgramGuideTime18To20Back2"),        TEXT("EPG番組表 18～20時背景2")},
	{HEXRGB(0x66007F), HEXRGB(0x9900BF), TEXT("ProgramGuideTime21To23Back"),         TEXT("EPG番組表 21～23時背景1")},
	{HEXRGB(0x33003F), HEXRGB(0x66007F), TEXT("ProgramGuideTime21To23Back2"),        TEXT("EPG番組表 21～23時背景2")},
	{HEXRGB(0xBBBBBB), HEXRGB(0xFFFFFF), TEXT("ProgramGuideTimeText"),               TEXT("EPG番組表 時間文字")},
	{HEXRGB(0x888888), CLR_INVALID,      TEXT("ProgramGuideTimeLine"),               TEXT("EPG番組表 時間線")},
	{HEXRGB(0xFF6600), CLR_INVALID,      TEXT("ProgramGuideCurTimeLine"),            TEXT("EPG番組表 現在時刻線")},
	{HEXRGB(0xBBBBBB), HEXRGB(0x000000), TEXT("ProgramGuideDateButtonText"),         TEXT("EPG番組表 日付ボタン文字")},
	{HEXRGB(0xFF3F1F), HEXRGB(0xFF2000), TEXT("ProgramGuideDateButtonSundayText"),   TEXT("EPG番組表 日付ボタン日曜文字")},
	{HEXRGB(0x4F9FFF), HEXRGB(0x0020FF), TEXT("ProgramGuideDateButtonSaturdayText"), TEXT("EPG番組表 日付ボタン土曜文字")},
	{HEXRGB(0x333333), HEXRGB(0xFFFFFF), TEXT("ProgramGuideDateButtonBack"),         TEXT("EPG番組表 日付ボタン背景1")},
	{HEXRGB(0x111111), HEXRGB(0xDDDDDD), TEXT("ProgramGuideDateButtonBack2"),        TEXT("EPG番組表 日付ボタン背景2")},
	{HEXRGB(0x111111), HEXRGB(0xDDDDDD), TEXT("ProgramGuideDateButtonBorder"),       TEXT("EPG番組表 日付ボタン外枠")},
	{HEXRGB(0xDDDDDD), HEXRGB(0x000000), TEXT("ProgramGuideDateButtonCurText"),      TEXT("EPG番組表 日付ボタン選択文字")},
	{HEXRGB(0x111111), HEXRGB(0xDDDDDD), TEXT("ProgramGuideDateButtonCurBack"),      TEXT("EPG番組表 日付ボタン選択背景1")},
	{HEXRGB(0x333333), HEXRGB(0xFFFFFF), TEXT("ProgramGuideDateButtonCurBack2"),     TEXT("EPG番組表 日付ボタン選択背景2")},
	{HEXRGB(0x111111), HEXRGB(0xDDDDDD), TEXT("ProgramGuideDateButtonCurBorder"),    TEXT("EPG番組表 日付ボタン選択外枠")},
	{HEXRGB(0xBBBBBB), HEXRGB(0x000000), TEXT("ProgramGuideDateButtonHotText"),      TEXT("EPG番組表 日付ボタンホット文字")},
	{HEXRGB(0x111111), HEXRGB(0xDDDDDD), TEXT("ProgramGuideDateButtonHotBack"),      TEXT("EPG番組表 日付ボタンホット背景1")},
	{HEXRGB(0x333333), HEXRGB(0xFFFFFF), TEXT("ProgramGuideDateButtonHotBack2"),     TEXT("EPG番組表 日付ボタンホット背景2")},
	{HEXRGB(0x111111), HEXRGB(0xDDDDDD), TEXT("ProgramGuideDateButtonHotBorder"),    TEXT("EPG番組表 日付ボタンホット外枠")},
	{HEXRGB(0xFFFFE0), CLR_INVALID,      TEXT("EPGContentNews"),                     TEXT("EPG番組表 ニュース番組")},
	{HEXRGB(0xE0E0FF), CLR_INVALID,      TEXT("EPGContentSports"),                   TEXT("EPG番組表 スポーツ番組")},
	{HEXRGB(0xFFE0F0), CLR_INVALID,      TEXT("EPGContentInformation"),              TEXT("EPG番組表 情報番組")},
	{HEXRGB(0xFFE0E0), CLR_INVALID,      TEXT("EPGContentDrama"),                    TEXT("EPG番組表 ドラマ")},
	{HEXRGB(0xE0FFE0), CLR_INVALID,      TEXT("EPGContentMusic"),                    TEXT("EPG番組表 音楽番組")},
	{HEXRGB(0xE0FFFF), CLR_INVALID,      TEXT("EPGContentVariety"),                  TEXT("EPG番組表 バラエティ番組")},
	{HEXRGB(0xFFF0E0), CLR_INVALID,      TEXT("EPGContentMovie"),                    TEXT("EPG番組表 映画")},
	{HEXRGB(0xFFE0FF), CLR_INVALID,      TEXT("EPGContentAnime"),                    TEXT("EPG番組表 アニメ/特撮")},
	{HEXRGB(0xFFFFE0), CLR_INVALID,      TEXT("EPGContentDocumentary"),              TEXT("EPG番組表 ドキュメンタリー/教養番組")},
	{HEXRGB(0xFFF0E0), CLR_INVALID,      TEXT("EPGContentTheater"),                  TEXT("EPG番組表 劇場/公演")},
	{HEXRGB(0xE0F0FF), CLR_INVALID,      TEXT("EPGContentEducation"),                TEXT("EPG番組表 趣味/教育番組")},
	{HEXRGB(0xE0F0FF), CLR_INVALID,      TEXT("EPGContentWelfare"),                  TEXT("EPG番組表 福祉番組")},
	{HEXRGB(0xF0F0F0), CLR_INVALID,      TEXT("EPGContentOther"),                    TEXT("EPG番組表 その他の番組")},
};

const CColorScheme::GradientInfo CColorScheme::m_GradientInfoList[NUM_GRADIENTS] = {
	{TEXT("StatusBack"),                        Theme::GradientDirection::Vert, false, COLOR_STATUSBACK1,                            COLOR_STATUSBACK2},
	{TEXT("StatusBottomItemBack"),              Theme::GradientDirection::Vert, false, COLOR_STATUSBOTTOMITEMBACK1,                  COLOR_STATUSBOTTOMITEMBACK2},
	{TEXT("StatusHighlightBack"),               Theme::GradientDirection::Vert, true,  COLOR_STATUSHIGHLIGHTBACK1,                   COLOR_STATUSHIGHLIGHTBACK2},
	{TEXT("StatusEventProgressBack"),           Theme::GradientDirection::Vert, true,  COLOR_STATUSEVENTPROGRESSBACK1,               COLOR_STATUSEVENTPROGRESSBACK2},
	{TEXT("StatusEventProgressElapsed"),        Theme::GradientDirection::Vert, true,  COLOR_STATUSEVENTPROGRESSELAPSED1,            COLOR_STATUSEVENTPROGRESSELAPSED2},
	{TEXT("PanelTabBack"),                      Theme::GradientDirection::Vert, true,  COLOR_PANELTABBACK1,                          COLOR_PANELTABBACK2},
	{TEXT("PanelCurTabBack"),                   Theme::GradientDirection::Vert, true,  COLOR_PANELCURTABBACK1,                       COLOR_PANELCURTABBACK2},
	{TEXT("PanelTabMargin"),                    Theme::GradientDirection::Vert, false, COLOR_PANELTABMARGIN1,                        COLOR_PANELTABMARGIN2},
	{TEXT("PanelTitleBack"),                    Theme::GradientDirection::Vert, true,  COLOR_PANELTITLEBACK1,                        COLOR_PANELTITLEBACK2},
	{TEXT("InformationPanelButtonBack"),        Theme::GradientDirection::Vert, true,  COLOR_INFORMATIONPANEL_BUTTONBACK1,           COLOR_INFORMATIONPANEL_BUTTONBACK2},
	{TEXT("InformationPanelHotButtonBack"),     Theme::GradientDirection::Vert, true,  COLOR_INFORMATIONPANEL_HOTBUTTONBACK1,        COLOR_INFORMATIONPANEL_HOTBUTTONBACK2},
	{TEXT("ProgramListChannelBack"),            Theme::GradientDirection::Vert, true,  COLOR_PROGRAMLISTPANEL_CHANNELBACK1,          COLOR_PROGRAMLISTPANEL_CHANNELBACK2},
	{TEXT("ProgramListCurChannelBack"),         Theme::GradientDirection::Vert, true,  COLOR_PROGRAMLISTPANEL_CURCHANNELBACK1,       COLOR_PROGRAMLISTPANEL_CURCHANNELBACK2},
	{TEXT("ProgramListChannelButtonBack"),      Theme::GradientDirection::Vert, true,  COLOR_PROGRAMLISTPANEL_CHANNELBUTTONBACK1,    COLOR_PROGRAMLISTPANEL_CHANNELBUTTONBACK2},
	{TEXT("ProgramListChannelButtonHotBack"),   Theme::GradientDirection::Vert, true,  COLOR_PROGRAMLISTPANEL_CHANNELBUTTONHOTBACK1, COLOR_PROGRAMLISTPANEL_CHANNELBUTTONHOTBACK2},
	{TEXT("ProgramListBack"),                   Theme::GradientDirection::Vert, true,  COLOR_PROGRAMLISTPANEL_EVENTBACK1,            COLOR_PROGRAMLISTPANEL_EVENTBACK2},
	{TEXT("ProgramListCurBack"),                Theme::GradientDirection::Vert, true,  COLOR_PROGRAMLISTPANEL_CUREVENTBACK1,         COLOR_PROGRAMLISTPANEL_CUREVENTBACK2},
	{TEXT("ProgramListTitleBack"),              Theme::GradientDirection::Vert, true,  COLOR_PROGRAMLISTPANEL_TITLEBACK1,            COLOR_PROGRAMLISTPANEL_TITLEBACK2},
	{TEXT("ProgramListCurTitleBack"),           Theme::GradientDirection::Vert, true,  COLOR_PROGRAMLISTPANEL_CURTITLEBACK1,         COLOR_PROGRAMLISTPANEL_CURTITLEBACK2},
	{TEXT("ChannelPanelChannelNameBack"),       Theme::GradientDirection::Vert, true,  COLOR_CHANNELPANEL_CHANNELNAMEBACK1,          COLOR_CHANNELPANEL_CHANNELNAMEBACK2},
	{TEXT("ChannelPanelCurChannelNameBack"),    Theme::GradientDirection::Vert, true,  COLOR_CHANNELPANEL_CURCHANNELNAMEBACK1,       COLOR_CHANNELPANEL_CURCHANNELNAMEBACK2},
	{TEXT("ChannelPanelEventNameBack"),         Theme::GradientDirection::Vert, true,  COLOR_CHANNELPANEL_EVENTNAME1BACK1,           COLOR_CHANNELPANEL_EVENTNAME1BACK2},
	{TEXT("ChannelPanelEventName2Back"),        Theme::GradientDirection::Vert, true,  COLOR_CHANNELPANEL_EVENTNAME2BACK1,           COLOR_CHANNELPANEL_EVENTNAME2BACK2},
	{TEXT("ChannelPanelCurEventNameBack"),      Theme::GradientDirection::Vert, true,  COLOR_CHANNELPANEL_CUREVENTNAME1BACK1,        COLOR_CHANNELPANEL_CUREVENTNAME1BACK2},
	{TEXT("ChannelPanelCurEventName2Back"),     Theme::GradientDirection::Vert, true,  COLOR_CHANNELPANEL_CUREVENTNAME2BACK1,        COLOR_CHANNELPANEL_CUREVENTNAME2BACK2},
	{TEXT("ChannelPanelFeaturedMark"),          Theme::GradientDirection::Vert, true,  COLOR_CHANNELPANEL_FEATUREDMARK1,             COLOR_CHANNELPANEL_FEATUREDMARK2},
	{TEXT("ChannelPanelProgress"),              Theme::GradientDirection::Vert, true,  COLOR_CHANNELPANEL_PROGRESS1,                 COLOR_CHANNELPANEL_PROGRESS2},
	{TEXT("ChannelPanelCurProgress"),           Theme::GradientDirection::Vert, true,  COLOR_CHANNELPANEL_CURPROGRESS1,              COLOR_CHANNELPANEL_CURPROGRESS2},
	{TEXT("ControlPanelBack"),                  Theme::GradientDirection::Vert, true,  COLOR_CONTROLPANELBACK1,                      COLOR_CONTROLPANELBACK2},
	{TEXT("ControlPanelHighlightBack"),         Theme::GradientDirection::Vert, true,  COLOR_CONTROLPANELHIGHLIGHTBACK1,             COLOR_CONTROLPANELHIGHLIGHTBACK2},
	{TEXT("ControlPanelCheckedBack"),           Theme::GradientDirection::Vert, true,  COLOR_CONTROLPANELCHECKEDBACK1,               COLOR_CONTROLPANELCHECKEDBACK2},
	{TEXT("TitleBarBack"),                      Theme::GradientDirection::Vert, true,  COLOR_TITLEBARBACK1,                          COLOR_TITLEBARBACK2},
	{TEXT("TitleBarIconBack"),                  Theme::GradientDirection::Vert, true,  COLOR_TITLEBARICONBACK1,                      COLOR_TITLEBARICONBACK2},
	{TEXT("TitleBarHighlightBack"),             Theme::GradientDirection::Vert, true,  COLOR_TITLEBARHIGHLIGHTBACK1,                 COLOR_TITLEBARHIGHLIGHTBACK2},
	{TEXT("SideBarBack"),                       Theme::GradientDirection::Horz, true,  COLOR_SIDEBARBACK1,                           COLOR_SIDEBARBACK2},
	{TEXT("SideBarHighlightBack"),              Theme::GradientDirection::Horz, true,  COLOR_SIDEBARHIGHLIGHTBACK1,                  COLOR_SIDEBARHIGHLIGHTBACK2},
	{TEXT("SideBarCheckBack"),                  Theme::GradientDirection::Horz, true,  COLOR_SIDEBARCHECKBACK1,                      COLOR_SIDEBARCHECKBACK2},
	{TEXT("NotificationBarBack"),               Theme::GradientDirection::Vert, true,  COLOR_NOTIFICATIONBARBACK1,                   COLOR_NOTIFICATIONBARBACK2},
	{TEXT("ProgramGuideFeaturedMark"),          Theme::GradientDirection::Horz, true,  COLOR_PROGRAMGUIDE_FEATUREDMARK1,             COLOR_PROGRAMGUIDE_FEATUREDMARK2},
	{TEXT("ProgramGuideChannelBack"),           Theme::GradientDirection::Vert, true,  COLOR_PROGRAMGUIDE_CHANNELBACK1,              COLOR_PROGRAMGUIDE_CHANNELBACK2},
	{TEXT("ProgramGuideCurChannelBack"),        Theme::GradientDirection::Vert, true,  COLOR_PROGRAMGUIDE_CURCHANNELBACK1,           COLOR_PROGRAMGUIDE_CURCHANNELBACK2},
	{TEXT("ProgramGuideTimeBack"),              Theme::GradientDirection::Horz, true,  COLOR_PROGRAMGUIDE_TIMEBACK1,                 COLOR_PROGRAMGUIDE_TIMEBACK2},
	{TEXT("ProgramGuideTime0To2Back"),          Theme::GradientDirection::Horz, true,  COLOR_PROGRAMGUIDE_TIMEBACK_0TO2_1,           COLOR_PROGRAMGUIDE_TIMEBACK_0TO2_2},
	{TEXT("ProgramGuideTime3To5Back"),          Theme::GradientDirection::Horz, true,  COLOR_PROGRAMGUIDE_TIMEBACK_3TO5_1,           COLOR_PROGRAMGUIDE_TIMEBACK_3TO5_2},
	{TEXT("ProgramGuideTime6To8Back"),          Theme::GradientDirection::Horz, true,  COLOR_PROGRAMGUIDE_TIMEBACK_6TO8_1,           COLOR_PROGRAMGUIDE_TIMEBACK_6TO8_2},
	{TEXT("ProgramGuideTime9To11Back"),         Theme::GradientDirection::Horz, true,  COLOR_PROGRAMGUIDE_TIMEBACK_9TO11_1,          COLOR_PROGRAMGUIDE_TIMEBACK_9TO11_2},
	{TEXT("ProgramGuideTime12To14Back"),        Theme::GradientDirection::Horz, true,  COLOR_PROGRAMGUIDE_TIMEBACK_12TO14_1,         COLOR_PROGRAMGUIDE_TIMEBACK_12TO14_2},
	{TEXT("ProgramGuideTime15To17Back"),        Theme::GradientDirection::Horz, true,  COLOR_PROGRAMGUIDE_TIMEBACK_15TO17_1,         COLOR_PROGRAMGUIDE_TIMEBACK_15TO17_2},
	{TEXT("ProgramGuideTime18To20Back"),        Theme::GradientDirection::Horz, true,  COLOR_PROGRAMGUIDE_TIMEBACK_18TO20_1,         COLOR_PROGRAMGUIDE_TIMEBACK_18TO20_2},
	{TEXT("ProgramGuideTime21To23Back"),        Theme::GradientDirection::Horz, true,  COLOR_PROGRAMGUIDE_TIMEBACK_21TO23_1,         COLOR_PROGRAMGUIDE_TIMEBACK_21TO23_2},
	{TEXT("ProgramGuideDateButtonBack"),        Theme::GradientDirection::Vert, true,  COLOR_PROGRAMGUIDE_DATEBUTTON_BACK1,          COLOR_PROGRAMGUIDE_DATEBUTTON_BACK2},
	{TEXT("ProgramGuideDateButtonCurBack"),     Theme::GradientDirection::Vert, true,  COLOR_PROGRAMGUIDE_DATEBUTTON_CURBACK1,       COLOR_PROGRAMGUIDE_DATEBUTTON_CURBACK2},
	{TEXT("ProgramGuideDateButtonHotBack"),     Theme::GradientDirection::Vert, true,  COLOR_PROGRAMGUIDE_DATEBUTTON_HOTBACK1,       COLOR_PROGRAMGUIDE_DATEBUTTON_HOTBACK2},
	{TEXT("ProgramGuideFavoriteButtonBack"),    Theme::GradientDirection::Vert, true,  -1,                                           -1},
	{TEXT("ProgramGuideFavoriteButtonCurBack"), Theme::GradientDirection::Vert, true,  -1,                                           -1},
	{TEXT("ProgramGuideFavoriteButtonHotBack"), Theme::GradientDirection::Vert, true,  -1,                                           -1},
};

const CColorScheme::BorderInfo CColorScheme::m_BorderInfoList[NUM_BORDERS] = {
	{TEXT("ScreenBorder"),                           Theme::BorderType::None,   COLOR_SCREENBORDER},
	{TEXT("WindowFrameBorder"),                      Theme::BorderType::None,   COLOR_WINDOWFRAMEBORDER},
	{TEXT("WindowActiveFrameBorder"),                Theme::BorderType::None,   COLOR_WINDOWACTIVEFRAMEBORDER},
	{TEXT("StatusBorder"),                           Theme::BorderType::Raised, COLOR_STATUSBORDER},
	{TEXT("StatusItemBorder"),                       Theme::BorderType::None,   COLOR_STATUSITEMBORDER},
	{TEXT("StatusBottomItemBorder"),                 Theme::BorderType::None,   COLOR_STATUSBOTTOMITEMBORDER},
	{TEXT("StatusHighlightBorder"),                  Theme::BorderType::None,   COLOR_STATUSHIGHLIGHTBORDER},
	{TEXT("StatusEventProgressBorder"),              Theme::BorderType::None,   COLOR_STATUSEVENTPROGRESSBORDER},
	{TEXT("StatusEventProgressElapsedBorder"),       Theme::BorderType::None,   COLOR_STATUSEVENTPROGRESSELAPSEDBORDER},
	{TEXT("TitleBarBorder"),                         Theme::BorderType::Raised, COLOR_TITLEBARBORDER},
	{TEXT("TitleBarCaptionBorder"),                  Theme::BorderType::None,   COLOR_TITLEBARTEXTBORDER},
	{TEXT("TitleBarIconBorder"),                     Theme::BorderType::None,   COLOR_TITLEBARICONBORDER},
	{TEXT("TitleBarHighlightBorder"),                Theme::BorderType::None,   COLOR_TITLEBARHIGHLIGHTBORDER},
	{TEXT("SideBarBorder"),                          Theme::BorderType::Raised, COLOR_SIDEBARBORDER},
	{TEXT("SideBarItemBorder"),                      Theme::BorderType::None,   COLOR_SIDEBARITEMBORDER},
	{TEXT("SideBarHighlightBorder"),                 Theme::BorderType::None,   COLOR_SIDEBARHIGHLIGHTBORDER},
	{TEXT("SideBarCheckBorder"),                     Theme::BorderType::Sunken, COLOR_SIDEBARCHECKBORDER},
	{TEXT("ProgramGuideStatusBorder"),               Theme::BorderType::Sunken, COLOR_STATUSBORDER},
	{TEXT("PanelTabBorder"),                         Theme::BorderType::Solid,  COLOR_PANELTABBORDER},
	{TEXT("PanelCurTabBorder"),                      Theme::BorderType::Solid,  COLOR_PANELCURTABBORDER},
	{TEXT("PanelTabMarginBorder"),                   Theme::BorderType::None,   COLOR_PANELTABMARGINBORDER},
	{TEXT("PanelTitleBorder"),                       Theme::BorderType::Raised, COLOR_PANELTITLEBORDER},
	{TEXT("InformationPanelEventInfoBorder"),        Theme::BorderType::None,   COLOR_INFORMATIONPANEL_EVENTINFOBORDER},
	{TEXT("InformationPanelButtonBorder"),           Theme::BorderType::None,   COLOR_INFORMATIONPANEL_BUTTONBORDER},
	{TEXT("InformationPanelHotButtonBorder"),        Theme::BorderType::None,   COLOR_INFORMATIONPANEL_HOTBUTTONBORDER},
	{TEXT("ProgramListPanelChannelBorder"),          Theme::BorderType::None,   COLOR_PROGRAMLISTPANEL_CHANNELBORDER},
	{TEXT("ProgramListPanelCurChannelBorder"),       Theme::BorderType::None,   COLOR_PROGRAMLISTPANEL_CURCHANNELBORDER},
	{TEXT("ProgramListPanelChannelButtonBorder"),    Theme::BorderType::None,   COLOR_PROGRAMLISTPANEL_CHANNELBUTTONBORDER},
	{TEXT("ProgramListPanelChannelButtonHotBorder"), Theme::BorderType::None,   COLOR_PROGRAMLISTPANEL_CHANNELBUTTONHOTBORDER},
	{TEXT("ProgramListPanelEventBorder"),            Theme::BorderType::None,   COLOR_PROGRAMLISTPANEL_EVENTBORDER},
	{TEXT("ProgramListPanelCurEventBorder"),         Theme::BorderType::None,   COLOR_PROGRAMLISTPANEL_CUREVENTBORDER},
	{TEXT("ProgramListPanelTitleBorder"),            Theme::BorderType::None,   COLOR_PROGRAMLISTPANEL_TITLEBORDER},
	{TEXT("ProgramListPanelCurTitleBorder"),         Theme::BorderType::None,   COLOR_PROGRAMLISTPANEL_CURTITLEBORDER},
	{TEXT("ChannelPanelChannelNameBorder"),          Theme::BorderType::None,   COLOR_CHANNELPANEL_CHANNELNAMEBORDER},
	{TEXT("ChannelPanelCurChannelNameBorder"),       Theme::BorderType::None,   COLOR_CHANNELPANEL_CURCHANNELNAMEBORDER},
	{TEXT("ChannelPanelEventNameBorder"),            Theme::BorderType::None,   COLOR_CHANNELPANEL_EVENTNAME1BORDER},
	{TEXT("ChannelPanelEventName2Border"),           Theme::BorderType::None,   COLOR_CHANNELPANEL_EVENTNAME2BORDER},
	{TEXT("ChannelPanelCurEventNameBorder"),         Theme::BorderType::None,   COLOR_CHANNELPANEL_CUREVENTNAME1BORDER},
	{TEXT("ChannelPanelCurEventName2Border"),        Theme::BorderType::None,   COLOR_CHANNELPANEL_CUREVENTNAME2BORDER},
	{TEXT("ChannelPanelFeaturedMarkBorder"),         Theme::BorderType::Solid,  COLOR_CHANNELPANEL_FEATUREDMARKBORDER},
	{TEXT("ChannelPanelProgressBorder"),             Theme::BorderType::None,   COLOR_CHANNELPANEL_PROGRESSBORDER},
	{TEXT("ChannelPanelCurProgressBorder"),          Theme::BorderType::None,   COLOR_CHANNELPANEL_CURPROGRESSBORDER},
	{TEXT("ControlPanelItemBorder"),                 Theme::BorderType::None,   COLOR_CONTROLPANELITEMBORDER},
	{TEXT("ControlPanelHighlightBorder"),            Theme::BorderType::None,   COLOR_CONTROLPANELHIGHLIGHTBORDER},
	{TEXT("ControlPanelCheckedBorder"),              Theme::BorderType::None,   COLOR_CONTROLPANELCHECKEDBORDER},
	{TEXT("ProgramGuideFeaturedMarkBorder"),         Theme::BorderType::Solid,  COLOR_PROGRAMGUIDE_FEATUREDMARKBORDER},
	{TEXT("ProgramGuideDateButtonBorder"),           Theme::BorderType::Raised, COLOR_PROGRAMGUIDE_DATEBUTTON_BORDER},
	{TEXT("ProgramGuideDateButtonCurBorder"),        Theme::BorderType::Sunken, COLOR_PROGRAMGUIDE_DATEBUTTON_CURBORDER},
	{TEXT("ProgramGuideDateButtonHotBorder"),        Theme::BorderType::Raised, COLOR_PROGRAMGUIDE_DATEBUTTON_HOTBORDER},
	{TEXT("ProgramGuideTimeButtonBorder"),           Theme::BorderType::Raised, -1},
	{TEXT("ProgramGuideTimeButtonCurBorder"),        Theme::BorderType::Sunken, -1},
	{TEXT("ProgramGuideTimeButtonHotBorder"),        Theme::BorderType::Raised, -1},
	{TEXT("ProgramGuideFavoriteButtonBorder"),       Theme::BorderType::Raised, -1},
	{TEXT("ProgramGuideFavoriteButtonCurBorder"),    Theme::BorderType::Sunken, -1},
	{TEXT("ProgramGuideFavoriteButtonHotBorder"),    Theme::BorderType::Raised, -1},
};

const Theme::BorderType CColorScheme::m_CustomDefaultBorderList[NUM_BORDERS] = {
	Theme::BorderType::None,
	Theme::BorderType::None,
	Theme::BorderType::None,
	Theme::BorderType::Raised,
	Theme::BorderType::None,
	Theme::BorderType::None,
	Theme::BorderType::Sunken,
	Theme::BorderType::None,
	Theme::BorderType::None,
	Theme::BorderType::Raised,
	Theme::BorderType::None,
	Theme::BorderType::None,
	Theme::BorderType::Sunken,
	Theme::BorderType::Raised,
	Theme::BorderType::None,
	Theme::BorderType::Sunken,
	Theme::BorderType::Sunken,
	Theme::BorderType::Sunken,
	Theme::BorderType::None,
	Theme::BorderType::Solid,
	Theme::BorderType::None,
	Theme::BorderType::Raised,
	Theme::BorderType::None,
	Theme::BorderType::None,
	Theme::BorderType::Sunken,
	Theme::BorderType::Solid,
	Theme::BorderType::Solid,
	Theme::BorderType::None,
	Theme::BorderType::Sunken,
	Theme::BorderType::Solid,
	Theme::BorderType::Solid,
	Theme::BorderType::Solid,
	Theme::BorderType::Solid,
	Theme::BorderType::Solid,
	Theme::BorderType::Solid,
	Theme::BorderType::Solid,
	Theme::BorderType::Solid,
	Theme::BorderType::Solid,
	Theme::BorderType::Solid,
	Theme::BorderType::Solid,
	Theme::BorderType::None,
	Theme::BorderType::None,
	Theme::BorderType::None,
	Theme::BorderType::Sunken,
	Theme::BorderType::Sunken,
	Theme::BorderType::Solid,
	Theme::BorderType::Raised,
	Theme::BorderType::Sunken,
	Theme::BorderType::Raised,
	Theme::BorderType::Raised,
	Theme::BorderType::Sunken,
	Theme::BorderType::Raised,
	Theme::BorderType::Raised,
	Theme::BorderType::Sunken,
	Theme::BorderType::Raised,
};


CColorScheme::CColorScheme()
{
	SetDefault();
}


CColorScheme::CColorScheme(const CColorScheme &ColorScheme)
{
	*this = ColorScheme;
}


COLORREF CColorScheme::GetColor(int Type) const
{
	if (Type < 0 || Type >= NUM_COLORS)
		return CLR_INVALID;
	return m_ColorList[Type];
}


COLORREF CColorScheme::GetColor(LPCTSTR pszText) const
{
	for (int i = 0; i < NUM_COLORS; i++) {
		if (::lstrcmpi(m_ColorInfoList[i].pszText, pszText) == 0)
			return m_ColorList[i];
	}
	return CLR_INVALID;
}


bool CColorScheme::SetColor(int Type, COLORREF Color)
{
	if (Type < 0 || Type >= NUM_COLORS)
		return false;
	m_ColorList[Type] = Color;
	return true;
}


Theme::GradientType CColorScheme::GetGradientType(int Gradient) const
{
	if (Gradient < 0 || Gradient >= NUM_GRADIENTS)
		return Theme::GradientType::Normal;
	return m_FillList[Gradient].Gradient.Type;
}


Theme::GradientType CColorScheme::GetGradientType(LPCTSTR pszText) const
{
	int Length = ::lstrlen(pszText);
	if (Length > 8 && ::lstrcmpi(pszText + (Length - 8), TEXT("Gradient")) == 0) {
		Length -= 8;
		for (int i = 0; i < NUM_GRADIENTS; i++) {
			if (::StrCmpNI(m_GradientInfoList[i].pszText, pszText, Length) == 0
					&& m_GradientInfoList[i].pszText[Length] == _T('\0'))
				return m_FillList[i].Gradient.Type;
		}
	}
	return Theme::GradientType::Normal;
}


bool CColorScheme::SetGradientStyle(int Gradient, const GradientStyle &Style)
{
	if (Gradient < 0 || Gradient >= NUM_GRADIENTS)
		return false;
	m_FillList[Gradient].Gradient.Type = Style.Type;
	m_FillList[Gradient].Gradient.Direction = Style.Direction;
	return true;
}


bool CColorScheme::GetGradientStyle(int Gradient, GradientStyle *pStyle) const
{
	if (Gradient < 0 || Gradient >= NUM_GRADIENTS)
		return false;
	*pStyle = m_FillList[Gradient].Gradient;
	return true;
}


bool CColorScheme::GetGradientStyle(int Gradient, Theme::GradientStyle *pStyle) const
{
	if (Gradient < 0 || Gradient >= NUM_GRADIENTS)
		return false;
	pStyle->Type = m_FillList[Gradient].Gradient.Type;
	pStyle->Direction = m_FillList[Gradient].Gradient.Direction;
	if (m_GradientInfoList[Gradient].Color1 >= 0) {
		pStyle->Color1 = m_ColorList[m_GradientInfoList[Gradient].Color1];
		pStyle->Color2 = m_ColorList[m_GradientInfoList[Gradient].Color2];
	} else {
		pStyle->Color1 = Theme::ThemeColor();
		pStyle->Color2 = Theme::ThemeColor();
	}
	return true;
}


bool CColorScheme::GetFillStyle(int Gradient, Theme::FillStyle *pStyle) const
{
	if (Gradient < 0 || Gradient >= NUM_GRADIENTS)
		return false;
	pStyle->Type = m_FillList[Gradient].Type;
	GetGradientStyle(Gradient, &pStyle->Gradient);
	return true;
}


Theme::BorderType CColorScheme::GetBorderType(int Border) const
{
	if (Border < 0 || Border >= NUM_BORDERS)
		return Theme::BorderType::None;
	return m_BorderList[Border];
}


bool CColorScheme::SetBorderType(int Border, Theme::BorderType Type)
{
	if (Border < 0 || Border >= NUM_BORDERS
			|| Type < Theme::BorderType::None || Type > Theme::BorderType::Raised)
		return false;
	m_BorderList[Border] = Type;
	return true;
}


bool CColorScheme::GetBorderStyle(int Border, Theme::BorderStyle *pStyle) const
{
	if (Border < 0 || Border >= NUM_BORDERS)
		return false;
	pStyle->Type = m_BorderList[Border];
	if (m_BorderInfoList[Border].Color >= 0)
		pStyle->Color = m_ColorList[m_BorderInfoList[Border].Color];
	else
		pStyle->Color = Theme::ThemeColor();
	return true;
}


void CColorScheme::SetName(LPCTSTR pszName)
{
	StringUtility::Assign(m_Name, pszName);
}


bool CColorScheme::Load(CSettings &Settings)
{
	TCHAR szText[MAX_COLORSCHEME_NAME];

	if (!Settings.SetSection(TEXT("ColorScheme")))
		return false;

	if (Settings.Read(TEXT("Name"), szText, lengthof(szText)))
		SetName(szText);

	TCHAR szBase[8];
	if (Settings.Read(TEXT("Base"), szBase, lengthof(szBase))) {
		if (::lstrcmpi(szBase, TEXT("dark")) == 0)
			m_BaseScheme = BaseSchemeType::Dark;
		else if (::lstrcmpi(szBase, TEXT("light")) == 0)
			m_BaseScheme = BaseSchemeType::Light;
	}

	m_LoadedFlags.reset();
	for (int i = 0; i < NUM_COLORS; i++) {
		if (Settings.ReadColor(m_ColorInfoList[i].pszText, &m_ColorList[i]))
			SetLoadedFlag(i);
	}

	for (int i = 0; i < NUM_GRADIENTS; i++) {
		if (m_GradientInfoList[i].Color1 >= 0
				&& IsLoaded(m_GradientInfoList[i].Color1)
				&& !IsLoaded(m_GradientInfoList[i].Color2)) {
			m_ColorList[m_GradientInfoList[i].Color2] = m_ColorList[m_GradientInfoList[i].Color1];
			SetLoadedFlag(m_GradientInfoList[i].Color2);
			m_FillList[i].Gradient.Type = Theme::GradientType::Normal;
		}
	}

	static const struct {
		int To, From;
	} ColorMap[] = {
//		{COLOR_STATUSBORDER,                          COLOR_STATUSBACK1},
		{COLOR_STATUSBOTTOMITEMBACK1,                 COLOR_STATUSBACK2},
		{COLOR_STATUSBOTTOMITEMBACK2,                 COLOR_STATUSBOTTOMITEMBACK1},
		{COLOR_STATUSBOTTOMITEMTEXT,                  COLOR_STATUSTEXT},
		{COLOR_STATUSBOTTOMITEMBORDER,                COLOR_STATUSBOTTOMITEMBACK1},
		{COLOR_STATUSEVENTPROGRESSBORDER,             COLOR_STATUSEVENTPROGRESSBACK1},
		{COLOR_STATUSEVENTPROGRESSELAPSEDBORDER,      COLOR_STATUSEVENTPROGRESSELAPSED1},
		{COLOR_WINDOWFRAMEBORDER,                     COLOR_WINDOWFRAMEBACK},
		{COLOR_WINDOWACTIVEFRAMEBACK,                 COLOR_WINDOWFRAMEBACK},
		{COLOR_WINDOWACTIVEFRAMEBORDER,               COLOR_WINDOWACTIVEFRAMEBACK},
		{COLOR_INFORMATIONPANEL_EVENTINFOBORDER,      COLOR_PROGRAMINFOBACK},
		{COLOR_INFORMATIONPANEL_BUTTONBACK1,          COLOR_PANELBACK},
		{COLOR_INFORMATIONPANEL_BUTTONBACK2,          COLOR_INFORMATIONPANEL_BUTTONBACK1},
		{COLOR_INFORMATIONPANEL_BUTTONTEXT,           COLOR_PANELTEXT},
		{COLOR_INFORMATIONPANEL_HOTBUTTONBACK1,       COLOR_PANELBACK},
		{COLOR_INFORMATIONPANEL_HOTBUTTONBACK2,       COLOR_INFORMATIONPANEL_HOTBUTTONBACK1},
		{COLOR_INFORMATIONPANEL_HOTBUTTONTEXT,        COLOR_PANELTEXT},
		{COLOR_PROGRAMLISTPANEL_CUREVENTTEXT,         COLOR_PROGRAMLISTPANEL_EVENTTEXT},
		{COLOR_PROGRAMLISTPANEL_CURTITLETEXT,         COLOR_PROGRAMLISTPANEL_TITLETEXT},
		{COLOR_PANELTABLINE,                          COLOR_PANELTABBORDER},
//		{COLOR_PANELTITLEBORDER,                      COLOR_PANELTITLEBACK1},
		{COLOR_CHANNELPANEL_CURCHANNELNAMETEXT,       COLOR_CHANNELPANEL_CHANNELNAMETEXT},
		{COLOR_CHANNELPANEL_EVENTNAME2TEXT,           COLOR_CHANNELPANEL_EVENTNAME1TEXT},
		{COLOR_CHANNELPANEL_EVENTNAME2BORDER,         COLOR_CHANNELPANEL_EVENTNAME1BORDER},
		{COLOR_CHANNELPANEL_CUREVENTNAME1TEXT,        COLOR_CHANNELPANEL_EVENTNAME1TEXT},
		{COLOR_CHANNELPANEL_CUREVENTNAME1BORDER,      COLOR_CHANNELPANEL_EVENTNAME1BORDER},
		{COLOR_CHANNELPANEL_CUREVENTNAME2TEXT,        COLOR_CHANNELPANEL_CUREVENTNAME1TEXT},
		{COLOR_CHANNELPANEL_CUREVENTNAME2BORDER,      COLOR_CHANNELPANEL_CUREVENTNAME1BORDER},
		{COLOR_CHANNELPANEL_PROGRESSBORDER,           COLOR_CHANNELPANEL_PROGRESS1},
		{COLOR_CHANNELPANEL_CURPROGRESSBORDER,        COLOR_CHANNELPANEL_CURPROGRESS1},
		{COLOR_PROGRAMLISTPANEL_CHANNELTEXT,          COLOR_CHANNELPANEL_CHANNELNAMETEXT},
		{COLOR_PROGRAMLISTPANEL_CURCHANNELTEXT,       COLOR_CHANNELPANEL_CURCHANNELNAMETEXT},
		{COLOR_PROGRAMLISTPANEL_CHANNELBUTTONTEXT,    COLOR_PROGRAMLISTPANEL_CHANNELTEXT},
		{COLOR_PROGRAMLISTPANEL_CHANNELBUTTONHOTTEXT, COLOR_PROGRAMLISTPANEL_CURCHANNELTEXT},
		{COLOR_CONTROLPANELBACK1,                     COLOR_PANELBACK},
		{COLOR_CONTROLPANELBACK2,                     COLOR_PANELBACK},
		{COLOR_CONTROLPANELTEXT,                      COLOR_PANELTEXT},
		{COLOR_CONTROLPANELCHECKEDTEXT,               COLOR_CONTROLPANELHIGHLIGHTTEXT},
		{COLOR_CONTROLPANELMARGIN,                    COLOR_PANELBACK},
		{COLOR_CAPTIONPANELBACK,                      COLOR_PROGRAMINFOBACK},
		{COLOR_CAPTIONPANELTEXT,                      COLOR_PROGRAMINFOTEXT},
		{COLOR_TITLEBARTEXT,                          COLOR_STATUSTEXT},
		{COLOR_TITLEBARICON,                          COLOR_TITLEBARTEXT},
		{COLOR_TITLEBARHIGHLIGHTICON,                 COLOR_STATUSHIGHLIGHTTEXT},
//		{COLOR_TITLEBARBORDER,                        COLOR_TITLEBARBACK1},
		{COLOR_SIDEBARICON,                           COLOR_STATUSTEXT},
		{COLOR_SIDEBARHIGHLIGHTICON,                  COLOR_STATUSHIGHLIGHTTEXT},
		{COLOR_SIDEBARCHECKICON,                      COLOR_SIDEBARICON},
//		{COLOR_SIDEBARCHECKBORDER,                    COLOR_SIDEBARCHECKBACK2},
//		{COLOR_SIDEBARBORDER,                         COLOR_SIDEBARBACK1},
		{COLOR_EVENTINFOPOPUP_BACK,                   COLOR_PROGRAMINFOBACK},
		{COLOR_EVENTINFOPOPUP_TEXT,                   COLOR_PROGRAMINFOTEXT},
		{COLOR_EVENTINFOPOPUP_EVENTTITLE,             COLOR_EVENTINFOPOPUP_TEXT},
		{COLOR_PROGRAMGUIDE_CHANNELHIGHLIGHTTEXT,     COLOR_PROGRAMGUIDE_CHANNELTEXT},
		{COLOR_PROGRAMGUIDE_CURCHANNELTEXT,           COLOR_PROGRAMGUIDE_CHANNELTEXT},
		{COLOR_PROGRAMGUIDE_TIMELINE,                 COLOR_PROGRAMGUIDE_TIMETEXT},
		{COLOR_PROGRAMGUIDE_DATEBUTTON_BORDER,        COLOR_PROGRAMGUIDE_DATEBUTTON_BACK1},
		{COLOR_PROGRAMGUIDE_DATEBUTTON_CURTEXT,       COLOR_PROGRAMGUIDE_DATEBUTTON_TEXT},
		{COLOR_PROGRAMGUIDE_DATEBUTTON_CURBACK1,      COLOR_PROGRAMGUIDE_DATEBUTTON_BACK2},
		{COLOR_PROGRAMGUIDE_DATEBUTTON_CURBACK2,      COLOR_PROGRAMGUIDE_DATEBUTTON_BACK1},
		{COLOR_PROGRAMGUIDE_DATEBUTTON_CURBORDER,     COLOR_PROGRAMGUIDE_DATEBUTTON_CURBACK2},
		{COLOR_PROGRAMGUIDE_DATEBUTTON_HOTTEXT,       COLOR_PROGRAMGUIDE_DATEBUTTON_TEXT},
	};

	for (const auto Map : ColorMap) {
		if (!IsLoaded(Map.To) && IsLoaded(Map.From)) {
			m_ColorList[Map.To] = m_ColorList[Map.From];
			SetLoadedFlag(Map.To);
		}
	}

	static const struct {
		int To, From1, From2;
	} MixMap[] = {
		{COLOR_STATUSBORDER,       COLOR_STATUSBACK1,       COLOR_STATUSBACK2},
		{COLOR_PANELTITLEBORDER,   COLOR_PANELTITLEBACK1,   COLOR_PANELTITLEBACK2},
		{COLOR_TITLEBARBORDER,     COLOR_TITLEBARBACK1,     COLOR_TITLEBARBACK2},
		{COLOR_SIDEBARCHECKBORDER, COLOR_SIDEBARCHECKBACK1, COLOR_SIDEBARCHECKBACK2},
		{COLOR_SIDEBARBORDER,      COLOR_SIDEBARBACK1,      COLOR_SIDEBARBACK2},
	};

	for (const auto Map : MixMap) {
		if (!IsLoaded(Map.To) && IsLoaded(Map.From1) && IsLoaded(Map.From2)) {
			m_ColorList[Map.To] = MixColor(m_ColorList[Map.From1], m_ColorList[Map.From2]);
			SetLoadedFlag(Map.To);
		}
	}

	for (int i = 0; i < NUM_GRADIENTS; i++) {
		TCHAR szName[128];

		StringFormat(szName, TEXT("{}Style"), m_GradientInfoList[i].pszText);
		m_FillList[i].Type = Theme::FillType::Gradient;
		if (Settings.Read(szName, szText, lengthof(szText))) {
			for (int j = 0; j < lengthof(FillTypeNameList); j++) {
				if (::lstrcmpi(szText, FillTypeNameList[j]) == 0) {
					m_FillList[i].Type = static_cast<Theme::FillType>(j);
					break;
				}
			}
		}

		StringFormat(szName, TEXT("{}Gradient"), m_GradientInfoList[i].pszText);
		m_FillList[i].Gradient.Type = Theme::GradientType::Normal;
		if (Settings.Read(szName, szText, lengthof(szText))) {
			for (int j = 0; j < lengthof(GradientTypeNameList); j++) {
				if (::lstrcmpi(szText, GradientTypeNameList[j]) == 0) {
					m_FillList[i].Gradient.Type = static_cast<Theme::GradientType>(j);
					break;
				}
			}
		} else {
			switch (i) {
			case GRADIENT_TITLEBARICON:
				m_FillList[i].Gradient.Type = m_FillList[GRADIENT_TITLEBARBACK].Gradient.Type;
				break;
			case GRADIENT_SIDEBARCHECKBACK:
				m_FillList[i].Gradient.Type = m_FillList[GRADIENT_SIDEBARBACK].Gradient.Type;
				break;
			}
		}

		StringFormat(szName, TEXT("{}GradientDirection"), m_GradientInfoList[i].pszText);
		m_FillList[i].Gradient.Direction = m_GradientInfoList[i].Direction;
		if (Settings.Read(szName, szText, lengthof(szText))) {
			for (int j = 0; j < lengthof(GradientDirectionList); j++) {
				if (::lstrcmpi(szText, GradientDirectionList[j]) == 0) {
					m_FillList[i].Gradient.Direction = static_cast<Theme::GradientDirection>(j);
					break;
				}
			}
		} else {
			switch (i) {
			case GRADIENT_TITLEBARICON:
				m_FillList[i].Gradient.Direction = m_FillList[GRADIENT_TITLEBARBACK].Gradient.Direction;
				break;
			case GRADIENT_SIDEBARCHECKBACK:
				m_FillList[i].Gradient.Direction = m_FillList[GRADIENT_SIDEBARBACK].Gradient.Direction;
				break;
			}
		}
	}

	static const struct {
		int To, From;
	} GradientMap[] = {
		{GRADIENT_CHANNELPANEL_CURCHANNELNAMEBACK,       GRADIENT_CHANNELPANEL_CHANNELNAMEBACK},
		{GRADIENT_CHANNELPANEL_EVENTNAMEBACK2,           GRADIENT_CHANNELPANEL_EVENTNAMEBACK1},
		{GRADIENT_CHANNELPANEL_CUREVENTNAMEBACK1,        GRADIENT_CHANNELPANEL_EVENTNAMEBACK1},
		{GRADIENT_CHANNELPANEL_CUREVENTNAMEBACK2,        GRADIENT_CHANNELPANEL_CUREVENTNAMEBACK1},
		{GRADIENT_CHANNELPANEL_PROGRESS,                 GRADIENT_STATUSEVENTPROGRESSELAPSED},
		{GRADIENT_CHANNELPANEL_CURPROGRESS,              GRADIENT_CHANNELPANEL_PROGRESS},
		{GRADIENT_PROGRAMLISTPANEL_CHANNELBACK,          GRADIENT_CHANNELPANEL_CHANNELNAMEBACK},
		{GRADIENT_PROGRAMLISTPANEL_CURCHANNELBACK,       GRADIENT_CHANNELPANEL_CURCHANNELNAMEBACK},
		{GRADIENT_PROGRAMLISTPANEL_CHANNELBUTTONBACK,    GRADIENT_CHANNELPANEL_CHANNELNAMEBACK},
//		{GRADIENT_PROGRAMLISTPANEL_CHANNELBUTTONHOTBACK, GRADIENT_CHANNELPANEL_CURCHANNELNAMEBACK},
		{GRADIENT_PROGRAMLISTPANEL_CHANNELBUTTONHOTBACK, GRADIENT_CONTROLPANELHIGHLIGHTBACK},
		{GRADIENT_PROGRAMLISTPANEL_CUREVENTBACK,         GRADIENT_PROGRAMLISTPANEL_EVENTBACK},
		{GRADIENT_PROGRAMLISTPANEL_CURTITLEBACK,         GRADIENT_PROGRAMLISTPANEL_TITLEBACK},
		{GRADIENT_CONTROLPANELCHECKEDBACK,               GRADIENT_CONTROLPANELHIGHLIGHTBACK},
		{GRADIENT_TITLEBARBACK,                          GRADIENT_STATUSBACK},
		{GRADIENT_TITLEBARICON,                          GRADIENT_TITLEBARBACK},
		{GRADIENT_TITLEBARHIGHLIGHTBACK,                 GRADIENT_STATUSHIGHLIGHTBACK},
		{GRADIENT_SIDEBARBACK,                           GRADIENT_STATUSBACK},
		{GRADIENT_SIDEBARHIGHLIGHTBACK,                  GRADIENT_STATUSHIGHLIGHTBACK},
		{GRADIENT_SIDEBARCHECKBACK,                      GRADIENT_SIDEBARBACK},
		{GRADIENT_PROGRAMGUIDECURCHANNELBACK,            GRADIENT_PROGRAMGUIDECHANNELBACK},
		{GRADIENT_PROGRAMGUIDE_DATEBUTTON_HOTBACK,       GRADIENT_PROGRAMGUIDE_DATEBUTTON_BACK},
	};

	for (const auto Map : GradientMap) {
		if (m_GradientInfoList[Map.To].Color1 >= 0
				&& !IsLoaded(m_GradientInfoList[Map.To].Color1)
				&& IsLoaded(m_GradientInfoList[Map.From].Color1)) {
			m_ColorList[m_GradientInfoList[Map.To].Color1] = m_ColorList[m_GradientInfoList[Map.From].Color1];
			m_ColorList[m_GradientInfoList[Map.To].Color2] = m_ColorList[m_GradientInfoList[Map.From].Color2];
			SetLoadedFlag(m_GradientInfoList[Map.To].Color1);
			SetLoadedFlag(m_GradientInfoList[Map.To].Color2);
			m_FillList[Map.To] = m_FillList[Map.From];
		}
	}

	bool BorderLoaded[NUM_BORDERS];

	for (int i = 0; i < NUM_BORDERS; i++) {
		m_BorderList[i] = m_BorderInfoList[i].DefaultType;
		BorderLoaded[i] = false;
	}
	if (Settings.SetSection(TEXT("Style"))) {
		for (int i = 0; i < NUM_BORDERS; i++) {
			if (Settings.Read(m_BorderInfoList[i].pszText, szText, lengthof(szText))) {
				bool fLoaded = false;
				for (int j = 0; j < lengthof(BorderTypeNameList); j++) {
					if (::lstrcmpi(szText, BorderTypeNameList[j]) == 0) {
						m_BorderList[i] = static_cast<Theme::BorderType>(j);
						fLoaded = true;
						break;
					}
				}
				BorderLoaded[i] = fLoaded;
			}
		}
	}

	static const struct {
		int To, From;
	} BorderMap[] = {
		{BORDER_PROGRAMLISTPANEL_CHANNEL,          BORDER_CHANNELPANEL_CHANNELNAME},
		{BORDER_PROGRAMLISTPANEL_CURCHANNEL,       BORDER_CHANNELPANEL_CURCHANNELNAME},
		{BORDER_PROGRAMLISTPANEL_CHANNELBUTTONHOT, BORDER_CONTROLPANELHIGHLIGHTITEM},
		{BORDER_SIDEBARITEM,                       BORDER_STATUSITEM},
		{BORDER_SIDEBARHIGHLIGHT,                  BORDER_STATUSHIGHLIGHT},
		{BORDER_PROGRAMGUIDE_DATEBUTTON_HOT,       BORDER_PROGRAMGUIDE_DATEBUTTON},
	};

	for (const auto Map : BorderMap) {
		if (!BorderLoaded[Map.To] && BorderLoaded[Map.From]) {
			m_BorderList[Map.To] = m_BorderList[Map.From];
		}
		const int ColorTo = m_BorderInfoList[Map.To].Color;
		const int ColorFrom = m_BorderInfoList[Map.From].Color;
		if (!IsLoaded(ColorTo) && IsLoaded(ColorFrom)) {
			m_ColorList[ColorTo] = m_ColorList[ColorFrom];
			SetLoadedFlag(ColorTo);
		}
	}

	for (int i = 0; i < NUM_COLORS; i++) {
		if (!IsLoaded(i)
				&& m_ColorInfoList[i].DefaultColor != m_ColorInfoList[i].DefaultLightColor
				&& m_ColorInfoList[i].DefaultLightColor != CLR_INVALID) {
			m_ColorList[i] =
				m_BaseScheme == BaseSchemeType::Dark ?
					m_ColorInfoList[i].DefaultColor :
					m_ColorInfoList[i].DefaultLightColor;
			SetLoadedFlag(i);
		}
	}

	return true;
}


bool CColorScheme::Save(CSettings &Settings, SaveFlag Flags) const
{
	if (!Settings.SetSection(TEXT("ColorScheme")))
		return false;

	bool fSaveAllColors = true, fSaveGradients = true;

	if (!!(Flags & SaveFlag::NoDefault)) {
		/*
			SaveFlag::NoDefault が指定されている場合、デフォルトから変更されていない時のみ設定を保存する。
			Loadの処理が複雑なので、単純に変更されているもののみ保存すればいいという訳ではないため、
			デフォルトから変更されている場合は全ての設定を保存する。
		*/
		CColorScheme DefaultColorScheme;
		int i;

		for (i = 0; i < NUM_COLORS; i++) {
			if ((i < COLOR_PROGRAMGUIDE_CONTENT_FIRST || i > COLOR_PROGRAMGUIDE_CONTENT_LAST)
					&& m_ColorList[i] != DefaultColorScheme.m_ColorList[i])
				break;
		}
		if (i == NUM_COLORS)
			fSaveAllColors = false;

		for (i = 0; i < NUM_GRADIENTS; i++) {
			if (m_FillList[i] != DefaultColorScheme.m_FillList[i])
				break;
		}
		if (i == NUM_GRADIENTS)
			fSaveGradients = false;

		if (!fSaveAllColors || !fSaveGradients)
			Settings.Clear();
	}

	if (!(Flags & SaveFlag::NoName))
		Settings.Write(TEXT("Name"), m_Name);

	Settings.Write(TEXT("Base"), m_BaseScheme == BaseSchemeType::Dark ? TEXT("dark") : TEXT("light"));

	for (int i = 0; i < NUM_COLORS; i++) {
		if (fSaveAllColors || m_ColorList[i] != m_ColorInfoList[i].DefaultColor)
			Settings.WriteColor(m_ColorInfoList[i].pszText, m_ColorList[i]);
	}

	if (fSaveGradients) {
		for (int i = 0; i < NUM_GRADIENTS; i++) {
			TCHAR szName[128];

			StringFormat(szName, TEXT("{}Type"), m_GradientInfoList[i].pszText);
			Settings.Write(szName, FillTypeNameList[static_cast<int>(m_FillList[i].Type)]);
			StringFormat(szName, TEXT("{}Gradient"), m_GradientInfoList[i].pszText);
			Settings.Write(szName, GradientTypeNameList[static_cast<int>(m_FillList[i].Gradient.Type)]);
			StringFormat(szName, TEXT("{}GradientDirection"), m_GradientInfoList[i].pszText);
			Settings.Write(szName, GradientDirectionList[static_cast<int>(m_FillList[i].Gradient.Direction)]);
		}
	}

	if (Settings.SetSection(TEXT("Style"))) {
		for (int i = 0; i < NUM_BORDERS; i++)
			Settings.Write(m_BorderInfoList[i].pszText, BorderTypeNameList[static_cast<int>(m_BorderList[i])]);
	}

	return true;
}


bool CColorScheme::Load(LPCTSTR pszFileName)
{
	CSettings Settings;

	if (!Settings.Open(pszFileName, CSettings::OpenFlag::Read))
		return false;

	if (!Load(Settings))
		return false;

	SetFileName(pszFileName);

	if (m_Name.empty()) {
		TCHAR szName[MAX_COLORSCHEME_NAME];
		StringCopy(szName, ::PathFindFileName(pszFileName));
		::PathRemoveExtension(szName);
		SetName(szName);
	}

	return true;
}


bool CColorScheme::Save(LPCTSTR pszFileName, SaveFlag Flags) const
{
	CSettings Settings;

	if (!Settings.Open(pszFileName, CSettings::OpenFlag::Write))
		return false;

	return Save(Settings, Flags);
}


bool CColorScheme::SetFileName(LPCTSTR pszFileName)
{
	StringUtility::Assign(m_FileName, pszFileName);
	return true;
}


void CColorScheme::SetBaseScheme(BaseSchemeType BaseScheme)
{
	m_BaseScheme = BaseScheme;
}


void CColorScheme::SetDefault()
{
	m_BaseScheme = BaseSchemeType::Dark;

	for (int i = 0; i < NUM_COLORS; i++)
		m_ColorList[i] = m_ColorInfoList[i].DefaultColor;

	for (int i = 0; i < NUM_GRADIENTS; i++) {
		m_FillList[i].Type = Theme::FillType::Gradient;
		m_FillList[i].Gradient.Type = Theme::GradientType::Normal;
		m_FillList[i].Gradient.Direction = m_GradientInfoList[i].Direction;
	}

	for (int i = 0; i < NUM_BORDERS; i++)
		m_BorderList[i] = m_CustomDefaultBorderList[i];
}


LPCTSTR CColorScheme::GetColorName(int Type)
{
	if (Type < 0 || Type >= NUM_COLORS)
		return nullptr;
	return m_ColorInfoList[Type].pszName;
}


COLORREF CColorScheme::GetDefaultColor(BaseSchemeType BaseScheme, int Type)
{
	if (Type < 0 || Type >= NUM_COLORS)
		return CLR_INVALID;
	if (BaseScheme == BaseSchemeType::Light
			&& m_ColorInfoList[Type].DefaultLightColor != CLR_INVALID)
		return m_ColorInfoList[Type].DefaultLightColor;
	return m_ColorInfoList[Type].DefaultColor;
}


Theme::GradientType CColorScheme::GetDefaultGradientType(int Gradient)
{
	return Theme::GradientType::Normal;
}


bool CColorScheme::GetDefaultGradientStyle(int Gradient, GradientStyle *pStyle)
{
	if (Gradient < 0 || Gradient >= NUM_GRADIENTS)
		return false;
	pStyle->Type = Theme::GradientType::Normal;
	pStyle->Direction = m_GradientInfoList[Gradient].Direction;
	return true;
}


bool CColorScheme::IsGradientDirectionEnabled(int Gradient)
{
	if (Gradient < 0 || Gradient >= NUM_GRADIENTS)
		return false;
	return m_GradientInfoList[Gradient].fEnableDirection;
}


Theme::BorderType CColorScheme::GetDefaultBorderType(int Border)
{
	if (Border < 0 || Border >= NUM_BORDERS)
		return Theme::BorderType::None;
	return m_BorderInfoList[Border].DefaultType;
}


bool CColorScheme::IsLoaded(int Type) const
{
	if (Type < 0 || Type >= NUM_COLORS)
		return false;
	return m_LoadedFlags[Type];
}


void CColorScheme::SetLoaded()
{
	m_LoadedFlags.set();
}


bool CColorScheme::CompareScheme(const CColorScheme &Scheme) const
{
	for (int i = 0; i < NUM_COLORS; i++) {
		if (Scheme.IsLoaded(i) && m_ColorList[i] != Scheme.m_ColorList[i])
			return false;
	}

	for (int i = 0; i < NUM_GRADIENTS; i++) {
		if (m_FillList[i] != Scheme.m_FillList[i])
			return false;
	}

	for (int i = 0; i < NUM_BORDERS; i++) {
		if (m_BorderList[i] != Scheme.m_BorderList[i])
			return false;
	}

	return true;
}


int CColorScheme::GetColorGradient(int Type)
{
	for (int i = 0; i < NUM_GRADIENTS; i++) {
		if (m_GradientInfoList[i].Color1 == Type
				|| m_GradientInfoList[i].Color2 == Type)
			return i;
	}
	return -1;
}


int CColorScheme::GetColorBorder(int Type)
{
	for (int i = 0; i < NUM_BORDERS; i++) {
		if (m_BorderInfoList[i].Color == Type)
			return i;
	}
	return -1;
}


void CColorScheme::SetLoadedFlag(int Color)
{
	m_LoadedFlags.set(Color);
}




bool CColorSchemeList::Add(CColorScheme *pColorScheme)
{
	if (pColorScheme == nullptr)
		return false;
	m_List.emplace_back(pColorScheme);
	return true;
}


bool CColorSchemeList::Insert(int Index, CColorScheme *pColorScheme)
{
	if (Index < 0)
		return false;
	if (static_cast<size_t>(Index) >= m_List.size())
		return Add(pColorScheme);
	auto i = m_List.begin();
	std::advance(i, Index);
	m_List.emplace(i, pColorScheme);
	return true;
}


bool CColorSchemeList::Load(LPCTSTR pszDirectory)
{
	WIN32_FIND_DATA wfd;
	TCHAR szFileName[MAX_PATH];

	::PathCombine(szFileName, pszDirectory, TEXT("*.httheme"));
	const HANDLE hFind = ::FindFirstFileEx(szFileName, FindExInfoBasic, &wfd, FindExSearchNameMatch, nullptr, 0);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
				CColorScheme *pColorScheme;

				::PathCombine(szFileName, pszDirectory, wfd.cFileName);
				pColorScheme = new CColorScheme;
				if (pColorScheme->Load(szFileName))
					Add(pColorScheme);
				else
					delete pColorScheme;
			}
		} while (::FindNextFile(hFind, &wfd));
		::FindClose(hFind);
	}
	return true;
}


void CColorSchemeList::Clear()
{
	m_List.clear();
}


CColorScheme *CColorSchemeList::GetColorScheme(int Index)
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_List.size())
		return nullptr;
	return m_List[Index].get();
}


bool CColorSchemeList::SetColorScheme(int Index, const CColorScheme *pColorScheme)
{
	if (Index < 0 || static_cast<size_t>(Index) >= m_List.size() || pColorScheme == nullptr)
		return false;
	*m_List[Index] = *pColorScheme;
	return true;
}


int CColorSchemeList::FindByName(LPCTSTR pszName, int FirstIndex) const
{
	if (pszName == nullptr)
		return -1;

	for (int i = std::max(FirstIndex, 0); i < static_cast<int>(m_List.size()); i++) {
		if (!IsStringEmpty(m_List[i]->GetName())
				&& ::lstrcmpi(m_List[i]->GetName(), pszName) == 0)
			return i;
	}
	return -1;
}


void CColorSchemeList::SortByName()
{
	if (m_List.size() > 1) {
		std::ranges::sort(
			m_List,
			[](const std::unique_ptr<CColorScheme> &ColorScheme1,
			   const std::unique_ptr<CColorScheme> &ColorScheme2) -> bool {
				return ::lstrcmpi(ColorScheme1->GetName(), ColorScheme2->GetName()) < 0;
			});
	}
}


}	// namespace TVTest
