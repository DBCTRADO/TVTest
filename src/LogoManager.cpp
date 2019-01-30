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
#include "TVTest.h"
#include "AppMain.h"
#include "LogoManager.h"
#include "LibISDB/LibISDB/Base/FileStream.hpp"
#include "LibISDB/LibISDB/Utilities/CRC.hpp"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace
{


constexpr std::size_t MAX_LOGO_BYTES = 1296;

const char PNG_SIGNATURE[] = "\x89PNG\r\n\x1A\n";
constexpr std::size_t PNG_SIGNATURE_BYTES = 8;


#include <pshpack1.h>

/*
	ロゴファイルのフォーマット
	┌──────────┐
	│LogoFileHeader      │
	├──────────┤
	│┌────────┐│
	││LogoImageHeader ││
	│├────────┤│
	││PNGデータ       ││
	│├────────┤│
	││CRC32           ││
	│└────────┘│
	│...                 │
	└──────────┘
*/

struct LogoFileHeader
{
	char Type[8];
	DWORD Version;
	DWORD NumImages;
};

const char LOGOFILEHEADER_TYPE[] = "LogoData";
constexpr DWORD LOGOFILEHEADER_VERSION = 1;

struct LogoImageHeader
{
	WORD NetworkID;
	WORD LogoID;
	WORD LogoVersion;
	BYTE LogoType;
	BYTE Reserved;
	WORD DataSize;
};

struct LogoImageHeader2
{
	WORD NetworkID;
	WORD LogoID;
	WORD LogoVersion;
	BYTE LogoType;
	BYTE Reserved1;
	WORD DataSize;
	BYTE Reserved2[6];
	ULONGLONG Time;
};

#include <poppack.h>


static int CompareLogoVersion(WORD Version1, WORD Version2)
{
	if (Version1 == Version2)
		return 0;
	if ((Version1 <= 2047 && Version2 <= 2047) || (Version1 >= 2048 && Version2 >= 2048))
		return Version1 < Version2 ? -1 : 1;
	if (Version1 <= 2047)
		return 1;
	return -1;
}


}




CLogoManager::CLogoManager()
	: m_LogoDirectory(TEXT(".\\Logo"))
	, m_fSaveLogo(false)
	, m_fSaveBmp(false)
	, m_fLogoUpdated(false)
	, m_fLogoIDMapUpdated(false)
	, m_LogoFileLastWriteTime()
	, m_LogoIDMapFileLastWriteTime()
{
}


void CLogoManager::Clear()
{
	m_LogoMap.clear();
}


bool CLogoManager::SetLogoDirectory(LPCTSTR pszDirectory)
{
	BlockLock Lock(m_Lock);

	m_LogoDirectory = pszDirectory;
	return true;
}


bool CLogoManager::SetSaveLogo(bool fSave)
{
	BlockLock Lock(m_Lock);

	m_fSaveLogo = fSave;
	return true;
}


bool CLogoManager::SetSaveLogoBmp(bool fSave)
{
	BlockLock Lock(m_Lock);

	m_fSaveBmp = fSave;
	return true;
}


bool CLogoManager::AssociateLogoID(WORD NetworkID, WORD ServiceID, WORD LogoID)
{
	BlockLock Lock(m_Lock);

	SetLogoIDMap(NetworkID, ServiceID, LogoID);
	return true;
}


