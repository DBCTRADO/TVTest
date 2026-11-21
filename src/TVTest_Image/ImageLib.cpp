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
#include "ImageUtil.h"
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


HGLOBAL CropImage(const BITMAPINFO *pbmi, const void *pBits, const RECT *pRect, int BitsPerPixel)
{
	if (pbmi == nullptr || pBits == nullptr || pRect == nullptr
			|| (BitsPerPixel != 24 && BitsPerPixel != 32))
		return nullptr;

	const BITMAPINFOHEADER &bmih = pbmi->bmiHeader;

	if (bmih.biSize < sizeof(BITMAPINFOHEADER)
			|| (bmih.biBitCount != 24 && bmih.biBitCount != 32)
			|| bmih.biCompression != BI_RGB)
		return nullptr;

	if (pRect->left >= pRect->right || pRect->top >= pRect->bottom
			|| pRect->left < 0 || pRect->left >= bmih.biWidth
			|| pRect->top < 0 || pRect->top >= std::abs(bmih.biHeight)
			|| pRect->right > bmih.biWidth
			|| pRect->bottom > std::abs(bmih.biHeight))
		return nullptr;

	const int DstWidth = pRect->right - pRect->left;
	const int DstHeight = pRect->bottom - pRect->top;
	const std::size_t DstRowBytes = DIB_ROW_BYTES(DstWidth, BitsPerPixel);

	HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, sizeof(BITMAPINFOHEADER) + DstRowBytes * DstHeight);
	if (hGlobal == nullptr)
		return nullptr;
	BITMAPINFOHEADER *pbmihDst = static_cast<BITMAPINFOHEADER*>(::GlobalLock(hGlobal));
	if (pbmihDst == nullptr) {
		::GlobalFree(hGlobal);
		return nullptr;
	}

	std::memcpy(pbmihDst, pbmi, sizeof(BITMAPINFOHEADER));
	pbmihDst->biSize = sizeof(BITMAPINFOHEADER);
	pbmihDst->biWidth = DstWidth;
	pbmihDst->biHeight = bmih.biHeight > 0 ? DstHeight : -DstHeight;
	pbmihDst->biBitCount = BitsPerPixel;

	const std::size_t SrcRowBytes = DIB_ROW_BYTES(bmih.biWidth, bmih.biBitCount);
	const BYTE *pSrc = static_cast<const BYTE*>(pBits) +
		((bmih.biHeight > 0) ?
			bmih.biHeight - pRect->bottom : pRect->top) * SrcRowBytes +
		pRect->left * bmih.biBitCount / 8;
	BYTE *pDst = reinterpret_cast<BYTE*>(pbmihDst + 1);

	for (int y = 0; y < DstHeight; y++) {
		CopyPixels(pDst, BitsPerPixel, pSrc, bmih.biBitCount, nullptr, DstWidth);
		pSrc += SrcRowBytes;
		pDst += DstRowBytes;
	}

	::GlobalUnlock(hGlobal);

	return hGlobal;
}


} // namespace ImageLib

} // namespace TVTest
