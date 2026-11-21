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
#include "Image.h"
#include "Util.h"
#include "Common/DebugDef.h"

#ifdef TVTEST_IMAGE_STATIC
#pragma comment(lib,"ImageLib.lib")
#pragma comment(lib,"libpng.lib")
#pragma comment(lib,"libjpeg.lib")
#pragma comment(lib,"zlib.lib")
#endif


namespace TVTest
{


size_t CalcDIBInfoSize(const BITMAPINFOHEADER *pbmih)
{
	size_t Size = pbmih->biSize;
	TVTEST_ASSERT(Size == sizeof (BITMAPINFOHEADER) || Size == sizeof(BITMAPV4HEADER) || Size == sizeof(BITMAPV5HEADER));

	if (pbmih->biBitCount <= 8)
		Size += (1_z << pbmih->biBitCount) * sizeof(RGBQUAD);
	else if (pbmih->biCompression == BI_BITFIELDS)
		Size += 3 * sizeof(DWORD);
	return Size;
}


size_t CalcDIBBitsSize(const BITMAPINFOHEADER *pbmih)
{
	return DIB_ROW_BYTES(pbmih->biWidth, pbmih->biBitCount) * std::abs(pbmih->biHeight);
}


size_t CalcDIBSize(const BITMAPINFOHEADER *pbmih)
{
	return CalcDIBInfoSize(pbmih) + CalcDIBBitsSize(pbmih);
}




CImageCodec::~CImageCodec()
{
#ifndef TVTEST_IMAGE_STATIC
	if (m_hLib != nullptr)
		::FreeLibrary(m_hLib);
#endif
}


bool CImageCodec::Init()
{
#ifndef TVTEST_IMAGE_STATIC
	if (m_hLib == nullptr) {
		TCHAR szPath[MAX_PATH];
		const DWORD Length = ::GetModuleFileName(nullptr, szPath, _countof(szPath));
		if (Length > 0 && Length < _countof(szPath)) {
			::PathRemoveFileSpec(szPath);
			if (::PathAppend(szPath, TEXT("TVTest_Image.dll")))
				m_hLib = ::LoadLibrary(szPath);
		}
		if (m_hLib == nullptr)
			return false;
		m_pSaveImage = reinterpret_cast<ImageDLL::SaveImageFunc>(::GetProcAddress(m_hLib, "SaveImage"));
		if (m_pSaveImage == nullptr) {
			::FreeLibrary(m_hLib);
			m_hLib = nullptr;
			return false;
		}
	}
#endif
	return true;
}


bool CImageCodec::SaveImage(
	LPCTSTR pszFileName, int Format, LPCTSTR pszOption,
	const BITMAPINFO *pbmi, const void *pBits, LPCTSTR pszComment)
{
#ifndef TVTEST_IMAGE_STATIC
	if (m_hLib == nullptr && !Init())
		return false;
#endif

	ImageLib::ImageSaveInfo Info;

	Info.pszFileName = pszFileName;
	Info.pszFormat = EnumSaveFormat(Format);
	Info.pszOption = pszOption;
	Info.pbmi = pbmi;
	Info.pBits = pBits;
	Info.pszComment = pszComment;

#ifndef TVTEST_IMAGE_STATIC
	return m_pSaveImage(&Info) != FALSE;
#else
	return ImageLib::SaveImage(&Info) != FALSE;
#endif
}


LPCTSTR CImageCodec::EnumSaveFormat(int Index) const
{
	switch (Index) {
	case 0: return TEXT("BMP");
	case 1: return TEXT("JPEG");
	case 2: return TEXT("PNG");
	}
	return nullptr;
}


LPCTSTR CImageCodec::GetExtension(int Index) const
{
	switch (Index) {
	case 0: return TEXT("bmp");
	case 1: return TEXT("jpg");
	case 2: return TEXT("png");
	}
	return nullptr;
}


int CImageCodec::FormatNameToIndex(LPCTSTR pszName) const
{
	LPCTSTR pszFormat;

	for (int i = 0; (pszFormat = EnumSaveFormat(i)) != nullptr; i++) {
		if (lstrcmpi(pszName, pszFormat) == 0)
			return i;
	}
	return -1;
}


HGLOBAL CImageCodec::LoadAribPngFromMemory(const void *pData, size_t DataSize)
{
#ifndef TVTEST_IMAGE_STATIC
	if (m_hLib == nullptr && !Init())
		return nullptr;
	const auto pLoadAribPngFromMemory =
		reinterpret_cast<ImageDLL::LoadAribPngFromMemoryFunc>(::GetProcAddress(m_hLib, "LoadAribPngFromMemory"));
	if (pLoadAribPngFromMemory == nullptr)
		return nullptr;
	return pLoadAribPngFromMemory(pData, DataSize);
#else
	return ImageLib::LoadAribPngFromMemory(pData, DataSize);
#endif
}


HGLOBAL CImageCodec::LoadAribPngFromFile(LPCTSTR pszFileName)
{
#ifndef TVTEST_IMAGE_STATIC
	if (m_hLib == nullptr && !Init())
		return nullptr;
	const auto pLoadAribPngFromFile =
		reinterpret_cast<ImageDLL::LoadAribPngFromFileFunc>(::GetProcAddress(m_hLib, "LoadAribPngFromFile"));
	if (pLoadAribPngFromFile == nullptr)
		return nullptr;
	return pLoadAribPngFromFile(pszFileName);
#else
	return ImageLib::LoadAribPngFromFile(pszFileName);
#endif
}


HGLOBAL CImageCodec::CropImage(const BITMAPINFO *pbmi, const void *pBits, const RECT *pRect, int BitsPerPixel)
{
#ifndef TVTEST_IMAGE_STATIC
	if (m_hLib == nullptr && !Init())
		return nullptr;
	const auto pCropImage =
		reinterpret_cast<ImageDLL::CropImageFunc>(::GetProcAddress(m_hLib, "CropImage"));
	if (pCropImage == nullptr)
		return nullptr;
	return pCropImage(pbmi, pBits, pRect, BitsPerPixel);
#else
	return ImageLib::CropImage(pbmi, pBits, pRect, BitsPerPixel);
#endif
}


HGLOBAL CImageCodec::ResizeImage(
	const BITMAPINFO *pbmi, const void *pBits, const RECT *pRect,
	int Width, int Height, int BitsPerPixel, ImageLib::ResizeInterpolation Interpolation)
{
#ifndef TVTEST_IMAGE_STATIC
	if (m_hLib == nullptr && !Init())
		return nullptr;
	const auto pResizeImage =
		reinterpret_cast<ImageDLL::ResizeImageFunc>(::GetProcAddress(m_hLib, "ResizeImage"));
	if (pResizeImage == nullptr)
		return nullptr;
	return pResizeImage(pbmi, pBits, pRect, Width, Height, BitsPerPixel, Interpolation);
#else
	return ImageLib::ResizeImage(pbmi, pBits, pRect, Width, Height, BitsPerPixel, Interpolation);
#endif
}


} // namespace TVTest