bool CLogoManager::SaveLogoFile(LPCTSTR pszFileName)
{
	BlockLock Lock(m_Lock);

	if (m_LogoMap.empty())
		return true;

	// ファイルが読み込んだ時から更新されている場合読み込み直す
	// (複数起動して他のプロセスが更新した可能性があるため)
	if (m_LogoFileLastWriteTime.dwLowDateTime != 0
			|| m_LogoFileLastWriteTime.dwHighDateTime != 0) {
		WIN32_FIND_DATA fd;
		HANDLE hFind = ::FindFirstFile(pszFileName, &fd);
		if (hFind != INVALID_HANDLE_VALUE) {
			::FindClose(hFind);
			if (::CompareFileTime(&fd.ftLastWriteTime, &m_LogoFileLastWriteTime) > 0) {
				TRACE(TEXT("CLogoManager::SaveLogoFile() : Reload file\n"));
				LoadLogoFile(pszFileName);
			}
		}
	}

	LibISDB::FileStream File;

	if (!File.Open(
				pszFileName,
				LibISDB::FileStream::OpenFlag::Write |
				LibISDB::FileStream::OpenFlag::Create |
				LibISDB::FileStream::OpenFlag::Truncate))
		return false;

	LogoFileHeader FileHeader;
	::CopyMemory(FileHeader.Type, LOGOFILEHEADER_TYPE, 8);
	FileHeader.Version = LOGOFILEHEADER_VERSION;
	FileHeader.NumImages = (DWORD)m_LogoMap.size();
	if (File.Write(&FileHeader, sizeof(FileHeader)) != sizeof(FileHeader))
		goto OnError;

	for (const auto &e : m_LogoMap) {
		LogoImageHeader2 ImageHeader = {};

		ImageHeader.NetworkID = e.second->GetNetworkID();
		ImageHeader.LogoID = e.second->GetLogoID();
		ImageHeader.LogoVersion = e.second->GetLogoVersion();
		ImageHeader.LogoType = e.second->GetLogoType();
		ImageHeader.DataSize = e.second->GetDataSize();
		ImageHeader.Time = e.second->GetTime().GetLinearSeconds();
		DWORD CRC = LibISDB::CRC32MPEG2::Calc((const uint8_t*)&ImageHeader, sizeof(ImageHeader));
		CRC = LibISDB::CRC32MPEG2::Calc(e.second->GetData(), ImageHeader.DataSize, CRC);
		if (File.Write(&ImageHeader, sizeof(ImageHeader)) != sizeof(ImageHeader)
				|| File.Write(e.second->GetData(), ImageHeader.DataSize) != ImageHeader.DataSize
				|| File.Write(&CRC, sizeof(CRC)) != sizeof(CRC))
			goto OnError;
	}

	return true;

OnError:
	File.Close();
	::DeleteFile(pszFileName);
	return false;
}


