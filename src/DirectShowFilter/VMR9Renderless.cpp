#include "stdafx.h"
#include <d3d9.h>
#include <vmr9.h>
//#include <streams.h>
#include <vector>
#include "VMR9Renderless.h"
#include "DirectShowUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




class CVMR9Allocator
	: public IVMRSurfaceAllocator9
	, public IVMRImagePresenter9
{
	CCritSec m_ObjectLock;
	HWND m_window;
	LONG m_RefCount;
	SIZE m_WindowSize;
	SIZE m_SourceSize;
	SIZE m_NativeVideoSize;
	RECT m_SourceRect;
	RECT m_DestRect;
	bool m_bCrop1088To1080;

	HMODULE m_hD3D9Lib;
	IDirect3D9 *m_pD3D;
	IDirect3DDevice9 *m_pD3DDev;
	IVMRSurfaceAllocatorNotify9 *m_pSurfaceAllocatorNotify;
	std::vector<IDirect3DSurface9*> m_Surfaces;
	//IDirect3DSurface9 m_pRenderTarget;
	//IDirect3DTexture9 m_pPrivateTexture;

	HANDLE m_hCaptureEvent;
	HANDLE m_hCaptureCompleteEvent;
	IDirect3DSurface9 *m_pCaptureSurface;

public:
	CVMR9Allocator(HRESULT *phr,HWND wnd,IDirect3D9 *d3d=NULL,IDirect3DDevice9 *d3dd=NULL);
	virtual ~CVMR9Allocator();

// IUnknown
	STDMETHOD(QueryInterface)(REFIID riid,void **ppvObject);
	STDMETHOD_(ULONG,AddRef)();
	STDMETHOD_(ULONG,Release)();

// IVMRSurfaceAllocator9
	STDMETHOD(InitializeDevice)(DWORD_PTR dwUserID,
						VMR9AllocationInfo *lpAllocInfo,DWORD *lpNumBuffers);
	STDMETHOD(TerminateDevice)(DWORD_PTR dwID);
	STDMETHOD(GetSurface)(DWORD_PTR dwUserID,DWORD SurfaceIndex,
						  DWORD SurfaceFlags,IDirect3DSurface9 **lplpSurface);
	STDMETHOD(AdviseNotify)(IVMRSurfaceAllocatorNotify9 *lpIVMRSurfAllocNotify);

// IVMRImagePresenter9
	STDMETHOD(StartPresenting)(DWORD_PTR dwUserID);
	STDMETHOD(StopPresenting)(DWORD_PTR dwUserID);
	STDMETHOD(PresentImage)(DWORD_PTR dwUserID,VMR9PresentationInfo *lpPresInfo);

// CVMR9Allocator
	//bool GetNativeVideoSize(LONG *pWidth,LONG *pHeight);
	bool SetVideoPosition(int SourceWidth,int SourceHeight,const RECT *pSrc,const RECT *pDst,const RECT *pWindowRect);
	bool GetVideoPosition(RECT *pSrc,RECT *pDst);
	bool RepaintVideo();
	bool SetCapture(bool fCapture);
	bool WaitCapture(DWORD TimeOut);
	bool GetCaptureSurface(IDirect3DSurface9 **ppSurface);
	void SetCrop1088To1080(bool bCrop) { m_bCrop1088To1080=bCrop; }

protected:
	HRESULT CreateDevice();
	void DeleteSurfaces();

	bool NeedToHandleDisplayChange();

	HRESULT PresentHelper(VMR9PresentationInfo *lpPresInfo);
	void CalcTransferRect(int SurfaceWidth,int SurfaceHeight,RECT *pSourceRect,RECT *pDestRect) const;
};


