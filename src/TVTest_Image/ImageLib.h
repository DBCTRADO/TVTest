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
		HGLOBAL LoadAribPngFromMemory(const void *pData,SIZE_T DataSize);
		HGLOBAL LoadAribPngFromFile(LPCTSTR pszFileName);

	}
}


#endif