bool CLogoManager::LoadLogoFile(LPCTSTR pszFileName)
{
	BlockLock Lock(m_Lock);

	LibISDB::FileStream File;

	if (!File.Open(
				pszFileName,
				LibISDB::FileStream::OpenFlag::Read |
				LibISDB::FileStream::OpenFlag::ShareRead |
				LibISDB::FileStream::OpenFlag::SequentialRead)) {
		TRACE(TEXT("CLogoManager::LoadLogoFile() : File open error \"%s\"\n"), pszFileName);
		return false;
	}

	LogoFileHeader FileHeader;
	if (File.Read(&FileHeader, sizeof(FileHeader)) != sizeof(FileHeader)
			|| std::memcmp(FileHeader.Type, LOGOFILEHEADER_TYPE, 8) != 0
			|| FileHeader.Version > LOGOFILEHEADER_VERSION
			|| FileHeader.NumImages == 0) {
		TRACE(TEXT("CLogoManager::LoadLogoFile() : File header error\n"));
		return false;
	}

	const DWORD ImageHeaderSize =
		FileHeader.Version == 0 ? sizeof(LogoImageHeader) : sizeof(LogoImageHeader2);
	BYTE Buffer[MAX_LOGO_BYTES];

	for (DWORD i = 0; i < FileHeader.NumImages; i++) {
		LogoImageHeader2 ImageHeader;

		if (File.Read(&ImageHeader, ImageHeaderSize) != ImageHeaderSize
				|| ImageHeader.LogoType > 0x05
				|| ImageHeader.DataSize <= PNG_SIGNATURE_BYTES
				|| ImageHeader.DataSize > MAX_LOGO_BYTES) {
			TRACE(TEXT("CLogoManager::LoadLogoFile() : Image header error\n"));
			return false;
		}

		DWORD CRC;
		if (File.Read(Buffer, ImageHeader.DataSize) != ImageHeader.DataSize
				|| std::memcmp(Buffer, PNG_SIGNATURE, PNG_SIGNATURE_BYTES) != 0
				|| File.Read(&CRC, sizeof(CRC)) != sizeof(CRC)
				|| CRC != LibISDB::CRC32MPEG2::Calc(
						Buffer, ImageHeader.DataSize,
						LibISDB::CRC32MPEG2::Calc((const uint8_t*)&ImageHeader, ImageHeaderSize))) {
			return false;
		}

		LibISDB::LogoDownloaderFilter::LogoData Data;
		Data.NetworkID = ImageHeader.NetworkID;
		Data.LogoID = ImageHeader.LogoID;
		Data.LogoVersion = ImageHeader.LogoVersion;
		Data.LogoType = ImageHeader.LogoType;
		Data.DataSize = ImageHeader.DataSize;
		Data.pData = Buffer;
		if (FileHeader.Version != 0 && ImageHeader.Time != 0) {
			// 以前のバージョンには時刻情報が壊れる不具合があったため、
			// 値が正常な範囲かチェックする
			// (2000年1月1日から2099年12月31日までを有効とみなしている)
			if (ImageHeader.Time >= 12591158400ULL && ImageHeader.Time < 15746918400ULL)
				Data.Time.FromLinearSeconds(ImageHeader.Time);
		}

		ULONGLONG Key = GetMapKey(Data.NetworkID, Data.LogoID, Data.LogoType);
		LogoMap::iterator itr = m_LogoMap.find(Key);
		if (itr != m_LogoMap.end()) {
			if (CompareLogoVersion(itr->second->GetLogoVersion(), Data.LogoVersion) < 0) {
				itr->second = std::make_unique<CLogoData>(&Data);
			}
		} else {
			m_LogoMap.emplace(Key, std::make_unique<CLogoData>(&Data));
		}
	}

	File.GetTime(nullptr, nullptr, &m_LogoFileLastWriteTime);

	return true;
}


bool CLogoManager::SaveLogoIDMap(LPCTSTR pszFileName)
{
	BlockLock Lock(m_Lock);

	if (m_LogoIDMap.empty())
		return true;

	// ファイルが読み込んだ時から更新されている場合読み込み直す
	// (複数起動して他のプロセスが更新した可能性があるため)
	if (m_LogoIDMapFileLastWriteTime.dwLowDateTime != 0
			|| m_LogoIDMapFileLastWriteTime.dwHighDateTime != 0) {
		WIN32_FIND_DATA fd;
		HANDLE hFind = ::FindFirstFile(pszFileName, &fd);
		if (hFind != INVALID_HANDLE_VALUE) {
			::FindClose(hFind);
			if (::CompareFileTime(&fd.ftLastWriteTime, &m_LogoIDMapFileLastWriteTime) > 0) {
				TRACE(TEXT("CLogoManager::SaveLogoIDMap() : Reload file\n"));
				LoadLogoIDMap(pszFileName);
			}
		}
	}

	for (const auto &e : m_LogoIDMap) {
		TCHAR szKey[16], szText[16];

		StringPrintf(szKey, TEXT("%08lX"), e.first);
		StringPrintf(szText, TEXT("%d"), e.second);
		::WritePrivateProfileString(TEXT("LogoIDMap"), szKey, szText, pszFileName);
	}
	return true;
}


