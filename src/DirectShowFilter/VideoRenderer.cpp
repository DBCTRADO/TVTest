#include "stdafx.h"
#include <d3d9.h>
#include <vmr9.h>
#include "VideoRenderer.h"
#include "VMR9Renderless.h"
#include "EVRenderer.h"
#include "DirectShowUtil.h"
#include "../HelperClass/StdUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


static const CLSID CLSID_madVR = {0xe1a8b82a, 0x32ce, 0x4b0d, {0xbe, 0x0d, 0xaa, 0x68, 0xc7, 0x72, 0xe4, 0x23}};




CVideoRenderer::CVideoRenderer()
	: m_pRenderer(NULL)
	, m_pFilterGraph(NULL)
	, m_hwndRender(NULL)
	, m_bCrop1088To1080(true)
{
}


CVideoRenderer::~CVideoRenderer()
{
	TRACE(TEXT("CVideoRenderer::~CVideoRenderer()\n"));
}


bool CVideoRenderer::ShowProperty(HWND hwndOwner)
{
	if (m_pRenderer)
		return DirectShowUtil::ShowPropertyPage(m_pRenderer,hwndOwner);
	return false;
}


bool CVideoRenderer::HasProperty()
{
	if (m_pRenderer)
		return DirectShowUtil::HasPropertyPage(m_pRenderer);
	return false;
}




class CVideoRenderer_Default : public CVideoRenderer
{
protected:
	IVideoWindow *m_pVideoWindow;
	IBasicVideo *m_pBasicVideo;

	bool InitializeBasicVideo(IGraphBuilder *pFilterGraph,HWND hwndRender,HWND hwndMessageDrain);

public:
	CVideoRenderer_Default();
	virtual bool Initialize(IGraphBuilder *pFilterGraph,IPin *pInputPin,HWND hwndRender,HWND hwndMessageDrain);
	virtual bool Finalize();
	virtual bool SetVideoPosition(int SourceWidth,int SourceHeight,const RECT *pSourceRect,
								const RECT *pDestRect,const RECT *pWindowRect);
	virtual bool GetDestPosition(RECT *pRect);
	virtual bool GetCurrentImage(void **ppBuffer);
	virtual bool ShowCursor(bool fShow);
	virtual bool SetVisible(bool fVisible);
};


CVideoRenderer_Default::CVideoRenderer_Default()
	: m_pVideoWindow(NULL)
	, m_pBasicVideo(NULL)
{
}


bool CVideoRenderer_Default::InitializeBasicVideo(IGraphBuilder *pFilterGraph,
										HWND hwndRender,HWND hwndMessageDrain)
{
	HRESULT hr;

	hr=pFilterGraph->QueryInterface(IID_IVideoWindow,reinterpret_cast<LPVOID *>(&m_pVideoWindow));
	if (FAILED(hr)) {
		SetError(hr,TEXT("IVideoWindowを取得できません。"));
		return false;
	}
	m_pVideoWindow->put_Owner((OAHWND)hwndRender);
	m_pVideoWindow->put_MessageDrain((OAHWND)hwndMessageDrain);
	m_pVideoWindow->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS);
	m_pVideoWindow->put_BackgroundPalette(OATRUE);
	m_pVideoWindow->put_BorderColor(RGB(0,0,0));
	m_pVideoWindow->put_Caption(L"");
	RECT rc;
	::GetClientRect(hwndRender,&rc);
	m_pVideoWindow->SetWindowPosition(0,0,rc.right,rc.bottom);
	m_pVideoWindow->SetWindowForeground(OATRUE);
	m_pVideoWindow->put_Visible(OATRUE);

	hr=pFilterGraph->QueryInterface(IID_IBasicVideo,reinterpret_cast<LPVOID *>(&m_pBasicVideo));
	if (FAILED(hr)) {
		SAFE_RELEASE(m_pVideoWindow);
		SetError(hr,TEXT("IBasicVideoを取得できません。"));
		return false;
	}
	m_pFilterGraph=pFilterGraph;
	return true;
}


bool CVideoRenderer_Default::Initialize(IGraphBuilder *pFilterGraph,IPin *pInputPin,HWND hwndRender,HWND hwndMessageDrain)
{
	HRESULT hr;

	hr=pFilterGraph->Render(pInputPin);
	if (FAILED(hr)) {
		SetError(hr,TEXT("映像レンダラを構築できません。"));
		return false;
	}
	if (!InitializeBasicVideo(pFilterGraph,hwndRender,hwndMessageDrain))
		return false;
	ClearError();
	return true;
}


