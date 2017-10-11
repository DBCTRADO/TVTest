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