CVMR9Allocator::CVMR9Allocator(HRESULT *phr,HWND wnd,IDirect3D9 *d3d,IDirect3DDevice9 *d3dd)
	: m_RefCount(1)
	, m_pD3D(d3d)
	, m_pD3DDev(d3dd)
	, m_pSurfaceAllocatorNotify(NULL)
	//, m_pRenderTarget(NULL)
	//, m_pPrivateTexture(NULL)
	, m_window(wnd)
	, m_pCaptureSurface(NULL)
	, m_bCrop1088To1080(true)
{
	CAutoLock Lock(&m_ObjectLock);

	RECT rc;
	::GetClientRect(wnd,&rc);
	m_WindowSize.cx=rc.right;
	m_WindowSize.cy=rc.bottom;
	m_SourceSize.cx=0;
	m_SourceSize.cy=0;
	m_NativeVideoSize.cx=0;
	m_NativeVideoSize.cy=0;
	::SetRectEmpty(&m_SourceRect);
	::SetRectEmpty(&m_DestRect);

	m_hCaptureEvent=::CreateEvent(NULL,FALSE,FALSE,NULL);
	m_hCaptureCompleteEvent=::CreateEvent(NULL,FALSE,FALSE,NULL);

	*phr=S_OK;

	m_hD3D9Lib=::LoadLibrary(TEXT("d3d9.dll"));
	if (m_hD3D9Lib==NULL) {
		*phr=E_FAIL;
		return;
	}

	if (m_pD3D==NULL) {
		ASSERT(d3dd==NULL);

		typedef IDirect3D9 *(WINAPI *Direct3DCreateFunc)(UINT SDKVersion);
		Direct3DCreateFunc pDirect3DCreate9=
			reinterpret_cast<Direct3DCreateFunc>(::GetProcAddress(m_hD3D9Lib,"Direct3DCreate9"));
		if (pDirect3DCreate9==NULL) {
			*phr=E_FAIL;
			return;
		}

		m_pD3D=(*pDirect3DCreate9)(D3D_SDK_VERSION);
		if (m_pD3D==NULL) {
			*phr=E_FAIL;
			return;
		}
	}

	if (m_pD3DDev==NULL)
		*phr=CreateDevice();
}


CVMR9Allocator::~CVMR9Allocator()
{
	DeleteSurfaces();
	SAFE_RELEASE(m_pSurfaceAllocatorNotify);
	//SAFE_RELEASE(m_pRenderTarget);
	SAFE_RELEASE(m_pD3DDev);
	SAFE_RELEASE(m_pD3D);
	::CloseHandle(m_hCaptureEvent);
	::CloseHandle(m_hCaptureCompleteEvent);
	if (m_hD3D9Lib)
		::FreeLibrary(m_hD3D9Lib);
}


STDMETHODIMP CVMR9Allocator::QueryInterface(REFIID riid,void **ppvObject)
{
	if (ppvObject==NULL)
		return E_POINTER;
	if (riid==IID_IVMRSurfaceAllocator9) {
		*ppvObject=static_cast<IVMRSurfaceAllocator9*>(this);
	} else if (riid==IID_IVMRImagePresenter9) {
		*ppvObject=static_cast<IVMRImagePresenter9*>(this);
	} else if (riid==IID_IUnknown) {
		*ppvObject=static_cast<IUnknown*>(static_cast<IVMRSurfaceAllocator9*>(this));
	} else {
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}


STDMETHODIMP_(ULONG) CVMR9Allocator::AddRef()
{
	return ::InterlockedIncrement(&m_RefCount);
}


STDMETHODIMP_(ULONG) CVMR9Allocator::Release()
{
	LONG Result=::InterlockedDecrement(&m_RefCount);
	if (Result==0)
		delete this;
	return Result;
}


HRESULT CVMR9Allocator::CreateDevice()
{
	HRESULT hr;

	SAFE_RELEASE(m_pD3DDev);

	D3DDISPLAYMODE dm;
	hr=m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT,&dm);
	if (FAILED(hr))
		return hr;

	D3DPRESENT_PARAMETERS pp;
	::ZeroMemory(&pp,sizeof(pp));
	pp.BackBufferWidth=1920;
	pp.BackBufferHeight=1080;
	pp.BackBufferFormat=dm.Format;
	pp.SwapEffect=D3DSWAPEFFECT_COPY;
	pp.hDeviceWindow=m_window;
	pp.Windowed=TRUE;
	//pp.Flags=D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

	hr=m_pD3D->CreateDevice(D3DADAPTER_DEFAULT,
							D3DDEVTYPE_HAL,
							m_window,
							D3DCREATE_SOFTWARE_VERTEXPROCESSING |
							D3DCREATE_MULTITHREADED,
							&pp,
							&m_pD3DDev);
	if (FAILED(hr)) {
		hr=m_pD3D->CreateDevice(D3DADAPTER_DEFAULT,
								D3DDEVTYPE_REF,
								m_window,
								D3DCREATE_SOFTWARE_VERTEXPROCESSING |
								D3DCREATE_MULTITHREADED,
								&pp,
								&m_pD3DDev);
		if (FAILED(hr))
			return hr;
	}

	//SAFE_RELEASE(m_pRenderTarget);
	//hr=m_pD3DDev->GetRenderTarget(0,&m_pRenderTarget);

	return hr;
}