bool CVideoRenderer_Default::Finalize()
{
	CHECK_RELEASE(m_pBasicVideo);
	if (m_pVideoWindow) {
		m_pVideoWindow->put_Visible(OAFALSE);
		m_pVideoWindow->put_Owner((OAHWND)NULL);
		CHECK_RELEASE(m_pVideoWindow);
	}
	return true;
}


bool CVideoRenderer_Default::SetVideoPosition(int SourceWidth,int SourceHeight,const RECT *pSourceRect,
								const RECT *pDestRect,const RECT *pWindowRect)
{
	if (m_pVideoWindow==NULL || m_pBasicVideo==NULL)
		return false;
	m_pBasicVideo->SetSourcePosition(pSourceRect->left,pSourceRect->top,
		pSourceRect->right-pSourceRect->left,pSourceRect->bottom-pSourceRect->top);
	m_pBasicVideo->SetDestinationPosition(pDestRect->left,pDestRect->top,
		pDestRect->right-pDestRect->left,pDestRect->bottom-pDestRect->top);
	m_pVideoWindow->SetWindowPosition(pWindowRect->left,pWindowRect->top,
		pWindowRect->right-pWindowRect->left,pWindowRect->bottom-pWindowRect->top);
	return true;
}


bool CVideoRenderer_Default::GetDestPosition(RECT *pRect)
{
	if (m_pBasicVideo==NULL
			|| FAILED(m_pBasicVideo->GetDestinationPosition(
					&pRect->left,&pRect->top,&pRect->right,&pRect->bottom)))
		return false;
	pRect->right+=pRect->left;
	pRect->bottom+=pRect->top;
	return true;
}


bool CVideoRenderer_Default::GetCurrentImage(void **ppBuffer)
{
	bool fOK=false;

	if (m_pBasicVideo!=NULL) {
		long Size;

		if (SUCCEEDED(m_pBasicVideo->GetCurrentImage(&Size,NULL)) && Size>0) {
			long *pDib;

			pDib=static_cast<long*>(::CoTaskMemAlloc(Size));
			if (pDib!=NULL) {
				if (SUCCEEDED(m_pBasicVideo->GetCurrentImage(&Size,pDib))) {
					*ppBuffer=pDib;
					fOK=true;
				} else {
					::CoTaskMemFree(pDib);
				}
			}
		}
	}
	return fOK;
}


bool CVideoRenderer_Default::ShowCursor(bool fShow)
{
	if (m_pVideoWindow) {
		if (SUCCEEDED(m_pVideoWindow->HideCursor(fShow?OAFALSE:OATRUE)))
			return true;
	}
	return false;
}


bool CVideoRenderer_Default::SetVisible(bool fVisible)
{
	if (m_pVideoWindow)
		return SUCCEEDED(m_pVideoWindow->put_Visible(fVisible));
	return false;
}




class CVideoRenderer_Basic : public CVideoRenderer_Default
{
	CLSID m_clsidRenderer;
	LPTSTR m_pszRendererName;
	bool m_bNoSourcePosition;

public:
	CVideoRenderer_Basic(const CLSID &clsid,LPCTSTR pszName,bool bNoSourcePosition=false);
	~CVideoRenderer_Basic();
	bool Initialize(IGraphBuilder *pFilterGraph,IPin *pInputPin,HWND hwndRender,HWND hwndMessageDrain);
	bool Finalize();
	bool SetVideoPosition(int SourceWidth,int SourceHeight,const RECT *pSourceRect,
						  const RECT *pDestRect,const RECT *pWindowRect);
};


CVideoRenderer_Basic::CVideoRenderer_Basic(const CLSID &clsid,LPCTSTR pszName,bool bNoSourcePosition)
	: m_clsidRenderer(clsid)
	, m_pszRendererName(StdUtil::strdup(pszName))
	, m_bNoSourcePosition(bNoSourcePosition)
{
}


CVideoRenderer_Basic::~CVideoRenderer_Basic()
{
	SAFE_RELEASE(m_pRenderer);
	delete [] m_pszRendererName;
}


