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


#ifndef TVTEST_TOOLTIP_H
#define TVTEST_TOOLTIP_H


namespace TVTest
{

	class CTooltip
	{
	protected:
		HWND m_hwndTooltip = nullptr;
		HWND m_hwndParent = nullptr;

	public:
		CTooltip() = default;
		~CTooltip();

		CTooltip(const CTooltip &) = delete;
		CTooltip &operator=(const CTooltip &) = delete;

		bool Create(HWND hwnd);
		void Destroy();
		bool IsCreated() const { return m_hwndTooltip != nullptr; }
		HWND GetHandle() const { return m_hwndTooltip; }
		void DeleteAllTools();
		bool Enable(bool fEnable);
		bool IsVisible() const;
		bool SetMaxWidth(int Width);
		bool SetPopDelay(int Delay);
		int NumTools() const;
		bool AddTool(UINT ID, const RECT &Rect, LPCTSTR pszText = LPSTR_TEXTCALLBACK, LPARAM lParam = 0);
		bool AddTool(HWND hwnd, LPCTSTR pszText = LPSTR_TEXTCALLBACK, LPARAM lParam = 0);
		bool DeleteTool(UINT ID);
		bool SetToolRect(UINT ID, const RECT &Rect);
		bool SetText(UINT ID, LPCTSTR pszText);
		bool AddTrackingTip(UINT ID, LPCTSTR pszText = LPSTR_TEXTCALLBACK, LPARAM lParam = 0);
		bool TrackActivate(UINT ID, bool fActivate);
		bool TrackPosition(int x, int y);
		bool SetFont(HFONT hfont);
	};

	class CBalloonTip
	{
		HWND m_hwndToolTips = nullptr;
		HWND m_hwndOwner = nullptr;

	public:
		CBalloonTip() = default;
		~CBalloonTip();

		CBalloonTip(const CBalloonTip &) = delete;
		CBalloonTip &operator=(const CBalloonTip &) = delete;

		bool Initialize(HWND hwnd);
		void Finalize();
		enum {
			ICON_NONE,
			ICON_INFO,
			ICON_WARNING,
			ICON_ERROR
		};
		bool Show(LPCTSTR pszText, LPCTSTR pszTitle, const POINT *pPos, int Icon = ICON_NONE);
		bool Hide();
	};

}	// namespace TVTest


#endif
