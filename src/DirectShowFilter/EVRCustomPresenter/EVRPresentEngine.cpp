#include "stdafx.h"
#include <dwmapi.h>
#include "EVRPresenterBase.h"
#include "EVRPresentEngine.h"
#include "../../Common/DebugDef.h"


// キャプチャにロック可能バックバッファを使う場合
//#define USE_LOCKABLE_BACKBUFFER


namespace
{


HRESULT FindAdapter(IDirect3D9 *pD3D9, HMONITOR hMonitor, UINT *pAdapterID)
{
	const UINT AdapterCount = pD3D9->GetAdapterCount();

	for (UINT i = 0; i < AdapterCount; i++) {
		HMONITOR hMonitorTmp = pD3D9->GetAdapterMonitor(i);

		if (hMonitorTmp == nullptr) {
			break;
		}
		if (hMonitorTmp == hMonitor) {
			*pAdapterID = i;
			return S_OK;
		}
	}

	return E_FAIL;
}


}




CEVRPresentEngine::CEVRPresentEngine(HRESULT &hr)
	: m_DeviceResetToken(0)
	, m_hwnd(nullptr)
	, m_rcDestRect()
	, m_DisplayMode()
	, m_pD3D9(nullptr)
	, m_pDevice(nullptr)
	, m_pDeviceManager(nullptr)
	, m_pSurfaceRepaint(nullptr)
	, m_LastPresentTime(0)
{
	hr = InitializeD3D();

	if (SUCCEEDED(hr)) {
	   hr = CreateD3DDevice();
	}
}


CEVRPresentEngine::~CEVRPresentEngine()
{
	SafeRelease(m_pDevice);
	SafeRelease(m_pSurfaceRepaint);
	SafeRelease(m_pDeviceManager);
	SafeRelease(m_pD3D9);
}


HRESULT CEVRPresentEngine::GetService(REFGUID guidService, REFIID riid, void **ppv)
{
	if (ppv == nullptr) {
		return E_POINTER;
	}

	*ppv = nullptr;

	if ((guidService != MR_VIDEO_RENDER_SERVICE) &&
		(guidService != MR_VIDEO_ACCELERATION_SERVICE)) {
		return MF_E_UNSUPPORTED_SERVICE;
	}

	HRESULT hr = S_OK;

	if (riid == __uuidof(IDirect3DDeviceManager9)) {
		if (m_pDeviceManager == nullptr) {
			hr = MF_E_UNSUPPORTED_SERVICE;
		} else {
			*ppv = m_pDeviceManager;
			m_pDeviceManager->AddRef();
		}
	} else {
		hr = MF_E_UNSUPPORTED_SERVICE;
	}

	return hr;
}


HRESULT CEVRPresentEngine::CheckFormat(D3DFORMAT Format)
{
	HRESULT hr;

	UINT Adapter = D3DADAPTER_DEFAULT;
	D3DDEVTYPE Type = D3DDEVTYPE_HAL;

	if (m_pDevice != nullptr) {
		D3DDEVICE_CREATION_PARAMETERS Params;

		hr = m_pDevice->GetCreationParameters(&Params);
		if (FAILED(hr)) {
			return hr;
		}

		Adapter = Params.AdapterOrdinal;
		Type = Params.DeviceType;
	}

	D3DDISPLAYMODE Mode;

	hr = m_pD3D9->GetAdapterDisplayMode(Adapter, &Mode);
	if (SUCCEEDED(hr)) {
		hr = m_pD3D9->CheckDeviceType(Adapter, Type, Mode.Format, Format, TRUE);
	}

	return hr;
}


HRESULT CEVRPresentEngine::SetVideoWindow(HWND hwnd)
{
	CAutoLock Lock(&m_ObjectLock);

	m_hwnd = hwnd;

	UpdateDestRect();

	HRESULT hr = CreateD3DDevice();

	return hr;
}


