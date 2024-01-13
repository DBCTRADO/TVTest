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


#ifndef TVTEST_DRIVER_MANAGER_H
#define TVTEST_DRIVER_MANAGER_H


#include <vector>
#include <memory>
#include "ChannelList.h"


namespace TVTest
{

	class CDriverInfo
	{
	public:
		CDriverInfo(LPCTSTR pszFileName);

		LPCTSTR GetFileName() const { return m_FileName.c_str(); }
		LPCTSTR GetTunerName() const { return m_TunerName.c_str(); }
		enum class LoadTuningSpaceListMode {
			Default,
			NoLoadDriver,
			UseDriver,
			UseDriverNoOpen,
		};
		bool LoadTuningSpaceList(LoadTuningSpaceListMode Mode = LoadTuningSpaceListMode::Default);
		void ClearTuningSpaceList();
		bool IsChannelFileLoaded() const { return m_fChannelFileLoaded; }
		bool IsDriverChannelLoaded() const { return m_fDriverSpaceLoaded; }
		bool IsTuningSpaceListLoaded() const {
			return m_fChannelFileLoaded || m_fDriverSpaceLoaded;
		}
		const CTuningSpaceList *GetTuningSpaceList() const { return &m_TuningSpaceList; }
		const CTuningSpaceList *GetDriverSpaceList() const { return &m_DriverSpaceList; }
		const CTuningSpaceList *GetAvailableTuningSpaceList() const;
		const CChannelList *GetChannelList(int Space) const;
		int NumDriverSpaces() const { return m_DriverSpaceList.NumSpaces(); }

	private:
		String m_FileName;
		String m_TunerName;
		bool m_fChannelFileLoaded = false;
		CTuningSpaceList m_TuningSpaceList;
		bool m_fDriverSpaceLoaded = false;
		CTuningSpaceList m_DriverSpaceList;
	};

	class CDriverManager
	{
	public:
		struct TunerSpec
		{
			enum class Flag : unsigned int {
				None          = 0x0000U,
				Network       = 0x0001U,
				File          = 0x0002U,
				Virtual       = 0x0004U,
				Volatile      = 0x0008U,
				NoEnumChannel = 0x0010U,
				TVTEST_ENUM_FLAGS_TRAILER
			};

			Flag Flags;
		};

		void Clear();
		bool Find(LPCTSTR pszDirectory);
		LPCTSTR GetBaseDirectory() const { return m_BaseDirectory.c_str(); }
		int NumDrivers() const { return static_cast<int>(m_DriverList.size()); }
		CDriverInfo *GetDriverInfo(int Index);
		const CDriverInfo *GetDriverInfo(int Index) const;
		int FindByFileName(LPCTSTR pszFileName) const;
		bool GetAllServiceList(CChannelList *pList) const;

		bool LoadTunerSpec(LPCTSTR pszFileName);
		bool GetTunerSpec(LPCTSTR pszTunerName, TunerSpec *pSpec) const;

	private:
		struct TunerSpecInfo
		{
			String TunerMask;
			TunerSpec Spec;
		};

		std::vector<std::unique_ptr<CDriverInfo>> m_DriverList;
		String m_BaseDirectory;
		std::vector<TunerSpecInfo> m_TunerSpecList;
	};

} // namespace TVTest


#endif
