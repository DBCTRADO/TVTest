/*
  TVTest
  Copyright(c) 2008-2020 DBCTRADO

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
#include <memory>
#include "libjpeg/jpeglib.h"
#include "libjpeg/jerror.h"
#include "ImageLib.h"
#include "Codec_JPEG.h"
#include "ImageUtil.h"


namespace TVTest
{

namespace ImageLib
{


struct JPEGDestinationInfo
{
	jpeg_destination_mgr dest;
	HANDLE hFile;
	JOCTET *pBuffer;
};

#define JPEG_OUTPUT_BUFFER_SIZE 4096




static void JPEGErrorExit(j_common_ptr cinfo)
{
	(*cinfo->err->output_message)(cinfo);
	throw cinfo;
}


static void JPEGErrorMessage(j_common_ptr cinfo)
{
#if 0
	char szText[JMSG_LENGTH_MAX];

	(*cinfo->err->format_message)(cinfo, szText);
	MessageBoxA(nullptr, szText, nullptr, MB_OK);
#endif
}


static void JPEGInitDestination(j_compress_ptr cinfo)
{
	JPEGDestinationInfo *pInfo = reinterpret_cast<JPEGDestinationInfo*>(cinfo->dest);

	pInfo->pBuffer =
		static_cast<JOCTET*>((*cinfo->mem->alloc_small)(
			reinterpret_cast<j_common_ptr>(cinfo),
			JPOOL_IMAGE, JPEG_OUTPUT_BUFFER_SIZE * sizeof(JOCTET)));
	pInfo->dest.next_output_byte = pInfo->pBuffer;
	pInfo->dest.free_in_buffer = JPEG_OUTPUT_BUFFER_SIZE;
}


static boolean JPEGEmptyOutputBuffer(j_compress_ptr cinfo)
{
	JPEGDestinationInfo *pInfo = reinterpret_cast<JPEGDestinationInfo*>(cinfo->dest);
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
	JPEGDestinationInfo *pInfo = reinterpret_cast<JPEGDestinationInfo*>(cinfo->dest);
	const DWORD Size = JPEG_OUTPUT_BUFFER_SIZE - static_cast<DWORD>(pInfo->dest.free_in_buffer);

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
	const HANDLE hFile = CreateFile(
		pInfo->pszFileName, GENERIC_WRITE, 0, nullptr,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	jpeg_compress_struct jcomp;
	jpeg_error_mgr jerr;

	jcomp.err = jpeg_std_error(&jerr);
	jerr.error_exit = JPEGErrorExit;
	jerr.output_message = JPEGErrorMessage;

	try {
		jpeg_create_compress(&jcomp);
		jcomp.dest = static_cast<jpeg_destination_mgr*>((jcomp.mem->alloc_small)(
			reinterpret_cast<j_common_ptr>(&jcomp), JPOOL_IMAGE, sizeof(JPEGDestinationInfo)));
		jcomp.dest->init_destination = JPEGInitDestination;
		jcomp.dest->empty_output_buffer = JPEGEmptyOutputBuffer;
		jcomp.dest->term_destination = JPEGTermDestination;

		JPEGDestinationInfo *pDstInfo = reinterpret_cast<JPEGDestinationInfo*>(jcomp.dest);
		pDstInfo->hFile = hFile;

		const int Width = pInfo->pbmi->bmiHeader.biWidth;
		const int Height = std::abs(pInfo->pbmi->bmiHeader.biHeight);

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
				&jcomp, JPEG_COM, reinterpret_cast<JOCTET*>(pInfo->pszComment),
				lstrlen(pInfo->pszComment));
#else
			const int Length = WideCharToMultiByte(CP_ACP, 0, pInfo->pszComment, -1, nullptr, 0, nullptr, nullptr);
			std::unique_ptr<char[]> Comment(new char[Length]);
			WideCharToMultiByte(CP_ACP, 0, pInfo->pszComment, -1, Comment.get(), Length, nullptr, nullptr);
			jpeg_write_marker(&jcomp, JPEG_COM, reinterpret_cast<JOCTET*>(Comment.get()), Length - 1);
#endif
		}

		std::unique_ptr<JSAMPLE[]> Buffer(new JSAMPLE[jcomp.image_width * jcomp.input_components]);
		const int BytesPerLine = DIB_ROW_BYTES(Width, pInfo->pbmi->bmiHeader.biBitCount);
		for (int y = 0; y < Height; y++) {
			const BYTE *p = static_cast<const BYTE*>(pInfo->pBits) +
				(pInfo->pbmi->bmiHeader.biHeight > 0 ? (Height - 1 - y) : y) * BytesPerLine;
			CopyToRGB24(
				Buffer.get(), p, pInfo->pbmi->bmiHeader.biBitCount,
				pInfo->pbmi->bmiColors, Width);
			JSAMPROW pScanline = Buffer.get();
			jpeg_write_scanlines(&jcomp, &pScanline, 1);
		}

		jpeg_finish_compress(&jcomp);
		jpeg_destroy_compress(&jcomp);
		CloseHandle(hFile);
	} catch (...) {
		jpeg_destroy_compress(&jcomp);
		CloseHandle(hFile);
		return false;
	}

	return true;
}


}	// namespace ImageLib

}	// namespace TVTest
