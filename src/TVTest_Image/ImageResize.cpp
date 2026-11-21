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


#define NOMINMAX
#include <windows.h>
#include <memory>
#include <cmath>
#include <numbers>
#include "ImageResize.h"
#include "ImageUtil.h"


namespace TVTest
{

namespace ImageLib
{

namespace
{


constexpr BYTE Clamp8(int v)
{
	return static_cast<BYTE>(v < 0 ? 0 : v > 255 ? 255: v);
}


// 最近傍法
bool NearestNeighbor(
	const BYTE *pSrcBits, int SrcWidth, int SrcHeight, std::size_t SrcStride, int SrcBitsPerPixel,
	BYTE *pDstBits, int DstWidth, int DstHeight, std::size_t DstStride, int DstBitsPerPixel)
{
	const double XScale = static_cast<double>(SrcWidth) / static_cast<double>(DstWidth);
	const double YScale = static_cast<double>(SrcHeight) / static_cast<double>(DstHeight);
	const int SrcPlanes = SrcBitsPerPixel / 8;
	const int DstPlanes = DstBitsPerPixel / 8;

	for (int y = 0; y < DstHeight; y++) {
		const BYTE *p = pSrcBits + static_cast<int>((static_cast<double>(y) + 0.5) * YScale) * SrcStride;
		BYTE *q = pDstBits + y * DstStride;

		for (int x = 0; x < DstWidth; x++) {
			const BYTE *p1 = p + static_cast<int>((static_cast<double>(x) + 0.5) * XScale) * SrcPlanes;
			q[0] = p1[0];
			q[1] = p1[1];
			q[2] = p1[2];
			q += DstPlanes;
		}
	}

	return true;
}


// 線形補間法
bool Bilinear(
	const BYTE *pSrcBits, int SrcWidth, int SrcHeight, std::size_t SrcStride, int SrcBitsPerPixel,
	BYTE *pDstBits, int DstWidth, int DstHeight, std::size_t DstStride, int DstBitsPerPixel)
{
	std::unique_ptr<int[]> SrcPos(new(std::nothrow) int[DstWidth]);
	if (!SrcPos)
		return false;

	for (int x = 0; x < DstWidth; x++) {
		int sx = static_cast<int>(static_cast<long long>((x << 8) + 0x80) * SrcWidth / DstWidth) - 0x80;
		if (sx < 0)
			sx = 0;
		else if (sx > (SrcWidth - 1) << 8)
			sx = (SrcWidth - 1) << 8;
		SrcPos[x] = sx;
	}

	const int SrcPlanes = SrcBitsPerPixel / 8;
	const int DstPlanes = DstBitsPerPixel / 8;

	for (int y = 0; y < DstHeight; y++) {
		int y1 = static_cast<int>(static_cast<long long>((y << 8) + 0x80) * SrcHeight / DstHeight) - 0x80;
		if (y1 < 0)
			y1 = 0;
		else if (y1 > (SrcHeight - 1) << 8)
			y1 = (SrcHeight - 1) << 8;
		const int dy2 = y1 & 0xFF;
		const int dy1 = 0x100 - dy2;
		BYTE *q = pDstBits + y * DstStride;

		for (int x = 0; x < DstWidth; x++) {
			const int x1 = SrcPos[x];
			const int dx2 = x1 & 0xFF;
			const int dx1 = 0x100 - dx2;
			const BYTE *p1 = pSrcBits + ((y1 >> 8) * SrcStride) + ((x1 >> 8) * SrcPlanes);
			const BYTE *p2 = p1 + (((dx2 + 0xFF) >> 8) * SrcPlanes);
			unsigned int b = (p1[0] * dx1 + p2[0] * dx2) * dy1;
			unsigned int g = (p1[1] * dx1 + p2[1] * dx2) * dy1;
			unsigned int r = (p1[2] * dx1 + p2[2] * dx2) * dy1;
			if (dy2 != 0) {
				p1 += SrcStride;
				p2 += SrcStride;
				b += (p1[0] * dx1 + p2[0] * dx2) * dy2;
				g += (p1[1] * dx1 + p2[1] * dx2) * dy2;
				r += (p1[2] * dx1 + p2[2] * dx2) * dy2;
			}
			q[0] = static_cast<BYTE>((b + 0x8000) >> 16);
			q[1] = static_cast<BYTE>((g + 0x8000) >> 16);
			q[2] = static_cast<BYTE>((r + 0x8000) >> 16);
			q += DstPlanes;
		}
	}

	return true;
}


// 平均画素法
bool Averaging(
	const BYTE *pSrcBits, int SrcWidth, int SrcHeight, std::size_t SrcStride, int SrcBitsPerPixel,
	BYTE *pDstBits, int DstWidth, int DstHeight, std::size_t DstStride, int DstBitsPerPixel)
{
	std::unique_ptr<int[]> XScaleTable(new(std::nothrow) int[DstWidth]);
	if (!XScaleTable)
		return false;
	for (int x = 0; x < DstWidth; x++)
		XScaleTable[x] = static_cast<int>(((static_cast<long long>(x + 1) * SrcWidth) << 8) / DstWidth);

	const int SrcPlanes = SrcBitsPerPixel / 8;
	const int DstPlanes = DstBitsPerPixel / 8;

	for (int y = 0; y < DstHeight; y++) {
		const int by = static_cast<int>(((static_cast<long long>(y) * SrcHeight) << 8) / DstHeight);
		const int ey = static_cast<int>(((static_cast<long long>(y + 1) * SrcHeight) << 8) / DstHeight);
		const int by1 = by >> 8;
		const int ey1 = (ey + 255) >> 8;
		int ex = 0;
		BYTE *q = pDstBits + y * DstStride;

		for (int x = 0; x < DstWidth; x++) {
			const int bx = ex;
			ex = XScaleTable[x];
			const int bx1 = bx >> 8;
			const int ex1 = (ex + 255) >> 8;
			unsigned int r = 0, g = 0, b = 0;
			int Divisor = 0;
			const BYTE *pSrcRow = pSrcBits + (by1 * SrcStride) + (bx1 * SrcPlanes);

			for (int y1 = by1; y1 < ey1; y1++) {
				int YWeight = 256;
				if (y1 == by1)
					YWeight -= by & 0xFF;
				if (y1 == ey1 - 1)
					YWeight = (YWeight * (((ey - 1) & 0xFF) + 1)) >> 8;
				const BYTE *p = pSrcRow;

				for (int x1 = bx1; x1 < ex1; x1++) {
					int XWeight = 256;
					if (x1 == bx1)
						XWeight -= bx & 0xFF;
					if (x1 == ex1 - 1)
						XWeight = (XWeight * (((ex - 1) & 0xFF) + 1)) >> 8;
					int Weight = XWeight * YWeight;
					if (Weight > 0) {
						b += p[0] * Weight;
						g += p[1] * Weight;
						r += p[2] * Weight;
						Divisor += Weight;
					}
					p += SrcPlanes;
				}

				pSrcRow += SrcStride;
			}

			q[0] = static_cast<BYTE>((b + (Divisor >> 1)) / Divisor);
			q[1] = static_cast<BYTE>((g + (Divisor >> 1)) / Divisor);
			q[2] = static_cast<BYTE>((r + (Divisor >> 1)) / Divisor);
			q += DstPlanes;
		}
	}

	return true;
}


typedef double (*InterpolationFunc)(double x);

// 汎用再サンプリング
bool GenericResample(
	const BYTE *pSrcBits, int SrcWidth, int SrcHeight, std::size_t SrcStride, int SrcBitsPerPixel,
	BYTE *pDstBits, int DstWidth, int DstHeight, std::size_t DstStride, int DstBitsPerPixel,
	InterpolationFunc pInterpolation, double KernelSize)
{
	const std::size_t BufferRowBytes = DstWidth * 3;
	std::unique_ptr<BYTE[]> Buffer(new(std::nothrow) BYTE[BufferRowBytes * SrcHeight]);
	if (!Buffer)
		return false;

	const double XScale = static_cast<double>(SrcWidth) / static_cast<double>(DstWidth);
	const double YScale = static_cast<double>(SrcHeight) / static_cast<double>(DstHeight);
	const double XWeightScale = std::min(1.0 / XScale, 1.0);
	const double YWeightScale = std::min(1.0 / YScale, 1.0);
	const double XRadius = std::max(KernelSize / XWeightScale, 0.5);
	const double YRadius = std::max(KernelSize / YWeightScale, 0.5);

	std::unique_ptr<double[]> WeightTable(new(std::nothrow) double[static_cast<int>(std::max(XRadius, YRadius) * 2.0) + 3]);
	if (!WeightTable)
		return false;

	const int SrcPlanes = SrcBitsPerPixel / 8;
	const int DstPlanes = DstBitsPerPixel / 8;

	for (int x = 0; x < DstWidth; x++) {
		const double sx = (static_cast<double>(x) + 0.5) * XScale - 0.5;
		const int x1 = std::max(static_cast<int>(sx - XRadius + 0.5), 0);
		const int x2 = std::min(static_cast<int>(sx + XRadius + 0.5), SrcWidth - 1);
		const int Diameter = x2 - x1 + 1;
		double Weight = 0.0;

		for (int i = 0; i < Diameter; i++) {
			const double w = pInterpolation((sx - static_cast<double>(x1 + i)) * XWeightScale);
			WeightTable[i] = w;
			Weight += w;
		}
		for (int i = 0; i < Diameter; i++)
			WeightTable[i] /= Weight;

		const BYTE *p = pSrcBits + (x1 * SrcPlanes);
		BYTE *q = Buffer.get() + (x * DstPlanes);

		for (int y = 0; y < SrcHeight; y++) {
			double r = 0.0, g = 0.0, b = 0.0;

			for (int i = 0; i < Diameter; i++) {
				const double w = WeightTable[i];
				b += static_cast<double>(p[i * SrcPlanes + 0]) * w;
				g += static_cast<double>(p[i * SrcPlanes + 1]) * w;
				r += static_cast<double>(p[i * SrcPlanes + 2]) * w;
			}
			q[0] = Clamp8(static_cast<int>(b + 0.5));
			q[1] = Clamp8(static_cast<int>(g + 0.5));
			q[2] = Clamp8(static_cast<int>(r + 0.5));
			p += SrcStride;
			q += BufferRowBytes;
		}
	}

	for (int y = 0; y < DstHeight; y++) {
		const double sy = (static_cast<double>(y) + 0.5) * YScale - 0.5;
		const int y1 = std::max(static_cast<int>(sy - YRadius + 0.5), 0);
		const int y2 = std::min(static_cast<int>(sy + YRadius + 0.5), SrcHeight - 1);
		const int Diameter = y2 - y1 + 1;
		double Weight = 0.0;

		for (int i = 0; i < Diameter; i++) {
			const double w = pInterpolation((sy - static_cast<double>(y1 + i)) * YWeightScale);
			WeightTable[i] = w;
			Weight += w;
		}
		for (int i = 0; i < Diameter; i++)
			WeightTable[i] /= Weight;

		const BYTE *p = Buffer.get() + (y1 * BufferRowBytes);
		BYTE *q = pDstBits + y * DstStride;

		for (int x = 0; x < DstWidth; x++) {
			double r = 0.0, g = 0.0, b = 0.0;

			for (int i = 0; i < Diameter; i++) {
				const double w = WeightTable[i];
				b += static_cast<double>(p[i * BufferRowBytes + 0]) * w;
				g += static_cast<double>(p[i * BufferRowBytes + 1]) * w;
				r += static_cast<double>(p[i * BufferRowBytes + 2]) * w;
			}
			q[0] = Clamp8(static_cast<int>(b + 0.5));
			q[1] = Clamp8(static_cast<int>(g + 0.5));
			q[2] = Clamp8(static_cast<int>(r + 0.5));
			p += 3;
			q += DstPlanes;
		}
	}

	return true;
}


template<double Size> double Lanczos(double x)
{
	x = std::fabs(x);
	if (x < std::numeric_limits<double>::epsilon())
		return 1.0;
	if (x >= Size)
		return 0.0;
	x *= std::numbers::pi;
	return std::sin(x) * std::sin(x / Size) * Size / (x * x);
}


double Bicubic(double x)
{
	x = std::fabs(x);
	if (x < 1.0)
		return 1.0 - 2.0 * x * x + x * x * x;
	if (x < 2.0)
		return 4.0 - 8.0 * x + 5.0 * x * x - x * x * x;
	return 0.0;
}


} // namespace


HGLOBAL ResizeImage(
	const BITMAPINFO *pbmi, const void *pBits, const RECT *pRect,
	int Width, int Height, int BitsPerPixel, ResizeInterpolation Interpolation)
{
	if (pbmi == nullptr || pBits == nullptr
			|| Width <= 0 || Height <= 0
			|| (BitsPerPixel != 24 && BitsPerPixel != 32))
		return nullptr;

	const BITMAPINFOHEADER &bmih = pbmi->bmiHeader;

	if (bmih.biSize < sizeof(BITMAPINFOHEADER)
			|| (bmih.biBitCount != 24 && bmih.biBitCount != 32)
			|| bmih.biCompression != BI_RGB)
		return nullptr;

	int SrcLeft, SrcTop, SrcWidth, SrcHeight;
	if (pRect != nullptr) {
		if (pRect->left >= pRect->right || pRect->top >= pRect->bottom
				|| pRect->left < 0 || pRect->left >= bmih.biWidth
				|| pRect->top < 0 || pRect->top >= std::abs(bmih.biHeight)
				|| pRect->right > bmih.biWidth
				|| pRect->bottom > std::abs(bmih.biHeight))
			return nullptr;
		SrcLeft = pRect->left;
		SrcTop = pRect->top;
		SrcWidth = pRect->right - pRect->left;
		SrcHeight = pRect->bottom - pRect->top;
	} else {
		SrcLeft = 0;
		SrcTop = 0;
		SrcWidth = bmih.biWidth;
		SrcHeight = std::abs(bmih.biHeight);
	}

	const std::size_t DstRowBytes = DIB_ROW_BYTES(Width, BitsPerPixel);

	HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, sizeof(BITMAPINFOHEADER) + DstRowBytes * Height);
	if (hGlobal == nullptr)
		return nullptr;
	BITMAPINFOHEADER *pbmihDst = static_cast<BITMAPINFOHEADER*>(::GlobalLock(hGlobal));
	if (pbmihDst == nullptr) {
		::GlobalFree(hGlobal);
		return nullptr;
	}

