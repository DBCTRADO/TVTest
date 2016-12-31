#pragma once


#include "VideoRenderer.h"

#include <ddraw.h>
/*
#define D3D_OVERLOADS
#include <d3d.h>
*/


class CVideoRenderer_VMR7Renderless
	: public CUnknown
	, public IVMRSurfaceAllocator
	, public IVMRImagePresenter
	, public IVMRSurfaceAllocatorNotify
	, public CVideoRenderer
{
	LPDIRECTDRAWSURFACE7 m_pddsPrimary;
	LPDIRECTDRAWSURFACE7 m_pddsPriText;
	LPDIRECTDRAWSURFACE7 m_pddsRenderT;

	IVMRSurfaceAllocator       *m_pSurfaceAllocator;
	IVMRImagePresenter         *m_pImagePresenter;
	IVMRSurfaceAllocatorNotify *m_pSurfaceAllocatorNotify;

	int m_Duration;

	HRESULT CreateDefaultAllocatorPresenter(HWND hwndRender);
	HRESULT AddVideoMixingRendererToFG();
	HRESULT OnSetDDrawDevice(LPDIRECTDRAW7 pDD,HMONITOR hMon);

public:
	CVideoRenderer_VMR7Renderless();
	~CVideoRenderer_VMR7Renderless();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

	// IVMRSurfaceAllocator
	STDMETHODIMP AllocateSurface(DWORD_PTR dwUserID,
								 VMRALLOCATIONINFO* lpAllocInfo,
								 DWORD* lpdwActualBackBuffers,
								 LPDIRECTDRAWSURFACE7* lplpSurface);
	STDMETHODIMP FreeSurface(DWORD_PTR dwUserID);
	STDMETHODIMP PrepareSurface(DWORD_PTR dwUserID,
						LPDIRECTDRAWSURFACE7 lpSurface,DWORD dwSurfaceFlags);
	STDMETHODIMP AdviseNotify(IVMRSurfaceAllocatorNotify* lpIVMRSurfAllocNotify);

	// IVMRSurfaceAllocatorNotify
	STDMETHODIMP AdviseSurfaceAllocator(DWORD_PTR dwUserID,
								IVMRSurfaceAllocator* lpIVRMSurfaceAllocator);
	STDMETHODIMP SetDDrawDevice(LPDIRECTDRAW7 lpDDrawDevice,HMONITOR hMonitor);
	STDMETHODIMP ChangeDDrawDevice(LPDIRECTDRAW7 lpDDrawDevice,HMONITOR hMonitor);
	STDMETHODIMP RestoreDDrawSurfaces();
	STDMETHODIMP NotifyEvent(LONG EventCode,LONG_PTR lp1,LONG_PTR lp2);
	STDMETHODIMP SetBorderColor(COLORREF clr);

	// IVMRImagePresenter
	STDMETHODIMP StartPresenting(DWORD_PTR dwUserID);
	STDMETHODIMP StopPresenting(DWORD_PTR dwUserID);
	STDMETHODIMP PresentImage(DWORD_PTR dwUserID,VMRPRESENTATIONINFO* lpPresInfo);

	// CVideoRenderer
	bool Initialize(IGraphBuilder *pFilterGraph,IPin *pInputPin,HWND hwndRender,HWND hwndMessageDrain);
	bool Finalize();
	bool SetVideoPosition(int SourceWidth,int SourceHeight,const RECT *pSourceRect,
							const RECT *pDestRect,const RECT *pWindowRect);
	bool GetDestPosition(RECT *pRect);
	bool GetCurrentImage(void **ppBuffer);
	bool RepaintVideo(HWND hwnd,HDC hdc);
	bool DisplayModeChanged();
	bool SetVisible(bool fVisible);
};
