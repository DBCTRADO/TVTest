#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include <limits>
#include <new>
#include "Image.h"


static double Lanczos2(double x)
{
	x = std::fabs(x);
	if (x < std::numeric_limits<double>::epsilon())
		return 1.0;
	if (x >= 2.0)
		return 0.0;
	x *= M_PI;
	return std::sin(x) * std::sin(x / 2.0) * 2.0 / (x * x);
}


static double Lanczos3(double x)
{
	x = std::fabs(x);
	if (x < std::numeric_limits<double>::epsilon())
		return 1.0;
	if (x >= 3.0)
		return 0.0;
	x *= M_PI;
	return std::sin(x) * std::sin(x / 3.0) * 3.0 / (x * x);
}


inline BYTE Clamp8(int v)
{
	return (BYTE)(v < 0 ? 0 : v > 255 ? 255: v);
}




CImage::~CImage()
{
	Free();
}


// 画像の作成
bool CImage::Create(int Width, int Height, int BitsPerPixel, int AspectRatioX, int AspectRatioY)
{
	Free();

	if (Width < 1 || Height < 1 || BitsPerPixel < 1)
		return false;

	std::size_t RowBytes = (Width * BitsPerPixel + 31) / 32 * 4;

	m_pPixels = new(std::nothrow) BYTE[Height * RowBytes];
	if (m_pPixels == nullptr)
		return false;

	m_Width = Width;
	m_Height = Height;
	m_BitsPerPixel = BitsPerPixel;
	m_AspectRatioX = AspectRatioX;
	m_AspectRatioY = AspectRatioY;
	m_FrameFlags = 0;
	m_RowBytes = RowBytes;

	return true;
}


// 画像の解放
void CImage::Free()
{
	if (m_pPixels != nullptr) {
		delete [] m_pPixels;
		m_pPixels = nullptr;
	}

	m_Width = 0;
	m_Height = 0;
	m_BitsPerPixel = 0;
	m_AspectRatioX = 0;
	m_AspectRatioY = 0;
	m_FrameFlags = 0;
	m_RowBytes = 0;
}


// アスペクト比を反映させた幅を取得
int CImage::GetDisplayWidth() const
{
	if (m_AspectRatioX <= 1 || m_AspectRatioY <= 1)
		return m_Width;
	return ::MulDiv(m_Height, m_AspectRatioX, m_AspectRatioY);
}


BYTE * CImage::GetRowPixels(int y)
{
	return m_pPixels + y * m_RowBytes;
}


const BYTE * CImage::GetRowPixels(int y) const
{
	return m_pPixels + y * m_RowBytes;
}


bool CImage::ExtractRow24(int y, BYTE *pDest) const
{
	if (m_pPixels == nullptr || y < 0 || y >= m_Height || pDest == nullptr)
		return false;

	const int Width = m_Width;
	const BYTE *p = GetRowPixels(y);
	BYTE *q = pDest;

	if (m_BitsPerPixel == 24) {
		std::memcpy(q, p, Width * 3);
	} else if (m_BitsPerPixel == 32) {
		for (int x = 0; x < Width; x++) {
			*q++ = p[0];
			*q++ = p[1];
			*q++ = p[2];
			p += 4;
		}
	} else {
		return false;
	}

	return true;
}


// 画像の複製
CImage * CImage::Clone() const
{
	if (m_pPixels == nullptr)
		return nullptr;

	CImage *pImage = new CImage;

	if (!pImage->Create(m_Width, m_Height, m_BitsPerPixel, m_AspectRatioX, m_AspectRatioY)) {
		delete pImage;
		return nullptr;
	}

	std::memcpy(pImage->m_pPixels, m_pPixels, m_Height * m_RowBytes);

	pImage->m_FrameFlags = m_FrameFlags;

	return pImage;
}


