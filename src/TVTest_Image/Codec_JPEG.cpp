/*
  TVTest
  Copyright(c) 2008-2017 DBCTRADO

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


#include <stdio.h>		// "jpeglib.h"で使用されている
#include <setjmp.h>
#include <windows.h>
#include <tchar.h>
#include "libjpeg/jpeglib.h"
#include "libjpeg/jerror.h"
#include "ImageLib.h"
#include "Codec_JPEG.h"
#include "ImageUtil.h"


namespace TVTest
{

namespace ImageLib
{


struct JPEGErrorInfo
{
	struct jpeg_error_mgr jerr;
	jmp_buf jmpbuf;
};

struct JPEGDestinationInfo
{
	struct jpeg_destination_mgr dest;
	HANDLE hFile;
	JOCTET *pBuffer;
};

#define JPEG_OUTPUT_BUFFER_SIZE 4096




static void JPEGErrorExit(j_common_ptr cinfo)
{
	JPEGErrorInfo *pjerrinfo = (JPEGErrorInfo*)cinfo->err;

	(*cinfo->err->output_message)(cinfo);
	longjmp(pjerrinfo->jmpbuf, 1);
}


static void JPEGErrorMessage(j_common_ptr cinfo)
{
#if 0
	//JPEGErrorInfo *pjerrinfo = (JPEGErrorInfo*)cinfo->err;
	char szText[JMSG_LENGTH_MAX];

	(*cinfo->err->format_message)(cinfo, szText);
	MessageBoxA(nullptr, szText, nullptr, MB_OK);
#endif
}


static void JPEGInitDestination(j_compress_ptr cinfo)
{
	JPEGDestinationInfo *pInfo = (JPEGDestinationInfo*)cinfo->dest;

	pInfo->pBuffer =
		(JOCTET*)(*cinfo->mem->alloc_small)(
			(j_common_ptr)cinfo,
			JPOOL_IMAGE, JPEG_OUTPUT_BUFFER_SIZE * sizeof(JOCTET));
	pInfo->dest.next_output_byte = pInfo->pBuffer;
	pInfo->dest.free_in_buffer = JPEG_OUTPUT_BUFFER_SIZE;
}


static boolean JPEGEmptyOutputBuffer(j_compress_ptr cinfo)
{
	JPEGDestinationInfo *pInfo = (JPEGDestinationInfo*)cinfo->dest;
	DWORD Write;

	if (!WriteFile(pInfo->hFile, pInfo->pBuffer, JPEG_OUTPUT_BUFFER_SIZE, &Write, nullptr)
			|| Write != JPEG_OUTPUT_BUFFER_SIZE)
		ERREXIT(cinfo, JERR_FILE_WRITE);
	pInfo->dest.next_output_byte = pInfo->pBuffer;
	pInfo->dest.free_in_buffer = JPEG_OUTPUT_BUFFER_SIZE;
	return TRUE;
}


static void JPEGTermDestination(j_compress_ptr cinfo)
{
	JPEGDestinationInfo *pInfo = (JPEGDestinationInfo*)cinfo->dest;
	DWORD Size = JPEG_OUTPUT_BUFFER_SIZE - (DWORD)pInfo->dest.free_in_buffer;

	if (Size > 0) {
		DWORD Write;

		if (!WriteFile(pInfo->hFile, pInfo->pBuffer, Size, &Write, nullptr)
				|| Write != Size)
			ERREXIT(cinfo, JERR_FILE_WRITE);
	}
	if (!FlushFileBuffers(pInfo->hFile))
		ERREXIT(cinfo, JERR_FILE_WRITE);
}


bool SaveJPEGFile(const ImageSaveInfo *pInfo)
{
	HANDLE hFile;
	struct jpeg_compress_struct jcomp;
	JPEGErrorInfo jerrinfo;
	JPEGDestinationInfo *pDstInfo;
	int Width, Height;
	volatile JSAMPROW pBuff = nullptr;
	int nBytesPerLine, y;
	const BYTE *p;

	hFile = CreateFile(
		pInfo->pszFileName, GENERIC_WRITE, 0, nullptr,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;
	jcomp.err = jpeg_std_error(&jerrinfo.jerr);
	jerrinfo.jerr.error_exit = JPEGErrorExit;
	jerrinfo.jerr.output_message = JPEGErrorMessage;
	if (setjmp(jerrinfo.jmpbuf)) {
		jpeg_destroy_compress(&jcomp);
		CloseHandle(hFile);
		if (pBuff != nullptr)
			delete [] pBuff;
		return false;
	}
	jpeg_create_compress(&jcomp);
	jcomp.dest = (struct jpeg_destination_mgr*)(jcomp.mem->alloc_small)(
		(j_common_ptr)&jcomp, JPOOL_IMAGE, sizeof(JPEGDestinationInfo));
	jcomp.dest->init_destination = JPEGInitDestination;
	jcomp.dest->empty_output_buffer = JPEGEmptyOutputBuffer;
	jcomp.dest->term_destination = JPEGTermDestination;
	pDstInfo = (JPEGDestinationInfo*)jcomp.dest;
	pDstInfo->hFile = hFile;
	Width = pInfo->pbmi->bmiHeader.biWidth;
	Height = abs(pInfo->pbmi->bmiHeader.biHeight);
	jcomp.image_width = Width;
	jcomp.image_height = Height;
	jcomp.input_components = 3;
	jcomp.in_color_space = JCS_RGB;
	jpeg_set_defaults(&jcomp);
	jpeg_set_quality(&jcomp, _ttoi(pInfo->pszOption), TRUE);
	jcomp.optimize_coding = TRUE;
	// Progressive
	//jpeg_simple_progression(&jcomp);
	jpeg_start_compress(&jcomp, TRUE);
	if (pInfo->pszComment != nullptr) {
		/* コメントの書き込み */
#ifndef UNICODE
		jpeg_write_marker(
			&jcomp, JPEG_COM, (JOCTET*)pInfo->pszComment,
			lstrlen(pInfo->pszComment));
#else
		int Length;
		LPSTR pszComment;

		Length = WideCharToMultiByte(CP_ACP, 0, pInfo->pszComment, -1, nullptr, 0, nullptr, nullptr);
		pszComment = new char[Length];
		WideCharToMultiByte(CP_ACP, 0, pInfo->pszComment, -1, pszComment, Length, nullptr, nullptr);
		jpeg_write_marker(&jcomp, JPEG_COM, (JOCTET*)pszComment, Length - 1);
		delete [] pszComment;
#endif
	}
	pBuff = new JSAMPLE[jcomp.image_width * jcomp.input_components];
	nBytesPerLine = DIB_ROW_BYTES(Width, pInfo->pbmi->bmiHeader.biBitCount);
	for (y = 0; y < Height; y++) {
		p = static_cast<const BYTE*>(pInfo->pBits) +
			(pInfo->pbmi->bmiHeader.biHeight > 0 ? (Height - 1 - y) : y) * nBytesPerLine;
		CopyToRGB24(
			pBuff, p, pInfo->pbmi->bmiHeader.biBitCount,
			pInfo->pbmi->bmiColors, Width);
		jpeg_write_scanlines(&jcomp, (JSAMPARRAY)&pBuff, 1);
	}
	delete [] pBuff;
	pBuff = nullptr;
	jpeg_finish_compress(&jcomp);
	jpeg_destroy_compress(&jcomp);
	CloseHandle(hFile);
	return true;
}


}	// namespace ImageLib

}	// namespace TVTest
