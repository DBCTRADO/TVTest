#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <algorithm>
#include <new>
#include "Image.h"
#include "ImageCodec.h"


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

	CImageCodec Codec;
	if (!Codec.ResizeImage(this, pImage, Resample)) {
		delete pImage;
		return nullptr;
	}

	pImage->m_FrameFlags = m_FrameFlags;

	return pImage;
}
