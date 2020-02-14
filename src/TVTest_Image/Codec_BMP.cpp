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
#include "ImageLib.h"
#include "Codec_BMP.h"
#include "ImageUtil.h"


namespace TVTest
{

namespace ImageLib
{


bool SaveBMPFile(const ImageSaveInfo *pInfo)
{
	int Width, Height, BitsPerPixel;
	HANDLE hFile;
	DWORD dwWrite;
	SIZE_T InfoBytes, RowBytes, BitsBytes;

	Width = pInfo->pbmi->bmiHeader.biWidth;
	Height = abs(pInfo->pbmi->bmiHeader.biHeight);
	BitsPerPixel = pInfo->pbmi->bmiHeader.biBitCount;
	InfoBytes = sizeof(BITMAPINFOHEADER);
	if (BitsPerPixel <= 8)
		InfoBytes += ((SIZE_T)1 << BitsPerPixel) * sizeof(RGBQUAD);
	else if (pInfo->pbmi->bmiHeader.biCompression == BI_BITFIELDS)
		InfoBytes += 3 * sizeof(DWORD);
	RowBytes = DIB_ROW_BYTES(Width, BitsPerPixel);
	BitsBytes = RowBytes * Height;
	hFile = CreateFile(
		pInfo->pszFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE) {
		return false;
	}
	/* BITMAPFILEHEADERの書き込み */
	BITMAPFILEHEADER bmfh;

	bmfh.bfType = 0x4D42;
	bmfh.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER) + InfoBytes + BitsBytes);
	bmfh.bfReserved1 = bmfh.bfReserved2 = 0;
	bmfh.bfOffBits = (DWORD)(sizeof(BITMAPFILEHEADER) + InfoBytes);
	if (!WriteFile(hFile, &bmfh, sizeof(BITMAPFILEHEADER), &dwWrite, nullptr)
			|| dwWrite != sizeof(BITMAPFILEHEADER)) {
		CloseHandle(hFile);
		return false;
	}
	/* ヘッダを書き込む */
	BITMAPINFOHEADER bmih;

	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biWidth = Width;
	bmih.biHeight = Height;
	bmih.biPlanes = 1;
	bmih.biBitCount = BitsPerPixel;
	bmih.biCompression =
		pInfo->pbmi->bmiHeader.biCompression == BI_BITFIELDS ?
			BI_BITFIELDS : BI_RGB;
	bmih.biSizeImage = 0;
	bmih.biXPelsPerMeter = 0;
	bmih.biYPelsPerMeter = 0;
	bmih.biClrUsed = 0;
	bmih.biClrImportant = 0;
	if (!WriteFile(hFile, &bmih, sizeof(BITMAPINFOHEADER), &dwWrite, nullptr)
			|| dwWrite != sizeof(BITMAPINFOHEADER)) {
		CloseHandle(hFile);
		return false;
	}
	if (InfoBytes > sizeof(BITMAPINFOHEADER)) {
		DWORD PalBytes = (DWORD)(InfoBytes - sizeof(BITMAPINFOHEADER));

		if (!WriteFile(
					hFile, pInfo->pbmi->bmiColors, PalBytes,
					&dwWrite, nullptr) || dwWrite != PalBytes) {
			CloseHandle(hFile);
			return false;
		}
	}
	/* ビットデータを書き込む */
	if (pInfo->pbmi->bmiHeader.biHeight > 0) {
		if (!WriteFile(hFile, pInfo->pBits, (DWORD)BitsBytes, &dwWrite, nullptr)
				|| dwWrite != BitsBytes) {
			CloseHandle(hFile);
			return false;
		}
	} else {
		int y;
		const BYTE *p;

		p = static_cast<const BYTE*>(pInfo->pBits) + (Height - 1) * RowBytes;
		for (y = 0; y < Height; y++) {
			if (!WriteFile(hFile, p, (DWORD)RowBytes, &dwWrite, nullptr)
					|| dwWrite != RowBytes) {
				CloseHandle(hFile);
				return false;
			}
			p -= RowBytes;
		}
	}
	CloseHandle(hFile);
	return true;
}


}	// namespace ImageLib

}	// namespace TVTest
