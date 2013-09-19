#pragma once


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


class CImageCodec {
	HMODULE m_hLib;
	SaveImageFunc m_pSaveImage;
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
};
