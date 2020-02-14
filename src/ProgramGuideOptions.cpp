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
#include "ProgramGuideOptions.h"
#include "DirectWriteOptionsDialog.h"
#include "DialogUtil.h"
#include "StyleUtil.h"
#include "DPIUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace
{

const LPCTSTR IEPG_ASSOCIATE_COMMAND = TEXT("iEpgAssociate");

}




CProgramGuideOptions::CProgramGuideOptions(CProgramGuide *pProgramGuide, CPluginManager *pPluginManager)
	: m_pProgramGuide(pProgramGuide)
	, m_pPluginManager(pPluginManager)
	, m_fOnScreen(false)
	, m_fScrollToCurChannel(false)
	, m_BeginHour(-1)
	, m_ViewHours(26)
	, m_ItemWidth(pProgramGuide->GetItemWidth())
	, m_LinesPerHour(pProgramGuide->GetLinesPerHour())
	, m_fUseDirectWrite(false)
	, m_VisibleEventIcons(m_pProgramGuide->GetVisibleEventIcons())
	, m_WheelScrollLines(pProgramGuide->GetWheelScrollLines())
{
	m_pProgramGuide->GetFont(&m_Font);

	m_DirectWriteRenderingParams.Gamma = 2.2f;
	m_DirectWriteRenderingParams.EnhancedContrast = 0.5f;
	m_DirectWriteRenderingParams.ClearTypeLevel = 0.5f;
	m_DirectWriteRenderingParams.PixelGeometry = DWRITE_PIXEL_GEOMETRY_RGB;
	m_DirectWriteRenderingParams.RenderingMode = DWRITE_RENDERING_MODE_DEFAULT;
}


CProgramGuideOptions::~CProgramGuideOptions()
{
	Destroy();
}


static bool CSVNextValue(LPTSTR *ppszText)
{
	LPTSTR p = *ppszText;
	while (*p == _T(' ') || *p == _T('\t'))
		p++;
	if (*p != _T(','))
		return false;
	*ppszText = p + 1;
	return true;
}

