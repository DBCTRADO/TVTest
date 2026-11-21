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


#ifndef TVTEST_IMAGE_RESIZE_H
#define TVTEST_IMAGE_RESIZE_H


namespace TVTest
{
	namespace ImageLib
	{

		enum class ResizeInterpolation
		{
			NearestNeighbor,
			Bilinear,
			Averaging,
			Lanczos2,
			Lanczos3,
			Bicubic,
			Last_ = Bicubic,
			Default_ = Lanczos2
		};

		HGLOBAL ResizeImage(
			const BITMAPINFO *pbmi, const void *pBits, const RECT *pRect,
			int Width, int Height, int BitsPerPixel, ResizeInterpolation Interpolation);

	}
}


#endif
