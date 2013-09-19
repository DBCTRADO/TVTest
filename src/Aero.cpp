#include "stdafx.h"
#include <dwmapi.h>
#include <uxtheme.h>
#include "TVTest.h"
#include "Aero.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CAeroGlass::CAeroGlass()
	: m_hDwmLib(NULL)
{
}


CAeroGlass::~CAeroGlass()
{
	if (m_hDwmLib)
		::FreeLibrary(m_hDwmLib);
}


bool CAeroGlass::LoadDwmLib()
{
	if (m_hDwmLib==NULL) {
		if (!Util::OS::IsWindowsVistaOrLater())
			return false;
		m_hDwmLib=::LoadLibrary(TEXT("dwmapi.dll"));
		if (m_hDwmLib==NULL)
			return false;
	}
	return true;
}


// コンポジションが有効か取得する
bool CAeroGlass::IsEnabled()
{
	if (!LoadDwmLib())
		return false;

	auto pIsCompositionEnabled=GET_LIBRARY_FUNCTION(m_hDwmLib,DwmIsCompositionEnabled);
	BOOL fEnabled;
	return pIsCompositionEnabled!=NULL
		&& pIsCompositionEnabled(&fEnabled)==S_OK && fEnabled;
}


// クライアント領域を透けさせる
bool CAeroGlass::ApplyAeroGlass(HWND hwnd,const RECT *pRect)
{
	if (!IsEnabled())
		return false;

	auto pExtendFrame=GET_LIBRARY_FUNCTION(m_hDwmLib,DwmExtendFrameIntoClientArea);
	if (pExtendFrame==NULL)
		return false;

	MARGINS Margins;

	Margins.cxLeftWidth=pRect->left;
	Margins.cxRightWidth=pRect->right;
	Margins.cyTopHeight=pRect->top;
	Margins.cyBottomHeight=pRect->bottom;

	return pExtendFrame(hwnd,&Margins)==S_OK;
}


// フレームの描画を無効にする
bool CAeroGlass::EnableNcRendering(HWND hwnd,bool fEnable)
{
	if (!LoadDwmLib())
		return false;

	auto pSetWindowAttribute=GET_LIBRARY_FUNCTION(m_hDwmLib,DwmSetWindowAttribute);
	if (pSetWindowAttribute==NULL)
		return false;

	DWMNCRENDERINGPOLICY ncrp=fEnable?DWMNCRP_USEWINDOWSTYLE:DWMNCRP_DISABLED;
	return pSetWindowAttribute(hwnd,DWMWA_NCRENDERING_POLICY,&ncrp,sizeof(ncrp))==S_OK;
}




class CBufferedPaintInitializer
{
public:
	HMODULE m_hThemeLib;

	CBufferedPaintInitializer()
		: m_hThemeLib(NULL)
	{
	}

	~CBufferedPaintInitializer()
	{
		if (m_hThemeLib!=NULL) {
			auto pBufferedPaintUnInit=GET_LIBRARY_FUNCTION(m_hThemeLib,BufferedPaintUnInit);
			if (pBufferedPaintUnInit!=NULL)
				pBufferedPaintUnInit();
			::FreeLibrary(m_hThemeLib);
		}
	}

	bool Initialize()
	{
		if (m_hThemeLib==NULL) {
			if (!Util::OS::IsWindowsVistaOrLater())
				return false;
			m_hThemeLib=::LoadLibrary(TEXT("uxtheme.dll"));
			if (m_hThemeLib!=NULL) {
				auto pBufferedPaintInit=GET_LIBRARY_FUNCTION(m_hThemeLib,BufferedPaintInit);
				if (pBufferedPaintInit==NULL
						|| pBufferedPaintInit()!=S_OK) {
					TRACE(TEXT("BufferedPaintInit() Failed\n"));
					::FreeLibrary(m_hThemeLib);
					m_hThemeLib=NULL;
					return false;
				}
			}
		}
		return true;
	}
};

static CBufferedPaintInitializer BufferedPaintInitializer;


CBufferedPaint::CBufferedPaint()
	: m_hPaintBuffer(NULL)
{
}


CBufferedPaint::~CBufferedPaint()
{
	End(false);
}


HDC CBufferedPaint::Begin(HDC hdc,const RECT *pRect,bool fErase)
{
	if (BufferedPaintInitializer.m_hThemeLib==NULL)
		return NULL;

	if (m_hPaintBuffer!=NULL) {
		if (!End(false))
			return NULL;
	}

	auto pBeginBufferedPaint=
		GET_LIBRARY_FUNCTION(BufferedPaintInitializer.m_hThemeLib,BeginBufferedPaint);
	if (pBeginBufferedPaint==NULL)
		return NULL;

	BP_PAINTPARAMS Params={sizeof(BP_PAINTPARAMS),0,NULL,NULL};
	if (fErase)
		Params.dwFlags|=BPPF_ERASE;
	HDC hdcBuffer;
	m_hPaintBuffer=pBeginBufferedPaint(hdc,pRect,BPBF_TOPDOWNDIB,&Params,&hdcBuffer);
	if (m_hPaintBuffer==NULL)
		return NULL;
	return hdcBuffer;
}


bool CBufferedPaint::End(bool fUpdate)
{
	if (m_hPaintBuffer!=NULL) {
		auto pEndBufferedPaint=
			GET_LIBRARY_FUNCTION(BufferedPaintInitializer.m_hThemeLib,EndBufferedPaint);
		if (pEndBufferedPaint==NULL)
			return false;
		pEndBufferedPaint(m_hPaintBuffer,fUpdate);
		m_hPaintBuffer=NULL;
	}
	return true;
}


bool CBufferedPaint::Clear(const RECT *pRect)
{
	if (m_hPaintBuffer==NULL)
		return false;
	auto pBufferedPaintClear=
		GET_LIBRARY_FUNCTION(BufferedPaintInitializer.m_hThemeLib,BufferedPaintClear);
	if (pBufferedPaintClear==NULL)
		return false;
	return pBufferedPaintClear(m_hPaintBuffer,pRect)==S_OK;
}


bool CBufferedPaint::SetAlpha(BYTE Alpha)
{
	if (m_hPaintBuffer==NULL)
		return false;
	auto pBufferedPaintSetAlpha=
		GET_LIBRARY_FUNCTION(BufferedPaintInitializer.m_hThemeLib,BufferedPaintSetAlpha);
	if (pBufferedPaintSetAlpha==NULL)
		return false;
	return pBufferedPaintSetAlpha(m_hPaintBuffer,NULL,Alpha)==S_OK;
}


bool CBufferedPaint::Initialize()
{
	return BufferedPaintInitializer.Initialize();
}


bool CBufferedPaint::IsSupported()
{
	return BufferedPaintInitializer.m_hThemeLib!=NULL;
}




void CDoubleBufferingDraw::OnPaint(HWND hwnd)
{
	::PAINTSTRUCT ps;
	::BeginPaint(hwnd,&ps);
	{
		CBufferedPaint BufferedPaint;
		RECT rc;
		::GetClientRect(hwnd,&rc);
		HDC hdc=BufferedPaint.Begin(ps.hdc,&rc);
		if (hdc!=NULL) {
			Draw(hdc,ps.rcPaint);
			BufferedPaint.End();
		} else {
			Draw(ps.hdc,ps.rcPaint);
		}
	}
	::EndPaint(hwnd,&ps);
}
