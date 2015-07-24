#include <windows.h>
#include <tchar.h>
#include "ImageLib.h"
#include "Codec_BMP.h"
#include "Codec_JPEG.h"
#include "Codec_PNG.h"


namespace TVTest
{

namespace ImageLib
{


bool SaveImage(const ImageSaveInfo *pInfo)
{
	if (pInfo==nullptr)
		return false;

	bool fResult;

	if (::lstrcmpi(pInfo->pszFormat,TEXT("BMP"))==0) {
		fResult=SaveBMPFile(pInfo);
	} else if (::lstrcmpi(pInfo->pszFormat,TEXT("JPEG"))==0) {
		fResult=SaveJPEGFile(pInfo);
	} else if (::lstrcmpi(pInfo->pszFormat,TEXT("PNG"))==0) {
		fResult=SavePNGFile(pInfo);
	} else {
		fResult=false;
	}

	return fResult;
}


HGLOBAL LoadAribPngFromMemory(const void *pData,SIZE_T DataSize)
{
	return LoadAribPng(pData,DataSize);
}


HGLOBAL LoadAribPngFromFile(LPCTSTR pszFileName)
{
	HANDLE hFile=::CreateFile(pszFileName,GENERIC_READ,FILE_SHARE_READ,nullptr,
							  OPEN_EXISTING,0,nullptr);
	if (hFile==INVALID_HANDLE_VALUE)
		return nullptr;
	LARGE_INTEGER FileSize;
	if (!::GetFileSizeEx(hFile,&FileSize)) {
		::CloseHandle(hFile);
		return nullptr;
	}
	if (FileSize.QuadPart>4*1024*1024) {
		::CloseHandle(hFile);
		return nullptr;
	}
	BYTE *pData=new BYTE[FileSize.LowPart];
	DWORD Read;
	if (!::ReadFile(hFile,pData,FileSize.LowPart,&Read,nullptr) || Read!=FileSize.LowPart) {
		delete [] pData;
		::CloseHandle(hFile);
		return nullptr;
	}
	::CloseHandle(hFile);
	HGLOBAL hDIB=LoadAribPng(pData,FileSize.LowPart);
	delete [] pData;
	return hDIB;
}


}	// namespace ImageLib

}	// namespace TVTest
