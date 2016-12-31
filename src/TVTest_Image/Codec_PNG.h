#ifndef TVTEST_PNG_H
#define TVTEST_PNG_H


namespace TVTest
{
	namespace ImageLib
	{

		bool SavePNGFile(const ImageSaveInfo *pInfo);
		HGLOBAL LoadAribPng(const void *pData,SIZE_T DataSize);

	}
}


#endif