HRESULT CEVRPresentEngine::SetDestinationRect(const RECT &rcDest)
{
	CAutoLock Lock(&m_ObjectLock);

	if (!::EqualRect(&rcDest, &m_rcDestRect)) {
		m_rcDestRect = rcDest;

		UpdateDestRect();
	}

	return S_OK;
}


HRESULT CEVRPresentEngine::CreateVideoSamples(
	IMFMediaType *pFormat, VideoSampleList &videoSampleQueue)
{
	if (m_hwnd == nullptr) {
		return MF_E_INVALIDREQUEST;
	}

	if (pFormat == nullptr) {
		return MF_E_UNEXPECTED;
	}

	CAutoLock Lock(&m_ObjectLock);

	ReleaseResources();

	HRESULT hr;

	D3DPRESENT_PARAMETERS pp;

	hr = GetSwapChainPresentParameters(pFormat, &pp);
	if (FAILED(hr)) {
		return hr;
	}

	UpdateDestRect();

	for (int i = 0; i < PRESENTER_BUFFER_COUNT; i++) {
		IDirect3DSwapChain9 *pSwapChain = nullptr;

		hr = m_pDevice->CreateAdditionalSwapChain(&pp, &pSwapChain);

		if (SUCCEEDED(hr)) {
			IMFSample *pVideoSample = nullptr;

			hr = CreateD3DSample(pSwapChain, &pVideoSample);

			if (SUCCEEDED(hr)) {
				hr = videoSampleQueue.InsertBack(pVideoSample);

				if (SUCCEEDED(hr)) {
					hr = pVideoSample->SetUnknown(SampleAttribute_SwapChain, pSwapChain);
				}

				pVideoSample->Release();
			}

			pSwapChain->Release();
		}

		if (FAILED(hr)) {
			break;
		}
	}

	if (SUCCEEDED(hr)) {
		hr = OnCreateVideoSamples(pp);
	}

	if (FAILED(hr)) {
		ReleaseResources();
	}

	return hr;
}


void CEVRPresentEngine::ReleaseResources()
{
	OnReleaseResources();

	SafeRelease(m_pSurfaceRepaint);
}


HRESULT CEVRPresentEngine::CheckDeviceState(DeviceState *pState)
{
	CAutoLock Lock(&m_ObjectLock);

	HRESULT hr;

	hr = m_pDevice->CheckDeviceState(m_hwnd);

	*pState = DeviceState_OK;

	switch (hr) {
	case S_OK:
	case S_PRESENT_OCCLUDED:
	case S_PRESENT_MODE_CHANGED:
		hr = S_OK;
		break;

	case D3DERR_DEVICELOST:
	case D3DERR_DEVICEHUNG:
		hr = CreateD3DDevice();
		if (FAILED(hr)) {
			return hr;
		}
		*pState = DeviceState_Reset;
		hr = S_OK;
		break;

	case D3DERR_DEVICEREMOVED:
		*pState = DeviceState_Removed;
		break;

	case E_INVALIDARG:
		hr = S_OK;
		break;
	}

	return hr;
}


HRESULT CEVRPresentEngine::PresentSample(IMFSample *pSample, LONGLONG llTarget)
{
	HRESULT hr = S_OK;

	IDirect3DSurface9 *pSurface = nullptr;

	if (pSample != nullptr) {
		IMFMediaBuffer *pBuffer = nullptr;

		hr = pSample->GetBufferByIndex(0, &pBuffer);

		if (SUCCEEDED(hr)) {
			hr = ::MFGetService(pBuffer, MR_BUFFER_SERVICE, IID_PPV_ARGS(&pSurface));

			pBuffer->Release();
		}
	} else if (m_pSurfaceRepaint != nullptr) {
		pSurface = m_pSurfaceRepaint;
		pSurface->AddRef();
	}

	if (pSurface != nullptr) {
		IDirect3DSwapChain9 *pSwapChain = nullptr;

		hr = pSurface->GetContainer(IID_PPV_ARGS(&pSwapChain));

		if (SUCCEEDED(hr)) {
			hr = PresentSwapChain(pSwapChain, pSurface);

			if (SUCCEEDED(hr)) {
				CAutoLock Lock(&m_RepaintSurfaceLock);

				CopyComPointer(m_pSurfaceRepaint, pSurface);
				m_LastPresentTime = llTarget;
			}

			pSwapChain->Release();
		}

		pSurface->Release();
	} else {
		PaintFrameWithGDI();
	}

	if (FAILED(hr)) {
		if ((hr == D3DERR_DEVICELOST) ||
			(hr == D3DERR_DEVICENOTRESET) ||
			(hr == D3DERR_DEVICEHUNG)) {
			PaintFrameWithGDI();
			hr = S_OK;
		}
	}

	return hr;
}


