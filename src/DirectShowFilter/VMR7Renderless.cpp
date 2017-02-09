#include "stdafx.h"
#include "VMR7Renderless.h"
#include "DirectShowUtil.h"
#include "../Common/DebugDef.h"




CVideoRenderer_VMR7Renderless::CVideoRenderer_VMR7Renderless()
	: CUnknown(TEXT("VMR7 Renderless"),NULL)
	, m_pddsPrimary(NULL)
	, m_pddsPriText(NULL)
	, m_pddsRenderT(NULL)
	, m_pSurfaceAllocator(NULL)
	, m_pImagePresenter(NULL)
	, m_pSurfaceAllocatorNotify(NULL)
	, m_Duration(-1)
{
	AddRef();
}


CVideoRenderer_VMR7Renderless::~CVideoRenderer_VMR7Renderless()
{
	TRACE(TEXT("CVideoRenderer_VMR7Renderless::~CVideoRenderer_VMR7Renderless()\n"));
	CHECK_RELEASE(m_pImagePresenter);
	CHECK_RELEASE(m_pSurfaceAllocator);
	CHECK_RELEASE(m_pSurfaceAllocatorNotify);
	CHECK_RELEASE(m_pRenderer);
}


HRESULT CVideoRenderer_VMR7Renderless::CreateDefaultAllocatorPresenter(HWND hwndRender)
{
	HRESULT hr;

	hr=::CoCreateInstance(CLSID_AllocPresenter,NULL,CLSCTX_INPROC_SERVER,
		IID_IVMRSurfaceAllocator,reinterpret_cast<LPVOID*>(&m_pSurfaceAllocator));
	if (SUCCEEDED(hr)) {
		m_pSurfaceAllocator->AdviseNotify(this);
		hr=m_pSurfaceAllocator->QueryInterface(IID_IVMRImagePresenter,
								reinterpret_cast<LPVOID*>(&m_pImagePresenter));
		if (SUCCEEDED(hr)) {
			IVMRWindowlessControl *pWindowlessControl;

			hr=m_pSurfaceAllocator->QueryInterface(IID_IVMRWindowlessControl,
							reinterpret_cast<LPVOID*>(&pWindowlessControl));
			if (SUCCEEDED(hr)) {
				RECT rc;

				pWindowlessControl->SetVideoClippingWindow(hwndRender);
				pWindowlessControl->SetBorderColor(RGB(0,0,0));
				pWindowlessControl->SetAspectRatioMode(VMR_ARMODE_NONE);
				::GetClientRect(hwndRender,&rc);
				pWindowlessControl->SetVideoPosition(NULL,&rc);
				pWindowlessControl->Release();
			}
		}
	}
	if (FAILED(hr)) {
		CHECK_RELEASE(m_pImagePresenter);
		CHECK_RELEASE(m_pSurfaceAllocator);
	}
	return hr;
}


STDMETHODIMP CVideoRenderer_VMR7Renderless::NonDelegatingQueryInterface(
	REFIID riid,void **ppv)
{
	if (riid==IID_IVMRSurfaceAllocator) {
		return GetInterface(static_cast<IVMRSurfaceAllocator*>(this),ppv);
	} else if (riid==IID_IVMRImagePresenter) {
		return GetInterface(static_cast<IVMRImagePresenter*>(this),ppv);
	}
    return CUnknown::NonDelegatingQueryInterface(riid,ppv);
}


STDMETHODIMP CVideoRenderer_VMR7Renderless::AllocateSurface(
	DWORD_PTR dwUserID,VMRALLOCATIONINFO *lpAllocInfo,DWORD* lpdwBuffer,LPDIRECTDRAWSURFACE7* lplpSurface)
{
	return m_pSurfaceAllocator->AllocateSurface(dwUserID,lpAllocInfo,lpdwBuffer,lplpSurface);
}


STDMETHODIMP CVideoRenderer_VMR7Renderless::FreeSurface(DWORD_PTR dwUserID)
{
	return m_pSurfaceAllocator->FreeSurface(dwUserID);
}


STDMETHODIMP CVideoRenderer_VMR7Renderless::PrepareSurface(DWORD_PTR dwUserID,
						LPDIRECTDRAWSURFACE7 lplpSurface,DWORD dwSurfaceFlags)
{
	return m_pSurfaceAllocator->PrepareSurface(dwUserID,lplpSurface,dwSurfaceFlags);
}


STDMETHODIMP CVideoRenderer_VMR7Renderless::AdviseNotify(
							IVMRSurfaceAllocatorNotify* lpIVMRSurfAllocNotify)
{
	return m_pSurfaceAllocator->AdviseNotify(lpIVMRSurfAllocNotify);
}