bool CLogoManager::LoadLogoIDMap(LPCTSTR pszFileName)
{
	BlockLock Lock(m_Lock);

	m_fLogoIDMapUpdated = false;

	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(pszFileName, &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		::FindClose(hFind);
		m_LogoIDMapFileLastWriteTime = fd.ftLastWriteTime;
	}

	LPTSTR pszSection = new TCHAR[32767];
	if (::GetPrivateProfileSection(TEXT("LogoIDMap"), pszSection, 32767, pszFileName) > 2) {
		for (LPTSTR p = pszSection; *p != _T('\0'); p += ::lstrlen(p) + 1) {
			int i;
			DWORD Key = 0;
			for (i = 0; i < 8; i++) {
				Key <<= 4;
				if (p[i] >= _T('0') && p[i] <= _T('9'))
					Key |= p[i] - _T('0');
				else if (p[i] >= _T('a') && p[i] <= _T('z'))
					Key |= p[i] - _T('a') + 10;
				else if (p[i] >= _T('A') && p[i] <= _T('Z'))
					Key |= p[i] - _T('A') + 10;
				else
					break;
			}
			if (i == 8 && p[i] == _T('=')) {
				m_LogoIDMap.emplace(Key, (WORD)::StrToInt(&p[i + 1]));
			}
		}
	}
	delete [] pszSection;

	return true;
}


HBITMAP CLogoManager::GetLogoBitmap(WORD NetworkID, WORD LogoID, BYTE LogoType)
{
	BlockLock Lock(m_Lock);

	ULONGLONG Key;
	LogoMap::iterator itr;
	if (LogoType == LOGOTYPE_SMALL || LogoType == LOGOTYPE_BIG) {
		static const BYTE SmallLogoPriority[] = {2, 0, 1, 5, 3, 4};
		static const BYTE BigLogoPriority[] = {5, 3, 4, 2, 0, 1};
		const BYTE *pPriority = LogoType == LOGOTYPE_SMALL ? SmallLogoPriority : BigLogoPriority;
		for (BYTE i = 0; i <= 5; i++) {
			Key = GetMapKey(NetworkID, LogoID, pPriority[i]);
			itr = m_LogoMap.find(Key);
			if (itr != m_LogoMap.end())
				break;
		}
		if (itr == m_LogoMap.end())
			LogoType = pPriority[0];
	} else {
		Key = GetMapKey(NetworkID, LogoID, LogoType);
		itr = m_LogoMap.find(Key);
	}
	if (itr == m_LogoMap.end()) {
		CLogoData *pLogoData = LoadLogoData(NetworkID, LogoID, LogoType);
		if (pLogoData != nullptr) {
			m_LogoMap.emplace(Key, pLogoData);
			m_fLogoUpdated = true;
			return pLogoData->GetBitmap(&m_ImageCodec);
		}
		return nullptr;
	}
	return itr->second->GetBitmap(&m_ImageCodec);
}


HBITMAP CLogoManager::GetAssociatedLogoBitmap(WORD NetworkID, WORD ServiceID, BYTE LogoType)
{
	BlockLock Lock(m_Lock);
	LogoIDMap::iterator itr = m_LogoIDMap.find(GetIDMapKey(NetworkID, ServiceID));
	if (itr == m_LogoIDMap.end())
		return nullptr;
	return GetLogoBitmap(NetworkID, itr->second, LogoType);
}


const Graphics::CImage *CLogoManager::GetLogoImage(WORD NetworkID, WORD LogoID, BYTE LogoType)
{
	BlockLock Lock(m_Lock);
	ULONGLONG Key = GetMapKey(NetworkID, LogoID, LogoType);
	LogoMap::iterator itr = m_LogoMap.find(Key);
	if (itr == m_LogoMap.end())
		return nullptr;
	return itr->second->GetImage(&m_ImageCodec);
}


const Graphics::CImage *CLogoManager::GetAssociatedLogoImage(WORD NetworkID, WORD ServiceID, BYTE LogoType)
{
	BlockLock Lock(m_Lock);
	LogoIDMap::iterator itr = m_LogoIDMap.find(GetIDMapKey(NetworkID, ServiceID));
	if (itr == m_LogoIDMap.end())
		return nullptr;
	return GetLogoImage(NetworkID, itr->second, LogoType);
}