void CVMR9Allocator::DeleteSurfaces()
{
	CAutoLock Lock(&m_ObjectLock);

	//SAFE_RELEASE(m_pPrivateTexture);

	for (size_t i=0;i<m_Surfaces.size();i++)
		SAFE_RELEASE(m_Surfaces[i]);

	SAFE_RELEASE(m_pCaptureSurface);
}


STDMETHODIMP CVMR9Allocator::InitializeDevice(
	DWORD_PTR dwUserID,VMR9AllocationInfo *lpAllocInfo,DWORD *lpNumBuffers)
{
	if (lpNumBuffers==NULL)
		return E_POINTER;
	if (m_pSurfaceAllocatorNotify==NULL)
		return E_FAIL;

	TRACE(TEXT("CVMRAllocator::InitializeDevice() : %u x %u (%u buffers)\n"),
		  (unsigned int)lpAllocInfo->dwWidth,
		  (unsigned int)lpAllocInfo->dwHeight,
		  (unsigned int)*lpNumBuffers);

	HRESULT hr;

	hr=m_pSurfaceAllocatorNotify->SetD3DDevice(m_pD3DDev,
		::MonitorFromWindow(m_window,MONITOR_DEFAULTTOPRIMARY));
	if (FAILED(hr))
		return hr;

#if 0	// テクスチャ・サーフェスを作成する場合
	D3DCAPS9 d3dcaps;
	m_pD3DDev->GetDeviceCaps(&d3dcaps);
	if (d3dcaps.TextureCaps&D3DPTEXTURECAPS_POW2) {
		DWORD Width=1,Height=1;

		while (Width<lpAllocInfo->dwWidth)
			Width<<=1;
		while (Height<lpAllocInfo->dwHeight)
			Height<<=1;
		lpAllocInfo->dwWidth=Width;
		lpAllocInfo->dwHeight=Height;
	}
	lpAllocInfo->dwFlags|=VMR9AllocFlag_TextureSurface;
#endif

	DeleteSurfaces();
	m_Surfaces.resize(*lpNumBuffers,NULL);
	hr=m_pSurfaceAllocatorNotify->AllocateSurfaceHelper(lpAllocInfo,lpNumBuffers,&m_Surfaces[0]);

	/*
	if (FAILED(hr) && (lpAllocInfo->dwFlags&VMR9AllocFlag_3DRenderTarget)==0) {
		DeleteSurfaces();

		// is surface YUV ?
		if (lpAllocInfo->Format>'0000') {
			D3DDISPLAYMODE dm;

			hr=m_pD3DDev->GetDisplayMode(NULL,&dm);
			if (FAILED(hr))
				return hr;

			// create the private texture
			hr=m_pD3DDev->CreateTexture(
				lpAllocInfo->dwWidth,lpAllocInfo->dwHeight,
				1,
				D3DUSAGE_RENDERTARGET,
				dm.Format,
				D3DPOOL_DEFAULT,
				&m_pPrivateTexture,
				NULL);
			if (FAILED(hr))
				return hr;
		}

		lpAllocInfo->dwFlags&=~VMR9AllocFlag_TextureSurface;
		lpAllocInfo->dwFlags|=VMR9AllocFlag_OffscreenSurface;

		hr=m_pSurfaceAllocatorNotify->AllocateSurfaceHelper(lpAllocInfo,lpNumBuffers,&m_Surfaces[0]);
		if (FAILED(hr))
			return hr;
	}
	*/

	return hr;
}