STDMETHODIMP CVideoRenderer_VMR7Renderless::AdviseSurfaceAllocator(
			DWORD_PTR dwUserID,IVMRSurfaceAllocator* lpIVRMSurfaceAllocator)
{
	return m_pSurfaceAllocatorNotify->AdviseSurfaceAllocator(dwUserID,lpIVRMSurfaceAllocator);
}


STDMETHODIMP CVideoRenderer_VMR7Renderless::SetDDrawDevice(LPDIRECTDRAW7 lpDDrawDevice,HMONITOR hMonitor)
{
    return m_pSurfaceAllocatorNotify->SetDDrawDevice(lpDDrawDevice,hMonitor);
}


STDMETHODIMP CVideoRenderer_VMR7Renderless::ChangeDDrawDevice(LPDIRECTDRAW7 lpDDrawDevice,HMONITOR hMonitor)
{
	return m_pSurfaceAllocatorNotify->ChangeDDrawDevice(lpDDrawDevice,hMonitor);
}


HRESULT WINAPI DDSurfEnumFunc(LPDIRECTDRAWSURFACE7 pdds,DDSURFACEDESC2 *pddsd,
															void *lpContext)
{
	LPDIRECTDRAWSURFACE7 *ppdds=static_cast<LPDIRECTDRAWSURFACE7*>(lpContext);
	DDSURFACEDESC2 ddsd;

	::ZeroMemory(&ddsd,sizeof(ddsd));
	ddsd.dwSize=sizeof(ddsd);
	HRESULT hr=pdds->GetSurfaceDesc(&ddsd);
	if (SUCCEEDED(hr)) {
		if (ddsd.ddsCaps.dwCaps&DDSCAPS_PRIMARYSURFACE) {
			*ppdds=pdds;
			return DDENUMRET_CANCEL;
		}
	}
	pdds->Release();
	return DDENUMRET_OK;
}


HRESULT CVideoRenderer_VMR7Renderless::OnSetDDrawDevice(LPDIRECTDRAW7 pDD,
														HMONITOR hMon)
{
	HRESULT hr;

	CHECK_RELEASE(m_pddsRenderT);
	CHECK_RELEASE(m_pddsPriText);
	CHECK_RELEASE(m_pddsPrimary);

	DDSURFACEDESC2 ddsd;

	::ZeroMemory(&ddsd,sizeof(ddsd));
	ddsd.dwSize=sizeof(ddsd);
	ddsd.dwFlags=DDSD_CAPS;
	ddsd.ddsCaps.dwCaps=DDSCAPS_PRIMARYSURFACE;
	hr=pDD->EnumSurfaces(DDENUMSURFACES_DOESEXIST | DDENUMSURFACES_ALL,
						 &ddsd,&m_pddsPrimary,DDSurfEnumFunc);
	if (FAILED(hr))
		return hr;
	if (m_pddsPrimary==NULL)
		return E_FAIL;

	MONITORINFOEX mix;
	mix.cbSize=sizeof(mix);
	::GetMonitorInfo(hMon,&mix);

	::ZeroMemory(&ddsd,sizeof(ddsd));
	ddsd.dwSize=sizeof(ddsd);
	ddsd.dwFlags=DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN;
	ddsd.dwWidth =mix.rcMonitor.right-mix.rcMonitor.left;
	ddsd.dwHeight=mix.rcMonitor.bottom-mix.rcMonitor.top;

	hr=pDD->CreateSurface(&ddsd,&m_pddsPriText,NULL);
	if (SUCCEEDED(hr))
		hr=pDD->CreateSurface(&ddsd,&m_pddsRenderT,NULL);
	if (FAILED(hr)) {
		CHECK_RELEASE(m_pddsRenderT);
		CHECK_RELEASE(m_pddsPriText);
		CHECK_RELEASE(m_pddsPrimary);
	}
	return hr;
}


STDMETHODIMP CVideoRenderer_VMR7Renderless::RestoreDDrawSurfaces()
{
    return m_pSurfaceAllocatorNotify->RestoreDDrawSurfaces();
}


STDMETHODIMP CVideoRenderer_VMR7Renderless::NotifyEvent(LONG EventCode,LONG_PTR lp1,LONG_PTR lp2)
{
	return m_pSurfaceAllocatorNotify->NotifyEvent(EventCode,lp1,lp2);
}


STDMETHODIMP CVideoRenderer_VMR7Renderless::SetBorderColor(COLORREF clr)
{
	return m_pSurfaceAllocatorNotify->SetBorderColor(clr);
}


STDMETHODIMP CVideoRenderer_VMR7Renderless::StartPresenting(DWORD_PTR dwUserID)
{
	return m_pImagePresenter->StartPresenting(dwUserID);
}


STDMETHODIMP CVideoRenderer_VMR7Renderless::StopPresenting(DWORD_PTR dwUserID)
{
	return m_pImagePresenter->StopPresenting(dwUserID);
}


