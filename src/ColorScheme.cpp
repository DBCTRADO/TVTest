/*
  TVTest
  Copyright(c) 2008-2019 DBCTRADO

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


constexpr COLORREF HEXRGB(DWORD hex) { return RGB(hex >> 16, (hex >> 8) & 0xFF, hex & 0xFF); }

}


const CColorScheme::ColorInfo CColorScheme::m_ColorInfoList[NUM_COLORS] = {
	{HEXRGB(0x333333),  TEXT("StatusBack"),                        TEXT("ステータスバー 背景1")},
	{HEXRGB(0x111111),  TEXT("StatusBack2"),                       TEXT("ステータスバー 背景2")},
	{HEXRGB(0x999999),  TEXT("StatusText"),                        TEXT("ステータスバー 文字")},
	{HEXRGB(0x777777),  TEXT("StatusItemBorder"),                  TEXT("ステータスバー 項目外枠")},
	{HEXRGB(0x111111),  TEXT("StatusBottomItemBack"),              TEXT("ステータスバー 下段背景1")},
	{HEXRGB(0x111111),  TEXT("StatusBottomItemBack2"),             TEXT("ステータスバー 下段背景2")},
	{HEXRGB(0x999999),  TEXT("StatusBottomItemText"),              TEXT("ステータスバー 下段文字")},
	{HEXRGB(0x111111),  TEXT("StatusBottomItemBorder"),            TEXT("ステータスバー 下段外枠")},
	{HEXRGB(0x4486E8),  TEXT("StatusHighlightBack"),               TEXT("ステータスバー 選択背景1")},
	{HEXRGB(0x3C76CC),  TEXT("StatusHighlightBack2"),              TEXT("ステータスバー 選択背景2")},
	{HEXRGB(0xDDDDDD),  TEXT("StatusHighlightText"),               TEXT("ステータスバー 選択文字")},
	{HEXRGB(0x3C76CC),  TEXT("StatusHighlightBorder"),             TEXT("ステータスバー 選択外枠")},
	{HEXRGB(0x111111),  TEXT("StatusBorder"),                      TEXT("ステータスバー 外枠")},
	{HEXRGB(0xDF3F00),  TEXT("StatusRecordingCircle"),             TEXT("ステータスバー 録画●")},
	{HEXRGB(0x444444),  TEXT("StatusEventProgressBack"),           TEXT("ステータスバー 番組経過時間背景1")},
	{HEXRGB(0x444444),  TEXT("StatusEventProgressBack2"),          TEXT("ステータスバー 番組経過時間背景2")},
	{HEXRGB(0x444444),  TEXT("StatusEventProgressBorder"),         TEXT("ステータスバー 番組経過時間外枠")},
	{HEXRGB(0x3465B0),  TEXT("StatusEventProgressElapsed"),        TEXT("ステータスバー 番組経過時間バー1")},
	{HEXRGB(0x3465B0),  TEXT("StatusEventProgressElapsed2"),       TEXT("ステータスバー 番組経過時間バー2")},
	{HEXRGB(0x3465B0),  TEXT("StatusEventProgressElapsedBorder"),  TEXT("ステータスバー 番組経過時間バー外枠")},
	{HEXRGB(0x222222),  TEXT("Splitter"),                          TEXT("分割線")},
	{HEXRGB(0x000000),  TEXT("ScreenBorder"),                      TEXT("画面の外枠")},
	{HEXRGB(0x555555),  TEXT("WindowFrame"),                       TEXT("ウィンドウ 細枠")},
	{HEXRGB(0x555555),  TEXT("WindowFrameBorder"),                 TEXT("ウィンドウ 細枠の境界")},
	{HEXRGB(0x666666),  TEXT("WindowActiveFrame"),                 TEXT("ウィンドウ アクティブ細枠")},
	{HEXRGB(0x666666),  TEXT("WindowActiveFrameBorder"),           TEXT("ウィンドウ アクティブ細枠の境界")},
	{HEXRGB(0x333333),  TEXT("PanelBack"),                         TEXT("パネル 背景")},
	{HEXRGB(0x999999),  TEXT("PanelText"),                         TEXT("パネル 文字")},
	{HEXRGB(0x000000),  TEXT("PanelTabBack"),                      TEXT("パネル タブ背景1")},
	{HEXRGB(0x222222),  TEXT("PanelTabBack2"),                     TEXT("パネル タブ背景2")},
	{HEXRGB(0x888888),  TEXT("PanelTabText"),                      TEXT("パネル タブ文字")},
	{HEXRGB(0x000000),  TEXT("PanelTabBorder"),                    TEXT("パネル タブ外枠")},
	{HEXRGB(0x555555),  TEXT("PanelCurTabBack"),                   TEXT("パネル 選択タブ背景1")},
	{HEXRGB(0x333333),  TEXT("PanelCurTabBack2"),                  TEXT("パネル 選択タブ背景2")},
	{HEXRGB(0xAAAAAA),  TEXT("PanelCurTabText"),                   TEXT("パネル 選択タブ文字")},
	{HEXRGB(0x444444),  TEXT("PanelCurTabBorder"),                 TEXT("パネル 選択タブ外枠")},
	{HEXRGB(0x000000),  TEXT("PanelTabMargin"),                    TEXT("パネル タブ余白1")},
	{HEXRGB(0x222222),  TEXT("PanelTabMargin2"),                   TEXT("パネル タブ余白2")},
	{HEXRGB(0x888888),  TEXT("PanelTabMarginBorder"),              TEXT("パネル タブ余白外枠")},
	{HEXRGB(0x444444),  TEXT("PanelTabLine"),                      TEXT("パネル タブ線")},
	{HEXRGB(0x333333),  TEXT("PanelTitleBack"),                    TEXT("パネル タイトル背景1")},
	{HEXRGB(0x111111),  TEXT("PanelTitleBack2"),                   TEXT("パネル タイトル背景2")},
	{HEXRGB(0xAAAAAA),  TEXT("PanelTitleText"),                    TEXT("パネル タイトル文字")},
	{HEXRGB(0x111111),  TEXT("PanelTitleBorder"),                  TEXT("パネル タイトル外枠")},
	{HEXRGB(0x111111),  TEXT("ProgramInfoBack"),                   TEXT("情報パネル 番組情報背景")},
	{HEXRGB(0xAAAAAA),  TEXT("ProgramInfoText"),                   TEXT("情報パネル 番組情報文字")},
	{HEXRGB(0x111111),  TEXT("ProgramInfoBorder"),                 TEXT("情報パネル 番組情報外枠")},
	{HEXRGB(0x333333),  TEXT("InformationPanelButtonBack"),        TEXT("情報パネル ボタン背景1")},
	{HEXRGB(0x333333),  TEXT("InformationPanelButtonBack2"),       TEXT("情報パネル ボタン背景2")},
	{HEXRGB(0x999999),  TEXT("InformationPanelButtonText"),        TEXT("情報パネル ボタン文字")},
	{HEXRGB(0x333333),  TEXT("InformationPanelButtonBorder"),      TEXT("情報パネル ボタン境界")},
	{HEXRGB(0x4486E8),  TEXT("InformationPanelHotButtonBack"),     TEXT("情報パネル 選択ボタン背景1")},
	{HEXRGB(0x3C76CC),  TEXT("InformationPanelHotButtonBack2"),    TEXT("情報パネル 選択ボタン背景2")},
	{HEXRGB(0xDDDDDD),  TEXT("InformationPanelHotButtonText"),     TEXT("情報パネル 選択ボタン文字")},
	{HEXRGB(0x3C76CC),  TEXT("InformationPanelHotButtonBorder"),   TEXT("情報パネル 選択ボタン境界")},
	{HEXRGB(0x333333),  TEXT("ProgramListChannelBack"),            TEXT("番組表パネル 局名背景1")},
	{HEXRGB(0x000000),  TEXT("ProgramListChannelBack2"),           TEXT("番組表パネル 局名背景2")},
	{HEXRGB(0xAAAAAA),  TEXT("ProgramListChannelText"),            TEXT("番組表パネル 局名文字")},
	{HEXRGB(0x000000),  TEXT("ProgramListChannelBorder"),          TEXT("番組表パネル 局名外枠")},
	{HEXRGB(0x4486E8),  TEXT("ProgramListCurChannelBack"),         TEXT("番組表パネル 現在局名背景1")},
	{HEXRGB(0x3C76CC),  TEXT("ProgramListCurChannelBack2"),        TEXT("番組表パネル 現在局名背景2")},
	{HEXRGB(0xDDDDDD),  TEXT("ProgramListCurChannelText"),         TEXT("番組表パネル 現在局名文字")},
	{HEXRGB(0x000000),  TEXT("ProgramListCurChannelBorder"),       TEXT("番組表パネル 現在局名外枠")},
	{HEXRGB(0x333333),  TEXT("ProgramListChannelButtonBack"),      TEXT("番組表パネル ボタン背景1")},
	{HEXRGB(0x000000),  TEXT("ProgramListChannelButtonBack2"),     TEXT("番組表パネル ボタン背景2")},
	{HEXRGB(0xAAAAAA),  TEXT("ProgramListChannelButtonText"),      TEXT("番組表パネル ボタン文字")},
	{HEXRGB(0x000000),  TEXT("ProgramListChannelButtonBorder"),    TEXT("番組表パネル ボタン外枠")},
	{HEXRGB(0x4486E8),  TEXT("ProgramListChannelButtonHotBack"),   TEXT("番組表パネル 選択ボタン背景1")},
	{HEXRGB(0x3C76CC),  TEXT("ProgramListChannelButtonHotBack2"),  TEXT("番組表パネル 選択ボタン背景2")},
	{HEXRGB(0xDDDDDD),  TEXT("ProgramListChannelButtonHotText"),   TEXT("番組表パネル 選択ボタン文字")},
	{HEXRGB(0x3C76CC),  TEXT("ProgramListChannelButtonHotBorder"), TEXT("番組表パネル 選択ボタン外枠")},
	{HEXRGB(0x333333),  TEXT("ProgramListBack"),                   TEXT("番組表パネル 番組内容背景1")},
	{HEXRGB(0x333333),  TEXT("ProgramListBack2"),                  TEXT("番組表パネル 番組内容背景2")},
	{HEXRGB(0x999999),  TEXT("ProgramListText"),                   TEXT("番組表パネル 番組内容文字")},
	{HEXRGB(0x444444),  TEXT("ProgramListBorder"),                 TEXT("番組表パネル 番組内容外枠")},
	{HEXRGB(0x222222),  TEXT("ProgramListCurBack"),                TEXT("番組表パネル 現在番組内容背景1")},
	{HEXRGB(0x333333),  TEXT("ProgramListCurBack2"),               TEXT("番組表パネル 現在番組内容背景2")},
	{HEXRGB(0xAAAAAA),  TEXT("ProgramListCurText"),                TEXT("番組表パネル 現在番組内容文字")},
	{HEXRGB(0x555555),  TEXT("ProgramListCurBorder"),              TEXT("番組表パネル 現在番組内容外枠")},
	{HEXRGB(0x333333),  TEXT("ProgramListTitleBack"),              TEXT("番組表パネル 番組名背景1")},
	{HEXRGB(0x000000),  TEXT("ProgramListTitleBack2"),             TEXT("番組表パネル 番組名背景2")},
	{HEXRGB(0xAAAAAA),  TEXT("ProgramListTitleText"),              TEXT("番組表パネル 番組名文字")},
	{HEXRGB(0x000000),  TEXT("ProgramListTitleBorder"),            TEXT("番組表パネル 番組名外枠")},
	{HEXRGB(0x4486E8),  TEXT("ProgramListCurTitleBack"),           TEXT("番組表パネル 現在番組名背景1")},
	{HEXRGB(0x3C76CC),  TEXT("ProgramListCurTitleBack2"),          TEXT("番組表パネル 現在番組名背景2")},
	{HEXRGB(0xDDDDDD),  TEXT("ProgramListCurTitleText"),           TEXT("番組表パネル 現在番組名文字")},
	{HEXRGB(0x000000),  TEXT("ProgramListCurTitleBorder"),         TEXT("番組表パネル 現在番組名外枠")},
	{HEXRGB(0x333333),  TEXT("ChannelPanelChannelNameBack"),       TEXT("チャンネルパネル 局名背景1")},
	{HEXRGB(0x000000),  TEXT("ChannelPanelChannelNameBack2"),      TEXT("チャンネルパネル 局名背景2")},
	{HEXRGB(0xAAAAAA),  TEXT("ChannelPanelChannelNameText"),       TEXT("チャンネルパネル 局名文字")},
	{HEXRGB(0x000000),  TEXT("ChannelPanelChannelNameBorder"),     TEXT("チャンネルパネル 局名外枠")},
	{HEXRGB(0x4486E8),  TEXT("ChannelPanelCurChannelNameBack"),    TEXT("チャンネルパネル 現在局名背景1")},
	{HEXRGB(0x3C76CC),  TEXT("ChannelPanelCurChannelNameBack2"),   TEXT("チャンネルパネル 現在局名背景2")},
	{HEXRGB(0xDDDDDD),  TEXT("ChannelPanelCurChannelNameText"),    TEXT("チャンネルパネル 現在局名文字")},
	{HEXRGB(0x000000),  TEXT("ChannelPanelCurChannelNameBorder"),  TEXT("チャンネルパネル 現在局名外枠")},
	{HEXRGB(0x444444),  TEXT("ChannelPanelEventNameBack"),         TEXT("チャンネルパネル 番組名1背景1")},
	{HEXRGB(0x333333),  TEXT("ChannelPanelEventNameBack2"),        TEXT("チャンネルパネル 番組名1背景2")},
	{HEXRGB(0x999999),  TEXT("ChannelPanelEventNameText"),         TEXT("チャンネルパネル 番組名1文字")},
	{HEXRGB(0x444444),  TEXT("ChannelPanelEventNameBorder"),       TEXT("チャンネルパネル 番組名1外枠")},
	{HEXRGB(0x222222),  TEXT("ChannelPanelEventName2Back"),        TEXT("チャンネルパネル 番組名2背景1")},
	{HEXRGB(0x222222),  TEXT("ChannelPanelEventName2Back2"),       TEXT("チャンネルパネル 番組名2背景2")},
	{HEXRGB(0x999999),  TEXT("ChannelPanelEventName2Text"),        TEXT("チャンネルパネル 番組名2文字")},
	{HEXRGB(0x333333),  TEXT("ChannelPanelEventName2Border"),      TEXT("チャンネルパネル 番組名2外枠")},
	{HEXRGB(0x444444),  TEXT("ChannelPanelCurEventNameBack"),      TEXT("チャンネルパネル 選択番組名1背景1")},
	{HEXRGB(0x333333),  TEXT("ChannelPanelCurEventNameBack2"),     TEXT("チャンネルパネル 選択番組名1背景2")},
	{HEXRGB(0xAAAAAA),  TEXT("ChannelPanelCurEventNameText"),      TEXT("チャンネルパネル 選択番組名1文字")},
	{HEXRGB(0x444444),  TEXT("ChannelPanelCurEventNameBorder"),    TEXT("チャンネルパネル 選択番組名1外枠")},
	{HEXRGB(0x222222),  TEXT("ChannelPanelCurEventName2Back"),     TEXT("チャンネルパネル 選択番組名2背景1")},
	{HEXRGB(0x222222),  TEXT("ChannelPanelCurEventName2Back2"),    TEXT("チャンネルパネル 選択番組名2背景2")},
	{HEXRGB(0xAAAAAA),  TEXT("ChannelPanelCurEventName2Text"),     TEXT("チャンネルパネル 選択番組名2文字")},
	{HEXRGB(0x333333),  TEXT("ChannelPanelCurEventName2Border"),   TEXT("チャンネルパネル 選択番組名2外枠")},
	{HEXRGB(0x00FF00),  TEXT("ChannelPanelFeaturedMark"),          TEXT("チャンネルパネル 注目マーク背景1")},
	{HEXRGB(0x00FF00),  TEXT("ChannelPanelFeaturedMark2"),         TEXT("チャンネルパネル 注目マーク背景2")},
	{HEXRGB(0x00BF00),  TEXT("ChannelPanelFeaturedMarkBorder"),    TEXT("チャンネルパネル 注目マーク外枠")},
	{HEXRGB(0x2D5899),  TEXT("ChannelPanelProgress"),              TEXT("チャンネルパネル 番組経過時間バー1")},
	{HEXRGB(0x2D5899),  TEXT("ChannelPanelProgress2"),             TEXT("チャンネルパネル 番組経過時間バー2")},
	{HEXRGB(0x2D5899),  TEXT("ChannelPanelProgressBorder"),        TEXT("チャンネルパネル 番組経過時間バー外枠")},
	{HEXRGB(0x3465B0),  TEXT("ChannelPanelCurProgress"),           TEXT("チャンネルパネル 選択番組経過時間バー1")},
	{HEXRGB(0x3465B0),  TEXT("ChannelPanelCurProgress2"),          TEXT("チャンネルパネル 選択番組経過時間バー2")},
	{HEXRGB(0x3465B0),  TEXT("ChannelPanelCurProgressBorder"),     TEXT("チャンネルパネル 選択番組経過時間バー外枠")},
	{HEXRGB(0x333333),  TEXT("ControlPanelBack"),                  TEXT("操作パネル 背景1")},
	{HEXRGB(0x333333),  TEXT("ControlPanelBack2"),                 TEXT("操作パネル 背景2")},
	{HEXRGB(0x999999),  TEXT("ControlPanelText"),                  TEXT("操作パネル 文字")},
	{HEXRGB(0x666666),  TEXT("ControlPanelItemBorder"),            TEXT("操作パネル 項目外枠")},
	{HEXRGB(0x4486E8),  TEXT("ControlPanelHighlightBack"),         TEXT("操作パネル 選択背景1")},
	{HEXRGB(0x3C76CC),  TEXT("ControlPanelHighlightBack2"),        TEXT("操作パネル 選択背景2")},
	{HEXRGB(0xDDDDDD),  TEXT("ControlPanelHighlightText"),         TEXT("操作パネル 選択文字")},
	{HEXRGB(0x3C76CC),  TEXT("ControlPanelHighlightBorder"),       TEXT("操作パネル 選択項目外枠")},
	{HEXRGB(0x444444),  TEXT("ControlPanelCheckedBack"),           TEXT("操作パネル チェック背景1")},
	{HEXRGB(0x555555),  TEXT("ControlPanelCheckedBack2"),          TEXT("操作パネル チェック背景2")},
	{HEXRGB(0xDDDDDD),  TEXT("ControlPanelCheckedText"),           TEXT("操作パネル チェック文字")},
	{HEXRGB(0x333333),  TEXT("ControlPanelCheckedBorder"),         TEXT("操作パネル チェック項目外枠")},
	{HEXRGB(0x333333),  TEXT("ControlPanelMargin"),                TEXT("操作パネル 余白")},
	{HEXRGB(0x333333),  TEXT("CaptionPanelBack"),                  TEXT("字幕パネル 背景")},
	{HEXRGB(0x999999),  TEXT("CaptionPanelText"),                  TEXT("字幕パネル 文字")},
	{HEXRGB(0x333333),  TEXT("TitleBarBack"),                      TEXT("タイトルバー 背景1")},
	{HEXRGB(0x111111),  TEXT("TitleBarBack2"),                     TEXT("タイトルバー 背景2")},
	{HEXRGB(0xAAAAAA),  TEXT("TitleBarText"),                      TEXT("タイトルバー 文字")},
	{HEXRGB(0x777777),  TEXT("TitleBarTextBorder"),                TEXT("タイトルバー 文字外枠")},
	{HEXRGB(0x333333),  TEXT("TitleBarIconBack"),                  TEXT("タイトルバー アイコン背景1")},
	{HEXRGB(0x111111),  TEXT("TitleBarIconBack2"),                 TEXT("タイトルバー アイコン背景2")},
	{HEXRGB(0x999999),  TEXT("TitleBarIcon"),                      TEXT("タイトルバー アイコン")},
	{HEXRGB(0x777777),  TEXT("TitleBarIconBorder"),                TEXT("タイトルバー アイコン外枠")},
	{HEXRGB(0x4486E8),  TEXT("TitleBarHighlightBack"),             TEXT("タイトルバー 選択背景1")},
	{HEXRGB(0x3C76CC),  TEXT("TitleBarHighlightBack2"),            TEXT("タイトルバー 選択背景2")},
	{HEXRGB(0xDDDDDD),  TEXT("TitleBarHighlightIcon"),             TEXT("タイトルバー 選択アイコン")},
	{HEXRGB(0x3C76CC),  TEXT("TitleBarHighlightIconBorder"),       TEXT("タイトルバー 選択アイコン外枠")},
	{HEXRGB(0x111111),  TEXT("TitleBarBorder"),                    TEXT("タイトルバー 外枠")},
	{HEXRGB(0x333333),  TEXT("SideBarBack"),                       TEXT("サイドバー 背景1")},
	{HEXRGB(0x111111),  TEXT("SideBarBack2"),                      TEXT("サイドバー 背景2")},
	{HEXRGB(0xAAAAAA),  TEXT("SideBarIcon"),                       TEXT("サイドバー アイコン")},
	{HEXRGB(0x777777),  TEXT("SideBarItemBorder"),                 TEXT("サイドバー 項目外枠")},
	{HEXRGB(0x4486E8),  TEXT("SideBarHighlightBack"),              TEXT("サイドバー 選択背景1")},
	{HEXRGB(0x3C76CC),  TEXT("SideBarHighlightBack2"),             TEXT("サイドバー 選択背景2")},
	{HEXRGB(0xDDDDDD),  TEXT("SideBarHighlightIcon"),              TEXT("サイドバー 選択アイコン")},
	{HEXRGB(0x3C76CC),  TEXT("SideBarHighlightBorder"),            TEXT("サイドバー 選択外枠")},
	{HEXRGB(0x333333),  TEXT("SideBarCheckBack"),                  TEXT("サイドバー チェック背景1")},
	{HEXRGB(0x444444),  TEXT("SideBarCheckBack2"),                 TEXT("サイドバー チェック背景2")},
	{HEXRGB(0xAAAAAA),  TEXT("SideBarCheckIcon"),                  TEXT("サイドバー チェックアイコン")},
	{HEXRGB(0x222222),  TEXT("SideBarCheckBorder"),                TEXT("サイドバー チェック外枠")},
	{HEXRGB(0x111111),  TEXT("SideBarBorder"),                     TEXT("サイドバー 外枠")},
	{HEXRGB(0x222222),  TEXT("NotificationBarBack"),               TEXT("通知バー 背景1")},
	{HEXRGB(0x333333),  TEXT("NotificationBarBack2"),              TEXT("通知バー 背景2")},
	{HEXRGB(0xBBBBBB),  TEXT("NotificationBarText"),               TEXT("通知バー 文字")},
	{HEXRGB(0xFF9F44),  TEXT("NotificationBarWarningText"),        TEXT("通知バー 警告文字")},
	{HEXRGB(0xFF4444),  TEXT("NotificationBarErrorText"),          TEXT("通知バー エラー文字")},
	{HEXRGB(0x333333),  TEXT("ProgramGuideBack"),                  TEXT("EPG番組表 背景")},
	{HEXRGB(0x222222),  TEXT("ProgramGuideText"),                  TEXT("EPG番組表 番組内容")},
	{HEXRGB(0x000000),  TEXT("ProgramGuideEventTitle"),            TEXT("EPG番組表 番組名")},
	{HEXRGB(0x0000BF),  TEXT("ProgramGuideHighlightText"),         TEXT("EPG番組表 検索番組内容")},
	{HEXRGB(0x0000FF),  TEXT("ProgramGuideHighlightTitle"),        TEXT("EPG番組表 検索番組名")},
	{HEXRGB(0x9999FF),  TEXT("ProgramGuideHighlightBack"),         TEXT("EPG番組表 検索番組背景")},
	{HEXRGB(0x6666FF),  TEXT("ProgramGuideHighlightBorder"),       TEXT("EPG番組表 検索番組枠")},
	{HEXRGB(0xCCFFCC),  TEXT("ProgramGuideFeaturedMark"),          TEXT("EPG番組表 注目マーク背景1")},
	{HEXRGB(0x99FF99),  TEXT("ProgramGuideFeaturedMark2"),         TEXT("EPG番組表 注目マーク背景2")},
	{HEXRGB(0x00EF00),  TEXT("ProgramGuideFeaturedMarkBorder"),    TEXT("EPG番組表 注目マーク外枠")},
	{HEXRGB(0x333333),  TEXT("ProgramGuideChannelBack"),           TEXT("EPG番組表 チャンネル名背景1")},
	{HEXRGB(0x111111),  TEXT("ProgramGuideChannelBack2"),          TEXT("EPG番組表 チャンネル名背景2")},
	{HEXRGB(0x999999),  TEXT("ProgramGuideChannelText"),           TEXT("EPG番組表 チャンネル名文字")},
	{HEXRGB(0x4486E8),  TEXT("ProgramGuideCurChannelBack"),        TEXT("EPG番組表 チャンネル名選択背景1")},
	{HEXRGB(0x3C76CC),  TEXT("ProgramGuideCurChannelBack2"),       TEXT("EPG番組表 チャンネル名選択背景2")},
	{HEXRGB(0xDDDDDD),  TEXT("ProgramGuideCurChannelText"),        TEXT("EPG番組表 チャンネル名選択文字")},
	{HEXRGB(0x333333),  TEXT("ProgramGuideTimeBack"),              TEXT("EPG番組表 日時背景1")},
	{HEXRGB(0x111111),  TEXT("ProgramGuideTimeBack2"),             TEXT("EPG番組表 日時背景2")},
	{HEXRGB(0x00337F),  TEXT("ProgramGuideTime0To2Back"),          TEXT("EPG番組表 0～2時背景1")},
	{HEXRGB(0x00193F),  TEXT("ProgramGuideTime0To2Back2"),         TEXT("EPG番組表 0～2時背景2")},
	{HEXRGB(0x00667F),  TEXT("ProgramGuideTime3To5Back"),          TEXT("EPG番組表 3～5時背景1")},
	{HEXRGB(0x00333F),  TEXT("ProgramGuideTime3To5Back2"),         TEXT("EPG番組表 3～5時背景2")},
	{HEXRGB(0x007F66),  TEXT("ProgramGuideTime6To8Back"),          TEXT("EPG番組表 6～8時背景1")},
	{HEXRGB(0x003F33),  TEXT("ProgramGuideTime6To8Back2"),         TEXT("EPG番組表 6～8時背景2")},
	{HEXRGB(0x667F00),  TEXT("ProgramGuideTime9To11Back"),         TEXT("EPG番組表 9～11時背景1")},
	{HEXRGB(0x333F00),  TEXT("ProgramGuideTime9To11Back2"),        TEXT("EPG番組表 9～11時背景2")},
	{HEXRGB(0x7F6600),  TEXT("ProgramGuideTime12To14Back"),        TEXT("EPG番組表 12～14時背景1")},
	{HEXRGB(0x3F3300),  TEXT("ProgramGuideTime12To14Back2"),       TEXT("EPG番組表 12～14時背景2")},
	{HEXRGB(0x7F3300),  TEXT("ProgramGuideTime15To17Back"),        TEXT("EPG番組表 15～17時背景1")},
	{HEXRGB(0x3F1900),  TEXT("ProgramGuideTime15To17Back2"),       TEXT("EPG番組表 15～17間背景2")},
	{HEXRGB(0x7F0066),  TEXT("ProgramGuideTime18To20Back"),        TEXT("EPG番組表 18～20時背景1")},
	{HEXRGB(0x3F0033),  TEXT("ProgramGuideTime18To20Back2"),       TEXT("EPG番組表 18～20時背景2")},
	{HEXRGB(0x66007F),  TEXT("ProgramGuideTime21To23Back"),        TEXT("EPG番組表 21～23時背景1")},
	{HEXRGB(0x33003F),  TEXT("ProgramGuideTime21To23Back2"),       TEXT("EPG番組表 21～23時背景2")},
	{HEXRGB(0xBBBBBB),  TEXT("ProgramGuideTimeText"),              TEXT("EPG番組表 時間文字")},
	{HEXRGB(0x888888),  TEXT("ProgramGuideTimeLine"),              TEXT("EPG番組表 時間線")},
	{HEXRGB(0xFF6600),  TEXT("ProgramGuideCurTimeLine"),           TEXT("EPG番組表 現在時刻線")},
	{HEXRGB(0xFFFFE0),  TEXT("EPGContentNews"),                    TEXT("EPG番組表 ニュース番組")},
	{HEXRGB(0xE0E0FF),  TEXT("EPGContentSports"),                  TEXT("EPG番組表 スポーツ番組")},
	{HEXRGB(0xFFE0F0),  TEXT("EPGContentInformation"),             TEXT("EPG番組表 情報番組")},
	{HEXRGB(0xFFE0E0),  TEXT("EPGContentDrama"),                   TEXT("EPG番組表 ドラマ")},
	{HEXRGB(0xE0FFE0),  TEXT("EPGContentMusic"),                   TEXT("EPG番組表 音楽番組")},
	{HEXRGB(0xE0FFFF),  TEXT("EPGContentVariety"),                 TEXT("EPG番組表 バラエティ番組")},
	{HEXRGB(0xFFF0E0),  TEXT("EPGContentMovie"),                   TEXT("EPG番組表 映画")},
	{HEXRGB(0xFFE0FF),  TEXT("EPGContentAnime"),                   TEXT("EPG番組表 アニメ/特撮")},
	{HEXRGB(0xFFFFE0),  TEXT("EPGContentDocumentary"),             TEXT("EPG番組表 ドキュメンタリー/教養番組")},
	{HEXRGB(0xFFF0E0),  TEXT("EPGContentTheater"),                 TEXT("EPG番組表 劇場/公演")},
	{HEXRGB(0xE0F0FF),  TEXT("EPGContentEducation"),               TEXT("EPG番組表 趣味/教育番組")},
	{HEXRGB(0xE0F0FF),  TEXT("EPGContentWelfare"),                 TEXT("EPG番組表 福祉番組")},
	{HEXRGB(0xF0F0F0),  TEXT("EPGContentOther"),                   TEXT("EPG番組表 その他の番組")},
};

const CColorScheme::GradientInfo CColorScheme::m_GradientInfoList[NUM_GRADIENTS] = {
	{TEXT("StatusBackGradient"),                      Theme::GradientDirection::Vert, false, COLOR_STATUSBACK1,                            COLOR_STATUSBACK2},
	{TEXT("StatusBottomItemBackGradient"),            Theme::GradientDirection::Vert, false, COLOR_STATUSBOTTOMITEMBACK1,                  COLOR_STATUSBOTTOMITEMBACK2},
	{TEXT("StatusHighlightBackGradient"),             Theme::GradientDirection::Vert, true,  COLOR_STATUSHIGHLIGHTBACK1,                   COLOR_STATUSHIGHLIGHTBACK2},
	{TEXT("StatusEventProgressBackGradient"),         Theme::GradientDirection::Vert, true,  COLOR_STATUSEVENTPROGRESSBACK1,               COLOR_STATUSEVENTPROGRESSBACK2},
	{TEXT("StatusEventProgressElapsedGradient"),      Theme::GradientDirection::Vert, true,  COLOR_STATUSEVENTPROGRESSELAPSED1,            COLOR_STATUSEVENTPROGRESSELAPSED2},
	{TEXT("PanelTabBackGradient"),                    Theme::GradientDirection::Vert, true,  COLOR_PANELTABBACK1,                          COLOR_PANELTABBACK2},
	{TEXT("PanelCurTabBackGradient"),                 Theme::GradientDirection::Vert, true,  COLOR_PANELCURTABBACK1,                       COLOR_PANELCURTABBACK2},
	{TEXT("PanelTabMarginGradient"),                  Theme::GradientDirection::Vert, false, COLOR_PANELTABMARGIN1,                        COLOR_PANELTABMARGIN2},
	{TEXT("PanelTitleBackGradient"),                  Theme::GradientDirection::Vert, true,  COLOR_PANELTITLEBACK1,                        COLOR_PANELTITLEBACK2},
	{TEXT("InformationPanelButtonBackGradient"),      Theme::GradientDirection::Vert, true,  COLOR_INFORMATIONPANEL_BUTTONBACK1,           COLOR_INFORMATIONPANEL_BUTTONBACK2},
	{TEXT("InformationPanelHotButtonBackGradient"),   Theme::GradientDirection::Vert, true,  COLOR_INFORMATIONPANEL_HOTBUTTONBACK1,        COLOR_INFORMATIONPANEL_HOTBUTTONBACK2},
	{TEXT("ProgramListChannelBackGradient"),          Theme::GradientDirection::Vert, true,  COLOR_PROGRAMLISTPANEL_CHANNELBACK1,          COLOR_PROGRAMLISTPANEL_CHANNELBACK2},
	{TEXT("ProgramListCurChannelBackGradient"),       Theme::GradientDirection::Vert, true,  COLOR_PROGRAMLISTPANEL_CURCHANNELBACK1,       COLOR_PROGRAMLISTPANEL_CURCHANNELBACK2},
	{TEXT("ProgramListChannelButtonBackGradient"),    Theme::GradientDirection::Vert, true,  COLOR_PROGRAMLISTPANEL_CHANNELBUTTONBACK1,    COLOR_PROGRAMLISTPANEL_CHANNELBUTTONBACK2},
	{TEXT("ProgramListChannelButtonHotBackGradient"), Theme::GradientDirection::Vert, true,  COLOR_PROGRAMLISTPANEL_CHANNELBUTTONHOTBACK1, COLOR_PROGRAMLISTPANEL_CHANNELBUTTONHOTBACK2},
	{TEXT("ProgramListBackGradient"),                 Theme::GradientDirection::Vert, true,  COLOR_PROGRAMLISTPANEL_EVENTBACK1,            COLOR_PROGRAMLISTPANEL_EVENTBACK2},
	{TEXT("ProgramListCurBackGradient"),              Theme::GradientDirection::Vert, true,  COLOR_PROGRAMLISTPANEL_CUREVENTBACK1,         COLOR_PROGRAMLISTPANEL_CUREVENTBACK2},
	{TEXT("ProgramListTitleBackGradient"),            Theme::GradientDirection::Vert, true,  COLOR_PROGRAMLISTPANEL_TITLEBACK1,            COLOR_PROGRAMLISTPANEL_TITLEBACK2},
	{TEXT("ProgramListCurTitleBackGradient"),         Theme::GradientDirection::Vert, true,  COLOR_PROGRAMLISTPANEL_CURTITLEBACK1,         COLOR_PROGRAMLISTPANEL_CURTITLEBACK2},
	{TEXT("ChannelPanelChannelNameBackGradient"),     Theme::GradientDirection::Vert, true,  COLOR_CHANNELPANEL_CHANNELNAMEBACK1,          COLOR_CHANNELPANEL_CHANNELNAMEBACK2},
	{TEXT("ChannelPanelCurChannelNameBackGradient"),  Theme::GradientDirection::Vert, true,  COLOR_CHANNELPANEL_CURCHANNELNAMEBACK1,       COLOR_CHANNELPANEL_CURCHANNELNAMEBACK2},
	{TEXT("ChannelPanelEventNameBackGradient"),       Theme::GradientDirection::Vert, true,  COLOR_CHANNELPANEL_EVENTNAME1BACK1,           COLOR_CHANNELPANEL_EVENTNAME1BACK2},
	{TEXT("ChannelPanelEventName2BackGradient"),      Theme::GradientDirection::Vert, true,  COLOR_CHANNELPANEL_EVENTNAME2BACK1,           COLOR_CHANNELPANEL_EVENTNAME2BACK2},
	{TEXT("ChannelPanelCurEventNameBackGradient"),    Theme::GradientDirection::Vert, true,  COLOR_CHANNELPANEL_CUREVENTNAME1BACK1,        COLOR_CHANNELPANEL_CUREVENTNAME1BACK2},
	{TEXT("ChannelPanelCurEventName2BackGradient"),   Theme::GradientDirection::Vert, true,  COLOR_CHANNELPANEL_CUREVENTNAME2BACK1,        COLOR_CHANNELPANEL_CUREVENTNAME2BACK2},
	{TEXT("ChannelPanelFeaturedMarkGradient"),        Theme::GradientDirection::Vert, true,  COLOR_CHANNELPANEL_FEATUREDMARK1,             COLOR_CHANNELPANEL_FEATUREDMARK2},
	{TEXT("ChannelPanelProgressGradient"),            Theme::GradientDirection::Vert, true,  COLOR_CHANNELPANEL_PROGRESS1,                 COLOR_CHANNELPANEL_PROGRESS2},
	{TEXT("ChannelPanelCurProgressGradient"),         Theme::GradientDirection::Vert, true,  COLOR_CHANNELPANEL_CURPROGRESS1,              COLOR_CHANNELPANEL_CURPROGRESS2},
	{TEXT("ControlPanelBackGradient"),                Theme::GradientDirection::Vert, true,  COLOR_CONTROLPANELBACK1,                      COLOR_CONTROLPANELBACK2},
	{TEXT("ControlPanelHighlightBackGradient"),       Theme::GradientDirection::Vert, true,  COLOR_CONTROLPANELHIGHLIGHTBACK1,             COLOR_CONTROLPANELHIGHLIGHTBACK2},
	{TEXT("ControlPanelCheckedBackGradient"),         Theme::GradientDirection::Vert, true,  COLOR_CONTROLPANELCHECKEDBACK1,               COLOR_CONTROLPANELCHECKEDBACK2},
	{TEXT("TitleBarBackGradient"),                    Theme::GradientDirection::Vert, true,  COLOR_TITLEBARBACK1,                          COLOR_TITLEBARBACK2},
	{TEXT("TitleBarIconBackGradient"),                Theme::GradientDirection::Vert, true,  COLOR_TITLEBARICONBACK1,                      COLOR_TITLEBARICONBACK2},
	{TEXT("TitleBarHighlightBackGradient"),           Theme::GradientDirection::Vert, true,  COLOR_TITLEBARHIGHLIGHTBACK1,                 COLOR_TITLEBARHIGHLIGHTBACK2},
	{TEXT("SideBarBackGradient"),                     Theme::GradientDirection::Horz, true,  COLOR_SIDEBARBACK1,                           COLOR_SIDEBARBACK2},
	{TEXT("SideBarHighlightBackGradient"),            Theme::GradientDirection::Horz, true,  COLOR_SIDEBARHIGHLIGHTBACK1,                  COLOR_SIDEBARHIGHLIGHTBACK2},
	{TEXT("SideBarCheckBackGradient"),                Theme::GradientDirection::Horz, true,  COLOR_SIDEBARCHECKBACK1,                      COLOR_SIDEBARCHECKBACK2},
	{TEXT("NotificationBarBackGradient"),             Theme::GradientDirection::Vert, true,  COLOR_NOTIFICATIONBARBACK1,                   COLOR_NOTIFICATIONBARBACK2},
	{TEXT("ProgramGuideFeaturedMarkGradient"),        Theme::GradientDirection::Horz, true,  COLOR_PROGRAMGUIDE_FEATUREDMARK1,             COLOR_PROGRAMGUIDE_FEATUREDMARK2},
	{TEXT("ProgramGuideChannelBackGradient"),         Theme::GradientDirection::Vert, true,  COLOR_PROGRAMGUIDE_CHANNELBACK1,              COLOR_PROGRAMGUIDE_CHANNELBACK2},
	{TEXT("ProgramGuideCurChannelBackGradient"),      Theme::GradientDirection::Vert, true,  COLOR_PROGRAMGUIDE_CURCHANNELBACK1,           COLOR_PROGRAMGUIDE_CURCHANNELBACK2},
	{TEXT("ProgramGuideTimeBackGradient"),            Theme::GradientDirection::Horz, true,  COLOR_PROGRAMGUIDE_TIMEBACK1,                 COLOR_PROGRAMGUIDE_TIMEBACK2},
	{TEXT("ProgramGuideTime0To2BackGradient"),        Theme::GradientDirection::Horz, true,  COLOR_PROGRAMGUIDE_TIMEBACK_0TO2_1,           COLOR_PROGRAMGUIDE_TIMEBACK_0TO2_2},
	{TEXT("ProgramGuideTime3To5BackGradient"),        Theme::GradientDirection::Horz, true,  COLOR_PROGRAMGUIDE_TIMEBACK_3TO5_1,           COLOR_PROGRAMGUIDE_TIMEBACK_3TO5_2},
	{TEXT("ProgramGuideTime6To8BackGradient"),        Theme::GradientDirection::Horz, true,  COLOR_PROGRAMGUIDE_TIMEBACK_6TO8_1,           COLOR_PROGRAMGUIDE_TIMEBACK_6TO8_2},
	{TEXT("ProgramGuideTime9To11BackGradient"),       Theme::GradientDirection::Horz, true,  COLOR_PROGRAMGUIDE_TIMEBACK_9TO11_1,          COLOR_PROGRAMGUIDE_TIMEBACK_9TO11_2},
	{TEXT("ProgramGuideTime12To14BackGradient"),      Theme::GradientDirection::Horz, true,  COLOR_PROGRAMGUIDE_TIMEBACK_12TO14_1,         COLOR_PROGRAMGUIDE_TIMEBACK_12TO14_2},
	{TEXT("ProgramGuideTime15To17BackGradient"),      Theme::GradientDirection::Horz, true,  COLOR_PROGRAMGUIDE_TIMEBACK_15TO17_1,         COLOR_PROGRAMGUIDE_TIMEBACK_15TO17_2},
	{TEXT("ProgramGuideTime18To20BackGradient"),      Theme::GradientDirection::Horz, true,  COLOR_PROGRAMGUIDE_TIMEBACK_18TO20_1,         COLOR_PROGRAMGUIDE_TIMEBACK_18TO20_2},
	{TEXT("ProgramGuideTime21To23BackGradient"),      Theme::GradientDirection::Horz, true,  COLOR_PROGRAMGUIDE_TIMEBACK_21TO23_1,         COLOR_PROGRAMGUIDE_TIMEBACK_21TO23_2},
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
};


CColorScheme::CColorScheme()
{
	SetDefault();
	::ZeroMemory(m_LoadedFlags, sizeof(m_LoadedFlags));
}


CColorScheme::CColorScheme(const CColorScheme &ColorScheme)
{
	*this = ColorScheme;
}


CColorScheme::~CColorScheme()
{
}


CColorScheme &CColorScheme::operator=(const CColorScheme &ColorScheme)
{
	if (&ColorScheme != this) {
		::CopyMemory(m_ColorList, ColorScheme.m_ColorList, sizeof(m_ColorList));
		::CopyMemory(m_GradientList, ColorScheme.m_GradientList, sizeof(m_GradientList));
		::CopyMemory(m_BorderList, ColorScheme.m_BorderList, sizeof(m_BorderList));
		m_Name = ColorScheme.m_Name;
		m_FileName = ColorScheme.m_FileName;
		::CopyMemory(m_LoadedFlags, ColorScheme.m_LoadedFlags, sizeof(m_LoadedFlags));
	}
	return *this;
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
	return m_GradientList[Gradient].Type;
}


Theme::GradientType CColorScheme::GetGradientType(LPCTSTR pszText) const
{
	for (int i = 0; i < NUM_GRADIENTS; i++) {
		if (::lstrcmpi(m_GradientInfoList[i].pszText, pszText) == 0)
			return m_GradientList[i].Type;
	}
	return Theme::GradientType::Normal;
}


bool CColorScheme::SetGradientStyle(int Gradient, const GradientStyle &Style)
{
	if (Gradient < 0 || Gradient >= NUM_GRADIENTS)
		return false;
	m_GradientList[Gradient].Type = Style.Type;
	m_GradientList[Gradient].Direction = Style.Direction;
	return true;
}


bool CColorScheme::GetGradientStyle(int Gradient, GradientStyle *pStyle) const
{
	if (Gradient < 0 || Gradient >= NUM_GRADIENTS)
		return false;
	*pStyle = m_GradientList[Gradient];
	return true;
}


bool CColorScheme::GetGradientStyle(int Gradient, Theme::GradientStyle *pStyle) const
{
	if (Gradient < 0 || Gradient >= NUM_GRADIENTS)
		return false;
	pStyle->Type = m_GradientList[Gradient].Type;
	pStyle->Direction = m_GradientList[Gradient].Direction;
	pStyle->Color1 = m_ColorList[m_GradientInfoList[Gradient].Color1];
	pStyle->Color2 = m_ColorList[m_GradientInfoList[Gradient].Color2];
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
	pStyle->Color = m_ColorList[m_BorderInfoList[Border].Color];
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
	::ZeroMemory(m_LoadedFlags, sizeof(m_LoadedFlags));
	for (int i = 0; i < NUM_COLORS; i++) {
		if (Settings.ReadColor(m_ColorInfoList[i].pszText, &m_ColorList[i]))
			SetLoadedFlag(i);
	}

	for (int i = 0; i < NUM_GRADIENTS; i++) {
		if (IsLoaded(m_GradientInfoList[i].Color1)
				&& !IsLoaded(m_GradientInfoList[i].Color2)) {
			m_ColorList[m_GradientInfoList[i].Color2] = m_ColorList[m_GradientInfoList[i].Color1];
			SetLoadedFlag(m_GradientInfoList[i].Color2);
			m_GradientList[i].Type = Theme::GradientType::Normal;
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
		{COLOR_PROGRAMGUIDE_CURCHANNELTEXT,           COLOR_PROGRAMGUIDE_CHANNELTEXT},
		{COLOR_PROGRAMGUIDE_TIMELINE,                 COLOR_PROGRAMGUIDE_TIMETEXT},
	};

	for (int i = 0; i < lengthof(ColorMap); i++) {
		const int To = ColorMap[i].To;
		if (!IsLoaded(To) && IsLoaded(ColorMap[i].From)) {
			m_ColorList[To] = m_ColorList[ColorMap[i].From];
			SetLoadedFlag(To);
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

	for (int i = 0; i < lengthof(MixMap); i++) {
		const int To = MixMap[i].To;
		if (!IsLoaded(To) && IsLoaded(MixMap[i].From1) && IsLoaded(MixMap[i].From2)) {
			m_ColorList[To] = MixColor(m_ColorList[MixMap[i].From1], m_ColorList[MixMap[i].From2]);
			SetLoadedFlag(To);
		}
	}

	for (int i = 0; i < NUM_GRADIENTS; i++) {
		if (Settings.Read(m_GradientInfoList[i].pszText, szText, lengthof(szText))) {
			if (szText[0] == '\0' || ::lstrcmpi(szText, TEXT("normal")) == 0)
				m_GradientList[i].Type = Theme::GradientType::Normal;
			else if (::lstrcmpi(szText, TEXT("glossy")) == 0)
				m_GradientList[i].Type = Theme::GradientType::Glossy;
			else if (::lstrcmpi(szText, TEXT("interlaced")) == 0)
				m_GradientList[i].Type = Theme::GradientType::Interlaced;
		} else {
			switch (i) {
			case GRADIENT_TITLEBARICON:
				m_GradientList[i].Type = m_GradientList[GRADIENT_TITLEBARBACK].Type;
				break;
			case GRADIENT_SIDEBARCHECKBACK:
				m_GradientList[i].Type = m_GradientList[GRADIENT_SIDEBARBACK].Type;
				break;
			}
		}

		TCHAR szName[128];
		StringPrintf(szName, TEXT("%sDirection"), m_GradientInfoList[i].pszText);
		m_GradientList[i].Direction = m_GradientInfoList[i].Direction;
		if (Settings.Read(szName, szText, lengthof(szText))) {
			for (int j = 0; j < lengthof(GradientDirectionList); j++) {
				if (::lstrcmpi(szText, GradientDirectionList[j]) == 0) {
					m_GradientList[i].Direction = (Theme::GradientDirection)j;
					break;
				}
			}
		} else {
			switch (i) {
			case GRADIENT_TITLEBARICON:
				m_GradientList[i].Direction = m_GradientList[GRADIENT_TITLEBARBACK].Direction;
				break;
			case GRADIENT_SIDEBARCHECKBACK:
				m_GradientList[i].Direction = m_GradientList[GRADIENT_SIDEBARBACK].Direction;
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
	};

	for (const auto &Map : GradientMap) {
		const int To = Map.To, From = Map.From;
		if (!IsLoaded(m_GradientInfoList[To].Color1)
				&& IsLoaded(m_GradientInfoList[From].Color1)) {
			m_ColorList[m_GradientInfoList[To].Color1] = m_ColorList[m_GradientInfoList[From].Color1];
			m_ColorList[m_GradientInfoList[To].Color2] = m_ColorList[m_GradientInfoList[From].Color2];
			SetLoadedFlag(m_GradientInfoList[To].Color1);
			SetLoadedFlag(m_GradientInfoList[To].Color2);
			m_GradientList[To] = m_GradientList[From];
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
				bool fLoaded = true;
				if (::lstrcmpi(szText, TEXT("none")) == 0) {
					m_BorderList[i] = Theme::BorderType::None;
				} else if (::lstrcmpi(szText, TEXT("solid")) == 0)
					m_BorderList[i] = Theme::BorderType::Solid;
				else if (::lstrcmpi(szText, TEXT("sunken")) == 0)
					m_BorderList[i] = Theme::BorderType::Sunken;
				else if (::lstrcmpi(szText, TEXT("raised")) == 0)
					m_BorderList[i] = Theme::BorderType::Raised;
				else
					fLoaded = false;
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
	};

	for (const auto &Map : BorderMap) {
		const int To = Map.To;
		const int From = Map.From;
		if (!BorderLoaded[To] && BorderLoaded[From]) {
			m_BorderList[To] = m_BorderList[From];
		}
		const int ColorTo = m_BorderInfoList[To].Color;
		const int ColorFrom = m_BorderInfoList[From].Color;
		if (!IsLoaded(ColorTo) && IsLoaded(ColorFrom)) {
			m_ColorList[ColorTo] = m_ColorList[ColorFrom];
			SetLoadedFlag(ColorTo);
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
			if (m_GradientList[i] != DefaultColorScheme.m_GradientList[i])
				break;
		}
		if (i == NUM_GRADIENTS)
			fSaveGradients = false;

		if (!fSaveAllColors || !fSaveGradients)
			Settings.Clear();
	}

	if (!(Flags & SaveFlag::NoName))
		Settings.Write(TEXT("Name"), m_Name);

	for (int i = 0; i < NUM_COLORS; i++) {
		if (fSaveAllColors || m_ColorList[i] != m_ColorInfoList[i].DefaultColor)
			Settings.WriteColor(m_ColorInfoList[i].pszText, m_ColorList[i]);
	}

	if (fSaveGradients) {
		for (int i = 0; i < NUM_GRADIENTS; i++) {
			static const LPCTSTR pszTypeName[] = {
				TEXT("normal"), TEXT("glossy"), TEXT("interlaced")
			};
			TCHAR szName[128];

			Settings.Write(m_GradientInfoList[i].pszText, pszTypeName[(int)m_GradientList[i].Type]);
			StringPrintf(szName, TEXT("%sDirection"), m_GradientInfoList[i].pszText);
			Settings.Write(szName, GradientDirectionList[(int)m_GradientList[i].Direction]);
		}
	}

	if (Settings.SetSection(TEXT("Style"))) {
		static const LPCTSTR pszTypeName[] = {
			TEXT("none"), TEXT("solid"), TEXT("sunken"), TEXT("raised")
		};

		for (int i = 0; i < NUM_BORDERS; i++)
			Settings.Write(m_BorderInfoList[i].pszText, pszTypeName[(int)m_BorderList[i]]);
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


void CColorScheme::SetDefault()
{
	for (int i = 0; i < NUM_COLORS; i++)
		m_ColorList[i] = m_ColorInfoList[i].DefaultColor;
	for (int i = 0; i < NUM_GRADIENTS; i++) {
		m_GradientList[i].Type = Theme::GradientType::Normal;
		m_GradientList[i].Direction = m_GradientInfoList[i].Direction;
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


COLORREF CColorScheme::GetDefaultColor(int Type)
{
	if (Type < 0 || Type >= NUM_COLORS)
		return CLR_INVALID;
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
	return (m_LoadedFlags[Type / 32] & (1 << (Type % 32))) != 0;
}


void CColorScheme::SetLoaded()
{
	::FillMemory(m_LoadedFlags, sizeof(m_LoadedFlags), 0xFF);
}


bool CColorScheme::CompareScheme(const CColorScheme &Scheme) const
{
	for (int i = 0; i < NUM_COLORS; i++) {
		if (Scheme.IsLoaded(i) && m_ColorList[i] != Scheme.m_ColorList[i])
			return false;
	}

	for (int i = 0; i < NUM_GRADIENTS; i++) {
		if (m_GradientList[i] != Scheme.m_GradientList[i])
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
	m_LoadedFlags[Color / 32] |= 1 << (Color % 32);
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
	if ((size_t)Index >= m_List.size())
		return Add(pColorScheme);
	auto i = m_List.begin();
	std::advance(i, Index);
	m_List.emplace(i, pColorScheme);
	return true;
}


bool CColorSchemeList::Load(LPCTSTR pszDirectory)
{
	HANDLE hFind;
	WIN32_FIND_DATA wfd;
	TCHAR szFileName[MAX_PATH];

	::PathCombine(szFileName, pszDirectory, TEXT("*.httheme"));
	hFind = ::FindFirstFile(szFileName, &wfd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			CColorScheme *pColorScheme;

			::PathCombine(szFileName, pszDirectory, wfd.cFileName);
			pColorScheme = new CColorScheme;
			if (pColorScheme->Load(szFileName))
				Add(pColorScheme);
			else
				delete pColorScheme;
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
	if (Index < 0 || (size_t)Index >= m_List.size())
		return nullptr;
	return m_List[Index].get();
}


bool CColorSchemeList::SetColorScheme(int Index, const CColorScheme *pColorScheme)
{
	if (Index < 0 || (size_t)Index >= m_List.size() || pColorScheme == nullptr)
		return false;
	*m_List[Index] = *pColorScheme;
	return true;
}


int CColorSchemeList::FindByName(LPCTSTR pszName, int FirstIndex) const
{
	if (pszName == nullptr)
		return -1;

	for (int i = std::max(FirstIndex, 0); i < (int)m_List.size(); i++) {
		if (!IsStringEmpty(m_List[i]->GetName())
				&& ::lstrcmpi(m_List[i]->GetName(), pszName) == 0)
			return i;
	}
	return -1;
}


void CColorSchemeList::SortByName()
{
	if (m_List.size() > 1) {
		std::sort(
			m_List.begin(), m_List.end(),
			[](const std::unique_ptr<CColorScheme> &ColorScheme1,
			   const std::unique_ptr<CColorScheme> &ColorScheme2) -> bool {
				return ::lstrcmpi(ColorScheme1->GetName(), ColorScheme2->GetName()) < 0;
			});
	}
}


}	// namespace TVTest
