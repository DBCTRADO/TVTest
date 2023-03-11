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


#include <windows.h>
#include <tchar.h>
#include <memory>
#include "ImageLib.h"
#include "Codec_BMP.h"
#include "Codec_JPEG.h"
#include "Codec_PNG.h"


namespace TVTest
{

namespace ImageLib
{


bool SaveImage(const ImageSaveInfo *pInfo)
{
	if (pInfo == nullptr)
		return false;

	bool fResult;

	if (::lstrcmpi(pInfo->pszFormat, TEXT("BMP")) == 0) {
		fResult = SaveBMPFile(pInfo);
	} else if (::lstrcmpi(pInfo->pszFormat, TEXT("JPEG")) == 0) {
		fResult = SaveJPEGFile(pInfo);
	} else if (::lstrcmpi(pInfo->pszFormat, TEXT("PNG")) == 0) {
		fResult = SavePNGFile(pInfo);
	} else {
		fResult = false;
	}

	return fResult;
}


HGLOBAL LoadAribPngFromMemory(const void *pData, size_t DataSize)
{
	return LoadAribPng(pData, DataSize);
}


HGLOBAL LoadAribPngFromFile(LPCTSTR pszFileName)
{
	const HANDLE hFile =::CreateFile(
		pszFileName, GENERIC_READ, FILE_SHARE_READ, nullptr,
		OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return nullptr;
	LARGE_INTEGER FileSize;
	if (!::GetFileSizeEx(hFile, &FileSize)) {
		::CloseHandle(hFile);
		return nullptr;
	}
	if (FileSize.QuadPart > 4 * 1024 * 1024) {
		::CloseHandle(hFile);
		return nullptr;
	}
	std::unique_ptr<BYTE[]> Data(new BYTE[FileSize.LowPart]);
	DWORD Read;
	if (!::ReadFile(hFile, Data.get(), FileSize.LowPart, &Read, nullptr) || Read != FileSize.LowPart) {
		::CloseHandle(hFile);
		return nullptr;
	}
	::CloseHandle(hFile);
	return LoadAribPng(Data.get(), FileSize.LowPart);
}


}	// namespace ImageLib

}	// namespace TVTest
