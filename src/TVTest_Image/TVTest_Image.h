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


#ifndef TVTEST_IMAGE_DLL_H
#define TVTEST_IMAGE_DLL_H


#include "ImageLib.h"


namespace TVTest
{

	namespace ImageDLL
	{

		typedef BOOL (WINAPI *SaveImageFunc)(const TVTest::ImageLib::ImageSaveInfo *pInfo);
		typedef HGLOBAL (WINAPI *LoadAribPngFromMemoryFunc)(const void *pData, SIZE_T DataSize);
		typedef HGLOBAL (WINAPI *LoadAribPngFromFileFunc)(LPCTSTR pszFileName);

	}

}


#endif