bool CVideoRenderer_Basic::Initialize(IGraphBuilder *pFilterGraph,IPin *pInputPin,HWND hwndRender,HWND hwndMessageDrain)
{
	HRESULT hr;
	TCHAR szMessage[256];

	hr=::CoCreateInstance(m_clsidRenderer,NULL,CLSCTX_INPROC_SERVER,
						  IID_IBaseFilter,reinterpret_cast<LPVOID*>(&m_pRenderer));
	if (FAILED(hr)) {
		StdUtil::snprintf(szMessage,sizeof(szMessage)/sizeof(TCHAR),
						  TEXT("%sのインスタンスを作成できません。"),
						  m_pszRendererName);
		SetError(hr,szMessage,TEXT("指定したレンダラがインストールされているか確認してください。"));
		return false;
	}

	hr=pFilterGraph->AddFilter(m_pRenderer,m_pszRendererName);
	if (FAILED(hr)) {
		SAFE_RELEASE(m_pRenderer);
		StdUtil::snprintf(szMessage,sizeof(szMessage)/sizeof(TCHAR),
						  TEXT("%sをフィルタグラフに追加できません。"),
						  m_pszRendererName);
		SetError(hr,szMessage);
		return false;
	}

	IFilterGraph2 *pFilterGraph2;
	hr=pFilterGraph->QueryInterface(IID_IFilterGraph2,
									reinterpret_cast<LPVOID*>(&pFilterGraph2));
	if (FAILED(hr)) {
		SAFE_RELEASE(m_pRenderer);
		SetError(hr,TEXT("IFilterGraph2を取得できません。"));
		return false;
	}
	hr=pFilterGraph2->RenderEx(pInputPin,
							   AM_RENDEREX_RENDERTOEXISTINGRENDERERS,NULL);
	pFilterGraph2->Release();
	if (FAILED(hr)) {
		SAFE_RELEASE(m_pRenderer);
		SetError(hr,TEXT("映像レンダラを構築できません。"));
		return false;
	}

	if (!InitializeBasicVideo(pFilterGraph,hwndRender,hwndMessageDrain)) {
		SAFE_RELEASE(m_pRenderer);
		return false;
	}

	ClearError();

	return true;
}


bool CVideoRenderer_Basic::Finalize()
{
	CVideoRenderer_Default::Finalize();
	SAFE_RELEASE(m_pRenderer);
	return true;
}


bool CVideoRenderer_Basic::SetVideoPosition(int SourceWidth,int SourceHeight,const RECT *pSourceRect,
											const RECT *pDestRect,const RECT *pWindowRect)
{
	if (m_pVideoWindow==NULL || m_pBasicVideo==NULL)
		return false;

	if (m_bNoSourcePosition) {
		/*
			IBasicVideo::SetSourcePosition() に対応していないレンダラ用
		*/
		const int CutWidth=pSourceRect->right-pSourceRect->left;
		const int CutHeight=pSourceRect->bottom-pSourceRect->top;
		if (CutWidth<=0 || CutHeight<=0)
			return false;
		const int WindowWidth=pWindowRect->right-pWindowRect->left;
		const int WindowHeight=pWindowRect->bottom-pWindowRect->top;
		RECT rcDest=*pDestRect;
		int DestWidth=rcDest.right-rcDest.left;
		int DestHeight=rcDest.bottom-rcDest.top;

		rcDest.left-=pSourceRect->left*DestWidth/CutWidth;
		rcDest.right+=(SourceWidth-pSourceRect->right)*DestWidth/CutWidth;
		rcDest.top-=pSourceRect->top*DestHeight/CutHeight;
		rcDest.bottom+=(SourceHeight-pSourceRect->bottom)*DestHeight/CutHeight;
		DestWidth=rcDest.right-rcDest.left;
		DestHeight=rcDest.bottom-rcDest.top;
		//::OffsetRect(&rcDest,(WindowWidth-DestWidth)/2,(WindowHeight-DestHeight)/2);
		m_pBasicVideo->SetDefaultSourcePosition();
		m_pBasicVideo->SetDestinationPosition(rcDest.left,rcDest.top,
											  DestWidth,DestHeight);
		m_pVideoWindow->SetWindowPosition(pWindowRect->left,pWindowRect->top,
										  WindowWidth,WindowHeight);
		TRACE(TEXT("CVideoRenderer_Basic::SetVideoPosition() : Src [%d, %d, %d, %d] Dest [%d, %d, %d, %d] -> [%d, %d, %d, %d]\n"),
			  pSourceRect->left,pSourceRect->top,pSourceRect->right,pSourceRect->bottom,
			  pDestRect->left,pDestRect->top,pDestRect->right,pDestRect->bottom,
			  rcDest.left,rcDest.top,rcDest.right,rcDest.bottom);
		return true;
	}

	return CVideoRenderer_Default::SetVideoPosition(SourceWidth,SourceHeight,pSourceRect,
													pDestRect,pWindowRect);
}