// 画像のリサイズ
CImage * CImage::Resize(int Width, int Height, ResampleType Resample) const
{
	if (m_pPixels == nullptr)
		return nullptr;

	if (Width == m_Width && Height == m_Height)
		return Clone();

	CImage *pImage = new CImage;

	if (!pImage->Create(Width, Height, m_BitsPerPixel, m_AspectRatioX, m_AspectRatioY)) {
		delete pImage;
		return nullptr;
	}

	bool fResult;

	switch (Resample) {
	case ResampleType::NearestNeighbor:
		fResult = NearestNeighbor(pImage);
		break;

	case ResampleType::Bilinear:
	default:
		fResult = Bilinear(pImage);
		break;

	case ResampleType::Averaging:
		fResult = Averaging(pImage);
		break;

	case ResampleType::Lanczos2:
		fResult = GenericResample(pImage, Lanczos2, 2.0);
		break;

	case ResampleType::Lanczos3:
		fResult = GenericResample(pImage, Lanczos3, 3.0);
		break;
	}

	if (!fResult) {
		delete pImage;
		return nullptr;
	}

	pImage->m_FrameFlags = m_FrameFlags;

	return pImage;
}


// 最近傍法
bool CImage::NearestNeighbor(CImage *pImage) const
{
	if ((m_BitsPerPixel != 24 && m_BitsPerPixel != 32)
			|| (pImage->m_BitsPerPixel != 24 && pImage->m_BitsPerPixel != 32))
		return false;

	const int OldWidth = m_Width, OldHeight = m_Height;
	const int NewWidth = pImage->m_Width, NewHeight = pImage->m_Height;
	const double XScale = (double)OldWidth / (double)NewWidth;
	const double YScale = (double)OldHeight / (double)NewHeight;
	const int SrcPlanes = m_BitsPerPixel / 8;
	const int DstPlanes = pImage->m_BitsPerPixel / 8;

	for (int y = 0; y < NewHeight; y++) {
		const BYTE *p = GetRowPixels((int)(((double)y + 0.5) * YScale));
		BYTE *q = pImage->GetRowPixels(y);

		for (int x = 0; x < NewWidth; x++) {
			const BYTE *p1 = p + ((int)(((double)x + 0.5) * XScale) * SrcPlanes);
			q[0] = p1[0];
			q[1] = p1[1];
			q[2] = p1[2];
			q += DstPlanes;
		}
	}

	return true;
}


// 線形補間法
bool CImage::Bilinear(CImage *pImage) const
{
	if ((m_BitsPerPixel != 24 && m_BitsPerPixel != 32)
			|| (pImage->m_BitsPerPixel != 24 && pImage->m_BitsPerPixel != 32))
		return false;

	const int OldWidth = m_Width, OldHeight = m_Height;
	const int NewWidth = pImage->m_Width, NewHeight = pImage->m_Height;

	int *pSrcPos = new(std::nothrow) int[NewWidth];
	if (pSrcPos == nullptr)
		return false;

	for (int x = 0; x < NewWidth; x++) {
		int sx = ((x << 8) + 0x80) * OldWidth / NewWidth - 0x80;
		if (sx < 0)
			sx = 0;
		else if (sx > (OldWidth - 1) << 8)
			sx = (OldWidth - 1) << 8;
		pSrcPos[x] = sx;
	}

	const int SrcPlanes = m_BitsPerPixel / 8;
	const int DstPlanes = pImage->m_BitsPerPixel / 8;

	for (int y = 0; y < NewHeight; y++) {
		int y1 = ((y << 8) + 0x80) * OldHeight / NewHeight - 0x80;
		if (y1 < 0)
			y1 = 0;
		else if (y1 > (OldHeight - 1) << 8)
			y1 = (OldHeight - 1) << 8;
		int dy2 = y1 & 0xFF;
		int dy1 = 0x100 - dy2;
		BYTE *q = pImage->GetRowPixels(y);

		for (int x = 0; x < NewWidth; x++) {
			int x1 = pSrcPos[x];
			int dx2 = x1 & 0xFF;
			int dx1 = 0x100 - dx2;
			const BYTE *p1 = m_pPixels + ((y1 >> 8) * m_RowBytes) + ((x1 >> 8) * SrcPlanes);
			const BYTE *p2 = p1 + (((dx2 + 0xFF) >> 8) * SrcPlanes);
			int b = (p1[0] * dx1 + p2[0] * dx2) * dy1;
			int g = (p1[1] * dx1 + p2[1] * dx2) * dy1;
			int r = (p1[2] * dx1 + p2[2] * dx2) * dy1;
			if (dy2 != 0) {
				p1 += m_RowBytes;
				p2 += m_RowBytes;
				b += (p1[0] * dx1 + p2[0] * dx2) * dy2;
				g += (p1[1] * dx1 + p2[1] * dx2) * dy2;
				r += (p1[2] * dx1 + p2[2] * dx2) * dy2;
			}
			q[0] = (BYTE)((b + 0x8000) >> 16);
			q[1] = (BYTE)((g + 0x8000) >> 16);
			q[2] = (BYTE)((r + 0x8000) >> 16);
			q += DstPlanes;
		}
	}

	delete [] pSrcPos;

	return true;
}


