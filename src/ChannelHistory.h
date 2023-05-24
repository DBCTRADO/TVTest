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


#ifndef TVTEST_CHANNEL_HISTORY_H
#define TVTEST_CHANNEL_HISTORY_H


#include <deque>
#include <memory>
#include "ChannelList.h"
#include "Settings.h"
#include "Menu.h"


namespace TVTest
{

	class CChannelHistory
	{
	public:
		void Clear();
		bool SetCurrentChannel(LPCTSTR pszDriverName, const CChannelInfo *pChannelInfo);
		const CTunerChannelInfo *Forward();
		const CTunerChannelInfo *Backward();

	private:
		std::deque<std::unique_ptr<CTunerChannelInfo>> m_ChannelList;
		int m_MaxChannelHistory = 20;
		int m_CurrentChannel = -1;
	};

	class CRecentChannelList
		: public CSettingsBase
	{
	public:
		CRecentChannelList();

		int NumChannels() const;
		void Clear();
		const CTunerChannelInfo *GetChannelInfo(int Index) const;
		bool Add(const CTunerChannelInfo &ChannelInfo);
		bool Add(LPCTSTR pszDriverName, const CChannelInfo *pChannelInfo);

		bool SetMenu(HWND hwnd, HMENU hmenu);
		bool OnMeasureItem(HWND hwnd, WPARAM wParam, LPARAM lParam);
		bool OnDrawItem(HWND hwnd, WPARAM wParam, LPARAM lParam);
		bool OnMenuSelect(HWND hwnd, WPARAM wParam, LPARAM lParam);
		bool OnUninitMenuPopup(HWND hwnd, WPARAM wParam, LPARAM lParam);

	// CSettingsBase
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	private:
		std::deque<std::unique_ptr<CTunerChannelInfo>> m_ChannelList;
		int m_MaxChannelHistory = 20;
		int m_MaxChannelHistoryMenu = 20;
		CChannelMenu m_ChannelMenu;
	};

}	// namespace TVTest


#endif