class CVideoRenderer_OverlayMixer : public CVideoRenderer_Default {
	ICaptureGraphBuilder2 *m_pCaptureGraphBuilder2;
public:
	CVideoRenderer_OverlayMixer();
	~CVideoRenderer_OverlayMixer();
	bool Initialize(IGraphBuilder *pFilterGraph,IPin *pInputPin,HWND hwndRender,HWND hwndMessageDrain);
	bool Finalize();
};


CVideoRenderer_OverlayMixer::CVideoRenderer_OverlayMixer()
	: m_pCaptureGraphBuilder2(NULL)
{
}


CVideoRenderer_OverlayMixer::~CVideoRenderer_OverlayMixer()
{
	SAFE_RELEASE(m_pRenderer);
	SAFE_RELEASE(m_pCaptureGraphBuilder2);
}


bool CVideoRenderer_OverlayMixer::Initialize(IGraphBuilder *pFilterGraph,IPin *pInputPin,HWND hwndRender,HWND hwndMessageDrain)
{
	HRESULT hr;

	hr=::CoCreateInstance(CLSID_OverlayMixer,NULL,CLSCTX_INPROC_SERVER,
						  IID_IBaseFilter,reinterpret_cast<LPVOID*>(&m_pRenderer));
	if (FAILED(hr)) {
		SetError(hr,TEXT("Overlay Mixerのインスタンスを作成できません。"));
		return false;
	}
	hr=pFilterGraph->AddFilter(m_pRenderer,L"Overlay Mixer");
	if (FAILED(hr)) {
		SAFE_RELEASE(m_pRenderer);
		SetError(hr,TEXT("OverlayMixerをフィルタグラフに追加できません。"));
		return false;
	}

	hr=::CoCreateInstance(CLSID_CaptureGraphBuilder2,NULL,CLSCTX_INPROC_SERVER,
						  IID_ICaptureGraphBuilder2,
						  reinterpret_cast<LPVOID*>(&m_pCaptureGraphBuilder2));
	if (FAILED(hr)) {
		SAFE_RELEASE(m_pRenderer);
		SetError(hr,TEXT("Capture Graph Builder2のインスタンスを作成できません。"));
		return false;
	}
	hr=m_pCaptureGraphBuilder2->SetFiltergraph(pFilterGraph);
	if (FAILED(hr)) {
		SAFE_RELEASE(m_pRenderer);
		SAFE_RELEASE(m_pCaptureGraphBuilder2);
		SetError(hr,TEXT("Capture Graph Builder2にフィルタグラフを設定できません。"));
		return false;
	}
	hr=m_pCaptureGraphBuilder2->RenderStream(NULL,NULL,pInputPin,m_pRenderer,NULL);
	if (FAILED(hr)) {
		SAFE_RELEASE(m_pRenderer);
		SAFE_RELEASE(m_pCaptureGraphBuilder2);
		SetError(hr,TEXT("映像レンダラを構築できません。"));
		return false;
	}

	if (!InitializeBasicVideo(pFilterGraph,hwndRender,hwndMessageDrain)) {
		SAFE_RELEASE(m_pRenderer);
		SAFE_RELEASE(m_pCaptureGraphBuilder2);
		return false;
	}
	ClearError();
	return true;
}


bool CVideoRenderer_OverlayMixer::Finalize()
{
	CVideoRenderer_Default::Finalize();
	CHECK_RELEASE(m_pRenderer);
	SAFE_RELEASE(m_pCaptureGraphBuilder2);
	return true;
}