// 平均画素法
bool CImage::Averaging(CImage *pImage) const
{
	if ((m_BitsPerPixel != 24 && m_BitsPerPixel != 32)
			|| (pImage->m_BitsPerPixel != 24 && pImage->m_BitsPerPixel != 32))
		return false;

	const int OldWidth = m_Width, OldHeight = m_Height;
	const int NewWidth = pImage->m_Width, NewHeight = pImage->m_Height;

	int *pXScaleTable = new(std::nothrow) int[NewWidth];
	if (pXScaleTable == nullptr)
		return false;
	for (int x = 0; x < NewWidth; x++)
		pXScaleTable[x] = (int)((((long long)(x + 1) * OldWidth) << 8) / (long long)NewWidth);

	const int SrcPlanes = m_BitsPerPixel / 8;
	const int DstPlanes = pImage->m_BitsPerPixel / 8;

	for (int y = 0; y < NewHeight; y++) {
		int by = (int)((((long long)y * OldHeight) << 8) / (long long)NewHeight);
		int ey = (int)((((long long)(y + 1) * OldHeight) << 8) / (long long)NewHeight);
		int by1 = by >> 8;
		int ey1 = (ey + 255) >> 8;
		int ex = 0;
		BYTE *q = pImage->GetRowPixels(y);

		for (int x = 0; x < NewWidth; x++) {
			int bx = ex;
			ex = pXScaleTable[x];
			int bx1 = bx >> 8;
			int ex1 = (ex + 255) >> 8;
			int r = 0, g = 0, b = 0;
			int Divisor = 0;
			const BYTE *pSrcRow = m_pPixels + (by1 * m_RowBytes) + (bx1 * SrcPlanes);

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

				pSrcRow += m_RowBytes;
			}

			q[0] = (BYTE)((b + (Divisor >> 1)) / Divisor);
			q[1] = (BYTE)((g + (Divisor >> 1)) / Divisor);
			q[2] = (BYTE)((r + (Divisor >> 1)) / Divisor);
			q += DstPlanes;
		}
	}

	return true;
}


