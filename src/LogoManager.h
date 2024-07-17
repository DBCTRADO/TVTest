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


#ifndef TVTEST_LOGO_MANAGER_H
#define TVTEST_LOGO_MANAGER_H


#include <map>
#include <memory>
#include "LibISDB/LibISDB/Filters/LogoDownloaderFilter.hpp"
#include "DrawUtil.h"
#include "Graphics.h"
#include "Image.h"


namespace TVTest
{

	class CLogoManager
		: public LibISDB::LogoDownloaderFilter::LogoHandler
	{
	public:
		struct LogoInfo
		{
			WORD NetworkID;
			WORD LogoID;
			WORD LogoVersion;
			BYTE LogoType;
			FILETIME UpdatedTime;
		};

		void Clear();
		bool SetLogoDirectory(LPCTSTR pszDirectory);
		LPCTSTR GetLogoDirectory() const { return m_LogoDirectory.c_str(); }
		bool SetSaveLogo(bool fSave);
		bool GetSaveLogo() const { return m_fSaveLogo; }
		bool SetSaveLogoBmp(bool fSave);
		bool GetSaveLogoBmp() const { return m_fSaveBmp; }
		void SetForceUpdate(bool fForce);
		bool GetForceUpdate() const { return m_fForceUpdate; }
		bool AssociateLogoID(WORD NetworkID, WORD ServiceID, WORD LogoID);
		bool SaveLogoFile(LPCTSTR pszFileName);
		bool LoadLogoFile(LPCTSTR pszFileName);
		bool IsLogoDataUpdated() const { return m_fLogoUpdated; }
		bool SaveLogoIDMap(LPCTSTR pszFileName);
		bool LoadLogoIDMap(LPCTSTR pszFileName);
		bool IsLogoIDMapUpdated() const { return m_fLogoIDMapUpdated; }
		HBITMAP GetLogoBitmap(WORD NetworkID, WORD LogoID, BYTE LogoType);
		HBITMAP GetAssociatedLogoBitmap(WORD NetworkID, WORD ServiceID, BYTE LogoType);
		const Graphics::CImage *GetLogoImage(WORD NetworkID, WORD LogoID, BYTE LogoType);
		const Graphics::CImage *GetAssociatedLogoImage(WORD NetworkID, WORD ServiceID, BYTE LogoType);
		HICON CreateLogoIcon(WORD NetworkID, WORD ServiceID, int Width, int Height);
		bool SaveLogoIcon(
			WORD NetworkID, WORD ServiceID, BYTE LogoType,
			int Width, int Height, LPCTSTR pszFileName);
		bool IsLogoAvailable(WORD NetworkID, WORD ServiceID, BYTE LogoType) const;
		DWORD GetAvailableLogoType(WORD NetworkID, WORD ServiceID) const;
		bool GetLogoInfo(WORD NetworkID, WORD ServiceID, BYTE LogoType, LogoInfo *pInfo) const;

		enum {
			LOGOTYPE_48x24,
			LOGOTYPE_36x24,
			LOGOTYPE_48x27,
			LOGOTYPE_72x36,
			LOGOTYPE_54x36,
			LOGOTYPE_64x36,
			LOGOTYPE_FIRST  = LOGOTYPE_48x24,
			LOGOTYPE_LAST   = LOGOTYPE_64x36,
			LOGOTYPE_SMALL  = 0xFF, // 取得できる中から小さいもの優先
			LOGOTYPE_BIG    = 0xFE  // 取得できる中から大きいもの優先
		};

	private:
		class CLogoData
		{
			WORD m_NetworkID;
			WORD m_LogoID;
			WORD m_LogoVersion;
			BYTE m_LogoType;
			WORD m_DataSize;
			std::unique_ptr<BYTE[]> m_Data;
			LibISDB::DateTime m_Time;
			DrawUtil::CBitmap m_Bitmap;
			Graphics::CImage m_Image; // m_Bitmap より先に破棄されるようにする必要がある

		public:
			CLogoData(const LibISDB::LogoDownloaderFilter::LogoData *pData);
			CLogoData(const CLogoData &Src);

			CLogoData &operator=(const CLogoData &Src);

			WORD GetNetworkID() const { return m_NetworkID; }
			WORD GetLogoID() const { return m_LogoID; }
			WORD GetLogoVersion() const { return m_LogoVersion; }
			void SetLogoVersion(WORD Version) { m_LogoVersion = Version; }
			BYTE GetLogoType() const { return m_LogoType; }
			WORD GetDataSize() const { return m_DataSize; }
			const BYTE *GetData() const { return m_Data.get(); }
			const LibISDB::DateTime &GetTime() const { return m_Time; }
			HBITMAP GetBitmap(CImageCodec *pCodec);
			const Graphics::CImage *GetImage(CImageCodec *pCodec);
			bool SaveToFile(LPCTSTR pszFileName) const;
			bool SaveBmpToFile(CImageCodec *pCodec, LPCTSTR pszFileName) const;
		};

		static inline ULONGLONG GetMapKey(WORD NID, WORD LogoID, BYTE LogoType) {
			return (static_cast<ULONGLONG>(NID) << 24) | (static_cast<ULONGLONG>(LogoID) << 8) | LogoType;
		}
		typedef std::map<ULONGLONG, std::unique_ptr<CLogoData>> LogoMap;
		static inline DWORD GetIDMapKey(WORD NID, WORD SID) {
			return (static_cast<DWORD>(NID) << 16) | static_cast<DWORD>(SID);
		}
		typedef std::map<DWORD, WORD> LogoIDMap;

		CFilePath m_LogoDirectory{TEXT(".\\Logo")};
		bool m_fSaveLogo = false;
		bool m_fSaveBmp = false;
		bool m_fForceUpdate = false;
		LogoMap m_LogoMap;
		LogoIDMap m_LogoIDMap;
		CImageCodec m_ImageCodec;
		mutable MutexLock m_Lock;
		bool m_fLogoUpdated = false;
		bool m_fLogoIDMapUpdated = false;
		FILETIME m_LogoFileLastWriteTime{};
		FILETIME m_LogoIDMapFileLastWriteTime{};

		bool SetLogoIDMap(WORD NetworkID, WORD ServiceID, WORD LogoID, bool fUpdate = true);
		CLogoData *FindLogoData(WORD NetworkID, WORD LogoID, BYTE LogoType);
		CLogoData *LoadLogoData(WORD NetworkID, WORD LogoID, BYTE LogoType);

	// LibISDB::LogoDownloaderFilter::LogoHandler
		void OnLogoDownloaded(const LibISDB::LogoDownloaderFilter::LogoData &Data) override;
	};

} // namespace TVTest


#endif