HRESULT CEVRPresentEngine::GetCurrentImage(
	BITMAPINFOHEADER *pBih, BYTE **pDib, DWORD *pcbDib, LONGLONG *pTimeStamp)
{
	if ((pBih == nullptr) || (pDib == nullptr) || (pcbDib == nullptr)) {
		return E_POINTER;
	}

	if (pBih->biSize != sizeof(BITMAPINFOHEADER)) {
		return E_INVALIDARG;
	}

	TRACE(TEXT("CEVRPresentEngine::GetCurrentImage()\n"));

	CAutoLock Lock(&m_RepaintSurfaceLock);

	if (m_pSurfaceRepaint == nullptr) {
		return E_FAIL;
	}

	HRESULT hr;

	D3DSURFACE_DESC Desc = {};

	hr = m_pSurfaceRepaint->GetDesc(&Desc);
	if (FAILED(hr)) {
		return hr;
	}

	TRACE(TEXT("Surface desc : Format %#x / Size %d x %d\n"), Desc.Format, Desc.Width, Desc.Height);

	if ((Desc.Format != D3DFMT_R8G8B8) &&
		(Desc.Format != D3DFMT_X8R8G8B8) &&
		(Desc.Format != D3DFMT_A8R8G8B8)) {
		return E_NOTIMPL;
	}

#ifdef USE_LOCKABLE_BACKBUFFER

	hr = GetDibFromSurface(m_pSurfaceRepaint, Desc, pBih, pDib, pcbDib);

#else

	IDirect3DSurface9 *pSurface = nullptr;

	hr = m_pDevice->CreateOffscreenPlainSurface(
		Desc.Width, Desc.Height, Desc.Format, D3DPOOL_SYSTEMMEM, &pSurface, nullptr);
	if (FAILED(hr)) {
		return hr;
	}

	hr = m_pDevice->GetRenderTargetData(m_pSurfaceRepaint, pSurface);

	if (SUCCEEDED(hr)) {
		hr = GetDibFromSurface(pSurface, Desc, pBih, pDib, pcbDib);
	}

	pSurface->Release();

#endif

	if (FAILED(hr)) {
		return hr;
	}

	if (pTimeStamp != nullptr) {
		*pTimeStamp = m_LastPresentTime;
	}

	return S_OK;
}


HRESULT CEVRPresentEngine::GetDibFromSurface(
	IDirect3DSurface9 *pSurface, const D3DSURFACE_DESC &Desc,
	BITMAPINFOHEADER *pBih, BYTE **pDib, DWORD *pcbDib)
{
	HRESULT hr;

	D3DLOCKED_RECT Rect = {};

	hr = pSurface->LockRect(&Rect, nullptr, D3DLOCK_READONLY);
	if (FAILED(hr)) {
		TRACE(TEXT("LockRect() failed (%#x)\n"), hr);
		return hr;
	}

	const int BytesPerPixel = (Desc.Format == D3DFMT_R8G8B8) ? 3 : 4;
	const DWORD RowStride = ((Desc.Width * BytesPerPixel) + 3) & ~3;
	const DWORD PixelBytes = RowStride * Desc.Height;

	BYTE *pBits = static_cast<BYTE*>(::CoTaskMemAlloc(PixelBytes));
	if (pBits == nullptr) {
		hr = E_OUTOFMEMORY;
	} else {
		::ZeroMemory(&pBih->biSize + 1, pBih->biSize - sizeof(pBih->biSize));

		pBih->biWidth = Desc.Width;
		pBih->biHeight = Desc.Height;
		pBih->biPlanes = 1;
		pBih->biBitCount = BytesPerPixel * 8;
		pBih->biSizeImage = PixelBytes;

		for (UINT y = 0; y < Desc.Height; y++) {
			::CopyMemory(
				pBits + ((Desc.Height - 1 - y) * RowStride),
				static_cast<const BYTE*>(Rect.pBits) + (y * Rect.Pitch),
				Desc.Width * BytesPerPixel);
		}

		*pDib = pBits;
		*pcbDib = PixelBytes;
	}

	pSurface->UnlockRect();

	return hr;
}


