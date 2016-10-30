#pragma once


#include "EVRScheduler.h"


class CEVRPresentEngine
	: public IEVRSchedulerCallback
{
public:
	enum DeviceState
	{
		DeviceState_OK,
		DeviceState_Reset,
		DeviceState_Removed
	};

	CEVRPresentEngine(HRESULT &hr);
	virtual ~CEVRPresentEngine();

	virtual HRESULT GetService(REFGUID guidService, REFIID riid, void **ppv);
	virtual HRESULT CheckFormat(D3DFORMAT Format);

	HRESULT SetVideoWindow(HWND hwnd);
	HWND GetVideoWindow() const { return m_hwnd; }
	HRESULT SetDestinationRect(const RECT &rcDest);
	RECT GetDestinationRect() const { return m_rcDestRect; };

	HRESULT CreateVideoSamples(IMFMediaType *pFormat, VideoSampleList &videoSampleQueue);
	void ReleaseResources();

	HRESULT CheckDeviceState(DeviceState *pState);

	// IEVRSchedulerCallback
	HRESULT PresentSample(IMFSample* pSample, LONGLONG llTarget) override;

	HRESULT GetCurrentImage(BITMAPINFOHEADER *pBih, BYTE **pDib, DWORD *pcbDib, LONGLONG *pTimeStamp);

	UINT RefreshRate() const { return m_DisplayMode.RefreshRate; }

protected:
	HRESULT InitializeD3D();
	HRESULT GetSwapChainPresentParameters(IMFMediaType *pType, D3DPRESENT_PARAMETERS *pPP);
	HRESULT CreateD3DDevice();
	HRESULT CreateD3DSample(IDirect3DSwapChain9 *pSwapChain, IMFSample **ppVideoSample);
	void InitPresentParameters(
		D3DPRESENT_PARAMETERS *pParameters,
		HWND hwnd, UINT Width, UINT Height, D3DFORMAT Format);
	HRESULT UpdateDestRect();
	HRESULT GetDibFromSurface(
		IDirect3DSurface9 *pSurface, const D3DSURFACE_DESC &Desc,
		BITMAPINFOHEADER *pBih, BYTE **pDib, DWORD *pcbDib);

	virtual HRESULT OnCreateVideoSamples(D3DPRESENT_PARAMETERS &pp) { return S_OK; }
	virtual void OnReleaseResources() {}

	virtual HRESULT PresentSwapChain(IDirect3DSwapChain9 *pSwapChain, IDirect3DSurface9 *pSurface);
	virtual void PaintFrameWithGDI();

	static const int PRESENTER_BUFFER_COUNT = 3;

	UINT m_DeviceResetToken;

	HWND m_hwnd;
	RECT m_rcDestRect;
	D3DDISPLAYMODE m_DisplayMode;

	CCritSec m_ObjectLock;

	IDirect3D9Ex *m_pD3D9;
	IDirect3DDevice9Ex *m_pDevice;
	IDirect3DDeviceManager9 *m_pDeviceManager;
	IDirect3DSurface9 *m_pSurfaceRepaint;

	LONGLONG m_LastPresentTime;
	CCritSec m_RepaintSurfaceLock;
};
