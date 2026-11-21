#pragma once


#include <cstddef>
#include "../../../src/TVTest_Image/TVTest_Image.h"


// 画像クラス
class CImage
{
public:
	using ResampleType = TVTest::ImageLib::ResizeInterpolation;

	~CImage();

	bool Create(int Width, int Height, int BitsPerPixel, int AspectRatioX, int AspectRatioY);
	void Free();
	int GetWidth() const { return m_Width; }
	int GetHeight() const { return m_Height; }
	int GetDisplayWidth() const;
	int GetDisplayHeight() const { return m_Height; }
	int GetBitsPerPixel() const { return m_BitsPerPixel; }
	int GetAspectRatioX() const { return m_AspectRatioX; }
	int GetAspectRatioY() const { return m_AspectRatioY; }
	void SetFrameFlags(unsigned int Flags) { m_FrameFlags = Flags; }
	unsigned int GetFrameFlags() const { return m_FrameFlags; }
	BYTE * GetPixels() { return m_pPixels; }
	const BYTE * GetPixels() const { return m_pPixels; }
	BYTE * GetRowPixels(int y);
	const BYTE * GetRowPixels(int y) const;
	std::size_t GetRowBytes() const { return m_RowBytes; }
	bool ExtractRow24(int y, BYTE *pDest) const;
	CImage * Clone() const;
	CImage * Resize(int Width, int Height, ResampleType Resample) const;

private:
	int m_Width = 0;
	int m_Height = 0;
	int m_BitsPerPixel = 0;
	int m_AspectRatioX = 0;
	int m_AspectRatioY = 0;
	unsigned int m_FrameFlags = 0;
	BYTE *m_pPixels = nullptr;
	std::size_t m_RowBytes = 0;
};
