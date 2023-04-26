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


#ifndef TVTEST_ZOOM_OPTIONS_H
#define TVTEST_ZOOM_OPTIONS_H


#include "Command.h"
#include "Settings.h"
#include "Dialog.h"
#include "ListView.h"


namespace TVTest
{

	class CZoomOptions
		: public CBasicDialog
		, public CSettingsBase
		, public CCommandManager::CCommandCustomizer
	{
	public:
		enum class ZoomType {
			Rate,
			Size,
			TVTEST_ENUM_CLASS_TRAILER
		};

		struct ZoomRate
		{
			int Rate;
			int Factor;

			int GetPercentage() const { return Factor != 0 ?::MulDiv(Rate, 100, Factor) : 0; }
		};

		struct ZoomSize
		{
			int Width;
			int Height;
		};

		struct ZoomInfo
		{
			ZoomType Type;
			ZoomRate Rate;
			ZoomSize Size;
			bool fVisible;
		};

		static constexpr int NUM_ZOOM_COMMANDS = 11 + 10;
		static constexpr int MAX_RATE = 1000;

		CZoomOptions();

	//CBasicDialog
		bool Show(HWND hwndOwner) override;

	//CSettingsBase
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	//CZoomOptions
		bool SetMenu(HMENU hmenu, const ZoomInfo *pCurZoom) const;
		bool GetZoomInfoByCommand(int Command, ZoomInfo *pInfo) const;

	private:
		struct ZoomCommandInfo
		{
			int Command;
			ZoomInfo Info;
		};
		static const ZoomCommandInfo m_DefaultZoomList[NUM_ZOOM_COMMANDS];

		ZoomInfo m_ZoomList[NUM_ZOOM_COMMANDS];

		int m_Order[NUM_ZOOM_COMMANDS];
		bool m_fChanging = false;
		ZoomInfo m_ZoomSettingList[NUM_ZOOM_COMMANDS];
		CListView m_ItemListView;

		int GetIndexByCommand(int Command) const;
		void FormatCommandText(int Command, const ZoomInfo &Info, LPTSTR pszText, size_t MaxLength) const;

		void SetItemState(HWND hDlg);
		int GetItemIndex(int Item);
		void UpdateItemText(int Item);

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	//CCommandCustomizer
		bool GetCommandText(int Command, LPTSTR pszText, size_t MaxLength) override;
	};

}	// namespace TVTest


#endif
