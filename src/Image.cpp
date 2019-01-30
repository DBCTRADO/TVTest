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


SIZE_T CalcDIBInfoSize(const BITMAPINFOHEADER *pbmih)
{
	SIZE_T Size;

	Size = sizeof(BITMAPINFOHEADER);
	if (pbmih->biBitCount <= 8)
		Size += ((SIZE_T)1 << pbmih->biBitCount) * sizeof(RGBQUAD);
	else if (pbmih->biCompression == BI_BITFIELDS)
		Size += 3 * sizeof(DWORD);
	return Size;
}


SIZE_T CalcDIBBitsSize(const BITMAPINFOHEADER *pbmih)
{
	return DIB_ROW_BYTES(pbmih->biWidth, pbmih->biBitCount) * abs(pbmih->biHeight);
}


SIZE_T CalcDIBSize(const BITMAPINFOHEADER *pbmih)
{
	return CalcDIBInfoSize(pbmih) + CalcDIBBitsSize(pbmih);
}


static void CropImage(
	const BITMAPINFO *pbmiSrc, const void *pSrcData,
	int Left, int Top, int Width, int Height, void *pDstData)
{
	int x, y;
	std::size_t SrcRowBytes, DstRowBytes;
	const BYTE *p;
	BYTE *q;

	SrcRowBytes = DIB_ROW_BYTES(pbmiSrc->bmiHeader.biWidth,
								pbmiSrc->bmiHeader.biBitCount);
	DstRowBytes = DIB_ROW_BYTES(Width, 24);
	q = (BYTE*)pDstData + (Height - 1) * DstRowBytes;
	for (y = 0; y < Height; y++) {
		p = (const BYTE*)pSrcData +
			((pbmiSrc->bmiHeader.biHeight > 0) ?
				(pbmiSrc->bmiHeader.biHeight - 1 - (Top + y)) : (Top + y)) * SrcRowBytes +
			Left * pbmiSrc->bmiHeader.biBitCount / 8;
		if (pbmiSrc->bmiHeader.biBitCount == 24) {
			CopyMemory(q, p, Width * 3);
		} else {
			BYTE *r;

			r = q;
			for (x = 0; x < Width; x++) {
				*r++ = *p++;
				*r++ = *p++;
				*r++ = *p++;
				p++;
			}
		}
		q -= DstRowBytes;
	}
}