// 汎用再サンプリング
bool CImage::GenericResample(CImage *pImage, KernelFunc pKernelFunc, double KernelSize) const
{
	if ((m_BitsPerPixel != 24 && m_BitsPerPixel != 32)
			|| (pImage->m_BitsPerPixel != 24 && pImage->m_BitsPerPixel != 32))
		return false;

	const int OldWidth = m_Width, OldHeight = m_Height;
	const int NewWidth = pImage->m_Width, NewHeight = pImage->m_Height;
	const std::size_t SrcRowBytes = m_RowBytes;
	const std::size_t BufferRowBytes = pImage->m_RowBytes;
	BYTE *pBuffer = new(std::nothrow) BYTE[OldHeight * BufferRowBytes];
	if (pBuffer == nullptr)
		return false;

	const double XScale = (double)OldWidth / (double)NewWidth;
	const double YScale = (double)OldHeight / (double)NewHeight;
	const double XWeightScale = std::min(1.0 / XScale, 1.0);
	const double YWeightScale = std::min(1.0 / YScale, 1.0);
	const double XRadius = std::max(KernelSize / XWeightScale, 0.5);
	const double YRadius = std::max(KernelSize / YWeightScale, 0.5);
	const int SrcPlanes = m_BitsPerPixel / 8;
	const int DstPlanes = pImage->m_BitsPerPixel / 8;

	double *pWeightTable = new(std::nothrow) double[(int)(std::max(XRadius, YRadius) * 2.0) + 3];
	if (pWeightTable == nullptr) {
		delete [] pBuffer;
		return false;
	}

	for (int x = 0; x < NewWidth; x++) {
		const double sx = ((double)x + 0.5) * XScale - 0.5;
		const int x1 = std::max((int)(sx - XRadius + 0.5), 0);
		const int x2 = std::min((int)(sx + XRadius + 0.5), OldWidth - 1);
		const int Diameter = x2 - x1 + 1;
		double Weight = 0.0;

		for (int i = 0; i < Diameter; i++) {
			double w = pKernelFunc((sx - (double)(x1 + i)) * XWeightScale);
			pWeightTable[i] = w;
			Weight += w;
		}
		for (int i = 0; i < Diameter; i++)
			pWeightTable[i] /= Weight;

		const BYTE *p = m_pPixels + (x1 * SrcPlanes);
		BYTE *q = pBuffer + (x * DstPlanes);

		for (int y = 0; y < OldHeight; y++) {
			double r = 0.0, g = 0.0, b = 0.0;

			for (int i = 0; i < Diameter; i++) {
				double w = pWeightTable[i];
				b += (double)p[i * SrcPlanes + 0] * w;
				g += (double)p[i * SrcPlanes + 1] * w;
				r += (double)p[i * SrcPlanes + 2] * w;
			}
			q[0] = Clamp8((int)(b + 0.5));
			q[1] = Clamp8((int)(g + 0.5));
			q[2] = Clamp8((int)(r + 0.5));
			p += SrcRowBytes;
			q += BufferRowBytes;
		}
	}

	for (int y = 0; y < NewHeight; y++) {
		const double sy = ((double)y + 0.5) * YScale - 0.5;
		const int y1 = std::max((int)(sy - YRadius + 0.5), 0);
		const int y2 = std::min((int)(sy + YRadius + 0.5), OldHeight - 1);
		const int Diameter = y2 - y1 + 1;
		double Weight = 0.0;

		for (int i = 0; i < Diameter; i++) {
			double w = pKernelFunc((sy - (double)(y1 + i)) * YWeightScale);
			pWeightTable[i] = w;
			Weight += w;
		}
		for (int i = 0; i < Diameter; i++)
			pWeightTable[i] /= Weight;

		const BYTE *p = pBuffer + (y1 * BufferRowBytes);
		BYTE *q = pImage->GetRowPixels(y);

		for (int x = 0; x < NewWidth; x++) {
			double r = 0.0, g = 0.0, b = 0.0;

			for (int i = 0; i < Diameter; i++) {
				double w = pWeightTable[i];
				b += (double)p[i * BufferRowBytes + 0] * w;
				g += (double)p[i * BufferRowBytes + 1] * w;
				r += (double)p[i * BufferRowBytes + 2] * w;
			}
			q[0] = Clamp8((int)(b + 0.5));
			q[1] = Clamp8((int)(g + 0.5));
			q[2] = Clamp8((int)(r + 0.5));
			p += SrcPlanes;
			q += DstPlanes;
		}
	}

	delete [] pWeightTable;
	delete [] pBuffer;

	return true;
}
