/*
  TVTest
  Copyright(c) 2008-2019 DBCTRADO

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


#include <windows.h>
#include "ImageUtil.h"


namespace TVTest
{


void CopyToRGB24(
	void *pDstBits, const void *pSrcBits, int nSrcBitsPerPixel,
	const RGBQUAD *prgb, int nLength)
{
	const BYTE *p;
	BYTE *q, *pbEnd;
	int i;

	p = (BYTE*)pSrcBits;
	q = (BYTE*)pDstBits;
	pbEnd = q + nLength * 3;

	switch (nSrcBitsPerPixel) {
	case 1:
		{
			int nShift;

			nShift = 7;
			while (q < pbEnd) {
				i = (*p >> nShift) & 1;
				*q++ = prgb[i].rgbRed;
				*q++ = prgb[i].rgbGreen;
				*q++ = prgb[i].rgbBlue;
				nShift--;
				if (nShift < 0) {
					nShift = 7;
					p++;
				}
			}
		}
		break;
	case 4:
		while (q < pbEnd) {
			i = *p >> 4;
			*q++ = prgb[i].rgbRed;
			*q++ = prgb[i].rgbGreen;
			*q++ = prgb[i].rgbBlue;
			if (q == pbEnd)
				break;
			i = *p++ & 0x0F;
			*q++ = prgb[i].rgbRed;
			*q++ = prgb[i].rgbGreen;
			*q++ = prgb[i].rgbBlue;
		}
		break;
	case 8:
		while (q < pbEnd) {
			i = *p++;
			*q++ = prgb[i].rgbRed;
			*q++ = prgb[i].rgbGreen;
			*q++ = prgb[i].rgbBlue;
		}
		break;
	case 24:
		while (q < pbEnd) {
			*q++ = p[2];
			*q++ = p[1];
			*q++ = p[0];
			p += 3;
		}
		break;
	case 32:
		while (q < pbEnd) {
			*q++ = p[2];
			*q++ = p[1];
			*q++ = p[0];
			p += 4;
		}
		break;
	}
}


}	// namespace TVTest
