#include "stdafx.h"
#include <d3d9.h>
#include <evr.h>
#include <evr9.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include "EVRenderer.h"
#include "DirectShowUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#pragma comment(lib,"mfuuid.lib")


#define EVR_VIDEO_WINDOW_CLASS TEXT("Bon DTV EVR Video Window")

typedef HRESULT (WINAPI *MFStartupFunc)(ULONG Version,DWORD dwFlags);
typedef HRESULT (WINAPI *MFShutdownFunc)();




static IMFVideoDisplayControl *GetVideoDisplayControl(IBaseFilter *pRenderer)
{
	HRESULT hr;
	IMFGetService *pGetService;
	IMFVideoDisplayControl *pDisplayControl;

	hr=pRenderer->QueryInterface(IID_IMFGetService,reinterpret_cast<LPVOID*>(&pGetService));
	if (FAILED(hr))
		return NULL;
	hr=pGetService->GetService(MR_VIDEO_RENDER_SERVICE,IID_IMFVideoDisplayControl,reinterpret_cast<LPVOID*>(&pDisplayControl));
	pGetService->Release();
	if (FAILED(hr))
		return NULL;
	return pDisplayControl;
}




CVideoRenderer_EVR::CVideoRenderer_EVR()
	: //m_hMFPlatLib(NULL),
#ifdef EVR_USE_VIDEO_WINDOW
	  m_hwndVideo(NULL)
	, m_hwndMessageDrain(NULL)
	, m_fShowCursor(true)
#endif
{
}


CVideoRenderer_EVR::~CVideoRenderer_EVR()
{
	/*
	if (m_hMFPlatLib) {
		MFShutdownFunc pShutdown=reinterpret_cast<MFShutdownFunc>(::GetProcAddress(m_hMFPlatLib,"MFShutdown"));

		if (pShutdown)
			pShutdown();
		::FreeLibrary(m_hMFPlatLib);
	}
	*/
}


