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


#ifndef TVTEST_IMAGE_LIB_H
#define TVTEST_IMAGE_LIB_H


namespace TVTest
{
	namespace ImageLib
	{

		struct ImageSaveInfo
		{
			LPCTSTR pszFileName;
			LPCTSTR pszFormat;
			LPCTSTR pszOption;
			const BITMAPINFO *pbmi;
			const void *pBits;
			LPCTSTR pszComment;
		};

		bool SaveImage(const ImageSaveInfo *pInfo);
		HGLOBAL LoadAribPngFromMemory(const void *pData, size_t DataSize);
		HGLOBAL LoadAribPngFromFile(LPCTSTR pszFileName);

	}
}


#endif
