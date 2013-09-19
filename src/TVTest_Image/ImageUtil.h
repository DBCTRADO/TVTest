#ifndef IMAGE_UTIL_H
#define IMAGE_UTIL_H


/*
#define DIB_ROW_BYTES(width,bpp) (((width)*(bpp)+31)/32*4)
*/
#define DIB_ROW_BYTES(width,bpp) ((((width)*(bpp)+31)>>5)<<2)


void CopyToRGB24(void *pDstBits,const void *pSrcBits,int nSrcBitsPerPixel,
											const RGBQUAD *prgb,int nLength);


#endif