HICON CLogoManager::CreateLogoIcon(WORD NetworkID, WORD ServiceID, int Width, int Height)
{
	if (Width < 1 || Height < 1)
		return nullptr;

	HBITMAP hbm = GetAssociatedLogoBitmap(
		NetworkID, ServiceID,
		(Width <= 36 && Height <= 24) ? LOGOTYPE_SMALL : LOGOTYPE_BIG);
	if (hbm == nullptr)
		return nullptr;

	// 本来の比率より縦長にしている(見栄えのため)
	int ImageWidth = Height * 16 / 10;
	int ImageHeight = Width * 10 / 16;
	return CreateIconFromBitmap(
		hbm, Width, Height,
		std::min(Width, ImageWidth),
		std::min(Height, ImageHeight));
}


bool CLogoManager::SaveLogoIcon(
	WORD NetworkID, WORD ServiceID, BYTE LogoType,
	int Width, int Height, LPCTSTR pszFileName)
{
	if (Width < 1 || Height < 1 || IsStringEmpty(pszFileName))
		return false;

	HBITMAP hbm = GetAssociatedLogoBitmap(NetworkID, ServiceID, LogoType);
	if (hbm == nullptr)
		return false;

	int ImageWidth = Height * 16 / 10;
	int ImageHeight = Width * 10 / 16;
	return SaveIconFromBitmap(
		pszFileName, hbm, Width, Height,
		std::min(Width, ImageWidth),
		std::min(Height, ImageHeight));
}


bool CLogoManager::IsLogoAvailable(WORD NetworkID, WORD ServiceID, BYTE LogoType) const
{
	BlockLock Lock(m_Lock);
	LogoMap::const_iterator itr = m_LogoMap.find(GetMapKey(NetworkID, ServiceID, LogoType));
	return itr != m_LogoMap.end();
}


DWORD CLogoManager::GetAvailableLogoType(WORD NetworkID, WORD ServiceID) const
{
	BlockLock Lock(m_Lock);

	LogoIDMap::const_iterator itrID = m_LogoIDMap.find(GetIDMapKey(NetworkID, ServiceID));
	if (itrID == m_LogoIDMap.end())
		return 0;
	const WORD LogoID = itrID->second;
	DWORD Flags = 0;
	for (BYTE i = 0; i <= 5; i++) {
		LogoMap::const_iterator itrLogo = m_LogoMap.find(GetMapKey(NetworkID, LogoID, i));
		if (itrLogo != m_LogoMap.end())
			Flags |= 1 << i;
	}
	return Flags;
}


bool CLogoManager::GetLogoInfo(WORD NetworkID, WORD ServiceID, BYTE LogoType, LogoInfo *pInfo) const
{
	if (pInfo == nullptr)
		return false;

	BlockLock Lock(m_Lock);

	auto itrID = m_LogoIDMap.find(GetIDMapKey(NetworkID, ServiceID));
	if (itrID == m_LogoIDMap.end())
		return false;
	auto itrLogo = m_LogoMap.find(GetMapKey(NetworkID, itrID->second, LogoType));
	if (itrLogo == m_LogoMap.end())
		return false;

	const CLogoData *pLogoData = itrLogo->second.get();

	pInfo->NetworkID = NetworkID;
	pInfo->LogoID = pLogoData->GetLogoID();
	pInfo->LogoVersion = pLogoData->GetLogoVersion();
	pInfo->LogoType = pLogoData->GetLogoType();
	LibISDB::DateTime Time;
	LibISDB::EPGTimeToUTCTime(pLogoData->GetTime(), &Time);
	SYSTEMTIME st = Time.ToSYSTEMTIME();
	::SystemTimeToFileTime(&st, &pInfo->UpdatedTime);

	return true;
}


