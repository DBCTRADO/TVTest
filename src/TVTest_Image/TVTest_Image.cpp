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
#include <tchar.h>
#include "TVTest_Image.h"

#pragma comment(lib,"ImageLib.lib")
#pragma comment(lib,"libpng.lib")
#pragma comment(lib,"libjpeg.lib")
#pragma comment(lib,"zlib.lib")


static HINSTANCE hInst;




BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pvReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH) {
		hInst = hInstance;
	}
	return TRUE;
}


extern "C" __declspec(dllexport) BOOL WINAPI SaveImage(const TVTest::ImageLib::ImageSaveInfo *pInfo)
{
	return TVTest::ImageLib::SaveImage(pInfo);
}


extern "C" __declspec(dllexport) HGLOBAL WINAPI LoadAribPngFromMemory(const void *pData, SIZE_T DataSize)
{
	return TVTest::ImageLib::LoadAribPngFromMemory(pData, DataSize);
}


extern "C" __declspec(dllexport) HGLOBAL WINAPI LoadAribPngFromFile(LPCTSTR pszFileName)
{
	return TVTest::ImageLib::LoadAribPngFromFile(pszFileName);
}