STDMETHODIMP CVMR9Allocator::TerminateDevice(DWORD_PTR dwID)
{
	DeleteSurfaces();
	return S_OK;
}


STDMETHODIMP CVMR9Allocator::GetSurface(DWORD_PTR dwUserID,DWORD SurfaceIndex,
							DWORD SurfaceFlags,IDirect3DSurface9 **lplpSurface)
{
	if (lplpSurface==NULL)
		return E_POINTER;

	if (SurfaceIndex>=m_Surfaces.size())
		return E_INVALIDARG;

	CAutoLock Lock(&m_ObjectLock);

	IDirect3DSurface9 *pSurface=m_Surfaces[SurfaceIndex];
	if (pSurface==NULL)
		return E_INVALIDARG;

	*lplpSurface=pSurface;
	pSurface->AddRef();

	return S_OK;
}


STDMETHODIMP CVMR9Allocator::AdviseNotify(IVMRSurfaceAllocatorNotify9 *lpIVMRSurfAllocNotify)
{
	CAutoLock Lock(&m_ObjectLock);
	HRESULT hr;

	m_pSurfaceAllocatorNotify=lpIVMRSurfAllocNotify;
	m_pSurfaceAllocatorNotify->AddRef();

	//HMONITOR hMonitor=m_pD3D->GetAdapterMonitor(D3DADAPTER_DEFAULT);
	HMONITOR hMonitor=::MonitorFromWindow(m_window,MONITOR_DEFAULTTOPRIMARY);
	hr=m_pSurfaceAllocatorNotify->SetD3DDevice(m_pD3DDev,hMonitor);

	return hr;
}


STDMETHODIMP CVMR9Allocator::StartPresenting(DWORD_PTR dwUserID)
{
	CAutoLock Lock(&m_ObjectLock);

	if (m_pD3DDev==NULL)
		return E_FAIL;

	return S_OK;
}


STDMETHODIMP CVMR9Allocator::StopPresenting(DWORD_PTR dwUserID)
{
	return S_OK;
}


STDMETHODIMP CVMR9Allocator::PresentImage(DWORD_PTR dwUserID,
										  VMR9PresentationInfo *lpPresInfo)
{
	CAutoLock Lock(&m_ObjectLock);
	HRESULT hr;

	if (NeedToHandleDisplayChange()) {
		// NOTE: this piece of code is left as a user exercise.
		// The D3DDevice here needs to be switched
		// to the device that is using another adapter
	}

	hr=PresentHelper(lpPresInfo);

	if (hr==D3DERR_DEVICELOST) {
		if (m_pD3DDev->TestCooperativeLevel()==D3DERR_DEVICENOTRESET) {
			DeleteSurfaces();
			hr=CreateDevice();
			if (FAILED(hr))
				return hr;

			//HMONITOR hMonitor=m_pD3D->GetAdapterMonitor(D3DADAPTER_DEFAULT);
			HMONITOR hMonitor=::MonitorFromWindow(m_window,MONITOR_DEFAULTTOPRIMARY);

			hr=m_pSurfaceAllocatorNotify->ChangeD3DDevice(m_pD3DDev,hMonitor);
			if (FAILED(hr))
				return hr;
		}
		hr=S_OK;
	}

	return hr;
}


static void MapRect(RECT *pRect,int XNum,int XDenom,int YNum,int YDenom)
{
	pRect->left=pRect->left*XNum/XDenom;
	pRect->top=pRect->top*YNum/YDenom;
	pRect->right=pRect->right*XNum/XDenom;
	pRect->bottom=pRect->bottom*YNum/YDenom;
}


