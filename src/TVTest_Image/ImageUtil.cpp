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

namespace
{


template<int R, int G, int B> void CopyPixelsToRGB24(
	void *pDstBits, const void *pSrcBits, int SrcBitsPerPixel,
	const RGBQUAD *pPalette, int Length)
{
	const BYTE *p = static_cast<const BYTE*>(pSrcBits);
	BYTE *q = static_cast<BYTE*>(pDstBits);

	switch (SrcBitsPerPixel) {
	case 1:
		{
			int Shift = 7;

			for (int x = 0; x < Length; x++) {
				const int i = (*p >> Shift) & 1;
				q[R] = pPalette[i].rgbRed;
				q[G] = pPalette[i].rgbGreen;
				q[B] = pPalette[i].rgbBlue;
				q += 3;
				Shift--;
				if (Shift < 0) {
					Shift = 7;
					p++;
				}
			}
		}
		break;

	case 4:
		for (int x = 0; x < Length; x++) {
			int i = *p >> 4;
			q[R] = pPalette[i].rgbRed;
			q[G] = pPalette[i].rgbGreen;
			q[B] = pPalette[i].rgbBlue;
			q += 3;
			x++;
			if (x == Length)
				break;
			i = *p++ & 0x0F;
			q[R] = pPalette[i].rgbRed;
			q[G] = pPalette[i].rgbGreen;
			q[B] = pPalette[i].rgbBlue;
			q += 3;
		}
		break;

	case 8:
		for (int x = 0; x < Length; x++) {
			const int i = *p++;
			q[R] = pPalette[i].rgbRed;
			q[G] = pPalette[i].rgbGreen;
			q[B] = pPalette[i].rgbBlue;
			q += 3;
		}
		break;

	case 24:
		for (int x = 0; x < Length; x++) {
			q[R] = p[2];
			q[G] = p[1];
			q[B] = p[0];
			p += 3;
			q += 3;
		}
		break;

	case 32:
		for (int x = 0; x < Length; x++) {
			q[R] = p[2];
			q[G] = p[1];
			q[B] = p[0];
			p += 4;
			q += 3;
		}
		break;
	}
}


template<int R, int G, int B, int A> void CopyPixelsToRGBA32(
	void *pDstBits, const void *pSrcBits, int SrcBitsPerPixel,
	const RGBQUAD *pPalette, int Length)
{
	const BYTE *p = static_cast<const BYTE*>(pSrcBits);
	BYTE *q = static_cast<BYTE*>(pDstBits);

	switch (SrcBitsPerPixel) {
	case 1:
		{
			int Shift = 7;

			for (int x = 0; x < Length; x++) {
				const int i = (*p >> Shift) & 1;
				q[R] = pPalette[i].rgbRed;
				q[G] = pPalette[i].rgbGreen;
				q[B] = pPalette[i].rgbBlue;
				q[A] = 0xFF;
				q += 4;
				Shift--;
				if (Shift < 0) {
					Shift = 7;
					p++;
				}
			}
		}
		break;

	case 4:
		for (int x = 0; x < Length; x++) {
			int i = *p >> 4;
			q[R] = pPalette[i].rgbRed;
			q[G] = pPalette[i].rgbGreen;
			q[B] = pPalette[i].rgbBlue;
			q[A] = 0xFF;
			q += 4;
			x++;
			if (x == Length)
				break;
			i = *p++ & 0x0F;
			q[R] = pPalette[i].rgbRed;
			q[G] = pPalette[i].rgbGreen;
			q[B] = pPalette[i].rgbBlue;
			q[A] = 0xFF;
			q += 4;
		}
		break;

	case 8:
		for (int x = 0; x < Length; x++) {
			const int i = *p++;
			q[R] = pPalette[i].rgbRed;
			q[G] = pPalette[i].rgbGreen;
			q[B] = pPalette[i].rgbBlue;
			q[A] = 0xFF;
			q += 4;
		}
		break;

	case 24:
		for (int x = 0; x < Length; x++) {
			q[R] = p[2];
			q[G] = p[1];
			q[B] = p[0];
			q[A] = 0xFF;
			p += 3;
			q += 4;
		}
		break;

	case 32:
		for (int x = 0; x < Length; x++) {
			q[R] = p[2];
			q[G] = p[1];
			q[B] = p[0];
			q[A] = p[3];
			p += 4;
			q += 4;
		}
		break;
	}
}


} // namespace


void CopyPixels(
	void *pDstBits, int DstBitsPerPixel,
	const void *pSrcBits, int SrcBitsPerPixel,
	const RGBQUAD *pPalette, int Length)
{
	switch (DstBitsPerPixel) {
	case 24:
		CopyPixelsToRGB24<2, 1, 0>(pDstBits, pSrcBits, SrcBitsPerPixel, pPalette, Length);
		break;

	case 32:
		CopyPixelsToRGBA32<2, 1, 0, 3>(pDstBits, pSrcBits, SrcBitsPerPixel, pPalette, Length);
		break;
	}
}


void CopyToRGB24(
	void *pDstBits, const void *pSrcBits, int SrcBitsPerPixel,
	const RGBQUAD *pPalette, int Length)
{
	CopyPixelsToRGB24<0, 1, 2>(pDstBits, pSrcBits, SrcBitsPerPixel, pPalette, Length);
}


void CopyToRGBA32(
	void *pDstBits, const void *pSrcBits, int SrcBitsPerPixel,
	const RGBQUAD *pPalette, int Length)
{
	CopyPixelsToRGBA32<0, 1, 2, 3>(pDstBits, pSrcBits, SrcBitsPerPixel, pPalette, Length);
}


} // namespace TVTest