void CLogoManager::OnLogoDownloaded(const LibISDB::LogoDownloaderFilter::LogoData &Data)
{
	// 透明なロゴは除外
	if (Data.DataSize <= 93)
		return;

	BlockLock Lock(m_Lock);

	const ULONGLONG Key = GetMapKey(Data.NetworkID, Data.LogoID, Data.LogoType);
	LogoMap::iterator itr = m_LogoMap.find(Key);
	bool fUpdated = false, fDataUpdated = false;
	CLogoData *pLogoData;
	if (itr != m_LogoMap.end()) {
		// バージョンが新しい場合のみ更新
		int VerCmp = CompareLogoVersion(itr->second->GetLogoVersion(), Data.LogoVersion);
		if (VerCmp < 0
				|| (VerCmp == 0 && itr->second->GetTime() < Data.Time)) {
			// BS/CSはバージョンが共通のため、データを比較して更新を確認する
			if (Data.DataSize != itr->second->GetDataSize()
					|| std::memcmp(Data.pData, itr->second->GetData(), Data.DataSize) != 0) {
				pLogoData = new CLogoData(&Data);
				itr->second.reset(pLogoData);
				fUpdated = true;
				fDataUpdated = true;
			} else if (VerCmp < 0) {
				itr->second->SetLogoVersion(Data.LogoVersion);
				fUpdated = true;
			}
		}
	} else {
		pLogoData = new CLogoData(&Data);
		m_LogoMap.emplace(Key, pLogoData);
		fUpdated = true;
		fDataUpdated = true;
	}

	if (fUpdated)
		m_fLogoUpdated = true;

	if (Data.ServiceList.size() > 0) {
		for (const auto &Service : Data.ServiceList) {
			SetLogoIDMap(Service.NetworkID, Service.ServiceID, Data.LogoID, fUpdated);
		}
	}

	if (fDataUpdated && (m_fSaveLogo || m_fSaveBmp)) {
		TCHAR szDirectory[MAX_PATH], szFileName[MAX_PATH];
		CFilePath FilePath;

		if (!GetAbsolutePath(m_LogoDirectory.c_str(), szDirectory, lengthof(szDirectory)))
			return;
		if (!::PathIsDirectory(szDirectory)) {
			// 一階層のみ自動的に作成
			if (!::CreateDirectory(szDirectory, nullptr)
					&& ::GetLastError() != ERROR_ALREADY_EXISTS)
				return;
		}
		if (m_fSaveLogo) {
			StringPrintf(
				szFileName, TEXT("%04X_%03X_%03X_%02X"),
				Data.NetworkID, Data.LogoID, Data.LogoVersion, Data.LogoType);
			FilePath = szDirectory;
			FilePath.Append(szFileName);
			if (!FilePath.IsFileExists())
				pLogoData->SaveToFile(FilePath.c_str());
		}
		if (m_fSaveBmp) {
			StringPrintf(
				szFileName, TEXT("%04X_%03X_%03X_%02X.bmp"),
				Data.NetworkID, Data.LogoID, Data.LogoVersion, Data.LogoType);
			FilePath = szDirectory;
			FilePath.Append(szFileName);
			if (!FilePath.IsFileExists())
				pLogoData->SaveBmpToFile(&m_ImageCodec, FilePath.c_str());
		}
	}
}


bool CLogoManager::SetLogoIDMap(WORD NetworkID, WORD ServiceID, WORD LogoID, bool fUpdate)
{
	const DWORD Key = GetIDMapKey(NetworkID, ServiceID);
	LogoIDMap::iterator i = m_LogoIDMap.find(Key);

	if (i == m_LogoIDMap.end()) {
		TRACE(
			TEXT("Logo ID mapped : NID %04x / SID %04x / Logo ID %04x\n"),
			NetworkID, ServiceID, LogoID);
		m_LogoIDMap.emplace(Key, LogoID);
		m_fLogoIDMapUpdated = true;
	} else if (fUpdate && i->second != LogoID) {
		TRACE(
			TEXT("Logo ID changed : NID %04x / SID %04x / Logo ID %04x -> %04x\n"),
			NetworkID, ServiceID, i->second, LogoID);
		i->second = LogoID;
		m_fLogoIDMapUpdated = true;
	} else {
		return false;
	}
	return true;
}


