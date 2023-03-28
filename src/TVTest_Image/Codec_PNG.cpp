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
#include <stdio.h>
#include <tchar.h>
#include <cstdlib>
#include <memory>
#include <string>
#include "libpng/png.h"
#include "zlib/zlib.h"
#include "ImageLib.h"
#include "Codec_PNG.h"
#include "ImageUtil.h"


namespace TVTest
{

namespace ImageLib
{


static void PNGError(png_structp png_ptr, png_const_charp error_msg)
{
#ifdef _DEBUG
	char Text[256];
	std::snprintf(Text, sizeof(Text), "libpng error: %s\n", error_msg != nullptr ? error_msg : "(undefined)");
	::OutputDebugStringA(Text);
#endif
	throw PNGError;
}


static void PNGWarning(png_structp png_ptr, png_const_charp warning_msg)
{
#ifdef _DEBUG
	char Text[256];
	std::snprintf(Text, sizeof(Text), "libpng warning: %s\n", warning_msg);
	::OutputDebugStringA(Text);
#endif
}


// PNG をファイルに保存する
bool SavePNGFile(const ImageSaveInfo *pInfo)
{
	FILE *fp;
	if (_tfopen_s(&fp, pInfo->pszFileName, TEXT("wbN")) != 0)
		return false;
	// 書き込み単位がとても小さく保存先によってはバッファリングの効果が大きいため
	std::setvbuf(fp, nullptr, _IOFBF, 64 * 1024);

	png_structp pPNG = nullptr;
	png_infop pPNGInfo = nullptr;

	try {
		pPNG = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, PNGError, PNGWarning);
		if (pPNG == nullptr) {
			std::fclose(fp);
			return false;
		}
		pPNGInfo = png_create_info_struct(pPNG);
		if (pPNGInfo == nullptr) {
			png_destroy_write_struct(&pPNG, nullptr);
			std::fclose(fp);
			return false;
		}

		png_init_io(pPNG, fp);
		png_set_compression_level(pPNG, _ttoi(pInfo->pszOption));

		const int Width = pInfo->pbmi->bmiHeader.biWidth;
		const int Height = std::abs(pInfo->pbmi->bmiHeader.biHeight);
		const int BitsPerPixel = pInfo->pbmi->bmiHeader.biBitCount;
		png_set_IHDR(
			pPNG, pPNGInfo, Width, Height,
			BitsPerPixel < 8 ? BitsPerPixel : 8,
			BitsPerPixel <= 8 ? PNG_COLOR_TYPE_PALETTE : PNG_COLOR_TYPE_RGB,
			//fInterlace ? PNG_INTERLACE_NONE : PNG_INTERLACE_ADAM7,
			PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

		if (BitsPerPixel <= 8) {
			const int Colors = 1 << BitsPerPixel;
			png_color PNGPalette[256];
			const RGBQUAD *prgb = pInfo->pbmi->bmiColors;

			for (int i = 0; i < Colors; i++) {
				PNGPalette[i].red = prgb->rgbRed;
				PNGPalette[i].green = prgb->rgbGreen;
				PNGPalette[i].blue = prgb->rgbBlue;
				prgb++;
			}
			png_set_PLTE(pPNG, pPNGInfo, PNGPalette, Colors);
		}

		if (pInfo->pszComment != nullptr) {
			png_text PNGText;
			std::string Text;

#ifndef UNICODE
			int Length = MultiByteToWideChar(CP_ACP, 0, pInfo->pszComment, -1, nullptr, 0);
			std::wstring Comment(Length);
			MultiByteToWideChar(CP_ACP, 0, pInfo->pszComment, -1, Comment.data(), Length);
			Length = WideCharToMultiByte(CP_UTF8, 0, Comment.c_str(), -1, nullptr, 0, nullptr, nullptr);
			Text.resize(Length);
			WideCharToMultiByte(CP_UTF8, 0, Comment.c_str(), -1, Text.data(), Length, nullptr, nullptr);
#else
			const int Length = WideCharToMultiByte(CP_UTF8, 0, pInfo->pszComment, -1, nullptr, 0, nullptr, nullptr);
			Text.resize(Length);
			WideCharToMultiByte(CP_UTF8, 0, pInfo->pszComment, -1, Text.data(), Length, nullptr, nullptr);
#endif
			PNGText.compression = PNG_ITXT_COMPRESSION_NONE;
			PNGText.key = const_cast<png_charp>("Comment");
			PNGText.text = Text.data();
			PNGText.text_length = 0;
			PNGText.itxt_length = ::lstrlenA(PNGText.text);
			PNGText.lang = nullptr;
			PNGText.lang_key = nullptr;
			png_set_text(pPNG, pPNGInfo, &PNGText, 1);
		}

		png_write_info(pPNG, pPNGInfo);

		if (BitsPerPixel > 8)
			png_set_bgr(pPNG);

		/*if (fInterlace)
			Passes = png_set_interlace_handling(pPNG);
		else
			Passes = 1;
		*/
		constexpr int Passes = 1;
		const int SrcRowBytes = DIB_ROW_BYTES(Width, BitsPerPixel);

		std::unique_ptr<BYTE[]> Buff;
		if (BitsPerPixel == 32)
			Buff.reset(new BYTE[Width * 3]);

		for (int i = 0; i < Passes; i++) {
			for (int y = 0; y < Height; y++) {
				png_bytep pRow =
					const_cast<png_bytep>(static_cast<const png_byte*>(pInfo->pBits) +
						(pInfo->pbmi->bmiHeader.biHeight > 0 ? (Height - 1 - y) : y) * SrcRowBytes);
				if (Buff) {
					const BYTE *p = pRow;
					BYTE *q = Buff.get();

					for (int x = 0; x < Width; x++) {
						*q++ = p[0];
						*q++ = p[1];
						*q++ = p[2];
						p += 4;
					}
					pRow = Buff.get();
				}
				png_write_rows(pPNG, &pRow, 1);
			}
		}

		png_write_end(pPNG, pPNGInfo);
		png_destroy_write_struct(&pPNG, &pPNGInfo);
		std::fclose(fp);
	} catch (...) {
		png_destroy_write_struct(&pPNG, &pPNGInfo);
		std::fclose(fp);
		return false;
	}

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
	((static_cast<DWORD>(c1) << 24) | (static_cast<DWORD>(c2) << 16) | (static_cast<DWORD>(c3) << 8) | static_cast<DWORD>(c4))

inline DWORD MSBFirst32(const BYTE *p)
{
	return (static_cast<DWORD>(p[0]) << 24) | (static_cast<DWORD>(p[1]) << 16) | (static_cast<DWORD>(p[2]) << 8) | static_cast<DWORD>(p[3]);
}

static const RGBA DefaultPalette[128] = {
	{  0,   0,   0, 255},
	{255,   0,   0, 255},
	{  0, 255,   0, 255},
	{255, 255,   0, 255},
	{  0,   0, 255, 255},
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
HGLOBAL LoadAribPng(const void *pData, size_t DataSize)
{
	if (pData == nullptr || DataSize <= 8)
		return nullptr;

	const BYTE *p = static_cast<const BYTE*>(pData);
	if (std::memcmp(p, "\x89PNG\r\n\x1A\n", 8) != 0)
		return nullptr;

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
	IHDR ImageHeader;
	const BYTE *pCompressedImageData = nullptr;
	size_t CompressedImageSize = 0;

	size_t Pos = 8;
	while (Pos + 8 < DataSize) {
		const DWORD ChunkSize = MSBFirst32(&p[Pos]);
		const DWORD ChunkType = MSBFirst32(&p[Pos + 4]);
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
		size_t BytesPerLine;
	} InterlacedImage[8];

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
	size_t ImageDataSize = 0;
	for (int i = ImageHeader.InterlaceMethod; i < 8; i++) {
		InterlacedImage[i].Width = (ImageHeader.Width + Adam7[i][0][0] - Adam7[i][0][1] - 1) / Adam7[i][0][0];
		InterlacedImage[i].Height = (ImageHeader.Height + Adam7[i][1][0] - Adam7[i][1][1] - 1) / Adam7[i][1][0];
		InterlacedImage[i].BytesPerLine = (InterlacedImage[i].Width * ImageHeader.BitDepth * PlanesPerPixel + 7) / 8 + 1;
		ImageDataSize += InterlacedImage[i].BytesPerLine * InterlacedImage[i].Height;
		if (i == 0)
			break;
	}
	std::unique_ptr<BYTE[]> ImageData(new BYTE[ImageDataSize]);

	uLongf DecompressSize = static_cast<uLongf>(ImageDataSize);
	if (uncompress(
				ImageData.get(), &DecompressSize,
				pCompressedImageData, static_cast<uLongf>(CompressedImageSize)) != Z_OK) {
		return nullptr;
	}

	// 常に32ビットDIBに変換する
	const HGLOBAL hDIB =::GlobalAlloc(GMEM_MOVEABLE, sizeof(BITMAPINFOHEADER) + ImageHeader.Width * 4 * ImageHeader.Height);
	if (hDIB == nullptr) {
		return nullptr;
	}
	BITMAPINFOHEADER *pbmih = static_cast<BITMAPINFOHEADER*>(::GlobalLock(hDIB));
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
	BYTE *pDIBBits = reinterpret_cast<BYTE*>(pbmih + 1);

	BYTE *q = ImageData.get();
	const int SampleMask = (1 << ImageHeader.BitDepth) - 1;
	const int PixelBytes = (ImageHeader.BitDepth * PlanesPerPixel + 7) / 8;

	for (int i = ImageHeader.InterlaceMethod; i < 8; i++) {
		for (int y = 0; y < InterlacedImage[i].Height; y++) {
			const int FilterType = *q++;
			BYTE *r = q;

			if (FilterType > 4) {
				::GlobalFree(hDIB);
				return nullptr;
			}
			for (int x = 0; static_cast<size_t>(x) < InterlacedImage[i].BytesPerLine - 1; x++, q++) {
				const int a = (x >= PixelBytes) ? *(q - PixelBytes) : 0;
				const int b = (y > 0) ? *(q - InterlacedImage[i].BytesPerLine) : 0;
				const int c = (x >= PixelBytes && y > 0) ? *(q - PixelBytes - InterlacedImage[i].BytesPerLine) : 0;
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
						const int pa = std::abs(a + b - c - a);
						const int pb = std::abs(a + b - c - b);
						const int pc = std::abs(a + b - c - c);
						*q += (pa <= pb && pa <= pc) ? a : (pb <= pc) ? b : c;
					}
					break;
				}
			}
			BYTE *pDestLine =
				pDIBBits +
					(ImageHeader.Height - 1 - (y * Adam7[i][1][0] + Adam7[i][1][1])) * (ImageHeader.Width * 4);
			for (int x = 0; x < InterlacedImage[i].Width; x++) {
				int Sample[4];
				for (int z = 0; z < PlanesPerPixel; z++) {
					int s = ImageHeader.BitDepth * (x * PlanesPerPixel + z);
					BYTE *t = r + s / 8;
					s = (8 - s - ImageHeader.BitDepth) & 7;
					if (ImageHeader.BitDepth == 16) {
						Sample[z] = (t[0] << 8) | t[1];
					} else { /* Bit_depth 1,2,4,8 */
						Sample[z] = (t[0] >> s)&SampleMask;
					}
				}
				const int x1 = x * Adam7[i][0][0] + Adam7[i][0][1];
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

	return hDIB;
}


}	// namespace ImageLib

}	// namespace TVTest
