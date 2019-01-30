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


#ifndef TVTEST_STATUS_OPTIONS_H
#define TVTEST_STATUS_OPTIONS_H


#include <vector>
#include "Options.h"
#include "StatusItems.h"
#include "WindowUtil.h"


namespace TVTest
{

	class CStatusOptions
		: public COptions
	{
	public:
		static const int OPACITY_MIN = 20;
		static const int OPACITY_MAX = 100;

		CStatusOptions(CStatusView *pStatusView);
		~CStatusOptions();

	// CSettingsBase
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	// CBasicDialog
		bool Create(HWND hwndOwner) override;

	// CStatusOptions
		bool ApplyOptions();
		bool ApplyItemList();
		void ApplyItemWidth();
		int RegisterItem(LPCTSTR pszID);
		bool SetItemVisibility(int ID, bool fVisible);
		const Style::Font &GetFont() const { return m_ItemFont; }
		bool GetShowPopup() const { return m_fShowPopup; }
		int GetPopupOpacity() const { return m_PopupOpacity; }
		bool GetShowTOTTime() const { return m_fShowTOTTime; }
		void SetShowTOTTime(bool fShow) { m_fShowTOTTime = fShow; }
		bool GetInterpolateTOTTime() const { return m_fInterpolateTOTTime; }
		void SetInterpolateTOTTime(bool fInterpolate) { m_fInterpolateTOTTime = fInterpolate; }
		bool IsPopupProgramInfoEnabled() const { return m_fEnablePopupProgramInfo; }
		void EnablePopupProgramInfo(bool fEnable) { m_fEnablePopupProgramInfo = fEnable; }
		bool GetShowEventProgress() const { return m_fShowEventProgress; }
		void SetShowEventProgress(bool fShow) { m_fShowEventProgress = fShow; }
		int GetPopupEventInfoWidth() const { return m_PopupEventInfoWidth; }
		int GetPopupEventInfoHeight() const { return m_PopupEventInfoHeight; }
		void SetPopupEventInfoSize(int Width, int Height) { m_PopupEventInfoWidth = Width; m_PopupEventInfoHeight = Height; }

	private:
		struct StatusItemInfo
		{
			int ID;
			String IDText;
			bool fVisible;
			int Width;
		};

		typedef std::vector<StatusItemInfo> StatusItemInfoList;

		class CItemListSubclass
			: public CWindowSubclass
		{
		public:
			CItemListSubclass(CStatusOptions *pStatusOptions);

		private:
			LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

			CStatusOptions *m_pStatusOptions;
		};

		CStatusView *m_pStatusView;
		StatusItemInfoList m_AvailItemList;
		StatusItemInfoList m_ItemList;
		StatusItemInfoList m_ItemListCur;
		int m_ItemID;
		int m_DPI;
		int m_StatusBarDPI;
		Style::Font m_ItemFont;
		bool m_fMultiRow;
		int m_MaxRows;
		bool m_fShowPopup;
		int m_PopupOpacity;

		Style::Font m_CurSettingFont;
		CItemListSubclass m_ItemListSubclass;
		int m_ItemHeight;
		int m_TextWidth;
		Style::Margins m_ItemMargin;
		Style::Size m_CheckSize;
		int m_DropInsertPos;
		UINT m_DragTimerID;
		bool m_fDragResize;

		bool m_fShowTOTTime;
		bool m_fInterpolateTOTTime;
		bool m_fEnablePopupProgramInfo;
		bool m_fShowEventProgress;
		int m_PopupEventInfoWidth;
		int m_PopupEventInfoHeight;

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void ApplyStyle() override;
		void RealizeStyle() override;

		void InitListBox();
		void CalcTextWidth();
		void SetListHExtent();
		void DrawInsertMark(HWND hwndList, int Pos);
		bool GetItemPreviewRect(HWND hwndList, int Index, RECT *pRect);
		bool IsCursorResize(HWND hwndList, int x, int y);
		void MakeItemList(StatusItemInfoList *pList) const;
	};

}	// namespace TVTest


#endif