STDMETHODIMP CVideoRenderer_VMR7Renderless::PresentImage(DWORD_PTR dwUserID,
											VMRPRESENTATIONINFO* lpPresInfo)
{
#if 0
	LPDIRECTDRAWSURFACE7 lpSurface=lpPresInfo->lpSurf;
	const REFERENCE_TIME rtNow=lpPresInfo->rtStart;
	const DWORD dwSurfaceFlags=lpPresInfo->dwFlags;

	if (m_iDuration>0) {
		HRESULT hr;
		RECT rs,rd;
		DDSURFACEDESC2 ddsdV;

		::ZeroMemory(&ddsdV,sizeof(ddsdV));
		ddsdV.dwSize=sizeof(ddsd);
		hr=lpSurface->GetSurfaceDesc(&ddsdV);

		DDSURFACEDESC2 ddsdP;
		::ZeroMemory(&ddsdP,sizeof(ddsdP));
		ddsdP.dwSize=sizeof(ddsd);
		hr=m_pddsPriText->GetSurfaceDesc(&ddsdP);

		FLOAT fPos = (FLOAT)m_Duration / 30.0F;
		FLOAT fPosInv = 1.0F - fPos;

		::SetRect(&rs, 0, 0,
			::MulDiv((int)ddsdV.dwWidth,30-m_Duration,30),
			ddsdV.dwHeight);
		::SetRect(&rd, 0, 0,
			::MulDiv((int)ddsdP.dwWidth,30-m_Duration,30),
			ddsdP.dwHeight);
		hr=m_pddsRenderT->Blt(&rd,lpSurface,&rs,DDBLT_WAIT,NULL);

		::SetRect(&rs, 0, 0,
			::MulDiv((int)ddsdP.dwWidth,m_Duration,30),
			ddsdP.dwHeight);
		::SetRect(&rd,
			(int)ddsdP.dwWidth-::MulDiv((int)ddsdP.dwWidth,m_Duration,30),
			0,
			ddsdP.dwWidth,
			ddsdP.dwHeight);
		hr=m_pddsRenderT->Blt(&rd,m_pddsPriText,&rs,DDBLT_WAIT,NULL);

		LPDIRECTDRAW lpdd;
		hr = m_pddsPrimary->GetDDInterface((LPVOID*)&lpdd);
		if (SUCCEEDED(hr)) {
			DWORD dwScanLine;
			for (;;) {
				hr=lpdd->GetScanLine(&dwScanLine);
				if (hr==DDERR_VERTICALBLANKINPROGRESS)
					break;
				if (FAILED(hr))
					break;
				if ((LONG)dwScanLine>=rd.top) {
					if ((LONG)dwScanLine<=rd.bottom)
						continue;
				}
				break;
			}
			lpdd->Release();
		}
		hr=m_pddsPrimary->Blt(NULL,m_pddsRenderT,NULL,DDBLT_WAIT,NULL);
		m_Duration--;
		if (m_Duration==0 && (ddsdV.ddsCaps.dwCaps&DDSCAPS_OVERLAY)!=0) {
			::InvalidateRect(m_hwndRender,NULL,FALSE);
		}
		return hr;
	}
#endif
	return m_pImagePresenter->PresentImage(dwUserID,lpPresInfo);
}