CLogoManager::CLogoData *CLogoManager::LoadLogoData(WORD NetworkID, WORD LogoID, BYTE LogoType)
{
	TCHAR szDirectory[MAX_PATH], szFileName[MAX_PATH], szMask[32];

	if (!GetAbsolutePath(m_LogoDirectory.c_str(), szDirectory, lengthof(szDirectory)))
		return false;
	// 最もバージョンが新しいロゴを探す
	StringPrintf(szMask, TEXT("%04X_%03X_\?\?\?_%02X"), NetworkID, LogoID, LogoType);
	if (::lstrlen(szDirectory) + 1 + ::lstrlen(szMask) >= lengthof(szFileName))
		return false;
	::PathCombine(szFileName, szDirectory, szMask);
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(szFileName, &fd);
	if (hFind == INVALID_HANDLE_VALUE)
		return nullptr;
	int NewerVersion = -1;
	do {
		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
			int Version;
			if (::StrToIntEx(&fd.cFileName[9], STIF_SUPPORT_HEX, &Version)) {
				if (Version > NewerVersion) {
					NewerVersion = Version;
					StringCopy(szFileName, fd.cFileName);
				}
			}
		}
	} while (::FindNextFile(hFind, &fd));
	::FindClose(hFind);
	if (NewerVersion < 0)
		return nullptr;

	// ロゴをファイルから読み込む
	::PathAppend(szDirectory, szFileName);
	HANDLE hFile = ::CreateFile(
		szDirectory, GENERIC_READ, FILE_SHARE_READ, nullptr,
		OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return nullptr;
	LARGE_INTEGER FileSize;
	if (!::GetFileSizeEx(hFile, &FileSize)
			|| FileSize.QuadPart < 64 || FileSize.QuadPart > 1024) {
		::CloseHandle(hFile);
		return nullptr;
	}
	BYTE *pData = new BYTE[FileSize.LowPart];
	DWORD Read;
	if (!::ReadFile(hFile, pData, FileSize.LowPart, &Read, nullptr)
			|| Read != FileSize.LowPart
			|| std::memcmp(pData, PNG_SIGNATURE, PNG_SIGNATURE_BYTES) != 0) {
		delete [] pData;
		::CloseHandle(hFile);
		return nullptr;
	}
	::CloseHandle(hFile);
	LibISDB::LogoDownloaderFilter::LogoData LogoData;
	LogoData.NetworkID = NetworkID;
	LogoData.LogoID = LogoID;
	LogoData.LogoVersion = (uint16_t)NewerVersion;
	LogoData.LogoType = LogoType;
	LogoData.DataSize = (uint16_t)Read;
	LogoData.pData = pData;
	CLogoData *pLogoData = new CLogoData(&LogoData);
	delete [] pData;
	return pLogoData;
}




CLogoManager::CLogoData::CLogoData(const LibISDB::LogoDownloaderFilter::LogoData *pData)
	: m_NetworkID(pData->NetworkID)
	, m_LogoID(pData->LogoID)
	, m_LogoVersion(pData->LogoVersion)
	, m_LogoType(pData->LogoType)
	, m_DataSize(pData->DataSize)
	, m_Time(pData->Time)
	, m_hbm(nullptr)
{
	m_Data = std::make_unique<BYTE[]>(pData->DataSize);
	::CopyMemory(m_Data.get(), pData->pData, pData->DataSize);
}


CLogoManager::CLogoData::CLogoData(const CLogoData &Src)
	: m_hbm(nullptr)
{
	*this = Src;
}


CLogoManager::CLogoData::~CLogoData()
{
	if (m_hbm != nullptr)
		::DeleteObject(m_hbm);
}


