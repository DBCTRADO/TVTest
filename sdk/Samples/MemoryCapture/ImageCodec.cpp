#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <shlwapi.h>
#include <new>
#include "ImageCodec.h"


struct ImageSaveInfo
{
	LPCWSTR pszFileName;
	LPCWSTR pszFormat;
	LPCWSTR pszOption;
	const BITMAPINFO *pbmi;
	const void *pBits;
	LPCWSTR pszComment;
};

BOOL WINAPI SaveImage(const ImageSaveInfo *pInfo);




const LPCTSTR CImageCodec::m_FormatStringList[] =
{
	TEXT("BMP"),
	TEXT("JPEG"),
	TEXT("PNG"),
};


CImageCodec::~CImageCodec()
{
	if (m_hLib != nullptr)
		::FreeLibrary(m_hLib);
}


// 画像をファイルに保存する
bool CImageCodec::SaveImageToFile(
	const CImage *pImage, LPCWSTR pszFileName, FormatType Format)
{
	if (pImage == nullptr || pszFileName == nullptr)
		return false;

	if (m_hLib == nullptr) {
		if (!LoadModule())
			return false;
	}

	auto pSaveImage =
		reinterpret_cast<decltype(SaveImage)*>(::GetProcAddress(m_hLib, "SaveImage"));
	if (pSaveImage == nullptr)
		return false;

	const int Width = pImage->GetWidth(), Height = pImage->GetHeight();
	const std::size_t RowBytes = (Width * 3 + 3) / 4 * 4;
	BYTE *pBuffer = new(std::nothrow) BYTE[RowBytes * Height];
	if (pBuffer == nullptr)
		return false;

	for (int y = 0; y < Height; y++)
		pImage->ExtractRow24(Height - 1 - y, pBuffer + (y * RowBytes));

	ImageSaveInfo SaveInfo = {};
	BITMAPINFO bmi = {};
	WCHAR szOption[16];

	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = Width;
	bmi.bmiHeader.biHeight = Height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;

	SaveInfo.pszFileName = pszFileName;
	SaveInfo.pszFormat = m_FormatStringList[static_cast<int>(Format)];
	SaveInfo.pszOption = szOption;
	SaveInfo.pbmi = &bmi;
	SaveInfo.pBits = pBuffer;

	switch (Format) {
	case FormatType::JPEG:
		::wsprintfW(szOption, L"%d", m_JpegQuality);
		break;
	case FormatType::PNG:
		::wsprintfW(szOption, L"%d", m_PngCompressionLevel);
		break;
	default:
		szOption[0] = L'\0';
		break;
	}

	bool fResult = pSaveImage(&SaveInfo) != FALSE;

	delete [] pBuffer;

	return fResult;
}


CImageCodec::FormatType CImageCodec::ParseFormatName(LPCTSTR pszName) const
{
	for (int i = 0; i < _countof(m_FormatStringList); i++) {
		if (::lstrcmpi(m_FormatStringList[i], pszName) == 0)
			return (FormatType)i;
	}

	return FormatType::Invalid;
}


LPCWSTR CImageCodec::GetFormatExtensions(FormatType Format) const
{
	switch (Format) {
	case FormatType::BMP:  return L".bmp";
	case FormatType::JPEG: return L".jpg\0.jpeg\0.jpe";
	case FormatType::PNG:  return L".png";
	}
	return nullptr;
}


// TVTest_Image.dll を読み込む
bool CImageCodec::LoadModule()
{
	if (m_hLib == nullptr) {
		TCHAR szPath[MAX_PATH];
		DWORD Length = ::GetModuleFileName(nullptr, szPath, _countof(szPath));
		if (Length > 0 && Length < _countof(szPath)) {
			::PathRemoveFileSpec(szPath);
			if (::PathAppend(szPath, TEXT("TVTest_Image.dll")))
				m_hLib = ::LoadLibrary(szPath);
		}
	}

	return m_hLib != nullptr;
}