bool CProgramGuideOptions::LoadSettings(CSettings &Settings)
{
	CProgramSearchDialog *pProgramSearch = m_pProgramGuide->GetProgramSearch();

	if (Settings.SetSection(TEXT("ProgramGuide"))) {
		int Value;
		bool f;

		Settings.Read(TEXT("OnScreen"), &m_fOnScreen);
		Settings.Read(TEXT("ScrollToCurChannel"), &m_fScrollToCurChannel);
		if (Settings.Read(TEXT("BeginHour"), &Value)
				&& Value >= -1 && Value <= 23)
			m_BeginHour = Value;
		m_pProgramGuide->SetBeginHour(m_BeginHour);
		if (Settings.Read(TEXT("ViewHours"), &Value)
				&& Value >= MIN_VIEW_HOURS && Value <= MAX_VIEW_HOURS)
			m_ViewHours = Value;
		if (Settings.Read(TEXT("ItemWidth"), &Value)
				&& Value >= CProgramGuide::MIN_ITEM_WIDTH
				&& Value <= CProgramGuide::MAX_ITEM_WIDTH)
			m_ItemWidth = Value;
		if (Settings.Read(TEXT("LinesPerHour"), &Value)
				&& Value >= CProgramGuide::MIN_LINES_PER_HOUR
				&& Value <= CProgramGuide::MAX_LINES_PER_HOUR)
			m_LinesPerHour = Value;
		m_pProgramGuide->SetUIOptions(m_LinesPerHour, m_ItemWidth);

		Settings.Read(TEXT("EventIcons"), &m_VisibleEventIcons);
		m_pProgramGuide->SetVisibleEventIcons(m_VisibleEventIcons);

		if (Settings.Read(TEXT("WheelScrollLines"), &Value))
			m_WheelScrollLines = Value;
		m_pProgramGuide->SetWheelScrollLines(m_WheelScrollLines);

		Settings.Read(TEXT("ProgramLDoubleClick"), &m_ProgramLDoubleClickCommand);

		if (Settings.Read(TEXT("ShowToolTip"), &f))
			m_pProgramGuide->SetShowToolTip(f);

		if (StyleUtil::ReadFontSettings(Settings, TEXT("Font"), &m_Font, true, &f)) {
			if (!f)
				m_fChanged = true;
		}
		m_pProgramGuide->SetFont(m_Font);

		Settings.Read(TEXT("UseDirectWrite"), &m_fUseDirectWrite);
		m_pProgramGuide->SetTextDrawEngine(
			m_fUseDirectWrite ?
				CTextDrawClient::TextDrawEngine::DirectWrite :
				CTextDrawClient::TextDrawEngine::GDI);

		unsigned int Mask;
		if (Settings.Read(TEXT("DirectWriteRenderingParamsMask"), &Mask))
			m_DirectWriteRenderingParams.Mask = static_cast<CDirectWriteRenderer::RenderingParams::ParamFlag>(Mask);
		Settings.Read(TEXT("DirectWriteGamma"), &m_DirectWriteRenderingParams.Gamma);
		Settings.Read(TEXT("DirectWriteEnhancedContrast"), &m_DirectWriteRenderingParams.EnhancedContrast);
		Settings.Read(TEXT("DirectWriteClearTypeLevel"), &m_DirectWriteRenderingParams.ClearTypeLevel);
		if (Settings.Read(TEXT("DirectWritePixelGeometry"), &Value))
			m_DirectWriteRenderingParams.PixelGeometry = static_cast<DWRITE_PIXEL_GEOMETRY>(Value);
		if (Settings.Read(TEXT("DirectWriteRenderingMode"), &Value))
			m_DirectWriteRenderingParams.RenderingMode = static_cast<DWRITE_RENDERING_MODE>(Value);
		m_pProgramGuide->SetDirectWriteRenderingParams(m_DirectWriteRenderingParams);

		bool fDragScroll;
		if (Settings.Read(TEXT("DragScroll"), &fDragScroll))
			m_pProgramGuide->SetDragScroll(fDragScroll);

		unsigned int Filter;
		if (Settings.Read(TEXT("Filter"), &Filter))
			m_pProgramGuide->SetFilter(Filter);

		bool fKeepTimePos;
		if (Settings.Read(TEXT("KeepTimePos"), &fKeepTimePos))
			m_pProgramGuide->SetKeepTimePos(fKeepTimePos);

		bool fShowFeaturedMark;
		if (Settings.Read(TEXT("ShowFeaturedMark"), &fShowFeaturedMark))
			m_pProgramGuide->SetShowFeaturedMark(fShowFeaturedMark);

		bool fExcludeNoEvent;
		if (Settings.Read(TEXT("ExcludeNoEventServices"), &fExcludeNoEvent))
			m_pProgramGuide->SetExcludeNoEventServices(fExcludeNoEvent);

		int Width, Height;
		m_pProgramGuide->GetInfoPopupSize(&Width, &Height);
		Settings.Read(TEXT("InfoPopupWidth"), &Width);
		Settings.Read(TEXT("InfoPopupHeight"), &Height);
		m_pProgramGuide->SetInfoPopupSize(Width, Height);

		// 検索履歴
		int NumSearchKeywords;
		if (Settings.Read(TEXT("NumSearchKeywords"), &NumSearchKeywords)
				&& NumSearchKeywords > 0) {
			if (NumSearchKeywords > CProgramSearchDialog::MAX_KEYWORD_HISTORY)
				NumSearchKeywords = CProgramSearchDialog::MAX_KEYWORD_HISTORY;
			std::vector<String> Keywords(NumSearchKeywords);
			size_t j = 0;
			for (int i = 0; i < NumSearchKeywords; i++) {
				TCHAR szName[32];

				StringPrintf(szName, TEXT("SearchKeyword%d"), i);
				if (Settings.Read(szName, &Keywords[j]) && !Keywords[j].empty())
					j++;
			}
			if (j > 0) {
				pProgramSearch->GetOptions().SetKeywordHistory(Keywords.data(), j);
			}
		}

		if (Settings.Read(TEXT("HighlightSearchResult"), &f))
			pProgramSearch->SetHighlightResult(f);

		for (int i = 0; i < CProgramSearchDialog::NUM_COLUMNS; i++) {
			TCHAR szName[32];

			StringPrintf(szName, TEXT("SearchColumn%d_Width"), i);
			if (Settings.Read(szName, &Value))
				pProgramSearch->SetColumnWidth(i, Value);
		}

		if (Settings.Read(TEXT("SearchResultListHeight"), &Value))
			pProgramSearch->SetResultListHeight(Value);

		if (Settings.Read(TEXT("SearchTarget"), &Value))
			pProgramSearch->SetSearchTarget(Value);

		CBasicDialog::Position Pos;
		if (Settings.Read(TEXT("SearchLeft"), &Pos.x)
				&& Settings.Read(TEXT("SearchTop"), &Pos.y)) {
			pProgramSearch->GetPosition(nullptr, nullptr, &Pos.Width, &Pos.Height);
			Settings.Read(TEXT("SearchWidth"), &Pos.Width);
			Settings.Read(TEXT("SearchHeight"), &Pos.Height);
			pProgramSearch->SetPosition(Pos);
		}
	}

	if (Settings.SetSection(TEXT("ProgramGuideTools"))) {
		unsigned int NumTools;

		if (Settings.Read(TEXT("ToolCount"), &NumTools) && NumTools > 0) {
			CProgramGuideToolList *pToolList = m_pProgramGuide->GetToolList();
			String ToolName, Command;

			for (unsigned int i = 0; i < NumTools; i++) {
				TCHAR szName[32];

				StringPrintf(szName, TEXT("Tool%u_Name"), i);
				if (!Settings.Read(szName, &ToolName) || ToolName.empty())
					break;
				StringPrintf(szName, TEXT("Tool%u_Command"), i);
				if (!Settings.Read(szName, &Command) || Command.empty())
					break;
				pToolList->Add(new CProgramGuideTool(ToolName, Command));
			}
		}
	}

	if (Settings.SetSection(TEXT("ProgramGuideService"))) {
		int ServiceCount;
		if (Settings.Read(TEXT("ExcludeServiceCount"), &ServiceCount) && ServiceCount > 0) {
			for (int i = 0; i < ServiceCount; i++) {
				TCHAR szName[32], szText[64];

				StringPrintf(szName, TEXT("ExcludeService%d"), i);
				if (!Settings.Read(szName, szText, lengthof(szText))
						|| szText[0] == _T('\0'))
					break;

				LPTSTR p = szText;
				WORD NetworkID = (WORD)_tcstoul(p, &p, 0);
				if (!CSVNextValue(&p))
					continue;
				WORD TransportStreamID = (WORD)_tcstoul(p, &p, 0);
				if (!CSVNextValue(&p))
					continue;
				WORD ServiceID = (WORD)_tcstoul(p, &p, 0);
				if (ServiceID == 0)
					continue;

				m_pProgramGuide->SetExcludeService(NetworkID, TransportStreamID, ServiceID, true);
			}
		}
	}

	CProgramGuideFavorites *pFavorites = m_pProgramGuide->GetFavorites();
	bool fFavoritesExists = false;
	if (Settings.SetSection(TEXT("ProgramGuideFavorites"))) {
		int FavoritesCount;

		if (Settings.Read(TEXT("FavoritesCount"), &FavoritesCount)) {
			fFavoritesExists = true;

			if (FavoritesCount >= 0) {
				CProgramGuideFavorites::FavoriteInfo FavoriteInfo;

				for (int i = 0; i < FavoritesCount; i++) {
					TCHAR szName[32];

					StringPrintf(szName, TEXT("Favorite%d_Name"), i);
					if (!Settings.Read(szName, &FavoriteInfo.Name)
							|| FavoriteInfo.Name.empty())
						break;
					StringPrintf(szName, TEXT("Favorite%d_Group"), i);
					if (!Settings.Read(szName, &FavoriteInfo.GroupID))
						break;
					StringPrintf(szName, TEXT("Favorite%d_Label"), i);
					if (!Settings.Read(szName, &FavoriteInfo.Label))
						break;
					FavoriteInfo.SetDefaultColors();
					StringPrintf(szName, TEXT("Favorite%d_BackColor"), i);
					Settings.ReadColor(szName, &FavoriteInfo.BackColor);
					StringPrintf(szName, TEXT("Favorite%d_TextColor"), i);
					Settings.ReadColor(szName, &FavoriteInfo.TextColor);

					pFavorites->Add(FavoriteInfo);
				}
			}
		}

		bool fFixedWidth;
		if (Settings.Read(TEXT("FixedWidth"), &fFixedWidth))
			pFavorites->SetFixedWidth(fFixedWidth);
	}
	if (!fFavoritesExists) {
		CProgramGuideFavorites::FavoriteInfo FavoriteInfo;

		FavoriteInfo.Name = TEXT("お気に入りチャンネル");
		FavoriteInfo.GroupID = TEXT("\\");
		FavoriteInfo.Label = TEXT("お気に入り");
		FavoriteInfo.SetDefaultColors();
		pFavorites->Add(FavoriteInfo);
	}

	if (Settings.SetSection(TEXT("ProgramGuideSearchSettings"))) {
		pProgramSearch->GetOptions().LoadSearchSettings(Settings, TEXT("Settings"));
	}

	return true;
}