CLogoManager::CLogoData &CLogoManager::CLogoData::operator=(const CLogoData &Src)
{
	if (&Src != this) {
		if (m_hbm != nullptr) {
			::DeleteObject(m_hbm);
			m_hbm = nullptr;
		}
		m_Image.Free();
		m_NetworkID = Src.m_NetworkID;
		m_LogoID = Src.m_LogoID;
		m_LogoVersion = Src.m_LogoVersion;
		m_LogoType = Src.m_LogoType;
		m_DataSize = Src.m_DataSize;
		m_Data = std::make_unique<BYTE[]>(Src.m_DataSize);
		::CopyMemory(m_Data.get(), Src.m_Data.get(), Src.m_DataSize);
		m_Time = Src.m_Time;
	}
	return *this;
}


HBITMAP CLogoManager::CLogoData::GetBitmap(CImageCodec *pCodec)
{
	if (m_hbm == nullptr) {
		HGLOBAL hDIB = pCodec->LoadAribPngFromMemory(m_Data.get(), m_DataSize);
		if (hDIB == nullptr)
			return nullptr;
		BITMAPINFO *pbmi = (BITMAPINFO*)::GlobalLock(hDIB);
		void *pBits;
		m_hbm = ::CreateDIBSection(nullptr, pbmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
		if (m_hbm == nullptr)
			return nullptr;
		::CopyMemory(
			pBits, (BYTE*)pbmi + CalcDIBInfoSize(&pbmi->bmiHeader),
			CalcDIBBitsSize(&pbmi->bmiHeader));
		::GlobalUnlock(hDIB);
		::GlobalFree(hDIB);
	}
	return m_hbm;
}


const Graphics::CImage *CLogoManager::CLogoData::GetImage(CImageCodec *pCodec)
{
	if (!m_Image.IsCreated()) {
		HGLOBAL hDIB = pCodec->LoadAribPngFromMemory(m_Data.get(), m_DataSize);
		if (hDIB == nullptr)
			return nullptr;
		BITMAPINFO *pbmi = (BITMAPINFO*)::GlobalLock(hDIB);
		bool fResult = m_Image.CreateFromDIB(pbmi, (BYTE*)pbmi + CalcDIBInfoSize(&pbmi->bmiHeader));
		::GlobalUnlock(hDIB);
		::GlobalFree(hDIB);
		if (!fResult)
			return nullptr;
	}
	return &m_Image;
}


bool CLogoManager::CLogoData::SaveToFile(LPCTSTR pszFileName) const
{
	HANDLE hFile = ::CreateFile(
		pszFileName, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;
	DWORD Write;
	if (!WriteFile(hFile, m_Data.get(), m_DataSize, &Write, nullptr) || Write != m_DataSize) {
		::CloseHandle(hFile);
		return false;
	}
	::CloseHandle(hFile);
	return true;
}


bool CLogoManager::CLogoData::SaveBmpToFile(CImageCodec *pCodec, LPCTSTR pszFileName) const
{
	HGLOBAL hDIB = pCodec->LoadAribPngFromMemory(m_Data.get(), m_DataSize);
	if (hDIB == nullptr)
		return false;
	HANDLE hFile = ::CreateFile(pszFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;
	BITMAPINFOHEADER *pbmih = static_cast<BITMAPINFOHEADER*>(::GlobalLock(hDIB));
	DWORD InfoSize = (DWORD)CalcDIBInfoSize(pbmih), BitsSize = (DWORD)CalcDIBBitsSize(pbmih);
	DWORD Write;
	BITMAPFILEHEADER bmfh;
	bmfh.bfType = 0x4D42;
	bmfh.bfSize = sizeof(BITMAPFILEHEADER) + InfoSize + BitsSize;
	bmfh.bfReserved1 = 0;
	bmfh.bfReserved2 = 0;
	bmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + InfoSize;
	bool fResult =
		::WriteFile(hFile, &bmfh, sizeof(bmfh), &Write, nullptr) && Write == sizeof(bmfh)
		&& ::WriteFile(hFile, pbmih, InfoSize + BitsSize, &Write, nullptr) && Write == InfoSize + BitsSize;
	::CloseHandle(hFile);
	::GlobalUnlock(hDIB);
	::GlobalFree(hDIB);
	return fResult;
}


}	// namespace TVTest
