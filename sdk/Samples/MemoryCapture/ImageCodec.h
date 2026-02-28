#pragma once


#include "Image.h"


// 画像コーデッククラス
class CImageCodec
{
public:
	enum class FormatType
	{
		Invalid = -1,
		BMP,
		JPEG,
		PNG,
	};

	~CImageCodec();

	bool SaveImageToFile(const CImage *pImage, LPCWSTR pszFileName, FormatType Format);
	FormatType ParseFormatName(LPCTSTR pszName) const;
	LPCWSTR GetFormatExtensions(FormatType Format) const;
	void SetJpegQuality(int Quality) { m_JpegQuality = Quality; }
	void SetPngCompressionLevel(int Level) { m_PngCompressionLevel = Level; }

private:
	HMODULE m_hLib = nullptr;
	int m_JpegQuality = 90;
	int m_PngCompressionLevel = 6;

	static const LPCTSTR m_FormatStringList[];

	bool LoadModule();
};