bool CProgramGuideOptions::SaveSettings(CSettings &Settings)
{
	const CProgramSearchDialog *pProgramSearch = m_pProgramGuide->GetProgramSearch();

	if (Settings.SetSection(TEXT("ProgramGuide"))) {
		Settings.Write(TEXT("OnScreen"), m_fOnScreen);
		Settings.Write(TEXT("ScrollToCurChannel"), m_fScrollToCurChannel);
		Settings.Write(TEXT("BeginHour"), m_BeginHour);
		Settings.Write(TEXT("ViewHours"), m_ViewHours);
		Settings.Write(TEXT("ItemWidth"), m_ItemWidth);
		Settings.Write(TEXT("LinesPerHour"), m_LinesPerHour);
		Settings.Write(TEXT("EventIcons"), m_VisibleEventIcons);
		Settings.Write(TEXT("WheelScrollLines"), m_WheelScrollLines);
		Settings.Write(TEXT("ProgramLDoubleClick"), m_ProgramLDoubleClickCommand);

		StyleUtil::WriteFontSettings(Settings, TEXT("Font"), m_Font);

		Settings.Write(TEXT("UseDirectWrite"), m_fUseDirectWrite);
		Settings.Write(TEXT("DirectWriteRenderingParamsMask"), static_cast<unsigned int>(m_DirectWriteRenderingParams.Mask));
		Settings.Write(TEXT("DirectWriteGamma"), m_DirectWriteRenderingParams.Gamma, 2);
		Settings.Write(TEXT("DirectWriteEnhancedContrast"), m_DirectWriteRenderingParams.EnhancedContrast, 2);
		Settings.Write(TEXT("DirectWriteClearTypeLevel"), m_DirectWriteRenderingParams.ClearTypeLevel, 2);
		Settings.Write(
			TEXT("DirectWritePixelGeometry"),
			static_cast<int>(m_DirectWriteRenderingParams.PixelGeometry));
		Settings.Write(
			TEXT("DirectWriteRenderingMode"),
			static_cast<int>(m_DirectWriteRenderingParams.RenderingMode));

		Settings.Write(TEXT("DragScroll"), m_pProgramGuide->GetDragScroll());
		Settings.Write(TEXT("ShowToolTip"), m_pProgramGuide->GetShowToolTip());
		Settings.Write(TEXT("Filter"), m_pProgramGuide->GetFilter());
		Settings.Write(TEXT("KeepTimePos"), m_pProgramGuide->GetKeepTimePos());
		Settings.Write(TEXT("ShowFeaturedMark"), m_pProgramGuide->GetShowFeaturedMark());
		Settings.Write(TEXT("ExcludeNoEventServices"), m_pProgramGuide->GetExcludeNoEventServices());

		int Width, Height;
		m_pProgramGuide->GetInfoPopupSize(&Width, &Height);
		Settings.Write(TEXT("InfoPopupWidth"), Width);
		Settings.Write(TEXT("InfoPopupHeight"), Height);

		int NumSearchKeywords = pProgramSearch->GetOptions().GetKeywordHistoryCount();
		Settings.Write(TEXT("NumSearchKeywords"), NumSearchKeywords);
		for (int i = 0; i < NumSearchKeywords; i++) {
			TCHAR szName[32];

			StringPrintf(szName, TEXT("SearchKeyword%d"), i);
			Settings.Write(szName, pProgramSearch->GetOptions().GetKeywordHistory(i));
		}

		Settings.Write(TEXT("HighlightSearchResult"), pProgramSearch->GetHighlightResult());

		for (int i = 0; i < CProgramSearchDialog::NUM_COLUMNS; i++) {
			TCHAR szName[32];

			StringPrintf(szName, TEXT("SearchColumn%d_Width"), i);
			Settings.Write(szName, pProgramSearch->GetColumnWidth(i));
		}

		Settings.Write(TEXT("SearchResultListHeight"), pProgramSearch->GetResultListHeight());
		Settings.Write(TEXT("SearchTarget"), pProgramSearch->GetSearchTarget());

		if (pProgramSearch->IsPositionSet()) {
			int Left, Top;
			pProgramSearch->GetPosition(&Left, &Top, &Width, &Height);
			Settings.Write(TEXT("SearchLeft"), Left);
			Settings.Write(TEXT("SearchTop"), Top);
			Settings.Write(TEXT("SearchWidth"), Width);
			Settings.Write(TEXT("SearchHeight"), Height);
		}
	}

	if (Settings.SetSection(TEXT("ProgramGuideTools"))) {
		const CProgramGuideToolList *pToolList = m_pProgramGuide->GetToolList();

		Settings.Clear();
		Settings.Write(TEXT("ToolCount"), (unsigned int)pToolList->NumTools());
		for (size_t i = 0; i < pToolList->NumTools(); i++) {
			const CProgramGuideTool *pTool = pToolList->GetTool(i);
			TCHAR szName[32];

			StringPrintf(szName, TEXT("Tool%u_Name"), (UINT)i);
			Settings.Write(szName, pTool->GetName());
			StringPrintf(szName, TEXT("Tool%u_Command"), (UINT)i);
			Settings.Write(szName, pTool->GetCommand());
		}
	}

	if (Settings.SetSection(TEXT("ProgramGuideService"))) {
		CProgramGuide::ServiceInfoList ExcludeServiceList;

		if (m_pProgramGuide->GetExcludeServiceList(&ExcludeServiceList)) {
			Settings.Clear();
			Settings.Write(TEXT("ExcludeServiceCount"), (unsigned int)ExcludeServiceList.size());
			for (size_t i = 0; i < ExcludeServiceList.size(); i++) {
				TCHAR szName[32], szText[64];
				StringPrintf(szName, TEXT("ExcludeService%u"), (unsigned int)i);
				StringPrintf(
					szText, TEXT("%u,%u,%u"),
					ExcludeServiceList[i].NetworkID,
					ExcludeServiceList[i].TransportStreamID,
					ExcludeServiceList[i].ServiceID);
				Settings.Write(szName, szText);
			}
		}
	}

	if (Settings.SetSection(TEXT("ProgramGuideFavorites"))) {
		const CProgramGuideFavorites *pFavorites = m_pProgramGuide->GetFavorites();
		const int FavoritesCount = (int)pFavorites->GetCount();

		Settings.Clear();
		Settings.Write(TEXT("FavoritesCount"), FavoritesCount);
		for (int i = 0; i < FavoritesCount; i++) {
			const CProgramGuideFavorites::FavoriteInfo *pFavoriteInfo = pFavorites->Get(i);
			TCHAR szName[32];

			StringPrintf(szName, TEXT("Favorite%d_Name"), i);
			Settings.Write(szName, pFavoriteInfo->Name);
			StringPrintf(szName, TEXT("Favorite%d_Group"), i);
			Settings.Write(szName, pFavoriteInfo->GroupID);
			StringPrintf(szName, TEXT("Favorite%d_Label"), i);
			Settings.Write(szName, pFavoriteInfo->Label);
			StringPrintf(szName, TEXT("Favorite%d_BackColor"), i);
			Settings.WriteColor(szName, pFavoriteInfo->BackColor);
			StringPrintf(szName, TEXT("Favorite%d_TextColor"), i);
			Settings.WriteColor(szName, pFavoriteInfo->TextColor);
		}

		Settings.Write(TEXT("FixedWidth"), pFavorites->GetFixedWidth());
	}

	if (Settings.SetSection(TEXT("ProgramGuideSearchSettings"))) {
		Settings.Clear();
		pProgramSearch->GetOptions().SaveSearchSettings(Settings, TEXT("Settings"));
	}

	return true;
}


