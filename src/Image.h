#pragma once


#ifndef TVTEST_IMAGE_STATIC
/*
	TVTEST_IMAGE_STATIC を定義すると、TVTest_Image.dll の機能が静的リンクされ
	TVTest_Image.dll を使用しなくなる。
*/
//#define TVTEST_IMAGE_STATIC
#endif

#include "TVTest_Image/TVTest_Image.h"


/*
#define DIB_ROW_BYTES(width,bpp) (((width)*(bpp)+31)/32*4)
*/
#define DIB_ROW_BYTES(width,bpp) ((((width)*(bpp)+31)>>5)<<2)


SIZE_T CalcDIBInfoSize(const BITMAPINFOHEADER *pbmih);
SIZE_T CalcDIBBitsSize(const BITMAPINFOHEADER *pbmih);
SIZE_T CalcDIBSize(const BITMAPINFOHEADER *pbmih);
HGLOBAL ResizeImage(const BITMAPINFO *pbmiSrc,const void *pSrcData,
									const RECT *pSrcRect,int Width,int Height);


class CImageCodec
{
public:
	CImageCodec();
	~CImageCodec();
	bool Init();
	bool SaveImage(LPCTSTR pszFileName,int Format,LPCTSTR pszOption,
			const BITMAPINFO *pbmi,const void *pBits,LPCTSTR pszComment=NULL);
	LPCTSTR EnumSaveFormat(int Index) const;
	LPCTSTR GetExtension(int Index) const;
	int FormatNameToIndex(LPCTSTR pszName) const;
	HGLOBAL LoadAribPngFromMemory(const void *pData,SIZE_T DataSize);
	HGLOBAL LoadAribPngFromFile(LPCTSTR pszFileName);

#ifndef TVTEST_IMAGE_STATIC
private:
	HMODULE m_hLib;
	SaveImageFunc m_pSaveImage;
#endif
};