HRESULT CVMR9Allocator::PresentHelper(VMR9PresentationInfo *lpPresInfo)
{
	if (lpPresInfo==NULL || lpPresInfo->lpSurf==NULL)
		return E_POINTER;

	HRESULT hr;
	D3DSURFACE_DESC desc;

	hr=lpPresInfo->lpSurf->GetDesc(&desc);
	if (FAILED(hr))
		return hr;

	m_NativeVideoSize.cx=desc.Width;
	m_NativeVideoSize.cy=desc.Height;

	if (::WaitForSingleObject(m_hCaptureEvent,0)==WAIT_OBJECT_0) {
		SAFE_RELEASE(m_pCaptureSurface);
		hr=m_pD3DDev->CreateOffscreenPlainSurface(
			desc.Width,desc.Height,D3DFMT_X8R8G8B8,//desc.Format,
			D3DPOOL_DEFAULT,&m_pCaptureSurface,NULL);
		if (SUCCEEDED(hr)) {
			hr=m_pD3DDev->StretchRect(lpPresInfo->lpSurf,NULL,m_pCaptureSurface,NULL,D3DTEXF_NONE);
			if (SUCCEEDED(hr)) {
				::SetEvent(m_hCaptureCompleteEvent);
			} else {
				SAFE_RELEASE(m_pCaptureSurface);
			}
		}
	}

	//m_pD3DDev->SetRenderTarget(0,m_pRenderTarget);

	IDirect3DSurface9 *pDstSurface;

	hr=m_pD3DDev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&pDstSurface);
	if (SUCCEEDED(hr)) {
		hr=m_pD3DDev->BeginScene();
		if (SUCCEEDED(hr)) {
			pDstSurface->GetDesc(&desc);
			RECT rcSource,rcDest;
			CalcTransferRect(desc.Width,desc.Height,&rcSource,&rcDest);
			m_pD3DDev->StretchRect(lpPresInfo->lpSurf,NULL,pDstSurface,NULL,D3DTEXF_NONE);
			hr=m_pD3DDev->EndScene();
			if (SUCCEEDED(hr)) {
				hr=m_pD3DDev->Present(&rcSource,&rcDest,NULL,NULL);
			}
		}
		pDstSurface->Release();
	}

	return hr;
}


void CVMR9Allocator::CalcTransferRect(int SurfaceWidth,int SurfaceHeight,RECT *pSourceRect,RECT *pDestRect) const
{
	if (!::IsRectEmpty(&m_SourceRect)) {
		*pSourceRect=m_SourceRect;
		if (m_SourceSize.cx>0 && m_SourceSize.cy>0)
			MapRect(pSourceRect,m_NativeVideoSize.cx,m_SourceSize.cx,
								m_NativeVideoSize.cy,m_SourceSize.cy);
		MapRect(pSourceRect,SurfaceWidth,m_NativeVideoSize.cx,
							SurfaceHeight,m_NativeVideoSize.cy);
	} else {
		::SetRect(pSourceRect,0,0,SurfaceWidth,SurfaceHeight);
	}
	if (m_bCrop1088To1080 && m_NativeVideoSize.cy==1088) {
		pSourceRect->top=pSourceRect->top*1080/1088;
		pSourceRect->bottom=pSourceRect->bottom*1080/1088;
	}
	if (!::IsRectEmpty(&m_DestRect)) {
		*pDestRect=m_DestRect;
	} else {
		::SetRect(pDestRect,0,0,m_WindowSize.cx,m_WindowSize.cy);
	}
}


bool CVMR9Allocator::NeedToHandleDisplayChange()
{
	if (!m_pSurfaceAllocatorNotify)
		return false;

	D3DDEVICE_CREATION_PARAMETERS Parameters;
	if (FAILED(m_pD3DDev->GetCreationParameters(&Parameters)))
		return false;

	HMONITOR hCurrentMonitor=m_pD3D->GetAdapterMonitor(Parameters.AdapterOrdinal);

	HMONITOR hMonitor=m_pD3D->GetAdapterMonitor(D3DADAPTER_DEFAULT);

	return hMonitor!=hCurrentMonitor;
}


bool CVMR9Allocator::RepaintVideo()
{
	CAutoLock Lock(&m_ObjectLock);
	HRESULT hr;
	IDirect3DSurface9 *pDstSurface;
	D3DSURFACE_DESC desc;
	RECT rcSource,rcDest;

	if (m_NativeVideoSize.cx==0 || m_NativeVideoSize.cy==0)
		return false;
	hr=m_pD3DDev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&pDstSurface);
	if (FAILED(hr))
		return false;
	pDstSurface->GetDesc(&desc);
	CalcTransferRect(desc.Width,desc.Height,&rcSource,&rcDest);
	hr=m_pD3DDev->Present(&rcSource,&rcDest,NULL,NULL);
	pDstSurface->Release();
	return SUCCEEDED(hr);
}


