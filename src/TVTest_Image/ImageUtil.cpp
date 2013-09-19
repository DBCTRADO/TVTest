#include <windows.h>
#include "ImageUtil.h"




void CopyToRGB24(void *pDstBits,const void *pSrcBits,int nSrcBitsPerPixel,
											const RGBQUAD *prgb,int nLength)
{
	const BYTE *p;
	BYTE *q,*pbEnd;
	int i;

	p=(BYTE*)pSrcBits;
	q=(BYTE*)pDstBits;
	pbEnd=q+nLength*3;
	switch (nSrcBitsPerPixel) {
	case 1:
		{
			int nShift;

			nShift=7;
			while (q<pbEnd) {
				i=(*p>>nShift)&1;
				*q++=prgb[i].rgbRed;
				*q++=prgb[i].rgbGreen;
				*q++=prgb[i].rgbBlue;
				nShift--;
				if (nShift<0) {
					nShift=7;
					p++;
				}
			}
		}
		break;
	case 4:
		while (q<pbEnd) {
			i=*p>>4;
			*q++=prgb[i].rgbRed;
			*q++=prgb[i].rgbGreen;
			*q++=prgb[i].rgbBlue;
			if (q==pbEnd)
				break;
			i=*p++&0x0F;
			*q++=prgb[i].rgbRed;
			*q++=prgb[i].rgbGreen;
			*q++=prgb[i].rgbBlue;
		}
		break;
	case 8:
		while (q<pbEnd) {
			i=*p++;
			*q++=prgb[i].rgbRed;
			*q++=prgb[i].rgbGreen;
			*q++=prgb[i].rgbBlue;
		}
		break;
	case 24:
		while (q<pbEnd) {
			*q++=p[2];
			*q++=p[1];
			*q++=p[0];
			p+=3;
		}
		break;
	case 32:
		while (q<pbEnd) {
			*q++=p[2];
			*q++=p[1];
			*q++=p[0];
			p+=4;
		}
		break;
	}
}
