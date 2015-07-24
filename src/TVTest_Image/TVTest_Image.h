#ifndef TVTEST_IMAGE_H
#define TVTEST_IMAGE_H


#include "ImageLib.h"


typedef BOOL (WINAPI *SaveImageFunc)(const TVTest::ImageLib::ImageSaveInfo *pInfo);
typedef HGLOBAL (WINAPI *LoadAribPngFromMemoryFunc)(const void *pData,SIZE_T DataSize);
typedef HGLOBAL (WINAPI *LoadAribPngFromFileFunc)(LPCTSTR pszFileName);


#endif