bool CVMR9Allocator::SetCapture(bool fCapture)
{
	::ResetEvent(m_hCaptureCompleteEvent);
	if (fCapture) {
		SAFE_RELEASE(m_pCaptureSurface);
		::SetEvent(m_hCaptureEvent);
	} else {
		::ResetEvent(m_hCaptureEvent);
	}
	return true;
}


bool CVMR9Allocator::WaitCapture(DWORD TimeOut)
{
	return ::WaitForSingleObject(m_hCaptureCompleteEvent,TimeOut)==WAIT_OBJECT_0;
}


bool CVMR9Allocator::GetCaptureSurface(IDirect3DSurface9 **ppSurface)
{
	if (m_pCaptureSurface==NULL)
		return false;
	*ppSurface=m_pCaptureSurface;
	m_pCaptureSurface->AddRef();
	return true;
}


/*
bool CVMR9Allocator::GetNativeVideoSize(LONG *pWidth,LONG *pHeight)
{
	CAutoLock Lock(&m_ObjectLock);

	if (m_NativeVideoSize.cx==0 || m_NativeVideoSize.cy==0)
		return false;
	if (pWidth)
		*pWidth=m_NativeVideoSize.cx;
	if (pHeight)
		*pHeight=m_NativeVideoSize.cy;
	return true;
}
*/


bool CVMR9Allocator::SetVideoPosition(int SourceWidth,int SourceHeight,const RECT *pSrc,const RECT *pDst,const RECT *pWindowRect)
{
	CAutoLock Lock(&m_ObjectLock);

	m_SourceSize.cx=SourceWidth;
	m_SourceSize.cy=SourceHeight;
	if (pSrc) {
		m_SourceRect=*pSrc;
	} else {
		::SetRectEmpty(&m_SourceRect);
	}
	if (pDst) {
		m_DestRect=*pDst;
	} else {
		::SetRectEmpty(&m_DestRect);
	}
	if (pWindowRect->right-pWindowRect->left!=m_WindowSize.cx
			|| pWindowRect->bottom-pWindowRect->top!=m_WindowSize.cy) {
	/*
		if (m_pD3DDev!=NULL && m_pSurfaceAllocatorNotify!=NULL) {
			HRESULT hr;
			D3DDISPLAYMODE dm;

			DeleteSurfaces();
			hr=m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT,&dm);
			D3DPRESENT_PARAMETERS pp;
			ZeroMemory(&pp,sizeof(pp));
			pp.BackBufferWidth=rc.right;
			pp.BackBufferHeight=rc.bottom;
			pp.BackBufferFormat=dm.Format;
			pp.SwapEffect=D3DSWAPEFFECT_COPY;
			pp.hDeviceWindow=m_window;
			pp.Windowed=TRUE;
			hr=m_pD3DDev->Reset(&pp);
			if (SUCCEEDED(hr)) {
				HMONITOR hMonitor=::MonitorFromWindow(m_window,MONITOR_DEFAULTTOPRIMARY);
				hr=m_pSurfaceAllocatorNotify->ChangeD3DDevice(m_pD3DDev,hMonitor);
				if (FAILED(hr))
					return false;
			}
		}
	*/
		m_WindowSize.cx=pWindowRect->right-pWindowRect->left;
		m_WindowSize.cy=pWindowRect->bottom-pWindowRect->top;
	}
	return true;
}


bool CVMR9Allocator::GetVideoPosition(RECT *pSrc,RECT *pDst)
{
	CAutoLock Lock(&m_ObjectLock);

	if (pSrc) {
		if (m_NativeVideoSize.cx==0 || m_NativeVideoSize.cy==0)
			return false;
		if (!::IsRectEmpty(&m_SourceRect)) {
			*pSrc=m_SourceRect;
		} else {
			pSrc->left=0;
			pSrc->top=0;
			pSrc->right=m_NativeVideoSize.cx;
			pSrc->bottom=m_NativeVideoSize.cy;
		}
	}
	if (pDst) {
		if (m_NativeVideoSize.cx==0 || m_NativeVideoSize.cy==0)
			return false;
		if (!::IsRectEmpty(&m_DestRect)) {
			*pDst=m_DestRect;
		} else {
			pDst->left=0;
			pDst->top=0;
			pDst->right=m_WindowSize.cx;
			pDst->bottom=m_WindowSize.cy;
		}
	}
	return true;
}




