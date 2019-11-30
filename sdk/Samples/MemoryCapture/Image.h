#pragma once


#include <cstddef>


// 画像クラス
class CImage
{
public:
	// 再サンプリングの種類
	enum ResampleType
	{
		Resample_NearestNeighbor, // 最近傍法
		Resample_Bilinear,        // 線形補間法
		Resample_Averaging,       // 平均画素法
		Resample_Lanczos2,        // Lanczos2
		Resample_Lanczos3         // Lanczos3
	};

	CImage();
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
	const BYTE *GetPixels() const { return m_pPixels; }
	BYTE *GetRowPixels(int y);
	const BYTE *GetRowPixels(int y) const;
	std::size_t GetRowBytes() const { return m_RowBytes; }
	bool ExtractRow24(int y, BYTE *pDest) const;
	CImage *Clone() const;
	CImage *Resize(int Width, int Height, ResampleType Resample) const;

private:
	int m_Width;
	int m_Height;
	int m_BitsPerPixel;
	int m_AspectRatioX;
	int m_AspectRatioY;
	unsigned int m_FrameFlags;
	BYTE *m_pPixels;
	std::size_t m_RowBytes;

	typedef double (*KernelFunc)(double x);

	bool NearestNeighbor(CImage *pImage) const;
	bool Bilinear(CImage *pImage) const;
	bool Averaging(CImage *pImage) const;
	bool GenericResample(CImage *pImage, KernelFunc pKernelFunc, double KernelSize) const;
};
