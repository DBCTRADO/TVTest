#include "stdafx.h"
#include <dwmapi.h>
#include <uxtheme.h>
#include "TVTest.h"
#include "Aero.h"
#include "Common/DebugDef.h"

#ifndef WIN_XP_SUPPORT
#pragma comment(lib,"dwmapi.lib")
#endif




CAeroGlass::CAeroGlass()
#ifdef WIN_XP_SUPPORT
	: m_hDwmLib(NULL)
#endif
{
}


CAeroGlass::~CAeroGlass()
{
#ifdef WIN_XP_SUPPORT
	if (m_hDwmLib)
		::FreeLibrary(m_hDwmLib);
#endif
}


#ifdef WIN_XP_SUPPORT
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
#endif


// コンポジションが有効か取得する
bool CAeroGlass::IsEnabled()
{
	BOOL fEnabled;

#ifdef WIN_XP_SUPPORT
	if (!LoadDwmLib())
		return false;

	auto pIsCompositionEnabled=GET_LIBRARY_FUNCTION(m_hDwmLib,DwmIsCompositionEnabled);

	return pIsCompositionEnabled!=NULL
		&& pIsCompositionEnabled(&fEnabled)==S_OK && fEnabled;
#else
	return ::DwmIsCompositionEnabled(&fEnabled)==S_OK && fEnabled;
#endif
}


// クライアント領域を透けさせる
bool CAeroGlass::ApplyAeroGlass(HWND hwnd,const RECT *pRect)
{
	if (!IsEnabled())
		return false;

#ifdef WIN_XP_SUPPORT
	auto pExtendFrame=GET_LIBRARY_FUNCTION(m_hDwmLib,DwmExtendFrameIntoClientArea);
	if (pExtendFrame==NULL)
		return false;
#endif

	MARGINS Margins;

	Margins.cxLeftWidth=pRect->left;
	Margins.cxRightWidth=pRect->right;
	Margins.cyTopHeight=pRect->top;
	Margins.cyBottomHeight=pRect->bottom;

#ifdef WIN_XP_SUPPORT
	return pExtendFrame(hwnd,&Margins)==S_OK;
#else
	return ::DwmExtendFrameIntoClientArea(hwnd,&Margins)==S_OK;
#endif
}


// フレームの描画を無効にする
bool CAeroGlass::EnableNcRendering(HWND hwnd,bool fEnable)
{
	DWMNCRENDERINGPOLICY ncrp=fEnable?DWMNCRP_USEWINDOWSTYLE:DWMNCRP_DISABLED;

#ifdef WIN_XP_SUPPORT
	if (!LoadDwmLib())
		return false;

	auto pSetWindowAttribute=GET_LIBRARY_FUNCTION(m_hDwmLib,DwmSetWindowAttribute);
	if (pSetWindowAttribute==NULL)
		return false;

	return pSetWindowAttribute(hwnd,DWMWA_NCRENDERING_POLICY,&ncrp,sizeof(ncrp))==S_OK;
#else
	return ::DwmSetWindowAttribute(hwnd,DWMWA_NCRENDERING_POLICY,&ncrp,sizeof(ncrp))==S_OK;
#endif
}




class CBufferedPaintInitializer
{
public:
#ifdef WIN_XP_SUPPORT

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

	bool IsInitialized() const { return m_hThemeLib!=NULL; }

#else	// WIN_XP_SUPPORT

	CBufferedPaintInitializer()
		: m_fInitialized(false)
	{
	}

	~CBufferedPaintInitializer()
	{
		if (m_fInitialized)
			::BufferedPaintUnInit();
	}

	bool Initialize()
	{
		if (!m_fInitialized) {
			if (::BufferedPaintInit()!=S_OK)
				return false;
			m_fInitialized=true;
		}
		return true;
	}

	bool IsInitialized() const { return m_fInitialized; }

private:
	bool m_fInitialized;

#endif
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
	if (!BufferedPaintInitializer.IsInitialized())
		return NULL;

	if (m_hPaintBuffer!=NULL) {
		if (!End(false))
			return NULL;
	}

#ifdef WIN_XP_SUPPORT
	auto pBeginBufferedPaint=
		GET_LIBRARY_FUNCTION(BufferedPaintInitializer.m_hThemeLib,BeginBufferedPaint);
	if (pBeginBufferedPaint==NULL)
		return NULL;
#endif

	BP_PAINTPARAMS Params={sizeof(BP_PAINTPARAMS),0,NULL,NULL};
	if (fErase)
		Params.dwFlags|=BPPF_ERASE;
	HDC hdcBuffer;
	m_hPaintBuffer=
#ifdef WIN_XP_SUPPORT
		pBeginBufferedPaint
#else
		::BeginBufferedPaint
#endif
			(hdc,pRect,BPBF_TOPDOWNDIB,&Params,&hdcBuffer);
	if (m_hPaintBuffer==NULL)
		return NULL;
	return hdcBuffer;
}


bool CBufferedPaint::End(bool fUpdate)
{
	if (m_hPaintBuffer!=NULL) {
#ifdef WIN_XP_SUPPORT
		auto pEndBufferedPaint=
			GET_LIBRARY_FUNCTION(BufferedPaintInitializer.m_hThemeLib,EndBufferedPaint);
		if (pEndBufferedPaint==NULL)
			return false;
		pEndBufferedPaint(m_hPaintBuffer,fUpdate);
#else
		::EndBufferedPaint(m_hPaintBuffer,fUpdate);
#endif
		m_hPaintBuffer=NULL;
	}
	return true;
}


bool CBufferedPaint::Clear(const RECT *pRect)
{
	if (m_hPaintBuffer==NULL)
		return false;
#ifdef WIN_XP_SUPPORT
	auto pBufferedPaintClear=
		GET_LIBRARY_FUNCTION(BufferedPaintInitializer.m_hThemeLib,BufferedPaintClear);
	if (pBufferedPaintClear==NULL)
		return false;
	return pBufferedPaintClear(m_hPaintBuffer,pRect)==S_OK;
#else
	return ::BufferedPaintClear(m_hPaintBuffer,pRect)==S_OK;
#endif
}


bool CBufferedPaint::SetAlpha(BYTE Alpha)
{
	if (m_hPaintBuffer==NULL)
		return false;
#ifdef WIN_XP_SUPPORT
	auto pBufferedPaintSetAlpha=
		GET_LIBRARY_FUNCTION(BufferedPaintInitializer.m_hThemeLib,BufferedPaintSetAlpha);
	if (pBufferedPaintSetAlpha==NULL)
		return false;
	return pBufferedPaintSetAlpha(m_hPaintBuffer,NULL,Alpha)==S_OK;
#else
	return ::BufferedPaintSetAlpha(m_hPaintBuffer,NULL,Alpha)==S_OK;
#endif
}


bool CBufferedPaint::Initialize()
{
	return BufferedPaintInitializer.Initialize();
}


bool CBufferedPaint::IsSupported()
{
	return BufferedPaintInitializer.IsInitialized();
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