CVideoRenderer_VMR9Renderless::CVideoRenderer_VMR9Renderless()
	: m_pAllocator(NULL)
{
}


CVideoRenderer_VMR9Renderless::~CVideoRenderer_VMR9Renderless()
{
	CHECK_RELEASE(m_pRenderer);
}


bool CVideoRenderer_VMR9Renderless::Initialize(IGraphBuilder *pFilterGraph,IPin *pInputPin,HWND hwndRender,HWND hwndMessageDrain)
{
	HRESULT hr;

	hr=::CoCreateInstance(CLSID_VideoMixingRenderer9,NULL,CLSCTX_INPROC_SERVER,
						  IID_IBaseFilter,reinterpret_cast<LPVOID*>(&m_pRenderer));
	if (FAILED(hr)) {
		SetError(hr,TEXT("VMR-9のインスタンスを作成できません。"));
		return false;
	}
	hr=pFilterGraph->AddFilter(m_pRenderer,L"VMR9");
	if (FAILED(hr)) {
		CHECK_RELEASE(m_pRenderer);
		SetError(hr,TEXT("VMR-9をフィルタグラフに追加できません。"));
		return false;
	}

	IVMRFilterConfig *pFilterConfig;
	m_pRenderer->QueryInterface(IID_IVMRFilterConfig9,
								reinterpret_cast<LPVOID*>(&pFilterConfig));
	pFilterConfig->SetRenderingMode(VMR9Mode_Renderless);
	pFilterConfig->Release();

	IVMRSurfaceAllocatorNotify9 *pSurfaceAllocatorNotify;
	hr=m_pRenderer->QueryInterface(IID_IVMRSurfaceAllocatorNotify9,
								   reinterpret_cast<void**>(&pSurfaceAllocatorNotify));
	m_pAllocator=new CVMR9Allocator(&hr,hwndRender);
	m_pAllocator->SetCrop1088To1080(m_bCrop1088To1080);
	hr=pSurfaceAllocatorNotify->AdviseSurfaceAllocator(12345,m_pAllocator);
	m_pAllocator->AdviseNotify(pSurfaceAllocatorNotify);
	pSurfaceAllocatorNotify->Release();

	IFilterGraph2 *pFilterGraph2;

	hr=pFilterGraph->QueryInterface(IID_IFilterGraph2,
									reinterpret_cast<LPVOID*>(&pFilterGraph2));
	if (FAILED(hr)) {
		SetError(hr,TEXT("IFilterGraph2を取得できません。"));
		return false;
	}
	hr=pFilterGraph2->RenderEx(pInputPin,
								AM_RENDEREX_RENDERTOEXISTINGRENDERERS,NULL);
	pFilterGraph2->Release();
	if (FAILED(hr)) {
		SetError(hr,TEXT("映像レンダラを構築できません。"));
		return false;
	}
	m_pFilterGraph=pFilterGraph;
	m_hwndRender=hwndRender;
	ClearError();
	return true;
}


bool CVideoRenderer_VMR9Renderless::Finalize()
{
	SAFE_RELEASE(m_pAllocator);
	//CHECK_RELEASE(m_pRenderer);
	return true;
}


bool CVideoRenderer_VMR9Renderless::SetVideoPosition(
	int SourceWidth,int SourceHeight,const RECT *pSourceRect,
	const RECT *pDestRect,const RECT *pWindowRect)
{
	if (m_pRenderer==NULL || m_pAllocator==NULL)
		return false;

	RECT rcDest;

	rcDest=*pDestRect;
	::OffsetRect(&rcDest,pWindowRect->left,pWindowRect->top);
	m_pAllocator->SetVideoPosition(SourceWidth,SourceHeight,pSourceRect,&rcDest,pWindowRect);
	::InvalidateRect(m_hwndRender,NULL,TRUE);
	return true;
}