class CVideoRenderer_VMR7 : public CVideoRenderer {
public:
	CVideoRenderer_VMR7();
	~CVideoRenderer_VMR7();
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


CVideoRenderer_VMR7::CVideoRenderer_VMR7()
{
}


CVideoRenderer_VMR7::~CVideoRenderer_VMR7()
{
	CHECK_RELEASE(m_pRenderer);
}


bool CVideoRenderer_VMR7::Initialize(IGraphBuilder *pFilterGraph,IPin *pInputPin,HWND hwndRender,HWND hwndMessageDrain)
{
	HRESULT hr;

	hr=::CoCreateInstance(CLSID_VideoMixingRenderer,NULL,CLSCTX_INPROC_SERVER,
						IID_IBaseFilter,reinterpret_cast<LPVOID*>(&m_pRenderer));
	if (FAILED(hr)) {
		SetError(hr,TEXT("VMRのインスタンスを作成できません。"));
		return false;
	}

	IVMRFilterConfig *pFilterConfig;
	hr=m_pRenderer->QueryInterface(IID_IVMRFilterConfig,
									reinterpret_cast<LPVOID*>(&pFilterConfig));
	if (FAILED(hr)) {
		SAFE_RELEASE(m_pRenderer);
		SetError(hr,TEXT("IVMRFilterConfigを取得できません。"));
		return false;
	}
#ifdef IMAGE_MIXER_VMR7_SUPPORTED
	pFilterConfig->SetNumberOfStreams(1);
#endif
	pFilterConfig->SetRenderingMode(VMRMode_Windowless);
	pFilterConfig->Release();

	IVMRMixerControl *pMixerControl;
	if (SUCCEEDED(m_pRenderer->QueryInterface(IID_IVMRMixerControl,
								reinterpret_cast<LPVOID*>(&pMixerControl)))) {
		DWORD MixingPref;

		pMixerControl->GetMixingPrefs(&MixingPref);
		MixingPref=(MixingPref&~MixerPref_DecimateMask)|MixerPref_NoDecimation;
		pMixerControl->SetMixingPrefs(MixingPref);
		pMixerControl->Release();
	}

	IVMRWindowlessControl *pWindowlessControl;
	RECT rc;
	hr=m_pRenderer->QueryInterface(IID_IVMRWindowlessControl,
							reinterpret_cast<LPVOID*>(&pWindowlessControl));
	if (FAILED(hr)) {
		SAFE_RELEASE(m_pRenderer);
		SetError(hr,TEXT("IVMRWindowlessControlを取得できません。"));
		return false;
	}
	pWindowlessControl->SetVideoClippingWindow(hwndRender);
	pWindowlessControl->SetBorderColor(RGB(0,0,0));
	pWindowlessControl->SetAspectRatioMode(VMR_ARMODE_NONE);
	::GetClientRect(hwndRender,&rc);
	pWindowlessControl->SetVideoPosition(NULL,&rc);
	pWindowlessControl->Release();

	hr=pFilterGraph->AddFilter(m_pRenderer,L"VMR7");
	if (FAILED(hr)) {
		SAFE_RELEASE(m_pRenderer);
		SetError(hr,TEXT("VMRをフィルタグラフに追加できません。"));
		return false;
	}

	IFilterGraph2 *pFilterGraph2;
	hr=pFilterGraph->QueryInterface(IID_IFilterGraph2,
									reinterpret_cast<LPVOID*>(&pFilterGraph2));
	if (FAILED(hr)) {
		SAFE_RELEASE(m_pRenderer);
		SetError(hr,TEXT("IFilterGraph2を取得できません。"));
		return false;
	}
	hr=pFilterGraph2->RenderEx(pInputPin,
							   AM_RENDEREX_RENDERTOEXISTINGRENDERERS,NULL);
	pFilterGraph2->Release();
	if (FAILED(hr)) {
		SAFE_RELEASE(m_pRenderer);
		SetError(hr,TEXT("映像レンダラを構築できません。"));
		return false;
	}
	m_pFilterGraph=pFilterGraph;
	m_hwndRender=hwndRender;
	return true;
}


bool CVideoRenderer_VMR7::Finalize()
{
	//CHECK_RELEASE(m_pRenderer);
	return true;
}


bool CVideoRenderer_VMR7::SetVideoPosition(int SourceWidth,int SourceHeight,const RECT *pSourceRect,
								const RECT *pDestRect,const RECT *pWindowRect)
{
	IVMRWindowlessControl *pWindowlessControl;
	HRESULT hr;
	RECT rcSrc,rcDest;

	if (m_pRenderer==NULL)
		return false;
	hr=m_pRenderer->QueryInterface(IID_IVMRWindowlessControl,
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


bool CVideoRenderer_VMR7::GetDestPosition(RECT *pRect)
{
	bool fOK=false;

	if (m_pRenderer) {
		IVMRWindowlessControl *pWindowlessControl;

		if (SUCCEEDED(m_pRenderer->QueryInterface(IID_IVMRWindowlessControl,
							reinterpret_cast<LPVOID*>(&pWindowlessControl)))) {
			fOK=SUCCEEDED(pWindowlessControl->GetVideoPosition(NULL,pRect));
			pWindowlessControl->Release();
		}
	}
	return fOK;
}


bool CVideoRenderer_VMR7::GetCurrentImage(void **ppBuffer)
{
	bool fOK=false;

	if (m_pRenderer) {
		IVMRWindowlessControl *pWindowlessControl;

		if (SUCCEEDED(m_pRenderer->QueryInterface(IID_IVMRWindowlessControl,
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


bool CVideoRenderer_VMR7::RepaintVideo(HWND hwnd,HDC hdc)
{
	bool fOK=false;

	if (m_pRenderer) {
		IVMRWindowlessControl *pWindowlessControl;

		if (SUCCEEDED(m_pRenderer->QueryInterface(IID_IVMRWindowlessControl,
							reinterpret_cast<LPVOID*>(&pWindowlessControl)))) {
			if (SUCCEEDED(pWindowlessControl->RepaintVideo(hwnd,hdc)))
				fOK=true;
			pWindowlessControl->Release();
		}
	}
	return fOK;
}


bool CVideoRenderer_VMR7::DisplayModeChanged()
{
	bool fOK=false;

	if (m_pRenderer) {
		IVMRWindowlessControl *pWindowlessControl;

		if (SUCCEEDED(m_pRenderer->QueryInterface(IID_IVMRWindowlessControl,
							reinterpret_cast<LPVOID*>(&pWindowlessControl)))) {
			if (SUCCEEDED(pWindowlessControl->DisplayModeChanged()))
				fOK=true;
			pWindowlessControl->Release();
		}
	}
	return fOK;
}


bool CVideoRenderer_VMR7::SetVisible(bool fVisible)
{
	// ウィンドウを再描画させる
	if (m_hwndRender)
		return ::InvalidateRect(m_hwndRender,NULL,TRUE)!=FALSE;
	return false;
}




class CVideoRenderer_VMR9 : public CVideoRenderer {
public:
	CVideoRenderer_VMR9();
	~CVideoRenderer_VMR9();
	bool Initialize(IGraphBuilder *pFilterGraph,IPin *pInputPin,HWND hwndRender,HWND hwndMessageDrain);
	bool Finalize();
	virtual bool SetVideoPosition(int SourceWidth,int SourceHeight,const RECT *pSourceRect,
								const RECT *pDestRect,const RECT *pWindowRect);
	bool GetDestPosition(RECT *pRect);
	bool GetCurrentImage(void **ppBuffer);
	bool RepaintVideo(HWND hwnd,HDC hdc);
	bool DisplayModeChanged();
	bool SetVisible(bool fVisible);
};


CVideoRenderer_VMR9::CVideoRenderer_VMR9()
{
}


CVideoRenderer_VMR9::~CVideoRenderer_VMR9()
{
	CHECK_RELEASE(m_pRenderer);
}


bool CVideoRenderer_VMR9::Initialize(IGraphBuilder *pFilterGraph,IPin *pInputPin,HWND hwndRender,HWND hwndMessageDrain)
{
	HRESULT hr;

	hr=::CoCreateInstance(CLSID_VideoMixingRenderer9,NULL,CLSCTX_INPROC_SERVER,
						IID_IBaseFilter,reinterpret_cast<LPVOID*>(&m_pRenderer));
	if (FAILED(hr)) {
		SetError(hr,TEXT("VMR-9のインスタンスを作成できません。"));
		return false;
	}

	IVMRFilterConfig *pFilterConfig;
	hr=m_pRenderer->QueryInterface(IID_IVMRFilterConfig9,
									reinterpret_cast<LPVOID*>(&pFilterConfig));
	if (FAILED(hr)) {
		SAFE_RELEASE(m_pRenderer);
		SetError(hr,TEXT("IVMRFilterConfig9を取得できません。"));
		return false;
	}
	pFilterConfig->SetRenderingMode(VMR9Mode_Windowless);
	pFilterConfig->Release();

	IVMRMixerControl9 *pMixerControl;
	if (SUCCEEDED(m_pRenderer->QueryInterface(IID_IVMRMixerControl9,
								reinterpret_cast<LPVOID*>(&pMixerControl)))) {
		DWORD MixingPref;

		pMixerControl->GetMixingPrefs(&MixingPref);
		MixingPref=(MixingPref&~MixerPref9_DecimateMask)|MixerPref9_NonSquareMixing;
		pMixerControl->SetMixingPrefs(MixingPref);
		pMixerControl->Release();
	}

	IVMRWindowlessControl9 *pWindowlessControl;
	RECT rc;
	hr=m_pRenderer->QueryInterface(IID_IVMRWindowlessControl9,
							reinterpret_cast<LPVOID*>(&pWindowlessControl));
	if (FAILED(hr)) {
		SAFE_RELEASE(m_pRenderer);
		SetError(hr,TEXT("IVMRWindowlessControl9を取得できません。"));
		return false;
	}
	pWindowlessControl->SetVideoClippingWindow(hwndRender);
	pWindowlessControl->SetBorderColor(RGB(0,0,0));
	pWindowlessControl->SetAspectRatioMode(VMR9ARMode_None);
	::GetClientRect(hwndRender,&rc);
	pWindowlessControl->SetVideoPosition(NULL,&rc);
	pWindowlessControl->Release();

	hr=pFilterGraph->AddFilter(m_pRenderer,L"VMR9");
	if (FAILED(hr)) {
		SAFE_RELEASE(m_pRenderer);
		SetError(hr,TEXT("VMR-9をフィルタグラフに追加できません。"));
		return false;
	}

	IFilterGraph2 *pFilterGraph2;
	hr=pFilterGraph->QueryInterface(IID_IFilterGraph2,
									reinterpret_cast<LPVOID*>(&pFilterGraph2));
	if (FAILED(hr)) {
		SAFE_RELEASE(m_pRenderer);
		SetError(hr,TEXT("IFilterGraph2を取得できません。"));
		return false;
	}
	hr=pFilterGraph2->RenderEx(pInputPin,
							   AM_RENDEREX_RENDERTOEXISTINGRENDERERS,NULL);
	pFilterGraph2->Release();
	if (FAILED(hr)) {
		SAFE_RELEASE(m_pRenderer);
		SetError(hr,TEXT("映像レンダラを構築できません。"));
		return false;
	}
	m_pFilterGraph=pFilterGraph;
	m_hwndRender=hwndRender;
	ClearError();
	return true;
}


bool CVideoRenderer_VMR9::Finalize()
{
	//CHECK_RELEASE(m_pRenderer);
	return true;
}


bool CVideoRenderer_VMR9::SetVideoPosition(int SourceWidth,int SourceHeight,const RECT *pSourceRect,
								const RECT *pDestRect,const RECT *pWindowRect)
{
	IVMRWindowlessControl9 *pWindowlessControl;
	HRESULT hr;
	RECT rcSrc,rcDest;

	if (m_pRenderer==NULL)
		return false;
	hr=m_pRenderer->QueryInterface(IID_IVMRWindowlessControl9,
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


bool CVideoRenderer_VMR9::GetDestPosition(RECT *pRect)
{
	bool fOK=false;

	if (m_pRenderer) {
		IVMRWindowlessControl9 *pWindowlessControl;

		if (SUCCEEDED(m_pRenderer->QueryInterface(IID_IVMRWindowlessControl9,
							reinterpret_cast<LPVOID*>(&pWindowlessControl)))) {
			fOK=SUCCEEDED(pWindowlessControl->GetVideoPosition(NULL,pRect));
			pWindowlessControl->Release();
		}
	}
	return fOK;
}


bool CVideoRenderer_VMR9::GetCurrentImage(void **ppBuffer)
{
	bool fOK=false;

	if (m_pRenderer) {
		IVMRWindowlessControl9 *pWindowlessControl;

		if (SUCCEEDED(m_pRenderer->QueryInterface(IID_IVMRWindowlessControl9,
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


bool CVideoRenderer_VMR9::RepaintVideo(HWND hwnd,HDC hdc)
{
	bool fOK=false;

	if (m_pRenderer!=NULL) {
		IVMRWindowlessControl9 *pWindowlessControl;

		if (SUCCEEDED(m_pRenderer->QueryInterface(IID_IVMRWindowlessControl9,
							reinterpret_cast<LPVOID*>(&pWindowlessControl)))) {
			if (SUCCEEDED(pWindowlessControl->RepaintVideo(hwnd,hdc)))
				fOK=true;
			pWindowlessControl->Release();
		}
	}
	return fOK;
}


bool CVideoRenderer_VMR9::DisplayModeChanged()
{
	bool fOK=false;

	if (m_pRenderer) {
		IVMRWindowlessControl9 *pWindowlessControl;

		if (SUCCEEDED(m_pRenderer->QueryInterface(IID_IVMRWindowlessControl9,
							reinterpret_cast<LPVOID*>(&pWindowlessControl)))) {
			if (SUCCEEDED(pWindowlessControl->DisplayModeChanged()))
				fOK=true;
			pWindowlessControl->Release();
		}
	}
	return fOK;
}


bool CVideoRenderer_VMR9::SetVisible(bool fVisible)
{
	// ウィンドウを再描画させる
	if (m_hwndRender)
		return ::InvalidateRect(m_hwndRender,NULL,TRUE)!=FALSE;
	return false;
}




#include <ddraw.h>
/*
#define D3D_OVERLOADS
#include <d3d.h>
*/


class CVideoRenderer_VMR7Renderless :
	public CUnknown,
	public IVMRSurfaceAllocator,
	public IVMRImagePresenter,
	public IVMRSurfaceAllocatorNotify,
	public CVideoRenderer
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




bool CVideoRenderer::CreateRenderer(RendererType Type,CVideoRenderer **ppRenderer)
{
	switch (Type) {
	case RENDERER_DEFAULT:
		*ppRenderer=new CVideoRenderer_Default;
		break;
	/*
	case RENDERER_VIDEORENDERER:
		*ppRenderer=new CVideoRenderer_Basic(CLSID_VideoRenderer,TEXT("Video Renderer"));
		break;
	*/
	case RENDERER_VMR7:
		*ppRenderer=new CVideoRenderer_VMR7;
		break;
	case RENDERER_VMR9:
		*ppRenderer=new CVideoRenderer_VMR9;
		break;
	case RENDERER_VMR7RENDERLESS:
		*ppRenderer=new CVideoRenderer_VMR7Renderless;
		break;
	case RENDERER_VMR9RENDERLESS:
		*ppRenderer=new CVideoRenderer_VMR9Renderless;
		break;
	case RENDERER_EVR:
		*ppRenderer=new CVideoRenderer_EVR;
		break;
	case RENDERER_OVERLAYMIXER:
		*ppRenderer=new CVideoRenderer_OverlayMixer;
		break;
	case RENDERER_madVR:
		*ppRenderer=new CVideoRenderer_Basic(CLSID_madVR,TEXT("madVR"),true);
		break;
	default:
		return false;
	}
	return true;
}


LPCTSTR CVideoRenderer::EnumRendererName(int Index)
{
	static const LPCTSTR pszRendererName[] = {
		TEXT("Default"),
		//TEXT("Video Renderer"),
		TEXT("VMR7"),
		TEXT("VMR9"),
		TEXT("VMR7 Renderless"),
		TEXT("VMR9 Renderless"),
		TEXT("EVR"),
		TEXT("Overlay Mixer"),
		TEXT("madVR"),
	};

	if (Index<0 || Index>=sizeof(pszRendererName)/sizeof(LPCTSTR))
		return NULL;
	return pszRendererName[Index];
}


CVideoRenderer::RendererType CVideoRenderer::ParseName(LPCTSTR pszName)
{
	LPCTSTR pszRenderer;
	int i;

	for (i=0;(pszRenderer=EnumRendererName(i))!=NULL;i++) {
		if (::lstrcmpi(pszName,pszRenderer)==0)
			return (RendererType)i;
	}
	return RENDERER_UNDEFINED;
}


static bool TestCreateInstance(REFCLSID clsid)
{
	HRESULT hr;
	IBaseFilter *pRenderer;

	hr=::CoCreateInstance(clsid,NULL,CLSCTX_INPROC_SERVER,
						  IID_IBaseFilter,reinterpret_cast<LPVOID*>(&pRenderer));
	if (FAILED(hr))
		return false;
	pRenderer->Release();
	return true;
}

bool CVideoRenderer::IsAvailable(RendererType Type)
{
	switch (Type) {
	case RENDERER_DEFAULT:
		break;
	/*
	case RENDERER_VIDEORENDERER:
		if (!TestCreateInstance(CLSID_VideoRenderer))
			return false;
		break;
	*/
	case RENDERER_VMR7:
	case RENDERER_VMR7RENDERLESS:
		if (!TestCreateInstance(CLSID_VideoMixingRenderer))
			return false;
		break;
	case RENDERER_VMR9:
	case RENDERER_VMR9RENDERLESS:
		if (!TestCreateInstance(CLSID_VideoMixingRenderer9))
			return false;
		break;
	case RENDERER_EVR:
		if (!TestCreateInstance(CLSID_EnhancedVideoRenderer))
			return false;
		break;
	case RENDERER_OVERLAYMIXER:
		if (!TestCreateInstance(CLSID_OverlayMixer))
			return false;
		break;
	case RENDERER_madVR:
		if (!TestCreateInstance(CLSID_madVR))
			return false;
		break;
	default:
		return false;
	}
	return true;
}