	std::memcpy(pbmihDst, pbmi, sizeof(BITMAPINFOHEADER));
	pbmihDst->biSize = sizeof(BITMAPINFOHEADER);
	pbmihDst->biWidth = Width;
	pbmihDst->biHeight = bmih.biHeight > 0 ? Height : -Height;
	pbmihDst->biBitCount = BitsPerPixel;

	const std::size_t SrcRowBytes = DIB_ROW_BYTES(bmih.biWidth, bmih.biBitCount);
	const BYTE *pSrc = static_cast<const BYTE*>(pBits) +
		((bmih.biHeight > 0) ?
			bmih.biHeight - SrcTop - SrcHeight : SrcTop) * SrcRowBytes +
		SrcLeft * bmih.biBitCount / 8;
	BYTE *pDst = reinterpret_cast<BYTE*>(pbmihDst + 1);
	bool fResult = false;

	switch (Interpolation) {
	case ResizeInterpolation::NearestNeighbor:
		fResult = NearestNeighbor(
			pSrc, SrcWidth, SrcHeight, SrcRowBytes, bmih.biBitCount,
			pDst, Width, Height, DstRowBytes, BitsPerPixel);
		break;

	case ResizeInterpolation::Bilinear:
		fResult = Bilinear(
			pSrc, SrcWidth, SrcHeight, SrcRowBytes, bmih.biBitCount,
			pDst, Width, Height, DstRowBytes, BitsPerPixel);
		break;

	case ResizeInterpolation::Averaging:
		fResult = Averaging(
			pSrc, SrcWidth, SrcHeight, SrcRowBytes, bmih.biBitCount,
			pDst, Width, Height, DstRowBytes, BitsPerPixel);
		break;

	case ResizeInterpolation::Lanczos2:
		fResult = GenericResample(
			pSrc, SrcWidth, SrcHeight, SrcRowBytes, bmih.biBitCount,
			pDst, Width, Height, DstRowBytes, BitsPerPixel,
			Lanczos<2.0>, 2.0);
		break;

	case ResizeInterpolation::Lanczos3:
		fResult = GenericResample(
			pSrc, SrcWidth, SrcHeight, SrcRowBytes, bmih.biBitCount,
			pDst, Width, Height, DstRowBytes, BitsPerPixel,
			Lanczos<3.0>, 3.0);
		break;

	case ResizeInterpolation::Bicubic:
		fResult = GenericResample(
			pSrc, SrcWidth, SrcHeight, SrcRowBytes, bmih.biBitCount,
			pDst, Width, Height, DstRowBytes, BitsPerPixel,
			Bicubic, 2.0);
		break;
	}

	::GlobalUnlock(hGlobal);

	if (!fResult) {
		::GlobalFree(hGlobal);
		return nullptr;
	}

	return hGlobal;
}


} // namespace ImageLib

} // namespace TVTest