bool CProgramGuideOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(
		hwndOwner,
		GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_OPTIONS_PROGRAMGUIDE));
}


bool CProgramGuideOptions::GetTimeRange(LibISDB::DateTime *pFirst, LibISDB::DateTime *pLast)
{
	LibISDB::GetCurrentEPGTime(pFirst);
	pFirst->TruncateToHours();
	*pLast = *pFirst;
	pLast->OffsetHours(m_ViewHours);
	return true;
}


int CProgramGuideOptions::ParseCommand(LPCTSTR pszCommand) const
{
	if (IsStringEmpty(pszCommand))
		return 0;
	if (::lstrcmpi(pszCommand, IEPG_ASSOCIATE_COMMAND) == 0)
		return CM_PROGRAMGUIDE_IEPGASSOCIATE;
	return 0;
}


INT_PTR CProgramGuideOptions::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			DlgCheckBox_Check(hDlg, IDC_PROGRAMGUIDEOPTIONS_ONSCREEN, m_fOnScreen);
			DlgCheckBox_Check(hDlg, IDC_PROGRAMGUIDEOPTIONS_SCROLLTOCURCHANNEL, m_fScrollToCurChannel);
			DlgComboBox_AddString(hDlg, IDC_PROGRAMGUIDEOPTIONS_BEGINHOUR, TEXT("現在時"));
			for (int i = 0; i <= 23; i++) {
				TCHAR szText[8];
				StringPrintf(szText, TEXT("%d時"), i);
				DlgComboBox_AddString(hDlg, IDC_PROGRAMGUIDEOPTIONS_BEGINHOUR, szText);
			}
			DlgComboBox_SetCurSel(hDlg, IDC_PROGRAMGUIDEOPTIONS_BEGINHOUR, m_BeginHour + 1);
			DlgEdit_SetInt(hDlg, IDC_PROGRAMGUIDEOPTIONS_VIEWHOURS, m_ViewHours);
			DlgUpDown_SetRange(hDlg, IDC_PROGRAMGUIDEOPTIONS_VIEWHOURS_UD, MIN_VIEW_HOURS, MAX_VIEW_HOURS);
			DlgEdit_SetInt(hDlg, IDC_PROGRAMGUIDEOPTIONS_CHANNELWIDTH, m_ItemWidth);
			DlgUpDown_SetRange(
				hDlg, IDC_PROGRAMGUIDEOPTIONS_CHANNELWIDTH_UD,
				CProgramGuide::MIN_ITEM_WIDTH, CProgramGuide::MAX_ITEM_WIDTH);
			DlgEdit_SetInt(hDlg, IDC_PROGRAMGUIDEOPTIONS_LINESPERHOUR, m_LinesPerHour);
			DlgUpDown_SetRange(
				hDlg, IDC_PROGRAMGUIDEOPTIONS_LINESPERHOUR_UD,
				CProgramGuide::MIN_LINES_PER_HOUR, CProgramGuide::MAX_LINES_PER_HOUR);

			m_CurSettingFont = m_Font;
			StyleUtil::SetFontInfoItem(hDlg, IDC_PROGRAMGUIDEOPTIONS_FONTINFO, m_CurSettingFont);

			DlgCheckBox_Check(hDlg, IDC_PROGRAMGUIDEOPTIONS_USEDIRECTWRITE, m_fUseDirectWrite);
			EnableDlgItem(
				hDlg,
				IDC_PROGRAMGUIDEOPTIONS_DIRECTWRITEOPTIONS,
				m_fUseDirectWrite);

			m_Tooltip.Create(hDlg);
			m_Tooltip.SetFont(GetWindowFont(hDlg));

			{
				HDC hdc = ::GetDC(hDlg);
				HDC hdcMem = ::CreateCompatibleDC(hdc);
				RECT rc;
				::GetClientRect(::GetDlgItem(hDlg, IDC_PROGRAMGUIDEOPTIONS_ICON_FIRST), &rc);
				int IconSize;
				if (rc.bottom <= CEpgIcons::DEFAULT_ICON_HEIGHT + 4)
					IconSize = CEpgIcons::DEFAULT_ICON_HEIGHT;
				else
					IconSize = rc.bottom;
				CEpgIcons EpgIcons;
				EpgIcons.Load();
				EpgIcons.BeginDraw(hdc);
				for (int i = 0; i <= CEpgIcons::ICON_LAST; i++) {
					DlgCheckBox_Check(
						hDlg, IDC_PROGRAMGUIDEOPTIONS_ICON_FIRST + i,
						(m_VisibleEventIcons & CEpgIcons::IconFlag(i)) != 0);
					HBITMAP hbm = ::CreateCompatibleBitmap(hdc, IconSize, IconSize);
					HGDIOBJ hOldBmp = ::SelectObject(hdcMem, hbm);
					EpgIcons.DrawIcon(hdcMem, 0, 0, IconSize, IconSize, i);
					::SelectObject(hdcMem, hOldBmp);
					::SendDlgItemMessage(
						hDlg, IDC_PROGRAMGUIDEOPTIONS_ICON_FIRST + i,
						BM_SETIMAGE, IMAGE_BITMAP,
						reinterpret_cast<LPARAM>(hbm));
					TCHAR szText[64];
					::GetDlgItemText(hDlg, IDC_PROGRAMGUIDEOPTIONS_ICON_FIRST + i, szText, lengthof(szText));
					m_Tooltip.AddTool(::GetDlgItem(hDlg, IDC_PROGRAMGUIDEOPTIONS_ICON_FIRST + i), szText);
				}
				EpgIcons.EndDraw();
				::DeleteDC(hdcMem);
				::ReleaseDC(hDlg, hdc);
			}

			DlgEdit_SetInt(hDlg, IDC_PROGRAMGUIDEOPTIONS_WHEELSCROLLLINES, m_WheelScrollLines);
			DlgUpDown_SetRange(hDlg, IDC_PROGRAMGUIDEOPTIONS_WHEELSCROLLLINES_UD, 0, 100);

			int Sel = m_ProgramLDoubleClickCommand.empty() ? 0 : -1;
			DlgComboBox_AddString(hDlg, IDC_PROGRAMGUIDEOPTIONS_PROGRAMLDOUBLECLICK, TEXT("何もしない"));
			DlgComboBox_AddString(hDlg, IDC_PROGRAMGUIDEOPTIONS_PROGRAMLDOUBLECLICK, TEXT("iEPG関連付け実行"));
			if (Sel < 0 && m_ProgramLDoubleClickCommand.compare(IEPG_ASSOCIATE_COMMAND) == 0)
				Sel = 1;
			for (int i = 0; i < m_pPluginManager->NumPlugins(); i++) {
				const CPlugin *pPlugin = m_pPluginManager->GetPlugin(i);

				for (int j = 0; j < pPlugin->NumProgramGuideCommands(); j++) {
					ProgramGuideCommandInfo CommandInfo;

					pPlugin->GetProgramGuideCommandInfo(j, &CommandInfo);
					if (CommandInfo.Type == PROGRAMGUIDE_COMMAND_TYPE_PROGRAM) {
						TCHAR szCommand[512];

						StringPrintf(
							szCommand, TEXT("%s:%s"),
							::PathFindFileName(pPlugin->GetFileName()),
							CommandInfo.pszText);
						LRESULT Index = DlgComboBox_AddString(
							hDlg, IDC_PROGRAMGUIDEOPTIONS_PROGRAMLDOUBLECLICK,
							CommandInfo.pszName);
						DlgComboBox_SetItemData(
							hDlg, IDC_PROGRAMGUIDEOPTIONS_PROGRAMLDOUBLECLICK,
							Index, reinterpret_cast<LPARAM>(DuplicateString(szCommand)));
						if (Sel < 0 && !m_ProgramLDoubleClickCommand.empty()
								&& StringUtility::CompareNoCase(m_ProgramLDoubleClickCommand, szCommand) == 0)
							Sel = (int)Index;
					}
				}
			}
			if (Sel >= 0)
				DlgComboBox_SetCurSel(hDlg, IDC_PROGRAMGUIDEOPTIONS_PROGRAMLDOUBLECLICK, Sel);

			CProgramGuideToolList *pToolList = m_pProgramGuide->GetToolList();
			HWND hwndList = GetDlgItem(hDlg, IDC_PROGRAMGUIDETOOL_LIST);
			HIMAGELIST himl = ::ImageList_Create(
				GetSystemMetricsWithDPI(SM_CXSMICON, m_CurrentDPI),
				GetSystemMetricsWithDPI(SM_CYSMICON, m_CurrentDPI),
				ILC_COLOR32 | ILC_MASK, 1, 4);
			RECT rc;
			LV_COLUMN lvc;

			ListView_SetImageList(hwndList, himl, LVSIL_SMALL);
			ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
			lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			lvc.fmt = LVCFMT_LEFT;
			lvc.cx = 80;
			lvc.pszText = const_cast<LPTSTR>(TEXT("名前"));
			ListView_InsertColumn(hwndList, 0, &lvc);
			lvc.fmt = LVCFMT_LEFT;
			GetClientRect(hwndList, &rc);
			lvc.cx = rc.right - lvc.cx;
			lvc.pszText = const_cast<LPTSTR>(TEXT("コマンド"));
			ListView_InsertColumn(hwndList, 1, &lvc);
			if (pToolList->NumTools() > 0) {
				ListView_SetItemCount(hwndList, pToolList->NumTools());
				for (size_t i = 0; i < pToolList->NumTools(); i++) {
					CProgramGuideTool *pTool = new CProgramGuideTool(*pToolList->GetTool(i));
					LV_ITEM lvi;

					lvi.mask = LVIF_TEXT | LVIF_PARAM;
					lvi.iItem = (int)i;
					lvi.iSubItem = 0;
					lvi.pszText = const_cast<LPTSTR>(pTool->GetName());
					lvi.lParam = reinterpret_cast<LPARAM>(pTool);
					if (pTool->GetIcon() != nullptr) {
						lvi.iImage = ::ImageList_AddIcon(himl, pTool->GetIcon());
						if (lvi.iImage >= 0)
							lvi.mask |= LVIF_IMAGE;
					}
					ListView_InsertItem(hwndList, &lvi);
					lvi.mask = LVIF_TEXT;
					lvi.iSubItem = 1;
					lvi.pszText = const_cast<LPTSTR>(pTool->GetCommand());
					ListView_SetItem(hwndList, &lvi);
				}
				ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE_USEHEADER);
				ListView_SetColumnWidth(hwndList, 1, LVSCW_AUTOSIZE_USEHEADER);
				SetDlgItemState();
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_PROGRAMGUIDEOPTIONS_CHOOSEFONT:
			if (StyleUtil::ChooseStyleFont(hDlg, &m_CurSettingFont))
				StyleUtil::SetFontInfoItem(hDlg, IDC_PROGRAMGUIDEOPTIONS_FONTINFO, m_CurSettingFont);
			return TRUE;

		case IDC_PROGRAMGUIDEOPTIONS_USEDIRECTWRITE:
			EnableDlgItemSyncCheckBox(
				hDlg,
				IDC_PROGRAMGUIDEOPTIONS_DIRECTWRITEOPTIONS,
				IDC_PROGRAMGUIDEOPTIONS_USEDIRECTWRITE);
			return TRUE;

		case IDC_PROGRAMGUIDEOPTIONS_DIRECTWRITEOPTIONS:
			{
				class CRenderingTester
					: public CDirectWriteOptionsDialog::CRenderingTester
				{
				public:
					CRenderingTester(
						CProgramGuide *pProgramGuide,
						const CDirectWriteRenderer::RenderingParams &Params,
						const Style::Font &Font)
						: m_pProgramGuide(pProgramGuide)
						, m_OldParams(Params)
						, m_Font(Font)
						, m_fApplied(false)
						, m_fFontChanged(false)
					{
					}

					void Apply(const CDirectWriteRenderer::RenderingParams &Params) override
					{
						if (!m_fApplied) {
							Style::Font Font;

							m_OldEngine = m_pProgramGuide->GetTextDrawEngine();
							if (m_OldEngine != CTextDrawClient::TextDrawEngine::DirectWrite)
								m_pProgramGuide->SetTextDrawEngine(CTextDrawClient::TextDrawEngine::DirectWrite);
							m_pProgramGuide->GetFont(&Font);
							if (m_Font != Font) {
								m_pProgramGuide->SetFont(m_Font);
								m_OldFont = Font;
								m_fFontChanged = true;
							}
							m_fApplied = true;
						}
						m_pProgramGuide->SetDirectWriteRenderingParams(Params);
					}

					void Reset() override
					{
						if (m_fApplied) {
							if (m_OldEngine != CTextDrawClient::TextDrawEngine::DirectWrite)
								m_pProgramGuide->SetTextDrawEngine(m_OldEngine);
							if (m_fFontChanged)
								m_pProgramGuide->SetFont(m_OldFont);
							m_pProgramGuide->SetDirectWriteRenderingParams(m_OldParams);
							m_fApplied = false;
						}
					}

					bool IsApplied() const { return m_fApplied; }
					bool IsFontChanged() const { return m_fFontChanged; }

				private:
					CProgramGuide *m_pProgramGuide;
					CDirectWriteRenderer::RenderingParams m_OldParams;
					Style::Font m_Font;
					Style::Font m_OldFont;
					bool m_fApplied;
					bool m_fFontChanged;
					CTextDrawClient::TextDrawEngine m_OldEngine;
				};

				CDirectWriteOptionsDialog Dialog(
					&m_DirectWriteRenderingParams, m_CurSettingFont.LogFont);
				CRenderingTester RenderingTester(
					m_pProgramGuide, m_DirectWriteRenderingParams, m_CurSettingFont);

				if (m_pProgramGuide->GetVisible())
					Dialog.SetRenderingTester(&RenderingTester);
				if (Dialog.Show(hDlg)) {
					if (RenderingTester.IsApplied()) {
#if 0
						if (!m_fUseDirectWrite)
							m_pProgramGuide->SetTextDrawEngine(CTextDrawClient::ENGINE_GDI);
						if (RenderingTester.IsFontChanged())
							m_pProgramGuide->SetFont(m_Font);
#else
						if (!m_fUseDirectWrite) {
							m_fUseDirectWrite = true;
							m_pProgramGuide->SetTextDrawEngine(CTextDrawClient::TextDrawEngine::DirectWrite);
						}
						if (RenderingTester.IsFontChanged()) {
							m_Font = m_CurSettingFont;
							m_pProgramGuide->SetFont(m_Font);
						}
#endif
					}
					m_pProgramGuide->SetDirectWriteRenderingParams(m_DirectWriteRenderingParams);
					m_fChanged = true;
				}
			}
			return TRUE;

		case IDC_PROGRAMGUIDETOOL_ADD:
			{
				CProgramGuideTool *pTool;

				pTool = new CProgramGuideTool;
				if (pTool->ShowDialog(hDlg)) {
					HWND hwndList = GetDlgItem(hDlg, IDC_PROGRAMGUIDETOOL_LIST);
					LV_ITEM lvi;

					lvi.mask = LVIF_STATE | LVIF_TEXT | LVIF_PARAM;
					lvi.iItem = ListView_GetItemCount(hwndList);
					lvi.iSubItem = 0;
					lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
					lvi.stateMask = lvi.state;
					lvi.pszText = const_cast<LPTSTR>(pTool->GetName());
					lvi.lParam = reinterpret_cast<LPARAM>(pTool);
					if (pTool->GetIcon() != nullptr) {
						lvi.iImage = ::ImageList_AddIcon(ListView_GetImageList(hwndList, LVSIL_SMALL), pTool->GetIcon());
						if (lvi.iImage >= 0)
							lvi.mask |= LVIF_IMAGE;
					}
					ListView_InsertItem(hwndList, &lvi);
					lvi.mask = LVIF_TEXT;
					lvi.iSubItem = 1;
					lvi.pszText = const_cast<LPTSTR>(pTool->GetCommand());
					ListView_SetItem(hwndList, &lvi);
					ListView_EnsureVisible(hwndList, lvi.iItem, FALSE);
				} else {
					delete pTool;
				}
			}
			return TRUE;

		case IDC_PROGRAMGUIDETOOL_EDIT:
			{
				HWND hwndList = GetDlgItem(hDlg, IDC_PROGRAMGUIDETOOL_LIST);
				LV_ITEM lvi;
				CProgramGuideTool *pTool;

				lvi.mask = LVIF_PARAM;
				lvi.iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
				lvi.iSubItem = 0;
				if (!ListView_GetItem(hwndList, &lvi))
					return TRUE;
				pTool = reinterpret_cast<CProgramGuideTool*>(lvi.lParam);
				if (pTool->ShowDialog(hDlg)) {
					lvi.mask = LVIF_TEXT;
					lvi.pszText = const_cast<LPTSTR>(pTool->GetName());
					if (pTool->GetIcon() != nullptr) {
						lvi.iImage = ::ImageList_AddIcon(ListView_GetImageList(hwndList, LVSIL_SMALL), pTool->GetIcon());
						if (lvi.iImage >= 0)
							lvi.mask |= LVIF_IMAGE;
					}
					ListView_SetItem(hwndList, &lvi);
					lvi.iSubItem = 1;
					lvi.pszText = const_cast<LPTSTR>(pTool->GetCommand());
					ListView_SetItem(hwndList, &lvi);
				}
			}
			return TRUE;

		case IDC_PROGRAMGUIDETOOL_UP:
			{
				HWND hwndList = GetDlgItem(hDlg, IDC_PROGRAMGUIDETOOL_LIST);
				LV_ITEM lvi;
				CProgramGuideTool *pTool;

				lvi.mask = LVIF_IMAGE | LVIF_PARAM;
				lvi.iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
				if (lvi.iItem <= 0)
					return TRUE;
				lvi.iSubItem = 0;
				ListView_GetItem(hwndList, &lvi);
				pTool = reinterpret_cast<CProgramGuideTool*>(lvi.lParam);
				ListView_DeleteItem(hwndList, lvi.iItem);
				lvi.mask = LVIF_STATE | LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
				lvi.iItem--;
				lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
				lvi.stateMask = lvi.state;
				lvi.pszText = const_cast<LPTSTR>(pTool->GetName());
				ListView_InsertItem(hwndList, &lvi);
				lvi.mask = LVIF_TEXT;
				lvi.iSubItem = 1;
				lvi.pszText = const_cast<LPTSTR>(pTool->GetCommand());
				ListView_SetItem(hwndList, &lvi);
				ListView_EnsureVisible(hwndList, lvi.iItem, FALSE);
			}
			return TRUE;

		case IDC_PROGRAMGUIDETOOL_DOWN:
			{
				HWND hwndList = GetDlgItem(hDlg, IDC_PROGRAMGUIDETOOL_LIST);
				LV_ITEM lvi;
				CProgramGuideTool *pTool;

				lvi.mask = LVIF_IMAGE | LVIF_PARAM;
				lvi.iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
				if (lvi.iItem < 0
						|| lvi.iItem + 1 == ListView_GetItemCount(hwndList))
					return TRUE;
				lvi.iSubItem = 0;
				ListView_GetItem(hwndList, &lvi);
				pTool = reinterpret_cast<CProgramGuideTool*>(lvi.lParam);
				ListView_DeleteItem(hwndList, lvi.iItem);
				lvi.mask = LVIF_STATE | LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
				lvi.iItem++;
				lvi.state = LVIS_FOCUSED | LVIS_SELECTED;
				lvi.stateMask = lvi.state;
				lvi.pszText = const_cast<LPTSTR>(pTool->GetName());
				ListView_InsertItem(hwndList, &lvi);
				lvi.mask = LVIF_TEXT;
				lvi.iSubItem = 1;
				lvi.pszText = const_cast<LPTSTR>(pTool->GetCommand());
				ListView_SetItem(hwndList, &lvi);
				ListView_EnsureVisible(hwndList, lvi.iItem, FALSE);
			}
			return TRUE;

		case IDC_PROGRAMGUIDETOOL_REMOVE:
			{
				HWND hwndList = GetDlgItem(hDlg, IDC_PROGRAMGUIDETOOL_LIST);
				LV_ITEM lvi;

				lvi.mask = LVIF_PARAM;
				lvi.iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
				lvi.iSubItem = 0;
				if (ListView_GetItem(hwndList, &lvi)) {
					CProgramGuideTool *pTool;

					ListView_DeleteItem(hwndList, lvi.iItem);
					pTool = reinterpret_cast<CProgramGuideTool*>(lvi.lParam);
					delete pTool;
				}
			}
			return TRUE;

		case IDC_PROGRAMGUIDETOOL_REMOVEALL:
			{
				HWND hwndList = GetDlgItem(hDlg, IDC_PROGRAMGUIDETOOL_LIST);

				DeleteAllTools();
				ListView_DeleteAllItems(hwndList);
				SetDlgItemState();
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case LVN_ITEMCHANGED:
			SetDlgItemState();
			return TRUE;

		case NM_RCLICK:
			{
				LPNMITEMACTIVATE pnmia = (LPNMITEMACTIVATE)lParam;
				HWND hwndList = GetDlgItem(hDlg, IDC_PROGRAMGUIDETOOL_LIST);

				if (pnmia->hdr.hwndFrom == hwndList
						&& ListView_GetNextItem(hwndList, -1, LVNI_SELECTED) >= 0) {
					static const int MenuIDs[] = {
						IDC_PROGRAMGUIDETOOL_EDIT,
						0,
						IDC_PROGRAMGUIDETOOL_UP,
						IDC_PROGRAMGUIDETOOL_DOWN,
						0,
						IDC_PROGRAMGUIDETOOL_REMOVE
					};

					PopupMenuFromControls(hDlg, MenuIDs, lengthof(MenuIDs), TPM_RIGHTBUTTON);
				}
			}
			return TRUE;

		case PSN_APPLY:
			{
				int Value;
				bool fUpdate = false;

				m_fOnScreen =
					DlgCheckBox_IsChecked(hDlg, IDC_PROGRAMGUIDEOPTIONS_ONSCREEN);
				m_fScrollToCurChannel =
					DlgCheckBox_IsChecked(hDlg, IDC_PROGRAMGUIDEOPTIONS_SCROLLTOCURCHANNEL);
				Value = (int)DlgComboBox_GetCurSel(hDlg, IDC_PROGRAMGUIDEOPTIONS_BEGINHOUR) - 1;
				if (m_BeginHour != Value) {
					m_BeginHour = Value;
					m_pProgramGuide->SetBeginHour(Value);
					fUpdate = true;
				}
				Value = ::GetDlgItemInt(hDlg, IDC_PROGRAMGUIDEOPTIONS_VIEWHOURS, nullptr, TRUE);
				Value = std::clamp(Value, MIN_VIEW_HOURS, MAX_VIEW_HOURS);
				if (m_ViewHours != Value) {
					LibISDB::DateTime First, Last;

					m_ViewHours = Value;
					m_pProgramGuide->GetTimeRange(&First, nullptr);
					Last = First;
					Last.OffsetHours(m_ViewHours);
					m_pProgramGuide->SetTimeRange(First, Last);
					fUpdate = true;
				}
				if (fUpdate)
					m_pProgramGuide->UpdateProgramGuide();
				Value = ::GetDlgItemInt(hDlg, IDC_PROGRAMGUIDEOPTIONS_CHANNELWIDTH, nullptr, TRUE);
				m_ItemWidth = std::clamp(Value, CProgramGuide::MIN_ITEM_WIDTH, CProgramGuide::MAX_ITEM_WIDTH);
				Value = ::GetDlgItemInt(hDlg, IDC_PROGRAMGUIDEOPTIONS_LINESPERHOUR, nullptr, TRUE);
				m_LinesPerHour = std::clamp(Value, CProgramGuide::MIN_LINES_PER_HOUR, CProgramGuide::MAX_LINES_PER_HOUR);
				m_pProgramGuide->SetUIOptions(m_LinesPerHour, m_ItemWidth);

				if (m_Font != m_CurSettingFont) {
					m_Font = m_CurSettingFont;
					m_pProgramGuide->SetFont(m_Font);
				}

				bool fUseDirectWrite =
					DlgCheckBox_IsChecked(hDlg, IDC_PROGRAMGUIDEOPTIONS_USEDIRECTWRITE);
				if (m_fUseDirectWrite != fUseDirectWrite) {
					m_fUseDirectWrite = fUseDirectWrite;
					m_pProgramGuide->SetTextDrawEngine(
						m_fUseDirectWrite ?
							CTextDrawClient::TextDrawEngine::DirectWrite :
							CTextDrawClient::TextDrawEngine::GDI);
				}

				UINT VisibleEventIcons = 0;
				for (int i = 0; i <= CEpgIcons::ICON_LAST; i++) {
					if (DlgCheckBox_IsChecked(hDlg, IDC_PROGRAMGUIDEOPTIONS_ICON_FIRST + i))
						VisibleEventIcons |= CEpgIcons::IconFlag(i);
				}
				if (m_VisibleEventIcons != VisibleEventIcons) {
					m_VisibleEventIcons = VisibleEventIcons;
					m_pProgramGuide->SetVisibleEventIcons(VisibleEventIcons);
					m_UpdateFlags |= UPDATE_EVENTICONS;
				}

				m_WheelScrollLines = ::GetDlgItemInt(hDlg, IDC_PROGRAMGUIDEOPTIONS_WHEELSCROLLLINES, nullptr, TRUE);
				m_pProgramGuide->SetWheelScrollLines(m_WheelScrollLines);

				LRESULT Sel = DlgComboBox_GetCurSel(hDlg, IDC_PROGRAMGUIDEOPTIONS_PROGRAMLDOUBLECLICK);
				if (Sel >= 0) {
					if (Sel == 0) {
						m_ProgramLDoubleClickCommand.clear();
					} else if (Sel == 1) {
						m_ProgramLDoubleClickCommand = IEPG_ASSOCIATE_COMMAND;
					} else {
						m_ProgramLDoubleClickCommand =
							reinterpret_cast<LPCTSTR>(
								DlgComboBox_GetItemData(hDlg, IDC_PROGRAMGUIDEOPTIONS_PROGRAMLDOUBLECLICK, Sel));
					}
				}

				CProgramGuideToolList *pToolList = m_pProgramGuide->GetToolList();
				HWND hwndList = GetDlgItem(hDlg, IDC_PROGRAMGUIDETOOL_LIST);
				int Items, i;

				pToolList->Clear();
				Items = ListView_GetItemCount(hwndList);
				if (Items > 0) {
					LV_ITEM lvi;

					lvi.mask = LVIF_PARAM;
					lvi.iSubItem = 0;
					for (i = 0; i < Items; i++) {
						lvi.iItem = i;
						ListView_GetItem(hwndList, &lvi);
						pToolList->Add(reinterpret_cast<CProgramGuideTool*>(lvi.lParam));
					}
				}

				m_fChanged = true;
			}
			break;

		case PSN_RESET:
			DeleteAllTools();
			break;
		}
		break;

	case WM_DESTROY:
		{
			for (LRESULT i = DlgComboBox_GetCount(hDlg, IDC_PROGRAMGUIDEOPTIONS_PROGRAMLDOUBLECLICK) - 1; i > 1; i--) {
				delete [] reinterpret_cast<LPTSTR>(
					DlgComboBox_GetItemData(hDlg, IDC_PROGRAMGUIDEOPTIONS_PROGRAMLDOUBLECLICK, i));
			}

			for (int i = 0; i <= CEpgIcons::ICON_LAST; i++) {
				HBITMAP hbm = reinterpret_cast<HBITMAP>(
					::SendDlgItemMessage(
						hDlg, IDC_PROGRAMGUIDEOPTIONS_ICON_FIRST + i,
						BM_SETIMAGE, IMAGE_BITMAP,
						reinterpret_cast<LPARAM>((HBITMAP)nullptr)));
				if (hbm != nullptr)
					::DeleteObject(hbm);
			}

			m_Tooltip.Destroy();
		}
		return TRUE;
	}

	return FALSE;
}