bool CVideoRenderer_EVR::Initialize(IGraphBuilder *pFilterGraph,IPin *pInputPin,HWND hwndRender,HWND hwndMessageDrain)
{
#ifdef EVR_USE_VIDEO_WINDOW
	static bool fRegistered=false;
	HINSTANCE hinst=GetWindowInstance(hwndRender);

	if (!fRegistered) {
		WNDCLASS wc;

		wc.style=CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc=VideoWndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=NULL;
		wc.hbrBackground=CreateSolidBrush(RGB(0,0,0));
		wc.lpszMenuName=NULL;
		wc.lpszClassName=EVR_VIDEO_WINDOW_CLASS;
		if (::RegisterClass(&wc)==0) {
			SetError(TEXT("EVRウィンドウクラスを登録できません。"));
			return false;
		}
		fRegistered=true;
	}
	m_hwndVideo=::CreateWindowEx(0,EVR_VIDEO_WINDOW_CLASS,NULL,
								 WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,0,0,0,0,
								 hwndRender,NULL,hinst,this);
	if (m_hwndVideo==NULL) {
		SetError(TEXT("EVRウィンドウを作成できません。"));
		return false;
	}
#endif

	HRESULT hr;

	// MFStartupは呼ばなくていいらしい
	/*
	m_hMFPlatLib=::LoadLibrary(TEXT("mfplat.dll"));
	if (m_hMFPlatLib==NULL) {
		SetError(TEXT("mfplat.dllをロードできません。"));
		return false;
	}
	MFStartupFunc pStartup=reinterpret_cast<MFStartupFunc>(::GetProcAddress(m_hMFPlatLib,"MFStartup"));
	if (pStartup==NULL) {
		SetError(TEXT("MFStartup関数のアドレスを取得できません。"));
		goto OnError;
	}
	hr=pStartup(MF_VERSION,MFSTARTUP_LITE);
	if (FAILED(hr)) {
		SetError(TEXT("Media Foundationの初期化ができません。"));
		goto OnError;
	}
	*/

	hr=::CoCreateInstance(CLSID_EnhancedVideoRenderer,NULL,CLSCTX_INPROC_SERVER,
						IID_IBaseFilter,reinterpret_cast<LPVOID*>(&m_pRenderer));
	if (FAILED(hr)) {
		SetError(hr,TEXT("EVRのインスタンスを作成できません。"),
					TEXT("システムがEVRに対応していない可能性があります。"));
		goto OnError;
	}

	IEVRFilterConfig *pFilterConfig;
	hr=m_pRenderer->QueryInterface(IID_IEVRFilterConfig,reinterpret_cast<LPVOID*>(&pFilterConfig));
	if (FAILED(hr)) {
		SetError(hr,TEXT("IEVRFilterConfigを取得できません。"));
		goto OnError;
	}
	pFilterConfig->SetNumberOfStreams(1);
	pFilterConfig->Release();

	hr=pFilterGraph->AddFilter(m_pRenderer,L"EVR");
	if (FAILED(hr)) {
		SetError(hr,TEXT("EVRをフィルタグラフに追加できません。"));
		goto OnError;
	}

	IFilterGraph2 *pFilterGraph2;
	hr=pFilterGraph->QueryInterface(IID_IFilterGraph2,
									reinterpret_cast<LPVOID*>(&pFilterGraph2));
	if (FAILED(hr)) {
		SetError(hr,TEXT("IFilterGraph2を取得できません。"));
		goto OnError;
	}
	hr=pFilterGraph2->RenderEx(pInputPin,
								AM_RENDEREX_RENDERTOEXISTINGRENDERERS,NULL);
	pFilterGraph2->Release();
	if (FAILED(hr)) {
		SetError(hr,TEXT("映像レンダラを構築できません。"));
		goto OnError;
	}

	IMFGetService *pGetService;
	hr=m_pRenderer->QueryInterface(IID_IMFGetService,reinterpret_cast<LPVOID*>(&pGetService));
	if (FAILED(hr)) {
		SetError(hr,TEXT("IMFGetServiceを取得できません。"));
		goto OnError;
	}

	IMFVideoDisplayControl *pDisplayControl;
	hr=pGetService->GetService(MR_VIDEO_RENDER_SERVICE,IID_IMFVideoDisplayControl,reinterpret_cast<LPVOID*>(&pDisplayControl));
	if (FAILED(hr)) {
		pGetService->Release();
		SetError(hr,TEXT("IMFVideoDisplayControlを取得できません。"));
		goto OnError;
	}
#ifdef EVR_USE_VIDEO_WINDOW
	pDisplayControl->SetVideoWindow(m_hwndVideo);
#else
	pDisplayControl->SetVideoWindow(hwndRender);
#endif
	pDisplayControl->SetAspectRatioMode(MFVideoARMode_None);
	/*
	RECT rc;
	::GetClientRect(hwndRender,&rc);
	pDisplayControl->SetVideoPosition(NULL,&rc);
	*/
	pDisplayControl->SetBorderColor(RGB(0,0,0));
	pDisplayControl->Release();

	IMFVideoProcessor *pVideoProcessor;
	hr=pGetService->GetService(MR_VIDEO_MIXER_SERVICE,IID_IMFVideoProcessor,reinterpret_cast<LPVOID*>(&pVideoProcessor));
	if (FAILED(hr)) {
		pGetService->Release();
		SetError(hr,TEXT("IMFVideoProcessorを取得できません。"));
		goto OnError;
	}
	pVideoProcessor->SetBackgroundColor(RGB(0,0,0));
/*
	UINT NumModes;
	GUID *pProcessingModes;
	if (SUCCEEDED(pVideoProcessor->GetAvailableVideoProcessorModes(&NumModes,&pProcessingModes))) {
#ifdef _DEBUG
		for (UINT i=0;i<NumModes;i++) {
			DXVA2_VideoProcessorCaps Caps;

			if (SUCCEEDED(pVideoProcessor->GetVideoProcessorCaps(&pProcessingModes[i],&Caps))) {
				TRACE(TEXT("EVR Video Processor %u\n"),i);
				TRACE(TEXT("DeviceCaps : %s\n"),
					  Caps.DeviceCaps==DXVA2_VPDev_EmulatedDXVA1?
						TEXT("DXVA2_VPDev_EmulatedDXVA1"):
					  Caps.DeviceCaps==DXVA2_VPDev_HardwareDevice?
						TEXT("DXVA2_VPDev_HardwareDevice"):
					  Caps.DeviceCaps==DXVA2_VPDev_SoftwareDevice?
						TEXT("DXVA2_VPDev_SoftwareDevice"):TEXT("Unknown"));
			}
		}
#endif
		for (UINT i=0;i<NumModes;i++) {
			DXVA2_VideoProcessorCaps Caps;

			if (SUCCEEDED(pVideoProcessor->GetVideoProcessorCaps(&pProcessingModes[i],&Caps))) {
				if (Caps.DeviceCaps==DXVA2_VPDev_HardwareDevice) {
					pVideoProcessor->SetVideoProcessorMode(&pProcessingModes[i]);
					break;
				}
			}
		}
		::CoTaskMemFree(pProcessingModes);
	}
*/
	pVideoProcessor->Release();

	pGetService->Release();

	m_pFilterGraph=pFilterGraph;
	m_hwndRender=hwndRender;
#ifdef EVR_USE_VIDEO_WINDOW
	m_hwndMessageDrain=hwndMessageDrain;
#endif

	ClearError();

	return true;

OnError:
	SAFE_RELEASE(m_pRenderer);
#ifdef EVR_USE_VIDEO_WINDOW
	::DestroyWindow(m_hwndVideo);
	m_hwndVideo=NULL;
#endif
	/*
	if (m_hMFPlatLib) {
		::FreeLibrary(m_hMFPlatLib);
		m_hMFPlatLib=NULL;
	}
	*/
	return false;
}