HRESULT CEVRPresentEngine::InitializeD3D()
{
	_ASSERT(m_pD3D9 == nullptr);
	_ASSERT(m_pDeviceManager == nullptr);

	HRESULT hr;

	hr = ::Direct3DCreate9Ex(D3D_SDK_VERSION, &m_pD3D9);
	if (SUCCEEDED(hr)) {
		hr = ::DXVA2CreateDirect3DDeviceManager9(&m_DeviceResetToken, &m_pDeviceManager);
	}

	return hr;
}


HRESULT CEVRPresentEngine::CreateD3DDevice()
{
	CAutoLock Lock(&m_ObjectLock);

	if ((m_pD3D9 == nullptr) || (m_pDeviceManager == nullptr)) {
		return MF_E_NOT_INITIALIZED;
	}

	HRESULT hr;

	UINT AdapterID = D3DADAPTER_DEFAULT;

	if (m_hwnd != nullptr) {
		HMONITOR hMonitor = ::MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);

		hr = FindAdapter(m_pD3D9, hMonitor, &AdapterID);
		if (FAILED(hr)) {
			return hr;
		}
	}

	D3DCAPS9 Caps;

	hr = m_pD3D9->GetDeviceCaps(AdapterID, D3DDEVTYPE_HAL, &Caps);
	if (FAILED(hr)) {
		return hr;
	}

	DWORD Vertex;

	if (Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) {
		Vertex = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	} else {
		Vertex = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	}

	D3DPRESENT_PARAMETERS pp;

	InitPresentParameters(&pp, ::GetDesktopWindow(), 1, 1, D3DFMT_UNKNOWN);

	IDirect3DDevice9Ex *pDevice = nullptr;

	hr = m_pD3D9->CreateDeviceEx(
		AdapterID,
		D3DDEVTYPE_HAL,
		pp.hDeviceWindow,
		Vertex | D3DCREATE_NOWINDOWCHANGES | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
		&pp, 
		nullptr,
		&pDevice
	);
	if (FAILED(hr)) {
		return hr;
	}

	hr = m_pD3D9->GetAdapterDisplayMode(AdapterID, &m_DisplayMode);
	if (SUCCEEDED(hr)) {
		hr = m_pDeviceManager->ResetDevice(pDevice, m_DeviceResetToken);
	}

	if (SUCCEEDED(hr)) {
		SafeRelease(m_pDevice);
		m_pDevice = pDevice;
	} else {
		pDevice->Release();
	}

	return hr;
}


HRESULT CEVRPresentEngine::CreateD3DSample(IDirect3DSwapChain9 *pSwapChain, IMFSample **ppVideoSample)
{
	HRESULT hr;

	IDirect3DSurface9 *pSurface = nullptr;

	hr = pSwapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pSurface);

	if (SUCCEEDED(hr)) {
		hr = m_pDevice->ColorFill(pSurface, nullptr, D3DCOLOR_ARGB(0xFF, 0x00, 0x00, 0x00));

		if (SUCCEEDED(hr)) {
			IMFSample *pSample = nullptr;

			hr = ::MFCreateVideoSampleFromSurface(pSurface, &pSample);

			if (SUCCEEDED(hr)) {
				*ppVideoSample = pSample;
			}
		}

		pSurface->Release();
	}

	return hr;
}


