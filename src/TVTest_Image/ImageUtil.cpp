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


#include <windows.h>
#include "ImageUtil.h"


namespace TVTest
{


void CopyToRGB24(
	void *pDstBits, const void *pSrcBits, int SrcBitsPerPixel,
	const RGBQUAD *prgb, int Length)
{
	const BYTE *p = static_cast<const BYTE*>(pSrcBits);
	BYTE *q = static_cast<BYTE*>(pDstBits);
	const BYTE *pEnd = q + Length * 3;

	switch (SrcBitsPerPixel) {
	case 1:
		{
			int Shift = 7;

			while (q < pEnd) {
				const int i = (*p >> Shift) & 1;
				*q++ = prgb[i].rgbRed;
				*q++ = prgb[i].rgbGreen;
				*q++ = prgb[i].rgbBlue;
				Shift--;
				if (Shift < 0) {
					Shift = 7;
					p++;
				}
			}
		}
		break;
	case 4:
		while (q < pEnd) {
			int i = *p >> 4;
			*q++ = prgb[i].rgbRed;
			*q++ = prgb[i].rgbGreen;
			*q++ = prgb[i].rgbBlue;
			if (q == pEnd)
				break;
			i = *p++ & 0x0F;
			*q++ = prgb[i].rgbRed;
			*q++ = prgb[i].rgbGreen;
			*q++ = prgb[i].rgbBlue;
		}
		break;
	case 8:
		while (q < pEnd) {
			const int i = *p++;
			*q++ = prgb[i].rgbRed;
			*q++ = prgb[i].rgbGreen;
			*q++ = prgb[i].rgbBlue;
		}
		break;
	case 24:
		while (q < pEnd) {
			*q++ = p[2];
			*q++ = p[1];
			*q++ = p[0];
			p += 3;
		}
		break;
	case 32:
		while (q < pEnd) {
			*q++ = p[2];
			*q++ = p[1];
			*q++ = p[0];
			p += 4;
		}
		break;
	}
}


} // namespace TVTest