void CProgramGuideOptions::RealizeStyle()
{
	CBasicDialog::RealizeStyle();
	m_Tooltip.SetFont(CBasicDialog::m_Font.GetHandle());
}


void CProgramGuideOptions::SetDlgItemState()
{
	HWND hwndList = ::GetDlgItem(m_hDlg, IDC_PROGRAMGUIDETOOL_LIST);
	int Items, Sel;

	Items = ListView_GetItemCount(hwndList);
	Sel = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
	EnableDlgItem(m_hDlg, IDC_PROGRAMGUIDETOOL_EDIT, Sel >= 0);
	EnableDlgItem(m_hDlg, IDC_PROGRAMGUIDETOOL_UP, Sel > 0);
	EnableDlgItem(m_hDlg, IDC_PROGRAMGUIDETOOL_DOWN, Sel >= 0 && Sel + 1 < Items);
	EnableDlgItem(m_hDlg, IDC_PROGRAMGUIDETOOL_REMOVE, Sel >= 0);
	EnableDlgItem(m_hDlg, IDC_PROGRAMGUIDETOOL_REMOVEALL, Items > 0);
}


void CProgramGuideOptions::DeleteAllTools()
{
	HWND hwndList = ::GetDlgItem(m_hDlg, IDC_PROGRAMGUIDETOOL_LIST);
	int Items;

	Items = ListView_GetItemCount(hwndList);
	if (Items > 0) {
		LV_ITEM lvi;
		CProgramGuideTool *pTool;

		lvi.mask = LVIF_PARAM;
		lvi.iSubItem = 0;
		for (int i = Items - 1; i >= 0; i--) {
			lvi.iItem = i;
			ListView_GetItem(hwndList, &lvi);
			pTool = reinterpret_cast<CProgramGuideTool*>(lvi.lParam);
			delete pTool;
		}
	}
}


}	// namespace TVTest
