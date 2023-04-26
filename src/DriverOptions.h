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


#ifndef TVTEST_DRIVER_OPTIONS_H
#define TVTEST_DRIVER_OPTIONS_H


#include <vector>
#include <memory>
#include "Options.h"
#include "DriverManager.h"
#include "ChannelManager.h"


namespace TVTest
{

	class CDriverSettings;

	class CDriverSettingList
	{
		std::vector<std::unique_ptr<CDriverSettings>> m_SettingList;

	public:
		CDriverSettingList() = default;
		CDriverSettingList(const CDriverSettingList &Src);

		CDriverSettingList &operator=(const CDriverSettingList &Src);

		void Clear();
		size_t NumDrivers() const { return m_SettingList.size(); }
		bool Add(CDriverSettings *pSettings);
		CDriverSettings *GetDriverSettings(size_t Index);
		const CDriverSettings *GetDriverSettings(size_t Index) const;
		int Find(LPCTSTR pszFileName) const;
	};

	class CDriverOptions
		: public COptions
	{
	public:
		struct ChannelInfo
		{
			int Space;
			int Channel;
			int ServiceID;
			int TransportStreamID;
			bool fAllChannels;
		};

		struct BonDriverOptions
		{
			bool fNoSignalLevel = false;
			bool fIgnoreInitialStream = true;
			bool fPurgeStreamOnChannelChange = true;
			bool fResetChannelChangeErrorCount = true;
			bool fPumpStreamSyncPlayback = false;
			DWORD FirstChannelSetDelay = 0;
			DWORD MinChannelChangeInterval = 0;

			BonDriverOptions() = default;
			BonDriverOptions(LPCTSTR pszBonDriverName);
		};

		CDriverOptions();
		~CDriverOptions();

	// CSettingsBase
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	// CBasicDialog
		bool Create(HWND hwndOwner) override;

	// CDriverOptions
		bool Initialize(CDriverManager *pDriverManager);
		bool GetInitialChannel(LPCTSTR pszFileName, ChannelInfo *pChannelInfo) const;
		bool SetLastChannel(LPCTSTR pszFileName, const ChannelInfo *pChannelInfo);
		bool IsNoSignalLevel(LPCTSTR pszFileName) const;
		bool IsResetChannelChangeErrorCount(LPCTSTR pszFileName) const;
		bool GetBonDriverOptions(LPCTSTR pszFileName, BonDriverOptions *pOptions) const;

	private:
		CDriverManager *m_pDriverManager = nullptr;
		CDriverSettingList m_SettingList;
		CDriverSettingList m_CurSettingList;
		CChannelList m_InitChannelList;

		CDriverSettings *GetBonDriverSettings(LPCTSTR pszFileName);
		const CDriverSettings *GetBonDriverSettings(LPCTSTR pszFileName) const;

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		void InitDlgItem(int Driver);
		void SetChannelList(int Driver);
		void AddChannelList(const CChannelList *pChannelList);
		CDriverSettings *GetCurSelDriverSettings() const;
	};

}	// namespace TVTest


#endif