void CEVRPresentEngine::InitPresentParameters(
	D3DPRESENT_PARAMETERS *pParameters,
	HWND hwnd, UINT Width, UINT Height, D3DFORMAT Format)
{
	BOOL bCompositionEnabled = FALSE;
	HMODULE hDwmLib = ::GetModuleHandle(TEXT("dwmapi.dll"));
	if (hDwmLib != nullptr) {
		auto pDwmIsCompositionEnabled =
			reinterpret_cast<decltype(DwmIsCompositionEnabled)*>(
				::GetProcAddress(hDwmLib, "DwmIsCompositionEnabled"));
		if (pDwmIsCompositionEnabled != nullptr) {
			pDwmIsCompositionEnabled(&bCompositionEnabled);
		}
	}

	::ZeroMemory(pParameters, sizeof(D3DPRESENT_PARAMETERS));

	pParameters->BackBufferWidth = Width;
	pParameters->BackBufferHeight = Height;
	pParameters->BackBufferFormat = Format;
	pParameters->SwapEffect = D3DSWAPEFFECT_COPY;
	pParameters->hDeviceWindow = hwnd;
	pParameters->Windowed = TRUE;
	pParameters->Flags = D3DPRESENTFLAG_VIDEO
#ifdef USE_LOCKABLE_BACKBUFFER
		| D3DPRESENTFLAG_LOCKABLE_BACKBUFFER
#endif
		;
	pParameters->PresentationInterval =
		bCompositionEnabled ?
			D3DPRESENT_INTERVAL_IMMEDIATE :
			D3DPRESENT_INTERVAL_DEFAULT;
}


HRESULT CEVRPresentEngine::PresentSwapChain(IDirect3DSwapChain9 *pSwapChain, IDirect3DSurface9 *pSurface)
{
	if (m_hwnd == nullptr) {
		return MF_E_INVALIDREQUEST;
	}

	return pSwapChain->Present(nullptr, &m_rcDestRect, m_hwnd, nullptr, 0);
}


void CEVRPresentEngine::PaintFrameWithGDI()
{
	HDC hdc = ::GetDC(m_hwnd);

	if (hdc != nullptr) {
		::FillRect(hdc, &m_rcDestRect, static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
		::ReleaseDC(m_hwnd, hdc);
	}
}


HRESULT CEVRPresentEngine::GetSwapChainPresentParameters(IMFMediaType *pType, D3DPRESENT_PARAMETERS *pPP)
{
	if (m_hwnd == nullptr) {
		return MF_E_INVALIDREQUEST;
	}

	HRESULT hr;

	UINT32 Width = 0, Height = 0;
	DWORD Format = 0;

	VideoType videoType(pType);

	::ZeroMemory(pPP, sizeof(D3DPRESENT_PARAMETERS));

	hr = videoType.GetFrameDimensions(&Width, &Height);
	if (FAILED(hr)) {
		return S_OK;
	}

	hr = videoType.GetFourCC(&Format);
	if (FAILED(hr)) {
		return S_OK;
	}

	InitPresentParameters(pPP, m_hwnd, Width, Height, static_cast<D3DFORMAT>(Format));

#if 0
	D3DDEVICE_CREATION_PARAMETERS params;
	hr = m_pDevice->GetCreationParameters(&params);
	if (SUCCEEDED(hr)) {
		if (params.DeviceType != D3DDEVTYPE_HAL) {
			pPP->Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
		}
	}
#endif

	return S_OK;
}


HRESULT CEVRPresentEngine::UpdateDestRect()
{
	if (m_hwnd == nullptr) {
		return S_FALSE;
	}

	RECT rcView;
	::GetClientRect(m_hwnd, &rcView);

	if (m_rcDestRect.right > rcView.right) {
		m_rcDestRect.right = rcView.right;
	}

	if (m_rcDestRect.bottom > rcView.bottom) {
		m_rcDestRect.bottom = rcView.bottom;
	}

	return S_OK;
}
