#include <windows.h>
#include <tchar.h>
#include "TVTest_Image.h"

#pragma comment(lib,"ImageLib.lib")
#pragma comment(lib,"libpng.lib")
#pragma comment(lib,"libjpeg.lib")
#pragma comment(lib,"zlib.lib")


static HINSTANCE hInst;




BOOL WINAPI DllMain(HINSTANCE hInstance,DWORD dwReason,LPVOID pvReserved)
{
	if (dwReason==DLL_PROCESS_ATTACH) {
		hInst=hInstance;
	}
	return TRUE;
}


extern "C" __declspec(dllexport) BOOL WINAPI SaveImage(const TVTest::ImageLib::ImageSaveInfo *pInfo)
{
	return TVTest::ImageLib::SaveImage(pInfo);
}


extern "C" __declspec(dllexport) HGLOBAL WINAPI LoadAribPngFromMemory(const void *pData,SIZE_T DataSize)
{
	return TVTest::ImageLib::LoadAribPngFromMemory(pData,DataSize);
}


extern "C" __declspec(dllexport) HGLOBAL WINAPI LoadAribPngFromFile(LPCTSTR pszFileName)
{
	return TVTest::ImageLib::LoadAribPngFromFile(pszFileName);
}
