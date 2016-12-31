#include "stdafx.h"
#include "EVRPresenterBase.h"
#include "EVRMediaType.h"
#include "../../Common/DebugDef.h"


HRESULT GetFrameRate(IMFMediaType *pType, MFRatio *pRatio)
{
	return ::MFGetAttributeRatio(pType, MF_MT_FRAME_RATE, (UINT32*)&pRatio->Numerator, (UINT32*)&pRatio->Denominator);
}


HRESULT GetVideoDisplayArea(IMFMediaType *pType, MFVideoArea *pArea)
{
	HRESULT hr = S_OK;
	BOOL bPanScan;

	bPanScan = ::MFGetAttributeUINT32(pType, MF_MT_PAN_SCAN_ENABLED, FALSE);

	if (bPanScan) {
		hr = pType->GetBlob(MF_MT_PAN_SCAN_APERTURE, (UINT8*)pArea, sizeof(MFVideoArea), nullptr);
	}

	if (!bPanScan || (hr == MF_E_ATTRIBUTENOTFOUND)) {
		hr = pType->GetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE, (UINT8*)pArea, sizeof(MFVideoArea), nullptr);

		if (hr == MF_E_ATTRIBUTENOTFOUND) {
			hr = pType->GetBlob(MF_MT_GEOMETRIC_APERTURE, (UINT8*)pArea, sizeof(MFVideoArea), nullptr);
		}

		if (hr == MF_E_ATTRIBUTENOTFOUND) {
			UINT32 Width = 0, Height = 0;

			hr = ::MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &Width, &Height);
			if (SUCCEEDED(hr)) {
				*pArea = MakeArea(0.0f, 0.0f, Width, Height);
			}
		}
	}

	return hr;
}


HRESULT GetDefaultStride(IMFMediaType *pType, LONG *pStride)
{
	HRESULT hr;
	LONG Stride = 0;

	hr = pType->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32*)&Stride);

	if (FAILED(hr)) {
		GUID Subtype = GUID_NULL;

		hr = pType->GetGUID(MF_MT_SUBTYPE, &Subtype);

		if (SUCCEEDED(hr)) {
			UINT32 Width = 0, Height = 0;

			hr = ::MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &Width, &Height);

			if (SUCCEEDED(hr)) {
				hr = ::MFGetStrideForBitmapInfoHeader(Subtype.Data1, Width, &Stride);

				if (SUCCEEDED(hr)) {
					pType->SetUINT32(MF_MT_DEFAULT_STRIDE, Stride);
				}
			}
		}
	}

	if (SUCCEEDED(hr)) {
		*pStride = Stride;
	}

	return hr;
}