bool CVideoRenderer_EVR::Finalize()
{
	SAFE_RELEASE(m_pRenderer);
#ifdef EVR_USE_VIDEO_WINDOW
	if (m_hwndVideo) {
		::DestroyWindow(m_hwndVideo);
		m_hwndVideo=NULL;
	}
#endif
	return true;
}


bool CVideoRenderer_EVR::SetVideoPosition(int SourceWidth,int SourceHeight,const RECT *pSourceRect,
								const RECT *pDestRect,const RECT *pWindowRect)
{
	bool fOK=false;

	if (m_pRenderer) {
		IMFVideoDisplayControl *pDisplayControl=GetVideoDisplayControl(m_pRenderer);

		if (pDisplayControl) {
			MFVideoNormalizedRect rcSrc;
			RECT rcDest;

			rcSrc.left=(float)pSourceRect->left/(float)SourceWidth;
			rcSrc.top=(float)pSourceRect->top/(float)SourceHeight;
			rcSrc.right=(float)pSourceRect->right/(float)SourceWidth;
			rcSrc.bottom=(float)pSourceRect->bottom/(float)SourceHeight;
			rcDest=*pDestRect;
			::OffsetRect(&rcDest,pWindowRect->left,pWindowRect->top);
#ifdef EVR_USE_VIDEO_WINDOW
			::SetWindowPos(m_hwndVideo,HWND_BOTTOM,
				rcDest.left,rcDest.top,
				rcDest.right-rcDest.left,rcDest.bottom-rcDest.top,
				SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
			/*
			::OffsetRect(&rcDest,-rcDest.left,-rcDest.top);
			fOK=SUCCEEDED(pDisplayControl->SetVideoPosition(&rcSrc,&rcDest));
			*/
			fOK=SUCCEEDED(pDisplayControl->SetVideoPosition(&rcSrc,NULL));
#else
			fOK=SUCCEEDED(pDisplayControl->SetVideoPosition(&rcSrc,&rcDest));

			// EVRのバグでバックバッファがクリアされない時があるので、強制的にクリアする
			COLORREF crBorder;
			pDisplayControl->GetBorderColor(&crBorder);
			pDisplayControl->SetBorderColor(crBorder==0?RGB(1,1,1):RGB(0,0,0));
			::InvalidateRect(m_hwndRender,NULL,TRUE);
#endif

			pDisplayControl->Release();
		}
	}
	return fOK;
}


bool CVideoRenderer_EVR::GetDestPosition(RECT *pRect)
{
	bool fOK=false;

#ifdef EVR_USE_VIDEO_WINDOW
	if (m_hwndVideo && pRect) {
		::GetWindowRect(m_hwndVideo,pRect);
		::MapWindowRect(NULL,m_hwndRender,pRect);
		fOK=true;
	}
#else
	if (m_pRenderer) {
		IMFVideoDisplayControl *pDisplayControl=GetVideoDisplayControl(m_pRenderer);

		if (pDisplayControl) {
			MFVideoNormalizedRect rcSrc;

			fOK=SUCCEEDED(pDisplayControl->GetVideoPosition(&rcSrc,pRect));
			pDisplayControl->Release();
			fOK=true;
		}
	}
#endif
	return fOK;
}


bool CVideoRenderer_EVR::GetCurrentImage(void **ppBuffer)
{
	bool fOK=false;

	if (m_pRenderer) {
		IMFVideoDisplayControl *pDisplayControl=GetVideoDisplayControl(m_pRenderer);

		if (pDisplayControl) {
			BITMAPINFOHEADER bmih;
			BYTE *pBits;
			DWORD BitsSize;
			LONGLONG TimeStamp=0;

			bmih.biSize=sizeof(BITMAPINFOHEADER);
			if (SUCCEEDED(pDisplayControl->GetCurrentImage(&bmih,&pBits,&BitsSize,&TimeStamp))) {
				BYTE *pDib;

				pDib=static_cast<BYTE*>(::CoTaskMemAlloc(sizeof(BITMAPINFOHEADER)+BitsSize));
				if (pDib) {
					::CopyMemory(pDib,&bmih,sizeof(BITMAPINFOHEADER));
					::CopyMemory(pDib+sizeof(BITMAPINFOHEADER),pBits,BitsSize);
					*ppBuffer=pDib;
					fOK=true;
				}
				::CoTaskMemFree(pBits);
			}
			pDisplayControl->Release();
		}
	}
	return fOK;
}


bool CVideoRenderer_EVR::ShowCursor(bool fShow)
{
#ifdef EVR_USE_VIDEO_WINDOW
	if (m_fShowCursor!=fShow) {
		if (m_hwndVideo!=NULL) {
			POINT pt;
			RECT rc;

			::GetCursorPos(&pt);
			::GetWindowRect(m_hwndVideo,&rc);
			if (::PtInRect(&rc,pt))
				::SetCursor(fShow?::LoadCursor(NULL,IDC_ARROW):NULL);
		}
		m_fShowCursor=fShow;
	}
#endif
	return true;
}


bool CVideoRenderer_EVR::RepaintVideo(HWND hwnd,HDC hdc)
{
	bool fOK=false;

	if (m_pRenderer) {
		IMFVideoDisplayControl *pDisplayControl=GetVideoDisplayControl(m_pRenderer);

		if (pDisplayControl) {
			fOK=SUCCEEDED(pDisplayControl->RepaintVideo());
			pDisplayControl->Release();
		}
	}
	return fOK;
}


bool CVideoRenderer_EVR::DisplayModeChanged()
{
	return true;
}


bool CVideoRenderer_EVR::SetVisible(bool fVisible)
{
#ifdef EVR_USE_VIDEO_WINDOW
	if (m_hwndVideo==NULL)
		return false;
	return ::ShowWindow(m_hwndVideo,fVisible?SW_SHOW:SW_HIDE)!=FALSE;
#else
	return true;
#endif
}


#ifdef EVR_USE_VIDEO_WINDOW


CVideoRenderer_EVR *CVideoRenderer_EVR::GetThis(HWND hwnd)
{
	return reinterpret_cast<CVideoRenderer_EVR*>(::GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


LRESULT CALLBACK CVideoRenderer_EVR::VideoWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CVideoRenderer_EVR *pThis=static_cast<CVideoRenderer_EVR*>(
				reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);

			::SetWindowLongPtr(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(pThis));
		}
		return 0;

	case WM_SIZE:
		{
			CVideoRenderer_EVR *pThis=GetThis(hwnd);

			if (pThis->m_pRenderer) {
				IMFVideoDisplayControl *pDisplayControl=GetVideoDisplayControl(pThis->m_pRenderer);

				if (pDisplayControl) {
					RECT rc;

					::SetRect(&rc,0,0,LOWORD(lParam),HIWORD(lParam));
					pDisplayControl->SetVideoPosition(NULL,&rc);
				}
			}
		}
		return 0;

	case WM_CHAR:
	case WM_DEADCHAR:
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSCHAR:
	case WM_SYSDEADCHAR:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_MOUSEACTIVATE:
	case WM_NCLBUTTONDBLCLK:
	case WM_NCLBUTTONDOWN:
	case WM_NCLBUTTONUP:
	case WM_NCRBUTTONDBLCLK:
	case WM_NCRBUTTONDOWN:
	case WM_NCRBUTTONUP:
	case WM_NCMBUTTONDBLCLK:
	case WM_NCMBUTTONDOWN:
	case WM_NCMBUTTONUP:
	case WM_NCMOUSEMOVE:
		{
			CVideoRenderer_EVR *pThis=GetThis(hwnd);

			if (pThis->m_hwndMessageDrain!=NULL) {
				::PostMessage(pThis->m_hwndMessageDrain,uMsg,wParam,lParam);
				return 0;
			}
		}
		break;

	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEMOVE:
		{
			CVideoRenderer_EVR *pThis=GetThis(hwnd);

			if (pThis->m_hwndMessageDrain!=NULL) {
				POINT pt;

				pt.x=GET_X_LPARAM(lParam);
				pt.y=GET_Y_LPARAM(lParam);
				::MapWindowPoints(hwnd,pThis->m_hwndMessageDrain,&pt,1);
				::PostMessage(pThis->m_hwndMessageDrain,uMsg,wParam,MAKELPARAM(pt.x,pt.y));
				return 0;
			}
		}
		break;

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT) {
			CVideoRenderer_EVR *pThis=GetThis(hwnd);

			::SetCursor(pThis->m_fShowCursor?::LoadCursor(NULL,IDC_ARROW):NULL);
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			CVideoRenderer_EVR *pThis=GetThis(hwnd);

			pThis->m_hwndVideo=NULL;
		}
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


#endif	// EVR_USE_VIDEO_WINDOW