bool CVideoRenderer_VMR9Renderless::GetDestPosition(RECT *pRect)
{
	if (m_pRenderer==NULL || m_pAllocator==NULL)
		return false;
	return m_pAllocator->GetVideoPosition(NULL,pRect);
}


bool CVideoRenderer_VMR9Renderless::GetCurrentImage(void **ppBuffer)
{
	bool bOK=false;

	if (m_pRenderer) {
		if (m_pAllocator->SetCapture(true)) {
			if (m_pAllocator->WaitCapture(1000)) {
				IDirect3DSurface9 *pSurface;

				if (m_pAllocator->GetCaptureSurface(&pSurface)) {
					D3DSURFACE_DESC desc;
					int Height;
					BITMAPINFO bmi;
					HBITMAP hbm;
					void *pBits;

					pSurface->GetDesc(&desc);
					Height=desc.Height;
					if (m_bCrop1088To1080 && Height==1088)
						Height=1080;
					::ZeroMemory(&bmi.bmiHeader,sizeof(BITMAPINFOHEADER));
					bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
					bmi.bmiHeader.biWidth=desc.Width;
					bmi.bmiHeader.biHeight=Height;
					bmi.bmiHeader.biPlanes=1;
					bmi.bmiHeader.biBitCount=24;
					hbm=::CreateDIBSection(NULL,&bmi,DIB_RGB_COLORS,&pBits,NULL,0);
					if (hbm) {
						HRESULT hr;
#if 1
						HDC hdcMem,hdcSurface;

						hr=pSurface->GetDC(&hdcSurface);
						if (SUCCEEDED(hr)) {
							hdcMem=::CreateCompatibleDC(hdcSurface);
							if (hdcMem) {
								HBITMAP hbmOld;

								hbmOld=(HBITMAP)::SelectObject(hdcMem,hbm);
								::BitBlt(hdcMem,0,0,desc.Width,Height,
										 hdcSurface,0,0,SRCCOPY);
								::SelectObject(hdcMem,hbmOld);
								::DeleteDC(hdcMem);
							}
							pSurface->ReleaseDC(hdcSurface);
#else
						D3DLOCKED_RECT rect;

						hr=pSurface->LockRect(&rect,NULL,0);
						if (SUCCEEDED(hr)) {
							BYTE *p=(BYTE*)rect.pBits,*q;
							SIZE_T DIBRowBytes=(desc.Width*3+3)/4*4;

							for (int y=0;y<Height;y++) {
								q=(BYTE*)pBits+(Height-1-y)*DIBRowBytes;
								for (DWORD x=0;x<desc.Width;x++) {
									q[0]=p[3];
									q[1]=p[2];
									q[2]=p[1];
									p+=4;
									q+=3;
								}
								p+=rect.Pitch-desc.Width*4;
							}
							pSurface->UnlockRect();
#endif
							SIZE_T BitsSize;
							BYTE *pDib;
							BitsSize=(desc.Width*3+3)/4*4*Height;
							pDib=(BYTE*)::CoTaskMemAlloc(sizeof(BITMAPINFOHEADER)+BitsSize);
							if (pDib) {
								::CopyMemory(pDib,&bmi.bmiHeader,sizeof(BITMAPINFOHEADER));
								::CopyMemory(pDib+sizeof(BITMAPINFOHEADER),pBits,BitsSize);
								bOK=true;
								*ppBuffer=pDib;
							}
						}
						::DeleteObject(hbm);
					}
				}
				pSurface->Release();
			}
			m_pAllocator->SetCapture(false);
		}
	}
	return bOK;
}


bool CVideoRenderer_VMR9Renderless::RepaintVideo(HWND hwnd,HDC hdc)
{
	if (m_pRenderer==NULL)
		return false;
	return m_pAllocator->RepaintVideo();
}


bool CVideoRenderer_VMR9Renderless::DisplayModeChanged()
{
	bool fOK=false;

	// Not yet...
	return fOK;
}


bool CVideoRenderer_VMR9Renderless::SetVisible(bool fVisible)
{
	// ウィンドウを再描画させる
	if (m_hwndRender)
		return ::InvalidateRect(m_hwndRender,NULL,TRUE)!=FALSE;
	return false;
}
