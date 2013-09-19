#include <windows.h>
#include <tchar.h>
#include "TVTest_Image.h"
#include "BMP.h"
#include "JPEG.h"
#include "PNG.h"


static HINSTANCE hInst;




BOOL WINAPI DllMain(HINSTANCE hInstance,DWORD dwReason,LPVOID pvReserved)
{
	if (dwReason==DLL_PROCESS_ATTACH) {
		hInst=hInstance;
	}
	return TRUE;
}


extern "C" __declspec(dllexport) BOOL WINAPI SaveImage(const ImageSaveInfo *pInfo)
{
	BOOL fResult;

	if (lstrcmpi(pInfo->pszFormat,TEXT("BMP"))==0) {
		fResult=SaveBMPFile(pInfo);
	} else if (lstrcmpi(pInfo->pszFormat,TEXT("JPEG"))==0) {
		fResult=SaveJPEGFile(pInfo);
	} else if (lstrcmpi(pInfo->pszFormat,TEXT("PNG"))==0) {
		fResult=SavePNGFile(pInfo);
	} else
		return FALSE;
	return fResult;
}


extern "C" __declspec(dllexport) HGLOBAL WINAPI LoadAribPngFromMemory(const void *pData,SIZE_T DataSize)
{
	return LoadAribPng(pData,DataSize);
}


extern "C" __declspec(dllexport) HGLOBAL WINAPI LoadAribPngFromFile(LPCTSTR pszFileName)
{
	HANDLE hFile=::CreateFile(pszFileName,GENERIC_READ,FILE_SHARE_READ,NULL,
							  OPEN_EXISTING,0,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return NULL;
	LARGE_INTEGER FileSize;
	if (!::GetFileSizeEx(hFile,&FileSize)) {
		::CloseHandle(hFile);
		return NULL;
	}
	if (FileSize.QuadPart>4*1024*1024) {
		::CloseHandle(hFile);
		return NULL;
	}
	BYTE *pData=new BYTE[FileSize.LowPart];
	DWORD Read;
	if (!::ReadFile(hFile,pData,FileSize.LowPart,&Read,NULL) || Read!=FileSize.LowPart) {
		delete [] pData;
		::CloseHandle(hFile);
		return NULL;
	}
	::CloseHandle(hFile);
	HGLOBAL hDIB=LoadAribPng(pData,FileSize.LowPart);
	delete [] pData;
	return hDIB;
}