bool CVideoRenderer_VMR7Renderless::Initialize(IGraphBuilder *pFilterGraph,
						IPin *pInputPin,HWND hwndRender,HWND hwndMessageDrain)
{
	HRESULT hr;

	hr=::CoCreateInstance(CLSID_VideoMixingRenderer,NULL,CLSCTX_INPROC,
					IID_IBaseFilter,reinterpret_cast<LPVOID*>(&m_pRenderer));
	if (FAILED(hr)) {
		SetError(hr,TEXT("VMRのインスタンスを作成できません。"));
		return false;
	}
	hr=pFilterGraph->AddFilter(m_pRenderer, L"VMR");
	if (FAILED(hr)) {
		SetError(hr,TEXT("VMRをフィルタグラフに追加できません。"));
		return false;
	}
	IVMRFilterConfig *pFilterConfig;
	hr=m_pRenderer->QueryInterface(IID_IVMRFilterConfig,reinterpret_cast<LPVOID*>(&pFilterConfig));
	pFilterConfig->SetRenderingMode(VMRMode_Renderless);
	pFilterConfig->Release();
	m_pRenderer->QueryInterface(IID_IVMRSurfaceAllocatorNotify,reinterpret_cast<LPVOID*>(&m_pSurfaceAllocatorNotify));
	CreateDefaultAllocatorPresenter(hwndRender);
	m_pSurfaceAllocatorNotify->AdviseSurfaceAllocator(1234,this);

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


bool CVideoRenderer_VMR7Renderless::Finalize()
{
	/*
	CHECK_RELEASE(m_pSurfaceAllocatorNotify);
	CHECK_RELEASE(m_pSurfaceAllocator);
	CHECK_RELEASE(m_pImagePresenter);
	CHECK_RELEASE(m_pRenderer);
	*/
	return true;
}


bool CVideoRenderer_VMR7Renderless::SetVideoPosition(int SourceWidth,int SourceHeight,const RECT *pSourceRect,
								const RECT *pDestRect,const RECT *pWindowRect)
{
	IVMRWindowlessControl *pWindowlessControl;
	HRESULT hr;
	RECT rcSrc,rcDest;

	if (m_pSurfaceAllocator==NULL)
		return false;
	hr=m_pSurfaceAllocator->QueryInterface(IID_IVMRWindowlessControl,
							reinterpret_cast<LPVOID*>(&pWindowlessControl));
	if (FAILED(hr))
		return false;

	LONG Width,Height;

	if (SUCCEEDED(pWindowlessControl->GetNativeVideoSize(&Width,&Height,NULL,NULL))) {
		if (SourceWidth>0 && SourceHeight>0) {
			rcSrc.left=pSourceRect->left*Width/SourceWidth;
			rcSrc.top=pSourceRect->top*Height/SourceHeight;
			rcSrc.right=pSourceRect->right*Width/SourceWidth;
			rcSrc.bottom=pSourceRect->bottom*Height/SourceHeight;
		} else {
			rcSrc=*pSourceRect;
		}
		if (m_bCrop1088To1080 && Height==1088) {
			rcSrc.top=rcSrc.top*1080/1088;
			rcSrc.bottom=rcSrc.bottom*1080/1088;
		}
	} else {
		rcSrc=*pSourceRect;
	}
	rcDest=*pDestRect;
	::OffsetRect(&rcDest,pWindowRect->left,pWindowRect->top);
	pWindowlessControl->SetVideoPosition(&rcSrc,&rcDest);
	pWindowlessControl->Release();
	::InvalidateRect(m_hwndRender,NULL,TRUE);
	return true;
}


bool CVideoRenderer_VMR7Renderless::GetDestPosition(RECT *pRect)
{
	bool fOK=false;

	if (m_pRenderer) {
		IVMRWindowlessControl *pWindowlessControl;

		if (SUCCEEDED(m_pSurfaceAllocator->QueryInterface(IID_IVMRWindowlessControl,
							reinterpret_cast<LPVOID*>(&pWindowlessControl)))) {
			fOK=SUCCEEDED(pWindowlessControl->GetVideoPosition(NULL,pRect));
			pWindowlessControl->Release();
		}
	}
	return fOK;
}


bool CVideoRenderer_VMR7Renderless::GetCurrentImage(void **ppBuffer)
{
	bool fOK=false;

	if (m_pRenderer) {
		IVMRWindowlessControl *pWindowlessControl;

		if (SUCCEEDED(m_pSurfaceAllocator->QueryInterface(IID_IVMRWindowlessControl,
							reinterpret_cast<LPVOID*>(&pWindowlessControl)))) {
			BYTE *pDib;

			if (SUCCEEDED(pWindowlessControl->GetCurrentImage(&pDib))) {
				*ppBuffer=pDib;
				fOK=true;
			}
			pWindowlessControl->Release();
		}
	}
	return fOK;
}


bool CVideoRenderer_VMR7Renderless::RepaintVideo(HWND hwnd,HDC hdc)
{
	bool fOK=false;

	if (m_pRenderer) {
		IVMRWindowlessControl *pWindowlessControl;

		if (SUCCEEDED(m_pSurfaceAllocator->QueryInterface(IID_IVMRWindowlessControl,
							reinterpret_cast<LPVOID*>(&pWindowlessControl)))) {
			if (SUCCEEDED(pWindowlessControl->RepaintVideo(hwnd,hdc)))
				fOK=true;
			pWindowlessControl->Release();
		}
	}
	return fOK;
}


bool CVideoRenderer_VMR7Renderless::DisplayModeChanged()
{
	bool fOK=false;

	if (m_pRenderer) {
		IVMRWindowlessControl *pWindowlessControl;

		if (SUCCEEDED(m_pSurfaceAllocator->QueryInterface(IID_IVMRWindowlessControl,
							reinterpret_cast<LPVOID*>(&pWindowlessControl)))) {
			if (SUCCEEDED(pWindowlessControl->DisplayModeChanged()))
				fOK=true;
			pWindowlessControl->Release();
		}
	}
	return fOK;
}


bool CVideoRenderer_VMR7Renderless::SetVisible(bool fVisible)
{
	// ウィンドウを再描画させる
	if (m_hwndRender)
		return ::InvalidateRect(m_hwndRender,NULL,TRUE)!=FALSE;
	return false;
}