HGLOBAL ResizeImage(
	const BITMAPINFO *pbmiSrc, const void *pSrcData,
	const RECT *pSrcRect, int Width, int Height)
{
	HGLOBAL hGlobal;
	BITMAPINFOHEADER *pbmihDst;
	void *pDstData;
	int *pnSrcPos;
	int SrcLeft, SrcTop, SrcWidth, SrcHeight;
	int SrcPlanes, SrcXCenter, SrcYCenter, DstXCenter, DstYCenter;
	int x, y, x1, y1, dx1, dy1, dx2, dy2;
	std::size_t YOffset;
	int b, g, r;
	std::size_t SrcRowBytes, DstPadBytes;
	const BYTE *p, *p1;
	BYTE *q;

	if (pbmiSrc->bmiHeader.biBitCount != 24 && pbmiSrc->bmiHeader.biBitCount != 32)
		return nullptr;
	hGlobal = GlobalAlloc(
		GMEM_MOVEABLE | GMEM_SHARE,
		sizeof(BITMAPINFOHEADER) + DIB_ROW_BYTES(Width, 24) * Height);
	if (hGlobal == nullptr)
		return nullptr;
	pbmihDst = (BITMAPINFOHEADER*)GlobalLock(hGlobal);
	pbmihDst->biSize = sizeof(BITMAPINFOHEADER);
	pbmihDst->biWidth = Width;
	pbmihDst->biHeight = Height;
	pbmihDst->biPlanes = 1;
	pbmihDst->biBitCount = 24;
	pbmihDst->biCompression = BI_RGB;
	pbmihDst->biSizeImage = 0;
	pbmihDst->biXPelsPerMeter = 0;
	pbmihDst->biYPelsPerMeter = 0;
	pbmihDst->biClrUsed = 0;
	pbmihDst->biClrImportant = 0;
	pDstData = pbmihDst + 1;
	if (pSrcRect != nullptr) {
		SrcLeft = pSrcRect->left;
		SrcTop = pSrcRect->top;
		SrcWidth = pSrcRect->right - pSrcRect->left;
		SrcHeight = pSrcRect->bottom - pSrcRect->top;
	} else {
		SrcLeft = SrcTop = 0;
		SrcWidth = pbmiSrc->bmiHeader.biWidth;
		SrcHeight = abs(pbmiSrc->bmiHeader.biHeight);
	}
	if (SrcWidth == Width && SrcHeight == Height) {
		CropImage(pbmiSrc, pSrcData, SrcLeft, SrcTop, Width, Height, pDstData);
		GlobalUnlock(hGlobal);
		return hGlobal;
	}
	SrcPlanes = pbmiSrc->bmiHeader.biBitCount / 8;
	pnSrcPos = new int[Width];
	SrcXCenter = ((SrcWidth - 1) << 8) / 2;
	SrcYCenter = ((SrcHeight - 1) << 8) / 2;
	DstXCenter = ((Width - 1) << 8) / 2;
	DstYCenter = ((Height - 1) << 8) / 2;
	for (x = 0; x < Width; x++) {
		pnSrcPos[x] = ((x << 8) - DstXCenter) * SrcWidth / Width + SrcXCenter;
		if (pnSrcPos[x] < 0)
			pnSrcPos[x] = 0;
		else if (pnSrcPos[x] > (SrcWidth - 1) << 8)
			pnSrcPos[x] = (SrcWidth - 1) << 8;
	}
	SrcRowBytes = DIB_ROW_BYTES(pbmiSrc->bmiHeader.biWidth, pbmiSrc->bmiHeader.biBitCount);
	DstPadBytes = DIB_ROW_BYTES(Width, 24) - Width * 3;
	if (SrcTop > 0)
		pSrcData = static_cast<const BYTE*>(pSrcData) +
			(pbmiSrc->bmiHeader.biHeight > 0 ? (pbmiSrc->bmiHeader.biHeight - (SrcHeight + SrcTop)) : SrcTop) * SrcRowBytes;
	q = (BYTE*)pDstData;
	for (y = 0; y < Height; y++) {
		y1 = ((y << 8) - DstYCenter) * SrcHeight / Height + SrcYCenter;
		if (y1 < 0)
			y1 = 0;
		else if (y1 > (SrcHeight - 1) << 8)
			y1 = (SrcHeight - 1) << 8;
		dy2 = y1 & 0xFF;
		dy1 = 0x100 - dy2;
		YOffset = dy2 > 0 ? SrcRowBytes : 0;
		for (x = 0; x < Width; x++) {
			x1 = pnSrcPos[x];
			dx2 = x1 & 0xFF;
			dx1 = 0x100 - dx2;
			p = static_cast<const BYTE*>(pSrcData) + (y1 >> 8) * SrcRowBytes +
				((x1 >> 8) + SrcLeft) * SrcPlanes;
			p1 = p + ((dx2 + 0xFF) >> 8) * SrcPlanes;
			b = (p[0] * dx1 + p1[0] * dx2) * dy1;
			g = (p[1] * dx1 + p1[1] * dx2) * dy1;
			r = (p[2] * dx1 + p1[2] * dx2) * dy1;
			p += YOffset;
			p1 += YOffset;
			b += (p[0] * dx1 + p1[0] * dx2) * dy2;
			g += (p[1] * dx1 + p1[1] * dx2) * dy2;
			r += (p[2] * dx1 + p1[2] * dx2) * dy2;
			*q++ = b >> 16;
			*q++ = g >> 16;
			*q++ = r >> 16;
		}
		q += DstPadBytes;
	}
	delete [] pnSrcPos;
	GlobalUnlock(hGlobal);
	return hGlobal;
}




CImageCodec::CImageCodec()
#ifndef TVTEST_IMAGE_STATIC
	: m_hLib(nullptr)
#endif
{
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
		m_hLib = ::LoadLibrary(TEXT("TVTest_Image.dll"));
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


HGLOBAL CImageCodec::LoadAribPngFromMemory(const void *pData, SIZE_T DataSize)
{
#ifndef TVTEST_IMAGE_STATIC
	if (m_hLib == nullptr && !Init())
		return nullptr;
	auto pLoadAribPngFromMemory =
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
	auto pLoadAribPngFromFile =
		reinterpret_cast<ImageDLL::LoadAribPngFromFileFunc>(::GetProcAddress(m_hLib, "LoadAribPngFromFile"));
	if (pLoadAribPngFromFile == nullptr)
		return nullptr;
	return pLoadAribPngFromFile(pszFileName);
#else
	return ImageLib::LoadAribPngFromFile(pszFileName);
#endif
}


}	// namespace TVTest
