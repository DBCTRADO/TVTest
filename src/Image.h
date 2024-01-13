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


#ifndef TVTEST_IMAGE_H
#define TVTEST_IMAGE_H


#ifndef TVTEST_IMAGE_STATIC
/*
	TVTEST_IMAGE_STATIC を定義すると、TVTest_Image.dll の機能が静的リンクされ
	TVTest_Image.dll を使用しなくなる。
*/
//#define TVTEST_IMAGE_STATIC
#endif

#include "TVTest_Image/TVTest_Image.h"


namespace TVTest
{

/*
	constexpr std::size_t DIB_ROW_BYTES(int width, int bpp) { return ((width * bpp) + 31) / 32 * 4; }
*/
	constexpr std::size_t DIB_ROW_BYTES(int width, int bpp) { return ((width * bpp + 31) >> 5) << 2; }


	size_t CalcDIBInfoSize(const BITMAPINFOHEADER *pbmih);
	size_t CalcDIBBitsSize(const BITMAPINFOHEADER *pbmih);
	size_t CalcDIBSize(const BITMAPINFOHEADER *pbmih);
	HGLOBAL ResizeImage(
		const BITMAPINFO *pbmiSrc, const void *pSrcData,
		const RECT *pSrcRect, int Width, int Height);


	class CImageCodec
	{
	public:
		CImageCodec() = default;
		~CImageCodec();

		CImageCodec(const CImageCodec &) = delete;
		CImageCodec &operator=(const CImageCodec &) = delete;

		bool Init();
		bool SaveImage(
			LPCTSTR pszFileName, int Format, LPCTSTR pszOption,
			const BITMAPINFO *pbmi, const void *pBits, LPCTSTR pszComment = nullptr);
		LPCTSTR EnumSaveFormat(int Index) const;
		LPCTSTR GetExtension(int Index) const;
		int FormatNameToIndex(LPCTSTR pszName) const;
		HGLOBAL LoadAribPngFromMemory(const void *pData, size_t DataSize);
		HGLOBAL LoadAribPngFromFile(LPCTSTR pszFileName);

#ifndef TVTEST_IMAGE_STATIC
	private:
		HMODULE m_hLib = nullptr;
		ImageDLL::SaveImageFunc m_pSaveImage = nullptr;
#endif
	};

} // namespace TVTest


#endif
