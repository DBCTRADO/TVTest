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


#include <windows.h>
#include <tchar.h>
#include "libpng/png.h"
#include "zlib/zlib.h"
#include "ImageLib.h"
#include "Codec_PNG.h"
#include "ImageUtil.h"


namespace TVTest
{

namespace ImageLib
{


static void WriteData(png_structp pPNG, png_bytep pbData, png_size_t Length)
{
	HANDLE hFile;
	DWORD dwWrite;

	hFile = (HANDLE)png_get_io_ptr(pPNG);
	if (!WriteFile(hFile, pbData, (DWORD)Length, &dwWrite, nullptr) || dwWrite != Length)
		png_error(pPNG, "Write Error");
}


static void FlushData(png_structp pPNG)
{
	HANDLE hFile;

	hFile = (HANDLE)png_get_io_ptr(pPNG);
	FlushFileBuffers(hFile);
}


// PNG をファイルに保存する
bool SavePNGFile(const ImageSaveInfo *pInfo)
{
	HANDLE hFile;
	int Width, Height, BitsPerPixel;
	png_structp pPNG;
	png_infop pPNGInfo;
	int i, nPasses, y;
	int nSrcRowBytes;
	png_bytep pbRow;
	BYTE *pBuff = nullptr;

	hFile = CreateFile(
		pInfo->pszFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;
	pPNG = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (pPNG == nullptr) {
		CloseHandle(hFile);
		return false;
	}
	pPNGInfo = png_create_info_struct(pPNG);
	if (pPNGInfo == nullptr) {
		png_destroy_write_struct(&pPNG, nullptr);
		CloseHandle(hFile);
		return false;
	}
	if (setjmp(png_jmpbuf(pPNG))) {
		png_destroy_write_struct(&pPNG, &pPNGInfo);
		if (pBuff != nullptr)
			delete [] pBuff;
		CloseHandle(hFile);
		return false;
	}
	png_set_write_fn(pPNG, hFile, WriteData, FlushData);
	png_set_compression_level(pPNG, _ttoi(pInfo->pszOption));
	Width = pInfo->pbmi->bmiHeader.biWidth;
	Height = abs(pInfo->pbmi->bmiHeader.biHeight);
	BitsPerPixel = pInfo->pbmi->bmiHeader.biBitCount;
	png_set_IHDR(
		pPNG, pPNGInfo, Width, Height,
		BitsPerPixel < 8 ? BitsPerPixel : 8,
		BitsPerPixel <= 8 ? PNG_COLOR_TYPE_PALETTE : PNG_COLOR_TYPE_RGB,
		//fInterlace ? PNG_INTERLACE_NONE : PNG_INTERLACE_ADAM7,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	if (BitsPerPixel <= 8) {
		int nColors = 1 << BitsPerPixel;
		png_color PNGPalette[256];
		const RGBQUAD *prgb;

		prgb = pInfo->pbmi->bmiColors;
		for (i = 0; i < nColors; i++) {
			PNGPalette[i].red = prgb->rgbRed;
			PNGPalette[i].green = prgb->rgbGreen;
			PNGPalette[i].blue = prgb->rgbBlue;
			prgb++;
		}
		png_set_PLTE(pPNG, pPNGInfo, PNGPalette, nColors);
	}
	if (pInfo->pszComment != nullptr) {
		png_text PNGText;

#ifndef UNICODE
		int Length = MultiByteToWideChar(CP_ACP, 0, pInfo->pszComment, -1, nullptr, 0);
		LPWSTR pszComment = new WCHAR[Length];
		MultiByteToWideChar(CP_ACP, 0, pInfo->pszComment, -1, pszComment, Length);
		Length = WideCharToMultiByte(CP_UTF8, 0, pszComment, -1, nullptr, 0, nullptr, nullptr);
		PNGText.text = new char[Length];
		WideCharToMultiByte(CP_UTF8, 0, pszComment, -1, PNGText.text, Length, nullptr, nullptr);
		delete [] pszComment;
#else
		int Length = WideCharToMultiByte(CP_UTF8, 0, pInfo->pszComment, -1, nullptr, 0, nullptr, nullptr);
		PNGText.text = new char[Length];
		WideCharToMultiByte(CP_UTF8, 0, pInfo->pszComment, -1, PNGText.text, Length, nullptr, nullptr);
#endif
		PNGText.compression = PNG_ITXT_COMPRESSION_NONE;
		PNGText.key = const_cast<png_charp>("Comment");
		PNGText.text_length = 0;
		PNGText.itxt_length =::lstrlenA(PNGText.text);
		PNGText.lang = nullptr;
		PNGText.lang_key = nullptr;
		png_set_text(pPNG, pPNGInfo, &PNGText, 1);
		delete [] PNGText.text;
	}
	png_write_info(pPNG, pPNGInfo);
	if (BitsPerPixel > 8)
		png_set_bgr(pPNG);
	/*if (fInterlace)
		nPasses = png_set_interlace_handling(pPNG);
	else
		nPasses = 1;
	 */
	nPasses = 1;
	nSrcRowBytes = DIB_ROW_BYTES(Width, BitsPerPixel);
	if (BitsPerPixel == 32)
		pBuff = new BYTE[Width * 3];
	for (i = 0; i < nPasses; i++) {
		for (y = 0; y < Height; y++) {
			pbRow =
				(png_bytep)pInfo->pBits +
					(pInfo->pbmi->bmiHeader.biHeight > 0 ? (Height - 1 - y) : y) * nSrcRowBytes;
			if (pBuff != nullptr) {
				int x;
				const BYTE *p;
				BYTE *q;

				p = pbRow;
				q = pBuff;
				for (x = 0; x < Width; x++) {
					*q++ = p[0];
					*q++ = p[1];
					*q++ = p[2];
					p += 4;
				}
				pbRow = pBuff;
			}
			png_write_rows(pPNG, &pbRow, 1);
		}
	}
	if (pBuff != nullptr) {
		delete [] pBuff;
		pBuff = nullptr;
	}
	png_write_end(pPNG, pPNGInfo);
	png_destroy_write_struct(&pPNG, &pPNGInfo);
	CloseHandle(hFile);
	return true;
}




struct IHDR
{
	DWORD Width;
	DWORD Height;
	BYTE BitDepth;
	BYTE ColorType;
	BYTE CompressionMethod;
	BYTE FilterMethod;
	BYTE InterlaceMethod;
};

struct RGBA
{
	BYTE Red, Green, Blue, Alpha;
};

#define CHUNK_TYPE(c1,c2,c3,c4) \
	(((DWORD)(c1) << 24) | ((DWORD)(c2) << 16) | ((DWORD)(c3) << 8) | (DWORD)c4)

inline DWORD MSBFirst32(const BYTE *p)
{
	return ((DWORD)p[0] << 24) | ((DWORD)p[1] << 16) | ((DWORD)p[2] << 8) | (DWORD)p[3];
}

static const RGBA DefaultPalette[128] = {
	{  0,   0,   0, 255},
	{255,   0,   0, 255},
	{  0, 255,   0, 255},
	{255, 255,   0, 255},
	{  0,   0,   0, 255},
	{255,   0, 255, 255},
	{  0, 255, 255, 255},
	{255, 255, 255, 255},
	{  0,   0,   0,   0},
	{170,   0,   0, 255},
	{  0, 170,   0, 255},
	{170, 170,   0, 255},
	{  0,   0, 170, 255},
	{170,   0, 170, 255},
	{  0, 170, 170, 255},
	{170, 170, 170, 255},
	{  0,   0,  85, 255},
	{  0,  85,   0, 255},
	{  0,  85,  85, 255},
	{  0,  85, 170, 255},
	{  0,  85, 255, 255},
	{  0, 170,  85, 255},
	{  0, 170, 255, 255},
	{  0, 255,  85, 255},
	{  0, 255, 170, 255},
	{ 85,   0,   0, 255},
	{ 85,   0,  85, 255},
	{ 85,   0, 170, 255},
	{ 85,   0, 255, 255},
	{ 85,  85,   0, 255},
	{ 85,  85,  85, 255},
	{ 85,  85, 170, 255},
	{ 85,  85, 255, 255},
	{ 85, 170,   0, 255},
	{ 85, 170,  85, 255},
	{ 85, 170, 170, 255},
	{ 85, 170, 255, 255},
	{ 85, 255,   0, 255},
	{ 85, 255,  85, 255},
	{ 85, 255, 170, 255},
	{ 85, 255, 255, 255},
	{170,   0,  85, 255},
	{170,   0, 255, 255},
	{170,  85,   0, 255},
	{170,  85,  85, 255},
	{170,  85, 170, 255},
	{170,  85, 255, 255},
	{170, 170,  85, 255},
	{170, 170, 255, 255},
	{170, 255,   0, 255},
	{170, 255,  85, 255},
	{170, 255, 170, 255},
	{170, 255, 255, 255},
	{255,   0,  85, 255},
	{255,   0, 170, 255},
	{255,  85,   0, 255},
	{255,  85,  85, 255},
	{255,  85, 170, 255},
	{255,  85, 255, 255},
	{255, 170,   0, 255},
	{255, 170,  85, 255},
	{255, 170, 170, 255},
	{255, 170, 255, 255},
	{255, 255,  85, 255},
	{255, 255, 170, 255},
	{  0,   0,   0, 128},
	{255,   0,   0, 128},
	{  0, 255,   0, 128},
	{255, 255,   0, 128},
	{  0,   0, 255, 128},
	{255,   0, 255, 128},
	{  0, 255, 255, 128},
	{255, 255, 255, 128},
	{170,   0,   0, 128},
	{  0, 170,   0, 128},
	{170, 170,   0, 128},
	{  0,   0, 170, 128},
	{170,   0, 170, 128},
	{  0, 170, 170, 128},
	{170, 170, 170, 128},
	{  0,   0,  85, 128},
	{  0,  85,   0, 128},
	{  0,  85,  85, 128},
	{  0,  85, 170, 128},
	{  0,  85, 255, 128},
	{  0, 170,  85, 128},
	{  0, 170, 255, 128},
	{  0, 255,  85, 128},
	{  0, 255, 170, 128},
	{ 85,   0,   0, 128},
	{ 85,   0,  85, 128},
	{ 85,   0, 170, 128},
	{ 85,   0, 255, 128},
	{ 85,  85,   0, 128},
	{ 85,  85,  85, 128},
	{ 85,  85, 170, 128},
	{ 85,  85, 255, 128},
	{ 85, 170,   0, 128},
	{ 85, 170,  85, 128},
	{ 85, 170, 170, 128},
	{ 85, 170, 255, 128},
	{ 85, 255,   0, 128},
	{ 85, 255,  85, 128},
	{ 85, 255, 170, 128},
	{ 85, 255, 255, 128},
	{170,   0,  85, 128},
	{170,   0, 255, 128},
	{170,  85,   0, 128},
	{170,  85,  85, 128},
	{170,  85, 170, 128},
	{170,  85, 255, 128},
	{170, 170,  85, 128},
	{170, 170, 255, 128},
	{170, 255,   0, 128},
	{170, 255,  85, 128},
	{170, 255, 170, 128},
	{170, 255, 255, 128},
	{255,   0,  85, 128},
	{255,   0, 170, 128},
	{255,  85,   0, 128},
	{255,  85,  85, 128},
	{255,  85, 170, 128},
	{255,  85, 255, 128},
	{255, 170,   0, 128},
	{255, 170,  85, 128},
	{255, 170, 170, 128},
	{255, 170, 255, 128},
	{255, 255,  85, 128},
};

// ARIB 形式の PNG を読み込む
HGLOBAL LoadAribPng(const void *pData, SIZE_T DataSize)
{
	static const BYTE Adam7[8][2][2] = {
		{{ 1, 0}, { 1, 0}},	// No interlace
		{{ 8, 0}, { 8, 0}},	// Interlaced image 1
		{{ 8, 4}, { 8, 0}},	// Interlaced image 2
		{{ 4, 0}, { 8, 4}},	// Interlaced image 3
		{{ 4, 2}, { 4, 0}},	// Interlaced image 4
		{{ 2, 0}, { 4, 2}},	// Interlaced image 5
		{{ 2, 1}, { 2, 0}},	// Interlaced image 6
		{{ 1, 0}, { 2, 1}},	// Interlaced image 7
	};
	const BYTE *p;
	SIZE_T Pos;
	IHDR ImageHeader;
	const BYTE *pCompressedImageData = nullptr;
	SIZE_T CompressedImageSize = 0;

	if (pData == nullptr || DataSize <= 8)
		return nullptr;
	p = static_cast<const BYTE*>(pData);
	if (memcmp(p, "\x89PNG\r\n\x1A\n", 8) != 0)
		return nullptr;
	Pos = 8;
	while (Pos + 8 < DataSize) {
		DWORD ChunkSize = MSBFirst32(&p[Pos]);
		DWORD ChunkType = MSBFirst32(&p[Pos + 4]);
		if (Pos + 8 + ChunkSize + 4 > DataSize)
			return nullptr;
		if (crc32(crc32(0, Z_NULL, 0), &p[Pos + 4], ChunkSize + 4) != MSBFirst32(&p[Pos + 8 + ChunkSize]))
			return nullptr;
		Pos += 8;
		switch (ChunkType) {
		case CHUNK_TYPE('I', 'H', 'D', 'R'):
			if (ChunkSize != 13)
				return nullptr;
			ImageHeader.Width = MSBFirst32(&p[Pos]);
			ImageHeader.Height = MSBFirst32(&p[Pos + 4]);
			ImageHeader.BitDepth = p[Pos + 8];
			ImageHeader.ColorType = p[Pos + 9];
			ImageHeader.CompressionMethod = p[Pos + 10];
			ImageHeader.FilterMethod = p[Pos + 11];
			ImageHeader.InterlaceMethod = p[Pos + 12];
			if (ImageHeader.ColorType == 1 || ImageHeader.ColorType == 5
					|| ImageHeader.ColorType > 6
					|| ImageHeader.CompressionMethod != 0
					|| ImageHeader.InterlaceMethod > 1)
				return nullptr;
			if ((ImageHeader.BitDepth != 1 && ImageHeader.BitDepth != 2
					&& ImageHeader.BitDepth != 4 && ImageHeader.BitDepth != 8
					&& ImageHeader.BitDepth != 16)
					|| (ImageHeader.ColorType == 3 && ImageHeader.BitDepth > 8))
				return nullptr;
			break;

		case CHUNK_TYPE('I', 'D', 'A', 'T'):
			pCompressedImageData = &p[Pos];
			CompressedImageSize = ChunkSize;
			break;

		case CHUNK_TYPE('I', 'E', 'N', 'D'):
			goto Decode;
		}
		Pos += ChunkSize + 4;
	}
Decode:
	if (pCompressedImageData == nullptr)
		return nullptr;

	int PlanesPerPixel;
	struct {
		int Width;
		int Height;
		SIZE_T BytesPerLine;
	} InterlacedImage[8];
	int i;
	SIZE_T ImageDataSize;
	BYTE *pImageData;

	switch (ImageHeader.ColorType) {
	case 0: // Grayscale
	case 3: // Indexed
		PlanesPerPixel = 1;
		break;
	case 4: // Grayscale + Alpha
		PlanesPerPixel = 2;
		break;
	case 2: // True color
		PlanesPerPixel = 3;
		break;
	case 6: // True color + Alpha
		PlanesPerPixel = 4;
		break;
	}
	ImageDataSize = 0;
	for (i = ImageHeader.InterlaceMethod; i < 8; i++) {
		InterlacedImage[i].Width = (ImageHeader.Width + Adam7[i][0][0] - Adam7[i][0][1] - 1) / Adam7[i][0][0];
		InterlacedImage[i].Height = (ImageHeader.Height + Adam7[i][1][0] - Adam7[i][1][1] - 1) / Adam7[i][1][0];
		InterlacedImage[i].BytesPerLine = (InterlacedImage[i].Width * ImageHeader.BitDepth * PlanesPerPixel + 7) / 8 + 1;
		ImageDataSize += InterlacedImage[i].BytesPerLine * InterlacedImage[i].Height;
		if (i == 0)
			break;
	}
	pImageData = new BYTE[ImageDataSize];

	uLongf DecompressSize = (uLongf)ImageDataSize;
	if (uncompress(
				pImageData, &DecompressSize,
				pCompressedImageData, (uLongf)CompressedImageSize) != Z_OK) {
		delete [] pImageData;
		return nullptr;
	}

	// 常に32ビットDIBに変換する
	HGLOBAL hDIB =::GlobalAlloc(GMEM_MOVEABLE, sizeof(BITMAPINFOHEADER) + ImageHeader.Width * 4 * ImageHeader.Height);
	if (hDIB == nullptr) {
		delete [] pImageData;
		return nullptr;
	}
	BITMAPINFOHEADER *pbmih = (BITMAPINFOHEADER*)::GlobalLock(hDIB);
	pbmih->biSize = sizeof(BITMAPINFOHEADER);
	pbmih->biWidth = ImageHeader.Width;
	pbmih->biHeight = ImageHeader.Height;
	pbmih->biPlanes = 1;
	pbmih->biBitCount = 32;
	pbmih->biCompression = BI_RGB;
	pbmih->biSizeImage = 0;
	pbmih->biXPelsPerMeter = 0;
	pbmih->biYPelsPerMeter = 0;
	pbmih->biClrUsed = 0;
	pbmih->biClrImportant = 0;
	BYTE *pDIBBits = (BYTE*)(pbmih + 1);

	BYTE *q = pImageData;
	int SampleMask = (1 << ImageHeader.BitDepth) - 1;
	int PixelBytes = (ImageHeader.BitDepth * PlanesPerPixel + 7) / 8;
	int x, y, z;

	for (i = ImageHeader.InterlaceMethod; i < 8; i++) {
		for (y = 0; y < InterlacedImage[i].Height; y++) {
			int FilterType = *q++;
			BYTE *r = q;

			if (FilterType > 4) {
				::GlobalFree(hDIB);
				delete [] pImageData;
				return nullptr;
			}
			for (x = 0; (SIZE_T)x < InterlacedImage[i].BytesPerLine - 1; x++, q++) {
				int a = (x >= PixelBytes) ? *(q - PixelBytes) : 0;
				int b = (y > 0) ? *(q - InterlacedImage[i].BytesPerLine) : 0;
				int c = (x >= PixelBytes && y > 0) ? *(q - PixelBytes - InterlacedImage[i].BytesPerLine) : 0;
				switch (FilterType) {
				case 0: // None
					break;
				case 1: // Sub
					*q += a;
					break;
				case 2: // Up
					*q += b;
					break;
				case 3: // Average
					*q += (a + b) / 2;
					break;
				case 4: // Paeth
					{
						int pa = abs(a + b - c - a);
						int pb = abs(a + b - c - b);
						int pc = abs(a + b - c - c);
						*q += (pa <= pb && pa <= pc) ? a : (pb <= pc) ? b : c;
					}
					break;
				}
			}
			BYTE *pDestLine =
				pDIBBits +
					(ImageHeader.Height - 1 - (y * Adam7[i][1][0] + Adam7[i][1][1])) * (ImageHeader.Width * 4);
			for (x = 0; x < InterlacedImage[i].Width; x++) {
				int Sample[4];
				for (z = 0; z < PlanesPerPixel; z++) {
					int s = ImageHeader.BitDepth * (x * PlanesPerPixel + z);
					BYTE *t = r + s / 8;
					s = (8 - s - ImageHeader.BitDepth) & 7;
					if (ImageHeader.BitDepth == 16) {
						Sample[z] = (t[0] << 8) | t[1];
					} else { /* Bit_depth 1,2,4,8 */
						Sample[z] = (t[0] >> s)&SampleMask;
					}
				}
				int x1 = x * Adam7[i][0][0] + Adam7[i][0][1];
				switch (ImageHeader.ColorType) {
				case 0:	// Grayscale
					pDestLine[x1 * 4 + 0] = Sample[0];
					pDestLine[x1 * 4 + 1] = Sample[0];
					pDestLine[x1 * 4 + 2] = Sample[0];
					pDestLine[x1 * 4 + 3] = 0xFF;
					break;
				case 4:	// Grayscale + Alpha
					pDestLine[x1 * 4 + 0] = Sample[0];
					pDestLine[x1 * 4 + 1] = Sample[0];
					pDestLine[x1 * 4 + 2] = Sample[0];
					pDestLine[x1 * 4 + 3] = Sample[1];
					break;
				case 2:	// True color
					pDestLine[x1 * 4 + 0] = (Sample[2] * 255) / SampleMask;
					pDestLine[x1 * 4 + 1] = (Sample[1] * 255) / SampleMask;
					pDestLine[x1 * 4 + 2] = (Sample[0] * 255) / SampleMask;
					pDestLine[x1 * 4 + 3] = 0xFF;
					break;
				case 6:	// True color + Alpha
					pDestLine[x1 * 4 + 0] = (Sample[2] * 255) / SampleMask;
					pDestLine[x1 * 4 + 1] = (Sample[1] * 255) / SampleMask;
					pDestLine[x1 * 4 + 2] = (Sample[0] * 255) / SampleMask;
					pDestLine[x1 * 4 + 3] = (Sample[3] * 255) / SampleMask;
					break;
				case 3:	// Indexed
					{
						const RGBA &Color = DefaultPalette[Sample[0] < 128 ? Sample[0] : 8];
						pDestLine[x1 * 4 + 0] = Color.Blue;
						pDestLine[x1 * 4 + 1] = Color.Green;
						pDestLine[x1 * 4 + 2] = Color.Red;
						pDestLine[x1 * 4 + 3] = Color.Alpha;
					}
					break;
				}
			}
		}
		if (i == 0)
			break;
	}
	delete [] pImageData;

	return hDIB;
}


}	// namespace ImageLib

}	// namespace TVTest
